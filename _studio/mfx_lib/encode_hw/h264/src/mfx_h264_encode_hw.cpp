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
#ifdef MFX_ENABLE_H264_VIDEO_ENCODE_HW
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
#include "mfx_h264_enc_common_hw.h"
#include "mfx_h264_encode_hw_utils.h"

#include "mfx_h264_encode_cm.h"
#include "mfx_h264_encode_cm_defs.h"

#include "vm_time.h"


using namespace MfxHwH264Encode;


namespace MfxHwH264EncodeHW
{
    mfxU32 PaddingBytesToWorkAroundHrdIssue(
        MfxVideoParam const &      video,
        Hrd                        hrd,
        std::list<DdiTask> const & submittedTasks,
        mfxU32                     fieldPicFlag,
        mfxU32                     secondPicFlag)
    {
        mfxExtCodingOption const & extOpt = GetExtBufferRef(video);

        if (video.mfx.RateControlMethod != MFX_RATECONTROL_CBR || IsOff(extOpt.NalHrdConformance))
            return 0;

        mfxF64 frameRate = mfxF64(video.mfx.FrameInfo.FrameRateExtN) / video.mfx.FrameInfo.FrameRateExtD;
        mfxU32 avgFrameSize = mfxU32(1000 * video.calcParam.targetKbps / frameRate);
        if (avgFrameSize <= 128 * 1024 * 8)
            return 0;

        for (DdiTaskCiter i = submittedTasks.begin(); i != submittedTasks.end(); ++i)
            hrd.RemoveAccessUnit(0, i->m_fieldPicFlag, false);
        if (secondPicFlag)
            hrd.RemoveAccessUnit(0, fieldPicFlag, false);
        hrd.RemoveAccessUnit(0, fieldPicFlag, false);

        mfxU32 bufsize  = 8000 * video.calcParam.bufferSizeInKB;
        mfxU32 bitrate  = GetMaxBitrateValue(video.calcParam.maxKbps) << (6 + SCALE_FROM_DRIVER);
        mfxU32 delay    = hrd.GetInitCpbRemovalDelay();
        mfxU32 fullness = mfxU32(mfxU64(delay) * bitrate / 90000.0);

        const mfxU32 maxFrameSize = video.mfx.FrameInfo.Width * video.mfx.FrameInfo.Height;
        if (fullness > bufsize)
            return std::min((fullness - bufsize + 7) / 8, maxFrameSize);

        return 0;
    }

    mfxU32 PaddingBytesToWorkAroundHrdIssue(
        MfxVideoParam const &       video,
        Hrd                         hrd,
        std::vector<mfxU16> const & submittedPicStruct,
        mfxU16                      currentPicStruct)
    {
        mfxExtCodingOption const * extOpt = GetExtBuffer(video);

        if (video.mfx.RateControlMethod != MFX_RATECONTROL_CBR || !extOpt ||IsOff(extOpt->NalHrdConformance))
            return 0;

        mfxF64 frameRate = mfxF64(video.mfx.FrameInfo.FrameRateExtN) / video.mfx.FrameInfo.FrameRateExtD;
        mfxU32 avgFrameSize = mfxU32(1000 * video.calcParam.targetKbps / frameRate);
        if (avgFrameSize <= 128 * 1024 * 8)
            return 0;

        for (size_t i = 0; i < submittedPicStruct.size(); i++)
        {
            hrd.RemoveAccessUnit(
                0,
                !(submittedPicStruct[i] & MFX_PICSTRUCT_PROGRESSIVE),
                false);
        }

        hrd.RemoveAccessUnit(
            0,
            !(currentPicStruct & MFX_PICSTRUCT_PROGRESSIVE),
            false);

        mfxU32 bufsize  = 8000 * video.calcParam.bufferSizeInKB;
        mfxU32 bitrate  = GetMaxBitrateValue(video.calcParam.maxKbps) << (6 + SCALE_FROM_DRIVER);
        mfxU32 delay    = hrd.GetInitCpbRemovalDelay();
        mfxU32 fullness = mfxU32(mfxU64(delay) * bitrate / 90000.0);

        const mfxU32 maxFrameSize = video.mfx.FrameInfo.Width * video.mfx.FrameInfo.Height;
        if (fullness > bufsize)
            return std::min((fullness - bufsize + 7) / 8, maxFrameSize);

        return 0;
    }

    mfxU16 GetFrameWidth(MfxVideoParam & par)
    {
        mfxExtCodingOptionSPSPPS & extBits = GetExtBufferRef(par);
        if (extBits.SPSBuffer)
        {
            mfxExtSpsHeader & extSps = GetExtBufferRef(par);
            return mfxU16(16 * (extSps.picWidthInMbsMinus1 + 1));
        }
        else
        {
            return par.mfx.FrameInfo.Width;
        }
    }

    mfxU16 GetFrameHeight(MfxVideoParam & par)
    {
        mfxExtCodingOptionSPSPPS & extBits = GetExtBufferRef(par);
        if (extBits.SPSBuffer)
        {
            mfxExtSpsHeader & extSps = GetExtBufferRef(par);
            return mfxU16(16 * (extSps.picHeightInMapUnitsMinus1 + 1) * (2 - extSps.frameMbsOnlyFlag));
        }
        else
        {
            return par.mfx.FrameInfo.Height;
        }
    }

    IDirect3DDeviceManager9 * GetDeviceManager(VideoCORE * core)
    {
        (void)core;

        throw std::logic_error("GetDeviceManager not implemented on Linux for Look Ahead");
    }

    VmeData * FindUnusedVmeData(std::vector<VmeData> & vmeData)
    {
        VmeData * oldest = 0;
        for (size_t i = 0; i < vmeData.size(); i++)
        {
            if (!vmeData[i].used)
                return &vmeData[i];
            if (oldest == 0 || oldest->encOrder > vmeData[i].encOrder)
                oldest = &vmeData[i];
        }
        return oldest;
    }
}
using namespace MfxHwH264EncodeHW;

mfxStatus MFXHWVideoENCODEH264::Init(mfxVideoParam * par)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "MFXHWVideoENCODEH264::Init");
    if (m_impl.get() != 0)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }


    std::unique_ptr<VideoENCODE> impl(
                (VideoENCODE *) new ImplementationAvc(m_core));

    mfxStatus sts = impl->Init(par);
    MFX_CHECK(
        sts >= MFX_ERR_NONE &&
        sts != MFX_WRN_PARTIAL_ACCELERATION, sts);

    m_impl = std::move(impl);
    return sts;
}

mfxStatus MFXHWVideoENCODEH264::Query(
    VideoCORE *     core,
    mfxVideoParam * in,
    mfxVideoParam * out,
    void *          state)
{


    if (in && IsMvcProfile(in->mfx.CodecProfile) && !IsHwMvcEncSupported())
        return MFX_ERR_UNSUPPORTED;

    if (in == 0)
        return ImplementationAvc::Query(core, in, out);



    if (state)
    {
        MFXHWVideoENCODEH264 * AVCEncoder = (MFXHWVideoENCODEH264*)state;

        if (!AVCEncoder->m_impl.get())
        {
            assert(!"Encoder implementation isn't initialized");
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        }

        return ImplementationAvc::Query(core, in, out, AVCEncoder->m_impl.get());
    }

    return ImplementationAvc::Query(core, in, out);
}

mfxStatus MFXHWVideoENCODEH264::QueryIOSurf(
    VideoCORE *            core,
    mfxVideoParam *        par,
    mfxFrameAllocRequest * request)
{

    if (IsMvcProfile(par->mfx.CodecProfile) && !IsHwMvcEncSupported())
        return MFX_WRN_PARTIAL_ACCELERATION;

    return ImplementationAvc::QueryIOSurf(core, par, request);
}

mfxStatus ImplementationAvc::Query(
    VideoCORE *     core,
    mfxVideoParam * in,
    mfxVideoParam * out,
    void *          state)
{
    MFX_CHECK_NULL_PTR2(core, out);

    mfxStatus sts;

    // "in" parameters should uniquely identify one of 4 Query operation modes (see MSDK spec for details)
    mfxU8 queryMode = DetermineQueryMode(in);
    MFX_CHECK(queryMode, MFX_ERR_UNDEFINED_BEHAVIOR); // input parameters are contradictory and don't allow to choose Query mode

    eMFXHWType platfrom = core->GetHWType();

    if (queryMode == 1) // see MSDK spec for details related to Query mode 1
    {
        Zero(out->mfx);

        out->IOPattern             = 1;
        out->Protected             = 1;
        out->AsyncDepth            = 1;
        out->mfx.CodecId           = 1;
        out->mfx.LowPower          = 1;
        if (!hasSupportVME(platfrom))
            out->mfx.LowPower      = 0;
        out->mfx.CodecLevel        = 1;
        out->mfx.CodecProfile      = 1;
        out->mfx.NumThread         = 0;
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
        out->mfx.NumThread         = 1;
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

        if (mfxExtMVCSeqDesc * opt = GetExtBuffer(*out))
        {
            opt->NumOP     = 1;
            opt->NumView   = 1;
            opt->NumViewId = 1;
        }

        if (mfxExtAVCRefListCtrl * ctrl = GetExtBuffer(*out))
        {
            mfxExtBuffer tmp = ctrl->Header;
            Zero(*ctrl);
            ctrl->Header = tmp;

            ctrl->NumRefIdxL0Active = 1;
            ctrl->NumRefIdxL1Active = 1;
            ctrl->ApplyLongTermIdx  = 1;

            ctrl->LongTermRefList[0].FrameOrder   = 1;
            ctrl->LongTermRefList[0].LongTermIdx  = 1;
            ctrl->PreferredRefList[0].FrameOrder  = 1;
            ctrl->RejectedRefList[0].FrameOrder   = 1;
        }

        if (mfxExtEncoderResetOption * resetOpt = GetExtBuffer(*out))
        {
            resetOpt->StartNewSequence = 1;
        }

        if (mfxExtEncoderROI * extRoi = GetExtBuffer(*out))
        {
            extRoi->NumROI          = 1;
#if MFX_VERSION > 1021
            extRoi->ROIMode         = MFX_ROI_MODE_QP_DELTA;
#endif // MFX_VERSION > 1021
            extRoi->ROI[0].Left     = 1;
            extRoi->ROI[0].Right    = 1;
            extRoi->ROI[0].Top      = 1;
            extRoi->ROI[0].Bottom   = 1;
            extRoi->ROI[0].DeltaQP = 1;
        }
        if (mfxExtEncoderCapability * extCap = GetExtBuffer(*out))
        {
            MFX_CHECK(MFX_HW_VAAPI != core->GetVAType(), MFX_ERR_UNSUPPORTED);
        }

#ifdef MFX_ENABLE_MFE
        if(mfxExtMultiFrameParam* ExtMultiFrameParam = GetExtBuffer(*out))
        {
            ExtMultiFrameParam->MaxNumFrames = 1;
            ExtMultiFrameParam->MFMode = 1;
        }

        if(mfxExtMultiFrameControl* ExtMultiFrameControl = GetExtBuffer(*out))
        {
            ExtMultiFrameControl->Timeout = 1;
            ExtMultiFrameControl->Flush = 1;
        }
#endif

    }
    else if (queryMode == 2)  // see MSDK spec for details related to Query mode 2
    {
        MFX_ENCODE_CAPS hwCaps = { };
        MfxVideoParam tmp = *in; // deep copy, create all supported extended buffers

        eMFXHWType platfrom = core->GetHWType();
        mfxStatus lpSts = SetLowPowerDefault(tmp, platfrom);
        // let use dedault values if input resolution is 0x0
        mfxU32 Width  = in->mfx.FrameInfo.Width == 0 ? 1920: in->mfx.FrameInfo.Width;
        mfxU32 Height =  in->mfx.FrameInfo.Height == 0 ? 1088: in->mfx.FrameInfo.Height;
        if (Width > 4096 || Height > 4096)
            sts = MFX_ERR_UNSUPPORTED;
        else
            sts = QueryHwCaps(core, hwCaps, &tmp);

        if (sts != MFX_ERR_NONE)
        {
            if (IsOn(in->mfx.LowPower))
                MFX_RETURN(MFX_ERR_UNSUPPORTED)
            else
                MFX_RETURN(MFX_WRN_PARTIAL_ACCELERATION)
        }

        sts = ReadSpsPpsHeaders(tmp);
        MFX_CHECK_STS(sts);

        eMFXGTConfig* pMFXGTConfig = QueryCoreInterface<eMFXGTConfig>(core, MFXICORE_GT_CONFIG_GUID);
        MFX_CHECK(pMFXGTConfig != nullptr, MFX_ERR_NULL_PTR);

        mfxStatus checkSts = CheckVideoParamQueryLike(tmp, hwCaps, platfrom, core->GetVAType(), *pMFXGTConfig);

        if (checkSts == MFX_WRN_PARTIAL_ACCELERATION)
            MFX_RETURN(MFX_WRN_PARTIAL_ACCELERATION)
        else if (checkSts == MFX_ERR_INCOMPATIBLE_VIDEO_PARAM)
            checkSts = MFX_ERR_UNSUPPORTED;
        else if (checkSts == MFX_ERR_NONE && lpSts != MFX_ERR_NONE) // transfer MFX_WRN_INCOMPATIBLE_VIDEO_PARAM to upper level
            checkSts = lpSts;

        /* FEI ENCODE additional check for input parameters */
        mfxExtFeiParam* feiParam = GetExtBuffer(*in);
        if ((NULL != feiParam) && (MFX_FEI_FUNCTION_ENCODE != feiParam->Func))
            checkSts = MFX_ERR_UNSUPPORTED;

        // Query should not modify UNKNOWN options
        if (in->mfx.LowPower == MFX_CODINGOPTION_UNKNOWN)
            tmp.mfx.LowPower = MFX_CODINGOPTION_UNKNOWN;

        out->IOPattern  = tmp.IOPattern;
        out->Protected  = tmp.Protected;
        out->AsyncDepth = tmp.AsyncDepth;
        out->mfx = tmp.mfx;

        // should have same number of buffers
        MFX_CHECK((in->NumExtParam == out->NumExtParam) && ((!in->ExtParam) == (!out->ExtParam)),
        MFX_ERR_UNSUPPORTED);

        // should have same buffers
        if (in->ExtParam && out->ExtParam)
        {
            for (mfxU32 i = 0; i < in->NumExtParam; i++)
                MFX_CHECK_NULL_PTR1(in->ExtParam[i]);

            for (mfxU32 i = 0; i < out->NumExtParam; i++)
                MFX_CHECK_NULL_PTR1(out->ExtParam[i]);

            for (mfxU32 i = 0; i < in->NumExtParam; i++)
            {
                if (in->ExtParam[i])
                {
                    if (IsRunTimeOnlyExtBuffer(in->ExtParam[i]->BufferId))
                        continue; // it's runtime only ext buffer. Nothing to check or correct, just move on.

                    MFX_CHECK(IsVideoParamExtBufferIdSupported(in->ExtParam[i]->BufferId),
                    MFX_ERR_UNSUPPORTED);

                    // buffer present twice in 'in'
                    MFX_CHECK (MfxHwH264Encode::GetExtBuffer(
                        in->ExtParam + i + 1,
                        in->NumExtParam - i - 1,
                        in->ExtParam[i]->BufferId) == 0, MFX_ERR_UNDEFINED_BEHAVIOR);

                    mfxExtBuffer * buf = GetExtBuffer(out->ExtParam, out->NumExtParam, in->ExtParam[i]->BufferId);
                    MFX_CHECK(buf, MFX_ERR_UNDEFINED_BEHAVIOR); // buffer doesn't present in 'out'

                    mfxExtBuffer * corrected = GetExtBuffer(tmp.ExtParam, tmp.NumExtParam, in->ExtParam[i]->BufferId);
                    MFX_CHECK_NULL_PTR1(corrected);

                    if (buf->BufferId == MFX_EXTBUFF_MVC_SEQ_DESC)
                    {
                        // mfxExtMVCSeqDesc is complex structure
                        // deep-copy is required if mvc description is fully initialized
                        mfxExtMVCSeqDesc & src = *(mfxExtMVCSeqDesc *)corrected;
                        mfxExtMVCSeqDesc & dst = *(mfxExtMVCSeqDesc *)buf;

                        sts = CheckBeforeCopyQueryLike(dst, src);
                        MFX_CHECK(sts == MFX_ERR_NONE, MFX_ERR_UNSUPPORTED);

                        Copy(dst, src);
                    }
                    else
                    {
                        // shallow-copy
                        MFX_INTERNAL_CPY(buf, corrected, corrected->BufferSz);
                    }
                }
            }

            for (mfxU32 i = 0; i < out->NumExtParam; i++)
            {
                if (out->ExtParam[i] && GetExtBuffer(in->ExtParam, in->NumExtParam, out->ExtParam[i]->BufferId) == 0)
                    MFX_RETURN(MFX_ERR_UNSUPPORTED);
            }
        }

        return checkSts;
    }
    else if (queryMode == 3)  // see MSDK spec for details related to Query mode 3
    {
        mfxStatus checkSts = MFX_ERR_NONE;
        // encoder isn't initialized. Query() can't operate in mode 3
        MFX_CHECK(state, MFX_ERR_UNDEFINED_BEHAVIOR);

        checkSts = CheckExtBufferId(*in);
        MFX_CHECK_STS(checkSts);

        MfxVideoParam newPar = *in;
        bool isIdrRequired = false;
        bool isBRCReset = false;

        ImplementationAvc * AVCEncoder = (ImplementationAvc*)state;

        checkSts = AVCEncoder->ProcessAndCheckNewParameters(newPar, isBRCReset, isIdrRequired);
        MFX_CHECK(checkSts >= MFX_ERR_NONE, checkSts);

        mfxExtEncoderResetOption * extResetOptIn = GetExtBuffer(*in);
        mfxExtEncoderResetOption * extResetOptOut = GetExtBuffer(*out);

        if (extResetOptOut != 0)
        {
            extResetOptOut->StartNewSequence = extResetOptIn->StartNewSequence;
            if (extResetOptIn->StartNewSequence == MFX_CODINGOPTION_UNKNOWN)
            {
                extResetOptOut->StartNewSequence = mfxU16(isIdrRequired ? MFX_CODINGOPTION_ON : MFX_CODINGOPTION_OFF);
            }
        }

        return checkSts;
    }
    else if (4 == queryMode)// Query mode 4: Query should do a single thing - report MB processing rate
    {
        mfxU32 mbPerSec[16] = {0, };
        // let use dedault values if input resolution is 0x0, 1920x1088 - should cover almost all cases
        out->mfx.TargetUsage = in->mfx.TargetUsage == 0 ? 4: in->mfx.TargetUsage;

        mfxExtEncoderCapability * extCaps = GetExtBuffer(*out);
        // can't return MB processing rate since mfxExtEncoderCapability isn't attached to "out"
        MFX_CHECK(extCaps, MFX_ERR_UNDEFINED_BEHAVIOR);

        MfxVideoParam tmp = *in;
        eMFXHWType platfrom = core->GetHWType();
        (void)SetLowPowerDefault(tmp, platfrom);

        // query MB processing rate from driver
        sts = QueryMbProcRate(core, *out, mbPerSec, &tmp);
        if (IsOn(in->mfx.LowPower) && sts != MFX_ERR_NONE)
            MFX_RETURN(MFX_ERR_UNSUPPORTED);

        if (sts != MFX_ERR_NONE)
        {
            extCaps->MBPerSec = 0;
            // driver don't support reporting of MB processing rate
            MFX_CHECK(sts != MFX_ERR_UNSUPPORTED, MFX_ERR_UNSUPPORTED);
            MFX_RETURN(MFX_WRN_PARTIAL_ACCELERATION); // any other HW problem
        }

        extCaps->MBPerSec = mbPerSec[out->mfx.TargetUsage-1];
        // driver returned status OK and MAX_MB_PER_SEC = 0. Treat this as driver doesn't support reporting of MAX_MB_PER_SEC for requested encoding configuration
        MFX_CHECK(extCaps->MBPerSec, MFX_ERR_UNSUPPORTED);

        return MFX_ERR_NONE;
    }
    else if (5 == queryMode)
    {
        MfxVideoParam tmp = *in;
        eMFXHWType platform = core->GetHWType();
        (void)SetLowPowerDefault(tmp, platform);
        if(IsOn(tmp.mfx.LowPower))
            return QueryGuid(core, DXVA2_INTEL_LOWPOWERENCODE_AVC);
        return QueryGuid(core, DXVA2_Intel_Encode_AVC);
    }

    return MFX_ERR_NONE;
}

mfxStatus ImplementationAvc::QueryIOSurf(
    VideoCORE *            core,
    mfxVideoParam *        par,
    mfxFrameAllocRequest * request)
{
    mfxU32 inPattern = par->IOPattern & MFX_IOPATTERN_IN_MASK;
    MFX_CHECK(
        inPattern == MFX_IOPATTERN_IN_SYSTEM_MEMORY ||
        inPattern == MFX_IOPATTERN_IN_VIDEO_MEMORY ||
        inPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY,
        MFX_ERR_INVALID_VIDEO_PARAM);

    MFX_ENCODE_CAPS hwCaps = {};
    MfxVideoParam tmp(*par);
    eMFXHWType platfrom = core->GetHWType();
    mfxStatus lpSts = SetLowPowerDefault(tmp, platfrom);

    mfxStatus sts = QueryHwCaps(core, hwCaps, &tmp);
    if (IsOn(par->mfx.LowPower) && sts != MFX_ERR_NONE)
    {
        MFX_RETURN(MFX_ERR_UNSUPPORTED);
    }

    sts = ReadSpsPpsHeaders(tmp);
    MFX_CHECK_STS(sts);

    sts = CheckWidthAndHeight(tmp);
    MFX_CHECK_STS(sts);

    sts = CopySpsPpsToVideoParam(tmp);
    MFX_CHECK(sts >= MFX_ERR_NONE, sts);

    eMFXGTConfig* pMFXGTConfig = QueryCoreInterface<eMFXGTConfig>(core, MFXICORE_GT_CONFIG_GUID);
    MFX_CHECK(pMFXGTConfig != nullptr, MFX_ERR_NULL_PTR);

    mfxStatus checkSts = CheckVideoParamQueryLike(tmp, hwCaps, platfrom, core->GetVAType(), *pMFXGTConfig);
    MFX_CHECK(checkSts != MFX_ERR_UNSUPPORTED, MFX_ERR_UNSUPPORTED);
    if (checkSts == MFX_WRN_PARTIAL_ACCELERATION)
        MFX_RETURN(MFX_WRN_PARTIAL_ACCELERATION) // return immediately
    else if (checkSts == MFX_ERR_NONE && lpSts != MFX_ERR_NONE)
        checkSts = lpSts;

    SetDefaults(tmp, hwCaps, true, core->GetHWType(), core->GetVAType(), *pMFXGTConfig);
    if (tmp.IOPattern == MFX_IOPATTERN_IN_SYSTEM_MEMORY)
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
        //if MDF is in pipeline need to allocate shared resource to avoid performance issues due to decompression when MMCD is enabled
    }

    request->NumFrameMin = CalcNumFrameMin(tmp, hwCaps);
    request->NumFrameSuggested = request->NumFrameMin;
    // get FrameInfo from original VideoParam
    request->Info = tmp.mfx.FrameInfo;

    return MFX_ERR_NONE;
}

