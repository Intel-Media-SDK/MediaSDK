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
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE)


namespace HEVCEHW
{
namespace Base
{
    mfxU16 GetMaxDpbSizeByLevel(mfxVideoParam const & par, mfxU32 PicSizeInSamplesY);
    mfxU32 GetMaxKbpsByLevel(mfxVideoParam const & par);
    mfxF64 GetMaxFrByLevel(mfxVideoParam const & par, mfxU32 PicSizeInSamplesY);
    mfxU32 GetMaxCpbInKBByLevel(mfxVideoParam const & par);

    mfxU16 GetMinLevel(
        mfxU32 frN
        , mfxU32 frD
        , mfxU16 PicWidthInLumaSamples
        , mfxU16 PicHeightInLumaSamples
        , mfxU16 MinRef
        , mfxU16 NumTileColumns
        , mfxU16 NumTileRows
        , mfxU32 NumSlice
        , mfxU32 BufferSizeInKB
        , mfxU32 MaxKbps
        , mfxU16 StartLevel);

} //Base
} //namespace HEVCEHW

#endif
