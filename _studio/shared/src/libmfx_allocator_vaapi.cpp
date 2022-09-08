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

enum {
    MFX_FOURCC_VP8_NV12    = MFX_MAKEFOURCC('V','P','8','N'),
    MFX_FOURCC_VP8_MBDATA  = MFX_MAKEFOURCC('V','P','8','M'),
    MFX_FOURCC_VP8_SEGMAP  = MFX_MAKEFOURCC('V','P','8','S'),
};

static inline unsigned int ConvertVP8FourccToMfxFourcc(mfxU32 fourcc)
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

static inline unsigned int ConvertMfxFourccToVAFormat(mfxU32 fourcc)
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
#ifndef ANDROID
    case MFX_FOURCC_A2RGB10:
        return VA_FOURCC_A2R10G10B10;
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
#if (MFX_VERSION >= 1031)
    case MFX_FOURCC_P016:
        return VA_FOURCC_P016;
    case MFX_FOURCC_Y216:
        return VA_FOURCC_Y216;
    case MFX_FOURCC_Y416:
        return VA_FOURCC_Y416;
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
            //  Enable this hint as required for creating RGBP surface for JPEG.
            if ((memType & MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET)
                    && (memType & MFX_MEMTYPE_FROM_DECODE))
            {
                attrib.resize(attrib.size()+1);
                attrib[1].type            = VASurfaceAttribUsageHint;
                attrib[1].flags           = VA_SURFACE_ATTRIB_SETTABLE;
                attrib[1].value.type      = VAGenericValueTypeInteger;
                attrib[1].value.value.i   = VA_SURFACE_ATTRIB_USAGE_HINT_DECODER;
            }
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
            format = va_fourcc;
            break;
    }
}

static inline bool isFourCCSupported(unsigned int va_fourcc)
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
#ifndef ANDROID
        case VA_FOURCC_A2R10G10B10:
#endif
#if (MFX_VERSION >= 1027)
        case VA_FOURCC_Y210:
        case VA_FOURCC_Y410:
#endif
#if (MFX_VERSION >= 1031)
        case VA_FOURCC_P016:
        case VA_FOURCC_Y216:
        case VA_FOURCC_Y416:
#endif
            return true;
        default:
            return false;
    }
}

static mfxStatus ReallocImpl(VADisplay* va_disp, vaapiMemIdInt *vaapi_mid, mfxFrameSurface1 *surf)
{
    MFX_CHECK_NULL_PTR3(va_disp, vaapi_mid, surf);
    MFX_CHECK_NULL_PTR1(vaapi_mid->m_surface);

    // VP8 hybrid driver has weird requirements for allocation of surfaces/buffers for VP8 encoding
    // to comply with them additional logic is required to support regular and VP8 hybrid allocation pathes
    mfxU32 mfx_fourcc = ConvertVP8FourccToMfxFourcc(surf->Info.FourCC);
    unsigned int va_fourcc = ConvertMfxFourccToVAFormat(mfx_fourcc);

    MFX_CHECK(isFourCCSupported(va_fourcc), MFX_ERR_UNSUPPORTED);

    VAStatus va_res = VA_STATUS_SUCCESS;
    if (MFX_FOURCC_P8 == vaapi_mid->m_fourcc)
    {
        mfxStatus sts = CheckAndDestroyVAbuffer(va_disp, *vaapi_mid->m_surface);
        MFX_CHECK(sts == MFX_ERR_NONE, MFX_ERR_MEMORY_ALLOC);
    }
    else
    {
        va_res = vaDestroySurfaces(va_disp, vaapi_mid->m_surface, 1);
        MFX_CHECK(va_res == VA_STATUS_SUCCESS, MFX_ERR_MEMORY_ALLOC);
        *vaapi_mid->m_surface = VA_INVALID_ID;
    }


    std::vector<VASurfaceAttrib> attrib;
    unsigned int format;
    FillSurfaceAttrs(attrib, format, surf->Info.FourCC, va_fourcc, 0);

    va_res = vaCreateSurfaces(va_disp,
                            format,
                            surf->Info.Width, surf->Info.Height,
                            vaapi_mid->m_surface,
                            1,
                            attrib.data(), attrib.size());
    MFX_CHECK(va_res == VA_STATUS_SUCCESS, MFX_ERR_MEMORY_ALLOC);

    // Update fourcc of reallocated element. VAid was updated automatically
    vaapi_mid->m_fourcc = surf->Info.FourCC;

    return MFX_ERR_NONE;
}

