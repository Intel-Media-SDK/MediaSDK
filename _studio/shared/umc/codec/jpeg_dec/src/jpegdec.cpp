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
#if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE) && defined(MFX_ENABLE_SW_FALLBACK)

#include "ippi.h"
#include "ipps.h"
#include <memory>
#include <stdio.h>
#include <string.h>
#include "jpegbase.h"
#include "jpegdec.h"
#include <cstdlib>
#include <assert.h>

extern void ConvertFrom_YUV444_To_YV12(const uint8_t *src[3], uint32_t srcPitch, uint8_t * dst[2], uint32_t dstPitch, mfxSize size);
extern void ConvertFrom_YUV422V_To_YV12(uint8_t *src[3], uint32_t srcPitch, uint8_t * dst[2], uint32_t dstPitch, mfxSize size);
extern void ConvertFrom_YUV422H_4Y_To_NV12(const uint8_t *src[3], uint32_t srcPitch, uint8_t * dst[2], uint32_t dstPitch, mfxSize size);
extern void ConvertFrom_YUV422V_4Y_To_NV12(const uint8_t *src[3], uint32_t srcPitch, uint8_t * dst[2], uint32_t dstPitch, mfxSize size);

#define HUFF_ROW_API // enable new huffman functions, to improve performance

#define DCT_QUANT_INV8x8To1x1LS(pMCUBuf, dst, qtbl)\
{\
  int val = (((pMCUBuf[0] * qtbl[0]) >> 3) + 128);\
  dst[0] = (uint8_t)(val > 255 ? 255 : (val < 0 ? 0 : val));\
}

CJPEGDecoder::CJPEGDecoder(void)
    : m_dst()
{
  Reset();
  return;
} // ctor


CJPEGDecoder::~CJPEGDecoder(void)
{
  Clean();
  return;
} // dtor


void CJPEGDecoder::Reset(void)
{
  m_jpeg_width             = 0;
  m_jpeg_height            = 0;
  m_jpeg_ncomp             = 0;
  m_jpeg_precision         = 8;
  m_jpeg_sampling          = JS_OTHER;
  m_jpeg_color             = JC_UNKNOWN;
  m_jpeg_quality           = 100;
  
  m_jpeg_mode              = JPEG_UNKNOWN;
  m_jpeg_dct_scale         = JD_1_1;
  m_dd_factor              = 1;

  m_jpeg_comment_detected  = 0;
  m_jpeg_comment           = 0;
  m_jpeg_comment_size      = 0;

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

  m_exif_app1_detected     = 0;
  m_exif_app1_data_size    = 0;
  m_exif_app1_data         = 0;

  m_adobe_app14_detected   = 0;
  m_adobe_app14_version    = 0;
  m_adobe_app14_flags0     = 0;
  m_adobe_app14_flags1     = 0;
  m_adobe_app14_transform  = 0;

  m_precision              = 0;
  m_max_hsampling          = 0;
  m_max_vsampling          = 0;
  m_numxMCU                = 0;
  m_numyMCU                = 0;
  m_mcuWidth               = 0;
  m_mcuHeight              = 0;
  m_ccWidth                = 0;
  m_ccHeight               = 0;
  m_xPadding               = 0;
  m_yPadding               = 0;
  m_rst_go                 = 0;
  m_mcu_decoded            = 0;
  m_mcu_to_decode          = 0;
  m_restarts_to_go         = 0;
  m_next_restart_num       = 0;
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
  m_dc_scan_completed      = 0;
  m_ac_scans_completed     = 0;
  m_init_done              = 0;
  m_marker                 = JM_NONE;

  m_block_buffer           = 0;
  m_block_buffer_size      = 0;
  m_num_threads            = 0;
  m_nblock                 = 0;

  m_use_qdct               = 0;
  m_sof_find               = 0;

  return;
} // CJPEGDecoder::Reset(void)


JERRCODE CJPEGDecoder::Clean(void)
{
  int i;

  m_jpeg_comment_detected = 0;

  if(0 != m_jpeg_comment)
  {
    free(m_jpeg_comment);
    m_jpeg_comment = 0;
    m_jpeg_comment_size = 0;
  }

  m_avi1_app0_detected    = 0;
  m_avi1_app0_polarity    = 0;
  m_avi1_app0_reserved    = 0;
  m_avi1_app0_field_size  = 0;
  m_avi1_app0_field_size2 = 0;

  m_jfif_app0_detected    = 0;
  m_jfxx_app0_detected    = 0;

  m_exif_app1_detected    = 0;

  if(0 != m_exif_app1_data)
  {
    free(m_exif_app1_data);
    m_exif_app1_data = 0;
  }

  m_adobe_app14_detected = 0;

  m_curr_scan->ncomps = 0;
  m_init_done   = 0;

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

  if(0 != m_block_buffer)
  {
    free(m_block_buffer);
    m_block_buffer = 0;
  }

  m_block_buffer_size = 0;

  m_state.Destroy();

  return JPEG_OK;
} // CJPEGDecoder::Clean()

JERRCODE CJPEGDecoder::SetDestination(
  uint8_t*   pDst,
  int      dstStep,
  mfxSize dstSize,
  int      dstChannels,
  JCOLOR   dstColor,
  JSS      dstSampling,
  int      dstPrecision,
  JDD      dstDctScale)
{
  if(0 == pDst)
    return JPEG_ERR_PARAMS;

  if(0 > dstStep)
    return JPEG_ERR_PARAMS;

  if(dstChannels <= 0 || dstChannels > 4)
    return JPEG_ERR_PARAMS;

  if(dstPrecision <= 0 || dstPrecision != m_jpeg_precision)
    return JPEG_ERR_PARAMS;

  m_dst.p.Data8u[0] = pDst;
  m_dst.lineStep[0] = dstStep;
  m_dst.width       = dstSize.width;
  m_dst.height      = dstSize.height;
  m_dst.nChannels   = dstChannels;
  m_dst.color       = dstColor;
  m_dst.sampling    = dstSampling;
  m_dst.precision   = dstPrecision;

  m_jpeg_dct_scale  = dstDctScale;

  m_dst.order = JD_PIXEL;

  return JPEG_OK;
} // CJPEGDecoder::SetDestination()


 JERRCODE CJPEGDecoder::SetDestination(
  int16_t*  pDst,
  int      dstStep,
  mfxSize dstSize,
  int      dstChannels,
  JCOLOR   dstColor,
  JSS      dstSampling,
  int      dstPrecision)
{
  m_dst.p.Data16s[0] = pDst;
  m_dst.lineStep[0]  = dstStep;
  m_dst.width        = dstSize.width;
  m_dst.height       = dstSize.height;
  m_dst.nChannels    = dstChannels;
  m_dst.color        = dstColor;
  m_dst.sampling     = dstSampling;
  m_dst.precision    = dstPrecision;

  m_dst.order = JD_PIXEL;

  return JPEG_OK;
} // CJPEGDecoder::SetDestination()


 JERRCODE CJPEGDecoder::SetDestination(
  uint8_t*   pDst[4],
  int      dstStep[4],
  mfxSize dstSize,
  int      dstChannels,
  JCOLOR   dstColor,
  JSS      dstSampling,
  int      dstPrecision,
  JDD      dstDctScale)
{
  m_dst.p.Data8u[0] = pDst[0];
  m_dst.p.Data8u[1] = pDst[1];
  m_dst.p.Data8u[2] = pDst[2];
  m_dst.p.Data8u[3] = pDst[3];
  m_dst.lineStep[0] = dstStep[0];
  m_dst.lineStep[1] = dstStep[1];
  m_dst.lineStep[2] = dstStep[2];
  m_dst.lineStep[3] = dstStep[3];

  m_dst.order = JD_PLANE;

  m_dst.width       = dstSize.width;
  m_dst.height      = dstSize.height;
  m_dst.nChannels   = dstChannels;
  m_dst.color       = dstColor;
  m_dst.sampling    = dstSampling;
  m_dst.precision   = dstPrecision;

  m_jpeg_dct_scale  = dstDctScale;

  return JPEG_OK;
} // CJPEGDecoder::SetDestination()


  JERRCODE CJPEGDecoder::SetDestination(
  int16_t*  pDst[4],
  int      dstStep[4],
  mfxSize dstSize,
  int      dstChannels,
  JCOLOR   dstColor,
  JSS      dstSampling,
  int      dstPrecision)
{
  m_dst.p.Data16s[0] = pDst[0];
  m_dst.p.Data16s[1] = pDst[1];
  m_dst.p.Data16s[2] = pDst[2];
  m_dst.p.Data16s[3] = pDst[3];
  m_dst.lineStep[0]  = dstStep[0];
  m_dst.lineStep[1]  = dstStep[1];
  m_dst.lineStep[2]  = dstStep[2];
  m_dst.lineStep[3]  = dstStep[3];

  m_dst.order = JD_PLANE;

  m_dst.width        = dstSize.width;
  m_dst.height       = dstSize.height;
  m_dst.nChannels    = dstChannels;
  m_dst.color        = dstColor;
  m_dst.sampling     = dstSampling;
  m_dst.precision    = dstPrecision;

  return JPEG_OK;
} // CJPEGDecoder::SetDestination()

JERRCODE CJPEGDecoder::ProcessRestart(void)
{
  JERRCODE  jerr;
  int status;

  status = mfxiDecodeHuffmanStateInit_JPEG_8u(m_state);
  if(ippStsNoErr != status)
  {
    LOG0("Error: mfxiDecodeHuffmanStateInit_JPEG_8u() failed");
    return JPEG_ERR_INTERNAL;
  }

  for(int n = 0; n < m_jpeg_ncomp; n++)
  {
    m_ccomp[n].m_lastDC = 0;
  }

  jerr = ParseRST();
  if(JPEG_OK != jerr)
  {
    LOG0("Error: ParseRST() failed");
    return jerr;
  }

  m_rst_go = 1;
  m_restarts_to_go = m_curr_scan->jpeg_restart_interval;

  return JPEG_OK;
} // CJPEGDecoder::ProcessRestart()


JERRCODE CJPEGDecoder::ParseAPP1(void)
{
  int i;
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

  if(b0 == 0x45 && // E
     b1 == 0x78 && // x
     b2 == 0x69 && // i
     b3 == 0x66 && // f
     b4 == 0)
  {
    m_exif_app1_detected  = 1;
    m_exif_app1_data_size = len;

    jerr = m_BitStreamIn.Seek(6);
    if(JPEG_OK != jerr)
      return jerr;

    len -= 6;

    if(m_exif_app1_data != 0)
    {
      free(m_exif_app1_data);
      m_exif_app1_data = 0;
    }

    m_exif_app1_data = (uint8_t*)malloc(len);
    if(0 == m_exif_app1_data)
      return JPEG_ERR_ALLOC;

    for(i = 0; i < len; i++)
    {
      jerr = m_BitStreamIn.ReadByte(&b0);
      if(JPEG_OK != jerr)
        return jerr;

      m_exif_app1_data[i] = (uint8_t)b0;
    }
  }
  else
  {
    jerr = m_BitStreamIn.Seek(len);
    if(JPEG_OK != jerr)
      return jerr;
  }

  m_marker = JM_NONE;

  return JPEG_OK;
} // CJPEGDecoder::ParseAPP1()

JERRCODE CJPEGDecoder::ParseCOM(void)
{
  int i;
  int c;
  int len;
  JERRCODE jerr;

  TRC0("-> COM");

  jerr = m_BitStreamIn.ReadWord(&len);
  if(JPEG_OK != jerr)
    return jerr;

  len -= 2;

  TRC1("  bytes for comment - ",len);

  m_jpeg_comment_detected = 1;
  m_jpeg_comment_size     = len;

  if(m_jpeg_comment != 0)
  {
    free(m_jpeg_comment);
  }

  m_jpeg_comment = (uint8_t*)malloc(len+1);
  if(0 == m_jpeg_comment)
    return JPEG_ERR_ALLOC;

  for(i = 0; i < len; i++)
  {
    jerr = m_BitStreamIn.ReadByte(&c);
    if(JPEG_OK != jerr)
      return jerr;
    m_jpeg_comment[i] = (uint8_t)c;
  }

  m_jpeg_comment[len] = 0;

  m_marker = JM_NONE;

  return JPEG_OK;
} // CJPEGDecoder::ParseCOM()

JERRCODE CJPEGDecoder::ParseSOF2(void)
{
  int i;
  int len;
  CJPEGColorComponent* curr_comp;
  JERRCODE jerr;

  TRC0("-> SOF2");

  jerr = m_BitStreamIn.ReadWord(&len);
  if(JPEG_OK != jerr)
    return jerr;

  len -= 2;

  jerr = m_BitStreamIn.ReadByte(&m_jpeg_precision);
  if(JPEG_OK != jerr)
    return jerr;

  if(m_jpeg_precision != 8)
  {
    return JPEG_NOT_IMPLEMENTED;
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

  for(i = 0; i < m_jpeg_ncomp; i++)
  {
    curr_comp = &m_ccomp[i];

    jerr = m_BitStreamIn.ReadByte(&curr_comp->m_id);
    if(JPEG_OK != jerr)
      return jerr;

    curr_comp->m_comp_no = i;

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

    if(curr_comp->m_hsampling <= 0 || curr_comp->m_vsampling <= 0 || curr_comp->m_q_selector >= MAX_QUANT_TABLES)
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

  m_jpeg_mode = JPEG_PROGRESSIVE;

  m_marker = JM_NONE;

  return JPEG_OK;
} // CJPEGDecoder::ParseSOF2()


JERRCODE CJPEGDecoder::ParseSOF3(void)
{
  int i;
  int len;
  CJPEGColorComponent* curr_comp;
  JERRCODE jerr;

  TRC0("-> SOF3");

  jerr = m_BitStreamIn.ReadWord(&len);
  if(JPEG_OK != jerr)
    return jerr;

  len -= 2;

  jerr = m_BitStreamIn.ReadByte(&m_jpeg_precision);
  if(JPEG_OK != jerr)
    return jerr;

  if(m_jpeg_precision < 2 || m_jpeg_precision > 16)
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

  len -= 6;

  if(len != m_jpeg_ncomp * 3)
  {
    // too short frame segment
    // need to have 3 bytes per component for parameters
    return JPEG_ERR_SOF_DATA;
  }

  for(i = 0; i < m_jpeg_ncomp; i++)
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

    if(curr_comp->m_hsampling <= 0 || curr_comp->m_vsampling <= 0 || curr_comp->m_q_selector >= MAX_QUANT_TABLES)
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

  m_jpeg_mode = JPEG_LOSSLESS;

  m_marker = JM_NONE;

  return JPEG_OK;
} // CJPEGDecoder::ParseSOF3()

JERRCODE CJPEGDecoder::ParseRST(void)
{
  JERRCODE jerr;

  TRC0("-> RST");

  if(m_marker == 0xff)
  {
    jerr = m_BitStreamIn.Seek(-1);
    if(JPEG_OK != jerr)
      return jerr;

    m_marker = JM_NONE;
  }

  if(m_marker == JM_NONE)
  {
    jerr = NextMarker(&m_marker);
    if(JPEG_OK != jerr)
    {
      LOG0("Error: NextMarker() failed");
      return jerr;
    }
  }

  TRC1("restart interval ",m_next_restart_num);
  if(m_marker == ((int)JM_RST0 + m_next_restart_num))
  {
    m_marker = JM_NONE;
  }
  else
  {
    LOG1("  - got marker   - ",m_marker);
    LOG1("  - but expected - ",(int)JM_RST0 + m_next_restart_num);
    m_marker = JM_NONE;
//    return JPEG_ERR_RST_DATA;
  }

  // Update next-restart state
  m_next_restart_num = (m_next_restart_num + 1) & 7;

  return JPEG_OK;

} // CJPEGDecoder::ParseRST()

JERRCODE CJPEGDecoder::ParseData()
{
    int32_t i;
    JERRCODE jerr = Init();
    if(JPEG_OK != jerr)
    {
      return jerr;
    }

    switch(m_jpeg_mode)
    {
    case JPEG_BASELINE:
    case JPEG_EXTENDED:
      {
        jerr = DecodeScanBaseline();
        if(JPEG_OK != jerr)
          return jerr;

      }
      break;

    case JPEG_PROGRESSIVE:
      {
        jerr = DecodeScanProgressive();

        m_ac_scans_completed = 0;
        for(i = 0; i < m_jpeg_ncomp; i++)
        {
          m_ac_scans_completed += m_ccomp[i].m_ac_scan_completed;
        }

        if(JPEG_OK != jerr ||
          (m_dc_scan_completed != 0 && m_ac_scans_completed == m_jpeg_ncomp))
        {
          int16_t* pMCUBuf;
          for(i = 0; i < (int) m_numyMCU; i++)
          {
            pMCUBuf = m_block_buffer + (i* m_numxMCU * DCTSIZE2* m_nblock);

            switch (m_jpeg_dct_scale)
            {
              case JD_1_1:
              {
                  jerr = ReconstructMCURowBL8x8(pMCUBuf, 0, m_numxMCU);
              }
              break;

              case JD_1_2:
              {
                jerr = ReconstructMCURowBL8x8To4x4(pMCUBuf, 0, m_numxMCU);
              }
              break;

              case JD_1_4:
              {
                jerr = ReconstructMCURowBL8x8To2x2(pMCUBuf, 0, m_numxMCU);
              }
              break;

              case JD_1_8:
              {
                jerr = ReconstructMCURowBL8x8To1x1(pMCUBuf, 0, m_numxMCU);
              }
              break;

              default:
                break;
            }
            if(JPEG_OK != jerr)
              return jerr;
            if(JD_PIXEL == m_dst.order) // pixel by pixel order
            {
              jerr = UpSampling(i, 0, m_numxMCU);
              if(JPEG_OK != jerr)
                return jerr;

             jerr = ColorConvert(i, 0, m_numxMCU);
              if(JPEG_OK != jerr)
                return jerr;
            }
            else          // plane order
            {
              if(m_jpeg_precision == 8)
              {
                jerr = ProcessBuffer(i);
                if(JPEG_OK != jerr)
                  return jerr;
              }
              else
                return JPEG_NOT_IMPLEMENTED; //not support 16-bit PLANE image
            }
          }
        }
      }
      break;

    case JPEG_LOSSLESS:
      if(m_curr_scan->ncomps == m_jpeg_ncomp)
      {
        jerr = DecodeScanLosslessIN();
        if(JPEG_OK != jerr)
          return jerr;
      }
      else
      {
        jerr = DecodeScanLosslessNI();
        if(JPEG_OK != jerr)
          return jerr;

        if(m_ac_scans_completed == m_jpeg_ncomp)
        {
          int16_t* pMCUBuf = m_block_buffer;

          for(i = 0; i < (int) m_numyMCU; i++)
          {
            if(m_curr_scan->jpeg_restart_interval && i*m_numxMCU % m_curr_scan->jpeg_restart_interval == 0)
              m_rst_go = 1;

            jerr = ReconstructMCURowLS(pMCUBuf, i);
            if(JPEG_OK != jerr)
              return jerr;

            jerr = ColorConvert(i, 0, m_numxMCU);
            if(JPEG_OK != jerr)
              return jerr;

            m_rst_go = 0;
          } // for m_numyMCU
        }
      }
      break;

    default:
        jerr = JPEG_ERR_INTERNAL;
        break;

    } // m_jpeg_mode

    if(JPEG_OK != jerr)
      return jerr;

    return JPEG_OK;
}

JERRCODE CJPEGDecoder::FindNextImage()
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
    case JM_EOI:
      jerr = ParseEOI();
      goto Exit;

    case JM_RST0:
    case JM_RST1:
    case JM_RST2:
    case JM_RST3:
    case JM_RST4:
    case JM_RST5:
    case JM_RST6:
    case JM_RST7:
      jerr = ParseRST();
      if(JPEG_OK != jerr)
      {
        return jerr;
      }
      break;

    case JM_SOS:
        break;

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
} // JERRCODE CJPEGDecoder::FindNextImage()

JERRCODE CJPEGDecoder::ParseJPEGBitStream(JOPERATION op)
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

    case JM_APP1:
      jerr = ParseAPP1();
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

    case JM_COM:
      jerr = ParseCOM();
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
      //jerr = ParseSOF1();
      //if(JPEG_OK != jerr)
      //{
      //  return jerr;
      //}
      //break;
      return JPEG_NOT_IMPLEMENTED;

    case JM_SOF2:
      //jerr = ParseSOF2();
      //if(JPEG_OK != jerr)
      //{
      //  return jerr;
      //}
      //break;
      return JPEG_NOT_IMPLEMENTED;

    case JM_SOF3:
      //jerr = ParseSOF3();
      //if(JPEG_OK != jerr)
      //{
      //  return jerr;
      //}
      //break;
      return JPEG_NOT_IMPLEMENTED;

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

      if(JO_READ_HEADER == op)
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

      if(JO_READ_DATA == op)
      {
          jerr = ParseData();
          if(JPEG_OK != jerr)
            return jerr;

      }
      break;

      // actually, it should never happen, because in a single threaded
      // application RTSm markers go with SOS data. Multithreaded application
      // don't call this function and process NAL unit by unit separately.
/*
    case JM_RST0:
    case JM_RST1:
    case JM_RST2:
    case JM_RST3:
    case JM_RST4:
    case JM_RST5:
    case JM_RST6:
    case JM_RST7:
      jerr = ParseRST();
      if(JPEG_OK != jerr)
      {
        return jerr;
      }

      m_rst_go = 1;
      m_restarts_to_go = m_curr_scan->jpeg_restart_interval;

      if(JO_READ_DATA == op)
      {
          jerr = ParseData();
          if(JPEG_OK != jerr)
            return jerr;

      }
      break;*/

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
} // CJPEGDecoder::ParseJPEGBitStream()

