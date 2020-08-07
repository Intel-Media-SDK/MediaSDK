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

#include "mfx_h264_la.h"
#include "mfx_common.h"

#if defined(MFX_ENABLE_H264_VIDEO_ENCODE_HW) && defined(MFX_ENABLE_LA_H264_VIDEO_HW)

#include <algorithm>
#include <numeric>

#include "cmrt_cross_platform.h"

#include "mfx_session.h"
#include "mfx_task.h"
#include "libmfx_core.h"
#include "libmfx_core_hw.h"
#include "libmfx_core_interface.h"
#include "mfx_ext_buffers.h"

#include "mfx_h264_encode_hw.h"
#include "mfx_enc_common.h"
#include "mfx_h264_enc_common_hw.h"
#include "mfx_h264_encode_hw_utils.h"

#include "mfx_h264_encode_cm.h"
#include "mfx_h264_encode_cm_defs.h"

#include "mfx_h264_la.h"
#include "mfx_ext_buffers.h"

using namespace MfxEncLA;

static mfxU16 GetGopSize(mfxVideoParam *par)
{
    return  par->mfx.GopPicSize == 0 ?  256 : par->mfx.GopPicSize ;
}
static mfxU16 GetRefDist(mfxVideoParam *par, eMFXHWType targetHw)
{
    mfxU16 GopSize = GetGopSize(par);
    mfxU16 refDist = par->mfx.GopRefDist;
    if (refDist == 0)
    {
        mfxExtLAControl *pControl = (mfxExtLAControl *)GetExtBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_LOOKAHEAD_CTRL);
        refDist = (
            IsOn(par->mfx.LowPower)
         && targetHw <= MFX_HW_TGL_LP )
          ? 1
          : ((pControl && IsOn(pControl->BPyramid)) ? 8 : 3);
    }
    return (std::min)(refDist, GopSize);
}
static mfxU16 GetAsyncDeph(mfxVideoParam *par)
{
    return par->AsyncDepth == 0 ? 3 : par->AsyncDepth;
}
static mfxStatus InitEncoderParameters(mfxVideoParam *par_in, mfxVideoParam *par_enc, eMFXHWType targetHw)
{
    mfxExtLAControl *pControl = (mfxExtLAControl *) GetExtBuffer(par_in->ExtParam, par_in->NumExtParam, MFX_EXTBUFF_LOOKAHEAD_CTRL);

    MFX_CHECK(pControl,MFX_ERR_UNDEFINED_BEHAVIOR);

    if (pControl->BPyramid == MFX_CODINGOPTION_ON)
    {
        par_enc->mfx.GopRefDist = ((par_in->mfx.GopRefDist >  8) || (par_in->mfx.GopRefDist == 0)) ? 8   : par_in->mfx.GopRefDist;
        par_enc->mfx.GopPicSize = (par_in->mfx.GopPicSize == 0) ? 1500   :  par_in->mfx.GopPicSize;
    }
    par_enc->AsyncDepth = GetAsyncDeph(par_in);
    par_enc->IOPattern  = par_in->IOPattern;
    par_enc->mfx.GopPicSize = GetGopSize(par_in);
    par_enc->mfx.GopRefDist = GetRefDist(par_in, targetHw);
    par_enc->mfx.RateControlMethod = MFX_RATECONTROL_LA;
    par_enc->mfx.NumRefFrame = 2;
    par_enc->mfx.FrameInfo = par_in->mfx.FrameInfo;
    par_enc->mfx.TargetKbps = 1000; // formal

    if (par_enc->IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY)
    {
        mfxExtOpaqueSurfaceAlloc *pOpaqIn  = (mfxExtOpaqueSurfaceAlloc *) GetExtBuffer(par_in->ExtParam, par_in->NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
        mfxExtOpaqueSurfaceAlloc *pOpaqEnc = (mfxExtOpaqueSurfaceAlloc *) GetExtBuffer(par_enc->ExtParam, par_enc->NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
        MFX_CHECK_NULL_PTR2(pOpaqIn, pOpaqEnc);
        pOpaqEnc->In.Type = pOpaqIn->In.Type;
        pOpaqEnc->In.NumSurface = 255; // formal
    }
    return MFX_ERR_NONE;
}
static bool bReference(mfxU16 frameType)
{
    return (!(frameType & MFX_FRAMETYPE_B));
}
static bool CheckExtenedBuffer(mfxVideoParam *par)
{
    mfxU32  BufferId[2] = {MFX_EXTBUFF_LOOKAHEAD_CTRL, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION};
    mfxU32  num = sizeof(BufferId)/sizeof(BufferId[0]);
    if (!par->ExtParam) return true;

    for (mfxU32 i = 0; i < par->NumExtParam; i++)
    {
        mfxU32 j = 0;
        for (; j < num; j++)
        {
            if (par->ExtParam[i] != 0 && par->ExtParam[i]->BufferId == BufferId[j]) break;
        }
        if (j == num) return false;
    }
    return true;
}
static
mfxStatus GetNativeSurface(
    VideoCORE & core,
    MfxHwH264Encode::MfxVideoParam const & video,
    mfxFrameSurface1 *  opaq_surface,
    mfxFrameSurface1 ** native_surface)
{
    mfxStatus sts = MFX_ERR_NONE;
   *native_surface = opaq_surface;

    if (video.IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY)
    {
        *native_surface = core.GetNativeSurface(opaq_surface);
        MFX_CHECK_NULL_PTR1(*native_surface);

        (*native_surface)->Info            = opaq_surface->Info;
        (*native_surface)->Data.TimeStamp  = opaq_surface->Data.TimeStamp;
        (*native_surface)->Data.FrameOrder = opaq_surface->Data.FrameOrder;
        (*native_surface)->Data.Corrupted  = opaq_surface->Data.Corrupted;
        (*native_surface)->Data.DataFlag   = opaq_surface->Data.DataFlag;
    }
    return sts;
}

static
mfxStatus GetOpaqSurface(
    VideoCORE & core,
    MfxHwH264Encode::MfxVideoParam const & video,
    mfxFrameSurface1 *native_surface,
    mfxFrameSurface1 ** opaq_surface)
{
    mfxStatus sts = MFX_ERR_NONE;

   *opaq_surface = native_surface;

    if (video.IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY)
    {
        *opaq_surface = core.GetOpaqSurface(native_surface->Data.MemId);
        MFX_CHECK_NULL_PTR1(*opaq_surface);

        (*opaq_surface)->Info            = native_surface->Info;
        (*opaq_surface)->Data.TimeStamp  = native_surface->Data.TimeStamp;
        (*opaq_surface)->Data.FrameOrder = native_surface->Data.FrameOrder;
        (*opaq_surface)->Data.Corrupted  = native_surface->Data.Corrupted;
        (*opaq_surface)->Data.DataFlag   = native_surface->Data.DataFlag;
    }
    return sts;
}
bool bEnc_LA(mfxVideoParam *par)
{
    mfxExtLAControl *pControl = (mfxExtLAControl *) GetExtBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_LOOKAHEAD_CTRL);
    return (pControl != 0);
}

mfxStatus VideoENC_LA::Query(VideoCORE* pCore, mfxVideoParam *in, mfxVideoParam *out)
{
    MFX_CHECK_NULL_PTR2(pCore,out);
    if (in == 0)
    {
        MFX_CHECK(out->NumExtParam > 0, MFX_ERR_UNDEFINED_BEHAVIOR);
        mfxExtLAControl *pControl = (mfxExtLAControl *) GetExtBuffer(out->ExtParam, out->NumExtParam, MFX_EXTBUFF_LOOKAHEAD_CTRL);
        MFX_CHECK_NULL_PTR1(pControl);
        out->AsyncDepth = 1;
        out->IOPattern = 1;
        out->Protected = 0;
        memset (&out->mfx,0,sizeof(out->mfx));
        out->mfx.CodecId = 1;
        out->mfx.GopPicSize = 1;
        out->mfx.GopRefDist = 1;
        out->mfx.FrameInfo.Width = 1;
        out->mfx.FrameInfo.Height = 1;


        pControl->DependencyDepth = 1;
        pControl->DownScaleFactor = 1;
        pControl->LookAheadDepth= 1;
        pControl->NumOutStream= 1;
        pControl->BPyramid = 1;
    }
    else
    {
        bool bChanged = false;

        MFX_CHECK(out->NumExtParam > 0, MFX_ERR_UNDEFINED_BEHAVIOR);
        MFX_CHECK(in->NumExtParam  > 0, MFX_ERR_UNDEFINED_BEHAVIOR);

        mfxExtLAControl *pControl_in = (mfxExtLAControl *) GetExtBuffer(in->ExtParam, in->NumExtParam, MFX_EXTBUFF_LOOKAHEAD_CTRL);
        mfxExtLAControl *pControl_out = (mfxExtLAControl *) GetExtBuffer(out->ExtParam, out->NumExtParam, MFX_EXTBUFF_LOOKAHEAD_CTRL);

        MFX_CHECK_NULL_PTR1(pControl_in);
        MFX_CHECK_NULL_PTR1(pControl_out);

        out->AsyncDepth = in->AsyncDepth;
        out->IOPattern  = in->IOPattern;
        out->Protected = 0;

       mfxU32 inPattern = out->IOPattern & MfxHwH264Encode::MFX_IOPATTERN_IN_MASK;
       MFX_CHECK(
            inPattern == MFX_IOPATTERN_IN_SYSTEM_MEMORY ||
            inPattern == MFX_IOPATTERN_IN_VIDEO_MEMORY ||
            inPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY,
            MFX_ERR_INVALID_VIDEO_PARAM);

       out->mfx.CodecId = in->mfx.CodecId ;
       out->mfx.GopPicSize = in->mfx.GopPicSize;
       out->mfx.GopRefDist = in->mfx.GopRefDist;
       out->mfx.FrameInfo.Width = in->mfx.FrameInfo.Width;
       out->mfx.FrameInfo.Height = in->mfx.FrameInfo.Height;
       out->mfx.TargetUsage = in->mfx.TargetUsage;

       if (pControl_in->BPyramid == MFX_CODINGOPTION_ON && out->mfx.GopRefDist !=0 && out->mfx.GopRefDist != 8)
       {
           out->mfx.GopRefDist = 0;
           bChanged = true;
       }
       if (pControl_in->BPyramid == MFX_CODINGOPTION_ON && out->mfx.GopPicSize &&  ((out->mfx.GopPicSize - 1) % 8) )
       {
           out->mfx.GopPicSize = 0;
           bChanged = true;
       }

       if (out->mfx.GopPicSize && out->mfx.GopRefDist > out->mfx.GopPicSize)
       {
           out->mfx.GopRefDist = 0;
           bChanged = true;
       }
       MFX_CHECK ((out->mfx.FrameInfo.Width & 0x03) == 0 && (out->mfx.FrameInfo.Height & 0x03) == 0, MFX_ERR_INVALID_VIDEO_PARAM);
       MFX_CHECK (pControl_in->NumOutStream == pControl_out->NumOutStream, MFX_ERR_INVALID_VIDEO_PARAM);


       pControl_out->DependencyDepth = pControl_in->DependencyDepth;
       pControl_out->DownScaleFactor = pControl_in->DownScaleFactor;
       pControl_out->LookAheadDepth  = pControl_in->LookAheadDepth;
       pControl_out->BPyramid = pControl_in->BPyramid;

       switch (pControl_out->DownScaleFactor)
       {
       case 0:
       case 1:
       case 2:
       case 4:
           break;
       default:
           pControl_out->DownScaleFactor = 0;
           break;
       }
       if (pControl_out->LookAheadDepth && (pControl_out->LookAheadDepth < pControl_out->DependencyDepth))
       {
           pControl_out->DependencyDepth = 0;
           bChanged = true;
       }
       std::copy(pControl_in->OutStream, pControl_in->OutStream + pControl_in->NumOutStream, pControl_out->OutStream);

       if (bChanged) return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;

    }
    return MFX_ERR_NONE;
}

mfxStatus VideoENC_LA::QueryIOSurf(VideoCORE* core, mfxVideoParam *par, mfxFrameAllocRequest *request)
{
    MFX_CHECK_NULL_PTR3(par,request,core);

    mfxExtLAControl *pControl = (mfxExtLAControl *) GetExtBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_LOOKAHEAD_CTRL);

    MFX_CHECK(pControl,MFX_ERR_UNDEFINED_BEHAVIOR);
    MFX_CHECK(pControl->LookAheadDepth,MFX_ERR_UNDEFINED_BEHAVIOR);

    mfxU32 inPattern = par->IOPattern & MfxHwH264Encode::MFX_IOPATTERN_IN_MASK;
    MFX_CHECK(
        inPattern == MFX_IOPATTERN_IN_SYSTEM_MEMORY ||
        inPattern == MFX_IOPATTERN_IN_VIDEO_MEMORY ||
        inPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY,
        MFX_ERR_INVALID_VIDEO_PARAM);

    if (inPattern == MFX_IOPATTERN_IN_SYSTEM_MEMORY)
    {
        request->Type =
            MFX_MEMTYPE_EXTERNAL_FRAME |
            MFX_MEMTYPE_FROM_ENCODE |
            MFX_MEMTYPE_SYSTEM_MEMORY;
    }
    else // MFX_IOPATTERN_IN_VIDEO_MEMORY || MFX_IOPATTERN_IN_OPAQUE_MEMORY
    {
        request->Type = MFX_MEMTYPE_FROM_ENCODE | MFX_MEMTYPE_DXVA2_DECODER_TARGET;
        request->Type |= (inPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY)
            ? MFX_MEMTYPE_OPAQUE_FRAME
            : MFX_MEMTYPE_EXTERNAL_FRAME;
    }
    request->NumFrameMin         = GetRefDist(par, core->GetHWType()) + GetAsyncDeph(par) + pControl->LookAheadDepth;
    request->NumFrameSuggested   = request->NumFrameMin;
    request->Info                = par->mfx.FrameInfo;

    return MFX_ERR_NONE;
}


