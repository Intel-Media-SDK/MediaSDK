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

#include "math.h"
#include <map>
#include "mfx_vp9_encode_hw_utils.h"

namespace MfxHwVP9Encode
{

VP9MfxVideoParam::VP9MfxVideoParam()
    : m_platform(MFX_HW_UNKNOWN)
    , m_inMemType(INPUT_VIDEO_MEMORY)
    , m_targetKbps(0)
    , m_maxKbps(0)
    , m_bufferSizeInKb(0)
    , m_initialDelayInKb(0)
    , m_segBufPassed(false)
    , m_tempLayersBufPassed(false)
    , m_webRTCMode(false)
    , m_numLayers(0)
{
    Zero(m_layerParam);
    Zero(m_extParam);
    Zero(m_extPar);
    Zero(m_extOpaque);
    Zero(m_extOpt2);
    Zero(m_extOpt3);
    Zero(m_extOptDDI);
    Zero(m_extSeg);
    Zero(m_extTempLayers);
}

VP9MfxVideoParam::VP9MfxVideoParam(VP9MfxVideoParam const & par)
{
    Construct(par);
}

VP9MfxVideoParam::VP9MfxVideoParam(mfxVideoParam const & par)
{
    Construct(par);
}

VP9MfxVideoParam::VP9MfxVideoParam(mfxVideoParam const & par, eMFXHWType const & platform)
{
    m_platform = platform;
    Construct(par);
}

VP9MfxVideoParam& VP9MfxVideoParam::operator=(VP9MfxVideoParam const & par)
{
    m_platform = par.m_platform;
    Construct(par);

    return *this;
}

VP9MfxVideoParam& VP9MfxVideoParam::operator=(mfxVideoParam const & par)
{
    Construct(par);

    return *this;
}

void VP9MfxVideoParam::CalculateInternalParams()
{
    if (IOPattern == MFX_IOPATTERN_IN_SYSTEM_MEMORY ||
        (IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY && (m_extOpaque.In.Type & MFX_MEMTYPE_SYSTEM_MEMORY)))
    {
        m_inMemType = INPUT_SYSTEM_MEMORY;
    }
    else
    {
        m_inMemType = INPUT_VIDEO_MEMORY;
    }

    m_targetKbps = m_maxKbps = m_bufferSizeInKb = m_initialDelayInKb = 0;
    mfxU16 mult = std::max<mfxU16>(mfx.BRCParamMultiplier, 1);

    //"RateControlMethod == 0" always maps to CBR or VBR depending on TargetKbps and MaxKbps
    if (IsBitrateBasedBRC(mfx.RateControlMethod) || mfx.RateControlMethod == 0)
    {
        m_targetKbps = mult * mfx.TargetKbps;
        m_maxKbps = mult * mfx.MaxKbps;

        if (IsBufferBasedBRC(mfx.RateControlMethod) || mfx.RateControlMethod == 0)
        {
            m_bufferSizeInKb = mult * mfx.BufferSizeInKB;
            m_initialDelayInKb = mult * mfx.InitialDelayInKB;
        }
    }

    m_numLayers = 0;
    for (mfxU16 i = 0; i < MAX_NUM_TEMP_LAYERS; i++)
    {
        if (m_extTempLayers.Layer[i].FrameRateScale)
        {
            m_numLayers++;
        }
        m_layerParam[i].Scale = m_extTempLayers.Layer[i].FrameRateScale;
        m_layerParam[i].targetKbps = mult * m_extTempLayers.Layer[i].TargetKbps;
    }
}

void VP9MfxVideoParam::SyncInternalParamToExternal()
{
    mfxU32 maxBrcVal32 = m_bufferSizeInKb;

    if (IsBitrateBasedBRC(mfx.RateControlMethod))
    {
        maxBrcVal32 = std::max({m_maxKbps, maxBrcVal32, m_targetKbps});

        if (IsBufferBasedBRC(mfx.RateControlMethod))
        {
            maxBrcVal32 = std::max(maxBrcVal32, m_initialDelayInKb);
        }

        for (mfxU16 i = 0; i < MAX_NUM_TEMP_LAYERS; i++)
        {
            maxBrcVal32 = std::max(maxBrcVal32, m_layerParam[i].targetKbps);
        }
    }

    mfxU16 mult = std::max<mfxU16>(mfx.BRCParamMultiplier, 1);

    if (maxBrcVal32)
    {
        mult = mfx.BRCParamMultiplier = static_cast<mfxU16>((maxBrcVal32 + 0x10000) / 0x10000);
    }

    mfx.BufferSizeInKB = (mfxU16)CeilDiv(m_bufferSizeInKb, mult);

    if (IsBitrateBasedBRC(mfx.RateControlMethod))
    {
        mfx.TargetKbps = (mfxU16)CeilDiv(m_targetKbps, mult);
        mfx.MaxKbps = (mfxU16)CeilDiv(m_maxKbps, mult);

        if (IsBufferBasedBRC(mfx.RateControlMethod))
        {
            mfx.InitialDelayInKB = (mfxU16)CeilDiv(m_initialDelayInKb, mult);
        }
    }

    for (mfxU16 i = 0; i < MAX_NUM_TEMP_LAYERS; i++)
    {
        m_extTempLayers.Layer[i].FrameRateScale = m_layerParam[i].Scale;
        m_extTempLayers.Layer[i].TargetKbps = static_cast<mfxU16>(CeilDiv(m_layerParam[i].targetKbps, mult));
    }
}

void VP9MfxVideoParam::Construct(mfxVideoParam const & par)
{
    mfxVideoParam & base = *this;
    base = par;

    Zero(m_extParam);

    InitExtBufHeader(m_extPar);
    InitExtBufHeader(m_extOpaque);
    InitExtBufHeader(m_extOpt2);
    InitExtBufHeader(m_extOpt3);
    InitExtBufHeader(m_extOptDDI);
    InitExtBufHeader(m_extSeg);
    InitExtBufHeader(m_extTempLayers);

    if (mfxExtVP9Param * opts = GetExtBuffer(par))
        m_extPar = *opts;

    if (mfxExtOpaqueSurfaceAlloc * opts = GetExtBuffer(par))
        m_extOpaque = *opts;

    if (mfxExtCodingOption2 * opts = GetExtBuffer(par))
        m_extOpt2 = *opts;

    if (mfxExtCodingOption3 * opts = GetExtBuffer(par))
        m_extOpt3 = *opts;

    if (mfxExtCodingOptionDDI * opts = GetExtBuffer(par))
        m_extOptDDI = *opts;

    m_segBufPassed = false;
    if (mfxExtVP9Segmentation * opts = GetExtBuffer(par))
    {
        m_extSeg = *opts;
        m_segBufPassed = true;
    }

    m_tempLayersBufPassed = false;
    if (mfxExtVP9TemporalLayers * opts = GetExtBuffer(par))
    {
        m_extTempLayers = *opts;
        m_tempLayersBufPassed = true;
    }

    m_webRTCMode = false;
    if (m_tempLayersBufPassed && m_extOpt3.ScenarioInfo == MFX_SCENARIO_VIDEO_CONFERENCE)
        m_webRTCMode = true;
	
    m_extParam[0] = &m_extPar.Header;
    m_extParam[1] = &m_extOpaque.Header;
    m_extParam[2] = &m_extOpt2.Header;
    m_extParam[3] = &m_extOpt3.Header;
    m_extParam[4] = &m_extSeg.Header;
    m_extParam[5] = &m_extTempLayers.Header;
    m_extParam[6] = &m_extOptDDI.Header;

    ExtParam = m_extParam;
    NumExtParam = mfxU16(sizeof m_extParam / sizeof m_extParam[0]);
    assert(NumExtParam == NUM_OF_SUPPORTED_EXT_BUFFERS);

    CalculateInternalParams();
}

bool isVideoSurfInput(mfxVideoParam const & video)
{
    mfxExtOpaqueSurfaceAlloc * pOpaq = GetExtBuffer(video);

    if (video.IOPattern & MFX_IOPATTERN_IN_VIDEO_MEMORY)
        return true;
    if (isOpaq(video) && pOpaq)
    {
        if (pOpaq->In.Type & MFX_MEMTYPE_DXVA2_DECODER_TARGET)
        {
            return true;
        }
    }
    return false;
}

mfxU32 ModifyLoopFilterLevelQPBased(mfxU32 QP, mfxU32 loopFilterLevel)
{
    if (loopFilterLevel)
        return loopFilterLevel;

    if(QP >= 40) {
        return (int)(-18.98682 + 0.3967082*(float) QP + 0.0005054*pow((float) QP-127.5, 2) - 9.692e-6*pow((float) QP-127.5, 3));
    } else {
        return  QP/4;
    }
}

mfxStatus InitVp9SeqLevelParam(VP9MfxVideoParam const &video, VP9SeqLevelParam &param)
{
    Zero(param);

    param.profile = (mfxU8)(video.mfx.CodecProfile - 1);
    param.bitDepth = BITDEPTH_8;
    param.subsamplingX = 1;
    param.subsamplingY = 1;
#if (MFX_VERSION >= 1027)
    mfxExtCodingOption3 opt3 = GetExtBufferRef(video);
    if (MFX_CHROMAFORMAT_YUV444 == (opt3.TargetChromaFormatPlus1 - 1))
    {
        param.subsamplingX = 0;
        param.subsamplingY = 0;
    }
    param.bitDepth = (mfxU8)opt3.TargetBitDepthLuma;
#endif //MFX_VERSION >= 1027

    param.colorSpace = UNKNOWN_COLOR_SPACE;
    param.colorRange = 0; // BT.709-6

    return MFX_ERR_NONE;
};

mfxStatus SetFramesParams(VP9MfxVideoParam const &par,
                          Task const & task,
                          mfxU8 frameType,
                          VP9FrameLevelParam &frameParam,
                          eMFXHWType platform)
{
    (void)platform;

    Zero(frameParam);
    frameParam.frameType = frameType;

    mfxExtVP9Param const &extPar = GetActualExtBufferRef(par, task.m_ctrl);

    if (par.mfx.RateControlMethod == MFX_RATECONTROL_CQP)
    {
        frameParam.baseQIndex = mfxU8(task.m_ctrl.QP > 0 ?
            task.m_ctrl.QP : frameParam.frameType == KEY_FRAME ?
            par.mfx.QPI : par.mfx.QPP);
    }

    frameParam.lfLevel   = (mfxU8)ModifyLoopFilterLevelQPBased(frameParam.baseQIndex, 0); // always 0 is passes since at the moment there is no LF level in MSDK API

    frameParam.width  = frameParam.renderWidth = extPar.FrameWidth;
    frameParam.height = frameParam.renderHeight = extPar.FrameHeight;

    /*frameParam.sharpness = (mfxU8)opt.SharpnessLevel;

    for (mfxU8 i = 0; i < 4; i ++)
    {
        frameParam.lfRefDelta[i] = (mfxI8)opt.LoopFilterRefDelta[i];
    }

    frameParam.lfModeDelta[0] = (mfxI8)opt.LoopFilterModeDelta[0];
    frameParam.lfModeDelta[1] = (mfxI8)opt.LoopFilterModeDelta[1];*/


    frameParam.qIndexDeltaLumaDC   = static_cast<mfxI8>(extPar.QIndexDeltaLumaDC);
    frameParam.qIndexDeltaChromaAC = static_cast<mfxI8>(extPar.QIndexDeltaChromaAC);
    frameParam.qIndexDeltaChromaDC = static_cast<mfxI8>(extPar.QIndexDeltaChromaDC);

    frameParam.errorResilentMode = par.m_webRTCMode ? 1 : 0;

    mfxExtCodingOption2 const & opt2 = GetExtBufferRef(par);
    if (IsOn(opt2.MBBRC))
    {
        frameParam.segmentation = BRC_SEGMENTATION;
    }
    else
    {
        mfxExtVP9Segmentation const & seg = GetActualExtBufferRef(par, task.m_ctrl);

        if (seg.NumSegments && AllMandatorySegMapParams(seg))
        {
            frameParam.segmentation = APP_SEGMENTATION;
            frameParam.segmentationUpdateMap = frameParam.frameType == KEY_FRAME ||
                frameParam.errorResilentMode ||
                false == CompareSegmentMaps(*task.m_pPrevSegment, seg);
            frameParam.segmentationUpdateData = frameParam.frameType == KEY_FRAME ||
                frameParam.errorResilentMode ||
                false == CompareSegmentParams(*task.m_pPrevSegment, seg);
            frameParam.segmentationTemporalUpdate = 0;
            frameParam.segmentIdBlockSize = seg.SegmentIdBlockSize;
        }
        else
        {
            frameParam.segmentation = NO_SEGMENTATION;
        }
    }

    frameParam.showFrame = 1;
    frameParam.intraOnly = 0;

    if (par.mfx.RateControlMethod != MFX_RATECONTROL_CQP)
    {
        // in BRC mode driver may update LF level and mode/ref LF deltas
        frameParam.modeRefDeltaEnabled = 1;
        frameParam.modeRefDeltaUpdate = 1;
#if (MFX_VERSION >= 1027)
        if (platform >= MFX_HW_ICL)
        {
            // WA: driver writes corrupted uncompressed frame header when mode or ref deltas are written by MSDK
            // TODO: remove this once driver behavior fixed
            frameParam.modeRefDeltaEnabled = 0;
            frameParam.modeRefDeltaUpdate = 0;
        }
#endif //MFX_VERSION >= 1027
    }

    mfxExtCodingOptionDDI const & extDdi = GetExtBufferRef(par);
    if (extDdi.RefreshFrameContext)
    {
        frameParam.refreshFrameContext = IsOn(extDdi.RefreshFrameContext) ? 1 : 0;
    }
    else
    {
        // context switching isn't supported by driver now. So refresh_frame_context is disabled for temporal scalability
        frameParam.refreshFrameContext = par.m_numLayers ? 0 : 1;
    }

    frameParam.allowHighPrecisionMV = 1;

    mfxU16 alignedWidth  = mfx::align2_value(frameParam.width,  8); // align to Mode Info block size (8 pixels)
    mfxU16 alignedHeight = mfx::align2_value(frameParam.height, 8); // align to Mode Info block size (8 pixels)

    frameParam.modeInfoRows = alignedHeight >> 3;
    frameParam.modeInfoCols = alignedWidth >> 3;

    frameParam.temporalLayer = CalcTemporalLayerIndex(par, task.m_frameOrderInGop);
    frameParam.nextTemporalLayer = CalcTemporalLayerIndex(par, task.m_frameOrderInGop + 1);

    if (!IsOff(extDdi.SuperFrameForTS) && par.m_numLayers &&
        frameParam.temporalLayer != frameParam.nextTemporalLayer
        && !par.m_webRTCMode)
    {
        frameParam.showFrame = 0;
    }

    if (IsOn(extDdi.ChangeFrameContextIdxForTS))
    {
        if (par.m_numLayers)
        {
            frameParam.frameContextIdx = static_cast<mfxU8>(frameParam.temporalLayer);
        }
        else
        {
            frameParam.frameContextIdx = static_cast<mfxU8>(task.m_frameOrder % 4);
        }
    }

    if (par.m_numLayers > 1 && IsOn(extDdi.ChangeFrameContextIdxForTS) && frameParam.refreshFrameContext)
    {
        // context refreshing should be disabled for certain frames in TS-encoding
        // this is demand by kernel processing logic
        if (IsNeedDisableRefreshForFrameTS(par, task.m_frameOrderInGop))
        {
            frameParam.refreshFrameContext = false;
        }
    }
#if (MFX_VERSION >= 1029)
    frameParam.log2TileRows = static_cast<mfxU8>(CeilLog2(extPar.NumTileRows));
    frameParam.log2TileCols = static_cast<mfxU8>(CeilLog2(extPar.NumTileColumns));
#endif // (MFX_VERSION >= 1029)

    return MFX_ERR_NONE;
}

#define DPB_SLOT_FOR_GOLD_REF dpbSize - 1

// construct default ref list. This list can be modified later for special cases
mfxStatus GetDefaultRefList(mfxU32 frameOrder, mfxU8 (&refList)[3], std::vector<sFrameEx*> const & dpb)
{
    mfxU8 dpbSize = (mfxU8)dpb.size();

    bool multiref = dpbSize > 1;
    if (multiref == true)
    {
        refList[REF_GOLD] = DPB_SLOT_FOR_GOLD_REF; // last DPB intry is always for LTR (= GOLD)
                                                   // for DPB size 2 LAST and ALT are both DBP[0]
                                                   // for DPB size 3 LAST and ALT alternate between DPB[0] and DPB[1]
        if (dpbSize == 2)
        {
            refList[REF_LAST] = refList[REF_ALT] = 0;
        }
        else
        {
            refList[REF_LAST] = 1 - frameOrder % 2;
            refList[REF_ALT] = frameOrder % 2;
        }
    }
    else
    {
        // single ref:
        // Last, Gold, Alt are pointing to DPB[0]
        refList[REF_GOLD] = refList[REF_ALT] = refList[REF_LAST] = 0;
    }

    return MFX_ERR_NONE;
}

mfxStatus DecideOnRefListAndDPBRefresh(
    VP9MfxVideoParam const & par,
    Task *pTask,
    std::vector<sFrameEx*>& dpb,
    VP9FrameLevelParam &frameParam,
    mfxU32 prevFrameOrderInRefStructure)
{
    if (frameParam.frameType == KEY_FRAME)
    {
        memset(frameParam.refreshRefFrames, 1, DPB_SIZE);
        return MFX_ERR_NONE;
    }

    mfxU8 dpbSize = (mfxU8)dpb.size();

    if (par.m_numLayers < 2)
    {
        // single layer
        mfxU32 frameOrder = pTask->m_frameOrderInRefStructure;

        GetDefaultRefList(
            frameOrder ? frameOrder : prevFrameOrderInRefStructure,
            frameParam.refList,
            dpb);

        // by default encoder always updates DPB slot pointed by "default" ALT
        mfxU8 dpbSlotToRefresh = frameParam.refList[REF_ALT];

        bool multiref = dpbSize > 1;
        if (multiref)
        {
            // modify ref list to workaround architecture limitations
            if (frameOrder == 0)
            {
                // special case - ref structure was reset w/o key-frame - use single ref for current frame
                frameParam.refList[REF_GOLD] = frameParam.refList[REF_ALT] = frameParam.refList[REF_LAST];
            }
            else if (dpb[frameParam.refList[REF_LAST]] == dpb[frameParam.refList[REF_GOLD]] &&
                dpb[frameParam.refList[REF_LAST]] != dpb[frameParam.refList[REF_ALT]])
            {
                // if LAST and GOLD point to same reference slot, other than is pointed by ALT
                // need to swap ALT and GOLD since arhitecture doesn't support use of LAST and ALT w/o GOLD
                std::swap(frameParam.refList[REF_GOLD], frameParam.refList[REF_ALT]);
            }
        }

        if (frameOrder == 0)
        {
            // refresh whole DPB with current frame
            memset(frameParam.refreshRefFrames, 1, DPB_SIZE);
        }
        else
        {
            // refresh chosen DPB slot with current frame
            frameParam.refreshRefFrames[dpbSlotToRefresh] = 1;
            // in addition change LTR (=GOLD) ref frame 4 times per second
            mfxU32 frameRate = par.mfx.FrameInfo.FrameRateExtN / par.mfx.FrameInfo.FrameRateExtD;
            if (frameRate >= 4 &&
                ((frameOrder % (frameRate / 2)) == 0 ||
                (frameOrder % (frameRate / 2)) == frameRate / 4))
            {
                frameParam.refreshRefFrames[DPB_SLOT_FOR_GOLD_REF] = 1;
            }
        }
    }
    else
    {
        // multiple temporal layers
        std::map<mfxU32, mfxU32> FOs;
        mfxU16 lastRefLayer = par.mfx.NumRefFrame >= par.m_numLayers ? par.m_numLayers - 1 : par.m_numLayers - 2;
        for (mfxU32 i = 0; i <= frameParam.temporalLayer && i <= lastRefLayer; i++)
        {
            FOs.emplace(dpb[i]->frameOrder, i);
        }

        if (par.m_webRTCMode)
        {
            // limit Picture Group to 8 frames
            static mfxU32 zeroLevelCounter = 0;
            if (frameParam.temporalLayer == 0) zeroLevelCounter++; // each time we get frame with TL id = 0, zeroLevelCounter increases

            // To limit PG to 8 frames (means 9 and next frames could not reffer to frames previous to 8 frame), we need to delete last reference that comes after 8's frame
            // For 3 TL case, TL pattern for first 8 frames would be 0212 0212. Zero-level is present here 2 times -> multiplier = 2.
            // For 2 TL case, TL pattern for first 8 frames would be 0101 0101. Zero-level is present here 4 times -> multiplier = 4.
            // Getting zeroLevelCounter divided by specific multiplier, we could make a decision that PG is ended and now we can erase reference: 

            mfxU8 multiplier = par.m_numLayers == 3 ? 2 : 4;

            if (zeroLevelCounter % multiplier == 0)
            {
                auto it = FOs.begin();
                FOs.erase(it);
            }
        }

        std::map<mfxU32, mfxU32>::reverse_iterator closest = FOs.rbegin();
        for (mfxU8 ref = REF_LAST; ref < REF_TOTAL && closest != FOs.rend(); ref++, closest++)
        {
            frameParam.refList[ref] = static_cast<mfxU8>(closest->second);
        }
        frameParam.refreshRefFrames[frameParam.temporalLayer] = 1;
    }

    memset(&frameParam.refBiases[0], 0, REF_TOTAL);

    return MFX_ERR_NONE;
}

mfxStatus UpdateDpb(VP9FrameLevelParam &frameParam, sFrameEx *pRecFrame, std::vector<sFrameEx*>&dpb, VideoCORE* pCore)
{
    for (mfxU8 i = 0; i < dpb.size(); i++)
    {
        if (frameParam.refreshRefFrames[i])
        {
            if (dpb[i])
            {
                mfxStatus sts = DecreaseRef(dpb[i], pCore);
                MFX_CHECK_STS(sts);
            }
            dpb[i] = pRecFrame;
            IncreaseRef(dpb[i]);
        }
    }
    return MFX_ERR_NONE;
}

//---------------------------------------------------------
// service class: MfxFrameAllocResponse
//---------------------------------------------------------

MfxFrameAllocResponse::MfxFrameAllocResponse()
    : m_info()
    , m_pCore(0)
    , m_numFrameActualReturnedByAllocFrames(0)
{
    Zero(m_info);
}

MfxFrameAllocResponse::~MfxFrameAllocResponse()
{
    Release();
}

mfxStatus MfxFrameAllocResponse::Alloc(
    VideoCORE*     pCore,
    mfxFrameAllocRequest & req,
    bool isCopyRequired)
{
    req.NumFrameSuggested = req.NumFrameMin; // no need in 2 different NumFrames

    if (pCore->GetVAType() == MFX_HW_D3D11)
    {
        mfxFrameAllocRequest tmp = req;
        tmp.NumFrameMin = tmp.NumFrameSuggested = 1;

        m_responseQueue.resize(req.NumFrameMin);
        m_mids.resize(req.NumFrameMin);

        for (int i = 0; i < req.NumFrameMin; i++)
        {
            mfxStatus sts = pCore->AllocFrames(&tmp, &m_responseQueue[i], isCopyRequired);
            MFX_CHECK_STS(sts);

            m_mids[i] = m_responseQueue[i].mids[0];
        }

        mids = &m_mids[0];
        NumFrameActual = req.NumFrameMin;
    }
    else
    {
        mfxStatus sts = pCore->AllocFrames(&req, this, isCopyRequired);
        MFX_CHECK_STS(sts);
    }

    if (NumFrameActual < req.NumFrameMin)
    {
        return MFX_ERR_MEMORY_ALLOC;
    }

    m_pCore = pCore;
    m_numFrameActualReturnedByAllocFrames = NumFrameActual;
    NumFrameActual = req.NumFrameMin;
    m_info = req.Info;

    return MFX_ERR_NONE;
}

mfxStatus MfxFrameAllocResponse::Release()
{
    if (m_numFrameActualReturnedByAllocFrames == 0)
    {
        // nothing was allocated, nothig to do
        return MFX_ERR_NONE;
    }

    if (m_pCore == 0)
    {
        return MFX_ERR_NULL_PTR;
    }

    if (m_pCore->GetVAType() == MFX_HW_D3D11)
    {
        for (size_t i = 0; i < m_responseQueue.size(); i++)
        {
            mfxStatus sts = m_pCore->FreeFrames(&m_responseQueue[i]);
            MFX_CHECK_STS(sts);
        }
        m_responseQueue.resize(0);
        m_numFrameActualReturnedByAllocFrames = 0;
    }
    else
    {
        if (mids)
        {
            NumFrameActual = m_numFrameActualReturnedByAllocFrames;
            mfxStatus sts = m_pCore->FreeFrames(this);
            MFX_CHECK_STS(sts);

            m_numFrameActualReturnedByAllocFrames = 0;
        }
    }

    return MFX_ERR_NONE;
}

//---------------------------------------------------------
// service class: ExternalFrames
//---------------------------------------------------------

void ExternalFrames::Init(mfxU32 numFrames)
{
    m_frames.resize(numFrames);
    {
        mfxU32 i = 0;
        std::list<sFrameEx>::iterator frame = m_frames.begin();
        for ( ;frame!= m_frames.end(); frame++)
        {
            Zero(*frame);
            frame->idInPool = i++;
        }
    }
}

mfxStatus ExternalFrames::GetFrame(mfxFrameSurface1 *pInFrame, sFrameEx *&pOutFrame )
{
    std::list<sFrameEx>::iterator frame = m_frames.begin();
    for ( ;frame!= m_frames.end(); frame++)
    {
        if (frame->pSurface == 0)
        {
            frame->pSurface = pInFrame;
            pOutFrame = &*frame;
            return MFX_ERR_NONE;
        }
        if (frame->pSurface == pInFrame)
        {
            pOutFrame = &*frame;
            return MFX_ERR_NONE;
        }
    }

    sFrameEx newFrame = {};
    newFrame.idInPool = (mfxU32)m_frames.size();
    newFrame.pSurface = pInFrame;

    m_frames.push_back(newFrame);
    pOutFrame = &m_frames.back();

    return MFX_ERR_NONE;
}

//---------------------------------------------------------
// service class: InternalFrames
//---------------------------------------------------------
mfxStatus InternalFrames::Init(VideoCORE *pCore, mfxFrameAllocRequest *pAllocReq, bool isCopyRequired)
{
    MFX_CHECK_NULL_PTR2 (pCore, pAllocReq);
    mfxU32 nFrames = pAllocReq->NumFrameMin;

    if (nFrames == 0)
    {
        return MFX_ERR_NONE;
    }

    //printf("internal frames init %d (request)\n", req.NumFrameSuggested);

    mfxStatus sts = MFX_ERR_NONE;
    sts = m_response.Alloc(pCore, *pAllocReq, isCopyRequired);
    MFX_CHECK_STS(sts);

    //printf("internal frames init %d (%d) [%d](response)\n", m_response.NumFrameActual,Num(),nFrames);

    m_surfaces.resize(nFrames);
    Zero(m_surfaces);

    //printf("internal frames init 1 [%d](response)\n", Num());

    m_frames.resize(nFrames);
    Zero(m_frames);

    //printf("internal frames init 2 [%d](response)\n", Num());

    for (mfxU32 i = 0; i < nFrames; i++)
    {
        m_frames[i].idInPool = i;
        m_frames[i].refCount = 0;
        m_surfaces[i].Data.MemId = m_response.mids[i];
        m_surfaces[i].Info = pAllocReq->Info;
        m_frames[i].pSurface = &m_surfaces[i];
    }
    return sts;
}

sFrameEx * InternalFrames::GetFreeFrame()
{
    std::vector<sFrameEx>::iterator frame = m_frames.begin();
    for (;frame != m_frames.end(); frame ++)
    {
        if (isFreeSurface(&frame[0]))
        {
            return &frame[0];
        }
    }
    return 0;
}

mfxStatus  InternalFrames::GetFrame(mfxU32 numFrame, sFrameEx * &Frame)
{
    MFX_CHECK(numFrame < m_frames.size(), MFX_ERR_UNDEFINED_BEHAVIOR);

    if (isFreeSurface(&m_frames[numFrame]))
    {
        Frame = &m_frames[numFrame];
        return MFX_ERR_NONE;
    }
    return MFX_WRN_DEVICE_BUSY;
}

mfxStatus InternalFrames::Release()
{
    mfxStatus sts = m_response.Release();
    MFX_CHECK_STS(sts);

    return MFX_ERR_NONE;
}

//---------------------------------------------------------
// service class: Task
//---------------------------------------------------------

mfxStatus GetRealSurface(
    VideoCORE *pCore,
    VP9MfxVideoParam const &par,
    Task const &task,
    mfxFrameSurface1 *& pSurface)
{
    if (par.IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY)
    {
        pSurface = pCore->GetNativeSurface(task.m_pRawFrame->pSurface);
    }
    else
    {
        pSurface = task.m_pRawFrame->pSurface;
    }

    return MFX_ERR_NONE;
}

mfxStatus GetInputSurface(
   VideoCORE *pCore,
    VP9MfxVideoParam const &par,
    Task const &task,
    mfxFrameSurface1 *& pSurface)
{
    if (task.m_pRawLocalFrame)
    {
        pSurface = task.m_pRawLocalFrame->pSurface;
    }
    else
    {
        mfxStatus sts = GetRealSurface(pCore, par, task, pSurface);
        MFX_CHECK_STS(sts);
    }

    return MFX_ERR_NONE;
}

mfxStatus CopyRawSurfaceToVideoMemory(
    VideoCORE *pCore,
    VP9MfxVideoParam const &par,
    Task const &task)
{
    if (par.m_inMemType == INPUT_SYSTEM_MEMORY)
    {
        MFX_CHECK_NULL_PTR1(task.m_pRawLocalFrame);
        mfxFrameSurface1 *pDd3dSurf = task.m_pRawLocalFrame->pSurface;
        mfxFrameSurface1 *pSysSurface = 0;
        mfxStatus sts = GetRealSurface(pCore, par, task, pSysSurface);
        MFX_CHECK_STS(sts);

        mfxFrameSurface1 sysSurf = *pSysSurface;

        sts = pCore->DoFastCopyWrapper(
            pDd3dSurf,
            MFX_MEMTYPE_INTERNAL_FRAME | MFX_MEMTYPE_DXVA2_DECODER_TARGET | MFX_MEMTYPE_FROM_ENCODE,
            &sysSurf,
            MFX_MEMTYPE_EXTERNAL_FRAME | MFX_MEMTYPE_SYSTEM_MEMORY);
        MFX_CHECK_STS(sts);
    }

    return MFX_ERR_NONE;
}

mfxStatus GetNativeHandleToRawSurface(
    VideoCORE & core,
    mfxMemId mid,
    mfxHDL *handle,
    VP9MfxVideoParam const & video)
{
    mfxStatus sts = MFX_ERR_NONE;

    mfxU32 iopattern = video.IOPattern;

    mfxExtOpaqueSurfaceAlloc& opaq = GetExtBufferRef(video);
    mfxU16 opaq_type = opaq.In.Type;

    if (iopattern == MFX_IOPATTERN_IN_SYSTEM_MEMORY ||
        (iopattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY && (opaq_type & MFX_MEMTYPE_SYSTEM_MEMORY)))
        sts = core.GetFrameHDL(mid, handle);
    else if (iopattern == MFX_IOPATTERN_IN_VIDEO_MEMORY)
        sts = core.GetExternalFrameHDL(mid, handle);
    else if (iopattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY) // opaq with internal video memory
        sts = core.GetFrameHDL(mid, handle);
    else
        sts = MFX_ERR_UNDEFINED_BEHAVIOR;

    return sts;
}

} // MfxHwVP9Encode

