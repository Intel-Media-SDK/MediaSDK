// Copyright (c) 2018 Intel Corporation
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

#ifdef UMC_ENABLE_H265_VIDEO_DECODER

#include "umc_h265_dec_defs.h"
#include "umc_h265_task_supplier.h"
#include "umc_h265_nal_spl.h"

#include "mfx_common_decode_int.h"


#include <functional>
#include <algorithm>
#include <iterator>

namespace UMC_HEVC_DECODER { namespace MFX_Utility
{

// Check HW capabilities
bool IsNeedPartialAcceleration_H265(mfxVideoParam* par, eMFXHWType /*type*/)
{
    if (!par)
        return false;

#if defined(MFX_VA_LINUX)
    if (par->mfx.FrameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV420 && par->mfx.FrameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV400)
        return true;

    if (par->mfx.FrameInfo.FourCC == MFX_FOURCC_P210 || par->mfx.FrameInfo.FourCC == MFX_FOURCC_NV16)
        return true;
#endif

    return false;
}

inline
mfxU16 MatchProfile(mfxU32 fourcc)
{
    switch (fourcc)
    {
        case MFX_FOURCC_NV12: return MFX_PROFILE_HEVC_MAIN;

        case MFX_FOURCC_P010: return MFX_PROFILE_HEVC_MAIN10;

        case MFX_FOURCC_NV16: return MFX_PROFILE_HEVC_REXT;

    }

    return MFX_PROFILE_UNKNOWN;
}


inline
bool CheckGUID(VideoCORE * core, eMFXHWType type, mfxVideoParam const* param)
{
    (void)type;

    mfxVideoParam vp = *param;
    mfxU16 profile = vp.mfx.CodecProfile & 0xFF;
    if (profile == MFX_PROFILE_UNKNOWN)
    {
        profile = MatchProfile(vp.mfx.FrameInfo.FourCC);
        vp.mfx.CodecProfile |= profile; //preserve tier
    }

#if defined (MFX_VA_LINUX)
    if (core->IsGuidSupported(DXVA_ModeHEVC_VLD_Main, &vp) != MFX_ERR_NONE)
        return false;

    //Linux doesn't check GUID, just [mfxVideoParam]
    switch (profile)
    {
        case MFX_PROFILE_HEVC_MAIN:
        case MFX_PROFILE_HEVC_MAINSP:
#if defined(ANDROID)
        case MFX_PROFILE_HEVC_MAIN10:
#endif
            return true;
    }

    return false;
#endif
}

// Returns implementation platform
eMFXPlatform GetPlatform_H265(VideoCORE * core, mfxVideoParam * par)
{
    (void)core;

    if (!par)
        return MFX_PLATFORM_SOFTWARE;

    eMFXPlatform platform = core->GetPlatformType();
    eMFXHWType typeHW = MFX_HW_UNKNOWN;
    typeHW = core->GetHWType();

    if (IsNeedPartialAcceleration_H265(par, typeHW) && platform != MFX_PLATFORM_SOFTWARE)
    {
        return MFX_PLATFORM_SOFTWARE;
    }

    if (platform != MFX_PLATFORM_SOFTWARE && !CheckGUID(core, typeHW, par))
        platform = MFX_PLATFORM_SOFTWARE;

    return platform;
}

bool IsBugSurfacePoolApplicable(eMFXHWType hwtype, mfxVideoParam * par)
{
    (void)hwtype;

    if (par == NULL)
        return false;


    return false;
}

inline
mfxU16 QueryMaxProfile(eMFXHWType type)
{

    if (type < MFX_HW_SCL)
        return MFX_PROFILE_HEVC_MAIN;
    else
        return MFX_PROFILE_HEVC_MAIN10;
}

inline
bool CheckChromaFormat(mfxU16 profile, mfxU16 format)
{
    VM_ASSERT(profile != MFX_PROFILE_UNKNOWN);
    VM_ASSERT(!(profile > MFX_PROFILE_HEVC_MAINSP));

    if (format > MFX_CHROMAFORMAT_YUV444)
        return false;

    struct supported_t
    {
        mfxU16 profile;
        mfxI8  chroma[4];
    } static const supported[] =
    {
        { MFX_PROFILE_HEVC_MAIN,   {                      -1, MFX_CHROMAFORMAT_YUV420,                      -1,                      -1 } },
        { MFX_PROFILE_HEVC_MAIN10, {                      -1, MFX_CHROMAFORMAT_YUV420,                      -1,                      -1 } },
        { MFX_PROFILE_HEVC_MAINSP, {                      -1, MFX_CHROMAFORMAT_YUV420,                      -1,                      -1 } },


    };

    supported_t const
        *f = supported,
        *l = f + sizeof(supported) / sizeof(supported[0]);
    for (; f != l; ++f)
        if (f->profile == profile)
            break;

    return
        f != l && (*f).chroma[format] != -1;
}

inline
bool CheckBitDepth(mfxU16 profile, mfxU16 bit_depth)
{
    VM_ASSERT(profile != MFX_PROFILE_UNKNOWN);
    VM_ASSERT(!(profile > MFX_PROFILE_HEVC_MAINSP));

    struct minmax_t
    {
        mfxU16 profile;
        mfxU8  lo, hi;
    } static const minmax[] =
    {
        { MFX_PROFILE_HEVC_MAIN,   8,  8 },
        { MFX_PROFILE_HEVC_MAIN10, 8, 10 },
        { MFX_PROFILE_HEVC_MAINSP, 8,  8 },
    };

    minmax_t const
        *f = minmax,
        *l = f + sizeof(minmax) / sizeof(minmax[0]);
    for (; f != l; ++f)
        if (f->profile == profile)
            break;

    return
        f != l &&
        !(bit_depth < f->lo) &&
        !(bit_depth > f->hi)
        ;
}

inline
mfxU32 CalculateFourcc(mfxU16 codecProfile, mfxFrameInfo const* frameInfo)
{
    //map profile + chroma fmt + bit depth => fcc
    //Main   - [4:2:0], [8] bit
    //Main10 - [4:2:0], [8, 10] bit
    //Extent - [4:2:0, 4:2:2, 4:4:4], [8, 10, 12, 16]

    if (codecProfile > MFX_PROFILE_HEVC_REXT &&
        codecProfile != H265_PROFILE_SCC)
        return 0;

    if (!CheckChromaFormat(codecProfile, frameInfo->ChromaFormat))
        return 0;

    if (!CheckBitDepth(codecProfile, frameInfo->BitDepthLuma))
        return 0;
    if (!CheckBitDepth(codecProfile, frameInfo->BitDepthChroma))
        return 0;

    mfxU16 bit_depth =
       MFX_MAX(frameInfo->BitDepthLuma, frameInfo->BitDepthChroma);

    //map chroma fmt & bit depth onto fourcc (NOTE: we currently don't support bit depth above 10 bit)
    mfxU32 const map[][4] =
    {
            /* 8 bit */      /* 10 bit */
        {               0,               0,               0, 0 }, //400
        { MFX_FOURCC_NV12, MFX_FOURCC_P010,               0, 0 }, //420
        {               0,               0,               0, 0 }, //422
        {               0,               0,               0, 0 }, //444
    };

    VM_ASSERT(
        (frameInfo->ChromaFormat == MFX_CHROMAFORMAT_YUV400 ||
         frameInfo->ChromaFormat == MFX_CHROMAFORMAT_YUV420 ||
         frameInfo->ChromaFormat == MFX_CHROMAFORMAT_YUV422) &&
        "Unsupported chroma format, should be validated before"
    );

    //align luma depth up to 2 (8-10-12 ...)
    bit_depth = (bit_depth + 2 - 1) & ~(2 - 1);
    VM_ASSERT(!(bit_depth & 1) && "Luma depth should be aligned up to 2");

    VM_ASSERT(
        (bit_depth ==  8 ||
         bit_depth == 10) &&
        "Unsupported bit depth, should be validated before"
    );

    mfxU16 const bit_depth_idx     = (bit_depth - 8) / 2;
    mfxU16 const max_bit_depth_idx = sizeof(map) / sizeof(map[0]);

    return bit_depth_idx < max_bit_depth_idx ?
        map[frameInfo->ChromaFormat][bit_depth_idx] : 0;
}

inline
bool CheckFourcc(mfxU32 fourcc, mfxU16 codecProfile, mfxFrameInfo const* frameInfo)
{
    VM_ASSERT(frameInfo);
    mfxFrameInfo fi = *frameInfo;

    if (codecProfile == MFX_PROFILE_UNKNOWN)
        //no profile defined, try to derive it from FOURCC
        codecProfile = MatchProfile(fourcc);

    if (!fi.BitDepthLuma)
    {
        //no depth defined, derive it from FOURCC
        switch (fourcc)
        {
            case MFX_FOURCC_NV12:
            case MFX_FOURCC_NV16:
                fi.BitDepthLuma = 8;
                break;

            case MFX_FOURCC_P010:
            case MFX_FOURCC_P210:
                fi.BitDepthLuma = 10;
                break;

            default:
                return false;
        }
    }

    if (!fi.BitDepthChroma)
        fi.BitDepthChroma = fi.BitDepthLuma;

    return
        CalculateFourcc(codecProfile, &fi) == fourcc;
}

// Initialize mfxVideoParam structure based on decoded bitstream header values
UMC::Status FillVideoParam(const H265SeqParamSet * seq, mfxVideoParam *par, bool full)
{
    par->mfx.CodecId = MFX_CODEC_HEVC;

    par->mfx.FrameInfo.Width = (mfxU16) (seq->pic_width_in_luma_samples);
    par->mfx.FrameInfo.Height = (mfxU16) (seq->pic_height_in_luma_samples);

    par->mfx.FrameInfo.Width = UMC::align_value<mfxU16>(par->mfx.FrameInfo.Width, 16);
    par->mfx.FrameInfo.Height = UMC::align_value<mfxU16>(par->mfx.FrameInfo.Height, 16);

    par->mfx.FrameInfo.BitDepthLuma = (mfxU16) (seq->bit_depth_luma);
    par->mfx.FrameInfo.BitDepthChroma = (mfxU16) (seq->bit_depth_chroma);
    par->mfx.FrameInfo.Shift = 0;

    //if (seq->frame_cropping_flag)
    {
        par->mfx.FrameInfo.CropX = (mfxU16)(seq->conf_win_left_offset + seq->def_disp_win_left_offset);
        par->mfx.FrameInfo.CropY = (mfxU16)(seq->conf_win_top_offset + seq->def_disp_win_top_offset);
        par->mfx.FrameInfo.CropH = (mfxU16)(par->mfx.FrameInfo.Height - (seq->conf_win_top_offset + seq->conf_win_bottom_offset + seq->def_disp_win_top_offset + seq->def_disp_win_bottom_offset));
        par->mfx.FrameInfo.CropW = (mfxU16)(par->mfx.FrameInfo.Width - (seq->conf_win_left_offset + seq->conf_win_right_offset + seq->def_disp_win_left_offset + seq->def_disp_win_right_offset));

        par->mfx.FrameInfo.CropH -= (mfxU16)(par->mfx.FrameInfo.Height - seq->pic_height_in_luma_samples);
        par->mfx.FrameInfo.CropW -= (mfxU16)(par->mfx.FrameInfo.Width - seq->pic_width_in_luma_samples);
    }

    par->mfx.FrameInfo.PicStruct = static_cast<mfxU16>(seq->field_seq_flag  ? MFX_PICSTRUCT_FIELD_SINGLE : MFX_PICSTRUCT_PROGRESSIVE);
    par->mfx.FrameInfo.ChromaFormat = seq->chroma_format_idc;

    if (seq->aspect_ratio_info_present_flag || full)
    {
        par->mfx.FrameInfo.AspectRatioW = (mfxU16)seq->sar_width;
        par->mfx.FrameInfo.AspectRatioH = (mfxU16)seq->sar_height;
    }
    else
    {
        par->mfx.FrameInfo.AspectRatioW = 0;
        par->mfx.FrameInfo.AspectRatioH = 0;
    }

    if (seq->getTimingInfo()->vps_timing_info_present_flag || full)
    {
        par->mfx.FrameInfo.FrameRateExtD = seq->getTimingInfo()->vps_num_units_in_tick;
        par->mfx.FrameInfo.FrameRateExtN = seq->getTimingInfo()->vps_time_scale;
    }
    else
    {
        par->mfx.FrameInfo.FrameRateExtD = 0;
        par->mfx.FrameInfo.FrameRateExtN = 0;
    }

    par->mfx.CodecProfile = (mfxU16)seq->m_pcPTL.GetGeneralPTL()->profile_idc;
    par->mfx.CodecLevel = (mfxU16)seq->m_pcPTL.GetGeneralPTL()->level_idc;
    par->mfx.CodecLevel |=
        seq->m_pcPTL.GetGeneralPTL()->tier_flag ? MFX_TIER_HEVC_HIGH : MFX_TIER_HEVC_MAIN;

    par->mfx.MaxDecFrameBuffering = (mfxU16)seq->sps_max_dec_pic_buffering[0];

    // CodecProfile can't be UNKNOWN here (it comes from SPS), that's asserted at CalculateFourcc
    par->mfx.FrameInfo.FourCC = CalculateFourcc(par->mfx.CodecProfile, &par->mfx.FrameInfo);

    par->mfx.DecodedOrder = 0;

    // video signal section
    mfxExtVideoSignalInfo * videoSignal = (mfxExtVideoSignalInfo *)GetExtendedBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_VIDEO_SIGNAL_INFO);
    if (videoSignal)
    {
        videoSignal->VideoFormat = (mfxU16)seq->video_format;
        videoSignal->VideoFullRange = (mfxU16)seq->video_full_range_flag;
        videoSignal->ColourDescriptionPresent = (mfxU16)seq->colour_description_present_flag;
        videoSignal->ColourPrimaries = (mfxU16)seq->colour_primaries;
        videoSignal->TransferCharacteristics = (mfxU16)seq->transfer_characteristics;
        videoSignal->MatrixCoefficients = (mfxU16)seq->matrix_coeffs;
    }

