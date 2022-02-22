// Copyright (c) 2017-2019 Intel Corporation
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
#if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "jpegbase.h"
#include "jpegdec_base.h"

#include "vm_debug.h"

CJPEGDecoderBase::CJPEGDecoderBase(void)
{
  Reset();
  return;
} // ctor


CJPEGDecoderBase::~CJPEGDecoderBase(void)
{
  Clean();
  return;
} // dtor


void CJPEGDecoderBase::Reset(void)
{
  m_jpeg_width             = 0;
  m_jpeg_height            = 0;
  m_jpeg_ncomp             = 0;
  m_jpeg_precision         = 8;
  m_jpeg_sampling          = JS_OTHER;
  m_jpeg_color             = JC_UNKNOWN;
  
  m_jpeg_mode              = JPEG_UNKNOWN;

  m_jfif_app0_detected     = 0;
  m_jfif_app0_major        = 0;
  m_jfif_app0_minor        = 0;
  m_jfif_app0_units        = 0;
  m_jfif_app0_xDensity     = 0;
  m_jfif_app0_yDensity     = 0;
  m_jfif_app0_thumb_width  = 0;
  m_jfif_app0_thumb_height = 0;

  m_jfxx_app0_detected     = 0;
  m_jfxx_thumbnails_type   = 0;

  m_avi1_app0_detected     = 0;
  m_avi1_app0_polarity     = 0;
  m_avi1_app0_reserved     = 0;
  m_avi1_app0_field_size   = 0;
  m_avi1_app0_field_size2  = 0;

  m_adobe_app14_detected   = 0;
  m_adobe_app14_version    = 0;
  m_adobe_app14_flags0     = 0;
  m_adobe_app14_flags1     = 0;
  m_adobe_app14_transform  = 0;

  m_precision              = 0;
  m_max_hsampling          = 0;
  m_max_vsampling          = 0;
  m_sos_len                = 0;
  m_curr_comp_no           = 0;
  m_num_scans              = 0;
  for(int i = 0; i < MAX_SCANS_PER_FRAME; i++)
  {
    m_scans[i].scan_no = i;
    m_scans[i].jpeg_restart_interval = 0;
    m_scans[i].min_h_factor = 0;
    m_scans[i].min_v_factor = 0;
    m_scans[i].numxMCU = 0;
    m_scans[i].numyMCU = 0;
    m_scans[i].mcuWidth = 0;
    m_scans[i].mcuHeight = 0;
    m_scans[i].xPadding = 0;
    m_scans[i].yPadding = 0;
    m_scans[i].ncomps = 0;
    m_scans[i].first_comp = 0;
  }
  m_curr_scan = &m_scans[0];
  m_ss                     = 0;
  m_se                     = 0;
  m_al                     = 0;
  m_ah                     = 0;
  m_marker                 = JM_NONE;

  m_nblock                 = 0;

  m_stream_in.Close();
  return;
} // CJPEGDecoderBase::Reset(void)


JERRCODE CJPEGDecoderBase::Clean(void)
{
  int i;

  m_avi1_app0_detected    = 0;
  m_avi1_app0_polarity    = 0;
  m_avi1_app0_reserved    = 0;
  m_avi1_app0_field_size  = 0;
  m_avi1_app0_field_size2 = 0;

  m_jfif_app0_detected    = 0;
  m_jfxx_app0_detected    = 0;

  m_adobe_app14_detected = 0;

  m_curr_scan->ncomps = 0;

  for(i = 0; i < MAX_COMPS_PER_SCAN; i++)
  {
    if(0 != m_ccomp[i].m_curr_row)
    {
      free(m_ccomp[i].m_curr_row);
      m_ccomp[i].m_curr_row = 0;
    }
    if(0 != m_ccomp[i].m_prev_row)
    {
      free(m_ccomp[i].m_prev_row);
      m_ccomp[i].m_prev_row = 0;
    }
  }

  for(i = 0; i < MAX_HUFF_TABLES; i++)
  {
    m_dctbl[i].Destroy();
    m_actbl[i].Destroy();
  }

  return JPEG_OK;
} // CJPEGDecoderBase::Clean()


#define BS_BUFLEN 16384

JERRCODE CJPEGDecoderBase::SetSource(const uint8_t* pBuf, size_t buflen)
{
  JERRCODE jerr = m_stream_in.Open(pBuf, buflen);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamIn.Attach(&m_stream_in);
  if(JPEG_OK != jerr)
    return jerr;

  return m_BitStreamIn.Init(BS_BUFLEN);
} // CJPEGDecoderBase::SetSource()

JERRCODE CJPEGDecoderBase::Seek(long offset, int origin)
{
  return m_stream_in.Seek(offset, origin);
}// CJPEGDecoderBase::Seek