ImplementationAvc::ImplementationAvc(VideoCORE * core)
: m_core(core)
, m_video()
, m_stat()
, m_sliceDivider()
, m_stagesToGo(0)
, m_bDeferredFrame(0)
, m_fieldCounter(0)
, m_1stFieldStatus(MFX_ERR_NONE)
, m_frameOrder(0)
, m_baseLayerOrder(0)
, m_frameOrderIdrInDisplayOrder(0)
, m_frameOrderIntraInDisplayOrder(0)
, m_frameOrderIPInDisplayOrder(0)
, m_frameOrderPyrStart(0)
, m_miniGopCount(0)
, m_frameOrderStartTScalStructure(0)
, m_baseLayerOrderStartIntraRefresh(0)
, m_intraStripeWidthInMBs(0)
, m_enabledSwBrc(false)
, m_hrd()
#if defined(MFX_ENABLE_ENCTOOLS)
, m_encTools()
, m_enabledEncTools(false)
#endif
, m_maxBsSize(0)
, m_caps()
, m_failedStatus(MFX_ERR_NONE)
, m_inputFrameType(0)
, m_NumSlices(0)
, m_useMBQPSurf(false)
, m_useMbControlSurfs(false)
, m_currentPlatform(MFX_HW_UNKNOWN)
, m_currentVaType(MFX_HW_NO)
, m_useWAForHighBitrates(false)
, m_isENCPAK(false)
, m_resetBRC(false)
, m_LowDelayPyramidLayer(0)
, m_LtrQp(0)
, m_LtrOrder(-1)
, m_RefQp(0)
, m_RefOrder(-1)
{
    memset(&m_recNonRef, 0, sizeof(m_recNonRef));

}

ImplementationAvc::~ImplementationAvc()
{
    amtScd.Close();

    DestroyDanglingCmResources();

    mfxExtCodingOption2 const * extOpt2 = GetExtBuffer(m_video);
    if (extOpt2 && IsOn(extOpt2->UseRawRef) && (m_inputFrameType == MFX_IOPATTERN_IN_VIDEO_MEMORY))
    {
        // in case of raw references and external frame allocation, encoder needs to unlock DPB surfaces
        ArrayDpbFrame const & finDpb = m_lastTask.m_dpbPostEncoding;
        for (mfxU32 i = 0; i < finDpb.Size(); i++)
            m_core->DecreaseReference(&finDpb[i].m_yuvRaw->Data);
    }
}
void ImplementationAvc::DestroyDanglingCmResources()
{
    if (m_cmDevice)
    {
        mfxExtCodingOption2 * extOpt2 = GetExtBuffer(m_video);
        for (DdiTaskIter i = m_lookaheadStarted.begin(), e = m_lookaheadStarted.end(); i != e; ++i)
        {
            m_cmCtx->DestroyEvent(i->m_event);
            if (extOpt2 && (extOpt2->MaxSliceSize == 0))
            {
                int ffid = i->m_fid[0];
                ArrayDpbFrame & iniDpb = i->m_dpb[ffid];
                for (mfxU32 j = 0; j < iniDpb.Size(); j++)
                    m_cmDevice->DestroySurface(iniDpb[j].m_cmRaw);
            }
            m_cmDevice->DestroySurface(i->m_cmRaw);
            m_cmDevice->DestroyVmeSurfaceG7_5(i->m_cmRefs);
            m_cmDevice->DestroyVmeSurfaceG7_5(i->m_cmRefsLa);
        }
    }
}
/*Class for modifying and restoring VideoParams.
  Needed for initialization some components */
class ModifiedVideoParams
{
public:
    ModifiedVideoParams() :
        m_bInit(false),
        m_RateControlMethod(0),
        m_LookAheadDepth(0),
        m_MaxKbps(0)
    {}
    void ModifyForDDI(MfxVideoParam & par, bool bSWBRC)
    {
        if (!bSWBRC)
            return;
        if (!m_bInit)
            SaveParams(par);
        // in the case of SWBRC driver works in CQP mode
        par.mfx.RateControlMethod = MFX_RATECONTROL_CQP;
    }
    void ModifyForBRC(MfxVideoParam & par, bool bSWBRC)
    {
        mfxExtCodingOption2 & extOpt2 = GetExtBufferRef(par);
        if (!(bSWBRC && extOpt2.MaxSliceSize))
            return;
        if (!m_bInit)
            SaveParams(par);
        // int the case of MaxSliceSize BRC works in VBR mode (instead LA)
        par.mfx.RateControlMethod = MFX_RATECONTROL_VBR;
        par.mfx.MaxKbps = par.mfx.TargetKbps * 2;
        extOpt2.LookAheadDepth = 0;
    }
    void Restore(MfxVideoParam & par)
    {
        if (!m_bInit)
            return;
        mfxExtCodingOption2 & extOpt2 = GetExtBufferRef(par);
        par.mfx.RateControlMethod = m_RateControlMethod;
        par.mfx.MaxKbps = m_MaxKbps;
        extOpt2.LookAheadDepth = m_LookAheadDepth;
        m_bInit = false;
    }

private:
    bool    m_bInit;
    mfxU16  m_RateControlMethod;
    mfxU16  m_LookAheadDepth;
    mfxU16  m_MaxKbps;

protected:
    void SaveParams(MfxVideoParam & par)
    {
        mfxExtCodingOption2 & extOpt2 = GetExtBufferRef(par);
        m_RateControlMethod = par.mfx.RateControlMethod;
        m_LookAheadDepth= extOpt2.LookAheadDepth;
        m_MaxKbps = par.mfx.MaxKbps;
        m_bInit = true;
    }
};

mfxStatus ImplementationAvc::Init(mfxVideoParam * par)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "ImplementationAvc::Init");
    mfxStatus sts = CheckExtBufferId(*par);
    MFX_CHECK_STS(sts);

    /* feiParam buffer attached by application */
    mfxExtFeiParam* feiParam = (mfxExtFeiParam*)GetExtBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_FEI_PARAM);
    m_isENCPAK = feiParam && (feiParam->Func == MFX_FEI_FUNCTION_ENCODE);
    if ((NULL != feiParam) && (feiParam->Func != MFX_FEI_FUNCTION_ENCODE))
        return MFX_ERR_INVALID_VIDEO_PARAM;


    m_video = *par;
    eMFXHWType platform = m_core->GetHWType();
    mfxStatus lpSts = SetLowPowerDefault(m_video, platform);

    sts = ReadSpsPpsHeaders(m_video);
    MFX_CHECK_STS(sts);

    sts = CheckWidthAndHeight(m_video);
    MFX_CHECK_STS(sts);

    m_ddi.reset(CreatePlatformH264Encoder(m_core));
    if (m_ddi.get() == 0)
        return MFX_WRN_PARTIAL_ACCELERATION;

    GUID guid;
    if (IsOn(m_video.mfx.LowPower))
    {
        guid = DXVA2_INTEL_LOWPOWERENCODE_AVC;
    }
    else
    {
        guid = DXVA2_Intel_Encode_AVC;
    }
    sts = m_ddi->CreateAuxilliaryDevice(
        m_core,
        guid,
        GetFrameWidth(m_video),
        GetFrameHeight(m_video));
        if (sts != MFX_ERR_NONE)
            return MFX_WRN_PARTIAL_ACCELERATION;


    sts = m_ddi->QueryEncodeCaps(m_caps);
    if (sts != MFX_ERR_NONE)
        return MFX_WRN_PARTIAL_ACCELERATION;

    m_currentPlatform = platform;
    m_currentVaType = m_core->GetVAType();

    mfxStatus spsppsSts = CopySpsPpsToVideoParam(m_video);

    eMFXGTConfig* pMFXGTConfig = QueryCoreInterface<eMFXGTConfig>(m_core, MFXICORE_GT_CONFIG_GUID);
    MFX_CHECK(pMFXGTConfig != nullptr, MFX_ERR_NULL_PTR);

    mfxStatus checkStatus = CheckVideoParam(m_video, m_caps, m_core->IsExternalFrameAllocator(), m_currentPlatform, m_currentVaType, *pMFXGTConfig, true);
    if (checkStatus == MFX_WRN_PARTIAL_ACCELERATION)
        return MFX_WRN_PARTIAL_ACCELERATION;
    else if (checkStatus < MFX_ERR_NONE)
        return checkStatus;
    else if (checkStatus == MFX_ERR_NONE)
        checkStatus = spsppsSts;

    if (checkStatus == MFX_ERR_NONE && lpSts != MFX_ERR_NONE) // transfer MFX_WRN_INCOMPATIBLE_VIDEO_PARAM to upper level
        checkStatus = lpSts;

    // CQP enabled
    mfxExtCodingOption2 & extOpt2 = GetExtBufferRef(m_video);
    m_enabledSwBrc = bRateControlLA(m_video.mfx.RateControlMethod) || (IsOn(extOpt2.ExtBRC) && (m_video.mfx.RateControlMethod == MFX_RATECONTROL_CBR || m_video.mfx.RateControlMethod == MFX_RATECONTROL_VBR));


    // need it for both ENCODE and ENC
    m_hrd.Setup(m_video);

#if defined(MFX_ENABLE_ENCTOOLS)
    if (H264EncTools::isEncToolNeeded(m_video))
    {
        if (MFX_HW_D3D11 == m_currentVaType || MFX_HW_D3D9 == m_currentVaType || MFX_HW_VAAPI == m_currentVaType)
        {
            mfxHandleType h_type = (MFX_HW_D3D11 == m_currentVaType ? MFX_HANDLE_D3D11_DEVICE : (MFX_HW_D3D9 == m_currentVaType ? MFX_HANDLE_DIRECT3D_DEVICE_MANAGER9 : MFX_HANDLE_VA_DISPLAY));
            mfxHDL device_handle = nullptr;
            m_core->GetHandle(h_type, &device_handle);
            mfxFrameAllocator* pFrameAlloc = QueryCoreInterface<mfxFrameAllocator>(m_core, MFXIEXTERNALLOC_GUID);

            mfxEncToolsCtrlExtDevice & extBufDevice = GetExtBufferRef(m_video);
            extBufDevice.DeviceHdl = device_handle;
            extBufDevice.HdlType = h_type;
            mfxEncToolsCtrlExtAllocator & extBufAlloc = GetExtBufferRef(m_video);
            extBufAlloc.pAllocator = pFrameAlloc;
        }

        sts = m_encTools.Init(m_video);
        MFX_CHECK_STS(sts);

        m_enabledEncTools = true;
        if (m_encTools.IsBRC())
            m_enabledSwBrc = true;

    }
#endif
    if (m_enabledSwBrc)
    {
#if defined (MFX_ENABLE_ENCTOOLS)
        // to change m_video before BRC Init and DDI init
        ModifiedVideoParams mod_params;
        if (!m_enabledEncTools)
        {
            m_brc.SetImpl(CreateBrc(m_video, m_caps));
            // to change m_video before BRC Init and DDI init
            mod_params.ModifyForBRC(m_video, true);
            m_brc.Init(m_video);
            mod_params.Restore(m_video);
        }
        mod_params.ModifyForDDI(m_video, true);
        sts = m_ddi->CreateAccelerationService(m_video);
        MFX_CHECK(sts == MFX_ERR_NONE, MFX_WRN_PARTIAL_ACCELERATION);
        mod_params.Restore(m_video);
#else
        m_brc.SetImpl(CreateBrc(m_video, m_caps));
        // to change m_video before BRC Init and DDI init
        ModifiedVideoParams mod_params;
        mod_params.ModifyForBRC(m_video, true);
        m_brc.Init(m_video);
        mod_params.Restore(m_video);
        mod_params.ModifyForDDI(m_video, true);
        sts = m_ddi->CreateAccelerationService(m_video);
        MFX_CHECK(sts == MFX_ERR_NONE, MFX_WRN_PARTIAL_ACCELERATION);
        mod_params.Restore(m_video);
#endif
    }
    else
    {
        sts = m_ddi->CreateAccelerationService(m_video);
        if (sts != MFX_ERR_NONE)
            return MFX_WRN_PARTIAL_ACCELERATION;
    }



    if (IsOn(extOpt2.EnableMAD))
    {
        ENCODE_ENC_CTRL_CAPS c_caps = {};
        sts = m_ddi->QueryEncCtrlCaps(c_caps);

        if (sts == MFX_ERR_NONE && c_caps.MAD)
        {
            m_ddi->GetEncCtrlCaps(c_caps);

            if (!c_caps.MAD)
            {
                c_caps.MAD = 1;
                sts = m_ddi->SetEncCtrlCaps(c_caps);
                MFX_CHECK_STS(sts);
            }
        }
        else
        {
            extOpt2.EnableMAD = MFX_CODINGOPTION_OFF;
            checkStatus = MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
        }
    }

    mfxFrameAllocRequest request = { };
    request.Info = m_video.mfx.FrameInfo;

    mfxU32  adaptGopDelay = 0;
#if defined(MFX_ENABLE_ENCTOOLS)
        adaptGopDelay = H264EncTools::GetPreEncDelay(m_video);

#endif

    m_emulatorForSyncPart.Init(m_video, adaptGopDelay);
    m_emulatorForAsyncPart = m_emulatorForSyncPart;

    mfxExtOpaqueSurfaceAlloc & extOpaq = GetExtBufferRef(m_video);

    // Allocate raw surfaces.
    // This is required only in case of system memory at input

    if (m_video.IOPattern == MFX_IOPATTERN_IN_SYSTEM_MEMORY)
    {
        request.Type = MFX_MEMTYPE_D3D_INT;
        request.NumFrameMin = mfxU16(CalcNumSurfRaw(m_video));

        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "MfxFrameAllocResponse Alloc");
            sts = m_raw.Alloc(m_core, request, true);
        }
        MFX_CHECK_STS(sts);
    }
    else if (m_video.IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY)
    {
        request.Type = extOpaq.In.Type;
        request.NumFrameMin = extOpaq.In.NumSurface;

        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "MfxFrameAllocResponse Alloc");
            sts = m_opaqResponse.Alloc(m_core, request, extOpaq.In.Surfaces, extOpaq.In.NumSurface);
        }
        MFX_CHECK_STS(sts);

        if (extOpaq.In.Type & MFX_MEMTYPE_SYSTEM_MEMORY)
        {
            request.Type = MFX_MEMTYPE_D3D_INT;
            request.NumFrameMin = extOpaq.In.NumSurface;
            {
                MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "MfxFrameAllocResponse Alloc");
                sts = m_raw.Alloc(m_core, request, true);
            }
        }
    }
    mfxExtCodingOption3 & extOpt3 = GetExtBufferRef(m_video);
    bool bPanicModeSupport = m_enabledSwBrc;
    if (m_raw.NumFrameActual == 0 && bPanicModeSupport)
    {
        request.Type = MFX_MEMTYPE_D3D_INT;
        request.NumFrameMin = mfxU16(m_video.AsyncDepth);
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "MfxFrameAllocResponse Alloc");
            sts = m_rawSkip.Alloc(m_core, request, true);
        }
        MFX_CHECK_STS(sts);
    }

    m_inputFrameType =
        m_video.IOPattern == MFX_IOPATTERN_IN_SYSTEM_MEMORY ||
        (m_video.IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY && (extOpaq.In.Type & MFX_MEMTYPE_SYSTEM_MEMORY))
        ? MFX_IOPATTERN_IN_SYSTEM_MEMORY
        : MFX_IOPATTERN_IN_VIDEO_MEMORY;

    // ENC+PAK always needs separate chain for reconstructions produced by PAK.
    bool bParallelEncPak = (m_video.mfx.RateControlMethod == MFX_RATECONTROL_CQP && m_video.mfx.GopRefDist > 2 && m_video.AsyncDepth > 2);
    //in case if RGB or YUY2 passed we still need encode in 420
    request.Info.FourCC = MFX_FOURCC_NV12;
    request.Info.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
    request.Type = MFX_MEMTYPE_D3D_INT;
    request.NumFrameMin = mfxU16(m_video.mfx.NumRefFrame +
        m_emulatorForSyncPart.GetStageGreediness(AsyncRoutineEmulator::STG_WAIT_ENCODE) +
        bParallelEncPak);
    request.Type |= MFX_MEMTYPE_INTERNAL_FRAME;
    //For MMCD encoder bind flag is required, panic mode using reconstruct frame copy for refframe skipping, where no warranty that compressed frame will be processed proper way.
    if (!bIntRateControlLA(par->mfx.RateControlMethod))
    {
        request.Type |= MFX_MEMTYPE_VIDEO_MEMORY_ENCODER_TARGET;
    }
    else
    {
    }
    sts = m_rec.Alloc(m_core, request, bPanicModeSupport);
    MFX_CHECK_STS(sts);

    sts = m_ddi->Register(m_rec.NumFrameActual ? m_rec : m_raw, D3DDDIFMT_NV12);
    MFX_CHECK_STS(sts);

    m_recFrameOrder.resize(request.NumFrameMin, 0xffffffff);

    m_recNonRef[0] = m_recNonRef[1] = 0xffffffff;




    // Allocate surfaces for bitstreams.
    // Need at least one such surface and more for async-mode.
    sts = m_ddi->QueryCompBufferInfo(D3DDDIFMT_INTELENCODE_BITSTREAMDATA, request);
    MFX_CHECK_STS(sts);

    request.Type = MFX_MEMTYPE_D3D_INT;
    request.NumFrameMin = (mfxU16)m_emulatorForSyncPart.GetStageGreediness(AsyncRoutineEmulator::STG_WAIT_ENCODE);
    if (IsFieldCodingPossible(m_video))
        request.NumFrameMin *= 2; // 2 bitstream surfaces per frame
    // driver may suggest too small buffer for bitstream
    request.Info.Width = std::max(request.Info.Width, m_video.mfx.FrameInfo.Width);
    // Check for Video Conference scenario to reduce memory footprint
    if (MFX_RATECONTROL_CQP == m_video.mfx.RateControlMethod && MFX_SCENARIO_VIDEO_CONFERENCE != extOpt3.ScenarioInfo)
        request.Info.Height = std::max<mfxU16>(request.Info.Height, m_video.mfx.FrameInfo.Height * 3);
    else
        request.Info.Height = std::max<mfxU16>(request.Info.Height, m_video.mfx.FrameInfo.Height * 3 / 2);

    // workaround for high bitrates on small resolutions,
    // as driver do not respect coded buffer size we have to provide buffer large enough
    if (MFX_RATECONTROL_CQP != m_video.mfx.RateControlMethod)
    {
        const mfxU32 hrdBufSize = m_video.calcParam.bufferSizeInKB * 1000;
        if (hrdBufSize > static_cast<mfxU32>(request.Info.Width * request.Info.Height))
            request.Info.Height = mfx::align2_value(static_cast<mfxU16>(hrdBufSize / request.Info.Width), 16);
    }

    //limit bs size to 4095*nMBs + slice_hdr_size * nSlice
    if (MFX_SCENARIO_VIDEO_CONFERENCE != extOpt3.ScenarioInfo)
    {
        const mfxU32 SLICE_BUFFER_SIZE  = 2048; //from HeaderPacker
        const mfxU32 MAX_MB_SIZE        = 512;  //4095 bits in bytes

        const mfxU32 nMBs         = (m_video.mfx.FrameInfo.Width * m_video.mfx.FrameInfo.Height) / 256;
        const mfxU32 maxNumSlices = std::max<mfxU16>(GetMaxNumSlices(m_video), 1);
        const mfxU32 maxBufSize   = MAX_MB_SIZE * nMBs + SLICE_BUFFER_SIZE * maxNumSlices;

        if (maxBufSize > static_cast<mfxU32>(request.Info.Width * request.Info.Height))
            request.Info.Height = mfx::align2_value(static_cast<mfxU16>(maxBufSize / request.Info.Width), 16);
    }


    m_maxBsSize = request.Info.Width * request.Info.Height;

    m_NumSlices = m_video.mfx.FrameInfo.Height / 16;

    sts = m_bit.Alloc(m_core, request, false);
    MFX_CHECK_STS(sts);

    if (IsOn(extOpt3.FadeDetection)
        || bIntRateControlLA(m_video.mfx.RateControlMethod))
    {
        m_cmDevice.Reset(TryCreateCmDevicePtr(m_core));
        if (m_cmDevice == NULL)
            return MFX_ERR_UNSUPPORTED;
        m_cmCtx.reset(new CmContext(m_video, m_cmDevice, m_core));
    }

    if (bIntRateControlLA(m_video.mfx.RateControlMethod))
    {
        request.Info.Width = m_video.calcParam.widthLa;
        request.Info.Height = m_video.calcParam.heightLa;

        mfxU32 numMb = request.Info.Width * request.Info.Height / 256;
        mfxI32 numVME = m_emulatorForSyncPart.GetStageGreediness(AsyncRoutineEmulator::STG_WAIT_LA) +
            m_emulatorForSyncPart.GetStageGreediness(AsyncRoutineEmulator::STG_START_ENCODE) - 1;
        numVME = numVME < (m_video.mfx.GopRefDist + 1) ? (m_video.mfx.GopRefDist + 1) : numVME;
        m_vmeDataStorage.resize(numVME);
        for (size_t i = 0; i < m_vmeDataStorage.size(); i++)
            m_vmeDataStorage[i].mb.resize(numMb);
        m_tmpVmeData.reserve(extOpt2.LookAheadDepth);

        if (extOpt2.LookAheadDS > MFX_LOOKAHEAD_DS_OFF)
        {
            request.Info.FourCC = MFX_FOURCC_NV12;
            request.Type        = MFX_MEMTYPE_D3D_INT;
            request.NumFrameMin = mfxU16(m_video.mfx.NumRefFrame + m_video.AsyncDepth);

            sts = m_rawLa.AllocCmSurfaces(m_cmDevice, request);
            MFX_CHECK_STS(sts);
        }

        request.Info.Width  = m_video.calcParam.widthLa  / 16 * sizeof(LAOutObject);
        request.Info.Height = m_video.calcParam.heightLa / 16;
        request.Info.FourCC = MFX_FOURCC_P8;
        request.Type        = MFX_MEMTYPE_D3D_INT;
        request.NumFrameMin = mfxU16(m_video.mfx.NumRefFrame + m_video.AsyncDepth);

        sts = m_mb.AllocCmBuffersUp(m_cmDevice, request);
        MFX_CHECK_STS(sts);

        request.Info.Width  = sizeof(CURBEData);
        request.Info.Height = 1;
        request.Info.FourCC = MFX_FOURCC_P8;
        request.Type        = MFX_MEMTYPE_D3D_INT;
        request.NumFrameMin = 1 + !!(m_video.AsyncDepth > 1);

        sts = m_curbe.AllocCmBuffers(m_cmDevice, request);
        MFX_CHECK_STS(sts);
    }

    if (IsOn(extOpt3.FadeDetection) && m_cmCtx.get() && m_cmCtx->isHistogramSupported())
    {
        request.Info.Width  = 256 * 2 * sizeof(uint);
        request.Info.Height = 1;
        request.Info.FourCC = MFX_FOURCC_P8;
        request.Type        = MFX_MEMTYPE_D3D_INT;
        request.NumFrameMin = mfxU16(m_video.mfx.NumRefFrame + m_video.AsyncDepth);

        sts = m_histogram.AllocCmBuffersUp(m_cmDevice, request);
        MFX_CHECK_STS(sts);
    }

    if (IsExtBrcSceneChangeSupported(m_video)
#if defined(MFX_ENABLE_ENCTOOLS)
        && !(m_enabledEncTools)
#endif
        )

    {
        request.Info.FourCC = MFX_FOURCC_P8;
        request.NumFrameMin = mfxU16(m_video.AsyncDepth);
        request.Info.Width = amtScd.Get_asc_subsampling_width();
        request.Info.Height = amtScd.Get_asc_subsampling_height();
        if (IsCmNeededForSCD(m_video))
        {
            if (!m_cmDevice)
            {
                m_cmDevice.Reset(TryCreateCmDevicePtr(m_core));
                if (!m_cmDevice)
                    return MFX_ERR_UNSUPPORTED;
            }
            request.Type = MFX_MEMTYPE_D3D_INT;
            sts = m_scd.AllocCmSurfacesUP(m_cmDevice, request);
            MFX_CHECK_STS(sts);
        }
        else
        {
            request.Type = MFX_MEMTYPE_SYS_INT;
            sts = m_scd.AllocFrames(m_core, request);
            MFX_CHECK_STS(sts);
        }
        sts = amtScd.Init(m_video.mfx.FrameInfo.CropW, m_video.mfx.FrameInfo.CropH, m_video.mfx.FrameInfo.Width, m_video.mfx.FrameInfo.PicStruct, m_cmDevice);// cmDevice_useGPU);
        MFX_CHECK_STS(sts);
    }

    if (IsMctfSupported(m_video))
    {
#if defined(MFX_ENABLE_MCTF_IN_AVC)
        if (!m_cmDevice)
        {
            m_cmDevice.Reset(TryCreateCmDevicePtr(m_core));
            if (!m_cmDevice)
                return MFX_ERR_UNSUPPORTED;
        }

        amtMctf = std::make_shared<CMC>();
        MFX_CHECK_NULL_PTR1(amtMctf);

        request.Info        = m_video.mfx.FrameInfo;
        request.Type        = MFX_MEMTYPE_D3D_INT;
        request.NumFrameMin = (mfxU16)m_emulatorForSyncPart.GetStageGreediness(AsyncRoutineEmulator::STG_WAIT_MCTF) + 1;

        sts = m_mctf.Alloc(m_core, request);
        MFX_CHECK_STS(sts);

        sts = amtMctf->MCTF_INIT(m_core, m_cmDevice, m_video.mfx.FrameInfo, NULL, IsCmNeededForSCD(m_video), true, true, true);
        MFX_CHECK_STS(sts);
#endif
    }


    sts = m_ddi->Register(m_bit, D3DDDIFMT_INTELENCODE_BITSTREAMDATA);
    MFX_CHECK_STS(sts);
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "cleanup");
        m_free.resize(m_emulatorForSyncPart.GetTotalGreediness() + m_video.AsyncDepth - 1);
        m_incoming.clear();
        m_ScDetectionStarted.clear();
        m_ScDetectionFinished.clear();
        m_MctfStarted.clear();
        m_MctfFinished.clear();
        m_reordering.clear();
        m_lookaheadStarted.clear();
        m_lookaheadFinished.clear();
        m_histRun.clear();
        m_histWait.clear();
        m_encoding.clear();
        m_timeStamps.clear();
    }
    m_fieldCounter   = 0;
    m_1stFieldStatus = MFX_ERR_NONE;
    m_frameOrder     = 0;
    m_stagesToGo     = AsyncRoutineEmulator::STG_BIT_CALL_EMULATOR;
    m_bDeferredFrame = 0;
    m_failedStatus   = MFX_ERR_NONE;
    m_baseLayerOrder = 0;
    m_frameOrderIdrInDisplayOrder = 0;
    m_frameOrderIntraInDisplayOrder = 0;
    m_frameOrderIPInDisplayOrder = 0;
    m_frameOrderPyrStart = 0;
    m_miniGopCount = 0;
    m_frameOrderStartTScalStructure = 0;

    m_lastTask = DdiTask();
    // initialization of parameters for Intra refresh
    if (extOpt2.IntRefType)
    {
        if (extOpt2.IntRefType == MFX_REFRESH_SLICE)
        {
            m_intraStripeWidthInMBs = 0;
        }
        else
        {
            mfxU16 refreshDimension = extOpt2.IntRefType == MFX_REFRESH_HORIZONTAL ? m_video.mfx.FrameInfo.Height >> 4 : m_video.mfx.FrameInfo.Width >> 4;
            m_intraStripeWidthInMBs = (refreshDimension + extOpt2.IntRefCycleSize - 1) / extOpt2.IntRefCycleSize;
        }
        m_baseLayerOrderStartIntraRefresh = 0;
    }
    Zero(m_stat);

    // FIXME: for SNB issue with HRD at high bitrates
    // FIXME: check what to do with WA on Linux (MFX_HW_VAAPI) - currently it is switched off
    m_useWAForHighBitrates = (MFX_HW_VAAPI != m_core->GetVAType()) && !m_enabledSwBrc &&
        m_video.mfx.RateControlMethod == MFX_RATECONTROL_CBR &&
        (m_currentPlatform < MFX_HW_HSW || m_currentPlatform == MFX_HW_VLV); // HRD WA for high bitrates isn't required for HSW and beyond

    // required for slice header patching
    if (isBitstreamUpdateRequired(m_video, m_caps, m_currentPlatform))
        m_tmpBsBuf.resize(m_maxBsSize);

    const size_t MAX_SEI_SIZE    = 10 * 1024;
    const size_t MAX_FILLER_SIZE = m_video.mfx.FrameInfo.Width * m_video.mfx.FrameInfo.Height;
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "Preallocated Vector Alloc");
        m_sei.Alloc(mfxU32(MAX_SEI_SIZE + MAX_FILLER_SIZE));
    }

