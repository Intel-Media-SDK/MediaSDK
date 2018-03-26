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

#pragma once

#include "base_allocator.h"
#include "hw_device.h"
#include <iostream>
#include <string>
#include <list>
#include <vector>

#if defined(LIBVA_SUPPORT)
 #include "vaapi_utils_drm.h"
 #include "vaapi_utils_x11.h"
#endif
#ifdef __gnu_linux__
  #include <stdint.h> // for uintptr_t on Linux
#endif


class frame_allocator : public mfxFrameAllocator{
public:
    enum AllocatorType
    {
        SOFTWARE,
        HARDWARE,
        HARDWARE_DX11,
        FAKE,
    };

    enum AllocMode
    {
        ALLOC_MIN_MINUS_1,
        ALLOC_MIN,
        ALLOC_AVG,
        ALLOC_MAX,
        ALLOC_LOTS
    };

    enum LockMode
    {
        ENABLE_ALL          = 0,
        DISABLE_LOCK        = 1,
        DISABLE_UNLOCK      = 2,
        CRASHED_LOCK        = 4,
        CRASHED_UNLOCK      = 8,
        ZERO_LUMA_LOCK      = 16,
        LARGE_PITCH_LOCK    = 32,
        LOCKED_SURF_LOCK    = 64,
        MIN_PITCH_LOCK      = 128,
    };

    enum OpaqueAllocMode
    {
        ALLOC_ERROR,
        ALLOC_EMPTY,
        ALLOC_UNSUPPORTED,
        ALLOC_INTERNAL
    };

    struct ExternalFrame : mfxFrameAllocResponse
    {
        mfxU16 w;
        mfxU16 h;
        mfxU16 type;

        ExternalFrame(const mfxFrameAllocResponse& response, mfxU16 _w, mfxU16 _h, mfxU16 _type)
            : mfxFrameAllocResponse(response)
            , w(_w)
            , h(_h)
            , type(_type)
        {}

        bool operator () (const ExternalFrame &response)const { return mids == response.mids; }
    };

    frame_allocator(AllocatorType _allocator_type, AllocMode _alloc_mode, LockMode _lock_mode, OpaqueAllocMode _opaque_mode);
    ~frame_allocator();
    
    static mfxStatus AllocFrame (mfxHDL pthis, mfxFrameAllocRequest *request, mfxFrameAllocResponse *response);
    static mfxStatus LockFrame  (mfxHDL pthis, mfxMemId mid, mfxFrameData *ptr);
    static mfxStatus UnLockFrame(mfxHDL pthis, mfxMemId mid, mfxFrameData *ptr);
    static mfxStatus GetHDL     (mfxHDL pthis, mfxMemId mid, mfxHDL *hdl);
    static mfxStatus Free       (mfxHDL pthis, mfxFrameAllocResponse *response);

    
    mfxStatus alloc_frame_nocheck   (mfxFrameAllocRequest *request, mfxFrameAllocResponse *response);
    mfxStatus lock_frame_nocheck    (mfxMemId mid, mfxFrameData *ptr);
    mfxStatus unlock_frame_nocheck  (mfxMemId mid, mfxFrameData *ptr);

    bool get_hdl(mfxHandleType& type, mfxHDL& hdl)
    {
        if(!this->hdl) {
            hdl = 0;
            type = static_cast<mfxHandleType>(1);
            return false;
        }
        type = hdl_t;
        hdl = this->hdl;
        return true;
    };
    unsigned int cnt_surf() { return surf_cnt; };
    const std::list<ExternalFrame>& GetExternalFrames() { return external_frames; };

    void reset(AllocMode _alloc_mode, LockMode _lock_mode, OpaqueAllocMode _opaque_mode)
    {
        alloc_mode  = _alloc_mode;
        lock_mode   = _lock_mode;
        opaque_mode = _opaque_mode;
    };

    bool valid() { return is_valid; };

private:
    AllocMode           alloc_mode;
    LockMode            lock_mode;
    OpaqueAllocMode     opaque_mode;
    AllocatorType       allocator_type;
    MFXFrameAllocator*  p_allocator;
    CHWDevice*          p_device;
    mfxAllocatorParams* p_allocator_par;
    mfxHDL              hdl;
    mfxHandleType       hdl_t;
    unsigned int        surf_cnt;
    bool                is_valid;

    std::list   <ExternalFrame> external_frames;
    std::vector <mfxMemId>      zero_mids;      //empty buffer for zero-mids alloc (opaque)

    static const unsigned int ZERO_MIDS_INITIAL_SIZE = 80;
};

frame_allocator::AllocatorType   getAllocatorType(std::string type);
frame_allocator::AllocMode       getAllocMode    (std::string mode);
frame_allocator::LockMode        getLockMode     (std::string mode);
frame_allocator::OpaqueAllocMode getOpaqueMode   (std::string mode);


class CVAAPIDevice : public CHWDevice
{
public:
    CVAAPIDevice(){}
    virtual ~CVAAPIDevice(){}

    virtual mfxStatus Init(
        mfxHDL hWindow,
        mfxU16 nViews,
        mfxU32 nAdapterNum) { return MFX_ERR_UNSUPPORTED; }
    virtual mfxStatus Reset() { return MFX_ERR_UNSUPPORTED; }
    virtual mfxStatus GetHandle(mfxHandleType type, mfxHDL *pHdl)
    {
#if defined(LIBVA_SUPPORT)
        if(pHdl)
        {
            *pHdl = m_libva.GetVADisplay();
            return MFX_ERR_NONE;
        }
#endif
        return MFX_ERR_UNSUPPORTED;
    }

    virtual mfxStatus SetHandle(mfxHandleType type, mfxHDL hdl) { return MFX_ERR_UNSUPPORTED; }
    virtual mfxStatus RenderFrame(mfxFrameSurface1 * pSurface, mfxFrameAllocator * pmfxAlloc) { return MFX_ERR_UNSUPPORTED; }
    virtual void      UpdateTitle(double fps) {}
    virtual void SetMondelloInput(bool /*isMondelloInputEnabled*/) { }
    virtual void Close(){}

private:
#if defined(LIBVA_DRM_SUPPORT)
    DRMLibVA m_libva;
#elif defined(LIBVA_X11_SUPPORT)
    X11LibVA m_libva;
#endif
};


class buffer_allocator : public mfxBufferAllocator
{
public:
    buffer_allocator();
    ~buffer_allocator();

    struct buffer{
        mfxU16      type; 
        mfxU32      size;
        mfxMemId    mid;
        
        buffer(mfxU16 _type, mfxU32 _size, mfxMemId _mid)
            : type(_type)
            , size(_size)
            , mid (_mid )
        {}
    };

    std::vector<buffer>& GetBuffers() { return buf; };

private:
    static mfxStatus MFX_CDECL _Alloc  (mfxHDL pthis, mfxU32 nbytes, mfxU16 type, mfxMemId *mid);
    static mfxStatus MFX_CDECL _Lock   (mfxHDL pthis, mfxMemId mid, mfxU8 **ptr);
    static mfxStatus MFX_CDECL _Unlock (mfxHDL pthis, mfxMemId mid);    
    static mfxStatus MFX_CDECL _Free   (mfxHDL pthis, mfxMemId mid);

    MFXBufferAllocator* p_allocator;
    std::vector<buffer> buf;
};
