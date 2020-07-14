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

#include "mfx_common.h"
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE) && defined(MFX_VA_LINUX)

#include "hevcehw_base_dirty_rect_lin.h"
#include "hevcehw_base_va_packer_lin.h"

using namespace HEVCEHW;
using namespace HEVCEHW::Base;

void Linux::Base::DirtyRect::InitAlloc(const FeatureBlocks& /*blocks*/, TPushIA Push)
{
    Push(BLK_SetCallChains
        , [this](StorageRW& global, StorageRW& /*local*/) -> mfxStatus
    {
        auto& vaPacker = VAPacker::CC::Get(global);

        vaPacker.AddPerPicMiscData[VAEncMiscParameterTypeDirtyRect].Push([this](
            VAPacker::CallChains::TAddMiscData::TExt
            , const StorageR& global
            , const StorageR& s_task
            , std::list<std::vector<mfxU8>>& data)
        {
            auto dirtyRect = GetRTExtBuffer<mfxExtDirtyRect>(global, s_task);
            auto& vaDirtyRect = AddVaMisc<VAEncMiscParameterBufferDirtyRect>(VAEncMiscParameterTypeDirtyRect, data);

            vaDirtyRect.num_roi_rectangle = 0;
            m_vaDirtyRects.clear();

            for (int i = 0; i < dirtyRect.NumRect; i++)
            {
                if (SkipRectangle(dirtyRect.Rect[i]))
                    continue;

                /* Driver expects width x height rect and
                MSDK uses the 'open' edge rect, so width = right - left.
                Borders will be further aligned in driver.
                We expect here Right > Left and Bottom > Top */
                m_vaDirtyRects.push_back({
                   int16_t(dirtyRect.Rect[i].Left)
                   , int16_t(dirtyRect.Rect[i].Top)
                   , uint16_t((dirtyRect.Rect[i].Right - dirtyRect.Rect[i].Left))
                   , uint16_t((dirtyRect.Rect[i].Bottom - dirtyRect.Rect[i].Top))
                });

            }
            vaDirtyRect.roi_rectangle = m_vaDirtyRects.empty() ? nullptr : m_vaDirtyRects.data();
            vaDirtyRect.num_roi_rectangle = m_vaDirtyRects.size();

            return vaDirtyRect.num_roi_rectangle ? 1 : 0;
        });

        return MFX_ERR_NONE;
    });
}

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
