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

#include "umc_defs.h"

#include "mfx_umc_alloc_wrapper.h"
#include "mfx_common.h"
#include "libmfx_core.h"
#include "mfx_common_int.h"
#include "mfxfei.h"


#if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE)
#include "mfx_vpp_jpeg_d3d9.h"
#endif

mfx_UMC_MemAllocator::mfx_UMC_MemAllocator():m_pCore(NULL)
{
}

mfx_UMC_MemAllocator::~mfx_UMC_MemAllocator()
{
}

UMC::Status mfx_UMC_MemAllocator::InitMem(UMC::MemoryAllocatorParams *, VideoCORE* mfxCore)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    UMC::Status Sts = UMC::UMC_OK;
    if(!mfxCore)
        return UMC::UMC_ERR_NULL_PTR;
    m_pCore = mfxCore;
    return Sts;
}

UMC::Status mfx_UMC_MemAllocator::Close()
{
    UMC::AutomaticUMCMutex guard(m_guard);

    UMC::Status sts = UMC::UMC_OK;
    m_pCore = 0;
    return sts;
}

UMC::Status mfx_UMC_MemAllocator::Alloc(UMC::MemID *pNewMemID, size_t Size, uint32_t , uint32_t )
{
    UMC::AutomaticUMCMutex guard(m_guard);

    mfxMemId memId;
    mfxStatus Sts = m_pCore->AllocBuffer((mfxU32)Size, /*MFX_MEMTYPE_PERSISTENT_MEMORY*/ MFX_MEMTYPE_SYSTEM_MEMORY, &memId);
    MFX_CHECK_UMC_STS(Sts);
    *pNewMemID = ((UMC::MemID)memId + 1);
    return UMC::UMC_OK;
}

void* mfx_UMC_MemAllocator::Lock(UMC::MemID MID)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    mfxStatus Sts = MFX_ERR_NONE;

    mfxU8 *ptr;
    Sts = m_pCore->LockBuffer((mfxHDL)(MID - 1), &ptr);
    if (Sts < MFX_ERR_NONE)
        return 0;

    return ptr;
}

UMC::Status mfx_UMC_MemAllocator::Unlock(UMC::MemID MID)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    UMC::Status sts = UMC::UMC_OK;
    m_pCore->UnlockBuffer((mfxHDL)(MID - 1));
    return sts;
}

UMC::Status mfx_UMC_MemAllocator::Free(UMC::MemID MID)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    m_pCore->FreeBuffer((mfxHDL)(MID - 1));
    return UMC::UMC_OK;
}

UMC::Status mfx_UMC_MemAllocator::DeallocateMem(UMC::MemID )
{
    UMC::Status sts = UMC::UMC_OK;
    return sts;
}


////////////////////////////////////////////////////////////////////////////////////////////////
// mfx_UMC_FrameAllocator implementation
////////////////////////////////////////////////////////////////////////////////////////////////
mfx_UMC_FrameAllocator::FrameInformation::FrameInformation()
    : m_locks(0)
    , m_referenceCounter(0)
{
}

void mfx_UMC_FrameAllocator::FrameInformation::Reset()
{
    m_locks = 0;
    m_referenceCounter = 0;
}

mfx_UMC_FrameAllocator::mfx_UMC_FrameAllocator()
    : m_curIndex(-1),
      m_IsUseExternalFrames(true),
      m_sfcVideoPostProcessing(false),
      m_pCore(0),
      m_externalFramesResponse(0),
      m_isSWDecode(false),
      m_IOPattern(0)
{
}

mfx_UMC_FrameAllocator::~mfx_UMC_FrameAllocator()
{
    Close();
}

UMC::Status mfx_UMC_FrameAllocator::InitMfx(UMC::FrameAllocatorParams *,
                                            VideoCORE* mfxCore,
                                            const mfxVideoParam *params,
                                            const mfxFrameAllocRequest *request,
                                            mfxFrameAllocResponse *response,
                                            bool isUseExternalFrames,
                                            bool isSWplatform)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    m_isSWDecode = isSWplatform;

    if (!mfxCore || !params)
        return UMC::UMC_ERR_NULL_PTR;

    m_IOPattern = params->IOPattern;

    if (!isUseExternalFrames && (!request || !response))
        return UMC::UMC_ERR_NULL_PTR;

    m_pCore = mfxCore;
    m_IsUseExternalFrames = isUseExternalFrames;

    int32_t bit_depth;
    if (params->mfx.FrameInfo.FourCC == MFX_FOURCC_P010 ||
        params->mfx.FrameInfo.FourCC == MFX_FOURCC_P210
        )
        bit_depth = 10;
    else
        bit_depth = 8;

    UMC::ColorFormat color_format;

    switch (params->mfx.FrameInfo.FourCC)
    {
    case MFX_FOURCC_NV12:
        color_format = UMC::NV12;
        break;
    case MFX_FOURCC_P010:
        color_format = UMC::NV12;
        break;
    case MFX_FOURCC_NV16:
        color_format = UMC::NV16;
        break;
    case MFX_FOURCC_P210:
        color_format = UMC::NV16;
        break;
    case MFX_FOURCC_RGB4:
        color_format = UMC::RGB32;
        break;
    case MFX_FOURCC_YV12:
        color_format = UMC::YUV420;
        break;
    case MFX_FOURCC_YUY2:
        color_format = UMC::YUY2;
        break;
    case MFX_FOURCC_AYUV:
        color_format = UMC::AYUV;
        break;
    default:
        return UMC::UMC_ERR_UNSUPPORTED;
    }

    UMC::Status umcSts = m_info.Init(params->mfx.FrameInfo.Width, params->mfx.FrameInfo.Height, color_format, bit_depth);

    m_surface.Info.Width = params->mfx.FrameInfo.Width;
    m_surface.Info.Height = params->mfx.FrameInfo.Height;
    m_surface.Info.BitDepthLuma = params->mfx.FrameInfo.BitDepthLuma;
    m_surface.Info.BitDepthChroma = params->mfx.FrameInfo.BitDepthChroma;

    if (umcSts != UMC::UMC_OK)
        return umcSts;

    if (!m_IsUseExternalFrames || !m_isSWDecode)
    {
        m_frameData.resize(response->NumFrameActual);
        m_extSurfaces.resize(response->NumFrameActual);

        for (mfxU32 i = 0; i < response->NumFrameActual; i++)
        {
            m_frameData[i].first.Data.MemId = response->mids[i];
            m_frameData[i].first.Data.MemType = request->Type;
            memcpy_s(&m_frameData[i].first.Info, sizeof(mfxFrameInfo), &request->Info, sizeof(mfxFrameInfo));

            // fill UMC frameData
            FrameInformation * frameMID = &m_frameData[i].second;
            frameMID->Reset();
            UMC::FrameData* frameData = &frameMID->m_frame;

            // set correct width & height to planes
            frameData->Init(&m_info, (UMC::FrameMemID)i, this);
        }
    }
    else
    {
        // make sure AddSurface() call would not result in UMC::FrameData reallocation
        // as it will result in UB in IncreaseReference&DecreaseReference as m_frameData would be in unspecified state
        m_frameData.reserve(response->NumFrameActual);
        m_extSurfaces.reserve(response->NumFrameActual);
    }

    mfxCore->SetWrapper(this);

    return UMC::UMC_OK;
}


