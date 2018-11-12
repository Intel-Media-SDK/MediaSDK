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
#include "membuffin.h"


CMemBuffInput::CMemBuffInput(void)
{
  m_buf     = 0;
  m_buflen  = 0;
  m_currpos = 0;

  return;
} // ctor


CMemBuffInput::~CMemBuffInput(void)
{
  Close();
  return;
} // dtor


JERRCODE CMemBuffInput::Open(const uint8_t *pBuf, size_t buflen)
{
  if(0 == pBuf)
    return JPEG_ERR_PARAMS;

  m_buf     = pBuf;
  m_buflen  = buflen;
  m_currpos = 0;

  return JPEG_OK;
} // CMemBuffInput::Open()


JERRCODE CMemBuffInput::Close(void)
{
  m_buf     = 0;
  m_buflen  = 0;
  m_currpos = 0;

  return JPEG_OK;
} // CMemBuffInput::Close()


JERRCODE CMemBuffInput::Seek(long offset, int origin)
{
  if(m_currpos + offset >= m_buflen || (long)m_currpos + offset < 0)
  {
    return JPEG_ERR_BUFF;
  }

  switch(origin)
  {
  case UIC_SEEK_CUR:
    m_currpos += offset;
    break;

  case UIC_SEEK_SET:
    m_currpos = offset;
    break;

  case UIC_SEEK_END:
    m_currpos = m_buflen;
    break;

  default:
    return JPEG_NOT_IMPLEMENTED;
  }

  return JPEG_OK;
} // CMemBuffInput::Seek()


JERRCODE CMemBuffInput::Read(void* buf,uic_size_t len,uic_size_t* cnt)
{
  uic_size_t rb = std::min<uic_size_t>(len, m_buflen - m_currpos);

  MFX_INTERNAL_CPY((uint8_t*)buf, m_buf + m_currpos,rb);

  m_currpos += rb;

  *cnt = rb;

  if(len != rb)
    return JPEG_ERR_BUFF;

  return JPEG_OK;
} // CMemBuffInput::Read()


JERRCODE CMemBuffInput::TellPos(size_t* pos)
{
  *pos = m_currpos;
  return JPEG_OK;
} // CMemBuffInput::TellPos()

#endif // MFX_ENABLE_MJPEG_VIDEO_DECODE

