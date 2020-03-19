// Copyright (c) 2017-2019 Intel Corporation
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

#include "mfxdefs.h"
#include "mfx_trace.h"
#if (MFX_VERSION >= 1027)
#include "mfxfeihevc.h"
#endif

#ifdef MFX_TRACE_ENABLE

#ifdef _MSVC_LANG
#pragma warning(disable: 4127) // conditional expression is constant
#endif

extern "C"
{
#include "mfx_trace_utils.h"
#include "mfx_trace_textlog.h"
#include "mfx_trace_stat.h"
#include "mfx_trace_itt.h"
#include "mfx_trace_ftrace.h"
}
#include <stdlib.h>
#include <string.h>
#include "vm_interlocked.h"
#include "mfx_reflect.h"

/*------------------------------------------------------------------------------*/

typedef mfxTraceU32 (*MFXTrace_InitFn)();

typedef mfxTraceU32 (*MFXTrace_SetLevelFn)(mfxTraceChar* category,
                                      mfxTraceLevel level);

typedef mfxTraceU32 (*MFXTrace_DebugMessageFn)(mfxTraceStaticHandle *static_handle,
                                          const char *file_name, mfxTraceU32 line_num,
                                          const char *function_name,
                                          mfxTraceChar* category, mfxTraceLevel level,
                                          const char *message, const char *format, ...);

typedef mfxTraceU32 (*MFXTrace_vDebugMessageFn)(mfxTraceStaticHandle *static_handle,
                                           const char *file_name, mfxTraceU32 line_num,
                                           const char *function_name,
                                           mfxTraceChar* category, mfxTraceLevel level,
                                           const char *message,
                                           const char *format, va_list args);

typedef mfxTraceU32 (*MFXTrace_BeginTaskFn)(mfxTraceStaticHandle *static_handle,
                                       const char *file_name, mfxTraceU32 line_num,
                                       const char *function_name,
                                       mfxTraceChar* category, mfxTraceLevel level,
                                       const char *task_name, mfxTraceTaskHandle *task_handle,
                                       const void *task_params);

typedef mfxTraceU32 (*MFXTrace_EndTaskFn)(mfxTraceStaticHandle *static_handle,
                                     mfxTraceTaskHandle *task_handle);

typedef mfxTraceU32 (*MFXTrace_CloseFn)(void);

struct mfxTraceAlgorithm
{
    mfxTraceU32              m_OutputInitilized;
    mfxTraceU32              m_OutputMask;
    MFXTrace_InitFn          m_InitFn;
    MFXTrace_SetLevelFn      m_SetLevelFn;
    MFXTrace_DebugMessageFn  m_DebugMessageFn;
    MFXTrace_vDebugMessageFn m_vDebugMessageFn;
    MFXTrace_BeginTaskFn     m_BeginTaskFn;
    MFXTrace_EndTaskFn       m_EndTaskFn;
    MFXTrace_CloseFn         m_CloseFn;
};

/*------------------------------------------------------------------------------*/

static mfxTraceU32      g_OutputMode = MFX_TRACE_OUTPUT_TRASH;
static mfxTraceU32      g_Level      = MFX_TRACE_LEVEL_DEFAULT;
static volatile uint32_t  g_refCounter = 0;

static mfxTraceU32           g_mfxTraceCategoriesNum = 0;
static mfxTraceCategoryItem* g_mfxTraceCategoriesTable = NULL;

