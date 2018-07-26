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

#ifndef __BS_DEF_H
#define __BS_DEF_H

//#undef __BS_TRACE__
//#define  __BS_TRACE__

typedef unsigned char    byte;
typedef unsigned char    Bs8u;
typedef unsigned short   Bs16u;
typedef unsigned int     Bs32u;

typedef signed char      Bs8s;
typedef signed short     Bs16s;
typedef signed int       Bs32s;

#ifdef __GNUC__
 typedef unsigned long long Bs64u;
 typedef signed long long   Bs64s;
#else
 typedef unsigned __int64 Bs64u;
 typedef signed __int64   Bs64s;
#endif

 #if defined( _WIN32 ) || defined ( _WIN64 )
  #define __STDCALL  __stdcall
  #define __CDECL    __cdecl
  #define __INT64    __int64
  #define __UINT64    unsigned __int64
#else
  #define __STDCALL
  #define __CDECL
  #define __INT64    long long
  #define __UINT64    unsigned long long
#endif

typedef enum {
    BS_ERR_NONE                 =  0,
    BS_ERR_UNKNOWN              = -1,
    BS_ERR_WRONG_UNITS_ORDER    = -2,
    BS_ERR_MORE_DATA            = -3,
    BS_ERR_INVALID_PARAMS       = -4,
    BS_ERR_MEM_ALLOC            = -5,
    BS_ERR_NOT_IMPLEMENTED      = -6,
    BS_ERR_NOT_ENOUGH_BUFFER    = -7,
    BS_ERR_BAD_HANDLE           = -8,
    BS_ERR_INCOMPLETE_DATA      = -9
} BSErr;

#define BS_MIN(x, y) ( ((x) < (y)) ? (x) : (y) )
#define BS_MAX(x, y) ( ((x) > (y)) ? (x) : (y) )
#define BS_ABS(x)    ( ((x) > 0) ? (x) : (-(x)) )

#endif //__BS_DEF_H
