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

#include "math.h"
#include "mfx_common.h"

#if defined (MFX_ENABLE_VPP)
#include "mfx_enc_common.h"
#include "mfx_session.h"

#include "mfx_vpp_hw.h"
#include "libmfx_core.h"
#include "libmfx_core_factory.h"
#include "libmfx_core_interface.h"

#include "mfx_vpp_utils.h"
#include "mfx_vpp_sw.h"


using namespace MfxHwVideoProcessing;
class CmDevice;

VideoVPPBase* CreateAndInitVPPImpl(mfxVideoParam *par, VideoCORE *core, mfxStatus *mfxSts)
{
    bool bHWInitFailed = false;
    VideoVPPBase * vpp = 0;
    if( MFX_PLATFORM_HARDWARE == core->GetPlatformType())
    {
        vpp = new VideoVPP_HW(core, mfxSts);
        if (*mfxSts != MFX_ERR_NONE)
        {
            delete vpp;
            return 0;
        }

        *mfxSts = vpp->Init(par);
        if (*mfxSts < MFX_ERR_NONE)
        {
            delete vpp;
            return 0;
        }

        if(MFX_WRN_INCOMPATIBLE_VIDEO_PARAM == *mfxSts || MFX_WRN_FILTER_SKIPPED == *mfxSts || MFX_ERR_NONE == *mfxSts)
        {
            return vpp;
        }

        delete vpp;
        vpp = 0;
        bHWInitFailed = true;
    }

    *mfxSts = MFX_ERR_UNSUPPORTED;
    return 0;
}

/* ******************************************************************** */
/*                           useful macros                              */
/* ******************************************************************** */

#ifndef VPP_CHECK_STS_SAFE
#define VPP_CHECK_STS_SAFE(sts, in, out)               \
{                                                      \
    if (sts != MFX_ERR_NONE && in)                       \
{                                                    \
    m_core->DecreaseReference( &(in->Data) );          \
}                                                    \
    if (sts != MFX_ERR_NONE && out)                      \
{                                                    \
    m_core->DecreaseReference( &(out->Data) );         \
}                                                    \
    MFX_CHECK_STS( sts );                                \
}
#endif


#define VPP_UPDATE_STAT( sts, stat )                      \
{                                                         \
    if( MFX_ERR_NONE == sts ) stat.NumFrame++;              \
    if( MFX_ERR_NULL_PTR == sts ) stat.NumCachedFrame++;\
}

#define VPP_UNLOCK_SURFACE(sts, surface)                  \
{                                                         \
    if( MFX_ERR_NONE == sts )  m_core->DecreaseReference( &surface->Data );\
}

/* ******************************************************************** */
/*      implementation of VPP Pipeline: interface methods               */
/* ******************************************************************** */

VideoVPPBase::VideoVPPBase(VideoCORE *core, mfxStatus* sts )
    : m_pipelineList()
    , m_core(core)
    , m_pHWVPP()
{
    /* opaque processing */
    m_bOpaqMode[VPP_IN]  = false;
    m_bOpaqMode[VPP_OUT] = false;

    memset(&m_requestOpaq[VPP_IN], 0, sizeof(mfxFrameAllocRequest));
    memset(&m_requestOpaq[VPP_OUT], 0, sizeof(mfxFrameAllocRequest));

    /* common */
    m_bDynamicDeinterlace = false;
    memset(&m_stat, 0, sizeof(mfxVPPStat));
    memset(&m_errPrtctState, 0, sizeof(sErrPrtctState));
    memset(&m_InitState, 0, sizeof(sErrPrtctState));

    VPP_CLEAN;

    *sts = MFX_ERR_NONE;
} // VideoVPPBase::VideoVPPBase(VideoCORE *core, mfxStatus* sts ) : VideoVPP()

VideoVPPBase::~VideoVPPBase()
{
    Close();
} // VideoVPPBase::~VideoVPPBase()

mfxStatus VideoVPPBase::Close(void)
{
    VPP_CHECK_NOT_INITIALIZED;

    m_stat.NumCachedFrame = 0;
    m_stat.NumFrame       = 0;

    m_bDynamicDeinterlace = false;

    /* opaque processing */

    if( m_bOpaqMode[VPP_IN] )
    {
        m_requestOpaq[VPP_IN].NumFrameMin = m_requestOpaq[VPP_IN].NumFrameSuggested = 0;
        m_requestOpaq[VPP_IN].Type = 0;
    }

    if( m_bOpaqMode[VPP_OUT] )
    {
        m_requestOpaq[VPP_OUT].NumFrameMin = m_requestOpaq[VPP_OUT].NumFrameSuggested = 0;
        m_requestOpaq[VPP_OUT].Type = 0;
    }

    m_bOpaqMode[VPP_IN] = m_bOpaqMode[VPP_OUT] = false;

    //m_numUsedFilters      = 0;
    m_pipelineList.resize(0);

    VPP_CLEAN;

    return MFX_ERR_NONE;// in according with spec

} // mfxStatus VideoVPPBase::Close(void)

