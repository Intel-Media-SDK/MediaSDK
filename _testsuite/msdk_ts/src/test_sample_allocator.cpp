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

#include "test_sample_allocator.h"
#include "sysmem_allocator.h"
#include "d3d_allocator.h"
#include "d3d11_allocator.h"
#include "vaapi_allocator.h"
#include "d3d_device.h"
#include "d3d11_device.h"
#include <algorithm>
//#include "test_common.h"

#define PREPARE_ALLOCATOR( t_allocator, t_device, t_par, e_hdl, c_copy_hdl)\
{                                              \
    p_allocator = new t_allocator;             \
    p_device    = new t_device;                \
    t_par* par  = new t_par;                   \
    hdl_t = e_hdl;                             \
    if (p_device && p_allocator && par)        \
    {                                          \
        p_device->Init(0, 0, 0);               \
        if (!p_device->GetHandle(hdl_t, &hdl)) \
        {                                      \
            par->c_copy_hdl;                   \
            p_allocator_par = par;             \
        }                                      \
    }                                          \
}

frame_allocator::frame_allocator(AllocatorType _allocator_type, AllocMode _alloc_mode, LockMode _lock_mode, OpaqueAllocMode _opaque_mode)
    : alloc_mode     (_alloc_mode)
    , lock_mode      (_lock_mode)
    , opaque_mode    (_opaque_mode)
    , allocator_type (_allocator_type)
    , p_allocator    (NULL)
    , p_device       (NULL)
    , p_allocator_par(NULL)
    , hdl            (NULL)
    , surf_cnt       (0)
    , is_valid       (false)
{
    mfxFrameAllocator::pthis  = this;
    mfxFrameAllocator::Alloc  = &AllocFrame;
    mfxFrameAllocator::Lock   = &LockFrame;
    mfxFrameAllocator::Unlock = &UnLockFrame;
    mfxFrameAllocator::GetHDL = &GetHDL;
    mfxFrameAllocator::Free   = &Free;

    switch (allocator_type)
    {
    case SOFTWARE:
        p_allocator = new SysMemFrameAllocator;
        break;
    case HARDWARE:
#if defined(LIBVA_SUPPORT)
        PREPARE_ALLOCATOR(
            vaapiFrameAllocator,
            CVAAPIDevice,
            vaapiAllocatorParams,
            MFX_HANDLE_VA_DISPLAY,
            m_dpy = (reinterpret_cast<VADisplay>(hdl))
        );

#elif defined(_WIN32) || defined(_WIN64)
        PREPARE_ALLOCATOR(
            D3DFrameAllocator,
            CD3D9Device,
            D3DAllocatorParams,
            MFX_HANDLE_D3D9_DEVICE_MANAGER,
            pManager = reinterpret_cast<IDirect3DDeviceManager9*>(hdl)
        );
        break;
    case HARDWARE_DX11:
        PREPARE_ALLOCATOR(
            D3D11FrameAllocator,
            CD3D11Device,
            D3D11AllocatorParams,
            MFX_HANDLE_D3D11_DEVICE,
            pDevice = reinterpret_cast<ID3D11Device*>(hdl)
        );
#endif //LIBVA_SUPPORT
        break;
    case FAKE:
    default:
        return;
    }
    if (p_allocator)
        if (!p_allocator->Init(p_allocator_par))
            is_valid = true;

    zero_mids.resize(ZERO_MIDS_INITIAL_SIZE, 0);
}

frame_allocator::~frame_allocator()
{
    if (p_allocator)
        delete p_allocator;
    if (p_device)
        delete p_device;
    if (p_allocator_par)
        delete p_allocator_par;
}

