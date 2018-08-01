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

#include "mctf_common.h"
#include "asc.h"
#include "asc_defs.h"

#include "genx_global.h"

#include "genx_me_bdw_isa.h"
#include "genx_mc_bdw_isa.h"
#include "genx_sd_bdw_isa.h"

#include "genx_me_skl_isa.h"
#include "genx_mc_skl_isa.h"
#include "genx_sd_skl_isa.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include "cmrt_cross_platform.h"

using std::min;
using  std::max;
using namespace ns_asc;

const mfxU16 CMC::AUTO_FILTER_STRENTH = 0;
const mfxU16 CMC::DEFAULT_FILTER_STRENGTH = 8;
const mfxU32 CMC::DEFAULT_BPP = 12 * MCTF_BITRATE_MULTIPLIER;
const mfxU16 CMC::DEFAULT_DEBLOCKING = MFX_CODINGOPTION_OFF;
const mfxU16 CMC::DEFAULT_OVERLAP = MFX_CODINGOPTION_OFF;
const mfxU16 CMC::DEFAULT_ME = MFX_MVPRECISION_INTEGER;
const mfxU16 CMC::DEFAULT_REFS = MCTF_TEMPORAL_MODE_2REF;

void CMC::QueryDefaultParams(IntMctfParams* pBuffer)
{
    if (!pBuffer) return;
    pBuffer->Deblocking = DEFAULT_DEBLOCKING;
    pBuffer->Overlap = DEFAULT_OVERLAP;
    pBuffer->subPelPrecision = DEFAULT_ME;
    pBuffer->TemporalMode = DEFAULT_REFS;
    pBuffer->FilterStrength = AUTO_FILTER_STRENTH; // [0...20]
    pBuffer->BitsPerPixelx100k= DEFAULT_BPP;
};

void CMC::QueryDefaultParams(mfxExtVppMctf* pBuffer)
{
    if (!pBuffer) return;
    IntMctfParams Mctfparam;
    QueryDefaultParams(&Mctfparam);
    pBuffer->FilterStrength = Mctfparam.FilterStrength;
#ifdef MFX_ENABLE_MCTF_EXT
    pBuffer->BitsPerPixelx100k = Mctfparam.BitsPerPixelx100k;
    pBuffer->Overlap = Mctfparam.Overlap;
    pBuffer->Deblocking = Mctfparam.Deblocking;
    pBuffer->TemporalMode = Mctfparam.TemporalMode;
    pBuffer->MVPrecision = Mctfparam.subPelPrecision;
#endif
};

mfxStatus CMC::CheckAndFixParams(mfxExtVppMctf* pBuffer)
{
    mfxStatus sts = MFX_ERR_NONE;
    if (!pBuffer) return MFX_ERR_NULL_PTR;
    if (pBuffer->FilterStrength > 20) {
        pBuffer->FilterStrength = AUTO_FILTER_STRENTH;
        sts = MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
    }
#ifdef MFX_ENABLE_MCTF_EXT
    if (MFX_CODINGOPTION_OFF != pBuffer->Deblocking &&
        MFX_CODINGOPTION_ON != pBuffer->Deblocking &&
        MFX_CODINGOPTION_UNKNOWN != pBuffer->Deblocking) {
        pBuffer->Deblocking = DEFAULT_DEBLOCKING;
        sts = MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
    }else
        if (MFX_CODINGOPTION_UNKNOWN == pBuffer->Deblocking){
            pBuffer->Deblocking = DEFAULT_DEBLOCKING;
            // keep previous status to not override possibe WRN
        }
    if (MFX_CODINGOPTION_OFF != pBuffer->Overlap &&
        MFX_CODINGOPTION_ON != pBuffer->Overlap &&
        MFX_CODINGOPTION_UNKNOWN != pBuffer->Overlap) {
        pBuffer->Overlap = DEFAULT_OVERLAP;
        sts = MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
    }else
        if (MFX_CODINGOPTION_UNKNOWN == pBuffer->Overlap) {
            pBuffer->Overlap = DEFAULT_OVERLAP;
            // keep previous status to not override possibe WRN
        }

    if (MCTF_TEMPORAL_MODE_SPATIAL != pBuffer->TemporalMode &&
        MCTF_TEMPORAL_MODE_1REF != pBuffer->TemporalMode &&
        MCTF_TEMPORAL_MODE_2REF != pBuffer->TemporalMode &&
        MCTF_TEMPORAL_MODE_4REF != pBuffer->TemporalMode &&
        MCTF_TEMPORAL_MODE_UNKNOWN != pBuffer->TemporalMode) {
        pBuffer->TemporalMode = DEFAULT_REFS;
        sts = MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
    }else
        if (MCTF_TEMPORAL_MODE_UNKNOWN == pBuffer->TemporalMode) {
            pBuffer->TemporalMode = DEFAULT_REFS;
            // keep previous status to not override possibe WRN
        }


    if (MFX_MVPRECISION_INTEGER != pBuffer->MVPrecision &&
        MFX_MVPRECISION_QUARTERPEL != pBuffer->MVPrecision &&
        MFX_MVPRECISION_UNKNOWN != pBuffer->MVPrecision) {
        pBuffer->MVPrecision = DEFAULT_ME;
        sts = MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
    }else
        if (MFX_MVPRECISION_UNKNOWN == pBuffer->MVPrecision)
        {
            pBuffer->MVPrecision = DEFAULT_ME;
            // keep previous status to not override possibe WRN
        }

    if (pBuffer->BitsPerPixelx100k > DEFAULT_BPP) {
        pBuffer->BitsPerPixelx100k = DEFAULT_BPP;
        sts = MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
    }
#endif
    return sts;
}

// this function fills MCTF params based on extended buffer
void CMC::FillParamControl(IntMctfParams* pBuffer, const mfxExtVppMctf* pSrc)
{
    pBuffer->FilterStrength = pSrc->FilterStrength;
#ifdef MFX_ENABLE_MCTF_EXT
    pBuffer->Deblocking =pSrc->Deblocking;
    pBuffer->Overlap = pSrc->Overlap;
    pBuffer->subPelPrecision = pSrc->MVPrecision;
    pBuffer->TemporalMode = pSrc->TemporalMode;
    pBuffer->BitsPerPixelx100k = pSrc->BitsPerPixelx100k;
#endif
}

inline mfxStatus CMC::SetFilterStrenght(unsigned short fs) {
    if (fs > 20) {
#ifdef MFX_MCTF_DEBUG_PRINT
        ASC_PRINTF("Invalid filter strength\nEnd\n");
#endif
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    p_ctrl->th = fs * 50;
    p_ctrl->sTh = min(fs + CHROMABASE, MAXCHROMA);//fs * 5;
    res = ctrlBuf->WriteSurface((const uint8_t *)p_ctrl.get(), NULL, sizeof(MeControlSmall));
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    return MFX_ERR_NONE;
}

inline mfxStatus CMC::SetupMeControl(const mfxFrameInfo& FrameInfo, mfxU16 th, mfxU16 subPelPre)
{
    p_ctrl.reset(new MeControlSmall);

    const mfxU8 Diamond[SEARCHPATHSIZE] = {
        0x0F,0xF1,0x0F,0x12,//5
        0x0D,0xE2,0x22,0x1E,//9
        0x10,0xFF,0xE2,0x20,//13
        0xFC,0x06,0xDD,//16
        0x2E,0xF1,0x3F,0xD3,0x11,0x3D,0xF3,0x1F,//24
        0xEB,0xF1,0xF1,0xF1,//28
        0x4E,0x11,0x12,0xF2,0xF1,//33
        0xE0,0xFF,0xFF,0x0D,0x1F,0x1F,//39
        0x20,0x11,0xCF,0xF1,0x05,0x11,//45
        0x00,0x00,0x00,0x00,0x00,0x00,//51
    };
    memcpy_s(p_ctrl->searchPath.sp, sizeof(Diamond), Diamond, sizeof(p_ctrl->searchPath.sp));
    p_ctrl->searchPath.lenSp = 16;
    p_ctrl->searchPath.maxNumSu = 57;

    p_ctrl->width = FrameInfo.Width;
    p_ctrl->height = FrameInfo.Height;

    mfxU16 CropX(FrameInfo.CropX), CropY(FrameInfo.CropY), CropW(FrameInfo.CropW), CropH(FrameInfo.CropH);

    // this code align crop regin to CROP_BLOCK_ALIGNMENT boundary;
    // doing this, it follows the principle that initial crop-region
    // to be included in the crop-region after alignment;

    mfxU16 CropRBX = CropX + CropW - 1;
    mfxU16 CropRBY = CropY + CropH - 1;
    CropX = (CropX / CROP_BLOCK_ALIGNMENT) * CROP_BLOCK_ALIGNMENT;
    CropY = (CropY / CROP_BLOCK_ALIGNMENT) * CROP_BLOCK_ALIGNMENT;
    CropW = CropRBX - CropX + 1;
    CropH = CropRBY - CropY + 1;

    CropW = (DIVUP(CropW, CROP_BLOCK_ALIGNMENT)) * CROP_BLOCK_ALIGNMENT;
    CropH = (DIVUP(CropH, CROP_BLOCK_ALIGNMENT)) * CROP_BLOCK_ALIGNMENT;

    if ((CropX + CropW > FrameInfo.Width) || (CropY + CropH > FrameInfo.Height)) {
#ifdef MFX_MCTF_DEBUG_PRINT
        ASC_PRINTF("Crop alignement led to out-of-surface coordinates.\n");
#endif
        return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;
    }

    p_ctrl->CropX = CropX;
    p_ctrl->CropY = CropY;
    p_ctrl->CropW = CropW;
    p_ctrl->CropH = CropH;

    if (th  > 20)
    {
#ifdef MFX_MCTF_DEBUG_PRINT
        ASC_PRINTF("MCTF Error: Filter strength too large; check your parameters\n");
#endif
        return MFX_ERR_INVALID_VIDEO_PARAM;
        
    }
    p_ctrl->th = th * 50;
    p_ctrl->sTh = min(th + CHROMABASE, MAXCHROMA);// th * 5;
    if (MFX_MVPRECISION_INTEGER == subPelPre)
        p_ctrl->subPrecision = 0;
    else
        if (MFX_MVPRECISION_QUARTERPEL == subPelPre)
            p_ctrl->subPrecision = 1;
        else
            return MFX_ERR_INVALID_VIDEO_PARAM;
    return MFX_ERR_NONE;
}

void CMC::TimeStart() {
}

void CMC::TimeStart(int index) {
    (void)index;
}

void CMC::TimeStop() {
}

mfxF64 CMC::CatchTime(const char* message, int print)
{
    mfxF64
        timeval = 0.0;
//    timeval = TimeMeasurement(pTimer->tStart, pTimer->tStop, pTimer->tFrequency);
    if (print)
        ASC_PRINTF("%s %0.3f ms.\n", message, timeval);
    return timeval;
}

mfxF64 CMC::CatchTime(int indexInit, int indexEnd, const char* message, int print) {
    (void)indexInit;
    (void)indexEnd;
    (void)message;
    (void)print;

    return 0.0;
}

void CMC::CatchEndTime(mfxI32 /* processed_frames */) {
    TimeStop();
//    CatchTime("Total process time:", 1);
}

mfxStatus CMC::MCTF_GET_FRAME(CmSurface2D* outFrame) {


    mfxStatus sts = MFX_ERR_NONE;
    if (!outFrame)
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    if (!mco)
    {
        // we are in the end of a stream, and it must be the configuration with
        // > 1 reference as there is no reason to get another surface out of MCTF 
        // if no delay was assumed.
        if (NO_REFERENCES == number_of_References || ONE_REFERENCE == number_of_References)
        {
            //this is undefined behavior
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        }
        else
        {
            // let map outframe to mco
            mco = outFrame;
            INT cmSts = mco->GetIndex(idxMco);
            MFX_CHECK((CM_SUCCESS == cmSts), MFX_ERR_DEVICE_FAILED);
        }
    }

    if (QfIn.size() == 5) 
    {
        if (lastFrame == 1) 
        {
#if !HALFWORK
            pMCTF_MERGE_func = &CMC::MCTF_RUN_BLEND2R;
            RotateBufferB();
#endif
            res = MCTF_RUN_MCTF_DEN();
            lastFrame++;
        }
        else if (lastFrame == 2)
#if HALFWORK
            MCTF_RUN_AMCTF(lastFrame);
#else
            res = MCTF_RUN_MCTF_DEN();
#endif
    }
    else if (QfIn.size() == 3) 
    {
        if (lastFrame == 1)
            //res = MCTF_RUN_AMCTF(lastFrame);
            res = MCTF_RUN_MCTF_DEN();
    }

    MFX_CHECK(mco, MFX_ERR_UNDEFINED_BEHAVIOR);
    mco = nullptr;
    if (!lastFrame)
        lastFrame = 1;
#ifdef MFX_MCTF_DEBUG_PRINT
    ASC_PRINTF("QfIn[CurrentIdx2Out]::FrameOrder: %u, OutFrame::FramOrder: %u \n", QfIn[CurrentIdx2Out].mfxFrame->Data.FrameOrder, outFrame->Data.FrameOrder);
    ASC_FFLUSH(stdout);
#endif
    return sts;
}

mfxStatus CMC::MCTF_TrackTimeStamp(mfxFrameSurface1 *outFrame)
{
    outFrame->Data.FrameOrder = QfIn[CurrentIdx2Out].mfxFrame->Data.FrameOrder;
    outFrame->Data.TimeStamp = QfIn[CurrentIdx2Out].mfxFrame->Data.TimeStamp;
    return MFX_ERR_NONE;
}

mfxStatus CMC::MCTF_InitQueue(mfxU16 refNum)
{
    mfxU32  buffer_size(0);
    switch (refNum) {
    case MCTF_TEMPORAL_MODE_4REF:
    {
        number_of_References = FOUR_REFERENCES;
        DefaultIdx2Out = 1;
        CurrentIdx2Out = 0;
        MctfState = AMCTF_NOT_READY;
        buffer_size = FOUR_REFERENCES + 1;
    };
    break;
    case MCTF_TEMPORAL_MODE_2REF:
    {
        number_of_References = TWO_REFERENCES;
        DefaultIdx2Out = 0;
        CurrentIdx2Out = 0;
        MctfState = AMCTF_NOT_READY;
        buffer_size = TWO_REFERENCES + 1;
    };
    break;
    case MCTF_TEMPORAL_MODE_1REF:
    {
        number_of_References = ONE_REFERENCE;
        DefaultIdx2Out = 0;
        CurrentIdx2Out = 0;
        MctfState = AMCTF_READY;
        buffer_size = ONE_REFERENCE + 1;
    };
    break;
    case MCTF_TEMPORAL_MODE_SPATIAL:
    { //refNum == 0
        number_of_References = NO_REFERENCES;
        DefaultIdx2Out = 0;
        CurrentIdx2Out = 0;
        MctfState = AMCTF_READY;
        buffer_size = ONE_REFERENCE ; 
    };
    break;
    default:
        return MFX_ERR_INVALID_VIDEO_PARAM;
    };

    for (mfxU32 i = 0; i < buffer_size; i++) {
        scene_numbers[i] = 0;
        QfIn.push_back(gpuFrameData());
        QfIn[i].fIdx = NULL;
        QfIn[i].idxMag = NULL;//For MRE use
    }

    return MFX_ERR_NONE;
}

mfxStatus CMC::DIM_SET(mfxU16 overlap)
{
//    if (p_ctrl->height <= MINHEIGHT)
    if (p_ctrl->CropH <= MINHEIGHT)
        blsize = 8;
    if (MFX_CODINGOPTION_OFF != overlap &&
        MFX_CODINGOPTION_ON != overlap &&
        MFX_CODINGOPTION_UNKNOWN != overlap)
        return MFX_ERR_INVALID_VIDEO_PARAM;
    overlap_Motion = overlap;
    switch (overlap_Motion)
    {
    case MFX_CODINGOPTION_ON:
    {
        ov_width_bl = (DIVUP(p_ctrl->CropW, blsize) * 2) - 1;
        ov_height_bl = (DIVUP(p_ctrl->CropH, blsize) * 2) - 1;
    }
    break;
    case MFX_CODINGOPTION_UNKNOWN:
    case MFX_CODINGOPTION_OFF:
    {
        ov_width_bl = DIVUP(p_ctrl->CropW, blsize) * 2;
        ov_height_bl = DIVUP(p_ctrl->CropH, blsize) * 2;
    }
    break;
    default:
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }
    int var_sc_area = DIVUP(p_ctrl->CropW, 16) * DIVUP(p_ctrl->CropH, 16);
    distRef.resize(ov_width_bl * ov_height_bl, 0);
    var_sc.resize(var_sc_area);

    return MFX_ERR_NONE;
}


mfxStatus CMC::IM_SURF_SET(CmSurface2D **p_surface, SurfaceIndex **p_idxSurf) {
    res = device->CreateSurface2D(p_ctrl->width, p_ctrl->height, CM_SURFACE_FORMAT_NV12, *p_surface);
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = (*p_surface)->GetIndex(*p_idxSurf);
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    return MFX_ERR_NONE;
}

mfxStatus CMC::IM_SURF_SET(AbstractSurfaceHandle pD3DSurf, CmSurface2D **p_surface, SurfaceIndex **p_idxSurf) {
    res = device->CreateSurface2D(pD3DSurf, *p_surface);
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = (*p_surface)->GetIndex(*p_idxSurf);
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    return MFX_ERR_NONE;
}

mfxStatus CMC::IM_MRE_SURF_SET(CmSurface2D **p_Surface, SurfaceIndex **p_idxSurf) {
    mfxU32 width = SUBMREDIM, height = SUBMREDIM;// , pitch = 0;
    res = device->CreateSurface2D(width * sizeof(mfxI16Pair), height, CM_SURFACE_FORMAT_A8, *p_Surface);
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = (*p_Surface)->GetIndex(*p_idxSurf);
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    return MFX_ERR_NONE;
}

mfxStatus CMC::IM_SURF_SET() {
    for (mfxU32 i = 0; i < QfIn.size(); i++) {
        MFX_SAFE_CALL(IM_MRE_SURF_SET(&QfIn[i].magData, &QfIn[i].idxMag));
        mfxHDLPair handle;
        // if a surface is opaque, need to extract normal surface
        mfxFrameSurface1 *pSurf = m_pCore->GetNativeSurface(QfIn[i].mfxFrame, false);
        QfIn[i].mfxFrame = pSurf ? pSurf : QfIn[i].mfxFrame;
        // GetFrameHDL is used as QfIn[].mfxFrme is allocated via call to Core Alloc functoin
        MFX_SAFE_CALL(m_pCore->GetFrameHDL(QfIn[i].mfxFrame->Data.MemId, reinterpret_cast<mfxHDL*>(&handle)));
        MFX_SAFE_CALL(IM_SURF_SET(reinterpret_cast<AbstractSurfaceHandle>(handle.first), &QfIn[i].frameData, &QfIn[i].fIdx));
        QfIn[i].scene_idx = 0;
    }
    return MFX_ERR_NONE;
}

mfxU16  CMC::MCTF_QUERY_NUMBER_OF_REFERENCES() {
    return number_of_References;
}

mfxU32 CMC::MCTF_GetQueueDepth()
{
    return (mfxU32)QfIn.size();
}

mfxStatus CMC::MCTF_SetMemory(const std::vector<mfxFrameSurface1*> & mfxSurfPool)
{
    if (mfxSurfPool.size() != QfIn.size())
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    else
    {
        auto inp_iter = mfxSurfPool.begin();
        for (auto it = QfIn.begin(); it != QfIn.end(); ++it, ++inp_iter)
            it->mfxFrame = *inp_iter;
        res = IM_SURF_SET();
        MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
        // mco & idxmco will be extracted from an output surface
        mco = nullptr;
        res = IM_SURF_SET(&mco2, &idxMco2);
        MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);

        //Setup for 2 references
        res = GEN_SURF_SET(&mv_1, &mvSys1, &idxMv_1);
        MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
        res = GEN_SURF_SET(&mv_2, &mvSys2, &idxMv_2);
        MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
        if (number_of_References)
        {
        //Setup for 4 references
        res = GEN_SURF_SET(&mv_3, &mvSys3, &idxMv_3);
        MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
        res = GEN_SURF_SET(&mv_4, &mvSys4, &idxMv_4);
        MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
        }

        res = GEN_SURF_SET(&distSurf, &distSys, &idxDist);
        MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
        res = GEN_NoiseSURF_SET(&noiseAnalysisSurf, &noiseAnalysisSys, &idxNoiseAnalysis);
        MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
        return MFX_ERR_NONE;
    }
}

