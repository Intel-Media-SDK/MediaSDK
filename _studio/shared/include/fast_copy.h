// Copyright (c) 2018-2020 Intel Corporation
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
#include "umc_mutex.h"
#include "fast_copy_c_impl.h"
#include "fast_copy_sse4_impl.h"

enum
{
    COPY_SYS_TO_SYS = 0,
    COPY_SYS_TO_VIDEO = 1,
    COPY_VIDEO_TO_SYS = 2,
};

typedef void(*t_copyVideoToSys)(const mfxU8* src, mfxU8* dst, int width);
typedef void(*t_copyVideoToSysShift)(const mfxU16* src, mfxU16* dst, int width, int shift);
typedef void(*t_copySysToVideoShift)(const mfxU16* src, mfxU16* dst, int width, int shift);

void copyVideoToSys(const mfxU8* src, mfxU8* dst, int width);
void copyVideoToSysShift(const mfxU16* src, mfxU16* dst, int width, int shift);
void copySysToVideoShift(const mfxU16* src, mfxU16* dst, int width, int shift);

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

        /* The purpose of mutex here is to make the Copy() atomic.
         * Without it CPU utilization grows dramatically due to cache trashing.
         */
        static UMC::Mutex mutex; // This is thread-safe since C++11
        UMC::AutomaticUMCMutex guard(mutex);

        mfxCopyRect<mfxU8>(pSrc, srcPitch, pDst, dstPitch, roi, flag);

        return MFX_ERR_NONE;
    }
    static mfxStatus CopyAndShift(mfxU16 *pDst, mfxU32 dstPitch, mfxU16 *pSrc, mfxU32 srcPitch, mfxSize roi, mfxU8 lshift, mfxU8 rshift, int flag)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "FastCopy::Copy");

        if (NULL == pDst || NULL == pSrc)
        {
            return MFX_ERR_NULL_PTR;
        }

        if (flag & COPY_VIDEO_TO_SYS)
        {
            for (int h = 0; h < roi.height; h++)
            {
                copyVideoToSysShift( pSrc, pDst, roi.width, rshift);
                pSrc = (mfxU16 *)((mfxU8*)pSrc + srcPitch);
                pDst = (mfxU16 *)((mfxU8*)pDst + dstPitch);
            }
        }
        else {
            for (int h = 0; h < roi.height; h++)
            {
                copySysToVideoShift( pSrc, pDst, roi.width, lshift);
                pSrc = (mfxU16 *)((mfxU8*)pSrc + srcPitch);
                pDst = (mfxU16 *)((mfxU8*)pDst + dstPitch);
            }
        }
        return MFX_ERR_NONE;
    }
};

#endif // __FAST_COPY_H__
