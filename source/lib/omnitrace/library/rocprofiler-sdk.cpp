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
#include "common/synchronized.hpp"
#include "core/config.hpp"
#include "core/containers/stable_vector.hpp"
#include "core/debug.hpp"
#include "core/gpu.hpp"
#include "core/perfetto.hpp"
#include "core/rocprofiler-sdk.hpp"
#include "core/state.hpp"
#include "library/components/category_region.hpp"
#include "library/rocm_smi.hpp"
#include "library/rocprofiler-sdk/counters.hpp"
#include "library/rocprofiler-sdk/fwd.hpp"
#include "library/thread_info.hpp"
#include "library/tracing.hpp"

#include <rocprofiler-sdk/agent.h>
#include <rocprofiler-sdk/cxx/hash.hpp>
#include <rocprofiler-sdk/cxx/name_info.hpp>
#include <rocprofiler-sdk/cxx/operators.hpp>
#include <rocprofiler-sdk/fwd.h>
#include <rocprofiler-sdk/marker/api_id.h>
#include <rocprofiler-sdk/registration.h>
#include <rocprofiler-sdk/rocprofiler.h>

#include <timemory/defines.h>
#include <timemory/process/threading.hpp>
#include <timemory/utility/demangle.hpp>
#include <timemory/utility/types.hpp>

#include <atomic>
#include <cctype>
#include <cstdint>
#include <deque>
#include <iostream>
#include <mutex>
#include <regex>
#include <sstream>
#include <string>
#include <unistd.h>
#include <unordered_map>
#include <vector>

