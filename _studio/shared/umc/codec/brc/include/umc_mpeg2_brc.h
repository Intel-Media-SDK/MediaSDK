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

#ifndef _UMC_MPEG2_BRC_H_
#define _UMC_MPEG2_BRC_H_

#include "umc_video_encoder.h"
#include "umc_brc.h"


#include "mfxdefs.h"

#define BRC_MIN(a, b) ((a) < (b) ? (a) : (b))
#define BRC_MAX(a, b) ((a) > (b) ? (a) : (b))

#define N_INST_RATE_THRESHLDS 4
#define N_QUALITY_LEVELS 7
#define N_QP_THRESHLDS 4
#define N_DEV_THRESHLDS 8
#define N_VAR_THRESHLDS 7


namespace UMC
{

class MPEG2BRC : public CommonBRC {

public:

  MPEG2BRC();
  ~MPEG2BRC();

  // Initialize with specified parameter(s)
  virtual Status Init(BaseCodecParams *init, int32_t nl = 1);

  // Close all resources
  virtual Status Close();

  virtual Status Reset(BaseCodecParams *init, int32_t nl = 1);

  virtual Status SetParams(BaseCodecParams* params, int32_t tid = 0);

  virtual Status PreEncFrame(FrameType frameType, int32_t recode = 0, int32_t tid = 0);
  virtual BRCStatus PostPackFrame(FrameType picType, int32_t bitsEncodedFrame, int32_t payloadBits = 0, int32_t recode = 0, int32_t = 0);

  virtual int32_t GetQP(FrameType frameType, int32_t tid = 0);
  virtual Status SetQP(int32_t qp, FrameType frameType, int32_t tid = 0);
  virtual Status SetPictureFlags(FrameType frameType, int32_t picture_structure, int32_t repeat_first_field, int32_t top_field_first, int32_t second_field);

//  virtual Status Query(UMCExtBuffer *pStat, uint32_t *numEntries);

protected:

  bool   mIsInit;
  double      rc_weight[3];          // frame weight (length proportion)
  double      rc_tagsize[3];         // bitsize target of the type
  double      rc_tagsize_frame[3];   // bitsize target of the type
  double      rc_tagsize_field[3];   // bitsize target of the type
  double      rc_dev;                // bitrate deviation (sum of GOP's frame diffs)
  double      rc_dev_saved;          // bitrate deviation (sum of GOP's frame diffs)
  double      gopw;
  int32_t      block_count;
  int32_t      qscale[3];             // qscale codes for 3 frame types (int32_t!)
  int32_t      prsize[3];             // bitsize of previous frame of the type
  int32_t      prqscale[3];           // quant scale value, used with previous frame of the type
  BrcPictureFlags      prpicture_flags[3];
  BrcPictureFlags      picture_flags, picture_flags_prev, picture_flags_IP;
  int32_t  quantiser_scale_value;
  int32_t  quantiser_scale_code;
  int32_t  q_scale_type;
  bool  full_hw;
//  int32_t  mBitsDesiredFrame;

  int32_t GetInitQP();
  BRCStatus UpdateQuantHRD(int32_t bEncoded, BRCStatus sts);
  Status CheckHRDParams();
//  Status CalculatePicTargets();
  int32_t ChangeQuant(int32_t quant);


  virtual Status PreEncFrameMidRange(FrameType frameType, int32_t recode = 0);
  virtual Status PreEncFrameFallBack(FrameType frameType, int32_t recode = 0);

  mfxI32  mIsFallBack;              // depending on bitrate calls PreEncFrameFallBack

  mfxI32  mQuantMax;                // max quantizer, just handy for using in different standards.
  mfxI32  mQuantMin;                // min quantizer

/*
  mfxI32  N_P_frame, N_B_frame;         // number of sP,B frames in GOP, calculated at init
  mfxI32  N_P_field, N_B_field;         // number of sP,B frames in GOP, calculated at init
  mfxI32  N_I, N_P, N_B;         // number of I,P,B frames in GOP, calculated at init
*/
  mfxI32  prev_frame_type;       // previous frame type
  mfxI32  mQualityLevel;         // quality level, from 0 to N_QUALITY_LEVELS-1, the higher - the better

