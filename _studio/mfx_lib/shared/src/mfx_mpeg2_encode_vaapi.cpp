// Copyright (c) 2018-2021 Intel Corporation
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

#if defined(MFX_ENABLE_MPEG2_VIDEO_ENCODE) && defined(MFX_VA_LINUX)

#include <va/va.h>
#include <assert.h>

#include "mfx_session.h"
#include "libmfx_core_vaapi.h"
#include "mfx_common_int.h"
#include "mfx_mpeg2_encode_vaapi.h"
#include "vaapi_ext_interface.h"
#include <climits>
#include "fast_copy.h"

#ifndef D3DDDIFMT_NV12
#define D3DDDIFMT_NV12 (D3DDDIFORMAT)(MFX_MAKEFOURCC('N', 'V', '1', '2'))
#endif

#define CODEC_MPEG2_ENC_FCODE_X(width) ((width < 200) ? 3 : (width < 500) ? 4 : (width < 1400) ? 5 : 6)
#define CODEC_MPEG2_ENC_FCODE_Y(fcodeX) ((fcodeX > 5) ? 5 : fcodeX)


namespace
{

    uint32_t ConvertRateControlMFX2VAAPI(mfxU8 rateControl)
    {
        switch (rateControl)
        {
        case MFX_RATECONTROL_CBR:  return VA_RC_CBR;
        case MFX_RATECONTROL_VBR:  return VA_RC_VBR;
        case MFX_RATECONTROL_AVBR: return VA_RC_VBR;
        case MFX_RATECONTROL_CQP:  return VA_RC_CQP;
        default: assert(!"Unsupported RateControl"); return 0;
        }

    }

    VAProfile ConvertProfileTypeMFX2VAAPI(mfxU8 type)
    {
        switch (type)
        {
            case MFX_PROFILE_MPEG2_SIMPLE: return VAProfileMPEG2Simple;
            case MFX_PROFILE_MPEG2_MAIN:   return VAProfileMPEG2Main;
            case MFX_PROFILE_MPEG2_HIGH:   /*assert(!"Unsupported profile type"); */ return VAProfileMPEG2Main;
            default: assert(!"Unsupported profile type"); return VAProfileNone;
        }

    } // VAProfile ConvertProfileTypeMFX2VAAPI(mfxU8 type)

    VAEncPictureType ConvertCodingTypeMFX2VAAPI(UCHAR codingType)
    {
        switch (codingType)
        {
            case CODING_TYPE_I: return VAEncPictureTypeIntra;
            case CODING_TYPE_P: return VAEncPictureTypePredictive;
            case CODING_TYPE_B: return VAEncPictureTypeBidirectional;
            default: assert(!"Unsupported picture coding type"); return (VAEncPictureType)-1;
        }
    } // VAEncPictureType ConvertCodingTypeMFX2VAAPI(UCHAR codingType)

    VASurfaceID ConvertSurfaceIdMFX2VAAPI(VideoCORE* core, mfxMemId id)
    {
        mfxStatus sts;
        VASurfaceID *pSurface = NULL;
        MFX_CHECK_WITH_ASSERT(core, VA_INVALID_SURFACE);
        sts = core->GetFrameHDL(id, (mfxHDL *)&pSurface);
        MFX_CHECK_WITH_ASSERT(MFX_ERR_NONE == sts, VA_INVALID_SURFACE);
        return *pSurface;
    }

    VASurfaceID ConvertSurfaceIdMFX2VAAPIwNULL(VideoCORE* core, mfxMemId id)
    {
        mfxStatus sts;
        VASurfaceID *pSurface = NULL;
        MFX_CHECK_WITH_ASSERT(core, VA_INVALID_SURFACE);
        if (id != 0)
        {
            sts = core->GetFrameHDL(id, (mfxHDL *)&pSurface);
            MFX_CHECK_WITH_ASSERT(MFX_ERR_NONE == sts, VA_INVALID_SURFACE);
        }
        else
        {
            return VA_INVALID_SURFACE;
        }
        return *pSurface;
    }


    /*int mpeg2enc_time_code(VAEncSequenceParameterBufferMPEG2 *seq_param, int num_frames)
    {
        int fps = (int)(seq_param->frame_rate + 0.5);
        int time_code = 0;
        int time_code_pictures, time_code_seconds, time_code_minutes, time_code_hours;
        int drop_frame_flag = 0;

        assert(fps <= 60);

        time_code_seconds = num_frames / fps;
        time_code_pictures = num_frames % fps;
        time_code |= time_code_pictures;

        time_code_minutes = time_code_seconds / 60;
        time_code_seconds = time_code_seconds % 60;
        time_code |= (time_code_seconds << 6);

        time_code_hours = time_code_minutes / 60;
        time_code_minutes = time_code_minutes % 60;

        time_code |= (1 << 12);     // marker_bit
        time_code |= (time_code_minutes << 13);

        time_code_hours = time_code_hours % 24;
        time_code |= (time_code_hours << 19);

        time_code |= (drop_frame_flag << 24);

        return time_code;
    }*/


    void FillSps(
        MfxHwMpeg2Encode::ExecuteBuffers* pExecuteBuffers,
        VAEncSequenceParameterBufferMPEG2 & sps)
    {
        assert(pExecuteBuffers);
        const ENCODE_SET_SEQUENCE_PARAMETERS_MPEG2 & winSps = pExecuteBuffers->m_sps;
        const ENCODE_SET_PICTURE_PARAMETERS_MPEG2 & winPps = pExecuteBuffers->m_pps;

        sps.picture_width   = winSps.FrameWidth;
        sps.picture_height  = winSps.FrameHeight;

        if (winSps.FrameRateCode > 0 && winSps.FrameRateCode <= 8)
        {
          const double ratetab[8]=
            {24000.0/1001.0, 24.0, 25.0, 30000.0/1001.0, 30.0, 50.0, 60000.0/1001.0, 60.0};
          sps.frame_rate = ratetab[winSps.FrameRateCode - 1]
            * (winSps.FrameRateExtN + 1) / (winSps.FrameRateExtD + 1);
        } else
          assert(!"Unknown FrameRateCode appeared.");

        sps.aspect_ratio_information = winSps.AspectRatio;
        // sps.vbv_buffer_size = winSps.vbv_buffer_size; // B = 16 * 1024 * vbv_buffer_size

        sps.intra_period    = pExecuteBuffers->m_GOPPictureSize;
        sps.ip_period       = pExecuteBuffers->m_GOPRefDist;
        // For VBR maxBitrate should be used as sps parameter, target bitrate has no place in sps, only in BRC
        sps.bits_per_second = (winSps.RateControlMethod == MFX_RATECONTROL_VBR && winSps.MaxBitRate > winSps.bit_rate) ?
            winSps.MaxBitRate : winSps.bit_rate;
        if (winSps.vbv_buffer_size)
        {
            sps.vbv_buffer_size = winSps.vbv_buffer_size;
        }
        else
        {
            sps.vbv_buffer_size = winSps.bit_rate * 45 / 1000;
        }

        //sps.vbv_buffer_size = 3; // B = 16 * 1024 * vbv_buffer_size

        int profile = 4, level = 8;

        switch (ConvertProfileTypeMFX2VAAPI(winSps.Profile))
        {
        case VAProfileMPEG2Simple:
            profile = 5;
            break;
        case VAProfileMPEG2Main:
            profile = 4;
            break;
        default:
            assert(0);
            break;
        }

        switch (winSps.Level)
        {
        case MFX_LEVEL_MPEG2_LOW:
            level = 10;
            break;
        case MFX_LEVEL_MPEG2_MAIN:
            level = 8;
            break;
        case MFX_LEVEL_MPEG2_HIGH1440:
            level = 6;
            break;
        case MFX_LEVEL_MPEG2_HIGH:
            level = 4;
            break;
        default:
            assert(0);
            break;
        }
        sps.sequence_extension.bits.profile_and_level_indication = profile << 4 | level;
        sps.sequence_extension.bits.chroma_format = 1; // CHROMA_FORMAT_420; // 4:2:0
        sps.sequence_extension.bits.frame_rate_extension_n = winSps.FrameRateExtN;
        sps.sequence_extension.bits.frame_rate_extension_d = winSps.FrameRateExtD;
        sps.sequence_extension.bits.progressive_sequence   = winSps.progressive_sequence;
        sps.sequence_extension.bits.low_delay              = winSps.low_delay; // FIXME
        sps.new_gop_header = winPps.bNewGop;
        sps.gop_header.bits.time_code = (1 << 12); // bit12: marker_bit
        if (sps.new_gop_header)
        {
            sps.gop_header.bits.time_code = winPps.time_code;
        }

        sps.gop_header.bits.closed_gop = (winPps.GopOptFlag & MFX_GOP_CLOSED) ? 1 : 0;


        if (winPps.picture_coding_type == CODING_TYPE_I)
        {
            sps.gop_header.bits.broken_link = (winPps.GopOptFlag & MFX_GOP_CLOSED) || (winPps.GopRefDist == 1);
        }


    } // void FillSps(...)

