// Copyright (c) 2019 Intel Corporation
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

#if defined(MFX_ENABLE_H264_VIDEO_ENCODE_HW) && defined(MFX_ENABLE_H264_VIDEO_FEI_ENC)

#include "mfx_h264_enc.h"

#if defined(_DEBUG)
#define mdprintf fprintf
#else
#define mdprintf(...)
#endif

using namespace MfxHwH264Encode;
using namespace MfxH264FEIcommon;

namespace MfxEncENC
{

static bool IsVideoParamExtBufferIdSupported(mfxU32 id)
{
    return
        id == MFX_EXTBUFF_FEI_PPS        ||
        id == MFX_EXTBUFF_FEI_SPS        ||
        id == MFX_EXTBUFF_CODING_OPTION  ||
        id == MFX_EXTBUFF_CODING_OPTION2 ||
        id == MFX_EXTBUFF_CODING_OPTION3 ||
        id == MFX_EXTBUFF_FEI_PARAM;
}


static mfxStatus CheckExtBufferId(mfxVideoParam const & par)
{
    for (mfxU32 i = 0; i < par.NumExtParam; ++i)
    {
        MFX_CHECK(par.ExtParam[i], MFX_ERR_INVALID_VIDEO_PARAM);
        MFX_CHECK(MfxEncENC::IsVideoParamExtBufferIdSupported(par.ExtParam[i]->BufferId), MFX_ERR_INVALID_VIDEO_PARAM);
        MFX_CHECK(!MfxHwH264Encode::GetExtBuffer(
            par.ExtParam + i + 1,
            par.NumExtParam - i - 1,
            par.ExtParam[i]->BufferId), MFX_ERR_INVALID_VIDEO_PARAM);
    }

    return MFX_ERR_NONE;
}

} // namespace MfxEncENC

bool bEnc_ENC(mfxVideoParam *par)
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

    return pControl ? (pControl->Func == MFX_FEI_FUNCTION_ENC) : false;
}

static mfxStatus AsyncRoutine(void * state, void * param, mfxU32, mfxU32)
{
    MFX_CHECK_NULL_PTR1(state);

    VideoENC_ENC & impl = *(VideoENC_ENC *)state;
    return  impl.RunFrameVmeENC(NULL, NULL);
}

mfxStatus VideoENC_ENC::RunFrameVmeENC(mfxENCInput *in, mfxENCOutput *out)
{
    mdprintf(stderr, "VideoENC_ENC::RunFrameVmeENC\n");

    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VideoENC_ENC::RunFrameVmeENC");

    mfxStatus sts = MFX_ERR_NONE;
    DdiTask & task = m_incoming.front();

    mfxU32 f_start = 0, fieldCount = task.m_fieldPicFlag;

    if (m_bSingleFieldMode)
    {
        // Set a field to process
        fieldCount = f_start = m_firstFieldDone; // 0 or 1
    }

    for (mfxU32 f = f_start; f <= fieldCount; ++f)
    {
        sts = m_ddi->Execute(task.m_handleRaw, task, task.m_fid[f], m_sei);
        MFX_CHECK(sts == MFX_ERR_NONE, Error(sts));
    }

    // After encoding of field - switch m_firstFieldDone flag between 0 and 1
    if (m_bSingleFieldMode)
    {
        m_firstFieldDone = 1 - m_firstFieldDone;
    }

    // In case of second field being encoded store this task as previous one for next frame
    if (0 == m_firstFieldDone)
    {
        m_prevTask = task;
    }

    return sts;
}

static mfxStatus AsyncQuery(void * state, void * param, mfxU32 /*threadNumber*/, mfxU32 /*callNumber*/)
{
    MFX_CHECK_NULL_PTR2(state, param);

    VideoENC_ENC & impl = *(VideoENC_ENC *)state;
    DdiTask      & task = *(DdiTask *)param;

    return impl.QueryStatus(task);
}

