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

#include "mfx_common.h"
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE)

#include "hevcehw_base_encoded_frame_info.h"

using namespace HEVCEHW;
using namespace HEVCEHW::Base;

void EncodedFrameInfo::SetSupported(ParamSupport& blocks)
{

    blocks.m_ebCopySupported[MFX_EXTBUFF_CODING_OPTION2].emplace_back(
        [](const mfxExtBuffer* pSrc, mfxExtBuffer* pDst) -> void
    {
        auto& src = *(const mfxExtCodingOption2*)pSrc;
        auto& dst = *(mfxExtCodingOption2*)pDst;
        dst.EnableMAD = src.EnableMAD;
    });
}

void EncodedFrameInfo::Query1WithCaps(const FeatureBlocks& /*blocks*/, TPushQ1 Push)
{
    Push(BLK_CheckMAD
        , [](const mfxVideoParam& /*in*/, mfxVideoParam& par, StorageW& /*global*/) -> mfxStatus
    {
        mfxExtCodingOption2* pCO2 = ExtBuffer::Get(par);
        MFX_CHECK(pCO2, MFX_ERR_NONE);

        //not supported in driver
        bool bChanged = CheckOrZero(
            pCO2->EnableMAD
            , (mfxU16)MFX_CODINGOPTION_UNKNOWN
            , (mfxU16)MFX_CODINGOPTION_OFF);

        MFX_CHECK(!bChanged, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);

        return MFX_ERR_NONE;
    });
}

void EncodedFrameInfo::QueryTask(const FeatureBlocks& /*blocks*/, TPushQT Push)
{
    Push(BLK_QueryTask
        , [this](
            StorageW& global
            , StorageW& s_task) -> mfxStatus
    {
        auto& task = Task::Common::Get(s_task);
        MFX_CHECK(task.pBsOut, MFX_ERR_UNDEFINED_BEHAVIOR);

        mfxExtAVCEncodedFrameInfo* pInfo = ExtBuffer::Get(*task.pBsOut);
        MFX_CHECK(pInfo, MFX_ERR_NONE);

        using TUsedRef = std::remove_reference<decltype(pInfo->UsedRefListL0[0])>::type;

        TUsedRef unused = {};
        unused.FrameOrder  = mfxU32(MFX_FRAMEORDER_UNKNOWN);
        unused.LongTermIdx = mfxU16(MFX_LONGTERM_IDX_NO_IDX);
        unused.PicStruct   = mfxU16(MFX_PICSTRUCT_UNKNOWN);

        std::fill(std::begin(pInfo->UsedRefListL0), std::end(pInfo->UsedRefListL0), unused);
        std::fill(std::begin(pInfo->UsedRefListL1), std::end(pInfo->UsedRefListL1), unused);

        auto picStruct = Glob::VideoParam::Get(global).mfx.FrameInfo.PicStruct;
        bool bFields = !!(picStruct & (MFX_PICSTRUCT_FIELD_TOP | MFX_PICSTRUCT_FIELD_BOTTOM));
        const mfxU16 PicStructTable[2][2] =
        {
            {mfxU16(MFX_PICSTRUCT_PROGRESSIVE), mfxU16(MFX_PICSTRUCT_PROGRESSIVE)},
            {mfxU16(MFX_PICSTRUCT_FIELD_TOP),   mfxU16(MFX_PICSTRUCT_FIELD_BOTTOM)}
        };

        auto GetUsedRef = [&](mfxU8 idx)
        {
            TUsedRef dst = {};
            auto& src = task.DPB.Active[idx % 16];

            dst.FrameOrder  = src.DisplayOrder;
            dst.LongTermIdx = mfxU16(MFX_LONGTERM_IDX_NO_IDX * !src.isLTR);
            dst.PicStruct   = PicStructTable[bFields][src.bBottomField];

            return dst;
        };

        std::transform(task.RefPicList[0], task.RefPicList[0] + task.NumRefActive[0], pInfo->UsedRefListL0, GetUsedRef);
        std::transform(task.RefPicList[1], task.RefPicList[1] + task.NumRefActive[1], pInfo->UsedRefListL1, GetUsedRef);

        pInfo->FrameOrder   = (task.pSurfIn->Data.FrameOrder == mfxU32(-1)) ? task.DisplayOrder : task.pSurfIn->Data.FrameOrder;
        pInfo->LongTermIdx  = mfxU16(MFX_LONGTERM_IDX_NO_IDX * !task.isLTR);
        pInfo->PicStruct    = PicStructTable[bFields][task.bBottomField];
        pInfo->QP           = 0;
        pInfo->BRCPanicMode = 0;
        pInfo->MAD          = 0;

        return GetDdiInfo(Glob::DDI_Feedback::Get(global).Get(task.StatusReportId), *pInfo);
    });
}

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
