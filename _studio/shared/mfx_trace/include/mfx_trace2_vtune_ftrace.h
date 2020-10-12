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

#ifndef __MFX_TRACE2_VTUNE_FTRACE_H__
#define __MFX_TRACE2_VTUNE_FTRACE_H__
#include <mfx_trace2.h>

#if defined(MFX_TRACE_ENABLE_FTRACE) && (defined(LINUX32) || defined(ANDROID))
#include <linux/limits.h>

namespace mfx
{

class VTune : public TraceBackend
{
private:
    int trace_handle = -1;
    char debugfs_prefix[PATH_MAX+1];
    char trace_file_name[PATH_MAX+1];
    const char *get_debugfs_prefix();
    const char *get_tracing_file(const char *file_name);
public:
    VTune();
    void handleEvent(const Trace::Event &e) override;
    ~VTune() override;
};

}

#endif  // MFX_TRACE_ENABLE_FTRACE

#endif  // __MFX_TRACE2_VTUNE_FTRACE_H__
