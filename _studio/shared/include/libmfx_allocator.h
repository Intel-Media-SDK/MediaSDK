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

#ifndef _LIBMFX_ALLOCATOR_H_
#define _LIBMFX_ALLOCATOR_H_

#include <vector>
#include "mfxvideo.h"


// Internal Allocators
namespace mfxDefaultAllocator
{
    mfxStatus AllocBuffer(mfxHDL pthis, mfxU32 nbytes, mfxU16 type, mfxMemId *mid);
    mfxStatus LockBuffer(mfxHDL pthis, mfxMemId mid, mfxU8 **ptr);
    mfxStatus UnlockBuffer(mfxHDL pthis, mfxMemId mid);
    mfxStatus FreeBuffer(mfxHDL pthis, mfxMemId mid);

    mfxStatus AllocFrames(mfxHDL pthis, mfxFrameAllocRequest *request, mfxFrameAllocResponse *response);
    mfxStatus LockFrame(mfxHDL pthis, mfxMemId mid, mfxFrameData *ptr);
    mfxStatus GetHDL(mfxHDL pthis, mfxMemId mid, mfxHDL *handle);
    mfxStatus UnlockFrame(mfxHDL pthis, mfxMemId mid, mfxFrameData *ptr=0);
    mfxStatus FreeFrames(mfxHDL pthis, mfxFrameAllocResponse *response);

    struct BufferStruct
    {
        mfxHDL      allocator;
        mfxU32      id;
        mfxU32      nbytes;
        mfxU16      type;
    };
    struct FrameStruct
    {
        mfxU32          id;
        mfxFrameInfo    info;
    };
}

class mfxWideBufferAllocator
{
public:
    std::vector<mfxDefaultAllocator::BufferStruct*> m_bufHdl;
    mfxWideBufferAllocator(void);
    ~mfxWideBufferAllocator(void);
    mfxBufferAllocator bufferAllocator;
};

class mfxBaseWideFrameAllocator
{
public:
    mfxBaseWideFrameAllocator(mfxU16 type = 0);
    virtual ~mfxBaseWideFrameAllocator();
    mfxFrameAllocator       frameAllocator;
    mfxWideBufferAllocator  wbufferAllocator;
    mfxU32                  NumFrames;
    std::vector<mfxHDL>     m_frameHandles;
    // Type of this allocator
    mfxU16                  type;
};
class mfxWideSWFrameAllocator : public  mfxBaseWideFrameAllocator
{
public:
    mfxWideSWFrameAllocator(mfxU16 type);
    virtual ~mfxWideSWFrameAllocator(void) {};
};

#endif

