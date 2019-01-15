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

#ifndef __JPEGENC_H__
#define __JPEGENC_H__

#include "umc_defs.h"
#if defined (MFX_ENABLE_MJPEG_VIDEO_ENCODE)

#include "umc_defs.h"
#include "ippdefs.h"
#include "ippcore.h"
#include "ipps.h"
#include "ippi.h"
#include "ippj.h"
#include "basestreamout.h"
#include "jpegbase.h"
#include "encqtbl.h"
#include "enchtbl.h"
#include "colorcomp.h"
#include "bitstreamout.h"


class CBaseStreamOutput;

typedef struct _JPEG_SCAN
{
  int ncomp;
  int id[MAX_COMPS_PER_SCAN];
  int Ss;
  int Se;
  int Ah;
  int Al;

} JPEG_SCAN;


class CJPEGEncoder
{
public:

  CJPEGEncoder(void);
  virtual ~CJPEGEncoder(void);

  CJPEGEncoder(const CJPEGEncoder&) = delete;
  CJPEGEncoder(CJPEGEncoder&&) = delete;
  CJPEGEncoder& operator=(const CJPEGEncoder&) = delete;
  CJPEGEncoder& operator=(CJPEGEncoder&&) = delete;

    JERRCODE SetSource(
    uint8_t*   pSrc,
    int      srcStep,
    mfxSize srcSize,
    int      srcChannels,
    JCOLOR   srcColor,
    JSS      srcSampling  = JS_444,
    int      srcPrecision = 8);

  JERRCODE SetSource(
    int16_t*  pSrc,
    int      srcStep,
    mfxSize srcSize,
    int      srcChannels,
    JCOLOR   srcColor,
    JSS      srcSampling = JS_444,
    int      srcPrecision = 16);

  JERRCODE SetSource(
    uint8_t*   pSrc[4],
    int      srcStep[4],
    mfxSize srcSize,
    int      srcChannels,
    JCOLOR   srcColor,
    JSS      srcSampling  = JS_411,
    int      srcPrecision = 8);

  JERRCODE SetSource(
    int16_t*  pSrc[4],
    int      srcStep[4],
    mfxSize srcSize,
    int      srcChannels,
    JCOLOR   srcColor,
    JSS      srcSampling  = JS_444,
    int      srcPrecision = 16);

  JERRCODE SetDestination(
    CBaseStreamOutput* pStreamOut);

  JERRCODE SetParams(
             JMODE mode,
             JCOLOR color,
             JSS sampling,
             int restart_interval,
             int interleaved,
             int piecesCountInField,
             int piecePosInField,
             int numScan,
             int piecePosInScan,
             int huff_opt,
             int quality,
             JTMODE threading_mode = JT_OLD);

  JERRCODE SetParams(
             JMODE mode,
             JCOLOR color,
             JSS sampling,
             int restart_interval,
             int huff_opt,
             int point_transform,
             int predictor);

  JERRCODE InitHuffmanTable(uint8_t bits[16], uint8_t vals[256], int tbl_id, HTBL_CLASS tbl_class);
  JERRCODE InitQuantTable(uint8_t  qnt[64], int tbl_id, int quality);
  JERRCODE InitQuantTable(uint16_t qnt[64], int tbl_id, int quality);

  JERRCODE AttachHuffmanTable(int tbl_id, HTBL_CLASS tbl_class, int comp_no);
  JERRCODE AttachQuantTable(int tbl_id, int comp_no);

  JERRCODE WriteHeader(void);
  JERRCODE WriteData(void);

  int NumOfBytes(void) { return m_BitStreamOut.NumOfBytes(); }

  JERRCODE SetComment( int comment_size, char* comment = 0);
  JERRCODE SetJFIFApp0Resolution( JRESUNITS units, int xdensity, int ydensity);


  uint16_t   GetNumQuantTables(void);
  JERRCODE FillQuantTable(int numTable, uint16_t* pTable);

  uint16_t   GetNumACTables(void);
  JERRCODE FillACTable(int numTable, uint8_t* pBits, uint8_t* pValues);

  uint16_t   GetNumDCTables(void);
  JERRCODE FillDCTable(int numTable, uint8_t* pBits, uint8_t* pValues);

  JERRCODE SetQuantTable(int numTable, uint16_t* pTable);
  JERRCODE SetACTable(int numTable, uint8_t* pBits, uint8_t* pValues);
  JERRCODE SetDCTable(int numTable, uint8_t* pBits, uint8_t* pValues);

  JERRCODE SetDefaultQuantTable(uint16_t quality);
  JERRCODE SetDefaultACTable();
  JERRCODE SetDefaultDCTable();

  bool     IsQuantTableInited();
  bool     IsACTableInited();
  bool     IsDCTableInited();

protected:
  IMAGE      m_src;

  CBitStreamOutput m_BitStreamOut;
  CBitStreamOutput* m_BitStreamOutT;

  int        m_jpeg_ncomp;
  int        m_jpeg_precision;
  JSS        m_jpeg_sampling;
  JCOLOR     m_jpeg_color;
  int        m_jpeg_quality;
  int        m_jpeg_restart_interval;
  JMODE      m_jpeg_mode;
  char*      m_jpeg_comment;

  int        m_numxMCU;
  int        m_numyMCU;
  int        m_mcuWidth;
  int        m_mcuHeight;
  int        m_ccWidth;
  int        m_ccHeight;
  int        m_xPadding;
  int        m_yPadding;

