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

#include "vm_time.h"

#if defined(LINUX32)

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>

/* yield the execution of current thread for msec miliseconds */
void vm_time_sleep(uint32_t msec)
{
    if (msec)
        usleep(1000 * msec);
    else
        sched_yield();
} /* void vm_time_sleep(uint32_t msec) */

uint32_t vm_time_get_current_time(void)
{
    struct timeval tval;

    if (0 != gettimeofday(&tval, NULL)) return 0;
    return 1000 * tval.tv_sec + (uint32_t)((double)tval.tv_usec/(double)1000);
} /* uint32_t vm_time_get_current_time(void) */

#define VM_TIME_MHZ 1000000

/* obtain the clock tick of an uninterrupted master clock */
vm_tick vm_time_get_tick(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (vm_tick)tv.tv_sec * (vm_tick)VM_TIME_MHZ + (vm_tick)tv.tv_usec;

} /* vm_tick vm_time_get_tick(void) */

/* obtain the master clock resolution */
vm_tick vm_time_get_frequency(void)
{
    return (vm_tick)VM_TIME_MHZ;

} /* vm_tick vm_time_get_frequency(void) */

/* Create the object of time measure */
vm_status vm_time_open(vm_time_handle *handle)
{
   if (NULL == handle)
       return VM_NULL_PTR;

   *handle = -1;
   return VM_OK;

} /* vm_status vm_time_open(vm_time_handle *handle) */

/* Initialize the object of time measure */
vm_status vm_time_init(vm_time *m)
{
   if (NULL == m)
       return VM_NULL_PTR;
   m->start = 0;
   m->diff = 0;
   m->freq = vm_time_get_frequency();
   return VM_OK;

} /* vm_status vm_time_init(vm_time *m) */

/* Close the object of time measure */
vm_status vm_time_close(vm_time_handle *handle)
{
   if (NULL == handle)
       return VM_NULL_PTR;

   *handle = -1;
   return VM_OK;

} /* vm_status vm_time_close(vm_time_handle *handle) */

/* Start the process of time measure */
vm_status vm_time_start(vm_time_handle handle, vm_time *m)
{
   (void)handle;

   if (NULL == m)
       return VM_NULL_PTR;

   m->start = vm_time_get_tick();
   return VM_OK;

} /* vm_status vm_time_start(vm_time_handle handle, vm_time *m) */

/* Stop the process of time measure and obtain the sampling time in seconds */
double vm_time_stop(vm_time_handle handle, vm_time *m)
{
   (void)handle;

   double speed_sec;
   long long end;

   end = vm_time_get_tick();
   m->diff += (end - m->start);

   if(m->freq == 0) m->freq = vm_time_get_frequency();
   speed_sec = (double)m->diff/(double)m->freq;

   return speed_sec;

} /* double vm_time_stop(vm_time_handle handle, vm_time *m) */

vm_status vm_time_gettimeofday( struct vm_timeval *TP, struct vm_timezone *TZP ) {
  return (gettimeofday(TP, TZP) == 0) ? 0 : VM_NOT_INITIALIZED;

}
#endif /* LINUX32 */
