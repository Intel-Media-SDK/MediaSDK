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

#if defined(MFX_ENABLE_H264_VIDEO_ENCODE_HW) && defined(MFX_VA_LINUX)

#include <va/va.h>
#include <va/va_enc_h264.h>
#include "mfxfei.h"
#include "libmfx_core_vaapi.h"
#include "mfx_h264_encode_vaapi.h"
#include "mfx_h264_encode_hw_utils.h"

#if defined(_DEBUG)
//#define mdprintf fprintf
#define mdprintf(...)
#else
#define mdprintf(...)
#endif

using namespace MfxHwH264Encode;

#if defined(MFX_ENABLE_H264_VIDEO_FEI_PREENC)
#include "mfx_h264_preenc.h"

VAAPIFEIPREENCEncoder::VAAPIFEIPREENCEncoder()
: VAAPIEncoder()
, m_codingFunction(0)
, m_statParamsId(VA_INVALID_ID)
{
} // VAAPIFEIPREENCEncoder::VAAPIFEIPREENCEncoder()

VAAPIFEIPREENCEncoder::~VAAPIFEIPREENCEncoder()
{

    Destroy();

} // VAAPIFEIPREENCEncoder::~VAAPIFEIPREENCEncoder()

mfxStatus VAAPIFEIPREENCEncoder::Destroy()
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "FEI::PreENC::Destroy");

    mfxStatus sts = MFX_ERR_NONE;

    MFX_DESTROY_VABUFFER(m_statParamsId, m_vaDisplay);

    for( mfxU32 i = 0; i < m_statMVId.size(); i++ )
    {
        MFX_DESTROY_VABUFFER(m_statMVId[i], m_vaDisplay);
    }

    for( mfxU32 i = 0; i < m_statOutId.size(); i++ )
    {
        MFX_DESTROY_VABUFFER(m_statOutId[i], m_vaDisplay);
    }

    sts = VAAPIEncoder::Destroy();

    return sts;

} // mfxStatus VAAPIFEIPREENCEncoder::Destroy()


mfxStatus VAAPIFEIPREENCEncoder::CreateAccelerationService(MfxVideoParam const & par)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "FEI::PreENC::CreateAccelerationService");
    m_videoParam = par;
    m_codingFunction = 0; //Unknown function

    const mfxExtFeiParam* params = GetExtBuffer(par);
    if (params)
    {
        if (MFX_FEI_FUNCTION_PREENC == params->Func)
        {
            m_codingFunction = params->Func;
            return CreatePREENCAccelerationService(par);
        }
    }

    return MFX_ERR_INVALID_VIDEO_PARAM;
} // mfxStatus VAAPIFEIPREENCEncoder::CreateAccelerationService(MfxVideoParam const & par)

mfxStatus VAAPIFEIPREENCEncoder::CreatePREENCAccelerationService(MfxVideoParam const & par)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "FEI::PreENC::CreatePREENCAccelerationService");

    MFX_CHECK(m_vaDisplay, MFX_ERR_DEVICE_FAILED);

    mfxI32 numEntrypoints = vaMaxNumEntrypoints(m_vaDisplay);
    MFX_CHECK(numEntrypoints, MFX_ERR_DEVICE_FAILED);

    std::vector<VAEntrypoint> pEntrypoints(numEntrypoints);

    // Find entry point for PreENC
    VAStatus vaSts = vaQueryConfigEntrypoints(
            m_vaDisplay,
            VAProfileNone, //specific for statistic
            Begin(pEntrypoints),
            &numEntrypoints);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    mfxI32 entrypointsIndx = 0;
    for (entrypointsIndx = 0; entrypointsIndx < numEntrypoints; entrypointsIndx++) {
        if (VAEntrypointStats == pEntrypoints[entrypointsIndx]) {
            break;
        }
    }
    MFX_CHECK(entrypointsIndx != numEntrypoints, MFX_ERR_DEVICE_FAILED);

    //check attributes of the configuration
    VAConfigAttrib attrib[2];
    //attrib[0].type = VAConfigAttribRTFormat;
    attrib[0].type = (VAConfigAttribType)VAConfigAttribStats;

    vaSts = vaGetConfigAttributes(m_vaDisplay,
            VAProfileNone,
            (VAEntrypoint)VAEntrypointStats,
            &attrib[0], 1);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    //if ((attrib[0].value & VA_RT_FORMAT_YUV420) == 0)
    //    return MFX_ERR_DEVICE_FAILED;

    //attrib[0].value = VA_RT_FORMAT_YUV420;


    vaSts = vaCreateConfig(
            m_vaDisplay,
            VAProfileNone,
            (VAEntrypoint)VAEntrypointStats,
            &attrib[0],
            1,
            &m_vaConfig); //don't configure stat attribs
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaCreateContext");
        // Encoder create
        vaSts = vaCreateContext(
            m_vaDisplay,
            m_vaConfig,
            m_width,
            m_height,
            VA_PROGRESSIVE,
            NULL,
            0,
            &m_vaContextEncode);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }

    /* Statistics buffers allocate & used always.
     * Buffers allocated once for whole stream processing.
     * Statistics buffer delete at the end of processing, by Close() method
     * Actually this is obligation to attach statistics buffers for I- frame/field
     * For P- frame/field only one MVout buffer maybe attached */

    mfxU32 currNumMbsW = ((m_videoParam.mfx.FrameInfo.Width  + 15) >> 4) << 4;
    mfxU32 currNumMbsH = ((m_videoParam.mfx.FrameInfo.Height + 15) >> 4) << 4;

    /* Interlaced case requires 32 height alignment */
    if (MFX_PICSTRUCT_PROGRESSIVE != m_videoParam.mfx.FrameInfo.PicStruct)
        currNumMbsH = ((m_videoParam.mfx.FrameInfo.Height + 31) >> 5) << 5;

    mfxU32 currNumMbs = (currNumMbsW * currNumMbsH) >> 8;
    mfxU32 currNumMbs_first_buff = currNumMbs;

    switch (m_videoParam.mfx.FrameInfo.PicStruct)
    {
    case MFX_PICSTRUCT_FIELD_TFF:
    case MFX_PICSTRUCT_FIELD_BFF:
        // In TFF/BFF case first buffer holds first field information.
        // In mixed picstructs case first buffer holds entire frame information.
        currNumMbs_first_buff >>= 1;
    case MFX_PICSTRUCT_UNKNOWN:

        m_statOutId.resize(2);
        std::fill(m_statOutId.begin(), m_statOutId.end(), VA_INVALID_ID);

        // buffer for frame/ top field. Again, this buffer for frame of for field
        // this buffer is always frame sized for mixed picstructs case
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                (VABufferType)VAStatsStatisticsBufferType,
                                sizeof(VAStatsStatisticsH264) * currNumMbs_first_buff,
                                1, //limitation from driver, num elements should be 1
                                NULL, //should be mapped later
                                &m_statOutId[0]);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        // buffer for bottom field only
        // this buffer is always half frame sized
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                (VABufferType)VAStatsStatisticsBottomFieldBufferType,
                                sizeof(VAStatsStatisticsH264) * currNumMbs / 2,
                                1, //limitation from driver, num elements should be 1
                                NULL, //should be mapped later
                                &m_statOutId[1]);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        m_statMVId.resize(2);
        std::fill(m_statMVId.begin(), m_statMVId.end(), VA_INVALID_ID);

        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                (VABufferType)VAStatsMVBufferType,
                                //sizeof (VAMotionVector)*mvsOut->NumMBAlloc * 16, //16 MV per MB
                                sizeof(VAMotionVector) * currNumMbs_first_buff * 16,
                                1, //limitation from driver, num elements should be 1
                                NULL, //should be mapped later
                                &m_statMVId[0]);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                (VABufferType)VAStatsMVBufferType,
                                //sizeof (VAMotionVector)*mvsOut->NumMBAlloc * 16, //16 MV per MB
                                sizeof(VAMotionVector) * currNumMbs / 2 * 16,
                                1, //limitation from driver, num elements should be 1
                                NULL, //should be mapped later
                                &m_statMVId[1]);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        break;

    case MFX_PICSTRUCT_PROGRESSIVE:

        m_statOutId.resize(1);
        std::fill(m_statOutId.begin(), m_statOutId.end(), VA_INVALID_ID);

        // buffer for frame
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                (VABufferType)VAStatsStatisticsBufferType,
                                sizeof(VAStatsStatisticsH264) * currNumMbs,
                                1, //limitation from driver, num elements should be 1
                                NULL, //should be mapped later
                                &m_statOutId[0]);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        /* One MV buffer for all */
        m_statMVId.resize(1);
        std::fill(m_statMVId.begin(), m_statMVId.end(), VA_INVALID_ID);

        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                (VABufferType)VAStatsMVBufferType,
                                //sizeof (VAMotionVector)*mvsOut->NumMBAlloc * 16, //16 MV per MB
                                sizeof(VAMotionVector)* currNumMbs * 16,
                                1,
                                NULL, //should be mapped later
                                &m_statMVId[0]);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        break;
    }

    Zero(m_sps);
    Zero(m_pps);
    Zero(m_slice);
    //------------------------------------------------------------------

    FillSps(par, m_sps);

    return MFX_ERR_NONE;
} // mfxStatus VAAPIFEIPREENCEncoder::CreatePREENCAccelerationService(MfxVideoParam const & par)


mfxStatus VAAPIFEIPREENCEncoder::Register(mfxFrameAllocResponse& response, D3DDDIFORMAT type)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "FEI::PreENC::Register");

    std::vector<ExtVASurface> * pQueue = (D3DDDIFMT_INTELENCODE_BITSTREAMDATA == type) ? &m_bsQueue : &m_reconQueue;
    MFX_CHECK(pQueue, MFX_ERR_NULL_PTR);

    mfxStatus sts;

    {
        // we should register allocated HW bitstreams and recon surfaces
        MFX_CHECK(response.mids, MFX_ERR_NULL_PTR);

        ExtVASurface extSurf;
        VASurfaceID *pSurface = NULL;

        for (mfxU32 i = 0; i < response.NumFrameActual; i++) {

            sts = m_core->GetFrameHDL(response.mids[i], (mfxHDL *) & pSurface);
            MFX_CHECK_STS(sts);

            extSurf.number = i;
            extSurf.surface = *pSurface;

            pQueue->push_back(extSurf);
        }
    }

    return MFX_ERR_NONE;

} // mfxStatus VAAPIFEIPREENCEncoder::Register(mfxFrameAllocResponse& response, D3DDDIFORMAT type)

mfxStatus VAAPIFEIPREENCEncoder::Execute(
        mfxHDLPair pair,
        DdiTask const & task,
        mfxU32 fieldId,
        PreAllocatedVector const & sei)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "FEI::PreENC::Execute");
    mdprintf(stderr, "\nVAAPIPREENCEncoder::Execute\n");

    mfxHDL surface = pair.first;
    mfxStatus mfxSts = MFX_ERR_NONE;
    VAStatus  vaSts;
    VAPictureStats past_ref, future_ref;
    VASurfaceID *inputSurface = (VASurfaceID*) surface;

    std::vector<VABufferID> configBuffers(MAX_CONFIG_BUFFERS_COUNT + m_slice.size() * 2);
    std::fill(configBuffers.begin(), configBuffers.end(), VA_INVALID_ID);

    mfxU16 buffersCount = 0;

    // IDs for input buffers
    VABufferID mvPredid = VA_INVALID_ID, qpid = VA_INVALID_ID;

    // IDs for output buffers
    VABufferID statParamsId = VA_INVALID_ID;

    mfxENCInput*  in  = reinterpret_cast<mfxENCInput* >(task.m_userData[0]);
    mfxENCOutput* out = reinterpret_cast<mfxENCOutput*>(task.m_userData[1]);

    mfxU32 feiFieldId = task.m_fid[fieldId];

    //find output buffers
    mfxExtFeiPreEncMV           * mvsOut    = GetExtBufferFEI(out, feiFieldId);
    mfxExtFeiPreEncMBStat       * mbstatOut = GetExtBufferFEI(out, feiFieldId);

    //find input buffers
    mfxExtFeiPreEncCtrl         * feiCtrl   = GetExtBufferFEI(in,  feiFieldId);
    mfxExtFeiEncQP              * feiQP     = GetExtBufferFEI(in,  feiFieldId);
    mfxExtFeiPreEncMVPredictors * feiMVPred = GetExtBufferFEI(in,  feiFieldId);


    VAStatsStatisticsParameterH264 statParams;
    memset(&statParams, 0, sizeof(VAStatsStatisticsParameterH264));

    statParams.adaptive_search           = feiCtrl->AdaptiveSearch;
    statParams.disable_statistics_output = /*(mbstatOut == NULL) ||*/ feiCtrl->DisableStatisticsOutput;
    /* There is a limitation from driver for now:
     * MSDK need to provide stat buffers for I- frame.
     * MSDK always attach statistic buffer for PreEnc right now
     * (But does not copy buffer to user if it is not required)
     * */
    statParams.disable_mv_output         = (mvsOut == NULL) || feiCtrl->DisableMVOutput;
    statParams.mb_qp                     = (feiQP == NULL) && feiCtrl->MBQp;
    statParams.mv_predictor_ctrl         = (feiMVPred != NULL) ? feiCtrl->MVPredictor : 0;
    statParams.frame_qp                  = feiCtrl->Qp;
    statParams.ft_enable                 = feiCtrl->FTEnable;
    statParams.inter_sad                 = feiCtrl->InterSAD;
    statParams.intra_sad                 = feiCtrl->IntraSAD;
    statParams.len_sp                    = feiCtrl->LenSP;
    statParams.search_path               = feiCtrl->SearchPath;
    statParams.sub_pel_mode              = feiCtrl->SubPelMode;
    statParams.sub_mb_part_mask          = feiCtrl->SubMBPartMask;
    statParams.ref_height                = feiCtrl->RefHeight;
    statParams.ref_width                 = feiCtrl->RefWidth;
    statParams.search_window             = feiCtrl->SearchWindow;
    statParams.enable_8x8_statistics     = feiCtrl->Enable8x8Stat;
    statParams.intra_part_mask           = feiCtrl->IntraPartMask;


    VABufferID outBuffers[3] = { VA_INVALID_ID, VA_INVALID_ID, VA_INVALID_ID };
    mfxU32 numOutBufs = 0;

    if ((statParams.mv_predictor_ctrl) && (feiMVPred != NULL) && (feiMVPred->MB != NULL))
    {
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                (VABufferType)VAStatsMVPredictorBufferType,
                                sizeof(VAMotionVector) *feiMVPred->NumMBAlloc,
                                1, //limitation from driver, num elements should be 1
                                feiMVPred->MB,
                                &mvPredid);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        mdprintf(stderr, "MVPred bufId=%d\n", mvPredid);
    }
    statParams.stats_params.mv_predictor = mvPredid;