UMC::Status mfx_UMC_FrameAllocator::Close()
{
    UMC::AutomaticUMCMutex guard(m_guard);

    Reset();
    m_frameData.clear();
    m_extSurfaces.clear();
    return UMC::UMC_OK;
}

void mfx_UMC_FrameAllocator::SetExternalFramesResponse(mfxFrameAllocResponse *response)
{
    m_externalFramesResponse = 0;

    if (!response || !response->NumFrameActual)
        return;

    m_externalFramesResponse = response;
}

UMC::Status mfx_UMC_FrameAllocator::Reset()
{
    UMC::AutomaticUMCMutex guard(m_guard);

    m_curIndex = -1;
    mfxStatus sts = MFX_ERR_NONE;

    // unlock internal sufraces
    for (mfxU32 i = 0; i < m_frameData.size(); i++)
    {
        m_frameData[i].first.Data.Locked = 0;  // if app ext allocator then should decrease Locked counter same times as locked by medisSDK
        m_frameData[i].second.Reset();
    }

    // free external sufraces
    for (mfxU32 i = 0; i < m_extSurfaces.size(); i++)
    {
        if (m_extSurfaces[i].isUsed)
        {
            sts = m_pCore->DecreaseReference(&m_extSurfaces[i].FrameSurface->Data);
            if (sts < MFX_ERR_NONE)
                return UMC::UMC_ERR_FAILED;
            m_extSurfaces[i].isUsed = false;
        }

        m_extSurfaces[i].FrameSurface = 0;
    }

    if (m_IsUseExternalFrames && m_isSWDecode)
    {
        m_extSurfaces.clear();
    }

    return UMC::UMC_OK;
}

UMC::Status mfx_UMC_FrameAllocator::GetFrameHandle(UMC::FrameMemID memId, void * handle)
{
    if (m_pCore->GetFrameHDL(ConvertMemId(memId), (mfxHDL*)handle) != MFX_ERR_NONE)
        return UMC::UMC_ERR_ALLOC;

    return UMC::UMC_OK;
}

UMC::Status mfx_UMC_FrameAllocator::Alloc(UMC::FrameMemID *pNewMemID, const UMC::VideoDataInfo * info, uint32_t )
{
    UMC::AutomaticUMCMutex guard(m_guard);

    mfxStatus sts = MFX_ERR_NONE;
    if (!pNewMemID)
        return UMC::UMC_ERR_NULL_PTR;

    mfxI32 index = FindFreeSurface();
    if (index == -1)
    {
        *pNewMemID = UMC::FRAME_MID_INVALID;
        return UMC::UMC_ERR_ALLOC;
    }

    *pNewMemID = (UMC::FrameMemID)index;

    mfxSize allocated = {m_frameData[index].first.Info.Width, m_frameData[index].first.Info.Height};
    mfxSize passed = {static_cast<int>(info->GetWidth()), static_cast<int>(info->GetHeight())};
    UMC::ColorFormat colorFormat = m_info.GetColorFormat();

    switch(colorFormat)
    {
    case UMC::YUV420:
    case UMC::GRAY:
    case UMC::YV12:
    case UMC::YUV422:
    case UMC::NV12:
    case UMC::NV16:
    case UMC::YUY2:
    case UMC::IMC3:
    case UMC::RGB32:
    case UMC::AYUV:
    case UMC::YUV444:
    case UMC::YUV411:
    case UMC::Y210:
    case UMC::Y216:
    case UMC::Y410:
    case UMC::Y416:
        break;
    default:
        return UMC::UMC_ERR_UNSUPPORTED;
    }

    if (colorFormat == UMC::NV12 && info->GetColorFormat() == UMC::NV12)
    {
        if ((m_info.GetPlaneSampleSize(0) != info->GetPlaneSampleSize(0)) || (m_info.GetPlaneSampleSize(1) != info->GetPlaneSampleSize(1)))
            return UMC::UMC_ERR_UNSUPPORTED;
    }

    if (passed.width > allocated.width ||
        passed.height > allocated.height)
    {
        return UMC::UMC_ERR_UNSUPPORTED;
    }

    sts = m_pCore->IncreasePureReference(m_frameData[index].first.Data.Locked);
    if (sts < MFX_ERR_NONE)
        return UMC::UMC_ERR_FAILED;

    if ((m_IsUseExternalFrames) || (m_sfcVideoPostProcessing))
    {
        if (m_extSurfaces[index].FrameSurface)
        {
            sts = m_pCore->IncreaseReference(&m_extSurfaces[index].FrameSurface->Data);
            if (sts < MFX_ERR_NONE)
                return UMC::UMC_ERR_FAILED;

            m_extSurfaces[m_curIndex].isUsed = true;
        }
    }

    m_frameData[index].second.Reset();
    m_curIndex = -1;

    return UMC::UMC_OK;
}

