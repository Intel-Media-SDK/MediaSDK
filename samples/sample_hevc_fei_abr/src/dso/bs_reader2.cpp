// Copyright (c) 2019 Intel Corporation
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

#include "bs_reader2.h"
#include <memory.h>

namespace BsReader2
{
File::File()
    : m_f(0)
{}

File::~File()
{
    Close();
}

bool File::Open(const char* file, Bs32u buffSize)
{
    Close();
#if !defined(__GNUC__) && !defined(__clang__)
  #pragma warning(disable:4996)
#endif

    m_f = fopen(file, "rb");
    if (m_f)
        m_b.resize(buffSize);
    return !!m_f;
}

void File::Close()
{
    if (m_f)
        fclose(m_f);
    m_f = 0;
}

bool File::UpdateBuffer(Bs8u*& start, Bs8u*& cur, Bs8u*& end, Bs32u keepBytes)
{
    if (!m_f || feof(m_f))
        return false;

    keepBytes = BS_MIN(keepBytes, Bs32u(cur - start));
    cur -= keepBytes;

    if (Bs32u(end - cur) >= m_b.size())
        return false;

    memmove(&m_b[0], cur, end - cur);

    start = &m_b[0];
    end = start + (end - cur);
    cur = start + keepBytes;

    end += fread(end, 1, m_b.size() - (end - start), m_f);

    return true;
}


Reader::Reader()
{
    m_bsStart = 0;
    m_bsEnd = 0;
    m_bs = 0;
    m_bitOffset = 0;
    m_bitStart = 0;
    m_emulation = false;
    m_updater = 0;
    m_trace = true;
    m_traceOffset = true;
    m_traceLevel = 0xFFFFFFFF;
    m_tla = m_trace;
    m_tln = 0;
    m_log = stdout;
}

Reader::~Reader()
{
}

void Reader::Reset(Bs8u* bs, Bs32u size, Bs8u bitOffset)
{
    if (bs)
    {
        m_bsStart = bs;
        m_bsEnd = bs + size;
        m_bs = bs;
        m_bitOffset = (bitOffset & 7);
        m_bitStart = (bitOffset & 7);
    }
    else
    {
        m_bs = m_bsStart;
        m_bitOffset = m_bitStart;
    }
    m_updater = 0;
    m_startOffset = 0;
}

void Reader::MoreData(Bs32u keepBytes)
{
    auto NewOffset = m_startOffset;
    keepBytes = BS_MIN(keepBytes, Bs32u(m_bs - m_bsStart));

    if (m_bsStart && m_bs && m_bsEnd)
        NewOffset += (m_bs - m_bsStart) - keepBytes;

    if (   !m_updater
        || !m_updater->UpdateBuffer(m_bsStart, m_bs, m_bsEnd, keepBytes))
        throw EndOfBuffer();

    m_startOffset = NewOffset;
}

bool Reader::MoreDataNoThrow(Bs32u keepBytes)
{
    auto NewOffset = m_startOffset;
    keepBytes = BS_MIN(keepBytes, Bs32u(m_bs - m_bsStart));

    if (m_bsStart && m_bs && m_bsEnd)
        NewOffset += (m_bs - m_bsStart) - keepBytes;

    if (   !m_updater
        || !m_updater->UpdateBuffer(m_bsStart, m_bs, m_bsEnd, keepBytes))
        return false;

    m_startOffset = NewOffset;
    return true;
}

bool Reader::NextStartCode(bool stopBefore)
{
    if (m_bitOffset)
        ++m_bs;

    m_bitOffset = 0;

    for (;; m_bs++)
    {
        if (m_bs >= m_bsEnd)
            MoreData();

        if (   m_bs - m_bsStart >= 2
            && m_bs[0]  == 0x01
            && m_bs[-1] == 0x00
            && m_bs[-2] == 0x00)
        {
            bool LongSC = (m_bs - m_bsStart >= 3) && m_bs[-3] == 0x00;

            if (stopBefore)
                m_bs -= (3 + LongSC);

            ++m_bs;

            return LongSC;
        }

        if (   m_emulation
            && m_bs - m_bsStart >= 2
            && m_bs[-1] == 0x00
            && m_bs[-2] == 0x00)
        {
            if (m_bs + 1 >= m_bsEnd)
                MoreDataNoThrow();

            if (   (m_bsEnd - m_bs) >= 1
                && m_bs[0] == 0x03
                && (m_bs[1] & 0xfc) == 0x00)
            {
                ++m_bs;
                ++m_emuBytes;
            }
        }
    }

    return false;
}

bool Reader::TrailingBits(bool stopBefore)
{
    if (m_bs >= m_bsEnd)
        MoreData();

    if (Bs8u((*m_bs) << m_bitOffset) == 0x80)
    {
        if (!stopBefore)
        {
            m_bitOffset = 0;
            ++m_bs;
        }
        return true;
    }

    return false;
}

bool Reader::NextBytes(Bs8u* b, Bs32u n)
{
    if (m_bs + n >= m_bsEnd)
        MoreDataNoThrow();

    if (m_bs + n >= m_bsEnd)
        return false;

    if (b != m_bs)
        memcpy(b, m_bs, n);

    return true;
}

Bs32u Reader::GetByte()
{
    if (m_bs >= m_bsEnd)
        MoreData();

    Bs32u b = *m_bs;

    ++m_bs;

    if (   m_emulation
        && m_bs - m_bsStart >= 2
        && m_bs[-1] == 0x00
        && m_bs[-2] == 0x00)
    {
        if (m_bs+1 >= m_bsEnd)
            MoreDataNoThrow();

        if (   (m_bsEnd - m_bs) >= 1
            && m_bs[0] == 0x03
            && (m_bs[1] & 0xfc) == 0x00)
        {
            ++m_bs;
            ++m_emuBytes;
        }
    }

    return b;
}

Bs32u Reader::GetBytes(Bs32u n, bool nothrow)
{
    Bs32u b = 0;

    while (n--)
    {
        if (m_bs >= m_bsEnd)
        {
            if (!nothrow)
                MoreData();
            else if (!MoreDataNoThrow())
                return (b << (8 * (n + 1)));
        }

        b = (b << 8) | *m_bs;

        ++m_bs;

        if (   m_emulation
            && m_bs - m_bsStart >= 2
            && m_bs[-1] == 0x00
            && m_bs[-2] == 0x00)
        {
            if (m_bs + 1 >= m_bsEnd)
                MoreDataNoThrow();

            if ((m_bsEnd - m_bs) >= 1
                && m_bs[0] == 0x03
                && (m_bs[1] & 0xfc) == 0x00)
            {
                ++m_bs;
                ++m_emuBytes;
            }
        }
    }

    return b;
}

void Reader::ReturnByte()
{
    if (--m_bs < m_bsStart)
        throw Exception(BS_ERR_NOT_ENOUGH_BUFFER);

    if (   m_emulation
        && (m_bs - m_bsStart) > 3
        && m_bs[ 0]       == 0x03
        && m_bs[-1]       == 0x00
        && m_bs[-2]       == 0x00
        && (m_bs[1] >> 2) == 0x00)
    {
        --m_emuBytes;

        if (--m_bs < m_bsStart)
            throw Exception(BS_ERR_NOT_ENOUGH_BUFFER);
    }
}

void Reader::ReturnBits(Bs32u n)
{
    while (n >= 8)
    {
        ReturnByte();
        n -= 8;
    }

    if (n > m_bitOffset)
    {
        ReturnByte();
        m_bitOffset += 8;
    }

    m_bitOffset -= n;
}

Bs32u Reader::ExtractData(Bs8u* buf, Bs32u size)
{
    assert(!m_bitOffset); // TODO
    assert(!m_emulation); // TODO
    Bs8u* begin = buf;
    Bs8u* end = buf + size;

    while (buf < end)
    {
        Bs32u S = Bs32u(m_bsEnd - m_bs);
        S = BS_MIN(S, size);

        memmove(buf, m_bs, S);
        buf  += S;
        m_bs += S;

        if (m_bsEnd == m_bs)
        {
            if (MoreDataNoThrow())
                break;
        }
    }

    return Bs32u(buf - begin);
}

Bs32u Reader::ExtractRBSP(Bs8u* buf, Bs32u size)
{
    Bs8u* begin = buf;

    m_bitOffset = 0;

    if (size < 4)
        throw Exception(BS_ERR_NOT_ENOUGH_BUFFER);

    if (m_bs + 4 >= m_bsEnd)
        MoreDataNoThrow();

    if (m_bs + 3 >= m_bsEnd)
    {
        while (m_bs < m_bsEnd)
            *buf++ = *m_bs++;

        return Bs32u(buf - begin);
    }

    *buf++ = *m_bs++;
    *buf++ = *m_bs++;
    *buf++ = *m_bs++;

    if (m_bs >= m_bsEnd && !MoreDataNoThrow())
        return Bs32u(buf - begin);

    size -= 3;

    for (;;)
    {
        if (   m_bs[0]  == 0x01
            && m_bs[-1] == 0x00
            && m_bs[-2] == 0x00)
        {
            bool LongSC = (m_bs - m_bsStart >= 3) && *(m_bs - 3) == 0x00;
            m_bs -= (1 + LongSC);
            buf  -= (1 + LongSC);

            return Bs32u(buf - begin);
        }

        if (!size--)
            throw Exception(BS_ERR_NOT_ENOUGH_BUFFER);

        *buf++ = *m_bs++;

        if (m_bs >= m_bsEnd && !MoreDataNoThrow())
            return Bs32u(buf - begin);

        if (   m_emulation
            && m_bs[-1]       == 0x03
            && m_bs[-2]       == 0x00
            && m_bs[-3]       == 0x00
            && (m_bs[0] >> 2) == 0x00)
        {
            m_emuBytes++;
            buf--;
        }
    }

    return 0;
}

Bs32u Reader::GetBit()
{
    if (m_bs >= m_bsEnd)
        MoreData();

    Bs32u b = (*m_bs >> (7 - m_bitOffset)) & 1;

    if (++m_bitOffset == 8)
    {
        ++m_bs;
        m_bitOffset = 0;

        if (   m_emulation
            && m_bs - m_bsStart >= 2
            && m_bs[-1] == 0x00
            && m_bs[-2] == 0x00)
        {
            if (m_bs+1 >= m_bsEnd)
                MoreDataNoThrow();

            if (   (m_bsEnd - m_bs) >= 1
                && m_bs[0] == 0x03
                && (m_bs[1] & 0xfc) == 0x00)
            {
                ++m_bs;
                ++m_emuBytes;
            }
        }
    }

    return b;
}

Bs32u Reader::GetBits(Bs32u n)
{
    Bs32u b = 0;

    while (n--)
        b = (b << 1) | GetBit();

    return b;
}

void Reader::GetBits(void* b, Bs32u o, Bs32u n)
{
    Bs8u *p = (Bs8u*)b;

    if (o)
    {
        p += (o / 8);
        o &= 7;
        *p = (*p & ~(0xFF >> o));
    }

    o = 7 - o;

    while (n--)
    {
        *p |= GetBit() << o;

        if (!o--)
        {
            o = 7;
            *++p = 0;
        }
    }
}

Bs32u Reader::GetUE()
{
    Bs32u lz = 0;

    while (!GetBit())
        lz++;

    return !lz ? 0 : ((1 << lz) | GetBits(lz)) - 1;
}

Bs32s Reader::GetSE()
{
    Bs32u ue = GetUE();
    return (ue & 1) ? (Bs32s)((ue + 1) >> 1) : -(Bs32s)((ue + 1) >> 1);
}

}
