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
#include "mfxmvc.h"

#include "mfx_vpp_utils.h"
#include "mfx_vpp_main.h"
#include "mfx_vpp_sw.h"

#include "mfx_vpp_mvc.h"


using namespace MfxVideoProcessing;

/* ******************************************************************** */
/*                 Main (High Level) Class of MSDK VPP                  */
/* ******************************************************************** */


mfxStatus VideoVPPMain::Query(VideoCORE* core, mfxVideoParam *in, mfxVideoParam *out)
{
    return ImplementationMvc::Query(core, in, out);
} // mfxStatus VideoVPPMain::Query(VideoCORE* core, mfxVideoParam *in, mfxVideoParam *out)


mfxStatus VideoVPPMain::QueryIOSurf(VideoCORE* core, mfxVideoParam *par, mfxFrameAllocRequest *request)
{

    return ImplementationMvc::QueryIOSurf(core, par, request);

} // mfxStatus VideoVPPMain::QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *request, const mfxU32 adapterNum)


VideoVPPMain::VideoVPPMain(VideoCORE *core, mfxStatus* sts )
: m_core( core )
{
    m_bOpaqMode[VPP_IN] = m_bOpaqMode[VPP_OUT] = false;

    *sts   = MFX_ERR_NONE;

} // VideoVPPMain::VideoVPPMain(VideoCORE *core, mfxStatus* sts )


VideoVPPMain::~VideoVPPMain()
{
    Close();

} // VideoVPPMain::~VideoVPPMain()


mfxStatus VideoVPPMain::Init(mfxVideoParam *par)
{
    MFX_CHECK_NULL_PTR1( par );
    mfxStatus internalSts = MFX_ERR_NONE;

    if( m_impl.get() )
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    std::unique_ptr<VideoVPP> impl(
              (VideoVPP*) new ImplementationMvc(m_core));

    mfxStatus mfxSts = impl->Init(par);
    MFX_CHECK(
        mfxSts == MFX_ERR_NONE                 ||
        mfxSts == MFX_WRN_PARTIAL_ACCELERATION ||
        mfxSts == MFX_WRN_FILTER_SKIPPED       ||
        mfxSts == MFX_WRN_INCOMPATIBLE_VIDEO_PARAM,
        mfxSts);

    /*if( MFX_WRN_PARTIAL_ACCELERATION == mfxSts)
    {
        isPartialAcceleration = true;
    }
    else if(MFX_WRN_FILTER_SKIPPED == mfxSts)
    {
        isFilterSkipped = true;
    }
    else if(MFX_WRN_INCOMPATIBLE_VIDEO_PARAM == mfxSts)
    {
        isIncompatibleParam = true;
    }*/

    internalSts = mfxSts;

    // opaque configuration    
    mfxSts = CheckOpaqMode( par, m_bOpaqMode );
    MFX_CHECK_STS( mfxSts );

    if( m_bOpaqMode[VPP_IN] || m_bOpaqMode[VPP_OUT] )
    {
        mfxExtOpaqueSurfaceAlloc *pOpaqAlloc = (mfxExtOpaqueSurfaceAlloc *)GetExtendedBufferInternal(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);

        mfxFrameAllocRequest  requestOpaq;

        if( m_bOpaqMode[VPP_IN] )
        {
            requestOpaq.Info = par->vpp.In;
            requestOpaq.NumFrameMin = requestOpaq.NumFrameSuggested = (mfxU16)pOpaqAlloc->In.NumSurface;
            requestOpaq.Type = (mfxU16)(pOpaqAlloc->In.Type|MFX_MEMTYPE_FROM_VPPIN);

            mfxSts = m_core->AllocFrames(&(requestOpaq),
                                         &(m_responseOpaq[VPP_IN]),
                                         pOpaqAlloc->In.Surfaces,
                                         pOpaqAlloc->In.NumSurface);
            MFX_CHECK_STS( mfxSts );
        }

        if( m_bOpaqMode[VPP_OUT] )
        {
            requestOpaq.Info = par->vpp.Out;
            requestOpaq.NumFrameMin = requestOpaq.NumFrameSuggested = (mfxU16)pOpaqAlloc->Out.NumSurface;
            requestOpaq.Type = (mfxU16)(pOpaqAlloc->Out.Type|MFX_MEMTYPE_FROM_VPPOUT);

            mfxSts = m_core->AllocFrames(&(requestOpaq),
                                         &(m_responseOpaq[VPP_OUT]),
                                         pOpaqAlloc->Out.Surfaces,
                                         pOpaqAlloc->Out.NumSurface);

            MFX_CHECK_STS( mfxSts );

        }
    }
    
    m_impl = std::move(impl);

    return (MFX_ERR_NONE == mfxSts) ? internalSts : mfxSts;

} // mfxStatus VideoVPPMain::Init(mfxVideoParam *par)


