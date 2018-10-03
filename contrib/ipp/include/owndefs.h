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

#ifndef __OWNDEFS_H__
#define __OWNDEFS_H__

#if defined( _VXWORKS )
  #include <vxWorks.h>
  #undef NONE
#endif

#include "ippdefs.h"

#if defined(__ICC) || defined(__ECC) || defined( __ICL ) || defined ( __ECL ) || defined(_MSC_VER)
  #define __INLINE static __inline
#elif defined( __GNUC__ )
  #define __INLINE static __inline__
#else
  #define __INLINE static
#endif

#if defined(__ICC) || defined(__ECC) || defined( __ICL ) || defined ( __ECL )
 #define __RESTRICT restrict
#elif !defined( __RESTRICT )
 #define __RESTRICT
#endif

#if defined( IPP_W32DLL )
  #if defined( _MSC_VER ) || defined( __ICL ) || defined ( __ECL )
    #define IPPFUN(type,name,arg) __declspec(dllexport) type __STDCALL name arg
  #else
    #define IPPFUN(type,name,arg)                extern type __STDCALL name arg
  #endif
#else
  #define   IPPFUN(type,name,arg)                extern type __STDCALL name arg
#endif

#if !defined ( _IA64) && (defined ( _WIN64 ) || defined( linux64 ))
#define _IA64
#endif

//structure represeting 128 bit unsigned integer type

typedef struct{
  Ipp64u low;
  Ipp64u high;
}Ipp128u;

#define _IPP_PX 0
#define _IPP_M6 1
#define _IPP_A6 2
#define _IPP_W7 4
#define _IPP_T7 8
#define _IPP_V8 16
#define _IPP_P8 32
#define _IPP_G9 64
#define _IPP_H9 128

#define _IPPXSC_PX 0
#define _IPPXSC_S1 1
#define _IPPXSC_S2 2
#define _IPPXSC_C2 4

#define _IPPLRB_PX 0
#define _IPPLRB_B1 1
#define _IPPLRB_B2 2

#define _IPP64_PX  _IPP_PX
#define _IPP64_I7 64

#define _IPP32E_PX _IPP_PX
#define _IPP32E_M7 32
#define _IPP32E_U8 64
#define _IPP32E_Y8 128
#define _IPP32E_E9 256
#define _IPP32E_L9 512

#define _IPPLP32_PX _IPP_PX
#define _IPPLP32_S8 1

#define _IPPLP64_PX _IPP_PX
#define _IPPLP64_N8 1

#if defined(__ICC) || defined(__ECC) || defined( __ICL ) || defined ( __ECL ) || (_MSC_VER >= 1300)
    #define __ALIGN8  __declspec (align(8))
    #define __ALIGN16 __declspec (align(16))
#if !defined( OSX32 )
    #define __ALIGN32 __declspec (align(32))
#else
    #define __ALIGN32 __declspec (align(16))
#endif
    #define __ALIGN64 __declspec (align(64))
#else
    #define __ALIGN8
    #define __ALIGN16
    #define __ALIGN32
    #define __ALIGN64
#endif

#if defined ( _M6 )
  #define _IPP    _IPP_M6
  #define _IPP64  _IPP64_PX
  #define _IPP32E _IPP32E_PX
  #define _IPPXSC _IPPXSC_PX
  #define _IPPLRB _IPPLRB_PX
  #define _IPPLP32 _IPPLP32_PX
  #define _IPPLP64 _IPPLP64_PX

#elif defined( _A6 )
  #define _IPP    _IPP_A6
  #define _IPP64  _IPP64_PX
  #define _IPP32E _IPP32E_PX
  #define _IPPXSC _IPPXSC_PX
  #define _IPPLRB _IPPLRB_PX
  #define _IPPLP32 _IPPLP32_PX
  #define _IPPLP64 _IPPLP64_PX

