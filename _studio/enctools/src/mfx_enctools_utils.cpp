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

#include <cstring>
#include <assert.h>
#include "mfx_enctools_utils.h"

namespace EncToolsUtils
{

    template <typename T> mfxStatus DownScaleNN(T const & pSrc, mfxU32 srcWidth, mfxU32 srcHeight, mfxU32 srcPitch,
        T & pDst, mfxU32 dstWidth, mfxU32 dstHeight, mfxU32 dstPitch)
    {
        mfxI32 step_w = srcWidth / dstWidth;
        mfxI32 step_h = srcHeight / dstHeight;
        for (mfxI32 y = 0; y < (mfxI32)dstHeight; y++) {
            for (mfxI32 x = 0; x < (mfxI32)dstWidth; x++) {
                T const * ps = &pSrc + ((y * step_h) * srcPitch) + (x * step_w);
                T* pd = &pDst + (y * dstPitch) + x;
                pd[0] = ps[0];
            }
        }
        return MFX_ERR_NONE;
    }

    template mfxStatus DownScaleNN(mfxU8 const & pSrc, mfxU32 srcWidth, mfxU32 srcHeight, mfxU32 srcPitch,
        mfxU8 & pDst, mfxU32 dstWidth, mfxU32 dstHeight, mfxU32 dstPitch);

}
