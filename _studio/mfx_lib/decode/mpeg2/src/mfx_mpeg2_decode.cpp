// Copyright (c) 2017-2018 Intel Corporation
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
#ifdef MFX_ENABLE_MPEG2_VIDEO_DECODE

#include "mfx_common_decode_int.h"
#include "mfx_mpeg2_decode.h"
#include "mfx_mpeg2_dec_common.h"
#include "mfx_enc_common.h"
#include "mfx_ext_buffers.h"

#include "vm_sys_info.h"



#include "umc_mpeg2_dec_hw.h"

enum { MFX_MB_WIDTH = 16 };
enum { MFX_MB_HEIGHT = 16 };

//#define _status_report_debug
//#define _threading_deb


#define THREAD_DEBUG_PRINTF(...)
#define THREAD_DEBUG_PRINTF__HOLDING_MUTEX(_guard, ...)

#define STATUS_REPORT_DEBUG_PRINTF(...)

enum
{
    ePIC   = 0x00,
    eUSER  = 0xb2,
    eSEQ   = 0xb3,
    eEXT   = 0xb5,
    eEND   = 0xb7,
    eGROUP = 0xb8
};

bool IsHWSupported(VideoCORE *pCore, mfxVideoParam *par)
{
    if((par->mfx.CodecProfile == MFX_PROFILE_MPEG1 && pCore->GetPlatformType()== MFX_PLATFORM_HARDWARE))
    {
        return false;
    }

#if defined (MFX_VA_LINUX)
    if (MFX_ERR_NONE != pCore->IsGuidSupported(sDXVA2_ModeMPEG2_VLD, par))
    {
        return false;
    }
#endif

    return true;
}

mfxU8 GetMfxCodecLevel(mfxI32 level)
{
    switch (level)
    {
        case MPEG2_LEVEL_LOW:  return MFX_LEVEL_MPEG2_LOW;
        case MPEG2_LEVEL_MAIN: return MFX_LEVEL_MPEG2_MAIN;
        case MPEG2_LEVEL_H14:  return MFX_LEVEL_MPEG2_HIGH1440;
        case MPEG2_LEVEL_HIGH: return MFX_LEVEL_MPEG2_HIGH;
        default:               return MFX_LEVEL_UNKNOWN;
    }
}

mfxU8 GetMfxCodecProfile(mfxI32 profile)
{
    switch (profile)
    {
        case MPEG2_PROFILE_SIMPLE:  return MFX_PROFILE_MPEG2_SIMPLE;
        case MPEG2_PROFILE_MAIN:    return MFX_PROFILE_MPEG2_MAIN;
        case MPEG2_PROFILE_HIGH:    return MFX_PROFILE_MPEG2_HIGH;
        default:                    return MFX_PROFILE_UNKNOWN;
    }
}

mfxU16 GetMfxPicStruct(mfxU32 progressiveSequence, mfxU32 progressiveFrame, mfxU32 topFieldFirst, mfxU32 repeatFirstField, mfxU32 pictureStructure, mfxU16 extendedPicStruct)
{
    mfxU16 picStruct = MFX_PICSTRUCT_UNKNOWN;

    if (1 == progressiveSequence)
    {
        picStruct = MFX_PICSTRUCT_PROGRESSIVE;
    }
    else // interlace sequence
    {
        picStruct |= ((topFieldFirst) ? MFX_PICSTRUCT_FIELD_TFF : MFX_PICSTRUCT_FIELD_BFF);

        if (0 == topFieldFirst && 1 == pictureStructure)
        {
            picStruct = MFX_PICSTRUCT_FIELD_TFF;
        }

        if (progressiveFrame)
        {
            picStruct |= MFX_PICSTRUCT_PROGRESSIVE;
        }
    }

    if (progressiveSequence == 1 || progressiveFrame == 1)
        if (repeatFirstField)
        {
            if (0 == progressiveSequence)
            {
                picStruct |= MFX_PICSTRUCT_FIELD_REPEATED;
            }
            else // progressive sequence
            {
                if (topFieldFirst)
                {
                    picStruct |= MFX_PICSTRUCT_FRAME_TRIPLING;
                }
                else
                {
                    picStruct |= MFX_PICSTRUCT_FRAME_DOUBLING;
                }
            }
        }

    if (0 == extendedPicStruct)
    {
        // cut decorative flags
        if (MFX_PICSTRUCT_PROGRESSIVE & picStruct)
        {
            picStruct = MFX_PICSTRUCT_PROGRESSIVE;
        }
        else if (MFX_PICSTRUCT_FIELD_TFF & picStruct)
        {
            picStruct = MFX_PICSTRUCT_FIELD_TFF;
        }
        else
        {
            picStruct = MFX_PICSTRUCT_FIELD_BFF;
        }
    }

    return picStruct;
}

void SetSurfaceTimeCode(mfxFrameSurface1* surface, int32_t display_index, UMC::MPEG2VideoDecoderBase *implUmc)
{
    if (!surface || !implUmc)
        return;

    mfxExtTimeCode* pExtTimeCode = (mfxExtTimeCode *)GetExtendedBuffer(surface->Data.ExtParam, surface->Data.NumExtParam, MFX_EXTBUFF_TIME_CODE);
    if (!pExtTimeCode)
        return;

    const UMC::sPictureHeader& ph = implUmc->GetPictureHeader(display_index);
    pExtTimeCode->DropFrameFlag    = ph.time_code.gop_drop_frame_flag;
    pExtTimeCode->TimeCodePictures = ph.time_code.gop_picture;
    pExtTimeCode->TimeCodeHours    = ph.time_code.gop_hours;
    pExtTimeCode->TimeCodeMinutes  = ph.time_code.gop_minutes;
    pExtTimeCode->TimeCodeSeconds  = ph.time_code.gop_seconds;
}

mfxU16 GetMfxPictureType(mfxU32 frameType)
{
    switch (frameType)
    {
        case I_PICTURE: return MFX_FRAMETYPE_I | MFX_FRAMETYPE_REF;
        case P_PICTURE: return MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF;
        case B_PICTURE: return MFX_FRAMETYPE_B;
        default:        return MFX_FRAMETYPE_UNKNOWN;
    }
}

mfxU16 GetMfxFieldType(mfxU32 frameType)
{
    switch (frameType)
    {
        case I_PICTURE: return MFX_FRAMETYPE_xI | MFX_FRAMETYPE_xREF;
        case P_PICTURE: return MFX_FRAMETYPE_xP | MFX_FRAMETYPE_xREF;
        case B_PICTURE: return MFX_FRAMETYPE_xB;
        default:        return MFX_FRAMETYPE_UNKNOWN;
    }
}

void SetSurfacePictureType(mfxFrameSurface1* surface, int32_t display_index, UMC::MPEG2VideoDecoderBase *implUmc)
{
    if (!surface || !implUmc)
        return;

    mfxExtDecodedFrameInfo* pDecodedFrameInfoExt = (mfxExtDecodedFrameInfo*)GetExtendedBuffer(surface->Data.ExtParam, surface->Data.NumExtParam, MFX_EXTBUFF_DECODED_FRAME_INFO);
    if (!pDecodedFrameInfoExt)
        return;

    const UMC::sPictureHeader& ph = implUmc->GetPictureHeader(display_index);
    const UMC::sSequenceHeader& sh = implUmc->GetSequenceHeader();

    mfxU32 frameType = implUmc->GetFrameType(display_index);
    pDecodedFrameInfoExt->FrameType = GetMfxPictureType(frameType);
    if (ph.first_in_sequence)
        pDecodedFrameInfoExt->FrameType |= MFX_FRAMETYPE_IDR;

    if (sh.progressive_sequence)
        return;

    if (ph.picture_structure == FRAME_PICTURE)
        pDecodedFrameInfoExt->FrameType |= GetMfxFieldType(frameType);
    else
        pDecodedFrameInfoExt->FrameType |= GetMfxFieldType(implUmc->GetFrameType(display_index+DPB));
}

inline
mfxU8 Mpeg2GetMfxChromaFormatFromUmcMpeg2(mfxU32 umcChromaFormat)
{
    switch (umcChromaFormat)
    {
        case 2: return MFX_CHROMAFORMAT_YUV420;
        case 4: return MFX_CHROMAFORMAT_YUV422;
        case 8: return MFX_CHROMAFORMAT_YUV444;
        default:return 0;
    }
}

void UpdateMfxVideoParam(mfxVideoParam& vPar, const UMC::sSequenceHeader& sh, const UMC::sPictureHeader& ph)
{
    vPar.mfx.CodecLevel = GetMfxCodecLevel(sh.level);
    vPar.mfx.CodecProfile = GetMfxCodecProfile(sh.profile);
  //  if(vPar.mfx.CodecProfile == MFX_PROFILE_UNKNOWN)
   //     vPar.mfx.CodecProfile = MFX_PROFILE_MPEG1;
    vPar.mfx.FrameInfo.AspectRatioW = sh.aspect_ratio_w;
    vPar.mfx.FrameInfo.AspectRatioH = sh.aspect_ratio_h;

    vPar.mfx.FrameInfo.CropW = (mfxU16)sh.width;
    vPar.mfx.FrameInfo.CropH = (mfxU16)sh.height;
    vPar.mfx.FrameInfo.CropX = 0;
    vPar.mfx.FrameInfo.CropY = 0;
    vPar.mfx.FrameInfo.Width = AlignValue(vPar.mfx.FrameInfo.CropW, MFX_MB_WIDTH);
    vPar.mfx.FrameInfo.Height = AlignValue(vPar.mfx.FrameInfo.CropH, MFX_MB_WIDTH);
    vPar.mfx.FrameInfo.ChromaFormat = Mpeg2GetMfxChromaFormatFromUmcMpeg2(sh.chroma_format);
    GetMfxFrameRate((mfxU8)sh.frame_rate_code, &vPar.mfx.FrameInfo.FrameRateExtN, &vPar.mfx.FrameInfo.FrameRateExtD);

    vPar.mfx.FrameInfo.PicStruct = GetMfxPicStruct(sh.progressive_sequence, ph.progressive_frame,
                                                   ph.top_field_first, ph.repeat_first_field, ph.picture_structure,
                                                   vPar.mfx.ExtendedPicStruct);
}

UMC::ColorFormat GetUmcColorFormat(mfxU8 colorFormat)
{
    switch (colorFormat)
    {
        case MFX_CHROMAFORMAT_YUV420: return UMC::YUV420;
        case MFX_CHROMAFORMAT_YUV422: return UMC::YUV422;
        case MFX_CHROMAFORMAT_YUV444: return UMC::YUV444;
        default: return UMC::NONE;
    }
}

static bool DARtoPAR(mfxI32 width, mfxI32 height, mfxI32 dar_h, mfxI32 dar_v,
                mfxI32 *par_h, mfxI32 *par_v)
{
  if(0 == width || 0 == height)
    return false;

  mfxI32 simple_tab[] = {2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59};
  mfxI32 i, denom;

  // suppose no overflow of 32s
  mfxI32 h = dar_h * height;
  mfxI32 v = dar_v * width;

  // remove common multipliers
  while( ((h|v)&1) == 0 )
  {
    h>>=1;
    v>>=1;
  }

  for(i=0;i<(mfxI32)(sizeof(simple_tab)/sizeof(simple_tab[0]));i++)
  {
    denom = simple_tab[i];
    while(h%denom==0 && v%denom==0) {
      v /= denom;
      h /= denom;
    }
    if(v<=denom || h<=denom)
      break;
  }
  *par_h = h;
  *par_v = v;
  return true;
}


static bool CalcAspectRatio(mfxI32 dar_code, mfxI32 width, mfxI32 height,
                                              mfxI32* AspectRatioW, mfxI32* AspectRatioH)
{
      bool ret = true;

      if(dar_code == 2)
      {
        ret = DARtoPAR(width, height, 4, 3, AspectRatioW, AspectRatioH);
      }
      else if(dar_code == 3)
      {
        ret = DARtoPAR(width, height, 16, 9, AspectRatioW, AspectRatioH);
      }
      else if(dar_code == 4)
      {
        ret = DARtoPAR(width, height, 221, 100, AspectRatioW, AspectRatioH);
      }
      else // dar_code == 1 or unknown
      {
        *AspectRatioW = 1;
        *AspectRatioH = 1;
      }
      return ret;
}

struct PicInfo {
    // from header
    mfxU16 temporal_reference = 0; // modulo 10 bit
    mfxU16 picture_type = 0;       // 1-I, 2-P, 3-B (table 6-12)
    mfxU16 picture_structure = 0;  // 1-TF, 2-BF, 3-Frame (table 6-14)
    bool top_ff = false;           // only if !prog.seq and frame structure
    bool repeat_ff = false;        // like top_ff
    // derived, assume interlace sequence
    mfxU16 first_polarity = 0;
    mfxU16 last_polarity = 0;
};

const mfxU8* FindAnyStartCode(const mfxU8* begin, const mfxU8* end);

static
bool GetPicInfo(PicInfo& picInfo, const mfxU8* begin, const mfxU8* end)
{
    const mfxU8* p = begin - 4;
find_pic_head:
    do {
        p = FindAnyStartCode(p + 4, end);
        if (p + 5 >= end)
            return false;
        if (p[3] >= 1 && p[3] <= 0xAF) // check to be not slices
            return false;
    } while (p[3] != ePIC);
    picInfo.temporal_reference = p[4] << 2 | p[5] >> 6;
    picInfo.picture_type = p[5] >> 3 & 7;
    if (picInfo.picture_type == 0 || picInfo.picture_type > 3)
        goto find_pic_head; // invalid data in picture header, find another

    p = FindAnyStartCode(p + 4, end);
    if (p + 7 >= end)
        return false; // need pic.struct from picture coding extension
    if (p[3] != eEXT || p[4] >> 4 != 8) {
        p -= 4; // must be picture coding ext, stream error. -=4 to process the header again
        goto find_pic_head;
    }
    picInfo.picture_structure = p[6] & 3;
    if (picInfo.picture_structure == 0)
        goto find_pic_head; // invalid pic.ext, find next pic.header

    picInfo.top_ff = ((p[7] & 0x80) != 0);
    picInfo.repeat_ff = ((p[7] & 2) != 0);
    picInfo.first_polarity = (picInfo.picture_structure == 3) ? (picInfo.top_ff ? 1 : 2) : picInfo.picture_structure;
    // field and frame&repeat both have last==first
    picInfo.last_polarity = (picInfo.picture_structure == 3 && !picInfo.repeat_ff) ? picInfo.first_polarity ^ 3 : picInfo.first_polarity;
    return true;
}

