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

#include <math.h>

#include "sample_camera_utils.h"
#include "sysmem_allocator.h"

#ifdef D3D_SURFACES_SUPPORT
#include "d3d_allocator.h"
#endif

/* ******************************************************************* */

msdk_char* FourCC2Str( mfxU32 FourCC )
{
    msdk_char* strFourCC = MSDK_STRING("YV12");//default

    switch ( FourCC )
    {
    case MFX_FOURCC_NV12:
        strFourCC = MSDK_STRING("NV12");
        break;

    case MFX_FOURCC_YV12:
        strFourCC = MSDK_STRING("YV12");
        break;

    case MFX_FOURCC_YUY2:
        strFourCC = MSDK_STRING("YUY2");
        break;

    case MFX_FOURCC_RGB3:
        strFourCC = MSDK_STRING("RGB3");
        break;

    case MFX_FOURCC_RGB4:
        strFourCC = MSDK_STRING("RGB4");
        break;

    case MFX_FOURCC_P010:
        strFourCC = MSDK_STRING("P010");
        break;

    case MFX_FOURCC_R16:
        strFourCC = MSDK_STRING("R16");
        break;
    }

    return strFourCC;
} // msdk_char* FourCC2Str( mfxU32 FourCC )

/* ******************************************************************* */

#ifdef D3D_SURFACES_SUPPORT
mfxStatus CreateDeviceManager(IDirect3DDeviceManager9** ppManager, mfxU32 nAdapterNum)
{
  MSDK_CHECK_POINTER(ppManager, MFX_ERR_NULL_PTR);

  IDirect3D9Ex* d3d;
  Direct3DCreate9Ex(D3D_SDK_VERSION, &d3d);

  if (!d3d)
  {
    return MFX_ERR_NULL_PTR;
  }

  POINT point = {0, 0};
  HWND window = WindowFromPoint(point);

  D3DPRESENT_PARAMETERS d3dParams;
  memset(&d3dParams, 0, sizeof(d3dParams));
  d3dParams.Windowed = TRUE;
  d3dParams.hDeviceWindow = window;
  d3dParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
  d3dParams.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
  d3dParams.Flags = D3DPRESENTFLAG_VIDEO;
  d3dParams.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
  d3dParams.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
  d3dParams.BackBufferCount = 1;
  d3dParams.BackBufferFormat = D3DFMT_X8R8G8B8;
  d3dParams.BackBufferWidth = 0;
  d3dParams.BackBufferHeight = 0;

  CComPtr<IDirect3DDevice9Ex> d3dDevice = 0;
  HRESULT hr = d3d->CreateDeviceEx(
                                nAdapterNum,
                                D3DDEVTYPE_HAL,
                                window,
                                D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE,
                                &d3dParams,
                                NULL,
                                &d3dDevice);

  if (FAILED(hr) || !d3dDevice)
  {
    return MFX_ERR_NULL_PTR;
  }

  UINT resetToken = 0;
  CComPtr<IDirect3DDeviceManager9> d3dDeviceManager = 0;
  hr = DXVA2CreateDirect3DDeviceManager9(&resetToken, &d3dDeviceManager);

  if (FAILED(hr) || !d3dDeviceManager)
  {
    return MFX_ERR_NULL_PTR;
  }

  hr = d3dDeviceManager->ResetDevice(d3dDevice, resetToken);
  if (FAILED(hr))
  {
    return MFX_ERR_UNDEFINED_BEHAVIOR;
  }

  *ppManager = d3dDeviceManager.Detach();

  if (NULL == *ppManager)
  {
    return MFX_ERR_NULL_PTR;
  }

  return MFX_ERR_NONE;

} // mfxStatus CreateDeviceManager(IDirect3DDeviceManager9** ppManager)
#endif


/* ******************************************************************* */

mfxStatus InitSurfaces(sMemoryAllocator* pAllocator, mfxFrameAllocRequest* pRequest, mfxFrameInfo* pInfo, bool bUsedAsExternalAllocator)
{
  mfxStatus sts = MFX_ERR_NONE;
  mfxU16    nFrames, i;

  sts = pAllocator->pMfxAllocator->Alloc(pAllocator->pMfxAllocator->pthis, pRequest, pAllocator->response);
  MSDK_CHECK_STATUS(sts, "pAllocator->pMfxAllocator->Alloc failed");

  nFrames = pAllocator->response->NumFrameActual;
  *(pAllocator->pSurfaces) = new mfxFrameSurface1 [nFrames];

  for (i = 0; i < nFrames; i++)
  {
    memset(&(pAllocator->pSurfaces[0][i]), 0, sizeof(mfxFrameSurface1));
    pAllocator->pSurfaces[0][i].Info = *pInfo;

    if (bUsedAsExternalAllocator)
    {
      pAllocator->pSurfaces[0][i].Data.MemId = pAllocator->response->mids[i];
    }
    else
    {
      sts = pAllocator->pMfxAllocator->Lock(pAllocator->pMfxAllocator->pthis,
                                            pAllocator->response->mids[i],
                                            &(pAllocator->pSurfaces[0][i].Data));
      MSDK_CHECK_STATUS(sts, "pAllocator->pMfxAllocator->Lock failed");
    }
  }

  return sts;

} // mfxStatus InitSurfaces(...)