const UMC::FrameData* mfx_UMC_FrameAllocator::Lock(UMC::FrameMemID mid)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    mfxU32 index = (mfxU32)mid;
    if (index >= m_frameData.size())
        return 0;

    mfxFrameData *data = 0;

    mfxFrameSurface1 check_surface;
    check_surface.Info.FourCC = m_frameData[index].first.Info.FourCC;

    if (m_IsUseExternalFrames)
    {
        if (m_frameData[index].first.Data.MemId != 0)
        {
            data = &m_frameData[index].first.Data;
            mfxStatus sts = m_pCore->LockExternalFrame(m_frameData[index].first.Data.MemId, data);

            if (sts < MFX_ERR_NONE || !data)
                return 0;

            check_surface.Data = *data;
            check_surface.Data.MemId = 0;
            sts = CheckFrameData(&check_surface);
            if (sts < MFX_ERR_NONE)
                return 0;
        }
        else
        {
            data = &m_extSurfaces[index].FrameSurface->Data;
        }
    }
    else
    {
        if (m_frameData[index].first.Data.MemId != 0)
        {
            data = &m_frameData[index].first.Data;
            mfxStatus sts = m_pCore->LockFrame(m_frameData[index].first.Data.MemId, data);

            if (sts < MFX_ERR_NONE || !data)
                return 0;

            check_surface.Data = *data;
            check_surface.Data.MemId = 0;
            sts = CheckFrameData(&check_surface);
            if (sts < MFX_ERR_NONE)
                return 0;
        }
        else // invalid situation, we always allocate internal frames with MemId
            return 0;
    }

    FrameInformation * frameMID = &m_frameData[index].second;
    UMC::FrameData* frameData = &frameMID->m_frame;
    mfxU32 pitch = data->PitchLow + ((mfxU32)data->PitchHigh << 16);

    switch (frameData->GetInfo()->GetColorFormat())
    {
    case UMC::NV16:
    case UMC::NV12:
        frameData->SetPlanePointer(data->Y, 0, pitch);
        frameData->SetPlanePointer(data->U, 1, pitch);
        break;
    case UMC::YUV420:
    case UMC::YUV422:
        frameData->SetPlanePointer(data->Y, 0, pitch);
        frameData->SetPlanePointer(data->U, 1, pitch >> 1);
        frameData->SetPlanePointer(data->V, 2, pitch >> 1);
        break;
    case UMC::IMC3:
        frameData->SetPlanePointer(data->Y, 0, pitch);
        frameData->SetPlanePointer(data->U, 1, pitch);
        frameData->SetPlanePointer(data->V, 2, pitch);
        break;
    case UMC::RGB32:
        {
            frameData->SetPlanePointer(data->B, 0, pitch);
        }
        break;
    case UMC::YUY2:
        {
            frameData->SetPlanePointer(data->Y, 0, pitch);
        }
        break;
    default:
        if (m_frameData[index].first.Data.MemId)
        {
            if (m_IsUseExternalFrames)
            {
                m_pCore->UnlockExternalFrame(m_extSurfaces[index].FrameSurface->Data.MemId);
            }
            else
            {
                m_pCore->UnlockFrame(m_frameData[index].first.Data.MemId);
            }
        }
        return 0;
    }

    //frameMID->m_locks++;
    return frameData;
}

UMC::Status mfx_UMC_FrameAllocator::Unlock(UMC::FrameMemID mid)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    mfxU32 index = (mfxU32)mid;
    if (index >= m_frameData.size())
        return UMC::UMC_ERR_FAILED;

    if (m_frameData[index].first.Data.MemId)
    {
        mfxStatus sts;
        if (m_IsUseExternalFrames)
            sts = m_pCore->UnlockExternalFrame(m_extSurfaces[index].FrameSurface->Data.MemId);
        else
            sts = m_pCore->UnlockFrame(m_frameData[index].first.Data.MemId);

        if (sts < MFX_ERR_NONE)
            return UMC::UMC_ERR_FAILED;
    }

    return UMC::UMC_OK;
}

UMC::Status mfx_UMC_FrameAllocator::IncreaseReference(UMC::FrameMemID mid)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    mfxU32 index = (mfxU32)mid;
    if (index >= m_frameData.size())
        return UMC::UMC_ERR_FAILED;

    FrameInformation * frameMID = &m_frameData[index].second;
    frameMID->m_referenceCounter++;

    return UMC::UMC_OK;
}

