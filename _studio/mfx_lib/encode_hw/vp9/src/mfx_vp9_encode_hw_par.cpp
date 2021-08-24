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

#include "mfx_common.h"

#include "mfxvp9.h"
#include "mfx_enc_common.h"
#include "mfx_vp9_encode_hw_par.h"
#include "mfx_vp9_encode_hw_utils.h"
#include <math.h>
#include <memory.h>
#include "mfx_common_int.h"
#include "mfx_ext_buffers.h"

namespace MfxHwVP9Encode
{

bool IsExtBufferSupportedInInit(mfxU32 id)
{
    return id == MFX_EXTBUFF_VP9_PARAM
        || id == MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION
        || id == MFX_EXTBUFF_CODING_OPTION2
        || id == MFX_EXTBUFF_CODING_OPTION3
        || id == MFX_EXTBUFF_DDI // RefreshFrameContext is used by driver
        || id == MFX_EXTBUFF_VP9_SEGMENTATION
        || id == MFX_EXTBUFF_VP9_TEMPORAL_LAYERS
        || id == MFX_EXTBUFF_ENCODER_RESET_OPTION;
}

bool IsExtBufferIgnoredInRuntime(mfxU32 id)
{
    // we may just ignore MFX_EXTBUFF_VPP_AUXDATA as it provides auxiliary information
    return id == MFX_EXTBUFF_VPP_AUXDATA;
}

bool IsExtBufferSupportedInRuntime(mfxU32 id)
{
    return id == MFX_EXTBUFF_VP9_PARAM
        || id == MFX_EXTBUFF_VP9_SEGMENTATION
        || IsExtBufferIgnoredInRuntime(id);
}

mfxStatus CheckExtBufferHeaders(mfxU16 numExtParam, mfxExtBuffer** extParam, bool isRuntime)
{
    for (mfxU16 i = 0; i < numExtParam; i++)
    {
        MFX_CHECK_NULL_PTR1(extParam);

        mfxExtBuffer *pBuf = extParam[i];

        // check that NumExtParam complies with ExtParam
        MFX_CHECK_NULL_PTR1(extParam[i]);

        // check that there is no ext buffer duplication in ExtParam
        for (mfxU16 j = i + 1; j < numExtParam; j++)
        {
            if (extParam[j]->BufferId == pBuf->BufferId)
            {
                return MFX_ERR_UNDEFINED_BEHAVIOR;
            }
        }

        bool isSupported = isRuntime ?
            IsExtBufferSupportedInRuntime(pBuf->BufferId) : IsExtBufferSupportedInInit(pBuf->BufferId);

        if (!isSupported)
        {
            return MFX_ERR_UNSUPPORTED;
        }
    }

    return MFX_ERR_NONE;
}

// below code allows to do 3 things:
// 1) Set 1 to parameters which are configurable (supported) by the encoder.
// 2) Copy only supported parameters:
//    a) During input paramerets validation to check if all parameters passed by app are supported.
//    b) In Reset() API function to inherrit all defaults from configuration that was actual prior to Reset() call.
#define COPY_PAR_IF_ZERO(pDst, pSrc, PAR)\
if (pDst->PAR == 0)\
{\
    pDst->PAR = pSrc->PAR;\
}

#define SET_OR_COPY_PAR(PAR)\
if (pSrc)\
{\
    COPY_PAR_IF_ZERO(pDst, pSrc, PAR); \
}\
else\
{\
    pDst->PAR = 1; \
}

#define SET_OR_COPY_PAR_DONT_INHERIT(PAR)\
if (pSrc)\
{\
    if (zeroDst)\
    {\
        COPY_PAR_IF_ZERO(pDst, pSrc, PAR); \
    }\
}\
else\
{\
    pDst->PAR = 1; \
}

#define COPY_PTR(PTR)\
if (pSrc)\
{\
    COPY_PAR_IF_ZERO(pDst, pSrc, PTR); \
}\

inline void SetOrCopy(mfxInfoMFX *pDst, mfxInfoMFX const *pSrc = 0, bool /*zeroDst*/ = true)
{
    SET_OR_COPY_PAR(FrameInfo.Width);
    SET_OR_COPY_PAR(FrameInfo.Height);
    SET_OR_COPY_PAR(FrameInfo.CropW);
    SET_OR_COPY_PAR(FrameInfo.CropH);
    SET_OR_COPY_PAR(FrameInfo.CropX);
    SET_OR_COPY_PAR(FrameInfo.CropY);
    SET_OR_COPY_PAR(FrameInfo.FrameRateExtN);
    SET_OR_COPY_PAR(FrameInfo.FrameRateExtD);
    SET_OR_COPY_PAR(FrameInfo.AspectRatioW);
    SET_OR_COPY_PAR(FrameInfo.AspectRatioH);
    SET_OR_COPY_PAR(FrameInfo.ChromaFormat);
    SET_OR_COPY_PAR(FrameInfo.BitDepthLuma);
    SET_OR_COPY_PAR(FrameInfo.BitDepthChroma);
    SET_OR_COPY_PAR(FrameInfo.Shift);
    SET_OR_COPY_PAR(FrameInfo.PicStruct);
    SET_OR_COPY_PAR(FrameInfo.FourCC);
    SET_OR_COPY_PAR(NumThread);
    SET_OR_COPY_PAR(LowPower);
    SET_OR_COPY_PAR(CodecId);
    SET_OR_COPY_PAR(CodecProfile);
    SET_OR_COPY_PAR(TargetUsage);
    SET_OR_COPY_PAR(GopPicSize);
    SET_OR_COPY_PAR(GopRefDist);
    SET_OR_COPY_PAR(RateControlMethod);
    SET_OR_COPY_PAR(BufferSizeInKB);
    SET_OR_COPY_PAR(InitialDelayInKB);
    SET_OR_COPY_PAR(QPI);
    SET_OR_COPY_PAR(TargetKbps);
    SET_OR_COPY_PAR(MaxKbps);
    SET_OR_COPY_PAR(BRCParamMultiplier);
    SET_OR_COPY_PAR(NumRefFrame);
}

inline void SetOrCopy(mfxExtVP9Param *pDst, mfxExtVP9Param const *pSrc = 0, bool zeroDst = true)
{
    SET_OR_COPY_PAR_DONT_INHERIT(FrameWidth);
    SET_OR_COPY_PAR_DONT_INHERIT(FrameHeight);

    SET_OR_COPY_PAR(WriteIVFHeaders);

    /*
    for (mfxU8 i = 0; i < MAX_REF_LF_DELTAS; i++)
    {
        SET_OR_COPY_PAR(LoopFilterRefDelta[i]);
    }
    for (mfxU8 i = 0; i < MAX_MODE_LF_DELTAS; i++)
    {
        SET_OR_COPY_PAR(LoopFilterModeDelta[i]);
    }
    */

    SET_OR_COPY_PAR(QIndexDeltaLumaDC);
    SET_OR_COPY_PAR(QIndexDeltaChromaAC);
    SET_OR_COPY_PAR(QIndexDeltaChromaDC);

#if (MFX_VERSION >= 1029)
    SET_OR_COPY_PAR(NumTileRows);
    SET_OR_COPY_PAR(NumTileColumns);
#endif
}

inline void SetOrCopy(mfxExtCodingOption2 *pDst, mfxExtCodingOption2 const *pSrc = 0, bool /*zeroDst*/ = true)
{
    SET_OR_COPY_PAR(MBBRC);
}

inline void SetOrCopy(mfxExtCodingOption3 *pDst, mfxExtCodingOption3 const *pSrc = 0, bool /*zeroDst*/ = true)
{
    (void)pSrc;
    (void)pDst;

#if (MFX_VERSION >= 1027)
    SET_OR_COPY_PAR(TargetChromaFormatPlus1);
    SET_OR_COPY_PAR(TargetBitDepthLuma);
    SET_OR_COPY_PAR(TargetBitDepthChroma);
    COPY_PTR(ScenarioInfo);
#endif
}

inline void SetOrCopy(mfxExtVP9Segmentation *pDst, mfxExtVP9Segmentation const *pSrc = 0, bool zeroDst = true)
{
    SET_OR_COPY_PAR_DONT_INHERIT(NumSegments);
    SET_OR_COPY_PAR(SegmentIdBlockSize);
    SET_OR_COPY_PAR(NumSegmentIdAlloc);

    for (mfxU8 i = 0; i < MAX_SEGMENTS; i++)
    {
        SET_OR_COPY_PAR_DONT_INHERIT(Segment[i].FeatureEnabled);
        SET_OR_COPY_PAR_DONT_INHERIT(Segment[i].ReferenceFrame);
        SET_OR_COPY_PAR_DONT_INHERIT(Segment[i].LoopFilterLevelDelta);
        SET_OR_COPY_PAR_DONT_INHERIT(Segment[i].QIndexDelta);
    }

    COPY_PTR(SegmentId);
}

inline void SetOrCopy(mfxExtVP9TemporalLayers *pDst, mfxExtVP9TemporalLayers const *pSrc = 0, bool zeroDst = true)
{
    for (mfxU8 i = 0; i < MAX_NUM_TEMP_LAYERS; i++)
    {
        SET_OR_COPY_PAR_DONT_INHERIT(Layer[i].FrameRateScale);
        SET_OR_COPY_PAR_DONT_INHERIT(Layer[i].TargetKbps);
    }
}

inline void SetOrCopy(mfxExtCodingOptionDDI *pDst, mfxExtCodingOptionDDI const *pSrc = 0, bool /*zeroDst*/ = true)
{
    SET_OR_COPY_PAR(RefreshFrameContext);
    SET_OR_COPY_PAR(ChangeFrameContextIdxForTS);
    SET_OR_COPY_PAR(SuperFrameForTS);
}

template<class T>
inline mfxStatus SetOrCopySupportedParams(T* pDst, T const *pSrc = 0, bool zeroDst = true)
{
    MFX_CHECK_NULL_PTR1(pDst);

    if (zeroDst)
    {
        ZeroExtBuffer(*pDst);
    }

    SetOrCopy(pDst, pSrc, zeroDst);

    return MFX_ERR_NONE;
}

template<>
inline mfxStatus SetOrCopySupportedParams<mfxInfoMFX>(mfxInfoMFX* pDst, mfxInfoMFX const *pSrc, bool zeroDst)
{
    MFX_CHECK_NULL_PTR1(pDst);

    if (zeroDst)
    {
        Zero(*pDst);
    }

    SetOrCopy(pDst, pSrc, zeroDst);

    return MFX_ERR_NONE;
}

#define SET_SUPPORTED_PARAMS(type)      \
{                                       \
    type * pExt = GetExtBuffer(par);    \
    if (pExt != 0)                      \
    {                                   \
        SetOrCopySupportedParams(pExt); \
    }                                   \
}

mfxStatus SetSupportedParameters(mfxVideoParam & par)
{
    par.AsyncDepth = 1;
    par.IOPattern = 1;
    par.Protected = 0;
    memset(par.reserved, 0, sizeof(par.reserved));
    memset(&par.reserved2, 0, sizeof(par.reserved2));
    memset(&par.reserved3, 0, sizeof(par.reserved3));

    // mfxInfoMfx
    SetOrCopySupportedParams(&par.mfx);

    // extended buffers
    mfxStatus sts = CheckExtBufferHeaders(par.NumExtParam, par.ExtParam);
    MFX_CHECK_STS(sts);

    SET_SUPPORTED_PARAMS(mfxExtVP9Param);
    SET_SUPPORTED_PARAMS(mfxExtCodingOption2);
    SET_SUPPORTED_PARAMS(mfxExtCodingOption3);
    SET_SUPPORTED_PARAMS(mfxExtVP9TemporalLayers);

    return MFX_ERR_NONE;
}

struct Bool
{
    Bool(bool initValue) : value(initValue) {};
    Bool & operator=(bool newValue)
    {
        value = newValue;
        return *this;
    };

