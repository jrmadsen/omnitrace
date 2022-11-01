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

#include "library/debug.hpp"
#include "library/runtime.hpp"
#include "library/state.hpp"
#include "library/thread_info.hpp"

#include <timemory/log/color.hpp>
#include <timemory/utility/filepath.hpp>

namespace omnitrace
{
namespace debug
{
namespace
{
struct source_location_history
{
    std::array<source_location, 10> data = {};
    size_t                          size = 0;
};

auto&
get_source_location_history()
{
    static thread_local auto _v = source_location_history{};
    return _v;
}

const std::string&
get_file_name()
{
    static auto _fname = tim::get_env<std::string>("OMNITRACE_LOG_FILE", "");
    return _fname;
}

std::atomic<FILE*>&
get_file_pointer()
{
    static auto _v = std::atomic<FILE*>{ []() {
        auto&&  _fname= get_file_name();
        if(!_fname.empty()) tim::log::colorized() = false;
        return (_fname.empty())
                   ? stderr
                   : tim::filepath::fopen(
                         settings::compose_output_filename(_fname, ".log"), "w");
    }() };
    return _v;
}
}  // namespace

void
set_source_location(source_location&& _v)
{
    auto& _hist                             = get_source_location_history();
    auto  _idx                              = _hist.size++;
    _hist.data.at(_idx % _hist.data.size()) = _v;
}

lock::lock()
: m_lk{ tim::type_mutex<decltype(std::cerr)>(), std::defer_lock }
{
    if(!m_lk.owns_lock())
    {
        push_thread_state(ThreadState::Internal);
        m_lk.lock();
    }
}

lock::~lock()
{
    if(m_lk.owns_lock())
    {
        m_lk.unlock();
        pop_thread_state();
    }
}

FILE*
get_file()
{
    return get_file_pointer();
}

void
close_file()
{
    if(get_file() != stderr)
    {
        auto* _file = get_file_pointer().load();
        get_file_pointer().store(stderr);
        fclose(_file);
        // Write the trace into a file.
        if(get_verbose() >= 0)
            operation::file_output_message<tim::project::omnitrace>{}(
                get_file_name(), std::string{ "debug" });
    }
}

int64_t
get_tid()
{
    static thread_local const auto& _info = thread_info::get();
    if(_info && _info->index_data) return _info->index_data->sequent_value;
    static auto              _counter = std::atomic<int64_t>{ 0 };
    static thread_local auto _v       = --_counter;
    return _v;
}
}  // namespace debug
}  // namespace omnitrace