mfxStatus VideoVPPBase::Init(mfxVideoParam *par)
{
    mfxStatus sts  = MFX_ERR_INVALID_VIDEO_PARAM;
    mfxStatus sts_wrn = MFX_ERR_NONE;

    MFX_CHECK_NULL_PTR1( par );

    VPP_CHECK_MULTIPLE_INIT;

    /* step [0]: checking */
    sts = CheckIOPattern( par );
    MFX_CHECK_STS(sts);

    if (par->Protected)
        return MFX_ERR_INVALID_VIDEO_PARAM;

    sts = CheckFrameInfo( &(par->vpp.In), VPP_IN, m_core->GetHWType());
    MFX_CHECK_STS( sts );

    sts = CheckFrameInfo( &(par->vpp.Out), VPP_OUT, m_core->GetHWType());
    MFX_CHECK_STS(sts);

    PicStructMode picStructMode = GetPicStructMode(par->vpp.In.PicStruct, par->vpp.Out.PicStruct);
    m_bDynamicDeinterlace = (DYNAMIC_DI_PICSTRUCT_MODE == picStructMode) ? true : false;

    sts = CheckExtParam(m_core, par->ExtParam,  par->NumExtParam);
    if( MFX_WRN_INCOMPATIBLE_VIDEO_PARAM == sts || MFX_WRN_FILTER_SKIPPED == sts)
    {
        sts_wrn = sts;
        sts = MFX_ERR_NONE;
    }
    MFX_CHECK_STS(sts);

    /* step [1]: building stage of VPP pipeline */
    sts = GetPipelineList( par, m_pipelineList, true);
    MFX_CHECK_STS(sts);

    // opaque configuration rules:
    // (1) in case of OPAQ request VPP should ignore IOPattern and use extBuffer native memory type
    // (2) VPP_IN abd VPP_OUT should be checked independently of one another
    sts = CheckOpaqMode( par, m_bOpaqMode );
    MFX_CHECK_STS( sts );

    if( m_bOpaqMode[VPP_IN] || m_bOpaqMode[VPP_OUT] )
    {
        sts = GetOpaqRequest( par, m_bOpaqMode, m_requestOpaq);
        MFX_CHECK_STS( sts );

        // VPP controls OPAQUE request.
        // will be combined with SW::CreatePipeline() to prevent multu run of QueryIOSurf()
        {
            mfxFrameAllocRequest  cntrlRequest[2];
            sts = QueryIOSurf(m_core, par, cntrlRequest);
            VPP_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
            MFX_CHECK_STS( sts );

            if( m_bOpaqMode[VPP_IN] &&
                (m_requestOpaq[VPP_IN].NumFrameMin < cntrlRequest[VPP_IN].NumFrameMin ||
                m_requestOpaq[VPP_IN].NumFrameSuggested < cntrlRequest[VPP_IN].NumFrameSuggested) )
            {
                return MFX_ERR_INVALID_VIDEO_PARAM;
            }

            if( m_bOpaqMode[VPP_OUT] &&
                (m_requestOpaq[VPP_OUT].NumFrameMin < cntrlRequest[VPP_OUT].NumFrameMin ||
                m_requestOpaq[VPP_OUT].NumFrameSuggested < cntrlRequest[VPP_OUT].NumFrameSuggested) )
            {
                return MFX_ERR_INVALID_VIDEO_PARAM;
            }
        }
    }

    sts = InternalInit(par);
    if (MFX_WRN_INCOMPATIBLE_VIDEO_PARAM == sts || MFX_WRN_FILTER_SKIPPED == sts)
    {
        sts_wrn = sts;
        sts = MFX_ERR_NONE;
    }
    MFX_CHECK_STS( sts );

    /* save init params to prevent core crash */
    m_errPrtctState.In  = par->vpp.In;
    m_errPrtctState.Out = par->vpp.Out;
    m_errPrtctState.IOPattern  = par->IOPattern;
    m_errPrtctState.AsyncDepth = par->AsyncDepth;

    sts = GetCompositionEnabledStatus(par);
    if (sts == MFX_ERR_NONE)
        m_errPrtctState.isCompositionModeEnabled = true;
    else
        m_errPrtctState.isCompositionModeEnabled = false;

    m_InitState = m_errPrtctState; // Save params on init

    m_stat.NumCachedFrame = 0;
    m_stat.NumFrame       = 0;

    VPP_INIT_SUCCESSFUL;

    if( MFX_ERR_NONE != sts_wrn )
    {
        return sts_wrn;
    }

    bool bCorrectionEnable = false;
    sts = CheckPlatformLimitations(m_core, *par, bCorrectionEnable);

    if (MFX_ERR_UNSUPPORTED == sts)
    {
        sts = MFX_ERR_INVALID_VIDEO_PARAM;
    }

    return sts;

} // mfxStatus VideoVPPBase::Init(mfxVideoParam *par)


mfxStatus VideoVPPBase::VppFrameCheck(mfxFrameSurface1 *in, mfxFrameSurface1 *out, mfxExtVppAuxData *,
                                    MFX_ENTRY_POINT [], mfxU32 &)
{

    //printf("\nVideoVPPBase::VppFrameCheck()\n"); fflush(stdout);

    mfxStatus sts = MFX_ERR_NONE;
    mfxStatus stsReadinessPipeline = MFX_ERR_NONE; stsReadinessPipeline;

    /* [IN] */
    // it is end of stream procedure if(NULL == in)

    if( NULL == out )
    {
        return MFX_ERR_NULL_PTR;
    }

    if (out->Data.Locked)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    // need for SW due to algorithm processing
    mfxU16  realOutPicStruct = out->Info.PicStruct; realOutPicStruct;

    /* *************************************** */
    /*              check info                 */
    /* *************************************** */
    if (in)
    {
        sts = CheckInputPicStruct( in->Info.PicStruct );
        MFX_CHECK_STS(sts);

        /* we have special case for composition:
         * if composition enabled sub stream's picture (WxH)
         * can be less than primary stream (WxH)
         * So, do check frame info only if composition is not enabled */
        if (m_errPrtctState.isCompositionModeEnabled == false)
        {
            sts = CompareFrameInfo( &(in->Info), &(m_errPrtctState.In));
            MFX_CHECK_STS(sts);
        }

        sts = CheckCropParam( &(in->Info) );
        MFX_CHECK_STS( sts );
    }

    sts = CompareFrameInfo( &(out->Info), &(m_errPrtctState.Out));
    MFX_CHECK_STS(sts);

    sts = CheckCropParam( &(out->Info) );
    MFX_CHECK_STS( sts );

    return sts;
} // mfxStatus VideoVPPBase::VppFrameCheck(...)


