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

#include "core/rocprofiler-sdk.hpp"
#include "core/config.hpp"
#include "core/debug.hpp"
#include "timemory.hpp"

#if defined(OMNITRACE_USE_ROCM) && OMNITRACE_USE_ROCM > 0

#    include <timemory/defines.h>
#    include <timemory/utility/demangle.hpp>

#    include <rocprofiler-sdk/agent.h>
#    include <rocprofiler-sdk/cxx/name_info.hpp>
#    include <rocprofiler-sdk/fwd.h>

#    include <algorithm>
#    include <cstdint>
#    include <set>
#    include <sstream>
#    include <string>
#    include <unordered_set>
#    include <vector>

#    define ROCPROFILER_CALL(result)                                                     \
        {                                                                                \
            rocprofiler_status_t CHECKSTATUS = (result);                                 \
            if(CHECKSTATUS != ROCPROFILER_STATUS_SUCCESS)                                \
            {                                                                            \
                auto        msg        = std::stringstream{};                            \
                std::string status_msg = rocprofiler_get_status_string(CHECKSTATUS);     \
                msg << "[" #result "][" << __FILE__ << ":" << __LINE__ << "] "           \
                    << "rocprofiler-sdk call [" << #result                               \
                    << "] failed with error code " << CHECKSTATUS                        \
                    << " :: " << status_msg;                                             \
                OMNITRACE_WARNING(0, "%s\n", msg.str().c_str());                         \
            }                                                                            \
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

#    define OMNITRACE_CONFIG_SETTING(TYPE, ENV_NAME, DESCRIPTION, INITIAL_VALUE, ...)    \
        [&]() {                                                                          \
            auto _ret = _config->insert<TYPE, TYPE>(                                     \
                ENV_NAME, get_setting_name(ENV_NAME), DESCRIPTION,                       \
                TYPE{ INITIAL_VALUE },                                                   \
                std::set<std::string>{ "custom", "omnitrace", "libomnitrace",            \
                                       __VA_ARGS__ });                                   \
            if(!_ret.second)                                                             \
            {                                                                            \
                OMNITRACE_PRINT("Warning! Duplicate setting: %s / %s\n",                 \
                                get_setting_name(ENV_NAME).c_str(), ENV_NAME);           \
            }                                                                            \
            return _config->find(ENV_NAME)->second;                                      \
        }()

}  // namespace

template <typename Tp>
std::string
to_lower(const Tp& _val)
{
    auto _v = std::string{ _val };
    for(auto& itr : _v)
        itr = ::tolower(itr);
    return _v;
}

void
config_settings(const std::shared_ptr<settings>& _config)
{
    // const auto agents                = std::vector<rocprofiler_agent_t>{};
    const auto buffered_tracing_info = rocprofiler::sdk::get_buffer_tracing_names();
    const auto callback_tracing_info = rocprofiler::sdk::get_callback_tracing_names();

    auto _skip_domains =
        std::unordered_set<std::string_view>{ "marker_core_api", "marker_control_api",
                                              "marker_name_api", "none", "code_object" };
    auto _domain_choices = std::vector<std::string>{};
    auto _add_domain     = [&_domain_choices, &_skip_domains](std::string_view _domain) {
        auto _v = to_lower(_domain);

        if(_skip_domains.count(_v) == 0)
        {
            auto itr = std::find(_domain_choices.begin(), _domain_choices.end(), _v);
            if(itr == _domain_choices.end()) _domain_choices.emplace_back(_v);
        }
    };

    static auto _option_names           = std::unordered_set<std::string>{};
    auto        _add_operation_settings = [&_config,
                                    &_skip_domains](std::string_view _domain_name,
                                                    const auto&      _domain) {
        auto _v = to_lower(_domain_name);

        if(_skip_domains.count(_v) > 0) return;

        auto _op_option_name = JOIN('_', "OMNITRACE_ROCM", _domain_name, "OPERATIONS");
        auto _bt_option_name =
            JOIN('_', "OMNITRACE_ROCM", _domain_name, "OPERATIONS_ANNOTATE_BACKTRACE");

        auto _op_choices = std::vector<std::string>{};
        for(auto itr : _domain.operations)
            _op_choices.emplace_back(std::string{ itr });

        if(_op_choices.empty()) return;

        if(_option_names.emplace(_op_option_name).second)
        {
            OMNITRACE_CONFIG_SETTING(
                std::string, _op_option_name.c_str(),
                "Specification of operations for domain (for API domains, this is a list "
                "of function names) [regex supported]",
                std::string{}, "rocm", "rocprofiler-sdk", "advanced")
                ->set_choices(_op_choices);
        }

        if(_option_names.emplace(_bt_option_name).second)
        {
            OMNITRACE_CONFIG_SETTING(
                std::string, _bt_option_name.c_str(),
                "Specification of domain operations which will record a backtrace (for "
                "API domains, this is a list of function names) [regex supported]",
                std::string{}, "rocm", "rocprofiler-sdk", "advanced")
                ->set_choices(_op_choices);
        }
    };

    _domain_choices.reserve(buffered_tracing_info.size());
    _domain_choices.reserve(callback_tracing_info.size());
    _add_domain("hip_api");
    _add_domain("hsa_api");
    _add_domain("marker_api");

    for(const auto& itr : buffered_tracing_info)
        _add_domain(itr.name);

    for(const auto& itr : callback_tracing_info)
        _add_domain(itr.name);

    std::sort(_domain_choices.begin(), _domain_choices.end());

    OMNITRACE_CONFIG_SETTING(std::string, "OMNITRACE_ROCM_DOMAINS",
                             "Specification of ROCm domains to trace/profile",
                             std::string{ "hip_runtime_api,marker_api,kernel_dispatch,"
                                          "memory_copy,scratch_memory,page_migration" },
                             "rocm", "rocprofiler-sdk")
        ->set_choices(_domain_choices);

    OMNITRACE_CONFIG_SETTING(
        std::string, "OMNITRACE_ROCM_EVENTS",
        "ROCm hardware counters. Use ':device=N' syntax to specify collection on device "
        "number N, e.g. ':device=0'. If no device specification is provided, the event "
        "is collected on every available device",
        "", "rocm", "hardware_counters");

    _skip_domains.emplace("kernel_dispatch");
    _skip_domains.emplace("page_migration");
    _skip_domains.emplace("scratch_memory");

    _add_operation_settings(
        "MARKER_API",
        callback_tracing_info[ROCPROFILER_CALLBACK_TRACING_MARKER_CORE_API]);

    for(const auto& itr : callback_tracing_info)
        _add_operation_settings(itr.name, itr);

    for(const auto& itr : buffered_tracing_info)
        _add_operation_settings(itr.name, itr);
}

