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
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE) && (MFX_VERSION >= MFX_VERSION_NEXT)

#include "hevcehw_base_dpb_report.h"

using namespace HEVCEHW;
using namespace HEVCEHW::Base;

void DPBReport::SetSupported(ParamSupport& blocks)
{
    blocks.m_ebCopySupported[MFX_EXTBUFF_DPB].emplace_back(
        [](const mfxExtBuffer* pSrc, mfxExtBuffer* pDst) -> void
    {
        auto& src = *(const mfxExtDPB*)pSrc;
        auto& dst = *(mfxExtDPB*)pDst;

        dst.DPBSize = src.DPBSize;

        for (mfxU32 i = 0; i < Size(src.DPB); ++i)
        {
            dst.DPB[i].FrameOrder = src.DPB[i].FrameOrder;
            dst.DPB[i].LongTermIdx = src.DPB[i].LongTermIdx;
            dst.DPB[i].PicType = src.DPB[i].PicType;
        }
    });
}

void DPBReport::QueryTask(const FeatureBlocks& /*blocks*/, TPushQT Push)
{
    Push(BLK_Report
        , [](StorageW& /*global*/, StorageW& s_task) -> mfxStatus
    {
        auto& task = Task::Common::Get(s_task);
        MFX_CHECK(task.pBsOut, MFX_ERR_NONE);

        mfxExtDPB* pDPBReport = ExtBuffer::Get(*task.pBsOut);
        MFX_CHECK(pDPBReport, MFX_ERR_NONE);

        auto& report = *pDPBReport;
        const auto& DPB = task.DPB.After;
        auto& i = report.DPBSize;

        for (i = 0; !isDpbEnd(DPB, i); ++i)
        {
            report.DPB[i].FrameOrder = DPB[i].DisplayOrder;
            report.DPB[i].LongTermIdx = DPB[i].isLTR ? 0 : MFX_LONGTERM_IDX_NO_IDX;
            report.DPB[i].PicType = MFX_PICTYPE_FRAME;
        }

        return MFX_ERR_NONE;
    });
}

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