UMC::Status mfx_UMC_FrameAllocator::DecreaseReference(UMC::FrameMemID mid)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    mfxU32 index = (mfxU32)mid;
    if (index >= m_frameData.size())
        return UMC::UMC_ERR_FAILED;

    FrameInformation * frameMID = &m_frameData[index].second;

    frameMID->m_referenceCounter--;
    if (!frameMID->m_referenceCounter)
    {
        if (frameMID->m_locks)
        {
            frameMID->m_referenceCounter++;
            return UMC::UMC_ERR_FAILED;
        }

        return Free(mid);
    }

    return UMC::UMC_OK;
}

UMC::Status mfx_UMC_FrameAllocator::Free(UMC::FrameMemID mid)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    mfxStatus sts = MFX_ERR_NONE;
    mfxU32 index = (mfxU32)mid;
    if (index >= m_frameData.size())
        return UMC::UMC_ERR_FAILED;

    sts = m_pCore->DecreasePureReference(m_frameData[index].first.Data.Locked);
    if (sts < MFX_ERR_NONE)
        return UMC::UMC_ERR_FAILED;

    if ((m_IsUseExternalFrames) || (m_sfcVideoPostProcessing))
    {
        sts = m_pCore->DecreaseReference(&m_extSurfaces[index].FrameSurface->Data);
        if (sts < MFX_ERR_NONE)
            return UMC::UMC_ERR_FAILED;
        m_extSurfaces[index].isUsed = false;
    }

    return UMC::UMC_OK;
}

mfxStatus mfx_UMC_FrameAllocator::SetCurrentMFXSurface(mfxFrameSurface1 *surf, bool isOpaq)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    if (surf->Data.Locked)
        return MFX_ERR_MORE_SURFACE;

    // check input surface

    if ((surf->Info.BitDepthLuma ? surf->Info.BitDepthLuma : 8) != (m_surface.Info.BitDepthLuma ? m_surface.Info.BitDepthLuma : 8))
        return MFX_ERR_INVALID_VIDEO_PARAM;

    if ((surf->Info.BitDepthChroma ? surf->Info.BitDepthChroma : 8) != (m_surface.Info.BitDepthChroma ? m_surface.Info.BitDepthChroma : 8))
        return MFX_ERR_INVALID_VIDEO_PARAM;

    if (surf->Info.FourCC == MFX_FOURCC_P010 || surf->Info.FourCC == MFX_FOURCC_P210)
    {
        if (m_isSWDecode)
        {
            if (surf->Info.Shift != 0)
                return MFX_ERR_INVALID_VIDEO_PARAM;
        }
        else
        {
            if ((m_IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) && surf->Info.Shift != 1)
                return MFX_ERR_INVALID_VIDEO_PARAM;
        }
    }

    mfxExtBuffer* extbuf = GetExtendedBuffer(surf->Data.ExtParam, surf->Data.NumExtParam, MFX_EXTBUFF_FEI_DEC_STREAM_OUT);
    if (extbuf && !m_IsUseExternalFrames)
        return MFX_ERR_INVALID_VIDEO_PARAM;

    if (m_externalFramesResponse && surf->Data.MemId)
    {
        bool isFound = false;
        for (mfxI32 i = 0; i < m_externalFramesResponse->NumFrameActual; i++)
        {
            if (!isOpaq)
            {
                if (m_pCore->MapIdx(m_externalFramesResponse->mids[i]) == surf->Data.MemId)
                {
                    isFound = true;
                    break;
                }
            }
            else // opaque means that we are working with internal surface as with external
            {
                if (m_externalFramesResponse->mids[i] == surf->Data.MemId)
                {
                    isFound = true;
                    break;
                }
            }
        }

        if (!isFound)
            return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    m_curIndex = -1;

    if ((!m_IsUseExternalFrames) && (!m_sfcVideoPostProcessing))
        m_curIndex = FindFreeSurface();
    else if ((!m_IsUseExternalFrames) && (m_sfcVideoPostProcessing))
    {
        for (mfxU32 i = 0; i < m_extSurfaces.size(); i++)
        {
            if (NULL == m_extSurfaces[i].FrameSurface)
            {
                /* new surface */
                m_curIndex = i;
                m_extSurfaces[m_curIndex].FrameSurface = surf;
                break;
            }
            if ( (NULL != m_extSurfaces[i].FrameSurface) &&
                  (0 == m_extSurfaces[i].FrameSurface->Data.Locked) &&
                  (m_extSurfaces[i].FrameSurface->Data.MemId == surf->Data.MemId) &&
                  (0 == m_frameData[i].first.Data.Locked) )
            {
                /* surfaces filled already */
                m_curIndex = i;
                m_extSurfaces[m_curIndex].FrameSurface = surf;
                break;
            }
        } // for (mfxU32 i = 0; i < m_extSurfaces.size(); i++)
    }
    else
    {
        m_curIndex = FindSurface(surf, isOpaq);

        if (m_curIndex != -1)
        {
            m_extSurfaces[m_curIndex].FrameSurface = surf;
            if (m_frameData[m_curIndex].first.Data.Locked) // surface was locked yet
            {
                m_curIndex = -1;
            }

            // update info
            m_frameData[m_curIndex].first.Info = surf->Info;
        }
        else
        {
            m_curIndex = AddSurface(surf);
            if (m_curIndex != -1)
                m_extSurfaces[m_curIndex].FrameSurface = surf;
        }
    }

    return MFX_ERR_NONE;
}

