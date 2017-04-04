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

#ifndef CM_DEF_H
#define CM_DEF_H

#include <limits>
#include <limits.h>
#include <stdlib.h>
class SurfaceIndex;

typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;

#ifdef __GNUC__
#ifndef __int8
typedef char __int8;
#endif

#ifndef __int16
typedef short __int16;
#endif

#ifndef __int32
typedef int __int32;
#endif

#ifndef __int64
typedef long long __int64;
#endif

#ifndef __uint64
typedef unsigned long long __uint64;
#endif

#ifndef UINT
typedef unsigned int  UINT;
#endif

#ifndef DWORD
typedef unsigned int DWORD;
#endif

#ifndef BYTE
typedef unsigned char BYTE;
#endif

#endif //__GNUC__

#if !defined(__CLANG_CM) && (!defined(_WIN32) || _MSC_VER >= 1600)
#include <stdint.h>
#else
typedef signed   __int8   int8_t;
typedef signed   __int16  int16_t;
typedef signed   __int32  int32_t;
typedef signed   __int64  int64_t;
typedef unsigned __int8   uint8_t;
typedef unsigned __int16  uint16_t;
typedef unsigned __int32  uint32_t;
typedef unsigned __int64  uint64_t;
#endif


#define SAT 1

typedef enum _CmAtomicOpType_
{
    ATOMIC_ADD                     = 0x0,
    ATOMIC_SUB                     = 0x1,
    ATOMIC_INC                     = 0x2,
    ATOMIC_DEC                     = 0x3,
    ATOMIC_MIN                     = 0x4,
    ATOMIC_MAX                     = 0x5,
    ATOMIC_XCHG                    = 0x6,
    ATOMIC_CMPXCHG                 = 0x7,
    ATOMIC_AND                     = 0x8,
    ATOMIC_OR                      = 0x9,
    ATOMIC_XOR                     = 0xa,
    ATOMIC_MINSINT                 = 0xb,
    ATOMIC_MAXSINT                 = 0xc
} CmAtomicOpType;


typedef enum _ChannelMaskType_
{	CM_R_ENABLE         = 1,
	CM_G_ENABLE         = 2,
	CM_GR_ENABLE        = 3,
	CM_B_ENABLE         = 4,
	CM_BR_ENABLE        = 5,
	CM_BG_ENABLE        = 6,
	CM_BGR_ENABLE       = 7,
    CM_A_ENABLE         = 8,
    CM_AR_ENABLE        = 9,
    CM_AG_ENABLE        = 10,
    CM_AGR_ENABLE       = 11,
    CM_AB_ENABLE        = 12,
    CM_ABR_ENABLE       = 13,
    CM_ABG_ENABLE       = 14,
    CM_ABGR_ENABLE      = 15
} ChannelMaskType;


typedef enum _CmRoundingMode_
{
    // These values are stored pre-shifted to remove the need to shift the values when applying to
    // the control word
    CM_RTE                = 0,       // Round to nearest or even
    CM_RTP                = 1 << 4,  // Round towards +ve inf
    CM_RTN                = 2 << 4,  // Round towards -ve inf
    CM_RTZ                = 3 << 4   // Round towards zero
} CmRoundingMode;

namespace CmEmulSys
{

static long long
abs(long long a)
{
    if (a < 0) {
        return -a;
    } else {
        return a;
    }
}

template<typename RT>
struct satur {
template<typename T> static RT
saturate(const T val, const int flags) {
    if ((flags & SAT) == 0) {
        return (RT) val;
    }

#ifdef max
#undef max
#undef min
#endif
    const RT t_max = std::numeric_limits<RT>::max();
    const RT t_min = std::numeric_limits<RT>::min();


    if (val > t_max) {
        return t_max;
    } else if ((val >= 0 ) && (t_min < 0)) {
        // RT is "signed" if t_min < 0
        // when comparing a signed and a unsigned variable, the signed one cast to unsigned first.
        return (RT) val;
    } else if (val < t_min) {
        return t_min;
    } else {
        return (RT) val;
    }
}
};

template<>
struct satur<float> {
template<typename T> static float
saturate(const T val, const int flags) {
    if ((flags & SAT) == 0) {
        return (float) val;
    }

    if (val < 0.) {
        return 0;
    } else if (val > 1.) {
        return 1.;
    } else {
        return (float) val;
    }
}
};

template<>
struct satur<double> {
template<typename T> static double
saturate(const T val, const int flags) {
    if ((flags & SAT) == 0) {
        return (double) val;
    }

    if (val < 0.) {
        return 0;
    } else if (val > 1.) {
        return 1.;
    } else {
        return (double) val;
    }
}
};

template<typename T1, bool B> struct _SetSatur {
    static uint SetSatur() {
        return 0;
    }
};

template <> struct _SetSatur<float, true> {
    static uint SetSatur() {
        return SAT;
    }
};

template <> struct _SetSatur<double, true> {
    static uint SetSatur() {
        return SAT;
    }
};

} /* ::CmEmulSys */

template <typename T1, typename T2> struct restype {
private:
    restype();
};

//#if defined(CM_NOCONV) || defined(CM_NONSTRICT)
#if defined(CM_NOCONV)

template <> struct restype<char, char>            { typedef short  type; };
template <> struct restype<char, unsigned char>   { typedef short  type; };
template <> struct restype<char, short>           { typedef short type; };
template <> struct restype<char, unsigned short>  { typedef short type; };
template <> struct restype<char, int>             { typedef int   type; };
template <> struct restype<char, unsigned int>    { typedef int   type; };
template <> struct restype<char, float>           { typedef float type; };
template <> struct restype<char, double>          { typedef double type; };
template <> struct restype<char, long long>       { typedef long long type; };
template <> struct restype<char, unsigned long long>           { typedef long long type; };

template <> struct restype<unsigned char, char>            { typedef short  type; };
template <> struct restype<unsigned char, unsigned char>   { typedef short  type; };
template <> struct restype<unsigned char, short>           { typedef short type; };
template <> struct restype<unsigned char, unsigned short>  { typedef short type; };
template <> struct restype<unsigned char, int>             { typedef int   type; };
template <> struct restype<unsigned char, unsigned int>    { typedef int   type; };
template <> struct restype<unsigned char, float>           { typedef float type; };
template <> struct restype<unsigned char, double>          { typedef double type; };
template <> struct restype<unsigned char, long long>       { typedef long long type; };
template <> struct restype<unsigned char, unsigned long long>           { typedef long long type; };

template <> struct restype<short, char>            { typedef short type; };
template <> struct restype<short, unsigned char>   { typedef short type; };
template <> struct restype<short, short>           { typedef short type; };
template <> struct restype<short, unsigned short>  { typedef short type; };
template <> struct restype<short, int>             { typedef int   type; };
template <> struct restype<short, unsigned int>    { typedef int   type; };
template <> struct restype<short, float>           { typedef float type; };
template <> struct restype<short, double>          { typedef double type; };
template <> struct restype<short, long long>       { typedef long long type; };
template <> struct restype<short, unsigned long long>           { typedef long long type; };

template <> struct restype<unsigned short, char>            { typedef short type; };
template <> struct restype<unsigned short, unsigned char>   { typedef short type; };
template <> struct restype<unsigned short, short>           { typedef short type; };
template <> struct restype<unsigned short, unsigned short>  { typedef short type; };
template <> struct restype<unsigned short, int>             { typedef int type; };
template <> struct restype<unsigned short, unsigned int>    { typedef int type; };
template <> struct restype<unsigned short, float>           { typedef float type; };
template <> struct restype<unsigned short, double>          { typedef double type; };
template <> struct restype<unsigned short, long long>       { typedef long long type; };
template <> struct restype<unsigned short, unsigned long long>           { typedef long long type; };

template <> struct restype<int, char>            { typedef int type; };
template <> struct restype<int, unsigned char>   { typedef int type; };
template <> struct restype<int, short>           { typedef int type; };
template <> struct restype<int, unsigned short>  { typedef int type; };
template <> struct restype<int, int>             { typedef int type; };
template <> struct restype<int, unsigned int>    { typedef int type; };
template <> struct restype<int, float>           { typedef float type; };
template <> struct restype<int, double>          { typedef double type; };
template <> struct restype<int, long long>       { typedef long long type; };
template <> struct restype<int, unsigned long long>           { typedef long long type; };

template <> struct restype<unsigned int, char>            { typedef int type; };
template <> struct restype<unsigned int, unsigned char>   { typedef int type; };
template <> struct restype<unsigned int, short>           { typedef int type; };
template <> struct restype<unsigned int, unsigned short>  { typedef int type; };
template <> struct restype<unsigned int, int>             { typedef int type; };
template <> struct restype<unsigned int, unsigned int>    { typedef int type; };
template <> struct restype<unsigned int, float>           { typedef float type; };
template <> struct restype<unsigned int, double>          { typedef double type; };
template <> struct restype<unsigned int, long long>       { typedef long long type; };
template <> struct restype<unsigned int, unsigned long long>           { typedef long long type; };

template <> struct restype<float, char>            { typedef float type; };
template <> struct restype<float, unsigned char>   { typedef float type; };
template <> struct restype<float, short>           { typedef float type; };
template <> struct restype<float, unsigned short>  { typedef float type; };
template <> struct restype<float, int>             { typedef float type; };
template <> struct restype<float, unsigned int>    { typedef float type; };
template <> struct restype<float, float>           { typedef float type; };
template <> struct restype<float, double>          { typedef double type; };
template <> struct restype<float, long long>       { typedef float type; };
template <> struct restype<float, unsigned long long>           { typedef float type; };

template <> struct restype<double, char>            { typedef double type; };
template <> struct restype<double, unsigned char>   { typedef double type; };
template <> struct restype<double, short>           { typedef double type; };
template <> struct restype<double, unsigned short>  { typedef double type; };
template <> struct restype<double, int>             { typedef double type; };
template <> struct restype<double, unsigned int>    { typedef double type; };
template <> struct restype<double, float>           { typedef double type; };
template <> struct restype<double, double>          { typedef double type; };
template <> struct restype<double, long long>       { typedef double type; };
template <> struct restype<double, unsigned long long>           { typedef double type; };

template <> struct restype<long long, char>            { typedef long long type; };
template <> struct restype<long long, unsigned char>   { typedef long long type; };
template <> struct restype<long long, short>           { typedef long long type; };
template <> struct restype<long long, unsigned short>  { typedef long long type; };
template <> struct restype<long long, int>             { typedef long long type; };
template <> struct restype<long long, unsigned int>    { typedef long long type; };
template <> struct restype<long long, float>           { typedef float type; };
template <> struct restype<long long, double>          { typedef double type; };
template <> struct restype<long long, long long>       { typedef long long type; };
template <> struct restype<long long, unsigned long long>           { typedef long long type; };