    mfxExtHEVCParam * hevcParam = (mfxExtHEVCParam *)GetExtendedBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_HEVC_PARAM);
    if (hevcParam)
    {
        hevcParam->PicWidthInLumaSamples = (mfxU16) (seq->pic_width_in_luma_samples);
        hevcParam->PicHeightInLumaSamples = (mfxU16) (seq->pic_height_in_luma_samples);

        hevcParam->GeneralConstraintFlags = 0;
        hevcParam->GeneralConstraintFlags |= seq->getPTL()->GetGeneralPTL()->max_12bit_constraint_flag ? MFX_HEVC_CONSTR_REXT_MAX_12BIT : 0;
        hevcParam->GeneralConstraintFlags |= seq->getPTL()->GetGeneralPTL()->max_10bit_constraint_flag ? MFX_HEVC_CONSTR_REXT_MAX_10BIT : 0;
        hevcParam->GeneralConstraintFlags |= seq->getPTL()->GetGeneralPTL()->max_8bit_constraint_flag ? MFX_HEVC_CONSTR_REXT_MAX_8BIT : 0;
        hevcParam->GeneralConstraintFlags |= seq->getPTL()->GetGeneralPTL()->max_422chroma_constraint_flag ? MFX_HEVC_CONSTR_REXT_MAX_422CHROMA : 0;
        hevcParam->GeneralConstraintFlags |= seq->getPTL()->GetGeneralPTL()->max_420chroma_constraint_flag ? MFX_HEVC_CONSTR_REXT_MAX_420CHROMA : 0;
        hevcParam->GeneralConstraintFlags |= seq->getPTL()->GetGeneralPTL()->max_monochrome_constraint_flag ? MFX_HEVC_CONSTR_REXT_MAX_MONOCHROME : 0;
        hevcParam->GeneralConstraintFlags |= seq->getPTL()->GetGeneralPTL()->intra_constraint_flag ? MFX_HEVC_CONSTR_REXT_INTRA : 0;
        hevcParam->GeneralConstraintFlags |= seq->getPTL()->GetGeneralPTL()->one_picture_only_constraint_flag ? MFX_HEVC_CONSTR_REXT_ONE_PICTURE_ONLY : 0;
        hevcParam->GeneralConstraintFlags |= seq->getPTL()->GetGeneralPTL()->lower_bit_rate_constraint_flag ? MFX_HEVC_CONSTR_REXT_LOWER_BIT_RATE : 0;
    }

