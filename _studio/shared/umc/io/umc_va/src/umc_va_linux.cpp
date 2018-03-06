// Copyright (c) 2017-2018 Intel Corporation
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

#include <umc_va_base.h>


#include "umc_defs.h"
#include "umc_va_linux.h"
#include "umc_va_video_processing.h"
#include "vm_file.h"
#include "mfx_trace.h"
#include "umc_frame_allocator.h"
#include "mfxstructures.h"

#define UMC_VA_NUM_OF_COMP_BUFFERS       8
#define UMC_VA_DECODE_STREAM_OUT_ENABLE  2

UMC::Status va_to_umc_res(VAStatus va_res)
{
    UMC::Status umcRes = UMC::UMC_OK;

    switch (va_res)
    {
    case VA_STATUS_SUCCESS:
        umcRes = UMC::UMC_OK;
        break;
    case VA_STATUS_ERROR_ALLOCATION_FAILED:
        umcRes = UMC::UMC_ERR_ALLOC;
        break;
    case VA_STATUS_ERROR_ATTR_NOT_SUPPORTED:
    case VA_STATUS_ERROR_UNSUPPORTED_PROFILE:
    case VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT:
    case VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT:
    case VA_STATUS_ERROR_UNSUPPORTED_BUFFERTYPE:
    case VA_STATUS_ERROR_FLAG_NOT_SUPPORTED:
    case VA_STATUS_ERROR_RESOLUTION_NOT_SUPPORTED:
        umcRes = UMC::UMC_ERR_UNSUPPORTED;
        break;
    case VA_STATUS_ERROR_INVALID_DISPLAY:
    case VA_STATUS_ERROR_INVALID_CONFIG:
    case VA_STATUS_ERROR_INVALID_CONTEXT:
    case VA_STATUS_ERROR_INVALID_SURFACE:
    case VA_STATUS_ERROR_INVALID_BUFFER:
    case VA_STATUS_ERROR_INVALID_IMAGE:
    case VA_STATUS_ERROR_INVALID_SUBPICTURE:
        umcRes = UMC::UMC_ERR_INVALID_PARAMS;
        break;
    case VA_STATUS_ERROR_INVALID_PARAMETER:
        umcRes = UMC::UMC_ERR_INVALID_PARAMS;
        break;
    case VA_STATUS_ERROR_DECODING_ERROR:
        umcRes = UMC::UMC_ERR_DEVICE_FAILED;
        break;
    case VA_STATUS_ERROR_HW_BUSY:
        umcRes = UMC::UMC_ERR_GPU_HANG;
        break;
    default:
        umcRes = UMC::UMC_ERR_FAILED;
        break;
    }
    return umcRes;
}

VAEntrypoint umc_to_va_entrypoint(uint32_t umc_entrypoint)
{
    VAEntrypoint va_entrypoint = (VAEntrypoint)-1;

    switch (umc_entrypoint)
    {
    case UMC::VA_VLD:
    case UMC::VA_VLD | UMC::VA_PROFILE_10:
        va_entrypoint = VAEntrypointVLD;
        break;
    default:
        va_entrypoint = (VAEntrypoint)-1;
        break;
    }
    return va_entrypoint;
}

// profile priorities for codecs
uint32_t g_Profiles[] =
{
    UMC::MPEG2_VLD,
    UMC::H264_VLD,
    UMC::H265_VLD,
    UMC::VC1_VLD,
    UMC::VP8_VLD,
    UMC::VP9_VLD,
    UMC::JPEG_VLD
};

// va profile priorities for different codecs
VAProfile g_Mpeg2Profiles[] =
{
    VAProfileMPEG2Main, VAProfileMPEG2Simple
};

VAProfile g_H264Profiles[] =
{
    VAProfileH264High, VAProfileH264Main, VAProfileH264ConstrainedBaseline
};

VAProfile g_H265Profiles[] =
{
    VAProfileHEVCMain
};

VAProfile g_H26510BitsProfiles[] =
{
    VAProfileHEVCMain10
};

VAProfile g_VC1Profiles[] =
{
    VAProfileVC1Advanced, VAProfileVC1Main, VAProfileVC1Simple
};

VAProfile g_VP8Profiles[] =
{
    VAProfileVP8Version0_3
};

VAProfile g_VP9Profiles[] =
{
    VAProfileVP9Profile0
};

VAProfile g_JPEGProfiles[] =
{
    VAProfileJPEGBaseline
};

