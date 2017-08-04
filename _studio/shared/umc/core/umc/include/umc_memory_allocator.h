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

#ifndef __UMC_MEMORY_ALLOCATOR_H__
#define __UMC_MEMORY_ALLOCATOR_H__

#include "umc_defs.h"
#include "umc_structures.h"
#include "umc_dynamic_cast.h"
#include "umc_mutex.h"

#define MID_INVALID 0

namespace UMC
{

typedef size_t MemID;

enum UMC_ALLOC_FLAGS
{
    UMC_ALLOC_PERSISTENT  = 0x01,
    UMC_ALLOC_FUNCLOCAL   = 0x02
};

class MemoryAllocatorParams
{
     DYNAMIC_CAST_DECL_BASE(MemoryAllocatorParams)

public:
    MemoryAllocatorParams(){}
    virtual ~MemoryAllocatorParams(){}
};

class MemoryAllocator
{
    DYNAMIC_CAST_DECL_BASE(MemoryAllocator)

public:
    MemoryAllocator(void){}
    virtual ~MemoryAllocator(void){}

    // Initiates object
    virtual Status Init(MemoryAllocatorParams *)  { return UMC_OK;}

    // Closes object and releases all allocated memory
    virtual Status Close() = 0;

    // Allocates or reserves physical memory and returns unique ID
    // Sets lock counter to 0
    virtual Status Alloc(MemID *pNewMemID, size_t Size, uint32_t Flags, uint32_t Align = 16) = 0;

    // Lock() provides pointer from ID. If data is not in memory (swapped)
    // prepares (restores) it. Increases lock counter
    virtual void* Lock(MemID MID) = 0;

    // Unlock() decreases lock counter
    virtual Status Unlock(MemID MID) = 0;

    // Notifies that the data won't be used anymore. Memory can be freed.
    virtual Status Free(MemID MID) = 0;

    // Immediately deallocates memory regardless of whether it is in use (locked) or no
    virtual Status DeallocateMem(MemID MID) = 0;

protected:
    Mutex m_guard;

private:
    // Declare private copy constructor to avoid accidental assignment
    // and klocwork complaining.
    MemoryAllocator(const MemoryAllocator &);
    MemoryAllocator & operator = (const MemoryAllocator &);
};

} //namespace UMC

#endif //__UMC_MEMORY_ALLOCATOR_H__
