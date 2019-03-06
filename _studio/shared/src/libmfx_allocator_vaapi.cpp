// Copyright (c) 2017-2019 Intel Corporation
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

#if defined (MFX_VA_LINUX)

#include <algorithm>
#include <vector>

#include "libmfx_allocator_vaapi.h"
#include "mfx_utils.h"
#include "mfx_ext_buffers.h"

#define VA_SAFE_CALL(__CALL)        \
{                                   \
    VAStatus va_sts = __CALL;       \
    if (VA_STATUS_SUCCESS != va_sts) return MFX_ERR_INVALID_HANDLE; \
}

#define VA_TO_MFX_STATUS(_va_res) \
    (VA_STATUS_SUCCESS == (_va_res))? MFX_ERR_NONE: MFX_ERR_DEVICE_FAILED;

enum {
    MFX_FOURCC_VP8_NV12    = MFX_MAKEFOURCC('V','P','8','N'),
    MFX_FOURCC_VP8_MBDATA  = MFX_MAKEFOURCC('V','P','8','M'),
    MFX_FOURCC_VP8_SEGMAP  = MFX_MAKEFOURCC('V','P','8','S'),
};

unsigned int ConvertVP8FourccToMfxFourcc(mfxU32 fourcc)
{
    switch (fourcc)
    {
    case MFX_FOURCC_VP8_NV12:
    case MFX_FOURCC_VP8_MBDATA:
        return MFX_FOURCC_NV12;
    case MFX_FOURCC_VP8_SEGMAP:
        return MFX_FOURCC_P8;

    default:
        return fourcc;
    }
}

unsigned int ConvertMfxFourccToVAFormat(mfxU32 fourcc)
{
    switch (fourcc)
    {
    case MFX_FOURCC_NV12:
        return VA_FOURCC_NV12;
    case MFX_FOURCC_YUY2:
        return VA_FOURCC_YUY2;
    case MFX_FOURCC_YV12:
        return VA_FOURCC_YV12;
    case MFX_FOURCC_AYUV:
        return VA_FOURCC_AYUV;
#if defined (MFX_ENABLE_FOURCC_RGB565)
    case MFX_FOURCC_RGB565:
        return VA_FOURCC_RGB565;
#endif
    case MFX_FOURCC_RGB4:
        return VA_FOURCC_ARGB;
    case MFX_FOURCC_BGR4:
        return VA_FOURCC_ABGR;
#ifdef MFX_ENABLE_RGBP
    case MFX_FOURCC_RGBP:
        return VA_FOURCC_RGBP;
#endif
    case MFX_FOURCC_P8:
        return VA_FOURCC_P208;
    case MFX_FOURCC_UYVY:
        return VA_FOURCC_UYVY;
    case MFX_FOURCC_P010:
        return VA_FOURCC_P010;
#if (MFX_VERSION >= 1027)
    case MFX_FOURCC_Y210:
         return VA_FOURCC_Y210;
    case MFX_FOURCC_Y410:
        return VA_FOURCC_Y410;
#endif
    default:
        VM_ASSERT(!"unsupported fourcc");
        return 0;
    }
}

