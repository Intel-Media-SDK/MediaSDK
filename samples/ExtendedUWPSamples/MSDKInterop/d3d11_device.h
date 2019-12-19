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

#pragma once

#if defined( _WIN32 ) || defined ( _WIN64 )

#include "sample_defs.h" // defines MFX_D3D11_SUPPORT

#if MFX_D3D11_SUPPORT
#include "hw_device.h"
#include <windows.h>
#include <d3d11.h>
#include <atlbase.h>

#include <dxgi1_2.h>

class CD3D11Device
{
public:
    CD3D11Device();
    virtual ~CD3D11Device();
    virtual mfxStatus Init(mfxU32 nAdapterNum);
    virtual mfxStatus GetHandle(mfxHandleType type, mfxHDL *pHdl);
    CComPtr<IDXGIFactory2> GetFactory() { return m_pDXGIFactory; }

protected:
    CComPtr<ID3D11Device>                   m_pD3D11Device;
    CComPtr<ID3D11DeviceContext>            m_pD3D11Ctx;

    CComQIPtr<IDXGIAdapter>                 m_pAdapter;

    CComPtr<IDXGIFactory2>                  m_pDXGIFactory;
};

#endif //#if defined( _WIN32 ) || defined ( _WIN64 )
#endif //#if MFX_D3D11_SUPPORT