mfxI32 mfx_UMC_FrameAllocator::AddSurface(mfxFrameSurface1 *surface)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    mfxI32 index = -1;

    if (!m_IsUseExternalFrames)
        return -1;

    if (surface->Data.MemId && !m_isSWDecode)
    {
        mfxU32 i;
        for (i = 0; i < m_extSurfaces.size(); i++)
        {
            if (surface->Data.MemId == m_pCore->MapIdx(m_frameData[i].first.Data.MemId))
            {
                m_extSurfaces[i].FrameSurface = surface;
                index = i;
                break;
            }
        }
    }
    else
    {
        m_extSurfaces.push_back(surf_descr(surface,false));
        index = (mfxI32)(m_extSurfaces.size() - 1);
    }

    switch (surface->Info.FourCC)
    {
    case MFX_FOURCC_NV12:
    case MFX_FOURCC_NV16:
    case MFX_FOURCC_YV12:
    case MFX_FOURCC_YUY2:
    case MFX_FOURCC_RGB4:
    case MFX_FOURCC_AYUV:
    case MFX_FOURCC_P010:
    case MFX_FOURCC_P210:
        break;
    default:
        return -1;
    }

    if (m_IsUseExternalFrames && m_isSWDecode)
    {
        FrameInfo  frameInfo;
        m_frameData.push_back(frameInfo); // potentially risky line, as UMC::FrameData deep copy ask this allocator to 
                                          // to increase/decrease frame references

        memset(&(m_frameData[index].first), 0, sizeof(m_frameData[index].first));
        m_frameData[index].first.Data.MemId = surface->Data.MemId;
        m_frameData[index].first.Info = surface->Info;

        // fill UMC frameData
        FrameInformation * frameMID = &m_frameData[index].second;
        frameMID->Reset();
        UMC::FrameData* frameData = &frameMID->m_frame;

        // set correct width & height to planes
        frameData->Init(&m_info, (UMC::FrameMemID)index, this);
    }

    return index;
}

mfxI32 mfx_UMC_FrameAllocator::FindSurface(mfxFrameSurface1 *surf, bool isOpaq)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    if (!surf)
        return -1;

    mfxFrameData * data = &surf->Data;

    if (data->MemId && m_IsUseExternalFrames)
    {
        mfxMemId    sMemId;
        for (mfxU32 i = 0; i < m_frameData.size(); i++)
        {
            sMemId = isOpaq == true ? m_frameData[i].first.Data.MemId : m_pCore->MapIdx(m_frameData[i].first.Data.MemId);
            if (sMemId == data->MemId)
            {
                return i;
            }
        }
    }

    for (mfxU32 i = 0; i < m_extSurfaces.size(); i++)
    {
        if (m_extSurfaces[i].FrameSurface == surf)
        {
            return i;
        }
    }

    return -1;
}

mfxI32 mfx_UMC_FrameAllocator::FindFreeSurface()
{
    UMC::AutomaticUMCMutex guard(m_guard);

    if ((m_IsUseExternalFrames) || (m_sfcVideoPostProcessing))
    {
        return m_curIndex;
    }

    if (m_curIndex != -1)
        return m_curIndex;

    for (mfxU32 i = 0; i < m_frameData.size(); i++)
    {
        if (!m_frameData[i].first.Data.Locked)
        {
            return i;
        }
    }

    return -1;
}

mfxFrameSurface1 * mfx_UMC_FrameAllocator::GetInternalSurface(UMC::FrameMemID index)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    if (m_IsUseExternalFrames)
    {
        return 0;
    }

    if (index >= 0)
    {
        if ((uint32_t)index >= m_frameData.size())
            return 0;
        return &m_frameData[index].first;
    }

    return 0;
}

mfxFrameSurface1 * mfx_UMC_FrameAllocator::GetSurfaceByIndex(UMC::FrameMemID index)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    if (index < 0)
        return 0;

    if ((uint32_t)index >= m_frameData.size())
        return 0;

    return m_IsUseExternalFrames ? m_extSurfaces[index].FrameSurface : &m_frameData[index].first;
}

void mfx_UMC_FrameAllocator::SetSfcPostProcessingFlag(bool flagToSet)
{
    m_sfcVideoPostProcessing = flagToSet;
}

mfxFrameSurface1 * mfx_UMC_FrameAllocator::GetSurface(UMC::FrameMemID index, mfxFrameSurface1 *surface, const mfxVideoParam * videoPar)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    if (!surface || !videoPar || 0 > index)
        return 0;

    if ((m_IsUseExternalFrames) || (m_sfcVideoPostProcessing))
    {
        if ((uint32_t)index >= m_frameData.size())
            return 0;
        return m_extSurfaces[index].FrameSurface;
    }
    else
    {
        mfxStatus sts = m_pCore->IncreaseReference(&surface->Data);
        if (sts < MFX_ERR_NONE)
            return 0;

        m_extSurfaces[index].FrameSurface = surface;
    }

    return surface;
}

