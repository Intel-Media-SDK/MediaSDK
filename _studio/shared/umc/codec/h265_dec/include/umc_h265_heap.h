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
#ifdef MFX_ENABLE_H265_VIDEO_DECODE

#ifndef __UMC_H265_HEAP_H
#define __UMC_H265_HEAP_H

#include <memory>
#include "umc_mutex.h"
#include "umc_h265_dec_defs.h"
#include "umc_media_data.h"

namespace UMC_HEVC_DECODER
{
//*********************************************************************************************/
// Memory buffer container class
//*********************************************************************************************/
class MemoryPiece
{
public:
    // Default constructor
    MemoryPiece()
    {
        Reset();
    }

    // Destructor
    ~MemoryPiece()
    {
        Release();
    }

    void Release()
    {
        if (m_pSourceBuffer)
            delete[] m_pSourceBuffer;
        Reset();
    }

    void SetData(UMC::MediaData *out)
    {
        Release();

        m_pDataPointer = (uint8_t*)out->GetDataPointer();
        m_nDataSize = out->GetDataSize();
        m_pts = out->GetTime();
    }

    // Allocate memory piece
    bool Allocate(size_t nSize)
    {
        Release();

        // allocate little more
        m_pSourceBuffer = h265_new_array_throw<uint8_t>((int32_t)nSize);
        m_pDataPointer = m_pSourceBuffer;
        m_nSourceSize = nSize;
        return true;
    }

    // Obtain data pointer
    uint8_t *GetPointer(){return m_pDataPointer;}

    size_t GetSize() const {return m_nSourceSize;}

    size_t GetDataSize() const {return m_nDataSize;}
    void SetDataSize(size_t dataSize) {m_nDataSize = dataSize;}

    double GetTime() const {return m_pts;}
    void SetTime(double pts) {m_pts = pts;}

protected:
    uint8_t *m_pSourceBuffer;                                     // (uint8_t *) pointer to source memory
    uint8_t *m_pDataPointer;                                      // (uint8_t *) pointer to source memory
    size_t m_nSourceSize;                                       // (size_t) allocated memory size
    size_t m_nDataSize;                                         // (size_t) data memory size
    double   m_pts;

    void Reset()
    {
        m_pts = 0;
        m_pSourceBuffer = 0;
        m_pDataPointer = 0;
        m_nSourceSize = 0;
        m_nDataSize = 0;
    }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Item class
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Heap_Objects;

class Item
{
public:
    Item(Heap_Objects * heap, void * ptr, size_t size, bool isTyped = false)
        : m_pNext(0)
        , m_Ptr(ptr)
        , m_Size(size)
        , m_isTyped(isTyped)
        , m_heap(heap)
    {
    }

    ~Item()
    {
    }

    Item* m_pNext;
    void * m_Ptr;
    size_t m_Size;
    bool   m_isTyped;
    Heap_Objects * m_heap;

    static Item * Allocate(Heap_Objects * heap, size_t size, bool isTyped = false)
    {
        uint8_t * ppp = new uint8_t[size + sizeof(Item)];
        if (!ppp)
            throw h265_exception(UMC::UMC_ERR_ALLOC);
        Item * item = new (ppp) Item(heap, 0, size, isTyped);
        item->m_Ptr = (uint8_t*)ppp + sizeof(Item);
        return item;
    }