static void FillSurfaceAttrs(std::vector<VASurfaceAttrib> &attrib, unsigned int &format,  const mfxU32 fourcc, const mfxU32 va_fourcc, const mfxU32 memType)
{
    attrib.clear();
    attrib.reserve(2);

    attrib.resize(attrib.size()+1);
    attrib[0].type            = VASurfaceAttribPixelFormat;
    attrib[0].flags           = VA_SURFACE_ATTRIB_SETTABLE;
    attrib[0].value.type      = VAGenericValueTypeInteger;
    attrib[0].value.value.i   = va_fourcc;

    format = va_fourcc; // ???

    switch (fourcc)
    {
    case MFX_FOURCC_VP8_NV12:
            // special configuration for NV12 surf allocation for VP8 hybrid encoder is required
            attrib.resize(attrib.size()+1);
            attrib[1].type            = VASurfaceAttribUsageHint;
            attrib[1].flags           = VA_SURFACE_ATTRIB_SETTABLE;
            attrib[1].value.type      = VAGenericValueTypeInteger;
            attrib[1].value.value.i   = VA_SURFACE_ATTRIB_USAGE_HINT_ENCODER;
            break;
        case MFX_FOURCC_VP8_MBDATA:
            // special configuration for MB data surf allocation for VP8 hybrid encoder is required
            attrib[0].value.value.i  = VA_FOURCC_P208;
            format                   = VA_FOURCC_P208;
            break;
#if VA_CHECK_VERSION(1,2,0)
        case MFX_FOURCC_P010:
            format = VA_RT_FORMAT_YUV420_10;
            break;
#endif
        case MFX_FOURCC_NV12:
            format = VA_RT_FORMAT_YUV420;
            break;
        case MFX_FOURCC_UYVY:
        case MFX_FOURCC_YUY2:
            format = VA_RT_FORMAT_YUV422;
            break;
            break;
        case MFX_FOURCC_A2RGB10:
            format = VA_RT_FORMAT_RGB32_10BPP;
            break;
        case MFX_FOURCC_RGBP:
            format = VA_RT_FORMAT_RGBP;
            break;
        case MFX_FOURCC_RGB4:
        case MFX_FOURCC_BGR4:
            format = VA_RT_FORMAT_RGB32;
            //  Enable this hint as required for creating RGB32 surface for MJPEG.
            if ((memType & MFX_MEMTYPE_VIDEO_MEMORY_ENCODER_TARGET)
                    && (memType & MFX_MEMTYPE_FROM_ENCODE))
            {
                //  Input Attribute Usage Hint
                attrib.resize(attrib.size()+1);
                attrib[1].flags           = VA_SURFACE_ATTRIB_SETTABLE;
                attrib[1].type            = VASurfaceAttribUsageHint;
                attrib[1].value.type      = VAGenericValueTypeInteger;
                attrib[1].value.value.i   = VA_SURFACE_ATTRIB_USAGE_HINT_ENCODER;
            }
            break;
        default:
            break;
    }
}

static bool isFourCCSupported(unsigned int va_fourcc)
{
    switch (va_fourcc)
    {
        case VA_FOURCC_NV12:
        case VA_FOURCC_YV12:
        case VA_FOURCC_YUY2:
        case VA_FOURCC_ARGB:
        case VA_FOURCC_ABGR:
#ifdef MFX_ENABLE_RGBP
        case VA_FOURCC_RGBP:
#endif
        case VA_FOURCC_UYVY:
        case VA_FOURCC_P208:
        case VA_FOURCC_P010:
        case VA_FOURCC_AYUV:
#if defined (MFX_ENABLE_FOURCC_RGB565)
        case VA_FOURCC_RGB565:
#endif
#if (MFX_VERSION >= 1027)
        case VA_FOURCC_Y210:
        case VA_FOURCC_Y410:
#endif
            return true;
        default:
            return false;
    }
}

static mfxStatus ReallocImpl(VADisplay* vaDisp, vaapiMemIdInt *vaapi_mid, mfxFrameSurface1 *surf)
{
    VAStatus  va_res  = VA_STATUS_SUCCESS;
    unsigned int va_fourcc = 0;

    MFX_CHECK_NULL_PTR3(vaDisp, vaapi_mid, surf);
    MFX_CHECK_NULL_PTR1(vaapi_mid->m_surface);

    mfxU32 fourcc = surf->Info.FourCC;

    // VP8 hybrid driver has weird requirements for allocation of surfaces/buffers for VP8 encoding
    // to comply with them additional logic is required to support regular and VP8 hybrid allocation pathes
    mfxU32 mfx_fourcc = ConvertVP8FourccToMfxFourcc(fourcc);
    va_fourcc = ConvertMfxFourccToVAFormat(mfx_fourcc);

    if (!isFourCCSupported(va_fourcc))
    {
        return MFX_ERR_UNSUPPORTED;
    }

    VASurfaceID surfaces[1] = { *vaapi_mid->m_surface };

    if (MFX_FOURCC_P8 == vaapi_mid->m_fourcc)
    {
        mfxStatus sts = CheckAndDestroyVAbuffer(vaDisp, surfaces[0]);
        MFX_CHECK(sts == MFX_ERR_NONE, MFX_ERR_MEMORY_ALLOC);
    }
    else
    {
        va_res = vaDestroySurfaces(vaDisp, surfaces, 1);
        MFX_CHECK(va_res == VA_STATUS_SUCCESS, MFX_ERR_MEMORY_ALLOC);
    }

    *vaapi_mid->m_surface = VA_INVALID_ID;

    std::vector<VASurfaceAttrib> attrib;
    unsigned int format;
    FillSurfaceAttrs(attrib, format,  fourcc, va_fourcc, 0);

    va_res = vaCreateSurfaces(vaDisp,
                            format,
                            surf->Info.Width, surf->Info.Height,
                            surfaces,
                            1,
                            attrib.data(), attrib.size());

    if (VA_STATUS_SUCCESS != va_res)
    {
        return MFX_ERR_DEVICE_FAILED;
    }

    // update vaapi surface ID in the m_frameHandles list element
    *vaapi_mid->m_surface = surfaces[0];
    vaapi_mid->m_fourcc = fourcc;

    return MFX_ERR_NONE;
}