mfxStatus mfx_UMC_FrameAllocator::PrepareToOutput(mfxFrameSurface1 *surface_work, UMC::FrameMemID index, const mfxVideoParam *, bool isOpaq)
{
    UMC::AutomaticUMCMutex guard(m_guard);
    mfxStatus sts;
    mfxU16 dstMemType = isOpaq?(MFX_MEMTYPE_INTERNAL_FRAME | MFX_MEMTYPE_DXVA2_DECODER_TARGET):(MFX_MEMTYPE_EXTERNAL_FRAME | MFX_MEMTYPE_DXVA2_DECODER_TARGET);
    

    UMC::FrameData* frame = &m_frameData[index].second.m_frame;

    if (m_IsUseExternalFrames)
        return MFX_ERR_NONE;

    m_surface.Info.Width = (mfxU16)frame->GetInfo()->GetWidth();
    m_surface.Info.Height = (mfxU16)frame->GetInfo()->GetHeight();

    switch (frame->GetInfo()->GetColorFormat())
    {
    case UMC::NV12:
        m_surface.Data.Y = frame->GetPlaneMemoryInfo(0)->m_planePtr;
        m_surface.Data.UV = frame->GetPlaneMemoryInfo(1)->m_planePtr;
        m_surface.Data.PitchHigh = (mfxU16)(frame->GetPlaneMemoryInfo(0)->m_pitch / (1 << 16));
        m_surface.Data.PitchLow  = (mfxU16)(frame->GetPlaneMemoryInfo(0)->m_pitch % (1 << 16));
        break;

    case UMC::YUV420:
        m_surface.Data.Y = frame->GetPlaneMemoryInfo(0)->m_planePtr;
        m_surface.Data.U = frame->GetPlaneMemoryInfo(1)->m_planePtr;
        m_surface.Data.V = frame->GetPlaneMemoryInfo(2)->m_planePtr;
        m_surface.Data.PitchHigh = (mfxU16)(frame->GetPlaneMemoryInfo(0)->m_pitch / (1 << 16));
        m_surface.Data.PitchLow  = (mfxU16)(frame->GetPlaneMemoryInfo(0)->m_pitch % (1 << 16));
        break;

    case UMC::IMC3:
        m_surface.Data.Y = frame->GetPlaneMemoryInfo(0)->m_planePtr;
        m_surface.Data.U = frame->GetPlaneMemoryInfo(1)->m_planePtr;
        m_surface.Data.V = frame->GetPlaneMemoryInfo(2)->m_planePtr;
        m_surface.Data.PitchHigh = (mfxU16)(frame->GetPlaneMemoryInfo(0)->m_pitch / (1 << 16));
        m_surface.Data.PitchLow  = (mfxU16)(frame->GetPlaneMemoryInfo(0)->m_pitch % (1 << 16));
        break;

    case UMC::RGB32:
        m_surface.Data.B = frame->GetPlaneMemoryInfo(0)->m_planePtr;
        m_surface.Data.G = m_surface.Data.B + 1;
        m_surface.Data.R = m_surface.Data.B + 2;
        m_surface.Data.A = m_surface.Data.B + 3;
        m_surface.Data.PitchHigh = (mfxU16)(frame->GetPlaneMemoryInfo(0)->m_pitch / (1 << 16));
        m_surface.Data.PitchLow  = (mfxU16)(frame->GetPlaneMemoryInfo(0)->m_pitch % (1 << 16));
        break;

    case UMC::YUY2:
        m_surface.Data.Y = frame->GetPlaneMemoryInfo(0)->m_planePtr;
        m_surface.Data.U = m_surface.Data.Y + 1;
        m_surface.Data.V = m_surface.Data.Y + 3;
        m_surface.Data.PitchHigh = (mfxU16)(frame->GetPlaneMemoryInfo(0)->m_pitch / (1 << 16));
        m_surface.Data.PitchLow  = (mfxU16)(frame->GetPlaneMemoryInfo(0)->m_pitch % (1 << 16));
        break;

    default:
        return MFX_ERR_UNSUPPORTED;
    }
    m_surface.Info.FourCC = surface_work->Info.FourCC;
    m_surface.Info.Shift = m_IsUseExternalFrames ? m_extSurfaces[index].FrameSurface->Info.Shift : m_frameData[index].first.Info.Shift;
    sts = m_pCore->DoFastCopyWrapper(surface_work,
                                     dstMemType,
                                     &m_surface,
                                     MFX_MEMTYPE_INTERNAL_FRAME | MFX_MEMTYPE_SYSTEM_MEMORY);

    MFX_CHECK_STS(sts);

    if (!m_IsUseExternalFrames)
    {
        mfxStatus temp_sts = m_pCore->DecreaseReference(&surface_work->Data);

        if (temp_sts < MFX_ERR_NONE && sts >= MFX_ERR_NONE)
        {
            sts = temp_sts;
        }

        m_extSurfaces[index].FrameSurface = 0;
    }

    return sts;
}


// D3D functionality
// we should copy to external SW surface
mfxStatus   mfx_UMC_FrameAllocator_D3D::PrepareToOutput(mfxFrameSurface1 *surface_work, UMC::FrameMemID index, const mfxVideoParam *,bool isOpaq)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    mfxStatus sts = MFX_ERR_NONE;
    mfxMemId memId = isOpaq?(m_frameData[index].first.Data.MemId):(m_pCore->MapIdx(m_frameData[index].first.Data.MemId));

    if ((surface_work->Data.MemId)&&
        (surface_work->Data.MemId == memId))
    {
        // all frames are external. No need to do anything
        return MFX_ERR_NONE;
    }
    else
    {
        if (!m_sfcVideoPostProcessing)
        {
            UMC::VideoDataInfo VInfo;

            mfxMemId idx = m_frameData[index].first.Data.MemId;
            memset(&m_surface.Data,0,sizeof(mfxFrameData));
            m_surface.Info = m_frameData[index].first.Info;
            m_surface.Data.Y = 0;
            m_surface.Data.MemId = idx;
            sts = m_pCore->DoFastCopyWrapper(surface_work,
                                             MFX_MEMTYPE_EXTERNAL_FRAME | MFX_MEMTYPE_SYSTEM_MEMORY,
                                             &m_surface,

                                             MFX_MEMTYPE_INTERNAL_FRAME | MFX_MEMTYPE_DXVA2_DECODER_TARGET
                                             );
            MFX_CHECK_STS(sts);
        }

        if (!m_IsUseExternalFrames)
        {
            if (!m_sfcVideoPostProcessing)
            {
                m_pCore->DecreaseReference(&surface_work->Data);
                m_extSurfaces[index].FrameSurface = 0;
            }
        }

        return sts;
    }
}

