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
#include "EncoderInterop.h"


using namespace MSDKInterop;

EncoderInterop^ EncoderInterop::Create(Windows::Storage::StorageFile^ source, Windows::Storage::StorageFile^ sink)
{
	if (!source || !sink)
	{
		return nullptr;
	}

	auto msdkEncoder = ref new EncoderInterop();
	
	msdkEncoder->m_StorageSource = source; 
	msdkEncoder->m_StorageSink = sink; 
	
	return msdkEncoder; 
}

mfxU32 EncoderInterop::CalculateBitrate(int frameSize)
{
	return frameSize < 414720 ? frameSize * 25 / 2592 : (frameSize - 414720) * 25 / 41088 + 4000;
}

mfxVideoParam EncoderInterop::LoadParams()
{
	mfxVideoParam Params = { 0 };

	if (!Width || !Height || SrcFourCC == SupportedFOURCC::None || !Framerate)
	{
		return Params;
	}

	Params.mfx.CodecId = Codec==CodecTypes::AVC ? MFX_CODEC_AVC : MFX_CODEC_HEVC;
	Params.mfx.RateControlMethod = (int)BRC;
	Params.IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY;

	Params.mfx.TargetUsage = TargetUsage;
	Params.mfx.GopRefDist = GOPRefDist;
	Params.mfx.NumSlice = NumSlice;

	Params.mfx.FrameInfo.CropX = 0;
	Params.mfx.FrameInfo.CropY = 0;
	Params.mfx.FrameInfo.CropW = Width;
	Params.mfx.FrameInfo.CropH = Height;
	Params.mfx.FrameInfo.Width = Params.mfx.FrameInfo.CropW;
	Params.mfx.FrameInfo.Height = Params.mfx.FrameInfo.CropH;

	Params.AsyncDepth = 1;

	Params.mfx.FrameInfo.FourCC = SrcFourCC == SupportedFOURCC::I420 ? MFX_FOURCC_NV12 : (unsigned int)SrcFourCC;
	Params.mfx.FrameInfo.ChromaFormat = FourCCToChroma(Params.mfx.FrameInfo.FourCC);

	if (BRC != BRCTypes::CQP)
	{
		Params.mfx.TargetKbps = Bitrate ? Bitrate : CalculateBitrate(Params.mfx.FrameInfo.Width*Params.mfx.FrameInfo.Height);
		Params.mfx.MaxKbps = BRC != BRCTypes::CBR ? MaxKbps : 0;
	}
	else
	{
		Params.mfx.QPI = QPI;
		Params.mfx.QPB = QPB;
		Params.mfx.QPP = QPP;
	}	

	ConvertFrameRate(Framerate, &Params.mfx.FrameInfo.FrameRateExtN, &Params.mfx.FrameInfo.FrameRateExtD);

	Params.mfx.FrameInfo.Shift = Params.mfx.FrameInfo.FourCC == MFX_FOURCC_P010;
	Params.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;

	//--- Ext coding options -----------------------------------------------------------------
	memset(&codingOption2, 0, sizeof(codingOption2));
	memset(&codingOption3, 0, sizeof(codingOption3));

	codingOption2.Header.BufferId = MFX_EXTBUFF_CODING_OPTION2;
	codingOption2.Header.BufferSz = sizeof(codingOption2);
	codingOption3.Header.BufferId = MFX_EXTBUFF_CODING_OPTION3;
	codingOption3.Header.BufferSz = sizeof(codingOption3);

	codingOption2.LookAheadDepth = LAD;
	codingOption2.MaxFrameSize =MaxFrameSize;
	codingOption2.MaxSliceSize = MaxSliceSize;
	codingOption2.IntRefType = (int)IntraRefreshType;
	codingOption2.IntRefCycleSize = IRCycle;

	codingOption3.QVBRQuality = Quality;
	if (Codec == CodecTypes::AVC)
	{
		// In case of HEVC this parameter should be left as default
		codingOption3.LowDelayBRC = LowDelayBRC ? MFX_CODINGOPTION_ON : MFX_CODINGOPTION_OFF;
	}
	codingOption3.WinBRCSize = WinBRCSize;
	codingOption3.WinBRCMaxAvgKbps = WinBRCMaxAvgKbps;
	codingOption3.IntRefCycleDist = IRDist;

	extBuffers.clear();
	extBuffers.push_back((mfxExtBuffer*)&codingOption2);
	extBuffers.push_back((mfxExtBuffer*)&codingOption3);
	Params.ExtParam = extBuffers.data();
	Params.NumExtParam = (mfxU16)extBuffers.size();

	return Params;
}

