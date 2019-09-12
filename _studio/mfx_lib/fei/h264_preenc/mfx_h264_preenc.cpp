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

#include "mfx_common.h"
#include "mfx_h264_preenc.h"
#include "fei_common.h"
#if defined(MFX_ENABLE_H264_VIDEO_ENCODE_HW) && defined(MFX_ENABLE_H264_VIDEO_FEI_PREENC)

#include <algorithm>
#include <numeric>

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

#include "mfx_h264_preenc.h"
#include "mfx_ext_buffers.h"

#include "mfx_h264_encode_vaapi.h"

using namespace MfxHwH264Encode;

namespace MfxEncPREENC
{

static bool IsVideoParamExtBufferIdSupported(mfxU32 id)
{
    return
        id == MFX_EXTBUFF_CODING_OPTION  ||
        id == MFX_EXTBUFF_CODING_OPTION2 ||
        id == MFX_EXTBUFF_CODING_OPTION3 ||
        id == MFX_EXTBUFF_FEI_PARAM;
}

mfxExtBuffer* GetExtBuffer(mfxExtBuffer** ebuffers, mfxU32 nbuffers, mfxU32 BufferId)
{
    if (ebuffers)
    {
        for (mfxU32 i = 0; i < nbuffers; ++i)
        {
            if (ebuffers[i] && ebuffers[i]->BufferId == BufferId)
            {
                return ebuffers[i];
            }
        }
    }
    return NULL;
}

static mfxStatus CheckExtBufferId(mfxVideoParam const & par)
{
    for (mfxU32 i = 0; i < par.NumExtParam; ++i)
    {
        MFX_CHECK(par.ExtParam[i], MFX_ERR_INVALID_VIDEO_PARAM);
        MFX_CHECK(MfxEncPREENC::IsVideoParamExtBufferIdSupported(par.ExtParam[i]->BufferId), MFX_ERR_INVALID_VIDEO_PARAM);
        MFX_CHECK(!MfxEncPREENC::GetExtBuffer(
            par.ExtParam + i + 1,
            par.NumExtParam - i - 1,
            par.ExtParam[i]->BufferId), MFX_ERR_INVALID_VIDEO_PARAM);
    }

    return MFX_ERR_NONE;
}

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

mfxStatus CheckVideoParamQueryLikePreEnc(
    MfxVideoParam &         par,
    MFX_ENCODE_CAPS const & hwCaps,
    eMFXHWType              platform,
    eMFXVAType              vaType)
{
    bool unsupported(false);
    bool changed(false);
    bool warning(false);

    mfxExtFeiParam * feiParam = GetExtBuffer(par);
    bool isPREENC = MFX_FEI_FUNCTION_PREENC == feiParam->Func;

    // check hw capabilities
    if (par.mfx.FrameInfo.Width  > hwCaps.ddi_caps.MaxPicWidth ||
        par.mfx.FrameInfo.Height > hwCaps.ddi_caps.MaxPicHeight)
        return Error(MFX_ERR_UNSUPPORTED);

    if ((par.mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_PART1 )!= MFX_PICSTRUCT_PROGRESSIVE && hwCaps.ddi_caps.NoInterlacedField){
        if(IsOn(par.mfx.LowPower))
        {
            par.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
            changed = true;
        }
        else
            return Error(MFX_WRN_PARTIAL_ACCELERATION);
    }

    if (hwCaps.ddi_caps.MaxNum_TemporalLayer != 0 &&
        hwCaps.ddi_caps.MaxNum_TemporalLayer < par.calcParam.numTemporalLayer)
        return Error(MFX_WRN_PARTIAL_ACCELERATION);

    if (!CheckTriStateOption(par.mfx.LowPower))
        changed = true;

    if (IsOn(par.mfx.LowPower))
    {
        unsupported = true;
        par.mfx.LowPower = 0;
    }

    if (par.IOPattern != 0)
    {
        if ((par.IOPattern & MFX_IOPATTERN_IN_MASK) != par.IOPattern)
        {
            changed = true;
            par.IOPattern &= MFX_IOPATTERN_IN_MASK;
        }

        if (par.IOPattern != MFX_IOPATTERN_IN_VIDEO_MEMORY)
        {
            changed = true;
            par.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY;
        }
    }

    if (par.mfx.GopPicSize > 0 &&
        par.mfx.GopRefDist > 0 &&
        par.mfx.GopRefDist > par.mfx.GopPicSize)
    {
        changed = true;
        par.mfx.GopRefDist = par.mfx.GopPicSize - 1;
    }

    if (par.mfx.GopRefDist > 1)
    {
        if (IsAvcBaseProfile(par.mfx.CodecProfile) ||
            par.mfx.CodecProfile == MFX_PROFILE_AVC_CONSTRAINED_HIGH)
        {
            changed = true;
            par.mfx.GopRefDist = 1;
        }
    }

    /* max allowed combination */
    if (par.mfx.FrameInfo.PicStruct > (MFX_PICSTRUCT_PART1|MFX_PICSTRUCT_PART2))
    { /* */
        unsupported = true;
        par.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_UNKNOWN;
    }

    if (par.mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_PART2)
    { // repeat/double/triple flags are for EncodeFrameAsync
        changed = true;
        par.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_UNKNOWN;
    }

    mfxU16 picStructPart1 = par.mfx.FrameInfo.PicStruct;
    if (par.mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_PART1)
    {
        if (picStructPart1 & (picStructPart1 - 1))
        { // more then one flag set
            changed = true;
            par.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_UNKNOWN;
        }
    }

    if (par.mfx.FrameInfo.Width & 15)
    {
        unsupported = true;
        par.mfx.FrameInfo.Width = 0;
    }

    if (par.mfx.FrameInfo.Height & 15)
    {
        unsupported = true;
        par.mfx.FrameInfo.Height = 0;
    }

    if ((par.mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_PROGRESSIVE) == 0 &&
        (par.mfx.FrameInfo.Height & 31) != 0)
    {
        unsupported = true;
        par.mfx.FrameInfo.PicStruct = 0;
        par.mfx.FrameInfo.Height = 0;
    }

    // driver doesn't support resolution 16xH
    if (par.mfx.FrameInfo.Width == 16)
    {
        unsupported = true;
        par.mfx.FrameInfo.Width = 0;
    }

    // driver doesn't support resolution Wx16
    if (par.mfx.FrameInfo.Height == 16)
    {
        unsupported = true;
        par.mfx.FrameInfo.Height = 0;
    }

    if (par.mfx.FrameInfo.Width > 0)
    {
        if (par.mfx.FrameInfo.CropX > par.mfx.FrameInfo.Width)
        {
            unsupported = true;
            par.mfx.FrameInfo.CropX = 0;
        }

        if (par.mfx.FrameInfo.CropX + par.mfx.FrameInfo.CropW > par.mfx.FrameInfo.Width)
        {
            unsupported = true;
            par.mfx.FrameInfo.CropW = par.mfx.FrameInfo.Width - par.mfx.FrameInfo.CropX;
        }
    }

    if (par.mfx.FrameInfo.Height > 0)
    {
        if (par.mfx.FrameInfo.CropY > par.mfx.FrameInfo.Height)
        {
            unsupported = true;
            par.mfx.FrameInfo.CropY = 0;
        }

        if (par.mfx.FrameInfo.CropY + par.mfx.FrameInfo.CropH > par.mfx.FrameInfo.Height)
        {
            unsupported = true;
            par.mfx.FrameInfo.CropH = par.mfx.FrameInfo.Height - par.mfx.FrameInfo.CropY;
        }
    }

    if (CorrectCropping(par) != MFX_ERR_NONE)
    { // cropping read from sps header always has correct alignment
        changed = true;
    }

    if (par.mfx.FrameInfo.ChromaFormat != 0 &&
        par.mfx.FrameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV420 &&
        par.mfx.FrameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV422 &&
        par.mfx.FrameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV444)
    {
        changed = true;
        par.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
    }

    if (par.mfx.FrameInfo.FourCC != 0 &&
        par.mfx.FrameInfo.FourCC != MFX_FOURCC_NV12)
    {
        unsupported = true;
        par.mfx.FrameInfo.FourCC = 0;
    }

    if (par.mfx.FrameInfo.FourCC == MFX_FOURCC_NV12 &&
        par.mfx.FrameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV420 &&
        par.mfx.FrameInfo.ChromaFormat != MFX_CHROMAFORMAT_MONOCHROME)
    {
        changed = true;
        par.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
    }

    if (hwCaps.ddi_caps.Color420Only &&
        (par.mfx.FrameInfo.ChromaFormat == MFX_CHROMAFORMAT_YUV422 ||
         par.mfx.FrameInfo.ChromaFormat == MFX_CHROMAFORMAT_YUV444))
    {
        unsupported = true;
        par.mfx.FrameInfo.ChromaFormat = 0;
    }

    if (isPREENC && !CheckTriStateOption(feiParam->SingleFieldProcessing))
        unsupported = true;

    return unsupported
        ? MFX_ERR_UNSUPPORTED
        : (changed || warning)
            ? MFX_WRN_INCOMPATIBLE_VIDEO_PARAM
            : MFX_ERR_NONE;
}

mfxStatus CheckVideoParamPreEncInit(
    MfxVideoParam &         par,
    MFX_ENCODE_CAPS const & hwCaps,
    eMFXHWType              platform,
    eMFXVAType              vaType)
{
    bool unsupported(false);
    bool changed(false);
    bool warning(false);

    mfxExtFeiParam * feiParam = GetExtBuffer(par);
    MFX_CHECK(feiParam, Error(MFX_ERR_UNSUPPORTED));

    if (MFX_FEI_FUNCTION_PREENC != feiParam->Func)
        unsupported = true;

    // check hw capabilities
    if (par.mfx.FrameInfo.Width  > hwCaps.ddi_caps.MaxPicWidth ||
        par.mfx.FrameInfo.Height > hwCaps.ddi_caps.MaxPicHeight)
        return Error(MFX_ERR_UNSUPPORTED);

    if ((par.mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_PART1 )!= MFX_PICSTRUCT_PROGRESSIVE && hwCaps.ddi_caps.NoInterlacedField){
        if(par.mfx.LowPower == MFX_CODINGOPTION_ON)
        {
            par.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
            changed = true;
        }
        else
            return Error(MFX_WRN_PARTIAL_ACCELERATION);
    }

    if (hwCaps.ddi_caps.MaxNum_TemporalLayer != 0 &&
        hwCaps.ddi_caps.MaxNum_TemporalLayer < par.calcParam.numTemporalLayer)
        return Error(MFX_WRN_PARTIAL_ACCELERATION);

    if (!CheckTriStateOption(par.mfx.LowPower))
        changed = true;

    if (IsOn(par.mfx.LowPower))
    {
        unsupported = true;
        par.mfx.LowPower = 0;
    }

    if (par.IOPattern != 0)
    {
        if (par.IOPattern != MFX_IOPATTERN_IN_VIDEO_MEMORY)
        {
            unsupported = true;
        }
    }

    if (par.mfx.GopPicSize > 0 &&
        par.mfx.GopRefDist > 0 &&
        par.mfx.GopRefDist > par.mfx.GopPicSize)
    {
        changed = true;
        par.mfx.GopRefDist = par.mfx.GopPicSize - 1;
    }

    if (par.mfx.GopRefDist > 1)
    {
        if (IsAvcBaseProfile(par.mfx.CodecProfile) ||
            par.mfx.CodecProfile == MFX_PROFILE_AVC_CONSTRAINED_HIGH)
        {
            changed = true;
            par.mfx.GopRefDist = 1;
        }
    }

    /* max allowed combination */
    if (par.mfx.FrameInfo.PicStruct > (MFX_PICSTRUCT_PART1|MFX_PICSTRUCT_PART2))
    { /* */
        unsupported = true;
        par.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_UNKNOWN;
    }

    if (par.mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_PART2)
    { // repeat/double/triple flags are for EncodeFrameAsync
        changed = true;
        par.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_UNKNOWN;
    }

    mfxU16 picStructPart1 = par.mfx.FrameInfo.PicStruct;
    if (par.mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_PART1)
    {
        if (picStructPart1 & (picStructPart1 - 1))
        { // more then one flag set
            changed = true;
            par.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_UNKNOWN;
        }
    }

    if (par.mfx.FrameInfo.Width & 15)
    {
        unsupported = true;
        par.mfx.FrameInfo.Width = 0;
    }

    if (par.mfx.FrameInfo.Height & 15)
    {
        unsupported = true;
        par.mfx.FrameInfo.Height = 0;
    }

    if ((par.mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_PROGRESSIVE) == 0 &&
        (par.mfx.FrameInfo.Height & 31) != 0)
    {
        unsupported = true;
        par.mfx.FrameInfo.PicStruct = 0;
        par.mfx.FrameInfo.Height = 0;
    }

    // driver doesn't support resolution 16xH
    if ((0 == par.mfx.FrameInfo.Width) || (par.mfx.FrameInfo.Width == 16))
    {
        unsupported = true;
        par.mfx.FrameInfo.Width = 0;
    }

    // driver doesn't support resolution Wx16
    if ((0 == par.mfx.FrameInfo.Height) || (par.mfx.FrameInfo.Height == 16))
    {
        unsupported = true;
        par.mfx.FrameInfo.Height = 0;
    }

    if (par.mfx.FrameInfo.Width > 0)
    {
        if (par.mfx.FrameInfo.CropX > par.mfx.FrameInfo.Width)
        {
            unsupported = true;
            par.mfx.FrameInfo.CropX = 0;
        }

        if (par.mfx.FrameInfo.CropX + par.mfx.FrameInfo.CropW > par.mfx.FrameInfo.Width)
        {
            unsupported = true;
            par.mfx.FrameInfo.CropW = par.mfx.FrameInfo.Width - par.mfx.FrameInfo.CropX;
        }
    }

    if (par.mfx.FrameInfo.Height > 0)
    {
        if (par.mfx.FrameInfo.CropY > par.mfx.FrameInfo.Height)
        {
            unsupported = true;
            par.mfx.FrameInfo.CropY = 0;
        }

        if (par.mfx.FrameInfo.CropY + par.mfx.FrameInfo.CropH > par.mfx.FrameInfo.Height)
        {
            unsupported = true;
            par.mfx.FrameInfo.CropH = par.mfx.FrameInfo.Height - par.mfx.FrameInfo.CropY;
        }
    }

    if (CorrectCropping(par) != MFX_ERR_NONE)
    { // cropping read from sps header always has correct alignment
        changed = true;
    }

    if (par.mfx.FrameInfo.ChromaFormat != 0 &&
        par.mfx.FrameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV420 &&
        par.mfx.FrameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV422 &&
        par.mfx.FrameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV444)
    {
        changed = true;
        par.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
    }

    if (par.mfx.FrameInfo.FourCC != 0 &&
        par.mfx.FrameInfo.FourCC != MFX_FOURCC_NV12)
    {
        unsupported = true;
        par.mfx.FrameInfo.FourCC = 0;
    }

    if (par.mfx.FrameInfo.FourCC == MFX_FOURCC_NV12 &&
        par.mfx.FrameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV420 &&
        par.mfx.FrameInfo.ChromaFormat != MFX_CHROMAFORMAT_MONOCHROME)
    {
        changed = true;
        par.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
    }

    if (hwCaps.ddi_caps.Color420Only &&
        (par.mfx.FrameInfo.ChromaFormat == MFX_CHROMAFORMAT_YUV422 ||
         par.mfx.FrameInfo.ChromaFormat == MFX_CHROMAFORMAT_YUV444))
    {
        unsupported = true;
        par.mfx.FrameInfo.ChromaFormat = 0;
    }

    if (!CheckTriStateOption(feiParam->SingleFieldProcessing))
    {
        unsupported = true;
    }

    if (IsOn(feiParam->SingleFieldProcessing) && (par.mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_PROGRESSIVE))
    {
        unsupported = true;
    }

//    if (par.mfx.NumRefFrame == 0)
//    {
//        changed = true;
//        par.mfx.NumRefFrame = 1;
//    }

//    if ((par.AsyncDepth == 0) || (par.AsyncDepth > 1))
//    {
//        /* Supported only Async 1 */
//        changed = true;
//        par.AsyncDepth = 1;
//    }

    return unsupported
        ? MFX_ERR_UNSUPPORTED
        : (changed || warning)
            ? MFX_WRN_INCOMPATIBLE_VIDEO_PARAM
            : MFX_ERR_NONE;
}

} // namespace MfxEncPREENC

