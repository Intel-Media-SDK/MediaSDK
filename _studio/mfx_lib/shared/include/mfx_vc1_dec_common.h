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

#include "mfx_common.h"


#if defined (MFX_ENABLE_VC1_VIDEO_DECODE)

#ifndef _MFX_VC1_DEC_COMMON_H_
#define _MFX_VC1_DEC_COMMON_H_

#include "mfxvideo.h"
#include "umc_video_decoder.h"
#include "umc_vc1_common_defs.h"

namespace MFXVC1DecCommon
{
    // need for dword alignment memory
    inline mfxU32    GetDWord_s(mfxU8* pData) { return ((*(pData+3))<<24) + ((*(pData+2))<<16) + ((*(pData+1))<<8) + *(pData);}
    
    // we sure about our types
    template <class T, class T1>
    static T bit_set(T value, uint32_t offset, uint32_t count, T1 in)
    {
        return (T)(value | (((1<<count) - 1)&in) << offset);
    };

    mfxStatus ConvertMfxToCodecParams(mfxVideoParam *par, UMC::BaseCodecParams* pVideoParams);
    mfxStatus ParseSeqHeader(mfxBitstream *bs, mfxVideoParam *par, mfxExtVideoSignalInfo *pVideoSignal, mfxExtCodingOptionSPSPPS *pSPS);
    mfxStatus PrepareSeqHeader(mfxBitstream *bs_in, mfxBitstream *bs_out);

    mfxStatus Query(VideoCORE* core, mfxVideoParam *in, mfxVideoParam *out);
}

#endif
#endif
