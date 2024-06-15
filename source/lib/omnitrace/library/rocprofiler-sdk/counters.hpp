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

#include "common/synchronized.hpp"
#include "core/timemory.hpp"
#include "library/rocprofiler-sdk/fwd.hpp"

#include <rocprofiler-sdk/agent.h>
#include <rocprofiler-sdk/buffer_tracing.h>
#include <rocprofiler-sdk/callback_tracing.h>
#include <rocprofiler-sdk/cxx/hash.hpp>
#include <rocprofiler-sdk/cxx/name_info.hpp>
#include <rocprofiler-sdk/cxx/operators.hpp>
#include <rocprofiler-sdk/dispatch_profile.h>
#include <rocprofiler-sdk/fwd.h>
#include <rocprofiler-sdk/registration.h>

#include <memory>
#include <unordered_map>
#include <vector>

namespace omnitrace
{
namespace rocprofiler_sdk
{
struct counter_dispatch_record
{
    const rocprofiler_profile_counting_dispatch_data_t* dispatch_data  = nullptr;
    rocprofiler_dispatch_id_t                           dispatch_id    = 0;
    rocprofiler_counter_id_t                            counter_id     = {};
    rocprofiler_record_counter_t                        record_counter = {};
};

struct counter_data_tag
{};

using counter_data_tracker = component::data_tracker<double, counter_data_tag>;
using counter_storage_type = typename counter_data_tracker::storage_type;
using counter_bundle_t     = tim::lightweight_tuple<counter_data_tracker>;

struct counter_event
{
    counter_dispatch_record            record   = {};
    mutable std::vector<counter_event> children = {};

    OMNITRACE_DEFAULT_OBJECT(counter_event)

    explicit counter_event(counter_dispatch_record&& _v)
    : record{ _v }
    {}

    void operator()(const client_data* tool_data, int64_t _index,
                    scope::config _scope) const
    {
        if(!record.dispatch_data) return;

        const auto& _dispatch_info = record.dispatch_data->dispatch_info;
        const auto* _kern_sym_data =
            tool_data->get_kernel_symbol_info(_dispatch_info.kernel_id);

        auto _bundle =
            counter_bundle_t{ tim::demangle(_kern_sym_data->kernel_name), _scope };

        _bundle.push(_dispatch_info.queue_id.handle).start().store(record.record_counter);

        // std::sort(children.begin(), children.end());
        for(const auto& itr : children)
            itr(tool_data, _index, _scope);

        _bundle.stop().pop(_dispatch_info.queue_id.handle);
    }
};

inline std::string
get_counter_description(const client_data* tool_data, std::string_view _v)
{
    const auto& _info = tool_data->events_info;
    for(const auto& itr : _info)
    {
        if(itr.symbol().find(_v) == 0 || itr.short_description().find(_v) == 0)
        {
            return itr.long_description();
        }
    }
    return std::string{};
}

struct counter_storage
{
    const client_data*                    tool_data          = nullptr;
    uint64_t                              device_id          = 0;
    int64_t                               index              = 0;
    std::string                           metric_name        = {};
    std::string                           metric_description = {};
    std::string                           storage_name       = {};
    std::unique_ptr<counter_storage_type> storage            = {};

    counter_storage(const client_data* _tool_data, uint64_t _devid, size_t _idx,
                    std::string_view _name)
    : tool_data{ _tool_data }
    , device_id{ _devid }
    , index{ static_cast<int64_t>(_idx) }
    , metric_name{ _name }
    , metric_description{ get_counter_description(_tool_data, metric_name) }
    {
        auto _metric_name = std::string{ _name };
        _metric_name =
            std::regex_replace(_metric_name, std::regex{ "(.*)\\[([0-9]+)\\]" }, "$1_$2");
        storage_name = JOIN('-', "rocprof", "device", device_id, _metric_name);
        storage = std::make_unique<counter_storage_type>(tim::standalone_storage{}, index,
                                                         storage_name);
    }

    ~counter_storage()                      = default;
    counter_storage(const counter_storage&) = delete;
    counter_storage(counter_storage&&)      = default;
    counter_storage& operator=(const counter_storage&) = delete;
    counter_storage& operator=(counter_storage&&) = default;

    friend bool operator<(const counter_storage& lhs, const counter_storage& rhs)
    {
        return std::tie(lhs.storage_name, lhs.device_id, lhs.index) <
               std::tie(rhs.storage_name, rhs.device_id, rhs.index);
    }

    void operator()(const counter_event& _event,
                    scope::config        _scope = scope::flat{}) const
    {
        operation::set_storage<counter_data_tracker>{}(storage.get(), index);
        _event(tool_data, index, _scope);
    }

    void write() const
    {
        operation::set_storage<counter_data_tracker>{}(storage.get(), index);
        counter_data_tracker::label()       = metric_name;
        counter_data_tracker::description() = metric_description;
        storage->write();
    }
};
}  // namespace rocprofiler_sdk
}  // namespace omnitrace
