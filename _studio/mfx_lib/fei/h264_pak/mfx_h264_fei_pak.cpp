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

#if defined(MFX_ENABLE_H264_VIDEO_ENCODE_HW) && defined(MFX_ENABLE_H264_VIDEO_FEI_PAK)

#include "mfx_h264_fei_pak.h"

#if defined(_DEBUG)
#define mdprintf fprintf
#else
#define mdprintf(...)
#endif

using namespace MfxHwH264Encode;
using namespace MfxH264FEIcommon;

namespace MfxEncPAK
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
        MFX_CHECK(MfxEncPAK::IsVideoParamExtBufferIdSupported(par.ExtParam[i]->BufferId), MFX_ERR_INVALID_VIDEO_PARAM);
        MFX_CHECK(!MfxHwH264Encode::GetExtBuffer(
            par.ExtParam + i + 1,
            par.NumExtParam - i - 1,
            par.ExtParam[i]->BufferId), MFX_ERR_INVALID_VIDEO_PARAM);
    }

    return MFX_ERR_NONE;
}

} // namespace MfxEncPAK

static const mfxU8 PAK_SUPPORTED_SEI[] = {
        1, // 00 buffering_period
        1, // 01 pic_timing
        1, // 02 pan_scan_rect
        1, // 03 filler_payload
        1, // 04 user_data_registered_itu_t_t35
        1, // 05 user_data_unregistered
        1, // 06 recovery_point
        1, // 07 dec_ref_pic_marking_repetition
        0, // 08 spare_pic
        1, // 09 scene_info
        0, // 10 sub_seq_info
        0, // 11 sub_seq_layer_characteristics
        0, // 12 sub_seq_characteristics
        1, // 13 full_frame_freeze
        1, // 14 full_frame_freeze_release
        1, // 15 full_frame_snapshot
        1, // 16 progressive_refinement_segment_start
        1, // 17 progressive_refinement_segment_end
        0, // 18 motion_constrained_slice_group_set
        1, // 19 film_grain_characteristics
        1, // 20 deblocking_filter_display_preference
        1, // 21 stereo_video_info
        0, // 22 post_filter_hint
        0, // 23 tone_mapping_info
        0, // 24 scalability_info
        0, // 25 sub_pic_scalable_layer
        0, // 26 non_required_layer_rep
        0, // 27 priority_layer_info
        0, // 28 layers_not_present
        0, // 29 layer_dependency_change
        0, // 30 scalable_nesting
        0, // 31 base_layer_temporal_hrd
        0, // 32 quality_layer_integrity_check
        0, // 33 redundant_pic_property
        0, // 34 tl0_dep_rep_index
        0, // 35 tl_switching_point
        0, // 36 parallel_decoding_info
        0, // 37 mvc_scalable_nesting
        0, // 38 view_scalability_info
        0, // 39 multiview_scene_info
        0, // 40 multiview_acquisition_info
        0, // 41 non_required_view_component
        0, // 42 view_dependency_change
        0, // 43 operation_points_not_present
        0, // 44 base_view_temporal_hrd
        1, // 45 frame_packing_arrangemen
};

#define PAK_SUPPORTED_MAX_PAYLOADS    (sizeof(PAK_SUPPORTED_SEI)/sizeof(PAK_SUPPORTED_SEI[0]))

static const mfxU8 SEI_STARTCODE[5] = { 0, 0, 0, 1, 6 };

#define SEI_STARTCODE_SIZE            (sizeof(SEI_STARTCODE))
#define SEI_TRAILING_SIZE             1
#define SEI_BUFFER_RESERVED_SIZE      (4 * 1024)
#define SEI_MAX_SIZE                  (1 * 1024 * 1024)

#define GET_SIZE_IN_BYTES(bits) (((bits) + 7) / 8)
#define IS_BYTE_ALIGNED(bits) (((bits) & 0x07) == 0)

bool bEnc_PAK(mfxVideoParam *par)
{
    MFX_CHECK(par, false);

    mfxExtFeiParam *pControl = NULL;

    for (mfxU16 i = 0; i < par->NumExtParam; ++i)
    {
        if (par->ExtParam[i] != 0 && par->ExtParam[i]->BufferId == MFX_EXTBUFF_FEI_PARAM){
            pControl = reinterpret_cast<mfxExtFeiParam *>(par->ExtParam[i]);
            break;
        }
    }

    return pControl ? (pControl->Func == MFX_FEI_FUNCTION_PAK) : false;
}


