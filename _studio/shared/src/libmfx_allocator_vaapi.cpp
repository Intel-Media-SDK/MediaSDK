// Copyright (c) 2017 Intel Corporation
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

#define VA_SURFACE_ATTRIB_USAGE_HINT_ENCODER    0x00000002
#define VASurfaceAttribUsageHint 8
#define VAEncMacroblockMapBufferType 29

// TODO: remove this internal definition once it appears in VAAPI
#define VA_FOURCC_R5G6B5 MFX_MAKEFOURCC('R','G','1','6')

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
#if (MFX_VERSION >= MFX_VERSION_NEXT)
    case MFX_FOURCC_RGB565:
        return VA_FOURCC_R5G6B5;
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

    default:
        VM_ASSERT(!"unsupported fourcc");
        return 0;
    }
}

mfxStatus
mfxDefaultAllocatorVAAPI::AllocFramesHW(
    mfxHDL                  pthis,
    mfxFrameAllocRequest*   request,
    mfxFrameAllocResponse*  response)
{
    mfxStatus mfx_res = MFX_ERR_NONE;
    VAStatus  va_res  = VA_STATUS_SUCCESS;
    unsigned int va_fourcc = 0;
    VASurfaceID* surfaces = NULL;
    VASurfaceAttrib attrib;
    vaapiMemIdInt *vaapi_mids = NULL, *vaapi_mid = NULL;
    mfxU32 fourcc = request->Info.FourCC;
    mfxU16 surfaces_num = request->NumFrameSuggested, numAllocated = 0, i = 0;
    bool bCreateSrfSucceeded = false;

    if (!pthis)
    {
        return MFX_ERR_INVALID_HANDLE;
    }

    memset(response, 0, sizeof(mfxFrameAllocResponse));

    mfxU32 mfx_fourcc = fourcc;
    va_fourcc = ConvertMfxFourccToVAFormat(mfx_fourcc);
    if (!va_fourcc || ((VA_FOURCC_NV12   != va_fourcc) &&
                       (VA_FOURCC_YV12   != va_fourcc) &&
                       (VA_FOURCC_YUY2   != va_fourcc) &&
                       (VA_FOURCC_ARGB   != va_fourcc) &&
                       (VA_FOURCC_ABGR   != va_fourcc) &&
#ifdef MFX_ENABLE_RGBP
                       (VA_FOURCC_RGBP   != va_fourcc) &&
#endif
                       (VA_FOURCC_UYVY   != va_fourcc) &&
                       (VA_FOURCC_P208   != va_fourcc) &&
                       (VA_FOURCC_P010   != va_fourcc) &&
                       (VA_FOURCC_AYUV   != va_fourcc) &&
                       (VA_FOURCC_R5G6B5 != va_fourcc)))
    {
        return MFX_ERR_MEMORY_ALLOC;
    }
    if (!surfaces_num)
    {
        return MFX_ERR_MEMORY_ALLOC;
    }

    mfxWideHWFrameAllocator *pSelf = (mfxWideHWFrameAllocator*)pthis;

    // frames were allocated
    // return existent frames
    if (pSelf->NumFrames)
    {
        if (request->NumFrameSuggested > pSelf->NumFrames)
            return MFX_ERR_MEMORY_ALLOC;
        else
        {
            response->mids = &pSelf->m_frameHandles[0];
            return MFX_ERR_NONE;
        }
    }

    // allocate frames in cycle
    if (MFX_ERR_NONE == mfx_res)
    {
        surfaces = (VASurfaceID*)calloc(surfaces_num, sizeof(VASurfaceID));
        vaapi_mids = (vaapiMemIdInt*)calloc(surfaces_num, sizeof(vaapiMemIdInt));
        if ((NULL == surfaces) || (NULL == vaapi_mids))
        {
            mfx_res = MFX_ERR_MEMORY_ALLOC;
        }
    }
    if (MFX_ERR_NONE == mfx_res)
    {
        if( VA_FOURCC_P208 != va_fourcc)
        {
            unsigned int format;
            VASurfaceAttrib *pAttrib = &attrib;

            attrib.type          = VASurfaceAttribPixelFormat;
            attrib.flags         = VA_SURFACE_ATTRIB_SETTABLE;
            attrib.value.type    = VAGenericValueTypeInteger;
            attrib.value.value.i = va_fourcc;
            format               = va_fourcc;

            if (va_fourcc == VA_FOURCC_NV12)
            {
                format = VA_RT_FORMAT_YUV420;
            }
            else if (va_fourcc == VA_FOURCC_UYVY)
            {
                format = VA_RT_FORMAT_YUV422;
            }
#ifdef MFX_ENABLE_RGBP            
            else if (va_fourcc == VA_FOURCC_RGBP)
            {
                format = VA_RT_FORMAT_RGBP;
            }
#endif

            if ( request->Type & MFX_MEMTYPE_FROM_ENCODE)
            {
                int             attribCnt = 0;
                VASurfaceAttrib surfaceAttrib[2];

                surfaceAttrib[attribCnt].type = VASurfaceAttribPixelFormat;
                surfaceAttrib[attribCnt].value.type = VAGenericValueTypeInteger;
                surfaceAttrib[attribCnt].flags = VA_SURFACE_ATTRIB_SETTABLE;
                surfaceAttrib[attribCnt++].value.value.i = va_fourcc;

                /*
                 *  Enable this hint as required for creating RGB32 surface for MJPEG.
                 */
                if ((request->Type & MFX_MEMTYPE_VIDEO_MEMORY_ENCODER_TARGET) 
                 && ((VA_FOURCC_ARGB == va_fourcc) || (VA_FOURCC_ABGR == va_fourcc)))
                {
              //  Input Attribute Usage Hint
                    surfaceAttrib[attribCnt].flags           = VA_SURFACE_ATTRIB_SETTABLE;
                    surfaceAttrib[attribCnt].type            = (VASurfaceAttribType) VASurfaceAttribUsageHint;
                    surfaceAttrib[attribCnt].value.type      = VAGenericValueTypeInteger;
                    surfaceAttrib[attribCnt++].value.value.i = VA_SURFACE_ATTRIB_USAGE_HINT_ENCODER;
                }

                va_res = vaCreateSurfaces(pSelf->pVADisplay,
                                    format,
                                    request->Info.Width, request->Info.Height,
                                    surfaces,
                                    surfaces_num,
                                    &surfaceAttrib[0],
                                    attribCnt);
            }
            else
            {
                va_res = vaCreateSurfaces(pSelf->pVADisplay,
                                    format,
                                    request->Info.Width, request->Info.Height,
                                    surfaces,
                                    surfaces_num,
                                    pAttrib, pAttrib ? 1 : 0);
            }
            mfx_res = VA_TO_MFX_STATUS(va_res);
            bCreateSrfSucceeded = (MFX_ERR_NONE == mfx_res);
        }
        else
        {
            VAContextID context_id = request->AllocId;
            mfxU32 codedbuf_size;
            VABufferType codedbuf_type;

            {
                int width32 = 32 * ((request->Info.Width + 31) >> 5);
                int height32 = 32 * ((request->Info.Height + 31) >> 5);
                codedbuf_size = static_cast<mfxU32>((width32 * height32) * 400LL / (16 * 16));

                codedbuf_type = VAEncCodedBufferType;
            }

            for (numAllocated = 0; numAllocated < surfaces_num; numAllocated++)
            {
                VABufferID coded_buf;

                va_res = vaCreateBuffer(pSelf->pVADisplay,
                                      context_id,
                                      codedbuf_type,
                                      codedbuf_size,
                                      1,
                                      NULL,
                                      &coded_buf);
                mfx_res = VA_TO_MFX_STATUS(va_res);
                if (MFX_ERR_NONE != mfx_res) break;
                surfaces[numAllocated] = coded_buf;
            }
        }
    }
    if (MFX_ERR_NONE == mfx_res)
    {
        for (i = 0; i < surfaces_num; ++i)
        {
            vaapi_mid = &(vaapi_mids[i]);
            vaapi_mid->m_fourcc = fourcc;
            vaapi_mid->m_surface = &(surfaces[i]);

            pSelf->m_frameHandles.push_back(vaapi_mid);
        }
        response->mids = &pSelf->m_frameHandles[0];
        response->NumFrameActual = surfaces_num;
        pSelf->NumFrames = surfaces_num;
    }
    else // i.e. MFX_ERR_NONE != mfx_res
    {
        response->mids = NULL;
        response->NumFrameActual = 0;
        if (VA_FOURCC_P208 != va_fourcc
            )
        {
            if (bCreateSrfSucceeded) vaDestroySurfaces(pSelf->pVADisplay, surfaces, surfaces_num);
        }
        else
        {
            for (i = 0; i < numAllocated; i++)
                vaDestroyBuffer(pSelf->pVADisplay, surfaces[i]);
        }
        if (vaapi_mids) { free(vaapi_mids); vaapi_mids = NULL; }
        if (surfaces) { free(surfaces); surfaces = NULL; }
    }
    return mfx_res;
}

