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

#include "mfx_brc_common.h"
#include "mfx_enc_common.h"
#include <algorithm>
#include <math.h>
#if defined(MFX_ENABLE_VIDEO_BRC_COMMON)

mfxStatus ConvertVideoParam_Brc(const mfxVideoParam *parMFX, UMC::VideoBrcParams *parUMC)
{
    MFX_CHECK_COND(parMFX != NULL);
    MFX_CHECK_COND((parMFX->mfx.FrameInfo.CropX + parMFX->mfx.FrameInfo.CropW) <= parMFX->mfx.FrameInfo.Width);
    MFX_CHECK_COND((parMFX->mfx.FrameInfo.CropY + parMFX->mfx.FrameInfo.CropH) <= parMFX->mfx.FrameInfo.Height);

    switch (parMFX->mfx.RateControlMethod) {
    case MFX_RATECONTROL_CBR:  parUMC->BRCMode = UMC::BRC_CBR; break;
    case MFX_RATECONTROL_AVBR: parUMC->BRCMode = UMC::BRC_AVBR; break;
    default:
    case MFX_RATECONTROL_VBR:  parUMC->BRCMode = UMC::BRC_VBR; break;
    }

    mfxU16 brcParamMultiplier = parMFX->mfx.BRCParamMultiplier ? parMFX->mfx.BRCParamMultiplier : 1;

    parUMC->targetBitrate = parMFX->mfx.TargetKbps * brcParamMultiplier * BRC_BITS_IN_KBIT;

    if (parUMC->BRCMode == UMC::BRC_AVBR)
    {
        parUMC->accuracy = parMFX->mfx.Accuracy;
        parUMC->convergence = parMFX->mfx.Convergence;
        parUMC->HRDBufferSizeBytes = 0;
        parUMC->HRDInitialDelayBytes = 0;
        parUMC->maxBitrate = parMFX->mfx.TargetKbps * brcParamMultiplier * BRC_BITS_IN_KBIT;
    }
    else
    {
        parUMC->maxBitrate = parMFX->mfx.MaxKbps * brcParamMultiplier * BRC_BITS_IN_KBIT;
        parUMC->HRDBufferSizeBytes = parMFX->mfx.BufferSizeInKB * brcParamMultiplier * BRC_BYTES_IN_KBYTE;
        parUMC->HRDInitialDelayBytes = parMFX->mfx.InitialDelayInKB * brcParamMultiplier * BRC_BYTES_IN_KBYTE;
    }

    parUMC->info.clip_info.width = parMFX->mfx.FrameInfo.Width;
    parUMC->info.clip_info.height = parMFX->mfx.FrameInfo.Height;

    parUMC->GOPPicSize = parMFX->mfx.GopPicSize;
    parUMC->GOPRefDist = parMFX->mfx.GopRefDist;

    parUMC->info.framerate = CalculateUMCFramerate(parMFX->mfx.FrameInfo.FrameRateExtN, parMFX->mfx.FrameInfo.FrameRateExtD);
    parUMC->frameRateExtD = parMFX->mfx.FrameInfo.FrameRateExtD;
    parUMC->frameRateExtN = parMFX->mfx.FrameInfo.FrameRateExtN;
    if (parUMC->info.framerate <= 0) {
        parUMC->info.framerate = 30;
        parUMC->frameRateExtD = 1;
        parUMC->frameRateExtN = 30;
    }

    return MFX_ERR_NONE;
}
#endif