// aka AllocImpl(surface)
mfxStatus mfxDefaultAllocatorVAAPI::ReallocFrameHW(mfxHDL pthis, mfxFrameSurface1 *surf, VASurfaceID *va_surf)
{
    MFX_CHECK_NULL_PTR3(pthis, surf, va_surf);

    mfxWideHWFrameAllocator *pSelf = reinterpret_cast<mfxWideHWFrameAllocator*>(pthis);

    for (mfxU32 i=0; i<pSelf->NumFrames; ++i)
    {
        vaapiMemIdInt *vaapi_mid = reinterpret_cast<vaapiMemIdInt *>(pSelf->m_frameHandles[i]);
        if (vaapi_mid) {
            if (*vaapi_mid->m_surface == *va_surf)
            {
                return ReallocImpl(pSelf->pVADisplay, vaapi_mid, surf);
            }
        }
    }
    return MFX_ERR_MEMORY_ALLOC;
}

mfxStatus
mfxDefaultAllocatorVAAPI::AllocFramesHW(
    mfxHDL                  pthis,
    mfxFrameAllocRequest*   request,
    mfxFrameAllocResponse*  response)
{
    MFX_CHECK_NULL_PTR2(request, response);
    MFX_CHECK(pthis, MFX_ERR_INVALID_HANDLE);
    MFX_CHECK(request->NumFrameSuggested, MFX_ERR_MEMORY_ALLOC);

    *response = {};

    // VP8/VP9 driver has weird requirements for allocation of surfaces/buffers for VP8/VP9 encoding
    // to comply with them additional logic is required to support regular and VP8/VP9 allocation pathes
    mfxU32 mfx_fourcc = ConvertVP8FourccToMfxFourcc(request->Info.FourCC);
    unsigned int va_fourcc = ConvertMfxFourccToVAFormat(mfx_fourcc);
    MFX_CHECK(isFourCCSupported(va_fourcc), MFX_ERR_UNSUPPORTED);


    auto pSelf = reinterpret_cast<mfxWideHWFrameAllocator*>(pthis);

    // Enough frames were allocated previously. Return existing frames
    if (pSelf->NumFrames)
    {
        MFX_CHECK(request->NumFrameSuggested <= pSelf->NumFrames, MFX_ERR_MEMORY_ALLOC);

        response->mids           = pSelf->m_frameHandles.data();
        response->NumFrameActual = request->NumFrameSuggested;
        response->AllocId        = request->AllocId;

        return MFX_ERR_NONE;
    }

    // Use temporary storage for preliminary operations. If some of them fail, current state of allocator remain unchanged.
    // When allocation will be finished, just move content of these vectors to internal allocator storage
    std::vector<VASurfaceID>   AllocatedSurfaces(request->NumFrameSuggested, VA_INVALID_ID);
    std::vector<vaapiMemIdInt> AllocatedMids(request->NumFrameSuggested);

    VAStatus  va_res  = VA_STATUS_SUCCESS;
    mfxStatus mfx_res = MFX_ERR_NONE;

    if( VA_FOURCC_P208 != va_fourcc)
    {
        unsigned int format;
        std::vector<VASurfaceAttrib> attrib;
        FillSurfaceAttrs(attrib, format, request->Info.FourCC, va_fourcc, request->Type);

        va_res = vaCreateSurfaces(pSelf->pVADisplay,
                            format,
                            request->Info.Width, request->Info.Height,
                            AllocatedSurfaces.data(),
                            AllocatedSurfaces.size(),
                            attrib.data(),
                            attrib.size());

        MFX_CHECK(va_res == VA_STATUS_SUCCESS, VA_TO_MFX_STATUS(va_res));
    }
    else
    {
        mfxU32 codedbuf_size, codedbuf_num;
        VABufferType codedbuf_type;

        if (request->Info.FourCC == MFX_FOURCC_VP8_SEGMAP)
        {
            codedbuf_size = request->Info.Width;
            codedbuf_num  = request->Info.Height;
            codedbuf_type = VAEncMacroblockMapBufferType;
        }
        else
        {
            int aligned_width  = mfx::align2_value(request->Info.Width,  32);
            int aligned_height = mfx::align2_value(request->Info.Height, 32);
            codedbuf_size = static_cast<mfxU32>((aligned_width * aligned_height) * 400LL / (16 * 16));

            codedbuf_num  = 1;
            codedbuf_type = VAEncCodedBufferType;
        }

        for (VABufferID& coded_buf : AllocatedSurfaces)
        {
            va_res = vaCreateBuffer(pSelf->pVADisplay,
                      VAContextID(request->AllocId),
                      codedbuf_type,
                      codedbuf_size,
                      codedbuf_num,
                      NULL,
                      &coded_buf);

            mfx_res = VA_TO_MFX_STATUS(va_res);

            if (mfx_res != MFX_ERR_NONE)
                break;
        }
    }

    if (MFX_ERR_NONE == mfx_res)
    {
        // Clean up existing state
        pSelf->NumFrames = 0;
        pSelf->m_frameHandles.clear();
        pSelf->m_frameHandles.reserve(request->NumFrameSuggested);

        // Push new frames
        for (mfxU32 i = 0; i < request->NumFrameSuggested; ++i)
        { 
            AllocatedMids[i].m_surface = &AllocatedSurfaces[i];
            AllocatedMids[i].m_fourcc  = request->Info.FourCC;

            pSelf->m_frameHandles.push_back(&AllocatedMids[i]);
        }
        response->mids           = pSelf->m_frameHandles.data();
        response->NumFrameActual = request->NumFrameSuggested;
        response->AllocId        = request->AllocId;

        pSelf->NumFrames = pSelf->m_frameHandles.size();

        // Save new frames in internal state
        pSelf->m_allocatedSurfaces = std::move(AllocatedSurfaces);
        pSelf->m_allocatedMids     = std::move(AllocatedMids);
    }
    else
    {
        // Some of vaCreateBuffer failed
        for (VABufferID& coded_buf : AllocatedSurfaces)
        {
            mfxStatus sts = CheckAndDestroyVAbuffer(pSelf->pVADisplay, coded_buf);
            MFX_CHECK_STS(sts);
        }
    }

    return mfx_res;
}

