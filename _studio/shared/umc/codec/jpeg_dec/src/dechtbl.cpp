// Copyright (c) 2017-2018 Intel Corporation
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

#include "jpegbase.h"
#include "dechtbl.h"
#include <cstdlib>

CJPEGDecoderHuffmanTable::CJPEGDecoderHuffmanTable(void)
{
  m_id     = 0;
  m_hclass = 0;
#ifdef MFX_ENABLE_SW_FALLBACK
  m_table  = 0;
#endif
  m_bEmpty = 1;
  m_bValid = 0;

  memset(m_bits, 0, sizeof(m_bits));
  memset(m_vals, 0, sizeof(m_vals));

  return;
} // ctor


CJPEGDecoderHuffmanTable::~CJPEGDecoderHuffmanTable(void)
{
  Destroy();
  return;
} // dtor


JERRCODE CJPEGDecoderHuffmanTable::Create(void)
{
#ifdef MFX_ENABLE_SW_FALLBACK
  int       size;
  int status;

  status = mfxiDecodeHuffmanSpecGetBufSize_JPEG_8u(&size);
  if(ippStsNoErr != status)
  {
    LOG1("IPP Error: mfxiDecodeHuffmanSpecGetBufSize_JPEG_8u() failed - ",status);
    return JPEG_ERR_INTERNAL;
  }

  if(0 != m_table)
  {
    free(m_table);
    m_table = 0;
  }

  m_table = (IppiDecodeHuffmanSpec*)malloc(size);
  if(0 == m_table)
  {
    LOG0("IPP Error: malloc() failed");
    return JPEG_ERR_ALLOC;
  }
#endif
  m_bEmpty = 0;
  m_bValid = 0;

  return JPEG_OK;
} // CJPEGDecoderHuffmanTable::Create()


JERRCODE CJPEGDecoderHuffmanTable::Destroy(void)
{
  m_id     = 0;
  m_hclass = 0;

  memset(m_bits, 0, sizeof(m_bits));
  memset(m_vals, 0, sizeof(m_vals));

#ifdef MFX_ENABLE_SW_FALLBACK
  if(0 != m_table)
  {
    free(m_table);
    m_table = 0;
  }
#endif
  m_bValid = 0;
  m_bEmpty = 1;

  return JPEG_OK;
} // CJPEGDecoderHuffmanTable::Destroy()


JERRCODE CJPEGDecoderHuffmanTable::Init(int id,int hclass,uint8_t* bits,uint8_t* vals)
{
  m_id     = id     & 0x0f;
  m_hclass = hclass & 0x0f;

  MFX_INTERNAL_CPY(m_bits,bits,16);
  MFX_INTERNAL_CPY(m_vals,vals,256);

#ifdef MFX_ENABLE_SW_FALLBACK
  int status = mfxiDecodeHuffmanSpecInit_JPEG_8u(m_bits,m_vals,m_table);
  if(ippStsNoErr != status)
  {
    LOG1("IPP Error: mfxiDecodeHuffmanSpecInit_JPEG_8u() failed - ",status);
    return JPEG_ERR_DHT_DATA;
  }
#endif
  m_bValid = 1;
  m_bEmpty = 0;

  return JPEG_OK;
} // CJPEGDecoderHuffmanTable::Init()



#ifdef MFX_ENABLE_SW_FALLBACK
CJPEGDecoderHuffmanState::CJPEGDecoderHuffmanState(void)
{
  m_state = 0;
  return;
} // ctor


CJPEGDecoderHuffmanState::~CJPEGDecoderHuffmanState(void)
{
  Destroy();
  return;
} // dtor


JERRCODE CJPEGDecoderHuffmanState::Create(void)
{
  int       size;
  int status;

  status = mfxiDecodeHuffmanStateGetBufSize_JPEG_8u(&size);
  if(ippStsNoErr != status)
  {
    LOG1("IPP Error: mfxiDecodeHuffmanStateGetBufSize_JPEG_8u() failed - ",status);
    return JPEG_ERR_INTERNAL;
  }

  if(0 != m_state)
  {
    free(m_state);
    m_state = 0;
  }

  m_state = (IppiDecodeHuffmanState*)malloc(size);
  if(0 == m_state)
  {
    LOG0("IPP Error: malloc() failed");
    return JPEG_ERR_ALLOC;
  }

  return JPEG_OK;
} // CJPEGDecoderHuffmanState::Create()


JERRCODE CJPEGDecoderHuffmanState::Destroy(void)
{
  if(0 != m_state)
  {
    free(m_state);
    m_state = 0;
  }

  return JPEG_OK;
} // CJPEGDecoderHuffmanState::Destroy()


JERRCODE CJPEGDecoderHuffmanState::Init(void)
{
  int status;

  status = mfxiDecodeHuffmanStateInit_JPEG_8u(m_state);
  if(ippStsNoErr != status)
  {
    LOG1("IPP Error: mfxiDecodeHuffmanStateInit_JPEG_8u() failed - ",status);
    return JPEG_ERR_INTERNAL;
  }

  return JPEG_OK;
} // CJPEGDecoderHuffmanState::Init()
#endif // #ifdef MFX_ENABLE_SW_FALLBACK

#endif // MFX_ENABLE_MJPEG_VIDEO_DECODE
