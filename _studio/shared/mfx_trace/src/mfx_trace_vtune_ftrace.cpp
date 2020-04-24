// Copyright (c) 2017-2020 Intel Corporation
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

#if defined(MFX_TRACE_ENABLE_FTRACE) && (defined(LINUX32) || defined(ANDROID))


extern "C" {

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "mfx_trace_utils.h"
#include "mfx_trace_ftrace.h"


static int trace_handle = -1;
static char debugfs_prefix[MAX_PATH+1];
static char trace_file_name[MAX_PATH+1];

namespace
{
    // not thread-safe
    const char *get_debugfs_prefix()
    {
        FILE *fp = fopen("/proc/mounts","r");

        if (!fp)
        {
            return "noperm";
        }

        bool found = false;
        while (!found)
        {
            char type[100];
            if (fscanf(fp, "%*s %256s %99s %*s %*d %*d\n", debugfs_prefix, type) != 2)
            {
                // eof reached
                break;
            }
            if (strcmp(type, "debugfs") == 0)
            {
                // got debugfs line
                found = true;
            }
        }
        fclose(fp);

        if (!found)
        {
            return "notfound";
        }

        return debugfs_prefix;
    }

    // not thread-safe
    const char *get_tracing_file(const char *file_name)
    {
        snprintf(trace_file_name, MAX_PATH, "%.160s/%.96s", get_debugfs_prefix(), file_name);
        return trace_file_name;
    }

}
/*------------------------------------------------------------------------------*/

mfxTraceU32 MFXTraceFtrace_Init()
{
    MFXTraceFtrace_Close();

    const char* file_name = get_tracing_file("tracing/trace_marker");
    trace_handle = open(file_name , O_WRONLY);
    if (trace_handle == -1)
    {
        return 1;
    }
    return 0;
}

/*------------------------------------------------------------------------------*/

mfxTraceU32 MFXTraceFtrace_Close(void)
{
    if (trace_handle != -1)
        close(trace_handle);
    trace_handle = -1;

    return 0;
}

/*------------------------------------------------------------------------------*/

mfxTraceU32 MFXTraceFtrace_SetLevel(mfxTraceChar* /*category*/, mfxTraceLevel /*level*/)
{
    return 1;
}

/*------------------------------------------------------------------------------*/

mfxTraceU32 MFXTraceFtrace_DebugMessage(mfxTraceStaticHandle* static_handle,
    const char * file_name, mfxTraceU32 line_num,
    const char * function_name,
    mfxTraceChar* category, mfxTraceLevel level,
    const char * message, const char * format, ...)
{
    mfxTraceU32 res = 0;
    va_list args;

    va_start(args, format);
    res = MFXTraceFtrace_vDebugMessage(static_handle,
        file_name , line_num,
        function_name,
        category, level,
        message, format, args);
    va_end(args);
    return res;
}

/*------------------------------------------------------------------------------*/

static const int NUMBER_OF_SEPARATORS = 5;

/* This function filters out messages that do not match the following pattern:
 * A|<action>|<codec>|<type>|<details1>|<details2>
 * It counts amount of '|' separators.
 */
mfxTraceU32 MFXTraceFtrace_vDebugMessage(mfxTraceStaticHandle* //static_handle
    , const char * //file_name
    , mfxTraceU32 //line_num
    , const char * function_name
    , mfxTraceChar* //category
    , mfxTraceLevel level
    , const char * message
    , const char * format
    , va_list args
    )
{
    (void)function_name;

    if (trace_handle == -1) return 1;

    if (MFX_TRACE_LEVEL_INTERNAL_VTUNE != level)
        return 0;

    if (!message)
        return 0;

    size_t len = MFX_TRACE_MAX_LINE_LENGTH;
    char str[MFX_TRACE_MAX_LINE_LENGTH] = {0}, *p_str = str;

    p_str = mfx_trace_sprintf(p_str, len, "msdk_v1: ");
    p_str = mfx_trace_sprintf(p_str, len, "%s", message);
    if (format)
    {
        p_str = mfx_trace_vsprintf(p_str, len, format, args);
    }
    p_str = mfx_trace_sprintf(p_str, len, "\n");

    int separators = 0;
    for (int i = 0; i < MFX_TRACE_MAX_LINE_LENGTH && str[i]; ++i)
    {
        separators += (str[i] == '|') ? 1 : 0;
    }

    if (separators != NUMBER_OF_SEPARATORS)
    {
        return 0;
    }

    int ret_value = 0;
    if (write(trace_handle, str, strlen(str)) == -1)
    {
        ret_value = 1;
    }

    if (fsync(trace_handle) == -1)
    {
        ret_value = 1;
    }

    return ret_value;
}

/*------------------------------------------------------------------------------*/

mfxTraceU32 MFXTraceFtrace_BeginTask(mfxTraceStaticHandle *static_handle
    , const char * //file_name
    , mfxTraceU32 //line_num
    , const char * //function_name
    , mfxTraceChar* //category
    , mfxTraceLevel level
    , const char * task_name
    , mfxTraceTaskHandle *handle
    , const void * /*task_params*/)
{
    (void)static_handle;
    (void)task_name;
    (void)handle;

    if (trace_handle == -1) return 1;

    if (MFX_TRACE_LEVEL_INTERNAL_VTUNE == level)
    {
        // TODO: ...
    }

    return 0;
}

/*------------------------------------------------------------------------------*/

mfxTraceU32 MFXTraceFtrace_EndTask(mfxTraceStaticHandle * //static_handle
    , mfxTraceTaskHandle *handle
    )
{
    (void)handle;

    if (trace_handle == -1) return 1;

    return 0;
}

} // extern "C"


#endif  // #if defined(MFX_TRACE_ENABLE_FTRACE) && (defined(LINUX32) || defined(ANDROID))
