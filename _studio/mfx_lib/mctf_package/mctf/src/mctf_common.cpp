// Copyright (c) 2020 Intel Corporation
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

#include "genx_me_gen8_isa.h"
#include "genx_mc_gen8_isa.h"
#include "genx_sd_gen8_isa.h"

#include "genx_me_gen9_isa.h"
#include "genx_mc_gen9_isa.h"
#include "genx_sd_gen9_isa.h"

#include "genx_me_gen11_isa.h"
#include "genx_mc_gen11_isa.h"
#include "genx_sd_gen11_isa.h"

#include "genx_me_gen11lp_isa.h"
#include "genx_mc_gen11lp_isa.h"
#include "genx_sd_gen11lp_isa.h"
#include "genx_me_gen12lp_isa.h"
#include "genx_mc_gen12lp_isa.h"
#include "genx_sd_gen12lp_isa.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <limits>
#include "cmrt_cross_platform.h"

using std::min;
using  std::max;
using namespace ns_asc;

const mfxU16 CMC::AUTO_FILTER_STRENGTH    = 0;
const mfxU16 CMC::DEFAULT_FILTER_STRENGTH = 8;
const mfxU16 CMC::INPIPE_FILTER_STRENGTH  = 5;
const mfxU32 CMC::DEFAULT_BPP             = 0; //Automode
const mfxU16 CMC::DEFAULT_DEBLOCKING      = MFX_CODINGOPTION_OFF;
const mfxU16 CMC::DEFAULT_OVERLAP         = MFX_CODINGOPTION_OFF;
const mfxU16 CMC::DEFAULT_ME              = MFX_MVPRECISION_INTEGER >> 1;
const mfxU16 CMC::DEFAULT_REFS            = MCTF_TEMPORAL_MODE_2REF;

void CMC::QueryDefaultParams(
    IntMctfParams * pBuffer
)
{
    if (!pBuffer) return;
    pBuffer->Deblocking        = DEFAULT_DEBLOCKING;
    pBuffer->Overlap           = DEFAULT_OVERLAP;
    pBuffer->subPelPrecision   = DEFAULT_ME;
    pBuffer->TemporalMode      = DEFAULT_REFS;
    pBuffer->FilterStrength    = DEFAULT_FILTER_STRENGTH; // [0...20]
    pBuffer->BitsPerPixelx100k = DEFAULT_BPP;
};

void CMC::QueryDefaultParams(
    mfxExtVppMctf * pBuffer
)
{
    if (!pBuffer) return;
    IntMctfParams
        Mctfparam;
    QueryDefaultParams(&Mctfparam);
    pBuffer->FilterStrength     = Mctfparam.FilterStrength;
#ifdef MFX_ENABLE_MCTF_EXT
    pBuffer->BitsPerPixelx100k  = Mctfparam.BitsPerPixelx100k;
    pBuffer->Overlap            = Mctfparam.Overlap;
    pBuffer->Deblocking         = Mctfparam.Deblocking;
    pBuffer->TemporalMode       = Mctfparam.TemporalMode;
    pBuffer->MVPrecision        = Mctfparam.subPelPrecision;
#endif
};

