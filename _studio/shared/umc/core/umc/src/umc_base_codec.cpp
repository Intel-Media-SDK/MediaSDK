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

#include "umc_base_codec.h"

using namespace UMC;

// Default constructor
BaseCodecParams::BaseCodecParams(void)
{
    m_SuggestedOutputSize = 0;
    m_SuggestedInputSize = 0;
    lpMemoryAllocator = 0;
    numThreads = 0;

    m_pData=NULL;

    profile = 0;
    level = 0;
}

// Constructor
BaseCodec::BaseCodec(void)
{
    m_pMemoryAllocator = 0;
    m_bOwnAllocator = false;
}

// Destructor
BaseCodec::~BaseCodec(void)
{
  BaseCodec::Close();
}

// Initialize codec with specified parameter(s)
// Has to be called if MemoryAllocator interface is used
Status BaseCodec::Init(BaseCodecParams *init)
{
  if (init == 0)
    return UMC_ERR_NULL_PTR;
  m_pMemoryAllocator = init->lpMemoryAllocator;
  return UMC_OK;
}

// Close all codec resources
Status BaseCodec::Close(void)
{
    return UMC_OK;
}

