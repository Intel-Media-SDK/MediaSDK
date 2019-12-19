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

#ifdef USE_EXTRAS
#include "mfxvideo.h"
#include <atomic>
#include <pplawait.h>
#include "StreamWriter.h"


extern "C"
{
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavcodec/avcodec.h>
}


const int FILESTREAMBUFFERSZ = 16384;
using namespace Windows::Storage::Streams; 
using namespace Concurrency; 

class FFMPEGBitstreamWriter : public CStreamWriter
{
public:
	FFMPEGBitstreamWriter();
	virtual ~FFMPEGBitstreamWriter();

	virtual mfxStatus Init(Windows::Storage::Streams::IRandomAccessStream^ outStream, mfxVideoParam Params) override;
	virtual mfxStatus WriteNextFrame(mfxBitstream* pBS) override;
	virtual mfxStatus Close() override;

protected: 
	Concurrency::cancellation_token_source _CTS;

	mfxU32 m_nProcessedFramesNum; 
	bool m_bInited; 
	bool m_bIsRegistered; 

	AVOutputFormat*    m_pFmt=nullptr; 
	AVFormatContext*   m_pCtx=nullptr; 
	mfxI32			   m_nFrameRateD=0;
	mfxI32			   m_nFrameRateN=0;

	AVStream*		   m_pVideoStream=nullptr;

	unsigned char* fileStreamBuffer; 
	IStream* fileStreamData;
	DataWriter^ dataWriter; 

	static int WritePacket(void *opaque, uint8_t *buf, int buf_size);
	static int64_t FileStreamSeek(void* ptr, int64_t pos, int whence);
};

#endif