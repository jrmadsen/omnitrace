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

#ifndef OMNITRACE_CATEGORIES_H_
#define OMNITRACE_CATEGORIES_H_

#include <stddef.h>
#include <stdint.h>

#if defined(__cplusplus)
extern "C"
{
#endif

    /// @typedef omnitrace_category_t
    /// @brief Identifier for categories
    ///
    typedef enum OMNITRACE_CATEGORIES
    {
        // Do not use first enum value
        OMNITRACE_CATEGORY_NONE = 0,
        // arrange these in the order most likely to
        // be used since they have to be iterated over
        OMNITRACE_CATEGORY_PYTHON,
        OMNITRACE_CATEGORY_USER,
        OMNITRACE_CATEGORY_HOST,
        OMNITRACE_CATEGORY_ROCM,
        OMNITRACE_CATEGORY_ROCM_HIP_API,
        OMNITRACE_CATEGORY_ROCM_HSA_API,
        OMNITRACE_CATEGORY_ROCM_KERNEL_DISPATCH,
        OMNITRACE_CATEGORY_ROCM_MEMORY_COPY,
        OMNITRACE_CATEGORY_ROCM_SCRATCH_MEMORY,
        OMNITRACE_CATEGORY_ROCM_PAGE_MIGRATION,
        OMNITRACE_CATEGORY_ROCM_COUNTER_COLLECTION,
        OMNITRACE_CATEGORY_ROCM_ROCTX,
        OMNITRACE_CATEGORY_ROCM_SMI,
        OMNITRACE_CATEGORY_ROCM_SMI_BUSY,
        OMNITRACE_CATEGORY_ROCM_SMI_TEMP,
        OMNITRACE_CATEGORY_ROCM_SMI_POWER,
        OMNITRACE_CATEGORY_ROCM_SMI_MEMORY_USAGE,
        OMNITRACE_CATEGORY_ROCM_RCCL,
        OMNITRACE_CATEGORY_SAMPLING,
        OMNITRACE_CATEGORY_PTHREAD,
        OMNITRACE_CATEGORY_KOKKOS,
        OMNITRACE_CATEGORY_MPI,
        OMNITRACE_CATEGORY_OMPT,
        OMNITRACE_CATEGORY_PROCESS_SAMPLING,
        OMNITRACE_CATEGORY_COMM_DATA,
        OMNITRACE_CATEGORY_CAUSAL,
        OMNITRACE_CATEGORY_CPU_FREQ,
        OMNITRACE_CATEGORY_PROCESS_PAGE,
        OMNITRACE_CATEGORY_PROCESS_VIRT,
        OMNITRACE_CATEGORY_PROCESS_PEAK,
        OMNITRACE_CATEGORY_PROCESS_CONTEXT_SWITCH,
        OMNITRACE_CATEGORY_PROCESS_PAGE_FAULT,
        OMNITRACE_CATEGORY_PROCESS_USER_MODE_TIME,
        OMNITRACE_CATEGORY_PROCESS_KERNEL_MODE_TIME,
        OMNITRACE_CATEGORY_THREAD_WALL_TIME,
        OMNITRACE_CATEGORY_THREAD_CPU_TIME,
        OMNITRACE_CATEGORY_THREAD_PAGE_FAULT,
        OMNITRACE_CATEGORY_THREAD_PEAK_MEMORY,
        OMNITRACE_CATEGORY_THREAD_CONTEXT_SWITCH,
        OMNITRACE_CATEGORY_THREAD_HARDWARE_COUNTER,
        OMNITRACE_CATEGORY_KERNEL_HARDWARE_COUNTER,
        OMNITRACE_CATEGORY_NUMA,
        OMNITRACE_CATEGORY_TIMER_SAMPLING,
        OMNITRACE_CATEGORY_OVERFLOW_SAMPLING,
        OMNITRACE_CATEGORY_LAST
        // the value of below enum is used for iterating
        // over the enum in C++ templates. It MUST
        // be the last enumerated id
    } omnitrace_category_t;

    /// @enum OMNITRACE_ANNOTATION_TYPE
    /// @brief Identifier for the data type of the annotation.
    /// if the data type is not a pointer, pass the address of
    /// data.
    /// @typedef OMNITRACE_ANNOTATION_TYPE omnitrace_annotation_type_t
    typedef enum OMNITRACE_ANNOTATION_TYPE
    {
        // Do not use first enum value
        OMNITRACE_VALUE_NONE = 0,
        // arrange these in the order most likely to
        // be used since they have to be iterated over
        OMNITRACE_VALUE_CSTR    = 1,
        OMNITRACE_STRING        = OMNITRACE_VALUE_CSTR,
        OMNITRACE_VALUE_SIZE_T  = 2,
        OMNITRACE_SIZE_T        = OMNITRACE_VALUE_SIZE_T,
        OMNITRACE_VALUE_INT64   = 3,
        OMNITRACE_INT64         = OMNITRACE_VALUE_INT64,
        OMNITRACE_I64           = OMNITRACE_VALUE_INT64,
        OMNITRACE_VALUE_UINT64  = 4,
        OMNITRACE_UINT64        = OMNITRACE_VALUE_UINT64,
        OMNITRACE_U64           = OMNITRACE_VALUE_UINT64,
        OMNITRACE_VALUE_FLOAT64 = 5,
        OMNITRACE_FLOAT64       = OMNITRACE_VALUE_FLOAT64,
        OMNITRACE_FP64          = OMNITRACE_VALUE_FLOAT64,
        OMNITRACE_VALUE_VOID_P  = 6,
        OMNITRACE_VOID_P        = OMNITRACE_VALUE_VOID_P,
        OMNITRACE_PTR           = OMNITRACE_VALUE_VOID_P,
        OMNITRACE_VALUE_INT32   = 7,
        OMNITRACE_INT32         = OMNITRACE_VALUE_INT32,
        OMNITRACE_I32           = OMNITRACE_VALUE_INT32,
        OMNITRACE_VALUE_UINT32  = 8,
        OMNITRACE_UINT32        = OMNITRACE_VALUE_UINT32,
        OMNITRACE_U32           = OMNITRACE_VALUE_UINT32,
        OMNITRACE_VALUE_FLOAT32 = 9,
        OMNITRACE_FLOAT32       = OMNITRACE_VALUE_FLOAT32,
        OMNITRACE_FP32          = OMNITRACE_VALUE_FLOAT32,
        OMNITRACE_VALUE_INT16   = 10,
        OMNITRACE_INT16         = OMNITRACE_VALUE_INT16,
        OMNITRACE_I16           = OMNITRACE_VALUE_INT16,
        OMNITRACE_VALUE_UINT16  = 11,
        OMNITRACE_UINT16        = OMNITRACE_VALUE_UINT16,
        OMNITRACE_U16           = OMNITRACE_VALUE_UINT16,
        // the value of below enum is used for iterating
        // over the enum in C++ templates. It MUST
        // be the last enumerated id
        OMNITRACE_VALUE_LAST
    } omnitrace_annotation_type_t;

    /// @struct omnitrace_annotation
    /// @brief A struct containing annotation data to be included in the perfetto trace.
    ///
    /// @code{.cpp}
    /// #include <cstddef>
    /// #include <cstdint>
    ///
    /// double
    /// compute_residual(size_t n, double* data);
    ///
    /// double
    /// compute(size_t n, double* data, size_t nitr, double tolerance)
    /// {
    ///     omnitrace_annotation_t _annotations[] = {
    ///         { "iteration", OMNITRACE_VALUE_SIZE_T, nullptr },
    ///         { "residual", OMNITRACE_VALUE_FLOAT64, nullptr },
    ///         { "data", OMNITRACE_VALUE_PTR, data },
    ///         { "size", OMNITRACE_VALUE_SIZE_T, &n },
    ///         { "tolerance", OMNITRACE_VALUE_FLOAT64, &tolerance },
    ///         nullptr
    ///     };
    ///
    ///     double residual = tolerance;
    ///     for(size_t i = 0; i < nitr; ++i)
    ///     {
    ///         omnitrace_user_push_annotated_region("compute", &_annotations);
    ///
    ///         residual = compute_residual(n, data);
    ///
    ///         _annotations[0].value = &i;
    ///         _annotations[1].value = &residual;
    ///         omnitrace_user_pop_annotated_region("compute", &_annotations);
    ///     }
    ///
    ///     return residual;
    /// }
    /// @endcode
    /// @typedef omnitrace_annotation omnitrace_annotation_t
    typedef struct omnitrace_annotation
    {
        /// label for annotation
        const char* name;
        /// omnitrace_annotation_type_t
        uintptr_t type;
        /// data to annotate
        void* value;
    } omnitrace_annotation_t;

#if defined(__cplusplus)
}
#endif

#endif  // OMNITRACE_TYPES_H_
