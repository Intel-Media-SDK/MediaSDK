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

#include "vm_time.h"

#define VM_TIMEOP(A, B, C, OP) \
{ \
    A = vvalue(B); \
    A = A OP vvalue(C); \
}

#define VM_TIMEDEST \
  destination[0].tv_sec = (uint32_t)(cv0 / 1000000); \
  destination[0].tv_usec = (long)(cv0 % 1000000)

static unsigned long long vvalue( struct vm_timeval* B )
{
  unsigned long long rtv;
  rtv = B[0].tv_sec;
  return ((rtv * 1000000) + B[0].tv_usec);
}


/* common (Linux and Windows time functions
 * may be placed before compilation fence. */
void vm_time_timeradd(struct vm_timeval* destination,  struct vm_timeval* src1, struct vm_timeval* src2)
{
  unsigned long long cv0;
  VM_TIMEOP(cv0, src1, src2, + );
  VM_TIMEDEST;
}

void vm_time_timersub(struct vm_timeval* destination,  struct vm_timeval* src1, struct vm_timeval* src2)
{
  unsigned long long cv0;
  VM_TIMEOP(cv0, src1, src2, - );
  VM_TIMEDEST;
}

int vm_time_timercmp(struct vm_timeval* src1, struct vm_timeval* src2, struct vm_timeval *threshold)
{
  if (threshold == NULL)
    return ( ((unsigned long long *)src1)[0] == ((unsigned long long *)src2)[0] ) ? 1 : 0;
  else
  {
    unsigned long long val1 = vvalue(src1);
    unsigned long long val2 = vvalue(src2);
    unsigned long long vald = (val1 > val2) ? (val1 - val2) : (val2 - val1);
    return ( vald < vvalue(threshold) ) ? 1 : 0;
  }
}

