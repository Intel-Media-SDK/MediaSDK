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

#ifndef __ENCHTBL_H__
#define __ENCHTBL_H__

#include "umc_defs.h"
#if defined (MFX_ENABLE_MJPEG_VIDEO_ENCODE)
#include "ippj.h"
#include "jpegbase.h"

class CJPEGEncoderHuffmanTable
{
private:
  IppiEncodeHuffmanSpec* m_table;
  bool                   m_bValid;

public:
  int                    m_id;
  int                    m_hclass;
  uint8_t                  m_bits[16];
  uint8_t                  m_vals[256];

  CJPEGEncoderHuffmanTable(void);
  virtual ~CJPEGEncoderHuffmanTable(void);

  JERRCODE Create(void);
  JERRCODE Destroy(void);
  JERRCODE Init(int id,int hclass,uint8_t* bits,uint8_t* vals);
  
  bool     IsValid(void)                { return m_bValid; }

  operator IppiEncodeHuffmanSpec*(void) { return m_table; }
};


class CJPEGEncoderHuffmanState
{
private:
  IppiEncodeHuffmanState* m_state;

public:
  CJPEGEncoderHuffmanState(void);
  virtual ~CJPEGEncoderHuffmanState(void);

  JERRCODE Create(void);
  JERRCODE Destroy(void);
  JERRCODE Init(void);

  operator IppiEncodeHuffmanState*(void) { return m_state; }
};

#endif // MFX_ENABLE_MJPEG_VIDEO_ENCODE
#endif // __ENCHTBL_H__