static mfxStatus AsyncRoutine(void * state, void * param, mfxU32, mfxU32)
{
    MFX_CHECK_NULL_PTR1(state);

    VideoPAK_PAK & impl = *(VideoPAK_PAK *)state;
    return  impl.RunFramePAK(NULL, NULL);
}

mfxStatus VideoPAK_PAK::RunFramePAK(mfxPAKInput *in, mfxPAKOutput *out)
{
    mdprintf(stderr, "VideoPAK_PAK::RunFramePAK\n");

    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VideoPAK_PAK::RunFramePAK");

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
#if MFX_VERSION >= 1023
        PrepareSeiMessageBuffer(m_video, task, task.m_fid[f]);
#endif // MFX_VERSION >= 1023
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

    VideoPAK_PAK & impl = *(VideoPAK_PAK *)state;
    DdiTask      & task = *(DdiTask *)param;

    return impl.QueryStatus(task);
}

mfxStatus VideoPAK_PAK::QueryStatus(DdiTask& task)
{
    mdprintf(stderr,"VideoPAK_PAK::QueryStatus\n");
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

    mfxPAKInput  *input  = reinterpret_cast<mfxPAKInput  *>(task.m_userData[0]);
    mfxPAKOutput *output = reinterpret_cast<mfxPAKOutput *>(task.m_userData[1]);

    m_core->DecreaseReference(&input->InSurface->Data);
    m_core->DecreaseReference(&output->OutSurface->Data);

    UMC::AutomaticUMCMutex guard(m_listMutex);
    //move that task to free tasks from m_incoming
    //m_incoming
    std::list<DdiTask>::iterator it = std::find(m_incoming.begin(), m_incoming.end(), task);
    MFX_CHECK(it != m_incoming.end(), MFX_ERR_NOT_FOUND);

    m_free.splice(m_free.end(), m_incoming, it);

    ReleaseResource(m_rec, task.m_midRec);

    return MFX_ERR_NONE;
}

mfxStatus VideoPAK_PAK::Query(VideoCORE *core, mfxVideoParam *in, mfxVideoParam *out)
{
    mdprintf(stderr, "VideoPAK_PAK::Query\n");

    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VideoPAK_PAK::Query");

    return MFX_ERR_NONE;
}

mfxStatus VideoPAK_PAK::QueryIOSurf(VideoCORE *core , mfxVideoParam *par, mfxFrameAllocRequest request[2])
{
    MFX_CHECK_NULL_PTR3(core, par, request);

    mfxU32 inPattern = par->IOPattern & MFX_IOPATTERN_IN_MASK;
    MFX_CHECK(
        inPattern == MFX_IOPATTERN_IN_VIDEO_MEMORY,
        MFX_ERR_INVALID_VIDEO_PARAM);

    MFX_CHECK(
        par->AsyncDepth == 0 ||
        par->AsyncDepth == 1,
        MFX_ERR_INVALID_VIDEO_PARAM);

    MFX_ENCODE_CAPS hwCaps {};
    mfxStatus sts = QueryHwCaps(core, hwCaps, par);
    MFX_CHECK_STS(sts);

    MfxVideoParam tmp(*par);
    // call MfxHwH264Encode::CheckVideoParam to modify paramters(such like GopRefDist,
    // NumRefFrame) if necessary, otherwise, need to copy checks from that function
    mfxStatus checkStatus = MfxHwH264Encode::CheckVideoParam(tmp,
                                                             hwCaps,
                                                             core->IsExternalFrameAllocator(),
                                                             core->GetHWType(),
                                                             core->GetVAType());
    MFX_CHECK(checkStatus >= MFX_ERR_NONE, checkStatus);

    // more check if needed

    request[0].Type                = MFX_MEMTYPE_FROM_PAK |
                                     MFX_MEMTYPE_DXVA2_DECODER_TARGET |
                                     MFX_MEMTYPE_EXTERNAL_FRAME;
    request[0].NumFrameMin         = tmp.mfx.GopRefDist + (tmp.AsyncDepth-1);
    request[0].NumFrameSuggested   = request[0].NumFrameMin;
    request[0].Info                = tmp.mfx.FrameInfo;

    request[1].Type                = MFX_MEMTYPE_FROM_PAK |
                                     MFX_MEMTYPE_DXVA2_DECODER_TARGET |
                                     MFX_MEMTYPE_INTERNAL_FRAME;
    request[1].NumFrameMin         = tmp.AsyncDepth + tmp.mfx.NumRefFrame;
    request[1].NumFrameSuggested   = request[1].NumFrameMin;
    request[1].Info                = tmp.mfx.FrameInfo;

    return MFX_ERR_NONE;
}