VideoENC_LA::VideoENC_LA(VideoCORE *core,  mfxStatus * sts)
    : m_bInit( false )
    , m_core( core )
    , m_numLastB(0)
{
    memset(&m_LaControl, 0, sizeof(m_LaControl));
    memset(&m_Request, 0, sizeof(m_Request));
    memset(&m_LASyncContext,0,sizeof(m_LASyncContext));
    memset(&m_LAAsyncContext,0,sizeof(m_LAAsyncContext));


    *sts = MFX_ERR_NONE;
}


VideoENC_LA::~VideoENC_LA()
{
    Close();

}
static mfxU16 CheckScaleFactor(mfxU16 scale, mfxU16 tu, bool b4k)
{
    if (((scale == 1) && (!b4k)) || (scale == 2) || (scale == 4))
        return scale;
    if (scale == 1 && b4k)
        return 2;

    switch (tu)
    {
    case 1:
        return b4k ? 2 : 1;
    case 6:
    case 7:
        return 4;
    default:
        return 2;
    }
}
static mfxU16 CheckLookAheadDependency(mfxU16 dep, mfxU16 /*tu*/)
{
    if (dep != 0) return dep;
    return 10;
}
static void InitHWCaps(MFX_ENCODE_CAPS &hwCaps)
{
    memset (&hwCaps, 0, sizeof(MFX_ENCODE_CAPS));
    hwCaps.ddi_caps.CodingLimitSet=1;
    hwCaps.ddi_caps.NoFieldFrame=1;
    hwCaps.ddi_caps.NoGapInFrameCount=1;
    hwCaps.ddi_caps.NoGapInPicOrder=1;
    hwCaps.ddi_caps.NoUnpairedField=1;
    hwCaps.ddi_caps.BitDepth8Only=1;
    hwCaps.ddi_caps.ConsecutiveMBs=1;
    hwCaps.ddi_caps.SliceStructure=4;
    hwCaps.ddi_caps.NoWeightedPred=1;
    hwCaps.ddi_caps.ClosedGopOnly=1;
    hwCaps.ddi_caps.NoFrameCropping=1;
    hwCaps.ddi_caps.FrameLevelRateCtrl=1;
    hwCaps.ddi_caps.RawReconRefToggle=1;
    hwCaps.ddi_caps.BRCReset=1;
    hwCaps.ddi_caps.RollingIntraRefresh=1;
    hwCaps.ddi_caps.UserMaxFrameSizeSupport = 1;
    hwCaps.ddi_caps.VCMBitrateControl=1;
    hwCaps.ddi_caps.NoESS=1;
    hwCaps.ddi_caps.Color420Only=1;
    hwCaps.ddi_caps.ICQBRCSupport=1;
    hwCaps.ddi_caps.EncFunc=1;
    hwCaps.ddi_caps.EncodeFunc=1;

    hwCaps.ddi_caps.MaxPicWidth=4096;
    hwCaps.ddi_caps.MaxPicHeight=4096;
    hwCaps.ddi_caps.MaxNum_Reference=8;
    hwCaps.ddi_caps.MaxNum_SPS_id=32;
    hwCaps.ddi_caps.MaxNum_PPS_id=255 ;
    hwCaps.ddi_caps.MaxNum_Reference1=2 ;
    hwCaps.ddi_caps.MBBRCSupport=1;
    hwCaps.ddi_caps.MaxNumOfROI=4 ;

    hwCaps.ddi_caps.SkipFrame=1;
    hwCaps.ddi_caps.MbQpDataSupport=1;
    hwCaps.ddi_caps.QVBRBRCSupport=1;

    hwCaps.CQPSupport=1;
    hwCaps.VBRSupport=1;
    hwCaps.CBRSupport=1;
}

mfxStatus VideoENC_LA::Init(mfxVideoParam *par)
{
    mfxVideoParam            par_enc = {};
    mfxExtCodingOptionDDI    ddi_opt = { {MFX_EXTBUFF_DDI, 0}};
    mfxExtOpaqueSurfaceAlloc opaq_opt = { {MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION, 0}};
    mfxExtBuffer*           pExtParam[2] = {(mfxExtBuffer*)&ddi_opt, (mfxExtBuffer*)&opaq_opt};
    mfxStatus sts = MFX_ERR_NONE;
    bool bPyramid = false;

    par_enc.ExtParam = pExtParam;
    par_enc.NumExtParam = sizeof(pExtParam)/sizeof(*pExtParam);


    MFX_CHECK_NULL_PTR1( par );
    MFX_CHECK( m_bInit == false, MFX_ERR_UNDEFINED_BEHAVIOR);

    auto pControl = (mfxExtLAControl *) GetExtBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_LOOKAHEAD_CTRL);
    MFX_CHECK_NULL_PTR1(pControl);

    m_LaControl = *pControl;
    bPyramid = (m_LaControl.BPyramid == MFX_CODINGOPTION_ON);

    MFX_CHECK_STS (QueryIOSurf(m_core, par, m_Request));
    memset(&m_LASyncContext, 0,sizeof(m_LASyncContext));
    memset(&m_LAAsyncContext,0,sizeof(m_LAAsyncContext));


    MFX_CHECK(MAX_RESOLUTIONS >= pControl->NumOutStream, MFX_ERR_INVALID_VIDEO_PARAM);
    MFX_CHECK((sizeof(pControl->OutStream)/sizeof(pControl->OutStream[0])) >= pControl->NumOutStream, MFX_ERR_INVALID_VIDEO_PARAM);
    MFX_CHECK(m_LaControl.LookAheadDepth >0 , MFX_ERR_INVALID_VIDEO_PARAM);
    MFX_CHECK(m_LaControl.DownScaleFactor == 0 ||m_LaControl.DownScaleFactor == 1 || m_LaControl.DownScaleFactor == 2 || m_LaControl.DownScaleFactor == 4, MFX_ERR_INVALID_VIDEO_PARAM);
    MFX_CHECK(m_LaControl.LookAheadDepth > m_LaControl.DependencyDepth, MFX_ERR_INVALID_VIDEO_PARAM);



    MFX_CHECK(CheckExtenedBuffer(par), MFX_ERR_UNDEFINED_BEHAVIOR);

    m_syncTasks.clear();
    m_VMETasks.clear();
    m_OutputTasks.clear();
    m_SurfacesForOutput.clear();
    m_miniGop.clear();
    m_numLastB = 0;

    {
        ddi_opt.LaScaleFactor = m_LaControl.DownScaleFactor = CheckScaleFactor(m_LaControl.DownScaleFactor, par->mfx.TargetUsage, par->mfx.FrameInfo.Width > 4000);
        ddi_opt.LookAheadDependency =  m_LaControl.DependencyDepth = CheckLookAheadDependency(m_LaControl.DependencyDepth,par->mfx.TargetUsage);

        MFX_CHECK_STS (InitEncoderParameters(par, &par_enc, m_core->GetHWType()));
        m_video = par_enc;

         MFX_ENCODE_CAPS hwCaps = {};
        //MfxHwH264Encode::QueryHwCaps(m_core, hwCaps, MSDK_Private_Guid_Encode_AVC_Query);
        InitHWCaps(hwCaps);
        sts = MfxHwH264Encode::CheckVideoParam(m_video, hwCaps, m_core->IsExternalFrameAllocator(), m_core->GetHWType());
        MFX_CHECK(sts >=0, sts);
    }

    m_cmDevice.Reset(MfxHwH264Encode::TryCreateCmDevicePtr(m_core));
    if (m_cmDevice == NULL)
        return MFX_ERR_UNSUPPORTED;
    m_cmCtx.reset(new CmContextLA(m_video, m_cmDevice, m_core));


    mfxU32 numMb = m_video.calcParam.widthLa * m_video.calcParam.heightLa / 256;

    m_VMERefList.Init(m_video.mfx.NumRefFrame, m_video.AsyncDepth > 1 ? 1 : 0, bPyramid);

    m_vmeDataStorage.resize (m_LaControl.DependencyDepth + m_video.AsyncDepth + m_video.mfx.NumRefFrame  + (bPyramid ? 2 : 0) + 1);
    for (size_t i = 0; i < m_vmeDataStorage.size(); i++)
        m_vmeDataStorage[i].mb.resize(numMb);


    mfxFrameAllocRequest request = { };
    request.Info = m_video.mfx.FrameInfo;

    if (m_video.IOPattern == MFX_IOPATTERN_IN_SYSTEM_MEMORY)
    {
        request.Type        = MfxHwH264Encode::MFX_MEMTYPE_D3D_INT;
        request.NumFrameMin = mfxU16(m_video.mfx.NumRefFrame + m_video.AsyncDepth + (bPyramid ? 2 : 0));

        sts = m_raw.Alloc(m_core, request, true);
        MFX_CHECK_STS(sts);
    }
    else if (m_video.IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY)
    {
        auto extOpaq = (mfxExtOpaqueSurfaceAlloc *)GetExtBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
        MFX_CHECK_NULL_PTR1(extOpaq);

        request.Type        = extOpaq->In.Type;
        request.NumFrameMin = extOpaq->In.NumSurface;

        sts = m_opaqResponse.Alloc(m_core, request, extOpaq->In.Surfaces, extOpaq->In.NumSurface);
        MFX_CHECK_STS(sts);

        if (extOpaq->In.Type & MFX_MEMTYPE_SYSTEM_MEMORY)
        {
            request.Type        = MfxHwH264Encode::MFX_MEMTYPE_D3D_INT;
            request.NumFrameMin = extOpaq->In.NumSurface;
            sts = m_raw.Alloc(m_core, request, true);
        }
    }

    request.Info.Width  = m_video.calcParam.widthLa / 16 * sizeof(MfxHwH264Encode::LAOutObject);
    request.Info.Height = m_video.calcParam.widthLa/ 16;
    request.Info.FourCC = MFX_FOURCC_P8;
    request.Type        = MfxHwH264Encode::MFX_MEMTYPE_D3D_INT;
    request.NumFrameMin = mfxU16(m_video.mfx.NumRefFrame + m_video.AsyncDepth + (bPyramid ? 2 : 0) + 1);

    sts = m_mb.AllocCmBuffersUp(m_cmDevice, request);
    MFX_CHECK_STS(sts);

    request.Info.Width  = sizeof(MfxHwH264Encode::CURBEData);
    request.Info.Height = 1;
    request.Info.FourCC = MFX_FOURCC_P8;
    request.Type        = MfxHwH264Encode::MFX_MEMTYPE_D3D_INT;
    request.NumFrameMin = 1 + m_video.AsyncDepth;

    sts = m_curbe.AllocCmBuffers(m_cmDevice, request);
    MFX_CHECK_STS(sts);

    if (m_LaControl.DownScaleFactor >1)
    {
        request.Info.Width  = m_video.calcParam.widthLa;
        request.Info.Height = m_video.calcParam.heightLa;

        request.Info.FourCC = MFX_FOURCC_NV12;
        request.Type        = MfxHwH264Encode::MFX_MEMTYPE_D3D_INT;
        request.NumFrameMin = mfxU16(m_video.mfx.NumRefFrame + m_video.AsyncDepth + (bPyramid ? 2 : 0) + 1);

        sts = m_rawLa.AllocCmSurfaces(m_cmDevice, request);
        MFX_CHECK_STS(sts);
    }
    m_bInit = true;
    return MFX_ERR_NONE;

}
mfxStatus VideoENC_LA::Reset(mfxVideoParam *par)
{
    Close();
    return Init(par);
}


