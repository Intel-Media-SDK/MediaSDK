// Copyright (c) 2018-2019 Intel Corporation
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
#if defined (MFX_ENABLE_MJPEG_VIDEO_ENCODE)

#include <string>

#include "ippcc.h"
#include "jpegbase.h"
#include "jpegenc.h"


CJPEGEncoder::CJPEGEncoder(void) : m_src()
{
  m_src.sampling    = JS_OTHER;

  m_jpeg_ncomp            = 0;
  m_jpeg_precision        = 8;
  m_jpeg_sampling         = JS_444;
  m_jpeg_color            = JC_UNKNOWN;
  m_jpeg_quality          = 100;
  m_jpeg_restart_interval = 0;
  m_jpeg_mode             = JPEG_BASELINE;
  m_jpeg_comment          = 0;

  m_numxMCU   = 0;
  m_numyMCU   = 0;
  m_mcuWidth  = 0;
  m_mcuHeight = 0;

  m_ccWidth  = 0;
  m_ccHeight = 0;
  m_xPadding = 0;
  m_yPadding = 0;

  m_num_scans  = 0;

  m_curr_scan.first_comp            = 0;
  m_curr_scan.jpeg_restart_interval = 0;
  m_curr_scan.mcuHeight             = 0;
  m_curr_scan.mcuWidth              = 0;
  m_curr_scan.min_h_factor          = 0;
  m_curr_scan.min_v_factor          = 0;
  m_curr_scan.ncomps                = 0;
  m_curr_scan.numxMCU               = 0;
  m_curr_scan.numyMCU               = 0;
  m_curr_scan.scan_no               = 0;
  m_curr_scan.xPadding              = 0;
  m_curr_scan.yPadding              = 0;

  m_ss = 0;
  m_se = 63;
  m_ah = 0;
  m_al = 0;

  m_predictor = 1;
  m_pt        = 0;

  // 1 - to set generated table, 0 for default or external tables
  m_optimal_htbl      = 0;
  m_scan_script       = 0;

  m_mcu_encoded       = 0;
  m_mcu_to_encode     = 0;

  m_block_buffer      = 0;
  m_block_buffer_size = 0;

  m_num_threads       = 0;
  m_nblock            = 0;

  m_jfif_app0_units    = JRU_NONE;
  m_jfif_app0_xDensity = 1;
  m_jfif_app0_yDensity = 1;

  m_num_rsti       = 0;
  m_rstiHeight     = 0;
  m_threading_mode = JT_OLD;

  m_piecesCountInField = 0;
  m_piecePosInField    = 0;
  m_piecePosInScan     = 0;

  m_externalQuantTable = false;
  m_externalHuffmanTable = false;
  m_state_t = NULL;
  m_BitStreamOutT = NULL;
  m_lastDC = NULL;


  return;
} // ctor


CJPEGEncoder::~CJPEGEncoder(void)
{
  Clean();
  return;
} // dtor


JERRCODE CJPEGEncoder::Clean(void)
{
  int i;

  for(i = 0; i < MAX_HUFF_TABLES; i++)
  {
    m_dctbl[i].Destroy();
    m_actbl[i].Destroy();
  }

  if(0 != m_scan_script)
  {
    delete[] m_scan_script;
    m_scan_script = 0;
  }

  if(0 != m_block_buffer)
  {
    mfxFree(m_block_buffer);
    m_block_buffer = 0;
  }

  m_block_buffer_size = 0;

  m_num_threads       = 0;
  m_nblock            = 0;

  for(i =0; i < m_jpeg_ncomp; i++)
  {
    m_ccomp[i].DeleteBufferCC();
    m_ccomp[i].DeleteBufferSS();
    m_ccomp[i].DeleteBufferLNZ();
  }

  m_src.p.Data8u[0] = 0;
  m_src.p.Data8u[1] = 0;
  m_src.p.Data8u[2] = 0;
  m_src.p.Data8u[3] = 0;

  m_src.width       = 0;
  m_src.height      = 0;
  m_src.lineStep[0] = 0;
  m_src.lineStep[1] = 0;
  m_src.lineStep[2] = 0;
  m_src.lineStep[3] = 0;
  m_src.precision   = 0;
  m_src.nChannels   = 0;
  m_src.color       = JC_UNKNOWN;
  m_src.sampling    = JS_OTHER;

  m_jpeg_ncomp            = 0;
  m_jpeg_sampling         = JS_444;
  m_jpeg_color            = JC_UNKNOWN;
  m_jpeg_quality          = 100;
  m_jpeg_mode             = JPEG_BASELINE;

  m_numxMCU = 0;
  m_numyMCU = 0;
  m_mcuWidth  = 0;
  m_mcuHeight = 0;

  m_ccWidth  = 0;
  m_ccHeight = 0;
  m_xPadding = 0;
  m_yPadding = 0;

  m_num_scans  = 0;

  m_curr_scan.first_comp            = 0;
  m_curr_scan.jpeg_restart_interval = 0;
  m_curr_scan.mcuHeight             = 0;
  m_curr_scan.mcuWidth              = 0;
  m_curr_scan.min_h_factor          = 0;
  m_curr_scan.min_v_factor          = 0;
  m_curr_scan.ncomps                = 0;
  m_curr_scan.numxMCU               = 0;
  m_curr_scan.numyMCU               = 0;
  m_curr_scan.scan_no               = 0;
  m_curr_scan.xPadding              = 0;
  m_curr_scan.yPadding              = 0;

  m_mcu_encoded   = 0;
  m_mcu_to_encode = 0;

  m_piecesCountInField = 0;
  m_piecePosInField    = 0;
  m_piecePosInScan     = 0;

  m_optimal_htbl = 0;

  m_state.Destroy();


  m_jpeg_restart_interval = 0;

  return JPEG_OK;
} // CJPEGEncoder::Clean()


JERRCODE CJPEGEncoder::SetSource(
  uint8_t*   pSrc,
  int      srcStep,
  mfxSize srcSize,
  int      srcChannels,
  JCOLOR   srcColor,
  JSS      srcSampling,
  int      srcPrecision)
{
  m_src.p.Data8u[0] = pSrc;
  m_src.lineStep[0] = srcStep;
  m_src.width       = srcSize.width;
  m_src.height      = srcSize.height;
  m_src.nChannels   = srcChannels;
  m_src.color       = srcColor;
  m_src.sampling    = srcSampling;
  m_src.precision   = srcPrecision;
  m_src.order        = JD_PIXEL;

  return JPEG_OK;
} // CJPEGEncoder::SetSource()


JERRCODE CJPEGEncoder::SetSource(
  int16_t*  pSrc,
  int      srcStep,
  mfxSize srcSize,
  int      srcChannels,
  JCOLOR   srcColor,
  JSS      srcSampling,
  int      srcPrecision)
{
  m_src.p.Data16s[0] = pSrc;
  m_src.lineStep[0]  = srcStep;
  m_src.width        = srcSize.width;
  m_src.height       = srcSize.height;
  m_src.nChannels    = srcChannels;
  m_src.color        = srcColor;
  m_src.sampling     = srcSampling;
  m_src.precision    = srcPrecision;
  m_src.order        = JD_PIXEL;

  if(m_src.precision > 10)
    m_optimal_htbl = 1;

  return JPEG_OK;
} // CJPEGEncoder::SetSource()


JERRCODE CJPEGEncoder::SetSource(
  uint8_t*   pSrc[4],
  int      srcStep[4],
  mfxSize srcSize,
  int      srcChannels,
  JCOLOR   srcColor,
  JSS      srcSampling ,
  int      srcPrecision)
{

  if(srcChannels == 4 && m_jpeg_color != srcColor)
    return JPEG_ERR_PARAMS;

  m_src.p.Data8u[0] = pSrc[0];
  m_src.p.Data8u[1] = pSrc[1];
  m_src.p.Data8u[2] = pSrc[2];
  m_src.p.Data8u[3] = pSrc[3];

  m_src.lineStep[0] = srcStep[0];
  m_src.lineStep[1] = srcStep[1];
  m_src.lineStep[2] = srcStep[2];
  m_src.lineStep[3] = srcStep[3];

  if(srcSampling == JS_422V)
  {
    m_src.lineStep[1] *= 2;
    m_src.lineStep[2] *= 2;
  }

  m_src.order     = JD_PLANE;
  m_src.width     = srcSize.width;
  m_src.height    = srcSize.height;
  m_src.nChannels = srcChannels;
  m_src.color     = srcColor;
  m_src.sampling  = srcSampling;
  m_src.precision = srcPrecision;

  return JPEG_OK;
} // CJPEGEncoder::SetSource()


JERRCODE CJPEGEncoder::SetSource(
  int16_t*  pSrc[4],
  int      srcStep[4],
  mfxSize srcSize,
  int      srcChannels,
  JCOLOR   srcColor,
  JSS      srcSampling,
  int      srcPrecision)
{
  m_src.p.Data16s[0] = pSrc[0];
  m_src.p.Data16s[1] = pSrc[1];
  m_src.p.Data16s[2] = pSrc[2];
  m_src.p.Data16s[3] = pSrc[3];

  m_src.lineStep[0] = srcStep[0];
  m_src.lineStep[1] = srcStep[1];
  m_src.lineStep[2] = srcStep[2];
  m_src.lineStep[3] = srcStep[3];

  m_src.order     = JD_PLANE;
  m_src.width     = srcSize.width;
  m_src.height    = srcSize.height;
  m_src.nChannels = srcChannels;
  m_src.color     = srcColor;
  m_src.sampling  = srcSampling;
  m_src.precision = srcPrecision;

  if(m_src.precision > 10)
    m_optimal_htbl = 1;

  return JPEG_OK;
} // CJPEGEncoder::SetSource()


JERRCODE CJPEGEncoder::SetDestination(
  CBaseStreamOutput* pStreamOut)
{
  return m_BitStreamOut.Attach(pStreamOut);
} // CJPEGEncoder::SetDestination()


JERRCODE CJPEGEncoder::SetParams(
  JMODE  mode,
  JCOLOR color,
  JSS    sampling,
  int    restart_interval,
  int    interleaved,
  int    piecesCountInField,
  int    piecePosInField,
  int    numScan,
  int    piecePosInScan,
  int    huff_opt,
  int    quality,
  JTMODE threading_mode)
{
  int      i;
  int      width, height;
  JERRCODE jerr = JPEG_OK;

  if(JD_PLANE == m_src.order)
  {
    if(m_src.precision <= 8)
    {
      if(m_src.nChannels == 2 && color != m_src.color)
        return JPEG_ERR_PARAMS;

      if(m_src.nChannels == 3 && color != m_src.color)
        return JPEG_ERR_PARAMS;

      if(m_src.nChannels == 4 && color != m_src.color)
        return JPEG_ERR_PARAMS;
    }
    else
    {
      if(m_src.nChannels == 3 && color != m_src.color && m_src.sampling != JS_444)
        return JPEG_ERR_PARAMS;

      if(m_src.nChannels == 4 && color != m_src.color && m_src.sampling != JS_444)
        return JPEG_ERR_PARAMS;
    }
  }

  // common params
  m_jpeg_mode             = mode;
  m_jpeg_color            = color;
  m_jpeg_sampling         = sampling;

  // baseline/progressive specific params
  m_jpeg_quality          = quality;
  m_optimal_htbl          = huff_opt;
  m_jpeg_precision        = m_src.precision;

  // threading params
  m_piecePosInField       = piecePosInField;
  m_piecePosInScan        = piecePosInScan;
  m_piecesCountInField    = piecesCountInField;

  switch(m_jpeg_sampling)
  {
  case JS_411:
      m_mcuWidth  = 32;
      m_mcuHeight = 8;
      break;
  case JS_420:
      m_mcuWidth  = 16;
      m_mcuHeight = 16;
      break;
  case JS_422H:
      m_mcuWidth  = 16;
      m_mcuHeight = 8;
      break;
  case JS_422V:
      m_mcuWidth  = 8;
      m_mcuHeight = 16;
      break;
  case JS_444:
      m_mcuWidth  = 8;
      m_mcuHeight = 8;
      break;
  default:
      return JPEG_ERR_PARAMS;
  }

  m_numxMCU = (m_src.width  + (m_mcuWidth  - 1)) / m_mcuWidth;
  m_numyMCU = (m_src.height + (m_mcuHeight - 1)) / m_mcuHeight;

  m_xPadding = m_numxMCU * m_mcuWidth  - m_src.width;
  m_yPadding = m_numyMCU * m_mcuHeight - m_src.height;

  if(interleaved)
  {
      m_num_scans = 1;

      m_curr_scan.mcuWidth  = m_mcuWidth;
      m_curr_scan.mcuHeight = m_mcuHeight;

      m_curr_scan.ncomps = (m_jpeg_color == JC_GRAY) ? 1 : 3;
      m_curr_scan.first_comp = 0;

      m_curr_scan.numxMCU = m_numxMCU;
      m_curr_scan.numyMCU = m_numyMCU;

      m_curr_scan.xPadding = m_xPadding;
      m_curr_scan.yPadding = m_yPadding;
  }
  else
  {
      m_num_scans = (m_jpeg_color == JC_GRAY) ? 1 : 3;

      m_curr_scan.mcuWidth = 8;
      m_curr_scan.mcuHeight = 8;

      m_curr_scan.ncomps = 1;
      m_curr_scan.first_comp = numScan;

      if(m_curr_scan.first_comp != 0)
      {
          switch(m_jpeg_sampling)
          {
          case JS_411:
              width  = (m_src.width + 3) / 4;
              height = m_src.height;
              break;
          case JS_420:
              width  = (m_src.width + 1) / 2;
              height = (m_src.height + 1) / 2;
              break;
          case JS_422H:
              width  = (m_src.width + 1) / 2;
              height = m_src.height;
              break;
          case JS_422V:
              width  = m_src.width;
              height = (m_src.height + 1) / 2;
              break;
          case JS_444:
              width  = m_src.width;
              height = m_src.height;
              break;
          default:
              return JPEG_ERR_PARAMS;
          }
      }
      else
      {
          width  = m_src.width;
          height = m_src.height;
      }

      m_curr_scan.numxMCU = (width  + (m_curr_scan.mcuWidth  - 1)) / m_curr_scan.mcuWidth;
      m_curr_scan.numyMCU = (height + (m_curr_scan.mcuHeight - 1)) / m_curr_scan.mcuHeight;

      m_curr_scan.xPadding = m_curr_scan.numxMCU * m_curr_scan.mcuWidth  - width;
      m_curr_scan.yPadding = m_curr_scan.numyMCU * m_curr_scan.mcuHeight - height;
  }

  m_jpeg_restart_interval = restart_interval;

  if(m_jpeg_restart_interval)
  {
      m_mcu_encoded = m_jpeg_restart_interval * m_piecePosInScan;
      m_mcu_to_encode = m_jpeg_restart_interval;
  }
  else
  {
      m_mcu_encoded = 0;
      m_mcu_to_encode = m_curr_scan.numxMCU * m_curr_scan.numyMCU;
  }

  if(m_mcu_encoded + m_mcu_to_encode > (uint32_t)(m_curr_scan.numxMCU * m_curr_scan.numyMCU))
  {
      m_mcu_to_encode = m_curr_scan.numxMCU * m_curr_scan.numyMCU - m_mcu_encoded;
  }

  m_threading_mode = threading_mode;
  {
    m_num_rsti   = 0;
    m_rstiHeight = 1;

    m_threading_mode = JT_OLD;
  }

  switch(m_jpeg_color)
  {
  case JC_GRAY:  m_jpeg_ncomp = 1; break;
  case JC_RGB:   m_jpeg_ncomp = 3; break;
  case JC_YCBCR: m_jpeg_ncomp = 3; break;
  case JC_CMYK:  m_jpeg_ncomp = 4; break;
  case JC_YCCK:  m_jpeg_ncomp = 4; break;
  default:       m_jpeg_ncomp = std::min(MAX_COMPS_PER_SCAN,m_src.nChannels); break;
  }

  int id[4] = { 0, 1, 1, 0 };

  for(i = 0; i < m_jpeg_ncomp; i++)
  {
    if(m_externalQuantTable)
    {
      switch(GetNumQuantTables())
      {
      case 1:
          jerr = AttachQuantTable(id[0], i);
          break;
      case 2:
          jerr = AttachQuantTable(id[i], i);
          break;
      case 3:
          int tmp = id[2];
          id[2] = 2;
          jerr = AttachQuantTable(id[i], i);
          id[2] = tmp;
          break;
      }
    }
    else
    {
      if(m_jpeg_color == JC_YCBCR || m_jpeg_color == JC_YCCK || m_jpeg_color == JC_NV12)
        jerr = AttachQuantTable(id[i], i);
      else
        jerr = AttachQuantTable(id[0], i);
    }

    if(JPEG_OK != jerr)
      return jerr;

    if(m_externalHuffmanTable)
    {
      switch(GetNumACTables())
      {
      case 1:
          jerr = AttachHuffmanTable(id[0], AC, i);
          break;
      case 2:
          jerr = AttachHuffmanTable(id[i], AC, i);
          break;
      }

      if(JPEG_OK != jerr)
        return jerr;

      switch(GetNumDCTables())
      {
      case 1:
          jerr = AttachHuffmanTable(id[0], DC, i);
          break;
      case 2:
          jerr = AttachHuffmanTable(id[i], DC, i);
          break;
      }

      if(JPEG_OK != jerr)
        return jerr;
    }
    else
    {
      if(m_jpeg_color == JC_YCBCR || m_jpeg_color == JC_YCCK || m_jpeg_color == JC_NV12)
      {
        jerr = AttachHuffmanTable(id[i], DC, i);
        if(JPEG_OK != jerr)
          return jerr;

        jerr = AttachHuffmanTable(id[i], AC, i);
        if(JPEG_OK != jerr)
          return jerr;
      }
      else
      {
        jerr = AttachHuffmanTable(id[0], DC, i);
        if(JPEG_OK != jerr)
          return jerr;

        jerr = AttachHuffmanTable(id[0], AC, i);
        if(JPEG_OK != jerr)
          return jerr;
      }
    }
  }

  return JPEG_OK;
} // CJPEGEncoder::SetParams()