bool bEnc_PREENC(mfxVideoParam *par)
{
    MFX_CHECK(par, false);

    mfxExtFeiParam *pControl = NULL;

    for (mfxU16 i = 0; i < par->NumExtParam; ++i)
    {
        if (par->ExtParam[i] != 0 && par->ExtParam[i]->BufferId == MFX_EXTBUFF_FEI_PARAM)
        {
            pControl = reinterpret_cast<mfxExtFeiParam *>(par->ExtParam[i]);
            break;
        }
    }

    return pControl ? (pControl->Func == MFX_FEI_FUNCTION_PREENC) : false;
}

static mfxStatus AsyncRoutine(void * state, void * param, mfxU32, mfxU32)
{
    MFX_CHECK_NULL_PTR1(state);

    VideoENC_PREENC & impl = *(VideoENC_PREENC *)state;
    return  impl.RunFrameVmeENC(NULL, NULL);
}

mfxStatus VideoENC_PREENC::RunFrameVmeENC(mfxENCInput *in, mfxENCOutput *out)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VideoENC_PREENC::RunFrameVmeENC");

    mfxStatus sts = MFX_ERR_NONE;
    DdiTask & task = m_incoming.front();
    mfxU32 f = 0, f_start = 0;
    mfxU32 fieldCount = task.m_fieldPicFlag;

    task.m_idx    = FindFreeResourceIndex(m_raw);
    task.m_midRaw = AcquireResource(m_raw, task.m_idx);

    sts = GetNativeHandleToRawSurface(*m_core, m_video, task, task.m_handleRaw);
    MFX_CHECK(sts == MFX_ERR_NONE, Error(sts));

    sts = CopyRawSurfaceToVideoMemory(*m_core, m_video, task);
    MFX_CHECK(sts == MFX_ERR_NONE, Error(sts));

    if (m_bSingleFieldMode)
    {
        // Set a field to process
        f_start = fieldCount = m_firstFieldDone;
    }

    for (f = f_start; f <= fieldCount; f++)
    {
        sts = m_ddi->Execute(task.m_handleRaw, task, task.m_fid[f], m_sei);
        MFX_CHECK(sts == MFX_ERR_NONE, Error(sts));
    }

    // After encoding of field - switch m_firstFieldDone flag between 0 and 1
    if (m_bSingleFieldMode)
    {
        m_firstFieldDone = 1 - m_firstFieldDone;
    }

    return sts;
}

