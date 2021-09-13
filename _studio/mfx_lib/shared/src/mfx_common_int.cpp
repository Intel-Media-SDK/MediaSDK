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

#include "mfx_common_int.h"
#include "mfx_ext_buffers.h"
#include "mfxpcp.h"
#include "mfxfei.h"
#include "mfx_utils.h"

#include <stdexcept>
#include <string>
#include <climits>


mfxExtBuffer* GetExtendedBuffer(mfxExtBuffer** extBuf, mfxU32 numExtBuf, mfxU32 id)
{
    if (extBuf != 0)
    {
        for (mfxU16 i = 0; i < numExtBuf; i++)
        {
            if (extBuf[i] != 0 && extBuf[i]->BufferId == id) // assuming aligned buffers
                return (extBuf[i]);
        }
    }

    return 0;
}

mfxExtBuffer* GetExtendedBufferInternal(mfxExtBuffer** extBuf, mfxU32 numExtBuf, mfxU32 id)
{
    mfxExtBuffer* result = GetExtendedBuffer(extBuf, numExtBuf, id);

    if (!result) throw std::logic_error(": no external buffer found");
    return result;
}

mfxStatus CheckFrameInfoCommon(mfxFrameInfo  *info, mfxU32 /* codecId */)
{
    MFX_CHECK_NULL_PTR1(info);

    MFX_CHECK(info->Width && info->Width % 16 == 0, MFX_ERR_INVALID_VIDEO_PARAM);
    MFX_CHECK(info->Height && info->Height % 16 == 0, MFX_ERR_INVALID_VIDEO_PARAM);

    switch (info->FourCC)
    {
    case MFX_FOURCC_NV12:
    case MFX_FOURCC_YV12:
    case MFX_FOURCC_YUY2:
    case MFX_FOURCC_RGB3:
    case MFX_FOURCC_RGB4:
#ifdef MFX_ENABLE_RGBP
    case MFX_FOURCC_RGBP:
#endif
    case MFX_FOURCC_P010:
    case MFX_FOURCC_NV16:
    case MFX_FOURCC_P210:
    case MFX_FOURCC_AYUV:

#if (MFX_VERSION >= 1027)
    case MFX_FOURCC_Y210:
    case MFX_FOURCC_Y410:
#endif //(MFX_VERSION >= 1027)

#if (MFX_VERSION >= 1031)
    case MFX_FOURCC_P016:
    case MFX_FOURCC_Y216:
    case MFX_FOURCC_Y416:
#endif

        break;
    default:
        MFX_RETURN(MFX_ERR_INVALID_VIDEO_PARAM);
    }

    MFX_CHECK((!info->BitDepthLuma || (info->BitDepthLuma >= 8)) &&
              (!info->BitDepthChroma || (info->BitDepthChroma >= 8))
              , MFX_ERR_INVALID_VIDEO_PARAM);

    if (info->BitDepthLuma > 8 || info->BitDepthChroma > 8)
    {
        switch (info->FourCC)
        {
        case MFX_FOURCC_P010:
        case MFX_FOURCC_P210:

#if (MFX_VERSION >= 1027)
        case MFX_FOURCC_Y210:
        case MFX_FOURCC_Y410:
#endif

#if (MFX_VERSION >= 1031)
        case MFX_FOURCC_P016:
        case MFX_FOURCC_Y216:
        case MFX_FOURCC_Y416:
#endif
            break;

        default:
            MFX_RETURN(MFX_ERR_INVALID_VIDEO_PARAM);
        }
    }

    if (info->Shift)
    {
        if (   info->FourCC != MFX_FOURCC_P010 && info->FourCC != MFX_FOURCC_P210
#if (MFX_VERSION >= 1027)
            && info->FourCC != MFX_FOURCC_Y210
#endif
#if (MFX_VERSION >= 1031)
            && info->FourCC != MFX_FOURCC_P016 && info->FourCC != MFX_FOURCC_Y216
            && info->FourCC != MFX_FOURCC_Y416
#endif
            )
            MFX_RETURN(MFX_ERR_INVALID_VIDEO_PARAM);
    }

    MFX_CHECK(info->ChromaFormat <= MFX_CHROMAFORMAT_YUV444,  MFX_ERR_INVALID_VIDEO_PARAM);

    MFX_CHECK(!(info->FrameRateExtN != 0 && info->FrameRateExtD == 0), MFX_ERR_INVALID_VIDEO_PARAM);

    MFX_CHECK((!info->AspectRatioW && !info->AspectRatioH) || (info->AspectRatioW && info->AspectRatioH), MFX_ERR_INVALID_VIDEO_PARAM);

    return MFX_ERR_NONE;
}

