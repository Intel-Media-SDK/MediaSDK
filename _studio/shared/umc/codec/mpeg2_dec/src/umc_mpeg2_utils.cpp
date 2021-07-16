// Copyright (c) 2018-2019 Intel Corporation
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


#include "umc_defs.h"

#if defined (MFX_ENABLE_MPEG2_VIDEO_DECODE)

#include "mfxstructures.h"
#include "umc_video_decoder.h"

#include "mfx_common_int.h"
#include "umc_mpeg2_utils.h"

#define CHECK_UNSUPPORTED(EXPR) if (EXPR) return MFX_ERR_UNSUPPORTED;

namespace UMC_MPEG2_DECODER
{
    // Check HW capabilities
    bool IsNeedPartialAcceleration_MPEG2(mfxVideoParam* par)
    {
        return (par->mfx.CodecProfile == MFX_PROFILE_MPEG1) ? true : false;
    }

    // Returns implementation platform
    eMFXPlatform GetPlatform_MPEG2(VideoCORE * core, mfxVideoParam * par)
    {
        if (!par)
            return MFX_PLATFORM_SOFTWARE;

        eMFXPlatform platform = core->GetPlatformType();

        if (IsNeedPartialAcceleration_MPEG2(par) && platform != MFX_PLATFORM_SOFTWARE)
        {
            return MFX_PLATFORM_SOFTWARE;
        }

        if (platform != MFX_PLATFORM_SOFTWARE)
        {
#if defined (MFX_VA_LINUX)
            if (MFX_ERR_NONE != core->IsGuidSupported(sDXVA2_ModeMPEG2_VLD, par))
            {
                platform = MFX_PLATFORM_SOFTWARE;
            }
#endif
        }

        return platform;
    }

    bool IsHWSupported(VideoCORE *core, mfxVideoParam *par)
    {
        if ((par->mfx.CodecProfile == MFX_PROFILE_MPEG1 && core->GetPlatformType()== MFX_PLATFORM_HARDWARE))
            return false;

#if defined (MFX_VA_LINUX)
        if (MFX_ERR_NONE != core->IsGuidSupported(sDXVA2_ModeMPEG2_VLD, par))
            return false;
#endif

        return true;
    }

    mfxStatus CheckFrameInfo(mfxFrameInfo & in, mfxFrameInfo & out)
    {
        mfxStatus sts = MFX_ERR_NONE;

        if (!((in.Width % 16 == 0) && (in.Width <= 4096)))
        {
            out.Width = 0;
            sts = MFX_ERR_UNSUPPORTED;
        }

        if (!((in.Height % 16 == 0) && (in.Height <= 4096)))
        {
            out.Height = 0;
            sts = MFX_ERR_UNSUPPORTED;
        }

        if (!(in.CropX <= out.Width))
        {
            out.CropX = 0;
            sts = MFX_ERR_UNSUPPORTED;
        }

        if (!(in.CropY <= out.Height))
        {
            out.CropY = 0;
            sts = MFX_ERR_UNSUPPORTED;
        }

        if (!(out.CropX + in.CropW <= out.Width))
        {
            out.CropW = 0;
            sts = MFX_ERR_UNSUPPORTED;
        }

        if (!(out.CropY + in.CropH <= out.Height))
        {
            out.CropH = 0;
            sts = MFX_ERR_UNSUPPORTED;
        }

        if (!(in.PicStruct == MFX_PICSTRUCT_PROGRESSIVE ||
            in.PicStruct == MFX_PICSTRUCT_FIELD_TFF ||
            in.PicStruct == MFX_PICSTRUCT_FIELD_BFF ||
            in.PicStruct == MFX_PICSTRUCT_UNKNOWN))
        {
            out.PicStruct = 0;
            sts = MFX_ERR_UNSUPPORTED;
        }

        if (!(in.ChromaFormat == MFX_CHROMAFORMAT_YUV420 || in.ChromaFormat == 0))
        {
            out.ChromaFormat = 0;
            sts = MFX_ERR_UNSUPPORTED;
        }
        return sts;
    }

