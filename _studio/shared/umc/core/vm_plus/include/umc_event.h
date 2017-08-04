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

#ifndef __UMC_EVENT_H__
#define __UMC_EVENT_H__

#include "vm_debug.h"
#include "vm_event.h"
#include "umc_structures.h"

namespace UMC
{

class Event
{
public:
    // Default constructor
    Event(void);
    // Destructor
    virtual ~Event(void);

    // Initialize event
    Status Init(int32_t iManual, int32_t iState);
    // Set event to signaled state
    Status Set(void);
    // Set event to non-signaled state
    Status Reset(void);
    // Pulse event (should not be used)
    Status Pulse(void);
    // Wait until event is signaled
    Status Wait(uint32_t msec);
    // Infinitely wait until event is signaled
    Status Wait(void);

protected:
    vm_event m_handle;                                          // (vm_event) handle to system event
};

inline
Status Event::Set(void)
{
    return vm_event_signal(&m_handle);

} // Status Event::Set(void)

inline
Status Event::Reset(void)
{
    return vm_event_reset(&m_handle);

} // Status Event::Reset(void)

inline
Status Event::Pulse(void)
{
    return vm_event_pulse(&m_handle);

} // Status Event::Pulse(void)

inline
Status Event::Wait(uint32_t msec)
{
    return vm_event_timed_wait(&m_handle, msec);

} // Status Event::Wait(uint32_t msec)

inline
Status Event::Wait(void)
{
    return vm_event_wait(&m_handle);

} // Status Event::Wait(void)

} // namespace UMC

#endif // __UMC_EVENT_H__
