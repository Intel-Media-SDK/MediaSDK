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

#ifndef __VM_INTERLOCKED_H__
#define __VM_INTERLOCKED_H__

#include "vm_types.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/* Thread-safe 16-bit variable incrementing */
uint16_t vm_interlocked_inc16(volatile uint16_t *pVariable);

/* Thread-safe 16-bit variable decrementing */
uint16_t vm_interlocked_dec16(volatile uint16_t *pVariable);

/* Thread-safe 32-bit variable incrementing */
uint32_t vm_interlocked_inc32(volatile uint32_t *pVariable);

/* Thread-safe 32-bit variable decrementing */
uint32_t vm_interlocked_dec32(volatile uint32_t *pVariable);

/* Thread-safe 32-bit variable comparing and storing */
uint32_t vm_interlocked_cas32(volatile uint32_t *pVariable, uint32_t with, uint32_t cmp);

/* Thread-safe 32-bit variable exchange */
uint32_t vm_interlocked_xchg32(volatile uint32_t *pVariable, uint32_t val);

#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */

#endif /* #ifndef __VM_INTERLOCKED_H__ */
