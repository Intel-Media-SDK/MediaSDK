// Copyright (c) 2017-2020 Intel Corporation
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

#if defined (MFX_ENABLE_MJPEG_VIDEO_ENCODE) && defined (MFX_VA_LINUX)

#include "mfx_mjpeg_encode_hw_utils.h"
#include "libmfx_core_factory.h"
#include "libmfx_core_interface.h"
#include "jpegbase.h"
#include "mfx_enc_common.h"

#include "mfx_mjpeg_encode_vaapi.h"
#include "libmfx_core_interface.h"
#include "mfx_session.h"
#include "mfx_mjpeg_encode_hw_utils.h"
#include "libmfx_core_vaapi.h"
#include "fast_copy.h"

using namespace MfxHwMJpegEncode;

VAAPIEncoder::VAAPIEncoder()
 : m_core(NULL)
 , m_width(-1)
 , m_height(-1)
 , m_caps()
 , m_vaDisplay(0)
 , m_vaContextEncode(VA_INVALID_ID)
 , m_vaConfig(VA_INVALID_ID)
 , m_qmBufferId(VA_INVALID_ID)
 , m_htBufferId(VA_INVALID_ID)
 , m_scanBufferId(VA_INVALID_ID)
 , m_ppsBufferId(VA_INVALID_ID)
 , m_priorityBufferId(VA_INVALID_ID)
 , m_priorityBuffer()
 , m_MaxContextPriority(0)
{
}

VAAPIEncoder::~VAAPIEncoder()
{
    Destroy();
}

mfxStatus VAAPIEncoder::CreateAuxilliaryDevice(
    VideoCORE * core,
    mfxU32      width,
    mfxU32      height,
    bool        isTemporal)
{
    (void)isTemporal;

    m_core = core;
    VAAPIVideoCORE * hwcore = dynamic_cast<VAAPIVideoCORE *>(m_core);
    MFX_CHECK_WITH_ASSERT(hwcore != 0, MFX_ERR_DEVICE_FAILED);
    if(hwcore)
    {
        mfxStatus mfxSts = hwcore->GetVAService(&m_vaDisplay);
        MFX_CHECK_STS(mfxSts);
    }

    MFX_CHECK(m_vaDisplay, MFX_ERR_DEVICE_FAILED);
    VAStatus vaSts;

    mfxI32 entrypointsIndx = 0;
    mfxI32 numEntrypoints = vaMaxNumEntrypoints(m_vaDisplay);
    MFX_CHECK(numEntrypoints, MFX_ERR_DEVICE_FAILED);

    std::vector<VAEntrypoint> pEntrypoints(numEntrypoints);

    vaSts = vaQueryConfigEntrypoints(
                m_vaDisplay,
                VAProfileJPEGBaseline,
                &pEntrypoints[0],
                &numEntrypoints);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    bool bEncodeEnable = false;
    for( entrypointsIndx = 0; entrypointsIndx < numEntrypoints; entrypointsIndx++ )
    {
        if( VAEntrypointEncPicture == pEntrypoints[entrypointsIndx] )
        {
            bEncodeEnable = true;
            break;
        }
    }
    if( !bEncodeEnable )
    {
        return MFX_ERR_DEVICE_FAILED;
    }
    m_width  = width;
    m_height = height;

    // set caps
    memset(&m_caps, 0, sizeof(m_caps));
    m_caps.Baseline         = 1;
    m_caps.Sequential       = 1;
    m_caps.Huffman          = 1;

    m_caps.NonInterleaved   = 0;
    m_caps.Interleaved      = 1;

    m_caps.SampleBitDepth   = 8;

    std::map<VAConfigAttribType, int> attrib_map;

    VAConfigAttribType attrib_types[] = {
        VAConfigAttribEncJPEG,
        VAConfigAttribMaxPictureWidth,
        VAConfigAttribMaxPictureHeight,
        VAConfigAttribContextPriority
    };

    std::vector<VAConfigAttrib> attrib;
    attrib.reserve(sizeof(attrib_types) / sizeof(attrib_types[0]));
    for (size_t i = 0; i < sizeof(attrib_types) / sizeof(attrib_types[0]); i++)
    {
        attrib.push_back(VAConfigAttrib{attrib_types[i], 0});
        attrib_map[attrib_types[i]] = i;
    }

    vaSts = vaGetConfigAttributes(m_vaDisplay,
                          VAProfileJPEGBaseline,
                          VAEntrypointEncPicture,
                          attrib.data(), attrib.size());

    VAConfigAttribValEncJPEG encAttribVal;
    encAttribVal.value      = attrib[attrib_map[VAConfigAttribEncJPEG]].value;
    m_caps.MaxNumComponent  = encAttribVal.bits.max_num_components;
    m_caps.MaxNumScan       = encAttribVal.bits.max_num_scans;
    m_caps.MaxNumHuffTable  = encAttribVal.bits.max_num_huffman_tables;
    m_caps.MaxNumQuantTable = encAttribVal.bits.max_num_quantization_tables;
    m_caps.MaxPicWidth      = attrib[attrib_map[VAConfigAttribMaxPictureWidth]].value;
    m_caps.MaxPicHeight     = attrib[attrib_map[VAConfigAttribMaxPictureHeight]].value;

    if(attrib[attrib_map[VAConfigAttribContextPriority]].value != VA_ATTRIB_NOT_SUPPORTED)
        m_MaxContextPriority = attrib[attrib_map[VAConfigAttribContextPriority]].value;

    return MFX_ERR_NONE;
}

