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

#if defined (MFX_ENABLE_MJPEG_VIDEO_ENCODE)
#if defined(__GNUC__)
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include "ippi.h"
#include "encqtbl.h"


CJPEGEncoderQuantTable::CJPEGEncoderQuantTable(void)
{
  m_id          = 0;
  m_precision   = 0;
  m_initialized = false;

  // align for max performance
  m_raw8u  = UMC::align_pointer<uint8_t *>(m_rbf,CPU_CACHE_LINE);
  m_raw16u = UMC::align_pointer<uint16_t *>(m_rbf,CPU_CACHE_LINE);
  m_qnt16u = UMC::align_pointer<uint16_t *>(m_qbf,CPU_CACHE_LINE);
  m_qnt32f = UMC::align_pointer<float *>(m_qbf,CPU_CACHE_LINE);

  memset(m_rbf, 0, sizeof(m_rbf));
  memset(m_qbf, 0, sizeof(m_qbf));

  return;
} // ctor


CJPEGEncoderQuantTable::~CJPEGEncoderQuantTable(void)
{
  m_id          = 0;
  m_precision   = 0;
  m_initialized = false;

  memset(m_rbf, 0, sizeof(m_rbf));
  memset(m_qbf, 0, sizeof(m_qbf));

  return;
} // dtor


JERRCODE CJPEGEncoderQuantTable::Init(int id,uint8_t raw[64],int quality)
{
  int status;

  m_id        = id;
  m_precision = 0; // 8-bit precision

  MFX_INTERNAL_CPY(m_raw8u,raw,DCTSIZE2);

  // scale according quality parameter
  if(quality)
  {
    status = mfxiQuantFwdRawTableInit_JPEG_8u(m_raw8u,quality);
    if(ippStsNoErr != status)
    {
      LOG1("IPP Error: ippiQuantFwdRawTableInit_JPEG_8u() failed - ",status);
      return JPEG_ERR_INTERNAL;
    }
  }

  status = mfxiQuantFwdTableInit_JPEG_8u16u(m_raw8u,m_qnt16u);
  if(ippStsNoErr != status)
  {
    LOG1("IPP Error: ippiQuantFwdTableInit_JPEG_8u() failed - ",status);
    return JPEG_ERR_INTERNAL;
  }

  m_initialized = true;

  return JPEG_OK;
} // CJPEGEncoderQuantTable::Init()


static
int ippiQuantFwdRawTableInit_JPEG_16u(
  uint16_t* raw,
  int     quality)
{
  int i;
  int val;

  if(quality > 100)
    quality = 100;

  if(quality < 50)
    quality = 5000 / quality;
  else
    quality = 200 - (quality * 2);

  for(i = 0; i < DCTSIZE2; i++)
  {
    val = (raw[i] * quality + 50) / 100;
    if(val < 1)
    {
      raw[i] = (uint16_t)1;
    }
    else if(val > 65535)
    {
      raw[i] = (uint16_t)65535;
    }
    else
    {
      raw[i] = (uint16_t)val;
    }
  }

  return ippStsNoErr;
} // ippiQuantFwdRawTableInit_JPEG_16u()


static
int ippiQuantFwdTableInit_JPEG_16u(
  uint16_t* raw,
  float* qnt)
{
  int       i;
  uint16_t    wb[DCTSIZE2];
  int status;

  status = mfxiZigzagInv8x8_16s_C1((const int16_t*)&raw[0],(int16_t*)&wb[0]);
  if(ippStsNoErr != status)
  {
    LOG1("IPP Error: ippiZigzagInv8x8_16s_C1() failed - ",status);
    return status;
  }

  for(i = 0; i < DCTSIZE2; i++)
  {
    qnt[i] = (float)(1.0 / wb[i]);
  }

  return ippStsNoErr;
} // ippiQuantFwdTableInit_JPEG_16u()


JERRCODE CJPEGEncoderQuantTable::Init(int id,uint16_t raw[64],int quality)
{
  int status;

  m_id        = id;
  m_precision = 1; // 16-bit precision

  MFX_INTERNAL_CPY((uint8_t*)m_raw16u,(uint8_t*)raw,DCTSIZE2*sizeof(uint16_t));

  if(quality)
  {
    status = ippiQuantFwdRawTableInit_JPEG_16u(m_raw16u,quality);
    if(ippStsNoErr != status)
    {
      LOG1("IPP Error: ippiQuantFwdRawTableInit_JPEG_16u() failed - ",status);
      return JPEG_ERR_INTERNAL;
    }
  }

  status = ippiQuantFwdTableInit_JPEG_16u(m_raw16u,m_qnt32f);

  if(ippStsNoErr != status)
  {
    LOG1("IPP Error: ippiQuantFwdTableInit_JPEG_16u() failed - ",status);
    return JPEG_ERR_INTERNAL;
  }

  m_initialized = true;

  return JPEG_OK;
} // CJPEGEncoderQuantTable::Init()

#endif // MFX_ENABLE_MJPEG_VIDEO_ENCODE
