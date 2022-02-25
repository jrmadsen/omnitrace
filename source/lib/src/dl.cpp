// MIT License
//
// Copyright (c) 2020, The Regents of the University of California,
// through Lawrence Berkeley National Laboratory (subject to receipt of any
// required approvals from the U.S. Dept. of Energy).  All rights reserved.
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

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <functional>
#include <gnu/libc-version.h>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#define OMNITRACE_FOLD_EXPRESSION(...) ((__VA_ARGS__), ...)
#define OMNITRACE_VISIBLE              __attribute__((visibility("default")))
#define OMNITRACE_HIDDEN               __attribute__((visibility("internal")))
#define OMNITRACE_INLINE               __attribute__((__always_inline__))
#define OMNITRACE_DLSYM(VARNAME, HANDLE, FUNCNAME)                                       \
    if(HANDLE)                                                                           \
    {                                                                                    \
        *(void**) (&VARNAME) = dlsym(HANDLE, FUNCNAME);                                  \
        if(VARNAME == nullptr && _omnitrace_dl_verbose >= 0)                             \
        {                                                                                \
            fprintf(stderr, "[omnitrace][dl][pid=%i]> %s :: %s\n", getpid(), FUNCNAME,   \
                    dlerror());                                                          \
        }                                                                                \
    }

//--------------------------------------------------------------------------------------//
//
//      omnitrace symbols
//
//--------------------------------------------------------------------------------------//

extern "C"
{
    void omnitrace_init_library(void) OMNITRACE_VISIBLE;
    void omnitrace_init(const char*, bool, const char*) OMNITRACE_VISIBLE;
    void omnitrace_finalize(void) OMNITRACE_VISIBLE;
    void omnitrace_set_env(const char* env_name, const char* env_val) OMNITRACE_VISIBLE;
    void omnitrace_set_mpi(bool use, bool attached) OMNITRACE_VISIBLE;
    void omnitrace_push_trace(const char* name) OMNITRACE_VISIBLE;
    void omnitrace_pop_trace(const char* name) OMNITRACE_VISIBLE;
}

//--------------------------------------------------------------------------------------//