// out - picture to be decoded, head - start of the next picture, tail - end of available data
// secondField is related to the former of two
bool VideoDECODEMPEG2InternalBase::VerifyPictureBits(mfxBitstream* currPicBs, const mfxU8* head, const mfxU8* tail)
{
    // info for both pictures
    PicInfo picInfo[2];
    const mfxU8* begin = currPicBs->Data + currPicBs->DataOffset;
    const mfxU8* end = begin + currPicBs->DataLength;

    // get info for current picture, it is completely in the buffer
    if (!GetPicInfo(picInfo[0], begin, end))
        return false;

    bool progseq = ((m_implUmc->GetSequenceHeader()).progressive_sequence != 0);

    if (!dec_frame_count)
        m_fieldsInCurrFrame = 0;

    // analyze next picture header and pictures' sequence
    if (GetPicInfo(picInfo[1], head, tail)) {

        if (!m_fieldsInCurrFrame && // new frame to be either frame or paired fields, same type or IP pair
            picInfo[0].picture_structure != 3 &&
            (((picInfo[0].picture_structure ^ picInfo[1].picture_structure) != 3) ||
                (picInfo[0].picture_type != picInfo[1].picture_type &&
                picInfo[0].picture_type != I_PICTURE && picInfo[1].picture_type != P_PICTURE)))
            return false;

        if (picInfo[0].temporal_reference == picInfo[1].temporal_reference) {
            if (m_fieldsInCurrFrame || (picInfo[0].picture_structure ^ picInfo[1].picture_structure) != 3) { // not field pair: must be field, first field
                if (picInfo[1].picture_type != I_PICTURE) // each I can start new count
                {
                    m_fieldsInCurrFrame = 0;
                    return false;
                }
            } else {    // field pair: same type or IP
                if (picInfo[0].picture_type != picInfo[1].picture_type &&
                    picInfo[0].picture_type != I_PICTURE && picInfo[1].picture_type != P_PICTURE)
                    return false; // invalid pair, broken stream
            }
        }
        else if (((picInfo[0].temporal_reference + 1) & 0x3ff) == picInfo[1].temporal_reference) { // consequent in display and coding order
            if (!progseq && (picInfo[0].last_polarity ^ picInfo[1].first_polarity) != 3) // invalid field order
            {
                m_fieldsInCurrFrame = 0;
                return false;
            }
        }
    }

    m_fieldsInCurrFrame |= picInfo[0].picture_structure;
    if (m_fieldsInCurrFrame == 3)
        m_fieldsInCurrFrame = 0; // frame complete

    return true;
}

VideoDECODEMPEG2::VideoDECODEMPEG2(VideoCORE* core, mfxStatus *sts)
: VideoDECODE()
, m_pCore(core)
, m_isInitialized()
, m_isSWImpl()
, m_SkipLevel(SKIP_NONE)
{
    *sts = MFX_ERR_NONE;

    if (!m_pCore)
        *sts = MFX_ERR_NULL_PTR;
}

VideoDECODEMPEG2::~VideoDECODEMPEG2()
{
    Close();
}

mfxStatus VideoDECODEMPEG2::Init(mfxVideoParam *par)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_API, "VideoDECODEMPEG2::Init");
    if (m_isInitialized)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    MFX_CHECK_NULL_PTR1(par);

    mfxStatus mfxSts = MFX_ERR_NONE;

    eMFXHWType type = MFX_HW_UNKNOWN;

#if defined (MFX_VA_LINUX)
        type = m_pCore->GetHWType();
#endif

    mfxSts = CheckVideoParamDecoders(par, m_pCore->IsExternalFrameAllocator(), type);

    if (mfxSts != MFX_ERR_NONE)
    {
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    mfxU16 IOPattern = par->IOPattern;

#if defined (MFX_VA_LINUX)

        if (!IsHWSupported(m_pCore, par))
        {
            return MFX_ERR_UNSUPPORTED;
        }

        if(m_isSWImpl)
        {
        }
        else
        {
            internalImpl.reset(new VideoDECODEMPEG2Internal_HW);
        }

        if(IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY)
            internalImpl->m_isSWBuf = true;
#else
        IOPattern |= MFX_IOPATTERN_IN_SYSTEM_MEMORY;
        internalImpl.reset(new VideoDECODEMPEG2Internal_SW);
        m_isSWImpl = true;
#endif

    mfxSts = internalImpl->Init(par, m_pCore);
    if (mfxSts != MFX_ERR_NONE)
        return mfxSts;

    UMC::Status umcRes = internalImpl->m_implUmc->Init(&internalImpl->m_vdPar);
    MFX_CHECK_UMC_STS(umcRes);

    umcRes = internalImpl->m_implUmc->GetInfo(&internalImpl->m_vdPar);
    MFX_CHECK_UMC_STS(umcRes);

    if (MFX_PLATFORM_HARDWARE == m_pCore->GetPlatformType() && true == m_isSWImpl)
    {
        mfxSts = MFX_WRN_PARTIAL_ACCELERATION;
    }

    m_isInitialized = true;

    return mfxSts;

} // mfxStatus VideoDECODEMPEG2::Init(mfxVideoParam *par)