JERRCODE CJPEGEncoder::SetParams(
  JMODE  mode,
  JCOLOR color,
  JSS    sampling,
  int    restart_interval,
  int    huff_opt,
  int    point_transform,
  int    predictor)
{
  int      c, i;
  JERRCODE jerr;

  if(JPEG_LOSSLESS != mode)
    return JPEG_ERR_PARAMS;

  if(JS_444 != sampling)
    return JPEG_NOT_IMPLEMENTED;

  m_jpeg_mode             = mode;
  m_jpeg_color            = color;
  m_jpeg_sampling         = sampling;
  m_jpeg_restart_interval = restart_interval;
  // lossless specific params
  m_pt                    = point_transform;
  m_predictor             = predictor;

  m_optimal_htbl          = m_src.precision > 11 ? 1 : huff_opt;

  m_jpeg_precision = m_src.precision;

  m_mcuWidth  = 1;
  m_mcuHeight = 1;

  m_numxMCU = (m_src.width  + (m_mcuWidth  - 1)) / m_mcuWidth;
  m_numyMCU = (m_src.height + (m_mcuHeight - 1)) / m_mcuHeight;

  switch(m_jpeg_color)
  {
  case JC_GRAY:  m_jpeg_ncomp = 1; break;
  case JC_RGB:   m_jpeg_ncomp = 3; break;
  case JC_YCBCR: m_jpeg_ncomp = 3; break;
  case JC_CMYK:  m_jpeg_ncomp = 4; break;
  case JC_YCCK:  m_jpeg_ncomp = 4; break;
  default:       m_jpeg_ncomp = std::min(MAX_COMPS_PER_SCAN,m_src.nChannels); break;
  }

  if(!m_optimal_htbl)
  {
    jerr = m_dctbl[0].Create();
    if(JPEG_OK != jerr)
      return jerr;

    jerr = InitHuffmanTable((uint8_t*)DefaultLuminanceDCBits, (uint8_t*)DefaultLuminanceDCValues, 0, DC);
    if(JPEG_OK != jerr)
      return jerr;

    // lossless mode defaults is one huffman table for all JPEG color components
    for(i = 0; i < m_jpeg_ncomp; i++)
    {
      jerr = AttachHuffmanTable(0, DC, i);
      if(JPEG_OK != jerr)
        return jerr;
    }
  }
  else
  {
    if (m_jpeg_ncomp > MAX_HUFF_TABLES)
      return JPEG_ERR_PARAMS;
    for(c = 0; c < m_jpeg_ncomp; c++)
    {
      jerr = m_dctbl[c].Create();
      if(JPEG_OK != jerr)
        return jerr;

      // optimal huff table for each JPEG color component
      jerr = AttachHuffmanTable(c, DC, c);
      if(JPEG_OK != jerr)
        return jerr;
    }
  }
  return JPEG_OK;
} // CJPEGEncoder::SetParams()


JERRCODE CJPEGEncoder::InitHuffmanTable(
  uint8_t      bits[16],
  uint8_t      vals[256],
  int        tbl_id,
  HTBL_CLASS tbl_class)
{
  JERRCODE jerr;

  if(MAX_HUFF_TABLES <= tbl_id)
    return JPEG_ERR_PARAMS;

  switch(tbl_class)
  {
  case DC:
    jerr = m_dctbl[tbl_id].Init(tbl_id,tbl_class,bits,vals);
    break;

  case AC:
    jerr = m_actbl[tbl_id].Init(tbl_id,tbl_class,bits,vals);
    break;

  default:
    return JPEG_ERR_PARAMS;
  }

  return jerr;
} // CJPEGEncoder::InitHuffmanTable()


JERRCODE CJPEGEncoder::InitQuantTable(
  uint8_t qnt[64],
  int   tbl_id,
  int   quality)
{
  if(MAX_QUANT_TABLES <= tbl_id)
    return JPEG_ERR_PARAMS;

  return m_qntbl[tbl_id].Init(tbl_id,qnt,quality);
} // CJPEGEncoder::InitQuantTable()


JERRCODE CJPEGEncoder::InitQuantTable(
  uint16_t qnt[64],
  int    tbl_id,
  int    quality)
{
  if(MAX_QUANT_TABLES <= tbl_id)
    return JPEG_ERR_PARAMS;

  return m_qntbl[tbl_id].Init(tbl_id,qnt,quality);
} // CJPEGEncoder::InitQuantTable()


JERRCODE CJPEGEncoder::AttachHuffmanTable(int tbl_id, HTBL_CLASS tbl_class, int comp_no)
{
  if(MAX_HUFF_TABLES < tbl_id)
    return JPEG_ERR_PARAMS;

  if(comp_no > m_jpeg_ncomp)
    return JPEG_ERR_PARAMS;

  switch(tbl_class)
  {
  case DC:
    m_ccomp[comp_no].m_dc_selector = tbl_id;
    break;

  case AC:
    m_ccomp[comp_no].m_ac_selector = tbl_id;
    break;

  default:
    return JPEG_ERR_PARAMS;
  }

  return JPEG_OK;
} // CJPEGEncoder::AttachHuffmanTable()


JERRCODE CJPEGEncoder::AttachQuantTable(int tbl_id, int comp_no)
{
  if(MAX_QUANT_TABLES < tbl_id)
    return JPEG_ERR_PARAMS;

  if(comp_no > m_jpeg_ncomp)
    return JPEG_ERR_PARAMS;

  m_ccomp[comp_no].m_q_selector = tbl_id;

  return JPEG_OK;
} // CJPEGEncoder::AttachQuantTable()


uint16_t CJPEGEncoder::GetNumQuantTables()
{
    uint16_t numTables = 0;

    for(int i=0; i<MAX_QUANT_TABLES; i++)
        if(m_qntbl[i].m_initialized)
            numTables++;

    return numTables;
} // CJPEGEncoder::GetNumQuantTables()


JERRCODE CJPEGEncoder::FillQuantTable(int numTable, uint16_t* pTable)
{
    uint8_t* qtbl = m_qntbl[numTable].m_raw8u;

    for(int i=0; i<DCTSIZE2; i++)
        pTable[i] = (uint16_t)qtbl[i];

    return JPEG_OK;
} // CJPEGEncoder::FillQuantTable(int numTable, uint16_t* pTable)


uint16_t CJPEGEncoder::GetNumACTables()
{
    uint16_t numTables = 0;

    for(int i=0; i<MAX_HUFF_TABLES; i++)
        if(m_actbl[i].IsValid())
            numTables++;

    return numTables;
} // CJPEGEncoder::GetNumACTables()


JERRCODE CJPEGEncoder::FillACTable(int numTable, uint8_t* pBits, uint8_t* pValues)
{
    const uint8_t* bits = m_actbl[numTable].m_bits;
    const uint8_t* values = m_actbl[numTable].m_vals;

    MFX_INTERNAL_CPY(pBits, bits, 16);
    MFX_INTERNAL_CPY(pValues, values, 162);

    return JPEG_OK;
} // CJPEGEncoder::FillACTable(int numTable, uint8_t* pBits, uint8_t* pValues)


uint16_t CJPEGEncoder::GetNumDCTables()
{
    uint16_t numTables = 0;

    for(int i=0; i<MAX_HUFF_TABLES; i++)
        if(m_dctbl[i].IsValid())
            numTables++;

    return numTables;
} // CJPEGEncoder::GetNumDCTables()


JERRCODE CJPEGEncoder::FillDCTable(int numTable, uint8_t* pBits, uint8_t* pValues)
{
    const uint8_t* bits = m_dctbl[numTable].m_bits;
    const uint8_t* values = m_dctbl[numTable].m_vals;

    MFX_INTERNAL_CPY(pBits, bits, 16);
    MFX_INTERNAL_CPY(pValues, values, 12);

    return JPEG_OK;
} // CJPEGEncoder::FillDCTable(int numTable, uint8_t* pBits, uint8_t* pValues)


JERRCODE CJPEGEncoder::SetQuantTable(int numTable, uint16_t* pTable)
{
    m_externalQuantTable = true;

    uint8_t raw[DCTSIZE2];

    for(int i=0; i<DCTSIZE2; i++)
        raw[i] = (uint8_t)pTable[i];

    return m_qntbl[numTable].Init(numTable,raw,50);
} // CJPEGEncoder::SetQuantTable(int numTable, uint16_t* pTable)


JERRCODE CJPEGEncoder::SetACTable(int numTable, uint8_t* pBits, uint8_t* pValues)
{
    m_externalHuffmanTable = true;

    JERRCODE jerr = JPEG_OK;

    jerr = m_actbl[numTable].Create();
    if(JPEG_OK != jerr)
        return jerr;

    jerr = InitHuffmanTable(pBits, pValues, numTable, AC);
    if(JPEG_OK != jerr)
        return jerr;

    return JPEG_OK;
} // CJPEGEncoder::SetACTable(int numTable, uint8_t* pBits, uint8_t* pValues)


JERRCODE CJPEGEncoder::SetDCTable(int numTable, uint8_t* pBits, uint8_t* pValues)
{
    m_externalHuffmanTable = true;

    JERRCODE jerr = JPEG_OK;

    jerr = m_dctbl[numTable].Create();
    if(JPEG_OK != jerr)
        return jerr;

    jerr = InitHuffmanTable(pBits, pValues, numTable, DC);
    if(JPEG_OK != jerr)
        return jerr;

    return JPEG_OK;
} // CJPEGEncoder::SetDCTable(int numTable, uint8_t* pBits, uint8_t* pValues)


JERRCODE CJPEGEncoder::SetDefaultQuantTable(uint16_t quality)
{
    JERRCODE jerr = JPEG_OK;

    if(!quality)
        return JPEG_ERR_PARAMS;

    m_externalQuantTable = false;

    jerr = InitQuantTable((uint8_t*)DefaultLuminanceQuant, 0, quality);
    if(JPEG_OK != jerr)
    {
      LOG0("Error: can't attach quant table");
      return jerr;
    }

    jerr = InitQuantTable((uint8_t*)DefaultChrominanceQuant, 1, quality);
    if(JPEG_OK != jerr)
    {
      LOG0("Error: can't attach quant table");
      return jerr;
    }

    return JPEG_OK;
} // CJPEGEncoder::SetDefaultQuantTable(uint16_t quality)


JERRCODE CJPEGEncoder::SetDefaultACTable()
{
    m_externalHuffmanTable = false;

    JERRCODE jerr = JPEG_OK;

    jerr = m_actbl[0].Create();
    if(JPEG_OK != jerr)
      return jerr;

    jerr = InitHuffmanTable((uint8_t*)DefaultLuminanceACBits, (uint8_t*)DefaultLuminanceACValues, 0, AC);
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_actbl[1].Create();
    if(JPEG_OK != jerr)
      return jerr;

    jerr = InitHuffmanTable((uint8_t*)DefaultChrominanceACBits, (uint8_t*)DefaultChrominanceACValues, 1, AC);
    if(JPEG_OK != jerr)
      return jerr;

    return JPEG_OK;
} // CJPEGEncoder::SetDefaultACTable()


JERRCODE CJPEGEncoder::SetDefaultDCTable()
{
    m_externalHuffmanTable = false;

    JERRCODE jerr = JPEG_OK;

    jerr = m_dctbl[0].Create();
    if(JPEG_OK != jerr)
      return jerr;

    jerr = InitHuffmanTable((uint8_t*)DefaultLuminanceDCBits, (uint8_t*)DefaultLuminanceDCValues, 0, DC);
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_dctbl[1].Create();
    if(JPEG_OK != jerr)
      return jerr;

    jerr = InitHuffmanTable((uint8_t*)DefaultChrominanceDCBits, (uint8_t*)DefaultChrominanceDCValues, 1, DC);
    if(JPEG_OK != jerr)
      return jerr;

    return JPEG_OK;
} // CJPEGEncoder::SetDefaultDCTable()


bool CJPEGEncoder::IsQuantTableInited()
{
    return m_qntbl[0].m_initialized;
} // CJPEGEncoder::IsQuantTableInited()


bool CJPEGEncoder::IsACTableInited()
{
    return m_actbl[0].IsValid();
} // CJPEGEncoder::IsACTableInited()


bool CJPEGEncoder::IsDCTableInited()
{
    return m_dctbl[0].IsValid();
} // CJPEGEncoder::IsDCTableInited()


JERRCODE CJPEGEncoder::WriteSOI(void)
{
  JERRCODE jerr;

  TRC0("-> WriteSOI");

  TRC1("  emit marker ",JM_SOI);

  jerr = m_BitStreamOut.WriteWord(0xff00 | JM_SOI);
  if(JPEG_OK != jerr)
    return jerr;

  return JPEG_OK;
} // CJPEGEncoder::WriteSOI()


JERRCODE CJPEGEncoder::WriteEOI(void)
{
  JERRCODE jerr;

  TRC0("-> WriteEOI");

  TRC1("emit marker ",JM_EOI);

  jerr = m_BitStreamOut.WriteWord(0xff00 | JM_EOI);
  if(JPEG_OK != jerr)
    return jerr;

  return JPEG_OK;
} // CJPEGEncoder::WriteEOI()


JERRCODE CJPEGEncoder::WriteAPP0(void)
{
  int len;
  JERRCODE jerr;

  TRC0("-> WriteAPP0");

  len = 2 + 5 + 2 + 1 + 2 + 2 + 1 + 1;

  TRC1("  emit marker ",JM_APP0);
  TRC1("    length ",len);

  jerr = m_BitStreamOut.WriteWord(0xff00 | JM_APP0);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteWord(len);
  if(JPEG_OK != jerr)
    return jerr;

  // identificator JFIF
  jerr = m_BitStreamOut.WriteByte('J');
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteByte('F');
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteByte('I');
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteByte('F');
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteByte(0);
  if(JPEG_OK != jerr)
    return jerr;

  // version
  jerr = m_BitStreamOut.WriteWord(0x0102);
  if(JPEG_OK != jerr)
    return jerr;

  // units: 0 - none, 1 - dot per inch, 2 - dot per cm
  jerr = m_BitStreamOut.WriteByte(m_jfif_app0_units);
  if(JPEG_OK != jerr)
    return jerr;

  // xDensity
  jerr = m_BitStreamOut.WriteWord(m_jfif_app0_xDensity);
  if(JPEG_OK != jerr)
    return jerr;

  // yDensity
  jerr = m_BitStreamOut.WriteWord(m_jfif_app0_yDensity);
  if(JPEG_OK != jerr)
    return jerr;

  // xThumbnails, yThumbnails
  jerr = m_BitStreamOut.WriteByte(0);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteByte(0);
  if(JPEG_OK != jerr)
    return jerr;

  return JPEG_OK;
} // CJPEGEncoder::WriteAPP0()


JERRCODE CJPEGEncoder::WriteAPP14(void)
{
  int len;
  JERRCODE jerr;

  TRC0("-> WriteAPP14");

  len = 2 + 5 + 2 + 2 + 2 + 1;

  TRC1("  emit marker ",JM_APP14);
  TRC1("    length ",len);

  jerr = m_BitStreamOut.WriteWord(0xff00 | JM_APP14);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteWord(len);
  if(JPEG_OK != jerr)
    return jerr;

  // identificator Adobe
  jerr = m_BitStreamOut.WriteByte('A');
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteByte('d');
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteByte('o');
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteByte('b');
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteByte('e');
  if(JPEG_OK != jerr)
    return jerr;

  // version
  jerr = m_BitStreamOut.WriteWord(100);
  if(JPEG_OK != jerr)
    return jerr;

  // Flags 0
  jerr = m_BitStreamOut.WriteWord(0);
  if(JPEG_OK != jerr)
    return jerr;

  // Flags 1
  jerr = m_BitStreamOut.WriteWord(0);
  if(JPEG_OK != jerr)
    return jerr;

  switch(m_jpeg_color)
  {
  case JC_YCBCR:
    jerr = m_BitStreamOut.WriteByte(1);
    break;

  case JC_YCCK:
    jerr = m_BitStreamOut.WriteByte(2);
    break;

  default:
    jerr = m_BitStreamOut.WriteByte(0);
    break;
  }

  if(JPEG_OK != jerr)
    return jerr;

  return JPEG_OK;
} // CJPEGEncoder::WriteAPP14()


JERRCODE CJPEGEncoder::WriteCOM(
  char*  comment)
{
  JERRCODE jerr;

  TRC0("-> WriteCOM");

  std::string str("Intel(R) Media SDK JPEG encoder");

  if(comment)
  {
    str += "; ";
    str.append(comment, strnlen(comment, 127));
  }

  // write 2 words, buf content and zero byte
  size_t len = str.size() + 1;
  const char* ptr = str.c_str();

  TRC1("  emit marker ",JM_COM);
  TRC1("    length ",len + 2);

  jerr = m_BitStreamOut.WriteWord(0xff00 | JM_COM);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteWord((int)len + 2);
  if(JPEG_OK != jerr)
    return jerr;

  for(size_t i = 0; i < len; i++)
  {
    jerr = m_BitStreamOut.WriteByte(ptr[i]);
    if(JPEG_OK != jerr)
      return jerr;
  }

  return JPEG_OK;
} // CJPEGEncoder::WriteCOM()


