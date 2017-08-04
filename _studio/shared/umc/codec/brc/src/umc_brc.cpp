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

#include "umc_brc.h"

namespace UMC
{

CommonBRC::CommonBRC()
{
  mIsInit = 0;
  mBitrate = 0;
  mFramerate = 30;
  mRCMode = BRC_CBR;
  mGOPPicSize = 20;
  mGOPRefDist = 1;
  memset(&mHRD, 0, sizeof(mHRD));
  N_B_frame = 0;
  N_B = 0;
  N_P = 0;
  N_P_field = 0;
  N_B_field = 0;
  mQuantUpdated = 0;
  mBitsDesiredFrame = 0;
  N_P_frame = 0;
  brc = NULL;
  mFrameType = NONE_PICTURE;
}

CommonBRC::~CommonBRC()
{
  Close();
}

Status CommonBRC::InitHRD()
{
  int32_t bitsPerFrame;
  if (mFramerate <= 0)
    return UMC_ERR_INVALID_PARAMS;

  bitsPerFrame = (int32_t)(mBitrate / mFramerate);

  if (BRC_CBR == mRCMode)
    mParams.maxBitrate = mParams.targetBitrate;
  if (mParams.maxBitrate < mParams.targetBitrate)
    mParams.maxBitrate = 1200 * 240000; // max for H.264 level 5.1, profile <= EXTENDED

  if (mParams.HRDBufferSizeBytes <= 0)
    mParams.HRDBufferSizeBytes = MFX_MAX_32S/2/8;
  if (mParams.HRDBufferSizeBytes * 8 < (bitsPerFrame << 1))
    mParams.HRDBufferSizeBytes = (bitsPerFrame << 1) / 8;

  if (mParams.HRDInitialDelayBytes <= 0)
    mParams.HRDInitialDelayBytes = (BRC_CBR == mRCMode ? mParams.HRDBufferSizeBytes/2 : mParams.HRDBufferSizeBytes);
  if (mParams.HRDInitialDelayBytes * 8 < bitsPerFrame)
    mParams.HRDInitialDelayBytes = bitsPerFrame / 8;
  if (mParams.HRDInitialDelayBytes > mParams.HRDBufferSizeBytes)
    mParams.HRDInitialDelayBytes = mParams.HRDBufferSizeBytes;

  mHRD.bufSize = mParams.HRDBufferSizeBytes * 8;
  mHRD.maxBitrate = mParams.maxBitrate;
  mHRD.bufFullness = mParams.HRDInitialDelayBytes * 8;
  mHRD.inputBitsPerFrame = mHRD.maxInputBitsPerFrame = mHRD.maxBitrate / mFramerate;

  mHRD.underflowQuant = -1;
  mHRD.frameNum = 0;

  return UMC_OK;
}

Status CommonBRC::Init(BaseCodecParams *params, int32_t)
{
  Status status = UMC_OK;
  VideoEncoderParams *videoParams = DynamicCast<VideoEncoderParams>(params);
  VideoBrcParams *brcParams = DynamicCast<VideoBrcParams>(params);

  if (videoParams != 0) {
    if (videoParams->info.clip_info.width <= 0 || videoParams->info.clip_info.height <= 0) {
      return UMC_ERR_INVALID_PARAMS;
    }
    if (NULL != brcParams) { // BRC parameters
      mParams = *brcParams;

      mBitrate = (uint32_t)mParams.targetBitrate;

      if (mParams.frameRateExtN && mParams.frameRateExtD)
          mFramerate = (double) mParams.frameRateExtN /  mParams.frameRateExtD;
      else
          mFramerate = mParams.info.framerate;

      mRCMode = mParams.BRCMode;

      if (mParams.GOPPicSize > 0)
        mGOPPicSize = mParams.GOPPicSize;
      else
        mParams.GOPPicSize = mGOPPicSize;
      if (mParams.GOPRefDist > 0 && mParams.GOPRefDist < mParams.GOPPicSize)
        mGOPRefDist = mParams.GOPRefDist;
      else
        mParams.GOPRefDist = mGOPRefDist;
    } else {
      *(VideoEncoderParams*) &mParams = *videoParams;
      mBitrate = mParams.info.bitrate;
      mFramerate = mParams.info.framerate;
    }
  } else if (params != 0) {
    return UMC_ERR_INVALID_PARAMS;
//    *(BaseCodecParams*)&mParams = *params;
  } else {
    if (!mIsInit)
      return UMC_ERR_NULL_PTR;
    else {
      // reset
      return UMC_OK;
    }
  }

  if (mBitrate <= 0 || mFramerate <= 0)
    return UMC_ERR_INVALID_PARAMS;
  mBitsDesiredFrame = (int32_t)((double)mBitrate / mFramerate);
  if (mBitsDesiredFrame <= 0)
    return UMC_ERR_INVALID_PARAMS;
  mIsInit = true;
  return status;
}


Status CommonBRC::GetParams(BaseCodecParams* params, int32_t)
{
  VideoBrcParams *brcParams = DynamicCast<VideoBrcParams>(params);
  VideoEncoderParams *videoParams = DynamicCast<VideoEncoderParams>(params);
  if (NULL != brcParams) {
    *brcParams = mParams;
  } else if (NULL != videoParams) {
    *params = *(VideoEncoderParams*)&mParams;
  } else {
    *params = *(BaseCodecParams*)&mParams;
  }
  return UMC_OK;
};

Status CommonBRC::Close()
{
  Status status = UMC_OK;
  if (!mIsInit)
    return UMC_ERR_NOT_INITIALIZED;
  mIsInit = false;
  return status;
}


Status CommonBRC::PreEncFrame(FrameType, int32_t, int32_t)
{
  return UMC_OK;
}

Status CommonBRC::PostPackFrame(FrameType, int32_t, int32_t, int32_t, int32_t)
{
  return UMC_OK;
}

int32_t CommonBRC::GetQP(FrameType, int32_t)
{
  return UMC_OK;
}

Status CommonBRC::SetQP(int32_t, FrameType, int32_t)
{
  return UMC_OK;
}

BRCStatus CommonBRC::UpdateAndCheckHRD(int32_t frameBits, int32_t recode)
{
  BRCStatus ret = BRC_OK;
  double bufFullness;

  if (!(recode & (BRC_EXT_FRAMESKIP - 1))) { // BRC_EXT_FRAMESKIP == 16
    mHRD.prevBufFullness = mHRD.bufFullness;
    mHRD.underflowQuant = -1;
  } else { // frame is being recoded - restore buffer state
    mHRD.bufFullness = mHRD.prevBufFullness;
  }

  mHRD.maxFrameSize = (int32_t)(mHRD.bufFullness - mHRD.roundError);
  mHRD.minFrameSize = (mRCMode == BRC_VBR ? 0 : (int32_t)(mHRD.bufFullness + 1 + mHRD.roundError + mHRD.inputBitsPerFrame - mHRD.bufSize));
  if (mHRD.minFrameSize < 0)
    mHRD.minFrameSize = 0;

  bufFullness = mHRD.bufFullness - frameBits;

  if (bufFullness < 1 + mHRD.roundError) {
    bufFullness = mHRD.inputBitsPerFrame;
    ret = BRC_ERR_BIG_FRAME;
    if (bufFullness > (double)mHRD.bufSize) // possible in VBR mode if at all (???)
      bufFullness = (double)mHRD.bufSize;
  } else {
    bufFullness += mHRD.inputBitsPerFrame;
    if (bufFullness > (double)mHRD.bufSize - mHRD.roundError) {
      bufFullness = (double)mHRD.bufSize - mHRD.roundError;
      if (mRCMode != BRC_VBR)
        ret = BRC_ERR_SMALL_FRAME;
    }
  }
  if (BRC_OK == ret)
    mHRD.frameNum++;
  else if ((recode & BRC_EXT_FRAMESKIP) || BRC_RECODE_EXT_PANIC == recode || BRC_RECODE_PANIC == recode) // no use in changing QP
    ret |= BRC_NOT_ENOUGH_BUFFER;

  mHRD.bufFullness = bufFullness;

  return ret;
}

/*
Status CommonBRC::ScaleRemovalDelay(double removalDelayScale)
{
  if (removalDelayScale <= 0)
    return UMC_ERR_INVALID_PARAMS;

//  mHRD.maxInputBitsPerFrame = (uint32_t)((mHRD.maxBitrate / mFramerate) * removalDelayScale); // == mHRD.inputBitsPerFrame for CBR
  mHRD.inputBitsPerFrame = mHRD.maxInputBitsPerFrame * removalDelayScale; //???

  return UMC_OK;
}
*/

Status CommonBRC::GetHRDBufferFullness(double *hrdBufFullness, int32_t recode, int32_t)
{
  *hrdBufFullness = (recode & (BRC_EXT_FRAMESKIP - 1)) ? mHRD.prevBufFullness : mHRD.bufFullness;

  return UMC_OK;
}

void CommonBRC::SetGOP()
{
  if (mGOPRefDist < 1) {
    N_P = 0;
    N_B = 0;
  } else if (mGOPRefDist == 1) {
    N_P = mGOPPicSize - 1;
    N_B = 0;
  } else {
    N_P = (mGOPPicSize/mGOPRefDist) - 1;
    N_B = (mGOPRefDist - 1) * (mGOPPicSize/mGOPRefDist);
  }
  N_P_frame = N_P;
  N_B_frame = N_B;

  N_P_field = N_P_frame * 2 + 1; // 2nd field of I is coded as P
  N_B_field = N_B_frame * 2;
}

Status CommonBRC::GetInitialCPBRemovalDelay(uint32_t*, int32_t)
{
  return UMC_OK;
}

}

