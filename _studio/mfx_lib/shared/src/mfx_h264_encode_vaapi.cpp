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

#include "mfx_common.h"

#if defined(MFX_ENABLE_H264_VIDEO_ENCODE_HW) && defined(MFX_VA_LINUX)

#include <va/va.h>
#include <va/va_enc_h264.h>
#include "mfxfei.h"
#include "libmfx_core_vaapi.h"
#include "mfx_common_int.h"
#include "mfx_h264_encode_vaapi.h"
#include "mfx_h264_encode_hw_utils.h"

#if defined(_DEBUG)
//#define mdprintf fprintf
#define mdprintf(...)
#else
#define mdprintf(...)
#endif

using namespace MfxHwH264Encode;

static mfxU8 ConvertMfxFrameType2VaapiSliceType(mfxU8 type)
{
    switch (type & MFX_FRAMETYPE_IPB)
    {
    case MFX_FRAMETYPE_I:
    case MFX_FRAMETYPE_P:
    case MFX_FRAMETYPE_B:
        return ConvertMfxFrameType2SliceType(type) % 5;

    default:
        assert("bad codingType");
        return 0xff;
    }
}

mfxU8 ConvertRateControlMFX2VAAPI(mfxU8 rateControl)
{
    switch (rateControl)
    {
    case MFX_RATECONTROL_CBR:  return VA_RC_CBR;
    case MFX_RATECONTROL_VBR:  return VA_RC_VBR;
    case MFX_RATECONTROL_AVBR: return VA_RC_VBR;
    case MFX_RATECONTROL_CQP:  return VA_RC_CQP;
    default: assert(!"Unsupported RateControl"); return 0;
    }

} // mfxU8 ConvertRateControlMFX2VAAPI(mfxU8 rateControl)

VAProfile ConvertProfileTypeMFX2VAAPI(mfxU32 type)
{
    switch (type)
    {
        case MFX_PROFILE_AVC_CONSTRAINED_BASELINE:
        case MFX_PROFILE_AVC_BASELINE:
            return VAProfileH264ConstrainedBaseline;
        case MFX_PROFILE_AVC_MAIN:
            return VAProfileH264Main;
        case MFX_PROFILE_AVC_HIGH:
            return VAProfileH264High;
        default:
            //assert(!"Unsupported profile type");
            return VAProfileH264High; //VAProfileNone;
    }

} // VAProfile ConvertProfileTypeMFX2VAAPI(mfxU8 type)

mfxU8 ConvertSliceStructureVAAPIToMFX(mfxU8 structure)
{
    switch (structure)
    {
        case VA_ENC_SLICE_STRUCTURE_POWER_OF_TWO_ROWS:
            return 1;
        case VA_ENC_SLICE_STRUCTURE_EQUAL_ROWS:
            return 2;
        case VA_ENC_SLICE_STRUCTURE_ARBITRARY_ROWS:
            return 3;
        case VA_ENC_SLICE_STRUCTURE_ARBITRARY_MACROBLOCKS:
            return 4;
        default:
            return 0;
    }
}

mfxStatus SetHRD(
    MfxVideoParam const & par,
    VADisplay    vaDisplay,
    VAContextID  vaContextEncode,
    VABufferID & hrdBuf_id)
{
    VAStatus vaSts;
    VAEncMiscParameterBuffer *misc_param;
    VAEncMiscParameterHRD *hrd_param;

    MFX_DESTROY_VABUFFER(hrdBuf_id, vaDisplay);

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaCreateBuffer");
        vaSts = vaCreateBuffer(vaDisplay,
            vaContextEncode,
            VAEncMiscParameterBufferType,
            sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterHRD),
            1,
            NULL,
            &hrdBuf_id);
    }
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    vaSts = vaMapBuffer(vaDisplay,
                 hrdBuf_id,
                (void **)&misc_param);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    misc_param->type = VAEncMiscParameterTypeHRD;
    hrd_param = (VAEncMiscParameterHRD *)misc_param->data;

    hrd_param->initial_buffer_fullness = par.calcParam.initialDelayInKB * 8000;
    hrd_param->buffer_size = par.calcParam.bufferSizeInKB * 8000;

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
        vaSts = vaUnmapBuffer(vaDisplay, hrdBuf_id);
    }
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    return MFX_ERR_NONE;
} // void SetHRD(...)

void FillBrcStructures(
    MfxVideoParam const & par,
    VAEncMiscParameterRateControl & vaBrcPar,
    VAEncMiscParameterFrameRate   & vaFrameRate)
{
    Zero(vaBrcPar);
    Zero(vaFrameRate);
    vaBrcPar.bits_per_second = GetMaxBitrateValue(par.calcParam.maxKbps) << (6 + SCALE_FROM_DRIVER);
    if(par.calcParam.maxKbps)
        vaBrcPar.target_percentage = (unsigned int)(100.0 * (mfxF64)par.calcParam.targetKbps / (mfxF64)par.calcParam.maxKbps);
    PackMfxFrameRate(par.mfx.FrameInfo.FrameRateExtN, par.mfx.FrameInfo.FrameRateExtD, vaFrameRate.framerate);
}

mfxStatus SetRateControl(
    MfxVideoParam const & par,
    mfxU32       mbbrc,
    mfxU8        minQP,
    mfxU8        maxQP,
    VADisplay    vaDisplay,
    VAContextID  vaContextEncode,
    VABufferID & rateParamBuf_id,
    bool         isBrcResetRequired = false)
{
    VAStatus vaSts;
    VAEncMiscParameterBuffer *misc_param;
    VAEncMiscParameterRateControl *rate_param;
    mfxExtCodingOption3 const * extOpt3  = GetExtBuffer(par);

    if ( rateParamBuf_id != VA_INVALID_ID)
    {
        vaDestroyBuffer(vaDisplay, rateParamBuf_id);
    }

    vaSts = vaCreateBuffer(vaDisplay,
                   vaContextEncode,
                   VAEncMiscParameterBufferType,
                   sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterRateControl),
                   1,
                   NULL,
                   &rateParamBuf_id);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    vaSts = vaMapBuffer(vaDisplay,
                 rateParamBuf_id,
                (void **)&misc_param);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    misc_param->type = VAEncMiscParameterTypeRateControl;
    rate_param = (VAEncMiscParameterRateControl *)misc_param->data;

    rate_param->bits_per_second = GetMaxBitrateValue(par.calcParam.maxKbps) << (6 + SCALE_FROM_DRIVER);
    rate_param->window_size     = par.mfx.Convergence * 100;

    rate_param->min_qp = minQP;
    rate_param->max_qp = maxQP;

    if(par.calcParam.maxKbps)
        rate_param->target_percentage = (unsigned int)(100.0 * (mfxF64)par.calcParam.targetKbps / (mfxF64)par.calcParam.maxKbps);

#if defined(LINUX_TARGET_PLATFORM_BXTMIN) || defined(LINUX_TARGET_PLATFORM_BXT) || defined(LINUX_TARGET_PLATFORM_CFL)
    // Activate frame tolerance sliding window mode
    if (extOpt3->WinBRCSize) {
        rate_param->rc_flags.bits.frame_tolerance_mode = 1;
    }
#endif  // defined(LINUX_TARGET_PLATFORM_BXTMIN) || defined(LINUX_TARGET_PLATFORM_BXT) || defined(LINUX_TARGET_PLATFORM_CFL)

/*
 * MBBRC control
 * Control VA_RC_MB 0: default, 1: enable, 2: disable, other: reserved
 */
    rate_param->rc_flags.bits.mb_rate_control = mbbrc & 0xf;
    rate_param->rc_flags.bits.reset = isBrcResetRequired;

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
        vaSts = vaUnmapBuffer(vaDisplay, rateParamBuf_id);
    }
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    return MFX_ERR_NONE;
}

mfxStatus SetFrameRate(
    MfxVideoParam const & par,
    VADisplay    vaDisplay,
    VAContextID  vaContextEncode,
    VABufferID & frameRateBuf_id)
{
    VAStatus vaSts;
    VAEncMiscParameterBuffer *misc_param;
    VAEncMiscParameterFrameRate *frameRate_param;

    if ( frameRateBuf_id != VA_INVALID_ID)
    {
        vaDestroyBuffer(vaDisplay, frameRateBuf_id);
    }

    vaSts = vaCreateBuffer(vaDisplay,
                   vaContextEncode,
                   VAEncMiscParameterBufferType,
                   sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterFrameRate),
                   1,
                   NULL,
                   &frameRateBuf_id);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    vaSts = vaMapBuffer(vaDisplay,
                 frameRateBuf_id,
                (void **)&misc_param);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    misc_param->type = VAEncMiscParameterTypeFrameRate;

    frameRate_param = (VAEncMiscParameterFrameRate *)misc_param->data;
    PackMfxFrameRate(par.mfx.FrameInfo.FrameRateExtN, par.mfx.FrameInfo.FrameRateExtD, frameRate_param->framerate);

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
        vaSts = vaUnmapBuffer(vaDisplay, frameRateBuf_id);
    }
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    return MFX_ERR_NONE;
}

static mfxStatus SetMaxFrameSize(
    const UINT   userMaxFrameSize,
    VADisplay    vaDisplay,
    VAContextID  vaContextEncode,
    VABufferID & frameSizeBuf_id)
{
    VAEncMiscParameterBuffer             *misc_param;
    VAEncMiscParameterBufferMaxFrameSize *p_maxFrameSize;

    if ( frameSizeBuf_id != VA_INVALID_ID)
    {
        vaDestroyBuffer(vaDisplay, frameSizeBuf_id);
    }

    VAStatus vaSts = vaCreateBuffer(vaDisplay,
                   vaContextEncode,
                   VAEncMiscParameterBufferType,
                   sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterBufferMaxFrameSize),
                   1,
                   NULL,
                   &frameSizeBuf_id);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    vaSts = vaMapBuffer(vaDisplay, frameSizeBuf_id, (void **)&misc_param);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    misc_param->type = VAEncMiscParameterTypeMaxFrameSize;
    p_maxFrameSize = (VAEncMiscParameterBufferMaxFrameSize *)misc_param->data;

    p_maxFrameSize->max_frame_size = userMaxFrameSize*8;    // in bits for libva

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
        vaSts = vaUnmapBuffer(vaDisplay, frameSizeBuf_id);
    }
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    return MFX_ERR_NONE;
}

static mfxStatus SetTrellisQuantization(
    mfxU32       trellis,
    VADisplay    vaDisplay,
    VAContextID  vaContextEncode,
    VABufferID & trellisBuf_id)
{
    VAStatus vaSts;
    VAEncMiscParameterBuffer        *misc_param;
    VAEncMiscParameterQuantization  *trellis_param;

    if ( trellisBuf_id != VA_INVALID_ID)
    {
        vaDestroyBuffer(vaDisplay, trellisBuf_id);
    }

    vaSts = vaCreateBuffer(vaDisplay,
                   vaContextEncode,
                   VAEncMiscParameterBufferType,
                   sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterQuantization),
                   1,
                   NULL,
                   &trellisBuf_id);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    vaSts = vaMapBuffer(vaDisplay, trellisBuf_id, (void **)&misc_param);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    misc_param->type = VAEncMiscParameterTypeQuantization;
    trellis_param = (VAEncMiscParameterQuantization *)misc_param->data;

    trellis_param->quantization_flags.value = trellis;

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
        vaSts = vaUnmapBuffer(vaDisplay, trellisBuf_id);
    }
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    return MFX_ERR_NONE;
} // void SetTrellisQuantization(...)

static mfxStatus SetRollingIntraRefresh(
    IntraRefreshState const & rirState,
    VADisplay    vaDisplay,
    VAContextID  vaContextEncode,
    VABufferID & rirBuf_id)
{
    VAStatus vaSts;
    VAEncMiscParameterBuffer *misc_param;
    VAEncMiscParameterRIR    *rir_param;

    if ( rirBuf_id != VA_INVALID_ID)
    {
        vaDestroyBuffer(vaDisplay, rirBuf_id);
    }

    vaSts = vaCreateBuffer(vaDisplay,
                   vaContextEncode,
                   VAEncMiscParameterBufferType,
                   sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterRIR),
                   1,
                   NULL,
                   &rirBuf_id);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    vaSts = vaMapBuffer(vaDisplay, rirBuf_id, (void **)&misc_param);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    misc_param->type = VAEncMiscParameterTypeRIR;
    rir_param = (VAEncMiscParameterRIR *)misc_param->data;

    rir_param->rir_flags.value             = rirState.refrType;
    rir_param->intra_insertion_location    = rirState.IntraLocation;
    rir_param->intra_insert_size           = rirState.IntraSize;
    rir_param->qp_delta_for_inserted_intra = mfxU8(rirState.IntRefQPDelta);

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
        vaSts = vaUnmapBuffer(vaDisplay, rirBuf_id);
    }
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    return MFX_ERR_NONE;
} // void SetRollingIntraRefresh(...)

mfxStatus SetQualityParams(
    MfxVideoParam const & par,
    VADisplay    vaDisplay,
    VAContextID  vaContextEncode,
    VABufferID & qualityParams_id,
    mfxEncodeCtrl const * pCtrl)
{
    VAStatus vaSts;
    VAEncMiscParameterBuffer *misc_param;
    VAEncMiscParameterEncQuality *quality_param;
    mfxExtCodingOption2 const * extOpt2  = GetExtBuffer(par);
    mfxExtCodingOption3 const * extOpt3  = GetExtBuffer(par);
    mfxExtFeiCodingOption const * extOptFEI = GetExtBuffer(par);

    if ( qualityParams_id != VA_INVALID_ID)
    {
        vaDestroyBuffer(vaDisplay, qualityParams_id);
    }

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaCreateBuffer");

        vaSts = vaCreateBuffer(vaDisplay,
                   vaContextEncode,
                   VAEncMiscParameterBufferType,
                   sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterEncQuality),
                   1,
                   NULL,
                   &qualityParams_id);
    }
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaMapBuffer");
        vaSts = vaMapBuffer(vaDisplay,
                 qualityParams_id,
                (void **)&misc_param);
    }
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    misc_param->type = (VAEncMiscParameterType)VAEncMiscParameterTypeEncQuality;
    quality_param = (VAEncMiscParameterEncQuality *)misc_param->data;

    quality_param->useRawPicForRef = extOpt2 && IsOn(extOpt2->UseRawRef);

    if (extOpt3)
    {
        quality_param->directBiasAdjustmentEnable       = IsOn(extOpt3->DirectBiasAdjustment);
        quality_param->globalMotionBiasAdjustmentEnable = IsOn(extOpt3->GlobalMotionBiasAdjustment);

        if (quality_param->globalMotionBiasAdjustmentEnable && extOpt3->MVCostScalingFactor < 4)
            quality_param->HMEMVCostScalingFactor = extOpt3->MVCostScalingFactor;

        quality_param->PanicModeDisable = IsOff(extOpt3->BRCPanicMode);

#ifdef MFX_ENABLE_H264_REPARTITION_CHECK
        switch (extOpt3->RepartitionCheckEnable)
        {
        case MFX_CODINGOPTION_ON:
            quality_param->ForceRepartitionCheck = 1;
            break;
        case MFX_CODINGOPTION_OFF:
            quality_param->ForceRepartitionCheck = 2;
            break;
        case MFX_CODINGOPTION_UNKNOWN:
        case MFX_CODINGOPTION_ADAPTIVE:
        default:
            quality_param->ForceRepartitionCheck = 0;
        }
#endif // MFX_ENABLE_H264_REPARTITION_CHECK

    }

    if (extOptFEI)
    {
        quality_param->HMEDisable      = !!extOptFEI->DisableHME;
        quality_param->SuperHMEDisable = !!extOptFEI->DisableSuperHME;
        quality_param->UltraHMEDisable = !!extOptFEI->DisableUltraHME;
    }

    if (pCtrl)
    {
        mfxExtCodingOption2 const * extOpt2rt  = GetExtBuffer(*pCtrl);
        mfxExtCodingOption3 const * extOpt3rt  = GetExtBuffer(*pCtrl);

        if (extOpt2rt)
            quality_param->useRawPicForRef = IsOn(extOpt2rt->UseRawRef);

        if (extOpt3rt)
        {
            quality_param->directBiasAdjustmentEnable       = IsOn(extOpt3rt->DirectBiasAdjustment);
            quality_param->globalMotionBiasAdjustmentEnable = IsOn(extOpt3rt->GlobalMotionBiasAdjustment);

            if (quality_param->globalMotionBiasAdjustmentEnable && extOpt3rt->MVCostScalingFactor < 4)
                quality_param->HMEMVCostScalingFactor = extOpt3rt->MVCostScalingFactor;

#ifdef MFX_ENABLE_H264_REPARTITION_CHECK
            switch (extOpt3rt->RepartitionCheckEnable)
            {
            case MFX_CODINGOPTION_ON:
                quality_param->ForceRepartitionCheck = 1;
                break;
            case MFX_CODINGOPTION_OFF:
                quality_param->ForceRepartitionCheck = 2;
                break;
            case MFX_CODINGOPTION_UNKNOWN:
                // lets stick with option specified on init if not specified per-frame
                break;
            case MFX_CODINGOPTION_ADAPTIVE:
            default:
                quality_param->ForceRepartitionCheck = 0;
            }
#endif // MFX_ENABLE_H264_REPARTITION_CHECK

        } // if (extOpt3rt)

    }

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
        vaSts = vaUnmapBuffer(vaDisplay, qualityParams_id);
    }
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    return MFX_ERR_NONE;
} // void SetQualityParams(...)

