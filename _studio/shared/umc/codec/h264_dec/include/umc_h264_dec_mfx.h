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

#ifndef __UMC_H264_DEC_MFX_H
#define __UMC_H264_DEC_MFX_H

#include "umc_defs.h"
#if defined (MFX_ENABLE_H264_VIDEO_DECODE)

#include "umc_h264_dec_defs_dec.h"
#include "mfxdefs.h"
#include "mfxstructures.h"

namespace UMC
{

inline mfxU8 ConvertH264FrameTypeToMFX(int32_t slice_type)
{
    mfxU8 mfx_type;
    switch(slice_type)
    {
    case UMC::INTRASLICE:
        mfx_type = MFX_FRAMETYPE_I;
        break;
    case UMC::PREDSLICE:
        mfx_type = MFX_FRAMETYPE_P;
        break;
    case UMC::BPREDSLICE:
        mfx_type = MFX_FRAMETYPE_B;
        break;
    case UMC::S_PREDSLICE:
        mfx_type = MFX_FRAMETYPE_P;//MFX_SLICETYPE_SP;
        break;
    case UMC::S_INTRASLICE:
        mfx_type = MFX_FRAMETYPE_I;//MFX_SLICETYPE_SI;
        break;
    default:
        VM_ASSERT(false);
        mfx_type = MFX_FRAMETYPE_I;
    }

    return mfx_type;
}

Status FillVideoParam(const UMC_H264_DECODER::H264SeqParamSet * seq, mfxVideoParam *par, bool full);
Status FillVideoParamExtension(const UMC_H264_DECODER::H264SeqParamSetMVCExtension * seqEx, mfxVideoParam *par);

} // namespace UMC

#endif // MFX_ENABLE_H264_VIDEO_DECODE

#endif // __UMC_H264_DEC_MFX_H
