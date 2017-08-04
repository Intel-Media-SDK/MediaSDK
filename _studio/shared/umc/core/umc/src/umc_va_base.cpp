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

#include "umc_va_base.h"
#include "umc_defs.h"

using namespace UMC;

Status VideoAccelerator::Close(void)
{
    m_allocator = 0;
    return UMC_OK;
}

Status VideoAccelerator::Reset(void)
{
    m_allocator = 0;
    return UMC_OK;
}

void UMCVACompBuffer::SetDataSize(int32_t size)
{
    DataSize = size;
    VM_ASSERT(DataSize <= BufferSize);
}

void UMCVACompBuffer::SetNumOfItem(int32_t )
{
}

Status UMCVACompBuffer::SetPVPState(void *buf, uint32_t size)
{
    if (16 < size)
        return UMC_ERR_ALLOC;
    if (NULL != buf)
    {
        if (0 == size)
            return UMC_ERR_ALLOC;
        PVPState = PVPStateBuf;
        MFX_INTERNAL_CPY(PVPState, buf, size);
    }
    else
        PVPState = NULL;

    return UMC_OK;
}