mfxStatus SetQualityLevel(
    MfxVideoParam const & par,
    VADisplay    vaDisplay,
    VAContextID  vaContextEncode,
    VABufferID & qualityParams_id,
    mfxEncodeCtrl const * pCtrl)
{
    VAStatus vaSts;
    VAEncMiscParameterBuffer *misc_param;
    VAEncMiscParameterBufferQualityLevel *quality_param;

    MFX_DESTROY_VABUFFER(qualityParams_id, vaDisplay);

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaCreateBuffer");
        vaSts = vaCreateBuffer(vaDisplay,
            vaContextEncode,
            VAEncMiscParameterBufferType,
            sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterBufferQualityLevel),
            1,
            NULL,
            &qualityParams_id);
    }
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaMapBuffer");
        vaSts = vaMapBuffer(vaDisplay,
            qualityParams_id,
            (void **)&misc_param);
    }
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    misc_param->type = (VAEncMiscParameterType)VAEncMiscParameterTypeQualityLevel;
    quality_param = (VAEncMiscParameterBufferQualityLevel *)misc_param->data;

    quality_param->quality_level = (unsigned int)(par.mfx.TargetUsage);

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
        vaSts = vaUnmapBuffer(vaDisplay, qualityParams_id);
    }
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    return MFX_ERR_NONE;
} // SetQualityLevel

mfxStatus SetSkipFrame(
    VADisplay    vaDisplay,
    VAContextID  vaContextEncode,
    VABufferID & skipParam_id,
    mfxU8 skipFlag,
    mfxU8 numSkipFrames,
    mfxU32 sizeSkipFrames)
{
    VAStatus vaSts;
    VAEncMiscParameterBuffer *misc_param;
    VAEncMiscParameterSkipFrame *skipParam;

    MFX_DESTROY_VABUFFER(skipParam_id, vaDisplay);

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaCreateBuffer");
        vaSts = vaCreateBuffer(vaDisplay,
            vaContextEncode,
            VAEncMiscParameterBufferType,
            sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterSkipFrame),
            1,
            NULL,
            &skipParam_id);
    }
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaMapBuffer");
        vaSts = vaMapBuffer(vaDisplay,
            skipParam_id,
            (void **)&misc_param);
    }
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    misc_param->type = (VAEncMiscParameterType)VAEncMiscParameterTypeSkipFrame;
    skipParam = (VAEncMiscParameterSkipFrame *)misc_param->data;

    skipParam->skip_frame_flag  = skipFlag;
    skipParam->num_skip_frames  = numSkipFrames;
    skipParam->size_skip_frames = sizeSkipFrames;

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
        vaSts = vaUnmapBuffer(vaDisplay, skipParam_id);
    }
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    return MFX_ERR_NONE;
}

static mfxStatus SetROI(
    DdiTask const & task,
    std::vector<VAEncROI> & arrayVAEncROI,
    VADisplay    vaDisplay,
    VAContextID  vaContextEncode,
    VABufferID & roiParam_id)
{
    VAStatus vaSts;
    VAEncMiscParameterBuffer *misc_param;

    VAEncMiscParameterBufferROI *roi_Param;
    unsigned int roi_buffer_size = sizeof(VAEncMiscParameterBufferROI);

    MFX_DESTROY_VABUFFER(roiParam_id, vaDisplay);

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaCreateBuffer");
        vaSts = vaCreateBuffer(vaDisplay,
            vaContextEncode,
            VAEncMiscParameterBufferType,
            sizeof(VAEncMiscParameterBuffer) + roi_buffer_size,
            1,
            NULL,
            &roiParam_id);
    }
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaMapBuffer");
        vaSts = vaMapBuffer(vaDisplay,
            roiParam_id,
            (void **)&misc_param);
    }
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    misc_param->type = (VAEncMiscParameterType)VAEncMiscParameterTypeROI;
    roi_Param = (VAEncMiscParameterBufferROI *)misc_param->data;
    memset(roi_Param, 0, roi_buffer_size);

    if (task.m_numRoi)
    {
        roi_Param->num_roi = task.m_numRoi;

        if (arrayVAEncROI.size() < task.m_numRoi)
        {
            arrayVAEncROI.resize(task.m_numRoi);
        }
        roi_Param->roi = Begin(arrayVAEncROI);
        memset(roi_Param->roi, 0, task.m_numRoi * sizeof(VAEncROI));

        for (mfxU32 i = 0; i < task.m_numRoi; i++)
        {
            roi_Param->roi[i].roi_rectangle.x = task.m_roi[i].Left;
            roi_Param->roi[i].roi_rectangle.y = task.m_roi[i].Top;
            roi_Param->roi[i].roi_rectangle.width  = task.m_roi[i].Right  - task.m_roi[i].Left;
            roi_Param->roi[i].roi_rectangle.height = task.m_roi[i].Bottom - task.m_roi[i].Top;
            roi_Param->roi[i].roi_value = task.m_roi[i].ROIValue;
        }
        roi_Param->max_delta_qp = 51;
        roi_Param->min_delta_qp = -51;

        roi_Param->roi_flags.bits.roi_value_is_qp_delta = 0;
#if MFX_VERSION > 1021
        if (task.m_roiMode == MFX_ROI_MODE_QP_DELTA) {
            roi_Param->roi_flags.bits.roi_value_is_qp_delta = 1;
        }
#endif // MFX_VERSION > 1021
    }

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
        vaSts = vaUnmapBuffer(vaDisplay, roiParam_id);
    }
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);


    return MFX_ERR_NONE;
}

void FillConstPartOfPps(
    MfxVideoParam const & par,
    VAEncPictureParameterBufferH264 & pps)
{
    mfxExtPpsHeader const *       extPps = GetExtBuffer(par);
    mfxExtSpsHeader const *       extSps = GetExtBuffer(par);
    assert( extPps != 0 );
    assert( extSps != 0 );

    if (!extPps || !extSps)
        return;

    pps.seq_parameter_set_id = 0; // extSps->seqParameterSetId; - driver doesn't support non-zero sps_id
    pps.pic_parameter_set_id = 0; // extPps->picParameterSetId; - driver doesn't support non-zero pps_id

    pps.last_picture = 0;
    pps.frame_num = 0;

    pps.pic_init_qp = extPps->picInitQpMinus26 + 26;
    pps.num_ref_idx_l0_active_minus1            = extPps->numRefIdxL0DefaultActiveMinus1;
    pps.num_ref_idx_l1_active_minus1            = extPps->numRefIdxL1DefaultActiveMinus1;
    pps.chroma_qp_index_offset                  = extPps->chromaQpIndexOffset;
    pps.second_chroma_qp_index_offset           = extPps->secondChromaQpIndexOffset;

    pps.pic_fields.bits.deblocking_filter_control_present_flag  = 1;
    pps.pic_fields.bits.entropy_coding_mode_flag                = extPps->entropyCodingModeFlag;

    pps.pic_fields.bits.pic_order_present_flag                  = extPps->bottomFieldPicOrderInframePresentFlag;
    pps.pic_fields.bits.weighted_pred_flag                      = extPps->weightedPredFlag;
    pps.pic_fields.bits.weighted_bipred_idc                     = extPps->weightedBipredIdc;
    pps.pic_fields.bits.constrained_intra_pred_flag             = extPps->constrainedIntraPredFlag;
    pps.pic_fields.bits.transform_8x8_mode_flag                 = extPps->transform8x8ModeFlag;
    pps.pic_fields.bits.pic_scaling_matrix_present_flag         = extPps->picScalingMatrixPresentFlag;

    mfxU32 i = 0;
    for( i = 0; i < 16; i++ )
    {
        pps.ReferenceFrames[i].picture_id = VA_INVALID_ID;
    }

} // void FillConstPartOfPps(...)
namespace MfxHwH264Encode
{
void UpdatePPS(
    DdiTask const & task,
    mfxU32          fieldId,
    VAEncPictureParameterBufferH264 & pps,
    std::vector<ExtVASurface> const & reconQueue)
{
    pps.frame_num = task.m_frameNum;

    pps.pic_fields.bits.idr_pic_flag       = (task.m_type[fieldId] & MFX_FRAMETYPE_IDR) ? 1 : 0;
    pps.pic_fields.bits.reference_pic_flag = (task.m_type[fieldId] & MFX_FRAMETYPE_REF) ? 1 : 0;

    pps.CurrPic.TopFieldOrderCnt    = task.GetPoc(TFIELD);
    pps.CurrPic.BottomFieldOrderCnt = task.GetPoc(BFIELD);
    if (task.GetPicStructForEncode() != MFX_PICSTRUCT_PROGRESSIVE)
        pps.CurrPic.flags = TFIELD == fieldId ? VA_PICTURE_H264_TOP_FIELD : VA_PICTURE_H264_BOTTOM_FIELD;
    else
        pps.CurrPic.flags = 0;

    mfxU32 i   = 0;
    mfxU32 idx = 0;

    for( i = 0; i < task.m_dpb[fieldId].Size(); i++ )
    {
        pps.ReferenceFrames[i].frame_idx = idx     = task.m_dpb[fieldId][i].m_frameIdx & 0x7f;
        pps.ReferenceFrames[i].picture_id          = reconQueue[idx].surface;
        pps.ReferenceFrames[i].flags               = task.m_dpb[fieldId][i].m_longterm ? VA_PICTURE_H264_LONG_TERM_REFERENCE : VA_PICTURE_H264_SHORT_TERM_REFERENCE;
        pps.ReferenceFrames[i].TopFieldOrderCnt    = task.m_dpb[fieldId][i].m_poc[0];
        pps.ReferenceFrames[i].BottomFieldOrderCnt = task.m_dpb[fieldId][i].m_poc[1];
    }

    for (; i < 16; i++)
    {
        pps.ReferenceFrames[i].picture_id          = VA_INVALID_ID;
        pps.ReferenceFrames[i].frame_idx           = 0xff;
        pps.ReferenceFrames[i].flags               = VA_PICTURE_H264_INVALID;
        pps.ReferenceFrames[i].TopFieldOrderCnt    = 0;
        pps.ReferenceFrames[i].BottomFieldOrderCnt = 0;
    }
} // void UpdatePPS(...)

void FillPWT(
    ENCODE_CAPS const &                         hwCaps,
    VAEncPictureParameterBufferH264 const &     pps,
    mfxExtPredWeightTable const &               pwt,
    VAEncSliceParameterBufferH264 &             slice)
{
    mfxU32 iNumRefL0 = 0;
    mfxU32 iNumRefL1 = 0;
    const mfxU32 iWeight = 0, iOffset = 1, iY = 0, iCb = 1, iCr = 2, iCbVa = 0, iCrVa = 1;
    mfxU32 ref = 0;

    // check parameter
    if (!((pps.pic_fields.bits.weighted_pred_flag == 1 && slice.slice_type % 5 == SLICE_TYPE_P) ||
          (pps.pic_fields.bits.weighted_bipred_idc == 1 && slice.slice_type % 5 == SLICE_TYPE_B)))
        return;

    iNumRefL0 = hwCaps.MaxNum_WeightedPredL0 < slice.num_ref_idx_l0_active_minus1 + 1 ? hwCaps.MaxNum_WeightedPredL0 : slice.num_ref_idx_l0_active_minus1 + 1;
    iNumRefL1 = hwCaps.MaxNum_WeightedPredL1 < slice.num_ref_idx_l1_active_minus1 + 1 ? hwCaps.MaxNum_WeightedPredL1 : slice.num_ref_idx_l1_active_minus1 + 1;

    // initialize
    Zero(slice.luma_log2_weight_denom);
    Zero(slice.chroma_log2_weight_denom);
    Zero(slice.luma_weight_l0_flag);
    Zero(slice.luma_weight_l1_flag);
    Zero(slice.chroma_weight_l0_flag);
    Zero(slice.chroma_weight_l1_flag);
    Zero(slice.luma_weight_l0);
    Zero(slice.luma_weight_l1);
    Zero(slice.luma_offset_l0);
    Zero(slice.luma_offset_l1);
    Zero(slice.chroma_weight_l0);
    Zero(slice.chroma_weight_l1);
    Zero(slice.chroma_offset_l0);
    Zero(slice.chroma_offset_l1);

    slice.luma_log2_weight_denom = (mfxU8)pwt.LumaLog2WeightDenom;
    slice.chroma_log2_weight_denom = (mfxU8)pwt.ChromaLog2WeightDenom;

    if (hwCaps.LumaWeightedPred)
    {
        // Set Luma L0
        if ((slice.slice_type % 5) == SLICE_TYPE_P || (slice.slice_type % 5) == SLICE_TYPE_B)
        {
            for (ref = 0; ref < iNumRefL0; ref++)
            {
                if (pwt.LumaWeightFlag[0][ref])
                {
                    slice.luma_weight_l0_flag |= 1 << ref;
                    Copy(slice.luma_weight_l0[ref], pwt.Weights[0][ref][iY][iWeight]);
                    Copy(slice.luma_offset_l0[ref], pwt.Weights[0][ref][iY][iOffset]);
                }
                else
                {
                    slice.luma_weight_l0[ref] = (1 << slice.luma_log2_weight_denom);
                    slice.luma_offset_l0[ref] = 0;
                }
            }
        }

        // Set Luma L1
        if ((slice.slice_type % 5) == SLICE_TYPE_B)
        {
            for (ref = 0; ref < iNumRefL1; ref++)
            {
                if (pwt.LumaWeightFlag[1][ref])
                {
                    slice.luma_weight_l1_flag |= 1 << ref;
                    Copy(slice.luma_weight_l1[ref], pwt.Weights[1][ref][iY][iWeight]);
                    Copy(slice.luma_offset_l1[ref], pwt.Weights[1][ref][iY][iOffset]);
                }
                else
                {
                    slice.luma_weight_l1[ref] = (1 << slice.luma_log2_weight_denom);
                    slice.luma_offset_l1[ref] = 0;
                }
            }
        }
    }

    if (hwCaps.ChromaWeightedPred)
    {
        // Set Chroma L0
        if ((slice.slice_type % 5) == SLICE_TYPE_P || (slice.slice_type % 5) == SLICE_TYPE_B)
        {
            for (ref = 0; ref < iNumRefL0; ref++)
            {
                if (pwt.ChromaWeightFlag[0][ref])
                {
                    slice.chroma_weight_l0_flag |= 1 << ref;
                    Copy(slice.chroma_weight_l0[ref][iCbVa], pwt.Weights[0][ref][iCb][iWeight]);
                    Copy(slice.chroma_weight_l0[ref][iCrVa], pwt.Weights[0][ref][iCr][iWeight]);
                    Copy(slice.chroma_offset_l0[ref][iCbVa], pwt.Weights[0][ref][iCb][iOffset]);
                    Copy(slice.chroma_offset_l0[ref][iCrVa], pwt.Weights[0][ref][iCr][iOffset]);
                }
                else
                {
                    slice.chroma_weight_l0[ref][iCbVa] = (1 << slice.chroma_log2_weight_denom);
                    slice.chroma_weight_l0[ref][iCrVa] = (1 << slice.chroma_log2_weight_denom);
                    slice.chroma_offset_l0[ref][iCbVa] = 0;
                    slice.chroma_offset_l0[ref][iCrVa] = 0;
                }
            }
        }

        // Set Chroma L1
        if ((slice.slice_type % 5) == SLICE_TYPE_B)
        {
            for (ref = 0; ref < iNumRefL1; ref++)
            {
                if (pwt.ChromaWeightFlag[1][ref])
                {
                    slice.chroma_weight_l1_flag |= 1 << ref;
                    Copy(slice.chroma_weight_l1[ref][iCbVa], pwt.Weights[1][ref][iCb][iWeight]);
                    Copy(slice.chroma_weight_l1[ref][iCrVa], pwt.Weights[1][ref][iCr][iWeight]);
                    Copy(slice.chroma_offset_l1[ref][iCbVa], pwt.Weights[1][ref][iCb][iOffset]);
                    Copy(slice.chroma_offset_l1[ref][iCrVa], pwt.Weights[1][ref][iCr][iOffset]);
                }
                else
                {
                    slice.chroma_weight_l1[ref][iCbVa] = (1 << slice.chroma_log2_weight_denom);
                    slice.chroma_weight_l1[ref][iCrVa] = (1 << slice.chroma_log2_weight_denom);
                    slice.chroma_offset_l1[ref][iCbVa] = 0;
                    slice.chroma_offset_l1[ref][iCrVa] = 0;
                }
            }
        }
    }
}