mfxTraceAlgorithm g_TraceAlgorithms[] =
{
#ifdef MFX_TRACE_ENABLE_TEXTLOG
    {
        0,
        MFX_TRACE_OUTPUT_TEXTLOG,
        MFXTraceTextLog_Init,
        MFXTraceTextLog_SetLevel,
        MFXTraceTextLog_DebugMessage,
        MFXTraceTextLog_vDebugMessage,
        MFXTraceTextLog_BeginTask,
        MFXTraceTextLog_EndTask,
        MFXTraceTextLog_Close
    },
#endif
#ifdef MFX_TRACE_ENABLE_STAT
    {
        0,
        MFX_TRACE_OUTPUT_STAT,
        MFXTraceStat_Init,
        MFXTraceStat_SetLevel,
        MFXTraceStat_DebugMessage,
        MFXTraceStat_vDebugMessage,
        MFXTraceStat_BeginTask,
        MFXTraceStat_EndTask,
        MFXTraceStat_Close
    },
#endif
#ifdef MFX_TRACE_ENABLE_ITT
    {
        0,
        MFX_TRACE_OUTPUT_ITT,
        MFXTraceITT_Init,
        MFXTraceITT_SetLevel,
        MFXTraceITT_DebugMessage,
        MFXTraceITT_vDebugMessage,
        MFXTraceITT_BeginTask,
        MFXTraceITT_EndTask,
        MFXTraceITT_Close
    },
#endif
#ifdef MFX_TRACE_ENABLE_FTRACE
    {
        0,
        MFX_TRACE_OUTPUT_FTRACE,
        MFXTraceFtrace_Init,
        MFXTraceFtrace_SetLevel,
        MFXTraceFtrace_DebugMessage,
        MFXTraceFtrace_vDebugMessage,
        MFXTraceFtrace_BeginTask,
        MFXTraceFtrace_EndTask,
        MFXTraceFtrace_Close
    },
#endif
};

/*------------------------------------------------------------------------------*/


#define CATEGORIES_BUFFER_SIZE 1024

mfxTraceU32 MFXTrace_GetRegistryParams(void)
{
    mfxTraceU32 value = 0;
    FILE* conf_file = mfx_trace_open_conf_file(MFX_TRACE_CONFIG);

    if (!conf_file) return 1;
    if (!mfx_trace_get_conf_dword(conf_file, MFX_TRACE_CONF_OMODE_TYPE, &value))
    {
        g_OutputMode = value;
    }
    if (!mfx_trace_get_conf_dword(conf_file, MFX_TRACE_CONF_LEVEL, &value))
    {
        g_Level = value;
    }
    fclose(conf_file);
    return 0;
}


/*------------------------------------------------------------------------------*/

mfxTraceU32 mfx_trace_get_category_index(mfxTraceChar* category, mfxTraceU32& index)
{
    if (category && g_mfxTraceCategoriesTable)
    {
        mfxTraceU32 i = 0;
        for (i = 0; i < g_mfxTraceCategoriesNum; ++i)
        {
            if (!mfx_trace_tcmp(g_mfxTraceCategoriesTable[i].m_name, category))
            {
                index = i;
                return 0;
            }
        }
    }
    return 1;
}

/*------------------------------------------------------------------------------*/

inline bool MFXTrace_IsPrintableCategoryAndLevel(mfxTraceChar* category, mfxTraceU32 level)
{
    mfxTraceU32 index = 0;

    if (!g_OutputMode)
    {
        return false;
    }
    if (!mfx_trace_get_category_index(category, index))
    {
        if (level <= g_mfxTraceCategoriesTable[index].m_level) return true;
    }
    else if (!g_mfxTraceCategoriesTable)
    {
        if (level <= g_Level) return true;
    }
    return false;
}

/*------------------------------------------------------------------------------*/

mfxTraceU32 MFXTrace_Init()
{
    mfxTraceU32 sts = 0;
    mfxTraceU32 i = 0;
    mfxTraceU32 output_mode = 0;

#if defined(MFX_TRACE_ENABLE_TEXTLOG)
    g_OutputMode |= MFX_TRACE_OUTPUT_TEXTLOG;
#endif
#if defined(MFX_TRACE_ENABLE_STAT)
    g_OutputMode |= MFX_TRACE_OUTPUT_STAT;
#endif
#if defined(MFX_TRACE_ENABLE_ITT)
    g_OutputMode |= MFX_TRACE_OUTPUT_ITT;
#endif
#if defined(MFX_TRACE_ENABLE_FTRACE)
    g_OutputMode |= MFX_TRACE_OUTPUT_FTRACE;
#endif

    if (vm_interlocked_inc32(&g_refCounter) != 1)
    {
        return sts;
    }

#if defined(MFX_TRACE_ENABLE_REFLECT)
    if (g_OutputMode & (MFX_TRACE_OUTPUT_ETW | MFX_TRACE_OUTPUT_TEXTLOG))
    {
        mfx_reflect::AccessibleTypesCollection::Initialize();
    }
#endif

    sts = MFXTrace_GetRegistryParams();
    if (!sts)
    {
        output_mode = g_OutputMode;
        g_OutputMode = 0;
        for (i = 0; i < sizeof(g_TraceAlgorithms)/sizeof(mfxTraceAlgorithm); ++i)
        {
            if (output_mode & g_TraceAlgorithms[i].m_OutputMask)
            {
                sts = g_TraceAlgorithms[i].m_InitFn();
                if (sts == 0)
                {
                    g_OutputMode |= output_mode;
                    g_TraceAlgorithms[i].m_OutputInitilized = g_TraceAlgorithms[i].m_OutputMask;
                }
            }
        }
        // checking search failure
        if (g_OutputMode == MFX_TRACE_OUTPUT_TRASH)
        {
            sts = 1;
        }
    }

    return sts;
}