VAProfile get_next_va_profile(uint32_t umc_codec, uint32_t profile)
{
    VAProfile va_profile = (VAProfile)-1;

    switch (umc_codec)
    {
    case UMC::VA_MPEG2:
        if (profile < UMC_ARRAY_SIZE(g_Mpeg2Profiles)) va_profile = g_Mpeg2Profiles[profile];
        break;
    case UMC::VA_H264:
        if (profile < UMC_ARRAY_SIZE(g_H264Profiles)) va_profile = g_H264Profiles[profile];
        break;
    case UMC::VA_H265:
        if (profile < UMC_ARRAY_SIZE(g_H265Profiles)) va_profile = g_H265Profiles[profile];
        break;
    case UMC::VA_H265 | UMC::VA_PROFILE_10:
        if (profile < UMC_ARRAY_SIZE(g_H26510BitsProfiles)) va_profile = g_H26510BitsProfiles[profile];
        break;
    case UMC::VA_VC1:
        if (profile < UMC_ARRAY_SIZE(g_VC1Profiles)) va_profile = g_VC1Profiles[profile];
        break;
    case UMC::VA_VP8:
        if (profile < UMC_ARRAY_SIZE(g_VP8Profiles)) va_profile = g_VP8Profiles[profile];
        break;
    case UMC::VA_VP9:
        if (profile < UMC_ARRAY_SIZE(g_VP9Profiles)) va_profile = g_VP9Profiles[profile];
        break;
    case UMC::VA_JPEG:
        if (profile < UMC_ARRAY_SIZE(g_JPEGProfiles)) va_profile = g_JPEGProfiles[profile];
        break;
    default:
        va_profile = (VAProfile)-1;
        break;
    }
    return va_profile;
}

