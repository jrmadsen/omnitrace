// MIT License
//
// Copyright (c) 2022 Advanced Micro Devices, Inc. All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "library/rocprofiler-sdk.hpp"
#include "api.hpp"
#include "core/config.hpp"
#include "core/debug.hpp"
#include "core/perfetto.hpp"
#include "core/state.hpp"
#include "library/rocm_smi.hpp"
#include "library/tracing.hpp"

#include <cctype>
#include <rocprofiler-sdk/agent.h>
#include <rocprofiler-sdk/cxx/name_info.hpp>
#include <rocprofiler-sdk/fwd.h>
#include <rocprofiler-sdk/registration.h>
#include <rocprofiler-sdk/rocprofiler.h>

#include <string>
#include <timemory/defines.h>
#include <timemory/utility/demangle.hpp>

#include <atomic>
#include <cstdint>
#include <deque>
#include <iostream>
#include <mutex>
#include <sstream>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#define ROCPROFILER_CALL(result)                                                         \
    {                                                                                    \
        rocprofiler_status_t CHECKSTATUS = (result);                                     \
        if(CHECKSTATUS != ROCPROFILER_STATUS_SUCCESS)                                    \
        {                                                                                \
            auto        msg        = std::stringstream{};                                \
            std::string status_msg = rocprofiler_get_status_string(CHECKSTATUS);         \
            msg << "[" #result "][" << __FILE__ << ":" << __LINE__ << "] "               \
                << "rocprofiler-sdk call [" << #result << "] failed with error code "    \
                << CHECKSTATUS << " :: " << status_msg;                                  \
            OMNITRACE_WARNING(0, "%s\n", msg.str().c_str());                             \
        }                                                                                \
    }