/*------------------------------------------------------------------------------*/

mfxTraceU32 MFXTrace_Close(void)
{
    mfxTraceU32 sts = 0, res = 0;
    mfxTraceU32 i = 0;

    if (vm_interlocked_dec32(&g_refCounter) != 0)
    {
        return sts;
    }

    for (i = 0; i < sizeof(g_TraceAlgorithms)/sizeof(mfxTraceAlgorithm); ++i)
    {
        if (g_OutputMode & g_TraceAlgorithms[i].m_OutputInitilized)
        {
            res = g_TraceAlgorithms[i].m_CloseFn();
            if (!sts && res) sts = res;
        }
    }
    g_OutputMode = 0;
    g_Level = MFX_TRACE_LEVEL_DEFAULT;
    if (g_mfxTraceCategoriesTable)
    {
        free(g_mfxTraceCategoriesTable);
        g_mfxTraceCategoriesTable = NULL;
    }
    g_mfxTraceCategoriesNum = 0;
    return res;
}

/*------------------------------------------------------------------------------*/

mfxTraceU32 MFXTrace_SetLevel(mfxTraceChar* category, mfxTraceLevel level)
{
    mfxTraceU32 sts = 0, res = 0;
    mfxTraceU32 i = 0;

    for (i = 0; i < sizeof(g_TraceAlgorithms)/sizeof(mfxTraceAlgorithm); ++i)
    {
        if (g_OutputMode & g_TraceAlgorithms[i].m_OutputInitilized)
        {
            res = g_TraceAlgorithms[i].m_SetLevelFn(category, level);
            if (!sts && res) sts = res;
        }
    }
    return sts;
}

/*------------------------------------------------------------------------------*/

mfxTraceU32 MFXTrace_DebugMessage(mfxTraceStaticHandle *static_handle,
                             const char *file_name, mfxTraceU32 line_num,
                             const char *function_name,
                             mfxTraceChar* category, mfxTraceLevel level,
                             const char *message, const char *format, ...)
{
    if (!MFXTrace_IsPrintableCategoryAndLevel(category, level)) return 0;

    mfxTraceU32 sts = 0, res = 0;
    mfxTraceU32 i = 0;
    va_list args;

    va_start(args, format);
    for (i = 0; i < sizeof(g_TraceAlgorithms)/sizeof(mfxTraceAlgorithm); ++i)
    {
        if (g_OutputMode & g_TraceAlgorithms[i].m_OutputInitilized)
        {
            res = g_TraceAlgorithms[i].m_vDebugMessageFn(static_handle,
                                                         file_name, line_num,
                                                         function_name,
                                                         category, level,
                                                         message, format, args);
            if (!sts && res) sts = res;
        }
    }
    va_end(args);
    return sts;
}

/*------------------------------------------------------------------------------*/

mfxTraceU32 MFXTrace_vDebugMessage(mfxTraceStaticHandle *static_handle,
                              const char *file_name, mfxTraceU32 line_num,
                              const char *function_name,
                              mfxTraceChar* category, mfxTraceLevel level,
                              const char *message,
                              const char *format, va_list args)
{
    if (!MFXTrace_IsPrintableCategoryAndLevel(category, level)) return 0;

    mfxTraceU32 sts = 0, res = 0;
    mfxTraceU32 i = 0;

    for (i = 0; i < sizeof(g_TraceAlgorithms)/sizeof(mfxTraceAlgorithm); ++i)
    {
        if (g_OutputMode & g_TraceAlgorithms[i].m_OutputInitilized)
        {
            res = g_TraceAlgorithms[i].m_vDebugMessageFn(static_handle,
                                                         file_name, line_num,
                                                         function_name,
                                                         category, level,
                                                         message, format, args);
            if (!sts && res) sts = res;
        }
    }
    return sts;
}

