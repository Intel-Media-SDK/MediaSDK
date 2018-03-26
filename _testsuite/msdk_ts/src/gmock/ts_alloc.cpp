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

#include "ts_alloc.h"
#include "time_defs.h"
#include <algorithm>

tsBufferAllocator::tsBufferAllocator()
{
}

tsBufferAllocator::~tsBufferAllocator()
{
    std::vector<buffer>& b = GetBuffers();
    std::vector<buffer>::iterator it = b.begin();
    while(it != b.end())
    {
        (*Free)(this, it->mid);
        it++;
    }
}

tsSurfacePool::tsSurfacePool(frame_allocator* allocator, bool d3d11)
    : m_allocator(allocator)
    , m_external(true)
    , m_isd3d11(d3d11)
{
}

tsSurfacePool::~tsSurfacePool()
{
    Close();
}

void tsSurfacePool::Close()
{
    if(m_allocator && !m_external)
    {
        delete m_allocator;
    }
}

void tsSurfacePool::SetAllocator(frame_allocator* allocator, bool external)
{
    Close();
    m_allocator = allocator;
    m_external = external;
}

void tsSurfacePool::UseDefaultAllocator(bool isSW)
{
    Close();
    m_allocator = new frame_allocator(
        isSW ? frame_allocator::SOFTWARE : (m_isd3d11 ? frame_allocator::HARDWARE_DX11 : frame_allocator::HARDWARE ),
        frame_allocator::ALLOC_MAX,
        frame_allocator::ENABLE_ALL,
        frame_allocator::ALLOC_EMPTY);
    m_external = false;
}


struct SurfPtr
{
    mfxFrameSurface1* m_surf;
    SurfPtr(mfxFrameSurface1* s) : m_surf(s) {}
    mfxFrameSurface1* operator() () { return m_surf++; }
};

void tsSurfacePool::AllocOpaque(mfxFrameAllocRequest request, mfxExtOpaqueSurfaceAlloc& osa)
{
    mfxFrameSurface1 s = {};

    s.Info = request.Info;
    s.Data.TimeStamp = MFX_TIMESTAMP_UNKNOWN;

    if(!s.Info.PicStruct)
    {
        s.Info.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    }

    m_pool.resize(request.NumFrameSuggested, s);
    m_opaque_pool.resize(request.NumFrameSuggested);
    std::generate(m_opaque_pool.begin(), m_opaque_pool.end(), SurfPtr(m_pool.data()));

    if(request.Type & (MFX_MEMTYPE_FROM_ENCODE | MFX_MEMTYPE_FROM_VPPIN))
    {
        osa.In.Type       = request.Type;
        osa.In.NumSurface = request.NumFrameSuggested;
        osa.In.Surfaces   = m_opaque_pool.data();
    }
    else
    {
        osa.Out.Type       = request.Type;
        osa.Out.NumSurface = request.NumFrameSuggested;
        osa.Out.Surfaces   = m_opaque_pool.data();
    }
}

mfxStatus tsSurfacePool::AllocSurfaces(mfxFrameAllocRequest request, bool direct_pointers)
{
    bool isSW = !(request.Type & (MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET|MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET));
    mfxFrameSurface1 s = {};
    mfxFrameAllocRequest* pRequest = &request;

    s.Info = request.Info;
    s.Data.TimeStamp = MFX_TIMESTAMP_UNKNOWN;

    if(!s.Info.PicStruct)
    {
        s.Info.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    }

    if(!m_allocator)
    {
        UseDefaultAllocator(isSW);
    }
    TRACE_FUNC3(m_allocator->AllocFrame, m_allocator, pRequest, &m_response);
    mfxStatus allocSts = m_allocator->AllocFrame(m_allocator, pRequest, &m_response);
    g_tsStatus.check(allocSts);
    TS_CHECK_MFX;

    if (allocSts >= MFX_ERR_NONE)
    {
        for(mfxU16 i = 0; i < m_response.NumFrameActual; i++)
        {
            if(m_response.mids)
            {
                s.Data.Y = 0;
                s.Data.MemId = m_response.mids[i];
                if(direct_pointers && isSW)
                {
                    LockSurface(s);
                    s.Data.MemId = 0;
                }
            }
            m_pool.push_back(s);
        }
    }

    return g_tsStatus.get();
}

