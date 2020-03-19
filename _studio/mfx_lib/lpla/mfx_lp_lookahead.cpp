// Copyright (c) 2014-2020 Intel Corporation
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

#include "mfx_lp_lookahead.h"
#include "mfx_h265_encode_hw.h"
#include "mfx_h265_encode_hw_utils.h"
#include "mfx_vpp_main.h"
#include "libmfx_core_interface.h"

#if defined (MFX_ENABLE_LP_LOOKAHEAD)

#define MSDK_ALIGN16(value)             (((value + 15) >> 4) << 4)
#define MSDK_MEMCPY_VAR(dstVarName, src, count) memcpy(&(dstVarName), (src), (count))

mfxStatus MfxLpLookAhead::Init(mfxVideoParam* param)
{
    mfxStatus mfxRes = MFX_ERR_NONE;
    MFX_CHECK_NULL_PTR1(param);

    if (m_bInitialized)
        return MFX_ERR_NONE;

    if (CheckDownScaling(*param))
    {
        mfxRes = CreateVpp(*param);
        MFX_CHECK_STS(mfxRes);
    }

    m_pEnc = new MFXVideoENCODEH265_HW(m_core, &mfxRes);
    MFX_CHECK_NULL_PTR1(m_pEnc);

    // following configuration comes from HW recommendation
    mfxVideoParam par         = *param;
    par.AsyncDepth            = 1;
    par.mfx.CodecId           = MFX_CODEC_HEVC;
    par.mfx.LowPower          = MFX_CODINGOPTION_ON;
    par.mfx.NumRefFrame       = 1;
    par.mfx.TargetUsage       = 7;
    par.mfx.RateControlMethod = MFX_RATECONTROL_CQP;
    par.mfx.CodecProfile      = MFX_PROFILE_HEVC_MAIN;
    par.mfx.CodecLevel        = MFX_LEVEL_HEVC_52;
    par.mfx.QPI               = 30;
    par.mfx.QPP               = 32;
    par.mfx.QPB               = 32;
    par.mfx.NumSlice          = 1;

    if (m_bNeedDownscale)
    {
        par.mfx.FrameInfo.CropX = 0;
        par.mfx.FrameInfo.CropY = 0;
        par.mfx.FrameInfo.CropW = m_dstWidth;
        par.mfx.FrameInfo.CropH = m_dstHeight;

        par.mfx.FrameInfo.Width = MSDK_ALIGN16(m_dstWidth);
        par.mfx.FrameInfo.Height= MSDK_ALIGN16(m_dstHeight);
    }

    //create the bitstream buffer
    memset(&m_bitstream, 0, sizeof(mfxBitstream));
    mfxU32 bufferSize = param->mfx.FrameInfo.Width * param->mfx.FrameInfo.Height * 3/2;
    m_bitstream.Data = new mfxU8[bufferSize];
    m_bitstream.MaxLength = bufferSize;

    mfxRes = m_pEnc->InitInternal(&par);
    m_bInitialized = true;

    return mfxRes;
}

mfxStatus MfxLpLookAhead::Reset(mfxVideoParam* param)
{
    (void*)param;
    // TODO: will implement it later
    return MFX_ERR_UNSUPPORTED;
}

mfxStatus MfxLpLookAhead::Close()
{
    if (!m_bInitialized)
    {
        return MFX_ERR_NOT_INITIALIZED;
    }

    if (m_bitstream.Data)
    {
        delete[]  m_bitstream.Data;
        m_bitstream.Data = nullptr;
    }
    if (m_pEnc)
    {
        delete m_pEnc;
        m_pEnc = nullptr;
    }

    DestoryVpp();

    m_bInitialized = false;
    return MFX_ERR_NONE;
}

mfxStatus MfxLpLookAhead::Submit(mfxFrameSurface1 * surface)
{
    mfxStatus mfxRes = MFX_ERR_NONE;
    MFX_CHECK_NULL_PTR1(surface);

    if (!m_bInitialized)
    {
        return MFX_ERR_NOT_INITIALIZED;
    }

    if (!m_bInExecution)
    {
        if (m_bNeedDownscale)
        {
            MFX_ENTRY_POINT entryPoint[2] = {};
            mfxU32 numEntryPoints = 1;

            mfxRes = m_pVpp->VppFrameCheck(surface, &m_dsSurface, nullptr, entryPoint, numEntryPoints);
            MFX_CHECK_STS(mfxRes);

            if (entryPoint[0].pRoutine)
            {
                mfxRes = entryPoint[0].pRoutine(entryPoint[0].pState, entryPoint[0].pParam, 0, 0);
                MFX_CHECK_STS(mfxRes);
            }

            if (entryPoint[1].pRoutine)
            {
                mfxRes = entryPoint[1].pRoutine(entryPoint[1].pState, entryPoint[1].pParam, 0, 0);
                MFX_CHECK_STS(mfxRes);
            }
        }

        m_bitstream.DataLength = 0;
        m_bitstream.DataOffset = 0;

        mfxFrameSurface1 *reordered_surface = nullptr;
        mfxEncodeInternalParams internal_params;

        mfxRes = m_pEnc->EncodeFrameCheck(
            nullptr,
            m_bNeedDownscale ? &m_dsSurface : surface,
            &m_bitstream,
            &reordered_surface,
            &internal_params,
            &m_entryPoint);
        MFX_CHECK_STS(mfxRes);

        m_bInExecution = true;
    }

    if (m_bInExecution)
        mfxRes = m_entryPoint.pRoutine(m_entryPoint.pState, m_entryPoint.pParam, 0, 0);

    if (mfxRes == MFX_ERR_NONE)
    {
        m_bInExecution = false;
        MfxHwH265Encode::Task * task = (MfxHwH265Encode::Task*)m_entryPoint.pParam;
        if (task->m_cqmHint != CQM_HINT_INVALID)
            m_cqmHint.push_back(task->m_cqmHint);
    }

    return mfxRes;
}