static mfxStatus AsyncQuery(void * state, void * param, mfxU32 /*threadNumber*/, mfxU32 /*callNumber*/)
{
    MFX_CHECK_NULL_PTR2(state, param);

    VideoENC_PREENC & impl = *(VideoENC_PREENC *)state;
    DdiTask& task = *(DdiTask *)param;
    return impl.QueryStatus(task);
}

mfxStatus VideoENC_PREENC::QueryStatus(DdiTask& task)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxU32 f = 0, f_start = 0;
    mfxU32 fieldCount = task.m_fieldPicFlag;

    if (m_bSingleFieldMode)
    {
        // This flag was flipped at the end of RunFramePAK, so flip it back to check status of correct field
        f_start = fieldCount = 1 - m_firstFieldDone;
    }

    for (f = f_start; f <= fieldCount; f++)
    {
        sts = m_ddi->QueryStatus(task, task.m_fid[f]);
        MFX_CHECK(sts != MFX_WRN_DEVICE_BUSY, MFX_TASK_BUSY);
        MFX_CHECK(sts == MFX_ERR_NONE, Error(sts));
    }

    m_core->DecreaseReference(&task.m_yuv->Data);

    UMC::AutomaticUMCMutex guard(m_listMutex);
    //move that task to free tasks from m_incoming
    std::list<DdiTask>::iterator it = std::find(m_incoming.begin(), m_incoming.end(), task);
    MFX_CHECK(it != m_incoming.end(), MFX_ERR_NOT_FOUND);

    m_free.splice(m_free.end(), m_incoming, it);

    return MFX_ERR_NONE;
}