JERRCODE CJPEGEncoder::WriteDQT(
  CJPEGEncoderQuantTable* qtbl)
{
  int i;
  int len;
  JERRCODE jerr;

  TRC0("-> WriteDQT");

  len = DCTSIZE2 + 2 + 1;

  if(qtbl->m_precision)
    len += DCTSIZE2;

  TRC1("  emit marker ",JM_DQT);
  TRC1("    length ",len);

  jerr = m_BitStreamOut.WriteWord(0xff00 | JM_DQT);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteWord(len);
  if(JPEG_OK != jerr)
    return jerr;

  // precision/id
  jerr = m_BitStreamOut.WriteByte((qtbl->m_precision << 4) | qtbl->m_id);
  if(JPEG_OK != jerr)
    return jerr;

  TRC1("  id ",qtbl->m_id);
  TRC1("  precision ",qtbl->m_precision);

  TRC(endl);
  for(i = 0; i < DCTSIZE2; i++)
  {
    TRC(" ");
    TRC((int)qtbl->m_raw[i]);
    if(i % 8 == 7)
    {
      TRC(endl);
    }
  }
  TRC(endl);

  for(i = 0; i < DCTSIZE2; i++)
  {
    if(qtbl->m_precision == 0)
    {
      jerr = m_BitStreamOut.WriteByte(qtbl->m_raw8u[i]);
    }
    else
    {
      jerr = m_BitStreamOut.WriteWord(qtbl->m_raw16u[i]);
    }
    if(JPEG_OK != jerr)
      return jerr;
  }

  return JPEG_OK;
} // CJPEGEncoder::WriteDQT()


JERRCODE CJPEGEncoder::WriteDHT(
  CJPEGEncoderHuffmanTable* htbl)
{
  int i;
  int len;
  int htbl_len;
  JERRCODE jerr;

  TRC0("-> WriteDHT");

  for(htbl_len = 0, i = 0; i < 16; i++)
  {
    htbl_len += htbl->m_bits[i];
  }

  len = 16 + htbl_len + 2 + 1;

  TRC1("  emit marker ",JM_DHT);
  TRC1("    length ",len);

  jerr = m_BitStreamOut.WriteWord(0xff00 | JM_DHT);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteWord(len);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteByte((htbl->m_hclass << 4) | htbl->m_id);
  if(JPEG_OK != jerr)
    return jerr;

  TRC1("  id ",htbl->m_id);
  TRC1("  class ",htbl->m_hclass);

  for(i = 0; i < 16; i++)
  {
    jerr = m_BitStreamOut.WriteByte(htbl->m_bits[i]);
    if(JPEG_OK != jerr)
      return jerr;
  }

  for(i = 0; i < htbl_len; i++)
  {
    jerr = m_BitStreamOut.WriteByte(htbl->m_vals[i]);
    if(JPEG_OK != jerr)
      return jerr;
  }

  return JPEG_OK;
} // CJPEGEncoder::WriteDHT()


JERRCODE CJPEGEncoder::WriteSOF0(void)
{
  int len;
  JERRCODE jerr;

  TRC0("-> WriteSOF0");

  len = 8 + m_jpeg_ncomp * 3;

  TRC1("  emit marker ",JM_SOF0);
  TRC1("    length ",len);

  jerr = m_BitStreamOut.WriteWord(0xff00 | JM_SOF0);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteWord(len);
  if(JPEG_OK != jerr)
    return jerr;

  // sample precision
  jerr = m_BitStreamOut.WriteByte(8);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteWord(m_src.height);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteWord(m_src.width);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteByte(m_jpeg_ncomp);
  if(JPEG_OK != jerr)
    return jerr;

  for(int i = 0; i < m_jpeg_ncomp; i++)
  {
    jerr = m_BitStreamOut.WriteByte(i+1);
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_BitStreamOut.WriteByte((m_ccomp[i].m_hsampling << 4) | m_ccomp[i].m_vsampling);
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_BitStreamOut.WriteByte(m_ccomp[i].m_q_selector);
    if(JPEG_OK != jerr)
      return jerr;
  }

  return JPEG_OK;
} // CJPEGEncoder::WriteSOF0()


JERRCODE CJPEGEncoder::WriteSOF1(void)
{
  int len;
  JERRCODE jerr;

  TRC0("-> WriteSOF1");

  len = 8 + m_jpeg_ncomp * 3;

  TRC1("  emit marker ",JM_SOF1);
  TRC1("    length ",sof1_len);

  jerr = m_BitStreamOut.WriteWord(0xff00 | JM_SOF1);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteWord(len);
  if(JPEG_OK != jerr)
    return jerr;

  // sample precision
  jerr = m_BitStreamOut.WriteByte(m_jpeg_precision);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteWord(m_src.height);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteWord(m_src.width);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteByte(m_jpeg_ncomp);
  if(JPEG_OK != jerr)
    return jerr;

  for(int i = 0; i < m_jpeg_ncomp; i++)
  {
    jerr = m_BitStreamOut.WriteByte(i+1);
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_BitStreamOut.WriteByte((m_ccomp[i].m_hsampling << 4) | m_ccomp[i].m_vsampling);
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_BitStreamOut.WriteByte(m_ccomp[i].m_q_selector);
    if(JPEG_OK != jerr)
      return jerr;
  }

  return JPEG_OK;
} // CJPEGEncoder::WriteSOF1()


JERRCODE CJPEGEncoder::WriteSOF2(void)
{
  int len;
  JERRCODE jerr;

  TRC0("-> WriteSOF2");

  len = 8 + m_jpeg_ncomp * 3;

  TRC1("  emit marker ",JM_SOF2);
  TRC1("    length ",len);

  jerr = m_BitStreamOut.WriteWord(0xff00 | JM_SOF2);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteWord(len);
  if(JPEG_OK != jerr)
    return jerr;

  // sample precision
  jerr = m_BitStreamOut.WriteByte(8);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteWord(m_src.height);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteWord(m_src.width);
  if(JPEG_OK != jerr)
    return jerr;

  TRC1("  height ",m_src.height);
  TRC1("  width  ",m_src.width);

  jerr = m_BitStreamOut.WriteByte(m_jpeg_ncomp);
  if(JPEG_OK != jerr)
    return jerr;

  for(int i = 0; i < m_jpeg_ncomp; i++)
  {
    jerr = m_BitStreamOut.WriteByte(i+1);
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_BitStreamOut.WriteByte((m_ccomp[i].m_hsampling << 4) | m_ccomp[i].m_vsampling);
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_BitStreamOut.WriteByte(m_ccomp[i].m_q_selector);
    if(JPEG_OK != jerr)
      return jerr;

    TRC1("    id ",i);
    TRC1("      h_sampling ",m_ccomp[i]->m_hsampling);
    TRC1("      v_sampling ",m_ccomp[i]->m_vsampling);
    TRC1("      q_selector ",m_ccomp[i]->m_q_selector);
  }

  return JPEG_OK;
} // CJPEGEncoder::WriteSOF2()


JERRCODE CJPEGEncoder::WriteSOF3(void)
{
  int len;
  JERRCODE jerr;

  TRC0("-> WriteSOF3");

  len = 8 + m_jpeg_ncomp * 3;

  TRC1("  emit marker ",JM_SOF3);
  TRC1("    length ",len);

  jerr = m_BitStreamOut.WriteWord(0xff00 | JM_SOF3);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteWord(len);
  if(JPEG_OK != jerr)
    return jerr;

  // sample precision
  jerr = m_BitStreamOut.WriteByte(m_jpeg_precision);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteWord(m_src.height);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteWord(m_src.width);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteByte(m_jpeg_ncomp);
  if(JPEG_OK != jerr)
    return jerr;

  for(int i = 0; i < m_jpeg_ncomp; i++)
  {
    jerr = m_BitStreamOut.WriteByte(i+1);
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_BitStreamOut.WriteByte((m_ccomp[i].m_hsampling << 4) | m_ccomp[i].m_vsampling);
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_BitStreamOut.WriteByte(m_ccomp[i].m_q_selector);
    if(JPEG_OK != jerr)
      return jerr;
  }

  return JPEG_OK;
} // CJPEGEncoder::WriteSOF3()


JERRCODE CJPEGEncoder::WriteDRI(
  int    restart_interval)
{
  int len;
  JERRCODE jerr;

  TRC0("-> WriteDRI");

  len = 2 + 2;

  TRC1("  emit marker ",JM_DRI);
  TRC1("    length ",len);

  jerr = m_BitStreamOut.WriteWord(0xff00 | JM_DRI);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteWord(len);
  if(JPEG_OK != jerr)
    return jerr;

  // emit restart interval
  if(JT_OLD == m_threading_mode)
  {
    jerr = m_BitStreamOut.WriteWord(restart_interval);
  }
  if(JPEG_OK != jerr)
    return jerr;

  TRC1("  restart ",restart_interval);

  return JPEG_OK;
} // CJPEGEncoder::WriteDRI()


JERRCODE CJPEGEncoder::WriteRST(
  int    next_restart_num)
{
  JERRCODE jerr;

  TRC0("-> WriteRST");

  TRC1("  emit marker ",JM_RST0 + next_restart_num);
  TRC1("    RST ",0xfff0 | (JM_RST0 + next_restart_num));

  // emit restart interval
  jerr = m_BitStreamOut.WriteWord(0xff00 | (JM_RST0 + next_restart_num));
  if(JPEG_OK != jerr)
    return jerr;

  // Update next-restart state
  //m_next_restart_num = (next_restart_num + 1) & 7;

  return JPEG_OK;
} // CJPEGEncoder::WriteRST()


JERRCODE CJPEGEncoder::ProcessRestart(
  int    id[MAX_COMPS_PER_SCAN],
  int    Ss,
  int    Se,
  int    Ah,
  int    Al)
{
  int       dstLen;
  int       currPos;
  uint8_t*    dst;
  JERRCODE  jerr;
  int status = ippStsNoErr;

  TRC0("-> ProcessRestart");

  dst    = m_BitStreamOut.GetDataPtr();
  dstLen = m_BitStreamOut.GetDataLen();

  jerr = m_BitStreamOut.FlushBuffer();
  if(JPEG_OK != jerr)
    return jerr;

  currPos = m_BitStreamOut.GetCurrPos();

  // flush IppiEncodeHuffmanState
  switch(m_jpeg_mode)
  {
  case JPEG_BASELINE:
  case JPEG_EXTENDED:
    {
      status = mfxiEncodeHuffman8x8_JPEG_16s1u_C1(
                 0,dst,dstLen,&currPos,0,0,0,m_state,1);
      break;
    }

  case JPEG_PROGRESSIVE:
    if(Ss == 0 && Se == 0)
    {
      // DC scan
      if(Ah == 0)
      {
        status = mfxiEncodeHuffman8x8_DCFirst_JPEG_16s1u_C1(
                   0,dst,dstLen,&currPos,0,0,0,m_state,1);
      }
      else
      {
        status = mfxiEncodeHuffman8x8_DCRefine_JPEG_16s1u_C1(
                   0,dst,dstLen,&currPos,0,m_state,1);
      }
    }
    else
    {
      // AC scan
      IppiEncodeHuffmanSpec* actbl = m_actbl[m_ccomp[id[0]].m_ac_selector];

      if(Ah == 0)
      {
        status = mfxiEncodeHuffman8x8_ACFirst_JPEG_16s1u_C1(
                   0,dst,dstLen,&currPos,Ss,Se,Al,actbl,m_state,1);
      }
      else
      {
        status = mfxiEncodeHuffman8x8_ACRefine_JPEG_16s1u_C1(
                   0,dst,dstLen,&currPos,Ss,Se,Al,actbl,m_state,1);
      }
    }
    break;

  case JPEG_LOSSLESS:
    status = mfxiEncodeHuffmanOne_JPEG_16s1u_C1(
               0,dst,dstLen,&currPos,0,m_state,1);
    break;
  default:
    break;
  }

  m_BitStreamOut.SetCurrPos(currPos);

  if(ippStsNoErr > status)
  {
    LOG1("IPP Error: mfxiEncodeHuffman8x8_JPEG_16s1u_C1() failed - ",status);
    return JPEG_ERR_INTERNAL;
  }

  status = mfxiEncodeHuffmanStateInit_JPEG_8u(m_state);
  if(ippStsNoErr != status)
  {
    return JPEG_ERR_INTERNAL;
  }

  //jerr = WriteRST(m_next_restart_num);
  //if(JPEG_OK != jerr)
  //{
  //  LOG1("IPP Error: WriteRST() failed - ",jerr);
  //  return jerr;
  //}

  for(int c = 0; c < m_jpeg_ncomp; c++)
  {
    m_ccomp[c].m_lastDC = 0;
  }

  //m_restarts_to_go = m_jpeg_restart_interval;

  return JPEG_OK;
} // CJPEGEncoder::ProcessRestart()


JERRCODE CJPEGEncoder::ProcessRestart(
  int    stat[2][256],
  int    id[MAX_COMPS_PER_SCAN],
  int    Ss,
  int    Se,
  int    Ah,
  int    Al)
{
  int status;

  TRC0("-> ProcessRestart");

  // flush IppiEncodeHuffmanState
  if(JPEG_PROGRESSIVE == m_jpeg_mode)
  {
    if(Ss == 0 && Se == 0)
    {
      // DC scan
      // nothing to do
    }
    else
    {
      // AC scan

      if(Ah == 0)
      {
        status = mfxiGetHuffmanStatistics8x8_ACFirst_JPEG_16s_C1(
                   0,&stat[m_ccomp[id[0]].m_ac_selector][0],Ss,Se,Al,m_state,1);

        if(ippStsNoErr > status)
        {
          LOG1("IPP Error: mfxiGetHuffmanStatistics8x8_ACFirst_JPEG_16s_C1() failed - ",status);
          return JPEG_ERR_INTERNAL;
        }
      }
      else
      {
        status = mfxiGetHuffmanStatistics8x8_ACRefine_JPEG_16s_C1(
                   0,&stat[m_ccomp[id[0]].m_ac_selector][0],Ss,Se,Al,m_state,1);

        if(ippStsNoErr > status)
        {
          LOG1("IPP Error: mfxiGetHuffmanStatistics8x8_ACRefine_JPEG_16s_C1() failed - ",status);
          return JPEG_ERR_INTERNAL;
        }
      }
    }
  }

  status = mfxiEncodeHuffmanStateInit_JPEG_8u(m_state);
  if(ippStsNoErr != status)
  {
    return JPEG_ERR_INTERNAL;
  }

  for(int c = 0; c < m_jpeg_ncomp; c++)
  {
    m_ccomp[c].m_lastDC = 0;
  }

  //m_restarts_to_go = m_jpeg_restart_interval;

  return JPEG_OK;
} // CJPEGEncoder::ProcessRestart()


JERRCODE CJPEGEncoder::WriteSOS(void)
{
  int len;
  JERRCODE jerr;

  TRC0("-> WriteSOS");

  len = 3 + m_curr_scan.ncomps*2 + 3;

  TRC1("  emit marker ",JM_SOS);
  TRC1("    length ",len);

  jerr = m_BitStreamOut.WriteWord(0xff00 | JM_SOS);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteWord(len);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteByte(m_curr_scan.ncomps);
  if(JPEG_OK != jerr)
    return jerr;

  TRC1("  ncomp ", m_curr_scan.ncomps);

  for(int i = m_curr_scan.first_comp; i < m_curr_scan.first_comp + m_curr_scan.ncomps; i++)
  {
    jerr = m_BitStreamOut.WriteByte(i+1);
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_BitStreamOut.WriteByte((m_ccomp[i].m_dc_selector << 4) | m_ccomp[i].m_ac_selector);
    if(JPEG_OK != jerr)
      return jerr;

    TRC1("    id ",i);
    TRC1("      dc_selector ",m_ccomp[i].m_dc_selector);
    TRC1("      ac_selector ",m_ccomp[i].m_ac_selector);
  }

  jerr = m_BitStreamOut.WriteByte(m_ss); // Ss
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteByte(m_se); // se
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteByte(((m_ah << 4) | m_al));  // Ah/Al
  if(JPEG_OK != jerr)
    return jerr;

  TRC1("  Ss ",m_ss);
  TRC1("  Se ",m_se);
  TRC1("  Ah ",m_ah);
  TRC1("  Al ",m_al);

  return JPEG_OK;
} // CJPEGEncoder::WriteSOS()


JERRCODE CJPEGEncoder::WriteSOS(
  int    ncomp,
  int    id[MAX_COMPS_PER_SCAN],
  int    Ss,
  int    Se,
  int    Ah,
  int    Al)
{
  int len;
  JERRCODE jerr;

  TRC0("-> WriteSOS");

  len = 3 + ncomp*2 + 3;

  TRC1("  emit marker ",JM_SOS);
  TRC1("    length ",len);

  jerr = m_BitStreamOut.WriteWord(0xff00 | JM_SOS);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteWord(len);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteByte(ncomp);
  if(JPEG_OK != jerr)
    return jerr;

  TRC1("  ncomp ",ncomp);

  for(int i = 0; i < ncomp; i++)
  {
    jerr = m_BitStreamOut.WriteByte(id[i]);
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_BitStreamOut.WriteByte((m_ccomp[id[i]].m_dc_selector << 4) | m_ccomp[id[i]].m_ac_selector);
    if(JPEG_OK != jerr)
      return jerr;

    TRC1("    id ",id[i]);
    TRC1("    dc_selector ",m_ccomp[id[i]].m_dc_selector);
    TRC1("    ac_selector ",m_ccomp[id[i]].m_ac_selector);
  }

  jerr = m_BitStreamOut.WriteByte(Ss);       // Ss
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteByte(Se);       // Se
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_BitStreamOut.WriteByte(((Ah & 0x0f) << 4) | (Al & 0x0f));  // Ah/Al
  if(JPEG_OK != jerr)
    return jerr;

  TRC1("  Ss ",Ss);
  TRC1("  Se ",Se);
  TRC1("  Ah ",Ah);
  TRC1("  Al ",Al);

  return JPEG_OK;
} // CJPEGEncoder::WriteSOS()



