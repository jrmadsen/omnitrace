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

#include "core/timemory.hpp"

#if defined(OMNITRACE_USE_ROCM)
#    include <rocprofiler-sdk/fwd.h>
#    include <rocprofiler-sdk/rocprofiler.h>
#endif

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace omnitrace
{
namespace rocprofiler_sdk
{
void
config_settings(const std::shared_ptr<settings>&);

#if defined(OMNITRACE_USE_ROCM)

std::unordered_set<rocprofiler_callback_tracing_kind_t>
get_callback_domains();

std::unordered_set<rocprofiler_buffer_tracing_kind_t>
get_buffered_domains();

std::vector<uint32_t>
get_operations(rocprofiler_callback_tracing_kind_t kindv);

std::vector<std::string>
get_rocm_events();

std::unordered_set<uint32_t>
get_backtrace_operations(rocprofiler_callback_tracing_kind_t kindv);

std::unordered_set<uint32_t>
get_backtrace_operations(rocprofiler_buffer_tracing_kind_t kindv);

#endif
}  // namespace rocprofiler_sdk
}  // namespace omnitrace