mfxStatus VideoENC_LA::GetVideoParam(mfxVideoParam *par)
{
    MFX_CHECK(CheckExtenedBuffer(par), MFX_ERR_UNDEFINED_BEHAVIOR);
    return MFX_ERR_NONE;
}

static mfxU16  GetFrameType(mfxU32 frameOrder, mfxU32 gopSize, mfxU32 gopRefDist, mfxU32 idrDistance)
{
    if (frameOrder % idrDistance == 0)
    {
        return MFX_FRAMETYPE_I|MFX_FRAMETYPE_REF|MFX_FRAMETYPE_IDR;
    }
    else if (frameOrder % gopSize == 0 )
    {
        return MFX_FRAMETYPE_I|MFX_FRAMETYPE_REF;
    }
    else if (((frameOrder % gopSize) % gopRefDist == 0) || ((frameOrder % idrDistance)== (idrDistance - 1)))
    {
        return MFX_FRAMETYPE_P|MFX_FRAMETYPE_REF;
    }
    return MFX_FRAMETYPE_B;
}

mfxStatus VideoENC_LA::InsertTaskWithReordenig(sLAInputTask &newTask, bool bEndOfSequence)
{
    if (!bEndOfSequence)
    {
        if (m_miniGop.size() == 0)
        {
            m_syncTasks.push_back(newTask);
        }
        m_miniGop.push_back(newTask);
    }
    else if (m_miniGop.size() >1)
    {
         size_t last_index = m_miniGop.size()  - 1;
         if (!bReference(m_miniGop[last_index].InputFrame.frameType))
            m_miniGop[last_index].InputFrame.frameType = MFX_FRAMETYPE_P|MFX_FRAMETYPE_REF;
    }

    if (m_miniGop.size()> 1 && bReference(m_miniGop[m_miniGop.size()  - 1].InputFrame.frameType))
    {
        size_t last_index = m_miniGop.size() - 1;
        MFX_CHECK(!(m_miniGop[last_index].InputFrame.frameType & MFX_FRAMETYPE_B), MFX_ERR_UNDEFINED_BEHAVIOR);

        m_miniGop[last_index].InputFrame.encFrameOrder = m_miniGop[0].InputFrame.encFrameOrder + m_numLastB + 1;
        if (m_miniGop[last_index].InputFrame.frameType & MFX_FRAMETYPE_P)
        {
            m_miniGop[last_index].RefFrame[0] = m_miniGop[0].InputFrame;
            MFX_CHECK_STS (m_core->IncreaseReference(&m_miniGop[last_index].RefFrame[0].pFrame->Data));
        }
        m_syncTasks.push_back(m_miniGop[last_index]);

        for (mfxU32 i = 1; i < last_index; i++)
        {
            MFX_CHECK((m_miniGop[i].InputFrame.frameType & MFX_FRAMETYPE_B), MFX_ERR_UNDEFINED_BEHAVIOR);

            m_miniGop[i].InputFrame.encFrameOrder = m_miniGop[last_index].InputFrame.encFrameOrder + i;

            m_miniGop[i].RefFrame[0] = m_miniGop[0].InputFrame;
            MFX_CHECK_STS (m_core->IncreaseReference(&m_miniGop[i].RefFrame[0].pFrame->Data));

            m_miniGop[i].RefFrame[1] = m_miniGop[last_index].InputFrame;
            MFX_CHECK_STS (m_core->IncreaseReference(&m_miniGop[i].RefFrame[1].pFrame->Data));

            m_syncTasks.push_back(m_miniGop[i]);
        }
        m_numLastB = m_miniGop[last_index].InputFrame.dispFrameOrder - m_miniGop[0].InputFrame.dispFrameOrder - 1;
        m_miniGop[0] = m_miniGop[last_index];
        m_miniGop.resize(1);
    }

    if (bEndOfSequence)
    {
        m_numLastB = 0;
        m_miniGop.resize(0);
        sLAInputTask finished_task {};
        m_syncTasks.push_back(finished_task);
    }


    return MFX_ERR_NONE;
}

mfxStatus VideoENC_LA::InsertTaskWithReordenigBPyr(sLAInputTask &newTask, bool bEndOfSequence)
{
    size_t last_index = 0;
    if (!bEndOfSequence)
    {
        if (m_miniGop.size() == 0)
        {
            m_syncTasks.push_back(newTask);
        }
        m_miniGop.push_back(newTask);
        last_index = m_miniGop.size()  - 1;
    }
    else if (m_miniGop.size() >1)
    {
        last_index = m_miniGop.size()  - 1;
        if (!bReference(m_miniGop[last_index].InputFrame.frameType))
            m_miniGop[last_index].InputFrame.frameType = MFX_FRAMETYPE_P|MFX_FRAMETYPE_REF;
    }

    if (m_miniGop.size() >1 && bReference(m_miniGop[last_index].InputFrame.frameType ))
    {
        MFX_CHECK(m_miniGop.size() <= 9, MFX_ERR_UNDEFINED_BEHAVIOR);
        last_index = m_miniGop.size()  - 1;
        mfxU32 numB = (mfxU32) (last_index - 1);

        static mfxU32 frame_num[8][9] = {{0,1},
                                         {0,2,1},
                                         {0,3,1,2},
                                         {0,4,2,1,3},
                                         {0,5,2,1,3,4},
                                         {0,6,3,1,2,4,5},
                                         {0,7,3,1,2,5,4,6},
                                         {0,8,4,2,1,3,6,5,7}};

        static mfxU32 ref_forw_num[8][9] = {{0,0},
                                            {0,0,0},
                                            {0,0,1,0},
                                            {0,0,0,2,0},
                                            {0,0,0,2,3,0},
                                            {0,0,1,0,3,4,0},
                                            {0,0,1,0,3,3,5,0},
                                            {0,0,0,2,0,4,4,6,0}};

        static mfxU32 ref_backw_num[8][9] = {{0,0},
                                            {0,2,2},
                                            {0,3,3,3},
                                            {0,2,4,4,4},
                                            {0,2,5,5,5,5},
                                            {0,3,3,6,6,6,6},
                                            {0,3,3,7,5,7,7,7},
                                            {0,2,4,4,8,6,8,8,8}};

        static mfxU32 ref_frame[8][9] =     {{1,1},
                                            {1,0,1},
                                            {1,1,0,1},
                                            {1,0,1,0,1},
                                            {1,0,1,1,0,1},
                                            {1,1,0,1,1,0,1},
                                            {1,1,0,1,0,1,0,1},
                                            {1,0,1,0,1,0,1,0,1}};

        static mfxU16 layers[8][9] =        {{0,0},
                                            {0,1,0},
                                            {0,1,2,0},
                                            {0,2,1,2,0},
                                            {0,2,1,2,3,0},
                                            {0,2,3,1,2,3,0},
                                            {0,2,3,1,3,2,3,0},
                                            {0,3,2,3,1,3,2,3,0}};


        for (mfxU32 i = 1; i <= last_index; i++)
        {
            if (ref_frame[numB][i])
                m_miniGop[i].InputFrame.frameType |= MFX_FRAMETYPE_REF;
            m_miniGop[i].InputFrame.layer = layers[numB][i];
            m_miniGop[frame_num[numB][i]].InputFrame.encFrameOrder = m_miniGop[frame_num[numB][i-1]].InputFrame.encFrameOrder + 1 + ((i == 1) ? m_numLastB : 0);
        }
        for (mfxU32 i = 1; i <= last_index; i++)
        {
            if (!(m_miniGop[i].InputFrame.frameType & MFX_FRAMETYPE_I))
            {
                m_miniGop[i].RefFrame[0] =m_miniGop[ref_forw_num[numB][i]].InputFrame;
                MFX_CHECK_STS (m_core->IncreaseReference(&m_miniGop[i].RefFrame[0].pFrame->Data));
            }
            if (m_miniGop[i].InputFrame.frameType & MFX_FRAMETYPE_B)
            {
                m_miniGop[i].RefFrame[1] = m_miniGop[ref_backw_num[numB][i]].InputFrame;
                MFX_CHECK_STS (m_core->IncreaseReference(&m_miniGop[i].RefFrame[1].pFrame->Data));
            }
        }
        for (mfxU32 i = 1; i <= last_index; i++)
        {
            m_syncTasks.push_back(m_miniGop[frame_num[numB][i]]);
        }
        m_numLastB = m_miniGop[last_index ].InputFrame.dispFrameOrder - m_miniGop[0].InputFrame.dispFrameOrder-1;
        m_miniGop[0] = m_miniGop[last_index ];
        m_miniGop.resize(1);
    }
    if (bEndOfSequence)
    {
        m_numLastB = 0;
        m_miniGop.resize(0);
        sLAInputTask finished_task {};
        m_syncTasks.push_back(finished_task);
    }
    return MFX_ERR_NONE;
}

mfxFrameSurface1 * VideoENC_LA::GetFrameToVME()
{
    if (m_syncTasks.size() == 0)
        return 0;

    std::list<sLAInputTask>::iterator it = m_syncTasks.begin();
    while (it != m_syncTasks.end() && (*it).bFilledForVME)  ++it;
    if ((it != m_syncTasks.end()) && (*it).InputFrame.pFrame)
    {
        (*it).bFilledForVME = 1;
        return (*it).InputFrame.pFrame;
    }
    else
        return 0;
}

//SubmitFrameLARoutine and QueryFrameLARoutine may be needed if we use a double entry point
#if 0
static mfxStatus SubmitFrameLARoutine(void *pState, void * pParam, mfxU32 /*threadNumber*/, mfxU32 /*callNumber*/)
{
    mfxStatus tskRes;

    VideoENC_LA *pLa = (VideoENC_LA *)pState;
    sAsyncParams *out = (sAsyncParams *)pParam;

    //printf("Start Run FrameVPP %x %x %x\n", pAsyncParams->surf_in, pAsyncParams->surf_out, pAsyncParams->aux);
    tskRes = pLa->SubmitFrameLA(out->reordered_surface);
    //printf("Run FrameVPP %x %x %x %d\n", pAsyncParams->surf_in, pAsyncParams->surf_out, pAsyncParams->aux, tskRes);

    return tskRes;

} // mfxStatus RunFrameVPPRoutine(void *pState, void *pParam, mfxU32 threadNumber, mfxU32 callNumber)

static mfxStatus QueryFrameLARoutine(void *pState, void *pParam, mfxU32 /*threadNumber*/, mfxU32 /*callNumber*/)
{
    mfxStatus tskRes;

    VideoENC_LA *pLa = (VideoENC_LA *)pState;
    sAsyncParams *out = (sAsyncParams *)pParam;

    //printf("Start Run FrameVPP %d %d\n", out && out->reordered_surface?  out->reordered_surface->Data.FrameOrder: 0, out && out->output_surface?  out->output_surface->Data.FrameOrder: 0);
    tskRes = pLa->QueryFrameLA(out->reordered_surface,out->stat);
    //printf("Run FrameVPP %d %d %d\n", out && out->reordered_surface?  out->reordered_surface->Data.FrameOrder: 0, out && out->output_surface?  out->output_surface->Data.FrameOrder: 0, tskRes);

    return tskRes;

} // mfxStatus RunFrameVPPRoutine(void *pState, void *pParam, mfxU32 threadNumber, mfxU32 callNumber)
#endif

