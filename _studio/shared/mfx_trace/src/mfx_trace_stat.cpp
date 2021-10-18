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

#ifdef MFX_TRACE_ENABLE_STAT
extern "C"
{
#include "mfx_trace_utils.h"
#include "mfx_trace_stat.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/*------------------------------------------------------------------------------*/

mfxTraceU32 MFXTraceStat_PrintHeader(void);
mfxTraceU32 MFXTraceStat_PrintInfo(mfxTraceStaticHandle* static_handle);

#define FORMAT_FN_NAME    "%-40s: "
#define FORMAT_TASK_NAME  "%-40s: "
#define FORMAT_FULL_STAT  "%10.6f, %6d, %10.6f, %10.6f"
#define FORMAT_SHORT_STAT "%10.6f, %6d, %10s, %10s"
#define FORMAT_CAT        ": %s"
#define FORMAT_LEV        ": LEV_%-02d"
#define FORMAT_FILE_NAME  ": %s"
#define FORMAT_LINE_NUM   ": %d"

#define FORMAT_HDR_FN_NAME   "%-40s: "
#define FORMAT_HDR_TASK_NAME "%-40s: "
#define FORMAT_HDR_STAT      "%10s, %6s, %10s, %10s"
#define FORMAT_HDR_CAT       FORMAT_CAT
#define FORMAT_HDR_LEV       ": %-6s"
#define FORMAT_HDR_FILE_NAME FORMAT_FILE_NAME
#define FORMAT_HDR_LINE_NUM  ": %s"

/*------------------------------------------------------------------------------*/

typedef mfxTraceU64 mfxTraceTick;

#if defined(LINUX32)
#include <sys/time.h>

#define MFX_TRACE_TIME_MHZ 1000000
#endif

/*------------------------------------------------------------------------------*/

static mfxTraceTick mfx_trace_get_frequency(void)
{
    return (mfxTraceTick)MFX_TRACE_TIME_MHZ;
}

static mfxTraceTick mfx_trace_get_tick(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (mfxTraceTick)tv.tv_sec * (mfxTraceTick)MFX_TRACE_TIME_MHZ + (mfxTraceTick)tv.tv_usec;
}

#define mfx_trace_get_time(T,S,F) ((double)(__INT64)((T)-(S))/(double)(__INT64)(F))

/*------------------------------------------------------------------------------*/

#define MFX_TRACE_STAT_NUM_OF_STAT_ITEMS 100

class mfxStatGlobalHandle
{
public:
    mfxStatGlobalHandle(void) :
        m_StatTableIndex(0),
        m_StatTableSize(0),
        m_pStatTable(NULL)
    {
        m_pStatTable = (mfxTraceStaticHandle**)malloc(MFX_TRACE_STAT_NUM_OF_STAT_ITEMS * sizeof(mfxTraceStaticHandle*));
        if (m_pStatTable) m_StatTableSize = MFX_TRACE_STAT_NUM_OF_STAT_ITEMS;
    };

    ~mfxStatGlobalHandle(void)
    {
        Close();
        if (m_pStatTable)
        {
            free(m_pStatTable);
            m_pStatTable = NULL;
        }
    };

    mfxTraceU32 AddItem(mfxTraceStaticHandle* pStatHandle)
    {
        if (!pStatHandle) return 1;
        if (m_StatTableIndex >= m_StatTableSize)
        {
            mfxTraceStaticHandle** pStatTable = NULL;

            pStatTable = (mfxTraceStaticHandle**)realloc(m_pStatTable, (m_StatTableSize + MFX_TRACE_STAT_NUM_OF_STAT_ITEMS) * sizeof(mfxTraceStaticHandle*));
            if (pStatTable)
            {
                m_pStatTable = pStatTable;
                m_StatTableSize += MFX_TRACE_STAT_NUM_OF_STAT_ITEMS;
            }
            else return 1;
        }
        if (!m_pStatTable || (m_StatTableIndex >= m_StatTableSize)) return 1;
        if (m_StatTableIndex < m_StatTableSize)
        {
            m_pStatTable[m_StatTableIndex] = pStatHandle;
            ++m_StatTableIndex;
        }
        return 0;
    };

    void Close(void)
    {
        mfxTraceU32 i = 0;

        if (m_StatTableIndex) MFXTraceStat_PrintHeader();
        for (i = 0; i < m_StatTableIndex; ++i)
        {
            MFXTraceStat_PrintInfo(m_pStatTable[i]);
            if (m_pStatTable[i]->sd4.str)
            {
                free(m_pStatTable[i]->sd4.str);
                m_pStatTable[i]->sd4.str = NULL;
            }
        }
        m_StatTableIndex = 0;
    };

protected:
    mfxTraceU32 m_StatTableIndex;
    mfxTraceU32 m_StatTableSize;
    mfxTraceStaticHandle** m_pStatTable;
};

/*------------------------------------------------------------------------------*/

static mfxStatGlobalHandle g_StatGlobalHandle;
static mfxTraceChar g_mfxTraceStatFileName[MAX_PATH] = {0};
static FILE* g_mfxTraceStatFile = NULL;
static mfxTraceU32 g_StatSuppress =
    MFX_TRACE_STAT_SUPPRESS_FILE_NAME |
    MFX_TRACE_STAT_SUPPRESS_LINE_NUM |
    MFX_TRACE_STAT_SUPPRESS_LEVEL;

/*------------------------------------------------------------------------------*/


mfxTraceU32 MFXTraceStat_GetRegistryParams(void)
{
    FILE* conf_file = mfx_trace_open_conf_file(MFX_TRACE_CONFIG);
    mfxTraceU32 value = 0;

    if (!conf_file) return 1;
    if (mfx_trace_get_conf_string(conf_file,
                                  MFX_TRACE_STAT_REG_FILE_NAME,
                                  g_mfxTraceStatFileName,
                                  sizeof(g_mfxTraceStatFileName)))
    {
        g_mfxTraceStatFileName[0] = '\0';
    }

    if (!mfx_trace_get_conf_dword(conf_file,
                                  MFX_TRACE_STAT_REG_SUPPRESS,
                                  &value))
    {
        g_StatSuppress = value;
    }
    if (!mfx_trace_get_conf_dword(conf_file,
                                  MFX_TRACE_STAT_REG_PERMIT,
                                  &value))
    {
        g_StatSuppress &= ~value;
    }
    fclose(conf_file);
    return 0;
}


/*------------------------------------------------------------------------------*/

mfxTraceU32 MFXTraceStat_Init()
{
    mfxTraceU32 sts = 0;

    sts = MFXTraceStat_Close();
    if (!sts) sts = MFXTraceStat_GetRegistryParams();
    if (!sts)
    {
        if (!mfx_trace_tcmp(g_mfxTraceStatFileName, MFX_TRACE_STRING("stdout"))) g_mfxTraceStatFile = stdout;
        else g_mfxTraceStatFile = mfx_trace_tfopen(g_mfxTraceStatFileName, MFX_TRACE_STRING("a"));
        if (!g_mfxTraceStatFile) return 1;
    }
    return sts;
}

/*------------------------------------------------------------------------------*/

mfxTraceU32 MFXTraceStat_Close(void)
{
    g_StatGlobalHandle.Close();
    if (g_mfxTraceStatFile)
    {
        fclose(g_mfxTraceStatFile);
        g_mfxTraceStatFile = NULL;
    }
    g_mfxTraceStatFileName[0] = '\0';
    g_StatSuppress = MFX_TRACE_STAT_SUPPRESS_FILE_NAME |
                     MFX_TRACE_STAT_SUPPRESS_LINE_NUM |
                     MFX_TRACE_STAT_SUPPRESS_LEVEL;
    return 0;
}

/*------------------------------------------------------------------------------*/

mfxTraceU32 MFXTraceStat_SetLevel(mfxTraceChar* /*category*/, mfxTraceLevel /*level*/)
{
    return 1;
}

/*------------------------------------------------------------------------------*/

mfxTraceU32 MFXTraceStat_DebugMessage(mfxTraceStaticHandle* static_handle,
                                 const char *file_name, mfxTraceU32 line_num,
                                 const char *function_name,
                                 mfxTraceChar* category, mfxTraceLevel level,
                                 const char *message, const char *format, ...)
{
    mfxTraceU32 res = 0;
    va_list args;

    va_start(args, format);
    res = MFXTraceStat_vDebugMessage(static_handle,
                                     file_name , line_num,
                                     function_name,
                                     category, level,
                                     message, format, args);
    va_end(args);
    return res;
}

/*------------------------------------------------------------------------------*/

mfxTraceU32 MFXTraceStat_vDebugMessage(mfxTraceStaticHandle* /*static_handle*/,
                                  const char* /*file_name*/, mfxTraceU32 /*line_num*/,
                                  const char* /*function_name*/,
                                  mfxTraceChar* /*category*/, mfxTraceLevel /*level*/,
                                  const char* /*message*/,
                                  const char* /*format*/, va_list /*args*/)
{
    if (!g_mfxTraceStatFile) return 1;
    return 0;
}

/*------------------------------------------------------------------------------*/

mfxTraceU32 MFXTraceStat_PrintHeader(void)
{
    if (!g_mfxTraceStatFile) return 1;

    char str[MFX_TRACE_MAX_LINE_LENGTH] = {0}, *p_str = str;
    size_t len = MFX_TRACE_MAX_LINE_LENGTH;

    {
        p_str = mfx_trace_sprintf(p_str, len, FORMAT_HDR_FN_NAME, "Function name");
        p_str = mfx_trace_sprintf(p_str, len, FORMAT_HDR_TASK_NAME, "Task name");
        p_str = mfx_trace_sprintf(p_str, len, FORMAT_HDR_STAT, "Total time", "Number", "Avg. time", "Std. dev.");
    }
    if (!(g_StatSuppress & MFX_TRACE_STAT_SUPPRESS_CATEGORY))
    {
        p_str = mfx_trace_sprintf(p_str, len, FORMAT_HDR_CAT, MFX_TRACE_STRING("Category"));
    }
    if (!(g_StatSuppress & MFX_TRACE_STAT_SUPPRESS_LEVEL))
    {
        p_str = mfx_trace_sprintf(p_str, len, FORMAT_HDR_LEV, "Level");
    }
    if (!(g_StatSuppress & MFX_TRACE_STAT_SUPPRESS_FILE_NAME))
    {
        p_str = mfx_trace_sprintf(p_str, len, FORMAT_HDR_FILE_NAME, "File name");
    }
    if (!(g_StatSuppress & MFX_TRACE_STAT_SUPPRESS_LINE_NUM))
    {
        p_str = mfx_trace_sprintf(p_str, len, FORMAT_HDR_LINE_NUM, "Line number");
    }
    {
        p_str = mfx_trace_sprintf(p_str, len, "\n");
    }
    fprintf(g_mfxTraceStatFile, "%s", str);
    fflush(g_mfxTraceStatFile);

    return 0;
}

/*------------------------------------------------------------------------------*/

mfxTraceU32 MFXTraceStat_PrintInfo(mfxTraceStaticHandle* static_handle)
{
    if (!g_mfxTraceStatFile) return 1;
    if (!static_handle) return 1;

    char str[MFX_TRACE_MAX_LINE_LENGTH] = {0}, *p_str = str;
    size_t len = MFX_TRACE_MAX_LINE_LENGTH;

    char *file_name = NULL, *function_name = NULL, *task_name = NULL;
    mfxTraceU32 line_num = 0;
    mfxTraceChar* category = NULL;
    mfxTraceLevel level = MFX_TRACE_LEVEL_DEFAULT;
    bool b_avg_not_applicable = (1 == static_handle->sd5.uint32)? true: false;
    double time = 0, avg_time = 0, std_div = 0;

    // restoring info
    category      = static_handle->category;
    level         = static_handle->level;

    file_name     = static_handle->sd1.str;
    line_num      = static_handle->sd2.uint32;
    function_name = static_handle->sd3.str;
    task_name     = static_handle->sd4.str;

    mfxTraceTick trace_frequency = mfx_trace_get_frequency();

    // calculating statistics
    time = mfx_trace_get_time(static_handle->sd6.tick, 0, trace_frequency);
    if (!b_avg_not_applicable)
    {
        avg_time = time/(double)static_handle->sd5.uint32;
        // that's approximation of standard deviation:
        // should be: n/(n-1) * [ sum (xk - x_avg)^2 ]
        // approximation is: n/(n-1) * [ 1/n * sum (xk*xk) - x_avg * x_avg]
        std_div = mfx_trace_get_time(static_handle->sd7.tick, 0, trace_frequency)/(double)static_handle->sd5.uint32;
        std_div /= trace_frequency;
        std_div -= avg_time*avg_time;
        std_div *= (double)static_handle->sd5.uint32/((double)(static_handle->sd5.uint32 - 1));
        std_div = sqrt(std_div);
    }
    {
        p_str = mfx_trace_sprintf(p_str, len, FORMAT_FN_NAME, function_name);
        if (task_name) p_str = mfx_trace_sprintf(p_str, len, FORMAT_TASK_NAME, task_name);
        else p_str = mfx_trace_sprintf(p_str, len, FORMAT_TASK_NAME, "");
    }
    if (!b_avg_not_applicable)
    {
        p_str = mfx_trace_sprintf(p_str, len, FORMAT_FULL_STAT,
                                  time,
                                  static_handle->sd5.uint32,
                                  avg_time, std_div);
    }
    else
    {
        p_str = mfx_trace_sprintf(p_str, len, FORMAT_SHORT_STAT,
                                  time,
                                  static_handle->sd5.uint32, "", "");
    }
    if (!(g_StatSuppress & MFX_TRACE_STAT_SUPPRESS_CATEGORY))
    {
        p_str = mfx_trace_sprintf(p_str, len, FORMAT_CAT, category);
    }
    if (!(g_StatSuppress & MFX_TRACE_STAT_SUPPRESS_LEVEL))
    {
        p_str = mfx_trace_sprintf(p_str, len, FORMAT_LEV, level);
    }
    if (!(g_StatSuppress & MFX_TRACE_STAT_SUPPRESS_FILE_NAME))
    {
        p_str = mfx_trace_sprintf(p_str, len, FORMAT_FILE_NAME, file_name);
    }
    if (!(g_StatSuppress & MFX_TRACE_STAT_SUPPRESS_LINE_NUM))
    {
        p_str = mfx_trace_sprintf(p_str, len, FORMAT_LINE_NUM, line_num);
    }
    {
        p_str = mfx_trace_sprintf(p_str, len, "\n");
    }
    fprintf(g_mfxTraceStatFile, "%s", str);
    fflush(g_mfxTraceStatFile);

    return 0;
}

/*------------------------------------------------------------------------------*/

mfxTraceU32 MFXTraceStat_BeginTask(mfxTraceStaticHandle *static_handle,
                              const char *file_name, mfxTraceU32 line_num,
                              const char *function_name,
                              mfxTraceChar* /*category*/, mfxTraceLevel /*level*/,
                              const char *task_name, mfxTraceTaskHandle *task_handle,
                              const void * /*task_params*/)
{
    if (!static_handle || !task_handle) return 1;

    if (0 == static_handle->sd5.uint32)
    {
        static_handle->sd1.str    = (char*)file_name;
        static_handle->sd2.uint32 = line_num;
        static_handle->sd3.str    = (char*)function_name;
        if (task_name)
        {
            static_handle->sd4.str = (char*)malloc( (strlen(task_name)+1)*sizeof(char) );
            if (static_handle->sd4.str)
            {
                strcpy(static_handle->sd4.str, task_name);
            }
        }
        else static_handle->sd4.str = NULL;
    }
    task_handle->sd1.tick = mfx_trace_get_tick();

    return 0;
}

/*------------------------------------------------------------------------------*/

mfxTraceU32 MFXTraceStat_EndTask(mfxTraceStaticHandle *static_handle,
                            mfxTraceTaskHandle *task_handle)
{
    if (!static_handle || !task_handle) return 1;


    mfxTraceTick time = 0;

    ++(static_handle->sd5.uint32);
    time = mfx_trace_get_tick() - task_handle->sd1.tick;
    static_handle->sd6.tick += time;
    static_handle->sd7.tick += time*time;

    if (1 == static_handle->sd5.uint32)
    {
        return g_StatGlobalHandle.AddItem(static_handle);
    }
    return 0;
}

} // extern "C"
#endif // #ifdef MFX_TRACE_ENABLE_STAT
