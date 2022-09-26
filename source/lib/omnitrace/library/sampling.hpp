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

#pragma once

#include "library/common.hpp"
#include "library/components/backtrace.hpp"
#include "library/components/backtrace_metrics.hpp"
#include "library/components/backtrace_timestamp.hpp"
#include "library/components/fwd.hpp"
#include "library/defines.hpp"
#include "library/thread_data.hpp"
#include "library/timemory.hpp"

#include <timemory/macros/language.hpp>
#include <timemory/variadic/types.hpp>

#include <cstdint>
#include <memory>
#include <set>
#include <type_traits>

namespace omnitrace
{
namespace sampling
{
using component::backtrace;             // NOLINT
using component::backtrace_cpu_clock;   // NOLINT
using component::backtrace_fraction;    // NOLINT
using component::backtrace_metrics;     // NOLINT
using component::backtrace_timestamp;   // NOLINT
using component::backtrace_wall_clock;  // NOLINT
using component::sampling_cpu_clock;
using component::sampling_gpu_busy;
using component::sampling_gpu_memory;
using component::sampling_gpu_power;
using component::sampling_gpu_temp;
using component::sampling_percent;
using component::sampling_wall_clock;

unique_ptr_t<std::set<int>>&
get_signal_types(int64_t _tid);

std::set<int>
setup();

std::set<int>
shutdown();

void
block_samples();

void
unblock_samples();

void block_signals(std::set<int> = {});

void unblock_signals(std::set<int> = {});

void
post_process();
}  // namespace sampling
}  // namespace omnitrace
