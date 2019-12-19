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
#include "DecodingPipeline.h"
#include "sample_defs.h"
#include "MSDKHandle.h"

#define WAIT_INTERVAL 20000
#define BUFFERED_FRAMES_NUM 5

CDecodingPipeline::CDecodingPipeline() : CThread11(1)
{
    fileSource = nullptr;
    IsHWLib = true;
    codecID = MFX_CODEC_AVC;
    AsyncDepth = 4;
    impl= 0;
    isDecodingEnding = false;
    isInitialized = false;
	pDecoder = NULL;

    MSDK_ZERO_MEMORY(decoderParams);
}

CDecodingPipeline::~CDecodingPipeline()
{
	Stop();
	Cleanup();
}

bool CDecodingPipeline::OnStart()
{
    isInitialized = false;

	Cleanup();

    // Initialize session
    if (InitSession() < MFX_ERR_NONE)
    {
        return false;
    }

    // Loading file
    reader.Close();
	if (reader.Init(fileSource) < MFX_ERR_NONE)
	{
		return false;
	}

    // Creating decoder
    if (!pDecoder)
    {
        pDecoder = new(std::nothrow) MFXVideoDECODE(session);
    }

    if (!pDecoder)
    {
        return false;
    }

    // Decoding bitstream header to get params
    mfxStatus sts = MFX_ERR_MORE_BITSTREAM;

    //--- Decoding Stream Header 
    MSDK_ZERO_MEMORY(decoderParams);
    decoderParams.mfx.CodecId = codecID;
    //decoderParams.IOPattern = MFX_IOPATTERN_OUT_VIDEO_MEMORY;
    decoderParams.IOPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
    decoderParams.AsyncDepth = AsyncDepth;

    reader.ReadNextFrame();

    // Querying for correct parameters
    sts = pDecoder->DecodeHeader(&reader.BitStream, &decoderParams);

    if (sts != MFX_ERR_NONE)
    {
        MSDK_PRINT_RET_MSG(sts, "Error reading stream header");
        return false;
    }

    //--- Querying for number of surfaces required for the decoding process
    mfxFrameAllocRequest req;
    MSDK_ZERO_MEMORY(req);
    sts = pDecoder->QueryIOSurf(&decoderParams, &req);
    MSDK_CHECK_STATUS_BOOL(sts, "Decoder QueryIOSurf failed");

    //--- Allocating surfaces and putting them into surfaces pool
    req.NumFrameMin = req.NumFrameSuggested = req.NumFrameSuggested + BUFFERED_FRAMES_NUM; // One more surface for rendering
    mfxFrameAllocResponse resp;
    MSDK_ZERO_MEMORY(resp); 
    sts = allocator.AllocFrames(&req, &resp);
    MSDK_CHECK_STATUS_BOOL(sts, "Cannot allocate frames");
    surfacesPool.Create(resp.mids, resp.NumFrameActual, 0, req.Type, &allocator, req.Info);

    //--- Initializing decoder
    sts = pDecoder->Init(&decoderParams);
    MSDK_CHECK_STATUS_BOOL(sts, "Cannot initialize decoder");

    isDecodingEnding = false;
    isInitialized = true;

    return true;
}

int CDecodingPipeline::GetOutputSurfacesCount()
{
	std::unique_lock<std::mutex> lock(mtxSurfaces);
	return (int)outputSurfaces.size();
}


bool CDecodingPipeline::RunOnce()
{
    mfxStatus sts = MFX_ERR_NONE;


    //--- Keep AsyncDepth frames in the queue for best performance
    while (decodingSurfaces.size() + GetOutputSurfacesCount() < AsyncDepth)
    {
        mfxFrameSurface1* pOutSurface = NULL;
        mfxSyncPoint syncPoint = NULL;

        mfxFrameSurface1* pFreeSurface = surfacesPool.GetFreeSurface();
        MSDK_CHECK_POINTER(pFreeSurface, false);

        //--- Decoding one frame
        bool shouldRepeat = true;
        while (shouldRepeat)
        {
            sts = pDecoder->DecodeFrameAsync(isDecodingEnding ? NULL : &reader.BitStream,
                pFreeSurface, &pOutSurface, &syncPoint);

            switch(sts)
            { 
            case MFX_ERR_MORE_DATA:
                if (reader.BitStream.DataLength > (mfxU32)(reader.BitStream.MaxLength*0.9))
                {
                    //--- If bitstream is full of data, but we still have an error - expand buffer
                    reader.ExpandBitstream(reader.BitStream.MaxLength);
                }
                if (isDecodingEnding)
                {
                    //--- Buffered frames are taken, time to close thread
                    return false;
                }
                else if (reader.ReadNextFrame() < MFX_ERR_NONE)
                {
                    //--- If we can't read more data from file - finish decoding, clean up buffers.
                    isDecodingEnding = true;
                }
                break;
            case MFX_ERR_DEVICE_FAILED:
            case MFX_WRN_DEVICE_BUSY:
                // If device failed, sleep and try again
                Sleep(10);
                break;
            case MFX_ERR_MORE_SURFACE:
                pFreeSurface = surfacesPool.GetFreeSurface();
                MSDK_CHECK_POINTER(pFreeSurface, false);
                break;
            default:
                shouldRepeat = false;
                break;
            }
        }

        MSDK_CHECK_STATUS_BOOL(sts, "DecodeFrameAsync failed");

        //--- Putting "decoding request" into queue (at this moment surface does not contain decoded data yet, it's just an empty container)
		if (pOutSurface)
		{
			CMfxFrameSurfaceExt* pOutSurfExt = surfacesPool.GetExtSurface(pOutSurface);
			if (pOutSurfExt)
			{
				pOutSurfExt->LinkedSyncPoint = syncPoint;
				pOutSurfExt->UserLock = true;
				decodingSurfaces.push_back(pOutSurfExt);
			}
		}
    }
    
    return SyncOneSurface()>=MFX_ERR_NONE;
}