static mfxStatus RunFrameVPPRoutine(void *pState, void *pParam, mfxU32 /*threadNumber*/, mfxU32 /*callNumber*/)
{
    mfxStatus sts = MFX_ERR_NONE;

    VideoENC_LA *pLa = (VideoENC_LA *)pState;
    sAsyncParams *out = (sAsyncParams *)pParam;

    //printf("Start Run FrameVPP %x %x %x\n", pAsyncParams->surf_in, pAsyncParams->surf_out, pAsyncParams->aux);

    if (!out->bFrameLASubmitted)
    {
        sts = pLa->SubmitFrameLA(out->reordered_surface);
        if (sts != MFX_TASK_BUSY)
            out->bFrameLASubmitted = true;
        MFX_CHECK(sts == MFX_ERR_NONE || sts == MFX_TASK_BUSY, sts);
    }

    return pLa->QueryFrameLA(out->reordered_surface, out->stat);

} // mfxStatus RunFrameVPPRoutine(void *pState, void *pParam, mfxU32 threadNumber, mfxU32 callNumber)
mfxStatus VideoENC_LA::ResetTaskCounters()
{
    // TO DO
    return MFX_TASK_DONE;
}


static mfxStatus CompleteFrameLARoutine(void *pState, void * pParam , mfxStatus /* taskRes */)
{
    VideoENC_LA *pLa = (VideoENC_LA *)pState;
    if (pParam)
    {
        delete (sAsyncParams *)pParam;
    }
    return pLa->ResetTaskCounters();
}

void* VideoENC_LA::GetDstForSync(MFX_ENTRY_POINT& pEntryPoint)
{
    sAsyncParams *pPar = (sAsyncParams *) pEntryPoint.pParam;
    return pPar ? pPar->output_surface : 0;
}
void* VideoENC_LA::GetSrcForSync(MFX_ENTRY_POINT& pEntryPoint)
{
    sAsyncParams *pPar = (sAsyncParams *) pEntryPoint.pParam;
    return pPar ? pPar->reordered_surface : 0;
}


mfxStatus VideoENC_LA::RunFrameVmeENCCheck(
                    mfxENCInput *input,
                    mfxENCOutput *output,
                    MFX_ENTRY_POINT pEntryPoints[],
                    mfxU32 &numEntryPoints)
{
    MFX_CHECK( m_bInit, MFX_ERR_UNDEFINED_BEHAVIOR);
    UMC::AutomaticUMCMutex guard(m_listMutex);
    mfxStatus sts = MFX_ERR_NONE;

    MFX_CHECK_NULL_PTR1(output);

    mfxFrameSurface1 *in = input ? input->InSurface:0;
    mfxExtLAFrameStatistics *aux = (mfxExtLAFrameStatistics *) GetExtBuffer(output->ExtParam, output->NumExtParam, MFX_EXTBUFF_LOOKAHEAD_STAT);
    MFX_CHECK_NULL_PTR1(aux);

    aux->OutSurface = 0;

    if (in)
    {
        sLAInputTask  newTask = {};

        m_core->IncreaseReference(&in->Data);
        newTask.InputFrame.dispFrameOrder = m_LASyncContext.numInput ++;
        //printf("Input frame %d\n", newTask.InputFrame.dispFrameOrder);
        MFX_CHECK_STS(GetNativeSurface(*m_core, m_video,in, &newTask.InputFrame.pFrame));
        newTask.InputFrame.frameType = GetFrameType(newTask.InputFrame.dispFrameOrder, m_video.mfx.GopPicSize, m_video.mfx.GopRefDist,m_video.mfx.GopPicSize*(m_video.mfx.IdrInterval + 1));

        m_LASyncContext.pocInput = (newTask.InputFrame.frameType & MFX_FRAMETYPE_I) ? 0 : m_LASyncContext.pocInput + 2;
        newTask.InputFrame.poc = m_LASyncContext.pocInput;

        if (m_LaControl.BPyramid == MFX_CODINGOPTION_ON)
        {
             MFX_CHECK_STS(InsertTaskWithReordenigBPyr(newTask, false));
        }
        else
        {
            MFX_CHECK_STS(InsertTaskWithReordenig(newTask, false));   // simple reordenig
        }

        if (m_LASyncContext.numBuffered < (mfxU32)m_video.mfx.GopRefDist + m_LaControl.LookAheadDepth)
        {
            m_LASyncContext.numBuffered ++;
            sts = (mfxStatus)MFX_ERR_MORE_DATA_SUBMIT_TASK;
            if (m_LASyncContext.numBufferedVME < m_video.mfx.GopRefDist)
            {
                m_LASyncContext.numBufferedVME ++;
                sts = MFX_ERR_MORE_DATA;
            }
        }
    }
    else
    {
        if (!m_LASyncContext.bLastFrameChecked)
        {
            sLAInputTask  newTask = {};
            if (m_LaControl.BPyramid == MFX_CODINGOPTION_ON)
            {
                 MFX_CHECK_STS(InsertTaskWithReordenigBPyr(newTask, true));
            }
            else
            {
                MFX_CHECK_STS(InsertTaskWithReordenig(newTask, true));   // simple reordenig
            }

            m_LASyncContext.bLastFrameChecked = true;
            //m_LASyncContext.numBufferedVME++;
        }
        sts = (m_LASyncContext.numBuffered--) > 0 ?  MFX_ERR_NONE : MFX_ERR_MORE_DATA;
    }
    if (MFX_ERR_NONE == sts ||
        MFX_ERR_MORE_DATA_SUBMIT_TASK == static_cast<int>(sts) ||
        MFX_ERR_MORE_SURFACE == sts)
    {
        mfxFrameSurface1* pReord = 0;
        sAsyncParams* pAsyncParams = new sAsyncParams;
        memset(pAsyncParams, 0, sizeof(sAsyncParams));
        pReord = GetFrameToVME();
        if (pReord)
        {
            m_SurfacesForOutput.push_back(pReord);
            mfxStatus ret = GetOpaqSurface(*m_core, m_video,pReord ,&pAsyncParams->reordered_surface);
            if (MFX_ERR_NONE != ret)
            {
                delete pAsyncParams;
                MFX_CHECK_STS(ret);
            }
        }
        else
        {
            pAsyncParams->reordered_surface = 0;
        }

        //printf("Check reordered %d\n",pAsyncParams->reordered_surface->Data.FrameOrder );

        if (MFX_ERR_NONE == sts)
        {
            if (m_SurfacesForOutput.size() > 0)
            {
                MFX_CHECK_STS(GetOpaqSurface(*m_core, m_video, m_SurfacesForOutput.front(), &pAsyncParams->output_surface));
                m_SurfacesForOutput.pop_front();
                aux->OutSurface = pAsyncParams->output_surface;
            }
            pAsyncParams->stat = output;
            //printf("Check reordered1\t %x, %d\n",aux, pAsyncParams->output_surface->Data.FrameOrder );
        }


        /*pEntryPoints[0].pRoutine = &SubmitFrameLARoutine;
        pEntryPoints[0].pCompleteProc =0;
        pEntryPoints[0].pState = this;
        pEntryPoints[0].requiredNumThreads = 1;
        pEntryPoints[0].pParam = pAsyncParams;

        pEntryPoints[1].pRoutine = &QueryFrameLARoutine;
        pEntryPoints[1].pCompleteProc = &CompleteFrameLARoutine;
        pEntryPoints[1].pState = this;
        pEntryPoints[1].requiredNumThreads = 1;
        pEntryPoints[1].pParam = pAsyncParams;

        numEntryPoints = 2;*/



        pEntryPoints[0].pRoutine = &RunFrameVPPRoutine;
        pEntryPoints[0].pCompleteProc = &CompleteFrameLARoutine;
        pEntryPoints[0].pState = this;
        pEntryPoints[0].requiredNumThreads = 1;
        pEntryPoints[0].pParam = pAsyncParams;

        numEntryPoints = 1;
    }
    return sts;
}

namespace MfxHwH264EncodeHW
{
    mfxU8 GetSkippedQp(MfxHwH264Encode::MbData const & mb);
    void DivideCost(std::vector<MfxHwH264Encode::MbData> & mb, mfxI32 width, mfxI32 height, mfxU32 cost, mfxI32 x, mfxI32 y);
};
using namespace MfxHwH264EncodeHW;

MfxHwH264Encode::VmeData * FindUnusedVmeData(std::vector<MfxHwH264Encode::VmeData> & vmeData)
{
    for (size_t i = 0; i < vmeData.size(); i++)
    {
        if (!vmeData[i].used)
            return &vmeData[i];
    }
    return 0;
}

static void PreEnc(MfxHwH264Encode::VmeData *VMEData, sSumVMEInfo& frameData, mfxU16  width_in,  mfxU16  height_in, mfxU16  width_out,  mfxU16  height_out)
{

    int w_in    = (width_in   + 15)>>4;
    int h_in    = (height_in  + 15)>>4;
    int w_out   = (width_out  + 15)>>4;
    int h_out   = (height_out + 15)>>4;
    float w_step = (float)w_in / ((float) w_out);
    float h_step = (float)h_in / ((float) h_out);

    memset(&frameData,0,sizeof(mfxLAFrameInfo));

    int i_start = 0;
    for (int i = 0; i < h_out; i++)
    {
        int j_start = 0;
        int i_last  = (int)((i + 1)*h_step);
        i_last = i_last < h_in ?  i_last : h_in;

        for (int j = 0; j < w_out; j++)
        {
            int j_last  = (int)((j + 1)*w_step);
            j_last = j_last < w_in ?  j_last : w_in;

            int inputMBNumber = i_start*w_in + j_start;

            if (i_start < i_last -1 || j_start < j_last -1) //area search is used
            {
                unsigned intra_cost = 0;

                for (int ii = i_start ; ii < i_last; ii++)
                {
                    for (int jj = j_start; jj < j_last; jj++)
                    {
                        int index = ii*w_in + jj;
                        const MfxHwH264Encode::MbData &in = VMEData->mb[index];
                        if (intra_cost <= in.intraCost)
                        {
                            intra_cost = in.intraCost;
                            inputMBNumber = index;
                        }
                    }
                }
            }

            const MfxHwH264Encode::MbData &inMB = VMEData->mb[inputMBNumber];

            frameData.InterCost += inMB.interCost;
            frameData.IntraCost += inMB.intraCost;
            frameData.DependencyCost  += inMB.propCost;

            if (inMB.intraMbFlag)
            {
               frameData.EstimatedRate[51] +=  inMB.dist;
            }
            else
            {
                mfxU32 skipQp = GetSkippedQp(inMB);
                if (skipQp > 0)
                    frameData.EstimatedRate[skipQp - 1] += (inMB.dist<<1);
            }
            j_start = j_last;
        }
        i_start = i_last;
    }
    for (int qp =  51; qp > 0; qp--)
    {
        frameData.EstimatedRate[qp - 1 ] += frameData.EstimatedRate[qp];
    }
}
static void PreEnc(MfxHwH264Encode::VmeData *VMEData, sSumVMEInfo& frameData, mfxF32 scale)
{

     for (mfxU32 i = 0; i < VMEData->mb.size();  i++)
     {
        MfxHwH264Encode::MbData* currMB = &VMEData->mb[i];
        frameData.InterCost         += currMB->interCost;
        frameData.IntraCost         += currMB->intraCost;
        frameData.DependencyCost    += currMB->propCost;

        if ((*currMB).intraMbFlag)
        {
            frameData.EstimatedRate[51] +=  currMB->dist;
        }
        else
        {
            mfxU32 skipQp = GetSkippedQp(*currMB);
            if (skipQp > 0)
                frameData.EstimatedRate[skipQp - 1] += (currMB->dist<<1);
        }
    }
    frameData.InterCost         = mfxU32(((mfxF32)frameData.InterCost) * scale);
    frameData.IntraCost         = mfxU32(((mfxF32)frameData.IntraCost)* scale);
    frameData.DependencyCost    = mfxU32(((mfxF32)frameData.DependencyCost)*scale);
    frameData.EstimatedRate[51] = mfxU64(((mfxF64)frameData.EstimatedRate[51])* scale);

    for (int qp =  51; qp > 0; qp--)
    {
        frameData.EstimatedRate[qp - 1 ] = mfxU64(((mfxF64)frameData.EstimatedRate[qp - 1 ])* scale);
        frameData.EstimatedRate[qp - 1 ] += frameData.EstimatedRate[qp];
    }

}

