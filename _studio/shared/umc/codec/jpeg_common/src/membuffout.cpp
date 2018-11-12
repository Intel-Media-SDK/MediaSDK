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
#if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE) || defined (MFX_ENABLE_MJPEG_VIDEO_ENCODE)
#ifndef __MEMBUFFOUT_H__
#include "membuffout.h"
#endif


CMemBuffOutput::CMemBuffOutput(void)
{
  m_buf     = 0;
  m_buflen  = 0;
  m_currpos = 0;

  return;
} // ctor


CMemBuffOutput::~CMemBuffOutput(void)
{
  Close();
  return;
} // dtor


JERRCODE CMemBuffOutput::Open(uint8_t* pBuf, int buflen)
{
  if(0 == pBuf)
    return JPEG_ERR_PARAMS;

  m_buf     = pBuf;
  m_buflen  = buflen;
  m_currpos = 0;

  return JPEG_OK;
} // CMemBuffOutput::Open()


JERRCODE CMemBuffOutput::Close(void)
{
  m_buf     = 0;
  m_buflen  = 0;
  m_currpos = 0;

  return JPEG_OK;
} // CMemBuffOutput::Close()


JERRCODE CMemBuffOutput::Write(void* buf,uic_size_t len,uic_size_t* cnt)
{
  uic_size_t wb = std::min<uic_size_t>(len, m_buflen - m_currpos);

  MFX_INTERNAL_CPY(m_buf + m_currpos,(uint8_t*)buf,wb);

  m_currpos += wb;

  *cnt = wb;

  if(len != wb)
    return JPEG_ERR_BUFF;

  return JPEG_OK;
} // CMemBuffOutput::Write()

#endif // MFX_ENABLE_MJPEG_VIDEO_DECODE || MFX_ENABLE_MJPEG_VIDEO_ENCODE
