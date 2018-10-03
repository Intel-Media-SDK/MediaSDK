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

#ifndef __CPUDEF_H__
#define __CPUDEF_H__

#include "ippcore.h"

#if defined( __cplusplus )
extern "C" {
#endif

#undef __CDECL
#if defined( _WIN32 ) || defined ( _WIN64 )
  #define __CDECL    __cdecl
#else
  #define __CDECL
#endif

/* Intel CPU informator */

int __CDECL mfxownGetFeature( Ipp64u MaskOfFeature );

int __CDECL mfxhas_cpuid ( void );
int  __CDECL mfxis_GenuineIntel ( void );
int  __CDECL mfxmax_cpuid_input( void );

#if defined( __cplusplus )
}
#endif

#endif /* __CPUDEF_H__ */

/* ////////////////////////// End of file "cpudef.h" //////////////////////// */
