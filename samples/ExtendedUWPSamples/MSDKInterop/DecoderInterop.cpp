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
#include "DecoderInterop.h"
#include "DecodingPipeline.h"

#define SAMPLE_WAIT_TIME_MS 5000

using namespace MSDKInterop;
using namespace Windows::Media::Core;
using namespace Windows::Media::MediaProperties;

DecoderInterop ^ DecoderInterop::CreatefromFile(Windows::Storage::StorageFile^ file)
{
    try
    {
        return ref new DecoderInterop(file);
    }
    catch (std::exception e)
    {
        std::string err(e.what());
        err += "\r\n";
        DebugMessage(std::wstring(err.begin(), err.end()).c_str());
        return nullptr;
    }
}

MediaStreamSource ^ DecoderInterop::GetMediaStreamSource()
{
    return mss;
}

DecoderInterop::~DecoderInterop()
{
	Close();
}

void DecoderInterop::Close()
{
	mss->Starting -= startingRequestedToken;
	mss->SampleRequested -= sampleRequestedToken;

	pipeline->Stop();
}


DecoderInterop::DecoderInterop(Windows::Storage::StorageFile^ file)
{
    pipeline.reset(new CDecodingPipeline);

    //--- Geting codec type from file extension -----------------------
    if (file->FileType == ".264" || file->FileType == ".h264")
    {
        pipeline->SetCodecID(MFX_CODEC_AVC);
    }
    else if (file->FileType == ".m2v" || file->FileType == ".mpg" || file->FileType == ".bs" || file->FileType == ".es")
    {
        pipeline->SetCodecID(MFX_CODEC_MPEG2);
    }
    else if (file->FileType == ".265" || file->FileType == ".h265" || file->FileType == ".hevc")
    {
        pipeline->SetCodecID(MFX_CODEC_HEVC);
    }
    else
    {
        throw std::exception("Invalid type of file selected");
    }

    pipeline->Load(file);

    if (!pipeline->Start())
    {
        throw std::exception("Cannot start decoding pipeline");
    }

    // Wait for pipeline to be started and initialized
    while (!pipeline->IsInitialized() && pipeline->IsRunning()) { MSDK_SLEEP(10); }

    if (pipeline->IsRunning())
    {
        videoStreamDescriptor = ConvertToVideoStreamDescriptor(pipeline->GetDecoderParams());

        if (videoStreamDescriptor == nullptr)
        {
            mss = nullptr;
            throw std::exception("Cannot create video descriptor");
        }

        mss = ref new MediaStreamSource(videoStreamDescriptor);
        if (mss)
        {
            //mss->Duration = mediaDuration;
            mss->CanSeek = false;
            mss->BufferTime = { 0 };

            startingRequestedToken = mss->Starting += ref new TypedEventHandler<MediaStreamSource ^, MediaStreamSourceStartingEventArgs ^>(this, &DecoderInterop::OnStarting);
            sampleRequestedToken = mss->SampleRequested += ref new TypedEventHandler<MediaStreamSource ^, MediaStreamSourceSampleRequestedEventArgs ^>(this, &DecoderInterop::OnSampleRequested);
        }
        else
        {
            throw std::exception("Cannot create MediaStreamSource");
        }
        return;
    }

    throw std::exception("Cannot initialize decoding pipeline");
}


void DecoderInterop::OnStarting(MediaStreamSource ^ sender, MediaStreamSourceStartingEventArgs ^ args)
{
    TimeSpan startPos = { 0 };
    args->Request->SetActualStartPosition(startPos);
	if (!pipeline->Start())
	{
		throw std::exception("Cannot start decoding pipeline");
	}
	while (!pipeline->IsInitialized() && pipeline->IsRunning()) { MSDK_SLEEP(10); }
}

void MSDKInterop::DecoderInterop::OnSampleRequested(MediaStreamSource ^ sender, MediaStreamSourceSampleRequestedEventArgs ^ args)
{
    mutexGuard.lock();


    if (mss != nullptr)
    {
        if (args->Request->StreamDescriptor == videoStreamDescriptor)
        {
            args->Request->Sample = GetNextSample();
        }
        else
        {
            args->Request->Sample = nullptr;
        }
    }
    mutexGuard.unlock();
}

