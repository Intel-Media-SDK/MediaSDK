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

#ifndef __UMC_SEMAPHORE_H__
#define __UMC_SEMAPHORE_H__

#include "vm_debug.h"
#include "vm_semaphore.h"
#include "umc_structures.h"

namespace UMC
{

class Semaphore
{
public:
    // Default constructor
    Semaphore(void);
    // Destructor
    virtual ~Semaphore(void);

    // Initialize semaphore
    Status Init(int32_t iInitCount);
    Status Init(int32_t iInitCount, int32_t iMaxCount);
    // Check semaphore state
    bool IsValid(void);
    // Try to obtain semaphore
    Status TryWait(void);
    // Wait until semaphore is signaled
    Status Wait(uint32_t msec);
    // Infinitely wait until semaphore is signaled
    Status Wait(void);
    // Set semaphore to signaled state
    Status Signal(uint32_t count = 1);

protected:
    vm_semaphore m_handle;                                      // (vm_semaphore) handle to system semaphore
};

inline
bool Semaphore::IsValid(void)
{
    return vm_semaphore_is_valid(&m_handle) ? true : false;

} // bool Semaphore::IsValid(void)

inline
Status Semaphore::TryWait(void)
{
    return vm_semaphore_try_wait(&m_handle);

} // Status Semaphore::TryWait(void)

inline
Status Semaphore::Wait(uint32_t msec)
{
    return vm_semaphore_timedwait(&m_handle, msec);

} // Status Semaphore::Wait(uint32_t msec)

inline
Status Semaphore::Wait(void)
{
    return vm_semaphore_wait(&m_handle);

} // Status Semaphore::Wait(void)

inline
Status Semaphore::Signal(uint32_t count)
{
    return vm_semaphore_post_many(&m_handle, count);

} // Status Semaphore::Signal(uint32_t count)

} // namespace UMC

#endif // __UMC_SEMAPHORE_H__