    void FillPps(
        VideoCORE * core,
        MfxHwMpeg2Encode::ExecuteBuffers* pExecuteBuffers,
        VAEncPictureParameterBufferMPEG2 & pps,
        mfxU8 *pUserData, mfxU32 userDataLen)
    {
        assert(pExecuteBuffers);
        const ENCODE_SET_PICTURE_PARAMETERS_MPEG2 & winPps = pExecuteBuffers->m_pps;

        pps.picture_type = ConvertCodingTypeMFX2VAAPI(winPps.picture_coding_type);
        pps.temporal_reference = winPps.temporal_reference;
        pps.vbv_delay = winPps.vbv_delay;

        pps.reconstructed_picture = ConvertSurfaceIdMFX2VAAPI(core, pExecuteBuffers->m_RecFrameMemID);

        pps.coded_buf = VA_INVALID_ID;

        pps.f_code[0][0] = winPps.f_code00;
        pps.f_code[0][1] = winPps.f_code01;
        pps.f_code[1][0] = winPps.f_code10;
        pps.f_code[1][1] = winPps.f_code11;

//        pps.user_data_length = 0;

        pps.picture_coding_extension.bits.intra_dc_precision         = winPps.intra_dc_precision; /* 8bits */
        pps.picture_coding_extension.bits.picture_structure          = 3; /* frame picture */
        pps.picture_coding_extension.bits.top_field_first            = winPps.InterleavedFieldBFF == 0 ? 1 : 0;
        pps.picture_coding_extension.bits.frame_pred_frame_dct       = winPps.frame_pred_frame_dct;//1; /* FIXME */
        pps.picture_coding_extension.bits.concealment_motion_vectors = winPps.concealment_motion_vectors;
        pps.picture_coding_extension.bits.q_scale_type               = winPps.q_scale_type;
        pps.picture_coding_extension.bits.intra_vlc_format           = winPps.intra_vlc_format;
        pps.picture_coding_extension.bits.alternate_scan             = winPps.alternate_scan;
        pps.picture_coding_extension.bits.repeat_first_field         = winPps.repeat_first_field;
        pps.picture_coding_extension.bits.progressive_frame          = (winPps.FieldCodingFlag == 0) && (winPps.FieldFrameCodingFlag == 0) ? 1 : 0;
        pps.picture_coding_extension.bits.composite_display_flag     = winPps.composite_display_flag;

        pps.composite_display.bits.v_axis            = winPps.v_axis;
        pps.composite_display.bits.field_sequence    = winPps.field_sequence;
        pps.composite_display.bits.sub_carrier       = winPps.sub_carrier;
        pps.composite_display.bits.burst_amplitude   = winPps.burst_amplitude;
        pps.composite_display.bits.sub_carrier_phase = winPps.sub_carrier_phase;

        pps.last_picture = winPps.bLastPicInStream;

        if (pps.picture_type == VAEncPictureTypeIntra)
        {
            pps.forward_reference_picture = VA_INVALID_SURFACE;
            pps.backward_reference_picture = VA_INVALID_SURFACE;
        }
        else if (pps.picture_type == VAEncPictureTypePredictive)
        {
            pps.forward_reference_picture = ConvertSurfaceIdMFX2VAAPI(core, pExecuteBuffers->m_RefFrameMemID[0]);
            pps.backward_reference_picture = VA_INVALID_SURFACE;
        }
        else if (pps.picture_type == VAEncPictureTypeBidirectional)
        {
            pps.forward_reference_picture = ConvertSurfaceIdMFX2VAAPIwNULL(core, pExecuteBuffers->m_RefFrameMemID[0]);
            pps.backward_reference_picture = ConvertSurfaceIdMFX2VAAPIwNULL(core, pExecuteBuffers->m_RefFrameMemID[1]);
        } else
        {
            assert(0);
        }

//         if (pUserData && userDataLen > 4)
//         {
//             mfxU32 len = std:min<mfxU32>(userDataLen - 4, sizeof(pps.user_data)/sizeof(pps.user_data[0]));
//             std::copy(pUserData + 4, pUserData + 4 + len, pps.user_data);
//             pps.user_data_length = len;
//         }
    } // void FillPps(...)

    template<class T> inline void Zero(T & obj)                { memset(&obj, 0, sizeof(obj)); }
    template<class T> inline void Zero(std::vector<T> & vec)   { memset(&vec[0], 0, sizeof(T) * vec.size()); }
    template<class T> inline void Zero(T * first, size_t cnt)  { memset(first, 0, sizeof(T) * cnt); }


} // anonymous namespace


using namespace MfxHwMpeg2Encode;

VAAPIEncoder::VAAPIEncoder(VideoCORE* core)
    : m_core(core)
    , m_vaDisplay(0)
    , m_vaContextEncode(VA_INVALID_ID)
    , m_vaConfig(VA_INVALID_ID)
    , m_vaSpsBuf()
    , m_spsBufferId(VA_INVALID_ID)
    , m_vaPpsBuf()
    , m_ppsBufferId(VA_INVALID_ID)
    , m_qmBufferId(VA_INVALID_ID)
    , m_numSliceGroups(0)
    , m_codedbufISize(0)
    , m_codedbufPBSize(0)
    , m_pMiscParamsFps(0)
    , m_pMiscParamsBrc(0)
    , m_pMiscParamsQuality(0)
    , m_pMiscParamsSeqInfo(0)
    , m_pMiscParamsSkipFrame(0)
    , m_miscParamFpsId(VA_INVALID_ID)
    , m_miscParamBrcId(VA_INVALID_ID)
    , m_miscParamQualityId(VA_INVALID_ID)
    , m_miscParamSeqInfoId(VA_INVALID_ID)
    , m_miscParamSkipFrameId(VA_INVALID_ID)
    , m_packedUserDataParamsId(VA_INVALID_ID)
    , m_packedUserDataId(VA_INVALID_ID)
    , m_mbqpBufferId(VA_INVALID_ID)
    , m_miscQualityParamId(VA_INVALID_ID)
    , m_priorityBufferId(VA_INVALID_ID)
    , m_priorityBuffer()
    , m_MaxContextPriority(0)
    , m_initFrameWidth(0)
    , m_initFrameHeight(0)
    , m_layout()
    , m_caps()
{
    std::fill_n(m_sliceParamBufferId, sizeof(m_sliceParamBufferId)/sizeof(m_sliceParamBufferId[0]), VA_INVALID_ID);

    Zero(m_allocResponseMB);
    Zero(m_allocResponseBS);
}

VAAPIEncoder::~VAAPIEncoder()
{
    Close();
}

inline bool CheckAttribValue(mfxU32 value)
{
    return (value != VA_ATTRIB_NOT_SUPPORTED) && (value != 0);
}

mfxStatus VAAPIEncoder::CreateAuxilliaryDevice(mfxU16 codecProfile)
{
    MFX_CHECK_NULL_PTR1(m_core);

    VAAPIVideoCORE * hwcore = dynamic_cast<VAAPIVideoCORE *>(m_core);
    MFX_CHECK_WITH_ASSERT(hwcore != 0, MFX_ERR_DEVICE_FAILED);

    if (hwcore)
    {
        mfxStatus mfxSts = hwcore->GetVAService(&m_vaDisplay);
        MFX_CHECK_STS(mfxSts);
    }

    memset(&m_caps, 0, sizeof(m_caps));

    m_caps.EncodeFunc        = 1;
    m_caps.BRCReset          = 1; // No bitrate resolution control
    m_caps.VCMBitrateControl = 0; // Video conference mode
    m_caps.HeaderInsertion   = 0; // We will provide headers (SPS, PPS) in binary format to the driver
    m_caps.MbQpDataSupport   = 1;
    m_caps.SkipFrame         = 1;

    m_caps.SliceIPBOnly      = 1;
    m_caps.MaxNum_Reference  = 1;
    m_caps.MaxPicWidth       = 1920;
    m_caps.MaxPicHeight      = 1088;
    m_caps.NoInterlacedField = 0; // Enable interlaced encoding
    m_caps.SliceStructure    = 1; // 1 - SliceDividerSnb; 2 - SliceDividerHsw; 3 - SliceDividerBluRay; the other - SliceDividerOneSlice

    std::map<VAConfigAttribType, size_t> idx_map;
    VAConfigAttribType attr_types[] = {
        VAConfigAttribMaxPictureHeight,
        VAConfigAttribMaxPictureWidth,
        VAConfigAttribEncSkipFrame,
        VAConfigAttribEncMaxRefFrames,
        VAConfigAttribEncSliceStructure
    };

    std::vector<VAConfigAttrib> attrs;
    attrs.reserve(sizeof(attr_types) / sizeof(attr_types[0]));

    for (size_t i=0; i < sizeof(attr_types)/sizeof(attr_types[0]); ++i)
    {
        attrs.emplace_back(VAConfigAttrib{attr_types[i], 0});
        idx_map[ attr_types[i] ] = i;
    }

    VAEntrypoint entrypoint = VAEntrypointEncSlice;

    VAStatus vaSts = vaGetConfigAttributes(m_vaDisplay,
                          ConvertProfileTypeMFX2VAAPI(codecProfile),
                          entrypoint,
                          attrs.data(), attrs.size());

    MFX_CHECK(!(VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT == vaSts ||
                VA_STATUS_ERROR_UNSUPPORTED_PROFILE    == vaSts),
                MFX_ERR_UNSUPPORTED);

    if (CheckAttribValue(attrs[idx_map[VAConfigAttribMaxPictureWidth]].value))
        m_caps.MaxPicWidth  = attrs[idx_map[VAConfigAttribMaxPictureWidth]].value;

    if (CheckAttribValue(attrs[idx_map[VAConfigAttribMaxPictureHeight]].value))
        m_caps.MaxPicHeight = attrs[idx_map[VAConfigAttribMaxPictureHeight]].value;

    if (CheckAttribValue(attrs[idx_map[VAConfigAttribEncSkipFrame]].value))
        m_caps.SkipFrame  = attrs[idx_map[VAConfigAttribEncSkipFrame]].value;

    if (CheckAttribValue(attrs[idx_map[VAConfigAttribEncMaxRefFrames]].value))
        m_caps.MaxNum_Reference  = attrs[idx_map[VAConfigAttribEncMaxRefFrames]].value;

    if (CheckAttribValue(attrs[idx_map[VAConfigAttribEncSliceStructure]].value))
        m_caps.SliceStructure  = attrs[idx_map[VAConfigAttribEncSliceStructure]].value;

    return MFX_ERR_NONE;
}