mfxStatus VideoENC_PREENC::Query(VideoCORE* core, mfxVideoParam *in, mfxVideoParam *out)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VideoENC_PREENC::Query");
    MFX_CHECK_NULL_PTR2(core, out);
    mfxStatus sts = MFX_ERR_NONE;
    MFX_ENCODE_CAPS hwCaps = { };

    if (NULL == in)
    {
        Zero(out->mfx);

        out->IOPattern                   = MFX_IOPATTERN_IN_VIDEO_MEMORY;
        out->AsyncDepth                  = 1;
        out->mfx.NumRefFrame             = 1;
        out->mfx.NumThread               = 1;
        out->mfx.EncodedOrder            = 1;
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

        return MFX_ERR_NONE;
    }

    MfxVideoParam tmp = *in; // deep copy, to create all ext buffers

    std::unique_ptr<DriverEncoder> ddi;
    ddi.reset( new VAAPIFEIPREENCEncoder );

    sts = ddi->CreateAuxilliaryDevice(
        core,
        DXVA2_Intel_Encode_AVC,
        GetFrameWidth(tmp),
        GetFrameHeight(tmp));

    MFX_CHECK(sts == MFX_ERR_NONE, MFX_WRN_PARTIAL_ACCELERATION);

    sts = ddi->QueryEncodeCaps(hwCaps);
    MFX_CHECK(sts == MFX_ERR_NONE, MFX_WRN_PARTIAL_ACCELERATION);

    mfxStatus checkSts = MfxEncPREENC::CheckVideoParamQueryLikePreEnc(tmp, hwCaps, core->GetHWType(), core->GetVAType());
    MFX_CHECK(checkSts != MFX_WRN_PARTIAL_ACCELERATION, MFX_WRN_PARTIAL_ACCELERATION);

    if (checkSts == MFX_ERR_INCOMPATIBLE_VIDEO_PARAM)
        checkSts = MFX_ERR_UNSUPPORTED;

    out->IOPattern  = tmp.IOPattern;
    out->AsyncDepth = tmp.AsyncDepth;
    out->mfx        = tmp.mfx;

    return checkSts;
} // mfxStatus VideoENC_PREENC::Query(VideoCORE*, mfxVideoParam *in, mfxVideoParam *out)