#elif defined( _W7 )
  #define _IPP    _IPP_W7
  #define _IPP64  _IPP64_PX
  #define _IPP32E _IPP32E_PX
  #define _IPPXSC _IPPXSC_PX
  #define _IPPLRB _IPPLRB_PX
  #define _IPPLP32 _IPPLP32_PX
  #define _IPPLP64 _IPPLP64_PX

#elif defined( _T7 )
  #define _IPP    _IPP_T7
  #define _IPP64  _IPP64_PX
  #define _IPP32E _IPP32E_PX
  #define _IPPXSC _IPPXSC_PX
  #define _IPPLRB _IPPLRB_PX
  #define _IPPLP32 _IPPLP32_PX
  #define _IPPLP64 _IPPLP64_PX

#elif defined( _V8 )
  #define _IPP    _IPP_V8
  #define _IPP64  _IPP64_PX
  #define _IPP32E _IPP32E_PX
  #define _IPPXSC _IPPXSC_PX
  #define _IPPLRB _IPPLRB_PX
  #define _IPPLP32 _IPPLP32_PX
  #define _IPPLP64 _IPPLP64_PX

#elif defined( _P8 )
  #define _IPP    _IPP_P8
  #define _IPP64  _IPP64_PX
  #define _IPP32E _IPP32E_PX
  #define _IPPXSC _IPPXSC_PX
  #define _IPPLRB _IPPLRB_PX
  #define _IPPLP32 _IPPLP32_PX
  #define _IPPLP64 _IPPLP64_PX

#elif defined( _G9 )
  #define _IPP    _IPP_G9
  #define _IPP64  _IPP64_PX
  #define _IPP32E _IPP32E_PX
  #define _IPPXSC _IPPXSC_PX
  #define _IPPLRB _IPPLRB_PX
  #define _IPPLP32 _IPPLP32_PX
  #define _IPPLP64 _IPPLP64_PX

#elif defined( _H9 )
  #define _IPP    _IPP_H9
  #define _IPP64  _IPP64_PX
  #define _IPP32E _IPP32E_PX
  #define _IPPXSC _IPPXSC_PX
  #define _IPPLRB _IPPLRB_PX
  #define _IPPLP32 _IPPLP32_PX
  #define _IPPLP64 _IPPLP64_PX

#elif defined( _M7 )
  #define _IPP    _IPP_PX
  #define _IPP64  _IPP64_PX
  #define _IPP32E _IPP32E_M7
  #define _IPPXSC _IPPXSC_PX
  #define _IPPLRB _IPPLRB_PX
  #define _IPPLP32 _IPPLP32_PX
  #define _IPPLP64 _IPPLP64_PX

#elif defined( _U8 )
  #define _IPP    _IPP_PX
  #define _IPP64  _IPP64_PX
  #define _IPP32E _IPP32E_U8
  #define _IPPXSC _IPPXSC_PX
  #define _IPPLRB _IPPLRB_PX
  #define _IPPLP32 _IPPLP32_PX
  #define _IPPLP64 _IPPLP64_PX

#elif defined( _Y8 )
  #define _IPP    _IPP_PX
  #define _IPP64  _IPP64_PX
  #define _IPP32E _IPP32E_Y8
  #define _IPPXSC _IPPXSC_PX
  #define _IPPLRB _IPPLRB_PX
  #define _IPPLP32 _IPPLP32_PX
  #define _IPPLP64 _IPPLP64_PX

#elif defined( _E9 )
  #define _IPP    _IPP_PX
  #define _IPP64  _IPP64_PX
  #define _IPP32E _IPP32E_E9
  #define _IPPXSC _IPPXSC_PX
  #define _IPPLRB _IPPLRB_PX
  #define _IPPLP32 _IPPLP32_PX
  #define _IPPLP64 _IPPLP64_PX

#elif defined( _L9 )
  #define _IPP    _IPP_PX
  #define _IPP64  _IPP64_PX
  #define _IPP32E _IPP32E_L9
  #define _IPPXSC _IPPXSC_PX
  #define _IPPLRB _IPPLRB_PX
  #define _IPPLP32 _IPPLP32_PX
  #define _IPPLP64 _IPPLP64_PX