JERRCODE CJPEGEncoder::Init(void)
{
  int       i;
  int       tr_buf_size = 0;
  CJPEGColorComponent* curr_comp;
  JERRCODE  jerr;

  m_num_threads = get_num_threads();

  m_ccWidth  = m_curr_scan.mcuWidth * m_curr_scan.numxMCU;
  m_ccHeight = m_curr_scan.mcuHeight;

  m_nblock = 0;

  for(m_nblock = 0, i = 0; i < m_jpeg_ncomp; i++)
  {
    curr_comp = &m_ccomp[i];

    curr_comp->m_id      = i;
    curr_comp->m_comp_no = i;

    switch(m_jpeg_sampling)
    {
    case JS_411:
        curr_comp->m_hsampling   = (i == 0) ? 4 : 1;
        curr_comp->m_vsampling   = 1;
        curr_comp->m_h_factor    = (m_num_scans != 1) ? 1 : (i == 0) ? 1 : 4;
        curr_comp->m_v_factor    = 1;
        break;
    case JS_420:
        curr_comp->m_hsampling   = (i == 0) ? 2 : 1;
        curr_comp->m_vsampling   = (i == 0) ? 2 : 1;
        curr_comp->m_h_factor    = (m_num_scans != 1) ? 1 : (i == 0) ? 1 : 2;
        curr_comp->m_v_factor    = (m_num_scans != 1) ? 1 : (i == 0) ? 1 : 2;
        break;
    case JS_422H:
        curr_comp->m_hsampling   = (i == 0) ? 2 : 1;
        curr_comp->m_vsampling   = 1;
        curr_comp->m_h_factor    = (m_num_scans != 1) ? 1 : (i == 0) ? 1 : 2;
        curr_comp->m_v_factor    = 1;
        break;
    case JS_422V:
        curr_comp->m_hsampling   = 1;
        curr_comp->m_vsampling   = (i == 0) ? 2 : 1;
        curr_comp->m_h_factor    = 1;
        curr_comp->m_v_factor    = (m_num_scans != 1) ? 1 : (i == 0) ? 1 : 2;
        break;
    case JS_444:
        curr_comp->m_hsampling   = 1;
        curr_comp->m_vsampling   = 1;
        curr_comp->m_h_factor    = 1;
        curr_comp->m_v_factor    = 1;
        break;
    default:
        return JPEG_ERR_PARAMS;
    }

    curr_comp->m_nblocks = curr_comp->m_hsampling * curr_comp->m_vsampling;

    m_nblock += curr_comp->m_nblocks;

    switch(m_jpeg_mode)
    {
    case JPEG_BASELINE:
      curr_comp->m_cc_height = m_curr_scan.mcuHeight;
      curr_comp->m_cc_step   = m_curr_scan.numxMCU * m_curr_scan.mcuWidth;
      break;

    default:
      return JPEG_ERR_PARAMS;
    }

    curr_comp->m_ss_height = curr_comp->m_cc_height / curr_comp->m_v_factor;
    curr_comp->m_ss_step   = curr_comp->m_cc_step   / curr_comp->m_h_factor;

    // color convert buffer
    jerr = curr_comp->CreateBufferCC(m_num_threads);
    if(JPEG_OK != jerr)
      return jerr;

    // sub-sampling buffer
    jerr = curr_comp->CreateBufferSS(m_num_threads);
    if(JPEG_OK != jerr)
      return jerr;

  } // for m_jpeg_ncomp

  if(JPEG_PROGRESSIVE == m_jpeg_mode)
  {
    LOG0("Error: JPEG_PROGRESSIVE is not supported in CJPEGEncoder::Init()");
    return JPEG_NOT_IMPLEMENTED;
  }

  if(m_num_scans != 1)
      m_nblock = 1;

  switch(m_jpeg_mode)
  {
  case JPEG_BASELINE:
    if(!m_optimal_htbl)
      tr_buf_size = m_curr_scan.numxMCU * m_nblock * DCTSIZE2 * sizeof(int16_t) * m_num_threads * m_rstiHeight;
    else
      tr_buf_size = m_curr_scan.numxMCU * m_curr_scan.numyMCU * m_nblock * DCTSIZE2 * sizeof(int16_t) * m_num_threads;
    break;

  default:
    return JPEG_ERR_PARAMS;
  }

  if(m_block_buffer_size < tr_buf_size)
  {
      if(m_block_buffer)
      {
          mfxFree(m_block_buffer);
          m_block_buffer = 0;
      }
      m_block_buffer_size = 0;
  }

  // MCUs buffer
  if(0 == m_block_buffer)
  {
    m_block_buffer = (int16_t*)mfxMalloc(tr_buf_size);
    if(0 == m_block_buffer)
    {
      return JPEG_ERR_ALLOC;
    }

    m_block_buffer_size = tr_buf_size;

    memset((uint8_t*)m_block_buffer, 0, tr_buf_size);
  }

  int buflen;

  buflen = (m_jpeg_mode == JPEG_LOSSLESS) ?
    std::max(ENC_DEFAULT_BUFLEN,m_numxMCU * m_jpeg_ncomp * 2 * 2) :
    ENC_DEFAULT_BUFLEN;

  jerr = m_BitStreamOut.Init(buflen);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = m_state.Create();
  if(JPEG_OK != jerr)
    return jerr;

  return JPEG_OK;
} // CJPEGEncoder::Init()


JERRCODE CJPEGEncoder::ColorConvert(uint32_t rowMCU, uint32_t colMCU, uint32_t maxMCU/*int nMCURow, int thread_id*/)
{
  int       cc_h;
  int       srcStep;
  int       macropixelSize = JD_PIXEL == m_src.order ? m_src.nChannels : 1;
  int       convert = 0;

  uint8_t*    pSrc8u  = 0;
  uint16_t*   pSrc16u = 0;
  mfxSize  roi;
  int status;

  cc_h = m_ccHeight;

  if(rowMCU == (uint32_t)m_numyMCU - 1)
  {
    cc_h = m_mcuHeight - m_yPadding;
  }

  roi.width  = (maxMCU - colMCU) * m_mcuWidth * m_ccomp[0].m_hsampling/*m_src.width*/;
  roi.height = cc_h;

  srcStep = m_src.lineStep[0];

  if(m_src.precision <= 8)
    pSrc8u  =                       m_src.p.Data8u[0]  + rowMCU * m_mcuHeight * srcStep + m_mcuWidth * colMCU * m_ccomp[0].m_hsampling * macropixelSize;
  else
    pSrc16u = (uint16_t*)((uint8_t*)m_src.p.Data16s[0] + rowMCU * m_mcuHeight * srcStep + m_mcuWidth * colMCU * m_ccomp[0].m_hsampling * macropixelSize);

  if(m_jpeg_color == JC_UNKNOWN && m_src.color == JC_UNKNOWN)
  {

    switch(m_jpeg_ncomp)
    {
    case 1:
      {
        int     dstStep;
        uint8_t*  pDst8u;
        uint16_t* pDst16u;

        dstStep = m_ccomp[0].m_cc_step;
        convert = 1;

        if(m_src.precision <= 8)
        {
          pDst8u = m_ccomp[0].GetCCBufferPtr(0/*thread_id*/);

          status = mfxiCopy_8u_C1R(pSrc8u,srcStep,pDst8u,dstStep,roi);
        }
        else
        {
          pDst16u = (uint16_t*)m_ccomp[0].GetCCBufferPtr(0/*thread_id*/);

          status = mfxiCopy_16s_C1R((int16_t*)pSrc16u,srcStep,(int16_t*)pDst16u,dstStep,roi);
        }

        if(ippStsNoErr != status)
        {
          LOG1("IPP Error: mfxiCopy_8u_C1R() failed - ",status);
          return JPEG_ERR_INTERNAL;
        }
      }
      break;

    case 3:
      {
        int     dstStep;
        uint8_t*  pDst8u[3];
        uint16_t* pDst16u[3];

        dstStep = m_ccomp[0].m_cc_step;
        convert = 1;

        if(m_src.precision <= 8)
        {
          pDst8u[0] = m_ccomp[0].GetCCBufferPtr(0/*thread_id*/);
          pDst8u[1] = m_ccomp[1].GetCCBufferPtr(0/*thread_id*/);
          pDst8u[2] = m_ccomp[2].GetCCBufferPtr(0/*thread_id*/);

          status = mfxiCopy_8u_C3P3R(pSrc8u,srcStep,pDst8u,dstStep,roi);
        }
        else
        {
          pDst16u[0] = (uint16_t*)m_ccomp[0].GetCCBufferPtr(0/*thread_id*/);
          pDst16u[1] = (uint16_t*)m_ccomp[1].GetCCBufferPtr(0/*thread_id*/);
          pDst16u[2] = (uint16_t*)m_ccomp[2].GetCCBufferPtr(0/*thread_id*/);

          status = mfxiCopy_16s_C3P3R((int16_t*)pSrc16u,srcStep,(int16_t**)pDst16u,dstStep,roi);
        }

        if(ippStsNoErr != status)
        {
          LOG1("IPP Error: mfxiCopy_8u_C3P3R() failed - ",status);
          return JPEG_ERR_INTERNAL;
        }
      }
      break;

    case 4:
      {
        int     dstStep;
        uint8_t*  pDst8u[4];
        uint16_t* pDst16u[4];

        dstStep = m_ccomp[0].m_cc_step;
        convert = 1;

        if(m_src.precision <= 8)
        {
          pDst8u[0] = m_ccomp[0].GetCCBufferPtr(0/*thread_id*/);
          pDst8u[1] = m_ccomp[1].GetCCBufferPtr(0/*thread_id*/);
          pDst8u[2] = m_ccomp[2].GetCCBufferPtr(0/*thread_id*/);
          pDst8u[3] = m_ccomp[3].GetCCBufferPtr(0/*thread_id*/);

          status = mfxiCopy_8u_C4P4R(pSrc8u,srcStep,pDst8u,dstStep,roi);
        }
        else
        {
          pDst16u[0] = (uint16_t*)m_ccomp[0].GetCCBufferPtr(0/*thread_id*/);
          pDst16u[1] = (uint16_t*)m_ccomp[1].GetCCBufferPtr(0/*thread_id*/);
          pDst16u[2] = (uint16_t*)m_ccomp[2].GetCCBufferPtr(0/*thread_id*/);
          pDst16u[3] = (uint16_t*)m_ccomp[3].GetCCBufferPtr(0/*thread_id*/);

          status = mfxiCopy_16s_C4P4R((int16_t*)pSrc16u,srcStep,(int16_t**)pDst16u,dstStep,roi);
        }

        if(ippStsNoErr != status)
        {
          LOG1("IPP Error: mfxiCopy_8u_C4P4R() failed - ",status);
          return JPEG_ERR_INTERNAL;
        }
      }
      break;

    default:
      return JPEG_NOT_IMPLEMENTED;
    }
  }

  // Gray to Gray
  if(m_src.color == JC_GRAY && m_jpeg_color == JC_GRAY)
  {
    int     dstStep;
    uint8_t*  pDst8u;
    uint16_t* pDst16u;

    dstStep = m_ccomp[0].m_cc_step;
    convert = 1;

    if(m_src.precision <= 8)
    {
      pDst8u  = m_ccomp[0].GetCCBufferPtr(0/*thread_id*/);

      status  = mfxiCopy_8u_C1R(pSrc8u,srcStep,pDst8u,dstStep,roi);
    }
    else
    {
      pDst16u = (uint16_t*)m_ccomp[0].GetCCBufferPtr(0/*thread_id*/);

      status = mfxiCopy_16s_C1R((int16_t*)pSrc16u,srcStep,(int16_t*)pDst16u,dstStep,roi);
    }

    if(ippStsNoErr != status)
    {
      LOG1("IPP Error: mfxiCopy_8u_C1R() failed - ",status);
      return JPEG_ERR_INTERNAL;
    }
  }

  // RGB to Gray
  if(m_src.color == JC_RGB && m_jpeg_color == JC_GRAY)
  {
    int    dstStep;
    uint8_t* pDst8u;

    if(m_src.precision > 8)
    {
      return JPEG_ERR_INTERNAL;
    }

    dstStep = m_ccomp[0].m_cc_step;
    convert = 1;

    pDst8u = m_ccomp[0].GetCCBufferPtr(0/*thread_id*/);

    if(!pSrc8u)
    {
      LOG0("Error: empty pSrc8u in CJPEGEncoder::ColorConvert()");
      return JPEG_ERR_INTERNAL;
    }
    status = mfxiRGBToY_JPEG_8u_C3C1R(pSrc8u,srcStep,pDst8u,dstStep,roi);
    if(ippStsNoErr != status)
    {
      LOG1("IPP Error: mfxiRGBToY_JPEG_8u_C3C1R() failed - ",status);
      return JPEG_ERR_INTERNAL;
    }
  }

  // RGB to RGB
  if(m_src.color == JC_RGB && m_jpeg_color == JC_RGB)
  {
    int     dstStep;
    uint8_t*  pDst8u[3];
    uint16_t* pDst16u[3];

    dstStep = m_ccomp[0].m_cc_step;
    convert = 1;

    if(m_src.precision <= 8)
    {
      pDst8u[0] = m_ccomp[0].GetCCBufferPtr(0/*thread_id*/);
      pDst8u[1] = m_ccomp[1].GetCCBufferPtr(0/*thread_id*/);
      pDst8u[2] = m_ccomp[2].GetCCBufferPtr(0/*thread_id*/);

      status = mfxiCopy_8u_C3P3R(pSrc8u,srcStep,pDst8u,dstStep,roi);
    }
    else
    {
      pDst16u[0] = (uint16_t*)m_ccomp[0].GetCCBufferPtr(0/*thread_id*/);
      pDst16u[1] = (uint16_t*)m_ccomp[1].GetCCBufferPtr(0/*thread_id*/);
      pDst16u[2] = (uint16_t*)m_ccomp[2].GetCCBufferPtr(0/*thread_id*/);

      status = mfxiCopy_16s_C3P3R((int16_t*)pSrc16u,srcStep,(int16_t**)pDst16u,dstStep,roi);
    }

    if(ippStsNoErr != status)
    {
      LOG1("IPP Error: mfxiCopy_8u_C3P3R() failed - ",status);
      return JPEG_ERR_INTERNAL;
    }
  }

  // BGR to RGB
  if(m_src.color == JC_BGR && m_jpeg_color == JC_RGB)
  {
    int     dstStep;
    uint8_t*  pDst8u[3];
    uint16_t* pDst16u[3];

    dstStep = m_ccomp[0].m_cc_step;
    convert = 1;

    if(m_src.precision <= 8)
    {
      pDst8u[2] = m_ccomp[0].GetCCBufferPtr(0/*thread_id*/);
      pDst8u[1] = m_ccomp[1].GetCCBufferPtr(0/*thread_id*/);
      pDst8u[0] = m_ccomp[2].GetCCBufferPtr(0/*thread_id*/);

      status = mfxiCopy_8u_C3P3R(pSrc8u,srcStep,pDst8u,dstStep,roi);
    }
    else
    {
      pDst16u[2] = (uint16_t*)m_ccomp[0].GetCCBufferPtr(0/*thread_id*/);
      pDst16u[1] = (uint16_t*)m_ccomp[1].GetCCBufferPtr(0/*thread_id*/);
      pDst16u[0] = (uint16_t*)m_ccomp[2].GetCCBufferPtr(0/*thread_id*/);

      status = mfxiCopy_16s_C3P3R((int16_t*)pSrc16u,srcStep,(int16_t**)pDst16u,dstStep,roi);
    }

    if(ippStsNoErr != status)
    {
      LOG1("IPP Error: mfxiCopy_8u_C3P3R() failed - ",status);
      return JPEG_ERR_INTERNAL;
    }
  }

  // BGRA to RGB
  if(m_src.color == JC_BGRA && m_jpeg_color == JC_RGB)
  {
    int     dstStep;
    uint8_t*  pDst8u[3];
    convert = 1;

    dstStep = m_ccomp[0].m_cc_step;

    if(m_src.precision <= 8)
    {
      pDst8u[0] = m_ccomp[0].GetCCBufferPtr(0/*thread_id*/);
      pDst8u[1] = m_ccomp[1].GetCCBufferPtr(0/*thread_id*/);
      pDst8u[2] = m_ccomp[2].GetCCBufferPtr(0/*thread_id*/);

      for(int i=0; i<roi.height; i++)
      {
          for(int j=0; j<roi.width; j++)
          {
              pDst8u[0][j] = pSrc8u[j*4 + 2];
              pDst8u[1][j] = pSrc8u[j*4 + 1];
              pDst8u[2][j] = pSrc8u[j*4 + 0];
          }
          pDst8u[0] += dstStep;
          pDst8u[1] += dstStep;
          pDst8u[2] += dstStep;
          pSrc8u += srcStep;
      }
    }
    else
    {
        return JPEG_NOT_IMPLEMENTED;
    }
  }

  // RGB to YCbCr
  if(m_src.color == JC_RGB && m_jpeg_color == JC_YCBCR)
  {
    int    dstStep;
    uint8_t* pDst8u[3];

    dstStep = m_ccomp[0].m_cc_step;
    convert = 1;

    pDst8u[0] = m_ccomp[0].GetCCBufferPtr(0/*thread_id*/);
    pDst8u[1] = m_ccomp[1].GetCCBufferPtr(0/*thread_id*/);
    pDst8u[2] = m_ccomp[2].GetCCBufferPtr(0/*thread_id*/);

    if(JD_PIXEL == m_src.order)
    {
      if(m_src.precision > 8)
      {
        return JPEG_ERR_INTERNAL;
      }
      status = mfxiRGBToYCbCr_JPEG_8u_C3P3R(pSrc8u,srcStep,pDst8u,dstStep,roi);
    }
    else
    {
      const uint8_t* pSrcP[3];

      pSrcP[0] = m_src.p.Data8u[0]  + colMCU * m_mcuHeight * srcStep + 8 * colMCU * m_ccomp[0].m_hsampling;
      pSrcP[1] = m_src.p.Data8u[1]  + colMCU * m_mcuHeight * srcStep + 8 * colMCU * m_ccomp[0].m_hsampling;
      pSrcP[2] = m_src.p.Data8u[2]  + colMCU * m_mcuHeight * srcStep + 8 * colMCU * m_ccomp[0].m_hsampling;

      status = mfxiRGBToYCbCr_JPEG_8u_P3R(pSrcP,srcStep,pDst8u,dstStep,roi);
    }

    if(ippStsNoErr != status)
    {
      LOG1("IPP Error: mfxiRGBToYCbCr_JPEG_8u_C3P3R() failed - ",status);
      return JPEG_ERR_INTERNAL;
    }
  }

  // BGR to YCbCr
  if(m_src.color == JC_BGR && m_jpeg_color == JC_YCBCR)
  {
    int    dstStep;
    uint8_t* pDst8u[3];

    if(m_src.precision > 8)
    {
      return JPEG_ERR_INTERNAL;
    }

    dstStep = m_ccomp[0].m_cc_step;
    convert = 1;

    pDst8u[0] = m_ccomp[0].GetCCBufferPtr(0/*thread_id*/);
    pDst8u[1] = m_ccomp[1].GetCCBufferPtr(0/*thread_id*/);
    pDst8u[2] = m_ccomp[2].GetCCBufferPtr(0/*thread_id*/);

    status = mfxiBGRToYCbCr_JPEG_8u_C3P3R(pSrc8u,srcStep,pDst8u,dstStep,roi);

    if(ippStsNoErr != status)
    {
      LOG1("IPP Error: mfxiBGRToYCbCr_JPEG_8u_C3P3R() failed - ",status);
      return JPEG_ERR_INTERNAL;
    }
  }

  // RGBA to YCbCr
  if(m_src.color == JC_RGBA && m_jpeg_color == JC_YCBCR)
  {
    int    dstStep;
    uint8_t* pDst8u[3];

    if(m_src.precision > 8)
    {
      return JPEG_ERR_INTERNAL;
    }

    dstStep = m_ccomp[0].m_cc_step;
    convert = 1;

    pDst8u[0] = m_ccomp[0].GetCCBufferPtr(0/*thread_id*/);
    pDst8u[1] = m_ccomp[1].GetCCBufferPtr(0/*thread_id*/);
    pDst8u[2] = m_ccomp[2].GetCCBufferPtr(0/*thread_id*/);

    status = mfxiRGBToYCbCr_JPEG_8u_C4P3R(pSrc8u,srcStep,pDst8u,dstStep,roi);

    if(ippStsNoErr != status)
    {
      LOG1("IPP Error: mfxiRGBToYCbCr_JPEG_8u_C4P3R() failed - ",status);
      return JPEG_ERR_INTERNAL;
    }
  }

  // YCbCr to YCbCr (422 sampling)
  if(m_src.color == JC_YCBCR && m_jpeg_color == JC_YCBCR &&
     m_src.sampling == JS_422H && m_jpeg_sampling == JS_422H)
  {
    int    dstStep[3];
    uint8_t* pDst8u[3];

    if(m_src.precision > 8)
    {
      return JPEG_ERR_INTERNAL;
    }

    convert = 1;

    dstStep[0] = m_ccomp[0].m_cc_step;
    dstStep[1] = m_ccomp[1].m_cc_step;
    dstStep[2] = m_ccomp[2].m_cc_step;

    pDst8u[0] = m_ccomp[0].GetCCBufferPtr(0/*thread_id*/);
    pDst8u[1] = m_ccomp[1].GetCCBufferPtr(0/*thread_id*/);
    pDst8u[2] = m_ccomp[2].GetCCBufferPtr(0/*thread_id*/);

    status = mfxiYCbCr422_8u_C2P3R(pSrc8u,srcStep,pDst8u,dstStep,roi);

    if(ippStsNoErr != status)
    {
      LOG1("IPP Error: mfxiYCbCr422_8u_C2P3R() failed - ",status);
      return JPEG_ERR_INTERNAL;
    }
  }

  // CMYK to CMYK
  if(m_src.color == JC_CMYK && m_jpeg_color == JC_CMYK)
  {
    int    dstStep;
    uint8_t* pDst8u[4];

    if(m_src.precision > 8)
    {
      return JPEG_ERR_INTERNAL;
    }

    dstStep = m_ccomp[0].m_cc_step;
    convert = 1;

    pDst8u[0] = m_ccomp[0].GetCCBufferPtr(0/*thread_id*/);
    pDst8u[1] = m_ccomp[1].GetCCBufferPtr(0/*thread_id*/);
    pDst8u[2] = m_ccomp[2].GetCCBufferPtr(0/*thread_id*/);
    pDst8u[3] = m_ccomp[3].GetCCBufferPtr(0/*thread_id*/);

    status = mfxiCopy_8u_C4P4R(pSrc8u,srcStep,pDst8u,dstStep,roi);

    if(ippStsNoErr != status)
    {
      LOG1("IPP Error: mfxiCopy_8u_C4P4R() failed - ",status);
      return JPEG_ERR_INTERNAL;
    }
  }

  // CMYK to YCCK
  if(m_src.color == JC_CMYK && m_jpeg_color == JC_YCCK)
  {
    int    dstStep;
    uint8_t* pDst8u[4];

    if(m_src.precision > 8)
    {
      return JPEG_ERR_INTERNAL;
    }

    dstStep = m_ccomp[0].m_cc_step;
    convert = 1;

    pDst8u[0] = m_ccomp[0].GetCCBufferPtr(0/*thread_id*/);
    pDst8u[1] = m_ccomp[1].GetCCBufferPtr(0/*thread_id*/);
    pDst8u[2] = m_ccomp[2].GetCCBufferPtr(0/*thread_id*/);
    pDst8u[3] = m_ccomp[3].GetCCBufferPtr(0/*thread_id*/);

    status = mfxiCMYKToYCCK_JPEG_8u_C4P4R(pSrc8u,srcStep,pDst8u,dstStep,roi);

    if(ippStsNoErr != status)
    {
      LOG1("IPP Error: mfxiCMYKToYCCK_JPEG_8u_C4P4R() failed - ",status);
      return JPEG_ERR_INTERNAL;
    }
  }

  if(!convert)
    return JPEG_NOT_IMPLEMENTED;

  return JPEG_OK;
} // CJPEGEncoder::ColorConvert()


