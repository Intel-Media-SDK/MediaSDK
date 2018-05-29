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

#if defined (MFX_ENABLE_H265_VIDEO_ENCODE)
#include "mfx_h265_encode_hw.h"
#include <assert.h>
#include <vm_time.h>
#include "fast_copy.h"

namespace MfxHwH265Encode
{

mfxStatus CheckVideoParam(MfxVideoParam & par, ENCODE_CAPS_HEVC const & caps, bool bInit = false);
void      SetDefaults    (MfxVideoParam & par, ENCODE_CAPS_HEVC const & hwCaps);
void      InheritDefaultValues(MfxVideoParam const & parInit, MfxVideoParam &  parReset);
bool      CheckTriStateOption(mfxU16 & opt);


inline mfxStatus GetWorstSts(mfxStatus sts1, mfxStatus sts2)
{
    // WRN statuses > 0, ERR statuses < 0, ERR_NONE = 0

    mfxStatus sts_max = (std::max)(sts1, sts2),
              sts_min = (std::min)(sts1, sts2);

    return sts_min == MFX_ERR_NONE ? sts_max : sts_min;
}

mfxU16 MaxRec(MfxVideoParam const & par)
{
    return par.AsyncDepth + par.mfx.NumRefFrame + ((par.AsyncDepth > 1)? 1: 0);
}
mfxU16 NumFramesForReord(MfxVideoParam const & par)
{
    if (par.isField())
    {
        return (par.mfx.GopRefDist - 1) * 2 + (par.bFieldReord ? 1 : 0);
    }
    return par.mfx.GopRefDist - 1;
}

mfxU16 MaxRaw(MfxVideoParam const & par)
{
    return par.AsyncDepth + NumFramesForReord(par) + par.RawRef * par.mfx.NumRefFrame + ((par.AsyncDepth > 1)? 1: 0);
}

mfxU16 MaxBs(MfxVideoParam const & par)
{
    return par.AsyncDepth + ((par.AsyncDepth > 1)? 1: 0); // added buffered task between submit into ddi and query
}
mfxU32 GetBsSize(mfxFrameInfo& info)
{
    return info.Width* info.Height;
}
mfxU32 GetMinBsSize(MfxVideoParam const & par)
{
    mfxU32 size = par.mfx.FrameInfo.Width * par.mfx.FrameInfo.Height;

    mfxF64 k = 2.0;
    if (par.mfx.FrameInfo.BitDepthLuma == 10)
        k = k + 0.3;
    if (par.mfx.FrameInfo.ChromaFormat == MFX_CHROMAFORMAT_YUV422)
        k = k + 0.5;
    else if (par.mfx.FrameInfo.ChromaFormat == MFX_CHROMAFORMAT_YUV444)
        k = k + 1.5;

    size = (mfxU32)(k*size);

    if (par.mfx.RateControlMethod == MFX_RATECONTROL_CBR && !par.isSWBRC() &&
        par.mfx.FrameInfo.FrameRateExtD!= 0)
    {
        mfxU32 avg_size =  par.TargetKbps * 1000 * par.mfx.FrameInfo.FrameRateExtD / (par.mfx.FrameInfo.FrameRateExtN * 8);
        if (size < 2*avg_size)
            size = 2*avg_size;
    }

    return size;

}
mfxU16 MaxTask(MfxVideoParam const & par)
{
    return par.AsyncDepth + NumFramesForReord(par) + ((par.AsyncDepth > 1)? 1: 0);
}


/*
    Setting default value for LowPower option.
    By default LowPower is OFF (using DualPipe)

    Return value:
    MFX_WRN_INCOMPATIBLE_VIDEO_PARAM - if initial value of par.mfx.LowPower is not equal to MFX_CODINGOPTION_ON, MFX_CODINGOPTION_OFF or MFX_CODINGOPTION_UNKNOWN
    MFX_ERR_NONE - if no errors
*/
mfxStatus SetLowpowerDefault(MfxVideoParam& par)
{
    mfxStatus sts = CheckTriStateOption(par.mfx.LowPower) == false ? MFX_ERR_NONE : MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;

    if (par.mfx.LowPower == MFX_CODINGOPTION_UNKNOWN)
        par.mfx.LowPower = MFX_CODINGOPTION_OFF;

    return sts;
}

mfxStatus LoadSPSPPS(MfxVideoParam& par, mfxExtCodingOptionSPSPPS* pSPSPPS)
{
    mfxStatus sts = MFX_ERR_NONE;

    if (!pSPSPPS)
        return MFX_ERR_NONE;

    if (pSPSPPS->SPSBuffer)
    {
        BitstreamReader bs(pSPSPPS->SPSBuffer, pSPSPPS->SPSBufSize);
        sts = HeaderReader::ReadSPS(bs, par.m_sps);
        MFX_CHECK_STS(sts);

        if (pSPSPPS->PPSBuffer)
        {
            bs.Reset(pSPSPPS->PPSBuffer, pSPSPPS->PPSBufSize);
            sts = HeaderReader::ReadPPS(bs, par.m_pps);
            MFX_CHECK_STS(sts);
        }

        Zero(par.m_vps);
        ((LayersInfo&)par.m_vps) = ((LayersInfo&)par.m_sps);
        par.m_vps.video_parameter_set_id = par.m_sps.video_parameter_set_id;

        par.SyncHeadersToMfxParam();
    }
    return sts;
}
#define printCaps(arg) printf("Caps: %s %d\n", #arg, m_caps.arg);
mfxStatus MFXVideoENCODEH265_HW::InitImpl(mfxVideoParam *par)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_API, "MFXVideoENCODEH265_HW::InitImpl");
    mfxStatus sts = MFX_ERR_NONE, qsts = MFX_ERR_NONE;
    mfxCoreParam coreParams = {};
    ENCODER_TYPE ddiType = ENCODER_DEFAULT;

    if (m_core.GetCoreParam(&coreParams))
       return  MFX_ERR_UNSUPPORTED;

    sts = ExtBuffer::CheckBuffers(*par);
    MFX_CHECK_STS(sts);

    m_vpar = *par;

    sts = m_core.QueryPlatform(&m_vpar.m_platform);
    MFX_CHECK_STS(sts);

    mfxStatus lpsts = SetLowpowerDefault(m_vpar);

    m_ddi.reset( CreateHWh265Encoder(&m_core, ddiType) );
    MFX_CHECK(m_ddi.get(), MFX_ERR_UNSUPPORTED);
    GUID encoder_guid = GetGUID(m_vpar);

    sts = m_ddi->CreateAuxilliaryDevice(
        &m_core,
        encoder_guid,
        m_vpar.m_ext.HEVCParam.PicWidthInLumaSamples,
        m_vpar.m_ext.HEVCParam.PicHeightInLumaSamples);
    MFX_CHECK(MFX_SUCCEEDED(sts), MFX_ERR_DEVICE_FAILED);

    sts = m_ddi->QueryEncodeCaps(m_caps);
    MFX_CHECK(MFX_SUCCEEDED(sts), MFX_ERR_DEVICE_FAILED);


    mfxExtCodingOptionSPSPPS* pSPSPPS = ExtBuffer::Get(*par);
    sts = LoadSPSPPS(m_vpar, pSPSPPS);
    MFX_CHECK_STS(sts);

    qsts = CheckVideoParam(m_vpar, m_caps, true);
    MFX_CHECK(qsts >= MFX_ERR_NONE, qsts);

    if (qsts == MFX_ERR_NONE && lpsts != MFX_ERR_NONE)
        qsts = lpsts;

    SetDefaults(m_vpar, m_caps);

    m_vpar.bMBQPInput = IsOn(m_vpar.m_ext.CO3.EnableMBQP) &&
        (m_vpar.mfx.RateControlMethod == MFX_RATECONTROL_CQP);

    m_vpar.SyncCalculableToVideoParam();
    m_vpar.AlignCalcWithBRCParamMultiplier();

    if (!pSPSPPS || !pSPSPPS->SPSBuffer)
        m_vpar.SyncMfxToHeadersParam();

    sts = CheckHeaders(m_vpar, m_caps);
    MFX_CHECK_STS(sts);

    m_hrd.Init(m_vpar.m_sps, m_vpar.InitialDelayInKB);

    sts = m_ddi->CreateAccelerationService(m_vpar);
    MFX_CHECK(MFX_SUCCEEDED(sts), MFX_ERR_DEVICE_FAILED);

    mfxFrameAllocRequest request = {};
    request.Info = m_vpar.mfx.FrameInfo;

    if (m_vpar.IOPattern == MFX_IOPATTERN_IN_SYSTEM_MEMORY)
    {
        request.Type        = MFX_MEMTYPE_D3D_INT;
        request.NumFrameMin = MaxRaw(m_vpar);

        sts = m_raw.Alloc(&m_core, request, true);
        MFX_CHECK_STS(sts);
    }
    else if (m_vpar.IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY)
    {
        mfxExtOpaqueSurfaceAlloc& opaq = m_vpar.m_ext.Opaque;

        sts = m_core.MapOpaqueSurface(opaq.In.NumSurface, opaq.In.Type, opaq.In.Surfaces);
        MFX_CHECK_STS(sts);

        if (opaq.In.Type & MFX_MEMTYPE_SYSTEM_MEMORY)
        {
            request.Type        = MFX_MEMTYPE_D3D_INT;
            request.NumFrameMin = opaq.In.NumSurface;
            sts = m_raw.Alloc(&m_core, request, true);
        }
    }

    bool bSkipFramesMode = ((m_vpar.isSWBRC() && (m_vpar.mfx.RateControlMethod == MFX_RATECONTROL_CBR || m_vpar.mfx.RateControlMethod == MFX_RATECONTROL_VBR)) || (m_vpar.m_ext.CO2.SkipFrame == MFX_SKIPFRAME_INSERT_DUMMY));
    if (m_raw.NumFrameActual == 0 && bSkipFramesMode)
    {
        request.Type        = MFX_MEMTYPE_D3D_INT;
        request.NumFrameMin =  mfxU16(m_vpar.AsyncDepth + m_vpar.RawRef * m_vpar.mfx.NumRefFrame) ;
        sts = m_rawSkip.Alloc(&m_core, request, true);
        MFX_CHECK_STS(sts);
    }

    request.Type        = MFX_MEMTYPE_D3D_INT;

    request.NumFrameMin = MaxRec(m_vpar);

    if(request.Info.FourCC == MFX_FOURCC_RGB4)
    {
        //in case of ARGB input we need NV12 reconstruct allocation
        request.Info.FourCC = (m_vpar.mfx.FrameInfo.BitDepthLuma == 10) ? MFX_FOURCC_P010 : MFX_FOURCC_NV12;
        request.Info.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
    }
    //For MMCD encoder bind flag is required.
    request.Type |= MFX_MEMTYPE_VIDEO_MEMORY_ENCODER_TARGET;
    sts = m_rec.Alloc(&m_core, request, false);
    MFX_CHECK_STS(sts);

    sts = m_ddi->Register(m_rec.NumFrameActual ? m_rec : m_raw, D3DDDIFMT_NV12);
    MFX_CHECK_STS(sts);

    sts = m_ddi->QueryCompBufferInfo(D3DDDIFMT_INTELENCODE_BITSTREAMDATA, request);
    MFX_CHECK_STS(sts);

    request.Type        = MFX_MEMTYPE_D3D_INT;
    request.NumFrameMin = MaxBs(m_vpar);

    if (GetBsSize(request.Info) < GetMinBsSize(m_vpar))
    {
        MFX_CHECK(request.Info.Width != 0, MFX_ERR_UNDEFINED_BEHAVIOR);
        request.Info.Height = (mfxU16)CeilDiv(GetMinBsSize(m_vpar), request.Info.Width);
    }

    sts = m_bs.Alloc(&m_core, request, false);
    MFX_CHECK_STS(sts);

    sts = m_ddi->Register(m_bs, D3DDDIFMT_INTELENCODE_BITSTREAMDATA);
    MFX_CHECK_STS(sts);

    m_task.Reset(m_vpar.isField(), MaxTask(m_vpar));

    Fill(m_lastTask, IDX_INVALID);


    m_NumberOfSlicesForOpt = m_vpar.mfx.NumSlice;
    ZeroParams();

    m_brc = CreateBrc(m_vpar);
    if (m_brc)
    {
        sts = m_brc->Init(m_vpar, m_vpar.HRDConformance);
        MFX_CHECK_STS(sts);
    }

    m_bInit = true;
    m_runtimeErr = MFX_ERR_NONE;

    return qsts;
}

