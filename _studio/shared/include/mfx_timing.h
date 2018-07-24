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

#ifndef __MFX_TIMING_H__
#define __MFX_TIMING_H__

//uncomment this define for timing instrumentation
//#define MFX_TIMING

#define MFX_TIMING_CHECK_REGISTRY   0x10000000
#define MFX_TIMING_PROXY_LIBSW      0x20000000
#define MFX_TIMING_PROXY_LIBHW      0x40000000

#define MFX_INVALID_STATUS_CODE     (int)0xffff

// for SetInput/SetOutput
enum
{
    MFX_COMP_DECODE  = 1,
    MFX_COMP_VPP     = 2,
    MFX_COMP_ENCODE  = 3,
};

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

namespace MFX
{


// empty declaration for normal build
class AutoTimer
{
public:
    static int Init(const void * /*filename*/, int /*level = -1*/) { return 0; }
    static unsigned int CreateUniqTaskId() {return 0;}

    AutoTimer(const char * /*name*/){}
    AutoTimer(const char * /*name*/, const char * /*param*/, int /*parami = 0*/)    {}
    AutoTimer(const char * /*name*/, bool /*bCreateId*/, unsigned int /*child_of*/, unsigned int /*parent_of*/)  {}
    ~AutoTimer() {};
    AutoTimer() {};

    void SetInput(void * /*handle*/, int /*component_index = -1*/) {};
    void SetOutput(void * /*handle*/, int /*component_index = -1*/) {};
    void AddParam(const char * /*param_name*/, const char * /*param_value*/) {};
    void AddParam(const char * /*param_name*/, int /*param_value*/) {};
    void Start(const char * /*name*/) {};
    void Stop(int /*return_code = MFX_INVALID_STATUS_CODE*/) {};
};


} // namespace UMC

#endif // __MFX_TIMING_H__
