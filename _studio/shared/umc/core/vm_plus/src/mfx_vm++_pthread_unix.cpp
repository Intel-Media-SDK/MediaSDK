// Copyright (c) 2017-2018 Intel Corporation
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


#if !defined _GNU_SOURCE
#  define _GNU_SOURCE /* may need on some OS to support PTHREAD_MUTEX_RECURSIVE */
#endif

#include "mfx_vm++_pthread.h"
#include <assert.h>

struct _MfxMutexHandle
{
    pthread_mutex_t m_mutex;
};

MfxMutex::MfxMutex(void)
{
    int res = 0;
    pthread_mutexattr_t mutex_attr;

    res = pthread_mutexattr_init(&mutex_attr);
    if (!res)
    {
        res = pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
        if (!res) res = pthread_mutex_init(&m_handle.m_mutex, &mutex_attr);
        pthread_mutexattr_destroy(&mutex_attr);
    }
    if (res) throw std::bad_alloc();
}

MfxMutex::~MfxMutex(void)
{
    int res = pthread_mutex_destroy(&m_handle.m_mutex);
    assert(!res); // we experienced undefined behavior
    (void)res;
}

mfxStatus MfxMutex::Lock(void)
{
    return (pthread_mutex_lock(&m_handle.m_mutex))? MFX_ERR_UNKNOWN: MFX_ERR_NONE;
}

mfxStatus MfxMutex::Unlock(void)
{
    return (pthread_mutex_unlock(&m_handle.m_mutex))? MFX_ERR_UNKNOWN: MFX_ERR_NONE;
}

bool MfxMutex::TryLock(void)
{
    return (pthread_mutex_trylock(&m_handle.m_mutex))? false: true;
}

