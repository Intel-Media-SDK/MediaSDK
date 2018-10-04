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
#if defined (MFX_ENABLE_MJPEG_VIDEO_ENCODE)

#include "ippcore.h"
#include "jpegbase.h"
#include "enchtbl.h"


CJPEGEncoderHuffmanTable::CJPEGEncoderHuffmanTable(void)
{
  m_id     = 0;
  m_hclass = 0;
  m_table  = 0;
  m_bValid = false;

  memset(m_bits, 0, sizeof(m_bits));
  memset(m_vals, 0, sizeof(m_vals));

  return;
} // ctor


CJPEGEncoderHuffmanTable::~CJPEGEncoderHuffmanTable(void)
{
  Destroy();
  return;
} // dtor


JERRCODE CJPEGEncoderHuffmanTable::Create(void)
{
  int       size;
  int status;

  m_bValid = false;

  status = mfxiEncodeHuffmanSpecGetBufSize_JPEG_8u(&size);
  if(ippStsNoErr != status)
  {
    LOG1("IPP Error: ippiEncodeHuffmanSpecGetBufSize_JPEG_8u() failed - ",status);
    return JPEG_ERR_INTERNAL;
  }

  if(0 != m_table)
  {
    mfxFree(m_table);
    m_table = 0;
  }

  m_table = (IppiEncodeHuffmanSpec*)mfxMalloc(size);
  if(0 == m_table)
  {
    LOG0("IPP Error: mfx_Malloc() failed");
    return JPEG_ERR_ALLOC;
  }

  return JPEG_OK;
} // CJPEGEncoderHuffmanTable::Create()


JERRCODE CJPEGEncoderHuffmanTable::Destroy(void)
{
  m_id     = 0;
  m_hclass = 0;
  m_bValid = false;

  memset(m_bits, 0, sizeof(m_bits));
  memset(m_vals, 0, sizeof(m_vals));

  if(0 != m_table)
  {
    mfxFree(m_table);
    m_table = 0;
  }

  return JPEG_OK;
} // CJPEGEncoderHuffmanTable::Destroy()


JERRCODE CJPEGEncoderHuffmanTable::Init(int id,int hclass,uint8_t* bits,uint8_t* vals)
{
  int status;

  m_id     = id     & 0x0f;
  m_hclass = hclass & 0x0f;

  MFX_INTERNAL_CPY(m_bits,bits,16);
  MFX_INTERNAL_CPY(m_vals,vals,256);

  status = mfxiEncodeHuffmanSpecInit_JPEG_8u(m_bits,m_vals,m_table);
  if(ippStsNoErr != status)
  {
    LOG1("IPP Error: ippiEncodeHuffmanSpecInit_JPEG_8u() failes - ", status);
    return JPEG_ERR_INTERNAL;
  }

  m_bValid = true;

  return JPEG_OK;
} // CJPEGEncoderHuffmanTable::Init()




CJPEGEncoderHuffmanState::CJPEGEncoderHuffmanState(void)
{
  m_state = 0;
  return;
} // ctor


CJPEGEncoderHuffmanState::~CJPEGEncoderHuffmanState(void)
{
  Destroy();
  return;
} // dtor


JERRCODE CJPEGEncoderHuffmanState::Create(void)
{
  int       size;
  int status;

  status = mfxiEncodeHuffmanStateGetBufSize_JPEG_8u(&size);
  if(ippStsNoErr != status)
  {
    LOG1("IPP Error: mfxiEncodeHuffmanStateGetBufSize_JPEG_8u() failed - ",status);
    return JPEG_ERR_INTERNAL;
  }

  if(0 != m_state)
  {
    mfxFree(m_state);
    m_state = 0;
  }

  m_state = (IppiEncodeHuffmanState*)mfxMalloc(size);
  if(0 == m_state)
  {
    LOG0("IPP Error: mfx_Malloc() failed");
    return JPEG_ERR_ALLOC;
  }

  return JPEG_OK;
} // CJPEGEncoderHuffmanState::Create()


JERRCODE CJPEGEncoderHuffmanState::Destroy(void)
{
  if(0 != m_state)
  {
    mfxFree(m_state);
    m_state = 0;
  }

  return JPEG_OK;
} // CJPEGEncoderHuffmanState::Destroy()


JERRCODE CJPEGEncoderHuffmanState::Init(void)
{
  int status;

  status = mfxiEncodeHuffmanStateInit_JPEG_8u(m_state);
  if(ippStsNoErr != status)
  {
    LOG1("IPP Error: mfxiEncodeHuffmanStateInit_JPEG_8u() failed - ",status);
    return JPEG_ERR_INTERNAL;
  }

  return JPEG_OK;
} // CJPEGEncoderHuffmanState::Init()

#endif // MFX_ENABLE_MJPEG_VIDEO_ENCODE