mfxStatus MfxLpLookAhead::Query(mfxU8 *cqmHint)
{
    MFX_CHECK_NULL_PTR1(cqmHint);

    if (!m_bInitialized)
    {
        return MFX_ERR_NOT_INITIALIZED;
    }

    if (m_cqmHint.empty())
    {
        return MFX_ERR_NOT_FOUND;
    }

    *cqmHint = m_cqmHint.front();
    m_cqmHint.pop_front();

    return MFX_ERR_NONE;
}


bool MfxLpLookAhead::CheckDownScaling(mfxVideoParam& par)
{
    if (par.mfx.FrameInfo.Width > m_dstWidth || par.mfx.FrameInfo.Height > m_dstHeight)
        m_bNeedDownscale = true;
    else
        m_bNeedDownscale = false;

    return m_bNeedDownscale;
}

mfxStatus MfxLpLookAhead::CreateVpp(mfxVideoParam& par)
{
    mfxStatus mfxRes = MFX_ERR_NONE;
    if (m_pVpp)
        return MFX_ERR_NONE;

    m_pVpp = new VideoVPPMain(m_core, &mfxRes);
    if (MFX_ERR_NONE != mfxRes)
    {
        delete m_pVpp;
        m_pVpp = nullptr;
        return mfxRes;
    }

    mfxVideoParam VPPParams = {};
    VPPParams.AsyncDepth = 1;
    VPPParams.IOPattern = par.IOPattern | MFX_IOPATTERN_OUT_VIDEO_MEMORY;
    // Input data
    MSDK_MEMCPY_VAR(VPPParams.vpp.In, &par.mfx.FrameInfo, sizeof(mfxFrameInfo));
    // Output data
    MSDK_MEMCPY_VAR(VPPParams.vpp.Out, &par.mfx.FrameInfo, sizeof(mfxFrameInfo));
    VPPParams.vpp.Out.CropX = 0;
    VPPParams.vpp.Out.CropY = 0;
    VPPParams.vpp.Out.CropW = m_dstWidth;
    VPPParams.vpp.Out.CropH = m_dstHeight;
    VPPParams.vpp.Out.Width = MSDK_ALIGN16(m_dstWidth);
    VPPParams.vpp.Out.Height = MSDK_ALIGN16(m_dstHeight);
 
    // Query number of required surfaces for VPP
    mfxFrameAllocRequest VPPRequest[2] = {};     // [0] - in, [1] - out
    mfxRes = m_pVpp->QueryIOSurf(m_core, &VPPParams, VPPRequest);
    MFX_CHECK_STS(mfxRes);

    VPPRequest[1].Type |= MFX_MEMTYPE_FROM_VPPOUT;

    m_pFrameAllocator = QueryCoreInterface<mfxFrameAllocator>(m_core, MFXIEXTERNALLOC_GUID);
    MFX_CHECK_NULL_PTR1(m_pFrameAllocator);

    mfxRes = m_pFrameAllocator->Alloc(m_pFrameAllocator->pthis, &VPPRequest[1], &m_dsResponse);
    MFX_CHECK_STS(mfxRes);

    MSDK_MEMCPY_VAR(m_dsSurface.Info, &VPPRequest[1].Info, sizeof(mfxFrameInfo));
    m_dsSurface.Data.MemId = m_dsResponse.mids[0];

    mfxExtVPPScaling scalingConfig = {};
    scalingConfig.Header.BufferId = MFX_EXTBUFF_VPP_SCALING;
    scalingConfig.Header.BufferSz = sizeof(mfxExtVPPScaling);
    scalingConfig.ScalingMode = MFX_SCALING_MODE_LOWPOWER;

    mfxExtBuffer* ExtBuffer[1];
    ExtBuffer[0] = (mfxExtBuffer*) &scalingConfig;
    VPPParams.NumExtParam = 1;
    VPPParams.ExtParam = (mfxExtBuffer**) &ExtBuffer[0];

    mfxRes = m_pVpp->Init(&VPPParams);

    return mfxRes;
}
void MfxLpLookAhead::DestoryVpp()
{
   if (m_pFrameAllocator)
        m_pFrameAllocator->Free(m_pFrameAllocator->pthis, &m_dsResponse);

    if (m_pVpp)
    {
        delete m_pVpp;
        m_pVpp = nullptr;
    }
}
#endif