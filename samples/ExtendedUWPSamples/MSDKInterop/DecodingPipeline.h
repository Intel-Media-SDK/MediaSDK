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
#include "mfxvideo++.h"
#include "Thread11.h"
#include "SimpleBitstreamReader.h"
#include <atomic>
#include <list>
#include "d3d11_device.h"
#include "d3d11_allocator.h"
#include "SysMemAllocator.h"
#include "SurfacesPool.h"
#include "Thread11.h"
#include "sample_defs.h"
#include <mutex>

using namespace Windows::Media::Core;

class CDecodingPipeline : public CThread11
{
public:

    enum PIPELINE_STATUS
    {
        PS_NONE, // No clip or error occured
        PS_STOPPED,
        PS_PAUSED,
        PS_PLAYING,
    };

    CDecodingPipeline();
	virtual ~CDecodingPipeline();

    void Load(Windows::Storage::StorageFile^ file);
    void SetCodecID(mfxU32 codec) { codecID = codec; }
    bool IsHWLib;
    mfxU32 AsyncDepth;

    int GetProgressPromilleage() { return  reader.GetFileSize() ? (int)(reader.GetBytesProcessed() * 1000 / reader.GetFileSize()) : 0; }
    mfxVideoParam GetDecoderParams() { return isInitialized ? decoderParams : mfxVideoParam{ 0 }; }
	mfxStatus GetSurfaceFromQueue(CMfxFrameSurfaceExt*& decodedSurface);

    bool IsInitialized() { return isInitialized; }

protected:
    virtual bool OnStart() override;
    virtual bool RunOnce() override;
    virtual void OnClose() override;

private:
    CDecodingPipeline(const CDecodingPipeline&) = delete;
    CDecodingPipeline& operator = (const CDecodingPipeline&) = delete;

private:
    mfxStatus InitSession();
    mfxStatus SyncOneSurface();
	int GetOutputSurfacesCount();

    MFXVideoSession session;
    mfxIMPL impl;

    CSimpleBitstreamReader reader;
    std::atomic<bool> isInitialized;
    
    mfxVideoParam decoderParams;
    MFXVideoDECODE* pDecoder=NULL;
    mfxU32 codecID;

    SysMemFrameAllocator allocator;
    CSurfacesPool surfacesPool;
    std::list<CMfxFrameSurfaceExt*> decodingSurfaces;

    Windows::Storage::StorageFile^ fileSource;

    bool isDecodingEnding;

    std::list<CMfxFrameSurfaceExt*> outputSurfaces;
    std::mutex mtxSurfaces;

    void EnqueueSurface(CMfxFrameSurfaceExt* surface);
	void Cleanup();
};