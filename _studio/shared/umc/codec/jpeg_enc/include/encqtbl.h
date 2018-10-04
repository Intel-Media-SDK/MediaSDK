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

#ifndef __ENCQTBL_H__
#define __ENCQTBL_H__

#include "umc_defs.h"
#if defined (MFX_ENABLE_MJPEG_VIDEO_ENCODE)
#include "ippj.h"
#include "jpegbase.h"

class CJPEGEncoderQuantTable
{
private:
  uint8_t   m_rbf[DCTSIZE2*sizeof(uint16_t)+(CPU_CACHE_LINE-1)];
  uint8_t   m_qbf[DCTSIZE2*sizeof(float)+(CPU_CACHE_LINE-1)];
  uint16_t* m_qnt16u;
  float* m_qnt32f;

public:
  int     m_id;
  bool    m_initialized;
  int     m_precision;
  uint8_t*  m_raw8u;
  uint16_t* m_raw16u;

  CJPEGEncoderQuantTable(void);
  virtual ~CJPEGEncoderQuantTable(void);

  JERRCODE Init(int id,uint8_t  raw[DCTSIZE2],int quality);
  JERRCODE Init(int id,uint16_t raw[DCTSIZE2],int quality);

  operator uint16_t*() { return m_precision == 0 ? m_qnt16u : 0; }
  operator float*() { return m_precision == 1 ? m_qnt32f : 0; }
};

#endif // MFX_ENABLE_MJPEG_VIDEO_ENCODE
#endif // __ENCQTBL_H__