bool EncoderInterop::StartEncoding()
{
	lastError = "";

#ifndef USE_EXTRAS
	if (m_StorageSink->FileType == ".mp4" || m_StorageSink->FileType == ".MP4")
	{
		lastError = "MP4 container is not supported in this version of interop (possibly extras are disabled)";
		return false;
	}
#endif


	try
	{
		mfxVideoParam Params = LoadParams();

		if (!Params.mfx.CodecId)
		{
			return false;
		}

		encodingPipeline = CEncodingPipeline::Instantiate(Params);

		IAsyncActionWithProgress<double>^ encodeAction;
		encodeAction = encodingPipeline->ReadAndEncodeAsync(m_StorageSource, m_StorageSink, (unsigned int)SrcFourCC);

		encodeAction->Progress = ref new AsyncActionProgressHandler<double>(
			[this](IAsyncActionWithProgress<double>^ asyncInfo, double progressInfo)
		{

			this->encodeProgress(this, progressInfo);
		});
		encodeAction->Completed = ref new AsyncActionWithProgressCompletedHandler<double>(
			[this](IAsyncActionWithProgress<double>^ asyncInfo, AsyncStatus status)
		{
			this->encodeComplete(this, status);
		});
	}
	catch(std::exception e)
	{ 
		std::string str(e.what());
		lastError = ref new String(std::wstring(str.begin(), str.end()).c_str());
		return false;
	}

	return true;
}

void EncoderInterop::Stop()
{
	if (encodingPipeline)
	{
		encodingPipeline->CancelEncoding();
	}
}

mfxU16 EncoderInterop::FourCCToChroma(mfxU32 fourCC)
{
	switch (fourCC)
	{
	case MFX_FOURCC_NV12:
	case MFX_FOURCC_P010:
		return MFX_CHROMAFORMAT_YUV420;
	case MFX_FOURCC_NV16:
	case MFX_FOURCC_P210:
	case MFX_FOURCC_YUY2:
		return MFX_CHROMAFORMAT_YUV422;
	case MFX_FOURCC_RGB4:
		return MFX_CHROMAFORMAT_YUV444;
	}

	return MFX_CHROMAFORMAT_YUV420;
}


mfxU32 EncoderInterop::FourCCToBPP(mfxU32 fourCC)
{
	switch (fourCC)
	{
	case MFX_FOURCC_NV12:
	case MFX_FOURCC_P010:
		return 12;
	case MFX_FOURCC_NV16:
	case MFX_FOURCC_P210:
	case MFX_FOURCC_YUY2:
		return 16;
	case MFX_FOURCC_RGB4:
		return 32;
	}

	return 0;
}

mfxStatus EncoderInterop::ConvertFrameRate(mfxF64 dFrameRate, mfxU32* pnFrameRateExtN, mfxU32* pnFrameRateExtD)
{
	MSDK_CHECK_POINTER(pnFrameRateExtN, MFX_ERR_NULL_PTR);
	MSDK_CHECK_POINTER(pnFrameRateExtD, MFX_ERR_NULL_PTR);

	mfxU32 fr;

	fr = (mfxU32)(dFrameRate + .5);

	if (fabs(fr - dFrameRate) < 0.0001)
	{
		*pnFrameRateExtN = fr;
		*pnFrameRateExtD = 1;
		return MFX_ERR_NONE;
	}

	fr = (mfxU32)(dFrameRate * 1.001 + .5);

	if (fabs(fr * 1000 - dFrameRate * 1001) < 10)
	{
		*pnFrameRateExtN = fr * 1000;
		*pnFrameRateExtD = 1001;
		return MFX_ERR_NONE;
	}

	*pnFrameRateExtN = (mfxU32)(dFrameRate * 10000 + .5);
	*pnFrameRateExtD = 10000;

	return MFX_ERR_NONE;
}

bool EncoderInterop::IsMP4Supported()
{
#ifdef USE_EXTRAS
	return true;
#else
	return false;
#endif
}