mfxStatus CMC::MCTF_INIT( VideoCORE * core, CmDevice *pCmDevice, const mfxFrameInfo& FrameInfo, const IntMctfParams* pMctfParam)
{
    version = 400;
    argIdx = 0;
    tsWidthFull = 0;
    tsWidth = 0;
    tsHeight = 0;
    bth = 500;
    blsize = 16;
    device = NULL;
    sceneNum = 0;
    countFrames = 0;
    firstFrame = 1;
    lastFrame = 0;
    exeTime = 0;


    //--filter configuration parameters
    m_AutoMode = MCTF_MODE::MCTF_NOT_INITIALIZED_MODE;
    ConfigMode = MCTF_CONFIGURATION::MCTF_NOT_CONFIGURED;
    number_of_References = TWO_REFERENCES;
    bitrate_Adaptation = false;
    deblocking_Control = MFX_CODINGOPTION_OFF;
    bpp = 0.0;
    m_FrameRateExtD = 1;
    m_FrameRateExtN = 0;

    ctrlBuf = 0;
    idxCtrl = 0;
    distSys = 0;
    time = 0;

    qpel1 = 0;
    qpel2 = 0;
    idxMv_1 = 0;

    bufferCount = 0;
    //ME elements
    genxRefs1 = 0;
    genxRefs2 = 0;
    genxRefs3 = 0;
    genxRefs4 = 0;
    idxMv_1 = NULL;
    idxMv_2 = NULL;
    idxMv_3 = NULL;
    idxMv_4 = NULL;

    //Motion Estimation
    programMe = 0;
    kernelMe = 0;

    //MC elements
    mco = 0;
    idxMco = 0;
    mco2 = 0;
    idxMco2 = 0;

    //Motion Compensation
    programMc = 0;
    kernelMcDen = 0;
    kernelMc1r = 0;
    kernelMc2r = 0;
    kernelMc4r = 0;
    //Common elements
    queue = NULL;
    //copyQ  = NULL;
    copyEv = NULL;

//    m_IOPattern = io_pattern;
//    m_ioMode = io_mode;

    if (core)
        m_pCore = core;
    else
        return MFX_ERR_NOT_INITIALIZED;

    if (pCmDevice)
        device = pCmDevice;
    else
        return MFX_ERR_NOT_INITIALIZED;

#ifdef MFX_MCTF_DEBUG_PRINT
    ASC_PRINTF(L"\nMCTF params: is configured with the following parameters(refs:bitrate:strenth:me:overlap:deblock): %d:%f:%d:%d:%d:%d:%d\n",
        pMctfParam->TemporalMode, pMctfParam->BitsPerPixelx100k / 100000.0, pMctfParam->FilterStrength, pMctfParam->subPelPrecision, pMctfParam->Overlap, pMctfParam->Deblocking);
#endif

    mfxStatus sts = MFX_ERR_NONE;
    pSCD.reset(new(ASC));

    IntMctfParams MctfParam{};
    QueryDefaultParams(&MctfParam);

    // if no MctfParams are passed, to use default
    if (!pMctfParam)
        pMctfParam = &MctfParam;

    sts = MCTF_SET_ENV(FrameInfo, pMctfParam);
    MFX_CHECK_STS(sts);
    return sts;
}

mfxStatus CMC::MCTF_SET_ENV(const mfxFrameInfo& FrameInfo, const IntMctfParams* pMctfParam)
{
    IntMctfParams localMctfParam = *pMctfParam;
    mfxStatus sts = MFX_ERR_NONE;
    if (!device)
        return MFX_ERR_NOT_INITIALIZED;

    res = device->CreateQueue(queue);
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    hwSize = 4;
    res = device->GetCaps(CAP_GPU_PLATFORM, hwSize, &hwType);
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);

    task = 0;
    // --- bitrate
    if (localMctfParam.BitsPerPixelx100k) //todo: to do correctcompare with 0.0 
        bitrate_Adaptation = true;
    else 
        bitrate_Adaptation = false;

    // ---- BitsPerPixel
    MCTF_UpdateBitrateInfo(localMctfParam.BitsPerPixelx100k);

    // --- deblock
    if (MFX_CODINGOPTION_ON != localMctfParam.Deblocking &&
        MFX_CODINGOPTION_OFF != localMctfParam.Deblocking &&
        MFX_CODINGOPTION_UNKNOWN != localMctfParam.Deblocking) {
#ifdef MFX_MCTF_DEBUG_PRINT
        ASC_PRINTF("Deblocking option not valid\n");
#endif
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    //Dimensions, control and io
    if (MCTF_TEMPORAL_MODE_SPATIAL == localMctfParam.TemporalMode)
    {
        localMctfParam.Overlap = MFX_CODINGOPTION_OFF;
        localMctfParam.Deblocking = MFX_CODINGOPTION_OFF;
        localMctfParam.subPelPrecision = MFX_MVPRECISION_INTEGER;
    }
    deblocking_Control = localMctfParam.Deblocking;

    // MRE is initialized inside
    sts = SetupMeControl(FrameInfo, localMctfParam.FilterStrength, localMctfParam.subPelPrecision);
    MFX_CHECK_STS(sts);

    sts = MCTF_InitQueue(localMctfParam.TemporalMode);
    MFX_CHECK_STS(sts);

    sts = DIM_SET(localMctfParam.Overlap);
    MFX_CHECK_STS(sts);

    if (bitrate_Adaptation || !localMctfParam.FilterStrength)
        m_AutoMode = MCTF_MODE::MCTF_AUTO_MODE;
    else
        m_AutoMode = MCTF_MODE::MCTF_MANUAL_MODE;

    if (bitrate_Adaptation)
        ConfigMode = MCTF_CONFIGURATION::MCTF_AUT_CA_BA;
    else
    {
        ConfigMode = MCTF_MODE::MCTF_AUTO_MODE == m_AutoMode ? MCTF_CONFIGURATION::MCTF_AUT_CA_NBA : MCTF_CONFIGURATION::MCTF_MAN_NCA_NBA;
    }

    pMCTF_SpDen_func = NULL;
    pMCTF_NOA_func = NULL;
    pMCTF_LOAD_func = NULL;
    pMCTF_ME_func = NULL;
    pMCTF_MERGE_func = NULL;
    pMCTF_func = NULL;

    pMCTF_func = &CMC::MCTF_RUN_MCTF_DEN;
    if (m_AutoMode == MCTF_MODE::MCTF_AUTO_MODE)
        pMCTF_NOA_func = &CMC::noise_estimator;

    if (number_of_References == FOUR_REFERENCES) {
        pMCTF_LOAD_func = &CMC::MCTF_LOAD_4REF;
        pMCTF_ME_func = &CMC::MCTF_RUN_ME_4REF;
#if HALFWORK
        pMCTF_MERGE_func = &CMC::MCTF_RUN_MERGE;
#else
        pMCTF_MERGE_func = &CMC::MCTF_BLEND4R;
#endif
    }
    else if (number_of_References == TWO_REFERENCES) {
        pMCTF_LOAD_func = &CMC::MCTF_LOAD_2REF;
        pMCTF_ME_func = &CMC::MCTF_RUN_ME_2REF;
#if HALFWORK
        pMCTF_MERGE_func = NULL;
#else
        pMCTF_MERGE_func = &CMC::MCTF_RUN_BLEND2R;
#endif
    }
    else if (number_of_References == ONE_REFERENCE) {
        pMCTF_func = &CMC::MCTF_RUN_MCTF_DEN_1Ref;
        pMCTF_LOAD_func = &CMC::MCTF_LOAD_1REF;
        pMCTF_ME_func = &CMC::MCTF_RUN_ME_1REF;
        pMCTF_MERGE_func = &CMC::MCTF_RUN_BLEND;
    }
    else if (number_of_References == NO_REFERENCES) {
        pMCTF_func = &CMC::MCTF_RUN_MCTF_DEN_1Ref;
        pMCTF_LOAD_func = &CMC::MCTF_LOAD_1REF;
        pMCTF_ME_func = NULL;
        pMCTF_MERGE_func = &CMC::MCTF_RUN_AMCTF;
    }
    else
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    //deblocking only if ME is used
    if (MFX_CODINGOPTION_ON == deblocking_Control && 
        (ONE_REFERENCE == number_of_References || 
         TWO_REFERENCES == number_of_References ||
         FOUR_REFERENCES == number_of_References)
        )
        pMCTF_SpDen_func = &CMC::MCTF_RUN_Denoise;

    if (blsize < VMEBLSIZE)
        p_ctrl->th = p_ctrl->th / 4;
    res = device->CreateBuffer(sizeof(MeControlSmall), ctrlBuf);
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = ctrlBuf->WriteSurface((const uint8_t *)p_ctrl.get(), NULL, sizeof(MeControlSmall));
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = ctrlBuf->GetIndex(idxCtrl);
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);

    //Motion Estimation
#ifdef MFX_ENABLE_KERNELS
    if (hwType == PLATFORM_INTEL_BDW)
        res = device->LoadProgram((void *)genx_me_bdw, sizeof(genx_me_bdw), programMe, "nojitter");
    else if (hwType >= PLATFORM_INTEL_SKL && hwType <= PLATFORM_INTEL_CFL)
        res = device->LoadProgram((void *)genx_me_skl, sizeof(genx_me_skl), programMe, "nojitter");
    else
#endif
        return MFX_ERR_UNSUPPORTED;
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    //ME Kernel
    if (MFX_CODINGOPTION_ON == overlap_Motion) 
    {
        res = device->CreateKernel(programMe, CM_KERNEL_FUNCTION(MeP16_1MV_MRE), kernelMe);
        res = device->CreateKernel(programMe, CM_KERNEL_FUNCTION(MeP16bi_1MV2_MRE), kernelMeB);
    }
    else
    if (MFX_CODINGOPTION_OFF == overlap_Motion || MFX_CODINGOPTION_UNKNOWN == overlap_Motion)
    {
        res = device->CreateKernel(programMe, CM_KERNEL_FUNCTION(MeP16_1MV_MRE_8x8), kernelMe);
        res = device->CreateKernel(programMe, CM_KERNEL_FUNCTION(MeP16bi_1MV2_MRE_8x8), kernelMeB);
    }
    else
        return MFX_ERR_INVALID_VIDEO_PARAM;

    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);

    //Motion Compensation
#ifdef MFX_ENABLE_KERNELS
    if (hwType == PLATFORM_INTEL_BDW)
        res = device->LoadProgram((void *)genx_mc_bdw, sizeof(genx_mc_bdw), programMc, "nojitter");
    else if (hwType >= PLATFORM_INTEL_SKL && hwType <= PLATFORM_INTEL_CFL)
        res = device->LoadProgram((void *)genx_mc_skl, sizeof(genx_mc_skl), programMc, "nojitter");
    else
#endif
        return MFX_ERR_UNSUPPORTED;
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);

#ifdef MFX_ENABLE_KERNELS
    if (hwType == PLATFORM_INTEL_BDW)
        res = device->LoadProgram((void *)genx_sd_bdw, sizeof(genx_mc_bdw), programDe, "nojitter");
    else if (hwType >= PLATFORM_INTEL_SKL && hwType <= PLATFORM_INTEL_CFL)
        res = device->LoadProgram((void *)genx_sd_skl, sizeof(genx_mc_skl), programDe, "nojitter");
    else