namespace UMC
{

VACompBuffer::VACompBuffer(void)
{
    m_NumOfItem = 0;
    m_index     = -1;
    m_id        = -1;
    m_bDestroy  = false;
}

VACompBuffer::~VACompBuffer(void)
{
}

Status VACompBuffer::SetBufferInfo(int32_t _type, int32_t _id, int32_t _index)
{
    type  = _type;
    m_id    = _id;
    m_index = _index;
    return UMC_OK;
}

Status VACompBuffer::SetDestroyStatus(bool _destroy)
{
    UNREFERENCED_PARAMETER(_destroy);
    m_bDestroy = true;
    return UMC_OK;
}

LinuxVideoAccelerator::LinuxVideoAccelerator(void)
    : m_sDecodeTraceStart("")
    , m_sDecodeTraceEnd("")
{
    m_dpy        = NULL;
    m_pContext   = NULL;
    m_pConfigId  = NULL;
    m_pKeepVAState = NULL;
    m_FrameState = lvaBeforeBegin;

    m_pCompBuffers  = NULL;
    m_NumOfFrameBuffers = 0;
    m_uiCompBuffersNum  = 0;
    m_uiCompBuffersUsed = 0;

    vm_mutex_set_invalid(&m_SyncMutex);

#if defined(ANDROID)
    m_isUseStatuReport  = false;
#else
    m_isUseStatuReport  = true;
#endif

    m_bH264MVCSupport   = false;
    memset(&m_guidDecoder, 0 , sizeof(GUID));
}

LinuxVideoAccelerator::~LinuxVideoAccelerator(void)
{
    Close();
}

Status LinuxVideoAccelerator::Init(VideoAcceleratorParams* pInfo)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "LinuxVideoAccelerator::Init");
    Status         umcRes = UMC_OK;
    VAStatus       va_res = VA_STATUS_SUCCESS;
    VAConfigAttrib va_attributes[4];

    LinuxVideoAcceleratorParams* pParams = DynamicCast<LinuxVideoAcceleratorParams>(pInfo);
    int32_t width = 0, height = 0;
    bool needRecreateContext = false;

    // checking errors in input parameters
    if ((UMC_OK == umcRes) && (NULL == pParams))
        umcRes = UMC_ERR_NULL_PTR;
    if ((UMC_OK == umcRes) && (NULL == pParams->m_pVideoStreamInfo))
        umcRes = UMC_ERR_INVALID_PARAMS;
    if ((UMC_OK == umcRes) && (NULL == pParams->m_Display))
        umcRes = UMC_ERR_INVALID_PARAMS;
    if ((UMC_OK == umcRes) && (NULL == pParams->m_pConfigId))
        umcRes = UMC_ERR_INVALID_PARAMS;
    if ((UMC_OK == umcRes) && (NULL == pParams->m_pContext))
        umcRes = UMC_ERR_INVALID_PARAMS;
    if ((UMC_OK == umcRes) && (NULL == pParams->m_pKeepVAState))
        umcRes = UMC_ERR_INVALID_PARAMS;
    if ((UMC_OK == umcRes) && (NULL == pParams->m_allocator))
        umcRes = UMC_ERR_INVALID_PARAMS;
    if ((UMC_OK == umcRes) && (pParams->m_iNumberSurfaces < 0))
        umcRes = UMC_ERR_INVALID_PARAMS;
    if (UMC_OK == umcRes)
    {
        vm_mutex_set_invalid(&m_SyncMutex);
        umcRes = (Status)vm_mutex_init(&m_SyncMutex);
    }
    // filling input parameters
    if (UMC_OK == umcRes)
    {
        m_dpy               = pParams->m_Display;
        m_pKeepVAState      = pParams->m_pKeepVAState;
        width               = pParams->m_pVideoStreamInfo->clip_info.width;
        height              = pParams->m_pVideoStreamInfo->clip_info.height;
        m_NumOfFrameBuffers = pParams->m_iNumberSurfaces;
        m_allocator         = pParams->m_allocator;
        m_FrameState        = lvaBeforeBegin;


        if (pParams->m_needVideoProcessingVA)
        {
            m_videoProcessingVA = new VideoProcessingVA();
        }

        // profile or stream type should be set
        if (UNKNOWN == (m_Profile & VA_CODEC))
        {
            umcRes = UMC_ERR_INVALID_PARAMS;
        }
    }
    if ((UMC_OK == umcRes) && (UNKNOWN == m_Profile))
        umcRes = UMC_ERR_INVALID_PARAMS;

    bool needAllocatedSurfaces =    (((m_Profile & VA_CODEC) != UMC::VA_H264)
                                  && ((m_Profile & VA_CODEC) != UMC::VA_H265)
                                  && ((m_Profile & VA_CODEC) != UMC::VA_VP8)
                                  && ((m_Profile & VA_CODEC) != UMC::VA_VP9)
                                  && ((m_Profile & VA_CODEC) != UMC::VA_VC1)
                                  && ((m_Profile & VA_CODEC) != UMC::VA_MPEG2)
#ifndef ANDROID
                                  && ((m_Profile & VA_CODEC) != UMC::VA_JPEG)
#endif
                                    );

    SetTraceStrings(m_Profile & VA_CODEC);

    // display initialization
    if (UMC_OK == umcRes)
    {
        int32_t i,j;
        int va_max_num_profiles      = vaMaxNumProfiles   (m_dpy);
        int va_max_num_entrypoints   = vaMaxNumEntrypoints(m_dpy);
        int va_num_profiles          = 0;
        int va_num_entrypoints       = 0;
        VAProfile*    va_profiles    = NULL;
        VAEntrypoint* va_entrypoints = NULL;
        VAProfile     va_profile     = (VAProfile)-1;
        VAEntrypoint  va_entrypoint  = (VAEntrypoint)-1;

        if (UMC_OK == umcRes)
        {
            if ((va_max_num_profiles <= 0) || (va_max_num_entrypoints <= 0))
                umcRes = UMC_ERR_FAILED;
        }
        if (UMC_OK == umcRes)
        {
            va_profiles    = new VAProfile[va_max_num_profiles];
            va_entrypoints = new VAEntrypoint[va_max_num_entrypoints];
            va_res = vaQueryConfigProfiles(m_dpy, va_profiles, &va_num_profiles);
            umcRes = va_to_umc_res(va_res);
        }

        if (UMC_OK == umcRes)
        {
            // checking support of some profile
            for (i = 0; (va_profile = get_next_va_profile(m_Profile & (VA_PROFILE | VA_CODEC), i)) != -1; ++i)
            {
                for (j = 0; j < va_num_profiles; ++j) if (va_profile == va_profiles[j]) break;
                if (j < va_num_profiles) break;
                else
                {
                    va_profile = (VAProfile)-1;
                    continue;
                }
            }
            if (va_profile < 0) umcRes = UMC_ERR_FAILED;

            if ((m_Profile & VA_CODEC) == UMC::VA_VC1)
            {
                if ((VC1_VIDEO_RCV == pInfo->m_pVideoStreamInfo->stream_subtype)
                  || (WMV3_VIDEO == pInfo->m_pVideoStreamInfo->stream_subtype))
                    va_profile = VAProfileVC1Main;
                else
                    va_profile = VAProfileVC1Advanced;
            }
        }
        if (UMC_OK == umcRes)
        {
            va_res = vaQueryConfigEntrypoints(m_dpy, va_profile, va_entrypoints, &va_num_entrypoints);
            umcRes = va_to_umc_res(va_res);
        }
        if (UMC_OK == umcRes)
        {
            uint32_t k = 0, profile = UNKNOWN;

            for (k = 0; k < UMC_ARRAY_SIZE(g_Profiles); ++k)
            {
                if (!(m_Profile & VA_ENTRY_POINT))
                {
                    // entrypoint is not specified, we may chose
                    if (m_Profile != (g_Profiles[k] & VA_CODEC)) continue;
                    profile = g_Profiles[k];
                }
                else
                {
                    // codec and entrypoint are specified, we just need to check validity
                    profile = m_Profile;
                }
                va_entrypoint = umc_to_va_entrypoint(profile & VA_ENTRY_POINT);
                for (i = 0; i < va_num_entrypoints; ++i) if (va_entrypoint == va_entrypoints[i]) break;
                if (i < va_num_entrypoints) break;
                else
                {
                    va_entrypoint = (VAEntrypoint)-1;
                    if (m_Profile == profile) break;
                    else continue;
                }
            }
            m_Profile = (UMC::VideoAccelerationProfile)profile;
            if (va_entrypoint == (VAEntrypoint)-1) umcRes = UMC_ERR_FAILED;
        }
        if (UMC_OK == umcRes)
        {
            int nattr = 0;
            // Assuming finding VLD, find out the format for the render target
            va_attributes[nattr++].type = VAConfigAttribRTFormat;

            va_attributes[nattr].type = VAConfigAttribDecSliceMode;
            va_attributes[nattr].value = VA_DEC_SLICE_MODE_NORMAL;
            nattr++;

            va_attributes[nattr++].type = (VAConfigAttribType)VAConfigAttribDecProcessing;

            va_attributes[nattr++].type = VAConfigAttribEncryption;

            va_res = vaGetConfigAttributes(m_dpy, va_profile, va_entrypoint, va_attributes, nattr);
            umcRes = va_to_umc_res(va_res);
        }

        if (UMC_OK == umcRes && (!(va_attributes[0].value & VA_RT_FORMAT_YUV420)))
            umcRes = UMC_ERR_FAILED;

        if (UMC_OK == umcRes)
        {
            va_attributes[1].value = VA_DEC_SLICE_MODE_NORMAL;

        }

        int32_t attribsNumber = 2;

        if (UMC_OK == umcRes && pParams->m_needVideoProcessingVA)
        {
            if (va_attributes[2].value == VA_DEC_PROCESSING_NONE)
                umcRes = UMC_ERR_FAILED;
            else
                attribsNumber++;
        }


        if (UMC_OK == umcRes)
        {
            m_pConfigId = pParams->m_pConfigId;
            if (*m_pConfigId == VA_INVALID_ID)
            {
                va_res = vaCreateConfig(m_dpy, va_profile, va_entrypoint, va_attributes, attribsNumber, m_pConfigId);
                umcRes = va_to_umc_res(va_res);
                needRecreateContext = true;
            }
        }

        delete[] va_profiles;
        delete[] va_entrypoints;
    }

    // creating context
    if (UMC_OK == umcRes)
    {
        m_pContext = pParams->m_pContext;
        if ((*m_pContext != VA_INVALID_ID) && needRecreateContext)
        {
            vaDestroyContext(m_dpy, *m_pContext);
            *m_pContext = VA_INVALID_ID;
        }
        if (*m_pContext == VA_INVALID_ID)
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaCreateContext");

            if (needAllocatedSurfaces)
            {
                VM_ASSERT(pParams->m_surf && "render targets tied to the context shoul not be NULL");
                va_res = vaCreateContext(m_dpy, *m_pConfigId, width, height, pParams->m_CreateFlags, (VASurfaceID*)pParams->m_surf, m_NumOfFrameBuffers, m_pContext);
            }
            else
            {
                VM_ASSERT(!pParams->m_surf && "render targets tied to the context shoul be NULL");
                va_res = vaCreateContext(m_dpy, *m_pConfigId, width, height, pParams->m_CreateFlags, NULL, 0, m_pContext);
            }

            umcRes = va_to_umc_res(va_res);
        }
    }

    return umcRes;
}

