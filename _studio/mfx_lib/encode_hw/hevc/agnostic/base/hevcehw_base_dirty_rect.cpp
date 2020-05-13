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

#include "hevcehw_base_dirty_rect.h"

using namespace HEVCEHW;
using namespace HEVCEHW::Base;

mfxStatus CheckAndFixRect(
    DirtyRect::RectData& rect
    , mfxVideoParam const & par
    , ENCODE_CAPS_HEVC const & caps)
{
    mfxU32 changed = 0;

    changed += CheckMaxOrClip(rect.Left, par.mfx.FrameInfo.Width);
    changed += CheckMaxOrClip(rect.Right, par.mfx.FrameInfo.Width);
    changed += CheckMaxOrClip(rect.Top, par.mfx.FrameInfo.Height);
    changed += CheckMaxOrClip(rect.Bottom, par.mfx.FrameInfo.Height);

    mfxU32 blockSize = 1 << (caps.BlockSize + 3);
    changed += AlignDown(rect.Left, blockSize);
    changed += AlignDown(rect.Top, blockSize);
    changed += AlignUp(rect.Right, blockSize);
    changed += AlignUp(rect.Bottom, blockSize);

    return
        changed
        ? MFX_WRN_INCOMPATIBLE_VIDEO_PARAM
        : MFX_ERR_NONE;
}

mfxStatus CheckAndFixDirtyRect(
    ENCODE_CAPS_HEVC const & caps
    , mfxVideoParam const & par
    , mfxExtDirtyRect& dr)
{
    mfxStatus sts = MFX_ERR_NONE, rsts = MFX_ERR_NONE;
    mfxU32 changed = 0, invalid = 0;

    if (caps.DirtyRectSupport == 0)
    {
        invalid++;
        dr.NumRect = 0;
    }

    changed += CheckMaxOrClip(dr.NumRect, DirtyRect::MAX_NUM_DIRTY_RECT);

    for (mfxU16 i = 0; i < dr.NumRect; i++)
    {
        // check that rectangle dimensions don't conflict with each other and don't exceed frame size
        rsts = CheckAndFixRect(dr.Rect[i], par, caps);
    }

    auto IsInvalidRect = [](const DirtyRect::RectData& rect)
    {
        // Dirty rectangle (0, 0, 0, 0) is a valid dirty rectangle and means that frame is not changed.
        if (rect.Left==0 && rect.Right==0 && rect.Top==0 && rect.Bottom==0) return false;
        return ((rect.Left >= rect.Right) || (rect.Top >= rect.Bottom));
    };
    mfxU16 numValidRect = mfxU16(std::remove_if(dr.Rect, dr.Rect + dr.NumRect, IsInvalidRect) - dr.Rect);
    changed += CheckMaxOrClip(dr.NumRect, numValidRect);

    if (changed)
        sts = MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
    if (invalid)
        sts = MFX_ERR_UNSUPPORTED;

    return GetWorstSts(sts, rsts);
}

void DirtyRect::SetSupported(ParamSupport& blocks)
{
    blocks.m_ebCopySupported[MFX_EXTBUFF_DIRTY_RECTANGLES].emplace_back(
        [](const mfxExtBuffer* pSrc, mfxExtBuffer* pDst) -> void
    {
        auto& src = *(const mfxExtDirtyRect*)pSrc;
        auto& dst = *(mfxExtDirtyRect*)pDst;

        dst.NumRect = src.NumRect;

        for (mfxU32 i = 0; i < Size(src.Rect); ++i)
        {
            dst.Rect[i].Left = src.Rect[i].Left;
            dst.Rect[i].Top = src.Rect[i].Top;
            dst.Rect[i].Right = src.Rect[i].Right;
            dst.Rect[i].Bottom = src.Rect[i].Bottom;
        }
    });
}

void DirtyRect::Query1WithCaps(const FeatureBlocks& /*blocks*/, TPushQ1 Push)
{
    Push(BLK_CheckAndFix
        , [](const mfxVideoParam& /*in*/, mfxVideoParam& par, StorageW& global) -> mfxStatus
    {
        mfxExtDirtyRect* pDR = ExtBuffer::Get(par);

        if (pDR && pDR->NumRect)
            return CheckAndFixDirtyRect(Glob::EncodeCaps::Get(global), par, *pDR);

        return MFX_ERR_NONE;
    });
}

void DirtyRect::AllocTask(const FeatureBlocks& /*blocks*/, TPushAT Push)
{
    Push(BLK_AllocTask
        , [this](
            StorageR& /*global*/
            , StorageW& task) -> mfxStatus
    {
        m_taskToRects.emplace(&task, std::vector<RectData>(MAX_NUM_DIRTY_RECT));
        return MFX_ERR_NONE;
    });
}

void DirtyRect::PostReorderTask(const FeatureBlocks& /*blocks*/, TPushPostRT Push)
{
    Push(BLK_ConfigureTask
        , [this](
            StorageW& global
            , StorageW& s_task) -> mfxStatus
    {
        auto& par = Glob::VideoParam::Get(global);
        mfxExtDirtyRect tmp;
        mfxExtDirtyRect* pDR = ExtBuffer::Get(Task::Common::Get(s_task).ctrl);

        if (pDR)
        {
            tmp = *pDR;

            if (CheckAndFixDirtyRect(Glob::EncodeCaps::Get(global), par, tmp) < MFX_ERR_NONE)
                pDR = nullptr;
            else
                pDR = &tmp;
        }

        if (!pDR)
            pDR = ExtBuffer::Get(par);

        auto& rects = m_taskToRects[&s_task];

        if (pDR)
            rects.assign(pDR->Rect, pDR->Rect + pDR->NumRect);
        else
            rects.clear();

        return MFX_ERR_NONE;
    });
}

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
