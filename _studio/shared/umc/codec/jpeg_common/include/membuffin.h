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

#ifndef __MEMBUFFIN_H__
#define __MEMBUFFIN_H__

#include "umc_defs.h"
#if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE)
#include "basestream.h"
#include "basestreamin.h"

class CMemBuffInput : public CBaseStreamInput
{
public:
  CMemBuffInput(void);
  ~CMemBuffInput(void);

  JERRCODE Open(const uint8_t* pBuf, size_t buflen);
  JERRCODE Close(void);

  JERRCODE Seek(long offset, int origin);
  JERRCODE Read(void* buf,uic_size_t len,uic_size_t* cnt);
  JERRCODE TellPos(size_t* pos);
  size_t NBytesRead(void) { return m_currpos; }

private:
  JERRCODE Open(vm_char* /*name*/) { return JPEG_NOT_IMPLEMENTED; }

protected:
  const uint8_t *m_buf;
  size_t m_buflen;
  size_t m_currpos;

};

#endif // MFX_ENABLE_MJPEG_VIDEO_DECODE
#endif // __MEMBUFFIN_H__

