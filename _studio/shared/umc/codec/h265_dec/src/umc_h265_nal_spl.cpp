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

#include "umc_h265_nal_spl.h"
#include "mfx_common.h" //  for trace routines

namespace UMC_HEVC_DECODER
{
// NAL unit definitions
enum
{
    NAL_UNITTYPE_SHIFT_H265     = 1,
    NAL_UNITTYPE_BITS_H265      = 0x7e,
};

// Change memory region to little endian for reading with 32-bit DWORDs and remove start code emulation prevention byteps
void SwapMemoryAndRemovePreventingBytes_H265(void *pDestination, size_t &nDstSize, void *pSource, size_t nSrcSize, std::vector<uint32_t> *pRemovedOffsets);

// Search bitstream for start code
static int32_t FindStartCode(const uint8_t *pb, size_t &nSize)
{
    // there is no data
    if ((int32_t) nSize < 4)
        return -1;

    // find start code
    while ((4 <= nSize) && ((0 != pb[0]) ||
                            (0 != pb[1]) ||
                            (1 != pb[2])))
    {
        pb += 1;
        nSize -= 1;
    }

    if (4 <= nSize)
        return ((pb[0] << 24) | (pb[1] << 16) | (pb[2] << 8) | (pb[3]));

    return -1;

} // int32_t FindStartCode(uint8_t * (&pb), size_t &nSize)

// NAL unit splitter class
class StartCodeIterator : public StartCodeIteratorBase
{
public:

    StartCodeIterator()
        : m_code(-1)
        , m_pts(-1)
    {
        Reset();
    }

    virtual void Reset()
    {
        m_code = -1;
        m_pts = -1;
        m_prev.clear();
    }

    // Initialize with bitstream buffer
    virtual int32_t Init(UMC::MediaData * pSource)
    {
        Reset();
        StartCodeIteratorBase::Init(pSource);
        int32_t iCode = UMC_HEVC_DECODER::FindStartCode(m_pSource, m_nSourceSize);
        return iCode;
    }

    // Returns first NAL unit ID in memory buffer
    virtual int32_t CheckNalUnitType(UMC::MediaData * pSource)
    {
        if (!pSource)
            return -1;

        uint8_t * source = (uint8_t *)pSource->GetDataPointer();
        size_t  size = pSource->GetDataSize();

        int32_t startCodeSize;
        return FindStartCode(source, size, startCodeSize);
    }

    // Set bitstream pointer to start code address
    virtual int32_t MoveToStartCode(UMC::MediaData * pSource)
    {
        if (!pSource)
            return -1;

        if (m_code == -1)
            m_prev.clear();

        uint8_t * source = (uint8_t *)pSource->GetDataPointer();
        size_t  size = pSource->GetDataSize();

        int32_t startCodeSize;
        int32_t iCodeNext = FindStartCode(source, size, startCodeSize);

        pSource->MoveDataPointer((int32_t)(source - (uint8_t *)pSource->GetDataPointer()));
        if (iCodeNext != -1)
        {
             pSource->MoveDataPointer(-startCodeSize);
        }

        return iCodeNext;
    }

    // Set destination bitstream pointer and size to NAL unit
    virtual int32_t GetNALUnit(UMC::MediaData * pSource, UMC::MediaData * pDst)
    {
        if (!pSource)
            return EndOfStream(pDst);

        int32_t iCode = GetNALUnitInternal(pSource, pDst);
        if (iCode == -1)
        {
            bool endOfStream = pSource && ((pSource->GetFlags() & UMC::MediaData::FLAG_VIDEO_DATA_END_OF_STREAM) != 0);
            if (endOfStream)
                iCode = EndOfStream(pDst);
        }

        return iCode;
    }

    // Set destination bitstream pointer and size to NAL unit
    int32_t GetNALUnitInternal(UMC::MediaData * pSource, UMC::MediaData * pDst)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "GetNALUnitInternal");
        static const uint8_t startCodePrefix[] = {0, 0, 1};

        if (m_code == -1)
            m_prev.clear();

        uint8_t * source = (uint8_t *)pSource->GetDataPointer();
        size_t  size = pSource->GetDataSize();

        if (!size)
            return -1;

        int32_t startCodeSize;

        int32_t iCodeNext = FindStartCode(source, size, startCodeSize);