void VAAPIEncoder::QueryEncodeCaps(ENCODE_CAPS & caps)
{
    caps = m_caps;
}

mfxStatus VAAPIEncoder::Init(ExecuteBuffers* pExecuteBuffers, mfxU32 numRefFrames, mfxU32 funcId)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VAAPIEncoder::Init");
    mfxStatus sts = MFX_ERR_UNSUPPORTED;
    assert(ENCODE_ENC_PAK_ID == funcId);

    Zero(m_vaSpsBuf);
    Zero(m_vaPpsBuf);
    Zero(m_sliceParam, MAX_SLICES);
    std::fill(m_sliceParamBufferId, m_sliceParamBufferId + MAX_SLICES, VA_INVALID_ID);
    Zero(m_allocResponseMB);
    Zero(m_allocResponseBS);

    m_pMiscParamsFps = (VAEncMiscParameterBuffer*)new mfxU8[sizeof(VAEncMiscParameterFrameRate) + sizeof(VAEncMiscParameterBuffer)];
    Zero((VAEncMiscParameterFrameRate &)m_pMiscParamsFps->data);
    m_pMiscParamsFps->type = VAEncMiscParameterTypeFrameRate;

    m_pMiscParamsBrc = (VAEncMiscParameterBuffer*)new mfxU8[sizeof(VAEncMiscParameterRateControl) + sizeof(VAEncMiscParameterBuffer)];
    Zero((VAEncMiscParameterRateControl &)m_pMiscParamsBrc->data);
    m_pMiscParamsBrc->type = VAEncMiscParameterTypeRateControl;

    m_pMiscParamsQuality = (VAEncMiscParameterBuffer*)new mfxU8[sizeof(VAEncMiscParameterEncQuality) + sizeof(VAEncMiscParameterBuffer)];
    Zero((VAEncMiscParameterEncQuality &)m_pMiscParamsQuality->data);
    m_pMiscParamsQuality->type = (VAEncMiscParameterType)VAEncMiscParameterTypeEncQuality;

    m_pMiscParamsSeqInfo = (VAEncMiscParameterBuffer*)new mfxU8[sizeof(VAEncMiscParameterExtensionDataSeqDisplayMPEG2) + sizeof(VAEncMiscParameterBuffer)];
    Zero((char*)&m_pMiscParamsSeqInfo->data, sizeof(VAEncMiscParameterExtensionDataSeqDisplayMPEG2));
    m_pMiscParamsSeqInfo->type = (VAEncMiscParameterType)VAEncMiscParameterTypeExtensionData;

    m_pMiscParamsSkipFrame = (VAEncMiscParameterBuffer*)new mfxU8[sizeof(VAEncMiscParameterSkipFrame) + sizeof(VAEncMiscParameterBuffer)];
    Zero((VAEncMiscParameterSkipFrame &)m_pMiscParamsSkipFrame->data);
    m_pMiscParamsSkipFrame->type = (VAEncMiscParameterType)VAEncMiscParameterTypeSkipFrame;

    sts = Init(ENCODE_ENC_PAK, pExecuteBuffers);
    MFX_CHECK_STS(sts);

    sts = GetBuffersInfo();
    MFX_CHECK_STS(sts);

    return sts;
} // mfxStatus VAAPIEncoder::Init(ExecuteBuffers* pExecuteBuffers, mfxU32 numRefFrames, mfxU32 funcId)

mfxStatus VAAPIEncoder::Init(ENCODE_FUNC func, ExecuteBuffers* pExecuteBuffers)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VAAPIEncoder::Init");

    m_initFrameWidth  = mfx::align2_value(pExecuteBuffers->m_sps.FrameWidth, 16);
    m_initFrameHeight = mfx::align2_value(pExecuteBuffers->m_sps.FrameHeight,
                            pExecuteBuffers->m_sps.progressive_sequence ? 16 : 32);

    //memset (&m_rawFrames, 0, sizeof(mfxRawFrames));

    ExtVASurface cleanSurf = {VA_INVALID_ID, 0, 0};
    std::fill(m_feedback.begin(), m_feedback.end(), cleanSurf);

    VAAPIVideoCORE * hwcore = dynamic_cast<VAAPIVideoCORE *>(m_core);
    if(hwcore)
    {
        mfxStatus mfxSts = hwcore->GetVAService(&m_vaDisplay);
        MFX_CHECK_STS(mfxSts);
    }

    VAStatus vaSts;
    VAProfile mpegProfile = ConvertProfileTypeMFX2VAAPI(pExecuteBuffers->m_sps.Profile);
    // should be moved to core->IsGuidSupported()
    {
        VAEntrypoint* pEntrypoints = NULL;
        mfxI32 entrypointsCount = 0, entrypointsIndx = 0;
        mfxI32 maxNumEntrypoints   = vaMaxNumEntrypoints(m_vaDisplay);

        if(maxNumEntrypoints)
            pEntrypoints = new VAEntrypoint[maxNumEntrypoints];
        else
            return MFX_ERR_DEVICE_FAILED;

        vaSts = vaQueryConfigEntrypoints(
            m_vaDisplay,
            mpegProfile,
            pEntrypoints,
            &entrypointsCount);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        bool bEncodeEnable = false;
        for( entrypointsIndx = 0; entrypointsIndx < entrypointsCount; entrypointsIndx++ )
        {
            if( VAEntrypointEncSlice == pEntrypoints[entrypointsIndx] )
            {
                bEncodeEnable = true;
                break;
            }
        }
        delete[] pEntrypoints;
        if( !bEncodeEnable )
        {
            return MFX_ERR_DEVICE_FAILED;// unsupport?
        }
    }
    // IsGuidSupported()

    // Configuration
    std::vector<VAConfigAttrib> attrib_priority(1);
    attrib_priority[0].type = VAConfigAttribContextPriority;
    vaGetConfigAttributes(m_vaDisplay,
        ConvertProfileTypeMFX2VAAPI(pExecuteBuffers->m_sps.Profile),
        VAEntrypointEncSlice,
        attrib_priority.data(), attrib_priority.size());
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    if (attrib_priority[0].value != VA_ATTRIB_NOT_SUPPORTED)
        m_MaxContextPriority = attrib_priority[0].value;

    VAConfigAttrib attrib[3];

    attrib[0].type = VAConfigAttribRTFormat;
    attrib[1].type = VAConfigAttribRateControl;
    //attrib[2].type = VAConfigAttribEncSkipFrame;

    vaGetConfigAttributes(m_vaDisplay,
        ConvertProfileTypeMFX2VAAPI(pExecuteBuffers->m_sps.Profile),
        VAEntrypointEncSlice,
        &attrib[0], 2);

    if ((attrib[0].value & VA_RT_FORMAT_YUV420) == 0)
        return MFX_ERR_DEVICE_FAILED;

    //m_caps.SkipFrame = (attrib[2].value & (~VA_ATTRIB_NOT_SUPPORTED)) ? 1 : 0 ;

    uint32_t vaRCType = ConvertRateControlMFX2VAAPI(pExecuteBuffers->m_sps.RateControlMethod);

    if ((attrib[1].value & vaRCType) == 0)
        return MFX_ERR_DEVICE_FAILED;

    attrib[0].value = VA_RT_FORMAT_YUV420;
    attrib[1].value = vaRCType;

    vaSts = vaCreateConfig(
        m_vaDisplay,
        mpegProfile,
        VAEntrypointEncSlice,
        attrib,
        2,
        &m_vaConfig);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    return MFX_ERR_NONE;

} // mfxStatus VAAPIEncoder::Init(ENCODE_FUNC func,ExecuteBuffers* pExecuteBuffers)


mfxStatus VAAPIEncoder::CreateContext(ExecuteBuffers* pExecuteBuffers, mfxU32 numRefFrames, mfxU32 funcId)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VAAPIEncoder::CreateContex");
    assert (m_vaContextEncode == VA_INVALID_ID);

    mfxStatus sts;
    VAStatus vaSts;
    std::vector<VASurfaceID> reconSurf;

    //for(size_t i = 0; i < 4; i++)
    //    reconSurf.push_back((VASurfaceID)i);


    for(size_t i = 0; i < m_recFrames.size(); i++)
        reconSurf.push_back(m_recFrames[i].surface);

    // Encoder create
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaCreateContext");
        vaSts = vaCreateContext(
            m_vaDisplay,
            m_vaConfig,
            m_initFrameWidth,
            m_initFrameHeight,
            VA_PROGRESSIVE,
            &*reconSurf.begin(),
            reconSurf.size(),
            &m_vaContextEncode);
    }
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    sts = CreateBSBuffer(numRefFrames, pExecuteBuffers);
    MFX_CHECK_STS(sts);

    if (pExecuteBuffers->m_mbqp_data)
    {
        m_mbqpDataBuffer.resize(mfx::align2_value(m_initFrameWidth / 16, 64) * mfx::align2_value(m_initFrameHeight / 16, 8));

//             if (IsOn(extOpt3->MBDisableSkipMap))
//                 m_mb_noskip_buffer.resize(mfx::align2_value(m_width / 16, 64) * mfx::align2_value(m_height / 16, 8)
    }

    return MFX_ERR_NONE;
} // mfxStatus VAAPIEncoder::CreateContext(ExecuteBuffers* pExecuteBuffers, mfxU32 numRefFrames, mfxU32 funcId)

mfxStatus VAAPIEncoder::GetBuffersInfo()
{
    return MFX_ERR_NONE;
} // mfxStatus VAAPIEncoder::GetBuffersInfo()