frame_allocator::AllocatorType getAllocatorType(std::string type)
{
    if ("sw"        == type) return frame_allocator::SOFTWARE;
    if ("hw"        == type) return frame_allocator::HARDWARE;
    if ("d3d"       == type) return frame_allocator::HARDWARE;
    if ("d3d11"     == type) return frame_allocator::HARDWARE_DX11;
    if ("fake"      == type) return frame_allocator::FAKE;
    if ("software"  == type) return frame_allocator::SOFTWARE;
    if ("hardware"  == type) return frame_allocator::HARDWARE;
    std::cout << "WARNING: unknown allocator type - \"" << type << "\"; assumed HARDWARE" << std::endl;
    return frame_allocator::HARDWARE;
}

frame_allocator::AllocMode getAllocMode(std::string mode)
{
    if ("min-1" == mode) return frame_allocator::ALLOC_MIN_MINUS_1;
    if ("min"   == mode) return frame_allocator::ALLOC_MIN;
    if ("avg"   == mode) return frame_allocator::ALLOC_AVG;
    if ("max"   == mode) return frame_allocator::ALLOC_MAX;
    if ("lots"  == mode) return frame_allocator::ALLOC_LOTS;
    std::cout << "WARNING: unknown alloc mode - \"" << mode << "\"; assumed ALLOC_MAX" << std::endl;
    return frame_allocator::ALLOC_MAX;
}

frame_allocator::LockMode getLockMode(std::string mode)
{
    if ("valid"             == mode) return frame_allocator::ENABLE_ALL;
    if ("nolock"            == mode) return frame_allocator::DISABLE_LOCK;
    if ("nounlock"          == mode) return frame_allocator::DISABLE_UNLOCK;
    if ("crashed_lock"      == mode) return frame_allocator::CRASHED_LOCK;
    if ("crashed_unlock"    == mode) return frame_allocator::CRASHED_UNLOCK;
    if ("zero_luma_lock"    == mode) return frame_allocator::ZERO_LUMA_LOCK;
    if ("large_pitch_lock"  == mode) return frame_allocator::LARGE_PITCH_LOCK;
    if ("locked_surf_lock"  == mode) return frame_allocator::LOCKED_SURF_LOCK;
    std::cout << "WARNING: unknown lock mode - \"" << mode << "\"; assumed ENABLE_ALL" << std::endl;
    return frame_allocator::ENABLE_ALL;
}

frame_allocator::OpaqueAllocMode getOpaqueMode(std::string mode)
{
    if ("error"       == mode) return frame_allocator::ALLOC_ERROR;
    if ("empty"       == mode) return frame_allocator::ALLOC_EMPTY;
    if ("unsupported" == mode) return frame_allocator::ALLOC_UNSUPPORTED;
    if ("internal"    == mode) return frame_allocator::ALLOC_INTERNAL;
    std::cout << "WARNING: unknown opaque alloc mode - \"" << mode << "\"; assumed ALLOC_ERROR" << std::endl;
    return frame_allocator::ALLOC_ERROR;
}

mfxStatus frame_allocator::AllocFrame(mfxHDL pthis, mfxFrameAllocRequest *request, mfxFrameAllocResponse *response)
{
    frame_allocator *instance = static_cast<frame_allocator*>(pthis);

    if (!instance || !request || !response)
        return MFX_ERR_NULL_PTR;

    return instance->alloc_frame_nocheck(request, response);
}


