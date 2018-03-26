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

#include "ts_aux_dev.h"

#if defined(_WIN32) || defined(_WIN64)

#undef  SAFE_RELEASE
#define SAFE_RELEASE(PTR) \
if (PTR) \
{ \
    PTR->Release(); \
    PTR = NULL; \
}

#define D3DFMT_NV12 (D3DFORMAT)(MFX_MAKEFOURCC('N', 'V', '1', '2'))

static const GUID DXVA2_Intel_Auxiliary_Device =
{ 0xa74ccae2, 0xf466, 0x45ae,{ 0x86, 0xf5, 0xab, 0x8b, 0xe8, 0xaf, 0x84, 0x83 } };

// From "Intel DXVA2 Auxiliary Functionality Device rev 0.6"
typedef enum
{
    AUXDEV_GET_ACCEL_GUID_COUNT = 1,
    AUXDEV_GET_ACCEL_GUIDS = 2,
    AUXDEV_GET_ACCEL_RT_FORMAT_COUNT = 3,
    AUXDEV_GET_ACCEL_RT_FORMATS = 4,
    AUXDEV_GET_ACCEL_FORMAT_COUNT = 5,
    AUXDEV_GET_ACCEL_FORMATS = 6,
    AUXDEV_QUERY_ACCEL_CAPS = 7,
    AUXDEV_CREATE_ACCEL_SERVICE = 8,
    AUXDEV_DESTROY_ACCEL_SERVICE = 9
} AUXDEV_FUNCTION_ID;

AuxiliaryDevice::AuxiliaryDevice()
{
    // zeroise objects
    memset(&D3DAuxObjects, 0, sizeof(D3DAuxObjects));
    m_Guid = GUID_NULL;

} // AuxiliaryDevice::AuxiliaryDevice(VideoCORE *pCore, mfxStatus sts)

AuxiliaryDevice::~AuxiliaryDevice(void)
{
    // release object
    Release();

} // AuxiliaryDevice::~AuxiliaryDevice(void)

