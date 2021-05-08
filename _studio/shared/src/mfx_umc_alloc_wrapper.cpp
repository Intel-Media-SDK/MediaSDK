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

#include "umc_defs.h"

#if defined(MFX_VA) && !defined MFX_DEC_VIDEO_POSTPROCESS_DISABLE
// For setting SFC surface
#include "umc_va_video_processing.h"
#endif

#include "mfx_umc_alloc_wrapper.h"
#include "mfx_common.h"
#include "libmfx_core.h"
#include "mfx_common_int.h"
#include "mfxfei.h"


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
mfx_UMC_FrameAllocator::InternalFrameData::FrameRefInfo::FrameRefInfo()
    : m_referenceCounter(0)
{
}

void mfx_UMC_FrameAllocator::InternalFrameData::FrameRefInfo::Reset()
{
    m_referenceCounter = 0;
}

bool mfx_UMC_FrameAllocator::InternalFrameData::IsValidMID(mfxU32 index) const
{
    if (index >= m_frameData.size())
        return false;

    return true;
}

mfxFrameSurface1 & mfx_UMC_FrameAllocator::InternalFrameData::GetSurface(mfxU32 index)
{
    if (!IsValidMID(index))
        throw std::exception();

    return m_frameData[index].first;
}

UMC::FrameData   & mfx_UMC_FrameAllocator::InternalFrameData::GetFrameData(mfxU32 index)
{
    if (!IsValidMID(index))
        throw std::exception();

    return m_frameData[index].second;
}

void mfx_UMC_FrameAllocator::InternalFrameData::Close()
{
    m_frameData.clear();
    m_frameDataRefs.clear();
}

void mfx_UMC_FrameAllocator::InternalFrameData::ResetFrameData(mfxU32 index)
{
    if (!IsValidMID(index))
        throw std::exception();

    m_frameDataRefs[index].Reset();
    m_frameData[index].second.Reset();
}

void mfx_UMC_FrameAllocator::InternalFrameData::Resize(mfxU32 size)
{
    m_frameData.resize(size);
    m_frameDataRefs.resize(size);
}

mfxU32 mfx_UMC_FrameAllocator::InternalFrameData::IncreaseRef(mfxU32 index)
{
    if (!IsValidMID(index))
        throw std::exception();

    FrameRefInfo * frameRef = &m_frameDataRefs[index];
    frameRef->m_referenceCounter++;
    return frameRef->m_referenceCounter;
}

mfxU32 mfx_UMC_FrameAllocator::InternalFrameData::DecreaseRef(mfxU32 index)
{
    if (!IsValidMID(index))
        throw std::exception();

    FrameRefInfo * frameRef = &m_frameDataRefs[index];
    frameRef->m_referenceCounter--;
    return frameRef->m_referenceCounter;
}

void mfx_UMC_FrameAllocator::InternalFrameData::Reset()
{
    // unlock internal sufraces
    for (mfxU32 i = 0; i < m_frameData.size(); i++)
    {
        m_frameData[i].first.Data.Locked = 0;  // if app ext allocator then should decrease Locked counter same times as locked by medisSDK
        m_frameData[i].second.Reset();
    }

    for (mfxU32 i = 0; i < m_frameDataRefs.size(); i++)
    {
        m_frameDataRefs[i].Reset();
    }
}

mfxU32 mfx_UMC_FrameAllocator::InternalFrameData::GetSize() const
{
    return (mfxU32)m_frameData.size();
}

void mfx_UMC_FrameAllocator::InternalFrameData::AddNewFrame(mfx_UMC_FrameAllocator * alloc, mfxFrameSurface1 *surface, UMC::VideoDataInfo * info)
{
    FrameRefInfo refInfo;
    m_frameDataRefs.push_back(refInfo);

    FrameInfo  frameInfo;
    m_frameData.push_back(frameInfo);

    mfxU32 index = (mfxU32)(m_frameData.size() - 1);;

    memset(&(m_frameData[index].first), 0, sizeof(m_frameData[index].first));
    m_frameData[index].first.Data.MemId = surface->Data.MemId;
    m_frameData[index].first.Info = surface->Info;

    // fill UMC frameData
    UMC::FrameData* frameData = &GetFrameData(index);

    // set correct width & height to planes
    frameData->Init(info, (UMC::FrameMemID)index, alloc);
}