JERRCODE CJPEGDecoderBase::DetectSampling(void)
{
  switch(m_jpeg_ncomp)
  {
  case 1:
    if(m_ccomp[0].m_hsampling == 1 && m_ccomp[0].m_vsampling == 1)
    {
      m_jpeg_sampling = JS_444;
    }
    else
    {
      return JPEG_ERR_BAD_DATA;
    }
    break;

  case 3:
    if(m_ccomp[0].m_hsampling == 1 && m_ccomp[0].m_vsampling == 1 &&
       m_ccomp[1].m_hsampling == 1 && m_ccomp[1].m_vsampling == 1 &&
       m_ccomp[2].m_hsampling == 1 && m_ccomp[2].m_vsampling == 1)
    {
      m_jpeg_sampling = JS_444;
    }
    else if(m_ccomp[0].m_hsampling == 2 && m_ccomp[0].m_vsampling == 1 &&
            m_ccomp[1].m_hsampling == 1 && m_ccomp[1].m_vsampling == 1 &&
            m_ccomp[2].m_hsampling == 1 && m_ccomp[2].m_vsampling == 1)
    {
      m_jpeg_sampling = JS_422H;
    }
    else if(m_ccomp[0].m_hsampling == 1 && m_ccomp[0].m_vsampling == 2 &&
            m_ccomp[1].m_hsampling == 1 && m_ccomp[1].m_vsampling == 1 &&
            m_ccomp[2].m_hsampling == 1 && m_ccomp[2].m_vsampling == 1)
    {
      m_jpeg_sampling = JS_422V;
    }
    else if(m_ccomp[0].m_hsampling == 2 && m_ccomp[0].m_vsampling == 2 &&
            m_ccomp[1].m_hsampling == 1 && m_ccomp[1].m_vsampling == 1 &&
            m_ccomp[2].m_hsampling == 1 && m_ccomp[2].m_vsampling == 1)
    {
      m_jpeg_sampling = JS_420;
    }
    else if(m_ccomp[0].m_hsampling == 4 && m_ccomp[0].m_vsampling == 1 &&
            m_ccomp[1].m_hsampling == 1 && m_ccomp[1].m_vsampling == 1 &&
            m_ccomp[2].m_hsampling == 1 && m_ccomp[2].m_vsampling == 1)
    {
      m_jpeg_sampling = JS_411;
    }
    else
    {
      m_jpeg_sampling = JS_OTHER;
    }
    break;

  case 4:
    if(m_ccomp[0].m_hsampling == 1 && m_ccomp[0].m_vsampling == 1 &&
       m_ccomp[1].m_hsampling == 1 && m_ccomp[1].m_vsampling == 1 &&
       m_ccomp[2].m_hsampling == 1 && m_ccomp[2].m_vsampling == 1 &&
       m_ccomp[3].m_hsampling == 1 && m_ccomp[3].m_vsampling == 1)
    {
      m_jpeg_sampling = JS_444;
    }
    else if(m_ccomp[0].m_hsampling == 2 && m_ccomp[0].m_vsampling == 1 &&
            m_ccomp[1].m_hsampling == 1 && m_ccomp[1].m_vsampling == 1 &&
            m_ccomp[2].m_hsampling == 1 && m_ccomp[2].m_vsampling == 1 &&
            m_ccomp[3].m_hsampling == 2 && m_ccomp[3].m_vsampling == 1)
    {
      m_jpeg_sampling = JS_422H;
    }
    else if(m_ccomp[0].m_hsampling == 1 && m_ccomp[0].m_vsampling == 2 &&
            m_ccomp[1].m_hsampling == 1 && m_ccomp[1].m_vsampling == 1 &&
            m_ccomp[2].m_hsampling == 1 && m_ccomp[2].m_vsampling == 1 &&
            m_ccomp[3].m_hsampling == 1 && m_ccomp[3].m_vsampling == 2)
    {
      m_jpeg_sampling = JS_422V;
    }
    else if(m_ccomp[0].m_hsampling == 2 && m_ccomp[0].m_vsampling == 2 &&
            m_ccomp[1].m_hsampling == 1 && m_ccomp[1].m_vsampling == 1 &&
            m_ccomp[2].m_hsampling == 1 && m_ccomp[2].m_vsampling == 1 &&
            m_ccomp[3].m_hsampling == 2 && m_ccomp[3].m_vsampling == 2)
    {
      m_jpeg_sampling = JS_420;
    }
    else if(m_ccomp[0].m_hsampling == 4 && m_ccomp[0].m_vsampling == 1 &&
            m_ccomp[1].m_hsampling == 1 && m_ccomp[1].m_vsampling == 1 &&
            m_ccomp[2].m_hsampling == 1 && m_ccomp[2].m_vsampling == 1 &&
            m_ccomp[3].m_hsampling == 4 && m_ccomp[3].m_vsampling == 1)
    {
      m_jpeg_sampling = JS_411;
    }
    else
    {
      m_jpeg_sampling = JS_OTHER;
    }
    break;
  }

  return JPEG_OK;
} // CJPEGDecoderBase::DetectSampling()


JERRCODE CJPEGDecoderBase::NextMarker(JMARKER* marker)
{
  int c;
  int n;
  JERRCODE jerr;

  n = 0;

  for(;;)
  {
    jerr = m_BitStreamIn.ReadByte(&c);
    if(JPEG_OK != jerr)
      return jerr;

    if(c != 0xff)
    {
      do
      {
        n++;

        jerr = m_BitStreamIn.ReadByte(&c);
        if(JPEG_OK != jerr)
          return jerr;

      } while(c != 0xff);
    }

    do
    {
      jerr = m_BitStreamIn.ReadByte(&c);
      if(JPEG_OK != jerr)
        return jerr;

    } while(c == 0xff);

    if(c != 0)
    {
      *marker = (JMARKER)c;
      break;
    }
  }

  if(n != 0)
  {
    TRC1("  skip enormous bytes - ",n);
  }

  return JPEG_OK;
} // CJPEGDecoderBase::NextMarker()


