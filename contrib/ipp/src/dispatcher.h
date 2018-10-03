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

#ifndef __DISPATCHER_H__
#define __DISPATCHER_H__

#if defined( __cplusplus )
extern "C" {
#endif

/*
  IPP libraries and CPU features mask fitness. Implemented only for IA32 and Intel64 (emt)
*/

#if defined( _ARCH_IA32 )
#ifdef _IPP_QUARK
#define PX_FM ( 0 )
#else
#define PX_FM ( ippCPUID_MMX | ippCPUID_SSE )
#endif

#elif defined (_ARCH_EM64T)

#define PX_FM ( ippCPUID_MMX | ippCPUID_SSE | ippCPUID_SSE2 )
#elif defined (_ARCH_LRB)
  #define PX_FM ( ippCPUID_ABR )
#elif defined (_ARCH_LRB2)
  #define PX_FM ( ippCPUID_KNC )
#elif defined (_ARCH_IA64)
  #error _ARCH_IA64 is not supported
#else
  #error undefined architecture
#endif

#if defined( __cplusplus )
}
#endif

#endif /* __DISPATCHER_H__ */