mfxStatus CDecodingPipeline::SyncOneSurface()
{
    if (decodingSurfaces.size())
    {
        //--- Synchronizing surface to finish decoding and get actual frame data
        CMfxFrameSurfaceExt* pFrontSurface = decodingSurfaces.front();
        mfxStatus sts = session.SyncOperation(pFrontSurface->LinkedSyncPoint, WAIT_INTERVAL);
        MSDK_CHECK_STATUS(sts, "SyncOperation failed");
        decodingSurfaces.front()->LinkedSyncPoint = NULL;

        //--- Pushing surface to the output queue
        EnqueueSurface(pFrontSurface);
        decodingSurfaces.pop_front();
    }
    return MFX_ERR_NONE;
}

void CDecodingPipeline::OnClose()
{
	//--- Syncing the rest of surfaces in the queue
	while (decodingSurfaces.size() && SyncOneSurface() >= MFX_ERR_NONE);

    //--- Signal that decoding is over
    EnqueueSurface(NULL);

	//--- Just in case of error, remove userlock from the rest surfaces in decodingSurfaces - to avoid hangs
	for (auto surf : decodingSurfaces)
	{
		surf->UserLock = false;
	}
}

void CDecodingPipeline::Cleanup()
{
	{
		std::unique_lock<std::mutex> lock(mtxSurfaces);
		outputSurfaces.clear();
	}

	// Assuming that all surfaces in surface pool are not used in other part of application (because pipeline should be destoroyed or restared only afer all previously allocated surfaces are free).
    mfxFrameAllocResponse resp = surfacesPool.GenerateAllocResponse();
    mfxStatus sts=allocator.FreeFrames(&resp);
    surfacesPool.Clear();
	
	if (pDecoder)
	{
		pDecoder->Close();
		delete pDecoder;
		pDecoder = NULL;
	}

	decodingSurfaces.clear();
    reader.Close();

	session.Close();

    MSDK_ZERO_MEMORY(decoderParams);
    isInitialized = false;
}

mfxStatus CDecodingPipeline::InitSession()
{
    mfxInitParam initPar;
    mfxVersion version;     // real API version with which library is initialized
    mfxStatus sts = MFX_ERR_NONE;

    MSDK_ZERO_MEMORY(initPar);
    
    // we set version to 1.0 and later we will query actual version of the library which will got leaded
    initPar.Version.Major = 1;
    initPar.Version.Minor = 0;

    initPar.GPUCopy = true;

    //--- Init session
    if (IsHWLib)
    {
        // try searching on all display adapters
        initPar.Implementation = MFX_IMPL_HARDWARE_ANY;
        initPar.Implementation |= MFX_IMPL_VIA_D3D11;

        // Library should pick first available compatible adapter during InitEx call with MFX_IMPL_HARDWARE_ANY
        sts = session.InitEx(initPar);
    }
    else
    {
        initPar.Implementation = MFX_IMPL_SOFTWARE;
        sts = session.InitEx(initPar);
    }

    MSDK_CHECK_STATUS(sts, "m_mfxSession.Init failed");

    //--- Query library version  and implementation
    sts = session.QueryVersion(&version); // get real API version of the loaded library
    MSDK_CHECK_STATUS(sts, "m_mfxSession.QueryVersion failed");

    sts = session.QueryIMPL(&impl); // get actual library implementation
    MSDK_CHECK_STATUS(sts, "m_mfxSession.QueryIMPL failed");

    sts = allocator.Init(NULL);
    MSDK_CHECK_STATUS(sts, "Allocator Init failed");
    sts = session.SetFrameAllocator(&allocator);
    MSDK_CHECK_STATUS(sts, "SetFrameAllocator failed");

    return MFX_ERR_NONE;
}

void CDecodingPipeline::Load(Windows::Storage::StorageFile^ file)
{
    Stop();
    fileSource = file;
}

void CDecodingPipeline::EnqueueSurface(CMfxFrameSurfaceExt* surface)
{
    std::unique_lock<std::mutex> lock(mtxSurfaces);
    outputSurfaces.push_back(surface);
}

mfxStatus CDecodingPipeline::GetSurfaceFromQueue(CMfxFrameSurfaceExt*& decodedSurface)
{
    std::unique_lock<std::mutex> lock(mtxSurfaces);
	if (outputSurfaces.empty())
	{
		return MFX_ERR_NOT_FOUND;
	}

	decodedSurface = outputSurfaces.front();
	outputSurfaces.pop_front();
	return decodedSurface == NULL ? MFX_ERR_MORE_DATA : MFX_ERR_NONE;
}