JERRCODE CJPEGDecoderBase::SkipMarker(void)
{
  int len;
  JERRCODE jerr;

  jerr = m_BitStreamIn.ReadWord(&len);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamIn.Seek(len - 2);
  if(JPEG_OK != jerr)
    return jerr;

  m_marker = JM_NONE;

  return JPEG_OK;
} // CJPEGDecoderBase::SkipMarker()

JERRCODE CJPEGDecoderBase::ParseSOI(void)
{
  TRC0("-> SOI");
  m_marker = JM_NONE;
  return JPEG_OK;
} // CJPEGDecoderBase::ParseSOI()


JERRCODE CJPEGDecoderBase::ParseEOI(void)
{
  TRC0("-> EOI");
  m_marker = JM_NONE;
  return JPEG_OK;
} // CJPEGDecoderBase::ParseEOI()


const int APP0_JFIF_LENGTH = 14;
const int APP0_JFXX_LENGTH = 6;
const int APP0_AVI1_LENGTH = 14;

JERRCODE CJPEGDecoderBase::ParseAPP0(void)
{
  int b0, b1, b2, b3, b4;
  int len;
  JERRCODE jerr;

  TRC0("-> APP0");

  jerr = m_BitStreamIn.ReadWord(&len);
  if(JPEG_OK != jerr)
    return jerr;

  len -= 2;

  jerr = m_BitStreamIn.CheckByte(0,&b0);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamIn.CheckByte(1,&b1);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamIn.CheckByte(2,&b2);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamIn.CheckByte(3,&b3);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamIn.CheckByte(4,&b4);
  if(JPEG_OK != jerr)
    return jerr;

  if(len >= APP0_JFIF_LENGTH &&
     b0 == 0x4a && // J
     b1 == 0x46 && // F
     b2 == 0x49 && // I
     b3 == 0x46 && // F
     b4 == 0)
  {
    // we've found JFIF APP0 marker
    len -= 5;

    jerr = m_BitStreamIn.Seek(5);
    if(JPEG_OK != jerr)
      return jerr;

    m_jfif_app0_detected = 1;

    jerr = m_BitStreamIn.ReadByte(&m_jfif_app0_major);
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_BitStreamIn.ReadByte(&m_jfif_app0_minor);
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_BitStreamIn.ReadByte(&m_jfif_app0_units);
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_BitStreamIn.ReadWord(&m_jfif_app0_xDensity);
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_BitStreamIn.ReadWord(&m_jfif_app0_yDensity);
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_BitStreamIn.ReadByte(&m_jfif_app0_thumb_width);
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_BitStreamIn.ReadByte(&m_jfif_app0_thumb_height);
    if(JPEG_OK != jerr)
      return jerr;

    len -= 9;
  }

  jerr = m_BitStreamIn.CheckByte(0,&b0);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamIn.CheckByte(1,&b1);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamIn.CheckByte(2,&b2);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamIn.CheckByte(3,&b3);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamIn.CheckByte(4,&b4);
  if(JPEG_OK != jerr)
    return jerr;

  if(len >= APP0_JFXX_LENGTH &&
     b0 == 0x4a && // J
     b1 == 0x46 && // F
     b2 == 0x58 && // X
     b3 == 0x58 && // X
     b4 == 0)
  {
    // we've found JFXX APP0 extension marker
    len -= 5;

    jerr = m_BitStreamIn.Seek(5);
    if(JPEG_OK != jerr)
      return jerr;

    m_jfxx_app0_detected = 1;

    jerr = m_BitStreamIn.ReadByte(&m_jfxx_thumbnails_type);
    if(JPEG_OK != jerr)
      return jerr;

    switch(m_jfxx_thumbnails_type)
    {
    case 0x10: break;
    case 0x11: break;
    case 0x13: break;
    default:   break;
    }
    len -= 1;
  }

  jerr = m_BitStreamIn.CheckByte(0,&b0);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamIn.CheckByte(1,&b1);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamIn.CheckByte(2,&b2);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamIn.CheckByte(3,&b3);
  if(JPEG_OK != jerr)
    return jerr;

  if(len >= APP0_AVI1_LENGTH &&
     b0 == 0x41 && // A
     b1 == 0x56 && // V
     b2 == 0x49 && // I
     b3 == 0x31)   // 1
  {
    // we've found AVI1 APP0 marker
    len -= 4;

    jerr = m_BitStreamIn.Seek(4);
    if(JPEG_OK != jerr)
      return jerr;

    m_avi1_app0_detected = 1;

    jerr = m_BitStreamIn.ReadByte(&m_avi1_app0_polarity);
    if(JPEG_OK != jerr)
      return jerr;

    len -= 1;

    if(len == 7) // old MJPEG AVI
      len -= 7;

    if(len == 9) // ODML MJPEG AVI
    {
      jerr = m_BitStreamIn.ReadByte(&m_avi1_app0_reserved);
      if(JPEG_OK != jerr)
        return jerr;

      jerr = m_BitStreamIn.ReadDword(&m_avi1_app0_field_size);
      if(JPEG_OK != jerr)
        return jerr;

      jerr = m_BitStreamIn.ReadDword(&m_avi1_app0_field_size2);
      if(JPEG_OK != jerr)
        return jerr;

      len -= 9;
    }
  }

  jerr = m_BitStreamIn.Seek(len);
  if(JPEG_OK != jerr)
    return jerr;

  m_marker = JM_NONE;

  return JPEG_OK;
} // CJPEGDecoderBase::ParseAPP0()

const int APP14_ADOBE_LENGTH = 12;