mfxStatus CheckFrameInfoEncoders(mfxFrameInfo  *info)
{
    if (info->CropX > info->Width)
        return MFX_ERR_INVALID_VIDEO_PARAM;

    if (info->CropY > info->Height)
        return MFX_ERR_INVALID_VIDEO_PARAM;

    if (info->CropX + info->CropW > info->Width)
        return MFX_ERR_INVALID_VIDEO_PARAM;

    if (info->CropY + info->CropH > info->Height)
        return MFX_ERR_INVALID_VIDEO_PARAM;

    switch (info->PicStruct)
    {
    case MFX_PICSTRUCT_UNKNOWN:
    case MFX_PICSTRUCT_PROGRESSIVE:
    case MFX_PICSTRUCT_FIELD_TFF:
    case MFX_PICSTRUCT_FIELD_BFF:
        break;

    default:
    case MFX_PICSTRUCT_FIELD_REPEATED:
    case MFX_PICSTRUCT_FRAME_DOUBLING:
    case MFX_PICSTRUCT_FRAME_TRIPLING:
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    // both zero or not zero
    if ((info->AspectRatioW || info->AspectRatioH) && !(info->AspectRatioW && info->AspectRatioH))
        return MFX_ERR_INVALID_VIDEO_PARAM;

    if ((info->FrameRateExtN || info->FrameRateExtD) && !(info->FrameRateExtN && info->FrameRateExtD))
        return MFX_ERR_INVALID_VIDEO_PARAM;

    return MFX_ERR_NONE;
}

mfxStatus CheckFrameInfoCodecs(mfxFrameInfo  *info, mfxU32 codecId, bool isHW)
{
    mfxStatus sts = CheckFrameInfoCommon(info, codecId);
    MFX_CHECK_STS(sts);

    switch (codecId)
    {
    case MFX_CODEC_JPEG:
        if (info->FourCC != MFX_FOURCC_NV12 && info->FourCC != MFX_FOURCC_RGB4 && info->FourCC != MFX_FOURCC_YUY2)
            MFX_RETURN(MFX_ERR_INVALID_VIDEO_PARAM);
        break;
    case MFX_CODEC_VP8:
        MFX_CHECK(info->FourCC == MFX_FOURCC_NV12 || info->FourCC == MFX_FOURCC_YV12, MFX_ERR_INVALID_VIDEO_PARAM);
        break;
    case MFX_CODEC_VP9:
        if (info->FourCC != MFX_FOURCC_NV12
            && info->FourCC != MFX_FOURCC_AYUV
            && info->FourCC != MFX_FOURCC_P010
#if (MFX_VERSION >= 1027)
            && info->FourCC != MFX_FOURCC_Y410
#endif
#if (MFX_VERSION >= 1031)
            && info->FourCC != MFX_FOURCC_P016
            && info->FourCC != MFX_FOURCC_Y416
#endif
            )
            MFX_RETURN(MFX_ERR_INVALID_VIDEO_PARAM);
        break;
    case MFX_CODEC_AVC:
        if (info->FourCC != MFX_FOURCC_NV12 &&
            info->FourCC != MFX_FOURCC_P010 &&
            info->FourCC != MFX_FOURCC_NV16 &&
            info->FourCC != MFX_FOURCC_P210)
            MFX_RETURN(MFX_ERR_INVALID_VIDEO_PARAM);
        break;
    case MFX_CODEC_HEVC:
        if (info->FourCC != MFX_FOURCC_NV12 &&
            info->FourCC != MFX_FOURCC_YUY2 &&
            info->FourCC != MFX_FOURCC_P010 &&
            info->FourCC != MFX_FOURCC_NV16 &&
            info->FourCC != MFX_FOURCC_P210 &&
            info->FourCC != MFX_FOURCC_AYUV
#if (MFX_VERSION >= 1027)
            && info->FourCC != MFX_FOURCC_Y210
            && info->FourCC != MFX_FOURCC_Y410
#endif
#if (MFX_VERSION >= 1031)
            && info->FourCC != MFX_FOURCC_P016
            && info->FourCC != MFX_FOURCC_Y216
            && info->FourCC != MFX_FOURCC_Y416
#endif
            )
            MFX_RETURN(MFX_ERR_INVALID_VIDEO_PARAM);
        break;
#if defined(MFX_ENABLE_AV1_VIDEO_DECODE) || defined(MFX_ENABLE_AV1_VIDEO_ENCODE)
    case MFX_CODEC_AV1:
            if (   info->FourCC != MFX_FOURCC_NV12
                && info->FourCC != MFX_FOURCC_YV12
                && info->FourCC != MFX_FOURCC_P010
                && info->FourCC != MFX_FOURCC_AYUV
                && info->FourCC != MFX_FOURCC_Y410)
            MFX_RETURN(MFX_ERR_INVALID_VIDEO_PARAM);
        break;
#endif

    default:
        MFX_CHECK(info->FourCC == MFX_FOURCC_NV12, MFX_ERR_INVALID_VIDEO_PARAM);
        break;
    }

    switch (codecId)
    {
    case MFX_CODEC_JPEG:
        MFX_CHECK(   info->ChromaFormat == MFX_CHROMAFORMAT_YUV420
                  || info->ChromaFormat == MFX_CHROMAFORMAT_YUV444
                  || info->ChromaFormat == MFX_CHROMAFORMAT_YUV400
                  || info->ChromaFormat == MFX_CHROMAFORMAT_YUV422H
                  , MFX_ERR_INVALID_VIDEO_PARAM);
        break;
    case MFX_CODEC_AVC:
        MFX_CHECK(   info->ChromaFormat == MFX_CHROMAFORMAT_YUV420
                  || info->ChromaFormat == MFX_CHROMAFORMAT_YUV422
                  || info->ChromaFormat == MFX_CHROMAFORMAT_YUV400
                  , MFX_ERR_INVALID_VIDEO_PARAM);
        break;
    case MFX_CODEC_HEVC:
    case MFX_CODEC_VP9:
        MFX_CHECK(   info->ChromaFormat == MFX_CHROMAFORMAT_YUV420
                  || info->ChromaFormat == MFX_CHROMAFORMAT_YUV400
                  || info->ChromaFormat == MFX_CHROMAFORMAT_YUV422
                  || info->ChromaFormat == MFX_CHROMAFORMAT_YUV444
                  , MFX_ERR_INVALID_VIDEO_PARAM);
        break;
    default:
        MFX_CHECK(info->ChromaFormat == MFX_CHROMAFORMAT_YUV420 || info->ChromaFormat == MFX_CHROMAFORMAT_YUV400, MFX_ERR_INVALID_VIDEO_PARAM);
        break;
    }

    if (codecId != MFX_CODEC_HEVC && (
           info->FourCC == MFX_FOURCC_P010
        || info->FourCC == MFX_FOURCC_P210
#if (MFX_VERSION >= 1027)
        || info->FourCC == MFX_FOURCC_Y210
#endif
#if (MFX_VERSION >= 1031)
        || info->FourCC == MFX_FOURCC_P016
        || info->FourCC == MFX_FOURCC_Y216
        || info->FourCC == MFX_FOURCC_Y416
#endif
        ))
    {
        MFX_CHECK(info->Shift == (isHW ? 1 : 0), MFX_ERR_INVALID_VIDEO_PARAM);
    }

    return MFX_ERR_NONE;
}

mfxStatus UpdateCscOutputFormat(mfxVideoParam *par, mfxFrameAllocRequest *request)
{
#ifdef MFX_DEC_VIDEO_POSTPROCESS_DISABLE
    (void)par;
    (void)request;
#else
    mfxExtDecVideoProcessing * videoProcessing = (mfxExtDecVideoProcessing *)GetExtendedBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_DEC_VIDEO_PROCESSING);
    if (videoProcessing && videoProcessing->Out.FourCC != par->mfx.FrameInfo.FourCC)
    {
        request->Info.ChromaFormat = videoProcessing->Out.ChromaFormat;
        request->Info.FourCC = videoProcessing->Out.FourCC;

        switch (videoProcessing->Out.FourCC)
        {
            // if is 8 bit, shift value has to be 0
        case MFX_FOURCC_NV12:
        case MFX_FOURCC_YUY2:
        case MFX_FOURCC_AYUV:
            request->Info.BitDepthLuma = 8;
            request->Info.Shift = 0;
            break;
            // 10 bit
        case MFX_FOURCC_P010:
#if (MFX_VERSION >= 1027)
        case MFX_FOURCC_Y210:
            request->Info.BitDepthLuma = 10;
            request->Info.Shift = 1;
            break;
        case MFX_FOURCC_Y410:
            request->Info.BitDepthLuma = 10;
            request->Info.Shift = 0;
            break;
#endif
            // 12 bit
#if (MFX_VERSION >= MFX_VERSION_NEXT)
        case MFX_FOURCC_P016:
        case MFX_FOURCC_Y416:
        case MFX_FOURCC_Y216:
            request->Info.BitDepthLuma = 12;
            request->Info.Shift = 1;
            break;
#endif
        case MFX_FOURCC_RGB4:
            request->Info.BitDepthLuma = 0;
            request->Info.Shift = 0;
            break;
        default:
            return MFX_ERR_UNSUPPORTED;
        }

        request->Info.BitDepthChroma = request->Info.BitDepthLuma;
    }
#endif // !MFX_DEC_VIDEO_POSTPROCESS_DISABLE

    return MFX_ERR_NONE;
}