JERRCODE CJPEGDecoder::Init(void)
{
  int i;
  int tr_buf_size = 0;
  CJPEGColorComponent* curr_comp;
  JERRCODE  jerr;

  if(m_init_done)
    return JPEG_OK;

  m_num_threads = get_num_threads();

  // TODO:
  //   need to add support for images with more than 4 components per frame

  if(m_dst.precision <= 8)
  {
    switch (m_jpeg_dct_scale)
    {
      case JD_1_1:
      default:
        {
          m_dd_factor = 1;
        }
        break;

      case JD_1_2:
        {
          m_dd_factor = 2;
        }
        break;

      case JD_1_4:
        {
          m_dd_factor = 4;
        }
        break;

      case JD_1_8:
        {
          m_dd_factor = 8;
        }
        break;
    }
  }

  for(i = 0; i < m_jpeg_ncomp; i++)
  {
    curr_comp = &m_ccomp[i];

    switch(m_jpeg_mode)
    {
    case JPEG_BASELINE:
    case JPEG_EXTENDED:
      {
        int ds = (m_dst.precision <= 8) ? sizeof(uint8_t) : sizeof(int16_t);

        int lnz_ds               = curr_comp->m_vsampling * curr_comp->m_hsampling;
        curr_comp->m_lnz_bufsize = lnz_ds * m_numxMCU;
        curr_comp->m_lnz_ds      = lnz_ds;

        curr_comp->m_cc_height = m_mcuHeight;
        curr_comp->m_cc_step   = m_numxMCU * m_mcuWidth * ds;

        curr_comp->m_ss_height = curr_comp->m_cc_height / curr_comp->m_v_factor;
        curr_comp->m_ss_step   = curr_comp->m_cc_step   / curr_comp->m_h_factor;

        if(curr_comp->m_h_factor == 2 && curr_comp->m_v_factor == 2)
        {
          curr_comp->m_ss_height += 2; // add border lines (top and bottom)
        }

        tr_buf_size = m_numxMCU * m_nblock * DCTSIZE2 * sizeof(int16_t) * m_num_threads;

        break;
      } // JPEG_BASELINE

      case JPEG_PROGRESSIVE:
      {
        int lnz_ds               = curr_comp->m_vsampling * curr_comp->m_hsampling;
        curr_comp->m_lnz_bufsize = lnz_ds * m_numxMCU;
        curr_comp->m_lnz_ds      = lnz_ds;

        curr_comp->m_cc_height = m_mcuHeight;
        curr_comp->m_cc_step   = m_numxMCU * m_mcuWidth;

        curr_comp->m_ss_height = curr_comp->m_cc_height / curr_comp->m_v_factor;
        curr_comp->m_ss_step   = curr_comp->m_cc_step   / curr_comp->m_h_factor;

        if(curr_comp->m_h_factor == 2 && curr_comp->m_v_factor == 2)
        {
          curr_comp->m_ss_height += 2; // add border lines (top and bottom)
        }

        tr_buf_size = m_numxMCU * m_numyMCU * m_nblock * DCTSIZE2 * sizeof(int16_t);

        break;
      } // JPEG_PROGRESSIVE

      case JPEG_LOSSLESS:
      {
        int lnz_ds               = curr_comp->m_vsampling * curr_comp->m_hsampling;
        curr_comp->m_lnz_bufsize = lnz_ds * m_numxMCU;
        curr_comp->m_lnz_ds      = lnz_ds;

        curr_comp->m_cc_height = m_mcuHeight;
        curr_comp->m_cc_step   = m_numxMCU * m_mcuWidth * sizeof(int16_t);

        curr_comp->m_ss_height = curr_comp->m_cc_height / curr_comp->m_v_factor;
        curr_comp->m_ss_step   = curr_comp->m_cc_step   / curr_comp->m_h_factor;

        if(m_curr_scan->ncomps == m_jpeg_ncomp)
          tr_buf_size = m_numxMCU * m_nblock * sizeof(int16_t);
        else
          tr_buf_size = m_numxMCU * m_numyMCU * m_nblock * sizeof(int16_t);

        curr_comp->m_curr_row = (int16_t*)malloc(curr_comp->m_cc_step * sizeof(int16_t));
        if(0 == curr_comp->m_curr_row)
          return JPEG_ERR_ALLOC;

        curr_comp->m_prev_row = (int16_t*)malloc(curr_comp->m_cc_step * sizeof(int16_t));
        if(0 == curr_comp->m_prev_row)
          return JPEG_ERR_ALLOC;

        break;
      } // JPEG_LOSSLESS

      default:
        return JPEG_ERR_PARAMS;
    } // m_jpeg_mode

    // color convert buffer
    jerr = curr_comp->CreateBufferCC(m_num_threads);
    if(JPEG_OK != jerr)
      return jerr;

    jerr = curr_comp->CreateBufferSS(m_num_threads);
    if(JPEG_OK != jerr)
      return jerr;

    jerr = curr_comp->CreateBufferLNZ(m_num_threads);
    if(JPEG_OK != jerr)
      return jerr;

  } // m_jpeg_ncomp

  if(0 == m_block_buffer)
  {
    m_block_buffer = (int16_t*)malloc(tr_buf_size);
    if(0 == m_block_buffer)
    {
      return JPEG_ERR_ALLOC;
    }

    m_block_buffer_size = tr_buf_size;

    memset((uint8_t*)m_block_buffer, 0, tr_buf_size);
  }

  m_state.Create();

  m_init_done = 1;

  return JPEG_OK;
} // CJPEGDecoder::Init()

static
uint32_t JPEG_BPP[JC_MAX] =
{
  1, // JC_UNKNOWN = 0,
  1, // JC_GRAY    = 1,
  3, // JC_RGB     = 2,
  3, // JC_BGR     = 3,
  2, // JC_YCBCR   = 4,
  4, // JC_CMYK    = 5,
  4, // JC_YCCK    = 6,
  4, // JC_BGRA    = 7,
  4, // JC_RGBA    = 8,

  1, // JC_IMC3    = 9,
  1 //JC_NV12    = 10
};

#define iRY  0x00004c8b
#define iGY  0x00009646
#define iBY  0x00001d2f
#define iRu  0x00002b33
#define iGu  0x000054cd
#define iBu  0x00008000
#define iGv  0x00006b2f
#define iBv  0x000014d1