MediaStreamSample ^ DecoderInterop::GetNextSample()
{
    MediaStreamSample^ sample;
    CMfxFrameSurfaceExt* pSurf=nullptr;
    LONGLONG ulTimeSpan = 0;
    LONGLONG pts = 0; //crude pts
    DataWriter^ dataWriter = ref new DataWriter();

	auto startTime = std::chrono::system_clock::now();
    
	mfxStatus sts = MFX_ERR_NONE;

	while ((sts = pipeline->GetSurfaceFromQueue(pSurf)) == MFX_ERR_NOT_FOUND && (std::chrono::system_clock::now() - startTime).count() < SAMPLE_WAIT_TIME_MS*1e4) // system clock counts 0.1us units
	{
		Sleep(0);
	};

    if (pSurf && sts==MFX_ERR_NONE)
    {
        sts = pSurf->pAlloc->Lock(pSurf->pAlloc->pthis, pSurf->Data.MemId, &(pSurf->Data));
        if (sts == MFX_ERR_NONE)
        {
			for (int i = 0; i < pSurf->Info.Height; i++)
			{
				auto YBuffer = ref new Platform::Array<uint8_t>(pSurf->Data.Y+pSurf->Data.Pitch *i,pSurf->Info.Width);
				dataWriter->WriteBytes(YBuffer);
			}
			for (int i = 0; i < pSurf->Info.Height / 2; i++)
			{
				auto UVBuffer = ref new Platform::Array<uint8_t>(pSurf->Data.UV + pSurf->Data.Pitch * i, pSurf->Info.Width );
				dataWriter->WriteBytes(UVBuffer);
			}
            IBuffer^ buf = dataWriter->DetachBuffer();
            UINT32 ui32Numerator = pSurf->Info.FrameRateExtN;
            UINT32 ui32Denominator = pSurf->Info.FrameRateExtD;
			ulTimeSpan = ui32Numerator != 0 ? ((ULONGLONG)ui32Denominator) * 10000000 / ui32Numerator : 1000000 / 3; // In case of malformed data using 30 FPS

            pts = pSurf->Data.FrameOrder * ulTimeSpan;
            sample = MediaStreamSample::CreateFromBuffer(buf, { pts });
            sample->Duration = {ulTimeSpan};
            sample->Discontinuous = false;

            sts = pSurf->pAlloc->Unlock(pSurf->pAlloc->pthis, pSurf->Data.MemId, &(pSurf->Data));
            pSurf->UserLock = false;
        }
    }
    else
    {
        sample = nullptr;
    }
    return sample;
}

VideoStreamDescriptor^ DecoderInterop::ConvertToVideoStreamDescriptor(mfxVideoParam params)
{
    VideoEncodingProperties^ videoProperties = VideoEncodingProperties::CreateUncompressed(MediaEncodingSubtypes::Nv12,
        params.mfx.FrameInfo.Width, params.mfx.FrameInfo.Height);

	if (params.mfx.FrameInfo.FrameRateExtD && params.mfx.FrameInfo.FrameRateExtN)
	{
		videoProperties->FrameRate->Denominator = params.mfx.FrameInfo.FrameRateExtD;
		videoProperties->FrameRate->Numerator = params.mfx.FrameInfo.FrameRateExtN;
	}
	else
	{
		//--- Defualt framerate  is 30 FPS
		videoProperties->FrameRate->Denominator = 30;
		videoProperties->FrameRate->Numerator = 1;
	}

	videoProperties->PixelAspectRatio->Denominator = params.mfx.FrameInfo.AspectRatioH;
    videoProperties->PixelAspectRatio->Numerator = params.mfx.FrameInfo.AspectRatioW;
//    videoProperties->Bitrate = params.mfx.TargetKbps;
    return  ref new VideoStreamDescriptor(videoProperties);
}