    void UpdateSlice(
        ENCODE_CAPS const &                         hwCaps,
        DdiTask const &                             task,
        mfxU32                                      fieldId,
        VAEncSequenceParameterBufferH264 const     & sps,
        VAEncPictureParameterBufferH264 const      & pps,
        std::vector<VAEncSliceParameterBufferH264> & slice,
        MfxVideoParam const                        & par,
        std::vector<ExtVASurface> const & reconQueue)
    {
        mfxU32 numPics  = task.GetPicStructForEncode() == MFX_PICSTRUCT_PROGRESSIVE ? 1 : 2;
        if (task.m_numSlice[fieldId])
            slice.resize(task.m_numSlice[fieldId]);
        mfxU32 numSlice = slice.size();
        mfxU32 idx = 0, ref = 0;

        // In case of single-field processing, only one buffer is attached
        mfxU32 idxToPickBuffer = task.m_singleFieldMode ? 0 : task.m_fid[fieldId];

        mfxExtCodingOptionDDI * extDdi      = GetExtBuffer(par);
        mfxExtCodingOption2   * extOpt2     = GetExtBuffer(par);
        mfxExtFeiSliceHeader  * extFeiSlice = GetExtBuffer(par, idxToPickBuffer);
        assert(extDdi      != 0);
        assert(extOpt2     != 0);
        assert(extFeiSlice != 0);

        mfxExtPredWeightTable const * pPWT = GetExtBuffer(task.m_ctrl, idxToPickBuffer);
        if (!pPWT)
            pPWT = &task.m_pwt[fieldId];

        SliceDivider divider = MakeSliceDivider(
            hwCaps.SliceStructure,
            task.m_numMbPerSlice,
            numSlice,
            sps.picture_width_in_mbs,
            sps.picture_height_in_mbs / numPics);

        ArrayDpbFrame const & dpb = task.m_dpb[fieldId];
        ArrayU8x33 const & list0 = task.m_list0[fieldId];
        ArrayU8x33 const & list1 = task.m_list1[fieldId];

        for( size_t i = 0; i < slice.size(); ++i, divider.Next() )
        {
            slice[i].macroblock_address = divider.GetFirstMbInSlice();
            slice[i].num_macroblocks    = divider.GetNumMbInSlice();
            slice[i].macroblock_info    = VA_INVALID_ID;

            for (ref = 0; ref < list0.Size(); ref++)
            {
                slice[i].RefPicList0[ref].frame_idx    = idx  = dpb[list0[ref] & 0x7f].m_frameIdx & 0x7f;
                slice[i].RefPicList0[ref].picture_id          = reconQueue[idx].surface;
                if (task.GetPicStructForEncode() != MFX_PICSTRUCT_PROGRESSIVE)
                    slice[i].RefPicList0[ref].flags               = list0[ref] >> 7 ? VA_PICTURE_H264_BOTTOM_FIELD : VA_PICTURE_H264_TOP_FIELD;
            }
            for (; ref < 32; ref++)
            {
                slice[i].RefPicList0[ref].picture_id = VA_INVALID_ID;
                slice[i].RefPicList0[ref].flags      = VA_PICTURE_H264_INVALID;
            }

            for (ref = 0; ref < list1.Size(); ref++)
            {
                slice[i].RefPicList1[ref].frame_idx    = idx  = dpb[list1[ref] & 0x7f].m_frameIdx & 0x7f;
                slice[i].RefPicList1[ref].picture_id          = reconQueue[idx].surface;
                if (task.GetPicStructForEncode() != MFX_PICSTRUCT_PROGRESSIVE)
                    slice[i].RefPicList1[ref].flags               = list1[ref] >> 7 ? VA_PICTURE_H264_BOTTOM_FIELD : VA_PICTURE_H264_TOP_FIELD;
            }
            for (; ref < 32; ref++)
            {
                slice[i].RefPicList1[ref].picture_id = VA_INVALID_ID;
                slice[i].RefPicList1[ref].flags      = VA_PICTURE_H264_INVALID;
            }

            slice[i].pic_parameter_set_id = pps.pic_parameter_set_id;
            slice[i].slice_type = ConvertMfxFrameType2VaapiSliceType( task.m_type[fieldId] );

            slice[i].direct_spatial_mv_pred_flag = 1;

            slice[i].num_ref_idx_l0_active_minus1 = mfxU8(MFX_MAX(1, task.m_list0[fieldId].Size()) - 1);
            slice[i].num_ref_idx_l1_active_minus1 = mfxU8(MFX_MAX(1, task.m_list1[fieldId].Size()) - 1);
            slice[i].num_ref_idx_active_override_flag   =
                        slice[i].num_ref_idx_l0_active_minus1 != pps.num_ref_idx_l0_active_minus1 ||
                        slice[i].num_ref_idx_l1_active_minus1 != pps.num_ref_idx_l1_active_minus1;

            slice[i].idr_pic_id        = task.m_idrPicId;
            slice[i].pic_order_cnt_lsb = mfxU16(task.GetPoc(fieldId));

            slice[i].delta_pic_order_cnt_bottom         = 0;
            slice[i].delta_pic_order_cnt[0]             = 0;
            slice[i].delta_pic_order_cnt[1]             = 0;
            slice[i].luma_log2_weight_denom             = 0;
            slice[i].chroma_log2_weight_denom           = 0;

            slice[i].cabac_init_idc                     = extDdi ? (mfxU8)extDdi->CabacInitIdcPlus1 - 1 : 0;
            slice[i].slice_qp_delta                     = mfxI8(task.m_cqpValue[fieldId] - pps.pic_init_qp);

            slice[i].disable_deblocking_filter_idc = (extFeiSlice->Slice ? extFeiSlice->Slice[i].DisableDeblockingFilterIdc : extOpt2->DisableDeblockingIdc);
            slice[i].slice_alpha_c0_offset_div2    = (extFeiSlice->Slice ? extFeiSlice->Slice[i].SliceAlphaC0OffsetDiv2     : 0);
            slice[i].slice_beta_offset_div2        = (extFeiSlice->Slice ? extFeiSlice->Slice[i].SliceBetaOffsetDiv2        : 0);

            if (pPWT)
                FillPWT(hwCaps, pps, *pPWT, slice[i]);
        }

    } // void UpdateSlice(...)
} // namespace MfxHwH264Encode

void UpdateSliceSizeLimited(
    ENCODE_CAPS const &                         hwCaps,
    DdiTask const &                             task,
    mfxU32                                      fieldId,
    VAEncSequenceParameterBufferH264 const     & sps,
    VAEncPictureParameterBufferH264 const      & pps,
    std::vector<VAEncSliceParameterBufferH264> & slice,
    MfxVideoParam const                        & par,
    std::vector<ExtVASurface> const & reconQueue)
{
    mfxU32 numPics = task.GetPicStructForEncode() == MFX_PICSTRUCT_PROGRESSIVE ? 1 : 2;
    mfxU32 idx = 0, ref = 0;

    mfxExtCodingOptionDDI * extDdi      = GetExtBuffer(par);
    mfxExtCodingOption2   * extOpt2     = GetExtBuffer(par);
    mfxExtFeiSliceHeader  * extFeiSlice = GetExtBuffer(par, task.m_fid[fieldId]);
    assert(extDdi      != 0);
    assert(extOpt2     != 0);
    assert(extFeiSlice != 0);

    mfxExtPredWeightTable const * pPWT = GetExtBuffer(task.m_ctrl, task.m_fid[fieldId]);
    if (!pPWT)
        pPWT = &task.m_pwt[fieldId];

    size_t numSlices = task.m_SliceInfo.size();
    if (numSlices != slice.size())
    {
        size_t old_size = slice.size();
        slice.resize(numSlices);
        for (size_t i = old_size; i < slice.size(); ++i)
        {
            slice[i] = slice[0];
            //slice[i].slice_id = (mfxU32)i;
        }
    }

    ArrayDpbFrame const & dpb = task.m_dpb[fieldId];
    ArrayU8x33 const & list0 = task.m_list0[fieldId];
    ArrayU8x33 const & list1 = task.m_list1[fieldId];

    for( size_t i = 0; i < slice.size(); ++i)
    {
        slice[i].macroblock_address = task.m_SliceInfo[i].startMB;
        slice[i].num_macroblocks    = task.m_SliceInfo[i].numMB;
        slice[i].macroblock_info    = VA_INVALID_ID;

        for (ref = 0; ref < list0.Size(); ref++)
        {
            slice[i].RefPicList0[ref].frame_idx    = idx  = dpb[list0[ref] & 0x7f].m_frameIdx & 0x7f;
            slice[i].RefPicList0[ref].picture_id          = reconQueue[idx].surface;
            if (task.GetPicStructForEncode() != MFX_PICSTRUCT_PROGRESSIVE)
                slice[i].RefPicList0[ref].flags               = list0[ref] >> 7 ? VA_PICTURE_H264_BOTTOM_FIELD : VA_PICTURE_H264_TOP_FIELD;
        }
        for (; ref < 32; ref++)
        {
            slice[i].RefPicList0[ref].picture_id = VA_INVALID_ID;
            slice[i].RefPicList0[ref].flags      = VA_PICTURE_H264_INVALID;
        }

        for (ref = 0; ref < list1.Size(); ref++)
        {
            slice[i].RefPicList1[ref].frame_idx    = idx  = dpb[list1[ref] & 0x7f].m_frameIdx & 0x7f;
            slice[i].RefPicList1[ref].picture_id          = reconQueue[idx].surface;
            if (task.GetPicStructForEncode() != MFX_PICSTRUCT_PROGRESSIVE)
                slice[i].RefPicList1[ref].flags               = list1[ref] >> 7 ? VA_PICTURE_H264_BOTTOM_FIELD : VA_PICTURE_H264_TOP_FIELD;
        }
        for (; ref < 32; ref++)
        {
            slice[i].RefPicList1[ref].picture_id = VA_INVALID_ID;
            slice[i].RefPicList1[ref].flags      = VA_PICTURE_H264_INVALID;
        }

        slice[i].pic_parameter_set_id = pps.pic_parameter_set_id;
        slice[i].slice_type = ConvertMfxFrameType2VaapiSliceType( task.m_type[fieldId] );

        slice[i].direct_spatial_mv_pred_flag = 1;

        slice[i].num_ref_idx_l0_active_minus1 = mfxU8(MFX_MAX(1, task.m_list0[fieldId].Size()) - 1);
        slice[i].num_ref_idx_l1_active_minus1 = mfxU8(MFX_MAX(1, task.m_list1[fieldId].Size()) - 1);
        slice[i].num_ref_idx_active_override_flag   =
                    slice[i].num_ref_idx_l0_active_minus1 != pps.num_ref_idx_l0_active_minus1 ||
                    slice[i].num_ref_idx_l1_active_minus1 != pps.num_ref_idx_l1_active_minus1;

        slice[i].idr_pic_id = task.m_idrPicId;
        slice[i].pic_order_cnt_lsb = mfxU16(task.GetPoc(fieldId));

        slice[i].delta_pic_order_cnt_bottom         = 0;
        slice[i].delta_pic_order_cnt[0]             = 0;
        slice[i].delta_pic_order_cnt[1]             = 0;
        slice[i].luma_log2_weight_denom             = 0;
        slice[i].chroma_log2_weight_denom           = 0;

        slice[i].cabac_init_idc                     = extDdi ? (mfxU8)extDdi->CabacInitIdcPlus1 - 1 : 0;
        slice[i].slice_qp_delta                     = mfxI8(task.m_cqpValue[fieldId] - pps.pic_init_qp);

        slice[i].disable_deblocking_filter_idc = (extFeiSlice->Slice ? extFeiSlice->Slice[i].DisableDeblockingFilterIdc : extOpt2->DisableDeblockingIdc);
        slice[i].slice_alpha_c0_offset_div2    = (extFeiSlice->Slice ? extFeiSlice->Slice[i].SliceAlphaC0OffsetDiv2     : 0);
        slice[i].slice_beta_offset_div2        = (extFeiSlice->Slice ? extFeiSlice->Slice[i].SliceBetaOffsetDiv2        : 0);

        if (pPWT)
            FillPWT(hwCaps, pps, *pPWT, slice[i]);
    }

} // void UpdateSliceSizeLimited(...)


////////////////////////////////////////////////////////////////////////////////////////////
//                    HWEncoder based on VAAPI
////////////////////////////////////////////////////////////////////////////////////////////

using namespace MfxHwH264Encode;

VAAPIEncoder::VAAPIEncoder()
    : m_core(NULL)
    , m_videoParam()
    , m_vaDisplay()
    , m_vaContextEncode(VA_INVALID_ID)
    , m_vaConfig(VA_INVALID_ID)
    , m_sps()
    , m_pps()
    , m_slice()
    , m_spsBufferId(VA_INVALID_ID)
    , m_hrdBufferId(VA_INVALID_ID)
    , m_rateParamBufferId(VA_INVALID_ID)
    , m_frameRateId(VA_INVALID_ID)
    , m_qualityLevelId(VA_INVALID_ID)
    , m_maxFrameSizeId(VA_INVALID_ID)
    , m_quantizationId(VA_INVALID_ID)
    , m_rirId(VA_INVALID_ID)
    , m_qualityParamsId(VA_INVALID_ID)
    , m_miscParameterSkipBufferId(VA_INVALID_ID)
    , m_roiBufferId(VA_INVALID_ID)
    , m_ppsBufferId(VA_INVALID_ID)
    , m_mbqpBufferId(VA_INVALID_ID)
    , m_mbNoSkipBufferId(VA_INVALID_ID)
    , m_packedAudHeaderBufferId(VA_INVALID_ID)
    , m_packedAudBufferId(VA_INVALID_ID)
    , m_packedSpsHeaderBufferId(VA_INVALID_ID)
    , m_packedSpsBufferId(VA_INVALID_ID)
    , m_packedPpsHeaderBufferId(VA_INVALID_ID)
    , m_packedPpsBufferId(VA_INVALID_ID)
    , m_packedSeiHeaderBufferId(VA_INVALID_ID)
    , m_packedSeiBufferId(VA_INVALID_ID)
    , m_packedSkippedSliceHeaderBufferId(VA_INVALID_ID)
    , m_packedSkippedSliceBufferId(VA_INVALID_ID)
    , m_feedbackCache()
    , m_bsQueue()
    , m_reconQueue()
    , m_width()
    , m_height()
    , m_userMaxFrameSize()
    , m_mbbrc()
    , m_caps()
    , m_RIRState()
    , m_curTrellisQuantization()
    , m_newTrellisQuantization()
    , m_arrayVAEncROI()
    , m_guard()
    , m_headerPacker()
    , m_numSkipFrames()
    , m_sizeSkipFrames()
    , m_skipMode()
    , m_isENCPAK(false)
    , m_vaBrcPar()
    , m_vaFrameRate()
    , m_mbqp_buffer()
    , m_mb_noskip_buffer()
#ifdef MFX_ENABLE_MFE
    , m_mfe(0)
#endif
{
    m_videoParam.mfx.CodecProfile = MFX_PROFILE_AVC_HIGH; // QueryHwCaps will use this value

} // VAAPIEncoder::VAAPIEncoder(VideoCORE* core)


VAAPIEncoder::~VAAPIEncoder()
{
    Destroy();

} // VAAPIEncoder::~VAAPIEncoder()


void VAAPIEncoder::FillSps(
    MfxVideoParam const & par,
    VAEncSequenceParameterBufferH264 & sps)
{
        mfxExtSpsHeader const * extSps = GetExtBuffer(par);
        assert( extSps != 0 );
        if (!extSps)
            return;

        sps.picture_width_in_mbs  = par.mfx.FrameInfo.Width  >> 4;
        sps.picture_height_in_mbs = par.mfx.FrameInfo.Height >> 4;

        sps.level_idc    = par.mfx.CodecLevel;

        sps.intra_period = par.mfx.GopPicSize;
        sps.ip_period    = par.mfx.GopRefDist;

        sps.bits_per_second     = GetMaxBitrateValue(par.calcParam.maxKbps) << (6 + SCALE_FROM_DRIVER);

        sps.time_scale        = extSps->vui.timeScale;
        sps.num_units_in_tick = extSps->vui.numUnitsInTick;

        sps.max_num_ref_frames   =  mfxU8((extSps->maxNumRefFrames + 1) / 2);
        sps.seq_parameter_set_id = 0; // extSps->seqParameterSetId; - driver doesn't support non-zero sps_id

        sps.bit_depth_luma_minus8    = extSps->bitDepthLumaMinus8;
        sps.bit_depth_chroma_minus8  = extSps->bitDepthChromaMinus8;

        sps.seq_fields.bits.chroma_format_idc                 = extSps->chromaFormatIdc;
        sps.seq_fields.bits.log2_max_frame_num_minus4         = extSps->log2MaxFrameNumMinus4;
        sps.seq_fields.bits.pic_order_cnt_type                = extSps->picOrderCntType;
        sps.seq_fields.bits.log2_max_pic_order_cnt_lsb_minus4 = extSps->log2MaxPicOrderCntLsbMinus4;

        sps.num_ref_frames_in_pic_order_cnt_cycle   = extSps->numRefFramesInPicOrderCntCycle;
        sps.offset_for_non_ref_pic                  = extSps->offsetForNonRefPic;
        sps.offset_for_top_to_bottom_field          = extSps->offsetForTopToBottomField;

        Copy(sps.offset_for_ref_frame,  extSps->offsetForRefFrame);

        sps.frame_crop_left_offset                  = mfxU16(extSps->frameCropLeftOffset);
        sps.frame_crop_right_offset                 = mfxU16(extSps->frameCropRightOffset);
        sps.frame_crop_top_offset                   = mfxU16(extSps->frameCropTopOffset);
        sps.frame_crop_bottom_offset                = mfxU16(extSps->frameCropBottomOffset);

        sps.seq_fields.bits.seq_scaling_matrix_present_flag         = extSps->seqScalingMatrixPresentFlag;
        sps.seq_fields.bits.delta_pic_order_always_zero_flag        = extSps->deltaPicOrderAlwaysZeroFlag;
        sps.seq_fields.bits.frame_mbs_only_flag                     = extSps->frameMbsOnlyFlag;
        sps.seq_fields.bits.mb_adaptive_frame_field_flag            = extSps->mbAdaptiveFrameFieldFlag;
        sps.seq_fields.bits.direct_8x8_inference_flag               = extSps->direct8x8InferenceFlag;

        sps.vui_parameters_present_flag                    = extSps->vuiParametersPresentFlag;
        sps.vui_fields.bits.timing_info_present_flag       = extSps->vui.flags.timingInfoPresent;
        sps.vui_fields.bits.bitstream_restriction_flag     = extSps->vui.flags.bitstreamRestriction;
        sps.vui_fields.bits.log2_max_mv_length_horizontal  = extSps->vui.log2MaxMvLengthHorizontal;
        sps.vui_fields.bits.log2_max_mv_length_vertical    = extSps->vui.log2MaxMvLengthVertical;

        sps.frame_cropping_flag                            = extSps->frameCroppingFlag;

        sps.sar_height        = extSps->vui.sarHeight;
        sps.sar_width         = extSps->vui.sarWidth;
        sps.aspect_ratio_idc  = extSps->vui.aspectRatioIdc;

        mfxExtCodingOption2 const * extOpt2 = GetExtBuffer(par);
        assert( extOpt2 != 0 );
        m_newTrellisQuantization = extOpt2 ? extOpt2->Trellis : 0;

} // void FillSps(...)

