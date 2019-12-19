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

#include "EncodingPipeline.h"
#include "SysMemAllocator.h"
#include "SimpleYUVReader.h"

#ifdef  USE_EXTRAS
#include "FFMPEGBitstreamWriter.h"
#endif //  USE_EXTRAS


mfxStatus CEncodingPipeline::SetEncParams(mfxVideoParam& pInParams)
{
	m_mfxEncParams = pInParams;

	m_mfxEncParams.mfx.FrameInfo.Width = MSDK_ALIGN16(pInParams.mfx.FrameInfo.Width);
	m_mfxEncParams.mfx.FrameInfo.Height = (MFX_PICSTRUCT_PROGRESSIVE == m_mfxEncParams.mfx.FrameInfo.PicStruct) ?
		MSDK_ALIGN16(pInParams.mfx.FrameInfo.Height) : MSDK_ALIGN32(pInParams.mfx.FrameInfo.Height);

	// Loading plugin and querying for parameters
	mfxStatus sts = m_pmfxENC->Query(&m_mfxEncParams, &m_mfxEncParams);
	MSDK_CHECK_STATUS(sts, "Query (for encoder) failed");

	return MFX_ERR_NONE;
}


mfxStatus CEncodingPipeline::CreateAllocator()
{
	mfxStatus sts = MFX_ERR_NONE;

	// create system memory allocator
	m_pMFXAllocator = std::make_unique<SysMemFrameAllocator>();
	MSDK_CHECK_POINTER(m_pMFXAllocator, MFX_ERR_MEMORY_ALLOC);

	// initialize memory allocator
	sts = m_pMFXAllocator->Init(m_pmfxAllocatorParams.get());

	return sts;
}

void CEncodingPipeline::DeleteFrames()
{
	// delete surfaces array
	auto encResponse = surfacesPool.GenerateAllocResponse();

	for (auto& pairSurf : surfacesPool.surfaces)
	{
		m_pMFXAllocator->Unlock(m_pMFXAllocator->pthis, pairSurf.second.Data.MemId, &(pairSurf.second.Data));
	}

	// delete frames
	if (m_pMFXAllocator)
	{
		m_pMFXAllocator->Free(m_pMFXAllocator->pthis, &encResponse);
	}

	surfacesPool.Clear();
}

void CEncodingPipeline::DeleteAllocator()
{
	// delete allocator
	m_pMFXAllocator.reset();
	m_pmfxAllocatorParams.reset();
}

CEncodingPipeline::CEncodingPipeline() :
	m_mfxBS({ 0 })
	, m_mfxEncParams({ 0 })
	, m_encCtrl({ 0 })
	, m_pMFXAllocator(nullptr)
	, m_pmfxAllocatorParams(nullptr)
	, m_pmfxENC(nullptr)
	, _CTS(cancellation_token_source())
{
}

CEncodingPipeline::~CEncodingPipeline()
{
	Close();
}

std::shared_ptr<CEncodingPipeline> CEncodingPipeline::Instantiate(mfxVideoParam &pParams)
{
	std::shared_ptr<CEncodingPipeline> enc = std::make_shared<CEncodingPipeline>();

	mfxStatus sts = MFX_ERR_NONE;

	mfxInitParam initPar = { 0 };

	// minimum API version with which library is initialized
	// set to 1.0 and later we will query actual version of
	// the library which will got leaded
	mfxVersion version = { {20, 1} };
	initPar.Version = version;
	// any implementation of library suites our needs
	initPar.Implementation = MFX_IMPL_AUTO_ANY;

	// Library should pick first available compatible adapter during InitEx call with MFX_IMPL_HARDWARE_ANY
	if (enc->m_mfxSession.InitEx(initPar) < MFX_ERR_NONE)
	{
		enc.reset();
		throw std::exception("Cannot initialize MSDK library and create session");
	}
	else if (MFXQueryVersion(enc->m_mfxSession, &version)< MFX_ERR_NONE) // get real API version of the loaded library
	{
		enc.reset();
		throw std::exception("Cannot get API version");
	}	
	else if (!(enc->m_pmfxENC = std::make_unique<MFXVideoENCODE>(enc->m_mfxSession)))
	{ 
		enc.reset();
		throw std::exception("Cannot create encoder object");
	}
	else if (enc->CreateAllocator() < MFX_ERR_NONE)
	{
		enc.reset();
		throw std::exception("Cannot create allocator");
	}
	else if (enc->SetEncParams(pParams) < MFX_ERR_NONE)
	{
		enc.reset();
		throw std::exception("Invalid parameters or plugin cannot be found");
	}
	else if (enc->AllocEncodingPipelineInputFrames() < MFX_ERR_NONE)
	{
		enc.reset();
		throw std::exception("Cannot allocate input surfaces");
	}
	else if (enc->AllocEncodingPipelineOutputBitream() < MFX_ERR_NONE)
	{
		enc.reset();
		throw std::exception("Cannot allocate output bitstream");
	}

	//--- Everything is fine, returning enc object
	return enc;
}