#if MFX_VERSION >= 1023
    #ifndef MFX_ENABLE_H264_REPARTITION_CHECK
        MFX_CHECK(extOpt3.RepartitionCheckEnable == MFX_CODINGOPTION_UNKNOWN, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
    #endif //MFX_ENABLE_H264_REPARTITION_CHECK
#endif // MFX_VERSION >= 1023


    // init slice divider
    bool fieldCoding = (m_video.mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_PROGRESSIVE) == 0;
    m_sliceDivider = MakeSliceDivider(
        (m_caps.ddi_caps.SliceLevelRateCtrl) ? SliceDividerType::ARBITRARY_MB_SLICE : SliceDividerType(m_caps.ddi_caps.SliceStructure),
        extOpt2.NumMbPerSlice,
        extOpt3.NumSliceP,
        m_video.mfx.FrameInfo.Width / 16,
        m_video.mfx.FrameInfo.Height / 16 / (fieldCoding ? 2 : 1));

    m_videoInit = m_video;

    return checkStatus;
}

mfxStatus ImplementationAvc::ProcessAndCheckNewParameters(
    MfxVideoParam & newPar,
    bool & brcReset,
    bool & isIdrRequired,
    mfxVideoParam const * newParIn)
{
    mfxStatus sts = MFX_ERR_NONE;

    /* feiParam buffer attached by application */
    mfxExtFeiParam* feiParam = NULL;
    if (newParIn)
    {
        feiParam = (mfxExtFeiParam*)GetExtBuffer(newParIn->ExtParam,
                                                 newParIn->NumExtParam,
                                                 MFX_EXTBUFF_FEI_PARAM);
    }
    m_isENCPAK = feiParam && (feiParam->Func == MFX_FEI_FUNCTION_ENCODE);
    if ((NULL != feiParam) && (feiParam->Func != MFX_FEI_FUNCTION_ENCODE))
            return MFX_ERR_INVALID_VIDEO_PARAM;


    mfxExtEncoderResetOption & extResetOpt = GetExtBufferRef(newPar);

    sts = ReadSpsPpsHeaders(newPar);
    MFX_CHECK_STS(sts);

    mfxExtOpaqueSurfaceAlloc & extOpaqNew = GetExtBufferRef(newPar);
    mfxExtOpaqueSurfaceAlloc & extOpaqOld = GetExtBufferRef(m_video);
    MFX_CHECK(
        extOpaqOld.In.Type       == extOpaqNew.In.Type       &&
        extOpaqOld.In.NumSurface == extOpaqNew.In.NumSurface,
        MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);

    mfxStatus spsppsSts = CopySpsPpsToVideoParam(newPar);


    //set NumSliceI/P/B to Numslice if not set by new parameters.
    ResetNumSliceIPB(newPar);

    InheritDefaultValues(m_video, newPar, m_caps, newParIn);

    eMFXGTConfig* pMFXGTConfig = QueryCoreInterface<eMFXGTConfig>(m_core, MFXICORE_GT_CONFIG_GUID);
    MFX_CHECK(pMFXGTConfig != nullptr, MFX_ERR_NULL_PTR);

    mfxStatus checkStatus = CheckVideoParam(newPar, m_caps, m_core->IsExternalFrameAllocator(), m_currentPlatform, m_currentVaType, *pMFXGTConfig);
    if (checkStatus == MFX_WRN_PARTIAL_ACCELERATION)
        return MFX_ERR_INVALID_VIDEO_PARAM;
    else if (checkStatus < MFX_ERR_NONE)
        return checkStatus;
    else if (checkStatus == MFX_ERR_NONE)
        checkStatus = spsppsSts;

    // check if change of temporal scalability required by new parameters
    mfxU32 tempLayerIdx = 0;
    bool changeTScalLayers = false;

    if (m_video.calcParam.tempScalabilityMode && newPar.calcParam.tempScalabilityMode)
    {
        // calculate temporal layer for next frame
        tempLayerIdx     = CalcTemporalLayerIndex(m_video, m_frameOrder - m_frameOrderStartTScalStructure);
        changeTScalLayers = m_video.calcParam.numTemporalLayer != newPar.calcParam.numTemporalLayer;
    }

    mfxExtSpsHeader const & extSpsNew = GetExtBufferRef(newPar);
    mfxExtSpsHeader const & extSpsOld = GetExtBufferRef(m_video);
    mfxExtCodingOption2 const & extOpt2New = GetExtBufferRef(newPar);
    mfxExtCodingOption2 const & extOpt2Old = GetExtBufferRef(m_video);
    mfxExtCodingOption3 const & extOpt3New = GetExtBufferRef(newPar);

    if(!IsOn(m_video.mfx.LowPower))
    {
        MFX_CHECK((extOpt2New.MaxSliceSize != 0) ==
                  (extOpt2Old.MaxSliceSize != 0),
                  MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
    }

    MFX_CHECK(!((m_video.mfx.RateControlMethod == MFX_RATECONTROL_LA ||
                m_video.mfx.RateControlMethod == MFX_RATECONTROL_LA_EXT ||
                m_video.mfx.RateControlMethod == MFX_RATECONTROL_LA_HRD) &&
                !IsOn(m_video.mfx.LowPower) && extOpt2New.MaxSliceSize == 0),
              MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);

    // check if IDR required after change of encoding parameters
    bool isSpsChanged = extSpsNew.vuiParametersPresentFlag == 0 ?
        memcmp(&extSpsNew, &extSpsOld, sizeof(mfxExtSpsHeader) - sizeof(VuiParameters)) != 0 :
        !Equal(extSpsNew, extSpsOld);

    isIdrRequired = isSpsChanged
        || (tempLayerIdx != 0 && changeTScalLayers)
        || newPar.mfx.GopPicSize != m_video.mfx.GopPicSize;

    if (isIdrRequired && IsOff(extResetOpt.StartNewSequence))
        return MFX_ERR_INVALID_VIDEO_PARAM; // Reset can't change parameters w/o IDR. Report an error

    mfxExtCodingOption & extOptNew = GetExtBufferRef(newPar);
    mfxExtCodingOption & extOptOld = GetExtBufferRef(m_video);

    brcReset =
        m_video.calcParam.targetKbps != newPar.calcParam.targetKbps ||
        m_video.calcParam.maxKbps    != newPar.calcParam.maxKbps;

    MFX_CHECK(
        IsAvcProfile(newPar.mfx.CodecProfile)                                   &&
        m_video.AsyncDepth                 == newPar.AsyncDepth                 &&
        m_videoInit.mfx.GopRefDist         >= newPar.mfx.GopRefDist             &&
        m_videoInit.mfx.NumSlice           >= newPar.mfx.NumSlice               &&
        m_videoInit.mfx.NumRefFrame        >= newPar.mfx.NumRefFrame            &&
        m_video.mfx.RateControlMethod      == newPar.mfx.RateControlMethod      &&
        m_videoInit.mfx.FrameInfo.Width    >= newPar.mfx.FrameInfo.Width        &&
        m_videoInit.mfx.FrameInfo.Height   >= newPar.mfx.FrameInfo.Height       &&
        m_video.mfx.FrameInfo.ChromaFormat == newPar.mfx.FrameInfo.ChromaFormat &&
        extOpt2Old.ExtBRC                  == extOpt2New.ExtBRC,
        MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);

    if (m_video.mfx.RateControlMethod != MFX_RATECONTROL_CQP)
    {
        MFX_CHECK(
            m_video.calcParam.initialDelayInKB == newPar.calcParam.initialDelayInKB &&
            m_video.calcParam.bufferSizeInKB   == newPar.calcParam.bufferSizeInKB,
            MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
    }

    MFX_CHECK(
        IsOn(extOptOld.FieldOutput) || extOptOld.FieldOutput == extOptNew.FieldOutput,
        MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);

    MFX_CHECK(
        IsOn(m_video.mfx.LowPower) == IsOn(newPar.mfx.LowPower),
        MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);


#if MFX_VERSION >= 1023
    // we can use feature only if sufs was allocated on init
    if (IsOn(extOpt3New.EnableMBForceIntra) && m_useMbControlSurfs == false)
        return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;
#endif


    if (IsOn(extOpt3New.FadeDetection) && !(m_cmCtx.get() && m_cmCtx->isHistogramSupported()))
        return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;

    if (bIntRateControlLA(m_video.mfx.RateControlMethod) )
    {
        MFX_CHECK(extOpt2Old.LookAheadDepth >= extOpt2New.LookAheadDepth,
                  MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
    }


    if (IsOn(extOpt2Old.ExtBRC))
    {
        mfxExtBRC & extBRCInit  = GetExtBufferRef(m_video);
        mfxExtBRC & extBRCReset = GetExtBufferRef(newPar);

        MFX_CHECK(
        extBRCInit.pthis        == extBRCReset.pthis        &&
        extBRCInit.Init         == extBRCReset.Init         &&
        extBRCInit.Reset        == extBRCReset.Reset        &&
        extBRCInit.Close        == extBRCReset.Close        &&
        extBRCInit.GetFrameCtrl == extBRCReset.GetFrameCtrl &&
        extBRCInit.Update       == extBRCReset.Update, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
    }


    return checkStatus;
} // ProcessAndCheckNewParameters

mfxStatus ImplementationAvc::Reset(mfxVideoParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;
    MFX_CHECK_NULL_PTR1(par);

    sts = CheckExtBufferId(*par);
    MFX_CHECK_STS(sts);

    MfxVideoParam newPar = *par;

    mfxExtCodingOption2 & extOpt2New = GetExtBufferRef(newPar);
    mfxExtCodingOption2 & extOpt2Old = GetExtBufferRef(m_video);

    bool isIdrRequired = false;
    bool isBRCReset = false;

    mfxStatus checkStatus = ProcessAndCheckNewParameters(newPar, isBRCReset, isIdrRequired, par);
    if (checkStatus < MFX_ERR_NONE)
        return checkStatus;

    mfxExtCodingOption3 & extOpt3New = GetExtBufferRef(newPar);
    mfxExtCodingOption3 & extOpt3Old = GetExtBufferRef(m_video);
    if (extOpt3New.NumSliceP != extOpt3Old.NumSliceP
        || ((extOpt2New.IntRefType != extOpt2Old.IntRefType) && (extOpt2New.IntRefType != 0))) // reset slice divider
    {
        bool fieldCoding = (newPar.mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_PROGRESSIVE) == 0;
        m_sliceDivider = MakeSliceDivider(
            (m_caps.ddi_caps.SliceLevelRateCtrl) ? SliceDividerType::ARBITRARY_MB_SLICE : SliceDividerType(m_caps.ddi_caps.SliceStructure),
            extOpt2New.NumMbPerSlice,
            extOpt3New.NumSliceP,
            newPar.mfx.FrameInfo.Width / 16,
            newPar.mfx.FrameInfo.Height / 16 / (fieldCoding ? 2 : 1));

        if (extOpt2New.IntRefType == MFX_REFRESH_SLICE)
            m_baseLayerOrderStartIntraRefresh = m_baseLayerOrder - 1;
    }

    // m_encoding contains few submitted and not queried tasks, wait for their completion
    for (DdiTaskIter i = m_encoding.begin(); i != m_encoding.end(); ++i)
        for (mfxU32 f = 0; f <= i->m_fieldPicFlag; f++)
            while ((sts = QueryStatus(*i, i->m_fid[f])) == MFX_TASK_BUSY)
                vm_time_sleep(1);
    while (!m_encoding.empty())
        OnEncodingQueried(m_encoding.begin());
    DestroyDanglingCmResources();

    mfxU32  adaptGopDelay = 0;
#if defined(MFX_ENABLE_ENCTOOLS)
    adaptGopDelay =  m_encTools.GetPreEncDelay(newPar);
#endif
    m_emulatorForSyncPart.Init(newPar, adaptGopDelay);
    m_emulatorForAsyncPart = m_emulatorForSyncPart;

    m_hrd.Reset(newPar);
    // to change m_video before DDI Reset
    ModifiedVideoParams mod_par;
    mod_par.ModifyForDDI(newPar, m_enabledSwBrc);
    m_ddi->Reset(newPar);
    mod_par.Restore(newPar);

    m_1stFieldStatus = MFX_ERR_NONE;
    m_fieldCounter   = 0;
    m_stagesToGo     = AsyncRoutineEmulator::STG_BIT_CALL_EMULATOR;
    m_bDeferredFrame = 0;

    mfxExtEncoderResetOption const & extResetOpt = GetExtBufferRef(newPar);

    // perform reset of encoder and start new sequence with IDR in following cases:
    // 1) change of encoding parameters require insertion of IDR
    // 2) application explicitly asked about starting new sequence
    if (isIdrRequired || IsOn(extResetOpt.StartNewSequence))
    {

        if (extOpt2Old.MaxSliceSize && m_lastTask.m_yuv && m_video.mfx.LowPower != MFX_CODINGOPTION_ON)
        {
            if (m_raw.Unlock(m_lastTask.m_idx) == (mfxU32)-1)
            {
               m_core->DecreaseReference(&m_lastTask.m_yuv->Data);
            }
            if (m_cmDevice)
            {
                m_cmDevice->DestroySurface(m_lastTask.m_cmRaw);
                m_lastTask.m_cmRaw = NULL;
            }
        }

        m_free.splice(m_free.end(), m_incoming);
        m_free.splice(m_free.end(), m_reordering);
        m_free.splice(m_free.end(), m_lookaheadStarted);
        m_free.splice(m_free.end(), m_lookaheadFinished);
        m_free.splice(m_free.end(), m_histRun);
        m_free.splice(m_free.end(), m_histWait);
        m_free.splice(m_free.end(), m_encoding);
        m_timeStamps.clear();

        for (DdiTaskIter i = m_free.begin(); i != m_free.end(); ++i)
        {
            if (i->m_yuv)
                m_core->DecreaseReference(&i->m_yuv->Data);
            *i = DdiTask();
        }

        Zero(m_stat);
        m_lastTask = DdiTask();

        m_frameOrder                  = 0;
        m_baseLayerOrder              = 0;
        m_frameOrderIdrInDisplayOrder = 0;
        m_frameOrderIntraInDisplayOrder = 0;
        m_frameOrderPyrStart = 0;
        m_frameOrderIPInDisplayOrder = 0;
        m_miniGopCount = 0;
        m_frameOrderStartTScalStructure = 0;

        m_1stFieldStatus = MFX_ERR_NONE;
        m_fieldCounter   = 0;

        m_raw.Unlock();
        m_rawLa.Unlock();
        m_mb.Unlock();
        m_rawSys.Unlock();
        m_rec.Unlock();
        m_bit.Unlock();

        m_recNonRef[0] = m_recNonRef[1] = 0xffffffff;

        // reset of Intra refresh
        if (extOpt2New.IntRefType)
        {
            if (extOpt2New.IntRefType == MFX_REFRESH_SLICE)
            {
                m_intraStripeWidthInMBs = 0;
            }
            else
            {
                mfxU16 refreshDimension = extOpt2New.IntRefType == MFX_REFRESH_HORIZONTAL ? m_video.mfx.FrameInfo.Height >> 4 : m_video.mfx.FrameInfo.Width >> 4;
                m_intraStripeWidthInMBs = (refreshDimension + extOpt2New.IntRefCycleSize - 1) / extOpt2New.IntRefCycleSize;
            }
            m_baseLayerOrderStartIntraRefresh = 0;
        }

    }
    else
    {
        if (m_video.calcParam.tempScalabilityMode && newPar.calcParam.tempScalabilityMode &&
            m_video.calcParam.numTemporalLayer != newPar.calcParam.numTemporalLayer)
        {
            // reset starting point of temporal scalability calculation if number of temporal layers was changed w/o IDR
            m_frameOrderStartTScalStructure = m_frameOrder;
        }

        if ((extOpt2New.IntRefType) && (extOpt2New.IntRefType != MFX_REFRESH_SLICE))
        {
            sts = UpdateIntraRefreshWithoutIDR(
                m_video,
                newPar,
                m_baseLayerOrder,
                m_baseLayerOrderStartIntraRefresh,
                m_baseLayerOrderStartIntraRefresh,
                m_intraStripeWidthInMBs,
                m_sliceDivider,
                m_caps);
            MFX_CHECK_STS(sts);
        }
    }
    m_video = newPar;
    bool bModParams = false;
    if (m_enabledSwBrc)
    {
        if (isIdrRequired)
        {
            mfxExtEncoderResetOption & resetOption = GetExtBufferRef(newPar);
            resetOption.StartNewSequence = MFX_CODINGOPTION_ON;
        }
        mod_par.ModifyForBRC(newPar, true);
        bModParams = true;
    }

#if defined(MFX_ENABLE_ENCTOOLS)
    if (H264EncTools::isEncToolNeeded(m_video))
    {
        sts = m_encTools.Reset(m_video);
        MFX_CHECK_STS(sts);
    }
    else
#endif
    if (m_enabledSwBrc)
    {
        sts = m_brc.Reset(newPar);
        MFX_CHECK_STS(sts);
    }
    if (bModParams)
        mod_par.Restore(newPar);
    return checkStatus;
}

mfxStatus ImplementationAvc::GetVideoParam(mfxVideoParam *par)
{
    MFX_CHECK_NULL_PTR1(par);

    // For buffers which are field-based
    std::map<mfxU32, mfxU32> buffers_offsets;

    for (mfxU32 i = 0; i < par->NumExtParam; i++)
    {
        if (buffers_offsets.find(par->ExtParam[i]->BufferId) == buffers_offsets.end())
            buffers_offsets[par->ExtParam[i]->BufferId] = 0;
        else
            buffers_offsets[par->ExtParam[i]->BufferId]++;


        if (mfxExtBuffer * buf = GetExtBuffer(m_video.ExtParam, m_video.NumExtParam, par->ExtParam[i]->BufferId, buffers_offsets[par->ExtParam[i]->BufferId]))
        {
            if (par->ExtParam[i]->BufferId == MFX_EXTBUFF_CODING_OPTION_SPSPPS)
            {
                // need to generate sps/pps nal units
                mfxExtCodingOptionSPSPPS * dst = (mfxExtCodingOptionSPSPPS *)par->ExtParam[i];

                mfxExtSpsHeader const & sps = GetExtBufferRef(m_video);
                mfxExtPpsHeader const & pps = GetExtBufferRef(m_video);

                try
                {
                    if (dst->SPSBuffer)
                    {
                        MFX_CHECK(dst->SPSBufSize, MFX_ERR_INVALID_VIDEO_PARAM);
                        OutputBitstream writerSps(dst->SPSBuffer, dst->SPSBufSize);
                        WriteSpsHeader(writerSps, sps);
                        dst->SPSBufSize = mfxU16((writerSps.GetNumBits() + 7) / 8);
                    }
                    if (dst->PPSBuffer)
                    {
                        MFX_CHECK(dst->PPSBufSize, MFX_ERR_INVALID_VIDEO_PARAM);
                        OutputBitstream writerPps(dst->PPSBuffer, dst->PPSBufSize);
                        WritePpsHeader(writerPps, pps);
                        dst->PPSBufSize = mfxU16((writerPps.GetNumBits() + 7) / 8);
                    }
                }
                catch (std::exception &)
                {
                    return MFX_ERR_INVALID_VIDEO_PARAM;
                }

                dst->SPSId = sps.seqParameterSetId;
                dst->PPSId = pps.picParameterSetId;
            }
            else
            {
                MFX_INTERNAL_CPY(par->ExtParam[i], buf, par->ExtParam[i]->BufferSz);
            }
        }
        else
        {
            return MFX_ERR_UNSUPPORTED;
        }
    }

    mfxExtBuffer ** ExtParam = par->ExtParam;
    mfxU16    NumExtParam = par->NumExtParam;

    MFX_INTERNAL_CPY(par, &(static_cast<mfxVideoParam &>(m_video)), sizeof(mfxVideoParam));

    par->ExtParam    = ExtParam;
    par->NumExtParam = NumExtParam;

    return MFX_ERR_NONE;
}

mfxStatus ImplementationAvc::GetFrameParam(mfxFrameParam *par)
{
    MFX_CHECK_NULL_PTR1(par);

    return MFX_ERR_UNSUPPORTED;
}

mfxStatus ImplementationAvc::GetEncodeStat(mfxEncodeStat *stat)
{
    MFX_CHECK_NULL_PTR1(stat);
    UMC::AutomaticUMCMutex guard(m_listMutex);
    *stat = m_stat;
    return MFX_ERR_NONE;
}

struct CompareByMidRec
{
    mfxMemId m_mid;
    CompareByMidRec(mfxMemId mid) : m_mid(mid) {}
    bool operator ()(DpbFrame const & frame) const { return frame.m_midRec == m_mid; }
};

struct CompareByMidRaw
{
    mfxMemId m_cmSurf;
    CompareByMidRaw(mfxMemId cmSurf) : m_cmSurf(cmSurf) {}
    bool operator ()(DpbFrame const & frame) const { return frame.m_cmRaw == m_cmSurf; }
};
struct CompareByFrameOrder
{
    mfxU32 m_FrameOrder;
    CompareByFrameOrder(mfxU32 frameOrder) : m_FrameOrder(frameOrder) {}
    bool operator ()(DpbFrame const & frame) const { return frame.m_frameOrder == m_FrameOrder; }
};

void ImplementationAvc::OnNewFrame()
{
    m_stagesToGo &= ~AsyncRoutineEmulator::STG_BIT_ACCEPT_FRAME;

    UMC::AutomaticUMCMutex guard(m_listMutex);
    m_reordering.splice(m_reordering.end(), m_incoming, m_incoming.begin());
}

void ImplementationAvc::SubmitScd()
{
    m_stagesToGo &= ~AsyncRoutineEmulator::STG_BIT_ACCEPT_FRAME;

    UMC::AutomaticUMCMutex guard(m_listMutex);
    m_ScDetectionStarted.splice(m_ScDetectionStarted.end(), m_incoming, m_incoming.begin());
}

void ImplementationAvc::OnScdQueried()
{
    m_stagesToGo &= ~AsyncRoutineEmulator::STG_BIT_START_SCD;

    UMC::AutomaticUMCMutex guard(m_listMutex);
    m_ScDetectionFinished.splice(m_ScDetectionFinished.end(), m_ScDetectionStarted, m_ScDetectionStarted.begin());
}

void ImplementationAvc::OnScdFinished()
{
    m_stagesToGo &= ~AsyncRoutineEmulator::STG_BIT_WAIT_SCD;

    UMC::AutomaticUMCMutex guard(m_listMutex);
    m_reordering.splice(m_reordering.end(), m_ScDetectionFinished, m_ScDetectionFinished.begin());
}

void ImplementationAvc::SubmitMCTF()
{
    m_stagesToGo &= ~AsyncRoutineEmulator::STG_BIT_WAIT_SCD;

    UMC::AutomaticUMCMutex guard(m_listMutex);
    m_MctfStarted.splice(m_MctfStarted.end(), m_ScDetectionFinished, m_ScDetectionFinished.begin());
}

void ImplementationAvc::OnMctfQueried()
{
    m_stagesToGo &= ~AsyncRoutineEmulator::STG_BIT_START_MCTF;

    UMC::AutomaticUMCMutex guard(m_listMutex);
    m_MctfFinished.splice(m_MctfFinished.end(), m_MctfStarted, m_MctfStarted.begin());
}

void ImplementationAvc::OnMctfFinished()
{
    m_stagesToGo &= ~AsyncRoutineEmulator::STG_BIT_WAIT_MCTF;

    UMC::AutomaticUMCMutex guard(m_listMutex);
    m_reordering.splice(m_reordering.end(), m_MctfFinished, m_MctfFinished.begin());
}

void ImplementationAvc::OnLookaheadSubmitted(DdiTaskIter task)
{
    m_stagesToGo &= ~AsyncRoutineEmulator::STG_BIT_START_LA;

    if (m_inputFrameType == MFX_IOPATTERN_IN_SYSTEM_MEMORY)
        m_core->DecreaseReference(&task->m_yuv->Data);
    m_lookaheadStarted.splice(m_lookaheadStarted.end(), m_reordering, task);
}


void ImplementationAvc::OnLookaheadQueried()
{
    m_stagesToGo &= ~AsyncRoutineEmulator::STG_BIT_WAIT_LA;

    DdiTask & task = m_lookaheadStarted.front();
    int fid = task.m_fid[0];
    mfxExtCodingOption2 & extOpt2 = GetExtBufferRef(m_video);

    if (extOpt2.MaxSliceSize == 0)
    {
        ArrayDpbFrame & iniDpb = task.m_dpb[fid];
        ArrayDpbFrame & finDpb = task.m_dpbPostEncoding;
        for (mfxU32 i = 0; i < iniDpb.Size(); i++)
        {
            // m_cmRaw is always filled
            if (std::find_if(finDpb.Begin(), finDpb.End(), CompareByFrameOrder(iniDpb[i].m_frameOrder)) == finDpb.End())
            {
                ReleaseResource(m_rawLa, iniDpb[i].m_cmRawLa);
                ReleaseResource(m_mb,    iniDpb[i].m_cmMb);
                if (m_cmDevice){

                    m_cmDevice->DestroySurface(iniDpb[i].m_cmRaw);
                    iniDpb[i].m_cmRaw = NULL;
                }
            }
        }
    }
    ReleaseResource(m_curbe, task.m_cmCurbe);

    if (m_cmDevice)
    {
        if(task.m_cmRefs)
        m_cmDevice->DestroyVmeSurfaceG7_5(task.m_cmRefs);
        if(task.m_cmRefsLa)
        m_cmDevice->DestroyVmeSurfaceG7_5(task.m_cmRefsLa);
        if(task.m_event)
        m_cmCtx->DestroyEvent(task.m_event);
    }

    if ((task.GetFrameType() & MFX_FRAMETYPE_REF) == 0)
    {
        ReleaseResource(m_rawLa, task.m_cmRawLa);
        ReleaseResource(m_mb,    task.m_cmMb);

        if (m_cmDevice)
        {
            m_cmDevice->DestroySurface(task.m_cmRaw);
            task.m_cmRaw = NULL;
        }
    }


    m_histRun.splice(m_histRun.end(), m_lookaheadStarted, m_lookaheadStarted.begin());
}

void ImplementationAvc::OnHistogramSubmitted()
{
    m_stagesToGo &= ~AsyncRoutineEmulator::STG_BIT_START_HIST;

    m_histWait.splice(m_histWait.end(), m_histRun, m_histRun.begin());
}

void ImplementationAvc::OnHistogramQueried()
{
    m_stagesToGo &= ~AsyncRoutineEmulator::STG_BIT_WAIT_HIST;

    DdiTask & task = m_histWait.front();

    int fid = task.m_fid[0];
    ArrayDpbFrame & iniDpb = task.m_dpb[fid];
    ArrayDpbFrame & finDpb = task.m_dpbPostEncoding;

    for (mfxU32 i = 0; i < iniDpb.Size(); i++)
    {
        if (std::find_if(finDpb.Begin(), finDpb.End(), CompareByFrameOrder(iniDpb[i].m_frameOrder)) == finDpb.End())
        {
            ReleaseResource(m_histogram, iniDpb[i].m_cmHist);
        }
    }

    if ((task.m_reference[0] + task.m_reference[1]) == 0)
    {
        ReleaseResource(m_histogram, task.m_cmHist);
    }

    if (m_cmDevice && task.m_cmRawForHist)
    {
        m_cmDevice->DestroySurface(task.m_cmRawForHist);
        task.m_cmRawForHist = 0;
    }

    if (m_cmCtx && task.m_event)
    {
        m_cmCtx->DestroyEvent(task.m_event);
        task.m_event = 0;
    }

    m_lookaheadFinished.splice(m_lookaheadFinished.end(), m_histWait, m_histWait.begin());
}


void ImplementationAvc::OnEncodingSubmitted(DdiTaskIter task)
{

    task->m_startTime = vm_time_get_current_time();

    MFX_TRACE_D(task->m_startTime);

    m_encoding.splice(m_encoding.end(), m_lookaheadFinished, task);
}


void ImplementationAvc::OnEncodingQueried(DdiTaskIter task)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "ImplementationAvc::OnEncodingQueried");
    m_stagesToGo &= ~AsyncRoutineEmulator::STG_BIT_WAIT_ENCODE;

    mfxExtCodingOption2 const & extOpt2 = GetExtBufferRef(m_video);

    int ffid = task->m_fid[0];
    ArrayDpbFrame const & iniDpb = task->m_dpb[ffid];
    ArrayDpbFrame const & finDpb = task->m_dpbPostEncoding;

    for (mfxU32 i = 0; i < iniDpb.Size(); i++)
    {
        if (std::find_if(finDpb.Begin(), finDpb.End(), CompareByMidRec(iniDpb[i].m_midRec)) == finDpb.End())
        {
            ReleaseResource(m_rec, iniDpb[i].m_midRec);

            if (IsOn(extOpt2.UseRawRef))
            {
                if (m_inputFrameType == MFX_IOPATTERN_IN_VIDEO_MEMORY)
                    m_core->DecreaseReference(&iniDpb[i].m_yuvRaw->Data);

                ReleaseResource(m_raw, iniDpb[i].m_midRaw);
                ReleaseResource(m_rawSkip, iniDpb[i].m_midRaw);
            }
        }
    }

    if (IsOff(extOpt2.UseRawRef) || (task->m_reference[0] + task->m_reference[1]) == 0)
    {
        if (m_inputFrameType == MFX_IOPATTERN_IN_VIDEO_MEMORY)
            m_core->DecreaseReference(&task->m_yuv->Data);

        ReleaseResource(m_raw, task->m_midRaw);
        ReleaseResource(m_rawSkip, task->m_midRaw);
    }

    ReleaseResource(m_bit, task->m_midBit[0]);
    ReleaseResource(m_bit, task->m_midBit[1]);
    if ((task->m_reference[0] + task->m_reference[1]) == 0)
        ReleaseResource(m_rec, task->m_midRec);

    if(m_useMBQPSurf && task->m_isMBQP)
        ReleaseResource(m_mbqp, task->m_midMBQP);

    if (m_useMbControlSurfs && task->m_isMBControl)
        ReleaseResource(m_mbControl, task->m_midMBControl);
#if defined(MFX_ENABLE_MCTF_IN_AVC)
    if (IsMctfSupported(m_video) && task->m_midMCTF)
    {
        ReleaseResource(m_mctf, task->m_midMCTF);
        task->m_midMCTF = MID_INVALID;
        MfxHwH264Encode::Zero(task->m_handleMCTF);
        if (m_cmDevice)
            m_cmDevice->DestroySurface(task->m_cmMCTF);
    }
#endif
    mfxU32 numBits = 8 * (task->m_bsDataLength[0] + task->m_bsDataLength[1]);
    *task = DdiTask();

    UMC::AutomaticUMCMutex guard(m_listMutex);

    m_stat.NumBit += numBits;
    m_stat.NumCachedFrame--;
    m_stat.NumFrame++;
#if defined (MFX_ENABLE_MFE)
    m_lastTask.m_endTime = vm_time_get_tick();
#endif
    m_free.splice(m_free.end(), m_encoding, task);
}


namespace
{
    void Change_DPB(
        ArrayDpbFrame &             dpb,
        mfxMemId const *            mids,
        std::vector<mfxU32> const & fo)
    {
        for (mfxU32 i = 0; i < dpb.Size(); i++)
        {
            mfxU32 idxRec = mfxU32(std::find(fo.begin(), fo.end(), dpb[i].m_frameOrder) - fo.begin());
            dpb[i].m_frameIdx = idxRec;
            dpb[i].m_midRec   = mids[idxRec];
        }
    }

    mfxStatus CountLeadingFF(
        VideoCORE & core,
        DdiTask &   task,
        mfxU32      fid)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "CountLeadingFF");
        mfxFrameData bitstream = {};

        task.m_numLeadingFF[fid] = 0;
        FrameLocker lock(&core, bitstream, task.m_midBit[fid]);
        if (bitstream.Y == 0)
            return Error(MFX_ERR_LOCK_MEMORY);

        mfxU32 skippedMax = std::min(15u, task.m_bsDataLength[fid]);
        while (*bitstream.Y == 0xff && task.m_numLeadingFF[fid] < skippedMax)
        {
            ++bitstream.Y;
            ++task.m_numLeadingFF[fid];
        }

        return MFX_ERR_NONE;
    }
}