template <> struct restype<unsigned long long, char>             { typedef long long type; };
template <> struct restype<unsigned long long, unsigned char>    { typedef long long type; };
template <> struct restype<unsigned long long, short>            { typedef long long type; };
template <> struct restype<unsigned long long, unsigned short>   { typedef long long type; };
template <> struct restype<unsigned long long, int>              { typedef long long type; };
template <> struct restype<unsigned long long, unsigned int>     { typedef long long type; };
template <> struct restype<unsigned long long, float>            { typedef float type; };
template <> struct restype<unsigned long long, double>           { typedef double type; };
template <> struct restype<unsigned long long, long long>        { typedef long long type; };
template <> struct restype<unsigned long long, unsigned long long>            { typedef long long type; };

#else

template <> struct restype<char, char>            { typedef int  type; };
template <> struct restype<char, unsigned char>   { typedef int  type; };
template <> struct restype<char, short>           { typedef int type; };
template <> struct restype<char, unsigned short>  { typedef int type; };
template <> struct restype<char, int>             { typedef int   type; };
template <> struct restype<char, unsigned int>    { typedef unsigned int   type; };
template <> struct restype<char, float>           { typedef float type; };
template <> struct restype<char, double>          { typedef double type; };
template <> struct restype<char, long long>       { typedef long long type; };
template <> struct restype<char, unsigned long long>           { typedef unsigned long long type; };

template <> struct restype<unsigned char, char>            { typedef int  type; };
template <> struct restype<unsigned char, unsigned char>   { typedef int  type; };
template <> struct restype<unsigned char, short>           { typedef int type; };
template <> struct restype<unsigned char, unsigned short>  { typedef int type; };
template <> struct restype<unsigned char, int>             { typedef int   type; };
template <> struct restype<unsigned char, unsigned int>    { typedef unsigned int   type; };
template <> struct restype<unsigned char, float>           { typedef float type; };
template <> struct restype<unsigned char, double>          { typedef double type; };
template <> struct restype<unsigned char, long long>       { typedef long long type; };
template <> struct restype<unsigned char, unsigned long long>           { typedef unsigned long long type; };

template <> struct restype<short, char>            { typedef int type; };
template <> struct restype<short, unsigned char>   { typedef int type; };
template <> struct restype<short, short>           { typedef int type; };
template <> struct restype<short, unsigned short>  { typedef int type; };
template <> struct restype<short, int>             { typedef int   type; };
template <> struct restype<short, unsigned int>    { typedef unsigned int   type; };
template <> struct restype<short, float>           { typedef float type; };
template <> struct restype<short, double>          { typedef double type; };
template <> struct restype<short, long long>       { typedef long long type; };
template <> struct restype<short, unsigned long long>           { typedef unsigned long long type; };

template <> struct restype<unsigned short, char>            { typedef int type; };
template <> struct restype<unsigned short, unsigned char>   { typedef int type; };
template <> struct restype<unsigned short, short>           { typedef int type; };
template <> struct restype<unsigned short, unsigned short>  { typedef int type; };
template <> struct restype<unsigned short, int>             { typedef int type; };
template <> struct restype<unsigned short, unsigned int>    { typedef unsigned int type; };
template <> struct restype<unsigned short, float>           { typedef float type; };
template <> struct restype<unsigned short, double>          { typedef double type; };
template <> struct restype<unsigned short, long long>       { typedef long long type; };
template <> struct restype<unsigned short, unsigned long long>           { typedef unsigned long long type; };

template <> struct restype<int, char>            { typedef int type; };
template <> struct restype<int, unsigned char>   { typedef int type; };
template <> struct restype<int, short>           { typedef int type; };
template <> struct restype<int, unsigned short>  { typedef int type; };
template <> struct restype<int, int>             { typedef int type; };
template <> struct restype<int, unsigned int>    { typedef unsigned int type; };
template <> struct restype<int, float>           { typedef float type; };
template <> struct restype<int, double>          { typedef double type; };
template <> struct restype<int, long long>       { typedef long long type; };
template <> struct restype<int, unsigned long long>           { typedef unsigned long long type; };

template <> struct restype<unsigned int, char>            { typedef unsigned int type; };
template <> struct restype<unsigned int, unsigned char>   { typedef unsigned int type; };
template <> struct restype<unsigned int, short>           { typedef unsigned int type; };
template <> struct restype<unsigned int, unsigned short>  { typedef unsigned int type; };
template <> struct restype<unsigned int, int>             { typedef unsigned int type; };
template <> struct restype<unsigned int, unsigned int>    { typedef unsigned int type; };
template <> struct restype<unsigned int, float>           { typedef float type; };
template <> struct restype<unsigned int, double>          { typedef double type; };
template <> struct restype<unsigned int, long long>       { typedef long long type; };
template <> struct restype<unsigned int, unsigned long long>           { typedef unsigned long long type; };

template <> struct restype<float, char>            { typedef float type; };
template <> struct restype<float, unsigned char>   { typedef float type; };
template <> struct restype<float, short>           { typedef float type; };
template <> struct restype<float, unsigned short>  { typedef float type; };
template <> struct restype<float, int>             { typedef float type; };
template <> struct restype<float, unsigned int>    { typedef float type; };
template <> struct restype<float, float>           { typedef float type; };
template <> struct restype<float, double>          { typedef double type; };
template <> struct restype<float, long long>       { typedef float type; };
template <> struct restype<float, unsigned long long>           { typedef float type; };

template <> struct restype<double, char>            { typedef double type; };
template <> struct restype<double, unsigned char>   { typedef double type; };
template <> struct restype<double, short>           { typedef double type; };
template <> struct restype<double, unsigned short>  { typedef double type; };
template <> struct restype<double, int>             { typedef double type; };
template <> struct restype<double, unsigned int>    { typedef double type; };
template <> struct restype<double, float>           { typedef double type; };
template <> struct restype<double, double>          { typedef double type; };
template <> struct restype<double, long long>       { typedef double type; };
template <> struct restype<double, unsigned long long>           { typedef double type; };

template <> struct restype<unsigned long long, char>            { typedef unsigned long long type; };
template <> struct restype<unsigned long long, unsigned char>   { typedef unsigned long long type; };
template <> struct restype<unsigned long long, short>           { typedef unsigned long long type; };
template <> struct restype<unsigned long long, unsigned short>  { typedef unsigned long long type; };
template <> struct restype<unsigned long long, int>             { typedef unsigned long long type; };
template <> struct restype<unsigned long long, unsigned int>    { typedef unsigned long long type; };
template <> struct restype<unsigned long long, float>           { typedef float type; };
template <> struct restype<unsigned long long, double>          { typedef double type; };
template <> struct restype<unsigned long long, long long>       { typedef unsigned long long type; };
template <> struct restype<unsigned long long, unsigned long long>           { typedef unsigned long long type; };

template <> struct restype<long long, char>            { typedef long long type; };
template <> struct restype<long long, unsigned char>   { typedef long long type; };
template <> struct restype<long long, short>           { typedef long long type; };
template <> struct restype<long long, unsigned short>  { typedef long long type; };
template <> struct restype<long long, int>             { typedef long long type; };
template <> struct restype<long long, unsigned int>    { typedef long long type; };
template <> struct restype<long long, float>           { typedef float type; };
template <> struct restype<long long, double>          { typedef double type; };
template <> struct restype<long long, long long>       { typedef long long type; };
template <> struct restype<long long, unsigned long long>           { typedef unsigned long long type; };

#endif

template <typename T1, typename T2> struct bitwise_restype {
private:
    bitwise_restype();
};

#if defined(CM_NOCONV)

template <> struct bitwise_restype<char, char>            { typedef char  type; };
template <> struct bitwise_restype<char, unsigned char>   { typedef short  type; };
template <> struct bitwise_restype<char, short>           { typedef short type; };
template <> struct bitwise_restype<char, unsigned short>  { typedef short type; };
template <> struct bitwise_restype<char, int>             { typedef int   type; };
template <> struct bitwise_restype<char, unsigned int>    { typedef int   type; };
template <> struct bitwise_restype<char, float>           { typedef float type; };
template <> struct bitwise_restype<char, double>          { typedef double type; };
template <> struct bitwise_restype<char, long long>       { typedef long long type; };
template <> struct bitwise_restype<char, unsigned long long>           { typedef long long type; };

template <> struct bitwise_restype<unsigned char, char>            { typedef char  type; };
template <> struct bitwise_restype<unsigned char, unsigned char>   { typedef char  type; };
template <> struct bitwise_restype<unsigned char, short>           { typedef short type; };
template <> struct bitwise_restype<unsigned char, unsigned short>  { typedef short type; };
template <> struct bitwise_restype<unsigned char, int>             { typedef int   type; };
template <> struct bitwise_restype<unsigned char, unsigned int>    { typedef int   type; };
template <> struct bitwise_restype<unsigned char, float>           { typedef float type; };
template <> struct bitwise_restype<unsigned char, double>          { typedef double type; };
template <> struct bitwise_restype<unsigned char, long long>       { typedef long long type; };
template <> struct bitwise_restype<unsigned char, unsigned long long>           { typedef long long type; };

template <> struct bitwise_restype<short, char>            { typedef short type; };
template <> struct bitwise_restype<short, unsigned char>   { typedef short type; };
template <> struct bitwise_restype<short, short>           { typedef short type; };
template <> struct bitwise_restype<short, unsigned short>  { typedef short type; };
template <> struct bitwise_restype<short, int>             { typedef int   type; };
template <> struct bitwise_restype<short, unsigned int>    { typedef int   type; };
template <> struct bitwise_restype<short, float>           { typedef float type; };
template <> struct bitwise_restype<short, double>          { typedef double type; };
template <> struct bitwise_restype<short, long long>       { typedef long long type; };
template <> struct bitwise_restype<short, unsigned long long>           { typedef long long type; };

template <> struct bitwise_restype<unsigned short, char>            { typedef short type; };
template <> struct bitwise_restype<unsigned short, unsigned char>   { typedef short type; };
template <> struct bitwise_restype<unsigned short, short>           { typedef short type; };
template <> struct bitwise_restype<unsigned short, unsigned short>  { typedef short type; };
template <> struct bitwise_restype<unsigned short, int>             { typedef int type; };
template <> struct bitwise_restype<unsigned short, unsigned int>    { typedef int type; };
template <> struct bitwise_restype<unsigned short, float>           { typedef float type; };
template <> struct bitwise_restype<unsigned short, double>          { typedef double type; };
template <> struct bitwise_restype<unsigned short, long long>       { typedef long long type; };
template <> struct bitwise_restype<unsigned short, unsigned long long>           { typedef long long type; };

template <> struct bitwise_restype<int, char>            { typedef int type; };
template <> struct bitwise_restype<int, unsigned char>   { typedef int type; };
template <> struct bitwise_restype<int, short>           { typedef int type; };
template <> struct bitwise_restype<int, unsigned short>  { typedef int type; };
template <> struct bitwise_restype<int, int>             { typedef int type; };
template <> struct bitwise_restype<int, unsigned int>    { typedef int type; };
template <> struct bitwise_restype<int, float>           { typedef float type; };
template <> struct bitwise_restype<int, double>          { typedef double type; };
template <> struct bitwise_restype<int, long long>       { typedef long long type; };
template <> struct bitwise_restype<int, unsigned long long>           { typedef long long type; };