  int        m_num_scans;
  JSCAN      m_curr_scan;

  int        m_ss;
  int        m_se;
  int        m_al;
  int        m_ah;
  int        m_predictor;
  int        m_pt;
  int        m_optimal_htbl;
  JPEG_SCAN* m_scan_script;

  // Number of MCU already encoded
  uint32_t     m_mcu_encoded;
  // Number of MCU remain in the current VLC unit
  uint32_t     m_mcu_to_encode;

  int16_t*    m_block_buffer;
  int        m_block_buffer_size;
  int        m_num_threads;
  int        m_nblock;

  int        m_jfif_app0_xDensity;
  int        m_jfif_app0_yDensity;
  JRESUNITS  m_jfif_app0_units;

  int        m_num_rsti;
  int        m_rstiHeight;
  JTMODE     m_threading_mode;
  
  int        m_piecesCountInField;
  int        m_piecePosInField;
  int        m_piecePosInScan;

  bool       m_externalQuantTable;
  bool       m_externalHuffmanTable;

  int16_t**   m_lastDC;

  CJPEGEncoderHuffmanState*   m_state_t;

  CJPEGColorComponent        m_ccomp[MAX_COMPS_PER_SCAN];
  CJPEGEncoderQuantTable     m_qntbl[MAX_QUANT_TABLES];
  CJPEGEncoderHuffmanTable   m_dctbl[MAX_HUFF_TABLES];
  CJPEGEncoderHuffmanTable   m_actbl[MAX_HUFF_TABLES];
  CJPEGEncoderHuffmanState   m_state;

  JERRCODE Init(void);
  JERRCODE Clean(void);
  JERRCODE ColorConvert(uint32_t rowMCU, uint32_t colMCU, uint32_t maxMCU/*int nMCURow, int thread_id = 0*/);
  JERRCODE DownSampling(uint32_t rowMCU, uint32_t colMCU, uint32_t maxMCU/*int nMCURow, int thread_id = 0*/);

  JERRCODE WriteSOI(void);
  JERRCODE WriteEOI(void);
  JERRCODE WriteAPP0(void);
  JERRCODE WriteAPP14(void);
  JERRCODE WriteSOF0(void);
  JERRCODE WriteSOF1(void);
  JERRCODE WriteSOF2(void);
  JERRCODE WriteSOF3(void);
  JERRCODE WriteDRI(int restart_interval);
  JERRCODE WriteRST(int next_restart_num);
  JERRCODE WriteSOS(void);
  JERRCODE WriteSOS(int ncomp,int id[MAX_COMPS_PER_SCAN],int Ss,int Se,int Ah,int Al);
  JERRCODE WriteDQT(CJPEGEncoderQuantTable* tbl);
  JERRCODE WriteDHT(CJPEGEncoderHuffmanTable* tbl);
  JERRCODE WriteCOM(char* comment = 0);

  JERRCODE EncodeScanBaseline(void);

  JERRCODE EncodeScanBaselineRSTI(void);
  JERRCODE EncodeScanBaselineRSTI_P(void);

  JERRCODE EncodeHuffmanMCURowBL_RSTI(int16_t* pMCUBuf, int thread_id = 0);
  JERRCODE ProcessRestart(int id[MAX_COMPS_PER_SCAN],int Ss,int Se,int Ah,int Al, int nRSTI, int thread_id);
  JERRCODE WriteRST_T(int next_restart_num,  int thread_id = 0);

  JERRCODE EncodeScanExtended(void);
  JERRCODE EncodeScanExtended_P(void);
  JERRCODE EncodeScanLossless(void);
  JERRCODE EncodeScanProgressive(void);

  JERRCODE EncodeScan(int ncomp,int id[MAX_COMPS_PER_SCAN],int Ss,int Se,int Ah,int Al);
  JERRCODE GenerateHuffmanTables(int ncomp,int id[MAX_COMPS_PER_SCAN],int Ss,int Se,int Ah,int Al);
  JERRCODE GenerateHuffmanTables(void);
  JERRCODE GenerateHuffmanTablesEX(void);

  JERRCODE ProcessRestart(int id[MAX_COMPS_PER_SCAN],int Ss,int Se,int Ah,int Al);
  JERRCODE ProcessRestart(int stat[2][256],int id[MAX_COMPS_PER_SCAN],int Ss,int Se,int Ah,int Al);

  JERRCODE EncodeHuffmanMCURowBL(int16_t* pMCUBuf, uint32_t colMCU, uint32_t maxMCU);
  JERRCODE EncodeHuffmanMCURowLS(int16_t* pMCUBuf);

  JERRCODE TransformMCURowBL(int16_t* pMCUBuf, uint32_t colMCU, uint32_t maxMCU/*int16_t* pMCUBuf, int thread_id = 0*/);

  JERRCODE ProcessBuffer(uint32_t rowMCU, uint32_t colMCU, uint32_t maxMCU);//(int nMCURow, int thread_id = 0);
  JERRCODE EncodeScanProgressive_P(void);

  JERRCODE TransformMCURowEX(int16_t* pMCUBuf, int thread_id = 0);
  JERRCODE TransformMCURowLS(int16_t* pMCUBuf, int nMCURow, int thread_id = 0);

};

#endif // MFX_ENABLE_MJPEG_VIDEO_ENCODE
#endif // __JPEGENC_H__
