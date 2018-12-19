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

#ifndef _UMC_BRC_H_
#define _UMC_BRC_H_

#include "umc_video_encoder.h"
#include "umc_video_brc.h"

#include "mfx_utils.h"

#define BRC_ABS(a) ((a) >= 0 ? (a) : -(a))

namespace UMC
{


typedef struct _BRC_HRDState
{
  uint32_t bufSize;
  double bufFullness;
  double prevBufFullness;
  double maxBitrate;
  double inputBitsPerFrame;
  double maxInputBitsPerFrame;
  int32_t frameNum;
  int32_t minFrameSize;
  int32_t maxFrameSize;
  int32_t underflowQuant;
  double roundError;
} BRC_HRDState;


class BrcParams : public VideoEncoderParams {
  DYNAMIC_CAST_DECL(BrcParams, VideoEncoderParams)
public:
  BrcParams();

};

class CommonBRC : public VideoBrc {

public:

  CommonBRC();
  virtual ~CommonBRC();

  // Initialize with specified parameter(s)
  virtual Status Init(BaseCodecParams *init, int32_t nl = 1);

  // Close all resources
  virtual Status Close();

  virtual Status Reset(BaseCodecParams *init, int32_t) 
  {
      return Init(init);
  };

  virtual Status SetParams(BaseCodecParams* params, int32_t)
  {
      return Init(params);
  };

  virtual Status GetParams(BaseCodecParams* params, int32_t tid = 0);

  virtual Status PreEncFrame(FrameType frameType, int32_t recode = 0, int32_t tid = 0);

//  virtual BRCStatus PostPackFrame(MediaData *inData, int32_t bitsEncodedFrame, int32_t recode = 0);
  virtual BRCStatus PostPackFrame(FrameType picType, int32_t bitsEncodedFrame, int32_t payloadBits = 0, int32_t recode = 0, int32_t poc = 0);
  virtual int32_t GetQP(FrameType picType, int32_t tid = 0);
  virtual Status SetQP(int32_t qp, FrameType picType, int32_t tid = 0);

//  virtual Status ScaleRemovalDelay(double removalDelayScale = 1.0);
  virtual Status GetHRDBufferFullness(double *hrdBufFullness, int32_t recode = 0, int32_t tid = 0);
  virtual Status GetInitialCPBRemovalDelay(uint32_t *initial_cpb_removal_delay, int32_t recode = 0);


  Status GetMinMaxFrameSize(int32_t *minFrameSizeInBits, int32_t *maxFrameSizeInBits) {
    if (minFrameSizeInBits)
      *minFrameSizeInBits = mHRD.minFrameSize;
    if (maxFrameSizeInBits)
      *maxFrameSizeInBits = mHRD.maxFrameSize;
    return UMC_OK;
  };

protected:

  void *brc; // can be a ptr to either common or codec-specific BRC (?)

  bool   mIsInit;

  Status InitHRD();
  BRCStatus UpdateAndCheckHRD(int32_t frameBits, int32_t recode);
  void SetGOP();

  int32_t mRCMode;
  uint32_t mBitrate;
  double mFramerate;
  BRC_HRDState mHRD;

  FrameType mFrameType;
  int32_t mQuantUpdated;

  int32_t mGOPPicSize;
  int32_t mGOPRefDist;

  int32_t  mBitsDesiredFrame;
  int32_t  N_P_frame, N_B_frame;         // number of sP,B frames in GOP, calculated at init
  int32_t  N_P_field, N_B_field;         // number of sP,B frames in GOP, calculated at init
  int32_t  N_P, N_B;                    // number of P,B frames in GOP, calculated at init
};

} // namespace UMC
#endif
