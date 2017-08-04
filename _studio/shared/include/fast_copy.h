// Copyright (c) 2017 Intel Corporation
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

#ifndef __FAST_COPY_H__
#define __FAST_COPY_H__

#include "umc_defs.h"
#include "mfx_trace.h"
#include "mfxdefs.h"
#include <algorithm>

enum
{
    COPY_SYS_TO_SYS = 0,
    COPY_SYS_TO_VIDEO = 1,
    COPY_VIDEO_TO_SYS = 2,
};

#include <immintrin.h>
inline void copyVideoToSys(const mfxU8* src, mfxU8* dst, int width)
{
    static const int item_size = 4*sizeof(__m128i);

    int align16 = (0x10 - (reinterpret_cast<size_t>(src) & 0xf)) & 0xf;
    for (int i = 0; i < align16; i++)
        *dst++ = *src++;

    int w = width - align16;
    if (w < 0)
        return;

    int width4 = w & (-item_size);

    __m128i * src_reg = (__m128i *)src;
    __m128i * dst_reg = (__m128i *)dst;

    int i = 0;
    for (; i < width4; i += item_size)
    {
        __m128i xmm0 = _mm_stream_load_si128(src_reg);
        __m128i xmm1 = _mm_stream_load_si128(src_reg+1);
        __m128i xmm2 = _mm_stream_load_si128(src_reg+2);
        __m128i xmm3 = _mm_stream_load_si128(src_reg+3);
        _mm_storeu_si128(dst_reg, xmm0);
        _mm_storeu_si128(dst_reg+1, xmm1);
        _mm_storeu_si128(dst_reg+2, xmm2);
        _mm_storeu_si128(dst_reg+3, xmm3);

        src_reg += 4;
        dst_reg += 4;
    }

    size_t tail_data_sz = w & (item_size - 1);
    if (tail_data_sz)
    {
        for (; tail_data_sz >= sizeof(__m128i); tail_data_sz -= sizeof(__m128i))
        {
            __m128i xmm0 = _mm_stream_load_si128(src_reg);
            _mm_storeu_si128(dst_reg, xmm0);
            src_reg += 1;
            dst_reg += 1;
        }

        src = (const mfxU8 *)src_reg;
        dst = (mfxU8 *)dst_reg;

        for (; tail_data_sz > 0; tail_data_sz--)
            *dst++ = *src++;
    }
}

template<typename T>
inline int mfxCopyRect(const T* pSrc, int srcStep, T* pDst, int dstStep, mfxSize roiSize, int flag)
{
    if (!pDst || !pSrc || roiSize.width < 0 || roiSize.height < 0 || srcStep < 0 || dstStep < 0)
        return -1;

    if (flag & COPY_VIDEO_TO_SYS)
    {
        for(int h = 0; h < roiSize.height; h++ )
        {
            copyVideoToSys((const mfxU8*)pSrc, (mfxU8*)pDst, roiSize.width*sizeof(T));
            pSrc = (T *)((mfxU8*)pSrc + srcStep);
            pDst = (T *)((mfxU8*)pDst + dstStep);
        }
    } else {
        for(int h = 0; h < roiSize.height; h++ )
        {
            std::copy(pSrc, pSrc + roiSize.width, pDst);
            pSrc = (T *)((mfxU8*)pSrc + srcStep);
            pDst = (T *)((mfxU8*)pDst + dstStep);
        }
    }

    return 0;
}


class FastCopy
{
public:
    // copy memory by streaming
    static mfxStatus Copy(mfxU8 *pDst, mfxU32 dstPitch, mfxU8 *pSrc, mfxU32 srcPitch, mfxSize roi, int flag)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "FastCopy::Copy");

        if (NULL == pDst || NULL == pSrc)
        {
            return MFX_ERR_NULL_PTR;
        }

        mfxCopyRect<mfxU8>(pSrc, srcPitch, pDst, dstPitch, roi, flag);

        return MFX_ERR_NONE;
    }
};

#endif // __FAST_COPY_H__