Status LinuxVideoAccelerator::Close(void)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "LinuxVideoAccelerator::Close");
    VACompBuffer* pCompBuf = NULL;
    uint32_t i;

    if (NULL != m_pCompBuffers)
    {
        for (i = 0; i < m_uiCompBuffersUsed; ++i)
        {
            pCompBuf = m_pCompBuffers[i];
            if (pCompBuf->NeedDestroy() && (NULL != m_dpy))
            {
                vaDestroyBuffer(m_dpy, pCompBuf->GetID());
            }
            UMC_DELETE(m_pCompBuffers[i]);
        }
        delete[] m_pCompBuffers;
        m_pCompBuffers = 0;
    }
    if (NULL != m_dpy)
    {
        if ((m_pContext && (*m_pContext != VA_INVALID_ID)) && !(m_pKeepVAState && *m_pKeepVAState))
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaDestroyContext");
            vaDestroyContext(m_dpy, *m_pContext);
            *m_pContext = VA_INVALID_ID;
        }
        if ((m_pConfigId && (*m_pConfigId != VA_INVALID_ID)) && !(m_pKeepVAState && *m_pKeepVAState))
        {
            vaDestroyConfig(m_dpy, *m_pConfigId);
            *m_pConfigId = VA_INVALID_ID;
        }

        m_dpy = NULL;
    }


    delete m_videoProcessingVA;
    m_videoProcessingVA = 0;

    m_FrameState = lvaBeforeBegin;
    m_uiCompBuffersNum  = 0;
    m_uiCompBuffersUsed = 0;
    vm_mutex_unlock (&m_SyncMutex);
    vm_mutex_destroy(&m_SyncMutex);

    return VideoAccelerator::Close();
}