#if MFX_VERSION >= 1023
    if ((statParams.mb_qp) && (feiQP != NULL) && (feiQP->MB != NULL))
    {
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                (VABufferType)VAEncQPBufferType,
                                sizeof(VAEncQPBufferH264)*feiQP->NumMBAlloc,
                                1, //limitation from driver, num elements should be 1
                                feiQP->MB,
                                &qpid);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        mdprintf(stderr, "Qp bufId=%d\n", qpid);
    }
    statParams.stats_params.qp = qpid;
#else
    if ((statParams.mb_qp) && (feiQP != NULL) && (feiQP->QP != NULL))
    {
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                (VABufferType)VAEncQPBufferType,
                                sizeof (VAEncQPBufferH264)*feiQP->NumQPAlloc,
                                1, //limitation from driver, num elements should be 1
                                feiQP->QP,
                                &qpid);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        mdprintf(stderr, "Qp bufId=%d\n", qpid);
    }
    statParams.stats_params.qp = qpid;
#endif

    /* PreEnc support only 1 forward and 1 backward reference */

    //currently only video memory is used, all input surfaces should be in video memory
    statParams.stats_params.num_past_references = 0;
    statParams.stats_params.past_references     = NULL;
    statParams.stats_params.past_ref_stat_buf   = NULL;

    if (feiCtrl->RefFrame[0])
    {
        statParams.stats_params.num_past_references = 1;

        mfxHDL handle;
        mfxSts = m_core->GetExternalFrameHDL(feiCtrl->RefFrame[0]->Data.MemId, &handle);
        MFX_CHECK_STS(mfxSts);

        VAPictureStats* l0surfs = &past_ref;
        l0surfs->picture_id = *(VASurfaceID*)handle;

        switch (feiCtrl->RefPictureType[0])
        {
        case MFX_PICTYPE_TOPFIELD:
            l0surfs->flags = VA_PICTURE_STATS_TOP_FIELD;
            break;
        case MFX_PICTYPE_BOTTOMFIELD:
            l0surfs->flags = VA_PICTURE_STATS_BOTTOM_FIELD;
            break;
        case MFX_PICTYPE_FRAME:
            l0surfs->flags = VA_PICTURE_STATS_PROGRESSIVE;
            break;
        }

        if (IsOn(feiCtrl->DownsampleReference[0]))
            l0surfs->flags |= VA_PICTURE_STATS_CONTENT_UPDATED;

        statParams.stats_params.past_references = l0surfs;
        // statParams.stats_params.past_ref_stat_buf = IsOn(feiCtrl->DownsampleReference[0]) ? &m_statOutId[surfPastIndexInList] : NULL;
    }

    statParams.stats_params.num_future_references = 0;
    statParams.stats_params.future_references     = NULL;
    statParams.stats_params.future_ref_stat_buf   = NULL;

    if (feiCtrl->RefFrame[1])
    {
        statParams.stats_params.num_future_references = 1;

        mfxHDL handle;
        mfxSts = m_core->GetExternalFrameHDL(feiCtrl->RefFrame[1]->Data.MemId, &handle);
        MFX_CHECK_STS(mfxSts);

        VAPictureStats* l1surfs = &future_ref;
        l1surfs->picture_id = *(VASurfaceID*)handle;

        switch (feiCtrl->RefPictureType[1])
        {
        case MFX_PICTYPE_TOPFIELD:
            l1surfs->flags = VA_PICTURE_STATS_TOP_FIELD;
            break;
        case MFX_PICTYPE_BOTTOMFIELD:
            l1surfs->flags = VA_PICTURE_STATS_BOTTOM_FIELD;
            break;
        case MFX_PICTYPE_FRAME:
            l1surfs->flags = VA_PICTURE_STATS_PROGRESSIVE;
            break;
        }

        if (IsOn(feiCtrl->DownsampleReference[1]))
            l1surfs->flags |= VA_PICTURE_STATS_CONTENT_UPDATED;

        statParams.stats_params.future_references = l1surfs;
        // statParams.stats_params.future_ref_stat_buf = IsOn(feiCtrl->DownsampleReference[1]) ? &m_statOutId[surfFutureIndexInList] : NULL;
    }

    if ((0 == statParams.stats_params.num_past_references) && (0 == statParams.stats_params.num_future_references))
    {
        statParams.disable_mv_output = 1;
    }

    if (!statParams.disable_mv_output)
    {
        outBuffers[numOutBufs++] = m_statMVId[feiFieldId];

        mdprintf(stderr, "MV bufId=%d\n", m_statMVId[feiFieldId]);
    }
    /*Actually this is obligation to attach statistics buffers for I- frame/field
    * For P- frame/field only one MVout buffer maybe attached
    *  */
    if (!statParams.disable_statistics_output)
    {
        /* Important!
         * We always have queue: {frame/top field, bottom buffer} ...!
         * So, attach buffers accordingly */
        outBuffers[numOutBufs++]      = m_statOutId[0];
        configBuffers[buffersCount++] = m_statOutId[0];

        mdprintf(stderr, "m_statOutId[%u]=%d\n", 0, m_statOutId[0]);

        /*In interlaced case we always need to attached both buffer, for first and for second field */
        if (MFX_PICSTRUCT_PROGRESSIVE != task.GetPicStructForEncode())
        {
            outBuffers[numOutBufs++]      = m_statOutId[1];
            configBuffers[buffersCount++] = m_statOutId[1];

            mdprintf(stderr, "m_statOutId[%u]=%d\n", 1, m_statOutId[1]);
        }
    } // if (!statParams.disable_statistics_output)

    statParams.stats_params.input.picture_id = *inputSurface;
    /*
     * feiCtrl->PictureType - value from user in runtime
     * task.m_fieldPicFlag - actually value from mfxVideoParams from Init()
     * And this values should be matched
     * */

    switch (feiCtrl->PictureType)
    {
    case MFX_PICTYPE_TOPFIELD:
        statParams.stats_params.input.flags = VA_PICTURE_STATS_TOP_FIELD;
        break;

    case MFX_PICTYPE_BOTTOMFIELD:
        statParams.stats_params.input.flags = VA_PICTURE_STATS_BOTTOM_FIELD;
        break;

    case MFX_PICTYPE_FRAME:
        statParams.stats_params.input.flags = VA_PICTURE_STATS_PROGRESSIVE;
        break;
    }

    if (!IsOff(feiCtrl->DownsampleInput) && (0 == feiFieldId))
        statParams.stats_params.input.flags |= VA_PICTURE_STATS_CONTENT_UPDATED;

    /* Link output VA buffers */
    statParams.stats_params.outputs = &outBuffers[0]; //bufIDs for outputs

    //MFX_DESTROY_VABUFFER(statParamsId, m_vaDisplay);
    vaSts = vaCreateBuffer(m_vaDisplay,
                            m_vaContextEncode,
                            (VABufferType)VAStatsStatisticsParameterBufferType,
                            sizeof (statParams),
                            1,
                            &statParams,
                            &statParamsId);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    mdprintf(stderr, "statParamsId=%d\n", statParamsId);
    configBuffers[buffersCount++] = statParamsId;


    assert(buffersCount <= configBuffers.size());

    //------------------------------------------------------------------
    // Rendering
    //------------------------------------------------------------------
    MFX_LTRACE_2(MFX_TRACE_LEVEL_HOTSPOTS, "A|PREENC|AVC|PACKET_START|", "%d|%d", m_vaContextEncode, task.m_frameNum);
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaBeginPicture");
        vaSts = vaBeginPicture(
                m_vaDisplay,
                m_vaContextEncode,
                *inputSurface);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaRenderPicture");

        vaSts = vaRenderPicture(
                m_vaDisplay,
                m_vaContextEncode,
                &configBuffers[0], /* vector store leaner in memory*/
                buffersCount);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaEndPicture");
        vaSts = vaEndPicture(m_vaDisplay, m_vaContextEncode);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }
    MFX_LTRACE_2(MFX_TRACE_LEVEL_HOTSPOTS, "A|PREENC|AVC|PACKET_END|", "%d|%d", m_vaContextEncode, task.m_frameNum);
    //------------------------------------------------------------------
    // PostStage
    //------------------------------------------------------------------
    // put to cache
    {
        UMC::AutomaticUMCMutex guard(m_guard);

        ExtVASurface currentFeedback;
        currentFeedback.number  = task.m_statusReportNumber[feiFieldId];
        currentFeedback.surface = *inputSurface;
        currentFeedback.mv      = (statParams.disable_mv_output == 0) ? m_statMVId[feiFieldId] : VA_INVALID_ID;

        /* we have following order in statistic buffers list in interlaced case:
         * {top bot}, {top bot}, {top bot}
         *  So, we have to correct behavior for BFF case
         * */
        /* use fieldId instead of feiFieldId to guarantee that top fields buffer goes first in list */
        currentFeedback.mbstat = m_statOutId[fieldId];

        m_statFeedbackCache.push_back(currentFeedback);
    }

    MFX_DESTROY_VABUFFER(mvPredid,     m_vaDisplay);
    MFX_DESTROY_VABUFFER(statParamsId, m_vaDisplay);
    MFX_DESTROY_VABUFFER(qpid,         m_vaDisplay);

    mdprintf(stderr, "submit_vaapi done: %d\n", task.m_frameOrder);
    return MFX_ERR_NONE;

} // mfxStatus VAAPIFEIPREENCEncoder::Execute( mfxHDL surface, DdiTask const & task, mfxU32 fieldId, PreAllocatedVector const & sei)

mfxStatus VAAPIFEIPREENCEncoder::QueryStatus(
        DdiTask & task,
        mfxU32 fieldId)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "FEI::PreENC::QueryStatus");
    VAStatus vaSts;
    mfxStatus sts = MFX_ERR_NONE;
    mdprintf(stderr, "VAAPIPREENCEncoder::QueryStatus\n");

    mdprintf(stderr, "query_vaapi status: %d\n", task.m_frameOrder);
    //------------------------------------------
    // (1) mapping feedbackNumber -> surface & bs
    VASurfaceID waitSurface = VA_INVALID_SURFACE;
    VABufferID statMVid     = VA_INVALID_ID;
    VABufferID statOUTid    = VA_INVALID_ID;

    mfxU32 indxSurf, feiFieldId = task.m_fid[fieldId];

    UMC::AutomaticUMCMutex guard(m_guard);

    for (indxSurf = 0; indxSurf < m_statFeedbackCache.size(); indxSurf++)
    {
        const ExtVASurface & currentFeedback = m_statFeedbackCache[indxSurf];

        if (currentFeedback.number == task.m_statusReportNumber[feiFieldId])
        {
            waitSurface = currentFeedback.surface;
            statMVid    = currentFeedback.mv;
            statOUTid   = currentFeedback.mbstat;
            break;
        }
    }
    MFX_CHECK(indxSurf != m_statFeedbackCache.size(), MFX_ERR_UNKNOWN);

    VASurfaceStatus surfSts = VASurfaceSkipped;

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaSyncSurface");
        vaSts = vaSyncSurface(m_vaDisplay, waitSurface);
    }
    // following code is workaround:
    // because of driver bug it could happen that decoding error will not be returned after decoder sync
    // and will be returned at subsequent encoder sync instead
    // just ignore VA_STATUS_ERROR_DECODING_ERROR in encoder
    if (vaSts == VA_STATUS_ERROR_DECODING_ERROR)
        vaSts = VA_STATUS_SUCCESS;
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaQuerySurfaceStatus");
        vaSts = vaQuerySurfaceStatus(m_vaDisplay, waitSurface, &surfSts);
    }
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    mdprintf(stderr, "sync on surface: %d\n", surfSts);

    switch (surfSts)
    {
        default: //for now driver does not return correct status
        case VASurfaceReady:
        {
            VAMotionVector* mvs;
            VAStatsStatisticsH264* mbstat;

            mfxENCInput*  in  = reinterpret_cast<mfxENCInput* >(task.m_userData[0]);
            mfxENCOutput* out = reinterpret_cast<mfxENCOutput*>(task.m_userData[1]);

            //find control buffer
            mfxExtFeiPreEncCtrl*   feiCtrl   = GetExtBufferFEI(in,  feiFieldId);
            //find output surfaces
            mfxExtFeiPreEncMV*     mvsOut    = GetExtBufferFEI(out, feiFieldId);
            mfxExtFeiPreEncMBStat* mbstatOut = GetExtBufferFEI(out, feiFieldId);

            if (!feiCtrl->DisableMVOutput && mvsOut && (VA_INVALID_ID != statMVid))
            {
                // Copy back MVs

                MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "MV");
                {
                    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaMapBuffer");
                    vaSts = vaMapBuffer(
                            m_vaDisplay,
                            statMVid,
                            (void **) (&mvs));
                }

                if(VA_STATUS_ERROR_ENCODING_ERROR == vaSts)
                {
                    sts = MFX_ERR_GPU_HANG;
                    break;
                }
                else
                {
                    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
                }

                FastCopyBufferVid2Sys(mvsOut->MB, mvs, 16 * sizeof(VAMotionVector) * mvsOut->NumMBAlloc);

                {
                    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
                    vaSts = vaUnmapBuffer(m_vaDisplay, statMVid);
                }
                MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
            }

            if (!feiCtrl->DisableStatisticsOutput && mbstatOut && VA_INVALID_ID != statOUTid)
            {
                // Copy back statistics

                MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "MBstat");
                {
                    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaMapBuffer");
                    vaSts = vaMapBuffer(
                            m_vaDisplay,
                            statOUTid,
                            (void **) (&mbstat));
                }

                if(VA_STATUS_ERROR_ENCODING_ERROR == vaSts)
                {
                    sts = MFX_ERR_GPU_HANG;
                    break;
                }
                else
                {
                    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
                }

                FastCopyBufferVid2Sys(mbstatOut->MB, mbstat, sizeof(VAStatsStatisticsH264) * mbstatOut->NumMBAlloc);

                {
                    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
                    vaSts = vaUnmapBuffer(m_vaDisplay, statOUTid);
                }
                MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
            }

            // remove task
            m_statFeedbackCache.erase(m_statFeedbackCache.begin() + indxSurf);
        }
            sts =  MFX_ERR_NONE;
            break;
        case VASurfaceRendering:
        case VASurfaceDisplaying:
            sts = MFX_WRN_DEVICE_BUSY;
            break;

        case VASurfaceSkipped:
            //default:
            assert(!"bad feedback status");
            sts =  MFX_ERR_DEVICE_FAILED;
            //return MFX_ERR_NONE;
            break;
    }

    mdprintf(stderr, "query_vaapi done\n");
    return sts;

} // mfxStatus VAAPIFEIPREENCEncoder::QueryStatus(DdiTask & task, mfxU32 fieldId)
#endif

