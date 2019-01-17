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

#ifndef __UMC_MUTEX_H__
#define __UMC_MUTEX_H__

#include "mfx_vm++_pthread.h"
#include "vm_debug.h"
#include "umc_structures.h"

namespace UMC
{

class Mutex : public MfxMutex
{
public:
    Mutex(void): MfxMutex() {}
    // Try to get ownership of mutex
    inline Status TryLock(void) { return (MfxMutex::TryLock())? UMC_OK: UMC_ERR_TIMEOUT; }

private: // functions
    Mutex(const Mutex&);
    Mutex& operator=(const Mutex&);
};

class AutomaticUMCMutex : public MfxAutoMutex
{
public:
    // Constructor
    AutomaticUMCMutex(UMC::Mutex &mutex)
        : MfxAutoMutex(mutex)
    {
    }

private: // functions
    // Preventing copy constructor and operator=
    AutomaticUMCMutex(const AutomaticUMCMutex&);
    AutomaticUMCMutex& operator=(const AutomaticUMCMutex&);
    // The following overloading 'new' operators prohibit
    // creating the object on the heap which is inefficient from
    // performance/memory point of view
    void* operator new(size_t);
    void* operator new(size_t, void*);
    void* operator new[](size_t);
    void* operator new[](size_t, void*);
};

} // namespace UMC

#endif // __UMC_MUTEX_H__