template <> struct bitwise_restype<unsigned int, char>            { typedef int type; };
template <> struct bitwise_restype<unsigned int, unsigned char>   { typedef int type; };
template <> struct bitwise_restype<unsigned int, short>           { typedef int type; };
template <> struct bitwise_restype<unsigned int, unsigned short>  { typedef int type; };
template <> struct bitwise_restype<unsigned int, int>             { typedef int type; };
template <> struct bitwise_restype<unsigned int, unsigned int>    { typedef int type; };
template <> struct bitwise_restype<unsigned int, float>           { typedef float type; };
template <> struct bitwise_restype<unsigned int, double>          { typedef double type; };
template <> struct bitwise_restype<unsigned int, long long>       { typedef long long type; };
template <> struct bitwise_restype<unsigned int, unsigned long long>           { typedef long long type; };

template <> struct bitwise_restype<float, char>            { typedef float type; };
template <> struct bitwise_restype<float, unsigned char>   { typedef float type; };
template <> struct bitwise_restype<float, short>           { typedef float type; };
template <> struct bitwise_restype<float, unsigned short>  { typedef float type; };
template <> struct bitwise_restype<float, int>             { typedef float type; };
template <> struct bitwise_restype<float, unsigned int>    { typedef float type; };
template <> struct bitwise_restype<float, float>           { typedef float type; };
template <> struct bitwise_restype<float, double>          { typedef double type; };
template <> struct bitwise_restype<float, long long>       { typedef float type; };
template <> struct bitwise_restype<float, unsigned long long>           { typedef float type; };

template <> struct bitwise_restype<double, char>            { typedef double type; };
template <> struct bitwise_restype<double, unsigned char>   { typedef double type; };
template <> struct bitwise_restype<double, short>           { typedef double type; };
template <> struct bitwise_restype<double, unsigned short>  { typedef double type; };
template <> struct bitwise_restype<double, int>             { typedef double type; };
template <> struct bitwise_restype<double, unsigned int>    { typedef double type; };
template <> struct bitwise_restype<double, float>           { typedef double type; };
template <> struct bitwise_restype<double, double>          { typedef double type; };
template <> struct bitwise_restype<double, long long>       { typedef double type; };
template <> struct bitwise_restype<double, unsigned long long>           { typedef double type; };

template <> struct bitwise_restype<long long, char>            { typedef long long type; };
template <> struct bitwise_restype<long long, unsigned char>   { typedef long long type; };
template <> struct bitwise_restype<long long, short>           { typedef long long type; };
template <> struct bitwise_restype<long long, unsigned short>  { typedef long long type; };
template <> struct bitwise_restype<long long, int>             { typedef long long type; };
template <> struct bitwise_restype<long long, unsigned int>    { typedef long long type; };
template <> struct bitwise_restype<long long, float>           { typedef float type; };
template <> struct bitwise_restype<long long, double>          { typedef double type; };
template <> struct bitwise_restype<long long, long long>       { typedef long long type; };
template <> struct bitwise_restype<long long, unsigned long long>           { typedef long long type; };

template <> struct bitwise_restype<unsigned long long, char>            { typedef long long type; };
template <> struct bitwise_restype<unsigned long long, unsigned char>   { typedef long long type; };
template <> struct bitwise_restype<unsigned long long, short>           { typedef long long type; };
template <> struct bitwise_restype<unsigned long long, unsigned short>  { typedef long long type; };
template <> struct bitwise_restype<unsigned long long, int>             { typedef long long type; };
template <> struct bitwise_restype<unsigned long long, unsigned int>    { typedef long long type; };
template <> struct bitwise_restype<unsigned long long, float>           { typedef float type; };
template <> struct bitwise_restype<unsigned long long, double>          { typedef double type; };
template <> struct bitwise_restype<unsigned long long, long long>       { typedef long long type; };
template <> struct bitwise_restype<unsigned long long, unsigned long long>           { typedef long long type; };
#else

template <> struct bitwise_restype<char, char>            { typedef char  type; };
template <> struct bitwise_restype<char, unsigned char>   { typedef unsigned char  type; };
template <> struct bitwise_restype<char, short>           { typedef short type; };
template <> struct bitwise_restype<char, unsigned short>  { typedef unsigned short type; };
template <> struct bitwise_restype<char, int>             { typedef int   type; };
template <> struct bitwise_restype<char, unsigned int>    { typedef unsigned int   type; };
template <> struct bitwise_restype<char, float>           { typedef float type; };
template <> struct bitwise_restype<char, double>          { typedef double type; };
template <> struct bitwise_restype<char, long long>       { typedef long long type; };
template <> struct bitwise_restype<char, unsigned long long>           { typedef unsigned long long type; };

template <> struct bitwise_restype<unsigned char, char>            { typedef char type; };
template <> struct bitwise_restype<unsigned char, unsigned char>   { typedef unsigned char type; };
template <> struct bitwise_restype<unsigned char, short>           { typedef short type; };
template <> struct bitwise_restype<unsigned char, unsigned short>  { typedef unsigned short type; };
template <> struct bitwise_restype<unsigned char, int>             { typedef int   type; };
template <> struct bitwise_restype<unsigned char, unsigned int>    { typedef unsigned int   type; };
template <> struct bitwise_restype<unsigned char, float>           { typedef float type; };
template <> struct bitwise_restype<unsigned char, double>          { typedef double type; };
template <> struct bitwise_restype<unsigned char, long long>       { typedef long long type; };
template <> struct bitwise_restype<unsigned char, unsigned long long>           { typedef unsigned long long type; };

template <> struct bitwise_restype<short, char>            { typedef short type; };
template <> struct bitwise_restype<short, unsigned char>   { typedef short type; };
template <> struct bitwise_restype<short, short>           { typedef short type; };
template <> struct bitwise_restype<short, unsigned short>  { typedef unsigned short type; };
template <> struct bitwise_restype<short, int>             { typedef int   type; };
template <> struct bitwise_restype<short, unsigned int>    { typedef unsigned int   type; };
template <> struct bitwise_restype<short, float>           { typedef float type; };
template <> struct bitwise_restype<short, double>          { typedef double type; };
template <> struct bitwise_restype<short, long long>       { typedef long long type; };
template <> struct bitwise_restype<short, unsigned long long>           { typedef unsigned long long type; };

template <> struct bitwise_restype<unsigned short, char>            { typedef unsigned short type; };
template <> struct bitwise_restype<unsigned short, unsigned char>   { typedef unsigned short type; };
template <> struct bitwise_restype<unsigned short, short>           { typedef unsigned short type; };
template <> struct bitwise_restype<unsigned short, unsigned short>  { typedef unsigned short type; };
template <> struct bitwise_restype<unsigned short, int>             { typedef int type; };
template <> struct bitwise_restype<unsigned short, unsigned int>    { typedef unsigned int type; };
template <> struct bitwise_restype<unsigned short, float>           { typedef float type; };
template <> struct bitwise_restype<unsigned short, double>          { typedef double type; };
template <> struct bitwise_restype<unsigned short, long long>       { typedef long long type; };
template <> struct bitwise_restype<unsigned short, unsigned long long>           { typedef unsigned long long type; };

template <> struct bitwise_restype<int, char>            { typedef int type; };
template <> struct bitwise_restype<int, unsigned char>   { typedef int type; };
template <> struct bitwise_restype<int, short>           { typedef int type; };
template <> struct bitwise_restype<int, unsigned short>  { typedef int type; };
template <> struct bitwise_restype<int, int>             { typedef int type; };
template <> struct bitwise_restype<int, unsigned int>    { typedef unsigned int type; };
template <> struct bitwise_restype<int, float>           { typedef float type; };
template <> struct bitwise_restype<int, double>          { typedef double type; };
template <> struct bitwise_restype<int, long long>       { typedef long long type; };
template <> struct bitwise_restype<int, unsigned long long>           { typedef unsigned long long type; };

template <> struct bitwise_restype<unsigned int, char>            { typedef unsigned int type; };
template <> struct bitwise_restype<unsigned int, unsigned char>   { typedef unsigned int type; };
template <> struct bitwise_restype<unsigned int, short>           { typedef unsigned int type; };
template <> struct bitwise_restype<unsigned int, unsigned short>  { typedef unsigned int type; };
template <> struct bitwise_restype<unsigned int, int>             { typedef unsigned int type; };
template <> struct bitwise_restype<unsigned int, unsigned int>    { typedef unsigned int type; };
template <> struct bitwise_restype<unsigned int, float>           { typedef float type; };
template <> struct bitwise_restype<unsigned int, double>          { typedef double type; };
template <> struct bitwise_restype<unsigned int, long long>       { typedef long long type; };
template <> struct bitwise_restype<unsigned int, unsigned long long>           { typedef unsigned long long type; };

template <> struct bitwise_restype<float, char>            { typedef float type; };
template <> struct bitwise_restype<float, unsigned char>   { typedef float type; };
template <> struct bitwise_restype<float, short>           { typedef float type; };
template <> struct bitwise_restype<float, unsigned short>  { typedef float type; };
template <> struct bitwise_restype<float, int>             { typedef float type; };
template <> struct bitwise_restype<float, unsigned int>    { typedef float type; };
template <> struct bitwise_restype<float, float>           { typedef float type; };
template <> struct bitwise_restype<float, double>          { typedef double type; };
template <> struct bitwise_restype<float, long long>       { typedef float type; };
template <> struct bitwise_restype<float, unsigned long long>           { typedef float type; };

template <> struct bitwise_restype<double, char>            { typedef double type; };
template <> struct bitwise_restype<double, unsigned char>   { typedef double type; };
template <> struct bitwise_restype<double, short>           { typedef double type; };
template <> struct bitwise_restype<double, unsigned short>  { typedef double type; };
template <> struct bitwise_restype<double, int>             { typedef double type; };
template <> struct bitwise_restype<double, unsigned int>    { typedef double type; };
template <> struct bitwise_restype<double, float>           { typedef double type; };
template <> struct bitwise_restype<double, double>          { typedef double type; };
template <> struct bitwise_restype<double, long long>       { typedef double type; };
template <> struct bitwise_restype<double, unsigned long long>           { typedef double type; };

template <> struct bitwise_restype<long long, char>            { typedef long long type; };
template <> struct bitwise_restype<long long, unsigned char>   { typedef long long type; };
template <> struct bitwise_restype<long long, short>           { typedef long long type; };
template <> struct bitwise_restype<long long, unsigned short>  { typedef long long type; };
template <> struct bitwise_restype<long long, int>             { typedef long long type; };
template <> struct bitwise_restype<long long, unsigned int>    { typedef long long type; };
template <> struct bitwise_restype<long long, float>           { typedef float type; };
template <> struct bitwise_restype<long long, double>          { typedef double type; };
template <> struct bitwise_restype<long long, long long>       { typedef long long type; };
template <> struct bitwise_restype<long long, unsigned long long>           { typedef unsigned long long type; };

