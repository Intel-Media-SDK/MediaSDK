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

#ifndef __JPEGDEC_H__
#define __JPEGDEC_H__

#include "umc_defs.h"
#if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE)
#include "jpegdec_base.h"

class CBaseStreamInput;

class CJPEGDecoder : public CJPEGDecoderBase
{
public:

  CJPEGDecoder(void);
  virtual ~CJPEGDecoder(void);

  virtual void Reset(void);

  virtual JERRCODE ReadHeader(
    int*     width,
    int*     height,
    int*     nchannels,
    JCOLOR*  color,
    JSS*     sampling,
    int*     precision);

  JERRCODE SetDestination(
    uint8_t*   pDst,
    int      dstStep,
    mfxSize dstSize,
    int      dstChannels,
    JCOLOR   dstColor,
    JSS      dstSampling = JS_444,
    int      dstPrecision = 8,
    JDD      dstDctScale = JD_1_1);

  JERRCODE SetDestination(
    int16_t*  pDst,
    int      dstStep,
    mfxSize dstSize,
    int      dstChannels,
    JCOLOR   dstColor,
    JSS      dstSampling = JS_444,
    int      dstPrecision = 16);

  JERRCODE SetDestination(
    uint8_t*   pDst[4],
    int      dstStep[4],
    mfxSize dstSize,
    int      dstChannels,
    JCOLOR   dstColor,
    JSS      dstSampling = JS_420,
    int      dstPrecision = 8,
    JDD      dstDctScale = JD_1_1);

  JERRCODE SetDestination(
    int16_t*  pDst[4],
    int      dstStep[4],
    mfxSize dstSize,
    int      dstChannels,
    JCOLOR   dstColor,
    JSS      dstSampling = JS_444,
    int      dstPrecision = 16);

  JERRCODE ReadPictureHeaders(void);
  // Read the whole image data
  JERRCODE ReadData(void);
  // Read only VLC NAL data unit. Don't you mind my using h264 slang ? :)
  JERRCODE ReadData(uint32_t restartNum, uint32_t restartsToDecode);

  void SetInColor(JCOLOR color)        { m_jpeg_color = color; }
  void SetDCTType(int dct_type)        { m_use_qdct = dct_type; }
  void Comment(uint8_t** buf, int* size) { *buf = m_jpeg_comment; *size = m_jpeg_comment_size; }

  JMODE Mode(void)                     { return m_jpeg_mode; }

  int    IsJPEGCommentDetected(void)   { return m_jpeg_comment_detected; }
  int    IsExifAPP1Detected(void)      { return m_exif_app1_detected; }
  uint8_t* GetExifAPP1Data(void)         { return m_exif_app1_data; }
  int    GetExifAPP1DataSize(void)     { return m_exif_app1_data_size; }

  int    IsAVI1APP0Detected(void)      { return m_avi1_app0_detected; }
  int    GetAVI1APP0Polarity(void)     { return m_avi1_app0_polarity; }

public:
  int       m_jpeg_quality;

  JDD       m_jpeg_dct_scale;
  int       m_dd_factor;
  int       m_use_qdct;

  // JPEG embedded comments variables
  int      m_jpeg_comment_detected;
  int      m_jpeg_comment_size;
  uint8_t*   m_jpeg_comment;

  // Exif APP1 related variables
  int      m_exif_app1_detected;
  int      m_exif_app1_data_size;
  uint8_t*   m_exif_app1_data;

  uint32_t   m_numxMCU;
  uint32_t   m_numyMCU;
  int      m_mcuWidth;
  int      m_mcuHeight;
  int      m_ccWidth;
  int      m_ccHeight;
  int      m_xPadding;
  int      m_yPadding;
  int      m_rst_go;
  // Number of MCU already decoded
  uint32_t   m_mcu_decoded;
  // Number of MCU remain in the current VLC unit
  uint32_t   m_mcu_to_decode;
  int      m_restarts_to_go;
  int      m_next_restart_num;
  int      m_dc_scan_completed;
  int      m_ac_scans_completed;
  int      m_init_done;

  int16_t*  m_block_buffer;
  int       m_block_buffer_size;
  int      m_num_threads;
  int      m_sof_find;


  IMAGE                       m_dst;
  CJPEGDecoderHuffmanState    m_state;

public:
  JERRCODE Init(void);
  virtual JERRCODE Clean(void);
  JERRCODE ColorConvert(uint32_t rowCMU, uint32_t colMCU, uint32_t maxMCU);
  JERRCODE UpSampling(uint32_t rowMCU, uint32_t colMCU, uint32_t maxMCU);

  JERRCODE FindNextImage();
  JERRCODE ParseData();
  virtual JERRCODE ParseJPEGBitStream(JOPERATION op);
  JERRCODE ParseAPP1(void);
  JERRCODE ParseSOF1(void);
  JERRCODE ParseSOF2(void);
  JERRCODE ParseSOF3(void);
  JERRCODE ParseRST(void);
  JERRCODE ParseCOM(void);

  JERRCODE DecodeScanBaseline(void);     // interleaved / non-interleaved scans
  JERRCODE DecodeScanBaselineIN(void);   // interleaved scan
  JERRCODE DecodeScanBaselineIN_P(void); // interleaved scan for plane image
  JERRCODE DecodeScanBaselineNI(void);   // non-interleaved scan
  JERRCODE DecodeScanLosslessIN(void);
  JERRCODE DecodeScanLosslessNI(void);
  JERRCODE DecodeScanProgressive(void);

  JERRCODE ProcessRestart(void);

  // huffman decode mcu row lossless process
  JERRCODE DecodeHuffmanMCURowLS(int16_t* pMCUBuf);

  // huffman decode mcu row baseline process
  JERRCODE DecodeHuffmanMCURowBL(int16_t* pMCUBuf, uint32_t colMCU, uint32_t maxMCU);

  // inverse DCT, de-quantization, level-shift for mcu row
  JERRCODE ReconstructMCURowBL8x8_NxN(int16_t* pMCUBuf, uint32_t colMCU, uint32_t maxMCU);
  JERRCODE ReconstructMCURowBL8x8(int16_t* pMCUBuf, uint32_t colMCU, uint32_t maxMCU);
  JERRCODE ReconstructMCURowBL8x8To4x4(int16_t* pMCUBuf, uint32_t colMCU, uint32_t maxMCU);
  JERRCODE ReconstructMCURowBL8x8To2x2(int16_t* pMCUBuf, uint32_t colMCU, uint32_t maxMCU);
  JERRCODE ReconstructMCURowBL8x8To1x1(int16_t* pMCUBuf, uint32_t colMCU, uint32_t maxMCU);
  JERRCODE ReconstructMCURowEX(int16_t* pMCUBuf, uint32_t colMCU, uint32_t maxMCU);

  JERRCODE ProcessBuffer(int nMCURow, int thread_id = 0);
  // reconstruct mcu row lossless process
  JERRCODE ReconstructMCURowLS(int16_t* pMCUBuf, int nMCURow,int thread_id = 0);

  ChromaType GetChromaType();
};

#endif // MFX_ENABLE_MJPEG_VIDEO_DECODE
#endif // __JPEGDEC_H__