mfxStatus mfxDefaultAllocatorVAAPI::FreeFramesHW(
    mfxHDL                  pthis,
    mfxFrameAllocResponse*  response)
{
    if (!pthis)
        return MFX_ERR_INVALID_HANDLE;

    mfxWideHWFrameAllocator *pSelf = (mfxWideHWFrameAllocator*)pthis;

    vaapiMemIdInt *vaapi_mids = NULL;
    VASurfaceID* surfaces = NULL;
    mfxU32 i = 0;
    bool isBufferMemory=false;

    if (!response) return MFX_ERR_NULL_PTR;

    if (response->mids)
    {
        vaapi_mids = (vaapiMemIdInt*)(response->mids[0]);
        mfxU32 mfx_fourcc = vaapi_mids->m_fourcc;
        isBufferMemory = (MFX_FOURCC_P8 == mfx_fourcc)?true:false;
        surfaces = vaapi_mids->m_surface;
        for (i = 0; i < response->NumFrameActual; ++i)
        {
            if (MFX_FOURCC_P8 == vaapi_mids[i].m_fourcc)
            {
                vaDestroyBuffer(pSelf->pVADisplay, surfaces[i]);
            }
        }
        free(vaapi_mids);
        response->mids = NULL;

        if (!isBufferMemory) vaDestroySurfaces(pSelf->pVADisplay, surfaces, response->NumFrameActual);
        free(surfaces);
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
        if (mfx_fourcc == MFX_FOURCC_NV12)
        {
            ptr->PitchHigh = (mfxU16)(va_image.pitches[0] / (1 << 16));
            ptr->PitchLow  = (mfxU16)(va_image.pitches[0] % (1 << 16));
            ptr->Y = pBuffer + va_image.offsets[0];
            ptr->U = pBuffer + va_image.offsets[1];
            ptr->V = ptr->U + 1;
        }
        else mfx_res = MFX_ERR_LOCK_MEMORY;
        break;
    case VA_FOURCC_YV12:
        if (mfx_fourcc == MFX_FOURCC_YV12)
        {
            ptr->PitchHigh = (mfxU16)(va_image.pitches[0] / (1 << 16));
            ptr->PitchLow  = (mfxU16)(va_image.pitches[0] % (1 << 16));
            ptr->Y = pBuffer + va_image.offsets[0];
            ptr->V = pBuffer + va_image.offsets[1];
            ptr->U = pBuffer + va_image.offsets[2];
        }
        else mfx_res = MFX_ERR_LOCK_MEMORY;
        break;
    case VA_FOURCC_YUY2:
        if (mfx_fourcc == MFX_FOURCC_YUY2)
        {
            ptr->PitchHigh = (mfxU16)(va_image.pitches[0] / (1 << 16));
            ptr->PitchLow  = (mfxU16)(va_image.pitches[0] % (1 << 16));
            ptr->Y = pBuffer + va_image.offsets[0];
            ptr->U = ptr->Y + 1;
            ptr->V = ptr->Y + 3;
        }
        else mfx_res = MFX_ERR_LOCK_MEMORY;
        break;
    case VA_FOURCC_UYVY:
        if (mfx_fourcc == MFX_FOURCC_UYVY)
        {
            ptr->PitchHigh = (mfxU16)(va_image.pitches[0] / (1 << 16));
            ptr->PitchLow  = (mfxU16)(va_image.pitches[0] % (1 << 16));
            ptr->U = pBuffer + va_image.offsets[0];
            ptr->Y = ptr->U + 1;
            ptr->V = ptr->U + 2;
        }
        else mfx_res = MFX_ERR_LOCK_MEMORY;
        break;
#if (MFX_VERSION >= MFX_VERSION_NEXT)
    case VA_FOURCC_R5G6B5:
        if (mfx_fourcc == MFX_FOURCC_RGB565)
        {
            ptr->PitchHigh = (mfxU16)(va_image.pitches[0] >> 16);
            ptr->PitchLow  = (mfxU16)(va_image.pitches[0] & 0xffff);
            ptr->B = pBuffer + va_image.offsets[0];
            ptr->G = ptr->B;
            ptr->R = ptr->B;
        }
        else mfx_res = MFX_ERR_LOCK_MEMORY;
        break;
#endif
    case VA_FOURCC_ARGB:
        if (mfx_fourcc == MFX_FOURCC_RGB4)
        {
            ptr->PitchHigh = (mfxU16)(va_image.pitches[0] / (1 << 16));
            ptr->PitchLow  = (mfxU16)(va_image.pitches[0] % (1 << 16));
            ptr->B = pBuffer + va_image.offsets[0];
            ptr->G = ptr->B + 1;
            ptr->R = ptr->B + 2;
            ptr->A = ptr->B + 3;
        }
        else if (mfx_fourcc == MFX_FOURCC_A2RGB10)
        {
            ptr->PitchHigh = (mfxU16)(va_image.pitches[0] >> 16);
            ptr->PitchLow  = (mfxU16)(va_image.pitches[0] & 0xffff);
            ptr->B = pBuffer + va_image.offsets[0];
            ptr->G = ptr->B;
            ptr->R = ptr->B;
            ptr->A = ptr->B;
        }
        else mfx_res = MFX_ERR_LOCK_MEMORY;
        break;
#ifdef MFX_ENABLE_RGBP
    case VA_FOURCC_RGBP:
        if (mfx_fourcc == MFX_FOURCC_RGBP)
        {
            ptr->PitchHigh = (mfxU16)(va_image.pitches[0] / (1 << 16));
            ptr->PitchLow  = (mfxU16)(va_image.pitches[0] % (1 << 16));
            ptr->B = pBuffer + va_image.offsets[0];
            ptr->G = pBuffer + va_image.offsets[1];
            ptr->R = pBuffer + va_image.offsets[2];
        }
        else mfx_res = MFX_ERR_LOCK_MEMORY;
        break;
#endif
    case VA_FOURCC_ABGR:
        if (mfx_fourcc == MFX_FOURCC_BGR4)
        {
            ptr->PitchHigh = (mfxU16)(va_image.pitches[0] / (1 << 16));
            ptr->PitchLow  = (mfxU16)(va_image.pitches[0] % (1 << 16));
            ptr->R = pBuffer + va_image.offsets[0];
            ptr->G = ptr->R + 1;
            ptr->B = ptr->R + 2;
            ptr->A = ptr->R + 3;
        }
        else mfx_res = MFX_ERR_LOCK_MEMORY;
        break;
    case VA_FOURCC_P208:
        if (mfx_fourcc == MFX_FOURCC_NV12)
        {
            ptr->PitchHigh = (mfxU16)(va_image.pitches[0] / (1 << 16));
            ptr->PitchLow  = (mfxU16)(va_image.pitches[0] % (1 << 16));
            ptr->Y = pBuffer + va_image.offsets[0];
        }
        else mfx_res = MFX_ERR_LOCK_MEMORY;
        break;
    case VA_FOURCC_P010:
        if (mfx_fourcc == MFX_FOURCC_P010)
        {
            ptr->PitchHigh = (mfxU16)(va_image.pitches[0] >> 16);
            ptr->PitchLow  = (mfxU16)(va_image.pitches[0] & 0xFFFF);
            ptr->Y = pBuffer + va_image.offsets[0];
            ptr->U = pBuffer + va_image.offsets[1];
            ptr->V = ptr->U + sizeof(mfxU16);
        }
        else mfx_res = MFX_ERR_LOCK_MEMORY;
        break;
    default:
        mfx_res = MFX_ERR_LOCK_MEMORY;
        break;
    }

    return mfx_res;
}