// aka AllocImpl(surface)
mfxStatus mfxDefaultAllocatorVAAPI::ReallocFrameHW(mfxHDL pthis, mfxFrameSurface1 *surf, VASurfaceID *va_surf)
{
    MFX_CHECK_NULL_PTR3(pthis, surf, va_surf);

    mfxWideHWFrameAllocator *self = reinterpret_cast<mfxWideHWFrameAllocator*>(pthis);

    auto it = std::find_if(std::begin(self->m_frameHandles), std::end(self->m_frameHandles),
                            [va_surf](mfxHDL hndl){ return hndl && *(reinterpret_cast<vaapiMemIdInt *>(hndl))->m_surface == *va_surf; });

    MFX_CHECK(it != std::end(self->m_frameHandles), MFX_ERR_MEMORY_ALLOC);

    return ReallocImpl(self->m_pVADisplay, reinterpret_cast<vaapiMemIdInt *>(*it), surf);
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


    auto self = reinterpret_cast<mfxWideHWFrameAllocator*>(pthis);

    // Enough frames were allocated previously. Return existing frames
    if (self->NumFrames)
    {
        MFX_CHECK(request->NumFrameSuggested <= self->NumFrames, MFX_ERR_MEMORY_ALLOC);

        response->mids           = self->m_frameHandles.data();
        response->NumFrameActual = request->NumFrameSuggested;
        response->AllocId        = request->AllocId;

        return MFX_ERR_NONE;
    }

    // Use temporary storage for preliminary operations. If some of them fail, current state of allocator remain unchanged.
    // When allocation will be finished, just move content of these vectors to internal allocator storage
    std::vector<VASurfaceID>   allocated_surfaces(request->NumFrameSuggested, VA_INVALID_ID);
    std::vector<vaapiMemIdInt> allocated_mids(request->NumFrameSuggested);

    VAStatus  va_res  = VA_STATUS_SUCCESS;
    mfxStatus mfx_res = MFX_ERR_NONE;

    if( VA_FOURCC_P208 != va_fourcc)
    {
        unsigned int format;
        std::vector<VASurfaceAttrib> attrib;
        FillSurfaceAttrs(attrib, format, request->Info.FourCC, va_fourcc, request->Type);

        va_res = vaCreateSurfaces(self->m_pVADisplay,
                            format,
                            request->Info.Width, request->Info.Height,
                            allocated_surfaces.data(),
                            allocated_surfaces.size(),
                            attrib.data(),
                            attrib.size());

        MFX_CHECK(va_res == VA_STATUS_SUCCESS, MFX_ERR_DEVICE_FAILED);
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

        for (VABufferID& coded_buf : allocated_surfaces)
        {
            va_res = vaCreateBuffer(self->m_pVADisplay,
                      VAContextID(request->AllocId),
                      codedbuf_type,
                      codedbuf_size,
                      codedbuf_num,
                      NULL,
                      &coded_buf);

            if (va_res != VA_STATUS_SUCCESS)
            {
                // Need to clean up already allocated buffers
                mfx_res = MFX_ERR_DEVICE_FAILED;
                break;
            }
        }
    }

    if (MFX_ERR_NONE == mfx_res)
    {
        // Clean up existing state
        self->NumFrames = 0;
        self->m_frameHandles.clear();
        self->m_frameHandles.reserve(request->NumFrameSuggested);

        // Push new frames
        for (mfxU32 i = 0; i < request->NumFrameSuggested; ++i)
        { 
            allocated_mids[i].m_surface = &allocated_surfaces[i];
            allocated_mids[i].m_fourcc  = request->Info.FourCC;

            self->m_frameHandles.push_back(&allocated_mids[i]);
        }
        response->mids           = self->m_frameHandles.data();
        response->NumFrameActual = request->NumFrameSuggested;
        response->AllocId        = request->AllocId;

        self->NumFrames = self->m_frameHandles.size();

        // Save new frames in internal state
        self->m_allocatedSurfaces = std::move(allocated_surfaces);
        self->m_allocatedMids     = std::move(allocated_mids);
    }
    else
    {
        // Some of vaCreateBuffer calls failed
        for (VABufferID& coded_buf : allocated_surfaces)
        {
            mfxStatus sts = CheckAndDestroyVAbuffer(self->m_pVADisplay, coded_buf);
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

        auto self = reinterpret_cast<mfxWideHWFrameAllocator*>(pthis);
        // Make sure that we are asked to clean memory which was allocated by current allocator
        MFX_CHECK(self->m_allocatedSurfaces.data() == vaapi_mids->m_surface, MFX_ERR_UNDEFINED_BEHAVIOR);

        if (ConvertVP8FourccToMfxFourcc(vaapi_mids->m_fourcc) == MFX_FOURCC_P8)
        {
            for (VABufferID& coded_buf : self->m_allocatedSurfaces)
            {
                mfxStatus sts = CheckAndDestroyVAbuffer(self->m_pVADisplay, coded_buf);
                MFX_CHECK_STS(sts);
            }
        }
        else
        {
            // Not Buffered memory
            VAStatus va_sts = vaDestroySurfaces(self->m_pVADisplay, vaapi_mids->m_surface, response->NumFrameActual);
            MFX_CHECK(VA_STATUS_SUCCESS == va_sts, MFX_ERR_DEVICE_FAILED);
        }
        response->mids = nullptr;

        // Reset internal state
        self->NumFrames = 0;
        self->m_frameHandles.clear();
        self->m_allocatedSurfaces.clear();
        self->m_allocatedMids.clear();
    }
    response->NumFrameActual = 0;

    return MFX_ERR_NONE;
}

mfxStatus mfxDefaultAllocatorVAAPI::SetFrameData(const VAImage &va_image, mfxU32 mfx_fourcc, mfxU8* p_buffer, mfxFrameData*  ptr)
{
    mfxStatus mfx_res = MFX_ERR_NONE;

    switch (va_image.format.fourcc)
    {
    case VA_FOURCC_NV12:
        if (mfx_fourcc != va_image.format.fourcc) return MFX_ERR_LOCK_MEMORY;

        {
            ptr->Y = p_buffer + va_image.offsets[0];
            ptr->U = p_buffer + va_image.offsets[1];
            ptr->V = ptr->U + 1;
        }
        break;

    case VA_FOURCC_YV12:
        if (mfx_fourcc != va_image.format.fourcc) return MFX_ERR_LOCK_MEMORY;

        {
            ptr->Y = p_buffer + va_image.offsets[0];
            ptr->V = p_buffer + va_image.offsets[1];
            ptr->U = p_buffer + va_image.offsets[2];
        }
        break;

    case VA_FOURCC_YUY2:
        if (mfx_fourcc != va_image.format.fourcc) return MFX_ERR_LOCK_MEMORY;

        {
            ptr->Y = p_buffer + va_image.offsets[0];
            ptr->U = ptr->Y + 1;
            ptr->V = ptr->Y + 3;
        }
        break;

    case VA_FOURCC_UYVY:
        if (mfx_fourcc != va_image.format.fourcc) return MFX_ERR_LOCK_MEMORY;

        {
            ptr->U = p_buffer + va_image.offsets[0];
            ptr->Y = ptr->U + 1;
            ptr->V = ptr->U + 2;
        }
        break;

#if defined (MFX_ENABLE_FOURCC_RGB565)
    case VA_FOURCC_RGB565:
        if (mfx_fourcc == MFX_FOURCC_RGB565)
        {
            ptr->B = p_buffer + va_image.offsets[0];
            ptr->G = ptr->B;
            ptr->R = ptr->B;
        }
        else return MFX_ERR_LOCK_MEMORY;
        break;

#endif
    case VA_FOURCC_ARGB:
        if (mfx_fourcc == MFX_FOURCC_RGB4)
        {
            ptr->B = p_buffer + va_image.offsets[0];
            ptr->G = ptr->B + 1;
            ptr->R = ptr->B + 2;
            ptr->A = ptr->B + 3;
        }
        else return MFX_ERR_LOCK_MEMORY;
        break;
#ifndef ANDROID
    case VA_FOURCC_A2R10G10B10:
        if (mfx_fourcc == MFX_FOURCC_A2RGB10)
        {
            ptr->B = p_buffer + va_image.offsets[0];
            ptr->G = ptr->B;
            ptr->R = ptr->B;
            ptr->A = ptr->B;
        }
        else return MFX_ERR_LOCK_MEMORY;
        break;
#endif
#ifdef MFX_ENABLE_RGBP
    case VA_FOURCC_RGBP:
        if (mfx_fourcc != va_image.format.fourcc) return MFX_ERR_LOCK_MEMORY;

        {
            ptr->B = p_buffer + va_image.offsets[0];
            ptr->G = p_buffer + va_image.offsets[1];
            ptr->R = p_buffer + va_image.offsets[2];
        }
        break;
#endif
    case VA_FOURCC_ABGR:
        if (mfx_fourcc == MFX_FOURCC_BGR4)
        {
            ptr->R = p_buffer + va_image.offsets[0];
            ptr->G = ptr->R + 1;
            ptr->B = ptr->R + 2;
            ptr->A = ptr->R + 3;
        }
        else return MFX_ERR_LOCK_MEMORY;
        break;

    case VA_FOURCC_P208:
        if (mfx_fourcc == MFX_FOURCC_NV12)
        {
            ptr->Y = p_buffer + va_image.offsets[0];
        }
        else return MFX_ERR_LOCK_MEMORY;
        break;

    case VA_FOURCC_P010:
#if (MFX_VERSION >= 1031)
    case VA_FOURCC_P016:
    case VA_FOURCC_P012:
#endif
        if ((mfx_fourcc == MFX_FOURCC_P010 && va_image.format.fourcc == VA_FOURCC_P010) ||
            (mfx_fourcc == MFX_FOURCC_P016 && (va_image.format.fourcc == VA_FOURCC_P012 ||
                                               va_image.format.fourcc == VA_FOURCC_P016)))
        {
            ptr->Y = p_buffer + va_image.offsets[0];
            ptr->U = p_buffer + va_image.offsets[1];
            ptr->V = ptr->U + sizeof(mfxU16);
        }
        else return MFX_ERR_LOCK_MEMORY;
        break;

    case VA_FOURCC_AYUV:
    /* Set AYUV mfxFrameData by XYUV VAImage */
#ifdef VA_FOURCC_XYUV
    case VA_FOURCC_XYUV:
#endif
        if (mfx_fourcc != MFX_FOURCC_AYUV) return MFX_ERR_LOCK_MEMORY;

        {
            ptr->V = p_buffer + va_image.offsets[0];
            ptr->U = ptr->V + 1;
            ptr->Y = ptr->V + 2;
            ptr->A = ptr->V + 3;
        }
        break;

#if (MFX_VERSION >= 1027)
    case VA_FOURCC_Y210:
#if (MFX_VERSION >= 1031)
    case VA_FOURCC_Y216:
    case VA_FOURCC_Y212:
#endif
        if ((mfx_fourcc == MFX_FOURCC_Y210 && va_image.format.fourcc == VA_FOURCC_Y210) ||
            (mfx_fourcc == MFX_FOURCC_Y216 && (va_image.format.fourcc == VA_FOURCC_Y216 ||
                                               va_image.format.fourcc == VA_FOURCC_Y212)))
        {
            ptr->Y16 = (mfxU16 *) (p_buffer + va_image.offsets[0]);
            ptr->U16 = ptr->Y16 + 1;
            ptr->V16 = ptr->Y16 + 3;
        }
        else return MFX_ERR_LOCK_MEMORY;
        break;

    case VA_FOURCC_Y410:
        if (mfx_fourcc != va_image.format.fourcc) return MFX_ERR_LOCK_MEMORY;

        {
            ptr->Y = ptr->U = ptr->V = ptr->A = 0;
            ptr->Y410 = (mfxY410*)(p_buffer + va_image.offsets[0]);
        }
        break;
#endif

#if (MFX_VERSION >= 1031)
    case VA_FOURCC_Y416:
    case VA_FOURCC_Y412:
        if (mfx_fourcc == MFX_FOURCC_Y416)
        {
            ptr->U16 = (mfxU16 *) (p_buffer + va_image.offsets[0]);
            ptr->Y16 = ptr->U16 + 1;
            ptr->V16 = ptr->Y16 + 1;
            ptr->A   = (mfxU8 *)(ptr->V16 + 1);
        }
        else return MFX_ERR_LOCK_MEMORY;
        break;

#endif

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

    auto self = reinterpret_cast<mfxWideHWFrameAllocator*>(pthis);

    VAStatus va_res = VA_STATUS_SUCCESS;

    mfxU32 mfx_fourcc = ConvertVP8FourccToMfxFourcc(vaapi_mids->m_fourcc);
    if (MFX_FOURCC_P8 == mfx_fourcc)   // bitstream processing
    {
        if (vaapi_mids->m_fourcc == MFX_FOURCC_VP8_SEGMAP)
        {
            mfxU8* p_buffer = nullptr;
            {
                MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaMapBuffer");
                va_res = vaMapBuffer(self->m_pVADisplay, *(vaapi_mids->m_surface), (void **)(&p_buffer));
                MFX_CHECK(va_res == VA_STATUS_SUCCESS, MFX_ERR_DEVICE_FAILED);
            }

            ptr->Y = p_buffer;
        }
        else
        {
            VACodedBufferSegment *coded_buffer_segment;
            {
                MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaMapBuffer");
                va_res =  vaMapBuffer(self->m_pVADisplay, *(vaapi_mids->m_surface), (void **)(&coded_buffer_segment));
                MFX_CHECK(va_res == VA_STATUS_SUCCESS, MFX_ERR_DEVICE_FAILED);
            }

            ptr->Y = reinterpret_cast<mfxU8*>(coded_buffer_segment->buf);
        }
    }
    else
    {
        va_res = vaDeriveImage(self->m_pVADisplay, *(vaapi_mids->m_surface), &(vaapi_mids->m_image));
        MFX_CHECK(va_res == VA_STATUS_SUCCESS, MFX_ERR_DEVICE_FAILED);

        mfxU8* p_buffer = nullptr;
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaMapBuffer");
            va_res = vaMapBuffer(self->m_pVADisplay, vaapi_mids->m_image.buf, (void **) &p_buffer);
            MFX_CHECK(va_res == VA_STATUS_SUCCESS, MFX_ERR_DEVICE_FAILED);
        }

        mfxStatus mfx_res = SetFrameData(vaapi_mids->m_image, mfx_fourcc, p_buffer, ptr);
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

    auto self = reinterpret_cast<mfxWideHWFrameAllocator*>(pthis);

    VAStatus va_res = VA_STATUS_SUCCESS;

    mfxU32 mfx_fourcc = ConvertVP8FourccToMfxFourcc(vaapi_mids->m_fourcc);
    if (MFX_FOURCC_P8 == mfx_fourcc)   // bitstream processing
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
        va_res = vaUnmapBuffer(self->m_pVADisplay, *(vaapi_mids->m_surface));
        MFX_CHECK(va_res == VA_STATUS_SUCCESS, MFX_ERR_DEVICE_FAILED);
    }
    else  // Image processing
    {
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
            va_res = vaUnmapBuffer(self->m_pVADisplay, vaapi_mids->m_image.buf);
            MFX_CHECK(va_res == VA_STATUS_SUCCESS, MFX_ERR_DEVICE_FAILED);
        }
        va_res = vaDestroyImage(self->m_pVADisplay, vaapi_mids->m_image.image_id);
        MFX_CHECK(va_res == VA_STATUS_SUCCESS, MFX_ERR_DEVICE_FAILED);

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
    , m_pVADisplay(reinterpret_cast<VADisplay *>(handle))
    , m_DecId(0)
{
    frameAllocator.Alloc  = &mfxDefaultAllocatorVAAPI::AllocFramesHW;
    frameAllocator.Lock   = &mfxDefaultAllocatorVAAPI::LockFrameHW;
    frameAllocator.GetHDL = &mfxDefaultAllocatorVAAPI::GetHDLHW;
    frameAllocator.Unlock = &mfxDefaultAllocatorVAAPI::UnlockFrameHW;
    frameAllocator.Free   = &mfxDefaultAllocatorVAAPI::FreeFramesHW;
}
#endif // (MFX_VA_LINUX)
/* EOF */