    static void Free(Item *item)
    {
        if (item->m_isTyped)
        {
            HeapObject * obj = reinterpret_cast<HeapObject *>(item->m_Ptr);
            obj->~HeapObject();
        }

        item->~Item();
        delete[] (uint8_t*)item;
    }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Collection of heap objects
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Heap_Objects
{
public:

    Heap_Objects()
        : m_pFirstFree(0)
    {
    }

    virtual ~Heap_Objects()
    {
        Release();
    }

    Item * GetItemForAllocation(size_t size, bool typed = false)
    {
        UMC::AutomaticUMCMutex guard(m_mGuard);

        if (!m_pFirstFree)
        {
            return 0;
        }

        if (m_pFirstFree->m_Size == size && m_pFirstFree->m_isTyped == typed)
        {
            Item * ptr = m_pFirstFree;
            m_pFirstFree = m_pFirstFree->m_pNext;
            VM_ASSERT(ptr->m_Size == size);
            return ptr;
        }

        Item * temp = m_pFirstFree;

        while (temp->m_pNext)
        {
            if (temp->m_pNext->m_Size == size && temp->m_pNext->m_isTyped == typed)
            {
                Item * ptr = temp->m_pNext;
                temp->m_pNext = temp->m_pNext->m_pNext;
                return ptr;
            }

            temp = temp->m_pNext;
        }

        return 0;
    }

    void* Allocate(size_t size, bool isTyped = false)
    {
        Item * item = GetItemForAllocation(size);
        if (!item)
        {
            item = Item::Allocate(this, size, isTyped);
        }

        return item->m_Ptr;
    }

    template<typename T>
    T* Allocate(size_t size = sizeof(T), bool isTyped = false)
    {
        return (T*)Allocate(size, isTyped);
    }

    template <typename T>
    T* AllocateObject()
    {
        Item * item = GetItemForAllocation(sizeof(T), true);

        if (!item)
        {
            void * ptr = Allocate(sizeof(T), true);
            return new(ptr) T();
        }

        return (T*)(item->m_Ptr);
    }

    void FreeObject(void * obj, bool force = false)
    {
        Free(obj, force);
    }

    void Free(void * obj, bool force = false)
    {
        if (!obj)
            return;

        UMC::AutomaticUMCMutex guard(m_mGuard);
        Item * item = (Item *) ((uint8_t*)obj - sizeof(Item));

        // check
        Item * temp = m_pFirstFree;

        while (temp)
        {
            if (temp == item) //was removed yet
                return;

            temp = temp->m_pNext;
        }

        if (force)
        {
            Item::Free(item);
            return;
        }
        else
        {
            if (item->m_isTyped)
            {
                HeapObject * object = reinterpret_cast<HeapObject *>(item->m_Ptr);
                object->Reset();
            }
        }

        item->m_pNext = m_pFirstFree;
        m_pFirstFree = item;
    }

    void Release()
    {
        UMC::AutomaticUMCMutex guard(m_mGuard);

        while (m_pFirstFree)
        {
            Item *pTemp = m_pFirstFree->m_pNext;
            Item::Free(m_pFirstFree);
            m_pFirstFree = pTemp;
        }
    }

private:

    Item * m_pFirstFree;
    UMC::Mutex m_mGuard;
};

//*********************************************************************************************/
// Memory buffer for storing decoded coefficients
//*********************************************************************************************/
class CoeffsBuffer : public HeapObject
{
public:
    // Default constructor
    CoeffsBuffer(void);
    // Destructor
    virtual ~CoeffsBuffer(void);

    // Initialize buffer
    UMC::Status Init(int32_t numberOfItems, int32_t sizeOfItem);

    bool IsInputAvailable() const;
    // Lock input buffer
    uint8_t* LockInputBuffer();
    // Unlock input buffer
    bool UnLockInputBuffer(size_t size);

    bool IsOutputAvailable() const;
    // Lock output buffer
    bool LockOutputBuffer(uint8_t *& pointer, size_t &size);
    // Unlock output buffer
    bool UnLockOutputBuffer();
    // Release object
    void Close(void);
    // Reset object
    virtual void Reset(void);

    virtual void Free();

protected:
    uint8_t *m_pbAllocatedBuffer;       // (uint8_t *) pointer to allocated unaligned buffer
    size_t m_lAllocatedBufferSize;    // (int32_t) size of allocated buffer

    uint8_t *m_pbBuffer;                // (uint8_t *) pointer to allocated buffer
    size_t m_lBufferSize;             // (int32_t) size of using buffer

    uint8_t *m_pbFree;                  // (uint8_t *) pointer to free space
    size_t m_lFreeSize;               // (int32_t) size of free space

    size_t m_lItemSize;               // (int32_t) size of output data portion

    struct BufferInfo
    {
        uint8_t * m_pPointer;
        size_t  m_Size;
        BufferInfo *m_pNext;
    };

    BufferInfo *m_pBuffers;           // (Buffer *) queue of filled sample info
};

} // namespace UMC_HEVC_DECODER

#endif // __UMC_H265_HEAP_H
#endif // MFX_ENABLE_H265_VIDEO_DECODE