        // Use start code data which is saved from previous call because start code could be split between input buffers from application
        if (!m_prev.empty())
        {
            if (iCodeNext == -1)
            {
                size_t szToAdd = source - (uint8_t *)pSource->GetDataPointer();
                size_t szToMove = szToAdd;
                if (m_prev.size() + szToAdd >  m_suggestedSize)
                {
                    szToAdd = (m_suggestedSize > m_prev.size()) ? m_suggestedSize - m_prev.size() : 0;
                }

                m_prev.insert(m_prev.end(), (uint8_t *)pSource->GetDataPointer(), (uint8_t *)pSource->GetDataPointer() + szToAdd);
                pSource->MoveDataPointer((int32_t)szToMove);
                return -1;
            }

            source -= startCodeSize;
            m_prev.insert(m_prev.end(), (uint8_t *)pSource->GetDataPointer(), source);
            pSource->MoveDataPointer((int32_t)(source - (uint8_t *)pSource->GetDataPointer()));

            pDst->SetFlags(UMC::MediaData::FLAG_VIDEO_DATA_NOT_FULL_FRAME);
            pDst->SetBufferPointer(&(m_prev[3]), m_prev.size() - 3);
            pDst->SetDataSize(m_prev.size() - 3);
            pDst->SetTime(m_pts);
            int32_t code = m_code;
            m_code = -1;
            m_pts = -1;
            return code;
        }

        if (iCodeNext == -1)
        {
            pSource->MoveDataPointer((int32_t)(source - (uint8_t *)pSource->GetDataPointer()));
            return -1;
        }

        m_pts = pSource->GetTime();
        m_code = iCodeNext;

        // move before start code
        pSource->MoveDataPointer((int32_t)(source - (uint8_t *)pSource->GetDataPointer() - startCodeSize));

        int32_t startCodeSize1;
        iCodeNext = FindStartCode(source, size, startCodeSize1);

        pSource->MoveDataPointer(startCodeSize);

        uint32_t flags = pSource->GetFlags();

        if (iCodeNext == -1 && !(flags & UMC::MediaData::FLAG_VIDEO_DATA_NOT_FULL_UNIT))
        {
            iCodeNext = 1;
            startCodeSize1 = 0;
            source += size;
            size = 0;
        }

        if (iCodeNext == -1)
        {
            if (m_code == NAL_UT_SPS)
            {
                pSource->MoveDataPointer(-startCodeSize); // leave start code for SPS
                return -1;
            }

            VM_ASSERT(!m_prev.size());

            size_t sz = source - (uint8_t *)pSource->GetDataPointer();
            size_t szToMove = sz;
            if (sz >  m_suggestedSize)
            {
                sz = m_suggestedSize;
            }

            if (!m_prev.size())
                m_prev.insert(m_prev.end(), startCodePrefix, startCodePrefix + sizeof(startCodePrefix));
            m_prev.insert(m_prev.end(), (uint8_t *)pSource->GetDataPointer(), (uint8_t *)pSource->GetDataPointer() + sz);
            pSource->MoveDataPointer((int32_t)szToMove);
            return -1;
        }

        // fill
        size_t nal_size = source - (uint8_t *)pSource->GetDataPointer() - startCodeSize1;
        pDst->SetBufferPointer((uint8_t*)pSource->GetDataPointer(), nal_size);
        pDst->SetDataSize(nal_size);
        pDst->SetFlags(pSource->GetFlags());
        pSource->MoveDataPointer((int32_t)nal_size);

        int32_t code = m_code;
        m_code = -1;

        pDst->SetTime(m_pts);
        m_pts = -1;
        return code;
    }

    // Reset state because stream is finished
    int32_t EndOfStream(UMC::MediaData * pDst)
    {
        if (m_code == -1)
        {
            m_prev.clear();
            return -1;
        }

        if (m_prev.size())
        {
            pDst->SetBufferPointer(&(m_prev[3]), m_prev.size() - 3);
            pDst->SetDataSize(m_prev.size() - 3);
            pDst->SetTime(m_pts);
            int32_t code = m_code;
            m_code = -1;
            m_pts = -1;
            return code;
        }

        m_code = -1;
        return -1;
    }

