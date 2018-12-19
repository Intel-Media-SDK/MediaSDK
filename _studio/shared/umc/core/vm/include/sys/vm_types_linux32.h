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

#if defined(LINUX32)

#include <pthread.h>
#include <sys/types.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <sys/select.h>

#define vm_main main


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#define ENABLE_COUNTER          100
#define DISABLE_COUNTER         101
#define GET_TSC_LOW             102
#define GET_TSC_HIGH            103

#define VM_ALIGN_DECL(X,Y) __attribute__ ((aligned(X))) Y

#define CONST_LL(X) X##LL
#define CONST_ULL(X) X##ULL

#define vm_timeval timeval
#define vm_timezone timezone

/* vm_event.h */
typedef struct vm_event
{
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    int32_t manual;
    int32_t state;
} vm_event;

/* vm_mmap.h */
typedef struct vm_mmap
{
    int32_t fd;
    void *address;
    size_t sizet;
    int32_t fAccessAttr;
} vm_mmap;

/* vm_semaphore.h */
typedef struct vm_semaphore
{
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    int32_t count;
    int32_t max_count;
} vm_semaphore;

typedef struct vm_time
{
   long long start;
   long long diff;
   long long freq;
} vm_time;



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LINUX32 */
