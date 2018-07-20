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

#include "mfx_common_decode_int.h"
#include "mfx_enc_common.h"

#include "umc_va_base.h"
#include "umc_defs.h"

MFXMediaDataAdapter::MFXMediaDataAdapter(mfxBitstream *pBitstream)
{
    Load(pBitstream);
}

void MFXMediaDataAdapter::Load(mfxBitstream *pBitstream)
{
    if (!pBitstream)
        return;

    SetBufferPointer(pBitstream->Data, pBitstream->DataOffset + pBitstream->DataLength);
    SetDataSize(pBitstream->DataOffset + pBitstream->DataLength);
    MoveDataPointer(pBitstream->DataOffset);
    SetTime(GetUmcTimeStamp(pBitstream->TimeStamp));

    SetFlags(0);

    if (pBitstream->DataFlag & MFX_BITSTREAM_EOS)
    {
        SetFlags(UMC::MediaData::FLAG_VIDEO_DATA_END_OF_STREAM);
    }
    else
    {
        if (!(pBitstream->DataFlag & MFX_BITSTREAM_COMPLETE_FRAME))
        {
            SetFlags(UMC::MediaData::FLAG_VIDEO_DATA_NOT_FULL_UNIT | UMC::MediaData::FLAG_VIDEO_DATA_NOT_FULL_FRAME);
        }
    }
}

void MFXMediaDataAdapter::Save(mfxBitstream *pBitstream)
{
    if (!pBitstream)
        return;

    pBitstream->DataOffset = (mfxU32)((mfxU8*)GetDataPointer() - (mfxU8*)GetBufferPointer());
    pBitstream->DataLength = (mfxU32)GetDataSize();
}

void MFXMediaDataAdapter::SetExtBuffer(mfxExtBuffer* extbuf)
{
    if (extbuf)
        SetAuxInfo(extbuf, extbuf->BufferSz, extbuf->BufferId);
}

mfxStatus ConvertUMCStatusToMfx(UMC::Status status)
{
    switch((UMC::eUMC_VA_Status)status)
    {
    case UMC::UMC_ERR_FRAME_LOCKED:
    case UMC::UMC_ERR_DEVICE_FAILED:
        return MFX_ERR_DEVICE_FAILED;
    case UMC::UMC_ERR_DEVICE_LOST:
        return MFX_ERR_DEVICE_LOST;
    }

    mfxStatus sts = MFX_ERR_NONE;

    switch (status)
    {
        case UMC::UMC_OK:
        case UMC::UMC_ERR_SYNC:
            sts = MFX_ERR_NONE;
            break;
        case UMC::UMC_ERR_NULL_PTR:
            sts = MFX_ERR_NULL_PTR;
            break;
        case UMC::UMC_ERR_NOT_ENOUGH_BUFFER:
            sts = MFX_ERR_NOT_ENOUGH_BUFFER;
            break;
        case UMC::UMC_ERR_NOT_IMPLEMENTED:
        case UMC::UMC_ERR_UNSUPPORTED:
            sts = MFX_ERR_UNSUPPORTED;
            break;
        case UMC::UMC_ERR_ALLOC:
            sts = MFX_ERR_MEMORY_ALLOC;
            break;
        case UMC::UMC_ERR_LOCK:
            sts = MFX_ERR_LOCK_MEMORY;
            break;
        case UMC::UMC_ERR_INIT:
        case UMC::UMC_ERR_INVALID_PARAMS:
        case UMC::UMC_ERR_INVALID_STREAM:
        case UMC::UMC_ERR_FAILED:
        case UMC::UMC_ERR_TIMEOUT:
            sts = MFX_ERR_UNDEFINED_BEHAVIOR;
            break;
        default :
            sts = MFX_ERR_UNKNOWN;
            break;
    }

    return sts;
}