inline namespace omnitrace
{
inline namespace dl
{
namespace
{
inline int
get_omnitrace_env();

bool
get_env(const char* env_id, bool _default);

std::string
get_env(const char* env_id, const char* _default);

int
get_env(const char* env_id, int _default);

template <typename DelimT, typename... Args>
auto
join(DelimT&& _delim, Args&&... _args)
{
    std::stringstream _ss{};
    OMNITRACE_FOLD_EXPRESSION(_ss << _delim << _args);
    if constexpr(std::is_same<DelimT, char>::value)
    {
        return _ss.str().substr(1);
    }
    else
    {
        return _ss.str().substr(std::string{ _delim }.length());
    }
}
// environment priority:
//  - OMNITRACE_DL_DEBUG
//  - OMNITRACE_DL_VERBOSE
//  - OMNITRACE_DEBUG
//  - OMNITRACE_VERBOSE
int _omnitrace_dl_verbose = get_env("OMNITRACE_DL_DEBUG", false)
                                ? 100
                                : get_env("OMNITRACE_DL_VERBOSE", get_omnitrace_env());

template <typename ContainerT = std::vector<std::string>>
inline ContainerT
delimit(const std::string& line, const char* delimiters = "\"',;: ");

// The docs for dlopen suggest that the combination of RTLD_LOCAL + RTLD_DEEPBIND
// (when available) helps ensure that the symbols in the instrumentation library
// libomnitrace.so will use it's own symbols... not symbols that are potentially
// instrumented. However, this only applies to the symbols in libomnitrace.so,
// which is NOT self-contained, i.e. symbols in timemory and the libs it links to
// (such as libpapi.so) are not protected by the deep-bind option. Additionally,
// it should be noted that DynInst does *NOT* add instrumentation by manipulating the
// dynamic linker (otherwise it would only be limited to shared libs) -- it manipulates
// the instructions in the binary so that a call to a function such as "main" actually
// calls "main_dyninst", which executes the instrumentation snippets around the actual
// "main" (this is the reason you need the dyninstAPI_RT library).
//
//  UPDATE:
//      Use of RTLD_DEEPBIND has been removed because it causes the dyninst
//      ProcControlAPI to segfault within pthread_cond_wait on certain executables.
//
// Here are the docs on the dlopen options used:
//
// RTLD_LAZY
//    Perform lazy binding. Only resolve symbols as the code that references them is
//    executed. If the symbol is never referenced, then it is never resolved. (Lazy
//    binding is only performed for function references; references to variables are
//    always immediately bound when the library is loaded.)
//
// RTLD_LOCAL
//    This is the converse of RTLD_GLOBAL, and the default if neither flag is specified.
//    Symbols defined in this library are not made available to resolve references in
//    subsequently loaded libraries.
//
// RTLD_DEEPBIND (since glibc 2.3.4)
//    Place the lookup scope of the symbols in this library ahead of the global scope.
//    This means that a self-contained library will use its own symbols in preference to
//    global symbols with the same name contained in libraries that have already been
//    loaded. This flag is not specified in POSIX.1-2001.
//
#if __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 4
auto        _omnitrace_dl_dlopen_flags = RTLD_LAZY | RTLD_LOCAL;
const char* _omnitrace_dl_dlopen_descr = "RTLD_LAZY | RTLD_LOCAL";
#else
auto        _omnitrace_dl_dlopen_flags = RTLD_LAZY | RTLD_LOCAL;
const char* _omnitrace_dl_dlopen_descr = "RTLD_LAZY | RTLD_LOCAL";
#endif

/// This class contains function pointers for omnitrace's instrumentation functions
struct OMNITRACE_HIDDEN indirect
{
    explicit OMNITRACE_INLINE indirect(std::string libpath)
    : m_libpath{ find_path(std::move(libpath)) }
    {
        if(_omnitrace_dl_verbose > 0)
        {
            fprintf(stderr, "[omnitrace][dl][pid=%i] libomnitrace.so resolved to '%s'\n",
                    getpid(), m_libpath.c_str());
        }
        auto        _omni_hsa_lib = m_libpath;
        const char* _hsa_lib      = getenv("HSA_TOOLS_LIB");
        if(_hsa_lib) _omni_hsa_lib.append(":").append(_hsa_lib);
        setenv("HSA_TOOLS_LIB", _omni_hsa_lib.c_str(), 1);
        open();
        init();
    }

    OMNITRACE_INLINE ~indirect() { dlclose(m_libhandle); }

    OMNITRACE_INLINE void open()
    {
        if(m_libhandle) return;

        auto* libhandle = dlopen(m_libpath.c_str(), _omnitrace_dl_dlopen_flags);

        if(libhandle)
        {
            m_libhandle = libhandle;
            if(_omnitrace_dl_verbose > 0)
            {
                fprintf(stderr, "[omnitrace][dl][pid=%i] dlopen(%s, %s) :: success\n",
                        getpid(), m_libpath.c_str(), _omnitrace_dl_dlopen_descr);
            }
        }
        else
        {
            if(_omnitrace_dl_verbose >= 0)
            {
                perror("dlopen");
                fprintf(stderr, "[omnitrace][dl][pid=%i] dlopen(%s, %s) :: %s\n",
                        getpid(), m_libpath.c_str(), _omnitrace_dl_dlopen_descr,
                        dlerror());
            }
        }

        dlerror();  // Clear any existing error
    }

    OMNITRACE_INLINE void init()
    {
        if(!m_libhandle) open();

        // Initialize all pointers
        OMNITRACE_DLSYM(omnitrace_init_library_f, m_libhandle, "omnitrace_init_library");
        OMNITRACE_DLSYM(omnitrace_init_f, m_libhandle, "omnitrace_init");
        OMNITRACE_DLSYM(omnitrace_finalize_f, m_libhandle, "omnitrace_finalize");
        OMNITRACE_DLSYM(omnitrace_set_env_f, m_libhandle, "omnitrace_set_env");
        OMNITRACE_DLSYM(omnitrace_set_mpi_f, m_libhandle, "omnitrace_set_mpi");
        OMNITRACE_DLSYM(omnitrace_push_trace_f, m_libhandle, "omnitrace_push_trace");
        OMNITRACE_DLSYM(omnitrace_pop_trace_f, m_libhandle, "omnitrace_pop_trace");
    }

    static OMNITRACE_INLINE std::string find_path(std::string&& _path)
    {
        auto _paths =
            delimit(join(":", get_env("OMNITRACE_PATH", ""),
                         get_env("LD_LIBRARY_PATH", ""), get_env("LIBRARY_PATH", "")),
                    ":");

        auto file_exists = [](const std::string& name) {
            struct stat buffer;
            return (stat(name.c_str(), &buffer) == 0);
        };

        for(auto&& itr : _paths)
        {
            auto _f = join('/', itr, _path);
            if(file_exists(_f)) return _f;
        }
        return _path;
    }

public:
    void (*omnitrace_init_library_f)(void)                   = nullptr;
    void (*omnitrace_init_f)(const char*, bool, const char*) = nullptr;
    void (*omnitrace_finalize_f)(void)                       = nullptr;
    void (*omnitrace_set_env_f)(const char*, const char*)    = nullptr;
    void (*omnitrace_set_mpi_f)(bool, bool)                  = nullptr;
    void (*omnitrace_push_trace_f)(const char*)              = nullptr;
    void (*omnitrace_pop_trace_f)(const char*)               = nullptr;

private:
    void*       m_libhandle = nullptr;
    std::string m_libpath   = {};
};

inline indirect&
get_indirect() OMNITRACE_HIDDEN;

template <typename FuncT, typename... Args>
inline void
invoke(const char* _name, FuncT&& _func, Args... _args) OMNITRACE_HIDDEN;
}  // namespace
}  // namespace dl
}  // namespace omnitrace