#endif
        return MFX_ERR_UNSUPPORTED;
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);

    //Denoising No reference Kernel
    res = device->CreateKernel(programDe, CM_KERNEL_FUNCTION(SpatialDenoiser_8x8_NV12), kernelMcDen);
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    //Motion Compensation 1 Ref Kernel
    res = device->CreateKernel(programMc, CM_KERNEL_FUNCTION(McP16_4MV_1SURF_WITH_CHR), kernelMc1r);
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    //Motion Compensation 2 Ref Kernel
    res = device->CreateKernel(programMc, CM_KERNEL_FUNCTION(McP16_4MV_2SURF_WITH_CHR), kernelMc2r);
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    //Motion Compensation 4 Ref Kernel
    if (number_of_References == FOUR_REFERENCES) {
        res = device->CreateKernel(programMc, CM_KERNEL_FUNCTION(MC_MERGE4), kernelMc4r);
        MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    }

    res = device->CreateKernel(programMc, CM_KERNEL_FUNCTION(MC_VAR_SC_CALC), kernelNoise);
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);

    sts= pSCD->Init(p_ctrl->CropW, p_ctrl->CropH, p_ctrl->width, MFX_PICSTRUCT_PROGRESSIVE, device);
    MFX_CHECK_STS(sts);
    sts = pSCD->SetGoPSize(Immediate_GoP);
    MFX_CHECK_STS(sts);
    pSCD->SetControlLevel(0);

    if (m_AutoMode == MCTF_MODE::MCTF_AUTO_MODE) 
    {
        sts = SetFilterStrenght(DEFAULT_FILTER_STRENGTH);
        m_RTParams.FilterStrength = DEFAULT_FILTER_STRENGTH;
    }
    else
    {
        sts = SetFilterStrenght(localMctfParam.FilterStrength);
        m_RTParams.FilterStrength = localMctfParam.FilterStrength;
    }
    // now initialize run-time parameters
    m_RTParams = localMctfParam;
    m_InitRTParams = m_RTParams;

    TimeStart();
    return sts;
}
mfxStatus CMC::MCTF_CheckRTParams(const IntMctfParams* pMctfControl)
{
    mfxStatus sts = MFX_ERR_NONE;
    if (pMctfControl) {
#ifdef MFX_ENABLE_MCTF_EXT
    // check BPP for max value. the threshold must be adjusted for higher bit-depths
        if (pMctfControl->BitsPerPixelx100k > (DEFAULT_BPP))
            sts = MFX_ERR_INVALID_VIDEO_PARAM;
#endif
        if (pMctfControl->FilterStrength > 20 || 0 == pMctfControl->FilterStrength)
            sts = MFX_ERR_INVALID_VIDEO_PARAM;
    }
    return sts;
}
mfxStatus CMC::MCTF_UpdateRTParams(IntMctfParams* pMctfControl)
{
    mfxStatus  sts = MCTF_CheckRTParams(pMctfControl);

    if (pMctfControl && MFX_ERR_NONE == sts)
        m_RTParams = *pMctfControl;
    else
        m_RTParams = m_InitRTParams;
    return MFX_ERR_NONE;
}
mfxStatus CMC::MCTF_UpdateANDApplyRTParams(mfxU8 /* srcNum */)
{
    // deblock can be controled for every mode
    if (MCTF_CONFIGURATION::MCTF_MAN_NCA_NBA == ConfigMode ||
        MCTF_CONFIGURATION::MCTF_AUT_CA_BA == ConfigMode ||
        MCTF_CONFIGURATION::MCTF_AUT_CA_NBA == ConfigMode)
    {

#ifdef MCTF_UPDATE_RT_FRAME_ORDER_BASED
        m_RTParams = QfIn[srcNum].mfxMctfControl;
#endif
        if (MCTF_CONFIGURATION::MCTF_MAN_NCA_NBA == ConfigMode)
        {
            SetFilterStrenght(m_RTParams.FilterStrength);
        }
#ifdef MFX_ENABLE_MCTF_EXT
        deblocking_Control = m_RTParams.Deblocking;
        //deblocking only if ME is used
        if (MFX_CODINGOPTION_ON == deblocking_Control &&
            (ONE_REFERENCE == number_of_References ||
                TWO_REFERENCES == number_of_References ||
                FOUR_REFERENCES == number_of_References)
            )
            pMCTF_SpDen_func = &CMC::MCTF_RUN_Denoise;
        else
            pMCTF_SpDen_func = NULL;

        if (MCTF_CONFIGURATION::MCTF_AUT_CA_BA == ConfigMode)
            MFX_SAFE_CALL (MCTF_UpdateBitrateInfo(m_RTParams.BitsPerPixelx100k));
#endif
    }
    return MFX_ERR_NONE;
}

// todo: what if bitrate is close to 0?
mfxStatus CMC::MCTF_UpdateBitrateInfo(mfxU32 BitsPerPexelx100k)
{
    if (MCTF_CONFIGURATION::MCTF_NOT_CONFIGURED == ConfigMode ||
        MCTF_CONFIGURATION::MCTF_AUT_CA_BA == ConfigMode )
    {
        // todo: intially here was a code that uses 12bpp in case 
        // of no bitrate or FPS numerator are passed;
        // what should be now?
        bpp = BitsPerPexelx100k * 1.0 / MCTF_BITRATE_MULTIPLIER;
        return MFX_ERR_NONE;
    }
    else
    {
        // if any other mode, update the bitrate does not have any effect;
        // let notify a caller about this; 
        // however, its not critical as MCTF can operate further
        return MFX_WRN_VALUE_NOT_CHANGED;
    }
}

mfxStatus CMC::GEN_NoiseSURF_SET(CmSurface2DUP **p_Surface, void **p_Sys, SurfaceIndex **p_idxSurf) {
    surfNoisePitch = 0;
    surfNoiseSize = 0;
//    res = device->GetSurface2DInfo(DIVUP(p_ctrl->width, 16) * sizeof(spatialNoiseAnalysis), DIVUP(p_ctrl->height, 16), CM_SURFACE_FORMAT_A8, surfNoisePitch, surfNoiseSize);
    res = device->GetSurface2DInfo(DIVUP(p_ctrl->CropW, 16) * sizeof(spatialNoiseAnalysis), DIVUP(p_ctrl->CropH, 16), CM_SURFACE_FORMAT_A8, surfNoisePitch, surfNoiseSize);
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);

    *p_Sys = CM_ALIGNED_MALLOC(surfNoiseSize, 0x1000);
    MFX_CHECK(*p_Sys, MFX_ERR_NULL_PTR);
    memset(*p_Sys, 0, surfNoiseSize);
//    res = device->CreateSurface2DUP(DIVUP(p_ctrl->width, 16) * sizeof(spatialNoiseAnalysis), DIVUP(p_ctrl->height, 16), CM_SURFACE_FORMAT_A8, *p_Sys, *p_Surface);
    res = device->CreateSurface2DUP(DIVUP(p_ctrl->CropW, 16) * sizeof(spatialNoiseAnalysis), DIVUP(p_ctrl->CropH, 16), CM_SURFACE_FORMAT_A8, *p_Sys, *p_Surface);
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = (*p_Surface)->GetIndex(*p_idxSurf);
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    return MFX_ERR_NONE;
}
mfxStatus CMC::GEN_SURF_SET(CmSurface2DUP **p_Surface, void **p_Sys, SurfaceIndex **p_idxSurf) {
    surfPitch = 0;
    surfSize = 0;
    res = device->GetSurface2DInfo(ov_width_bl * sizeof(mfxI16Pair), ov_height_bl, CM_SURFACE_FORMAT_A8, surfPitch, surfSize);
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    *p_Sys = CM_ALIGNED_MALLOC(surfSize, 0x1000);
    MFX_CHECK(*p_Sys, MFX_ERR_NULL_PTR);
    memset(*p_Sys, 0, surfSize);
    res = device->CreateSurface2DUP(ov_width_bl * sizeof(mfxI16Pair), ov_height_bl, CM_SURFACE_FORMAT_A8, *p_Sys, *p_Surface);
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = (*p_Surface)->GetIndex(*p_idxSurf);
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    return MFX_ERR_NONE;
}

mfxStatus CMC::MCTF_GetEmptySurface(mfxFrameSurface1** ppSurface )
{
    size_t buffer_size = QfIn.size() - 1;
    if (bufferCount > buffer_size) {
#ifdef MFX_MCTF_DEBUG_PRINT
        ASC_PRINTF("Error: Invalid frame buffer position\n");
#endif
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    if (QfIn[bufferCount].mfxFrame->Data.Locked) {
        *ppSurface = nullptr;
        return MFX_ERR_NONE;
    }
    else
    {
        m_pCore->IncreaseReference(&(QfIn[bufferCount].mfxFrame->Data));
        *ppSurface = QfIn[bufferCount].mfxFrame;
        return MFX_ERR_NONE;
    }
}

mfxStatus CMC::MCTF_PUT_FRAME(mfxU32 sceneNumber, CmSurface2D* OutSurf)
{
    size_t buffer_size = QfIn.size() - 1;
    if (bufferCount > buffer_size) 
    {
#ifdef MFX_MCTF_DEBUG_PRINT
        ASC_PRINTF("Error: Invalid frame buffer position\n");
#endif
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

#ifdef MCTF_UPDATE_RT_FRAME_ORDER_BASED
    QfIn[bufferCount].mfxMctfControl = m_RTParams;
#endif
    QfIn[bufferCount].scene_idx = sceneNumber;
    QfIn[bufferCount].frame_number = countFrames;

    // if outFrame is nullptr, it means we cannot output result;
    if (OutSurf)
    {
        mco = OutSurf;
        INT cmSts = OutSurf->GetIndex(idxMco);
        MFX_CHECK((CM_SUCCESS == cmSts), MFX_ERR_DEVICE_FAILED);
    }
    return MFX_ERR_NONE;
}

mfxStatus CMC::MCTF_UpdateBufferCount() {
    size_t buffer_size = QfIn.size() - 1;
    if (bufferCount > buffer_size)
    {
#ifdef MFX_MCTF_DEBUG_PRINT
        ASC_PRINTF("Error: Invalid frame buffer position\n");
#endif
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    bufferCount = (bufferCount < buffer_size) ? bufferCount + 1 : buffer_size;
    return MFX_ERR_NONE;
}

mfxI32 CMC::MCTF_LOAD_1REF() {
    res = device->CreateVmeSurfaceG7_5(QfIn[1].frameData, &QfIn[0].frameData, NULL, 1, 0, genxRefs1);
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_LOAD_2REF() {
    res = device->CreateVmeSurfaceG7_5(QfIn[1].frameData, &QfIn[0].frameData, &QfIn[2].frameData, 1, 1, genxRefs1);
    MCTF_CHECK_CM_ERR(res, res);
    res = device->CreateVmeSurfaceG7_5(QfIn[1].frameData, &QfIn[2].frameData, NULL, 1, 0, genxRefs2);
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_LOAD_4REF() {
    res = device->CreateVmeSurfaceG7_5(QfIn[2].frameData, &QfIn[1].frameData, &QfIn[3].frameData, 1, 1, genxRefs1);
    MCTF_CHECK_CM_ERR(res, res);
    res = device->CreateVmeSurfaceG7_5(QfIn[2].frameData, &QfIn[3].frameData, NULL, 1, 0, genxRefs2);
    MCTF_CHECK_CM_ERR(res, res);
    res = device->CreateVmeSurfaceG7_5(QfIn[2].frameData, &QfIn[0].frameData, &QfIn[4].frameData, 1, 1, genxRefs3);
    MCTF_CHECK_CM_ERR(res, res);
    res = device->CreateVmeSurfaceG7_5(QfIn[2].frameData, &QfIn[4].frameData, NULL, 1, 0, genxRefs4);
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_SET_KERNELMe(SurfaceIndex *GenxRefs, SurfaceIndex *idxMV,
    mfxU16 start_x, mfxU16 start_y, mfxU8 blSize) {
    argIdx = 0;
    res = kernelMe->SetKernelArg(argIdx++, sizeof(*idxCtrl), idxCtrl);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMe->SetKernelArg(argIdx++, sizeof(*GenxRefs), GenxRefs);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMe->SetKernelArg(argIdx++, sizeof(*idxDist), idxDist);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMe->SetKernelArg(argIdx++, sizeof(*idxMV), idxMV);
    MCTF_CHECK_CM_ERR(res, res);
    // set start mb
    mfxU16Pair start_xy = { start_x, start_y };
    res = kernelMe->SetKernelArg(argIdx++, sizeof(start_xy), &start_xy);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMe->SetKernelArg(argIdx++, sizeof(blSize), &blSize);
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_SET_KERNELMeMRE(
    SurfaceIndex *GenxRefs, SurfaceIndex *idxMV,
    SurfaceIndex *idxMRE, mfxU16 start_x,
    mfxU16 start_y, mfxU8 blSize) {
    argIdx = 0;
    res = kernelMe->SetKernelArg(argIdx++, sizeof(*idxCtrl), idxCtrl);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMe->SetKernelArg(argIdx++, sizeof(*GenxRefs), GenxRefs);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMe->SetKernelArg(argIdx++, sizeof(*idxDist), idxDist);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMe->SetKernelArg(argIdx++, sizeof(*idxMV), idxMV);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMe->SetKernelArg(argIdx++, sizeof(*idxMRE), idxMRE);
    MCTF_CHECK_CM_ERR(res, res);
    // set start mb
    mfxU16Pair start_xy = { start_x, start_y };
    res = kernelMe->SetKernelArg(argIdx++, sizeof(start_xy), &start_xy);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMe->SetKernelArg(argIdx++, sizeof(blSize), &blSize);
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_SET_KERNELMeBi(
    SurfaceIndex *GenxRefs, SurfaceIndex *idxMV,
    SurfaceIndex *idxMV2, mfxU16 start_x,
    mfxU16 start_y, mfxU8 blSize,
    mfxI8 forwardRefDist, mfxI8 backwardRefDist) {
    argIdx = 0;
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(*idxCtrl), idxCtrl);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(*GenxRefs), GenxRefs);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(*idxDist), idxDist);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(*idxMV), idxMV);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(*idxDist), idxDist);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(*idxMV2), idxMV2);
    MCTF_CHECK_CM_ERR(res, res);
    // set start mb
    mfxU16Pair start_xy = { start_x, start_y };
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(start_xy), &start_xy);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(blSize), &blSize);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(forwardRefDist), &forwardRefDist);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(backwardRefDist), &backwardRefDist);
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_SET_KERNELMeBi(
    SurfaceIndex *GenxRefs, SurfaceIndex *idxMV,
    SurfaceIndex *idxMV2, SurfaceIndex *idxMRE1,
    SurfaceIndex *idxMRE2, mfxU16 start_x,
    mfxU16 start_y, mfxU8 blSize,
    mfxI8 forwardRefDist, mfxI8 backwardRefDist) {
    argIdx = 0;
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(*idxCtrl), idxCtrl);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(*GenxRefs), GenxRefs);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(*idxDist), idxDist);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(*idxMV), idxMV);
    MCTF_CHECK_CM_ERR(res, res);
    /*res = kernelMeB->SetKernelArg(argIdx++, sizeof(*idxDist), idxDist);
    MCTF_CHECK_CM_ERR(res);*/
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(*idxMV2), idxMV2);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(*idxMRE1), idxMRE1);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(*idxMRE2), idxMRE2);
    MCTF_CHECK_CM_ERR(res, res);
    // set start mb
    mfxU16Pair start_xy = { start_x, start_y };
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(start_xy), &start_xy);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(blSize), &blSize);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(forwardRefDist), &forwardRefDist);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(backwardRefDist), &backwardRefDist);
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_SET_KERNELMeBi(
    SurfaceIndex *GenxRefs, SurfaceIndex *GenxRefs2,
    SurfaceIndex *idxMV, SurfaceIndex *idxMV2,
    mfxU16 start_x, mfxU16 start_y, mfxU8 blSize,
    mfxI8 forwardRefDist, mfxI8 backwardRefDist) {
    argIdx = 0;
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(*idxCtrl), idxCtrl);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(*GenxRefs), GenxRefs);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(*GenxRefs2), GenxRefs2);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(*idxDist), idxDist);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(SurfaceIndex), idxMV);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(SurfaceIndex), idxMV2);
    MCTF_CHECK_CM_ERR(res, res);
    // set start mb
    mfxU16Pair start_xy = { start_x, start_y };
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(start_xy), &start_xy);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(blSize), &blSize);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(forwardRefDist), &forwardRefDist);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(backwardRefDist), &backwardRefDist);
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_SET_KERNELMeBiMRE(
    SurfaceIndex *GenxRefs, SurfaceIndex *GenxRefs2,
    SurfaceIndex *idxMV, SurfaceIndex *idxMV2,
    SurfaceIndex *idxMRE1, SurfaceIndex *idxMRE2,
    mfxU16 start_x, mfxU16 start_y, mfxU8 blSize,
    mfxI8 forwardRefDist, mfxI8 backwardRefDist) {
    argIdx = 0;
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(*idxCtrl), idxCtrl);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(*GenxRefs), GenxRefs);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(*GenxRefs2), GenxRefs2);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(*idxDist), idxDist);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(*idxMV), idxMV);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(*idxMV2), idxMV2);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(SurfaceIndex), idxMRE1);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(SurfaceIndex), idxMRE2);
    MCTF_CHECK_CM_ERR(res, res);
    // set start mb
    mfxU16Pair start_xy = { start_x, start_y };
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(start_xy), &start_xy);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(blSize), &blSize);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(forwardRefDist), &forwardRefDist);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->SetKernelArg(argIdx++, sizeof(backwardRefDist), &backwardRefDist);
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_SET_KERNELMc(mfxU16 start_x, mfxU16 start_y, mfxU8 srcNum, mfxU8 refNum) {
    argIdx = 0;
    res = kernelMc1r->SetKernelArg(argIdx++, sizeof(*idxCtrl), idxCtrl);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc1r->SetKernelArg(argIdx++, sizeof(*QfIn[refNum].fIdx), QfIn[refNum].fIdx);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc1r->SetKernelArg(argIdx++, sizeof(*idxMv_1), idxMv_1);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc1r->SetKernelArg(argIdx++, sizeof(*QfIn[srcNum].fIdx), QfIn[srcNum].fIdx);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc1r->SetKernelArg(argIdx++, sizeof(*idxMco), idxMco);
    MCTF_CHECK_CM_ERR(res, res);
    // set start mb
    mfxU16Pair start_xy = { start_x, start_y };
    res = kernelMc1r->SetKernelArg(argIdx++, sizeof(start_xy), &start_xy);
    MCTF_CHECK_CM_ERR(res, res);
    //scene numbers
    mfxU8quad scene_num_gpu = { (mfxU8)scene_numbers[0], (mfxU8)scene_numbers[1], (mfxU8)scene_numbers[2], 2 };
    res = kernelMc1r->SetKernelArg(argIdx++, sizeof(scene_num_gpu), &scene_num_gpu);
    MCTF_CHECK_CM_ERR(res, res);
    return res;

}

