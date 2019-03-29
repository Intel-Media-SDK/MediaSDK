/******************************************************************************\
Copyright (c) 2005-2019, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This sample was distributed or derived from the Intel's Media Samples package.
The original version of this sample may be obtained from https://software.intel.com/en-us/intel-media-server-studio
or https://software.intel.com/en-us/media-client-solutions-support.
\**********************************************************************************/

#include "mfx_samples_config.h"
#include <algorithm>

#if defined(WIN32) || defined(WIN64)

//prefast singnature used in combaseapi.h
#ifndef _PREFAST_
    #pragma warning(disable:4068)
#endif

#include "d3d_device.h"
#include "d3d_allocator.h"
#include "sample_defs.h"

#include "atlbase.h"

CD3D9Device::CD3D9Device()
{
    m_pD3D9 = NULL;
    m_pD3DD9 = NULL;
    m_pDeviceManager9 = NULL;
    MSDK_ZERO_MEMORY(m_D3DPP);
    m_resetToken = 0;

    m_nViews = 0;

    MSDK_ZERO_MEMORY(m_backBufferDesc);
    m_pDXVAVPS = NULL;
    m_pDXVAVP_Left = NULL;
    m_pDXVAVP_Right = NULL;

    MSDK_ZERO_MEMORY(m_targetRect);

    MSDK_ZERO_MEMORY(m_VideoDesc);
    MSDK_ZERO_MEMORY(m_BltParams);
    MSDK_ZERO_MEMORY(m_Sample);

    // Initialize DXVA structures

    DXVA2_AYUVSample16 color = {
        0x8000,          // Cr
        0x8000,          // Cb
        0x1000,          // Y
        0xffff           // Alpha
    };

    DXVA2_ExtendedFormat format =   {           // DestFormat
        DXVA2_SampleProgressiveFrame,           // SampleFormat
        DXVA2_VideoChromaSubsampling_MPEG2,     // VideoChromaSubsampling
        DXVA_NominalRange_0_255,                // NominalRange
        DXVA2_VideoTransferMatrix_BT709,        // VideoTransferMatrix
        DXVA2_VideoLighting_bright,             // VideoLighting
        DXVA2_VideoPrimaries_BT709,             // VideoPrimaries
        DXVA2_VideoTransFunc_709                // VideoTransferFunction
    };

    // init m_VideoDesc structure
    MSDK_MEMCPY_VAR(m_VideoDesc.SampleFormat, &format, sizeof(DXVA2_ExtendedFormat));
    m_VideoDesc.SampleWidth                         = 0;
    m_VideoDesc.SampleHeight                        = 0;
    m_VideoDesc.InputSampleFreq.Numerator           = 60;
    m_VideoDesc.InputSampleFreq.Denominator         = 1;
    m_VideoDesc.OutputFrameFreq.Numerator           = 60;
    m_VideoDesc.OutputFrameFreq.Denominator         = 1;

    // init m_BltParams structure
    MSDK_MEMCPY_VAR(m_BltParams.DestFormat, &format, sizeof(DXVA2_ExtendedFormat));
    MSDK_MEMCPY_VAR(m_BltParams.BackgroundColor, &color, sizeof(DXVA2_AYUVSample16));

    // init m_Sample structure
    m_Sample.Start = 0;
    m_Sample.End = 1;
    m_Sample.SampleFormat = format;
    m_Sample.PlanarAlpha.Fraction = 0;
    m_Sample.PlanarAlpha.Value = 1;

    m_bIsA2rgb10 = FALSE;
}

bool CD3D9Device::CheckOverlaySupport()
{
    D3DCAPS9                d3d9caps;
    D3DOVERLAYCAPS          d3doverlaycaps = {0};
    IDirect3D9ExOverlayExtension *d3d9overlay = NULL;
    bool overlaySupported = false;

    memset(&d3d9caps, 0, sizeof(d3d9caps));
    HRESULT hr = m_pD3D9->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &d3d9caps);
    if (FAILED(hr) || !(d3d9caps.Caps & D3DCAPS_OVERLAY))
    {
        overlaySupported = false;
    }
    else
    {
        hr = m_pD3D9->QueryInterface(IID_PPV_ARGS(&d3d9overlay));
        if (FAILED(hr) || (d3d9overlay == NULL))
        {
            overlaySupported = false;
        }
        else
        {
            hr = d3d9overlay->CheckDeviceOverlayType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                m_D3DPP.BackBufferWidth,
                m_D3DPP.BackBufferHeight,
                m_D3DPP.BackBufferFormat, NULL,
                D3DDISPLAYROTATION_IDENTITY, &d3doverlaycaps);
            MSDK_SAFE_RELEASE(d3d9overlay);

            if (FAILED(hr))
            {
                overlaySupported = false;
            }
            else
            {
                overlaySupported = true;
            }
        }
    }

    return overlaySupported;
}