static void AnalyzeVmeData(MfxHwH264Encode::VmeData * cur, MfxHwH264Encode::VmeData * l0, MfxHwH264Encode::VmeData * l1, mfxU32 width, mfxU32 height)
{


    mfxI32 w = width  >> 4;
    mfxI32 h = height >> 4;

    for (mfxI32 y = 0; y < h; y++)
    {
        MfxHwH264Encode::MbData const * mb = &cur->mb[y * w];
        for (mfxI32 x = 0; x < w; x++, mb++)
        {
            if (!mb->intraMbFlag)
            {
                mfxU32 amount   = mb->intraCost - mb->interCost;
                mfxF64 frac     = amount / mfxF64(mb->intraCost);
                mfxU32 propCost = mfxU32(amount + mb->propCost * frac + 0.5);

                if (mb->mbType == MBTYPE_BP_L0_16x16)
                {
                    if (l0)
                        DivideCost(l0->mb, w, h, propCost,
                        (x << 4) + ((mb->mv[0].x + 2) >> 2),
                        (y << 4) + ((mb->mv[0].y + 2) >> 2));
                }
                else if (mb->mbType == MBTYPE_B_L1_16x16)
                {
                    if (l1)
                        DivideCost(l1->mb, w, h, propCost,
                        (x << 4) + ((mb->mv[1].x + 2) >> 2),
                        (y << 4) + ((mb->mv[1].y + 2) >> 2));
                }
                else if (mb->mbType == MBTYPE_B_Bi_16x16)
                {
                    if (l0)
                        DivideCost(l0->mb, w, h, (propCost * mb->w0 + 32) >> 6,
                        (x << 4) + ((mb->mv[0].x + 2) >> 2),
                        (y << 4) + ((mb->mv[0].y + 2) >> 2));
                    if (l1)
                        DivideCost(l1->mb, w, h, (propCost * mb->w1 + 32) >> 6,
                        (x << 4) + ((mb->mv[1].x + 2) >> 2),
                        (y << 4) + ((mb->mv[1].y + 2) >> 2));
                }
                else
                {
                    assert(!"invalid mb mode");
                }
            }
        }
    }

}
mfxStatus CopyRawSurfaceToVideoMemory(  VideoCORE &  core,
                                        MfxHwH264Encode::MfxVideoParam const & video,
                                        mfxFrameSurface1 *  src_sys,
                                        mfxMemId            dst_d3d,
                                        mfxHDL&             handle)
{
    mfxExtOpaqueSurfaceAlloc const * extOpaq = GetExtBuffer(video);

    mfxFrameData d3dSurf {};
    if (video.IOPattern == MFX_IOPATTERN_IN_SYSTEM_MEMORY ||
        (video.IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY && (extOpaq->In.Type & MFX_MEMTYPE_SYSTEM_MEMORY)))
    {
        mfxFrameData sysSurf = src_sys->Data;
        d3dSurf.MemId = dst_d3d;

        MfxHwH264Encode::FrameLocker lock2(&core, sysSurf, true);

        MFX_CHECK_NULL_PTR1(sysSurf.Y)
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_INTERNAL, "Copy input (sys->d3d)");
            MFX_CHECK_STS(MfxHwH264Encode::CopyFrameDataBothFields(&core, d3dSurf, sysSurf, video.mfx.FrameInfo));
        }

        MFX_CHECK_STS(lock2.Unlock());
    }
    else
    {
        d3dSurf.MemId =  src_sys->Data.MemId;
    }

    if (video.IOPattern != MFX_IOPATTERN_IN_OPAQUE_MEMORY && video.IOPattern != MFX_IOPATTERN_IN_SYSTEM_MEMORY)
       MFX_CHECK_STS(core.GetExternalFrameHDL(d3dSurf.MemId, &handle))
    else
       MFX_CHECK_STS(core.GetFrameHDL(d3dSurf.MemId, &handle));

    return MFX_ERR_NONE;
}

mfxStatus VideoENC_LA::RunFrameVmeENC(mfxENCInput * , mfxENCOutput *)
{
    mfxStatus sts = MFX_ERR_NONE;
    return sts;
}
mfxStatus VideoENC_LA::InitVMEData( sVMEFrameInfo *   pVME,
                                    mfxU32            EncOrder,
                                    mfxU32            DispOrder,
                                    mfxFrameSurface1* RawFrame,
                                    CmBufferUP *      CmMb)
{
    mfxMemId  dst_d3d = MfxHwH264Encode::AcquireResource(m_raw);
    mfxHDLPair  currHandle;
    currHandle.first = currHandle.second = 0;

    MFX_CHECK_STS(CopyRawSurfaceToVideoMemory(*m_core,m_video,RawFrame,dst_d3d, currHandle.first));

    pVME->EncOrder   = EncOrder;
    pVME->DispOrder = DispOrder;
    pVME->CmRaw     =  CreateSurface(m_cmDevice, currHandle.first, m_core->GetVAType());
    pVME->CmRawLA   = (CmSurface2D *)MfxHwH264Encode::AcquireResource(m_rawLa);
    pVME->VmeData   = FindUnusedVmeData(m_vmeDataStorage);
    pVME->CmMb      = CmMb;
    pVME->RawFrame =  dst_d3d;

    // find the oldest VME data
    if (!pVME->VmeData )
    {
        for (size_t i = 0; i < m_vmeDataStorage.size(); i++)
        {
            if (!pVME->VmeData  || pVME->VmeData->encOrder > m_vmeDataStorage[i].encOrder)
            {
                pVME->VmeData =  &(m_vmeDataStorage[i]);
            }
        }
    }
    MFX_CHECK(pVME->VmeData && (pVME->CmRawLA || m_LaControl.DownScaleFactor <2) && pVME->CmMb, MFX_ERR_NULL_PTR);

    return MFX_ERR_NONE;
}
mfxStatus VideoENC_LA::FreeUnusedVMEData(sVMEFrameInfo *pVME)
{
    pVME->EncOrder = 0;
    MfxHwH264Encode::ReleaseResource(m_mb, pVME->CmMb);
    if (m_cmDevice && pVME->CmRaw){
        m_cmDevice->DestroySurface(pVME->CmRaw);
        pVME->CmRaw = NULL;
    }
    ReleaseResource(m_rawLa, pVME->CmRawLA);
    ReleaseResource(m_raw, pVME->RawFrame);

    // VmeData  structure will be deleted later

    return MFX_ERR_NONE;
}
mfxStatus VideoENC_LA::SubmitFrameLA(mfxFrameSurface1 *pInSurface)
{
    mfxFrameSurface1*       inputSurface = nullptr;
    sLAInputTask            currTask {};

    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_INTERNAL, "LA::SubmitFrame");

    if (pInSurface)
    {
        MFX_CHECK_STS(GetNativeSurface(*m_core, m_video,pInSurface, &inputSurface));
    }

    {
        UMC::AutomaticUMCMutex guard(m_listMutex);
        MFX_CHECK(m_syncTasks.size() > 0, MFX_ERR_UNDEFINED_BEHAVIOR);
        currTask = m_syncTasks.front();
        MFX_CHECK(inputSurface == currTask.InputFrame.pFrame, MFX_ERR_UNDEFINED_BEHAVIOR);
    }
    //printf("VideoENC_LA::SubmitFrameLA %d, %d, %x,%x\n",pInSurface->Data.FrameOrder, currTask.InputFrame.pFrame->Data.FrameOrder, inputSurface,  currTask.InputFrame.pFrame);


    //submit LA DDI task
    if (inputSurface)
    {
        sLADdiTask              currDDILATask = { 0 };
        sLADdiTask*             task = &currDDILATask;

        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_INTERNAL, "LA::Submit DDI");

        task->m_TaskInfo = currTask;
        mfxHDLPair cmMb = MfxHwH264Encode::AcquireResourceUp(m_mb);

        MFX_CHECK_STS(InitVMEData(&task->m_Curr, currTask.InputFrame.encFrameOrder, currTask.InputFrame.dispFrameOrder, currTask.InputFrame.pFrame, (CmBufferUP *)cmMb.first));

        task->m_cmMbSys = (void *)cmMb.second;
        task->m_cmCurbe = (CmBuffer *)MfxHwH264Encode::AcquireResource(m_curbe);

        MFX_CHECK(task->m_cmCurbe && task->m_cmMbSys, MFX_ERR_NULL_PTR);

        if (currTask.InputFrame.frameType & MFX_FRAMETYPE_REF)
        {
            sVMEFrameInfo* p = m_VMERefList.GetFree();
            if (!p)
            {
                p = m_VMERefList.GetOldest();
                MFX_CHECK_NULL_PTR1(p);
                FreeUnusedVMEData(p);
            }
            MFX_CHECK_NULL_PTR1(p);
            *p = task->m_Curr;
            p->bUsed = true;
        }
        int fwd = !(currTask.InputFrame.frameType & MFX_FRAMETYPE_I);
        int bwd = (currTask.InputFrame.frameType & MFX_FRAMETYPE_B);

        if (fwd)
        {
            sVMEFrameInfo *p = m_VMERefList.GetByEncOrder(task->m_TaskInfo.RefFrame[REF_FORW].encFrameOrder);
            MFX_CHECK_NULL_PTR1(p);
            task->m_Ref[REF_FORW] = *p;
        }
        if (bwd)
        {
            sVMEFrameInfo *p = m_VMERefList.GetByEncOrder(task->m_TaskInfo.RefFrame[REF_BACKW].encFrameOrder);
            MFX_CHECK_NULL_PTR1(p);
            task->m_Ref[REF_BACKW] = *p;
        }


        task->m_cmRefs = CreateVmeSurfaceG75(m_cmDevice, task->m_Curr.CmRaw,
            fwd ? &task->m_Ref[REF_FORW].CmRaw : 0,
            bwd ? &task->m_Ref[REF_BACKW].CmRaw : 0, !!fwd, !!bwd);

        if (m_LaControl.DownScaleFactor > 1)
            task->m_cmRefsLa = CreateVmeSurfaceG75(m_cmDevice, task->m_Curr.CmRawLA,
                fwd ? &task->m_Ref[REF_FORW].CmRawLA : 0, bwd ? &task->m_Ref[REF_BACKW].CmRawLA : 0, !!fwd, !!bwd);
        if (!bwd)
        {
            task->m_Ref[REF_BACKW].CmMb = 0;
        }

        task->m_Curr.VmeData->poc = currTask.InputFrame.poc;
        task->m_Curr.VmeData->encOrder = currTask.InputFrame.encFrameOrder;
        task->m_Curr.VmeData->pocL0 = fwd ? currTask.RefFrame[REF_FORW].poc : 0xffffffff;
        task->m_Curr.VmeData->pocL1 = bwd ? currTask.RefFrame[REF_BACKW].poc : 0xffffffff;
        task->m_Curr.VmeData->used = true;


        task->m_event = m_cmCtx->RunVme(*task, 26);
        m_LAAsyncContext.numInputFrames++;
        {
            UMC::AutomaticUMCMutex guard(m_listMutex);
            m_syncTasks.pop_front();
            m_VMETasks.push_back(*task);
        }
    }
    return MFX_ERR_NONE;

} // mfxStatus VideoVPPSW::RunFrameVPP(