#if defined(MFX_ENABLE_H264_VIDEO_FEI_ENC)

VAAPIFEIENCEncoder::VAAPIFEIENCEncoder()
: VAAPIEncoder()
, m_codingFunction(MFX_FEI_FUNCTION_ENC)
, m_statParamsId(VA_INVALID_ID)
, m_statMVId(VA_INVALID_ID)
, m_statOutId(VA_INVALID_ID)
, m_codedBufferId(VA_INVALID_ID){
} // VAAPIFEIENCEncoder::VAAPIFEIENCEncoder()

VAAPIFEIENCEncoder::~VAAPIFEIENCEncoder()
{

    Destroy();

} // VAAPIFEIENCEncoder::~VAAPIFEIENCEncoder()

mfxStatus VAAPIFEIENCEncoder::Destroy()
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "FEI::ENC::Destroy");

    mfxStatus sts = MFX_ERR_NONE;

    MFX_DESTROY_VABUFFER(m_statParamsId,  m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_statMVId,      m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_statOutId,     m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_codedBufferId, m_vaDisplay);

    sts = VAAPIEncoder::Destroy();

    return sts;

} // VAAPIFEIENCEncoder::Destroy()

mfxStatus VAAPIFEIENCEncoder::Reset(MfxVideoParam const & par)
{
    m_videoParam = par;

    FillSps(par, m_sps);

    MFX_CHECK_WITH_ASSERT(MFX_ERR_NONE == SetHRD(par, m_vaDisplay, m_vaContextEncode, m_hrdBufferId), MFX_ERR_DEVICE_FAILED);
    MFX_CHECK_WITH_ASSERT(MFX_ERR_NONE == SetFrameRate(par, m_vaDisplay, m_vaContextEncode, m_frameRateId), MFX_ERR_DEVICE_FAILED);

    FillConstPartOfPps(par, m_pps);

    if (m_caps.HeaderInsertion == 0)
        m_headerPacker.Init(par, m_caps);

    return MFX_ERR_NONE;

}


mfxStatus VAAPIFEIENCEncoder::CreateAccelerationService(MfxVideoParam const & par)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "FEI::ENC::CreateAccelerationService");
    m_videoParam = par;

    //check for ext buffer for FEI
    m_codingFunction = 0; //Unknown function

    const mfxExtFeiParam* params = GetExtBuffer(par);
    if (params)
    {
        if (MFX_FEI_FUNCTION_ENC == params->Func)
        {
            m_codingFunction = params->Func;
            return CreateENCAccelerationService(par);
        }
    }

    return MFX_ERR_INVALID_VIDEO_PARAM;
} // mfxStatus VAAPIFEIENCEncoder::CreateAccelerationService(MfxVideoParam const & par)

mfxStatus VAAPIFEIENCEncoder::CreateENCAccelerationService(MfxVideoParam const & par)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "FEI::ENC::CreateENCAccelerationService");
    MFX_CHECK(m_vaDisplay, MFX_ERR_DEVICE_FAILED);

    VAStatus vaSts = VA_STATUS_SUCCESS;
    VAProfile profile = ConvertProfileTypeMFX2VAAPI(m_videoParam.mfx.CodecProfile);
    //mfxU8 vaRCType = ConvertRateControlMFX2VAAPI(par.mfx.RateControlMethod);
    /* Working only with RC_CPQ */
    //vaRCType = VA_RC_CQP;
    VAEntrypoint entrypoints[5];
    int num_entrypoints, slice_entrypoint;
    VAConfigAttrib attrib[4];

    vaSts = vaQueryConfigEntrypoints(m_vaDisplay, profile, entrypoints, &num_entrypoints);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    for (slice_entrypoint = 0; slice_entrypoint < num_entrypoints; ++slice_entrypoint)
    {
        if (entrypoints[slice_entrypoint] == VAEntrypointFEI)
            break;
    }
    /* not find VAEntrypointFEI entry point */
    MFX_CHECK(slice_entrypoint != num_entrypoints, MFX_ERR_DEVICE_FAILED);

    /* find out the format for the render target, and rate control mode */
    attrib[0].type = VAConfigAttribRTFormat;
    attrib[1].type = VAConfigAttribRateControl;
    attrib[2].type = (VAConfigAttribType)VAConfigAttribFEIFunctionType;
    attrib[3].type = (VAConfigAttribType)VAConfigAttribFEIMVPredictors;
    vaSts = vaGetConfigAttributes(m_vaDisplay, profile,
                                    (VAEntrypoint)VAEntrypointFEI, &attrib[0], 4);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    /* not find desired YUV420 RT format */
    MFX_CHECK(attrib[0].value & VA_RT_FORMAT_YUV420, MFX_ERR_DEVICE_FAILED);

    if ((attrib[1].value & VA_RC_CQP) == 0) {
        /* Can't find matched RC mode */
        printf("Can't find the desired RC mode, exit\n");
        return MFX_ERR_DEVICE_FAILED;
    }

    attrib[0].value = VA_RT_FORMAT_YUV420; /* set to desired RT format */
    attrib[1].value = VA_RC_CQP;
    attrib[2].value = VA_FEI_FUNCTION_ENC;
    attrib[3].value = 1; /* set 0 MV Predictor */

    vaSts = vaCreateConfig(m_vaDisplay, profile,
                                (VAEntrypoint)VAEntrypointFEI, &attrib[0], 4, &m_vaConfig);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    std::vector<VASurfaceID> rawSurf;
    rawSurf.reserve(m_reconQueue.size());
    for (mfxU32 i = 0; i < m_reconQueue.size(); ++i)
        rawSurf.push_back(m_reconQueue[i].surface);

    {
        // Encoder create
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaCreateContext");
        vaSts = vaCreateContext(
            m_vaDisplay,
            m_vaConfig,
            m_width,
            m_height,
            VA_PROGRESSIVE,
            Begin(rawSurf),
            rawSurf.size(),
            &m_vaContextEncode);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }

//#endif

    m_slice.resize(par.mfx.NumSlice); // it is enough for encoding
    m_sliceBufferId.resize(par.mfx.NumSlice);
    m_packedSliceHeaderBufferId.resize(par.mfx.NumSlice);
    m_packedSliceBufferId.resize(par.mfx.NumSlice);

    std::fill(m_sliceBufferId.begin(),             m_sliceBufferId.end(),             VA_INVALID_ID);
    std::fill(m_packedSliceHeaderBufferId.begin(), m_packedSliceHeaderBufferId.end(), VA_INVALID_ID);
    std::fill(m_packedSliceBufferId.begin(),       m_packedSliceBufferId.end(),       VA_INVALID_ID);

    // Reserve buffers for each reconstructed surface (one per field)
    mfxU32 n_fields   = 1 + !(m_videoParam.mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_PROGRESSIVE);
    mfxU32 numBuffers = m_reconQueue.size() * n_fields;

    m_vaFeiMBStatId.resize(n_fields);
    m_vaFeiMVOutId.resize(numBuffers);
    m_vaFeiMCODEOutId.resize(numBuffers);

    std::fill(m_vaFeiMBStatId.begin(),   m_vaFeiMBStatId.end(),   VA_INVALID_ID);
    std::fill(m_vaFeiMVOutId.begin(),    m_vaFeiMVOutId.end(),    VA_INVALID_ID);
    std::fill(m_vaFeiMCODEOutId.begin(), m_vaFeiMCODEOutId.end(), VA_INVALID_ID);

    Zero(m_sps);
    Zero(m_pps);
    Zero(m_slice);
    //------------------------------------------------------------------

    FillSps(par, m_sps);
    SetHRD(par, m_vaDisplay, m_vaContextEncode, m_hrdBufferId);
    SetFrameRate(par, m_vaDisplay, m_vaContextEncode, m_frameRateId);
    FillConstPartOfPps(par, m_pps);

    if (m_caps.HeaderInsertion == 0)
        m_headerPacker.Init(par, m_caps);

    return MFX_ERR_NONE;
} // mfxStatus VAAPIFEIENCEncoder::CreateENCAccelerationService(MfxVideoParam const & par)


mfxStatus VAAPIFEIENCEncoder::Register(mfxFrameAllocResponse& response, D3DDDIFORMAT type)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "FEI::ENC::Register");

    std::vector<ExtVASurface> * pQueue = (D3DDDIFMT_INTELENCODE_BITSTREAMDATA == type) ? &m_bsQueue : &m_reconQueue;
    MFX_CHECK(pQueue, MFX_ERR_NULL_PTR);

    mfxStatus sts;

    {
        // we should register allocated HW bitstreams and recon surfaces
        MFX_CHECK(response.mids, MFX_ERR_NULL_PTR);

        ExtVASurface extSurf;
        VASurfaceID *pSurface = NULL;

        for (mfxU32 i = 0; i < response.NumFrameActual; i++) {

            sts = m_core->GetFrameHDL(response.mids[i], (mfxHDL *) & pSurface);
            MFX_CHECK_STS(sts);

            extSurf.number  = i;
            extSurf.surface = *pSurface;

            pQueue->push_back(extSurf);
        }
    }


    return MFX_ERR_NONE;

} // mfxStatus VAAPIFEIENCEncoder::Register(mfxFrameAllocResponse& response, D3DDDIFORMAT type)

