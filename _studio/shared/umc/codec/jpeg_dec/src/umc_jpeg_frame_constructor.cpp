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
#if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE)

#include "umc_jpeg_frame_constructor.h"
#include "jpegdec_base.h"

namespace UMC
{

//////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////
JpegFrameConstructor::JpegFrameConstructor()
    : m_prevLengthOfSegment(0)
    , m_code(0)
    , m_pts(-1)
    , m_suggestedSize(0)
{
    m_mediaData.SetExData(&m_mediaDataEx);
    Reset();
}

JpegFrameConstructor::~JpegFrameConstructor()
{
}

void JpegFrameConstructor::Init()
{
}

void JpegFrameConstructor::Reset()
{
    m_code = 0;
    m_pts = -1;
    m_suggestedSize = 0;
    m_prevLengthOfSegment = 0;
    m_RestartCount = 0;

    m_prev.clear();
    m_frame.clear();
    m_mediaDataEx.count = 0;
    m_mediaData.Close();

    m_flags.isSOI = 0;
    m_flags.isEOI = 0;
    m_flags.isSOS = 0;
}

void JpegFrameConstructor::Close()
{
    Reset();
}

int32_t JpegFrameConstructor::FindMarkerCode(uint8_t * (&source), size_t & size, int32_t & startCodeSize)
{
    uint32_t ffCount = 0;

    if (m_prevLengthOfSegment)
    {
        size_t copySize = std::min(m_prevLengthOfSegment, size);
        m_prevLengthOfSegment -= copySize;
        source += copySize;
        size -= copySize;

        if (m_prevLengthOfSegment)
            return 0;
    }

    for (;;)
    {
        uint32_t i = 0;
        ffCount = 0;
        for (; i < (uint32_t)size; i++)
        {
            if (source[i] == 0xff)
            {
                ffCount = 1;
                i++;
                break;
            }
        }

        for (; i < (uint32_t)size; i++, ffCount++)
        {
            if (source[i] != 0xff)
            {
                break;
            }
        }

        source += i;
        size -= i;

        if (size <= 0)
        {
            break;
        }

        if (!source[0])
        {
            ffCount = 0;
        }
        else
        {
            ffCount = 1;
            startCodeSize = ffCount + 1;
            size -= 1;
            int32_t code = source[0];
            source++;
            return code;
        }
    }

    source -= ffCount;
    size = ffCount;
    ffCount = 0;
    startCodeSize = 0;
    return 0;
}

int32_t JpegFrameConstructor::CheckMarker(MediaData * pSource)
{
    if (!pSource)
        return 0;

    if (!m_code)
        m_prev.clear();

    uint8_t * source = (uint8_t *)pSource->GetDataPointer();
    size_t  size = pSource->GetDataSize();

    int32_t startCodeSize;
    int32_t iCodeNext = FindMarkerCode(source, size, startCodeSize);

    pSource->MoveDataPointer((int32_t)(source - (uint8_t *)pSource->GetDataPointer()));
    if (iCodeNext)
    {
         pSource->MoveDataPointer(-startCodeSize);
    }

    return iCodeNext;
}

int32_t JpegFrameConstructor::GetMarker(MediaData * pSource, MediaData * pDst)
{
    if (!pSource)
        return EndOfStream(pDst);

    if (!m_code)
        m_prev.clear();

    uint8_t * source = (uint8_t *)pSource->GetDataPointer();
    size_t  size = pSource->GetDataSize();

    if (!size)
        return 0;

    int32_t startCodeSize;

    int32_t iCodeNext = FindMarkerCode(source, size, startCodeSize);

    if (m_prev.size())
    {
        if (!iCodeNext)
        {
            size_t sz = source - (uint8_t *)pSource->GetDataPointer();
            if (m_suggestedSize && m_prev.size() + sz >  m_suggestedSize)
            {
                m_prev.clear();
                sz = std::min(sz, m_suggestedSize);
            }

            m_prev.insert(m_prev.end(), (uint8_t *)pSource->GetDataPointer(), (uint8_t *)pSource->GetDataPointer() + sz);
            pSource->MoveDataPointer((int32_t)sz);
            return 0;
        }

        source -= startCodeSize;
        m_prev.insert(m_prev.end(), (uint8_t *)pSource->GetDataPointer(), source);
        pSource->MoveDataPointer((int32_t)(source - (uint8_t *)pSource->GetDataPointer()));

        pDst->SetBufferPointer(&(m_prev[0]), m_prev.size());
        pDst->SetDataSize(m_prev.size());
        pDst->SetTime(m_pts);
        int32_t code = m_code;
        m_code = 0;
        m_pts = -1;

        return code;
    }

    if (!iCodeNext)
    {
        pSource->MoveDataPointer((int32_t)(source - (uint8_t *)pSource->GetDataPointer()));
        return 0;
    }

    m_pts = pSource->GetTime();
    m_code = iCodeNext;

    // move before start code
    pSource->MoveDataPointer((int32_t)(source - (uint8_t *)pSource->GetDataPointer() - startCodeSize));

    uint32_t flags = pSource->GetFlags();

    bool wasNextMarkerFound = true;

    switch(iCodeNext)
    {
    case JM_RST0:  // standalone markers
    case JM_RST1:
    case JM_RST2:
    case JM_RST3:
    case JM_RST4:
    case JM_RST5:
    case JM_RST6:
    case JM_RST7:
    case JM_SOI:
    case JM_EOI:
    case JM_TEM:
        break;
    default:   // marker's segment.
        {
            if (size >= 2)
            {
                size_t length = (source[0] << 8) | source[1];

                m_prevLengthOfSegment = length;
                wasNextMarkerFound = true;
            }
            else
            {
                return 0;
            }
        }
        break;
    }

    if(m_code != JM_EOI)
    {
        int32_t startCodeSize1 = 0;

        iCodeNext = wasNextMarkerFound ? FindMarkerCode(source, size, startCodeSize1) : 0;

        //pSource->MoveDataPointer(startCodeSize);

        if (!iCodeNext && !(flags & MediaData::FLAG_VIDEO_DATA_NOT_FULL_UNIT))
        {
            iCodeNext = 1;
            startCodeSize1 = 0;
        }

        if (!iCodeNext)
        {
            VM_ASSERT(!m_prev.size());
            size_t sz = source - (uint8_t *)pSource->GetDataPointer();
            if (m_suggestedSize && sz >  m_suggestedSize)
            {
                sz = m_suggestedSize;
            }

            m_prev.insert(m_prev.end(), (uint8_t *)pSource->GetDataPointer(), (uint8_t *)pSource->GetDataPointer() + sz);
            pSource->MoveDataPointer((int32_t)sz);
            return 0;
        }

        // fill
        size_t nal_size = source - (uint8_t *)pSource->GetDataPointer() - startCodeSize1;
        pDst->SetBufferPointer((uint8_t*)pSource->GetDataPointer(), nal_size);
        pDst->SetDataSize(nal_size);
        pSource->MoveDataPointer((int32_t)nal_size);
    }
    else
    {
        pSource->MoveDataPointer(2);
    }

    int32_t code = m_code;
    m_code = 0;

    pDst->SetTime(m_pts);
    m_pts = -1;
    return code;
}

int32_t JpegFrameConstructor::EndOfStream(MediaData * pDst)
{
    if (!m_code)
    {
        m_prev.clear();
        return 0;
    }

    if (m_prev.size())
    {
        pDst->SetBufferPointer(&(m_prev[0]), m_prev.size());
        pDst->SetDataSize(m_prev.size());
        pDst->SetTime(m_pts);
        int32_t code = m_code;
        m_code = 0;
        m_pts = -1;
        return code;
    }

    m_code = 0;
    return 0;
}

Status JpegFrameConstructor::AddMarker(uint32_t marker, MediaDataEx::_MediaDataEx* pMediaDataEx, size_t nBufferSize, MediaData *dst)
{
    if (!dst->GetDataSize())
        return UMC_OK;

    uint32_t element = pMediaDataEx->count;

    if (element >= pMediaDataEx->limit - 1)
        return UMC_ERR_FAILED;

    m_frame.insert(m_frame.end(), (uint8_t*)dst->GetDataPointer(), (uint8_t*)dst->GetDataPointer() + dst->GetDataSize() );
    
    pMediaDataEx->offsets[0] = 0;
    // process RST marker
    if((JM_RST0 <= marker) && (JM_RST7 >= marker))
    {
        // add marker to MediaDataEx
        if((unsigned long long)m_frame.size() > ((unsigned long long)nBufferSize * element) / (pMediaDataEx->limit - 2))
        {
            pMediaDataEx->values[element] = marker | (m_RestartCount << 8);
            pMediaDataEx->offsets[element + 1] = (uint32_t)m_frame.size();
            pMediaDataEx->count++;
        }
        // the end of previous interval would be the start of next
        else
        {
            pMediaDataEx->offsets[element] = (uint32_t)m_frame.size();
        }
        m_RestartCount++;
    }
    // process SOS marker (add it anyway)
    else if(JM_SOS == marker)
    {
        pMediaDataEx->values[element] = marker | (m_RestartCount << 8);
        pMediaDataEx->offsets[element + 1] = (uint32_t)m_frame.size();
        pMediaDataEx->count++;
        m_RestartCount++;
    }
    else
    {
        pMediaDataEx->values[element] = marker | (m_RestartCount << 8);
        pMediaDataEx->offsets[element + 1] = (uint32_t)m_frame.size();
        pMediaDataEx->count++;
    }

    return UMC_OK;
}

void JpegFrameConstructor::ResetForNewFrame()
{
    m_frame.clear();
    m_mediaDataEx.count = 0;
    m_flags.isSOI = 0;
    m_flags.isEOI = 0;
    m_flags.isSOS = 0;
    m_RestartCount = 0;
}

MediaDataEx * JpegFrameConstructor::GetFrame(MediaData * in, uint32_t maxBitstreamSize)
{
    for (;;)
    {
        MediaData dst;
        uint32_t marker = GetMarker(in, &dst);
        
        if (marker == JM_NONE)
        {
            if(in == nullptr)
                break;

            // frame is not declared as "complete", and next marker is not found
            if(in->GetFlags() & MediaData::FLAG_VIDEO_DATA_NOT_FULL_FRAME)
                break;

            // frame is declared as "complete", but it does not contain SOS marker
            if(!(in->GetFlags() & MediaData::FLAG_VIDEO_DATA_NOT_FULL_FRAME) && m_flags.isSOS == 0)
                break;
        }      

        switch(marker)
        {
        case JM_SOI:
            if (m_flags.isSOI && m_flags.isSOS && m_frame.size() && !m_flags.isEOI) // EOI wasn't received
            {
                m_flags.isEOI = 1; // emulate EOI
                m_mediaData.SetBufferPointer(&m_frame[0], m_frame.size());
                m_mediaData.SetDataSize(m_frame.size());
                m_mediaData.SetFlags(in != nullptr ? in->GetFlags() : 0);
                return &m_mediaData;
            }

            m_mediaData.SetTime(in != nullptr ? in->GetTime() : 0);
            m_frame.clear();
            m_mediaDataEx.count = 0;
            m_flags.isSOI = 1;
            m_flags.isEOI = 0;
            m_flags.isSOS = 0;
            m_RestartCount = 0;
            break;

            // JPEG picture data
        case JM_RST0:
        case JM_RST1:
        case JM_RST2:
        case JM_RST3:
        case JM_RST4:
        case JM_RST5:
        case JM_RST6:
        case JM_RST7:
        case JM_SOS:
            if (m_flags.isSOI)
            {
                m_flags.isSOS = 1;
            }
            else
            {
                continue;
            }
            break;

        case JM_EOI:
        default:
            break;
        }

        if (AddMarker(marker, &m_mediaDataEx, std::max(in != nullptr ? in->GetBufferSize() : 0, (size_t)maxBitstreamSize), &dst) != UMC_OK)
        {
            ResetForNewFrame();
            continue;
        }

        if (marker == JM_EOI)
        {
            if (m_flags.isEOI || !m_frame.size() || !m_flags.isSOS)
            {
                ResetForNewFrame();
                continue;
            }

            m_flags.isEOI = 1;

            if (m_frame.size())
            {
                m_flags.isEOI = 0;
                m_flags.isSOS = 0;

                m_mediaData.SetBufferPointer(&m_frame[0], m_frame.size());
                m_mediaData.SetDataSize(m_frame.size());
                m_mediaData.SetFlags(in != nullptr ? in->GetFlags() : 0);
                return &m_mediaData;
            }
        }

        if(marker == JM_NONE &&
           (in == nullptr || !(in->GetFlags() & MediaData::FLAG_VIDEO_DATA_NOT_FULL_FRAME)) &&
           m_flags.isSOS == 1)
        {
            if (m_frame.size())
            {
                m_flags.isEOI = 0;
                m_flags.isSOS = 0;

                m_mediaData.SetBufferPointer(&m_frame[0], m_frame.size());
                m_mediaData.SetDataSize(m_frame.size());
                m_mediaData.SetFlags(in != nullptr ? in->GetFlags() : 0);
                return &m_mediaData;
            }
        }
    }

    return 0;
}

} // end namespace UMC


#endif // MFX_ENABLE_MJPEG_VIDEO_DECODE