JERRCODE CJPEGDecoderBase::ParseAPP14(void)
{
  int b0, b1, b2, b3, b4;
  int len;
  JERRCODE jerr;

  TRC0("-> APP14");

  jerr = m_BitStreamIn.ReadWord(&len);
  if(JPEG_OK != jerr)
    return jerr;

  len -= 2;

  jerr = m_BitStreamIn.CheckByte(0,&b0);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamIn.CheckByte(1,&b1);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamIn.CheckByte(2,&b2);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamIn.CheckByte(3,&b3);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamIn.CheckByte(4,&b4);
  if(JPEG_OK != jerr)
    return jerr;

  if(len >= APP14_ADOBE_LENGTH &&
     b0 == 0x41 && // A
     b1 == 0x64 && // d
     b2 == 0x6f && // o
     b3 == 0x62 && // b
     b4 == 0x65)   // e
  {
    // we've found Adobe APP14 marker
    len -= 5;

    jerr = m_BitStreamIn.Seek(5);
    if(JPEG_OK != jerr)
      return jerr;

    m_adobe_app14_detected = 1;

    jerr = m_BitStreamIn.ReadWord(&m_adobe_app14_version);
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_BitStreamIn.ReadWord(&m_adobe_app14_flags0);
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_BitStreamIn.ReadWord(&m_adobe_app14_flags1);
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_BitStreamIn.ReadByte(&m_adobe_app14_transform);
    if(JPEG_OK != jerr)
      return jerr;

    TRC1("  adobe_app14_version   - ",m_adobe_app14_version);
    TRC1("  adobe_app14_flags0    - ",m_adobe_app14_flags0);
    TRC1("  adobe_app14_flags1    - ",m_adobe_app14_flags1);
    TRC1("  adobe_app14_transform - ",m_adobe_app14_transform);

    len -= 7;
  }

  jerr = m_BitStreamIn.Seek(len);
  if(JPEG_OK != jerr)
    return jerr;

  m_marker = JM_NONE;

  return JPEG_OK;
} // CJPEGDecoderBase::ParseAPP14()


JERRCODE CJPEGDecoderBase::ParseDQT(void)
{
  int i;
  int id;
  int len;
  JERRCODE jerr;

  TRC0("-> DQT");

  jerr = m_BitStreamIn.ReadWord(&len);
  if(JPEG_OK != jerr)
    return jerr;

  len -= 2;

  while(len > 0)
  {
    jerr = m_BitStreamIn.ReadByte(&id);
    if(JPEG_OK != jerr)
      return jerr;

    int precision = (id & 0xf0) >> 4;

    TRC1("  id        - ",(id & 0x0f));
    TRC1("  precision - ",precision);

    if((id & 0x0f) >= MAX_QUANT_TABLES)
    {
      return JPEG_ERR_DQT_DATA;
    }

    int q;
    uint8_t qnt[DCTSIZE2*sizeof(int16_t)];
    uint8_t*  pq8  = (uint8_t*) qnt;
    uint16_t* pq16 = (uint16_t*)qnt;

    for(i = 0; i < DCTSIZE2; i++)
    {
      if(precision)
      {
        jerr = m_BitStreamIn.ReadWord(&q);
        pq16[i] = (uint16_t)q;
      }
      else
      {
        jerr = m_BitStreamIn.ReadByte(&q);
        pq8[i]  = (uint8_t)q;
      }

      if(JPEG_OK != jerr)
        return jerr;
    }

    if(precision == 1)
      jerr = m_qntbl[id & 0x0f].Init(id,pq16);
    else
      jerr = m_qntbl[id & 0x0f].Init(id,pq8);
    if(JPEG_OK != jerr)
    {
      return jerr;
    }

    len -= DCTSIZE2 + DCTSIZE2*precision + 1;
  }

  if(len != 0)
  {
    return JPEG_ERR_DQT_DATA;
  }

  m_marker = JM_NONE;

  return JPEG_OK;
} // CJPEGDecoderBase::ParseDQT()


JERRCODE CJPEGDecoderBase::ParseDHT(void)
{
  int i;
  int len;
  int index;
  int count;
  JERRCODE jerr;

  TRC0("-> DHT");

  jerr = m_BitStreamIn.ReadWord(&len);
  if(JPEG_OK != jerr)
    return jerr;

  len -= 2;

  int v;
  uint8_t bits[MAX_HUFF_BITS];
  uint8_t vals[MAX_HUFF_VALS];

  memset(bits, 0, sizeof(bits));
  memset(vals, 0, sizeof(vals));

  while(len > 16)
  {
    jerr = m_BitStreamIn.ReadByte(&index);
    if(JPEG_OK != jerr)
      return jerr;

    count = 0;
    for(i = 0; i < MAX_HUFF_BITS; i++)
    {
      jerr = m_BitStreamIn.ReadByte(&v);
      if(JPEG_OK != jerr)
        return jerr;

      bits[i] = (uint8_t)v;
      count += bits[i];
    }

    len -= 16 + 1;

    if(count > MAX_HUFF_VALS || count > len)
    {
      return JPEG_ERR_DHT_DATA;
    }

    for(i = 0; i < count; i++)
    {
      jerr = m_BitStreamIn.ReadByte(&v);
      if(JPEG_OK != jerr)
        return jerr;

      vals[i] = (uint8_t)v;
    }

    len -= count;

    if(index >> 4)
    {
      // make AC Huffman table
      if(m_actbl[index & 0x03].IsEmpty())
      {
        jerr = m_actbl[index & 0x03].Create();
        if(JPEG_OK != jerr)
        {
          LOG0("    Can't create AC huffman table");
          return jerr;
        }
      }

      TRC1("    AC Huffman Table - ",index & 0x03);
      jerr = m_actbl[index & 0x03].Init(index & 0x03,index >> 4,bits,vals);
      if(JPEG_OK != jerr)
      {
        LOG0("    Can't build AC huffman table");
        return jerr;
      }
    }
    else
    {
      // make DC Huffman table
      if(m_dctbl[index & 0x03].IsEmpty())
      {
        jerr = m_dctbl[index & 0x03].Create();
        if(JPEG_OK != jerr)
        {
          LOG0("    Can't create DC huffman table");
          return jerr;
        }
      }

      TRC1("    DC Huffman Table - ",index & 0x03);
      jerr = m_dctbl[index & 0x03].Init(index & 0x03,index >> 4,bits,vals);
      if(JPEG_OK != jerr)
      {
        LOG0("    Can't build DC huffman table");
        return jerr;
      }
    }
  }

  if(len != 0)
  {
    return JPEG_ERR_DHT_DATA;
  }

  m_marker = JM_NONE;

  return JPEG_OK;
} // CJPEGDecoderBase::ParseDHT()