    return UMC::UMC_OK;
}

// Helper class for gathering header NAL units
class HeadersAnalyzer
{
public:

    HeadersAnalyzer(TaskSupplier_H265 * supplier)
        : m_isVPSFound(false)
        , m_isSPSFound(false)
        , m_isPPSFound(false)
        , m_supplier(supplier)
        , m_lastSlice(0)
    {}

    virtual ~HeadersAnalyzer()
    {
        if (m_lastSlice)
            m_lastSlice->DecrementReference();
    }

    // Decode a memory buffer looking for header NAL units in it
    virtual UMC::Status DecodeHeader(UMC::MediaData* params, mfxBitstream *bs, mfxVideoParam *out);
    // Find headers nal units and parse them
    virtual UMC::Status ProcessNalUnit(UMC::MediaData * data);
    // Returns whether necessary headers are found
    virtual bool IsEnough() const
    { return m_isSPSFound && m_isPPSFound; }

protected:

    bool m_isVPSFound;
    bool m_isSPSFound;
    bool m_isPPSFound;

    TaskSupplier_H265 * m_supplier;
    H265Slice * m_lastSlice;
};

// Decode a memory buffer looking for header NAL units in it
UMC::Status HeadersAnalyzer::DecodeHeader(UMC::MediaData * data, mfxBitstream *bs, mfxVideoParam *)
{
    if (!data)
        return UMC::UMC_ERR_NULL_PTR;

    m_lastSlice = 0;

    H265SeqParamSet* first_sps = 0;
    notifier0<H265SeqParamSet> sps_guard(&H265Slice::DecrementReference);

    UMC::Status umcRes = UMC::UMC_ERR_NOT_ENOUGH_DATA;
    for ( ; data->GetDataSize() > 3; )
    {
        m_supplier->GetNalUnitSplitter()->MoveToStartCode(data); // move data pointer to start code

        if (!m_isSPSFound) // move point to first start code
        {
            bs->DataOffset = (mfxU32)((mfxU8*)data->GetDataPointer() - (mfxU8*)data->GetBufferPointer());
            bs->DataLength = (mfxU32)data->GetDataSize();
        }

        umcRes = ProcessNalUnit(data);

        if (umcRes == UMC::UMC_ERR_UNSUPPORTED)
            umcRes = UMC::UMC_OK;

        if (umcRes != UMC::UMC_OK)
            break;

        if (!first_sps && m_isSPSFound)
        {
            first_sps = m_supplier->GetHeaders()->m_SeqParams.GetCurrentHeader();
            VM_ASSERT(first_sps && "Current SPS should be valid when [m_isSPSFound]");

            MFX_CHECK_NULL_PTR1(first_sps);

            first_sps->IncrementReference();
            sps_guard.Reset(first_sps);
        }

        if (IsEnough())
            break;
    }

    if (umcRes == UMC::UMC_ERR_SYNC) // move pointer
    {
        bs->DataOffset = (mfxU32)((mfxU8*)data->GetDataPointer() - (mfxU8*)data->GetBufferPointer());
        bs->DataLength = (mfxU32)data->GetDataSize();
        return UMC::UMC_ERR_NOT_ENOUGH_DATA;
    }

    if (umcRes == UMC::UMC_ERR_NOT_ENOUGH_DATA)
    {
        bool isEOS = ((data->GetFlags() & UMC::MediaData::FLAG_VIDEO_DATA_END_OF_STREAM) != 0) ||
            ((data->GetFlags() & UMC::MediaData::FLAG_VIDEO_DATA_NOT_FULL_FRAME) == 0);
        if (isEOS)
        {
            return UMC::UMC_OK;
        }
    }

    if (IsEnough())
    {
        H265SeqParamSet* last_sps = m_supplier->GetHeaders()->m_SeqParams.GetCurrentHeader();
        if (first_sps && first_sps != last_sps)
            m_supplier->GetHeaders()->m_SeqParams.AddHeader(first_sps);

        return UMC::UMC_OK;
    }

    return UMC::UMC_ERR_NOT_ENOUGH_DATA;
}