    // MediaSDK API Query function
    mfxStatus Query(VideoCORE *core, mfxVideoParam *in, mfxVideoParam *out)
    {
        MFX_CHECK_NULL_PTR1(out);
        mfxStatus res = MFX_ERR_NONE;

        eMFXHWType type = core->GetHWType();
        if (!in)
        {
            // configurability mode
            memset(out, 0, sizeof(*out));
            out->mfx.CodecId = MFX_CODEC_MPEG2;
            out->mfx.FrameInfo.FourCC = 1;
            out->mfx.FrameInfo.Width = 1;
            out->mfx.FrameInfo.Height = 1;
            out->mfx.FrameInfo.CropX = 1;
            out->mfx.FrameInfo.CropY = 1;
            out->mfx.FrameInfo.CropW = 1;
            out->mfx.FrameInfo.CropH = 1;
            out->mfx.FrameInfo.AspectRatioH = 1;
            out->mfx.FrameInfo.AspectRatioW = 1;

            out->mfx.FrameInfo.FrameRateExtN = 1;
            out->mfx.FrameInfo.FrameRateExtD = 1;
            out->mfx.FrameInfo.PicStruct = 1;
            out->mfx.CodecProfile = 1;
            out->mfx.CodecLevel = 1;
            out->mfx.ExtendedPicStruct = 1;
            out->mfx.TimeStampCalc = 1;

            out->Protected = 0;

            out->mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;

            out->IOPattern = type == MFX_HW_UNKNOWN ? MFX_IOPATTERN_OUT_SYSTEM_MEMORY : MFX_IOPATTERN_OUT_VIDEO_MEMORY;

            // DECODE's configurables
            out->mfx.NumThread = 1;
            out->AsyncDepth = MFX_AUTO_ASYNC_DEPTH_VALUE;
        }
        else
        {
            // checking mode
            CHECK_UNSUPPORTED(1 == in->mfx.DecodedOrder);

            if (in->mfx.FrameInfo.FourCC != MFX_FOURCC_NV12)
            {
                out->mfx.FrameInfo.FourCC = 0;
                return MFX_ERR_UNSUPPORTED;
            }

            CHECK_UNSUPPORTED( (in->NumExtParam == 0) != (in->ExtParam == nullptr) );

            if (in->NumExtParam && !in->Protected)
            {
                auto pOpaq = (mfxExtOpaqueSurfaceAlloc *)GetExtendedBuffer(in->ExtParam, in->NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);

                // ignore opaque extension buffer
                CHECK_UNSUPPORTED(in->NumExtParam != 1 || !(in->NumExtParam == 1 && NULL != pOpaq));
            }

            mfxExtBuffer **pExtBuffer = out->ExtParam;
            mfxU16 numDistBuf = out->NumExtParam;
            *out = *in;

            out->AsyncDepth = in->AsyncDepth == 0 ? MFX_AUTO_ASYNC_DEPTH_VALUE : out->AsyncDepth;
            out->ExtParam = pExtBuffer;
            out->NumExtParam = numDistBuf;

            res = CheckFrameInfo(in->mfx.FrameInfo, out->mfx.FrameInfo);
            MFX_CHECK_STS(res);

            out->mfx.CodecId = in->mfx.CodecId != MFX_CODEC_MPEG2 ? 0 : out->mfx.CodecId;

            if (!(in->mfx.CodecLevel == MFX_LEVEL_MPEG2_LOW ||
                in->mfx.CodecLevel == MFX_LEVEL_MPEG2_MAIN ||
                in->mfx.CodecLevel == MFX_LEVEL_MPEG2_HIGH1440 ||
                in->mfx.CodecLevel == MFX_LEVEL_MPEG2_HIGH ||
                in->mfx.CodecLevel == 0))
            {
                out->mfx.CodecLevel = 0;
                return MFX_ERR_UNSUPPORTED;
            }

            if (!(in->mfx.CodecProfile == MFX_PROFILE_MPEG2_SIMPLE ||
                in->mfx.CodecProfile == MFX_PROFILE_MPEG2_MAIN ||
                in->mfx.CodecProfile == MFX_PROFILE_MPEG2_HIGH ||
                in->mfx.CodecProfile == MFX_PROFILE_MPEG1 ||
                in->mfx.CodecProfile == 0))
            {
                out->mfx.CodecProfile = 0;
                return MFX_ERR_UNSUPPORTED;
            }

            if ((in->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY) || (in->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY))
                out->IOPattern = in->IOPattern;
            else if (MFX_PLATFORM_SOFTWARE == core->GetPlatformType())
                out->IOPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
            else
                out->IOPattern = MFX_IOPATTERN_OUT_VIDEO_MEMORY;

            CHECK_UNSUPPORTED(!IsHWSupported(core, in));
        }

        return res;
    }