/* ******************************************************************* */

mfxStatus CARGB16VideoReader::Init(sInputParams* pParams)
{
    Close();

#if defined (WIN32) || defined(WIN64)
    WIN32_FILE_ATTRIBUTE_DATA ffi;
    if ( GetFileAttributesEx(pParams->strSrcFile, GetFileExInfoStandard, &ffi) )
    {
       m_bSingleFileMode = true;
    }
#endif
    msdk_strcopy(m_FileNameBase, pParams->strSrcFile);
    m_FileNum = 0;

    m_Width  = pParams->frameInfo->nWidth;
    m_Height = pParams->frameInfo->nHeight;

    return MFX_ERR_NONE;
}

CARGB16VideoReader::~CARGB16VideoReader()
{
    Close();
}

void CARGB16VideoReader::Close()
{
    if (m_fSrc != 0)
    {
        fclose(m_fSrc);
        m_fSrc = 0;
    }
}

mfxStatus CARGB16VideoReader::LoadNextFrame(mfxFrameData* pData, mfxFrameInfo* pInfo, mfxU32 type)
{
    return ( m_bSingleFileMode ) ? LoadNextFrameSingle(pData, pInfo, type) : LoadNextFrameSequential(pData, pInfo, type);
}

mfxStatus CARGB16VideoReader::LoadNextFrameSingle(mfxFrameData* pData, mfxFrameInfo* pInfo, mfxU32)
{
    MSDK_CHECK_POINTER(pData, MFX_ERR_NOT_INITIALIZED);
    MSDK_CHECK_POINTER(pInfo, MFX_ERR_NOT_INITIALIZED);

    if ( m_FileNum )
    {
        // File has been read already
        return MFX_ERR_MORE_DATA;
    }

    MSDK_FOPEN(m_fSrc, m_FileNameBase, MSDK_STRING("rb"));
    MSDK_CHECK_POINTER(m_fSrc, MFX_ERR_MORE_DATA);

    mfxI32 w, h, i, pitch;
    mfxI32 nBytesRead;
    mfxU16 *ptr = std::min({pData->Y16, pData->V16, pData->U16});

    w = m_Width;
    h = m_Height;
    int shift = 16 - pInfo->BitDepthLuma;
    pitch = (mfxI32)(pData->Pitch >> 1);
    for (i = 0; i < h; i++)
    {
#if defined(_WIN32) || defined(_WIN64)
        nBytesRead = (mfxI32)fread_s(ptr + i * pitch, pData->Pitch, sizeof(mfxU16), 4*w, m_fSrc);
#else
        nBytesRead = (mfxI32)fread(ptr + i * pitch, sizeof(mfxU16), 4*w, m_fSrc);
#endif
        IOSTREAM_CHECK_NOT_EQUAL(nBytesRead, 4*w, MFX_ERR_MORE_DATA);

        for (int j = 0; j < 4*w; j++)
            ptr[i*pitch + j] = ptr[i*pitch + j] << shift;
    }
    pData->FrameOrder = m_FileNum;
    m_FileNum++;
    fclose(m_fSrc);

    return MFX_ERR_NONE;
}

mfxStatus CARGB16VideoReader::LoadNextFrameSequential(mfxFrameData* pData, mfxFrameInfo* pInfo, mfxU32 type)
{
    MSDK_CHECK_POINTER(pData, MFX_ERR_NOT_INITIALIZED);
    MSDK_CHECK_POINTER(pInfo, MFX_ERR_NOT_INITIALIZED);

    int filenameIndx = m_FileNum;

    msdk_char fname[MSDK_MAX_FILENAME_LEN];

    const msdk_char *pExt;
    switch (type) {
    case MFX_FOURCC_ARGB16:
        pExt = MSDK_STRING("argb16");
        break;
    case MFX_FOURCC_ABGR16:
        pExt = MSDK_STRING("abrg16");
        break;
    default:
        return MFX_ERR_UNSUPPORTED;
    }

#if defined(_WIN32) || defined(_WIN64)
    msdk_sprintf(fname, MSDK_MAX_FILENAME_LEN, MSDK_STRING("%s%08d.%s"), m_FileNameBase, filenameIndx, pExt);
#else
    msdk_sprintf(fname, MSDK_STRING("%s%08d.%s"), m_FileNameBase, filenameIndx, pExt);
#endif

    MSDK_FOPEN(m_fSrc, fname, MSDK_STRING("rb"));
    MSDK_CHECK_POINTER(m_fSrc, MFX_ERR_MORE_DATA);

    mfxI32 w, h, i, pitch;
    mfxI32 nBytesRead;
    mfxU16 *ptr = std::min({pData->Y16, pData->V16, pData->U16});

    w = m_Width;
    h = m_Height;

    pitch = (mfxI32)(pData->Pitch >> 1);

    for (i = 0; i < h; i++)
    {
#if defined(_WIN32) || defined(_WIN64)
        nBytesRead = (mfxI32)fread_s(ptr + i * pitch, pData->Pitch, sizeof(mfxU16), 4*w, m_fSrc);
#else
        nBytesRead = (mfxI32)fread(ptr + i * pitch, sizeof(mfxU16), 4*w, m_fSrc);
#endif
        IOSTREAM_CHECK_NOT_EQUAL(nBytesRead, 4*w, MFX_ERR_MORE_DATA);
    }

    pData->FrameOrder = m_FileNum;
    m_FileNum++;

    fclose(m_fSrc);

    return MFX_ERR_NONE;
}