#elif defined( _I7 )
  #define _IPP    _IPP_PX
  #define _IPP64  _IPP64_I7
  #define _IPP32E _IPP32E_PX
  #define _IPPXSC _IPPXSC_PX
  #define _IPPLRB _IPPLRB_PX
  #define _IPPLP32 _IPPLP32_PX
  #define _IPPLP64 _IPPLP64_PX

#elif defined( _B1 )
  #define _IPP    _IPP_PX
  #define _IPP64  _IPP64_PX
  #define _IPP32E _IPP32E_PX
  #define _IPPXSC _IPPXSC_PX
  #define _IPPLRB _IPPLRB_B1
  #define _IPPLP32 _IPPLP32_PX
  #define _IPPLP64 _IPPLP64_PX

#elif defined( _B2 )
  #define _IPP    _IPP_PX
  #define _IPP64  _IPP64_PX
  #define _IPP32E _IPP32E_PX
  #define _IPPXSC _IPPXSC_PX
  #define _IPPLRB _IPPLRB_B2
  #define _IPPLP32 _IPPLP32_PX
  #define _IPPLP64 _IPPLP64_PX

#elif defined( _S8 )
  #define _IPP    _IPP_V8
  #define _IPP64  _IPP64_PX
  #define _IPP32E _IPP32E_PX
  #define _IPPXSC _IPPXSC_PX
  #define _IPPLRB _IPPLRB_PX
  #define _IPPLP32 _IPPLP32_S8
  #define _IPPLP64 _IPPLP64_PX

#elif defined( _N8 )
  #define _IPP    _IPP_PX
  #define _IPP64  _IPP64_PX
  #define _IPP32E _IPP32E_U8
  #define _IPPXSC _IPPXSC_PX
  #define _IPPLRB _IPPLRB_PX
  #define _IPPLP32 _IPPLP32_PX
  #define _IPPLP64 _IPPLP64_N8

#elif defined( _StrongARM )
  #define _IPP    _IPP_PX
  #define _IPP64  _IPP64_PX
  #define _IPP32E _IPP32E_PX
  #define _IPPXSC _IPPXSC_S1
  #define _PX    /* This string must be removed after migation period */
  #define _IPPLRB _IPPLRB_PX
  #define _IPPLP32 _IPPLP32_PX
  #define _IPPLP64 _IPPLP64_PX

#elif defined( _XScale )
  #define _IPP    _IPP_PX
  #define _IPP64  _IPP64_PX
  #define _IPP32E _IPP32E_PX
  #define _IPPXSC _IPPXSC_S2
  #define _PX    /* This string must be removed after migation period */
  #define _IPPLRB _IPPLRB_PX
  #define _IPPLP32 _IPPLP32_PX
  #define _IPPLP64 _IPPLP64_PX

#elif defined( _XScale_Concan )
  #define _IPP    _IPP_PX
  #define _IPP64  _IPP64_PX
  #define _IPP32E _IPP32E_PX
  #define _IPPXSC _IPPXSC_C2
  #define _PX    /* This string must be removed after migation period */
  #define _IPPLRB _IPPLRB_PX
  #define _IPPLP32 _IPPLP32_PX
  #define _IPPLP64 _IPPLP64_PX

#else
  #define _IPP    _IPP_PX
  #define _IPP64  _IPP64_PX
  #define _IPP32E _IPP32E_PX
  #define _IPPXSC _IPPXSC_PX
  #define _IPPLRB _IPPLRB_PX
  #define _IPPLP32 _IPPLP32_PX
  #define _IPPLP64 _IPPLP64_PX

#endif


#define _IPP_ARCH_IA32    1
#define _IPP_ARCH_IA64    2
#define _IPP_ARCH_EM64T   4
#define _IPP_ARCH_XSC     8
#define _IPP_ARCH_LRB     16
#define _IPP_ARCH_LP32    32
#define _IPP_ARCH_LP64    64
#define _IPP_ARCH_LRB2    128

