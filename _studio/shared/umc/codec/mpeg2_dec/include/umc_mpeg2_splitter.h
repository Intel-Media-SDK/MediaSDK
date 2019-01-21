// Copyright (c) 2018 Intel Corporation
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

#pragma once

#include "umc_defs.h"

#if defined (MFX_ENABLE_MPEG2_VIDEO_DECODE)

#include "umc_mpeg2_defs.h"

namespace UMC
{ class MediaData; }

namespace UMC_MPEG2_DECODER
{
    // The purpose of this class is to search syntax elements in provided bistream based on 0x0 0x0 0x1 sequence.
    class RawHeaderIterator
    {
    public:

        void Reset()
        {
            m_source = nullptr;
            m_cache.resize(0);
            m_pts = -1;
            m_needCleanCache = false;
        }

        RawUnit & operator *()
        {
            return m_nalu;
        }

        RawUnit * operator ->()
        {
            return &m_nalu;
        }

        // Load new data from source and searches for a unit
        void LoadData(UMC::MediaData*);

        // Searches for a unit
        RawHeaderIterator & operator++();
        RawHeaderIterator operator++(int)
        {
            RawHeaderIterator tmp;
            ++*this;
            return tmp;
        }

        bool operator ==(RawHeaderIterator const & right) const
        {
            return m_nalu.begin == right.m_nalu.begin && m_nalu.end == right.m_nalu.end;
        }

        bool operator !=(RawHeaderIterator const & right) const
        {
            return !(*this == right);
        }

        static uint8_t* FindStartCode(uint8_t * begin, uint8_t * end);

    private:
        RawUnit FindUnit(UMC::MediaData* in); // Find unit in provided bitstream. Save bs to cache if needed
        RawUnit OnEOS(); // Action on end of stream

        RawUnit FindRawUnit(uint8_t * begin, uint8_t * end); // Find unit by start code

        RawUnit m_nalu = {};

        UMC::MediaData* m_source = nullptr;

        std::vector<uint8_t>  m_cache;  // Started but not completed unit
        double                m_pts = -1;
        bool                  m_needCleanCache = false;
    };

    // This is a wrapper under RawHeaderIterator class.
    // The additional functionality introduced by the class is to return sequence/sequence extension or picture/picture extension headers in pairs.
    // This helps to handle corrupted stream where one of the headers is missing.
    class Splitter
    {
    public:
        void Reset()
        {
            m_it.Reset();
            m_buff.clear();
            m_cleanUpBuffer = false;
            m_typeExtToFind = 0;
        }

        // Main API function
        RawUnit GetUnits(UMC::MediaData*);

    private:

        RawHeaderIterator     m_it; // unit iterator

        std::vector <uint8_t> m_buff;  // Copy of found sequence or picture header
        bool                  m_cleanUpBuffer = false;

        uint8_t               m_typeExtToFind = 0; // Specifies type of extension header which we currently are looking for

    };
}

#endif // MFX_ENABLE_MPEG2_VIDEO_DECODE
