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
#include "jpegbase.h"
#include "basestream.h"
#include "basestreamout.h"
#include "bitstreamout.h"


CBitStreamOutput::CBitStreamOutput(void)
{
  m_out     = 0;
  m_pData   = 0;
  m_DataLen = 0;
  m_currPos = 0;
  m_nBytesWritten = 0;

  return;
} // ctor


CBitStreamOutput::~CBitStreamOutput(void)
{
  Detach();
  return;
} // dtor


JERRCODE CBitStreamOutput::Attach(CBaseStreamOutput* out)
{
  Detach();

  m_out = out;

  return JPEG_OK;
} // CBitStreamOutput::Attach()


JERRCODE CBitStreamOutput::Detach(void)
{
  delete[] m_pData;

  m_out     = 0;
  m_pData   = 0;
  m_DataLen = 0;
  m_currPos = 0;

  m_nBytesWritten = 0;

  return JPEG_OK;
} // CBitStreamOutput::Detach()


JERRCODE CBitStreamOutput::Init(int bufSize)
{
  m_DataLen = (int)bufSize;

  delete[] m_pData;

  m_pData = new uint8_t[m_DataLen];

  m_currPos = 0; // no data yet

  m_nBytesWritten = 0;

  return JPEG_OK;
} // CBitStreamOutput::Init()


JERRCODE CBitStreamOutput::FlushBuffer(int nMinBytes)
{
  int remainder;
  uic_size_t cnt = 0;

  if(m_currPos > m_DataLen)
    return JPEG_ERR_BUFF;

  if(nMinBytes)
  {
    remainder = m_DataLen - m_currPos;

    if(remainder > nMinBytes)
      return JPEG_OK;
  }

  m_out->Write(m_pData,m_currPos,&cnt);
  if((int)cnt != m_currPos)
    return JPEG_ERR_FILE;

  m_nBytesWritten += m_currPos;
  m_currPos = 0;

  return JPEG_OK;
} // CBitStreamOutput::FlushBuffer()


JERRCODE CBitStreamOutput::FlushBitStream(CBitStreamOutput &bitStream)
{
  uint8_t* buf     = 0;
  int    currPos  = 0;
  int    dataLen = 0;

  uic_size_t cnt = 0;

  buf     = bitStream.GetDataPtr();
  currPos = bitStream.GetCurrPos();
  dataLen = bitStream.GetDataLen();

  if(currPos > dataLen)
    return JPEG_ERR_BUFF;

  m_out->Write(buf,currPos,&cnt);
  if((int)cnt != currPos)
    return JPEG_ERR_FILE;

  m_nBytesWritten += currPos;
  m_currPos = 0;
  bitStream.SetCurrPos(0);

  return JPEG_OK;
} // CBitStreamOutput::FlushBitStream()


JERRCODE CBitStreamOutput::WriteByte(int byte)
{
  JERRCODE jerr;

  if(m_currPos >= m_DataLen)
  {
    jerr = FlushBuffer();
    if(JPEG_OK != jerr)
      return jerr;
  }

  m_pData[m_currPos] = (uint8_t)byte;
  m_currPos++;

  return JPEG_OK;
} // CBitStreamOutput::WriteByte()


JERRCODE CBitStreamOutput::WriteWord(int word)
{
  int byte0;
  int byte1;
  JERRCODE jerr;

  byte0 = word >> 8;
  byte1 = word & 0x00ff;

  jerr = WriteByte(byte0);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = WriteByte(byte1);
  if(JPEG_OK != jerr)
    return jerr;

  return JPEG_OK;
} // CBitStreamOutput::WriteWord()


JERRCODE CBitStreamOutput::WriteDword(int dword)
{
  int word0;
  int word1;
  JERRCODE jerr;

  word0 = dword >> 16;
  word1 = dword & 0x0000ffff;

  jerr = WriteWord(word0);
  if(JPEG_OK != jerr)
    return jerr;

  jerr = WriteWord(word1);
  if(JPEG_OK != jerr)
    return jerr;

  return JPEG_OK;
} // CBitStreamOutput::WriteDword()

#endif // MFX_ENABLE_MJPEG_VIDEO_DECODE || MFX_ENABLE_MJPEG_VIDEO_ENCODE
