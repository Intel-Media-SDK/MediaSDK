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

#ifndef __BITSTREAMOUT_H__
#define __BITSTREAMOUT_H__

#include "umc_defs.h"
#if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE) || defined (MFX_ENABLE_MJPEG_VIDEO_ENCODE)

#include "jpegbase.h"


class CBaseStreamOutput;

const int ENC_DEFAULT_BUFLEN = 4096; // internal intermediate output buffer

class CBitStreamOutput
{
public:
  CBitStreamOutput(void);
  virtual ~CBitStreamOutput(void);

  JERRCODE Attach(CBaseStreamOutput* out);
  JERRCODE Detach(void);

  JERRCODE Init(int bufSize = ENC_DEFAULT_BUFLEN);
  JERRCODE FlushBuffer(int nMinBytes = 0);

  JERRCODE FlushBitStream(CBitStreamOutput& bitStream);

  uint8_t* GetDataPtr(void) const { return m_pData; }
  int GetDataLen(void) const { return m_DataLen; }
  int GetCurrPos(void) const { return m_currPos; }
  void SetCurrPos(int cp) { m_currPos = cp; return; }

  int NumOfBytes(void) const { return m_nBytesWritten; }

  JERRCODE WriteByte(int byte);
  JERRCODE WriteWord(int word);
  JERRCODE WriteDword(int dword);

protected:
  CBaseStreamOutput* m_out;

  uint8_t*             m_pData;
  int                m_DataLen;
  int                m_currPos;

  int                m_nBytesWritten;
};

#endif // MFX_ENABLE_MJPEG_VIDEO_DECODE || MFX_ENABLE_MJPEG_VIDEO_ENCODE
#endif // __BITSTREAMOUT_H__