mfxStatus VideoVPPBase::QueryIOSurf(VideoCORE* core, mfxVideoParam *par, mfxFrameAllocRequest *request)
{
    mfxStatus mfxSts;

    MFX_CHECK_NULL_PTR2(par, request);

    mfxSts = CheckFrameInfo( &(par->vpp.In), VPP_IN, core->GetHWType());
    MFX_CHECK_STS( mfxSts );

    mfxSts = CheckFrameInfo( &(par->vpp.Out), VPP_OUT, core->GetHWType());
    MFX_CHECK_STS( mfxSts );

    // make sense?
    //mfxSts = CheckExtParam(par->ExtParam,  par->NumExtParam);
    //if( MFX_WRN_INCOMPATIBLE_VIDEO_PARAM == mfxSts )
    //{
    //    mfxSts = MFX_ERR_NONE;
    //    //bWarningIncompatible = true;
    //}
    //MFX_CHECK_STS(mfxSts);

    //PicStructMode m_picStructSupport = GetTypeOfInitPicStructSupport(par->vpp.In.PicStruct, par->vpp.Out.PicStruct);

    // default settings
    // VPP_IN
    request[VPP_IN].Info = par->vpp.In;
    request[VPP_IN].NumFrameMin = 1;
    request[VPP_IN].NumFrameSuggested = 1;

    //VPP_OUT
    request[VPP_OUT].Info = par->vpp.Out;
    request[VPP_OUT].NumFrameMin = 1;
    request[VPP_OUT].NumFrameSuggested = 1;

    /* correction */
    std::vector<mfxU32> pipelineList;
    //mfxU32 lenList;

    mfxSts = GetPipelineList( par, pipelineList, true );
    MFX_CHECK_STS( mfxSts );


    mfxU16 framesCountMin[2];
    mfxU16 framesCountSuggested[2];
    mfxSts = GetExternalFramesCount(core, par, &pipelineList[0], (mfxU32)pipelineList.size(), framesCountMin, framesCountSuggested);
    MFX_CHECK_STS( mfxSts );

    request[VPP_IN].NumFrameMin  = framesCountMin[VPP_IN];
    request[VPP_OUT].NumFrameMin = framesCountMin[VPP_OUT];

    request[VPP_IN].NumFrameSuggested  = framesCountSuggested[VPP_IN];
    request[VPP_OUT].NumFrameSuggested = framesCountSuggested[VPP_OUT];

    if( MFX_PLATFORM_HARDWARE == core->GetPlatformType() )
    {
        mfxFrameAllocRequest hwRequest[2];
        mfxSts = VideoVPPHW::QueryIOSurf(VideoVPPHW::ALL, core, par, hwRequest);

        bool bSWLib = (mfxSts == MFX_ERR_NONE) ? false : true;
        if( !bSWLib )
        {
            // suggested
            request[VPP_IN].NumFrameSuggested = MFX_MAX(request[VPP_IN].NumFrameSuggested, hwRequest[VPP_IN].NumFrameSuggested);
            request[VPP_OUT].NumFrameSuggested = MFX_MAX(request[VPP_OUT].NumFrameSuggested, hwRequest[VPP_OUT].NumFrameSuggested);

            // min
            request[VPP_IN].NumFrameMin  = MFX_MAX(request[VPP_IN].NumFrameMin, hwRequest[VPP_IN].NumFrameMin);
            request[VPP_OUT].NumFrameMin = MFX_MAX(request[VPP_OUT].NumFrameMin, hwRequest[VPP_OUT].NumFrameMin);
        }

        mfxU16 vppAsyncDepth = (0 == par->AsyncDepth) ? MFX_AUTO_ASYNC_DEPTH_VALUE : par->AsyncDepth;

        {
            // suggested
            request[VPP_IN].NumFrameSuggested  *= vppAsyncDepth;
            request[VPP_OUT].NumFrameSuggested *= vppAsyncDepth;

            // min
            request[VPP_IN].NumFrameMin  *= vppAsyncDepth;
            request[VPP_OUT].NumFrameMin *= vppAsyncDepth;
        }

        mfxSts = CheckIOPattern_AndSetIOMemTypes(par->IOPattern, &(request[VPP_IN].Type), &(request[VPP_OUT].Type), bSWLib);
        MFX_CHECK_STS(mfxSts);
        return (bSWLib)? MFX_ERR_UNSUPPORTED : MFX_ERR_NONE;
    }
    return MFX_ERR_NONE;

} // mfxStatus VideoVPPBase::QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *request, const mfxU32 adapterNum)

mfxStatus VideoVPPBase::GetVPPStat(mfxVPPStat *stat)
{
    MFX_CHECK_NULL_PTR1(stat);

    VPP_CHECK_NOT_INITIALIZED;

    if( 0 == m_pipelineList.size() ) return MFX_ERR_NOT_INITIALIZED;

    stat->NumCachedFrame = m_stat.NumCachedFrame;
    stat->NumFrame       = m_stat.NumFrame;

    return MFX_ERR_NONE;

} // mfxStatus VideoVPPBase::GetVPPStat(mfxVPPStat *stat)

mfxStatus VideoVPPBase::GetVideoParam(mfxVideoParam *par)
{
    MFX_CHECK_NULL_PTR1( par )

    par->vpp.In  = m_errPrtctState.In;
    par->vpp.Out = m_errPrtctState.Out;

    par->Protected  = 0;
    par->IOPattern  = m_errPrtctState.IOPattern;
    par->AsyncDepth = m_errPrtctState.AsyncDepth;

    if( NULL == par->ExtParam || 0 == par->NumExtParam)
    {
        return MFX_ERR_NONE;
    }

    for( mfxU32 i = 0; i < par->NumExtParam; i++ )
    {
        if( MFX_EXTBUFF_VPP_DOUSE == par->ExtParam[i]->BufferId )
        {
            mfxExtVPPDoUse* pVPPHint = (mfxExtVPPDoUse*)(par->ExtParam[i]);
            mfxU32 numUsedFilters = 0;

            for( mfxU32 filterIndex = 0; filterIndex < GetNumUsedFilters(); filterIndex++ )
            {
                switch ( m_pipelineList[filterIndex] )
                {
                    case MFX_EXTBUFF_VPP_CSC:
                    case MFX_EXTBUFF_VPP_RESIZE:
                    case MFX_EXTBUFF_VPP_ITC:
                    case MFX_EXTBUFF_VPP_CSC_OUT_RGB4:
                    case MFX_EXTBUFF_VPP_CSC_OUT_A2RGB10:
                    {
                        continue;
                    }
                    case MFX_EXTBUFF_VPP_DENOISE:
                    case MFX_EXTBUFF_VPP_SCENE_ANALYSIS:
                    case MFX_EXTBUFF_VPP_PROCAMP:
                    case MFX_EXTBUFF_VPP_DETAIL:
                    case MFX_EXTBUFF_VPP_FRAME_RATE_CONVERSION:
                    case MFX_EXTBUFF_VPP_IMAGE_STABILIZATION:
                    case MFX_EXTBUFF_VPP_COMPOSITE:
                    case MFX_EXTBUFF_VPP_FIELD_PROCESSING:
                    case MFX_EXTBUFF_VPP_DEINTERLACING:
                    case MFX_EXTBUFF_VPP_DI:
                    case MFX_EXTBUFF_VPP_DI_30i60p:
                    case MFX_EXTBUFF_VPP_VIDEO_SIGNAL_INFO:
                    case MFX_EXTBUFF_VPP_MIRRORING:
#if (MFX_VERSION >= 1025)
                    case MFX_EXTBUFF_VPP_COLOR_CONVERSION:
#endif
                    {
                        if(numUsedFilters + 1 > pVPPHint->NumAlg)
                            return MFX_ERR_UNDEFINED_BEHAVIOR;

                        pVPPHint->AlgList[numUsedFilters] = m_pipelineList[filterIndex];
                        numUsedFilters++;
                        break;
                    }
                    default:
                        return MFX_ERR_UNDEFINED_BEHAVIOR;
                }
            }
        }
    }

    return MFX_ERR_NONE;

} // mfxStatus VideoVPPBase::GetVideoParam(mfxVideoParam *par)

mfxU32 VideoVPPBase::GetNumUsedFilters()
{
  return ( (mfxU32)m_pipelineList.size() );

} // mfxU32 VideoVPPBase::GetNumUsedFilters()

mfxStatus VideoVPPBase::CheckIOPattern( mfxVideoParam* par )
{
  if (0 == par->IOPattern) // IOPattern is mandatory parameter
  {
      return MFX_ERR_INVALID_VIDEO_PARAM;
  }

  if (!m_core->IsExternalFrameAllocator() && (par->IOPattern & (MFX_IOPATTERN_OUT_VIDEO_MEMORY | MFX_IOPATTERN_IN_VIDEO_MEMORY)))
  {
    return MFX_ERR_INVALID_VIDEO_PARAM;
  }

  if ((par->IOPattern & MFX_IOPATTERN_IN_VIDEO_MEMORY) &&
      (par->IOPattern & MFX_IOPATTERN_IN_SYSTEM_MEMORY))
  {
    return MFX_ERR_INVALID_VIDEO_PARAM;
  }


  if ((par->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) &&
      (par->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY))
  {
    return MFX_ERR_INVALID_VIDEO_PARAM;
  }

  return MFX_ERR_NONE;

} // mfxStatus VideoVPPBase::CheckIOPattern( mfxVideoParam* par )

