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

#if !defined(__MFX_LOG_H)
#define __MFX_LOG_H

#include <stddef.h>

// Declare the type of handle to logging object
typedef
void * log_t;

// Initialize the logging stuff. If log path is not set,
// the default log path and object is used.
extern "C"
log_t mfxLogInit(const char *pLogPath = 0);

// Write something to the log. If the log handle is zero,
// the default log file is used.
extern "C"
void mfxLogWriteA(const log_t log, const char *pString, ...);

// Close the specific log. Default log can't be closed.
extern "C"
void mfxLogClose(log_t log);

class mfxLog
{
public:
    // Default constructor
    mfxLog(void)
    {
        log = 0;
    }

    // Destructor
    ~mfxLog(void)
    {
        Close();
    }

    // Initialize the log
    log_t Init(const char *pLogPath)
    {
        // Close the object before initialization
        Close();

        log = mfxLogInit(pLogPath);

        return log;
    }

    // Initialize the log
    void Close()
    {
        mfxLogClose(log);

        log = 0;
    }

    // Handle to the protected log
    log_t log;
};

#endif // __MFX_LOG_H