mfxStatus VAAPIFEIENCEncoder::Execute(
        mfxHDLPair pair,
        DdiTask const & task,
        mfxU32 fieldId,
        PreAllocatedVector const & sei)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "FEI::ENC::Execute");
    mdprintf(stderr, "VAAPIFEIENCEncoder::Execute\n");

    VAStatus  vaSts  = VA_STATUS_SUCCESS;
    mfxStatus mfxSts = MFX_ERR_NONE;

    VASurfaceID *inputSurface = (VASurfaceID*) (pair.first);

    mfxU32 feiFieldId = task.m_fid[fieldId];

    // Offset to choose correct buffers (use value from Init for correct repositioning during mixed-picstructs encoding)
    mfxU32 idxRecon = task.m_idxRecon * (1 + (m_videoParam.mfx.FrameInfo.PicStruct != MFX_PICSTRUCT_PROGRESSIVE));

    std::vector<VABufferID> configBuffers(MAX_CONFIG_BUFFERS_COUNT + m_slice.size() * 2);
    std::fill(configBuffers.begin(), configBuffers.end(), VA_INVALID_ID);

    mfxU16 buffersCount = 0;

    /* HRD parameter */
    mdprintf(stderr, "m_hrdBufferId=%d\n", m_hrdBufferId);
    configBuffers[buffersCount++] = m_hrdBufferId;

    /* frame rate parameter */
    mdprintf(stderr, "m_frameRateId=%d\n", m_frameRateId);
    configBuffers[buffersCount++] = m_frameRateId;

    mfxENCInput*  in  = reinterpret_cast<mfxENCInput* >(task.m_userData[0]);
    mfxENCOutput* out = reinterpret_cast<mfxENCOutput*>(task.m_userData[1]);

    VABufferID vaFeiFrameControlId = VA_INVALID_ID;
    VABufferID vaFeiMVPredId       = VA_INVALID_ID;
    VABufferID vaFeiMBControlId    = VA_INVALID_ID;
    VABufferID vaFeiMBQPId         = VA_INVALID_ID;

    // In case of single-field processing, only one buffer is attached
    mfxU32 idxToPickBuffer = task.m_singleFieldMode ? 0 : feiFieldId;

    // Input ext buffers
    mfxExtFeiSliceHeader     * pDataSliceHeader = GetExtBufferFEI(in, idxToPickBuffer);
    mfxExtFeiEncMBCtrl       * mbctrl           = GetExtBufferFEI(in, idxToPickBuffer);
    mfxExtFeiEncMVPredictors * mvpred           = GetExtBufferFEI(in, idxToPickBuffer);
    mfxExtFeiEncFrameCtrl    * frameCtrl        = GetExtBufferFEI(in, idxToPickBuffer);
    mfxExtFeiEncQP           * mbqp             = GetExtBufferFEI(in, idxToPickBuffer);
    // Output ext buffers
    mfxExtFeiEncMBStat       * mbstat           = GetExtBufferFEI(out, idxToPickBuffer);
    mfxExtFeiEncMV           * mvout            = GetExtBufferFEI(out, idxToPickBuffer);
    mfxExtFeiPakMBCtrl       * mbcodeout        = GetExtBufferFEI(out, idxToPickBuffer);

    // Update internal MSDK PPS
    mfxExtFeiPPS             * pDataPPS         = GetExtBufferFEI(in, idxToPickBuffer);
    mfxExtPpsHeader          * extPps           = GetExtBuffer(m_videoParam);

    // Force PPS insertion if manual PPS with new parameters provided
    if (task.m_insertPps[fieldId])
    {
        extPps->seqParameterSetId              = pDataPPS->SPSId;
        extPps->picParameterSetId              = pDataPPS->PPSId;

        extPps->picInitQpMinus26               = pDataPPS->PicInitQP - 26;
        extPps->numRefIdxL0DefaultActiveMinus1 = (std::max)(pDataPPS->NumRefIdxL0Active, mfxU16(1)) - 1;
        extPps->numRefIdxL1DefaultActiveMinus1 = (std::max)(pDataPPS->NumRefIdxL1Active, mfxU16(1)) - 1;

        extPps->chromaQpIndexOffset            = pDataPPS->ChromaQPIndexOffset;
        extPps->secondChromaQpIndexOffset      = pDataPPS->SecondChromaQPIndexOffset;
        extPps->transform8x8ModeFlag           = pDataPPS->Transform8x8ModeFlag;
    }

    // Pack headers if required
    if (task.m_insertSps[fieldId] || task.m_insertPps[fieldId])
    {
        m_headerPacker.Init(m_videoParam, m_caps);
    }

    VAEncPackedHeaderParameterBuffer packed_header_param_buffer;

    // SPS
    if (task.m_insertSps[fieldId])
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "SPS");
        std::vector<ENCODE_PACKEDHEADER_DATA> const & packedSpsArray = m_headerPacker.GetSps();
        ENCODE_PACKEDHEADER_DATA const & packedSps = packedSpsArray[0];

        packed_header_param_buffer.type                = VAEncPackedHeaderSequence;
        packed_header_param_buffer.has_emulation_bytes = !packedSps.SkipEmulationByteCount;
        packed_header_param_buffer.bit_length          = packedSps.DataLength*8;

        MFX_DESTROY_VABUFFER(m_packedSpsHeaderBufferId, m_vaDisplay);
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                VAEncPackedHeaderParameterBufferType,
                                sizeof(packed_header_param_buffer),
                                1,
                                &packed_header_param_buffer,
                                &m_packedSpsHeaderBufferId);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        MFX_DESTROY_VABUFFER(m_packedSpsBufferId, m_vaDisplay);
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                VAEncPackedHeaderDataBufferType,
                                packedSps.DataLength, 1, packedSps.pData,
                                &m_packedSpsBufferId);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        //packedBufferIndexes.push_back(buffersCount);
        //packedDataSize += packed_header_param_buffer.bit_length;
        configBuffers[buffersCount++] = m_packedSpsHeaderBufferId;
        configBuffers[buffersCount++] = m_packedSpsBufferId;
        /* sequence parameter set */
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                VAEncSequenceParameterBufferType,
                                sizeof(m_sps),1,
                                &m_sps,
                                &m_spsBufferId);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        mdprintf(stderr, "m_spsBufferId=%d\n", m_spsBufferId);
        configBuffers[buffersCount++] = m_spsBufferId;
    }

    if (frameCtrl != NULL && frameCtrl->MVPredictor && mvpred != NULL)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaCreateBuffer (MVP)");
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                (VABufferType)VAEncFEIMVPredictorBufferType,
                                sizeof(VAEncFEIMVPredictorH264)*mvpred->NumMBAlloc,
                                1, //limitation from driver, num elements should be 1
                                mvpred->MB,
                                &vaFeiMVPredId);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        mdprintf(stderr, "vaFeiMVPredId=%d\n", vaFeiMVPredId);
    }

    if (frameCtrl != NULL && frameCtrl->PerMBInput && mbctrl != NULL)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaCreateBuffer (MBctrl)");
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                (VABufferType)VAEncFEIMBControlBufferType,
                                sizeof(VAEncFEIMBControlH264)*mbctrl->NumMBAlloc,
                                1,//limitation from driver, num elements should be 1
                                mbctrl->MB,
                                &vaFeiMBControlId);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        mdprintf(stderr, "vaFeiMBControlId=%d\n", vaFeiMBControlId);
    }

    if (frameCtrl != NULL && frameCtrl->PerMBQp && mbqp != NULL)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaCreateBuffer (MBqp)");
#if MFX_VERSION >= 1023
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                (VABufferType)VAEncQPBufferType,
                                sizeof(VAEncQPBufferH264)*mbqp->NumMBAlloc,
                                1, //limitation from driver, num elements should be 1
                                mbqp->MB,
                                &vaFeiMBQPId);
#else
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                (VABufferType)VAEncQPBufferType,
                                sizeof (VAEncQPBufferH264)*mbqp->NumQPAlloc,
                                1, //limitation from driver, num elements should be 1
                                mbqp->QP,
                                &vaFeiMBQPId);