mfxStatus VAAPIEncoder::CreateAccelerationService(mfxVideoParam const & par)
{
    MFX_CHECK(m_vaDisplay, MFX_ERR_DEVICE_FAILED);
    VAStatus vaSts;

    mfxI32 entrypointsIndx = 0;
    mfxI32 numEntrypoints = vaMaxNumEntrypoints(m_vaDisplay);
    MFX_CHECK(numEntrypoints, MFX_ERR_DEVICE_FAILED);

    std::vector<VAEntrypoint> pEntrypoints(numEntrypoints);

    MFX_CHECK_WITH_ASSERT(par.mfx.CodecProfile == MFX_PROFILE_JPEG_BASELINE, MFX_ERR_DEVICE_FAILED);

    vaSts = vaQueryConfigEntrypoints(
                m_vaDisplay,
                VAProfileJPEGBaseline,
                &pEntrypoints[0],
                &numEntrypoints);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    bool bEncodeEnable = false;
    for( entrypointsIndx = 0; entrypointsIndx < numEntrypoints; entrypointsIndx++ )
    {
        if( VAEntrypointEncPicture == pEntrypoints[entrypointsIndx] )
        {
            bEncodeEnable = true;
            break;
        }
    }
    if( !bEncodeEnable )
    {
        return MFX_ERR_DEVICE_FAILED;
    }

    // Configuration
    VAConfigAttrib attrib;

    attrib.type = VAConfigAttribRTFormat;

    vaSts = vaGetConfigAttributes(m_vaDisplay,
                          VAProfileJPEGBaseline,
                          VAEntrypointEncPicture,
                          &attrib, 1);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    if (!(attrib.value & VA_RT_FORMAT_YUV420) || !(attrib.value & VA_RT_FORMAT_YUV422) ) //to be do
        return MFX_ERR_DEVICE_FAILED;

    vaSts = vaCreateConfig(
        m_vaDisplay,
        VAProfileJPEGBaseline,
        VAEntrypointEncPicture,
        NULL,
        0,
        &m_vaConfig);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

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

    return MFX_ERR_NONE;
}

mfxStatus VAAPIEncoder::QueryBitstreamBufferInfo(mfxFrameAllocRequest& request)
{

    // request linear buffer
    request.Info.FourCC = MFX_FOURCC_P8;

    // context_id required for allocation video memory (tmp solution)
    request.AllocId = m_vaContextEncode;
    return MFX_ERR_NONE;
}

mfxStatus VAAPIEncoder::QueryEncodeCaps(JpegEncCaps & caps)
{
    MFX_CHECK_NULL_PTR1(m_core);

    caps = m_caps;

    return MFX_ERR_NONE;
}