template <> struct bitwise_restype<unsigned long long, char>            { typedef unsigned long long type; };
template <> struct bitwise_restype<unsigned long long, unsigned char>   { typedef unsigned long long type; };
template <> struct bitwise_restype<unsigned long long, short>           { typedef unsigned long long type; };
template <> struct bitwise_restype<unsigned long long, unsigned short>  { typedef unsigned long long type; };
template <> struct bitwise_restype<unsigned long long, int>             { typedef unsigned long long type; };
template <> struct bitwise_restype<unsigned long long, unsigned int>    { typedef unsigned long long type; };
template <> struct bitwise_restype<unsigned long long, float>           { typedef float type; };
template <> struct bitwise_restype<unsigned long long, double>          { typedef double type; };
template <> struct bitwise_restype<unsigned long long, long long>       { typedef unsigned long long type; };
template <> struct bitwise_restype<unsigned long long, unsigned long long>           { typedef unsigned long long type; };

#endif

template <typename T1, typename T2> struct restype_ex {
private:
    restype_ex();
};
//#ifdef CM_NONSTRICT
#if 0
template <> struct restype_ex<char, char>            { typedef short  type; };
template <> struct restype_ex<char, unsigned char>   { typedef short  type; };
template <> struct restype_ex<char, short>           { typedef short type; };
template <> struct restype_ex<char, unsigned short>  { typedef short type; };
template <> struct restype_ex<char, int>             { typedef int   type; };
template <> struct restype_ex<char, unsigned int>    { typedef int   type; };
template <> struct restype_ex<char, float>           { typedef float type; };
template <> struct restype_ex<char, double>          { typedef double type; };

template <> struct restype_ex<unsigned char, char>            { typedef short  type; };
template <> struct restype_ex<unsigned char, unsigned char>   { typedef short  type; };
template <> struct restype_ex<unsigned char, short>           { typedef short type; };
template <> struct restype_ex<unsigned char, unsigned short>  { typedef short type; };
template <> struct restype_ex<unsigned char, int>             { typedef int   type; };
template <> struct restype_ex<unsigned char, unsigned int>    { typedef int   type; };
template <> struct restype_ex<unsigned char, float>           { typedef float type; };
template <> struct restype_ex<unsigned char, double>          { typedef double type; };

template <> struct restype_ex<short, char>            { typedef short type; };
template <> struct restype_ex<short, unsigned char>   { typedef short type; };
template <> struct restype_ex<short, short>           { typedef short type; };
template <> struct restype_ex<short, unsigned short>  { typedef short type; };
template <> struct restype_ex<short, int>             { typedef int   type; };
template <> struct restype_ex<short, unsigned int>    { typedef int   type; };
template <> struct restype_ex<short, float>           { typedef float type; };
template <> struct restype_ex<short, double>          { typedef double type; };

template <> struct restype_ex<unsigned short, char>            { typedef short type; };
template <> struct restype_ex<unsigned short, unsigned char>   { typedef short type; };
template <> struct restype_ex<unsigned short, short>           { typedef short type; };
template <> struct restype_ex<unsigned short, unsigned short>  { typedef short type; };
template <> struct restype_ex<unsigned short, int>             { typedef int type; };
template <> struct restype_ex<unsigned short, unsigned int>    { typedef int type; };
template <> struct restype_ex<unsigned short, float>           { typedef float type; };
template <> struct restype_ex<unsigned short, double>          { typedef double type; };

template <> struct restype_ex<int, char>            { typedef int type; };
template <> struct restype_ex<int, unsigned char>   { typedef int type; };
template <> struct restype_ex<int, short>           { typedef int type; };
template <> struct restype_ex<int, unsigned short>  { typedef int type; };
template <> struct restype_ex<int, int>             { typedef int type; };
template <> struct restype_ex<int, unsigned int>    { typedef int type; };
template <> struct restype_ex<int, float>           { typedef float type; };
template <> struct restype_ex<int, double>          { typedef double type; };

template <> struct restype_ex<unsigned int, char>            { typedef int type; };
template <> struct restype_ex<unsigned int, unsigned char>   { typedef int type; };
template <> struct restype_ex<unsigned int, short>           { typedef int type; };
template <> struct restype_ex<unsigned int, unsigned short>  { typedef int type; };
template <> struct restype_ex<unsigned int, int>             { typedef int type; };
template <> struct restype_ex<unsigned int, unsigned int>    { typedef int type; };
template <> struct restype_ex<unsigned int, float>           { typedef float type; };
template <> struct restype_ex<unsigned int, double>          { typedef double type; };

template <> struct restype_ex<float, char>            { typedef float type; };
template <> struct restype_ex<float, unsigned char>   { typedef float type; };
template <> struct restype_ex<float, short>           { typedef float type; };
template <> struct restype_ex<float, unsigned short>  { typedef float type; };
template <> struct restype_ex<float, int>             { typedef float type; };
template <> struct restype_ex<float, unsigned int>    { typedef float type; };
template <> struct restype_ex<float, float>           { typedef float type; };
template <> struct restype_ex<float, double>          { typedef double type; };

template <> struct restype_ex<double, char>            { typedef double type; };
template <> struct restype_ex<double, unsigned char>   { typedef double type; };
template <> struct restype_ex<double, short>           { typedef double type; };
template <> struct restype_ex<double, unsigned short>  { typedef double type; };
template <> struct restype_ex<double, int>             { typedef double type; };
template <> struct restype_ex<double, unsigned int>    { typedef double type; };
template <> struct restype_ex<double, float>           { typedef double type; };
template <> struct restype_ex<double, double>          { typedef double type; };

template <> struct restype_ex<long long, char>            { typedef long long type; };
template <> struct restype_ex<long long, unsigned char>   { typedef long long type; };
template <> struct restype_ex<long long, short>           { typedef long long type; };
template <> struct restype_ex<long long, unsigned short>  { typedef long long type; };
template <> struct restype_ex<long long, int>             { typedef long long type; };
template <> struct restype_ex<long long, unsigned int>    { typedef long long type; };
template <> struct restype_ex<long long, float>           { typedef float type; };
template <> struct restype_ex<long long, double>          { typedef double type; };
template <> struct restype_ex<long long, long long>       { typedef long long type; };
template <> struct restype_ex<long long, unsigned long long>           { typedef long long type; };

template <> struct restype_ex<unsigned long long, char>            { typedef long long type; };
template <> struct restype_ex<unsigned long long, unsigned char>   { typedef long long type; };
template <> struct restype_ex<unsigned long long, short>           { typedef long long type; };
template <> struct restype_ex<unsigned long long, unsigned short>  { typedef long long type; };
template <> struct restype_ex<unsigned long long, int>             { typedef long long type; };
template <> struct restype_ex<unsigned long long, unsigned int>    { typedef long long type; };
template <> struct restype_ex<unsigned long long, float>           { typedef float type; };
template <> struct restype_ex<unsigned long long, double>          { typedef double type; };
template <> struct restype_ex<unsigned long long, long long>       { typedef long long type; };
template <> struct restype_ex<unsigned long long, unsigned long long>           { typedef long long type; };

template <typename T> struct maxtype;
template<> struct maxtype<float> { typedef float type; };
template<> struct maxtype<char> { typedef char type; };
template<> struct maxtype<short> { typedef short type; };
template<> struct maxtype<int> { typedef int type; };
template<> struct maxtype<uchar> { typedef uchar type; };
template<> struct maxtype<ushort> { typedef ushort type; };
template<> struct maxtype<uint> { typedef uint type; };
template<> struct maxtype<double> { typedef double type; };
template<> struct maxtype<long long> { typedef long long type; };
template<> struct maxtype<unsigned long long> { typedef unsigned long long type; };

#else

template <> struct restype_ex<char, char>            { typedef int  type; };
template <> struct restype_ex<char, unsigned char>   { typedef int  type; };
template <> struct restype_ex<char, short>           { typedef int type; };
template <> struct restype_ex<char, unsigned short>  { typedef int type; };
template <> struct restype_ex<char, int>             { typedef long long   type; };
template <> struct restype_ex<char, unsigned int>    { typedef long long   type; };
template <> struct restype_ex<char, float>           { typedef float type; };
template <> struct restype_ex<char, double>           { typedef double type; };

template <> struct restype_ex<unsigned char, char>            { typedef int  type; };
template <> struct restype_ex<unsigned char, unsigned char>   { typedef int  type; };
template <> struct restype_ex<unsigned char, short>           { typedef int type; };
template <> struct restype_ex<unsigned char, unsigned short>  { typedef int type; };
template <> struct restype_ex<unsigned char, int>             { typedef long long   type; };
template <> struct restype_ex<unsigned char, unsigned int>    { typedef long long   type; };
template <> struct restype_ex<unsigned char, float>           { typedef float type; };
template <> struct restype_ex<unsigned char, double>          { typedef double type; };
template <> struct restype_ex<unsigned char, long long>       { typedef long long type; };
template <> struct restype_ex<unsigned char, unsigned long long>           { typedef long long type; };

template <> struct restype_ex<short, char>            { typedef int type; };
template <> struct restype_ex<short, unsigned char>   { typedef int type; };
template <> struct restype_ex<short, short>           { typedef int type; };
template <> struct restype_ex<short, unsigned short>  { typedef int type; };
template <> struct restype_ex<short, int>             { typedef long long   type; };
template <> struct restype_ex<short, unsigned int>    { typedef long long   type; };
template <> struct restype_ex<short, float>           { typedef float type; };
template <> struct restype_ex<short, double>          { typedef double type; };
template <> struct restype_ex<short, long long>       { typedef long long type; };
template <> struct restype_ex<short, unsigned long long>           { typedef long long type; };

template <> struct restype_ex<unsigned short, char>            { typedef int type; };
template <> struct restype_ex<unsigned short, unsigned char>   { typedef int type; };
template <> struct restype_ex<unsigned short, short>           { typedef int type; };
template <> struct restype_ex<unsigned short, unsigned short>  { typedef int type; };
template <> struct restype_ex<unsigned short, int>             { typedef long long type; };
template <> struct restype_ex<unsigned short, unsigned int>    { typedef long long type; };
template <> struct restype_ex<unsigned short, float>           { typedef float type; };
template <> struct restype_ex<unsigned short, double>          { typedef double type; };
template <> struct restype_ex<unsigned short, long long>       { typedef long long type; };
template <> struct restype_ex<unsigned short, unsigned long long>           { typedef long long type; };

template <> struct restype_ex<int, char>            { typedef long long type; };
template <> struct restype_ex<int, unsigned char>   { typedef long long type; };
template <> struct restype_ex<int, short>           { typedef long long type; };
template <> struct restype_ex<int, unsigned short>  { typedef long long type; };
template <> struct restype_ex<int, int>             { typedef long long type; };
template <> struct restype_ex<int, unsigned int>    { typedef long long type; };
template <> struct restype_ex<int, float>           { typedef float type; };
template <> struct restype_ex<int, double>          { typedef double type; };
template <> struct restype_ex<int, long long>       { typedef long long type; };
template <> struct restype_ex<int, unsigned long long>           { typedef long long type; };