JERRCODE CJPEGDecoder::ColorConvert(uint32_t rowMCU, uint32_t colMCU, uint32_t maxMCU)
{
  int       cc_h;
  mfxSize  roi;
  int status;
  uint32_t bpp;

  cc_h = m_curr_scan->mcuHeight * m_curr_scan->min_v_factor;
  if (rowMCU == m_curr_scan->numyMCU - 1)
  {
    cc_h -= m_curr_scan->yPadding;
  }
  roi.height = (cc_h + m_dd_factor - 1) / m_dd_factor;

  roi.width  = (maxMCU - colMCU) * m_curr_scan->mcuWidth * m_curr_scan->min_h_factor;
  if (maxMCU == m_curr_scan->numxMCU)
  {
      roi.width -= m_curr_scan->xPadding;
  }

  if(roi.height == 0)
    return JPEG_OK;


  if(JC_RGB == m_jpeg_color && (JC_BGRA == m_dst.color || m_jpeg_ncomp != m_curr_scan->ncomps))
  {
      int     srcStep;
      uint8_t*  pSrc8u[3];
      int     dstStep;
      uint8_t*  pDst8u;

      srcStep = m_ccomp[0].m_cc_step;

      pSrc8u[0] = m_ccomp[0].GetCCBufferPtr<uint8_t> (0) + colMCU * m_curr_scan->mcuWidth * m_curr_scan->min_h_factor;
      pSrc8u[1] = m_ccomp[1].GetCCBufferPtr<uint8_t> (0) + colMCU * m_curr_scan->mcuWidth * m_curr_scan->min_h_factor;
      pSrc8u[2] = m_ccomp[2].GetCCBufferPtr<uint8_t> (0) + colMCU * m_curr_scan->mcuWidth * m_curr_scan->min_h_factor;

      dstStep = m_dst.lineStep[0];
      bpp = JPEG_BPP[m_dst.color % JC_MAX];

      pDst8u   = m_dst.p.Data8u[0] + 
          rowMCU * m_curr_scan->mcuHeight * m_curr_scan->min_v_factor * dstStep / m_dd_factor + 
          colMCU * m_curr_scan->mcuWidth * m_curr_scan->min_h_factor * bpp;

      for(int n = m_curr_scan->first_comp; n < m_curr_scan->first_comp + m_curr_scan->ncomps; n++)
      {
          for(int i=0; i<roi.height; i++)
              for(int j=0; j<roi.width; j++)
              {
                  pDst8u[i*dstStep + j*4 + 2 - n] = pSrc8u[n][i*srcStep + j];
                  pDst8u[i*dstStep + j*4 + 3] = 0xFF;
              }
      }
  }
  else if (JC_YCBCR == m_jpeg_color && JC_NV12 == m_dst.color)
  {      
      int     srcStep[3];
      uint8_t*  pSrc8u[3];
      int     dstStep[2];
      uint8_t*  pDst8u[2];

      srcStep[0] = m_ccomp[0].m_cc_step;
      srcStep[1] = m_ccomp[1].m_cc_step;
      srcStep[2] = m_ccomp[2].m_cc_step;

      pSrc8u[0] = m_ccomp[0].GetCCBufferPtr<uint8_t> (0) + colMCU * m_curr_scan->mcuWidth * m_curr_scan->min_h_factor;
      pSrc8u[1] = m_ccomp[1].GetCCBufferPtr<uint8_t> (0) + colMCU * m_curr_scan->mcuWidth * m_curr_scan->min_h_factor / 2;
      pSrc8u[2] = m_ccomp[2].GetCCBufferPtr<uint8_t> (0) + colMCU * m_curr_scan->mcuWidth * m_curr_scan->min_h_factor / 2;

      dstStep[0] = m_dst.lineStep[0];
      dstStep[1] = m_dst.lineStep[1];

      pDst8u[0]   = m_dst.p.Data8u[0] + 
          rowMCU * m_curr_scan->mcuHeight * m_curr_scan->min_v_factor * dstStep[0] / m_dd_factor + 
          colMCU * m_curr_scan->mcuWidth * m_curr_scan->min_h_factor;
      pDst8u[1]   = m_dst.p.Data8u[1] + 
          rowMCU * m_curr_scan->mcuHeight * m_curr_scan->min_v_factor * dstStep[1] / (2 * m_dd_factor) + 
          colMCU * m_curr_scan->mcuWidth * m_curr_scan->min_h_factor;

      for(int n = m_curr_scan->first_comp; n < m_curr_scan->first_comp + m_curr_scan->ncomps; n++)
      {
          if(n == 0)
          {
              for(int i=0; i<roi.height; i++)
                  for(int j=0; j<roi.width; j++)
                  {
                      pDst8u[0][i*dstStep[0] + j] = pSrc8u[0][i*srcStep[0] + j];
                  }
          }
          else
          {
              for(int i=0; i < roi.height >> 1; i++)
                  for(int j=0; j < roi.width  >> 1; j++)
                  {
                      pDst8u[1][i*dstStep[1] + j * 2 + n - 1] = pSrc8u[n][i*srcStep[n] + j];
                  }
          }
      }
  }
  else if(JC_YCBCR == m_jpeg_color && JC_YCBCR == m_dst.color && JS_444 == m_dst.sampling && m_jpeg_ncomp != m_curr_scan->ncomps)
  {
      int     srcStep[3];
      uint8_t*  pSrc8u[3];
      int     dstStep[3];
      uint8_t*  pDst8u[3];

      srcStep[0] = m_ccomp[0].m_cc_step;
      srcStep[1] = m_ccomp[0].m_cc_step;
      srcStep[2] = m_ccomp[0].m_cc_step;

      pSrc8u[0] = m_ccomp[0].GetCCBufferPtr<uint8_t> (0) + colMCU * m_curr_scan->mcuWidth * m_curr_scan->min_h_factor;
      pSrc8u[1] = m_ccomp[1].GetCCBufferPtr<uint8_t> (0) + colMCU * m_curr_scan->mcuWidth * m_curr_scan->min_h_factor;
      pSrc8u[2] = m_ccomp[2].GetCCBufferPtr<uint8_t> (0) + colMCU * m_curr_scan->mcuWidth * m_curr_scan->min_h_factor;

      dstStep[0] = m_dst.lineStep[0];
      dstStep[1] = m_dst.lineStep[1];
      dstStep[2] = m_dst.lineStep[2];

      pDst8u[0] = m_dst.p.Data8u[0] + 
          rowMCU * m_curr_scan->mcuHeight * m_curr_scan->min_v_factor * dstStep[0] / m_dd_factor + 
          colMCU * m_curr_scan->mcuWidth * m_curr_scan->min_h_factor;
      pDst8u[1] = m_dst.p.Data8u[1] + 
          rowMCU * m_curr_scan->mcuHeight * m_curr_scan->min_v_factor * dstStep[1] / m_dd_factor + 
          colMCU * m_curr_scan->mcuWidth * m_curr_scan->min_h_factor;
      pDst8u[2] = m_dst.p.Data8u[2] + 
          rowMCU * m_curr_scan->mcuHeight * m_curr_scan->min_v_factor * dstStep[2] / m_dd_factor + 
          colMCU * m_curr_scan->mcuWidth * m_curr_scan->min_h_factor;

      for(int n = m_curr_scan->first_comp; n < m_curr_scan->first_comp + m_curr_scan->ncomps; n++)
      {
          for(int i=0; i<roi.height; i++)
              for(int j=0; j<roi.width; j++)
              {
                  pDst8u[n][i*dstStep[n] + j] = pSrc8u[n][i*srcStep[n] + j];
              }
      }
  }
  else if(JC_GRAY == m_jpeg_color && m_dst.color == JC_BGRA)
  {
      int    srcStep;
      uint8_t* pSrc8u;
      int    dstStep;
      uint8_t* pDst8u;

      srcStep = m_ccomp[0].m_cc_step;

      pSrc8u = m_ccomp[0].GetCCBufferPtr<uint8_t> (0) + colMCU * m_curr_scan->mcuWidth;

      dstStep = m_dst.lineStep[0];
      bpp = JPEG_BPP[m_dst.color % JC_MAX];
    
      pDst8u   = m_dst.p.Data8u[0] + 
          rowMCU * m_curr_scan->mcuHeight * m_curr_scan->min_v_factor * dstStep / m_dd_factor + 
          colMCU * m_curr_scan->mcuWidth * m_curr_scan->min_h_factor * bpp;

      for(int i=0; i<roi.height; i++)
          for(int j=0; j<roi.width; j++)
          {
              pDst8u[i*dstStep + j*4 + 0] = pSrc8u[i*srcStep + j];
              pDst8u[i*dstStep + j*4 + 1] = pSrc8u[i*srcStep + j];
              pDst8u[i*dstStep + j*4 + 2] = pSrc8u[i*srcStep + j];
              pDst8u[i*dstStep + j*4 + 3] = 0xFF;
          }
  }
  else if(JC_GRAY == m_jpeg_color && m_dst.color == JC_NV12)
  {
      int    srcStep;
      uint8_t* pSrc8u; 
      int     dstStep[2];
      uint8_t*  pDst8u[2];

      srcStep = m_ccomp[0].m_cc_step;
      pSrc8u = m_ccomp[0].GetCCBufferPtr<uint8_t> (colMCU);

      dstStep[0] = m_dst.lineStep[0];
      dstStep[1] = m_dst.lineStep[1];

      pDst8u[0]   = m_dst.p.Data8u[0] + 
          rowMCU * m_curr_scan->mcuHeight * m_curr_scan->min_v_factor * dstStep[0] / m_dd_factor + 
          colMCU * m_curr_scan->mcuWidth * m_curr_scan->min_h_factor;
      pDst8u[1]   = m_dst.p.Data8u[1] + 
          rowMCU * m_curr_scan->mcuHeight * m_curr_scan->min_v_factor * dstStep[1] / (2 * m_dd_factor) + 
          colMCU * m_curr_scan->mcuWidth * m_curr_scan->min_h_factor;

      for(int i=0; i<roi.height >> 1; i++)
          for(int j=0; j<roi.width; j++)
          {
              pDst8u[0][ (i<<1)   *dstStep[0] + j] = pSrc8u[ (i<<1)   *srcStep + j];
              pDst8u[0][((i<<1)+1)*dstStep[0] + j] = pSrc8u[((i<<1)+1)*srcStep + j];

              pDst8u[1][i*dstStep[1] + j] = 0x80;
          }
  }
  else if(m_jpeg_ncomp == m_curr_scan->ncomps)
  {
      if (JC_RGB == m_jpeg_color && JC_NV12 == m_dst.color)
      {
          int     srcStep;
          uint8_t*  pSrc8u[3];
          int     dstStep[2];
          uint8_t*  pDst8u[2];
          int     r0,r1,r2,r3, g0,g1,g2,g3, b0,b1,b2,b3;

          srcStep = m_ccomp[0].m_cc_step;

          pSrc8u[0] = m_ccomp[0].GetCCBufferPtr<uint8_t> (0) + colMCU * m_curr_scan->mcuWidth;
          pSrc8u[1] = m_ccomp[1].GetCCBufferPtr<uint8_t> (0) + colMCU * m_curr_scan->mcuWidth;
          pSrc8u[2] = m_ccomp[2].GetCCBufferPtr<uint8_t> (0) + colMCU * m_curr_scan->mcuWidth;

          dstStep[0] = m_dst.lineStep[0];
          dstStep[1] = m_dst.lineStep[1];

          pDst8u[0]   = m_dst.p.Data8u[0] + 
              rowMCU * m_curr_scan->mcuHeight * m_curr_scan->min_v_factor * dstStep[0] / m_dd_factor + 
              colMCU * m_curr_scan->mcuWidth * m_curr_scan->min_h_factor;
          pDst8u[1]   = m_dst.p.Data8u[1] + 
              rowMCU * m_curr_scan->mcuHeight * m_curr_scan->min_v_factor * dstStep[1] / (2 * m_dd_factor) + 
              colMCU * m_curr_scan->mcuWidth * m_curr_scan->min_h_factor;

          for(int i=0; i<roi.height >> 1; i++)
              for(int j=0; j<roi.width >> 1; j++)
          {
              r0 = pSrc8u[0][ (i<<1)   *srcStep + (j<<1)  ];
              g0 = pSrc8u[1][ (i<<1)   *srcStep + (j<<1)  ];
              b0 = pSrc8u[2][ (i<<1)   *srcStep + (j<<1)  ];              
              r1 = pSrc8u[0][ (i<<1)   *srcStep + (j<<1)+1];
              g1 = pSrc8u[1][ (i<<1)   *srcStep + (j<<1)+1];
              b1 = pSrc8u[2][ (i<<1)   *srcStep + (j<<1)+1];
              r2 = pSrc8u[0][((i<<1)+1)*srcStep + (j<<1)  ];
              g2 = pSrc8u[1][((i<<1)+1)*srcStep + (j<<1)  ];
              b2 = pSrc8u[2][((i<<1)+1)*srcStep + (j<<1)  ];
              r3 = pSrc8u[0][((i<<1)+1)*srcStep + (j<<1)+1];
              g3 = pSrc8u[1][((i<<1)+1)*srcStep + (j<<1)+1];
              b3 = pSrc8u[2][((i<<1)+1)*srcStep + (j<<1)+1];

              pDst8u[0][ (i<<1)   *dstStep[0] + (j<<1)  ] = (uint8_t)(( iRY*r0 + iGY*g0 + iBY*b0 + 0x008000) >> 16);
              pDst8u[0][ (i<<1)   *dstStep[0] + (j<<1)+1] = (uint8_t)(( iRY*r1 + iGY*g1 + iBY*b1 + 0x008000) >> 16);
              pDst8u[0][((i<<1)+1)*dstStep[0] + (j<<1)  ] = (uint8_t)(( iRY*r2 + iGY*g2 + iBY*b2 + 0x008000) >> 16);
              pDst8u[0][((i<<1)+1)*dstStep[0] + (j<<1)+1] = (uint8_t)(( iRY*r3 + iGY*g3 + iBY*b3 + 0x008000) >> 16);

              r0 = r0+r1+r2+r3;
              g0 = g0+g1+g2+g3;
              b0 = b0+b1+b2+b3;

              pDst8u[1][i*dstStep[1] + (j<<1)  ] = (uint8_t)((-iRu*r0 - iGu*g0 + iBu*b0 + 0x2000000) >> 18);
              pDst8u[1][i*dstStep[1] + (j<<1)+1] = (uint8_t)(( iBu*r0 - iGv*g0 - iBv*b0 + 0x2000000) >> 18);
          }
      }
      else if (JC_YCBCR == m_jpeg_color && JC_BGRA == m_dst.color)
      {
          int     srcStep;
          const uint8_t*  pSrc8u[3];
          int     dstStep;
          uint8_t*  pDst8u;

          srcStep = m_ccomp[0].m_cc_step;

          pSrc8u[0] = m_ccomp[0].GetCCBufferPtr<uint8_t> (0) + colMCU * m_curr_scan->mcuWidth;
          pSrc8u[1] = m_ccomp[1].GetCCBufferPtr<uint8_t> (0) + colMCU * m_curr_scan->mcuWidth;
          pSrc8u[2] = m_ccomp[2].GetCCBufferPtr<uint8_t> (0) + colMCU * m_curr_scan->mcuWidth;

          dstStep = m_dst.lineStep[0];
          bpp = JPEG_BPP[m_dst.color % JC_MAX];

          pDst8u   = m_dst.p.Data8u[0] + rowMCU * m_curr_scan->mcuHeight * m_curr_scan->min_v_factor * dstStep / m_dd_factor + 
              colMCU * m_curr_scan->mcuWidth * m_curr_scan->min_h_factor * bpp;

          status = mfxiYCbCrToBGR_JPEG_8u_P3C4R(pSrc8u, srcStep, pDst8u, dstStep, roi, 0xFF);

          if(ippStsNoErr != status)
          {
              LOG1("IPP Error: mfxiYCbCrToBGR_JPEG_8u_P3C3R() failed - ",status);
              return JPEG_ERR_INTERNAL;
          }
      }
  }

  return JPEG_OK;


} // JERRCODE CJPEGDecoder::ColorConvert(uint32_t rowCMU, uint32_t colMCU, uint32_t maxMCU)


