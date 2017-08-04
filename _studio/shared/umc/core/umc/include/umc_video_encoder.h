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

#ifndef __UMC_VIDEO_ENCODER_H__
#define __UMC_VIDEO_ENCODER_H__

#include "umc_base_codec.h"

namespace UMC
{

class VideoEncoderParams : public BaseCodecParams
{
    DYNAMIC_CAST_DECL(VideoEncoderParams, BaseCodecParams)
public:
    // Constructor
    VideoEncoderParams() :
        numEncodedFrames(0),
        qualityMeasure(51)
    {
      info.clip_info.width     = 0;
      info.clip_info.height    = 0;
      info.color_format        = YUV420;
      info.bitrate             = 0;
      info.aspect_ratio_width  = 1;
      info.aspect_ratio_height = 1;
      info.framerate           = 30;
      info.duration            = 0;
      info.interlace_type      = PROGRESSIVE;
      info.stream_type         = UNDEF_VIDEO;
      info.stream_subtype      = UNDEF_VIDEO_SUBTYPE;
      info.streamPID           = 0;
    }
    // Destructor
    virtual ~VideoEncoderParams(void){}
    // Read parameter from file
    virtual Status ReadParamFile(const vm_char * /*ParFileName*/)
    {
      return UMC_ERR_NOT_IMPLEMENTED;
    }

    VideoStreamInfo info;               // (VideoStreamInfo) compressed video info
    int32_t          numEncodedFrames;   // (int32_t) number of encoded frames

    // additional controls
    int32_t qualityMeasure;      // per cent, represent quantization precision
};

/******************************************************************************/

class VideoEncoder : public BaseCodec
{
    DYNAMIC_CAST_DECL(VideoEncoder, BaseCodec)
public:
    // Destructor
    virtual ~VideoEncoder() {};
};

/******************************************************************************/

} // end namespace UMC

#endif /* __UMC_VIDEO_ENCODER_H__ */
