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

#ifndef __UMC_H264_SEGMENT_DECODER_BASE_H
#define __UMC_H264_SEGMENT_DECODER_BASE_H

#include "umc_h264_dec.h"

namespace UMC
{

class H264Slice;
class TaskBroker;

class H264SegmentDecoderBase
{
public:
    H264SegmentDecoderBase(TaskBroker * pTaskBroker)
        : m_iNumber(0)
        , m_pTaskBroker(pTaskBroker)
    {
    }

    virtual ~H264SegmentDecoderBase()
    {
    }

    virtual UMC::Status Init(int32_t iNumber)
    {
        m_iNumber = iNumber;
        return UMC::UMC_OK;
    }

    // Decode slice's segment
    virtual UMC::Status ProcessSegment(void) = 0;

    virtual void RestoreErrorRect(int32_t , int32_t , H264Slice * )
    {
    }

protected:
    int32_t m_iNumber;                                           // (int32_t) ordinal number of decoder
    TaskBroker * m_pTaskBroker;
};

} // namespace UMC


#endif /* __UMC_H264_SEGMENT_DECODER_BASE_H */
#endif // MFX_ENABLE_H264_VIDEO_DECODE
