/*//////////////////////////////////////////////////////////////////////////////
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
*/
#include "fast_copy_sse4_impl.h"

#if defined(__SSE4_1__) || defined(_WIN32)

#include <immintrin.h>

void copyVideoToSys_SSE4(const mfxU8* src, mfxU8* dst, int width)
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

void copyVideoToSysShift_SSE4(const mfxU16* src, mfxU16* dst, int width, int shift)
{
    static const int item_size = 4 * sizeof(__m128i);

    int align16 = (0x10 - (reinterpret_cast<size_t>((mfxU8*)src) & 0xf)) & 0xf;
    for (int i = 0; i < align16/2; i++)
        *dst++ = (*src++)>>shift;

    int w = width*2 - align16;
    if (w < 0)
        return;

    int width4 = w & (-item_size);

    __m128i * src_reg = (__m128i *)src;
    __m128i * dst_reg = (__m128i *)dst;

    int i = 0;
    for (; i < width4; i += item_size)
    {
        __m128i xmm0 = _mm_stream_load_si128(src_reg);
        __m128i xmm1 = _mm_stream_load_si128(src_reg + 1);
        __m128i xmm2 = _mm_stream_load_si128(src_reg + 2);
        __m128i xmm3 = _mm_stream_load_si128(src_reg + 3);
        __m128i xmm4 = _mm_srli_epi16(xmm0, shift);
        __m128i xmm5 = _mm_srli_epi16(xmm1, shift);
        __m128i xmm6 = _mm_srli_epi16(xmm2, shift);
        __m128i xmm7 = _mm_srli_epi16(xmm3, shift);
        _mm_storeu_si128(dst_reg, xmm4);
        _mm_storeu_si128(dst_reg + 1, xmm5);
        _mm_storeu_si128(dst_reg + 2, xmm6);
        _mm_storeu_si128(dst_reg + 3, xmm7);

        src_reg += 4;
        dst_reg += 4;
    }

    size_t tail_data_sz = w & (item_size - 1);
    if (tail_data_sz)
    {
        for (; tail_data_sz >= sizeof(__m128i); tail_data_sz -= sizeof(__m128i))
        {
            __m128i xmm0 = _mm_stream_load_si128(src_reg);
            __m128i xmm1 = _mm_srli_epi16(xmm0, shift);
            _mm_storeu_si128(dst_reg, xmm1);
            src_reg += 1;
            dst_reg += 1;
        }

        src = (const mfxU16 *)src_reg;
        dst = (mfxU16 *)dst_reg;

        for (; tail_data_sz > 0; tail_data_sz--)
            *dst++ = (*src++)>>shift;
    }
}

void copySysToVideoShift_SSE4(const mfxU16* src, mfxU16* dst, int width, int shift)
{
    static const int item_size = 4 * sizeof(__m128i);

    int align16 = (0x10 - (reinterpret_cast<size_t>((mfxU8*)src) & 0xf)) & 0xf;
    for (int i = 0; i < align16; i++)
        *dst++ = (*src++)<< shift;

    int w = width*2 - align16;
    if (w < 0)
        return;

    int width4 = w & (-item_size);

    __m128i * src_reg = (__m128i *)src;
    __m128i * dst_reg = (__m128i *)dst;

    int i = 0;
    for (; i < width4; i += item_size)
    {
        __m128i xmm0 = _mm_load_si128(src_reg);
        __m128i xmm1 = _mm_load_si128(src_reg + 1);
        __m128i xmm2 = _mm_load_si128(src_reg + 2);
        __m128i xmm3 = _mm_load_si128(src_reg + 3);
        __m128i xmm4 = _mm_slli_epi32(xmm0, shift);
        __m128i xmm5 = _mm_slli_epi32(xmm1, shift);
        __m128i xmm6 = _mm_slli_epi32(xmm2, shift);
        __m128i xmm7 = _mm_slli_epi32(xmm3, shift);
        _mm_storeu_si128(dst_reg, xmm4);
        _mm_storeu_si128(dst_reg + 1, xmm5);
        _mm_storeu_si128(dst_reg + 2, xmm6);
        _mm_storeu_si128(dst_reg + 3, xmm7);

        src_reg += 4;
        dst_reg += 4;
    }

    size_t tail_data_sz = w & (item_size - 1);
    if (tail_data_sz)
    {
        for (; tail_data_sz >= sizeof(__m128i); tail_data_sz -= sizeof(__m128i))
        {
            __m128i xmm0 = _mm_load_si128(src_reg);
            __m128i xmm1 = _mm_slli_epi32(xmm0, shift);
            _mm_storeu_si128(dst_reg, xmm1);
            src_reg += 1;
            dst_reg += 1;
        }

        src = (const mfxU16 *)src_reg;
        dst = (mfxU16 *)dst_reg;

        for (; tail_data_sz > 0; tail_data_sz--)
            *dst++ = (*src++)<<shift;
    }
}

#endif // __SSE4_1__ || _WIN32