JERRCODE CJPEGDecoderBase::ParseSOF0(void)
{
  int i;
  int len;
  CJPEGColorComponent* curr_comp;
  JERRCODE jerr;

  TRC0("-> SOF0");

  jerr = m_BitStreamIn.ReadWord(&len);
  if(JPEG_OK != jerr)
    return jerr;

  len -= 2;

  jerr = m_BitStreamIn.ReadByte(&m_jpeg_precision);
  if(JPEG_OK != jerr)
    return jerr;

  if(m_jpeg_precision != 8 && m_jpeg_precision != 12)
  {
    return JPEG_ERR_SOF_DATA;
  }

  jerr = m_BitStreamIn.ReadWord(&m_jpeg_height);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamIn.ReadWord(&m_jpeg_width);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamIn.ReadByte(&m_jpeg_ncomp);
  if(JPEG_OK != jerr)
    return jerr;

  TRC1("  height    - ",m_jpeg_height);
  TRC1("  width     - ",m_jpeg_width);
  TRC1("  nchannels - ",m_jpeg_ncomp);

  if(m_jpeg_ncomp < 0 || m_jpeg_ncomp > MAX_COMPS_PER_SCAN)
  {
    return JPEG_ERR_SOF_DATA;
  }

  len -= 6;

  if(len != m_jpeg_ncomp * 3)
  {
    return JPEG_ERR_SOF_DATA;
  }

  for(m_nblock = 0, i = 0; i < m_jpeg_ncomp; i++)
  {
    curr_comp = &m_ccomp[i];

    jerr = m_BitStreamIn.ReadByte(&curr_comp->m_id);
    if(JPEG_OK != jerr)
      return jerr;

    int ss;

    jerr = m_BitStreamIn.ReadByte(&ss);
    if(JPEG_OK != jerr)
      return jerr;

    curr_comp->m_hsampling  = (ss >> 4) & 0x0f;
    curr_comp->m_vsampling  = (ss     ) & 0x0f;

    if(m_jpeg_ncomp == 1)
    {
      curr_comp->m_hsampling  = 1;
      curr_comp->m_vsampling  = 1;
    }

    jerr = m_BitStreamIn.ReadByte(&curr_comp->m_q_selector);
    if(JPEG_OK != jerr)
      return jerr;

    if(curr_comp->m_hsampling <= 0 || curr_comp->m_hsampling > 4 ||
       curr_comp->m_vsampling <= 0 || curr_comp->m_vsampling > 4 || curr_comp->m_q_selector >= MAX_QUANT_TABLES)
    {
      return JPEG_ERR_SOF_DATA;
    }

    // num of DU block per component
    curr_comp->m_nblocks = curr_comp->m_hsampling * curr_comp->m_vsampling;

    // num of DU blocks per frame
    m_nblock += curr_comp->m_nblocks;

    TRC1("    id ",curr_comp->m_id);
    TRC1("      hsampling - ",curr_comp->m_hsampling);
    TRC1("      vsampling - ",curr_comp->m_vsampling);
    TRC1("      qselector - ",curr_comp->m_q_selector);
  }

  jerr = DetectSampling();
  if(JPEG_OK != jerr)
  {
    return jerr;
  }

  m_max_hsampling = m_ccomp[0].m_hsampling;
  m_max_vsampling = m_ccomp[0].m_vsampling;

  for(i = 0; i < m_jpeg_ncomp; i++)
  {
    curr_comp = &m_ccomp[i];

    if(m_max_hsampling < curr_comp->m_hsampling)
      m_max_hsampling = curr_comp->m_hsampling;

    if(m_max_vsampling < curr_comp->m_vsampling)
      m_max_vsampling = curr_comp->m_vsampling;
  }

  for(i = 0; i < m_jpeg_ncomp; i++)
  {
    curr_comp = &m_ccomp[i];

    curr_comp->m_h_factor = m_max_hsampling / curr_comp->m_hsampling;
    curr_comp->m_v_factor = m_max_vsampling / curr_comp->m_vsampling;
  }

  m_jpeg_mode = JPEG_BASELINE;

  m_marker = JM_NONE;

  return JPEG_OK;
} // CJPEGDecoderBase::ParseSOF0()

