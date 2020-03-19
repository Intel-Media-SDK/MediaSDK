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

#pragma once
#include "libmfx_core_interface.h"
#include "feature_blocks/mfx_feature_blocks_utils.h"
#include <vector>

namespace MfxEncodeHW
{
class ResPool
    : public MfxFeatureBlocks::Storable
{
public:
    static constexpr mfxU8 IDX_INVALID = 0xff;

    struct Resource
    {
        mfxU8    Idx = IDX_INVALID;
        mfxMemId Mid = nullptr;
    };

    ResPool(VideoCORE& core);

    ~ResPool();

    virtual mfxStatus Alloc(
         const mfxFrameAllocRequest & req
        , bool isCopyRequired);

    mfxStatus AllocOpaque(
        const mfxFrameInfo & info
        , mfxU16 type
        , mfxFrameSurface1 **surfaces
        , mfxU16 numSurface);

    mfxFrameAllocResponse GetResponse()   const { return m_response; }
    mfxFrameInfo          GetInfo()       const { return m_info; }
    Resource              Acquire();
    void                  Release(mfxU32 idx) { Unlock(idx); }
    void                  ClearFlag(mfxU32 idx);
    void                  SetFlag(mfxU32 idx, mfxU32 flag);
    mfxU32                GetFlag(mfxU32 idx);
    void                  UnlockAll();
    mfxU32                Lock(mfxU32 idx);
    mfxU32                Unlock(mfxU32 idx);
    mfxU32                Locked(mfxU32 idx) const;

    virtual void Free();

    bool IsExternal() { return m_bExternal; };

protected:
    ResPool(ResPool const &) = delete;
    ResPool & operator =(ResPool const &) = delete;

    VideoCORE& m_core;

    std::vector<mfxFrameAllocResponse> m_responseQueue;
    std::vector<mfxMemId>              m_mids;
    std::vector<mfxU32>                m_locked;
    std::vector<mfxU32>                m_flag;
    mfxFrameInfo                       m_info           = {};
    mfxFrameAllocResponse              m_response       = {};
    bool                               m_bExternal      = true;
    bool                               m_bOpaque        = false;
    mfxU16                             m_numFrameActual = 0;
};

} //namespace MfxEHW