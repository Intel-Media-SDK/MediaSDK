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

#include "umc_defs.h"
#if defined (MFX_ENABLE_H264_VIDEO_DECODE)

#ifndef __UMC_H264_NAL_SPL_H
#define __UMC_H264_NAL_SPL_H

#include <vector>
#include "umc_h264_dec_defs_dec.h"
#include "umc_media_data_ex.h"
#include "umc_h264_heap.h"

namespace UMC
{

inline
bool IsItAllowedCode(int32_t iCode)
{
    return ((NAL_UT_SLICE <= (iCode & NAL_UNITTYPE_BITS)) &&
        (NAL_UT_PPS >= (iCode & NAL_UNITTYPE_BITS)) &&
        (NAL_UT_SEI != (iCode & NAL_UNITTYPE_BITS))) ||
        (NAL_UT_SPS_EX == (iCode & NAL_UNITTYPE_BITS)) ||
        (NAL_UT_AUXILIARY == (iCode & NAL_UNITTYPE_BITS));
} // bool IsItAllowedCode(int32_t iCode)

inline
bool IsHeaderCode(int32_t iCode)
{
    return (NAL_UT_SPS == (iCode & NAL_UNITTYPE_BITS)) ||
           (NAL_UT_SPS_EX == (iCode & NAL_UNITTYPE_BITS)) ||
           (NAL_UT_PPS == (iCode & NAL_UNITTYPE_BITS)) ||
           (NAL_UT_SUBSET_SPS == (iCode & NAL_UNITTYPE_BITS));
}

inline
bool IsVLCCode(int32_t iCode)
{
    return ((NAL_UT_SLICE <= (iCode & NAL_UNITTYPE_BITS)) &&
           (NAL_UT_IDR_SLICE >= (iCode & NAL_UNITTYPE_BITS))) ||
           (NAL_UT_AUXILIARY == (iCode & NAL_UNITTYPE_BITS)) ||
           (NAL_UT_CODED_SLICE_EXTENSION == (iCode & NAL_UNITTYPE_BITS));
}

class MediaData;
class NALUnitSplitter;

class NalUnit : public MediaData
{
public:
    NalUnit()
        : MediaData()
        , m_nal_unit_type(NAL_UT_UNSPECIFIED)
        , m_use_external_memory(false)
    {
    }

    NAL_Unit_Type GetNalUnitType() const
    {
        return (NAL_Unit_Type)m_nal_unit_type;
    }

    bool IsUsedExternalMem() const
    {
        return m_use_external_memory;
    }

    int m_nal_unit_type;
    bool m_use_external_memory;
};

class SwapperBase
{
public:
    virtual ~SwapperBase() {}

    virtual void SwapMemory(uint8_t *pDestination, size_t &nDstSize, uint8_t *pSource, size_t nSrcSize) = 0;
    virtual void SwapMemory(H264MemoryPiece * pMemDst, H264MemoryPiece * pMemSrc, uint8_t defaultValue = DEFAULT_NU_TAIL_VALUE) = 0;

    virtual void CopyBitStream(uint8_t *pDestination, uint8_t *pSource, size_t &nSrcSize) = 0;
};

class StartCodeIteratorBase
{
public:

    StartCodeIteratorBase()
        : m_pSource(0)
        , m_nSourceSize(0)
        , m_pSourceBase(0)
        , m_nSourceBaseSize(0)
        , m_suggestedSize(10 * 1024)
    {
    }

    virtual ~StartCodeIteratorBase()
    {
    }

    virtual int32_t Init(MediaData * pSource)
    {
        m_pSourceBase = m_pSource = (uint8_t *) pSource->GetDataPointer();
        m_nSourceBaseSize = m_nSourceSize = pSource->GetDataSize();
        return -1;
    }

    int32_t GetCurrentOffset()
    {
        return (int32_t)(m_pSource - m_pSourceBase);
    }

    virtual void SetSuggestedSize(size_t size)
    {
        if (size > m_suggestedSize)
            m_suggestedSize = size;
    }

    virtual int32_t GetNext() = 0;

    virtual int32_t CheckNalUnitType(MediaData * pSource) = 0;
    virtual int32_t MoveToStartCode(MediaData * pSource) = 0;
    virtual int32_t GetNALUnit(MediaData * pSource, NalUnit * pDst) = 0;

    virtual void Reset() = 0;

protected:
    uint8_t * m_pSource;
    size_t  m_nSourceSize;

    uint8_t * m_pSourceBase;
    size_t  m_nSourceBaseSize;

    size_t  m_suggestedSize;
};

class NALUnitSplitter
{
public:

    NALUnitSplitter();

    virtual ~NALUnitSplitter();

    virtual void Init();
    virtual void Release();

    virtual int32_t CheckNalUnitType(MediaData * pSource);
    virtual int32_t MoveToStartCode(MediaData * pSource);
    virtual NalUnit * GetNalUnits(MediaData * in);

    virtual void Reset();

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

    NalUnit     m_nalUnit;
};

} // namespace UMC

#endif // __UMC_H264_NAL_SPL_H
#endif // MFX_ENABLE_H264_VIDEO_DECODE