mfxStatus AuxiliaryDevice::Initialize(IDirect3DDeviceManager9 *pD3DDeviceManager)
{
    HRESULT hr;

    if (pD3DDeviceManager)
    {
        D3DAuxObjects.pD3DDeviceManager = pD3DDeviceManager;
    }

    if (!D3DAuxObjects.m_pDXVideoDecoderService)
    {

        MFX_CHECK(D3DAuxObjects.pD3DDeviceManager, MFX_ERR_NOT_INITIALIZED);

        hr = D3DAuxObjects.pD3DDeviceManager->OpenDeviceHandle(&D3DAuxObjects.m_hDXVideoDecoderService);

        hr = D3DAuxObjects.pD3DDeviceManager->GetVideoService(D3DAuxObjects.m_hDXVideoDecoderService,
                                                              IID_IDirectXVideoDecoderService,
                                                              (void**)&D3DAuxObjects.m_pDXVideoDecoderService);

        D3DAuxObjects.pD3DDeviceManager->CloseDeviceHandle(D3DAuxObjects.m_hDXVideoDecoderService);
        D3DAuxObjects.m_hDXVideoDecoderService = INVALID_HANDLE_VALUE;
        MFX_CHECK(SUCCEEDED(hr), MFX_ERR_DEVICE_FAILED);
    }

    MFX_CHECK(D3DAuxObjects.m_pDXVideoDecoderService, MFX_ERR_NOT_INITIALIZED);

    UINT    cDecoderGuids = 0;
    GUID    *pDecoderGuids = NULL;

    // retrieves an array of GUIDs that identifies the decoder devices supported by the graphics hardware
    hr = D3DAuxObjects.m_pDXVideoDecoderService->GetDecoderDeviceGuids(&cDecoderGuids, &pDecoderGuids);

    MFX_CHECK(cDecoderGuids, MFX_ERR_DEVICE_FAILED);

    // check that intel auxiliary device supported
    while(cDecoderGuids)
    {
        cDecoderGuids--;

        if (DXVA2_Intel_Auxiliary_Device == pDecoderGuids[cDecoderGuids])
        {
            // auxiliary device was found
            break;
        }

        MFX_CHECK(cDecoderGuids, MFX_ERR_UNSUPPORTED);
    }

    if (pDecoderGuids)
    {
        CoTaskMemFree(pDecoderGuids);
    }

    UINT cFormats = 0;
    D3DFORMAT *pFormats = NULL;
    D3DFORMAT format = (D3DFORMAT)MAKEFOURCC('I', 'M', 'C', '3');

    // retrieves the supported render targets for a specified decoder device
    // this function call is necessary only for compliant with MS DXVA2 decoder interface
    // the actual render is obtained through another call
    hr = D3DAuxObjects.m_pDXVideoDecoderService->GetDecoderRenderTargets(DXVA2_Intel_Auxiliary_Device,
                                                                         &cFormats, &pFormats);
    MFX_CHECK(SUCCEEDED(hr), MFX_ERR_DEVICE_FAILED);

    // delete temporal memory
    if (pFormats)
    {
        format = pFormats[0];
        CoTaskMemFree(pFormats);
    }

    DXVA2_VideoDesc             DXVA2VideoDesc = {0};
    DXVA2_ConfigPictureDecode   *pConfig = NULL;
    UINT                        cConfigurations = 0;

    // set surface size as small as possible
    DXVA2VideoDesc.SampleWidth  = 64;
    DXVA2VideoDesc.SampleHeight = 64;

    DXVA2VideoDesc.Format                              = D3DFMT_NV12;

    // creates DXVA decoder render target
    // the surfaces are required by DXVA2 in order to create decoder device
    // and are not used for any others purpose by auxiliary device

    if (D3DAuxObjects.m_pD3DSurface)
    {
        D3DAuxObjects.m_pD3DSurface->Release();
        D3DAuxObjects.m_pD3DSurface = NULL;
    }

    // try to create surface of IMC3 color format
    hr = D3DAuxObjects.m_pDXVideoDecoderService->CreateSurface(DXVA2VideoDesc.SampleWidth,
                                                               DXVA2VideoDesc.SampleHeight,
                                                               0,
                                                               (D3DFORMAT)MAKEFOURCC('I', 'M', 'C', '3'),
                                                               D3DPOOL_DEFAULT,
                                                               0,
                                                               DXVA2_VideoDecoderRenderTarget,
                                                               &D3DAuxObjects.m_pD3DSurface,
                                                               NULL);
    if (FAILED(hr))
    {
        // try color format reported by GetDecoderRenderTargets()
        hr = D3DAuxObjects.m_pDXVideoDecoderService->CreateSurface(DXVA2VideoDesc.SampleWidth,
                                                                   DXVA2VideoDesc.SampleHeight,
                                                                   0,
                                                                   format,
                                                                   D3DPOOL_DEFAULT,
                                                                   0,
                                                                   DXVA2_VideoDecoderRenderTarget,
                                                                   &D3DAuxObjects.m_pD3DSurface,
                                                                   NULL);
    }

    MFX_CHECK(SUCCEEDED(hr), MFX_ERR_DEVICE_FAILED);

    // retrieves the configurations that are available for a decoder device
    // this function call is necessary only for compliant with MS DXVA2 decoder interface
    hr = D3DAuxObjects.m_pDXVideoDecoderService->GetDecoderConfigurations(DXVA2_Intel_Auxiliary_Device,
                                                                          &DXVA2VideoDesc,
                                                                          NULL,
                                                                          &cConfigurations,
                                                                          &pConfig);
    MFX_CHECK(SUCCEEDED(hr), MFX_ERR_DEVICE_FAILED);
    MFX_CHECK(pConfig, MFX_ERR_DEVICE_FAILED);

    DXVA2_ConfigPictureDecode m_Config = pConfig[0];

    // creates a video decoder device
    hr = D3DAuxObjects.m_pDXVideoDecoderService->CreateVideoDecoder(DXVA2_Intel_Auxiliary_Device,
                                                                    &DXVA2VideoDesc,
                                                                    &m_Config,
                                                                    &D3DAuxObjects.m_pD3DSurface,
                                                                    1,
                                                                    &D3DAuxObjects.m_pDXVideoDecoder);
    MFX_CHECK(SUCCEEDED(hr), MFX_ERR_DEVICE_FAILED);

    if (pConfig)
    {
        CoTaskMemFree(pConfig);
    }


    return MFX_ERR_NONE;

} // mfxStatus AuxiliaryDevice::Initialize(void)

mfxStatus AuxiliaryDevice::Release(void)
{
    ReleaseAccelerationService();

    SAFE_RELEASE(D3DAuxObjects.m_pD3DSurface);
    SAFE_RELEASE(D3DAuxObjects.m_pDummySurface);
    SAFE_RELEASE(D3DAuxObjects.m_pDXVideoDecoder);
    SAFE_RELEASE(D3DAuxObjects.m_pDXVideoDecoderService);
    SAFE_RELEASE(D3DAuxObjects.m_pDXVideoProcessorService);
    SAFE_RELEASE(D3DAuxObjects.m_pDXRegistrationDevice);


    return MFX_ERR_NONE;

} // mfxStatus AuxiliaryDevice::Release(void)