VideoPAK_PAK::VideoPAK_PAK(VideoCORE *core,  mfxStatus * sts)
    : m_bInit(false)
    , m_core(core)
    , m_caps()
    , m_video()
    , m_videoInit()
    , m_prevTask()
    , m_inputFrameType(0)
    , m_currentPlatform(MFX_HW_UNKNOWN)
    , m_currentVaType(MFX_HW_NO)
    , m_bSingleFieldMode(false)
    , m_firstFieldDone(0)
{
    *sts = MFX_ERR_NONE;
}

VideoPAK_PAK::~VideoPAK_PAK()
{
    Close();
}

mfxStatus VideoPAK_PAK::Init(mfxVideoParam *par)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VideoPAK_PAK::Init");

    MFX_CHECK_NULL_PTR1(par);

    mfxStatus sts = MfxEncPAK::CheckExtBufferId(*par);
    MFX_CHECK_STS(sts);

    m_video = *par;
    //add ext buffers from par to m_video

    MfxVideoParam tmp(*par);

    sts = CheckWidthAndHeight(tmp);
    MFX_CHECK_STS(sts);

    m_ddi.reset(new VAAPIFEIPAKEncoder);

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

    /* The entire recon surface pool should be passed to vaContexCreat() finction on Init() stage.
     * And only these surfaces should be passed to driver within Execute() call.
     * The size of recon pool is limited to 127 surfaces. */
    request.Type              = MFX_MEMTYPE_FROM_PAK | MFX_MEMTYPE_DXVA2_DECODER_TARGET | MFX_MEMTYPE_INTERNAL_FRAME;
    request.NumFrameMin       = m_video.AsyncDepth + m_video.mfx.NumRefFrame;
    request.NumFrameSuggested = request.NumFrameMin;
    request.AllocId           = par->AllocId;
    MFX_CHECK(request.NumFrameMin <= 127, MFX_ERR_UNSUPPORTED);

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