#if defined ( _ARCH_IA32 )
  #define _IPP_ARCH    _IPP_ARCH_IA32

#elif defined( _ARCH_IA64 )
  #define _IPP_ARCH    _IPP_ARCH_IA64

#elif defined( _ARCH_EM64T )
  #define _IPP_ARCH    _IPP_ARCH_EM64T

#elif defined( _ARCH_XSC )
  #define _IPP_ARCH    _IPP_ARCH_XSC

#elif defined( _ARCH_LRB )
  #define _IPP_ARCH    _IPP_ARCH_LRB

#elif defined( _ARCH_LRB2 )
  #define _IPP_ARCH    _IPP_ARCH_LRB2

#elif defined( _ARCH_LP32 )
  #define _IPP_ARCH    _IPP_ARCH_LP32

#elif defined( _ARCH_LP64 )
  #define _IPP_ARCH    _IPP_ARCH_LP64

#else
  #if defined(_M_AMD64) || defined(__x86_64) || defined(__x86_64__)
    #define _IPP_ARCH    _IPP_ARCH_EM64T

  #elif defined(_M_IA64) || defined(__ia64) || defined(__ia64__)
    #define _IPP_ARCH    _IPP_ARCH_IA64

  #elif defined(_M_ARM) || defined(_ARM_) || defined(ARM) || defined(__arm__)
    #define _IPP_ARCH    _IPP_ARCH_XSC

  #else
    #define _IPP_ARCH    _IPP_ARCH_IA32

  #endif
#endif

#if ((_IPP_ARCH == _IPP_ARCH_IA32) || (_IPP_ARCH == _IPP_ARCH_XSC) || (_IPP_ARCH == _IPP_ARCH_LP32))
__INLINE
Ipp32s IPP_INT_PTR( const void* ptr )  {
    union {
        void*   Ptr;
        Ipp32s  Int;
    } dd;
    dd.Ptr = (void*)ptr;
    return dd.Int;
}

__INLINE
Ipp32u IPP_UINT_PTR( const void* ptr )  {
    union {
        void*   Ptr;
        Ipp32u  Int;
    } dd;
    dd.Ptr = (void*)ptr;
    return dd.Int;
}
#elif ((_IPP_ARCH == _IPP_ARCH_IA64) || (_IPP_ARCH == _IPP_ARCH_EM64T) || (_IPP_ARCH == _IPP_ARCH_LRB) || (_IPP_ARCH == _IPP_ARCH_LRB2) || (_IPP_ARCH == _IPP_ARCH_LP64))
__INLINE
Ipp64s IPP_INT_PTR( const void* ptr )  {
    union {
        void*   Ptr;
        Ipp64s  Int;
    } dd;
    dd.Ptr = (void*)ptr;
    return dd.Int;
}

__INLINE
Ipp64u IPP_UINT_PTR( const void* ptr )  {
    union {
        void*    Ptr;
        Ipp64u   Int;
    } dd;
    dd.Ptr = (void*)ptr;
    return dd.Int;
}
#else
  #define IPP_INT_PTR( ptr )  ( (long)(ptr) )
  #define IPP_UINT_PTR( ptr ) ( (unsigned long)(ptr) )
#endif

#define IPP_BYTES_TO_ALIGN(ptr, align) ((-(IPP_INT_PTR(ptr)&((align)-1)))&((align)-1))
#define IPP_ALIGNED_PTR(ptr, align) (void*)( (unsigned char*)(ptr) + (IPP_BYTES_TO_ALIGN( ptr, align )) )

#define IPP_MALLOC_ALIGNED_BYTES   64