void ImplementationAvc::BrcPreEnc(
    DdiTask const & task)
{
    mfxExtCodingOption2 const & extOpt2 = GetExtBufferRef(m_video);

    DdiTaskIter j = m_lookaheadFinished.begin();
    mfxU32 numLaFrames = (mfxU32)m_lookaheadFinished.size();
    while (j->m_encOrder != task.m_encOrder)
        ++j, --numLaFrames;

    numLaFrames = std::min<mfxU32>(extOpt2.LookAheadDepth, numLaFrames);

    m_tmpVmeData.resize(numLaFrames);
    for (size_t i = 0; i < m_tmpVmeData.size(); ++i, ++j)
        m_tmpVmeData[i] = j->m_vmeData;

    m_brc.PreEnc(task.m_brcFrameParams, m_tmpVmeData);
}
#if defined(MFX_ENABLE_MCTF_IN_AVC)
mfxStatus ImplementationAvc::SubmitToMctf(DdiTask * pTask)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_API, "VideoVPPHW::SubmitToMctf");

    pTask->m_bFrameReady = false;
    bool isIntraFrame    = (pTask->GetFrameType() & MFX_FRAMETYPE_I) || (pTask->GetFrameType() & MFX_FRAMETYPE_IDR);
    bool isPFrame        = (pTask->GetFrameType() & MFX_FRAMETYPE_P);
    bool isAnchorFrame   = isIntraFrame || isPFrame;

    amtMctf->IntBufferUpdate(pTask->m_SceneChange, isIntraFrame, pTask->m_doMCTFIntraFiltering);
    if (isAnchorFrame || pTask->m_SceneChange)
    {
        pTask->m_idxMCTF = FindFreeResourceIndex(m_mctf);
        pTask->m_midMCTF = AcquireResource(m_mctf, pTask->m_idxMCTF);
        if (pTask->m_midMCTF)
        {
            MFX_SAFE_CALL(m_core->GetFrameHDL(pTask->m_midMCTF, &pTask->m_handleMCTF.first));
        }
        else
            isAnchorFrame = false; //No resource available to generate filtered output, let it pass.
    }
    if (IsCmNeededForSCD(m_video))
    {
        MFX_SAFE_CALL(amtMctf->MCTF_PUT_FRAME(
            pTask->m_yuv,
            pTask->m_handleMCTF,
            &pTask->m_cmMCTF,
            true,
            nullptr,
            isAnchorFrame,
            pTask->m_doMCTFIntraFiltering));
    }
    else
    {
        mfxFrameData
            pData = pTask->m_yuv->Data;
        FrameLocker
            lock2(m_core, pData, true);
        if (pData.Y == 0)
            return Error(MFX_ERR_LOCK_MEMORY);

        MFX_SAFE_CALL(amtMctf->MCTF_PUT_FRAME(
            pData.Y,
            pTask->m_handleMCTF,
            &pTask->m_cmMCTF,
            false,
            nullptr,
            isAnchorFrame,
            pTask->m_doMCTFIntraFiltering));
    }

    // --- access to the internal MCTF queue to increase buffer_count: no need to protect by mutex, as 1 writer & 1 reader
    MFX_SAFE_CALL(amtMctf->MCTF_UpdateBufferCount());

    // filtering itself
    MFX_SAFE_CALL(amtMctf->MCTF_DO_FILTERING_IN_AVC());

    return MFX_ERR_NONE;
}

mfxStatus ImplementationAvc::QueryFromMctf(void *pParam)
{
    DdiTask *pTask = (DdiTask*)pParam;
    MFX_CHECK_NULL_PTR1(pTask);

    pTask->m_bFrameReady = amtMctf->MCTF_ReadyToOutput();
    //Check if noise analysis determined if filter is not neeeded and free resources and handle
    if (!amtMctf->MCTF_CHECK_FILTER_USE() && (pTask->m_handleMCTF.first))
    {
        ReleaseResource(m_mctf, pTask->m_midMCTF);
        pTask->m_midMCTF = MID_INVALID;
        MfxHwH264Encode::Zero(pTask->m_handleMCTF);
        if (m_cmDevice)
            m_cmDevice->DestroySurface(pTask->m_cmMCTF);
    }
    MFX_SAFE_CALL(amtMctf->MCTF_RELEASE_FRAME(IsCmNeededForSCD(m_video)));

    return MFX_ERR_NONE;
}
#endif
using namespace ns_asc;
mfxStatus ImplementationAvc::SCD_Put_Frame(DdiTask & task)
{
    task.m_SceneChange = false;
    mfxFrameSurface1 *pSurfI = nullptr;
    pSurfI = m_core->GetNativeSurface(task.m_yuv);
    pSurfI = pSurfI ? pSurfI : task.m_yuv;
    mfxHDLPair handle = { nullptr,nullptr };
    if (IsCmNeededForSCD(m_video))
    {
        if (m_video.IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY)
        {
            MFX_SAFE_CALL(m_core->GetFrameHDL(pSurfI->Data.MemId, reinterpret_cast<mfxHDL*>(&handle)));
        }
        else
        {
            MFX_SAFE_CALL(m_core->GetExternalFrameHDL(pSurfI->Data.MemId, reinterpret_cast<mfxHDL*>(&handle), false));
        }

        mfxHDLPair cmScdSurf = { nullptr,nullptr };
        task.m_idxScd = FindFreeResourceIndex(m_scd);
        cmScdSurf = AcquireResourceUp(m_scd, task.m_idxScd);
        if (cmScdSurf.first != nullptr && cmScdSurf.second != nullptr && task.m_wsGpuImage == nullptr)
        {
            task.m_wsGpuImage = (CmSurface2DUP *)cmScdSurf.first;
            task.m_wsGpuImage->GetIndex(task.m_wsIdxGpuImage);
            task.m_Yscd = (mfxU8*)cmScdSurf.second;
            task.m_wsSubSamplingEv = nullptr;
        }
        else
            return MFX_ERR_DEVICE_FAILED;
        MFX_SAFE_CALL(amtScd.QueueFrameProgressive(handle.first, task.m_wsIdxGpuImage, &task.m_wsSubSamplingEv, &task.m_wsSubSamplingTask));
    }
    else
    {
        mfxFrameData pData = task.m_yuv->Data;
        FrameLocker lock2(m_core, pData, true);
        if (pData.Y == 0)
            return Error(MFX_ERR_LOCK_MEMORY);
        task.m_idxScd = FindFreeResourceIndex(m_scd);
        task.m_Yscd = (mfxU8*)m_scd.GetSysmemBuffer(task.m_idxScd);
        MFX_SAFE_CALL(amtScd.PutFrameProgressive(pData.Y, pData.Pitch));
    }
    return MFX_ERR_NONE;
}
constexpr mfxU32 MAX_PYR_SIZE = 4;
inline bool bIsRefValid(mfxU32 frameOrder, mfxU32 distFromIntra, mfxU32 diff)
{
    return (distFromIntra >= diff && frameOrder >= diff);
}
inline void AddRefInPreferredList(DdiTask & task, mfxU32 distFromIntra, mfxU32 diff, mfxU32 &numPrefered)
{
    if (bIsRefValid(task.m_frameOrder, distFromIntra, diff) && numPrefered < 8)
    {
        task.m_internalListCtrl.PreferredRefList[numPrefered].FrameOrder = task.m_frameOrder - diff;
        task.m_internalListCtrl.PreferredRefList[numPrefered].PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
        numPrefered++;
    }
}
inline void AddRefInRejectedList(DdiTask & task, mfxU32 distFromIntra, mfxU32 diff, mfxU32 &numRejected)
{
    if (bIsRefValid(task.m_frameOrder, distFromIntra, diff) && numRejected < 8)
    {
        task.m_internalListCtrl.RejectedRefList[numRejected].FrameOrder = task.m_frameOrder - diff;
        task.m_internalListCtrl.RejectedRefList[numRejected].PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
        numRejected++;
    }
}
static void StartNewPyr(DdiTask & task, DdiTask & lastTask, mfxU32 numRefActiveP, bool bLastFrameUsing, mfxU32 &numRejected)
{
    mfxU32 maxKeyRef = numRefActiveP;

    std::vector <mfxU32> keyRefs;
    bool bNonKeyLastRef = false;

    for (mfxU32 i = 0; i < lastTask.m_dpbPostEncoding.Size(); i++)
    {
        mfxU32 diff = task.m_frameOrder - lastTask.m_dpbPostEncoding[i].m_frameOrder;
        if (lastTask.m_dpbPostEncoding[i].m_keyRef)
        {
            //store key frames list
            keyRefs.push_back(task.m_frameOrder - lastTask.m_dpbPostEncoding[i].m_frameOrder);
        }
        else if (!bLastFrameUsing || diff != 1)
        {
            //reject non key frames exept previous frame (if bLastFrameUsing mode)
            task.m_internalListCtrl.RejectedRefList[numRejected].FrameOrder = lastTask.m_dpbPostEncoding[i].m_frameOrder;
            task.m_internalListCtrl.RejectedRefList[numRejected].PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
            numRejected++;
        }
        else
            bNonKeyLastRef = true;
    }

    if (maxKeyRef > 1 && bNonKeyLastRef)
        maxKeyRef--;

    if (keyRefs.size() > maxKeyRef)
    {
        std::sort(keyRefs.begin(), keyRefs.end());
        for (mfxU32 i = maxKeyRef; i < keyRefs.size(); i++)
        {
            task.m_internalListCtrl.RejectedRefList[numRejected].FrameOrder = task.m_frameOrder - keyRefs[i];
            task.m_internalListCtrl.RejectedRefList[numRejected].PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
            numRejected++;
        }
    }
}
mfxStatus ImplementationAvc::BuildPPyr(DdiTask & task, mfxU32 pyrWidth, bool bLastFrameUsing, bool bResetPyr)
{
    mfxExtCodingOption3 const   & extOpt3 = GetExtBufferRef(m_video);
    mfxExtCodingOptionDDI const & extOptDDI = GetExtBufferRef(m_video);
    MFX_CHECK(m_video.mfx.GopRefDist == 1 &&
        extOpt3.PRefType == MFX_P_REF_PYRAMID &&
        pyrWidth > 0 && pyrWidth <= MAX_PYR_SIZE
        &&!IsOn(extOpt3.ExtBrcAdaptiveLTR),
        MFX_ERR_NONE);

    mfxExtAVCRefListCtrl * ctrl = GetExtBuffer(task.m_ctrl);
    mfxExtAVCRefLists * advCtrl = GetExtBuffer(task.m_ctrl, task.m_fid[0]);
    MFX_CHECK(ctrl == NULL && advCtrl == NULL, MFX_ERR_NONE);

    if (bResetPyr)
        m_frameOrderPyrStart = task.m_frameOrder;

    mfxU32 distFromIntra = task.m_frameOrder - m_frameOrderIntraInDisplayOrder;
    mfxU32 distFromPyrStart = task.m_frameOrder - m_frameOrderPyrStart;
    mfxU32 posInPyr = distFromPyrStart % pyrWidth;

    InitExtBufHeader(task.m_internalListCtrl);
    task.m_internalListCtrlPresent = true;
    if (posInPyr == 0)
    {
        task.m_keyReference = 1;
        mfxU32 numRejected = 0;
        StartNewPyr(task, m_lastTask, extOptDDI.NumActiveRefP, bLastFrameUsing, numRejected);
    }
    else
    {
        // frameOrder difference between current frame and Reference frame
        mfxU32 refDiff[MAX_PYR_SIZE - 1][MAX_PYR_SIZE - 1][2] = {
             {{1,pyrWidth + 1}, {0,0}, {0,0}},//pyrWidth == 2
             {{1,pyrWidth + 1}, {2,pyrWidth + 2}, {0,0}},//pyrWidth == 3
             {{1,pyrWidth + 1}, {2,pyrWidth + 2}, {1,3}} }; //pyrWidth == 4

        mfxU32 refDiffLastFrame[MAX_PYR_SIZE - 1][MAX_PYR_SIZE - 1][2] = {
             {{1,pyrWidth + 1}, {0,0}, {0,0}},//pyrWidth == 2
             {{1,pyrWidth + 1}, {1,2}, {0,0}},//pyrWidth == 3
             {{1,pyrWidth + 1}, {1,2}, {1,3}} }; //pyrWidth == 4

        mfxU32 rejDiff[MAX_PYR_SIZE - 1][MAX_PYR_SIZE - 1] = {
            {0, 0, 0},//pyrWidth == 2
            {0, 1, 0},//pyrWidth == 3
            {0, 1, 1} }; //pyrWidth == 4

        mfxI32 rejDiffLastFrame[MAX_PYR_SIZE - 1][MAX_PYR_SIZE - 1] = {
            {0 , 0, 0},//pyrWidth == 2
            {0,  1, 0},//pyrWidth == 3
            {0,  1, 1} }; //pyrWidth == 4

        mfxU32 layer[MAX_PYR_SIZE - 1][MAX_PYR_SIZE - 1]
        {
            {1,0,0},
            {2,1,0},
            {2,1,2}
        };


        mfxU32 numPrefered = 0;
        mfxU32 numRejected = 0;

        for (int i = 0; i < 2; i++)
        {
            mfxU32 diff = bLastFrameUsing ?
                refDiffLastFrame[pyrWidth - 2][posInPyr - 1][i] :
                refDiff[pyrWidth - 2][posInPyr - 1][i];
            AddRefInPreferredList(task, distFromIntra, diff, numPrefered);
        }
        {
            mfxU32 diff = bLastFrameUsing ?
                rejDiffLastFrame[pyrWidth - 2][posInPyr - 1] :
                rejDiff[pyrWidth - 2][posInPyr - 1];
            AddRefInRejectedList(task, distFromIntra, diff, numRejected);
        }
        task.m_LowDelayPyramidLayer = layer[pyrWidth - 2][posInPyr - 1];
        if (IsOn(extOpt3.EnableQPOffset))
        {
            task.m_QPdelta = extOpt3.QPOffset[layer[pyrWidth - 2][posInPyr - 1]];
            task.m_bQPDelta = true;
        }
    }
    return MFX_ERR_NONE;
}


