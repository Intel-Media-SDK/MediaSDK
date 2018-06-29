/******************************************************************************\
Copyright (c) 2005-2018, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This sample was distributed or derived from the Intel's Media Samples package.
The original version of this sample may be obtained from https://software.intel.com/en-us/intel-media-server-studio
or https://software.intel.com/en-us/media-client-solutions-support.
\**********************************************************************************/

#ifndef __THREAD_DEFS_H__
#define __THREAD_DEFS_H__

#include "mfxdefs.h"
#include "vm/strings_defs.h"

typedef unsigned int (MFX_STDCALL * msdk_thread_callback)(void*);


#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>

struct msdkMutexHandle
{
    pthread_mutex_t m_mutex;
};

struct msdkSemaphoreHandle
{
    msdkSemaphoreHandle(mfxU32 count):
        m_count(count)
    {}

    mfxU32 m_count;
    pthread_cond_t m_semaphore;
    pthread_mutex_t m_mutex;
};

struct msdkEventHandle
{
    msdkEventHandle(bool manual, bool state):
        m_manual(manual),
        m_state(state)
    {}

    bool m_manual;
    bool m_state;
    pthread_cond_t m_event;
    pthread_mutex_t m_mutex;
};

class MSDKEvent;

struct msdkThreadHandle
{
    msdkThreadHandle(
        msdk_thread_callback func,
        void* arg):
      m_func(func),
      m_arg(arg),
      m_event(0),
      m_thread(0)
    {}

    msdk_thread_callback m_func;
    void* m_arg;
    MSDKEvent* m_event;
    pthread_t m_thread;
};


class MSDKMutex: public msdkMutexHandle
{
public:
    MSDKMutex(void);
    ~MSDKMutex(void);

    mfxStatus Lock(void);
    mfxStatus Unlock(void);
    int Try(void);

private:
    MSDKMutex(const MSDKMutex&);
    void operator=(const MSDKMutex&);
};

class AutomaticMutex
{
public:
    AutomaticMutex(MSDKMutex& mutex);
    ~AutomaticMutex(void);

private:
    mfxStatus Lock(void);
    mfxStatus Unlock(void);

    MSDKMutex& m_rMutex;
    bool m_bLocked;

private:
    AutomaticMutex(const AutomaticMutex&);
    void operator=(const AutomaticMutex&);
};

class MSDKSemaphore: public msdkSemaphoreHandle
{
public:
    MSDKSemaphore(mfxStatus &sts, mfxU32 count = 0);
    ~MSDKSemaphore(void);

    mfxStatus Post(void);
    mfxStatus Wait(void);

private:
    MSDKSemaphore(const MSDKSemaphore&);
    void operator=(const MSDKSemaphore&);
};

class MSDKEvent: public msdkEventHandle
{
public:
    MSDKEvent(mfxStatus &sts, bool manual, bool state);
    ~MSDKEvent(void);

    mfxStatus Signal(void);
    mfxStatus Reset(void);
    mfxStatus Wait(void);
    mfxStatus TimedWait(mfxU32 msec);

private:
    MSDKEvent(const MSDKEvent&);
    void operator=(const MSDKEvent&);
};

class MSDKThread: public msdkThreadHandle
{
public:
    MSDKThread(mfxStatus &sts, msdk_thread_callback func, void* arg);
    ~MSDKThread(void);

    mfxStatus Wait(void);
    mfxStatus TimedWait(mfxU32 msec);
    mfxStatus GetExitCode();

    friend void* msdk_thread_start(void* arg);

private:
    MSDKThread(const MSDKThread&);
    void operator=(const MSDKThread&);
};

mfxU32 msdk_get_current_pid();
mfxStatus msdk_setrlimit_vmem(mfxU64 size);
mfxStatus msdk_thread_get_schedtype(const msdk_char*, mfxI32 &type);
void msdk_thread_printf_scheduling_help();

#endif //__THREAD_DEFS_H__
