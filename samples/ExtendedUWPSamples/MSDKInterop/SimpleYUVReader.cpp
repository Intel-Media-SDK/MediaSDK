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
#include "SimpleYUVReader.h"

bool CSimpleYUVReader::Open(IRandomAccessStream^ stream, mfxU32 colorFormat)
{
	bytesRead = 0;

	if (MFX_FOURCC_NV12 != colorFormat &&
		MFX_FOURCC_YV12 != colorFormat &&
		MFX_FOURCC_I420 != colorFormat &&
		MFX_FOURCC_YUY2 != colorFormat &&
		MFX_FOURCC_RGB4 != colorFormat &&
		MFX_FOURCC_BGR4 != colorFormat &&
		MFX_FOURCC_P010 != colorFormat &&
		MFX_FOURCC_P210 != colorFormat)
	{
		return false;
	}

	this->colorFormat = colorFormat;

	if (MFX_FOURCC_P010 == colorFormat)
	{
		shouldShiftP010High = true;
	}

	dataReader = ref new DataReader(stream);

	colorFormat = MFX_FOURCC_YV12;
	shouldShiftP010High = false;

	return true;
}

CSimpleYUVReader::~CSimpleYUVReader()
{
	Close();
}

void CSimpleYUVReader::Close()
{
	dataReader = nullptr;
}