JERRCODE CJPEGDecoderBase::ParseDRI(void)
{
  int len, jpeg_restart_interval;
  JERRCODE jerr;

  TRC0("-> DRI");

  jerr = m_BitStreamIn.ReadWord(&len);
  if(JPEG_OK != jerr)
    return jerr;

  len -= 2;

  if(len != 2)
  {
    return JPEG_ERR_RST_DATA;
  }

  jerr = m_BitStreamIn.ReadWord(&jpeg_restart_interval);
  if(JPEG_OK != jerr)
      return jerr;

  for(int i=m_curr_scan->scan_no; i<MAX_SCANS_PER_FRAME; i++)
  {
      m_scans[i].jpeg_restart_interval = jpeg_restart_interval;
  }

  TRC1("  restart interval - ", m_curr_scan->jpeg_restart_interval);

  m_marker = JM_NONE;

  return JPEG_OK;
} // CJPEGDecoderBase::ParseDRI()

JERRCODE CJPEGDecoderBase::ParseSOS(JOPERATION op)
{
  int i,j;
  int ci;
  int len;
  int gcd_found;
  int hsampling_gcd = 1, vsampling_gcd = 1;
  int hsamplings[MAX_COMPS_PER_SCAN], vsamplings[MAX_COMPS_PER_SCAN];
  int component_ids[MAX_COMPS_PER_SCAN];
  int scan_max_hsampling = 1, scan_max_vsampling = 1;
  int du_width, du_height;
  int numComps = 0;
  JERRCODE jerr;

  TRC0("-> SOS");

  jerr = m_BitStreamIn.ReadWord(&len);
  if(JPEG_OK != jerr)
    return jerr;

  // store position to return to in subsequent ReadData call
  m_sos_len = len;

  len -= 2;

  jerr = m_BitStreamIn.ReadByte(&(m_curr_scan->ncomps));
  if(JPEG_OK != jerr)
    return jerr;

  if(m_curr_scan->ncomps < 1 || m_curr_scan->ncomps > MAX_COMPS_PER_SCAN)
  {
    return JPEG_ERR_SOS_DATA;
  }

  if(len != ((m_curr_scan->ncomps * 2) + 4))
  {
    return JPEG_ERR_SOS_DATA;
  }

  TRC1("  ncomps - ",m_scan_ncomps);

  for(i = 0; i < m_curr_scan->ncomps; i++)
  {
    int id;
    int huff_sel;

    jerr = m_BitStreamIn.ReadByte(&id);
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_BitStreamIn.ReadByte(&huff_sel);
    if(JPEG_OK != jerr)
      return jerr;

    TRC1("    id - ",id);
    TRC1("      dc_selector - ",(huff_sel >> 4) & 0x0f);
    TRC1("      ac_selector - ",(huff_sel     ) & 0x0f);

    for(ci = 0; ci < m_jpeg_ncomp; ci++)
    {
      if(id == m_ccomp[ci].m_id)
      {
        m_curr_comp_no        = ci;
        m_ccomp[ci].m_comp_no = ci;
        m_ccomp[ci].m_lastDC = 0;

        hsamplings[numComps] = m_ccomp[ci].m_hsampling;
        vsamplings[numComps] = m_ccomp[ci].m_vsampling;
        component_ids[numComps] = ci;
        numComps++;

        goto comp_id_match;
      }
    }

    return JPEG_ERR_SOS_DATA;

comp_id_match:

    m_ccomp[ci].m_dc_selector = (huff_sel >> 4) & 0x0f;
    if (m_ccomp[ci].m_dc_selector >= MAX_HUFF_TABLES)
      return JPEG_ERR_SOS_DATA;

    m_ccomp[ci].m_ac_selector = (huff_sel     ) & 0x0f;
    if (m_ccomp[ci].m_ac_selector >= MAX_HUFF_TABLES)
      return JPEG_ERR_SOS_DATA;
  }

  // find greatest common divisor for sampling factors of components in scan
  for(i = 2; i < MAX_BLOCKS_PER_MCU; i++)
  {
      gcd_found = 1;
      for(j = 0; j < numComps; j++)
          if(hsamplings[j] % i != 0)
              gcd_found = 0;
      if(gcd_found)
          hsampling_gcd = i;

      gcd_found = 1;
      for(j = 0; j < numComps; j++)
          if(vsamplings[j] % i != 0)
              gcd_found = 0;
      if(gcd_found)
          vsampling_gcd = i;
  }

  m_curr_scan->min_h_factor = MAX_BLOCKS_PER_MCU;
  m_curr_scan->min_v_factor = MAX_BLOCKS_PER_MCU;

  // setting MCU parameters for current scan
  for(i = 0; i < numComps; i++)
  {
      m_ccomp[component_ids[i]].m_scan_hsampling = m_ccomp[component_ids[i]].m_hsampling / hsampling_gcd;
      if(scan_max_hsampling < m_ccomp[component_ids[i]].m_scan_hsampling)
          scan_max_hsampling = m_ccomp[component_ids[i]].m_scan_hsampling;

      m_ccomp[component_ids[i]].m_scan_vsampling = m_ccomp[component_ids[i]].m_vsampling / vsampling_gcd;
      if(scan_max_vsampling < m_ccomp[component_ids[i]].m_scan_vsampling)
          scan_max_vsampling = m_ccomp[component_ids[i]].m_scan_vsampling;

      if(m_curr_scan->min_h_factor > m_ccomp[component_ids[i]].m_h_factor)
          m_curr_scan->min_h_factor = m_ccomp[component_ids[i]].m_h_factor;

      if(m_curr_scan->min_v_factor > m_ccomp[component_ids[i]].m_v_factor)
          m_curr_scan->min_v_factor = m_ccomp[component_ids[i]].m_v_factor;
  }

  // DU block dimensions (8x8 for DCT based modes and 1x1 for lossless mode)
  du_width  = (JPEG_LOSSLESS == m_jpeg_mode) ? 1 : 8;
  du_height = (JPEG_LOSSLESS == m_jpeg_mode) ? 1 : 8;

  // MCU dimensions
  m_curr_scan->mcuWidth  = du_width  * std::max(scan_max_hsampling, 1);
  m_curr_scan->mcuHeight = du_height * std::max(scan_max_vsampling, 1);

  // num of MCUs in whole scan
  m_curr_scan->numxMCU = (m_jpeg_width  + (m_curr_scan->mcuWidth * m_curr_scan->min_h_factor  - 1)) 
      / (m_curr_scan->mcuWidth * m_curr_scan->min_h_factor);
  m_curr_scan->numyMCU = (m_jpeg_height + (m_curr_scan->mcuHeight * m_curr_scan->min_v_factor - 1)) 
      / (m_curr_scan->mcuHeight * m_curr_scan->min_v_factor);

  // not completed MCUs should be padded
  m_curr_scan->xPadding = m_curr_scan->numxMCU * m_curr_scan->mcuWidth * m_curr_scan->min_h_factor  - m_jpeg_width;
  m_curr_scan->yPadding = m_curr_scan->numyMCU * m_curr_scan->mcuHeight * m_curr_scan->min_v_factor - m_jpeg_height;

  jerr = m_BitStreamIn.ReadByte(&m_ss);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamIn.ReadByte(&m_se);
  if(JPEG_OK != jerr)
    return jerr;

  int t;

  jerr = m_BitStreamIn.ReadByte(&t);
  if(JPEG_OK != jerr)
    return jerr;

  m_ah = (t >> 4) & 0x0f;
  m_al = (t     ) & 0x0f;

  TRC1("  Ss - ",m_ss);
  TRC1("  Se - ",m_se);
  TRC1("  Ah - ",m_ah);
  TRC1("  Al - ",m_al);

  if(JO_READ_HEADER == op)
  {
    // detect JPEG color space
    if(m_jpeg_color == JC_UNKNOWN)
    {
        if(m_jfif_app0_detected)
        {
          switch(m_jpeg_ncomp)
          {
          case 1:  m_jpeg_color = JC_GRAY;    break;
          case 3:  m_jpeg_color = JC_YCBCR;   break;
          default: m_jpeg_color = JC_UNKNOWN; break;
          }
        }

        if(m_adobe_app14_detected)
        {
          switch(m_adobe_app14_transform)
          {
          case 0:
            switch(m_jpeg_ncomp)
            {
            case 1:  m_jpeg_color = JC_GRAY;    break;
            case 3:  m_jpeg_color = JC_RGB;     break;
            case 4:  m_jpeg_color = JC_CMYK;    break;
            default: m_jpeg_color = JC_UNKNOWN; break;
            }
            break;

          case 1:  m_jpeg_color = JC_YCBCR;   break;
          case 2:  m_jpeg_color = JC_YCCK;    break;
          default: m_jpeg_color = JC_UNKNOWN; break;
          }
        }

        // try to guess what color space is used...
        if(!m_jfif_app0_detected && !m_adobe_app14_detected)
        {
          switch(m_jpeg_ncomp)
          {
          case 1:  m_jpeg_color = JC_GRAY;    break;
          case 3:  m_jpeg_color = (m_jpeg_mode != JPEG_LOSSLESS && m_jpeg_precision == 8) ? JC_YCBCR : JC_UNKNOWN; break;
          default: m_jpeg_color = JC_UNKNOWN; break;
          }
        }
    }
  }

  m_marker = JM_NONE;

  return JPEG_OK;
} // CJPEGDecoderBase::ParseSOS()