mfxStatus CBufferedVideoReader::LoadNextFrame(mfxFrameData* pData, mfxFrameInfo* pInfo, mfxU32 bayerType)
{
    bayerType; pInfo;

    pData->Y16 = buffer[nCurrentFrame];
    pData->FrameOrder = nCurrentFrame;
    nCurrentFrame++;
    return MFX_ERR_NONE;
}

mfxStatus CBufferedVideoReader::Init(sInputParams *pParams) {
    //Valid only for Bayer sequence input
    msdk_strcopy(m_FileNameBase, pParams->strSrcFile);
    m_FileNum = 0;
    nFramesToProceed = pParams->nFramesToProceed;
    m_DoPadding = pParams->bDoPadding;
    printf("Buffering ");
    while (m_FileNum < pParams->nFramesToProceed) {
        int filenameIndx = m_FileNum;
        msdk_char fname[MSDK_MAX_FILENAME_LEN];
        const msdk_char *pExt;
        switch (pParams->inputType) {
        case MFX_CAM_BAYER_GRBG:
            pExt = MSDK_STRING("gr16");
            break;
        case MFX_CAM_BAYER_GBRG:
            pExt = MSDK_STRING("gb16");
            break;
        case MFX_CAM_BAYER_BGGR:
            pExt = MSDK_STRING("bg16");
            break;
        case MFX_CAM_BAYER_RGGB:
        default:
            pExt = MSDK_STRING("rg16");
            break;
        }

#if defined(_WIN32) || defined(_WIN64)
        msdk_sprintf(fname, MSDK_MAX_FILENAME_LEN, MSDK_STRING("%s%08d.%s"), m_FileNameBase, filenameIndx, pExt);
#else
        msdk_sprintf(fname, MSDK_STRING("%s%08d.%s"), m_FileNameBase, filenameIndx, pExt);
#endif
        MSDK_FOPEN(m_fSrc, fname, MSDK_STRING("rb"));

        MSDK_CHECK_POINTER(m_fSrc, MFX_ERR_MORE_DATA);

        mfxI32 w, h, i, pitch;
        mfxI32 nBytesRead;

        w = m_Width = pParams->frameInfo->nWidth;
        h = m_Height = pParams->frameInfo->nHeight;

        //buffer for FOURCC_R16 is w*h*2
        mfxU16* data = new mfxU16[w * h * 2];
        mfxU16 *ptr = data;

        pitch = w;

        if (!m_DoPadding)
        {
            for (i = 0; i < h; i++)
            {
                nBytesRead = (mfxI32)fread(ptr + i * pitch, sizeof(mfxU16), w, m_fSrc);

                IOSTREAM_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
            }
        }
        buffer.push_back(data);
        printf(".");
        m_FileNum++;

        fclose(m_fSrc);
    }
    printf("\n");
    return MFX_ERR_NONE;
}

CBufferedVideoReader::~CBufferedVideoReader() {
    Close();
}

void CBufferedVideoReader::Close() {
    if (m_fSrc != 0)
    {
        fclose(m_fSrc);
        m_fSrc = 0;
    }
    while (buffer.size() > 0) {
        if (buffer[0]) {
            delete buffer[0];
            buffer.erase(buffer.begin());
        }
    }
}

CRawVideoReader::CRawVideoReader()
{
    m_fSrc = 0;
    m_bSingleFileMode = false;
    m_Height = m_Width = 0;
    m_FileNum = 0;
    m_DoPadding = false;
#ifdef CONVERT_TO_LSB
    m_pPaddingBuffer = 0;
#endif
} // CRawVideoReader::CRawVideoReader()

mfxStatus CRawVideoReader::Init(sInputParams* pParams)
{
    Close();

#if defined (WIN32) || defined(WIN64)
    WIN32_FILE_ATTRIBUTE_DATA ffi;
    if ( GetFileAttributesEx(pParams->strSrcFile, GetFileExInfoStandard, &ffi) )
    {
       m_bSingleFileMode = true;
    }
#endif
    msdk_strcopy(m_FileNameBase, pParams->strSrcFile);
    m_FileNum = 0;
    m_DoPadding = pParams->bDoPadding;

#ifdef CONVERT_TO_LSB
    MSDK_SAFE_DELETE(m_pPaddingBuffer);
    m_paddingBufSize = pParams->frameInfo[VPP_IN].nWidth;
    m_pPaddingBuffer = new mfxU16[m_paddingBufSize];
#endif

    m_Width  = pParams->frameInfo->nWidth;
    m_Height = pParams->frameInfo->nHeight;

    return MFX_ERR_NONE;

} // mfxStatus CRawVideoReader::Init(const msdk_char *strFileName)