mfxI32 CMC::MCTF_SET_KERNELMc2r(mfxU16 start_x, mfxU16 start_y) {
    argIdx = 0;
    res = kernelMc2r->SetKernelArg(argIdx++, sizeof(*idxCtrl), idxCtrl);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc2r->SetKernelArg(argIdx++, sizeof(*QfIn[0].fIdx), QfIn[0].fIdx);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc2r->SetKernelArg(argIdx++, sizeof(*idxMv_1), idxMv_1);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc2r->SetKernelArg(argIdx++, sizeof(*QfIn[2].fIdx), QfIn[2].fIdx);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc2r->SetKernelArg(argIdx++, sizeof(*idxMv_2), idxMv_2);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc2r->SetKernelArg(argIdx++, sizeof(*QfIn[1].fIdx), QfIn[1].fIdx);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc2r->SetKernelArg(argIdx++, sizeof(*idxMco), idxMco);
    MCTF_CHECK_CM_ERR(res, res);
    // set start mb
    mfxU16Pair start_xy = { start_x, start_y };
    res = kernelMc2r->SetKernelArg(argIdx++, sizeof(start_xy), &start_xy);
    MCTF_CHECK_CM_ERR(res, res);
    //scene numbers
    mfxU8quad scene_num_gpu = { (mfxU8)scene_numbers[0], (mfxU8)scene_numbers[1], (mfxU8)scene_numbers[2], 2 };
    res = kernelMc2r->SetKernelArg(argIdx++, sizeof(scene_num_gpu), &scene_num_gpu);
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_SET_KERNELMc2rDen(mfxU16 start_x, mfxU16 start_y) {
    argIdx = 0;
    res = kernelMc2r->SetKernelArg(argIdx++, sizeof(*idxCtrl), idxCtrl);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc2r->SetKernelArg(argIdx++, sizeof(*QfIn[0].fIdx), QfIn[0].fIdx);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc2r->SetKernelArg(argIdx++, sizeof(*idxMv_1), idxMv_1);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc2r->SetKernelArg(argIdx++, sizeof(*QfIn[2].fIdx), QfIn[2].fIdx);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc2r->SetKernelArg(argIdx++, sizeof(*idxMv_2), idxMv_2);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc2r->SetKernelArg(argIdx++, sizeof(*QfIn[1].fIdx), QfIn[1].fIdx);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc2r->SetKernelArg(argIdx++, sizeof(*idxMco2), idxMco2);
    MCTF_CHECK_CM_ERR(res, res);
    // set start mb
    mfxU16Pair start_xy = { start_x, start_y };
    res = kernelMc2r->SetKernelArg(argIdx++, sizeof(start_xy), &start_xy);
    MCTF_CHECK_CM_ERR(res, res);
    //scene numbers
    mfxU8quad scene_num_gpu = { (mfxU8)scene_numbers[0], (mfxU8)scene_numbers[1], (mfxU8)scene_numbers[2], 2 };
    res = kernelMc2r->SetKernelArg(argIdx++, sizeof(scene_num_gpu), &scene_num_gpu);
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_SET_KERNELMc4r(mfxU16 start_x, mfxU16 start_y) {
    argIdx = 0;
    res = kernelMc4r->SetKernelArg(argIdx++, sizeof(*idxCtrl), idxCtrl);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc4r->SetKernelArg(argIdx++, sizeof(*idxRef1), idxRef1);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc4r->SetKernelArg(argIdx++, sizeof(*idxMv_1), idxMv_1);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc4r->SetKernelArg(argIdx++, sizeof(*idxRef2), idxRef2);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc4r->SetKernelArg(argIdx++, sizeof(*idxMv_2), idxMv_2);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc4r->SetKernelArg(argIdx++, sizeof(*idxRef3), idxRef3);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc4r->SetKernelArg(argIdx++, sizeof(*idxMv_3), idxMv_3);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc4r->SetKernelArg(argIdx++, sizeof(*idxRef4), idxRef4);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc4r->SetKernelArg(argIdx++, sizeof(*idxMv_4), idxMv_4);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc4r->SetKernelArg(argIdx++, sizeof(*idxSrc), idxSrc);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc4r->SetKernelArg(argIdx++, sizeof(*idxMco), idxMco);
    MCTF_CHECK_CM_ERR(res, res);
    // set start mb
    mfxU16Pair start_xy = { start_x, start_y };
    res = kernelMc4r->SetKernelArg(argIdx++, sizeof(start_xy), &start_xy);
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_SET_KERNELMc4r(mfxU16 start_x, mfxU16 start_y, SurfaceIndex *multiIndex) {
    argIdx = 0;
    res = kernelMc4r->SetKernelArg(argIdx++, sizeof(*idxCtrl), idxCtrl);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc4r->SetKernelArg(argIdx++, sizeof(*multiIndex), multiIndex);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc4r->SetKernelArg(argIdx++, sizeof(*idxSrc), idxSrc);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc4r->SetKernelArg(argIdx++, sizeof(*idxMco), idxMco);
    MCTF_CHECK_CM_ERR(res, res);
    // set start mb
    mfxU16Pair start_xy = { start_x, start_y };
    res = kernelMc4r->SetKernelArg(argIdx++, sizeof(start_xy), &start_xy);
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_SET_KERNELMcMerge(mfxU16 start_x, mfxU16 start_y) {
    argIdx = 0;
    res = kernelMc4r->SetKernelArg(argIdx++, sizeof(*idxMco), idxMco);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc4r->SetKernelArg(argIdx++, sizeof(*idxMco2), idxMco2);
    MCTF_CHECK_CM_ERR(res, res);
    // set start mb
    mfxU16Pair start_xy = { start_x, start_y };
    res = kernelMc4r->SetKernelArg(argIdx++, sizeof(start_xy), &start_xy);
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_SET_KERNELMc4r(mfxU16 start_x, mfxU16 start_y, mfxU8 runType) {
    argIdx = 0;

    mfxU8
        currentFrame = 2,
        pastRef = 1,
        futureRef = 3;
    SurfaceIndex
        **mcOut = &idxMco,
        **pastMv = &idxMv_1,
        **futureMv = &idxMv_2;

    if (runType == DEN_FAR_RUN) {
        pastRef = 0;
        futureRef = 4;
        mcOut = &idxMco2;
        pastMv = &idxMv_3;
        futureMv = &idxMv_4;
    }

    res = kernelMc2r->SetKernelArg(argIdx++, sizeof(*idxCtrl), idxCtrl);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc2r->SetKernelArg(argIdx++, sizeof(*QfIn[pastRef].fIdx), QfIn[pastRef].fIdx);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc2r->SetKernelArg(argIdx++, sizeof(**pastMv), *pastMv);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc2r->SetKernelArg(argIdx++, sizeof(*QfIn[futureRef].fIdx), QfIn[futureRef].fIdx);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc2r->SetKernelArg(argIdx++, sizeof(**futureMv), *futureMv);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc2r->SetKernelArg(argIdx++, sizeof(*QfIn[currentFrame].fIdx), QfIn[currentFrame].fIdx);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc2r->SetKernelArg(argIdx++, sizeof(**mcOut), *mcOut);
    MCTF_CHECK_CM_ERR(res, res);
    // set start mb
    mfxU16Pair start_xy = { start_x, start_y };
    res = kernelMc2r->SetKernelArg(argIdx++, sizeof(start_xy), &start_xy);
    MCTF_CHECK_CM_ERR(res, res);
    //scene numbers
    mfxU8quad scene_num_gpu = { (mfxU8)scene_numbers[pastRef], (mfxU8)scene_numbers[currentFrame], (mfxU8)scene_numbers[futureRef], 2 };
    res = kernelMc2r->SetKernelArg(argIdx++, sizeof(scene_num_gpu), &scene_num_gpu);
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_SET_KERNEL_Noise(mfxU16 srcNum, mfxU16 start_x, mfxU16 start_y) {
    argIdx = 0;
    res = kernelNoise->SetKernelArg(argIdx++, sizeof(*QfIn[srcNum].fIdx), QfIn[srcNum].fIdx);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelNoise->SetKernelArg(argIdx++, sizeof(*idxNoiseAnalysis), idxNoiseAnalysis);
    MCTF_CHECK_CM_ERR(res, res);
    // set start mb
    mfxU16Pair start_xy = { start_x, start_y };
    res = kernelNoise->SetKernelArg(argIdx++, sizeof(start_xy), &start_xy);
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_SET_KERNELDe(mfxU16 srcNum, mfxU16 start_x, mfxU16 start_y) {
    argIdx = 0;
    res = kernelMcDen->SetKernelArg(argIdx++, sizeof(*idxCtrl), idxCtrl);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMcDen->SetKernelArg(argIdx++, sizeof(*QfIn[srcNum].fIdx), QfIn[srcNum].fIdx);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMcDen->SetKernelArg(argIdx++, sizeof(*idxMco), idxMco);
    MCTF_CHECK_CM_ERR(res, res);
    // set start mb
    mfxU16Pair start_xy = { start_x, start_y };
    res = kernelMcDen->SetKernelArg(argIdx++, sizeof(start_xy), &start_xy);
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_SET_KERNELDe(mfxU16 start_x, mfxU16 start_y) {
    argIdx = 0;
    res = kernelMcDen->SetKernelArg(argIdx++, sizeof(*idxCtrl), idxCtrl);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMcDen->SetKernelArg(argIdx++, sizeof(*idxMco), idxMco);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMcDen->SetKernelArg(argIdx++, sizeof(*idxMco), idxMco);
    MCTF_CHECK_CM_ERR(res, res);
    // set start mb
    mfxU16Pair start_xy = { start_x, start_y };
    res = kernelMcDen->SetKernelArg(argIdx++, sizeof(start_xy), &start_xy);
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_RUN_TASK_NA(CmKernel *kernel, bool reset,
    mfxU16 widthTs, mfxU16 heightTs) {
    res = kernel->SetThreadCount(widthTs * heightTs);
    MCTF_CHECK_CM_ERR(res, res);
    res = device->CreateThreadSpace(widthTs, heightTs, threadSpace);
    MCTF_CHECK_CM_ERR(res, res);
    res = threadSpace->SelectThreadDependencyPattern(CM_NONE_DEPENDENCY);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernel->AssociateThreadSpace(threadSpace);

    if (reset) {
        res = task->Reset();
        MCTF_CHECK_CM_ERR(res, res);
    }
    else {
        res = device->CreateTask(task);
        MCTF_CHECK_CM_ERR(res, res);
    }
    res = task->AddKernel(kernel);
    MCTF_CHECK_CM_ERR(res, res);
    res = device->CreateQueue(queue);
    MCTF_CHECK_CM_ERR(res, res);
    res = queue->Enqueue(task, e);
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_RUN_TASK(CmKernel *kernel, bool reset) {
    res = kernel->SetThreadCount(tsWidth * tsHeight);
    MCTF_CHECK_CM_ERR(res, res);
    res = device->CreateThreadSpace(tsWidth, tsHeight, threadSpace);
    MCTF_CHECK_CM_ERR(res, res);
    res = threadSpace->SelectThreadDependencyPattern(CM_NONE_DEPENDENCY);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernel->AssociateThreadSpace(threadSpace);

    if (reset) {
        res = task->Reset();
        MCTF_CHECK_CM_ERR(res, res);
    }
    else {
        res = device->CreateTask(task);
        MCTF_CHECK_CM_ERR(res, res);
    }
    res = task->AddKernel(kernel);
    MCTF_CHECK_CM_ERR(res, res);
    res = device->CreateQueue(queue);
    MCTF_CHECK_CM_ERR(res, res);
    res = queue->Enqueue(task, e);
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_RUN_DOUBLE_TASK(CmKernel *meKernel, CmKernel *mcKernel, bool reset) {
    res = meKernel->SetThreadCount(tsWidth * tsHeight);
    MCTF_CHECK_CM_ERR(res, res);
    res = device->CreateThreadSpace(tsWidth, tsHeight, threadSpace2);
    MCTF_CHECK_CM_ERR(res, res);
    res = threadSpace2->SelectThreadDependencyPattern(CM_NONE_DEPENDENCY);
    MCTF_CHECK_CM_ERR(res, res);
    res = meKernel->AssociateThreadSpace(threadSpace2);

    res = mcKernel->SetThreadCount(tsWidthMC * tsHeightMC);
    MCTF_CHECK_CM_ERR(res, res);
    res = device->CreateThreadSpace(tsWidthMC, tsHeightMC, threadSpaceMC);
    MCTF_CHECK_CM_ERR(res, res);
    res = threadSpaceMC->SelectThreadDependencyPattern(CM_NONE_DEPENDENCY);
    MCTF_CHECK_CM_ERR(res, res);
    res = mcKernel->AssociateThreadSpace(threadSpaceMC);

    if (reset) {
        res = task->Reset();
        MCTF_CHECK_CM_ERR(res, res);
    }
    else {
        res = device->CreateTask(task);
        MCTF_CHECK_CM_ERR(res, res);
    }
    res = task->AddKernel(meKernel);
    MCTF_CHECK_CM_ERR(res, res);
    res = task->AddKernel(mcKernel);
    MCTF_CHECK_CM_ERR(res, res);
    res = device->CreateQueue(queue);
    MCTF_CHECK_CM_ERR(res, res);
    res = queue->Enqueue(task, e);
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_RUN_MCTASK(CmKernel *kernel, bool reset) {
    res = kernel->SetThreadCount(tsWidthMC * tsHeightMC);
    MCTF_CHECK_CM_ERR(res, res);
    res = device->CreateThreadSpace(tsWidthMC, tsHeightMC, threadSpaceMC2);
    MCTF_CHECK_CM_ERR(res, res);
    res = threadSpaceMC2->SelectThreadDependencyPattern(CM_NONE_DEPENDENCY);
    MCTF_CHECK_CM_ERR(res, res);
    //res = kernel->AssociateThreadSpace(threadSpaceMC2);

    if (reset) {
        res = task->Reset();
        MCTF_CHECK_CM_ERR(res, res);
    }
    else {
        res = device->CreateTask(task);
        MCTF_CHECK_CM_ERR(res, res);
    }
    res = task->AddKernel(kernel);
    MCTF_CHECK_CM_ERR(res, res);
    res = device->CreateQueue(queue);
    MCTF_CHECK_CM_ERR(res, res);
    res = queue->Enqueue(task, e, threadSpaceMC2);
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_RUN_TASK(CmKernel *kernel, bool reset, CmThreadSpace *tS) {
    res = kernel->SetThreadCount(tsWidth * tsHeight);
    MCTF_CHECK_CM_ERR(res, res);
    res = device->CreateThreadSpace(tsWidth, tsHeight, tS);
    MCTF_CHECK_CM_ERR(res, res);
    res = tS->SelectThreadDependencyPattern(CM_NONE_DEPENDENCY);
    MCTF_CHECK_CM_ERR(res, res);
    if (reset) {
        res = task->Reset();
        MCTF_CHECK_CM_ERR(res, res);
    }
    else {
        res = device->CreateTask(task);
        MCTF_CHECK_CM_ERR(res, res);
    }
    res = task->AddKernel(kernel);
    MCTF_CHECK_CM_ERR(res, res);
    res = device->CreateQueue(queue);
    MCTF_CHECK_CM_ERR(res, res);
    res = queue->Enqueue(task, e, tS);
    MCTF_CHECK_CM_ERR(res, res);
    res = device->DestroyThreadSpace(tS);
    return res;
}

mfxI32 CMC::MCTF_RUN_ME(SurfaceIndex *GenxRefs, SurfaceIndex *idxMV) {
    time = 0;
    mfxU8 blSize = VMEBLSIZE / 2;
//    tsHeight = (DIVUP(p_ctrl->height, VMEBLSIZE) * 2) - 1;//Motion Estimation of any block size is performed in 16x16 units
//    tsWidthFull = (DIVUP(p_ctrl->width, VMEBLSIZE) * 2) - 1;
    tsHeight = (DIVUP(p_ctrl->CropH, VMEBLSIZE) * 2) - 1;//Motion Estimation of any block size is performed in 16x16 units
    tsWidthFull = (DIVUP(p_ctrl->CropW, VMEBLSIZE) * 2) - 1;
    tsWidth = tsWidthFull;
//    res = MCTF_SET_KERNELMe(GenxRefs, idxMV, 0, 0, blSize);
    res = MCTF_SET_KERNELMe(GenxRefs, idxMV, DIVUP(p_ctrl->CropX, blSize), DIVUP(p_ctrl->CropY, blSize),  blSize);
    MCTF_CHECK_CM_ERR(res, res);
    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW) {
        tsWidth = (tsWidthFull >> 1) & ~1;  // must be even for 32x32 blocks 
    }
    threadSpace = 0;
    res = MCTF_RUN_TASK(kernelMe, task != 0);
    MCTF_CHECK_CM_ERR(res, res);

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW) {
        mfxU16 start_mbX = tsWidth;
        tsWidth = tsWidthFull - tsWidth;
//        res = MCTF_SET_KERNELMe(GenxRefs, idxMV, start_mbX, 0, blSize);
        res = MCTF_SET_KERNELMe(GenxRefs, idxMV, start_mbX, DIVUP(p_ctrl->CropY, blSize), blSize);
        MCTF_CHECK_CM_ERR(res, res);
        // the rest of frame TS
        res = MCTF_RUN_TASK(kernelMe, task != 0);
        MCTF_CHECK_CM_ERR(res, res);
    }
    res = e->WaitForTaskFinished();
    MCTF_CHECK_CM_ERR(res, res);
#if TIMINGINFO
    time += GetAccurateGpuTime(queue, task, threadSpace);
#endif
    UINT64 executionTime;
    e->GetExecutionTime(executionTime);
    exeTime += executionTime / 1000;
    device->DestroyThreadSpace(threadSpace);
    queue->DestroyEvent(e);
    device->DestroyVmeSurfaceG7_5(GenxRefs);
    e = 0;
    return res;
}
mfxI32 CMC::MCTF_RUN_ME(
    SurfaceIndex *GenxRefs,
    SurfaceIndex *idxMV, SurfaceIndex *idxMV2,
    char forwardRefDist, char backwardRefDist) {
    time = 0;
    mfxU8 blSize = VMEBLSIZE / 2;
//    tsHeight = (DIVUP(p_ctrl->height, VMEBLSIZE) * 2) - 1;//Motion Estimation of any block size is performed in 16x16 units
//    tsWidthFull = (DIVUP(p_ctrl->width, VMEBLSIZE) * 2) - 1;
    tsHeight = (DIVUP(p_ctrl->CropH, VMEBLSIZE) * 2) - 1;//Motion Estimation of any block size is performed in 16x16 units
    tsWidthFull = (DIVUP(p_ctrl->CropW, VMEBLSIZE) * 2) - 1;
    tsWidth = tsWidthFull;
//    res = MCTF_SET_KERNELMeBi(GenxRefs, idxMV, idxMV2, 0, 0, blSize, forwardRefDist, backwardRefDist);
    res = MCTF_SET_KERNELMeBi(GenxRefs, idxMV, idxMV2, DIVUP(p_ctrl->CropX, blSize), DIVUP(p_ctrl->CropY, blSize), blSize, forwardRefDist, backwardRefDist);
    MCTF_CHECK_CM_ERR(res, res);

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW) {
        tsWidth = (tsWidthFull >> 1) & ~1;  // must be even for 32x32 blocks 
    }

    threadSpace = 0;
    res = MCTF_RUN_TASK(kernelMeB, task != 0);
    MCTF_CHECK_CM_ERR(res, res);

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW) {
        mfxU16 start_mbX = tsWidth;
        tsWidth = tsWidthFull - tsWidth;
//        res = MCTF_SET_KERNELMeBi(GenxRefs, idxMV, idxMV2, start_mbX, 0, blSize, forwardRefDist, backwardRefDist);
        res = MCTF_SET_KERNELMeBi(GenxRefs, idxMV, idxMV2, start_mbX, DIVUP(p_ctrl->CropY, blSize), blSize, forwardRefDist, backwardRefDist);
        MCTF_CHECK_CM_ERR(res, res);
        res = MCTF_RUN_TASK(kernelMeB, task != 0);
        MCTF_CHECK_CM_ERR(res, res);
    }
    res = e->WaitForTaskFinished();
    MCTF_CHECK_CM_ERR(res, res);
#if TIMINGINFO
    time += GetAccurateGpuTime(queue, task, threadSpace);
#endif
    UINT64 executionTime;
    e->GetExecutionTime(executionTime);
    exeTime += executionTime / 1000;
    device->DestroyThreadSpace(threadSpace);
    queue->DestroyEvent(e);
    device->DestroyVmeSurfaceG7_5(GenxRefs);
    e = 0;
    return res;
}

mfxI32 CMC::MCTF_RUN_ME_MRE(
    SurfaceIndex *GenxRefs,
    SurfaceIndex *idxMV, SurfaceIndex *idxMV2,
    SurfaceIndex *idxMRE1, SurfaceIndex *idxMRE2,
    char forwardRefDist, char backwardRefDist) {
    time = 0;
    mfxU8 blSize = VMEBLSIZE/* / 2*/;
//    tsHeight = DIVUP(p_ctrl->height, VMEBLSIZE);/*(DIVUP(p_ctrl->height, VMEBLSIZE) * 2) - 1;*///Motion Estimation of any block size is performed in 16x16 units
//    tsWidthFull = DIVUP(p_ctrl->width, VMEBLSIZE);/*(DIVUP(p_ctrl->width,  VMEBLSIZE) * 2) - 1;*/
    tsHeight = DIVUP(p_ctrl->CropH, VMEBLSIZE);/*(DIVUP(p_ctrl->height, VMEBLSIZE) * 2) - 1;*///Motion Estimation of any block size is performed in 16x16 units
    tsWidthFull = DIVUP(p_ctrl->CropW, VMEBLSIZE);/*(DIVUP(p_ctrl->width,  VMEBLSIZE) * 2) - 1;*/

    tsWidth = tsWidthFull;
//    res = MCTF_SET_KERNELMeBi(GenxRefs, idxMV, idxMV2, idxMRE1, idxMRE2, 0, 0, blSize, forwardRefDist, backwardRefDist);
    res = MCTF_SET_KERNELMeBi(GenxRefs, idxMV, idxMV2, idxMRE1, idxMRE2, DIVUP(p_ctrl->CropX, blSize), DIVUP(p_ctrl->CropY, blSize), blSize, forwardRefDist, backwardRefDist);
    MCTF_CHECK_CM_ERR(res, res);
    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW) {
        tsWidth = (tsWidthFull >> 1) & ~1;  // must be even for 32x32 blocks 
    }

    threadSpace = 0;
    res = MCTF_RUN_TASK(kernelMeB, task != 0);
    MCTF_CHECK_CM_ERR(res, res);

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW) {
        mfxU16 start_mbX = tsWidth;
        tsWidth = tsWidthFull - tsWidth;
//        res = MCTF_SET_KERNELMeBi(GenxRefs, idxMV, idxMV2, start_mbX, 0, blSize, forwardRefDist, backwardRefDist);
        res = MCTF_SET_KERNELMeBi(GenxRefs, idxMV, idxMV2, start_mbX, DIVUP(p_ctrl->CropY, blSize), blSize, forwardRefDist, backwardRefDist);
        MCTF_CHECK_CM_ERR(res, res);
        // the rest of frame TS
        res = MCTF_RUN_TASK(kernelMeB, task != 0);
        MCTF_CHECK_CM_ERR(res, res);
    }
    res = e->WaitForTaskFinished();
    MCTF_CHECK_CM_ERR(res, res);
#if TIMINGINFO
    time += GetAccurateGpuTime(queue, task, threadSpace);
#endif
    UINT64 executionTime;
    e->GetExecutionTime(executionTime);
    exeTime += executionTime / 1000;
    device->DestroyThreadSpace(threadSpace);
    queue->DestroyEvent(e);
    device->DestroyVmeSurfaceG7_5(GenxRefs);
    e = 0;
    return res;
}

mfxU8 CMC::SetOverlapOp() {
    mfxU8 blSize = 0;
    switch (overlap_Motion)
    {
    case MFX_CODINGOPTION_ON:
    {
        blSize = VMEBLSIZE / 2;
        tsHeight = (DIVUP(p_ctrl->CropH, VMEBLSIZE) * 2) - 1;//Motion Estimation of any block size is performed in 16x16 units
        tsWidthFull = (DIVUP(p_ctrl->CropW, VMEBLSIZE) * 2) - 1;
    }
    break;
    case MFX_CODINGOPTION_UNKNOWN:
    case MFX_CODINGOPTION_OFF:
    {
        blSize = VMEBLSIZE;
        tsHeight = DIVUP(p_ctrl->CropH, VMEBLSIZE);//Motion Estimation of any block size is performed in 16x16 units
        tsWidthFull = DIVUP(p_ctrl->CropW, VMEBLSIZE);
    }
    break;
    default:
        throw CMCRuntimeError();
    }
    tsWidth = tsWidthFull;
    return blSize;
}

mfxU8 CMC::SetOverlapOp_half() {
    mfxU8 blSize = 0;
    switch (overlap_Motion)
    {
    case MFX_CODINGOPTION_ON:
    {
        blSize = VMEBLSIZE / 2;
        tsHeight = DIVUP(p_ctrl->CropH, VMEBLSIZE) - 1;//Motion Estimation of any block size is performed in 16x16 units
        tsWidthFull = (DIVUP(p_ctrl->CropW, VMEBLSIZE) * 2) - 1;
    }
    break;

    case MFX_CODINGOPTION_OFF:
    case MFX_CODINGOPTION_UNKNOWN:
    {
        blSize = VMEBLSIZE;
        tsHeight = DIVUP(p_ctrl->CropH, VMEBLSIZE) / 2;//Motion Estimation of any block size is performed in 16x16 units
        tsWidthFull = DIVUP(p_ctrl->CropW, VMEBLSIZE);
    }
    break;
    default:
        throw CMCRuntimeError();
    }
    tsWidth = tsWidthFull;
    return blSize;
}

mfxI32 CMC::MCTF_RUN_ME(
    SurfaceIndex *GenxRefs, SurfaceIndex *GenxRefs2,
    SurfaceIndex *idxMV, SurfaceIndex *idxMV2,
    char forwardRefDist, char backwardRefDist) {
    time = 0;
    mfxU8 blSize = VMEBLSIZE / 2;
//    tsHeight = (DIVUP(p_ctrl->height, VMEBLSIZE) * 2) - 1;//Motion Estimation of any block size is performed in 16x16 units
//    tsWidthFull = (DIVUP(p_ctrl->width, VMEBLSIZE) * 2) - 1;
    tsHeight = (DIVUP(p_ctrl->CropH, VMEBLSIZE) * 2) - 1;//Motion Estimation of any block size is performed in 16x16 units
    tsWidthFull = (DIVUP(p_ctrl->CropW, VMEBLSIZE) * 2) - 1;
    tsWidth = tsWidthFull;
//    res = MCTF_SET_KERNELMeBi(GenxRefs, GenxRefs2, idxMV, idxMV2, 0, 0, blSize, forwardRefDist, backwardRefDist);
    res = MCTF_SET_KERNELMeBi(GenxRefs, GenxRefs2, idxMV, idxMV2, DIVUP(p_ctrl->CropX, blSize), DIVUP(p_ctrl->CropY, blSize), blSize, forwardRefDist, backwardRefDist);
    MCTF_CHECK_CM_ERR(res, res);
    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW) {
        tsWidth = (tsWidthFull >> 1) & ~1;  // must be even for 32x32 blocks 
    }

    threadSpace = 0;
    res = MCTF_RUN_TASK(kernelMeB, task != 0);
    MCTF_CHECK_CM_ERR(res, res);

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW) {
        mfxU16 start_mbX = tsWidth;
        tsWidth = tsWidthFull - tsWidth;
//        res = MCTF_SET_KERNELMeBi(GenxRefs, GenxRefs2, idxMV, idxMV2, start_mbX, 0, blSize, forwardRefDist, backwardRefDist);
        res = MCTF_SET_KERNELMeBi(GenxRefs, GenxRefs2, idxMV, idxMV2, start_mbX, DIVUP(p_ctrl->CropY, blSize), blSize, forwardRefDist, backwardRefDist);
        MCTF_CHECK_CM_ERR(res, res);
        // the rest of frame TS
        res = MCTF_RUN_TASK(kernelMeB, task != 0);
        MCTF_CHECK_CM_ERR(res, res);
    }
    res = e->WaitForTaskFinished();
    MCTF_CHECK_CM_ERR(res, res);
#if TIMINGINFO
    time += GetAccurateGpuTime(queue, task, threadSpace);
#endif
    UINT64 executionTime;
    e->GetExecutionTime(executionTime);
    exeTime += executionTime / 1000;
    device->DestroyThreadSpace(threadSpace);
    queue->DestroyEvent(e);
    device->DestroyVmeSurfaceG7_5(GenxRefs);
    device->DestroyVmeSurfaceG7_5(GenxRefs2);
    e = 0;
    return res;
}

mfxI32 CMC::MCTF_WAIT_ME_MRE(SurfaceIndex *GenxRefs) {
    res = e->WaitForTaskFinished();
    MCTF_CHECK_CM_ERR(res, res);
    UINT64 executionTime;
    e->GetExecutionTime(executionTime);
    exeTime += executionTime / 1000;
    device->DestroyThreadSpace(threadSpace);
    queue->DestroyEvent(e);
    device->DestroyVmeSurfaceG7_5(GenxRefs);
    e = 0;
    return res;
}

mfxI32 CMC::MCTF_WAIT_ME_MRE(
    SurfaceIndex *GenxRefs, SurfaceIndex *GenxRefs2) {
    res = e->WaitForTaskFinished();
    MCTF_CHECK_CM_ERR(res, res);
    UINT64 executionTime;
    e->GetExecutionTime(executionTime);
    exeTime += executionTime / 1000;
    device->DestroyThreadSpace(threadSpace);
    queue->DestroyEvent(e);
    device->DestroyVmeSurfaceG7_5(GenxRefs);
    device->DestroyVmeSurfaceG7_5(GenxRefs2);
    e = 0;
    return res;
}

mfxI32 CMC::MCTF_RUN_ME_MRE(SurfaceIndex *GenxRefs, SurfaceIndex *idxMV, SurfaceIndex *idxMRE) {
    time = 0;

    mfxU8
        blSize = SetOverlapOp();
    //res = MCTF_SET_KERNELMeMRE(GenxRefs, idxMV, idxMRE, 0, 0, blSize);
    res = MCTF_SET_KERNELMeMRE(GenxRefs, idxMV, idxMRE, DIVUP(p_ctrl->CropX, blSize) , DIVUP(p_ctrl->CropY, blSize), blSize);
    MCTF_CHECK_CM_ERR(res, res);
    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW) {
        tsWidth = (tsWidthFull >> 1) & ~1;  // must be even for 32x32 blocks 
    }

    threadSpace = 0;
    res = MCTF_RUN_TASK(kernelMe, task != 0);
    MCTF_CHECK_CM_ERR(res, res);

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW) {
        mfxU16 start_mbX = tsWidth;
        tsWidth = tsWidthFull - tsWidth;
//        res = MCTF_SET_KERNELMeMRE(GenxRefs, idxMV, idxMRE, start_mbX, 0, blSize);
        res = MCTF_SET_KERNELMeMRE(GenxRefs, idxMV, idxMRE, start_mbX, DIVUP(p_ctrl->CropY, blSize), blSize);
        MCTF_CHECK_CM_ERR(res, res);
        // the rest of frame TS
        res = MCTF_RUN_TASK(kernelMe, task != 0);
        MCTF_CHECK_CM_ERR(res, res);
    }
    return res;
}


mfxI32 CMC::MCTF_RUN_ME_MC_H(
    SurfaceIndex *GenxRefs, SurfaceIndex *GenxRefs2,
    SurfaceIndex *idxMV, SurfaceIndex *idxMV2,
    SurfaceIndex *idxMRE1, SurfaceIndex *idxMRE2,
    char forwardRefDist, char backwardRefDist,
    mfxU8 mcSufIndex) {
#if !_MRE_
    idxMRE1, idxMRE2;
#endif
    time = 0;

    mfxU8
        blSize = SetOverlapOp_half();
//    res = MCTF_SET_KERNELMeBiMRE(GenxRefs, GenxRefs2, idxMV, idxMV2, idxMRE1, idxMRE2, 0, 0, blSize, forwardRefDist, backwardRefDist);
    res = MCTF_SET_KERNELMeBiMRE(GenxRefs, GenxRefs2, idxMV, idxMV2, idxMRE1, idxMRE2, DIVUP(p_ctrl->CropX, blSize), DIVUP(p_ctrl->CropY, blSize), blSize, forwardRefDist, backwardRefDist);

    MCTF_CHECK_CM_ERR(res, res);

    threadSpace = 0;

    res = MCTF_RUN_TASK(kernelMeB, task != 0);
    MCTF_CHECK_CM_ERR(res, res);
    res = e->WaitForTaskFinished();
    MCTF_CHECK_CM_ERR(res, res);
    res = device->DestroyThreadSpace(threadSpace);
    MCTF_CHECK_CM_ERR(res, res);

    res = device->DestroyTask(task);
    MCTF_CHECK_CM_ERR(res, res);
    UINT64 executionTime;
    e->GetExecutionTime(executionTime);
    exeTime += executionTime / 1000;
    res = queue->DestroyEvent(e);
    MCTF_CHECK_CM_ERR(res, res);
    task = 0;
    e = 0;

    mfxU16 multiplier = 2;
//    tsHeightMC = (DIVUP(p_ctrl->height, blsize) * multiplier);
//    tsWidthFullMC = (DIVUP(p_ctrl->width, blsize) * multiplier);
    tsHeightMC = (DIVUP(p_ctrl->CropH, blsize) * multiplier);
    tsWidthFullMC = (DIVUP(p_ctrl->CropW, blsize) * multiplier);
    tsWidthMC = tsWidthFullMC;

    if (pMCTF_NOA_func)
        (this->*(pMCTF_NOA_func))();

    //res = MCTF_SET_KERNELMeBiMRE(GenxRefs, GenxRefs2, idxMV, idxMV2, idxMRE1, idxMRE2, 0, tsHeight, blSize, forwardRefDist, backwardRefDist);
    res = MCTF_SET_KERNELMeBiMRE(GenxRefs, GenxRefs2, idxMV, idxMV2, idxMRE1, idxMRE2, DIVUP(p_ctrl->CropX, blsize), tsHeight, blSize, forwardRefDist, backwardRefDist);

    MCTF_CHECK_CM_ERR(res, res);
    if (mcSufIndex == 0) {
        //        res = MCTF_SET_KERNELMc2r(0, 0);
        res = MCTF_SET_KERNELMc2r(DIVUP(p_ctrl->CropX, blSize)*2, DIVUP(p_ctrl->CropY, blSize)*2);
    }
    else if (mcSufIndex == 1) {
        //        res = MCTF_SET_KERNELMc4r(0, 0, DEN_CLOSE_RUN);
        res = MCTF_SET_KERNELMc4r(DIVUP(p_ctrl->CropX, blSize)*2, DIVUP(p_ctrl->CropY, blSize)*2, DEN_CLOSE_RUN);
    }
    else {
        //        res = MCTF_SET_KERNELMc4r(0, 0, DEN_FAR_RUN);
        res = MCTF_SET_KERNELMc4r(DIVUP(p_ctrl->CropX, blSize)*2, DIVUP(p_ctrl->CropY, blSize)*2, DEN_FAR_RUN);
    }
    MCTF_CHECK_CM_ERR(res, res);

    threadSpace = 0;
    threadSpaceMC = 0;

    MCTF_RUN_DOUBLE_TASK(kernelMeB, kernelMc2r, task != 0);
    MCTF_CHECK_CM_ERR(res, res);
    res = e->WaitForTaskFinished();
    MCTF_CHECK_CM_ERR(res, res);
    e->GetExecutionTime(executionTime);
    exeTime += executionTime / 1000;
    res = device->DestroyThreadSpace(threadSpace2);
    MCTF_CHECK_CM_ERR(res, res);
    res = device->DestroyThreadSpace(threadSpaceMC);
    MCTF_CHECK_CM_ERR(res, res);

    res = device->DestroyVmeSurfaceG7_5(GenxRefs);
    MCTF_CHECK_CM_ERR(res, res);
    res = device->DestroyVmeSurfaceG7_5(GenxRefs2);
    MCTF_CHECK_CM_ERR(res, res);
    res = device->DestroyTask(task);
    MCTF_CHECK_CM_ERR(res, res);
    res = queue->DestroyEvent(e);
    task = 0;
    e = 0;
    return res;
}

mfxI32 CMC::MCTF_RUN_ME_MRE(
    SurfaceIndex *GenxRefs, SurfaceIndex *GenxRefs2,
    SurfaceIndex *idxMV, SurfaceIndex *idxMV2,
    SurfaceIndex *idxMRE1, SurfaceIndex *idxMRE2,
    char forwardRefDist, char backwardRefDist) {
    time = 0;

    mfxU8
        blSize = SetOverlapOp();
    res = MCTF_SET_KERNELMeBiMRE(GenxRefs, GenxRefs2, idxMV, idxMV2, idxMRE1, idxMRE2, 0, 0, blSize, forwardRefDist, backwardRefDist);
    MCTF_CHECK_CM_ERR(res, res);
    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW) {
        tsWidth = (tsWidthFull >> 1) & ~1;  // must be even for 32x32 blocks 
    }

    threadSpace = 0;
    res = MCTF_RUN_TASK(kernelMeB, task != 0);
    MCTF_CHECK_CM_ERR(res, res);

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW) {
        mfxU16
            start_mbX = tsWidth;
        tsWidth = tsWidthFull - tsWidth;
        res = MCTF_SET_KERNELMeBiMRE(GenxRefs, GenxRefs2, idxMV, idxMV2, idxMRE1, idxMRE2, start_mbX, 0, blSize, forwardRefDist, backwardRefDist);

        MCTF_CHECK_CM_ERR(res, res);
        // the rest of frame TS
        res = MCTF_RUN_TASK(kernelMeB, task != 0);
        MCTF_CHECK_CM_ERR(res, res);
    }
    return res;
}


void CMC::GET_DISTDATA() {
    for (int y = 0; y < ov_height_bl; y++)
        memcpy_s(distRef.data() + y * ov_width_bl, sizeof(mfxU32) * ov_width_bl * ov_height_bl,(char *)distSys + y * surfPitch, sizeof(mfxU32) * ov_width_bl);
}

void CMC::GET_DISTDATA_H() {
    for (int y = 0; y < ov_height_bl / 2; y++)
        memcpy_s(distRef.data() + y * ov_width_bl, sizeof(mfxU32) * ov_width_bl * ov_height_bl, (char *)distSys + y * surfPitch, sizeof(mfxU32) * ov_width_bl);
}

void CMC::GET_NOISEDATA() {
    int var_sc_area = DIVUP(p_ctrl->CropW, 16) * DIVUP(p_ctrl->CropH, 16);
    for (int y = 0; y < DIVUP(p_ctrl->CropH, 16); y++)
        memcpy_s(var_sc.data() + y * DIVUP(p_ctrl->CropW, 16), var_sc_area, (char *)noiseAnalysisSys + y * surfNoisePitch, sizeof(spatialNoiseAnalysis) * DIVUP(p_ctrl->CropW, 16));
}

mfxF64 CMC::GET_TOTAL_SAD() {
    mfxF64 total_sad = 0.0;
    // to not lose due-to normalization of floats
    mfxU64 uTotalSad = 0;
    switch (overlap_Motion)
    {
    case MFX_CODINGOPTION_ON:
    {//overlapped modes, need to remove extra SAD blocks and lines
        mfxI32
            pos_offset = 0;
        for (mfxI32 i = 0; i < ov_height_bl; i += OVERLAP_OFFSET) {
            pos_offset = i * ov_width_bl;
            for (mfxI32 j = 0; j < ov_width_bl; j += OVERLAP_OFFSET)
                uTotalSad += distRef[pos_offset + j];
        }
    }
    break;
    case MFX_CODINGOPTION_UNKNOWN:
    case MFX_CODINGOPTION_OFF:
    {// Non overlapped, all blocks are needed for frame SAD calculation
        for (mfxU32 i = 0; i < distRef.size(); i++)
            uTotalSad += distRef[i];
    }
    break;
    default:
        throw CMCRuntimeError();
    }
//    return total_sad / (p_ctrl->width * p_ctrl->height);
    total_sad = mfxF64(uTotalSad);
    return total_sad / (p_ctrl->CropW * p_ctrl->CropH);

}

mfxU16 CalcNoiseStrength(double NSC, double NSAD)
{
    // 10 epsilons
    if (std::fabs(NSC) <= 10 * std::numeric_limits<double>::epsilon()) return 0;
    mfxF64
        s,
#if defined (MCTF_MODEL_FOR_x264_OR_x265)
        c3 = 251.3383273,//189.199402,
        c2 = -257.236,//-259.6469397,
        c1 = 101.9745,//125.4713269,
        c0 = -8.38753,//-13.64224334,
#elif defined (MCTF_MODEL_FOR_MSDK)
        // optimal for AVC/HEVC, MSDK implementation
        c3 = 247.99601,
        c2 = -195.205078,
        c1 = 46.510905,
        c0 = -0.736656,
#else
        static_assert(false, "no macro for a MCTF model is defined!");
#endif
        STC = NSAD / sqrt(NSC);
    s = c3 * pow(STC, 3.0) + c2 * pow(STC, 2.0) + c1 * STC + c0;
    s = max(0.0, min(20.0, s));
    return (mfxU16)(s + 0.5);
}


mfxU32 CalcSTC(mfxF64 SCpp2, mfxF64 sadpp) {
    mfxU32
        stcVal = 0;
    sadpp *= sadpp;
    // Ref was Recon (quantization err in ref biases sad)
    if (sadpp < 0.03*SCpp2)  stcVal = 0; // Very-Low
    else if (sadpp < 0.09*SCpp2)  stcVal = 1; // Low
    else if (sadpp < 0.20*SCpp2)  stcVal = 2; // Low-Medium
    else if (sadpp < 0.36*SCpp2)  stcVal = 3; // Medium
    else if (sadpp < 1.44*SCpp2)  stcVal = 4; // Medium-High
    else if (sadpp < 3.24*SCpp2)  stcVal = 5; // High
    else                          stcVal = 6; // VeryHigh
    return stcVal;
}


void CMC::GetSpatioTemporalComplexityFrame(mfxU8 currentFrame)
{
    mfxU32
        i;
    mfxF64
        SCpp2 = QfIn[currentFrame].frame_sc;
    static mfxF32
        lmt_sc2[10] = { 16.0, 81.0, 225.0, 529.0, 1024.0, 1764.0, 2809.0, 4225.0, 6084.0, (mfxF32)INT_MAX }; // lower limit of SFM(Rs,Cs) range for spatial classification

    for (i = 0; i < 10; i++) {
        if (SCpp2 < lmt_sc2[i]) {
            QfIn[currentFrame].sc = i;
            break;
        }
    }
    QfIn[currentFrame].tc = 0;
    QfIn[currentFrame].stc = 0;

    mfxF64
        sadpp = QfIn[currentFrame].frame_sad;

    static mfxF64
        lmt_tc[10] = { 0.75, 1.5, 2.25, 3.00, 4.00, 5.00, 6.00, 7.50, 9.25, mfxF64(INT_MAX) };               // lower limit of AFD
    for (i = 0; i < 10; i++) {
        if (sadpp < lmt_tc[i]) {
            QfIn[currentFrame].tc = i;
            break;
        }
    }

    QfIn[currentFrame].stc = CalcSTC(SCpp2, sadpp);
}

mfxU32 CMC::computeQpClassFromBitRate(mfxU8 currentFrame)
{
    mfxF64
        scL = log10(QfIn[currentFrame].sc),
        sadL = log10(QfIn[currentFrame].frame_sad);
    mfxF64
        d0 = sadL * scL,
        A, B = -0.75;
    d0 = pow(d0, 2.03);
    // 0.567701x + 1.092071
    A = 0.567701 * d0 + 1.092071;

    mfxU32         //    0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53
        QP_CLASS[54] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 };
    mfxF64
        y, qs;
    y = A * pow(bpp, B);
    qs = 1.0 + log(y) / log(2.0);

    mfxU32
        QP = (mfxU32)(6.0 * qs) + 4;
    mfxU32
        QCL = (QP > 53) ? 3 : QP_CLASS[QP];
    //QCL = (QP < 0) ? 0 : ((QP > 53) ? 3 : QP_CLASS[QP]);

    return QCL;
}


