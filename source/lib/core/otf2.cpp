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

#include "otf2.hpp"
#include "concepts.hpp"
#include "config.hpp"
#include "library/runtime.hpp"
#include "library/thread_info.hpp"
#include "library/tracing.hpp"
#include "timemory/hash/types.hpp"
#include "utility.hpp"

#include <chrono>
#include <otf2/OTF2_Definitions.h>
#include <otf2/OTF2_GeneralDefinitions.h>
#include <otf2/OTF2_Pthread_Locks.h>
#include <otf2/otf2.h>

#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

namespace omnitrace
{
namespace otf2
{
namespace
{
// auto tpd = pthread_key_t{};

OTF2_TimeStamp
get_time()
{
    return tracing::now();
}

OTF2_FlushType
pre_flush(void* userData, OTF2_FileType fileType, OTF2_LocationRef location,
          void* callerData, bool fini)
{
    (void) userData;
    (void) fileType;
    (void) location;
    (void) callerData;
    (void) fini;
    return OTF2_FLUSH;
}

OTF2_TimeStamp
post_flush(void* userData, OTF2_FileType fileType, OTF2_LocationRef location)
{
    (void) userData;
    (void) fileType;
    (void) location;
    return get_time();
}

auto       flush_callbacks = OTF2_FlushCallbacks{ pre_flush, post_flush };
archive_t* archive         = nullptr;
}  // namespace

event_writer_t*&
init_event_writer(int64_t _tid, event_writer_t*& _evt_writer)
{
    return (_evt_writer = OTF2_Archive_GetEvtWriter(archive, _tid));
}

void
setup()
{
    auto _output_info = config::get_otf2_output_filename();

    OMNITRACE_WARNING_F(0, "OTF2 output filename: %s | %s\n", _output_info.first.c_str(),
                        _output_info.second.c_str());
    std::this_thread::sleep_for(std::chrono::seconds{ 5 });

    archive = OTF2_Archive_Open(_output_info.first.c_str(), _output_info.second.c_str(),
                                OTF2_FILEMODE_WRITE, 1024 * 1024,  // event chunk size
                                4 * 1024 * 1024,                   // def chunk size
                                OTF2_SUBSTRATE_POSIX, OTF2_COMPRESSION_NONE);
    OTF2_Archive_SetFlushCallbacks(archive, &flush_callbacks, nullptr);
    OTF2_Archive_SetSerialCollectiveCallbacks(archive);
    OTF2_Pthread_Archive_SetLockingCallbacks(archive, nullptr);
    OTF2_Archive_OpenEvtFiles(archive);
}

void
start()
{}

void
stop()
{}

void
post_process()
{
    auto max_trace_length = uint64_t{ 0 };
    OTF2_Archive_CloseEvtFiles(archive);
    OTF2_Archive_OpenDefFiles(archive);
    for(size_t i = 0; i < max_supported_threads; ++i)
    {
        const auto& _info = thread_info::get(i, SequentTID);
        if(_info && !_info->is_offset)
            max_trace_length =
                std::max<uint64_t>(max_trace_length, _info->get_duration());

        if(get_event_writer(i, false))
        {
            OTF2_DefWriter* def_writer = OTF2_Archive_GetDefWriter(archive, i);
            OTF2_Archive_CloseDefWriter(archive, def_writer);
        }
    }
    OTF2_Archive_CloseDefFiles(archive);
    OTF2_GlobalDefWriter* global_def_writer = OTF2_Archive_GetGlobalDefWriter(archive);
    OTF2_GlobalDefWriter_WriteClockProperties(
        global_def_writer, std::chrono::steady_clock::period::den,
        std::chrono::steady_clock::now().time_since_epoch().count(), max_trace_length,
        std::chrono::system_clock::now().time_since_epoch().count());

    auto _hash_data          = tim::hash_map_t{};
    auto _condense_hash_data = [&_hash_data](const auto& _v) {
        _hash_data.reserve(_hash_data.size() + _v.size());
        for(const auto& itr : _v)
            _hash_data.emplace(itr.first, itr.second);
    };

    _condense_hash_data(*tim::hash::get_main_hash_ids());
    for(size_t i = 0; i < max_supported_threads; ++i)
    {
        auto& hitr = tracing::get_timemory_hash_ids(i);
        if(hitr) _condense_hash_data(*hitr);
    }

    OTF2_GlobalDefWriter_WriteString(global_def_writer, 0, "");
    for(const auto& itr : _hash_data)
    {
        if(itr.first != 0)
            OTF2_GlobalDefWriter_WriteString(global_def_writer, itr.first,
                                             itr.second.c_str());
    }

    for(const auto& itr : _hash_data)
    {
        if(itr.first != 0)
            OTF2_GlobalDefWriter_WriteRegion(
                global_def_writer, itr.first, itr.first, 0, 0, OTF2_REGION_ROLE_FUNCTION,
                OTF2_PARADIGM_USER, OTF2_REGION_FLAG_NONE, 0, 0, 0);
    }

    auto _exe_name = config::get_exe_name();
    auto _exe_hash = tim::get_hash_id(_exe_name);
    OTF2_GlobalDefWriter_WriteString(global_def_writer, _exe_hash, _exe_name.c_str());

    auto _node_name = std::string{ "node" };
    {
        char _hostname_c[PATH_MAX];
        if(gethostname(_hostname_c, PATH_MAX) == 0 &&
           strnlen(_hostname_c, PATH_MAX) < PATH_MAX)
            _node_name = std::string{ _hostname_c };
    }
    auto _node_hash = tim::get_hash_id(_node_name);
    OTF2_GlobalDefWriter_WriteString(global_def_writer, _node_hash, _node_name.c_str());

    OTF2_GlobalDefWriter_WriteSystemTreeNode(global_def_writer, 0, _exe_hash, _node_hash,
                                             OTF2_UNDEFINED_SYSTEM_TREE_NODE);
    OTF2_GlobalDefWriter_WriteLocationGroup(global_def_writer, 0, _exe_hash,
                                            OTF2_LOCATION_GROUP_TYPE_PROCESS, 0,
                                            OTF2_UNDEFINED_LOCATION_GROUP);

    for(size_t i = 0; i < max_supported_threads; ++i)
    {
        auto _name = JOIN(" ", "Thread", i);
        auto _hash = tim::get_hash_id(_name);
        if(i > 0)
        {
            OTF2_GlobalDefWriter_WriteString(global_def_writer, _hash, _name.c_str());
        }
        OTF2_GlobalDefWriter_WriteLocation(global_def_writer, i,  // id
                                           _hash, OTF2_LOCATION_TYPE_CPU_THREAD,
                                           2,  // # events
                                           0   // location group
        );
    }
}

void
shutdown()
{
    OTF2_Archive_Close(archive);
}
}  // namespace otf2
}  // namespace omnitrace