namespace omnitrace
{
namespace rocprofiler_sdk
{
namespace
{
using tool_agent_vec_t = std::vector<tool_agent>;
client_data* tool_data = new client_data{};

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

// this function creates a rocprofiler profile config on the first entry
std::vector<rocprofiler_counter_id_t>
create_agent_profile(rocprofiler_agent_id_t          agent_id,
                     const std::vector<std::string>& counters,
                     //  const tool_agent_vec_t&         gpu_agents,
                     //  const agent_counter_info_map_t& counters_info,
                     //  agent_counter_profile_map_t&    data)
                     client_data* data = tool_data)
{
    using counter_vec_t = std::vector<rocprofiler_counter_id_t>;

    // check if already created
    auto itr = data->agent_counter_profiles.find(agent_id);
    if(itr != data->agent_counter_profiles.end()) return counter_vec_t{};

    auto        profile      = std::optional<rocprofiler_profile_config_id_t>{};
    auto        expected_v   = counters.size();
    auto        found_v      = std::vector<std::string_view>{};
    auto        counters_v   = counter_vec_t{};
    const auto* tool_agent_v = data->get_gpu_tool_agent(agent_id);

    constexpr auto device_qualifier = std::string_view{ ":device=" };
    for(const auto& itr : counters)
    {
        auto name_v = itr;
        if(auto pos = std::string::npos;
           (pos = itr.find(device_qualifier)) != std::string::npos)
        {
            name_v        = itr.substr(0, pos);
            auto dev_id_s = itr.substr(pos + device_qualifier.length());

            OMNITRACE_CONDITIONAL_ABORT(dev_id_s.empty() ||
                                            dev_id_s.find_first_not_of("0123456789") !=
                                                std::string::npos,
                                        "invalid device qualifier format (':device=N) "
                                        "where N is the GPU id: %s\n",
                                        itr.c_str());

            auto dev_id_v = std::stoul(dev_id_s);

            OMNITRACE_PRINT_F("tool agent device id=%lu, name=%s, device_id=%lu\n",
                              tool_agent_v->device_id, name_v.c_str(), dev_id_v);
            // skip this counter if the counter is for a specific device id (which
            // doesn't this agent's device id)
            if(dev_id_v != tool_agent_v->device_id)
            {
                --expected_v;  // is not expected
                continue;
            }
        }

        auto _old_name_v = name_v;
        name_v =
            std::regex_replace(name_v, std::regex{ "^(.*)(\\[)([0-9]+)(\\])$" }, "$1");
        if(name_v != _old_name_v)
            OMNITRACE_PRINT_F("tool agent device id=%lu, old_name=%s, name=%s\n",
                              tool_agent_v->device_id, _old_name_v.c_str(),
                              name_v.c_str());

        // search the gpu agent counter info for a counter with a matching name
        for(const auto& citr : data->agent_counter_info.at(agent_id))
        {
            if(name_v == std::string_view{ citr.name })
            {
                counters_v.emplace_back(citr.id);
                found_v.emplace_back(itr);
            }
        }
    }

    if(counters_v.size() != expected_v)
    {
        auto requested_counters =
            timemory::join::join(timemory::join::array_config{ ", ", "", "" }, counters);
        auto found_counters =
            timemory::join::join(timemory::join::array_config{ ", ", "", "" }, found_v);

        OMNITRACE_ABORT_F(
            "Unable to find all counters for agent %i (gpu-%li, %s) in %s. Found: %s\n",
            tool_agent_v->agent->node_id, tool_agent_v->device_id,
            tool_agent_v->agent->name, requested_counters.c_str(),
            found_counters.c_str());
    }

    if(!counters_v.empty())
    {
        auto profile_v = rocprofiler_profile_config_id_t{};
        ROCPROFILER_CALL(rocprofiler_create_profile_config(
            agent_id, counters_v.data(), counters_v.size(), &profile_v));
        profile = profile_v;
    }

    data->agent_counter_profiles.emplace(agent_id, profile);

    return counters_v;
}

const kernel_symbol_data_t*
get_kernel_symbol_info(uint64_t _kernel_id)
{
    return tool_data->get_kernel_symbol_info(_kernel_id);
}

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

template <typename CategoryT>
void
tool_tracing_callback_impl(rocprofiler_callback_tracing_record_t record,
                           rocprofiler_user_data_t* user_data, rocprofiler_timestamp_t ts)
{
    if(get_use_perfetto())
    {
        auto args = callback_arg_array_t{};
        if(config::get_perfetto_annotations())
        {
            rocprofiler_iterate_callback_tracing_kind_operation_args(record, save_args, 2,
                                                                     &args);
        }

        auto _name = tool_data->callback_tracing_info.at(record.kind, record.operation);

        uint64_t _beg_ts = user_data->value;
        uint64_t _end_ts = ts;

        tracing::push_perfetto_ts(
            CategoryT{}, _name.data(), _beg_ts,
            ::perfetto::Flow::ProcessScoped(record.correlation_id.internal),
            [&](::perfetto::EventContext ctx) {
                if(config::get_perfetto_annotations())
                {
                    tracing::add_perfetto_annotation(ctx, "begin_ns", _beg_ts);

                    for(const auto& [key, val] : args)
                        tracing::add_perfetto_annotation(ctx, key, val);
                }
            });
        tracing::pop_perfetto_ts(
            CategoryT{}, _name.data(), _end_ts, [&](::perfetto::EventContext ctx) {
                if(config::get_perfetto_annotations())
                    tracing::add_perfetto_annotation(ctx, "end_ns", _end_ts);
            });
    }
}

void
tool_control_callback(rocprofiler_callback_tracing_record_t record,
                      rocprofiler_user_data_t* /*user_data*/, void* /*callback_data*/)
{
    if(record.kind == ROCPROFILER_CALLBACK_TRACING_MARKER_CONTROL_API)
    {
        if(record.operation == ROCPROFILER_MARKER_CONTROL_API_ID_roctxProfilerPause &&
           record.phase == ROCPROFILER_CALLBACK_PHASE_ENTER)
        {
            stop();
        }
        else if(record.operation ==
                    ROCPROFILER_MARKER_CONTROL_API_ID_roctxProfilerResume &&
                record.phase == ROCPROFILER_CALLBACK_PHASE_EXIT)
        {
            start();
        }
    }
}

void
tool_code_object_callback(rocprofiler_callback_tracing_record_t record,
                          rocprofiler_user_data_t* /*user_data*/, void* /*callback_data*/)
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
                tool_data->code_object_records.wlock([ts, &record, &data_v](auto& _data) {
                    _data.emplace_back(
                        code_object_callback_record_t{ ts, record, data_v });
                });
            }
            else if(record.operation ==
                    ROCPROFILER_CODE_OBJECT_DEVICE_KERNEL_SYMBOL_REGISTER)
            {
                auto data_v = *static_cast<kernel_symbol_data_t*>(record.payload);
                tool_data->kernel_symbol_records.wlock(
                    [ts, &record, &data_v](auto& _data) {
                        _data.emplace_back(
                            new kernel_symbol_callback_record_t{ ts, record, data_v });
                    });
            }
        }
        return;
    }
}

