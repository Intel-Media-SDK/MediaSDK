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

#ifndef __MFX_VMPLUSPLUS_PTHREAD_H__
#define __MFX_VMPLUSPLUS_PTHREAD_H__

#include "mfxdefs.h"

#include <new>

/* Getting platform-specific definitions. */
#include "sys/mfx_vm++_pthread_windows.h"
#include "sys/mfx_vm++_pthread_unix.h"

/*------------------------------------------------------------------------------*/

class MfxMutex
{
public:
    MfxMutex(void);
    virtual ~MfxMutex(void);

    mfxStatus Lock(void);
    mfxStatus Unlock(void);
    bool TryLock(void);
    
private: // variables
    MfxMutexHandle m_handle;

private: // functions
    MfxMutex(const MfxMutex&);
    MfxMutex& operator=(const MfxMutex&);
};

/*------------------------------------------------------------------------------*/

class MfxAutoMutex
{
public:
    MfxAutoMutex(MfxMutex& mutex);
    virtual ~MfxAutoMutex(void);

    mfxStatus Lock(void);
    mfxStatus Unlock(void);

private: // variables
    MfxMutex& m_rMutex;
    bool m_bLocked;

private: // functions
    MfxAutoMutex(const MfxAutoMutex&);
    MfxAutoMutex& operator=(const MfxAutoMutex&);
    // The following overloading 'new' operators prohibit
    // creating the object on the heap which is inefficient from
    // performance/memory point of view
    void* operator new(size_t);
    void* operator new(size_t, void*);
    void* operator new[](size_t);
    void* operator new[](size_t, void*);
};

/*------------------------------------------------------------------------------*/

#endif //__MFX_VMPLUSPLUS_PTHREAD_H__
