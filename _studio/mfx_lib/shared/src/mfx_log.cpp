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

#include <mfx_log.h>

#include <umc_mutex.h>
#include <umc_automatic_mutex.h>

#if defined(LINUX32) || defined(LINUX64)
#error Overlappped I/O mechanism is not implemented in Unix version.
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/syscall.h>

// Be care. Need to check sizeof.
typedef char byte_t;
typedef long dword_t;
typedef long DWORD;
typedef bool BOOL;
#define FALSE false;
#define TRUE true;

#endif

#include <stdio.h>

using namespace UMC;

// static section of the file
namespace
{

// Declare the byte type
typedef unsigned char byte_t;
typedef unsigned int dword_t;

// Declare the default log path
const char *MFX_LOG_DEFAULT_PATH = "c:\\mfx_log.log";

enum
{
    MFX_LOG_ALIGN_VALUE = 64 * 1024,
    MFX_LOG_NUM_PARTS = 4,
    MFX_LOG_DEF_SIZE = 1024 * 1024
};

// Declare log class
class mfxLogObject
{
public:
    // Default constructor
    mfxLogObject(void)
    {
#if defined(LINUX32) || defined(LINUX64)
        m_file = NULL;
#endif

#if defined(LINUX32) || defined(LINUX64)
#       error Overlappped I/O mechanism is not implemented in Unix version.
#endif

        m_pAlloc = 0;
        m_allocSize = 0;

        memset(m_pBuf, 0, sizeof(m_pBuf));
        m_pCur = 0;
        m_bufSize = 0;

        m_logIdx = 0;

        m_callNumber = 0;
    }

    // Destructor
    ~mfxLogObject(void)
    {
        // exchange the last buffer to avoid data loss
        if (m_pCur)
        {
            if (MFX_LOG_DEF_SIZE != m_bufSize)
            {
                memset(m_pCur, 0, m_bufSize);
                m_pCur += m_bufSize;
                m_bufSize = 0;
                ExchangeBuffer();
            }
        }

        Release();
    }

    bool Init(const char *pLogPath)
    {
        dword_t i;

        // check error(s)
        if (0 == pLogPath)
        {
            return false;
        }

        // release the object before initialzation
        Release();

        // open the destination file
#if defined(LINUX32) || defined(LINUX64)
        m_file = fopen(pLogPath, "rw");
        if (NULL == m_file)
        {
            return false;
        }
#endif

#if defined(LINUX32) || defined(LINUX64)
#       error Overlappped I/O mechanism is not implemented in Unix version.
#endif

        // create the output buffer
        m_pAlloc = new byte_t [MFX_LOG_DEF_SIZE * MFX_LOG_NUM_PARTS + MFX_LOG_ALIGN_VALUE];
        m_allocSize = MFX_LOG_DEF_SIZE * MFX_LOG_NUM_PARTS;

        // set the working buffers
        m_pBuf[0] = m_pAlloc + ((MFX_LOG_ALIGN_VALUE - ((size_t) (m_pAlloc - 0))) & (MFX_LOG_ALIGN_VALUE - 1));
        for (i = 1; i < MFX_LOG_NUM_PARTS; i += 1)
        {
            m_pBuf[i] = m_pBuf[i - 1] + MFX_LOG_DEF_SIZE;
        }
        // set the current buffer position
        m_pCur = m_pBuf[0];
        m_bufSize = MFX_LOG_DEF_SIZE;

        m_callNumber = 0;

        return true;
    }

    void AddString(const char *pStr, size_t strLen)
    {
        AutomaticUMCMutex guard(m_guard);
        char cStr[32];
        size_t callNumLen;

        // check error(s)
        if (false == isReady())
        {
            return;
        }

        // render current call number
#if defined(LINUX32) || defined(LINUX64)
        callNumLen = sprintf(cStr, "[% 8u]", m_callNumber += 1);
#endif
        AddStringSafe(cStr, callNumLen);
        AddStringSafe(pStr, strLen);
    }

    bool isReady(void)
    {
        return (0 != m_pCur);
    }

protected:
    // Release the object
    void Release(void)
    {
        dword_t i;
        DWORD nWritten;

#if defined(LINUX32) || defined(LINUX64)
#       error Overlappped I/O mechanism is not implemented in Unix version.
#endif

#if defined(LINUX32) || defined(LINUX64)
        if (NULL != m_file)
        {
            fclose(m_file);
            m_file = NULL;
        }
#endif

        if (m_pAlloc)
        {
            delete [] m_pAlloc;
        }

#if defined(LINUX32) || defined(LINUX64)
#       error Overlappped I/O mechanism is not implemented in Unix version.
#endif

        m_pAlloc = 0;
        m_allocSize = 0;

        memset(m_pBuf, 0, sizeof(m_pBuf));
        m_pCur = 0;
        m_bufSize = 0;

        m_logIdx = 0;
    }

