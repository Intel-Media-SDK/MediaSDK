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
#include "SurfacesPool.h"
#include "vm/time_defs.h"
#include "sample_defs.h"

void CSurfacesPool::Create(mfxMemId* mids, mfxU32 midsCount, mfxU32 allocID, mfxU16 memType, mfxFrameAllocator* pAllocator, const mfxFrameInfo& frameInfo)
{
    Clear();

    for (mfxU32 i = 0; i <midsCount; i++)
    {
        CMfxFrameSurfaceExt surf;
        surf.Info = frameInfo;
        surf.Data.MemId = mids[i];
        surf.pAlloc = pAllocator;
        surfaces[mids[i]] = surf;
    }

    this->memType = memType;
    this->allocID = allocID;
}

CSurfacesPool::~CSurfacesPool(void)
{
    Clear();
}

void CSurfacesPool::Clear(void)
{
    surfaces.clear();
}

CMfxFrameSurfaceExt * CSurfacesPool::GetFreeSurface()
{
    mfxU16 idx = MSDK_INVALID_SURF_IDX;

    //wait if there's no free surface
    for (unsigned int i = 0; i < MSDK_SURFACE_WAIT_INTERVAL; i += SleepInterval)
    {
        for (std::map<mfxMemId, CMfxFrameSurfaceExt>::iterator it = surfaces.begin(); it != surfaces.end(); it++, i++)
        {
            if (!it->second.Data.Locked && !it->second.UserLock)
            {
                return &it->second;
            }
        }

        MSDK_SLEEP(SleepInterval);
    }

    MSDK_PRINT_RET_MSG(MFX_ERR_NOT_FOUND,"Cannot find free surface, GetFreeSurface() timed out");
    return NULL;
}

int CSurfacesPool::GetUserLockedCount()
{
    int count=0;
    for (std::map<mfxMemId, CMfxFrameSurfaceExt>::iterator it = surfaces.begin(); it != surfaces.end(); it++)
    {
        if (it->second.UserLock)
        {
            count++;
        }
    }
    return count;
}

int CSurfacesPool::GetLockedCount()
{
    int count=0;
    for (std::map<mfxMemId, CMfxFrameSurfaceExt>::iterator it = surfaces.begin(); it != surfaces.end(); it++)
    {
        if (it->second.Data.Locked)
        {
            count++;
        }
    }
    return count;
}

mfxStatus CSurfacesPool::AddSurfaces(mfxMemId* mids, mfxU32 midsCount, mfxU32 allocID, mfxU16 memType, const mfxFrameInfo& frameInfo)
{
    //TODO: Add Request.Type check after refactoring
    if(this->allocID != allocID)
    {
        MSDK_PRINT_RET_MSG(MFX_ERR_INVALID_VIDEO_PARAM, "SurfacesPool: Attempt to extend surfaces pool with different AllocID is illegal");
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    if(surfaces.size() && !AreFrameInfosEqual(surfaces[0].Info,frameInfo))
    {
        MSDK_PRINT_RET_MSG(MFX_ERR_INVALID_VIDEO_PARAM, "SurfacesPool: Attempt to extend surfaces pool with different FrameInfo is illegal");
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }
    if (MFX_MEMTYPE_BASE(this->memType) != MFX_MEMTYPE_BASE(memType))
    {
        MSDK_PRINT_RET_MSG(MFX_ERR_INVALID_VIDEO_PARAM, "SurfacesPool: Invalid memory type of surfaces added to the pool");
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }
    this->memType |= (memType & 0xFF00);

    // Filling internal surfaces buffer
    for (mfxU32 i = 0; i < midsCount; i++)
    {
        CMfxFrameSurfaceExt surf;
        surf.Info = frameInfo;
        surf.Data.MemId = mids[i];
        surfaces[mids[i]]=surf;
    }

    return MFX_ERR_NONE;
}

bool CSurfacesPool::IsCompatibleDecoderPool(const mfxFrameAllocRequest* pRequest)
{
    return surfaces.size() && 
        (pRequest->AllocId == allocID && pRequest->Info.Width <= surfaces[0].Info.Width && pRequest->Info.Height <= surfaces[0].Info.Height);
}

mfxFrameAllocResponse CSurfacesPool::GenerateAllocResponse()
{
    mfxFrameAllocResponse response;
    MSDK_ZERO_MEMORY(response);
    response.AllocId=allocID;
    response.mids = new mfxMemId[surfaces.size()];
    int i = 0;
    for(std::map<mfxMemId, CMfxFrameSurfaceExt>::iterator it = surfaces.begin(); it!=surfaces.end(); it++, i++)
    {
        response.mids[i]=it->second.Data.MemId;
    }
    response.NumFrameActual=(mfxU16)surfaces.size();
    return response;
}

CMfxFrameSurfaceExt * CSurfacesPool::GetExtSurface(mfxFrameSurface1 * pSurf)
{
    std::map<mfxMemId, CMfxFrameSurfaceExt>::iterator it = surfaces.find(pSurf->Data.MemId);
    return it != surfaces.end() ? &it->second : NULL;
}

bool AreFrameInfosEqual(const mfxFrameInfo& info1, const mfxFrameInfo& info2)
{
    // Treat zero height or width as "any"
    return (info1.Width == 0 || info2.Width == 0 || info1.Width == info2.Width) &&
        (info1.Height == 0 || info2.Height == 0 || info1.Height == info2.Height) && info1.FourCC == info2.FourCC;
}