mfxStatus VideoENC_ENC::QueryStatus(DdiTask& task)
{
    mdprintf(stderr, "VideoENC_ENC::QueryStatus\n");

    mfxStatus sts = MFX_ERR_NONE;

    mfxU32 f_start = 0, fieldCount = task.m_fieldPicFlag;

    if (m_bSingleFieldMode)
    {
        // This flag was flipped at the end of RunFramePAK, so flip it back to check status of correct field
        f_start = fieldCount = 1 - m_firstFieldDone;
    }

    for (mfxU32 f = f_start; f <= fieldCount; f++)
    {
        sts = m_ddi->QueryStatus(task, task.m_fid[f]);
        MFX_CHECK(sts != MFX_WRN_DEVICE_BUSY, MFX_TASK_BUSY);
        MFX_CHECK(sts == MFX_ERR_NONE, Error(sts));
    }

    mfxENCInput  *input  = reinterpret_cast<mfxENCInput  *>(task.m_userData[0]);
    mfxENCOutput *output = reinterpret_cast<mfxENCOutput *>(task.m_userData[1]);

    m_core->DecreaseReference(&input->InSurface->Data);
    m_core->DecreaseReference(&output->OutSurface->Data);

    UMC::AutomaticUMCMutex guard(m_listMutex);
    //move that task to free tasks from m_incoming
    std::list<DdiTask>::iterator it = std::find(m_incoming.begin(), m_incoming.end(), task);
    MFX_CHECK(it != m_incoming.end(), MFX_ERR_NOT_FOUND);

    m_free.splice(m_free.end(), m_incoming, it);

    ReleaseResource(m_rec, task.m_midRec);

    return MFX_ERR_NONE;
}

mfxStatus VideoENC_ENC::QueryIOSurf(VideoCORE* , mfxVideoParam *par, mfxFrameAllocRequest *request)
{
    MFX_CHECK_NULL_PTR2(par,request);

    mfxU32 inPattern = par->IOPattern & MFX_IOPATTERN_IN_MASK;
    MFX_CHECK(
        inPattern == MFX_IOPATTERN_IN_SYSTEM_MEMORY ||
        inPattern == MFX_IOPATTERN_IN_VIDEO_MEMORY  ||
        inPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY,
        MFX_ERR_INVALID_VIDEO_PARAM);

    if (inPattern == MFX_IOPATTERN_IN_SYSTEM_MEMORY)
    {
        request->Type =
            MFX_MEMTYPE_EXTERNAL_FRAME |
            MFX_MEMTYPE_FROM_ENCODE    |
            MFX_MEMTYPE_SYSTEM_MEMORY;
    }
    else // MFX_IOPATTERN_IN_VIDEO_MEMORY || MFX_IOPATTERN_IN_OPAQUE_MEMORY
    {
        request->Type  = MFX_MEMTYPE_FROM_ENCODE | MFX_MEMTYPE_DXVA2_DECODER_TARGET;
        request->Type |= (inPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY)
            ? MFX_MEMTYPE_OPAQUE_FRAME
            : MFX_MEMTYPE_EXTERNAL_FRAME;
    }
    /* 2*par->mfx.GopRefDist */
    request->NumFrameMin         = 2*par->mfx.GopRefDist + par->AsyncDepth;
    request->NumFrameSuggested   = request->NumFrameMin;
    request->Info                = par->mfx.FrameInfo;
    return MFX_ERR_NONE;
}


VideoENC_ENC::VideoENC_ENC(VideoCORE *core,  mfxStatus * sts)
    : m_bInit(false)
    , m_core(core)
    , m_caps()
    , m_video()
    , m_videoInit()
    , m_prevTask()
    , m_inputFrameType()
    , m_currentPlatform(MFX_HW_UNKNOWN)
    , m_currentVaType(MFX_HW_NO)
    , m_bSingleFieldMode(false)
    , m_firstFieldDone(0)
{
    *sts = MFX_ERR_NONE;
}


VideoENC_ENC::~VideoENC_ENC()
{
    Close();
}

