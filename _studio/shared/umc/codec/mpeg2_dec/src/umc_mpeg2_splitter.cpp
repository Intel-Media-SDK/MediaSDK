// Copyright (c) 2018-2019 Intel Corporation
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

#if defined (MFX_ENABLE_MPEG2_VIDEO_DECODE)

#include "umc_media_data.h"
#include "umc_mpeg2_splitter.h"

namespace UMC_MPEG2_DECODER
{
    void RawHeaderIterator::LoadData(UMC::MediaData* source)
    {
        m_source = source;
        m_nalu = RawUnit(); // reset

        if (!m_source)
        {
            m_nalu = OnEOS();
            return;
        }

        m_nalu = FindUnit(m_source);

        if ( (-1 == m_nalu.type) &&  // didn't find complete unit
                m_cache.size() &&       // but have beginning of the unit
                !((m_source->GetFlags() & UMC::MediaData::FLAG_VIDEO_DATA_NOT_FULL_FRAME)) ) // and complete frame flag is set
        {
            m_nalu = OnEOS();
        }
    }

    RawHeaderIterator & RawHeaderIterator::operator++()
    {
        LoadData(m_source); // pass m_source as an argument to re-use LoadData() code in operator++

        return *this;
    }

    RawUnit RawHeaderIterator::FindUnit(UMC::MediaData* in)
    {
        const auto begin = (uint8_t*) in->GetDataPointer();
        const auto end   = begin + in->GetDataSize();
        uint32_t readSize = 0;
        RawUnit unit;

        if (m_needCleanCache)
        {
            m_cache.clear();
            m_needCleanCache = false;
            m_pts = -1;
        }

        // We have a cached (an uncompleted) unit and got new piece of bitstream data
        if (m_cache.size())
        {
            const auto unitStart = FindStartCode(begin, end); // Check if new data have any start codes
            if (unitStart) // Start code was found. Need to close the cached unit
            {
                readSize = unitStart - begin; // Data before found startcode is the end of the cached unit
                m_cache.insert(m_cache.end(), (uint8_t *)begin, (uint8_t *)begin + readSize); // Load this data to the cached unit

                unit = { m_cache.data(), m_cache.data() + m_cache.size(), m_cache[prefix_size], m_pts } ; // Construct unit from cached data
                m_needCleanCache = true; // Need to clean up the cache on next search iteration
            }
            else // start code wasn't found
            {
                // Consume all data from buffer except last 1 or 2 bytes if they have zero values (considering [... 0x0 0x0] [0x1 ...] sequence).
                // These two bytes are to prevent start code chopping between two separate FindRawUnit() calls.
                // An exception from this is 'MFX_BITSTREAM_COMPLETE_FRAME' case.
                const size_t len = end - begin;
                const uint32_t numZerosAtEnd = ((len > 0) && (0 == end[-1])) ? 1 + ((len > 1) && (0 == end[-2])) : 0;
                const uint32_t numBytesToLeave = !(m_source->GetFlags() & UMC::MediaData::FLAG_VIDEO_DATA_NOT_FULL_FRAME) ? 0 : numZerosAtEnd;
                readSize = end - begin - numBytesToLeave;
                m_cache.insert(m_cache.end(), begin, end - numBytesToLeave); // Load almost the whole data (without trailing bytes if any) to the cache
            }

            in->MoveDataPointer(readSize);
            return unit;
        }

        RawUnit tmpUnit = FindRawUnit(begin, end);
        if (-1 == tmpUnit.type || (!tmpUnit.begin && !tmpUnit.end)) // Nothing found
        {
            readSize = end - begin; // Skip all data
        }
        else if (tmpUnit.begin && !tmpUnit.end) // Found only where unit starts
        {
            // Consume all data from buffer except last 1 or 2 bytes if they have zero values (considering [... 0x0 0x0] [0x1 ...] sequence).
            // These two bytes are to prevent start code chopping between two separate FindRawUnit() calls.
            // An exception from this is 'MFX_BITSTREAM_COMPLETE_FRAME' case.
            const size_t len = end - begin;
            const uint32_t numZerosAtEnd = ((len > 0) && (0 == end[-1])) ? 1 + ((len > 1) && (0 == end[-2])) : 0;
            const uint32_t numBytesToLeave = !(m_source->GetFlags() & UMC::MediaData::FLAG_VIDEO_DATA_NOT_FULL_FRAME) ? 0 : numZerosAtEnd;
            readSize = end - begin - numBytesToLeave;
            m_cache.insert(m_cache.end(), tmpUnit.begin, end - numBytesToLeave); // But save to the cache only since start code
            m_pts = in->GetTime();
        }
        else // found begin and end
        {
            readSize = tmpUnit.end - begin; // Up to the end of the unit
            unit = tmpUnit;
            unit.pts = in->GetTime();
        }

        in->MoveDataPointer(readSize);

        return unit;
    }