mfxStatus VideoENC_PREENC::QueryIOSurf(VideoCORE* , mfxVideoParam *par, mfxFrameAllocRequest *request)
{
    MFX_CHECK_NULL_PTR2(par,request);
    return MFX_ERR_NONE;
} // mfxStatus VideoENC_PREENC::QueryIOSurf(VideoCORE* , mfxVideoParam *par, mfxFrameAllocRequest *request)


VideoENC_PREENC::VideoENC_PREENC(VideoCORE *core,  mfxStatus * sts)
    : m_bInit( false )
    , m_core( core )
    , m_caps()
    , m_inputFrameType()
    , m_currentPlatform(MFX_HW_UNKNOWN)
    , m_currentVaType(MFX_HW_NO)
    , m_bSingleFieldMode(false)
    , m_firstFieldDone(0)
{
    *sts = MFX_ERR_NONE;
}


VideoENC_PREENC::~VideoENC_PREENC()
{
    Close();
}

mfxStatus VideoENC_PREENC::Init(mfxVideoParam *par)
{
    MFX_CHECK_NULL_PTR1(par);

    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VideoENC_PREENC::Init");
    mfxStatus sts = MfxEncPREENC::CheckExtBufferId(*par);
    MFX_CHECK_STS(sts);

    m_video = *par;
    //add ext buffers from par to m_video

    /* Check W & H*/
    MFX_CHECK((par->mfx.FrameInfo.Width > 0 && par->mfx.FrameInfo.Height > 0), MFX_ERR_INVALID_VIDEO_PARAM);

    m_ddi.reset( new VAAPIFEIPREENCEncoder );

    sts = m_ddi->CreateAuxilliaryDevice(
        m_core,
        DXVA2_Intel_Encode_AVC,
        GetFrameWidth(m_video),
        GetFrameHeight(m_video));

    MFX_CHECK(sts == MFX_ERR_NONE, MFX_WRN_PARTIAL_ACCELERATION);

    sts = m_ddi->QueryEncodeCaps(m_caps);
    MFX_CHECK(sts == MFX_ERR_NONE, MFX_WRN_PARTIAL_ACCELERATION);

    m_currentPlatform = m_core->GetHWType();
    m_currentVaType   = m_core->GetVAType();

    mfxStatus checkStatus = MfxEncPREENC::CheckVideoParamPreEncInit(m_video, m_caps, m_currentPlatform, m_currentVaType);
    switch (checkStatus)
    {
    case MFX_ERR_UNSUPPORTED:
        return MFX_ERR_INVALID_VIDEO_PARAM;
    case MFX_ERR_INVALID_VIDEO_PARAM:
    case MFX_WRN_PARTIAL_ACCELERATION:
    case MFX_ERR_INCOMPATIBLE_VIDEO_PARAM:
        return checkStatus;
    default:
        break;
    }

    mfxExtFeiParam * feiParam = GetExtBuffer(m_video);
    m_bSingleFieldMode = IsOn(feiParam->SingleFieldProcessing);

    sts = m_ddi->CreateAccelerationService(m_video);
    MFX_CHECK(sts == MFX_ERR_NONE, MFX_WRN_PARTIAL_ACCELERATION);

    sts = MfxH264FEIcommon::CheckInitExtBuffers(m_video, *par);
    MFX_CHECK_STS(sts);

    m_inputFrameType =
        (m_video.IOPattern == MFX_IOPATTERN_IN_SYSTEM_MEMORY || m_video.IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY)
            ? MFX_IOPATTERN_IN_SYSTEM_MEMORY
            : MFX_IOPATTERN_IN_VIDEO_MEMORY;

    m_free.resize(m_video.AsyncDepth);
    m_incoming.clear();

    m_bInit = true;
    return checkStatus;
}