mfxStatus VideoENC_ENC::Init(mfxVideoParam *par)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VideoENC_ENC::Init");

    MFX_CHECK_NULL_PTR1(par);

    mfxStatus sts = MfxEncENC::CheckExtBufferId(*par);
    MFX_CHECK_STS(sts);

    m_video = *par;
    //add ext buffers from par to m_video

    MfxVideoParam tmp(*par);

    sts = CheckWidthAndHeight(tmp);
    MFX_CHECK_STS(sts);

    m_ddi.reset(new VAAPIFEIENCEncoder);

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

    mfxStatus checkStatus = CheckVideoParam(m_video, m_caps, m_core->IsExternalFrameAllocator(), m_currentPlatform, m_currentVaType);
    MFX_CHECK(checkStatus != MFX_WRN_PARTIAL_ACCELERATION, MFX_ERR_INVALID_VIDEO_PARAM);
    MFX_CHECK(checkStatus >= MFX_ERR_NONE, checkStatus);

    const mfxExtFeiParam* params = GetExtBuffer(m_video);

    m_bSingleFieldMode = IsOn(params->SingleFieldProcessing);

    //raw surfaces should be created before accel service
    mfxFrameAllocRequest request = { };
    request.Info = m_video.mfx.FrameInfo;

    /* ENC does not generate real reconstruct surface,
     * and this surface should be unchanged
     * BUT (!)
     * (1): this surface should be from reconstruct surface pool which was passed to
     * component when vaCreateContext was called
     * (2): And it should be same surface which will be used for PAK reconstructed in next call
     * (3): And main rule: ENC (N number call) and PAK (N number call) should have same exactly
     * same reference /reconstruct list !
     * */
    request.Type              = MFX_MEMTYPE_FROM_ENC | MFX_MEMTYPE_DXVA2_DECODER_TARGET | MFX_MEMTYPE_INTERNAL_FRAME;
    request.NumFrameMin       = m_video.AsyncDepth + m_video.mfx.NumRefFrame;
    request.NumFrameSuggested = request.NumFrameMin;
    request.AllocId           = par->AllocId;

    sts = m_rec.Alloc(m_core, request, false, true);
    MFX_CHECK_STS(sts);

    sts = m_ddi->Register(m_rec, D3DDDIFMT_NV12);
    MFX_CHECK_STS(sts);

    m_recFrameOrder.resize(m_rec.NumFrameActual, 0xffffffff);

    sts = CheckInitExtBuffers(m_video, *par);
    MFX_CHECK_STS(sts);

    sts = m_ddi->CreateAccelerationService(m_video);
    MFX_CHECK(sts == MFX_ERR_NONE, MFX_WRN_PARTIAL_ACCELERATION);

    m_inputFrameType =
        (m_video.IOPattern == MFX_IOPATTERN_IN_SYSTEM_MEMORY || m_video.IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY)
            ? MFX_IOPATTERN_IN_SYSTEM_MEMORY
            : MFX_IOPATTERN_IN_VIDEO_MEMORY;


    m_free.resize(m_video.AsyncDepth);
    m_incoming.clear();

    m_videoInit = m_video;

    m_bInit = true;
    return checkStatus;
}

mfxStatus VideoENC_ENC::Reset(mfxVideoParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;
    MFX_CHECK_NULL_PTR1(par);

    sts = MfxEncENC::CheckExtBufferId(*par);
    MFX_CHECK_STS(sts);

    MfxVideoParam newPar = *par;

    bool isIdrRequired = false;
    mfxStatus checkStatus = ProcessAndCheckNewParameters(newPar, isIdrRequired, par);
    if (checkStatus < MFX_ERR_NONE)
        return checkStatus;

    m_ddi->Reset(newPar);

    mfxExtEncoderResetOption const * extResetOpt = GetExtBuffer(newPar);

    // perform reset of encoder and start new sequence with IDR in following cases:
    // 1) change of encoding parameters require insertion of IDR
    // 2) application explicitly asked about starting new sequence
    if (isIdrRequired || IsOn(extResetOpt->StartNewSequence))
    {
        UMC::AutomaticUMCMutex guard(m_listMutex);
        m_free.splice(m_free.end(), m_incoming);

        for (DdiTaskIter i = m_free.begin(); i != m_free.end(); ++i)
        {
            if (i->m_yuv)
                m_core->DecreaseReference(&i->m_yuv->Data);
            *i = DdiTask();
        }

        m_rec.Unlock();
    }

    m_video = newPar;

    return checkStatus;
}

