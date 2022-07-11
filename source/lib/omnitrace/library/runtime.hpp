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

#include "library/api.hpp"
#include "library/common.hpp"
#include "library/components/fork_gotcha.hpp"
#include "library/components/mpi_gotcha.hpp"
#include "library/components/pthread_gotcha.hpp"
#include "library/components/roctracer.hpp"
#include "library/defines.hpp"
#include "library/state.hpp"
#include "library/thread_data.hpp"
#include "library/timemory.hpp"

#include <memory>
#include <timemory/backends/threading.hpp>
#include <timemory/macros/language.hpp>

#include <string>
#include <string_view>
#include <unordered_set>

namespace omnitrace
{
// bundle of components around omnitrace_init and omnitrace_finalize
using main_bundle_t =
    tim::lightweight_tuple<comp::wall_clock, comp::peak_rss, comp::cpu_clock,
                           comp::cpu_util, pthread_gotcha>;

using gotcha_bundle_t = tim::lightweight_tuple<fork_gotcha_t, mpi_gotcha_t>;

// bundle of components around each thread
#if defined(TIMEMORY_RUSAGE_THREAD) && TIMEMORY_RUSAGE_THREAD > 0
using omnitrace_thread_bundle_t =
    tim::lightweight_tuple<comp::wall_clock, comp::thread_cpu_clock,
                           comp::thread_cpu_util, comp::peak_rss>;
#else
using omnitrace_thread_bundle_t =
    tim::lightweight_tuple<comp::wall_clock, comp::thread_cpu_clock,
                           comp::thread_cpu_util>;
#endif

std::unique_ptr<main_bundle_t>&
get_main_bundle();

std::unique_ptr<gotcha_bundle_t>&
get_gotcha_bundle();

std::atomic<uint64_t>&
get_cpu_cid() TIMEMORY_HOT;

unique_ptr_t<std::vector<uint64_t>>&
get_cpu_cid_stack(int64_t _tid = threading::get_id(), int64_t _parent = 0) TIMEMORY_HOT;

using cpu_cid_data_t       = std::tuple<uint64_t, uint64_t, uint32_t>;
using cpu_cid_pair_t       = std::tuple<uint64_t, uint32_t>;
using cpu_cid_parent_map_t = std::unordered_map<uint64_t, cpu_cid_pair_t>;

unique_ptr_t<cpu_cid_parent_map_t>&
get_cpu_cid_parents(int64_t _tid = threading::get_id()) TIMEMORY_HOT;

cpu_cid_data_t
create_cpu_cid_entry(int64_t _tid = threading::get_id()) TIMEMORY_HOT;

cpu_cid_pair_t
get_cpu_cid_entry(uint64_t _cid, int64_t _tid = threading::get_id()) TIMEMORY_HOT;

tim::mutex_t&
get_cpu_cid_stack_lock(int64_t _tid = threading::get_id()) TIMEMORY_HOT;

ThreadState
get_thread_state() TIMEMORY_HOT;

/// returns old state
ThreadState set_thread_state(ThreadState) TIMEMORY_HOT;

ThreadState push_thread_state(ThreadState) TIMEMORY_HOT;

ThreadState
pop_thread_state() TIMEMORY_HOT;

struct scoped_thread_state
{
    scoped_thread_state(ThreadState _v) { push_thread_state(_v); }
    ~scoped_thread_state() { pop_thread_state(); }
};
}  // namespace omnitrace

#define OMNITRACE_SCOPED_THREAD_STATE(STATE)                                             \
    ::omnitrace::scoped_thread_state OMNITRACE_VARIABLE(                                 \
        OMNITRACE_VAR_NAME_COMBINE(scoped_thread_state_, __LINE__))                      \
    {                                                                                    \
        ::omnitrace::STATE                                                               \
    }
