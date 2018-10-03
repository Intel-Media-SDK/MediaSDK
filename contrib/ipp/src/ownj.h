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

#ifndef __OWNJ_H__
#define __OWNJ_H__

#ifndef __OWNDEFS_H__
#include "owndefs.h"
#endif

#ifndef __IPPS_H__
#include "ipps.h"
#endif
#ifndef __IPPI_H__
#include "ippi.h"
#endif
#ifndef __IPPJ_H__
#include "ippj.h"
#endif

#define DCTSIZE   8
#define DCTSIZE2 64

#if _IPP_ARCH == _IPP_ARCH_LRB
#define CPU_CACHE_LINE 64
#else
#define CPU_CACHE_LINE 32
#endif


#define OWN_BADARG_RET(expr) \
  {if (expr) { IPP_ERROR_RET( ippStsBadArgErr ); }}


#define OWNAPI( type,name,arg )  extern type __CDECL name arg
#define OWNFUN( type,name,arg )  extern type __CDECL name arg
#define LOCFUN( type,name,arg )  static type __CDECL name arg
#define ASMAPI OWNAPI
#define ASMFUN OWNFUN


#endif /* __OWNJ_H__ */

/* ///////////////////////// End of file "ownj.h" ////////////////////////// */

