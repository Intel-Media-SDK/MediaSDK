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

#include "umc_defs.h"
#include "umc_structures.h"

#if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE)

#include "decqtbl.h"

CJPEGDecoderQuantTable::CJPEGDecoderQuantTable(void)
{
  m_id          = 0;
  m_precision   = 0;
  m_initialized = 0;

  // align for max performance
  m_raw8u  = UMC::align_pointer<uint8_t *>(m_rbf, CPU_CACHE_LINE);
  m_raw16u = UMC::align_pointer<uint16_t *>(m_rbf,CPU_CACHE_LINE);
  memset(m_rbf, 0, sizeof(m_rbf));

#ifdef MFX_ENABLE_SW_FALLBACK
  m_qnt16u = UMC::align_pointer<uint16_t *>(m_qbf,CPU_CACHE_LINE);
  m_qnt32f = UMC::align_pointer<float *>(m_qbf,CPU_CACHE_LINE);

  memset(m_qbf, 0, sizeof(m_qbf));
#endif
  return;
} // ctor


CJPEGDecoderQuantTable::~CJPEGDecoderQuantTable(void)
{
  m_id          = 0;
  m_precision   = 0;
  m_initialized = 0;

  memset(m_rbf, 0, sizeof(m_rbf));
#ifdef MFX_ENABLE_SW_FALLBACK
  memset(m_qbf, 0, sizeof(m_qbf));
#endif
  return;
} // dtor


JERRCODE CJPEGDecoderQuantTable::Init(int id,uint8_t raw[64])
{
  m_id        = id & 0x0f;
  m_precision = 0; // 8-bit precision

  MFX_INTERNAL_CPY(m_raw8u,raw,DCTSIZE2);

#ifdef MFX_ENABLE_SW_FALLBACK
  int status = mfxiQuantInvTableInit_JPEG_8u16u(m_raw8u,m_qnt16u);
  if(ippStsNoErr != status)
  {
    LOG1("IPP Error: mfxiQuantInvTableInit_JPEG_8u16u() failed - ",status);
    return JPEG_ERR_INTERNAL;
  }
#endif
  m_initialized = 1;

  return JPEG_OK;
} // CJPEGDecoderQuantTable::Init()

#ifdef MFX_ENABLE_SW_FALLBACK
static
int mfxiQuantInvTableInit_JPEG_16u32f(
  uint16_t* raw,
  float* qnt)
{
  uint16_t    wb[DCTSIZE2];
  int status;

  status = mfxiZigzagInv8x8_16s_C1((int16_t*)raw,(int16_t*)wb);
  if(ippStsNoErr != status)
  {
    return status;
  }

  for(int i = 0; i < DCTSIZE2; i++)
    ((float*)qnt)[i] = (float)((uint16_t*)wb)[i];

  return ippStsNoErr;
} // mfxiQuantInvTableInit_JPEG_16u32f()
#endif

JERRCODE CJPEGDecoderQuantTable::Init(int id,uint16_t raw[64])
{
  m_id        = id & 0x0f;
  m_precision = 1; // 16-bit precision

  MFX_INTERNAL_CPY((int16_t*)m_raw16u, (int16_t*)raw, DCTSIZE2*sizeof(int16_t));
#ifdef MFX_ENABLE_SW_FALLBACK
  int status = mfxiQuantInvTableInit_JPEG_16u32f(m_raw16u,m_qnt32f);
  if(ippStsNoErr != status)
  {
    LOG1("IPP Error: mfxiQuantInvTableInit_JPEG_16u32f() failed - ",status);
    return JPEG_ERR_INTERNAL;
  }
#endif

  m_initialized = 1;

  return JPEG_OK;
} // CJPEGDecoderQuantTable::Init()

#ifdef MFX_ENABLE_SW_FALLBACK
JERRCODE CJPEGDecoderQuantTable::ConvertToLowPrecision(void)
{
  int status;

  status = mfxiZigzagInv8x8_16s_C1((int16_t*)m_raw16u,(int16_t*)m_qnt16u);
  if(ippStsNoErr != status)
  {
    return JPEG_ERR_INTERNAL;
  }

  m_precision   = 0; // 8-bit precision
  m_initialized = 1;

  return JPEG_OK;
} // CJPEGDecoderQuantTable::ConvertToLowPrecision()


JERRCODE CJPEGDecoderQuantTable::ConvertToHighPrecision(void)
{
  int       step;
  mfxSize  roi = { DCTSIZE, DCTSIZE };
  uint16_t    wb[DCTSIZE2];
  int status;

  step = DCTSIZE * sizeof(int16_t);

  status = mfxiConvert_8u16u_C1R(m_raw8u,DCTSIZE*sizeof(uint8_t),wb,step,roi);
  if(ippStsNoErr != status)
  {
    return JPEG_ERR_INTERNAL;
  }

  status = mfxiCopy_16s_C1R((int16_t*)wb,step,(int16_t*)m_raw16u,step,roi);
  if(ippStsNoErr != status)
  {
    return JPEG_ERR_INTERNAL;
  }

  status = mfxiQuantInvTableInit_JPEG_16u32f(m_raw16u,m_qnt32f);
  if(ippStsNoErr != status)
  {
    return JPEG_ERR_INTERNAL;
  }

  m_precision   = 1; // 16-bit precision
  m_initialized = 1;

  return JPEG_OK;
} // CJPEGDecoderQuantTable::ConvertToHighPrecision()
#endif // MFX_ENABLE_SW_FALLBACK

#endif // MFX_ENABLE_MJPEG_VIDEO_DECODE