mfxStatus VideoENC_LA::QueryFrameLA(mfxFrameSurface1 *pInSurface, mfxENCOutput *output)
{
    mfxExtLAFrameStatistics *aux = output ? (mfxExtLAFrameStatistics *) GetExtBuffer(output->ExtParam, output->NumExtParam, MFX_EXTBUFF_LOOKAHEAD_STAT) : 0;
    mfxFrameSurface1 *out_t = aux ? aux->OutSurface: 0;
    mfxFrameSurface1 *out = 0;

    if (out_t)
    {
        mfxStatus sts = GetNativeSurface(*m_core, m_video,out_t, &out);
        if(sts != MFX_ERR_NONE)
            return sts;
    }

    //if (out)
    //   printf("VideoENC_LA::QueryFrameLA %d\n",out->Data.FrameOrder);


    bool     bNoInput = (pInSurface == 0);
    bool     bSubmittedFrames = false;
    bool     bRistrict = false;

    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_INTERNAL, "LA::Query");

    bSubmittedFrames    = m_VMETasks.size() > 0;

    if (bSubmittedFrames)
    {
        sLADdiTask              currDDILATask = {0};
        sLADdiTask*             task  = &currDDILATask;

        {
            UMC::AutomaticUMCMutex guard(m_listMutex);
            currDDILATask = m_VMETasks.front();
        }

        mfxStatus sts = m_cmCtx->QueryVme(*task, task->m_event);
        if(sts != MFX_ERR_NONE)
            return sts;

        {
            UMC::AutomaticUMCMutex guard(m_listMutex);
            m_VMETasks.pop_front();
        }

        //printf("m_cmCtx->QueryVme %d,  %d,%d,%d,  %d, ref %d %d\n", task->m_vmeData->encOrder,task->m_vmeData->poc,task->m_vmeData->pocL0, task->m_vmeData->pocL1,task->m_vmeData->interCost, task->m_vmeDataRef[REF_FORW]==0 ? 0: task->m_vmeDataRef[REF_FORW]->encOrder, task->m_vmeDataRef[REF_BACKW]==0 ? 0: task->m_vmeDataRef[REF_BACKW]->encOrder);

        // release unused VME resources
        {
            ReleaseResource(m_curbe, task->m_cmCurbe);
            if (m_cmDevice)
            {
                m_cmDevice->DestroyVmeSurfaceG7_5(task->m_cmRefs);
                m_cmDevice->DestroyVmeSurfaceG7_5(task->m_cmRefsLa);
                m_cmCtx->DestroyEvent(task->m_event);
            }
            if (!(task->m_TaskInfo.InputFrame.frameType & MFX_FRAMETYPE_REF))
            {
                FreeUnusedVMEData(&task->m_Curr);
            }
        }
        //VME data analisys, store frame level info
        {

            sLASummaryTask frameForOutput = {};
            frameForOutput.frameInfo  = task->m_TaskInfo.InputFrame;
            frameForOutput.m_vmeData  = task->m_Curr.VmeData;
            frameForOutput.m_vmeDataRef[0] = task->m_Ref[0].VmeData;
            frameForOutput.m_vmeDataRef[1] = task->m_Ref[1].VmeData;


            m_OutputTasks.push_back(frameForOutput);

            if (task->m_TaskInfo.RefFrame[0].pFrame)
                m_core->DecreaseReference(&task->m_TaskInfo.RefFrame[0].pFrame->Data);
            if (task->m_TaskInfo.RefFrame[1].pFrame)
                m_core->DecreaseReference(&task->m_TaskInfo.RefFrame[1].pFrame->Data);

            if (output && (m_OutputTasks.size() < m_LaControl.DependencyDepth))
                bRistrict = 1;

            if (m_OutputTasks.size() >= m_LaControl.DependencyDepth)
            {
                std::list<sLASummaryTask>::iterator currTask = m_OutputTasks.end();
                // zero propCost fields for last DependencyDepth tasks
                {
                    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_INTERNAL, "ZeroVmeData");
                    currTask = m_OutputTasks.end();
                    for (int i = 0; i < m_LaControl.DependencyDepth; i++)
                    {
                        //MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_INTERNAL, "LA::xx");
                        currTask--;
                        MfxHwH264Encode::VmeData *vme = (*currTask).m_vmeData;
                        vme->propCost = 0;
                        for (mfxU32  k = 0 ;k< vme->mb.size(); k++)
                        {
                            vme->mb[k].propCost = 0;
                        }
                    }
                }
                // VME for the last DependencyDepth-1 tasks
                {
                    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_INTERNAL, "AnalyzeVmeData");
                    currTask = m_OutputTasks.end();
                    for (int i=0; i < m_LaControl.DependencyDepth-1; i++)
                    {
                        --currTask;
                        AnalyzeVmeData((*currTask).m_vmeData, (*currTask).m_vmeDataRef[REF_FORW], (*currTask).m_vmeDataRef[REF_BACKW], m_video.calcParam.widthLa, m_video.calcParam.heightLa);
                    }
                }
                // PreEnc for task previous last VME task (calculates frame level info)
                std::list<sLASummaryTask>::iterator PreEncTask = currTask;
                --PreEncTask;


                for (int i = 0; i < m_LaControl.NumOutStream; i ++)
                {
                    //printf("LA::PreEnc(%d) - %x", (*PreEncTask).frameInfo.encFrameOrder, (*PreEncTask).frameInfo.pFrame);
                    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_INTERNAL, "PreEnc");

                    mfxF32 scale1 =  (mfxF32)m_LaControl.OutStream[i].Width /  (mfxF32)m_video.calcParam.widthLa;
                    mfxF32 scale2 =  (mfxF32)m_LaControl.OutStream[i].Height / (mfxF32)m_video.calcParam.heightLa;
                    if (scale1 > 1.0f && scale2 > 1.0f)
                    {
                        PreEnc((*PreEncTask).m_vmeData, (*PreEncTask).VMESum[i], scale1*scale2);
                    }
                    else
                    {
                        PreEnc((*PreEncTask).m_vmeData, (*PreEncTask).VMESum[i], m_video.calcParam.widthLa,m_video.calcParam.heightLa, m_LaControl.OutStream[i].Width,m_LaControl.OutStream[i].Height );
                    }
                }
                if (!((*PreEncTask).frameInfo.frameType & MFX_FRAMETYPE_REF))
                {
                    //printf("del: %d,  %d,%d,%d,  %d\n", (*PreEncTask).m_vmeData->encOrder,(*PreEncTask).m_vmeData->poc,(*PreEncTask).m_vmeData->pocL0, (*PreEncTask).m_vmeData->pocL1,(*PreEncTask).m_vmeData->interCost);
                    (*PreEncTask).m_vmeData->used = false;
                }
            }
        }
    }
    // proceed buffered frames
    if ((!bSubmittedFrames && !m_LAAsyncContext.bPreEncLastFrames && bNoInput && m_OutputTasks.size() >0) || bRistrict)
    {
        std::list<sLASummaryTask>::iterator currTask = m_OutputTasks.end();
        for (int i = 0; i < m_LaControl.DependencyDepth; i++)
        {
            currTask --;
            for (int j = 0; j < m_LaControl.NumOutStream; j++)
            {
                mfxF32 scale1 =  (mfxF32)m_LaControl.OutStream[j].Width /  (mfxF32)m_video.calcParam.widthLa;
                mfxF32 scale2 =  (mfxF32)m_LaControl.OutStream[j].Height / (mfxF32)m_video.calcParam.heightLa;
                if (scale1 > 1.0f && scale2 > 1.0f)
                {
                    PreEnc((*currTask).m_vmeData, (*currTask).VMESum[j], scale1*scale2);
                }
                else
                {
                    PreEnc((*currTask).m_vmeData, (*currTask).VMESum[j], m_video.calcParam.widthLa,m_video.calcParam.heightLa, m_LaControl.OutStream[j].Width,m_LaControl.OutStream[j].Height );
                }
            }
            (*currTask).m_vmeData->used = false;
            if ( currTask == m_OutputTasks.begin()) break;
        }
        m_LAAsyncContext.bPreEncLastFrames = true;
    }
    else if (!bSubmittedFrames &&!bNoInput)
    {
        return MFX_TASK_BUSY;
    }

    // prepare output
    if (out)
    {
        MFX_CHECK(aux, MFX_ERR_NULL_PTR);

        sLASummaryTask frameForOutput = m_OutputTasks.front();

        //char task_name [40];
        //sprintf(task_name,"LA::prepare_output(%d) - %x", frameForOutput.frameInfo.encFrameOrder, frameForOutput.frameInfo.pFrame);
        //MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_INTERNAL, task_name);

        mfxU16 frameNum = (m_OutputTasks.size() > m_LaControl.LookAheadDepth) ? m_LaControl.LookAheadDepth : (mfxU16)m_OutputTasks.size();
        frameNum = (frameNum>=m_LaControl.DependencyDepth && !m_LAAsyncContext.bPreEncLastFrames) ? frameNum - m_LaControl.DependencyDepth : frameNum;

         // summury MB data and copy into output structure

        //printf("out: %d,  %d, %d\n", , out->Data.FrameOrder,  frameForOutput.frameInfo.pFrame->Data.FrameOrder);
        MFX_CHECK(aux->NumFrame*aux->NumStream <= aux->NumAlloc, MFX_ERR_MEMORY_ALLOC);
        MFX_CHECK(out == frameForOutput.frameInfo.pFrame, MFX_ERR_UNDEFINED_BEHAVIOR )


        aux->NumFrame   = frameNum;
        aux->NumStream  = m_LaControl.NumOutStream;
        aux->OutSurface->Info.PicStruct = frameForOutput.frameInfo.pFrame->Info.PicStruct;

        for (int j = 0; j < m_LaControl.NumOutStream; j ++)
        {
            std::list<sLASummaryTask>::iterator nextTask = m_OutputTasks.begin();
            mfxLAFrameInfo* pFrameData =  &aux->FrameStat[frameNum*j];
            for (mfxU32 i = 0; i < frameNum; i++)
            {
                pFrameData[i].Width  = m_LaControl.OutStream[j].Width;
                pFrameData[i].Height = m_LaControl.OutStream[j].Height;

                pFrameData[i].FrameType         = nextTask->frameInfo.frameType;
                pFrameData[i].FrameDisplayOrder = nextTask->frameInfo.dispFrameOrder;
                pFrameData[i].FrameEncodeOrder  = nextTask->frameInfo.encFrameOrder;
                pFrameData[i].Layer             = nextTask->frameInfo.layer;

                pFrameData[i].IntraCost         = nextTask->VMESum[j].IntraCost;
                pFrameData[i].InterCost         = nextTask->VMESum[j].InterCost;
                pFrameData[i].DependencyCost    = nextTask->VMESum[j].DependencyCost;

                std::copy(std::begin(nextTask->VMESum[j].EstimatedRate), std::end(nextTask->VMESum[j].EstimatedRate), std::begin(pFrameData[i].EstimatedRate));
                nextTask++;
            }
        }
        m_core->DecreaseReference(&out_t->Data);


        m_OutputTasks.pop_front();
    }
    return MFX_ERR_NONE;

} // mfxStatus VideoVPPSW::RunFrameVPP(


mfxStatus VideoENC_LA::Close(void)
{
    if (!m_bInit) return MFX_ERR_NONE;
    m_bInit = false;

    sVMEFrameInfo *p=  m_VMERefList.GetOldest();

    while (p )
    {
        p->VmeData->used = false;
        FreeUnusedVMEData(p);
        p->bUsed = false;
        p =  m_VMERefList.GetOldest();
    }

    m_core->FreeFrames(&m_raw);
    m_core->FreeFrames(&m_opaqResponse);



    return MFX_ERR_NONE;

}

const mfxU16 Dist10[26] =
{
    0,0,0,0,2,
    4,7,11,17,25,
    35,50,68,91,119,
    153,194,241,296,360,
    432,513,604,706,819,
    944
};

const mfxU16 Dist25[26] =
{
    0,0,0,0,
    12,32,51,69,87,
    107,129,154,184,219,
    260,309,365,431,507,
    594,694,806,933,1074,
    1232,1406
};

namespace MfxHwH264EncodeHW
{


