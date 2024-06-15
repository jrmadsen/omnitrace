#
set(rocprofiler_sdk_sources ${CMAKE_CURRENT_LIST_DIR}/fwd.cpp)

set(rocprofiler_sdk_headers ${CMAKE_CURRENT_LIST_DIR}/fwd.hpp)

target_sources(omnitrace-object-library PRIVATE ${rocprofiler_sdk_sources}
                                                ${rocprofiler_sdk_headers})