Status LinuxVideoAccelerator::BeginFrame(int32_t FrameBufIndex)
{
    Status   umcRes = UMC_OK;
    VAStatus va_res = VA_STATUS_SUCCESS;

    if ((UMC_OK == umcRes) && ((FrameBufIndex < 0) || (FrameBufIndex >= m_NumOfFrameBuffers)))
        umcRes = UMC_ERR_INVALID_PARAMS;

    VASurfaceID *surface;
    Status sts = m_allocator->GetFrameHandle(FrameBufIndex, &surface);
    if (sts != UMC_OK)
        return sts;

    if (UMC_OK == umcRes)
    {
        if (lvaBeforeBegin == m_FrameState)
        {
            {
                MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaBeginPicture");
                MFX_LTRACE_2(MFX_TRACE_LEVEL_EXTCALL, m_sDecodeTraceStart, "%d|%d", *m_pContext, 0);
                va_res = vaBeginPicture(m_dpy, *m_pContext, *surface);
            }
            umcRes = va_to_umc_res(va_res);
            if (UMC_OK == umcRes) m_FrameState = lvaBeforeEnd;
        }
    }
    return umcRes;
}

Status LinuxVideoAccelerator::AllocCompBuffers(void)
{
    Status umcRes = UMC_OK;

    if ((UMC_OK == umcRes) && (m_uiCompBuffersUsed >= m_uiCompBuffersNum))
    {
        if (NULL == m_pCompBuffers)
        {
            m_uiCompBuffersNum = UMC_VA_NUM_OF_COMP_BUFFERS;
            m_pCompBuffers = new VACompBuffer*[m_uiCompBuffersNum];
        }
        else
        {
            uint32_t uiNewCompBuffersNum = 0;
            VACompBuffer** pNewCompBuffers = NULL;

            uiNewCompBuffersNum = m_uiCompBuffersNum + UMC_VA_NUM_OF_COMP_BUFFERS;
            pNewCompBuffers = new VACompBuffer*[uiNewCompBuffersNum];

            MFX_INTERNAL_CPY((uint8_t*)pNewCompBuffers, (const uint8_t*)m_pCompBuffers, m_uiCompBuffersNum*sizeof(VACompBuffer*));
            delete[] m_pCompBuffers;
            m_uiCompBuffersNum = uiNewCompBuffersNum;
            m_pCompBuffers = pNewCompBuffers;
        }
    }
    return umcRes;
}

