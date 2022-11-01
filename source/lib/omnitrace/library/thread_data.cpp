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

#include "library/thread_data.hpp"
#include "library/components/pthread_create_gotcha.hpp"
#include "library/thread_info.hpp"
#include "library/utility.hpp"

#include <timemory/backends/threading.hpp>
#include <timemory/components/timing/backends.hpp>

namespace omnitrace
{
instrumentation_bundles::instance_array_t&
instrumentation_bundles::instances()
{
    static auto _v = instance_array_t{};
    return _v;
}

void
thread_deleter<void>::operator()() const
{
    component::pthread_create_gotcha::shutdown(threading::get_id());
    set_thread_state(ThreadState::Completed);
    if(get_state() < State::Finalized && threading::get_id() == 0)
        omnitrace_finalize_hidden();
}

template struct thread_deleter<void>;
}  // namespace omnitrace
