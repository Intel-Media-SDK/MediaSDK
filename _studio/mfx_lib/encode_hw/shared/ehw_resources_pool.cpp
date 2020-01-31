// Copyright (c) 2020 Intel Corporation
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

#include "ehw_resources_pool.h"

namespace MfxEncodeHW
{

ResPool::ResPool(VideoCORE& core)
    : m_core(core)
{
}

ResPool::~ResPool()
{
    Free();
}

ResPool::Resource ResPool::Acquire()
{
    Resource res;
    res.Idx = mfxU8(std::find(m_locked.begin(), m_locked.end(), 0u) - m_locked.begin());

    if (res.Idx >= GetResponse().NumFrameActual)
    {
        res.Idx = IDX_INVALID;
        return res;
    }

    Lock(res.Idx);
    ClearFlag(res.Idx);

    res.Mid = GetResponse().mids[res.Idx];

    return res;
}

void ResPool::Free()
{
    if (m_response.mids)
    {
        m_response.NumFrameActual = m_numFrameActual;

        m_core.FreeFrames(&m_response);

        m_response.mids = 0;
    }
}

mfxStatus ResPool::Alloc(
    const mfxFrameAllocRequest& request
    , bool isCopyRequired)
{
    auto req = request;
    req.NumFrameSuggested = req.NumFrameMin;

    mfxStatus sts = m_core.AllocFrames(&req, &m_response, isCopyRequired);
    MFX_CHECK_STS(sts);

    MFX_CHECK(m_response.NumFrameActual >= req.NumFrameMin, MFX_ERR_MEMORY_ALLOC);

    m_locked.resize(req.NumFrameMin, 0);
    std::fill(m_locked.begin(), m_locked.end(), 0);

    m_flag.resize(req.NumFrameMin, 0);
    std::fill(m_flag.begin(), m_flag.end(), 0);

    m_info                    = req.Info;
    m_numFrameActual          = m_response.NumFrameActual;
    m_response.NumFrameActual = req.NumFrameMin;
    m_bExternal               = false;
    m_bOpaque                 = false;

    return MFX_ERR_NONE;
}

mfxStatus ResPool::AllocOpaque(
    const mfxFrameInfo & info
    , mfxU16 type
    , mfxFrameSurface1 **surfaces
    , mfxU16 numSurface)
{
    mfxFrameAllocRequest req = {};

    req.Info = info;
    req.NumFrameMin = req.NumFrameSuggested = numSurface;
    req.Type = type;

    mfxStatus sts = m_core.AllocFrames(&req, &m_response, surfaces, numSurface);
    MFX_CHECK_STS(sts);
    MFX_CHECK(m_response.NumFrameActual >= numSurface, MFX_ERR_MEMORY_ALLOC);

    m_info                    = info;
    m_numFrameActual          = m_response.NumFrameActual;
    m_response.NumFrameActual = req.NumFrameMin;
    m_bOpaque                 = true;

    return sts;
}

mfxU32 ResPool::Lock(mfxU32 idx)
{
    if (idx >= m_locked.size())
        return 0;
    assert(m_locked[idx] < 0xffffffff);
    return ++m_locked[idx];
}

void ResPool::ClearFlag(mfxU32 idx)
{
    assert(idx < m_flag.size());
    if (idx < m_flag.size())
    {
        m_flag[idx] = 0;
    }
}

void ResPool::SetFlag(mfxU32 idx, mfxU32 flag)
{
    assert(idx < m_flag.size());
    if (idx < m_flag.size())
    {
        m_flag[idx] |= flag;
    }
}

mfxU32 ResPool::GetFlag(mfxU32 idx)
{
    assert(idx < m_flag.size());
    if (idx < m_flag.size())
    {
        return m_flag[idx];
    }
    return 0;
}

void ResPool::UnlockAll()
{
    std::fill(m_locked.begin(), m_locked.end(), 0);
    std::fill(m_flag.begin(), m_flag.end(), 0);
}

mfxU32 ResPool::Unlock(mfxU32 idx)
{
    if (idx >= m_locked.size())
        return mfxU32(-1);
    assert(m_locked[idx] > 0);
    return --m_locked[idx];
}

mfxU32 ResPool::Locked(mfxU32 idx) const
{
    return (idx < m_locked.size()) ? m_locked[idx] : 1;
}

} //namespace MfxEHW