    void SetCosts(MfxHwH264Encode::mfxVMEUNIIn & costs,
        mfxU32        frameType,
        mfxU32        qp,
        mfxU32        intraSad,
        mfxU32        ftqBasedSkip);
    mfxU32 SetSearchPath(MfxHwH264Encode::mfxVMEIMEIn & spath,
        mfxU32        frameType,
        mfxU32        meMethod);
    mfxU8 GetMaxMvsPer2Mb(mfxU32 level);
    const mfxU32 BATCHBUFFER_END   = 0x5000000;
    mfxU32 GetMaxMvLenY(mfxU32 level);
    void Write(CmBuffer * buffer, void * buf, CmEvent * e = 0);
    SurfaceIndex & GetIndex(CmSurface2D * surface);
    SurfaceIndex & GetIndex(CmBuffer * buffer);
    SurfaceIndex & GetIndex(CmBufferUP * buffer);
    mfxU32 Map44LutValueBack(mfxU32 val);
    mfxU16 GetVmeMvCostP(
        mfxU32 const         lutMv[65],
        MfxHwH264Encode::LAOutObject const & mb);
    mfxU16 GetVmeMvCostB(
        mfxU32 const         lutMv[65],
        MfxHwH264Encode::LAOutObject const & mb);

};
    mfxI32 CalcDistScaleFactor(
        mfxU32 pocCur,
        mfxU32 pocL0,
        mfxU32 pocL1)
    {
        mfxI32 tb = mfx::clamp(mfxI32(pocCur - pocL0), -128, 127);
        mfxI32 td = mfx::clamp(mfxI32(pocL1  - pocL0), -128, 127);
        mfxI32 tx = (16384 + abs(td/2)) / td;
        return mfx::clamp((tb * tx + 32) >> 6, -1024, 1023);
    }