template <> struct restype_ex<unsigned int, char>            { typedef long long type; };
template <> struct restype_ex<unsigned int, unsigned char>   { typedef long long type; };
template <> struct restype_ex<unsigned int, short>           { typedef long long type; };
template <> struct restype_ex<unsigned int, unsigned short>  { typedef long long type; };
template <> struct restype_ex<unsigned int, int>             { typedef long long type; };
template <> struct restype_ex<unsigned int, unsigned int>    { typedef long long type; };
template <> struct restype_ex<unsigned int, float>           { typedef float type; };
template <> struct restype_ex<unsigned int, double>          { typedef double type; };
template <> struct restype_ex<unsigned int, long long>       { typedef long long type; };
template <> struct restype_ex<unsigned int, unsigned long long>           { typedef long long type; };

template <> struct restype_ex<float, char>            { typedef float type; };
template <> struct restype_ex<float, unsigned char>   { typedef float type; };
template <> struct restype_ex<float, short>           { typedef float type; };
template <> struct restype_ex<float, unsigned short>  { typedef float type; };
template <> struct restype_ex<float, int>             { typedef float type; };
template <> struct restype_ex<float, unsigned int>    { typedef float type; };
template <> struct restype_ex<float, float>           { typedef float type; };
template <> struct restype_ex<float, double>          { typedef double type; };
template <> struct restype_ex<float, long long>       { typedef float type; };
template <> struct restype_ex<float, unsigned long long>           { typedef float type; };

template <> struct restype_ex<double, char>            { typedef double type; };
template <> struct restype_ex<double, unsigned char>   { typedef double type; };
template <> struct restype_ex<double, short>           { typedef double type; };
template <> struct restype_ex<double, unsigned short>  { typedef double type; };
template <> struct restype_ex<double, int>             { typedef double type; };
template <> struct restype_ex<double, unsigned int>    { typedef double type; };
template <> struct restype_ex<double, float>           { typedef double type; };
template <> struct restype_ex<double, double>          { typedef double type; };
template <> struct restype_ex<double, long long>       { typedef double type; };
template <> struct restype_ex<double, unsigned long long>           { typedef double type; };

template <> struct restype_ex<long long, char>            { typedef long long type; };
template <> struct restype_ex<long long, unsigned char>   { typedef long long type; };
template <> struct restype_ex<long long, short>           { typedef long long type; };
template <> struct restype_ex<long long, unsigned short>  { typedef long long type; };
template <> struct restype_ex<long long, int>             { typedef long long type; };
template <> struct restype_ex<long long, unsigned int>    { typedef long long type; };
template <> struct restype_ex<long long, float>           { typedef float type; };
template <> struct restype_ex<long long, double>          { typedef double type; };
template <> struct restype_ex<long long, long long>       { typedef long long type; };
template <> struct restype_ex<long long, unsigned long long>           { typedef long long type; };

template <> struct restype_ex<unsigned long long, char>            { typedef long long type; };
template <> struct restype_ex<unsigned long long, unsigned char>   { typedef long long type; };
template <> struct restype_ex<unsigned long long, short>           { typedef long long type; };
template <> struct restype_ex<unsigned long long, unsigned short>  { typedef long long type; };
template <> struct restype_ex<unsigned long long, int>             { typedef long long type; };
template <> struct restype_ex<unsigned long long, unsigned int>    { typedef long long type; };
template <> struct restype_ex<unsigned long long, float>           { typedef float type; };
template <> struct restype_ex<unsigned long long, double>          { typedef double type; };
template <> struct restype_ex<unsigned long long, long long>       { typedef long long type; };
template <> struct restype_ex<unsigned long long, unsigned long long>           { typedef long long type; };

template <typename T> struct maxtype;
template<> struct maxtype<float>       { typedef float type; };
template<> struct maxtype<char>        { typedef int type; };
template<> struct maxtype<short>       { typedef int type; };
template<> struct maxtype<int>         { typedef int type; };
template<> struct maxtype<uchar>       { typedef uint type; };
template<> struct maxtype<ushort>      { typedef uint type; };
template<> struct maxtype<uint>        { typedef uint type; };
template<> struct maxtype<double>      { typedef double type; };
template<> struct maxtype<long long>   { typedef long long type; };
template<> struct maxtype<unsigned long long>       { typedef unsigned long long type; };

#endif

template <typename T1, typename T2> struct uchar_type {
private:
        uchar_type();
};
template <> struct uchar_type<char, char>            { typedef uchar type; };
template <> struct uchar_type<char, unsigned char>   { typedef uchar type; };
template <> struct uchar_type<char, short>           { typedef uchar type; };
template <> struct uchar_type<char, unsigned short>  { typedef uchar type; };
template <> struct uchar_type<char, int>             { typedef uchar type; };
template <> struct uchar_type<char, unsigned int>    { typedef uchar type; };
template <> struct uchar_type<char, float>           { typedef uchar type; };
template <> struct uchar_type<char, double>          { typedef uchar type; };
template <> struct uchar_type<char, long long>       { typedef uchar type; };
template <> struct uchar_type<char, unsigned long long>           { typedef uchar type; };

template <> struct uchar_type<unsigned char, char>            { typedef uchar type; };
template <> struct uchar_type<unsigned char, unsigned char>   { typedef uchar type; };
template <> struct uchar_type<unsigned char, short>           { typedef uchar type; };
template <> struct uchar_type<unsigned char, unsigned short>  { typedef uchar type; };
template <> struct uchar_type<unsigned char, int>             { typedef uchar type; };
template <> struct uchar_type<unsigned char, unsigned int>    { typedef uchar type; };
template <> struct uchar_type<unsigned char, float>           { typedef uchar type; };
template <> struct uchar_type<unsigned char, double>          { typedef uchar type; };
template <> struct uchar_type<unsigned char, long long>       { typedef uchar type; };
template <> struct uchar_type<unsigned char, unsigned long long>           { typedef uchar type; };

template <> struct uchar_type<short, char>            { typedef uchar type; };
template <> struct uchar_type<short, unsigned char>   { typedef uchar type; };
template <> struct uchar_type<short, short>           { typedef uchar type; };
template <> struct uchar_type<short, unsigned short>  { typedef uchar type; };
template <> struct uchar_type<short, int>             { typedef uchar type; };
template <> struct uchar_type<short, unsigned int>    { typedef uchar type; };
template <> struct uchar_type<short, float>           { typedef uchar type; };
template <> struct uchar_type<short, double>          { typedef uchar type; };
template <> struct uchar_type<short, long long>       { typedef uchar type; };
template <> struct uchar_type<short, unsigned long long>           { typedef uchar type; };

template <> struct uchar_type<unsigned short, char>            { typedef uchar type; };
template <> struct uchar_type<unsigned short, unsigned char>   { typedef uchar type; };
template <> struct uchar_type<unsigned short, short>           { typedef uchar type; };
template <> struct uchar_type<unsigned short, unsigned short>  { typedef uchar type; };
template <> struct uchar_type<unsigned short, int>             { typedef uchar type; };
template <> struct uchar_type<unsigned short, unsigned int>    { typedef uchar type; };
template <> struct uchar_type<unsigned short, float>           { typedef uchar type; };
template <> struct uchar_type<unsigned short, double>          { typedef uchar type; };
template <> struct uchar_type<unsigned short, long long>       { typedef uchar type; };
template <> struct uchar_type<unsigned short, unsigned long long>           { typedef uchar type; };

template <> struct uchar_type<int, char>            { typedef uchar type; };
template <> struct uchar_type<int, unsigned char>   { typedef uchar type; };
template <> struct uchar_type<int, short>           { typedef uchar type; };
template <> struct uchar_type<int, unsigned short>  { typedef uchar type; };
template <> struct uchar_type<int, int>             { typedef uchar type; };
template <> struct uchar_type<int, unsigned int>    { typedef uchar type; };
template <> struct uchar_type<int, float>           { typedef uchar type; };
template <> struct uchar_type<int, double>          { typedef uchar type; };
template <> struct uchar_type<int, long long>       { typedef uchar type; };
template <> struct uchar_type<int, unsigned long long>           { typedef uchar type; };

template <> struct uchar_type<unsigned int, char>            { typedef uchar type; };
template <> struct uchar_type<unsigned int, unsigned char>   { typedef uchar type; };
template <> struct uchar_type<unsigned int, short>           { typedef uchar type; };
template <> struct uchar_type<unsigned int, unsigned short>  { typedef uchar type; };
template <> struct uchar_type<unsigned int, int>             { typedef uchar type; };
template <> struct uchar_type<unsigned int, unsigned int>    { typedef uchar type; };
template <> struct uchar_type<unsigned int, float>           { typedef uchar type; };
template <> struct uchar_type<unsigned int, double>          { typedef uchar type; };
template <> struct uchar_type<unsigned int, long long>       { typedef uchar type; };
template <> struct uchar_type<unsigned int, unsigned long long>           { typedef uchar type; };

template <> struct uchar_type<float, char>            { typedef uchar type; };
template <> struct uchar_type<float, unsigned char>   { typedef uchar type; };
template <> struct uchar_type<float, short>           { typedef uchar type; };
template <> struct uchar_type<float, unsigned short>  { typedef uchar type; };
template <> struct uchar_type<float, int>             { typedef uchar type; };
template <> struct uchar_type<float, unsigned int>    { typedef uchar type; };
template <> struct uchar_type<float, float>           { typedef uchar type; };
template <> struct uchar_type<float, double>          { typedef uchar type; };
template <> struct uchar_type<float, long long>       { typedef uchar type; };
template <> struct uchar_type<float, unsigned long long>           { typedef uchar type; };

template <> struct uchar_type<double, char>            { typedef uchar type; };
template <> struct uchar_type<double, unsigned char>   { typedef uchar type; };
template <> struct uchar_type<double, short>           { typedef uchar type; };
template <> struct uchar_type<double, unsigned short>  { typedef uchar type; };
template <> struct uchar_type<double, int>             { typedef uchar type; };
template <> struct uchar_type<double, unsigned int>    { typedef uchar type; };
template <> struct uchar_type<double, float>           { typedef uchar type; };
template <> struct uchar_type<double, double>          { typedef uchar type; };
template <> struct uchar_type<double, long long>       { typedef uchar type; };
template <> struct uchar_type<double, unsigned long long>           { typedef uchar type; };

template <> struct uchar_type<long long, char>            { typedef uchar type; };
template <> struct uchar_type<long long, unsigned char>   { typedef uchar type; };
template <> struct uchar_type<long long, short>           { typedef uchar type; };
template <> struct uchar_type<long long, unsigned short>  { typedef uchar type; };
template <> struct uchar_type<long long, int>             { typedef uchar type; };
template <> struct uchar_type<long long, unsigned int>    { typedef uchar type; };
template <> struct uchar_type<long long, float>           { typedef uchar type; };
template <> struct uchar_type<long long, double>          { typedef uchar type; };
template <> struct uchar_type<long long, long long>       { typedef uchar type; };
template <> struct uchar_type<long long, unsigned long long>           { typedef uchar type; };

