// Copyright (c) 2019-2020 Intel Corporation
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
#include "hevcehw_base.h"

#if defined(MFX_ENABLE_H265_VIDEO_ENCODE) && defined (MFX_VA_LINUX)

#include "hevcehw_base_data.h"
#include "hevcehw_base_iddi_packer.h"
#include "ehw_utils_vaapi.h"
#include "va/va.h"

namespace HEVCEHW
{
namespace Linux
{
namespace Base
{
using namespace HEVCEHW::Base;
using namespace MfxEncodeHW;

class CUQPMap
{
public:
    mfxU32                            m_width;
    mfxU32                            m_height;
    mfxU32                            m_pitch;
    mfxU32                            m_h_aligned;

    mfxU32                            m_block_width;
    mfxU32                            m_block_height;
    std::vector<mfxI8>                m_buffer;

    CUQPMap() :
        m_width(0),
        m_height(0),
        m_pitch(0),
        m_h_aligned(0),
        m_block_width(0),
        m_block_height(0) {}

    void Init(mfxU32 picWidthInLumaSamples, mfxU32 picHeightInLumaSamples, mfxU32 blockSize);
};

class VAPacker
    : public IDDIPacker
    , protected VAAPIParPacker
{
public:
    VAPacker(mfxU32 FeatureId)
        : FeatureBase(FeatureId)
        , IDDIPacker(FeatureId)
    {
        SetTraceName("Base_VAPacker");
    }

    struct CallChains
        : Storable
    {
        using TReadFeedback = CallChain<mfxStatus
            , const StorageR& //glob
            , StorageW& //task
            , const VACodedBufferSegment&>;
        TReadFeedback ReadFeedback;

        using TInitSPS = CallChain<void
            , const StorageR& //glob
            , VAEncSequenceParameterBufferHEVC&>;
        TInitSPS InitSPS;

        using TUpdatePPS = CallChain<void
            , const StorageR& //glob
            , const StorageR& //task
            , const VAEncSequenceParameterBufferHEVC&
            , VAEncPictureParameterBufferHEVC&>;
        TUpdatePPS UpdatePPS;

        using TInitPriority = CallChain<void
            , const mfxU32&
            , const mfxPriority&
            , VAContextParameterUpdateBuffer&>;
        TInitPriority InitPriority;

        using TFillCUQPData = CallChain<bool
            , const StorageR& //glob
            , const StorageR& //task
            , CUQPMap&>;
        TFillCUQPData FillCUQPData;

        using TAddMiscData = CallChain<bool
            , const StorageR& //glob
            , const StorageR& //task
            , std::list<std::vector<mfxU8>>&>;
        std::map<VAEncMiscParameterType, TAddMiscData> AddPerPicMiscData;
        std::map<VAEncMiscParameterType, TAddMiscData> AddPerSeqMiscData;
    };

    using CC = StorageVar<Base::Glob::ReservedKey0, CallChains>;

protected:
    virtual void Query1WithCaps(const FeatureBlocks& blocks, TPushQ1 Push) override;
    virtual void InitAlloc(const FeatureBlocks& blocks, TPushIA Push) override;
    virtual void InitInternal(const FeatureBlocks& blocks, TPushII Push) override;
    virtual void SubmitTask(const FeatureBlocks& blocks, TPushST Push) override;
    virtual void QueryTask(const FeatureBlocks& blocks, TPushQT Push) override;
    virtual void ResetState(const FeatureBlocks& blocks, TPushRS Push) override;

    VAEncSequenceParameterBufferHEVC            m_sps;
    VAEncPictureParameterBufferHEVC             m_pps;
    VAContextParameterUpdateBuffer              m_hevcPriorityBuf;
    std::vector<VAEncSliceParameterBufferHEVC>  m_slices;
    CUQPMap                                     m_qpMap;
    mfxU32                                      m_numSkipFrames = 0;
    mfxU32                                      m_sizeSkipFrames = 0;
    mfxU32                                      m_resetHintFlags = 0;
    std::list<std::vector<mfxU8>>               m_vaPerSeqMiscData;
    std::list<std::vector<mfxU8>>               m_vaPerPicMiscData;
};

} //Base
} //Linux
} //namespace HEVCEHW

#endif