// Find headers nal units and parse them
UMC::Status HeadersAnalyzer::ProcessNalUnit(UMC::MediaData * data)
{
    try
    {
        int32_t startCode = m_supplier->GetNalUnitSplitter()->CheckNalUnitType(data);

        bool needProcess = false;

        UMC::MediaDataEx *nalUnit = m_supplier->GetNalUnit(data);

        switch (startCode)
        {
            case NAL_UT_CODED_SLICE_TRAIL_R:
            case NAL_UT_CODED_SLICE_TRAIL_N:
            case NAL_UT_CODED_SLICE_TLA_R:
            case NAL_UT_CODED_SLICE_TSA_N:
            case NAL_UT_CODED_SLICE_STSA_R:
            case NAL_UT_CODED_SLICE_STSA_N:
            case NAL_UT_CODED_SLICE_BLA_W_LP:
            case NAL_UT_CODED_SLICE_BLA_W_RADL:
            case NAL_UT_CODED_SLICE_BLA_N_LP:
            case NAL_UT_CODED_SLICE_IDR_W_RADL:
            case NAL_UT_CODED_SLICE_IDR_N_LP:
            case NAL_UT_CODED_SLICE_CRA:
            case NAL_UT_CODED_SLICE_RADL_R:
            case NAL_UT_CODED_SLICE_RASL_R:
            {
                if (IsEnough())
                {
                    return UMC::UMC_OK;
                }
                else
                    break; // skip nal unit
            }
            break;

        case NAL_UT_VPS:
        case NAL_UT_SPS:
        case NAL_UT_PPS:
            needProcess = true;
            break;

        default:
            break;
        };

        if (!nalUnit)
        {
            return UMC::UMC_ERR_NOT_ENOUGH_DATA;
        }

        if (needProcess)
        {
            try
            {
                UMC::Status umcRes = m_supplier->ProcessNalUnit(nalUnit);
                if (umcRes < UMC::UMC_OK)
                {
                    return UMC::UMC_OK;
                }
            }
            catch(h265_exception& ex)
            {
                if (ex.GetStatus() != UMC::UMC_ERR_UNSUPPORTED)
                {
                    throw;
                }
            }

            switch (startCode)
            {
            case NAL_UT_VPS:
                m_isVPSFound = true;
                break;

            case NAL_UT_SPS:
                m_isSPSFound = true;
                break;

            case NAL_UT_PPS:
                m_isPPSFound = true;
                break;

            default:
                break;
            };

            return UMC::UMC_OK;
        }
    }
    catch(const h265_exception & ex)
    {
        return ex.GetStatus();
    }

    return UMC::UMC_OK;
}

