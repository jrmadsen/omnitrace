// Copyright (c) 2018 Advanced Micro Devices, Inc. All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// with the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// * Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimers.
//
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimers in the
// documentation and/or other materials provided with the distribution.
//
// * Neither the names of Advanced Micro Devices, Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this Software without specific prior written permission.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH
// THE SOFTWARE.

#if defined(NDEBUG)
#    undef NDEBUG
#endif

#include "library/rocm_smi.hpp"
#include "library/common.hpp"
#include "library/components/fwd.hpp"
#include "library/config.hpp"
#include "library/critical_trace.hpp"
#include "library/debug.hpp"
#include "library/gpu.hpp"
#include "library/perfetto.hpp"
#include "library/runtime.hpp"
#include "library/state.hpp"
#include "library/thread_info.hpp"

#include <timemory/backends/threading.hpp>
#include <timemory/components/timing/backends.hpp>
#include <timemory/units.hpp>
#include <timemory/utility/locking.hpp>

#include <rocm_smi/rocm_smi.h>

#include <cassert>
#include <chrono>
#include <ios>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/resource.h>
#include <thread>

#define OMNITRACE_ROCM_SMI_CALL(ERROR_CODE)                                              \
    ::omnitrace::rocm_smi::check_error(ERROR_CODE, __FILE__, __LINE__)

