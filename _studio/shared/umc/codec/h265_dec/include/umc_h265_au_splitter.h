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
#ifdef MFX_ENABLE_H265_VIDEO_DECODE

#ifndef __UMC_H265_AU_SPLITTER_H
#define __UMC_H265_AU_SPLITTER_H

#include <vector>
#include "umc_h265_dec_defs.h"
#include "umc_media_data_ex.h"
#include "umc_h265_heap.h"
#include "umc_h265_headers.h"
#include "umc_video_decoder.h"

namespace UMC_HEVC_DECODER
{

class NALUnitSplitter_H265;

// NAL unit splitter wrapper class
class AU_Splitter_H265
{
public:
    AU_Splitter_H265();
    virtual ~AU_Splitter_H265();

    void Init(UMC::VideoDecoderParams *init);
    void Close();

    void Reset();

    // Wrapper for NAL unit splitter CheckNalUnitType
    int32_t CheckNalUnitType(UMC::MediaData * pSource);

    // Wrapper for NAL unit splitter CheckNalUnitType GetNalUnit
    UMC::MediaDataEx * GetNalUnit(UMC::MediaData * src);
    // Returns internal NAL unit splitter
    NALUnitSplitter_H265 * GetNalUnitSplitter();

protected:

    Heap_Objects    m_ObjHeap;
    Headers         m_Headers;

protected:

    std::unique_ptr<NALUnitSplitter_H265> m_pNALSplitter;
};

} // namespace UMC_HEVC_DECODER

#endif // __UMC_H265_AU_SPLITTER_H
#endif // MFX_ENABLE_H265_VIDEO_DECODE