JERRCODE CJPEGDecoder::UpSampling(uint32_t rowMCU, uint32_t colMCU, uint32_t maxMCU)
{
  int i, j, k, n, c;
  int need_upsampling;
  CJPEGColorComponent* curr_comp;
  int status;

  // if image format is YCbCr and destination format is NV12 need special upsampling (see below)
  if(JC_YCBCR != m_jpeg_color || JC_NV12 != m_dst.color)
  {
      for(k = m_curr_scan->first_comp; k < m_curr_scan->first_comp + m_curr_scan->ncomps; k++)
      {
        curr_comp       = &m_ccomp[k];
        need_upsampling = curr_comp->m_need_upsampling;

        // sampling 444
        // nothing to do for 444

        // sampling 422H
        if(curr_comp->m_h_factor == 2 && curr_comp->m_v_factor == 1 && need_upsampling)
        {
          int    srcStep;
          int    dstStep;
          uint8_t* pSrc;
          uint8_t* pDst;
          uint32_t srcWidth, intervalSize, tileSize, pixelToProcess;

          need_upsampling = 0;

          srcWidth = (maxMCU - colMCU) * 8 * curr_comp->m_scan_hsampling;
          srcStep = curr_comp->m_ss_step;
          dstStep = curr_comp->m_cc_step;

          // set the pointer to source buffer
          pSrc = curr_comp->GetSSBufferPtr<uint8_t> (0) + 8 * colMCU * curr_comp->m_scan_hsampling;
          // set the pointer to destination buffer
          pDst = curr_comp->GetCCBufferPtr<uint8_t> (0) + 8 * colMCU * curr_comp->m_h_factor;

          intervalSize = m_curr_scan->jpeg_restart_interval ? 
                         m_curr_scan->jpeg_restart_interval * 8 * curr_comp->m_scan_hsampling : 
                         srcWidth / m_dd_factor;

          tileSize =  (m_curr_scan->jpeg_restart_interval && !colMCU) ? 
                      (m_curr_scan->jpeg_restart_interval - (m_curr_scan->numxMCU * rowMCU) % m_curr_scan->jpeg_restart_interval)* 8 * curr_comp->m_scan_hsampling :
                      intervalSize;

          for(i = 0; i < m_mcuHeight / m_dd_factor; i++)
          {
              j = 0;
              pixelToProcess = std::min(tileSize, srcWidth / m_dd_factor);
              while(j < (int) srcWidth / m_dd_factor)
              {
                  status = mfxiSampleUpRowH2V1_Triangle_JPEG_8u_C1(pSrc + j, pixelToProcess , pDst + j * 2);
                  if(ippStsNoErr != status)
                  {
                    LOG0("Error: mfxiSampleUpRowH2V1_Triangle_JPEG_8u_C1() failed!");
                    return JPEG_ERR_INTERNAL;
                  }
                  j += pixelToProcess;
                  pixelToProcess = std::min(intervalSize, srcWidth / m_dd_factor - j);
              }

              pSrc += srcStep;
              pDst += dstStep;
          }
        }

        // sampling 422V
        if(curr_comp->m_h_factor == 1 && curr_comp->m_v_factor == 2 && need_upsampling)
        {
          int    srcStep;
          uint8_t* pSrc;
          uint8_t* pDst;
          uint32_t srcWidth;

          need_upsampling = 0;

          srcWidth = (maxMCU - colMCU) * 8 * curr_comp->m_hsampling;
          srcStep = curr_comp->m_ss_step;

          // set the pointer to source buffer
          pSrc = curr_comp->GetSSBufferPtr<uint8_t> (0) + 8 * colMCU * curr_comp->m_scan_hsampling;
          // set the pointer to destination buffer
          pDst = curr_comp->GetCCBufferPtr<uint8_t> (0) + 8 * colMCU * curr_comp->m_h_factor;

          for(i = 0; i < (m_mcuHeight >> 1) - 1; i++)
          {
            status = mfxsCopy_8u(pSrc,pDst,srcWidth);
            if(ippStsNoErr != status)
            {
              LOG0("Error: mfxs_Copy_8u() failed!");
              return JPEG_ERR_INTERNAL;
            }
            pDst += srcStep; // dstStep is the same as srcStep

            for(n = 0; n < (int)srcWidth; n++)
                pDst[n] = (pSrc[n] + pSrc[srcStep + n]) >> 1;

            pDst += srcStep; // dstStep is the same as srcStep
            pSrc += srcStep;
          }

          status = mfxsCopy_8u(pSrc,pDst,srcWidth);
          pDst += srcStep;

          status = mfxsCopy_8u(pSrc,pDst,srcWidth);
        }

        // sampling 420
        if(curr_comp->m_h_factor == 2 && curr_comp->m_v_factor == 2 && need_upsampling)
        {
          int    ddShift;
          int    srcStep;
          int    dstStep;
          uint8_t* pSrc;
          uint8_t* pDst;
          uint32_t srcWidth, intervalSize, tileSize, pixelToProcess;

          need_upsampling = 0;

          srcWidth = (maxMCU - colMCU) * 8 * curr_comp->m_scan_hsampling;
          srcStep = curr_comp->m_ss_step;
          dstStep = curr_comp->m_cc_step;

          // set the pointer to source buffer
          pSrc = curr_comp->GetSSBufferPtr<uint8_t> (0) + 8 * colMCU * curr_comp->m_scan_hsampling;
          // set the pointer to destination buffer
          pDst = curr_comp->GetCCBufferPtr<uint8_t> (0) + 8 * colMCU * curr_comp->m_h_factor;

          intervalSize = m_curr_scan->jpeg_restart_interval ? 
                         m_curr_scan->jpeg_restart_interval * 8 * curr_comp->m_scan_hsampling : 
                         srcWidth / m_dd_factor;

          tileSize =  (m_curr_scan->jpeg_restart_interval && !colMCU) ? 
                      (m_curr_scan->jpeg_restart_interval - (m_curr_scan->numxMCU * rowMCU) % m_curr_scan->jpeg_restart_interval)* 8 * curr_comp->m_scan_hsampling :
                      intervalSize;

          // filling the zero row
          j = 0;
          pixelToProcess = std::min(tileSize, srcWidth / m_dd_factor);
          while(j < (int) srcWidth / m_dd_factor)
          {
              status = mfxiSampleUpRowH2V2_Triangle_JPEG_8u_C1(pSrc + j, pSrc + j, pixelToProcess, pDst + j * 2);    
              if(ippStsNoErr != status)
              {
                LOG0("Error: mfxiSampleUpRowH2V2_Triangle_JPEG_8u_C1() failed!");
                return JPEG_ERR_INTERNAL;
              }
              j += pixelToProcess;
              pixelToProcess = std::min(intervalSize, srcWidth / m_dd_factor - j);
          }
          pDst += dstStep;

          ddShift = (m_dd_factor == 1) ? 1 : (m_dd_factor == 2) ? 2 : (m_dd_factor == 4) ? 3 : 4;
          for(i = 0; i < (m_mcuHeight >> ddShift) - 1; i++)
          {
              // filling odd rows
              j = 0;
              pixelToProcess = std::min(tileSize, srcWidth / m_dd_factor);
              while(j < (int) srcWidth / m_dd_factor)
              {
                  status = mfxiSampleUpRowH2V2_Triangle_JPEG_8u_C1(pSrc + j, pSrc + srcStep + j, pixelToProcess, pDst + j * 2);    
                  if(ippStsNoErr != status)
                  {
                    LOG0("Error: mfxiSampleUpRowH2V2_Triangle_JPEG_8u_C1() failed!");
                    return JPEG_ERR_INTERNAL;
                  }
                  j += pixelToProcess;
                  pixelToProcess = std::min(intervalSize, srcWidth / m_dd_factor - j);
              }
              pDst += dstStep;

              // filling even rows
              j = 0;
              pixelToProcess = std::min(tileSize, srcWidth / m_dd_factor);
              while(j < (int) srcWidth / m_dd_factor)
              {
                  status = mfxiSampleUpRowH2V2_Triangle_JPEG_8u_C1(pSrc + srcStep + j, pSrc + j, pixelToProcess, pDst + j * 2);    
                  if(ippStsNoErr != status)
                  {
                    LOG0("Error: mfxiSampleUpRowH2V2_Triangle_JPEG_8u_C1() failed!");
                    return JPEG_ERR_INTERNAL;
                  }
                  j += pixelToProcess;
                  pixelToProcess = std::min(intervalSize, srcWidth / m_dd_factor - j);
              }
              pDst += dstStep;
              pSrc += srcStep;
          }

          // filling the last row
          j = 0;
          pixelToProcess = std::min(tileSize, srcWidth / m_dd_factor);
          while(j < (int) srcWidth / m_dd_factor)
          {
              if (pDst > curr_comp->GetCCBufferPtr<uint8_t>(0) + (curr_comp->m_cc_bufsize - 1) * sizeof(uint8_t))
              {
                 LOG0("Error: CCBuferPtr out of bound!");
                 return JPEG_ERR_BUFF;
              }
              status = mfxiSampleUpRowH2V2_Triangle_JPEG_8u_C1(pSrc + j, pSrc + j, pixelToProcess, pDst + j * 2);    
              if(ippStsNoErr != status)
              {
                LOG0("Error: mfxiSampleUpRowH2V2_Triangle_JPEG_8u_C1() failed!");
                return JPEG_ERR_INTERNAL;
              }
              j += pixelToProcess;
              pixelToProcess = std::min(intervalSize, srcWidth / m_dd_factor - j);
          }
        } // 420

        // sampling 411
        if(curr_comp->m_h_factor == 4 && curr_comp->m_v_factor == 1 && need_upsampling)
        {
          int    srcStep;
          int    dstStep;
          uint8_t* pSrc;
          uint8_t* pDst;
          uint32_t srcWidth, intervalSize, tileSize, pixelToProcess;

          need_upsampling = 0;

          srcWidth = (maxMCU - colMCU) * 8 * curr_comp->m_scan_hsampling;
          srcStep = curr_comp->m_ss_step;
          dstStep = curr_comp->m_cc_step;

          // set the pointer to source buffer
          pSrc = curr_comp->GetSSBufferPtr<uint8_t> (0) + 8 * colMCU * curr_comp->m_scan_hsampling;
          // set the pointer to temporary buffer
          std::unique_ptr<uint8_t[]> pTmp( new uint8_t[2 * srcWidth / m_dd_factor] );
          // set the pointer to destination buffer
          pDst = curr_comp->GetCCBufferPtr<uint8_t> (0) + 8 * colMCU * curr_comp->m_h_factor;
         
          intervalSize = m_curr_scan->jpeg_restart_interval ? 
                         m_curr_scan->jpeg_restart_interval * 8 * curr_comp->m_scan_hsampling : 
                         srcWidth / m_dd_factor;

          tileSize =  (m_curr_scan->jpeg_restart_interval && !colMCU) ? 
                      (m_curr_scan->jpeg_restart_interval - (m_curr_scan->numxMCU * rowMCU) % m_curr_scan->jpeg_restart_interval)* 8 * curr_comp->m_scan_hsampling :
                      intervalSize;

          for(i = 0; i < m_mcuHeight / m_dd_factor; i++)
          {
              j = 0;
              pixelToProcess = std::min(tileSize, srcWidth / m_dd_factor);
              while(j < (int) srcWidth / m_dd_factor)
              {
                  status = mfxiSampleUpRowH2V1_Triangle_JPEG_8u_C1(pSrc + j, pixelToProcess, pTmp.get() + j * 2);
                  if(ippStsNoErr != status)
                  {
                    LOG0("Error: mfxiSampleUpRowH2V1_Triangle_JPEG_8u_C1() failed!");
                    return JPEG_ERR_INTERNAL;
                  }
                  j += pixelToProcess;
                  pixelToProcess = std::min(intervalSize, srcWidth / m_dd_factor - j);
              }
              
              j = 0;
              pixelToProcess = std::min(2 * tileSize, 2 * srcWidth / m_dd_factor);
              while(j < 2 * (int) srcWidth / m_dd_factor)
              {
                  status = mfxiSampleUpRowH2V1_Triangle_JPEG_8u_C1(pTmp.get() + j, pixelToProcess, pDst + j * 2);
                  if(ippStsNoErr != status)
                  {
                    LOG0("Error: mfxiSampleUpRowH2V1_Triangle_JPEG_8u_C1() failed!");
                    return JPEG_ERR_INTERNAL;
                  }
                  j += pixelToProcess;
                  pixelToProcess = std::min(2 * intervalSize, 2 * srcWidth / m_dd_factor - j);
              }

              pSrc += srcStep;
              pDst += dstStep;
          }

        } // 411

        // arbitrary sampling
        if((curr_comp->m_h_factor != 1 || curr_comp->m_v_factor != 1) && need_upsampling)
        {
          int    srcStep;
          int    dstStep;
          int    v_step;
          int    h_step;
          int    val;
          uint8_t* pSrc;
          uint8_t* pDst;
          uint8_t* p;
          uint32_t srcWidth;

          srcWidth = (maxMCU - colMCU) * 8 * curr_comp->m_scan_hsampling;
          srcStep = curr_comp->m_ss_step;
          dstStep = curr_comp->m_cc_step;

          h_step  = curr_comp->m_h_factor;
          v_step  = curr_comp->m_v_factor;

          // set the pointer to source buffer
          pSrc = curr_comp->GetSSBufferPtr<uint8_t> (0) + 8 * colMCU * curr_comp->m_scan_hsampling;
          // set the pointer to destination buffer
          pDst = curr_comp->GetCCBufferPtr<uint8_t> (0) + 8 * colMCU * curr_comp->m_h_factor;

          for(n = 0; n < curr_comp->m_ss_height; n++)
          {
            p = pDst;
            for(i = 0; i < (int) srcWidth; i++)
            {
              val = pSrc[i];
              for(j = 0; j < h_step; j++)
                pDst[j] = (uint8_t)val;

              pDst += h_step;
            } //  for i

            for(c = 0; c < v_step - 1; c++)
            {
              status = mfxsCopy_8u(p,pDst,srcWidth);
              if(ippStsNoErr != status)
              {
                LOG0("Error: mfxs_Copy_8u() failed!");
                return JPEG_ERR_INTERNAL;
              }

              pDst += dstStep;
            } //for c

            pDst = p + dstStep*v_step;

            pSrc += srcStep;
          } // for n
        } // if
      } // for m_jpeg_ncomp
  }
  else
  {
      // for all scan components 
      for(k = m_curr_scan->first_comp; k < m_curr_scan->first_comp + m_curr_scan->ncomps; k++)
      {
        curr_comp       = &m_ccomp[k];

        // downsampling for both dimentions
        if(JS_444 == m_jpeg_sampling && k != 0)
        {
            int    srcStep;
            int    tmpStep;
            uint8_t* pSrc;
            uint8_t* pDst;
            mfxSize srcRoiSize;
            mfxSize tmpRoiSize;

            srcStep = curr_comp->m_cc_step;
            tmpStep = srcStep;            

            // set the pointer to source buffer
            pSrc = curr_comp->GetCCBufferPtr<uint8_t> (0) + 8 * colMCU;
            if(pSrc == nullptr)
              return JPEG_ERR_BUFF;
            // set the pointer to temporary buffer
            std::unique_ptr<uint8_t[]> pTmp( new uint8_t[tmpStep * m_curr_scan->mcuHeight / 2] );
            // set the pointer to destination buffer
            pDst = curr_comp->GetCCBufferPtr<uint8_t> (0) + 8 * colMCU / 2;  

            // set ROI of source
            srcRoiSize.width = (maxMCU - colMCU) * 8;
            srcRoiSize.height = m_curr_scan->mcuHeight;

            // set ROI of source
            tmpRoiSize.width = (maxMCU - colMCU) * 8 / 2;
            tmpRoiSize.height = m_curr_scan->mcuHeight / 2;

            status = mfxiSampleDownH2V2_JPEG_8u_C1R(pSrc, srcStep, srcRoiSize, pTmp.get(), tmpStep, tmpRoiSize);
            if(ippStsNoErr != status)
            {
                LOG0("Error: mfxiSampleDownH2V2_JPEG_8u_C1R() failed!");
                return JPEG_ERR_INTERNAL;
            }

            status = mfxsCopy_8u(pTmp.get(), pDst, tmpStep * m_curr_scan->mcuHeight / 2);
            if(ippStsNoErr != status)
            {
                LOG0("Error: mfxs_Copy_8u() failed!");
                return JPEG_ERR_INTERNAL;
            }
        }

        // vertical downsampling 
        if(JS_422H == m_jpeg_sampling && k != 0)
        {
            int    srcStep;
            int    dstStep;
            uint8_t* pSrc;
            uint8_t* pDst;
            mfxSize srcRoiSize;

            srcStep = curr_comp->m_ss_step;
            dstStep = curr_comp->m_cc_step;     

            // set the pointer to source buffer
            pSrc = curr_comp->GetSSBufferPtr<uint8_t> (0) + 8 * colMCU;
            // set the pointer to destination buffer
            pDst = curr_comp->GetCCBufferPtr<uint8_t> (0) + 8 * colMCU;  

            // set ROI of source
            srcRoiSize.width = (maxMCU - colMCU) * 8;
            srcRoiSize.height = m_curr_scan->mcuHeight;

            for(n = 0; n < srcRoiSize.height / 2; n++)
            {
                status = mfxsCopy_8u(pSrc, pDst, srcRoiSize.width);
                if(ippStsNoErr != status)
                {
                    LOG0("Error: mfxs_Copy_8u() failed!");
                    return JPEG_ERR_INTERNAL;
                }

                pSrc += 2 * srcStep;  
                pDst += dstStep;   
            }
        }

        //horisontal downsampling
        if(JS_422V == m_jpeg_sampling && k != 0)
        {
            int    srcStep;
            int    dstStep;
            uint8_t* pSrc;
            uint8_t* pDst;
            mfxSize srcRoiSize;
            mfxSize dstRoiSize;

            srcStep = curr_comp->m_ss_step;
            dstStep = curr_comp->m_cc_step;

            // set the pointer to source buffer
            pSrc = curr_comp->GetSSBufferPtr<uint8_t> (0) + m_curr_scan->mcuWidth * colMCU;
            // set the pointer to destination buffer
            pDst = curr_comp->GetCCBufferPtr<uint8_t> (0) + m_curr_scan->mcuWidth * colMCU / 2;

            // set ROI of source
            srcRoiSize.width = (maxMCU - colMCU) * m_curr_scan->mcuWidth;
            srcRoiSize.height = 8;

            // set ROI of destination
            dstRoiSize.width = (maxMCU - colMCU) * m_curr_scan->mcuWidth / 2;
            dstRoiSize.height = 8;

            status = mfxiSampleDownH2V1_JPEG_8u_C1R(pSrc, srcStep, srcRoiSize, pDst, dstStep, dstRoiSize);
            if(ippStsNoErr != status)
            {
                LOG0("Error: mfxiSampleDownH2V2_JPEG_8u_C1R() failed!");
                return JPEG_ERR_INTERNAL;
            }
        }

        // just copy data
        if(JS_420 == m_jpeg_sampling && k != 0)
        {
            int    srcStep;
            int    dstStep;
            uint8_t* pSrc;
            uint8_t* pDst;
            mfxSize srcRoiSize;

            srcStep = curr_comp->m_ss_step;
            dstStep = curr_comp->m_cc_step;

            // set the pointer to source buffer
            pSrc = curr_comp->GetSSBufferPtr<uint8_t> (0) + 8 * colMCU;
            // set the pointer to destination buffer
            pDst = curr_comp->GetCCBufferPtr<uint8_t> (0) + 8 * colMCU;

            // set ROI of source
            srcRoiSize.width = (maxMCU - colMCU) * 8;
            srcRoiSize.height = 8;

            for(n = 0; n < srcRoiSize.height; n++)
            {
                status = mfxsCopy_8u(pSrc, pDst, srcRoiSize.width);
                if(ippStsNoErr != status)
                {
                    LOG0("Error: mfxs_Copy_8u() failed!");
                    return JPEG_ERR_INTERNAL;
                }

                pSrc += srcStep;  
                pDst += dstStep;   
            }
        }

        // even colomns from even rows, odd colomns from odd rows
        if(JS_411 == m_jpeg_sampling && k != 0)
        {
            int    srcStep;
            int    dstStep;
            uint8_t* pSrc;
            uint8_t* pDst;
            mfxSize srcRoiSize;

            srcStep = curr_comp->m_ss_step;
            dstStep = curr_comp->m_cc_step;

            // set the pointer to source buffer
            pSrc = curr_comp->GetSSBufferPtr<uint8_t> (0) + 8 * colMCU;
            // set the pointer to destination buffer
            pDst = curr_comp->GetCCBufferPtr<uint8_t> (0) + 8 * colMCU * 2;  

            // set ROI of source
            srcRoiSize.width = (maxMCU - colMCU) * 8;
            srcRoiSize.height = m_curr_scan->mcuHeight;

            for(n = 0; n < srcRoiSize.height / 2; n++)
            { 
                for(i = 0; i < srcRoiSize.width; i++)
                {
                    pDst[n*dstStep + 2*i  ] = pSrc[2*n*srcStep + i];
                    pDst[n*dstStep + 2*i+1] = pSrc[2*n*srcStep + srcStep + i];
                }
            }
        }
      }
  }

  return JPEG_OK;
} // JERRCODE CJPEGDecoder::UpSampling(uint32_t rowMCU, uint32_t colMCU, uint32_t maxMCU)


JERRCODE CJPEGDecoder::ProcessBuffer(int nMCURow, int thread_id)
{
  int                  c;
  int                  yPadd = 0;
  int                  srcStep = 0;
  int                  dstStep = 0;
  int                  copyHeight = 0;
  int                  ssHeight;
  uint8_t*               pSrc8u  = 0;
  uint8_t*               pDst8u  = 0;
  uint16_t*              pSrc16u = 0;
  uint16_t*              pDst16u = 0;
  mfxSize             roi;
  int            status;
  CJPEGColorComponent* curr_comp;

  if(m_jpeg_precision <= 8)
  {
    for(c = 0; c < m_jpeg_ncomp; c++)
    {
      curr_comp = &m_ccomp[c];

      if(curr_comp->m_v_factor == 1  && curr_comp->m_h_factor == 1)
      {
        srcStep    = curr_comp->m_cc_step;
        pSrc8u     = curr_comp->GetCCBufferPtr(thread_id);
        copyHeight = curr_comp->m_ss_height;
        yPadd      = m_yPadding;
      }

      if(curr_comp->m_v_factor == 2  && curr_comp->m_h_factor == 2)
      {
        srcStep    = curr_comp->m_ss_step;
        pSrc8u     = curr_comp->GetSSBufferPtr(thread_id);
        copyHeight = curr_comp->m_ss_height - 2;
        yPadd      = m_yPadding/2;
        pSrc8u     = pSrc8u + srcStep; // skip upper border line
      }

      if(curr_comp->m_v_factor == 1  && curr_comp->m_h_factor == 2)
      {
        srcStep    = curr_comp->m_ss_step;
        pSrc8u     = curr_comp->GetSSBufferPtr(thread_id);
        copyHeight = curr_comp->m_ss_height;
        yPadd      = m_yPadding;
      }

      ssHeight = copyHeight;

      if(nMCURow == (int) m_numyMCU - 1)
      {
      copyHeight -= yPadd;
    }

    roi.width  = srcStep    / m_dd_factor;
    roi.height = copyHeight / m_dd_factor;

    if(roi.height == 0)
      return JPEG_OK;

    pDst8u = m_dst.p.Data8u[c] + nMCURow * ssHeight * m_dst.lineStep[c] / m_dd_factor;

    status = mfxiCopy_8u_C1R(pSrc8u, srcStep, pDst8u, m_dst.lineStep[c], roi);
    if(ippStsNoErr != status)
    {
      LOG1("IPP Error: mfxiCopy_8u_C1R() failed - ",status);
      return JPEG_ERR_INTERNAL;
    }
  }
  }
  else      // 16-bit(>= 8) planar image with YCBCR color and 444 sampling
  {
    for(c = 0; c < m_jpeg_ncomp; c++)
    {
      curr_comp  = &m_ccomp[c];
      srcStep    = curr_comp->m_cc_step;
      pSrc16u    = (uint16_t*)curr_comp->GetCCBufferPtr(thread_id);

      copyHeight = m_ccHeight;

      if(nMCURow == (int) m_numyMCU - 1)
      {
        copyHeight = m_mcuHeight - m_yPadding;
      }

      roi.width  = m_dst.width;
      roi.height = copyHeight;

      if(roi.height == 0)
        return JPEG_OK;

      dstStep  = m_dst.lineStep[c];

      pDst16u  = (uint16_t*)((uint8_t*)m_dst.p.Data16s[c] + nMCURow * m_mcuHeight * dstStep);

      status   = mfxiCopy_16s_C1R((int16_t*)pSrc16u, srcStep, (int16_t*)pDst16u, dstStep, roi);
      if(ippStsNoErr != status)
      {
        LOG1("IPP Error: mfxiCopy_16s_C1R() failed - ",status);
        return JPEG_ERR_INTERNAL;
      }
    } // for c
  }

  return JPEG_OK;
} // JERRCODE CJPEGDecoder::ProcessBuffer()


JERRCODE CJPEGDecoder::DecodeHuffmanMCURowBL(int16_t* pMCUBuf, uint32_t colMCU, uint32_t maxMCU)
{
  int       n, k, l;
  uint32_t j;
  int       srcLen;
  int       currPos;
  uint8_t*    src;
  JERRCODE  jerr;
  int status;

  src    = m_BitStreamIn.GetDataPtr();
  srcLen = m_BitStreamIn.GetDataLen();

  for(j = colMCU; j < maxMCU; j++)
  {
    if(m_curr_scan->jpeg_restart_interval)
    {
      if(m_restarts_to_go == 0)
      {
        jerr = ProcessRestart();
        if(JPEG_OK != jerr)
        {
          LOG0("Error: ProcessRestart() failed!");
          return jerr;
        }
      }
    }

    for(n = m_curr_scan->first_comp; n < m_curr_scan->first_comp + m_curr_scan->ncomps; n++)
    {
      int16_t*                lastDC = &m_ccomp[n].m_lastDC;
      if (m_ccomp[n].m_dc_selector >= MAX_HUFF_TABLES)
      {
          return JPEG_ERR_PARAMS;
      }
      if (m_dctbl[m_ccomp[n].m_dc_selector].IsEmpty())
          return JPEG_ERR_PARAMS;      
      IppiDecodeHuffmanSpec* dctbl  = m_dctbl[m_ccomp[n].m_dc_selector];
      IppiDecodeHuffmanSpec* actbl  = m_actbl[m_ccomp[n].m_ac_selector];
      for(k = 0; k < m_ccomp[n].m_scan_vsampling; k++)
      {
        for(l = 0; l < m_ccomp[n].m_scan_hsampling; l++)
        {
          m_BitStreamIn.FillBuffer(SAFE_NBYTES);

          currPos = m_BitStreamIn.GetCurrPos();

          status = mfxiDecodeHuffman8x8_JPEG_1u16s_C1(
                     src,srcLen,&currPos,pMCUBuf,lastDC,(int*)&m_marker,
                     dctbl,actbl,m_state);

          m_BitStreamIn.SetCurrPos(currPos);

          if(ippStsNoErr > status)
          {
            LOG0("Error: mfxiDecodeHuffman8x8_JPEG_1u16s_C1() failed!");
            m_marker = JM_NONE;
            return JPEG_ERR_INTERNAL;
          }

          pMCUBuf += DCTSIZE2;
        } // for m_hsampling
      } // for m_vsampling
    } // for m_jpeg_ncomp

    m_restarts_to_go--;
  } // for m_numxMCU

  return JPEG_OK;

} // JERRCODE CJPEGDecoder::DecodeHuffmanMCURowBL(int16_t* pMCUBuf, uint32_t colMCU, uint32_t maxMCU)