mfxStatus VideoENC_ENC::GetVideoParam(mfxVideoParam *par)
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

mfxStatus VideoENC_ENC::ProcessAndCheckNewParameters(
    MfxVideoParam & newPar,
    bool & isIdrRequired,
    mfxVideoParam const * newParIn)
{
    MFX_CHECK_NULL_PTR1(newParIn);

    InheritDefaultValues(m_video, newPar, m_caps, newParIn);

    mfxStatus checkStatus = CheckVideoParam(newPar, m_caps, m_core->IsExternalFrameAllocator(), m_currentPlatform, m_currentVaType);
    MFX_CHECK(checkStatus != MFX_WRN_PARTIAL_ACCELERATION, MFX_ERR_INVALID_VIDEO_PARAM);
    MFX_CHECK(checkStatus >= MFX_ERR_NONE, checkStatus);

    mfxStatus sts = CheckInitExtBuffers(m_video, *newParIn);
    MFX_CHECK_STS(sts);

    mfxExtSpsHeader const & extSpsNew = GetExtBufferRef(newPar);
    mfxExtSpsHeader const & extSpsOld = GetExtBufferRef(m_video);

    // check if IDR required after change of encoding parameters
    bool isSpsChanged = extSpsNew.vuiParametersPresentFlag == 0 ?
        memcmp(&extSpsNew, &extSpsOld, sizeof(mfxExtSpsHeader) - sizeof(VuiParameters)) != 0 :
    !Equal(extSpsNew, extSpsOld);

    isIdrRequired = isSpsChanged
        || newPar.mfx.GopPicSize != m_video.mfx.GopPicSize;

    mfxExtEncoderResetOption * extResetOpt = GetExtBuffer(newPar);

    // Reset can't change parameters w/o IDR. Report an error
    MFX_CHECK(!(isIdrRequired && IsOff(extResetOpt->StartNewSequence)), MFX_ERR_INVALID_VIDEO_PARAM);

    MFX_CHECK(
        IsAvcProfile(newPar.mfx.CodecProfile)                                   &&
        m_video.AsyncDepth                 == newPar.AsyncDepth                 &&
        m_videoInit.mfx.GopRefDist         >= newPar.mfx.GopRefDist             &&
        m_videoInit.mfx.NumSlice           >= newPar.mfx.NumSlice               &&
        m_videoInit.mfx.NumRefFrame        >= newPar.mfx.NumRefFrame            &&
        m_video.mfx.RateControlMethod      == newPar.mfx.RateControlMethod      &&
        m_videoInit.mfx.FrameInfo.Width    >= newPar.mfx.FrameInfo.Width        &&
        m_videoInit.mfx.FrameInfo.Height   >= newPar.mfx.FrameInfo.Height       &&
        m_video.mfx.FrameInfo.ChromaFormat == newPar.mfx.FrameInfo.ChromaFormat,
        MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);

    mfxExtCodingOption * extOptNew = GetExtBuffer(newPar);
    mfxExtCodingOption * extOptOld = GetExtBuffer(m_video);

    MFX_CHECK(
        IsOn(extOptOld->FieldOutput) || extOptOld->FieldOutput == extOptNew->FieldOutput,
        MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);

    return checkStatus;
}


