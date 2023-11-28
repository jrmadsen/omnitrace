# ======================================================================================
# OTF2.cmake
#
# Configure OTF2 for omnitrace
#
# ======================================================================================

include_guard(GLOBAL)

include(FetchContent)
include(ExternalProject)
include(ProcessorCount)

# ---------------------------------------------------------------------------------------#
#
# build
#
# ---------------------------------------------------------------------------------------#

# if(OMNITRACE_BUILD_OTF2 OR TRUE)

fetchcontent_declare(
    otf2-source
    URL https://perftools.pages.jsc.fz-juelich.de/cicd/otf2/tags/otf2-3.0.3/otf2-3.0.3.tar.gz
    URL_HASH SHA256=18a3905f7917340387e3edc8e5766f31ab1af41f4ecc5665da6c769ca21c4ee8)

fetchcontent_getproperties(ot2f-source)

if(NOT ot2f-source_POPULATED)
    message(STATUS "Downloading OTF2...")
    fetchcontent_populate(otf2-source)
endif()

set(_otf2_root ${PROJECT_BINARY_DIR}/external/otf2)
set(_otf2_inc_dirs $<BUILD_INTERFACE:${_otf2_root}/install/include>)
set(_otf2_lib_dirs $<BUILD_INTERFACE:${_otf2_root}/install/lib>)
set(_otf2_libs
    $<BUILD_INTERFACE:${_otf2_root}/install/lib/libotf2${CMAKE_STATIC_LIBRARY_SUFFIX}>)
set(_otf2_build_byproducts
    "${_otf2_root}/install/lib/libotf2${CMAKE_STATIC_LIBRARY_SUFFIX}")

externalproject_add(
    omnitrace-otf2-build
    PREFIX ${_otf2_root}
    SOURCE_DIR ${otf2-source_SOURCE_DIR}
    BUILD_IN_SOURCE 1
    DOWNLOAD_COMMAND ""
    PATCH_COMMAND
        ${CMAKE_COMMAND} -E env CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER}
        <SOURCE_DIR>/configure -q --prefix=${_otf2_root}/install CFLAGS=-fPIC\ -O3
        CXXFLAGS=-fPIC\ -O3 PYTHON=:
    CONFIGURE_COMMAND ${MAKE_COMMAND} install -s
    BUILD_COMMAND ""
    BUILD_BYPRODUCTS "${_otf2_build_byproducts}"
    INSTALL_COMMAND "")

target_include_directories(omnitrace-otf2 SYSTEM INTERFACE ${_otf2_inc_dirs})
target_link_directories(omnitrace-otf2 INTERFACE ${_otf2_lib_dirs})
target_link_libraries(omnitrace-otf2 INTERFACE ${_otf2_libs})
