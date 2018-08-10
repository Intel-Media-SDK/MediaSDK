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
#include "umc_structures.h"

#if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE)
#if defined(__GNUC__)
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

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

  return;
} // ctor


CJPEGDecoderQuantTable::~CJPEGDecoderQuantTable(void)
{
  m_id          = 0;
  m_precision   = 0;
  m_initialized = 0;

  memset(m_rbf, 0, sizeof(m_rbf));
  return;
} // dtor


JERRCODE CJPEGDecoderQuantTable::Init(int id,uint8_t raw[64])
{
  m_id        = id & 0x0f;
  m_precision = 0; // 8-bit precision

  MFX_INTERNAL_CPY(m_raw8u,raw,DCTSIZE2);

  m_initialized = 1;

  return JPEG_OK;
} // CJPEGDecoderQuantTable::Init()


JERRCODE CJPEGDecoderQuantTable::Init(int id,uint16_t raw[64])
{
  m_id        = id & 0x0f;
  m_precision = 1; // 16-bit precision

  MFX_INTERNAL_CPY((int16_t*)m_raw16u, (int16_t*)raw, DCTSIZE2*sizeof(int16_t));

  m_initialized = 1;

  return JPEG_OK;
} // CJPEGDecoderQuantTable::Init()


#endif // MFX_ENABLE_MJPEG_VIDEO_DECODE
