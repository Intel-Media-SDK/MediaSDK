// Copyright (c) 2017-2018 Intel Corporation
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

#ifndef __VM_DEBUG_H__
#define __VM_DEBUG_H__

#include "vm_types.h"
#include "vm_strings.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
/*
#undef  VM_DEBUG      // uncomment to disable debug trace
#define VM_DEBUG (-1) // uncomment to enable full debug trace
*/
/* ============================================================================
// Define ASSERT and VERIFY for debugging purposes
*/
#define VM_ASSERT(f) ((void) 0)


typedef enum vm_debug_level {  /* debug level */
    VM_DEBUG_NONE           = 0x0000, /* none of the debug levels */
    VM_DEBUG_ERROR          = 0x0001, /* critical errors */
    VM_DEBUG_WARNING        = 0x0002, /* warning/non-critical errors */
    VM_DEBUG_INFO           = 0x0004, /* general attribute information */
    VM_DEBUG_MEMORY         = 0x0008, /* new/delete and alloc/free information */
    VM_DEBUG_PROGRESS       = 0x0010, /* general progressive information */
    VM_DEBUG_CALL           = 0x0020, /* information about calls */
    VM_DEBUG_VERBOSE        = 0x0040, /* other information */
    VM_DEBUG_MACROS         = 0x0080, /* used in macros in umc_structures.h */
    VM_DEBUG_LOG_ALL        = 0x00FF, /* all information */

    VM_DEBUG_SHOW_FILELINE  = 0x0100, /* show filename & line info in the logging message */
    VM_DEBUG_SHOW_DIRECTORY = 0x0200, /* show filename with directory */
    VM_DEBUG_SHOW_TIME      = 0x0400, /* show time info in the logging message */
    VM_DEBUG_SHOW_PID       = 0x0800, /* show thread PID in the logging message */
    VM_DEBUG_SHOW_FUNCTION  = 0x1000, /* show function name */
    VM_DEBUG_SHOW_LEVEL     = 0x2000, /* show level in the logging message */
    VM_DEBUG_SHOW_THIS      = 0x4000, /* show 'this' pointer */
    VM_DEBUG_SHOW_ALL       = 0xFF00, /* show all */

    VM_DEBUG_ALL            = 0xFFFF  /* all above */

} vm_debug_level;

typedef enum vm_debug_output {  /* debug output */
    VM_DEBUG_AUTO       = 0,        /* automatic (stdio for console app, OutputDebugString for win32 app) */
    VM_DEBUG_CONSOLE    = 0x10000,  /* stdio or stderr */
    VM_DEBUG_SYSLOG     = 0x20000,  /* OutputDebugString or syslog */
    VM_DEBUG_FILE       = 0x40000   /* append to a debug file */

} vm_debug_output;

/* ///////////////////// */

#if defined(UNICODE) || defined(_UNICODE)
#define VM_STRING_FORMAT VM_STRING("%S")
#else
#define VM_STRING_FORMAT VM_STRING("%s")
#endif

/* ///////////////////// */


#define vm_debug_trace(level, format)
#define vm_debug_trace1(level, format, a1)
#define vm_debug_trace2(level, format, a1, a2)
#define vm_debug_trace3(level, format, a1, a2, a3)
#define vm_debug_trace4(level, format, a1, a2, a3, a4)
#define vm_debug_trace_withfunc(level, func, format)

#define vm_trace_GUID(guid)


/* ////////////////// */

#define vm_debug_trace_i(level, a1)    \
  vm_debug_trace1(level, VM_STRING(#a1) VM_STRING(" = %d"), a1)

#define vm_debug_trace_x(level, a1)    \
  vm_debug_trace2(level, VM_STRING(#a1) VM_STRING(" = 0x%x = %d"), a1, a1)

#define vm_debug_trace_f(level, a1)    \
  vm_debug_trace1(level, VM_STRING(#a1) VM_STRING(" = %.4f"), a1)

#define vm_debug_trace_s(level, a1)    \
  vm_debug_trace1(level, VM_STRING("#####") VM_STRING(" %s"), VM_STRING(a1))

#define vm_debug_trace_ss(level, a1)    \
  vm_debug_trace1(level, VM_STRING_FORMAT, a1)

/* ///////////////////// */

#define vm_trace_i(a1)    \
  vm_debug_trace_i(VM_DEBUG_VERBOSE, a1)

#define vm_trace_x(a1)    \
  vm_debug_trace_x(VM_DEBUG_VERBOSE, a1)

#define vm_trace_f(a1)    \
  vm_debug_trace_f(VM_DEBUG_VERBOSE, a1)

#define vm_trace_s(a1)    \
  vm_debug_trace_s(VM_DEBUG_VERBOSE, a1)

#define vm_trace_ss(a1)    \
  vm_debug_trace_ss(VM_DEBUG_VERBOSE, a1)

#define TRACE_FUNC  vm_trace_s("")

#undef  SAFE_RELEASE
#define SAFE_RELEASE(PTR) \
if (PTR) \
{ \
    vm_debug_trace1(VM_DEBUG_MEMORY, VM_STRING("[RELEASE] %s"), VM_STRING(#PTR)); \
    PTR->Release(); \
    PTR = NULL; \
}

#define _PRINTABLE(_x)  ((_x < ' ') ? ' ' : _x)

#define vm_trace_fourcc(x) \
    vm_debug_trace4(VM_DEBUG_VERBOSE, \
        VM_STRING(#x) VM_STRING(" = %c%c%c%c"), \
        _PRINTABLE(((char*)&(x))[0]),  \
        _PRINTABLE(((char*)&(x))[1]),  \
        _PRINTABLE(((char*)&(x))[2]),  \
        _PRINTABLE(((char*)&(x))[3]))

/* ///////////////////// */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VM_DEBUG_H__ */