JERRCODE CJPEGEncoder::DownSampling(uint32_t rowMCU, uint32_t colMCU, uint32_t maxMCU/*int nMCURow, int thread_id*/)
{
  int i, j, k;
  int cc_h;
  CJPEGColorComponent* curr_comp;
  int status;

  uint8_t  val;
  uint8_t* p;
  uint8_t* p1;
  uint8_t* p2;

  for(k = 0; k < m_jpeg_ncomp; k++)
  {
    curr_comp = &m_ccomp[k];
    cc_h = curr_comp->m_cc_height;

    // expand right edge
    if(m_xPadding)
    {
      for(i = 0; i < cc_h; i++)
      {
        if(m_src.precision <= 8)
        {
          p = curr_comp->GetCCBufferPtr(0/*thread_id*/) + i*curr_comp->m_cc_step;
          val = p[(maxMCU - colMCU) * 8 * curr_comp->m_hsampling - 1];
          for(j = 0; j < m_xPadding; j++)
          {
            p[(maxMCU - colMCU) * 8 * curr_comp->m_hsampling + j] = val;
          }
        }
        else
        {
          uint16_t  v16;
          uint16_t* p16;
          p16 = (uint16_t*)(curr_comp->GetCCBufferPtr(0/*thread_id*/) + i*curr_comp->m_cc_step);
          v16 = p16[(maxMCU - colMCU) * 8 * curr_comp->m_hsampling - 1];
          for(j = 0; j < m_xPadding; j++)
          {
            p16[(maxMCU - colMCU) * 8 * curr_comp->m_hsampling + j] = v16;
          }
        }
      }
    }

    // expand bottom edge only for last MCU row
    if(rowMCU == (uint32_t)m_numyMCU - 1)
    {
      cc_h = cc_h - m_yPadding;
      p = curr_comp->GetCCBufferPtr(0/*thread_id*/) + (cc_h-1) * curr_comp->m_cc_step;
      p1 = p;

      for(i = 0; i < m_yPadding; i++)
      {
        p1 += curr_comp->m_cc_step;
        MFX_INTERNAL_CPY(p1,p,curr_comp->m_cc_step);
      }
    }

    // sampling 444
    if(curr_comp->m_h_factor == 1 && curr_comp->m_v_factor == 1)
    {
      uint8_t* pSrc = curr_comp->GetCCBufferPtr(0/*thread_id*/);
      uint8_t* pDst = curr_comp->GetSSBufferPtr(0/*thread_id*/);

      MFX_INTERNAL_CPY(pDst,pSrc,curr_comp->m_cc_bufsize);
    }

    // sampling 422
    if(curr_comp->m_h_factor == 2 && curr_comp->m_v_factor == 1)
    {
      int    srcStep;
      int    dstStep;
      uint8_t* pSrc;
      uint8_t* pDst;

      srcStep = curr_comp->m_cc_step;
      dstStep = curr_comp->m_ss_step;

      pSrc = curr_comp->GetCCBufferPtr(0/*thread_id*/);
      pDst = curr_comp->GetSSBufferPtr(0/*thread_id*/);

      if(m_src.sampling == JS_422H)
      {
        mfxSize roi;
        roi.width  = (maxMCU - colMCU) * 8 * curr_comp->m_hsampling;//curr_comp->m_ss_step;
        roi.height = curr_comp->m_ss_height;

        status = mfxiCopy_8u_C1R(pSrc,srcStep,pDst,dstStep,roi);
        if(ippStsNoErr != status)
        {
          LOG0("Error: mfxiCopy_8u_C1R() failed!");
          return JPEG_ERR_INTERNAL;
        }
      }
      else
      {
        for(i = 0; i < m_mcuHeight; i++)
        {
          int srcWidth = (maxMCU - colMCU) * 8 * curr_comp->m_hsampling;

          status = mfxiSampleDownRowH2V1_Box_JPEG_8u_C1(pSrc, srcWidth, pDst);
          if(ippStsNoErr != status)
          {
            LOG0("Error: mfxiSampleDownRowH2V1_Box_JPEG_8u_C1() failed!");
            return JPEG_ERR_INTERNAL;
          }

          pSrc += srcStep;
          pDst += dstStep;
        }
      }
    }

    // sampling 420
    if(curr_comp->m_h_factor == 2 && curr_comp->m_v_factor == 2)
    {
      int    srcStep;
      int    srcWidth;
      uint8_t* pSrc;
      uint8_t* pDst;

      srcStep = curr_comp->m_cc_step;
      srcWidth = (maxMCU - colMCU) * 8 * curr_comp->m_hsampling;

      pSrc = curr_comp->GetCCBufferPtr(0/*thread_id*/);
      pDst = curr_comp->GetSSBufferPtr(0/*thread_id*/);

      for(i = 0; i < cc_h; i += 2)
      {
        p1 = pSrc + (i+0)*srcStep;
        p2 = pSrc + (i+1)*srcStep;

        status = mfxiSampleDownRowH2V2_Box_JPEG_8u_C1(p1, p2, srcWidth, pDst);
        if(ippStsNoErr != status)
        {
          LOG0("Error: mfxiSampleUpRowH2V2_Triangle_JPEG_8u_C1() failed!");
          return JPEG_ERR_INTERNAL;
        }

        pDst += curr_comp->m_ss_step;
      }
    }
  } // for m_jpeg_ncomp

  return JPEG_OK;
} // CJPEGEncoder::DownSampling()


JERRCODE CJPEGEncoder::ProcessBuffer(uint32_t rowMCU, uint32_t colMCU, uint32_t maxMCU)//(int nMCURow, int thread_id)
{
  int                  i, j, c;
  int                  copyHeight;
  int                  yPadd   = 0;
  int                  xPadd   = 0;
  int                  srcStep;
  uint8_t*               pSrc8u  = 0;
  uint8_t*               pDst8u  = 0;
  //uint16_t*              pSrc16u = 0;
  //uint16_t*              pDst16u = 0;
  CJPEGColorComponent* curr_comp;
  int            status;
  mfxSize             roi;

  uint8_t  val;
  uint8_t* p;
  uint8_t* p1;

  for(c = m_curr_scan.first_comp; c < m_curr_scan.first_comp + m_curr_scan.ncomps; c++)
  {
    curr_comp = &m_ccomp[c];
    srcStep   = m_src.lineStep[c];

    if(m_src.precision <= 8)
    {
      pSrc8u     = m_src.p.Data8u[c]  + rowMCU * curr_comp->m_ss_height * srcStep + colMCU * m_curr_scan.mcuWidth / curr_comp->m_h_factor;//8 * curr_comp->m_hsampling;
      copyHeight = curr_comp->m_ss_height;
      if(curr_comp->m_v_factor == 1)
      {
        yPadd = m_curr_scan.yPadding;
      }
      else
      {
        yPadd = m_curr_scan.yPadding/2;
      }

      if(rowMCU == (uint32_t)m_curr_scan.numyMCU - 1)
      {
        copyHeight -= yPadd;
      }

      uint32_t srcWidth = (maxMCU - colMCU) * m_curr_scan.mcuWidth / curr_comp->m_h_factor;//8 * curr_comp->m_hsampling;

      roi.width  = srcWidth;
      roi.height = copyHeight;

      pDst8u = curr_comp->GetSSBufferPtr(0);//thread_id);

      status = mfxiCopy_8u_C1R(pSrc8u, srcStep, pDst8u, curr_comp->m_ss_step, roi);
    }
    else
    {
      return JPEG_NOT_IMPLEMENTED;
    }

    if(ippStsNoErr != status)
    {
      LOG1("IPP Error: mfxiCopy_8u_C1R() failed - ",status);
      return JPEG_ERR_INTERNAL;
    }

    // expand right edge
    if(m_curr_scan.xPadding)
    {
      for(i = 0; i < curr_comp->m_ss_height; i++)
      {
        int width;

        if(curr_comp->m_h_factor == 1)
        {
          width = (maxMCU - colMCU) * m_curr_scan.mcuWidth / curr_comp->m_h_factor;//8 * curr_comp->m_hsampling;
          xPadd = m_curr_scan.xPadding;
        }
        else
        {
          width = ((maxMCU - colMCU) * m_curr_scan.mcuWidth / curr_comp->m_h_factor);//8 * curr_comp->m_hsampling;
          xPadd = (m_curr_scan.xPadding + 1) / 2;
        }

        p     = curr_comp->GetSSBufferPtr(0/*thread_id*/) + i*curr_comp->m_ss_step;
        val   = p[width - xPadd - 1];

        for(j = 0; j < xPadd; j++)
        {
          p[width - xPadd + j] = val;
        }
      }
    }

    // expand bottom edge only for last MCU row
    if(rowMCU == (uint32_t)m_curr_scan.numyMCU - 1)
    {
      p = curr_comp->GetSSBufferPtr(0/*thread_id*/) + (copyHeight - 1) * curr_comp->m_ss_step;
      p1 = p;
      uint32_t srcWidth = (maxMCU - colMCU) * m_curr_scan.mcuWidth / curr_comp->m_h_factor;

      for(i = 0; i < yPadd; i++)
      {
        p1 += curr_comp->m_ss_step;
        MFX_INTERNAL_CPY(p1,p,srcWidth);
      }
    }
  } // for m_jpeg_comp

  return JPEG_OK;
} // CJPEGEncoder::ProcessBuffer()


JERRCODE CJPEGEncoder::TransformMCURowBL(int16_t* pMCUBuf, uint32_t colMCU, uint32_t maxMCU/*int     thread_id*/)
{
  int c;
  int vs;
  int hs;
  int curr_mcu;
  int srcStep;
  uint8_t*               src;
  uint16_t*              qtbl;
  CJPEGColorComponent* curr_comp;
  int            status;

  for(curr_mcu = 0; curr_mcu < (int)(maxMCU - colMCU); curr_mcu++)
  {
    for(c = m_curr_scan.first_comp; c < m_curr_scan.first_comp + m_curr_scan.ncomps; c++)
    {
      curr_comp = &m_ccomp[c];

      qtbl = m_qntbl[curr_comp->m_q_selector];

      srcStep = curr_comp->m_ss_step;

      for(vs = 0; vs < m_curr_scan.mcuHeight / (8 * curr_comp->m_v_factor); vs++) // curr_comp->m_vsampling
      {
        src  = curr_comp->GetSSBufferPtr(0/*thread_id*/) +
               curr_mcu * m_curr_scan.mcuWidth / curr_comp->m_h_factor +  //8*curr_comp->m_hsampling
               8 * vs * srcStep;

        for(hs = 0; hs < m_curr_scan.mcuWidth / (8 * curr_comp->m_h_factor); hs++) // curr_comp->m_hsampling
        {
          src += 8*hs;

          status = mfxiDCTQuantFwd8x8LS_JPEG_8u16s_C1R(
                     src,srcStep,pMCUBuf,qtbl);

          if(ippStsNoErr != status)
          {
            LOG0("Error: mfxiDCTQuantFwd8x8LS_JPEG_8u16s_C1R() failed!");
            return JPEG_ERR_INTERNAL;
          }

          pMCUBuf += DCTSIZE2;
        } // for m_hsampling
      } // for m_vsampling
    } // for m_jpeg_ncomp
  } // for m_numxMCU

  return JPEG_OK;
} // CJPEGEncoder::TransformMCURowBL()