mfxStatus VAAPIEncoder::QueryMbDataLayout()
{
    // HRESULT hr = 0;

    memset(&m_layout, 0, sizeof(m_layout));

    /*
    hr = m_pDevice->Execute(MBDATA_LAYOUT_ID, 0, 0,&m_layout,sizeof(m_layout));
    MFX_CHECK(SUCCEEDED(hr), MFX_ERR_DEVICE_FAILED);
    */
    return MFX_ERR_NONE;

} // mfxStatus VAAPIEncoder::QueryMbDataLayout()


mfxStatus VAAPIEncoder::QueryCompBufferInfo(D3DDDIFORMAT type, mfxFrameAllocRequest* pRequest, ExecuteBuffers* pExecuteBuffers)
{
    if (type == D3DDDIFMT_INTELENCODE_MBDATA)
    {
        pRequest->Info.Width  = std::max(pRequest->Info.Width,  pExecuteBuffers->m_sps.FrameWidth);
        pRequest->Info.Height = std::max(pRequest->Info.Height, pExecuteBuffers->m_sps.FrameHeight);
    }
    else if (type == D3DDDIFMT_INTELENCODE_BITSTREAMDATA)
    {
        pRequest->Info.Width  = std::max(pRequest->Info.Width,  pExecuteBuffers->m_sps.FrameWidth);
        pRequest->Info.Height = std::max(pRequest->Info.Height, pExecuteBuffers->m_sps.FrameHeight);
    }
    else
    {
        assert(!"unknown buffer type");
    }

    if(MFX_RATECONTROL_CQP == pExecuteBuffers->m_sps.RateControlMethod)
    {
        m_codedbufISize = pExecuteBuffers->m_sps.FrameWidth*pExecuteBuffers->m_sps.FrameHeight*4;
        m_codedbufPBSize =  pExecuteBuffers->m_sps.FrameWidth*pExecuteBuffers->m_sps.FrameHeight*4;
    }
    else
    {
        const mfxU32 rawFrameSize = pExecuteBuffers->m_sps.FrameWidth*pExecuteBuffers->m_sps.FrameHeight*3/2;
        const mfxU32 vbvSize = pExecuteBuffers->m_sps.vbv_buffer_size * 1000;
        if(vbvSize > rawFrameSize)
            m_codedbufISize = m_codedbufPBSize = vbvSize;
        else
            m_codedbufISize = m_codedbufPBSize = rawFrameSize;
    }

    pRequest->Info.Width = m_codedbufISize / pRequest->Info.Height / 3 * 2;

    // request linear buffer
    pRequest->Info.FourCC = MFX_FOURCC_P8;

    // context_id required for allocation video memory (tmp solution)
    pRequest->AllocId = m_vaContextEncode;

    return MFX_ERR_NONE;

} // mfxStatus VAAPIEncoder::QueryCompBufferInfo(D3DDDIFORMAT type, mfxFrameAllocRequest* pRequest, ExecuteBuffers * pExecuteBuffers)


mfxStatus VAAPIEncoder::Register(
    const mfxFrameAllocResponse* pResponse,
    D3DDDIFORMAT type)
{
    std::vector<ExtVASurface> * pQueue = (D3DDDIFMT_INTELENCODE_BITSTREAMDATA == type)
        ? &m_bsQueue
        : &m_reconQueue;

    mfxStatus sts = MFX_ERR_NONE;

    MFX_CHECK(pResponse->mids, MFX_ERR_NULL_PTR);

    for (int i = 0; i < pResponse->NumFrameActual; i++)
    {
        ExtVASurface extSurf = {};
        VASurfaceID *pSurface = 0;

        sts = m_core->GetFrameHDL(pResponse->mids[i], (mfxHDL *)&pSurface);
        MFX_CHECK_STS(sts);

        extSurf.number  = i;
        extSurf.surface = *pSurface;

        pQueue->push_back( extSurf );
    }

    //hr = m_pDevice->BeginFrame(SurfaceReg.surface[0], &SurfaceReg);
    //MFX_CHECK(SUCCEEDED(hr), MFX_ERR_DEVICE_FAILED);

    //hr = m_pDevice->EndFrame(0);
    //MFX_CHECK(SUCCEEDED(hr), MFX_ERR_DEVICE_FAILED);

    return MFX_ERR_NONE;
} // mfxStatus VAAPIEncoder::Register(const mfxFrameAllocResponse* pResponse, D3DDDIFORMAT type)

mfxStatus VAAPIEncoder::RegisterRefFrames (const mfxFrameAllocResponse* pResponse)
{
    MFX_CHECK(pResponse->mids, MFX_ERR_NULL_PTR);
    MFX_CHECK(pResponse->NumFrameActual < NUM_FRAMES, MFX_ERR_UNSUPPORTED);

    m_recFrames.resize(pResponse->NumFrameActual);

    mfxStatus sts;
    VASurfaceID *pSurface = NULL;


    // we should register allocated HW bitstreams and recon surfaces
    for (int i = 0; i < pResponse->NumFrameActual; i++)
    {
        sts = m_core->GetFrameHDL(pResponse->mids[i], (mfxHDL *)&pSurface);
        MFX_CHECK_STS(sts);

        m_recFrames[i].surface = *pSurface;
        m_recFrames[i].number = i;
    }

    //return MFX_ERR_NONE;

    sts = Register(pResponse, D3DDDIFMT_NV12);
    MFX_CHECK_STS(sts);

    return MFX_ERR_NONE;
} // mfxStatus VAAPIEncoder::RegisterRefFrames (const mfxFrameAllocResponse* pResponse)


mfxI32 VAAPIEncoder::GetRecFrameIndex (mfxMemId memID)
{
    mfxStatus sts;
    VASurfaceID *pSurface = NULL;
    sts = m_core->GetFrameHDL(memID, (mfxHDL *)&pSurface);
    MFX_CHECK_WITH_ASSERT(MFX_ERR_NONE == sts, -1);

    for (size_t i = 0; i < m_recFrames.size(); i++)
    {
        if (m_recFrames[i].surface == *pSurface)
            return static_cast<mfxI32>(i);
    }

    return -1;
} // mfxI32 VAAPIEncoder::GetRecFrameIndex (mfxMemId memID)


mfxI32 VAAPIEncoder::GetRawFrameIndex (mfxMemId memID, bool bAddFrames)
{
    assert(0);
    mfxStatus sts;
    VASurfaceID *pSurface = NULL;
    sts = m_core->GetFrameHDL(memID, (mfxHDL *)&pSurface);
    MFX_CHECK_WITH_ASSERT(MFX_ERR_NONE == sts, -1);

    for (size_t i = 0; i < m_recFrames.size(); i++)
    {
        if (m_rawFrames[i].surface == *pSurface)
            return static_cast<mfxI32>(i);
    }

    if (bAddFrames && m_rawFrames.size() < NUM_FRAMES)
    {
        ExtVASurface surf = {*pSurface, 0, 0};
        m_rawFrames.push_back(surf);
        return m_rawFrames.size() - 1;
    }

    return -1;
} // mfxI32 VAAPIEncoder::GetRawFrameIndex (mfxMemId memID, bool bAddFrames)


mfxStatus VAAPIEncoder::CreateCompBuffers(ExecuteBuffers* pExecuteBuffers, mfxU32 numRefFrames)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameAllocRequest request = {};

    // Buffer for MB INFO
    sts = QueryCompBufferInfo(D3DDDIFMT_INTELENCODE_MBDATA, &request, pExecuteBuffers);
    MFX_CHECK_STS(sts);

    request.NumFrameMin = (request.NumFrameMin < numRefFrames) ? (mfxU16)numRefFrames : request.NumFrameMin;
    request.NumFrameSuggested = (request.NumFrameSuggested < request.NumFrameMin) ? request.NumFrameMin : request.NumFrameSuggested;

    if (m_allocResponseMB.NumFrameActual == 0)
    {
        request.Info.FourCC = MFX_FOURCC_P8;//D3DFMT_P8;
        request.Type = MFX_MEMTYPE_FROM_ENCODE | MFX_MEMTYPE_DXVA2_DECODER_TARGET|MFX_MEMTYPE_INTERNAL_FRAME;
        sts = m_core->AllocFrames(&request, &m_allocResponseMB);
        MFX_CHECK_STS(sts);
    }
    else
    {
        if (m_allocResponseMB.NumFrameActual < request.NumFrameMin)
        {
            return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;
        }
    }
    sts = Register(&m_allocResponseMB, D3DDDIFMT_INTELENCODE_MBDATA);
    MFX_CHECK_STS(sts);

    return MFX_ERR_NONE;
} // mfxStatus VAAPIEncoder::CreateCompBuffers(ExecuteBuffers* pExecuteBuffers, mfxU32 numRefFrames)


mfxStatus VAAPIEncoder::CreateBSBuffer(mfxU32 numRefFrames, ExecuteBuffers* pExecuteBuffers)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VAAPIEncoder::CreateBSBuffer");
    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameAllocRequest request = {};

    // Buffer for MB INFO
    sts = QueryCompBufferInfo(D3DDDIFMT_INTELENCODE_BITSTREAMDATA, &request, pExecuteBuffers);
    MFX_CHECK_STS(sts);

    request.NumFrameMin = (request.NumFrameMin < numRefFrames) ? (mfxU16)numRefFrames : request.NumFrameMin;
    request.NumFrameSuggested = (request.NumFrameSuggested < request.NumFrameMin) ? request.NumFrameMin : request.NumFrameSuggested;

    if (m_allocResponseMB.NumFrameActual == 0)
    {
        request.Info.FourCC = MFX_FOURCC_P8;//D3DFMT_P8;
        request.Type = MFX_MEMTYPE_FROM_ENCODE | MFX_MEMTYPE_DXVA2_DECODER_TARGET|MFX_MEMTYPE_INTERNAL_FRAME;
        sts = m_core->AllocFrames(&request, &m_allocResponseBS);
        MFX_CHECK_STS(sts);
    }
    else
    {
        if (m_allocResponseBS.NumFrameActual < request.NumFrameMin)
        {
            return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;
        }
    }
    sts = Register(&m_allocResponseBS, D3DDDIFMT_INTELENCODE_BITSTREAMDATA);
    MFX_CHECK_STS(sts);

    return MFX_ERR_NONE;
} // mfxStatus VAAPIEncoder::CreateBSBuffer(mfxU32 numRefFrames)


