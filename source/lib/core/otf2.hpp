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

#include "categories.hpp"
#include "common.hpp"

#include "debug.hpp"
#include "library/thread_data.hpp"
#include "library/thread_info.hpp"

#include <otf2/otf2.h>

#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace omnitrace
{
namespace otf2
{
using event_writer_t = OTF2_EvtWriter;
using archive_t      = OTF2_Archive;

event_writer_t*&
init_event_writer(int64_t, event_writer_t*&);

inline event_writer_t*
get_event_writer(int64_t _tid, bool _init = true)
{
    using thread_data_t = thread_data<identity<event_writer_t*>>;
    static auto& _v     = thread_data_t::instance(
        construct_on_init{}, []() -> event_writer_t* { return nullptr; });

    if(OMNITRACE_UNLIKELY(!_v)) return nullptr;

    auto*& _v_tid = _v->at(_tid);
    if(OMNITRACE_UNLIKELY(_v_tid == nullptr && _init))
        _v_tid = init_event_writer(_tid, _v_tid);
    return _v_tid;
}

void
setup();

void
start();

void
stop();

void
post_process();

void
shutdown();
}  // namespace otf2
}  // namespace omnitrace