mfxStatus VideoVPPBase::QueryCaps(VideoCORE * core, MfxHwVideoProcessing::mfxVppCaps& caps)
{
    mfxStatus sts = MFX_ERR_NONE;

    if(MFX_PLATFORM_HARDWARE == core->GetPlatformType() )
    {
        sts = VideoVPPHW::QueryCaps(core, caps);
        caps.uFrameRateConversion= 1; // "1" means general FRC is supported. "Interpolation" modes descibed by caps.frcCaps
        caps.uDeinterlacing      = 1; // "1" means general deinterlacing is supported
        caps.uVideoSignalInfo    = 1; // "1" means general VSI is supported
        if (sts >= MFX_ERR_NONE)
           return sts;
    }

    return MFX_ERR_UNSUPPORTED;
} // mfxStatus VideoVPPBase::QueryCaps((VideoCORE * core, MfxHwVideoProcessing::mfxVppCaps& caps)


mfxStatus VideoVPPBase::Query(VideoCORE * core, mfxVideoParam *in, mfxVideoParam *out)
{
    mfxStatus mfxSts = MFX_ERR_NONE;
    core;

    MFX_CHECK_NULL_PTR1( out );

    if( NULL == in )
    {
        memset(&out->mfx, 0, sizeof(mfxInfoMFX));
        memset(&out->vpp, 0, sizeof(mfxInfoVPP));

        // We have to set FourCC and FrameRate below to
        // pass requirements of CheckPlatformLimitation for frame interpolation

        /* vppIn */
        out->vpp.In.FourCC       = 1;
        out->vpp.In.Height       = 1;
        out->vpp.In.Width        = 1;
        out->vpp.In.PicStruct    = 1;
        out->vpp.In.FrameRateExtN = 1;
        out->vpp.In.FrameRateExtD = 1;

        /* vppOut */
        out->vpp.Out.FourCC       = 1;
        out->vpp.Out.Height       = 1;
        out->vpp.Out.Width        = 1;
        out->vpp.Out.PicStruct    = 1;
        out->vpp.Out.FrameRateExtN = 1;
        out->vpp.Out.FrameRateExtD = 1;

        out->IOPattern           = 1;
        /* protected content is not supported. check it */
        out->Protected           = 1;
        out->AsyncDepth          = 1;

        if (0 == out->NumExtParam)
        {
            out->NumExtParam     = 1;
        }
        else
        {
            // check for IS and AFRC
            out->vpp.In.FourCC         = MFX_FOURCC_NV12;
            out->vpp.Out.FourCC        = MFX_FOURCC_NV12;
            out->vpp.In.FrameRateExtN  = 30;
            out->vpp.Out.FrameRateExtN = 60;
            mfxSts = CheckPlatformLimitations(core, *out, true);
        }
        return mfxSts;
    }
    else
    {
        out->vpp       = in->vpp;

        /* [asyncDepth] section */
        out->AsyncDepth = in->AsyncDepth;

        /* [Protected] section */
        out->Protected = in->Protected;
        if( out->Protected )
        {
            out->Protected = 0;
            mfxSts = MFX_ERR_UNSUPPORTED;
        }


        /* [IOPattern] section
         * Reuse check function from QueryIOsurf
         * Zero value just skipped.
         */
        mfxU16 inPattern;
        mfxU16 outPattern;
        if (0 == in->IOPattern || MFX_ERR_NONE == CheckIOPattern_AndSetIOMemTypes(in->IOPattern, &inPattern, &outPattern, true))
        {
            out->IOPattern = in->IOPattern;
        }
        else
        {
            mfxSts = MFX_ERR_UNSUPPORTED;
            out->IOPattern = 0;
        }

        /* [ExtParam] section */
        if ((in->ExtParam == 0 && out->ExtParam != 0) ||
            (in->ExtParam != 0 && out->ExtParam == 0) ||
            (in->NumExtParam != out->NumExtParam))
        {
            mfxSts = MFX_ERR_UNDEFINED_BEHAVIOR;
        }

        if (0 != in->ExtParam)
        {
            for (int i = 0; i < in->NumExtParam; i++)
            {
                MFX_CHECK_NULL_PTR1( in->ExtParam[i] );
            }
        }

        if (0 != out->ExtParam)
        {
            for (int i = 0; i < out->NumExtParam; i++)
            {
                MFX_CHECK_NULL_PTR1(out->ExtParam[i]);
            }
        }

        if( out->NumExtParam > MAX_NUM_OF_VPP_EXT_PARAM)
        {
            out->NumExtParam = 0;
            mfxSts = MFX_ERR_UNSUPPORTED;
        }

        if( 0 == out->NumExtParam && in->ExtParam )
        {
            mfxSts = MFX_ERR_UNDEFINED_BEHAVIOR;
        }

        if (in->ExtParam && out->ExtParam && (in->NumExtParam == out->NumExtParam) )
        {
            mfxU16 i;

            // to prevent multiple initialization
            std::vector<mfxU32> filterList(1);
            bool bMultipleInitDNU    = false;
            bool bMultipleInitDOUSE  = false;

            for (i = 0; i < out->NumExtParam; i++)
            {
                if ((in->ExtParam[i] == 0 && out->ExtParam[i] != 0) ||
                    (in->ExtParam[i] != 0 && out->ExtParam[i] == 0))
                {
                    //mfxSts = MFX_ERR_UNDEFINED_BEHAVIOR;
                    mfxSts = MFX_ERR_NULL_PTR;
                    continue; // stop working with ExtParam[i]
                }

                if (in->ExtParam[i] && out->ExtParam[i])
                {
                    if (in->ExtParam[i]->BufferId != out->ExtParam[i]->BufferId)
                    {
                        mfxSts = MFX_ERR_UNDEFINED_BEHAVIOR;
                        continue; // stop working with ExtParam[i]
                    }

                    if (in->ExtParam[i]->BufferSz != out->ExtParam[i]->BufferSz)
                    {
                        mfxSts = MFX_ERR_UNDEFINED_BEHAVIOR;
                        continue; // stop working with ExtParam[i]
                    }

                    // --------------------------------
                    // analysis of configurable filters
                    // --------------------------------
                    if( IsConfigurable( in->ExtParam[i]->BufferId ) )
                    {
                        if( IsFilterFound(&filterList[0], (mfxU32)filterList.size(), in->ExtParam[i]->BufferId) )
                        {
                            mfxSts = MFX_ERR_UNDEFINED_BEHAVIOR;
                        }
                        else
                        {
                            memcpy_s((uint8_t*)out->ExtParam[i], (int)GetConfigSize(in->ExtParam[i]->BufferId), (uint8_t*)in->ExtParam[i], (int)GetConfigSize(in->ExtParam[i]->BufferId));

                            mfxStatus extSts = ExtendedQuery(core, in->ExtParam[i]->BufferId, out->ExtParam[i]);
                            if( MFX_ERR_NONE != extSts )
                            {
                                mfxSts = extSts;
                            }

                            filterList.push_back(in->ExtParam[i]->BufferId);
                        }

                        continue; // stop working with ExtParam[i]
                    }

                    // --------------------------------
                    // analysis of DONOTUSE structure
                    // --------------------------------
                    else if( MFX_EXTBUFF_VPP_DONOTUSE == in->ExtParam[i]->BufferId )
                    {
                        if( bMultipleInitDNU )
                        {
                            mfxSts = MFX_ERR_UNDEFINED_BEHAVIOR;
                            continue;// stop working with ExtParam[i]
                        }

                        bMultipleInitDNU = true;

                        // deep analysis
                        //--------------------------------------
                        {
                            mfxExtVPPDoNotUse*   extDoNotUseIn  = (mfxExtVPPDoNotUse*)in->ExtParam[i];
                            mfxExtVPPDoNotUse*   extDoNotUseOut = (mfxExtVPPDoNotUse*)out->ExtParam[i];

                            if(extDoNotUseIn->NumAlg != extDoNotUseOut->NumAlg)
                            {
                                mfxSts = MFX_ERR_UNDEFINED_BEHAVIOR;
                                continue; // stop working with ExtParam[i]
                            }

                            if( 0 == extDoNotUseIn->NumAlg )
                            {
                                extDoNotUseIn->NumAlg = 0;

                                mfxSts = MFX_ERR_UNDEFINED_BEHAVIOR;
                                continue; // stop working with ExtParam[i]
                            }
                            if(extDoNotUseIn->NumAlg > 4)
                            {
                                extDoNotUseIn->NumAlg = 0;
                                mfxSts = MFX_ERR_UNSUPPORTED;
                                continue; // stop working with ExtParam[i]
                            }

                            if( NULL == extDoNotUseOut->AlgList || NULL == extDoNotUseIn->AlgList )
                            {
                                mfxSts = MFX_ERR_UNDEFINED_BEHAVIOR;
                                continue; // stop working with ExtParam[i]
                            }

                            for( mfxU32 algIdx = 0; algIdx < extDoNotUseIn->NumAlg; algIdx++ )
                            {
                                // app must turn off filter once only
                                if( IsFilterFound( extDoNotUseIn->AlgList, algIdx, extDoNotUseIn->AlgList[algIdx] ) )
                                {
                                    mfxSts = MFX_ERR_UNSUPPORTED;
                                    continue; // stop working with ExtParam[i]
                                }
                                extDoNotUseOut->AlgList[algIdx] = extDoNotUseIn->AlgList[algIdx];
                            }
                            extDoNotUseOut->NumAlg = extDoNotUseIn->NumAlg;

                            for( mfxU32 extParIdx = 0; extParIdx < in->NumExtParam; extParIdx++ )
                            {
                                // configured via extended parameters filter should not be disabled
                                if ( IsFilterFound( extDoNotUseIn->AlgList, extDoNotUseIn->NumAlg, in->ExtParam[extParIdx]->BufferId ) )
                                {
                                    mfxU32 filterIdx = GetFilterIndex( extDoNotUseIn->AlgList, extDoNotUseIn->NumAlg, in->ExtParam[extParIdx]->BufferId );
                                    extDoNotUseIn->AlgList[filterIdx] = 0;
                                    mfxSts = MFX_ERR_UNDEFINED_BEHAVIOR;
                                    continue; // stop working with ExtParam[i]
                                }
                            }
                        }
                    }

                    // --------------------------------
                    // analysis of DOUSE structure
                    // --------------------------------
                    else if( MFX_EXTBUFF_VPP_DOUSE == in->ExtParam[i]->BufferId )
                    {
                        if( bMultipleInitDOUSE )
                        {
                            mfxSts = MFX_ERR_UNDEFINED_BEHAVIOR;
                            continue;// stop working with ExtParam[i]
                        }

                        bMultipleInitDOUSE = true;

                        // deep analysis
                        //--------------------------------------
                        {
                            mfxExtVPPDoUse*   extDoUseIn  = (mfxExtVPPDoUse*)in->ExtParam[i];
                            mfxExtVPPDoUse*   extDoUseOut = (mfxExtVPPDoUse*)out->ExtParam[i];

                            if(extDoUseIn->NumAlg != extDoUseOut->NumAlg)
                            {
                                mfxSts = MFX_ERR_UNDEFINED_BEHAVIOR;
                                continue; // stop working with ExtParam[i]
                            }

                            if( 0 == extDoUseIn->NumAlg )
                            {
                                extDoUseIn->NumAlg = 0;

                                mfxSts = MFX_ERR_UNDEFINED_BEHAVIOR;
                                continue; // stop working with ExtParam[i]
                            }

                            if( NULL == extDoUseOut->AlgList || NULL == extDoUseIn->AlgList )
                            {
                                mfxSts = MFX_ERR_UNDEFINED_BEHAVIOR;
                                continue; // stop working with ExtParam[i]
                            }

                            for( mfxU32 algIdx = 0; algIdx < extDoUseIn->NumAlg; algIdx++ )
                            {
                                if( !CheckDoUseCompatibility( extDoUseIn->AlgList[algIdx] ) )
                                {
                                    mfxSts = MFX_ERR_UNSUPPORTED;
                                    continue; // stop working with ExtParam[i]
                                }

                                // app must turn off filter once only
                                if( IsFilterFound( extDoUseIn->AlgList, algIdx, extDoUseIn->AlgList[algIdx] ) )
                                {
                                    mfxSts = MFX_ERR_UNSUPPORTED;
                                    continue; // stop working with ExtParam[i]
                                }

                                if(MFX_EXTBUFF_VPP_COMPOSITE == extDoUseIn->AlgList[algIdx])
                                {
                                    mfxSts = MFX_ERR_INVALID_VIDEO_PARAM;
                                    continue; // stop working with ExtParam[i]
                                }

                                if(MFX_EXTBUFF_VPP_FIELD_PROCESSING == extDoUseIn->AlgList[algIdx])
                                {
                                    /* NOTE:
                                     * It's legal to use DOUSE for field processing,
                                     * but application must attach appropriate ext buffer to mfxFrameData for each input surface
                                     */
                                    //mfxSts = MFX_ERR_UNSUPPORTED;
                                    //continue;
                                }
                                extDoUseOut->AlgList[algIdx] = extDoUseIn->AlgList[algIdx];
                            }
                            extDoUseOut->NumAlg = extDoUseIn->NumAlg;
                        }
                        //--------------------------------------
                    }
                    else if( MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION == in->ExtParam[i]->BufferId )
                    {
                        // No specific checks for Opaque ext buffer at the moment.
                    }
                    else
                    {
                        out->ExtParam[i]->BufferId = 0;
                        mfxSts = MFX_ERR_UNSUPPORTED;

                    }// if( MFX_EXTBUFF_VPP_XXX == in->ExtParam[i]->BufferId )

                } // if(in->ExtParam[i] && out->ExtParam[i])

            } //  for (i = 0; i < out->NumExtParam; i++)

        } // if (in->ExtParam && out->ExtParam && (in->NumExtParam == out->NumExtParam) )

        if ( out->vpp.In.FourCC  != MFX_FOURCC_P010 &&
             out->vpp.In.FourCC  != MFX_FOURCC_P210 &&
             out->vpp.Out.FourCC == MFX_FOURCC_A2RGB10 ){
            if( out->vpp.In.FourCC )
            {
                out->vpp.In.FourCC = 0;
                mfxSts = MFX_ERR_UNSUPPORTED;
            }
        }

        if ( out->vpp.In.FourCC  == MFX_FOURCC_P010 &&
             out->vpp.Out.FourCC != MFX_FOURCC_A2RGB10 &&
             out->vpp.Out.FourCC != MFX_FOURCC_NV12 &&
             out->vpp.Out.FourCC != MFX_FOURCC_P010 &&
             out->vpp.Out.FourCC != MFX_FOURCC_P210 &&
             out->vpp.Out.FourCC != MFX_FOURCC_RGB4){
            if( out->vpp.In.FourCC )
            {
                out->vpp.In.FourCC = 0;
                mfxSts = MFX_ERR_UNSUPPORTED;
            }
        }

        /* [IN VPP] data */
        if( out->vpp.In.FourCC != MFX_FOURCC_YV12 &&
            out->vpp.In.FourCC != MFX_FOURCC_NV12 &&
            out->vpp.In.FourCC != MFX_FOURCC_YUY2 &&
#if (MFX_VERSION >= MFX_VERSION_NEXT)
            out->vpp.In.FourCC != MFX_FOURCC_RGB565 &&
#endif
            out->vpp.In.FourCC != MFX_FOURCC_RGB4 &&
            out->vpp.In.FourCC != MFX_FOURCC_P010 &&
            out->vpp.In.FourCC != MFX_FOURCC_UYVY &&
            out->vpp.In.FourCC != MFX_FOURCC_P210 &&
            out->vpp.In.FourCC != MFX_FOURCC_AYUV)
        {
            if( out->vpp.In.FourCC )
            {
                out->vpp.In.FourCC = 0;
                mfxSts = MFX_ERR_UNSUPPORTED;
            }
        }

        if (out->vpp.In.PicStruct != MFX_PICSTRUCT_PROGRESSIVE &&
            out->vpp.In.PicStruct != MFX_PICSTRUCT_FIELD_TFF   &&
            out->vpp.In.PicStruct != MFX_PICSTRUCT_FIELD_BFF   &&
            out->vpp.In.PicStruct != MFX_PICSTRUCT_FIELD_SINGLE &&
            out->vpp.In.PicStruct != MFX_PICSTRUCT_UNKNOWN)
        {

            if( out->vpp.In.PicStruct )
            {
                out->vpp.In.PicStruct = 0;
                mfxSts = MFX_ERR_UNSUPPORTED;
            }
        }

        if ( !(out->vpp.In.FrameRateExtN * out->vpp.In.FrameRateExtD) &&
            (out->vpp.In.FrameRateExtN + out->vpp.In.FrameRateExtD) )
        {
            out->vpp.In.FrameRateExtN = 0;
            out->vpp.In.FrameRateExtD = 0;
            mfxSts = MFX_ERR_UNSUPPORTED;
        }

        if( out->vpp.In.Width )
        {
            if ( (out->vpp.In.Width & 15 ) != 0 )
            {
                out->vpp.In.Width = 0;
                mfxSts = MFX_ERR_UNSUPPORTED;
            }
        }

        if (out->vpp.In.Height)
        {
            if ((out->vpp.In.Height  & 15) !=0 )
            {
                out->vpp.In.Height = 0;
                mfxSts = MFX_ERR_UNSUPPORTED;
            }
        }

        /* [OUT VPP] data */
        if( out->vpp.Out.FourCC != MFX_FOURCC_YV12 &&
            out->vpp.Out.FourCC != MFX_FOURCC_NV12 &&
            out->vpp.Out.FourCC != MFX_FOURCC_YUY2 &&
            out->vpp.Out.FourCC != MFX_FOURCC_RGB4 &&
            out->vpp.Out.FourCC != MFX_FOURCC_P010 &&
            out->vpp.Out.FourCC != MFX_FOURCC_P210 &&
            out->vpp.Out.FourCC != MFX_FOURCC_AYUV &&
            out->vpp.Out.FourCC != MFX_FOURCC_A2RGB10 )
        {
            out->vpp.Out.FourCC = 0;
            mfxSts = MFX_ERR_UNSUPPORTED;
        }

        if (out->vpp.Out.PicStruct != MFX_PICSTRUCT_PROGRESSIVE &&
            out->vpp.Out.PicStruct != MFX_PICSTRUCT_FIELD_TFF   &&
            out->vpp.Out.PicStruct != MFX_PICSTRUCT_FIELD_BFF   &&
            out->vpp.Out.PicStruct != MFX_PICSTRUCT_FIELD_SINGLE &&
            out->vpp.Out.PicStruct != MFX_PICSTRUCT_UNKNOWN)
        {
            if(out->vpp.Out.PicStruct)
            {
                out->vpp.Out.PicStruct = 0;
                mfxSts = MFX_ERR_UNSUPPORTED;
            }
        }

        if ( !(out->vpp.Out.FrameRateExtN * out->vpp.Out.FrameRateExtD) &&
            (out->vpp.Out.FrameRateExtN + out->vpp.Out.FrameRateExtD))
        {
            out->vpp.Out.FrameRateExtN = 0;
            out->vpp.Out.FrameRateExtD = 0;
            mfxSts = MFX_ERR_UNSUPPORTED;
        }

        if ( out->vpp.Out.Width )
        {
            if ( (out->vpp.Out.Width & 15 ) != 0 )
            {
                mfxSts = MFX_ERR_UNSUPPORTED;
            }
        }

        if( out->vpp.Out.Height )
        {
            if ((out->vpp.Out.Height  & 15) !=0)
            {
                out->vpp.Out.Height = 0;
                mfxSts = MFX_ERR_UNSUPPORTED;
            }
        }

        MFX_CHECK_STS(mfxSts);

        //-------------------------------------------------
        // FRC, IS and similar enhancement algorithms
        // special "interface" to signal on application level about support/unsupport ones
        //-------------------------------------------------
        bool bCorrectionEnable = true;
        mfxSts = CheckPlatformLimitations(core, *out, bCorrectionEnable);
        //-------------------------------------------------

        mfxStatus   hwQuerySts = MFX_ERR_NONE,
                    swQuerySts = MFX_ERR_NONE;

        if(MFX_PLATFORM_HARDWARE == core->GetPlatformType())
        {
            // HW VPP checking
            hwQuerySts = VideoVPPHW::Query(core, out);

            // Statuses returned by Init differ in several cases from Query
            if (MFX_ERR_INVALID_VIDEO_PARAM == hwQuerySts || MFX_ERR_UNSUPPORTED == hwQuerySts)
            {
                return MFX_ERR_UNSUPPORTED;
            }

            if (MFX_WRN_INCOMPATIBLE_VIDEO_PARAM == hwQuerySts || MFX_WRN_FILTER_SKIPPED == hwQuerySts)
            {
                return hwQuerySts;
            }

            if(MFX_ERR_NONE == hwQuerySts)
            {
                return mfxSts;
            }
            else
            {
                hwQuerySts = MFX_WRN_PARTIAL_ACCELERATION;
            }
        }

        MFX_CHECK_STS(MFX_ERR_UNSUPPORTED);

        if (hwQuerySts != MFX_ERR_NONE)
            return hwQuerySts;
        if (swQuerySts != MFX_ERR_NONE)
            return swQuerySts;

        return mfxSts;
    }//else
} // mfxStatus VideoVPPBase::Query(VideoCORE *core, mfxVideoParam *in, mfxVideoParam *out)