JERRCODE CJPEGDecoderBase::ParseJPEGBitStream(JOPERATION op)
{
  JERRCODE jerr = JPEG_OK;

  m_marker = JM_NONE;

  for(;;)
  {
    if(JM_NONE == m_marker)
    {
      jerr = NextMarker(&m_marker);
      if(JPEG_OK != jerr)
      {
        return jerr;
      }
    }

    switch(m_marker)
    {
    case JM_SOI:
      jerr = ParseSOI();
      if(JPEG_OK != jerr)
      {
        return jerr;
      }
      break;

    case JM_APP0:
      jerr = ParseAPP0();
      if(JPEG_OK != jerr)
      {
        return jerr;
      }
      break;

    case JM_APP14:
      jerr = ParseAPP14();
      if(JPEG_OK != jerr)
      {
        return jerr;
      }
      break;

    case JM_DQT:
      jerr = ParseDQT();
      if(JPEG_OK != jerr)
      {
        return jerr;
      }
      break;

    case JM_SOF0:
      jerr = ParseSOF0();
      if(JPEG_OK != jerr)
      {
        return jerr;
      }
      break;

    case JM_SOF1:
    case JM_SOF2:
    case JM_SOF3:
    case JM_SOF5:
    case JM_SOF6:
    case JM_SOF7:
    case JM_SOF9:
    case JM_SOFA:
    case JM_SOFB:
    case JM_SOFD:
    case JM_SOFE:
    case JM_SOFF:
      return JPEG_NOT_IMPLEMENTED;

    case JM_DHT:
      jerr = ParseDHT();
      if(JPEG_OK != jerr)
      {
        return jerr;
      }
      break;

    case JM_DRI:
      jerr = ParseDRI();
      if(JPEG_OK != jerr)
      {
        return jerr;
      }
      break;

    case JM_SOS:
      jerr = ParseSOS(op);
      if(JPEG_OK != jerr)
      {
        return jerr;
      }

      if (JO_READ_HEADER == op)
      {
          if(m_BitStreamIn.GetCurrPos() - (m_sos_len + 2) != 0)
          {
              jerr = m_BitStreamIn.Seek(-(m_sos_len + 2));
              if(JPEG_OK != jerr)
                  return jerr;
          }
          else
          {
              m_BitStreamIn.SetCurrPos(0);
          }

        // stop here, when we are reading header
        return JPEG_OK;
      }
      break;

    case JM_EOI:
      jerr = ParseEOI();
      goto Exit;

    default:
      TRC1("-> Unknown marker ",m_marker);
      TRC0("..Skipping");
      jerr = SkipMarker();
      if(JPEG_OK != jerr)
        return jerr;

      break;
    }
  }

Exit:

  return jerr;
} // CJPEGDecoderBase::ParseJPEGBitStream()