    void AddStringSafe(const char *pStr, size_t strLen)
    {
        // check error(s)
        if ((0 == pStr) ||
            (strLen > MFX_LOG_DEF_SIZE))
        {
            return;
        }

        do
        {
            size_t copySize;

            // copy the string to the buffer
            copySize = (m_bufSize < strLen) ? (m_bufSize) : (strLen);
            std::copy(pStr, pStr + copySize, m_pCur);
            m_pCur += copySize;
            m_bufSize -= copySize;
            pStr += copySize;
            strLen -= copySize;

            // issue file writing
            if (0 == m_bufSize)
            {
                ExchangeBuffer();
            }

        } while (strLen);
    }

    // Set the current buffer to the flushing state,
    // get a fresh buffer to save.
    void ExchangeBuffer(void)
    {
        dword_t newLogIdx = (m_logIdx + 1) % MFX_LOG_NUM_PARTS;
        DWORD nWritten;
        BOOL bRes;

        // the current buffer has nothing
        if (m_bufSize == MFX_LOG_DEF_SIZE)
        {
            return;
        }

#if defined(LINUX32) || defined(LINUX64)
#       error Overlappped I/O mechanism is not implemented in Unix version.
#endif

#if defined(LINUX32) || defined(LINUX64)
        // start writing operation
        bRes = fwrite(m_pBuf[m_logIdx],
                      MFX_LOG_DEF_SIZE,
                      1,
                      m_file);
        if (MFX_LOG_DEF_SIZE != bRes)
        {
            return;
        }
#endif

        // exchange the buffer
        m_logIdx = newLogIdx;
        m_pCur = m_pBuf[m_logIdx];
        m_bufSize = MFX_LOG_DEF_SIZE;
    }

    // Log guard
    Mutex m_guard;

    // Output file
#if defined(LINUX32) || defined(LINUX64)
    FILE *m_file;
#endif

    // Overlapped I/O are absent on Linux.
    // Unix sockets provides same functionality in the readv() and writev() calls.
    // Some Unixes provide the aio_*() family of functions, but this is not implemented widely at the moment.

    // Allocated buffer
    byte_t *m_pAlloc;
    // Allocated buffer size
    size_t m_allocSize;

    // Array of working buffers
    byte_t *(m_pBuf[MFX_LOG_NUM_PARTS]);

    // Current position in the buffer
    byte_t *m_pCur;
    // Remain buffer size
    size_t m_bufSize;

    // Current log buffer index
    dword_t m_logIdx;

    // Current call number
    dword_t m_callNumber;
};

mfxLogObject defaultLogObject;

const log_t defaultLog = &defaultLogObject;

} // namespace

extern "C"
log_t mfxLogInit(const char *pLogPath)
{
    mfxLogObject *pLog = NULL;

    // check if the default log is initialized already
    if (0 == pLogPath)
    {
        if (defaultLogObject.isReady())
        {
            return (log_t) defaultLog;
        }
    }

    try
    {
        // allocate one more log object
        if (pLogPath)
        {
            pLog = new mfxLogObject();
        }
        else
        {
            pLog = &defaultLogObject;
            pLogPath = MFX_LOG_DEFAULT_PATH;
        }
        if (false == pLog->Init(pLogPath))
        {
            throw (int) 0;
        }
    }
    catch(...)
    {
        // delete log object
        if ((pLog) &&
            (pLog != &defaultLogObject))
        {
            delete pLog;
            pLog = 0;
        }
    }

    return (log_t) pLog;

} // log_t mfxLogInit(const char *pLogPath)

extern "C"
void mfxLogWriteA(const log_t log, const char *pString, ...)
{
    mfxLogObject *pLog = (mfxLogObject *) log;
    char cStr[4096];
    va_list argList;
    size_t strLen;

    if (0 == pLog)
    {
        pLog = &defaultLogObject;
    }

    // create argument list
    va_start(argList, pString);
    // render the string
    strLen = vsprintf(cStr, pString, argList);

    pLog->AddString(cStr, strLen);

} // void mfxLogWriteA(const log_t log, const char *pString, ...)

extern "C"
void mfxLogClose(log_t log)
{
    mfxLogObject *pLog = (mfxLogObject *) log;

    // check if it is not a default log object
    if ((0 == pLog) ||
        (pLog == &defaultLogObject))
    {
        return;
    }

    try
    {
        delete pLog;
    }
    catch(...)
    {
    }

} // void mfxLogClose(log_t log)