void
tool_tracing_callback(rocprofiler_callback_tracing_record_t record,
                      rocprofiler_user_data_t* user_data, void* /*callback_data*/)
{
    auto ts = rocprofiler_timestamp_t{};
    ROCPROFILER_CALL(rocprofiler_get_timestamp(&ts));

    if(record.phase == ROCPROFILER_CALLBACK_PHASE_ENTER)
    {
        user_data->value = ts;

        switch(record.kind)
        {
            case ROCPROFILER_CALLBACK_TRACING_HSA_CORE_API:
            case ROCPROFILER_CALLBACK_TRACING_HSA_AMD_EXT_API:
            case ROCPROFILER_CALLBACK_TRACING_HSA_IMAGE_EXT_API:
            case ROCPROFILER_CALLBACK_TRACING_HSA_FINALIZE_EXT_API:
            {
                auto _name =
                    tool_data->callback_tracing_info.at(record.kind, record.operation);
                component::category_region<category::rocm_hsa_api>::start<
                    quirk::timemory>(_name);
                break;
            }
            case ROCPROFILER_CALLBACK_TRACING_HIP_RUNTIME_API:
            case ROCPROFILER_CALLBACK_TRACING_HIP_COMPILER_API:
            {
                auto _name =
                    tool_data->callback_tracing_info.at(record.kind, record.operation);
                component::category_region<category::rocm_hip_api>::start<
                    quirk::timemory>(_name);
                break;
            }
            case ROCPROFILER_CALLBACK_TRACING_MARKER_CORE_API:
            {
                auto _name =
                    tool_data->callback_tracing_info.at(record.kind, record.operation);
                component::category_region<category::rocm_roctx>::start<quirk::timemory>(
                    _name);
                break;
            }
            case ROCPROFILER_CALLBACK_TRACING_NONE:
            case ROCPROFILER_CALLBACK_TRACING_LAST:
            case ROCPROFILER_CALLBACK_TRACING_MARKER_CONTROL_API:
            case ROCPROFILER_CALLBACK_TRACING_MARKER_NAME_API:
            case ROCPROFILER_CALLBACK_TRACING_CODE_OBJECT:
            case ROCPROFILER_CALLBACK_TRACING_SCRATCH_MEMORY:
            case ROCPROFILER_CALLBACK_TRACING_KERNEL_DISPATCH:
            case ROCPROFILER_CALLBACK_TRACING_MEMORY_COPY:
            {
                break;
            }
        }
    }
    else if(record.phase == ROCPROFILER_CALLBACK_PHASE_EXIT)
    {
        switch(record.kind)
        {
            case ROCPROFILER_CALLBACK_TRACING_HSA_CORE_API:
            case ROCPROFILER_CALLBACK_TRACING_HSA_AMD_EXT_API:
            case ROCPROFILER_CALLBACK_TRACING_HSA_IMAGE_EXT_API:
            case ROCPROFILER_CALLBACK_TRACING_HSA_FINALIZE_EXT_API:
            {
                tool_tracing_callback_impl<category::rocm_hsa_api>(record, user_data, ts);
                break;
            }
            case ROCPROFILER_CALLBACK_TRACING_HIP_RUNTIME_API:
            case ROCPROFILER_CALLBACK_TRACING_HIP_COMPILER_API:
            {
                tool_tracing_callback_impl<category::rocm_hip_api>(record, user_data, ts);
                break;
            }
            case ROCPROFILER_CALLBACK_TRACING_MARKER_CORE_API:
            {
                tool_tracing_callback_impl<category::rocm_roctx>(record, user_data, ts);
                break;
            }
            case ROCPROFILER_CALLBACK_TRACING_NONE:
            case ROCPROFILER_CALLBACK_TRACING_LAST:
            case ROCPROFILER_CALLBACK_TRACING_MARKER_CONTROL_API:
            case ROCPROFILER_CALLBACK_TRACING_MARKER_NAME_API:
            case ROCPROFILER_CALLBACK_TRACING_CODE_OBJECT:
            case ROCPROFILER_CALLBACK_TRACING_SCRATCH_MEMORY:
            case ROCPROFILER_CALLBACK_TRACING_KERNEL_DISPATCH:
            case ROCPROFILER_CALLBACK_TRACING_MEMORY_COPY:
            {
                break;
            }
        }

        switch(record.kind)
        {
            case ROCPROFILER_CALLBACK_TRACING_HSA_CORE_API:
            case ROCPROFILER_CALLBACK_TRACING_HSA_AMD_EXT_API:
            case ROCPROFILER_CALLBACK_TRACING_HSA_IMAGE_EXT_API:
            case ROCPROFILER_CALLBACK_TRACING_HSA_FINALIZE_EXT_API:
            {
                auto _name =
                    tool_data->callback_tracing_info.at(record.kind, record.operation);
                component::category_region<category::rocm_hsa_api>::stop<quirk::timemory>(
                    _name);
                break;
            }
            case ROCPROFILER_CALLBACK_TRACING_HIP_RUNTIME_API:
            case ROCPROFILER_CALLBACK_TRACING_HIP_COMPILER_API:
            {
                auto _name =
                    tool_data->callback_tracing_info.at(record.kind, record.operation);
                component::category_region<category::rocm_hip_api>::stop<quirk::timemory>(
                    _name);
                break;
            }
            case ROCPROFILER_CALLBACK_TRACING_MARKER_CORE_API:
            {
                auto _name =
                    tool_data->callback_tracing_info.at(record.kind, record.operation);
                component::category_region<category::rocm_roctx>::stop<quirk::timemory>(
                    _name);
                break;
            }
            case ROCPROFILER_CALLBACK_TRACING_NONE:
            case ROCPROFILER_CALLBACK_TRACING_LAST:
            case ROCPROFILER_CALLBACK_TRACING_MARKER_CONTROL_API:
            case ROCPROFILER_CALLBACK_TRACING_MARKER_NAME_API:
            case ROCPROFILER_CALLBACK_TRACING_CODE_OBJECT:
            case ROCPROFILER_CALLBACK_TRACING_SCRATCH_MEMORY:
            case ROCPROFILER_CALLBACK_TRACING_KERNEL_DISPATCH:
            case ROCPROFILER_CALLBACK_TRACING_MEMORY_COPY:
            {
                break;
            }
        }
    }
}