mfxStatus VideoVPPMain::Close( void )
{
    if( !m_impl.get() )
    {
        return MFX_ERR_NONE;
    }

    m_impl->Close();
    m_impl.reset();

    /* opaque processing */
    if( m_bOpaqMode[VPP_IN] )
    {
        m_core->FreeFrames( &(m_responseOpaq[VPP_IN]) );
        m_responseOpaq[VPP_IN].NumFrameActual  = 0;
    }
    if( m_bOpaqMode[VPP_OUT] )
    {
        m_core->FreeFrames( &(m_responseOpaq[VPP_OUT]) );
        m_responseOpaq[VPP_OUT].NumFrameActual = 0;
    }

    m_bOpaqMode[VPP_IN] = m_bOpaqMode[VPP_OUT] = false;

    return MFX_ERR_NONE;

} // mfxStatus VideoVPPMain::Close( void )

mfxTaskThreadingPolicy VideoVPPMain::GetThreadingPolicy(void)
{
    return MFX_TASK_THREADING_INTRA;

} // mfxTaskThreadingPolicy VideoVPPMain::GetThreadingPolicy(void)


mfxStatus VideoVPPMain::VppFrameCheck(mfxFrameSurface1 *in,
                                      mfxFrameSurface1 *out,
                                      mfxExtVppAuxData *aux,
                                      MFX_ENTRY_POINT pEntryPoint[],
                                      mfxU32 &numEntryPoints)
{
    MFX_CHECK_NULL_PTR1( out );

    if( !m_impl.get() )
    {
        return MFX_ERR_NOT_INITIALIZED;
    }

    if (in && m_bOpaqMode[VPP_IN])
    {
        if (in->Data.Y || in->Data.U || in->Data.V || in->Data.A || in->Data.MemId)
            return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    if (out && m_bOpaqMode[VPP_OUT])
    {
        if (out->Data.Y || out->Data.U || out->Data.V || out->Data.A || out->Data.MemId)
            return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    mfxFrameSurface1* pInputNative  =  GetNativeSurface(in, VPP_IN);
    mfxFrameSurface1* pOutputNative =  GetNativeSurface(out, VPP_OUT);

    // to prevent Opaque issue
    if( (in && (NULL == pInputNative)) || (out && (NULL == pOutputNative)) )
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }


    mfxStatus mfxSts = m_impl->VppFrameCheck( pInputNative, pOutputNative, aux, pEntryPoint, numEntryPoints );


    // Check() updated out:frameInfo & out:Data. so we need sync in case of output opaq surface
    if( m_bOpaqMode[VPP_OUT] && pOutputNative )
    {
        if( (MFX_ERR_NONE == mfxSts) ||
            (MFX_ERR_MORE_DATA == mfxSts) ||
            ((mfxStatus)MFX_ERR_MORE_DATA_SUBMIT_TASK == mfxSts) ||
            (MFX_ERR_MORE_SURFACE == mfxSts) )
        {
            out->Data.FrameOrder = pOutputNative->Data.FrameOrder;
            out->Data.TimeStamp  = pOutputNative->Data.TimeStamp;
            out->Info = pOutputNative->Info;
        }
    }

    return mfxSts;

} // mfxStatus VideoVPPMain::VppFrameCheck(...)


mfxStatus VideoVPPMain::RunFrameVPP(mfxFrameSurface1 *in, mfxFrameSurface1 *out, mfxExtVppAuxData *aux)
{

    mfxFrameSurface1* pNativeInput  = GetNativeSurface(in,   VPP_IN);
    mfxFrameSurface1* pNativeOutput = GetNativeSurface(out,  VPP_OUT);

    mfxStatus mfxSts = m_impl->RunFrameVPP( 
        pNativeInput,
        pNativeOutput,
        aux);

    return mfxSts;

} // mfxStatus VideoVPPMain::RunFrameVPP(mfxFrameSurface1 *in, mfxFrameSurface1 *out, mfxExtVppAuxData *aux)

#endif // MFX_ENABLE_VPP
/* EOF */