mfxStatus VAAPIEncoder::FillSlices(ExecuteBuffers* pExecuteBuffers)
{
    VAEncSliceParameterBufferMPEG2 *sliceParam;
    VAStatus vaSts;
    int i, width_in_mbs, height_in_mbs;

//    assert(m_vaPpsBuf.picture_coding_extension.bits.q_scale_type == 0);

    width_in_mbs = (m_vaSpsBuf.picture_width + 15) >> 4;
    if (m_vaSpsBuf.sequence_extension.bits.progressive_sequence)
    {
        height_in_mbs = (m_vaSpsBuf.picture_height + 15) >> 4;
    }
    else
    {
        height_in_mbs = ((m_vaSpsBuf.picture_height + 31) >> 5) * 2;
    }
    m_numSliceGroups = 1;

    MFX_CHECK_WITH_ASSERT(height_in_mbs == pExecuteBuffers->m_pps.NumSlice, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);

    mfxStatus sts;
    for (i = 0; i < height_in_mbs; i++) {
        ENCODE_SET_SLICE_HEADER_MPEG2&  ddiSlice = pExecuteBuffers->m_pSlice[i];
        assert(ddiSlice.NumMbsForSlice == width_in_mbs);
        sliceParam = &m_sliceParam[i];
        sliceParam->macroblock_address = i * width_in_mbs;
        sliceParam->num_macroblocks    = ddiSlice.NumMbsForSlice;
        sliceParam->is_intra_slice     = ddiSlice.IntraSlice;
        // prevent GPU hang due to different scale_code in different slices
        sliceParam->quantiser_scale_code = pExecuteBuffers->m_pSlice[0].quantiser_scale_code;

        sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_sliceParamBufferId[i]);
        MFX_CHECK_STS(sts);
    }

    vaSts = vaCreateBuffer(m_vaDisplay,
        m_vaContextEncode,
        VAEncSliceParameterBufferType,
        sizeof(*sliceParam),
        height_in_mbs,
        m_sliceParam,
        m_sliceParamBufferId);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    return MFX_ERR_NONE;
} // mfxStatus VAAPIEncoder::FillSlices(ExecuteBuffers* pExecuteBuffers)

mfxStatus VAAPIEncoder::FillMiscParameterBuffer(ExecuteBuffers* pExecuteBuffers)
{
    VAEncMiscParameterFrameRate   & miscFps     = (VAEncMiscParameterFrameRate   &)m_pMiscParamsFps->data;
    VAEncMiscParameterRateControl & miscBrc     = (VAEncMiscParameterRateControl &)m_pMiscParamsBrc->data;
    VAEncMiscParameterEncQuality  & miscQuality = (VAEncMiscParameterEncQuality  &)m_pMiscParamsQuality->data;

    VAStatus                      vaSts;

    PackMfxFrameRate(pExecuteBuffers->m_FrameRateExtN, pExecuteBuffers->m_FrameRateExtD, miscFps.framerate);

    miscBrc.bits_per_second = pExecuteBuffers->m_sps.MaxBitRate * 1000;
    miscBrc.target_percentage = pExecuteBuffers->m_sps.MaxBitRate ? uint32_t(pExecuteBuffers->m_sps.bit_rate * 100. / pExecuteBuffers->m_sps.MaxBitRate) : 0;

    miscQuality.PanicModeDisable = pExecuteBuffers->m_bDisablePanicMode ? 1 : 0;

    mfxStatus sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_miscParamFpsId);
    MFX_CHECK_STS(sts);

    vaSts = vaCreateBuffer(m_vaDisplay,
        m_vaContextEncode,
        VAEncMiscParameterBufferType,
        sizeof(VAEncMiscParameterFrameRate) + sizeof(VAEncMiscParameterBuffer),
        1,
        m_pMiscParamsFps,
        &m_miscParamFpsId);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_miscParamBrcId);
    MFX_CHECK_STS(sts);

    if (pExecuteBuffers->m_sps.RateControlMethod == MFX_RATECONTROL_VBR)
    {
        vaSts = vaCreateBuffer(m_vaDisplay,
            m_vaContextEncode,
            VAEncMiscParameterBufferType,
            sizeof(VAEncMiscParameterRateControl) + sizeof(VAEncMiscParameterBuffer),
            1,
            m_pMiscParamsBrc,
            &m_miscParamBrcId);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }

    sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_miscParamQualityId);
    MFX_CHECK_STS(sts);

    vaSts = vaCreateBuffer(m_vaDisplay,
        m_vaContextEncode,
        VAEncMiscParameterBufferType,
        sizeof(VAEncMiscParameterEncQuality) + sizeof(VAEncMiscParameterBuffer),
        1,
        m_pMiscParamsQuality,
        &m_miscParamQualityId);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    return MFX_ERR_NONE;
} // mfxStatus VAAPIEncoder::FillMiscParameterBuffer(ExecuteBuffers* pExecuteBuffers)

mfxStatus VAAPIEncoder::FillQualityLevelBuffer(ExecuteBuffers* pExecuteBuffers)
{
    VAStatus vaSts;
    VAEncMiscParameterBuffer *misc_param;
    VAEncMiscParameterBufferQualityLevel *quality_param;

    mfxStatus sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_miscQualityParamId);
    MFX_CHECK_STS(sts);

    vaSts = vaCreateBuffer(m_vaDisplay,
                   m_vaContextEncode,
                   VAEncMiscParameterBufferType,
                   sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterBufferQualityLevel),
                   1,
                   NULL,
                   &m_miscQualityParamId);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    vaSts = vaMapBuffer(m_vaDisplay,
                 m_miscQualityParamId,
                (void **)&misc_param);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    misc_param->type = (VAEncMiscParameterType)VAEncMiscParameterTypeQualityLevel;
    quality_param = (VAEncMiscParameterBufferQualityLevel *)misc_param->data;

    unsigned int tu = pExecuteBuffers->m_sps.TargetUsage;
    if (tu == 0)
        tu = 3;

    quality_param->quality_level = tu;

    vaSts = vaUnmapBuffer(m_vaDisplay, m_miscQualityParamId);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    return MFX_ERR_NONE;
} // mfxStatus VAAPIEncoder::FillQualityLevelBuffer(ExecuteBuffers* pExecuteBuffers)


mfxStatus VAAPIEncoder::FillUserDataBuffer(mfxU8 *pUserData, mfxU32 userDataLen)
{
    VAStatus vaSts;
    VAEncPackedHeaderParameterBuffer packedParamsBuffer = {};
    packedParamsBuffer.type = VAEncPackedHeaderRawData;
    packedParamsBuffer.has_emulation_bytes = false;
    mfxU32 correctedLength = std::min(userDataLen, UINT_MAX/8); // keep start code
    packedParamsBuffer.bit_length = correctedLength * 8;

    mfxStatus sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_packedUserDataParamsId);
    MFX_CHECK_STS(sts);

    vaSts = vaCreateBuffer(m_vaDisplay,
        m_vaContextEncode,
        VAEncPackedHeaderParameterBufferType,
        sizeof(packedParamsBuffer),
        1,
        &packedParamsBuffer,
        &m_packedUserDataParamsId);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_packedUserDataId);
    MFX_CHECK_STS(sts);

    vaSts = vaCreateBuffer(m_vaDisplay,
        m_vaContextEncode,
        VAEncPackedHeaderDataBufferType,
        correctedLength, 1, pUserData,
        &m_packedUserDataId);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    return MFX_ERR_NONE;
} // mfxStatus VAAPIEncoder::FillUserDataBuffer(mfxU8 *pUserData, mfxU32 userDataLen)

mfxStatus VAAPIEncoder::FillVideoSignalInfoBuffer(ExecuteBuffers* pExecuteBuffers)
{
    // two steps cast is to work around “strict aliasing” rule warrning
    void * data = m_pMiscParamsSeqInfo->data;
    VAEncMiscParameterExtensionDataSeqDisplayMPEG2& miscSeqInfo = *((VAEncMiscParameterExtensionDataSeqDisplayMPEG2*)data);

    const mfxExtVideoSignalInfo & signalInfo = pExecuteBuffers->m_VideoSignalInfo;
    // VideoFullRange; - unused
    miscSeqInfo.extension_start_code_identifier = 0x02; // from spec
    miscSeqInfo.video_format = signalInfo.VideoFormat;
    miscSeqInfo.colour_description = signalInfo.ColourDescriptionPresent;
    if (signalInfo.ColourDescriptionPresent)
    {
        miscSeqInfo.colour_primaries = signalInfo.ColourPrimaries;
        miscSeqInfo.transfer_characteristics = signalInfo.TransferCharacteristics;
        miscSeqInfo.matrix_coefficients = signalInfo.MatrixCoefficients;
    }
    else
    {
        miscSeqInfo.colour_primaries = 0;
        miscSeqInfo.transfer_characteristics = 0;
        miscSeqInfo.matrix_coefficients = 0;
    }
    miscSeqInfo.display_horizontal_size = pExecuteBuffers->m_sps.FrameWidth;
    miscSeqInfo.display_vertical_size = pExecuteBuffers->m_sps.FrameHeight;

    mfxStatus sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_miscParamSeqInfoId);
    MFX_CHECK_STS(sts);

    VAStatus vaSts = vaCreateBuffer(m_vaDisplay,
        m_vaContextEncode,
        VAEncMiscParameterBufferType,
        sizeof(VAEncMiscParameterExtensionDataSeqDisplayMPEG2) + sizeof(VAEncMiscParameterBuffer),
        1,
        m_pMiscParamsSeqInfo,
        &m_miscParamSeqInfoId);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    return MFX_ERR_NONE;

} // mfxStatus VAAPIEncoder::FillVideoSignalInfoBuffer(ExecuteBuffers* pExecuteBuffers)