mfxStatus VideoPAK_PAK::Reset(mfxVideoParam *par)
{
    MFX_CHECK_NULL_PTR1(par);

    mfxStatus sts = MfxEncPAK::CheckExtBufferId(*par);
    MFX_CHECK_STS(sts);

    MfxVideoParam newPar = *par;

    bool isIdrRequired = false;
    mfxStatus checkStatus = ProcessAndCheckNewParameters(newPar, isIdrRequired, par);
    MFX_CHECK(checkStatus >= MFX_ERR_NONE, checkStatus);

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

mfxStatus VideoPAK_PAK::GetVideoParam(mfxVideoParam *par)
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

mfxStatus VideoPAK_PAK::CheckPAKPayloads(const mfxPayload * const * payload,mfxU16 numPayload)
{
    mfxU32 SEISize = 0;

    //check payloads are supported in the FEI lib
    for (mfxU16 i = 0; i< numPayload; ++i)
    {
        //check only present payloads
        if (payload[i] && payload[i]->NumBit > 0)
        {
            //check if byte-aligned
            MFX_CHECK(
                IS_BYTE_ALIGNED(payload[i]->NumBit),
                MFX_ERR_UNDEFINED_BEHAVIOR);

            //check Data pointer
            MFX_CHECK(
                payload[i]->Data,
                MFX_ERR_UNDEFINED_BEHAVIOR);

            //check buffer size
            MFX_CHECK(
                GET_SIZE_IN_BYTES(payload[i]->NumBit) <= payload[i]->BufSize,
                MFX_ERR_UNDEFINED_BEHAVIOR);

            //SEI messages type constraints
            MFX_CHECK(
                payload[i]->Type < PAK_SUPPORTED_MAX_PAYLOADS &&
                PAK_SUPPORTED_SEI[payload[i]->Type] == 1,
                MFX_ERR_UNDEFINED_BEHAVIOR);

            SEISize += GET_SIZE_IN_BYTES(payload[i]->NumBit);
        }
    }

    //Check payload buffer size
    MFX_CHECK(SEISize <= SEI_MAX_SIZE, MFX_ERR_UNDEFINED_BEHAVIOR);
    return MFX_ERR_NONE;
}

mfxU32 VideoPAK_PAK::GetPAKPayloadSize(
    MfxVideoParam const & video,
    const mfxPayload* const* payload,
    mfxU16 numPayload)
{
    mfxExtCodingOption const *   extOpt = GetExtBuffer(video);
    mfxU32 BufferSz = 0;

    for (mfxU16 i = 0; i < numPayload; i++)
    {
        BufferSz += GET_SIZE_IN_BYTES(payload[i]->NumBit);
    }

    if (IsOn(extOpt->SingleSeiNalUnit))
    {
        BufferSz += (SEI_STARTCODE_SIZE + SEI_TRAILING_SIZE);
    }
    else
    {
        BufferSz += (SEI_STARTCODE_SIZE + SEI_TRAILING_SIZE) * numPayload;
    }

    return BufferSz;
}

#if MFX_VERSION >= 1023
void VideoPAK_PAK::PrepareSeiMessageBuffer(
    MfxVideoParam const & video,
    DdiTask const & task,
    mfxU32 fieldId)
{
    mfxPAKInput const *          input = reinterpret_cast<mfxPAKInput*>(task.m_userData[0]);
    mfxExtCodingOption const *   extOpt = GetExtBuffer(video);

    mfxU32 fieldPicFlag = (task.GetPicStructForEncode() != MFX_PICSTRUCT_PROGRESSIVE);
    mfxU32 secondFieldPicFlag = (task.GetFirstField() != fieldId);

    mfxU32 hasAtLeastOneSei =
        input->Payload && (input->NumPayload > secondFieldPicFlag)
        && (input->Payload[secondFieldPicFlag] != 0);
    if (!hasAtLeastOneSei)
    {
        return;
    }

    mfxU32 BufferSz = GetPAKPayloadSize(video, input->Payload, input->NumPayload);
    if (task.m_addRepackSize[fieldId] && hasAtLeastOneSei)
    {
        BufferSz += task.m_addRepackSize[fieldId];
    }

    //Reserve 4K Bytes for m_sei buffer to improve stability
    BufferSz += SEI_BUFFER_RESERVED_SIZE;

    if (m_sei.Capacity() < BufferSz)
    {
        m_sei.Alloc(BufferSz);
    }

    OutputBitstream writer(m_sei.Buffer(), m_sei.Capacity());

    if (hasAtLeastOneSei && IsOn(extOpt->SingleSeiNalUnit))
    {
        writer.PutRawBytes(SEI_STARTCODE, SEI_STARTCODE + SEI_STARTCODE_SIZE);
    }

    //Add user-defined message
    for (mfxU32 i = secondFieldPicFlag; i< input->NumPayload; i = i + 1 + fieldPicFlag)
    {
        if (input->Payload[i] != 0)
        {
            if (IsOff(extOpt->SingleSeiNalUnit))
            {
                writer.PutRawBytes(SEI_STARTCODE, SEI_STARTCODE + SEI_STARTCODE_SIZE);
            }
            mfxU32 uPayloadSize = GET_SIZE_IN_BYTES(input->Payload[i]->NumBit);
            for (mfxU32 b = 0; b < uPayloadSize; b++)
            {
                writer.PutBits(input->Payload[i]->Data[b], 8);
            }
            if (IsOff(extOpt->SingleSeiNalUnit))
            {
                writer.PutTrailingBits();
            }
        }
    }

    if (hasAtLeastOneSei && IsOn(extOpt->SingleSeiNalUnit))
    {
        writer.PutTrailingBits();
    }

    //Add repack compensation to the end of last sei NALu
    //It's padding done with trailing_zero_8bits. This padding could has greater size than real repack overhead.
    if (task.m_addRepackSize[fieldId] && hasAtLeastOneSei)
    {
        writer.PutFillerBytes(0xff, task.m_addRepackSize[fieldId]);
    }

    m_sei.SetSize(GET_SIZE_IN_BYTES(writer.GetNumBits()));
}
#endif //MFX_VERSION >= 1023

mfxU16 VideoPAK_PAK::GetBufferPeriodPayloadIdx(
    const mfxPayload * const * payload,
    mfxU16 numPayload,
    mfxU32 start,
    mfxU32 step)
{
    mfxU32 i = start;
    for (; i < numPayload; i += step)
    {
        if (payload[i] && payload[i]->NumBit > 0 &&
            payload[i]->Type == 0 /*Buffer Period SEI payload type*/)
        {
            break;
        }
    }

    return i;
}

mfxStatus VideoPAK_PAK::ChangeBufferPeriodPayloadIndxIfNeed(
    mfxPayload** payload,
    mfxU16 numPayload)
{
    mfxU32 fieldMaxCount = m_video.mfx.FrameInfo.PicStruct == MFX_PICSTRUCT_PROGRESSIVE ? 1 : 2;
    mfxStatus mfxSts = MFX_ERR_NONE;

    for (mfxU32 i = 0; i < fieldMaxCount; i++)
    {
        mfxU32 start = i;
        mfxU32 step  = fieldMaxCount;
        mfxU16 index;

        index = GetBufferPeriodPayloadIdx(payload, numPayload, start, step);
        if (index != start && index < numPayload)
        {
            mfxPayload* tmp = payload[index];
            for (mfxU32 j = index; j > start; j -= step)
            {
                MFX_CHECK_WITH_ASSERT((j-step) >= 0, MFX_ERR_INVALID_VIDEO_PARAM);
                payload[j] = payload[j-step];
            }
            payload[start] = tmp;
            mfxSts = MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
        }
    }
    return mfxSts;
}

mfxStatus VideoPAK_PAK::RunFramePAKCheck(
                    mfxPAKInput  *input,
                    mfxPAKOutput *output,
                    MFX_ENTRY_POINT pEntryPoints[],
                    mfxU32 &numEntryPoints)
{
    mdprintf(stderr, "VideoPAK_PAK::RunFramePAKCheck\n");

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

    MFX_CHECK_NULL_PTR1(output->Bs);
    m_free.front().m_bs = output->Bs;

    m_incoming.splice(m_incoming.end(), m_free, m_free.begin());
    DdiTask& task = m_incoming.front();

    task.m_singleFieldMode = m_bSingleFieldMode;
    task.m_picStruct       = GetPicStruct(m_video, task);
    task.m_fieldPicFlag    = task.m_picStruct[ENC] != MFX_PICSTRUCT_PROGRESSIVE;
    task.m_fid[0]          = task.m_picStruct[ENC] == MFX_PICSTRUCT_FIELD_BFF;
    task.m_fid[1]          = task.m_fieldPicFlag - task.m_fid[0];

    //To record returned status that may contain warning status
    mfxStatus mfxSts = MFX_ERR_NONE;

#if MFX_VERSION >= 1023
    // For now PAK doesn't have output extension buffers
    MFX_CHECK(!output->NumExtParam, MFX_ERR_UNDEFINED_BEHAVIOR);

    mfxStatus sts = CheckRuntimeExtBuffers(input, output, m_video, task);
#else
    mfxStatus sts = CheckRuntimeExtBuffers(output, output, m_video, task);
#endif // MFX_VERSION >= 1023
    MFX_CHECK_STS(sts);

#if MFX_VERSION >= 1023
    //Check whether input payload is supported or not
    MFX_CHECK(!CheckPAKPayloads(input->Payload, input->NumPayload), MFX_ERR_INVALID_VIDEO_PARAM);

    //Refer to H.264 spec 7.4.1.2.3, when an SEI NAL unit
    //containing a buffering period SEI message is present,
    //the buffering period SEI message shall be the first SEI
    //message payload of the first SEI NAL unit in the access unit
    mfxSts = ChangeBufferPeriodPayloadIndxIfNeed(input->Payload, input->NumPayload);
    MFX_CHECK(mfxSts >= MFX_ERR_NONE, MFX_ERR_INVALID_VIDEO_PARAM);
#endif // MFX_VERSION >= 1023

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

        // Additionally check PAK specific requirements for extension buffers
        mfxExtFeiEncMV     * mvout     = GetExtBufferFEI(input, idxToPickBuffer);
        mfxExtFeiPakMBCtrl * mbcodeout = GetExtBufferFEI(input, idxToPickBuffer);
        MFX_CHECK(mvout && mbcodeout, MFX_ERR_INVALID_VIDEO_PARAM);

        // Frame type detection
#if MFX_VERSION >= 1023
        mfxExtFeiPPS* extFeiPPSinRuntime = GetExtBufferFEI(input, idxToPickBuffer);
        mfxU8 type = extFeiPPSinRuntime->FrameType;
#else
        mfxExtFeiPPS* extFeiPPSinRuntime = GetExtBufferFEI(output, idxToPickBuffer);
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

        sts = CopyRawSurfaceToVideoMemory(*m_core, m_video, task);
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
        ConfigureTaskFEI(task, m_prevTask, m_video, input, output, m_frameOrder_frameNum);
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

    return mfxSts;
}


mfxStatus VideoPAK_PAK::Close(void)
{
    MFX_CHECK(m_bInit, MFX_ERR_NONE);
    m_bInit = false;
    m_ddi->Destroy();

    m_core->FreeFrames(&m_rec);
    //m_core->FreeFrames(&m_opaqHren);

    return MFX_ERR_NONE;
}


mfxStatus VideoPAK_PAK::ProcessAndCheckNewParameters(
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

#endif  // defined(MFX_VA) && defined(MFX_ENABLE_H264_VIDEO_ENCODE_HW) && defined(MFX_ENABLE_H264_VIDEO_FEI_PAK)