CRawVideoReader::~CRawVideoReader()
{
  Close();

} // CRawVideoReader::~CRawVideoReader()

void CRawVideoReader::Close()
{
  if (m_fSrc != 0)
  {
    fclose(m_fSrc);
    m_fSrc = 0;
  }

#ifdef CONVERT_TO_LSB
  MSDK_SAFE_DELETE(m_pPaddingBuffer)
#endif

} // void CRawVideoReader::Close()

void CRawVideoReader::SetStartFileNumber(mfxI32 fileNum)
{
    m_FileNum = fileNum;
}

mfxStatus CRawVideoReader::LoadNextFrame(mfxFrameData* pData, mfxFrameInfo* pInfo, mfxU32 bayerType)
{
    return ( m_bSingleFileMode ) ? LoadNextFrameSingle(pData, pInfo, bayerType) : LoadNextFrameSequential(pData, pInfo, bayerType);
}

mfxStatus CRawVideoReader::LoadNextFrameSingle(mfxFrameData* pData, mfxFrameInfo* pInfo, mfxU32)
{
    MSDK_CHECK_POINTER(pData, MFX_ERR_NOT_INITIALIZED);
    MSDK_CHECK_POINTER(pInfo, MFX_ERR_NOT_INITIALIZED);

    if ( m_FileNum )
    {
        // File has been read already
        return MFX_ERR_MORE_DATA;
    }

    MSDK_FOPEN(m_fSrc, m_FileNameBase, MSDK_STRING("rb"));
    MSDK_CHECK_POINTER(m_fSrc, MFX_ERR_MORE_DATA);

    mfxI32 w, h, i, j, pitch;
    mfxI32 nBytesRead;
    mfxU16 *ptr = pData->Y16;

    w = m_Width;
    h = m_Height;

    pitch = (mfxI32)(pData->Pitch >> 1);

#ifdef CONVERT_TO_LSB
    int shift = 16 - pInfo->BitDepthLuma;
#endif

    if (!m_DoPadding)
    {
        for (i = 0; i < h; i++)
        {
#if defined(_WIN32) || defined(_WIN64)
            nBytesRead = (mfxI32)fread_s(ptr + i * pitch, pData->Pitch, sizeof(mfxU16), w, m_fSrc);
#else
            nBytesRead = (mfxI32)fread(ptr + i * pitch, sizeof(mfxU16), w, m_fSrc);
#endif
            IOSTREAM_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
        }
    }
    else
    {
        ptr = pData->Y16 + 8 + 8*pitch;
        for (i = 0; i < h; i++)
        {
            mfxU16 *rowPtr = ptr + i * pitch;
#ifdef CONVERT_TO_LSB
            nBytesRead = (mfxI32)fread_s(m_pPaddingBuffer, m_paddingBufSize*sizeof(mfxU16), sizeof(mfxU16), w, m_fSrc);
            IOSTREAM_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
            for (j = 0; j < w; j++)
                rowPtr[j] = m_pPaddingBuffer[j] >> shift;
#else
#if defined(_WIN32) || defined(_WIN64)
            nBytesRead = (mfxI32)fread_s(rowPtr, w*sizeof(mfxU16), sizeof(mfxU16), w, m_fSrc);
#else
            nBytesRead = (mfxI32)fread(rowPtr, sizeof(mfxU16), w, m_fSrc);
#endif
            IOSTREAM_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
#endif
            for (j = 0; j < 7; j++)
            {
                rowPtr[-j - 1] = rowPtr[j + 1];
                rowPtr[w + j] = rowPtr[w - 2 - j];
            }
            rowPtr[-8] = rowPtr[-7]; // not used
            rowPtr[w + 7] = rowPtr[w + 6]; // not used
        }

        for (j = 0; j < 7; j++)
        {
            mfxU16 *pDst = pData->Y16 + (7 - j)*pitch;
            mfxU16  *pSrc = pData->Y16 + (9 + j)*pitch;
            MSDK_MEMCPY(pDst, pSrc, (w + 16)*sizeof(mfxU16));
        }
        MSDK_MEMCPY(pData->Y16, pData->Y16+pitch, (w + 16)*sizeof(mfxU16));

        for (j = 0; j < 7; j++)
        {
            mfxU16 *pDst = pData->Y16 + (8 + h + j)*pitch;
            mfxU16 *pSrc = pData->Y16 + (8 + h - 2 - j)*pitch;
            MSDK_MEMCPY(pDst, pSrc, (w + 16)*sizeof(mfxU16));
        }
        MSDK_MEMCPY(pData->Y16 + (15 + h)*pitch, pData->Y16 + (14 + h)*pitch, (w + 16)*sizeof(mfxU16));
   }

    pData->FrameOrder = m_FileNum;
    m_FileNum++;

    fclose(m_fSrc);

    return MFX_ERR_NONE;
}