mfxStatus VAAPIEncoder::FillMBQPBuffer(
    ExecuteBuffers* pExecuteBuffers,
    mfxU8*          mbqp,
    mfxU32          numMB)
{
    VAStatus vaSts;

    int width_in_mbs, height_in_mbs;

    //    assert(m_vaPpsBuf.picture_coding_extension.bits.q_scale_type == 0);

    width_in_mbs = (m_vaSpsBuf.picture_width + 15) >> 4;
    if (m_vaSpsBuf.sequence_extension.bits.progressive_sequence)
    {
        height_in_mbs = (m_vaSpsBuf.picture_height + 15) >> 4;
    }
    else
    {
        height_in_mbs = ((m_vaSpsBuf.picture_height + 31) >> 5) * 2;
    }

    //width(64byte alignment) height(8byte alignment)
    mfxU32 bufW = mfx::align2_value(width_in_mbs * 4, 64);
    mfxU32 bufH = mfx::align2_value(height_in_mbs,     8);

    if (   mbqp && numMB >= static_cast<mfxU32>(width_in_mbs * height_in_mbs)
        && m_mbqpDataBuffer.size()*4 >= (bufW* bufH))
    {

        Zero(m_mbqpDataBuffer);
        for (mfxU32 mbRow = 0; mbRow < static_cast<mfxU32>(height_in_mbs); mbRow ++)
            for (mfxU32 mbCol = 0; mbCol < static_cast<mfxU32>(width_in_mbs); mbCol ++)
                m_mbqpDataBuffer[mbRow * (bufW/4) + mbCol].qp_y = mbqp[mbRow * width_in_mbs + mbCol];
    }

    mfxStatus sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_mbqpBufferId);
    MFX_CHECK_STS(sts);

    vaSts = vaCreateBuffer(m_vaDisplay,
        m_vaContextEncode,
        VAEncQPBufferType,
        bufW,
        bufH,//m_mbqpDataBuffer.size(),
        &m_mbqpDataBuffer[0],
        &m_mbqpBufferId);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

/*    static int counter = 0;
    printf("Frame: %d %d macroblocks\nQP:", counter++, numMB);
    for (int i=0; i < numMB; i++){
        if ((i % 22) == 0)
            printf("\n");

        printf("%d  ", mbqp[i]);
    }
    printf("\n");*/

    return MFX_ERR_NONE;
} // mfxStatus VAAPIEncoder::FillMBQPBuffer(ExecuteBuffers* pExecuteBuffers ,mfxU8* mbqp, mfxU32 numMB)



mfxStatus VAAPIEncoder::FillSkipFrameBuffer(mfxU8 skipFlag)
{
    //printf("******* Sending SkipFrame buffer\n");
    VAStatus vaSts;
    VAEncMiscParameterSkipFrame & skipParam = (VAEncMiscParameterSkipFrame &)m_pMiscParamsSkipFrame->data;

    skipParam.skip_frame_flag = skipFlag ? 1 : 0;
    skipParam.num_skip_frames = 0; // unused in MPEG2
    skipParam.size_skip_frames = 0; // unused in MPEG2

    mfxStatus sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_miscParamSkipFrameId);
    MFX_CHECK_STS(sts);

    vaSts = vaCreateBuffer(m_vaDisplay,
        m_vaContextEncode,
        VAEncMiscParameterBufferType,
        sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterSkipFrame),
        1,
        m_pMiscParamsSkipFrame,
        &m_miscParamSkipFrameId);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    return MFX_ERR_NONE;
} // mfxStatus VAAPIEncoder::FillSkipFrameBuffer(mfxU8 skipFlag)

mfxStatus VAAPIEncoder::FillPriorityBuffer(mfxPriority& priority)
{
    VAStatus vaSts;
    memset(&m_priorityBuffer, 0, sizeof(VAContextParameterUpdateBuffer));
    m_priorityBuffer.flags.bits.context_priority_update = 1;

    if(priority == MFX_PRIORITY_LOW)
    {
        m_priorityBuffer.context_priority.bits.priority = 0;
    }
    else if (priority == MFX_PRIORITY_HIGH)
    {
        m_priorityBuffer.context_priority.bits.priority = m_MaxContextPriority;
    }
    else
    {
        m_priorityBuffer.context_priority.bits.priority = m_MaxContextPriority/2;
    }

    mfxStatus sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_priorityBufferId);
    MFX_CHECK_STS(sts);

    vaSts = vaCreateBuffer(m_vaDisplay,
        m_vaContextEncode,
        VAContextParameterUpdateBufferType,
        sizeof(m_priorityBuffer),
        1,
        &m_priorityBuffer,
        &m_priorityBufferId);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    return MFX_ERR_NONE;
}