private:
    std::vector<uint8_t>  m_prev;
    int32_t   m_code;
    double   m_pts;

    // Searches NAL unit start code, places input pointer to it and fills up size paramters
    // ML: OPT: TODO: Replace with MaxL's fast start code search
    int32_t FindStartCode(uint8_t * (&pb), size_t & size, int32_t & startCodeSize)
    {
        uint32_t zeroCount = 0;

        int32_t i = 0;
        for (; i < (int32_t)size - 2; )
        {
            if (pb[1])
            {
                pb += 2;
                i += 2;
                continue;
            }

            zeroCount = 0;
            if (!pb[0])
                zeroCount++;

            uint32_t j;
            for (j = 1; j < (uint32_t)size - i; j++)
            {
                if (pb[j])
                    break;
            }

            zeroCount = zeroCount ? j: j - 1;

            pb += j;
            i += j;

            if (i >= (int32_t)size)
            {
                break;
            }

            if (zeroCount >= 2 && pb[0] == 1)
            {
                startCodeSize = std::min(zeroCount + 1, 4u);
                size -= i + 1;
                pb++; // remove 0x01 symbol
                if (size >= 1)
                {
                    return (pb[0] & NAL_UNITTYPE_BITS_H265) >> NAL_UNITTYPE_SHIFT_H265;
                }
                else
                {
                    pb -= startCodeSize;
                    size += startCodeSize;
                    startCodeSize = 0;
                    return -1;
                }
            }

            zeroCount = 0;
        }

        if (!zeroCount)
        {
            for (uint32_t k = 0; k < size - i; k++, pb++)
            {
                if (pb[0])
                {
                    zeroCount = 0;
                    continue;
                }

                zeroCount++;
            }
        }

        zeroCount = std::min(zeroCount, 3u);
        pb -= zeroCount;
        size = zeroCount;
        startCodeSize = zeroCount;
        return -1;
    }
};

// Memory big-to-little endian converter implementation
class Swapper : public SwapperBase
{
public:

    virtual void SwapMemory(uint8_t *pDestination, size_t &nDstSize, uint8_t *pSource, size_t nSrcSize, std::vector<uint32_t> *pRemovedOffsets)
    {
        SwapMemoryAndRemovePreventingBytes_H265(pDestination, nDstSize, pSource, nSrcSize, pRemovedOffsets);
    }

    virtual void SwapMemory(MemoryPiece * pMemDst, MemoryPiece * pMemSrc, std::vector<uint32_t> *pRemovedOffsets)
    {
        size_t dstSize = pMemSrc->GetDataSize();
        SwapMemory(pMemDst->GetPointer(),
                    dstSize,
                    pMemSrc->GetPointer(),
                    pMemSrc->GetDataSize(),
                    pRemovedOffsets);

        VM_ASSERT(pMemDst->GetSize() >= dstSize);
        size_t tail_size = std::min<size_t>(pMemDst->GetSize() - dstSize, DEFAULT_NU_TAIL_SIZE);
        memset(pMemDst->GetPointer() + dstSize, DEFAULT_NU_TAIL_VALUE, tail_size);
        pMemDst->SetDataSize(dstSize);
        pMemDst->SetTime(pMemSrc->GetTime());
    }
};

NALUnitSplitter_H265::NALUnitSplitter_H265()
    : m_pSwapper(0)
    , m_pStartCodeIter(0)
{
    m_MediaData.SetExData(&m_MediaDataEx);
}

NALUnitSplitter_H265::~NALUnitSplitter_H265()
{
    Release();
}

// Initialize splitter with default values
void NALUnitSplitter_H265::Init()
{
    Release();

    m_pSwapper = new Swapper();
    m_pStartCodeIter = new StartCodeIterator();
}

// Reset state
void NALUnitSplitter_H265::Reset()
{
    if (m_pStartCodeIter)
    {
        m_pStartCodeIter->Reset();
    }
}

// Free resources
void NALUnitSplitter_H265::Release()
{
    delete m_pSwapper;
    m_pSwapper = 0;
    delete m_pStartCodeIter;
    m_pStartCodeIter = 0;
}

// Returns first NAL unit ID in memory buffer
int32_t NALUnitSplitter_H265::CheckNalUnitType(UMC::MediaData * pSource)
{
    return m_pStartCodeIter->CheckNalUnitType(pSource); // find first start code
}

// Set bitstream pointer to start code address
int32_t NALUnitSplitter_H265::MoveToStartCode(UMC::MediaData * pSource)
{
    return m_pStartCodeIter->MoveToStartCode(pSource); // find first start code
}

// Set destination bitstream pointer and size to NAL unit
UMC::MediaDataEx * NALUnitSplitter_H265::GetNalUnits(UMC::MediaData * pSource)
{
    UMC::MediaDataEx * out = &m_MediaData;
    UMC::MediaDataEx::_MediaDataEx* pMediaDataEx = &m_MediaDataEx;

    int32_t iCode = m_pStartCodeIter->GetNALUnit(pSource, out);

    if (iCode == -1)
    {
        pMediaDataEx->count = 0;
        return 0;
    }

    pMediaDataEx->values[0] = iCode;

    pMediaDataEx->offsets[0] = 0;
    pMediaDataEx->offsets[1] = (int32_t)out->GetDataSize();
    pMediaDataEx->count = 1;
    pMediaDataEx->index = 0;
    return out;
}