mfxStatus CRawVideoReader::LoadNextFrameSequential(mfxFrameData* pData, mfxFrameInfo* pInfo, mfxU32 bayerType)
{
    MSDK_CHECK_POINTER(pData, MFX_ERR_NOT_INITIALIZED);
    MSDK_CHECK_POINTER(pInfo, MFX_ERR_NOT_INITIALIZED);

    //if (m_FileNum == 0)
    //    m_FileNum++;
    int filenameIndx = m_FileNum;

#ifdef READING_LOOP
    filenameIndx %= 118;
    if (filenameIndx == 0) filenameIndx++;
#endif
    msdk_char fname[MSDK_MAX_FILENAME_LEN];

    const msdk_char *pExt;

    switch (bayerType) {
    case MFX_CAM_BAYER_GRBG:
        pExt = MSDK_STRING("gr16");
        break;
    case MFX_CAM_BAYER_GBRG:
        pExt = MSDK_STRING("gb16");
        break;
    case MFX_CAM_BAYER_BGGR:
        pExt = MSDK_STRING("bg16");
        break;
    case MFX_CAM_BAYER_RGGB:
    default:
        pExt = MSDK_STRING("rg16");
        break;
    }
#if defined(_WIN32) || defined(_WIN64)
    msdk_sprintf(fname, MSDK_MAX_FILENAME_LEN, MSDK_STRING("%s%08d.%s"), m_FileNameBase, filenameIndx, pExt);
#else
    msdk_sprintf(fname, MSDK_STRING("%s%08d.%s"), m_FileNameBase, filenameIndx, pExt);
#endif
    //msdk_sprintf(fname, MSDK_MAX_FILENAME_LEN, MSDK_STRING("%s"), m_FileNameBase, filenameIndx);

    MSDK_FOPEN(m_fSrc, fname, MSDK_STRING("rb"));
    MSDK_CHECK_POINTER(m_fSrc, MFX_ERR_MORE_DATA);

    //MSDK_FOPEN(m_fSrc, m_FileNameBase, MSDK_STRING("rb"));
    //MSDK_CHECK_POINTER(m_fSrc, MFX_ERR_MORE_DATA);

    mfxI32 w, h, i, j, pitch;
    mfxI32 nBytesRead;
    mfxU16 *ptr = pData->Y16;

    w = m_Width;
    h = m_Height;

    pitch = (mfxI32)(pData->Pitch >> 1);

#ifdef CONVERT_TO_LSB
    int shift = 16 - pInfo->BitDepthLuma;
#endif

    if (!m_DoPadding)
    {
        for (i = 0; i < h; i++)
        {
#if defined(_WIN32) || defined(_WIN64)
            nBytesRead = (mfxI32)fread_s(ptr + i * pitch, pData->Pitch, sizeof(mfxU16), w, m_fSrc);
#else
            nBytesRead = (mfxI32)fread(ptr + i * pitch, sizeof(mfxU16), w, m_fSrc);
#endif
            IOSTREAM_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
        }
    }
    else
    {
        ptr = pData->Y16 + 8 + 8*pitch;
        for (i = 0; i < h; i++)
        {
            mfxU16 *rowPtr = ptr + i * pitch;
#ifdef CONVERT_TO_LSB
            nBytesRead = (mfxI32)fread_s(m_pPaddingBuffer, m_paddingBufSize*sizeof(mfxU16), sizeof(mfxU16), w, m_fSrc);
            IOSTREAM_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
            for (j = 0; j < w; j++)
                rowPtr[j] = m_pPaddingBuffer[j] >> shift;
#else
#if defined(_WIN32) || defined(_WIN64)
            nBytesRead = (mfxI32)fread_s(rowPtr, w*sizeof(mfxU16), sizeof(mfxU16), w, m_fSrc);
#else
            nBytesRead = (mfxI32)fread(rowPtr, sizeof(mfxU16), w, m_fSrc);
#endif
            IOSTREAM_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
#endif
            for (j = 0; j < 7; j++)
            {
                rowPtr[-j - 1] = rowPtr[j + 1];
                rowPtr[w + j] = rowPtr[w - 2 - j];
            }
            rowPtr[-8] = rowPtr[-7]; // not used
            rowPtr[w + 7] = rowPtr[w + 6]; // not used
        }

        for (j = 0; j < 7; j++)
        {
            mfxU16 *pDst = pData->Y16 + (7 - j)*pitch;
            mfxU16 *pSrc = pData->Y16 + (9 + j)*pitch;
            MSDK_MEMCPY(pDst, pSrc, (w + 16)*sizeof(mfxU16));
        }
        MSDK_MEMCPY(pData->Y16, pData->Y16+pitch, (w + 16)*sizeof(mfxU16));

        for (j = 0; j < 7; j++)
        {
            mfxU16 *pDst = pData->Y16 + (8 + h + j)*pitch;
            mfxU16  *pSrc = pData->Y16 + (8 + h - 2 - j)*pitch;
            MSDK_MEMCPY(pDst, pSrc, (w + 16)*sizeof(mfxU16));
        }
        MSDK_MEMCPY(pData->Y16 + (15 + h)*pitch, pData->Y16 + (14 + h)*pitch, (w + 16)*sizeof(mfxU16));
   }

    pData->FrameOrder = m_FileNum;
    m_FileNum++;

    fclose(m_fSrc);

    return MFX_ERR_NONE;
}


