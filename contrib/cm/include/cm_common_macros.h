// Copyright (c) 2017 Intel Corporation
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

// used to control generation of compile time assertions
#if defined(__CLANG_CM) && _MSC_VER >= 1600
    #define CM_STATIC_ERROR(C,M)   static_assert((C),"CM:e:" M)
    #define CM_STATIC_WARNING(C,M) static_assert((C),"CM:w:" M)
#else
    #define CM_STATIC_ERROR(C,M)
    #define CM_STATIC_WARNING(C,M)
#endif

#ifndef NEW_CM_RT
#define NEW_CM_RT  // Defined for new CM Runtime APIs
#endif

#ifndef AUTO_CM_MODE_SET
#if defined(__CM)
    /// Defined these macros for the Intel CM Compiler.
    #ifdef __GNUC__
    #define _GENX_MAIN_ __attribute__((genx_main))
    #define _GENX_ __attribute__((genx))
    #define _GENX_STACKCALL_ __attribute__((genx_stackcall))
    #define _CM_OUTPUT_ __attribute__((cm_output))
    #else
    #define _GENX_MAIN_ __declspec(genx_main)
    #define _GENX_ __declspec(genx)
    #define _GENX_STACKCALL_ __declspec(genx_stackcall)
    #define _CM_OUTPUT_ __declspec(cm_output)
    #endif
#else
    /// Defined these macros for MSVC and GCC.
    #define CM_GENX
    #define CM_EMU
    #define _GENX_MAIN_
    #define _GENX_
    #define _GENX_STACKCALL_
    #define _CM_OUTPUT_
#endif /* __CM */
#define AUTO_CM_MODE_SET
#endif /* AUTO_CM_MODE_SET */

#ifndef CM_NOINLINE
    #ifndef CM_EMU
    #ifndef __GNUC__
    #define CM_NOINLINE __declspec(noinline)
    #else
    #define CM_NOINLINE __attribute__((noinline))
    #endif
    #else
    #define CM_NOINLINE
    #endif /* CM_EMU */
#endif

#ifndef CM_INLINE
    #ifndef __GNUC__
    #define CM_INLINE __forceinline
    #else
    #define CM_INLINE inline __attribute__((always_inline))
    #endif
#endif

#ifndef CM_API
#ifndef __GNUC__
    #if defined(NEW_CM_RT) && defined(LIBCM_TEST_EXPORTS)
    #define CM_API __declspec(dllexport)
    #elif defined(NEW_CM_RT)
    #define CM_API
    #else
    #define CM_API
    #endif /* CM_EMU */
#else
    #if defined(NEW_CM_RT) && defined(LIBCM_TEST_EXPORTS)
    #define CM_API __attribute__((visibility("default")))
    #elif defined(NEW_CM_RT)
    #define CM_API
    #else
    #define CM_API
    #endif /* CM_EMU */
#endif
#endif /* CM_API */

#ifndef CMRT_LIBCM_API
#ifndef __GNUC__
    #if defined(NEW_CM_RT) && defined(LIBCM_TEST_EXPORTS)
    #define CMRT_LIBCM_API __declspec(dllexport)
    #elif defined(NEW_CM_RT)
    #define CMRT_LIBCM_API __declspec(dllimport)
    #else
    #define CMRT_LIBCM_API
    #endif /* CM_EMU */
#else
    #if defined(NEW_CM_RT) && defined(LIBCM_TEST_EXPORTS)
    #define CMRT_LIBCM_API __attribute__((visibility("default")))
    #elif defined(NEW_CM_RT)
    #define CMRT_LIBCM_API __attribute__((visibility("default")))
    #else
    #define CMRT_LIBCM_API
    #endif /* CM_EMU */
#endif
#endif /* CMRT_LIBCM_API */

#define CM_CHK_RESULT(cm_call)                                  \
do {                                                            \
    int result = cm_call;                                       \
    if (result != CM_SUCCESS) {                                 \
        fprintf(stderr, "Invalid CM call at %s:%d. Error %d: ", \
        __FILE__, __LINE__, result);                            \
        fprintf(stderr, ".\n");                                 \
        exit(EXIT_FAILURE);                                     \
    }                                                           \
} while(false)