VAConfigAttrib createVAConfigAttrib(VAConfigAttribType type, unsigned int value)
{
    VAConfigAttrib tmp = {type, value};
    return tmp;
}

mfxStatus VAAPIEncoder::CreateAuxilliaryDevice(
    VideoCORE* core,
    GUID guid,
    mfxU32 width,
    mfxU32 height,
    bool /*isTemporal*/)
{
    m_core = core;

    VAAPIVideoCORE * hwcore = dynamic_cast<VAAPIVideoCORE *>(m_core);
    MFX_CHECK_WITH_ASSERT(hwcore != 0, MFX_ERR_DEVICE_FAILED);
    if(hwcore)
    {
        mfxStatus mfxSts = hwcore->GetVAService(&m_vaDisplay);
        MFX_CHECK_STS(mfxSts);
    }

    m_width  = width;
    m_height = height;

    memset(&m_caps, 0, sizeof(m_caps));

    m_caps.BRCReset        = 1; // no bitrate resolution control
    m_caps.HeaderInsertion = 0; // we will provide headers (SPS, PPS) in binary format to the driver

    std::map<VAConfigAttribType, int> idx_map;
    VAConfigAttribType attr_types[] = {
        VAConfigAttribRTFormat,
        VAConfigAttribRateControl,
        VAConfigAttribEncQuantization,
        VAConfigAttribEncIntraRefresh,
        VAConfigAttribMaxPictureHeight,
        VAConfigAttribMaxPictureWidth,
        VAConfigAttribEncInterlaced,
        VAConfigAttribEncMaxRefFrames,
        VAConfigAttribEncSliceStructure,
        VAConfigAttribEncROI,
        VAConfigAttribEncSkipFrame
    };

    std::vector<VAConfigAttrib> attrs;
    attrs.reserve(sizeof(attr_types) / sizeof(attr_types[0]));

    for (size_t i=0; i < sizeof(attr_types)/sizeof(attr_types[0]); ++i)
    {
        attrs.push_back( createVAConfigAttrib(attr_types[i], 0) );
        idx_map[ attr_types[i] ] = i;
    }

    VAEntrypoint entrypoint = VAEntrypointEncSlice;

    if ((MSDK_Private_Guid_Encode_AVC_LowPower_Query == guid) ||
        (DXVA2_INTEL_LOWPOWERENCODE_AVC == guid))
    {
        entrypoint = VAEntrypointEncSliceLP;
    }

    VAStatus vaSts = vaGetConfigAttributes(m_vaDisplay,
                          ConvertProfileTypeMFX2VAAPI(m_videoParam.mfx.CodecProfile),
                          entrypoint,
                          Begin(attrs), attrs.size());
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    m_caps.VCMBitrateControl =
        (attrs[idx_map[VAConfigAttribRateControl]].value & VA_RC_VCM) ? 1 : 0; //Video conference mode
    m_caps.TrelisQuantization =
        (attrs[idx_map[VAConfigAttribEncQuantization]].value & (~VA_ATTRIB_NOT_SUPPORTED)) ? 1 : 0;
    m_caps.vaTrellisQuantization =
        attrs[idx_map[VAConfigAttribEncQuantization]].value;
    m_caps.RollingIntraRefresh =
        (attrs[idx_map[VAConfigAttribEncIntraRefresh]].value & (~VA_ATTRIB_NOT_SUPPORTED)) ? 1 : 0 ;
    m_caps.vaRollingIntraRefresh =
        attrs[idx_map[VAConfigAttribEncIntraRefresh]].value;
    m_caps.SkipFrame =
        (attrs[idx_map[VAConfigAttribEncSkipFrame]].value & (~VA_ATTRIB_NOT_SUPPORTED)) ? 1 : 0 ;

    m_caps.UserMaxFrameSizeSupport = 1;
    m_caps.MBBRCSupport            = 1;
    m_caps.MbQpDataSupport         = 1;
    m_caps.NoWeightedPred          = 0;
    m_caps.LumaWeightedPred        = 1;
    m_caps.ChromaWeightedPred      = 1;
    m_caps.MaxNum_WeightedPredL0   = 4;
    m_caps.MaxNum_WeightedPredL1   = 2;
    m_caps.Color420Only            = 1;

    if ((attrs[idx_map[VAConfigAttribMaxPictureWidth]].value != VA_ATTRIB_NOT_SUPPORTED) &&
        (attrs[idx_map[VAConfigAttribMaxPictureWidth]].value != 0))
        m_caps.MaxPicWidth  = attrs[idx_map[VAConfigAttribMaxPictureWidth]].value;
    else
        m_caps.MaxPicWidth = 1920;

    if ((attrs[idx_map[VAConfigAttribMaxPictureHeight]].value != VA_ATTRIB_NOT_SUPPORTED) &&
        (attrs[idx_map[VAConfigAttribMaxPictureHeight]].value != 0))
        m_caps.MaxPicHeight = attrs[idx_map[VAConfigAttribMaxPictureHeight]].value;
    else
        m_caps.MaxPicHeight = 1088;

    if (attrs[idx_map[VAConfigAttribEncSliceStructure]].value != VA_ATTRIB_NOT_SUPPORTED)
    {
        m_caps.SliceStructure =
            ConvertSliceStructureVAAPIToMFX(attrs[idx_map[VAConfigAttribEncSliceStructure]].value);
    }
    else
    {
        const eMFXHWType hwtype = m_core->GetHWType();
        m_caps.SliceStructure = (hwtype != MFX_HW_VLV && hwtype >= MFX_HW_HSW) ? 4 : 1; // 1 - SliceDividerSnb; 2 - SliceDividerHsw;
    }                                                                                   // 3 - SliceDividerBluRay; 4 - arbitrary slice size in MBs; the other - SliceDividerOneSlice

    if (attrs[idx_map[VAConfigAttribEncInterlaced]].value != VA_ATTRIB_NOT_SUPPORTED)
        m_caps.NoInterlacedField = attrs[idx_map[VAConfigAttribEncInterlaced]].value;
    else
        m_caps.NoInterlacedField = 0;

    if (attrs[idx_map[VAConfigAttribEncMaxRefFrames]].value != VA_ATTRIB_NOT_SUPPORTED)
    {
        m_caps.MaxNum_Reference  =  attrs[idx_map[VAConfigAttribEncMaxRefFrames]].value & 0xffff;
        m_caps.MaxNum_Reference1 = (attrs[idx_map[VAConfigAttribEncMaxRefFrames]].value >>16) & 0xffff;
    }
    else
    {
        m_caps.MaxNum_Reference  = 1;
        m_caps.MaxNum_Reference1 = 1;
    }

#if defined(LINUX_TARGET_PLATFORM_BXTMIN) || defined(LINUX_TARGET_PLATFORM_BXT) || defined (LINUX_TARGET_PLATFORM_CFL)
    // Officially only APL and CFL supports ROI.

    if (attrs[idx_map[VAConfigAttribEncROI]].value != VA_ATTRIB_NOT_SUPPORTED)
    {
        VAConfigAttribValEncROI *VaEncROIValPtr = reinterpret_cast<VAConfigAttribValEncROI *>(&attrs[idx_map[VAConfigAttribEncROI]].value);
        assert(VaEncROIValPtr->bits.num_roi_regions < 32);

        m_caps.MaxNumOfROI                = VaEncROIValPtr->bits.num_roi_regions;
        m_caps.ROIBRCPriorityLevelSupport = VaEncROIValPtr->bits.roi_rc_priority_support;
        m_caps.ROIBRCDeltaQPLevelSupport  = VaEncROIValPtr->bits.roi_rc_qp_delta_support;
    }
    else
#endif
    {
        m_caps.MaxNumOfROI = 0;
    }

    return MFX_ERR_NONE;
} // mfxStatus VAAPIEncoder::CreateAuxilliaryDevice(VideoCORE* core, GUID guid, mfxU32 width, mfxU32 height)


mfxStatus VAAPIEncoder::CreateAccelerationService(MfxVideoParam const & par)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VAAPIEncoder::CreateAccelerationService");
    if(IsMvcProfile(par.mfx.CodecProfile))
        return MFX_WRN_PARTIAL_ACCELERATION;

    if(0 == m_reconQueue.size())
    {
    /* We need to pass reconstructed surfaces wheh call vaCreateContext().
     * Here we don't have this info.
     */
        m_videoParam = par;
        return MFX_ERR_NONE;
    }

    MFX_CHECK(m_vaDisplay, MFX_ERR_DEVICE_FAILED);
    VAStatus vaSts;

    mfxI32 entrypointsIndx = 0;
    mfxI32 numEntrypoints = vaMaxNumEntrypoints(m_vaDisplay);
    MFX_CHECK(numEntrypoints, MFX_ERR_DEVICE_FAILED);

    std::vector<VAEntrypoint> pEntrypoints(numEntrypoints);

    vaSts = vaQueryConfigEntrypoints(
                m_vaDisplay,
                ConvertProfileTypeMFX2VAAPI(par.mfx.CodecProfile),
                Begin(pEntrypoints),
                &numEntrypoints);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);


#if defined(MFX_ENABLE_H264_VIDEO_FEI_ENCPAK) || defined(MFX_ENABLE_H264_VIDEO_FEI_PREENC)
    //first check for ENCPAK support
    //check for ext buffer for FEI
    if (par.NumExtParam > 0)
    {
        const mfxExtFeiParam* params = GetExtBuffer(par);

        if (params->Func == MFX_FEI_FUNCTION_ENCODE)
        {
            for( entrypointsIndx = 0; entrypointsIndx < numEntrypoints; entrypointsIndx++ )
            {
                if (VAEntrypointFEI == pEntrypoints[entrypointsIndx])
                {
                    m_isENCPAK = true;
                    break;
                }
            }

            if(!m_isENCPAK) return MFX_ERR_UNSUPPORTED;
        }
    }
#endif

    VAEntrypoint entryPoint = IsOn(par.mfx.LowPower) ? VAEntrypointEncSliceLP : VAEntrypointEncSlice;
    if( !m_isENCPAK )
    {
        bool bEncodeEnable = false;
        for( entrypointsIndx = 0; entrypointsIndx < numEntrypoints; entrypointsIndx++ )
        {
            if( entryPoint == pEntrypoints[entrypointsIndx] )
            {
                bEncodeEnable = true;
                break;
            }
        }
        if( !bEncodeEnable )
        {
            return MFX_ERR_DEVICE_FAILED;
        }
    }
#if defined(MFX_ENABLE_H264_VIDEO_FEI_ENCPAK) || defined(MFX_ENABLE_H264_VIDEO_FEI_PREENC)
    else
    {
        entryPoint = (VAEntrypoint)VAEntrypointFEI;
    }
#endif

    // Configuration
    VAConfigAttrib attrib[3];
    mfxI32 numAttrib = 0;
    mfxU32 flag = VA_PROGRESSIVE;

    attrib[0].type = VAConfigAttribRTFormat;
    attrib[1].type = VAConfigAttribRateControl;
    numAttrib += 2;

#if defined(MFX_ENABLE_H264_VIDEO_FEI_ENCPAK) || defined(MFX_ENABLE_H264_VIDEO_FEI_PREENC)
    if ( m_isENCPAK )
    {
        attrib[2].type = (VAConfigAttribType)VAConfigAttribFEIFunctionType;
        numAttrib++;
    }
#endif

    vaSts = vaGetConfigAttributes(m_vaDisplay,
                          ConvertProfileTypeMFX2VAAPI(par.mfx.CodecProfile),
                          entryPoint,
                          &attrib[0], numAttrib);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    if ((attrib[0].value & VA_RT_FORMAT_YUV420) == 0)
        return MFX_ERR_DEVICE_FAILED;

    mfxU8 vaRCType = ConvertRateControlMFX2VAAPI(par.mfx.RateControlMethod);

    mfxExtCodingOption2 const *extOpt2 = GetExtBuffer(par);
    if( NULL == extOpt2 )
    {
        assert( extOpt2 );
        return (MFX_ERR_UNKNOWN);
    }
    m_mbbrc = IsOn(extOpt2->MBBRC) ? 1 : IsOff(extOpt2->MBBRC) ? 2 : 0;
    m_skipMode = extOpt2->SkipFrame;

    if ((attrib[1].value & vaRCType) == 0)
        return MFX_ERR_DEVICE_FAILED;

#if defined(MFX_ENABLE_H264_VIDEO_FEI_ENCPAK) || defined(MFX_ENABLE_H264_VIDEO_FEI_PREENC)
    if(m_isENCPAK)
    {
        //check function
        if (!(attrib[2].value & VA_FEI_FUNCTION_ENC_PAK))
        {
            return MFX_ERR_DEVICE_FAILED;
        }
        attrib[2].value = VA_FEI_FUNCTION_ENC_PAK;
    }
#endif

    attrib[0].value = VA_RT_FORMAT_YUV420;
    attrib[1].value = vaRCType;

    vaSts = vaCreateConfig(
        m_vaDisplay,
        ConvertProfileTypeMFX2VAAPI(par.mfx.CodecProfile),
        entryPoint,
        attrib,
        numAttrib,
        &m_vaConfig);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    std::vector<VASurfaceID> reconSurf;
    for(unsigned int i = 0; i < m_reconQueue.size(); i++)
        reconSurf.push_back(m_reconQueue[i].surface);

    // Encoder create

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaCreateContext");
        vaSts = vaCreateContext(
            m_vaDisplay,
            m_vaConfig,
            m_width,
            m_height,
            flag,
            Begin(reconSurf),
            reconSurf.size(),
            &m_vaContextEncode);
    }
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

#ifdef MFX_ENABLE_MFE
    const mfxExtMultiFrameParam & mfeParam = GetExtBufferRef(par);

    if (mfeParam.MaxNumFrames > 1)
    {
        m_mfe = CreatePlatformMFEEncoder(m_core);
        if (0 == m_mfe)
            return MFX_ERR_DEVICE_FAILED;

        mfxStatus sts = m_mfe->Create(mfeParam, m_vaDisplay);
        MFX_CHECK_STS(sts);
        //progressive to be changed for particular 2 field cases
        sts = m_mfe->Join(m_vaContextEncode,par.mfx.FrameInfo.PicStruct!=MFX_PICSTRUCT_PROGRESSIVE);
        MFX_CHECK_STS(sts);
    }
#endif

    mfxU16 maxNumSlices = GetMaxNumSlices(par);

    m_slice.resize(maxNumSlices);
    m_sliceBufferId.resize(maxNumSlices);
    m_packeSliceHeaderBufferId.resize(maxNumSlices);
    m_packedSliceBufferId.resize(maxNumSlices);

    std::fill(m_sliceBufferId.begin(),            m_sliceBufferId.end(),            VA_INVALID_ID);
    std::fill(m_packeSliceHeaderBufferId.begin(), m_packeSliceHeaderBufferId.end(), VA_INVALID_ID);
    std::fill(m_packedSliceBufferId.begin(),      m_packedSliceBufferId.end(),      VA_INVALID_ID);

#if defined(MFX_ENABLE_H264_VIDEO_FEI_ENCPAK) || defined(MFX_ENABLE_H264_VIDEO_FEI_PREENC)
    if (m_isENCPAK)
    {
        mfxU32 numBuffers = m_reconQueue.size();
        if (MFX_PICSTRUCT_PROGRESSIVE != m_videoParam.mfx.FrameInfo.PicStruct)
            numBuffers = 2*numBuffers;

        m_vaFeiMBStatId.resize(numBuffers);
        m_vaFeiMVOutId.resize(numBuffers);
        m_vaFeiMCODEOutId.resize(numBuffers);

        std::fill(m_vaFeiMBStatId.begin(),   m_vaFeiMBStatId.end(),   VA_INVALID_ID);
        std::fill(m_vaFeiMVOutId.begin(),    m_vaFeiMVOutId.end(),    VA_INVALID_ID);
        std::fill(m_vaFeiMCODEOutId.begin(), m_vaFeiMCODEOutId.end(), VA_INVALID_ID);

        m_vaFeiMBStatBufSize.resize(numBuffers);
        m_vaFeiMVOutBufSize.resize(numBuffers);
        m_vaFeiMCODEOutBufSize.resize(numBuffers);

        std::fill(m_vaFeiMBStatBufSize.begin(),   m_vaFeiMBStatBufSize.end(),   0);
        std::fill(m_vaFeiMVOutBufSize.begin(),    m_vaFeiMVOutBufSize.end(),    0);
        std::fill(m_vaFeiMCODEOutBufSize.begin(), m_vaFeiMCODEOutBufSize.end(), 0);
    }
