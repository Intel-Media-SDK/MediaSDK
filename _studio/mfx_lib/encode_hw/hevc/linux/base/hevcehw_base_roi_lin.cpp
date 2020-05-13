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
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE) && defined(MFX_ENABLE_HEVCE_ROI) && defined(MFX_VA_LINUX)

#include "hevcehw_base_roi_lin.h"
#include "hevcehw_base_va_packer_lin.h"

using namespace HEVCEHW;
using namespace HEVCEHW::Base;

void Linux::Base::ROI::InitAlloc(const FeatureBlocks& /*blocks*/, TPushIA Push)
{
    Push(BLK_SetCallChains
        , [this](StorageRW& global, StorageRW& /*local*/) -> mfxStatus
    {
        auto& vaPacker = VAPacker::CC::Get(global);

        if (m_bViaCuQp)
        {
            vaPacker.FillCUQPData.Push([this](
                VAPacker::CallChains::TFillCUQPData::TExt prev
                , const StorageR& global
                , const StorageR& s_task
                , CUQPMap& qpMap)
            {
                bool  bRes = prev(global, s_task, qpMap);
                auto& task = Task::Common::Get(s_task);
                auto roi = GetRTExtBuffer<mfxExtEncoderROI>(global, s_task);

                if (roi.NumROI)
                {
                    auto& caps = Glob::EncodeCaps::Get(global);
                    auto& par = Glob::VideoParam::Get(global);
                    CheckAndFixROI(caps, par, roi);
                }

                bool bFillQPMap =
                    !bRes
                    && roi.NumROI
                    && qpMap.m_width
                    && qpMap.m_height
                    && qpMap.m_block_width
                    && qpMap.m_block_height;

                if (bFillQPMap)
                {
                    mfxU32 x = 0, y = 0;
                    auto IsXYInRect = [&](const RectData& r)
                    {
                        return (x * qpMap.m_block_width) >= r.Left
                            && (x * qpMap.m_block_width) < r.Right
                            && (y * qpMap.m_block_height) >= r.Top
                            && (y * qpMap.m_block_height) < r.Bottom;
                    };
                    auto NextQP = [&]()
                    {
                        auto pEnd = roi.ROI + roi.NumROI;
                        auto pRect = std::find_if(roi.ROI, pEnd, IsXYInRect);

                        ++x;

                        if (pRect != pEnd)
                            return mfxI8(task.QpY + pRect->DeltaQP);

                        return mfxI8(task.QpY);
                    };
                    auto FillQpMapRow = [&](mfxI8& qpMapRow)
                    {
                        x = 0;
                        std::generate_n(&qpMapRow, qpMap.m_width, NextQP);
                        ++y;
                    };
                    auto itQpMapRowsBegin = MakeStepIter(qpMap.m_buffer.data(), qpMap.m_pitch);
                    auto itQpMapRowsEnd = MakeStepIter(qpMap.m_buffer.data() + qpMap.m_height * qpMap.m_pitch);

                    std::for_each(itQpMapRowsBegin, itQpMapRowsEnd, FillQpMapRow);

                    bRes = true;
                }

                return bRes;
            });
        }
        else
        {
            vaPacker.AddPerPicMiscData[VAEncMiscParameterTypeROI].Push([this](
                VAPacker::CallChains::TAddMiscData::TExt
                , const StorageR& global
                , const StorageR& s_task
                , std::list<std::vector<mfxU8>>& data)
            {
                auto roi = GetRTExtBuffer<mfxExtEncoderROI>(global, s_task);
                bool bNeedROI = !!roi.NumROI;

                if (bNeedROI)
                {
                    auto& caps = Glob::EncodeCaps::Get(global);
                    auto& par = Glob::VideoParam::Get(global);
                    CheckAndFixROI(caps, par, roi);
                    bNeedROI = !!roi.NumROI;
                }

                if (bNeedROI)
                {
                    auto& vaROI = AddVaMisc<VAEncMiscParameterBufferROI>(VAEncMiscParameterTypeROI, data);

                    auto   MakeVAEncROI = [](const RectData& rect)
                    {
                        VAEncROI roi = {};
                        roi.roi_rectangle.x      = rect.Left;
                        roi.roi_rectangle.y      = rect.Top;
                        roi.roi_rectangle.width  = rect.Right - rect.Left;
                        roi.roi_rectangle.height = rect.Bottom - rect.Top;
                        roi.roi_value            = int8_t(rect.DeltaQP);
                        return roi;
                    };

                    m_vaROI.resize(roi.NumROI);

                    vaROI.num_roi = roi.NumROI;
                    vaROI.roi     = m_vaROI.data();

                    std::transform(roi.ROI, roi.ROI + roi.NumROI, vaROI.roi, MakeVAEncROI);

                    vaROI.max_delta_qp = 51;
                    vaROI.min_delta_qp = -51;
                    vaROI.roi_flags.bits.roi_value_is_qp_delta = 1;
                }

                return bNeedROI;
            });
        }

        return MFX_ERR_NONE;
    });
}

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