template <> struct uchar_type<unsigned long long, char>            { typedef uchar type; };
template <> struct uchar_type<unsigned long long, unsigned char>   { typedef uchar type; };
template <> struct uchar_type<unsigned long long, short>           { typedef uchar type; };
template <> struct uchar_type<unsigned long long, unsigned short>  { typedef uchar type; };
template <> struct uchar_type<unsigned long long, int>             { typedef uchar type; };
template <> struct uchar_type<unsigned long long, unsigned int>    { typedef uchar type; };
template <> struct uchar_type<unsigned long long, float>           { typedef uchar type; };
template <> struct uchar_type<unsigned long long, double>          { typedef uchar type; };
template <> struct uchar_type<unsigned long long, long long>       { typedef uchar type; };
template <> struct uchar_type<unsigned long long, unsigned long long>           { typedef uchar type; };

template <typename T1, typename T2> struct ushort_type {
private:
        ushort_type();
};
template <> struct ushort_type<char, char>            { typedef ushort type; };
template <> struct ushort_type<char, unsigned char>   { typedef ushort type; };
template <> struct ushort_type<char, short>           { typedef ushort type; };
template <> struct ushort_type<char, unsigned short>  { typedef ushort type; };
template <> struct ushort_type<char, int>             { typedef ushort type; };
template <> struct ushort_type<char, unsigned int>    { typedef ushort type; };
template <> struct ushort_type<char, float>           { typedef ushort type; };
template <> struct ushort_type<char, double>          { typedef ushort type; };
template <> struct ushort_type<char, long long>       { typedef ushort type; };
template <> struct ushort_type<char, unsigned long long>           { typedef ushort type; };

template <> struct ushort_type<unsigned char, char>            { typedef ushort type; };
template <> struct ushort_type<unsigned char, unsigned char>   { typedef ushort type; };
template <> struct ushort_type<unsigned char, short>           { typedef ushort type; };
template <> struct ushort_type<unsigned char, unsigned short>  { typedef ushort type; };
template <> struct ushort_type<unsigned char, int>             { typedef ushort type; };
template <> struct ushort_type<unsigned char, unsigned int>    { typedef ushort type; };
template <> struct ushort_type<unsigned char, float>           { typedef ushort type; };
template <> struct ushort_type<unsigned char, double>          { typedef ushort type; };
template <> struct ushort_type<unsigned char, long long>       { typedef ushort type; };
template <> struct ushort_type<unsigned char, unsigned long long>           { typedef ushort type; };

template <> struct ushort_type<short, char>            { typedef ushort type; };
template <> struct ushort_type<short, unsigned char>   { typedef ushort type; };
template <> struct ushort_type<short, short>           { typedef ushort type; };
template <> struct ushort_type<short, unsigned short>  { typedef ushort type; };
template <> struct ushort_type<short, int>             { typedef ushort type; };
template <> struct ushort_type<short, unsigned int>    { typedef ushort type; };
template <> struct ushort_type<short, float>           { typedef ushort type; };
template <> struct ushort_type<short, double>          { typedef ushort type; };
template <> struct ushort_type<short, long long>       { typedef ushort type; };
template <> struct ushort_type<short, unsigned long long>           { typedef ushort type; };

template <> struct ushort_type<unsigned short, char>            { typedef ushort type; };
template <> struct ushort_type<unsigned short, unsigned char>   { typedef ushort type; };
template <> struct ushort_type<unsigned short, short>           { typedef ushort type; };
template <> struct ushort_type<unsigned short, unsigned short>  { typedef ushort type; };
template <> struct ushort_type<unsigned short, int>             { typedef ushort type; };
template <> struct ushort_type<unsigned short, unsigned int>    { typedef ushort type; };
template <> struct ushort_type<unsigned short, float>           { typedef ushort type; };
template <> struct ushort_type<unsigned short, double>          { typedef ushort type; };
template <> struct ushort_type<unsigned short, long long>       { typedef ushort type; };
template <> struct ushort_type<unsigned short, unsigned long long>           { typedef ushort type; };

template <> struct ushort_type<int, char>            { typedef ushort type; };
template <> struct ushort_type<int, unsigned char>   { typedef ushort type; };
template <> struct ushort_type<int, short>           { typedef ushort type; };
template <> struct ushort_type<int, unsigned short>  { typedef ushort type; };
template <> struct ushort_type<int, int>             { typedef ushort type; };
template <> struct ushort_type<int, unsigned int>    { typedef ushort type; };
template <> struct ushort_type<int, float>           { typedef ushort type; };
template <> struct ushort_type<int, double>          { typedef ushort type; };
template <> struct ushort_type<int, long long>       { typedef ushort type; };
template <> struct ushort_type<int, unsigned long long>           { typedef ushort type; };

template <> struct ushort_type<unsigned int, char>            { typedef ushort type; };
template <> struct ushort_type<unsigned int, unsigned char>   { typedef ushort type; };
template <> struct ushort_type<unsigned int, short>           { typedef ushort type; };
template <> struct ushort_type<unsigned int, unsigned short>  { typedef ushort type; };
template <> struct ushort_type<unsigned int, int>             { typedef ushort type; };
template <> struct ushort_type<unsigned int, unsigned int>    { typedef ushort type; };
template <> struct ushort_type<unsigned int, float>           { typedef ushort type; };
template <> struct ushort_type<unsigned int, double>          { typedef ushort type; };
template <> struct ushort_type<unsigned int, long long>       { typedef ushort type; };
template <> struct ushort_type<unsigned int, unsigned long long>           { typedef ushort type; };

template <> struct ushort_type<float, char>            { typedef ushort type; };
template <> struct ushort_type<float, unsigned char>   { typedef ushort type; };
template <> struct ushort_type<float, short>           { typedef ushort type; };
template <> struct ushort_type<float, unsigned short>  { typedef ushort type; };
template <> struct ushort_type<float, int>             { typedef ushort type; };
template <> struct ushort_type<float, unsigned int>    { typedef ushort type; };
template <> struct ushort_type<float, float>           { typedef ushort type; };
template <> struct ushort_type<float, double>          { typedef ushort type; };
template <> struct ushort_type<float, long long>       { typedef ushort type; };
template <> struct ushort_type<float, unsigned long long>           { typedef ushort type; };

template <> struct ushort_type<double, char>            { typedef ushort type; };
template <> struct ushort_type<double, unsigned char>   { typedef ushort type; };
template <> struct ushort_type<double, short>           { typedef ushort type; };
template <> struct ushort_type<double, unsigned short>  { typedef ushort type; };
template <> struct ushort_type<double, int>             { typedef ushort type; };
template <> struct ushort_type<double, unsigned int>    { typedef ushort type; };
template <> struct ushort_type<double, float>           { typedef ushort type; };
template <> struct ushort_type<double, double>          { typedef ushort type; };
template <> struct ushort_type<double, long long>       { typedef ushort type; };
template <> struct ushort_type<double, unsigned long long>           { typedef ushort type; };

template <> struct ushort_type<long long, char>            { typedef ushort type; };
template <> struct ushort_type<long long, unsigned char>   { typedef ushort type; };
template <> struct ushort_type<long long, short>           { typedef ushort type; };
template <> struct ushort_type<long long, unsigned short>  { typedef ushort type; };
template <> struct ushort_type<long long, int>             { typedef ushort type; };
template <> struct ushort_type<long long, unsigned int>    { typedef ushort type; };
template <> struct ushort_type<long long, float>           { typedef ushort type; };
template <> struct ushort_type<long long, double>          { typedef ushort type; };
template <> struct ushort_type<long long, long long>       { typedef ushort type; };
template <> struct ushort_type<long long, unsigned long long>           { typedef ushort type; };

template <> struct ushort_type<unsigned long long, char>            { typedef ushort type; };
template <> struct ushort_type<unsigned long long, unsigned char>   { typedef ushort type; };
template <> struct ushort_type<unsigned long long, short>           { typedef ushort type; };
template <> struct ushort_type<unsigned long long, unsigned short>  { typedef ushort type; };
template <> struct ushort_type<unsigned long long, int>             { typedef ushort type; };
template <> struct ushort_type<unsigned long long, unsigned int>    { typedef ushort type; };
template <> struct ushort_type<unsigned long long, float>           { typedef ushort type; };
template <> struct ushort_type<unsigned long long, double>          { typedef ushort type; };
template <> struct ushort_type<unsigned long long, long long>       { typedef ushort type; };
template <> struct ushort_type<unsigned long long, unsigned long long>           { typedef ushort type; };

template <typename T1, typename T2> struct uint_type {
private:
        uint_type();
};
template <> struct uint_type<char, char>            { typedef uint type; };
template <> struct uint_type<char, unsigned char>   { typedef uint type; };
template <> struct uint_type<char, short>           { typedef uint type; };
template <> struct uint_type<char, unsigned short>  { typedef uint type; };
template <> struct uint_type<char, int>             { typedef uint type; };
template <> struct uint_type<char, unsigned int>    { typedef uint type; };
template <> struct uint_type<char, float>           { typedef uint type; };
template <> struct uint_type<char, double>          { typedef uint type; };
template <> struct uint_type<char, long long>       { typedef uint type; };
template <> struct uint_type<char, unsigned long long>           { typedef uint type; };

template <> struct uint_type<unsigned char, char>            { typedef uint type; };
template <> struct uint_type<unsigned char, unsigned char>   { typedef uint type; };
template <> struct uint_type<unsigned char, short>           { typedef uint type; };
template <> struct uint_type<unsigned char, unsigned short>  { typedef uint type; };
template <> struct uint_type<unsigned char, int>             { typedef uint type; };
template <> struct uint_type<unsigned char, unsigned int>    { typedef uint type; };
template <> struct uint_type<unsigned char, float>           { typedef uint type; };
template <> struct uint_type<unsigned char, double>          { typedef uint type; };
template <> struct uint_type<unsigned char, long long>       { typedef uint type; };
template <> struct uint_type<unsigned char, unsigned long long>           { typedef uint type; };

template <> struct uint_type<short, char>            { typedef uint type; };
template <> struct uint_type<short, unsigned char>   { typedef uint type; };
template <> struct uint_type<short, short>           { typedef uint type; };
template <> struct uint_type<short, unsigned short>  { typedef uint type; };
template <> struct uint_type<short, int>             { typedef uint type; };
template <> struct uint_type<short, unsigned int>    { typedef uint type; };
template <> struct uint_type<short, float>           { typedef uint type; };
template <> struct uint_type<short, double>          { typedef uint type; };
template <> struct uint_type<short, long long>       { typedef uint type; };
template <> struct uint_type<short, unsigned long long>           { typedef uint type; };

