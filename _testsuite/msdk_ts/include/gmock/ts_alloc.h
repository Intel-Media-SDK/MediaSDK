/* /////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018 Intel Corporation
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
*/

#pragma once

#include "ts_common.h"

class tsBufferAllocator : public buffer_allocator
{
public:
    tsBufferAllocator();
    ~tsBufferAllocator();
};

class tsSurfacePool
{
private:
    frame_allocator* m_allocator;
    bool             m_external;
    bool             m_isd3d11;
    std::vector<mfxFrameSurface1> m_pool;
    std::vector<mfxFrameSurface1*> m_opaque_pool;
    mfxFrameAllocResponse m_response;

    void Close();

public:
    tsSurfacePool(frame_allocator* allocator = 0, bool d3d11 = !!((g_tsImpl) & 0xF00));
    virtual ~tsSurfacePool();

    inline frame_allocator* GetAllocator() { return m_allocator; }
    inline mfxU32           PoolSize()     { return (mfxU32)m_pool.size(); }

    void SetAllocator       (frame_allocator* allocator, bool external);
    void UseDefaultAllocator(bool isSW = false);
    void AllocOpaque        (mfxFrameAllocRequest request, mfxExtOpaqueSurfaceAlloc& osa);

    mfxStatus AllocSurfaces (mfxFrameAllocRequest request, bool direct_pointers = true);
    mfxStatus FreeSurfaces  ();
    mfxStatus LockSurface   (mfxFrameSurface1& s);
    mfxStatus UnlockSurface (mfxFrameSurface1& s);

    virtual mfxFrameSurface1* GetSurface();

    mfxFrameSurface1* GetSurface(mfxU32 ind);

    mfxStatus UpdateSurfaceResolutionInfo(const mfxU16 &new_width, const mfxU16 &new_height);
    mfxStatus CheckLockedCounter();
};
