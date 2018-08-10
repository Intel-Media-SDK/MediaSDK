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

#ifndef __BITSTREAMIN_H__
#define __BITSTREAMIN_H__

#include "umc_defs.h"
#if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE)
#include "jpegbase.h"


class CBaseStreamInput;

const int DEC_DEFAULT_BUFLEN = 4096; // internal intermediate input buffer

class CBitStreamInput
{
public:
  CBitStreamInput(void);
  virtual ~CBitStreamInput(void);

  JERRCODE Attach(CBaseStreamInput* in);
  JERRCODE Detach(void);

  JERRCODE Init(int bufSize = DEC_DEFAULT_BUFLEN);
  JERRCODE FillBuffer(int nMinBytes = 0);

  uint8_t* GetDataPtr(void) const { return m_pData; }
  int    GetDataLen(void) const { return m_DataLen; }
  uint8_t* GetCurrPtr(void) const { return m_pData + m_currPos; }
  int    GetCurrPos(void) const { return m_currPos; }
  void   SetCurrPos(int cp)     { m_nUsedBytes += (cp - m_currPos); m_currPos = cp; }
  int    GetNumUsedBytes(void)  { return m_nUsedBytes; }

  JERRCODE Seek(long offset, int origin = UIC_SEEK_CUR);
  JERRCODE CheckByte(int pos, int* byte);
  JERRCODE ReadByte(int* byte);
  JERRCODE ReadWord(int* word);
  JERRCODE ReadDword(int* dword);

protected:
  CBaseStreamInput* m_in;

  uint8_t*            m_pData;
  int               m_DataLen;
  int               m_currPos;
  int               m_nUsedBytes;

  int               m_eod;
};

#endif // MFX_ENABLE_MJPEG_VIDEO_DECODE
#endif // __BITSTREAMIN_H__