//surface pool created in AllocOpaque() or/and AllocSurfaces() is freed in this function
mfxStatus tsSurfacePool::FreeSurfaces()
{
    if (m_pool.size() == 0)
        return MFX_ERR_NONE;

    //If opaque memory is not set, (m_pool.size() > 0) && (m_opaque_pool.size() == 0)
    if (m_opaque_pool.size() == 0 && m_allocator) {
        TRACE_FUNC2(m_allocator->Free, m_allocator, &m_response);
        g_tsStatus.check( m_allocator->Free(m_allocator, &m_response) );
        TS_CHECK_MFX;
    }

    m_pool.clear();
    m_opaque_pool.clear();

    return g_tsStatus.get();
}

mfxStatus tsSurfacePool::LockSurface(mfxFrameSurface1& s)
{
    mfxStatus mfxRes = MFX_ERR_NOT_INITIALIZED;
    if(s.Data.Y)
    {
        return MFX_ERR_NONE;
    }
    if(m_allocator)
    {
        TRACE_FUNC3(m_allocator->LockFrame, m_allocator, s.Data.MemId, &s.Data);
        mfxRes = m_allocator->LockFrame(m_allocator, s.Data.MemId, &s.Data);
    }
    g_tsStatus.check( mfxRes );
    return g_tsStatus.get();
}

mfxStatus tsSurfacePool::UnlockSurface(mfxFrameSurface1& s)
{
    mfxStatus mfxRes = MFX_ERR_NOT_INITIALIZED;
    if(s.Data.MemId)
    {
        return MFX_ERR_NONE;
    }
    if(m_allocator)
    {
        TRACE_FUNC3(m_allocator->UnLockFrame, m_allocator, s.Data.MemId, &s.Data);
        mfxRes = m_allocator->UnLockFrame(m_allocator, s.Data.MemId, &s.Data);
    }
    g_tsStatus.check( mfxRes );
    return g_tsStatus.get();
}

mfxFrameSurface1* tsSurfacePool::GetSurface()
{
    std::vector<mfxFrameSurface1>::iterator it = m_pool.begin();
    // set timer to 30 sec for simulation (simics/MediaSolo)
    // and to 100 ms for real HW
    mfxU32 timeout = g_tsConfig.sim ? 30000 : 100;
    mfxU32 step = 5;

    while (timeout)
    {
        while (it != m_pool.end())
        {
            if (!it->Data.Locked)
                return &(*it);
            it++;
        }
        MSDK_SLEEP(step);
        timeout -= step;
        it = m_pool.begin();
    }
    g_tsLog << "ALL SURFACES ARE LOCKED!\n";
    g_tsStatus.check( MFX_ERR_NULL_PTR );

    return 0;
}

mfxFrameSurface1* tsSurfacePool::GetSurface(mfxU32 ind)
{
    if (ind >= m_pool.size())
        return 0;

    return &m_pool[ind];
}

mfxStatus tsSurfacePool::UpdateSurfaceResolutionInfo(const mfxU16 &new_width, const mfxU16 &new_height)
{
    std::vector<mfxFrameSurface1>::iterator it = m_pool.begin();

    while (it != m_pool.end())
    {
        if (!it->Data.Locked)
        {
            if (new_width > it->Info.Width || new_height > it->Info.Height)
            {
                // the new size cannot be bigger then the old one, because no memory reallocation here
                return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;
            }
            else
            {
                it->Info.Width = new_width;
                it->Info.Height = new_height;
            }
        }
        else
        {
            // some surface is locked and cannot be updated - continue processing does not make sence
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        }
        it++;
    }

    return MFX_ERR_NONE;
}

mfxStatus tsSurfacePool::CheckLockedCounter()
{
    std::vector<mfxFrameSurface1>::iterator it = m_pool.begin();

    while (it != m_pool.end())
    {
        if (it->Data.Locked)
            return MFX_ERR_UNDEFINED_BEHAVIOR;

        it++;
    }
    return MFX_ERR_NONE;
}
