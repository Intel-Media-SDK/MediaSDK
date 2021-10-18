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

#include "mfx_vpx_dec_common.h"

#include "mfx_common_decode_int.h"
#include "mfx_enc_common.h"

#include "umc_defs.h"

namespace MFX_VPX_Utility

{
    inline mfxU32 GetMaxWidth(mfxU32 codecId, eMFXHWType hwType)
    {
        switch (codecId)
        {
        case MFX_CODEC_VP8:
            return 4096;
        case MFX_CODEC_VP9:
            if (hwType < MFX_HW_KBL)
                return 4096;
            return 16384;
#if defined(MFX_ENABLE_AV1_VIDEO_DECODE)
        case MFX_CODEC_AV1:
            return 16384;
#endif
        default: return 0;
        }
    }

    inline mfxU32 GetMaxHeight(mfxU32 codecId, eMFXHWType hwType)
    {
        switch (codecId)
        {
        case MFX_CODEC_VP8:
            return 4096;
        case MFX_CODEC_VP9:
            if (hwType < MFX_HW_KBL)
                return 4096;
            return 16384;
#if defined(MFX_ENABLE_AV1_VIDEO_DECODE)
        case MFX_CODEC_AV1:
            return 16384;
#endif
        default: return 0;
        }
    }

    inline bool CheckProfile(mfxU32 codecId, mfxU16 profile)
    {
        switch (codecId)
        {
        case MFX_CODEC_VP8: return profile <= MFX_PROFILE_VP8_3;
        case MFX_CODEC_VP9: return profile <= MFX_PROFILE_VP9_3;
#if defined(MFX_ENABLE_AV1_VIDEO_DECODE)
        case MFX_CODEC_AV1: return profile <= MFX_PROFILE_AV1_PRO;
#endif
        default: return false;
        }
    }