mfxStatus ImplementationAvc::SCD_Get_FrameType(DdiTask & task)
{
    if (task.m_wsSubSamplingEv)
    {
        MFX_SAFE_CALL(amtScd.ProcessQueuedFrame(&task.m_wsSubSamplingEv, &task.m_wsSubSamplingTask, &task.m_wsGpuImage, &task.m_Yscd));
        ReleaseResource(m_scd, (mfxHDL)task.m_wsGpuImage);
    }
    mfxExtCodingOption2 const & extOpt2 = GetExtBufferRef(m_video);
    mfxExtCodingOption3 const & extOpt3 = GetExtBufferRef(m_video);
    task.m_SceneChange = amtScd.Get_frame_shot_Decision();
#if defined(MFX_ENABLE_MCTF_IN_AVC)
    task.m_doMCTFIntraFiltering = amtScd.Get_intra_frame_denoise_recommendation();
#endif

    if (extOpt3.PRefType == MFX_P_REF_PYRAMID)
    {
        // Adaptive low Delay Quantizer settings for extbrc
        m_LowDelayPyramidLayer = (!(task.m_type[0] & MFX_FRAMETYPE_P) || task.m_SceneChange) ? 0
            : ((amtScd.Get_PDist_advice() > 1 || amtScd.Get_RepeatedFrame_advice()) ? (m_LowDelayPyramidLayer ? 0 : 1) : 0);
        task.m_LowDelayPyramidLayer = m_LowDelayPyramidLayer;
    }
    else
        task.m_LowDelayPyramidLayer = m_LowDelayPyramidLayer = 0;

    if(task.m_SceneChange)
    {
        bool bPyr = extOpt2.BRefType == MFX_B_REF_PYRAMID;

        if (IsOn(extOpt2.AdaptiveI))
        {
            mfxI32 idist   = (mfxI32)(task.m_frameOrder - m_frameOrderIntraInDisplayOrder);
            mfxI32 idrdist = (mfxI32)(task.m_frameOrder - m_frameOrderIdrInDisplayOrder);

            mfxExtCodingOptionDDI const * extDdi = GetExtBuffer(m_video);
            MFX_CHECK_NULL_PTR1(extDdi);

            mfxI32 numRef     = std::min(extDdi->NumActiveRefP, m_video.mfx.NumRefFrame);
            mfxI32 minPDist   = numRef * m_video.mfx.GopRefDist;
            mfxI32 minIdrDist = (task.m_frameLtrOff ? numRef : std::max(8, numRef)) * (bPyr ? 2 : m_video.mfx.GopRefDist);

            minIdrDist = std::min(minIdrDist, m_video.mfx.GopPicSize/2);
            minPDist   = std::min(minPDist, minIdrDist);

            if (!(task.m_type[0] & MFX_FRAMETYPE_I) && idist < minPDist && IsOn(extOpt2.AdaptiveB))
            {
                // inside ref list, B to P but Dont break Pyr Structure
                if (!bPyr)
                {
                    task.m_ctrl.FrameType = (MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF);
                    task.m_type = ExtendFrameType(task.m_ctrl.FrameType);
                }
            }
            else if (!(task.m_type[0] & MFX_FRAMETYPE_IDR) && idrdist < minIdrDist)
            {
                // outside ref list, B/P to I but Dont break Pyr Structure
                if (!bPyr)
                {
                    task.m_ctrl.FrameType = (MFX_FRAMETYPE_I | MFX_FRAMETYPE_REF);
                    task.m_type = ExtendFrameType(task.m_ctrl.FrameType);
                }
            }
            else
            {
                // IDR
                task.m_ctrl.FrameType = (MFX_FRAMETYPE_I | MFX_FRAMETYPE_REF | MFX_FRAMETYPE_IDR);
                task.m_type = ExtendFrameType(task.m_ctrl.FrameType);
             }
        }
        else if (IsOn(extOpt2.AdaptiveB))
        {
            if (!(task.m_type[0] & MFX_FRAMETYPE_I))
            {
                // B to P but Dont break Pyr Structure
                if (!bPyr)
                {
                    task.m_ctrl.FrameType = (MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF);
                    task.m_type = ExtendFrameType(task.m_ctrl.FrameType);
                }
            }
        }
    }
    return MFX_ERR_NONE;
}

using namespace ns_asc;
mfxStatus ImplementationAvc::Prd_LTR_Operation(DdiTask & task)
{
    if (task.m_wsSubSamplingEv)
    {
        MFX_SAFE_CALL(amtScd.ProcessQueuedFrame(&task.m_wsSubSamplingEv, &task.m_wsSubSamplingTask, &task.m_wsGpuImage, &task.m_Yscd));
        m_scd.UpdateResourcePointers(task.m_idxScd, (void *)task.m_Yscd, (void *)task.m_wsGpuImage);
        ReleaseResource(m_scd, (mfxHDL)task.m_wsGpuImage);
    }
    task.m_frameLtrReassign = 0;
    task.m_LtrOrder = m_LtrOrder;
    task.m_RefOrder = m_RefOrder;
    task.m_LtrQp = m_LtrQp;
    task.m_RefQp = m_RefQp;
    task.m_frameLtrOff = 1;

    if (IsAdaptiveLtrOn(m_video))
    {
        ASC_LTR_DEC scd_LTR_hint = NO_LTR;
        MFX_SAFE_CALL(amtScd.get_LTR_op_hint(scd_LTR_hint));
        task.m_frameLtrOff = (scd_LTR_hint == NO_LTR);

        if (m_LtrQp && m_RefQp && m_RefQp < m_LtrQp - 1 && m_LtrOrder >= 0 && m_RefOrder > m_LtrOrder)
        {
            task.m_frameLtrReassign = 1;
        }
    }
    return MFX_ERR_NONE;
}

mfxStatus ImplementationAvc::CalculateFrameCmplx(DdiTask const &task, mfxU32 &raca128)
{
    mfxFrameSurface1 *pSurfI = nullptr;
    pSurfI = m_core->GetNativeSurface(task.m_yuv);
    pSurfI = pSurfI ? pSurfI : task.m_yuv;
    mfxHDLPair handle = { nullptr,nullptr };
    mfxF64 raca = 0;
    // Raca = l2 norm of average abs row diff and average abs col diff
    raca128 = 0;

    if (IsCmNeededForSCD(m_video))
    {
        if (m_video.IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY)
        {
            MFX_SAFE_CALL(m_core->GetFrameHDL(pSurfI->Data.MemId, reinterpret_cast<mfxHDL*>(&handle)));
        }
        else
        {
            MFX_SAFE_CALL(m_core->GetExternalFrameHDL(pSurfI->Data.MemId, reinterpret_cast<mfxHDL*>(&handle), false));
        }
        MFX_SAFE_CALL(amtScd.calc_RaCa_Surf(handle.first, raca));
    }
    else
    {
        mfxFrameData Data = task.m_yuv->Data;
        mfxFrameInfo Info = task.m_yuv->Info;

        FrameLocker lock2(m_core, Data, true);
        if (Data.Y == 0)
            return Error(MFX_ERR_LOCK_MEMORY);
        mfxI32 w, h, pitch;
        mfxU8 *ptr;

        if (Info.CropH > 0 && Info.CropW > 0)
        {
            w = Info.CropW;
            h = Info.CropH;
        }
        else
        {
            w = Info.Width;
            h = Info.Height;
        }

        pitch = Data.Pitch;
        ptr = Data.Y + Info.CropX + Info.CropY * pitch;

        if (ptr)
            MFX_SAFE_CALL(amtScd.calc_RaCa_pic(ptr, w, h, Data.Pitch, raca));
    }

    if (raca < BRC_MIN_RACA) raca = BRC_MIN_RACA;
    if (raca > BRC_MAX_RACA) raca = BRC_MAX_RACA;
    raca128 = (mfxU16)(raca * BRC_RACA_SCALE);

    return MFX_ERR_NONE;
}

struct FindNonSkip
{
    FindNonSkip(mfxU16 skipFrameMode) : m_skipFrameMode(skipFrameMode) {}

    template <class T> bool operator ()(T const & task) const
    {
        return (m_skipFrameMode != MFX_SKIPFRAME_INSERT_NOTHING &&
            m_skipFrameMode != MFX_SKIPFRAME_INSERT_DUMMY) ||
            task.m_ctrl.SkipFrame == 0;
    }

    mfxU16 m_skipFrameMode;
};
mfxStatus FixForcedFrameType (DdiTask & newTask, mfxU32 frameOrder)
{
    if (frameOrder == 0 &&
        !(newTask.m_type[0] & MFX_FRAMETYPE_IDR) &&
        !(newTask.m_type[1] & MFX_FRAMETYPE_IDR))
    {
        if (newTask.m_type[0] & MFX_FRAMETYPE_I)
            newTask.m_type[0] |= MFX_FRAMETYPE_IDR;
        else if (newTask.m_type[1] & MFX_FRAMETYPE_I)
            newTask.m_type[1] |= MFX_FRAMETYPE_IDR;
        else
            return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    newTask.m_type[0] |= MFX_FRAMETYPE_REF;
    newTask.m_type[1] |= MFX_FRAMETYPE_REF;

    return MFX_ERR_NONE;
}

void ImplementationAvc::AssignFrameTypes(DdiTask & newTask)
{
    //newTask.m_frameOrder = m_frameOrder;
    newTask.m_picStruct = GetPicStruct(m_video, newTask);
    newTask.m_fieldPicFlag = newTask.m_picStruct[ENC] != MFX_PICSTRUCT_PROGRESSIVE;
    newTask.m_fid[0] = newTask.m_picStruct[ENC] == MFX_PICSTRUCT_FIELD_BFF;
    newTask.m_fid[1] = newTask.m_fieldPicFlag - newTask.m_fid[0];

    newTask.m_baseLayerOrder = m_baseLayerOrder;

    if (newTask.m_picStruct[ENC] == MFX_PICSTRUCT_FIELD_BFF)
        std::swap(newTask.m_type.top, newTask.m_type.bot);

    newTask.m_frameOrderIdrInDisplayOrder = m_frameOrderIdrInDisplayOrder;
    newTask.m_frameOrderStartTScalStructure = m_frameOrderStartTScalStructure;

    if (newTask.GetFrameType() & MFX_FRAMETYPE_B)
    {
        newTask.m_loc = GetBiFrameLocation(m_video, newTask.m_frameOrder - m_frameOrderIPInDisplayOrder, newTask.m_currGopRefDist,m_miniGopCount);
        newTask.m_type[0] |= newTask.m_loc.refFrameFlag;
        newTask.m_type[1] |= newTask.m_loc.refFrameFlag;
    }
    if ((newTask.GetFrameType() & MFX_FRAMETYPE_I) ||
        (newTask.GetFrameType() & MFX_FRAMETYPE_P) ||
        (newTask.GetFrameType() & MFX_FRAMETYPE_IDR))
    {
        m_frameOrderIPInDisplayOrder = newTask.m_frameOrder;

        m_miniGopCount++;
    }


    if (newTask.GetFrameType() & MFX_FRAMETYPE_IDR)
    {
        m_frameOrderIdrInDisplayOrder = newTask.m_frameOrder;
        m_frameOrderStartTScalStructure = m_frameOrderIdrInDisplayOrder; // IDR always starts new temporal scalabilty structure
        newTask.m_frameOrderStartTScalStructure = m_frameOrderStartTScalStructure;
        m_frameOrderPyrStart = newTask.m_frameOrder;
        m_miniGopCount = 0;
    }

    if (newTask.GetFrameType() & MFX_FRAMETYPE_I)
    {
        m_frameOrderIntraInDisplayOrder = newTask.m_frameOrder;
        m_baseLayerOrderStartIntraRefresh = newTask.m_baseLayerOrder;
        m_frameOrderPyrStart = newTask.m_frameOrder;
    }

}
mfxU32 GetMaxFrameSize(DdiTask const & task, MfxVideoParam const &video, Hrd const &hrd)
{
    mfxExtCodingOption2    const & extOpt2 = GetExtBufferRef(video);
    mfxExtCodingOption3    const & extOpt3 = GetExtBufferRef(video);

    mfxU32 maxFrameSize_hrd = hrd.GetMaxFrameSize(task.GetFrameType() & MFX_FRAMETYPE_IDR)>>3;
    mfxU32 maxFrameSize     = (task.GetFrameType() & MFX_FRAMETYPE_I) ? extOpt3.MaxFrameSizeI : extOpt3.MaxFrameSizeP;
    maxFrameSize = (!maxFrameSize) ? extOpt2.MaxFrameSize : maxFrameSize;

    return (maxFrameSize_hrd && maxFrameSize) ?
        std::min(maxFrameSize_hrd,maxFrameSize):
        std::max(maxFrameSize_hrd,maxFrameSize);
}
inline void GetHRDParamFromBRC(DdiTask  & task, mfxU32 &initCpbRemoval, mfxU32 &initCpbRemovalOffset)
{
    (void)task;
    (void)initCpbRemoval;
    (void)initCpbRemovalOffset;

#if (MFX_VERSION >= 1029)
    if (task.m_brcFrameCtrl.InitialCpbRemovalDelay ||
        task.m_brcFrameCtrl.InitialCpbRemovalOffset)
    {
        initCpbRemoval = task.m_brcFrameCtrl.InitialCpbRemovalDelay;
        initCpbRemovalOffset = task.m_brcFrameCtrl.InitialCpbRemovalOffset;
    }
#endif
}
inline void UpdateBRCParams(DdiTask  & task)
{
    task.m_cqpValue[0] = task.m_cqpValue[1] = (mfxU8)task.m_brcFrameCtrl.QpY;
    GetHRDParamFromBRC(task, task.m_initCpbRemoval, task.m_initCpbRemovalOffset);
}

void ImplementationAvc::PreserveTimeStamp(mfxU64 timeStamp)
{
    // insert the unknown time stamp in the list end.
    if (timeStamp == static_cast<mfxU64>(MFX_TIMESTAMP_UNKNOWN))
    {
        m_timeStamps.push_back(timeStamp);
        return;
    }
    // insert the valid time stamp in the increasing order.
    auto it = std::find_if(std::begin(m_timeStamps), std::end(m_timeStamps),
                 [timeStamp](mfxU64 currTimeStamp)
                 {
                     return (currTimeStamp  != static_cast<mfxU64>(MFX_TIMESTAMP_UNKNOWN))
                         && (static_cast<mfxI64>(currTimeStamp) > static_cast<mfxI64>(timeStamp));
                 });

    m_timeStamps.insert(it, timeStamp);
}

void ImplementationAvc::AssignDecodeTimeStamp(DdiTask & task)
{
    mfxU8 numReorderFrames = GetNumReorderFrames(m_video);
    if (task.m_encOrder < numReorderFrames)
    {
        // For the first reorder frames the DTS is calculated from the first frame's timestamp
        // DTS = firstTimeStamp - distanceToFirstFrameInTime
        mfxU64 firstTimeStamp = m_timeStamps.front();
        mfxU32 distToFirstTimeStamp = numReorderFrames-task.m_encOrder;
        mfxU16 distInTickUnit = static_cast<mfxU16>(2 * distToFirstTimeStamp);
        task.m_decodeTimeStamp =  CalcDTSFromPTS(m_video.mfx.FrameInfo, distInTickUnit, firstTimeStamp);
    }
    else
    {
        task.m_decodeTimeStamp = static_cast<mfxI64>(m_timeStamps.front());
        m_timeStamps.pop_front();
    }
}

mfxStatus ImplementationAvc::CheckSliceSize(DdiTask &task, bool &bToRecode)
{
    mfxU32   bsSizeAvail = mfxU32(m_tmpBsBuf.size());
    mfxU8    *pBS = &m_tmpBsBuf[0];
    mfxStatus sts = MFX_ERR_NONE;
    mfxExtCodingOption2    const & extOpt2 = GetExtBufferRef(m_video);

    bToRecode = false;

    MFX_CHECK(task.m_fieldPicFlag == 0, MFX_ERR_UNDEFINED_BEHAVIOR);



    if ((sts = CopyBitstream(*m_core, m_video, task, task.m_fid[0], pBS, bsSizeAvail)) != MFX_ERR_NONE)
        return Error(sts);

    sts = UpdateSliceInfo(pBS, pBS + task.m_bsDataLength[task.m_fid[0]], extOpt2.MaxSliceSize, task, bToRecode);
    if (sts != MFX_ERR_NONE)
        return Error(sts);

    if (bToRecode)
    {
        if (task.m_repack == 0)
        {
            sts = CorrectSliceInfo(task, 70, m_video.calcParam.widthLa, m_video.calcParam.heightLa);
            if (sts != MFX_ERR_NONE && sts != MFX_ERR_UNDEFINED_BEHAVIOR)
                return Error(sts);
            if (sts == MFX_ERR_UNDEFINED_BEHAVIOR)
                task.m_repack = 1;
        }
        if (task.m_repack > 0)
        {
            if (task.m_repack > 5 && task.m_SliceInfo.size() > 255)
            {
                sts = CorrectSliceInfo(task, 70, m_video.calcParam.widthLa, m_video.calcParam.heightLa);
                if (sts != MFX_ERR_NONE && sts != MFX_ERR_UNDEFINED_BEHAVIOR)
                    return Error(sts);
            }
            else
            {
                size_t old_slice_size = task.m_SliceInfo.size();
                sts = CorrectSliceInfoForsed(task, m_video.calcParam.widthLa, m_video.calcParam.heightLa);
                if (sts != MFX_ERR_NONE)
                    return Error(sts);
                if (old_slice_size == task.m_SliceInfo.size() && task.m_repack < 4)
                    task.m_repack = 4;
            }
        }
        if (task.m_repack >= 4)
        {
            if (task.m_cqpValue[0] < 51)
            {
                task.m_cqpValue[0] = task.m_cqpValue[0] + 1 + (mfxU8)(task.m_repack - 4);
                if (task.m_cqpValue[0] > 51)
                    task.m_cqpValue[0] = 51;
                task.m_cqpValue[1] = task.m_cqpValue[0];
            }
            else if (task.m_SliceInfo.size() > 255)
                return MFX_ERR_UNDEFINED_BEHAVIOR;
        }
    }
    return sts;
}
mfxStatus ImplementationAvc::CheckBufferSize(DdiTask &task, bool &bToRecode, mfxU32 bsDataLength, mfxBitstream * bs)
{
    if ((bsDataLength > (bs->MaxLength - bs->DataOffset - bs->DataLength)))
    {
        if (task.m_cqpValue[0] == 51)
            return Error(MFX_ERR_UNDEFINED_BEHAVIOR);
        task.m_cqpValue[0] = task.m_cqpValue[0] + 1;
        task.m_cqpValue[1] = task.m_cqpValue[0];
        // printf("Recoding 0: frame %d, qp %d\n", task->m_frameOrder, task->m_cqpValue[0]);
        bToRecode = true;
    }
    return MFX_ERR_NONE;
}
mfxStatus ImplementationAvc::CheckBRCStatus(DdiTask &task, bool &bToRecode, mfxU32 bsDataLength)
{
    mfxExtCodingOption2    const & extOpt2 = GetExtBufferRef(m_video);
    task.m_brcFrameParams.CodedFrameSize = bsDataLength;
    mfxU32 res;

#if defined(MFX_ENABLE_ENCTOOLS)
    mfxBRCFrameStatus frame_sts = {};
    mfxStatus ests;
    if (m_enabledEncTools)
    {
        ests = m_encTools.SubmitEncodeResult(&task.m_brcFrameParams, task.m_brcFrameCtrl.QpY);
        MFX_CHECK_STS(ests);

        ests = m_encTools.GetEncodeStatus(&frame_sts, task.m_frameOrder);

        switch (frame_sts.BRCStatus)
        {
        case ::MFX_BRC_OK:
            res = UMC::BRC_OK;
            break;
        case ::MFX_BRC_BIG_FRAME:
            res = UMC::BRC_ERR_BIG_FRAME;
            break;
        case ::MFX_BRC_SMALL_FRAME:
            res = UMC::BRC_ERR_SMALL_FRAME;
            break;
        case ::MFX_BRC_PANIC_BIG_FRAME:
            res = UMC::BRC_ERR_BIG_FRAME | UMC::BRC_NOT_ENOUGH_BUFFER;
            break;
        case ::MFX_BRC_PANIC_SMALL_FRAME:
            res = UMC::BRC_ERR_SMALL_FRAME | UMC::BRC_NOT_ENOUGH_BUFFER;
            break;
        default:
            res = UMC::BRC_OK;
        }
    }
    else
#endif
    res = m_brc.Report(task.m_brcFrameParams, 0, GetMaxFrameSize(task, m_video, m_hrd), task.m_brcFrameCtrl);

    MFX_CHECK((mfxI32)res != UMC::BRC_ERROR, MFX_ERR_UNDEFINED_BEHAVIOR);
    if ((res != 0) && (!extOpt2.MaxSliceSize))
    {
        if (task.m_panicMode)
            return MFX_ERR_UNDEFINED_BEHAVIOR;

        task.m_brcFrameParams.NumRecode++;
        if ((task.m_cqpValue[0] == 51 || (res & UMC::BRC_NOT_ENOUGH_BUFFER)) && (res & UMC::BRC_ERR_BIG_FRAME))
        {
            task.m_panicMode = 1;
            task.m_repack = 100;
            task.m_isSkipped = true;
            bToRecode = true;
        }
        else if (((res & UMC::BRC_NOT_ENOUGH_BUFFER) || (task.m_repack > 2)) && (res & UMC::BRC_ERR_SMALL_FRAME))
        {
#if defined(MFX_ENABLE_ENCTOOLS)
            if (m_enabledEncTools)
            {
                task.m_minFrameSize = frame_sts.MinFrameSize;
                task.m_brcFrameParams.CodedFrameSize = task.m_minFrameSize;
                ests = m_encTools.SubmitEncodeResult(&task.m_brcFrameParams, task.m_brcFrameCtrl.QpY);
                MFX_CHECK_STS(ests);
                ests = m_encTools.GetEncodeStatus(&frame_sts, task.m_frameOrder);
                MFX_CHECK_STS(ests);
            }
            else
#endif
            {
                task.m_minFrameSize = m_brc.GetMinFrameSize() / 8;
                task.m_brcFrameParams.CodedFrameSize = task.m_minFrameSize;
                m_brc.Report(task.m_brcFrameParams, 0, m_hrd.GetMaxFrameSize((task.m_type[task.m_fid[0]] & MFX_FRAMETYPE_IDR)), task.m_brcFrameCtrl);
            }
            bToRecode = false; //Padding is in update bitstream
        }
        else
        {
#if defined(MFX_ENABLE_ENCTOOLS)
            if (m_enabledEncTools)
            {
                ests = m_encTools.SubmitFrameForEncoding(task);
                MFX_CHECK_STS(ests);
                ests = m_encTools.GetFrameCtrl(&task.m_brcFrameCtrl, task.m_frameOrder);
                MFX_CHECK_STS(ests);
            } else
#endif
            m_brc.GetQpForRecode(task.m_brcFrameParams, task.m_brcFrameCtrl);
            UpdateBRCParams(task);
            bToRecode = true;
        }
    }
    return MFX_ERR_NONE;
}



#if defined(MFX_ENABLE_ENCTOOLS)
mfxStatus ImplementationAvc::FillPreEncParams(DdiTask &task)
{
    mfxStatus sts = MFX_ERR_NONE;
    if (m_encTools.IsAdaptiveGOP() || m_encTools.IsAdaptiveQP() || m_encTools.IsAdaptiveLTR() || m_encTools.IsAdaptiveRef())
    {
        mfxEncToolsHintPreEncodeGOP st = {};
        sts = m_encTools.QueryPreEncRes(task.m_frameOrder, st);
        MFX_CHECK_STS(sts);

        if (task.m_type[0] == 0)
        {
            task.m_type = ExtendFrameType(st.FrameType);
            task.m_currGopRefDist = st.MiniGopSize;
            AssignFrameTypes(task);
        }
        else
            task.m_currGopRefDist = m_video.mfx.GopRefDist;

        if (m_encTools.IsAdaptiveGOP() && !m_encTools.IsAdaptiveLTR() && !m_encTools.IsAdaptiveRef() && !m_encTools.IsAdaptiveQP())
        {
            task.m_QPdelta = st.QPDelta;
            task.m_bQPDelta = true;
        }

        if (m_encTools.IsAdaptiveQP())
        {
            task.m_QPdelta = st.QPDelta;
            task.m_bQPDelta = true;
            task.m_QPmodulation = st.QPModulation;
#ifdef ENABLE_APQ_LQ
            if (task.m_currGopRefDist == 8) {
                if ((task.m_type[0] & MFX_FRAMETYPE_B)) {
                    task.m_ALQOffset = 2;
                }
                else if ((task.m_type[0] & MFX_FRAMETYPE_P)) {
                    task.m_ALQOffset = -1;
                }
            }
#endif
            //printf("%d type %d, m_QPdelta %d m_QPmodulation %d, currGopRefDist %d, ALQOffset %d\n", task.m_frameOrder, st.FrameType, st.QPDelta, st.QPModulation, task.m_currGopRefDist, task.m_ALQOffset);
        }

        if (m_encTools.IsAdaptiveLTR() || m_encTools.IsAdaptiveRef())
        {
            task.m_QPdelta = st.QPDelta;
            task.m_bQPDelta = true;

            mfxEncToolsHintPreEncodeARefFrames ref = {};
            sts = m_encTools.QueryPreEncARef(task.m_frameOrder, ref);
            MFX_CHECK_STS(sts);

            InitExtBufHeader(task.m_internalListCtrl);
            task.m_internalListCtrlPresent = true;

            if (ref.CurrFrameType == MFX_REF_FRAME_TYPE_LTR)
            {
                task.m_internalListCtrl.LongTermRefList[0].FrameOrder = task.m_frameOrder;
                task.m_internalListCtrl.LongTermRefList[0].PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
            }
            if (ref.CurrFrameType == MFX_REF_FRAME_TYPE_KEY)
                task.m_keyReference = 1;

            for (mfxU32 i = 0; i < std::min(ref.PreferredRefListSize, (mfxU16)32); i++)
            {
                task.m_internalListCtrl.PreferredRefList[i].FrameOrder =
                    ref.PreferredRefList[i];
                task.m_internalListCtrl.PreferredRefList[i].PicStruct =
                    MFX_PICSTRUCT_PROGRESSIVE;
            }
            for (mfxU32 i = 0; i < std::min(ref.RejectedRefListSize, (mfxU16)32); i++)
            {
                task.m_internalListCtrl.RejectedRefList[i].FrameOrder =
                    ref.RejectedRefList[i];
                task.m_internalListCtrl.RejectedRefList[i].PicStruct =
                    MFX_PICSTRUCT_PROGRESSIVE;
            }
        }

    }



    return MFX_ERR_NONE;
}
#endif

mfxStatus ImplementationAvc::AsyncRoutine(mfxBitstream * bs)
{
    mfxExtCodingOption     const & extOpt  = GetExtBufferRef(m_video);
    mfxExtCodingOptionDDI  const & extDdi  = GetExtBufferRef(m_video);
    mfxExtCodingOption2    const & extOpt2 = GetExtBufferRef(m_video);
    mfxExtCodingOption3    const & extOpt3 = GetExtBufferRef(m_video);

    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "ImplementationAvc::AsyncRoutine");


    if (m_stagesToGo == 0)
    {
        UMC::AutomaticUMCMutex guard(m_listMutex);
        m_stagesToGo = m_emulatorForAsyncPart.Go(!m_incoming.empty());
    }


    mfxExtFeiParam const * extFeiParams = GetExtBuffer(m_video, 0);
    mfxU32 stagesToGo = 0;

    if (IsOn(extFeiParams->SingleFieldProcessing) && (1 == m_fieldCounter))
    {
        stagesToGo = m_stagesToGo;
        /* Coding last field in FEI Field processing mode
        * task is ready, just need to call m_ddi->Execute() and
        * m_ddi->QueryStatus()... And free encoding task
        */
        m_stagesToGo = AsyncRoutineEmulator::STG_BIT_RESTART*2;
    }



    if (m_stagesToGo & AsyncRoutineEmulator::STG_BIT_ACCEPT_FRAME)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "Avc::STG_BIT_ACCEPT_FRAME");
        DdiTask & newTask = m_incoming.front();

       if (m_video.mfx.RateControlMethod == MFX_RATECONTROL_LA_EXT)
       {
            const mfxExtLAFrameStatistics *vmeData = GetExtBuffer(newTask.m_ctrl);
            MFX_CHECK_NULL_PTR1(vmeData);
            mfxLAFrameInfo *pInfo = &vmeData->FrameStat[0];


            newTask.m_ctrl.FrameType =  (pInfo->FrameDisplayOrder == 0) ? (mfxU8)( pInfo->FrameType |MFX_FRAMETYPE_IDR): (mfxU8) pInfo->FrameType;
            newTask.m_type = ExtendFrameType(newTask.m_ctrl.FrameType);
            newTask.m_frameOrder   = pInfo->FrameDisplayOrder;

            MFX_CHECK(newTask.m_ctrl.FrameType, MFX_ERR_UNDEFINED_BEHAVIOR);
       }

        if (!m_video.mfx.EncodedOrder)
        {
            if (IsExtBrcSceneChangeSupported(m_video)
#if defined(MFX_ENABLE_ENCTOOLS)
                || (m_enabledEncTools && m_encTools.IsAdaptiveGOP())
#endif
                )
            {
                newTask.m_frameOrder = m_frameOrder;
            }
            else
            {
                if (newTask.m_type[0] == 0)
                    newTask.m_type = GetFrameType(m_video, m_frameOrder - m_frameOrderIdrInDisplayOrder);
                else
                {
                    mfxStatus sts = FixForcedFrameType(newTask, m_frameOrder - m_frameOrderIdrInDisplayOrder);
                    if (sts != MFX_ERR_NONE)
                        return Error(sts);
                }

                newTask.m_frameOrder = m_frameOrder;
                AssignFrameTypes(newTask);

#if defined(MFX_ENABLE_ENCTOOLS_LPLA)
                if (!m_encTools.IsLookAhead())
#endif
                    BuildPPyr(newTask, GetPPyrSize(m_video, 0, false), true, false);
            }

            m_timeStamps.push_back(newTask.m_timeStamp);
            m_frameOrder++;
        }
        else
        {
            newTask.m_picStruct    = GetPicStruct(m_video, newTask);
            newTask.m_fieldPicFlag = newTask.m_picStruct[ENC] != MFX_PICSTRUCT_PROGRESSIVE;
            newTask.m_fid[0]       = newTask.m_picStruct[ENC] == MFX_PICSTRUCT_FIELD_BFF;
            newTask.m_fid[1]       = newTask.m_fieldPicFlag - newTask.m_fid[0];

            if (newTask.m_picStruct[ENC] == MFX_PICSTRUCT_FIELD_BFF)
                std::swap(newTask.m_type.top, newTask.m_type.bot);

            PreserveTimeStamp(newTask.m_timeStamp);
        }

        // move task to reordering queue
        //printf("\rACCEPTED      do=%4d eo=%4d type=%d\n", newTask.m_frameOrder, newTask.m_encOrder, newTask.m_type[0]); fflush(stdout);
            SubmitScd();
            // move task to reordering queue
            //OnNewFrame();
    }

    if (m_stagesToGo & AsyncRoutineEmulator::STG_BIT_START_SCD)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "Avc::STG_BIT_START_SCD");
        DdiTask & task = m_ScDetectionStarted.back();