void* LinuxVideoAccelerator::GetCompBuffer(int32_t buffer_type, UMCVACompBuffer **buf, int32_t size, int32_t index)
{
    uint32_t i;
    VACompBuffer* pCompBuf = NULL;
    void* pBufferPointer = NULL;

    if (NULL != buf) *buf = NULL;

    vm_mutex_lock(&m_SyncMutex);
    for (i = 0; i < m_uiCompBuffersUsed; ++i)
    {
        pCompBuf = m_pCompBuffers[i];
        if ((pCompBuf->GetType() == buffer_type) && (pCompBuf->GetIndex() == index)) break;
    }
    if (i >= m_uiCompBuffersUsed)
    {
        AllocCompBuffers();
        pCompBuf = GetCompBufferHW(buffer_type, size, index);
        if (NULL != pCompBuf)
        {
            m_pCompBuffers[m_uiCompBuffersUsed] = pCompBuf;
            ++m_uiCompBuffersUsed;
        }
    }
    if (NULL != pCompBuf)
    {
        if (NULL != buf) *buf = pCompBuf;
        pBufferPointer = pCompBuf->GetPtr();
    }
    vm_mutex_unlock(&m_SyncMutex);
    return pBufferPointer;
}

VACompBuffer* LinuxVideoAccelerator::GetCompBufferHW(int32_t type, int32_t size, int32_t index)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_INTERNAL, "GetCompBufferHW");
    VAStatus   va_res = VA_STATUS_SUCCESS;
    VABufferID id;
    uint8_t*      buffer = NULL;
    uint32_t     buffer_size = 0;
    VACompBuffer* pCompBuffer = NULL;

    if (VA_STATUS_SUCCESS == va_res)
    {
        VABufferType va_type         = (VABufferType)type;
        unsigned int va_size         = 0;
        unsigned int va_num_elements = 0;

        if (VASliceParameterBufferType == va_type)
        {
            switch (m_Profile & VA_CODEC)
            {
            case UMC::VA_MPEG2:
                va_size         = sizeof(VASliceParameterBufferMPEG2);
                va_num_elements = size/sizeof(VASliceParameterBufferMPEG2);
                break;
            case UMC::VA_H264:
                if (m_bH264ShortSlice)
                {
                    va_size         = sizeof(VASliceParameterBufferBase);
                    va_num_elements = size/sizeof(VASliceParameterBufferBase);
                }
                else
                {
                    va_size         = sizeof(VASliceParameterBufferH264);
                    va_num_elements = size/sizeof(VASliceParameterBufferH264);
                }
                break;
            case UMC::VA_VC1:
                va_size         = sizeof(VASliceParameterBufferVC1);
                va_num_elements = size/sizeof(VASliceParameterBufferVC1);
                break;
            case UMC::VA_VP8:
                va_size         = sizeof(VASliceParameterBufferVP8);
                va_num_elements = size/sizeof(VASliceParameterBufferVP8);
                break;
            case UMC::VA_JPEG:
                va_size         = sizeof(VASliceParameterBufferJPEGBaseline);
                va_num_elements = size/sizeof(VASliceParameterBufferJPEGBaseline);
                break;
            case UMC::VA_VP9:
                va_size         = sizeof(VASliceParameterBufferVP9);
                va_num_elements = size/sizeof(VASliceParameterBufferVP9);
                break;
            case UMC::VA_H265:
                va_size         = sizeof(VASliceParameterBufferHEVC);
                va_num_elements = size/sizeof(VASliceParameterBufferHEVC);
                break;
            default:
                va_size         = 0;
                va_num_elements = 0;
                break;
            }
        }
        else
        {
            va_size         = size;
            va_num_elements = 1;
        }
        buffer_size = va_size * va_num_elements;

        va_res = vaCreateBuffer(m_dpy, *m_pContext, va_type, va_size, va_num_elements, NULL, &id);
    }
    if (VA_STATUS_SUCCESS == va_res)
    {
        va_res = vaMapBuffer(m_dpy, id, (void**)&buffer);
    }
    if (VA_STATUS_SUCCESS == va_res)
    {
        pCompBuffer = new VACompBuffer();
        pCompBuffer->SetBufferPointer(buffer, buffer_size);
        pCompBuffer->SetDataSize(0);
        pCompBuffer->SetBufferInfo(type, id, index);
        pCompBuffer->SetDestroyStatus(true);
    }
    return pCompBuffer;
}