void CmContextLA::SetCurbeData(
    MfxHwH264Encode::CURBEData & curbeData,
    sLADdiTask const &   task,
    mfxU32            qp)
{
    mfxExtCodingOptionDDI const * extDdi = MfxHwH264Encode::GetExtBuffer(m_video);
    //mfxExtCodingOption const *    extOpt = GetExtBuffer(m_video);

    mfxU32 interSad       = 2; // 0-sad,2-haar
    mfxU32 intraSad       = 2; // 0-sad,2-haar
    mfxU32 ftqBasedSkip   = 3; //3;
    mfxU32 blockBasedSkip = 1;
    mfxU32 qpIdx          = (qp + 1) >> 1;
    mfxU32 transformFlag  = 0;//(extOpt->IntraPredBlockSize > 1);
    mfxU32 meMethod       = 6;
    mfxU16 frameType = task.m_TaskInfo.InputFrame.frameType;

    mfxU32 skipVal = (frameType & MFX_FRAMETYPE_P) ? (Dist10[qpIdx]) : (Dist25[qpIdx]);
    if (!blockBasedSkip)
        skipVal *= 3;
    else if (!transformFlag)
        skipVal /= 2;

    MfxHwH264Encode::mfxVMEUNIIn costs {};
    SetCosts(costs, frameType, qp, intraSad, ftqBasedSkip);

    MfxHwH264Encode::mfxVMEIMEIn spath;
    SetSearchPath(spath, frameType, meMethod);

    //init CURBE data based on Image State and Slice State. this CURBE data will be
    //written to the surface and sent to all kernels.
    Zero(curbeData);

    //DW0
    curbeData.SkipModeEn            = !(frameType & MFX_FRAMETYPE_I);
    curbeData.AdaptiveEn            = 1;
    curbeData.BiMixDis              = 0;
    curbeData.EarlyImeSuccessEn     = 0;
    curbeData.T8x8FlagForInterEn    = 1;
    curbeData.EarlyImeStop          = 0;
    //DW1
    curbeData.MaxNumMVs             = (GetMaxMvsPer2Mb(m_video.mfx.CodecLevel) >> 1) & 0x3F;    // 8
    mfxI32 biWeight = 32;
    if (frameType & MFX_FRAMETYPE_B)
    {
        biWeight = CalcDistScaleFactor(task.m_TaskInfo.InputFrame.poc, task.m_TaskInfo.RefFrame[0].poc, task.m_TaskInfo.RefFrame[1].poc) >>2;
        biWeight = (biWeight < -64 || biWeight > 128)? 32: biWeight;
    }
    curbeData.BiWeight              = biWeight; // TO DO

    curbeData.UniMixDisable         = 0;
    //DW2
    curbeData.MaxNumSU              = 57;
    curbeData.LenSP                 = 16;
    //DW3
    curbeData.SrcSize               = 0;
    curbeData.MbTypeRemap           = 0;
    curbeData.SrcAccess             = 0;
    curbeData.RefAccess             = 0;
    curbeData.SearchCtrl            = (frameType & MFX_FRAMETYPE_B) ? 7 : 0;
    curbeData.DualSearchPathOption  = 0;
    curbeData.SubPelMode            = 3; // all modes
    curbeData.SkipType              = 0;//!!(task.m_type[0] & MFX_FRAMETYPE_B); //for B 0-16x16, 1-8x8
    curbeData.DisableFieldCacheAllocation = 0;
    curbeData.InterChromaMode       = 0;
    curbeData.FTQSkipEnable         = !(frameType & MFX_FRAMETYPE_I);
    curbeData.BMEDisableFBR         = !!(frameType & MFX_FRAMETYPE_P);
    curbeData.BlockBasedSkipEnabled = blockBasedSkip;
    curbeData.InterSAD              = interSad;
    curbeData.IntraSAD              = intraSad;
    curbeData.SubMbPartMask         = (frameType & MFX_FRAMETYPE_I) ? 0 : 0x7e; // only 16x16 for Inter
    //DW4
    /*
    curbeData.SliceMacroblockHeightMinusOne = m_video.mfx.FrameInfo.Height / 16 - 1;
    curbeData.PictureHeightMinusOne = m_video.mfx.FrameInfo.Height / 16 - 1;
    curbeData.PictureWidthMinusOne  = m_video.mfx.FrameInfo.Width / 16 - 1;
    */
    curbeData.SliceMacroblockHeightMinusOne = widthLa / 16 - 1;
    curbeData.PictureHeightMinusOne = heightLa / 16 - 1;
    curbeData.PictureWidthMinusOne  = widthLa / 16 - 1;
    //DW5
    curbeData.RefWidth              = (frameType & MFX_FRAMETYPE_B) ? 32 : 48;
    curbeData.RefHeight             = (frameType & MFX_FRAMETYPE_B) ? 32 : 40;
    //DW6
    curbeData.BatchBufferEndCommand = BATCHBUFFER_END;
    //DW7
    curbeData.IntraPartMask         = 2|4; //no4x4 and no8x8 //transformFlag ? 0 : 2/*no8x8*/;
    curbeData.NonSkipZMvAdded       = !!(frameType & MFX_FRAMETYPE_P);
    curbeData.NonSkipModeAdded      = !!(frameType & MFX_FRAMETYPE_P);
    curbeData.IntraCornerSwap       = 0;
    curbeData.MVCostScaleFactor     = 0;
    curbeData.BilinearEnable        = 0;
    curbeData.SrcFieldPolarity      = 0;
    curbeData.WeightedSADHAAR       = 0;
    curbeData.AConlyHAAR            = 0;
    curbeData.RefIDCostMode         = !(frameType & MFX_FRAMETYPE_I);
    curbeData.SkipCenterMask        = !!(frameType & MFX_FRAMETYPE_P);
    //DW8
    curbeData.ModeCost_0            = costs.ModeCost[LUTMODE_INTRA_NONPRED];
    curbeData.ModeCost_1            = costs.ModeCost[LUTMODE_INTRA_16x16];
    curbeData.ModeCost_2            = costs.ModeCost[LUTMODE_INTRA_8x8];
    curbeData.ModeCost_3            = costs.ModeCost[LUTMODE_INTRA_4x4];
    //DW9
    curbeData.ModeCost_4            = costs.ModeCost[LUTMODE_INTER_16x8];
    curbeData.ModeCost_5            = costs.ModeCost[LUTMODE_INTER_8x8q];
    curbeData.ModeCost_6            = costs.ModeCost[LUTMODE_INTER_8x4q];
    curbeData.ModeCost_7            = costs.ModeCost[LUTMODE_INTER_4x4q];
    //DW10
    curbeData.ModeCost_8            = costs.ModeCost[LUTMODE_INTER_16x16];
    curbeData.ModeCost_9            = costs.ModeCost[LUTMODE_INTER_BWD];
    curbeData.RefIDCost             = costs.ModeCost[LUTMODE_REF_ID];
    curbeData.ChromaIntraModeCost   = costs.ModeCost[LUTMODE_INTRA_CHROMA];
    //DW11
    curbeData.MvCost_0              = costs.MvCost[0];
    curbeData.MvCost_1              = costs.MvCost[1];
    curbeData.MvCost_2              = costs.MvCost[2];
    curbeData.MvCost_3              = costs.MvCost[3];
    //DW12
    curbeData.MvCost_4              = costs.MvCost[4];
    curbeData.MvCost_5              = costs.MvCost[5];
    curbeData.MvCost_6              = costs.MvCost[6];
    curbeData.MvCost_7              = costs.MvCost[7];
    //DW13
    curbeData.QpPrimeY              = qp;
    curbeData.QpPrimeCb             = qp;
    curbeData.QpPrimeCr             = qp;
    curbeData.TargetSizeInWord      = 0xff;
    //DW14
    curbeData.FTXCoeffThresh_DC               = costs.FTXCoeffThresh_DC;
    curbeData.FTXCoeffThresh_1                = costs.FTXCoeffThresh[0];
    curbeData.FTXCoeffThresh_2                = costs.FTXCoeffThresh[1];
    //DW15
    curbeData.FTXCoeffThresh_3                = costs.FTXCoeffThresh[2];
    curbeData.FTXCoeffThresh_4                = costs.FTXCoeffThresh[3];
    curbeData.FTXCoeffThresh_5                = costs.FTXCoeffThresh[4];
    curbeData.FTXCoeffThresh_6                = costs.FTXCoeffThresh[5];
    //DW16
    curbeData.IMESearchPath0                  = spath.IMESearchPath0to31[0];
    curbeData.IMESearchPath1                  = spath.IMESearchPath0to31[1];
    curbeData.IMESearchPath2                  = spath.IMESearchPath0to31[2];
    curbeData.IMESearchPath3                  = spath.IMESearchPath0to31[3];
    //DW17
    curbeData.IMESearchPath4                  = spath.IMESearchPath0to31[4];
    curbeData.IMESearchPath5                  = spath.IMESearchPath0to31[5];
    curbeData.IMESearchPath6                  = spath.IMESearchPath0to31[6];
    curbeData.IMESearchPath7                  = spath.IMESearchPath0to31[7];
    //DW18
    curbeData.IMESearchPath8                  = spath.IMESearchPath0to31[8];
    curbeData.IMESearchPath9                  = spath.IMESearchPath0to31[9];
    curbeData.IMESearchPath10                 = spath.IMESearchPath0to31[10];
    curbeData.IMESearchPath11                 = spath.IMESearchPath0to31[11];
    //DW19
    curbeData.IMESearchPath12                 = spath.IMESearchPath0to31[12];
    curbeData.IMESearchPath13                 = spath.IMESearchPath0to31[13];
    curbeData.IMESearchPath14                 = spath.IMESearchPath0to31[14];
    curbeData.IMESearchPath15                 = spath.IMESearchPath0to31[15];
    //DW20
    curbeData.IMESearchPath16                 = spath.IMESearchPath0to31[16];
    curbeData.IMESearchPath17                 = spath.IMESearchPath0to31[17];
    curbeData.IMESearchPath18                 = spath.IMESearchPath0to31[18];
    curbeData.IMESearchPath19                 = spath.IMESearchPath0to31[19];
    //DW21
    curbeData.IMESearchPath20                 = spath.IMESearchPath0to31[20];
    curbeData.IMESearchPath21                 = spath.IMESearchPath0to31[21];
    curbeData.IMESearchPath22                 = spath.IMESearchPath0to31[22];
    curbeData.IMESearchPath23                 = spath.IMESearchPath0to31[23];
    //DW22
    curbeData.IMESearchPath24                 = spath.IMESearchPath0to31[24];
    curbeData.IMESearchPath25                 = spath.IMESearchPath0to31[25];
    curbeData.IMESearchPath26                 = spath.IMESearchPath0to31[26];
    curbeData.IMESearchPath27                 = spath.IMESearchPath0to31[27];
    //DW23
    curbeData.IMESearchPath28                 = spath.IMESearchPath0to31[28];
    curbeData.IMESearchPath29                 = spath.IMESearchPath0to31[29];
    curbeData.IMESearchPath30                 = spath.IMESearchPath0to31[30];
    curbeData.IMESearchPath31                 = spath.IMESearchPath0to31[31];
    //DW24
    curbeData.IMESearchPath32                 = spath.IMESearchPath32to55[0];
    curbeData.IMESearchPath33                 = spath.IMESearchPath32to55[1];
    curbeData.IMESearchPath34                 = spath.IMESearchPath32to55[2];
    curbeData.IMESearchPath35                 = spath.IMESearchPath32to55[3];
    //DW25
    curbeData.IMESearchPath36                 = spath.IMESearchPath32to55[4];
    curbeData.IMESearchPath37                 = spath.IMESearchPath32to55[5];
    curbeData.IMESearchPath38                 = spath.IMESearchPath32to55[6];
    curbeData.IMESearchPath39                 = spath.IMESearchPath32to55[7];
    //DW26
    curbeData.IMESearchPath40                 = spath.IMESearchPath32to55[8];
    curbeData.IMESearchPath41                 = spath.IMESearchPath32to55[9];
    curbeData.IMESearchPath42                 = spath.IMESearchPath32to55[10];
    curbeData.IMESearchPath43                 = spath.IMESearchPath32to55[11];
    //DW27
    curbeData.IMESearchPath44                 = spath.IMESearchPath32to55[12];
    curbeData.IMESearchPath45                 = spath.IMESearchPath32to55[13];
    curbeData.IMESearchPath46                 = spath.IMESearchPath32to55[14];
    curbeData.IMESearchPath47                 = spath.IMESearchPath32to55[15];
    //DW28
    curbeData.IMESearchPath48                 = spath.IMESearchPath32to55[16];
    curbeData.IMESearchPath49                 = spath.IMESearchPath32to55[17];
    curbeData.IMESearchPath50                 = spath.IMESearchPath32to55[18];
    curbeData.IMESearchPath51                 = spath.IMESearchPath32to55[19];
    //DW29
    curbeData.IMESearchPath52                 = spath.IMESearchPath32to55[20];
    curbeData.IMESearchPath53                 = spath.IMESearchPath32to55[21];
    curbeData.IMESearchPath54                 = spath.IMESearchPath32to55[22];
    curbeData.IMESearchPath55                 = spath.IMESearchPath32to55[23];
    //DW30
    curbeData.Intra4x4ModeMask                = 0;
    curbeData.Intra8x8ModeMask                = 0;
    //DW31
    curbeData.Intra16x16ModeMask              = 0;
    curbeData.IntraChromaModeMask             = 1; // 0; 1 means Luma only
    curbeData.IntraComputeType                = 0;
    //DW32
    curbeData.SkipVal                         = skipVal;
    curbeData.MultipredictorL0EnableBit       = 0xEF;
    curbeData.MultipredictorL1EnableBit       = 0xEF;
    //DW33
    curbeData.IntraNonDCPenalty16x16          = 36;
    curbeData.IntraNonDCPenalty8x8            = 12;
    curbeData.IntraNonDCPenalty4x4            = 4;
    //DW34
    curbeData.MaxVmvR                         = GetMaxMvLenY(m_video.mfx.CodecLevel) * 4; // 2044
    //DW35
    curbeData.PanicModeMBThreshold            = 0xFF;
    curbeData.SmallMbSizeInWord               = 0xFF;
    curbeData.LargeMbSizeInWord               = 0xFF;
    //DW36
    curbeData.HMECombineOverlap               = 1;  //  0;
    curbeData.HMERefWindowsCombiningThreshold = (frameType & MFX_FRAMETYPE_B) ? 8 : 16; //  0; (should be =8 for B frames)
    curbeData.CheckAllFractionalEnable        = 0;
    //DW37
    curbeData.CurLayerDQId                    = extDdi->LaScaleFactor;  // 0; use 8 bit as LaScaleFactor
    curbeData.TemporalId                      = 0;
    curbeData.NoInterLayerPredictionFlag      = 1;
    curbeData.AdaptivePredictionFlag          = 0;
    curbeData.DefaultBaseModeFlag             = 0;
    curbeData.AdaptiveResidualPredictionFlag  = 0;
    curbeData.DefaultResidualPredictionFlag   = 0;
    curbeData.AdaptiveMotionPredictionFlag    = 0;
    curbeData.DefaultMotionPredictionFlag     = 0;
    curbeData.TcoeffLevelPredictionFlag       = 0;
    curbeData.UseHMEPredictor                 = 0;  //!!IsOn(extDdi->Hme);
    curbeData.SpatialResChangeFlag            = 0;
    curbeData.isFwdFrameShortTermRef          = !(frameType & MFX_FRAMETYPE_I); //task.m_list0[0].Size() > 0 && !task.m_dpb[0][task.m_list0[0][0] & 127].m_longterm;
    //DW38
    curbeData.ScaledRefLayerLeftOffset        = 0;
    curbeData.ScaledRefLayerRightOffset       = 0;
    //DW39
    curbeData.ScaledRefLayerTopOffset         = 0;
    curbeData.ScaledRefLayerBottomOffset      = 0;
}
CmEvent * CmContextLA::RunVme(sLADdiTask const & task,
                              mfxU32          qp)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_INTERNAL, "RunVme");
    qp = 26;

    CmKernel * kernelPreMe = SelectKernelPreMe(task.m_TaskInfo.InputFrame.frameType);

    MfxHwH264Encode::CURBEData curbeData;
    SetCurbeData(curbeData, task, qp);
    Write(task.m_cmCurbe, &curbeData);

    mfxU32 numMbColsLa = widthLa / 16;
    mfxU32 numMbRowsLa = heightLa / 16;

    if (LaScaleFactor > 1)
    {


        MfxHwH264Encode::SetKernelArg(kernelPreMe,
        GetIndex(task.m_cmCurbe), GetIndex(task.m_Curr.CmRaw), GetIndex(task.m_Curr.CmRawLA), *task.m_cmRefsLa,
        GetIndex(task.m_Curr.CmMb), task.m_Ref[REF_BACKW].CmMb ? GetIndex(task.m_Ref[REF_BACKW].CmMb) : GetIndex(m_nullBuf));

    } else  {
        MfxHwH264Encode::SetKernelArg(kernelPreMe,
            GetIndex(task.m_cmCurbe), GetIndex(m_nullBuf), GetIndex(task.m_Curr.CmRaw), *task.m_cmRefs,
            GetIndex(task.m_Curr.CmMb), task.m_Ref[REF_BACKW].CmMb ? GetIndex(task.m_Ref[REF_BACKW].CmMb) : GetIndex(m_nullBuf));
    }

    CmEvent * e = 0;
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_INTERNAL, "Enqueue ME kernel");
        e = EnqueueKernel(kernelPreMe, numMbColsLa, numMbRowsLa, CM_WAVEFRONT26);
    }
    return e;
}
mfxStatus CmContextLA::QueryVme(sLADdiTask const & task,
                           CmEvent *       e)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_INTERNAL, "QueryVme");

    MFX_CHECK_NULL_PTR1(e);

    INT status = e->WaitForTaskFinished();
    if (status == CM_EXCEED_MAX_TIMEOUT)
        return MFX_ERR_GPU_HANG;
    else if(status != CM_SUCCESS)
        throw MfxHwH264Encode::CmRuntimeError();

    MfxHwH264Encode::LAOutObject * cmMb = (MfxHwH264Encode::LAOutObject *)task.m_cmMbSys;
    MfxHwH264Encode::VmeData *      cur  = task.m_Curr.VmeData;

    MFX_CHECK_NULL_PTR2(cmMb, cur);

    { MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_INTERNAL, "Compensate costs");
    MfxHwH264Encode::mfxVMEUNIIn const & costs = SelectCosts(task.m_TaskInfo.InputFrame.frameType);
    for (size_t i = 0; i < cur->mb.size(); i++)
    {
        MfxHwH264Encode::LAOutObject & mb = cmMb[i];

        if (mb.IntraMbFlag)
        {
            mfxU16 bitCostLambda = mfxU16(Map44LutValueBack(costs.ModeCost[LUTMODE_INTRA_16x16]));
            assert(mb.intraCost >= bitCostLambda);
            mb.dist = mb.intraCost - bitCostLambda;
        }
        else
        {
            if (mb.MbType5Bits != MBTYPE_BP_L0_16x16 &&
                mb.MbType5Bits != MBTYPE_B_L1_16x16  &&
                mb.MbType5Bits != MBTYPE_B_Bi_16x16)
                assert(0);

            mfxU32 modeCostLambda = Map44LutValueBack(costs.ModeCost[LUTMODE_INTER_16x16]);
            mfxU32 mvCostLambda   = (task.m_TaskInfo.InputFrame.frameType & MFX_FRAMETYPE_P)
                ? GetVmeMvCostP(m_lutMvP, mb) : GetVmeMvCostB(m_lutMvB, mb);
            mfxU16 bitCostLambda = mfxU16(std::min<mfxU32>(mb.interCost, modeCostLambda + mvCostLambda));
            mb.dist = mb.interCost - bitCostLambda;
        }
    }
    }


    { MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_INTERNAL, "Convert mb data");
    //MfxHwH264Encode::mfxExtPpsHeader const * extPps = MfxHwH264Encode::GetExtBuffer(m_video);

    cur->intraCost = 0;
    cur->interCost = 0;

    for (size_t i = 0; i < cur->mb.size(); i++)
    {
        cur->mb[i].intraCost     = cmMb[i].intraCost;
        cur->mb[i].interCost     = std::min(cmMb[i].intraCost, cmMb[i].interCost);
        cur->mb[i].intraMbFlag   = cmMb[i].IntraMbFlag;
        cur->mb[i].skipMbFlag    = cmMb[i].SkipMbFlag;
        cur->mb[i].mbType        = cmMb[i].MbType5Bits;
        cur->mb[i].subMbShape    = cmMb[i].SubMbShape;
        cur->mb[i].subMbPredMode = cmMb[i].SubMbPredMode;

        mfxI32 biWeight = 32;
        if (task.m_TaskInfo.InputFrame.frameType & MFX_FRAMETYPE_B)
        {
            biWeight = CalcDistScaleFactor(task.m_TaskInfo.InputFrame.poc, task.m_TaskInfo.RefFrame[0].poc, task.m_TaskInfo.RefFrame[1].poc) >>2;
            biWeight = (biWeight < -64 || biWeight > 128)? 32: biWeight;
        }

        cur->mb[i].w1            = mfxU8(biWeight);
        cur->mb[i].w0            = mfxU8(64 - cur->mb[i].w1);

        cur->mb[i].costCenter0.x = cmMb[i].costCenter0X;
        cur->mb[i].costCenter0.y = cmMb[i].costCenter0Y;
        cur->mb[i].costCenter1.x = cmMb[i].costCenter1X;
        cur->mb[i].costCenter1.y = cmMb[i].costCenter1Y;
        cur->mb[i].dist          = cmMb[i].dist;
        cur->mb[i].propCost      = 0;

        MfxHwH264Encode::Copy(cur->mb[i].lumaCoeffSum, cmMb[i].lumaCoeffSum);
        MfxHwH264Encode::Copy(cur->mb[i].lumaCoeffCnt, cmMb[i].lumaCoeffCnt);
        MfxHwH264Encode::Copy(cur->mb[i].mv, cmMb[i].mv);

        cur->intraCost += cur->mb[i].intraCost;
        cur->interCost += cur->mb[i].interCost;
    }
    }

    return MFX_ERR_NONE;
}

#endif  // MFX_ENABLE_H264_VIDEO_ENCODE_HW && MFX_ENABLE_LA_H264_VIDEO_HW

/* EOF */