mfxStatus CheckAudioParamCommon(mfxAudioParam *in)
{
//    mfxStatus sts;

    switch (in->mfx.CodecId)
    {
        case MFX_CODEC_AAC:
        case MFX_CODEC_MP3:
            break;
        default:
            return MFX_ERR_INVALID_AUDIO_PARAM;
    }

    return MFX_ERR_NONE;
}

mfxStatus CheckAudioParamDecoders(mfxAudioParam *in)
{
    mfxStatus sts = CheckAudioParamCommon(in);
    if (sts < MFX_ERR_NONE)
        return sts;

    return MFX_ERR_NONE;
}

mfxStatus CheckAudioParamEncoders(mfxAudioParam *in)
{
    mfxStatus sts = CheckAudioParamCommon(in);
    if (sts < MFX_ERR_NONE)
        return sts;

    return MFX_ERR_NONE;
}


static mfxStatus CheckVideoParamCommon(mfxVideoParam *in, eMFXHWType type)
{
    MFX_CHECK_NULL_PTR1(in);

    mfxStatus sts = CheckFrameInfoCodecs(&in->mfx.FrameInfo, in->mfx.CodecId, type != MFX_HW_UNKNOWN);
    MFX_CHECK_STS(sts);

    if (in->Protected)
    {
        MFX_CHECK(type != MFX_HW_UNKNOWN && IS_PROTECTION_ANY(in->Protected), MFX_ERR_INVALID_VIDEO_PARAM);
    }

    switch (in->mfx.CodecId)
    {
        case MFX_CODEC_AVC:
        case MFX_CODEC_HEVC:
        case MFX_CODEC_MPEG2:
        case MFX_CODEC_VC1:
        case MFX_CODEC_JPEG:
        case MFX_CODEC_VP8:
        case MFX_CODEC_VP9:
#if defined(MFX_ENABLE_AV1_VIDEO_DECODE) || defined(MFX_ENABLE_AV1_VIDEO_ENCODE)
        case MFX_CODEC_AV1:
#endif
            break;
        default:
            MFX_RETURN(MFX_ERR_INVALID_VIDEO_PARAM);
    }

    MFX_CHECK(in->IOPattern, MFX_ERR_INVALID_VIDEO_PARAM);

    if (   in->mfx.FrameInfo.FourCC == MFX_FOURCC_P010
        || in->mfx.FrameInfo.FourCC == MFX_FOURCC_P210
#if (MFX_VERSION >= 1027)
        || in->mfx.FrameInfo.FourCC == MFX_FOURCC_Y210
#endif
#if (MFX_VERSION >= 1031)
        || in->mfx.FrameInfo.FourCC == MFX_FOURCC_P016
        || in->mfx.FrameInfo.FourCC == MFX_FOURCC_Y216
        || in->mfx.FrameInfo.FourCC == MFX_FOURCC_Y416
#endif
       )
    {
        if (type == MFX_HW_UNKNOWN)
        {
            MFX_CHECK(in->mfx.FrameInfo.Shift == 0, MFX_ERR_INVALID_VIDEO_PARAM);
        }
        else
        {
            MFX_CHECK(!(in->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) || in->mfx.FrameInfo.Shift == 1, MFX_ERR_INVALID_VIDEO_PARAM);
        }
    }

    return MFX_ERR_NONE;
}