    // Table 6-4 – frame_rate_value
    void GetMfxFrameRate(uint8_t frame_rate_value, mfxU32 & frameRateN, mfxU32 & frameRateD)
    {
        switch (frame_rate_value)
        {
            case 0:  frameRateN = 30;    frameRateD = 1;    break;
            case 1:  frameRateN = 24000; frameRateD = 1001; break;
            case 2:  frameRateN = 24;    frameRateD = 1;    break;
            case 3:  frameRateN = 25;    frameRateD = 1;    break;
            case 4:  frameRateN = 30000; frameRateD = 1001; break;
            case 5:  frameRateN = 30;    frameRateD = 1;    break;
            case 6:  frameRateN = 50;    frameRateD = 1;    break;
            case 7:  frameRateN = 60000; frameRateD = 1001; break;
            case 8:  frameRateN = 60;    frameRateD = 1;    break;
            default: frameRateN = 30;    frameRateD = 1;
        }
        return;
    }

    // Table 8-2 – Profile identification
    mfxU8 GetMfxCodecProfile(uint8_t profile)
    {
        switch (profile)
        {
            case 5:
                return MFX_PROFILE_MPEG2_SIMPLE;
            case 4:
                return MFX_PROFILE_MPEG2_MAIN;
            case 1:
                return MFX_PROFILE_MPEG2_HIGH;
            default:
                return MFX_PROFILE_UNKNOWN;
        }
    }

    // Table 8-3 – Level identification
    mfxU8 GetMfxCodecLevel(uint8_t level)
    {
        switch (level)
        {
            case 10:
                return MFX_LEVEL_MPEG2_LOW;
            case 8:
                return MFX_LEVEL_MPEG2_MAIN;
            case 6:
                return MFX_LEVEL_MPEG2_HIGH1440;
            case 4:
                return MFX_LEVEL_MPEG2_HIGH;
            default:
                return MFX_LEVEL_UNKNOWN;
        }
    }

    void DARtoPAR(uint32_t width, uint32_t height, uint32_t dar_h, uint32_t dar_v,
                uint16_t & par_h, uint16_t &par_v)
    {
        uint32_t simple_tab[] = {2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59};

        uint32_t h = dar_h * height;
        uint32_t v = dar_v * width;

        // remove common multipliers
        while ( ((h|v)&1) == 0 )
        {
            h >>= 1;
            v >>= 1;
        }

        uint32_t denom;
        for (uint32_t i=0; i < (uint32_t)(sizeof(simple_tab)/sizeof(simple_tab[0])); ++i)
        {
            denom = simple_tab[i];
            while(h%denom==0 && v%denom==0)
            {
                v /= denom;
                h /= denom;
            }
            if (v <= denom || h <= denom)
                break;
        }
        par_h = h;
        par_v = v;
    }

    void CalcAspectRatio(uint32_t dar, uint32_t width, uint32_t height,
                        uint16_t & aspectRatioW, uint16_t & aspectRatioH)
    {
        if (dar == 2)
        {
            DARtoPAR(width, height, 4, 3, aspectRatioW, aspectRatioH);
        }
        else if (dar == 3)
        {
            DARtoPAR(width, height, 16, 9, aspectRatioW, aspectRatioH);
        }
        else if (dar == 4)
        {
            DARtoPAR(width, height, 221, 100, aspectRatioW, aspectRatioH);
        }
        else // dar == 1 or unknown
        {
            aspectRatioW = 1;
            aspectRatioH = 1;
        }
    }

    // Validate input parameters
    bool CheckVideoParam(mfxVideoParam *in)
    {
        if (!in)
            return false;

        if (MFX_CODEC_MPEG2 != in->mfx.CodecId)
            return false;

        if (in->mfx.FrameInfo.FourCC != MFX_FOURCC_NV12)
            return false;

        // both zero or not zero
        if ((in->mfx.FrameInfo.AspectRatioW || in->mfx.FrameInfo.AspectRatioH) && !(in->mfx.FrameInfo.AspectRatioW && in->mfx.FrameInfo.AspectRatioH))
            return false;

        if ((MFX_PROFILE_UNKNOWN      != in->mfx.CodecProfile) &&
            (MFX_PROFILE_MPEG2_SIMPLE != in->mfx.CodecProfile) &&
            (MFX_PROFILE_MPEG2_MAIN   != in->mfx.CodecProfile) &&
            (MFX_PROFILE_MPEG2_HIGH   != in->mfx.CodecProfile))
            return false;

        switch (in->mfx.FrameInfo.PicStruct)
        {
        case MFX_PICSTRUCT_UNKNOWN:
        case MFX_PICSTRUCT_PROGRESSIVE:
        case MFX_PICSTRUCT_FIELD_TFF:
        case MFX_PICSTRUCT_FIELD_BFF:
            break;
        default:
            return false;
        }

        if (in->mfx.FrameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV420)
            return false;

        if (!(in->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) && !(in->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY) && !(in->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY))
            return false;

        if ((in->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) && (in->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY))
            return false;

        if ((in->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY) && (in->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY))
            return false;

        if ((in->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY) && (in->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY))
            return false;

        return true;
    }