mfxStatus VideoVPPBase::Reset(mfxVideoParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;

    MFX_CHECK_NULL_PTR1( par );

    VPP_CHECK_NOT_INITIALIZED;

    sts = CheckFrameInfo( &(par->vpp.In),  VPP_IN, m_core->GetHWType());
    MFX_CHECK_STS( sts );

    sts = CheckFrameInfo( &(par->vpp.Out), VPP_OUT, m_core->GetHWType());
    MFX_CHECK_STS(sts);

    //-----------------------------------------------------
    // specific check for Reset()
    if( m_InitState.In.PicStruct != par->vpp.In.PicStruct || m_InitState.Out.PicStruct != par->vpp.Out.PicStruct)
    {
        return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;
    }

    /* IOPattern check */
    if( m_InitState.IOPattern != par->IOPattern )
    {
        return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;
    }

    if (par->Protected)
        return MFX_ERR_INVALID_VIDEO_PARAM;

    /* AsyncDepth */
    if( m_InitState.AsyncDepth < par->AsyncDepth )
    {
        return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;
    }

    /* in general, in/out resolution should be <= m_initParam.resolution */
    if( (par->vpp.In.Width  > m_InitState.In.Width)  || (par->vpp.In.Height  > m_InitState.In.Height) ||
        (par->vpp.Out.Width > m_InitState.Out.Width) || (par->vpp.Out.Height > m_InitState.Out.Height) )
    {
        return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;
    }
    //-----------------------------------------------------

    /* Opaque */
    if( m_bOpaqMode[VPP_IN] || m_bOpaqMode[VPP_OUT] )
    {
        bool bLocalOpaqMode[2] = {false, false};

        sts = CheckOpaqMode( par, bLocalOpaqMode );
        MFX_CHECK_STS( sts );

        if( bLocalOpaqMode[VPP_IN] && !m_bOpaqMode[VPP_IN] )
        {
            return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;
        }

        if( bLocalOpaqMode[VPP_OUT] && !m_bOpaqMode[VPP_OUT] )
        {
            return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;
        }

        if( bLocalOpaqMode[VPP_IN] || bLocalOpaqMode[VPP_OUT] )
        {
            mfxFrameAllocRequest localOpaqRequest[2];
            sts = GetOpaqRequest( par, bLocalOpaqMode, localOpaqRequest);
            MFX_CHECK_STS( sts );

            if( bLocalOpaqMode[VPP_IN] )
            {
                if ( m_requestOpaq[VPP_IN].NumFrameMin != localOpaqRequest[VPP_IN].NumFrameMin ||
                    m_requestOpaq[VPP_IN].NumFrameSuggested != localOpaqRequest[VPP_IN].NumFrameSuggested )
                {
                    return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;
                }
            }
            if( bLocalOpaqMode[VPP_OUT] )
            {
                if ( m_requestOpaq[VPP_OUT].NumFrameMin != localOpaqRequest[VPP_OUT].NumFrameMin ||
                    m_requestOpaq[VPP_OUT].NumFrameSuggested != localOpaqRequest[VPP_OUT].NumFrameSuggested )
                {
                    return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;
                }
            }
        }
    }// Opaque

    /* save init params to prevent core crash */
    m_errPrtctState.In  = par->vpp.In;
    m_errPrtctState.Out = par->vpp.Out;
    m_errPrtctState.IOPattern  = par->IOPattern;
    m_errPrtctState.AsyncDepth = par->AsyncDepth;

    mfxStatus compSts = GetCompositionEnabledStatus(par);
    if (compSts == MFX_ERR_NONE)
        m_errPrtctState.isCompositionModeEnabled = true;
    else
        m_errPrtctState.isCompositionModeEnabled = false;

    return sts;

} // mfxStatus VideoVPPBase::Reset(mfxVideoParam *par)


