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

#ifndef __UMC_FRAME_ALLOCATOR_H__
#define __UMC_FRAME_ALLOCATOR_H__

#include "umc_defs.h"
#include "umc_structures.h"
#include "umc_dynamic_cast.h"
#include "umc_mutex.h"

#define FMID_INVALID 0

namespace UMC
{

class VideoDataInfo;
class FrameData;

typedef int32_t FrameMemID;

enum
{
    FRAME_MID_INVALID = -1
};

/*enum UMC_ALLOC_FLAGS
{
    UMC_ALLOC_PERSISTENT  = 0x01,
    UMC_ALLOC_FUNCLOCAL   = 0x02
};*/

class FrameAllocatorParams
{
     DYNAMIC_CAST_DECL_BASE(FrameAllocatorParams)

public:
    FrameAllocatorParams(){}
    virtual ~FrameAllocatorParams(){}
};

class FrameAllocator
{
    DYNAMIC_CAST_DECL_BASE(FrameAllocator)

public:
    FrameAllocator(void){}
    virtual ~FrameAllocator(void){}

    // Initiates object
    virtual Status Init(FrameAllocatorParams *) { return UMC_OK;}

    // Closes object and releases all allocated memory
    virtual Status Close() = 0;

    virtual Status Reset() = 0;

    // Allocates or reserves physical memory and returns unique ID
    // Sets lock counter to 0
    virtual Status Alloc(FrameMemID *pNewMemID, const VideoDataInfo * info, uint32_t Flags) = 0;

    virtual Status GetFrameHandle(UMC::FrameMemID MID, void * handle) = 0;

    // Lock() provides pointer from ID. If data is not in memory (swapped)
    // prepares (restores) it. Increases lock counter
    virtual const FrameData* Lock(FrameMemID MID) = 0;

    // Unlock() decreases lock counter
    virtual Status Unlock(FrameMemID MID) = 0;

    // Notifies that the data won't be used anymore. Memory can be freed.
    virtual Status IncreaseReference(FrameMemID MID) = 0;

    // Notifies that the data won't be used anymore. Memory can be freed.
    virtual Status DecreaseReference(FrameMemID MID) = 0;

protected:
    Mutex m_guard;

private:
    // Declare private copy constructor to avoid accidental assignment
    // and klocwork complaining.
    FrameAllocator(const FrameAllocator &);
    FrameAllocator & operator = (const FrameAllocator &);
};

} //namespace UMC

#endif //__UMC_FRAME_ALLOCATOR_H__