    // Initialize mfxVideoParam structure based on decoded bitstream header values
    UMC::Status UMC_MPEG2_DECODER::MFX_Utility::FillVideoParam(const MPEG2SequenceHeader & seq,
                                                               const MPEG2SequenceExtension * seqExt,
                                                               const MPEG2SequenceDisplayExtension * dispExt,
                                                               mfxVideoParam & par)
    {
        par.mfx.CodecId = MFX_CODEC_MPEG2;

        par.mfx.FrameInfo.PicStruct = seqExt && seqExt->progressive_sequence ? MFX_PICSTRUCT_PROGRESSIVE : MFX_PICSTRUCT_UNKNOWN;

        par.mfx.FrameInfo.CropX = 0;
        par.mfx.FrameInfo.CropY = 0;
        par.mfx.FrameInfo.CropW = seq.horizontal_size_value;
        par.mfx.FrameInfo.CropH = seq.vertical_size_value;

        par.mfx.FrameInfo.Width  = mfx::align2_value(par.mfx.FrameInfo.CropW, 16);
        par.mfx.FrameInfo.Height = mfx::align2_value(par.mfx.FrameInfo.CropH, (MFX_PICSTRUCT_PROGRESSIVE == par.mfx.FrameInfo.PicStruct) ? 16 : 32);

        par.mfx.FrameInfo.BitDepthLuma   = 8;
        par.mfx.FrameInfo.BitDepthChroma = 8;
        par.mfx.FrameInfo.Shift = 0;

        if (seqExt)
        {
            par.mfx.FrameInfo.ChromaFormat = seqExt->chroma_format == CHROMA_FORMAT_420 ?
                                                MFX_CHROMAFORMAT_YUV420 :
                                                (seqExt->chroma_format == CHROMA_FORMAT_422 ?
                                                        MFX_CHROMAFORMAT_YUV422 :
                                                        MFX_CHROMAFORMAT_YUV444);

            // Table 8-1 – Meaning of bits in profile_and_level_indication
            par.mfx.CodecProfile = GetMfxCodecProfile((seqExt->profile_and_level_indication >> 4) & 7); // [6:4] bits
            par.mfx.CodecLevel = GetMfxCodecLevel(seqExt->profile_and_level_indication & 0xF); // [3:0] bits
        }
        else
        {
            par.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
            par.mfx.CodecProfile = MFX_PROFILE_MPEG1;
            par.mfx.CodecLevel = MFX_LEVEL_UNKNOWN;
        }

        // 6.3.3
        uint32_t w = dispExt ? dispExt->display_horizontal_size : seq.horizontal_size_value;
        uint32_t h = dispExt ? dispExt->display_vertical_size : seq.vertical_size_value;
        CalcAspectRatio(seq.aspect_ratio_information, w, h, par.mfx.FrameInfo.AspectRatioW, par.mfx.FrameInfo.AspectRatioH);

        GetMfxFrameRate(seq.frame_rate_code, par.mfx.FrameInfo.FrameRateExtN, par.mfx.FrameInfo.FrameRateExtD);

        par.mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;

        // video signal section
        mfxExtVideoSignalInfo * videoSignal = (mfxExtVideoSignalInfo *)GetExtendedBuffer(par.ExtParam, par.NumExtParam, MFX_EXTBUFF_VIDEO_SIGNAL_INFO);
        if (!videoSignal)
            return UMC::UMC_OK;

        if (dispExt)
        {
            videoSignal->VideoFormat              = (mfxU16)dispExt->video_format;
            videoSignal->ColourPrimaries          = (mfxU16)dispExt->colour_primaries;
            videoSignal->TransferCharacteristics  = (mfxU16)dispExt->transfer_characteristics;
            videoSignal->MatrixCoefficients       = (mfxU16)dispExt->matrix_coefficients;
            videoSignal->ColourDescriptionPresent = (mfxU16)dispExt->colour_description;
        }
        else
        {
            // default values
            videoSignal->VideoFormat              = 5; // unspecified video format
            videoSignal->ColourPrimaries          = 1;
            videoSignal->TransferCharacteristics  = 1;
            videoSignal->MatrixCoefficients       = 1;
            videoSignal->ColourDescriptionPresent = 0;
        }

        return UMC::UMC_OK;
    }
}
#endif // MFX_ENABLE_MPEG2_VIDEO_DECODE