mfxStatus CD3D9Device::FillD3DPP(mfxHDL hWindow, mfxU16 nViews, D3DPRESENT_PARAMETERS &D3DPP)
{
    mfxStatus sts = MFX_ERR_NONE;

    D3DPP.Windowed = true;
    D3DPP.hDeviceWindow = (HWND)hWindow;

    D3DPP.Flags                      = D3DPRESENTFLAG_VIDEO;
    D3DPP.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
    D3DPP.PresentationInterval       = D3DPRESENT_INTERVAL_IMMEDIATE; // note that this setting leads to an implicit timeBeginPeriod call
    D3DPP.BackBufferCount            = 1;
    D3DPP.BackBufferFormat           = (m_bIsA2rgb10) ? D3DFMT_A2R10G10B10 : D3DFMT_X8R8G8B8;

    if (hWindow)
    {
        RECT r;
        GetClientRect((HWND)hWindow, &r);
        int x = GetSystemMetrics(SM_CXSCREEN);
        int y = GetSystemMetrics(SM_CYSCREEN);
        D3DPP.BackBufferWidth  = std::min<LONG>(r.right - r.left, x);
        D3DPP.BackBufferHeight = std::min<LONG>(r.bottom - r.top, y);
    }
    else
    {
        D3DPP.BackBufferWidth  = GetSystemMetrics(SM_CYSCREEN);
        D3DPP.BackBufferHeight = GetSystemMetrics(SM_CYSCREEN);
    }
    //
    // Mark the back buffer lockable if software DXVA2 could be used.
    // This is because software DXVA2 device requires a lockable render target
    // for the optimal performance.
    //
    {
        D3DPP.Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
    }

    bool isOverlaySupported = CheckOverlaySupport();
    if (2 == nViews && !isOverlaySupported)
        return MFX_ERR_UNSUPPORTED;

    bool needOverlay = (2 == nViews) ? true : false;

    D3DPP.SwapEffect = needOverlay ? D3DSWAPEFFECT_OVERLAY : D3DSWAPEFFECT_DISCARD;

    return sts;
}

mfxStatus CD3D9Device::Init(
    mfxHDL hWindow,
    mfxU16 nViews,
    mfxU32 nAdapterNum)
{
    mfxStatus sts = MFX_ERR_NONE;

    if (2 < nViews)
        return MFX_ERR_UNSUPPORTED;

    m_nViews = nViews;

    HRESULT hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &m_pD3D9);
    if (!m_pD3D9 || FAILED(hr))
        return MFX_ERR_DEVICE_FAILED;

    ZeroMemory(&m_D3DPP, sizeof(m_D3DPP));
    sts = FillD3DPP(hWindow, nViews, m_D3DPP);
    MSDK_CHECK_STATUS(sts, "FillD3DPP failed");

    hr = m_pD3D9->CreateDeviceEx(
        nAdapterNum,
        D3DDEVTYPE_HAL,
        (HWND)hWindow,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE,
        &m_D3DPP,
        NULL,
        &m_pD3DD9);
    if (FAILED(hr))
        return MFX_ERR_NULL_PTR;

    if(hWindow)
    {
        hr = m_pD3DD9->ResetEx(&m_D3DPP, NULL);
        if (FAILED(hr))
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        hr = m_pD3DD9->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
        if (FAILED(hr))
            return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    UINT resetToken = 0;

    hr = DXVA2CreateDirect3DDeviceManager9(&resetToken, &m_pDeviceManager9);
    if (FAILED(hr))
        return MFX_ERR_NULL_PTR;

    hr = m_pDeviceManager9->ResetDevice(m_pD3DD9, resetToken);
    if (FAILED(hr))
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    m_resetToken = resetToken;

    return sts;
}

mfxStatus CD3D9Device::Reset()
{
    HRESULT hr = NO_ERROR;
    MSDK_CHECK_POINTER(m_pD3DD9, MFX_ERR_NULL_PTR);

    if (m_D3DPP.Windowed)
    {
        RECT r;
        GetClientRect((HWND)m_D3DPP.hDeviceWindow, &r);
        int x = GetSystemMetrics(SM_CXSCREEN);
        int y = GetSystemMetrics(SM_CYSCREEN);
        m_D3DPP.BackBufferWidth  = std::min<LONG>(r.right - r.left, x);
        m_D3DPP.BackBufferHeight = std::min<LONG>(r.bottom - r.top, y);
    }
    else
    {
        m_D3DPP.BackBufferWidth  = GetSystemMetrics(SM_CXSCREEN);
        m_D3DPP.BackBufferHeight = GetSystemMetrics(SM_CYSCREEN);
    }

    // Reset will change the parameters, so use a copy instead.
    D3DPRESENT_PARAMETERS d3dpp = m_D3DPP;
    hr = m_pD3DD9->ResetEx(&d3dpp, NULL);

    if (FAILED(hr))
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    hr = m_pDeviceManager9->ResetDevice(m_pD3DD9, m_resetToken);
    if (FAILED(hr))
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    return MFX_ERR_NONE;
}