mfxStatus VideoENC_PREENC::Reset(mfxVideoParam *par)
{
    Close();
    return Init(par);
}

mfxStatus VideoENC_PREENC::GetVideoParam(mfxVideoParam *par)
{
    MFX_CHECK_NULL_PTR1(par);

    // For buffers which are field-based
    std::map<mfxU32, mfxU32> buffers_offsets;

    for (mfxU32 i = 0; i < par->NumExtParam; ++i)
    {
        if (buffers_offsets.find(par->ExtParam[i]->BufferId) == buffers_offsets.end())
            buffers_offsets[par->ExtParam[i]->BufferId] = 0;
        else
            buffers_offsets[par->ExtParam[i]->BufferId]++;

        if (mfxExtBuffer * buf = MfxHwH264Encode::GetExtBuffer(m_video.ExtParam, m_video.NumExtParam, par->ExtParam[i]->BufferId, buffers_offsets[par->ExtParam[i]->BufferId]))
        {
            MFX_INTERNAL_CPY(par->ExtParam[i], buf, par->ExtParam[i]->BufferSz);
        }
        else
        {
            return MFX_ERR_UNSUPPORTED;
        }
    }

    mfxExtBuffer ** ExtParam = par->ExtParam;
    mfxU16       NumExtParam = par->NumExtParam;

    MFX_INTERNAL_CPY(par, &(static_cast<mfxVideoParam &>(m_video)), sizeof(mfxVideoParam));

    par->ExtParam    = ExtParam;
    par->NumExtParam = NumExtParam;

    return MFX_ERR_NONE;
}

