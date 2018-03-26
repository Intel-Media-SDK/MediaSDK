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

#if defined(_WIN32) || defined(_WIN64)

#pragma once

#include "d3d_device.h"
#include "d3d11_device.h"
#include "ts_common.h"

#define MFX_CHECK(EXPR, ERR)    { if (!(EXPR)) return (ERR); }

struct D3DAuxiliaryObjects
{
    HANDLE                          m_hRegistrationDevice;

    HANDLE                          m_hRegistration;

    HANDLE                          m_hDXVideoDecoderService;
    HANDLE                          m_hDXVideoProcessorService;

    IDirect3DDeviceManager9         *pD3DDeviceManager;

    IDirectXVideoDecoderService     *m_pDXVideoDecoderService;
    IDirectXVideoProcessorService   *m_pDXVideoProcessorService;

    IDirectXVideoDecoder            *m_pDXVideoDecoder;
    IDirectXVideoProcessor          *m_pDXRegistrationDevice;
    IDirect3DSurface9               *m_pD3DSurface;
    IDirect3DSurface9               *m_pDummySurface;
};

class AuxiliaryDevice
{
public:
    // default constructor
    AuxiliaryDevice(void);
    // default destructor
    ~AuxiliaryDevice(void);

    // initialize auxiliary device
    mfxStatus Initialize(IDirect3DDeviceManager9 *pD3DDeviceManager = NULL);

    // release auxiliary device object
    mfxStatus Release(void);

    // destroy acceleration service
    mfxStatus ReleaseAccelerationService();

    // check that specified device is supported
    mfxStatus IsAccelerationServiceExist(const GUID guid);

    // BeginFrame/Execute/EndFrame commands
    //HRESULT BeginFrame(IDirect3DSurface9 *pRenderTarget, void *pvPVPData);
    //HRESULT EndFrame(HANDLE *pHandleComplete);
    HRESULT Execute(mfxU32 func, void* input, mfxU32 inSize, void* output = NULL, mfxU32 outSize = 0);

    // query capabilities
    mfxStatus QueryAccelCaps(CONST GUID *pAccelGuid, void *pCaps, mfxU32 *puCapSize);

protected:

    mfxStatus CreateSurfaceRegistrationDevice(void);

    // current working service guid
    GUID m_Guid;

    // auxiliary D3D objects
    D3DAuxiliaryObjects D3DAuxObjects;
};

#endif // defined(_WIN32) || defined(_WIN64)