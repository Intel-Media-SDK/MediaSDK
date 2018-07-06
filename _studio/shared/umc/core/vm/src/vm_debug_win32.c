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


#include <stdio.h>
#include <stdarg.h>
#include <vm_file.h>
#include <vm_debug.h>

#include <umc_structures.h>

#if defined LINUX32
# include <time.h>
# include <sys/types.h>
# include <unistd.h>
# include <syslog.h>
#endif

#ifdef _MSVC_LANG
#pragma warning(disable:4047)
#pragma warning(disable:4024)
#pragma warning(disable:4133)
#endif

#if defined(UMC_VA) && ( defined(LINUX32) || defined(LINUX64) )
#endif

/* here is line for global enable/disable debug trace
// and specify trace info */
#if defined(_DEBUG)
#endif


vm_debug_level vm_debug_setlevel(vm_debug_level level)
{
    return level;
}

vm_debug_output vm_debug_setoutput(vm_debug_output output)
{
    return output;
}

void vm_debug_setfile(vm_char *file, int32_t truncate)
{
    (void)file;
    (void)truncate;
}

void vm_debug_message(const vm_char *format, ...)
{
    (void)format;
}


static struct {
    int code;
    vm_char *string;
}
ErrorStringTable[] =
{
    { 0,                    VM_STRING("No any errors") },
    /* VM */
    { VM_OPERATION_FAILED,  VM_STRING("General operation fault") },
    { VM_NOT_INITIALIZED,   VM_STRING("VM_NOT_INITIALIZED") },
    { VM_TIMEOUT,           VM_STRING("VM_TIMEOUT") },
    { VM_NOT_ENOUGH_DATA,   VM_STRING("VM_NOT_ENOUGH_DATA") },
    { VM_NULL_PTR,          VM_STRING("VM_NULL_PTR") },
    { VM_SO_CANT_LOAD,      VM_STRING("VM_SO_CANT_LOAD") },
    { VM_SO_INVALID_HANDLE, VM_STRING("VM_SO_INVALID_HANDLE") },
    { VM_SO_CANT_GET_ADDR,  VM_STRING("VM_SO_CANT_GET_ADDR") },
    /* UMC */
    { -899,                 VM_STRING("UMC_ERR_INIT: Failed to initialize codec") },
    { -897,                 VM_STRING("UMC_ERR_SYNC: Required suncronization code was not found") },
    { -896,                 VM_STRING("UMC_ERR_NOT_ENOUGH_BUFFER: Buffer size is not enough") },
    { -895,                 VM_STRING("UMC_ERR_END_OF_STREAM: End of stream") },
    { -894,                 VM_STRING("UMC_ERR_OPEN_FAILED: Device/file open error") },
    { -883,                 VM_STRING("UMC_ERR_ALLOC: Failed to allocate memory") },
    { -882,                 VM_STRING("UMC_ERR_INVALID_STREAM: Invalid stream") },
    { -879,                 VM_STRING("UMC_ERR_UNSUPPORTED: Unsupported") },
    { -878,                 VM_STRING("UMC_ERR_NOT_IMPLEMENTED: Not implemented yet") },
    { -876,                 VM_STRING("UMC_ERR_INVALID_PARAMS: Incorrect parameters") },
    { 1,                    VM_STRING("UMC_WRN_INVALID_STREAM") },
    { 2,                    VM_STRING("UMC_WRN_REPOSITION_INPROGRESS") },
    { 3,                    VM_STRING("UMC_WRN_INFO_NOT_READY") },
    /* Win */
    { 0x80004001L,          VM_STRING("E_NOTIMPL: Not implemented") },
    { 0x80004002L,          VM_STRING("E_NOINTERFACE: No such interface supported") },
    { 0x80004003L,          VM_STRING("E_POINTER: Invalid pointer") },
    { 0x80004004L,          VM_STRING("E_ABORT: Operation aborted") },
    { 0x80004005L,          VM_STRING("E_FAIL: Unspecified error") },
    { 0x8000FFFFL,          VM_STRING("E_UNEXPECTED: Catastrophic failure") },
    { 0x80070005L,          VM_STRING("E_ACCESSDENIED: General access denied error") },
    { 0x80070006L,          VM_STRING("E_HANDLE: Invalid handle") },
    { 0x8007000EL,          VM_STRING("E_OUTOFMEMORY: Ran out of memory") },
    { 0x80070057L,          VM_STRING("E_INVALIDARG: One or more arguments are invalid") },
    /* DirectShow */
    { 0x8004020BL,          VM_STRING("VFW_E_RUNTIME_ERROR: A run-time error occurred") },
    { 0x80040210L,          VM_STRING("VFW_E_BUFFERS_OUTSTANDING: One or more buffers are still active") },
    { 0x80040211L,          VM_STRING("VFW_E_NOT_COMMITTED: Cannot allocate a sample when the allocator is not active") },
    { 0x80040227L,          VM_STRING("VFW_E_WRONG_STATE: The operation could not be performed because the filter is in the wrong state") },
};

int32_t vm_trace_hresult_func(int32_t hr, vm_char *mess, void *pthis, vm_char *func, vm_char *file, uint32_t line)
{
    (void)pthis;

    // touch unreferenced parameters
    (void)hr;
    (void)mess;
    (void)func;
    (void)file;
    (void)line;

    return hr;
}

#if defined(LINUX32)
void vm_debug_trace_ex(int32_t level, const vm_char * func_name, const vm_char * file_name,
                       int32_t num_line, const vm_char * format, ...)
{
    (void)level;
    (void)func_name;
    (void)file_name;
    (void)num_line;
    (void)format;
}
#endif

//#endif
/* EOF */