    mfxStatus Query(VideoCORE *core, mfxVideoParam const*p_in, mfxVideoParam *p_out, mfxU32 codecId, eMFXHWType type)
    {
        MFX_CHECK_NULL_PTR1(p_out);
        mfxStatus  sts = MFX_ERR_NONE;

        if (p_in == p_out)
        {
            mfxVideoParam in1 = *p_in;
            return Query(core, &in1, p_out, codecId, type);
        }

        memset(&p_out->mfx, 0, sizeof(mfxInfoMFX));

        if (p_in)
        {
            if (p_in->mfx.CodecId == codecId)
                p_out->mfx.CodecId = p_in->mfx.CodecId;

            if (CheckProfile(codecId, p_in->mfx.CodecProfile))
                p_out->mfx.CodecProfile = p_in->mfx.CodecProfile;

            switch (p_in->mfx.CodecLevel)
            {
            case MFX_LEVEL_UNKNOWN:
                p_out->mfx.CodecLevel = p_in->mfx.CodecLevel;
                break;
            }

#if defined(MFX_ENABLE_AV1_VIDEO_DECODE)
            if (codecId == MFX_CODEC_AV1)
            {
                p_out->mfx.CodecLevel = p_in->mfx.CodecLevel;
                p_out->mfx.FilmGrain = p_in->mfx.FilmGrain;
            }
#endif

            if (p_in->mfx.NumThread < 128)
                p_out->mfx.NumThread = p_in->mfx.NumThread;

            if (p_in->AsyncDepth < MFX_MAX_ASYNC_DEPTH_VALUE) // Actually AsyncDepth > 5-7 is for debugging only.
                p_out->AsyncDepth = p_in->AsyncDepth;

            if ((p_in->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY) || (p_in->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY))
            {
                if (!((p_in->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) && (p_in->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY)))
                    p_out->IOPattern = p_in->IOPattern;
            }

            switch (p_in->mfx.FrameInfo.FourCC)
            {
            case MFX_FOURCC_NV12:
            case MFX_FOURCC_AYUV:
            case MFX_FOURCC_P010:
#if (MFX_VERSION >= 1027)
            case MFX_FOURCC_Y410:
#endif
#if (MFX_VERSION >= 1031)
            case MFX_FOURCC_P016:
            case MFX_FOURCC_Y416:
#endif
                p_out->mfx.FrameInfo.FourCC = p_in->mfx.FrameInfo.FourCC;
                break;
            default:
                sts = MFX_ERR_UNSUPPORTED;
                break;
            }

            switch (p_in->mfx.FrameInfo.ChromaFormat)
            {
            case MFX_CHROMAFORMAT_YUV420:
            case MFX_CHROMAFORMAT_YUV444:
                p_out->mfx.FrameInfo.ChromaFormat = p_in->mfx.FrameInfo.ChromaFormat;
                break;
            default:
                if (p_in->mfx.FrameInfo.FourCC)
                    sts = MFX_ERR_UNSUPPORTED;

                break;
            }

            if (p_in->mfx.FrameInfo.FourCC && p_in->mfx.FrameInfo.ChromaFormat)
            {
                if ((   p_in->mfx.FrameInfo.FourCC == MFX_FOURCC_NV12 && p_in->mfx.FrameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV420)
                    || (p_in->mfx.FrameInfo.FourCC == MFX_FOURCC_AYUV && p_in->mfx.FrameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV444)
                    || (p_in->mfx.FrameInfo.FourCC == MFX_FOURCC_P010 && p_in->mfx.FrameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV420)
#if (MFX_VERSION >= 1027)
                  //|| (p_in->mfx.FrameInfo.FourCC == MFX_FOURCC_Y210 && p_in->mfx.FrameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV422)
                    || (p_in->mfx.FrameInfo.FourCC == MFX_FOURCC_Y410 && p_in->mfx.FrameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV444)
#endif
#if (MFX_VERSION >= 1031)
                    || (p_in->mfx.FrameInfo.FourCC == MFX_FOURCC_P016 && p_in->mfx.FrameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV420)
                    || (p_in->mfx.FrameInfo.FourCC == MFX_FOURCC_Y416 && p_in->mfx.FrameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV444)
#endif
                    )
                {
                    p_out->mfx.FrameInfo.FourCC = 0;
                    p_out->mfx.FrameInfo.ChromaFormat = 0;
                    p_out->mfx.FrameInfo.BitDepthLuma = 0;
                    p_out->mfx.FrameInfo.BitDepthChroma = 0;
                    sts = MFX_ERR_UNSUPPORTED;
                }
            }

            p_out->mfx.FrameInfo.BitDepthLuma   = p_in->mfx.FrameInfo.BitDepthLuma;
            p_out->mfx.FrameInfo.BitDepthChroma = p_in->mfx.FrameInfo.BitDepthChroma;
            p_out->mfx.FrameInfo.Shift          = p_in->mfx.FrameInfo.Shift;

            if ((p_in->mfx.FrameInfo.FourCC == MFX_FOURCC_NV12
                || p_in->mfx.FrameInfo.FourCC == MFX_FOURCC_AYUV) &&
               ((p_in->mfx.FrameInfo.BitDepthLuma   != 0 && p_in->mfx.FrameInfo.BitDepthLuma   != 8) ||
                (p_in->mfx.FrameInfo.BitDepthChroma != 0 && p_in->mfx.FrameInfo.BitDepthChroma != 8) ||
                    p_in->mfx.FrameInfo.Shift))
            {
                p_out->mfx.FrameInfo.BitDepthLuma = 0;
                p_out->mfx.FrameInfo.BitDepthChroma = 0;
                p_out->mfx.FrameInfo.Shift = 0;
                sts = MFX_ERR_UNSUPPORTED;
            }
            if ((  p_in->mfx.FrameInfo.FourCC == MFX_FOURCC_P010
#if (MFX_VERSION >= 1027)
                || p_in->mfx.FrameInfo.FourCC == MFX_FOURCC_Y410
#endif
                ) &&
                ((p_in->mfx.FrameInfo.BitDepthLuma != 0 && p_in->mfx.FrameInfo.BitDepthLuma != 10) ||
                 (p_in->mfx.FrameInfo.BitDepthChroma != 0 && p_in->mfx.FrameInfo.BitDepthChroma != 10)))
            {
                p_out->mfx.FrameInfo.BitDepthLuma = 0;
                p_out->mfx.FrameInfo.BitDepthChroma = 0;
                p_out->mfx.FrameInfo.Shift = 0;
                sts = MFX_ERR_UNSUPPORTED;
            }
#if (MFX_VERSION >= 1031)
            if ((  p_in->mfx.FrameInfo.FourCC == MFX_FOURCC_P016
                || p_in->mfx.FrameInfo.FourCC == MFX_FOURCC_Y416) &&
                ((p_in->mfx.FrameInfo.BitDepthLuma != 0 && p_in->mfx.FrameInfo.BitDepthLuma != 12) ||
                 (p_in->mfx.FrameInfo.BitDepthChroma != 0 && p_in->mfx.FrameInfo.BitDepthChroma != 12)))
            {
                p_out->mfx.FrameInfo.BitDepthLuma = 0;
                p_out->mfx.FrameInfo.BitDepthChroma = 0;
                p_out->mfx.FrameInfo.Shift = 0;
                sts = MFX_ERR_UNSUPPORTED;
            }
#endif

            if (!p_in->mfx.FrameInfo.ChromaFormat && !(!p_in->mfx.FrameInfo.FourCC && !p_in->mfx.FrameInfo.ChromaFormat))
                sts = MFX_ERR_UNSUPPORTED;

            if (p_in->mfx.FrameInfo.Width % 16 == 0 && p_in->mfx.FrameInfo.Width <= GetMaxWidth(codecId, core->GetHWType()))
                p_out->mfx.FrameInfo.Width = p_in->mfx.FrameInfo.Width;
            else
                sts = MFX_ERR_UNSUPPORTED;

            if (p_in->mfx.FrameInfo.Height % 16 == 0 && p_in->mfx.FrameInfo.Height <= GetMaxHeight(codecId, core->GetHWType()))
                p_out->mfx.FrameInfo.Height = p_in->mfx.FrameInfo.Height;
            else
                sts = MFX_ERR_UNSUPPORTED;

            if (p_in->mfx.FrameInfo.CropX <= p_out->mfx.FrameInfo.Width)
                p_out->mfx.FrameInfo.CropX = p_in->mfx.FrameInfo.CropX;

            if (p_in->mfx.FrameInfo.CropY <= p_out->mfx.FrameInfo.Height)
                p_out->mfx.FrameInfo.CropY = p_in->mfx.FrameInfo.CropY;

            if (p_out->mfx.FrameInfo.CropX + p_in->mfx.FrameInfo.CropW <= p_out->mfx.FrameInfo.Width)
                p_out->mfx.FrameInfo.CropW = p_in->mfx.FrameInfo.CropW;

            if (p_out->mfx.FrameInfo.CropY + p_in->mfx.FrameInfo.CropH <= p_out->mfx.FrameInfo.Height)
                p_out->mfx.FrameInfo.CropH = p_in->mfx.FrameInfo.CropH;

            if (p_in->mfx.FrameInfo.FrameRateExtN != 0 && p_in->mfx.FrameInfo.FrameRateExtD == 0)
            {
                sts = MFX_ERR_UNSUPPORTED;
            }
            else
            {
                p_out->mfx.FrameInfo.FrameRateExtN = p_in->mfx.FrameInfo.FrameRateExtN;
                p_out->mfx.FrameInfo.FrameRateExtD = p_in->mfx.FrameInfo.FrameRateExtD;
            }

            if ((p_in->mfx.FrameInfo.AspectRatioW == 0 && p_in->mfx.FrameInfo.AspectRatioH == 0) ||
                (p_in->mfx.FrameInfo.AspectRatioW != 0 && p_in->mfx.FrameInfo.AspectRatioH != 0))
            {
                p_out->mfx.FrameInfo.AspectRatioW = p_in->mfx.FrameInfo.AspectRatioW;
                p_out->mfx.FrameInfo.AspectRatioH = p_in->mfx.FrameInfo.AspectRatioH;
            }
            else
            {
                sts = MFX_ERR_UNSUPPORTED;
            }

            switch (p_in->mfx.FrameInfo.PicStruct)
            {
            case MFX_PICSTRUCT_UNKNOWN:
            case MFX_PICSTRUCT_PROGRESSIVE:
                p_out->mfx.FrameInfo.PicStruct = p_in->mfx.FrameInfo.PicStruct;
                break;
            default:
                sts = MFX_ERR_UNSUPPORTED;
                break;
            }

            if (p_in->mfx.ExtendedPicStruct)
                sts = MFX_ERR_UNSUPPORTED;

            if (p_in->mfx.DecodedOrder)
                sts = MFX_ERR_UNSUPPORTED;

            mfxStatus stsExt = CheckDecodersExtendedBuffers(p_in);
            if (stsExt < MFX_ERR_NONE)
                sts = MFX_ERR_UNSUPPORTED;

            if (p_in->Protected)
            {
                sts = MFX_ERR_UNSUPPORTED;
            }

            mfxExtOpaqueSurfaceAlloc *opaque_in = (mfxExtOpaqueSurfaceAlloc *)GetExtBuffer(p_in->ExtParam, p_in->NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
            mfxExtOpaqueSurfaceAlloc *opaque_out = (mfxExtOpaqueSurfaceAlloc *)GetExtBuffer(p_out->ExtParam, p_out->NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);

            if (opaque_in && opaque_out)
            {
                MFX_CHECK(opaque_out->In.Surfaces && opaque_in->In.Surfaces, MFX_ERR_UNDEFINED_BEHAVIOR);
                opaque_out->In.Type = opaque_in->In.Type;
                opaque_out->In.NumSurface = opaque_in->In.NumSurface;
                if (opaque_in->In.Surfaces != opaque_out->In.Surfaces)
                    std::copy_n(opaque_in->In.Surfaces, opaque_in->In.NumSurface, opaque_out->In.Surfaces);

                MFX_CHECK(opaque_out->Out.Surfaces && opaque_in->Out.Surfaces, MFX_ERR_UNDEFINED_BEHAVIOR);
                opaque_out->Out.Type = opaque_in->Out.Type;
                opaque_out->Out.NumSurface = opaque_in->Out.NumSurface;
                if (opaque_in->Out.Surfaces != opaque_out->Out.Surfaces)
                    std::copy_n(opaque_in->Out.Surfaces, opaque_in->Out.NumSurface, opaque_out->Out.Surfaces);
            }
            else
            {
                MFX_CHECK(!opaque_out && !opaque_in, MFX_ERR_UNDEFINED_BEHAVIOR);
            }
        }
        else
        {
            p_out->mfx.CodecId = codecId;
            p_out->mfx.CodecProfile = 1;
            p_out->mfx.CodecLevel = 1;

#if defined(MFX_ENABLE_AV1_VIDEO_DECODE)
            if (codecId == MFX_CODEC_AV1)
            {
                p_out->mfx.CodecLevel = MFX_LEVEL_AV1_2;
                p_out->mfx.FilmGrain = 1;
            }
#endif

            p_out->mfx.NumThread = 1;

            p_out->AsyncDepth = 1;

            // mfxFrameInfo
            p_out->mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
            p_out->mfx.FrameInfo.Width = 16;
            p_out->mfx.FrameInfo.Height = 16;

            p_out->mfx.FrameInfo.FrameRateExtN = 1;
            p_out->mfx.FrameInfo.FrameRateExtD = 1;

            p_out->mfx.FrameInfo.BitDepthLuma = 8;
            p_out->mfx.FrameInfo.BitDepthChroma = 8;
            p_out->mfx.FrameInfo.Shift = 0;

            p_out->mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;

            if (type == MFX_HW_UNKNOWN)
            {
                p_out->IOPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
            }
            else
            {
                p_out->IOPattern = MFX_IOPATTERN_OUT_VIDEO_MEMORY;
            }
        }

        return sts;
    }

    bool CheckFrameInfo(const mfxFrameInfo &frameInfo, mfxU32 codecId, eMFXPlatform platform,  eMFXHWType hwtype)
    {
        (void)hwtype;

        if (codecId == MFX_CODEC_VP8 || platform == MFX_PLATFORM_SOFTWARE)
            if (frameInfo.Width > 4096 || frameInfo.Height > 4096)
                return false;

        if (frameInfo.Height % 16 || frameInfo.Width % 16)
            return false;

        if ((frameInfo.AspectRatioW != 0 && frameInfo.AspectRatioH == 0) ||
            (frameInfo.AspectRatioW == 0 && frameInfo.AspectRatioH != 0))
            return false;

        if (codecId != MFX_CODEC_VP8)
        {
            if (   frameInfo.FourCC != MFX_FOURCC_NV12
                && frameInfo.FourCC != MFX_FOURCC_AYUV
                && frameInfo.FourCC != MFX_FOURCC_P010
#if (MFX_VERSION >= 1027)
                //&& p_in->mfx.FrameInfo.FourCC != MFX_FOURCC_Y210
                && !(frameInfo.FourCC == MFX_FOURCC_Y410 && hwtype >= MFX_HW_ICL)
#endif
#if (MFX_VERSION >= 1031)
                && frameInfo.FourCC != MFX_FOURCC_P016
                && frameInfo.FourCC != MFX_FOURCC_Y416
#endif

            )
                return false;

            switch (frameInfo.ChromaFormat)
            {
            case MFX_CHROMAFORMAT_YUV420:
            case MFX_CHROMAFORMAT_YUV422:
            case MFX_CHROMAFORMAT_YUV444:
                break;

            default:
                return false;
            }

            if (frameInfo.ChromaFormat)
            {
                if ((frameInfo.FourCC == MFX_FOURCC_NV12 && frameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV420)
                    || (frameInfo.FourCC == MFX_FOURCC_AYUV && frameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV444)
                    || (frameInfo.FourCC == MFX_FOURCC_P010 && frameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV420)
#if (MFX_VERSION >= 1027)
                    || (frameInfo.FourCC == MFX_FOURCC_Y410 && frameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV444)
#endif
#if (MFX_VERSION >= 1031)
                   || (frameInfo.FourCC == MFX_FOURCC_P016 && frameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV420)
                   || (frameInfo.FourCC == MFX_FOURCC_Y416 && frameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV444)
#endif
                    )
                    return false;
            }
        }

        return true;
    }

    bool CheckInfoMFX(const mfxInfoMFX &mfx, mfxU32 codecId, eMFXPlatform platform,  eMFXHWType hwtype)
    {
        if (codecId != mfx.CodecId)
            return false;

        if (codecId == MFX_CODEC_VP8)
        {
            if (MFX_FOURCC_NV12 != mfx.FrameInfo.FourCC)
                return false;
            if (MFX_CHROMAFORMAT_YUV420 != mfx.FrameInfo.ChromaFormat)
            {
                return false;
            }
            if (mfx.CodecProfile > MFX_PROFILE_VP8_3)
                return false;

            if (mfx.CodecLevel != MFX_LEVEL_UNKNOWN)
                return false;
        }

        return CheckFrameInfo(mfx.FrameInfo, codecId, platform, hwtype);
    }

    bool CheckVideoParam(mfxVideoParam const*p_in, mfxU32 codecId, eMFXPlatform platform, eMFXHWType hwtype)
    {
        if (!p_in)
            return false;

        if (p_in->Protected)
            return false;

        if (!(p_in->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) && !(p_in->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY) && !(p_in->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY))
            return false;

        if ((p_in->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) && (p_in->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY) && (p_in->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY))
            return false;

        return CheckInfoMFX(p_in->mfx, codecId, platform, hwtype);
    }

    mfxStatus QueryIOSurfInternal(mfxVideoParam const*p_params, mfxFrameAllocRequest *p_request)
    {
        p_request->Info = p_params->mfx.FrameInfo;

        p_request->NumFrameMin = p_params->mfx.CodecId == MFX_CODEC_VP8 ? mfxU16(4) : mfxU16(8);

        p_request->NumFrameMin += p_params->AsyncDepth ? p_params->AsyncDepth : MFX_AUTO_ASYNC_DEPTH_VALUE;

#if defined(MFX_ENABLE_AV1_VIDEO_DECODE)
        if ((p_params->mfx.CodecId == MFX_CODEC_AV1) && p_params->mfx.FilmGrain)
            p_request->NumFrameMin = 2 * p_request->NumFrameMin; // we need two output surfaces for each frame when film_grain is applied
#endif
        // Increase minimum number by one
        // E.g., decoder unlocks references in sync part (NOT async), so in order to free some surface
        // application need an additional surface to call DecodeFrameAsync()
        p_request->NumFrameMin += 2;

        p_request->NumFrameSuggested = p_request->NumFrameMin;

        if (p_params->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY)
        {
            p_request->Type = MFX_MEMTYPE_DXVA2_DECODER_TARGET | MFX_MEMTYPE_INTERNAL_FRAME | MFX_MEMTYPE_FROM_DECODE;
        }
        else if (p_params->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY)
        {
            p_request->Type = MFX_MEMTYPE_DXVA2_DECODER_TARGET | MFX_MEMTYPE_EXTERNAL_FRAME | MFX_MEMTYPE_FROM_DECODE;
        }
        else if (p_params->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY)
        {
            p_request->Type = MFX_MEMTYPE_DXVA2_DECODER_TARGET | MFX_MEMTYPE_OPAQUE_FRAME | MFX_MEMTYPE_FROM_DECODE;
        }

        return MFX_ERR_NONE;
    }

}