mfxStatus VideoENC_PREENC::RunFrameVmeENCCheck(
                    mfxENCInput  *input,
                    mfxENCOutput *output,
                    MFX_ENTRY_POINT pEntryPoints[],
                    mfxU32 &numEntryPoints)
{
    MFX_CHECK(m_bInit, MFX_ERR_UNDEFINED_BEHAVIOR);

    MFX_CHECK_NULL_PTR2(input, output);
    MFX_CHECK_NULL_PTR1(input->InSurface);

    /* This value have to be initialized here
     * as we use MfxHwH264Encode::GetPicStruct
     * (Legacy encoder do initialization  by itself)
     * */
    mfxExtCodingOption * extOpt = GetExtBuffer(m_video);
    extOpt->FieldOutput = MFX_CODINGOPTION_OFF;

    PairU16 picStruct = GetPicStruct(m_video, input->InSurface->Info.PicStruct);

    mfxU32  fieldCount = (picStruct[ENC] & MFX_PICSTRUCT_PROGRESSIVE) ? 1 : 2;

    for (mfxU32 i = 0; i < input->NumExtParam; ++i)
    {
        MFX_CHECK_NULL_PTR1(input->ExtParam[i]);
        MFX_CHECK(MfxH264FEIcommon::IsRunTimeInputExtBufferIdSupported(m_video, input->ExtParam[i]->BufferId), MFX_ERR_INVALID_VIDEO_PARAM);

        bool buffer_pair = MfxHwH264Encode::GetExtBuffer(input->ExtParam + i + 1, input->NumExtParam - i - 1,
                                                         input->ExtParam[i]->BufferId) ||
                           MfxHwH264Encode::GetExtBuffer(input->ExtParam, i, input->ExtParam[i]->BufferId);

        bool buffer_pair_required = MfxH264FEIcommon::IsRunTimeExtBufferPairRequired(m_video, input->ExtParam[i]->BufferId);

        // PreENC: even in single-field mode, paired buffers are required for each field.
        buffer_pair_required = (fieldCount == 2) ? buffer_pair_required : false;

        // input paired buffer check
        MFX_CHECK(!(!buffer_pair && buffer_pair_required), MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
    }

    for (mfxU32 i = 0; i < output->NumExtParam; ++i)
    {
        MFX_CHECK_NULL_PTR1(output->ExtParam[i]);
        MFX_CHECK(MfxH264FEIcommon::IsRunTimeOutputExtBufferIdSupported(m_video, output->ExtParam[i]->BufferId), MFX_ERR_INVALID_VIDEO_PARAM);

        bool buffer_pair = MfxHwH264Encode::GetExtBuffer(output->ExtParam + i + 1, output->NumExtParam - i - 1,
                                                         output->ExtParam[i]->BufferId) ||
                           MfxHwH264Encode::GetExtBuffer(output->ExtParam, i, output->ExtParam[i]->BufferId);

        bool buffer_pair_required = MfxH264FEIcommon::IsRunTimeExtBufferPairRequired(m_video, output->ExtParam[i]->BufferId);

        // PreENC: even in single-field mode, paired buffers are required for each field.
        buffer_pair_required = (fieldCount == 2) ? buffer_pair_required : false;

        //output paired buffer check
        MFX_CHECK(!(!buffer_pair && buffer_pair_required), MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
    }

    // For frame type detection
    PairU8 frame_type = PairU8(mfxU8(MFX_FRAMETYPE_UNKNOWN), mfxU8(MFX_FRAMETYPE_UNKNOWN));

    mfxU32 SizeInMB = input->InSurface->Info.Width * input->InSurface->Info.Height / 256;
    mfxU32 NumMB = (MFX_PICSTRUCT_PROGRESSIVE & picStruct[ENC]) ? SizeInMB : SizeInMB / 2;

    for (mfxU32 field = 0; field < fieldCount; ++field)
    {
        mfxU32 fieldParity = picStruct[ENC] == MFX_PICSTRUCT_FIELD_BFF ? 1 - field : field;

        mfxExtFeiPreEncCtrl* feiCtrl = GetExtBufferFEI(input, field);
        MFX_CHECK_NULL_PTR1(feiCtrl); // this is fatal error

        if      (!feiCtrl->RefFrame[0] && !feiCtrl->RefFrame[1]) frame_type[fieldParity] = MFX_FRAMETYPE_I;
        else if ( feiCtrl->RefFrame[0] && !feiCtrl->RefFrame[1]) frame_type[fieldParity] = MFX_FRAMETYPE_P;
        else                                                     frame_type[fieldParity] = MFX_FRAMETYPE_B;

        MFX_CHECK((feiCtrl->PictureType == MFX_PICTYPE_TOPFIELD    && picStruct[ENC] == (!field ? MFX_PICSTRUCT_FIELD_TFF : MFX_PICSTRUCT_FIELD_BFF)) ||
                  (feiCtrl->PictureType == MFX_PICTYPE_BOTTOMFIELD && picStruct[ENC] == (!field ? MFX_PICSTRUCT_FIELD_BFF : MFX_PICSTRUCT_FIELD_TFF)) ||
                  (feiCtrl->PictureType == MFX_PICTYPE_FRAME       && picStruct[ENC] == MFX_PICSTRUCT_PROGRESSIVE),
                  MFX_ERR_INVALID_VIDEO_PARAM);

        //check DownsampleInput
        MFX_CHECK(feiCtrl->DownsampleInput == MFX_CODINGOPTION_ON  ||
                  feiCtrl->DownsampleInput == MFX_CODINGOPTION_OFF ||
                  feiCtrl->DownsampleInput == MFX_CODINGOPTION_UNKNOWN, MFX_ERR_INVALID_VIDEO_PARAM);

        for (mfxU32 ref = 0; ref < 2; ++ref)
        {
            if (feiCtrl->RefFrame[ref])
            {
                MFX_CHECK(feiCtrl->RefPictureType[ref] == MFX_PICTYPE_TOPFIELD    ||
                          feiCtrl->RefPictureType[ref] == MFX_PICTYPE_BOTTOMFIELD ||
                          feiCtrl->RefPictureType[ref] == MFX_PICTYPE_FRAME, MFX_ERR_INVALID_VIDEO_PARAM);

                //check DownsampleReference
                MFX_CHECK(feiCtrl->DownsampleReference[ref] == MFX_CODINGOPTION_ON  ||
                          feiCtrl->DownsampleReference[ref] == MFX_CODINGOPTION_OFF ||
                          feiCtrl->DownsampleReference[ref] == MFX_CODINGOPTION_UNKNOWN, MFX_ERR_INVALID_VIDEO_PARAM);
            }
        }

        if (!feiCtrl->DisableMVOutput)
        {
            mfxExtFeiPreEncMV* feiPreEncMV = GetExtBufferFEI(output, field);
            MFX_CHECK(feiPreEncMV,                      MFX_ERR_UNDEFINED_BEHAVIOR);
            MFX_CHECK(feiPreEncMV->NumMBAlloc >= NumMB, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
            MFX_CHECK(feiPreEncMV->MB != NULL,          MFX_ERR_UNDEFINED_BEHAVIOR);
        }

        if (!feiCtrl->DisableStatisticsOutput)
        {
            mfxExtFeiPreEncMBStat* feiPreEncMBStat = GetExtBufferFEI(output, field);
            MFX_CHECK(feiPreEncMBStat,                      MFX_ERR_UNDEFINED_BEHAVIOR);
            MFX_CHECK(feiPreEncMBStat->NumMBAlloc >= NumMB, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
            MFX_CHECK(feiPreEncMBStat->MB != NULL,          MFX_ERR_UNDEFINED_BEHAVIOR);
        }

        if (feiCtrl->FTEnable)
        {
            if(feiCtrl->MBQp)
            {
            //check mfxExtFeiEncQP
                mfxExtFeiEncQP* feiEncQP = GetExtBufferFEI(input, field);
                MFX_CHECK(feiEncQP,                      MFX_ERR_UNDEFINED_BEHAVIOR);
                MFX_CHECK(feiEncQP->NumMBAlloc >= NumMB, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
                MFX_CHECK(feiEncQP->MB != NULL,          MFX_ERR_UNDEFINED_BEHAVIOR);
            }

            else
            {
                MFX_CHECK(feiCtrl->Qp > 0 && feiCtrl->Qp <= 51, MFX_ERR_INVALID_VIDEO_PARAM);
            }
        }

        MFX_CHECK(feiCtrl->SubMBPartMask < 0x7f, MFX_ERR_INVALID_VIDEO_PARAM);

        MFX_CHECK(feiCtrl->SubPelMode == 0x00 || feiCtrl->SubPelMode == 0x01 || feiCtrl->SubPelMode == 0x03, MFX_ERR_INVALID_VIDEO_PARAM);

        MFX_CHECK(feiCtrl->InterSAD == 0x00 || feiCtrl->InterSAD == 0x02, MFX_ERR_INVALID_VIDEO_PARAM);

        MFX_CHECK(feiCtrl->IntraSAD == 0x00 || feiCtrl->IntraSAD == 0x02, MFX_ERR_INVALID_VIDEO_PARAM);

        MFX_CHECK(feiCtrl->IntraPartMask < 0x07, MFX_ERR_INVALID_VIDEO_PARAM);

        MFX_CHECK(feiCtrl->SearchWindow > 0 && feiCtrl->SearchWindow < 9, MFX_ERR_INVALID_VIDEO_PARAM);

        MFX_CHECK(feiCtrl->MVPredictor <= 0x03, MFX_ERR_INVALID_VIDEO_PARAM);

        //check mfxExtFeiPreEncMVPredictors
        if (feiCtrl->MVPredictor > 0x00)
        {
            mfxExtFeiPreEncMVPredictors* feiPreEncMVPredictors = GetExtBufferFEI(input, field);
            MFX_CHECK(feiPreEncMVPredictors,                       MFX_ERR_UNDEFINED_BEHAVIOR);
            MFX_CHECK(feiPreEncMVPredictors->NumMBAlloc >= NumMB,  MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
            MFX_CHECK(feiPreEncMVPredictors->MB != NULL,           MFX_ERR_UNDEFINED_BEHAVIOR);
        }

    }

    UMC::AutomaticUMCMutex guard(m_listMutex);

    m_free.front().m_yuv          = input->InSurface;
    //m_free.front().m_ctrl = 0;
    m_free.front().m_type         = frame_type;
    m_free.front().m_picStruct    = picStruct;
    m_free.front().m_fieldPicFlag = picStruct[ENC] != MFX_PICSTRUCT_PROGRESSIVE;
    m_free.front().m_fid[0]       = picStruct[ENC] == MFX_PICSTRUCT_FIELD_BFF;
    m_free.front().m_fid[1]       = m_free.front().m_fieldPicFlag - m_free.front().m_fid[0];
    m_free.front().m_extFrameTag  = input->InSurface->Data.FrameOrder;
    m_free.front().m_frameOrder   = input->InSurface->Data.FrameOrder;
    m_free.front().m_timeStamp    = input->InSurface->Data.TimeStamp;
    m_free.front().m_userData.resize(2);
    m_free.front().m_userData[0]  = input;
    m_free.front().m_userData[1]  = output;

    m_incoming.splice(m_incoming.end(), m_free, m_free.begin());
    DdiTask& task = m_incoming.front();


    /* We need to match picture types...
     * (1): If Init() was for "MFX_PICSTRUCT_UNKNOWN", all Picture types is allowed
     * (2): Else allowed only picture type which was on Init() stage
     *  */
    mfxU16 picStructTask = task.GetPicStructForEncode(), picStructInit = m_video.mfx.FrameInfo.PicStruct;
    MFX_CHECK(MFX_PICSTRUCT_UNKNOWN == picStructInit || picStructInit == picStructTask, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM)

    m_core->IncreaseReference(&input->InSurface->Data);

    pEntryPoints[0].pState               = this;
    pEntryPoints[0].pParam               = &m_incoming.front();
    pEntryPoints[0].pCompleteProc        = 0;
    pEntryPoints[0].pGetSubTaskProc      = 0;
    pEntryPoints[0].pCompleteSubTaskProc = 0;
    pEntryPoints[0].requiredNumThreads   = 1;
    pEntryPoints[0].pRoutineName         = "AsyncRoutine";
    pEntryPoints[0].pRoutine             = AsyncRoutine;
    pEntryPoints[1] = pEntryPoints[0];
    pEntryPoints[1].pRoutineName         = "Async Query";
    pEntryPoints[1].pRoutine             = AsyncQuery;
    pEntryPoints[1].pParam               = &m_incoming.front();

    numEntryPoints = 2;

    return MFX_ERR_NONE;
}


mfxStatus VideoENC_PREENC::Close(void)
{
    MFX_CHECK(m_bInit, MFX_ERR_NONE);
    m_bInit = false;
    m_ddi->Destroy();

    m_core->FreeFrames(&m_raw);

    return MFX_ERR_NONE;
}


#endif  // defined(MFX_VA) && defined(MFX_ENABLE_H264_VIDEO_ENCODE_HW) && defined(MFX_ENABLE_H264_VIDEO_FEI_PREENC)