Status
LinuxVideoAccelerator::Execute()
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_INTERNAL, "Execute");
    Status         umcRes = UMC_OK;
    VAStatus       va_res = VA_STATUS_SUCCESS;
    VAStatus       va_sts = VA_STATUS_SUCCESS;
    VABufferID     id;
    uint32_t         i;
    VACompBuffer*  pCompBuf = NULL;

    vm_mutex_lock(&m_SyncMutex);

    if (UMC_OK == umcRes)
    {
        for (i = 0; i < m_uiCompBuffersUsed; i++)
        {
            pCompBuf = m_pCompBuffers[i];
            id = pCompBuf->GetID();

            if (!m_bH264ShortSlice)
            {
                if (pCompBuf->GetType() == VASliceParameterBufferType)
                {
                    va_sts = vaBufferSetNumElements(m_dpy, id, pCompBuf->GetNumOfItem());
                    if (VA_STATUS_SUCCESS == va_res) va_res = va_sts;
                }
            }

            va_sts = vaUnmapBuffer(m_dpy, id);
            if (VA_STATUS_SUCCESS == va_res) va_res = va_sts;


            {
                MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaRenderPicture");
                va_sts = vaRenderPicture(m_dpy, *m_pContext, &id, 1); // TODO: send all at once?
                if (VA_STATUS_SUCCESS == va_res) va_res = va_sts;
            }
        }
    }

    vm_mutex_unlock(&m_SyncMutex);
    if (UMC_OK == umcRes)
    {
        umcRes = va_to_umc_res(va_res);
    }
    return umcRes;
}

Status LinuxVideoAccelerator::EndFrame(void*)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_INTERNAL, "EndFrame");
    VAStatus va_res = VA_STATUS_SUCCESS;

    vm_mutex_lock(&m_SyncMutex);

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaEndPicture");
        va_res = vaEndPicture(m_dpy, *m_pContext);
        MFX_LTRACE_2(MFX_TRACE_LEVEL_EXTCALL, m_sDecodeTraceEnd, "%d|%d", *m_pContext, 0);
    }

    m_FrameState = lvaBeforeBegin;

    VACompBuffer* pCompBuf = NULL;
    for (uint32_t i = 0; i < m_uiCompBuffersUsed; ++i)
    {
        pCompBuf = m_pCompBuffers[i];
        if (pCompBuf->NeedDestroy())
        {
            VAStatus va_sts = vaDestroyBuffer(m_dpy, pCompBuf->GetID());
            if (VA_STATUS_SUCCESS == va_res)
                va_res = va_sts;
        }
        UMC_DELETE(pCompBuf);
    }
    m_uiCompBuffersUsed = 0;

    vm_mutex_unlock(&m_SyncMutex);
    return va_to_umc_res(va_res);
}

/* TODO: need to rewrite return value type (possible problems with signed/unsigned) */
int32_t LinuxVideoAccelerator::GetSurfaceID(int32_t idx)
{
    VASurfaceID *surface;
    Status sts = m_allocator->GetFrameHandle(idx, &surface);
    if (sts != UMC_OK)
        return VA_INVALID_SURFACE;

    return *surface;
}

uint16_t LinuxVideoAccelerator::GetDecodingError()
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "GetDecodingError");
    uint16_t error = 0;

#ifndef ANDROID
    // NOTE: at the moment there is no such support for Android, so no need to execute...
    VAStatus va_sts;

    // TODO: to reduce number of checks we can cache all used render targets
    // during vaBeginPicture() call. For now check all render targets binded to context
    for(int cnt = 0; cnt < m_NumOfFrameBuffers; ++cnt)
    {
        VASurfaceDecodeMBErrors* pVaDecErr = NULL;
        VASurfaceID *surface;
        Status sts = m_allocator->GetFrameHandle(cnt, &surface);
        if (sts != UMC_OK)
            return sts;

        va_sts = vaQuerySurfaceError(m_dpy, *surface, VA_STATUS_ERROR_DECODING_ERROR, (void**)&pVaDecErr);

        if (VA_STATUS_SUCCESS == va_sts)
        {
            if (NULL != pVaDecErr)
            {
                for (int i = 0; pVaDecErr[i].status != -1; ++i)
                {
                    error = MFX_CORRUPTION_MAJOR;
                }
            }
            else
            {
                error = MFX_CORRUPTION_MAJOR;
            }
        }
    }