namespace omnitrace
{
namespace rocprofiler_sdk
{
namespace
{
std::string
get_setting_name(std::string _v)
{
    constexpr auto _prefix = tim::string_view_t{ "omnitrace_" };
    for(auto& itr : _v)
        itr = tolower(itr);
    auto _pos = _v.find(_prefix);
    if(_pos == 0) return _v.substr(_prefix.length());
    return _v;
}

#define OMNITRACE_CONFIG_SETTING(TYPE, ENV_NAME, DESCRIPTION, INITIAL_VALUE, ...)        \
    [&]() {                                                                              \
        auto _ret = _config->insert<TYPE, TYPE>(                                         \
            ENV_NAME, get_setting_name(ENV_NAME), DESCRIPTION, TYPE{ INITIAL_VALUE },    \
            std::set<std::string>{ "custom", "omnitrace", "libomnitrace",                \
                                   __VA_ARGS__ });                                       \
        if(!_ret.second)                                                                 \
        {                                                                                \
            OMNITRACE_PRINT("Warning! Duplicate setting: %s / %s\n",                     \
                            get_setting_name(ENV_NAME).c_str(), ENV_NAME);               \
        }                                                                                \
        return _config->find(ENV_NAME)->second;                                          \
    }()

rocprofiler_client_id_t*      rocp_id   = nullptr;
rocprofiler_client_finalize_t rocp_fini = nullptr;
rocprofiler_context_id_t      rocp_ctx  = {};

// buffers
rocprofiler_buffer_id_t kernel_dispatch_buffer = {};
rocprofiler_buffer_id_t memory_copy_buffer     = {};

auto buffers = std::array<rocprofiler_buffer_id_t*, 2>{
    &kernel_dispatch_buffer,
    &memory_copy_buffer,
};

auto agents = std::vector<rocprofiler_agent_t>{};

struct code_object_callback_record_t
{
    uint64_t                                             timestamp = 0;
    rocprofiler_callback_tracing_record_t                record    = {};
    rocprofiler_callback_tracing_code_object_load_data_t payload   = {};
};

using kernel_symbol_data_t =
    rocprofiler_callback_tracing_code_object_kernel_symbol_register_data_t;
using kernel_symbol_map_t =
    std::unordered_map<rocprofiler_kernel_id_t, kernel_symbol_data_t>;

struct kernel_symbol_callback_record_t
{
    uint64_t                              timestamp = 0;
    rocprofiler_callback_tracing_record_t record    = {};
    kernel_symbol_data_t                  payload   = {};
};

using callback_arg_array_t = std::vector<std::pair<std::string, std::string>>;

void
thread_precreate(rocprofiler_runtime_library_t /*lib*/, void* /*tool_data*/)
{
    push_thread_state(ThreadState::Internal);
}

void
thread_postcreate(rocprofiler_runtime_library_t /*lib*/, void* /*tool_data*/)
{
    pop_thread_state();
}

auto code_object_records   = std::vector<code_object_callback_record_t>{};
auto kernel_symbol_records = std::vector<kernel_symbol_callback_record_t*>{};
auto buffered_tracing_info = rocprofiler::sdk::get_buffer_tracing_names();
auto callback_tracing_info = rocprofiler::sdk::get_callback_tracing_names();

int
save_args(rocprofiler_callback_tracing_kind_t /*kind*/, uint32_t /*operation*/,
          uint32_t /*arg_number*/, const void* const /*arg_value_addr*/,
          int32_t /*arg_indirection_count*/, const char* /*arg_type*/,
          const char* arg_name, const char*        arg_value_str,
          int32_t /*arg_dereference_count*/, void* data)
{
    auto* argvec = static_cast<callback_arg_array_t*>(data);
    argvec->emplace_back(arg_name, arg_value_str);
    return 0;
}

void
tool_tracing_callback(rocprofiler_callback_tracing_record_t record,
                      rocprofiler_user_data_t* user_data, void* /*callback_data*/)
{
    auto ts = rocprofiler_timestamp_t{};
    ROCPROFILER_CALL(rocprofiler_get_timestamp(&ts));

    if(record.kind == ROCPROFILER_CALLBACK_TRACING_CODE_OBJECT)
    {
        if(record.phase == ROCPROFILER_CALLBACK_PHASE_ENTER)
        {
            if(record.operation == ROCPROFILER_CODE_OBJECT_LOAD)
            {
                auto data_v =
                    *static_cast<rocprofiler_callback_tracing_code_object_load_data_t*>(
                        record.payload);
                code_object_records.emplace_back(
                    code_object_callback_record_t{ ts, record, data_v });
            }
            else if(record.operation ==
                    ROCPROFILER_CODE_OBJECT_DEVICE_KERNEL_SYMBOL_REGISTER)
            {
                auto data_v = *static_cast<kernel_symbol_data_t*>(record.payload);
                kernel_symbol_records.emplace_back(
                    new kernel_symbol_callback_record_t{ ts, record, data_v });
            }
        }
    }

    if(record.phase == ROCPROFILER_CALLBACK_PHASE_ENTER)
    {
        user_data->value = ts;
    }
    else if(record.phase == ROCPROFILER_CALLBACK_PHASE_EXIT)
    {
        if(get_use_perfetto())
        {
            auto args = callback_arg_array_t{};
            if(config::get_perfetto_annotations())
            {
                rocprofiler_iterate_callback_tracing_kind_operation_args(
                    record, save_args, 2, &args);
            }

            auto _name = callback_tracing_info.at(record.kind, record.operation);

            uint64_t _beg_ts = user_data->value;
            uint64_t _end_ts = ts;
            tracing::push_perfetto_ts(
                category::rocm_hsa{}, _name.data(), _beg_ts,
                ::perfetto::Flow::ProcessScoped(record.correlation_id.internal),
                [&](::perfetto::EventContext ctx) {
                    if(config::get_perfetto_annotations())
                    {
                        tracing::add_perfetto_annotation(ctx, "begin_ns", _beg_ts);

                        for(const auto& [key, val] : args)
                        {
                            tracing::add_perfetto_annotation(ctx, key, val);
                        }
                    }
                });
            tracing::pop_perfetto_ts(category::rocm_hsa{}, _name.data(), _end_ts,
                                     [&](::perfetto::EventContext ctx) {
                                         if(config::get_perfetto_annotations())
                                         {
                                             tracing::add_perfetto_annotation(
                                                 ctx, "end_ns", _end_ts);
                                         }
                                     });
        }
    }
}

void
tool_tracing_buffered(rocprofiler_context_id_t /*context*/,
                      rocprofiler_buffer_id_t /*buffer_id*/,
                      rocprofiler_record_header_t** headers, size_t num_headers,
                      void* /*user_data*/, uint64_t /*drop_count*/)
{
    if(num_headers == 0 || headers == nullptr) return;

    for(size_t i = 0; i < num_headers; ++i)
    {
        auto* header = headers[i];

        if(header == nullptr) continue;

        if(header->category == ROCPROFILER_BUFFER_CATEGORY_TRACING)
        {
            if(header->kind == ROCPROFILER_BUFFER_TRACING_KERNEL_DISPATCH)
            {
                auto* record =
                    static_cast<rocprofiler_buffer_tracing_kernel_dispatch_record_t*>(
                        header->payload);

                const kernel_symbol_data_t* _kern_sym_data = nullptr;
                for(const auto& itr : kernel_symbol_records)
                {
                    if(record->dispatch_info.kernel_id == itr->payload.kernel_id)
                    {
                        _kern_sym_data = &itr->payload;
                        break;
                    }
                }

                auto                       _corr_id  = record->correlation_id.internal;
                auto                       _beg_ns   = record->start_timestamp;
                auto                       _end_ns   = record->end_timestamp;
                auto                       _agent_id = record->dispatch_info.agent_id;
                auto                       _queue_id = record->dispatch_info.queue_id;
                const rocprofiler_agent_t* _agent    = nullptr;

                for(const auto& itr : agents)
                {
                    if(itr.id.handle == _agent_id.handle)
                    {
                        _agent = &itr;
                        break;
                    }
                }

                if(get_use_perfetto())
                {
                    auto _track_desc = [](int32_t _device_id_v, int64_t _queue_id_v) {
                        if(config::get_perfetto_roctracer_per_stream())
                            return JOIN("", "ROCm GPU Activity Device ", _device_id_v,
                                        ", Queue ", _queue_id_v);
                        return JOIN("", "ROCm GPU Activity Device ", _device_id_v);
                    };

                    const auto _track =
                        tracing::get_perfetto_track(category::device_hip{}, _track_desc,
                                                    _agent->node_id, _queue_id.handle);

                    tracing::push_perfetto_track(
                        category::device_hip{},
                        tim::demangle(_kern_sym_data->kernel_name).c_str(), _track,
                        _beg_ns, ::perfetto::Flow::ProcessScoped(_corr_id),
                        [&](::perfetto::EventContext ctx) {
                            if(config::get_perfetto_annotations())
                            {
                                tracing::add_perfetto_annotation(ctx, "begin_ns",
                                                                 _beg_ns);
                                tracing::add_perfetto_annotation(ctx, "end_ns", _end_ns);
                                tracing::add_perfetto_annotation(ctx, "corr_id",
                                                                 _corr_id);
                                tracing::add_perfetto_annotation(ctx, "device",
                                                                 _agent->node_id);
                                tracing::add_perfetto_annotation(ctx, "queue",
                                                                 _queue_id.handle);
                                tracing::add_perfetto_annotation(
                                    ctx, "kernel_id", record->dispatch_info.kernel_id);
                                tracing::add_perfetto_annotation(
                                    ctx, "private_segment_size",
                                    record->dispatch_info.private_segment_size);
                                tracing::add_perfetto_annotation(
                                    ctx, "group_segment_size",
                                    record->dispatch_info.group_segment_size);
                                tracing::add_perfetto_annotation(
                                    ctx, "workgroup_size",
                                    JOIN("", "(",
                                         JOIN(',', record->dispatch_info.workgroup_size.x,
                                              record->dispatch_info.workgroup_size.y,
                                              record->dispatch_info.workgroup_size.z),
                                         ")"));
                                tracing::add_perfetto_annotation(
                                    ctx, "grid_size",
                                    JOIN("", "(",
                                         JOIN(',', record->dispatch_info.grid_size.x,
                                              record->dispatch_info.grid_size.y,
                                              record->dispatch_info.grid_size.z),
                                         ")"));
                            }
                        });
                    tracing::pop_perfetto_track(category::device_hip{}, "", _track,
                                                _end_ns);
                }
            }
            else if(header->kind == ROCPROFILER_BUFFER_TRACING_MEMORY_COPY)
            {
                auto* record =
                    static_cast<rocprofiler_buffer_tracing_memory_copy_record_t*>(
                        header->payload);

                auto _corr_id      = record->correlation_id.internal;
                auto _beg_ns       = record->start_timestamp;
                auto _end_ns       = record->end_timestamp;
                auto _dst_agent_id = record->dst_agent_id;
                auto _src_agent_id = record->src_agent_id;

                const rocprofiler_agent_t* _dst_agent = nullptr;
                const rocprofiler_agent_t* _src_agent = nullptr;
                const rocprofiler_agent_t* _agent     = nullptr;

                for(const auto& itr : agents)
                {
                    if(itr.id.handle == _dst_agent_id.handle) _dst_agent = &itr;

                    if(itr.id.handle == _src_agent_id.handle) _src_agent = &itr;

                    if(_dst_agent && _src_agent) break;
                }

                if(_dst_agent && _dst_agent->type == ROCPROFILER_AGENT_TYPE_GPU)
                    _agent = _dst_agent;
                else if(_src_agent && _src_agent->type == ROCPROFILER_AGENT_TYPE_GPU)
                    _agent = _src_agent;

                if(get_use_perfetto())
                {
                    auto _track_desc = [](int32_t _device_id_v) {
                        return JOIN("", "ROCm GPU Copy Device ", _device_id_v);
                    };

                    const auto _track = tracing::get_perfetto_track(
                        category::device_hip{}, _track_desc, _agent->node_id);

                    tracing::push_perfetto_track(
                        category::device_hip{},
                        buffered_tracing_info.at(record->kind, record->operation).data(),
                        _track, _beg_ns, ::perfetto::Flow::ProcessScoped(_corr_id),
                        [&](::perfetto::EventContext ctx) {
                            if(config::get_perfetto_annotations())
                            {
                                tracing::add_perfetto_annotation(ctx, "begin_ns",
                                                                 _beg_ns);
                                tracing::add_perfetto_annotation(ctx, "end_ns", _end_ns);
                                tracing::add_perfetto_annotation(ctx, "corr_id",
                                                                 _corr_id);
                                tracing::add_perfetto_annotation(ctx, "dst_agent",
                                                                 _dst_agent->node_id);
                                tracing::add_perfetto_annotation(ctx, "src_agent",
                                                                 _src_agent->node_id);
                            }
                        });
                    tracing::pop_perfetto_track(category::device_hip{}, "", _track,
                                                _end_ns);
                }
            }
            else
            {
                throw std::runtime_error{
                    "unexpected rocprofiler_record_header_t tracing category kind"
                };
            }
        }
        else
        {
            throw std::runtime_error{
                "unexpected rocprofiler_record_header_t category + kind"
            };
        }
    }
}

bool
is_active(rocprofiler_context_id_t ctx)
{
    int  status = 0;
    auto errc   = rocprofiler_context_is_active(ctx, &status);
    return (errc == ROCPROFILER_STATUS_SUCCESS && status > 0);
}

void
start()
{
    if(!is_active(rocp_ctx))
    {
        ROCPROFILER_CALL(rocprofiler_start_context(rocp_ctx));
    }
}

void
stop()
{
    if(is_active(rocp_ctx))
    {
        ROCPROFILER_CALL(rocprofiler_stop_context(rocp_ctx));
    }
}

void
flush()
{
    for(auto* itr : buffers)
    {
        if(itr && itr->handle > 0)
        {
            auto status = rocprofiler_flush_buffer(*itr);
            if(status != ROCPROFILER_STATUS_ERROR_BUFFER_BUSY)
            {
                ROCPROFILER_CALL(status);
            }
        }
    }
}

int
tool_init(rocprofiler_client_finalize_t fini_func, void* tool_data)
{
    if(!config::settings_are_configured() && get_state() < State::Active)
        omnitrace_init_tooling_hidden();

    if(config::get_use_process_sampling() && config::get_use_rocm_smi())
    {
        OMNITRACE_VERBOSE_F(1, "Setting rocm_smi state to active...\n");
        rocm_smi::set_state(State::Active);
    }

    rocprofiler_query_available_agents_cb_t iterate_cb =
        [](rocprofiler_agent_version_t /*version*/, const void** agents_arr,
           size_t num_agents, void* user_data) {
            auto* agents_v = static_cast<std::vector<rocprofiler_agent_t>*>(user_data);
            for(size_t i = 0; i < num_agents; ++i)
                agents_v->emplace_back(
                    *static_cast<const rocprofiler_agent_v0_t*>(agents_arr[i]));
            return ROCPROFILER_STATUS_SUCCESS;
        };

    ROCPROFILER_CALL(
        rocprofiler_query_available_agents(ROCPROFILER_AGENT_INFO_VERSION_0, iterate_cb,
                                           sizeof(rocprofiler_agent_t), &agents));

    rocp_fini = fini_func;

    ROCPROFILER_CALL(rocprofiler_create_context(&rocp_ctx));

    for(auto itr : { ROCPROFILER_CALLBACK_TRACING_HSA_CORE_API,
                     ROCPROFILER_CALLBACK_TRACING_HSA_AMD_EXT_API,
                     ROCPROFILER_CALLBACK_TRACING_HSA_IMAGE_EXT_API,
                     ROCPROFILER_CALLBACK_TRACING_HSA_FINALIZE_EXT_API })
    {
        ROCPROFILER_CALL(rocprofiler_configure_callback_tracing_service(
            rocp_ctx, itr, nullptr, 0, tool_tracing_callback, nullptr));
    }

    ROCPROFILER_CALL(rocprofiler_configure_callback_tracing_service(
        rocp_ctx, ROCPROFILER_CALLBACK_TRACING_HIP_RUNTIME_API, nullptr, 0,
        tool_tracing_callback, nullptr));

    ROCPROFILER_CALL(rocprofiler_configure_callback_tracing_service(
        rocp_ctx, ROCPROFILER_CALLBACK_TRACING_CODE_OBJECT, nullptr, 0,
        tool_tracing_callback, nullptr));

    ROCPROFILER_CALL(rocprofiler_configure_callback_tracing_service(
        rocp_ctx, ROCPROFILER_CALLBACK_TRACING_MARKER_CORE_API, nullptr, 0,
        tool_tracing_callback, nullptr));

    ROCPROFILER_CALL(rocprofiler_configure_callback_tracing_service(
        rocp_ctx, ROCPROFILER_CALLBACK_TRACING_MARKER_CONTROL_API, nullptr, 0,
        tool_tracing_callback, nullptr));

    ROCPROFILER_CALL(rocprofiler_configure_callback_tracing_service(
        rocp_ctx, ROCPROFILER_CALLBACK_TRACING_MARKER_NAME_API, nullptr, 0,
        tool_tracing_callback, nullptr));

    constexpr auto buffer_size = 8192;
    constexpr auto watermark   = 7936;

    ROCPROFILER_CALL(rocprofiler_create_buffer(
        rocp_ctx, buffer_size, watermark, ROCPROFILER_BUFFER_POLICY_LOSSLESS,
        tool_tracing_buffered, tool_data, &kernel_dispatch_buffer));

    ROCPROFILER_CALL(rocprofiler_create_buffer(
        rocp_ctx, buffer_size, watermark, ROCPROFILER_BUFFER_POLICY_LOSSLESS,
        tool_tracing_buffered, tool_data, &memory_copy_buffer));

    ROCPROFILER_CALL(rocprofiler_configure_buffer_tracing_service(
        rocp_ctx, ROCPROFILER_BUFFER_TRACING_KERNEL_DISPATCH, nullptr, 0,
        kernel_dispatch_buffer));

    ROCPROFILER_CALL(rocprofiler_configure_buffer_tracing_service(
        rocp_ctx, ROCPROFILER_BUFFER_TRACING_MEMORY_COPY, nullptr, 0,
        memory_copy_buffer));

    auto client_thread = rocprofiler_callback_thread_t{};
    ROCPROFILER_CALL(rocprofiler_create_callback_thread(&client_thread));

    for(auto* itr : buffers)
    {
        ROCPROFILER_CALL(rocprofiler_assign_callback_thread(*itr, client_thread));
    }

    {
        int valid_ctx = 0;
        ROCPROFILER_CALL(rocprofiler_context_is_valid(rocp_ctx, &valid_ctx));
        if(valid_ctx == 0)
        {
            // notify rocprofiler that initialization failed
            // and all the contexts, buffers, etc. created
            // should be ignored
            return -1;
        }
    }

    start();

    // no errors
    return 0;
}

void
tool_fini(void* /*tool_data*/)
{
    static std::atomic_flag _once = ATOMIC_FLAG_INIT;
    if(_once.test_and_set()) return;

    stop();
    flush();

    std::cerr << "[" << getpid() << "][" << __FUNCTION__ << "] Finalization complete.\n"
              << std::flush;
}
}  // namespace

void
config_settings(const std::shared_ptr<settings>& _config)
{
    const auto _skip_domains =
        std::unordered_set<std::string_view>{ "marker_core_api", "marker_control_api",
                                              "marker_name_api" };
    auto _domain_choices = std::vector<std::string>{};
    auto _add_domain     = [&_domain_choices, &_skip_domains](std::string_view _domain) {
        auto _v = std::string{ _domain };
        for(auto& itr : _v)
            itr = ::tolower(itr);

        if(_skip_domains.count(_v) == 0)
        {
            auto itr = std::find(_domain_choices.begin(), _domain_choices.end(), _v);
            if(itr == _domain_choices.end()) _domain_choices.emplace_back(_v);
        }
    };

    _domain_choices.reserve(buffered_tracing_info.size());
    _domain_choices.reserve(callback_tracing_info.size());
    _add_domain("hip_api");
    _add_domain("hsa_api");

    for(const auto& itr : buffered_tracing_info)
        _add_domain(itr.name);

    for(const auto& itr : callback_tracing_info)
        _add_domain(itr.name);

    OMNITRACE_CONFIG_SETTING(
        std::string, "OMNITRACE_ROCM_DOMAINS",
        "Specification of ROCm domains to trace/profile",
        std::string{ "hip,kernels,marker,scratch_memory,page_migration" }, "rocm")
        ->set_choices(_domain_choices);

    OMNITRACE_CONFIG_SETTING(
        std::string, "OMNITRACE_ROCM_EVENTS",
        "ROCm hardware counters. Use ':device=N' syntax to specify collection on device "
        "number N, e.g. ':device=0'. If no device specification is provided, the event "
        "is collected on every available device",
        "", "rocm", "hardware_counters");
}

void
setup()
{
    if(int status = 0;
       rocprofiler_is_initialized(&status) == ROCPROFILER_STATUS_SUCCESS && status == 0)
    {
        ROCPROFILER_CALL(rocprofiler_force_configure(&rocprofiler_configure));
    }
}

void
shutdown()
{
    // shutdown
}
}  // namespace rocprofiler_sdk
}  // namespace omnitrace