void CEncodingPipeline::Close()
{
	DeleteFrames();

	m_pmfxENC.reset();
	m_mfxSession.Close();

	// allocator if used as external for MediaSDK must be deleted after SDK components
	DeleteAllocator();
}

mfxStatus CEncodingPipeline::AllocEncodingPipelineInputFrames()
{
	//MSDK_CHECK_POINTER(m_pmfxENC, MFX_ERR_NOT_INITIALIZED);

	mfxStatus sts = MFX_ERR_NONE;
	mfxFrameAllocRequest encRequest = { 0 };

	// Calculate the number of surfaces for components.
	// QueryIOSurf functions tell how many surfaces are required to produce at least 1 output.
	// To achieve better performance we provide extra surfaces.
	// 1 extra surface at input allows to get 1 extra output.
	sts = m_pmfxENC->QueryIOSurf(&m_mfxEncParams, &encRequest);
	MSDK_CHECK_STATUS(sts, "QueryIOSurf (for encoder) failed");

	sts = m_pmfxENC->Init(&m_mfxEncParams);
	MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
	MSDK_CHECK_STATUS(sts, "m_pMFXEnc->Init failed");

	if (encRequest.NumFrameSuggested < m_mfxEncParams.AsyncDepth)
		return MFX_ERR_MEMORY_ALLOC;

	// correct initially requested and prepare allocation request
	m_mfxEncParams.mfx.FrameInfo = encRequest.Info;

	// alloc frames for encoder
	mfxFrameAllocResponse encResponse;
	sts = m_pMFXAllocator->Alloc(m_pMFXAllocator->pthis, &encRequest, &encResponse);
	MSDK_CHECK_STATUS(sts, "m_pMFXAllocator->Alloc failed");

	surfacesPool.Create(encResponse.mids, encResponse.NumFrameActual,0, encRequest.Type, m_pMFXAllocator.get(), encRequest.Info);

	//--- System memory surfaces may be kept locked all the time (lock here means that Y,U,V and other fields are filled with proper pointer values)
	for (auto& pairSurf : surfacesPool.surfaces)
	{
		sts = m_pMFXAllocator->Lock(m_pMFXAllocator->pthis, pairSurf.second.Data.MemId, &(pairSurf.second.Data));
		if (sts < MFX_ERR_NONE)
		{
			return sts;
		}
	}

	return sts;
}

void CEncodingPipeline::CancelEncoding()
{
	_CTS.cancel();
}

IAsyncActionWithProgress<double>^ CEncodingPipeline::ReadAndEncodeAsync(StorageFile ^ storageSource, StorageFile ^ storageSink, mfxU32 sourceFileFourCC)
{
	using namespace std;

	//--- Create writer object
	pStreamWriter = 
#ifdef USE_EXTRAS
		storageSink->FileType == ".mp4" || storageSink->FileType == ".MP4" ?
		reinterpret_cast<CStreamWriter*>(new FFMPEGBitstreamWriter()):
#endif
		reinterpret_cast<CStreamWriter*>(new CRawStreamWriter());

	return create_async([=](progress_reporter<double> reporter) 
	{
		//--- Opening input file and reader
		return create_task(storageSource->OpenAsync(FileAccessMode::Read), _CTS.get_token())
			.then([=](IRandomAccessStream^ istr) 
		{
			CSimpleYUVReader reader;

			if(reader.Open(istr,sourceFileFourCC))		
			{
				try
				{
					//--- Preparing data for writing 
					//--- Opening output file
					auto outputStream = create_task(storageSink->OpenAsync(FileAccessMode::ReadWrite), _CTS.get_token()).get();

					mfxStatus sts = MFX_ERR_NONE;

					//--- Initialize writer object
					pStreamWriter->Init(outputStream, m_mfxEncParams);

					int totalLength = 0, tl2 = 0;
					while (MFX_ERR_NONE == sts)
					{
						if (_CTS.get_token().is_canceled())
						{
							pStreamWriter->Close();
							reader.Close();
							cancel_current_task();
						}

						mfxSyncPoint EncSyncP = nullptr;

						//--- Reading raw data 
						mfxFrameSurface1* pSurface = surfacesPool.GetFreeSurface();			
						sts = reader.LoadNextFrame(pSurface);

						if(sts>=MFX_ERR_NONE)
						{
							sts = ProceedFrame(pSurface, EncSyncP);
						}
						else if(sts==MFX_ERR_MORE_DATA)
						{
							//--- If there's no more data in file - call ProceedFrame with NULL instead of surface to push all cached data out of encoder
							sts = ProceedFrame(NULL, EncSyncP);
						}

						//--- Synchronizing operation (waiting for encoding operation to become completed) and writing data
						if (EncSyncP && MFX_ERR_NONE == sts)
						{
							sts = m_mfxSession.SyncOperation(EncSyncP, MSDK_WAIT_INTERVAL);

							//--- Writing compressed bitstream
							if (MFX_ERR_NONE == sts)
							{
								pStreamWriter->WriteNextFrame(&m_mfxBS);
								totalLength += m_mfxBS.DataLength;

								m_mfxBS.DataOffset = m_mfxBS.DataLength = 0;

								//--- Reporting encoding progress
								reporter.report((mfxU32)((reader.GetBytesRead()*100.0) / istr->Size));
							}
						}
					}
					//--- Closing writer
					pStreamWriter->Close();
					reader.Close();
					outputStream = nullptr;

				}
				catch (...)
				{
					pStreamWriter->Close();
					cancel_current_task();
				}
			}

			//
			// please note that continuation context should not be the same as UI thread, that is why use_arbitrary() is used
			// otherwise it will not be possible to wait for asynchronous operations completing by calling get() or wait()
			//
		}, _CTS.get_token(), task_continuation_context::use_arbitrary()).then([](task<void> tsk) {
			tsk.get();
		}, _CTS.get_token());

	});
}