#if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE)
mfxStatus DoHwJpegCc(VideoVppJpegD3D9 **pCc, 
                     VideoCORE* mfxCore, 
                     mfxFrameSurface1 *pDst, 
                     mfxFrameSurface1 *pSrc, 
                     const mfxVideoParam * par, 
                     bool isD3DToSys, 
                     mfxU16 *taskId)
{
    mfxStatus sts = MFX_ERR_NONE;

    if (*pCc == NULL)
    {
        *pCc = new VideoVppJpegD3D9(mfxCore, isD3DToSys,(par->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY)?true:false);

        sts = (*pCc)->Init(par);
        MFX_CHECK_STS( sts );
    }

    sts = (*pCc)->BeginHwJpegProcessing(pSrc, pDst, taskId);

    return sts;
}

mfxStatus DoHwJpegCcFw(VideoVppJpegD3D9 **pCc, 
                       VideoCORE* mfxCore, 
                       mfxFrameSurface1 *pDst, 
                       mfxFrameSurface1 *pSrcTop, 
                       mfxFrameSurface1 *pSrcBottom, 
                       const mfxVideoParam * par, 
                       bool isD3DToSys, 
                       mfxU16 *taskId)
{
    mfxStatus sts = MFX_ERR_NONE;

    if (*pCc == NULL)
    {
        *pCc = new VideoVppJpegD3D9(mfxCore, isD3DToSys,(par->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY)?true:false);

        sts = (*pCc)->Init(par);
        MFX_CHECK_STS( sts );
    }

    sts = (*pCc)->BeginHwJpegProcessing(pSrcTop, pSrcBottom, pDst, taskId);

    return sts;
}

UMC::Status mfx_UMC_FrameAllocator_D3D_Converter::InitMfx(UMC::FrameAllocatorParams *,
                                                          VideoCORE* mfxCore,
                                                          const mfxVideoParam *params,
                                                          const mfxFrameAllocRequest *request,
                                                          mfxFrameAllocResponse *response,
                                                          bool isUseExternalFrames,
                                                          bool isSWplatform)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    m_isSWDecode = isSWplatform;

    if (!mfxCore || !params)
        return UMC::UMC_ERR_NULL_PTR;

    if (!isUseExternalFrames && (!request || !response))
        return UMC::UMC_ERR_NULL_PTR;

    m_pCore = mfxCore;
    m_IsUseExternalFrames = isUseExternalFrames;

    UMC::ColorFormat color_format;

    switch (params->mfx.FrameInfo.FourCC)
    {
    case MFX_FOURCC_NV12:
        color_format = UMC::NV12;
        break;
    case MFX_FOURCC_RGB4:
        color_format = UMC::RGB32;
        break;
    //case MFX_FOURCC_YV12:
    //    color_format = UMC::YUV420;
    //    break;
    case MFX_FOURCC_YUV400:
        color_format = UMC::GRAY;
        break;
    case MFX_FOURCC_IMC3:
        color_format = UMC::IMC3;
        break;
    case MFX_FOURCC_YUV422H:
    case MFX_FOURCC_YUV422V:
        color_format = UMC::YUV422;
        break;
    case MFX_FOURCC_YUV444:
        color_format = UMC::YUV444;
        break;
    case MFX_FOURCC_YUV411:
        color_format = UMC::YUV411;
        break;
    case MFX_FOURCC_RGBP:
        color_format = UMC::YUV444;
        break;
    default:
        return UMC::UMC_ERR_UNSUPPORTED;
    }

    UMC::Status umcSts = m_info.Init(params->mfx.FrameInfo.Width, params->mfx.FrameInfo.Height, color_format, 8);

    m_surface.Info.Width = params->mfx.FrameInfo.Width;
    m_surface.Info.Height = params->mfx.FrameInfo.Height;
    m_surface.Info.BitDepthLuma = params->mfx.FrameInfo.BitDepthLuma;
    m_surface.Info.BitDepthChroma = params->mfx.FrameInfo.BitDepthChroma;

    if (umcSts != UMC::UMC_OK)
        return umcSts;

    if (!m_IsUseExternalFrames ||
        !m_isSWDecode)
    {
        m_frameData.resize(response->NumFrameActual);
        m_extSurfaces.resize(response->NumFrameActual);

        for (mfxU32 i = 0; i < response->NumFrameActual; i++)
        {
            m_frameData[i].first.Data.MemId = response->mids[i];

            MFX_INTERNAL_CPY(&m_frameData[i].first.Info, &request->Info, sizeof(mfxFrameInfo));

            // fill UMC frameData
            FrameInformation * frameMID = &m_frameData[i].second;
            frameMID->Reset();
            UMC::FrameData* frameData = &frameMID->m_frame;

            // set correct width & height to planes
            frameData->Init(&m_info, (UMC::FrameMemID)i, this);
        }
    }

    mfxCore->SetWrapper(this);

    return UMC::UMC_OK;
}