mfxStatus VAAPIEncoder::RegisterBitstreamBuffer(mfxFrameAllocResponse& response)
{
    std::vector<ExtVASurface> * pQueue;
    mfxStatus sts;
    pQueue = &m_bsQueue;

    {
        // we should register allocated HW bitstreams and recon surfaces
        MFX_CHECK( response.mids, MFX_ERR_NULL_PTR );

        ExtVASurface extSurf = {VA_INVALID_ID, 0, 0, 0};
        VASurfaceID *pSurface = NULL;

        for (mfxU32 i = 0; i < response.NumFrameActual; i++)
        {
            sts = m_core->GetFrameHDL(response.mids[i], (mfxHDL *)&pSurface);
            MFX_CHECK_STS(sts);

            extSurf.number  = i;
            extSurf.surface = *pSurface;
            pQueue->push_back( extSurf );
        }
    }

    return MFX_ERR_NONE;
}

mfxStatus VAAPIEncoder::FillPriorityBuffer(mfxPriority& priority)
{
    VAStatus vaSts;
    memset(&m_priorityBuffer, 0, sizeof(VAContextParameterUpdateBuffer));
    m_priorityBuffer.flags.bits.context_priority_update = 1; 

    if(priority == MFX_PRIORITY_LOW)
    {
        m_priorityBuffer.context_priority.bits.priority = 0;
    }
    else if (priority == MFX_PRIORITY_HIGH)
    {
        m_priorityBuffer.context_priority.bits.priority = m_MaxContextPriority;
    }
    else
    {
        m_priorityBuffer.context_priority.bits.priority = m_MaxContextPriority/2;
    }
    mfxStatus sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_priorityBufferId);
    MFX_CHECK_STS(sts);

    vaSts = vaCreateBuffer(m_vaDisplay,
        m_vaContextEncode,
        VAContextParameterUpdateBufferType,
        sizeof(m_priorityBuffer),
        1,
        &m_priorityBuffer,
        &m_priorityBufferId);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    return MFX_ERR_NONE;
}