void CD3D9Device::Close()
{
    MSDK_SAFE_RELEASE(m_pDXVAVP_Left);
    MSDK_SAFE_RELEASE(m_pDXVAVP_Right);
    MSDK_SAFE_RELEASE(m_pDXVAVPS);

    MSDK_SAFE_RELEASE(m_pDeviceManager9);
    MSDK_SAFE_RELEASE(m_pD3DD9);
    MSDK_SAFE_RELEASE(m_pD3D9);
}

CD3D9Device::~CD3D9Device()
{
    Close();
}

mfxStatus CD3D9Device::GetHandle(mfxHandleType type, mfxHDL *pHdl)
{
    if (MFX_HANDLE_DIRECT3D_DEVICE_MANAGER9 == type && pHdl != NULL)
    {
        *pHdl = m_pDeviceManager9;

        return MFX_ERR_NONE;
    }
    return MFX_ERR_UNSUPPORTED;
}

mfxStatus CD3D9Device::SetHandle(mfxHandleType type, mfxHDL hdl)
{
    if (MFX_HANDLE_DEVICEWINDOW == type && hdl != NULL) //for render window handle
    {
        m_D3DPP.hDeviceWindow = (HWND)hdl;
        return MFX_ERR_NONE;
    }
    return MFX_ERR_UNSUPPORTED;
}

mfxStatus CD3D9Device::RenderFrame(mfxFrameSurface1 * pSurface, mfxFrameAllocator * pmfxAlloc)
{
    HRESULT hr = S_OK;

    // Rendering of MVC is not supported
    if (2 == m_nViews)
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    MSDK_CHECK_POINTER(pSurface, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(m_pDeviceManager9, MFX_ERR_NOT_INITIALIZED);
    MSDK_CHECK_POINTER(pmfxAlloc, MFX_ERR_NULL_PTR);

    hr = m_pD3DD9->TestCooperativeLevel();

    switch (hr)
    {
        case D3D_OK :
            break;

        case D3DERR_DEVICELOST :
        {
            return MFX_ERR_DEVICE_LOST;
        }

        case D3DERR_DEVICENOTRESET :
            {
            return MFX_ERR_UNKNOWN;
        }

        default :
        {
            return MFX_ERR_UNKNOWN;
        }
    }

    CComPtr<IDirect3DSurface9> pBackBuffer;
    hr = m_pD3DD9->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);

    mfxHDLPair* dxMemId = (mfxHDLPair*)pSurface->Data.MemId;

    hr = m_pD3DD9->StretchRect((IDirect3DSurface9*)dxMemId->first, NULL, pBackBuffer, NULL, D3DTEXF_LINEAR);
    if (FAILED(hr))
    {
        return MFX_ERR_UNKNOWN;
    }

    if (SUCCEEDED(hr))
    {
        hr = m_pD3DD9->Present(NULL, NULL, NULL, NULL);
    }

    return SUCCEEDED(hr) ? MFX_ERR_NONE : MFX_ERR_DEVICE_FAILED;
}

mfxStatus CD3D9Device::CreateVideoProcessors()
{
    if (2 == m_nViews)
        return MFX_ERR_UNDEFINED_BEHAVIOR;

   MSDK_SAFE_RELEASE(m_pDXVAVP_Left);
   MSDK_SAFE_RELEASE(m_pDXVAVP_Right);

   HRESULT hr ;

   ZeroMemory(&m_backBufferDesc, sizeof(m_backBufferDesc));
   IDirect3DSurface9 *backBufferTmp = NULL;
   hr = m_pD3DD9->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBufferTmp);
   if (NULL != backBufferTmp)
       backBufferTmp->GetDesc(&m_backBufferDesc);
   MSDK_SAFE_RELEASE(backBufferTmp);

   if (SUCCEEDED(hr))
   {
       // Create DXVA2 Video Processor Service.
       hr = DXVA2CreateVideoService(m_pD3DD9,
           IID_IDirectXVideoProcessorService,
           (void**)&m_pDXVAVPS);
   }

   if (SUCCEEDED(hr))
   {
       hr = m_pDXVAVPS->CreateVideoProcessor(DXVA2_VideoProcProgressiveDevice,
           &m_VideoDesc,
           m_D3DPP.BackBufferFormat,
           1,
           &m_pDXVAVP_Right);
   }

   return SUCCEEDED(hr) ? MFX_ERR_NONE : MFX_ERR_DEVICE_FAILED;
}

#endif // #if defined(WIN32) || defined(WIN64)
