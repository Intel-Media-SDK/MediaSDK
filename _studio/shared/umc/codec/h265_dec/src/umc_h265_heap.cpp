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

#include "umc_defs.h"
#ifdef UMC_ENABLE_H265_VIDEO_DECODER

#include "umc_structures.h"
#include "umc_h265_heap.h"
#include "umc_h265_dec_defs.h"
#include <cstdarg>

namespace UMC_HEVC_DECODER
{

enum
{
    COEFFS_BUFFER_ALIGN_VALUE_H265                 = 128
};

CoeffsBuffer::CoeffsBuffer(void)
{
    // reset variables
    m_pbAllocatedBuffer = 0;
    m_lAllocatedBufferSize = 0;

    m_pbBuffer = 0;
    m_lBufferSize = 0;
    m_pbFree = 0;
    m_lFreeSize = 0;
    m_pBuffers = 0;
    m_lItemSize = 0;

} // CoeffsBuffer::CoeffsBuffer(void)

CoeffsBuffer::~CoeffsBuffer(void)
{
    Close();
} // CoeffsBuffer::~CoeffsBuffer(void)

void CoeffsBuffer::Close(void)
{
    // delete buffer
    if (m_pbAllocatedBuffer)
        delete[] m_pbAllocatedBuffer;

    // reset variables
    m_pbAllocatedBuffer = 0;
    m_lAllocatedBufferSize = 0;
    m_pbBuffer = 0;
    m_lBufferSize = 0;
    m_pbFree = 0;
    m_lFreeSize = 0;
    m_pBuffers = 0;
} // void CoeffsBuffer::Close(void)

UMC::Status CoeffsBuffer::Init(int32_t numberOfItems, int32_t sizeOfItem)
{
    int32_t lMaxSampleSize = sizeOfItem + COEFFS_BUFFER_ALIGN_VALUE_H265 + (int32_t)sizeof(BufferInfo);
    int32_t lAllocate = lMaxSampleSize * numberOfItems;

    if ((int32_t)m_lBufferSize != lAllocate)
    {
        Close();

        // allocate buffer
        m_pbAllocatedBuffer = h265_new_array_throw<uint8_t>(lAllocate + COEFFS_BUFFER_ALIGN_VALUE_H265);
        m_lBufferSize = lAllocate;

        m_lAllocatedBufferSize = lAllocate + COEFFS_BUFFER_ALIGN_VALUE_H265;

        // align buffer
        m_pbBuffer = UMC::align_pointer<uint8_t *> (m_pbAllocatedBuffer, COEFFS_BUFFER_ALIGN_VALUE_H265);
    }
    
    m_pBuffers = 0;
    m_pbFree = m_pbBuffer;
    m_lFreeSize = m_lBufferSize;

    // save preferred size(s)
    m_lItemSize = sizeOfItem;
    return UMC::UMC_OK;

} // Status CoeffsBuffer::Init(MediaReceiverParams *init)

bool CoeffsBuffer::IsInputAvailable() const
{
    size_t lFreeSize;

    // check error(s)
    if (NULL == m_pbFree)
        return false;

    // get free size
    if (m_pbFree >= m_pbBuffer + (m_lBufferSize - m_lFreeSize))
    {
        lFreeSize = m_pbBuffer + m_lBufferSize - m_pbFree;
    }
    else
    {
        lFreeSize = m_lFreeSize;
        if (lFreeSize < m_lItemSize + COEFFS_BUFFER_ALIGN_VALUE_H265 + sizeof(BufferInfo))
            return false;
    }

    // check free size
    if (lFreeSize < m_lItemSize + COEFFS_BUFFER_ALIGN_VALUE_H265 + sizeof(BufferInfo))
    {
        // when used data is present,
        // concatenate dummy bytes to last buffer info
        if (m_pBuffers)
        {
            return (m_lFreeSize - lFreeSize > m_lItemSize + COEFFS_BUFFER_ALIGN_VALUE_H265 + sizeof(BufferInfo));
        }
        // when no used data,
        // simple move pointer(s)
        else
        {
            return (m_lFreeSize == m_lBufferSize);
        }
    }

    return true;
} // bool CoeffsBuffer::IsInputAvailable() const

uint8_t* CoeffsBuffer::LockInputBuffer()
{
    size_t lFreeSize;

    // check error(s)
    if (NULL == m_pbFree)
        return 0;

    if (m_pBuffers == 0)
    {  // We could do it, because only one thread could get input buffer
        m_pbFree = m_pbBuffer;
        m_lFreeSize = m_lBufferSize;
    }

    // get free size
    if (m_pbFree >= m_pbBuffer + (m_lBufferSize - m_lFreeSize))
    {
        lFreeSize = m_pbBuffer + m_lBufferSize - m_pbFree;
    }
    else
    {
        lFreeSize = m_lFreeSize;
        if (lFreeSize < m_lItemSize + COEFFS_BUFFER_ALIGN_VALUE_H265 + sizeof(BufferInfo))
            return 0;
    }

    // check free size
    if (lFreeSize < m_lItemSize + COEFFS_BUFFER_ALIGN_VALUE_H265 + sizeof(BufferInfo))
    {
        // when used data is present,
        // concatenate dummy bytes to last buffer info
        if (m_pBuffers)
        {
            BufferInfo *pTemp;

            // find last sample info
            pTemp = m_pBuffers;
            while (pTemp->m_pNext)
                pTemp = pTemp->m_pNext;
            pTemp->m_Size += lFreeSize;

            // update variable(s)
            m_pbFree = m_pbBuffer;
            m_lFreeSize -= lFreeSize;

            return LockInputBuffer();
        }
        // when no used data,
        // simple move pointer(s)
        else
        {
            if (m_lFreeSize == m_lBufferSize)
            {
                m_pbFree = m_pbBuffer;
                return m_pbFree;
            }
            return 0;
        }
    }

    return m_pbFree;
} // bool CoeffsBuffer::LockInputBuffer(MediaData* in)

bool CoeffsBuffer::UnLockInputBuffer(size_t size)
{
    size_t lFreeSize;
    BufferInfo *pTemp;
    uint8_t *pb;

    // check error(s)
    if (NULL == m_pbFree)
        return false;

    // when no data given
    //if (0 == size) // even for size = 0 we should add buffer
      //  return true;

    // get free size
    if (m_pbFree >= m_pbBuffer + (m_lBufferSize - m_lFreeSize))
        lFreeSize = m_pbBuffer + m_lBufferSize - m_pbFree;
    else
        lFreeSize = m_lFreeSize;

    // check free size
    if (lFreeSize < m_lItemSize)
        return false;

    // check used data
    if (size + COEFFS_BUFFER_ALIGN_VALUE_H265 + sizeof(BufferInfo) > lFreeSize) // DEBUG : should not be !!!
    {
        VM_ASSERT(false);
        return false;
    }

    // get new buffer info
    pb = UMC::align_pointer<uint8_t *> (m_pbFree + size, COEFFS_BUFFER_ALIGN_VALUE_H265);
    pTemp = reinterpret_cast<BufferInfo *> (pb);

    size += COEFFS_BUFFER_ALIGN_VALUE_H265 + sizeof(BufferInfo);

    // fill buffer info
    pTemp->m_Size = size;
    pTemp->m_pPointer = m_pbFree;
    pTemp->m_pNext = 0;

    // add sample to end of queue
    if (m_pBuffers)
    {
        BufferInfo *pWork = m_pBuffers;

        while (pWork->m_pNext)
            pWork = pWork->m_pNext;

        pWork->m_pNext = pTemp;
    }
    else
        m_pBuffers = pTemp;

    // update variable(s)
    m_pbFree += size;
    if (m_pbBuffer + m_lBufferSize == m_pbFree)
        m_pbFree = m_pbBuffer;
    m_lFreeSize -= size;

    return true;
} // bool CoeffsBuffer::UnLockInputBuffer(size_t size)

bool CoeffsBuffer::IsOutputAvailable() const
{
    return (0 != m_pBuffers);
} // bool CoeffsBuffer::IsOutputAvailable() const

bool CoeffsBuffer::LockOutputBuffer(uint8_t* &pointer, size_t &size)
{
    // check error(s)
    if (0 == m_pBuffers)
        return false;

    // set used pointer
    pointer = m_pBuffers->m_pPointer;
    size = m_pBuffers->m_Size;
    return true;
} // bool CoeffsBuffer::LockOutputBuffer(uint8_t* &pointer, size_t &size)

bool CoeffsBuffer::UnLockOutputBuffer()
{
    // no filled data is present
    if (0 == m_pBuffers)
        return false;

    // update variable(s)
    m_lFreeSize += m_pBuffers->m_Size;
    m_pBuffers = m_pBuffers->m_pNext;

    return true;
} // bool CoeffsBuffer::UnLockOutputBuffer()

void CoeffsBuffer::Reset()
{
    // align buffer
    m_pbBuffer = UMC::align_pointer<uint8_t *> (m_pbAllocatedBuffer, COEFFS_BUFFER_ALIGN_VALUE_H265);

    m_pBuffers = 0;

    m_pbFree = m_pbBuffer;
    m_lFreeSize = m_lBufferSize;
} //void CoeffsBuffer::Reset()

void CoeffsBuffer::Free()
{
    HeapObject::Free();
}

void HeapObject::Free()
{
    Item * item = (Item *) ((uint8_t*)this - sizeof(Item));
    item->m_heap->Free(this);
}

void RefCounter::IncrementReference() const
{
    m_refCounter++;
}

void RefCounter::DecrementReference()
{
    m_refCounter--;

    VM_ASSERT(m_refCounter >= 0);
    if (!m_refCounter)
    {
        Free();
    }
}

} // namespace UMC_HEVC_DECODER
#endif // UMC_ENABLE_H265_VIDEO_DECODER