mfxStatus VAAPIEncoder::Execute(ExecuteBuffers* pExecuteBuffers, mfxU32 funcId, mfxU8 *pUserData, mfxU32 userDataLen)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VAAPIEncoder::Execute");
    mfxStatus mfxSts;
    VAStatus vaSts;

    std::vector<VABufferID> configBuffers;
    configBuffers.reserve(16);

    if (pExecuteBuffers->m_bAddSPS)
    {
        // TODO: fill sps only once
        Zero(m_vaSpsBuf);
        FillSps(pExecuteBuffers, m_vaSpsBuf);

        mfxSts = CheckAndDestroyVAbuffer(m_vaDisplay, m_spsBufferId);
        MFX_CHECK_STS(mfxSts);

        vaSts = vaCreateBuffer(m_vaDisplay,
            m_vaContextEncode,
            VAEncSequenceParameterBufferType,
            sizeof(m_vaSpsBuf),
            1,
            &m_vaSpsBuf,
            &m_spsBufferId);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        if (m_spsBufferId != VA_INVALID_ID)
            configBuffers.push_back(m_spsBufferId);

        // mfxExtVideoSignalInfo present - insert only with SPS
        if (pExecuteBuffers->m_bAddDisplayExt)
        {
            mfxSts = FillVideoSignalInfoBuffer(pExecuteBuffers);
            MFX_CHECK(mfxSts == MFX_ERR_NONE, MFX_ERR_DEVICE_FAILED);

            if (m_miscParamSeqInfoId != VA_INVALID_ID)
                configBuffers.push_back(m_miscParamSeqInfoId);
        }

        pExecuteBuffers->m_bAddSPS = 0;

        if (funcId == ENCODE_ENC_PAK_ID &&
            (pExecuteBuffers->m_quantMatrix.load_intra_quantiser_matrix ||
             pExecuteBuffers->m_quantMatrix.load_non_intra_quantiser_matrix ||
             pExecuteBuffers->m_quantMatrix.load_chroma_intra_quantiser_matrix ||
             pExecuteBuffers->m_quantMatrix.load_chroma_non_intra_quantiser_matrix))
        {
            mfxSts = CheckAndDestroyVAbuffer(m_vaDisplay, m_qmBufferId);
            MFX_CHECK_STS(mfxSts);

            vaSts = vaCreateBuffer(m_vaDisplay,
                m_vaContextEncode,
                VAQMatrixBufferType,
                sizeof(pExecuteBuffers->m_quantMatrix),
                1,
                &pExecuteBuffers->m_quantMatrix,
                &m_qmBufferId);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

            if (m_qmBufferId != VA_INVALID_ID)
                configBuffers.push_back(m_qmBufferId);
        }
    }

    // TODO: fill pps only once
    Zero(m_vaPpsBuf);
    FillPps(m_core, pExecuteBuffers, m_vaPpsBuf, pUserData, userDataLen);

    mfxSts = FillMiscParameterBuffer(pExecuteBuffers);
    MFX_CHECK(mfxSts == MFX_ERR_NONE, MFX_ERR_DEVICE_FAILED);

    mfxSts = FillQualityLevelBuffer(pExecuteBuffers);
    MFX_CHECK(mfxSts == MFX_ERR_NONE, MFX_ERR_DEVICE_FAILED);

    if (pExecuteBuffers->m_SkipFrame && (pExecuteBuffers->m_pps.picture_coding_type == CODING_TYPE_B))
    {
        mfxSts = FillSkipFrameBuffer(pExecuteBuffers->m_SkipFrame);
        MFX_CHECK(mfxSts == MFX_ERR_NONE, MFX_ERR_DEVICE_FAILED);
    }
    else
    {
        mfxSts = CheckAndDestroyVAbuffer(m_vaDisplay, m_miscParamSkipFrameId);
        MFX_CHECK_STS(mfxSts);
    }

    mfxSts = FillSlices(pExecuteBuffers);

    MFX_CHECK(mfxSts == MFX_ERR_NONE, MFX_ERR_DEVICE_FAILED);

    MFX_CHECK_WITH_ASSERT(pExecuteBuffers->m_idxBs >= 0  && pExecuteBuffers->m_idxBs < m_bsQueue.size(), MFX_ERR_DEVICE_FAILED);
    m_vaPpsBuf.coded_buf = m_bsQueue[pExecuteBuffers->m_idxBs].surface;

    mfxSts = CheckAndDestroyVAbuffer(m_vaDisplay, m_ppsBufferId);
    MFX_CHECK_STS(mfxSts);

    vaSts = vaCreateBuffer(m_vaDisplay,
        m_vaContextEncode,
        VAEncPictureParameterBufferType,
        sizeof(m_vaPpsBuf),
        1,
        &m_vaPpsBuf,
        &m_ppsBufferId);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    if (m_ppsBufferId != VA_INVALID_ID)
        configBuffers.push_back(m_ppsBufferId);

    if (m_miscParamFpsId != VA_INVALID_ID)
        configBuffers.push_back(m_miscParamFpsId);

    if (m_miscParamBrcId != VA_INVALID_ID)
        configBuffers.push_back(m_miscParamBrcId);

    if (m_miscParamQualityId != VA_INVALID_ID)
        configBuffers.push_back(m_miscParamQualityId);

    if (m_miscQualityParamId != VA_INVALID_ID)
        configBuffers.push_back(m_miscQualityParamId);

    if (m_miscParamSkipFrameId != VA_INVALID_ID)
        configBuffers.push_back(m_miscParamSkipFrameId);

    if (pUserData && userDataLen > 0)
    {
        mfxSts = FillUserDataBuffer(pUserData, userDataLen);
        MFX_CHECK(mfxSts == MFX_ERR_NONE, MFX_ERR_DEVICE_FAILED);

        if (m_packedUserDataParamsId != VA_INVALID_ID)
            configBuffers.push_back(m_packedUserDataParamsId);

        if (m_packedUserDataId != VA_INVALID_ID)
            configBuffers.push_back(m_packedUserDataId);
    }

    bool isMBQP = pExecuteBuffers->m_mbqp_data && (pExecuteBuffers->m_mbqp_data[0] != 0);
    if (isMBQP)
    {
        mfxSts = FillMBQPBuffer(pExecuteBuffers, (mfxU8 *)pExecuteBuffers->m_mbqp_data, pExecuteBuffers->m_nMBs);
        MFX_CHECK(mfxSts == MFX_ERR_NONE, MFX_ERR_DEVICE_FAILED);

        if (m_mbqpBufferId != VA_INVALID_ID)
            configBuffers.push_back(m_mbqpBufferId);

        pExecuteBuffers->m_mbqp_data[0] = 0;
    }

    //configure the GPU priority parameters
    if(m_MaxContextPriority)
    {
        mfxPriority contextPriority = m_core->GetSession()->m_priority;
        mfxSts = FillPriorityBuffer(contextPriority);
        MFX_CHECK(mfxSts == MFX_ERR_NONE, MFX_ERR_DEVICE_FAILED);
        if (m_priorityBufferId != VA_INVALID_ID)
            configBuffers.push_back(m_priorityBufferId);
    }

    //------------------------------------------------------------------
    // Rendering
    //------------------------------------------------------------------
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "Rendering");
        MFX_LTRACE_I(MFX_TRACE_LEVEL_PARAMS, pExecuteBuffers->m_idxMb);
        MFX_LTRACE_2(MFX_TRACE_LEVEL_HOTSPOTS, "A|ENCODE|MPEG2|PACKET_START|", "%d|%d", m_vaContextEncode, 0);
        //TODO: external frame HDL??
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaBeginPicture");
            vaSts = vaBeginPicture(m_vaDisplay,
                m_vaContextEncode,
               *(VASurfaceID*)pExecuteBuffers->m_pSurface);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        }
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaRenderPicture(buf)");
            vaSts = vaRenderPicture(m_vaDisplay,
                m_vaContextEncode,
                configBuffers.data(),
                configBuffers.size());
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        }

        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaRenderPicture(slice)");
            vaSts = vaRenderPicture(m_vaDisplay,
                m_vaContextEncode,
                &m_sliceParamBufferId[0],
                m_numSliceGroups);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        }

        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaEndPicture");
            vaSts = vaEndPicture(m_vaDisplay, m_vaContextEncode);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        }
        MFX_LTRACE_2(MFX_TRACE_LEVEL_HOTSPOTS, "A|ENCODE|MPEG2|PACKET_END|", "%d|%d", m_vaContextEncode, 0);
        //vaSts = vaSyncSurface(m_vaDisplay, *(VASurfaceID*)pExecuteBuffers->m_pSurface);
        //MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    }

    //------------------------------------------------------------------
    // PostStage
    //------------------------------------------------------------------
    // put to cache
    {
        UMC::AutomaticUMCMutex guard(m_guard);
        ExtVASurface currentFeedback = {
            *(VASurfaceID*)pExecuteBuffers->m_pSurface,
            pExecuteBuffers->m_pps.StatusReportFeedbackNumber,
            pExecuteBuffers->m_idxBs
        };
        //currentFeedback.number  =  pExecuteBuffers->m_pps.StatusReportFeedbackNumber;//   task.m_statusReportNumber[fieldId];
        //currentFeedback.surface = *(VASurfaceID*)pExecuteBuffers->m_pSurface; //ConvertSurfaceIdMFX2VAAPI(m_core, pExecuteBuffers->m_pSurface);
        //currentFeedback.idxBs    = pExecuteBuffers->m_idxBs;                         // task.m_idxBs[fieldId];
        m_feedback.push_back( currentFeedback );
    }

    return MFX_ERR_NONE;

} // mfxStatus VAAPIEncoder::Execute(ExecuteBuffers* pExecuteBuffers, mfxU32 funcId)


mfxStatus VAAPIEncoder::SetFrames (ExecuteBuffers* pExecuteBuffers)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxI32 ind = 0;

    if (pExecuteBuffers->m_RecFrameMemID)
    {
        ind = GetRecFrameIndex(pExecuteBuffers->m_RecFrameMemID);
    }
    else
    {
        ind = 0xff;
    }

    pExecuteBuffers->m_pps.CurrReconstructedPic.Index7Bits     =  mfxU8(ind < 0 ? 0 : ind);
    pExecuteBuffers->m_pps.CurrReconstructedPic.AssociatedFlag =  0;
    pExecuteBuffers->m_idxMb = (DWORD)ind;
    pExecuteBuffers->m_idxBs = (DWORD)ind;

    if (pExecuteBuffers->m_bUseRawFrames)
    {
        ind = GetRawFrameIndex(pExecuteBuffers->m_CurrFrameMemID,true);
        MFX_CHECK(ind>=0,MFX_ERR_NOT_FOUND);
    }
    //else CurrOriginalPic == CurrReconstructedPic

    pExecuteBuffers->m_pps.CurrOriginalPic.Index7Bits     =  mfxU8(ind);
    pExecuteBuffers->m_pps.CurrOriginalPic.AssociatedFlag =  0;

    if (pExecuteBuffers->m_RefFrameMemID[0])
    {
        ind =  GetRecFrameIndex(pExecuteBuffers->m_RefFrameMemID[0]);
        MFX_CHECK(ind>=0,MFX_ERR_NOT_FOUND);
        pExecuteBuffers->m_pps.RefFrameList[0].Index7Bits = mfxU8(ind);
        pExecuteBuffers->m_pps.RefFrameList[0].AssociatedFlag = 0;
    }
    else
    {
        pExecuteBuffers->m_pps.RefFrameList[0].bPicEntry = 0xff;
    }

    if (pExecuteBuffers->m_RefFrameMemID[1])
    {
        ind = GetRecFrameIndex(pExecuteBuffers->m_RefFrameMemID[1]);
        MFX_CHECK(ind>=0,MFX_ERR_NOT_FOUND);
        pExecuteBuffers->m_pps.RefFrameList[1].Index7Bits = mfxU8(ind);
        pExecuteBuffers->m_pps.RefFrameList[1].AssociatedFlag = 0;
    }
    else
    {
        pExecuteBuffers->m_pps.RefFrameList[1].bPicEntry = 0xff;
    }
    if (pExecuteBuffers->m_bExternalCurrFrame)
    {
        sts = m_core->GetExternalFrameHDL(pExecuteBuffers->m_CurrFrameMemID,(mfxHDL *)&pExecuteBuffers->m_pSurface);
    }
    else
    {
        sts = m_core->GetFrameHDL(pExecuteBuffers->m_CurrFrameMemID,(mfxHDL *)&pExecuteBuffers->m_pSurface);
    }
    MFX_CHECK_STS(sts);

    /*printf("CurrOriginalPic %d, CurrReconstructedPic %d, RefFrameList[0] %d, RefFrameList[1] %d\n",
            pExecuteBuffers->m_pps.CurrOriginalPic.Index7Bits,
            pExecuteBuffers->m_pps.CurrReconstructedPic.Index7Bits,
            pExecuteBuffers->m_pps.RefFrameList[0].Index7Bits,
            pExecuteBuffers->m_pps.RefFrameList[1].Index7Bits);*/

    return sts;
} // mfxStatus VAAPIEncoder::SetFrames (ExecuteBuffers* pExecuteBuffers)


mfxStatus VAAPIEncoder::Execute(ExecuteBuffers* pExecuteBuffers, mfxU8 *pUserData, mfxU32 userDataLen)
{
    MFX::AutoTimer timer(__FUNCTION__);

    mfxStatus sts = MFX_ERR_NONE;

    sts = Execute(pExecuteBuffers, ENCODE_ENC_PAK_ID, pUserData, userDataLen);
    MFX_CHECK_STS(sts);

    return sts;

} // mfxStatus VAAPIEncoder::Execute(ExecuteBuffers* pExecuteBuffers, mfxU8 *pUserData, mfxU32 userDataLen)