    bool operator==(bool valueToCheck)
    {
        return (value == valueToCheck);
    };

    bool value;
};

bool CheckTriStateOption(mfxU16 & opt)
{
    if (opt != MFX_CODINGOPTION_UNKNOWN &&
        opt != MFX_CODINGOPTION_ON &&
        opt != MFX_CODINGOPTION_OFF)
    {
        opt = MFX_CODINGOPTION_UNKNOWN;
        return false;
    }

    return true;
}

template <class T, class U>
bool CheckRange(T & opt, U min, U max)
{
    if (opt < min)
    {
        opt = static_cast<T>(min);
        return false;
    }

    if (opt > max)
    {
        opt = static_cast<T>(max);
        return false;
    }

    return true;
}

template <class T, class U>
bool CheckRangeDflt(T & opt, U min, U max, U deflt)
{
    if (opt < static_cast<T>(min) || opt > static_cast<T>(max))
    {
        opt = static_cast<T>(deflt);
        return false;
    }

    return true;
}

#define INHERIT_DEFAULTS(type, zeroDst)            \
{                                                  \
    type& dst = GetExtBufferRef(defaultsDst);      \
    type& src = GetExtBufferRef(defaultsSrc);      \
    SetOrCopySupportedParams(&dst, &src, zeroDst); \
}

void InheritDefaults(VP9MfxVideoParam& defaultsDst, VP9MfxVideoParam const & defaultsSrc)
{
    // inherit default from mfxInfoMfx
    SetOrCopySupportedParams(&defaultsDst.mfx, &defaultsSrc.mfx, false);

    INHERIT_DEFAULTS(mfxExtVP9Param, false);
    INHERIT_DEFAULTS(mfxExtCodingOption2, false);
    INHERIT_DEFAULTS(mfxExtCodingOption3, false);

    mfxExtVP9Segmentation* pSegDst = GetExtBuffer(defaultsDst);
    mfxExtVP9Segmentation* pSegSrc = GetExtBuffer(defaultsSrc);
    if (defaultsDst.m_segBufPassed == true)
    {
        mfxU16 numSegmentsDst = pSegDst->NumSegments;
        if (numSegmentsDst != 0)
        {
            // NumSegments set to 0 means disabling of segmentation
            // so there is no need to inherit any segmentation paramaters
            SetOrCopySupportedParams(pSegDst, pSegSrc, false);
        }
    }
    else
    {
        SetOrCopySupportedParams(pSegDst, pSegSrc);
    }

    INHERIT_DEFAULTS(mfxExtVP9TemporalLayers, !defaultsDst.m_tempLayersBufPassed);
    INHERIT_DEFAULTS(mfxExtCodingOptionDDI, false);
}

#define CLEAN_OUT_NONCONFIGURABLE_PARAMETERS(type)  \
{                                               \
    type& tmpBuf = GetExtBufferRef(tmp);        \
    type& inBuf = GetExtBufferRef(par);         \
    SetOrCopySupportedParams(&inBuf, &tmpBuf);  \
    if (memcmp(&inBuf, &tmpBuf, sizeof(type)))  \
    {                                           \
        sts = MFX_ERR_UNSUPPORTED;              \
    }                                           \
}


mfxStatus CleanOutNonconfigurableParameters(VP9MfxVideoParam &par)
{
    VP9MfxVideoParam tmp = par;
    mfxStatus sts = SetOrCopySupportedParams(&par.mfx, &tmp.mfx);
    MFX_CHECK_STS(sts);
    if (memcmp(&par.mfx, &tmp.mfx, sizeof(mfxInfoMFX)))
    {
        sts = MFX_ERR_UNSUPPORTED;
    }

    CLEAN_OUT_NONCONFIGURABLE_PARAMETERS(mfxExtVP9Param);
    CLEAN_OUT_NONCONFIGURABLE_PARAMETERS(mfxExtCodingOption2);
    CLEAN_OUT_NONCONFIGURABLE_PARAMETERS(mfxExtCodingOption3);
    CLEAN_OUT_NONCONFIGURABLE_PARAMETERS(mfxExtVP9Segmentation);
    CLEAN_OUT_NONCONFIGURABLE_PARAMETERS(mfxExtVP9TemporalLayers);

    return sts;
}

bool IsBrcModeSupported(mfxU16 brc)
{
    return brc == MFX_RATECONTROL_CBR
        || brc == MFX_RATECONTROL_VBR
        || brc == MFX_RATECONTROL_CQP
        || brc == MFX_RATECONTROL_ICQ;
}

bool IsChromaFormatSupported(mfxU16 profile, mfxU16 chromaFormat)
{
    switch (profile)
    {
    case MFX_PROFILE_VP9_0:
    case MFX_PROFILE_VP9_2:
        return chromaFormat == MFX_CHROMAFORMAT_YUV420;
    case MFX_PROFILE_VP9_1:
    case MFX_PROFILE_VP9_3:
        return chromaFormat == MFX_CHROMAFORMAT_YUV420 ||
            chromaFormat == MFX_CHROMAFORMAT_YUV444;
    }

    return false;
}

bool IsBitDepthSupported(mfxU16 profile, mfxU16 bitDepth)
{
    switch (profile)
    {
    case MFX_PROFILE_VP9_0:
    case MFX_PROFILE_VP9_1:
        return bitDepth == BITDEPTH_8;
    case MFX_PROFILE_VP9_2:
    case MFX_PROFILE_VP9_3:
        return bitDepth == 8 ||
            bitDepth == BITDEPTH_10;
    }

    return false;
}

mfxU16 GetChromaFormat(mfxU32 fourcc)
{
    switch (fourcc)
    {
    case MFX_FOURCC_NV12:
    case MFX_FOURCC_P010:
        return MFX_CHROMAFORMAT_YUV420;
    case MFX_FOURCC_AYUV:
#if (MFX_VERSION >= 1027)
    case MFX_FOURCC_Y410:
#endif
        return MFX_CHROMAFORMAT_YUV444;
    default:
        return 0;
    }
}

mfxU16 GetBitDepth(mfxU32 fourcc)
{
    switch (fourcc)
    {
    case MFX_FOURCC_NV12:
    case MFX_FOURCC_AYUV:
        return BITDEPTH_8;
#if (MFX_VERSION >= 1027)
    case MFX_FOURCC_Y410:
#endif
    case MFX_FOURCC_P010:
        return BITDEPTH_10;
    default:
        return 0;
    }
}

// check bit depth itself and it's compliance with fourcc
bool CheckBitDepth(mfxU16 depth, mfxU32 fourcc)
{
    if (depth != 0 &&
        ((depth != BITDEPTH_8 && depth != BITDEPTH_10)
        || (fourcc != 0 && depth != GetBitDepth(fourcc))))
    {
        return false;
    }

    return true;
}

// check chroma format itself and it's compliance with fourcc
bool CheckChromaFormat(mfxU16 format, mfxU32 fourcc)
{
    if ((format != MFX_CHROMAFORMAT_YUV420 && format != MFX_CHROMAFORMAT_YUV444)
        || (fourcc != 0 && format != GetChromaFormat(fourcc)))
    {
        return false;
    }

    return true;
}

bool CheckFourcc(mfxU32 fourcc, ENCODE_CAPS_VP9 const &caps)
{
    return fourcc == MFX_FOURCC_NV12 || (fourcc == MFX_FOURCC_AYUV && caps.YUV444ReconSupport)  // 8 bit
        || (caps.MaxEncodedBitDepth > 0 && (fourcc == MFX_FOURCC_P010  // 10 bit
#if (MFX_VERSION >= 1027)
            || (fourcc == MFX_FOURCC_Y410 && caps.YUV444ReconSupport)
#endif
            ));
}

mfxU16 MapTUToSupportedRange(mfxU16 tu)
{
    switch (tu)
    {
    case MFX_TARGETUSAGE_1:
    case MFX_TARGETUSAGE_2:
        return MFX_TARGETUSAGE_BEST_QUALITY;
    case MFX_TARGETUSAGE_3:
    case MFX_TARGETUSAGE_4:
    case MFX_TARGETUSAGE_5:
        return MFX_TARGETUSAGE_BALANCED;
    case MFX_TARGETUSAGE_6:
    case MFX_TARGETUSAGE_7:
        return MFX_TARGETUSAGE_BEST_SPEED;
    default:
        return MFX_TARGETUSAGE_UNKNOWN;
    }
}

inline bool IsFeatureSupported(ENCODE_CAPS_VP9 const & caps, mfxU8 feature)
{
    return (caps.SegmentFeatureSupport & (1 << feature)) != 0;
}

inline void Disable(mfxU16& features, mfxU8 feature)
{
    features &= ~(1 << feature);
}

inline bool CheckFeature(mfxU16& features, mfxI16* featureValue, mfxU8 feature, ENCODE_CAPS_VP9 const & caps)
{
    bool status = true;
    // check QIndex feature
    if (IsFeatureEnabled(features, feature) && !IsFeatureSupported(caps, feature))
    {
        Disable(features, feature);
        status = false;
    }

    if (featureValue && *featureValue && !IsFeatureEnabled(features, feature))
    {
        *featureValue = 0;
        status = false;
    }

    return status;
}

bool CheckAndFixQIndexDelta(mfxI16& qIndexDelta, mfxU16 qIndex)
{
    mfxI16 minQIdxDelta = qIndex ? 1 - qIndex : 1 - MAX_Q_INDEX;
    mfxI16 maxQIdxDelta = MAX_Q_INDEX - qIndex;

    // if Q index is OK, but Q index value + delta is out of valid range - clamp Q index delta
    return Clamp(qIndexDelta, minQIdxDelta, maxQIdxDelta);
}

mfxStatus CheckPerSegmentParams(mfxVP9SegmentParam& segPar, ENCODE_CAPS_VP9 const & caps, mfxInfoMFX &video_par, mfxEncodeCtrl *ctrl_par = nullptr)
{
    Bool changed = false;
    mfxU16& features = segPar.FeatureEnabled;

    if (video_par.RateControlMethod == MFX_RATECONTROL_CQP) {
        // check QIndex feature
        if (false == CheckFeature(features, &segPar.QIndexDelta, FEAT_QIDX, caps))
        {
            changed = true;
        }

        // if delta Q index value is out of valid range - just ignore it
        if (false == CheckRangeDflt(segPar.QIndexDelta, -MAX_Q_INDEX, MAX_Q_INDEX, 0))
        {
            changed = true;
        }
        else
        {
            // if delta Q index value is OK, but Q index value + delta is out of valid range - clamp Q index delta
            if (false == CheckAndFixQIndexDelta(segPar.QIndexDelta, video_par.QPI))
            {
                changed = true;
            }

            // if delta Q index value is OK, but Q index value + global QP + frame QP delta is out of valid range - clamp Q index delta
            if (ctrl_par)
            {
                mfxExtVP9Param* pExtParRuntime = GetExtBuffer(*ctrl_par);
                if (pExtParRuntime)
                {
                    if (!CheckAndFixQIndexDelta(segPar.QIndexDelta, video_par.QPI + pExtParRuntime->QIndexDeltaLumaDC) ||
                        !CheckAndFixQIndexDelta(segPar.QIndexDelta, video_par.QPI + pExtParRuntime->QIndexDeltaChromaAC) ||
                        !CheckAndFixQIndexDelta(segPar.QIndexDelta, video_par.QPI + pExtParRuntime->QIndexDeltaChromaDC))
                    {
                        changed = true;
                    }
                }
            }
        }
    }

    // check LF Level feature
    if (false == CheckFeature(features, &segPar.LoopFilterLevelDelta, FEAT_LF_LVL, caps))
    {
        changed = true;
    }

    if (false == CheckRangeDflt(segPar.LoopFilterLevelDelta, -MAX_LF_LEVEL, MAX_LF_LEVEL, 0))
    {
        changed = true;
    }

    // check reference feature
    if (false == CheckFeature(features, reinterpret_cast<mfxI16*>(&segPar.ReferenceFrame), FEAT_REF, caps))
    {
        changed = true;
    }

    if (segPar.ReferenceFrame &&
        segPar.ReferenceFrame != MFX_VP9_REF_INTRA &&
        segPar.ReferenceFrame != MFX_VP9_REF_LAST &&
        segPar.ReferenceFrame != MFX_VP9_REF_GOLDEN &&
        segPar.ReferenceFrame != MFX_VP9_REF_ALTREF)
    {
        segPar.ReferenceFrame = 0;
        changed = true;
    }

    // check skip feature
    if (false == CheckFeature(features, 0, FEAT_SKIP, caps))
    {
        changed = true;
    }

    return (changed == true) ?
        MFX_WRN_INCOMPATIBLE_VIDEO_PARAM : MFX_ERR_NONE;
}

mfxStatus CheckSegmentationMap(mfxU8 const * segMap, mfxU32 numSegmentIdAlloc, mfxU16 numSegments)
{
    for (mfxU32 i = 0; i < numSegmentIdAlloc; i++)
    {
        if (segMap[i] >= numSegments)
        {
            return MFX_ERR_UNSUPPORTED;
        }
    }

    return MFX_ERR_NONE;
}

inline mfxStatus GetCheckStatus(Bool& changed, Bool& unsupported)
{
    if (unsupported == true)
    {
        return MFX_ERR_UNSUPPORTED;
    }

    return (changed == true) ? MFX_WRN_INCOMPATIBLE_VIDEO_PARAM : MFX_ERR_NONE;
}

inline void ConvertStatusToBools(Bool& changed, Bool& unsupported, mfxStatus sts)
{
    if (sts == MFX_WRN_INCOMPATIBLE_VIDEO_PARAM)
    {
        changed = true;
    }
    else if (sts == MFX_ERR_UNSUPPORTED)
    {
        unsupported = true;
    }
}

// if called with only video_par - assumed stream init, if both video_par and ctrl_par - assumed on-frame checking
mfxStatus CheckSegmentationParam(mfxExtVP9Segmentation& seg, mfxU32 frameWidth, mfxU32 frameHeight, ENCODE_CAPS_VP9 const & caps, VP9MfxVideoParam const &par, mfxEncodeCtrl *ctrl_par = nullptr)
{
    Bool changed = false;
    Bool unsupported = false;

    mfxInfoMFX video_par = par.mfx;

    if ((seg.NumSegments != 0 || true == AnyMandatorySegMapParam(seg)) && caps.ForcedSegmentationSupport == 0)
    {
        // driver/HW don't support segmentation
        // set all segmentation parameters to 0
        ZeroExtBuffer(seg);
        unsupported = true;
    }

    if (seg.NumSegments > MAX_SEGMENTS)
    {
        // further parameter check hardly rely on NumSegments. Cannot fix value of NumSegments and then use modified value for checks. Need to drop and return MFX_ERR_UNSUPPORTED
        seg.NumSegments = 0;
        unsupported = true;
    }

    // currently only 64x64 block size for segmentation map supported (HW limitation)
    if (seg.SegmentIdBlockSize && seg.SegmentIdBlockSize != MFX_VP9_SEGMENT_ID_BLOCK_SIZE_64x64)
    {
        seg.SegmentIdBlockSize = 0;
        unsupported = true;
    }

    // check that NumSegmentIdAlloc is enough for given frame resolution and block size
    mfxU16 blockSize = MapIdToBlockSize(seg.SegmentIdBlockSize);
    if (seg.NumSegmentIdAlloc && blockSize && frameWidth && frameHeight)
    {
        mfxU32 widthInBlocks = (frameWidth + blockSize - 1) / blockSize;
        mfxU32 heightInBlocks = (frameHeight + blockSize - 1) / blockSize;
        mfxU32 sizeInBlocks = widthInBlocks * heightInBlocks;

        if (seg.NumSegmentIdAlloc && seg.NumSegmentIdAlloc < sizeInBlocks)
        {
            seg.NumSegmentIdAlloc = 0;
            seg.SegmentIdBlockSize = 0;
            unsupported = true;
        }
    }

    for (mfxU16 i = 0; i < seg.NumSegments; i++)
    {
        mfxStatus sts = CheckPerSegmentParams(seg.Segment[i], caps, video_par, ctrl_par);
        if (sts == MFX_WRN_INCOMPATIBLE_VIDEO_PARAM)
        {
            changed = true;
        }
    }

    // clean out per-segment parameters for segments with numbers exceeding seg.NumSegments
    if (seg.NumSegments)
    {
        for (mfxU16 i = seg.NumSegments; i < MAX_SEGMENTS; i++)
        {
            if (seg.Segment[i].LoopFilterLevelDelta ||
                seg.Segment[i].QIndexDelta ||
                seg.Segment[i].ReferenceFrame)
            {
                Zero(seg.Segment[i]);
                changed = true;
            }
        }
    }

    // check that segmentation map contains only valid segment ids
    if (seg.NumSegments && seg.NumSegmentIdAlloc && seg.SegmentId)
    {
        mfxStatus sts = CheckSegmentationMap(seg.SegmentId, seg.NumSegmentIdAlloc, seg.NumSegments);
        if (sts == MFX_ERR_UNSUPPORTED)
        {
            seg.SegmentId = 0;
            unsupported = true;
        }
    }

    // corner cases checks
    if (seg.NumSegments == 1)
    {
        // corner case: only one segment is set
        // segmentation doesn't make sense here, because one segment covers whole frame and all params can be applied directly to frame
        if (seg.Segment[0].QIndexDelta)
        {
            if (ctrl_par == nullptr)
            {
                // global segmentation
                if (video_par.RateControlMethod == MFX_RATECONTROL_CQP)
                {
                    // CQP: apply segment's QP_delta to global QP and disable segmentation
                    CheckAndFixQIndexDelta(seg.Segment[0].QIndexDelta, video_par.QPI);
                    video_par.QPI += seg.Segment[0].QIndexDelta;
                    CheckAndFixQIndexDelta(seg.Segment[0].QIndexDelta, video_par.QPP);
                    video_par.QPP += seg.Segment[0].QIndexDelta;
                }
                else
                {
                    unsupported = true;
                }
            }
            else
            {
                // on-frame segmentation
                unsupported = true;
            }

            seg.Segment[0].QIndexDelta = 0;
        }

        seg.NumSegments = 0;

        changed = true;
    }
    else if (seg.NumSegments > 1)
    {
        // corner case: several segments are set but no segment params
        // segmentation doesn't make sense because no actual changes are applied
        Bool enabled_deltas = false;
        for (mfxU16 i = 0; i < seg.NumSegments; i++)
        {
            if (seg.Segment[i].QIndexDelta || seg.Segment[i].LoopFilterLevelDelta || seg.Segment[i].FeatureEnabled || seg.Segment[i].ReferenceFrame)
            {
                enabled_deltas = true;
                break;
            }
        }
        if (enabled_deltas == false)
        {
            seg.NumSegments = 0;
            changed = true;
        }
    }

    return GetCheckStatus(changed, unsupported);
}

mfxStatus CheckTemporalLayers(VP9MfxVideoParam& par, ENCODE_CAPS_VP9 const & caps)
{
    Bool changed = false;
    Bool unsupported = false;

    if (par.m_layerParam[0].Scale && par.m_layerParam[0].Scale != 1)
    {
        par.m_layerParam[0].Scale = 0;
        unsupported = true;
    }

    mfxU8 i = 0;
    mfxU16 prevScale = 0;
    mfxU32 prevBitrate = 0;
    par.m_numLayers = 0;
    bool gapsInScales = false;

    for (; i < MAX_NUM_TEMP_LAYERS_SUPPORTED; i++)
    {
        mfxU16& scale = par.m_layerParam[i].Scale;
        mfxU32& bitrate = par.m_layerParam[i].targetKbps;

        if (scale)
        {
            if (prevScale && (scale <= prevScale || scale % prevScale))
            {
                scale = 0;
                unsupported = true;
            }
            else
            {
                prevScale = scale;
            }
        }
        else
        {
            gapsInScales = true;
        }

        if (bitrate)
        {
            if (par.mfx.RateControlMethod &&
                false == IsBitrateBasedBRC(par.mfx.RateControlMethod))
            {
                bitrate = 0;
                changed = true;
            }
            else
            {
                if (caps.TemporalLayerRateCtrl == 0 || bitrate <= prevBitrate)
                {
                    bitrate = 0;
                    unsupported = true;
                }
                else
                {
                    prevBitrate = bitrate;
                }
            }
        }

        if (scale && !gapsInScales)
        {
            par.m_numLayers++;
        }
    }

    if (unsupported == true)
    {
        par.m_numLayers = 0;
    }

    if (par.m_numLayers)
    {
        mfxU16 temporalCycleLenght = par.m_layerParam[par.m_numLayers - 1].Scale;
        if (par.mfx.GopPicSize && par.mfx.GopPicSize < temporalCycleLenght)
        {
            par.m_numLayers = 0;
            i = 0;
            changed = true;
        }
    }

    for (; i < MAX_NUM_TEMP_LAYERS; i++)
    {
        if (par.m_layerParam[i].Scale != 0 ||
            par.m_layerParam[i].targetKbps != 0)
        {
            Zero(par.m_layerParam[i]);
            changed = true;
        }
    }

    return GetCheckStatus(changed, unsupported);
}

inline mfxU16 MinRefsForTemporalLayers(mfxU16 numTL)
{
    if (numTL < 3)
    {
        return 1;
    }
    else
    {
        return numTL - 1;
    }
}

bool CheckAndFixCTQIdxDeltas(mfxExtVP9Param& extPar)
{
    bool isOk = true;

    if (false == CheckRangeDflt(extPar.QIndexDeltaLumaDC, -MAX_ABS_COEFF_TYPE_Q_INDEX_DELTA, MAX_ABS_COEFF_TYPE_Q_INDEX_DELTA, 0)) isOk = false;
    if (false == CheckRangeDflt(extPar.QIndexDeltaChromaAC, -MAX_ABS_COEFF_TYPE_Q_INDEX_DELTA, MAX_ABS_COEFF_TYPE_Q_INDEX_DELTA, 0)) isOk = false;
    if (false == CheckRangeDflt(extPar.QIndexDeltaChromaDC, -MAX_ABS_COEFF_TYPE_Q_INDEX_DELTA, MAX_ABS_COEFF_TYPE_Q_INDEX_DELTA, 0)) isOk = false;

    return isOk;
}

bool CheckAndFixCTQIdxDeltasQPBased(mfxExtVP9Param& extPar, mfxU16 qIndex)
{
    bool isOk = true;

    if (false == CheckAndFixQIndexDelta(extPar.QIndexDeltaLumaDC, qIndex)) isOk = false;
    if (false == CheckAndFixQIndexDelta(extPar.QIndexDeltaChromaAC, qIndex)) isOk = false;
    if (false == CheckAndFixQIndexDelta(extPar.QIndexDeltaChromaDC, qIndex)) isOk = false;

    return isOk;
}

mfxStatus CheckParameters(VP9MfxVideoParam &par, ENCODE_CAPS_VP9 const &caps)
{
    Bool changed = false;
    Bool unsupported = false;

    if (par.mfx.LowPower != MFX_CODINGOPTION_UNKNOWN &&
        par.mfx.LowPower != MFX_CODINGOPTION_ON &&
        par.mfx.LowPower != MFX_CODINGOPTION_OFF)
    {
        par.mfx.LowPower = MFX_CODINGOPTION_ON;
        changed = true;
    }

    if (par.mfx.LowPower == MFX_CODINGOPTION_OFF)
    {
        par.mfx.LowPower = MFX_CODINGOPTION_ON;
    }

    // clean out non-configurable params but do not return any errors on that (ignore mode)
    /*mfxStatus err = */CleanOutNonconfigurableParameters(par);

    if (par.IOPattern &&
        par.IOPattern != MFX_IOPATTERN_IN_VIDEO_MEMORY &&
        par.IOPattern != MFX_IOPATTERN_IN_SYSTEM_MEMORY &&
        par.IOPattern != MFX_IOPATTERN_IN_OPAQUE_MEMORY)
    {
        par.IOPattern = 0;
        unsupported = true;
    }

    if (par.Protected != 0)
    {
        par.Protected = 0;
        unsupported = true;
    }

    // check mfxInfoMfx
    double      frameRate = 0.0;

    mfxFrameInfo& fi = par.mfx.FrameInfo;

    if ((fi.Width & 0x0f) != 0 || (fi.Height & 0x0f) != 0)
    {
        fi.Width = 0;
        fi.Height = 0;
        unsupported = true;
    }

    if (fi.Width > caps.MaxPicWidth)
    {
        fi.Width = 0;
        unsupported = true;
    }

    if (fi.Height > caps.MaxPicHeight)
    {
        fi.Height = 0;
        unsupported = true;
    }
    //VP9 doesn't support CropX, CropY due to absence of syntax in bitstream header
    if ((fi.Width  && (fi.CropW > fi.Width))  ||
        (fi.Height && (fi.CropH > fi.Height)) ||
        fi.CropX ||
        fi.CropY)
    {
        fi.CropW = 0;
        fi.CropH = 0;
        fi.CropX = 0;
        fi.CropY = 0;
        unsupported = true;
    }

    if ((fi.FrameRateExtN == 0) != (fi.FrameRateExtD == 0))
    {
        fi.FrameRateExtN = 0;
        fi.FrameRateExtD = 0;
        unsupported = true;
    }

    if (fi.FrameRateExtD != 0)
    {
        frameRate = (double)fi.FrameRateExtN / (double)fi.FrameRateExtD;
    }

    if (fi.FrameRateExtD != 0 &&
        (frameRate < 1.0 || frameRate > 180.0))
    {
        fi.FrameRateExtN = 0;
        fi.FrameRateExtD = 0;
        unsupported = true;
    }

    if (fi.AspectRatioH != 0 || fi.AspectRatioW != 0)
    {
        if (!(fi.AspectRatioH == 1 && fi.AspectRatioW == 1))
        {
            fi.AspectRatioH = 1;
            fi.AspectRatioW = 1;
            changed = true;
        }
    }

    if (fi.PicStruct > MFX_PICSTRUCT_PROGRESSIVE)
    {
        fi.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
        unsupported = true;
    }

#if (MFX_VERSION >= 1027)
    {
        mfxExtCodingOption3& opt3 = GetExtBufferRef(par);
        mfxU32& fourcc         = fi.FourCC;
        mfxU16& inFormat       = fi.ChromaFormat;
        mfxU16& inDepthLuma    = fi.BitDepthLuma;
        mfxU16& inDepthChroma  = fi.BitDepthChroma;
        mfxU16& profile        = par.mfx.CodecProfile;
        mfxU16& outFormatP1    = opt3.TargetChromaFormatPlus1;
        mfxU16& outDepthLuma   = opt3.TargetBitDepthLuma;
        mfxU16& outDepthChroma = opt3.TargetBitDepthChroma;

        if (fourcc != 0
            && false == CheckFourcc(fourcc, caps))
        {
            fourcc = 0;
            unsupported = true;
        }

        if (false == CheckChromaFormat(inFormat, fourcc))
        {
            inFormat = 0;
            unsupported = true;
        }

        if (false == CheckBitDepth(inDepthLuma, fourcc))
        {
            inDepthLuma = 0;
            unsupported = true;
        }

        if (false == CheckBitDepth(inDepthChroma, fourcc))
        {
            inDepthChroma = 0;
            unsupported = true;
        }

        if (inDepthLuma != 0 && inDepthChroma != 0 &&
            inDepthLuma != inDepthChroma)
        {
            inDepthChroma = 0;
            unsupported = true;
        }

        if (fi.Shift != 0
            && fourcc != MFX_FOURCC_P010
            )
        {
            fi.Shift = 0;
            unsupported = true;
        }

        if (outFormatP1 != 0 && false == CheckChromaFormat(outFormatP1 - 1, fourcc))
        {
            outFormatP1 = 0;
            unsupported = true;
        }

        if (false == CheckBitDepth(outDepthLuma, fourcc))
        {
            outDepthLuma = 0;
            unsupported = true;
        }

        if (false == CheckBitDepth(outDepthChroma, fourcc))
        {
            outDepthChroma = 0;
            unsupported = true;
        }

        if (outDepthLuma != 0 && outDepthChroma != 0 &&
            outDepthLuma != outDepthChroma)
        {
            outDepthChroma = 0;
            unsupported = true;
        }

        // check compliance of profile and chroma format, bit depth
        if (profile != 0)
        {
            if (outFormatP1 != 0 && false == IsChromaFormatSupported(profile, outFormatP1 - 1))
            {
                outFormatP1 = 0;
                unsupported = true;
            }

            if (outDepthLuma != 0 && false == IsBitDepthSupported(profile, outDepthLuma))
            {
                outDepthLuma = 0;
                unsupported = true;
            }

            if (outDepthChroma != 0 && false == IsBitDepthSupported(profile, outDepthChroma))
            {
                outDepthChroma = 0;
                unsupported = true;
            }
        }
    }
#else // MFX_VERSION >= 1027
    if (par.mfx.CodecProfile > MFX_PROFILE_VP9_0)
    {
        par.mfx.CodecProfile = MFX_PROFILE_UNKNOWN;
        unsupported = true;
    }

    if (fi.ChromaFormat > MFX_CHROMAFORMAT_YUV420)
    {
        fi.ChromaFormat = 0;
        unsupported = true;
    }

    if (fi.FourCC != 0 && fi.FourCC != MFX_FOURCC_NV12)
    {
        fi.FourCC = 0;
        unsupported = true;
    }

    if (fi.BitDepthLuma != 0 && fi.BitDepthLuma != 8)
    {
        fi.BitDepthLuma = 0;
        unsupported = true;
    }

    if (fi.BitDepthChroma != 0 && fi.BitDepthChroma != 8)
    {
        fi.BitDepthChroma = 0;
        unsupported = true;
    }

    if (fi.Shift != 0)
    {
        fi.Shift = 0;
        unsupported = true;
    }
#endif // MFX_VERSION >= 1027

    if (par.mfx.NumThread > 1)
    {
        par.mfx.NumThread = 1;
        changed = true;
    }

    if (par.mfx.TargetUsage)
    {
        par.mfx.TargetUsage = MapTUToSupportedRange(par.mfx.TargetUsage);
        if (par.mfx.TargetUsage == MFX_TARGETUSAGE_UNKNOWN)
        {
            unsupported = true;
        }
    }

    if (par.mfx.GopRefDist > 1)
    {
        par.mfx.GopRefDist = 1;
        changed = true;
    }

    mfxStatus tlSts = CheckTemporalLayers(par, caps);
    ConvertStatusToBools(changed, unsupported, tlSts);

    mfxU16& brcMode = par.mfx.RateControlMethod;

    mfxExtCodingOption2& opt2 = GetExtBufferRef(par);

    if (brcMode != 0)
    {
        if (false == IsBrcModeSupported(brcMode))
        {
            brcMode = 0;
            par.m_initialDelayInKb = 0;
            par.m_bufferSizeInKb = 0;
            par.m_targetKbps = 0;
            par.m_maxKbps = 0;
            par.mfx.BRCParamMultiplier = 0;
            unsupported = true;
        }
        else if (IsBitrateBasedBRC(brcMode))
        {
            if (par.m_numLayers)
            {
                const mfxU32 bitrateForTopTempLayer = par.m_layerParam[par.m_numLayers - 1].targetKbps;
                if (bitrateForTopTempLayer && par.m_targetKbps &&
                    bitrateForTopTempLayer != par.m_targetKbps)
                {
                    par.m_targetKbps = bitrateForTopTempLayer;
                    changed = true;
                }
            }

            if ((brcMode == MFX_RATECONTROL_CBR  &&
                par.m_maxKbps && par.m_maxKbps != par.m_targetKbps) ||
                (brcMode == MFX_RATECONTROL_VBR  &&
                    par.m_maxKbps && par.m_maxKbps < par.m_targetKbps))
            {
                par.m_maxKbps = par.m_targetKbps;
                changed = true;
            }

            if (IsBufferBasedBRC(brcMode) && par.m_bufferSizeInKb &&
                par.m_initialDelayInKb > par.m_bufferSizeInKb)
            {
                par.m_initialDelayInKb = 0;
                unsupported = true;
            }
        }
        else if (brcMode == MFX_RATECONTROL_CQP)
        {
            if (par.mfx.QPI > MAX_Q_INDEX)
            {
                par.mfx.QPI = MAX_Q_INDEX;
                changed = true;
            }

            if (par.mfx.QPP > MAX_Q_INDEX)
            {
                par.mfx.QPP = MAX_Q_INDEX;
                changed = true;
            }

            if (par.mfx.QPB > 0)
            {
                par.mfx.QPB = 0;
                changed = true;
            }

            if (par.mfx.BRCParamMultiplier > 1)
            {
                par.mfx.BRCParamMultiplier = 1;
                changed = true;
            }
        }
        else if (brcMode == MFX_RATECONTROL_ICQ)
        {
            if (par.mfx.ICQQuality > MAX_ICQ_QUALITY_INDEX)
            {
                par.mfx.ICQQuality = MAX_ICQ_QUALITY_INDEX;
                changed = true;
            }
        }
    }


    if (par.m_numLayers == 0)
    {
        // VP9 spec allows to store up to 8 reference frames.
        // this VP9 implementation use maximum 3 of 8 so far.
        // we don't need to allocate more if really only 3 are used.
        if (par.mfx.NumRefFrame > DPB_SIZE_REAL)
        {
            par.mfx.NumRefFrame = DPB_SIZE_REAL;
            changed = true;
        }

        // For targetUsage 1: MaxNum_Reference is 3
        // For targetUsage 4: MaxNum_Reference is 2
        // For targetUsage 7: MaxNum_Reference is 1
        int RefActiveP = 3;
        if (par.mfx.TargetUsage)
        {
            if (par.mfx.TargetUsage == MFX_TARGETUSAGE_BALANCED)
            {
                RefActiveP = 2;
            }
            else if (par.mfx.TargetUsage == MFX_TARGETUSAGE_BEST_SPEED)
            {
                RefActiveP = 1;
            }
        }

        if (par.mfx.NumRefFrame > RefActiveP)
        {
            par.mfx.NumRefFrame = RefActiveP;
            changed = true;
        }
    }
    else
    {

        if (par.mfx.NumRefFrame &&
            par.mfx.NumRefFrame < MinRefsForTemporalLayers(par.m_numLayers))
        {
            par.mfx.NumRefFrame = MinRefsForTemporalLayers(par.m_numLayers);
            changed = true;
        }
    }


    mfxExtCodingOptionDDI &opt = GetExtBufferRef(par);
    if (false == CheckTriStateOption(opt.WriteIVFHeaders))
    {
        changed = true;
    }

     mfxExtVP9Param &extPar = GetExtBufferRef(par);

     if (extPar.FrameWidth % 2)
     {
         extPar.FrameWidth = 0;
         unsupported = true;
     }

     if (extPar.FrameHeight % 2)
     {
         extPar.FrameHeight = 0;
         unsupported = true;
     }

     if (extPar.FrameHeight &&
         extPar.FrameHeight > caps.MaxPicHeight)
     {
         extPar.FrameHeight = 0;
         unsupported = true;
     }

     if (extPar.FrameWidth &&
         extPar.FrameWidth > caps.MaxPicWidth)
     {
         extPar.FrameWidth = 0;
         unsupported = true;
     }

     if (extPar.FrameWidth && fi.Width &&
         extPar.FrameWidth > fi.Width)
     {
         extPar.FrameWidth = 0;
         unsupported = true;
     }

     if (extPar.FrameHeight && fi.Height &&
         extPar.FrameHeight > fi.Height)
     {
         extPar.FrameHeight = 0;
         unsupported = true;
     }

     /*if (extPar.FrameWidth &&
         (fi.CropX + fi.CropW > extPar.FrameWidth) ||
         extPar.FrameHeight &&
         (fi.CropY + fi.CropH > extPar.FrameHeight))
     {
         fi.CropX = 0;
         fi.CropW = 0;
         fi.CropY = 0;
         fi.CropH = 0;
         unsupported = true;
     }*/

    if (false == CheckAndFixCTQIdxDeltas(extPar))
    {
        changed = true;
    }
    if (brcMode == MFX_RATECONTROL_CQP)
    {
        if (false == CheckAndFixCTQIdxDeltasQPBased(extPar, par.mfx.QPI))
        {
            changed = true;
        }
        if (false == CheckAndFixCTQIdxDeltasQPBased(extPar, par.mfx.QPP))
        {
            changed = true;
        }
    }

    /*for (mfxU8 i = 0; i < MAX_REF_LF_DELTAS; i++)
    {
        if (false == CheckRangeDflt(extPar.LoopFilterRefDelta[i], -MAX_LF_LEVEL, MAX_LF_LEVEL, 0))
        {
            changed = true;
        }
    }

    for (mfxU8 i = 0; i < MAX_MODE_LF_DELTAS; i++)
    {
        if (false == CheckRangeDflt(extPar.LoopFilterModeDelta[i], -MAX_LF_LEVEL, MAX_LF_LEVEL, 0))
        {
            changed = true;
        }
    }*/

    // check mfxExtCodingOption2
    if (false == CheckTriStateOption(opt2.MBBRC))
    {
        changed = true;
    }

    if (IsOn(opt2.MBBRC) && caps.AutoSegmentationSupport == 0)
    {
        opt2.MBBRC = MFX_CODINGOPTION_OFF;
        unsupported = true;
    }

    if (brcMode == MFX_RATECONTROL_CQP && IsOn(opt2.MBBRC))
    {
        opt2.MBBRC = MFX_CODINGOPTION_OFF;
        changed = true;
    }

    mfxExtOpaqueSurfaceAlloc& opaq = GetExtBufferRef(par);
    if (par.IOPattern &&
        par.IOPattern != MFX_IOPATTERN_IN_OPAQUE_MEMORY
        && opaq.In.NumSurface)
    {
        opaq.In.NumSurface = 0;
        changed = true;
    }

    mfxU16 width = extPar.FrameWidth ? extPar.FrameWidth : fi.Width;
    mfxU16 height = extPar.FrameHeight ? extPar.FrameHeight : fi.Height;

    mfxExtVP9Segmentation& seg = GetExtBufferRef(par);

    mfxStatus segSts = CheckSegmentationParam(seg, width, height, caps, par);
    ConvertStatusToBools(changed, unsupported, segSts);

    if (IsOn(opt2.MBBRC) && seg.NumSegments)
    {
        // explicit segmentation overwrites MBBRC
        opt2.MBBRC = MFX_CODINGOPTION_OFF;
        changed = true;
    }

#if (MFX_VERSION >= 1029)
    mfxU16& rows = extPar.NumTileRows;
    mfxU16& cols = extPar.NumTileColumns;
    if (rows * cols  > 1 && caps.TileSupport == 0)
    {
        rows = cols = 1;
        unsupported = true;
    }

    if (rows && height)
    {
        mfxU16 heightInTiles = static_cast<mfxU16>(CeilDiv(height, MIN_TILE_HEIGHT));
        mfxU16 maxPossibleRows = std::min<mfxU16>(heightInTiles, MAX_NUM_TILE_ROWS);
        if (rows > maxPossibleRows)
        {
            rows = maxPossibleRows;
            changed = true;
        }
        if (rows)
        {
            mfxU16 lowerPowOf2 = (1 << FloorLog2(rows));
            if (rows != lowerPowOf2)
            {
                rows = lowerPowOf2;
                changed = true;
            }
        }
    }

    if (cols && width)
    {
        mfxU16 widthInMinTileCols = static_cast<mfxU16>(CeilDiv(width, MIN_TILE_WIDTH));
        mfxU16 maxPossibleCols = std::min<mfxU16>(widthInMinTileCols, MAX_NUM_TILES);
        if (cols > maxPossibleCols)
        {
            cols = maxPossibleCols;
            changed = true;
        }

        mfxU16 lowerPowOf2 = (1 << FloorLog2(cols));
        mfxU16 correctValue = lowerPowOf2;
        mfxU16 minPossibleCols = static_cast<mfxU16>(CeilDiv(width, MAX_TILE_WIDTH));
        if (cols < minPossibleCols)
        {
            cols = minPossibleCols;
            changed = true;
            mfxU16 higherPowOf2 = (1 << CeilLog2(cols));
            correctValue = higherPowOf2;
        }

        if (cols != correctValue)
        {
            cols = correctValue;
            changed = true;
        }
    }

    mfxU16 numPipes = static_cast<mfxU16>(caps.NumScalablePipesMinus1 + 1);

    if ((cols > numPipes && rows > 1) ||
        (rows * cols > MAX_NUM_TILES))
    {
        cols = 0;
        rows = 0;
        unsupported = true;
    }

    // known limitation: temporal scalability and tiles don't work together
    if (par.m_numLayers > 1 && (rows > 1 || cols > 1))
    {
        rows = cols = 1;
        unsupported = true;
    }
#endif // MFX_VERSION >= 1029

    return GetCheckStatus(changed, unsupported);
}

template <typename T>
inline bool SetDefault(T &par, int defaultValue)
{
    bool defaultSet = false;

    if (par == 0)
    {
        par = (T)defaultValue;
        defaultSet = true;
    }

    return defaultSet;
}

template <typename T>
inline bool SetTwoDefaults(T &par1, T &par2, int defaultValue1, int defaultValue2)
{
    bool defaultsSet = false;

    if (par1 == 0 && par2 == 0)
    {
        par1 = (T)defaultValue1;
        par2 = (T)defaultValue2;
        defaultsSet = true;
    }

    return defaultsSet;
}

inline mfxU32 GetDefaultBufferSize(VP9MfxVideoParam const &par)
{
    if (IsBufferBasedBRC(par.mfx.RateControlMethod))
    {
        const mfxU8 defaultBufferInSec = 2;
        return defaultBufferInSec * ((par.m_targetKbps + 7) / 8);
    }
    else
    {
        const mfxExtVP9Param& extPar = GetExtBufferRef(par);
        if (par.mfx.FrameInfo.FourCC == MFX_FOURCC_P010
#if (MFX_VERSION >= 1027)
            || par.mfx.FrameInfo.FourCC == MFX_FOURCC_Y410
#endif
            ) {
            return (extPar.FrameWidth * extPar.FrameHeight * 3) / 1000; // size of two uncompressed 420 8bit frames in KB
        }
        else
            return (extPar.FrameWidth * extPar.FrameHeight * 3) / 2 / 1000;  // size of uncompressed 420 8bit frame in KB
    }
}

inline mfxU16 GetDefaultAsyncDepth(VP9MfxVideoParam const &/*par*/)
{
    return 2;
}

inline mfxU16 GetMinProfile(mfxU16 depth, mfxU16 format)
{
    return MFX_PROFILE_VP9_0 +
        (depth > 8) * 2 +
        (format > MFX_CHROMAFORMAT_YUV420);
}

void SetDefaultsForProfileAndFrameInfo(VP9MfxVideoParam& par)
{
    mfxFrameInfo& fi = par.mfx.FrameInfo;

    SetDefault(fi.ChromaFormat, GetChromaFormat(fi.FourCC));
    SetDefault(fi.BitDepthLuma, GetBitDepth(fi.FourCC));
    SetDefault(fi.BitDepthChroma, GetBitDepth(fi.FourCC));

#if (MFX_VERSION >= 1027)
    mfxExtCodingOption3 &opt3 = GetExtBufferRef(par);
    SetDefault(opt3.TargetChromaFormatPlus1, fi.ChromaFormat + 1);
    SetDefault(opt3.TargetBitDepthLuma, fi.BitDepthLuma);
    SetDefault(opt3.TargetBitDepthChroma, fi.BitDepthChroma);

    SetDefault(par.mfx.CodecProfile, GetMinProfile(opt3.TargetBitDepthLuma, opt3.TargetChromaFormatPlus1 - 1));
#else //MFX_VERSION >= 1027
    SetDefault(par.mfx.CodecProfile, MFX_PROFILE_VP9_0);
#endif //MFX_VERSION >= 1027
}

#define DEFAULT_GOP_SIZE 0xffff
#define DEFAULT_FRAME_RATE 30

mfxStatus SetDefaults(
    VP9MfxVideoParam &par,
    ENCODE_CAPS_VP9 const &caps)
{
    SetDefault(par.AsyncDepth, GetDefaultAsyncDepth(par));

    // mfxInfoMfx
    SetDefault(par.mfx.TargetUsage, MFX_TARGETUSAGE_BALANCED);
    SetDefault(par.mfx.GopPicSize, DEFAULT_GOP_SIZE);
    SetDefault(par.mfx.GopRefDist, 1);
    SetDefault(par.mfx.NumRefFrame, 1);
    SetDefault(par.mfx.BRCParamMultiplier, 1);
    SetDefault(par.mfx.LowPower, MFX_CODINGOPTION_ON);
    SetDefault(par.mfx.NumThread, 1);
    if (par.mfx.TargetKbps && (par.mfx.TargetKbps < par.mfx.MaxKbps))
    {
        SetDefault(par.mfx.RateControlMethod, MFX_RATECONTROL_VBR);
    }
    else
    {
        SetDefault(par.mfx.RateControlMethod, MFX_RATECONTROL_CBR);
    }

    if (IsBitrateBasedBRC(par.mfx.RateControlMethod))
    {
        if (par.m_numLayers)
        {
            const mfxU32 bitrateForTopTempLayer = par.m_layerParam[par.m_numLayers - 1].targetKbps;
            SetDefault(par.m_targetKbps, bitrateForTopTempLayer);
        }
        SetDefault(par.m_maxKbps, par.m_targetKbps);
    }

    // mfxInfomfx.FrameInfo
    mfxFrameInfo &fi = par.mfx.FrameInfo;
    mfxExtVP9Param& extPar = GetExtBufferRef(par);
    if (extPar.FrameWidth)
    {
        SetDefault(fi.CropW, std::min(fi.Width, extPar.FrameWidth));
    }
    else
    {
        SetDefault(fi.CropW, fi.Width);
        SetDefault(extPar.FrameWidth, fi.CropW);
    }
    if (extPar.FrameHeight)
    {
        SetDefault(fi.CropH, std::min(fi.Height, extPar.FrameHeight));
    }
    else
    {
        SetDefault(fi.CropH, fi.Height);
        SetDefault(extPar.FrameHeight, fi.CropH);
    }

    SetDefault(par.m_bufferSizeInKb, GetDefaultBufferSize(par));
    if (IsBufferBasedBRC(par.mfx.RateControlMethod))
    {
        SetDefault(par.m_initialDelayInKb, par.m_bufferSizeInKb / 2);
    }

    if (par.mfx.RateControlMethod == MFX_RATECONTROL_CQP)
    {
        SetDefault(par.mfx.QPI, 128);
        SetDefault(par.mfx.QPP, par.mfx.QPI + 5);
    }

    mfxExtCodingOption2& opt2 = GetExtBufferRef(par);
    /*mfxExtVP9Segmentation& seg = GetExtBufferRef(par);
    if (par.mfx.RateControlMethod != MFX_RATECONTROL_CQP &&
        caps.AutoSegmentationSupport && !AllMandatorySegMapParams(seg))
    {
        SetDefault(opt2.MBBRC, MFX_CODINGOPTION_ON);
    }
    else
    {
        SetDefault(opt2.MBBRC, MFX_CODINGOPTION_OFF);
    }*/

    // by default disable auto-segmentation (aka MBBRC) for all configurations and platforms
    SetDefault(opt2.MBBRC, MFX_CODINGOPTION_OFF);

    if (false == SetTwoDefaults(fi.FrameRateExtN, fi.FrameRateExtD, 30, 1))
    {
        SetDefault(fi.FrameRateExtN, fi.FrameRateExtD * DEFAULT_FRAME_RATE);
        SetDefault(fi.FrameRateExtD,
            fi.FrameRateExtN % DEFAULT_FRAME_RATE == 0 ? fi.FrameRateExtN / DEFAULT_FRAME_RATE : 1);
    }
    SetTwoDefaults(fi.AspectRatioW, fi.AspectRatioH, 1, 1);
    SetDefault(fi.PicStruct, MFX_PICSTRUCT_PROGRESSIVE);

    // profile, chroma format, bit depth
    SetDefaultsForProfileAndFrameInfo(par);

#if (MFX_VERSION >= 1029)
    SetDefault(extPar.NumTileColumns, (extPar.FrameWidth + MAX_TILE_WIDTH - 1) / MAX_TILE_WIDTH);
    SetDefault(extPar.NumTileRows, 1);
#endif // (MFX_VERSION >= 1029)

    // ext buffers
    // TODO: uncomment when buffer mfxExtVP9CodingOption will be added to API
    // mfxExtVP9CodingOption &opt = GetExtBufferRef(par);
    mfxExtCodingOptionDDI &opt = GetExtBufferRef(par);
    SetDefault(opt.WriteIVFHeaders, MFX_CODINGOPTION_ON);

    // check final set of parameters
    mfxStatus sts = CheckParameters(par, caps);
    MFX_CHECK_STS(sts);

    return MFX_ERR_NONE;
}

mfxStatus CheckParametersAndSetDefaults(
    VP9MfxVideoParam &par,
    ENCODE_CAPS_VP9 const &caps)
{
    mfxStatus sts = MFX_ERR_NONE;

    // check parameters defined by application
    mfxStatus checkSts = MFX_ERR_NONE;
    checkSts = CheckParameters(par, caps);
    if (checkSts == MFX_ERR_UNSUPPORTED)
    {
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    // check presence of mandatory parameters
    // (1) resolution
    mfxFrameInfo const &fi = par.mfx.FrameInfo;
    if (fi.Width == 0 || fi.Height == 0)
    {
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    // (2) fourcc
    if (fi.FourCC == 0)
    {
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    // (3) target bitrate
    if ((IsBitrateBasedBRC(par.mfx.RateControlMethod)
        || par.mfx.RateControlMethod == 0)
        && par.m_numLayers == 0
        && par.m_targetKbps == 0)
    {
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    // (4) crops
    if ((fi.CropW == 0) != (fi.CropH == 0))
    {
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }
    mfxExtVP9Param& extPar = GetExtBufferRef(par);
    if ((extPar.FrameWidth == 0) != (extPar.FrameHeight == 0))
    {
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }
    // (5) opaque memory allocation
    if (par.IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY)
    {
        mfxExtOpaqueSurfaceAlloc &opaq = GetExtBufferRef(par);
        if (opaq.In.NumSurface == 0 ||
            opaq.In.Surfaces == 0 ||
            (opaq.In.Type & MFX_MEMTYPE_SYS_OR_D3D) == 0)
        {
            return MFX_ERR_INVALID_VIDEO_PARAM;
        }
    }

    // (6) Mandatory segmentation parameters
    mfxExtVP9Segmentation const & seg = GetExtBufferRef(par);
    if (AnyMandatorySegMapParam(seg) && seg.NumSegments != 0 && !AllMandatorySegMapParams(seg))
    {
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    // (7) At least one valid layer for temporal scalability
    if (par.m_tempLayersBufPassed && par.m_numLayers == 0)
    {
        mfxExtVP9TemporalLayers const & tl = GetExtBufferRef(par);
        for (mfxU8 i = 0; i < MAX_NUM_TEMP_LAYERS_SUPPORTED; i++)
        {
            if (tl.Layer[i].FrameRateScale != 0)
            {
                return MFX_ERR_INVALID_VIDEO_PARAM;
            }
        }
    }

    // (8) Bitrates for all temporal layers in case of bitrate-based BRC
    if (par.m_numLayers &&
        (IsBitrateBasedBRC(par.mfx.RateControlMethod) || par.mfx.RateControlMethod == 0))
    {
        for (mfxU8 i = 0; i < par.m_numLayers; i++)
        {
            if (par.m_layerParam[i].targetKbps == 0)
            {
                return MFX_ERR_INVALID_VIDEO_PARAM;
            }
        }
    }


    // check non-mandatory parameters which require return of WARNING if not set
    if (par.mfx.RateControlMethod == MFX_RATECONTROL_CQP &&
        (par.mfx.QPI == 0 || par.mfx.QPP == 0))
    {
        checkSts = MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
    }

    if (IsBitrateBasedBRC(par.mfx.RateControlMethod) &&
        par.m_numLayers && par.m_targetKbps == 0)
    {
        checkSts = MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
    }

    // set defaults for parameters not defined by application
    sts = SetDefaults(par, caps);
    MFX_CHECK(sts >= 0, sts);

    // during parameters validation we worked with internal parameter versions
    // now need to update external (API) versions of these parameters
    par.SyncInternalParamToExternal();

    return checkSts;
}

mfxStatus CheckSurface(
    VP9MfxVideoParam const & video,
    mfxFrameSurface1 const & surface,
    mfxU32 initWidth,
    mfxU32 initHeight,
    ENCODE_CAPS_VP9 const &caps)
{
    mfxExtOpaqueSurfaceAlloc const & extOpaq = GetExtBufferRef(video);
    bool isOpaq = video.IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY && extOpaq.In.NumSurface > 0;

    // check that surface contains valid data
    MFX_CHECK(CheckFourcc(surface.Info.FourCC, caps), MFX_ERR_INVALID_VIDEO_PARAM);

    if (video.m_inMemType == INPUT_SYSTEM_MEMORY)
    {
        MFX_CHECK(!LumaIsNull(&surface) || surface.Data.MemId, MFX_ERR_NULL_PTR);
#if (MFX_VERSION >= 1027)
        if (surface.Info.FourCC != MFX_FOURCC_Y410)
#endif
        {
            MFX_CHECK(surface.Data.U != 0 || (surface.Data.MemId && LumaIsNull(&surface)), MFX_ERR_NULL_PTR);
            MFX_CHECK(surface.Data.V != 0 || (surface.Data.MemId && LumaIsNull(&surface)), MFX_ERR_NULL_PTR);
        }
    }
    else if (isOpaq == false)
    {
        MFX_CHECK(surface.Data.MemId != 0, MFX_ERR_INVALID_VIDEO_PARAM);
    }

    const mfxExtVP9Param& extPar = GetExtBufferRef(video);
    MFX_CHECK(surface.Info.Width >= extPar.FrameWidth, MFX_ERR_INVALID_VIDEO_PARAM);
    MFX_CHECK(surface.Info.Height >= extPar.FrameHeight, MFX_ERR_INVALID_VIDEO_PARAM);
    MFX_CHECK(surface.Info.Width <= initWidth, MFX_ERR_INVALID_VIDEO_PARAM);
    MFX_CHECK(surface.Info.Height <= initHeight, MFX_ERR_INVALID_VIDEO_PARAM);

    return MFX_ERR_NONE;
}

mfxStatus CheckAndFixCtrl(
    VP9MfxVideoParam const & video,
    mfxEncodeCtrl & ctrl,
    ENCODE_CAPS_VP9 const & caps)
{
    mfxStatus checkSts = MFX_ERR_NONE;

    // check mfxEncodeCtrl for correct parameters
    if (ctrl.QP > 255)
    {
        ctrl.QP = 255;
        checkSts = MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
    }

    if (ctrl.FrameType > MFX_FRAMETYPE_P)
    {
        ctrl.FrameType = MFX_FRAMETYPE_P;
        checkSts = MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
    }

    mfxExtVP9Param* pExtParRuntime = GetExtBuffer(ctrl);
    const mfxExtVP9Param& extParInit = GetExtBufferRef(video);

    if (pExtParRuntime)
    {
        if (false == CheckAndFixCTQIdxDeltas(*pExtParRuntime))
        {
            checkSts = MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
        }
        if (false == CheckAndFixCTQIdxDeltasQPBased(*pExtParRuntime, ctrl.QP))
        {
            checkSts = MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
        }

        if ((pExtParRuntime->FrameWidth && pExtParRuntime->FrameWidth != extParInit.FrameWidth) ||
            (pExtParRuntime->FrameHeight && pExtParRuntime->FrameHeight != extParInit.FrameHeight))
        {
            // if set, runtime values of FrameWidth/FrameHeight should be same as static values
            // we cannot just remove whole mfxExtVP9Param since it has other parameters
            // so just return warning to app to notify
            checkSts = MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
        }

        pExtParRuntime->FrameWidth = extParInit.FrameWidth;
        pExtParRuntime->FrameHeight = extParInit.FrameHeight;
    }

    mfxExtVP9Segmentation* seg = GetExtBuffer(ctrl);
    if (seg)
    {
        bool removeSegBuffer = false;
        mfxExtCodingOption2 const & opt2 = GetExtBufferRef(video);
        mfxStatus sts = MFX_ERR_NONE;
        if (IsOn(opt2.MBBRC))
        {
            // segmentation ext buffer conflicts with MBBRC. It will be ignored.
            removeSegBuffer = true;
        }
        else if (seg->NumSegments)
        {
            const mfxExtVP9Param& extPar = GetExtBufferRef(video);

            sts = CheckSegmentationParam(*seg, extPar.FrameWidth, extPar.FrameHeight, caps, video, &ctrl);
            if (sts == MFX_ERR_UNSUPPORTED ||
                (true == AnyMandatorySegMapParam(*seg) && false == AllMandatorySegMapParams(*seg)) ||
                IsOn(opt2.MBBRC))
            {
                // provided segmentation parameters are invalid or lack mandatory information.
                // Ext buffer will be ignored. Report to application about it with warning.
                removeSegBuffer = true;
            }
            else
            {
                if (sts == MFX_WRN_INCOMPATIBLE_VIDEO_PARAM)
                {
                    checkSts = MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
                }

                mfxExtVP9Segmentation const & segInit = GetExtBufferRef(video);
                CombineInitAndRuntimeSegmentBuffers(*seg, segInit);
                if (false == AllMandatorySegMapParams(*seg))
                {
                    // neither runtime ext buffer not init don't contain valid segmentation map.
                    // Ext buffer will be ignored. Report to application about it with warning.
                    removeSegBuffer = true;
                }
            }
        }

        if (removeSegBuffer)
        {
            // remove ext buffer from mfxEncodeCtrl
            // and report to application about it with warning.
            checkSts = MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
            sts = RemoveExtBuffer(ctrl, MFX_EXTBUFF_VP9_SEGMENTATION);
            MFX_CHECK(sts >= MFX_ERR_NONE, checkSts);
        }
    }

    mfxExtVP9Segmentation* seg_video = GetExtBuffer(video);
    if (seg_video)
    {
        const mfxExtVP9Param& extParSeg = GetExtBufferRef(video);
        mfxStatus sts = CheckSegmentationParam(*seg_video, extParSeg.FrameWidth, extParSeg.FrameHeight, caps, video, &ctrl);
        MFX_CHECK_STS(sts);
    }

    return checkSts;
}

mfxStatus CheckBitstream(
    VP9MfxVideoParam const & video,
    mfxBitstream        * bs)
{
    mfxStatus checkSts = MFX_ERR_NONE;
    MFX_CHECK_NULL_PTR1(bs);
    MFX_CHECK_NULL_PTR1(bs->Data);

    // check bitstream buffer for enough space
    MFX_CHECK(bs->MaxLength > 0, MFX_ERR_NOT_ENOUGH_BUFFER);
    MFX_CHECK(bs->DataOffset < bs->MaxLength, MFX_ERR_UNDEFINED_BEHAVIOR);
    MFX_CHECK(bs->MaxLength > bs->DataOffset + bs->DataLength, MFX_ERR_NOT_ENOUGH_BUFFER);
    MFX_CHECK(bs->DataOffset + bs->DataLength + video.m_bufferSizeInKb * 1000 <= bs->MaxLength, MFX_ERR_NOT_ENOUGH_BUFFER);

    return checkSts;
}
} //namespace MfxHwVP9Encode
