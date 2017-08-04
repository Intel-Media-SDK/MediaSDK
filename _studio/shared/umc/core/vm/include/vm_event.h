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

#ifndef __VM_EVENT_H__
#define __VM_EVENT_H__

#include "vm_types.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/* Invalidate an event */
void vm_event_set_invalid(vm_event *event);

/* Verify if an event is valid */
int32_t vm_event_is_valid(vm_event *event);

/* Init an event. Event is created unset. return 1 if success */
vm_status vm_event_init(vm_event *event, int32_t manual, int32_t state);
vm_status vm_event_named_init(vm_event *event, int32_t manual, int32_t state, const char *pcName);

/* Set the event to either HIGH (>0) or LOW (0) state */
vm_status vm_event_signal(vm_event *event);
vm_status vm_event_reset(vm_event *event);

/* Pulse the event 0 -> 1 -> 0 */
vm_status vm_event_pulse(vm_event *event);

/* Wait for event to be high with blocking */
vm_status vm_event_wait(vm_event *event);

/* Wait for event to be high without blocking, return 1 if signaled */
vm_status vm_event_timed_wait(vm_event *event, uint32_t msec);

/* Destroy the event */
void vm_event_destroy(vm_event *event);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VM_EVENT_H__ */
