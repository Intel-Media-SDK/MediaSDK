/******************************************************************************\
Copyright (c) 2005-2017, Intel Corporation
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

#pragma once

#include <fstream>
#include <sstream>
#include <d3d9.h>
#include <dxva2api.h>
#include <stdexcept>
#include <atlbase.h>

#define D3DFMT_NV12 (D3DFORMAT)MAKEFOURCC('N','V','1','2')

// auto_ptr-like behavior
class LockedD3DDevice
{
private:
    IDirect3DDeviceManager9 *m_pD3DDeviceManager;
    HANDLE m_hDevice;
    IDirect3DDevice9 *m_pD3DDevice;

    // this is a simple helper class - hide operator=
    LockedD3DDevice &operator=(LockedD3DDevice &other);
public:
    LockedD3DDevice(IDirect3DDeviceManager9 *pD3DDeviceManager, HANDLE hDevice)
        : m_pD3DDeviceManager(pD3DDeviceManager)
        , m_hDevice(hDevice)
        , m_pD3DDevice(NULL)
    {
        if (FAILED(m_pD3DDeviceManager->LockDevice(m_hDevice, &m_pD3DDevice, true)))
            throw std::runtime_error("Couldn't lock device");
    }

    // "move" constructor (transfering ownership)
    LockedD3DDevice(LockedD3DDevice &other)
        : m_pD3DDeviceManager(other.m_pD3DDeviceManager)
        , m_hDevice(other.m_hDevice)
        , m_pD3DDevice(other.m_pD3DDevice)
    {
        other.release();
    }

    ~LockedD3DDevice()
    {
        release();
    }

    void release()
    {
        if (m_pD3DDevice)
        {
            m_pD3DDevice->Release();
            m_pD3DDeviceManager->UnlockDevice(m_hDevice, true);

            m_pD3DDevice = NULL;
            m_pD3DDeviceManager = NULL;
            m_hDevice = 0;
        }
    }

    IDirect3DDevice9 *get() const
    {
        return m_pD3DDevice;
    }
};

// simple class providing safe D3DDeviceManager storing and device locking
class SafeD3DDeviceManager
{
protected:
    CComPtr<IDirect3DDeviceManager9> m_pD3DDeviceManager;
    HANDLE m_hDevice;
public:
    SafeD3DDeviceManager(IDirect3DDeviceManager9 *pD3DDeviceManager)
        : m_pD3DDeviceManager(pD3DDeviceManager)
        , m_hDevice(0)
    {
        if (FAILED(m_pD3DDeviceManager->OpenDeviceHandle(&m_hDevice)))
            throw std::runtime_error("Couldn't open Direct3D device handle");
    }
    ~SafeD3DDeviceManager()
    {
        m_pD3DDeviceManager->CloseDeviceHandle(&m_hDevice);
    }
    LockedD3DDevice LockDevice()
    {
        return LockedD3DDevice(m_pD3DDeviceManager, m_hDevice);
    }
};

inline void DumpRect(IDirect3DSurface9 *pSurf, const std::string &file)
{
    std::ofstream out(file.c_str(), std::ios::out | std::ios::binary);

    D3DSURFACE_DESC desc;
    pSurf->GetDesc(&desc);

    RECT rect = {0, 0, (LONG)desc.Width, (LONG)desc.Height};
    D3DLOCKED_RECT locked_rect;
    pSurf->LockRect(&locked_rect, &rect, D3DLOCK_READONLY);
    out.write(reinterpret_cast<const char *>(locked_rect.pBits), locked_rect.Pitch * desc.Height);
    pSurf->UnlockRect();
    out.close();
}