#if defined(MFX_ENABLE_ENCTOOLS)
        if (m_enabledEncTools && (m_encTools.IsPreEncNeeded() || m_encTools.IsLookAhead()))
        {
            mfxFrameSurface1  tmpSurface = *task.m_yuv;
            mfxStatus sts = m_encTools.SubmitForPreEnc(task.m_frameOrder, &tmpSurface);
            if (sts == MFX_ERR_MORE_DATA)
                sts = MFX_ERR_NONE;
            MFX_CHECK_STS(sts);
        }
#endif
        if (IsExtBrcSceneChangeSupported(m_video))
        {
            MFX_CHECK_STS(SCD_Put_Frame(task));
        }

        OnScdQueried();
    }

    if (m_stagesToGo & AsyncRoutineEmulator::STG_BIT_WAIT_SCD)
    {
        DdiTask & task = m_ScDetectionFinished.front();

        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "Avc::STG_BIT_WAIT_SCD");
#if defined(MFX_ENABLE_ENCTOOLS)
        if (m_enabledEncTools)
        {
            mfxStatus sts = FillPreEncParams(task);
            MFX_CHECK_STS(sts);


        }
#endif
        if (IsExtBrcSceneChangeSupported(m_video))
        {

            if (task.m_type[0] == 0)
                task.m_type = GetFrameType(m_video, task.m_frameOrder - m_frameOrderIdrInDisplayOrder);

            MFX_CHECK_STS(Prd_LTR_Operation(task));
            MFX_CHECK_STS(SCD_Get_FrameType(task));
            AssignFrameTypes(task);
        }
        SubmitMCTF();
    }

    if (m_stagesToGo & AsyncRoutineEmulator::STG_BIT_START_MCTF)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "Avc::STG_BIT_START_MCTF");
        if (IsMctfSupported(m_video))
        {
#if defined(MFX_ENABLE_MCTF_IN_AVC)
            DdiTask & task = m_MctfStarted.back();
            MFX_CHECK_STS(SubmitToMctf(&task));
#endif
        }
        OnMctfQueried();
    }

    if (m_stagesToGo & AsyncRoutineEmulator::STG_BIT_WAIT_MCTF)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "Avc::STG_BIT_WAIT_MCTF");
        if (IsMctfSupported(m_video))
        {
#if defined(MFX_ENABLE_MCTF_IN_AVC)
            DdiTask & task = m_MctfFinished.front();
            MFX_CHECK_STS(QueryFromMctf(&task));
#endif
        }
        OnMctfFinished();
    }

    if (m_stagesToGo & AsyncRoutineEmulator::STG_BIT_START_LA)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "Avc::START_LA");
        bool gopStrict = !!(m_video.mfx.GopOptFlag & MFX_GOP_STRICT);
        bool closeGopForSceneChange = (extOpt2.BRefType != MFX_B_REF_PYRAMID) && IsOn(extOpt2.AdaptiveB);
        DdiTaskIter task = (m_video.mfx.EncodedOrder)
            ? m_reordering.begin()
            : ReorderFrame(m_lastTask.m_dpbPostEncoding,
                m_reordering.begin(), m_reordering.end(),
                gopStrict, m_reordering.size() < m_video.mfx.GopRefDist,
                closeGopForSceneChange);
        if (task == m_reordering.end())
            return Error(MFX_ERR_UNDEFINED_BEHAVIOR);
        task->m_idx    = FindFreeResourceIndex(m_raw);
        task->m_midRaw = AcquireResource(m_raw, task->m_idx);

        mfxStatus sts = GetNativeHandleToRawSurface(*m_core, m_video, *task, task->m_handleRaw);
        if (sts != MFX_ERR_NONE)
            return Error(sts);

        sts = CopyRawSurfaceToVideoMemory(*m_core, m_video, *task);
        if (sts != MFX_ERR_NONE)
            return Error(sts);
        if (bIntRateControlLA(m_video.mfx.RateControlMethod))
        {
            mfxHDLPair cmMb = AcquireResourceUp(m_mb);
            task->m_cmMb    = (CmBufferUP *)cmMb.first;
            task->m_cmMbSys = (void *)cmMb.second;
            task->m_cmRawLa = (CmSurface2D *)AcquireResource(m_rawLa);
            task->m_cmCurbe = (CmBuffer *)AcquireResource(m_curbe);
            task->m_vmeData = FindUnusedVmeData(m_vmeDataStorage);
            if ((!task->m_cmRawLa && extOpt2.LookAheadDS > MFX_LOOKAHEAD_DS_OFF) || !task->m_cmMb || !task->m_cmCurbe || !task->m_vmeData)
                return Error(MFX_ERR_UNDEFINED_BEHAVIOR);
            task->m_cmRaw = CreateSurface(m_cmDevice, task->m_handleRaw, m_currentVaType);
        }

        if (IsOn(extOpt3.FadeDetection) && m_cmCtx.get() && m_cmCtx->isHistogramSupported())
        {
            mfxHDLPair cmHist = AcquireResourceUp(m_histogram);
            task->m_cmHist = (CmBufferUP *)cmHist.first;
            task->m_cmHistSys = (mfxU32 *)cmHist.second;

            if (!task->m_cmHist)
                return Error(MFX_ERR_UNDEFINED_BEHAVIOR);

            memset(task->m_cmHistSys, 0, sizeof(uint)* 512);

            task->m_cmRawForHist = CreateSurface(m_cmDevice, task->m_handleRaw, m_currentVaType);
        }

        task->m_isENCPAK = m_isENCPAK;
        if (m_isENCPAK && (NULL != bs))
            task->m_bs = bs;

        // keep the hwtype info in case of no VideoCore interface
        task->m_hwType = m_currentPlatform;

        ConfigureTask(*task, m_lastTask, m_video, m_caps);
        AssignDecodeTimeStamp(*task);

        Zero(task->m_IRState);
        if (task->m_tidx == 0)
        {
            // current frame is in base temporal layer
            // insert intra refresh MBs and update base layer counter
            task->m_IRState = GetIntraRefreshState(
                m_video,
                mfxU32(task->m_baseLayerOrder - m_baseLayerOrderStartIntraRefresh),
                &(task->m_ctrl),
                m_intraStripeWidthInMBs,
                m_sliceDivider,
                m_caps);
            m_baseLayerOrder ++;
        }

        if (bIntRateControlLA(m_video.mfx.RateControlMethod))
        {
            int ffid = task->m_fid[0];
            ArrayDpbFrame const & dpb = task->m_dpb[ffid];
            ArrayU8x33 const &    l0  = task->m_list0[ffid];
            ArrayU8x33 const &    l1  = task->m_list1[ffid];

            DdiTask * fwd = 0;
            if (l0.Size() > 0)
                fwd = find_if_ptr4(m_lookaheadFinished, m_lookaheadStarted, m_histRun, m_histWait,
                    FindByFrameOrder(dpb[l0[0] & 127].m_frameOrder));

            DdiTask * bwd = 0;
            if (l1.Size() > 0)
                bwd = find_if_ptr4(m_lookaheadFinished, m_lookaheadStarted, m_histRun, m_histWait,
                    FindByFrameOrder(dpb[l1[0] & 127].m_frameOrder));

            if ((!fwd) && l0.Size() >0  && extOpt2.MaxSliceSize) //TO DO
            {
                fwd = &m_lastTask;
            }

            task->m_cmRefs = CreateVmeSurfaceG75(m_cmDevice, task->m_cmRaw,
                fwd ? &fwd->m_cmRaw : 0, bwd ? &bwd->m_cmRaw : 0, !!fwd, !!bwd);

            if (extOpt2.LookAheadDS > MFX_LOOKAHEAD_DS_OFF)
                task->m_cmRefsLa = CreateVmeSurfaceG75(m_cmDevice, task->m_cmRawLa,
                    fwd ? &fwd->m_cmRawLa : 0, bwd ? &bwd->m_cmRawLa : 0, !!fwd, !!bwd);

            task->m_cmRefMb = bwd ? bwd->m_cmMb : 0;
            task->m_fwdRef  = fwd;
            task->m_bwdRef  = bwd;

            SubmitLookahead(*task);
        }

        //printf("\rLA_SUBMITTED  do=%4d eo=%4d type=%d\n", task->m_frameOrder, task->m_encOrder, task->m_type[0]); fflush(stdout);
        if (extOpt2.MaxSliceSize && m_lastTask.m_yuv && !m_caps.ddi_caps.SliceLevelRateCtrl)
        {
            if (m_raw.Unlock(m_lastTask.m_idx) == (mfxU32)-1)
            {
                m_core->DecreaseReference(&m_lastTask.m_yuv->Data);
            }
            ReleaseResource(m_rawLa, m_lastTask.m_cmRawLa);
            ReleaseResource(m_mb,    m_lastTask.m_cmMb);
            if (m_cmDevice)
            {
                m_cmDevice->DestroySurface(m_lastTask.m_cmRaw);
                m_lastTask.m_cmRaw = NULL;
            }
        }
        m_lastTask = *task;
        if (extOpt2.MaxSliceSize && m_lastTask.m_yuv && !m_caps.ddi_caps.SliceLevelRateCtrl)
        {
            if (m_raw.Lock(m_lastTask.m_idx) == 0)
            {
                m_core->IncreaseReference(&m_lastTask.m_yuv->Data);
            }
        }
        OnLookaheadSubmitted(task);
    }


    if (m_stagesToGo & AsyncRoutineEmulator::STG_BIT_WAIT_LA)
    {
        mfxStatus sts = MFX_ERR_NONE;
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "Avc::WAIT_LA");
        if (bIntRateControlLA(m_video.mfx.RateControlMethod))
            sts = QueryLookahead(m_lookaheadStarted.front());

        if(sts != MFX_ERR_NONE)
            return sts;

        //printf("\rLA_SYNCED     do=%4d eo=%4d type=%d\n", m_lookaheadStarted.front().m_frameOrder, m_lookaheadStarted.front().m_encOrder, m_lookaheadStarted.front().m_type[0]); fflush(stdout);
        OnLookaheadQueried();
    }

    if (m_stagesToGo & AsyncRoutineEmulator::STG_BIT_START_HIST)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "Avc::STG_BIT_START_HIST");
        if (IsOn(extOpt3.FadeDetection) && m_cmCtx.get() && m_cmCtx->isHistogramSupported())
        {
            DdiTask & task = m_histRun.front();

            task.m_event = m_cmCtx->RunHistogram(task,
                m_video.mfx.FrameInfo.CropW, m_video.mfx.FrameInfo.CropH,
                m_video.mfx.FrameInfo.CropX, m_video.mfx.FrameInfo.CropY);
        }

        OnHistogramSubmitted();
    }

    if (m_stagesToGo & AsyncRoutineEmulator::STG_BIT_WAIT_HIST)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "Avc::STG_BIT_WAIT_HIST");
        DdiTask & task = m_histWait.front();
        mfxStatus sts = MFX_ERR_NONE;
        if (IsOn(extOpt3.FadeDetection) && m_cmCtx.get() && m_cmCtx->isHistogramSupported())
            sts = m_cmCtx->QueryHistogram(task.m_event);

        if(sts != MFX_ERR_NONE)
            return sts;
        CalcPredWeightTable(task, m_caps.ddi_caps.MaxNum_WeightedPredL0, m_caps.ddi_caps.MaxNum_WeightedPredL1);

        OnHistogramQueried();


        if (extDdi.LookAheadDependency > 0 && m_lookaheadFinished.size() >= extDdi.LookAheadDependency)
        {
            DdiTaskIter end = m_lookaheadFinished.end();
            DdiTaskIter beg = end;
            std::advance(beg, -extDdi.LookAheadDependency);

            AnalyzeVmeData(beg, end, m_video.calcParam.widthLa, m_video.calcParam.heightLa);
        }
    }

    if ((m_stagesToGo & AsyncRoutineEmulator::STG_BIT_START_ENCODE) || m_bDeferredFrame)
    {
        bool bParallelEncPak = (m_video.mfx.RateControlMethod == MFX_RATECONTROL_CQP && m_video.mfx.GopRefDist > 2 && m_video.AsyncDepth > 2);
        //char task_name [40];
        //sprintf(task_name,"Avc::START_ENCODE (%d) - %x", task->m_encOrder, task->m_yuv);
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "Avc::START_ENCODE");

        Hrd hrd = m_hrd; // tmp copy
        mfxU32 numEncCall = m_bDeferredFrame + 1;
        for (mfxU32 i = 0; i < numEncCall; i++)
        {
            DdiTaskIter task = FindFrameToStartEncode(m_video, m_lookaheadFinished.begin(), m_lookaheadFinished.end());
            if (task == m_lookaheadFinished.end())
                break;

            bool toSkip = IsFrameToSkip(*task, m_rec, m_recFrameOrder, m_enabledSwBrc);
            if ((task->isSEIHRDParam(extOpt, extOpt2) || toSkip) && (!m_encoding.empty()))
            {
                // wait until all previously submitted encoding tasks are finished
                m_bDeferredFrame ++;
                m_stagesToGo &= ~AsyncRoutineEmulator::STG_BIT_START_ENCODE;
                break;
            }

            task->m_initCpbRemoval = hrd.GetInitCpbRemovalDelay();
            task->m_initCpbRemovalOffset = hrd.GetInitCpbRemovalDelayOffset();

            if (bParallelEncPak)
            {
                if (task->m_type[0] & MFX_FRAMETYPE_REF)
                {
                    m_rec.Lock(m_recNonRef[0]);
                    m_rec.Lock(m_recNonRef[1]);

                    task->m_idxRecon = FindFreeResourceIndex(m_rec);

                    m_rec.Unlock(m_recNonRef[0]);
                    m_rec.Unlock(m_recNonRef[1]);
                    m_recNonRef[0] = m_recNonRef[1] = 0xffffffff;
                }
                else
                {
                    task->m_idxRecon = FindFreeResourceIndex(m_rec);

                    m_recNonRef[0] = m_recNonRef[1];
                    m_recNonRef[1] = task->m_idxRecon;
                }
            }
            else
            {
                task->m_idxRecon = FindFreeResourceIndex(m_rec);
            }

            task->m_idxBs[0] = FindFreeResourceIndex(m_bit);
            task->m_midRec = AcquireResource(m_rec, task->m_idxRecon);
            task->m_midBit[0] = AcquireResource(m_bit, task->m_idxBs[0]);
            if (!task->m_midRec || !task->m_midBit[0])
                return Error(MFX_ERR_UNDEFINED_BEHAVIOR);

            if (task->m_fieldPicFlag)
            {
                task->m_idxBs[1] = FindFreeResourceIndex(m_bit);
                task->m_midBit[1] = AcquireResource(m_bit, task->m_idxBs[1]);
                if (!task->m_midBit[1])
                    return Error(MFX_ERR_UNDEFINED_BEHAVIOR);
            }

            // Change DPB
            m_recFrameOrder[task->m_idxRecon] = task->m_frameOrder;
            Change_DPB(task->m_dpb[0], m_rec.mids, m_recFrameOrder);
            Change_DPB(task->m_dpb[1], m_rec.mids, m_recFrameOrder);
            Change_DPB(task->m_dpbPostEncoding, m_rec.mids, m_recFrameOrder);


            if (m_enabledSwBrc)
            {
                task->InitBRCParams();

                if (bIntRateControlLA(m_video.mfx.RateControlMethod))
                    BrcPreEnc(*task);
                else if (m_video.mfx.RateControlMethod == MFX_RATECONTROL_LA_EXT)
                {
                    const mfxExtLAFrameStatistics *vmeData = GetExtBuffer(task->m_ctrl);
                    MFX_CHECK_NULL_PTR1(vmeData);
                    mfxStatus sts = m_brc.SetFrameVMEData(vmeData, m_video.mfx.FrameInfo.Width, m_video.mfx.FrameInfo.Height);
                    if (sts != MFX_ERR_NONE)
                        return Error(sts);
                }

                if (IsExtBrcSceneChangeSupported(m_video)
                    && (task->GetFrameType() & MFX_FRAMETYPE_I) && (task->m_encOrder == 0 || m_video.mfx.GopPicSize != 1))
                {
                    mfxStatus sts = CalculateFrameCmplx(*task, task->m_brcFrameParams.FrameCmplx);
                    if (sts != MFX_ERR_NONE)
                        return Error(sts);
                }

#if defined(MFX_ENABLE_ENCTOOLS)
                if (m_enabledEncTools)
                {
                    mfxStatus etSts;
                    etSts = m_encTools.SubmitFrameForEncoding(*task);
                    MFX_CHECK_STS(etSts);
                    etSts = m_encTools.GetFrameCtrl(&task->m_brcFrameCtrl, task->m_frameOrder);
                    MFX_CHECK_STS(etSts);
                }
                else
#endif
                m_brc.GetQp(task->m_brcFrameParams, task->m_brcFrameCtrl);
                UpdateBRCParams(*task);

                if ((m_video.mfx.RateControlMethod == MFX_RATECONTROL_CBR || m_video.mfx.RateControlMethod == MFX_RATECONTROL_VBR))
                {
                    m_LtrOrder = task->m_LtrOrder;
                    m_LtrQp = (task->m_longTermFrameIdx != NO_INDEX_U8) ? task->m_cqpValue[0] : task->m_LtrQp;
                    if (task->m_type[0] & MFX_FRAMETYPE_REF)
                    {
                        m_RefQp = task->m_cqpValue[0];
                        m_RefOrder = task->m_frameOrder;
                    }
                }

                if (extOpt2.MaxSliceSize)
                {
                    mfxStatus sts = FillSliceInfo(*task, extOpt2.MaxSliceSize, extOpt2.MaxSliceSize * m_NumSlices, m_video.calcParam.widthLa, m_video.calcParam.heightLa);
                    if (sts != MFX_ERR_NONE)
                        return Error(sts);
                    //printf("EST frameSize %d\n", m_brc.GetDistFrameSize());
                }
            }

            if (IsOn(extOpt3.EnableMBQP))
            {
                const mfxExtMBQP *mbqp = GetExtBuffer(task->m_ctrl);
                mfxU32 wMB = (m_video.mfx.FrameInfo.CropW + 15) / 16;
                mfxU32 hMB = (m_video.mfx.FrameInfo.CropH + 15) / 16;
                task->m_isMBQP = mbqp && mbqp->QP && mbqp->NumQPAlloc >= wMB * hMB;
    #ifdef ENABLE_APQ_LQ
                    if (!task->m_isMBQP && task->m_ALQOffset != 0) {
                        task->m_isMBQP = true;
                        if (task->m_ALQOffset > 0) {
                            if (task->m_cqpValue[0] > task->m_ALQOffset && task->m_cqpValue[1] > task->m_ALQOffset) {
                                task->m_cqpValue[0] = (mfxU8)((mfxI32)task->m_cqpValue[0] - task->m_ALQOffset);
                                task->m_cqpValue[1] = (mfxU8)((mfxI32)task->m_cqpValue[1] - task->m_ALQOffset);
                            } else {
                                task->m_ALQOffset = 0;
                                task->m_isMBQP = false;
                            }
                        } else if (task->m_ALQOffset < 0) {
                            if (task->m_cqpValue[0] > 51 + task->m_ALQOffset || task->m_cqpValue[1] > 51 + task->m_ALQOffset) {
                                task->m_ALQOffset = 0;
                                task->m_isMBQP = false;
                            }
                        }
                    }
                    else {
                        task->m_ALQOffset = 0;
                    }
    #endif
                if (m_useMBQPSurf && task->m_isMBQP)
                {
                    task->m_idxMBQP = FindFreeResourceIndex(m_mbqp);
                    task->m_midMBQP = AcquireResource(m_mbqp, task->m_idxMBQP);
                }
            }

            // In case of progressive frames in PAFF mode need to switch the flag off to prevent m_fieldCounter changes
            task->m_singleFieldMode = (task->m_fieldPicFlag != 0) && IsOn(extFeiParams->SingleFieldProcessing);

#ifdef ENABLE_H264_MBFORCE_INTRA
            {
                if (IsOn(extOpt3.EnableMBForceIntra) && m_useMbControlSurfs)
                {
                    const mfxExtMBForceIntra *mbct = GetExtBuffer(task->m_ctrl);
                    mfxU32 wMB = (m_video.mfx.FrameInfo.CropW + 15) / 16;
                    mfxU32 hMB = (m_video.mfx.FrameInfo.CropH + 15) / 16;

                    task->m_isMBControl = mbct && mbct->Map && mbct->MapSize >= wMB * hMB;

                    if (task->m_isMBControl)
                    {
                        task->m_idxMBControl = FindFreeResourceIndex(m_mbControl);
                        task->m_midMBControl = AcquireResource(m_mbControl, task->m_idxMBControl);

                        mfxFrameData mbsurf = {};
                        FrameLocker lock(m_core, mbsurf, task->m_midMBControl);

                        MFX_CHECK_WITH_ASSERT(mbsurf.Y, MFX_ERR_LOCK_MEMORY);

#ifdef MFX_VA_WIN
                        for (mfxU32 y = 0; y < hMB; y++)
                        {
                            ENCODE_MBCONTROL* line = ((ENCODE_MBCONTROL*)mbsurf.Y) + y * wMB;
                            for (mfxU32 x = 0; x < wMB; x++)
                                line[x].MBParams.fields.bForceIntra = mbct->Map[y * wMB + x];
                        }
#else
#error "unimplemented code"
#endif
                    }
                }
            }
#endif
            if (toSkip)
            {
                mfxStatus sts = CodeAsSkipFrame(*m_core, m_video, *task, m_rawSkip, m_rec);
                if (sts != MFX_ERR_NONE)
                    return Error(sts);
            }

            for (mfxU32 f = 0; f <= task->m_fieldPicFlag; f++)
            {
                mfxU32 fieldId = task->m_fid[f];

                if (m_useWAForHighBitrates)
                    task->m_fillerSize[fieldId] = PaddingBytesToWorkAroundHrdIssue(
                        m_video, m_hrd, m_encoding, task->m_fieldPicFlag, f);

                mfxU16 recovery_frame_cnt = (mfxU16)std::count_if(m_reordering.begin(), m_reordering.end(), [](DdiTask task_item) {return task_item.m_type[0] & MFX_FRAMETYPE_REF; });
                if (!IsOn(extOpt.FramePicture))
                    recovery_frame_cnt *= 2; // assume that both paired fields are or aren't reference.
                PrepareSeiMessageBuffer(m_video, *task, fieldId, m_sei, recovery_frame_cnt);

#ifdef MFX_ENABLE_SVC_VIDEO_ENCODE_HW
                bool needSvcPrefix = IsSvcProfile(m_video.mfx.CodecProfile) || (m_video.calcParam.numTemporalLayer > 0);
#else
                bool needSvcPrefix = (m_video.calcParam.numTemporalLayer > 0);
#endif

                if (task->m_insertAud[f] == 0
                    && task->m_insertSps[f] == 0
                    && task->m_insertPps[f] == 0
                    && m_sei.Size() == 0
                    && needSvcPrefix == 0)
                    task->m_AUStartsFromSlice[f] = 1;
                else
                    task->m_AUStartsFromSlice[f] = 0;

                mfxStatus sts = MFX_ERR_NONE;

                //printf("Execute: %d, type %d, qp %d\n", task->m_frameOrder, task->m_type[0], task->m_cqpValue[0]);
#if defined(MFX_ENABLE_MCTF_IN_AVC)
                if(task->m_handleMCTF.first)//Intercept encoder so MCTF denoised frame can be fed at the right moment.
                    sts = m_ddi->Execute(task->m_handleMCTF, *task, fieldId, m_sei);
                else
#endif
                sts = m_ddi->Execute(task->m_handleRaw, *task, fieldId, m_sei);
                MFX_CHECK(sts == MFX_ERR_NONE, Error(sts));

#ifndef MFX_AVC_ENCODING_UNIT_DISABLE
                if (task->m_collectUnitsInfo && m_sei.Size() > 0)
                {
                    mfxU32 offset = task->m_headersCache[fieldId].size() > 0 ? task->m_headersCache[fieldId].back().Offset + task->m_headersCache[fieldId].back().Size : 0;

                    task->m_headersCache[fieldId].emplace_back();
                    task->m_headersCache[fieldId].back().Type = NALU_SEI;
                    task->m_headersCache[fieldId].back().Size = m_sei.Size();
                    task->m_headersCache[fieldId].back().Offset = offset;
                }
#endif

                /* FEI Field processing mode: store first field */
                if (task->m_singleFieldMode && (0 == m_fieldCounter))
                {
                    m_fieldCounter = 1;

                    task->m_bsDataLength[0] = task->m_bsDataLength[1] = 0;

                    sts = QueryStatus(*task, fieldId);
                    MFX_CHECK(sts == MFX_ERR_NONE, Error(sts));

                    if ((NULL == task->m_bs) && (bs != NULL))
                        task->m_bs = bs;

                    sts = UpdateBitstream(*task, fieldId);
                    MFX_CHECK(sts == MFX_ERR_NONE, Error(sts));

                    /*DO NOT submit second field for execution in this case
                        * (FEI Field processing mode)*/
                    break;
                }
            }

            //printf("\rENC_SUBMITTED do=%4d eo=%4d type=%d\n", task->m_frameOrder, task->m_encOrder, task->m_type[0]); fflush(stdout);
            OnEncodingSubmitted(task);
            if (m_bDeferredFrame)
                m_bDeferredFrame--;

        }
        m_stagesToGo &= ~AsyncRoutineEmulator::STG_BIT_START_ENCODE;

    }


    if (m_stagesToGo & AsyncRoutineEmulator::STG_BIT_WAIT_ENCODE)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "Avc::WAIT_ENCODE");
        DdiTaskIter task = FindFrameToWaitEncode(m_encoding.begin(), m_encoding.end());

        mfxStatus sts = MFX_ERR_NONE;
        if (m_enabledSwBrc)
        {
            for (;; ++task->m_repack)
            {
                mfxU32 bsDataLength = 0;
                for (mfxU32 f = 0; f <= task->m_fieldPicFlag; f++)
                {
                    if ((sts = QueryStatus(*task, task->m_fid[f])) != MFX_ERR_NONE)
                        return sts;
                    bsDataLength += task->m_bsDataLength[task->m_fid[f]];
                }
                //printf("Real frameSize %d, repack %d\n", bsDataLength, task->m_repack);
                bool bRecoding = task->m_toRecode;
                if (!bRecoding && extOpt2.MaxSliceSize)
                {
                    sts = CheckSliceSize(*task, bRecoding);
                    if (sts != MFX_ERR_NONE)
                        return Error(sts);
                }
                if (!bRecoding)
                {
                    sts = CheckBufferSize(*task, bRecoding, bsDataLength, bs);
                    if (sts != MFX_ERR_NONE)
                        return Error(sts);
                }
                if (!bRecoding)
                {
                     if (IsOn(extOpt2.MBBRC))
                        task->m_brcFrameCtrl.QpY = task->m_cqpValue[0];
                    sts = CheckBRCStatus(*task, bRecoding, bsDataLength);
                    if (sts != MFX_ERR_NONE)
                        return Error(sts);
                }
                if (bRecoding)
                {

                    DdiTaskIter curTask = task;
                    DdiTaskIter nextTask;

                    task->m_toRecode = false;

                    // wait for next tasks
                    while ((nextTask = FindFrameToWaitEncodeNext(m_encoding.begin(), m_encoding.end(), curTask)) != curTask)
                    {
                        for (mfxU32 f = 0; f <= nextTask->m_fieldPicFlag; f++)
                        {
                            while ((sts = QueryStatus(*nextTask, nextTask->m_fid[f])) == MFX_TASK_BUSY)
                            {
                                vm_time_sleep(0);
                            }
                            if (sts != MFX_ERR_NONE)
                                return sts;
                        }
                        if (!extOpt2.MaxSliceSize)
                        {
#if defined(MFX_ENABLE_ENCTOOLS)
                            if (m_enabledEncTools)
                            {
                                mfxStatus ests;
                                ests = m_encTools.GetFrameCtrl(&nextTask->m_brcFrameCtrl, nextTask->m_frameOrder);
                                MFX_CHECK_STS(ests);
                            } else
#endif
                            m_brc.GetQpForRecode(nextTask->m_brcFrameParams, nextTask->m_brcFrameCtrl);
                            UpdateBRCParams(*nextTask);
                            bRecoding = true;
                        }
                        curTask = nextTask;
                    }
                    // restart  encoded task
                    nextTask = curTask = task;
                    do
                    {
                        if (m_enabledSwBrc && (m_video.mfx.RateControlMethod == MFX_RATECONTROL_CBR || m_video.mfx.RateControlMethod == MFX_RATECONTROL_VBR))
                        {
                            if (nextTask->m_longTermFrameIdx != NO_INDEX_U8 && nextTask->m_LtrOrder == m_LtrOrder) {
                                m_LtrQp = nextTask->m_cqpValue[0];
                            }
                            if (nextTask->m_type[0] & MFX_FRAMETYPE_REF)
                            {
                                m_RefQp = nextTask->m_cqpValue[0];
                                m_RefOrder = nextTask->m_frameOrder;
                            }
                        }
                        curTask = nextTask;
                        curTask->m_bsDataLength[0] = curTask->m_bsDataLength[1] = 0;
                        bool toSkip = IsFrameToSkip(*curTask, m_rec, m_recFrameOrder, m_enabledSwBrc);
                        if (toSkip)
                        {
                            sts = CodeAsSkipFrame(*m_core, m_video, *curTask, m_rawSkip, m_rec);
                            if (sts == MFX_WRN_DEVICE_BUSY)
                            {
                                curTask->m_toRecode = true;
                            }
                            else if (sts != MFX_ERR_NONE)
                                return Error(sts);
                        }
                        for (mfxU32 f = 0; f <= curTask->m_fieldPicFlag; f++)
                        {
                            PrepareSeiMessageBuffer(m_video, *curTask, curTask->m_fid[f], m_sei);
                            while ((sts =  m_ddi->Execute(curTask->m_handleRaw, *curTask, curTask->m_fid[f], m_sei)) == MFX_TASK_BUSY)
                            {
                                vm_time_sleep(0);
                            }
                            if ( sts != MFX_ERR_NONE)
                                return Error(sts);
                        }
                    } while ((nextTask = FindFrameToWaitEncodeNext(m_encoding.begin(), m_encoding.end(), curTask)) != curTask);

                    continue;
                }
                break;
            }
            m_rec.SetFlag(task->m_idxRecon, H264_FRAME_FLAG_READY);
            task->m_bs = bs;
            for (mfxU32 f = 0; f <= task->m_fieldPicFlag; f++)
            {
                //printf("Update bitstream: %d, len %d\n",task->m_encOrder, task->m_bsDataLength[task->m_fid[f]]);

                if ((sts = UpdateBitstream(*task, task->m_fid[f])) != MFX_ERR_NONE)
                    return Error(sts);
            }
            m_NumSlices = (mfxU32)task->m_SliceInfo.size();
            if (extOpt2.MaxSliceSize && task->m_repack < 4)
            {
                mfxF32 w_avg = 0;
                for (size_t t = 0; t < task->m_SliceInfo.size(); t ++ )
                    w_avg = w_avg + task->m_SliceInfo[t].weight;
                w_avg = w_avg/m_NumSlices;
                if (w_avg < 70.0f)
                    m_NumSlices =  (mfxU32)w_avg* m_NumSlices / 70;
            }
            OnEncodingQueried(task);
        }
        else if (IsOff(extOpt.FieldOutput))
        {
            mfxU32 f = 0;
            mfxU32 f_start = 0;
            mfxU32 f_end = task->m_fieldPicFlag;

            /* Query results if NO FEI Field processing mode (this is legacy encoding) */
            if (!task->m_singleFieldMode)
            {
                for (f = f_start; f <= f_end; f++)
                {
                    if ((sts = QueryStatus(*task, task->m_fid[f])) != MFX_ERR_NONE)
                        return sts;
                }
                task->m_bs = bs;
                for (f = f_start; f <= f_end; f++)
                {

                    if ((sts = UpdateBitstream(*task, task->m_fid[f])) != MFX_ERR_NONE)
                        return Error(sts);
                }

                OnEncodingQueried(task);
            } // if (!task->m_singleFieldMode)
        }
        else
        {
            std::pair<mfxBitstream *, mfxU32> * pair = reinterpret_cast<std::pair<mfxBitstream *, mfxU32> *>(bs);
            assert(pair->second < 2);
            task->m_bs = pair->first;
            mfxU32 fid = task->m_fid[pair->second & 1];

            if ((sts = QueryStatus(*task, fid)) != MFX_ERR_NONE)
                return sts;
            if ((sts = UpdateBitstream(*task, fid)) != MFX_ERR_NONE)
                return Error(sts);

            if (task->m_fieldCounter == 2)
            {
                OnEncodingQueried(task);
                UMC::AutomaticUMCMutex guard(m_listMutex);
                m_listOfPairsForFieldOutputMode.pop_front();
                m_listOfPairsForFieldOutputMode.pop_front();
            }
        }

    }

    /* FEI Field processing mode: second (last) field processing */
    if ( ((AsyncRoutineEmulator::STG_BIT_RESTART*2) == m_stagesToGo ) &&
         IsOn(extFeiParams->SingleFieldProcessing) && (1 == m_fieldCounter) )
    {
        DdiTaskIter task = FindFrameToWaitEncode(m_encoding.begin(), m_encoding.end());
        mfxU32 f = 1; // coding second field
        PrepareSeiMessageBuffer(m_video, *task, task->m_fid[f], m_sei);

        mfxStatus sts = m_ddi->Execute(task->m_handleRaw, *task, task->m_fid[f], m_sei);
        MFX_CHECK(sts == MFX_ERR_NONE, Error(sts));

        task->m_bsDataLength[0] = task->m_bsDataLength[1] = 0;

        sts = QueryStatus(*task, task->m_fid[f]);
        MFX_CHECK(sts == MFX_ERR_NONE, Error(sts));

        if ((NULL == task->m_bs) && (bs != NULL))
            task->m_bs = bs;

        sts = UpdateBitstream(*task, task->m_fid[f]);
        MFX_CHECK(sts == MFX_ERR_NONE, Error(sts));

        m_fieldCounter = 0;
        m_stagesToGo = stagesToGo;
        OnEncodingQueried(task);
    } // if (( (AsyncRoutineEmulator::STG_BIT_RESTART*2) == m_stagesToGo ) &&


    if (m_stagesToGo & AsyncRoutineEmulator::STG_BIT_RESTART)
    {
        m_stagesToGo = AsyncRoutineEmulator::STG_BIT_CALL_EMULATOR;
        return MFX_TASK_BUSY;
    }

    return MFX_TASK_DONE;
}