mfxStatus AuxiliaryDevice::IsAccelerationServiceExist(const GUID guid)
{
    HRESULT hr;
    mfxU32 guidsCount = 0;

    // obtain number of supported guid
    hr = Execute(AUXDEV_GET_ACCEL_GUID_COUNT, 0, 0, &guidsCount, sizeof(mfxU32));
    MFX_CHECK(SUCCEEDED(hr), MFX_ERR_DEVICE_FAILED);
    MFX_CHECK(guidsCount, MFX_ERR_DEVICE_FAILED);

    std::vector<GUID> guids(guidsCount);
    hr = Execute(AUXDEV_GET_ACCEL_GUIDS, 0, 0, &guids[0], guidsCount * sizeof(GUID));
    MFX_CHECK(SUCCEEDED(hr), MFX_ERR_UNKNOWN);

    mfxU32 i;
    for (i = 0; i < guidsCount; i++)
    {
        if (IsEqualGUID(guids[i], guid))
        {
            break;
        }
    }
    MFX_CHECK(i < guidsCount, MFX_ERR_UNSUPPORTED);

    return MFX_ERR_NONE;

} // mfxStatus AuxiliaryDevice::IsAccelerationServiceExist(const GUID guid)

mfxStatus AuxiliaryDevice::ReleaseAccelerationService()
{
    HRESULT hr;

    if (D3DAuxObjects.m_pDXVideoDecoder && m_Guid != GUID_NULL)
    {
        m_Guid = GUID_NULL;
        hr = Execute(AUXDEV_DESTROY_ACCEL_SERVICE, (void*)&m_Guid, sizeof(GUID));
        MFX_CHECK(SUCCEEDED(hr), MFX_ERR_DEVICE_FAILED);
    }

    return MFX_ERR_NONE;

} // mfxStatus AuxiliaryDevice::ReleaseAccelerationService()

HRESULT AuxiliaryDevice::Execute(mfxU32 func,
                                 void*  input,
                                 mfxU32 inSize,
                                 void*  output,
                                 mfxU32 outSize)
{
    MFX_CHECK(D3DAuxObjects.m_pDXVideoDecoder, E_FAIL);

    if (AUXDEV_CREATE_ACCEL_SERVICE == func && NULL == input)
        return E_POINTER;

    DXVA2_DecodeExtensionData extensionData;
    extensionData.Function = func;
    extensionData.pPrivateInputData = input;
    extensionData.PrivateInputDataSize = inSize;
    extensionData.pPrivateOutputData = output;
    extensionData.PrivateOutputDataSize = outSize;

    DXVA2_DecodeExecuteParams executeParams;
    executeParams.NumCompBuffers = 0;
    executeParams.pCompressedBuffers = 0;
    executeParams.pExtensionData = &extensionData;

    HRESULT hr = D3DAuxObjects.m_pDXVideoDecoder->Execute(&executeParams);

    if (AUXDEV_CREATE_ACCEL_SERVICE == func && SUCCEEDED(hr))
        m_Guid = *(GUID*)input; // to call AUXDEV_DESTROY_ACCEL_SERVICE in destructor

    return hr;

} // HRESULT AuxiliaryDevice::Execute(mfxU32 func, void*  input, mfxU32 inSize, void*  output, mfxU32 outSize)

mfxStatus AuxiliaryDevice::QueryAccelCaps(CONST GUID *pAccelGuid, void *pCaps, UINT *puCapSize)
{
    DXVA2_DecodeExecuteParams sExecute;
    DXVA2_DecodeExtensionData sExtension;

    HRESULT hRes;

    // check parameters (pCaps may be NULL => puCapSize will receive the size of the device caps)
    if (!pAccelGuid || !puCapSize)
    {
        return MFX_ERR_NULL_PTR;
    }

    // check the decoder object
    MFX_CHECK(D3DAuxObjects.m_pDXVideoDecoder, MFX_ERR_NULL_PTR);

    // query caps
    sExtension.Function              = AUXDEV_QUERY_ACCEL_CAPS;
    sExtension.pPrivateInputData     = (PVOID)pAccelGuid;
    sExtension.PrivateInputDataSize  = sizeof(GUID);
    sExtension.pPrivateOutputData    = pCaps;
    sExtension.PrivateOutputDataSize = *puCapSize;

    sExecute.NumCompBuffers     = 0;
    sExecute.pCompressedBuffers = 0;
    sExecute.pExtensionData     = &sExtension;

    hRes = D3DAuxObjects.m_pDXVideoDecoder->Execute(&sExecute);
    MFX_CHECK(SUCCEEDED(hRes), MFX_ERR_DEVICE_FAILED);

    *puCapSize = sExtension.PrivateOutputDataSize;

    return MFX_ERR_NONE;

} // mfxStatus AuxiliaryDevice::QueryAccelCaps(CONST GUID *pAccelGuid, void *pCaps, UINT *puCapSize)

#endif  // defined(_WIN32) || defined(_WIN64)
