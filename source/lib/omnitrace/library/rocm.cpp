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

#include "library/rocm.hpp"
#include "core/config.hpp"
#include "core/debug.hpp"
#include "core/dynamic_library.hpp"
#include "core/gpu.hpp"
#include "library/rocm_smi.hpp"
#include "library/runtime.hpp"
#include "library/thread_data.hpp"
#include "library/tracing.hpp"

#include <timemory/backends/cpu.hpp>
#include <timemory/backends/threading.hpp>
#include <timemory/utility/types.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <tuple>

#if defined(OMNITRACE_USE_ROCM) && OMNITRACE_USE_ROCM > 0
#    include <rocprofiler-sdk/rocprofiler.h>
#endif

namespace omnitrace
{
namespace rocm
{
std::vector<hardware_counter_info>
rocm_events()
{
    auto _events = std::vector<hardware_counter_info>{};
    return _events;
}
}  // namespace rocm
}  // namespace omnitrace

// HSA-runtime tool on-load method
extern "C"
{
    bool OnLoad(HsaApiTable* table, uint64_t runtime_version, uint64_t failed_tool_count,
                const char* const* failed_tool_names)
    {
        tim::consume_parameters(table, runtime_version, failed_tool_count,
                                failed_tool_names);

        static bool _once = false;
        if(_once) return true;
        _once = true;

        OMNITRACE_SCOPED_SAMPLING_ON_CHILD_THREADS(false);

        if(!tim::get_env("OMNITRACE_INIT_TOOLING", true)) return true;
        if(!tim::settings::enabled()) return true;

        OMNITRACE_BASIC_VERBOSE_F(1, "Loading ROCm tooling...\n");

        if(!omnitrace::config::settings_are_configured() &&
           omnitrace::get_state() < omnitrace::State::Active)
            omnitrace_init_tooling_hidden();

        OMNITRACE_SCOPED_THREAD_STATE(ThreadState::Internal);

        if(omnitrace::get_use_process_sampling() && omnitrace::get_use_rocm_smi())
        {
            OMNITRACE_VERBOSE_F(1, "Setting rocm_smi state to active...\n");
            omnitrace::rocm_smi::set_state(omnitrace::State::Active);
        }

        omnitrace::gpu::add_device_metadata();

        return true;
    }
}
