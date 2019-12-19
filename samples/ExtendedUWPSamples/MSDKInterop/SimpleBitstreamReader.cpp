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
#include "SimpleBitstreamReader.h"
#include "sample_defs.h"

using namespace concurrency; 
using namespace Platform;
using namespace Windows::Storage; 
using namespace Windows::Storage::Streams; 
 
 


#define INITIAL_BITSTREAM_SIZE 10000

//template <typename T>
//auto ExecSync(Windows::Foundation::IAsyncOperation<T^>^ asyncFunc)
//{
//    auto task = concurrency::create_task(asyncFunc);
//    task.wait();
//    return task.get();
//}

CSimpleBitstreamReader::CSimpleBitstreamReader()
{
    m_bInited = false;
    fileSize = 0;
    bytesCompleted = 0;
	stream = nullptr;
	dataReader = nullptr;
	MSDK_ZERO_MEMORY(BitStream);
}

CSimpleBitstreamReader::~CSimpleBitstreamReader()
{
    Close();
}

void CSimpleBitstreamReader::Close()
{
    stream = nullptr;
    dataReader = nullptr;

    m_bInited = false;
    fileSize = 0;
    bytesCompleted = 0;
    delete[] BitStream.Data;
    MSDK_ZERO_MEMORY(BitStream);
}

void CSimpleBitstreamReader::Reset()
{
    if (!m_bInited)
        return;

    stream->Seek(0);

    bytesCompleted = 0;
    
}

mfxStatus CSimpleBitstreamReader::Init(Windows::Storage::StorageFile^ fileSource)
{
	try
	{
		if (fileSource && !fileSource->IsAvailable)
		{
			return MFX_ERR_NULL_PTR;
		}

		Close();

		auto task = concurrency::create_task(fileSource->OpenReadAsync());
		task.wait();
        
		stream = task.get();
		fileSize = stream->Size;
		m_bInited = true;

		dataReader = ref new Windows::Storage::Streams::DataReader(stream);
		m_bInited = true;


		// Initializing bitstream
		MSDK_ZERO_MEMORY(BitStream);
		BitStream.Data = new(std::nothrow) unsigned char[INITIAL_BITSTREAM_SIZE];
		MSDK_CHECK_POINTER_SAFE(BitStream.Data, MFX_ERR_NULL_PTR, Close());
		BitStream.MaxLength = INITIAL_BITSTREAM_SIZE;

		return MFX_ERR_NONE;
	}
	catch (...)
	{
		return MFX_ERR_NULL_PTR;
	}
}

mfxStatus CSimpleBitstreamReader::ReadNextFrame()
{
    if (!m_bInited)
        return MFX_ERR_NOT_INITIALIZED;

    try
    {
        mfxU32 nBytesRead = 0;

        bytesCompleted += BitStream.DataOffset;
        memmove(BitStream.Data, BitStream.Data + BitStream.DataOffset, BitStream.DataLength);
        BitStream.DataOffset = 0;

        nBytesRead = concurrency::create_task(dataReader->LoadAsync(BitStream.MaxLength - BitStream.DataLength)).get();

        //nBytesRead = (mfxU32)fread(BitStream.Data + BitStream.DataLength, 1, BitStream.MaxLength - BitStream.DataLength, m_fSource);

        if (0 == nBytesRead)
        {
            return MFX_ERR_MORE_DATA;
        }


        Platform::ArrayReference<unsigned char> arrayWrapper((unsigned char*)BitStream.Data + BitStream.DataLength, nBytesRead);
        dataReader->ReadBytes(arrayWrapper);

        BitStream.DataLength += nBytesRead;
    }
    catch (Platform::Exception^ e)
    {
 		return MFX_ERR_UNKNOWN;
    }

    return MFX_ERR_NONE;
}

mfxStatus CSimpleBitstreamReader::ExpandBitstream(unsigned int extraSize)
{
    unsigned char* oldPtr = BitStream.Data;
    BitStream.Data = new(std::nothrow) unsigned char[BitStream.MaxLength +extraSize];
    MSDK_CHECK_POINTER_SAFE(BitStream.Data, MFX_ERR_NULL_PTR, Close());
    MSDK_MEMCPY(BitStream.Data, oldPtr, BitStream.MaxLength);
    BitStream.MaxLength += extraSize;
    return MFX_ERR_NONE;
}
