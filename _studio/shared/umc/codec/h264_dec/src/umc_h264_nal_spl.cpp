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
#if defined (MFX_ENABLE_H264_VIDEO_DECODE)

#include <vector>
#include "umc_structures.h"
#include "umc_h264_nal_spl.h"

namespace UMC
{
void SwapMemoryAndRemovePreventingBytes(void *pDestination, size_t &nDstSize, void *pSource, size_t nSrcSize);

static int32_t FindStartCode(uint8_t * (&pb), size_t &nSize)
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

    virtual int32_t Init(MediaData * pSource)
    {
        Reset();
        StartCodeIteratorBase::Init(pSource);
        int32_t iCode = UMC::FindStartCode(m_pSource, m_nSourceSize);
        return iCode;
    }

    virtual int32_t GetNext()
    {
        m_pSource += 3;
        m_nSourceSize -= 3;
        int32_t iCode = UMC::FindStartCode(m_pSource, m_nSourceSize);
        return iCode;
    }

    virtual int32_t CheckNalUnitType(MediaData * pSource)
    {
        if (!pSource)
            return -1;

        uint8_t * source = (uint8_t *)pSource->GetDataPointer();
        size_t  size = pSource->GetDataSize();

        int32_t startCodeSize;
        return FindStartCode(source, size, startCodeSize);
    }

    virtual int32_t MoveToStartCode(MediaData * pSource)
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

    virtual int32_t GetNALUnit(MediaData * pSource, NalUnit * pDst)
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

    int32_t GetNALUnitInternal(MediaData * pSource, NalUnit * pDst)
    {
        if (m_code == -1)
            m_prev.clear();

        uint8_t * source = (uint8_t *)pSource->GetDataPointer();
        size_t  size = pSource->GetDataSize();

        if (!size)
            return -1;

        int32_t startCodeSize;

        int32_t iCodeNext = FindStartCode(source, size, startCodeSize);

        if (m_prev.size())
        {
            if (iCodeNext == -1)
            {
                size_t sz = source - (uint8_t *)pSource->GetDataPointer();
                if (m_prev.size() + sz >  m_suggestedSize)
                {
                    m_prev.clear();
                    sz = std::min(sz, m_suggestedSize);
                }

                m_prev.insert(m_prev.end(), (uint8_t *)pSource->GetDataPointer(), (uint8_t *)pSource->GetDataPointer() + sz);
                pSource->MoveDataPointer((int32_t)sz);
                return -1;
            }

            source -= startCodeSize;
            m_prev.insert(m_prev.end(), (uint8_t *)pSource->GetDataPointer(), source);
            pSource->MoveDataPointer((int32_t)(source - (uint8_t *)pSource->GetDataPointer()));

            pDst->m_use_external_memory = false;
            pDst->SetFlags(MediaData::FLAG_VIDEO_DATA_NOT_FULL_FRAME);
            pDst->SetBufferPointer(&(m_prev[0]), m_prev.size());
            pDst->SetDataSize(m_prev.size());
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

        if (iCodeNext == -1 && !(flags & MediaData::FLAG_VIDEO_DATA_NOT_FULL_UNIT))
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
            if (sz >  m_suggestedSize)
            {
                sz = m_suggestedSize;
            }

            m_prev.insert(m_prev.end(), (uint8_t *)pSource->GetDataPointer(), (uint8_t *)pSource->GetDataPointer() + sz);
            pSource->MoveDataPointer((int32_t)sz);
            return -1;
        }

        // fill
        size_t nal_size = source - (uint8_t *)pSource->GetDataPointer() - startCodeSize1;
        pDst->SetBufferPointer((uint8_t*)pSource->GetDataPointer(), nal_size);
        pDst->SetDataSize(nal_size);
        pSource->MoveDataPointer((int32_t)nal_size);
        pDst->SetFlags(pSource->GetFlags());

        int32_t code = m_code;
        m_code = -1;

        pDst->SetTime(m_pts);
        m_pts = -1;
        return code;
    }