template <> struct uint_type<unsigned short, char>            { typedef uint type; };
template <> struct uint_type<unsigned short, unsigned char>   { typedef uint type; };
template <> struct uint_type<unsigned short, short>           { typedef uint type; };
template <> struct uint_type<unsigned short, unsigned short>  { typedef uint type; };
template <> struct uint_type<unsigned short, int>             { typedef uint type; };
template <> struct uint_type<unsigned short, unsigned int>    { typedef uint type; };
template <> struct uint_type<unsigned short, float>           { typedef uint type; };
template <> struct uint_type<unsigned short, double>          { typedef uint type; };
template <> struct uint_type<unsigned short, long long>       { typedef uint type; };
template <> struct uint_type<unsigned short, unsigned long long>           { typedef uint type; };

template <> struct uint_type<int, char>            { typedef uint type; };
template <> struct uint_type<int, unsigned char>   { typedef uint type; };
template <> struct uint_type<int, short>           { typedef uint type; };
template <> struct uint_type<int, unsigned short>  { typedef uint type; };
template <> struct uint_type<int, int>             { typedef uint type; };
template <> struct uint_type<int, unsigned int>    { typedef uint type; };
template <> struct uint_type<int, float>           { typedef uint type; };
template <> struct uint_type<int, double>          { typedef uint type; };
template <> struct uint_type<int, long long>       { typedef uint type; };
template <> struct uint_type<int, unsigned long long>           { typedef uint type; };

template <> struct uint_type<unsigned int, char>            { typedef uint type; };
template <> struct uint_type<unsigned int, unsigned char>   { typedef uint type; };
template <> struct uint_type<unsigned int, short>           { typedef uint type; };
template <> struct uint_type<unsigned int, unsigned short>  { typedef uint type; };
template <> struct uint_type<unsigned int, int>             { typedef uint type; };
template <> struct uint_type<unsigned int, unsigned int>    { typedef uint type; };
template <> struct uint_type<unsigned int, float>           { typedef uint type; };
template <> struct uint_type<unsigned int, double>          { typedef uint type; };
template <> struct uint_type<unsigned int, long long>       { typedef uint type; };
template <> struct uint_type<unsigned int, unsigned long long>           { typedef uint type; };

template <> struct uint_type<float, char>            { typedef uint type; };
template <> struct uint_type<float, unsigned char>   { typedef uint type; };
template <> struct uint_type<float, short>           { typedef uint type; };
template <> struct uint_type<float, unsigned short>  { typedef uint type; };
template <> struct uint_type<float, int>             { typedef uint type; };
template <> struct uint_type<float, unsigned int>    { typedef uint type; };
template <> struct uint_type<float, float>           { typedef uint type; };
template <> struct uint_type<float, double>          { typedef uint type; };
template <> struct uint_type<float, long long>       { typedef uint type; };
template <> struct uint_type<float, unsigned long long>           { typedef uint type; };

template <> struct uint_type<double, char>            { typedef uint type; };
template <> struct uint_type<double, unsigned char>   { typedef uint type; };
template <> struct uint_type<double, short>           { typedef uint type; };
template <> struct uint_type<double, unsigned short>  { typedef uint type; };
template <> struct uint_type<double, int>             { typedef uint type; };
template <> struct uint_type<double, unsigned int>    { typedef uint type; };
template <> struct uint_type<double, float>           { typedef uint type; };
template <> struct uint_type<double, double>          { typedef uint type; };
template <> struct uint_type<double, long long>       { typedef uint type; };
template <> struct uint_type<double, unsigned long long>           { typedef uint type; };

template <> struct uint_type<long long, char>            { typedef uint type; };
template <> struct uint_type<long long, unsigned char>   { typedef uint type; };
template <> struct uint_type<long long, short>           { typedef uint type; };
template <> struct uint_type<long long, unsigned short>  { typedef uint type; };
template <> struct uint_type<long long, int>             { typedef uint type; };
template <> struct uint_type<long long, unsigned int>    { typedef uint type; };
template <> struct uint_type<long long, float>           { typedef uint type; };
template <> struct uint_type<long long, double>          { typedef uint type; };
template <> struct uint_type<long long, long long>       { typedef uint type; };
template <> struct uint_type<long long, unsigned long long>           { typedef uint type; };

template <> struct uint_type<unsigned long long, char>            { typedef uint type; };
template <> struct uint_type<unsigned long long, unsigned char>   { typedef uint type; };
template <> struct uint_type<unsigned long long, short>           { typedef uint type; };
template <> struct uint_type<unsigned long long, unsigned short>  { typedef uint type; };
template <> struct uint_type<unsigned long long, int>             { typedef uint type; };
template <> struct uint_type<unsigned long long, unsigned int>    { typedef uint type; };
template <> struct uint_type<unsigned long long, float>           { typedef uint type; };
template <> struct uint_type<unsigned long long, double>          { typedef uint type; };
template <> struct uint_type<unsigned long long, long long>       { typedef uint type; };
template <> struct uint_type<unsigned long long, unsigned long long>           { typedef uint type; };

template <typename T1, typename T2> struct int_uint_type {
private:
        int_uint_type();
};
template <> struct int_uint_type<char, char>             { typedef int type; };
template <> struct int_uint_type<char, unsigned char>    { typedef int type; };
template <> struct int_uint_type<char, short>            { typedef int type; };
template <> struct int_uint_type<char, unsigned short>   { typedef int type; };
template <> struct int_uint_type<char, int>              { typedef int type; };
template <> struct int_uint_type<char, unsigned int>     { typedef int type; };
template <> struct int_uint_type<char, long long>        { typedef int type; };
template <> struct int_uint_type<char, unsigned long long>            { typedef int type; };

template <> struct int_uint_type<short, char>             { typedef int type; };
template <> struct int_uint_type<short, unsigned char>    { typedef int type; };
template <> struct int_uint_type<short, short>            { typedef int type; };
template <> struct int_uint_type<short, unsigned short>   { typedef int type; };
template <> struct int_uint_type<short, int>              { typedef int type; };
template <> struct int_uint_type<short, unsigned int>     { typedef int type; };
template <> struct int_uint_type<short, long long>        { typedef int type; };
template <> struct int_uint_type<short, unsigned long long>            { typedef int type; };

template <> struct int_uint_type<int, char>               { typedef int type; };
template <> struct int_uint_type<int, unsigned char>      { typedef int type; };
template <> struct int_uint_type<int, short>              { typedef int type; };
template <> struct int_uint_type<int, unsigned short>     { typedef int type; };
template <> struct int_uint_type<int, int>                { typedef int type; };
template <> struct int_uint_type<int, unsigned int>       { typedef int type; };
template <> struct int_uint_type<int, long long>          { typedef int type; };
template <> struct int_uint_type<int, unsigned long long>              { typedef int type; };

template <> struct int_uint_type<long long, char>               { typedef int type; };
template <> struct int_uint_type<long long, unsigned char>      { typedef int type; };
template <> struct int_uint_type<long long, short>              { typedef int type; };
template <> struct int_uint_type<long long, unsigned short>     { typedef int type; };
template <> struct int_uint_type<long long, int>                { typedef int type; };
template <> struct int_uint_type<long long, unsigned int>       { typedef int type; };
template <> struct int_uint_type<long long, long long>          { typedef int type; };
template <> struct int_uint_type<long long, unsigned long long>              { typedef int type; };

template <> struct int_uint_type<unsigned char, char>             { typedef uint type; };
template <> struct int_uint_type<unsigned char, unsigned char>    { typedef uint type; };
template <> struct int_uint_type<unsigned char, short>            { typedef uint type; };
template <> struct int_uint_type<unsigned char, unsigned short>   { typedef uint type; };
template <> struct int_uint_type<unsigned char, int>              { typedef uint type; };
template <> struct int_uint_type<unsigned char, unsigned int>     { typedef uint type; };
template <> struct int_uint_type<unsigned char, long long>        { typedef uint type; };
template <> struct int_uint_type<unsigned char, unsigned long long>            { typedef uint type; };

template <> struct int_uint_type<unsigned short, char>             { typedef uint type; };
template <> struct int_uint_type<unsigned short, unsigned char>    { typedef uint type; };
template <> struct int_uint_type<unsigned short, short>            { typedef uint type; };
template <> struct int_uint_type<unsigned short, unsigned short>   { typedef uint type; };
template <> struct int_uint_type<unsigned short, int>              { typedef uint type; };
template <> struct int_uint_type<unsigned short, unsigned int>     { typedef uint type; };
template <> struct int_uint_type<unsigned short, long long>        { typedef uint type; };
template <> struct int_uint_type<unsigned short, unsigned long long>            { typedef uint type; };

template <> struct int_uint_type<unsigned int, char>             { typedef uint type; };
template <> struct int_uint_type<unsigned int, unsigned char>    { typedef uint type; };
template <> struct int_uint_type<unsigned int, short>            { typedef uint type; };
template <> struct int_uint_type<unsigned int, unsigned short>   { typedef uint type; };
template <> struct int_uint_type<unsigned int, int>              { typedef uint type; };
template <> struct int_uint_type<unsigned int, unsigned int>     { typedef uint type; };
template <> struct int_uint_type<unsigned int, long long>        { typedef uint type; };
template <> struct int_uint_type<unsigned int, unsigned long long>            { typedef uint type; };

template <> struct int_uint_type<unsigned long long, char>             { typedef uint type; };
template <> struct int_uint_type<unsigned long long, unsigned char>    { typedef uint type; };
template <> struct int_uint_type<unsigned long long, short>            { typedef uint type; };
template <> struct int_uint_type<unsigned long long, unsigned short>   { typedef uint type; };
template <> struct int_uint_type<unsigned long long, int>              { typedef uint type; };
template <> struct int_uint_type<unsigned long long, unsigned int>     { typedef uint type; };
template <> struct int_uint_type<unsigned long long, long long>        { typedef uint type; };
template <> struct int_uint_type<unsigned long long, unsigned long long>            { typedef uint type; };

template <typename T1, typename T2> struct restype_sat {
private:
    restype_sat();
};

template <> struct restype_sat<char, char>            { typedef int  type; };
template <> struct restype_sat<char, unsigned char>   { typedef int  type; };
template <> struct restype_sat<char, short>           { typedef int type; };
template <> struct restype_sat<char, unsigned short>  { typedef int type; };
template <> struct restype_sat<char, int>             { typedef long long   type; };
template <> struct restype_sat<char, unsigned int>    { typedef long long   type; };
template <> struct restype_sat<char, float>           { typedef float type; };
template <> struct restype_sat<char, double>          { typedef double type; };
template <> struct restype_sat<char, long long>       { typedef long long type; };
template <> struct restype_sat<char, unsigned long long>           { typedef long long type; };