mfxStatus mfxDefaultAllocatorVAAPI::FreeFramesHW(
    mfxHDL                  pthis,
    mfxFrameAllocResponse*  response)
{
    MFX_CHECK(pthis, MFX_ERR_INVALID_HANDLE);
    MFX_CHECK_NULL_PTR1(response);

    if (response->mids)
    {
        auto vaapi_mids = reinterpret_cast<vaapiMemIdInt*>(response->mids[0]);
        MFX_CHECK_NULL_PTR1(vaapi_mids);
        MFX_CHECK_NULL_PTR1(vaapi_mids->m_surface);

        auto pSelf = reinterpret_cast<mfxWideHWFrameAllocator*>(pthis);
        // Make sure that we are asked to clean memory which was allocated by current allocator
        MFX_CHECK(pSelf->m_allocatedSurfaces.data() == vaapi_mids->m_surface, MFX_ERR_UNDEFINED_BEHAVIOR);

        if (ConvertVP8FourccToMfxFourcc(vaapi_mids->m_fourcc) == MFX_FOURCC_P8)
        {
            for (VABufferID& coded_buf : pSelf->m_allocatedSurfaces)
            {
                mfxStatus sts = CheckAndDestroyVAbuffer(pSelf->pVADisplay, coded_buf);
                MFX_CHECK_STS(sts);
            }
        }
        else
        {
            // Not Buffered memory
            VAStatus vaSts = vaDestroySurfaces(pSelf->pVADisplay, vaapi_mids->m_surface, response->NumFrameActual);
            MFX_CHECK(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        }
        response->mids = nullptr;

        // Reset internal state
        pSelf->NumFrames = 0;
        pSelf->m_frameHandles.clear();
        pSelf->m_allocatedSurfaces.clear();
        pSelf->m_allocatedMids.clear();
    }
    response->NumFrameActual = 0;

    return MFX_ERR_NONE;
}

mfxStatus mfxDefaultAllocatorVAAPI::SetFrameData(const VAImage &va_image, mfxU32 mfx_fourcc, mfxU8* pBuffer, mfxFrameData*  ptr)
{
    mfxStatus mfx_res = MFX_ERR_NONE;

    switch (va_image.format.fourcc)
    {
    case VA_FOURCC_NV12:
        if (mfx_fourcc != va_image.format.fourcc) return MFX_ERR_LOCK_MEMORY;

        {
            ptr->Y = pBuffer + va_image.offsets[0];
            ptr->U = pBuffer + va_image.offsets[1];
            ptr->V = ptr->U + 1;
        }
        break;

    case VA_FOURCC_YV12:
        if (mfx_fourcc != va_image.format.fourcc) return MFX_ERR_LOCK_MEMORY;

        {
            ptr->Y = pBuffer + va_image.offsets[0];
            ptr->V = pBuffer + va_image.offsets[1];
            ptr->U = pBuffer + va_image.offsets[2];
        }
        break;

    case VA_FOURCC_YUY2:
        if (mfx_fourcc != va_image.format.fourcc) return MFX_ERR_LOCK_MEMORY;

        {
            ptr->Y = pBuffer + va_image.offsets[0];
            ptr->U = ptr->Y + 1;
            ptr->V = ptr->Y + 3;
        }
        break;

    case VA_FOURCC_UYVY:
        if (mfx_fourcc != va_image.format.fourcc) return MFX_ERR_LOCK_MEMORY;

        {
            ptr->U = pBuffer + va_image.offsets[0];
            ptr->Y = ptr->U + 1;
            ptr->V = ptr->U + 2;
        }
        break;

#if defined (MFX_ENABLE_FOURCC_RGB565)
    case VA_FOURCC_RGB565:
        if (mfx_fourcc == MFX_FOURCC_RGB565)
        {
            ptr->B = pBuffer + va_image.offsets[0];
            ptr->G = ptr->B;
            ptr->R = ptr->B;
        }
        else return MFX_ERR_LOCK_MEMORY;
        break;

#endif
    case VA_FOURCC_ARGB:
        if (mfx_fourcc == MFX_FOURCC_RGB4)
        {
            ptr->B = pBuffer + va_image.offsets[0];
            ptr->G = ptr->B + 1;
            ptr->R = ptr->B + 2;
            ptr->A = ptr->B + 3;
        }
        else if (mfx_fourcc == MFX_FOURCC_A2RGB10)
        {
            ptr->B = pBuffer + va_image.offsets[0];
            ptr->G = ptr->B;
            ptr->R = ptr->B;
            ptr->A = ptr->B;
        }
        else return MFX_ERR_LOCK_MEMORY;
        break;
#ifdef MFX_ENABLE_RGBP
    case VA_FOURCC_RGBP:
        if (mfx_fourcc != va_image.format.fourcc) return MFX_ERR_LOCK_MEMORY;

        {
            ptr->B = pBuffer + va_image.offsets[0];
            ptr->G = pBuffer + va_image.offsets[1];
            ptr->R = pBuffer + va_image.offsets[2];
        }
        break;
#endif
    case VA_FOURCC_ABGR:
        if (mfx_fourcc == MFX_FOURCC_BGR4)
        {
            ptr->R = pBuffer + va_image.offsets[0];
            ptr->G = ptr->R + 1;
            ptr->B = ptr->R + 2;
            ptr->A = ptr->R + 3;
        }
        else return MFX_ERR_LOCK_MEMORY;
        break;

    case VA_FOURCC_P208:
        if (mfx_fourcc == MFX_FOURCC_NV12)
        {
            ptr->Y = pBuffer + va_image.offsets[0];
        }
        else return MFX_ERR_LOCK_MEMORY;
        break;

    case VA_FOURCC_P010:
        if (mfx_fourcc != va_image.format.fourcc) return MFX_ERR_LOCK_MEMORY;

        {
            ptr->Y = pBuffer + va_image.offsets[0];
            ptr->U = pBuffer + va_image.offsets[1];
            ptr->V = ptr->U + sizeof(mfxU16);
        }
        break;

    case MFX_FOURCC_AYUV:
        if (mfx_fourcc != va_image.format.fourcc) return MFX_ERR_LOCK_MEMORY;

        {
            ptr->V = pBuffer + va_image.offsets[0];
            ptr->U = ptr->V + 1;
            ptr->Y = ptr->V + 2;
            ptr->A = ptr->V + 3;
        }
        break;

#if (MFX_VERSION >= 1027)
    case VA_FOURCC_Y210:
        if (mfx_fourcc != va_image.format.fourcc) return MFX_ERR_LOCK_MEMORY;

        {
            ptr->Y16 = (mfxU16 *) (pBuffer + va_image.offsets[0]);
            ptr->U16 = ptr->Y16 + 1;
            ptr->V16 = ptr->Y16 + 3;
        }
        break;

    case MFX_FOURCC_Y410:
        if (mfx_fourcc != va_image.format.fourcc) return MFX_ERR_LOCK_MEMORY;

        {
            ptr->Y = ptr->U = ptr->V = ptr->A = 0;
            ptr->Y410 = (mfxY410*)(pBuffer + va_image.offsets[0]);
        }
        break;
#endif

    case MFX_FOURCC_VP8_SEGMAP:
        if (mfx_fourcc == MFX_FOURCC_P8)
        {
            ptr->Y = pBuffer;
        }
        else return MFX_ERR_LOCK_MEMORY;
        break;

    default:
        return MFX_ERR_LOCK_MEMORY;

    }

    ptr->PitchHigh = (mfxU16)(va_image.pitches[0] / (1 << 16));
    ptr->PitchLow  = (mfxU16)(va_image.pitches[0] % (1 << 16));

    return mfx_res;
}

mfxStatus
mfxDefaultAllocatorVAAPI::LockFrameHW(
    mfxHDL         pthis,
    mfxMemId       mid,
    mfxFrameData*  ptr)
{
    MFX_CHECK(pthis, MFX_ERR_INVALID_HANDLE);
    MFX_CHECK(mid,   MFX_ERR_INVALID_HANDLE);
    MFX_CHECK_NULL_PTR1(ptr);

    auto vaapi_mids = reinterpret_cast<vaapiMemIdInt*>(mid);
    MFX_CHECK(vaapi_mids->m_surface, MFX_ERR_INVALID_HANDLE);

    auto pSelf = reinterpret_cast<mfxWideHWFrameAllocator*>(pthis);

    VAStatus va_res = VA_STATUS_SUCCESS;

    mfxU32 mfx_fourcc = ConvertVP8FourccToMfxFourcc(vaapi_mids->m_fourcc);
    if (MFX_FOURCC_P8 == mfx_fourcc)   // bitstream processing
    {
        if (vaapi_mids->m_fourcc == MFX_FOURCC_VP8_SEGMAP)
        {
            mfxU8* pBuffer = nullptr;
            {
                MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaMapBuffer");
                va_res = vaMapBuffer(pSelf->pVADisplay, *(vaapi_mids->m_surface), (void **)(&pBuffer));
                MFX_CHECK(va_res == VA_STATUS_SUCCESS, VA_TO_MFX_STATUS(va_res));
            }

            ptr->Y = pBuffer;
        }
        else
        {
            VACodedBufferSegment *coded_buffer_segment;
            {
                MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaMapBuffer");
                va_res =  vaMapBuffer(pSelf->pVADisplay, *(vaapi_mids->m_surface), (void **)(&coded_buffer_segment));
                MFX_CHECK(va_res == VA_STATUS_SUCCESS, VA_TO_MFX_STATUS(va_res));
            }

            ptr->Y = reinterpret_cast<mfxU8*>(coded_buffer_segment->buf);
        }
    }
    else
    {
        va_res = vaDeriveImage(pSelf->pVADisplay, *(vaapi_mids->m_surface), &(vaapi_mids->m_image));
        MFX_CHECK(va_res == VA_STATUS_SUCCESS, VA_TO_MFX_STATUS(va_res));

        mfxU8* pBuffer = nullptr;
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaMapBuffer");
            va_res = vaMapBuffer(pSelf->pVADisplay, vaapi_mids->m_image.buf, (void **) &pBuffer);
            MFX_CHECK(va_res == VA_STATUS_SUCCESS, VA_TO_MFX_STATUS(va_res));
        }

        mfxStatus mfx_res = SetFrameData(vaapi_mids->m_image, mfx_fourcc, pBuffer, ptr);
        MFX_CHECK_STS(mfx_res);
    }

    return MFX_ERR_NONE;
}