JERRCODE CJPEGDecoderBase::FindSOI()
{
  JERRCODE jerr = JPEG_OK;
  m_marker = JM_NONE;

  for(;;)
  {
    jerr = NextMarker(&m_marker);

    if(JM_SOI == m_marker)
    {
        if(m_BitStreamIn.GetCurrPos() - 2 != 0)
        {
            jerr = m_BitStreamIn.Seek(-2);
            if(JPEG_OK != jerr)
                return jerr;
        }
        else
        {
            m_BitStreamIn.SetCurrPos(0);
        }
        return JPEG_OK;
    }

    if(JPEG_ERR_BUFF == jerr)
    {
        return JPEG_OK;
    }

    if(JPEG_OK != jerr)
    {
        return jerr;
    }
  }
} // CJPEGDecoderBase::ParseJPEGBitStream()

JERRCODE CJPEGDecoderBase::ReadHeader(
  int*    width,
  int*    height,
  int*    nchannels,
  JCOLOR* color,
  JSS*    sampling,
  int*    precision)
{
  JERRCODE jerr;

  // parse bitstream up to SOS marker
  jerr = ParseJPEGBitStream(JO_READ_HEADER);

  if(JPEG_OK != jerr)
  {
    LOG0("Error: ParseJPEGBitStream() failed");
    return jerr;
  }

  if(JPEG_UNKNOWN == m_jpeg_mode)
    return JPEG_ERR_BAD_DATA;

  *width     = m_jpeg_width;
  *height    = m_jpeg_height;
  *nchannels = m_jpeg_ncomp;
  *color     = m_jpeg_color;
  *sampling  = m_jpeg_sampling;
  *precision = m_jpeg_precision;

  return JPEG_OK;
} // CJPEGDecoderBase::ReadHeader()

uint16_t CJPEGDecoderBase::GetNumQuantTables()
{
    uint16_t numTables = 0;

    for(int i=0; i<MAX_QUANT_TABLES; i++)
        if(m_qntbl[i].m_initialized)
            numTables++;

    return numTables;
}
  
JERRCODE CJPEGDecoderBase::FillQuantTable(int numTable, uint16_t* pTable)
{
    uint8_t* qtbl = m_qntbl[numTable].m_raw8u;

    for(int i=0; i<DCTSIZE2; i++)
        pTable[i] = (uint16_t)qtbl[i];

    return JPEG_OK;
}

uint16_t CJPEGDecoderBase::GetNumACTables()
{
    uint16_t numTables = 0;

    for(int i=0; i<MAX_HUFF_TABLES; i++)
        if(m_actbl[i].IsValid())
            numTables++;

    return numTables;
}

JERRCODE CJPEGDecoderBase::FillACTable(int numTable, uint8_t* pBits, uint8_t* pValues)
{
    const uint8_t* bits = m_actbl[numTable].GetBits();
    const uint8_t* values = m_actbl[numTable].GetValues();

    MFX_INTERNAL_CPY(pBits, bits, 16);
    MFX_INTERNAL_CPY(pValues, values, 162);

    return JPEG_OK;
}

uint16_t CJPEGDecoderBase::GetNumDCTables()
{
    uint16_t numTables = 0;

    for(int i=0; i<MAX_HUFF_TABLES; i++)
        if(m_dctbl[i].IsValid())
            numTables++;

    return numTables;
}

JERRCODE CJPEGDecoderBase::FillDCTable(int numTable, uint8_t* pBits, uint8_t* pValues)
{
    const uint8_t* bits = m_dctbl[numTable].GetBits();
    const uint8_t* values = m_dctbl[numTable].GetValues();

    MFX_INTERNAL_CPY(pBits, bits, 16);
    MFX_INTERNAL_CPY(pValues, values, 12);

    return JPEG_OK;
}

bool CJPEGDecoderBase::IsInterleavedScan()
{
    if(m_curr_scan->ncomps == m_jpeg_ncomp)
        return true;
    
    return false;
}

#endif // MFX_ENABLE_MJPEG_VIDEO_DECODE