void CMC::noise_estimator() 
{
    mfxU8
        currentFrame = (number_of_References <= 2) ? 1 : 2;
    mfxU32
//        width = DIVUP(p_ctrl->width, 16),
//        height = DIVUP(p_ctrl->height, 16),
        width = DIVUP(p_ctrl->CropW, 16),
        height = DIVUP(p_ctrl->CropH, 16),
        count = 0,
        row, col;
    mfxI16
        fsModVal = 0;
    mfxF32
        tvar = 281,
        var, SCpp, SADpp;

    QfIn[currentFrame].noise_var = 0.0,
    QfIn[currentFrame].noise_sad = 0.0,
    QfIn[currentFrame].noise_sc = 0.0;
    QfIn[currentFrame].noise_count = 1;

    QfIn[currentFrame].frame_sc = 0.0;
    QfIn[currentFrame].frame_sad = 0.0;
    QfIn[currentFrame].frame_Cs = 0.0;
    QfIn[currentFrame].frame_Rs = 0.0;

    GET_DISTDATA_H();
    res = MCTF_RUN_Noise_Analysis(currentFrame);
    GET_NOISEDATA();
    mfxU32 distRefStride = 2 * width;
    switch (overlap_Motion)
    {
    case MFX_CODINGOPTION_ON:
    {
        for (row = 1; row < height / 2 - 1; row++) {
            for (col = 1; col < width - 1; col++) {
                var = var_sc[row * width + col].var;
                SCpp = var_sc[row * width + col].SCpp;
                QfIn[currentFrame].frame_sc += SCpp;
                SADpp = distRef[row * 2 * width * 2 + col * 2] / 256;
                QfIn[currentFrame].frame_sad += SADpp;
                if (var < tvar && SCpp <tvar && SCpp>1.0 && (SADpp*SADpp) <= SCpp) {
                    count++;
                    QfIn[currentFrame].noise_var += var;
                    QfIn[currentFrame].noise_sc += SCpp;
                    QfIn[currentFrame].noise_sad += SADpp;
                }
            }
        }
    }
    break;
    case MFX_CODINGOPTION_UNKNOWN:
    case MFX_CODINGOPTION_OFF:
    {
        for (row = 1; row < height / 2 - 1; row++) {
            for (col = 1; col < width - 1; col++) {
                var = var_sc[row * width + col].var;
                SCpp = var_sc[row * width + col].SCpp;
                QfIn[currentFrame].frame_sc += SCpp;
                // NB: division by 256 is done in integers;
                // thus, for each calculation, 8 LSB are truncated
                // works good; but additional efforts can be taken,
                // to consider higher precision (thru calculations in integers)
                // followed by wise truncating.
                SADpp = (distRef[row * 2 * distRefStride + col * 2] +
                    distRef[row * 2 * distRefStride + col * 2 + 1] +
                    distRef[(row * 2 + 1) * distRefStride + col * 2] +
                    distRef[(row * 2 + 1) * distRefStride + col * 2 + 1]) / 256;
                QfIn[currentFrame].frame_sad += SADpp;
                if (var < tvar && SCpp <tvar && SCpp>1.0 && (SADpp*SADpp) <= SCpp) 
                {
                    ++count;
                    QfIn[currentFrame].noise_var += var;
                    QfIn[currentFrame].noise_sc += SCpp;
                    QfIn[currentFrame].noise_sad += SADpp;

                }
            }
        }
    }
    break;
    default:
        throw CMCRuntimeError();
    }
    QfIn[currentFrame].frame_sc /= ((height / 2 - 2) * (width - 2));
    QfIn[currentFrame].frame_sad /= ((height / 2 - 2) * (width - 2));

    if (count)
    {
        // noise_count is reserved for future use.
        QfIn[currentFrame].noise_count = count;
        QfIn[currentFrame].noise_var /= count;
        QfIn[currentFrame].noise_sc /= count;
        QfIn[currentFrame].noise_sad /= count;
    }
    mfxU16
        filterStrenght = CalcNoiseStrength(QfIn[currentFrame].noise_sc, QfIn[currentFrame].noise_sad);
    if (bitrate_Adaptation)
    {
        GetSpatioTemporalComplexityFrame(currentFrame);
        mfxU32
            QLC = computeQpClassFromBitRate(currentFrame);
        if (QLC == 0)
            fsModVal = QfIn[currentFrame].stc > 0.35 ? -2 : -1;
        else if (QLC == 1)
            fsModVal = 0;
        else if (QLC == 2)
            fsModVal = 1;
        else
            fsModVal = 2;

        mfxI32 limit = filterStrenght > 14 ? 13 : 20;
        filterStrenght = mfxU16(max(0, min(limit, (mfxI16)filterStrenght + fsModVal)));
    }
    res = SetFilterStrenght(filterStrenght);
}