void ConvertMFXParamsToUMC(mfxVideoParam const* par, UMC::VideoStreamInfo *umcVideoParams)
{
    umcVideoParams->clip_info.height = par->mfx.FrameInfo.Height;
    umcVideoParams->clip_info.width = par->mfx.FrameInfo.Width;

    umcVideoParams->disp_clip_info.height = umcVideoParams->clip_info.height;
    umcVideoParams->disp_clip_info.width = umcVideoParams->clip_info.width;

    if(par->mfx.CodecId == MFX_CODEC_JPEG && (MFX_ROTATION_90 == par->mfx.Rotation || MFX_ROTATION_270 == par->mfx.Rotation))
    {
        std::swap(umcVideoParams->clip_info.height, umcVideoParams->clip_info.width);
    }

    switch (par->mfx.FrameInfo.FourCC)
    {
    case MFX_FOURCC_RGB4:
        umcVideoParams->color_format = UMC::RGB32;
        break;
    case MFX_FOURCC_RGB3:
        umcVideoParams->color_format = UMC::RGB24;
        break;
    case MFX_FOURCC_YUY2:
        umcVideoParams->color_format = UMC::YUY2;
        break;
    case MFX_FOURCC_YV12:
        umcVideoParams->color_format = UMC::YV12;
        break;
    case MFX_FOURCC_P010:
        umcVideoParams->color_format = UMC::P010;
        break;
    case MFX_FOURCC_P210:
        umcVideoParams->color_format = UMC::P210;
        break;
#if (MFX_VERSION >= 1027)
    case MFX_FOURCC_Y210:
        umcVideoParams->color_format = UMC::Y210;
        break;
    case MFX_FOURCC_Y410:
        umcVideoParams->color_format = UMC::Y410;
        break;
#endif

    case MFX_FOURCC_AYUV:
        umcVideoParams->color_format = UMC::YUV444A;
        break;
    case MFX_FOURCC_IMC3:
    case MFX_FOURCC_YUV400:
    case MFX_FOURCC_YUV411:
    case MFX_FOURCC_YUV422H:
    case MFX_FOURCC_YUV422V:
    case MFX_FOURCC_YUV444:
    case MFX_FOURCC_RGBP:
        umcVideoParams->color_format = UMC::YUY2;
        break;

    case MFX_FOURCC_NV12:
    default:
        umcVideoParams->color_format = UMC::NV12;
        break;
    }

    umcVideoParams->interlace_type = UMC::PROGRESSIVE;

    if (par->mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_FIELD_BFF)
        umcVideoParams->interlace_type = UMC::INTERLEAVED_BOTTOM_FIELD_FIRST;

    if (par->mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_FIELD_TFF)
        umcVideoParams->interlace_type = UMC::INTERLEAVED_TOP_FIELD_FIRST;

    umcVideoParams->stream_subtype = UMC::UNDEF_VIDEO_SUBTYPE;
    umcVideoParams->stream_type = UMC::H264_VIDEO;

    umcVideoParams->framerate = (par->mfx.FrameInfo.FrameRateExtN && par->mfx.FrameInfo.FrameRateExtD)? (double)par->mfx.FrameInfo.FrameRateExtN / par->mfx.FrameInfo.FrameRateExtD: 0;

    umcVideoParams->profile = ExtractProfile(par->mfx.CodecProfile);
    umcVideoParams->level = par->mfx.CodecLevel;
}

void ConvertMFXParamsToUMC(mfxVideoParam const* par, UMC::VideoDecoderParams *umcVideoParams)
{
    ConvertMFXParamsToUMC(par, &umcVideoParams->info);

    umcVideoParams->numThreads = par->mfx.NumThread;

    switch(par->mfx.TimeStampCalc)
    {
    case MFX_TIMESTAMPCALC_TELECINE:
        umcVideoParams->lFlags |= UMC::FLAG_VDEC_TELECINE_PTS;
        break;

    case MFX_TIMESTAMPCALC_UNKNOWN:
        break;

    default:
        VM_ASSERT(false);
        break;
    }
}

mfxU32 ConvertUMCColorFormatToFOURCC(UMC::ColorFormat format)
{
    switch (format)
    {
        case UMC::NV12:    return MFX_FOURCC_NV12;
        case UMC::AYUV:    return MFX_FOURCC_AYUV;
        case UMC::RGB32:   return MFX_FOURCC_RGB4;
        case UMC::RGB24:   return MFX_FOURCC_RGB3;
        case UMC::YUY2:    return MFX_FOURCC_YUY2;
        case UMC::YV12:    return MFX_FOURCC_YV12;
        case UMC::P010:    return MFX_FOURCC_P010;
        case UMC::P210:    return MFX_FOURCC_P210;
#if (MFX_VERSION >= 1027)
        case UMC::Y210:    return MFX_FOURCC_Y210;
        case UMC::Y410:    return MFX_FOURCC_Y410;
#endif
        case UMC::YUV444A: return MFX_FOURCC_AYUV;
        case UMC::IMC3:    return MFX_FOURCC_IMC3;
        case UMC::YUV411:  return MFX_FOURCC_YUV411;
        case UMC::YUV444:  return MFX_FOURCC_YUV444;
        case UMC::UYVY:    return MFX_FOURCC_UYVY;

        default:
            VM_ASSERT(!"Unknown color format");
            return MFX_FOURCC_NV12;
    }
}