JERRCODE CJPEGDecoder::DecodeHuffmanMCURowLS(int16_t* pMCUBuf)
{
  int       c;
  uint8_t*    src;
  int16_t*   dst[4];
  int       srcLen;
  int       currPos;
  const IppiDecodeHuffmanSpec* dctbl[4];
  JERRCODE  jerr;
  int status;

  for(c = 0; c < m_jpeg_ncomp; c++)
  {
    dst[c] = pMCUBuf + c * m_numxMCU;
    dctbl[c] = m_dctbl[m_ccomp[c].m_dc_selector];
  }

  src    = m_BitStreamIn.GetDataPtr();
  srcLen = m_BitStreamIn.GetDataLen();

#if defined (HUFF_ROW_API)

  if(m_curr_scan->jpeg_restart_interval)
  {
    if(m_restarts_to_go == 0)
    {
      jerr = ProcessRestart();
      if(JPEG_OK != jerr)
      {
        LOG0("Error: ProcessRestart() failed!");
        return jerr;
      }
    }
  }

  m_BitStreamIn.FillBuffer();

  currPos = m_BitStreamIn.GetCurrPos();

  status = mfxiDecodeHuffmanRow_JPEG_1u16s_C1P4(
             src,srcLen,&currPos,dst, m_numxMCU, m_jpeg_ncomp, (int*)&m_marker,
             dctbl,m_state);

  m_BitStreamIn.SetCurrPos(currPos);

  if(ippStsNoErr > status)
  {
    LOG0("Error: mfxiDecodeHuffmanRow_JPEG_1u16s_C1P4() failed!");
    return JPEG_ERR_INTERNAL;
  }

  m_restarts_to_go -= m_numxMCU;

#else

  int                    j, h, v;
  CJPEGColorComponent*   curr_comp;

  for(j = 0; j < m_numxMCU; j++)
  {
    if(m_curr_scan->jpeg_restart_interval)
    {
      if(m_restarts_to_go == 0)
      {
        jerr = ProcessRestart();
        if(JPEG_OK != jerr)
        {
          LOG0("Error: ProcessRestart() failed!");
          return jerr;
        }
      }
    }

    for(c = 0; c < m_jpeg_ncomp; c++)
    {
      curr_comp = &m_ccomp[c];

      for(v = 0; v < curr_comp->m_vsampling; v++)
      {
        for(h = 0; h < curr_comp->m_hsampling; h++)
        {
          m_BitStreamIn.FillBuffer(SAFE_NBYTES);

          currPos = m_BitStreamIn.GetCurrPos();

          status = mfxiDecodeHuffmanOne_JPEG_1u16s_C1(
                     src,srcLen,&currPos,dst[c],(int*)&m_marker,
                     dctbl[c],m_state);

          m_BitStreamIn.SetCurrPos(currPos);

          if(ippStsNoErr > status)
          {
            LOG0("Error: mfxiDecodeHuffmanOne_JPEG_1u16s_C1() failed!");
            return JPEG_ERR_INTERNAL;
          }

          dst[c]++;
        } // for m_hsampling
      } // for m_vsampling
    } // for m_jpeg_ncomp

    m_restarts_to_go --;
  } // for m_numxMCU
#endif

  return JPEG_OK;
} // CJPEGDecoder::DecodeHuffmanMCURowLS()


JERRCODE CJPEGDecoder::ReconstructMCURowBL8x8_NxN(int16_t* pMCUBuf,
                                                  uint32_t colMCU,
                                                  uint32_t maxMCU)
{
  int       c, k, l, curr_lnz;
  uint32_t mcu_col;
  uint8_t*    lnz     = 0;
  uint8_t*    dst     = 0;
  int       dstStep = m_ccWidth;
  int status;
  CJPEGColorComponent* curr_comp;
  const int thread_id = 0;

  for(mcu_col = colMCU; mcu_col < maxMCU; mcu_col++)
  {
    for(c = 0; c < m_jpeg_ncomp; c++)
    {
      curr_comp = &m_ccomp[c];
      lnz       = m_ccomp[c].GetLNZBufferPtr(thread_id);
      curr_lnz  = mcu_col * curr_comp->m_lnz_ds;

      if (curr_comp->m_q_selector >= MAX_QUANT_TABLES)
      {
        return JPEG_ERR_PARAMS;
      }
      uint16_t* qtbl = m_qntbl[curr_comp->m_q_selector];
      if(!qtbl)
      {
          LOG1("Error: in CJPEGDecoder::ReconstructMCURowBL8x8_NxN() m_qntbl[] is empty for ",
               curr_comp->m_q_selector);
          return JPEG_ERR_INTERNAL;
      }

      for(k = 0; k < curr_comp->m_vsampling; k++)
      {
        if(curr_comp->m_hsampling == m_max_hsampling &&
           curr_comp->m_vsampling == m_max_vsampling)
        {
          dstStep = curr_comp->m_cc_step;
          dst     = curr_comp->GetCCBufferPtr(thread_id) + mcu_col*8*curr_comp->m_hsampling + k*8*dstStep;
        }
        else
        {
          dstStep = curr_comp->m_ss_step;
          dst     = curr_comp->GetSSBufferPtr(thread_id) + mcu_col*8*curr_comp->m_hsampling + k*8*dstStep;

          curr_comp->m_need_upsampling = 1;
        }

        // skip border row (when 244 or 411 sampling)
        if(curr_comp->m_h_factor == 2 && curr_comp->m_v_factor == 2)
        {
          dst += dstStep;
        }

        for(l = 0; l < curr_comp->m_hsampling; l++)
        {
          dst += l*8;


          if(lnz[curr_lnz] == 1)  // 1x1
          {
            status = mfxiDCTQuantInv8x8LS_1x1_JPEG_16s8u_C1R(pMCUBuf, dst, dstStep, qtbl);
          }
          else if(lnz[curr_lnz] < 5 && pMCUBuf[16] == 0) //2x2
          {
            status = mfxiDCTQuantInv8x8LS_2x2_JPEG_16s8u_C1R(pMCUBuf, dst, dstStep, qtbl);
          }
          else if(lnz[curr_lnz] <= 24 //4x4
                 && pMCUBuf[32] == 0
                 && pMCUBuf[33] == 0
                 && pMCUBuf[34] == 0
                 && pMCUBuf[4]  == 0
                 && pMCUBuf[12] == 0)
          {
            status = mfxiDCTQuantInv8x8LS_4x4_JPEG_16s8u_C1R(pMCUBuf, dst, dstStep, qtbl);
          }
          else      // 8x8
          {
            status = mfxiDCTQuantInv8x8LS_JPEG_16s8u_C1R(pMCUBuf, dst, dstStep, qtbl);
          }

          curr_lnz = curr_lnz + 1;

          if(ippStsNoErr > status)
          {
            LOG0("Error: mfxiDCTQuantInv8x8_NxNLS_JPEG_16s8u_C1R() failed!");
            return JPEG_ERR_INTERNAL;
          }

          pMCUBuf += DCTSIZE2;
        } // for m_hsampling
      } // for m_vsampling
    } // for m_jpeg_ncomp
  } // for m_numxMCU

  return JPEG_OK;
} // CJPEGDecoder::ReconstructMCURowBL8x8_NxN()


JERRCODE CJPEGDecoder::ReconstructMCURowBL8x8(int16_t* pMCUBuf,
                                              uint32_t colMCU,
                                              uint32_t maxMCU)
{
    int       c, k, l;
  uint32_t mcu_col;
  uint8_t*    dst     = 0;
  uint8_t*    p       = 0;
  int       dstStep = m_ccWidth;
  uint16_t*   qtbl;
  int status;
  CJPEGColorComponent* curr_comp;
  const uint32_t thread_id = 0;

  for(mcu_col = colMCU; mcu_col < maxMCU; mcu_col++)
  {
    for(c = m_curr_scan->first_comp; c < m_curr_scan->first_comp + m_curr_scan->ncomps; c++)
    {
      curr_comp = &m_ccomp[c];

      qtbl = m_qntbl[curr_comp->m_q_selector];
      if(!qtbl)
      {
        LOG1("Error: in CJPEGDecoder::ReconstructMCURowBL8x8_NxN() m_qntbl[] is empty for ",
             curr_comp->m_q_selector);
        return JPEG_ERR_INTERNAL;
      }

      for(k = 0; k < curr_comp->m_scan_vsampling; k++)
      {
        if(curr_comp->m_hsampling == m_max_hsampling &&
           curr_comp->m_vsampling == m_max_vsampling)
        {
          dstStep = curr_comp->m_cc_step;
          dst     = curr_comp->GetCCBufferPtr(thread_id) + mcu_col*8*curr_comp->m_scan_hsampling + k*8*dstStep;
        }
        else
        {
          dstStep = curr_comp->m_ss_step;
          dst     = curr_comp->GetSSBufferPtr(thread_id) + mcu_col*8*curr_comp->m_scan_hsampling + k*8*dstStep;

          curr_comp->m_need_upsampling = 1;
        }

        for(l = 0; l < curr_comp->m_scan_hsampling; l++)
        {
          p = dst + l*8;


          status = mfxiDCTQuantInv8x8LS_JPEG_16s8u_C1R(pMCUBuf, p, dstStep, qtbl);

          if(ippStsNoErr > status)
          {
            LOG0("Error: mfxiDCTQuantInv8x8LS_JPEG_16s8u_C1R() failed!");
            return JPEG_ERR_INTERNAL;
          }

          pMCUBuf += DCTSIZE2;
        } // for m_hsampling
      } // for m_vsampling
    } // for m_jpeg_ncomp
  } // for m_numxMCU

  return JPEG_OK;

} // CJPEGDecoder::ReconstructMCURowBL8x8()


JERRCODE CJPEGDecoder::ReconstructMCURowBL8x8To4x4(int16_t* pMCUBuf,
                                                   uint32_t colMCU,
                                                   uint32_t maxMCU)
{
  int       c, k, l;
  uint32_t mcu_col;
  uint8_t*    dst     = 0;
  int       dstStep = m_ccWidth;
  uint16_t*   qtbl;
  int status;
  CJPEGColorComponent* curr_comp;
  const int thread_id = 0;

  for(mcu_col = colMCU; mcu_col < maxMCU; mcu_col++)
  {
    for(c = 0; c < m_jpeg_ncomp; c++)
    {
      curr_comp = &m_ccomp[c];
      qtbl = m_qntbl[curr_comp->m_q_selector];
      if(!qtbl)
      {
        LOG1("Error: in CJPEGDecoder::ReconstructMCURowBL8x8To4x4() m_qntbl[] is empty for ",
             curr_comp->m_q_selector);
        return JPEG_ERR_INTERNAL;
      }

      for(k = 0; k < curr_comp->m_vsampling; k++)
      {
        if(curr_comp->m_hsampling == m_max_hsampling &&
           curr_comp->m_vsampling == m_max_vsampling)
        {
          dstStep = curr_comp->m_cc_step;
          dst     = curr_comp->GetCCBufferPtr(thread_id) + mcu_col*4*curr_comp->m_hsampling + k*4*dstStep;

          for(l = 0; l < curr_comp->m_hsampling; l++)
          {
            dst += ((l == 0) ? 0 : 1)*4;

          status = mfxiDCTQuantInv8x8To4x4LS_JPEG_16s8u_C1R(pMCUBuf, dst, dstStep, qtbl);

          if(ippStsNoErr > status)
          {
            LOG0("Error: mfxiDCTQuantInv8x8To4x4LS_JPEG_16s8u_C1R() failed!");
            return JPEG_ERR_INTERNAL;
          }
            pMCUBuf += DCTSIZE2;
          } // for m_hsampling
        }
        else
        {
          if(curr_comp->m_h_factor == 2 && curr_comp->m_v_factor == 2 && m_dst.order == JD_PIXEL)
          {
            dstStep = curr_comp->m_cc_step;
            dst     = curr_comp->GetCCBufferPtr(thread_id) + mcu_col*8*curr_comp->m_hsampling + k*8*dstStep;

            for(l = 0; l < curr_comp->m_hsampling; l++)
            {
              dst += ((l == 0) ? 0 : 1)*8;

              status = mfxiDCTQuantInv8x8LS_JPEG_16s8u_C1R(pMCUBuf, dst, dstStep, qtbl);

              if(ippStsNoErr > status)
              {
                LOG0("Error: mfxiDCTQuantInv8x8LS_JPEG_16s8u_C1R() failed!");
                return JPEG_ERR_INTERNAL;
              }
              pMCUBuf += DCTSIZE2;
            } // for m_hsampling
          }
          else
          {
            dstStep = curr_comp->m_ss_step;
            dst     = curr_comp->GetSSBufferPtr(thread_id) + mcu_col*4*curr_comp->m_hsampling + k*4*dstStep;

            curr_comp->m_need_upsampling = 1;

            // skip border row (when 244 or 411 sampling)
            if(curr_comp->m_h_factor == 2 && curr_comp->m_v_factor == 2)
            {
              dst += dstStep;
            }

            for(l = 0; l < curr_comp->m_hsampling; l++)
            {
              dst += ((l == 0) ? 0 : 1)*4;

              status = mfxiDCTQuantInv8x8To4x4LS_JPEG_16s8u_C1R(pMCUBuf, dst, dstStep, qtbl);

              if(ippStsNoErr > status)
              {
                LOG0("Error: mfxiDCTQuantInv8x8To4x4LS_JPEG_16s8u_C1R() failed!");
                return JPEG_ERR_INTERNAL;
              }
              pMCUBuf += DCTSIZE2;
            } // for m_hsampling
          }
        } // for m_hsampling
      } // for m_vsampling
    } // for m_jpeg_ncomp
  } // for m_numxMCU

  return JPEG_OK;
} // CJPEGDecoder::ReconstructMCURowBL8x8To4x4()


JERRCODE CJPEGDecoder::ReconstructMCURowBL8x8To2x2(int16_t* pMCUBuf,
                                                   uint32_t colMCU,
                                                   uint32_t maxMCU)
{
  int       c, k, l;
  uint32_t mcu_col;
  uint8_t*    dst     = 0;
  int       dstStep = m_ccWidth;
  uint16_t*   qtbl;
  int status;
  CJPEGColorComponent* curr_comp;
  const int thread_id = 0;

  for(mcu_col = colMCU; mcu_col < maxMCU; mcu_col++)
  {
    for(c = 0; c < m_jpeg_ncomp; c++)
    {
      curr_comp = &m_ccomp[c];
      qtbl = m_qntbl[curr_comp->m_q_selector];
      if(!qtbl)
      {
        LOG1("Error: in CJPEGDecoder::ReconstructMCURowBL8x8To4x4() m_qntbl[] is empty for ",
             curr_comp->m_q_selector);
        return JPEG_ERR_INTERNAL;
      }

      for(k = 0; k < curr_comp->m_vsampling; k++)
      {
        if(curr_comp->m_hsampling == m_max_hsampling &&
           curr_comp->m_vsampling == m_max_vsampling)
        {
          dstStep = curr_comp->m_cc_step;
          dst     = curr_comp->GetCCBufferPtr(thread_id) + mcu_col*2*curr_comp->m_hsampling + k*2*dstStep;

          for(l = 0; l < curr_comp->m_hsampling; l++)
          {
            dst += ((l == 0) ? 0 : 1)*2;

            status = mfxiDCTQuantInv8x8To2x2LS_JPEG_16s8u_C1R(pMCUBuf, dst, dstStep, qtbl);

            if(ippStsNoErr > status)
            {
              LOG0("Error: mfxiDCTQuantInv8x8To2x2LS_JPEG_16s8u_C1R() failed!");
              return JPEG_ERR_INTERNAL;
            }
            pMCUBuf += DCTSIZE2;
          } // for m_hsampling
        }
        else
        {
          if(curr_comp->m_h_factor == 2 && curr_comp->m_v_factor == 2 && m_dst.order == JD_PIXEL)
          {
            dstStep = curr_comp->m_cc_step;
            dst     = curr_comp->GetCCBufferPtr(thread_id) + mcu_col*4*curr_comp->m_hsampling + k*4*dstStep;

            for(l = 0; l < curr_comp->m_hsampling; l++)
            {
              dst += ((l == 0) ? 0 : 1)*4;

              status = mfxiDCTQuantInv8x8To4x4LS_JPEG_16s8u_C1R(pMCUBuf, dst, dstStep, qtbl);

              if(ippStsNoErr > status)
              {
                LOG0("Error: mfxiDCTQuantInv8x8To4x4LS_JPEG_16s8u_C1R() failed!");
                return JPEG_ERR_INTERNAL;
              }
              pMCUBuf += DCTSIZE2;
            } // for m_hsampling
          }
          else
          {
            dstStep = curr_comp->m_ss_step;
            dst     = curr_comp->GetSSBufferPtr(thread_id) + mcu_col*2*curr_comp->m_hsampling + k*2*dstStep;

            curr_comp->m_need_upsampling = 1;

            // skip border row (when 244 or 411 sampling)
            if(curr_comp->m_h_factor == 2 && curr_comp->m_v_factor == 2)
            {
              dst += dstStep;
            }

            for(l = 0; l < curr_comp->m_hsampling; l++)
            {
              dst += ((l == 0) ? 0 : 1)*2;

              status = mfxiDCTQuantInv8x8To2x2LS_JPEG_16s8u_C1R(pMCUBuf, dst, dstStep, qtbl);

              if(ippStsNoErr > status)
              {
                LOG0("Error: mfxiDCTQuantInv8x8To2x2LS_JPEG_16s8u_C1R() failed!");
                return JPEG_ERR_INTERNAL;
              }
              pMCUBuf += DCTSIZE2;
            } // for m_hsampling
          }
        } // for m_hsampling
      } // for m_vsampling
    } // for m_jpeg_ncomp
  } // for m_numxMCU

  return JPEG_OK;
} // CJPEGDecoder::ReconstructMCURowBL8x8To2x2()