/* ******************************************************************* */

CBmpWriter::CBmpWriter()
{
  MSDK_ZERO_MEMORY(m_bih);
  MSDK_ZERO_MEMORY(m_bfh);
  m_maxNumFilesToCreate = -1;
  m_FileNum = 0;
  return;
}

mfxStatus CBmpWriter::Init(const msdk_char *strFileNameBase, mfxU32 width, mfxU32 height, mfxI32 maxNumFiles)
{
    MSDK_CHECK_POINTER(strFileNameBase, MFX_ERR_NULL_PTR);
    msdk_strcopy(m_FileNameBase, strFileNameBase);

    int buffSize = width * height * 4; //RGBA-8

    m_bfh.bfType = 0x4D42;
    m_bfh.bfSize = sizeof(BITMAPINFOHEADER) + sizeof(BITMAPFILEHEADER) + buffSize;
    m_bfh.bfOffBits = sizeof(BITMAPINFOHEADER) + sizeof(BITMAPFILEHEADER);

    m_bih.biSize   = sizeof(BITMAPINFOHEADER);
    m_bih.biHeight = height ;
    m_bih.biWidth  = width;
    m_bih.biBitCount = 32;
    m_bih.biPlanes = 1;
    m_bih.biCompression = BI_RGB;

    if (maxNumFiles > 0)
        m_maxNumFilesToCreate = maxNumFiles;

    return MFX_ERR_NONE;
}

#define CHECK_NOT_EQUAL(P, X, ERR)          {if ((X) != (P)) {fclose(f); return ERR;}}

mfxStatus CBmpWriter::WriteFrame(mfxFrameData* pData, const msdk_char *fileId, mfxFrameInfo* pInfo)
{
    msdk_char fname[MSDK_MAX_FILENAME_LEN];
    FILE *f;

    if (m_maxNumFilesToCreate > 0 && m_FileNum >= m_maxNumFilesToCreate)
        return MFX_ERR_NONE;

    MSDK_CHECK_POINTER(pData, MFX_ERR_NOT_INITIALIZED);
    MSDK_CHECK_POINTER(pInfo, MFX_ERR_NOT_INITIALIZED);

#if defined(_WIN32) || defined(_WIN64)
    if (fileId)
        msdk_sprintf(fname, MSDK_MAX_FILENAME_LEN, MSDK_STRING("%s%s.bmp"), m_FileNameBase, fileId);
    else
        msdk_sprintf(fname, MSDK_MAX_FILENAME_LEN, MSDK_STRING("%s%d.bmp"), m_FileNameBase, m_FileNum);
#else
    if (fileId)
        msdk_sprintf(fname, MSDK_STRING("%s%s.bmp"), m_FileNameBase, fileId);
    else
        msdk_sprintf(fname, MSDK_STRING("%s%d.bmp"), m_FileNameBase, m_FileNum);
#endif

    m_FileNum++;

    MSDK_FOPEN(f, fname, MSDK_STRING("wb"));
    MSDK_CHECK_POINTER(f, MFX_ERR_NULL_PTR);

    mfxU32 nbytes;

    nbytes = (mfxU32)fwrite(&m_bfh, 1, sizeof(BITMAPFILEHEADER), f);
    CHECK_NOT_EQUAL(nbytes, sizeof(BITMAPFILEHEADER), MFX_ERR_UNDEFINED_BEHAVIOR);

    nbytes = (mfxU32)fwrite(&m_bih, 1, sizeof(BITMAPINFOHEADER), f);
    CHECK_NOT_EQUAL(nbytes, sizeof(BITMAPINFOHEADER), MFX_ERR_UNDEFINED_BEHAVIOR);

    mfxU32 width_bytes = m_bih.biWidth * 4;

    if (pInfo->FourCC == MFX_FOURCC_ARGB16)
    {
        mfxU8 *pOut = new mfxU8[width_bytes];
        for (int y = m_bih.biHeight - 1; y >= 0; y--)
        {
            for (int x = 0; x < (int)width_bytes; x++)
            {
                pOut[x] = (mfxU8)(pData->V16[x + y*(pData->Pitch >> 1)] >> (pInfo->BitDepthLuma - 8));
            }
            nbytes = (mfxU32)fwrite(pOut, 1, width_bytes, f);
            if (nbytes != width_bytes) {
                delete[] pOut;
                fclose(f);
                return MFX_ERR_UNDEFINED_BEHAVIOR;
            }
        }
        delete[] pOut;
    }
    else
    for (int y = m_bih.biHeight - 1; y >= 0; y--)
    {
        nbytes = (mfxU32)fwrite((mfxU8*)pData->B + y*pData->Pitch, 1, width_bytes, f);
        CHECK_NOT_EQUAL(nbytes, width_bytes, MFX_ERR_UNDEFINED_BEHAVIOR);
    }

    fclose(f);

    return MFX_ERR_NONE;
}