#endif
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        configBuffers[buffersCount++] = vaFeiMBQPId;
        mdprintf(stderr, "vaFeiMBQPId=%d\n", vaFeiMBQPId);
    }

    //output buffer for MB distortions
    if ((NULL != mbstat) && (VA_INVALID_ID == m_vaFeiMBStatId[feiFieldId]))
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaCreateBuffer (MBstat)");
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                (VABufferType)VAEncFEIDistortionBufferType,
                                sizeof(VAEncFEIDistortionH264)*mbstat->NumMBAlloc,
                                1, //limitation from driver, num elements should be 1
                                NULL, //should be mapped later
                                &m_vaFeiMBStatId[feiFieldId]);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        mdprintf(stderr, "MB Stat bufId[%d]=%d\n", feiFieldId, m_vaFeiMBStatId[feiFieldId]);
    }

    //output buffer for MV
    if ((NULL != mvout) && (VA_INVALID_ID == m_vaFeiMVOutId[idxRecon+feiFieldId]))
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaCreateBuffer (MV)");
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                (VABufferType)VAEncFEIMVBufferType,
                                sizeof(VAMotionVector) * 16 * mvout->NumMBAlloc,
                                1, //limitation from driver, num elements should be 1
                                NULL, //should be mapped later
                                &m_vaFeiMVOutId[idxRecon+feiFieldId]);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        mdprintf(stderr, "MV Out bufId[%d]=%d\n", idxRecon+feiFieldId, m_vaFeiMVOutId[idxRecon+feiFieldId]);
    }

    //output buffer for MBCODE (PAK object cmds)
    if ((NULL != mbcodeout) && (VA_INVALID_ID == m_vaFeiMCODEOutId[idxRecon+feiFieldId]))
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaCreateBuffer (MBcode)");
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                (VABufferType)VAEncFEIMBCodeBufferType,
                                sizeof(VAEncFEIMBCodeH264)*mbcodeout->NumMBAlloc,
                                1, //limitation from driver, num elements should be 1
                                NULL, //should be mapped later
                                &m_vaFeiMCODEOutId[idxRecon+feiFieldId]);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        mdprintf(stderr, "MCODE Out bufId[%d]=%d\n", idxRecon+feiFieldId, m_vaFeiMCODEOutId[idxRecon+feiFieldId]);
    }

    // Configure FrameControl buffer
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "FrameCtrl");
        VAEncMiscParameterBuffer *miscParam;
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                VAEncMiscParameterBufferType,
                                sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterFEIFrameControlH264),
                                1,
                                NULL,
                                &vaFeiFrameControlId);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaMapBuffer");
            vaSts = vaMapBuffer(m_vaDisplay,
                vaFeiFrameControlId,
                (void **)&miscParam);
        }
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED)

        miscParam->type = (VAEncMiscParameterType)VAEncMiscParameterTypeFEIFrameControl;
        VAEncMiscParameterFEIFrameControlH264* vaFeiFrameControl = (VAEncMiscParameterFEIFrameControlH264*)miscParam->data;
        memset(vaFeiFrameControl, 0, sizeof(VAEncMiscParameterFEIFrameControlH264)); //check if we need this

        vaFeiFrameControl->function                 = VA_FEI_FUNCTION_ENC;
        vaFeiFrameControl->adaptive_search          = frameCtrl->AdaptiveSearch;
        vaFeiFrameControl->distortion_type          = frameCtrl->DistortionType;
        vaFeiFrameControl->inter_sad                = frameCtrl->InterSAD;
        vaFeiFrameControl->intra_part_mask          = frameCtrl->IntraPartMask;

        vaFeiFrameControl->intra_sad                = frameCtrl->IntraSAD;
        vaFeiFrameControl->len_sp                   = frameCtrl->LenSP;
        vaFeiFrameControl->search_path              = frameCtrl->SearchPath;

        vaFeiFrameControl->distortion               = m_vaFeiMBStatId[feiFieldId];
        vaFeiFrameControl->mv_data                  = m_vaFeiMVOutId[idxRecon + feiFieldId];
        vaFeiFrameControl->mb_code_data             = m_vaFeiMCODEOutId[idxRecon + feiFieldId];
        vaFeiFrameControl->qp                       = vaFeiMBQPId;
        vaFeiFrameControl->mb_ctrl                  = vaFeiMBControlId;
        vaFeiFrameControl->mb_input                 = frameCtrl->PerMBInput;
        vaFeiFrameControl->mb_qp                    = frameCtrl->PerMBQp;
        vaFeiFrameControl->mb_size_ctrl             = frameCtrl->MBSizeCtrl;
        vaFeiFrameControl->multi_pred_l0            = frameCtrl->MultiPredL0;
        vaFeiFrameControl->multi_pred_l1            = frameCtrl->MultiPredL1;
        vaFeiFrameControl->mv_predictor             = vaFeiMVPredId;
        vaFeiFrameControl->mv_predictor_enable      = frameCtrl->MVPredictor;
        vaFeiFrameControl->num_mv_predictors_l0     = frameCtrl->MVPredictor ? frameCtrl->NumMVPredictors[0] : 0;
        vaFeiFrameControl->num_mv_predictors_l1     = frameCtrl->MVPredictor ? frameCtrl->NumMVPredictors[1] : 0;
        vaFeiFrameControl->ref_height               = frameCtrl->RefHeight;
        vaFeiFrameControl->ref_width                = frameCtrl->RefWidth;
        vaFeiFrameControl->repartition_check_enable = frameCtrl->RepartitionCheckEnable;
        vaFeiFrameControl->search_window            = frameCtrl->SearchWindow;
        vaFeiFrameControl->sub_mb_part_mask         = frameCtrl->SubMBPartMask;
        vaFeiFrameControl->sub_pel_mode             = frameCtrl->SubPelMode;

        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
            vaSts = vaUnmapBuffer(m_vaDisplay, vaFeiFrameControlId);  //check for deletions
        }
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        configBuffers[buffersCount++] = vaFeiFrameControlId;
        mdprintf(stderr, "vaFeiFrameControlId=%d\n", vaFeiFrameControlId);
    }

    // Fill PPS
    UpdatePPS(task, fieldId, m_pps, m_reconQueue);

    // Update from FEI PPS
    m_pps.pic_init_qp                             = extPps->picInitQpMinus26 + 26;
    m_pps.num_ref_idx_l0_active_minus1            = extPps->numRefIdxL0DefaultActiveMinus1;
    m_pps.num_ref_idx_l1_active_minus1            = extPps->numRefIdxL1DefaultActiveMinus1;
    m_pps.chroma_qp_index_offset                  = extPps->chromaQpIndexOffset;
    m_pps.second_chroma_qp_index_offset           = extPps->secondChromaQpIndexOffset;
    m_pps.pic_fields.bits.transform_8x8_mode_flag = extPps->transform8x8ModeFlag;

    /* ENC & PAK surfaces management notes:
     * (1): ENC does not generate real reconstruct surface, but driver use surface ID to store
     * additional internal information.
     * (2): PAK generate real reconstruct surface.
     * (3): Generated reconstructed surfaces become references and managed accordingly by application.
     * (4): Library does not manage reference by itself.
     * (5): And of course main rule: ENC (N number call) and PAK (N number call) should have same exactly
     * same reference /reconstruct list !
     * */

    mfxHDL recon_handle;
    mfxSts = m_core->GetExternalFrameHDL(out->OutSurface->Data.MemId, &recon_handle);
    MFX_CHECK(MFX_ERR_NONE == mfxSts, MFX_ERR_INVALID_HANDLE);
    m_pps.CurrPic.picture_id = *(VASurfaceID*) recon_handle; //id in the memory by ptr

    // Driver select progressive / interlaced based on this field
    m_pps.CurrPic.flags = task.m_fieldPicFlag ? (TFIELD == fieldId ? VA_PICTURE_H264_TOP_FIELD : VA_PICTURE_H264_BOTTOM_FIELD) : 0;

    /* Need to allocated coded buffer: this is does not used by ENC actually */
    if (VA_INVALID_ID == m_codedBufferId)
    {
        int width32  = ((m_videoParam.mfx.FrameInfo.Width  + 31) >> 5) << 5;
        int height32 = ((m_videoParam.mfx.FrameInfo.Height + 31) >> 5) << 5;
        int codedbuf_size = static_cast<int>((width32 * height32) * 400LL / (16 * 16)); //from libva spec

        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                VAEncCodedBufferType,
                                codedbuf_size,
                                1,
                                NULL,
                                &m_codedBufferId);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        //configBuffers[buffersCount++] = m_codedBufferId;
        mdprintf(stderr, "m_codedBufferId=%d\n", m_codedBufferId);
    }
    m_pps.coded_buf = m_codedBufferId;

    if ( (m_videoParam.mfx.CodecProfile & 0xff) == MFX_PROFILE_AVC_BASELINE ||
         (m_videoParam.mfx.CodecProfile & 0xff) == MFX_PROFILE_AVC_MAIN     ||
         (frameCtrl->IntraPartMask & 0x2))
    {
        m_pps.pic_fields.bits.transform_8x8_mode_flag = 0;
    }

    vaSts = vaCreateBuffer(m_vaDisplay,
                            m_vaContextEncode,
                            VAEncPictureParameterBufferType,
                            sizeof(m_pps),
                            1,
                            &m_pps,
                            &m_ppsBufferId);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    configBuffers[buffersCount++] = m_ppsBufferId;
    mdprintf(stderr, "m_ppsBufferId=%d\n", m_ppsBufferId);

    if (task.m_insertPps[fieldId])
    {
        // PPS
        std::vector<ENCODE_PACKEDHEADER_DATA> const & packedPpsArray = m_headerPacker.GetPps();
        ENCODE_PACKEDHEADER_DATA const & packedPps = packedPpsArray[0];

        packed_header_param_buffer.type                = VAEncPackedHeaderPicture;
        packed_header_param_buffer.has_emulation_bytes = !packedPps.SkipEmulationByteCount;
        packed_header_param_buffer.bit_length          = packedPps.DataLength*8;

        MFX_DESTROY_VABUFFER(m_packedPpsHeaderBufferId, m_vaDisplay);
        vaSts = vaCreateBuffer(m_vaDisplay,
                m_vaContextEncode,
                VAEncPackedHeaderParameterBufferType,
                sizeof(packed_header_param_buffer),
                1,
                &packed_header_param_buffer,
                &m_packedPpsHeaderBufferId);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        MFX_DESTROY_VABUFFER(m_packedPpsBufferId, m_vaDisplay);
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                VAEncPackedHeaderDataBufferType,
                                packedPps.DataLength, 1, packedPps.pData,
                                &m_packedPpsBufferId);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        //packedBufferIndexes.push_back(buffersCount);
        //packedDataSize += packed_header_param_buffer.bit_length;
        configBuffers[buffersCount++] = m_packedPpsHeaderBufferId;
        configBuffers[buffersCount++] = m_packedPpsBufferId;
    }

    // Fill SliceHeaders
    UpdateSlice(m_caps, task, fieldId, m_sps, m_pps, m_slice, m_videoParam, m_reconQueue);

    for (size_t i = 0; i < m_slice.size(); ++i)
    {
        // Note: legacy use 5,6,7 type values, but for FEI 0,1,2

        m_slice[i].macroblock_address            = pDataSliceHeader->Slice[i].MBAddress;
        m_slice[i].num_macroblocks               = pDataSliceHeader->Slice[i].NumMBs;
        m_slice[i].slice_type                    = pDataSliceHeader->Slice[i].SliceType;
        m_slice[i].idr_pic_id                    = pDataSliceHeader->Slice[i].IdrPicId;
        m_slice[i].cabac_init_idc                = pDataSliceHeader->Slice[i].CabacInitIdc;
        m_slice[i].slice_qp_delta                = pDataSliceHeader->Slice[i].SliceQPDelta;
        m_slice[i].disable_deblocking_filter_idc = pDataSliceHeader->Slice[i].DisableDeblockingFilterIdc;
        m_slice[i].slice_alpha_c0_offset_div2    = pDataSliceHeader->Slice[i].SliceAlphaC0OffsetDiv2;
        m_slice[i].slice_beta_offset_div2        = pDataSliceHeader->Slice[i].SliceBetaOffsetDiv2;
    } // for (size_t i = 0; i < m_slice.size(); ++i)

    mfxU32 prefix_bytes = (task.m_AUStartsFromSlice[fieldId] + 8) * m_headerPacker.isSvcPrefixUsed();

    //Slice headers only
    std::vector<ENCODE_PACKEDHEADER_DATA> const & packedSlices = m_headerPacker.PackSlices(task, fieldId);

    for (size_t i = 0; i < packedSlices.size(); ++i)
    {
        ENCODE_PACKEDHEADER_DATA const & packedSlice = packedSlices[i];

        if (prefix_bytes)
        {
            packed_header_param_buffer.type                = VAEncPackedHeaderRawData;
            packed_header_param_buffer.has_emulation_bytes = 1;
            packed_header_param_buffer.bit_length          = (prefix_bytes * 8);

            vaSts = vaCreateBuffer(m_vaDisplay,
                                    m_vaContextEncode,
                                    VAEncPackedHeaderParameterBufferType,
                                    sizeof(packed_header_param_buffer),
                                    1,
                                    &packed_header_param_buffer,
                                    &m_packedSvcPrefixHeaderBufferId[i]);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

            vaSts = vaCreateBuffer(m_vaDisplay,
                                    m_vaContextEncode,
                                    VAEncPackedHeaderDataBufferType,
                                    prefix_bytes, 1, packedSlice.pData,
                                    &m_packedSvcPrefixBufferId[i]);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

            configBuffers[buffersCount++] = m_packedSvcPrefixHeaderBufferId[i];
            configBuffers[buffersCount++] = m_packedSvcPrefixBufferId[i];
        }

        packed_header_param_buffer.type                = VAEncPackedHeaderH264_Slice;
        packed_header_param_buffer.has_emulation_bytes = 0;
        packed_header_param_buffer.bit_length          = packedSlice.DataLength - (prefix_bytes * 8); // DataLength is already in bits !

        //MFX_DESTROY_VABUFFER(m_packedSliceHeaderBufferId[i], m_vaDisplay);
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                VAEncPackedHeaderParameterBufferType,
                                sizeof(packed_header_param_buffer),
                                1,
                                &packed_header_param_buffer,
                                &m_packedSliceHeaderBufferId[i]);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        //MFX_DESTROY_VABUFFER(m_packedSliceBufferId[i], m_vaDisplay);
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                VAEncPackedHeaderDataBufferType,
                                (packedSlice.DataLength + 7) / 8 - prefix_bytes, 1, packedSlice.pData + prefix_bytes,
                                &m_packedSliceBufferId[i]);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        configBuffers[buffersCount++] = m_packedSliceHeaderBufferId[i];
        configBuffers[buffersCount++] = m_packedSliceBufferId[i];
    }

    for(size_t i = 0; i < m_slice.size(); ++i)
    {
        //MFX_DESTROY_VABUFFER(m_sliceBufferId[i], m_vaDisplay);
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                VAEncSliceParameterBufferType,
                                sizeof(m_slice[i]),
                                1,
                                &m_slice[i],
                                &m_sliceBufferId[i]);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        //configBuffers[buffersCount++] = m_sliceBufferId[i];
        mdprintf(stderr, "m_sliceBufferId[%zu]=%d\n", i, m_sliceBufferId[i]);
    }

    assert(buffersCount <= configBuffers.size());

    //------------------------------------------------------------------
    // Rendering
    //------------------------------------------------------------------


    MFX_LTRACE_2(MFX_TRACE_LEVEL_HOTSPOTS, "A|ENC|AVC|PACKET_START|", "%d|%d", m_vaContextEncode, task.m_frameNum);
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaBeginPicture");
        vaSts = vaBeginPicture(
                m_vaDisplay,
                m_vaContextEncode,
                *inputSurface);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        mdprintf(stderr,"inputSurface = %d\n",*inputSurface);
    }
    {
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaRenderPicture");
            vaSts = vaRenderPicture(
                m_vaDisplay,
                m_vaContextEncode,
                &configBuffers[0], /* vector store leaner in memory*/
                buffersCount);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        }
        for(size_t i = 0; i < m_slice.size(); i++)
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaRenderPicture");
            vaSts = vaRenderPicture(
                m_vaDisplay,
                m_vaContextEncode,
                &m_sliceBufferId[i],
                1);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        }
    }
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaEndPicture");
        vaSts = vaEndPicture(m_vaDisplay, m_vaContextEncode);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }
    MFX_LTRACE_2(MFX_TRACE_LEVEL_HOTSPOTS, "A|ENC|AVC|PACKET_END|", "%d|%d", m_vaContextEncode, task.m_frameNum);
    //------------------------------------------------------------------
    // PostStage
    //------------------------------------------------------------------
    // put to cache
    {
        UMC::AutomaticUMCMutex guard(m_guard);

        ExtVASurface currentFeedback;
        currentFeedback.number    = task.m_statusReportNumber[feiFieldId];
        currentFeedback.surface   = *inputSurface;
        currentFeedback.mv        = m_vaFeiMVOutId[idxRecon + feiFieldId];
        currentFeedback.mbstat    = m_vaFeiMBStatId[feiFieldId];
        currentFeedback.mbcode    = m_vaFeiMCODEOutId[idxRecon + feiFieldId];
        m_statFeedbackCache.push_back(currentFeedback);
    }

    MFX_DESTROY_VABUFFER(vaFeiFrameControlId,       m_vaDisplay);
    MFX_DESTROY_VABUFFER(vaFeiMVPredId,             m_vaDisplay);
    MFX_DESTROY_VABUFFER(vaFeiMBControlId,          m_vaDisplay);
    MFX_DESTROY_VABUFFER(vaFeiMBQPId,               m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_packedSpsHeaderBufferId, m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_packedSpsBufferId,       m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_spsBufferId,             m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_ppsBufferId,             m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_packedPpsHeaderBufferId, m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_packedPpsBufferId,       m_vaDisplay);

    for(size_t i = 0; i < m_slice.size(); i++)
    {
        MFX_DESTROY_VABUFFER(m_sliceBufferId[i],             m_vaDisplay);
        MFX_DESTROY_VABUFFER(m_packedSliceHeaderBufferId[i], m_vaDisplay);
        MFX_DESTROY_VABUFFER(m_packedSliceBufferId[i],       m_vaDisplay);
    }

    mdprintf(stderr, "submit_vaapi done: %d\n", task.m_frameOrder);
    return MFX_ERR_NONE;

} // mfxStatus VAAPIFEIENCEncoder::Execute( mfxHDL surface, DdiTask const & task, mfxU32 fieldId, PreAllocatedVector const & sei)

