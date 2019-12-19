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
#include "EncodingPipeline.h"
#include "mfxvideo++.h"
#include "SimpleYUVReader.h"
#include <vector>

#define CREATE_PROPERTY(type, name, initVal) public: property type name {\
	   type get() { return prop##name; }\
	   void set(type value){prop##name = value;}};\
	   private: type prop##name = initVal;

namespace MSDKInterop
{
	public enum class CodecTypes
	{
		AVC=0,
		HEVC
	};

	public enum class BRCTypes
	{
		CBR=1,
		VBR=2,
		QVBR=14,
		LookAhead=8,
		CQP=3
	};

	public enum class IntraRefreshTypes
	{
		No=0,
		Vertical,
		Horizontal,
		Slice
	};

	public enum class SupportedFOURCC
	{
		None =0,
		NV12 = MFX_FOURCC_NV12,
		RGB4 = MFX_FOURCC_RGB4,
		P010 = MFX_FOURCC_P010,
		I420 = MFX_FOURCC_I420
	};

	ref class EncoderInterop;

	public delegate void EncodeProgressHandler(EncoderInterop^ sender, double progress);
	public delegate void EncodeCompletedHandler(EncoderInterop^ sender, AsyncStatus status);

	public ref class EncoderInterop sealed
	{
	public:
		
		static EncoderInterop^ Create(Windows::Storage::StorageFile^ source, Windows::Storage::StorageFile^ sink);
		bool StartEncoding();
		void Stop();
		event EncodeProgressHandler ^ encodeProgress; 
		event EncodeCompletedHandler ^ encodeComplete; 

		String^ GetLastError() { return lastError; }

		static bool IsMP4Supported();
		 
	private: 
		mfxVideoParam LoadParams();
		mfxU32 CalculateBitrate(int frameSize);
		mfxU16 FourCCToChroma(mfxU32 fourCC);
		mfxU32 FourCCToBPP(mfxU32 fourCC);
		mfxStatus ConvertFrameRate(mfxF64 dFrameRate, mfxU32 * pnFrameRateExtN, mfxU32 * pnFrameRateExtD);

		mfxVideoParam m_VideoParams; 
		std::shared_ptr<CEncodingPipeline> encodingPipeline;

		Windows::Storage::StorageFile^ m_StorageSource; 
		Windows::Storage::StorageFile^ m_StorageSink; 

		Windows::UI::Core::CoreDispatcher^ m_dispatcher;

		mfxExtCodingOption2 codingOption2;
		mfxExtCodingOption3 codingOption3;

		std::vector<mfxExtBuffer*> extBuffers;

		String^ lastError="";

		//--- Encoding parameters ----------------
		CREATE_PROPERTY(int, Width, 0);
		CREATE_PROPERTY(int, Height, 0);
		CREATE_PROPERTY(SupportedFOURCC, SrcFourCC, SupportedFOURCC::None);

		CREATE_PROPERTY(CodecTypes, Codec, CodecTypes::AVC);
		CREATE_PROPERTY(double, Framerate, 30);

		CREATE_PROPERTY(unsigned short, TargetUsage, 4);
		CREATE_PROPERTY(BRCTypes, BRC, BRCTypes::VBR);
		CREATE_PROPERTY(unsigned short, MaxKbps, 0);
		CREATE_PROPERTY(unsigned short, Quality, 37);
		CREATE_PROPERTY(unsigned short, QPI, 0);
		CREATE_PROPERTY(unsigned short, QPP, 0);
		CREATE_PROPERTY(unsigned short, QPB, 0);
		CREATE_PROPERTY(unsigned short, LAD, 0);
		CREATE_PROPERTY(unsigned short, Bitrate, 0);
		CREATE_PROPERTY(bool, LowDelayBRC, false);
		CREATE_PROPERTY(unsigned int, MaxFrameSize, 0);
		CREATE_PROPERTY(unsigned short, GOPRefDist, 0);
		CREATE_PROPERTY(unsigned short, WinBRCSize, 0);
		CREATE_PROPERTY(unsigned short, WinBRCMaxAvgKbps, 0);
		CREATE_PROPERTY(unsigned short, NumSlice, 0);
		CREATE_PROPERTY(unsigned int, MaxSliceSize, 0);
		CREATE_PROPERTY(IntraRefreshTypes, IntraRefreshType, IntraRefreshTypes::No);
		CREATE_PROPERTY(unsigned short, IRDist, 0);
		CREATE_PROPERTY(unsigned short, IRCycle, 2);

	};


};