#endif

    Zero(m_sps);
    Zero(m_pps);
    Zero(m_slice);
    //------------------------------------------------------------------

    FillSps(par, m_sps);
    FillBrcStructures(par, m_vaBrcPar, m_vaFrameRate);

    MFX_CHECK_WITH_ASSERT(MFX_ERR_NONE == SetHRD(par, m_vaDisplay, m_vaContextEncode, m_hrdBufferId),                              MFX_ERR_DEVICE_FAILED);
    MFX_CHECK_WITH_ASSERT(MFX_ERR_NONE == SetRateControl(par, m_mbbrc, 0, 0, m_vaDisplay, m_vaContextEncode, m_rateParamBufferId), MFX_ERR_DEVICE_FAILED);
    MFX_CHECK_WITH_ASSERT(MFX_ERR_NONE == SetFrameRate(par, m_vaDisplay, m_vaContextEncode, m_frameRateId),                        MFX_ERR_DEVICE_FAILED);
    MFX_CHECK_WITH_ASSERT(MFX_ERR_NONE == SetQualityLevel(par, m_vaDisplay, m_vaContextEncode, m_qualityLevelId),            MFX_ERR_DEVICE_FAILED);
    MFX_CHECK_WITH_ASSERT(MFX_ERR_NONE == SetQualityParams(par, m_vaDisplay, m_vaContextEncode, m_qualityParamsId),                MFX_ERR_DEVICE_FAILED);


    FillConstPartOfPps(par, m_pps);

    if (m_caps.HeaderInsertion == 0)
    {
        m_headerPacker.Init(par, m_caps);

        if (m_headerPacker.isSvcPrefixUsed())
        {
            m_packedSvcPrefixHeaderBufferId.resize(maxNumSlices, VA_INVALID_ID);
            m_packedSvcPrefixBufferId.resize(maxNumSlices, VA_INVALID_ID);
        }
    }

    mfxExtCodingOption3 const *extOpt3 = GetExtBuffer(par);

    if (extOpt3)
    {
        if (IsOn(extOpt3->EnableMBQP))
            m_mbqp_buffer.resize(((m_width / 16 + 63) & ~63) * ((m_height / 16 + 7) & ~7));

        if (IsOn(extOpt3->MBDisableSkipMap))
            m_mb_noskip_buffer.resize(((m_width / 16 + 63) & ~63) * ((m_height / 16 + 7) & ~7));
    }

    return MFX_ERR_NONE;

} // mfxStatus VAAPIEncoder::CreateAccelerationService(MfxVideoParam const & par)


mfxStatus VAAPIEncoder::Reset(MfxVideoParam const & par)
{
//    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_SCHED, "Enc Reset");

    m_videoParam = par;
    mfxExtCodingOption2 const *extOpt2 = GetExtBuffer(par);
    mfxExtCodingOption3 const *extOpt3 = GetExtBuffer(par);
    if( NULL == extOpt2 )
    {
        assert( extOpt2 );
        return (MFX_ERR_UNKNOWN);
    }
    m_mbbrc = IsOn(extOpt2->MBBRC) ? 1 : IsOff(extOpt2->MBBRC) ? 2 : 0;
    m_skipMode = extOpt2->SkipFrame;

    FillSps(par, m_sps);
    VAEncMiscParameterRateControl oldBrcPar = m_vaBrcPar;
    VAEncMiscParameterFrameRate oldFrameRate = m_vaFrameRate;
    FillBrcStructures(par, m_vaBrcPar, m_vaFrameRate);
    bool isBrcResetRequired =
           !Equal(m_vaBrcPar, oldBrcPar)
        || !Equal(m_vaFrameRate, oldFrameRate)
        || m_userMaxFrameSize != extOpt2->MaxFrameSize;

    MFX_CHECK_WITH_ASSERT(MFX_ERR_NONE == SetHRD(par, m_vaDisplay, m_vaContextEncode, m_hrdBufferId),                                                  MFX_ERR_DEVICE_FAILED);
    MFX_CHECK_WITH_ASSERT(MFX_ERR_NONE == SetRateControl(par, m_mbbrc, 0, 0, m_vaDisplay, m_vaContextEncode, m_rateParamBufferId, isBrcResetRequired), MFX_ERR_DEVICE_FAILED);
    MFX_CHECK_WITH_ASSERT(MFX_ERR_NONE == SetFrameRate(par, m_vaDisplay, m_vaContextEncode, m_frameRateId),                                            MFX_ERR_DEVICE_FAILED);
    MFX_CHECK_WITH_ASSERT(MFX_ERR_NONE == SetQualityLevel(par, m_vaDisplay, m_vaContextEncode, m_qualityLevelId),                                MFX_ERR_DEVICE_FAILED);
    MFX_CHECK_WITH_ASSERT(MFX_ERR_NONE == SetQualityParams(par, m_vaDisplay, m_vaContextEncode, m_qualityParamsId),                                    MFX_ERR_DEVICE_FAILED);

    FillConstPartOfPps(par, m_pps);

    if (m_caps.HeaderInsertion == 0)
        m_headerPacker.Init(par, m_caps);

    if (extOpt3)
    {
        if (IsOn(extOpt3->EnableMBQP))
            m_mbqp_buffer.resize(((m_width / 16 + 63) & ~63) * ((m_height / 16 + 7) & ~7));

        if (IsOn(extOpt3->MBDisableSkipMap))
            m_mb_noskip_buffer.resize(((m_width / 16 + 63) & ~63) * ((m_height / 16 + 7) & ~7));
    }
    /* Destroy existing FEI buffers
     * For next Execute() call new buffer sets will re-allocated */
    if (m_isENCPAK)
    {
        for( mfxU32 i = 0; i < m_vaFeiMBStatId.size(); i++ )
            MFX_DESTROY_VABUFFER(m_vaFeiMBStatId[i], m_vaDisplay);

        for( mfxU32 i = 0; i < m_vaFeiMVOutId.size(); i++ )
            MFX_DESTROY_VABUFFER(m_vaFeiMVOutId[i], m_vaDisplay);

        for( mfxU32 i = 0; i < m_vaFeiMCODEOutId.size(); i++ )
            MFX_DESTROY_VABUFFER(m_vaFeiMCODEOutId[i], m_vaDisplay);

        std::fill(m_vaFeiMBStatBufSize.begin(),   m_vaFeiMBStatBufSize.end(),   0);
        std::fill(m_vaFeiMVOutBufSize.begin(),    m_vaFeiMVOutBufSize.end(),    0);
        std::fill(m_vaFeiMCODEOutBufSize.begin(), m_vaFeiMCODEOutBufSize.end(), 0);
    }

    return MFX_ERR_NONE;

} // mfxStatus VAAPIEncoder::Reset(MfxVideoParam const & par)


mfxStatus VAAPIEncoder::QueryCompBufferInfo(D3DDDIFORMAT type, mfxFrameAllocRequest& request)
{
    type;

    // request linear buffer
    request.Info.FourCC = MFX_FOURCC_P8;

    // context_id required for allocation video memory (tmp solution)
    request.AllocId = m_vaContextEncode;

    return MFX_ERR_NONE;

} // mfxStatus VAAPIEncoder::QueryCompBufferInfo(D3DDDIFORMAT type, mfxFrameAllocRequest& request)


mfxStatus VAAPIEncoder::QueryEncodeCaps(ENCODE_CAPS& caps)
{

    caps = m_caps;

    return MFX_ERR_NONE;

} // mfxStatus VAAPIEncoder::QueryEncodeCaps(ENCODE_CAPS& caps)

mfxStatus VAAPIEncoder::QueryMbPerSec(mfxVideoParam const & par, mfxU32 (&mbPerSec)[16])
{
    VAConfigID config = VA_INVALID_ID;

    VAConfigAttrib attrib[2];
    attrib[0].type = VAConfigAttribRTFormat;
    attrib[0].value = VA_RT_FORMAT_YUV420;
    attrib[1].type = VAConfigAttribRateControl;
    attrib[1].value = ConvertRateControlMFX2VAAPI(par.mfx.RateControlMethod);

    VAStatus vaSts = vaCreateConfig(
        m_vaDisplay,
        ConvertProfileTypeMFX2VAAPI(par.mfx.CodecProfile),
        IsOn(par.mfx.LowPower) ? VAEntrypointEncSliceLP : VAEntrypointEncSlice,
        attrib,
        2,
        &config);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    VAProcessingRateParameter proc_rate_buf = { };
    mfxU32 & processing_rate = mbPerSec[0];

    proc_rate_buf.proc_buf_enc.level_idc = par.mfx.CodecLevel ? par.mfx.CodecLevel : 0xff;
    proc_rate_buf.proc_buf_enc.quality_level = par.mfx.TargetUsage ? par.mfx.TargetUsage : 0xffff;
    proc_rate_buf.proc_buf_enc.intra_period = par.mfx.GopPicSize ? par.mfx.GopPicSize : 0xffff;
    proc_rate_buf.proc_buf_enc.ip_period = par.mfx.GopRefDist ? par.mfx.GopRefDist : 0xffff;

    vaSts = vaQueryProcessingRate(m_vaDisplay, config, &proc_rate_buf, &processing_rate);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    vaDestroyConfig(m_vaDisplay, config);

    return MFX_ERR_NONE;
}

mfxStatus VAAPIEncoder::QueryInputTilingSupport(mfxVideoParam const & par, mfxU32 &inputTiling)
{
    VAConfigID config = VA_INVALID_ID;
    VAEntrypoint targetEntrypoint = IsOn(par.mfx.LowPower) ? VAEntrypointEncSliceLP : VAEntrypointEncSlice;

    VAConfigAttrib attrib[2];
    attrib[0].type = VAConfigAttribRTFormat;
    attrib[0].value = VA_RT_FORMAT_YUV420;
    attrib[1].type = VAConfigAttribRateControl;
    attrib[1].value = ConvertRateControlMFX2VAAPI(par.mfx.RateControlMethod);

    VAStatus vaSts = vaCreateConfig(
        m_vaDisplay,
        ConvertProfileTypeMFX2VAAPI(par.mfx.CodecProfile),
        targetEntrypoint,
        attrib,
        2,
        &config);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    VAProfile    profile;
    VAEntrypoint entrypoint;
    mfxI32       numAttribs, maxNumAttribs;
    numAttribs = maxNumAttribs = vaMaxNumConfigAttributes(m_vaDisplay);

    std::vector<VAConfigAttrib> attrs;
    attrs.resize(maxNumAttribs);

    vaSts = vaQueryConfigAttributes(m_vaDisplay, config, &profile, &entrypoint, Begin(attrs), &numAttribs);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    MFX_CHECK_WITH_ASSERT((mfxU32)numAttribs < static_cast<mfxU32>(maxNumAttribs), MFX_ERR_UNDEFINED_BEHAVIOR);

    if (entrypoint == targetEntrypoint)
    {
        for(mfxI32 i=0; i<numAttribs; i++)
        {
            if (attrs[i].type == VAConfigAttribInputTiling)
                inputTiling = attrs[i].value;
        }
    }

    vaDestroyConfig(m_vaDisplay, config);

    return MFX_ERR_NONE;
}

mfxStatus VAAPIEncoder::QueryHWGUID(VideoCORE * core, GUID guid, bool isTemporal)
{
    core;
    guid;
    isTemporal;

    return MFX_ERR_UNSUPPORTED;
}

mfxStatus VAAPIEncoder::Register(mfxFrameAllocResponse& response, D3DDDIFORMAT type)
{
    std::vector<ExtVASurface> * pQueue;
    mfxStatus sts;

    if( D3DDDIFMT_INTELENCODE_BITSTREAMDATA == type )
    {
        pQueue = &m_bsQueue;
    }
    else
    {
        pQueue = &m_reconQueue;
    }

    {
        // we should register allocated HW bitstreams and recon surfaces
        MFX_CHECK( response.mids, MFX_ERR_NULL_PTR );

        ExtVASurface extSurf;
        VASurfaceID *pSurface = NULL;

        for (mfxU32 i = 0; i < response.NumFrameActual; i++)
        {

            sts = m_core->GetFrameHDL(response.mids[i], (mfxHDL *)&pSurface);
            MFX_CHECK_STS(sts);

            extSurf.number  = i;
            extSurf.surface = *pSurface;

            pQueue->push_back( extSurf );
        }
    }
    if( D3DDDIFMT_INTELENCODE_BITSTREAMDATA != type )
    {
        sts = CreateAccelerationService(m_videoParam);
        MFX_CHECK_STS(sts);
    }

    return MFX_ERR_NONE;

} // mfxStatus VAAPIEncoder::Register(mfxFrameAllocResponse& response, D3DDDIFORMAT type)


mfxStatus VAAPIEncoder::Register(mfxMemId memId, D3DDDIFORMAT type)
{
    memId;
    type;

    return MFX_ERR_UNSUPPORTED;

} // mfxStatus VAAPIEncoder::Register(mfxMemId memId, D3DDDIFORMAT type)


bool operator==(const ENCODE_ENC_CTRL_CAPS& l, const ENCODE_ENC_CTRL_CAPS& r)
{
    return memcmp(&l, &r, sizeof(ENCODE_ENC_CTRL_CAPS)) == 0;
}

bool operator!=(const ENCODE_ENC_CTRL_CAPS& l, const ENCODE_ENC_CTRL_CAPS& r)
{
    return !(l == r);
}
//static int debug_frame_bum = 0;

mfxStatus VAAPIEncoder::Execute(
    mfxHDLPair      pair,
    DdiTask const & task,
    mfxU32          fieldId,
    PreAllocatedVector const & sei)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VAAPIEncoder::Execute");

    mfxHDL surface = pair.first;
    VAEncPackedHeaderParameterBuffer packed_header_param_buffer;
    VASurfaceID reconSurface;
    VASurfaceID *inputSurface = (VASurfaceID*)surface;
    VABufferID  codedBuffer;
    std::vector<VABufferID> configBuffers;
    std::vector<mfxU32> packedBufferIndexes;
    mfxU32      i;
    mfxU16      buffersCount = 0;
    mfxU32      packedDataSize = 0;
    VAStatus    vaSts;
    mfxU8 skipFlag  = task.SkipFlag();
    mfxU16 skipMode = m_skipMode;
    mfxExtCodingOption2     const * ctrlOpt2      = GetExtBuffer(task.m_ctrl);
    mfxExtCodingOption3     const * ctrlOpt3      = GetExtBuffer(task.m_ctrl);
    mfxExtMBDisableSkipMap  const * ctrlNoSkipMap = GetExtBuffer(task.m_ctrl);
    mfxExtCodingOption      const*  extOpt        = GetExtBuffer(m_videoParam);
    mfxU8 qp_delta_list[] = {0,0,0,0, 0,0,0,0};

    if (ctrlOpt2 && ctrlOpt2->SkipFrame <= MFX_SKIPFRAME_BRC_ONLY)
        skipMode = ctrlOpt2->SkipFrame;

    if (skipMode == MFX_SKIPFRAME_BRC_ONLY)
    {
        skipFlag = 0; // encode current frame as normal
        m_numSkipFrames += (mfxU8)task.m_ctrl.SkipFrame;
    }

    configBuffers.resize(MAX_CONFIG_BUFFERS_COUNT + m_slice.size() * 2 + m_packedSvcPrefixBufferId.size() * 2);

    // update params
    {
        size_t slice_size_old = m_slice.size();
        //Destroy old buffers
        for(size_t i=0; i<slice_size_old; i++)
        {
            MFX_DESTROY_VABUFFER(m_sliceBufferId[i],            m_vaDisplay);
            MFX_DESTROY_VABUFFER(m_packeSliceHeaderBufferId[i], m_vaDisplay);
            MFX_DESTROY_VABUFFER(m_packedSliceBufferId[i],      m_vaDisplay);
        }

        for(size_t i=0; i<m_packedSvcPrefixBufferId.size(); i++)
        {
            MFX_DESTROY_VABUFFER(m_packedSvcPrefixHeaderBufferId[i], m_vaDisplay);
            MFX_DESTROY_VABUFFER(m_packedSvcPrefixBufferId[i],       m_vaDisplay);
        }

        UpdatePPS(task, fieldId, m_pps, m_reconQueue);

        if (task.m_SliceInfo.size())
            UpdateSliceSizeLimited(m_caps, task, fieldId, m_sps, m_pps, m_slice, m_videoParam, m_reconQueue);
        else
            UpdateSlice(m_caps, task, fieldId, m_sps, m_pps, m_slice, m_videoParam, m_reconQueue);

        if (slice_size_old != m_slice.size())
        {
            m_sliceBufferId.resize(m_slice.size());
            m_packeSliceHeaderBufferId.resize(m_slice.size());
            m_packedSliceBufferId.resize(m_slice.size());

            if (m_headerPacker.isSvcPrefixUsed())
            {
                m_packedSvcPrefixHeaderBufferId.resize(m_slice.size());
                m_packedSvcPrefixBufferId.resize(m_slice.size());
            }
        }

        configBuffers.resize(MAX_CONFIG_BUFFERS_COUNT + m_slice.size() * 2 + m_packedSvcPrefixBufferId.size() * 2);
    }
    /* for debug only */
    //fprintf(stderr, "----> Encoding frame = %u, type = %u\n", debug_frame_bum++, ConvertMfxFrameType2SliceType( task.m_type[fieldId]) -5 );

    //------------------------------------------------------------------
    // find bitstream
    mfxU32 idxBs = task.m_idxBs[fieldId];
    if( idxBs < m_bsQueue.size() )
    {
        codedBuffer = m_bsQueue[idxBs].surface;
    }
    else
    {
        return MFX_ERR_UNKNOWN;
    }

    // find reconstructed surface
    mfxU32 idxRecon = task.m_idxRecon;
    if( idxRecon < m_reconQueue.size())
    {
        reconSurface = m_reconQueue[ idxRecon ].surface;
    }
    else
    {
        return MFX_ERR_UNKNOWN;
    }

    m_pps.coded_buf = codedBuffer;
    m_pps.CurrPic.picture_id = reconSurface;