std::unordered_set<rocprofiler_callback_tracing_kind_t>
get_callback_domains()
{
    const auto callback_tracing_info = rocprofiler::sdk::get_callback_tracing_names();
    const auto supported = std::unordered_set<rocprofiler_callback_tracing_kind_t>{
        ROCPROFILER_CALLBACK_TRACING_HSA_CORE_API,
        ROCPROFILER_CALLBACK_TRACING_HSA_AMD_EXT_API,
        ROCPROFILER_CALLBACK_TRACING_HSA_IMAGE_EXT_API,
        ROCPROFILER_CALLBACK_TRACING_HSA_FINALIZE_EXT_API,
        ROCPROFILER_CALLBACK_TRACING_HIP_RUNTIME_API,
        ROCPROFILER_CALLBACK_TRACING_HIP_COMPILER_API,
        ROCPROFILER_CALLBACK_TRACING_MARKER_CORE_API,
        ROCPROFILER_CALLBACK_TRACING_CODE_OBJECT,
    };

    auto _data = std::unordered_set<rocprofiler_callback_tracing_kind_t>{};
    auto _domains =
        tim::delimit(config::get_setting_value<std::string>("OMNITRACE_ROCM_DOMAINS")
                         .value_or(std::string{}),
                     " ,;:\t\n");

    const auto valid_choices =
        settings::instance()->at("OMNITRACE_ROCM_DOMAINS")->get_choices();

    auto invalid_domain = [&valid_choices](const auto& domainv) {
        return !std::any_of(valid_choices.begin(), valid_choices.end(),
                            [&domainv](const auto& aitr) { return (aitr == domainv); });
    };

    for(const auto& itr : _domains)
    {
        if(invalid_domain(itr))
        {
            OMNITRACE_THROW("unsupported OMNITRACE_ROCM_DOMAINS value: %s\n",
                            itr.c_str());
        }

        OMNITRACE_PRINT_F("- domain: %s\n", itr.c_str());
        if(itr == "hsa_api")
        {
            for(auto itr : { ROCPROFILER_CALLBACK_TRACING_HSA_CORE_API,
                             ROCPROFILER_CALLBACK_TRACING_HSA_AMD_EXT_API,
                             ROCPROFILER_CALLBACK_TRACING_HSA_IMAGE_EXT_API,
                             ROCPROFILER_CALLBACK_TRACING_HSA_FINALIZE_EXT_API })
                _data.emplace(itr);
        }
        else if(itr == "hip_api")
        {
            for(auto itr : { ROCPROFILER_CALLBACK_TRACING_HIP_COMPILER_API,
                             ROCPROFILER_CALLBACK_TRACING_HIP_COMPILER_API })
                _data.emplace(itr);
        }
        else if(itr == "marker_api" || itr == "roctx")
        {
            _data.emplace(ROCPROFILER_CALLBACK_TRACING_MARKER_CORE_API);
        }
        else
        {
            for(size_t idx = 0; idx < callback_tracing_info.size(); ++idx)
            {
                auto ditr = callback_tracing_info[idx];
                auto dval = static_cast<rocprofiler_callback_tracing_kind_t>(idx);
                if(itr == to_lower(ditr.name) && supported.count(dval) > 0)
                {
                    _data.emplace(dval);
                    OMNITRACE_PRINT_F("- domain: %s (found)\n", itr.c_str());
                    break;
                }
            }
        }
    }

    OMNITRACE_PRINT_F("- domains: %zu\n", _data.size());

    return _data;
}