mfx_UMC_FrameAllocator::mfx_UMC_FrameAllocator()
    : m_curIndex(-1)
    , m_IsUseExternalFrames(true)
    , m_sfcVideoPostProcessing(false)
    , m_surface_info()
    , m_pCore(0)
    , m_externalFramesResponse(0)
    , m_isSWDecode(false)
    , m_IOPattern(0)
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
#if (MFX_VERSION >= 1027)
        || params->mfx.FrameInfo.FourCC == MFX_FOURCC_Y210
        || params->mfx.FrameInfo.FourCC == MFX_FOURCC_Y410
#endif
        )
        bit_depth = 10;
#if (MFX_VERSION >= 1031)
    else if (params->mfx.FrameInfo.FourCC == MFX_FOURCC_P016 ||
             params->mfx.FrameInfo.FourCC == MFX_FOURCC_Y216 ||
             params->mfx.FrameInfo.FourCC == MFX_FOURCC_Y416)
        bit_depth = 12;
#endif
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
#if (MFX_VERSION >= 1027)
    case MFX_FOURCC_Y210:
        color_format = UMC::Y210;
        break;
    case MFX_FOURCC_Y410:
        color_format = UMC::Y410;
        break;
#endif
#if (MFX_VERSION >= 1031)
    case MFX_FOURCC_P016:
        color_format = UMC::P016;
        break;
    case MFX_FOURCC_Y216:
        color_format = UMC::Y216;
        break;
    case MFX_FOURCC_Y416:
        color_format = UMC::Y416;
        break;
#endif
    default:
        return UMC::UMC_ERR_UNSUPPORTED;
    }

    UMC::Status umcSts = m_info.Init(params->mfx.FrameInfo.Width, params->mfx.FrameInfo.Height, color_format, bit_depth);

    m_surface_info = params->mfx.FrameInfo;

    if (umcSts != UMC::UMC_OK)
        return umcSts;

    if (!m_IsUseExternalFrames || !m_isSWDecode)
    {
        m_frameDataInternal.Resize(response->NumFrameActual);
        m_extSurfaces.resize(response->NumFrameActual);

        for (mfxU32 i = 0; i < response->NumFrameActual; i++)
        {
            mfxFrameSurface1 & surface = m_frameDataInternal.GetSurface(i);
            surface.Data.MemId   = response->mids[i];
            surface.Data.MemType = request->Type;
            surface.Info         = request->Info;

            // fill UMC frameData
            UMC::FrameData& frameData = m_frameDataInternal.GetFrameData(i);

            // set correct width & height to planes
            frameData.Init(&m_info, (UMC::FrameMemID)i, this);
        }
    }
    else
    {
        m_extSurfaces.reserve(response->NumFrameActual);
    }

    mfxCore->SetWrapper(this);

    return UMC::UMC_OK;
}


UMC::Status mfx_UMC_FrameAllocator::Close()
{
    UMC::AutomaticUMCMutex guard(m_guard);

    Reset();
    m_frameDataInternal.Close();
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

    m_frameDataInternal.Reset();

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

static mfxStatus SetSurfaceForSFC(VideoCORE& core, mfxFrameSurface1& surf)
{
#if defined(MFX_VA) && !defined MFX_DEC_VIDEO_POSTPROCESS_DISABLE
    // Set surface for SFC
    UMC::VideoAccelerator * va = nullptr;

    core.GetVA((mfxHDL*)&va, MFX_MEMTYPE_FROM_DECODE);
    MFX_CHECK_HDL(va);

    auto video_processing_va = va->GetVideoProcessingVA();

    if (video_processing_va && core.GetVAType() == MFX_HW_VAAPI)
    {
        mfxHDL surfHDL = {};
        mfxStatus sts = core.GetExternalFrameHDL(surf.Data. MemId, &surfHDL, false);

        MFX_CHECK_STS(sts);
        video_processing_va->SetOutputSurface(surfHDL);
    }
#else
    std::ignore = core;
    std::ignore = surf;
#endif

    return MFX_ERR_NONE;
}

UMC::Status mfx_UMC_FrameAllocator::Alloc(UMC::FrameMemID *pNewMemID, const UMC::VideoDataInfo * info, uint32_t a_flags)
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

    mfxFrameInfo &surfInfo = m_frameDataInternal.GetSurface(index).Info;

    mfxSize allocated = { surfInfo.Width, surfInfo.Height};
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
    case UMC::P016:
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
        if (!(a_flags & mfx_UMC_ReallocAllowed))
            return UMC::UMC_ERR_UNSUPPORTED;
    }

    sts = m_pCore->IncreasePureReference(m_frameDataInternal.GetSurface(index).Data.Locked);
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

            if (m_sfcVideoPostProcessing)
            {
                SetSurfaceForSFC(*m_pCore, *m_extSurfaces[index].FrameSurface);
            }
        }
    }

    m_frameDataInternal.ResetFrameData(index);
    m_curIndex = -1;

    if (passed.width > allocated.width ||
        passed.height > allocated.height)
    {
        if (a_flags & mfx_UMC_ReallocAllowed)
            return UMC::UMC_ERR_NOT_ENOUGH_BUFFER;
    }
    return UMC::UMC_OK;
}

