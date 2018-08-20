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

#include "umc_defs.h"
#if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE) || defined (MFX_ENABLE_MJPEG_VIDEO_ENCODE)

#ifndef __COLORCOMP_H__
#include "colorcomp.h"
#endif


CJPEGColorComponent::CJPEGColorComponent(void)
{
  m_id          = 0;
  m_comp_no     = 0;
  m_hsampling   = 0;
  m_vsampling   = 0;
  m_scan_hsampling = 0;
  m_scan_vsampling = 0;
  m_h_factor    = 0;
  m_v_factor    = 0;
  m_nblocks     = 0;
  m_q_selector  = 0;
  m_dc_selector = 0;
  m_ac_selector = 0;
  m_lastDC      = 0;
  m_ac_scan_completed = 0;
  m_cc_height   = 0;
  m_cc_step     = 0;
  m_cc_bufsize  = 0;
  m_ss_height   = 0;
  m_ss_step     = 0;
  m_ss_bufsize  = 0;
  m_curr_row    = 0;
  m_prev_row    = 0;

  m_lnz_bufsize = 0;
  m_lnz_ds      = 0;

  m_need_upsampling   = 0;

  return;
} // ctor


CJPEGColorComponent::~CJPEGColorComponent(void)
{
  return;
} // dtor;


JERRCODE CJPEGColorComponent::CreateBufferCC(int num_threads)
{
  m_cc_bufsize = m_cc_step * m_cc_height;

  return m_cc_buf.Allocate(m_cc_bufsize * num_threads);
} // CJPEGColorComponent::CreateBufferCC()


JERRCODE CJPEGColorComponent::DeleteBufferCC(void)
{
  return m_cc_buf.Delete();
} // CJPEGColorComponent::DeleteBufferCC()


JERRCODE CJPEGColorComponent::CreateBufferSS(int num_threads)
{
  m_ss_bufsize = m_ss_step * m_ss_height;

  return m_ss_buf.Allocate(m_ss_bufsize * num_threads);
} // CJPEGColorComponent::CreateBufferSS()


JERRCODE CJPEGColorComponent::DeleteBufferSS(void)
{
  return m_ss_buf.Delete();
} // CJPEGColorComponent::DeleteBufferSS()


uint8_t* CJPEGColorComponent::GetCCBufferPtr(int thread_id)
{
  uint8_t* ptr = m_cc_buf;

  return &ptr[m_cc_bufsize * thread_id];
} // CJPEGColorComponent::GetCCBufferPtr()


uint8_t* CJPEGColorComponent::GetSSBufferPtr(int thread_id)
{
  uint8_t* ptr = m_ss_buf;

  return &ptr[m_ss_bufsize*thread_id];
} // CJPEGColorComponent::GetCCBufferPtr()


JERRCODE CJPEGColorComponent::CreateBufferLNZ(int num_threads)
{
  return m_lnz_buf.Allocate(m_lnz_bufsize * num_threads);
} // CJPEGColorComponent::CreateBufferLNZ()


JERRCODE CJPEGColorComponent::DeleteBufferLNZ(void)
{
  return m_lnz_buf.Delete();
} // CJPEGColorComponent::DeleteBufferLNZ()


uint8_t* CJPEGColorComponent::GetLNZBufferPtr(int thread_id)
{
  uint8_t* ptr = m_lnz_buf;

  return &ptr[m_lnz_bufsize * thread_id];
} // CJPEGColorComponent::GetLNZBufferPtr()

#endif // MFX_ENABLE_MJPEG_VIDEO_DECODE || MFX_ENABLE_MJPEG_VIDEO_ENCODE

