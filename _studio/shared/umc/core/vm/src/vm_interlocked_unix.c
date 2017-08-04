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

#include <vm_interlocked.h>

static uint16_t vm_interlocked_add16(volatile uint16_t *pVariable, uint16_t value_to_add)
{
    asm volatile ("lock; xaddw %0,%1"
                  : "=r" (value_to_add), "=m" (*pVariable)
                  : "0" (value_to_add), "m" (*pVariable)
                  : "memory", "cc");
    return value_to_add;
}

static uint32_t vm_interlocked_add32(volatile uint32_t *pVariable, uint32_t value_to_add)
{
    asm volatile ("lock; xaddl %0,%1"
                  : "=r" (value_to_add), "=m" (*pVariable)
                  : "0" (value_to_add), "m" (*pVariable)
                  : "memory", "cc");
    return value_to_add;
}

uint16_t vm_interlocked_inc16(volatile uint16_t *pVariable)
{
    return vm_interlocked_add16(pVariable, 1) + 1;
} /* uint16_t vm_interlocked_inc16(uint16_t *pVariable) */

uint16_t vm_interlocked_dec16(volatile uint16_t *pVariable)
{
    return vm_interlocked_add16(pVariable, (uint16_t)-1) - 1;
} /* uint16_t vm_interlocked_dec16(uint16_t *pVariable) */

uint32_t vm_interlocked_inc32(volatile uint32_t *pVariable)
{
    return vm_interlocked_add32(pVariable, 1) + 1;
} /* uint32_t vm_interlocked_inc32(uint32_t *pVariable) */

uint32_t vm_interlocked_dec32(volatile uint32_t *pVariable)
{
    return vm_interlocked_add32(pVariable, (uint32_t)-1) - 1;
} /* uint32_t vm_interlocked_dec32(uint32_t *pVariable) */

uint32_t vm_interlocked_cas32(volatile uint32_t *pVariable, uint32_t value_to_exchange, uint32_t value_to_compare)
{
    uint32_t previous_value;

    asm volatile ("lock; cmpxchgl %1,%2"
                  : "=a" (previous_value)
                  : "r" (value_to_exchange), "m" (*pVariable), "0" (value_to_compare)
                  : "memory", "cc");
    return previous_value;
} /* uint32_t vm_interlocked_cas32(volatile uint32_t *pVariable, uint32_t value_to_exchange, uint32_t value_to_compare) */

uint32_t vm_interlocked_xchg32(volatile uint32_t *pVariable, uint32_t value)
{
    uint32_t previous_value = value;

    asm volatile ("lock; xchgl %0,%1"
                  : "=r" (previous_value), "+m" (*pVariable)
                  : "0" (previous_value));
    return previous_value;
} /* uint32_t vm_interlocked_xchg32(volatile uint32_t *pVariable, uint32_t value); */

#else
# pragma warning( disable: 4206 )
#endif /* #if defined(LINUX32) || defined(__APPLE__) */
