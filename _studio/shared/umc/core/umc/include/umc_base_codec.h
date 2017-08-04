// Copyright (c) 2017 Intel Corporation
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef __UMC_BASE_CODEC_H__
#define __UMC_BASE_CODEC_H__

#include "umc_media_data.h"
#include "umc_memory_allocator.h"

namespace UMC
{

class BaseCodecParams
{
    DYNAMIC_CAST_DECL_BASE(BaseCodecParams)

public:
    // Default constructor
    BaseCodecParams(void);

    // Destructor
    virtual ~BaseCodecParams(void){}

    MediaData *m_pData;
    MemoryAllocator *lpMemoryAllocator; // (MemoryAllocator *) pointer to memory allocator object

    uint32_t m_SuggestedInputSize;   //max suggested frame size of input stream
    uint32_t m_SuggestedOutputSize;  //max suggested frame size of output stream

    int32_t             numThreads; // maximum number of threads to use

    int32_t  profile; // profile
    int32_t  level;  // level
};

class BaseCodec
{
    DYNAMIC_CAST_DECL_BASE(BaseCodec)

public:
    // Constructor
    BaseCodec(void);

    // Destructor
    virtual ~BaseCodec(void);

    // Initialize codec with specified parameter(s)
    // Has to be called if MemoryAllocator interface is used
    virtual Status Init(BaseCodecParams *init);

    // Compress (decompress) next frame
    virtual Status GetFrame(MediaData *in, MediaData *out) = 0;

    // Get codec working (initialization) parameter(s)
    virtual Status GetInfo(BaseCodecParams *info) = 0;

    // Close all codec resources
    virtual Status Close(void);

    // Set codec to initial state
    virtual Status Reset(void) = 0;

protected:
    MemoryAllocator *m_pMemoryAllocator; // (MemoryAllocator*) pointer to memory allocator
    bool             m_bOwnAllocator;    // True when default allocator is used
};

} // end namespace UMC

#endif /* __UMC_BASE_CODEC_H__ */
