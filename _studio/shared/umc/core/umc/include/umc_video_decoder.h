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

#ifndef __UMC_VIDEO_DECODER_H__
#define __UMC_VIDEO_DECODER_H__

#include "umc_structures.h"
#include "umc_video_data.h"
#include "umc_base_codec.h"
#include "umc_base_color_space_converter.h"

namespace UMC
{

class VideoAccelerator;

class VideoDecoderParams : public BaseCodecParams
{
    DYNAMIC_CAST_DECL(VideoDecoderParams, BaseCodecParams)

public:
    // Default constructor
    VideoDecoderParams();
    // Destructor
    virtual ~VideoDecoderParams();

    VideoStreamInfo         info;                           // (VideoStreamInfo) compressed video info
    uint32_t                  lFlags;                         // (uint32_t) decoding flag(s)

    BaseCodec               *pPostProcessing;               // (BaseCodec*) pointer to post processing

    VideoAccelerator        *pVideoAccelerator;             // pointer to video accelerator
};

/******************************************************************************/

class VideoDecoder : public BaseCodec
{
    DYNAMIC_CAST_DECL(VideoDecoder, BaseCodec)

public:
    VideoDecoder(void) :
        m_PostProcessing(NULL),
        m_allocatedPostProcessing(NULL)
    {}

    // Destructor
    virtual ~VideoDecoder(void);

    // BaseCodec methods
    // Get codec working (initialization) parameter(s)
    virtual Status GetInfo(BaseCodecParams *info);
    // Set new working parameter(s)
    virtual Status SetParams(BaseCodecParams *params);

    // Additional methods
    // Reset skip frame counter
    virtual Status ResetSkipCount() = 0;
    // Increment skip frame counter
    virtual Status SkipVideoFrame(int32_t) = 0;
    // Get skip frame counter statistic
    virtual uint32_t GetNumOfSkippedFrames() = 0;

    // returns closed capture data
    virtual Status GetUserData(MediaData* /*pCC*/)
    {
        return UMC_ERR_NOT_IMPLEMENTED;
    }

protected:

    VideoStreamInfo         m_ClipInfo;                         // (VideoStreamInfo) clip info
    BaseCodec               *m_PostProcessing;                  // (BaseCodec*) pointer to post processing
    BaseCodec               *m_allocatedPostProcessing;         // (BaseCodec*) pointer to default post processing allocated by decoder

private:
    // Declare private copy constructor to avoid accidental assignment
    // and klocwork complaining.
    VideoDecoder(const VideoDecoder &);
    VideoDecoder & operator = (const VideoDecoder &);
};

} // end namespace UMC

#endif // __UMC_VIDEO_DECODER_H__
