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

#include <math.h>
#include "umc_video_brc.h"

namespace UMC
{

VideoBrcParams::VideoBrcParams()
{
  BRCMode = BRC_CBR;
  targetBitrate = 500000; // Kbps = 1000 bps
  maxBitrate = targetBitrate;
  HRDBufferSizeBytes = targetBitrate * 2 / 30; // ~target frame size (at 30 fps) * 16; KB = 1024 B
  HRDInitialDelayBytes = HRDBufferSizeBytes / 2;
  GOPPicSize = 20;
  GOPRefDist = 1;
  accuracy = 10;
  convergence = 500;
  frameRateExtN_1 = 0; // svc specific
  frameRateExtD = 0;
  frameRateExtN = 0;
}

VideoBrc::VideoBrc()
{
}

VideoBrc::~VideoBrc()
{
}


Status VideoBrc::CheckCorrectParams_MPEG2(VideoBrcParams *inBrcParams, VideoBrcParams *outBrcParams)
{
  double bitsPerFrame;
  Status status = UMC_OK;
  int32_t brcMode, maxBitrate, targetBitrate, bufferSizeBytes;
  uint32_t maxBufSizeBits, bufSizeBits;
  double framerate;
  int32_t initialDelayBytes;
  double bufFullness;
  double gopFactor;

  if (inBrcParams == 0)
    return UMC_ERR_NULL_PTR;

  brcMode = inBrcParams->BRCMode;
  maxBitrate = inBrcParams->maxBitrate;
  targetBitrate = inBrcParams->targetBitrate;
  bufferSizeBytes = inBrcParams->HRDBufferSizeBytes;
  framerate = inBrcParams->info.framerate;
  bufFullness = inBrcParams->HRDInitialDelayBytes * 8;
  gopFactor = inBrcParams->GOPPicSize >= 100 ? 1 : 10 / sqrt((double)inBrcParams->GOPPicSize);

  if (framerate <= 0)
    return UMC_ERR_FAILED;


  if (outBrcParams)
    *outBrcParams = *inBrcParams;

  if (static_cast<unsigned int>(bufferSizeBytes) > MFX_MAX_32U >> 3)
    bufferSizeBytes = MFX_MAX_32U >> 3;

  bufSizeBits = bufferSizeBytes << 3;
  bitsPerFrame = targetBitrate / framerate;

  maxBufSizeBits = (uint32_t)(0xfffe * (unsigned long long)targetBitrate / 90000); // for CBR

  if (bitsPerFrame < inBrcParams->info.clip_info.width * inBrcParams->info.clip_info.height * 8 * gopFactor / (double)500) {
    status = UMC_ERR_INVALID_PARAMS;
    bitsPerFrame = inBrcParams->info.clip_info.width * inBrcParams->info.clip_info.height * 8 * gopFactor / (double)500;
    targetBitrate = (int32_t)(bitsPerFrame * framerate);
  }

  if (brcMode != BRC_CBR && brcMode != BRC_VBR) {
    status = UMC_ERR_INVALID_PARAMS;
    brcMode = BRC_VBR;
  }

  if (brcMode == BRC_VBR) {
    if (maxBitrate <= targetBitrate) {
      status = UMC_ERR_INVALID_PARAMS;
      maxBitrate = targetBitrate;
    }
    if (bufSizeBits > (uint32_t)16384 * 0x3fffe)
      bufSizeBits = (uint32_t)16384 * 0x3fffe;
  } else {
    if (bufSizeBits > maxBufSizeBits) {
      status = UMC_ERR_INVALID_PARAMS;
      if (maxBufSizeBits > (uint32_t)16384 * 0x3fffe)
        maxBufSizeBits = (uint32_t)16384 * 0x3fffe;
      if (maxBufSizeBits < (uint32_t)(bitsPerFrame + 0.5)) { // framerate is too low for CBR
        brcMode = BRC_VBR;
        maxBitrate = targetBitrate;
      }
      if (bufFullness > maxBufSizeBits/2) { // leave mHRD.bufFullness unchanged if below mHRD.bufSize/2
        double newBufFullness = bufFullness * maxBufSizeBits / bufSizeBits;
        if (newBufFullness < (double)maxBufSizeBits / 2)
          newBufFullness = (double)maxBufSizeBits / 2;
        bufFullness = newBufFullness;
      }
      bufSizeBits = maxBufSizeBits; // will be coded as (mHRD.bufSize + 16383) &~ 0x3fff
    } else {
      if (bufSizeBits > (uint32_t)16384 * 0x3fffe) {
        status = UMC_ERR_INVALID_PARAMS;
        bufSizeBits = (uint32_t)16384 * 0x3fffe;
      }
      if (bufFullness > (double)bufSizeBits) {
        status = UMC_ERR_INVALID_PARAMS;
        bufFullness = bufSizeBits;
      }
    }
  }

  if (bufSizeBits < 2 * bitsPerFrame) {
    status = UMC_ERR_INVALID_PARAMS;
    bufSizeBits = (uint32_t)(2 * bitsPerFrame);
  }

  if (brcMode == BRC_VBR)
    bufFullness = bufSizeBits; // vbv_delay = 0xffff (buffer is initially full) in case of VBR. TODO: check the possibility of VBR with vbv_delay != 0xffff

  if (bufFullness <= 0) { // not set
    status = UMC_ERR_INVALID_PARAMS;
    bufFullness = bufSizeBits / 2;
  } else if (bufFullness < bitsPerFrame) { // set but is too small
    status = UMC_ERR_INVALID_PARAMS;
    bufFullness = bitsPerFrame;
  }

  bufferSizeBytes = bufSizeBits >> 3;
  initialDelayBytes = (uint32_t)bufFullness >> 3;

  if (outBrcParams) {
    outBrcParams->BRCMode = brcMode;
    outBrcParams->targetBitrate = targetBitrate;
    if (brcMode == BRC_VBR)
      outBrcParams->maxBitrate = maxBitrate;
    outBrcParams->HRDBufferSizeBytes = bufferSizeBytes;
    outBrcParams->HRDInitialDelayBytes = initialDelayBytes;
  }

  return status;
}

Status VideoBrc::SetPictureFlags(FrameType, int32_t, int32_t, int32_t, int32_t)
{
  return UMC_OK;
}

Status VideoBrc::GetInitialCPBRemovalDelay(uint32_t *, int32_t)
{
  return UMC_OK;
}

}