mfxStatus VAAPIFEIENCEncoder::QueryStatus(
        DdiTask & task,
        mfxU32 fieldId)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "FEI::ENC::QueryStatus");
    VAStatus vaSts;
    mfxStatus sts = MFX_ERR_NONE;

    mdprintf(stderr, "VAAPIFEIENCEncoder::QueryStatus frame: %d\n", task.m_frameOrder);
    //------------------------------------------
    // (1) mapping feedbackNumber -> surface & bs
    VASurfaceID waitSurface     = VA_INVALID_SURFACE;
    VABufferID vaFeiMBStatId    = VA_INVALID_ID;
    VABufferID vaFeiMBCODEOutId = VA_INVALID_ID;
    VABufferID vaFeiMVOutId     = VA_INVALID_ID;
    mfxU32 indxSurf;

    mfxU32 feiFieldId = task.m_fid[fieldId];

    UMC::AutomaticUMCMutex guard(m_guard);

    for (indxSurf = 0; indxSurf < m_statFeedbackCache.size(); indxSurf++)
    {
        const ExtVASurface & currentFeedback = m_statFeedbackCache[indxSurf];

        if (currentFeedback.number == task.m_statusReportNumber[feiFieldId])
        {
            waitSurface      = currentFeedback.surface;
            vaFeiMBStatId    = currentFeedback.mbstat;
            vaFeiMBCODEOutId = currentFeedback.mbcode;
            vaFeiMVOutId     = currentFeedback.mv;
            break;
        }
    }
    MFX_CHECK(indxSurf != m_statFeedbackCache.size(), MFX_ERR_UNKNOWN);

    VASurfaceStatus surfSts = VASurfaceSkipped;

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaSyncSurface");
        vaSts = vaSyncSurface(m_vaDisplay, waitSurface);
    }

    // ignore VA_STATUS_ERROR_DECODING_ERROR in encoder
    if (vaSts == VA_STATUS_ERROR_DECODING_ERROR)
        vaSts = VA_STATUS_SUCCESS;
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    vaSts = vaQuerySurfaceStatus(m_vaDisplay, waitSurface, &surfSts);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    mdprintf(stderr, "sync on surface: %d\n", surfSts);

    switch (surfSts)
    {
        default: //for now driver does not return correct status
        case VASurfaceReady:
        {
            // In case of single-field processing, only one buffer is attached
            mfxU32 idxToPickBuffer = task.m_singleFieldMode ? 0 : feiFieldId;

            mfxENCOutput*       out       = reinterpret_cast<mfxENCOutput*>(task.m_userData[1]);
            mfxExtFeiEncMBStat* mbstat    = GetExtBufferFEI(out, idxToPickBuffer);
            mfxExtFeiEncMV*     mvout     = GetExtBufferFEI(out, idxToPickBuffer);
            mfxExtFeiPakMBCtrl* mbcodeout = GetExtBufferFEI(out, idxToPickBuffer);

            /* NO Bitstream in ENC */
            task.m_bsDataLength[feiFieldId] = 0;

            if (mbstat != NULL && vaFeiMBStatId != VA_INVALID_ID)
            {
                MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "MBstat");
                VAEncFEIDistortionH264* mbs;
                {
                    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaMapBuffer");
                    vaSts = vaMapBuffer(
                            m_vaDisplay,
                            vaFeiMBStatId,
                            (void **) (&mbs));
                }
                MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
                //copy to output in task here MVs
                FastCopyBufferVid2Sys(mbstat->MB, mbs, sizeof(VAEncFEIDistortionH264) * mbstat->NumMBAlloc);

                {
                    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
                    vaSts = vaUnmapBuffer(m_vaDisplay, vaFeiMBStatId);
                    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
                }
                mdprintf(stderr, "Destroy - Distortion bufId=%d\n", m_vaFeiMBStatId[feiFieldId]);
                MFX_DESTROY_VABUFFER(m_vaFeiMBStatId[feiFieldId], m_vaDisplay);
            }

            if (mvout != NULL && vaFeiMVOutId != VA_INVALID_ID)
            {
                MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "MV");
                VAMotionVector* mvs;
                {
                    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaMapBuffer");
                    vaSts = vaMapBuffer(
                            m_vaDisplay,
                            vaFeiMVOutId,
                            (void **) (&mvs));
                }

                MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

                //copy to output in task here MVs
                FastCopyBufferVid2Sys(mvout->MB, mvs, sizeof(VAMotionVector) * 16 * mvout->NumMBAlloc);

                {
                    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
                    vaSts = vaUnmapBuffer(m_vaDisplay, vaFeiMVOutId);
                    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
                }
//                mdprintf(stderr, "Destroy - MV bufId=%d\n", m_vaFeiMVOutId[feiFieldId]);
//                MFX_DESTROY_VABUFFER(m_vaFeiMVOutId[feiFieldId], m_vaDisplay);
            }

            if (mbcodeout != NULL && vaFeiMBCODEOutId != VA_INVALID_ID)
            {
                MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "MBcode");
                VAEncFEIMBCodeH264* mbcs;
                {
                    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaMapBuffer");
                    vaSts = vaMapBuffer(
                            m_vaDisplay,
                            vaFeiMBCODEOutId,
                            (void **) (&mbcs));
                }
                MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

                //copy to output in task here MVs
                FastCopyBufferVid2Sys(mbcodeout->MB, mbcs, sizeof(VAEncFEIMBCodeH264) * mbcodeout->NumMBAlloc);

                {
                    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
                    vaSts = vaUnmapBuffer(m_vaDisplay, vaFeiMBCODEOutId);
                    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
                }
//                mdprintf(stderr, "Destroy - MBCODE bufId=%d\n", m_vaFeiMCODEOutId[feiFieldId]);
//                MFX_DESTROY_VABUFFER(m_vaFeiMCODEOutId[feiFieldId], m_vaDisplay);
            }

            // remove task
            m_statFeedbackCache.erase(m_statFeedbackCache.begin() + indxSurf);
        }
            sts = MFX_ERR_NONE;
            break;
        case VASurfaceRendering:
        case VASurfaceDisplaying:
            sts = MFX_WRN_DEVICE_BUSY;
            break;

        case VASurfaceSkipped:
            //default:
            assert(!"bad feedback status");
            sts =  MFX_ERR_DEVICE_FAILED;
            //return MFX_ERR_NONE;
            break;
    }

    mdprintf(stderr, "query_vaapi done\n");

    return sts;
} // mfxStatus VAAPIFEIENCEncoder::QueryStatus( DdiTask & task, mfxU32 fieldId)

#endif //#if defined(MFX_ENABLE_H264_VIDEO_FEI_ENC)


#if defined(MFX_ENABLE_H264_VIDEO_FEI_ENC) && defined(MFX_ENABLE_H264_VIDEO_FEI_PAK)

VAAPIFEIPAKEncoder::VAAPIFEIPAKEncoder()
: VAAPIEncoder()
, m_codingFunction(MFX_FEI_FUNCTION_PAK)
, m_statParamsId(VA_INVALID_ID)
, m_statMVId(VA_INVALID_ID)
, m_statOutId(VA_INVALID_ID)
//, m_codedBufferId[0](VA_INVALID_ID)
{
    m_codedBufferId[0] = m_codedBufferId[1] = VA_INVALID_ID;
} // VAAPIFEIPAKEncoder::VAAPIFEIPAKEncoder()

VAAPIFEIPAKEncoder::~VAAPIFEIPAKEncoder()
{

    Destroy();

} // VAAPIFEIPAKEncoder::~VAAPIFEIPAKEncoder()

mfxStatus VAAPIFEIPAKEncoder::Destroy()
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "FEI::PAK::Destroy");

    mfxStatus sts = MFX_ERR_NONE;

    MFX_DESTROY_VABUFFER(m_statParamsId,     m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_statMVId,         m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_statOutId,        m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_codedBufferId[0], m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_codedBufferId[1], m_vaDisplay);

    sts = VAAPIEncoder::Destroy();

    return sts;

} // mfxStatus VAAPIFEIPAKEncoder::Destroy()

mfxStatus VAAPIFEIPAKEncoder::Reset(MfxVideoParam const & par)
{
    m_videoParam = par;

    FillSps(par, m_sps);

    MFX_CHECK_WITH_ASSERT(MFX_ERR_NONE == SetHRD(par, m_vaDisplay, m_vaContextEncode, m_hrdBufferId),       MFX_ERR_DEVICE_FAILED);
    MFX_CHECK_WITH_ASSERT(MFX_ERR_NONE == SetFrameRate(par, m_vaDisplay, m_vaContextEncode, m_frameRateId), MFX_ERR_DEVICE_FAILED);

    FillConstPartOfPps(par, m_pps);

    if (m_caps.HeaderInsertion == 0)
        m_headerPacker.Init(par, m_caps);

    return MFX_ERR_NONE;

}

mfxStatus VAAPIFEIPAKEncoder::CreateAccelerationService(MfxVideoParam const & par)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "FEI::PAK::CreateAccelerationService");
    m_videoParam = par;

    //check for ext buffer for FEI
    m_codingFunction = 0; //Unknown function

    const mfxExtFeiParam* params = GetExtBuffer(par);
    if (params)
    {
        if (MFX_FEI_FUNCTION_PAK == params->Func)
        {
            m_codingFunction = params->Func;
            return CreatePAKAccelerationService(par);
        }
    }

    return MFX_ERR_INVALID_VIDEO_PARAM;
} // mfxStatus VAAPIFEIPAKEncoder::CreateAccelerationService(MfxVideoParam const & par)

mfxStatus VAAPIFEIPAKEncoder::CreatePAKAccelerationService(MfxVideoParam const & par)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "FEI::PAK::CreatePAKAccelerationService");
    MFX_CHECK(m_vaDisplay, MFX_ERR_DEVICE_FAILED);

    VAStatus vaSts = VA_STATUS_SUCCESS;
    VAProfile profile = ConvertProfileTypeMFX2VAAPI(m_videoParam.mfx.CodecProfile);

    //mfxU8 vaRCType = ConvertRateControlMFX2VAAPI(par.mfx.RateControlMethod);
    /* Working only with RC_CPQ */
    //vaRCType = VA_RC_CQP;

    VAEntrypoint entrypoints[5];
    int num_entrypoints, slice_entrypoint;
    VAConfigAttrib attrib[4];

    vaSts = vaQueryConfigEntrypoints( m_vaDisplay, profile,
                                entrypoints,&num_entrypoints);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    for (slice_entrypoint = 0; slice_entrypoint < num_entrypoints; slice_entrypoint++){
        if (entrypoints[slice_entrypoint] == VAEntrypointFEI)
            break;
    }
    /* not find VAEntrypointFEI entry point */
    MFX_CHECK(slice_entrypoint != num_entrypoints, MFX_ERR_DEVICE_FAILED);

    /* find out the format for the render target, and rate control mode */
    attrib[0].type = VAConfigAttribRTFormat;
    attrib[1].type = VAConfigAttribRateControl;
    attrib[2].type = (VAConfigAttribType)VAConfigAttribFEIFunctionType;
    attrib[3].type = (VAConfigAttribType)VAConfigAttribFEIMVPredictors;
    vaSts = vaGetConfigAttributes(m_vaDisplay, profile,
                                    (VAEntrypoint)VAEntrypointFEI, &attrib[0], 4);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    /* not find desired YUV420 RT format */
    MFX_CHECK(attrib[0].value & VA_RT_FORMAT_YUV420, MFX_ERR_DEVICE_FAILED);


    if ((attrib[1].value & VA_RC_CQP) == 0) {
        /* Can't find matched RC mode */
        printf("Can't find the desired RC mode, exit\n");
        return MFX_ERR_DEVICE_FAILED;
    }

    attrib[0].value = VA_RT_FORMAT_YUV420; /* set to desired RT format */
    attrib[1].value = VA_RC_CQP;
    attrib[2].value = VA_FEI_FUNCTION_PAK;
    attrib[3].value = 1; /* set 0 MV Predictor */

    vaSts = vaCreateConfig(m_vaDisplay, profile,
                                (VAEntrypoint)VAEntrypointFEI, &attrib[0], 4, &m_vaConfig);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    std::vector<VASurfaceID> rawSurf;
    rawSurf.reserve(m_reconQueue.size());
    for (mfxU32 i = 0; i < m_reconQueue.size(); ++i)
        rawSurf.push_back(m_reconQueue[i].surface);


    // Encoder create
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaCreateContext");
        vaSts = vaCreateContext(
            m_vaDisplay,
            m_vaConfig,
            m_width,
            m_height,
            VA_PROGRESSIVE,
            Begin(rawSurf),
            rawSurf.size(),
            &m_vaContextEncode);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }

    m_slice.resize(par.mfx.NumSlice); // it is enough for encoding
    m_sliceBufferId.resize(par.mfx.NumSlice);
    m_packedSliceHeaderBufferId.resize(par.mfx.NumSlice);
    m_packedSliceBufferId.resize(par.mfx.NumSlice);

    std::fill(m_sliceBufferId.begin(),             m_sliceBufferId.end(),             VA_INVALID_ID);
    std::fill(m_packedSliceHeaderBufferId.begin(), m_packedSliceHeaderBufferId.end(), VA_INVALID_ID);
    std::fill(m_packedSliceBufferId.begin(),       m_packedSliceBufferId.end(),       VA_INVALID_ID);

    m_vaFeiMVOutId.resize(1);
    m_vaFeiMCODEOutId.resize(1);

    std::fill(m_vaFeiMVOutId.begin(),    m_vaFeiMVOutId.end(),    VA_INVALID_ID);
    std::fill(m_vaFeiMCODEOutId.begin(), m_vaFeiMCODEOutId.end(), VA_INVALID_ID);

    Zero(m_sps);
    Zero(m_pps);
    Zero(m_slice);
    //------------------------------------------------------------------

    FillSps(par, m_sps);
    SetHRD(par, m_vaDisplay, m_vaContextEncode, m_hrdBufferId);
    SetFrameRate(par, m_vaDisplay, m_vaContextEncode, m_frameRateId);
    FillConstPartOfPps(par, m_pps);

    if (m_caps.HeaderInsertion == 0)
        m_headerPacker.Init(par, m_caps);

    return MFX_ERR_NONE;
} // mfxStatus VAAPIFEIPAKEncoder::CreatePAKAccelerationService(MfxVideoParam const & par)


mfxStatus VAAPIFEIPAKEncoder::Register(mfxFrameAllocResponse& response, D3DDDIFORMAT type)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "FEI::PAK::Register");

    std::vector<ExtVASurface> * pQueue = (D3DDDIFMT_INTELENCODE_BITSTREAMDATA == type) ? &m_bsQueue : &m_reconQueue;
    MFX_CHECK(pQueue, MFX_ERR_NULL_PTR);

    mfxStatus sts;

    {
        // we should register allocated HW bitstreams and recon surfaces
        MFX_CHECK(response.mids, MFX_ERR_NULL_PTR);

        ExtVASurface extSurf;
        VASurfaceID *pSurface = NULL;

        for (mfxU32 i = 0; i < response.NumFrameActual; i++)
        {
            sts = m_core->GetFrameHDL(response.mids[i], (mfxHDL *) & pSurface);
            MFX_CHECK_STS(sts);

            extSurf.number  = i;
            extSurf.surface = *pSurface;

            pQueue->push_back(extSurf);
        }
    }

    return MFX_ERR_NONE;

} // mfxStatus VAAPIFEIPAKEncoder::Register(mfxFrameAllocResponse& response, D3DDDIFORMAT type)