#if defined(MFX_ENABLE_H264_VIDEO_FEI_ENCPAK) || defined(MFX_ENABLE_H264_VIDEO_FEI_PREENC)

    VABufferID vaFeiFrameControlId = VA_INVALID_ID;
    VABufferID vaFeiMVPredId       = VA_INVALID_ID;
    VABufferID vaFeiMBControlId    = VA_INVALID_ID;
    VABufferID vaFeiMBQPId         = VA_INVALID_ID;

    // Parity to order conversion (FEI ext buffers attached by field order, but MSDK operates in terms of fields parity)
    mfxU32 feiFieldId = task.m_fid[fieldId];

    if (m_isENCPAK)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "FEI config");

        // Multiply by 2 for interlaced
        if (MFX_PICSTRUCT_PROGRESSIVE != m_videoParam.mfx.FrameInfo.PicStruct)
            idxRecon *= 2;

        // In case of single-field processing, only one buffer is attached
        mfxU32 idxToPickBuffer = task.m_singleFieldMode ? 0 : feiFieldId;

        //find ext buffers
        mfxExtFeiEncMBCtrl       * mbctrl      = GetExtBuffer(task.m_ctrl, idxToPickBuffer);
        mfxExtFeiEncMVPredictors * mvpred      = GetExtBuffer(task.m_ctrl, idxToPickBuffer);
        mfxExtFeiEncFrameCtrl    * frameCtrl   = GetExtBuffer(task.m_ctrl, idxToPickBuffer);
        mfxExtFeiEncQP           * mbqp        = GetExtBuffer(task.m_ctrl, idxToPickBuffer);
        mfxExtFeiRepackCtrl      * rePakCtrl   = GetExtBuffer(task.m_ctrl, idxToPickBuffer);
        mfxExtFeiSliceHeader     * extFeiSlice = GetExtBuffer(task.m_ctrl, idxToPickBuffer);

        /* Output buffers passed via mfxBitstream structure*/
        mfxExtFeiEncMBStat * mbstat    = NULL;
        mfxExtFeiEncMV     * mvout     = NULL;
        mfxExtFeiPakMBCtrl * mbcodeout = NULL;

        if (NULL != task.m_bs)
        {
            mbstat    = GetExtBufferFEI(task.m_bs, idxToPickBuffer);
            mvout     = GetExtBufferFEI(task.m_bs, idxToPickBuffer);
            mbcodeout = GetExtBufferFEI(task.m_bs, idxToPickBuffer);
        }

        if (frameCtrl->MVPredictor && mvpred != NULL)
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaCreateBuffer (MVP)");
            vaSts = vaCreateBuffer(m_vaDisplay,
                    m_vaContextEncode,
                    (VABufferType)VAEncFEIMVPredictorBufferType,
                    sizeof(VAEncFEIMVPredictorH264)*mvpred->NumMBAlloc,
                    1, //limitation from driver, num elements should be 1
                    mvpred->MB,
                    &vaFeiMVPredId);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        }

        if (frameCtrl->PerMBInput && mbctrl != NULL)
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaCreateBuffer (MBctrl)");
            vaSts = vaCreateBuffer(m_vaDisplay,
                    m_vaContextEncode,
                    (VABufferType)VAEncFEIMBControlBufferType,
                    sizeof(VAEncFEIMBControlH264)*mbctrl->NumMBAlloc,
                    1, //limitation from driver, num elements should be 1
                    mbctrl->MB,
                    &vaFeiMBControlId);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        }

        if (frameCtrl->PerMBQp && mbqp != NULL)
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaCreateBuffer (MBqp)");
#if MFX_VERSION >= 1023
            vaSts = vaCreateBuffer(m_vaDisplay,
                    m_vaContextEncode,
                    (VABufferType)VAEncQPBufferType,
                    sizeof(VAEncQPBufferH264)*mbqp->NumMBAlloc,
                    1, //limitation from driver, num elements should be 1
                    mbqp->MB,
                    &vaFeiMBQPId);
#else
            vaSts = vaCreateBuffer(m_vaDisplay,
                    m_vaContextEncode,
                    (VABufferType)VAEncQPBufferType,
                    sizeof (VAEncQPBufferH264)*mbqp->NumQPAlloc,
                    1, //limitation from driver, num elements should be 1
                    mbqp->QP,
                    &vaFeiMBQPId);
#endif
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        }

        // Override deblocking parameters from init with runtime parameters, if provided
        if (NULL != extFeiSlice)
        {
            for( size_t i = 0; i < m_slice.size(); ++i)
            {
                m_slice[i].disable_deblocking_filter_idc = extFeiSlice->Slice[i].DisableDeblockingFilterIdc;
                m_slice[i].slice_alpha_c0_offset_div2    = extFeiSlice->Slice[i].SliceAlphaC0OffsetDiv2;
                m_slice[i].slice_beta_offset_div2        = extFeiSlice->Slice[i].SliceBetaOffsetDiv2;
            }
        }

        //output buffer for MB distortions
        if (mbstat != NULL)
        {
            mfxU32 vaFeiMBStatBufSize = sizeof(VAEncFEIDistortionH264) * mbstat->NumMBAlloc;
            if (VA_INVALID_ID == m_vaFeiMBStatId[idxRecon + feiFieldId] ||
                vaFeiMBStatBufSize > m_vaFeiMBStatBufSize[idxRecon + feiFieldId])
            {
                MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "MBstat");
                MFX_DESTROY_VABUFFER(m_vaFeiMBStatId[idxRecon + feiFieldId], m_vaDisplay);
                m_vaFeiMBStatBufSize[idxRecon + feiFieldId] = 0;
                {
                    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaCreateBuffer");
                    vaSts = vaCreateBuffer(m_vaDisplay,
                            m_vaContextEncode,
                            (VABufferType)VAEncFEIDistortionBufferType,
                            vaFeiMBStatBufSize,
                            1, //limitation from driver, num elements should be 1
                            NULL, //should be mapped later
                            &m_vaFeiMBStatId[idxRecon + feiFieldId]);
                }
                MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
                //mdprintf(stderr, "MB Stat bufId=%d\n", vaFeiMBStatId);
                m_vaFeiMBStatBufSize[idxRecon + feiFieldId] = vaFeiMBStatBufSize;
            }
        }

        /* MbCode and MVout buffer always should be in pair
         * If one from buffers does not requested by application but one of them requested
         * library have to create both buffers sets. And for this need to know number of MBs
         * */
        mfxU32 numMbCodeToAlloc = mbcodeout ? mbcodeout->NumMBAlloc : 0;
        mfxU32 numMVCodeToAlloc = mvout     ? mvout->NumMBAlloc     : numMbCodeToAlloc;

        if ((NULL != mvout) && (NULL == mbcodeout))
            numMbCodeToAlloc = numMVCodeToAlloc;

        if ((mvout != NULL) || (mbcodeout != NULL))
        {
            //output buffer for MV
            mfxU32 vaFeiMVOutBufSize = sizeof(VAMotionVector) * 16 * numMVCodeToAlloc;
            if (VA_INVALID_ID == m_vaFeiMVOutId[idxRecon + feiFieldId] ||
                vaFeiMVOutBufSize > m_vaFeiMVOutBufSize[idxRecon + feiFieldId])
            {
                MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "MV");
                MFX_DESTROY_VABUFFER(m_vaFeiMVOutId[idxRecon + feiFieldId], m_vaDisplay);
                m_vaFeiMVOutBufSize[idxRecon + feiFieldId] = 0;
                {
                    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaCreateBuffer");
                    vaSts = vaCreateBuffer(m_vaDisplay,
                            m_vaContextEncode,
                            (VABufferType)VAEncFEIMVBufferType,
                            vaFeiMVOutBufSize,
                            1, //limitation from driver, num elements should be 1
                            NULL, //should be mapped later
                            &m_vaFeiMVOutId[idxRecon + feiFieldId]);
                }
                MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
                //mdprintf(stderr, "MV Out bufId=%d\n", vaFeiMVOutId);
                m_vaFeiMVOutBufSize[idxRecon + feiFieldId] = vaFeiMVOutBufSize;
            }

            //output buffer for MBCODE (PAK object cmds)
            mfxU32 vaFeiMCODEOutBufSize = sizeof(VAEncFEIMBCodeH264) * numMbCodeToAlloc;
            if (VA_INVALID_ID == m_vaFeiMCODEOutId[idxRecon + feiFieldId] ||
                vaFeiMCODEOutBufSize > m_vaFeiMCODEOutBufSize[idxRecon + feiFieldId])
            {
                MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "MBcode");
                MFX_DESTROY_VABUFFER(m_vaFeiMCODEOutId[idxRecon + feiFieldId], m_vaDisplay);
                m_vaFeiMCODEOutBufSize[idxRecon + feiFieldId] = 0;
                {
                    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaCreateBuffer");
                    vaSts = vaCreateBuffer(m_vaDisplay,
                            m_vaContextEncode,
                            (VABufferType)VAEncFEIMBCodeBufferType,
                            vaFeiMCODEOutBufSize,
                            1, //limitation from driver, num elements should be 1
                            NULL, //should be mapped later
                            &m_vaFeiMCODEOutId[idxRecon + feiFieldId]);
                }
                MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
                //mdprintf(stderr, "MCODE Out bufId=%d\n", vaFeiMCODEOutId);
                m_vaFeiMCODEOutBufSize[idxRecon + feiFieldId] = vaFeiMCODEOutBufSize;
            }
        }

        // Configure FrameControl buffer
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "FrameCtrl");
            VAEncMiscParameterBuffer *miscParam;

            {
                MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaCreateBuffer");
                vaSts = vaCreateBuffer(m_vaDisplay,
                    m_vaContextEncode,
                    VAEncMiscParameterBufferType,
                    sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterFEIFrameControlH264),
                    1,
                    NULL,
                    &vaFeiFrameControlId);
                MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
            }

            {
                MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaMapBuffer");
                vaSts = vaMapBuffer(m_vaDisplay,
                    vaFeiFrameControlId,
                    (void **)&miscParam);
            }
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

            miscParam->type = (VAEncMiscParameterType)VAEncMiscParameterTypeFEIFrameControl;
            VAEncMiscParameterFEIFrameControlH264* vaFeiFrameControl = (VAEncMiscParameterFEIFrameControlH264*)miscParam->data;
            memset(vaFeiFrameControl, 0, sizeof(VAEncMiscParameterFEIFrameControlH264)); //check if we need this

            vaFeiFrameControl->function        = VA_FEI_FUNCTION_ENC_PAK;
            vaFeiFrameControl->adaptive_search = frameCtrl->AdaptiveSearch;
            vaFeiFrameControl->distortion_type = frameCtrl->DistortionType;
            vaFeiFrameControl->inter_sad       = frameCtrl->InterSAD;
            vaFeiFrameControl->intra_part_mask = frameCtrl->IntraPartMask;

            vaFeiFrameControl->intra_sad       = frameCtrl->IntraSAD;
            vaFeiFrameControl->len_sp          = frameCtrl->LenSP;
            vaFeiFrameControl->search_path     = frameCtrl->SearchPath;

            vaFeiFrameControl->distortion      = m_vaFeiMBStatId[idxRecon + feiFieldId];
            vaFeiFrameControl->mv_data         = m_vaFeiMVOutId[idxRecon + feiFieldId];
            vaFeiFrameControl->mb_code_data    = m_vaFeiMCODEOutId[idxRecon + feiFieldId];

            vaFeiFrameControl->qp              = vaFeiMBQPId;
            vaFeiFrameControl->mb_ctrl         = vaFeiMBControlId;
            vaFeiFrameControl->mb_input        = frameCtrl->PerMBInput;
            vaFeiFrameControl->mb_qp           = frameCtrl->PerMBQp;  //not supported for now
            vaFeiFrameControl->mb_size_ctrl    = frameCtrl->MBSizeCtrl;
            vaFeiFrameControl->multi_pred_l0   = frameCtrl->MultiPredL0;
            vaFeiFrameControl->multi_pred_l1   = frameCtrl->MultiPredL1;

            vaFeiFrameControl->mv_predictor         = vaFeiMVPredId;
            vaFeiFrameControl->mv_predictor_enable  = frameCtrl->MVPredictor;
            vaFeiFrameControl->num_mv_predictors_l0 = frameCtrl->MVPredictor ? frameCtrl->NumMVPredictors[0] : 0;
            vaFeiFrameControl->num_mv_predictors_l1 = frameCtrl->MVPredictor ? frameCtrl->NumMVPredictors[1] : 0;

            vaFeiFrameControl->ref_height               = frameCtrl->RefHeight;
            vaFeiFrameControl->ref_width                = frameCtrl->RefWidth;
            vaFeiFrameControl->repartition_check_enable = frameCtrl->RepartitionCheckEnable;
            vaFeiFrameControl->search_window            = frameCtrl->SearchWindow;
            vaFeiFrameControl->sub_mb_part_mask         = frameCtrl->SubMBPartMask;
            vaFeiFrameControl->sub_pel_mode             = frameCtrl->SubPelMode;

            if (NULL != rePakCtrl)
            {
                vaFeiFrameControl->max_frame_size = rePakCtrl->MaxFrameSize;
                vaFeiFrameControl->num_passes     = rePakCtrl->NumPasses;

                for (mfxU32 ii = 0; ii < vaFeiFrameControl->num_passes; ++ii)
                    qp_delta_list[ii] = rePakCtrl->DeltaQP[ii];

                vaFeiFrameControl->delta_qp = &qp_delta_list[0];
            }
            //MFX_DESTROY_VABUFFER(m_spsBufferId, m_vaDisplay);

            {
                MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
                vaSts = vaUnmapBuffer(m_vaDisplay, vaFeiFrameControlId);  //check for deletions
            }
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

            configBuffers[buffersCount++] = vaFeiFrameControlId;
        }
    }