void WipeMfxBitstream(mfxBitstream& pBitstream)
{
	MSDK_SAFE_DELETE_ARRAY(pBitstream.Data);
}

mfxStatus ExtendMfxBitstream(mfxBitstream& pBitstream, mfxU32 nSize)
{
	if (nSize <= pBitstream.MaxLength)
		nSize = std::max<mfxU32>(pBitstream.MaxLength * 2, nSize);

	mfxU8* pData = new (std::nothrow) mfxU8[nSize];
	MSDK_CHECK_POINTER(pData, MFX_ERR_MEMORY_ALLOC);

	if (pBitstream.Data) {
		memmove(pData, pBitstream.Data + pBitstream.DataOffset, pBitstream.DataLength);
	}

	WipeMfxBitstream(pBitstream);

	pBitstream.Data = pData;
	pBitstream.DataOffset = 0;
	pBitstream.MaxLength = nSize;

	return MFX_ERR_NONE;
}

mfxStatus CEncodingPipeline::AllocEncodingPipelineOutputBitream()
{
	mfxVideoParam par = { 0 };

	// find out the required buffer size
	mfxStatus sts = m_pmfxENC->GetVideoParam(&par);
	MSDK_CHECK_STATUS(sts, "GetFirstEncoder failed");

	// reallocate bigger buffer for output
	sts = ExtendMfxBitstream(m_mfxBS, par.mfx.BufferSizeInKB * 1000);

	return sts;
}

mfxStatus CEncodingPipeline::ProceedFrame(mfxFrameSurface1* pSurface, mfxSyncPoint& EncSyncP)
{
	MSDK_CHECK_POINTER(m_pmfxENC, MFX_ERR_NOT_INITIALIZED);

	mfxStatus sts = MFX_ERR_NONE;

	sts = MFX_ERR_NONE;
	int guardCount = 0;

	// at this point surface for encoder contains either a frame from file or a frame processed by vpp
	EncSyncP = 0;
	do
	{
		sts = m_pmfxENC->EncodeFrameAsync(&m_encCtrl, pSurface, &m_mfxBS, &EncSyncP);

		if ((!EncSyncP) && (MFX_ERR_NONE != sts))
		{
			// repeat the call if warning and no output
			switch (sts) {
			case MFX_WRN_DEVICE_BUSY:
				Sleep(1); // wait if device is busy
				break;

			case MFX_ERR_NOT_ENOUGH_BUFFER:
				sts = AllocEncodingPipelineOutputBitream();
				MSDK_CHECK_STATUS(sts, "AlloCEncodingPipelineOutputBitream failed");
				break;

			case MFX_ERR_MORE_DATA:
				// MFX_ERR_MORE_DATA means either that we're just started( if we actually have data in bufferSrc) 
				// or that we've completed encoding and flushed buffers (is bufferSrc is NULL)
				sts = pSurface ? MFX_ERR_NONE : sts;
				break;
			}
		}
	} while (sts != MFX_ERR_NONE && guardCount++<10);

	nFramesProcessed++;

	return sts;
}