mfxStatus VAAPIEncoder::Execute(DdiTask &task, mfxHDL surface)
{
    VAStatus vaSts;
    ExecuteBuffers *pExecuteBuffers = task.m_pDdiData;
    ExtVASurface codedbuffer = m_bsQueue[task.m_idxBS];
    pExecuteBuffers->m_pps.coded_buf = (VABufferID)codedbuffer.surface;

    vaSts = vaBeginPicture(m_vaDisplay, m_vaContextEncode, *(VASurfaceID*)surface);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    DestroyBuffers();
    vaSts = vaCreateBuffer(m_vaDisplay, m_vaContextEncode, VAEncPictureParameterBufferType, sizeof(VAEncPictureParameterBufferJPEG), 1, &pExecuteBuffers->m_pps, &m_ppsBufferId);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    if(pExecuteBuffers->m_dqt_list.size())
    {
        // only the first dq table has been handled
        vaSts = vaCreateBuffer(m_vaDisplay, m_vaContextEncode, VAQMatrixBufferType, sizeof(VAQMatrixBufferJPEG), 1, &pExecuteBuffers->m_dqt_list[0], &m_qmBufferId);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }
    if(pExecuteBuffers->m_dht_list.size())
    {
        // only the first huffmn table has been handled
        vaSts = vaCreateBuffer(m_vaDisplay, m_vaContextEncode, VAHuffmanTableBufferType, sizeof(VAHuffmanTableBufferJPEGBaseline), 1, &pExecuteBuffers->m_dht_list[0], &m_htBufferId);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }
    if(pExecuteBuffers->m_payload_list.size())
    {
        m_appBufferIds.resize(pExecuteBuffers->m_payload_list.size());
        for( mfxU8 index = 0; index < pExecuteBuffers->m_payload_list.size(); index++)
        {
            vaSts = vaCreateBuffer(m_vaDisplay,
                                   m_vaContextEncode,
                                   VAEncPackedHeaderDataBufferType,
                                   pExecuteBuffers->m_payload_list[index].length,
                                   1,
                                   pExecuteBuffers->m_payload_list[index].data,
                                   &m_appBufferIds[index]);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        }
    }
    if(pExecuteBuffers->m_scan_list.size() == 1)
    {
        vaSts = vaCreateBuffer(m_vaDisplay, m_vaContextEncode, VAEncSliceParameterBufferType, sizeof(VAEncSliceParameterBufferJPEG), 1, &pExecuteBuffers->m_scan_list[0], &m_scanBufferId);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }
    else
    {
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    //Gpu priortiy
    if(m_MaxContextPriority)
    {
        mfxPriority contextPriority = m_core->GetSession()->m_priority;
        mfxStatus mfxSts = FillPriorityBuffer(contextPriority);
        MFX_CHECK(mfxSts == MFX_ERR_NONE, MFX_ERR_DEVICE_FAILED);
    }

    vaSts = vaRenderPicture(m_vaDisplay, m_vaContextEncode, (VABufferID *)&m_ppsBufferId, 1);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    if(m_qmBufferId != VA_INVALID_ID)
    {
        vaSts = vaRenderPicture(m_vaDisplay, m_vaContextEncode, (VABufferID *)&m_qmBufferId, 1);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }

    if( m_htBufferId != VA_INVALID_ID)
    {
        vaSts = vaRenderPicture(m_vaDisplay, m_vaContextEncode, (VABufferID *)&m_htBufferId, 1);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }

    if(m_appBufferIds.size())
    {
        for( mfxU8 index = 0; index < m_appBufferIds.size(); index++)
        {
            vaSts = vaRenderPicture(m_vaDisplay, m_vaContextEncode, (VABufferID *)&m_appBufferIds[index], 1);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        }
    }

    vaSts = vaRenderPicture(m_vaDisplay, m_vaContextEncode, (VABufferID *)&m_scanBufferId, 1);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    if(m_MaxContextPriority)
    {
        vaSts = vaRenderPicture(m_vaDisplay, m_vaContextEncode, (VABufferID *)&m_priorityBufferId, 1);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }

    vaSts = vaEndPicture(m_vaDisplay, m_vaContextEncode);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    {
        UMC::AutomaticUMCMutex guard(m_guard);

        ExtVASurface currentFeedback = {VA_INVALID_ID, 0, 0, 0};
        currentFeedback.number  = task.m_statusReportNumber;
        currentFeedback.surface = *(VASurfaceID*)surface;
        currentFeedback.idxBs   = task.m_idxBS;
        currentFeedback.size    = 0;

        m_feedbackCache.push_back( currentFeedback );
    }
    return MFX_ERR_NONE;
}

mfxStatus VAAPIEncoder::QueryStatus(DdiTask & task)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_SCHED, "Enc QueryStatus");
    VAStatus vaSts;
    bool isFound = false;
    VASurfaceID waitSurface;
    mfxU32 waitIdxBs;
    mfxU32 waitSize;
    mfxU32 indxSurf;
    UMC::AutomaticUMCMutex guard(m_guard);

    for( indxSurf = 0; indxSurf < m_feedbackCache.size(); indxSurf++ )
    {
        ExtVASurface currentFeedback = m_feedbackCache[ indxSurf ];
        if( currentFeedback.number == task.m_statusReportNumber )
        {
            waitSurface = currentFeedback.surface;
            waitIdxBs   = currentFeedback.idxBs;
            waitSize    = currentFeedback.size;
            isFound  = true;
            break;
        }
    }

    if( !isFound )
    {
        return MFX_ERR_UNKNOWN;
    }

    // find used bitstream
    VABufferID codedBuffer;
    if( waitIdxBs < m_bsQueue.size())
    {
        codedBuffer = m_bsQueue[waitIdxBs].surface;
    }
    else
    {
        return MFX_ERR_UNKNOWN;
    }
    if (waitSurface != VA_INVALID_SURFACE) // Not skipped frame
    {
        VASurfaceStatus surfSts = VASurfaceSkipped;


        m_feedbackCache.erase(m_feedbackCache.begin() + indxSurf);
        guard.Unlock();

        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_SCHED, "Enc vaSyncSurface");
            vaSts = vaSyncSurface(m_vaDisplay, waitSurface);
            // it could happen that decoding error will not be returned after decoder sync
            // and will be returned at subsequent encoder sync instead
            // just ignore VA_STATUS_ERROR_DECODING_ERROR in encoder
            if (vaSts == VA_STATUS_ERROR_DECODING_ERROR)
                vaSts = VA_STATUS_SUCCESS;
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        }
        surfSts = VASurfaceReady;

        switch (surfSts)
        {
            case VASurfaceReady:
                VACodedBufferSegment *codedBufferSegment;

                {
                    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_SCHED, "Enc vaMapBuffer");
                    vaSts = vaMapBuffer(
                        m_vaDisplay,
                        codedBuffer,
                        (void **)(&codedBufferSegment));

                    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
                }
                task.m_bsDataLength = codedBufferSegment->size;

                {
                    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_SCHED, "Enc vaUnmapBuffer");
                    vaSts = vaUnmapBuffer( m_vaDisplay, codedBuffer );
                    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
                }

                return MFX_ERR_NONE;

            case VASurfaceRendering:
            case VASurfaceDisplaying:
                return MFX_WRN_DEVICE_BUSY;

            case VASurfaceSkipped:
            default:
                assert(!"bad feedback status");
                return MFX_ERR_DEVICE_FAILED;
        }
    }
    else
    {
        task.m_bsDataLength = waitSize;
        m_feedbackCache.erase(m_feedbackCache.begin() + indxSurf);
    }

    return MFX_ERR_NONE;
}

