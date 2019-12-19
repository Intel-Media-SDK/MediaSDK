/******************************************************************************\
Copyright (c) 2005-2018, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This sample was distributed or derived from the Intel's Media Samples package.
\**********************************************************************************/

#include "pch.h"

#if defined(_WIN32) || defined(_WIN64)

#include "sample_defs.h"

#if MFX_D3D11_SUPPORT

#include "d3d11_device.h"

CD3D11Device::CD3D11Device()
{
}

CD3D11Device::~CD3D11Device()
{
}

mfxStatus CD3D11Device::Init(
    mfxU32 nAdapterNum)
{
    mfxStatus sts = MFX_ERR_NONE;
    HRESULT hres = S_OK;

    static D3D_FEATURE_LEVEL FeatureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };
    D3D_FEATURE_LEVEL pFeatureLevelsOut;

    hres = CreateDXGIFactory1(__uuidof(IDXGIFactory2), (void**)(&m_pDXGIFactory) );
    if (FAILED(hres))
        return MFX_ERR_DEVICE_FAILED;

    hres = m_pDXGIFactory->EnumAdapters(nAdapterNum,&m_pAdapter);
    if (FAILED(hres))
        return MFX_ERR_DEVICE_FAILED;

    hres =  D3D11CreateDevice(m_pAdapter ,
                            D3D_DRIVER_TYPE_UNKNOWN,
                            NULL,
                            0,
                            FeatureLevels,
                            MSDK_ARRAY_LEN(FeatureLevels),
                            D3D11_SDK_VERSION,
                            &m_pD3D11Device,
                            &pFeatureLevelsOut,
                            &m_pD3D11Ctx);

    if (FAILED(hres))
        return MFX_ERR_DEVICE_FAILED;

    return sts;
}

mfxStatus CD3D11Device::GetHandle(mfxHandleType type, mfxHDL *pHdl)
{
    if (MFX_HANDLE_D3D11_DEVICE == type)
    {
        *pHdl = m_pD3D11Device.p;
        return MFX_ERR_NONE;
    }
    return MFX_ERR_UNSUPPORTED;
}


#endif // #if MFX_D3D11_SUPPORT
#endif // #if defined(_WIN32) || defined(_WIN64)
