// Copyright (c) 2018-2019 Intel Corporation
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
#if defined (MFX_ENABLE_MJPEG_VIDEO_ENCODE)

#include <stdio.h>
#include <string.h>
#include "jpegbase.h"
#include "jpegenc.h"


JERRCODE CJPEGEncoder::WriteRST_T(
  int    next_restart_num,
  int    thread_id)
{
    (void)thread_id;
  JERRCODE jerr;

  TRC0("-> WriteRST");

  TRC1("  emit marker ",JM_RST0 + next_restart_num);
  TRC1("    RST ",0xfff0 | (JM_RST0 + next_restart_num));

  // emit restart interval
  jerr = m_BitStreamOut.WriteWord(0xff00 | (JM_RST0 + next_restart_num));
  if(JPEG_OK != jerr)
    return jerr;


  return JPEG_OK;
} // CJPEGEncoder::WriteRST()


JERRCODE CJPEGEncoder::ProcessRestart(
  int    id[MAX_COMPS_PER_SCAN],
  int    Ss,
  int    Se,
  int    Ah,
  int    Al,
  int    nRSTI,
  int    thread_id)
{
  int       c;
  int       dstLen;
  int       currPos;
  uint8_t*    dst;
  JERRCODE  jerr;
  int status = ippStsNoErr;

  TRC0("-> ProcessRestart");
  
  dst    = m_BitStreamOut.GetDataPtr();
  dstLen = m_BitStreamOut.GetDataLen();

  jerr = m_BitStreamOut.FlushBuffer();
  if(JPEG_OK != jerr)
    return jerr;

  currPos = m_BitStreamOut.GetCurrPos();

  // flush IppiEncodeHuffmanState
  switch(m_jpeg_mode)
  {
  case JPEG_BASELINE:
  case JPEG_EXTENDED:
    {
      status = mfxiEncodeHuffman8x8_JPEG_16s1u_C1(
                 0,dst,dstLen,&currPos,0,0,0,m_state,1);

      break;
    }

  case JPEG_PROGRESSIVE:
    if(Ss == 0 && Se == 0)
    {
      // DC scan
      if(Ah == 0)
      {
        status = mfxiEncodeHuffman8x8_DCFirst_JPEG_16s1u_C1(
                   0,dst,dstLen,&currPos,0,0,0,m_state_t[thread_id],1);
      }
      else
      {
        status = mfxiEncodeHuffman8x8_DCRefine_JPEG_16s1u_C1(
                   0,dst,dstLen,&currPos,0,m_state_t[thread_id],1);
      }
    }
    else
    {
      // AC scan
      IppiEncodeHuffmanSpec* actbl = m_actbl[m_ccomp[id[0]].m_ac_selector];

      if(Ah == 0)
      {
        status = mfxiEncodeHuffman8x8_ACFirst_JPEG_16s1u_C1(
                   0,dst,dstLen,&currPos,Ss,Se,Al,actbl,m_state_t[thread_id],1);
      }
      else
      {
        status = mfxiEncodeHuffman8x8_ACRefine_JPEG_16s1u_C1(
                   0,dst,dstLen,&currPos,Ss,Se,Al,actbl,m_state_t[thread_id],1);
      }
    }
    break;

  case JPEG_LOSSLESS:
    status = mfxiEncodeHuffmanOne_JPEG_16s1u_C1(
               0,dst,dstLen,&currPos,0,m_state_t[thread_id],1);
    break;
  default:
    break;
  }
  m_BitStreamOut.SetCurrPos(currPos);
  if(ippStsNoErr > status)
  {
    LOG1("IPP Error: mfxiEncodeHuffman8x8_JPEG_16s1u_C1() failed - ",status);
    return JPEG_ERR_INTERNAL;
  }
   status = mfxiEncodeHuffmanStateInit_JPEG_8u(m_state);
  if(ippStsNoErr != status)
  {
    return JPEG_ERR_INTERNAL;
  }

  jerr = WriteRST_T(nRSTI & 7, thread_id);
  if(JPEG_OK != jerr)
  {
    LOG1("IPP Error: WriteRST() failed - ",jerr);
    return jerr;
  }

  for(c = 0; c < m_jpeg_ncomp; c++)
  {
    m_ccomp[c].m_lastDC = 0;
  }


  return JPEG_OK;
} // CJPEGEncoder::ProcessRestart()


