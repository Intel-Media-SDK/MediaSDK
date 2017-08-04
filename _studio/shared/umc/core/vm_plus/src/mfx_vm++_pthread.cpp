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

#include "mfx_vm++_pthread.h"

MfxAutoMutex::MfxAutoMutex(MfxMutex& mutex):
    m_rMutex(mutex),
    m_bLocked(false)
{
    if (MFX_ERR_NONE != Lock()) throw std::bad_alloc();
}

MfxAutoMutex::~MfxAutoMutex(void)
{
    Unlock();
}

mfxStatus MfxAutoMutex::Lock(void)
{
    mfxStatus sts = MFX_ERR_NONE;
    if (!m_bLocked)
    {
        if (!m_rMutex.TryLock())
        {
            /* add time measurement here to estimate how long you sleep on mutex... */
            sts = m_rMutex.Lock();
        }
        m_bLocked = true;
    }
    return sts;
}

mfxStatus MfxAutoMutex::Unlock(void)
{
    mfxStatus sts = MFX_ERR_NONE;
    if (m_bLocked)
    {
        sts = m_rMutex.Unlock();
        m_bLocked = false;
    }
    return sts;
}
