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

#ifndef __UMC_VIDEO_PROCESSING_H__
#define __UMC_VIDEO_PROCESSING_H__

#include "umc_video_data.h"
#include "umc_base_codec.h"

namespace UMC
{

BaseCodec *createVideoProcessing();

enum DeinterlacingMethod
{
    NO_DEINTERLACING        = 0,
    DEINTERLACING_BLEND     = 1,
    DEINTERLACING_DUPLICATE = 2,
    DEINTERLACING_SPATIAL   = 3,
    DEINTERLACING_ADAPTIVE  = 4,
    DEINTERLACING_SMOOTH    = 5
};

//////////////////////////////////////////////////////////////////////

class VideoProcessingParams : public BaseCodecParams
{
  DYNAMIC_CAST_DECL(VideoProcessingParams, BaseCodecParams)
public:
  uint32_t    InterpolationMethod; // interpolation method to perform image resampling (see ippi.h)
  DeinterlacingMethod m_DeinterlacingMethod;  // deinterlacing method
  UMC::sRECT   SrcCropArea;         // source crop region (zero region means full frame)

  VideoProcessingParams()
  {
    InterpolationMethod = 0;
    m_DeinterlacingMethod = NO_DEINTERLACING; //DEINTERLACING_BLEND;//DEINTERLACING_DUPLICATE;
    /* KW fix*/
    //memset(&SrcCropArea, 0, sizeof(SrcCropArea));
    SrcCropArea.bottom = 0;
    SrcCropArea.left = 0;
    SrcCropArea.right = 0;
    SrcCropArea.top = 0;
  }
};

//////////////////////////////////////////////////////////////////////

// auxiliary class
class VideoDataExt : public VideoData
{
  DYNAMIC_CAST_DECL(VideoDataExt, VideoData)
public:
  Status Crop(UMC::sRECT SrcCropArea);
  Status Convert_YV12_To_YUV420();
};

//////////////////////////////////////////////////////////////////////

#define MAX_NUM_FILTERS  8

class VideoProcessing : public BaseCodec
{
  DYNAMIC_CAST_DECL(VideoProcessing, BaseCodec)
public:
  VideoProcessing();
  virtual ~VideoProcessing();

  virtual Status Init(BaseCodecParams *init);

  virtual Status AddFilter(BaseCodec *filter, int atEnd);

  virtual Status GetFrame(MediaData *input, MediaData *output);

  virtual Status GetInfo(BaseCodecParams *info);

  virtual Status Close(void);

  virtual Status Reset(void);

  virtual Status SetParams(BaseCodecParams *params);

protected:
  VideoProcessingParams   Param;
  int numFilters;
  int iD3DProcessing;
  int iDeinterlacing;
  int iColorConv;
  int iColorConv0;
  int iResizing;
  bool bSrcCropArea;
  BaseCodec *pFilter[MAX_NUM_FILTERS];
  uint8_t bFiltering[MAX_NUM_FILTERS];
  VideoData tmpData[MAX_NUM_FILTERS];
  VideoDataExt tmp_in[1];
  VideoDataExt tmp_out[1];
};

} // namespace UMC

#endif /* __UMC_VIDEO_PROCESSING_H__ */