mfxStatus VAAPIFEIPAKEncoder::Execute(
        mfxHDLPair  pair,
        DdiTask const & task,
        mfxU32 fieldId,
        PreAllocatedVector const & sei)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "FEI::PAK::Execute");

    mdprintf(stderr, "VAAPIFEIPAKEncoder::Execute\n");

    VAStatus  vaSts  = VA_STATUS_SUCCESS;
    mfxStatus mfxSts = MFX_ERR_NONE;

    mfxU32 feiFieldId = task.m_fid[fieldId];

    std::vector<VABufferID> configBuffers(MAX_CONFIG_BUFFERS_COUNT + m_slice.size() * 2);
    std::fill(configBuffers.begin(), configBuffers.end(), VA_INVALID_ID);

    mfxU16 buffersCount = 0;

    /* HRD parameter */
    mdprintf(stderr, "m_hrdBufferId=%d\n", m_hrdBufferId);
    configBuffers[buffersCount++] = m_hrdBufferId;

    /* frame rate parameter */
    mdprintf(stderr, "m_frameRateId=%d\n", m_frameRateId);
    configBuffers[buffersCount++] = m_frameRateId;

    mfxPAKInput*  in  = reinterpret_cast<mfxPAKInput* >(task.m_userData[0]);
    mfxPAKOutput* out = reinterpret_cast<mfxPAKOutput*>(task.m_userData[1]);

    VABufferID vaFeiFrameControlId = VA_INVALID_ID;

    // In case of single-field processing, only one buffer is attached
    mfxU32 idxToPickBuffer = task.m_singleFieldMode ? 0 : feiFieldId;

    // Extension buffers
#if MFX_VERSION >= 1023
    mfxExtFeiSliceHeader  * pDataSliceHeader = GetExtBufferFEI(in, idxToPickBuffer);
#else
    mfxExtFeiSliceHeader  * pDataSliceHeader = GetExtBufferFEI(out, idxToPickBuffer);
#endif // MFX_VERSION >= 1023
    mfxExtFeiEncMV        * mvout            = GetExtBufferFEI(in, idxToPickBuffer);
    mfxExtFeiPakMBCtrl    * mbcodeout        = GetExtBufferFEI(in, idxToPickBuffer);

    // Update internal PPS from FEI PPS buffer
    mfxExtFeiPPS          * pDataPPS         = GetExtBufferFEI(in, idxToPickBuffer);
    mfxExtPpsHeader       * extPps           = GetExtBuffer(m_videoParam);

    // Force PPS insertion if manual PPS with new parameters provided
    if (task.m_insertPps[fieldId])
    {
        extPps->seqParameterSetId              = pDataPPS->SPSId;
        extPps->picParameterSetId              = pDataPPS->PPSId;

        extPps->picInitQpMinus26               = pDataPPS->PicInitQP - 26;
        extPps->numRefIdxL0DefaultActiveMinus1 = (std::max)(pDataPPS->NumRefIdxL0Active, mfxU16(1)) - 1;
        extPps->numRefIdxL1DefaultActiveMinus1 = (std::max)(pDataPPS->NumRefIdxL1Active, mfxU16(1)) - 1;

        extPps->chromaQpIndexOffset            = pDataPPS->ChromaQPIndexOffset;
        extPps->secondChromaQpIndexOffset      = pDataPPS->SecondChromaQPIndexOffset;
        extPps->transform8x8ModeFlag           = pDataPPS->Transform8x8ModeFlag;
    }

    // Pack headers if required
    if (task.m_insertSps[fieldId] || task.m_insertPps[fieldId])
    {
        m_headerPacker.Init(m_videoParam, m_caps);
    }

    VAEncPackedHeaderParameterBuffer packed_header_param_buffer;

    // AUD
    if (task.m_insertAud[fieldId])
    {
        ENCODE_PACKEDHEADER_DATA const & packedAud = m_headerPacker.PackAud(task, fieldId);

        packed_header_param_buffer.type                = VAEncPackedHeaderRawData;
        packed_header_param_buffer.has_emulation_bytes = !packedAud.SkipEmulationByteCount;
        packed_header_param_buffer.bit_length          = packedAud.DataLength * 8;

        MFX_DESTROY_VABUFFER(m_packedAudHeaderBufferId, m_vaDisplay);
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                VAEncPackedHeaderParameterBufferType,
                                sizeof(packed_header_param_buffer),
                                1,
                                &packed_header_param_buffer,
                                &m_packedAudHeaderBufferId);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        MFX_DESTROY_VABUFFER(m_packedAudBufferId, m_vaDisplay);
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                VAEncPackedHeaderDataBufferType,
                                packedAud.DataLength, 1, packedAud.pData,
                                &m_packedAudBufferId);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        configBuffers[buffersCount++] = m_packedAudHeaderBufferId;
        configBuffers[buffersCount++] = m_packedAudBufferId;
    }

    // SPS
    if (task.m_insertSps[fieldId])
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "SPS");
        std::vector<ENCODE_PACKEDHEADER_DATA> const & packedSpsArray = m_headerPacker.GetSps();
        ENCODE_PACKEDHEADER_DATA const & packedSps = packedSpsArray[0];

        packed_header_param_buffer.type                = VAEncPackedHeaderSequence;
        packed_header_param_buffer.has_emulation_bytes = !packedSps.SkipEmulationByteCount;
        packed_header_param_buffer.bit_length          = packedSps.DataLength*8;

        MFX_DESTROY_VABUFFER(m_packedSpsHeaderBufferId, m_vaDisplay);
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                VAEncPackedHeaderParameterBufferType,
                                sizeof(packed_header_param_buffer),
                                1,
                                &packed_header_param_buffer,
                                &m_packedSpsHeaderBufferId);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        MFX_DESTROY_VABUFFER(m_packedSpsBufferId, m_vaDisplay);
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                VAEncPackedHeaderDataBufferType,
                                packedSps.DataLength, 1, packedSps.pData,
                                &m_packedSpsBufferId);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        //packedBufferIndexes.push_back(buffersCount);
        //packedDataSize += packed_header_param_buffer.bit_length;
        configBuffers[buffersCount++] = m_packedSpsHeaderBufferId;
        configBuffers[buffersCount++] = m_packedSpsBufferId;
        /* sequence parameter set */
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                VAEncSequenceParameterBufferType,
                                sizeof(m_sps),1,
                                &m_sps,
                                &m_spsBufferId);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        mdprintf(stderr, "m_spsBufferId=%d\n", m_spsBufferId);
        configBuffers[buffersCount++] = m_spsBufferId;
    }

    // Create buffer for MV input
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "MV");
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                (VABufferType)VAEncFEIMVBufferType,
                                sizeof(VAMotionVector) * 16 * mvout->NumMBAlloc,
                                1, //limitation from driver, num elements should be 1
                                mvout->MB,
                                &m_vaFeiMVOutId[0]);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        mdprintf(stderr, "MV Out bufId[%d]=%d\n", 0, m_vaFeiMVOutId[0]);
    }

    // Create buffer for PAKobj input
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "MBcode");

        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                (VABufferType)VAEncFEIMBCodeBufferType,
                                sizeof(VAEncFEIMBCodeH264)*mbcodeout->NumMBAlloc,
                                1, //limitation from driver, num elements should be 1
                                mbcodeout->MB,
                                &m_vaFeiMCODEOutId[0]);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        mdprintf(stderr, "MCODE Out bufId[%d]=%d\n", 0, m_vaFeiMCODEOutId[0]);
    }

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "FrameCtrl");
        VAEncMiscParameterBuffer *miscParam;
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                VAEncMiscParameterBufferType,
                                sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterFEIFrameControlH264),
                                1, //limitation from driver, num elements should be 1
                                NULL,
                                &vaFeiFrameControlId);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        mdprintf(stderr, "vaFeiFrameControlId=%d\n", vaFeiFrameControlId);

        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaMapBuffer");
            vaSts = vaMapBuffer(m_vaDisplay,
                vaFeiFrameControlId,
                (void **)&miscParam);
        }
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED)

        miscParam->type = (VAEncMiscParameterType)VAEncMiscParameterTypeFEIFrameControl;
        VAEncMiscParameterFEIFrameControlH264* vaFeiFrameControl = (VAEncMiscParameterFEIFrameControlH264*)miscParam->data;
        memset(vaFeiFrameControl, 0, sizeof(VAEncMiscParameterFEIFrameControlH264)); //check if we need this

        vaFeiFrameControl->function     = VA_FEI_FUNCTION_PAK;

        vaFeiFrameControl->mv_data      = m_vaFeiMVOutId[0];
        vaFeiFrameControl->mb_code_data = m_vaFeiMCODEOutId[0];

        vaFeiFrameControl->mb_ctrl      = VA_INVALID_ID;
        vaFeiFrameControl->distortion   = VA_INVALID_ID;
        vaFeiFrameControl->qp           = VA_INVALID_ID;
        vaFeiFrameControl->mv_predictor = VA_INVALID_ID;

        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
            vaSts = vaUnmapBuffer(m_vaDisplay, vaFeiFrameControlId);  //check for deletions
        }
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED)

        configBuffers[buffersCount++] = vaFeiFrameControlId;
    }

    // Fill PPS
    UpdatePPS(task, fieldId, m_pps, m_reconQueue);

    // Update from FEI PPS
    m_pps.pic_init_qp                             = extPps->picInitQpMinus26 + 26;
    m_pps.num_ref_idx_l0_active_minus1            = extPps->numRefIdxL0DefaultActiveMinus1;
    m_pps.num_ref_idx_l1_active_minus1            = extPps->numRefIdxL1DefaultActiveMinus1;
    m_pps.chroma_qp_index_offset                  = extPps->chromaQpIndexOffset;
    m_pps.second_chroma_qp_index_offset           = extPps->secondChromaQpIndexOffset;
    m_pps.pic_fields.bits.transform_8x8_mode_flag = extPps->transform8x8ModeFlag;

    /* ENC & PAK surfaces management notes:
    * (1): ENC does not generate real reconstruct surface, but driver use surface ID to store
    * additional internal information.
    * (2): PAK generate real reconstruct surface.
    * (3): Generated reconstructed surfaces become references and managed accordingly by application.
    * (4): Library does not manage reference by itself.
    * (5): And of course main rule: ENC (N number call) and PAK (N number call) should have same exactly
    * same reference /reconstruct list !
    * */

    mfxHDL recon_handle;
    mfxSts = m_core->GetExternalFrameHDL(out->OutSurface->Data.MemId, &recon_handle);
    MFX_CHECK(MFX_ERR_NONE == mfxSts, MFX_ERR_INVALID_HANDLE);
    m_pps.CurrPic.picture_id = *(VASurfaceID*) recon_handle; //id in the memory by ptr

    // Driver select progressive / interlaced based on this field
    m_pps.CurrPic.flags = task.m_fieldPicFlag ? (TFIELD == fieldId ? VA_PICTURE_H264_TOP_FIELD : VA_PICTURE_H264_BOTTOM_FIELD) : 0;


    /* Need to allocated coded buffer */
    if (VA_INVALID_ID == m_codedBufferId[feiFieldId])
    {
        int width32  = ((m_videoParam.mfx.FrameInfo.Width  + 31) >> 5) << 5;
        int height32 = ((m_videoParam.mfx.FrameInfo.Height + 31) >> 5) << 5;
        int codedbuf_size = static_cast<int>((width32 * height32) * 400LL / (16 * 16));

        // To workaround an issue with VA coded bufer overflow due to IPCM violation.
        // TODO: consider removing it once IPCM issue is fixed.
        codedbuf_size = 2 * codedbuf_size;

        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                VAEncCodedBufferType,
                                codedbuf_size,
                                1,
                                NULL,
                                &m_codedBufferId[feiFieldId]);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        mdprintf(stderr, "m_codedBufferId=%d\n", m_codedBufferId[feiFieldId]);
    }
    m_pps.coded_buf = m_codedBufferId[feiFieldId];

    vaSts = vaCreateBuffer(m_vaDisplay,
                            m_vaContextEncode,
                            VAEncPictureParameterBufferType,
                            sizeof(m_pps),
                            1,
                            &m_pps,
                            &m_ppsBufferId);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    configBuffers[buffersCount++] = m_ppsBufferId;
    mdprintf(stderr, "m_ppsBufferId=%d\n", m_ppsBufferId);

    if (task.m_insertPps[fieldId])
    {
        // PPS
        std::vector<ENCODE_PACKEDHEADER_DATA> const & packedPpsArray = m_headerPacker.GetPps();
        ENCODE_PACKEDHEADER_DATA const & packedPps = packedPpsArray[0];

        packed_header_param_buffer.type                = VAEncPackedHeaderPicture;
        packed_header_param_buffer.has_emulation_bytes = !packedPps.SkipEmulationByteCount;
        packed_header_param_buffer.bit_length          = packedPps.DataLength*8;

        MFX_DESTROY_VABUFFER(m_packedPpsHeaderBufferId, m_vaDisplay);
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                VAEncPackedHeaderParameterBufferType,
                                sizeof(packed_header_param_buffer),
                                1,
                                &packed_header_param_buffer,
                                &m_packedPpsHeaderBufferId);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        mdprintf(stderr, "m_packedPpsHeaderBufferId=%d\n", m_packedPpsHeaderBufferId);

        MFX_DESTROY_VABUFFER(m_packedPpsBufferId, m_vaDisplay);
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                VAEncPackedHeaderDataBufferType,
                                packedPps.DataLength, 1, packedPps.pData,
                                &m_packedPpsBufferId);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        mdprintf(stderr, "m_packedPpsBufferId=%d\n", m_packedPpsBufferId);

        //packedBufferIndexes.push_back(buffersCount);
        //packedDataSize += packed_header_param_buffer.bit_length;
        configBuffers[buffersCount++] = m_packedPpsHeaderBufferId;
        configBuffers[buffersCount++] = m_packedPpsBufferId;
    }

     //SEI
    if (sei.Size() > 0)
    {
        packed_header_param_buffer.type                = VAEncPackedHeaderRawData;
        packed_header_param_buffer.has_emulation_bytes = 1;
        packed_header_param_buffer.bit_length          = sei.Size() * 8;

        MFX_DESTROY_VABUFFER(m_packedSeiHeaderBufferId, m_vaDisplay);
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                VAEncPackedHeaderParameterBufferType,
                                sizeof(packed_header_param_buffer),
                                1,
                                &packed_header_param_buffer,
                                &m_packedSeiHeaderBufferId);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        MFX_DESTROY_VABUFFER(m_packedSeiBufferId,m_vaDisplay);
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                VAEncPackedHeaderDataBufferType,
                                sei.Size(), 1, RemoveConst(sei.Buffer()),
                                &m_packedSeiBufferId);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        configBuffers[buffersCount++] = m_packedSeiHeaderBufferId;
        configBuffers[buffersCount++] = m_packedSeiBufferId;
    }

    // Fill SliceHeaders
    UpdateSlice(m_caps, task, fieldId, m_sps, m_pps, m_slice, m_videoParam, m_reconQueue);

    for (size_t i = 0; i < m_slice.size(); ++i)
    {
        // Note: legacy use 5,6,7 type values, but for FEI 0,1,2

        m_slice[i].macroblock_address            = pDataSliceHeader->Slice[i].MBAddress;
        m_slice[i].num_macroblocks               = pDataSliceHeader->Slice[i].NumMBs;
        m_slice[i].slice_type                    = pDataSliceHeader->Slice[i].SliceType;
        m_slice[i].idr_pic_id                    = pDataSliceHeader->Slice[i].IdrPicId;
        m_slice[i].cabac_init_idc                = pDataSliceHeader->Slice[i].CabacInitIdc;
        m_slice[i].slice_qp_delta                = pDataSliceHeader->Slice[i].SliceQPDelta;
        m_slice[i].disable_deblocking_filter_idc = pDataSliceHeader->Slice[i].DisableDeblockingFilterIdc;
        m_slice[i].slice_alpha_c0_offset_div2    = pDataSliceHeader->Slice[i].SliceAlphaC0OffsetDiv2;
        m_slice[i].slice_beta_offset_div2        = pDataSliceHeader->Slice[i].SliceBetaOffsetDiv2;
    } // for (size_t i = 0; i < m_slice.size(); ++i)

    mfxU32 prefix_bytes = (task.m_AUStartsFromSlice[fieldId] + 8) * m_headerPacker.isSvcPrefixUsed();

    std::vector<ENCODE_PACKEDHEADER_DATA> const & packedSlices = m_headerPacker.PackSlices(task, fieldId);

    for (size_t i = 0; i < packedSlices.size(); i++)
    {
        ENCODE_PACKEDHEADER_DATA const & packedSlice = packedSlices[i];

        if (prefix_bytes)
        {
            packed_header_param_buffer.type                = VAEncPackedHeaderRawData;
            packed_header_param_buffer.has_emulation_bytes = 1;
            packed_header_param_buffer.bit_length          = (prefix_bytes * 8);

            vaSts = vaCreateBuffer(m_vaDisplay,
                                    m_vaContextEncode,
                                    VAEncPackedHeaderParameterBufferType,
                                    sizeof(packed_header_param_buffer),
                                    1,
                                    &packed_header_param_buffer,
                                    &m_packedSvcPrefixHeaderBufferId[i]);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

            vaSts = vaCreateBuffer(m_vaDisplay,
                                    m_vaContextEncode,
                                    VAEncPackedHeaderDataBufferType,
                                    prefix_bytes, 1, packedSlice.pData,
                                    &m_packedSvcPrefixBufferId[i]);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

            configBuffers[buffersCount++] = m_packedSvcPrefixHeaderBufferId[i];
            configBuffers[buffersCount++] = m_packedSvcPrefixBufferId[i];
        }

        packed_header_param_buffer.type                = VAEncPackedHeaderH264_Slice;
        packed_header_param_buffer.has_emulation_bytes = 0;
        packed_header_param_buffer.bit_length          = packedSlice.DataLength - (prefix_bytes * 8); // DataLength is already in bits !

        //MFX_DESTROY_VABUFFER(m_packedSliceHeaderBufferId[i], m_vaDisplay);
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                VAEncPackedHeaderParameterBufferType,
                                sizeof(packed_header_param_buffer),
                                1,
                                &packed_header_param_buffer,
                                &m_packedSliceHeaderBufferId[i]);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        //MFX_DESTROY_VABUFFER(m_packedSliceBufferId[i], m_vaDisplay);
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                VAEncPackedHeaderDataBufferType,
                                (packedSlice.DataLength + 7) / 8 - prefix_bytes, 1, packedSlice.pData + prefix_bytes,
                                &m_packedSliceBufferId[i]);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        configBuffers[buffersCount++] = m_packedSliceHeaderBufferId[i];
        configBuffers[buffersCount++] = m_packedSliceBufferId[i];
    }

    for( size_t i = 0; i < m_slice.size(); i++ )
    {
        //MFX_DESTROY_VABUFFER(m_sliceBufferId[i], m_vaDisplay);
        vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextEncode,
                                VAEncSliceParameterBufferType,
                                sizeof(m_slice[i]),
                                1,
                                &m_slice[i],
                                &m_sliceBufferId[i]);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        //configBuffers[buffersCount++] = m_sliceBufferId[i];
        mdprintf(stderr, "m_sliceBufferId[%zu]=%d\n", i, m_sliceBufferId[i]);
    }


    assert(buffersCount <= configBuffers.size());


    mfxHDL handle_inp;
    mfxSts = m_core->GetExternalFrameHDL(in->InSurface->Data.MemId, &handle_inp);
    MFX_CHECK(MFX_ERR_NONE == mfxSts, MFX_ERR_INVALID_HANDLE);
    VASurfaceID *inputSurface = (VASurfaceID*)handle_inp; //id in the memory by ptr

    //------------------------------------------------------------------
    // Rendering
    //------------------------------------------------------------------

    MFX_LTRACE_2(MFX_TRACE_LEVEL_HOTSPOTS, "A|PAK|AVC|PACKET_START|", "%d|%d", m_vaContextEncode, task.m_frameNum);
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaBeginPicture");
        vaSts = vaBeginPicture(
                m_vaDisplay,
                m_vaContextEncode,
                *inputSurface);

        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        mdprintf(stderr,"inputSurface = %d\n",*inputSurface);
    }
    {
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaRenderPicture");
            vaSts = vaRenderPicture(
                m_vaDisplay,
                m_vaContextEncode,
                &configBuffers[0], /* vector store leaner in memory*/
                buffersCount);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        }
        for(size_t i = 0; i < m_slice.size(); i++)
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaRenderPicture");
            vaSts = vaRenderPicture(
                m_vaDisplay,
                m_vaContextEncode,
                &m_sliceBufferId[i],
                1);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        }
    }
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaEndPicture");
        vaSts = vaEndPicture(m_vaDisplay, m_vaContextEncode);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }
    MFX_LTRACE_2(MFX_TRACE_LEVEL_HOTSPOTS, "A|PAK|AVC|PACKET_END|", "%d|%d", m_vaContextEncode, task.m_frameNum);

    //------------------------------------------------------------------
    // PostStage
    //------------------------------------------------------------------
    // put to cache
    {
        UMC::AutomaticUMCMutex guard(m_guard);

        ExtVASurface currentFeedback;
        currentFeedback.number    = task.m_statusReportNumber[feiFieldId];
        currentFeedback.surface   = *inputSurface;

        m_statFeedbackCache.push_back(currentFeedback);
    }

    MFX_DESTROY_VABUFFER(m_vaFeiMVOutId[0],         m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_vaFeiMCODEOutId[0],      m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_packedAudHeaderBufferId, m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_packedAudBufferId,       m_vaDisplay);
    MFX_DESTROY_VABUFFER(vaFeiFrameControlId,       m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_packedSpsHeaderBufferId, m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_packedSpsBufferId,       m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_spsBufferId,             m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_ppsBufferId,             m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_packedPpsHeaderBufferId, m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_packedPpsBufferId,       m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_packedSeiHeaderBufferId, m_vaDisplay);
    MFX_DESTROY_VABUFFER(m_packedSeiBufferId,       m_vaDisplay);

    for( size_t i = 0; i < m_slice.size(); i++ )
    {
        MFX_DESTROY_VABUFFER(m_sliceBufferId[i],             m_vaDisplay);
        MFX_DESTROY_VABUFFER(m_packedSliceHeaderBufferId[i], m_vaDisplay);
        MFX_DESTROY_VABUFFER(m_packedSliceBufferId[i],       m_vaDisplay);
    }

    mdprintf(stderr, "submit_vaapi done: %d\n", task.m_frameOrder);
    return MFX_ERR_NONE;

} // mfxStatus VAAPIFEIPAKEncoder::Execute( mfxHDL surface, DdiTask const & task, mfxU32 fieldId, PreAllocatedVector const & sei)