extern "C" rocprofiler_tool_configure_result_t*
rocprofiler_configure(uint32_t version, const char* runtime_version, uint32_t priority,
                      rocprofiler_client_id_t* id)
{
    // only activate once
    {
        static bool _first = true;
        if(!_first) return nullptr;
        _first = false;
    }

    (void) priority;

    // set the client name
    id->name = "Omnitrace";

    // store client info
    ::omnitrace::rocprofiler_sdk::rocp_id = id;

    // compute major/minor/patch version info
    uint32_t major = version / 10000;
    uint32_t minor = (version % 10000) / 100;
    uint32_t patch = version % 100;

    // generate info string
    auto info = std::stringstream{};
    info << id->name << " is using rocprofiler-sdk v" << major << "." << minor << "."
         << patch << " (" << runtime_version << ")";

    OMNITRACE_VERBOSE_F(0, "%s\n", info.str().c_str());
    OMNITRACE_VERBOSE_F(2, "client_id=%u, priority=%u\n", id->handle, priority);

    ROCPROFILER_CALL(rocprofiler_at_internal_thread_create(
        omnitrace::rocprofiler_sdk::thread_precreate,
        omnitrace::rocprofiler_sdk::thread_postcreate,
        ROCPROFILER_LIBRARY | ROCPROFILER_HSA_LIBRARY | ROCPROFILER_HIP_LIBRARY |
            ROCPROFILER_MARKER_LIBRARY,
        nullptr));

    // create configure data
    static auto cfg =
        rocprofiler_tool_configure_result_t{ sizeof(rocprofiler_tool_configure_result_t),
                                             &::omnitrace::rocprofiler_sdk::tool_init,
                                             &::omnitrace::rocprofiler_sdk::tool_fini,
                                             nullptr };

    // return pointer to configure data
    return &cfg;
}