#endif
    return error;
}


void LinuxVideoAccelerator::SetTraceStrings(uint32_t umc_codec)
{
    switch (umc_codec)
    {
    case UMC::VA_MPEG2:
        m_sDecodeTraceStart = "A|DECODE|MPEG2|PACKET_START|";
        m_sDecodeTraceEnd = "A|DECODE|MPEG2|PACKET_END|";
        break;
    case UMC::VA_H264:
        m_sDecodeTraceStart = "A|DECODE|H264|PACKET_START|";
        m_sDecodeTraceEnd = "A|DECODE|H264|PACKET_END|";
        break;
    case UMC::VA_H265:
        m_sDecodeTraceStart = "A|DECODE|H265|PACKET_START|";
        m_sDecodeTraceEnd = "A|DECODE|H265|PACKET_END|";
        break;
    case UMC::VA_VC1:
        m_sDecodeTraceStart = "A|DECODE|VC1|PACKET_START|";
        m_sDecodeTraceEnd = "A|DECODE|VC1|PACKET_END|";
        break;
    case UMC::VA_VP8:
        m_sDecodeTraceStart = "A|DECODE|VP8|PACKET_START|";
        m_sDecodeTraceEnd = "A|DECODE|VP8|PACKET_END|";
        break;
    case UMC::VA_VP9:
        m_sDecodeTraceStart = "A|DECODE|VP9|PACKET_START|";
        m_sDecodeTraceEnd = "A|DECODE|VP9|PACKET_END|";
        break;
    case UMC::VA_JPEG:
        m_sDecodeTraceStart = "A|DECODE|JPEG|PACKET_START|";
        m_sDecodeTraceEnd = "A|DECODE|JPEG|PACKET_END|";
        break;
    default:
        m_sDecodeTraceStart = "";
        m_sDecodeTraceEnd = "";
        break;
    }
}

Status LinuxVideoAccelerator::QueryTaskStatus(int32_t FrameBufIndex, void * status, void * error)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "QueryTaskStatus");
    if ((FrameBufIndex < 0) || (FrameBufIndex >= m_NumOfFrameBuffers))
        return UMC_ERR_INVALID_PARAMS;

    VASurfaceID *surface;
    Status sts = m_allocator->GetFrameHandle(FrameBufIndex, &surface);
    if (sts != UMC_OK)
        return sts;

    VASurfaceStatus surface_status;

    VAStatus va_status = vaQuerySurfaceStatus(m_dpy, *surface, &surface_status);
    VAStatus va_sts;
    if ((VA_STATUS_SUCCESS == va_status) && (VASurfaceReady == surface_status))
    {
        // handle decoding errors
        va_sts = vaSyncSurface(m_dpy, *surface);

        if (error)
        {
            switch (va_sts)
            {
                case VA_STATUS_ERROR_DECODING_ERROR:
                    *(uint16_t*)error = GetDecodingError();
                    break;

                case VA_STATUS_ERROR_HW_BUSY:
                    va_status = VA_STATUS_ERROR_HW_BUSY;
                    break;
            }
        }
    }

    if (NULL != status)
    {
        *(VASurfaceStatus*)status = surface_status; // done or not
    }

    Status umcRes = va_to_umc_res(va_status); // OK or not

    return umcRes;
}

Status LinuxVideoAccelerator::SyncTask(int32_t FrameBufIndex, void *surfCorruption)
{
    if ((FrameBufIndex < 0) || (FrameBufIndex >= m_NumOfFrameBuffers))
        return UMC_ERR_INVALID_PARAMS;

    VASurfaceID *surface;
    Status umcRes = m_allocator->GetFrameHandle(FrameBufIndex, &surface);
    if (umcRes != UMC_OK)
        return umcRes;

    VAStatus va_sts = VA_STATUS_SUCCESS;
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaSyncSurface");
        va_sts = vaSyncSurface(m_dpy, *surface);
    }
    if (VA_STATUS_ERROR_DECODING_ERROR == va_sts)
    {
        if (surfCorruption) *(uint16_t*)surfCorruption = GetDecodingError();
        return UMC_OK;
    }
    if (VA_STATUS_ERROR_OPERATION_FAILED == va_sts)
    {
        if (surfCorruption) *(uint16_t*)surfCorruption = MFX_CORRUPTION_MAJOR;
        return UMC_OK;
    }
    return va_to_umc_res(va_sts);
}

}; // namespace UMC