inline
mfxU32 ConvertUMCStreamTypeToCodec(UMC::VideoStreamType type)
{
    switch (type)
    {
        case UMC::MPEG2_VIDEO: return MFX_CODEC_MPEG2;
        case UMC::VC1_VIDEO:   return MFX_CODEC_VC1;
        case UMC::H264_VIDEO:  return MFX_CODEC_AVC;
        case UMC::HEVC_VIDEO:  return MFX_CODEC_HEVC;
        case UMC::VP9_VIDEO:   return MFX_CODEC_VP9;
        default:
            VM_ASSERT(!"Unknown stream type");
            return 0;
    }
}

void ConvertUMCParamsToMFX(UMC::VideoStreamInfo const* si, mfxVideoParam* par)
{
    par->mfx.CodecId      = ConvertUMCStreamTypeToCodec(si->stream_type);
    par->mfx.CodecProfile = mfxU16(si->profile);
    par->mfx.CodecLevel   = mfxU16(si->level);

    par->mfx.FrameInfo.Height = UMC::align_value<mfxU16>(si->clip_info.height, 16);
    par->mfx.FrameInfo.Width  = UMC::align_value<mfxU16>(si->clip_info.width,  16);

    par->mfx.FrameInfo.CropX  = par->mfx.FrameInfo.CropY = 0;
    par->mfx.FrameInfo.CropH  = mfxU16(si->disp_clip_info.height);
    par->mfx.FrameInfo.CropW  = mfxU16(si->disp_clip_info.width);

    par->mfx.FrameInfo.BitDepthLuma = 0;
        par->mfx.FrameInfo.BitDepthChroma = 0;

    par->mfx.FrameInfo.FourCC = ConvertUMCColorFormatToFOURCC(si->color_format);

    switch (si->interlace_type)
    {
        case UMC::PROGRESSIVE:
            par->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
            break;

        case UMC::INTERLEAVED_BOTTOM_FIELD_FIRST:
            par->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_FIELD_BFF;
            break;

        case UMC::INTERLEAVED_TOP_FIELD_FIRST:
            par->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_FIELD_TFF;
            break;

        default:
            par->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_UNKNOWN;
    }

    //TODO: si->framerate
    par->mfx.FrameInfo.FrameRateExtN = par->mfx.FrameInfo.FrameRateExtD = 1;

    par->mfx.FrameInfo.AspectRatioW =  mfxU16(si->aspect_ratio_width);
    par->mfx.FrameInfo.AspectRatioH =  mfxU16(si->aspect_ratio_height);
}

void ConvertUMCParamsToMFX(UMC::VideoDecoderParams const* vp, mfxVideoParam* par)
{
    ConvertUMCParamsToMFX(&vp->info, par);

    par->mfx.NumThread = mfxU16(vp->numThreads);
    par->mfx.TimeStampCalc = mfxU16(
        vp->lFlags & UMC::FLAG_VDEC_TELECINE_PTS ?
        MFX_TIMESTAMPCALC_TELECINE : MFX_TIMESTAMPCALC_UNKNOWN
    );
}

bool IsNeedChangeVideoParam(mfxVideoParam *)
{
    return false;
}

void RefCounter::IncrementReference() const
{
    m_refCounter++;
}

void RefCounter::DecrementReference()
{
    m_refCounter--;

    VM_ASSERT(m_refCounter >= 0);
    if (!m_refCounter)
    {
        Free();
    }
}

mfxU16 FourCcBitDepth(mfxU32 fourCC)
{
    mfxU16 bitDepth = 0;

    switch (fourCC)
    {
    case MFX_FOURCC_NV12:
    case MFX_FOURCC_NV16:
    case MFX_FOURCC_YUY2:
    case MFX_FOURCC_AYUV:
        bitDepth = 8;
        break;

    case MFX_FOURCC_P010:
    case MFX_FOURCC_P210:
#if (MFX_VERSION >= 1027)
    case MFX_FOURCC_Y210:
    case MFX_FOURCC_Y410:
#endif
        bitDepth = 10;
        break;

    default:
        bitDepth = 0;
    }

    return bitDepth;
}

bool InitBitDepthFields(mfxFrameInfo *info)
{
    if (info->BitDepthLuma == 0)
    {
        info->BitDepthLuma = FourCcBitDepth(info->FourCC);
    }

    if (info->BitDepthChroma == 0)
    {
        info->BitDepthChroma = info->BitDepthLuma;
    }

    return ((info->BitDepthLuma != 0) &&
        (info->BitDepthChroma != 0));
}