mfxStatus VAAPIEncoder::UpdateBitstream(
    mfxMemId       MemId,
    DdiTask      & task)
{
    mfxU8      * bsData    = task.bs->Data + task.bs->DataOffset + task.bs->DataLength;
    mfxSize     roi       = {(int)task.m_bsDataLength, 1};
    mfxFrameData bitstream = { };

    if (task.m_bsDataLength + task.bs->DataOffset + task.bs->DataLength > task.bs->MaxLength)
        return MFX_ERR_NOT_ENOUGH_BUFFER;

    m_core->LockFrame(MemId, &bitstream);
    MFX_CHECK(bitstream.Y != 0, MFX_ERR_LOCK_MEMORY);

    mfxStatus sts = FastCopy::Copy(
        bsData, task.m_bsDataLength,
        (uint8_t *)bitstream.Y, task.m_bsDataLength,
        roi, COPY_VIDEO_TO_SYS);
    assert(sts == MFX_ERR_NONE);

    task.bs->DataLength += task.m_bsDataLength;
    m_core->UnlockFrame(MemId, &bitstream);

    return sts;
}

mfxStatus VAAPIEncoder::DestroyBuffers()
{

    mfxStatus sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_qmBufferId);
    std::ignore = MFX_STS_TRACE(sts);

    sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_htBufferId);
    std::ignore = MFX_STS_TRACE(sts);

    sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_scanBufferId);
    std::ignore = MFX_STS_TRACE(sts);

    sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_ppsBufferId);
    std::ignore = MFX_STS_TRACE(sts);

    if(m_MaxContextPriority)
    {
        sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_priorityBufferId);
        std::ignore = MFX_STS_TRACE(sts);
    }

    for (size_t index = 0; index < m_appBufferIds.size(); index++)
    {
        sts = CheckAndDestroyVAbuffer(m_vaDisplay, m_appBufferIds[index]);
        std::ignore = MFX_STS_TRACE(sts);
    }
    m_appBufferIds.clear();

    return MFX_ERR_NONE;
}

mfxStatus VAAPIEncoder::Destroy()
{
    m_bsQueue.clear();
    m_feedbackCache.clear();
    DestroyBuffers();

    if( m_vaContextEncode )
    {
        VAStatus vaSts = vaDestroyContext( m_vaDisplay, m_vaContextEncode );
        std::ignore = MFX_STS_TRACE(vaSts);
        m_vaContextEncode = 0;
    }

    if( m_vaConfig )
    {
        VAStatus vaSts = vaDestroyConfig( m_vaDisplay, m_vaConfig );
        std::ignore = MFX_STS_TRACE(vaSts);
        m_vaConfig = 0;
    }
    return MFX_ERR_NONE;
}

#endif // #if defined (MFX_ENABLE_MJPEG_VIDEO_ENCODE) && defined (MFX_VA_LINUX)