auto&
get_kernel_dispatch_timestamps()
{
    static auto _v = std::unordered_map<rocprofiler_dispatch_id_t, timing_interval>{};
    return _v;
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

                // save for post-processing
                get_kernel_dispatch_timestamps().emplace(
                    record->dispatch_info.dispatch_id,
                    timing_interval{ record->start_timestamp, record->end_timestamp });

                const auto* _kern_sym_data =
                    get_kernel_symbol_info(record->dispatch_info.kernel_id);

                auto                       _corr_id  = record->correlation_id.internal;
                auto                       _beg_ns   = record->start_timestamp;
                auto                       _end_ns   = record->end_timestamp;
                auto                       _agent_id = record->dispatch_info.agent_id;
                auto                       _queue_id = record->dispatch_info.queue_id;
                const rocprofiler_agent_t* _agent    = nullptr;

                for(const auto& itr : tool_data->agents)
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
                        return JOIN("", "ROCm GPU Activity Device ", _device_id_v,
                                    ", Queue ", _queue_id_v);
                    };

                    const auto _track = tracing::get_perfetto_track(
                        category::rocm_kernel_dispatch{}, _track_desc, _agent->node_id,
                        _queue_id.handle);

                    auto _name = tim::demangle(_kern_sym_data->kernel_name);
                    component::category_region<category::rocm_kernel_dispatch>::start<
                        quirk::perfetto>(
                        _name, _track, _beg_ns, ::perfetto::Flow::ProcessScoped(_corr_id),
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
                    component::category_region<category::rocm_kernel_dispatch>::stop<
                        quirk::perfetto>(_name, _track, _end_ns);
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

                for(const auto& itr : tool_data->agents)
                {
                    if(itr.id.handle == _dst_agent_id.handle) _dst_agent = &itr;
                    if(itr.id.handle == _src_agent_id.handle) _src_agent = &itr;
                    if(_dst_agent && _src_agent) break;
                }

                if(get_use_perfetto())
                {
                    auto _track_desc = [](int32_t                 _device_id_v,
                                          rocprofiler_thread_id_t _tid) {
                        const auto& _tid_v = thread_info::get(_tid, SystemTID);
                        return JOIN("", "ROCm Copy to Device ", _device_id_v, ", Thread ",
                                    _tid_v->index_data->sequent_value);
                    };

                    const auto _track = tracing::get_perfetto_track(
                        category::rocm_memory_copy{}, _track_desc,
                        _dst_agent->logical_node_id, record->thread_id);

                    tracing::push_perfetto_track(
                        category::rocm_memory_copy{},
                        tool_data->buffered_tracing_info
                            .at(record->kind, record->operation)
                            .data(),
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
                    tracing::pop_perfetto_track(category::rocm_memory_copy{}, "", _track,
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
        else if(header->category == ROCPROFILER_BUFFER_CATEGORY_COUNTERS)
        {
            if(header->kind ==
               ROCPROFILER_COUNTER_RECORD_PROFILE_COUNTING_DISPATCH_HEADER)
            {}
            else if(header->kind == ROCPROFILER_COUNTER_RECORD_VALUE)
            {
                // Print the returned counter data.
                auto* record =
                    static_cast<rocprofiler_record_counter_t*>(header->payload);
                OMNITRACE_PRINT_F("id: %lu, value: %f, dispatch_id=%lu, ts=%zu\n",
                                  record->id, record->counter_value, record->dispatch_id,
                                  record->user_data.value);
            }
            else
            {
                throw std::runtime_error{ "unexpected rocprofiler_record_header_t "
                                          "profile counting category kind" };
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

auto&
get_counter_dispatch_data()
{
    static auto _v =
        container::stable_vector<rocprofiler_profile_counting_dispatch_data_t>{};
    return _v;
}

auto&
get_counter_dispatch_records()
{
    static auto _v = std::vector<counter_dispatch_record>{};
    return _v;
}

using counter_storage_map_t =
    std::unordered_map<rocprofiler_counter_id_t, counter_storage>;
using agent_counter_storage_map_t =
    std::unordered_map<rocprofiler_agent_id_t, counter_storage_map_t>;

auto*&
get_counter_storage()
{
    static auto* _v = new agent_counter_storage_map_t{};
    return _v;
}

void
counter_record_callback(rocprofiler_profile_counting_dispatch_data_t dispatch_data,
                        rocprofiler_record_counter_t* record_data, size_t record_count,
                        rocprofiler_user_data_t /*user_data*/,
                        void* /*callback_data_arg*/)
{
    auto* _agent_counter_storage = get_counter_storage();
    if(!_agent_counter_storage) return;

    auto _dispatch_id = dispatch_data.dispatch_info.dispatch_id;

    auto _aggregate =
        std::unordered_map<rocprofiler_counter_id_t, rocprofiler_record_counter_t>{};
    for(size_t i = 0; i < record_count; ++i)
    {
        auto _counter_id = rocprofiler_counter_id_t{};
        ROCPROFILER_CALL(
            rocprofiler_query_record_counter_id(record_data[i].id, &_counter_id));

        if(!_aggregate.emplace(_counter_id, record_data[i]).second)
        {
            _aggregate[_counter_id].counter_value += record_data[i].counter_value;
        }
    }

    auto _agent_id = dispatch_data.dispatch_info.agent_id;
    // auto _queue_id = dispatch_data.dispatch_info.queue_id;

    if(_agent_counter_storage->count(_agent_id) == 0)
        _agent_counter_storage->emplace(_agent_id, counter_storage_map_t{});

    // OMNITRACE_PRINT_F("\n");
    // OMNITRACE_PRINT_F("dispatch_id: %lu\n", _dispatch_id);
    for(auto itr : _aggregate)
    {
        if(_agent_counter_storage->at(_agent_id).count(itr.first) == 0)
        {
            static size_t _storage_idx = 0;

            const auto* _agent = tool_data->get_gpu_tool_agent(_agent_id);
            const auto* _info  = tool_data->get_tool_counter_info(_agent_id, itr.first);

            OMNITRACE_CONDITIONAL_ABORT_F(
                !_agent, "unable to find tool agent for agent (id=%zu)\n",
                _agent_id.handle);
            OMNITRACE_CONDITIONAL_ABORT_F(
                !_info,
                "unable to find counter info for counter (id=%zu) on agent (id=%zu)\n",
                itr.first.handle, _agent_id.handle);

            auto _dev_id = static_cast<uint32_t>(_agent->device_id);

            _agent_counter_storage->at(_agent_id).emplace(
                itr.first,
                counter_storage{ tool_data, _dev_id, _storage_idx++, _info->name });
        }

        auto _event = counter_event{ counter_dispatch_record{ &dispatch_data, _dispatch_id,
                                                              itr.first, itr.second } };

        _agent_counter_storage->at(_agent_id).at(itr.first)(_event);

        // get_counter_dispatch_records().emplace_back(_record);
        // OMNITRACE_PRINT_F("- counter_id=%lu :: %12.3f\n", itr.first.handle,
        //                   itr.second.counter_value);
    }
}

void
counter_dispatch_callback(rocprofiler_profile_counting_dispatch_data_t dispatch_data,
                          rocprofiler_profile_config_id_t*             config,
                          rocprofiler_user_data_t* /*user_data*/, void* callback_data_arg)
{
    auto* _data = as_client_data(callback_data_arg);
    if(!_data || !config) return;

    // *config = rocprofiler_profile_config_id_t{ .handle = 0 };
    if(auto itr =
           _data->agent_counter_profiles.find(dispatch_data.dispatch_info.agent_id);
       itr != _data->agent_counter_profiles.end() && itr->second)
    {
        *config = *itr->second;
    }
}

// void
// agent_counter_profile_callback(rocprofiler_context_id_t context_id,
// rocprofiler_agent_id_t agent,
//             rocprofiler_agent_set_profile_callback_t set_config, void*)
// {
//     if(!agent_counter_profiles) return;

//     if(auto itr = agent_counter_profiles->find(agent);
//        itr != agent_counter_profiles->end() && itr->second)
//         set_config(context_id, *itr->second);
// }

bool
is_initialized(rocprofiler_context_id_t ctx)
{
    return (ctx.handle > 0);
}

bool
is_active(rocprofiler_context_id_t ctx)
{
    int  status = 0;
    auto errc   = rocprofiler_context_is_active(ctx, &status);
    return (errc == ROCPROFILER_STATUS_SUCCESS && status > 0);
}

bool
is_valid(rocprofiler_context_id_t ctx)
{
    int  status = 0;
    auto errc   = rocprofiler_context_is_valid(ctx, &status);
    return (errc == ROCPROFILER_STATUS_SUCCESS && status > 0);
}

void
flush()
{
    if(!tool_data) return;

    for(auto itr : tool_data->get_buffers())
    {
        if(itr.handle > 0)
        {
            auto status = rocprofiler_flush_buffer(itr);
            if(status != ROCPROFILER_STATUS_ERROR_BUFFER_BUSY)
            {
                ROCPROFILER_CALL(status);
            }
        }
    }
}

int
tool_init(rocprofiler_client_finalize_t fini_func, void* user_data)
{
    auto domains = settings::instance()->at("OMNITRACE_ROCM_DOMAINS");

    OMNITRACE_VERBOSE_F(1, "Omnitrace ROCm Domains:\n");
    for(const auto& itr : domains->get_choices())
        OMNITRACE_VERBOSE_F(1, "- %s\n", itr.c_str());

    auto _callback_domains = rocprofiler_sdk::get_callback_domains();
    auto _buffered_domain  = rocprofiler_sdk::get_buffered_domains();
    auto _counter_events   = rocprofiler_sdk::get_rocm_events();

    auto* _data        = as_client_data(user_data);
    _data->client_fini = fini_func;

    _data->initialize();
    if(!_counter_events.empty()) _data->initialize_event_info();

    ROCPROFILER_CALL(rocprofiler_create_context(&_data->primary_ctx));

    ROCPROFILER_CALL(rocprofiler_configure_callback_tracing_service(
        _data->primary_ctx, ROCPROFILER_CALLBACK_TRACING_CODE_OBJECT, nullptr, 0,
        tool_code_object_callback, nullptr));

    for(auto itr : { ROCPROFILER_CALLBACK_TRACING_HSA_CORE_API,
                     ROCPROFILER_CALLBACK_TRACING_HSA_AMD_EXT_API,
                     ROCPROFILER_CALLBACK_TRACING_HSA_IMAGE_EXT_API,
                     ROCPROFILER_CALLBACK_TRACING_HSA_FINALIZE_EXT_API,
                     ROCPROFILER_CALLBACK_TRACING_HIP_RUNTIME_API,
                     ROCPROFILER_CALLBACK_TRACING_HIP_COMPILER_API,
                     ROCPROFILER_CALLBACK_TRACING_MARKER_CORE_API })
    {
        if(_callback_domains.count(itr) > 0)
        {
            ROCPROFILER_CALL(rocprofiler_configure_callback_tracing_service(
                _data->primary_ctx, itr, nullptr, 0, tool_tracing_callback, nullptr));
        }
    }

    constexpr auto buffer_size = 8192;
    constexpr auto watermark   = 7936;

    if(_buffered_domain.count(ROCPROFILER_BUFFER_TRACING_KERNEL_DISPATCH) > 0)
    {
        ROCPROFILER_CALL(rocprofiler_create_buffer(
            _data->primary_ctx, buffer_size, watermark,
            ROCPROFILER_BUFFER_POLICY_LOSSLESS, tool_tracing_buffered, tool_data,
            &_data->kernel_dispatch_buffer));

        ROCPROFILER_CALL(rocprofiler_configure_buffer_tracing_service(
            _data->primary_ctx, ROCPROFILER_BUFFER_TRACING_KERNEL_DISPATCH, nullptr, 0,
            _data->kernel_dispatch_buffer));
    }

    if(_buffered_domain.count(ROCPROFILER_BUFFER_TRACING_MEMORY_COPY) > 0)
    {
        ROCPROFILER_CALL(rocprofiler_create_buffer(
            _data->primary_ctx, buffer_size, watermark,
            ROCPROFILER_BUFFER_POLICY_LOSSLESS, tool_tracing_buffered, tool_data,
            &_data->memory_copy_buffer));

        ROCPROFILER_CALL(rocprofiler_configure_buffer_tracing_service(
            _data->primary_ctx, ROCPROFILER_BUFFER_TRACING_MEMORY_COPY, nullptr, 0,
            _data->memory_copy_buffer));
    }

    if(!_counter_events.empty())
    {
        for(const auto& itr : _data->gpu_agents)
        {
            _data->agent_events.emplace(
                itr.agent->id,
                create_agent_profile(itr.agent->id, _counter_events, _data));
        }

        ROCPROFILER_CALL(rocprofiler_create_context(&_data->counter_ctx));

        ROCPROFILER_CALL(rocprofiler_configure_callback_dispatch_profile_counting_service(
            _data->counter_ctx, counter_dispatch_callback, _data, counter_record_callback,
            _data))

        // ROCPROFILER_CALL(rocprofiler_create_buffer(
        //     counter_ctx, buffer_size, watermark,
        //     ROCPROFILER_BUFFER_POLICY_LOSSLESS, tool_tracing_buffered, tool_data,
        //     &counter_collection_buffer));

        // for(const auto& itr : *agent_counter_profiles)
        // {
        //     ROCPROFILER_CALL(rocprofiler_configure_agent_profile_counting_service(
        //         counter_ctx, counter_collection_buffer, itr.first,
        //         agent_counter_profile_callback, nullptr));
        // }
    }

    for(const auto& itr : _data->get_buffers())
    {
        if(itr.handle > 0)
        {
            auto client_thread = rocprofiler_callback_thread_t{};
            ROCPROFILER_CALL(rocprofiler_create_callback_thread(&client_thread));
            ROCPROFILER_CALL(rocprofiler_assign_callback_thread(itr, client_thread));
        }
    }

    // throwaway context for handling the profiler control API. If primary_ctx were used,
    // we would get profiler pause callback but never get profiler resume callback
    {
        auto _local_ctx = rocprofiler_context_id_t{ .handle = 0 };
        ROCPROFILER_CALL(rocprofiler_create_context(&_local_ctx));
        ROCPROFILER_CALL(rocprofiler_configure_callback_tracing_service(
            _local_ctx, ROCPROFILER_CALLBACK_TRACING_MARKER_CONTROL_API, nullptr, 0,
            tool_control_callback, _data));
    }

    if(!is_valid(_data->primary_ctx))
    {
        // notify rocprofiler that initialization failed and all the contexts, buffers,
        // etc. created should be ignored
        return -1;
    }

    gpu::add_device_metadata();

    if(config::get_use_process_sampling() && config::get_use_rocm_smi())
    {
        OMNITRACE_VERBOSE_F(1, "Setting rocm_smi state to active...\n");
        rocm_smi::set_state(State::Active);
    }

    start();

    // no errors
    return 0;
}

void
tool_fini(void* callback_data)
{
    static std::atomic_flag _once = ATOMIC_FLAG_INIT;
    if(_once.test_and_set()) return;

    flush();
    stop();

    if(config::get_use_process_sampling() && config::get_use_rocm_smi())
        rocm_smi::shutdown();

    if(get_counter_storage())
    {
        auto _storages = std::vector<const counter_storage*>{};
        for(const auto& citr : *get_counter_storage())
        {
            for(const auto& itr : citr.second)
                _storages.emplace_back(&itr.second);
        }

        std::sort(_storages.begin(), _storages.end(),
                  [](const counter_storage* lhs, const counter_storage* rhs) {
                      return *lhs < *rhs;
                  });

        for(const auto* itr : _storages)
            itr->write();
        _storages.clear();

        get_counter_storage()->clear();
        delete get_counter_storage();
        get_counter_storage() = nullptr;
    }

    auto* _data        = as_client_data(callback_data);
    _data->client_id   = nullptr;
    _data->client_fini = nullptr;

    delete tool_data;
    tool_data = nullptr;
}
}  // namespace

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
    if(tool_data && tool_data->client_id && tool_data->client_fini)
        tool_data->client_fini(*tool_data->client_id);
}

void
config()
{}

void
post_process()
{}

void
sample()
{}

void
start()
{
    if(!tool_data) return;

    for(auto itr : tool_data->get_contexts())
    {
        if(is_initialized(itr) && !is_active(itr))
        {
            ROCPROFILER_CALL(rocprofiler_start_context(itr));
        }
    }
}

void
stop()
{
    if(!tool_data) return;

    for(auto itr : tool_data->get_contexts())
    {
        if(is_initialized(itr) && is_active(itr))
        {
            ROCPROFILER_CALL(rocprofiler_stop_context(itr));
        }
    }
}

std::vector<hardware_counter_info>
get_rocm_events_info()
{
    if(!tool_data)
    {
        auto _tool_data_v = client_data{};
        _tool_data_v.initialize_event_info();
        return _tool_data_v.events_info;
    }

    if(tool_data->events_info.empty()) tool_data->initialize_event_info();

    return tool_data->events_info;
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

    if(!tim::get_env("OMNITRACE_INIT_TOOLING", true)) return nullptr;
    if(!tim::settings::enabled()) return nullptr;

    if(!omnitrace::config::settings_are_configured() &&
       omnitrace::get_state() < omnitrace::State::Active)
        omnitrace_init_tooling_hidden();

    // set the client name
    id->name = "Omnitrace";

    // ensure tool data exists
    if(!omnitrace::rocprofiler_sdk::tool_data)
        omnitrace::rocprofiler_sdk::tool_data =
            new omnitrace::rocprofiler_sdk::client_data{};

    // store client info
    omnitrace::rocprofiler_sdk::tool_data->client_id = id;

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
                                             omnitrace::rocprofiler_sdk::tool_data };

    // return pointer to configure data
    return &cfg;
}