mfxStatus VideoENC_ENC::RunFrameVmeENCCheck(
                    mfxENCInput  *input,
                    mfxENCOutput *output,
                    MFX_ENTRY_POINT pEntryPoints[],
                    mfxU32 &numEntryPoints)
{
    mdprintf(stderr, "VideoENC_ENC::RunFrameVmeENCCheck\n");

    // Check that all appropriate parameters passed

    MFX_CHECK(m_bInit, MFX_ERR_UNDEFINED_BEHAVIOR);

    MFX_CHECK_NULL_PTR2(input, output);
    MFX_CHECK_NULL_PTR2(input->InSurface, output->OutSurface);

    // Frame order -1 is forbidden
    MFX_CHECK(input->InSurface->Data.FrameOrder != 0xffffffff, MFX_ERR_UNDEFINED_BEHAVIOR);

    output->OutSurface->Data.FrameOrder = input->InSurface->Data.FrameOrder;

    // Configure new task

    UMC::AutomaticUMCMutex guard(m_listMutex);

    m_free.front().m_yuv         = input->InSurface;
    //m_free.front().m_ctrl      = 0;

    m_free.front().m_extFrameTag = input->InSurface->Data.FrameOrder;
    m_free.front().m_frameOrder  = input->InSurface->Data.FrameOrder;
    m_free.front().m_timeStamp   = input->InSurface->Data.TimeStamp;
    m_free.front().m_userData.resize(2);
    m_free.front().m_userData[0] = input;
    m_free.front().m_userData[1] = output;

    m_incoming.splice(m_incoming.end(), m_free, m_free.begin());
    DdiTask& task = m_incoming.front();

    task.m_singleFieldMode = m_bSingleFieldMode;
    task.m_picStruct       = GetPicStruct(m_video, task);
    task.m_fieldPicFlag    = task.m_picStruct[ENC] != MFX_PICSTRUCT_PROGRESSIVE;
    task.m_fid[0]          = task.m_picStruct[ENC] == MFX_PICSTRUCT_FIELD_BFF;
    task.m_fid[1]          = task.m_fieldPicFlag - task.m_fid[0];

    mfxStatus sts = CheckRuntimeExtBuffers(input, output, m_video, task);
    MFX_CHECK_STS(sts);

    // For frame type detection
    PairU8 frame_type = PairU8(mfxU8(MFX_FRAMETYPE_UNKNOWN), mfxU8(MFX_FRAMETYPE_UNKNOWN));

    mfxU32 f_start = 0, fieldCount = m_video.mfx.FrameInfo.PicStruct == MFX_PICSTRUCT_PROGRESSIVE ? 0 : 1;

    if (m_bSingleFieldMode)
    {
        // Set a field to process
        fieldCount = f_start = m_firstFieldDone = FirstFieldProcessingDone(input, task); // 0 or 1
    }

    for (mfxU32 field = f_start; field <= fieldCount; ++field)
    {
        // In case of single-field processing, only one buffer is attached
        mfxU32 idxToPickBuffer = m_bSingleFieldMode ? 0 : field;
        mfxU32 fieldParity     = task.m_fid[field];

        // Additionally check ENC specific requirements for extension buffers
        mfxExtFeiEncMV     * mvout     = GetExtBufferFEI(output, idxToPickBuffer);
        mfxExtFeiPakMBCtrl * mbcodeout = GetExtBufferFEI(output, idxToPickBuffer);

        // Driver need both buffer to generate bitstream. If they both are not provided, driver will create them on his side
        MFX_CHECK(!!mvout == !!mbcodeout, MFX_ERR_INVALID_VIDEO_PARAM);

        // Check FrameCtrl settings
        mfxExtFeiEncFrameCtrl * frameCtrl = GetExtBufferFEI(input, idxToPickBuffer);
        MFX_CHECK(frameCtrl,                    MFX_ERR_UNDEFINED_BEHAVIOR);
        MFX_CHECK(frameCtrl->SearchWindow != 0, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);

        mfxExtCodingOption const * extOpt = GetExtBuffer(m_video);
        if (((m_video.mfx.CodecProfile & 0xff) == MFX_PROFILE_AVC_BASELINE || (m_video.mfx.CodecProfile & 0xff) == MFX_PROFILE_AVC_MAIN
            || extOpt->IntraPredBlockSize == MFX_BLOCKSIZE_MIN_16X16) && !(frameCtrl->IntraPartMask & 0x02))
        {
            // For Main and Baseline profiles 8x8 transform is prohibited
            return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;
        }

        // Frame type detection
        mfxExtFeiPPS* extFeiPPSinRuntime = GetExtBufferFEI(input, idxToPickBuffer);

#if MFX_VERSION >= 1023
        mfxU8 type = extFeiPPSinRuntime->FrameType;
#else
        mfxU8 type = extFeiPPSinRuntime->PictureType;
#endif // MFX_VERSION >= 1023

        // MFX_FRAMETYPE_xI / P / B disallowed here
        MFX_CHECK(!(type & 0xff00), MFX_ERR_UNDEFINED_BEHAVIOR);

        switch (type & 0xf)
        {
        case MFX_FRAMETYPE_UNKNOWN:
        case MFX_FRAMETYPE_I:
        case MFX_FRAMETYPE_P:
        case MFX_FRAMETYPE_B:
            break;
        default:
            return MFX_ERR_UNDEFINED_BEHAVIOR;
            break;
        }

        frame_type[fieldParity] = type;

        // Having both valid frame types simplifies calculations in ConfigureTask
        if (m_bSingleFieldMode || fieldCount < 1)
        {
            frame_type[!fieldParity] = m_bSingleFieldMode && field != 0 ? task.m_type[!fieldParity] : (frame_type[fieldParity] & ~MFX_FRAMETYPE_IDR);
        }
    }

    task.m_type = frame_type;

    m_core->IncreaseReference(&input->InSurface->Data);
    m_core->IncreaseReference(&output->OutSurface->Data);

    // Configure current task
    {
        sts = GetNativeHandleToRawSurface(*m_core, m_video, task, task.m_handleRaw);
        MFX_CHECK(sts == MFX_ERR_NONE, Error(sts));

        mfxHDL handle_src, handle_rec;
        sts = m_core->GetExternalFrameHDL(output->OutSurface->Data.MemId, &handle_src);
        MFX_CHECK(sts == MFX_ERR_NONE, MFX_ERR_INVALID_HANDLE);

        mfxMemId midRec;
        mfxU32 *src_surf_id = (mfxU32 *)handle_src, *rec_surf_id, i;

        for (i = 0; i < m_rec.NumFrameActual; ++i)
        {
            midRec = AcquireResource(m_rec, i);

            sts = m_core->GetFrameHDL(m_rec.mids[i], &handle_rec);
            MFX_CHECK(MFX_ERR_NONE == sts, MFX_ERR_INVALID_HANDLE);

            rec_surf_id = (mfxU32 *)handle_rec;
            if ((*src_surf_id) == (*rec_surf_id))
            {
                task.m_idxRecon = i;
                task.m_midRec   = midRec;
                break;
            }
            else
            {
                ReleaseResource(m_rec, midRec);
            }
        }
        MFX_CHECK(task.m_idxRecon != NO_INDEX && task.m_midRec != MID_INVALID && i != m_rec.NumFrameActual, MFX_ERR_UNDEFINED_BEHAVIOR);

#if MFX_VERSION >= 1023
        ConfigureTaskFEI(task, m_prevTask, m_video, input);
#else
        ConfigureTaskFEI(task, m_prevTask, m_video, input, input, m_frameOrder_frameNum);
#endif // MFX_VERSION >= 1023

        // Change DPB
        m_recFrameOrder[task.m_idxRecon] = task.m_frameOrder;

        for (mfxU32 field = f_start; field <= fieldCount; ++field)
        {
            const mfxU32 & fieldParity = task.m_fid[field];
            sts = Change_DPB(task.m_dpb[fieldParity], m_rec.mids, m_recFrameOrder);
            MFX_CHECK(sts == MFX_ERR_NONE, Error(sts));
        }

        sts = Change_DPB(task.m_dpbPostEncoding, m_rec.mids, m_recFrameOrder);
        MFX_CHECK(sts == MFX_ERR_NONE, Error(sts));
    }

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


mfxStatus VideoENC_ENC::Close(void)
{
    MFX_CHECK(m_bInit, MFX_ERR_NONE);
    m_bInit = false;
    m_ddi->Destroy();

    m_core->FreeFrames(&m_rec);
    //m_core->FreeFrames(&m_opaqHren);

    return MFX_ERR_NONE;
}

#endif  // if defined(MFX_VA) && defined(MFX_ENABLE_H264_VIDEO_ENCODE_HW) && defined(MFX_ENABLE_H264_VIDEO_FEI_ENC)
