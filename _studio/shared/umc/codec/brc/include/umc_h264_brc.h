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

#ifndef _UMC_H264_BRC_H_
#define _UMC_H264_BRC_H_

#include "umc_video_encoder.h"
#include "umc_brc.h"


namespace UMC
{

class H264BRC : public CommonBRC {

public:

  H264BRC();
  ~H264BRC();

  // Initialize with specified parameter(s)
  virtual Status Init(BaseCodecParams *init, int32_t nl = 1);

  // Close all resources
  virtual Status Close();

  virtual Status Reset(BaseCodecParams *init, int32_t nl = 1);

  virtual Status SetParams(BaseCodecParams* params, int32_t tid = 0);

  virtual BRCStatus PostPackFrame(FrameType picType, int32_t bitsEncodedFrame, int32_t payloadBits, int32_t recode = 0, int32_t poc = 0);

  virtual int32_t GetQP(FrameType frameType, int32_t tid = 0);
  virtual Status SetQP(int32_t qp, FrameType frameType, int32_t tid = 0);

  virtual Status SetPictureFlags(FrameType frameType, int32_t picture_structure, int32_t repeat_first_field = 0, int32_t top_field_first = 0, int32_t second_field = 0);
//  virtual Status Query(UMCExtBuffer *pStat, uint32_t *numEntries);
  virtual Status GetInitialCPBRemovalDelay(uint32_t *initial_cpb_removal_delay, int32_t recode = 0);

protected:
  bool   mIsInit;
//  int32_t  mBitsDesiredFrame;
  long long  mBitsEncodedTotal, mBitsDesiredTotal;
  int32_t  mQuantI, mQuantP, mQuantB, mQuantMax, mQuantMin, mQuantPrev, mQuantOffset, mQPprev;
  int32_t  mRCfap, mRCqap, mRCbap, mRCq;
  double  mRCqa, mRCfa, mRCqa0;
  double  mRCfa_short;
  int32_t  mQuantIprev, mQuantPprev, mQuantBprev;
  int32_t  mBitsEncoded;
  int32_t  mBitDepth;
  BrcPictureFlags  mPictureFlags, mPictureFlagsPrev;
  int32_t mRecode;
  int32_t GetInitQP();
  BRCStatus UpdateQuant(int32_t bEncoded, int32_t totalPicBits);
  BRCStatus UpdateQuantHRD(int32_t bEncoded, BRCStatus sts, int32_t payloadBits = 0);
  Status InitHRD();
  unsigned long long mMaxBitsPerPic, mMaxBitsPerPicNot0;
  int32_t mSceneChange;
  int32_t mBitsEncodedP, mBitsEncodedPrev;
  int32_t mPoc, mSChPoc;
  uint32_t mMaxBitrate;
  long long mBF, mBFsaved;
};

} // namespace UMC
#endif

