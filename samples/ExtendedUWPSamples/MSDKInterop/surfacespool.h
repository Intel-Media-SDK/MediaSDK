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

#include "mfxstructures.h"
#include <map>
#include "MfxFrameSurfaceExt.h"

#define MSDK_SURFACE_WAIT_INTERVAL 20000

bool AreFrameInfosEqual(const mfxFrameInfo& info1, const mfxFrameInfo& info2);

class CSurfacesPool
{
public:
    CSurfacesPool(mfxU32 sleepInterval=10):SleepInterval(sleepInterval) {}
    void Create(mfxMemId* mids, mfxU32 midsCount, mfxU32 allocID, mfxU16 memType, mfxFrameAllocator* pAllocator, const mfxFrameInfo& frameInfo);
    ~CSurfacesPool(void);

    void Clear(void);

    CMfxFrameSurfaceExt* GetFreeSurface();
  
    int GetUserLockedCount();
    int GetLockedCount();
    mfxU16 GetSurfacesCount(){return (mfxU16)surfaces.size();}

    bool IsCompatibleDecoderPool(const mfxFrameAllocRequest* pRequest);
    mfxFrameAllocResponse GenerateAllocResponse();

    mfxU32 SleepInterval; // milliseconds
    std::map<mfxMemId,CMfxFrameSurfaceExt> surfaces;

    CMfxFrameSurfaceExt* GetExtSurface(mfxFrameSurface1* pSurf);

protected:
    CSurfacesPool(const CSurfacesPool&)=delete;
    CSurfacesPool& operator =(const CSurfacesPool&)=delete;

    // IMPORTANT:mids stored inside response object and passed here are reused in Pool object. 
    // DO NOT free mids array after calling to this function (it will be automatically freed during pool object destruction)
    mfxStatus AddSurfaces(mfxMemId* mids, mfxU32 midsCount, mfxU32 allocID, mfxU16 memType, const mfxFrameInfo& frameInfo);

    mfxU32 allocID = 0;
    mfxU16 memType = 0;
};