    RawUnit RawHeaderIterator::OnEOS()
    {
        RawUnit nalu;
        if (m_needCleanCache)
        {
            m_cache.clear();
            m_needCleanCache = false;
            m_pts = -1;
        }

        if (m_cache.size())
        {
            nalu = { m_cache.data(), m_cache.data() + m_cache.size(), m_cache[prefix_size], m_pts } ; // Construct unit from cached data
            m_needCleanCache = true; // Need to clean up the cache on next iteration
        }

        return nalu;
    }

    // Find start code
    uint8_t * RawHeaderIterator::FindStartCode(uint8_t * begin, uint8_t * end)
    {
        for (; begin < end - prefix_size; ++begin)
        {
            if (begin[0] == 0 && begin[1] == 0 && begin[2] == 1)
            {
                return begin;
            }
        }
        return nullptr;
    }

    // Find unit start, end and type
    RawUnit RawHeaderIterator::FindRawUnit(uint8_t * begin, uint8_t * end)
    {
        begin = FindStartCode(begin, end - 1); // Find beginning of unit. "- 1" is to obtain type of unit
        if (nullptr == begin)
            return RawUnit();

        uint8_t type = begin[prefix_size];

        uint8_t * next = FindStartCode(begin + prefix_size + 1, end); // Find beginning of another unit

        return RawUnit(begin, next, type, -1); // "next" can be either nullptr or actuall address
    }

    // Main API call
    RawUnit Splitter::GetUnits(UMC::MediaData* source)
    {
        if (m_cleanUpBuffer)
        {
            m_buff.clear();
            m_cleanUpBuffer = false;
        }

        m_it.LoadData(source); // Find new unit

        // The loop is to handle invalid cases like: seq header, seq header, seq extension. If so, a header without a paired extension header will be skipped.
        while (true)
        {
            bool isHandlePairedHeaders = false;
            if (m_buff.size() && (m_it != RawHeaderIterator()))
            {
                isHandlePairedHeaders = true;
            }
            else if (SEQUENCE_HEADER == m_it->type || PICTURE_HEADER == m_it->type)
            {
                m_typeExtToFind = (SEQUENCE_HEADER == m_it->type) ? SEQUENCE_EXTENSION : PICTURE_CODING_EXTENSION;

                m_buff.clear();
                m_buff.insert(m_buff.end(), (uint8_t *)m_it->begin, (uint8_t *)m_it->end); // Put to cache

                ++m_it; // Look for another unit
                if (m_it != RawHeaderIterator())
                    isHandlePairedHeaders = true;
            }

            if (isHandlePairedHeaders)
            {
                size_t size = m_it->end - m_it->begin;
                auto type_ext = (MPEG2_UNIT_TYPE_EXT) ((m_it->type == EXTENSION && size >= 5) ? // ">= 5" is due to we need a header extension type which is in the 4th byte
                                                           (m_it->begin[prefix_size + 1] >> 4) : // begin[prefix_size + 1] >> 4 extracts header extension type
                                                           0 );
                if (m_typeExtToFind == type_ext)
                {
                    m_buff.insert(m_buff.end(), (uint8_t *)m_it->begin, (uint8_t *)m_it->end);

                    m_cleanUpBuffer = true;
                    m_typeExtToFind = 0;

                    return { m_buff.data(), m_buff.data() + m_buff.size(), m_buff[prefix_size], -1 };
                }
                else
                {
                    m_buff.clear(); // drop sequence/picture header without corresponding sequence/picture extension header
                    m_cleanUpBuffer = 0;
                    continue;
                }
            }

            break;
        }

        return *m_it;
    }
}
#endif // MFX_ENABLE_MPEG2_VIDEO_DECODE