/* ******************************************************************* */
CRawVideoWriter::CRawVideoWriter() :
    m_FileNum(0)
    , m_maxNumFilesToCreate(0)
{
    MSDK_ZERO_MEMORY(m_FileNameBase);
}

mfxStatus CRawVideoWriter::Init(sInputParams* pParams)
{
    MSDK_CHECK_POINTER(pParams, MFX_ERR_NULL_PTR);

    msdk_strcopy(m_FileNameBase, pParams->strDstFile);

    if (pParams->maxNumBmpFiles > 0)
        m_maxNumFilesToCreate = pParams->maxNumBmpFiles;
//#ifdef SHIFT_OUT_TO_LSB
//    MSDK_SAFE_DELETE(m_pShiftBuffer);
//    m_shiftBufSize = pParams->frameInfo[VPP_OUT].nWidth;
//    m_pShiftBuffer = new mfxU16[m_shiftBufSize];
//#endif

    return MFX_ERR_NONE;
}

mfxStatus CRawVideoWriter::WriteFrameNV12(mfxFrameData* pData, const msdk_char *fileId, mfxFrameInfo* pInfo) {
    msdk_char fname[MSDK_MAX_FILENAME_LEN];
    FILE *f;
    mfxU32 i = 0, j = 0;
    mfxU32 h = 0, cx = 0, cy = 0, cw = 0, ch = 0;
    mfxU8* buf = pData->Y;
    mfxU32 p = pData->Pitch;

    MSDK_CHECK_POINTER(pData, MFX_ERR_NOT_INITIALIZED);
    MSDK_CHECK_POINTER(pInfo, MFX_ERR_NOT_INITIALIZED);

    #if defined(_WIN32) || defined(_WIN64)
        if (fileId)
            msdk_sprintf(fname, MSDK_MAX_FILENAME_LEN, MSDK_STRING("%s%s.nv12"), m_FileNameBase, fileId);
        else
            msdk_sprintf(fname, MSDK_MAX_FILENAME_LEN, MSDK_STRING("%s%d.nv12"), m_FileNameBase, m_FileNum);
    #else
        if (fileId)
            msdk_sprintf(fname, MSDK_STRING("%s%s.argb16"), m_FileNameBase, fileId);
        else
            msdk_sprintf(fname, MSDK_STRING("%s%d.argb16"), m_FileNameBase, m_FileNum);
    #endif

    m_FileNum++;

    MSDK_FOPEN(f, fname, MSDK_STRING("wb"));
    MSDK_CHECK_POINTER(f, MFX_ERR_NULL_PTR);

    h = pInfo->Height;
    cx = pInfo->CropX;
    cy = pInfo->CropY;
    cw = pInfo->CropW;
    ch = pInfo->CropH;

    for (i = 0; i < ch; ++i)
    {
        fwrite(buf + cx + (cy + i)*p, 1, cw, f);
        fflush(f);
    }
    buf += p*h + cx + cy*p / 2;
    for (i = 0; i < ch / 2; ++i)
        for (j = 1; j <= cw / 2; ++j)
        {
            fwrite(buf + p*i + 2 * j - 2, 1, 1, f);
            fwrite(buf + p*i + 2 * j - 1, 1, 1, f);
        }

    fclose(f);

    return MFX_ERR_NONE;

}