// Find bitstream header NAL units, parse them and fill application parameters structure
UMC::Status DecodeHeader(TaskSupplier_H265 * supplier, UMC::VideoDecoderParams* params, mfxBitstream *bs, mfxVideoParam *out)
{
    UMC::Status umcRes = UMC::UMC_OK;

    if (!params->m_pData)
        return UMC::UMC_ERR_NULL_PTR;

    if (!params->m_pData->GetDataSize())
        return UMC::UMC_ERR_NOT_ENOUGH_DATA;

    umcRes = supplier->PreInit(params);
    if (umcRes != UMC::UMC_OK)
        return UMC::UMC_ERR_FAILED;

    HeadersAnalyzer headersDecoder(supplier);
    umcRes = headersDecoder.DecodeHeader(params->m_pData, bs, out);

    if (umcRes != UMC::UMC_OK)
        return umcRes;

    return
        umcRes = supplier->GetInfo(params);
}

// MediaSDK DECODE_Query API function
mfxStatus Query_H265(VideoCORE *core, mfxVideoParam *in, mfxVideoParam *out, eMFXHWType type)
{
    MFX_CHECK_NULL_PTR1(out);
    mfxStatus  sts = MFX_ERR_NONE;

    if (in == out)
    {
        mfxVideoParam in1;
        MFX_INTERNAL_CPY(&in1, in, sizeof(mfxVideoParam));
        return MFX_Utility::Query_H265(core, &in1, out, type);
    }

    memset(&out->mfx, 0, sizeof(mfxInfoMFX));

    if (in)
    {
        if (in->mfx.CodecId == MFX_CODEC_HEVC)
            out->mfx.CodecId = in->mfx.CodecId;

        //use [core :: GetHWType] instead of given argument [type]
        //because it may be unknown after [GetPlatform_H265]
        mfxU16 profile = QueryMaxProfile(core->GetHWType());
        if (in->mfx.CodecProfile == MFX_PROFILE_HEVC_MAINSP ||
            in->mfx.CodecProfile <= profile)
            out->mfx.CodecProfile = in->mfx.CodecProfile;
        else
        {
            sts = MFX_ERR_UNSUPPORTED;
        }

        if (out->mfx.CodecProfile != MFX_PROFILE_UNKNOWN)
            profile = out->mfx.CodecProfile;

        mfxU32 const level =
            ExtractProfile(in->mfx.CodecLevel);

        switch (level)
        {
        case MFX_LEVEL_UNKNOWN:
        case MFX_LEVEL_HEVC_1:
        case MFX_LEVEL_HEVC_2:
        case MFX_LEVEL_HEVC_21:
        case MFX_LEVEL_HEVC_3:
        case MFX_LEVEL_HEVC_31:
        case MFX_LEVEL_HEVC_4:
        case MFX_LEVEL_HEVC_41:
        case MFX_LEVEL_HEVC_5:
        case MFX_LEVEL_HEVC_51:
        case MFX_LEVEL_HEVC_52:
        case MFX_LEVEL_HEVC_6:
        case MFX_LEVEL_HEVC_61:
        case MFX_LEVEL_HEVC_62:
            out->mfx.CodecLevel = in->mfx.CodecLevel;
            break;
        default:
            sts = MFX_ERR_UNSUPPORTED;
            break;
        }

        if (in->mfx.NumThread < 128)
        {
            out->mfx.NumThread = in->mfx.NumThread;
        }
        else
        {
            sts = MFX_ERR_UNSUPPORTED;
        }

        out->AsyncDepth = in->AsyncDepth;

        out->mfx.DecodedOrder = in->mfx.DecodedOrder;

        if (in->mfx.DecodedOrder > 1)
        {
            sts = MFX_ERR_UNSUPPORTED;
            out->mfx.DecodedOrder = 0;
        }

        if (in->mfx.TimeStampCalc)
        {
            if (in->mfx.TimeStampCalc == 1)
                in->mfx.TimeStampCalc = out->mfx.TimeStampCalc;
            else
                sts = MFX_ERR_UNSUPPORTED;
        }

        if (in->mfx.ExtendedPicStruct)
        {
            if (in->mfx.ExtendedPicStruct == 1)
                in->mfx.ExtendedPicStruct = out->mfx.ExtendedPicStruct;
            else
                sts = MFX_ERR_UNSUPPORTED;
        }

        if ((in->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY) || (in->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) ||
            (in->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY))
        {
            uint32_t mask = in->IOPattern & 0xf0;
            if (mask == MFX_IOPATTERN_OUT_VIDEO_MEMORY || mask == MFX_IOPATTERN_OUT_SYSTEM_MEMORY || mask == MFX_IOPATTERN_OUT_OPAQUE_MEMORY)
                out->IOPattern = in->IOPattern;
            else
                sts = MFX_ERR_UNSUPPORTED;
        }

        if (in->mfx.FrameInfo.FourCC)
        {
            // mfxFrameInfo
            if (in->mfx.FrameInfo.FourCC == MFX_FOURCC_NV12 ||
                in->mfx.FrameInfo.FourCC == MFX_FOURCC_P010 ||
                in->mfx.FrameInfo.FourCC == MFX_FOURCC_P210
                )
                out->mfx.FrameInfo.FourCC = in->mfx.FrameInfo.FourCC;
            else
                sts = MFX_ERR_UNSUPPORTED;
        }

        if (in->mfx.FrameInfo.ChromaFormat == MFX_CHROMAFORMAT_YUV400 ||
            CheckChromaFormat(profile, in->mfx.FrameInfo.ChromaFormat))
            out->mfx.FrameInfo.ChromaFormat = in->mfx.FrameInfo.ChromaFormat;
        else
            sts = MFX_ERR_UNSUPPORTED;

        if (in->mfx.FrameInfo.Width % 16 == 0 && in->mfx.FrameInfo.Width <= 16384)
            out->mfx.FrameInfo.Width = in->mfx.FrameInfo.Width;
        else
        {
            out->mfx.FrameInfo.Width = 0;
            sts = MFX_ERR_UNSUPPORTED;
        }

        if (in->mfx.FrameInfo.Height % 16 == 0 && in->mfx.FrameInfo.Height <= 16384)
            out->mfx.FrameInfo.Height = in->mfx.FrameInfo.Height;
        else
        {
            out->mfx.FrameInfo.Height = 0;
            sts = MFX_ERR_UNSUPPORTED;
        }

        if ((in->mfx.FrameInfo.Width || in->mfx.FrameInfo.Height) && !(in->mfx.FrameInfo.Width && in->mfx.FrameInfo.Height))
        {
            out->mfx.FrameInfo.Width = 0;
            out->mfx.FrameInfo.Height = 0;
            sts = MFX_ERR_UNSUPPORTED;
        }

        out->mfx.FrameInfo.FrameRateExtN = in->mfx.FrameInfo.FrameRateExtN;
        out->mfx.FrameInfo.FrameRateExtD = in->mfx.FrameInfo.FrameRateExtD;

        if ((in->mfx.FrameInfo.FrameRateExtN || in->mfx.FrameInfo.FrameRateExtD) && !(in->mfx.FrameInfo.FrameRateExtN && in->mfx.FrameInfo.FrameRateExtD))
        {
            out->mfx.FrameInfo.FrameRateExtN = 0;
            out->mfx.FrameInfo.FrameRateExtD = 0;
            sts = MFX_ERR_UNSUPPORTED;
        }

        out->mfx.FrameInfo.AspectRatioW = in->mfx.FrameInfo.AspectRatioW;
        out->mfx.FrameInfo.AspectRatioH = in->mfx.FrameInfo.AspectRatioH;

        if ((in->mfx.FrameInfo.AspectRatioW || in->mfx.FrameInfo.AspectRatioH) && !(in->mfx.FrameInfo.AspectRatioW && in->mfx.FrameInfo.AspectRatioH))
        {
            out->mfx.FrameInfo.AspectRatioW = 0;
            out->mfx.FrameInfo.AspectRatioH = 0;
            sts = MFX_ERR_UNSUPPORTED;
        }

        out->mfx.FrameInfo.BitDepthLuma = in->mfx.FrameInfo.BitDepthLuma;
        if (in->mfx.FrameInfo.BitDepthLuma && !CheckBitDepth(profile, in->mfx.FrameInfo.BitDepthLuma))
        {
            out->mfx.FrameInfo.BitDepthLuma = 0;
            sts = MFX_ERR_UNSUPPORTED;
        }

        out->mfx.FrameInfo.BitDepthChroma = in->mfx.FrameInfo.BitDepthChroma;
        if (in->mfx.FrameInfo.BitDepthChroma && !CheckBitDepth(profile, in->mfx.FrameInfo.BitDepthChroma))
        {
            out->mfx.FrameInfo.BitDepthChroma = 0;
            sts = MFX_ERR_UNSUPPORTED;
        }

        if (in->mfx.FrameInfo.FourCC &&
            !CheckFourcc(in->mfx.FrameInfo.FourCC, profile, &in->mfx.FrameInfo))
        {
            out->mfx.FrameInfo.FourCC = 0;
            sts = MFX_ERR_UNSUPPORTED;
        }

        out->mfx.FrameInfo.Shift = in->mfx.FrameInfo.Shift;
        if (   in->mfx.FrameInfo.FourCC == MFX_FOURCC_P010 || in->mfx.FrameInfo.FourCC == MFX_FOURCC_P210
            )
        {
            if (in->mfx.FrameInfo.Shift > 1)
            {
                out->mfx.FrameInfo.Shift = 0;
                sts = MFX_ERR_UNSUPPORTED;
            }
        }
        else
        {
            if (in->mfx.FrameInfo.Shift)
            {
                out->mfx.FrameInfo.Shift = 0;
                sts = MFX_ERR_UNSUPPORTED;
            }
        }

        switch (in->mfx.FrameInfo.PicStruct)
        {
        case MFX_PICSTRUCT_UNKNOWN:
        case MFX_PICSTRUCT_PROGRESSIVE:
        case MFX_PICSTRUCT_FIELD_SINGLE:
            out->mfx.FrameInfo.PicStruct = in->mfx.FrameInfo.PicStruct;
            break;
        default:
            sts = MFX_ERR_UNSUPPORTED;
            break;
        }

        mfxStatus stsExt = CheckDecodersExtendedBuffers(in);
        if (stsExt < MFX_ERR_NONE)
            sts = MFX_ERR_UNSUPPORTED;


        if (GetPlatform_H265(core, out) != core->GetPlatformType() && sts == MFX_ERR_NONE)
        {
            VM_ASSERT(GetPlatform_H265(core, out) == MFX_PLATFORM_SOFTWARE);
            sts = MFX_ERR_UNSUPPORTED;
        }
    }
    else
    {
        out->mfx.CodecId = MFX_CODEC_HEVC;
        out->mfx.CodecProfile = 1;
        out->mfx.CodecLevel = 1;

        out->mfx.NumThread = 1;

        out->mfx.DecodedOrder = 1;

        out->mfx.SliceGroupsPresent = 1;
        out->mfx.ExtendedPicStruct = 1;
        out->AsyncDepth = 1;

        // mfxFrameInfo
        out->mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
        out->mfx.FrameInfo.Width = 16;
        out->mfx.FrameInfo.Height = 16;

        out->mfx.FrameInfo.FrameRateExtN = 1;
        out->mfx.FrameInfo.FrameRateExtD = 1;

        out->mfx.FrameInfo.AspectRatioW = 1;
        out->mfx.FrameInfo.AspectRatioH = 1;

        out->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;

        out->mfx.FrameInfo.BitDepthLuma = 8;
        out->mfx.FrameInfo.BitDepthChroma = 8;
        out->mfx.FrameInfo.Shift = 0;

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
    }

    return sts;
}