mfxTaskThreadingPolicy VideoVPPBase::GetThreadingPolicy(void)
{
    return MFX_TASK_THREADING_INTRA;

} // mfxTaskThreadingPolicy VideoVPPBase::GetThreadingPolicy(void)

//---------------------------------------------------------
//                            UTILS
//---------------------------------------------------------

mfxStatus VideoVPPBase::CheckPlatformLimitations(
    VideoCORE* core,
    mfxVideoParam & param,
    bool bCorrectionEnable)
{
    std::vector<mfxU32> capsList;

    MfxHwVideoProcessing::mfxVppCaps caps;
    QueryCaps(core, caps);
    ConvertCaps2ListDoUse(caps, capsList);

    std::vector<mfxU32> pipelineList;
    bool bExtendedSupport = true;
    mfxStatus sts = GetPipelineList( &param, pipelineList, bExtendedSupport );
    MFX_CHECK_STS(sts);

    std::vector<mfxU32> supportedList;
    std::vector<mfxU32> unsupportedList;

    // compare pipelineList and capsList
    mfxStatus capsSts = GetCrossList(pipelineList, capsList, supportedList, unsupportedList);// this function could return WRN_FILTER_SKIPPED


    // check unsupported list if we need to reset ext buffer fields
    if(!unsupportedList.empty())
    {
        if (IsFilterFound(&unsupportedList[0], (mfxU32)unsupportedList.size(), MFX_EXTBUFF_VPP_IMAGE_STABILIZATION))
        {
            SetMFXISMode(param, 0);
        }
    }

    return (MFX_ERR_NONE != capsSts) ? capsSts : sts;

} // mfxStatus CheckPlatformLimitations(...)


