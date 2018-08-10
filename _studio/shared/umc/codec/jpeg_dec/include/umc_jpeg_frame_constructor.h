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

#ifndef __UMC_JPEG_FRAME_CONSTRUCTOR_H
#define __UMC_JPEG_FRAME_CONSTRUCTOR_H

#include "umc_defs.h"
#if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE)

#include <vector>
#include "umc_media_data_ex.h"

namespace UMC
{

class JpegFrameConstructor
{
public:

    JpegFrameConstructor();

    virtual ~JpegFrameConstructor();

    virtual void Init();

    virtual void Reset();

    virtual void Close();

    virtual MediaDataEx * GetFrame(MediaData * in, uint32_t maxBitstreamSize);

    int32_t CheckMarker(MediaData * pSource);

    int32_t GetMarker(MediaData * in, MediaData * pDst);

protected:

    Status AddMarker(uint32_t marker, MediaDataEx::_MediaDataEx* pMediaDataEx, size_t nBufferSize, MediaData *dst);
    int32_t EndOfStream(MediaData * pDst);

    int32_t FindMarkerCode(uint8_t * (&source), size_t & size, int32_t & startCodeSize);

    void ResetForNewFrame();

    size_t              m_prevLengthOfSegment;
    std::vector<uint8_t>  m_prev;
    std::vector<uint8_t>  m_frame;
    MediaDataEx         m_mediaData;
    MediaDataEx::_MediaDataEx m_mediaDataEx;

    int32_t              m_code; // marker code
    double              m_pts;
    size_t              m_suggestedSize;
    int32_t              m_RestartCount;

    struct JpegFCFlags
    {
        uint8_t isSOI : 1;
        uint8_t isEOI : 1;
        uint8_t isSOS : 1;
    };

    JpegFCFlags       m_flags;
};

} // end namespace UMC

#endif // MFX_ENABLE_MJPEG_VIDEO_DECODE
#endif //__UMC_JPEG_FRAME_CONSTRUCTOR_H