const UMC::FrameData* mfx_UMC_FrameAllocator::Lock(UMC::FrameMemID mid)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    mfxU32 index = (mfxU32)mid;
    if (!m_frameDataInternal.IsValidMID(index))
        return 0;

    mfxFrameData *data = 0;

    mfxFrameSurface1 check_surface;
    mfxFrameSurface1 &internal_surface = m_frameDataInternal.GetSurface(index);
    check_surface.Info.FourCC = internal_surface.Info.FourCC;

    if (m_IsUseExternalFrames)
    {
        if (internal_surface.Data.MemId != 0)
        {
            data = &internal_surface.Data;
            mfxStatus sts = m_pCore->LockExternalFrame(internal_surface.Data.MemId, data);

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
        if (internal_surface.Data.MemId != 0)
        {
            data = &internal_surface.Data;
            mfxStatus sts = m_pCore->LockFrame(internal_surface.Data.MemId, data);

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

    UMC::FrameData* frameData = &m_frameDataInternal.GetFrameData(index);
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
        if (internal_surface.Data.MemId)
        {
            if (m_IsUseExternalFrames)
            {
                m_pCore->UnlockExternalFrame(m_extSurfaces[index].FrameSurface->Data.MemId);
            }
            else
            {
                m_pCore->UnlockFrame(internal_surface.Data.MemId);
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
    if (!m_frameDataInternal.IsValidMID(index))
        return UMC::UMC_ERR_FAILED;

    mfxFrameSurface1 &internal_surface = m_frameDataInternal.GetSurface(index);
    if (internal_surface.Data.MemId)
    {
        mfxStatus sts;
        if (m_IsUseExternalFrames)
            sts = m_pCore->UnlockExternalFrame(m_extSurfaces[index].FrameSurface->Data.MemId);
        else
            sts = m_pCore->UnlockFrame(internal_surface.Data.MemId);

        if (sts < MFX_ERR_NONE)
            return UMC::UMC_ERR_FAILED;
    }

    return UMC::UMC_OK;
}

UMC::Status mfx_UMC_FrameAllocator::IncreaseReference(UMC::FrameMemID mid)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    mfxU32 index = (mfxU32)mid;
    if (!m_frameDataInternal.IsValidMID(index))
        return UMC::UMC_ERR_FAILED;

    m_frameDataInternal.IncreaseRef(index);

    return UMC::UMC_OK;
}

UMC::Status mfx_UMC_FrameAllocator::DecreaseReference(UMC::FrameMemID mid)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    mfxU32 index = (mfxU32)mid;
    if (!m_frameDataInternal.IsValidMID(index))
        return UMC::UMC_ERR_FAILED;

    mfxU32 refCounter = m_frameDataInternal.DecreaseRef(index);
    if (!refCounter)
    {
        return Free(mid);
    }

    return UMC::UMC_OK;
}

UMC::Status mfx_UMC_FrameAllocator::Free(UMC::FrameMemID mid)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    mfxStatus sts = MFX_ERR_NONE;
    mfxU32 index = (mfxU32)mid;
    if (!m_frameDataInternal.IsValidMID(index))
        return UMC::UMC_ERR_FAILED;

    sts = m_pCore->DecreasePureReference(m_frameDataInternal.GetSurface(index).Data.Locked);
    if (sts < MFX_ERR_NONE)
        return UMC::UMC_ERR_FAILED;

    if ((m_IsUseExternalFrames) || (m_sfcVideoPostProcessing))
    {
        if (m_extSurfaces[index].FrameSurface)
        {
            sts = m_pCore->DecreaseReference(&m_extSurfaces[index].FrameSurface->Data);
            if (sts < MFX_ERR_NONE)
                return UMC::UMC_ERR_FAILED;
        }
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
    if (!(m_sfcVideoPostProcessing && (surf->Info.FourCC != m_surface_info.FourCC)))// if csc is done via sfc, will not do below checks
    {
        if ((surf->Info.BitDepthLuma ? surf->Info.BitDepthLuma : 8) != (m_surface_info.BitDepthLuma ? m_surface_info.BitDepthLuma : 8))
            return MFX_ERR_INVALID_VIDEO_PARAM;

        if ((surf->Info.BitDepthChroma ? surf->Info.BitDepthChroma : 8) != (m_surface_info.BitDepthChroma ? m_surface_info.BitDepthChroma : 8))
            return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    if (   surf->Info.FourCC == MFX_FOURCC_P010
        || surf->Info.FourCC == MFX_FOURCC_P210
#if (MFX_VERSION >= 1027)
        || surf->Info.FourCC == MFX_FOURCC_Y210
#endif
#if (MFX_VERSION >= 1031)
        || surf->Info.FourCC == MFX_FOURCC_P016
        || surf->Info.FourCC == MFX_FOURCC_Y216
        || surf->Info.FourCC == MFX_FOURCC_Y416
#endif
        )
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
                  (0 == m_frameDataInternal.GetSurface(i).Data.Locked) )
            {
                /* surfaces filled already */
                m_curIndex = i;
                m_extSurfaces[m_curIndex].FrameSurface = surf;
                break;
            }
        }

        // Still not found. It may happen if decoder gets 'surf' surface which on app size belongs to
        // a pool bigger than m_extSurfaces/m_frameDataInternal pools which decoder is aware.
        if (m_curIndex == -1)
        {
            for (mfxU32 i = 0; i < m_extSurfaces.size(); i++)
            {
                // So attemping to find an expired slot in m_extSurfaces
                if (!m_extSurfaces[i].isUsed && (0 == m_frameDataInternal.GetSurface(i).Data.Locked))
                {
                    m_curIndex = i;
                    m_extSurfaces[m_curIndex].FrameSurface = surf;
                    break;
                }
            }
        }
    }
    else
    {
        m_curIndex = FindSurface(surf, isOpaq);

        if (m_curIndex != -1)
        {
            mfxFrameSurface1 &internalSurf = m_frameDataInternal.GetSurface(m_curIndex);
            m_extSurfaces[m_curIndex].FrameSurface = surf;
            if (internalSurf.Data.Locked) // surface was locked yet
            {
                m_curIndex = -1;
            }

            // update info
            internalSurf.Info = surf->Info;
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
            if (surface->Data.MemId == m_pCore->MapIdx(m_frameDataInternal.GetSurface(i).Data.MemId))
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
#if (MFX_VERSION >= 1027)
    case MFX_FOURCC_Y210:
    case MFX_FOURCC_Y410:
#endif
#if (MFX_VERSION >= 1031)
    case MFX_FOURCC_Y216:
#endif

        break;
    default:
        return -1;
    }

    if (m_IsUseExternalFrames && m_isSWDecode)
    {
        m_frameDataInternal.AddNewFrame(this, surface, &m_info);
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
        mfxMemId sMemId;
        for (mfxU32 i = 0; i < m_frameDataInternal.GetSize(); i++)
        {
            mfxMemId memId = m_frameDataInternal.GetSurface(i).Data.MemId;
            sMemId = isOpaq == true ? memId : m_pCore->MapIdx(memId);
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

    for (mfxU32 i = 0; i < m_frameDataInternal.GetSize(); i++)
    {
        if (!m_frameDataInternal.GetSurface(i).Data.Locked)
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
        if (!m_frameDataInternal.IsValidMID((mfxU32)index))
            return 0;
        return &m_frameDataInternal.GetSurface(index);
    }

    return 0;
}

mfxFrameSurface1 * mfx_UMC_FrameAllocator::GetSurfaceByIndex(UMC::FrameMemID index)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    if (index < 0)
        return 0;

    if (!m_frameDataInternal.IsValidMID((mfxU32)index))
        return 0;

    return m_IsUseExternalFrames ? m_extSurfaces[index].FrameSurface : &m_frameDataInternal.GetSurface(index);
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
        if ((uint32_t)index >= m_extSurfaces.size())
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
    
    UMC::FrameData* frame = &m_frameDataInternal.GetFrameData(index);

    if (m_IsUseExternalFrames)
        return MFX_ERR_NONE;

    mfxFrameSurface1 surface;

    memset(&surface, 0, sizeof(mfxFrameSurface1));

    surface.Info = m_surface_info;
    surface.Info.Width  = (mfxU16)frame->GetInfo()->GetWidth();
    surface.Info.Height = (mfxU16)frame->GetInfo()->GetHeight();

    switch (frame->GetInfo()->GetColorFormat())
    {
    case UMC::NV12:
        surface.Data.Y = frame->GetPlaneMemoryInfo(0)->m_planePtr;
        surface.Data.UV = frame->GetPlaneMemoryInfo(1)->m_planePtr;
        surface.Data.PitchHigh = (mfxU16)(frame->GetPlaneMemoryInfo(0)->m_pitch / (1 << 16));
        surface.Data.PitchLow  = (mfxU16)(frame->GetPlaneMemoryInfo(0)->m_pitch % (1 << 16));
        break;

    case UMC::YUV420:
        surface.Data.Y = frame->GetPlaneMemoryInfo(0)->m_planePtr;
        surface.Data.U = frame->GetPlaneMemoryInfo(1)->m_planePtr;
        surface.Data.V = frame->GetPlaneMemoryInfo(2)->m_planePtr;
        surface.Data.PitchHigh = (mfxU16)(frame->GetPlaneMemoryInfo(0)->m_pitch / (1 << 16));
        surface.Data.PitchLow  = (mfxU16)(frame->GetPlaneMemoryInfo(0)->m_pitch % (1 << 16));
        break;

    case UMC::IMC3:
        surface.Data.Y = frame->GetPlaneMemoryInfo(0)->m_planePtr;
        surface.Data.U = frame->GetPlaneMemoryInfo(1)->m_planePtr;
        surface.Data.V = frame->GetPlaneMemoryInfo(2)->m_planePtr;
        surface.Data.PitchHigh = (mfxU16)(frame->GetPlaneMemoryInfo(0)->m_pitch / (1 << 16));
        surface.Data.PitchLow  = (mfxU16)(frame->GetPlaneMemoryInfo(0)->m_pitch % (1 << 16));
        break;

    case UMC::RGB32:
        surface.Data.B = frame->GetPlaneMemoryInfo(0)->m_planePtr;
        surface.Data.G = surface.Data.B + 1;
        surface.Data.R = surface.Data.B + 2;
        surface.Data.A = surface.Data.B + 3;
        surface.Data.PitchHigh = (mfxU16)(frame->GetPlaneMemoryInfo(0)->m_pitch / (1 << 16));
        surface.Data.PitchLow  = (mfxU16)(frame->GetPlaneMemoryInfo(0)->m_pitch % (1 << 16));
        break;

    case UMC::YUY2:
        surface.Data.Y = frame->GetPlaneMemoryInfo(0)->m_planePtr;
        surface.Data.U = surface.Data.Y + 1;
        surface.Data.V = surface.Data.Y + 3;
        surface.Data.PitchHigh = (mfxU16)(frame->GetPlaneMemoryInfo(0)->m_pitch / (1 << 16));
        surface.Data.PitchLow  = (mfxU16)(frame->GetPlaneMemoryInfo(0)->m_pitch % (1 << 16));
        break;

    default:
        return MFX_ERR_UNSUPPORTED;
    }
    surface.Info.FourCC = surface_work->Info.FourCC;
    surface.Info.Shift = m_IsUseExternalFrames ? m_extSurfaces[index].FrameSurface->Info.Shift : m_frameDataInternal.GetSurface(index).Info.Shift;

    //Performance issue. We need to unlock mutex to let decoding thread run async.
    guard.Unlock();
    sts = m_pCore->DoFastCopyWrapper(surface_work,
                                     dstMemType,
                                     &surface,
                                     MFX_MEMTYPE_INTERNAL_FRAME | MFX_MEMTYPE_SYSTEM_MEMORY);
    guard.Lock();

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
    mfxMemId memInternal = m_frameDataInternal.GetSurface(index).Data.MemId;
    mfxMemId memId = isOpaq?(memInternal):(m_pCore->MapIdx(memInternal));

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
            mfxFrameSurface1 surface;

            mfxFrameSurface1 & internalSurf = m_frameDataInternal.GetSurface(index);
            mfxMemId idx = internalSurf.Data.MemId;
            memset(&surface.Data,0,sizeof(mfxFrameData));
            surface.Info = internalSurf.Info;
            surface.Data.Y = 0;
            surface.Data.MemId = idx;
            //Performance issue. We need to unlock mutex to let decoding thread run async.
            guard.Unlock();
            sts = m_pCore->DoFastCopyWrapper(surface_work,
                                             MFX_MEMTYPE_EXTERNAL_FRAME | MFX_MEMTYPE_SYSTEM_MEMORY,
                                             &surface,

                                             MFX_MEMTYPE_INTERNAL_FRAME | MFX_MEMTYPE_DXVA2_DECODER_TARGET
                                             );
            guard.Lock();
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