mfxStatus VAAPIEncoder::Close()
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VAAPIEncoder::Close");
    delete [] (mfxU8 *)m_pMiscParamsFps;
    m_pMiscParamsFps = 0;
    delete [] (mfxU8 *)m_pMiscParamsBrc;
    m_pMiscParamsBrc = 0;
    delete [] (mfxU8 *)m_pMiscParamsQuality;
    m_pMiscParamsQuality = 0;
    delete [] m_pMiscParamsSeqInfo;
    m_pMiscParamsSeqInfo = 0;
    delete [] m_pMiscParamsSkipFrame;
    m_pMiscParamsSkipFrame = 0;

    mfxStatus sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_spsBufferId);
    std::ignore = MFX_STS_TRACE(sts);

    sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_qmBufferId);
    std::ignore = MFX_STS_TRACE(sts);

    sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_ppsBufferId);
    std::ignore = MFX_STS_TRACE(sts);

    for (mfxU32 i = 0; i < MAX_SLICES; i++)
    {
        sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_sliceParamBufferId[i]);
        std::ignore = MFX_STS_TRACE(sts);
    }

    sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_miscParamFpsId);
    std::ignore = MFX_STS_TRACE(sts);

    sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_miscParamBrcId);
    std::ignore = MFX_STS_TRACE(sts);

    sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_miscParamQualityId);
    std::ignore = MFX_STS_TRACE(sts);

    sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_miscQualityParamId);
    std::ignore = MFX_STS_TRACE(sts);

    sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_miscParamSeqInfoId);
    std::ignore = MFX_STS_TRACE(sts);

    sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_miscParamSkipFrameId);
    std::ignore = MFX_STS_TRACE(sts);

    sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_packedUserDataParamsId);
    std::ignore = MFX_STS_TRACE(sts);

    sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_packedUserDataId);
    std::ignore = MFX_STS_TRACE(sts);

    sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_mbqpBufferId);
    std::ignore = MFX_STS_TRACE(sts);

    if(m_MaxContextPriority)
    {
        sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_priorityBufferId);
        std::ignore = MFX_STS_TRACE(sts);
    }

    if (m_allocResponseMB.NumFrameActual != 0)
    {
        m_core->FreeFrames(&m_allocResponseMB);
        Zero(m_allocResponseMB);
    }
    if (m_allocResponseBS.NumFrameActual != 0)
    {
        m_core->FreeFrames(&m_allocResponseBS);
        Zero(m_allocResponseBS);
    }

    m_bsQueue.clear();
    m_reconQueue.clear();

    if (m_vaContextEncode != VA_INVALID_ID)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaDestroyContext");

        VAStatus vaSts = vaDestroyContext(m_vaDisplay, m_vaContextEncode);
        std::ignore = MFX_STS_TRACE(vaSts);
        m_vaContextEncode = VA_INVALID_ID;
    }

    if (m_vaConfig != VA_INVALID_ID)
    {
        VAStatus vaSts = vaDestroyConfig(m_vaDisplay, m_vaConfig);
        std::ignore = MFX_STS_TRACE(vaSts);
        m_vaConfig = VA_INVALID_ID;
    }

    return MFX_ERR_NONE;
} // mfxStatus VAAPIEncoder::Close()


mfxStatus VAAPIEncoder::FillMBBufferPointer(ExecuteBuffers* pExecuteBuffers)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_INTERNAL, "CopyMB");
    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameData Frame = {};

    //pExecuteBuffers->m_idxMb = 0;
    if (pExecuteBuffers->m_idxMb >= DWORD(m_allocResponseMB.NumFrameActual))
    {
        return MFX_ERR_UNSUPPORTED;
    }

    Frame.MemId = m_allocResponseMB.mids[pExecuteBuffers->m_idxMb];
    sts = m_core->LockFrame(Frame.MemId,&Frame);
    MFX_CHECK_STS(sts);

    int numMB = 0;
    for (int i = 0; i<(int)pExecuteBuffers->m_pps.NumSlice;i++)
    {
        numMB = numMB + (int)pExecuteBuffers->m_pSlice[i].NumMbsForSlice;
    }
    //{
    //    FILE* f = fopen("MB data.txt","ab+");
    //    fwrite(Frame.Y,1,Frame.Pitch*numMB,f);
    //    fclose(f);
    //}

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_INTERNAL, "CopyMBData");
        mfxFrameSurface1 src = {};
        mfxFrameSurface1 dst = {};

        src.Data        = Frame;
        src.Data.Y     += m_layout.MB_CODE_offset;
        src.Data.Pitch  = mfxU16(m_layout.MB_CODE_stride);
        src.Info.Width  = mfxU16(sizeof(ENCODE_ENC_MB_DATA_MPEG2));
        src.Info.Height = mfxU16(numMB);
        src.Info.FourCC = MFX_FOURCC_P8;
        dst.Data.Y      = (mfxU8 *)pExecuteBuffers->m_pMBs;
        dst.Data.Pitch  = mfxU16(sizeof(ENCODE_ENC_MB_DATA_MPEG2));
        dst.Info.Width  = mfxU16(sizeof(ENCODE_ENC_MB_DATA_MPEG2));
        dst.Info.Height = mfxU16(numMB);
        dst.Info.FourCC = MFX_FOURCC_P8;

        sts = m_core->DoFastCopyExtended(&dst, &src);
        MFX_CHECK_STS(sts);
    }

    sts = m_core->UnlockFrame(Frame.MemId);
    MFX_CHECK_STS(sts);

    return sts;

} // mfxStatus VAAPIEncoder::FillMBBufferPointer(ExecuteBuffers* pExecuteBuffers)

mfxStatus VAAPIEncoder::FillBSBuffer(mfxU32 nFeedback,mfxU32 nBitstream, mfxBitstream* pBitstream, Encryption *pEncrypt)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VAAPIEncoder::FillBSBuffer");

    mfxStatus sts = MFX_ERR_NONE;

    //MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_SCHED, "Enc QueryStatus");
    VAStatus vaSts;

    //------------------------------------------
    // (1) mapping feedbackNumber -> surface & bs
    bool isFound = false;
#if !VA_CHECK_VERSION(1,9,0)
    VASurfaceID waitSurface;
#endif
    mfxU32 waitIdxBs;
    mfxU32 indxSurf;
    mfxU32 bitstreamSize = 0;

    UMC::AutomaticUMCMutex guard(m_guard);

    for (indxSurf = 0; indxSurf < m_feedback.size(); indxSurf++)
    {
        ExtVASurface currentFeedback = m_feedback[ indxSurf ];

        if (currentFeedback.number == nFeedback)
        {
#if !VA_CHECK_VERSION(1,9,0)
            waitSurface = currentFeedback.surface;
#endif
            waitIdxBs   = currentFeedback.idxBs;
            isFound  = true;

            break;
        }
    }

    if( !isFound )
    {
        return MFX_ERR_UNKNOWN;
    }

    // find used bitstream
    VABufferID codedBuffer;
    if( waitIdxBs < m_bsQueue.size())
    {
        codedBuffer = m_bsQueue[waitIdxBs].surface;
    }
    else
    {
        return MFX_ERR_UNKNOWN;
    }

    VASurfaceStatus surfSts = VASurfaceSkipped;

#if VA_CHECK_VERSION(1,9,0)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaSyncBuffer");
        vaSts = vaSyncBuffer(m_vaDisplay, codedBuffer, VA_TIMEOUT_INFINITE);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }
#else
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaSyncSurface");
        vaSts = vaSyncSurface(m_vaDisplay, waitSurface);
        // following code is workaround:
        // because of driver bug it could happen that decoding error will not be returned after decoder sync
        // and will be returned at subsequent encoder sync instead
        // just ignore VA_STATUS_ERROR_DECODING_ERROR in encoder
        if (vaSts == VA_STATUS_ERROR_DECODING_ERROR)
            vaSts = VA_STATUS_SUCCESS;
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }
#endif
    surfSts = VASurfaceReady;

    switch (surfSts)
    {
    case VASurfaceReady:
        break;
        // return MFX_ERR_NONE;

    case VASurfaceRendering:
    case VASurfaceDisplaying:
        return MFX_WRN_DEVICE_BUSY;

    case VASurfaceSkipped:
    default:
        assert(!"bad feedback status");
        return MFX_ERR_DEVICE_FAILED;
    }

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "CopyBitsream");

        VACodedBufferSegment *codedBufferSegment;
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaMapBuffer");
            vaSts = vaMapBuffer(
                m_vaDisplay,
                codedBuffer,
                (void **)(&codedBufferSegment));
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        }
        // m_codedbufISize same as m_codedbufPBSize, we should not read more than allocated
        if (codedBufferSegment->size > m_codedbufISize)
            sts = MFX_ERR_DEVICE_FAILED;
        else
            bitstreamSize = codedBufferSegment->size;

        if (codedBufferSegment->status & VA_CODED_BUF_STATUS_BAD_BITSTREAM)
            sts = MFX_ERR_GPU_HANG;
        else if (!codedBufferSegment->size || !codedBufferSegment->buf)
            sts = MFX_ERR_DEVICE_FAILED;

        MFX_CHECK_STS(sts);

        // remove task
        m_feedback.erase( m_feedback.begin() + indxSurf );

        MFX_CHECK(pBitstream->DataLength + pBitstream->DataOffset + bitstreamSize < pBitstream->MaxLength, MFX_ERR_NOT_ENOUGH_BUFFER);

        mfxSize roi = {(mfxI32)bitstreamSize, 1};

        mfxU8 *pData = (mfxU8*)codedBufferSegment->buf;

        mfxStatus ret = FastCopy::Copy(pBitstream->Data + pBitstream->DataLength + pBitstream->DataOffset,  bitstreamSize,
            pData, bitstreamSize,
            roi, COPY_VIDEO_TO_SYS);

        MFX_CHECK(ret == MFX_ERR_NONE, MFX_ERR_UNDEFINED_BEHAVIOR);

        pBitstream->DataLength += bitstreamSize;

        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
            vaSts = vaUnmapBuffer( m_vaDisplay, codedBuffer );
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        }
    }

    return sts;
} // mfxStatus VAAPIEncoder::FillBSBuffer(mfxU32 nFeedback,mfxU32 nBitstream, mfxBitstream* pBitstream, Encryption *pEncrypt)

#endif // (MFX_ENABLE_MPEG2_VIDEO_ENCODE) && (MFX_VA_LINUX)
/* EOF */