namespace omnitrace
{
namespace rocm_smi
{
using bundle_t          = std::deque<data>;
using sampler_instances = thread_data<bundle_t, category::rocm_smi>;

namespace
{
bool&
is_initialized()
{
    static bool _v = false;
    return _v;
}

void
check_error(rsmi_status_t _code, const char* _file, int _line)
{
    if(_code == RSMI_STATUS_SUCCESS) return;
    const char* _msg = nullptr;
    auto        _err = rsmi_status_string(_code, &_msg);
    if(_err != RSMI_STATUS_SUCCESS)
        OMNITRACE_THROW("rsmi_status_string failed. No error message available. "
                        "Error code %i originated at %s:%i\n",
                        static_cast<int>(_code), _file, _line);
    OMNITRACE_THROW("[%s:%i] Error code %i :: %s", _file, _line, static_cast<int>(_code),
                    _msg);
}

std::atomic<State>&
get_state()
{
    static std::atomic<State> _v{ State::PreInit };
    return _v;
}
}  // namespace

//--------------------------------------------------------------------------------------//

size_t                           data::device_count     = 0;
std::set<uint32_t>               data::device_list      = {};
std::unique_ptr<data::promise_t> data::polling_finished = {};

data::data(uint32_t _dev_id) { sample(_dev_id); }

void
data::sample(uint32_t _dev_id)
{
    auto _ts = tim::get_clock_real_now<size_t, std::nano>();
    assert(_ts < std::numeric_limits<int64_t>::max());

    auto _state = get_state().load();

    if(_state != State::Active) return;

    m_dev_id = _dev_id;
    m_ts     = _ts;

#define OMNITRACE_RSMI_GET(FUNCTION, ...)                                                \
    try                                                                                  \
    {                                                                                    \
        OMNITRACE_ROCM_SMI_CALL(FUNCTION(__VA_ARGS__));                                  \
    } catch(std::runtime_error & _e)                                                     \
    {                                                                                    \
        OMNITRACE_VERBOSE_F(                                                             \
            0, "[%s] Exception: %s. Disabling future samples from rocm-smi...\n",        \
            #FUNCTION, _e.what());                                                       \
        get_state().store(State::Disabled);                                              \
    }

    OMNITRACE_RSMI_GET(rsmi_dev_busy_percent_get, _dev_id, &m_busy_perc);
    OMNITRACE_RSMI_GET(rsmi_dev_temp_metric_get, _dev_id, RSMI_TEMP_TYPE_EDGE,
                       RSMI_TEMP_CURRENT, &m_temp);
    OMNITRACE_RSMI_GET(rsmi_dev_power_ave_get, _dev_id, 0, &m_power);
    OMNITRACE_RSMI_GET(rsmi_dev_memory_usage_get, _dev_id, RSMI_MEM_TYPE_VRAM,
                       &m_mem_usage);

#undef OMNITRACE_RSMI_GET
}

void
data::print(std::ostream& _os) const
{
    std::stringstream _ss{};
    _ss << "device: " << m_dev_id << ", busy = " << m_busy_perc << "%, temp = " << m_temp
        << ", power = " << m_power << ", memory usage = " << m_mem_usage;
    _os << _ss.str();
}

namespace
{
std::vector<unique_ptr_t<bundle_t>*> _bundle_data{};
}

void
config()
{
    _bundle_data.resize(data::device_count, nullptr);
    for(size_t i = 0; i < data::device_count; ++i)
    {
        if(data::device_list.count(i) > 0)
        {
            _bundle_data.at(i) = &sampler_instances::instances().at(i);
            if(!*_bundle_data.at(i))
                *_bundle_data.at(i) = unique_ptr_t<bundle_t>{ new bundle_t{} };
        }
    }

    data::get_initial().resize(data::device_count);
    for(auto itr : data::device_list)
        data::get_initial().at(itr).sample(itr);
}

void
sample()
{
    for(auto itr : data::device_list)
    {
        if(rocm_smi::get_state() != State::Active) continue;
        OMNITRACE_DEBUG_F("Polling rocm-smi for device %u...\n", itr);
        auto& _data = *_bundle_data.at(itr);
        if(!_data) continue;
        _data->emplace_back(data{ itr });
        OMNITRACE_DEBUG_F("    %s\n", TIMEMORY_JOIN("", _data->back()).c_str());
    }
}

void
set_state(State _v)
{
    rocm_smi::get_state().store(_v);
}

std::vector<data>&
data::get_initial()
{
    static std::vector<data> _v{};
    return _v;
}

bool
data::setup()
{
    perfetto_counter_track<data>::init();
    rocm_smi::set_state(State::PreInit);
    return true;
}

bool
data::shutdown()
{
    OMNITRACE_DEBUG("Shutting down rocm-smi...\n");
    rocm_smi::set_state(State::Finalized);
    return true;
}

#define GPU_METRIC(COMPONENT, ...)                                                       \
    if constexpr(tim::trait::is_available<COMPONENT>::value)                             \
    {                                                                                    \
        auto* _val = _v.get<COMPONENT>();                                                \
        if(_val)                                                                         \
        {                                                                                \
            _val->set_value(itr.__VA_ARGS__);                                            \
            _val->set_accum(itr.__VA_ARGS__);                                            \
        }                                                                                \
    }

void
data::post_process(uint32_t _dev_id)
{
    using component::sampling_gpu_busy;
    using component::sampling_gpu_memory;
    using component::sampling_gpu_power;
    using component::sampling_gpu_temp;

    if(device_count < _dev_id) return;

    auto&       _rocm_smi_v = sampler_instances::instances().at(_dev_id);
    auto        _rocm_smi   = (_rocm_smi_v) ? *_rocm_smi_v : std::deque<rocm_smi::data>{};
    const auto& _thread_info = thread_info::get(0, InternalTID);

    OMNITRACE_VERBOSE(1, "Post-processing %zu rocm-smi samples from device %u\n",
                      _rocm_smi.size(), _dev_id);

    OMNITRACE_CI_THROW(!_thread_info, "Missing thread info for thread 0");
    if(!_thread_info) return;

    auto _process_perfetto = [&]() {
        for(auto& itr : _rocm_smi)
        {
            using counter_track = perfetto_counter_track<data>;
            if(itr.m_dev_id != _dev_id) continue;
            if(!counter_track::exists(_dev_id))
            {
                auto addendum = [&](const char* _v) {
                    return JOIN(" ", "GPU", _v, JOIN("", '[', _dev_id, ']'), "(S)");
                };
                counter_track::emplace(_dev_id, addendum("Busy"), "%");
                counter_track::emplace(_dev_id, addendum("Temperature"), "deg C");
                counter_track::emplace(_dev_id, addendum("Power"), "watts");
                counter_track::emplace(_dev_id, addendum("Memory Usage"), "megabytes");
            }
            uint64_t _ts = itr.m_ts;
            if(!_thread_info->is_valid_time(_ts)) continue;

            double _busy  = itr.m_busy_perc;
            double _temp  = itr.m_temp / 1.0e3;
            double _power = itr.m_power / 1.0e6;
            double _usage = itr.m_mem_usage / static_cast<double>(units::megabyte);
            TRACE_COUNTER("device_busy", counter_track::at(_dev_id, 0), _ts, _busy);
            TRACE_COUNTER("device_temp", counter_track::at(_dev_id, 1), _ts, _temp);
            TRACE_COUNTER("device_power", counter_track::at(_dev_id, 2), _ts, _power);
            TRACE_COUNTER("device_memory_usage", counter_track::at(_dev_id, 3), _ts,
                          _usage);
        }
    };

    if(get_use_perfetto()) _process_perfetto();

    if(!get_use_timemory()) return;

#if !defined(TIMEMORY_USE_MPI)
    // timemory + MPI here causes hangs for some reason. it is unclear why
    using samp_bundle_t = tim::lightweight_tuple<sampling_gpu_busy, sampling_gpu_temp,
                                                 sampling_gpu_power, sampling_gpu_memory>;

    using entry_t     = critical_trace::entry;
    auto _gpu_entries = critical_trace::get_entries(
        [](const entry_t& _e) { return (_e.device == critical_trace::Device::GPU); });

    for(auto& itr : _rocm_smi)
    {
        auto _ts = itr.m_ts;
        if(!_thread_info->is_valid_time(_ts)) continue;

        auto _entries = std::vector<std::pair<std::string_view, const entry_t*>>{};
        for(const auto& eitr : _gpu_entries)
        {
            if(_ts >= eitr.second.begin_ns && _ts <= eitr.second.end_ns)
                _entries.emplace_back(std::string_view{ eitr.first }, &eitr.second);
        }

        std::vector<samp_bundle_t> _tc{};
        _tc.reserve(_entries.size());
        for(auto& eitr : _entries)
        {
            auto& _v = _tc.emplace_back(eitr.first);
            _v.push();
            _v.start();
            _v.stop();

            GPU_METRIC(sampling_gpu_busy, m_busy_perc)
            GPU_METRIC(sampling_gpu_temp, m_temp / 1.0e3)  // provided in milli-degree C
            GPU_METRIC(sampling_gpu_power,
                       m_power * units::microwatt / static_cast<double>(units::watt))
            GPU_METRIC(sampling_gpu_memory,
                       m_mem_usage / static_cast<double>(units::megabyte))

            _v.pop();
        }
    }
#endif
}

//--------------------------------------------------------------------------------------//

void
setup()
{
    auto_lock_t _lk{ type_mutex<category::rocm_smi>() };

    if(is_initialized() || !get_use_rocm_smi()) return;

    OMNITRACE_SCOPED_SAMPLING_ON_CHILD_THREADS(false);

    // assign the data value to determined by rocm-smi
    data::device_count = device_count();

    auto _devices_v = get_sampling_gpus();
    for(auto& itr : _devices_v)
        itr = tolower(itr);
    if(_devices_v == "off")
        _devices_v = "none";
    else if(_devices_v == "on")
        _devices_v = "all";
    bool _all_devices = _devices_v.find("all") != std::string::npos || _devices_v.empty();
    bool _no_devices  = _devices_v.find("none") != std::string::npos;

    std::set<uint32_t> _devices = {};
    auto               _emplace = [&_devices](auto idx) {
        if(idx < data::device_count) _devices.emplace(idx);
    };

    if(_all_devices)
    {
        for(uint32_t i = 0; i < data::device_count; ++i)
            _emplace(i);
    }
    else if(!_no_devices)
    {
        auto _enabled = tim::delimit(_devices_v, ",; \t");
        for(auto&& itr : _enabled)
        {
            if(itr.find_first_not_of("0123456789-") != std::string::npos)
            {
                OMNITRACE_THROW("Invalid GPU specification: '%s'. Only numerical values "
                                "(e.g., 0) or ranges (e.g., 0-7) are permitted.",
                                itr.c_str());
            }

            if(itr.find('-') != std::string::npos)
            {
                auto _v = tim::delimit(itr, "-");
                OMNITRACE_CONDITIONAL_THROW(_v.size() != 2,
                                            "Invalid GPU range specification: '%s'. "
                                            "Required format N-M, e.g. 0-4",
                                            itr.c_str());
                for(auto i = std::stoul(_v.at(0)); i < std::stoul(_v.at(1)); ++i)
                    _emplace(i);
            }
            else
            {
                _emplace(std::stoul(itr));
            }
        }
    }

    data::device_list = _devices;

    try
    {
        for(auto itr : _devices)
        {
            uint16_t dev_id = 0;
            OMNITRACE_ROCM_SMI_CALL(rsmi_dev_id_get(itr, &dev_id));
            // dev_id holds the device ID of device i, upon a successful call
        }

        is_initialized() = true;

        data::setup();
    } catch(std::runtime_error& _e)
    {
        OMNITRACE_VERBOSE(0, "Exception thrown when initializing rocm-smi: %s\n",
                          _e.what());
        data::device_list = {};
    }
}

void
shutdown()
{
    auto_lock_t _lk{ type_mutex<category::rocm_smi>() };

    if(!is_initialized()) return;

    try
    {
        if(data::shutdown())
        {
            OMNITRACE_ROCM_SMI_CALL(rsmi_shut_down());
        }
    } catch(std::runtime_error& _e)
    {
        OMNITRACE_VERBOSE(0, "Exception thrown when shutting down rocm-smi: %s\n",
                          _e.what());
    }

    is_initialized() = false;
}

void
post_process()
{
    for(auto itr : data::device_list)
        data::post_process(itr);
}

uint32_t
device_count()
{
    uint32_t _num_devices = 0;
    try
    {
        static auto _rsmi_init_once = []() { OMNITRACE_ROCM_SMI_CALL(rsmi_init(0)); };
        static std::once_flag _once{};
        std::call_once(_once, _rsmi_init_once);

        OMNITRACE_ROCM_SMI_CALL(rsmi_num_monitor_devices(&_num_devices));
    } catch(const std::exception& _e)
    {
        OMNITRACE_BASIC_PRINT("Exception thrown getting the rocm-smi devices: %s\n",
                              _e.what());
    }
    return _num_devices;
}
}  // namespace rocm_smi
}  // namespace omnitrace

OMNITRACE_INSTANTIATE_EXTERN_COMPONENT(
    TIMEMORY_ESC(data_tracker<double, omnitrace::component::backtrace_gpu_busy>), true,
    double)

OMNITRACE_INSTANTIATE_EXTERN_COMPONENT(
    TIMEMORY_ESC(data_tracker<double, omnitrace::component::backtrace_gpu_temp>), true,
    double)

OMNITRACE_INSTANTIATE_EXTERN_COMPONENT(
    TIMEMORY_ESC(data_tracker<double, omnitrace::component::backtrace_gpu_power>), true,
    double)

OMNITRACE_INSTANTIATE_EXTERN_COMPONENT(
    TIMEMORY_ESC(data_tracker<double, omnitrace::component::backtrace_gpu_memory>), true,
    double)