mfxStatus mfx_UMC_FrameAllocator_D3D_Converter::StartPreparingToOutput(mfxFrameSurface1 *surface_work,
                                                                       UMC::FrameData* in,
                                                                       const mfxVideoParam * par,
                                                                       VideoVppJpegD3D9 **pCc,
                                                                       mfxU16 *taskId,
                                                                       bool isOpaq)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    mfxStatus sts = MFX_ERR_NONE;
    bool isD3DToSys = false;

    if(par->mfx.FrameInfo.PicStruct == MFX_PICSTRUCT_PROGRESSIVE)
    {
        UMC::FrameMemID index = in->GetFrameMID();

        mfxMemId memId = isOpaq?(m_frameData[index].first.Data.MemId):(m_pCore->MapIdx(m_frameData[index].first.Data.MemId));

        mfxHDLPair pPair;
        if(isOpaq)
            sts = m_pCore->GetFrameHDL(surface_work->Data.MemId, (mfxHDL*)&pPair);
        else
            sts = m_pCore->GetExternalFrameHDL(surface_work->Data.MemId, (mfxHDL*)&pPair);

        if ((pPair.first)&&
            (pPair.first == memId))
        {
            // all frames are external. No need to do anything
            return MFX_ERR_NONE;
        }
        else
        {
            mfxFrameSurface1 pSrc;

            pSrc = m_frameData[index].first;
            if(par->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY)
            {
                isD3DToSys = true;
            }
            else if (par->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY)
            {
                mfxExtOpaqueSurfaceAlloc *pOpaqAlloc = (mfxExtOpaqueSurfaceAlloc *)GetExtendedBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
                if (!pOpaqAlloc)
                    return MFX_ERR_INVALID_VIDEO_PARAM;

                isD3DToSys = !((pOpaqAlloc->Out.Type & MFX_MEMTYPE_SYSTEM_MEMORY) == 0);
            }

            sts = DoHwJpegCc(pCc, m_pCore, surface_work, &pSrc, par, isD3DToSys, taskId);
            if (sts < MFX_ERR_NONE)
                return sts;

            return MFX_ERR_NONE;
        }
    }
    else
    {
        UMC::FrameMemID indexTop = in[0].GetFrameMID();
        UMC::FrameMemID indexBottom = in[1].GetFrameMID();

        // Opaque for interlaced content currently is unsupported
        mfxMemId memId = isOpaq?(m_frameData[indexTop].first.Data.MemId):(m_pCore->MapIdx(m_frameData[indexTop].first.Data.MemId));

        mfxHDLPair pPair;
        if(isOpaq)
            sts = m_pCore->GetFrameHDL(surface_work->Data.MemId, (mfxHDL*)&pPair);
        else
            sts = m_pCore->GetExternalFrameHDL(surface_work->Data.MemId, (mfxHDL*)&pPair);

        if ((pPair.first)&&
            (pPair.first == memId))
        {
            // all frames are external. No need to do anything
            return MFX_ERR_NONE;
        }
        else
        {
            mfxFrameSurface1 pSrcTop, pSrcBottom;

            pSrcTop = m_frameData[indexTop].first;
            pSrcBottom = m_frameData[indexBottom].first;

            if(par->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY)
            {
                isD3DToSys = true;
            }
            else if (par->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY)
            {
                mfxExtOpaqueSurfaceAlloc *pOpaqAlloc = (mfxExtOpaqueSurfaceAlloc *)GetExtendedBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
                if (!pOpaqAlloc)
                    return MFX_ERR_INVALID_VIDEO_PARAM;

                isD3DToSys = !((pOpaqAlloc->Out.Type & MFX_MEMTYPE_SYSTEM_MEMORY) == 0);
            }

            sts = DoHwJpegCcFw(pCc, m_pCore, surface_work, &pSrcTop, &pSrcBottom, par, isD3DToSys, taskId);
            if (sts < MFX_ERR_NONE)
                return sts;

            return MFX_ERR_NONE;
        }
    }
}

mfxStatus mfx_UMC_FrameAllocator_D3D_Converter::CheckPreparingToOutput(mfxFrameSurface1 *surface_work,
                                                                       UMC::FrameData* in,
                                                                       const mfxVideoParam * par,
                                                                       VideoVppJpegD3D9 **pCc,
                                                                       mfxU16 taskId)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    mfxStatus sts = (*pCc)->QueryTaskRoutine(taskId);
    if (sts == MFX_TASK_BUSY)
    {
        return sts;
    }
    if (sts != MFX_TASK_DONE)
        return sts;

    if(par->mfx.FrameInfo.PicStruct == MFX_PICSTRUCT_PROGRESSIVE)
    {
        UMC::FrameMemID index = in->GetFrameMID();

        mfxFrameSurface1 pSrc = m_frameData[index].first;
        sts = (*pCc)->EndHwJpegProcessing(&pSrc, surface_work);
        if (sts < MFX_ERR_NONE)
            return sts;

        if (!m_IsUseExternalFrames)
        {
            m_pCore->DecreaseReference(&surface_work->Data);
            m_extSurfaces[index].FrameSurface = 0;
        }
    }
    else
    {
        UMC::FrameMemID indexTop = in[0].GetFrameMID();
        UMC::FrameMemID indexBottom = in[1].GetFrameMID();

        mfxFrameSurface1 pSrcTop, pSrcBottom;

        pSrcTop = m_frameData[indexTop].first;
        pSrcBottom = m_frameData[indexBottom].first;

        sts = (*pCc)->EndHwJpegProcessing(&pSrcTop, &pSrcBottom, surface_work);
        if (sts < MFX_ERR_NONE)
            return sts;

        if (!m_IsUseExternalFrames)
        {
            m_pCore->DecreaseReference(&surface_work->Data);
            m_extSurfaces[indexTop].FrameSurface = 0;
        }
    }

    return MFX_ERR_NONE;
}

void mfx_UMC_FrameAllocator_D3D_Converter::SetJPEGInfo(mfx_UMC_FrameAllocator_D3D_Converter::JPEG_Info * jpegInfo)
{
    m_jpegInfo = *jpegInfo;
}
#endif // #if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE) && defined (MFX_VA_WIN)