JERRCODE CJPEGEncoder::TransformMCURowEX(
  int16_t* pMCUBuf,
  int     thread_id)
{
  int c;
  int vs;
  int hs;
  int curr_mcu;
  int srcStep;
  uint16_t*              src;
  float*              qtbl;
  CJPEGColorComponent* curr_comp;
  int            status;

  for(curr_mcu = 0; curr_mcu < m_numxMCU; curr_mcu++)
  {
    for(c = 0; c < m_jpeg_ncomp; c++)
    {
      curr_comp = &m_ccomp[c];

      qtbl = m_qntbl[curr_comp->m_q_selector];

      srcStep = curr_comp->m_ss_step;

      for(vs = 0; vs < curr_comp->m_vsampling; vs++)
      {
        src  = (uint16_t*)curr_comp->GetSSBufferPtr(thread_id) +
               8*curr_mcu*curr_comp->m_hsampling +
               8*vs*srcStep;

        for(hs = 0; hs < curr_comp->m_hsampling; hs++)
        {
          src += 8*hs;

          status = mfxiDCTQuantFwd8x8LS_JPEG_16u16s_C1R(
                     src,srcStep,pMCUBuf,qtbl);

          if(ippStsNoErr != status)
          {
            LOG0("Error: mfxiDCTQuantFwd8x8LS_JPEG_8u16s_C1R() failed!");
            return JPEG_ERR_INTERNAL;
          }

          pMCUBuf += DCTSIZE2;
        } // for m_hsampling
      } // for m_vsampling
    } // for m_jpeg_ncomp
  } // for m_numxMCU

  return JPEG_OK;
} // CJPEGEncoder::TransformMCURowEX()


JERRCODE CJPEGEncoder::TransformMCURowLS(int16_t* /*pMCUBuf*/, int /*nMCURow*/, int /*thread_id*/)
{
  return JPEG_OK;
} // CJPEGEncoder::TransformMCURowLS()


JERRCODE CJPEGEncoder::EncodeHuffmanMCURowBL(int16_t* pMCUBuf, uint32_t colMCU, uint32_t maxMCU)
{
  int                    c;
  int                    vs;
  int                    hs;
  int                    mcu;
  int                    dstLen;
  int                    currPos;
  uint8_t*                 dst;
  CJPEGColorComponent*   curr_comp;
  IppiEncodeHuffmanSpec* pDCTbl = 0;
  IppiEncodeHuffmanSpec* pACTbl = 0;
  JERRCODE               jerr;
  int              status;

  dst    = m_BitStreamOut.GetDataPtr();
  dstLen = m_BitStreamOut.GetDataLen();

  for(mcu = 0; mcu < (int)(maxMCU - colMCU); mcu++)
  {
    //// process restart interval, if any
    //if(m_jpeg_restart_interval)
    //{
    //  if(m_restarts_to_go == 0)
    //  {
    //    jerr = ProcessRestart(0,0,63,0,0);
    //    if(JPEG_OK != jerr)
    //      return jerr;
    //  }
    //}

    for(c = m_curr_scan.first_comp; c < m_curr_scan.first_comp + m_curr_scan.ncomps; c++)
    {
      curr_comp = &m_ccomp[c];
      pDCTbl = m_dctbl[curr_comp->m_dc_selector];
      pACTbl = m_actbl[curr_comp->m_ac_selector];

      for(vs = 0; vs < m_curr_scan.mcuHeight / (8 * curr_comp->m_v_factor); vs++) // curr_comp->m_vsampling
      {
        for(hs = 0; hs < m_curr_scan.mcuWidth / (8 * curr_comp->m_h_factor); hs++) // curr_comp->m_hsampling
        {
          jerr = m_BitStreamOut.FlushBuffer(SAFE_NBYTES);
          if(JPEG_OK != jerr)
            return jerr;

          currPos = m_BitStreamOut.GetCurrPos();

          status = mfxiEncodeHuffman8x8_JPEG_16s1u_C1(
                     pMCUBuf,dst,dstLen,&currPos,
                     &curr_comp->m_lastDC,pDCTbl,pACTbl,m_state,0);

          m_BitStreamOut.SetCurrPos(currPos);

          if(ippStsJPEGHuffTableErr == status)
          {
            return JPEG_ERR_DHT_DATA;
          }
          else if(ippStsNoErr > status)
          {
            LOG1("IPP Error: mfxiEncodeHuffman8x8_JPEG_16s1u_C1() failed - ",status);
            return JPEG_ERR_INTERNAL;
          }

          pMCUBuf += DCTSIZE2;
        } // for m_hsampling
      } // for m_vsampling
    } // for m_jpeg_ncomp

    //if(m_jpeg_restart_interval)
    //{
    //  if(m_restarts_to_go == 0)
    //  {
    //    m_restarts_to_go = m_jpeg_restart_interval;
    //  }
    //  m_restarts_to_go --;
    //}
  } // for numxMCU

  return JPEG_OK;
} // CJPEGEncoder::EncodeHuffmanMCURowBL()


JERRCODE CJPEGEncoder::EncodeHuffmanMCURowLS(int16_t* /*pMCUBuf*/)
{
  return JPEG_OK;
} // CJPEGEncoder::EncodeHuffmanMCURowLS()


JERRCODE CJPEGEncoder::GenerateHuffmanTables(
  int ncomp,
  int id[MAX_COMPS_PER_SCAN],
  int Ss,
  int Se,
  int Ah,
  int Al)
{
  int  i;
  int  j;
  int  k;
  int  n;
  int  l;
  int  c;
  int  dc_statistics[2][256];
  int  ac_statistics[2][256];
  uint8_t bits[16];
  uint8_t vals[256];
  JERRCODE  jerr;
  int status;

  mfxsZero_8u((uint8_t*)dc_statistics,sizeof(dc_statistics));
  mfxsZero_8u((uint8_t*)ac_statistics,sizeof(ac_statistics));

  mfxsZero_8u(bits,sizeof(bits));
  mfxsZero_8u(vals,sizeof(vals));

  for(n = 0; n < m_jpeg_ncomp; n++)
  {
    m_ccomp[n].m_lastDC = 0;
  }

  //m_next_restart_num = 0;
  //m_restarts_to_go   = m_jpeg_restart_interval;

  status = mfxiEncodeHuffmanStateInit_JPEG_8u(m_state);
  if(ippStsNoErr != status)
  {
    return JPEG_ERR_INTERNAL;
  }

  int16_t* block;

  if(Ss != 0 && Se != 0)
  {
    // AC scan
    for(i = 0; i < m_numyMCU; i++)
    {
      for(k = 0; k < m_ccomp[id[0]].m_vsampling; k++)
      {
        if(i*m_ccomp[id[0]].m_vsampling*8 + k*8 >= m_src.height)
          break;

        for(j = 0; j < m_numxMCU; j++)
        {
          block = m_block_buffer + (DCTSIZE2 * m_nblock * (j + (i * m_numxMCU)));

          // skip any relevant components
          for(c = 0; c < m_ccomp[id[0]].m_comp_no; c++)
          {
            block += (DCTSIZE2 * m_ccomp[c].m_hsampling *
                                 m_ccomp[c].m_vsampling);
          }

          // Skip over relevant 8x8 blocks from this component.
          block += (k * DCTSIZE2 * m_ccomp[id[0]].m_hsampling);

          for(l = 0; l < m_ccomp[id[0]].m_hsampling; l++)
          {
            if(m_jpeg_restart_interval)
            {
              //if(m_restarts_to_go == 0)
              //{
              //  jerr = ProcessRestart(ac_statistics,id,Ss,Se,Ah,Al);
              //  if(JPEG_OK != jerr)
              //  {
              //    LOG0("Error: ProcessRestart() failed!");
              //    return jerr;
              //  }
              //}
            }

            // Ignore the last column(s) of the image.
            if(((j * m_ccomp[id[0]].m_hsampling * 8) + (l * 8)) >= m_src.width)
              break;

            if(Ah == 0)
            {
              status = mfxiGetHuffmanStatistics8x8_ACFirst_JPEG_16s_C1(
                         block,&ac_statistics[m_ccomp[id[0]].m_ac_selector][0],
                         Ss,Se,Al,m_state,0);

              if(ippStsNoErr > status)
              {
                LOG0("Error: mfxiGetHuffmanStatistics8x8_ACFirst_JPEG_16s_C1() failed!");
                return JPEG_ERR_INTERNAL;
              }
            }
            else
            {
              status = mfxiGetHuffmanStatistics8x8_ACRefine_JPEG_16s_C1(
                         block,&ac_statistics[m_ccomp[id[0]].m_ac_selector][0],
                         Ss,Se,Al,m_state,0);

              if(ippStsNoErr > status)
              {
                LOG0("Error: mfxiGetHuffmanStatistics8x8_ACRefine_JPEG_16s_C1() failed!");
                return JPEG_ERR_INTERNAL;
              }
            }

            block += DCTSIZE2;
            //m_restarts_to_go --;
          } // for m_hsampling
        } // for m_numxMCU
      } // for m_vsampling
    } // for m_numyMCU

    if(Ah == 0)
    {
      status = mfxiGetHuffmanStatistics8x8_ACFirst_JPEG_16s_C1(
                 0,ac_statistics[m_ccomp[id[0]].m_ac_selector],
                 Ss,Se,Al,m_state,1);

      if(ippStsNoErr > status)
      {
        LOG0("Error: mfxiGetHuffmanStatistics8x8_ACFirst_JPEG_16s_C1() failed!");
        return JPEG_ERR_INTERNAL;
      }
    }
    else
    {
      status = mfxiGetHuffmanStatistics8x8_ACRefine_JPEG_16s_C1(
                 0,ac_statistics[m_ccomp[id[0]].m_ac_selector],
                 Ss,Se,Al,m_state,1);

      if(ippStsNoErr > status)
      {
        LOG0("Error: mfxiGetHuffmanStatistics8x8_ACRefine_JPEG_16s_C1() failed!");
        return JPEG_ERR_INTERNAL;
      }
    }

    status = mfxiEncodeHuffmanRawTableInit_JPEG_8u(
               &ac_statistics[m_ccomp[id[0]].m_ac_selector][0],
               bits,vals);

    if(ippStsNoErr > status)
    {
      LOG0("Error: mfxiEncodeHuffmanRawTableInit_JPEG_8u() failed!");
      return JPEG_ERR_INTERNAL;
    }

    jerr = m_actbl[m_ccomp[id[0]].m_ac_selector].Init(m_ccomp[id[0]].m_ac_selector,1,bits,vals);
    if(JPEG_OK != jerr)
    {
      LOG0("Error: can't init huffman table");
      return jerr;
    }

    jerr = WriteDHT(&m_actbl[m_ccomp[id[0]].m_ac_selector]);
    if(JPEG_OK != jerr)
    {
      LOG0("Error: WriteDHT() failed");
      return jerr;
    }
  }
  else
  {
    // DC scan
    if(Ah == 0)
    {
      for(i = 0; i < m_numyMCU; i++)
      {
        for(j = 0; j < m_numxMCU; j++)
        {
          if(m_jpeg_restart_interval)
          {
            //if(m_restarts_to_go == 0)
            //{
            //  jerr = ProcessRestart(dc_statistics,id,Ss,Se,Ah,Al);
            //  if(JPEG_OK != jerr)
            //  {
            //    LOG0("Error: ProcessRestart() failed!");
            //    return jerr;
            //  }
            //}
          }

          block = m_block_buffer + (DCTSIZE2 * m_nblock * (j + (i * m_numxMCU)));

          // first DC scan
          for(n = 0; n < m_jpeg_ncomp; n++)
          {
            int16_t* lastDC = &m_ccomp[n].m_lastDC;

            for(k = 0; k < m_ccomp[n].m_vsampling; k++)
            {
              for(l = 0; l < m_ccomp[n].m_hsampling; l++)
              {
                status = mfxiGetHuffmanStatistics8x8_DCFirst_JPEG_16s_C1(
                           block,dc_statistics[m_ccomp[n].m_dc_selector],
                           lastDC,Al);

                if(ippStsNoErr > status)
                {
                  LOG0("Error: mfxiGetHuffmanStatistics8x8_DCFirst_JPEG_16s_C1() failed!");
                  return JPEG_ERR_INTERNAL;
                }

                block += DCTSIZE2;
              } // for m_hsampling
            } // for m_vsampling
          } // for m_jpeg_ncomp
          //m_restarts_to_go --;
        } // for m_numxMCU
      } // for m_numyMCU

      for(n = 0; n < ncomp; n++)
      {
        status = mfxiEncodeHuffmanRawTableInit_JPEG_8u(
                   dc_statistics[m_ccomp[n].m_dc_selector],
                   bits,vals);

        if(ippStsNoErr > status)
        {
          LOG0("Error: mfxiEncodeHuffmanRawTableInit_JPEG_8u() failed!");
          return JPEG_ERR_INTERNAL;
        }

        jerr = m_dctbl[m_ccomp[n].m_dc_selector].Init(m_ccomp[n].m_dc_selector,0,bits,vals);
        if(JPEG_OK != jerr)
        {
          LOG0("Error: can't init huffman table");
          return jerr;
        }

        jerr = WriteDHT(&m_dctbl[m_ccomp[n].m_dc_selector]);
        if(JPEG_OK != jerr)
        {
          LOG0("Error: WriteDHT() failed");
          return jerr;
        }
      } // for ncomp
    } // Ah == 0
  }

  return JPEG_OK;
} // CJPEGEncoder::GenerateHuffmanTables()


JERRCODE CJPEGEncoder::GenerateHuffmanTables(void)
{
  int       i, j, c;
  int       huffStatistics[4][256];
  uint8_t     bits[16];
  uint8_t     vals[256];
  int16_t*   ptr;
  int16_t*   pMCUBuf;
  JERRCODE  jerr;
  int status;

  //m_next_restart_num = 0;
  //m_restarts_to_go   = m_jpeg_restart_interval;

  status = mfxiEncodeHuffmanStateInit_JPEG_8u(m_state);
  if(ippStsNoErr != status)
  {
    return JPEG_ERR_INTERNAL;
  }

  mfxsZero_8u((uint8_t*)huffStatistics,sizeof(huffStatistics));

  for(i = 0; i < m_numyMCU; i++)
  {
    pMCUBuf = m_block_buffer + i * m_jpeg_ncomp * m_numxMCU;

    jerr = ColorConvert(i, 0, m_numxMCU);
    if(JPEG_OK != jerr)
      return jerr;
    jerr = TransformMCURowLS(pMCUBuf, i);
    if(JPEG_OK != jerr)
      return jerr;

    // process restart interval, if any
    if(m_jpeg_restart_interval)
    {
      //if(m_restarts_to_go == 0)
      //{
      //  jerr = ProcessRestart(0,0,63,0,0);
      //  if(JPEG_OK != jerr)
      //    return jerr;
      //}
    }

    for(c = 0; c < m_jpeg_ncomp; c++)
    {
      ptr = pMCUBuf + c * m_numxMCU;
      for(j = 0; j < m_numxMCU; j++)
      {
        status = mfxiGetHuffmanStatisticsOne_JPEG_16s_C1(
                   ptr, huffStatistics[c]);

        if(ippStsNoErr > status)
        {
          LOG1("IPP Error: mfxiGetHuffmanStatisticsOne_JPEG_16s_C1() failed - ",status);
          return JPEG_ERR_INTERNAL;
        }

        ptr++;
      } // for numxMCU
    } // for m_jpeg_ncomp

    if(m_jpeg_restart_interval)
    {
      //m_restarts_to_go -= m_numxMCU;
      //if(m_restarts_to_go == 0)
      //{
      //  ;//m_rst_go = 1;
      //}
    }
  } // for numyMCU

  for(c = 0; c < m_jpeg_ncomp; c++)
  {
    mfxsZero_8u(bits,sizeof(bits));
    mfxsZero_8u(vals,sizeof(vals));

    status = mfxiEncodeHuffmanRawTableInit_JPEG_8u(huffStatistics[c],bits,vals);
    if(ippStsNoErr > status)
    {
      LOG0("Error: mfxiEncodeHuffmanRawTableInit_JPEG_8u() failed!");
      return JPEG_ERR_INTERNAL;
    }

    jerr = AttachHuffmanTable(c,DC,c);
    if(JPEG_OK != jerr)
    {
      LOG0("Error: WriteDHT() failed");
      return jerr;
    }

    jerr = InitHuffmanTable(bits, vals, m_ccomp[c].m_dc_selector, DC);
    if(JPEG_OK != jerr)
    {
      LOG0("Error: can't init huffman table");
      return jerr;
    }

    jerr = WriteDHT(&m_dctbl[c]);
    if(JPEG_OK != jerr)
    {
      LOG0("Error: WriteDHT() failed");
      return jerr;
    }
  } // for m_jpeg_ncomp

  return JPEG_OK;
} // CJPEGEncoder::GenerateHuffmanTables()