mfxI32 CMC::MCTF_RUN_Noise_Analysis(mfxU8 srcNum) {
//    res = MCTF_SET_KERNEL_Noise(srcNum, 0, 0);
    res = MCTF_SET_KERNEL_Noise(srcNum, DIVUP(p_ctrl->CropX, 16), DIVUP(p_ctrl->CropY, 16));
    MCTF_CHECK_CM_ERR(res, res);
    mfxU16
        tsHeightNA = DIVUP(p_ctrl->CropH, 16),
        tsWidthFullNA = DIVUP(p_ctrl->CropW, 16),
        tsWidthNA = tsWidthFullNA;

    if (tsWidthFullNA > CM_MAX_THREADSPACE_WIDTH_FOR_MW) {
        tsWidthNA = (tsWidthFullNA >> 1) & ~1;  // must be even for 32x32 blocks 
    }

    threadSpace = 0;
    res = MCTF_RUN_TASK_NA(kernelNoise, task != 0, tsWidthNA, tsHeightNA);
    MCTF_CHECK_CM_ERR(res, res);

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW) {
        mfxU16
            start_mbX = tsWidth;
        tsWidth = tsWidthFull - tsWidth;
        res = MCTF_SET_KERNEL_Noise(srcNum, start_mbX, DIVUP(p_ctrl->CropY, 16));
        MCTF_CHECK_CM_ERR(res, res);
        if (threadSpace != NULL) {
            res = device->DestroyThreadSpace(threadSpace);
            MCTF_CHECK_CM_ERR(res, res);
        }
        // the rest of frame TS
        res = MCTF_RUN_TASK_NA(kernelNoise, task != 0, tsWidthNA, tsHeightNA);
        MCTF_CHECK_CM_ERR(res, res);
    }
    res = e->WaitForTaskFinished();
    MCTF_CHECK_CM_ERR(res, res);
