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
#if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE)

#include "jpegbase.h"
#include "dechtbl.h"
#include <cstdlib>

CJPEGDecoderHuffmanTable::CJPEGDecoderHuffmanTable(void)
{
  m_id     = 0;
  m_hclass = 0;
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

  m_bValid = 0;
  m_bEmpty = 1;

  return JPEG_OK;
} // CJPEGDecoderHuffmanTable::Destroy()


JERRCODE CJPEGDecoderHuffmanTable::Init(int id,int hclass,uint8_t* bits,uint8_t* vals)
{
  int status;

  m_id     = id     & 0x0f;
  m_hclass = hclass & 0x0f;

  MFX_INTERNAL_CPY(m_bits,bits,16);
  MFX_INTERNAL_CPY(m_vals,vals,256);

  m_bValid = 1;
  m_bEmpty = 0;

  return JPEG_OK;
} // CJPEGDecoderHuffmanTable::Init()




#endif // MFX_ENABLE_MJPEG_VIDEO_DECODE