mfxStatus VideoDECODEMPEG2::Reset(mfxVideoParam *par)
{
    MFX_CHECK_NULL_PTR1(par);

    if (par->AsyncDepth != internalImpl->m_vPar.AsyncDepth)
    {
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    eMFXHWType type = m_pCore->GetHWType();

    mfxStatus mfxSts = CheckVideoParamDecoders(par, m_pCore->IsExternalFrameAllocator(), type);
    if (MFX_ERR_NONE != mfxSts)
    {
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    mfxExtOpaqueSurfaceAlloc *pOpqExt = (mfxExtOpaqueSurfaceAlloc *)GetExtendedBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
    if (pOpqExt)
    {
        if (false == internalImpl->IsOpaqueMemory())
        {
            // decoder was not initialized with opaque extended buffer
            return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;
        }

        if (internalImpl->allocRequest.NumFrameMin != pOpqExt->Out.NumSurface)
        {
            return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;
        }
    }

    m_SkipLevel = SKIP_NONE;

    mfxSts = internalImpl->Reset(par);
    MFX_CHECK_STS(mfxSts);

    internalImpl->m_reset_done = true;

    if (m_isSWImpl && m_pCore->GetPlatformType() == MFX_PLATFORM_HARDWARE)
    {
        return MFX_WRN_PARTIAL_ACCELERATION;
    }

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMPEG2::Close()
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_API, "VideoDECODEMPEG2::Close");
    // immediately return, object is not initialized
    if (false == m_isInitialized)
    {
        return MFX_ERR_NONE;
    }

    internalImpl->Close();
    m_isInitialized = false;

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMPEG2::GetDecodeStat(mfxDecodeStat *stat)
{
    MFX_CHECK_NULL_PTR1(stat);

    if (false == m_isInitialized)
    {
        return MFX_ERR_NOT_INITIALIZED;
    }

    stat->NumFrame = internalImpl->display_frame_count;
    stat->NumCachedFrame = internalImpl->cashed_frame_count;
    stat->NumSkippedFrame = internalImpl->skipped_frame_count;

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMPEG2::GetUserData(mfxU8 *ud, mfxU32 *sz, mfxU64 *ts, mfxU16 bufsize)
{
    MFX_CHECK_NULL_PTR3(ud, sz, ts);

    mfxF64 fts;
    UMC::Status umcSts = internalImpl->m_implUmc->GetCCData(ud, sz, fts, bufsize);

    if (UMC::UMC_OK == umcSts)
    {
        *ts = GetMfxTimeStamp(fts);

        return MFX_ERR_NONE;
    }

    *ts = 0;

    if (UMC::UMC_ERR_NOT_ENOUGH_BUFFER == umcSts)
    {
        return MFX_ERR_NOT_ENOUGH_BUFFER;
    }

    return MFX_ERR_UNDEFINED_BEHAVIOR;
}

mfxStatus VideoDECODEMPEG2::GetPayload( mfxU64 *ts, mfxPayload *payload )
{
    mfxStatus sts = MFX_ERR_NONE;

    if (false == m_isInitialized)
    {
        return MFX_ERR_NOT_INITIALIZED;
    }

    MFX_CHECK_NULL_PTR2( ts, payload);

    sts = GetUserData(payload->Data, &payload->NumBit, ts, payload->BufSize);
    MFX_CHECK_STS(sts);

    if(0 < payload->NumBit)
    {
        // user data start code type
        payload->Type = USER_DATA_START_CODE;
    }
    else
    {
        payload->Type = 0;
    }

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMPEG2::DecodeHeader(VideoCORE * /*core*/, mfxBitstream* bs, mfxVideoParam* par)
{
    MFX_CHECK_NULL_PTR3(bs,bs->Data,par);

    mfxU8* ptr;
    mfxU8* beg = bs->Data + bs->DataOffset;
    mfxU8* end = bs->Data + bs->DataOffset + bs->DataLength;

    bool find_seq_header = false;
    bool find_seq_ext    = false;
    bool find_seq_d_ext    = false;
    mfxI32 ShiftSH = 0;

    if (bs->MaxLength < bs->DataOffset)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    if(4 > bs->DataLength)
    {
        return MFX_ERR_MORE_DATA;
    }

    mfxU32 dar_code = 0;
    mfxU32 frame_rate_code = 0;
    mfxI32 frame_rate_extension_d = 0;
    mfxI32 frame_rate_extension_n = 0;
    par->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    int32_t W = 0, H = 0;

    mfxU8 *pShStart = NULL;
    mfxU8 *pShEnd = NULL;

    uint16_t video_format = 5; // unspecified video format
    uint16_t colour_primaries = 1;
    uint16_t transfer_characteristics = 1;
    uint16_t matrix_coefficients = 1;
    uint16_t color_description_present = 0;

    for (ptr = beg; ptr + 4 < end; ++ptr)
    {
        if (ptr[0] == 0 && ptr[1] == 0 && ptr[2] == 1 && ptr[3] == 0xb3)
        {
            // seq header already found
            if (find_seq_header)
                break;

            pShStart = ptr;

            if(ptr + 7 >= end)
                return MFX_ERR_MORE_DATA;

            ShiftSH = (mfxI32)(ptr - beg);
            par->mfx.CodecId = MFX_CODEC_MPEG2;
            par->mfx.FrameInfo.CropX = 0;
            par->mfx.FrameInfo.CropY = 0;
            par->mfx.FrameInfo.CropW = (ptr[4] << 4) + ((ptr[5] >> 4) & 0xf);
            par->mfx.FrameInfo.CropH = ((ptr[5] & 0xf) << 8) + ptr[6];

            frame_rate_code = ptr[7]  & 0xf;
            dar_code = ((ptr[7] >> 4) & 0xf);

            par->mfx.FrameInfo.Width = (par->mfx.FrameInfo.CropW + 15) & ~0x0f;
            par->mfx.FrameInfo.Height = (par->mfx.FrameInfo.CropH + 15) & ~0x0f;
            par->mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;

            if (par->mfx.FrameInfo.FourCC != MFX_FOURCC_YV12 &&
                par->mfx.FrameInfo.FourCC != MFX_FOURCC_NV12)
            {
                par->mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
            }

            find_seq_header = true;

            if(find_seq_ext && find_seq_d_ext)
                break;
        } // 0xb3 code
        else if (ptr[0] == 0 && ptr[1] == 0 && ptr[2] == 1 && ptr[3] == 0xb5)
        {
            // extensions section

            // get extension id
            uint32_t code = (ptr[4] & 0xf0);

            if (0x10 == code)
            {
                // sequence extension section
                if (find_seq_ext)
                    break;

                pShEnd = ptr;

                if (end <= ptr + 9)
                    return MFX_ERR_MORE_DATA;

                code = (ptr[4] << 24) | (ptr[5] << 16) | (ptr[6] << 8) | ptr[7];

                par->mfx.CodecProfile = GetMfxCodecProfile((code >> 24) & 0x7);
                par->mfx.CodecLevel = GetMfxCodecLevel((code >> 20) & 0xf);
                uint32_t progr = (code >> 19) & 1;
                progr != 0 ? par->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE:
                             par->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_UNKNOWN;

                int32_t ChromaFormat = 1 << ((code >> 17) & ((1 << 2) - 1));

                switch (ChromaFormat)
                {
                    case 2:
                        par->mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
                        break;
                    case 4:
                        par->mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV422;
                        break;
                    case 8:
                        par->mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV444;
                        break;
                    default:
                        break;
                }

                par->mfx.FrameInfo.CropW  = (mfxU16)((par->mfx.FrameInfo.CropW  & 0xfff)
                                                      | ((code >> (15-12)) & 0x3000));
                par->mfx.FrameInfo.CropH = (mfxU16)((par->mfx.FrameInfo.CropH & 0xfff)
                                                      | ((code >> (13-12)) & 0x3000));

                par->mfx.FrameInfo.Width = (par->mfx.FrameInfo.CropW + 15) & ~0x0f;
                par->mfx.FrameInfo.Height = (par->mfx.FrameInfo.CropH + 15) & ~0x0f;

                // 4k case
                if (0 == par->mfx.FrameInfo.CropW)
                {
                    par->mfx.FrameInfo.CropW = par->mfx.FrameInfo.Width;
                }

                if (0 == par->mfx.FrameInfo.CropH)
                {
                    par->mfx.FrameInfo.CropH = par->mfx.FrameInfo.Height;
                }

                code = (ptr[8] << 8) | ptr[9];
                frame_rate_extension_d = code & 31;
                frame_rate_extension_n = (code >> 5) & 3;

                find_seq_ext = true;

                if (true == find_seq_header && true == find_seq_d_ext)
                    break;
            }

            else if (0x20 == code)
            {
                // sequence display extension section
                video_format = (ptr[4] >> 1) & 0x07;
                uint8_t colour_description = ptr[4] & 0x01;
                color_description_present = colour_description;
                if (colour_description) // 4+24 bits skip
                {
                    if (ptr + 11 >= end)
                        return MFX_ERR_MORE_DATA;

                    colour_primaries = ptr[5];
                    transfer_characteristics = ptr[6];
                    matrix_coefficients = ptr[7];

                    code = ((ptr[8]) << 24) | (ptr[9] << 16) | (ptr[10] << 8) | ptr[11];//28 bit
                }
                else // 4 bit skip (28 bit)
                {
                    if (ptr + 8 >= end)
                        return MFX_ERR_MORE_DATA;

                    code = ((ptr[5]) << 24) | (ptr[6] << 16) | (ptr[7] << 8) | ptr[8];
                }

                W = (code >> 18);
                H = (code >> 3) & 0x00003fff;

                find_seq_d_ext = true;

                if(true == find_seq_header && true == find_seq_ext)
                    break;
            }
        }
        else if (IsMpeg2StartCodeEx(ptr) && find_seq_header)
            break;
    }
    if (find_seq_header)
    {
        // fill extension buffer

        mfxExtCodingOptionSPSPPS *spspps = (mfxExtCodingOptionSPSPPS *) GetExtendedBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_CODING_OPTION_SPSPPS);

        if (NULL != spspps)
        {
            mfxU16 size = 0;

            size = (mfxU16)(pShEnd - pShStart);

            if (find_seq_ext)
            {
                size += 10;
            }

            if (find_seq_d_ext)
            {
                if (color_description_present)
                    size += 12;
                else
                    size += 9;
            }

            std::copy(pShStart, pShStart + size, spspps->SPSBuffer);
            spspps->SPSBufSize = size;
        }

        mfxExtVideoSignalInfo *signalInfo = (mfxExtVideoSignalInfo *) GetExtendedBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_VIDEO_SIGNAL_INFO);

        if (NULL != signalInfo)
        {
            signalInfo->VideoFormat = video_format;
            signalInfo->ColourPrimaries = colour_primaries;
            signalInfo->TransferCharacteristics = transfer_characteristics;
            signalInfo->MatrixCoefficients = matrix_coefficients;
            signalInfo->ColourDescriptionPresent = color_description_present;
        }

        if(!find_seq_d_ext || W == 0 || H == 0)
        {
            W = (mfxI32) par->mfx.FrameInfo.CropW;
            H = (mfxI32) par->mfx.FrameInfo.CropH;
        }

        mfxI32 AspectRatioW, AspectRatioH;

        if(!CalcAspectRatio((mfxI32)dar_code,
            W, H,
            &AspectRatioW, &AspectRatioH))
            return MFX_ERR_UNSUPPORTED;

        par->mfx.FrameInfo.AspectRatioW = (mfxU16)AspectRatioW;
        par->mfx.FrameInfo.AspectRatioH = (mfxU16)AspectRatioH;

        bs->DataOffset += ShiftSH;
        bs->DataLength -= ShiftSH;
        GetMfxFrameRate((mfxU8)frame_rate_code, &par->mfx.FrameInfo.FrameRateExtN, &par->mfx.FrameInfo.FrameRateExtD);
        par->mfx.FrameInfo.FrameRateExtD *= frame_rate_extension_d + 1;
        par->mfx.FrameInfo.FrameRateExtN *= frame_rate_extension_n + 1;

        if (false == find_seq_ext)
        {
            par->mfx.CodecProfile = MFX_PROFILE_MPEG1;
            par->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
        }

        if (MFX_PICSTRUCT_PROGRESSIVE != par->mfx.FrameInfo.PicStruct)
        {
            mfxU16 h = (par->mfx.FrameInfo.CropH + 31) & ~(31);

            if (par->mfx.FrameInfo.Height < h)
            {
                par->mfx.FrameInfo.Height = h;
            }
        }

        return MFX_ERR_NONE;
    }

    ShiftSH = (mfxI32)(ptr - beg + 4);

    if(bs->DataLength >= (mfxU32)ShiftSH)
    {
        bs->DataOffset += ShiftSH;
        bs->DataLength -= ShiftSH;
    }

    return MFX_ERR_MORE_DATA;
}


mfxStatus VideoDECODEMPEG2::Query(VideoCORE *core, mfxVideoParam *in, mfxVideoParam *out)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_API, "VideoDECODEMPEG2::Query");

    MFX_CHECK_NULL_PTR1(out);
    mfxStatus res = MFX_ERR_NONE;

    eMFXHWType type = core->GetHWType();
    if (!in)
    { // configurability mode
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

        if (type == MFX_HW_UNKNOWN)
        {
            out->IOPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
        }
        else
        {
            out->IOPattern = MFX_IOPATTERN_OUT_VIDEO_MEMORY;
        }
        // DECODE's configurables
        out->mfx.NumThread = 1;
        out->AsyncDepth = 3;
    }
    else
    { // checking mode


        if (1 == in->mfx.DecodedOrder)
        {
            return MFX_ERR_UNSUPPORTED;
        }

        if(in->mfx.FrameInfo.FourCC != MFX_FOURCC_NV12)
        {
            out->mfx.FrameInfo.FourCC = 0;
            return MFX_ERR_UNSUPPORTED;
        }

        if (in->NumExtParam != 0 && !in->ExtParam)
            return MFX_ERR_UNSUPPORTED;

        if (!in->NumExtParam && in->ExtParam)
            return MFX_ERR_UNSUPPORTED;

        if (in->NumExtParam && !in->Protected)
        {
            mfxExtOpaqueSurfaceAlloc *pOpaq = (mfxExtOpaqueSurfaceAlloc *)GetExtendedBuffer(in->ExtParam, in->NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);

            // ignore opaque extension buffer
            if (in->NumExtParam != 1 || !(in->NumExtParam == 1 && NULL != pOpaq))
                return MFX_ERR_UNSUPPORTED;
        }

        mfxExtBuffer **pExtBuffer = out->ExtParam;
        mfxU16 numDistBuf = out->NumExtParam;
        *out = *in;

        if (in->AsyncDepth == 0)
            out->AsyncDepth = 3;
        out->ExtParam = pExtBuffer;
        out->NumExtParam = numDistBuf;

        if (!((in->mfx.FrameInfo.Width % 16 == 0)&&
            (in->mfx.FrameInfo.Width <= 4096)))
        {
            out->mfx.FrameInfo.Width = 0;
            return MFX_ERR_UNSUPPORTED;
        }

        if (!((in->mfx.FrameInfo.Height % 16 == 0)&&
            (in->mfx.FrameInfo.Height <= 4096)))
        {
            out->mfx.FrameInfo.Height = 0;
            return MFX_ERR_UNSUPPORTED;
        }

        if (!(in->mfx.FrameInfo.CropX <= out->mfx.FrameInfo.Width))
        {
            out->mfx.FrameInfo.CropX = 0;
            return MFX_ERR_UNSUPPORTED;
        }

        if (!(in->mfx.FrameInfo.CropY <= out->mfx.FrameInfo.Height))
        {
            out->mfx.FrameInfo.CropY = 0;
            return MFX_ERR_UNSUPPORTED;
        }

        if (!(out->mfx.FrameInfo.CropX + in->mfx.FrameInfo.CropW <= out->mfx.FrameInfo.Width))
        {
            out->mfx.FrameInfo.CropW = 0;
            return MFX_ERR_UNSUPPORTED;
        }

        if(!(out->mfx.FrameInfo.CropY + in->mfx.FrameInfo.CropH <= out->mfx.FrameInfo.Height))
        {
            out->mfx.FrameInfo.CropH = 0;
            return MFX_ERR_UNSUPPORTED;
        }

        if(!(in->mfx.FrameInfo.PicStruct == MFX_PICSTRUCT_PROGRESSIVE ||
           in->mfx.FrameInfo.PicStruct == MFX_PICSTRUCT_FIELD_TFF ||
           in->mfx.FrameInfo.PicStruct == MFX_PICSTRUCT_FIELD_BFF ||
           in->mfx.FrameInfo.PicStruct == MFX_PICSTRUCT_UNKNOWN))
        {
           out->mfx.FrameInfo.PicStruct = 0;
           return MFX_ERR_UNSUPPORTED;
        }

        if (!(in->mfx.FrameInfo.ChromaFormat == MFX_CHROMAFORMAT_YUV420 ||
            in->mfx.FrameInfo.ChromaFormat == 0))
        {
            out->mfx.FrameInfo.ChromaFormat = 0;
            return MFX_ERR_UNSUPPORTED;
        }

        if(in->mfx.CodecId != MFX_CODEC_MPEG2)
            out->mfx.CodecId = 0;


        if (!(in->mfx.CodecLevel == MFX_LEVEL_MPEG2_LOW ||
            in->mfx.CodecLevel == MFX_LEVEL_MPEG2_MAIN ||
            in->mfx.CodecLevel == MFX_LEVEL_MPEG2_HIGH1440 ||
            in->mfx.CodecLevel == MFX_LEVEL_MPEG2_HIGH ||
            in->mfx.CodecLevel == 0))
        {
            out->mfx.CodecLevel = 0;
            return MFX_ERR_UNSUPPORTED;
        }

        if(!(in->mfx.CodecProfile == MFX_PROFILE_MPEG2_SIMPLE ||
           in->mfx.CodecProfile == MFX_PROFILE_MPEG2_MAIN ||
           in->mfx.CodecProfile == MFX_PROFILE_MPEG2_HIGH ||
           in->mfx.CodecProfile == MFX_PROFILE_MPEG1 ||
           in->mfx.CodecProfile == 0))
        {
            out->mfx.CodecProfile = 0;
            return MFX_ERR_UNSUPPORTED;
        }

        if ((in->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY) ||
            (in->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY))
        {
            out->IOPattern = in->IOPattern;
        }
        else if (MFX_PLATFORM_SOFTWARE == core->GetPlatformType())
        {
            out->IOPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
        }
        else
        {
            out->IOPattern = MFX_IOPATTERN_OUT_VIDEO_MEMORY;
        }

         if (!IsHWSupported(core, in))
         {
             return MFX_ERR_UNSUPPORTED;
         }

    }

    return res;
}

mfxStatus VideoDECODEMPEG2::QueryIOSurf(VideoCORE *core, mfxVideoParam *par, mfxFrameAllocRequest *request)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_API, "VideoDECODEMPEG2::QueryIOSurf");
    MFX_CHECK_NULL_PTR2(par, request);

    if (!(par->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) &&
        !(par->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY) &&
        !(par->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY))
    {
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    if ((par->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) &&
        (par->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY))
    {
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    if ((par->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY) &&
        (par->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY))
    {
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    if ((par->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY) &&
        (par->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY))
    {
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    // call internal query io surface
    mfxStatus sts = VideoDECODEMPEG2InternalBase::QueryIOSurfInternal(core, par, request);
    MFX_CHECK_STS(sts);

    // ?? TODO: what is about opaque memory
    if (MFX_PLATFORM_SOFTWARE == core->GetPlatformType())
    {
        if ((par->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) &&
             false == par->mfx.DecodedOrder)
        {
            request->NumFrameSuggested = request->NumFrameMin = 1;
        }
    }
    else if (MFX_PLATFORM_HARDWARE == core->GetPlatformType())
    {
        if (false == IsHWSupported(core, par))
        {
            return MFX_ERR_UNSUPPORTED;
        }

        if ((par->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY) &&
             false == par->mfx.DecodedOrder)
        {
            request->NumFrameSuggested = request->NumFrameMin = 1 + par->AsyncDepth;
        }
    }

    return MFX_ERR_NONE;

} // mfxStatus VideoDECODEMPEG2::QueryIOSurf(VideoCORE *core, mfxVideoParam *par, mfxFrameAllocRequest *request)

mfxStatus VideoDECODEMPEG2::GetVideoParam(mfxVideoParam *par)
{
    if(!m_isInitialized)
        return MFX_ERR_NOT_INITIALIZED;

    MFX_CHECK_NULL_PTR1(par);
    return internalImpl->GetVideoParam(par);
}

mfxStatus VideoDECODEMPEG2::SetSkipMode(mfxSkipMode mode)
{
    mfxStatus ret = MFX_ERR_NONE;

    switch(mode)
    {
    case MFX_SKIPMODE_MORE:
        if(m_SkipLevel < SKIP_ALL)
        {
            m_SkipLevel++;
            internalImpl->m_implUmc->SetSkipMode(m_SkipLevel);
        }
        else
        {
            ret = MFX_WRN_VALUE_NOT_CHANGED;
        }
        break;
    case MFX_SKIPMODE_LESS:
        if(m_SkipLevel > SKIP_NONE)
        {
            m_SkipLevel--;
            internalImpl->m_implUmc->SetSkipMode(m_SkipLevel);
        }
        else
        {
            ret = MFX_WRN_VALUE_NOT_CHANGED;
        }
        break;
    case MFX_SKIPMODE_NOSKIP:
        if(m_SkipLevel <= SKIP_B)
        {
            if(m_SkipLevel == SKIP_NONE)
            {
                ret = MFX_WRN_VALUE_NOT_CHANGED;
                break;
            }
            m_SkipLevel = SKIP_NONE;
            internalImpl->m_implUmc->SetSkipMode(m_SkipLevel);
        }
        else
        {
            Reset(&internalImpl->m_vPar);
            internalImpl->m_implUmc->SetSkipMode(m_SkipLevel);
        }
        break;
    default:
        ret = MFX_ERR_UNDEFINED_BEHAVIOR;
        break;
    }
    return ret;
}

static mfxStatus __CDECL MPEG2TaskRoutine(void *pState, void *pParam, mfxU32 /*threadNumber*/, mfxU32 /*callNumber*/)
{
    VideoDECODEMPEG2InternalBase *decoder = (VideoDECODEMPEG2InternalBase*)pState;

    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_API, "VideoDECODEMPEG2::TaskRoutine");
    mfxStatus sts = decoder->TaskRoutine(pParam);
    return sts;
}


mfxStatus VideoDECODEMPEG2::CheckFrameData(const mfxFrameSurface1 *pSurface)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_API, "VideoDECODEMPEG2::CheckFrameData");
    MFX_CHECK_NULL_PTR1(pSurface);

    if (0 != pSurface->Data.Locked)
    {
        return MFX_ERR_MORE_SURFACE;
    }

    if (pSurface->Info.Width <  internalImpl->m_InitW ||
        pSurface->Info.Height < internalImpl->m_InitH)
    {
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    // robustness checking
    if (pSurface->Info.CropW > pSurface->Info.Width ||
        pSurface->Info.CropH > pSurface->Info.Height)
    {
        return MFX_ERR_UNKNOWN;
    }

    if (NULL == pSurface->Data.MemId)
    {
        switch (pSurface->Info.FourCC)
        {
            case MFX_FOURCC_NV12:

                if (NULL == pSurface->Data.Y || NULL == pSurface->Data.UV)
                {
                    return MFX_ERR_UNDEFINED_BEHAVIOR;
                }

                break;

            case MFX_FOURCC_YV12:
            case MFX_FOURCC_YUY2:

                if (NULL == pSurface->Data.Y || NULL == pSurface->Data.U || NULL == pSurface->Data.V)
                {
                    return MFX_ERR_UNDEFINED_BEHAVIOR;
                }

                break;

            case MFX_FOURCC_RGB3:

                if (NULL == pSurface->Data.R || NULL == pSurface->Data.G || NULL == pSurface->Data.B)
                {
                    return MFX_ERR_UNDEFINED_BEHAVIOR;
                }

                break;

            case MFX_FOURCC_RGB4:

                if (NULL == pSurface->Data.A ||
                    NULL == pSurface->Data.R ||
                    NULL == pSurface->Data.G ||
                    NULL == pSurface->Data.B)
                {
                    return MFX_ERR_UNDEFINED_BEHAVIOR;
                }

                break;

            default:

                break;
            }

        if (0x7FFF < pSurface->Data.Pitch)
        {
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        }
    }


    if (pSurface->Info.Width >  internalImpl->m_InitW ||
        pSurface->Info.Height > internalImpl->m_InitH)
    {
        //return MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
    }

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMPEG2::DecodeFrameCheck(mfxBitstream *bs,
                                             mfxFrameSurface1 *surface_work,
                                             mfxFrameSurface1 **surface_disp,
                                             MFX_ENTRY_POINT *pEntryPoint)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_API, "VideoDECODEMPEG2::DecodeFrameCheck");
    mfxStatus sts = MFX_ERR_NONE;

    MFX_CHECK_NULL_PTR1(surface_disp);
    MFX_CHECK_NULL_PTR1(surface_work);

    if (false == m_isInitialized)
    {
        return MFX_ERR_NOT_INITIALIZED;
    }

    if (true == internalImpl->IsOpaqueMemory())
    {
        if (surface_work->Data.MemId || surface_work->Data.Y || surface_work->Data.R || surface_work->Data.A || surface_work->Data.UV) // opaq surface
            return MFX_ERR_UNDEFINED_BEHAVIOR;

        surface_work = internalImpl->GetOriginalSurface(surface_work);
    }

    if (bs)
    {
        sts = CheckBitstream(bs);
        MFX_CHECK_STS(sts);
    }

    sts = CheckFrameData(surface_work);
    MFX_CHECK_STS(sts);

    sts = internalImpl->ConstructFrame(bs, surface_work);
    MFX_CHECK_STS(sts);

    return internalImpl->DecodeFrameCheck(bs, surface_work, surface_disp, pEntryPoint);
}