#if TIMINGINFO
    time += GetAccurateGpuTime(queue, task, threadSpace);
#ifdef MFX_MCTF_DEBUG_PRINT
    ASC_PRINTF("TIME=%.3fms ", time / 1000000.0);
#endif
#endif
    device->DestroyThreadSpace(threadSpace);
    queue->DestroyEvent(e);
    e = 0;
    return res;
}

mfxI32 CMC::MCTF_RUN_BLEND() {
    mfxU16 multiplier = 2;
    //    res = MCTF_SET_KERNELMc(0, 0, 1, 0);
    res = MCTF_SET_KERNELMc(DIVUP(p_ctrl->CropX, blsize) * multiplier, DIVUP(p_ctrl->CropY, blsize) * multiplier, 1, 0);
    MCTF_CHECK_CM_ERR(res, res);
    tsHeight = (DIVUP(p_ctrl->CropH, blsize) * multiplier);
    tsWidthFull = (DIVUP(p_ctrl->CropW, blsize) * multiplier);
    tsWidth = tsWidthFull;

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW) {
        tsWidth = (tsWidthFull >> 1) & ~1;  // must be even for 32x32 blocks 
    }

    threadSpace = 0;
    res = MCTF_RUN_TASK(kernelMc1r, task != 0);
    MCTF_CHECK_CM_ERR(res, res);

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW) {
        mfxU16
            start_mbX = tsWidth;
        tsWidth = tsWidthFull - tsWidth;
        res = MCTF_SET_KERNELMc(start_mbX, DIVUP(p_ctrl->CropY, blsize) * multiplier, 1, 0);
        MCTF_CHECK_CM_ERR(res, res);
        // the rest of frame TS
        res = MCTF_RUN_TASK(kernelMc1r, task != 0);
        MCTF_CHECK_CM_ERR(res, res);
    }
    res = e->WaitForTaskFinished();
    MCTF_CHECK_CM_ERR(res, res);
    UINT64 executionTime;
    e->GetExecutionTime(executionTime);
    exeTime += executionTime / 1000;
#if TIMINGINFO
    time += GetAccurateGpuTime(queue, task, threadSpace);
#ifdef MFX_MCTF_DEBUG_PRINT
    ASC_PRINTF("TIME=%.3fms ", time / 1000000.0);
#endif
#endif
    device->DestroyThreadSpace(threadSpace);
    queue->DestroyEvent(e);
    e = 0;
    return res;
}

mfxI32 CMC::MCTF_RUN_BLEND(mfxU8 srcNum, mfxU8 refNum) {
    mfxU16 multiplier = 2;
    //    res = MCTF_SET_KERNELMc(0, 0, srcNum, refNum);
    res = MCTF_SET_KERNELMc(DIVUP(p_ctrl->CropX, blsize) * multiplier , DIVUP(p_ctrl->CropY, blsize) * multiplier, srcNum, refNum);
    MCTF_CHECK_CM_ERR(res, res);
    tsHeight = (DIVUP(p_ctrl->CropH, blsize) * multiplier);
    tsWidthFull = (DIVUP(p_ctrl->CropW, blsize) * multiplier);
    tsWidth = tsWidthFull;

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW) {
        tsWidth = (tsWidthFull >> 1) & ~1;  // must be even for 32x32 blocks 
    }

    threadSpace = 0;
    res = MCTF_RUN_TASK(kernelMc1r, task != 0);
    MCTF_CHECK_CM_ERR(res, res);

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW) {
        mfxU16
            start_mbX = tsWidth;
        tsWidth = tsWidthFull - tsWidth;
        res = MCTF_SET_KERNELMc(start_mbX, DIVUP(p_ctrl->CropY, blsize) * multiplier, srcNum, refNum);
        MCTF_CHECK_CM_ERR(res, res);
        // the rest of frame TS
        res = MCTF_RUN_TASK(kernelMc1r, task != 0);
        MCTF_CHECK_CM_ERR(res, res);
    }
    res = e->WaitForTaskFinished();
    MCTF_CHECK_CM_ERR(res, res);
#if TIMINGINFO
    time += GetAccurateGpuTime(queue, task, threadSpace);
#ifdef MFX_MCTF_DEBUG_PRINT
    ASC_PRINTF("TIME=%.3fms ", time / 1000000.0);
#endif
#endif
    device->DestroyThreadSpace(threadSpace);
    queue->DestroyEvent(e);
    e = 0;
    return res;
}

mfxI32 CMC::MCTF_RUN_BLEND2R() {
    mfxU16 multiplier = 2;
    res = MCTF_SET_KERNELMc2r(DIVUP(p_ctrl->CropX, blsize) * multiplier, DIVUP(p_ctrl->CropY, blsize) * multiplier);
    MCTF_CHECK_CM_ERR(res, res);

    tsHeightMC = (DIVUP(p_ctrl->CropH, blsize) * multiplier);
    tsWidthFullMC = (DIVUP(p_ctrl->CropW, blsize) * multiplier);
    tsWidthMC = tsWidthFullMC;

    if (tsWidthFullMC > CM_MAX_THREADSPACE_WIDTH_FOR_MW) {
        tsWidthMC = (tsWidthFullMC >> 1) & ~1;  // must be even for 32x32 blocks 
    }

    threadSpaceMC = 0;
    res = MCTF_RUN_MCTASK(kernelMc2r, task != 0);
    MCTF_CHECK_CM_ERR(res, res);

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW) {
        mfxU16 start_mbX = tsWidthMC;
        tsWidthMC = tsWidthFullMC - tsWidthMC;
        res = MCTF_SET_KERNELMc2r(start_mbX, DIVUP(p_ctrl->CropY, blsize) * multiplier);
        MCTF_CHECK_CM_ERR(res, res);
        // the rest of frame TS
        res = MCTF_RUN_MCTASK(kernelMc2r, task != 0);
        MCTF_CHECK_CM_ERR(res, res);
    }
    res = e->WaitForTaskFinished();
    MCTF_CHECK_CM_ERR(res, res);
#if TIMINGINFO
    time += GetAccurateGpuTime(queue, task, threadSpace);
#ifdef MFX_MCTF_DEBUG_PRINT
    ASC_PRINTF("TIME=%.3fms ", time / 1000000.0);
#endif
#endif
    UINT64 executionTime;
    e->GetExecutionTime(executionTime);
    exeTime += executionTime / 1000;
    device->DestroyThreadSpace(threadSpaceMC);
    queue->DestroyEvent(e);
    e = 0;
    return res;
}

mfxI32 CMC::MCTF_RUN_BLEND2R_DEN() {
    mfxU16 multiplier = 2;
    //res = MCTF_SET_KERNELMc2rDen(0, 0);
    res = MCTF_SET_KERNELMc2rDen(DIVUP(p_ctrl->CropX, blsize) * multiplier, DIVUP(p_ctrl->CropY, blsize) * multiplier);
    MCTF_CHECK_CM_ERR(res, res);
    tsHeight = (DIVUP(p_ctrl->CropH, blsize) * multiplier);
    tsWidthFull = (DIVUP(p_ctrl->CropW, blsize) * multiplier);
    tsWidth = tsWidthFull;

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW) {
        tsWidth = (tsWidthFull >> 1) & ~1;  // must be even for 32x32 blocks 
    }

    threadSpace = 0;
    res = MCTF_RUN_TASK(kernelMc2r, task != 0);
    MCTF_CHECK_CM_ERR(res, res);

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW) {
        mfxU16
            start_mbX = tsWidth;
        tsWidth = tsWidthFull - tsWidth;

        res = MCTF_SET_KERNELMc2rDen(start_mbX, DIVUP(p_ctrl->CropY, blsize) * multiplier);
        MCTF_CHECK_CM_ERR(res, res);
        // the rest of frame TS
        res = MCTF_RUN_TASK(kernelMc2r, task != 0);
        MCTF_CHECK_CM_ERR(res, res);
    }
    res = e->WaitForTaskFinished();
    MCTF_CHECK_CM_ERR(res, res);
#if TIMINGINFO
    time += GetAccurateGpuTime(queue, task, threadSpace);
#ifdef MFX_MCTF_DEBUG_PRINT
    ASC_PRINTF("TIME=%.3fms ", time / 1000000.0);
#endif
#endif
    device->DestroyThreadSpace(threadSpace);
    queue->DestroyEvent(e);
    e = 0;
    return res;
}

mfxI32 CMC::MCTF_RUN_BLEND4R(DRT typeRun) {
    if (typeRun > 1) {
#ifdef MFX_MCTF_DEBUG_PRINT
        ASC_PRINTF("MCTF_RUN_BLEND4R: typRun > 1!\n");
#endif
        return -2666;
    }
    mfxU16 multiplier = 2;
    res = MCTF_SET_KERNELMc4r(DIVUP(p_ctrl->CropX, blsize) * multiplier, DIVUP(p_ctrl->CropY, blsize) * multiplier, typeRun);
    MCTF_CHECK_CM_ERR(res, res);
    tsHeight = (DIVUP(p_ctrl->CropH, blsize) * multiplier);
    tsWidthFull = (DIVUP(p_ctrl->CropW, blsize) * multiplier);
    tsWidth = tsWidthFull;

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW) {
        tsWidth = (tsWidthFull >> 1) & ~1;  // must be even for 32x32 blocks 
    }

    threadSpace = 0;
    res = MCTF_RUN_TASK(kernelMc2r, task != 0);
    MCTF_CHECK_CM_ERR(res, res);

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW) {
        mfxU16
            start_mbX = tsWidth;
        tsWidth = tsWidthFull - tsWidth;
//        res = MCTF_SET_KERNELMc4r(start_mbX, 0, typeRun);
        res = MCTF_SET_KERNELMc4r(start_mbX, DIVUP(p_ctrl->CropY, blsize) * multiplier, typeRun);
        MCTF_CHECK_CM_ERR(res, res);
        // the rest of frame TS
        res = MCTF_RUN_TASK(kernelMc2r, task != 0);
        MCTF_CHECK_CM_ERR(res, res);
    }
    res = e->WaitForTaskFinished();
    MCTF_CHECK_CM_ERR(res, res);
#if TIMINGINFO
    time += GetAccurateGpuTime(queue, task, threadSpace);
#ifdef MFX_MCTF_DEBUG_PRINT
    ASC_PRINTF("TIME=%.3fms ", time / 1000000.0);
#endif
#endif
    device->DestroyThreadSpace(threadSpace);
    queue->DestroyEvent(e);
    e = 0;
    return res;
}

mfxI32 CMC::MCTF_RUN_MERGE() {
    //res = MCTF_SET_KERNELMcMerge(0, 0);
    res = MCTF_SET_KERNELMcMerge(DIVUP(p_ctrl->CropX, 16), DIVUP(p_ctrl->CropY, 16));
    MCTF_CHECK_CM_ERR(res, res);
    //tsHeight = DIVUP(p_ctrl->height, 16);
    //tsWidthFull = DIVUP(p_ctrl->width, 16);
    tsHeight = DIVUP(p_ctrl->CropH, 16);
    tsWidthFull = DIVUP(p_ctrl->CropW, 16);
    tsWidth = tsWidthFull;

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW) {
        tsWidth = (tsWidthFull >> 1) & ~1;  // must be even for 32x32 blocks 
    }

    threadSpace = 0;
    res = MCTF_RUN_TASK(kernelMc4r, task != 0);
    MCTF_CHECK_CM_ERR(res, res);

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW) {
        mfxU16
            start_mbX = tsWidth;
        tsWidth = tsWidthFull - tsWidth;
        //        res = MCTF_SET_KERNELMc4r(start_mbX, 0);
        res = MCTF_SET_KERNELMc4r(start_mbX, DIVUP(p_ctrl->CropY, 16));
        MCTF_CHECK_CM_ERR(res, res);
        // the rest of frame TS
        res = MCTF_RUN_TASK(kernelMc4r, task != 0);
        MCTF_CHECK_CM_ERR(res, res);
    }
    res = e->WaitForTaskFinished();
    MCTF_CHECK_CM_ERR(res, res);
#if TIMINGINFO
    time += GetAccurateGpuTime(queue, task, threadSpace);
#ifdef MFX_MCTF_DEBUG_PRINT
    ASC_PRINTF("TIME=%.3fms ", time / 1000000.0);
#endif
#endif
    device->DestroyThreadSpace(threadSpace);
    queue->DestroyEvent(e);
    e = 0;
    return res;
    }

mfxI32 CMC::MCTF_RUN_BLEND4R(SurfaceIndex *multiIndex) {
    mfxU16 multiplier = 2;
    //    res = MCTF_SET_KERNELMc4r(0, 0, multiIndex);
    res = MCTF_SET_KERNELMc4r(DIVUP(p_ctrl->CropX, blsize) * multiplier, DIVUP(p_ctrl->CropY, blsize) * multiplier, multiIndex);
    MCTF_CHECK_CM_ERR(res, res);
    //    tsHeight = (DIVUP(p_ctrl->height, blsize) * multiplier);
    //    tsWidthFull = (DIVUP(p_ctrl->width, blsize) * multiplier);
    tsHeight = (DIVUP(p_ctrl->CropH, blsize) * multiplier);
    tsWidthFull = (DIVUP(p_ctrl->CropW, blsize) * multiplier);
    tsWidth = tsWidthFull;

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW) {
        tsWidth = (tsWidthFull >> 1) & ~1;  // must be even for 32x32 blocks 
    }

    threadSpace = 0;
    res = MCTF_RUN_TASK(kernelMc2r, task != 0);
    MCTF_CHECK_CM_ERR(res, res);

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW) {
        mfxU16
            start_mbX = tsWidth;
        tsWidth = tsWidthFull - tsWidth;
        //        res = MCTF_SET_KERNELMc4r(start_mbX, 0, multiIndex);
        res = MCTF_SET_KERNELMc4r(start_mbX, DIVUP(p_ctrl->CropY, blsize) * multiplier, multiIndex);
        MCTF_CHECK_CM_ERR(res, res);
        res = MCTF_RUN_TASK(kernelMc2r, task != 0);
        MCTF_CHECK_CM_ERR(res, res);
    }
    res = e->WaitForTaskFinished();
    MCTF_CHECK_CM_ERR(res, res);
#if TIMINGINFO
    time += GetAccurateGpuTime(queue, task, threadSpace);
#ifdef MFX_MCTF_DEBUG_PRINT
    ASC_PRINTF("TIME=%.3fms ", time / 1000000.0);
#endif
#endif
    device->DestroyThreadSpace(threadSpace);
    queue->DestroyEvent(e);
    e = 0;
    return res;
    }

void CMC::RotateBufferA() {
    std::swap(QfIn[0], QfIn[1]);
    std::swap(QfIn[0], QfIn[2]);
}

void CMC::RotateBufferB() {
    std::swap(QfIn[0], QfIn[1]);
    std::swap(QfIn[1], QfIn[2]);
    std::swap(QfIn[2], QfIn[3]);
}

void CMC::RotateBuffer() {
    mfxU8 correction = ((QfIn.size() > 3) && (firstFrame < 3)) << 1;
    for (mfxU8 i = 0; i < QfIn.size() - 1 - correction; i++) {
        std::swap(QfIn[i], QfIn[i + 1]);
    }
}

void CMC::AssignSceneNumber() {
    for (mfxU8 i = 0; i < QfIn.size(); i++)
        scene_numbers[i] = QfIn[i].scene_idx;
}

mfxI32 CMC::MCTF_RUN_ME_1REF() {
    res = MCTF_RUN_ME_MRE(genxRefs1, idxMv_1, QfIn[0].idxMag);
    MCTF_CHECK_CM_ERR(res, res);
    res = MCTF_WAIT_ME_MRE(genxRefs1);
    MCTF_CHECK_CM_ERR(res, res);

    if (pMCTF_NOA_func)
        (this->*(pMCTF_NOA_func))();

    return res;
}

mfxI32 CMC::MCTF_RUN_ME_2REF() {
#if HALFWORK
    res = MCTF_RUN_ME_MC_H(genxRefs1, genxRefs2, idxMv_1, idxMv_2, QfIn[0].idxMag, QfIn[1].idxMag, forward_distance, backward_distance, 0);
    MCTF_CHECK_CM_ERR(res, res);
#else
    res = MCTF_RUN_ME_MRE(genxRefs1, genxRefs2, idxMv_1, idxMv_2, QfIn[0].idxMag, QfIn[1].idxMag, -1);
    MCTF_CHECK_CM_ERR(res, res);
    res = MCTF_WAIT_ME_MRE(genxRefs1, genxRefs2);
    MCTF_CHECK_CM_ERR(res, res);

    if (pMCTF_NOA_func)
        (this->*(pMCTF_NOA_func))();
#endif
    return res;
}

