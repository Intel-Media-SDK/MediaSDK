// Copyright (c) 2020 Intel Corporation
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

#include <mfx_trace2_vtune_ftrace.h>

#if defined(MFX_TRACE_ENABLE_FTRACE) && (defined(LINUX32) || defined(ANDROID))
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>

const char *mfx::VTune::get_debugfs_prefix()
{
    FILE *fp = fopen("/proc/mounts","r");
    if (!fp)
        return "noperm";

    bool found = false;
    while (!found)
    {
        char type[100];
        if (fscanf(fp, "%*s %256s %99s %*s %*d %*d\n", debugfs_prefix, type) != 2)
            break; // eof reached
        if (strcmp(type, "debugfs") == 0)
            found = true; // got debugfs line
    }
    fclose(fp);

    if (!found)
        return "notfound";

    return debugfs_prefix;
}

const char *mfx::VTune::get_tracing_file(const char *file_name)
{
    snprintf(trace_file_name, PATH_MAX, "%.160s/%.96s", get_debugfs_prefix(), file_name);
    return trace_file_name;
}

mfx::VTune::VTune()
{
    const char* file_name = get_tracing_file("tracing/trace_marker");
    trace_handle = open(file_name, O_WRONLY);
}

void mfx::VTune::handleEvent(const mfx::Trace::Event &e)
{
    if (e.type != mfx::Trace::EventType::INFO) return;
    if (trace_handle == -1) return;
    if (e.map.find("vtune_ftrace_message") == e.map.end()) return;
    if (e.map.find("vtune_ftrace_message")->second.type != mfx::Trace::NodeType::STRING) return;
    const char *str = e.map.find("vtune_ftrace_message")->second.str.c_str();
    int len = strlen(str);
    if (write(trace_handle, str, len) == -1) return;
    fsync(trace_handle);
}

mfx::VTune::~VTune()
{
    if (trace_handle != -1)
        close(trace_handle);
}

#endif  // MFX_TRACE_ENABLE_FTRACE