  mfxF64  instant_rate_thresholds[N_INST_RATE_THRESHLDS]; // constant, calculated at init, thresholds for instatnt bitrate
  mfxF64  deviation_thresholds[N_DEV_THRESHLDS]; // constant, calculated at init, buffer fullness/deviation thresholds


};

class Mpeg2_BrcParams : public VideoEncoderParams
{
  DYNAMIC_CAST_DECL(Mpeg2_BrcParams, VideoEncoderParams)
public:
  Mpeg2_BrcParams():
        frameWidth  (0),
        frameHeight (0),
        maxFrameSize(0)
  {
        quant[0] = quant[1] = quant[2] = 0;
  }
  int32_t  frameWidth;
  int32_t  frameHeight;
  int32_t  maxFrameSize;
  int16_t  quant[3]; // I,P,B
};

class MPEG2BRC_CONST_QUNT : public CommonBRC {

public:

    MPEG2BRC_CONST_QUNT() {};
    ~MPEG2BRC_CONST_QUNT(){};

  // Initialize with specified parameter(s)
  virtual Status Init(BaseCodecParams *init, int32_t = 0)
  {
      Mpeg2_BrcParams *brcParams = DynamicCast<Mpeg2_BrcParams>(init);
      if (brcParams && brcParams->quant[0]>0 && brcParams->frameHeight > 0 && brcParams->frameWidth >0 )
      {
          MFX_INTERNAL_CPY((uint8_t*)&m_params, (uint8_t*)brcParams, sizeof(Mpeg2_BrcParams));
          m_params.frameHeight = brcParams->frameHeight;
          m_params.frameWidth  = brcParams->frameWidth;

          m_params.maxFrameSize = brcParams->frameHeight*brcParams->frameWidth * 2;
          m_params.quant[0] =  brcParams->quant[0];
          m_params.quant[1] = (brcParams->quant[1]>0) ? brcParams->quant[1]:brcParams->quant[0];
          m_params.quant[2] = (brcParams->quant[2]>0) ? brcParams->quant[2]:brcParams->quant[0];

          return UMC_OK;
      }
      return UMC_ERR_UNSUPPORTED;
  }

  // Close all resources
  virtual Status Close() {return UMC_OK;}

  virtual Status Reset(BaseCodecParams *, int32_t) {return UMC_OK;}

  virtual Status SetParams(BaseCodecParams* params, int32_t = 0)
  {
      return Init(params);
  }
  virtual Status GetParams(BaseCodecParams* params, int32_t = 0)
  {
       Mpeg2_BrcParams *brcParams = DynamicCast<Mpeg2_BrcParams>(params);
       if (brcParams)
       {
          brcParams->maxFrameSize = m_params.maxFrameSize;
          brcParams->quant[0] = m_params.quant[0];
          brcParams->quant[1] = m_params.quant[1];
          brcParams->quant[2] = m_params.quant[2];

          brcParams->frameHeight = m_params.frameHeight;
          brcParams->frameWidth  = m_params.frameWidth;
       }
       else
       {
           VideoBrcParams *videoParams = DynamicCast<VideoBrcParams>(params);
           videoParams->HRDBufferSizeBytes = m_params.maxFrameSize;
           videoParams->HRDInitialDelayBytes = 0;
           videoParams->maxBitrate = 0;
           videoParams->BRCMode = BRC_VBR;

       }
       return UMC_OK;
  }


  virtual Status PreEncFrame(FrameType /*frameType*/, int32_t /*recode*/, int32_t)
  {
      return UMC_OK;
  }
  virtual BRCStatus PostPackFrame(FrameType /*picType*/, int32_t /*bitsEncodedFrame*/, int32_t /*payloadBits*/ = 0, int32_t /*recode*/ = 0, int32_t = 0)
  {
      return UMC_OK;
  }
  virtual int32_t GetQP(FrameType frameType, int32_t = 0)
  {
      switch (frameType)
      {
      case I_PICTURE:
          return m_params.quant[0];
      case P_PICTURE:
          return m_params.quant[1];
      case B_PICTURE:
          return m_params.quant[2];
      default:
          return m_params.quant[0];
      }
  }
  virtual Status SetQP(int32_t qp, FrameType frameType, int32_t = 0)
  {
      switch (frameType)
      {
      case I_PICTURE:
          return m_params.quant[0] = (int16_t)qp;
      case P_PICTURE:
          return m_params.quant[1] = (int16_t)qp;
      case B_PICTURE:
          return m_params.quant[2] = (int16_t)qp;
      default:
           return UMC_ERR_UNSUPPORTED;
      }
  }
  virtual Status SetPictureFlags(FrameType /*frameType*/, int32_t /*picture_structure*/, int32_t /*repeat_first_field*/, int32_t /*top_field_first*/, int32_t /*second_field*/)
  {
      return UMC_OK;
  }

private:

    Mpeg2_BrcParams m_params;

};
} // namespace UMC
#endif