mfxStatus ImplementationAvc::AsyncRoutineHelper(void * state, void * param, mfxU32, mfxU32)
{
    ImplementationAvc & impl = *(ImplementationAvc *)state;

    if (impl.m_failedStatus != MFX_ERR_NONE)
        return impl.m_failedStatus;

    mfxStatus sts = MFX_ERR_NONE;
    try
    {
        sts = impl.AsyncRoutine((mfxBitstream *)param);
        //printf("encoder sts = %d\n",sts);
        if (sts != MFX_TASK_BUSY && sts != MFX_TASK_DONE)
            impl.m_failedStatus = sts;
    }
    catch (...)
    {
        impl.m_failedStatus = MFX_ERR_DEVICE_FAILED;
        sts = MFX_ERR_DEVICE_FAILED;
    }

    return sts;
}


mfxStatus ImplementationAvc::EncodeFrameCheck(
    mfxEncodeCtrl *           ctrl,
    mfxFrameSurface1 *        surface,
    mfxBitstream *            bs,
    mfxFrameSurface1 **       reordered_surface,
    mfxEncodeInternalParams * internalParams,
    MFX_ENTRY_POINT           entryPoints[],
    mfxU32 &                  numEntryPoints)
{
    char task_name [240];
    sprintf(task_name,"Avc::EncodeFrameCheck - %p", surface);
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_INTERNAL, task_name);
    mfxExtCodingOption const & extOpt = GetExtBufferRef(m_video);
    if (IsOff(extOpt.FieldOutput))
    {
        return EncodeFrameCheckNormalWay(ctrl, surface, bs,
            reordered_surface, internalParams, entryPoints, numEntryPoints);
    }
    else
    {
        if (m_fieldCounter == 0)
        {
            mfxStatus sts = EncodeFrameCheckNormalWay(ctrl, surface, bs,
                reordered_surface, internalParams, entryPoints, numEntryPoints);
            if (sts == MFX_WRN_DEVICE_BUSY || sts < MFX_ERR_NONE)
                return sts;

            UMC::AutomaticUMCMutex guard(m_listMutex);
            m_listOfPairsForFieldOutputMode.push_back(std::make_pair(bs, 0));
            entryPoints->pParam = &m_listOfPairsForFieldOutputMode.back();

            m_fieldCounter = 1;
            m_1stFieldStatus = sts;
            return MFX_ERR_MORE_BITSTREAM;
        }
        else
        {
            m_fieldCounter = 0;

            *reordered_surface = surface;

            UMC::AutomaticUMCMutex guard(m_listMutex);
            m_listOfPairsForFieldOutputMode.push_back(std::make_pair(bs, 1));
            entryPoints[0].pState               = this;
            entryPoints[0].pParam               = &m_listOfPairsForFieldOutputMode.back();
            entryPoints[0].pCompleteProc        = 0;
            entryPoints[0].pGetSubTaskProc      = 0;
            entryPoints[0].pCompleteSubTaskProc = 0;
            entryPoints[0].requiredNumThreads   = 1;
            entryPoints[0].pRoutineName         = "AsyncRoutine";
            entryPoints[0].pRoutine             = AsyncRoutineHelper;
            numEntryPoints = 1;

            return m_1stFieldStatus;
        }
    }
}