/*------------------------------------------------------------------------------*/

mfxTraceU32 MFXTrace_BeginTask(mfxTraceStaticHandle *static_handle,
                          const char *file_name, mfxTraceU32 line_num,
                          const char *function_name,
                          mfxTraceChar* category, mfxTraceLevel level,
                          const char *task_name, mfxTraceTaskHandle *task_handle,
                          const void *task_params)
{
    // store category and level to check for MFXTrace_IsPrintableCategoryAndLevel in MFXTrace_EndTask
    if (static_handle)
    {
        static_handle->category = category;
        static_handle->level    = level;
    }

    if (!MFXTrace_IsPrintableCategoryAndLevel(category, level)) return 0;

    mfxTraceU32 sts = 0, res = 0;
    mfxTraceU32 i = 0;

    for (i = 0; i < sizeof(g_TraceAlgorithms)/sizeof(mfxTraceAlgorithm); ++i)
    {
        if (g_OutputMode & g_TraceAlgorithms[i].m_OutputInitilized)
        {
            res = g_TraceAlgorithms[i].m_BeginTaskFn(static_handle,
                                                     file_name, line_num,
                                                     function_name,
                                                     category, level,
                                                     task_name, task_handle, task_params);
            if (!sts && res) sts = res;
        }
    }
    return sts;
}

/*------------------------------------------------------------------------------*/

mfxTraceU32 MFXTrace_EndTask(mfxTraceStaticHandle *static_handle,
                        mfxTraceTaskHandle *task_handle)
{
    mfxTraceChar* category = NULL;
    mfxTraceLevel level = MFX_TRACE_LEVEL_DEFAULT;

    // use category and level stored in MFXTrace_BeginTask values to check for MFXTrace_IsPrintableCategoryAndLevel
    // preserve previous behaviour if static_handle is null that should never happend in normal usage model
    if (static_handle)
    {
        category = static_handle->category;
        level    = static_handle->level;
    }

    if (!MFXTrace_IsPrintableCategoryAndLevel(category, level)) return 0;

    mfxTraceU32 sts = 0, res = 0;
    mfxTraceU32 i = 0;

    for (i = 0; i < sizeof(g_TraceAlgorithms)/sizeof(mfxTraceAlgorithm); ++i)
    {
        if (g_OutputMode & g_TraceAlgorithms[i].m_OutputInitilized)
        {
            res = g_TraceAlgorithms[i].m_EndTaskFn(static_handle, task_handle);
            if (!sts && res) sts = res;
        }
    }
    return sts;
}

/*------------------------------------------------------------------------------*/
// C++ class MFXTraceTask

static unsigned int CreateUniqTaskId()
{
    static unsigned int g_tasksId = 0;
    unsigned int add_val = 1;
    asm volatile ("lock; xaddl %0,%1"
                  : "=r" (add_val), "=m" (g_tasksId)
                  : "0" (add_val), "m" (g_tasksId)
                  : "memory", "cc");
    return add_val; // incremented result will be stored in add_val
}

MFXTraceTask::MFXTraceTask(mfxTraceStaticHandle *static_handle,
                 const char *file_name, mfxTraceU32 line_num,
                 const char *function_name,
                 mfxTraceChar* category, mfxTraceLevel level,
                 const char *task_name,
                 const bool bCreateID)
{
    mfxTraceU32 sts;
    m_pStaticHandle = static_handle;
    memset(&m_TraceTaskHandle, 0, sizeof(m_TraceTaskHandle));
    m_TaskID = (bCreateID) ? CreateUniqTaskId() : 0;
    sts = MFXTrace_BeginTask(static_handle,
                       file_name, line_num,
                       function_name,
                       category, level,
                       task_name,
                       &m_TraceTaskHandle,
                       (bCreateID) ? &m_TaskID : 0);
    m_bStarted = (sts == 0);
}

mfxTraceU32 MFXTraceTask::GetID()
{
    return m_TaskID;
}

void MFXTraceTask::Stop()
{
    if (m_bStarted)
    {
        MFXTrace_EndTask(m_pStaticHandle, &m_TraceTaskHandle);
        m_bStarted = false;
    }
}

MFXTraceTask::~MFXTraceTask()
{
    Stop();
}

#endif //#ifdef MFX_TRACE_ENABLE