inline bool IsMpeg2StartCode(const mfxU8* p)
{
    return p[0] == 0 && p[1] == 0 && p[2] == 1 && (p[3] == ePIC || p[3] == eSEQ || p[3] == eEND || p[3] == eGROUP);
}
inline bool IsMpeg2StartCodeEx(const mfxU8* p)
{
    return p[0] == 0 && p[1] == 0 && p[2] == 1 && (p[3] == ePIC || p[3] == eEXT || p[3] == eSEQ || p[3] == eEND || p[3] == eGROUP);
}

inline bool IsMpeg2UserDataStartCode(const mfxU8* p)
{
    return p[0] == 0 && p[1] == 0 && p[2] == 1 && p[3] == eUSER;
}

const mfxU8* FindUserDataStartCode(const mfxU8* begin, const mfxU8* end)
{
    for (; begin + 3 < end; ++begin)
        if (IsMpeg2UserDataStartCode(begin))
            break;
    return begin;
}

inline bool IsMpeg2AnyStartCode(const mfxU8* p)
{
    return p[0] == 0 && p[1] == 0 && p[2] == 1;
}

const mfxU8* FindAnyStartCode(const mfxU8* begin, const mfxU8* end)
{
    for (; begin + 3 < end; ++begin)
        if (IsMpeg2AnyStartCode(begin))
            break;
    return begin;
}

const mfxU8* FindStartCode(const mfxU8* begin, const mfxU8* end)
{
    for (; begin + 3 < end; ++begin)
        if (IsMpeg2StartCode(begin))
            break;
    return begin;
}
const mfxU8* FindStartCodeEx(const mfxU8* begin, const mfxU8* end)
{
    for (; begin + 3 < end; ++begin)
        if (IsMpeg2StartCodeEx(begin))
            break;
    return begin;
}

mfxStatus AppendBitstream(mfxBitstream& bs, const mfxU8* ptr, mfxU32 bytes)
{
    if (bs.DataOffset + bs.DataLength + bytes > bs.MaxLength)
        return MFX_ERR_NOT_ENOUGH_BUFFER;
    MFX_INTERNAL_CPY(bs.Data + bs.DataOffset + bs.DataLength, ptr, bytes);
    bs.DataLength += bytes;
    return MFX_ERR_NONE;
}

mfxStatus CutUserData(mfxBitstream *in, mfxBitstream *out, const mfxU8 *tail)
{
    const mfxU8* head = in->Data + in->DataOffset;
    const mfxU8* UserDataStart = FindUserDataStartCode(head, tail);
    while ( UserDataStart + 3 < tail)
    {
        mfxStatus sts = AppendBitstream(*out, head, (mfxU32)(UserDataStart - head));
        MFX_CHECK_STS(sts);
        MoveBitstreamData(*in, (mfxU32)(UserDataStart - head) + 4);

        head = in->Data + in->DataOffset;
        const mfxU8* UserDataEnd = FindAnyStartCode(head, tail);
        MoveBitstreamData(*in, (mfxU32)(UserDataEnd - head));

        head = in->Data + in->DataOffset;
        UserDataStart = FindUserDataStartCode(head, tail);
    }
    return MFX_ERR_NONE;
}

VideoDECODEMPEG2InternalBase::VideoDECODEMPEG2InternalBase()
{
    m_pCore = 0;
    m_isSWBuf = false;
    m_isSWDecoder = true;
    m_found_SH = false;
    m_found_RA_Frame = false;
    m_first_SH = true;

    for(int i = 0; i < DPB; i++)
    {
        mid[i] = -1;
    }

    for(int i = 0; i < NUM_FRAMES; i++)
    {
        m_frame[i].Data = NULL;
        m_frame_in_use[i] = false;
    }

    m_FrameAllocator = 0;

    m_isDecodedOrder = false;
    m_reset_done = false;

    m_isOpaqueMemory = false;
    m_isFrameRateFromInit = false;

    memset(&allocResponse, 0, sizeof(mfxFrameAllocResponse));
    memset(&m_opaqueResponse, 0, sizeof(mfxFrameAllocResponse));

    Reset(0);
}

VideoDECODEMPEG2InternalBase::~VideoDECODEMPEG2InternalBase()
{
}