mfxStatus frame_allocator::alloc_frame_nocheck(mfxFrameAllocRequest *request, mfxFrameAllocResponse *response)
{
    mfxFrameAllocRequest int_request = *request;
    bool zero_alloc = false;

    if (allocator_type == FAKE)
        return MFX_ERR_UNSUPPORTED;

    if (!p_allocator)
        return MFX_ERR_NOT_INITIALIZED;

    switch (alloc_mode)
    {
    case ALLOC_MIN_MINUS_1:
        int_request.NumFrameSuggested = request->NumFrameMin - 1 > 0 ? request->NumFrameMin - 1 : request->NumFrameMin;
        break;
    case ALLOC_MIN:
        int_request.NumFrameSuggested = request->NumFrameMin;
        break;
    case ALLOC_AVG:
        int_request.NumFrameSuggested = (request->NumFrameMin + request->NumFrameSuggested) / 2;
        break;
    case ALLOC_MAX:
        //int_request.NumFrameSuggested = request->NumFrameSuggested;
        break;
    case ALLOC_LOTS:
        int_request.NumFrameSuggested = request->NumFrameSuggested * 4;
        break;
    default:
        break;
    }

    if (int_request.Type & MFX_MEMTYPE_OPAQUE_FRAME)
    {
        switch (opaque_mode)
        {
        case ALLOC_ERROR:
            std::cout << "FAILED: MFX_MEMTYPE_OPAQUE_FRAME is invalid request type for external allocator" << std::endl;
            return MFX_ERR_MEMORY_ALLOC;
        case ALLOC_UNSUPPORTED:
            return MFX_ERR_UNSUPPORTED;
        case ALLOC_EMPTY:
            zero_alloc = true;
        case ALLOC_INTERNAL:
        default: break;
        }
    }

    if (zero_alloc)
    {
        response->NumFrameActual = int_request.NumFrameSuggested;
        if (zero_mids.size() < int_request.NumFrameSuggested)
            zero_mids.resize(int_request.NumFrameSuggested, 0);
        response->mids = zero_mids.data();
        return MFX_ERR_NONE;
    }

    mfxStatus sts = p_allocator->AllocFrames(&int_request, response);

    if (!sts)
    {
        if ((int_request.Type & MFX_MEMTYPE_EXTERNAL_FRAME) && (int_request.Type & MFX_MEMTYPE_FROM_DECODE))
        {
            ExternalFrame f(*response, int_request.Info.CropW, int_request.Info.CropH, int_request.Type);
            std::list<ExternalFrame>::iterator it = std::find_if(external_frames.begin(), external_frames.end(), f);

            if (it != external_frames.end())
                return sts; //no new frames allocated

            external_frames.push_back(f);
        }
        surf_cnt += response->NumFrameActual;
        std::cout << response->NumFrameActual << " surfaces allocated" << std::endl;
    }

    return sts;
}


mfxStatus frame_allocator::LockFrame(mfxHDL pthis, mfxMemId mid, mfxFrameData *ptr)
{
    frame_allocator *instance = static_cast<frame_allocator*>(pthis);
    mfxStatus mfxRes = MFX_ERR_NONE;

    if (!instance || !ptr)
        return MFX_ERR_NULL_PTR;

    if (0 != (instance->lock_mode & DISABLE_LOCK))
        return MFX_ERR_LOCK_MEMORY;
    if (0 != (instance->lock_mode & CRASHED_LOCK))
    {
        mfxU8 *ptr = NULL;
        *ptr = 0xFF;    /* crash */
    }

    mfxRes = instance->lock_frame_nocheck(mid, ptr);
    if (0 != (instance->lock_mode & ZERO_LUMA_LOCK))
        ptr->Y = NULL;
    else if (0 != (instance->lock_mode & LARGE_PITCH_LOCK))
    {
        ptr->Pitch = 0x8000;
        ptr->PitchHigh = 0xFFFF;
    }
    else if (0 != (instance->lock_mode & MIN_PITCH_LOCK))
    {
        ptr->Pitch = 1;
        ptr->PitchHigh = 0;
    }
    else if (0 != (instance->lock_mode & LOCKED_SURF_LOCK))
        ptr->Locked = 1;

    return mfxRes;
}

mfxStatus frame_allocator::lock_frame_nocheck(mfxMemId mid, mfxFrameData *ptr)
{
    if (allocator_type == FAKE)
        return MFX_ERR_UNSUPPORTED;

    if (!p_allocator)
        return MFX_ERR_NOT_INITIALIZED;

    return p_allocator->LockFrame(mid, ptr);
}