mfxStatus CheckVideoParamDecoders(mfxVideoParam *in, bool IsExternalFrameAllocator, eMFXHWType type)
{
    mfxStatus sts = CheckVideoParamCommon(in, type);
    MFX_CHECK(sts >= MFX_ERR_NONE, sts);

    MFX_CHECK((in->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) || (in->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY) || (in->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY)
        , MFX_ERR_INVALID_VIDEO_PARAM);

    MFX_CHECK(!(in->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) || !(in->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY)
        , MFX_ERR_INVALID_VIDEO_PARAM);

    if (in->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY)
    {
        MFX_CHECK(!(in->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY), MFX_ERR_INVALID_VIDEO_PARAM);
        MFX_CHECK(!(in->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY), MFX_ERR_INVALID_VIDEO_PARAM);
    }

    if (in->mfx.DecodedOrder && in->mfx.CodecId != MFX_CODEC_JPEG && in->mfx.CodecId != MFX_CODEC_AVC && in->mfx.CodecId != MFX_CODEC_HEVC)
        return MFX_ERR_UNSUPPORTED;

    if (!IsExternalFrameAllocator)
        MFX_CHECK(!(in->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY), MFX_ERR_INVALID_VIDEO_PARAM);

    sts = CheckDecodersExtendedBuffers(in);
    MFX_CHECK(sts >= MFX_ERR_NONE, sts);

    return MFX_ERR_NONE;
}

mfxStatus CheckVideoParamEncoders(mfxVideoParam *in, bool IsExternalFrameAllocator, eMFXHWType type)
{
    mfxStatus sts = CheckFrameInfoEncoders(&in->mfx.FrameInfo);
    if (sts < MFX_ERR_NONE)
        return sts;

    sts = CheckVideoParamCommon(in, type);
    if (sts < MFX_ERR_NONE)
        return sts;

    if (!IsExternalFrameAllocator && (in->IOPattern & MFX_IOPATTERN_IN_VIDEO_MEMORY))
        return MFX_ERR_INVALID_VIDEO_PARAM;

    if (in->Protected && !(in->IOPattern & MFX_IOPATTERN_IN_VIDEO_MEMORY))
        return MFX_ERR_INVALID_VIDEO_PARAM;

    return MFX_ERR_NONE;
}

mfxStatus CheckAudioFrame(const mfxAudioFrame *aFrame)
{
    if (!aFrame || !aFrame->Data)
        return MFX_ERR_NULL_PTR;

    if (aFrame->DataLength > aFrame->MaxLength)
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    return MFX_ERR_NONE;
}


mfxStatus CheckBitstream(const mfxBitstream *bs)
{
    if (!bs || !bs->Data)
        return MFX_ERR_NULL_PTR;

    if (bs->DataOffset + bs->DataLength > bs->MaxLength)
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    return MFX_ERR_NONE;
}