VideoVPP_HW::VideoVPP_HW(VideoCORE *core, mfxStatus* sts)
    : VideoVPPBase(core, sts)
{
}

mfxStatus VideoVPP_HW::InternalInit(mfxVideoParam *par)
{
    mfxStatus sts = MFX_ERR_UNDEFINED_BEHAVIOR;
    CommonCORE* pCommonCore = NULL;

    bool bIsFilterSkipped  = false;
    bool isFieldProcessing = IsFilterFound(&m_pipelineList[0], (mfxU32)m_pipelineList.size(), MFX_EXTBUFF_VPP_FIELD_PROCESSING)
                          || IsFilterFound(&m_pipelineList[0], (mfxU32)m_pipelineList.size(), MFX_EXTBUFF_VPP_FIELD_WEAVING)
                          || IsFilterFound(&m_pipelineList[0], (mfxU32)m_pipelineList.size(), MFX_EXTBUFF_VPP_FIELD_SPLITTING);


    pCommonCore = QueryCoreInterface<CommonCORE>(m_core, MFXIVideoCORE_GUID);
    MFX_CHECK(pCommonCore, MFX_ERR_UNDEFINED_BEHAVIOR);

    VideoVPPHW::IOMode mode = VideoVPPHW::GetIOMode(par, m_requestOpaq);

    m_pHWVPP.reset(new VideoVPPHW(mode, m_core));

    if (isFieldProcessing)
    {
        CmDevice * device = QueryCoreInterface<CmDevice>(m_core, MFXICORECM_GUID);
        MFX_CHECK(device, MFX_ERR_UNDEFINED_BEHAVIOR);

        m_pHWVPP.get()->SetCmDevice(device);
    }
    sts = m_pHWVPP.get()->Init(par); // OK or ERR only
    if (MFX_WRN_FILTER_SKIPPED == sts)
    {
        bIsFilterSkipped = true;
        sts = MFX_ERR_NONE;
    }
    if (MFX_ERR_NONE != sts)
    {
        m_pHWVPP.reset(0);
    }
    MFX_CHECK_STS( sts );


    return (bIsFilterSkipped) ? MFX_WRN_FILTER_SKIPPED : MFX_ERR_NONE;
}

