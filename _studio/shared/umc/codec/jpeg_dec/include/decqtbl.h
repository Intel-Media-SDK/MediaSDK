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

#ifndef __DECQTBL_H__
#define __DECQTBL_H__

#include "umc_defs.h"
#if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE)
#include "jpegbase.h"



class CJPEGDecoderQuantTable
{
private:
  uint8_t   m_rbf[DCTSIZE2*sizeof(uint16_t)+(CPU_CACHE_LINE-1)];

public:
  int     m_id;
  int     m_precision;
  int     m_initialized;
  uint8_t*  m_raw8u;
  uint16_t* m_raw16u;

  CJPEGDecoderQuantTable(void);
  virtual ~CJPEGDecoderQuantTable(void);

  JERRCODE Init(int id,uint8_t  raw[DCTSIZE2]);

  JERRCODE Init(int id,uint16_t raw[DCTSIZE2]);
};


#endif // MFX_ENABLE_MJPEG_VIDEO_DECODE
#endif // __DECQTBL_H__
