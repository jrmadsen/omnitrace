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

void
config_settings(const std::shared_ptr<settings>& _config)
{
    // const auto agents                = std::vector<rocprofiler_agent_t>{};
    const auto buffered_tracing_info = rocprofiler::sdk::get_buffer_tracing_names();
    const auto callback_tracing_info = rocprofiler::sdk::get_callback_tracing_names();

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
        std::string{
            "hip_runtime,marker_core,kernels,memory_copy,scratch_memory,page_migration" },
        "rocm", "rocprofiler-sdk")
        ->set_choices(_domain_choices);

    OMNITRACE_CONFIG_SETTING(
        std::string, "OMNITRACE_ROCM_EVENTS",
        "ROCm hardware counters. Use ':device=N' syntax to specify collection on device "
        "number N, e.g. ':device=0'. If no device specification is provided, the event "
        "is collected on every available device",
        "", "rocm", "hardware_counters");
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
