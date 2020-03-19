/******************************************************************************\
Copyright (c) 2018-2019, Intel Corporation
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

#include "fei_utils.h"

SurfacesPool::SurfacesPool(MFXFrameAllocator* allocator)
    : m_pAllocator(allocator)
{
    MSDK_ZERO_MEMORY(m_response);
}

SurfacesPool::~SurfacesPool()
{
    if (m_pAllocator)
    {
        m_pAllocator->Free(m_pAllocator->pthis, &m_response);
        m_pAllocator = NULL;
    }
}

void SurfacesPool::SetAllocator(MFXFrameAllocator* allocator)
{
    m_pAllocator = allocator;
}

mfxStatus SurfacesPool::AllocSurfaces(mfxFrameAllocRequest& request)
{
    MSDK_CHECK_POINTER(m_pAllocator, MFX_ERR_NULL_PTR);

    // alloc frames
    mfxStatus sts = m_pAllocator->Alloc(m_pAllocator->pthis, &request, &m_response);
    MSDK_CHECK_STATUS(sts, "m_pAllocator->Alloc failed");

    mfxFrameSurface1 surf;
    MSDK_ZERO_MEMORY(surf);

    MSDK_MEMCPY_VAR(surf.Info, &request.Info, sizeof(mfxFrameInfo));

    m_pool.reserve(m_response.NumFrameActual);
    for (mfxU16 i = 0; i < m_response.NumFrameActual; i++)
    {
        if (m_response.mids)
        {
            surf.Data.MemId = m_response.mids[i];
        }
        m_pool.push_back(surf);
    }

    return MFX_ERR_NONE;
}

mfxFrameSurface1* SurfacesPool::GetFreeSurface()
{
    if (m_pool.empty()) // seems AllocSurfaces wasn't called
        return nullptr;

    CTimer t;
    t.Start();

    do
    {
        auto it = std::find_if(std::begin(m_pool), std::end(m_pool), [](const mfxFrameSurface1 & item) { return item.Data.Locked == 0; });

        if (it != std::end(m_pool))
        {
            return &(*it);
        }

        MSDK_SLEEP(1);

    } while (t.GetTime() < MSDK_SURFACE_WAIT_INTERVAL / 1000);

    msdk_printf(MSDK_STRING("ERROR: No free surfaces in pool (during long period)\n"));

    return nullptr;
}

mfxStatus SurfacesPool::LockSurface(mfxFrameSurface1* pSurf)
{
    mfxStatus sts = MFX_ERR_NOT_INITIALIZED;

    if (m_pAllocator)
    {
        sts = m_pAllocator->Lock(m_pAllocator->pthis, pSurf->Data.MemId, &(pSurf->Data));
        MSDK_CHECK_STATUS(sts, "m_pAllocator->Lock failed");
    }

    return sts;
}

mfxStatus SurfacesPool::UnlockSurface(mfxFrameSurface1* pSurf)
{
    mfxStatus sts = MFX_ERR_NOT_INITIALIZED;

    if (m_pAllocator)
    {
        sts = m_pAllocator->Unlock(m_pAllocator->pthis, pSurf->Data.MemId, &(pSurf->Data));
        MSDK_CHECK_STATUS(sts, "m_pAllocator->Unlock failed");
    }

    return sts;
}

/**********************************************************************************/

mfxStatus Decoder::QueryIOSurf(mfxFrameAllocRequest* request)
{
    return m_DEC->QueryIOSurf(&m_par, request);
}

mfxStatus Decoder::PreInit()
{
    mfxStatus sts = MFX_ERR_NONE;

    sts = m_FileReader.Init(m_inPars.strSrcFile);
    MSDK_CHECK_STATUS(sts, "Can't open input file");

    m_Bitstream.Extend(1024 * 1024);

    m_DEC.reset(new MFXVideoDECODE(*m_session));

    m_par.AsyncDepth  = 1;
    m_par.mfx.CodecId = m_inPars.DecodeId;
    m_par.IOPattern   = MFX_IOPATTERN_OUT_VIDEO_MEMORY;

    sts = InitDecParams(m_par);
    MSDK_CHECK_STATUS(sts, "Can't initialize decoder params");

    return sts;
}

mfxStatus Decoder::GetActualFrameInfo(mfxFrameInfo & info)
{
    info = m_par.mfx.FrameInfo;

    return MFX_ERR_NONE;
}

mfxStatus Decoder::Init()
{
    mfxStatus sts = MFX_ERR_NONE;

    m_par.mfx.DecodedOrder = true;

    sts = m_DEC->Init(&m_par);
    MSDK_CHECK_STATUS(sts, "Decoder initialization failed");

    return sts;
}