mfxI32 CMC::MCTF_RUN_ME_4REF() {
#if HALFWORK
    res = MCTF_RUN_ME_MC_H(genxRefs1, genxRefs2, idxMv_1, idxMv_2, QfIn[1].idxMag, QfIn[2].idxMag, forward_distance, backward_distance, 1);
    MCTF_CHECK_CM_ERR(res, res);
    res = MCTF_RUN_ME_MC_H(genxRefs3, genxRefs4, idxMv_3, idxMv_4, QfIn[0].idxMag, QfIn[3].idxMag, forward_distance, backward_distance, 2);
#else
    res = MCTF_RUN_ME_MRE(genxRefs1, genxRefs2, idxMv_1, idxMv_2, QfIn[1].idxMag, QfIn[2].idxMag, forward_distance, backward_distance);
    MCTF_CHECK_CM_ERR(res, res);
    res = MCTF_WAIT_ME_MRE(genxRefs1, genxRefs2);
    MCTF_CHECK_CM_ERR(res, res);

    if (pMCTF_NOA_func)
        (this->*(pMCTF_NOA_func))();

    res = MCTF_RUN_ME_MRE(genxRefs3, genxRefs4, idxMv_3, idxMv_4, QfIn[0].idxMag, QfIn[3].idxMag, forward_distance, backward_distance);
    MCTF_CHECK_CM_ERR(res, res);
    res = MCTF_WAIT_ME_MRE(genxRefs3, genxRefs4);
    MCTF_CHECK_CM_ERR(res, res);
#endif
    return res;
}


mfxI32 CMC::MCTF_BLEND4R() {
    res = MCTF_RUN_BLEND4R(DEN_CLOSE_RUN);
    MCTF_CHECK_CM_ERR(res, res);
    res = MCTF_RUN_BLEND4R(DEN_FAR_RUN);
    MCTF_CHECK_CM_ERR(res, res);
    res = MCTF_RUN_MERGE();
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_RUN_MCTF_DEN_1Ref() {
    CMC::TimeStart(2);
    if (pMCTF_LOAD_func) {
        res = (this->*(pMCTF_LOAD_func))();
        MCTF_CHECK_CM_ERR(res, res);
    }
    AssignSceneNumber();
    if (pMCTF_ME_func) {
        res = (this->*(pMCTF_ME_func))();
        MCTF_CHECK_CM_ERR(res, res);
    }
    if (pMCTF_MERGE_func) {
        res = (this->*(pMCTF_MERGE_func))();
        MCTF_CHECK_CM_ERR(res, res);
    }
    if (pMCTF_SpDen_func)
        res = (this->*(pMCTF_SpDen_func))();
    RotateBuffer();
    //    pTimer->calctime += CatchTime(2, 3, "MCTF calc:", PRINTTIMING);
    return res;
}

mfxI32 CMC::MCTF_RUN_MCTF_DEN() {
    CMC::TimeStart(2);
    if (pMCTF_LOAD_func) {
        res = (this->*(pMCTF_LOAD_func))();
        MCTF_CHECK_CM_ERR(res, res);
    }
    AssignSceneNumber();
    if (pMCTF_ME_func) {
        res = (this->*(pMCTF_ME_func))();
        MCTF_CHECK_CM_ERR(res, res);
    }
    if (pMCTF_MERGE_func) {
        res = (this->*(pMCTF_MERGE_func))();
        MCTF_CHECK_CM_ERR(res, res);
    }
    if (pMCTF_SpDen_func)
        res = (this->*(pMCTF_SpDen_func))();
    RotateBuffer();
    //    pTimer->calctime += CatchTime(2, 3, "MCTF calc:", PRINTTIMING);
    return res;
}

mfxI32 CMC::MCTF_RUN_MCTF_DEN_4REF() {
    CMC::TimeStart(2);
    res = (this->*(pMCTF_LOAD_func))();
    MCTF_CHECK_CM_ERR(res, res);
    AssignSceneNumber();
    res = (this->*(pMCTF_ME_func))();
    MCTF_CHECK_CM_ERR(res, res);
    res = (this->*(pMCTF_MERGE_func))();
    MCTF_CHECK_CM_ERR(res, res);
    if (pMCTF_SpDen_func)
        res = (this->*(pMCTF_SpDen_func))();
    RotateBuffer();
    //    pTimer->calctime += CatchTime(2, 3, "MCTF calc:", PRINTTIMING);
    return res;
}

mfxI32 CMC::MCTF_RUN_AMCTF_DEN() {
    CMC::TimeStart(2);
    res = (this->*(pMCTF_LOAD_func))();
    MCTF_CHECK_CM_ERR(res, res);
    AssignSceneNumber();
    res = MCTF_RUN_ME_MRE(genxRefs1, genxRefs2, idxMv_1, idxMv_2, QfIn[0].idxMag, QfIn[1].idxMag, forward_distance, backward_distance);
    MCTF_CHECK_CM_ERR(res, res);
    res = MCTF_RUN_BLEND2R();
    MCTF_CHECK_CM_ERR(res, res);
    res = MCTF_RUN_Denoise();
    MCTF_CHECK_CM_ERR(res, res);
    RotateBuffer();
    //    pTimer->calctime += CatchTime(2, 3, "MCTF calc:", PRINTTIMING);
    return res;
}

mfxI32 CMC::MCTF_RUN_AMCTF() {
    CMC::TimeStart(2);

    res = MCTF_RUN_Denoise(1);
    MCTF_CHECK_CM_ERR(res, res);
    //    pTimer->calctime += CatchTime(2, 3, "MCTF calc:", PRINTTIMING);
    return res;
}

mfxI32 CMC::MCTF_RUN_AMCTF(mfxU16 srcNum) {
    CMC::TimeStart(2);

    res = MCTF_RUN_Denoise(srcNum);
    MCTF_CHECK_CM_ERR(res, res);
    //    pTimer->calctime += CatchTime(2, 3, "MCTF calc:", PRINTTIMING);
    return res;
}

mfxI32 CMC::MCTF_RUN_Denoise(mfxU16 srcNum) {
    //    res = MCTF_SET_KERNELDe(srcNum, 0, 0);
    res = MCTF_SET_KERNELDe(srcNum, DIVUP(p_ctrl->CropX, 8), DIVUP(p_ctrl->CropY, 8));
    MCTF_CHECK_CM_ERR(res, res);
    //    tsHeight = DIVUP(p_ctrl->height, 8);
    //    tsWidthFull = DIVUP(p_ctrl->width, 8);
    tsHeight = DIVUP(p_ctrl->CropH, 8);
    tsWidthFull = DIVUP(p_ctrl->CropW, 8);
    tsWidth = tsWidthFull;

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW) {
        tsWidth = (tsWidthFull >> 1) & ~1;  // must be even for 32x32 blocks 
    }

    threadSpace = 0;
    res = MCTF_RUN_TASK(kernelMcDen, task != 0);
    MCTF_CHECK_CM_ERR(res, res);

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW) {
        mfxU16
            start_mbX = tsWidth;
        tsWidth = tsWidthFull - tsWidth;
        //        res = MCTF_SET_KERNELDe(srcNum, start_mbX, 0);
        res = MCTF_SET_KERNELDe(srcNum, start_mbX, DIVUP(p_ctrl->CropY, 8));
        MCTF_CHECK_CM_ERR(res, res);
        if (threadSpace != NULL) {
            res = device->DestroyThreadSpace(threadSpace);
            MCTF_CHECK_CM_ERR(res, res);
        }
        // the rest of frame TS
        res = MCTF_RUN_TASK(kernelMcDen, task != 0);
        MCTF_CHECK_CM_ERR(res, res);
    }
    res = e->WaitForTaskFinished();
    MCTF_CHECK_CM_ERR(res, res);
    UINT64 executionTime;
    e->GetExecutionTime(executionTime);
    exeTime += executionTime / 1000;
#if TIMINGINFO
    time += GetAccurateGpuTime(queue, task, threadSpace);
#ifdef MFX_MCTF_DEBUG_PRINT
    ASC_PRINTF("TIME=%.3fms ", time / 1000000.0);
#endif
#endif
    res = device->DestroyThreadSpace(threadSpace);
    MCTF_CHECK_CM_ERR(res, res);
    res = queue->DestroyEvent(e);
    MCTF_CHECK_CM_ERR(res, res);
    e = 0;
    return res;
}

mfxI32 CMC::MCTF_RUN_Denoise() {
    //    res = MCTF_SET_KERNELDe(0, 0);
    res = MCTF_SET_KERNELDe(DIVUP(p_ctrl->CropX, 8), DIVUP(p_ctrl->CropY, 8));
    MCTF_CHECK_CM_ERR(res, res);
    //    tsHeight = DIVUP(p_ctrl->height, 8);
    //    tsWidthFull = DIVUP(p_ctrl->width, 8);
    tsHeight = DIVUP(p_ctrl->CropH, 8);
    tsWidthFull = DIVUP(p_ctrl->CropW, 8);
    tsWidth = tsWidthFull;

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW) {
        tsWidth = (tsWidthFull >> 1) & ~1;  // must be even for 32x32 blocks 
    }

    threadSpace = 0;
    res = MCTF_RUN_TASK(kernelMcDen, task != 0);
    MCTF_CHECK_CM_ERR(res, res);

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW) {
        mfxU16
            start_mbX = tsWidth;
        tsWidth = tsWidthFull - tsWidth;
        //        res = MCTF_SET_KERNELDe(start_mbX, 0);
        res = MCTF_SET_KERNELDe(start_mbX, DIVUP(p_ctrl->CropY, 8));
        MCTF_CHECK_CM_ERR(res, res);
        res = MCTF_RUN_TASK(kernelMcDen, task != 0);
        MCTF_CHECK_CM_ERR(res, res);
    }
    res = e->WaitForTaskFinished();
    MCTF_CHECK_CM_ERR(res, res);
    UINT64 executionTime;
    e->GetExecutionTime(executionTime);
    exeTime += executionTime / 1000;
#if TIMINGINFO
    time += GetAccurateGpuTime(queue, task, threadSpace);
#ifdef MFX_MCTF_DEBUG_PRINT
    ASC_PRINTF("TIME=%.3fms ", time / 1000000.0);
#endif
#endif
    device->DestroyThreadSpace(threadSpace);
    queue->DestroyEvent(e);
    e = 0;
    return res;
}

mfxStatus CMC::MCTF_PUT_FRAME(IntMctfParams* pMctfControl, CmSurface2D* InSurf, CmSurface2D* OutSurf)
{
    lastFrame = 0;
    TimeStart(2);
    if (!InSurf)
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    INT cmSts = 0;

    SurfaceIndex* idxFrom;
    cmSts = InSurf->GetIndex(idxFrom);
    MFX_CHECK((CM_SUCCESS == cmSts), MFX_ERR_DEVICE_FAILED);
    
    MFX_SAFE_CALL(pSCD->PutFrameProgressive(idxFrom));
    sceneNum += pSCD->Get_frame_shot_Decision();

    MFX_SAFE_CALL(MCTF_UpdateRTParams(pMctfControl));
    MFX_SAFE_CALL(MCTF_PUT_FRAME(sceneNum, OutSurf));
    
    //    pTimer->calctime += CatchTime(2, 3, "SCD+MRE calc:", PRINTTIMING);
    forward_distance = -1;
    backward_distance = 1;
    countFrames++;
    return MFX_ERR_NONE;
}

mfxStatus CMC::MCTF_DO_FILTERING()
{
    // do filtering based on temporal mode & how many frames are
    // already in the queue:
    switch (number_of_References)
    {
    case 4://if (number_of_References == 4)
    {
        if (bufferCount < 3)
        {
            MctfState = AMCTF_NOT_READY;
            mco = nullptr;
            return MFX_ERR_NONE;
        }

        switch (firstFrame)
        {
        case 1:
        {
#if HALFWORK
            pMCTF_MERGE_func = NULL;
#else
            pMCTF_MERGE_func = &CMC::MCTF_RUN_BLEND2R;
#endif
            //todo: check is it correct that the current frame is 0?
            MCTF_UpdateANDApplyRTParams(0);
            pMCTF_ME_func = &CMC::MCTF_RUN_ME_2REF;
            pMCTF_LOAD_func = &CMC::MCTF_LOAD_2REF;
            RotateBufferA();
            res = MCTF_RUN_MCTF_DEN();
            CurrentIdx2Out = 0;
            MctfState = AMCTF_READY;
            firstFrame = 2;
            break;
        }
        case 2:
        {
            //todo: check is it correct that the current frame is 1?
            MCTF_UpdateANDApplyRTParams(1);
            res = MCTF_RUN_MCTF_DEN();
            RotateBufferA();
            CurrentIdx2Out = 1;
            MctfState = AMCTF_READY;
            firstFrame = 3;
#if HALFWORK
            pMCTF_MERGE_func = &CMC::MCTF_RUN_MERGE;
#else
            pMCTF_MERGE_func = &CMC::MCTF_BLEND4R;
#endif
            pMCTF_ME_func = &CMC::MCTF_RUN_ME_4REF;
            pMCTF_LOAD_func = &CMC::MCTF_LOAD_4REF;
            break;
        }

        default:
        {
            MCTF_UpdateANDApplyRTParams(2);
            res = (this->*(pMCTF_func))();
            CurrentIdx2Out = DefaultIdx2Out;
            MctfState = AMCTF_READY;
            break;
        }
        }
    };
    break;
    case 2://else if (number_of_References == 2)
    {
        if (bufferCount < 2) /*One frame delay*/
        {
            MctfState = AMCTF_NOT_READY;
            mco = nullptr;
            return MFX_ERR_NONE;
        }

        switch (firstFrame)
        {
            case 0:
            {
                MCTF_UpdateANDApplyRTParams(1);
                res = (this->*(pMCTF_func))();
                CurrentIdx2Out = DefaultIdx2Out;
                MctfState = AMCTF_READY;
                break;
            }
            default:
            {
                MCTF_UpdateANDApplyRTParams(0);
                res = MCTF_RUN_AMCTF(0);
                firstFrame = 0;
                CurrentIdx2Out = 0;
                MctfState = AMCTF_READY;
                break;
            }
        }
    };
    break;
    case 1: //else if (number_of_References == 1)
    {
        if (firstFrame)
        {
            MCTF_UpdateANDApplyRTParams(0);
            res = MCTF_RUN_AMCTF(0);
            firstFrame = 0;
            MctfState = AMCTF_READY;
        }
        else
        {
            MCTF_UpdateANDApplyRTParams(1);
            if (QfIn[0].scene_idx != QfIn[1].scene_idx)
                res = MCTF_RUN_AMCTF();
            else
                res = (this->*(pMCTF_func))();
            MctfState = AMCTF_READY;
        }
        CurrentIdx2Out = DefaultIdx2Out;
    };
    break;
    case 0://else if (number_of_References == 0)
    {
        MCTF_UpdateANDApplyRTParams(0);
        res = MCTF_RUN_AMCTF(0);
        firstFrame = 0;
        MctfState = AMCTF_READY;
        CurrentIdx2Out = DefaultIdx2Out;
    };
    break;
    default://    else
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    return MFX_ERR_NONE;
}

void CMC::MCTF_CLOSE() {
    if (kernelMe)
        device->DestroyKernel(kernelMe);
    if (kernelMeB)
        device->DestroyKernel(kernelMeB);
    if (kernelMcDen)
        device->DestroyKernel(kernelMcDen);
    if (kernelMc1r)
        device->DestroyKernel(kernelMc1r);
    if (kernelMc2r)
        device->DestroyKernel(kernelMc2r);
    if (kernelMc4r)
        device->DestroyKernel(kernelMc4r);
    if (programMe)
        device->DestroyProgram(programMe);
    if (programMc)
        device->DestroyProgram(programMc);
    if (programDe)
        device->DestroyProgram(programDe);
    if (ctrlBuf)
        device->DestroySurface(ctrlBuf);


    if (task)
        device->DestroyTask(task);
    if (e)
        queue->DestroyEvent(e);
    if (copyEv)
        queue->DestroyEvent(copyEv);

    if (mco2)
        device->DestroySurface(mco2);
    if (idxMv_1) {
        device->DestroySurface2DUP(mv_1);
        CM_ALIGNED_FREE(mvSys1);
    }
    if (idxMv_2) {
        device->DestroySurface2DUP(mv_2);
        CM_ALIGNED_FREE(mvSys2);
    }
    if (idxMv_3) {
        device->DestroySurface2DUP(mv_3);
        CM_ALIGNED_FREE(mvSys3);
    }
    if (idxMv_4) {
        device->DestroySurface2DUP(mv_4);
        CM_ALIGNED_FREE(mvSys4);
    }
    if (distSurf) {
        device->DestroySurface2DUP(distSurf);
        CM_ALIGNED_FREE(distSys);
    }
    if (noiseAnalysisSurf) {
        device->DestroySurface2DUP(noiseAnalysisSurf);
        CM_ALIGNED_FREE(noiseAnalysisSys);
    }
  
    if (pSCD) {
        pSCD->Close();
        pSCD = nullptr;
    }
}