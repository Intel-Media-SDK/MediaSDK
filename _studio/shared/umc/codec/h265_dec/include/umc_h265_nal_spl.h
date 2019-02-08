// Copyright (c) 2017-2019 Intel Corporation
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

#include "umc_defs.h"
#ifdef MFX_ENABLE_H265_VIDEO_DECODE

#ifndef __UMC_H265_NAL_SPL_H
#define __UMC_H265_NAL_SPL_H

#include <vector>
#include "umc_h265_dec_defs.h"
#include "umc_media_data_ex.h"
#include "umc_h265_heap.h"

namespace UMC_HEVC_DECODER
{

// Big endian to little endian converter class
class SwapperBase
{
public:
    virtual ~SwapperBase() {}

    virtual void SwapMemory(uint8_t *pDestination, size_t &nDstSize, uint8_t *pSource, size_t nSrcSize, std::vector<uint32_t> *pRemovedOffsets) = 0;
    virtual void SwapMemory(MemoryPiece * pMemDst, MemoryPiece * pMemSrc, std::vector<uint32_t> *pRemovedOffsets) = 0;
};

// NAL unit start code search class
class StartCodeIteratorBase
{
public:

    StartCodeIteratorBase()
        : m_pSource(0)
        , m_nSourceSize(0)
        , m_pSourceBase(0)
        , m_nSourceBaseSize(0)
        , m_suggestedSize(10 * 1024) // Actual size is calculated in CalculateSuggestedSize
    {
    }

    virtual ~StartCodeIteratorBase()
    {
    }

    virtual int32_t Init(UMC::MediaData * pSource)
    {
        m_pSourceBase = m_pSource = (uint8_t *) pSource->GetDataPointer();
        m_nSourceBaseSize = m_nSourceSize = pSource->GetDataSize();
        return 0;
    }

    int32_t GetCurrentOffset()
    {
        return (int32_t)(m_pSource - m_pSourceBase);
    }

    // Set maximum NAL unit size
    virtual void SetSuggestedSize(size_t size)
    {
        if (size > m_suggestedSize)
            m_suggestedSize = size;
    }

    // Returns first NAL unit ID in memory buffer
    virtual int32_t CheckNalUnitType(UMC::MediaData * pSource) = 0;
    // Set bitstream pointer to start code address
    virtual int32_t MoveToStartCode(UMC::MediaData * pSource) = 0;
    // Set destination bitstream pointer and size to NAL unit
    virtual int32_t GetNALUnit(UMC::MediaData * pSource, UMC::MediaData * pDst) = 0;

    virtual void Reset() = 0;

protected:
    uint8_t * m_pSource;
    size_t  m_nSourceSize;

    uint8_t * m_pSourceBase;
    size_t  m_nSourceBaseSize;

    size_t  m_suggestedSize;
};

// NAL unit splitter utility class
class NALUnitSplitter_H265
{
public:

    NALUnitSplitter_H265();

    virtual ~NALUnitSplitter_H265();

    // Initialize splitter with default values
    virtual void Init();
    // Free resources
    virtual void Release();

    // Returns first NAL unit ID in memory buffer
    virtual int32_t CheckNalUnitType(UMC::MediaData * pSource);
    // Set bitstream pointer to start code address
    virtual int32_t MoveToStartCode(UMC::MediaData * pSource);
    // Set destination bitstream pointer and size to NAL unit
    virtual UMC::MediaDataEx * GetNalUnits(UMC::MediaData * in);

    // Reset state
    virtual void Reset();

    // Set maximum NAL unit size
    virtual void SetSuggestedSize(size_t size)
    {
        if (!m_pStartCodeIter)
            return;

        m_pStartCodeIter->SetSuggestedSize(size);
    }

    SwapperBase * GetSwapper()
    {
        return m_pSwapper;
    }

protected:

    SwapperBase *   m_pSwapper;
    StartCodeIteratorBase * m_pStartCodeIter;

    UMC::MediaDataEx m_MediaData;
    UMC::MediaDataEx::_MediaDataEx m_MediaDataEx;
};

} // namespace UMC_HEVC_DECODER

#endif // __UMC_H265_NAL_SPL_H
#endif // MFX_ENABLE_H265_VIDEO_DECODE