JERRCODE CJPEGDecoder::ReconstructMCURowBL8x8To1x1(int16_t* pMCUBuf,
                                                   uint32_t colMCU,
                                                   uint32_t maxMCU)
{
  int       c, k, l;
  uint32_t mcu_col;
  uint8_t*    dst     = 0;
  int       dstStep = m_ccWidth;
  uint16_t*   qtbl;
  int status;

  CJPEGColorComponent* curr_comp;
  const int thread_id = 0;

  for(mcu_col = colMCU; mcu_col < maxMCU; mcu_col++)
  {
    for(c = 0; c < m_jpeg_ncomp; c++)
    {
      curr_comp = &m_ccomp[c];
      qtbl = m_qntbl[curr_comp->m_q_selector];
      if(!qtbl)
      {
          LOG1("Error: in CJPEGDecoder::ReconstructMCURowBL8x8To1x1() m_qntbl[] is empty for ",
               curr_comp->m_q_selector);
          return JPEG_ERR_INTERNAL;
      }

      for(k = 0; k < curr_comp->m_vsampling; k++)
      {
        if(curr_comp->m_hsampling == m_max_hsampling &&
           curr_comp->m_vsampling == m_max_vsampling)
        {
          dstStep = curr_comp->m_cc_step;
          dst     = curr_comp->GetCCBufferPtr(thread_id) + mcu_col*curr_comp->m_hsampling + k*dstStep;

          for(l = 0; l < curr_comp->m_hsampling; l++)
          {
            dst += (l == 0) ? 0 : 1;

            DCT_QUANT_INV8x8To1x1LS(pMCUBuf, dst, qtbl);

            pMCUBuf += DCTSIZE2;
          } // for m_hsampling
        }
        else
        {
          if(curr_comp->m_h_factor == 2 && curr_comp->m_v_factor == 2 && m_dst.order == JD_PIXEL)
          {
            dstStep = curr_comp->m_cc_step;
            dst     = curr_comp->GetCCBufferPtr(thread_id) + mcu_col*2*curr_comp->m_hsampling + k*2*dstStep;

            for(l = 0; l < curr_comp->m_hsampling; l++)
            {
              dst += ((l == 0) ? 0 : 1)*2;

              status = mfxiDCTQuantInv8x8To2x2LS_JPEG_16s8u_C1R(pMCUBuf, dst, dstStep, qtbl);

              if(ippStsNoErr > status)
              {
                LOG0("Error: mfxiDCTQuantInv8x8To2x2LS_JPEG_16s8u_C1R() failed!");
                return JPEG_ERR_INTERNAL;
              }
              pMCUBuf += DCTSIZE2;
            } // for m_hsampling
          }
          else
          {
            dstStep = curr_comp->m_ss_step;
            dst     = curr_comp->GetSSBufferPtr(thread_id) + mcu_col*curr_comp->m_hsampling + k*dstStep;

            curr_comp->m_need_upsampling = 1;

            // skip border row (when 244 or 411 sampling)
            if(curr_comp->m_h_factor == 2 && curr_comp->m_v_factor == 2)
            {
              dst += dstStep;
            }

            for(l = 0; l < curr_comp->m_hsampling; l++)
            {
              dst += (l == 0) ? 0 : 1;

              DCT_QUANT_INV8x8To1x1LS(pMCUBuf, dst, qtbl);

              pMCUBuf += DCTSIZE2;
            } // for m_hsampling
          } // if
        } // for m_hsampling
      } // for m_vsampling
    } // for m_jpeg_ncomp
  } // for m_numxMCU

  return JPEG_OK;
} // CJPEGDecoder::ReconstructMCURowBL8x8To1x1()


JERRCODE CJPEGDecoder::ReconstructMCURowEX(int16_t* pMCUBuf,
                                           uint32_t colMCU,
                                           uint32_t maxMCU)
{
  int       c, k, l;
  uint32_t mcu_col;
  uint16_t*   dst = 0;
  int       dstStep;
  float*   qtbl;
  int status;
  CJPEGColorComponent* curr_comp;
  const int thread_id = 0;

  for(mcu_col = colMCU; mcu_col < maxMCU; mcu_col++)
  {
    for(c = 0; c < m_jpeg_ncomp; c++)
    {
      curr_comp = &m_ccomp[c];

      qtbl = m_qntbl[curr_comp->m_q_selector];
      if(!qtbl)
      {
        LOG1("Error: in CJPEGDecoder::ReconstructMCURowBL8x8To1x1() m_qntbl[] is empty for ",
             curr_comp->m_q_selector);
        return JPEG_ERR_INTERNAL;
      }

      for(k = 0; k < curr_comp->m_vsampling; k++)
      {
        if(curr_comp->m_hsampling == m_max_hsampling &&
           curr_comp->m_vsampling == m_max_vsampling)
        {
          dstStep = curr_comp->m_cc_step;
          dst     = (uint16_t*)(curr_comp->GetCCBufferPtr(thread_id) + mcu_col*8*curr_comp->m_hsampling*sizeof(int16_t) + k*8*dstStep*sizeof(int16_t));
        }
        else
        {
          dstStep = curr_comp->m_ss_step;
          dst     = (uint16_t*)(curr_comp->GetSSBufferPtr(thread_id) + mcu_col*8*curr_comp->m_hsampling*sizeof(int16_t) + k*8*dstStep*sizeof(int16_t));

          curr_comp->m_need_upsampling = 1;
        }

        // skip border row (when 244 or 411 sampling)
        if(curr_comp->m_h_factor == 2 && curr_comp->m_v_factor == 2)
        {
          dst += dstStep;
        }

        for(l = 0; l < curr_comp->m_hsampling; l++)
        {
          dst += l*8;

          status = mfxiDCTQuantInv8x8LS_JPEG_16s16u_C1R(
                     pMCUBuf,dst,dstStep,qtbl);
          if(ippStsNoErr > status)
          {
            LOG0("Error: mfxiDCTQuantInv8x8LS_JPEG_16s16u_C1R() failed!");
            return JPEG_ERR_INTERNAL;
          }



          pMCUBuf += DCTSIZE2;
        } // for m_hsampling
      } // for m_vsampling
    } // for m_jpeg_ncomp
  } // for m_numxMCU

  return JPEG_OK;
} // CJPEGDecoder::ReconstructMCURowEX()


JERRCODE CJPEGDecoder::ReconstructMCURowLS(
  int16_t* pMCUBuf,
  int     nMCURow,
  int     thread_id)
{
  int       n;
  int       dstStep;
  int16_t*   ptr;
  int16_t*   pCurrRow;
  int16_t*   pPrevRow;
  uint8_t*    pDst8u  = 0;
  int16_t*   pDst16s = 0;
  mfxSize  roi;
  int status;
  CJPEGColorComponent* curr_comp;

  roi.width  = m_dst.width;
  roi.height = 1;

  for(n = 0; n < m_jpeg_ncomp; n++)
  {
    curr_comp = &m_ccomp[n];

    if(m_dst.precision <= 8)
    {
      dstStep = curr_comp->m_cc_step;
      pDst8u  = curr_comp->GetCCBufferPtr(thread_id);
    }
    else
    {
      dstStep = curr_comp->m_cc_step;
      pDst16s = (int16_t*)curr_comp->GetCCBufferPtr(thread_id);
    }

    if(m_jpeg_ncomp == m_curr_scan->ncomps)
      ptr = pMCUBuf + n*m_numxMCU;
    else
      ptr = pMCUBuf + n*m_numxMCU*m_numyMCU + nMCURow*m_numxMCU;

    pCurrRow = curr_comp->m_curr_row;
    pPrevRow = curr_comp->m_prev_row;

    if(0 != nMCURow && 0 == m_rst_go)
    {
      status = mfxiReconstructPredRow_JPEG_16s_C1(
        ptr,pPrevRow,pCurrRow,m_dst.width,m_ss);
    }
    else
    {
      status = mfxiReconstructPredFirstRow_JPEG_16s_C1(
        ptr,pCurrRow,m_dst.width,m_jpeg_precision,m_al);
    }

    if(ippStsNoErr != status)
    {
      return JPEG_ERR_INTERNAL;
    }

    // do point-transform if any
    status = mfxsLShiftC_16s(pCurrRow,m_al,pPrevRow,m_dst.width);
    if(ippStsNoErr != status)
    {
      return JPEG_ERR_INTERNAL;
    }

    if(m_dst.precision <= 8)
    {
      status = mfxiAndC_16u_C1IR(0xFF, (uint16_t*)pPrevRow, m_dst.width*sizeof(int16_t),roi);
      status = mfxiConvert_16u8u_C1R((uint16_t*)pPrevRow,m_dst.width*sizeof(int16_t),pDst8u,dstStep,roi);
    }
    else
      status = mfxsCopy_16s(pPrevRow,pDst16s,m_dst.width);

    if(ippStsNoErr != status)
    {
      return JPEG_ERR_INTERNAL;
    }

    curr_comp->m_curr_row = pPrevRow;
    curr_comp->m_prev_row = pCurrRow;
  } // for m_jpeg_ncomp

  m_rst_go = 0;

  return JPEG_OK;
} // CJPEGDecoder::ReconstructMCURowLS()


JERRCODE CJPEGDecoder::DecodeScanBaseline(void)
{
    int status;
  JERRCODE  jerr = JPEG_OK;

  status = mfxiDecodeHuffmanStateInit_JPEG_8u(m_state);
  if(ippStsNoErr != status)
  {
    return JPEG_ERR_INTERNAL;
  }

  if(0 == m_curr_scan->scan_no)
  {
      m_curr_scan->first_comp = 0;
  }
  else if(1 == m_curr_scan->scan_no && (2 == m_curr_scan->ncomps || (1 == m_curr_scan->ncomps && 3 == m_num_scans)))
  {
      m_curr_scan->first_comp = 1;
  }
  else if((1 == m_curr_scan->scan_no && 1 == m_curr_scan->ncomps && 2 == m_num_scans) || 2 == m_curr_scan->scan_no)
  {
      m_curr_scan->first_comp = 2;
  }
  else
  {
      return JPEG_ERR_SOS_DATA;
  }

  m_marker = JM_NONE;

  // workaround for 8-bit qnt tables in 12-bit scans
  if(m_qntbl[0].m_initialized && m_qntbl[0].m_precision == 0 && m_jpeg_precision == 12)
    m_qntbl[0].ConvertToHighPrecision();

  if(m_qntbl[1].m_initialized && m_qntbl[1].m_precision == 0 && m_jpeg_precision == 12)
    m_qntbl[1].ConvertToHighPrecision();

  if(m_qntbl[2].m_initialized && m_qntbl[2].m_precision == 0 && m_jpeg_precision == 12)
    m_qntbl[2].ConvertToHighPrecision();

  if(m_qntbl[3].m_initialized && m_qntbl[3].m_precision == 0 && m_jpeg_precision == 12)
    m_qntbl[3].ConvertToHighPrecision();

  // workaround for 16-bit qnt tables in 8-bit scans
  if(m_qntbl[0].m_initialized && m_qntbl[0].m_precision == 1 && m_jpeg_precision == 8)
    m_qntbl[0].ConvertToLowPrecision();

  if(m_qntbl[1].m_initialized && m_qntbl[1].m_precision == 1 && m_jpeg_precision == 8)
    m_qntbl[1].ConvertToLowPrecision();

  if(m_qntbl[2].m_initialized && m_qntbl[2].m_precision == 1 && m_jpeg_precision == 8)
    m_qntbl[2].ConvertToLowPrecision();

  if(m_qntbl[3].m_initialized && m_qntbl[3].m_precision == 1 && m_jpeg_precision == 8)
    m_qntbl[3].ConvertToLowPrecision();

  if(m_dctbl[0].IsEmpty())
  {
    jerr = m_dctbl[0].Create();
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_dctbl[0].Init(0,0,(uint8_t*)&DefaultLuminanceDCBits[0],(uint8_t*)&DefaultLuminanceDCValues[0]);
    if(JPEG_OK != jerr)
      return jerr;
  }

  if(m_dctbl[1].IsEmpty())
  {
    jerr = m_dctbl[1].Create();
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_dctbl[1].Init(1,0,(uint8_t*)&DefaultChrominanceDCBits[0],(uint8_t*)&DefaultChrominanceDCValues[0]);
    if(JPEG_OK != jerr)
      return jerr;
  }

  if(m_actbl[0].IsEmpty())
  {
    jerr = m_actbl[0].Create();
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_actbl[0].Init(0,1,(uint8_t*)&DefaultLuminanceACBits[0],(uint8_t*)&DefaultLuminanceACValues[0]);
    if(JPEG_OK != jerr)
      return jerr;
  }

  if(m_actbl[1].IsEmpty())
  {
    jerr = m_actbl[1].Create();
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_actbl[1].Init(1,1,(uint8_t*)&DefaultChrominanceACBits[0],(uint8_t*)&DefaultChrominanceACValues[0]);
    if(JPEG_OK != jerr)
      return jerr;
  }

    {
        int16_t* pMCUBuf;
        uint32_t rowMCU, colMCU, maxMCU;
        uint32_t numxMCU = m_curr_scan->numxMCU;
        uint32_t numyMCU = m_curr_scan->numyMCU;

        // the pointer to Buffer for a current thread.
        pMCUBuf = m_block_buffer;

        // set the iterators
        rowMCU = 0;
        colMCU = 0;
        maxMCU = numxMCU;
        if (m_curr_scan->jpeg_restart_interval)
        {
            if(numxMCU == 0)
              return JPEG_ERR_PARAMS;
            rowMCU = m_mcu_decoded / numxMCU;
            colMCU = m_mcu_decoded % numxMCU;
            maxMCU = (numxMCU < colMCU + m_mcu_to_decode) ?
                     (numxMCU) :
                     (colMCU + m_mcu_to_decode);
        }

        while (rowMCU < numyMCU)
        {
            // decode a MCU row
            if(m_numxMCU * m_nblock * DCTSIZE2 > (uint32_t)m_block_buffer_size)
              return JPEG_ERR_BUFF;
            mfxsZero_16s(pMCUBuf, m_numxMCU * m_nblock * DCTSIZE2);

            jerr = DecodeHuffmanMCURowBL(pMCUBuf, colMCU, maxMCU);
            if (JPEG_OK != jerr)
                return jerr;

            // reconstruct a MCU row
            if(m_jpeg_precision == 12)
                jerr = ReconstructMCURowEX(pMCUBuf, colMCU, maxMCU);
            else
            {
                switch (m_jpeg_dct_scale)
                {
                default:
                case JD_1_1:
                    {
                        if(m_use_qdct)
                            jerr = ReconstructMCURowBL8x8_NxN(pMCUBuf, colMCU, maxMCU);
                        else
                            jerr = ReconstructMCURowBL8x8(pMCUBuf, colMCU, maxMCU);
                    }
                    break;

                case JD_1_2:
                    {
                        jerr = ReconstructMCURowBL8x8To4x4(pMCUBuf, colMCU, maxMCU);
                    }
                    break;

                case JD_1_4:
                    {
                        jerr = ReconstructMCURowBL8x8To2x2(pMCUBuf, colMCU, maxMCU);
                    }
                    break;

                case JD_1_8:
                    {
                        jerr = ReconstructMCURowBL8x8To1x1(pMCUBuf, colMCU, maxMCU);
                    }
                    break;
                }
            }

            if (JPEG_OK != jerr)
                return jerr;


            jerr = UpSampling(rowMCU, colMCU, maxMCU);
            if (JPEG_OK != jerr)
                return jerr;

            jerr = ColorConvert(rowMCU, colMCU, maxMCU);
            if (JPEG_OK != jerr)
                return jerr;

            // increment interators
            if (m_curr_scan->jpeg_restart_interval)
            {
                m_mcu_decoded += (maxMCU - colMCU);
                m_mcu_to_decode -= (maxMCU - colMCU);
                if (0 == m_mcu_to_decode)
                {
                    return JPEG_OK;
                }
                maxMCU = (numxMCU < m_mcu_to_decode) ?
                         (numxMCU) :
                         (m_mcu_to_decode);
            }
            else
            {
                maxMCU = numxMCU;
            }

            rowMCU += 1;
            colMCU = 0;
        }
    }

    return JPEG_OK;
} // CJPEGDecoder::DecodeScanBaseline()