mfxStatus frame_allocator::UnLockFrame(mfxHDL pthis, mfxMemId mid, mfxFrameData *ptr)
{
    frame_allocator *instance = static_cast<frame_allocator*>(pthis);

    if (0 != (instance->lock_mode & DISABLE_UNLOCK))
        return MFX_ERR_INVALID_HANDLE;
    if (0 != (instance->lock_mode & CRASHED_UNLOCK))
    {
        mfxU8 *ptr = NULL;
        *ptr = 0xFF;    /* crash */
    }

    return instance->unlock_frame_nocheck(mid, ptr);
}

mfxStatus frame_allocator::unlock_frame_nocheck(mfxMemId mid, mfxFrameData *ptr)
{
    if (allocator_type == FAKE)
        return MFX_ERR_UNSUPPORTED;

    if (!p_allocator)
        return MFX_ERR_NOT_INITIALIZED;

    return p_allocator->UnlockFrame(mid, ptr);
}


mfxStatus frame_allocator::GetHDL(mfxHDL pthis, mfxMemId mid, mfxHDL *hdl)
{
    frame_allocator *instance = static_cast<frame_allocator*>(pthis);

    if (FAKE == instance->allocator_type)
        return MFX_ERR_UNSUPPORTED;

    if (!instance->p_allocator)
        return MFX_ERR_NOT_INITIALIZED;

    return instance->p_allocator->GetFrameHDL(mid, hdl);
}


mfxStatus frame_allocator::Free(mfxHDL pthis, mfxFrameAllocResponse *response)
{
    frame_allocator *instance = static_cast<frame_allocator*>(pthis);

    if (FAKE == instance->allocator_type)
        return MFX_ERR_UNSUPPORTED;

    if (!instance->p_allocator)
        return MFX_ERR_NOT_INITIALIZED;

    return instance->p_allocator->FreeFrames(response);
}

buffer_allocator::buffer_allocator()
{
    pthis  = this;
    Alloc  = _Alloc;
    Lock   = _Lock;
    Free   = _Free;
    Unlock = _Unlock;
    p_allocator = new SysMemBufferAllocator;
}

buffer_allocator::~buffer_allocator()
{
    if (p_allocator)
        delete p_allocator;
}

mfxStatus buffer_allocator::_Alloc  (mfxHDL pthis, mfxU32 nbytes, mfxU16 type, mfxMemId *mid)
{
    buffer_allocator *instance = static_cast<buffer_allocator*>(pthis);

    if (!instance->p_allocator)
        return MFX_ERR_NOT_INITIALIZED;

    mfxStatus sts = instance->p_allocator->AllocBuffer(nbytes, type, mid);

    if (!sts)
    {
        instance->buf.push_back(buffer(type, nbytes, *mid));
        std::cout << nbytes << " bytes allocated" << std::endl;
    }

    return sts;
}

mfxStatus buffer_allocator::_Lock   (mfxHDL pthis, mfxMemId mid, mfxU8 **ptr)
{
    buffer_allocator *instance = static_cast<buffer_allocator*>(pthis);

    if (!instance->p_allocator)
        return MFX_ERR_NOT_INITIALIZED;

    return instance->p_allocator->LockBuffer(mid, ptr);
}

mfxStatus buffer_allocator::_Unlock (mfxHDL pthis, mfxMemId mid)
{
    buffer_allocator *instance = static_cast<buffer_allocator*>(pthis);

    if (!instance->p_allocator)
        return MFX_ERR_NOT_INITIALIZED;

    return instance->p_allocator->UnlockBuffer(mid);
}

mfxStatus buffer_allocator::_Free   (mfxHDL pthis, mfxMemId mid)
{
    buffer_allocator *instance = static_cast<buffer_allocator*>(pthis);

    if (!instance->p_allocator)
        return MFX_ERR_NOT_INITIALIZED;

    return instance->p_allocator->FreeBuffer(mid);
}