mfxStatus VideoVPP_HW::Reset(mfxVideoParam *par)
{
    mfxStatus sts = VideoVPPBase::Reset(par);
    MFX_CHECK_STS( sts );

    sts = m_pHWVPP.get()->Reset(par);


    MFX_CHECK_STS(sts);

    bool bCorrectionEnable = false;
    sts = CheckPlatformLimitations(m_core, *par, bCorrectionEnable);
    return sts;
}

mfxStatus VideoVPP_HW::Close(void)
{
    mfxStatus sts = VideoVPPBase::Close();
    m_pHWVPP.reset(0);
    return sts;

} // mfxStatus VideoVPPBase::Close(void)

mfxStatus VideoVPP_HW::GetVideoParam(mfxVideoParam *par)
{
    mfxStatus sts = VideoVPPBase::GetVideoParam(par);
    MFX_CHECK_STS( sts );

    if (m_pHWVPP.get())
    {
        return m_pHWVPP->GetVideoParams(par);
    }

    return sts;
}

mfxStatus VideoVPP_HW::VppFrameCheck(mfxFrameSurface1 *in, mfxFrameSurface1 *out, mfxExtVppAuxData *aux,
                                MFX_ENTRY_POINT pEntryPoints[], mfxU32 &numEntryPoints)
{
    mfxStatus sts = VideoVPPBase::VppFrameCheck(in, out, aux, pEntryPoints, numEntryPoints);
    MFX_CHECK_STS( sts );

    mfxStatus internalSts = m_pHWVPP.get()->VppFrameCheck( in, out, aux, pEntryPoints, numEntryPoints);

    bool isInverseTelecinedEnabled = false;
    const DdiTask* pTask = (DdiTask*)pEntryPoints[0].pParam ;

    isInverseTelecinedEnabled = IsFilterFound(&m_pipelineList[0], (mfxU32)m_pipelineList.size(), MFX_EXTBUFF_VPP_ITC);

    if (MFX_ERR_MORE_DATA == internalSts && true == isInverseTelecinedEnabled)
    {
        //internalSts = (mfxStatus) MFX_ERR_MORE_DATA_SUBMIT_TASK;
    }

    if( out && (MFX_ERR_NONE == internalSts || MFX_ERR_MORE_SURFACE == internalSts) )
    {
        sts = PassThrough(NULL != in ? &(in->Info) : NULL, &(out->Info), pTask->taskIndex);
        //MFX_CHECK_STS( sts );
    }

    return (MFX_ERR_NONE == internalSts) ? sts : internalSts;
}

mfxStatus VideoVPP_HW::PassThrough(mfxFrameInfo* In, mfxFrameInfo* Out, mfxU32 taskIndex)
{
    if( In ) // no delay
    {
        mfxStatus sts;

        Out->AspectRatioH = In->AspectRatioH;
        Out->AspectRatioW = In->AspectRatioW;
        Out->PicStruct    = UpdatePicStruct( In->PicStruct, Out->PicStruct, m_bDynamicDeinterlace, sts, taskIndex );

        m_errPrtctState.Deffered.AspectRatioH = Out->AspectRatioH;
        m_errPrtctState.Deffered.AspectRatioW = Out->AspectRatioW;
        m_errPrtctState.Deffered.PicStruct    = Out->PicStruct;

        // not "pass through" process. Frame Rates from Init.
        Out->FrameRateExtN = m_errPrtctState.Out.FrameRateExtN;
        Out->FrameRateExtD = m_errPrtctState.Out.FrameRateExtD;

        //return MFX_ERR_NONE;
        return sts;
    }
    else
    {
        if ( MFX_PICSTRUCT_UNKNOWN == Out->PicStruct && m_bDynamicDeinterlace )
        {
            // Fix for case when app retrievs cached frames from ADI3->60 and output surf has unknown picstruct
            Out->PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
        }
        // in case of HW_VPP in==NULL means ERROR due to absence of delayed frames and should be processed before.
        // here we return OK only
        return MFX_ERR_NONE;
    }
} //  mfxStatus VideoVPPBase::PassThrough(mfxFrameInfo* In, mfxFrameInfo* Out)

mfxStatus VideoVPP_HW::RunFrameVPP(mfxFrameSurface1* , mfxFrameSurface1* , mfxExtVppAuxData *)
{
    return MFX_ERR_NONE;
}


#endif // MFX_ENABLE_VPP
/* EOF */

