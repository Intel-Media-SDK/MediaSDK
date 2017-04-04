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

#ifndef __MFX_COMMON_DECODE_INT_H__
#define __MFX_COMMON_DECODE_INT_H__

#include <vector>
#include "mfx_config.h"
#include "mfx_common_int.h"
#include "umc_video_decoder.h"

class MFXMediaDataAdapter : public UMC::MediaData
{
public:
    MFXMediaDataAdapter(mfxBitstream *pBitstream = 0);

    void Load(mfxBitstream *pBitstream);
    void Save(mfxBitstream *pBitstream);
};

mfxStatus ConvertUMCStatusToMfx(UMC::Status status);

void ConvertMFXParamsToUMC(mfxVideoParam *par, UMC::VideoDecoderParams *umcVideoParams);
void ConvertMFXParamsToUMC(mfxVideoParam *par, UMC::VideoStreamInfo *umcVideoParams);

bool IsNeedChangeVideoParam(mfxVideoParam *par);

inline mfxU32 ExtractProfile(mfxU32 profile)
{
    return profile & 0xFF;
}

inline bool IsMVCProfile(mfxU32 profile)
{
    profile = ExtractProfile(profile);
    return (profile == MFX_PROFILE_AVC_MULTIVIEW_HIGH || profile == MFX_PROFILE_AVC_STEREO_HIGH);
}



#endif