// Validate input parameters
bool CheckVideoParam_H265(mfxVideoParam *in, eMFXHWType /* type */)
{
    if (!in)
        return false;

    if (MFX_CODEC_HEVC != in->mfx.CodecId)
        return false;

    // FIXME: Add check that width is multiple of minimal CU size
    if (in->mfx.FrameInfo.Width > 16384 /* || (in->mfx.FrameInfo.Width % in->mfx.FrameInfo.reserved[0]) */)
        return false;

    // FIXME: Add check that height is multiple of minimal CU size
    if (in->mfx.FrameInfo.Height > 16384 /* || (in->mfx.FrameInfo.Height % in->mfx.FrameInfo.reserved[0]) */)
        return false;


    if (in->mfx.FrameInfo.FourCC != MFX_FOURCC_NV12 &&
        in->mfx.FrameInfo.FourCC != MFX_FOURCC_NV16 &&
        in->mfx.FrameInfo.FourCC != MFX_FOURCC_P010 &&
        in->mfx.FrameInfo.FourCC != MFX_FOURCC_P210
        )
        return false;

    // both zero or not zero
    if ((in->mfx.FrameInfo.AspectRatioW || in->mfx.FrameInfo.AspectRatioH) && !(in->mfx.FrameInfo.AspectRatioW && in->mfx.FrameInfo.AspectRatioH))
        return false;

    if (in->mfx.CodecProfile != MFX_PROFILE_HEVC_MAIN &&
        in->mfx.CodecProfile != MFX_PROFILE_HEVC_MAIN10 &&
        in->mfx.CodecProfile != MFX_PROFILE_HEVC_MAINSP &&
        in->mfx.CodecProfile != MFX_PROFILE_HEVC_REXT
        )
        return false;

    //BitDepthLuma & BitDepthChroma is also checked here
    if (!CheckFourcc(in->mfx.FrameInfo.FourCC, in->mfx.CodecProfile, &in->mfx.FrameInfo))
        return false;

    if (   in->mfx.FrameInfo.FourCC == MFX_FOURCC_P010 || in->mfx.FrameInfo.FourCC == MFX_FOURCC_P210
        )
    {
        if (in->mfx.FrameInfo.Shift > 1)
            return false;
    }
    else
    {
        if (in->mfx.FrameInfo.Shift)
            return false;
    }

    switch (in->mfx.FrameInfo.PicStruct)
    {
    case MFX_PICSTRUCT_UNKNOWN:
    case MFX_PICSTRUCT_PROGRESSIVE:
    case MFX_PICSTRUCT_FIELD_TFF:
    case MFX_PICSTRUCT_FIELD_BFF:
    case MFX_PICSTRUCT_FIELD_REPEATED:
    case MFX_PICSTRUCT_FRAME_DOUBLING:
    case MFX_PICSTRUCT_FRAME_TRIPLING:
    case MFX_PICSTRUCT_FIELD_SINGLE:
        break;
    default:
        return false;
    }

    if (in->mfx.FrameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV400 &&
        in->mfx.FrameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV420 &&
        in->mfx.FrameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV422
        )
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

} } // namespace UMC_HEVC_DECODER

#endif // UMC_ENABLE_H265_VIDEO_DECODER