mfxStatus
mfxDefaultAllocatorVAAPI::LockFrameHW(
    mfxHDL         pthis,
    mfxMemId       mid,
    mfxFrameData*  ptr)
{
    MFX_CHECK(pthis, MFX_ERR_INVALID_HANDLE);
    mfxWideHWFrameAllocator *pSelf = (mfxWideHWFrameAllocator*)pthis;

    mfxStatus mfx_res = MFX_ERR_NONE;
    VAStatus  va_res  = VA_STATUS_SUCCESS;
    vaapiMemIdInt* vaapi_mids = (vaapiMemIdInt*)mid;
    mfxU8* pBuffer = 0;

    if (!vaapi_mids || !(vaapi_mids->m_surface)) return MFX_ERR_INVALID_HANDLE;

    mfxU32 mfx_fourcc = vaapi_mids->m_fourcc;
    if (MFX_FOURCC_P8 == mfx_fourcc)   // bitstream processing
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaMapBuffer");
        VACodedBufferSegment *coded_buffer_segment;
            va_res =  vaMapBuffer(pSelf->pVADisplay, *(vaapi_mids->m_surface), (void **)(&coded_buffer_segment));
        mfx_res = VA_TO_MFX_STATUS(va_res);
        if (MFX_ERR_NONE == mfx_res)
        {
                ptr->Y = (mfxU8*)coded_buffer_segment->buf;
        }
    }
    else
    {
        va_res = vaDeriveImage(pSelf->pVADisplay, *(vaapi_mids->m_surface), &(vaapi_mids->m_image));
        mfx_res = VA_TO_MFX_STATUS(va_res);

        if (MFX_ERR_NONE == mfx_res)
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaMapBuffer");
            va_res = vaMapBuffer(pSelf->pVADisplay, vaapi_mids->m_image.buf, (void **) &pBuffer);
            mfx_res = VA_TO_MFX_STATUS(va_res);
        }

        if (MFX_ERR_NONE == mfx_res)
        {
            mfx_res = SetFrameData(vaapi_mids->m_image, mfx_fourcc, pBuffer, ptr);
        }
    }
    return mfx_res;
}

