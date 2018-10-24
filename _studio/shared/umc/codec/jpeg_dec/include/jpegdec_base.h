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

#ifndef __JPEGDEC_BASE_H__
#define __JPEGDEC_BASE_H__

#include "umc_defs.h"
#if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE)
#include "jpegbase.h"
#include "decqtbl.h"
#include "dechtbl.h"
#include "colorcomp.h"
#include "membuffin.h"
#include "bitstreamin.h"

class CJPEGDecoderBase
{
public:

  CJPEGDecoderBase(void);
  virtual ~CJPEGDecoderBase(void);

  virtual void Reset(void);

  JERRCODE SetSource(const uint8_t* pBuf, size_t buflen);
  JERRCODE Seek(long offset, int origin);

  virtual JERRCODE ReadHeader(
    int*     width,
    int*     height,
    int*     nchannels,
    JCOLOR*  color,
    JSS*     sampling,
    int*     precision);

  int  GetNumDecodedBytes(void)        { return m_BitStreamIn.GetNumUsedBytes(); }

  int    GetSOSLen(void)               { return m_sos_len; }

  uint16_t   GetNumQuantTables(void);
  JERRCODE FillQuantTable(int numTable, uint16_t* pTable);

  uint16_t   GetNumACTables(void);
  JERRCODE FillACTable(int numTable, uint8_t* pBits, uint8_t* pValues);

  uint16_t   GetNumDCTables(void);
  JERRCODE FillDCTable(int numTable, uint8_t* pBits, uint8_t* pValues);

  bool     IsInterleavedScan(void);

public:
  int       m_jpeg_width;
  int       m_jpeg_height;
  int       m_jpeg_ncomp;
  int       m_jpeg_precision;
  JSS       m_jpeg_sampling;
  JCOLOR    m_jpeg_color;
  JMODE     m_jpeg_mode;

  // JFIF APP0 related varibales
  int      m_jfif_app0_detected;
  int      m_jfif_app0_major;
  int      m_jfif_app0_minor;
  int      m_jfif_app0_units;
  int      m_jfif_app0_xDensity;
  int      m_jfif_app0_yDensity;
  int      m_jfif_app0_thumb_width;
  int      m_jfif_app0_thumb_height;

  // JFXX APP0 related variables
  int      m_jfxx_app0_detected;
  int      m_jfxx_thumbnails_type;

  // AVI1 APP0 related variables
  int      m_avi1_app0_detected;
  int      m_avi1_app0_polarity;
  int      m_avi1_app0_reserved;
  int      m_avi1_app0_field_size;
  int      m_avi1_app0_field_size2;

  // Adobe APP14 related variables
  int      m_adobe_app14_detected;
  int      m_adobe_app14_version;
  int      m_adobe_app14_flags0;
  int      m_adobe_app14_flags1;
  int      m_adobe_app14_transform;

  int      m_precision;
  int      m_max_hsampling;
  int      m_max_vsampling;
  // Number of MCU remain in the current VLC unit
  int      m_sos_len;
  int      m_curr_comp_no;
  int      m_num_scans;
  JSCAN    m_scans[MAX_SCANS_PER_FRAME];
  JSCAN*   m_curr_scan;
  int      m_ss;
  int      m_se;
  int      m_al;
  int      m_ah;
  JMARKER  m_marker;
  CMemBuffInput m_stream_in;

  int      m_nblock;

  CBitStreamInput             m_BitStreamIn;
  CJPEGColorComponent         m_ccomp[MAX_COMPS_PER_SCAN];
  CJPEGDecoderQuantTable      m_qntbl[MAX_QUANT_TABLES];
  CJPEGDecoderHuffmanTable    m_dctbl[MAX_HUFF_TABLES];
  CJPEGDecoderHuffmanTable    m_actbl[MAX_HUFF_TABLES];

public:
  virtual JERRCODE Clean(void);

  JERRCODE FindSOI();

  virtual JERRCODE ParseJPEGBitStream(JOPERATION op);
  JERRCODE ParseSOI(void);
  JERRCODE ParseEOI(void);
  JERRCODE ParseAPP0(void);
  JERRCODE ParseAPP14(void);
  JERRCODE ParseSOF0(void);
  JERRCODE ParseDRI(void);
  JERRCODE ParseSOS(JOPERATION op);
  JERRCODE ParseDQT(void);
  JERRCODE ParseDHT(void);

  JERRCODE NextMarker(JMARKER* marker);
  JERRCODE SkipMarker(void);

  JERRCODE DetectSampling(void);
};

#endif // MFX_ENABLE_MJPEG_VIDEO_DECODE
#endif // __JPEGDEC_BASE_H__
