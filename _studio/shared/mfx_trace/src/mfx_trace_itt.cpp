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

#include "mfx_trace.h"

#ifdef MFX_TRACE_ENABLE_ITT
extern "C"
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"

#include <ittnotify.h>

#pragma GCC diagnostic pop

#include <stdio.h>
#include "mfx_trace_utils.h"
#include "mfx_trace_textlog.h"



static __itt_domain* GetDomain(){
    static __itt_domain* pTheDomain  = __itt_domain_create("MediaSDK");
    return pTheDomain;
}

/*------------------------------------------------------------------------------*/

mfxTraceU32 MFXTraceITT_Init()
{
    return 0;
}

/*------------------------------------------------------------------------------*/

mfxTraceU32 MFXTraceITT_Close(void)
{
    return 0;
}

/*------------------------------------------------------------------------------*/

mfxTraceU32 MFXTraceITT_SetLevel(mfxTraceChar* /*category*/, mfxTraceLevel /*level*/)
{
    return 1;
}

/*------------------------------------------------------------------------------*/

mfxTraceU32 MFXTraceITT_DebugMessage(mfxTraceStaticHandle* /*static_handle*/,
                                   const char * /*file_name*/, mfxTraceU32 /*line_num*/,
                                   const char * /*function_name*/,
                                   mfxTraceChar* /*category*/, mfxTraceLevel /*level*/,
                                   const char * /*message*/, const char * /*format*/, ...)
{
    mfxTraceU32 res = 0;
    return res;
}

/*------------------------------------------------------------------------------*/

mfxTraceU32 MFXTraceITT_vDebugMessage(mfxTraceStaticHandle* //static_handle
                                      ,const char * //file_name
                                      ,mfxTraceU32 //line_num
                                      ,const char * //function_name
                                      ,mfxTraceChar* //category
                                      ,mfxTraceLevel //level
                                      ,const char * //message
                                      ,const char * //format
                                      ,va_list //args
                                      )
{
    return 0;
}

/*------------------------------------------------------------------------------*/

mfxTraceU32 MFXTraceITT_BeginTask(mfxTraceStaticHandle *static_handle
                                ,const char * //file_name
                                ,mfxTraceU32 //line_num
                                ,const char * //function_name
                                ,mfxTraceChar* //category
                                ,mfxTraceLevel level
                                ,const char * task_name
                                ,mfxTraceTaskHandle *handle
                                ,const void * /*task_params*/)
{
    if (!static_handle || !handle) return 1;

    if (MFX_TRACE_LEVEL_API == level ||
        MFX_TRACE_LEVEL_INTERNAL_VTUNE == level)
    {
        // cache string handle across task instances
        if (NULL == static_handle->itt1.ptr)
        {
            static_handle->itt1.ptr = __itt_string_handle_create(task_name);
        }

        // task is traced
        handle->itt1.uint32 = 1;

        __itt_task_begin(GetDomain(), __itt_null, __itt_null,
            (__itt_string_handle*)static_handle->itt1.ptr);
    }

    return 0;
}

/*------------------------------------------------------------------------------*/

mfxTraceU32 MFXTraceITT_EndTask(mfxTraceStaticHandle * //static_handle
                                ,mfxTraceTaskHandle *handle
                                )
{
    if (!handle) return 1;

    if (1 == handle->itt1.uint32) __itt_task_end(GetDomain());

    return 0;
}

} // extern "C"
#endif // #ifdef MFX_TRACE_ENABLE_ITT