JERRCODE CJPEGEncoder::EncodeHuffmanMCURowBL_RSTI(int16_t* pMCUBuf, int thread_id)
{
  int                    c;
  int                    vs;
  int                    hs;
  int                    xmcu;
  int                    dstLen;
  int                    currPos;
  uint8_t*                 dst;
  CJPEGColorComponent*   curr_comp;
  IppiEncodeHuffmanSpec* pDCTbl = 0;
  IppiEncodeHuffmanSpec* pACTbl = 0;
  int              status;

  (void)thread_id;

  dst    = m_BitStreamOut.GetDataPtr();
  dstLen = m_BitStreamOut.GetDataLen();

  for(xmcu = 0; xmcu < m_numxMCU; xmcu++)
  {
    for(c = 0; c < m_jpeg_ncomp; c++)
    {
      curr_comp = &m_ccomp[c];
      pDCTbl = m_dctbl[curr_comp->m_dc_selector];
      pACTbl = m_actbl[curr_comp->m_ac_selector];

      for(vs = 0; vs < curr_comp->m_vsampling; vs++)
      {
        for(hs = 0; hs < curr_comp->m_hsampling; hs++)
        {
          m_BitStreamOut.FlushBuffer(SAFE_NBYTES);

          currPos = m_BitStreamOut.GetCurrPos();

          status = mfxiEncodeHuffman8x8_JPEG_16s1u_C1(
                     pMCUBuf,dst,dstLen,&currPos,
                     &curr_comp->m_lastDC,pDCTbl,pACTbl,m_state,0);


          m_BitStreamOut.SetCurrPos(currPos);
          if(ippStsNoErr > status)
          {
            LOG1("IPP Error: mfxiEncodeHuffman8x8_JPEG_16s1u_C1() failed - ",status);
            return JPEG_ERR_INTERNAL;
          }

          pMCUBuf += DCTSIZE2;
        } // for m_hsampling
      } // for m_vsampling
    } // for m_jpeg_ncomp
  } // for numxMCU

  return JPEG_OK;
} // CJPEGEncoder::EncodeHuffmanMCURowBL_RSTI()



JERRCODE CJPEGEncoder::EncodeScanBaselineRSTI(void)
{
  int i;
  int dstLen;
  int currPos;
  uint8_t* dst;

  JERRCODE  jerr;
  int status;

  for(i = 0; i < m_jpeg_ncomp; i++)
  {
    m_ccomp[i].m_lastDC = 0;
  }

  status = mfxiEncodeHuffmanStateInit_JPEG_8u(m_state);
  if(ippStsNoErr != status)
  {
    return JPEG_ERR_INTERNAL;
  }

  jerr = WriteSOS();
  if(JPEG_OK != jerr)
  {
    LOG0("Error: WriteSOS() failed");
    return jerr;
  }


  {
    int     rh         = 0;
    int     currMCURow = 0;
    int     thread_id  = 0;
    int16_t* pMCUBuf    = 0;  // the pointer to Buffer for a current thread.


    pMCUBuf = m_block_buffer + (thread_id) * m_numxMCU * m_nblock * DCTSIZE2*m_rstiHeight;
      for(int curr_rsti = 0; curr_rsti < m_num_rsti; curr_rsti++)
      {
        currMCURow = curr_rsti*m_rstiHeight;

        rh = std::min(m_rstiHeight, m_numyMCU - currMCURow);

        for(int r = 0; r < rh; r++)
        {
          jerr = ColorConvert(currMCURow + r, 0, m_numxMCU/*thread_id*/);
          if(JPEG_OK != jerr)
          {
            return jerr;
          }

          jerr = DownSampling(currMCURow + r, 0, m_numxMCU/*thread_id*/);
          if(JPEG_OK != jerr)
          {
            return jerr;
          }

          jerr = TransformMCURowBL(pMCUBuf, 0, m_numxMCU/*thread_id*/);
          if(JPEG_OK != jerr)
          {
            return jerr;
          }

          jerr = EncodeHuffmanMCURowBL_RSTI(pMCUBuf, thread_id);
          if(JPEG_OK != jerr)
          {
            return jerr;
          }
        }

         // do restart procedure evry rsti
         if(m_jpeg_restart_interval && curr_rsti  < m_num_rsti - 1)
              jerr = ProcessRestart(0,0,63,0,0, curr_rsti, thread_id);

         // flush IppiEncodeHuffmanState for last RSTI
         if(curr_rsti  == m_num_rsti - 1)
         {
           dst    = m_BitStreamOutT[thread_id].GetDataPtr();
           dstLen = m_BitStreamOutT[thread_id].GetDataLen();

           currPos = m_BitStreamOutT[thread_id].GetCurrPos();

           status = mfxiEncodeHuffman8x8_JPEG_16s1u_C1(
                    0,dst,dstLen,&currPos,0,0,0,m_state_t[thread_id],1);

           m_BitStreamOutT[thread_id].SetCurrPos(currPos);
         }


      } // for curr_rsti
} // omp

  if(JPEG_OK != jerr)
  {
    return jerr;
  }

  if(ippStsNoErr > status)
  {
    LOG1("IPP Error: mfxiEncodeHuffman8x8_JPEG_16s1u_C1() failed - ",status);
    return JPEG_ERR_INTERNAL;
  }

  return JPEG_OK;
} // CJPEGEncoder::EncodeScanBaselineRSTI()