mfxStatus CMC::CheckAndFixParams(
    mfxExtVppMctf * pBuffer
)
{
    mfxStatus
        sts = MFX_ERR_NONE;
    if (!pBuffer) return MFX_ERR_NULL_PTR;
    if (pBuffer->FilterStrength > 20)
    {
        pBuffer->FilterStrength = AUTO_FILTER_STRENGTH;
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
void CMC::FillParamControl(
    IntMctfParams       * pBuffer,
    const mfxExtVppMctf * pSrc
)
{
    pBuffer->FilterStrength     = pSrc->FilterStrength;
#ifdef MFX_ENABLE_MCTF_EXT
    pBuffer->Deblocking         = pSrc->Deblocking;
    pBuffer->Overlap            = pSrc->Overlap;
    pBuffer->subPelPrecision    = pSrc->MVPrecision;
    pBuffer->TemporalMode       = pSrc->TemporalMode;
    pBuffer->BitsPerPixelx100k  = pSrc->BitsPerPixelx100k;
#endif
}

mfxStatus CMC::SetFilterStrenght(
    unsigned short tFs,//Temporal filter strength
    unsigned short sFs //Spatial filter strength
)
{
    if (tFs > 21 || sFs > 21)
        return MFX_ERR_INVALID_VIDEO_PARAM;

    p_ctrl->th  = tFs * 50;
    if (sFs)
        p_ctrl->sTh = (mfxU16)min(sFs + CHROMABASE, MAXCHROMA);
    else
        p_ctrl->sTh = MCTFNOFILTER;
    res         = ctrlBuf->WriteSurface((const uint8_t *)p_ctrl.get(), NULL, sizeof(MeControlSmall));
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    return MFX_ERR_NONE;
}

inline mfxStatus CMC::SetFilterStrenght(
    unsigned short fs
)
{
    MCTF_CHECK_CM_ERR(SetFilterStrenght(fs, fs), MFX_ERR_DEVICE_FAILED);
    return MFX_ERR_NONE;
}

inline mfxStatus CMC::SetupMeControl(
    const mfxFrameInfo & FrameInfo,
    mfxU16               th,
    mfxU16               subPelPre
)
{
    p_ctrl.reset(new MeControlSmall);
    const mfxU8 Diamond[MCTFSEARCHPATHSIZE] = {
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
    std::copy(std::begin(Diamond), std::end(Diamond), std::begin(p_ctrl->searchPath.sp));

    p_ctrl->searchPath.lenSp    = 16;
    p_ctrl->searchPath.maxNumSu = 57;

    p_ctrl->width  = FrameInfo.Width;
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

    if ((CropX + CropW > FrameInfo.Width) || (CropY + CropH > FrameInfo.Height))
        return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;

    p_ctrl->CropX = CropX;
    p_ctrl->CropY = CropY;
    p_ctrl->CropW = CropW;
    p_ctrl->CropH = CropH;

    if (th > 20)
        return MFX_ERR_INVALID_VIDEO_PARAM;
    p_ctrl->th  = th * 50;
    p_ctrl->sTh = (mfxU16)min(th + CHROMABASE, MAXCHROMA);
    p_ctrl->mre_width  = 0;
    p_ctrl->mre_height = 0;
    if (MFX_MVPRECISION_INTEGER >> 1 == subPelPre)
        p_ctrl->subPrecision = 0;
    else
        if (MFX_MVPRECISION_QUARTERPEL >> 1 == subPelPre)
            p_ctrl->subPrecision = 1;
        else
            return MFX_ERR_INVALID_VIDEO_PARAM;
    return MFX_ERR_NONE;
}

mfxStatus CMC::MCTF_GET_FRAME(
    CmSurface2D * outFrame
)
{
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
            res = MCTF_RUN_MCTF_DEN(true);
            lastFrame++;
        }
        else if (lastFrame == 2)
            MCTF_RUN_AMCTF(lastFrame);
    }
    else if (QfIn.size() == 3) 
    {
        if (lastFrame == 1)
            //res = MCTF_RUN_AMCTF(lastFrame);
            res = MCTF_RUN_MCTF_DEN(true);
    }

    MFX_CHECK(mco, MFX_ERR_UNDEFINED_BEHAVIOR);
    mco = nullptr;
    if (!lastFrame)
        lastFrame = 1;
    return sts;
}

mfxStatus CMC::MCTF_GET_FRAME(
    mfxU8 * outFrame
)
{
    mfxStatus sts = MFX_ERR_NONE;
    if (!outFrame)
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    if (!mco)
    {
        if (NO_REFERENCES == number_of_References || ONE_REFERENCE == number_of_References)
        {
            //this is undefined behavior
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        }
        else
            return MFX_ERR_DEVICE_FAILED;
    }

    if (lastFrame == 1)
        res = MCTF_RUN_MCTF_DEN(true);

    MFX_CHECK(mco, MFX_ERR_UNDEFINED_BEHAVIOR);

    if (QfIn[0].filterStrength > 0)
    {
        res = queue->EnqueueCopyGPUToCPU(mco, outFrame, copyEv);
        MFX_CHECK((CM_SUCCESS == res), MFX_ERR_DEVICE_FAILED);

        CM_STATUS
            status = CM_STATUS_FLUSHED;

        copyEv->GetStatus(status);
        while (status != CM_STATUS_FINISHED)
            copyEv->GetStatus(status);

        MctfState = AMCTF_NOT_READY;
    }

    return sts;
}

bool CMC::MCTF_CHECK_FILTER_USE()
{
    if (QfIn.front().filterStrength == 0)
        return false;
    return true;
}

mfxStatus CMC::MCTF_RELEASE_FRAME(
    bool isCmUsed
)
{
    if (MCTF_ReadyToOutput())
    {
        MctfState = AMCTF_NOT_READY;
        if (mco2)
        {
            mco = mco2;
            idxMco = idxMco2;
            mco2 = nullptr;
            idxMco2 = nullptr;
        }
    }
    //Buffer has rotated enough at this point that last position is old and needs to eb recycled.
    if (isCmUsed)
    {
        if (QfIn.back().frameData)
        {
            device->DestroySurface(QfIn.back().frameData);//This Surface is created by MCTF and needs to be destroyed after denoised frame has been used.
            QfIn.back().frameData = nullptr;
            QfIn.back().fIdx      = nullptr;
            QfIn.back().mfxFrame  = nullptr;
        }
    }
    return MFX_ERR_NONE;
}

mfxStatus CMC::MCTF_TrackTimeStamp(
    mfxFrameSurface1 * outFrame
)
{
    outFrame->Data.FrameOrder = QfIn[CurrentIdx2Out].mfxFrame->Data.FrameOrder;
    outFrame->Data.TimeStamp = QfIn[CurrentIdx2Out].mfxFrame->Data.TimeStamp;
    return MFX_ERR_NONE;
}

mfxStatus CMC::MCTF_InitQueue(
    mfxU16 refNum
)
{
    mfxU32  buffer_size(0);
    switch (refNum)
    {
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

    for (mfxU8 i = 0; i < buffer_size; i++)
    {
        scene_numbers[i] = 0;
        QfIn.push_back(gpuFrameData());
    }

    return MFX_ERR_NONE;
}

mfxStatus CMC::DIM_SET(
    mfxU16 overlap
)
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


mfxStatus CMC::IM_SURF_SET(
    CmSurface2D  ** p_surface,
    SurfaceIndex ** p_idxSurf
)
{
    res = device->CreateSurface2D(p_ctrl->width, p_ctrl->height, CM_SURFACE_FORMAT_NV12, *p_surface);
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = (*p_surface)->GetIndex(*p_idxSurf);
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    return MFX_ERR_NONE;
}

mfxStatus CMC::IM_SURF_SET(
    AbstractSurfaceHandle   pD3DSurf,
    CmSurface2D          ** p_surface,
    SurfaceIndex         ** p_idxSurf
)
{
    res = device->CreateSurface2D(pD3DSurf, *p_surface);
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = (*p_surface)->GetIndex(*p_idxSurf);
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    return MFX_ERR_NONE;
}

mfxStatus CMC::IM_MRE_SURF_SET(
    CmSurface2D  ** p_Surface,
    SurfaceIndex ** p_idxSurf
)
{
    mfxU32 
        width = SUBMREDIM, height = SUBMREDIM;
    res = device->CreateSurface2D(width * sizeof(mfxI16Pair), height, CM_SURFACE_FORMAT_A8, *p_Surface);
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = (*p_Surface)->GetIndex(*p_idxSurf);
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    return MFX_ERR_NONE;
}

mfxStatus CMC::IM_SURF_SET_Int() {
    for (mfxU32 i = 0; i < QfIn.size(); i++) {
        res += IM_SURF_SET(&QfIn[i].frameData, &QfIn[i].fIdx);
        QfIn[i].scene_idx = 0;
    }
    return MFX_ERR_NONE;
}

mfxStatus CMC::IM_SURF_SET()
{
    for (mfxU32 i = 0; i < QfIn.size(); i++)
    {
        MFX_SAFE_CALL(IM_MRE_SURF_SET(&QfIn[i].magData, &QfIn[i].idxMag));
        mfxHDLPair
            handle;
        // if a surface is opaque, need to extract normal surface
        mfxFrameSurface1 
            * pSurf = m_pCore->GetNativeSurface(QfIn[i].mfxFrame);
        QfIn[i].mfxFrame = pSurf ? pSurf : QfIn[i].mfxFrame;
        // GetFrameHDL is used as QfIn[].mfxFrme is allocated via call to Core Alloc function
        MFX_SAFE_CALL(m_pCore->GetFrameHDL(QfIn[i].mfxFrame->Data.MemId, reinterpret_cast<mfxHDL*>(&handle)));
        MFX_SAFE_CALL(IM_SURF_SET(reinterpret_cast<AbstractSurfaceHandle>(handle.first), &QfIn[i].frameData, &QfIn[i].fIdx));
        QfIn[i].scene_idx = 0;
    }
    return MFX_ERR_NONE;
}

mfxU16  CMC::MCTF_QUERY_NUMBER_OF_REFERENCES()
{
    return number_of_References;
}

mfxU32 CMC::MCTF_GetQueueDepth()
{
    return (mfxU32)QfIn.size();
}

mfxStatus CMC::MCTF_SetMemory(
    const std::vector<mfxFrameSurface1*> & mfxSurfPool
)
{
    if (mfxSurfPool.size() != QfIn.size())
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    else
    {
        auto inp_iter = mfxSurfPool.begin();
        for (auto it = QfIn.begin(); it != QfIn.end() && inp_iter != mfxSurfPool.end(); ++it, ++inp_iter)
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
        if (number_of_References > 2)
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

mfxStatus CMC::MCTF_SetMemory(
    bool isCmUsed
)
{
        // mco & idxmco will be extracted from input/output surface
        if (!isCmUsed)
        {
            res = IM_SURF_SET_Int();
            MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
        }
        mco     = nullptr;
        idxMco  = nullptr;
        mco2    = nullptr;
        idxMco2 = nullptr;

        //Setup for 2 references
        res = GEN_SURF_SET(&mv_1, &mvSys1, &idxMv_1);
        MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
        res = GEN_SURF_SET(&mv_2, &mvSys2, &idxMv_2);
        MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
        if (number_of_References > 2)
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

mfxStatus CMC::MCTF_INIT(
    VideoCORE           * core,
    CmDevice            * pCmDevice,
    const mfxFrameInfo  & FrameInfo,
    const IntMctfParams * pMctfParam,
    bool                  isCmUsed,
    const bool            externalSCD,
    const bool            useFilterAdaptControl,
    const bool            isNCActive
)
{
    version         = 400;
    argIdx          = 0;
    tsWidthFull     = 0;
    tsWidth         = 0;
    tsHeight        = 0;
    bth             = 500;
    blsize          = 16;
    device          = NULL;
    sceneNum        = 0;
    countFrames     = 0;
    firstFrame      = 1;
    lastFrame       = 0;
    exeTime         = 0;
    exeTimeT        = 0;
    m_externalSCD   = externalSCD;
    m_adaptControl  = useFilterAdaptControl;
    m_doFilterFrame = false;

    //--filter configuration parameters
    m_AutoMode = MCTF_MODE::MCTF_NOT_INITIALIZED_MODE;
    ConfigMode = MCTF_CONFIGURATION::MCTF_NOT_CONFIGURED;
    number_of_References = MCTF_TEMPORAL_MODE_2REF;
    bitrate_Adaptation = false;
    deblocking_Control = MFX_CODINGOPTION_OFF;
    bpp = 0.0;
    m_FrameRateExtD = 1;
    m_FrameRateExtN = 0;

    ctrlBuf = 0;
    idxCtrl = 0;
    distSys = 0;
    time    = 0;

    qpel1   = 0;
    qpel2   = 0;
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
    kernelMe  = 0;

    //MC elements
    mco     = 0;
    idxMco  = 0;
    mco2    = 0;
    idxMco2 = 0;

    //Motion Compensation
    programMc   = 0;
    kernelMcDen = 0;
    kernelMc1r  = 0;
    kernelMc2r  = 0;
    kernelMc4r  = 0;
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

    mfxStatus sts = MFX_ERR_NONE;
    if(!m_externalSCD)
        pSCD.reset(new(ASC));

    IntMctfParams MctfParam{};
    QueryDefaultParams(&MctfParam);

    // if no MctfParams are passed, to use default
    if (!pMctfParam)
        pMctfParam = &MctfParam;
    sts = MCTF_SET_ENV(core, FrameInfo, pMctfParam, isCmUsed, isNCActive);

    MFX_CHECK_STS(sts);
    return sts;
}

mfxStatus CMC::MCTF_INIT(
    VideoCORE           * core,
    CmDevice            * pCmDevice,
    const mfxFrameInfo  & FrameInfo,
    const IntMctfParams * pMctfParam
)
{
    return (MCTF_INIT(core, pCmDevice, FrameInfo, pMctfParam, false, false));
}

mfxStatus CMC::MCTF_INIT(
    VideoCORE           * core,
    CmDevice            * pCmDevice,
    const mfxFrameInfo  & FrameInfo,
    const IntMctfParams * pMctfParam,
    const bool            externalSCD,
    const bool            isNCActive
)
{
    return (MCTF_INIT(core, pCmDevice, FrameInfo, pMctfParam, false, externalSCD, false, isNCActive));
}

mfxStatus CMC::MCTF_SET_ENV(
    VideoCORE           * core,
    const mfxFrameInfo  & FrameInfo,
    const IntMctfParams * pMctfParam,
    bool                  isCmUsed,
    bool                  isNCActive
)
{
    IntMctfParams localMctfParam = *pMctfParam;
    mfxStatus sts = MFX_ERR_NONE;
    if (!device)
        return MFX_ERR_NOT_INITIALIZED;

    hwSize = 4;
    res = device->GetCaps(CAP_GPU_PLATFORM, hwSize, &hwType);
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);

    if (core->GetHWType() >= MFX_HW_ICL)
        res = device->CreateQueueEx(queue, CM_VME_QUEUE_CREATE_OPTION);
    else
        res = device->CreateQueue(queue);
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
        MFX_CODINGOPTION_UNKNOWN != localMctfParam.Deblocking)
        return MFX_ERR_INVALID_VIDEO_PARAM;

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

    pMCTF_SpDen_func = nullptr;
    pMCTF_NOA_func   = nullptr;
    pMCTF_LOAD_func  = nullptr;
    pMCTF_ME_func    = nullptr;
    pMCTF_MERGE_func = nullptr;
    pMCTF_func       = nullptr;

    pMCTF_func       = &CMC::MCTF_RUN_MCTF_DEN;
    if (m_AutoMode == MCTF_MODE::MCTF_AUTO_MODE || isNCActive)
        pMCTF_NOA_func = &CMC::noise_estimator;

    if (number_of_References == FOUR_REFERENCES)
    {
        pMCTF_LOAD_func = &CMC::MCTF_LOAD_4REF;
        pMCTF_ME_func = &CMC::MCTF_RUN_ME_4REF;
        pMCTF_MERGE_func = &CMC::MCTF_RUN_MERGE;
    }
    else if (number_of_References == TWO_REFERENCES)
    {
        pMCTF_LOAD_func = &CMC::MCTF_LOAD_2REF;
        if (isNCActive)
            pMCTF_ME_func = &CMC::MCTF_RUN_ME_2REF_HE;
        else
            pMCTF_ME_func = &CMC::MCTF_RUN_ME_2REF;
        pMCTF_MERGE_func = NULL;
    }
    else if (number_of_References == ONE_REFERENCE)
    {
        pMCTF_func = &CMC::MCTF_RUN_MCTF_DEN_1REF;
        pMCTF_LOAD_func = &CMC::MCTF_LOAD_1REF;
        pMCTF_ME_func = &CMC::MCTF_RUN_ME_1REF;
        pMCTF_MERGE_func = &CMC::MCTF_RUN_BLEND;
    }
    else if (number_of_References == NO_REFERENCES)
    {
        pMCTF_func = &CMC::MCTF_RUN_MCTF_DEN_1REF;
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

    //Init internal buffer which is not shared
    if (isNCActive)
    {
        res = MCTF_SetMemory(isCmUsed);
        MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    }

    //Motion Estimation
    switch (hwType)
    {
#ifdef MFX_ENABLE_KERNELS
    case PLATFORM_INTEL_BDW:
        res = device->LoadProgram((void *)genx_me_gen8, sizeof(genx_me_gen8), programMe, "nojitter");
        break;
    case PLATFORM_INTEL_ICL:
        res = device->LoadProgram((void *)genx_me_gen11, sizeof(genx_me_gen11), programMe, "nojitter");
        break;
    case PLATFORM_INTEL_ICLLP:
        res = device->LoadProgram((void *)genx_me_gen11lp, sizeof(genx_me_gen11lp), programMe, "nojitter");
        break;
    case PLATFORM_INTEL_TGLLP:
    case PLATFORM_INTEL_RKL:
    case PLATFORM_INTEL_DG1:
        res = device->LoadProgram((void *)genx_me_gen12lp, sizeof(genx_me_gen12lp), programMe, "nojitter");
        break;
    case PLATFORM_INTEL_SKL:
    case PLATFORM_INTEL_BXT:
    case PLATFORM_INTEL_CNL:
    case PLATFORM_INTEL_KBL:
    case PLATFORM_INTEL_CFL:
    case PLATFORM_INTEL_GLK:
        res = device->LoadProgram((void *)genx_me_gen9, sizeof(genx_me_gen9), programMe, "nojitter");
        break;
#endif
    default:
        return MFX_ERR_UNSUPPORTED;
    }
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);

    //ME Kernel
    if (MFX_CODINGOPTION_ON == overlap_Motion) 
    {
        res = device->CreateKernel(programMe, CM_KERNEL_FUNCTION(MeP16_1MV_MRE), kernelMe);
        res = device->CreateKernel(programMe, CM_KERNEL_FUNCTION(MeP16bi_1MV2_MRE), kernelMeB);
        res = device->CreateKernel(programMe, CM_KERNEL_FUNCTION(MeP16bi_1MV2_MRE), kernelMeB2);

    }
    else
    if (MFX_CODINGOPTION_OFF == overlap_Motion || MFX_CODINGOPTION_UNKNOWN == overlap_Motion)
    {
        res = device->CreateKernel(programMe, CM_KERNEL_FUNCTION(MeP16_1MV_MRE_8x8), kernelMe);
        if (isNCActive)
        {
            res = device->CreateKernel(programMe, CM_KERNEL_FUNCTION(MeP16_1ME_2BiRef_MRE_8x8), kernelMeB);
            res = device->CreateKernel(programMe, CM_KERNEL_FUNCTION(MeP16_1ME_2BiRef_MRE_8x8), kernelMeB2);
        }
        else
        {
            res = device->CreateKernel(programMe, CM_KERNEL_FUNCTION(MeP16bi_1MV2_MRE_8x8), kernelMeB);
            res = device->CreateKernel(programMe, CM_KERNEL_FUNCTION(MeP16bi_1MV2_MRE_8x8), kernelMeB2);
        }

    }
    else
        return MFX_ERR_INVALID_VIDEO_PARAM;
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);

    //Motion Compensation
    switch (hwType)
    {
#ifdef MFX_ENABLE_KERNELS
    case PLATFORM_INTEL_BDW:
        res = device->LoadProgram((void *)genx_mc_gen8, sizeof(genx_mc_gen8), programMc, "nojitter");
        break;
    case PLATFORM_INTEL_ICL:
        res = device->LoadProgram((void *)genx_mc_gen11, sizeof(genx_mc_gen11), programMc, "nojitter");
        break;
    case PLATFORM_INTEL_ICLLP:
        res = device->LoadProgram((void *)genx_mc_gen11lp, sizeof(genx_mc_gen11lp), programMc, "nojitter");
        break;
    case PLATFORM_INTEL_TGLLP:
    case PLATFORM_INTEL_RKL:
    case PLATFORM_INTEL_DG1:
        res = device->LoadProgram((void *)genx_mc_gen12lp, sizeof(genx_mc_gen12lp), programMc, "nojitter");
        break;
    case PLATFORM_INTEL_SKL:
    case PLATFORM_INTEL_BXT:
    case PLATFORM_INTEL_CNL:
    case PLATFORM_INTEL_KBL:
    case PLATFORM_INTEL_CFL:
    case PLATFORM_INTEL_GLK:
        res = device->LoadProgram((void *)genx_mc_gen9, sizeof(genx_mc_gen9), programMc, "nojitter");
        break;
#endif
    default:
        return MFX_ERR_UNSUPPORTED;
    }
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);

    switch (hwType)
    {
#ifdef MFX_ENABLE_KERNELS
    case PLATFORM_INTEL_BDW:
        res = device->LoadProgram((void *)genx_sd_gen8, sizeof(genx_sd_gen8), programDe, "nojitter");
        break;
    case PLATFORM_INTEL_ICL:
        res = device->LoadProgram((void *)genx_sd_gen11, sizeof(genx_sd_gen11), programDe, "nojitter");
        break;
    case PLATFORM_INTEL_ICLLP:
        res = device->LoadProgram((void *)genx_sd_gen11lp, sizeof(genx_sd_gen11lp), programDe, "nojitter");
        break;
    case PLATFORM_INTEL_TGLLP:
    case PLATFORM_INTEL_RKL:
    case PLATFORM_INTEL_DG1:
        res = device->LoadProgram((void *)genx_sd_gen12lp, sizeof(genx_sd_gen12lp), programDe, "nojitter");
        break;
    case PLATFORM_INTEL_SKL:
    case PLATFORM_INTEL_BXT:
    case PLATFORM_INTEL_CNL:
    case PLATFORM_INTEL_KBL:
    case PLATFORM_INTEL_CFL:
    case PLATFORM_INTEL_GLK:
        res = device->LoadProgram((void *)genx_sd_gen9, sizeof(genx_sd_gen9), programDe, "nojitter");
        break;
#endif
    default:
        return MFX_ERR_UNSUPPORTED;
    }
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
    if (!m_externalSCD)
    {
        sts = pSCD->Init(p_ctrl->CropW, p_ctrl->CropH, p_ctrl->width, MFX_PICSTRUCT_PROGRESSIVE, device);
        MFX_CHECK_STS(sts);
        sts = pSCD->SetGoPSize(Immediate_GoP);
        MFX_CHECK_STS(sts);
        pSCD->SetControlLevel(0);
    }
    if (m_AutoMode == MCTF_MODE::MCTF_AUTO_MODE) 
    {
        sts = SetFilterStrenght(DEFAULT_FILTER_STRENGTH);
        m_RTParams.FilterStrength = DEFAULT_FILTER_STRENGTH;
    }
    else
    {
        if (isNCActive)
        {
            sts = SetFilterStrenght(INPIPE_FILTER_STRENGTH);
            m_RTParams.FilterStrength = INPIPE_FILTER_STRENGTH;
        }
        else
        {
            sts = SetFilterStrenght(localMctfParam.FilterStrength);
            m_RTParams.FilterStrength = localMctfParam.FilterStrength;
        }
    }
    // now initialize run-time parameters
    m_RTParams = localMctfParam;
    m_InitRTParams = m_RTParams;

    return sts;
}
mfxStatus CMC::MCTF_CheckRTParams(
    const IntMctfParams * pMctfControl
)
{
    mfxStatus sts = MFX_ERR_NONE;
    if (pMctfControl)
    {
#ifdef MFX_ENABLE_MCTF_EXT
    // check BPP for max value. the threshold must be adjusted for higher bit-depths
        if (pMctfControl->BitsPerPixelx100k > (DEFAULT_BPP))
            sts = MFX_ERR_INVALID_VIDEO_PARAM;
#endif
        if (pMctfControl->FilterStrength > 21)
            sts = MFX_ERR_INVALID_VIDEO_PARAM;
    }
    return sts;
}
mfxStatus CMC::MCTF_UpdateRTParams(
    IntMctfParams * pMctfControl
)
{
    mfxStatus  sts = MCTF_CheckRTParams(pMctfControl);

    if (pMctfControl && MFX_ERR_NONE == sts)
        m_RTParams = *pMctfControl;
    else
        m_RTParams = m_InitRTParams;
    return MFX_ERR_NONE;
}
mfxStatus CMC::MCTF_UpdateANDApplyRTParams(
    mfxU8 srcNum
)
{
    // deblock can be controled for every mode
    if (MCTF_CONFIGURATION::MCTF_MAN_NCA_NBA == ConfigMode ||
        MCTF_CONFIGURATION::MCTF_AUT_CA_BA == ConfigMode ||
        MCTF_CONFIGURATION::MCTF_AUT_CA_NBA == ConfigMode)
    {

#ifdef MCTF_UPDATE_RT_FRAME_ORDER_BASED
        m_RTParams = QfIn[srcNum].mfxMctfControl;
#else
        (void)srcNum;
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
mfxStatus CMC::MCTF_UpdateBitrateInfo(
    mfxU32 BitsPerPexelx100k
)
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

mfxStatus CMC::GEN_NoiseSURF_SET(
    CmSurface2DUP ** p_Surface,
    void          ** p_Sys,
    SurfaceIndex  ** p_idxSurf
)
{
    surfNoisePitch = 0;
    surfNoiseSize = 0;
    res = device->GetSurface2DInfo(DIVUP(p_ctrl->CropW, 16) * sizeof(spatialNoiseAnalysis), DIVUP(p_ctrl->CropH, 16), CM_SURFACE_FORMAT_A8, surfNoisePitch, surfNoiseSize);
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);

    *p_Sys = CM_ALIGNED_MALLOC(surfNoiseSize, 0x1000);
    MFX_CHECK(*p_Sys, MFX_ERR_NULL_PTR);
    memset(*p_Sys, 0, surfNoiseSize);
    res = device->CreateSurface2DUP(DIVUP(p_ctrl->CropW, 16) * sizeof(spatialNoiseAnalysis), DIVUP(p_ctrl->CropH, 16), CM_SURFACE_FORMAT_A8, *p_Sys, *p_Surface);
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = (*p_Surface)->GetIndex(*p_idxSurf);
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    return MFX_ERR_NONE;
}
mfxStatus CMC::GEN_SURF_SET(
    CmSurface2DUP ** p_Surface,
    void          ** p_Sys,
    SurfaceIndex  **p_idxSurf
)
{
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

mfxStatus CMC::MCTF_GetEmptySurface(
    mfxFrameSurface1 ** ppSurface
)
{
    size_t buffer_size = QfIn.size() - 1;
    if (bufferCount > buffer_size)
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    if (QfIn[bufferCount].mfxFrame->Data.Locked)
    {
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

mfxStatus CMC::MCTF_PUT_FRAME(
    mfxU32 sceneNumber,
    CmSurface2D* OutSurf
)
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

mfxStatus CMC::MCTF_UpdateBufferCount()
{
    size_t buffer_size = QfIn.size() - 1;
    if (bufferCount > buffer_size)
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    bufferCount = (bufferCount < buffer_size) ? bufferCount + 1 : buffer_size;
    return MFX_ERR_NONE;
}

mfxI32 CMC::MCTF_LOAD_1REF()
{
    res = device->CreateVmeSurfaceG7_5(QfIn[1].frameData, &QfIn[0].frameData, NULL, 1, 0, genxRefs1);
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_LOAD_2REF()
{
    res = device->CreateVmeSurfaceG7_5(QfIn[1].frameData, &QfIn[0].frameData, &QfIn[2].frameData, 1, 1, genxRefs1);
    MCTF_CHECK_CM_ERR(res, res);
    res = device->CreateVmeSurfaceG7_5(QfIn[1].frameData, &QfIn[2].frameData, NULL, 1, 0, genxRefs2);
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_LOAD_4REF()
{
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

mfxI32 CMC::MCTF_SET_KERNELMe(
    SurfaceIndex * GenxRefs,
    SurfaceIndex * idxMV,
    mfxU16         start_x,
    mfxU16         start_y,
    mfxU8          blSize)
{
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

mfxI32 CMC::MCTF_SET_KERNELMeBi(
    SurfaceIndex * GenxRefs,
    SurfaceIndex * GenxRefs2,
    SurfaceIndex * idxMV,
    SurfaceIndex * idxMV2,
    mfxU16         start_x,
    mfxU16         start_y,
    mfxU8          blSize,
    mfxI8          forwardRefDist,
    mfxI8          backwardRefDist
)
{
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

mfxU32 CMC::MCTF_SET_KERNELMeBiMRE(
    SurfaceIndex* GenxRefs, SurfaceIndex* GenxRefs2,
    SurfaceIndex* idxMV, SurfaceIndex* idxMV2,
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

mfxU32 CMC::MCTF_SET_KERNELMeBiMRE2(
    SurfaceIndex* GenxRefs, SurfaceIndex* GenxRefs2,
    SurfaceIndex* idxMV, SurfaceIndex* idxMV2,
    mfxU16 start_x, mfxU16 start_y, mfxU8 blSize,
    mfxI8 forwardRefDist, mfxI8 backwardRefDist) {
    argIdx = 0;
    res = kernelMeB2->SetKernelArg(argIdx++, sizeof(*idxCtrl), idxCtrl);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB2->SetKernelArg(argIdx++, sizeof(*GenxRefs), GenxRefs);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB2->SetKernelArg(argIdx++, sizeof(*GenxRefs2), GenxRefs2);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB2->SetKernelArg(argIdx++, sizeof(*idxDist), idxDist);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB2->SetKernelArg(argIdx++, sizeof(*idxMV), idxMV);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB2->SetKernelArg(argIdx++, sizeof(*idxMV2), idxMV2);
    MCTF_CHECK_CM_ERR(res, res);
    // set start mb
    mfxU16Pair start_xy = { start_x, start_y };
    res = kernelMeB2->SetKernelArg(argIdx++, sizeof(start_xy), &start_xy);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB2->SetKernelArg(argIdx++, sizeof(blSize), &blSize);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB2->SetKernelArg(argIdx++, sizeof(forwardRefDist), &forwardRefDist);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB2->SetKernelArg(argIdx++, sizeof(backwardRefDist), &backwardRefDist);
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_SET_KERNELMc(
    mfxU16 start_x,
    mfxU16 start_y,
    mfxU8 srcNum,
    mfxU8 refNum
)
{
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

mfxI32 CMC::MCTF_SET_KERNELMc2r(
    mfxU16 start_x,
    mfxU16 start_y
)
{
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

mfxI32 CMC::MCTF_SET_KERNELMc2rDen(
    mfxU16 start_x,
    mfxU16 start_y
)
{
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

mfxI32 CMC::MCTF_SET_KERNELMc4r(
    mfxU16 start_x,
    mfxU16 start_y
)
{
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

mfxI32 CMC::MCTF_SET_KERNELMc4r(
    mfxU16         start_x,
    mfxU16         start_y,
    SurfaceIndex * multiIndex
)
{
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

mfxI32 CMC::MCTF_SET_KERNELMcMerge(
    mfxU16 start_x,
    mfxU16 start_y
)
{
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

mfxI32 CMC::MCTF_SET_KERNELMc4r(
    mfxU16 start_x,
    mfxU16 start_y,
    mfxU8  runType
)
{
    argIdx = 0;
    mfxU8
        currentFrame = 2,
        pastRef = 1,
        futureRef = 3;
    SurfaceIndex
        **mcOut = &idxMco,
        **pastMv = &idxMv_1,
        **futureMv = &idxMv_2;

    if (runType == DEN_FAR_RUN)
    {
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

mfxI32 CMC::MCTF_SET_KERNEL_Noise(
    mfxU16 srcNum,
    mfxU16 start_x,
    mfxU16 start_y
)
{
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

mfxI32 CMC::MCTF_SET_KERNELDe(
    mfxU16 srcNum,
    mfxU16 start_x,
    mfxU16 start_y
)
{
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

mfxI32 CMC::MCTF_SET_KERNELDe(
    mfxU16 start_x,
    mfxU16 start_y
)
{
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

mfxI32 CMC::MCTF_Enqueue(
    CmTask* taskInt,
    CmEvent* & eInt,
    const CmThreadSpace* tSInt
)
{
    mfxI32
        ret = CM_FAILURE;
        switch (mctf_HWType){
        case MFX_HW_ICL:
            ret = queue->EnqueueFast(taskInt, eInt, tSInt);
            break;
        default:
            ret = queue->Enqueue(taskInt, eInt, tSInt);
        }
    return ret;
}

mfxI32 CMC::MCTF_RUN_TASK_NA(
    CmKernel * kernel,
    bool       reset,
    mfxU16     widthTs,
    mfxU16     heightTs
)
{
    res = kernel->SetThreadCount(widthTs * heightTs);
    MCTF_CHECK_CM_ERR(res, res);
    res = device->CreateThreadSpace(widthTs, heightTs, threadSpace);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernel->AssociateThreadSpace(threadSpace);
    MCTF_CHECK_CM_ERR(res, res);

    if (reset)
    {
        res = task->Reset();
        MCTF_CHECK_CM_ERR(res, res);
    }
    else
    {
        res = device->CreateTask(task);
        MCTF_CHECK_CM_ERR(res, res);
    }
    res = task->AddKernel(kernel);
    MCTF_CHECK_CM_ERR(res, res);
    res = MCTF_Enqueue(task, e);
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_RUN_TASK(
    CmKernel * kernel,
    bool       reset
)
{
    res = kernel->SetThreadCount(tsWidth * tsHeight);
    MCTF_CHECK_CM_ERR(res, res);
    res = device->CreateThreadSpace(tsWidth, tsHeight, threadSpace);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernel->AssociateThreadSpace(threadSpace);
    MCTF_CHECK_CM_ERR(res, res);

    if (reset)
    {
        res = task->Reset();
        MCTF_CHECK_CM_ERR(res, res);
    }
    else
    {
        res = device->CreateTask(task);
        MCTF_CHECK_CM_ERR(res, res);
    }
    res = task->AddKernel(kernel);
    MCTF_CHECK_CM_ERR(res, res);
    res = MCTF_Enqueue(task, e);
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_RUN_DOUBLE_TASK(
    CmKernel * meKernel,
    CmKernel * mcKernel,
    bool       reset
)
{
    res = meKernel->SetThreadCount(tsWidth * tsHeight);
    MCTF_CHECK_CM_ERR(res, res);
    res = device->CreateThreadSpace(tsWidth, tsHeight, threadSpace2);
    MCTF_CHECK_CM_ERR(res, res);
    res = meKernel->AssociateThreadSpace(threadSpace2);
    MCTF_CHECK_CM_ERR(res, res);

    res = mcKernel->SetThreadCount(tsWidthMC * tsHeightMC);
    MCTF_CHECK_CM_ERR(res, res);
    res = device->CreateThreadSpace(tsWidthMC, tsHeightMC, threadSpaceMC);
    MCTF_CHECK_CM_ERR(res, res);
    res = mcKernel->AssociateThreadSpace(threadSpaceMC);
    MCTF_CHECK_CM_ERR(res, res);

    if (reset)
    {
        res = task->Reset();
        MCTF_CHECK_CM_ERR(res, res);
    }
    else
    {
        res = device->CreateTask(task);
        MCTF_CHECK_CM_ERR(res, res);
    }
    res = task->AddKernel(meKernel);
    MCTF_CHECK_CM_ERR(res, res);
    res = task->AddSync();
    MCTF_CHECK_CM_ERR(res, res);
    res = task->AddKernel(mcKernel);
    MCTF_CHECK_CM_ERR(res, res);
    res = MCTF_Enqueue(task, e);
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_RUN_MCTASK(
    CmKernel * kernel,
    bool       reset
)
{
    res = kernel->SetThreadCount(tsWidthMC * tsHeightMC);
    MCTF_CHECK_CM_ERR(res, res);
    res = device->CreateThreadSpace(tsWidthMC, tsHeightMC, threadSpaceMC2);
    MCTF_CHECK_CM_ERR(res, res);

    if (reset)
    {
        res = task->Reset();
        MCTF_CHECK_CM_ERR(res, res);
    }
    else
    {
        res = device->CreateTask(task);
        MCTF_CHECK_CM_ERR(res, res);
    }
    res = task->AddKernel(kernel);
    MCTF_CHECK_CM_ERR(res, res);
    res = MCTF_Enqueue(task, e, threadSpaceMC2);
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_RUN_TASK(
    CmKernel      * kernel,
    bool            reset,
    CmThreadSpace * tS
)
{
    res = kernel->SetThreadCount(tsWidth * tsHeight);
    MCTF_CHECK_CM_ERR(res, res);
    res = device->CreateThreadSpace(tsWidth, tsHeight, tS);
    MCTF_CHECK_CM_ERR(res, res);
    if (reset)
    {
        res = task->Reset();
        MCTF_CHECK_CM_ERR(res, res);
    }
    else
    {
        res = device->CreateTask(task);
        MCTF_CHECK_CM_ERR(res, res);
    }
    res = task->AddKernel(kernel);
    MCTF_CHECK_CM_ERR(res, res);
    res = MCTF_Enqueue(task, e, tS);
    MCTF_CHECK_CM_ERR(res, res);
    res = device->DestroyThreadSpace(tS);
    return res;
}

mfxU8 CMC::SetOverlapOp()
{
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

mfxU8 CMC::SetOverlapOp_half()
{
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
    SurfaceIndex * GenxRefs,
    SurfaceIndex * idxMV
)
{
    time = 0;

    mfxU8
        blSize = SetOverlapOp();
    res = MCTF_SET_KERNELMe(GenxRefs, idxMV, DIVUP(p_ctrl->CropX, blSize) , DIVUP(p_ctrl->CropY, blSize), blSize);
    MCTF_CHECK_CM_ERR(res, res);
    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW)
        tsWidth = (tsWidthFull >> 1) & ~1;  // must be even for 32x32 blocks 

    threadSpace = 0;
    res = MCTF_RUN_TASK(kernelMe, task != 0);
    MCTF_CHECK_CM_ERR(res, res);

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW)
    {
        mfxU16 start_mbX = tsWidth;
        tsWidth = tsWidthFull - tsWidth;
        res = MCTF_SET_KERNELMe(GenxRefs, idxMV, start_mbX, DIVUP(p_ctrl->CropY, blSize), blSize);
        MCTF_CHECK_CM_ERR(res, res);
        // the rest of frame TS
        res = MCTF_RUN_TASK(kernelMe, task != 0);
        MCTF_CHECK_CM_ERR(res, res);
    }
    return res;
}

mfxI32 CMC::MCTF_RUN_ME(
    SurfaceIndex * GenxRefs,
    SurfaceIndex * GenxRefs2,
    SurfaceIndex * idxMV,
    SurfaceIndex * idxMV2,
    char           forwardRefDist,
    char           backwardRefDist
)
{
    mfxU8
        blSize = SetOverlapOp();
    res = MCTF_SET_KERNELMeBi(GenxRefs, GenxRefs2, idxMV, idxMV2, 0, 0, blSize, forwardRefDist, backwardRefDist);
    MCTF_CHECK_CM_ERR(res, res);
    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW)
        tsWidth = (tsWidthFull >> 1) & ~1;  // must be even for 32x32 blocks 

    threadSpace = 0;
    res = MCTF_RUN_TASK(kernelMeB, task != 0);
    MCTF_CHECK_CM_ERR(res, res);

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW)
    {
        mfxU16
            start_mbX = tsWidth;
        tsWidth = tsWidthFull - tsWidth;
        res = MCTF_SET_KERNELMeBi(GenxRefs, GenxRefs2, idxMV, idxMV2, start_mbX, 0, blSize, forwardRefDist, backwardRefDist);

        MCTF_CHECK_CM_ERR(res, res);
        // the rest of frame TS
        res = MCTF_RUN_TASK(kernelMeB, task != 0);
        MCTF_CHECK_CM_ERR(res, res);
    }
    return res;
}

mfxI32 CMC::MCTF_RUN_ME_MC_HE(
    SurfaceIndex* GenxRefs,
    SurfaceIndex* GenxRefs2,
    SurfaceIndex* idxMV,
    SurfaceIndex* idxMV2,
    mfxI8         forwardRefDist,
    mfxI8         backwardRefDist,
    mfxU8         mcSufIndex
)
{
    p_ctrl->sTh = MCTF_CHROMAOFF;
    p_ctrl->th = QfIn[1].filterStrength * 50;
    res = ctrlBuf->WriteSurface((const mfxU8 *)p_ctrl.get(), NULL, sizeof(MeControlSmall));
    MCTF_CHECK_CM_ERR(res, res);
    time = 0;

    mfxU8
        blSize = SetOverlapOp_half();
    mfxU16
        multiplier = 2;
    UINT64
        executionTime = 0;
    tsHeightMC = (DIVUP(p_ctrl->height, blsize) * multiplier);
    tsWidthFullMC = (DIVUP(p_ctrl->width, blsize) * multiplier);
    tsWidthMC = tsWidthFullMC;

    res = MCTF_SET_KERNELMeBiMRE(GenxRefs, GenxRefs2, idxMV, idxMV2, 0, 0, blSize, forwardRefDist, backwardRefDist);
    MCTF_CHECK_CM_ERR(res, res);
    res = MCTF_SET_KERNELMeBiMRE2(GenxRefs, GenxRefs2, idxMV, idxMV2, 0, tsHeight, blSize, forwardRefDist, backwardRefDist);
    MCTF_CHECK_CM_ERR(res, res);

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MO)
    {
        tsWidth = (tsWidthFull >> 1) & ~1;  // must be even for 32x32 blocks 
    }

    if (tsWidthFullMC > CM_MAX_THREADSPACE_WIDTH_FOR_MO)
    {
        tsWidthMC = (tsWidthFullMC >> 1) & ~1;  // must be even for 32x32 blocks 
    }

    threadSpace   = 0;
    threadSpace2  = 0;
    threadSpaceMC = 0;

    res = kernelMeB->SetThreadCount(tsWidth * tsHeight);
    MCTF_CHECK_CM_ERR(res, res);
    res = device->CreateThreadSpace(tsWidth, tsHeight, threadSpace);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB->AssociateThreadSpace(threadSpace);
    MCTF_CHECK_CM_ERR(res, res);

    res = kernelMeB2->SetThreadCount(tsWidth * tsHeight);
    MCTF_CHECK_CM_ERR(res, res);
    res = device->CreateThreadSpace(tsWidth, tsHeight, threadSpace2);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMeB2->AssociateThreadSpace(threadSpace2);
    MCTF_CHECK_CM_ERR(res, res);

    res = kernelMc2r->SetThreadCount(tsWidthMC * tsHeightMC);
    MCTF_CHECK_CM_ERR(res, res);
    res = device->CreateThreadSpace(tsWidthMC, tsHeightMC, threadSpaceMC);
    MCTF_CHECK_CM_ERR(res, res);
    res = kernelMc2r->AssociateThreadSpace(threadSpaceMC);
    MCTF_CHECK_CM_ERR(res, res);

    if (task)
        res = task->Reset();
    else
        res = device->CreateTask(task);
    MCTF_CHECK_CM_ERR(res, res);

    res = task->AddKernel(kernelMeB);
    MCTF_CHECK_CM_ERR(res, res);
    if (pMCTF_NOA_func && p_ctrl->th == MCTFADAPTIVEVAL)
    {
        res = MCTF_Enqueue(task, e);
        MCTF_CHECK_CM_ERR(res, res);
        res = e->WaitForTaskFinished();
        MCTF_CHECK_CM_ERR(res, res);

        res = device->DestroyThreadSpace(threadSpace);
        MCTF_CHECK_CM_ERR(res, res);

        e->GetExecutionTime(executionTime);
        exeTime += executionTime / 1000;
        exeTimeT += executionTime / 1000;

        (this->*(pMCTF_NOA_func))(m_adaptControl);
        if (QfIn[1].filterStrength == 0)
        {
            MctfState = AMCTF_READY;
            return CM_SUCCESS;
        }

        if (mcSufIndex == 0)
            res = MCTF_SET_KERNELMc2r(0, 0);
        else if (mcSufIndex == 1)
            res = MCTF_SET_KERNELMc4r(0, 0, DEN_CLOSE_RUN);
        else
            res = MCTF_SET_KERNELMc4r(0, 0, DEN_FAR_RUN);
        MCTF_CHECK_CM_ERR(res, res);

        res = task->Reset();
        MCTF_CHECK_CM_ERR(res, res);

    }
    else {
        res = task->AddSync();
        if (mcSufIndex == 0)
            res = MCTF_SET_KERNELMc2r(0, 0);
        else if (mcSufIndex == 1)
            res = MCTF_SET_KERNELMc4r(0, 0, DEN_CLOSE_RUN);
        else
            res = MCTF_SET_KERNELMc4r(0, 0, DEN_FAR_RUN);
        MCTF_CHECK_CM_ERR(res, res);
    }

    res = task->AddKernel(kernelMeB2);
    MCTF_CHECK_CM_ERR(res, res);
    res = task->AddSync();
    MCTF_CHECK_CM_ERR(res, res);
    res = task->AddKernel(kernelMc2r);
    MCTF_CHECK_CM_ERR(res, res);

    res = MCTF_Enqueue(task, e);
    MCTF_CHECK_CM_ERR(res, res);
    MctfState = AMCTF_READY;
    res = e->WaitForTaskFinished();
    MCTF_CHECK_CM_ERR(res, res);
    e->GetExecutionTime(executionTime);
    exeTime += executionTime / 1000;

    if (!pMCTF_NOA_func) {
        res = device->DestroyThreadSpace(threadSpace);
        MCTF_CHECK_CM_ERR(res, res);
    }
    
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
    MctfState = AMCTF_READY;
    task = 0;
    return res;
}

mfxI32 CMC::MCTF_RUN_ME_MC_H(
    SurfaceIndex * GenxRefs,
    SurfaceIndex * GenxRefs2,
    SurfaceIndex * idxMV,
    SurfaceIndex * idxMV2,
    mfxI8          forwardRefDist,
    mfxI8           backwardRefDist,
    mfxU8          mcSufIndex
)
{
    UINT64 executionTime;
    mfxU8
        blSize = SetOverlapOp_half();
    res = MCTF_SET_KERNELMeBi(GenxRefs, GenxRefs2, idxMV, idxMV2, DIVUP(p_ctrl->CropX, blSize), DIVUP(p_ctrl->CropY, blSize), blSize, forwardRefDist, backwardRefDist);

    MCTF_CHECK_CM_ERR(res, res);

    threadSpace = 0;

    res = MCTF_RUN_TASK(kernelMeB, task != 0);
    res = e->WaitForTaskFinished();
    MCTF_CHECK_CM_ERR(res, res);
    e->GetExecutionTime(executionTime);
    exeTime += executionTime / 1000;
    
    mfxU16 multiplier = 2;
    tsHeightMC = (DIVUP(p_ctrl->CropH, blsize) * multiplier);
    tsWidthFullMC = (DIVUP(p_ctrl->CropW, blsize) * multiplier);
    tsWidthMC = tsWidthFullMC;

    if (pMCTF_NOA_func)
    {
        res = MCTF_Enqueue(task, e);
        MCTF_CHECK_CM_ERR(res, res);
        res = e->WaitForTaskFinished();
        MCTF_CHECK_CM_ERR(res, res);
        res = device->DestroyThreadSpace(threadSpace);
        MCTF_CHECK_CM_ERR(res, res);

        res = device->DestroyTask(task);
        MCTF_CHECK_CM_ERR(res, res);
        e->GetExecutionTime(executionTime);
        exeTime += executionTime / 1000;
        res = queue->DestroyEvent(e);
        MCTF_CHECK_CM_ERR(res, res);
        task = 0;
        e = 0;
        (this->*(pMCTF_NOA_func))(m_adaptControl);
    }
    else
        res = task->AddSync();
    res = MCTF_SET_KERNELMeBi(GenxRefs, GenxRefs2, idxMV, idxMV2, DIVUP(p_ctrl->CropX, blsize), tsHeight, blSize, forwardRefDist, backwardRefDist);

    MCTF_CHECK_CM_ERR(res, res);
    if (mcSufIndex == 0)
        res = MCTF_SET_KERNELMc2r(DIVUP(p_ctrl->CropX, blSize) * 2, DIVUP(p_ctrl->CropY, blSize) * 2);
    else if (mcSufIndex == 1)
        res = MCTF_SET_KERNELMc4r(DIVUP(p_ctrl->CropX, blSize) * 2, DIVUP(p_ctrl->CropY, blSize) * 2, DEN_CLOSE_RUN);
    else
        res = MCTF_SET_KERNELMc4r(DIVUP(p_ctrl->CropX, blSize) * 2, DIVUP(p_ctrl->CropY, blSize) * 2, DEN_FAR_RUN);
    MCTF_CHECK_CM_ERR(res, res);

    threadSpace2 = 0;
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
    if (threadSpace)
    {
        res = device->DestroyThreadSpace(threadSpace);
        MCTF_CHECK_CM_ERR(res, res);
    }

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

void CMC::GET_DISTDATA()
{
    for (int y = 0; y < ov_height_bl; y++)
    {
        mfxU32 * src = reinterpret_cast<mfxU32*>(reinterpret_cast<mfxU8*>(distSys) + y * surfPitch);
        std::copy(src, src + ov_width_bl, &distRef[y * ov_width_bl]);
    }
}

void CMC::GET_DISTDATA_H()
{
    for (int y = 0; y < ov_height_bl / 2; y++)
    {
        mfxU32 * src = reinterpret_cast<mfxU32*>(reinterpret_cast<mfxU8*>(distSys) + y * surfPitch);
        std::copy(src, src + ov_width_bl, &distRef[y * ov_width_bl]);
    }
}

void CMC::GET_NOISEDATA()
{
    for (int y = 0; y < DIVUP(p_ctrl->CropH, 16); y++)
    {
        spatialNoiseAnalysis * src = reinterpret_cast<spatialNoiseAnalysis*>(reinterpret_cast<mfxU8*>(noiseAnalysisSys) + y * surfNoisePitch);
        std::copy(src, src + DIVUP(p_ctrl->CropW, 16), &var_sc[y * DIVUP(p_ctrl->CropW, 16)]);
    }
}

mfxF64 CMC::GET_TOTAL_SAD()
{
    mfxF64 total_sad = 0.0;
    // to not lose due-to normalization of floats
    mfxU64 uTotalSad = 0;
    switch (overlap_Motion)
    {
    case MFX_CODINGOPTION_ON:
    {//overlapped modes, need to remove extra SAD blocks and lines
        mfxI32
            pos_offset = 0;
        for (mfxI32 i = 0; i < ov_height_bl; i += OVERLAP_OFFSET)
        {
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
    total_sad = mfxF64(uTotalSad);
    return total_sad / (p_ctrl->CropW * p_ctrl->CropH);
}

mfxU16 CalcNoiseStrength(
    double NSC,
    double NSAD
)
{
    // 10 epsilons
    if (std::fabs(NSC) <= 10 * std::numeric_limits<double>::epsilon()) return 0;
    mfxF64
        s,
        s2,

        c3 = -907.05,
        c2 =  752.69,
        c1 = -175.7,
        c0 =  14.6,
        d3 = -0.0000004,
        d2 =  0.0002,
        d1 = -0.0245,
        d0 =  4.1647,

        ISTC = NSAD * NSC,
        STC = NSAD / sqrt(NSC);

    s  = c3 * pow(STC, 3.0) + c2 * pow(STC, 2.0) + c1 * STC + c0;
    s2 = d3 * pow(ISTC, 3.0) + d2 * pow(ISTC, 2.0) + d1 * ISTC + d0;
    s = NMIN(s, s2) + 5;
    s  = NMAX(0.0, NMIN(20.0, s));
    return (mfxU16)(s + 0.5);
}


mfxU8 CalcSTC(mfxF64 SCpp2, mfxF64 sadpp)
{
    mfxU8
        stcVal = 0;
    sadpp *= sadpp;
    // Ref was Recon (quantization err in ref biases sad)
    if      (sadpp < 0.03*SCpp2)  stcVal = 0; // Very-Low
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
    mfxU8
        i;
    mfxF64
        SCpp2 = QfIn[currentFrame].frame_sc;
    static mfxF32
        lmt_sc2[10] = { 16.0, 81.0, 225.0, 529.0, 1024.0, 1764.0, 2809.0, 4225.0, 6084.0, (mfxF32)INT_MAX }; // lower limit of SFM(Rs,Cs) range for spatial classification

    for (i = 0; i < 10; i++)
    {
        if (SCpp2 < lmt_sc2[i])
        {
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
    for (i = 0; i < 10; i++)
    {
        if (sadpp < lmt_tc[i])
        {
            QfIn[currentFrame].tc = i;
            break;
        }
    }
    QfIn[currentFrame].stc = CalcSTC(SCpp2, sadpp);
}

mfxU32 CMC::computeQpClassFromBitRate(
    mfxU8 currentFrame
)
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
    return QCL;
}

bool CMC::FilterEnable(
   gpuFrameData frame)
{
    mfxF64
        noise_sad    = frame.noise_sad;
    mfxU32
        noise_count  = frame.noise_count;
    bool
        enableFilter = false;
    //Enabling/Disabling filter model coeficcients
    mfxF64
        s,
#if defined(MFX_ENABLE_SOFT_CURVE)
        c2 = 165.51,
        c1 = -1614.4,
        c0 = 4085.7;
    s = c2 * pow(noise_sad, 2) + c1 * noise_sad + c0;
#else
        c1 = 574.5,
        c0 = 2409.925;
    s = c1 * noise_sad + c0;
#endif

    
    enableFilter = (mfxF64)noise_count > s;
    return enableFilter;
}

mfxI32 CMC::noise_estimator(
    bool adaptControl
)
{
    mfxU8
        currentFrame = (number_of_References <= 2) ? 1 : 2,
        previousFrame = currentFrame - 1;
    mfxU32
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
    MCTF_CHECK_CM_ERR(res, res);

    GET_NOISEDATA();
    mfxU32 distRefStride = 2 * width;
    switch (overlap_Motion)
    {
    case MFX_CODINGOPTION_ON:
    {
        for (row = 1; row < height / 2 - 1; row++)
        {
            for (col = 1; col < width - 1; col++)
            {
                var = var_sc[row * width + col].var;
                SCpp = var_sc[row * width + col].SCpp;
                QfIn[currentFrame].frame_sc += SCpp;
                SADpp = (mfxF32)(distRef[row * 2 * width * 2 + col * 2] / 256);
                QfIn[currentFrame].frame_sad += SADpp;
                if (var < tvar && SCpp <tvar && SCpp>1.0 && (SADpp*SADpp) <= SCpp)
                {
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
        for (row = 1; row < height / 2 - 1; row++)
        {
            for (col = 1; col < width - 1; col++)
            {
                var = var_sc[row * width + col].var;
                SCpp = var_sc[row * width + col].SCpp;
                QfIn[currentFrame].frame_sc += SCpp;
                // NB: division by 256 is done in integers;
                // thus, for each calculation, 8 LSB are truncated
                // works good; but additional efforts can be taken,
                // to consider higher precision (thru calculations in integers)
                // followed by wise truncating.
                SADpp = (mfxF32)((distRef[row * 2 * distRefStride + col * 2] +
                    distRef[row * 2 * distRefStride + col * 2 + 1] +
                    distRef[(row * 2 + 1) * distRefStride + col * 2] +
                    distRef[(row * 2 + 1) * distRefStride + col * 2 + 1]) / 256);
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
        filterstrength = MCTFSTRENGTH;
    if (QfIn[currentFrame].scene_idx != QfIn[previousFrame].scene_idx)
    {
        filterstrength = QfIn[previousFrame].filterStrength;
    }
    else
    {
        filterstrength = CalcNoiseStrength(QfIn[currentFrame].noise_sc, QfIn[currentFrame].noise_sad);
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

            mfxI32 limit = filterstrength > 14 ? 13 : 20;
            filterstrength = mfxU16(max(0, min(limit, (mfxI16)filterstrength + fsModVal)));
        }
    }
    if (adaptControl)
    {
        filterstrength = FilterEnable(QfIn[currentFrame]) ? filterstrength : MCTFNOFILTER;
        m_doFilterFrame = !!filterstrength;
    }
    QfIn[currentFrame].filterStrength = filterstrength;
    gopBasedFilterStrength = filterstrength;
    if (adaptControl)
        res = SetFilterStrenght(filterstrength, MCTFNOFILTER);
    else
        res = SetFilterStrenght(filterstrength);
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_RUN_Noise_Analysis(mfxU8 srcNum)
{
    res = MCTF_SET_KERNEL_Noise(srcNum, DIVUP(p_ctrl->CropX, 16), DIVUP(p_ctrl->CropY, 16));
    MCTF_CHECK_CM_ERR(res, res);
    mfxU16
        tsHeightNA = DIVUP(p_ctrl->CropH, 16),
        tsWidthFullNA = DIVUP(p_ctrl->CropW, 16),
        tsWidthNA = tsWidthFullNA;

    if (tsWidthFullNA > CM_MAX_THREADSPACE_WIDTH_FOR_MW)
        tsWidthNA = (tsWidthFullNA >> 1) & ~1;  // must be even for 32x32 blocks 

    threadSpace = 0;
    res = MCTF_RUN_TASK_NA(kernelNoise, task != 0, tsWidthNA, tsHeightNA);
    MCTF_CHECK_CM_ERR(res, res);

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW)
    {
        mfxU16
            start_mbX = tsWidth;
        tsWidth = tsWidthFull - tsWidth;
        res = MCTF_SET_KERNEL_Noise(srcNum, start_mbX, DIVUP(p_ctrl->CropY, 16));
        MCTF_CHECK_CM_ERR(res, res);
        if (threadSpace != NULL)
        {
            res = device->DestroyThreadSpace(threadSpace);
            MCTF_CHECK_CM_ERR(res, res);
        }
        // the rest of frame TS
        res = MCTF_RUN_TASK_NA(kernelNoise, task != 0, tsWidthNA, tsHeightNA);
        MCTF_CHECK_CM_ERR(res, res);
    }
    res = e->WaitForTaskFinished();
    MCTF_CHECK_CM_ERR(res, res);
    device->DestroyThreadSpace(threadSpace);
    queue->DestroyEvent(e);
    e = 0;
    return res;
}

mfxI32 CMC::MCTF_RUN_BLEND()
{
    mfxU16 multiplier = 2;
    res = MCTF_SET_KERNELMc(DIVUP(p_ctrl->CropX, blsize) * multiplier, DIVUP(p_ctrl->CropY, blsize) * multiplier, 1, 0);
    MCTF_CHECK_CM_ERR(res, res);
    tsHeight = (DIVUP(p_ctrl->CropH, blsize) * multiplier);
    tsWidthFull = (DIVUP(p_ctrl->CropW, blsize) * multiplier);
    tsWidth = tsWidthFull;

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW)
        tsWidth = (tsWidthFull >> 1) & ~1;  // must be even for 32x32 blocks 

    threadSpace = 0;
    res = MCTF_RUN_TASK(kernelMc1r, task != 0);
    MCTF_CHECK_CM_ERR(res, res);

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW)
    {
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
    device->DestroyThreadSpace(threadSpace);
    queue->DestroyEvent(e);
    e = 0;
    return res;
}

mfxI32 CMC::MCTF_RUN_BLEND(
    mfxU8 srcNum,
    mfxU8 refNum
)
{
    mfxU16 multiplier = 2;
    res = MCTF_SET_KERNELMc(DIVUP(p_ctrl->CropX, blsize) * multiplier , DIVUP(p_ctrl->CropY, blsize) * multiplier, srcNum, refNum);
    MCTF_CHECK_CM_ERR(res, res);
    tsHeight = (DIVUP(p_ctrl->CropH, blsize) * multiplier);
    tsWidthFull = (DIVUP(p_ctrl->CropW, blsize) * multiplier);
    tsWidth = tsWidthFull;

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW)
        tsWidth = (tsWidthFull >> 1) & ~1;  // must be even for 32x32 blocks 

    threadSpace = 0;
    res = MCTF_RUN_TASK(kernelMc1r, task != 0);
    MCTF_CHECK_CM_ERR(res, res);

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW)
    {
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
    device->DestroyThreadSpace(threadSpace);
    queue->DestroyEvent(e);
    e = 0;
    return res;
}

mfxI32 CMC::MCTF_RUN_BLEND2R()
{
    mfxU16 multiplier = 2;
    res = MCTF_SET_KERNELMc2r(DIVUP(p_ctrl->CropX, blsize) * multiplier, DIVUP(p_ctrl->CropY, blsize) * multiplier);
    MCTF_CHECK_CM_ERR(res, res);

    tsHeightMC = (DIVUP(p_ctrl->CropH, blsize) * multiplier);
    tsWidthFullMC = (DIVUP(p_ctrl->CropW, blsize) * multiplier);
    tsWidthMC = tsWidthFullMC;

    if (tsWidthFullMC > CM_MAX_THREADSPACE_WIDTH_FOR_MW)
        tsWidthMC = (tsWidthFullMC >> 1) & ~1;  // must be even for 32x32 blocks 

    threadSpaceMC = 0;
    res = MCTF_RUN_MCTASK(kernelMc2r, task != 0);
    MCTF_CHECK_CM_ERR(res, res);

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW)
    {
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
    UINT64 executionTime;
    e->GetExecutionTime(executionTime);
    exeTime += executionTime / 1000;
    device->DestroyThreadSpace(threadSpaceMC);
    queue->DestroyEvent(e);
    e = 0;
    return res;
}

mfxI32 CMC::MCTF_RUN_BLEND2R_DEN()
{
    mfxU16 multiplier = 2;
    res = MCTF_SET_KERNELMc2rDen(DIVUP(p_ctrl->CropX, blsize) * multiplier, DIVUP(p_ctrl->CropY, blsize) * multiplier);
    MCTF_CHECK_CM_ERR(res, res);
    tsHeight = (DIVUP(p_ctrl->CropH, blsize) * multiplier);
    tsWidthFull = (DIVUP(p_ctrl->CropW, blsize) * multiplier);
    tsWidth = tsWidthFull;

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW)
        tsWidth = (tsWidthFull >> 1) & ~1;  // must be even for 32x32 blocks 

    threadSpace = 0;
    res = MCTF_RUN_TASK(kernelMc2r, task != 0);
    MCTF_CHECK_CM_ERR(res, res);

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW)
    {
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
    device->DestroyThreadSpace(threadSpace);
    queue->DestroyEvent(e);
    e = 0;
    return res;
}

mfxI32 CMC::MCTF_RUN_BLEND4R(
    DRT typeRun
)
{
    if (typeRun > 1)
        return MFX_ERR_INVALID_VIDEO_PARAM;
    mfxU16 multiplier = 2;
    res = MCTF_SET_KERNELMc4r(DIVUP(p_ctrl->CropX, blsize) * multiplier, DIVUP(p_ctrl->CropY, blsize) * multiplier, (mfxU8)typeRun);
    MCTF_CHECK_CM_ERR(res, res);
    tsHeight = (DIVUP(p_ctrl->CropH, blsize) * multiplier);
    tsWidthFull = (DIVUP(p_ctrl->CropW, blsize) * multiplier);
    tsWidth = tsWidthFull;

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW)
        tsWidth = (tsWidthFull >> 1) & ~1;  // must be even for 32x32 blocks

    threadSpace = 0;
    res = MCTF_RUN_TASK(kernelMc2r, task != 0);
    MCTF_CHECK_CM_ERR(res, res);

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW)
    {
        mfxU16
            start_mbX = tsWidth;
        tsWidth = tsWidthFull - tsWidth;
        res = MCTF_SET_KERNELMc4r(start_mbX, DIVUP(p_ctrl->CropY, blsize) * multiplier, (mfxU8)typeRun);
        MCTF_CHECK_CM_ERR(res, res);
        // the rest of frame TS
        res = MCTF_RUN_TASK(kernelMc2r, task != 0);
        MCTF_CHECK_CM_ERR(res, res);
    }
    res = e->WaitForTaskFinished();
    MCTF_CHECK_CM_ERR(res, res);
    device->DestroyThreadSpace(threadSpace);
    queue->DestroyEvent(e);
    e = 0;
    return res;
}

mfxI32 CMC::MCTF_RUN_MERGE()
{
    res = MCTF_SET_KERNELMcMerge(DIVUP(p_ctrl->CropX, 16), DIVUP(p_ctrl->CropY, 16));
    MCTF_CHECK_CM_ERR(res, res);
    tsHeight = DIVUP(p_ctrl->CropH, 16);
    tsWidthFull = DIVUP(p_ctrl->CropW, 16);
    tsWidth = tsWidthFull;

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW)
        tsWidth = (tsWidthFull >> 1) & ~1;  // must be even for 32x32 blocks 

    threadSpace = 0;
    res = MCTF_RUN_TASK(kernelMc4r, task != 0);
    MCTF_CHECK_CM_ERR(res, res);

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW)
    {
        mfxU16
            start_mbX = tsWidth;
        tsWidth = tsWidthFull - tsWidth;
        res = MCTF_SET_KERNELMc4r(start_mbX, DIVUP(p_ctrl->CropY, 16));
        MCTF_CHECK_CM_ERR(res, res);
        // the rest of frame TS
        res = MCTF_RUN_TASK(kernelMc4r, task != 0);
        MCTF_CHECK_CM_ERR(res, res);
    }
    res = e->WaitForTaskFinished();
    MCTF_CHECK_CM_ERR(res, res);
    device->DestroyThreadSpace(threadSpace);
    queue->DestroyEvent(e);
    e = 0;
    return res;
}

mfxI32 CMC::MCTF_RUN_BLEND4R(
    SurfaceIndex * multiIndex
)
{
    mfxU16 multiplier = 2;
    res = MCTF_SET_KERNELMc4r(DIVUP(p_ctrl->CropX, blsize) * multiplier, DIVUP(p_ctrl->CropY, blsize) * multiplier, multiIndex);
    MCTF_CHECK_CM_ERR(res, res);
    tsHeight = (DIVUP(p_ctrl->CropH, blsize) * multiplier);
    tsWidthFull = (DIVUP(p_ctrl->CropW, blsize) * multiplier);
    tsWidth = tsWidthFull;

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW)
        tsWidth = (tsWidthFull >> 1) & ~1;  // must be even for 32x32 blocks 

    threadSpace = 0;
    res = MCTF_RUN_TASK(kernelMc2r, task != 0);
    MCTF_CHECK_CM_ERR(res, res);

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW)
    {
        mfxU16
            start_mbX = tsWidth;
        tsWidth = tsWidthFull - tsWidth;
        res = MCTF_SET_KERNELMc4r(start_mbX, DIVUP(p_ctrl->CropY, blsize) * multiplier, multiIndex);
        MCTF_CHECK_CM_ERR(res, res);
        res = MCTF_RUN_TASK(kernelMc2r, task != 0);
        MCTF_CHECK_CM_ERR(res, res);
    }
    res = e->WaitForTaskFinished();
    MCTF_CHECK_CM_ERR(res, res);
    device->DestroyThreadSpace(threadSpace);
    queue->DestroyEvent(e);
    e = 0;
    return res;
}

void CMC::RotateBufferA()
{
    std::swap(QfIn[0], QfIn[1]);
    std::swap(QfIn[0], QfIn[2]);
}

void CMC::RotateBufferB()
{
    std::swap(QfIn[0], QfIn[1]);
    std::swap(QfIn[1], QfIn[2]);
    std::swap(QfIn[2], QfIn[3]);
}

void CMC::RotateBuffer()
{
    mfxU8 correction = ((QfIn.size() > 3) && (firstFrame < 3)) << 1;
    for (mfxU8 i = 0; i < QfIn.size() - 1 - correction; i++)
        std::swap(QfIn[i], QfIn[i + 1]);
}

void CMC::AssignSceneNumber()
{
    for (mfxU8 i = 0; i < QfIn.size(); i++)
        scene_numbers[i] = QfIn[i].scene_idx;
}

mfxI32 CMC::MCTF_RUN_ME_1REF()
{
    res = MCTF_RUN_ME(genxRefs1, idxMv_1);
    MCTF_CHECK_CM_ERR(res, res);

    res = e->WaitForTaskFinished();
    MCTF_CHECK_CM_ERR(res, res);
    UINT64 executionTime;
    e->GetExecutionTime(executionTime);
    exeTime += executionTime / 1000;
    device->DestroyThreadSpace(threadSpace);
    queue->DestroyEvent(e);
    device->DestroyVmeSurfaceG7_5(genxRefs1);
    e = 0;

    if (pMCTF_NOA_func)
        (this->*(pMCTF_NOA_func))(m_adaptControl);

    return res;
}

mfxI32 CMC::MCTF_RUN_ME_2REF_HE()
{
    res = MCTF_RUN_ME_MC_HE(genxRefs1, genxRefs2, idxMv_1, idxMv_2, forward_distance, backward_distance, 0);
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_RUN_ME_2REF()
{
    res = MCTF_RUN_ME_MC_H(genxRefs1, genxRefs2, idxMv_1, idxMv_2, forward_distance, backward_distance, 0);
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_RUN_ME_4REF()
{
    res = MCTF_RUN_ME_MC_H(genxRefs1, genxRefs2, idxMv_1, idxMv_2, forward_distance, backward_distance, 1);
    MCTF_CHECK_CM_ERR(res, res);
    res = MCTF_RUN_ME_MC_H(genxRefs3, genxRefs4, idxMv_3, idxMv_4, forward_distance, backward_distance, 2);
    return res;
}


mfxI32 CMC::MCTF_BLEND4R()
{
    res = MCTF_RUN_BLEND4R(DEN_CLOSE_RUN);
    MCTF_CHECK_CM_ERR(res, res);
    res = MCTF_RUN_BLEND4R(DEN_FAR_RUN);
    MCTF_CHECK_CM_ERR(res, res);
    res = MCTF_RUN_MERGE();
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_RUN_MCTF_DEN_1REF(
    bool
)
{
    if (pMCTF_LOAD_func)
    {
        res = (this->*(pMCTF_LOAD_func))();
        MCTF_CHECK_CM_ERR(res, res);
    }
    AssignSceneNumber();
    if (pMCTF_ME_func)
    {
        res = (this->*(pMCTF_ME_func))();
        MCTF_CHECK_CM_ERR(res, res);
    }
    if (pMCTF_MERGE_func)
    {
        res = (this->*(pMCTF_MERGE_func))();
        MCTF_CHECK_CM_ERR(res, res);
    }
    if (pMCTF_SpDen_func)
        res = (this->*(pMCTF_SpDen_func))();
    RotateBuffer();
    return res;
}

mfxI32 CMC::MCTF_RUN_MCTF_DEN(
    bool notInPipeline
)
{
    if ((QfIn[1].filterStrength > 0) || notInPipeline)
    {
        if (pMCTF_LOAD_func)
        {
            res = (this->*(pMCTF_LOAD_func))();
            MCTF_CHECK_CM_ERR(res, res);
        }
        AssignSceneNumber();
        if (pMCTF_ME_func)
        {
            res = (this->*(pMCTF_ME_func))();
            MCTF_CHECK_CM_ERR(res, res);
        }
        if (pMCTF_MERGE_func)
        {
            res = (this->*(pMCTF_MERGE_func))();
            MCTF_CHECK_CM_ERR(res, res);
        }
        if (pMCTF_SpDen_func)
            res = (this->*(pMCTF_SpDen_func))();
    }
    RotateBuffer();
    return res;
}

mfxI32 CMC::MCTF_RUN_MCTF_DEN_4REF(
    bool
)
{
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
    return res;
}

mfxI32 CMC::MCTF_RUN_AMCTF_DEN()
{
    res = (this->*(pMCTF_LOAD_func))();
    MCTF_CHECK_CM_ERR(res, res);
    AssignSceneNumber();
    res = MCTF_RUN_ME(genxRefs1, genxRefs2, idxMv_1, idxMv_2, forward_distance, backward_distance);
    MCTF_CHECK_CM_ERR(res, res);
    res = MCTF_RUN_BLEND2R();
    MCTF_CHECK_CM_ERR(res, res);
    res = MCTF_RUN_Denoise();
    MCTF_CHECK_CM_ERR(res, res);
    RotateBuffer();
    return res;
}

mfxI32 CMC::MCTF_RUN_AMCTF()
{
    res = MCTF_RUN_Denoise(1);
    MCTF_CHECK_CM_ERR(res, res);
    return res;
}

mfxI32 CMC::MCTF_RUN_AMCTF(mfxU16 srcNum)
{
    res = MCTF_RUN_Denoise(srcNum);
    MCTF_CHECK_CM_ERR(res, res);
    MctfState = AMCTF_READY;
    return res;
}

mfxI32 CMC::MCTF_RUN_Denoise(mfxU16 srcNum)
{
    if (QfIn[srcNum].filterStrength == 21)
        p_ctrl->sTh = MCTFSTRENGTH * 50 / 25;
    else
        p_ctrl->sTh = QfIn[srcNum].filterStrength * 50 / 25;
    res = ctrlBuf->WriteSurface((const mfxU8 *)p_ctrl.get(), NULL, sizeof(MeControlSmall));
    MCTF_CHECK_CM_ERR(res, res);
    res = MCTF_SET_KERNELDe(srcNum, 0, 0);
    MCTF_CHECK_CM_ERR(res, res);
    p_ctrl->sTh = 0;
    tsHeight    = DIVUP(p_ctrl->height, 8);
    tsWidthFull = DIVUP(p_ctrl->width,  8);
    tsWidth     = tsWidthFull;

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW) {
        tsWidth = (tsWidthFull >> 1) & ~1;  // must be even for 32x32 blocks 
    }

    threadSpace = 0;
    res = MCTF_RUN_TASK(kernelMcDen, task != 0);
    MCTF_CHECK_CM_ERR(res, res);

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW)
    {
        mfxU16
            start_mbX = tsWidth;
        tsWidth = tsWidthFull - tsWidth;
        res = MCTF_SET_KERNELDe(srcNum, start_mbX, 0);
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
    return res;
}

mfxI32 CMC::MCTF_RUN_Denoise()
{
    res = MCTF_SET_KERNELDe(DIVUP(p_ctrl->CropX, 8), DIVUP(p_ctrl->CropY, 8));
    MCTF_CHECK_CM_ERR(res, res);
    tsHeight = DIVUP(p_ctrl->CropH, 8);
    tsWidthFull = DIVUP(p_ctrl->CropW, 8);
    tsWidth = tsWidthFull;

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW)
        tsWidth = (tsWidthFull >> 1) & ~1;  // must be even for 32x32 blocks

    threadSpace = 0;
    res = MCTF_RUN_TASK(kernelMcDen, task != 0);
    MCTF_CHECK_CM_ERR(res, res);

    if (tsWidthFull > CM_MAX_THREADSPACE_WIDTH_FOR_MW)
    {
        mfxU16
            start_mbX = tsWidth;
        tsWidth = tsWidthFull - tsWidth;
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
    return res;
}

mfxU32 CMC::IM_SURF_PUT(
    CmSurface2D *p_surface,
    mfxU8 *p_data)
{
    CM_STATUS
        status = CM_STATUS_FLUSHED;
    res = queue->EnqueueCopyCPUToGPU(p_surface, p_data, copyEv);
    MCTF_CHECK_CM_ERR(res, res);
    copyEv->GetStatus(status);
    while (status != CM_STATUS_FINISHED)
        copyEv->GetStatus(status);
    return res;
}

void CMC::IntBufferUpdate(
    bool isSceneChange,
    bool isIntraFrame,
    bool doIntraFiltering)
{
    size_t
        buffer_size = QfIn.size() - 1;
    if (bufferCount > buffer_size) {
        printf("Error: Invalid frame buffer position\n");
        exit(-1);
    }
    if (bufferCount == 0)
        QfIn.back().frame_number = 0;
    else
        QfIn.back().frame_number = (QfIn.end() - 2)->frame_number + 1;

    if (!firstFrame)
        sceneNum += isSceneChange;

    QfIn.back().isSceneChange = (firstFrame || isSceneChange);
    QfIn.back().isIntra = isIntraFrame;

    if (isSceneChange || isIntraFrame || bufferCount == 0)
        countFrames = 0;
    else
        countFrames++;
    QfIn.back().frame_relative_position = countFrames;
    QfIn.back().frame_added             = false;
    QfIn.back().scene_idx               = sceneNum;
    if((QfIn.back().isSceneChange || QfIn.back().isIntra) && doIntraFiltering)
        m_doFilterFrame = true;
}

void CMC::BufferFilterAssignment(
    mfxU16 * filterStrength,
    bool     doIntraFiltering,
    bool     isAnchorFrame,
    bool     isSceneChange)
{
    if (!filterStrength)
    {
        if (isAnchorFrame && isSceneChange)
        {
            QfIn.back().filterStrength = doIntraFiltering ? MCTFSTRENGTH : MCTFNOFILTER;
#ifdef MFX_MCTF_DEBUG_PRINT
            ASC_PRINTF("\nI frame denoising is: %i, strength set at: %i\n", doIntraFiltering, QfIn[bufferCount].filterStrength));
#endif
            gopBasedFilterStrength = MCTFADAPTIVE;
        }
        else if (isAnchorFrame)
            QfIn.back().filterStrength = gopBasedFilterStrength;
        else
            QfIn.back().filterStrength = MCTFNOFILTER;
    }
    else
        QfIn.back().filterStrength = *filterStrength;
}

mfxStatus CMC::MCTF_PUT_FRAME(
    void         * frameInData,
    mfxHDLPair     frameOutHandle,
    CmSurface2D ** frameOutData,
    bool           isCmSurface,
    mfxU16       * filterStrength,
    bool           needsOutput,
    bool           doIntraFiltering
)
{
    if (!frameInData)
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    if (isCmSurface)
    {
        mfxFrameSurface1 *
            mfxFrame = (mfxFrameSurface1 *)frameInData;
        mfxHDLPair
            handle;
        // if a surface is opaque, need to extract normal surface
        mfxFrameSurface1
            * pSurf = m_pCore->GetNativeSurface(mfxFrame);
        QfIn.back().mfxFrame = pSurf ? pSurf : mfxFrame;
        if (pSurf)
        {
            MFX_SAFE_CALL(m_pCore->GetFrameHDL(QfIn.back().mfxFrame->Data.MemId, reinterpret_cast<mfxHDL*>(&handle)));
        }
        else
        {
            MFX_SAFE_CALL(m_pCore->GetExternalFrameHDL(QfIn.back().mfxFrame->Data.MemId, reinterpret_cast<mfxHDL*>(&handle)));
        }
        MFX_SAFE_CALL(IM_SURF_SET(reinterpret_cast<AbstractSurfaceHandle>(handle.first), &QfIn.back().frameData, &QfIn.back().fIdx));
    }
    else
    {
        res = IM_SURF_PUT(QfIn.back().frameData, (mfxU8*)frameInData);
        MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    }
    QfIn.back().frame_added = true;
    if (needsOutput)
    {
        SurfaceIndex
            *fIdx = nullptr;
        MFX_SAFE_CALL(IM_SURF_SET(reinterpret_cast<AbstractSurfaceHandle>(frameOutHandle.first), frameOutData, &fIdx));
        QfIn.back().fOut    = *frameOutData;
        QfIn.back().fIdxOut = fIdx;
    }

    if (m_doFilterFrame)
        BufferFilterAssignment(filterStrength, doIntraFiltering, needsOutput, QfIn.back().isSceneChange);
    else
        QfIn.back().filterStrength = MCTFNOFILTER;
    return MFX_ERR_NONE;
}

mfxStatus CMC::MCTF_PUT_FRAME(
    IntMctfParams * pMctfControl,
    CmSurface2D   * InSurf,
    CmSurface2D   * OutSurf
)
{
    lastFrame = 0;
    if (!InSurf)
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    INT cmSts = 0;

    SurfaceIndex* idxFrom;
    cmSts = InSurf->GetIndex(idxFrom);
    MFX_CHECK((CM_SUCCESS == cmSts), MFX_ERR_DEVICE_FAILED);
    
    MFX_SAFE_CALL(pSCD->PutFrameProgressive(idxFrom));

    MCTF_PUT_FRAME(
        pMctfControl,
        OutSurf,
        pSCD->Get_frame_shot_Decision()
    );
    return MFX_ERR_NONE;
}

mfxStatus CMC::MCTF_PUT_FRAME(
    IntMctfParams * pMctfControl,
    CmSurface2D   * OutSurf,
    mfxU32          schgDesicion
)
{
    lastFrame = 0;
    sceneNum += schgDesicion;
    MFX_SAFE_CALL(MCTF_UpdateRTParams(pMctfControl));
    MFX_SAFE_CALL(MCTF_PUT_FRAME(sceneNum, OutSurf));
    forward_distance = -1;
    backward_distance = 1;
    countFrames++;
    return MFX_ERR_NONE;
}

mfxStatus CMC::MCTF_DO_FILTERING_IN_AVC()
{
    // do filtering based on temporal mode & how many frames are
    // already in the queue:
    res = MFX_ERR_NONE;
    switch (number_of_References)
    {
    case 2://else if (number_of_References == 2)
    {
        if (bufferCount < 2) /*One frame delay*/
        {
            firstFrame = 0;
            RotateBuffer();
            MctfState = AMCTF_NOT_READY;
            return MFX_ERR_NONE;
        }

        MCTF_UpdateANDApplyRTParams(1);
        if (QfIn[1].fOut)
        {
            mco    = QfIn[1].fOut;
            idxMco = QfIn[1].fIdxOut;
            if (QfIn[1].isSceneChange)
            {
                if(QfIn[1].filterStrength)
                    res = MCTF_RUN_AMCTF(1);
                RotateBuffer();
            }
            else
                res = (this->*(pMCTF_func))(false);
            CurrentIdx2Out = DefaultIdx2Out;
            MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
            QfIn.front().fOut    = mco    = nullptr;
            QfIn.front().fIdxOut = idxMco = nullptr;
        }
        else
            RotateBuffer();
        break;
    };
    break;
    default://    else
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    MCTF_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    return MFX_ERR_NONE;
}

mfxU16 CMC::MCTF_QUERY_FILTER_STRENGTH()
{
    return QfIn[1].filterStrength;
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
            pMCTF_MERGE_func = NULL;
            //todo: check is it correct that the current frame is 0?
            MCTF_UpdateANDApplyRTParams(0);
            pMCTF_ME_func = &CMC::MCTF_RUN_ME_2REF;
            pMCTF_LOAD_func = &CMC::MCTF_LOAD_2REF;
            RotateBufferA();
            res = MCTF_RUN_MCTF_DEN(true);
            CurrentIdx2Out = 0;
            MctfState = AMCTF_READY;
            firstFrame = 2;
            break;
        }
        case 2:
        {
            //todo: check is it correct that the current frame is 1?
            MCTF_UpdateANDApplyRTParams(1);
            res = MCTF_RUN_MCTF_DEN(true);
            RotateBufferA();
            CurrentIdx2Out = 1;
            MctfState = AMCTF_READY;
            firstFrame = 3;

            pMCTF_MERGE_func = &CMC::MCTF_RUN_MERGE;
            pMCTF_ME_func = &CMC::MCTF_RUN_ME_4REF;
            pMCTF_LOAD_func = &CMC::MCTF_LOAD_4REF;
            break;
        }

        default:
        {
            MCTF_UpdateANDApplyRTParams(2);
            res = (this->*(pMCTF_func))(true);
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
                res = (this->*(pMCTF_func))(true);
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
            {
                res = MCTF_RUN_AMCTF();
                RotateBuffer();
            }
            else
                res = (this->*(pMCTF_func))(true);
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

void CMC::MCTF_CLOSE()
{
    if (kernelMe)
        device->DestroyKernel(kernelMe);
    if (kernelMeB)
        device->DestroyKernel(kernelMeB);
    if (kernelMeB2)
        device->DestroyKernel(kernelMeB2);
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
    if (idxMv_1)
    {
        device->DestroySurface2DUP(mv_1);
        CM_ALIGNED_FREE(mvSys1);
    }
    if (idxMv_2)
    {
        device->DestroySurface2DUP(mv_2);
        CM_ALIGNED_FREE(mvSys2);
    }
    if (idxMv_3)
    {
        device->DestroySurface2DUP(mv_3);
        CM_ALIGNED_FREE(mvSys3);
    }
    if (idxMv_4)
    {
        device->DestroySurface2DUP(mv_4);
        CM_ALIGNED_FREE(mvSys4);
    }
    if (distSurf)
    {
        device->DestroySurface2DUP(distSurf);
        CM_ALIGNED_FREE(distSys);
    }
    if (noiseAnalysisSurf)
    {
        device->DestroySurface2DUP(noiseAnalysisSurf);
        CM_ALIGNED_FREE(noiseAnalysisSys);
    }
  
    if (pSCD)
    {
        pSCD->Close();
        pSCD = nullptr;
    }
}