mfxStatus mfxDefaultAllocatorVAAPI::UnlockFrameHW(
    mfxHDL         pthis,
    mfxMemId       mid,
    mfxFrameData*  ptr)
{
    MFX_CHECK(pthis, MFX_ERR_INVALID_HANDLE);
    MFX_CHECK(mid,   MFX_ERR_INVALID_HANDLE);

    auto vaapi_mids = reinterpret_cast<vaapiMemIdInt*>(mid);
    MFX_CHECK(vaapi_mids->m_surface, MFX_ERR_INVALID_HANDLE);

    auto pSelf = reinterpret_cast<mfxWideHWFrameAllocator*>(pthis);

    VAStatus va_res = VA_STATUS_SUCCESS;

    mfxU32 mfx_fourcc = ConvertVP8FourccToMfxFourcc(vaapi_mids->m_fourcc);
    if (MFX_FOURCC_P8 == mfx_fourcc)   // bitstream processing
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
        va_res = vaUnmapBuffer(pSelf->pVADisplay, *(vaapi_mids->m_surface));
        MFX_CHECK(va_res == VA_STATUS_SUCCESS, VA_TO_MFX_STATUS(va_res));
    }
    else  // Image processing
    {
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
            va_res = vaUnmapBuffer(pSelf->pVADisplay, vaapi_mids->m_image.buf);
            MFX_CHECK(va_res == VA_STATUS_SUCCESS, VA_TO_MFX_STATUS(va_res));
        }
        va_res = vaDestroyImage(pSelf->pVADisplay, vaapi_mids->m_image.image_id);
        MFX_CHECK(va_res == VA_STATUS_SUCCESS, VA_TO_MFX_STATUS(va_res));

        if (ptr)
        {
            ptr->PitchLow  = 0;
            ptr->PitchHigh = 0;
            ptr->Y     = nullptr;
            ptr->U     = nullptr;
            ptr->V     = nullptr;
            ptr->A     = nullptr;
        }
    }
    return MFX_ERR_NONE;
}