// Utility class for writing 32-bit little endian integers
class H265DwordPointer_
{
public:
    // Default constructor
    H265DwordPointer_(void)
    {
        m_pDest = NULL;
        m_iCur = 0;
        m_nByteNum = 0;
    }

    H265DwordPointer_ operator = (void *pDest)
    {
        m_pDest = (uint32_t *) pDest;
        m_nByteNum = 0;
        m_iCur = 0;

        return *this;
    }

    // Increment operator
    H265DwordPointer_ &operator ++ (void)
    {
        if (4 == ++m_nByteNum)
        {
            *m_pDest = m_iCur;
            m_pDest += 1;
            m_nByteNum = 0;
            m_iCur = 0;
        }
        else
            m_iCur <<= 8;

        return *this;
    }

    uint8_t operator = (uint8_t nByte)
    {
        m_iCur = (m_iCur & ~0x0ff) | ((uint32_t) nByte);

        return nByte;
    }

protected:
    uint32_t *m_pDest;                                            // (uint32_t *) pointer to destination buffer
    uint32_t m_nByteNum;                                          // (uint32_t) number of current byte in dword
    uint32_t m_iCur;                                              // (uint32_t) current dword
};

// Utility class for reading big endian bitstream
class H265SourcePointer_
{
public:
    // Default constructor
    H265SourcePointer_(void)
    {
        m_nZeros = 0;
        m_pSource = NULL;
        m_nRemovedBytes = 0;
    }

    H265SourcePointer_ &operator = (void *pSource)
    {
        m_pSource = (uint8_t *) pSource;

        m_nZeros = 0;
        m_nRemovedBytes = 0;

        return *this;
    }

    H265SourcePointer_ &operator ++ (void)
    {
        uint8_t bCurByte = m_pSource[0];

        if (0 == bCurByte)
            m_nZeros += 1;
        else
        {
            if ((3 == bCurByte) && (2 <= m_nZeros))
                m_nRemovedBytes += 1;
            m_nZeros = 0;
        }

        m_pSource += 1;

        return *this;
    }

    bool IsPrevent(void)
    {
        if ((3 == m_pSource[0]) && (2 <= m_nZeros))
            return true;
        else
            return false;
    }

    operator uint8_t (void)
    {
        return m_pSource[0];
    }

    uint32_t GetRemovedBytes(void)
    {
        return m_nRemovedBytes;
    }

protected:
    uint8_t *m_pSource;                                           // (uint8_t *) pointer to destination buffer
    uint32_t m_nZeros;                                            // (uint32_t) number of preceding zeros
    uint32_t m_nRemovedBytes;                                     // (uint32_t) number of removed bytes
};

// Change memory region to little endian for reading with 32-bit DWORDs and remove start code emulation prevention byteps
void SwapMemoryAndRemovePreventingBytes_H265(void *pDestination, size_t &nDstSize, void *pSource, size_t nSrcSize, std::vector<uint32_t> *pRemovedOffsets)
{
    H265DwordPointer_ pDst;
    H265SourcePointer_ pSrc;
    size_t i;

    // DwordPointer object is swapping written bytes
    // H265SourcePointer_ removes preventing start-code bytes

    // reset pointer(s)
    pSrc = pSource;
    pDst = pDestination;

    // first two bytes
    i = 0;
    while (i < std::min<uint32_t>(2, nSrcSize))
    {
        pDst = (uint8_t) pSrc;
        ++pDst;
        ++pSrc;
        ++i;
    }

    // do swapping
    if (NULL != pRemovedOffsets)
    {
        while (i < (uint32_t) nSrcSize)
        {
            if (false == pSrc.IsPrevent())
            {
                pDst = (uint8_t) pSrc;
                ++pDst;
            }
            else
                pRemovedOffsets->push_back(uint32_t(i));

            ++pSrc;
            ++i;
        }
    }
    else
    {
        while (i < (uint32_t) nSrcSize)
        {
            if (false == pSrc.IsPrevent())
            {
                pDst = (uint8_t) pSrc;
                ++pDst;
            }

            ++pSrc;
            ++i;
        }
    }

    // write padding bytes
    nDstSize = nSrcSize - pSrc.GetRemovedBytes();
    while (nDstSize & 3)
    {
        pDst = (uint8_t) (0);
        ++nDstSize;
        ++pDst;
    }

} // void SwapMemoryAndRemovePreventingBytes_H265(void *pDst, size_t &nDstSize, void *pSrc, size_t nSrcSize, , std::vector<uint32_t> *pRemovedOffsets)

} // namespace UMC_HEVC_DECODER

#endif // MFX_ENABLE_H265_VIDEO_DECODE