/* Check if pointers required for given FOURCC is NOT null */
inline
mfxStatus CheckFramePointers(mfxFrameInfo const& info, mfxFrameData const& data)
{
    switch (info.FourCC)
    {
        case MFX_FOURCC_A2RGB10:     MFX_CHECK(data.B, MFX_ERR_UNDEFINED_BEHAVIOR); break;

#if (MFX_VERSION >= 1027)
        case MFX_FOURCC_Y410:        MFX_CHECK(data.Y410, MFX_ERR_UNDEFINED_BEHAVIOR); break;

        case MFX_FOURCC_Y210:        MFX_CHECK(data.Y16 && data.U16 && data.V16, MFX_ERR_UNDEFINED_BEHAVIOR); break;
#endif

        case MFX_FOURCC_P8:
        case MFX_FOURCC_P8_TEXTURE:
        case MFX_FOURCC_R16:         MFX_CHECK(data.Y, MFX_ERR_UNDEFINED_BEHAVIOR); break;

        case MFX_FOURCC_NV12:
        case MFX_FOURCC_NV16:
        case MFX_FOURCC_P010:
#if (MFX_VERSION >= 1031)
        case MFX_FOURCC_P016:
#endif
        case MFX_FOURCC_P210:        MFX_CHECK(data.Y && data.UV, MFX_ERR_UNDEFINED_BEHAVIOR); break;

#if (MFX_VERSION >= 1031)
        case MFX_FOURCC_Y216:        MFX_CHECK(data.Y16 && data.U16 && data.V16, MFX_ERR_UNDEFINED_BEHAVIOR); break;
        case MFX_FOURCC_Y416:        MFX_CHECK(data.Y16 && data.U16 && data.V16 && data.A , MFX_ERR_UNDEFINED_BEHAVIOR); break;
#endif
#if defined (MFX_ENABLE_FOURCC_RGB565)
        case MFX_FOURCC_RGB565:      MFX_CHECK(data.R && data.G && data.B, MFX_ERR_UNDEFINED_BEHAVIOR); break;
#endif // MFX_ENABLE_FOURCC_RGB565
#ifdef MFX_ENABLE_RGBP
        case MFX_FOURCC_RGBP:
#endif
        case MFX_FOURCC_RGB3:        MFX_CHECK(data.R && data.G && data.B, MFX_ERR_UNDEFINED_BEHAVIOR); break;

        case MFX_FOURCC_AYUV:
        case MFX_FOURCC_AYUV_RGB4:
        case MFX_FOURCC_RGB4:
        case MFX_FOURCC_BGR4:
        case MFX_FOURCC_ARGB16:
        case MFX_FOURCC_ABGR16:      MFX_CHECK(data.R && data.G && data.B && data.A, MFX_ERR_UNDEFINED_BEHAVIOR); break;

        case MFX_FOURCC_YV12:
        case MFX_FOURCC_YUY2:
        default:                     MFX_CHECK(data.Y && data.U && data.V, MFX_ERR_UNDEFINED_BEHAVIOR); break;
    }

    return MFX_ERR_NONE;
}


mfxStatus CheckFrameData(const mfxFrameSurface1 *surface)
{
    if (!surface)
        return MFX_ERR_NULL_PTR;

    if (surface->Data.MemId)
        return MFX_ERR_NONE;

    mfxStatus sts;
    return
        sts = CheckFramePointers(surface->Info, surface->Data);
}

mfxStatus CheckDecodersExtendedBuffers(mfxVideoParam const* par)
{
    static const mfxU32 g_commonSupportedExtBuffers[]       = {
                                                               MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION,
                                                               MFX_EXTBUFF_DEC_ADAPTIVE_PLAYBACK,
    };

    static const mfxU32 g_decoderSupportedExtBuffersAVC[]   = {
                                                               MFX_EXTBUFF_MVC_SEQ_DESC,
                                                               MFX_EXTBUFF_MVC_TARGET_VIEWS,
                                                               MFX_EXTBUFF_DEC_VIDEO_PROCESSING,
                                                               MFX_EXTBUFF_FEI_PARAM};

    static const mfxU32 g_decoderSupportedExtBuffersHEVC[]  = {
                                                               MFX_EXTBUFF_HEVC_PARAM,
	                                                         MFX_EXTBUFF_DEC_VIDEO_PROCESSING
                                                               };

    static const mfxU32 g_decoderSupportedExtBuffersVC1[]   = {MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION,
                                                               };


    static const mfxU32 g_decoderSupportedExtBuffersMJPEG[] = {MFX_EXTBUFF_JPEG_HUFFMAN,
                                                               MFX_EXTBUFF_DEC_VIDEO_PROCESSING,
                                                               MFX_EXTBUFF_JPEG_QT};

    const mfxU32 *supported_buffers = 0;
    mfxU32 numberOfSupported = 0;

    if (par->mfx.CodecId == MFX_CODEC_AVC)
    {
        supported_buffers = g_decoderSupportedExtBuffersAVC;
        numberOfSupported = sizeof(g_decoderSupportedExtBuffersAVC) / sizeof(g_decoderSupportedExtBuffersAVC[0]);
    }
    else if (par->mfx.CodecId == MFX_CODEC_VC1 || par->mfx.CodecId == MFX_CODEC_MPEG2)
    {
        supported_buffers = g_decoderSupportedExtBuffersVC1;
        numberOfSupported = sizeof(g_decoderSupportedExtBuffersVC1) / sizeof(g_decoderSupportedExtBuffersVC1[0]);
    }
    else if (par->mfx.CodecId == MFX_CODEC_HEVC)
    {
        supported_buffers = g_decoderSupportedExtBuffersHEVC;
        numberOfSupported = sizeof(g_decoderSupportedExtBuffersHEVC) / sizeof(g_decoderSupportedExtBuffersHEVC[0]);
    }
    else if (par->mfx.CodecId == MFX_CODEC_JPEG)
    {
        supported_buffers = g_decoderSupportedExtBuffersMJPEG;
        numberOfSupported = sizeof(g_decoderSupportedExtBuffersMJPEG) / sizeof(g_decoderSupportedExtBuffersMJPEG[0]);
    }
    else
    {
        supported_buffers = g_commonSupportedExtBuffers;
        numberOfSupported = sizeof(g_commonSupportedExtBuffers) / sizeof(g_commonSupportedExtBuffers[0]);
    }

    if (!supported_buffers)
        return MFX_ERR_NONE;

    const mfxU32 *common_supported_buffers = g_commonSupportedExtBuffers;
    mfxU32 common_numberOfSupported = sizeof(g_commonSupportedExtBuffers) / sizeof(g_commonSupportedExtBuffers[0]);

    for (mfxU32 i = 0; i < par->NumExtParam; i++)
    {
        if (par->ExtParam[i] == NULL)
        {
           return MFX_ERR_NULL_PTR;
        }

        bool is_known = false;
        for (mfxU32 j = 0; j < numberOfSupported; ++j)
        {
            if (par->ExtParam[i]->BufferId == supported_buffers[j])
            {
                is_known = true;
                break;
            }
        }

        for (mfxU32 j = 0; j < common_numberOfSupported; ++j)
        {
            if (par->ExtParam[i]->BufferId == common_supported_buffers[j])
            {
                is_known = true;
                break;
            }
        }

        if (!is_known)
        {
            return MFX_ERR_UNSUPPORTED;
        }
    }

    return MFX_ERR_NONE;
}