mfxStatus CRawVideoWriter::WriteFrameARGB16(mfxFrameData* pData, const msdk_char *fileId, mfxFrameInfo* pInfo)
{
    mfxU32 i, h, w, pitch;
    mfxU16* ptr;

    msdk_char fname[MSDK_MAX_FILENAME_LEN];
    FILE *f;

    if (m_maxNumFilesToCreate > 0 && m_FileNum >= m_maxNumFilesToCreate)
        return MFX_ERR_NONE;

    MSDK_CHECK_POINTER(pData, MFX_ERR_NOT_INITIALIZED);
    MSDK_CHECK_POINTER(pInfo, MFX_ERR_NOT_INITIALIZED);

#if defined(_WIN32) || defined(_WIN64)
    if (fileId) {
        if (pInfo->FourCC == MFX_FOURCC_ARGB16)
            msdk_sprintf(fname, MSDK_MAX_FILENAME_LEN, MSDK_STRING("%s%s.argb16"), m_FileNameBase, fileId);
        else if (pInfo->FourCC == MFX_FOURCC_ABGR16)
            msdk_sprintf(fname, MSDK_MAX_FILENAME_LEN, MSDK_STRING("%s%s.abgr16"), m_FileNameBase, fileId);
    }
    else {
        if (pInfo->FourCC == MFX_FOURCC_ARGB16)
            msdk_sprintf(fname, MSDK_MAX_FILENAME_LEN, MSDK_STRING("%s%d.argb16"), m_FileNameBase, m_FileNum);
        else if (pInfo->FourCC == MFX_FOURCC_ABGR16)
            msdk_sprintf(fname, MSDK_MAX_FILENAME_LEN, MSDK_STRING("%s%d.abgr16"), m_FileNameBase, m_FileNum);
    }
#else
    if (fileId) {
        if (pInfo->FourCC == MFX_FOURCC_ARGB16)
            msdk_sprintf(fname, MSDK_STRING("%s%s.argb16"), m_FileNameBase, fileId);
        else if (pInfo->FourCC == MFX_FOURCC_ABGR16)
            msdk_sprintf(fname, MSDK_STRING("%s%s.abgr16"), m_FileNameBase, fileId);
    }
    else {
        if (pInfo->FourCC == MFX_FOURCC_ARGB16)
            msdk_sprintf(fname, MSDK_STRING("%s%d.argb16"), m_FileNameBase, m_FileNum);
        else if (pInfo->FourCC == MFX_FOURCC_ABGR16)
            msdk_sprintf(fname, MSDK_STRING("%s%d.abgr16"), m_FileNameBase, m_FileNum);
    }
#endif

    m_FileNum++;

    MSDK_FOPEN(f, fname, MSDK_STRING("wb"));
    MSDK_CHECK_POINTER(f, MFX_ERR_NULL_PTR);

    if (pInfo->CropH > 0 && pInfo->CropW > 0)
    {
        w = pInfo->CropW;
        h = pInfo->CropH;
    }
    else
    {
        w = pInfo->Width;
        h = pInfo->Height;
    }

    pitch = ( pData->PitchLow + ((mfxU32)pData->PitchHigh << 16))>>1;

    ptr = std::min({pData->Y16, pData->V16, pData->U16}) + pInfo->CropX * 4 + pInfo->CropY * pitch;
#ifdef SHIFT_OUT_TO_LSB
    int shift = 16 - pInfo->BitDepthLuma;
    for (i = 0; i < h; i++)
    {
        mfxU32 j;
        for (j = 0; j < 4*w; j++)
            ptr[i*pitch + j] = ptr[i*pitch + j] >> shift;

        if (fwrite(ptr + i*pitch, sizeof(mfxU16), 4*w, f) != (4*w))
        {
            MSDK_CHECK_STATUS_SAFE(MFX_ERR_UNDEFINED_BEHAVIOR, "fwrite failed", fclose(f));
        }
    }
#else
    for (i = 0; i < h; i++)
    {
        MSDK_CHECK_NOT_EQUAL(fwrite(ptr + i*pitch, sizeof(mfxU16), 4*w, f), 4*w, MFX_ERR_UNDEFINED_BEHAVIOR);
    }
#endif
    fclose(f);

    return MFX_ERR_NONE;

} // mfxStatus CRawVideoWriter::WriteFrame(...)

mfxStatus GetFreeSurface(mfxFrameSurface1* pSurfacesPool, mfxU16 nPoolSize, mfxFrameSurface1** ppSurface)
{
  MSDK_CHECK_POINTER(pSurfacesPool, MFX_ERR_NULL_PTR);
  MSDK_CHECK_POINTER(ppSurface,     MFX_ERR_NULL_PTR);

  mfxU32 timeToSleep = 1; // milliseconds
  mfxU32 numSleeps = MSDK_VPP_WAIT_INTERVAL / timeToSleep + 1; // at least 1

  mfxU32 i = 0;

  //wait if there's no free surface
  while ((MSDK_INVALID_SURF_IDX == GetFreeSurfaceIndex(pSurfacesPool, nPoolSize)) && (i < numSleeps))
  {
    MSDK_SLEEP(timeToSleep);
    i++;
  }

  mfxU16 index = GetFreeSurfaceIndex(pSurfacesPool, nPoolSize);

  if (index < nPoolSize)
  {

    *ppSurface = &(pSurfacesPool[index]);

//    (*ppSurface)->Data.Locked++;
    msdk_atomic_inc16((volatile mfxU16*)&((*ppSurface)->Data.Locked));
    return MFX_ERR_NONE;
  }

  return MFX_ERR_NOT_ENOUGH_BUFFER;

} // mfxStatus GetFreeSurface(...)


void ReleaseSurface(mfxFrameSurface1* pSurface)
{
    if (pSurface->Data.Locked) {
        msdk_atomic_dec16((volatile mfxU16*)&(pSurface->Data.Locked));
        //pSurface->Data.Locked--;
    }
}


/* EOF */
