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

#ifndef __COLORCOMP_H__
#define __COLORCOMP_H__

#include "umc_defs.h"
#if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE) || defined (MFX_ENABLE_MJPEG_VIDEO_ENCODE)
#include "jpegbase.h"



class CJPEGColorComponent
{
public:
  int m_id;
  int m_comp_no;
  int m_hsampling;
  int m_vsampling;
  int m_scan_hsampling;
  int m_scan_vsampling;
  int m_h_factor;
  int m_v_factor;
  int m_nblocks;
  int m_q_selector;
  int m_dc_selector;
  int m_ac_selector;
  int m_ac_scan_completed;
  int m_cc_height;
  int m_cc_step;
  int m_cc_bufsize;
  int m_ss_height;
  int m_ss_step;
  int m_ss_bufsize;
  int m_need_upsampling;
  int16_t m_lastDC;

  CMemoryBuffer m_cc_buf;
  CMemoryBuffer m_ss_buf;
  CMemoryBuffer m_row1;
  CMemoryBuffer m_row2;

  CMemoryBuffer m_lnz_buf;
  int           m_lnz_bufsize;
  int           m_lnz_ds;

  int16_t* m_curr_row;
  int16_t* m_prev_row;

  CJPEGColorComponent(void);
  virtual ~CJPEGColorComponent(void);

  JERRCODE CreateBufferCC(int num_threads = 1);
  JERRCODE DeleteBufferCC(void);

  JERRCODE CreateBufferSS(int num_threads = 1);
  JERRCODE DeleteBufferSS(void);

  JERRCODE CreateBufferLNZ(int num_threads = 1);
  JERRCODE DeleteBufferLNZ(void);

  uint8_t* GetCCBufferPtr(int thread_id = 0);
  // Get the buffer pointer
  template <class type_t> inline
  type_t *GetCCBufferPtr(const uint32_t colMCU) const;

  uint8_t* GetSSBufferPtr(int thread_id = 0);
  // Get the buffer pointer
  template <class type_t> inline
  type_t *GetSSBufferPtr(const uint32_t colMCU) const;

  uint8_t* GetLNZBufferPtr(int thread_id = 0);

};

template <class type_t> inline
type_t *CJPEGColorComponent::GetCCBufferPtr (const uint32_t colMCU) const
{
  type_t *ptr = (type_t *) m_cc_buf.m_buffer;

  return (ptr + colMCU * 8 * m_hsampling);

} // type_t *CJPEGColorComponent::GetCCBufferPtr (const uint32_t colMCU) const

template <class type_t> inline
type_t *CJPEGColorComponent::GetSSBufferPtr(const uint32_t colMCU) const
{
  type_t *ptr = (type_t *) m_ss_buf.m_buffer;

  return (ptr + colMCU * 8 * m_hsampling);

} // type_t *CJPEGColorComponent::GetSSBufferPtr(const uint32_t colMCU) const

#endif // MFX_ENABLE_MJPEG_VIDEO_DECODE || MFX_ENABLE_MJPEG_VIDEO_ENCODE
#endif // __COLORCOMP_H__