mfxStatus mfxDefaultAllocatorVAAPI::UnlockFrameHW(
    mfxHDL         pthis,
    mfxMemId       mid,
    mfxFrameData*  ptr)
{
    // TBD
    if (!pthis)
        return MFX_ERR_INVALID_HANDLE;

    mfxWideHWFrameAllocator *pSelf = (mfxWideHWFrameAllocator*)pthis;

    vaapiMemIdInt* vaapi_mids = (vaapiMemIdInt*)mid;

    if (!vaapi_mids || !(vaapi_mids->m_surface)) return MFX_ERR_INVALID_HANDLE;

    mfxU32 mfx_fourcc = vaapi_mids->m_fourcc;

    if (MFX_FOURCC_P8 == mfx_fourcc)   // bitstream processing
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
        vaUnmapBuffer(pSelf->pVADisplay, *(vaapi_mids->m_surface));
    }
    else  // Image processing
    {
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
            vaUnmapBuffer(pSelf->pVADisplay, vaapi_mids->m_image.buf);
        }
        vaDestroyImage(pSelf->pVADisplay, vaapi_mids->m_image.image_id);

        if (NULL != ptr)
        {
            ptr->PitchLow  = 0;
            ptr->PitchHigh = 0;
            ptr->Y     = NULL;
            ptr->U     = NULL;
            ptr->V     = NULL;
            ptr->A     = NULL;
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
    if (!pthis)
        return MFX_ERR_INVALID_HANDLE;

    //mfxWideHWFrameAllocator *pSelf = (mfxWideHWFrameAllocator*)pthis;
    if (0 == mid)
        return MFX_ERR_INVALID_HANDLE;

    vaapiMemIdInt* vaapi_mids = (vaapiMemIdInt*)mid;

    if (!handle || !vaapi_mids || !(vaapi_mids->m_surface)) return MFX_ERR_INVALID_HANDLE;

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