#endif
    //------------------------------------------------------------------
    // buffer creation & configuration
    //------------------------------------------------------------------
    {
        // 1. sequence level
        {
            MFX_DESTROY_VABUFFER(m_spsBufferId, m_vaDisplay);
            vaSts = vaCreateBuffer(m_vaDisplay,
                                   m_vaContextEncode,
                                   VAEncSequenceParameterBufferType,
                                   sizeof(m_sps),
                                   1,
                                   &m_sps,
                                   &m_spsBufferId);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

            configBuffers[buffersCount++] = m_spsBufferId;
        }

        // 2. Picture level
        {
            MFX_DESTROY_VABUFFER(m_ppsBufferId, m_vaDisplay);
            vaSts = vaCreateBuffer(m_vaDisplay,
                                   m_vaContextEncode,
                                   VAEncPictureParameterBufferType,
                                   sizeof(m_pps),
                                   1,
                                   &m_pps,
                                   &m_ppsBufferId);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

            configBuffers[buffersCount++] = m_ppsBufferId;
        }

        // 3. Slice level
        for( i = 0; i < m_slice.size(); i++ )
        {
            //MFX_DESTROY_VABUFFER(m_sliceBufferId[i], m_vaDisplay);
            vaSts = vaCreateBuffer(m_vaDisplay,
                                    m_vaContextEncode,
                                    VAEncSliceParameterBufferType,
                                    sizeof(m_slice[i]),
                                    1,
                                    &m_slice[i],
                                    &m_sliceBufferId[i]);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        }
    }

    if (m_caps.HeaderInsertion == 1 && skipFlag == NO_SKIP)
    {
        // SEI
        if (sei.Size() > 0)
        {
            packed_header_param_buffer.type = VAEncPackedHeaderRawData;
            packed_header_param_buffer.has_emulation_bytes = 1;
            packed_header_param_buffer.bit_length = sei.Size()*8;

            MFX_DESTROY_VABUFFER(m_packedSeiHeaderBufferId, m_vaDisplay);
            vaSts = vaCreateBuffer(m_vaDisplay,
                    m_vaContextEncode,
                    VAEncPackedHeaderParameterBufferType,
                    sizeof(packed_header_param_buffer),
                    1,
                    &packed_header_param_buffer,
                    &m_packedSeiHeaderBufferId);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

            MFX_DESTROY_VABUFFER(m_packedSeiBufferId, m_vaDisplay);
            vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                VAEncPackedHeaderDataBufferType,
                                sei.Size(), 1, RemoveConst(sei.Buffer()),
                                &m_packedSeiBufferId);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

            configBuffers[buffersCount++] = m_packedSeiHeaderBufferId;
            configBuffers[buffersCount++] = m_packedSeiBufferId;
        }
    }
    else
    {
        // AUD
        if (task.m_insertAud[fieldId])
        {
            ENCODE_PACKEDHEADER_DATA const & packedAud = m_headerPacker.PackAud(task, fieldId);

            packed_header_param_buffer.type = VAEncPackedHeaderRawData;
            packed_header_param_buffer.has_emulation_bytes = !packedAud.SkipEmulationByteCount;
            packed_header_param_buffer.bit_length = packedAud.DataLength*8;

            MFX_DESTROY_VABUFFER(m_packedAudHeaderBufferId, m_vaDisplay);
            vaSts = vaCreateBuffer(m_vaDisplay,
                    m_vaContextEncode,
                    VAEncPackedHeaderParameterBufferType,
                    sizeof(packed_header_param_buffer),
                    1,
                    &packed_header_param_buffer,
                    &m_packedAudHeaderBufferId);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

            MFX_DESTROY_VABUFFER(m_packedAudBufferId, m_vaDisplay);
            vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                VAEncPackedHeaderDataBufferType,
                                packedAud.DataLength, 1, packedAud.pData,
                                &m_packedAudBufferId);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

            packedBufferIndexes.push_back(buffersCount);
            packedDataSize += packed_header_param_buffer.bit_length;
            configBuffers[buffersCount++] = m_packedAudHeaderBufferId;
            configBuffers[buffersCount++] = m_packedAudBufferId;
        }
        // SPS
        if (task.m_insertSps[fieldId])
        {
            std::vector<ENCODE_PACKEDHEADER_DATA> const & packedSpsArray = m_headerPacker.GetSps();
            ENCODE_PACKEDHEADER_DATA const & packedSps = packedSpsArray[0];

            packed_header_param_buffer.type = VAEncPackedHeaderSequence;
            packed_header_param_buffer.has_emulation_bytes = !packedSps.SkipEmulationByteCount;
            packed_header_param_buffer.bit_length = packedSps.DataLength*8;

            MFX_DESTROY_VABUFFER(m_packedSpsHeaderBufferId, m_vaDisplay);
            vaSts = vaCreateBuffer(m_vaDisplay,
                    m_vaContextEncode,
                    VAEncPackedHeaderParameterBufferType,
                    sizeof(packed_header_param_buffer),
                    1,
                    &packed_header_param_buffer,
                    &m_packedSpsHeaderBufferId);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

            MFX_DESTROY_VABUFFER(m_packedSpsBufferId, m_vaDisplay);
            vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                VAEncPackedHeaderDataBufferType,
                                packedSps.DataLength, 1, packedSps.pData,
                                &m_packedSpsBufferId);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

            packedBufferIndexes.push_back(buffersCount);
            packedDataSize += packed_header_param_buffer.bit_length;
            configBuffers[buffersCount++] = m_packedSpsHeaderBufferId;
            configBuffers[buffersCount++] = m_packedSpsBufferId;
        }

        if (task.m_insertPps[fieldId])
        {
            // PPS
            std::vector<ENCODE_PACKEDHEADER_DATA> const & packedPpsArray = m_headerPacker.GetPps();
            ENCODE_PACKEDHEADER_DATA const & packedPps = packedPpsArray[0];

            packed_header_param_buffer.type = VAEncPackedHeaderPicture;
            packed_header_param_buffer.has_emulation_bytes = !packedPps.SkipEmulationByteCount;
            packed_header_param_buffer.bit_length = packedPps.DataLength*8;

            MFX_DESTROY_VABUFFER(m_packedPpsHeaderBufferId, m_vaDisplay);
            vaSts = vaCreateBuffer(m_vaDisplay,
                    m_vaContextEncode,
                    VAEncPackedHeaderParameterBufferType,
                    sizeof(packed_header_param_buffer),
                    1,
                    &packed_header_param_buffer,
                    &m_packedPpsHeaderBufferId);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

            MFX_DESTROY_VABUFFER(m_packedPpsBufferId, m_vaDisplay);
            vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                VAEncPackedHeaderDataBufferType,
                                packedPps.DataLength, 1, packedPps.pData,
                                &m_packedPpsBufferId);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

            packedBufferIndexes.push_back(buffersCount);
            packedDataSize += packed_header_param_buffer.bit_length;
            configBuffers[buffersCount++] = m_packedPpsHeaderBufferId;
            configBuffers[buffersCount++] = m_packedPpsBufferId;
        }

        // SEI
        if (sei.Size() > 0)
        {
            packed_header_param_buffer.type = VAEncPackedHeaderRawData;
            packed_header_param_buffer.has_emulation_bytes = 1;
            packed_header_param_buffer.bit_length = sei.Size()*8;

            MFX_DESTROY_VABUFFER(m_packedSeiHeaderBufferId, m_vaDisplay);
            vaSts = vaCreateBuffer(m_vaDisplay,
                    m_vaContextEncode,
                    VAEncPackedHeaderParameterBufferType,
                    sizeof(packed_header_param_buffer),
                    1,
                    &packed_header_param_buffer,
                    &m_packedSeiHeaderBufferId);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

            MFX_DESTROY_VABUFFER(m_packedSeiBufferId, m_vaDisplay);
            vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                VAEncPackedHeaderDataBufferType,
                                sei.Size(), 1, RemoveConst(sei.Buffer()),
                                &m_packedSeiBufferId);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

            packedBufferIndexes.push_back(buffersCount);
            packedDataSize += packed_header_param_buffer.bit_length;
            configBuffers[buffersCount++] = m_packedSeiHeaderBufferId;
            configBuffers[buffersCount++] = m_packedSeiBufferId;
        }

        if (skipFlag != NO_SKIP)
        {
            // The whole slice
            ENCODE_PACKEDHEADER_DATA const & packedSkippedSlice = m_headerPacker.PackSkippedSlice(task, fieldId);

            packed_header_param_buffer.type = VAEncPackedHeaderRawData;
            packed_header_param_buffer.has_emulation_bytes = !packedSkippedSlice.SkipEmulationByteCount;
            packed_header_param_buffer.bit_length = packedSkippedSlice.DataLength*8;

            MFX_DESTROY_VABUFFER(m_packedSkippedSliceHeaderBufferId, m_vaDisplay);
            vaSts = vaCreateBuffer(m_vaDisplay,
                m_vaContextEncode,
                VAEncPackedHeaderParameterBufferType,
                sizeof(packed_header_param_buffer),
                1,
                &packed_header_param_buffer,
                &m_packedSkippedSliceHeaderBufferId);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

            MFX_DESTROY_VABUFFER(m_packedSkippedSliceBufferId, m_vaDisplay);
            vaSts = vaCreateBuffer(m_vaDisplay,
                m_vaContextEncode,
                VAEncPackedHeaderDataBufferType,
                packedSkippedSlice.DataLength, 1, packedSkippedSlice.pData,
                &m_packedSkippedSliceBufferId);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

            packedBufferIndexes.push_back(buffersCount);
            packedDataSize += packed_header_param_buffer.bit_length;
            configBuffers[buffersCount++] = m_packedSkippedSliceHeaderBufferId;
            configBuffers[buffersCount++] = m_packedSkippedSliceBufferId;

        }
        else
        {
            if ((m_core->GetHWType() >= MFX_HW_HSW) && (m_core->GetHWType() != MFX_HW_VLV))
            {
                // length of Prefix NAL unit: now it's always 8 bytes for reference picture, and 7 bytes for non-reference picture
                // FIXME: need to add zero_byte if AU starts from Prefix NAL unit
                mfxU32 prefix_bytes = (7 + !!task.m_nalRefIdc[fieldId]) * m_headerPacker.isSvcPrefixUsed();

                //Slice headers only
                std::vector<ENCODE_PACKEDHEADER_DATA> const & packedSlices = m_headerPacker.PackSlices(task, fieldId);
                for (size_t i = 0; i < packedSlices.size(); i++)
                {
                    ENCODE_PACKEDHEADER_DATA const & packedSlice = packedSlices[i];

                    if (prefix_bytes)
                    {
                        packed_header_param_buffer.type = VAEncPackedHeaderRawData;
                        packed_header_param_buffer.has_emulation_bytes = 1;
                        packed_header_param_buffer.bit_length = (prefix_bytes * 8);

                        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                VAEncPackedHeaderParameterBufferType,
                                sizeof(packed_header_param_buffer),
                                1,
                                &packed_header_param_buffer,
                                &m_packedSvcPrefixHeaderBufferId[i]);
                        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

                        vaSts = vaCreateBuffer(m_vaDisplay,
                                            m_vaContextEncode,
                                            VAEncPackedHeaderDataBufferType,
                                             prefix_bytes, 1, packedSlice.pData,
                                            &m_packedSvcPrefixBufferId[i]);
                        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

                        configBuffers[buffersCount++] = m_packedSvcPrefixHeaderBufferId[i];
                        configBuffers[buffersCount++] = m_packedSvcPrefixBufferId[i];
                    }

                    packed_header_param_buffer.type = VAEncPackedHeaderH264_Slice;
                    packed_header_param_buffer.has_emulation_bytes = 0;
                    packed_header_param_buffer.bit_length = packedSlice.DataLength - (prefix_bytes * 8); // DataLength is already in bits !

                    //MFX_DESTROY_VABUFFER(m_packeSliceHeaderBufferId[i], m_vaDisplay);
                    vaSts = vaCreateBuffer(m_vaDisplay,
                            m_vaContextEncode,
                            VAEncPackedHeaderParameterBufferType,
                            sizeof(packed_header_param_buffer),
                            1,
                            &packed_header_param_buffer,
                            &m_packeSliceHeaderBufferId[i]);
                    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

                    //MFX_DESTROY_VABUFFER(m_packedSliceBufferId[i], m_vaDisplay);
                    vaSts = vaCreateBuffer(m_vaDisplay,
                                        m_vaContextEncode,
                                        VAEncPackedHeaderDataBufferType,
                                        (packedSlice.DataLength + 7) / 8 - prefix_bytes, 1, packedSlice.pData + prefix_bytes,
                                        &m_packedSliceBufferId[i]);
                    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

                    configBuffers[buffersCount++] = m_packeSliceHeaderBufferId[i];
                    configBuffers[buffersCount++] = m_packedSliceBufferId[i];
                }
            }
        }
    }

    configBuffers[buffersCount++] = m_hrdBufferId;
    MFX_CHECK_WITH_ASSERT(MFX_ERR_NONE == SetRateControl(m_videoParam, m_mbbrc, task.m_minQP, task.m_maxQP,
                                                         m_vaDisplay, m_vaContextEncode, m_rateParamBufferId), MFX_ERR_DEVICE_FAILED);
    configBuffers[buffersCount++] = m_rateParamBufferId;
    configBuffers[buffersCount++] = m_frameRateId;
    configBuffers[buffersCount++] = m_qualityLevelId;

/*
 * Limit frame size by application/user level
 */
    m_userMaxFrameSize = (UINT)task.m_maxIFrameSize;
//    if (task.m_frameOrder)
//        m_sps.bResetBRC = true;
    MFX_CHECK_WITH_ASSERT(MFX_ERR_NONE == SetMaxFrameSize(m_userMaxFrameSize, m_vaDisplay,
                                                          m_vaContextEncode, m_maxFrameSizeId), MFX_ERR_DEVICE_FAILED);
    configBuffers[buffersCount++] = m_maxFrameSizeId;

#if !defined(ANDROID)
/*
 *  By default (0) - driver will decide.
 *  1 - disable trellis quantization
 *  x0E - enable for any type of frames
 */
    if (m_newTrellisQuantization != 0)
    {
        m_curTrellisQuantization = m_newTrellisQuantization;
        MFX_CHECK_WITH_ASSERT(MFX_ERR_NONE == SetTrellisQuantization(m_curTrellisQuantization, m_vaDisplay,
                                                                     m_vaContextEncode, m_quantizationId), MFX_ERR_DEVICE_FAILED);
        configBuffers[buffersCount++] = m_quantizationId;
    }
#endif

 /*
 *   RollingIntraRefresh
 */
    if (memcmp(&task.m_IRState, &m_RIRState, sizeof(m_RIRState)))
    {
        m_RIRState = task.m_IRState;
        MFX_CHECK_WITH_ASSERT(MFX_ERR_NONE == SetRollingIntraRefresh(m_RIRState, m_vaDisplay,
                                                                     m_vaContextEncode, m_rirId), MFX_ERR_DEVICE_FAILED);
        configBuffers[buffersCount++] = m_rirId;
    }

    if (task.m_numRoi)
    {
        MFX_CHECK_WITH_ASSERT(MFX_ERR_NONE == SetROI(task, m_arrayVAEncROI, m_vaDisplay, m_vaContextEncode, m_roiBufferId),
                              MFX_ERR_DEVICE_FAILED);
        configBuffers[buffersCount++] = m_roiBufferId;
    }

    /*FEI has its own interface for MBQp*/
    if ((task.m_isMBQP) && (!m_isENCPAK))
    {
        const mfxExtMBQP *mbqp = GetExtBuffer(task.m_ctrl);
        mfxU32 mbW = m_sps.picture_width_in_mbs;
        mfxU32 mbH = m_sps.picture_height_in_mbs / (2 - !task.m_fieldPicFlag);
        //width(64byte alignment) height(8byte alignment)
        mfxU32 bufW = ((mbW + 63) & ~63);
        mfxU32 bufH = ((mbH + 7) & ~7);
        mfxU32 fieldOffset = (mfxU32)fieldId * (mbH * mbW) * (mfxU32)!!task.m_fieldPicFlag;

        if (mbqp && mbqp->QP && mbqp->NumQPAlloc >= mbW * m_sps.picture_height_in_mbs
            && m_mbqp_buffer.size() >= (bufW * bufH))
        {

            Zero(m_mbqp_buffer);
            for (mfxU32 mbRow = 0; mbRow < mbH; mbRow ++)
                for (mfxU32 mbCol = 0; mbCol < mbW; mbCol ++)
                    m_mbqp_buffer[mbRow * bufW + mbCol].qp = mbqp->QP[fieldOffset + mbRow * mbW + mbCol];

            MFX_DESTROY_VABUFFER(m_mbqpBufferId, m_vaDisplay);
            // LibVA expect full buffer size w/o interlace adjustments
            vaSts = vaCreateBuffer(m_vaDisplay,
                m_vaContextEncode,
                (VABufferType)VAEncQPBufferType,
                bufW * sizeof(VAEncQPBufferH264),
                ((m_sps.picture_height_in_mbs + 7) & ~7),
                &m_mbqp_buffer[0],
                &m_mbqpBufferId);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

            configBuffers[buffersCount++] = m_mbqpBufferId;
        }
    }

    if (ctrlNoSkipMap)
    {
        mfxU32 mbW = m_sps.picture_width_in_mbs;
        mfxU32 mbH = m_sps.picture_height_in_mbs / (2 - !task.m_fieldPicFlag);
        //width(64byte alignment) height(8byte alignment)
        mfxU32 bufW = ((mbW + 63) & ~63);
        mfxU32 bufH = ((mbH + 7) & ~7);
        mfxU32 fieldOffset = (mfxU32)fieldId * (mbH * mbW) * (mfxU32)!!task.m_fieldPicFlag;

        if (   m_mb_noskip_buffer.size() >= (bufW * bufH)
            && ctrlNoSkipMap->Map
            && ctrlNoSkipMap->MapSize >= (mbW * m_sps.picture_height_in_mbs))
        {
            Zero(m_mb_noskip_buffer);
            for (mfxU32 mbRow = 0; mbRow < mbH; mbRow ++)
                MFX_INTERNAL_CPY(&m_mb_noskip_buffer[mbRow * bufW], &ctrlNoSkipMap->Map[fieldOffset + mbRow * mbW], mbW);

            MFX_DESTROY_VABUFFER(m_mbNoSkipBufferId, m_vaDisplay);
            vaSts = vaCreateBuffer(m_vaDisplay,
                    m_vaContextEncode,
                    (VABufferType)VAEncMacroblockDisableSkipMapBufferType,
                    (bufW * bufH),
                    1,
                    &m_mb_noskip_buffer[0],
                    &m_mbNoSkipBufferId);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

            configBuffers[buffersCount++] = m_mbNoSkipBufferId;
        }
    }


    if (ctrlOpt2 || ctrlOpt3)
    {
        MFX_CHECK_WITH_ASSERT(MFX_ERR_NONE == SetQualityParams(m_videoParam, m_vaDisplay,
                                                               m_vaContextEncode, m_qualityParamsId, &task.m_ctrl), MFX_ERR_DEVICE_FAILED);
    }
    if (VA_INVALID_ID != m_qualityParamsId) configBuffers[buffersCount++] = m_qualityParamsId;

    assert(buffersCount <= configBuffers.size());

    mfxU32 storedSize = 0;

#ifndef MFX_AVC_ENCODING_UNIT_DISABLE
    if (task.m_collectUnitsInfo)
    {
        m_headerPacker.GetHeadersInfo(task.m_headersCache[fieldId], task, fieldId);
    }