mfxStatus MFXVideoENCODEH265_HW::Init(mfxVideoParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;
    MFX_CHECK_NULL_PTR1(par);

    MFX_CHECK(par->mfx.CodecId == MFX_CODEC_HEVC, MFX_ERR_UNSUPPORTED);
    MFX_CHECK(!m_bInit, MFX_ERR_UNDEFINED_BEHAVIOR);

    sts = InitImpl(par);

    if (!m_bInit)
        FreeResources();

    return sts;
}

mfxStatus MFXVideoENCODEH265_HW::QueryIOSurf(mfxCoreInterface *core, mfxVideoParam *par, mfxFrameAllocRequest *request)
{
    mfxStatus sts = MFX_ERR_NONE;
    MFX_CHECK_NULL_PTR2(par, request);

    MFX_CHECK(par->mfx.CodecId == MFX_CODEC_HEVC, MFX_ERR_UNSUPPORTED);

    MFXCoreInterface _core = *core;

    MfxVideoParam tmp = *par;
    ENCODE_CAPS_HEVC caps = {};

    sts = ExtBuffer::CheckBuffers(*par);
    MFX_CHECK_STS(sts);

    switch (par->IOPattern & MFX_IOPATTERN_IN_MASK)
    {
    case MFX_IOPATTERN_IN_SYSTEM_MEMORY:
        request->Type = MFX_MEMTYPE_SYS_EXT;
        break;
    case MFX_IOPATTERN_IN_VIDEO_MEMORY:
        request->Type = MFX_MEMTYPE_D3D_EXT;
        break;
    case MFX_IOPATTERN_IN_OPAQUE_MEMORY:
        request->Type = MFX_MEMTYPE_FROM_ENCODE | MFX_MEMTYPE_DXVA2_DECODER_TARGET | MFX_MEMTYPE_OPAQUE_FRAME;
        break;
    default: return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    _core.QueryPlatform(&tmp.m_platform);

    (void)SetLowpowerDefault(tmp);

    sts = QueryHwCaps(&_core, GetGUID(tmp), caps);
    MFX_CHECK_STS(sts);

    MfxHwH265Encode::CheckVideoParam(tmp, caps);
    SetDefaults(tmp, caps);

    request->Info = tmp.mfx.FrameInfo;
    request->Info.Shift = tmp.mfx.CodecProfile == MFX_PROFILE_HEVC_MAIN10? 1: 0;
    request->NumFrameMin = MaxRaw(tmp);

    request->NumFrameSuggested = request->NumFrameMin;

    return sts;
}

mfxStatus MFXVideoENCODEH265_HW::Query(mfxCoreInterface *core, mfxVideoParam *in, mfxVideoParam *out)
{
    mfxStatus sts = MFX_ERR_NONE;
    MFX_CHECK_NULL_PTR1(out);

    if (!in)
    {
        Zero(out->mfx);

        out->IOPattern             = 1;
        out->Protected             = 1;
        out->AsyncDepth            = 1;
        //out->mfx.CodecId           = 1;
        out->mfx.LowPower          = 1;
        out->mfx.CodecLevel        = 1;
        out->mfx.CodecProfile      = 1;
        out->mfx.TargetUsage       = 1;
        out->mfx.GopPicSize        = 1;
        out->mfx.GopRefDist        = 1;
        out->mfx.GopOptFlag        = 1;
        out->mfx.IdrInterval       = 1;
        out->mfx.RateControlMethod = 1;
        out->mfx.InitialDelayInKB  = 1;
        out->mfx.BufferSizeInKB    = 1;
        out->mfx.TargetKbps        = 1;
        out->mfx.MaxKbps           = 1;
        out->mfx.NumSlice          = 1;
        out->mfx.NumRefFrame       = 1;
        out->mfx.EncodedOrder      = 1;

        out->mfx.FrameInfo.FourCC        = 1;
        out->mfx.FrameInfo.Width         = 1;
        out->mfx.FrameInfo.Height        = 1;
        out->mfx.FrameInfo.CropX         = 1;
        out->mfx.FrameInfo.CropY         = 1;
        out->mfx.FrameInfo.CropW         = 1;
        out->mfx.FrameInfo.CropH         = 1;
        out->mfx.FrameInfo.FrameRateExtN = 1;
        out->mfx.FrameInfo.FrameRateExtD = 1;
        out->mfx.FrameInfo.AspectRatioW  = 1;
        out->mfx.FrameInfo.AspectRatioH  = 1;
        out->mfx.FrameInfo.ChromaFormat  = 1;
        out->mfx.FrameInfo.PicStruct     = 1;
    }
    else
    {
        MfxVideoParam tmp = *in;
        ENCODE_CAPS_HEVC caps = {};
        mfxExtEncoderCapability * enc_cap = ExtBuffer::Get(*in);

        if (enc_cap!=0)
        {
            return MFX_ERR_UNSUPPORTED;
        }

        MFXCoreInterface _core = *core;

        sts = _core.QueryPlatform(&tmp.m_platform);
        MFX_CHECK_STS(sts);

        MFX_CHECK(in->mfx.CodecId == MFX_CODEC_HEVC, MFX_ERR_UNSUPPORTED);

        // matching ExtBuffers
        sts = ExtBuffer::CheckBuffers(*in, *out);
        if (sts == MFX_ERR_INVALID_VIDEO_PARAM)
            sts = MFX_ERR_UNSUPPORTED;
        MFX_CHECK_STS(sts);

        mfxStatus lpsts = SetLowpowerDefault(tmp);

        sts = QueryHwCaps(&_core, GetGUID(tmp), caps);
        MFX_CHECK_STS(sts);

        mfxExtCodingOptionSPSPPS* pSPSPPS = ExtBuffer::Get(*in);
        if (pSPSPPS && pSPSPPS->SPSBuffer)
        {
            sts = LoadSPSPPS(tmp, pSPSPPS);
            MFX_CHECK_STS(sts);

            sts = CheckHeaders(tmp, caps);
            MFX_CHECK_STS(sts);
        }

        sts = MfxHwH265Encode::CheckVideoParam(tmp, caps);
        if (sts == MFX_ERR_INVALID_VIDEO_PARAM)
            sts = MFX_ERR_UNSUPPORTED;

        if (sts == MFX_ERR_NONE && lpsts != MFX_ERR_NONE)
            sts = lpsts;

        tmp.FillPar(*out, true);

        // SetLowPowerDefault may change LowPower to default value
        // if LowPower was invalid set it to Zero to mimic Query behavior
        if (lpsts == MFX_WRN_INCOMPATIBLE_VIDEO_PARAM)
        {
            out->mfx.LowPower = 0;
        }
        else // otherwise 'hide' default value
        {
            out->mfx.LowPower = in->mfx.LowPower;
        }
    }

    return sts;
}
mfxStatus    MFXVideoENCODEH265_HW::WaitingForAsyncTasks(bool bResetTasks)
{
    mfxStatus sts = MFX_ERR_NONE;

    if (m_runtimeErr == 0)
    {
       // scheduler must wait until all async tasks will be ready.

        if (!bResetTasks)
           return sts;
    }

    for (;;)
    {
        //waiting for async tasks are finished
        Task* task = m_task.GetTaskForQuery();
        if (!task)
            break;

        m_task.SubmitForQuery(task);
        task->m_stage = STAGE_READY;
        if (task->m_surf)
            FreeTask(*task);
        m_task.Ready(task);

    }

    // drop other tasks (not submitted into async part)
    for (;;)
    {
        Task* pTask = m_task.GetNewTask();
        if (!pTask)
            break;

        if (pTask->m_surf)
        {
            m_core.DecreaseReference(&pTask->m_surf->Data);
            pTask->m_surf = 0;
        }
        m_task.SkipTask(pTask);
    }

    for (;;)
    {
        Task* pTask = m_task.Reorder(m_vpar, m_lastTask.m_dpb[0], true);
        if (!pTask)
            break;
        m_task.Submit(pTask);
    }
    // free reordered tasks
    for (;;)
    {
        Task* task = m_task.GetTaskForSubmit();
        if (!task)
            break;

        m_task.SubmitForQuery(task);
        if (task->m_surf)
            FreeTask(*task);
        m_task.Ready(task);
    }

    m_task.Reset(m_vpar.isField());

    m_raw.Unlock();
    m_rawSkip.Unlock();
    m_rec.Unlock();
    m_bs.Unlock();
    m_CuQp.Unlock();

    Fill(m_lastTask, 0xFF);
    ZeroParams();

    return sts;
}


mfxStatus  MFXVideoENCODEH265_HW::CheckVideoParam(MfxVideoParam & par, ENCODE_CAPS_HEVC const & caps, bool bInit /*= false*/)
{
    mfxStatus sts = ExtraCheckVideoParam(par, caps, bInit);
    MFX_CHECK(sts >= MFX_ERR_NONE, sts);

    return GetWorstSts(MfxHwH265Encode::CheckVideoParam(par, caps, bInit), sts);
}

mfxStatus   MFXVideoENCODEH265_HW::Reset(mfxVideoParam *par)
{
    mfxStatus sts = MFX_ERR_NONE, qsts = MFX_ERR_NONE;
    MFX_CHECK_NULL_PTR1(par);

    MFX_CHECK(m_bInit, MFX_ERR_NOT_INITIALIZED);

    sts = ExtBuffer::CheckBuffers(*par);
    MFX_CHECK_STS(sts);

    MfxVideoParam parNew = *par;

    mfxExtEncoderResetOption * pResetOpt = ExtBuffer::Get(*par);
    mfxExtCodingOptionSPSPPS* pSPSPPS = ExtBuffer::Get(*par);

    sts = LoadSPSPPS(parNew, pSPSPPS);
    MFX_CHECK_STS(sts);

    Copy(parNew.m_platform, m_vpar.m_platform);
    InheritDefaultValues(m_vpar, parNew);
    parNew.SyncVideoToCalculableParam();

    qsts = CheckVideoParam(parNew, m_caps);
    MFX_CHECK(qsts >= MFX_ERR_NONE, qsts);

    SetDefaults(parNew, m_caps);

    parNew.SyncCalculableToVideoParam();
    parNew.AlignCalcWithBRCParamMultiplier();

    if (!pSPSPPS || !pSPSPPS->SPSBuffer)
        parNew.SyncMfxToHeadersParam(m_NumberOfSlicesForOpt);

    sts = CheckHeaders(parNew, m_caps);
    MFX_CHECK_STS(sts);

    MFX_CHECK(m_vpar.LCUSize ==  parNew.LCUSize, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM); // LCU Size Can't be changed

    MFX_CHECK(
           parNew.mfx.CodecId                == MFX_CODEC_HEVC
        && m_vpar.AsyncDepth                 == parNew.AsyncDepth
        && m_vpar.mfx.GopRefDist             >= parNew.mfx.GopRefDist
        //&& m_vpar.mfx.NumSlice               >= parNew.mfx.NumSlice
        && m_vpar.mfx.NumRefFrame            >= parNew.mfx.NumRefFrame
        && m_vpar.mfx.RateControlMethod      == parNew.mfx.RateControlMethod
        && m_rec.m_info.Width                >= parNew.mfx.FrameInfo.Width
        && m_rec.m_info.Height               >= parNew.mfx.FrameInfo.Height
        && m_vpar.mfx.FrameInfo.ChromaFormat == parNew.mfx.FrameInfo.ChromaFormat
        && m_vpar.IOPattern                  == parNew.IOPattern
        ,  MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);

    MFX_CHECK(m_vpar.m_ext.CO2.ExtBRC == parNew.m_ext.CO2.ExtBRC, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
    if (IsOn(m_vpar.m_ext.CO2.ExtBRC))
    {
        mfxExtBRC*   extBRCInit = &m_vpar.m_ext.extBRC;
        mfxExtBRC*   extBRCReset = &parNew.m_ext.extBRC;

        MFX_CHECK(
            extBRCInit->pthis == extBRCReset->pthis &&
            extBRCInit->Init == extBRCReset->Init &&
            extBRCInit->Reset == extBRCReset->Reset &&
            extBRCInit->Close == extBRCReset->Close &&
            extBRCInit->GetFrameCtrl == extBRCReset->GetFrameCtrl &&
            extBRCInit->Update == extBRCReset->Update, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
    }

    if (m_vpar.mfx.RateControlMethod == MFX_RATECONTROL_CBR ||
        m_vpar.mfx.RateControlMethod == MFX_RATECONTROL_VBR ||
        m_vpar.mfx.RateControlMethod == MFX_RATECONTROL_VCM)
    {
        MFX_CHECK(
            m_vpar.InitialDelayInKB == parNew.InitialDelayInKB &&
            m_vpar.BufferSizeInKB   == parNew.BufferSizeInKB,
            MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
    }

    mfxU32 tempLayerIdx = 0;
    bool changeTScalLayers = false;
    bool isIdrRequired = false;

    // check if change of temporal scalability required by new parameters
    if (m_vpar.isTL() && parNew.isTL())
    {
        // calculate temporal layer for next frame
        tempLayerIdx      = m_vpar.GetTId(m_frameOrder);
        changeTScalLayers = m_vpar.NumTL() != parNew.NumTL();
    }

    // check if IDR required after change of encoding parameters
    bool isSpsChanged = m_vpar.m_sps.vui_parameters_present_flag == 0 ?
        memcmp(&m_vpar.m_sps, &parNew.m_sps, sizeof(SPS) - sizeof(VUI)) != 0 :
    !Equal(m_vpar.m_sps, parNew.m_sps);

    isIdrRequired = isSpsChanged
        || (tempLayerIdx != 0 && changeTScalLayers)
        || m_vpar.mfx.GopPicSize != parNew.mfx.GopPicSize
        || m_vpar.m_ext.CO2.IntRefType != parNew.m_ext.CO2.IntRefType;

    /*
    printf("isIdrRequired %d, isSpsChanged %d \n", isIdrRequired, isSpsChanged);
    if (isSpsChanged)
    {
        printf("addres of pic_width_in_luma_samples %d\n", (mfxU8*)&m_vpar.m_sps.pic_width_in_luma_samples -(mfxU8*)&m_vpar.m_sps);
        printf("addres of log2_diff_max_min_pcm_luma_coding_block_size %d\n", (mfxU8*)&m_vpar.m_sps.log2_diff_max_min_pcm_luma_coding_block_size -(mfxU8*)&m_vpar.m_sps);
        printf("addres of lt_ref_pic_poc_lsb_sps[0] %d\n", (mfxU8*)&m_vpar.m_sps.lt_ref_pic_poc_lsb_sps[0] -(mfxU8*)&m_vpar.m_sps);
        printf("addres of vui[0] %d\n", (mfxU8*)&m_vpar.m_sps.vui -(mfxU8*)&m_vpar.m_sps);

        for (int t = 0; t < sizeof(m_vpar.m_sps); t++)
        {
            if (((mfxU8*)&m_vpar.m_sps)[t]  != ((mfxU8*)&parNew.m_sps)[t])
                printf("Difference in %d, %d, %d\n",t, ((mfxU8*)&m_vpar.m_sps)[t], ((mfxU8*)&parNew.m_sps)[t]);
        }
    }
    */

    if (isIdrRequired && pResetOpt && IsOff(pResetOpt->StartNewSequence))
        return MFX_ERR_INVALID_VIDEO_PARAM; // Reset can't change parameters w/o IDR. Report an error

    isIdrRequired = isIdrRequired || (pResetOpt && IsOn(pResetOpt->StartNewSequence));

    bool brcReset = false;

    if (m_vpar.mfx.RateControlMethod == MFX_RATECONTROL_CBR ||
        m_vpar.mfx.RateControlMethod == MFX_RATECONTROL_VBR ||
        m_vpar.mfx.RateControlMethod == MFX_RATECONTROL_VCM)
    {
        if ( m_vpar.TargetKbps != parNew.TargetKbps||
             m_vpar.BufferSizeInKB != parNew.BufferSizeInKB ||
             m_vpar.InitialDelayInKB != parNew.InitialDelayInKB||
             m_vpar.mfx.FrameInfo.FrameRateExtN != parNew.mfx.FrameInfo.FrameRateExtN ||
             m_vpar.mfx.FrameInfo.FrameRateExtD != parNew.mfx.FrameInfo.FrameRateExtD ||
             m_vpar.m_ext.CO2.MaxFrameSize != parNew.m_ext.CO2.MaxFrameSize)
             brcReset = true;
    }
    if (m_vpar.mfx.RateControlMethod == MFX_RATECONTROL_VBR || m_vpar.mfx.RateControlMethod == MFX_RATECONTROL_VCM)
    {
        if (m_vpar.MaxKbps != parNew.MaxKbps)  brcReset = true;
    }

    if (brcReset &&
        m_vpar.mfx.RateControlMethod == MFX_RATECONTROL_CBR &&
        (parNew.HRDConformance || !isIdrRequired))
        return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;

    // waiting for submitted in driver tasks
    MFX_CHECK_STS(WaitingForAsyncTasks(isIdrRequired));

    if (isIdrRequired)
    {
        brcReset = true;
    }

    if (!Equal(m_vpar.m_pps, parNew.m_pps))
        m_task.Reset(parNew.isField(), 0, (mfxU16)INSERT_PPS);

    m_vpar = parNew;

    m_hrd.Reset(m_vpar.m_sps);

    if (m_brc )
    {
        if (isIdrRequired)
        {
            parNew.m_ext.ResetOpt.StartNewSequence = MFX_CODINGOPTION_ON;
        }
        sts = m_brc->Reset(parNew);
        MFX_CHECK_STS(sts);
        brcReset = false;
    }

    sts = m_ddi->Reset(m_vpar, brcReset);
    MFX_CHECK_STS(sts);

    return qsts;
}

mfxStatus  MFXVideoENCODEH265_HW::GetVideoParam(mfxVideoParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;

    MFX_CHECK(m_bInit, MFX_ERR_NOT_INITIALIZED);
    MFX_CHECK_NULL_PTR1(par);

    sts = m_vpar.FillPar(*par);
    return sts;
}

mfxStatus  MFXVideoENCODEH265_HW::EncodeFrameSubmit(mfxEncodeCtrl *ctrl, mfxFrameSurface1 *surface, mfxBitstream *bs, mfxThreadTask *thread_task)
{
    mfxStatus sts = MFX_ERR_NONE;
    Task* task = 0;

    MFX_CHECK(m_bInit, MFX_ERR_NOT_INITIALIZED);
    MFX_CHECK_NULL_PTR1(bs);

    MFX_CHECK_STS(m_runtimeErr);

    if (surface)
    {
        MFX_CHECK((surface->Data.Y == 0) == (surface->Data.UV == 0), MFX_ERR_UNDEFINED_BEHAVIOR);
        MFX_CHECK(surface->Data.Pitch < 0x8000, MFX_ERR_UNDEFINED_BEHAVIOR);
        MFX_CHECK(surface->Info.Width  >=  m_vpar.mfx.FrameInfo.Width, MFX_ERR_INVALID_VIDEO_PARAM);
        MFX_CHECK(surface->Info.Height >=  m_vpar.mfx.FrameInfo.Height, MFX_ERR_INVALID_VIDEO_PARAM);
    }

    {
        MFX_CHECK(bs->DataOffset <= bs->MaxLength, MFX_ERR_UNDEFINED_BEHAVIOR);
        MFX_CHECK(bs->DataOffset + bs->DataLength + m_vpar.BufferSizeInKB * 1000u <= bs->MaxLength, MFX_ERR_NOT_ENOUGH_BUFFER);
        MFX_CHECK_NULL_PTR1(bs->Data);
    }

    sts = ExtraParametersCheck(ctrl, surface, bs);
    MFX_CHECK_STS(sts);

    if (surface)
    {
        task = m_task.New();
        MFX_CHECK(task, MFX_WRN_DEVICE_BUSY);

        task->m_surf = surface;
        if (ctrl)
            task->m_ctrl = *ctrl;

        if (m_vpar.IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY)
        {
            task->m_surf_real = 0;

            sts = m_core.GetRealSurface(task->m_surf, &task->m_surf_real);
            MFX_CHECK_STS(sts);

            if (task->m_surf_real == 0)
                return MFX_ERR_UNDEFINED_BEHAVIOR;

            task->m_surf_real->Info            = task->m_surf->Info;
            task->m_surf_real->Data.TimeStamp  = task->m_surf->Data.TimeStamp;
            task->m_surf_real->Data.FrameOrder = task->m_surf->Data.FrameOrder;
            task->m_surf_real->Data.Corrupted  = task->m_surf->Data.Corrupted;
            task->m_surf_real->Data.DataFlag   = task->m_surf->Data.DataFlag;
        }
        else
            task->m_surf_real = task->m_surf;

        m_core.IncreaseReference(&surface->Data);
    }
    else if (m_numBuffered)
    {
        task = m_task.New();
        MFX_CHECK(task, MFX_WRN_DEVICE_BUSY);
        task->m_bs = bs;
        *thread_task = task;
        m_numBuffered --;
        return MFX_ERR_NONE;
    }
    else
        return MFX_ERR_MORE_DATA;

    task->m_bs = bs;
    *thread_task = task;

    if (surface!=0)
    {
        mfxI32 numBuff = (!m_vpar.mfx.EncodedOrder) ? (m_vpar.mfx.GopRefDist- 1)*(m_vpar.isField()?2:1): 0;
        mfxI32 asyncDepth = (m_vpar.AsyncDepth > 1) ? 1 : 0;
        if ((mfxI32)m_numBuffered < numBuff + asyncDepth)
        {
            m_numBuffered ++;
            sts = MFX_ERR_MORE_DATA_SUBMIT_TASK;
            task->m_bs = 0;
        }
    }


    return sts;
}
mfxStatus  MFXVideoENCODEH265_HW::PrepareTask(Task& input_task)
{
    mfxStatus sts = MFX_ERR_NONE;
    Task* task = &input_task;
    MFX_CHECK (task->m_stage == FRAME_NEW, MFX_ERR_NONE);

    if (task->m_surf)
    {
        if (m_vpar.mfx.EncodedOrder)
        {
            task->m_frameType = task->m_ctrl.FrameType;
            m_frameOrder = task->m_surf->Data.FrameOrder;

            if (m_brc && m_brc->IsVMEBRC())
            {
                mfxExtLAFrameStatistics *vmeData = ExtBuffer::Get(task->m_ctrl);
                MFX_CHECK_NULL_PTR1(vmeData);
                mfxStatus status = m_brc->SetFrameVMEData(vmeData, m_vpar.mfx.FrameInfo.Width, m_vpar.mfx.FrameInfo.Height);
                MFX_CHECK_STS(status);
                mfxLAFrameInfo *pInfo = &vmeData->FrameStat[0];
                task->m_frameType = (mfxU16)pInfo->FrameType;
                m_frameOrder = pInfo->FrameDisplayOrder;
                task->m_eo = pInfo->FrameEncodeOrder;
                MFX_CHECK(m_frameOrder <= task->m_eo + m_vpar.mfx.GopRefDist - 1, MFX_ERR_UNDEFINED_BEHAVIOR);
                task->m_level = pInfo->Layer;
            }
            MFX_CHECK(m_frameOrder != static_cast<mfxU32>(MFX_FRAMEORDER_UNKNOWN), MFX_ERR_UNDEFINED_BEHAVIOR);
            MFX_CHECK(task->m_frameType != MFX_FRAMETYPE_UNKNOWN, MFX_ERR_UNDEFINED_BEHAVIOR);
            MFX_CHECK((mfxI32)(m_frameOrder - (m_lastTask.m_eo + 1)) <= (m_vpar.mfx.GopRefDist - 1)* (m_vpar.isField() ? 2 : 1), MFX_ERR_UNDEFINED_BEHAVIOR);
            if (!isValid(m_lastTask.m_dpb[1][0]))
                MFX_CHECK(task->m_frameType & MFX_FRAMETYPE_IDR, MFX_ERR_UNDEFINED_BEHAVIOR);
        }
        else
        {
            task->m_frameType = GetFrameType(m_vpar, (m_vpar.isField() ?  (m_lastIDR %2) : 0) + m_frameOrder - m_lastIDR);
            if (task->m_ctrl.FrameType & MFX_FRAMETYPE_IDR)
                task->m_frameType = MFX_FRAMETYPE_I|MFX_FRAMETYPE_REF|MFX_FRAMETYPE_IDR;
        }

        if (task->m_frameType & MFX_FRAMETYPE_IDR)
            m_lastIDR = m_frameOrder;

        task->m_poc = m_frameOrder - m_lastIDR;
        task->m_fo =  m_frameOrder;
        task->m_bpo = (mfxU32)MFX_FRAMEORDER_UNKNOWN;
        task->m_secondField = m_vpar.isField() ? (!!(task->m_fo & 1)) : false;
        task->m_bottomField = false;

        if (m_vpar.isField())
            task->m_bottomField = (task->m_surf->Info.PicStruct & (MFX_PICSTRUCT_FIELD_TFF | MFX_PICSTRUCT_FIELD_BFF)) == 0 ?
                (m_vpar.isBFF() != task->m_secondField) :
                (task->m_surf->Info.PicStruct & MFX_PICSTRUCT_FIELD_BFF) != 0;

        m_frameOrder ++;
        task->m_stage = FRAME_ACCEPTED;
    }
    else
    {
        m_task.Submit(task);
    }

    if (!m_vpar.mfx.EncodedOrder)
        task = m_task.Reorder(m_vpar, m_lastTask.m_dpb[1], !task->m_surf);
    MFX_CHECK(task, MFX_ERR_NONE);

    if (task->m_surf)
    {
        task->m_idxRaw = (mfxU8)FindFreeResourceIndex(m_raw);
        task->m_idxRec = (mfxU8)FindFreeResourceIndex(m_rec);
        task->m_idxBs  = (mfxU8)FindFreeResourceIndex(m_bs);
        task->m_idxCUQp = (mfxU8)FindFreeResourceIndex(m_CuQp);
        MFX_CHECK(task->m_idxBs  != IDX_INVALID, MFX_ERR_NONE);
        MFX_CHECK(task->m_idxRec != IDX_INVALID, MFX_ERR_NONE);

        task->m_midRaw = AcquireResource(m_raw, task->m_idxRaw);
        task->m_midRec = AcquireResource(m_rec, task->m_idxRec);
        task->m_midBs  = AcquireResource(m_bs,  task->m_idxBs);
        task->m_midCUQp  = AcquireResource(m_bs,  task->m_idxCUQp);
        MFX_CHECK(task->m_midRec && task->m_midBs, MFX_ERR_UNDEFINED_BEHAVIOR);

        ConfigureTask(*task, m_lastTask, m_vpar, m_caps, m_baseLayerOrder);

        m_lastTask = *task;
        m_task.Submit(task);
    }

    return sts;
}

mfxStatus  MFXVideoENCODEH265_HW::Execute(mfxThreadTask thread_task, mfxU32 /*uid_p*/, mfxU32 /*uid_a*/)
{
    MFX_CHECK(m_bInit, MFX_ERR_NOT_INITIALIZED);
    MFX_CHECK_STS(m_runtimeErr);

    Task* inputTask = (Task*) thread_task;
    mfxStatus sts = MFX_ERR_NONE;

    MFX_CHECK(m_bInit, MFX_ERR_NOT_INITIALIZED);
    MFX_CHECK_NULL_PTR1(inputTask);

    sts = PrepareTask(*inputTask);
    MFX_CHECK_STS(sts);

    while (Task* taskForExecute = m_task.GetTaskForSubmit(true))
    {
        if ((taskForExecute->m_insertHeaders & INSERT_BPSEI) && m_task.GetTaskForQuery())
            break; // sync is needed

        if (taskForExecute->m_surf)
        {
            mfxHDLPair surfaceHDL = {};

            taskForExecute->m_cpb_removal_delay = (taskForExecute->m_eo == 0) ? 0 : (taskForExecute->m_eo - m_prevBPEO);

            if (taskForExecute->m_insertHeaders & INSERT_BPSEI)
            {
                taskForExecute->m_initial_cpb_removal_delay = m_hrd.GetInitCpbRemovalDelay(*taskForExecute);
                taskForExecute->m_initial_cpb_removal_offset = m_hrd.GetInitCpbRemovalDelayOffset();
                m_prevBPEO = taskForExecute->m_eo;
            }
            if (m_brc)
            {
               if (IsOn(m_vpar.mfx.LowPower) || m_vpar.m_platform.CodeName >= MFX_PLATFORM_KABYLAKE
                   )
                   taskForExecute->m_qpY = (mfxI8)Clip3( 0, 51, m_brc->GetQP(m_vpar, *taskForExecute));  //driver limitation
               else
                   taskForExecute->m_qpY = (mfxI8)Clip3( -6 * m_vpar.m_sps.bit_depth_luma_minus8, 51, m_brc->GetQP(m_vpar, *taskForExecute));

               taskForExecute->m_sh.slice_qp_delta = mfxI8(taskForExecute->m_qpY - (m_vpar.m_pps.init_qp_minus26 + 26));
               if (taskForExecute->m_recode && m_vpar.AsyncDepth > 1)
               {
                   taskForExecute->m_sh.temporal_mvp_enabled_flag = 0; // WA
               }
            }

            if (IsFrameToSkip(*taskForExecute,  m_rec, m_vpar.isSWBRC()))
            {
                sts = CodeAsSkipFrame(m_core,m_vpar,*taskForExecute,m_rawSkip, m_rec);
                MFX_CHECK_STS(sts);
            }
            sts = GetNativeHandleToRawSurface(m_core, m_vpar, *taskForExecute, surfaceHDL);
            MFX_CHECK_STS(sts);

            if (!IsFrameToSkip(*taskForExecute,  m_rec, m_vpar.isSWBRC()))
            {
                sts = CopyRawSurfaceToVideoMemory(m_core, m_vpar, *taskForExecute);
                MFX_CHECK_STS(sts);
            }
            ExtraTaskPreparation(*taskForExecute);

            sts = m_ddi->Execute(*taskForExecute, surfaceHDL);
            MFX_CHECK_STS(sts);

            m_task.SubmitForQuery(taskForExecute);
        }
    }

   if (inputTask->m_bs)
   {
        mfxBRCStatus brcStatus = MFX_BRC_OK;
        mfxBitstream* bs = inputTask->m_bs;
        Task* taskForQuery = m_task.GetTaskForQuery();
        MFX_CHECK (taskForQuery, MFX_ERR_UNDEFINED_BEHAVIOR);

        taskForQuery->m_bs = bs;    // bs->ExtParam can be used in QueryStatus
        sts = m_ddi->QueryStatus(*taskForQuery);
        if (sts == MFX_WRN_DEVICE_BUSY)
            return MFX_TASK_BUSY;

        if (taskForQuery->m_bsDataLength > GetBsSize(m_bs.m_info))
            sts = MFX_ERR_DEVICE_FAILED; //possible memory corruption in driver

        if (sts < 0)
            m_runtimeErr = sts;

        MFX_CHECK_STS(sts);


       ENCODE_PACKEDHEADER_DATA* pSEI = m_ddi->PackHeader(*taskForQuery, SUFFIX_SEI_NUT);
       mfxU32 SEI_len = pSEI && pSEI->DataLength ? pSEI->DataLength  : 0;
       if ((!m_brc && m_hrd.Enabled()) || m_vpar.Protected)
           SEI_len = 0;

        if (m_brc)
        {
            brcStatus = m_brc->PostPackFrame(m_vpar,*taskForQuery, (taskForQuery->m_bsDataLength + SEI_len)*8,0,taskForQuery->m_recode);
            //printf("m_brc->PostPackFrame poc %d, qp %d, len %d, type %d, status %d\n", taskForQuery->m_poc,taskForQuery->m_qpY, taskForQuery->m_bsDataLength,taskForQuery->m_codingType, brcStatus);
            if (brcStatus != MFX_BRC_OK)
            {
                MFX_CHECK(brcStatus != MFX_BRC_ERROR,  MFX_ERR_NOT_ENOUGH_BUFFER);
                MFX_CHECK(!taskForQuery->m_bSkipped, MFX_ERR_NOT_ENOUGH_BUFFER);

                if ((brcStatus & MFX_BRC_NOT_ENOUGH_BUFFER) && (brcStatus & MFX_BRC_ERR_SMALL_FRAME))
                {
                    mfxI32 minSize = 0, maxSize = 0;
                    //padding is needed
                    m_brc->GetMinMaxFrameSize(&minSize, &maxSize);
                   taskForQuery->m_minFrameSize = (mfxU32) ((minSize + 7) >> 3);
                   brcStatus = m_brc->PostPackFrame(m_vpar,*taskForQuery, taskForQuery->m_minFrameSize<<3,0,++taskForQuery->m_recode);
                   MFX_CHECK(brcStatus != MFX_BRC_ERROR,  MFX_ERR_UNDEFINED_BEHAVIOR);
                }
                else
                {
                    // recoding is needed
                    if (brcStatus & MFX_BRC_NOT_ENOUGH_BUFFER)
                    {
                        //skip frame
                        taskForQuery->m_bSkipped = true;
                    }
                    taskForQuery->m_recode++;
                    sts = m_task.PutTasksForRecode(taskForQuery); //return task in execute pool
                    MFX_CHECK_STS(sts);

                    while (Task* task = m_task.GetTaskForQuery())
                    {
                        WaitForQueringTask(*task);
                        sts = m_task.PutTasksForRecode(task); //return task in execute pool
                        MFX_CHECK_STS(sts);
                    }
                    return MFX_TASK_BUSY;
                }
            }
        }

        //update bitstream
        if (taskForQuery->m_bsDataLength)
        {
            mfxFrameAllocator & fa = m_core.FrameAllocator();
            mfxFrameData codedFrame = {};
            mfxU32 bytesAvailable = bs->MaxLength - bs->DataOffset - bs->DataLength;
            mfxU32 bytes2copy     = taskForQuery->m_bsDataLength;
            mfxI32 dpbOutputDelay = taskForQuery->m_fo +  GetNumReorderFrames(m_vpar.mfx.GopRefDist-1,m_vpar.isBPyramid(), m_vpar.isField(), m_vpar.bFieldReord) - taskForQuery->m_eo;
            mfxU8* bsData         = bs->Data + bs->DataOffset + bs->DataLength;
            mfxU32* pDataLength   = &bs->DataLength;


            MFX_CHECK(bytesAvailable >= bytes2copy, MFX_ERR_NOT_ENOUGH_BUFFER);
            //codedFrame.MemType = MFX_MEMTYPE_INTERNAL_FRAME;
            sts = fa.Lock(fa.pthis, taskForQuery->m_midBs, &codedFrame);
            MFX_CHECK(codedFrame.Y, MFX_ERR_LOCK_MEMORY);
            mfxSize roi = {(int32_t)bytes2copy,1};
            //memcpy_s(bs->Data + bs->DataOffset + bs->DataLength, bytes2copy, codedFrame.Y, bytes2copy);
            FastCopy::Copy(bsData,bytes2copy,codedFrame.Y,codedFrame.Pitch,roi,COPY_VIDEO_TO_SYS);
            sts = fa.Unlock(fa.pthis, taskForQuery->m_midBs, &codedFrame);
            MFX_CHECK_STS(sts);

            *pDataLength += bytes2copy;
            bytesAvailable -= bytes2copy;

            if (SEI_len)
            {
                MFX_CHECK(bytesAvailable >= pSEI->DataLength, MFX_ERR_NOT_ENOUGH_BUFFER);

                memcpy_s(bs->Data + bs->DataOffset + bs->DataLength, pSEI->DataLength, pSEI->pData + pSEI->DataOffset, pSEI->DataLength);

                bs->DataLength += pSEI->DataLength;
                bytes2copy     += pSEI->DataLength;
                bytesAvailable -= pSEI->DataLength;
            }

            if (taskForQuery->m_minFrameSize > bytes2copy)
            {
                mfxU32 padding = taskForQuery->m_minFrameSize - bytes2copy;
                MFX_CHECK(bytesAvailable >= padding , MFX_ERR_NOT_ENOUGH_BUFFER);
                memset(bs->Data + bs->DataOffset + bs->DataLength, 0, padding);

                bs->DataLength += padding;
                bytes2copy     += padding;
                bytesAvailable -= padding;
            }

            m_hrd.Update(bytes2copy * 8, *taskForQuery);

            bs->TimeStamp       = taskForQuery->m_surf->Data.TimeStamp;
            bs->DecodeTimeStamp = CalcDTSFromPTS(this->m_vpar.mfx.FrameInfo, (mfxU16)dpbOutputDelay, bs->TimeStamp);
            bs->PicStruct       = taskForQuery->m_surf->Info.PicStruct;
            bs->FrameType       = taskForQuery->m_frameType;

            if (taskForQuery->m_ldb)
            {
                bs->FrameType &= ~MFX_FRAMETYPE_B;
                bs->FrameType |=  MFX_FRAMETYPE_P;
            }

        }


        taskForQuery->m_stage |= FRAME_ENCODED;
        FreeTask(*taskForQuery);
        m_task.Ready(taskForQuery);

        if (!inputTask->m_surf)
        {
            //formal task
             m_task.SubmitForQuery(inputTask);
             m_task.Ready(inputTask);
        }
   }
   return sts;
}

mfxStatus  MFXVideoENCODEH265_HW::FreeResources(mfxThreadTask /*thread_task*/, mfxStatus /*sts*/)
{
    return MFX_ERR_NONE;
}
mfxStatus  MFXVideoENCODEH265_HW::FreeTask(Task &task)
{
    if (task.m_midBs)
    {
        ReleaseResource(m_bs,  task.m_midBs);
        task.m_midBs = 0;
    }
    if (task.m_midCUQp)
    {
        ReleaseResource(m_bs,  task.m_midCUQp);
        task.m_midCUQp = 0;
    }

    if (!m_vpar.RawRef)
    {
        if (task.m_surf)
        {
            m_core.DecreaseReference(&task.m_surf->Data);
            task.m_surf = 0;
        }
        if (task.m_midRaw)
        {
            ReleaseResource(m_raw, task.m_midRaw);
            ReleaseResource(m_rawSkip, task.m_midRaw);
            task.m_midRaw =0;
        }
    }

    if (!(task.m_frameType & MFX_FRAMETYPE_REF))
    {
        if (task.m_midRec)
        {
            ReleaseResource(m_rec, task.m_midRec);
            task.m_midRec = 0;
        }
        if (m_vpar.RawRef)
        {
            if (task.m_surf)
            {
                m_core.DecreaseReference(&task.m_surf->Data);
                task.m_surf = 0;
            }
            if (task.m_midRaw)
            {
                ReleaseResource(m_raw, task.m_midRaw);
                ReleaseResource(m_rawSkip, task.m_midRaw);
                task.m_midRaw = 0;
            }
        }
    }
    for (mfxU16 i = 0, j = 0; !isDpbEnd(task.m_dpb[TASK_DPB_BEFORE], i); i ++)
    {
        for (j = 0; !isDpbEnd(task.m_dpb[TASK_DPB_AFTER], j); j ++)
            if (task.m_dpb[TASK_DPB_BEFORE][i].m_idxRec == task.m_dpb[TASK_DPB_AFTER][j].m_idxRec)
                break;

        if (isDpbEnd(task.m_dpb[TASK_DPB_AFTER], j))
        {
            if (task.m_dpb[TASK_DPB_BEFORE][i].m_midRec)
            {
                ReleaseResource(m_rec, task.m_dpb[TASK_DPB_BEFORE][i].m_midRec);
                task.m_dpb[TASK_DPB_BEFORE][i].m_midRec = 0;
            }

            if (m_vpar.RawRef)
            {
                if (task.m_dpb[TASK_DPB_BEFORE][i].m_surf)
                {
                    m_core.DecreaseReference(&task.m_dpb[TASK_DPB_BEFORE][i].m_surf->Data);
                    task.m_dpb[TASK_DPB_BEFORE][i].m_surf = 0;
                }
                if (task.m_dpb[TASK_DPB_BEFORE][i].m_midRaw)
                {
                    ReleaseResource(m_raw, task.m_dpb[TASK_DPB_BEFORE][i].m_midRaw);
                    task.m_dpb[TASK_DPB_BEFORE][i].m_midRaw = 0;
                }
            }
        }
    }

    return MFX_ERR_NONE;
}
mfxStatus  MFXVideoENCODEH265_HW::WaitForQueringTask(Task& task)
{
   mfxStatus sts = m_ddi->QueryStatus(task);
   while  (sts ==  MFX_WRN_DEVICE_BUSY)
   {
       vm_time_sleep(1);
       sts = m_ddi->QueryStatus(task);
   }
   return sts;
}

void  MFXVideoENCODEH265_HW::FreeResources()
{
    mfxExtOpaqueSurfaceAlloc& opaq = m_vpar.m_ext.Opaque;

    m_rec.Free();
    m_raw.Free();
    m_rawSkip.Free();
    m_bs.Free();

    m_ddi.reset();

    m_frameOrder = 0;
    Zero(m_lastTask);
    Zero(m_caps);

    if (m_vpar.IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY && opaq.In.Surfaces)
    {
        m_core.UnmapOpaqueSurface(opaq.In.NumSurface, opaq.In.Type, opaq.In.Surfaces);
        Zero(opaq);
    }
    if (m_brc)
    {
        delete m_brc;
        m_brc = 0;
    }
}

mfxStatus  MFXVideoENCODEH265_HW::Close()
{
    MFX_CHECK(m_bInit, MFX_ERR_NOT_INITIALIZED);

    MFX_CHECK_STS(WaitingForAsyncTasks(true));
    FreeResources();

    m_bInit = false;
    return MFX_ERR_NONE;
}

};
#endif
