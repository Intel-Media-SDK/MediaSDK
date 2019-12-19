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

#ifdef USE_EXTRAS
#include "FFMPEGBitstreamWriter.h"
#include "shcore.h"

using namespace concurrency;
using namespace Platform;

FFMPEGBitstreamWriter::FFMPEGBitstreamWriter()
{
	if (!m_bIsRegistered)
	{
		av_register_all();
		m_bIsRegistered = true;
	}
}


FFMPEGBitstreamWriter::~FFMPEGBitstreamWriter()
{
	Close();
}

mfxStatus FFMPEGBitstreamWriter::Init(Windows::Storage::Streams::IRandomAccessStream^ outStream, mfxVideoParam Params)
{
	mfxStatus sts = MFX_ERR_NONE;
	m_pVideoStream = NULL;
	HRESULT hr = S_OK;
	int ret;

	if (Params.mfx.CodecId == MFX_CODEC_AVC)
	{
		m_pFmt = av_guess_format("mp4", NULL, NULL);

		if (!m_pFmt)
		{
			OutputDebugString(L"Cannot intialize muxer for format mp4");
			return MFX_ERR_NOT_INITIALIZED;
		}

		m_pFmt->video_codec = AV_CODEC_ID_H264;


		avformat_alloc_output_context2(&m_pCtx, av_guess_format("mp4", NULL, NULL), NULL, NULL);

		if (!m_pCtx)
		{
			sts = MFX_ERR_NOT_INITIALIZED;
			return sts;
		}

		m_pFmt = m_pCtx->oformat;
		if (m_pFmt->video_codec != AV_CODEC_ID_NONE) {
			m_pVideoStream = avformat_new_stream(m_pCtx, NULL);
			if (!m_pVideoStream)
			{
				sts = MFX_ERR_NOT_INITIALIZED;
				return sts;
			}
			AVCodecParameters *c = m_pVideoStream->codecpar;
			c->codec_id = m_pFmt->video_codec;  // AV_CODEC_ID_H264;
			c->codec_type = AVMEDIA_TYPE_VIDEO;
			c->bit_rate = Params.mfx.TargetKbps * 1000;
			c->width = Params.mfx.FrameInfo.Width;
			c->height = Params.mfx.FrameInfo.Height;

			m_nFrameRateD = Params.mfx.FrameInfo.FrameRateExtD;
			m_nFrameRateN = Params.mfx.FrameInfo.FrameRateExtN;

			fileStreamBuffer = (unsigned char*)av_malloc(FILESTREAMBUFFERSZ);
			if (fileStreamBuffer == nullptr)
			{
				hr = E_OUTOFMEMORY;
			}
		}

		//Set up the custom IO
		AVIOContext* pAvioCtx = avio_alloc_context(fileStreamBuffer, FILESTREAMBUFFERSZ,
			1,
			reinterpret_cast<void *>(outStream),
			NULL,
			WritePacket,
			FileStreamSeek
		);
		if (pAvioCtx == nullptr)
		{
			hr = E_OUTOFMEMORY;
		}
		m_pCtx->pb = pAvioCtx;
		m_pCtx->flags |= AVFMT_FLAG_CUSTOM_IO;

		ret = avformat_write_header(m_pCtx, NULL);

		m_nProcessedFramesNum = 0;
	}

	return MFX_ERR_NONE;
}

mfxStatus FFMPEGBitstreamWriter::WriteNextFrame(mfxBitstream * pBS)
{
	mfxStatus sts = MFX_ERR_NONE;

	AVStream* videostream = m_pCtx->streams[0];
	AVPacket pkt = { 0 };
	av_init_packet(&pkt);

	++m_nProcessedFramesNum;
	pkt.dts = m_nProcessedFramesNum;

	AVRational tb = { m_nFrameRateD, m_nFrameRateN };

	pkt.dts = av_rescale_q(pkt.dts, tb, videostream->time_base);
	pkt.pts = pkt.dts;
	pkt.stream_index = m_pVideoStream->index;
	pkt.data = pBS->Data + pBS->DataOffset;
	pkt.size = pBS->DataLength;


	if (pBS->FrameType == (MFX_FRAMETYPE_I | MFX_FRAMETYPE_REF | MFX_FRAMETYPE_IDR))
		pkt.flags |= AV_PKT_FLAG_KEY;

	if (av_interleaved_write_frame(m_pCtx, &pkt)) {
		return MFX_ERR_UNKNOWN;
	}


	return sts;
}

mfxStatus FFMPEGBitstreamWriter::Close()
{
	int ret = av_write_trailer(m_pCtx);

	if (m_pVideoStream)
	{
		av_free(m_pVideoStream);
	}

	if (m_pFmt)
	{
		av_free(m_pFmt);
	}

	if (m_pCtx)
	{
		av_free(m_pCtx);
	}
	return ret ? MFX_ERR_UNKNOWN : MFX_ERR_NONE;
}

int FFMPEGBitstreamWriter::WritePacket(void *opaque, uint8_t *buf, int buf_size)
{
	Windows::Storage::Streams::IRandomAccessStream^ outStream = reinterpret_cast<Windows::Storage::Streams::IRandomAccessStream^>(opaque);

	IOutputStream^ outstream = outStream->GetOutputStreamAt(outStream->Position);

	DataWriter^ dw = ref new DataWriter(outstream);

	dw->WriteBytes(ref new Platform::Array<byte>(buf, buf_size));

	auto task_write = create_task(dw->StoreAsync());
	task_write.wait();

	auto task_flush = create_task(dw->FlushAsync());
	task_flush.wait();

	// WriteBytes does not affect Position of the stream, so we have to set it manually
	outStream->Seek(outStream->Position + (long)buf_size);

	return buf_size;

}


// Static function to seek in file stream. Credit to Philipp Sch http://www.codeproject.com/Tips/489450/Creating-Custom-FFmpeg-IO-Context
int64_t FFMPEGBitstreamWriter::FileStreamSeek(void* ptr, int64_t pos, int whence)
{
	Windows::Storage::Streams::IRandomAccessStream^ outStream = reinterpret_cast<Windows::Storage::Streams::IRandomAccessStream^>(ptr);
	outStream->Seek(pos);
	return pos; // Return the new position:
}
#endif