JERRCODE CJPEGEncoder::GenerateHuffmanTablesEX(void)
{
  int       i, j, c, cc, htbl_limit;
  int       vs;
  int       hs;
  int       dc_Statistics[4][256];
  int       ac_Statistics[4][256];
  uint8_t     bits[16];
  uint8_t     vals[256];
  int16_t*   pMCUBuf;
  CJPEGColorComponent*   curr_comp;
  JERRCODE  jerr;
  int status;

  //m_next_restart_num = 0;
  //m_restarts_to_go   = m_jpeg_restart_interval;

  status = mfxiEncodeHuffmanStateInit_JPEG_8u(m_state);
  if(ippStsNoErr != status)
  {
    return JPEG_ERR_INTERNAL;
  }

  mfxsZero_8u((uint8_t*)dc_Statistics,sizeof(dc_Statistics));
  mfxsZero_8u((uint8_t*)ac_Statistics,sizeof(ac_Statistics));

  for(i = 0; i < m_numyMCU; i++)
  {
    pMCUBuf = m_block_buffer + i * m_numxMCU * m_nblock * DCTSIZE2;
    if(JD_PIXEL == m_src.order)
    {
      jerr = ColorConvert(i, 0, m_numxMCU);
      if(JPEG_OK != jerr)
        return jerr;
      jerr = DownSampling(i, 0, m_numxMCU);
      if(JPEG_OK != jerr)
        return jerr;
    }
    else // m_src.order == JD_PLANE
    {
      jerr = ProcessBuffer(i, 0, m_numxMCU);
      if(JPEG_OK != jerr)
        return jerr;
    }

    if(JPEG_BASELINE == m_jpeg_mode)
    {
      jerr = TransformMCURowBL(pMCUBuf, 0, m_numxMCU);
    }
    else
    {
      if(m_jpeg_precision > 8)
        jerr = TransformMCURowEX(pMCUBuf, 0);
      else
        jerr = TransformMCURowBL(pMCUBuf, 0, m_numxMCU);
    }

    if(JPEG_OK != jerr)
      return jerr;

    for(j = 0; j < m_numxMCU; j++)
    {
      // process restart interval, if any
      if(m_jpeg_restart_interval)
      {
        //if(m_restarts_to_go == 0)
        //{
        //  jerr = ProcessRestart(0,0,0,63,0,0);
        //  if(JPEG_OK != jerr)
        //    return jerr;
        //}
      }

      for(c = 0; c < m_jpeg_ncomp; c++)
      {
        cc = c;
        if(JPEG_BASELINE == m_jpeg_mode)
        {
          if(m_jpeg_ncomp > 1 && (m_jpeg_color == JC_YCBCR || m_jpeg_color == JC_YCCK || m_jpeg_color == JC_NV12))
            cc = !!c;
          else
            cc = 0;
        }


        curr_comp = &m_ccomp[c];
        for(vs = 0; vs < curr_comp->m_vsampling; vs++)
        {
          for(hs = 0; hs < curr_comp->m_hsampling; hs++)
          {
            status = mfxiGetHuffmanStatistics8x8_JPEG_16s_C1(
                       pMCUBuf, dc_Statistics[cc], ac_Statistics[cc], &m_ccomp[c].m_lastDC);

          if(ippStsNoErr > status)
          {
            LOG1("IPP Error: mfxiGetHuffmanStatistics8x8_JPEG_16s_C1() failed - ",status);
            return JPEG_ERR_INTERNAL;
          }

          pMCUBuf += DCTSIZE2;
          }
        }
      } // for m_jpeg_ncomp

      if(m_jpeg_restart_interval)
      {
        //if(m_restarts_to_go == 0)
        //{
        //  m_restarts_to_go = m_jpeg_restart_interval;
        //}
        //m_restarts_to_go --;
      }
    } // for numxMCU
  } // for numyMCU



  for(c = 0; c < m_jpeg_ncomp; c++)
  {
    cc = c;
    if(JPEG_BASELINE == m_jpeg_mode)
    {
      if(m_jpeg_ncomp > 1 && (m_jpeg_color == JC_YCBCR || m_jpeg_color == JC_YCCK || m_jpeg_color == JC_NV12))
        cc = !!c;
      else
        cc = 0;
    }

    mfxsZero_8u(bits,sizeof(bits));
    mfxsZero_8u(vals,sizeof(vals));

    status = mfxiEncodeHuffmanRawTableInit_JPEG_8u(dc_Statistics[cc],bits,vals);
    if(ippStsNoErr > status)
    {
      LOG0("Error: mfxiEncodeHuffmanRawTableInit_JPEG_8u() failed!");
      return JPEG_ERR_INTERNAL;
    }

    jerr = AttachHuffmanTable(cc, DC, c);
    if(JPEG_OK != jerr)
      return jerr;

    jerr = InitHuffmanTable(bits, vals, m_ccomp[c].m_dc_selector, DC);
    if(JPEG_OK != jerr)
    {
      LOG0("Error: can't init huffman table");
      return jerr;
    }

    mfxsZero_8u(bits,sizeof(bits));
    mfxsZero_8u(vals,sizeof(vals));

    status = mfxiEncodeHuffmanRawTableInit_JPEG_8u(ac_Statistics[cc],bits,vals);
    if(ippStsNoErr > status)
    {
      LOG0("Error: mfxiEncodeHuffmanRawTableInit_JPEG_8u() failed!");
      return JPEG_ERR_INTERNAL;
    }

    jerr = AttachHuffmanTable(cc, AC, c);
    if(JPEG_OK != jerr)
      return jerr;

    jerr = InitHuffmanTable(bits, vals, m_ccomp[c].m_ac_selector, AC);
    if(JPEG_OK != jerr)
    {
      LOG0("Error: can't init huffman table");
      return jerr;
    }

    if(JPEG_BASELINE == m_jpeg_mode)
    {
      if(m_jpeg_ncomp > 1 && (m_jpeg_color == JC_YCBCR || m_jpeg_color == JC_YCCK || m_jpeg_color == JC_NV12))
        htbl_limit = 2;
      else
        htbl_limit = 1;

      if(c < htbl_limit)
      {
        jerr = WriteDHT(&m_dctbl[cc]);
        if(JPEG_OK != jerr)
        {
          LOG0("Error: WriteDHT() failed");
          return jerr;
        }

        jerr = WriteDHT(&m_actbl[cc]);
        if(JPEG_OK != jerr)
        {
          LOG0("Error: WriteDHT() failed");
          return jerr;
        }
      }
    }
    else
    {
      jerr = WriteDHT(&m_dctbl[c]);
      if(JPEG_OK != jerr)
      {
        LOG0("Error: WriteDHT() failed");
        return jerr;
      }

      jerr = WriteDHT(&m_actbl[c]);
      if(JPEG_OK != jerr)
      {
        LOG0("Error: WriteDHT() failed");
        return jerr;
      }
    }
  }

  return JPEG_OK;
} // CJPEGEncoder::GenerateHuffmanTablesEX()


JERRCODE CJPEGEncoder::EncodeScan(
  int ncomp,
  int id[MAX_COMPS_PER_SCAN],
  int Ss,
  int Se,
  int Ah,
  int Al)
{
  int  i;
  int  j;
  int  k;
  int  n;
  int  l;
  int  c;
  int  dstLen;
  int  currPos;
  uint8_t*    dst;
  int16_t*   block;
  JERRCODE  jerr;
  int status;

  //m_next_restart_num = 0;
  //m_restarts_to_go   = m_jpeg_restart_interval;

  dst    = m_BitStreamOut.GetDataPtr();
  dstLen = m_BitStreamOut.GetDataLen();

  for(n = 0; n < m_jpeg_ncomp; n++)
  {
    m_ccomp[n].m_lastDC = 0;
  }

  status = mfxiEncodeHuffmanStateInit_JPEG_8u(m_state);
  if(ippStsNoErr != status)
  {
    return JPEG_ERR_INTERNAL;
  }

  jerr = WriteSOS(ncomp,id,Ss,Se,Ah,Al);
  if(JPEG_OK != jerr)
  {
    LOG0("Error: WriteSOS() failed");
    return jerr;
  }

  if(Ss != 0 && Se != 0)
  {
    // AC scan
    for(i = 0; i < m_numyMCU; i++)
    {
      for(k = 0; k < m_ccomp[id[0]].m_vsampling; k++)
      {
        if(i*m_ccomp[id[0]].m_vsampling*8 + k*8 >= m_src.height)
          break;

        for(j = 0; j < m_numxMCU; j++)
        {
          block = m_block_buffer + (DCTSIZE2 * m_nblock * (j + (i * m_numxMCU)));

          // skip any relevant components
          for(c = 0; c < m_ccomp[id[0]].m_comp_no; c++)
          {
            block += (DCTSIZE2 * m_ccomp[c].m_hsampling *
                                 m_ccomp[c].m_vsampling);
          }

          // Skip over relevant 8x8 blocks from this component.
          block += (k * DCTSIZE2 * m_ccomp[id[0]].m_hsampling);

          for(l = 0; l < m_ccomp[id[0]].m_hsampling; l++)
          {
            // Ignore the last column(s) of the image.
            if(((j * m_ccomp[id[0]].m_hsampling * 8) + (l * 8)) >= m_src.width)
              break;

            if(m_jpeg_restart_interval)
            {
              //if(m_restarts_to_go == 0)
              //{
              //  jerr = ProcessRestart(id,Ss,Se,Ah,Al);
              //  if(JPEG_OK != jerr)
              //  {
              //    LOG0("Error: ProcessRestart() failed!");
              //    return jerr;
              //  }
              //}
            }

            IppiEncodeHuffmanSpec* actbl = m_actbl[m_ccomp[id[0]].m_ac_selector];

            if(Ah == 0)
            {
              jerr = m_BitStreamOut.FlushBuffer(SAFE_NBYTES);
              if(JPEG_OK != jerr)
                return jerr;

              currPos = m_BitStreamOut.GetCurrPos();

              status = mfxiEncodeHuffman8x8_ACFirst_JPEG_16s1u_C1(
                         block,dst,dstLen,&currPos,Ss,Se,
                         Al,actbl,m_state,0);

              m_BitStreamOut.SetCurrPos(currPos);

              if(ippStsNoErr > status)
              {
                LOG1("Error: mfxiEncodeHuffman8x8_ACFirst_JPEG_16s1u_C1() failed!",ippGetStatusString(status));
                return JPEG_ERR_INTERNAL;
              }
            }
            else
            {
              jerr = m_BitStreamOut.FlushBuffer(SAFE_NBYTES);
              if(JPEG_OK != jerr)
                return jerr;

              currPos = m_BitStreamOut.GetCurrPos();

              status = mfxiEncodeHuffman8x8_ACRefine_JPEG_16s1u_C1(
                         block,dst,dstLen,&currPos,Ss,Se,
                         Al,actbl,m_state,0);

              m_BitStreamOut.SetCurrPos(currPos);

              if(ippStsNoErr > status)
              {
                LOG1("Error: mfxiEncodeHuffman8x8_ACRefine_JPEG_16s1u_C1() failed!",ippGetStatusString(status));
                return JPEG_ERR_INTERNAL;
              }
            }

            block += DCTSIZE2;

            //m_restarts_to_go --;
          } // for m_hsampling
        } // for m_numxMCU
      } // for m_vsampling
    } // for m_numyMCU

    IppiEncodeHuffmanSpec* actbl = m_actbl[m_ccomp[id[0]].m_ac_selector];

    if(Ah == 0)
    {
      jerr = m_BitStreamOut.FlushBuffer(SAFE_NBYTES);
      if(JPEG_OK != jerr)
        return jerr;

      currPos = m_BitStreamOut.GetCurrPos();

      status = mfxiEncodeHuffman8x8_ACFirst_JPEG_16s1u_C1(
                 0,dst,dstLen,&currPos,Ss,Se,
                 Al,actbl,m_state,1);

      m_BitStreamOut.SetCurrPos(currPos);

      if(ippStsNoErr > status)
      {
        LOG0("Error: mfxiEncodeHuffman8x8_ACFirst_JPEG_16s1u_C1() failed!");
        return JPEG_ERR_INTERNAL;
      }
    }
    else
    {
      jerr = m_BitStreamOut.FlushBuffer(SAFE_NBYTES);
      if(JPEG_OK != jerr)
        return jerr;

      currPos = m_BitStreamOut.GetCurrPos();

      status = mfxiEncodeHuffman8x8_ACRefine_JPEG_16s1u_C1(
                 0,dst,dstLen,&currPos,Ss,Se,
                 Al,actbl,m_state,1);

      m_BitStreamOut.SetCurrPos(currPos);

      if(ippStsNoErr > status)
      {
        LOG0("Error: mfxiEncodeHuffman8x8_ACRefine_JPEG_16s1u_C1() failed!");
        return JPEG_ERR_INTERNAL;
      }
    }
  }
  else
  {
    // DC scan
    for(i = 0; i < m_numyMCU; i++)
    {
      for(j = 0; j < m_numxMCU; j++)
      {
        if(m_jpeg_restart_interval)
        {
          //if(m_restarts_to_go == 0)
          //{
          //  jerr = ProcessRestart(id,Ss,Se,Ah,Al);
          //  if(JPEG_OK != jerr)
          //  {
          //    LOG0("Error: ProcessRestart() failed!");
          //    return jerr;
          //  }
          //}
        }

        block = m_block_buffer + (DCTSIZE2 * m_nblock * (j + (i * m_numxMCU)));

        if(Ah == 0)
        {
          // first DC scan
          for(n = 0; n < m_jpeg_ncomp; n++)
          {
            int16_t* lastDC = &m_ccomp[n].m_lastDC;
            IppiEncodeHuffmanSpec* dctbl = m_dctbl[m_ccomp[n].m_dc_selector];

            for(k = 0; k < m_ccomp[n].m_vsampling; k++)
            {
              for(l = 0; l < m_ccomp[n].m_hsampling; l++)
              {
                jerr = m_BitStreamOut.FlushBuffer(SAFE_NBYTES);
                if(JPEG_OK != jerr)
                  return jerr;

                currPos = m_BitStreamOut.GetCurrPos();

                status = mfxiEncodeHuffman8x8_DCFirst_JPEG_16s1u_C1(
                           block,dst,dstLen,&currPos,
                           lastDC,Al,dctbl,m_state,0);

                m_BitStreamOut.SetCurrPos(currPos);

                if(ippStsNoErr > status)
                {
                  LOG1("Error: mfxiEncodeHuffman8x8_DCFirst_JPEG_16s1u_C1() failed!",ippGetStatusString(status));
                  return JPEG_ERR_INTERNAL;
                }

                block += DCTSIZE2;
              } // for m_hsampling
            } // for m_vsampling
          } // for m_jpeg_ncomp
        }
        else
        {
          // refine DC scan
          for(n = 0; n < m_jpeg_ncomp; n++)
          {
            for(k = 0; k < m_ccomp[n].m_vsampling; k++)
            {
              for(l = 0; l < m_ccomp[n].m_hsampling; l++)
              {
                jerr = m_BitStreamOut.FlushBuffer(SAFE_NBYTES);
                if(JPEG_OK != jerr)
                  return jerr;

                currPos = m_BitStreamOut.GetCurrPos();

                status = mfxiEncodeHuffman8x8_DCRefine_JPEG_16s1u_C1(
                           block,dst,dstLen,&currPos,
                           Al,m_state,0);

                m_BitStreamOut.SetCurrPos(currPos);

                if(ippStsNoErr > status)
                {
                  LOG0("Error: mfxiEncodeHuffman8x8_DCRefine_JPEG_16s1u_C1() failed!");
                  return JPEG_ERR_INTERNAL;
                }

                block += DCTSIZE2;
              } // for m_hsampling
            } // for m_vsampling
          } // for m_jpeg_ncomp
        }
        //m_restarts_to_go --;
      } // for m_numxMCU
    } // for m_numyMCU

    if(Ah == 0)
    {
      jerr = m_BitStreamOut.FlushBuffer(SAFE_NBYTES);
      if(JPEG_OK != jerr)
        return jerr;

      currPos = m_BitStreamOut.GetCurrPos();

      status = mfxiEncodeHuffman8x8_DCFirst_JPEG_16s1u_C1(
                 0,dst,dstLen,&currPos,0,0,0,m_state,1);

      m_BitStreamOut.SetCurrPos(currPos);

      if(ippStsNoErr > status)
      {
        LOG0("Error: mfxiEncodeHuffman8x8_DCFirst_JPEG_16s1u_C1() failed!");
        return JPEG_ERR_INTERNAL;
      }
    }
    else
    {
      jerr = m_BitStreamOut.FlushBuffer(SAFE_NBYTES);
      if(JPEG_OK != jerr)
        return jerr;

      currPos = m_BitStreamOut.GetCurrPos();

      status = mfxiEncodeHuffman8x8_DCRefine_JPEG_16s1u_C1(
                 0,dst,dstLen,&currPos,0,m_state,1);

      m_BitStreamOut.SetCurrPos(currPos);

      if(ippStsNoErr > status)
      {
        LOG0("Error: mfxiEncodeHuffman8x8_DCRefine_JPEG_16s1u_C1() failed!");
        return JPEG_ERR_INTERNAL;
      }
    }
  }

  return JPEG_OK;
} // CJPEGEncoder::EncodeScan()