    int32_t EndOfStream(NalUnit * pDst)
    {
        if (m_code == -1)
        {
            m_prev.clear();
            return -1;
        }

        if (m_prev.size())
        {
            pDst->SetBufferPointer(&(m_prev[0]), m_prev.size());
            pDst->SetDataSize(m_prev.size());
            pDst->SetTime(m_pts);
            pDst->m_use_external_memory = false;
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
                zeroCount = 0;
                if (size >= 1)
                {
                    return pb[0] & NAL_UNITTYPE_BITS;
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
        zeroCount = 0;
        startCodeSize = 0;
        return -1;
    }
};

class Swapper : public SwapperBase
{
public:

    virtual void SwapMemory(uint8_t *pDestination, size_t &nDstSize, uint8_t *pSource, size_t nSrcSize)
    {
        SwapMemoryAndRemovePreventingBytes(pDestination, nDstSize, pSource, nSrcSize);
    }

    virtual void SwapMemory(H264MemoryPiece * pMemDst, H264MemoryPiece * pMemSrc, uint8_t defaultValue)
    {
        size_t dstSize = pMemSrc->GetDataSize();
        SwapMemory(pMemDst->GetPointer(),
                    dstSize,
                    pMemSrc->GetPointer(),
                    pMemSrc->GetDataSize());

        size_t tail_size = pMemDst->GetSize() - dstSize;
        memset(pMemDst->GetPointer() + dstSize, defaultValue, tail_size);

        VM_ASSERT(pMemDst->GetSize() >= dstSize);
        pMemDst->SetDataSize(dstSize);
        pMemDst->SetTime(pMemSrc->GetTime());
    }

    virtual void CopyBitStream(uint8_t *pDestination, uint8_t *pSource, size_t &nSrcSize)
    {
        MFX_INTERNAL_CPY(pDestination, pSource, (uint32_t)nSrcSize);
    }
};

NALUnitSplitter::NALUnitSplitter()
    : m_pSwapper(0)
    , m_pStartCodeIter(0)
{
}

NALUnitSplitter::~NALUnitSplitter()
{
    Release();
}

void NALUnitSplitter::Init()
{
    Release();

    m_pSwapper = new Swapper();
    m_pStartCodeIter = new StartCodeIterator();
}

void NALUnitSplitter::Reset()
{
    if (m_pStartCodeIter)
    {
        m_pStartCodeIter->Reset();
    }
}

void NALUnitSplitter::Release()
{
    delete m_pSwapper;
    m_pSwapper = 0;
    delete m_pStartCodeIter;
    m_pStartCodeIter = 0;
}

int32_t NALUnitSplitter::CheckNalUnitType(MediaData * pSource)
{
    return m_pStartCodeIter->CheckNalUnitType(pSource); // find first start code
}

int32_t NALUnitSplitter::MoveToStartCode(MediaData * pSource)
{
    return m_pStartCodeIter->MoveToStartCode(pSource); // find first start code
}

NalUnit * NALUnitSplitter::GetNalUnits(MediaData * pSource)
{
    m_nalUnit.m_use_external_memory = true;

    int32_t iCode = m_pStartCodeIter->GetNALUnit(pSource, &m_nalUnit);

    if (iCode == -1)
    {
        return 0;
    }

    m_nalUnit.m_nal_unit_type = iCode;

    /*static int k = 0;
    if (k)
    {
        static FILE  * fl = fopen("f:\\out.avc", "wb");
        fwrite(start_code_prefix, 4, 1, fl);
        fwrite(out->GetDataPointer(), out->GetDataSize(), 1, fl);
    }*/

    return &m_nalUnit;
}

/* temporal class definition */
class H264DwordPointer_
{
public:
    // Default constructor
    H264DwordPointer_(void)
    {
        m_pDest = NULL;
        m_nByteNum = 0;
        m_iCur = 0;
    }

    H264DwordPointer_ operator = (void *pDest)
    {
        m_pDest = (uint32_t *) pDest;
        m_nByteNum = 0;
        m_iCur = 0;

        return *this;
    }

    // Increment operator
    H264DwordPointer_ &operator ++ (void)
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

class H264SourcePointer_
{
public:
    // Default constructor
    H264SourcePointer_(void)
    {
        m_pSource = NULL;
        m_nZeros = 0;
        m_nRemovedBytes = 0;
    }

    H264SourcePointer_ &operator = (void *pSource)
    {
        m_pSource = (uint8_t *) pSource;

        m_nZeros = 0;
        m_nRemovedBytes = 0;

        return *this;
    }

    H264SourcePointer_ &operator ++ (void)
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

void SwapMemoryAndRemovePreventingBytes(void *pDestination, size_t &nDstSize, void *pSource, size_t nSrcSize)
{
    H264DwordPointer_ pDst;
    H264SourcePointer_ pSrc;
    size_t i;

    // DwordPointer object is swapping written bytes
    // H264SourcePointer_ removes preventing start-code bytes

    // reset pointer(s)
    pSrc = pSource;
    pDst = pDestination;

    // first two bytes
    i = 0;
    while (i < std::min(size_t(2), nSrcSize))
    {
        pDst = (uint8_t) pSrc;
        ++pDst;
        ++pSrc;
        ++i;
    }

    // do swapping
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

    // write padding bytes
    nDstSize = nSrcSize - pSrc.GetRemovedBytes();
    while (nDstSize & 3)
    {
        pDst = (uint8_t) (DEFAULT_NU_TAIL_VALUE);
        ++nDstSize;
        ++pDst;
    }

} // void SwapMemoryAndRemovePreventingBytes(void *pDst, size_t &nDstSize, void *pSrc, size_t nSrcSize)

} // namespace UMC
#endif // MFX_ENABLE_H264_VIDEO_DECODE