//--------------------------------------------------------------------------------------//

extern "C"
{
    void omnitrace_init_library(void)
    {
        invoke(__FUNCTION__, get_indirect().omnitrace_init_library_f);
    }

    void omnitrace_init(const char* a, bool b, const char* c)
    {
        invoke(__FUNCTION__, get_indirect().omnitrace_init_f, a, b, c);
    }

    void omnitrace_finalize(void)
    {
        invoke(__FUNCTION__, get_indirect().omnitrace_finalize_f);
    }

    void omnitrace_push_trace(const char* name)
    {
        invoke(__FUNCTION__, get_indirect().omnitrace_push_trace_f, name);
    }

    void omnitrace_pop_trace(const char* name)
    {
        invoke(__FUNCTION__, get_indirect().omnitrace_pop_trace_f, name);
    }

    void omnitrace_set_env(const char* a, const char* b)
    {
        setenv(a, b, 0);
        invoke(__FUNCTION__, get_indirect().omnitrace_set_env_f, a, b);
    }

    void omnitrace_set_mpi(bool a, bool b)
    {
        invoke(__FUNCTION__, get_indirect().omnitrace_set_mpi_f, a, b);
    }
}

//--------------------------------------------------------------------------------------//

inline namespace omnitrace
{
inline namespace dl
{
namespace
{
template <typename ContainerT>
inline ContainerT
delimit(const std::string& line, const char* delimiters)
{
    ContainerT _result{};
    size_t     _beginp = 0;  // position that is the beginning of the new string
    size_t     _delimp = 0;  // position of the delimiter in the string
    while(_beginp < line.length() && _delimp < line.length())
    {
        // find the first character (starting at _delimp) that is not a delimiter
        _beginp = line.find_first_not_of(delimiters, _delimp);
        // if no a character after or at _end that is not a delimiter is not found
        // then we are done
        if(_beginp == std::string::npos) break;
        // starting at the position of the new string, find the next delimiter
        _delimp = line.find_first_of(delimiters, _beginp);
        std::string _tmp{};
        // starting at the position of the new string, get the characters
        // between this position and the next delimiter
        _tmp = line.substr(_beginp, _delimp - _beginp);
        // don't add empty strings
        if(!_tmp.empty()) _result.emplace(_result.end(), _tmp);
    }
    return _result;
}

std::string
get_env(const char* env_id, const char* _default)
{
    if(strlen(env_id) == 0) return _default;
    char* env_var = ::std::getenv(env_id);
    if(env_var) return std::string{ env_var };
    return _default;
}

int
get_env(const char* env_id, int _default)
{
    if(strlen(env_id) == 0) return _default;
    char* env_var = ::std::getenv(env_id);
    if(env_var) return std::stoi(env_var);
    return _default;
}

bool
get_env(const char* env_id, bool _default)
{
    if(strlen(env_id) == 0) return _default;
    char* env_var = ::std::getenv(env_id);
    if(env_var)
    {
        if(std::string{ env_var }.find_first_not_of("0123456789") == std::string::npos)
            return static_cast<bool>(std::stoi(env_var));
        else
        {
            for(size_t i = 0; i < strlen(env_var); ++i)
                env_var[i] = tolower(env_var[i]);
            for(const auto& itr : { "off", "false", "no", "n", "f", "0" })
                if(strcmp(env_var, itr) == 0) return false;
        }
        return true;
    }
    return _default;
}

int
get_omnitrace_env()
{
    auto&& _debug = get_env("OMNITRACE_DEBUG", false);
    return get_env("OMNITRACE_VERBOSE", (_debug) ? 100 : 0);
}

indirect&
get_indirect()
{
    static auto _v = indirect{ get_env("OMNITRACE_LIBRARY", "libomnitrace.so") };
    return _v;
}

int32_t&
get_guard()
{
    static thread_local int32_t _v = 0;
    return _v;
}

template <typename FuncT, typename... Args>
void
invoke(const char* _name, FuncT&& _func, Args... _args)
{
    if(_func)
    {
        // if _lk is ever greater than zero on the same thread, this
        // means a function within the current function is calling
        // our instrumentation so we ignore the call
        int32_t _lk = get_guard()++;
        if(_lk == 0)
        {
            std::invoke(std::forward<FuncT>(_func), _args...);
        }
        else if(_omnitrace_dl_verbose > 2)
        {
            fflush(stderr);
            fprintf(stderr, "[omnitrace][dl] call to %s was guarded :: value = %i\n",
                    _name, _lk);
            fflush(stderr);
        }
        // decrement the guard as it exits the scope
        --get_guard();
    }
    else if(_omnitrace_dl_verbose >= 0)
    {
        fprintf(stderr, "[omnitrace][dl] %s\n",
                join("", "null function pointer to ", _name, ". Ignoring ", _name, "(",
                     _args..., ")")
                    .c_str());
    }
}
}  // namespace
}  // namespace dl
}  // namespace omnitrace