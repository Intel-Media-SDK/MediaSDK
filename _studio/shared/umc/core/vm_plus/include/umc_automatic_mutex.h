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

#ifndef __UMC_AUTOMATIC_MUTEX_H__
#define __UMC_AUTOMATIC_MUTEX_H__

#include "vm_mutex.h"

namespace UMC
{

#pragma pack(1)

class AutomaticMutex
{
private:
    // The following overloading 'new' operators prohibit
    // creating the object on the heap which is inefficient from
    // performance/memory point of view
    void* operator new(size_t);
    void* operator new(size_t, void*);
    void* operator new[](size_t);
    void* operator new[](size_t, void*);

public:
    // Constructor
    AutomaticMutex(vm_mutex &mutex)
    {
        if (vm_mutex_is_valid(&mutex))
        {
            m_pMutex = &mutex;

            // lock mutex
            vm_mutex_lock(m_pMutex);
            m_bLocked = true;
        }
        else
        {
            m_pMutex = (vm_mutex *) 0;
            m_bLocked = false;
        }
    }

    // Destructor
    ~AutomaticMutex(void)
    {
        Unlock();
    }

    // lock mutex again
    void Lock(void)
    {
        if (m_pMutex)
        {
            if ((vm_mutex_is_valid(m_pMutex)) && (false == m_bLocked))
            {
                vm_mutex_lock(m_pMutex);
                m_bLocked = true;
            }
        }
    }

    // Unlock mutex
    void Unlock(void)
    {
        if (m_pMutex)
        {
            if ((vm_mutex_is_valid(m_pMutex)) && (m_bLocked))
            {
                vm_mutex_unlock(m_pMutex);
                m_bLocked = false;
            }
        }
    }

protected:
    vm_mutex *m_pMutex;                                         // (vm_mutex *) pointer to using mutex
    bool m_bLocked;                                             // (bool) mutex is own locked
};

#pragma pack()

} // end namespace UMC

#endif // __UMC_AUTOMATIC_MUTEX_H__