template <> struct restype_sat<unsigned char, char>            { typedef int  type; };
template <> struct restype_sat<unsigned char, unsigned char>   { typedef int  type; };
template <> struct restype_sat<unsigned char, short>           { typedef int type; };
template <> struct restype_sat<unsigned char, unsigned short>  { typedef int type; };
template <> struct restype_sat<unsigned char, int>             { typedef long long   type; };
template <> struct restype_sat<unsigned char, unsigned int>    { typedef long long   type; };
template <> struct restype_sat<unsigned char, float>           { typedef float type; };
template <> struct restype_sat<unsigned char, double>          { typedef double type; };
template <> struct restype_sat<unsigned char, long long>       { typedef long long type; };
template <> struct restype_sat<unsigned char, unsigned long long>           { typedef long long type; };

template <> struct restype_sat<short, char>            { typedef int type; };
template <> struct restype_sat<short, unsigned char>   { typedef int type; };
template <> struct restype_sat<short, short>           { typedef int type; };
template <> struct restype_sat<short, unsigned short>  { typedef int type; };
template <> struct restype_sat<short, int>             { typedef long long   type; };
template <> struct restype_sat<short, unsigned int>    { typedef long long   type; };
template <> struct restype_sat<short, float>           { typedef float type; };
template <> struct restype_sat<short, double>          { typedef double type; };
template <> struct restype_sat<short, long long>       { typedef long long type; };
template <> struct restype_sat<short, unsigned long long>           { typedef long long type; };

template <> struct restype_sat<unsigned short, char>            { typedef int type; };
template <> struct restype_sat<unsigned short, unsigned char>   { typedef int type; };
template <> struct restype_sat<unsigned short, short>           { typedef int type; };
template <> struct restype_sat<unsigned short, unsigned short>  { typedef unsigned int type; };
template <> struct restype_sat<unsigned short, int>             { typedef long long type; };
template <> struct restype_sat<unsigned short, unsigned int>    { typedef long long type; };
template <> struct restype_sat<unsigned short, float>           { typedef float type; };
template <> struct restype_sat<unsigned short, double>          { typedef double type; };
template <> struct restype_sat<unsigned short, long long>       { typedef long long type; };
template <> struct restype_sat<unsigned short, unsigned long long>           { typedef long long type; };

template <> struct restype_sat<int, char>            { typedef long long type; };
template <> struct restype_sat<int, unsigned char>   { typedef long long type; };
template <> struct restype_sat<int, short>           { typedef long long type; };
template <> struct restype_sat<int, unsigned short>  { typedef long long type; };
template <> struct restype_sat<int, int>             { typedef long long type; };
template <> struct restype_sat<int, unsigned int>    { typedef long long type; };
template <> struct restype_sat<int, float>           { typedef float type; };
template <> struct restype_sat<int, double>          { typedef double type; };
template <> struct restype_sat<int, long long>       { typedef long long type; };
template <> struct restype_sat<int, unsigned long long>           { typedef long long type; };

template <> struct restype_sat<unsigned int, char>            { typedef long long type; };
template <> struct restype_sat<unsigned int, unsigned char>   { typedef long long type; };
template <> struct restype_sat<unsigned int, short>           { typedef long long type; };
template <> struct restype_sat<unsigned int, unsigned short>  { typedef long long type; };
template <> struct restype_sat<unsigned int, int>             { typedef long long type; };
template <> struct restype_sat<unsigned int, unsigned int>    { typedef long long type; };
template <> struct restype_sat<unsigned int, float>           { typedef float type; };
template <> struct restype_sat<unsigned int, double>          { typedef double type; };
template <> struct restype_sat<unsigned int, long long>       { typedef long long type; };
template <> struct restype_sat<unsigned int, unsigned long long>           { typedef long long type; };

template <> struct restype_sat<float, char>            { typedef float type; };
template <> struct restype_sat<float, unsigned char>   { typedef float type; };
template <> struct restype_sat<float, short>           { typedef float type; };
template <> struct restype_sat<float, unsigned short>  { typedef float type; };
template <> struct restype_sat<float, int>             { typedef float type; };
template <> struct restype_sat<float, unsigned int>    { typedef float type; };
template <> struct restype_sat<float, float>           { typedef float type; };
template <> struct restype_sat<float, double>           { typedef double type; };

template <> struct restype_sat<double, char>            { typedef double type; };
template <> struct restype_sat<double, unsigned char>   { typedef double type; };
template <> struct restype_sat<double, short>           { typedef double type; };
template <> struct restype_sat<double, unsigned short>  { typedef double type; };
template <> struct restype_sat<double, int>             { typedef double type; };
template <> struct restype_sat<double, unsigned int>    { typedef double type; };
template <> struct restype_sat<double, float>           { typedef double type; };
template <> struct restype_sat<double, double>          { typedef double type; };
template <> struct restype_sat<double, long long>       { typedef double type; };
template <> struct restype_sat<double, unsigned long long>           { typedef double type; };

template <typename T> struct abstype;
template<> struct abstype<float> { typedef float type; };
template<> struct abstype<char> { typedef uchar type; };
template<> struct abstype<short> { typedef ushort type; };
template<> struct abstype<int> { typedef int type; };
template<> struct abstype<uchar> { typedef uchar type; };
template<> struct abstype<ushort> { typedef ushort type; };
template<> struct abstype<uint> { typedef uint type; };
template<> struct abstype<double> { typedef double type; };
template<> struct abstype<long long> { typedef unsigned long long type; };
template<> struct abstype<unsigned long long> { typedef unsigned long long type; };

template <typename T>
struct to_int {
    typedef T Int;
};
template<> struct to_int<float> { typedef int Int; };
template<> struct to_int<double> { typedef int Int; };

template <bool VALUE> struct check_true{
    static const bool value = false;
};
template <> struct check_true<true> {
    static const bool value = true;
};

template <typename T> struct inttype;
template <> struct inttype<char> {
    static const bool value = true;
};
template <> struct inttype<unsigned char> {
    static const bool value = true;
};
template <> struct inttype<short> {
    static const bool value = true;
};
template <> struct inttype<unsigned short> {
    static const bool value = true;
};
template <> struct inttype<int> {
    static const bool value = true;
};
template <> struct inttype<unsigned int> {
    static const bool value = true;
};
template <> struct inttype<long long> {
    static const bool value = true;
};
template <> struct inttype<unsigned long long> {
    static const bool value = true;
};

template <typename T> struct is_inttype {
    static const bool value = false;
};
template <> struct is_inttype<char> {
    static const bool value = true;
};
template <> struct is_inttype<unsigned char> {
    static const bool value = true;
};
template <> struct is_inttype<short> {
    static const bool value = true;
};
template <> struct is_inttype<unsigned short> {
    static const bool value = true;
};
template <> struct is_inttype<int> {
    static const bool value = true;
};
template <> struct is_inttype<unsigned int> {
    static const bool value = true;
};
template <> struct is_inttype<long long> {
    static const bool value = true;
};
template <> struct is_inttype<unsigned long long> {
    static const bool value = true;
};

template <typename T> struct is_byte_type {
    static const bool value = false;
};
template <> struct is_byte_type<char> {
    static const bool value = true;
};
template <> struct is_byte_type<uchar> {
    static const bool value = true;
};

template <typename T> struct is_word_type {
    static const bool value = false;
};
template <> struct is_word_type<short> {
    static const bool value = true;
};
template <> struct is_word_type<ushort> {
    static const bool value = true;
};

template <typename T> struct is_dword_type {
    static const bool value = false;
};
template <> struct is_dword_type<int> {
    static const bool value = true;
};
template <> struct is_dword_type<uint> {
    static const bool value = true;
};

template <typename T> struct is_fp_type {
    static const bool value = false;
};
template <> struct is_fp_type<float> {
    static const bool value = true;
};

template <typename T> struct is_df_type {
    static const bool value = false;
};
template <> struct is_df_type<double> {
    static const bool value = true;
};

template <typename T> struct is_fp_or_dword_type {
    static const bool value = false;
};
template <> struct is_fp_or_dword_type<int> {
    static const bool value = true;
};
template <> struct is_fp_or_dword_type<uint> {
    static const bool value = true;
};
template <> struct is_fp_or_dword_type<float> {
    static const bool value = true;
};
// The check is only used for dataport APIs,
// which also support df data type.
template <> struct is_fp_or_dword_type<double> {
    static const bool value = true;
};

template <typename T> struct is_ushort_type {
    static const bool value = false;
};
template <> struct is_ushort_type<ushort> {
    static const bool value = true;
};

template <typename T1, typename T2> struct is_float_dword {
    static const bool value = false;
};
template <> struct is_float_dword<float, int> {
    static const bool value = true;
};
template <> struct is_float_dword<float, uint> {
    static const bool value = true;
};
template <> struct is_float_dword<int, float> {
    static const bool value = true;
};
template <> struct is_float_dword<uint, float> {
    static const bool value = true;
};

template <typename T> struct fptype {
    static const bool value = false;
};
template <> struct fptype<float> {
    static const bool value = true;
};

template <typename T> struct dftype {
    static const bool value = false;
};
template <> struct dftype<double> {
    static const bool value = true;
};

template <typename T> struct cmtype;
template <> struct cmtype<char> {
    static const bool value = true;
};

template <> struct cmtype<signed char> {
    static const bool value = true;
};

template <> struct cmtype<unsigned char> {
    static const bool value = true;
};

template <> struct cmtype<short> {
    static const bool value = true;
};

template <> struct cmtype<unsigned short> {
    static const bool value = true;
};
template <> struct cmtype<int> {
    static const bool value = true;
};

template <> struct cmtype<unsigned int> {
    static const bool value = true;
};
template <> struct cmtype<float> {
    static const bool value = true;
};

template <> struct cmtype<double> {
    static const bool value = true;
};

template <> struct cmtype<long long> {
    static const bool value = true;
};

template <> struct cmtype<unsigned long long> {
    static const bool value = true;
};

template <> struct cmtype<SurfaceIndex> {
    static const bool value = true;
};

template<typename T> struct bytetype;
template<> struct bytetype<char> {
    static const bool value = true;
};
template<> struct bytetype<uchar> {
    static const bool value = true;
};

template<typename T> struct wordtype;
template<> struct wordtype<short> {
    static const bool value = true;
};
template<> struct wordtype<ushort> {
    static const bool value = true;
};

template<typename T> struct dwordtype;
template<> struct dwordtype<int> {
    static const bool value = true;
};
template<> struct dwordtype<uint> {
    static const bool value = true;
};

template<typename T> struct unsignedtype{
    static const bool value = false;
};
template<> struct unsignedtype<uint> {
    static const bool value = true;
};
template<> struct unsignedtype<ushort> {
    static const bool value = true;
};
template<> struct unsignedtype<uchar> {
    static const bool value = true;
};
template<> struct unsignedtype<unsigned long long> {
    static const bool value = true;
};

template<typename T> struct uinttype;
template<> struct uinttype<uint> {
    static const bool value = true;
};

template <uint N1, uint N2> struct ressize {
    static const uint size = (N1 > N2)?N1:N2;
    static const bool conformable = check_true<N1%size == 0 && N2%size == 0>::value;
};



#endif /* CM_DEF_H */
