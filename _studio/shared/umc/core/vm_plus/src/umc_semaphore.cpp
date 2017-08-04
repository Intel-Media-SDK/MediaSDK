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

#include "umc_semaphore.h"

namespace UMC
{

Semaphore::Semaphore(void)
{
    vm_semaphore_set_invalid(&m_handle);

} // Semaphore::Semaphore(void)

Semaphore::~Semaphore(void)
{
    if (vm_semaphore_is_valid(&m_handle))
    {
        vm_semaphore_post(&m_handle);
        vm_semaphore_destroy(&m_handle);
    }

} // Semaphore::~Semaphore(void)

Status Semaphore::Init(int32_t iInitCount)
{
    if (vm_semaphore_is_valid(&m_handle))
    {
        vm_semaphore_post(&m_handle);
        vm_semaphore_destroy(&m_handle);
    }
    return vm_semaphore_init(&m_handle, iInitCount);

} // Status Semaphore::Init(int32_t iInitCount)

Status Semaphore::Init(int32_t iInitCount, int32_t iMaxCount)
{
    if (vm_semaphore_is_valid(&m_handle))
    {
        vm_semaphore_post(&m_handle);
        vm_semaphore_destroy(&m_handle);
    }
    return vm_semaphore_init_max(&m_handle, iInitCount, iMaxCount);

} // Status Semaphore::Init(int32_t iInitCount, int32_t iMaxCount)

} // namespace UMC