JERRCODE CJPEGDecoder::DecodeScanBaselineIN(void)
{
  int status;
  JERRCODE  jerr = JPEG_OK;

  status = mfxiDecodeHuffmanStateInit_JPEG_8u(m_state);
  if(ippStsNoErr != status)
  {
    return JPEG_ERR_INTERNAL;
  }

  m_marker = JM_NONE;

  // workaround for 8-bit qnt tables in 12-bit scans
  if(m_qntbl[0].m_initialized && m_qntbl[0].m_precision == 0 && m_jpeg_precision == 12)
    m_qntbl[0].ConvertToHighPrecision();

  if(m_qntbl[1].m_initialized && m_qntbl[1].m_precision == 0 && m_jpeg_precision == 12)
    m_qntbl[1].ConvertToHighPrecision();

  if(m_qntbl[2].m_initialized && m_qntbl[2].m_precision == 0 && m_jpeg_precision == 12)
    m_qntbl[2].ConvertToHighPrecision();

  if(m_qntbl[3].m_initialized && m_qntbl[3].m_precision == 0 && m_jpeg_precision == 12)
    m_qntbl[3].ConvertToHighPrecision();

  // workaround for 16-bit qnt tables in 8-bit scans
  if(m_qntbl[0].m_initialized && m_qntbl[0].m_precision == 1 && m_jpeg_precision == 8)
    m_qntbl[0].ConvertToLowPrecision();

  if(m_qntbl[1].m_initialized && m_qntbl[1].m_precision == 1 && m_jpeg_precision == 8)
    m_qntbl[1].ConvertToLowPrecision();

  if(m_qntbl[2].m_initialized && m_qntbl[2].m_precision == 1 && m_jpeg_precision == 8)
    m_qntbl[2].ConvertToLowPrecision();

  if(m_qntbl[3].m_initialized && m_qntbl[3].m_precision == 1 && m_jpeg_precision == 8)
    m_qntbl[3].ConvertToLowPrecision();

  if(m_dctbl[0].IsEmpty())
  {
    jerr = m_dctbl[0].Create();
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_dctbl[0].Init(0,0,(uint8_t*)&DefaultLuminanceDCBits[0],(uint8_t*)&DefaultLuminanceDCValues[0]);
    if(JPEG_OK != jerr)
      return jerr;
  }

  if(m_dctbl[1].IsEmpty())
  {
    jerr = m_dctbl[1].Create();
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_dctbl[1].Init(1,0,(uint8_t*)&DefaultChrominanceDCBits[0],(uint8_t*)&DefaultChrominanceDCValues[0]);
    if(JPEG_OK != jerr)
      return jerr;
  }

  if(m_actbl[0].IsEmpty())
  {
    jerr = m_actbl[0].Create();
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_actbl[0].Init(0,1,(uint8_t*)&DefaultLuminanceACBits[0],(uint8_t*)&DefaultLuminanceACValues[0]);
    if(JPEG_OK != jerr)
      return jerr;
  }

  if(m_actbl[1].IsEmpty())
  {
    jerr = m_actbl[1].Create();
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_actbl[1].Init(1,1,(uint8_t*)&DefaultChrominanceACBits[0],(uint8_t*)&DefaultChrominanceACValues[0]);
    if(JPEG_OK != jerr)
      return jerr;
  }

    {
        int16_t* pMCUBuf;
        uint32_t rowMCU, colMCU, maxMCU;

        // the pointer to Buffer for a current thread.
        pMCUBuf = m_block_buffer;

        // set the iterators
        rowMCU = 0;
        colMCU = 0;
        maxMCU = m_numxMCU;
        if (m_curr_scan->jpeg_restart_interval)
        {
            rowMCU = m_mcu_decoded / m_numxMCU;
            colMCU = m_mcu_decoded % m_numxMCU;
            maxMCU = (m_numxMCU < colMCU + m_mcu_to_decode) ?
                     (m_numxMCU) :
                     (colMCU + m_mcu_to_decode);
        }

        while (rowMCU < m_numyMCU)
        {
            // decode a MCU row
            mfxsZero_16s(pMCUBuf,m_numxMCU * m_nblock * DCTSIZE2);

            jerr = DecodeHuffmanMCURowBL(pMCUBuf, colMCU, maxMCU);
            if (JPEG_OK != jerr)
                return jerr;

            // reconstruct a MCU row
            if(m_jpeg_precision == 12)
                jerr = ReconstructMCURowEX(pMCUBuf, colMCU, maxMCU);
            else
            {
                switch (m_jpeg_dct_scale)
                {
                default:
                case JD_1_1:
                    {
                        if(m_use_qdct)
                            jerr = ReconstructMCURowBL8x8_NxN(pMCUBuf, colMCU, maxMCU);
                        else
                            jerr = ReconstructMCURowBL8x8(pMCUBuf, colMCU, maxMCU);
                    }
                    break;

                case JD_1_2:
                    {
                        jerr = ReconstructMCURowBL8x8To4x4(pMCUBuf, colMCU, maxMCU);
                    }
                    break;

                case JD_1_4:
                    {
                        jerr = ReconstructMCURowBL8x8To2x2(pMCUBuf, colMCU, maxMCU);
                    }
                    break;

                case JD_1_8:
                    {
                        jerr = ReconstructMCURowBL8x8To1x1(pMCUBuf, colMCU, maxMCU);
                    }
                    break;
                }
            }

            if(JPEG_OK != jerr)
                continue;


            jerr = UpSampling(rowMCU, colMCU, maxMCU);
            if(JPEG_OK != jerr)
                continue;

            jerr = ColorConvert(rowMCU, colMCU, maxMCU);
            if(JPEG_OK != jerr)
                continue;

            // increment interators
            if (m_curr_scan->jpeg_restart_interval)
            {
                m_mcu_decoded += (maxMCU - colMCU);
                m_mcu_to_decode -= (maxMCU - colMCU);
                if (0 == m_mcu_to_decode)
                {
                    return JPEG_ERR_BUFF;
                }
                maxMCU = (m_numxMCU < m_mcu_to_decode) ?
                         (m_numxMCU) :
                         (m_mcu_to_decode);
            }
            else
            {
                maxMCU = m_numxMCU;
            }

            rowMCU += 1;
            colMCU = 0;
        }
    }

    return JPEG_OK;

} // CJPEGDecoder::DecodeScanBaselineIN()


JERRCODE CJPEGDecoder::DecodeScanBaselineIN_P(void)
{
  int status;
  JERRCODE  jerr = JPEG_OK;

  status = mfxiDecodeHuffmanStateInit_JPEG_8u(m_state);
  if(ippStsNoErr != status)
  {
    return JPEG_ERR_INTERNAL;
  }

  m_marker = JM_NONE;

  // workaround for 8-bit qnt tables in 12-bit scans
  if(m_qntbl[0].m_initialized && m_qntbl[0].m_precision == 0 && m_jpeg_precision == 12)
    m_qntbl[0].ConvertToHighPrecision();

  if(m_qntbl[1].m_initialized && m_qntbl[1].m_precision == 0 && m_jpeg_precision == 12)
    m_qntbl[1].ConvertToHighPrecision();

  if(m_qntbl[2].m_initialized && m_qntbl[2].m_precision == 0 && m_jpeg_precision == 12)
    m_qntbl[2].ConvertToHighPrecision();

  if(m_qntbl[3].m_initialized && m_qntbl[3].m_precision == 0 && m_jpeg_precision == 12)
    m_qntbl[3].ConvertToHighPrecision();

  // workaround for 16-bit qnt tables in 8-bit scans
  if(m_qntbl[0].m_initialized && m_qntbl[0].m_precision == 1 && m_jpeg_precision == 8)
    m_qntbl[0].ConvertToLowPrecision();

  if(m_qntbl[1].m_initialized && m_qntbl[1].m_precision == 1 && m_jpeg_precision == 8)
    m_qntbl[1].ConvertToLowPrecision();

  if(m_qntbl[2].m_initialized && m_qntbl[2].m_precision == 1 && m_jpeg_precision == 8)
    m_qntbl[2].ConvertToLowPrecision();

  if(m_qntbl[3].m_initialized && m_qntbl[3].m_precision == 1 && m_jpeg_precision == 8)
    m_qntbl[3].ConvertToLowPrecision();

  if(m_dctbl[0].IsEmpty())
  {
    jerr = m_dctbl[0].Create();
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_dctbl[0].Init(0,0,(uint8_t*)&DefaultLuminanceDCBits[0],(uint8_t*)&DefaultLuminanceDCValues[0]);
    if(JPEG_OK != jerr)
      return jerr;
  }

  if(m_dctbl[1].IsEmpty())
  {
    jerr = m_dctbl[1].Create();
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_dctbl[1].Init(1,0,(uint8_t*)&DefaultChrominanceDCBits[0],(uint8_t*)&DefaultChrominanceDCValues[0]);
    if(JPEG_OK != jerr)
      return jerr;
  }

  if(m_actbl[0].IsEmpty())
  {
    jerr = m_actbl[0].Create();
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_actbl[0].Init(0,1,(uint8_t*)&DefaultLuminanceACBits[0],(uint8_t*)&DefaultLuminanceACValues[0]);
    if(JPEG_OK != jerr)
      return jerr;
  }

  if(m_actbl[1].IsEmpty())
  {
    jerr = m_actbl[1].Create();
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_actbl[1].Init(1,1,(uint8_t*)&DefaultChrominanceACBits[0],(uint8_t*)&DefaultChrominanceACValues[0]);
    if(JPEG_OK != jerr)
      return jerr;
  }

  {
    int     i;
    int     idThread = 0;
    int16_t* pMCUBuf;  // the pointer to Buffer for a current thread.


    pMCUBuf = m_block_buffer + idThread * m_numxMCU * m_nblock * DCTSIZE2;

    i = 0;

    while(i < (int) m_numyMCU)
    {
      {
        if(i < (int) m_numyMCU)
        {
          mfxsZero_16s(pMCUBuf,m_numxMCU * m_nblock * DCTSIZE2);

          jerr = DecodeHuffmanMCURowBL(pMCUBuf, 0, m_numxMCU);
//          if(JPEG_OK != jerr)
//            i = m_numyMCU;
        }
      }

      if(i < (int) m_numyMCU)
      {
        if(m_jpeg_precision == 12)
          jerr = ReconstructMCURowEX(pMCUBuf, 0, m_numxMCU);
        else
        {
        switch (m_jpeg_dct_scale)
        {
          case JD_1_1:
          {
            if(m_use_qdct)
              jerr = ReconstructMCURowBL8x8_NxN(pMCUBuf, 0, m_numxMCU);
            else
              jerr = ReconstructMCURowBL8x8(pMCUBuf, 0, m_numxMCU);
          }
          break;

          case JD_1_2:
          {
            jerr = ReconstructMCURowBL8x8To4x4(pMCUBuf, 0, m_numxMCU);
          }
          break;

          case JD_1_4:
          {
            jerr = ReconstructMCURowBL8x8To2x2(pMCUBuf, 0, m_numxMCU);
          }
          break;

          case JD_1_8:
          {
            jerr = ReconstructMCURowBL8x8To1x1(pMCUBuf, 0, m_numxMCU);
          }
          break;

          default:
            break;
          }
        }

        if(JPEG_OK != jerr)
          continue;


        jerr = ProcessBuffer(i,idThread);
        if(JPEG_OK != jerr)
          continue;

      }

      i++;
    } // for m_numyMCU
  } // OMP

  return JPEG_OK;
} // CJPEGDecoder::DecodeScanBaselineIN_P()


JERRCODE CJPEGDecoder::DecodeScanBaselineNI(void)
{
  int       i, j, k, l, c;
  int       srcLen;
  int       currPos;
  uint8_t*    src;
  int16_t*   block;
  JERRCODE  jerr;
  int status;

  m_ac_scans_completed += m_curr_scan->ncomps;

  status = mfxiDecodeHuffmanStateInit_JPEG_8u(m_state);
  if(ippStsNoErr != status)
  {
    return JPEG_ERR_INTERNAL;
  }

  m_marker = JM_NONE;

  src    = m_BitStreamIn.GetDataPtr();
  srcLen = m_BitStreamIn.GetDataLen();

  if(m_dctbl[0].IsEmpty())
  {
    jerr = m_dctbl[0].Create();
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_dctbl[0].Init(0,0,(uint8_t*)&DefaultLuminanceDCBits[0],(uint8_t*)&DefaultLuminanceDCValues[0]);
    if(JPEG_OK != jerr)
      return jerr;
  }

  if(m_dctbl[1].IsEmpty())
  {
    jerr = m_dctbl[1].Create();
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_dctbl[1].Init(1,0,(uint8_t*)&DefaultChrominanceDCBits[0],(uint8_t*)&DefaultChrominanceDCValues[0]);
    if(JPEG_OK != jerr)
      return jerr;
  }

  if(m_actbl[0].IsEmpty())
  {
    jerr = m_actbl[0].Create();
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_actbl[0].Init(0,1,(uint8_t*)&DefaultLuminanceACBits[0],(uint8_t*)&DefaultLuminanceACValues[0]);
    if(JPEG_OK != jerr)
      return jerr;
  }

  if(m_actbl[1].IsEmpty())
  {
    jerr = m_actbl[1].Create();
    if(JPEG_OK != jerr)
      return jerr;

    jerr = m_actbl[1].Init(1,1,(uint8_t*)&DefaultChrominanceACBits[0],(uint8_t*)&DefaultChrominanceACValues[0]);
    if(JPEG_OK != jerr)
      return jerr;
  }

  for(i = 0; i < (int) m_numyMCU; i++)
  {
    for(k = 0; k < m_ccomp[m_curr_comp_no].m_vsampling; k++)
    {
      if(i*m_ccomp[m_curr_comp_no].m_vsampling*8 + k*8 >= m_jpeg_height)
        break;

      for(j = 0; j < (int) m_numxMCU; j++)
      {
        for(c = 0; c < m_curr_scan->ncomps; c++)
        {
          block = m_block_buffer + (DCTSIZE2*m_nblock*(j+(i*m_numxMCU)));

          // skip any relevant components
          for(c = 0; c < m_ccomp[m_curr_comp_no].m_comp_no; c++)
          {
            block += (DCTSIZE2*m_ccomp[c].m_nblocks);
          }

          // Skip over relevant 8x8 blocks from this component.
          block += (k * DCTSIZE2 * m_ccomp[m_curr_comp_no].m_hsampling);

          for(l = 0; l < m_ccomp[m_curr_comp_no].m_hsampling; l++)
          {
            // Ignore the last column(s) of the image.
            if(((j*m_ccomp[m_curr_comp_no].m_hsampling*8) + (l*8)) >= m_jpeg_width)
              break;

            if(m_curr_scan->jpeg_restart_interval)
            {
              if(m_restarts_to_go == 0)
              {
                jerr = ProcessRestart();
                if(JPEG_OK != jerr)
                {
                  LOG0("Error: ProcessRestart() failed!");
                  return jerr;
                }
              }
            }

            int16_t*                lastDC = &m_ccomp[m_curr_comp_no].m_lastDC;
            IppiDecodeHuffmanSpec* dctbl = m_dctbl[m_ccomp[m_curr_comp_no].m_dc_selector];
            IppiDecodeHuffmanSpec* actbl = m_actbl[m_ccomp[m_curr_comp_no].m_ac_selector];

            m_BitStreamIn.FillBuffer(SAFE_NBYTES);

            currPos = m_BitStreamIn.GetCurrPos();

            status = mfxiDecodeHuffman8x8_JPEG_1u16s_C1(
                       src,srcLen,&currPos,block,lastDC,(int*)&m_marker,
                       dctbl,actbl,m_state);

            m_BitStreamIn.SetCurrPos(currPos);

            if(ippStsNoErr > status)
            {
              LOG0("Error: mfxiDecodeHuffman8x8_JPEG_1u16s_C1() failed!");
              return JPEG_ERR_INTERNAL;
            }

            block += DCTSIZE2;

            m_restarts_to_go --;
          } // for m_hsampling
        } // for m_scan_ncomps
      } // for m_numxMCU
    } // for m_vsampling
  } // for m_numyMCU

  return JPEG_OK;
} // CJPEGDecoder::DecodeScanBaselineNI()