JERRCODE CJPEGEncoder::EncodeScanBaselineRSTI_P(void)
{
  int i;
  int dstLen;
  int currPos;
  uint8_t* dst;

  JERRCODE  jerr;
  int status;

  for(i = 0; i < m_jpeg_ncomp; i++)
  {
    m_ccomp[i].m_lastDC = 0;
  }

  status = mfxiEncodeHuffmanStateInit_JPEG_8u(m_state);
  if(ippStsNoErr != status)
  {
    return JPEG_ERR_INTERNAL;
  }

  jerr = WriteSOS();
  if(JPEG_OK != jerr)
  {
    LOG0("Error: WriteSOS() failed");
    return jerr;
  }


  {
    int     rh         = 0;
    int     currMCURow = 0;
    int     thread_id  = 0;
    int16_t* pMCUBuf    = 0;  // the pointer to Buffer for a current thread.


    pMCUBuf = m_block_buffer + (thread_id) * m_numxMCU * m_nblock * DCTSIZE2*m_rstiHeight;
      for(int curr_rsti = 0; curr_rsti < m_num_rsti; curr_rsti++)
      {
        currMCURow = curr_rsti*m_rstiHeight;

        rh = std::min(m_rstiHeight, m_numyMCU - currMCURow);

        for(int r = 0; r < rh; r++)
        {
          if(m_src.color == m_jpeg_color)
          {
            jerr = ProcessBuffer(currMCURow + r, 0, m_numxMCU/*thread_id*/);
            if(JPEG_OK != jerr)
            {
              return jerr;
            }
          }
          else
          {
            jerr = ColorConvert(currMCURow + r, 0, m_numxMCU/*thread_id*/);
            if(JPEG_OK != jerr)
            {
              return jerr;
            }

            jerr = DownSampling(currMCURow + r, 0, m_numxMCU/*thread_id*/);
            if(JPEG_OK != jerr)
            {
              return jerr;
            }
          }

          jerr = TransformMCURowBL(pMCUBuf, 0, m_numxMCU/*thread_id*/);
          if(JPEG_OK != jerr)
          {
            return jerr;
          }

          jerr = EncodeHuffmanMCURowBL_RSTI(pMCUBuf, thread_id);
          if(JPEG_OK != jerr)
          {
            return jerr;
          }
        }

         // do restart procedure evry rsti
         if(m_jpeg_restart_interval && curr_rsti  < m_num_rsti - 1)
              jerr = ProcessRestart(0,0,63,0,0, curr_rsti, thread_id);

         // flush IppiEncodeHuffmanState for last RSTI
         if(curr_rsti  == m_num_rsti - 1)
         {
           dst    = m_BitStreamOutT[thread_id].GetDataPtr();
           dstLen = m_BitStreamOutT[thread_id].GetDataLen();

           currPos = m_BitStreamOutT[thread_id].GetCurrPos();

           status = mfxiEncodeHuffman8x8_JPEG_16s1u_C1(
                    0,dst,dstLen,&currPos,0,0,0,m_state_t[thread_id],1);

           m_BitStreamOutT[thread_id].SetCurrPos(currPos);
         }


      } // for curr_rsti
} // omp

  if(JPEG_OK != jerr)
  {
    return jerr;
  }

  if(ippStsNoErr > status)
  {
    LOG1("IPP Error: mfxiEncodeHuffman8x8_JPEG_16s1u_C1() failed - ",status);
    return JPEG_ERR_INTERNAL;
  }

  return JPEG_OK;
} // CJPEGEncoder::EncodeScanBaselineRSTI_P()

#endif // MFX_ENABLE_MJPEG_VIDEO_ENCODE