mfxStatus Decoder::GetFrame(mfxFrameSurface1* & outSrf)
{
    mfxStatus sts = MFX_ERR_MORE_SURFACE;
    mfxFrameSurface1 * workSrf = NULL;
    mfxSyncPoint syncp = NULL;
    bool bEOS = false;

    while (MFX_ERR_MORE_DATA == sts || MFX_ERR_MORE_SURFACE == sts || MFX_ERR_NONE < sts)
    {
        if (MFX_WRN_DEVICE_BUSY == sts)
        {
            WaitForDeviceToBecomeFree(*m_session, m_LastSyncp, sts);
        }
        else if (MFX_ERR_MORE_DATA == sts)
        {
            sts = m_FileReader.ReadNextFrame(&m_Bitstream);
            if (MFX_ERR_MORE_DATA == sts)
            {
                bEOS = true;
                sts = MFX_ERR_NONE;
            }
            MSDK_BREAK_ON_ERROR(sts);
        }
        else if (MFX_ERR_MORE_SURFACE == sts)
        {
            workSrf = m_pOutSurfPool->GetFreeSurface();
            MSDK_CHECK_POINTER(workSrf, MFX_ERR_MEMORY_ALLOC);
        }

        sts = m_DEC->DecodeFrameAsync(bEOS ? NULL : &m_Bitstream, workSrf, &outSrf, &syncp);

        if (bEOS && MFX_ERR_MORE_DATA == sts)
            break;

        if (MFX_ERR_NONE == sts)
        {
            m_LastSyncp = syncp;
        }

        if (syncp && MFX_ERR_NONE < sts)
        {
            sts = MFX_ERR_NONE;
        }

    }
    if (MFX_ERR_NONE == sts && syncp)
    {
        sts = m_session->SyncOperation(syncp, MSDK_WAIT_INTERVAL);
        MSDK_CHECK_STATUS(sts, "Decoder SyncOperation failed");
    }

    return sts;
}

void Decoder::Close()
{
    return;
}

mfxStatus Decoder::InitDecParams(MfxVideoParamsWrapper & par)
{
    mfxStatus sts = MFX_ERR_NONE;

    sts = m_FileReader.ReadNextFrame(&m_Bitstream);
    if (MFX_ERR_MORE_DATA == sts)
        return sts;

    MSDK_CHECK_STATUS(sts, "ReadNextFrame failed");

    for (;;)
    {
        sts = m_DEC->DecodeHeader(&m_Bitstream, &par);

        if (MFX_ERR_MORE_DATA == sts)
        {
            if (m_Bitstream.MaxLength == m_Bitstream.DataLength)
            {
                m_Bitstream.Extend(m_Bitstream.MaxLength * 2);
            }

            sts = m_FileReader.ReadNextFrame(&m_Bitstream);
            if (MFX_ERR_MORE_DATA == sts)
                return sts;
        }
        else
            break;
    }

    return sts;
}

/**********************************************************************************/

void DrawPixel(mfxI32 x, mfxI32 y, mfxU8 *pPic, mfxI32 nPicWidth, mfxI32 nPicHeight, mfxU8 u8Pixel)
{
    mfxI32 nPixPos;

    if (!pPic || x < 0 || x >= nPicWidth || y < 0 || y >= nPicHeight)
        return;

    nPixPos = y * nPicWidth + x;
    pPic[nPixPos] = u8Pixel;
}
// Bresenham's line algorithm
void DrawLine(mfxI32 x0, mfxI32 y0, mfxI32 dx, mfxI32 dy, mfxU8 *pPic, mfxI32 nPicWidth, mfxI32 nPicHeight, mfxU8 u8Pixel)
{
    using std::swap;

    mfxI32 x1 = x0 + dx;
    mfxI32 y1 = y0 + dy;
    bool bSteep = abs(dy) > abs(dx);
    if (bSteep)
    {
        swap(x0, y0);
        swap(x1, y1);
    }
    if (x0 > x1)
    {
        swap(x0, x1);
        swap(y0, y1);
    }
    mfxI32 nDeltaX = x1 - x0;
    mfxI32 nDeltaY = abs(y1 - y0);
    mfxI32 nError = nDeltaX / 2;
    mfxI32 nYStep;
    if (y0 < y1)
        nYStep = 1;
    else
        nYStep = -1;

    for (; x0 <= x1; x0++)
    {
        if (bSteep)
            DrawPixel(y0, x0, pPic, nPicWidth, nPicHeight, u8Pixel);
        else
            DrawPixel(x0, y0, pPic, nPicWidth, nPicHeight, u8Pixel);

        nError -= nDeltaY;
        if (nError < 0)
        {
            y0 += nYStep;
            nError += nDeltaX;
        }
    }
}

/**********************************************************************************/

mfxStatus CopySurface(mfxFrameSurface1 & dst, const mfxFrameSurface1 & src)
{
    if (src.Info.FourCC != dst.Info.FourCC ||
        src.Info.Width  != dst.Info.Width  ||
        src.Info.Height != dst.Info.Height)
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    switch (src.Info.FourCC)
    {
    case MFX_FOURCC_NV12:
        for (mfxU32 i = 0; i < src.Info.CropH/2; ++i)
        {
            memcpy(dst.Data.Y + i * dst.Data.Pitch,
                    src.Data.Y + i * src.Data.Pitch,
                    src.Info.Width);
            memcpy(dst.Data.UV + i * dst.Data.Pitch,
                    src.Data.UV + i * src.Data.Pitch,
                    src.Info.Width);
        }
        for (mfxU32 i = src.Info.CropH/2; i < src.Info.CropH; ++i)
        {
            memcpy(dst.Data.Y + i * dst.Data.Pitch,
                    src.Data.Y + i * src.Data.Pitch,
                    src.Info.Width);
        }
        dst.Info.CropW = src.Info.CropW;
        dst.Info.CropH = src.Info.CropH;

    break;
    default:
        return MFX_ERR_UNSUPPORTED;
    }
    return MFX_ERR_NONE;
}