mfxStatus VideoDECODEMPEG2InternalBase::Reset(mfxVideoParam *par)
{
    if (m_FrameAllocator)
    {
        for (mfxU32 i = 0; i < DPB; i++)
        {
            if (mid[i] >= 0)
            {
                m_FrameAllocator->DecreaseReference(mid[i]);
                mid[i] = -1;
            }
        }

        m_FrameAllocator->Reset();
    }

    prev_index      = -1;
    curr_index      = -1;
    next_index      = -1;
    display_index   = -1;
    display_order   =  0;
    dec_frame_count = 0;
    dec_field_count = 0;
    last_frame_count = 0;
    display_frame_count = 0;
    cashed_frame_count = 0;
    skipped_frame_count = 0;
    last_timestamp   = 0;
    last_good_timestamp = 0.0;

    m_found_SH = false;
    m_found_RA_Frame = false;
    m_first_SH = true;
    m_new_bs   = true;

    ResetFcState(m_fcState);

    m_task_num = 0;
    m_prev_task_num = -1;

    for(int i = 0; i < NUM_FRAMES; i++)
    {
        m_frame[i].DataLength = 0;
        m_frame[i].DataOffset = 0;
        m_frame_in_use[i] = false;
        m_time[i] = (mfxF64)(-1);
    }

    m_frame_curr = -1;
    m_frame_free = -1;
    m_frame_constructed = true;

    memset(m_last_bytes,0,NUM_REST_BYTES);

    m_resizing = false;
    m_InitPicStruct = 0;

    m_vdPar.info.framerate = 0;

    if (par)
    {
        m_vPar = *par;

        m_vdPar.info.stream_type = UMC::MPEG2_VIDEO;
        m_vdPar.info.clip_info.width = par->mfx.FrameInfo.Width;
        m_vdPar.info.clip_info.height = par->mfx.FrameInfo.Height;

        m_vdPar.info.aspect_ratio_width = par->mfx.FrameInfo.AspectRatioW;
        m_vdPar.info.aspect_ratio_height = par->mfx.FrameInfo.AspectRatioH;

        mfxU32 frameRateD = par->mfx.FrameInfo.FrameRateExtD;
        mfxU32 frameRateN = par->mfx.FrameInfo.FrameRateExtN;

        if (0 != frameRateD && 0 != frameRateN)
        {
            m_vdPar.info.framerate = (double) frameRateN / frameRateD;
            m_isFrameRateFromInit = true;
        }

        m_vdPar.info.color_format = GetUmcColorFormat((mfxU8)par->mfx.FrameInfo.ChromaFormat);

        if (UMC::NONE == m_vdPar.info.color_format)
        {
            return MFX_ERR_INVALID_VIDEO_PARAM;
        }

        m_vdPar.lFlags |= par->mfx.DecodedOrder ? 0 : UMC::FLAG_VDEC_REORDER;
        m_vdPar.lFlags |= UMC::FLAG_VDEC_EXTERNAL_SURFACE_USE;

        switch(par->mfx.TimeStampCalc)
        {
            case MFX_TIMESTAMPCALC_TELECINE:
                m_vdPar.lFlags |= UMC::FLAG_VDEC_TELECINE_PTS;
                break;

            case MFX_TIMESTAMPCALC_UNKNOWN:
                break;

            default:
                return MFX_ERR_INVALID_VIDEO_PARAM;
        }

        m_vdPar.numThreads = (mfxU16)(par->AsyncDepth ? par->AsyncDepth : m_pCore->GetAutoAsyncDepth());

        if (m_InitH || m_InitW)
        {
            if (2048 < par->mfx.FrameInfo.Width || 2048 < par->mfx.FrameInfo.Height)
            {
                return MFX_ERR_INVALID_VIDEO_PARAM;
            }

            if (m_InitW < par->mfx.FrameInfo.Width || m_InitH < par->mfx.FrameInfo.Height)
            {
                return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;
            }
        }

        m_isDecodedOrder = (1 == par->mfx.DecodedOrder) ? true : false;
    }
    else
    {
        m_InitW = 0;
        m_InitH = 0;
    }

    if (m_implUmc.get())
        m_implUmc->Reset();

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMPEG2InternalBase::Close()
{
    m_implUmc->Close();

    for (int i = 0; i < DPB; i++)
        if(mid[i] >= 0)
            m_FrameAllocator->DecreaseReference(mid[i]);

    for (int i = 0; i < DPB; i++)
    {
        mid[i] = -1;
    }

    for (int i = 0; i < NUM_FRAMES; i++)
    {
        if(m_frame[i].Data) delete []m_frame[i].Data;
        m_frame[i].Data = NULL;
    }

    if(m_FrameAllocator)
        m_FrameAllocator->Close();

    delete m_FrameAllocator;
    m_FrameAllocator = 0;

    mfxStatus mfxRes = MFX_ERR_NONE;

    if (0 < allocResponse.NumFrameActual)
    {
        mfxRes = m_pCore->FreeFrames(&allocResponse);
        memset(&allocResponse, 0, sizeof(mfxFrameAllocResponse));
    }

    if (0 < m_opaqueResponse.NumFrameActual && true == m_isOpaqueMemory)
    {
        mfxRes = m_pCore->FreeFrames(&m_opaqueResponse);
        memset(&m_opaqueResponse, 0, sizeof(mfxFrameAllocResponse));
    }

    m_isOpaqueMemory = false;

    return mfxRes;
}

mfxStatus VideoDECODEMPEG2InternalBase::Init(mfxVideoParam *par, VideoCORE * core)
{
    m_pCore = core;

    m_implUmc->Close();

    mfxStatus sts = AllocFrames(par);
    MFX_CHECK_STS(sts);

    Reset(par);

    uint32_t size = uint32_t (par->mfx.FrameInfo.Width * par->mfx.FrameInfo.Height *3L/2);
    uint32_t MaxLength = 1024 * 500;

    if (size > MaxLength)
    {
        MaxLength = size;
    }

    for (int i = 0; i < NUM_FRAMES; i++)
    {
        memset(&m_frame[i], 0, sizeof(mfxBitstream));
        m_frame[i].MaxLength = MaxLength;
        m_frame[i].Data = new mfxU8[MaxLength];
        m_frame[i].DataLength = 0;
        m_frame[i].DataOffset = 0;
        m_frame_in_use[i] = false;
    }

    m_InitW = par->mfx.FrameInfo.Width;
    m_InitH = par->mfx.FrameInfo.Height;

    m_InitPicStruct = par->mfx.FrameInfo.PicStruct;
    m_Protected = par->Protected;
    m_isDecodedOrder = (1 == par->mfx.DecodedOrder) ? true : false;

    maxNumFrameBuffered = NUM_FRAMES_BUFFERED;
    // video conference mode
    // async depth == 1 means no bufferezation
    if (1 == par->AsyncDepth)
    {
        maxNumFrameBuffered = 1;
    }

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMPEG2InternalBase::QueryIOSurfInternal(VideoCORE *core, mfxVideoParam *par, mfxFrameAllocRequest *request)
{
    if (par->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY)
    {
        request->Type = MFX_MEMTYPE_EXTERNAL_FRAME | MFX_MEMTYPE_DXVA2_DECODER_TARGET | MFX_MEMTYPE_FROM_DECODE;
    }
    else if (par->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY)
    {
        request->Type = MFX_MEMTYPE_EXTERNAL_FRAME | MFX_MEMTYPE_SYSTEM_MEMORY | MFX_MEMTYPE_FROM_DECODE;
    }
    else // opaque memory case
    {
        if (MFX_PLATFORM_SOFTWARE == core->GetPlatformType())
        {
            request->Type = MFX_MEMTYPE_OPAQUE_FRAME | MFX_MEMTYPE_SYSTEM_MEMORY | MFX_MEMTYPE_FROM_DECODE;
        }
        else
        {
            request->Type = MFX_MEMTYPE_OPAQUE_FRAME | MFX_MEMTYPE_DXVA2_DECODER_TARGET | MFX_MEMTYPE_FROM_DECODE;
        }
    }

    if (MFX_PLATFORM_HARDWARE == core->GetPlatformType())
    {
        // check hardware restrictions
        if (false == IsHWSupported(core, par))
        {
            //request->Type = MFX_MEMTYPE_EXTERNAL_FRAME | MFX_MEMTYPE_SYSTEM_MEMORY | MFX_MEMTYPE_FROM_DECODE;

            if (par->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY)
            {
                request->Type = MFX_MEMTYPE_OPAQUE_FRAME | MFX_MEMTYPE_SYSTEM_MEMORY | MFX_MEMTYPE_FROM_DECODE;
            }
        }
    }

    request->Info = par->mfx.FrameInfo;

    // output color format is NV12
    request->Info.FourCC = MFX_FOURCC_NV12;
    request->NumFrameMin = NUM_FRAMES_BUFFERED + 3;
    request->NumFrameSuggested = request->NumFrameMin = request->NumFrameMin + (par->AsyncDepth ? par->AsyncDepth : core->GetAutoAsyncDepth());
    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMPEG2InternalBase::AllocFrames(mfxVideoParam *par)
{
    memset(&allocResponse, 0, sizeof(mfxFrameAllocResponse));

    mfxU16 IOPattern = par->IOPattern;

    // frames allocation
    int useInternal = m_isSWDecoder ? (IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) : (IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY);

    if (IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY)
    {
        mfxExtOpaqueSurfaceAlloc *pOpaqAlloc = (mfxExtOpaqueSurfaceAlloc *)GetExtendedBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
        if (!pOpaqAlloc)
            return MFX_ERR_INVALID_VIDEO_PARAM;

        useInternal = m_isSWDecoder ? !(pOpaqAlloc->Out.Type & MFX_MEMTYPE_SYSTEM_MEMORY) : (pOpaqAlloc->Out.Type & MFX_MEMTYPE_SYSTEM_MEMORY);
    }

    if (!m_isSWDecoder || useInternal || (IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY))
    {
        // positive condition means that decoder was configured as
        //  a. as hardware implementation or
        //  b. as software implementation with d3d9 output surfaces
        QueryIOSurfInternal(m_pCore, par, &allocRequest);

        //const mfxU16 NUM = allocRequest.NumFrameSuggested;
        mfxExtOpaqueSurfaceAlloc *pOpqExt = 0;
        if (MFX_IOPATTERN_OUT_OPAQUE_MEMORY & IOPattern) // opaque memory case
        {
            pOpqExt = (mfxExtOpaqueSurfaceAlloc *)GetExtendedBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
            if (!pOpqExt || allocRequest.NumFrameMin > pOpqExt->Out.NumSurface)
                return MFX_ERR_INVALID_VIDEO_PARAM;

            m_isOpaqueMemory = true;

            if (m_isSWDecoder)
            {
                allocRequest.Type = (MFX_MEMTYPE_SYSTEM_MEMORY & pOpqExt->Out.Type) ? (mfxU16)pOpqExt->Out.Type :
                    (mfxU16)(MFX_MEMTYPE_OPAQUE_FRAME | MFX_MEMTYPE_FROM_DECODE | MFX_MEMTYPE_DXVA2_DECODER_TARGET);
            }

            allocRequest.NumFrameMin = allocRequest.NumFrameSuggested = (mfxU16)pOpqExt->Out.NumSurface;
        }
        else
        {
            allocRequest.Type = MFX_MEMTYPE_FROM_DECODE;
            allocRequest.Type |= m_isSWDecoder ? MFX_MEMTYPE_SYSTEM_MEMORY : MFX_MEMTYPE_DXVA2_DECODER_TARGET;
            allocRequest.Type |= useInternal ? MFX_MEMTYPE_INTERNAL_FRAME : MFX_MEMTYPE_EXTERNAL_FRAME;
        }

        allocResponse.NumFrameActual = allocRequest.NumFrameSuggested;

        mfxStatus mfxSts;
        if (pOpqExt)
        {
            mfxSts  = m_pCore->AllocFrames(&allocRequest,
                                           &allocResponse,
                                           pOpqExt->Out.Surfaces,
                                           pOpqExt->Out.NumSurface);
        }
        else
        {
            bool isNeedCopy = ((MFX_IOPATTERN_OUT_SYSTEM_MEMORY & IOPattern) && (allocRequest.Type & MFX_MEMTYPE_INTERNAL_FRAME)) ||
                ((MFX_IOPATTERN_OUT_VIDEO_MEMORY & IOPattern) && (m_isSWDecoder));
            allocRequest.AllocId = par->AllocId;
            mfxSts = m_pCore->AllocFrames(&allocRequest, &allocResponse, isNeedCopy);
            if(mfxSts)
                return MFX_ERR_INVALID_VIDEO_PARAM;
        }

        if (mfxSts != MFX_ERR_NONE && mfxSts != MFX_ERR_NOT_FOUND)
        {
            // second status means that external allocator was not found, it is ok
            return mfxSts;
        }

        if (!useInternal)
            m_FrameAllocator->SetExternalFramesResponse(&allocResponse);
    }

    UMC::Status sts = m_FrameAllocator->InitMfx(NULL, m_pCore, par, &allocRequest, &allocResponse, !useInternal, m_isSWDecoder);
    if (UMC::UMC_OK != sts)
    {
        return MFX_ERR_MEMORY_ALLOC;
    }

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMPEG2InternalBase::GetVideoParam(mfxVideoParam *par)
{
    MFX_CHECK_NULL_PTR1(par);

    par->AllocId    = m_vPar.AllocId;
    par->AsyncDepth = m_vPar.AsyncDepth;
    par->mfx        = m_vPar.mfx;
    par->Protected  = m_vPar.Protected;
    par->IOPattern  = m_vPar.IOPattern;

    auto *spspps = reinterpret_cast<mfxExtCodingOptionSPSPPS *>(GetExtendedBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_CODING_OPTION_SPSPPS));

    if (spspps && spspps->SPSBuffer)
    {
        UMC::Status sts = m_implUmc->GetSequenceHeaderMemoryMask(spspps->SPSBuffer, spspps->SPSBufSize);
        if (UMC::UMC_OK != sts)
            return ConvertUMCStatusToMfx(sts);
    }

    auto *signal_info = reinterpret_cast<mfxExtVideoSignalInfo *>(GetExtendedBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_VIDEO_SIGNAL_INFO));

    if (signal_info)
    {
        UMC::Status sts = m_implUmc->GetSignalInfoInformation(signal_info);
        if (UMC::UMC_OK != sts)
            return ConvertUMCStatusToMfx(sts);
    }

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMPEG2InternalBase::UpdateWorkSurfaceParams(int task_num)
{
    mfxFrameSurface1 *pSurface = m_FrameAllocator->GetSurfaceByIndex(mid[task_num]);
    MFX_CHECK_NULL_PTR1(pSurface);

    pSurface->Info.CropW        = m_vPar.mfx.FrameInfo.CropW;
    pSurface->Info.CropH        = m_vPar.mfx.FrameInfo.CropH;
    pSurface->Info.CropX        = m_vPar.mfx.FrameInfo.CropX;
    pSurface->Info.CropY        = m_vPar.mfx.FrameInfo.CropY;
    pSurface->Info.AspectRatioH = m_vPar.mfx.FrameInfo.AspectRatioH;
    pSurface->Info.AspectRatioW = m_vPar.mfx.FrameInfo.AspectRatioW;

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMPEG2InternalBase::UpdateCurrVideoParams(mfxFrameSurface1 *surface_work, int task_num)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameSurface1 *pSurface = surface_work;

    if (true == m_isOpaqueMemory)
    {
        pSurface = m_pCore->GetOpaqSurface(surface_work->Data.MemId, true);
    }

    if (dec_field_count & 1)
    {
        return MFX_ERR_NONE;
    }

    const UMC::sSequenceHeader& sh = m_implUmc->GetSequenceHeader();
    const UMC::sPictureHeader& ph = m_implUmc->GetPictureHeader(task_num);

    if (m_vPar.mfx.FrameInfo.AspectRatioH != 0 || m_vPar.mfx.FrameInfo.AspectRatioW != 0)
        if(sh.aspect_ratio_w != m_vPar.mfx.FrameInfo.AspectRatioW || sh.aspect_ratio_h != m_vPar.mfx.FrameInfo.AspectRatioH)
            sts = MFX_WRN_VIDEO_PARAM_CHANGED;

    if (m_isFrameRateFromInit == false && ((uint32_t)sh.frame_rate_extension_d != m_vPar.mfx.FrameInfo.FrameRateExtD || (uint32_t)sh.frame_rate_extension_n != m_vPar.mfx.FrameInfo.FrameRateExtN))
    {
        sts = MFX_WRN_VIDEO_PARAM_CHANGED;
    }

    pSurface->Info.CropW = m_vPar.mfx.FrameInfo.CropW;
    pSurface->Info.CropH = m_vPar.mfx.FrameInfo.CropH;
    pSurface->Info.CropX = m_vPar.mfx.FrameInfo.CropX;
    pSurface->Info.CropY = m_vPar.mfx.FrameInfo.CropY;
    pSurface->Info.AspectRatioH = m_vPar.mfx.FrameInfo.AspectRatioH;
    pSurface->Info.AspectRatioW = m_vPar.mfx.FrameInfo.AspectRatioW;

    UpdateMfxVideoParam(m_vPar, sh, ph);

    pSurface->Info.PicStruct = m_vPar.mfx.FrameInfo.PicStruct;
    pSurface->Info.FrameRateExtD = m_vPar.mfx.FrameInfo.FrameRateExtD;
    pSurface->Info.FrameRateExtN = m_vPar.mfx.FrameInfo.FrameRateExtN;

    return sts;
}

mfxFrameSurface1 *VideoDECODEMPEG2InternalBase::GetOriginalSurface(mfxFrameSurface1 *pSurface)
{
    if (true == m_isOpaqueMemory)
    {
        return m_pCore->GetNativeSurface(pSurface);
    }

    return pSurface;
}

mfxStatus VideoDECODEMPEG2InternalBase::SetOutputSurfaceParams(mfxFrameSurface1 *surface, int disp_index)
{
    MFX_CHECK_NULL_PTR1(surface);

    surface->Data.TimeStamp = GetMfxTimeStamp(m_implUmc->GetCurrDecodedTime(disp_index));
    surface->Data.DataFlag = (mfxU16)((m_implUmc->isOriginalTimeStamp(disp_index)) ? MFX_FRAMEDATA_ORIGINAL_TIMESTAMP : 0);
    SetSurfacePicStruct(surface, disp_index);
    UpdateOutputSurfaceParamsFromWorkSurface(surface, disp_index);
    SetSurfacePictureType(surface, disp_index, m_implUmc.get());
    SetSurfaceTimeCode(surface, disp_index, m_implUmc.get());

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMPEG2InternalBase::SetSurfacePicStruct(mfxFrameSurface1 *surface, int disp_index)
{
    MFX_CHECK_NULL_PTR1(surface);

    if (disp_index < 0 || disp_index >= 2*DPB)
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    const UMC::sPictureHeader& ph = m_implUmc->GetPictureHeader(disp_index);
    const UMC::sSequenceHeader& sh = m_implUmc->GetSequenceHeader();

    surface->Info.PicStruct = GetMfxPicStruct(sh.progressive_sequence, ph.progressive_frame,
                                              ph.top_field_first, ph.repeat_first_field, ph.picture_structure,
                                              m_vPar.mfx.ExtendedPicStruct);
    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMPEG2InternalBase::UpdateOutputSurfaceParamsFromWorkSurface(mfxFrameSurface1 *outputSurface, int disp_index)
{
    if (!m_isSWBuf && !m_isOpaqueMemory)
        return MFX_ERR_NONE;

    mfxFrameSurface1 *internalSurface = m_FrameAllocator->GetSurfaceByIndex(mid[disp_index]);
    MFX_CHECK_NULL_PTR1(internalSurface);
    outputSurface->Info.CropW = internalSurface->Info.CropW;
    outputSurface->Info.CropH = internalSurface->Info.CropH;
    outputSurface->Info.CropX = internalSurface->Info.CropX;
    outputSurface->Info.CropY = internalSurface->Info.CropY;
    outputSurface->Info.AspectRatioH = internalSurface->Info.AspectRatioH;
    outputSurface->Info.AspectRatioW = internalSurface->Info.AspectRatioW;
    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMPEG2InternalBase::GetOutputSurface(mfxFrameSurface1 **surface_out, mfxFrameSurface1 *surface_work, UMC::FrameMemID index)
{
    mfxFrameSurface1 *pNativeSurface =  m_FrameAllocator->GetSurface(index, surface_work, &m_vPar);

    if (pNativeSurface)
    {
        if (m_isOpaqueMemory)
            *surface_out = m_pCore->GetOpaqSurface(pNativeSurface->Data.MemId);
        else
            *surface_out = pNativeSurface;
    }
    else
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMPEG2InternalBase::ConstructFrame(mfxBitstream *bs, mfxFrameSurface1 *surface_work)
{
    if (NeedToReturnCriticalStatus(bs))
        return ReturningCriticalStatus();

    if (m_frame_constructed && bs != NULL)
    {
        if (!SetFree_m_frame())
        {
            return MFX_WRN_DEVICE_BUSY;
        }

        m_frame[m_frame_free].DataLength = 0;
        m_frame[m_frame_free].DataOffset = 0;
    }

    m_frame_constructed = false;

    if (bs)
    {
        if (!bs->DataLength)
        {
            return MFX_ERR_MORE_DATA;
        }

        if (true == m_new_bs)
        {
            m_time[m_frame_free] = GetUmcTimeStamp(bs->TimeStamp);
            m_time[m_frame_free] = (m_time[m_frame_free]==last_good_timestamp)?-1.0:m_time[m_frame_free];
        }

        mfxStatus sts = ConstructFrame(bs, &m_frame[m_frame_free], surface_work);

        if (MFX_ERR_NONE != sts)
        {
            m_frame_constructed = false;
            m_new_bs   = false;
            return sts;
        }

        last_good_timestamp = m_time[m_frame_free] == -1.0?last_good_timestamp:m_time[m_frame_free];
        m_frame_in_use[m_frame_free] = true;
        m_new_bs = true;
    }
    else
    {
        m_time[m_frame_free] = (mfxF64)(-1);

        if (0 < m_last_bytes[3])
        {
            uint8_t *pData = m_frame[m_frame_free].Data + m_frame[m_frame_free].DataOffset + m_frame[m_frame_free].DataLength;
            std::copy(m_last_bytes, m_last_bytes + m_last_bytes[3], pData);

            m_frame[m_frame_free].DataLength += m_last_bytes[3];
            memset(m_last_bytes, 0, NUM_REST_BYTES);
        }
    }

    m_found_SH = false;
    m_frame_constructed = true;

    if (!(dec_field_count & 1)) {
        UMC::AutomaticUMCMutex guard(m_guard);
        m_task_num = m_implUmc->FindFreeTask();
    }

    if (-1 == m_task_num)
    {
        return MFX_WRN_DEVICE_BUSY;
    }

    if (last_frame_count > 0)
    {
        if(m_implUmc->GetRetBufferLen() <= 0)
            return MFX_ERR_MORE_DATA;
    }

    mfxStatus sts = m_FrameAllocator->SetCurrentMFXSurface(surface_work, m_isOpaqueMemory);
    MFX_CHECK_STS(sts);
    if (m_FrameAllocator->FindFreeSurface() == -1)
        return MFX_WRN_DEVICE_BUSY;

    if (false == SetCurr_m_frame() && bs)
    {
        return MFX_WRN_DEVICE_BUSY;
    }

    return sts;
}

mfxStatus VideoDECODEMPEG2InternalBase::ConstructFrame(mfxBitstream *in, mfxBitstream *out, mfxFrameSurface1 *surface_work)
{
    mfxStatus sts = MFX_ERR_NONE;
    do
    {
        sts = ConstructFrameImpl(in, out, surface_work);

        if (sts == MFX_ERR_NOT_ENOUGH_BUFFER)
        {
            out->DataLength = 0;
            out->DataOffset = 0;
            memset(m_last_bytes, 0, NUM_REST_BYTES);
            ResetFcState(m_fcState);
            m_found_SH = false; // to parse and verify next SeqH
        }
    } while (MFX_ERR_NOT_ENOUGH_BUFFER == sts);

    return sts;
}

mfxStatus VideoDECODEMPEG2InternalBase::ConstructFrameImpl(mfxBitstream *in, mfxBitstream *out, mfxFrameSurface1 *surface_work)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VideoDECODEMPEG2::ConstructFrameImpl");
    mfxStatus sts = MFX_ERR_NONE;

    MFX_CHECK_NULL_PTR2(in, out);

    MFX_CHECK_NULL_PTR2(in->Data, out->Data);

    if (MINIMAL_BITSTREAM_LENGTH > in->DataLength)
    {
        AppendBitstream(*out, in->Data + in->DataOffset, in->DataLength);
        MoveBitstreamData(*in, (mfxU32)(in->DataLength));
        memset(m_last_bytes,0,NUM_REST_BYTES);

        return MFX_ERR_MORE_DATA;
    }

    // calculate tail of bitstream chunk
    const mfxU8 *tail = in->Data + in->DataOffset + in->DataLength;

    while (!m_fcState.picStart)
    {
        const mfxU8* head = in->Data + in->DataOffset;
        const mfxU8* curr = FindStartCode(head, tail);

        if (curr >= tail - 3)
        {
            MoveBitstreamData(*in, (mfxU32)(curr - head));
            memset(m_last_bytes, 0, NUM_REST_BYTES);

            return MFX_ERR_MORE_DATA;
        }
        if (eSEQ == curr[3] && false == m_found_SH)
        {
            if(tail < curr + 6)
            {
                return MFX_ERR_MORE_DATA;
            }

            mfxU16 CropW = (curr[4] << 4) + ((curr[5] >> 4) & 0xf);
            mfxU16 CropH = ((curr[5] & 0xf) << 8) + curr[6];
            mfxU16 Width = (CropW + 15) & ~0x0f;
            mfxU16 Height = (CropH + 15) & ~0x0f;

            const mfxU8* ptr = FindStartCodeEx(curr + 4, tail);

            // check that data length is enough to read whole sequence extension
            if(tail < ptr + 7)
            {
                return MFX_ERR_MORE_DATA;
            }

            if (eEXT == ptr[3])
            {
                uint32_t code = (ptr[4] & 0xf0);

                if(0x10 == code)
                {
                    code = (ptr[4] << 24) | (ptr[5] << 16) | (ptr[6] << 8) | ptr[7];
                    uint32_t progressive_seq = (code >> 19) & 1;

                    CropW = (mfxU16)((CropW  & 0xfff) | ((code >> (15-12)) & 0x3000));
                    CropH = (mfxU16)((CropH & 0xfff) | ((code >> (13-12)) & 0x3000));
                    Width = (CropW + 15) & ~0x0f;
                    Height = (CropH + 15) & ~0x0f;
                    if(0 == progressive_seq)
                    {
                        Height = (CropH + 31) & ~(31);
                    }

                    mfxU8 profile_and_level = (code >> 20) & 0xff;
//                  mfxU8 profile = (profile_and_level >> 4) & 0x7;
                    mfxU8 level = profile_and_level & 0xf;

                    switch(level)
                    {
                        case  4:
                        case  6:
                        case  8:
                        case 10:
                            break;
                        default:
                            MoveBitstreamData(*in, (mfxU32)(curr - head) + 4);
                            memset(m_last_bytes, 0, NUM_REST_BYTES);
                            continue;
                    }

                    mfxU8 chroma = (code >> 17) & 3;
                    const int chroma_yuv420 = 1;
                    if (chroma != chroma_yuv420)
                    {
                        MoveBitstreamData(*in, (mfxU32)(curr - head) + 4);
                        memset(m_last_bytes, 0, NUM_REST_BYTES);
                        continue;
                    }
                }
            }
            else
            {
                if(m_InitPicStruct != MFX_PICSTRUCT_PROGRESSIVE)
                {
                    Height = (CropH + 31) & ~(31);
                }
            }
            mfxVideoParam vpCopy = m_vPar;
            vpCopy.mfx.FrameInfo.Width = Width;
            vpCopy.mfx.FrameInfo.Height = Height;

            if (!IsHWSupported(m_pCore, &vpCopy) || Width == 0 || Height == 0)
            {
                MoveBitstreamData(*in, (mfxU32)(curr - head) + 4);
                memset(m_last_bytes, 0, NUM_REST_BYTES);
                continue;
            }
            if (m_InitW <  Width || m_InitH < Height || surface_work->Info.Width <  Width || surface_work->Info.Height < Height)
            {
                m_resizing = true;

                return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;
            }

            m_found_SH = true;

            if (false == m_first_SH && (surface_work->Info.CropW <  CropW || surface_work->Info.CropH < CropH))
            {
                m_resizing = true;

                return MFX_WRN_VIDEO_PARAM_CHANGED;
            }

            if (false == m_first_SH)
                if (m_InitW >  Width || m_InitH > Height)
                {
                    m_resizing = true;
                    return MFX_WRN_VIDEO_PARAM_CHANGED;
                }

            m_first_SH = false;

            if (eEXT == ptr[3])
            {
                sts = AppendBitstream(*out, curr, (mfxU32)(ptr - curr));
                if (sts == MFX_ERR_NOT_ENOUGH_BUFFER)
                    MoveBitstreamData(*in, (mfxU32)(ptr - curr)); // huge frame - skip it
                MFX_CHECK_STS(sts);
                curr = ptr;
            }
        }

        MoveBitstreamData(*in, (mfxU32)(curr - head));

        if (curr[3] == eSEQ ||
            curr[3] == eEXT ||
            curr[3] == ePIC ||
            curr[3] == eGROUP)
        {
            sts = AppendBitstream(*out, curr, 4);
            MFX_CHECK_STS(sts);

            MoveBitstreamData(*in, 4);

            m_fcState.picStart = 1;

            if (ePIC == curr[3])
            {
                m_fcState.picHeader = FcState::FRAME;

                if (MFX_BITSTREAM_COMPLETE_FRAME == in->DataFlag)
                {
                    uint32_t len = in->DataLength;

                    head = in->Data + in->DataOffset;
                    curr = FindStartCode(head, tail);

                    MFX_CHECK(curr >= head, MFX_ERR_UNDEFINED_BEHAVIOR);

                    // if start code found
                    if( (tail - curr) > 3 &&
                        (curr[3] == eSEQ ||
                         curr[3] == eEXT ||
                         curr[3] == ePIC ||
                         curr[3] == eEND ||
                         curr[3] == eGROUP)
                      )
                    {
                        len = (mfxU32)(curr - head);
                    }

                    MFX_CHECK(out->MaxLength >= out->DataLength, MFX_ERR_UNDEFINED_BEHAVIOR);

                    if(out->MaxLength - out->DataLength < len)
                    {
                        len = out->MaxLength - out->DataLength;
                    }

                    sts = AppendBitstream(*out, head, len);
                    MFX_CHECK_STS(sts);

                    MoveBitstreamData(*in, len);

                    if (len < (8-4) + 9) // (min pic. header - startcode) + min pic.coding ext.
                        return MFX_ERR_NOT_ENOUGH_BUFFER; // to start again with next picture

                    m_fcState.picStart = 0;
                    m_fcState.picHeader = FcState::NONE;
                    memset(m_last_bytes, 0, NUM_REST_BYTES);

                    if(m_resizing)
                    {
                        m_resizing = false;
                        //return MFX_WRN_VIDEO_PARAM_CHANGED;
                    }

                    return MFX_ERR_NONE;
                }
            }
        }
        else
        {
            MoveBitstreamData(*in, 4);
        }
    }

    bool skipped = false;
    for ( ; ; )
    {
        const mfxU8* head = in->Data + in->DataOffset;
        const mfxU8* curr = FindStartCode(head, tail);

        if (skipped)
        {
            skipped = false;
            MoveBitstreamData(*in, (mfxU32)(curr - head));
            head = in->Data + in->DataOffset;
        }

        if (curr + 3 >= tail)
        {
            // Not enough buffer, it is possible due to long user data. Try to "cut" it off
            if (out->DataOffset + out->DataLength + (curr - head) > out->MaxLength)
            {
                sts = CutUserData(in, out, curr);
                MFX_CHECK_STS(sts);
            }

            head = in->Data + in->DataOffset;
            sts = AppendBitstream(*out, head, (mfxU32)(curr - head));
            MFX_CHECK_STS(sts);

            MoveBitstreamData(*in, (mfxU32)(curr - head));

            mfxU8 *p = (mfxU8 *) curr;

            m_last_bytes[3] = 0;

            while(p < tail)
            {
                m_last_bytes[m_last_bytes[3]] = *p;
                p++;
                m_last_bytes[3]++;
            }

            if(m_resizing)
            {
                m_resizing = false;
                //return MFX_WRN_VIDEO_PARAM_CHANGED;
            }

            return MFX_ERR_MORE_DATA;
        }
        else
        {
            if (eEND == curr[3] && m_fcState.picHeader == FcState::FRAME)
            {
                // append end_sequence_code to the end of current picture
                curr += 4;
            }

            if (ePIC == curr[3] && !m_found_RA_Frame)
            {
                if (tail < curr + 6)
                    return MFX_ERR_MORE_DATA;

                mfxI32 pic_type = (curr[5] >> 3) & 0x7;
                if (pic_type == I_PICTURE ||
                    pic_type == P_PICTURE)
                    m_found_RA_Frame = true;
                else
                {
                    skipped = true;
                    MoveBitstreamData(*in, 6);
                    continue;
                }
            }

            // Not enough buffer, it is possible due to long user data. Try to "cut" it off
            if (out->DataOffset + out->DataLength + (curr - head) > out->MaxLength)
            {
                sts = CutUserData(in, out, curr);
                MFX_CHECK_STS(sts);
            }

            head = in->Data + in->DataOffset;
            sts = AppendBitstream(*out, head, (mfxU32)(curr - head));
            MFX_CHECK_STS(sts);

            MoveBitstreamData(*in, (mfxU32)(curr - head));

            if (m_fcState.picHeader == FcState::FRAME && !VerifyPictureBits(out, curr, tail))
                return MFX_ERR_NOT_ENOUGH_BUFFER; // to start again with next picture

            if (FcState::FRAME == m_fcState.picHeader)
            {
                // If buffer contains less than 8 bytes it means there is no full frame in that buffer
                // and we need to find next start code
                // It is possible in case of corruption when start code is found inside of frame
                if (out->DataLength > 8)
                {
                    m_fcState.picStart = 0;
                    m_fcState.picHeader = FcState::NONE;
                    memset(m_last_bytes, 0, NUM_REST_BYTES);

                    return MFX_ERR_NONE;
                }
            }

            if (ePIC == curr[3])
            {
                // FIXME: for now assume that all pictures are frames
                m_fcState.picHeader = FcState::FRAME;

                if(in->DataFlag == MFX_BITSTREAM_COMPLETE_FRAME)
                {
                    sts = AppendBitstream(*out, curr, 4);
                    MFX_CHECK_STS(sts);

                    MoveBitstreamData(*in, 4);

                    uint32_t len = in->DataLength;

                    head = in->Data + in->DataOffset;
                    curr = FindStartCode(head, tail);

                    MFX_CHECK(curr >= head, MFX_ERR_UNDEFINED_BEHAVIOR);

                    // start code was found
                    if( (tail - curr) > 3 &&
                        (curr[3] == eSEQ  ||
                         curr[3] == eEXT ||
                         curr[3] == ePIC ||
                         curr[3] == eEND ||
                         curr[3] == eGROUP))
                    {
                        len = (uint32_t)(curr - head);
                    }

                    MFX_CHECK(out->MaxLength >= out->DataLength, MFX_ERR_UNDEFINED_BEHAVIOR);

                    if(out->MaxLength - out->DataLength < len)
                    {
                        len = out->MaxLength - out->DataLength;
                    }

                    sts = AppendBitstream(*out, head, len);
                    MFX_CHECK_STS(sts);

                    MoveBitstreamData(*in, len);
                    m_fcState.picStart = 0;
                    m_fcState.picHeader = FcState::NONE;
                    memset(m_last_bytes, 0, NUM_REST_BYTES);

                    return MFX_ERR_NONE;
                }
            }

            sts = AppendBitstream(*out, curr, 4);
            MFX_CHECK_STS(sts);

            MoveBitstreamData(*in, 4);
        }
    }
}

#if defined (MFX_VA_LINUX)

static bool IsStatusReportEnable(VideoCORE * core)
{
    UMC::VideoAccelerator *va;
    core->GetVA((mfxHDL*)&va, MFX_MEMTYPE_FROM_DECODE);

    if (true == va->IsUseStatusReport())
    {
        return true;
    }

    return false;
}

mfxStatus VideoDECODEMPEG2Internal_HW::ConstructFrame(mfxBitstream *bs, mfxFrameSurface1 *surface_work)
{

    return VideoDECODEMPEG2InternalBase::ConstructFrame(bs, surface_work);
}

VideoDECODEMPEG2Internal_HW::VideoDECODEMPEG2Internal_HW()
{
    m_FrameAllocator = new mfx_UMC_FrameAllocator_D3D;
    m_isSWDecoder = false;


    m_implUmcHW = new UMC::MPEG2VideoDecoderHW();
    m_implUmc.reset(m_implUmcHW);
}

mfxStatus VideoDECODEMPEG2Internal_HW::Init(mfxVideoParam *par, VideoCORE * core)
{
    mfxStatus sts = VideoDECODEMPEG2InternalBase::Init(par, core);
    MFX_CHECK_STS(sts);


    m_vdPar.numThreads = 1;

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMPEG2Internal_HW::Reset(mfxVideoParam *par)
{
    mfxStatus sts = VideoDECODEMPEG2InternalBase::Reset(par);
    MFX_CHECK_STS(sts);


    m_NumThreads = 1;


    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMPEG2Internal_HW::Close()
{
    mfxStatus sts = VideoDECODEMPEG2InternalBase::Close();
    return sts;
}

mfxStatus VideoDECODEMPEG2Internal_HW::AllocFrames(mfxVideoParam *par)
{
    mfxStatus sts = VideoDECODEMPEG2InternalBase::AllocFrames(par);
    MFX_CHECK_STS(sts);

    // create video accelerator
    if (m_pCore->CreateVA(par, &allocRequest, &allocResponse, m_FrameAllocator) != MFX_ERR_NONE)
        return MFX_ERR_INVALID_VIDEO_PARAM;

    m_pCore->GetVA((mfxHDL*)&m_vdPar.pVideoAccelerator, MFX_MEMTYPE_FROM_DECODE);
    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMPEG2Internal_HW::GetVideoParam(mfxVideoParam *par)
{

    return VideoDECODEMPEG2InternalBase::GetVideoParam(par);
}


mfxStatus VideoDECODEMPEG2Internal_HW::RestoreDecoder(int32_t frame_buffer_num, UMC::FrameMemID mem_id_to_unlock, int32_t task_num_to_unlock, bool end_frame, bool remove_2frames, int decrease_dec_field_count)
{
    (void)end_frame;

    m_frame[frame_buffer_num].DataLength = 0;
    m_frame[frame_buffer_num].DataOffset = 0;
    m_frame_in_use[frame_buffer_num] = false;

    if (mem_id_to_unlock >= 0)
        m_FrameAllocator->DecreaseReference(mem_id_to_unlock);

    if (task_num_to_unlock >= 0 && task_num_to_unlock < 2*DPB)
    {
        UMC::AutomaticUMCMutex guard(m_guard);
        m_implUmc->UnLockTask(task_num_to_unlock);
    }

#if defined (MFX_VA_LINUX)
    if (end_frame)
        m_implUmcHW->pack_w.m_va->EndFrame();
#endif

    if (remove_2frames)
        m_implUmcHW->RestoreDecoderStateAndRemoveLastField();
    else
        m_implUmcHW->RestoreDecoderState();

    dec_field_count -= decrease_dec_field_count;

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMPEG2Internal_HW::DecodeFrameCheck(mfxBitstream *bs,
                               mfxFrameSurface1 *surface_work,
                               mfxFrameSurface1 **surface_disp,
                               MFX_ENTRY_POINT *pEntryPoint)
{
    int disp_index = -1;

    m_in[m_task_num].SetBufferPointer(m_frame[m_frame_curr].Data + m_frame[m_frame_curr].DataOffset,
                                        m_frame[m_frame_curr].DataLength);

    m_in[m_task_num].SetDataSize(m_frame[m_frame_curr].DataLength);
    m_in[m_task_num].SetTime(m_time[m_frame_curr]);

    UMC::Status umcRes = UMC::UMC_OK;

    bool IsField = false;
    bool IsSkipped = false;

    if (8 < m_frame[m_frame_curr].DataLength)
    {
        m_implUmcHW->SaveDecoderState();
        umcRes = m_implUmc->GetPictureHeader(&m_in[m_task_num], m_task_num, m_prev_task_num);

        //VM_ASSERT( m_implUmc.PictureHeader[m_task_num].picture_coding_type != 3 || ( mid[ m_implUmc.frame_buffer.latest_next ] != -1 && mid[ m_implUmc.frame_buffer.latest_prev ] != -1 ));

        IsField = !m_implUmc->IsFramePictureStructure(m_task_num);
        if (m_task_num >= DPB && !IsField)
        {
            int decrease_dec_field_count = dec_field_count % 2 == 0 ? 0 : 1;
            int32_t previous_field = m_task_num - DPB;
            if (previous_field > DPB)
                return MFX_ERR_UNKNOWN;
            MFX_CHECK_STS(RestoreDecoder(m_frame_curr, mid[previous_field], previous_field, NO_END_FRAME, REMOVE_LAST_2_FRAMES, decrease_dec_field_count))
            return MFX_ERR_MORE_DATA;
        }

        if (UMC::UMC_OK != umcRes)
        {
            MFX_CHECK_STS(RestoreDecoder(m_frame_curr, NO_SURFACE_TO_UNLOCK, NO_TASK_TO_UNLOCK, NO_END_FRAME, REMOVE_LAST_FRAME, 0))

            IsSkipped = m_implUmc->IsFrameSkipped();

            if (IsSkipped && !(dec_field_count & 1))
            {
                skipped_frame_count += 1;
            }

            if (false == m_reset_done && NULL == bs)
            {
                disp_index = m_implUmc->GetDisplayIndex();

                if (0 > disp_index)
                {
                    umcRes = m_implUmc->GetPictureHeader(NULL, m_task_num, m_prev_task_num);

                    if (UMC::UMC_ERR_INVALID_STREAM == umcRes)
                    {
                        return MFX_ERR_UNKNOWN;
                    }

                    disp_index = m_implUmc->GetDisplayIndex();
                }

                if (0 <= disp_index)
                {
                    mfxStatus sts = GetOutputSurface(surface_disp, surface_work, mid[disp_index]);
                    if (sts < MFX_ERR_NONE)
                        return sts;

                    if (true == m_isDecodedOrder)
                    {
                        (*surface_disp)->Data.FrameOrder = 0xffffffff;
                    }

                    SetOutputSurfaceParams(*surface_disp, disp_index);

                    if (false == m_isDecodedOrder)
                    {
                        (*surface_disp)->Data.FrameOrder = display_frame_count;
                    }

                    display_frame_count++;

                    if (true == m_isDecodedOrder)
                    {
                        (*surface_disp)->Data.FrameOrder = display_order;
                        display_order++;
                    }

                    if ((true == IsField && !(dec_field_count & 1)) || false == IsField)
                    {
                        pEntryPoint->requiredNumThreads = m_NumThreads;
                        pEntryPoint->pRoutine = &MPEG2TaskRoutine;
                        pEntryPoint->pState = (void*)this;
                        pEntryPoint->pRoutineName = (char*)"DecodeMPEG2";

                        m_task_param[m_task_num].NumThreads = m_implUmc->GetCurrThreadsNum(m_task_num);
                        m_task_param[m_task_num].m_frame_curr = m_frame_curr;
                        m_task_param[m_task_num].m_isDecodedOrder = m_isDecodedOrder;
                        m_task_param[m_task_num].curr_index = curr_index;
                        m_task_param[m_task_num].prev_index = prev_index;
                        m_task_param[m_task_num].next_index = next_index;
                        m_task_param[m_task_num].surface_out = GetOriginalSurface(*surface_disp);
                        m_task_param[m_task_num].surface_work = surface_work;
                        m_task_param[m_task_num].display_index = disp_index;
                        m_task_param[m_task_num].m_FrameAllocator = m_FrameAllocator;
                        m_task_param[m_task_num].mid = mid;
                        m_task_param[m_task_num].in = &m_in[m_task_num];
                        m_task_param[m_task_num].m_frame = m_frame;
                        m_task_param[m_task_num].m_frame_in_use = m_frame_in_use;
                        m_task_param[m_task_num].task_num = m_task_num;
                        m_task_param[m_task_num].m_isSoftwareBuffer = m_isSWBuf;

                        pEntryPoint->pParam = (void *)(&(m_task_param[m_task_num]));
                    }

                    return MFX_ERR_NONE;
                }
            }

            return MFX_ERR_MORE_DATA;
        }

        m_reset_done = false;

        umcRes = m_implUmc->GetInfo(&m_vdPar);

        mfxStatus sts = UpdateCurrVideoParams(surface_work, m_task_num);
        if (sts != MFX_ERR_NONE && sts != MFX_WRN_VIDEO_PARAM_CHANGED)
            return sts;

        if (surface_work->Info.CropW > surface_work->Info.Width || surface_work->Info.CropH > surface_work->Info.Height)
        {
            return MFX_ERR_UNKNOWN;
        }

        if ((false == m_isDecodedOrder && maxNumFrameBuffered <= (uint32_t)(m_implUmc->GetRetBufferLen())) ||
                true == m_isDecodedOrder)
        {
            disp_index = m_implUmc->GetDisplayIndex();
        }

        if (false == IsField || (true == IsField && !(dec_field_count & 1)))
        {
            curr_index = m_task_num;
            next_index = m_implUmc->GetNextDecodingIndex(curr_index);
            prev_index = m_implUmc->GetPrevDecodingIndex(curr_index);

            UMC::VideoDataInfo Info;

            if (1 == m_implUmc->GetSequenceHeader().progressive_sequence)
            {
                Info.Init(surface_work->Info.Width,
                            (surface_work->Info.CropH + 15) & ~0x0f,
                            m_vdPar.info.color_format);
            }
            else
            {
                Info.Init(surface_work->Info.Width,
                            surface_work->Info.Height,
                            m_vdPar.info.color_format);
            }

            m_FrameAllocator->Alloc(&mid[curr_index], &Info, 0);

            if (0 > mid[curr_index])
            {
                return MFX_ERR_LOCK_MEMORY;
            }

            m_FrameAllocator->IncreaseReference(mid[curr_index]);

#if defined (MFX_VA_LINUX)
            umcRes = m_implUmcHW->pack_w.m_va->BeginFrame(mid[curr_index]);

            if (UMC::UMC_OK != umcRes)
            {
                return MFX_ERR_DEVICE_FAILED;
            }

            m_implUmcHW->pack_w.va_index = mid[curr_index];
#endif
        }
        else
        {
#if defined (MFX_VA_LINUX)
            umcRes = m_implUmcHW->pack_w.m_va->BeginFrame(mid[curr_index]);

            if (UMC::UMC_OK != umcRes)
            {
                return MFX_ERR_DEVICE_FAILED;
            }

            m_implUmcHW->pack_w.va_index = mid[curr_index];
#endif
        }

        mfxStatus s = UpdateWorkSurfaceParams(curr_index);
        MFX_CHECK_STS(s);


        umcRes = m_implUmc->ProcessRestFrame(m_task_num);

        if (UMC::UMC_OK != umcRes)
        {
            MFX_CHECK_STS(RestoreDecoder(m_frame_curr, mid[curr_index], NO_TASK_TO_UNLOCK, END_FRAME, REMOVE_LAST_FRAME, 0))
            return MFX_ERR_MORE_DATA;
        }

        if (true == IsField)
        {
            dec_field_count++;
        }
        else
        {
            dec_field_count += 2;
            cashed_frame_count++;
        }

        dec_frame_count++;

        memset(&m_task_param[m_task_num],0,sizeof(MParam));
        m_implUmc->LockTask(m_task_num);

        umcRes = m_implUmc->DecodeSlices(0, m_task_num);

        if (UMC::UMC_OK != umcRes &&
            UMC::UMC_ERR_NOT_ENOUGH_DATA != umcRes &&
            UMC::UMC_ERR_SYNC != umcRes)
        {
            MFX_CHECK_STS(RestoreDecoder(m_frame_curr, mid[curr_index], m_task_num, END_FRAME, REMOVE_LAST_FRAME, IsField?1:2))
            return MFX_ERR_MORE_DATA;
        }

        umcRes = m_implUmcHW->PostProcessFrame(disp_index, m_task_num);

        if (umcRes != UMC::UMC_OK && umcRes != UMC::UMC_ERR_NOT_ENOUGH_DATA)
        {
            MFX_CHECK_STS(RestoreDecoder(m_frame_curr, mid[curr_index], m_task_num, END_FRAME, REMOVE_LAST_FRAME, IsField?1:2))
            return MFX_ERR_MORE_DATA;
        }

        m_frame[m_frame_curr].DataLength = 0;
        m_frame[m_frame_curr].DataOffset = 0;

        m_prev_task_num = m_task_num;

        if (true == IsField && (dec_field_count & 1))
        {
            if (DPB <= m_task_num)
            {
                return MFX_ERR_UNDEFINED_BEHAVIOR;
            }

            m_task_num += DPB;
        }

        STATUS_REPORT_DEBUG_PRINTF("use task idx %d\n", m_task_num)

        if (0 <= disp_index)
        {
            mfxStatus status = GetOutputSurface(surface_disp, surface_work, mid[disp_index]);
            if (status < MFX_ERR_NONE)
                return status;
        }

        if ((true == IsField && !(dec_field_count & 1)) || false == IsField)
        {
            pEntryPoint->requiredNumThreads = m_NumThreads;
            pEntryPoint->pRoutine = &MPEG2TaskRoutine;
            pEntryPoint->pState = (void*)this;
            pEntryPoint->pRoutineName = (char *)"DecodeMPEG2";

            m_task_param[m_task_num].NumThreads = m_implUmc->GetCurrThreadsNum(m_task_num);
            m_task_param[m_task_num].m_isDecodedOrder = m_isDecodedOrder;
            m_task_param[m_task_num].m_frame_curr = m_frame_curr;

            STATUS_REPORT_DEBUG_PRINTF("frame curr %d\n", m_frame_curr)

            m_task_param[m_task_num].curr_index = curr_index;
            m_task_param[m_task_num].prev_index = prev_index;
            m_task_param[m_task_num].next_index = next_index;
            m_task_param[m_task_num].surface_out = GetOriginalSurface(*surface_disp);
            m_task_param[m_task_num].surface_work = surface_work;
            m_task_param[m_task_num].display_index = disp_index;
            m_task_param[m_task_num].m_FrameAllocator = m_FrameAllocator;
            m_task_param[m_task_num].mid = mid;
            m_task_param[m_task_num].in = &m_in[m_task_num];
            m_task_param[m_task_num].m_frame = m_frame;
            m_task_param[m_task_num].m_frame_in_use = m_frame_in_use;
            m_task_param[m_task_num].task_num = m_task_num;
            m_task_param[m_task_num].m_isSoftwareBuffer = m_isSWBuf;
            m_task_param[m_task_num].IsSWImpl = false;

            pEntryPoint->pParam = (void *)(&(m_task_param[m_task_num]));
        }
        else
        {
            m_frame_in_use[m_frame_curr] = false;
        }

        if (0 <= disp_index)
        {
            SetOutputSurfaceParams(*surface_disp, disp_index);

            if (false == m_isDecodedOrder)
            {
                (*surface_disp)->Data.FrameOrder = display_frame_count;
            }
            else
            {
                (*surface_disp)->Data.FrameOrder = 0xffffffff;
            }

            display_frame_count++;

            if (true == m_isDecodedOrder)
            {
                if (B_PICTURE == m_implUmc->GetFrameType(disp_index))
                {
                    (*surface_disp)->Data.FrameOrder = display_order;
                    display_order++;
                }
                else // I or P
                {
                    int32_t p_index = m_implUmc->GetPrevDecodingIndex(disp_index);

                    if (0 <= p_index)
                    {
                        mfxFrameSurface1 *pSurface;

                        pSurface = m_FrameAllocator->GetSurface(mid[p_index], surface_work, &m_vPar);

                        if (NULL == pSurface)
                        {
                            return MFX_ERR_UNDEFINED_BEHAVIOR;
                        }

                        pSurface->Data.FrameOrder = display_order;
                        display_order++;
                    }
                }
            }

            m_implUmc->PostProcessUserData(disp_index);

            return sts;

        } // display_index >=0

        if (true == m_isDecodedOrder)
        {
            return MFX_ERR_MORE_DATA;
        }

        return MFX_ERR_MORE_SURFACE;

    }
    else // if (8 >= m_frame[m_frame_curr].DataLength)
    {
        last_frame_count++;

        umcRes = m_implUmc->GetPictureHeader(NULL,m_task_num,m_prev_task_num);
        UpdateCurrVideoParams(surface_work, m_task_num);

        if (UMC::UMC_ERR_INVALID_STREAM == umcRes)
        {
            return MFX_ERR_UNKNOWN;
        }

        disp_index = m_implUmc->GetDisplayIndex();

        if (0 <= disp_index)
        {
            if (false == m_isDecodedOrder && ((true == IsField && !(dec_field_count & 1)) || false == IsField))
            {
                mfxStatus sts = GetOutputSurface(surface_disp, surface_work, mid[disp_index]);
                if (sts < MFX_ERR_NONE)
                    return sts;

                pEntryPoint->requiredNumThreads = m_NumThreads;
                pEntryPoint->pRoutine = &MPEG2TaskRoutine;
                pEntryPoint->pState = (void*)this;
                pEntryPoint->pRoutineName = (char *)"DecodeMPEG2";

                SetOutputSurfaceParams(*surface_disp, disp_index);

                memset(&m_task_param[m_task_num],0,sizeof(MParam));
                m_implUmc->LockTask(m_task_num);

                m_task_param[m_task_num].NumThreads = m_implUmc->GetCurrThreadsNum(m_task_num);
                m_task_param[m_task_num].m_isDecodedOrder = m_isDecodedOrder;
                m_task_param[m_task_num].m_frame_curr = m_frame_curr;
                m_task_param[m_task_num].curr_index = curr_index;
                m_task_param[m_task_num].prev_index = prev_index;
                m_task_param[m_task_num].next_index = next_index;
                m_task_param[m_task_num].surface_out = GetOriginalSurface(*surface_disp);
                m_task_param[m_task_num].surface_work = surface_work;
                m_task_param[m_task_num].display_index = disp_index;
                m_task_param[m_task_num].m_FrameAllocator = m_FrameAllocator;
                m_task_param[m_task_num].mid = mid;
                m_task_param[m_task_num].in = &m_in[m_task_num];
                m_task_param[m_task_num].m_frame = m_frame;
                m_task_param[m_task_num].m_frame_in_use = m_frame_in_use;
                m_task_param[m_task_num].task_num = m_task_num;
                m_task_param[m_task_num].m_isSoftwareBuffer = m_isSWBuf;


                pEntryPoint->pParam = (void *)(&(m_task_param[m_task_num]));

                m_prev_task_num = m_task_num;

                m_implUmc->PostProcessUserData(disp_index);

                display_frame_count++;
                return sts;
            }
            else
            {
                mfxFrameSurface1 *pSurface;
                pSurface = m_FrameAllocator->GetSurface(mid[disp_index], surface_work, &m_vPar);

                if (NULL == pSurface)
                {
                    return MFX_ERR_UNDEFINED_BEHAVIOR;
                }

                pSurface->Data.FrameOrder = display_order;
                pSurface->Data.TimeStamp = last_timestamp;

                return MFX_ERR_MORE_DATA;
            }
        }

        return MFX_ERR_MORE_DATA;
    }
}

mfxStatus VideoDECODEMPEG2Internal_HW::TaskRoutine(void *pParam)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VideoDECODEMPEG2Internal_HW::TaskRoutine");
    MParam *parameters = (MParam *)pParam;

    mfxStatus sts = PerformStatusCheck(pParam);
    MFX_CHECK_STS(sts);

    if (0 <= parameters->display_index && true == parameters->m_isSoftwareBuffer)
    {
        sts = parameters->m_FrameAllocator->PrepareToOutput(parameters->surface_out,
                                                            parameters->mid[parameters->display_index],
                                                            &parameters->m_vPar,
                                                            m_isOpaqueMemory);

        MFX_CHECK_STS(sts);
    }

    sts = CompleteTasks(pParam);
    MFX_CHECK_STS(sts);

    return MFX_TASK_DONE;
}

mfxStatus VideoDECODEMPEG2Internal_HW::CompleteTasks(void *pParam)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VideoDECODEMPEG2Internal_HW::CompleteTasks");
    MParam *parameters = (MParam *)pParam;

    UMC::AutomaticUMCMutex guard(m_guard);

    THREAD_DEBUG_PRINTF(
        "(THREAD %x) CompleteTasks: task %x number, task num %d, curr thr idx %d, compl thr %d\n",
        GetCurrentThreadId(), pParam, parameters->task_num, parameters->m_curr_thread_idx, parameters->m_thread_completed)

    int32_t disp_index = parameters->display_index;

    if (0 <= disp_index)
    {
        THREAD_DEBUG_PRINTF("(THREAD %x) Dumping\n", GetCurrentThreadId())
        STATUS_REPORT_DEBUG_PRINTF("thread task idx %d, display address %x\n", parameters->task_num, parameters->surface_out)

        if (B_PICTURE == m_implUmc->GetFrameType(disp_index))
        {
            parameters->m_FrameAllocator->DecreaseReference(parameters->mid[disp_index]);
            parameters->mid[disp_index] = -1;

            m_implUmc->UnLockTask(disp_index);
        }
        else // I or P
        {
            int32_t p_index = m_implUmc->GetPrevDecodingIndex(disp_index);

            if (true == m_isDecodedOrder && 0 <= p_index)
            {
                    p_index = m_implUmc->GetPrevDecodingIndex(p_index);

                if (0 <= p_index)
                {
                    parameters->m_FrameAllocator->DecreaseReference(parameters->mid[p_index]);
                    parameters->mid[p_index] = -1;

                    m_implUmc->UnLockTask(p_index);
                }
            }
            else if (0 <= p_index)
            {
                parameters->m_FrameAllocator->DecreaseReference(parameters->mid[p_index]);
                parameters->mid[p_index] = -1;

                m_implUmc->UnLockTask(p_index);
            }
        }
    }

    parameters->m_frame_in_use[parameters->m_frame_curr] = false;
    STATUS_REPORT_DEBUG_PRINTF("m_frame_curr %d is %d\n", parameters->m_frame_curr, parameters->m_frame_in_use[parameters->m_frame_curr])

    return MFX_TASK_DONE;
}

mfxStatus VideoDECODEMPEG2Internal_HW::GetStatusReportByIndex(int32_t /*current_index*/, mfxU32 /*currIdx*/)
{
    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMPEG2Internal_HW::GetStatusReport(int32_t current_index, UMC::FrameMemID surface_id)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VideoDECODEMPEG2Internal_HW::GetStatusReport");

    UMC::VideoAccelerator *va;
    m_pCore->GetVA((mfxHDL*)&va, MFX_MEMTYPE_FROM_DECODE);

    UMC::Status sts = UMC::UMC_OK;
    mfxU16 surfCorruption = 0;


    sts = va->SyncTask(surface_id, &surfCorruption);

    STATUS_REPORT_DEBUG_PRINTF("index %d with corruption: %d (sts:%d)\n", surface_id, surfCorruption, sts)

    if (sts != UMC::UMC_OK)
    {
        mfxStatus CriticalErrorStatus = (sts == UMC::UMC_ERR_GPU_HANG) ? MFX_ERR_GPU_HANG : MFX_ERR_DEVICE_FAILED;
        SetCriticalErrorOccured(CriticalErrorStatus);
        return CriticalErrorStatus;
    }


    if (surfCorruption)
    {
        m_implUmc->SetCorruptionFlag(current_index);
    }


    return MFX_ERR_NONE;
}

void VideoDECODEMPEG2Internal_HW::TranslateCorruptionFlag(int32_t disp_index, mfxFrameSurface1 * surface)
{
    if (!surface)
    {
        return;
    }

    mfxU32 frameType = m_implUmc->GetFrameType(disp_index);
    int32_t fwd_index = m_implUmc->GetPrevDecodingIndex(disp_index);
    int32_t bwd_index = m_implUmc->GetNextDecodingIndex(disp_index);

    surface->Data.Corrupted = 0;

    if (m_implUmc->GetCorruptionFlag(disp_index))
    {
        surface->Data.Corrupted = MFX_CORRUPTION_MAJOR;
    }

    switch (frameType)
    {
    case I_PICTURE:
        break;

    case P_PICTURE:

        if (fwd_index >= 0 && m_implUmc->GetCorruptionFlag(fwd_index))
        {
            m_implUmc->SetCorruptionFlag(disp_index);
            surface->Data.Corrupted |= MFX_CORRUPTION_REFERENCE_FRAME;
        }

        break;

    case B_PICTURE:

        if ((fwd_index >= 0 && m_implUmc->GetCorruptionFlag(fwd_index))
         || (bwd_index >= 0 && m_implUmc->GetCorruptionFlag(bwd_index)))
        {
            m_implUmc->SetCorruptionFlag(disp_index);
            surface->Data.Corrupted |= MFX_CORRUPTION_REFERENCE_FRAME;
        }

        break;

    default:
        break;
    }

}

mfxStatus VideoDECODEMPEG2Internal_HW::PerformStatusCheck(void *pParam)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VideoDECODEMPEG2Internal_HW::PerformStatusCheck");
    MFX_CHECK_NULL_PTR1(pParam);
    MParam *parameters = (MParam *)pParam;
    int32_t disp_index = parameters->display_index;
    int32_t current_index = parameters->curr_index;

    if (disp_index < 0)
    {
        GetStatusReport(current_index, parameters->mid[current_index]);

        TranslateCorruptionFlag(current_index, parameters->surface_work);

        return MFX_ERR_NONE;
    }


    if (!IsStatusReportEnable(m_pCore))
        return MFX_ERR_NONE;

    mfxStatus sts = GetStatusReport(disp_index, parameters->mid[disp_index]);

    if (MFX_ERR_NONE != sts)
    {
        parameters->m_FrameAllocator->DecreaseReference(parameters->mid[disp_index]);
        parameters->m_frame_in_use[parameters->m_frame_curr] = false;
        return sts;
    }

    mfxU32 frameType = m_implUmc->GetFrameType(disp_index);
    // int32_t fwd_index = m_implUmc->GetPrevDecodingIndex(disp_index);
    int32_t bwd_index = m_implUmc->GetNextDecodingIndex(disp_index);

    switch (frameType)
    {
        case I_PICTURE:
        case P_PICTURE:
            break;
        case B_PICTURE:
            // backward reference
            if (bwd_index >= 0)
            {
                // should be similar to forward reference, but this function called in display order
                // following call is workaround for incorrect time of this function call (previous to next_index)

                GetStatusReport(bwd_index, parameters->mid[bwd_index]);
            }

            break;
        default:
            return MFX_ERR_UNKNOWN;
    }

    TranslateCorruptionFlag(disp_index, parameters->surface_out);

    return MFX_ERR_NONE;
}
#endif // #if defined (MFX_VA_WIN) || defined (MFX_VA_LINUX)


#endif //MFX_ENABLE_MPEG2_VIDEO_DECODE