mfxStatus
mfxDefaultAllocatorVAAPI::GetHDLHW(
    mfxHDL    pthis,
    mfxMemId  mid,
    mfxHDL*   handle)
{
    MFX_CHECK(pthis, MFX_ERR_INVALID_HANDLE);
    MFX_CHECK(mid,   MFX_ERR_INVALID_HANDLE);

    auto vaapi_mids = reinterpret_cast<vaapiMemIdInt*>(mid);
    MFX_CHECK(vaapi_mids->m_surface, MFX_ERR_INVALID_HANDLE);

    *handle = vaapi_mids->m_surface; //VASurfaceID* <-> mfxHDL
    return MFX_ERR_NONE;
}

mfxDefaultAllocatorVAAPI::mfxWideHWFrameAllocator::mfxWideHWFrameAllocator(
    mfxU16  type,
    mfxHDL  handle)
    : mfxBaseWideFrameAllocator(type)
    , m_DecId(0)
{
    frameAllocator.Alloc = &mfxDefaultAllocatorVAAPI::AllocFramesHW;
    frameAllocator.Lock = &mfxDefaultAllocatorVAAPI::LockFrameHW;
    frameAllocator.GetHDL = &mfxDefaultAllocatorVAAPI::GetHDLHW;
    frameAllocator.Unlock = &mfxDefaultAllocatorVAAPI::UnlockFrameHW;
    frameAllocator.Free = &mfxDefaultAllocatorVAAPI::FreeFramesHW;
    pVADisplay = (VADisplay *)handle;
}
#endif // (MFX_VA_LINUX)
/* EOF */