JERRCODE CJPEGDecoder::DecodeScanProgressive(void)
{
  int       i, j, k, n, l, c;
  int       srcLen;
  int       currPos;
  uint8_t*    src;
  int16_t*   block;
  JERRCODE  jerr;
  int status;

  status = mfxiDecodeHuffmanStateInit_JPEG_8u(m_state);
  if(ippStsNoErr != status)
  {
    return JPEG_ERR_INTERNAL;
  }

  m_marker = JM_NONE;

  src    = m_BitStreamIn.GetDataPtr();
  srcLen = m_BitStreamIn.GetDataLen();

  if(m_ss != 0 && m_se != 0)
  {
    // AC scan
    for(i = 0; i < (int) m_numyMCU; i++)
    {
      for(k = 0; k < m_ccomp[m_curr_comp_no].m_vsampling; k++)
      {
        if(i*m_ccomp[m_curr_comp_no].m_vsampling*8 + k*8 >= m_jpeg_height)
          break;

        for(j = 0; j < (int) m_numxMCU; j++)
        {
          block = m_block_buffer + (DCTSIZE2*m_nblock*(j+(i*m_numxMCU)));

          // skip any relevant components
          for(c = 0; c < m_ccomp[m_curr_comp_no].m_comp_no; c++)
          {
            block += (DCTSIZE2*m_ccomp[c].m_nblocks);
          }

          // Skip over relevant 8x8 blocks from this component.
          block += (k * DCTSIZE2 * m_ccomp[m_curr_comp_no].m_hsampling);

          for(l = 0; l < m_ccomp[m_curr_comp_no].m_hsampling; l++)
          {
            // Ignore the last column(s) of the image.
            if(((j*m_ccomp[m_curr_comp_no].m_hsampling*8) + (l*8)) >= m_jpeg_width)
              break;

            if(m_curr_scan->jpeg_restart_interval)
            {
              if(m_restarts_to_go == 0)
              {
                jerr = ProcessRestart();
                if(JPEG_OK != jerr)
                {
                  LOG0("Error: ProcessRestart() failed!");
                  return jerr;
                }
              }
            }

            IppiDecodeHuffmanSpec* actbl = m_actbl[m_ccomp[m_curr_comp_no].m_ac_selector];

            if(m_ah == 0)
            {
              m_BitStreamIn.FillBuffer(SAFE_NBYTES);

              currPos = m_BitStreamIn.GetCurrPos();

              status = mfxiDecodeHuffman8x8_ACFirst_JPEG_1u16s_C1(
                         src,srcLen,&currPos,block,(int*)&m_marker,
                         m_ss,m_se,m_al,actbl,m_state);

              m_BitStreamIn.SetCurrPos(currPos);

              if(ippStsNoErr > status)
              {
                LOG0("Error: mfxiDecodeHuffman8x8_ACFirst_JPEG_1u16s_C1() failed!");
                return JPEG_ERR_INTERNAL;
              }
            }
            else
            {
              m_BitStreamIn.FillBuffer(SAFE_NBYTES);

              currPos = m_BitStreamIn.GetCurrPos();

              status = mfxiDecodeHuffman8x8_ACRefine_JPEG_1u16s_C1(
                         src,srcLen,&currPos,block,(int*)&m_marker,
                         m_ss,m_se,m_al,actbl,m_state);

              m_BitStreamIn.SetCurrPos(currPos);

              if(ippStsNoErr > status)
              {
                LOG0("Error: mfxiDecodeHuffman8x8_ACRefine_JPEG_1u16s_C1() failed!");
                return JPEG_ERR_INTERNAL;
              }
            }

            block += DCTSIZE2;

            m_restarts_to_go --;
          } // for m_hsampling
        } // for m_numxMCU
      } // for m_vsampling
    } // for m_numyMCU

    if(m_al == 0 && m_se == 63)
    {
      m_ccomp[m_curr_comp_no].m_ac_scan_completed = 1;
    }
  }
  else
  {
    // DC scan
    for(i = 0; i < (int) m_numyMCU; i++)
    {
      for(j = 0; j < (int) m_numxMCU; j++)
      {
        if(m_curr_scan->jpeg_restart_interval)
        {
          if(m_restarts_to_go == 0)
          {
            jerr = ProcessRestart();
            if(JPEG_OK != jerr)
            {
              LOG0("Error: ProcessRestart() failed!");
              return jerr;
            }
          }
        }

        block = m_block_buffer + (DCTSIZE2*m_nblock*(j+(i*m_numxMCU)));

        if(m_ah == 0)
        {
          // first DC scan
          for(n = 0; n < m_jpeg_ncomp; n++)
          {
            int16_t*                lastDC = &m_ccomp[n].m_lastDC;
            IppiDecodeHuffmanSpec* dctbl  = m_dctbl[m_ccomp[n].m_dc_selector];

            for(k = 0; k < m_ccomp[n].m_vsampling; k++)
            {
              for(l = 0; l < m_ccomp[n].m_hsampling; l++)
              {
                m_BitStreamIn.FillBuffer(SAFE_NBYTES);

                currPos = m_BitStreamIn.GetCurrPos();

                status = mfxiDecodeHuffman8x8_DCFirst_JPEG_1u16s_C1(
                           src,srcLen,&currPos,block,lastDC,(int*)&m_marker,
                           m_al,dctbl,m_state);

                m_BitStreamIn.SetCurrPos(currPos);

                if(ippStsNoErr > status)
                {
                  LOG0("Error: mfxiDecodeHuffman8x8_DCFirst_JPEG_1u16s_C1() failed!");
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
                m_BitStreamIn.FillBuffer(SAFE_NBYTES);

                currPos = m_BitStreamIn.GetCurrPos();

                status = mfxiDecodeHuffman8x8_DCRefine_JPEG_1u16s_C1(
                           src,srcLen,&currPos,block,(int*)&m_marker,
                           m_al,m_state);

                m_BitStreamIn.SetCurrPos(currPos);

                if(ippStsNoErr > status)
                {
                  LOG0("Error: mfxiDecodeHuffman8x8_DCRefine_JPEG_1u16s_C1() failed!");
                  return JPEG_ERR_INTERNAL;
                }

                block += DCTSIZE2;
              } // for m_hsampling
            } // for m_vsampling
          } // for m_jpeg_ncomp
        }
        m_restarts_to_go --;
      } // for m_numxMCU
    } // for m_numyMCU

    if(m_al == 0)
    {
      m_dc_scan_completed = 1;
    }
  }

  return JPEG_OK;
} // CJPEGDecoder::DecodeScanProgressive()


JERRCODE CJPEGDecoder::DecodeScanLosslessIN(void)
{
  int       i;
  int16_t*   pMCUBuf;
  JERRCODE  jerr;
  int status;

  status = mfxiDecodeHuffmanStateInit_JPEG_8u(m_state);
  if(ippStsNoErr != status)
  {
    return JPEG_ERR_INTERNAL;
  }

  m_marker = JM_NONE;

  m_ac_scans_completed += m_curr_scan->ncomps;

  pMCUBuf = m_block_buffer;

  for(i = 0; i < (int) m_numyMCU; i++)
  {
    jerr = DecodeHuffmanMCURowLS(pMCUBuf);
    if(JPEG_OK != jerr)
    {
      return jerr;
    }

    jerr = ReconstructMCURowLS(pMCUBuf, i);
    if(JPEG_OK != jerr)
    {
      return jerr;
    }

    if(m_curr_scan->ncomps == m_jpeg_ncomp)
    {
      jerr = ColorConvert(i, 0, m_numxMCU);
      if(JPEG_OK != jerr)
        return jerr;
    }
  } // for m_numyMCU

  return JPEG_OK;
} // CJPEGDecoder::DecodeScanLosslessIN()


JERRCODE CJPEGDecoder::DecodeScanLosslessNI(void)
{
  int       i, j, n, v, h;
  uint8_t*    src;
  int       srcLen;
  int       currPos;
  int16_t*   ptr;
  int16_t*   pMCUBuf;
  JERRCODE  jerr;
  int status;

  status = mfxiDecodeHuffmanStateInit_JPEG_8u(m_state);
  if(ippStsNoErr != status)
  {
    return JPEG_ERR_INTERNAL;
  }

  m_marker = JM_NONE;

  m_ac_scans_completed += m_curr_scan->ncomps;

  pMCUBuf = m_block_buffer + m_curr_comp_no*m_numxMCU*m_numyMCU;

  src    = m_BitStreamIn.GetDataPtr();
  srcLen = m_BitStreamIn.GetDataLen();

  for(i = 0; i < (int) m_numyMCU; i++)
  {
    for(j = 0; j < (int) m_numxMCU; j++)
    {
      if(m_curr_scan->jpeg_restart_interval)
      {
        if(m_restarts_to_go == 0)
        {
          jerr = ProcessRestart();
          if(JPEG_OK != jerr)
          {
            LOG0("Error: ProcessRestart() failed!");
            return jerr;
          }
        }
      }

      for(n = 0; n < m_curr_scan->ncomps; n++)
      {
        CJPEGColorComponent*   curr_comp = &m_ccomp[m_curr_comp_no];
        IppiDecodeHuffmanSpec* dctbl = m_dctbl[curr_comp->m_dc_selector];

        ptr = pMCUBuf + j*m_mcuWidth;

        for(v = 0; v < curr_comp->m_vsampling; v++)
        {
          for(h = 0; h < curr_comp->m_hsampling; h++)
          {
            m_BitStreamIn.FillBuffer(SAFE_NBYTES);

            currPos = m_BitStreamIn.GetCurrPos();

            status = mfxiDecodeHuffmanOne_JPEG_1u16s_C1(
              src,srcLen,&currPos,ptr,(int*)&m_marker,
              dctbl,m_state);

            m_BitStreamIn.SetCurrPos(currPos);

            if(ippStsNoErr > status)
            {
              LOG0("Error: mfxiDecodeHuffmanOne_JPEG_1u16s_C1() failed!");
              return JPEG_ERR_INTERNAL;
            }

            ptr++;
          } // for m_hsampling
        } // for m_vsampling
      } // for m_jpeg_ncomp

      m_restarts_to_go --;
    } // for m_numxMCU

    pMCUBuf += m_numxMCU;
  } // for m_numyMCU

  return JPEG_OK;
} // CJPEGDecoder::DecodeScanLosslessNI()


JERRCODE CJPEGDecoder::ReadHeader(
  int*    width,
  int*    height,
  int*    nchannels,
  JCOLOR* color,
  JSS*    sampling,
  int*    precision)
{
  int      du_width;
  int      du_height;
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

  // DU block dimensions (8x8 for DCT based modes and 1x1 for lossless mode)
  du_width  = (JPEG_LOSSLESS == m_jpeg_mode) ? 1 : 8;
  du_height = (JPEG_LOSSLESS == m_jpeg_mode) ? 1 : 8;

  // MCU dimensions
  m_mcuWidth  = du_width  * std::max(m_max_hsampling,1);
  m_mcuHeight = du_height * std::max(m_max_vsampling,1);

  // num of MCUs in whole image
  m_numxMCU = (m_jpeg_width  + (m_mcuWidth  - 1)) / m_mcuWidth;
  m_numyMCU = (m_jpeg_height + (m_mcuHeight - 1)) / m_mcuHeight;

  // not completed MCUs should be padded
  m_xPadding = m_numxMCU * m_mcuWidth  - m_jpeg_width;
  m_yPadding = m_numyMCU * m_mcuHeight - m_jpeg_height;

  // dimensions of internal buffer for color conversion
  m_ccWidth  = m_mcuWidth * m_numxMCU;
  m_ccHeight = m_mcuHeight;

  *width     = m_jpeg_width;
  *height    = m_jpeg_height;
  *nchannels = m_jpeg_ncomp;
  *color     = m_jpeg_color;
  *sampling  = m_jpeg_sampling;
  *precision = m_jpeg_precision;

  return JPEG_OK;
} // CJPEGDecoder::ReadHeader()

JERRCODE CJPEGDecoder::ReadData(void)
{
    return ParseJPEGBitStream(JO_READ_DATA);

} // CJPEGDecoder::ReadData(void)

JERRCODE CJPEGDecoder::ReadData(uint32_t restartNum, uint32_t restartsToDecode)
{
    JERRCODE jerr = JPEG_OK;

    m_marker = JM_NONE;

    // find the start of VLC unit
    jerr = NextMarker(&m_marker);
    if (JPEG_OK != jerr)
    {
        return jerr;
    }

    // parse the VLC unit's header
    switch (m_marker)
    {
    case JM_SOS:
        jerr = ParseSOS(JO_READ_DATA);
        if (JPEG_OK != jerr)
        {
            return jerr;
        }
        break;

    case JM_RST0:
    case JM_RST1:
    case JM_RST2:
    case JM_RST3:
    case JM_RST4:
    case JM_RST5:
    case JM_RST6:
    case JM_RST7:
        jerr = ParseRST();
        if (JPEG_OK != jerr)
        {
            return jerr;
        }

        // reset DC predictors
        if (m_jpeg_ncomp < 0 || m_jpeg_ncomp > MAX_COMPS_PER_SCAN)
        {
            return JPEG_ERR_SOF_DATA;
        }
        for (int n = 0; n < m_jpeg_ncomp; n++)
        {
            m_ccomp[n].m_lastDC = 0;
        }

        break;

    default:
        return JPEG_ERR_SOS_DATA;
        break;
    }

    // set the number of MCU to process
    if(0 == m_curr_scan->scan_no && 
        0 != m_scans[0].jpeg_restart_interval)
    {
        m_mcu_decoded = restartNum * m_scans[0].jpeg_restart_interval;
    }
    else if(1 == m_curr_scan->scan_no && 
        0 != m_scans[0].jpeg_restart_interval && 
        0 != m_scans[1].jpeg_restart_interval)
    {
        m_mcu_decoded = (restartNum - (m_scans[0].numxMCU * m_scans[0].numyMCU + m_scans[0].jpeg_restart_interval - 1) / m_scans[0].jpeg_restart_interval) * 
            m_scans[1].jpeg_restart_interval;
    }
    else if(2 == m_curr_scan->scan_no && 
        0 != m_scans[0].jpeg_restart_interval && 
        0 != m_scans[1].jpeg_restart_interval && 
        0 != m_scans[2].jpeg_restart_interval)
    {
        m_mcu_decoded = (restartNum - (m_scans[0].numxMCU * m_scans[0].numyMCU + m_scans[0].jpeg_restart_interval - 1) / m_scans[0].jpeg_restart_interval - 
            (m_scans[1].numxMCU * m_scans[1].numyMCU + m_scans[1].jpeg_restart_interval - 1) / m_scans[1].jpeg_restart_interval) * m_scans[2].jpeg_restart_interval;
    }
    else
    {
        m_mcu_decoded = 0;
    }

    m_mcu_to_decode = (m_curr_scan->jpeg_restart_interval) ?
                      (m_curr_scan->jpeg_restart_interval * restartsToDecode) :
                      (m_curr_scan->numxMCU * m_curr_scan->numyMCU);

    m_restarts_to_go = m_curr_scan->jpeg_restart_interval;

    // decode VLC unit data
    jerr = ParseData();

    return jerr;

} // CJPEGDecoder::ReadData(uint32_t restartNum)

JERRCODE CJPEGDecoder::ReadPictureHeaders(void)
{
    return JPEG_OK;
}

ChromaType CJPEGDecoder::GetChromaType()
{
/*
 0:YUV400 (grayscale image)
    1:YUV420        Y: h=2 v=2, Cb/Cr: h=1 v=1
 2:YUV422H_2Y       Y: h=2 v=1, Cb/Cr: h=1 v=1
    3:YUV444        Y: h=1 v=1, Cb/Cr: h=1 v=1
    4:YUV411        Y: h=4 v=1, Cb/Cr: h=1 v=1
 5:YUV422V_2Y       Y: h=1 v=2, Cb/Cr: h=1 v=1
 6:YUV422H_4Y       Y: h=2 v=1, Cb/Cr: h=1 v=2
 7:YUV422V_4Y       Y: h=1 v=2, Cb/Cr: h=2 v=1
*/

    if (m_ccomp[0].m_hsampling == 4)
    {
        assert(m_ccomp[0].m_vsampling == 1);
        return CHROMA_TYPE_YUV411; // YUV411
    }

    if (m_ccomp[0].m_hsampling == 1) // YUV422V_4Y, YUV422V_2Y, YUV444
    {
        if (m_ccomp[0].m_vsampling == 1) // YUV444
        {
            return CHROMA_TYPE_YUV444;
        }
        else
        {
            assert(m_ccomp[0].m_vsampling == 2);
            return (m_ccomp[1].m_hsampling == 1) ? CHROMA_TYPE_YUV422V_2Y : CHROMA_TYPE_YUV422V_4Y; // YUV422V_2Y, YUV422V_4Y
        }
    }

    if (m_ccomp[0].m_hsampling == 2) // YUV420, YUV422H_2Y, YUV422H_4Y
    {
        if (m_ccomp[0].m_vsampling == 1)
        {
            return (m_ccomp[1].m_vsampling == 1) ? CHROMA_TYPE_YUV422H_2Y : CHROMA_TYPE_YUV422H_4Y; // YUV422H_2Y, YUV422H_4Y
        }
        else
        {
            assert(m_ccomp[0].m_vsampling == 2);
            return CHROMA_TYPE_YUV420; // YUV420;
        }
    }

    return CHROMA_TYPE_YUV400;
}

void CopyPlane4To2(uint8_t  *pDstUV,
                   int32_t iDstStride,
                   const uint8_t  *pSrcU,
                   const uint8_t  *pSrcV,
                   int32_t iSrcStride,
                   mfxSize size)
{
    int32_t y, x;

    for (y = 0; y < size.height / 2; y += 1)
    {
        for (x = 0; x < size.width / 2; x += 1)
        {
            pDstUV[2*x] = pSrcU[x * 2];
            pDstUV[2*x + 1] = pSrcV[x * 2];
        }

        pSrcU += 2*iSrcStride;
        pSrcV += 2*iSrcStride;
        pDstUV += iDstStride;
    }

} // void CopyPlane(uint8_t *pDst,

void DownSamplingPlane(uint8_t  *pSrcU, uint8_t  *pSrcV, int32_t iSrcStride, mfxSize size)
{
    int32_t y, x;

    for (y = 0; y < size.height / 2; y += 1)
    {
        for (x = 0; x < size.width / 2; x += 1)
        {
            pSrcU[x] = pSrcU[2*x];
            pSrcV[x] = pSrcV[2*x];
        }

        pSrcU += iSrcStride;
        pSrcV += iSrcStride;
    }

} // void CopyPlane(uint8_t *pDst,

void CopyPlane(uint8_t  *pDst,
               int32_t iDstStride,
               const uint8_t  *pSrc,
               int32_t iSrcStride,
               mfxSize size)
{
    mfxiCopy_8u_C1R(pSrc,
                    iSrcStride,
                    pDst,
                    iDstStride,
                    size);

} // void CopyPlane(uint8_t *pDst,

void ConvertFrom_YUV444_To_YV12(const uint8_t *src[3], uint32_t srcPitch, uint8_t * dst[2], uint32_t dstPitch, mfxSize size)
{
    CopyPlane(dst[0],
              dstPitch,
              src[0],
              srcPitch,
              size);

    CopyPlane4To2(dst[1],
                  dstPitch,
                  src[1],
                  src[2],
                  srcPitch,
                  size);
}

void ConvertFrom_YUV422V_To_YV12(uint8_t *src[3], uint32_t srcPitch, uint8_t * [2], uint32_t , mfxSize size)
{
    DownSamplingPlane(src[1], src[2], srcPitch, size);
}

void ConvertFrom_YUV422H_4Y_To_NV12(const uint8_t *src[3], uint32_t srcPitch, uint8_t * dst[2], uint32_t dstPitch, mfxSize size)
{
    CopyPlane(dst[0],
              dstPitch,
              src[0],
              srcPitch,
              size);

    CopyPlane4To2(dst[1],
                  dstPitch,
                  src[1],
                  src[2],
                  srcPitch,
                  size);
}

void ConvertFrom_YUV422V_4Y_To_NV12(const uint8_t *src[3], uint32_t srcPitch, uint8_t * dst[2], uint32_t dstPitch, mfxSize size)
{
    CopyPlane(dst[0],
              dstPitch,
              src[0],
              srcPitch,
              size);

    CopyPlane4To2(dst[1],
                  dstPitch,
                  src[1],
                  src[2],
                  srcPitch,
                  size);
}


#endif // MFX_ENABLE_MJPEG_VIDEO_DECODE && MFX_ENABLE_SW_FALLBACK
