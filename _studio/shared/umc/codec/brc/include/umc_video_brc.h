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

#ifndef _UMC_VIDEO_BRC_H_
#define _UMC_VIDEO_BRC_H_

#include "umc_video_encoder.h"

namespace UMC
{

enum eBrcPictureFlags
{
  BRC_TOP_FIELD = 1,
  BRC_BOTTOM_FIELD = 2,
  BRC_FRAME = BRC_TOP_FIELD | BRC_BOTTOM_FIELD,
  BRC_REPEAT_FIRST_FIELD = 4,
  BRC_TOP_FIELD_FIRST = 8,
  BRC_SECOND_FIELD = 12
};

typedef int32_t BrcPictureFlags;

#define BRC_BYTES_IN_KBYTE 1000
#define BRC_BITS_IN_KBIT 1000

enum BRCMethod
{
  BRC_CBR = 0,
  BRC_VBR,
  BRC_AVBR,
};

enum BRCAlgorithm
{
  BRC_COMMON = -1,
  BRC_H264,
  BRC_MPEG2,
  BRC_SVC
};

enum eBRCStatus
{
  BRC_ERROR                   = -1,
  BRC_OK                      = 0x0,
  BRC_ERR_BIG_FRAME           = 0x1,
  BRC_BIG_FRAME               = 0x2,
  BRC_ERR_SMALL_FRAME         = 0x4,
  BRC_SMALL_FRAME             = 0x8,
  BRC_NOT_ENOUGH_BUFFER       = 0x10
};

enum eBRCRecode
{
  BRC_RECODE_NONE           = 0,
  BRC_RECODE_QP             = 1,
  BRC_RECODE_PANIC          = 2,
  BRC_RECODE_EXT_QP         = 3,
  BRC_RECODE_EXT_PANIC      = 4,
  BRC_EXT_FRAMESKIP         = 16
};

typedef int32_t BRCStatus;

class VideoBrcParams : public VideoEncoderParams {
  DYNAMIC_CAST_DECL(VideoBrcParams, VideoEncoderParams)
public:
  VideoBrcParams();

  int32_t  HRDInitialDelayBytes;
  int32_t  HRDBufferSizeBytes;

  int32_t  targetBitrate;
  int32_t  maxBitrate;
  int32_t  BRCMode;

  int32_t GOPPicSize;
  int32_t GOPRefDist;
  uint32_t frameRateExtD;
  uint32_t frameRateExtN;
  uint32_t frameRateExtN_1;

  uint16_t accuracy;
  uint16_t convergence;
};

class VideoBrc {

public:

  VideoBrc();
  virtual ~VideoBrc();

  // Initialize with specified parameter(s)
  virtual Status Init(BaseCodecParams *init, int32_t numTempLayers = 1) = 0;

  // Close all resources
  virtual Status Close() = 0;

  virtual Status Reset(BaseCodecParams *init, int32_t numTempLayers = 1) = 0;

  virtual Status SetParams(BaseCodecParams* params, int32_t tid = 0) = 0;
  virtual Status GetParams(BaseCodecParams* params, int32_t tid = 0) = 0;
  virtual Status GetHRDBufferFullness(double *hrdBufFullness, int32_t recode = 0, int32_t tid = 0) = 0;
  virtual Status PreEncFrame(FrameType frameType, int32_t recode = 0, int32_t tid = 0) = 0;
  virtual BRCStatus PostPackFrame(FrameType picType, int32_t bitsEncodedFrame, int32_t payloadBits = 0, int32_t recode = 0, int32_t poc = 0) = 0;

  virtual int32_t GetQP(FrameType frameType, int32_t tid = -1) = 0;
  virtual Status SetQP(int32_t qp, FrameType frameType, int32_t tid = 0) = 0;

//  virtual Status ScaleRemovalDelay(double removalDelayScale) = 0;
  virtual Status SetPictureFlags(FrameType frameType, int32_t picture_structure, int32_t repeat_first_field = 0, int32_t top_field_first = 0, int32_t second_field = 0);

  virtual Status GetMinMaxFrameSize(int32_t *minFrameSizeInBits, int32_t *maxFrameSizeInBits) = 0;

  static Status CheckCorrectParams_MPEG2(VideoBrcParams *inBrcParams, VideoBrcParams *outBrcParams = NULL);

  virtual Status GetInitialCPBRemovalDelay(uint32_t *initial_cpb_removal_delay, int32_t recode = 0);

protected:
  //VideoBrc *brc;
  VideoBrcParams mParams;
};

} // namespace UMC
#endif