#endif

    if (skipFlag != NORMAL_MODE)
    {
        MFX_CHECK_WITH_ASSERT(MFX_ERR_NONE == SetSkipFrame(m_vaDisplay, m_vaContextEncode, m_miscParameterSkipBufferId,
                                                           skipFlag ? skipFlag : !!m_numSkipFrames,
                                                           m_numSkipFrames, m_sizeSkipFrames), MFX_ERR_DEVICE_FAILED);

        configBuffers[buffersCount++] = m_miscParameterSkipBufferId;

        m_numSkipFrames  = 0;
        m_sizeSkipFrames = 0;

        mdprintf(stderr, "task.m_frameNum=%d\n", task.m_frameNum);
        mdprintf(stderr, "inputSurface=%d\n", *inputSurface);
        mdprintf(stderr, "m_pps.CurrPic.picture_id=%d\n", m_pps.CurrPic.picture_id);
        mdprintf(stderr, "m_pps.ReferenceFrames[0]=%d\n", m_pps.ReferenceFrames[0].picture_id);
        mdprintf(stderr, "m_pps.ReferenceFrames[1]=%d\n", m_pps.ReferenceFrames[1].picture_id);
        //------------------------------------------------------------------
        // Rendering
        //------------------------------------------------------------------
        MFX_LTRACE_2(MFX_TRACE_LEVEL_HOTSPOTS, "A|ENCODE|AVC|PACKET_START|", "%d|%d", m_vaContextEncode, task.m_frameNum);
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaBeginPicture");

            vaSts = vaBeginPicture(
                m_vaDisplay,
                m_vaContextEncode,
                *inputSurface);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        }
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaRenderPicture");
            vaSts = vaRenderPicture(
                m_vaDisplay,
                m_vaContextEncode,
                Begin(configBuffers),
                buffersCount);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

            for( i = 0; i < m_slice.size(); i++)
            {
                vaSts = vaRenderPicture(
                    m_vaDisplay,
                    m_vaContextEncode,
                    &m_sliceBufferId[i],
                    1);
                MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
            }
        }
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaEndPicture");

            vaSts = vaEndPicture(m_vaDisplay, m_vaContextEncode);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        }
        MFX_LTRACE_2(MFX_TRACE_LEVEL_HOTSPOTS, "A|ENCODE|AVC|PACKET_END|", "%d|%d", m_vaContextEncode, task.m_frameNum);
    }
    else
    {
        VACodedBufferSegment *codedBufferSegment;
        {
            codedBuffer = m_bsQueue[task.m_idxBs[fieldId]].surface;
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaMapBuffer");
            vaSts = vaMapBuffer(
                m_vaDisplay,
                codedBuffer,
                (void **)(&codedBufferSegment));
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        }
        codedBufferSegment->next = 0;
        codedBufferSegment->reserved = 0;
        codedBufferSegment->status = 0;

        MFX_CHECK_WITH_ASSERT(codedBufferSegment->buf, MFX_ERR_DEVICE_FAILED);

        mfxU8 *  bsDataStart = (mfxU8 *)codedBufferSegment->buf;
        mfxU8 *  bsDataEnd   = bsDataStart;

        if (skipMode != MFX_SKIPFRAME_INSERT_NOTHING)
        {
            for (size_t i = 0; i < packedBufferIndexes.size(); i++)
            {
                size_t headerIndex = packedBufferIndexes[i];

                void *pBufferHeader;
                {
                    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaMapBuffer");
                    vaSts = vaMapBuffer(m_vaDisplay, configBuffers[headerIndex], &pBufferHeader);
                }
                MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

                void *pData;
                {
                    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaMapBuffer");
                    vaSts = vaMapBuffer(m_vaDisplay, configBuffers[headerIndex + 1], &pData);
                }
                MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

                if (pBufferHeader && pData)
                {
                    VAEncPackedHeaderParameterBuffer const & header = *(VAEncPackedHeaderParameterBuffer const *)pBufferHeader;

                    mfxU32 lenght = (header.bit_length+7)/8;

                    assert(mfxU32(bsDataStart + m_width*m_height - bsDataEnd) > lenght);
                    assert(header.has_emulation_bytes);

                    MFX_INTERNAL_CPY(bsDataEnd, pData, lenght);
                    bsDataEnd += lenght;
                }

                vaSts = vaUnmapBuffer(m_vaDisplay, configBuffers[headerIndex]);
                MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

                vaSts = vaUnmapBuffer(m_vaDisplay, configBuffers[headerIndex + 1]);
                MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
            }
        }

        storedSize = mfxU32(bsDataEnd - bsDataStart);
        codedBufferSegment->size = storedSize;

        m_numSkipFrames ++;
        m_sizeSkipFrames += (skipMode != MFX_SKIPFRAME_INSERT_NOTHING) ? storedSize : 0;

        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
            vaSts = vaUnmapBuffer( m_vaDisplay, codedBuffer );
        }
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }
#ifdef MFX_ENABLE_MFE
    if (m_mfe){
        mfxU32 timeout = task.m_mfeTimeToWait>>task.m_fieldPicFlag;
        /*if(!task.m_userTimeout)
        {
            mfxU32 passed = (task.m_beginTime - vm_time_get_tick());
            if (passed < task.m_mfeTimeToWait)
            {
                timeout = timeout - passed;//-time for encode;//need to add a table with encoding times to
            }
            else
            {
                timeout = 0;
            }
        }*/
        mfxStatus sts = m_mfe->Submit(m_vaContextEncode, (task.m_flushMfe? 0 : timeout), skipFlag == NORMAL_MODE);
        if (sts != MFX_ERR_NONE)
            return sts;
    }
#endif

    //------------------------------------------------------------------
    // PostStage
    //------------------------------------------------------------------
    // put to cache
    {
        UMC::AutomaticUMCMutex guard(m_guard);

        ExtVASurface currentFeedback;
        currentFeedback.number  = task.m_statusReportNumber[fieldId];
        currentFeedback.surface = (NORMAL_MODE == skipFlag) ? VA_INVALID_SURFACE : *inputSurface;
        currentFeedback.idxBs   = task.m_idxBs[fieldId];
        currentFeedback.size    = storedSize;

#if defined(MFX_ENABLE_H264_VIDEO_FEI_ENCPAK) || defined(MFX_ENABLE_H264_VIDEO_FEI_PREENC)
        if (m_isENCPAK)
        {
            currentFeedback.mv     = m_vaFeiMVOutId[idxRecon + feiFieldId];
            currentFeedback.mbstat = m_vaFeiMBStatId[idxRecon + feiFieldId];
            currentFeedback.mbcode = m_vaFeiMCODEOutId[idxRecon + feiFieldId];
        }
#endif
        m_feedbackCache.push_back( currentFeedback );
    }

#if defined(MFX_ENABLE_H264_VIDEO_FEI_ENCPAK) || defined(MFX_ENABLE_H264_VIDEO_FEI_PREENC)
    MFX_DESTROY_VABUFFER(vaFeiFrameControlId, m_vaDisplay);
    MFX_DESTROY_VABUFFER(vaFeiMVPredId,       m_vaDisplay);
    MFX_DESTROY_VABUFFER(vaFeiMBControlId,    m_vaDisplay);
    MFX_DESTROY_VABUFFER(vaFeiMBQPId,         m_vaDisplay);
#endif

    return MFX_ERR_NONE;
} // mfxStatus VAAPIEncoder::Execute(ExecuteBuffers& data, mfxU32 fieldId)


mfxStatus VAAPIEncoder::QueryStatus(
    DdiTask & task,
    mfxU32    fieldId)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VAAPIEncoder::QueryStatus");
    mfxStatus sts = MFX_ERR_NONE;
    bool isFound = false;
    VASurfaceID waitSurface;
    mfxU32 waitIdxBs;
    mfxU32 waitSize;
    mfxU32 indxSurf;
    ExtVASurface currentFeedback;

    UMC::AutomaticUMCMutex guard(m_guard);

    for( indxSurf = 0; indxSurf < m_feedbackCache.size(); indxSurf++ )
    {
        currentFeedback = m_feedbackCache[ indxSurf ];

        if( currentFeedback.number == task.m_statusReportNumber[fieldId] )
        {
            waitSurface = currentFeedback.surface;
            waitIdxBs   = currentFeedback.idxBs;
            waitSize    = currentFeedback.size;

            isFound  = true;
            break;
        }
    }

    if( !isFound )
    {
        return MFX_ERR_UNKNOWN;
    }

    if (VA_INVALID_SURFACE == waitSurface) //skipped frame
    {
        task.m_bsDataLength[fieldId] = waitSize;
        m_feedbackCache.erase(m_feedbackCache.begin() + indxSurf);

        return MFX_ERR_NONE;
    }

    // find used bitstream
    VABufferID codedBuffer;
    if(waitIdxBs < m_bsQueue.size())
    {
        codedBuffer = m_bsQueue[waitIdxBs].surface;
    }
    else
    {
        return MFX_ERR_UNKNOWN;
    }

    VAStatus vaSts = VA_STATUS_SUCCESS;

    m_feedbackCache.erase(m_feedbackCache.begin() + indxSurf);
    guard.Unlock();

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaSyncSurface");
        vaSts = vaSyncSurface(m_vaDisplay, waitSurface);
        // ignore VA_STATUS_ERROR_DECODING_ERROR in encoder
        if (vaSts == VA_STATUS_ERROR_DECODING_ERROR)
            vaSts = VA_STATUS_SUCCESS;
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }


    VACodedBufferSegment *codedBufferSegment = 0;
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaMapBuffer");
        vaSts = vaMapBuffer(
            m_vaDisplay,
            codedBuffer,
            (void **)(&codedBufferSegment));
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }

    task.m_bsDataLength[fieldId] = codedBufferSegment->size;

    // Disable QP report on Linux until driver fix.
    //task.m_qpY[fieldId] = (codedBufferSegment->status & VA_CODED_BUF_STATUS_PICTURE_AVE_QP_MASK);

    if (codedBufferSegment->status & VA_CODED_BUF_STATUS_BAD_BITSTREAM)
        sts = MFX_ERR_GPU_HANG;
    else if (!codedBufferSegment->size || !codedBufferSegment->buf)
        sts = MFX_ERR_DEVICE_FAILED;

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
        vaSts = vaUnmapBuffer( m_vaDisplay, codedBuffer );
    }
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    MFX_CHECK_STS(sts);

    if (m_isENCPAK)
    {
        // Use parity to order conversion here (FEI ext buffers attached by field order, but MSDK operates in terms of fields parity)
        sts = VAAPIEncoder::QueryStatusFEI(task, task.m_fid[fieldId],
                                           currentFeedback, codedBufferSegment->status);
        MFX_CHECK_STS(sts);
    }

    return sts;
} //mfxStatus VAAPIEncoder::QueryStatus

mfxStatus VAAPIEncoder::QueryStatusFEI(
    DdiTask const & task,
    mfxU32  feiFieldId,
    ExtVASurface const & curFeedback,
    mfxU32  codedStatus)
{
#if defined(MFX_ENABLE_H264_VIDEO_FEI_ENCPAK) || defined(MFX_ENABLE_H264_VIDEO_FEI_PREENC)

    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VAAPIEncoder::QueryStatusFEI");
    VAStatus vaSts = VA_STATUS_SUCCESS;
    VABufferID vaFeiMBStatId    = curFeedback.mbstat;
    VABufferID vaFeiMBCODEOutId = curFeedback.mbcode;
    VABufferID vaFeiMVOutId     = curFeedback.mv;

    //find ext buffers
    mfxExtFeiEncMBStat  * mbstat     = NULL;
    mfxExtFeiEncMV      * mvout      = NULL;
    mfxExtFeiPakMBCtrl  * mbcodeout  = NULL;
#if (MFX_VERSION >= 1025)
    mfxExtFeiRepackStat * repackStat = NULL;
#endif

    // In case of single-field processing, only one buffer is attached
    mfxU32 idxToPickBuffer = task.m_singleFieldMode ? 0 : feiFieldId;

    if (NULL != task.m_bs)
    {
        mbstat     = GetExtBufferFEI(task.m_bs, idxToPickBuffer);
        mvout      = GetExtBufferFEI(task.m_bs, idxToPickBuffer);
        mbcodeout  = GetExtBufferFEI(task.m_bs, idxToPickBuffer);
#if (MFX_VERSION >= 1025)
        repackStat = GetExtBufferFEI(task.m_bs, idxToPickBuffer);
#endif
    }

    if (mbstat != NULL && vaFeiMBStatId != VA_INVALID_ID)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "MBstat");
        VAEncFEIDistortionH264* mbs;
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaMapBuffer");
            vaSts = vaMapBuffer(
                    m_vaDisplay,
                    vaFeiMBStatId,
                    (void **) (&mbs));
        }
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        //copy to output in task here MVs
        FastCopyBufferVid2Sys(mbstat->MB, mbs, sizeof(VAEncFEIDistortionH264) * mbstat->NumMBAlloc);

        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
            vaSts = vaUnmapBuffer(m_vaDisplay, vaFeiMBStatId);
        }
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }

    if (mvout != NULL && vaFeiMVOutId != VA_INVALID_ID)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "MV");
        VAMotionVector* mvs;
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaMapBuffer");
            vaSts = vaMapBuffer(
                    m_vaDisplay,
                    vaFeiMVOutId,
                    (void **) (&mvs));
        }
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        //copy to output in task here MVs
        FastCopyBufferVid2Sys(mvout->MB, mvs, sizeof(VAMotionVector) * 16 * mvout->NumMBAlloc);

        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
            vaSts = vaUnmapBuffer(m_vaDisplay, vaFeiMVOutId);
        }
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }

    if (mbcodeout != NULL && vaFeiMBCODEOutId != VA_INVALID_ID)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "MBcode");
        VAEncFEIMBCodeH264* mbcs;
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaMapBuffer");
            vaSts = vaMapBuffer(
                m_vaDisplay,
                vaFeiMBCODEOutId,
                (void **) (&mbcs));
        }
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        //copy to output in task here MVs
        FastCopyBufferVid2Sys(mbcodeout->MB, mbcs, sizeof(VAEncFEIMBCodeH264) * mbcodeout->NumMBAlloc);

        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
            vaSts = vaUnmapBuffer(m_vaDisplay, vaFeiMBCODEOutId);
        }
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }

#if (MFX_VERSION >= 1025)
    // Hard-coding below will get removed once related libva interface is updated
    if (repackStat)
        repackStat->NumPasses = (codedStatus & 0xf000000) >> 24;
#endif

    return MFX_ERR_NONE;
#else
    return MFX_ERR_UNKNOWN;
#endif
} //mfxStatus VAAPIEncoder::QueryStatusFEI

mfxStatus VAAPIEncoder::Destroy()
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VAAPIEncoder::Destroy");

    MFX_DESTROY_VABUFFER(m_spsBufferId,               m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_hrdBufferId,               m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_rateParamBufferId,         m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_frameRateId,               m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_qualityLevelId,            m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_maxFrameSizeId,            m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_quantizationId,            m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_rirId,                     m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_qualityParamsId,           m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_miscParameterSkipBufferId, m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_roiBufferId,               m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_ppsBufferId,               m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_mbqpBufferId,              m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_mbNoSkipBufferId, m_vaDisplay);
    for( mfxU32 i = 0; i < m_slice.size(); i++ )
    {
        MFX_DESTROY_VABUFFER(m_sliceBufferId[i],            m_vaDisplay);
        MFX_DESTROY_VABUFFER(m_packeSliceHeaderBufferId[i], m_vaDisplay);
        MFX_DESTROY_VABUFFER(m_packedSliceBufferId[i],      m_vaDisplay);
    }
    for( mfxU32 i = 0; i < m_packedSvcPrefixBufferId.size(); i++ )
    {
        MFX_DESTROY_VABUFFER(m_packedSvcPrefixHeaderBufferId[i], m_vaDisplay);
        MFX_DESTROY_VABUFFER(m_packedSvcPrefixBufferId[i],       m_vaDisplay);
    }
    MFX_DESTROY_VABUFFER(m_packedAudHeaderBufferId,          m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_packedAudBufferId,                m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_packedSpsHeaderBufferId,          m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_packedSpsBufferId,                m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_packedPpsHeaderBufferId,          m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_packedPpsBufferId,                m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_packedSeiHeaderBufferId,          m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_packedSeiBufferId,                m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_packedSkippedSliceHeaderBufferId, m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_packedSkippedSliceBufferId,       m_vaDisplay);

    for( mfxU32 i = 0; i < m_vaFeiMBStatId.size(); i++ )
    {
        MFX_DESTROY_VABUFFER(m_vaFeiMBStatId[i], m_vaDisplay);
    }

    for( mfxU32 i = 0; i < m_vaFeiMVOutId.size(); i++ )
    {
        MFX_DESTROY_VABUFFER(m_vaFeiMVOutId[i], m_vaDisplay);
    }

    for( mfxU32 i = 0; i < m_vaFeiMCODEOutId.size(); i++ )
    {
        MFX_DESTROY_VABUFFER(m_vaFeiMCODEOutId[i], m_vaDisplay);
    }

    if (m_vaContextEncode != VA_INVALID_ID)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaDestroyContext");
        vaDestroyContext(m_vaDisplay, m_vaContextEncode);
        m_vaContextEncode = VA_INVALID_ID;
    }

    if (m_vaConfig != VA_INVALID_ID)
    {
        vaDestroyConfig(m_vaDisplay, m_vaConfig);
        m_vaConfig = VA_INVALID_ID;
    }

    return MFX_ERR_NONE;
} // mfxStatus VAAPIEncoder::Destroy()

#endif // (MFX_ENABLE_H264_VIDEO_ENCODE) && (MFX_VA_LINUX)
/* EOF */