std::unordered_set<rocprofiler_buffer_tracing_kind_t>
get_buffered_domains()
{
    const auto buffer_tracing_info = rocprofiler::sdk::get_buffer_tracing_names();
    const auto supported = std::unordered_set<rocprofiler_buffer_tracing_kind_t>{
        ROCPROFILER_BUFFER_TRACING_KERNEL_DISPATCH,
        ROCPROFILER_BUFFER_TRACING_MEMORY_COPY,
        ROCPROFILER_BUFFER_TRACING_PAGE_MIGRATION,
        ROCPROFILER_BUFFER_TRACING_SCRATCH_MEMORY,
    };

    auto _data = std::unordered_set<rocprofiler_buffer_tracing_kind_t>{};
    auto _domains =
        tim::delimit(config::get_setting_value<std::string>("OMNITRACE_ROCM_DOMAINS")
                         .value_or(std::string{}),
                     " ,;:\t\n");
    const auto valid_choices =
        settings::instance()->at("OMNITRACE_ROCM_DOMAINS")->get_choices();

    auto invalid_domain = [&valid_choices](const auto& domainv) {
        return !std::any_of(valid_choices.begin(), valid_choices.end(),
                            [&domainv](const auto& aitr) { return (aitr == domainv); });
    };

    for(const auto& itr : _domains)
    {
        if(invalid_domain(itr))
        {
            OMNITRACE_THROW("unsupported OMNITRACE_ROCM_DOMAINS value: %s\n",
                            itr.c_str());
        }

        OMNITRACE_PRINT_F("- domain: %s\n", itr.c_str());
        if(itr == "hsa_api")
        {
            for(auto itr : { ROCPROFILER_BUFFER_TRACING_HSA_CORE_API,
                             ROCPROFILER_BUFFER_TRACING_HSA_AMD_EXT_API,
                             ROCPROFILER_BUFFER_TRACING_HSA_IMAGE_EXT_API,
                             ROCPROFILER_BUFFER_TRACING_HSA_FINALIZE_EXT_API })
                _data.emplace(itr);
        }
        else if(itr == "hip_api")
        {
            for(auto itr : { ROCPROFILER_BUFFER_TRACING_HIP_COMPILER_API,
                             ROCPROFILER_BUFFER_TRACING_HIP_COMPILER_API })
                _data.emplace(itr);
        }
        else if(itr == "marker_api" || itr == "roctx")
        {
            _data.emplace(ROCPROFILER_BUFFER_TRACING_MARKER_CORE_API);
        }
        else
        {
            for(size_t idx = 0; idx < buffer_tracing_info.size(); ++idx)
            {
                auto ditr = buffer_tracing_info[idx];
                auto dval = static_cast<rocprofiler_buffer_tracing_kind_t>(idx);
                if(itr == to_lower(ditr.name) && supported.count(dval) > 0)
                {
                    OMNITRACE_PRINT_F("- domain: %s (found)\n", itr.c_str());
                    _data.emplace(dval);
                    break;
                }
            }
        }
    }

    OMNITRACE_PRINT_F("- domains: %zu\n", _data.size());

    return _data;
}

std::vector<std::string>
get_rocm_events()
{
    return tim::delimit(
        get_setting_value<std::string>("OMNITRACE_ROCM_EVENTS").value_or(std::string{}),
        " ,;\t\n");
}

std::vector<uint32_t>
get_operations(rocprofiler_callback_tracing_kind_t kindv)
{
    (void) kindv;
    return std::vector<uint32_t>{};
}

std::unordered_set<uint32_t>
get_backtrace_operations(rocprofiler_callback_tracing_kind_t kindv)
{
    (void) kindv;
    return std::unordered_set<uint32_t>{};
}

std::unordered_set<uint32_t>
get_backtrace_operations(rocprofiler_buffer_tracing_kind_t kindv)
{
    (void) kindv;
    return std::unordered_set<uint32_t>{};
}

}  // namespace rocprofiler_sdk
}  // namespace omnitrace

#else

namespace omnitrace
{
namespace rocprofiler_sdk
{
void
config_settings(const std::shared_ptr<settings>&)
{}
}  // namespace rocprofiler_sdk
}  // namespace omnitrace

#endif