mfxStatus CSimpleYUVReader::LoadNextFrame(mfxFrameSurface1 * pSurface)
{
	// check if reader is initialized
	MSDK_CHECK_POINTER(pSurface, MFX_ERR_NULL_PTR);

	mfxU16 w, h, i, pitch;
	mfxU8 *ptr, *ptr2;	
	mfxFrameInfo& pInfo = pSurface->Info;
	mfxFrameData& pData = pSurface->Data;

	mfxU32 vid = pInfo.FrameId.ViewId;

	if (pInfo.CropH > 0 && pInfo.CropW > 0)
	{
		w = pInfo.CropW;
		h = pInfo.CropH;
	}
	else
	{
		w = pInfo.Width;
		h = pInfo.Height;
	}

	mfxU32 nBytesPerPixel = (pInfo.FourCC == MFX_FOURCC_P010 || pInfo.FourCC == MFX_FOURCC_P210) ? 2 : 1;

	if (MFX_FOURCC_YUY2 == pInfo.FourCC || MFX_FOURCC_RGB4 == pInfo.FourCC || MFX_FOURCC_BGR4 == pInfo.FourCC)
	{
		//Packed format: Luminance and chrominance are on the same plane
		switch (colorFormat)
		{
		case MFX_FOURCC_RGB4:
		case MFX_FOURCC_BGR4:

			pitch = pData.Pitch;
			ptr = MSDK_MIN(MSDK_MIN(pData.R, pData.G), pData.B);
			ptr = ptr + pInfo.CropX + pInfo.CropY * pData.Pitch;

			for (i = 0; i < h; i++)
			{
				if (!ReadData(ptr + i * pitch, 4 * w))
				{
					return MFX_ERR_MORE_DATA;
				}
			}
			break;
		case MFX_FOURCC_YUY2:
			pitch = pData.Pitch;
			ptr = pData.Y + pInfo.CropX + pInfo.CropY * pData.Pitch;

			for (i = 0; i < h; i++)
			{
				if (!ReadData(ptr + i * pitch, 2 * w))
				{
					return MFX_ERR_MORE_DATA;
				}
			}
			break;
		default:
			return MFX_ERR_UNSUPPORTED;
		}
	}
	else if (MFX_FOURCC_NV12 == pInfo.FourCC || MFX_FOURCC_YV12 == pInfo.FourCC || MFX_FOURCC_P010 == pInfo.FourCC || MFX_FOURCC_P210 == pInfo.FourCC)
	{
		pitch = pData.Pitch;
		ptr = pData.Y + pInfo.CropX + pInfo.CropY * pData.Pitch;

		// read luminance plane
		for (i = 0; i < h; i++)
		{
			if (!ReadData(ptr + i * pitch, nBytesPerPixel *w))
			{
				return MFX_ERR_MORE_DATA;
			}

			// Shifting data if required
			if ((MFX_FOURCC_P010 == pInfo.FourCC || MFX_FOURCC_P210 == pInfo.FourCC) && shouldShiftP010High)
			{
				mfxU16* shortPtr = (mfxU16*)(ptr + i * pitch);
				for (int idx = 0; idx < w; idx++)
				{
					shortPtr[idx] <<= 6;
				}
			}
		}

		// read chroma planes
		switch (colorFormat) // color format of data in the input file
		{
		case MFX_FOURCC_I420:
		case MFX_FOURCC_YV12:
			switch (pInfo.FourCC)
			{
			case MFX_FOURCC_NV12:

				mfxU8 buf[2048]; // maximum supported chroma width for nv12
				mfxU32 j, dstOffset[2];
				w /= 2;
				h /= 2;
				ptr = pData.UV + pInfo.CropX + (pInfo.CropY / 2) * pitch;
				if (w > 2048)
				{
					return MFX_ERR_UNSUPPORTED;
				}

				if (colorFormat == MFX_FOURCC_I420) {
					dstOffset[0] = 0;
					dstOffset[1] = 1;
				}
				else {
					dstOffset[0] = 1;
					dstOffset[1] = 0;
				}

				// load first chroma plane: U (input == I420) or V (input == YV12)
				for (i = 0; i < h; i++)
				{
					if (!ReadData(buf, w))
					{
						return MFX_ERR_MORE_DATA;
					}

					for (j = 0; j < w; j++)
					{
						ptr[i * pitch + j * 2 + dstOffset[0]] = buf[j];
					}
				}

				// load second chroma plane: V (input == I420) or U (input == YV12)
				for (i = 0; i < h; i++)
				{

					if (!ReadData(buf, w))
					{
						return MFX_ERR_MORE_DATA;
					}

					for (j = 0; j < w; j++)
					{
						ptr[i * pitch + j * 2 + dstOffset[1]] = buf[j];
					}
				}

				break;
			case MFX_FOURCC_YV12:
				w /= 2;
				h /= 2;
				pitch /= 2;

				if (colorFormat == MFX_FOURCC_I420) {
					ptr = pData.U + (pInfo.CropX / 2) + (pInfo.CropY / 2) * pitch;
					ptr2 = pData.V + (pInfo.CropX / 2) + (pInfo.CropY / 2) * pitch;
				}
				else {
					ptr = pData.V + (pInfo.CropX / 2) + (pInfo.CropY / 2) * pitch;
					ptr2 = pData.U + (pInfo.CropX / 2) + (pInfo.CropY / 2) * pitch;
				}

				for (i = 0; i < h; i++)
				{
					if (!ReadData(ptr + i * pitch, w))
					{
						return MFX_ERR_MORE_DATA;
					}
				}
				for (i = 0; i < h; i++)
				{
					if (!ReadData(ptr2 + i * pitch, w))
					{
						return MFX_ERR_MORE_DATA;
					}
				}
				break;
			default:
				return MFX_ERR_UNSUPPORTED;
			}
			break;
		case MFX_FOURCC_NV12:
		case MFX_FOURCC_P010:
		case MFX_FOURCC_P210:
			if (MFX_FOURCC_P210 != pInfo.FourCC)
			{
				h /= 2;
			}
			ptr = pData.UV + pInfo.CropX + (pInfo.CropY / 2) * pitch;
			for (i = 0; i < h; i++)
			{
				if (!ReadData(ptr + i * pitch, nBytesPerPixel*w))
				{
					return MFX_ERR_MORE_DATA;
				}

				// Shifting data if required
				if ((MFX_FOURCC_P010 == pInfo.FourCC || MFX_FOURCC_P210 == pInfo.FourCC) && shouldShiftP010High)
				{
					mfxU16* shortPtr = (mfxU16*)(ptr + i * pitch);
					for (int idx = 0; idx < w; idx++)
					{
						shortPtr[idx] <<= 6;
					}
				}
			}

			break;
		default:
			return MFX_ERR_UNSUPPORTED;
		}
	}

	return MFX_ERR_NONE;
}


bool CSimpleYUVReader::ReadData(byte* dst, unsigned int size)
{
	if (concurrency::create_task(dataReader->LoadAsync(size)).get() == size)
	{
		if (dataReader->UnconsumedBufferLength >= size)
		{
			dataReader->ReadBytes(Platform::ArrayReference<byte>(dst, size));
			bytesRead += size;
			return true;
		}
	}
	return false;
}
