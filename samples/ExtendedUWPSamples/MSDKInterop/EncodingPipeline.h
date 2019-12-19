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

#ifndef __ENCODING_PIPELINE_H__
#define __ENCODING_PIPELINE_H__

#include "base_allocator.h"

#include "mfxvideo++.h"
#include "StreamWriter.h"
#include "surfacespool.h"

#include <vector>
#include <memory>

using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace concurrency;
using namespace Platform;
using namespace Windows::UI::Popups;
using namespace Windows::Foundation;

class CEncodingPipeline
{
	friend class std::_Ref_count_obj<CEncodingPipeline>;
private:

public:
	CEncodingPipeline();
	static std::shared_ptr<CEncodingPipeline> Instantiate(mfxVideoParam &pParams);
	~CEncodingPipeline();

	mfxStatus ProceedFrame(mfxFrameSurface1* pSurface, mfxSyncPoint& EncSyncP);
	void Close();
	void CancelEncoding();

	IAsyncActionWithProgress<double>^ ReadAndEncodeAsync(StorageFile^ storageSource, StorageFile^ storageSink, mfxU32 sourceFileFourCC);

protected:
	Concurrency::cancellation_token_source _CTS;

	MFXVideoSession m_mfxSession;
	std::unique_ptr<MFXVideoENCODE> m_pmfxENC;

	mfxVideoParam m_mfxEncParams;

	mfxBitstream m_mfxBS;

	std::unique_ptr<MFXFrameAllocator> m_pMFXAllocator;
	std::unique_ptr<mfxAllocatorParams> m_pmfxAllocatorParams;

	CSurfacesPool surfacesPool; // frames array for encoder input (vpp output)

	// external parameters for each component are stored in a vector
	std::vector<mfxExtBuffer*> m_EncExtParams;

	mfxEncodeCtrl m_encCtrl;

	mfxStatus SetEncParams(mfxVideoParam& pParams);

	mfxStatus CreateAllocator();
	void DeleteAllocator();
	void DeleteFrames();

	mfxStatus AllocEncodingPipelineInputFrames();
	mfxStatus AllocEncodingPipelineOutputBitream();

	mfxU32 nFramesProcessed = 0;
	mfxU16 nEncSurfIdx = 0;     // index of free surface for encoder input (vpp output)

	CStreamWriter* pStreamWriter = NULL;
};

#endif // __ENCODING_PIPELINE_H__