mfxStatus ImplementationAvc::EncodeFrameCheckNormalWay(
    mfxEncodeCtrl *           ctrl,
    mfxFrameSurface1 *        surface,
    mfxBitstream *            bs,
    mfxFrameSurface1 **       reordered_surface,
    mfxEncodeInternalParams * ,//internalParams,
    MFX_ENTRY_POINT           entryPoints[],
    mfxU32 &                  numEntryPoints)
{
    MFX_CHECK_STS(m_failedStatus);

    mfxStatus checkSts = CheckEncodeFrameParam(
        m_video, ctrl, surface, bs,
        m_core->IsExternalFrameAllocator(), m_caps, m_currentPlatform);
    if (checkSts < MFX_ERR_NONE)
        return checkSts;

    mfxStatus status = checkSts;

    mfxExtFeiParam const * extFeiParams = GetExtBuffer(m_video, 0);
    /* If (1): encoder in state "FEI field processing mode"
     * and (2): first field is coded already
     * scheduler can start execution immediately as task stored and ready
     * */
    if (extFeiParams && IsOn(extFeiParams->SingleFieldProcessing) && (1 == m_fieldCounter))
    {
        entryPoints[0].pState               = this;
        entryPoints[0].pParam               = bs;
        entryPoints[0].pCompleteProc        = 0;
        entryPoints[0].pGetSubTaskProc      = 0;
        entryPoints[0].pCompleteSubTaskProc = 0;
        entryPoints[0].requiredNumThreads   = 1;
        entryPoints[0].pRoutineName         = "AsyncRoutine";
        entryPoints[0].pRoutine             = AsyncRoutineHelper;
        numEntryPoints = 1;

        // Copy ctrl with all settings and Extension buffers
        m_encoding.front().m_ctrl = *ctrl;
        return status;
    }

    {
        UMC::AutomaticUMCMutex guard(m_listMutex);
        if (m_free.empty())
            return MFX_WRN_DEVICE_BUSY;
    }

    mfxU32 stagesToGo = m_emulatorForSyncPart.Go(!!surface);

    while (stagesToGo & AsyncRoutineEmulator::STG_BIT_RESTART)
        stagesToGo = m_emulatorForSyncPart.Go(!!surface);

    if (stagesToGo == AsyncRoutineEmulator::STG_BIT_CALL_EMULATOR)
        return MFX_ERR_MORE_DATA; // end of encoding session

    if ((stagesToGo & AsyncRoutineEmulator::STG_BIT_WAIT_ENCODE) == 0)
    {
        status = mfxStatus(MFX_ERR_MORE_DATA_SUBMIT_TASK);
        bs = 0; // no output will be generated
    }

    if (surface)
    {
        mfxEncodeCtrl defaultCtrl = {};
        if (ctrl == 0)
            ctrl = &defaultCtrl;

        UMC::AutomaticUMCMutex guard(m_listMutex);

        m_free.front().m_yuv  = surface;
        m_free.front().m_ctrl = *ctrl;
        m_free.front().m_type = ExtendFrameType(ctrl->FrameType);

        m_free.front().m_extFrameTag = surface->Data.FrameOrder;
        m_free.front().m_frameOrder  = surface->Data.FrameOrder;
        m_free.front().m_timeStamp   = surface->Data.TimeStamp;
        m_core->IncreaseReference(&surface->Data);

        mfxU16 const MaxNumOfROI = 0;
        m_free.front().m_roi.Resize(MaxNumOfROI);

        m_stat.NumCachedFrame++;
#ifdef MFX_ENABLE_MFE
        mfxExtMultiFrameControl * mfeCtrl = GetExtBuffer(*ctrl);
        if (mfeCtrl && mfeCtrl->Timeout)// Use user timeout to wait for frames
        {
            m_free.front().m_userTimeout = true;
            m_free.front().m_mfeTimeToWait = mfeCtrl->Timeout;
        }
        else if (mfeCtrl && mfeCtrl->Flush)// Force submission with current frame
        {
            m_free.front().m_flushMfe = true;
        }
        else //otherwise use calculated timeout or passed by user at inialization.
        {
            mfxExtMultiFrameControl & mfeInitCtrl = GetExtBufferRef(m_video);
            m_free.front().m_userTimeout = false;
            m_free.front().m_mfeTimeToWait = mfeInitCtrl.Timeout;
        }

        m_free.front().m_beginTime = vm_time_get_tick();
#endif
        m_incoming.splice(m_incoming.end(), m_free, m_free.begin());
    }


    *reordered_surface = surface;

    entryPoints[0].pState               = this;
    entryPoints[0].pParam               = bs;
    entryPoints[0].pCompleteProc        = 0;
    entryPoints[0].pGetSubTaskProc      = 0;
    entryPoints[0].pCompleteSubTaskProc = 0;
    entryPoints[0].requiredNumThreads   = 1;
    entryPoints[0].pRoutineName         = "AsyncRoutine";
    entryPoints[0].pRoutine             = AsyncRoutineHelper;
    numEntryPoints = 1;

    return status;
}


void ImplementationAvc::SubmitLookahead(
    DdiTask & task)
{
    task.m_vmeData->poc      = task.GetPoc(0);
    task.m_vmeData->pocL0    = task.m_fwdRef ? task.m_fwdRef->GetPoc(0) : 0xffffffff;
    task.m_vmeData->pocL1    = task.m_bwdRef ? task.m_bwdRef->GetPoc(0) : 0xffffffff;
    task.m_vmeData->encOrder = task.m_encOrder;
    task.m_vmeData->used     = true;

    task.m_event = m_cmCtx->RunVme(task, 26);
}


mfxStatus ImplementationAvc::QueryLookahead(
    DdiTask & task)
{
    return m_cmCtx->QueryVme(task, task.m_event);
}


mfxStatus ImplementationAvc::QueryStatus(
    DdiTask & task,
    mfxU32    fid)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "ImplementationAvc::QueryStatus");
    if (task.m_bsDataLength[fid] == 0)
    {
        mfxStatus sts = MFX_TASK_BUSY;

        sts = m_ddi->QueryStatus(task, fid);
        MFX_TRACE_3("m_ddi->QueryStatus", "Task[field=%d feedback=%d] sts=%d \n", fid, task.m_statusReportNumber[fid], sts);

        if (sts == MFX_WRN_DEVICE_BUSY)
            return MFX_TASK_BUSY;

        if (sts != MFX_ERR_NONE)
            return Error(sts);

        if (m_video.Protected == 0)
            if ((sts = CountLeadingFF(*m_core, task, fid)) != MFX_ERR_NONE)
                return Error(sts);
    }

    return MFX_ERR_NONE;
}

mfxStatus ImplementationAvc::UpdateBitstream(
    DdiTask & task,
    mfxU32    fid)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "ImplementationAvc::UpdateBitstream");
    mfxFrameData bitstream = {};

    bool needIntermediateBitstreamBuffer =
        m_video.calcParam.numTemporalLayer > 0 ||
        (IsSlicePatchNeeded(task, fid) || (m_video.mfx.NumRefFrame & 1));

    bool doPatch = (IsOn(m_video.mfx.LowPower) && (m_video.calcParam.numTemporalLayer > 0)) ||
        needIntermediateBitstreamBuffer ||
        IsInplacePatchNeeded(m_video, task, fid);


    if ((!((IsOn(m_video.mfx.LowPower) && (m_video.calcParam.numTemporalLayer > 0))) &&
        m_caps.ddi_caps.HeaderInsertion == 0 &&
        (m_currentPlatform != MFX_HW_IVB || m_core->GetVAType() != MFX_HW_VAAPI))
        || m_video.Protected != 0)
        doPatch = needIntermediateBitstreamBuffer = false;

    // Lock d3d surface with compressed picture.
    MFX_LTRACE_S(MFX_TRACE_LEVEL_INTERNAL, task.m_FrameName);

    FrameLocker lock(m_core, bitstream, task.m_midBit[fid]);
    if (bitstream.Y == 0)
        return Error(MFX_ERR_LOCK_MEMORY);

    mfxU32 skippedff = task.m_numLeadingFF[fid];
    task.m_bsDataLength[fid] -= skippedff;
    bitstream.Y += skippedff;

    if ((!m_video.Protected || task.m_notProtected) && (*((mfxU32*)bitstream.Y) == 0x00)) {
        MFX_LTRACE_S(MFX_TRACE_LEVEL_EXTCALL, "First 4 bytes of output bitstream don't contain Annex B NAL unit startcode - start_code_prefix_one_3bytes");
    }

    mfxU32   bsSizeActual  = task.m_bsDataLength[fid];
    mfxU32   bsSizeToCopy  = task.m_bsDataLength[fid];
    mfxU32   bsSizeAvail   = task.m_bs->MaxLength - task.m_bs->DataOffset - task.m_bs->DataLength;
    mfxU8 *  bsData        = task.m_bs->Data + task.m_bs->DataOffset + task.m_bs->DataLength;
    mfxU32 * dataLength    = &task.m_bs->DataLength;

    mfxU32 seconfFieldOffset = 0;
    if (fid)
        seconfFieldOffset = task.m_bs->DataLength;

    if (m_video.Protected == 0 || task.m_notProtected)
    {
        if (needIntermediateBitstreamBuffer)
        {
            bsData      = &m_tmpBsBuf[0];
            bsSizeAvail = mfxU32(m_tmpBsBuf.size());
        }
    }

    mfxU32 initialDataLength = *dataLength;

    assert(bsSizeToCopy <= bsSizeAvail);

    if (bsSizeToCopy > bsSizeAvail)
    {
        bsSizeToCopy = bsSizeAvail;
        bsSizeActual = bsSizeAvail;
        if (m_video.Protected)
        {
            bsSizeToCopy = mfx::align2_value(bsSizeToCopy - 15, 16);
            bsSizeActual = std::min(bsSizeActual, bsSizeToCopy);
        }
    }

    // Avoid segfaults on very high bitrates
    if (bsSizeToCopy > m_maxBsSize)
    {
        // bsSizeToCopy = m_maxBsSize;
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_INTERNAL, "Too big bitstream surface unlock (bitstream)");
        MFX_LTRACE_S(MFX_TRACE_LEVEL_INTERNAL, task.m_FrameName);
        lock.Unlock();
        return Error(MFX_ERR_DEVICE_FAILED);
    }

    // Copy compressed picture from d3d surface to buffer in system memory
    if (bsSizeToCopy)
    {
        FastCopyBufferVid2Sys(bsData, bitstream.Y, bsSizeToCopy);
    }

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_INTERNAL, "Surface unlock (bitstream)");
        MFX_LTRACE_S(MFX_TRACE_LEVEL_INTERNAL, task.m_FrameName);
        mfxStatus sts = lock.Unlock();
        MFX_CHECK_STS(sts);
    }

#ifndef MFX_AVC_ENCODING_UNIT_DISABLE
    mfxExtEncodedUnitsInfo * encUnitsInfo = NULL;
    if (task.m_collectUnitsInfo
        )
    {
        encUnitsInfo = (mfxExtEncodedUnitsInfo*)GetExtBuffer(task.m_bs->ExtParam, task.m_bs->NumExtParam, MFX_EXTBUFF_ENCODED_UNITS_INFO);
    }
#endif

    if (doPatch)
    {
        mfxU8 * dbegin = bsData;
        mfxU8 * dend   = bsData + bsSizeActual;

        if (needIntermediateBitstreamBuffer)
        {
            dbegin = task.m_bs->Data + task.m_bs->DataOffset + task.m_bs->DataLength;
            dend   = task.m_bs->Data + task.m_bs->MaxLength;
        }

        mfxU8 * endOfPatchedBitstream =
            IsOn(m_video.mfx.LowPower)?
            InsertSVCNAL(task, fid, bsData, bsData + bsSizeActual, dbegin, dend)://insert SVC NAL for temporal scalability
            PatchBitstream(m_video, task, fid, bsData, bsData + bsSizeActual, dbegin, dend);

        *dataLength += (mfxU32)(endOfPatchedBitstream - dbegin);
    }
    else
    {
        *dataLength += bsSizeActual;
    }

    if (m_enabledSwBrc)
    {
        mfxU32 minFrameSize = task.m_minFrameSize;
        mfxU32 frameSize = *dataLength - initialDataLength;
        bsData      += frameSize;
        bsSizeAvail -= frameSize;
        if (frameSize < minFrameSize)
        {
            CheckedMemset(bsData, bsData + bsSizeAvail, 0, minFrameSize - frameSize);
            *dataLength += minFrameSize - frameSize;
        }
        else
        {
            CheckedMemset(bsData, bsData + bsSizeAvail, 0, skippedff);
            *dataLength += skippedff;
        }
    }

    setFrameInfo(task, fid);

    // Update bitstream fields
    task.m_bs->TimeStamp = task.m_timeStamp;
    task.m_bs->DecodeTimeStamp = task.m_decodeTimeStamp;

    task.m_bs->PicStruct = task.GetPicStructForDisplay();
    task.m_bs->FrameType = task.m_type[task.GetFirstField()] & ~MFX_FRAMETYPE_KEYPIC;
    if (task.m_fieldPicFlag)
        task.m_bs->FrameType = mfxU16(task.m_bs->FrameType | ((task.m_type[!task.GetFirstField()]& ~MFX_FRAMETYPE_KEYPIC) << 8));

    mfxExtCodingOption const &extOpt = GetExtBufferRef(m_video);

    if (task.m_bs->NumExtParam > 0)
    {
        // setting of mfxExtAVCEncodedFrameInfo isn't supported for FieldOutput mode at the moment
        if (IsOff(extOpt.FieldOutput))
        {
            mfxExtAVCEncodedFrameInfo * encFrameInfo = (mfxExtAVCEncodedFrameInfo*)GetExtBuffer(task.m_bs->ExtParam, task.m_bs->NumExtParam, MFX_EXTBUFF_ENCODED_FRAME_INFO);
            if (encFrameInfo)
            {
                if (task.m_fieldPicFlag == 0)
                {
                    // should return actual reference lists to application
                    // at the moment it's supported for progressive encoding only
                    encFrameInfo->FrameOrder = task.m_extFrameTag;
                    encFrameInfo->LongTermIdx = task.m_longTermFrameIdx == NO_INDEX_U8 ? NO_INDEX_U16 : task.m_longTermFrameIdx;
                    encFrameInfo->MAD = task.m_mad[fid];

                    if ( bRateControlLA(m_video.mfx.RateControlMethod))
                        encFrameInfo->QP = task.m_cqpValue[fid];
                    else
                        encFrameInfo->QP = task.m_qpY[fid];

                    // only return of ref list L0 is supported at the moment
                    mfxU8 i = 0;
                    for (i = 0; i < task.m_list0[0].Size(); i ++)
                    {
                        DpbFrame& refFrame = task.m_dpb[0][task.m_list0[0][i] & 127]; // retrieve corresponding ref frame from DPB
                        encFrameInfo->UsedRefListL0[i].FrameOrder = refFrame.m_extFrameTag;
                        if (refFrame.m_longterm && refFrame.m_longTermIdxPlus1) // reference frame is LTR with valid LTR idx
                            encFrameInfo->UsedRefListL0[i].LongTermIdx = refFrame.m_longTermIdxPlus1 - 1;
                        else
                            encFrameInfo->UsedRefListL0[i].LongTermIdx = NO_INDEX_U16;
                        encFrameInfo->UsedRefListL0[i].PicStruct = (mfxU16)MFX_PICSTRUCT_PROGRESSIVE;
                    }

                    for (; i < 32; i ++)
                    {
                        encFrameInfo->UsedRefListL0[i].FrameOrder  = (mfxU32)MFX_FRAMEORDER_UNKNOWN;
                        encFrameInfo->UsedRefListL0[i].LongTermIdx = NO_INDEX_U16;
                        encFrameInfo->UsedRefListL0[i].PicStruct   = (mfxU16)MFX_PICSTRUCT_UNKNOWN;
                    }
                }
                else if (fid)
                {
                    encFrameInfo->SecondFieldOffset = seconfFieldOffset;
                }
            }
        }

#ifndef MFX_AVC_ENCODING_UNIT_DISABLE
        if (task.m_collectUnitsInfo)
        {
            FillEncodingUnitsInfo(
                task,
                task.m_bs->Data + task.m_bs->DataOffset,
                task.m_bs->Data + task.m_bs->DataOffset + task.m_bs->DataLength,
                encUnitsInfo,
                fid
            );

        }
        if (task.m_headersCache[fid].size() > 0) //if we have previously collected data about headers/slices
            task.m_headersCache[fid].clear();
#endif
    } //if (task.m_bs->NumExtParam > 0)


    task.m_fieldCounter++;

    // Update hrd buffer
    m_hrd.RemoveAccessUnit(
        task.m_bs->DataLength - initialDataLength,
        task.m_fieldPicFlag,
        (task.m_type[fid] & MFX_FRAMETYPE_IDR) != 0);

    if (initialDataLength == *dataLength)
    {
        MFX_LTRACE_S(MFX_TRACE_LEVEL_PARAMS, "Unexpected behavior : length of bitstream stayed unchanged");
    }

    return MFX_ERR_NONE;
}

#ifndef MFX_AVC_ENCODING_UNIT_DISABLE
/*
Method for filling mfxExtEncodedUnitsInfo ext-buffer

task         - in  - DDITask
sbegin       - in  - pointer to the start of bitstream, might be NULL if encUnitsList->size()>0
send         - in  - pointer to the end of bitstream, might be NULL if encUnitsList->size()>0
encUnitsInfo - out - destination ext-buffer
fid          - in  - field id
*/
void ImplementationAvc::FillEncodingUnitsInfo(
    DdiTask &task,
    mfxU8 *sbegin,
    mfxU8 *send,
    mfxExtEncodedUnitsInfo *encUnitsInfo,
    mfxU32 fid
    )
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "NALU Reporting");
    if (!encUnitsInfo)
        return;

    if (sbegin != NULL && send != NULL)
    {
        mfxU32 offset = 0;

        if (fid == 0)
        {
            encUnitsInfo->NumUnitsEncoded = 0;
        }
        else //calculate starting offset in bitstream for second field
        {
            offset = task.m_bsDataLength[0];
        }

        if (offset)
        {
            for (size_t i = 0; i < task.m_headersCache[fid].size(); ++i)
            {
                task.m_headersCache[fid][i].Offset += offset; //actualize offsets
            }
        }

        if (encUnitsInfo->NumUnitsAlloc > encUnitsInfo->NumUnitsEncoded)
        {
            size_t count = std::min<size_t>(encUnitsInfo->NumUnitsAlloc - encUnitsInfo->NumUnitsEncoded, task.m_headersCache[fid].size());

            std::copy(std::begin(task.m_headersCache[fid]), std::begin(task.m_headersCache[fid]) + count,
                        encUnitsInfo->UnitInfo + encUnitsInfo->NumUnitsEncoded);
        }

        if (task.m_headersCache[fid].size() > 0)
        {
            offset = task.m_headersCache[fid].back().Offset + task.m_headersCache[fid].back().Size; //in case we have hidden units
        }

        encUnitsInfo->NumUnitsEncoded += mfxU16(std::min(size_t(encUnitsInfo->NumUnitsAlloc) - encUnitsInfo->NumUnitsEncoded, task.m_headersCache[fid].size()));

        if (task.m_SliceInfo.size() <= 1 &&
            task.m_numSlice.top <= 1 && task.m_numSlice.bot <= 1
            && task.m_fieldPicFlag == false) //if we have only one slice in bitstream
        {
            if (encUnitsInfo->NumUnitsEncoded < encUnitsInfo->NumUnitsAlloc) {
                encUnitsInfo->UnitInfo[encUnitsInfo->NumUnitsEncoded].Type = sbegin[offset+3] & 0x1F;
                encUnitsInfo->UnitInfo[encUnitsInfo->NumUnitsEncoded].Size = (mfxU32)((ptrdiff_t)send - (ptrdiff_t)sbegin - offset);
                encUnitsInfo->UnitInfo[encUnitsInfo->NumUnitsEncoded].Offset = offset;
            }
            ++encUnitsInfo->NumUnitsEncoded;
        }
        else
        {
            for (NaluIterator nalu(sbegin + offset, send); nalu != NaluIterator(); ++nalu)
            {
                if (nalu->type != NALU_IDR && nalu->type != NALU_NON_IDR) break; //End of field
                if (encUnitsInfo->NumUnitsEncoded < encUnitsInfo->NumUnitsAlloc)
                {
                    encUnitsInfo->UnitInfo[encUnitsInfo->NumUnitsEncoded].Type = nalu->type;
                    encUnitsInfo->UnitInfo[encUnitsInfo->NumUnitsEncoded].Size = mfxU32(nalu->end - nalu->begin);
                    encUnitsInfo->UnitInfo[encUnitsInfo->NumUnitsEncoded].Offset = offset;
                    offset += encUnitsInfo->UnitInfo[encUnitsInfo->NumUnitsEncoded].Size;
                }
                ++encUnitsInfo->NumUnitsEncoded;
            }
        }
    }
} //ImplementationAvc::FillEncodingUnitsInfo
#endif

void ImplementationAvc::setFrameInfo( DdiTask & task, mfxU32    fid )
{



    mfxExtCodingOption const &extOpt = GetExtBufferRef(m_video);
    mfxU32 seconfFieldOffset = 0;
    if (fid)
        seconfFieldOffset = task.m_bs->DataLength;

    if (task.m_bs->NumExtParam > 0)
    {
        // setting of mfxExtAVCEncodedFrameInfo isn't supported for FieldOutput mode at the moment
        if (IsOff(extOpt.FieldOutput))
        {
            mfxExtAVCEncodedFrameInfo * encFrameInfo = (mfxExtAVCEncodedFrameInfo*)GetExtBuffer(task.m_bs->ExtParam, task.m_bs->NumExtParam, MFX_EXTBUFF_ENCODED_FRAME_INFO);
            if (encFrameInfo)
            {
                if (task.m_fieldPicFlag == 0)
                {
                    // should return actual reference lists to application
                    // at the moment it's supported for progressive encoding only
                    encFrameInfo->FrameOrder = task.m_extFrameTag;
                    encFrameInfo->LongTermIdx = task.m_longTermFrameIdx == NO_INDEX_U8 ? NO_INDEX_U16 : task.m_longTermFrameIdx;
                    encFrameInfo->MAD = task.m_mad[fid];

                    if ( bRateControlLA(m_video.mfx.RateControlMethod))
                        encFrameInfo->QP = task.m_cqpValue[fid];
                    else
                        encFrameInfo->QP = task.m_qpY[fid];

                    // only return of ref list L0 is supported at the moment
                    mfxU8 i = 0;
                    for (i = 0; i < task.m_list0[0].Size(); i ++)
                    {
                        DpbFrame& refFrame = task.m_dpb[0][task.m_list0[0][i] & 127]; // retrieve corresponding ref frame from DPB
                        encFrameInfo->UsedRefListL0[i].FrameOrder = refFrame.m_extFrameTag;
                        if (refFrame.m_longterm && refFrame.m_longTermIdxPlus1) // reference frame is LTR with valid LTR idx
                            encFrameInfo->UsedRefListL0[i].LongTermIdx = refFrame.m_longTermIdxPlus1 - 1;
                        else
                            encFrameInfo->UsedRefListL0[i].LongTermIdx = NO_INDEX_U16;
                        encFrameInfo->UsedRefListL0[i].PicStruct = (mfxU16)MFX_PICSTRUCT_PROGRESSIVE;
                    }

                    for (; i < 32; i ++)
                    {
                        encFrameInfo->UsedRefListL0[i].FrameOrder  = (mfxU32)MFX_FRAMEORDER_UNKNOWN;
                        encFrameInfo->UsedRefListL0[i].LongTermIdx = NO_INDEX_U16;
                        encFrameInfo->UsedRefListL0[i].PicStruct   = (mfxU16)MFX_PICSTRUCT_UNKNOWN;
                    }
                }
                else if (fid)
                {
                    encFrameInfo->SecondFieldOffset = seconfFieldOffset;
                }
            }
        }


    } //if (task.m_bs->NumExtParam > 0)

}





#endif // MFX_ENABLE_H264_VIDEO_ENCODE_HW