// converts u32 nom and denom to packed u16+u16, used in va
mfxStatus PackMfxFrameRate(mfxU32 nom, mfxU32 den, mfxU32& packed)
{
    mfxStatus sts = MFX_ERR_NONE;
    if (!nom)
    {
        packed = 0;
        return sts;
    }

    if (!den) // denominator assumed 1 if is 0
        den = 1;

    if ((nom | den) >> 16) // don't fit to u16
    {
        mfxU32 gcd = nom; // will be greatest common divisor
        mfxU32 rem = den;
        while (rem > 0)
        {
            mfxU32 oldrem = rem;
            rem = gcd % rem;
            gcd = oldrem;
        }
        if (gcd > 1)
        {
            nom /= gcd;
            den /= gcd;
        }
        if ((nom | den) >> 16) // still don't fit to u16 - lose precision
        {
            if (nom > den) // make nom 0xffff for max precision
            {
                den = (mfxU32)((mfxF64)den * 0xffff / nom + .5);
                if (!den)
                    den = 1;
                nom = 0xffff;
            }
            else
            {
                nom = (mfxU32)((mfxF64)nom * 0xffff / den + .5);
                den = 0xffff;
            }
            sts = MFX_WRN_VIDEO_PARAM_CHANGED;
        }
    }
    packed = (den << 16) | nom;
    return sts;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Extended Buffer class
//////////////////////////////////////////////////////////////////////////////////////////////////////////
ExtendedBuffer::ExtendedBuffer()
{
}

ExtendedBuffer::~ExtendedBuffer()
{
    Release();
}

void ExtendedBuffer::Release()
{
    if(m_buffers.empty())
        return;

    BuffersList::iterator iter = m_buffers.begin();
    BuffersList::iterator iter_end = m_buffers.end();

    for( ; iter != iter_end; ++iter)
    {
        mfxU8 * buffer = (mfxU8 *)*iter;
        delete[] buffer;
    }

    m_buffers.clear();
}

size_t ExtendedBuffer::GetCount() const
{
    return m_buffers.size();
}

void ExtendedBuffer::AddBuffer(mfxExtBuffer * in)
{
    if (GetBufferByIdInternal(in->BufferId))
        return;

    mfxExtBuffer * buffer = (mfxExtBuffer *)(new mfxU8[in->BufferSz]);
    memset(buffer, 0, in->BufferSz);
    buffer->BufferSz = in->BufferSz;
    buffer->BufferId = in->BufferId;
    AddBufferInternal(buffer);
}

void ExtendedBuffer::AddBufferInternal(mfxExtBuffer * buffer)
{
    m_buffers.push_back(buffer);
}

mfxExtBuffer ** ExtendedBuffer::GetBuffers()
{
    return &m_buffers[0];
}

mfxExtBuffer * ExtendedBuffer::GetBufferByIdInternal(mfxU32 id)
{
    BuffersList::iterator iter = m_buffers.begin();
    BuffersList::iterator iter_end = m_buffers.end();

    for( ; iter != iter_end; ++iter)
    {
        mfxExtBuffer * buffer = *iter;
        if (buffer->BufferId == id)
            return buffer;
    }

    return 0;
}

mfxExtBuffer * ExtendedBuffer::GetBufferByPositionInternal(mfxU32 pos)
{
    if (pos >= m_buffers.size())
        return 0;

    return m_buffers[pos];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  mfxVideoParamWrapper implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////
mfxVideoParamWrapper::mfxVideoParamWrapper()
    : m_mvcSequenceBuffer(0)
{
}

mfxVideoParamWrapper::mfxVideoParamWrapper(const mfxVideoParam & par)
    : m_mvcSequenceBuffer(0)
{
    CopyVideoParam(par);
}

mfxVideoParamWrapper::~mfxVideoParamWrapper()
{
    delete[] m_mvcSequenceBuffer;
}

mfxVideoParamWrapper & mfxVideoParamWrapper::operator = (const mfxVideoParam & par)
{
    CopyVideoParam(par);
    return *this;
}

mfxVideoParamWrapper & mfxVideoParamWrapper::operator = (const mfxVideoParamWrapper & par)
{
    return mfxVideoParamWrapper::operator = ( *((const mfxVideoParam *)&par));
}

bool mfxVideoParamWrapper::CreateExtendedBuffer(mfxU32 bufferId)
{
    if (m_buffers.GetBufferById<void *>(bufferId))
        return true;

    switch(bufferId)
    {
    case MFX_EXTBUFF_VIDEO_SIGNAL_INFO:
        m_buffers.AddTypedBuffer<mfxExtVideoSignalInfo>(bufferId);
        break;

    case MFX_EXTBUFF_CODING_OPTION_SPSPPS:
        m_buffers.AddTypedBuffer<mfxExtCodingOptionSPSPPS>(bufferId);
        break;

    case MFX_EXTBUFF_HEVC_PARAM:
        m_buffers.AddTypedBuffer<mfxExtHEVCParam>(bufferId);
        break;

    default:
        VM_ASSERT(false);
        return false;
    }

    NumExtParam = (mfxU16)m_buffers.GetCount();
    ExtParam = NumExtParam ? m_buffers.GetBuffers() : 0;

    return true;
}

void mfxVideoParamWrapper::CopyVideoParam(const mfxVideoParam & par)
{
    mfxVideoParam * temp = this;
    *temp = par;

    this->NumExtParam = 0;
    this->ExtParam = 0;

    for (mfxU32 i = 0; i < par.NumExtParam; i++)
    {
        switch(par.ExtParam[i]->BufferId)
        {
        case MFX_EXTBUFF_DEC_VIDEO_PROCESSING:
        case MFX_EXTBUFF_MVC_TARGET_VIEWS:
        case MFX_EXTBUFF_VIDEO_SIGNAL_INFO:
        case MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION:
        case MFX_EXTBUFF_DEC_ADAPTIVE_PLAYBACK:
        case MFX_EXTBUFF_JPEG_QT:
        case MFX_EXTBUFF_JPEG_HUFFMAN:
        case MFX_EXTBUFF_HEVC_PARAM:
        case MFX_EXTBUFF_FEI_PARAM:
            {
                void * in = GetExtendedBufferInternal(par.ExtParam, par.NumExtParam, par.ExtParam[i]->BufferId);
                m_buffers.AddBuffer(par.ExtParam[i]);
                mfxExtBuffer * out = m_buffers.GetBufferById<mfxExtBuffer >(par.ExtParam[i]->BufferId);
                if (NULL == out)
                {
                    VM_ASSERT(false);
                    throw UMC::UMC_ERR_FAILED;
                }

                mfxU8 *src = reinterpret_cast<mfxU8*>(in), *dst = reinterpret_cast<mfxU8*>(out);
                std::copy(src, src + par.ExtParam[i]->BufferSz, dst);
            }
            break;
        case MFX_EXTBUFF_MVC_SEQ_DESC:
            {
                mfxExtMVCSeqDesc * mvcPoints = (mfxExtMVCSeqDesc *)GetExtendedBufferInternal(par.ExtParam, par.NumExtParam, MFX_EXTBUFF_MVC_SEQ_DESC);

                m_buffers.AddTypedBuffer<mfxExtMVCSeqDesc>(MFX_EXTBUFF_MVC_SEQ_DESC);
                mfxExtMVCSeqDesc * points = m_buffers.GetBufferById<mfxExtMVCSeqDesc>(MFX_EXTBUFF_MVC_SEQ_DESC);

                size_t size = mvcPoints->NumView * sizeof(mfxMVCViewDependency) + mvcPoints->NumOP * sizeof(mfxMVCOperationPoint) + mvcPoints->NumViewId * sizeof(mfxU16);
                if (!m_mvcSequenceBuffer)
                    m_mvcSequenceBuffer = new mfxU8[size];
                else
                {
                    delete[] m_mvcSequenceBuffer;
                    m_mvcSequenceBuffer = new mfxU8[size];
                }

                mfxU8 * ptr = m_mvcSequenceBuffer;

                if (points)
                {
                    points->NumView = points->NumViewAlloc = mvcPoints->NumView;
                    points->View = (mfxMVCViewDependency * )ptr;
                    std::copy(mvcPoints->View, mvcPoints->View + mvcPoints->NumView, points->View);
                    ptr += mvcPoints->NumView * sizeof(mfxMVCViewDependency);

                    points->NumView = points->NumViewAlloc = mvcPoints->NumView;
                    points->ViewId = (mfxU16 *)ptr;
                    std::copy(mvcPoints->ViewId, mvcPoints->ViewId + mvcPoints->NumViewId, points->ViewId);
                    ptr += mvcPoints->NumViewId * sizeof(mfxU16);

                    points->NumOP = points->NumOPAlloc = mvcPoints->NumOP;
                    points->OP = (mfxMVCOperationPoint *)ptr;
                    std::copy(mvcPoints->OP, mvcPoints->OP + mvcPoints->NumOP, points->OP);

                    mfxU16 * targetView = points->ViewId;
                    for (mfxU32 j = 0; j < points->NumOP; j++)
                    {
                        points->OP[j].TargetViewId = targetView;
                        targetView += points->OP[j].NumTargetViews;
                    }
                }
            }
            break;

        case MFX_EXTBUFF_CODING_OPTION_SPSPPS:
            break;

        default:
            {
                VM_ASSERT(false);
                throw UMC::UMC_ERR_FAILED;
            }
            break;
        };
    }

    NumExtParam = (mfxU16)m_buffers.GetCount();
    ExtParam = NumExtParam ? m_buffers.GetBuffers() : 0;
}

inline
mfxU32 GetMinPitch(mfxU32 fourcc, mfxU16 width)
{
    switch (fourcc)
    {
        case MFX_FOURCC_P8:
        case MFX_FOURCC_P8_TEXTURE:
        case MFX_FOURCC_NV12:
        case MFX_FOURCC_YV12:
#ifdef MFX_ENABLE_RGBP
        case MFX_FOURCC_RGBP:
#endif
        case MFX_FOURCC_NV16:        return width * 1;
#if defined (MFX_ENABLE_FOURCC_RGB565)
        case MFX_FOURCC_RGB565:      return width * 2;
#endif // MFX_ENABLE_FOURCC_RGB565
        case MFX_FOURCC_R16:         return width * 2;

        case MFX_FOURCC_RGB3:        return width * 3;

        case MFX_FOURCC_AYUV:
        case MFX_FOURCC_AYUV_RGB4:
        case MFX_FOURCC_RGB4:
        case MFX_FOURCC_BGR4:
        case MFX_FOURCC_A2RGB10:     return width * 4;

        case MFX_FOURCC_ARGB16:
        case MFX_FOURCC_ABGR16:      return width * 8;

        case MFX_FOURCC_YUY2:
        case MFX_FOURCC_UYVY:        return width * 2;

        case MFX_FOURCC_P010:
#if (MFX_VERSION >= 1031)
        case MFX_FOURCC_P016:
#endif
        case MFX_FOURCC_P210:        return width * 2;

#if (MFX_VERSION >= 1027)
        case MFX_FOURCC_Y210:
        case MFX_FOURCC_Y410:        return width * 4;
#endif

#if (MFX_VERSION >= 1031)
        case MFX_FOURCC_Y216:        return width * 4;
        case MFX_FOURCC_Y416:        return width * 8;
#endif
    }

    return 0;
}

mfxU8* GetFramePointer(mfxU32 fourcc, mfxFrameData const& data)
{
    switch (fourcc)
    {
        case MFX_FOURCC_RGB3:
        case MFX_FOURCC_RGB4:
        case MFX_FOURCC_BGR4:
#ifdef MFX_ENABLE_RGBP
        case MFX_FOURCC_RGBP:
#endif
        case MFX_FOURCC_ARGB16:
        case MFX_FOURCC_ABGR16:      return std::min({data.R, data.G, data.B}); break;
#if defined (MFX_ENABLE_FOURCC_RGB565)
        case MFX_FOURCC_RGB565:      return data.R; break;
#endif // MFX_ENABLE_FOURCC_RGB565
        case MFX_FOURCC_R16:         return reinterpret_cast<mfxU8*>(data.Y16); break;

        case MFX_FOURCC_AYUV:        return data.V; break;

        case MFX_FOURCC_UYVY:        return data.U; break;

        case MFX_FOURCC_A2RGB10:     return data.B; break;

#if (MFX_VERSION >= 1027)
        case MFX_FOURCC_Y410:        return reinterpret_cast<mfxU8*>(data.Y410); break;
#endif

#if (MFX_VERSION >= 1031)
        case MFX_FOURCC_Y416:        return reinterpret_cast<mfxU8*>(data.U16); break;
#endif

        default:                     return data.Y;
    }
}

mfxStatus GetFramePointerChecked(mfxFrameInfo const& info, mfxFrameData const& data, mfxU8** ptr)
{
    MFX_CHECK(ptr, MFX_ERR_UNDEFINED_BEHAVIOR);

    *ptr = GetFramePointer(info.FourCC, data);
    if (!*ptr)
        return MFX_ERR_NONE;

    mfxStatus sts = CheckFramePointers(info, data);
    MFX_CHECK_STS(sts);

    mfxU32 const pitch = (data.PitchHigh << 16) | data.PitchLow;
    mfxU32 const min_pitch = GetMinPitch(info.FourCC, info.Width);

    return
        !min_pitch || pitch < min_pitch ? MFX_ERR_UNDEFINED_BEHAVIOR : MFX_ERR_NONE;
}
