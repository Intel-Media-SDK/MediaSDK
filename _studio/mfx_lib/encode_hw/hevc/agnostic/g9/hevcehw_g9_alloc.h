// Copyright (c) 2019 Intel Corporation
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

#pragma once

#include "mfx_common.h"
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE)

#include "hevcehw_base.h"
#include "hevcehw_g9_data.h"
#include "mfxstructures.h"
#include <vector>

namespace HEVCEHW
{
namespace Gen9
{

class MfxFrameAllocResponse
    : public mfxFrameAllocResponse
    , public IAllocation
{
public:
    MfxFrameAllocResponse(VideoCORE& core);

    ~MfxFrameAllocResponse();

    virtual
    mfxStatus Alloc(
         mfxFrameAllocRequest & req
        , bool isCopyRequired) override;

    virtual
    mfxStatus AllocOpaque(
        const mfxFrameInfo & info
        , mfxU16 type
        , mfxFrameSurface1 **surfaces
        , mfxU16 numSurface) override;

    virtual
    const mfxFrameAllocResponse& Response() const override
    {
        return *this;
    }

    virtual
    const mfxFrameInfo& Info() const override
    {
        return m_info;
    }

    virtual
    Resource Acquire() override;

    virtual
    void Release(mfxU32 idx) override
    {
        Unlock(idx);
    }

    virtual void   ClearFlag(mfxU32 idx) override;
    virtual void   SetFlag(mfxU32 idx, mfxU32 flag) override;
    virtual mfxU32 GetFlag(mfxU32 idx) override;
    virtual void   Unlock() override;

    mfxU32  Lock(mfxU32 idx);
    mfxU32  Unlock(mfxU32 idx);
    mfxU32  Locked(mfxU32 idx) const;

    virtual void Free();

    bool isExternal() { return m_isExternal; };


    mfxFrameInfo m_info = {};

protected:
    MfxFrameAllocResponse(MfxFrameAllocResponse const &) = delete;
    MfxFrameAllocResponse & operator =(MfxFrameAllocResponse const &) = delete;

    VideoCORE& m_core;
    mfxU16 m_numFrameActualReturnedByAllocFrames = 0;

    std::vector<mfxFrameAllocResponse> m_responseQueue;
    std::vector<mfxMemId>              m_mids;
    std::vector<mfxU32>                m_locked;
    std::vector<mfxU32>                m_flag;
    bool m_isExternal = true;
    bool m_isOpaque   = false;
};

class Allocator
    : public FeatureBase
{
public:
#define DECL_BLOCK_LIST\
    DECL_BLOCK(Init)
#define DECL_FEATURE_NAME "G9_Allocator"
#include "hevcehw_decl_blocks.h"

    Allocator(mfxU32 FeatureId)
        : FeatureBase(FeatureId)
    {}

protected:
    virtual void InitAlloc(const FeatureBlocks& blocks, TPushIA Push) override;
};

} //Gen9
} //namespace HEVCEHW

#endif