mfxStatus VAAPIFEIPAKEncoder::QueryStatus(
        DdiTask & task,
        mfxU32 fieldId)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "FEI::PAK::QueryStatus");
    VAStatus vaSts;
    mfxStatus sts = MFX_ERR_NONE;

    mdprintf(stderr, "VAAPIFEIPAKEncoder::QueryStatus frame: %d\n", task.m_frameOrder);
    //------------------------------------------
    // (1) mapping feedbackNumber -> surface & bs
    VASurfaceID waitSurface     = VA_INVALID_SURFACE;

    mfxU32 indxSurf, feiFieldId = task.m_fid[fieldId];

    UMC::AutomaticUMCMutex guard(m_guard);

    for (indxSurf = 0; indxSurf < m_statFeedbackCache.size(); ++indxSurf)
    {
        const ExtVASurface & currentFeedback = m_statFeedbackCache[indxSurf];

        if (currentFeedback.number == task.m_statusReportNumber[feiFieldId])
        {
            waitSurface = currentFeedback.surface;
            break;
        }
    }
    MFX_CHECK(indxSurf != m_statFeedbackCache.size(), MFX_ERR_UNKNOWN);


    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaSyncSurface");
        vaSts = vaSyncSurface(m_vaDisplay, waitSurface);
    }

    //  ignore VA_STATUS_ERROR_DECODING_ERROR in encoder
    if (vaSts == VA_STATUS_ERROR_DECODING_ERROR)
        vaSts = VA_STATUS_SUCCESS;
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    VASurfaceStatus surfSts = VASurfaceSkipped;

    vaSts = vaQuerySurfaceStatus(m_vaDisplay, waitSurface, &surfSts);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    mdprintf(stderr, "sync on surface: %d\n", surfSts);

    switch (surfSts)
    {
        default:
        case VASurfaceReady:
        {
            VACodedBufferSegment *codedBufferSegment;
            {
                MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaMapBuffer");
                vaSts = vaMapBuffer(
                            m_vaDisplay,
                            m_codedBufferId[feiFieldId],
                            (void **)(&codedBufferSegment));
                MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
            }

            if (codedBufferSegment->status & VA_CODED_BUF_STATUS_BAD_BITSTREAM)
            {
                sts = MFX_ERR_GPU_HANG;
            }
            else if (!codedBufferSegment->size || !codedBufferSegment->buf)
            {
                sts = MFX_ERR_DEVICE_FAILED;
            }
            else
            {
                task.m_bsDataLength[feiFieldId] = codedBufferSegment->size;

                FastCopyBufferVid2Sys(task.m_bs->Data + task.m_bs->DataLength,
                                      codedBufferSegment->buf, codedBufferSegment->size);

                task.m_bs->DataLength += codedBufferSegment->size;
                //Update other fields in mfxBitstream
                task.m_bs->TimeStamp = task.m_timeStamp;
                task.m_bs->DecodeTimeStamp = CalcDTSFromPTS(m_videoParam.mfx.FrameInfo,
                                             mfxU16(task.m_dpbOutputDelay), task.m_timeStamp);
                task.m_bs->PicStruct = task.GetPicStructForDisplay();
                task.m_bs->FrameType = task.m_type[task.GetFirstField()] & ~MFX_FRAMETYPE_KEYPIC;
                if (task.m_fieldPicFlag)
                    task.m_bs->FrameType = mfxU16(task.m_bs->FrameType |
                    ((task.m_type[!task.GetFirstField()]& ~MFX_FRAMETYPE_KEYPIC) << 8));
                sts = MFX_ERR_NONE;
            }

            {
                MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
                vaSts = vaUnmapBuffer( m_vaDisplay, m_codedBufferId[feiFieldId] );
                MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

                // ??? may affect for performance
                MFX_DESTROY_VABUFFER(m_codedBufferId[feiFieldId], m_vaDisplay);
            }

            // remove task
            m_statFeedbackCache.erase(m_statFeedbackCache.begin() + indxSurf);
        }
            break;
        case VASurfaceRendering:
        case VASurfaceDisplaying:
            sts = MFX_WRN_DEVICE_BUSY;
            break;

        case VASurfaceSkipped:
            //default:
            assert(!"bad feedback status");
            sts = MFX_ERR_DEVICE_FAILED;
            //return MFX_ERR_NONE;
            break;
    }

    mdprintf(stderr, "query_vaapi done\n");
    return sts;
} // mfxStatus VAAPIFEIPAKEncoder::QueryStatus( DdiTask & task, mfxU32 fieldId)

#endif //defined(MFX_ENABLE_H264_VIDEO_FEI_ENC) && defined(MFX_ENABLE_H264_VIDEO_FEI_PAK)

#endif // (MFX_ENABLE_H264_VIDEO_ENCODE) && (MFX_VA_LINUX)
/* EOF */