JERRCODE CJPEGEncoder::EncodeScanBaseline(void)
{
  int i;
  int dstLen;
  int currPos;
  uint8_t* dst;
  JERRCODE  jerr = JPEG_OK;
  int status;

  for(i = 0; i < m_jpeg_ncomp; i++)
  {
    m_ccomp[i].m_lastDC = 0;
  }

  //m_next_restart_num = 0;
  //m_restarts_to_go   = m_jpeg_restart_interval;

  status = mfxiEncodeHuffmanStateInit_JPEG_8u(m_state);
  if(ippStsNoErr != status)
  {
    return JPEG_ERR_INTERNAL;
  }

  if(m_piecePosInScan == 0)
  {
      jerr = WriteSOS();
      if(JPEG_OK != jerr)
      {
        LOG0("Error: WriteSOS() failed");
        return jerr;
      }
  }


  {
    uint32_t  rowMCU, colMCU, maxMCU;
    int     thread_id = 0;
    int16_t* pMCUBuf   = 0;  // the pointer to Buffer for a current thread.


    pMCUBuf = m_block_buffer + thread_id * m_curr_scan.numxMCU * m_nblock * DCTSIZE2;

    // set the iterators
    rowMCU = m_mcu_encoded / m_curr_scan.numxMCU;
    colMCU = m_mcu_encoded % m_curr_scan.numxMCU;
    maxMCU = ((uint32_t)m_curr_scan.numxMCU < colMCU + m_mcu_to_encode) ? (m_curr_scan.numxMCU) : (colMCU + m_mcu_to_encode);

    //while(curr_row < m_numyMCU)
    while (rowMCU < (uint32_t)m_curr_scan.numyMCU)
    {


      if(rowMCU < (uint32_t)m_curr_scan.numyMCU)
      {
        if(m_src.color == m_jpeg_color && JD_PLANE == m_src.order)
        {
          jerr = ProcessBuffer(rowMCU, colMCU, maxMCU);
          if(JPEG_OK != jerr)
          {
              return jerr;
          }
        }
        else
        {
          jerr = ColorConvert(rowMCU, colMCU, maxMCU);
          if(JPEG_OK != jerr)
          {
              return jerr;
          }

          jerr = DownSampling(rowMCU, colMCU, maxMCU);
          if(JPEG_OK != jerr)
          {
              return jerr;
          }
        }

        jerr = TransformMCURowBL(pMCUBuf, colMCU, maxMCU);
        if(JPEG_OK != jerr)
        {
            return jerr;
        }

        jerr = EncodeHuffmanMCURowBL(pMCUBuf, colMCU, maxMCU);
        if(JPEG_OK != jerr)
        {
            return jerr;
        }
      }

      // increment interators
      m_mcu_encoded += (maxMCU - colMCU);
      m_mcu_to_encode -= (maxMCU - colMCU);
      if (0 == m_mcu_to_encode)
      {
          break;
      }
      maxMCU = ((uint32_t)m_curr_scan.numxMCU < m_mcu_to_encode) ?
                (m_curr_scan.numxMCU) :
                (m_mcu_to_encode);

      rowMCU += 1;
      colMCU = 0;

    } // for m_numyMCU
  } // OMP


  dst    = m_BitStreamOut.GetDataPtr();
  dstLen = m_BitStreamOut.GetDataLen();

  jerr = m_BitStreamOut.FlushBuffer(SAFE_NBYTES);
  if(JPEG_OK != jerr)
  {
      return jerr;
  }

  currPos = m_BitStreamOut.GetCurrPos();

  // flush IppiEncodeHuffmanState
  status = mfxiEncodeHuffman8x8_JPEG_16s1u_C1(0,dst,dstLen,&currPos,0,0,0,m_state,1);

  m_BitStreamOut.SetCurrPos(currPos);

  jerr = m_BitStreamOut.FlushBuffer();
  if(JPEG_OK != jerr)
  {
      return jerr;
  }

  if(ippStsNoErr > status)
  {
      LOG1("IPP Error: mfxiEncodeHuffman8x8_JPEG_16s1u_C1() failed - ",status);
      return JPEG_ERR_INTERNAL;
  }

  return JPEG_OK;
} // CJPEGEncoder::EncodeScanBaseline_P()


JERRCODE CJPEGEncoder::EncodeScanExtended(void)
{
  int i;
  int dstLen;
  int currPos;
  uint8_t* dst;
  JERRCODE  jerr;
  int status;

  for(i = 0; i < m_jpeg_ncomp; i++)
  {
    m_ccomp[i].m_lastDC = 0;
  }

  //m_next_restart_num = 0;
  //m_restarts_to_go   = m_jpeg_restart_interval;

  status = mfxiEncodeHuffmanStateInit_JPEG_8u(m_state);
  if(ippStsNoErr != status)
  {
    return JPEG_ERR_INTERNAL;
  }

  jerr = WriteSOS();
  if(JPEG_OK != jerr)
  {
    LOG0("Error: WriteSOS() failed");
    return jerr;
  }

  if(!m_optimal_htbl)
  {

  i = 0;

  {
    int     curr_row  = 0;
    int     thread_id = 0;
    int16_t* pMCUBuf   = 0;  // the pointer to Buffer for a current thread.

    curr_row = i;


    pMCUBuf = m_block_buffer + thread_id * m_numxMCU * m_nblock * DCTSIZE2;

    while(curr_row < m_numyMCU)
    {
      curr_row = i;
      i++;
      if(curr_row < m_numyMCU)
      {
        ColorConvert(curr_row, 0, m_numxMCU/*thread_id*/);

        DownSampling(curr_row, 0, m_numxMCU/*thread_id*/);

        TransformMCURowEX(pMCUBuf, thread_id);

        EncodeHuffmanMCURowBL(pMCUBuf, 0, m_numxMCU);
      }

      curr_row++;
    } // for m_numyMCU
  } // OMP

  }
  else
  {
    int16_t* mcurow;
    for(i = 0; i < m_numyMCU; i++)
    {
      mcurow = m_block_buffer + i * m_numxMCU * m_nblock * DCTSIZE2;
      jerr = EncodeHuffmanMCURowBL(mcurow, 0, m_numxMCU);
      if(JPEG_OK != jerr)
        return jerr;
    }
  }

  dst    = m_BitStreamOut.GetDataPtr();
  dstLen = m_BitStreamOut.GetDataLen();

  jerr = m_BitStreamOut.FlushBuffer(SAFE_NBYTES);
  if(JPEG_OK != jerr)
    return jerr;

  currPos = m_BitStreamOut.GetCurrPos();

  // flush IppiEncodeHuffmanState
  status = mfxiEncodeHuffman8x8_JPEG_16s1u_C1(
             0,dst,dstLen,&currPos,0,0,0,m_state,1);

  m_BitStreamOut.SetCurrPos(currPos);

  if(ippStsNoErr > status)
  {
    LOG1("IPP Error: mfxiEncodeHuffman8x8_JPEG_16s1u_C1() failed - ",status);
    return JPEG_ERR_INTERNAL;
  }

  return JPEG_OK;
} // CJPEGEncoder::EncodeScanExtended()


JERRCODE CJPEGEncoder::EncodeScanExtended_P(void)
{
  int i;
  int dstLen;
  int currPos;
  uint8_t* dst;
  JERRCODE  jerr;
  int status;

  for(i = 0; i < m_jpeg_ncomp; i++)
  {
    m_ccomp[i].m_lastDC = 0;
  }

  //m_next_restart_num = 0;
  //m_restarts_to_go   = m_jpeg_restart_interval;

  status = mfxiEncodeHuffmanStateInit_JPEG_8u(m_state);
  if(ippStsNoErr != status)
  {
    return JPEG_ERR_INTERNAL;
  }

  jerr = WriteSOS();
  if(JPEG_OK != jerr)
  {
    LOG0("Error: WriteSOS() failed");
    return jerr;
  }

  if(!m_optimal_htbl)
  {

  i = 0;

  {
    int     curr_row  = 0;
    int     thread_id = 0;
    int16_t* pMCUBuf   = 0;  // the pointer to Buffer for a current thread.

    curr_row = i;


    pMCUBuf = m_block_buffer + thread_id * m_numxMCU * m_nblock * DCTSIZE2;

    while(curr_row < m_numyMCU)
    {
      curr_row = i;
      i++;
      if(curr_row < m_numyMCU)
      {
        ProcessBuffer(curr_row, 0, m_numxMCU/*thread_id*/);

        TransformMCURowEX(pMCUBuf, thread_id);

        EncodeHuffmanMCURowBL(pMCUBuf, 0, m_numxMCU);
      }

      curr_row++;
    } // for m_numyMCU
  } // OMP

  }
  else
  {
    int16_t* mcurow;
    for(i = 0; i < m_numyMCU; i++)
    {
      mcurow = m_block_buffer + i * m_numxMCU * m_nblock * DCTSIZE2;
      jerr = EncodeHuffmanMCURowBL(mcurow, 0, m_numxMCU);
      if(JPEG_OK != jerr)
        return jerr;
    }
  }

  dst    = m_BitStreamOut.GetDataPtr();
  dstLen = m_BitStreamOut.GetDataLen();

  jerr = m_BitStreamOut.FlushBuffer(SAFE_NBYTES);
  if(JPEG_OK != jerr)
    return jerr;

  currPos = m_BitStreamOut.GetCurrPos();

  // flush IppiEncodeHuffmanState
  status = mfxiEncodeHuffman8x8_JPEG_16s1u_C1(
             0,dst,dstLen,&currPos,0,0,0,m_state,1);

  m_BitStreamOut.SetCurrPos(currPos);

  if(ippStsNoErr > status)
  {
    LOG1("IPP Error: mfxiEncodeHuffman8x8_JPEG_16s1u_C1() failed - ",status);
    return JPEG_ERR_INTERNAL;
  }

  return JPEG_OK;
} // CJPEGEncoder::EncodeScanExtended_P()


JERRCODE CJPEGEncoder::EncodeScanProgressive(void)
{
  return JPEG_OK;
} // CJPEGEncoder::EncodeScanProgressive()


JERRCODE CJPEGEncoder::EncodeScanProgressive_P(void)
{
  return JPEG_OK;
} // CJPEGEncoder::EncodeScanProgressive_P()


JERRCODE CJPEGEncoder::EncodeScanLossless(void)
{
  int         i;
  int         dstLen;
  int         currPos;
  uint8_t*      dst;
  int16_t*     pMCUBuf;
  int   status;
  JERRCODE    jerr;

  //m_next_restart_num = 0;
  //m_restarts_to_go   = m_jpeg_restart_interval;

  status = mfxiEncodeHuffmanStateInit_JPEG_8u(m_state);
  if(ippStsNoErr != status)
  {
    return JPEG_ERR_INTERNAL;
  }

  m_ss = m_predictor;
  m_se = 0;
  m_ah = 0;
  m_al = m_pt;

  jerr = WriteSOS();
  if(JPEG_OK != jerr)
  {
    LOG0("Error: WriteSOS() failed");
    return jerr;
  }

  if(!m_optimal_htbl)
  {
    for(i = 0; i < m_numyMCU; i++)
    {
      pMCUBuf  = m_block_buffer;
      jerr = ColorConvert(i, 0, m_numxMCU);
      if(JPEG_OK != jerr)
        return jerr;

      jerr = TransformMCURowLS(pMCUBuf, i);
      if(JPEG_OK != jerr)
        return jerr;

      jerr = EncodeHuffmanMCURowLS(pMCUBuf);
      if(JPEG_OK != jerr)
        return jerr;
    } // for m_numyMCU
  }
  else
  {
    for(i = 0; i < m_numyMCU; i++)
    {
      pMCUBuf  = m_block_buffer + i * m_jpeg_ncomp * m_numxMCU;

      jerr = EncodeHuffmanMCURowLS(pMCUBuf);
      if(JPEG_OK != jerr)
        return jerr;
    } // for m_numyMCU
  }

  jerr = m_BitStreamOut.FlushBuffer();
  if(JPEG_OK != jerr)
    return jerr;

  dst    = m_BitStreamOut.GetDataPtr();
  dstLen = m_BitStreamOut.GetDataLen();

  currPos = m_BitStreamOut.GetCurrPos();

  // flush IppiEncodeHuffmanState
  status = mfxiEncodeHuffmanOne_JPEG_16s1u_C1(
             0,dst,dstLen,&currPos,0,m_state,1);

  m_BitStreamOut.SetCurrPos(currPos);

  if(ippStsNoErr > status)
  {
    LOG1("IPP Error: mfxiEncodeHuffmanOne_JPEG_16s1u_C1() failed - ",status);
    return JPEG_ERR_INTERNAL;
  }

  return JPEG_OK;
} // CJPEGEncoder::EncodeScanLossless()


JERRCODE CJPEGEncoder::WriteHeader(void)
{
  JERRCODE jerr;

  jerr = Init();
  if(JPEG_OK != jerr)
  {
    LOG0("Error: can't init encoder");
    return jerr;
  }

  // write header if piece is first in the field
  if(0 == m_piecePosInField)
  {
      jerr = WriteSOI();
      if(JPEG_OK != jerr)
      {
        LOG0("Error: WriteSOI() failed");
        return jerr;
      }

      if(m_jpeg_mode != JPEG_LOSSLESS)
      {
        switch(m_jpeg_color)
        {
          case JC_GRAY:
          case JC_YCBCR:
          case JC_NV12:
            jerr = WriteAPP0();
            if(JPEG_OK != jerr)
            {
              LOG0("Error: WriteAPP0() failed");
              return jerr;
            }
            break;

          case JC_RGB:
          case JC_CMYK:
          case JC_YCCK:
            jerr = WriteAPP14();
            if(JPEG_OK != jerr)
            {
              LOG0("Error: WriteAPP14() failed");
              return jerr;
            }
            break;

          default:
            break;
        }
      }

      /*jerr = WriteCOM(m_jpeg_comment);
      if(JPEG_OK != jerr)
      {
        LOG0("Error: WriteCOM() failed");
        return jerr;
      }*/

      if(m_jpeg_mode != JPEG_LOSSLESS)
      {
        jerr = WriteDQT(&m_qntbl[0]);
        if(JPEG_OK != jerr)
        {
          LOG0("Error: WriteDQT() failed");
          return jerr;
        }

        if(m_jpeg_ncomp != 1 && (m_jpeg_color == JC_YCBCR || m_jpeg_color == JC_YCCK || m_jpeg_color == JC_NV12))
        {
          jerr = WriteDQT(&m_qntbl[1]);
          if(JPEG_OK != jerr)
          {
            LOG0("Error: WriteDQT() failed");
            return jerr;
          }
        }
      }

      switch(m_jpeg_mode)
      {
      case JPEG_BASELINE:    jerr = WriteSOF0(); break;
      //case JPEG_EXTENDED:    jerr = WriteSOF1(); break;
      //case JPEG_PROGRESSIVE: jerr = WriteSOF2(); break;
      //case JPEG_LOSSLESS:    jerr = WriteSOF3(); break;
      default:
        return JPEG_ERR_PARAMS;
      }

      if(JPEG_OK != jerr)
      {
        LOG0("Error: WriteSOFx() failed");
        return jerr;
      }

      switch(m_jpeg_mode)
      {
      case JPEG_BASELINE:
        if(!m_optimal_htbl)
        {
          jerr = WriteDHT(&m_dctbl[0]);
          if(JPEG_OK != jerr)
          {
            LOG0("Error: WriteDHT() failed");
            return jerr;
          }

          jerr = WriteDHT(&m_actbl[0]);
          if(JPEG_OK != jerr)
          {
            LOG0("Error: WriteDHT() failed");
            return jerr;
          }

          if(m_jpeg_ncomp != 1 && (m_jpeg_color == JC_YCBCR || m_jpeg_color == JC_YCCK || m_jpeg_color == JC_NV12))
          {
            jerr = WriteDHT(&m_dctbl[1]);
            if(JPEG_OK != jerr)
            {
              LOG0("Error: WriteDHT() failed");
              return jerr;
            }

            jerr = WriteDHT(&m_actbl[1]);
            if(JPEG_OK != jerr)
            {
              LOG0("Error: WriteDHT() failed");
              return jerr;
            }
          }
        }
        else
        {
          GenerateHuffmanTablesEX();
        }
        break;

      case JPEG_PROGRESSIVE:
        // always generated tables
        break;

      default:
        break;
      }
  }

  // write DRI if piece is first in the scan
  if(0 == m_piecePosInScan)
  {
      if(m_jpeg_restart_interval)
      {
        jerr = WriteDRI(m_jpeg_restart_interval);
        if(JPEG_OK != jerr)
        {
          LOG0("Error: WriteDRI() failed");
          return jerr;
        }
      }
  }

  jerr = m_BitStreamOut.FlushBuffer();
  if(JPEG_OK != jerr)
    return jerr;

  return JPEG_OK;
} // CJPEGEncoder::WriteHeader()


JERRCODE CJPEGEncoder::WriteData(void)
{
  JERRCODE  jerr = JPEG_OK;

  switch(m_jpeg_mode)
  {
    case JPEG_BASELINE:
      {
              jerr = EncodeScanBaseline();
      }
      break;

    case JPEG_EXTENDED:
      {
        if(JD_PLANE == m_src.order)
          jerr = EncodeScanExtended_P();
        else
          jerr = EncodeScanExtended();
      }
      break;

    case JPEG_PROGRESSIVE:
      {
        if(JD_PLANE == m_src.order)
          jerr = EncodeScanProgressive_P();
        else
          jerr = EncodeScanProgressive();
      }
      break;

    case JPEG_LOSSLESS:
      jerr = EncodeScanLossless();
      break;

    default:
      return JPEG_ERR_INTERNAL;
  }

  if(JPEG_OK != jerr)
  {
    LOG0("Error: EncodeScanX() failed");
    return jerr;
  }

  // write EOI if piece is last in the field
  if(m_piecePosInField == m_piecesCountInField - 1)
  {
      jerr = WriteEOI();
      if(JPEG_OK != jerr)
      {
        LOG0("Error: WriteEOI() failed");
        return jerr;
      }
  }

  jerr = m_BitStreamOut.FlushBuffer();
  if(JPEG_OK != jerr)
    return jerr;

  return JPEG_OK;
} // CJPEGEncoder::WriteData()


JERRCODE CJPEGEncoder::SetComment( int comment_size, char* comment)
{
  if(comment_size > 65533)
    return JPEG_ERR_PARAMS;

  if(comment != 0)
    m_jpeg_comment = comment;

  return JPEG_OK;
} // CJPEGEncoder::SetComment()


JERRCODE CJPEGEncoder::SetJFIFApp0Resolution(JRESUNITS units, int xdensity, int ydensity)
{
  if(units > JRU_DPC  || units < JRU_NONE)
    return JPEG_ERR_PARAMS;

  m_jfif_app0_units    = units;
  m_jfif_app0_xDensity = xdensity;
  m_jfif_app0_yDensity = ydensity;

  return JPEG_OK;
} // CJPEGEncoder::SetApp0Params()
#endif // MFX_ENABLE_MJPEG_VIDEO_ENCODE
