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
#include "pch.h"
#include "mfxvideo.h"
#include <atomic>

using namespace concurrency;

class CSimpleBitstreamReader
{
public:
    CSimpleBitstreamReader();
    virtual ~CSimpleBitstreamReader();

    //resets position to file begin
    virtual void      Reset();
    virtual void      Close();
    virtual mfxStatus Init(Windows::Storage::StorageFile^ fileSource);
    virtual mfxStatus ReadNextFrame();
    mfxBitstream BitStream;
    mfxStatus ExpandBitstream(unsigned int extraSize);
    mfxU64 GetFileSize() { return fileSize; }
    mfxU64 GetBytesProcessed() { return bytesCompleted + BitStream.DataOffset; }

protected:
    Windows::Storage::Streams::IRandomAccessStreamWithContentType^ stream;
    Windows::Storage::Streams::DataReader^ dataReader;
    std::atomic<bool>      m_bInited;
    std::atomic<mfxU64> fileSize;
    std::atomic<mfxU64> bytesCompleted;

private:
};

