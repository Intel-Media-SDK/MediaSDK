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

#ifndef __D3D11_ALLOCATOR_H__
#define __D3D11_ALLOCATOR_H__

#include "base_allocator.h"
#include <limits>

#ifdef __gnu_linux__
#include <stdint.h> // for uintptr_t on Linux
#endif

#if (defined(_WIN32) || defined(_WIN64))

#include <d3d11.h>
#include <vector>
#include <map>

struct ID3D11VideoDevice;
struct ID3D11VideoContext;

struct D3D11AllocatorParams : mfxAllocatorParams
{
    ID3D11Device *pDevice;
    bool bUseSingleTexture;
    DWORD uncompressedResourceMiscFlags;

    D3D11AllocatorParams()
        : pDevice()
        , bUseSingleTexture()
        , uncompressedResourceMiscFlags()
    {
    }
};

class D3D11FrameAllocator: public BaseFrameAllocator
{
public:

    D3D11FrameAllocator();
    virtual ~D3D11FrameAllocator();

    virtual mfxStatus Init(mfxAllocatorParams *pParams);
    virtual mfxStatus Close();
    virtual ID3D11Device * GetD3D11Device()
    {
        return m_initParams.pDevice;
    };
    virtual mfxStatus LockFrame(mfxMemId mid, mfxFrameData *ptr);
    virtual mfxStatus UnlockFrame(mfxMemId mid, mfxFrameData *ptr);
    virtual mfxStatus GetFrameHDL(mfxMemId mid, mfxHDL *handle);

protected:
    static  DXGI_FORMAT ConverColortFormat(mfxU32 fourcc);
    virtual mfxStatus CheckRequestType(mfxFrameAllocRequest *request);
    virtual mfxStatus ReleaseResponse(mfxFrameAllocResponse *response);
    virtual mfxStatus AllocImpl(mfxFrameAllocRequest *request, mfxFrameAllocResponse *response);
    virtual mfxStatus ReallocImpl(mfxMemId midIn, const mfxFrameInfo *info, mfxU16 memType, mfxMemId *midOut);

    D3D11AllocatorParams m_initParams;
    ID3D11DeviceContext *m_pDeviceContext;

    struct TextureResource
    {
        std::vector<mfxMemId> outerMids;
        std::vector<ID3D11Texture2D*> textures;
        std::vector<ID3D11Texture2D*> stagingTexture;
        bool             bAlloc;

        TextureResource()
            : bAlloc(true)
        {
        }

        static bool isAllocated (TextureResource & that)
        {
            return that.bAlloc;
        }
        ID3D11Texture2D* GetTexture(mfxMemId id)
        {
            if (outerMids.empty())
                return NULL;

            return textures[((uintptr_t)id - (uintptr_t)outerMids.front()) % textures.size()];
        }
        UINT GetSubResource(mfxMemId id)
        {
            if (outerMids.empty())
                return NULL;

            return (UINT)(((uintptr_t)id - (uintptr_t)outerMids.front()) / textures.size());
        }
        void Release()
        {
            size_t i = 0;
            for(i = 0; i < textures.size(); i++)
            {
                textures[i]->Release();
            }
            textures.clear();

            for(i = 0; i < stagingTexture.size(); i++)
            {
                stagingTexture[i]->Release();
            }
            stagingTexture.clear();

            //marking texture as deallocated
            bAlloc = false;
        }
    };
    class TextureSubResource
    {
        TextureResource * m_pTarget;
        ID3D11Texture2D * m_pTexture;
        ID3D11Texture2D * m_pStaging;
        UINT m_subResource;
    public:
        TextureSubResource(TextureResource * pTarget = NULL, mfxMemId id = 0)
            : m_pTarget(pTarget)
            , m_pTexture()
            , m_subResource()
            , m_pStaging(NULL)
        {
            if (NULL != m_pTarget && !m_pTarget->outerMids.empty())
            {
                ptrdiff_t idx = (uintptr_t)MFXReadWriteMid(id).raw() - (uintptr_t)m_pTarget->outerMids.front();
                m_pTexture = m_pTarget->textures[idx % m_pTarget->textures.size()];
                m_subResource = (UINT)(idx / m_pTarget->textures.size());
                m_pStaging = m_pTarget->stagingTexture.empty() ? NULL : m_pTarget->stagingTexture[idx];
            }
        }
        ID3D11Texture2D* GetStaging()const
        {
            return m_pStaging;
        }
        ID3D11Texture2D* GetTexture()const
        {
            return m_pTexture;
        }
        UINT GetSubResource()const
        {
            return m_subResource;
        }
        void Release()
        {
            if (NULL != m_pTarget)
                m_pTarget->Release();
        }
    };

    TextureSubResource GetResourceFromMid(mfxMemId);

    std::list <TextureResource> m_resourcesByRequest;//each alloc request generates new item in list

    typedef std::list <TextureResource>::iterator referenceType;
    std::vector<referenceType> m_memIdMap;
};

#endif // #if defined(_WIN32) || defined(_WIN64)
#endif // __D3D11_ALLOCATOR_H__