#if defined( __cplusplus )
extern "C" {
#endif

/* /////////////////////////////////////////////////////////////////////////////
           Helpers
  /////////////////////////////////////////////////////////////////////////// */

#define IPP_NOERROR_RET()  return ippStsNoErr
#define IPP_ERROR_RET( ErrCode )  return (ErrCode)

#ifdef _IPP_DEBUG

    #define IPP_BADARG_RET( expr, ErrCode )\
                {if (expr) { IPP_ERROR_RET( ErrCode ); }}

#else

    #define IPP_BADARG_RET( expr, ErrCode )

#endif


    #define IPP_BAD_SIZE_RET( n )\
                IPP_BADARG_RET( (n)<=0, ippStsSizeErr )

    #define IPP_BAD_STEP_RET( n )\
                IPP_BADARG_RET( (n)<=0, ippStsStepErr )

    #define IPP_BAD_PTR1_RET( ptr )\
                IPP_BADARG_RET( NULL==(ptr), ippStsNullPtrErr )

    #define IPP_BAD_PTR2_RET( ptr1, ptr2 )\
                IPP_BADARG_RET(((NULL==(ptr1))||(NULL==(ptr2))), ippStsNullPtrErr)

    #define IPP_BAD_PTR3_RET( ptr1, ptr2, ptr3 )\
                IPP_BADARG_RET(((NULL==(ptr1))||(NULL==(ptr2))||(NULL==(ptr3))),\
                                                         ippStsNullPtrErr)

    #define IPP_BAD_PTR4_RET( ptr1, ptr2, ptr3, ptr4 )\
                {IPP_BAD_PTR2_RET( ptr1, ptr2 ); IPP_BAD_PTR2_RET( ptr3, ptr4 )}

    #define IPP_BAD_ISIZE_RET(roi) \
               IPP_BADARG_RET( ((roi).width<=0 || (roi).height<=0), ippStsSizeErr)


/* ////////////////////////////////////////////////////////////////////////// */

/* Define NULL pointer value */
#ifndef NULL
#ifdef  __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

#define UNREFERENCED_PARAMETER(p) (p)=(p)

#if defined( _IPP_MARK_LIBRARY )
static char G[] = {73, 80, 80, 71, 101, 110, 117, 105, 110, 101, 243, 193, 210, 207, 215};
#endif


#define STR2(x)           #x
#define STR(x)       STR2(x)
#define MESSAGE( desc )\
     message(__FILE__ "(" STR(__LINE__) "):" #desc)

/* ////////////////////////////////////////////////////////////////////////// */

/* intrinsics */
#if (_IPPXSC >= _IPPXSC_C2)
    #if defined(__INTEL_COMPILER)
        #include "mmintrin.h"
    #endif
#elif (_IPP64 >= _IPP64_I7)
    #if defined(__INTEL_COMPILER)
//        #define _SSE2_INTEGER_FOR_ITANIUM
        #include "ia64intrin.h"
        #include "emmintrin.h"
    #endif
#elif (_IPP >= _IPP_A6) || (_IPP32E >= _IPP32E_M7)
    #if defined(__INTEL_COMPILER) || (_MSC_VER >= 1300)
        #if (_IPP == _IPP_A6)
            #include "xmmintrin.h"
        #elif (_IPP == _IPP_W7)
            #if defined(__INTEL_COMPILER)
              #include "emmintrin.h"
            #else
              #undef _W7
              #include "emmintrin.h"
              #define _W7
            #endif
            #define _mm_loadu _mm_loadu_si128
        #elif (_IPP == _IPP_T7) || (_IPP32E == _IPP32E_M7)
            #if defined(__INTEL_COMPILER)
                #include "pmmintrin.h"
                #define _mm_loadu _mm_lddqu_si128
            #elif (_MSC_FULL_VER >= 140050110)
                #include "intrin.h"
                #define _mm_loadu _mm_lddqu_si128
            #elif (_MSC_FULL_VER < 140050110)
                #include "emmintrin.h"
                #define _mm_loadu _mm_loadu_si128
            #endif
        #elif (_IPP == _IPP_V8) || (_IPP32E == _IPP32E_U8)
            #if defined(__INTEL_COMPILER)
                #include "tmmintrin.h"
                #define _mm_loadu _mm_lddqu_si128
            #elif (_MSC_FULL_VER >= 140050110)
                #include "intrin.h"
                #define _mm_loadu _mm_lddqu_si128
            #elif (_MSC_FULL_VER < 140050110)
                #include "emmintrin.h"
                #define _mm_loadu _mm_loadu_si128
            #endif
        #elif (_IPP == _IPP_P8) || (_IPP32E == _IPP32E_Y8)
            #if defined(__INTEL_COMPILER)
                #include "smmintrin.h"
                #define _mm_loadu _mm_lddqu_si128
            #elif (_MSC_FULL_VER >= 140050110)
                #include "intrin.h"
                #define _mm_loadu _mm_lddqu_si128
            #elif (_MSC_FULL_VER < 140050110)
                #include "emmintrin.h"
                #define _mm_loadu _mm_loadu_si128
            #endif
        #elif (_IPP >= _IPP_G9) || (_IPP32E >= _IPP32E_E9)
            #if defined(__INTEL_COMPILER)
                #include "immintrin.h"
                #define _mm_loadu _mm_lddqu_si128
            #elif (_MSC_FULL_VER >= 160021003)
                #include "immintrin.h"
                #define _mm_loadu _mm_lddqu_si128
            #endif
        #endif
    #endif
#elif (_IPPLP32 >= _IPPLP32_S8) || (_IPPLP64 >= _IPPLP64_N8)
    #if defined(__INTEL_COMPILER)
        #include "tmmintrin.h"
        #define _mm_loadu _mm_lddqu_si128
    #elif (_MSC_FULL_VER >= 140050110)
        #include "intrin.h"
        #define _mm_loadu _mm_lddqu_si128
    #elif (_MSC_FULL_VER < 140050110)
        #include "emmintrin.h"
        #define _mm_loadu _mm_loadu_si128
    #endif
#elif (_IPPLRB >= _IPPLRB_B1)
    #if defined(__INTEL_COMPILER) || defined(_REF_LIB)
        #include "immintrin.h"
    #endif
    #if defined (_REF_LIB)
        extern void lmmintrin_init(void);
    #endif
#endif

// **** intrinsics for bit casting ****
//unsigned int      __intel_castf32_u32(float)
//float             __intel_castu32_f32(unsigned int)
//unsigned __int64  __intel_castf64_u64(double)
//double            __intel_castu64_f64(unsigned __int64)
#if defined(__ICC) || defined(__ECC) || defined( __ICL ) || defined ( __ECL )
 #define __CAST_32f32u(val) __intel_castf32_u32((Ipp32f)val)
 #define __CAST_32u32f(val) __intel_castu32_f32((Ipp32u)val)
#else
 #define __CAST_32f32u(val) ( *((Ipp32u*)&val) )
 #define __CAST_32u32f(val) ( *((Ipp32f*)&val) )
#endif


#if defined(__INTEL_COMPILER)
#define __IVDEP ivdep
#else
#define __IVDEP message("message :: 'ivdep' is not defined")
#endif
//usage: #pragma __IVDEP

/* ////////////////////////////////////////////////////////////////////////// */
#if (((_IPP > _IPP_PX) || (_IPP32E > _IPP32E_PX) || \
      (_IPPLP32 > _IPPLP32_PX) || (_IPPLP64 > _IPPLP64_PX) || \
      (_IPP64 > _IPP64_PX) || (_IPPLRB > _IPPLRB_PX)) && defined(_MERGED_BLD))
  #define _NOT_PX_MERGED_BLD
#endif


/* ////////////////////////////////////////////////////////////////////////// */

/* header depended on the architecture */

#ifndef __DEPENDENCESP_H__
  #include "dependencesp.h"
#endif

#ifndef __DEPENDENCEIP_H__
  #include "dependenceip.h"
#endif


#if defined( __cplusplus )
}
#endif

#endif /* __OWNDEFS_H__ */
/* //////////////////////// End of file "owndefs.h" ///////////////////////// */
