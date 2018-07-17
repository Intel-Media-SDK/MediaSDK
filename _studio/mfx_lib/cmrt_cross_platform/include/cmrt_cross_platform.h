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


#ifndef __CMRT_CROSS_PLATFORM_H__
#define __CMRT_CROSS_PLATFORM_H__

#include "mfx_common.h"


/* Applicable for old and new CMAPI */

#define CM_LINUX

#ifndef CM_NOINLINE
#ifndef CM_EMU
#ifndef __GNUC__
#define CM_NOINLINE __declspec(noinline)
#else
#define CM_NOINLINE __attribute__((noinline))
#endif
#else
#define CM_NOINLINE
#endif /* CM_EMU */
#endif

#ifndef CM_INLINE
#ifndef __GNUC__
#define CM_INLINE __forceinline
#else
#define CM_INLINE inline __attribute__((always_inline))
#endif
#endif

#if defined(__clang__)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#elif defined(__GNUC__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif

class SurfaceIndex
{
public:
    CM_NOINLINE SurfaceIndex() { index = 0; };
    CM_NOINLINE SurfaceIndex(const SurfaceIndex& _src) { index = _src.index; };
    CM_NOINLINE SurfaceIndex(const unsigned int& _n) { index = _n; };
    CM_NOINLINE SurfaceIndex& operator = (const unsigned int& _n) { this->index = _n; return *this; };
    CM_NOINLINE SurfaceIndex& operator + (const unsigned int& _n) { this->index += _n; return *this; };
    virtual unsigned int get_data(void) { return index; };

private:
    unsigned int index;
#ifdef CM_LINUX
    unsigned char extra_byte;
#endif
};

class SamplerIndex
{
public:
    CM_NOINLINE SamplerIndex() { index = 0; };
    CM_NOINLINE SamplerIndex(SamplerIndex& _src) { index = _src.get_data(); };
    CM_NOINLINE SamplerIndex(const unsigned int& _n) { index = _n; };
    CM_NOINLINE SamplerIndex& operator = (const unsigned int& _n) { this->index = _n; return *this; };
    virtual unsigned int get_data(void) { return index; };

private:
    unsigned int index;
#ifdef CM_LINUX
    unsigned char extra_byte;
#endif
};

#ifdef _MSVC_LANG
#pragma warning(push)
#pragma warning(disable: 4100)
#pragma warning(disable: 4201)
#endif

typedef void * AbstractSurfaceHandle;
typedef void * AbstractDeviceHandle;

struct IDirect3DSurface9;
struct IDirect3DDeviceManager9;
struct ID3D11Texture2D;
struct ID3D11Device;

//Using CM_DX9 by default

#ifdef __cplusplus
#   define EXTERN_C     extern "C"
#else
#   define EXTERN_C
#endif

#ifdef CM_LINUX
#define LONG INCORRECT_64BIT_LONG
#define ULONG INCORRECT_64BIT_ULONG
#ifdef __int64
#undef __int64
#endif

#include <time.h>
#include <va/va.h>

#define _tmain main

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <malloc.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <x86intrin.h>
#include <iostream>

typedef int BOOL;

#ifndef FALSE
#define FALSE     0
#endif
#ifndef TRUE
#define TRUE      1
#endif

typedef char byte;
typedef unsigned char BYTE;
typedef unsigned int UINT32;
typedef UINT32 DWORD;
typedef int INT;
typedef unsigned int UINT;
typedef signed char INT8;
typedef unsigned char UINT8;
typedef signed short INT16;
typedef unsigned short UINT16;
typedef signed int INT32;
typedef signed long long INT64;
typedef unsigned long long UINT64;

typedef enum _VACMTEXTUREADDRESS {
    VACMTADDRESS_WRAP = 1,
    VACMTADDRESS_MIRROR = 2,
    VACMTADDRESS_CLAMP = 3,
    VACMTADDRESS_BORDER = 4,
    VACMTADDRESS_MIRRORONCE = 5,

    VACMTADDRESS_FORCE_DWORD = 0x7fffffff
} VACMTEXTUREADDRESS;

typedef enum _VACMTEXTUREFILTERTYPE {
    VACMTEXF_NONE = 0,
    VACMTEXF_POINT = 1,
    VACMTEXF_LINEAR = 2,
    VACMTEXF_ANISOTROPIC = 3,
    VACMTEXF_FLATCUBIC = 4,
    VACMTEXF_GAUSSIANCUBIC = 5,
    VACMTEXF_PYRAMIDALQUAD = 6,
    VACMTEXF_GAUSSIANQUAD = 7,
    VACMTEXF_CONVOLUTIONMONO = 8,    // Convolution filter for monochrome textures
    VACMTEXF_FORCE_DWORD = 0x7fffffff
} VACMTEXTUREFILTERTYPE;

#define CM_MAX_TIMEOUT 2

#define CM_ATTRIBUTE(attribute) __attribute__((attribute))

typedef enum _VA_CM_FORMAT {

    VA_CM_FMT_UNKNOWN = 0,

    VA_CM_FMT_A8R8G8B8 = 21,
    VA_CM_FMT_X8R8G8B8 = 22,
    VA_CM_FMT_A8 = 28,
    VA_CM_FMT_A2B10G10R10 = 31,
    VA_CM_FMT_A8B8G8R8 = 32,
    VA_CM_FMT_A16B16G16R16 = 36,
    VA_CM_FMT_P8 = 41,
    VA_CM_FMT_L8 = 50,
    VA_CM_FMT_A8L8 = 51,
    VA_CM_FMT_R16U = 57,
    VA_CM_FMT_V8U8 = 60,
    VA_CM_FMT_R8U = 62,
    VA_CM_FMT_D16 = 80,
    VA_CM_FMT_L16 = 81,
    VA_CM_FMT_A16B16G16R16F = 113,
    VA_CM_FMT_R32F = 114,
    VA_CM_FMT_NV12 = VA_FOURCC_NV12,
    VA_CM_FMT_UYVY = VA_FOURCC_UYVY,
    VA_CM_FMT_YUY2 = VA_FOURCC_YUY2,
    VA_CM_FMT_444P = VA_FOURCC_444P,
    VA_CM_FMT_411P = VA_FOURCC_411P,
    VA_CM_FMT_422H = VA_FOURCC_422H,
    VA_CM_FMT_422V = VA_FOURCC_422V,
    VA_CM_FMT_IMC3 = VA_FOURCC_IMC3,
    VA_CM_FMT_YV12 = VA_FOURCC_YV12,
    VA_CM_FMT_P010 = VA_FOURCC_P010,
    VA_CM_FMT_P016 = VA_FOURCC_P016,

    VA_CM_FMT_MAX = 0xFFFFFFFF
} VA_CM_FORMAT;

// FIXME temp solution to have same type name as generated by VC++
template<typename T>
inline const char * CM_TYPE_NAME_UNMANGLED();

// FIXME need inline below functions, otherwise it will cause multiple definition when ld
template<> inline const char * CM_TYPE_NAME_UNMANGLED<char>() { return "char"; }
template<> inline const char * CM_TYPE_NAME_UNMANGLED<signed char>() { return "signed char"; }
template<> inline const char * CM_TYPE_NAME_UNMANGLED<unsigned char>() { return "unsigned char"; }
template<> inline const char * CM_TYPE_NAME_UNMANGLED<short>() { return "short"; }
template<> inline const char * CM_TYPE_NAME_UNMANGLED<unsigned short>() { return "unsigned short"; }
template<> inline const char * CM_TYPE_NAME_UNMANGLED<int>() { return "int"; }
template<> inline const char * CM_TYPE_NAME_UNMANGLED<unsigned int>() { return "unsigned int"; }
template<> inline const char * CM_TYPE_NAME_UNMANGLED<long>() { return "long"; }
template<> inline const char * CM_TYPE_NAME_UNMANGLED<unsigned long>() { return "unsigned long"; }
template<> inline const char * CM_TYPE_NAME_UNMANGLED<float>() { return "float"; }
template<> inline const char * CM_TYPE_NAME_UNMANGLED<double>() { return "double"; }

#define CM_TYPE_NAME(type)   CM_TYPE_NAME_UNMANGLED<type>()

inline void * CM_ALIGNED_MALLOC(size_t size, size_t alignment)
{
    return memalign(alignment, size);
}

inline void CM_ALIGNED_FREE(void * memory)
{
    free(memory);
}

//multi-thread API:
#define THREAD_HANDLE pthread_t
inline void CM_THREAD_CREATE(THREAD_HANDLE *handle, void * start_routine, void * arg)
{
    int err = 0;
    err = pthread_create(handle, NULL, (void * (*)(void *))start_routine, arg);
    if (err) {
        printf(" cm create thread failed! \n");
        exit(-1);
    }
}
inline void CM_THREAD_EXIT(void * retval)
{
    pthread_exit(retval);
}

inline int CM_THREAD_JOIN(THREAD_HANDLE *handle_array, int thread_cnt)
{
    void *tret;
    for (int i = 0; i < thread_cnt; i++)
    {
        pthread_join(handle_array[i], &tret);
    }
    return 0;
}

#define CM_SURFACE_FORMAT                       VA_CM_FORMAT

#define CM_SURFACE_FORMAT_UNKNOWN               VA_CM_FMT_UNKNOWN
#define CM_SURFACE_FORMAT_A8R8G8B8              VA_CM_FMT_A8R8G8B8
#define CM_SURFACE_FORMAT_X8R8G8B8              VA_CM_FMT_X8R8G8B8
#define CM_SURFACE_FORMAT_A8B8G8R8              VA_CM_FMT_A8B8G8R8
#define CM_SURFACE_FORMAT_A8                    VA_CM_FMT_A8
#define CM_SURFACE_FORMAT_P8                    VA_CM_FMT_P8
#define CM_SURFACE_FORMAT_R32F                  VA_CM_FMT_R32F
#define CM_SURFACE_FORMAT_NV12                  VA_CM_FMT_NV12
#define CM_SURFACE_FORMAT_UYVY                  VA_CM_FMT_UYVY
#define CM_SURFACE_FORMAT_YUY2                  VA_CM_FMT_YUY2
#define CM_SURFACE_FORMAT_V8U8                  VA_CM_FMT_V8U8

#define CM_SURFACE_FORMAT_R8_UINT               VA_CM_FMT_R8U
#define CM_SURFACE_FORMAT_R16_UINT              VA_CM_FMT_R16U
#define CM_SURFACE_FORMAT_R16_SINT              VA_CM_FMT_A8L8
#define CM_SURFACE_FORMAT_D16                   VA_CM_FMT_D16
#define CM_SURFACE_FORMAT_L16                   VA_CM_FMT_L16
#define CM_SURFACE_FORMAT_A16B16G16R16          VA_CM_FMT_A16B16G16R16
#define CM_SURFACE_FORMAT_R10G10B10A2           VA_CM_FMT_A2B10G10R10
#define CM_SURFACE_FORMAT_A16B16G16R16F         VA_CM_FMT_A16B16G16R16F

#define CM_SURFACE_FORMAT_444P                  VA_CM_FMT_444P
#define CM_SURFACE_FORMAT_422H                  VA_CM_FMT_422H
#define CM_SURFACE_FORMAT_422V                  VA_CM_FMT_422V
#define CM_SURFACE_FORMAT_411P                  VA_CM_FMT_411P
#define CM_SURFACE_FORMAT_IMC3                  VA_CM_FMT_IMC3
#define CM_SURFACE_FORMAT_YV12                  VA_CM_FMT_YV12
#define CM_SURFACE_FORMAT_P010                  VA_CM_FMT_P010
#define CM_SURFACE_FORMAT_P016                  VA_CM_FMT_P016


#define CM_TEXTURE_ADDRESS_TYPE                 VACMTEXTUREADDRESS
#define CM_TEXTURE_ADDRESS_WRAP                 VACMTADDRESS_WRAP
#define CM_TEXTURE_ADDRESS_MIRROR               VACMTADDRESS_MIRROR
#define CM_TEXTURE_ADDRESS_CLAMP                VACMTADDRESS_CLAMP
#define CM_TEXTURE_ADDRESS_BORDER               VACMTADDRESS_BORDER
#define CM_TEXTURE_ADDRESS_MIRRORONCE           VACMTADDRESS_MIRRORONCE

#define CM_TEXTURE_FILTER_TYPE                  VACMTEXTUREFILTERTYPE
#define CM_TEXTURE_FILTER_TYPE_NONE             VACMTEXF_NONE
#define CM_TEXTURE_FILTER_TYPE_POINT            VACMTEXF_POINT
#define CM_TEXTURE_FILTER_TYPE_LINEAR           VACMTEXF_LINEAR
#define CM_TEXTURE_FILTER_TYPE_ANISOTROPIC      VACMTEXF_ANISOTROPIC
#define CM_TEXTURE_FILTER_TYPE_FLATCUBIC        VACMTEXF_FLATCUBIC
#define CM_TEXTURE_FILTER_TYPE_GAUSSIANCUBIC    VACMTEXF_GAUSSIANCUBIC
#define CM_TEXTURE_FILTER_TYPE_PYRAMIDALQUAD    VACMTEXF_PYRAMIDALQUAD
#define CM_TEXTURE_FILTER_TYPE_GAUSSIANQUAD     VACMTEXF_GAUSSIANQUAD
#define CM_TEXTURE_FILTER_TYPE_CONVOLUTIONMONO  VACMTEXF_CONVOLUTIONMONO

/* Surrport for common-used data type */
#define  _TCHAR char
#define __cdecl
#ifndef TRUE
#define TRUE 1
#endif

typedef int HKEY;

typedef unsigned int uint;
typedef unsigned int* PUINT;

typedef float FLOAT;
typedef unsigned long long DWORDLONG;
#ifndef ULONG_PTR
#define ULONG_PTR unsigned long
#endif

/* Handle Type */
typedef void *HMODULE;
typedef void *HINSTANCE;
typedef int HANDLE;
typedef void *PVOID;
typedef int WINBOOL;
typedef BOOL *PBOOL;
typedef unsigned long ULONG;
typedef ULONG *PULONG;
typedef unsigned short USHORT;
typedef USHORT *PUSHORT;
typedef unsigned char UCHAR;
typedef UCHAR *PUCHAR;
typedef char CHAR;
typedef short SHORT;
typedef long LONG;
typedef double DOUBLE;

#define __int8 char
#define __int16 short
#define __int32 int
#define __int64 long long

typedef unsigned short WORD;
typedef float FLOAT;
typedef FLOAT *PFLOAT;
typedef BYTE *PBYTE;
typedef int *PINT;
typedef WORD *PWORD;
typedef DWORD *PDWORD;
typedef unsigned int *PUINT;
typedef LONG HRESULT;
typedef long long LONGLONG;

typedef union _LARGE_INTEGER {
    struct {
        uint32_t LowPart;
        int32_t HighPart;
    } u;
    int64_t QuadPart;
} LARGE_INTEGER;

//Performance
EXTERN_C INT QueryPerformanceFrequency(LARGE_INTEGER *lpFrequency);
EXTERN_C INT QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount);

struct BITMAPFILEHEADER
{
    WORD  bfType;
    DWORD bfSize;
    WORD  bfReserved1;
    WORD  bfReserved2;
    DWORD bfOffBits;
}  __attribute__((packed));

struct BITMAPINFOHEADER
{
    DWORD biSize;
    DWORD  biWidth;
    DWORD  biHeight;
    WORD  biPlanes;
    WORD  biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    DWORD  biXPelsPerMeter;
    DWORD  biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
};

struct RGBQUAD
{
    BYTE rgbBlue;
    BYTE rgbGreen;
    BYTE rgbRed;
    BYTE rgbReserved;
};


#ifdef CMRT_EMU
// below macro definition is to workaround a bug in g++4.4
template<typename kernelFunctionTy>
inline void * CM_KERNEL_FUNCTION_TO_POINTER(kernelFunctionTy kernelFunction)
{
    return (void *)kernelFunction;
}
#define CM_KERNEL_FUNCTION2(...) #__VA_ARGS__, CM_KERNEL_FUNCTION_TO_POINTER(__VA_ARGS__)
#else
#define CM_KERNEL_FUNCTION2(...) #__VA_ARGS__
#endif

#ifdef CMRT_EMU
// below macro definition is to workaround a bug in g++4.4
template<typename kernelFunctionTy>
inline void * CM_KERNEL_FUNCTION_POINTER(kernelFunctionTy kernelFunction)
{
    return (void *)kernelFunction;
}
#define _NAME(...) #__VA_ARGS__, CM_KERNEL_FUNCTION_POINTER(__VA_ARGS__)
#else
#define _NAME(...) #__VA_ARGS__
#endif

#else // ifdef CM_LINUX

#define CM_ATTRIBUTE(attribute) __declspec(attribute)
#define CM_TYPE_NAME(type)  typeid(type).name()

inline void * CM_ALIGNED_MALLOC(size_t size, size_t alignment)
{
    return _aligned_malloc(size, alignment);
}

inline void CM_ALIGNED_FREE(void * memory)
{
    _aligned_free(memory);
}

#if !CM_METRO
//multi-thread API:
#define THREAD_HANDLE HANDLE
inline void CM_THREAD_CREATE(THREAD_HANDLE *handle, void * start_routine, void * arg)
{
    handle[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start_routine, (LPVOID)arg, 0, NULL);
}
inline void CM_THREAD_EXIT(void * retval)
{
    ExitThread(0);
}

inline int CM_THREAD_JOIN(THREAD_HANDLE *handle_array, int thread_cnt)
{
    DWORD ret = WaitForMultipleObjects(thread_cnt, handle_array, true, INFINITE);
    return ret;
}
#endif

#define CM_SURFACE_FORMAT                       D3DFORMAT

#define CM_SURFACE_FORMAT_UNKNOWN               D3DFMT_UNKNOWN
#define CM_SURFACE_FORMAT_A8R8G8B8              D3DFMT_A8R8G8B8
#define CM_SURFACE_FORMAT_X8R8G8B8              D3DFMT_X8R8G8B8
#define CM_SURFACE_FORMAT_A8B8G8R8              D3DFMT_A8B8G8R8
#define CM_SURFACE_FORMAT_A8                    D3DFMT_A8
#define CM_SURFACE_FORMAT_P8                    D3DFMT_P8
#define CM_SURFACE_FORMAT_R32F                  D3DFMT_R32F
#define CM_SURFACE_FORMAT_NV12                  (D3DFORMAT)MAKEFOURCC('N','V','1','2')
#define CM_SURFACE_FORMAT_P016                  (D3DFORMAT)MAKEFOURCC('P','0','1','6')
#define CM_SURFACE_FORMAT_P010                  (D3DFORMAT)MAKEFOURCC('P','0','1','0')
#define CM_SURFACE_FORMAT_UYVY                  D3DFMT_UYVY
#define CM_SURFACE_FORMAT_YUY2                  D3DFMT_YUY2
#define CM_SURFACE_FORMAT_V8U8                  D3DFMT_V8U8
#define CM_SURFACE_FORMAT_A8L8                  D3DFMT_A8L8
#define CM_SURFACE_FORMAT_D16                   D3DFMT_D16
#define CM_SURFACE_FORMAT_L16                   D3DFMT_L16
#define CM_SURFACE_FORMAT_A16B16G16R16          D3DFMT_A16B16G16R16
#define CM_SURFACE_FORMAT_A16B16G16R16F         D3DFMT_A16B16G16R16F
#define CM_SURFACE_FORMAT_R10G10B10A2           D3DFMT_A2B10G10R10
#define CM_SURFACE_FORMAT_IRW0                  (D3DFORMAT)MAKEFOURCC('I','R','W','0')
#define CM_SURFACE_FORMAT_IRW1                  (D3DFORMAT)MAKEFOURCC('I','R','W','1')
#define CM_SURFACE_FORMAT_IRW2                  (D3DFORMAT)MAKEFOURCC('I','R','W','2')
#define CM_SURFACE_FORMAT_IRW3                  (D3DFORMAT)MAKEFOURCC('I','R','W','3')
#define CM_SURFACE_FORMAT_R16_FLOAT             D3DFMT_R16F  //beryl
#define CM_SURFACE_FORMAT_A8P8                  D3DFMT_A8P8
#define CM_SURFACE_FORMAT_I420                  (D3DFORMAT)MAKEFOURCC('I','4','2','0')
#define CM_SURFACE_FORMAT_IMC3                  (D3DFORMAT)MAKEFOURCC('I','M','C','3')
#define CM_SURFACE_FORMAT_IA44                  (D3DFORMAT)MAKEFOURCC('I','A','4','4')
#define CM_SURFACE_FORMAT_AI44                  (D3DFORMAT)MAKEFOURCC('A','I','4','4')
#define CM_SURFACE_FORMAT_P216                  (D3DFORMAT)MAKEFOURCC('P','2','1','6')
#define CM_SURFACE_FORMAT_Y410                  (D3DFORMAT)MAKEFOURCC('Y','4','1','0')
#define CM_SURFACE_FORMAT_Y416                  (D3DFORMAT)MAKEFOURCC('Y','4','1','6')
#define CM_SURFACE_FORMAT_Y210                  (D3DFORMAT)MAKEFOURCC('Y','2','1','0')
#define CM_SURFACE_FORMAT_Y216                  (D3DFORMAT)MAKEFOURCC('Y','2','1','6')
#define CM_SURFACE_FORMAT_AYUV                  (D3DFORMAT)MAKEFOURCC('A','Y','U','V')
#define CM_SURFACE_FORMAT_YV12                  (D3DFORMAT)MAKEFOURCC('Y','V','1','2')
#define CM_SURFACE_FORMAT_400P                  (D3DFORMAT)MAKEFOURCC('4','0','0','P')
#define CM_SURFACE_FORMAT_411P                  (D3DFORMAT)MAKEFOURCC('4','1','1','P')
#define CM_SURFACE_FORMAT_411R                  (D3DFORMAT)MAKEFOURCC('4','1','1','R')
#define CM_SURFACE_FORMAT_422H                  (D3DFORMAT)MAKEFOURCC('4','2','2','H')
#define CM_SURFACE_FORMAT_422V                  (D3DFORMAT)MAKEFOURCC('4','2','2','V')
#define CM_SURFACE_FORMAT_444P                  (D3DFORMAT)MAKEFOURCC('4','4','4','P')
#define CM_SURFACE_FORMAT_RGBP                  (D3DFORMAT)MAKEFOURCC('R','G','B','P')
#define CM_SURFACE_FORMAT_BGRP                  (D3DFORMAT)MAKEFOURCC('B','G','R','P')

#define CM_TEXTURE_ADDRESS_TYPE                 D3DTEXTUREADDRESS

#define CM_TEXTURE_ADDRESS_WRAP                 D3DTADDRESS_WRAP
#define CM_TEXTURE_ADDRESS_MIRROR               D3DTADDRESS_MIRROR
#define CM_TEXTURE_ADDRESS_CLAMP                D3DTADDRESS_CLAMP
#define CM_TEXTURE_ADDRESS_BORDER               D3DTADDRESS_BORDER
#define CM_TEXTURE_ADDRESS_MIRRORONCE           D3DTADDRESS_MIRRORONCE

#define CM_TEXTURE_FILTER_TYPE                  D3DTEXTUREFILTERTYPE

#define CM_TEXTURE_FILTER_TYPE_NONE             D3DTEXF_NONE
#define CM_TEXTURE_FILTER_TYPE_POINT            D3DTEXF_POINT
#define CM_TEXTURE_FILTER_TYPE_LINEAR           D3DTEXF_LINEAR
#define CM_TEXTURE_FILTER_TYPE_ANISOTROPIC      D3DTEXF_ANISOTROPIC
#define CM_TEXTURE_FILTER_TYPE_FLATCUBIC        D3DTEXF_FLATCUBIC
#define CM_TEXTURE_FILTER_TYPE_GAUSSIANCUBIC    D3DTEXF_GAUSSIANCUBIC
#define CM_TEXTURE_FILTER_TYPE_PYRAMIDALQUAD    D3DTEXF_PYRAMIDALQUAD
#define CM_TEXTURE_FILTER_TYPE_GAUSSIANQUAD     D3DTEXF_GAUSSIANQUAD
#define CM_TEXTURE_FILTER_TYPE_CONVOLUTIONMONO  D3DTEXF_CONVOLUTIONMONO


#ifdef CMRT_EMU
// FIXME CM_KERNEL_FUNCTION macro provides wrong kernel name w/ unexplicit template like CM_KERNEL_FUNCTION(kernel<T>)
#define CM_KERNEL_FUNCTION2(...) #__VA_ARGS__, (void *)(void (__cdecl *) (void))__VA_ARGS__
#else
#define CM_KERNEL_FUNCTION2(...) #__VA_ARGS__
#endif

#ifdef CMRT_EMU
#define _NAME(...) #__VA_ARGS__, (void (__cdecl *)(void))__VA_ARGS__
#else
#define _NAME(...) #__VA_ARGS__
#endif

#endif

#define CM_RT_API

#define CM_KERNEL_FUNCTION(...) CM_KERNEL_FUNCTION2(__VA_ARGS__)

#define CM_1_0          100
#define CM_2_0          200
#define CM_2_1          201
#define CM_2_2          202
#define CM_2_3          203
#define CM_2_4          204
#define CM_3_0          300
#define CM_4_0          400
#define CM_5_0          500
#define CM_6_0          600
#define CM_7_0          700

/* Return codes */

#define CM_SUCCESS                                  0
#define CM_FAILURE                                  -1
#define CM_NOT_IMPLEMENTED                          -2
#define CM_SURFACE_ALLOCATION_FAILURE               -3
#define CM_OUT_OF_HOST_MEMORY                       -4
#define CM_SURFACE_FORMAT_NOT_SUPPORTED             -5
#define CM_EXCEED_SURFACE_AMOUNT                    -6
#define CM_EXCEED_KERNEL_ARG_AMOUNT                 -7
#define CM_EXCEED_KERNEL_ARG_SIZE_IN_BYTE           -8
#define CM_INVALID_ARG_INDEX                        -9
#define CM_INVALID_ARG_VALUE                        -10
#define CM_INVALID_ARG_SIZE                         -11
#define CM_INVALID_THREAD_INDEX                     -12
#define CM_INVALID_WIDTH                            -13
#define CM_INVALID_HEIGHT                           -14
#define CM_INVALID_DEPTH                            -15
#define CM_INVALID_COMMON_ISA                       -16
#define CM_D3DDEVICEMGR_OPEN_HANDLE_FAIL            -17 // IDirect3DDeviceManager9::OpenDeviceHandle fail
#define CM_D3DDEVICEMGR_DXVA2_E_NEW_VIDEO_DEVICE    -18 // IDirect3DDeviceManager9::LockDevice return DXVA2_E_VIDEO_DEVICE_LOCKED
#define CM_D3DDEVICEMGR_LOCK_DEVICE_FAIL            -19 // IDirect3DDeviceManager9::LockDevice return other fail than DXVA2_E_VIDEO_DEVICE_LOCKED
#define CM_EXCEED_SAMPLER_AMOUNT                    -20
#define CM_EXCEED_MAX_KERNEL_PER_ENQUEUE            -21
#define CM_EXCEED_MAX_KERNEL_SIZE_IN_BYTE           -22
#define CM_EXCEED_MAX_THREAD_AMOUNT_PER_ENQUEUE     -23
#define CM_EXCEED_VME_STATE_G6_AMOUNT               -24
#define CM_INVALID_THREAD_SPACE                     -25
#define CM_EXCEED_MAX_TIMEOUT                       -26
#define CM_JITDLL_LOAD_FAILURE                      -27
#define CM_JIT_COMPILE_FAILURE                      -28
#define CM_JIT_COMPILESIM_FAILURE                   -29
#define CM_INVALID_THREAD_GROUP_SPACE               -30
#define CM_THREAD_ARG_NOT_ALLOWED                   -31
#define CM_INVALID_GLOBAL_BUFFER_INDEX              -32
#define CM_INVALID_BUFFER_HANDLER                   -33
#define CM_EXCEED_MAX_SLM_SIZE                      -34
#define CM_JITDLL_OLDER_THAN_ISA                    -35
#define CM_INVALID_HARDWARE_THREAD_NUMBER           -36
#define CM_GTPIN_INVOKE_FAILURE                     -37
#define CM_INVALIDE_L3_CONFIGURATION                -38
#define CM_INVALID_D3D11_TEXTURE2D_USAGE            -39
#define CM_INTEL_GFX_NOTFOUND                       -40
#define CM_GPUCOPY_INVALID_SYSMEM                   -41
#define CM_GPUCOPY_INVALID_WIDTH                    -42
#define CM_GPUCOPY_INVALID_STRIDE                   -43
#define CM_EVENT_DRIVEN_FAILURE                     -44
#define CM_LOCK_SURFACE_FAIL                        -45 // Lock surface failed
#define CM_INVALID_GENX_BINARY                      -46
#define CM_FEATURE_NOT_SUPPORTED_IN_DRIVER          -47 // driver out-of-sync
#define CM_QUERY_DLL_VERSION_FAILURE                -48 //Fail in getting DLL file version
#define CM_KERNELPAYLOAD_PERTHREADARG_MUTEX_FAIL    -49
#define CM_KERNELPAYLOAD_PERKERNELARG_MUTEX_FAIL    -50
#define CM_KERNELPAYLOAD_SETTING_FAILURE            -51
#define CM_KERNELPAYLOAD_SURFACE_INVALID_BTINDEX    -52
#define CM_NOT_SET_KERNEL_ARGUMENT                  -53
#define CM_GPUCOPY_INVALID_SURFACES                 -54
#define CM_GPUCOPY_INVALID_SIZE                     -55
#define CM_GPUCOPY_OUT_OF_RESOURCE                  -56
#define CM_DEVICE_INVALID_D3DDEVICE                 -57
#define CM_SURFACE_DELAY_DESTROY                    -58
#define CM_INVALID_VEBOX_STATE                      -59
#define CM_INVALID_VEBOX_SURFACE                    -60
#define CM_FEATURE_NOT_SUPPORTED_BY_HARDWARE        -61
#define CM_RESOURCE_USAGE_NOT_SUPPORT_READWRITE     -62
#define CM_MULTIPLE_MIPLEVELS_NOT_SUPPORTED         -63
#define CM_INVALID_UMD_CONTEXT                      -64
#define CM_INVALID_LIBVA_SURFACE                    -65
#define CM_INVALID_LIBVA_INITIALIZE                 -66
#define CM_KERNEL_THREADSPACE_NOT_SET               -67
#define CM_INVALID_KERNEL_THREADSPACE               -68
#define CM_KERNEL_THREADSPACE_THREADS_NOT_ASSOCIATED -69
#define CM_KERNEL_THREADSPACE_INTEGRITY_FAILED      -70
#define CM_INVALID_USERPROVIDED_GENBINARY           -71
#define CM_INVALID_PRIVATE_DATA                     -72
#define CM_INVALID_MOS_RESOURCE_HANDLE              -73
#define CM_SURFACE_CACHED                           -74
#define CM_SURFACE_IN_USE                           -75
#define CM_INVALID_GPUCOPY_KERNEL                   -76
#define CM_INVALID_DEPENDENCY_WITH_WALKING_PATTERN  -77
#define CM_INVALID_MEDIA_WALKING_PATTERN            -78
#define CM_FAILED_TO_ALLOCATE_SVM_BUFFER            -79
#define CM_EXCEED_MAX_POWER_OPTION_FOR_PLATFORM     -80
#define CM_INVALID_KERNEL_THREADGROUPSPACE          -81
#define CM_INVALID_KERNEL_SPILL_CODE                -82
#define CM_UMD_DRIVER_NOT_SUPPORTED                 -83
#define CM_INVALID_GPU_FREQUENCY_VALUE              -84
#define CM_SYSTEM_MEMORY_NOT_4KPAGE_ALIGNED         -85
#define CM_KERNEL_ARG_SETTING_FAILED                -86
#define CM_NO_AVAILABLE_SURFACE                     -87
#define CM_VA_SURFACE_NOT_SUPPORTED                 -88
#define CM_TOO_MUCH_THREADS                         -89
#define CM_NULL_POINTER                             -90
#define CM_EXCEED_MAX_NUM_2D_ALIASES                -91
#define CM_INVALID_PARAM_SIZE                       -92
#define CM_GT_UNSUPPORTED                           -93
#define CM_GTPIN_FLAG_NO_LONGER_SUPPORTED           -94
#define CM_PLATFORM_UNSUPPORTED_FOR_API             -95
#define CM_TASK_MEDIA_RESET                         -96
#define CM_KERNELPAYLOAD_SAMPLER_INVALID_BTINDEX    -97
#define CM_EXCEED_MAX_NUM_BUFFER_ALIASES            -98
#define CM_SYSTEM_MEMORY_NOT_4PIXELS_ALIGNED        -99
#define CM_FAILED_TO_CREATE_CURBE_SURFACE           -100
#define CM_INVALID_CAP_NAME                         -101

/* End of return codes */

typedef enum _GPU_PLATFORM {
    PLATFORM_INTEL_UNKNOWN = 0,
    PLATFORM_INTEL_SNB = 1,   //Sandy Bridge
    PLATFORM_INTEL_IVB = 2,   //Ivy Bridge
    PLATFORM_INTEL_HSW = 3,   //Haswell
    PLATFORM_INTEL_BDW = 4,   //Broadwell
    PLATFORM_INTEL_VLV = 5,   //ValleyView
    PLATFORM_INTEL_CHV = 6,   //CherryView
    PLATFORM_INTEL_SKL = 7,   //SKL
    PLATFORM_INTEL_BXT = 8,   //Broxton
    PLATFORM_INTEL_CNL = 9,   //Cannonlake
    PLATFORM_INTEL_ICL = 10,  //Icelake
    PLATFORM_INTEL_KBL = 11,  //Kabylake
    PLATFORM_INTEL_GLV = 12,  //Glenview
    PLATFORM_INTEL_ICLLP = 13,  //IcelakeLP
    PLATFORM_INTEL_GLK = 16,   //GeminiLake
    PLATFORM_INTEL_CFL = 17  //CofeeLake
} GPU_PLATFORM;

//Time in seconds before kernel should timeout
#define CM_MAX_TIMEOUT 2
//Time in milliseconds before kernel should timeout
#define CM_MAX_TIMEOUT_MS CM_MAX_TIMEOUT*1000
#define CM_MAX_TIMEOUT_SIM    600000

typedef enum _CM_STATUS
{
    CM_STATUS_QUEUED = 0,
    CM_STATUS_FLUSHED = 1,
    CM_STATUS_FINISHED = 2,
    CM_STATUS_STARTED = 3
} CM_STATUS;

typedef struct _CM_SAMPLER_STATE
{
    CM_TEXTURE_FILTER_TYPE minFilterType;
    CM_TEXTURE_FILTER_TYPE magFilterType;
    CM_TEXTURE_ADDRESS_TYPE addressU;
    CM_TEXTURE_ADDRESS_TYPE addressV;
    CM_TEXTURE_ADDRESS_TYPE addressW;
} CM_SAMPLER_STATE;

typedef enum _CM_DEVICE_CAP_NAME
{
    CAP_KERNEL_COUNT_PER_TASK,
    CAP_KERNEL_BINARY_SIZE,
    CAP_SAMPLER_COUNT,
    CAP_SAMPLER_COUNT_PER_KERNEL,
    CAP_BUFFER_COUNT,
    CAP_SURFACE2D_COUNT,
    CAP_SURFACE3D_COUNT,
    CAP_SURFACE_COUNT_PER_KERNEL,
    CAP_ARG_COUNT_PER_KERNEL,
    CAP_ARG_SIZE_PER_KERNEL,
    CAP_USER_DEFINED_THREAD_COUNT_PER_TASK,
    CAP_HW_THREAD_COUNT,
    CAP_SURFACE2D_FORMAT_COUNT,
    CAP_SURFACE2D_FORMATS,
    CAP_SURFACE3D_FORMAT_COUNT,
    CAP_SURFACE3D_FORMATS,
    CAP_VME_STATE_COUNT,
    CAP_GPU_PLATFORM,
    CAP_GT_PLATFORM,
    CAP_MIN_FREQUENCY,
    CAP_MAX_FREQUENCY,
    CAP_L3_CONFIG,
    CAP_GPU_CURRENT_FREQUENCY,
    CAP_USER_DEFINED_THREAD_COUNT_PER_TASK_NO_THREAD_ARG,
    CAP_USER_DEFINED_THREAD_COUNT_PER_MEDIA_WALKER,
    CAP_USER_DEFINED_THREAD_COUNT_PER_THREAD_GROUP,
    CAP_SURFACE2DUP_COUNT,
    CAP_PLATFORM_INFO,
    CAP_MAX_BUFFER_SIZE
} CM_DEVICE_CAP_NAME;


typedef enum _CM_DEPENDENCY_PATTERN
{
    CM_NONE_DEPENDENCY = 0,    //All threads run parallel, scanline dispatch
    CM_WAVEFRONT = 1,
    CM_WAVEFRONT26 = 2,
    CM_VERTICAL_DEPENDENCY = 3,
    CM_HORIZONTAL_DEPENDENCY = 4,
} CM_DEPENDENCY_PATTERN;

typedef enum _CM_SAMPLER8x8_SURFACE_
{
    CM_AVS_SURFACE = 0,
    CM_VA_SURFACE = 1
}CM_SAMPLER8x8_SURFACE;

typedef enum _CM_SURFACE_ADDRESS_CONTROL_MODE_
{
    CM_SURFACE_CLAMP = 0,
    CM_SURFACE_MIRROR = 1
}CM_SURFACE_ADDRESS_CONTROL_MODE;

typedef struct _CM_SURFACE_DETAILS {
    UINT        width;
    UINT        height;
    UINT        depth;
    CM_SURFACE_FORMAT   format;
    UINT        planeIndex;
    UINT        pitch;
    UINT        slicePitch;
    UINT        SurfaceBaseAddress;
    UINT8       TiledSurface;
    UINT8       TileWalk;
    UINT        XOffset;
    UINT        YOffset;

}CM_SURFACE_DETAILS;

typedef struct _CM_AVS_COEFF_TABLE {
    float   FilterCoeff_0_0;
    float   FilterCoeff_0_1;
    float   FilterCoeff_0_2;
    float   FilterCoeff_0_3;
    float   FilterCoeff_0_4;
    float   FilterCoeff_0_5;
    float   FilterCoeff_0_6;
    float   FilterCoeff_0_7;
}CM_AVS_COEFF_TABLE;

typedef struct _CM_AVS_INTERNEL_COEFF_TABLE {
    BYTE   FilterCoeff_0_0;
    BYTE   FilterCoeff_0_1;
    BYTE   FilterCoeff_0_2;
    BYTE   FilterCoeff_0_3;
    BYTE   FilterCoeff_0_4;
    BYTE   FilterCoeff_0_5;
    BYTE   FilterCoeff_0_6;
    BYTE   FilterCoeff_0_7;
}CM_AVS_INTERNEL_COEFF_TABLE;

typedef struct _CM_CONVOLVE_COEFF_TABLE {
    float   FilterCoeff_0_0;
    float   FilterCoeff_0_1;
    float   FilterCoeff_0_2;
    float   FilterCoeff_0_3;
    float   FilterCoeff_0_4;
    float   FilterCoeff_0_5;
    float   FilterCoeff_0_6;
    float   FilterCoeff_0_7;
    float   FilterCoeff_0_8;
    float   FilterCoeff_0_9;
    float   FilterCoeff_0_10;
    float   FilterCoeff_0_11;
    float   FilterCoeff_0_12;
    float   FilterCoeff_0_13;
    float   FilterCoeff_0_14;
    float   FilterCoeff_0_15;
    float   FilterCoeff_0_16;
    float   FilterCoeff_0_17;
    float   FilterCoeff_0_18;
    float   FilterCoeff_0_19;
    float   FilterCoeff_0_20;
    float   FilterCoeff_0_21;
    float   FilterCoeff_0_22;
    float   FilterCoeff_0_23;
    float   FilterCoeff_0_24;
    float   FilterCoeff_0_25;
    float   FilterCoeff_0_26;
    float   FilterCoeff_0_27;
    float   FilterCoeff_0_28;
    float   FilterCoeff_0_29;
    float   FilterCoeff_0_30;
    float   FilterCoeff_0_31;
}CM_CONVOLVE_COEFF_TABLE;

/*
*   MISC SAMPLER8x8 State
*/
typedef struct _CM_MISC_STATE {
    //DWORD 0
    union {
        struct {
            DWORD   Row0 : 16;
            DWORD   Reserved : 8;
            DWORD   Width : 4;
            DWORD   Height : 4;
        };
        struct {
            DWORD value;
        };
    } DW0;

    //DWORD 1
    union {
        struct {
            DWORD   Row1 : 16;
            DWORD   Row2 : 16;
        };
        struct {
            DWORD value;
        };
    } DW1;

    //DWORD 2
    union {
        struct {
            DWORD   Row3 : 16;
            DWORD   Row4 : 16;
        };
        struct {
            DWORD value;
        };
    } DW2;

    //DWORD 3
    union {
        struct {
            DWORD   Row5 : 16;
            DWORD   Row6 : 16;
        };
        struct {
            DWORD value;
        };
    } DW3;

    //DWORD 4
    union {
        struct {
            DWORD   Row7 : 16;
            DWORD   Row8 : 16;
        };
        struct {
            DWORD value;
        };
    } DW4;

    //DWORD 5
    union {
        struct {
            DWORD   Row9 : 16;
            DWORD   Row10 : 16;
        };
        struct {
            DWORD value;
        };
    } DW5;

    //DWORD 6
    union {
        struct {
            DWORD   Row11 : 16;
            DWORD   Row12 : 16;
        };
        struct {
            DWORD value;
        };
    } DW6;

    //DWORD 7
    union {
        struct {
            DWORD   Row13 : 16;
            DWORD   Row14 : 16;
        };
        struct {
            DWORD value;
        };
    } DW7;
} CM_MISC_STATE;

typedef struct _CM_MISC_STATE_MSG {
    //DWORD 0
    union {
        struct {
            DWORD   Row0 : 16;
            DWORD   Reserved : 8;
            DWORD   Width : 4;
            DWORD   Height : 4;
        };
        struct {
            DWORD value;
        };
    }DW0;

    //DWORD 1
    union {
        struct {
            DWORD   Row1 : 16;
            DWORD   Row2 : 16;
        };
        struct {
            DWORD value;
        };
    }DW1;

    //DWORD 2
    union {
        struct {
            DWORD   Row3 : 16;
            DWORD   Row4 : 16;
        };
        struct {
            DWORD value;
        };
    }DW2;

    //DWORD 3
    union {
        struct {
            DWORD   Row5 : 16;
            DWORD   Row6 : 16;
        };
        struct {
            DWORD value;
        };
    }DW3;

    //DWORD 4
    union {
        struct {
            DWORD   Row7 : 16;
            DWORD   Row8 : 16;
        };
        struct {
            DWORD value;
        };
    }DW4;

    //DWORD 5
    union {
        struct {
            DWORD   Row9 : 16;
            DWORD   Row10 : 16;
        };
        struct {
            DWORD value;
        };
    }DW5;

    //DWORD 6
    union {
        struct {
            DWORD   Row11 : 16;
            DWORD   Row12 : 16;
        };
        struct {
            DWORD value;
        };
    }DW6;

    //DWORD 7
    union {
        struct {
            DWORD   Row13 : 16;
            DWORD   Row14 : 16;
        };
        struct {
            DWORD value;
        };
    }DW7;
} CM_MISC_STATE_MSG;

typedef enum _CM_FASTCOPY_OPTION
{
    CM_FASTCOPY_OPTION_NONBLOCKING  = 0x00,
    CM_FASTCOPY_OPTION_BLOCKING     = 0x01
} CM_FASTCOPY_OPTION;

/* End of Applicable for old and new CMAPI */


#ifdef CMAPIUPDATE

#include <cstdint>
#include <cstddef>

#define CM_MIN_SURF_WIDTH   1
#define CM_MIN_SURF_HEIGHT  1
#define CM_MIN_SURF_DEPTH   2

#define CM_MAX_1D_SURF_WIDTH  0X40000000 // 2^30

//IVB+
#define CM_MAX_2D_SURF_WIDTH    16384
#define CM_MAX_2D_SURF_HEIGHT   16384

#define CM_MAX_3D_SURF_WIDTH    2048
#define CM_MAX_3D_SURF_HEIGHT   2048
#define CM_MAX_3D_SURF_DEPTH    2048

#define CM_MAX_OPTION_SIZE_IN_BYTE          512
#define CM_MAX_KERNEL_NAME_SIZE_IN_BYTE     256
#define CM_MAX_ISA_FILE_NAME_SIZE_IN_BYTE   256

#define CM_MAX_THREADSPACE_WIDTH_FOR_MW        511
#define CM_MAX_THREADSPACE_HEIGHT_FOR_MW       511
#define CM_MAX_THREADSPACE_WIDTH_FOR_MO        512
#define CM_MAX_THREADSPACE_HEIGHT_FOR_MO       512
#define CM_MAX_THREADSPACE_WIDTH_SKLUP_FOR_MW  2047
#define CM_MAX_THREADSPACE_HEIGHT_SKLUP_FOR_MW 2047

class CmEvent;
#define CM_CALLBACK __cdecl
typedef void (CM_CALLBACK *callback_function)(CmEvent*, void *);

extern class CmEvent *CM_NO_EVENT;

struct CM_SAMPLER_STATE_EX;

// Cm Device Create Option
#define CM_DEVICE_CREATE_OPTION_DEFAULT                     0
#define CM_DEVICE_CREATE_OPTION_SCRATCH_SPACE_DISABLE       1
#define CM_DEVICE_CREATE_OPTION_TDR_DISABLE                 64  //Work only for DX11 so far

#define CM_DEVICE_CREATE_OPTION_SURFACE_REUSE_ENABLE        1024

#define CM_DEVICE_CONFIG_MIDTHREADPREEMPTION_OFFSET           22
#define CM_DEVICE_CONFIG_MIDTHREADPREEMPTION_DISENABLE         (1 << CM_DEVICE_CONFIG_MIDTHREADPREEMPTION_OFFSET)

struct CM_VME_SURFACE_STATE_PARAM;

#define CM_MAX_DEPENDENCY_COUNT         8
#define CM_NUM_DWORD_FOR_MW_PARAM       16

/**************** L3/Cache ***************/
typedef enum _MEMORY_OBJECT_CONTROL
{
    MEMORY_OBJECT_CONTROL_SKL_DEFAULT = 0,
    MEMORY_OBJECT_CONTROL_SKL_NO_L3,
    MEMORY_OBJECT_CONTROL_SKL_NO_LLC_ELLC,
    MEMORY_OBJECT_CONTROL_SKL_NO_LLC,
    MEMORY_OBJECT_CONTROL_SKL_NO_ELLC,
    MEMORY_OBJECT_CONTROL_SKL_NO_LLC_L3,
    MEMORY_OBJECT_CONTROL_SKL_NO_ELLC_L3,
    MEMORY_OBJECT_CONTROL_SKL_NO_CACHE,
    MEMORY_OBJECT_CONTROL_SKL_COUNT,

    MEMORY_OBJECT_CONTROL_ICL_DEFAULT = 0,
    MEMORY_OBJECT_CONTROL_ICL_COUNT,

    MEMORY_OBJECT_CONTROL_UNKNOWN = 0xff
} MEMORY_OBJECT_CONTROL;

typedef enum _MEMORY_TYPE {
    CM_USE_PTE,
    CM_UN_CACHEABLE,
    CM_WRITE_THROUGH,
    CM_WRITE_BACK,

    // BDW
    MEMORY_TYPE_BDW_UC_WITH_FENCE = 0,
    MEMORY_TYPE_BDW_UC,
    MEMORY_TYPE_BDW_WT,
    MEMORY_TYPE_BDW_WB

} MEMORY_TYPE;

struct L3ConfigRegisterValues
{
    unsigned int config_register0;
    unsigned int config_register1;
    unsigned int config_register2;
    unsigned int config_register3;
};

typedef enum _L3_SUGGEST_CONFIG
{
    BDW_L3_PLANE_DEFAULT = 0,
    BDW_L3_PLANE_1,
    BDW_L3_PLANE_2,
    BDW_L3_PLANE_3,
    BDW_L3_PLANE_4,
    BDW_L3_PLANE_5,
    BDW_L3_PLANE_6,
    BDW_L3_PLANE_7,
    BDW_L3_CONFIG_COUNT,

    SKL_L3_PLANE_DEFAULT = 0,
    SKL_L3_PLANE_1,
    SKL_L3_PLANE_2,
    SKL_L3_PLANE_3,
    SKL_L3_PLANE_4,
    SKL_L3_PLANE_5,
    SKL_L3_PLANE_6,
    SKL_L3_PLANE_7,
    SKL_L3_CONFIG_COUNT,


    ICL_L3_PLANE_DEFAULT = 0,
    ICL_L3_PLANE_1,
    ICL_L3_PLANE_2,
    ICL_L3_PLANE_3,
    ICL_L3_PLANE_4,
    ICL_L3_PLANE_5,
    ICL_L3_CONFIG_COUNT,

    ICLLP_L3_PLANE_DEFAULT = 0,
    ICLLP_L3_PLANE_1,
    ICLLP_L3_PLANE_2,
    ICLLP_L3_PLANE_3,
    ICLLP_L3_PLANE_4,
    ICLLP_L3_PLANE_5,
    ICLLP_L3_PLANE_6,
    ICLLP_L3_PLANE_7,
    ICLLP_L3_PLANE_8,
    ICLLP_L3_CONFIG_COUNT,
    BDW_SLM_PLANE_DEFAULT = BDW_L3_PLANE_5,
    SKL_SLM_PLANE_DEFAULT = SKL_L3_PLANE_5
} L3_SUGGEST_CONFIG;

/*
*  AVS SAMPLER8x8 STATE
*/
#define CM_NUM_COEFF_ROWS 17
#define CM_NUM_COEFF_ROWS_SKL 32
typedef struct _CM_AVS_NONPIPLINED_STATE {
    bool BypassXAF;
    bool BypassYAF;
    BYTE DefaultSharpLvl;
    BYTE maxDerivative4Pixels;
    BYTE maxDerivative8Pixels;
    BYTE transitionArea4Pixels;
    BYTE transitionArea8Pixels;
    CM_AVS_COEFF_TABLE Tbl0X[CM_NUM_COEFF_ROWS_SKL];
    CM_AVS_COEFF_TABLE Tbl0Y[CM_NUM_COEFF_ROWS_SKL];
    CM_AVS_COEFF_TABLE Tbl1X[CM_NUM_COEFF_ROWS_SKL];
    CM_AVS_COEFF_TABLE Tbl1Y[CM_NUM_COEFF_ROWS_SKL];
    bool bEnableRGBAdaptive;
    bool bAdaptiveFilterAllChannels;
}CM_AVS_NONPIPLINED_STATE;

typedef struct _CM_AVS_INTERNEL_NONPIPLINED_STATE {
    bool BypassXAF;
    bool BypassYAF;
    BYTE DefaultSharpLvl;
    BYTE maxDerivative4Pixels;
    BYTE maxDerivative8Pixels;
    BYTE transitionArea4Pixels;
    BYTE transitionArea8Pixels;
    CM_AVS_INTERNEL_COEFF_TABLE Tbl0X[CM_NUM_COEFF_ROWS_SKL];
    CM_AVS_INTERNEL_COEFF_TABLE Tbl0Y[CM_NUM_COEFF_ROWS_SKL];
    CM_AVS_INTERNEL_COEFF_TABLE Tbl1X[CM_NUM_COEFF_ROWS_SKL];
    CM_AVS_INTERNEL_COEFF_TABLE Tbl1Y[CM_NUM_COEFF_ROWS_SKL];
    bool bEnableRGBAdaptive;
    bool bAdaptiveFilterAllChannels;
}CM_AVS_INTERNEL_NONPIPLINED_STATE;

typedef struct _CM_AVS_STATE_MSG {
    bool AVSTYPE; //true nearest, false adaptive
    bool EightTapAFEnable; //HSW+
    bool BypassIEF; //ignored for BWL, moved to sampler8x8 payload.
    bool ShuffleOutputWriteback; //SKL mode only to be set when AVS msg sequence is 4x4 or 8x4
    bool HDCDirectWriteEnable;
    unsigned short GainFactor;
    unsigned char GlobalNoiseEstm;
    unsigned char StrongEdgeThr;
    unsigned char WeakEdgeThr;
    unsigned char StrongEdgeWght;
    unsigned char RegularWght;
    unsigned char NonEdgeWght;
    unsigned short wR3xCoefficient;
    unsigned short wR3cCoefficient;
    unsigned short wR5xCoefficient;
    unsigned short wR5cxCoefficient;
    unsigned short wR5cCoefficient;
    //For Non-piplined states
    unsigned short stateID;
    CM_AVS_NONPIPLINED_STATE * AvsState;
} CM_AVS_STATE_MSG;

typedef BYTE CM_AVS_STATE_MSG_EX[4244];
/*
*  CONVOLVE STATE DATA STRUCTURES
*/

//*-----------------------------------------------------------------------------
//| CM Convolve type for SKL+
//*-----------------------------------------------------------------------------
typedef enum _CM_CONVOLVE_SKL_TYPE
{
    CM_CONVOLVE_SKL_TYPE_2D = 0,
    CM_CONVOLVE_SKL_TYPE_1D = 1,
    CM_CONVOLVE_SKL_TYPE_1P = 2
} CM_CONVOLVE_SKL_TYPE;

#define CM_NUM_CONVOLVE_ROWS 16
#define CM_NUM_CONVOLVE_ROWS_SKL 32
typedef struct _CM_CONVOLVE_STATE_MSG {
    bool CoeffSize; //true 16-bit, false 8-bit
    byte SclDwnValue; //Scale down value
    byte Width; //Kernel Width
    byte Height; //Kernel Height
                 //SKL mode
    bool isVertical32Mode;
    bool isHorizontal32Mode;
    CM_CONVOLVE_SKL_TYPE nConvolveType;
    CM_CONVOLVE_COEFF_TABLE Table[CM_NUM_CONVOLVE_ROWS_SKL];
} CM_CONVOLVE_STATE_MSG;

enum CM_SAMPLER_STATE_TYPE {
    CM_SAMPLER8X8_AVS = 0,
    CM_SAMPLER8X8_CONV = 1,
    CM_SAMPLER8X8_MISC = 3,
    CM_SAMPLER8X8_CONV1DH = 4,
    CM_SAMPLER8X8_CONV1DV = 5,
    CM_SAMPLER8X8_AVS_EX = 6,
    CM_SAMPLER8X8_NONE
};

struct CM_SAMPLER_8X8_DESCR {
    CM_SAMPLER_STATE_TYPE stateType;
    union {
        CM_AVS_STATE_MSG *avs;
        CM_AVS_STATE_MSG_EX *avs_ex;
        CM_CONVOLVE_STATE_MSG *conv;
        CM_MISC_STATE_MSG *misc; //ERODE/DILATE/MINMAX
    };
};


struct CM_FLAG;

#define CM_NULL_SURFACE                     0xFFFF

//!
//! CM Sampler8X8
//!
class CmSampler8x8
{
public:
    CM_RT_API virtual INT GetIndex(SamplerIndex* & pIndex) = 0;

};
/***********END SAMPLER8X8********************/

class CmEvent
{
public:
    CM_RT_API virtual INT GetStatus(CM_STATUS& status) = 0;
    CM_RT_API virtual INT GetExecutionTime(UINT64& time) = 0;
    CM_RT_API virtual INT WaitForTaskFinished(DWORD dwTimeOutMs = CM_MAX_TIMEOUT_MS) = 0;
};

class CmThreadSpace;
class CmThreadGroupSpace;

class CmKernel
{
public:
    CM_RT_API virtual INT SetThreadCount(UINT count) = 0;
    CM_RT_API virtual INT SetKernelArg(UINT index, size_t size, const void * pValue) = 0;

    CM_RT_API virtual INT SetThreadArg(UINT threadId, UINT index, size_t size, const void * pValue) = 0;
    CM_RT_API virtual INT SetStaticBuffer(UINT index, const void * pValue) = 0;
    CM_RT_API virtual INT SetSurfaceBTI(SurfaceIndex* pSurface, UINT BTIndex) = 0;
    CM_RT_API virtual INT AssociateThreadSpace(CmThreadSpace* & pTS) = 0;
};

class CmTask
{
public:
    CM_RT_API virtual INT AddKernel(CmKernel *pKernel) = 0;
    CM_RT_API virtual INT Reset(void) = 0;
    CM_RT_API virtual INT AddSync(void) = 0;
};

class CmProgram;
class SurfaceIndex;
class SamplerIndex;

class CmBuffer
{
public:
    CM_RT_API virtual INT GetIndex(SurfaceIndex*& pIndex) = 0;
    CM_RT_API virtual INT ReadSurface(unsigned char* pSysMem, CmEvent* pEvent, UINT64 sysMemSize = 0xFFFFFFFFFFFFFFFFULL) = 0;
    CM_RT_API virtual INT WriteSurface(const unsigned char* pSysMem, CmEvent* pEvent, UINT64 sysMemSize = 0xFFFFFFFFFFFFFFFFULL) = 0;
    CM_RT_API virtual INT InitSurface(const DWORD initValue, CmEvent* pEvent) = 0;
    CM_RT_API virtual INT SelectMemoryObjectControlSetting(MEMORY_OBJECT_CONTROL option) = 0;
};

class CmBufferUP
{
public:
    CM_RT_API virtual INT GetIndex(SurfaceIndex*& pIndex) = 0;
    CM_RT_API virtual INT SelectMemoryObjectControlSetting(MEMORY_OBJECT_CONTROL option) = 0;
};

class CmBufferSVM;

typedef void * CmAbstractSurfaceHandle;

class CmSurface2D
{
public:
    CM_RT_API virtual INT GetIndex(SurfaceIndex*& pIndex) = 0;
    CM_RT_API virtual INT ReadSurface(unsigned char* pSysMem, CmEvent* pEvent, UINT64 sysMemSize = 0xFFFFFFFFFFFFFFFFULL) = 0;
    CM_RT_API virtual INT WriteSurface(const unsigned char* pSysMem, CmEvent* pEvent, UINT64 sysMemSize = 0xFFFFFFFFFFFFFFFFULL) = 0;
    CM_RT_API virtual INT ReadSurfaceStride(unsigned char* pSysMem, CmEvent* pEvent, const UINT stride, UINT64 sysMemSize = 0xFFFFFFFFFFFFFFFFULL) = 0;
    CM_RT_API virtual INT WriteSurfaceStride(const unsigned char* pSysMem, CmEvent* pEvent, const UINT stride, UINT64 sysMemSize = 0xFFFFFFFFFFFFFFFFULL) = 0;
    CM_RT_API virtual INT InitSurface(const DWORD initValue, CmEvent* pEvent) = 0;
#ifdef CM_LINUX
    CM_RT_API virtual INT GetVaSurfaceID(VASurfaceID  &iVASurface) = 0;
#endif
};

class CmSurface2DUP
{
public:
    CM_RT_API virtual INT GetIndex(SurfaceIndex*& pIndex) = 0;
    CM_RT_API virtual INT SelectMemoryObjectControlSetting(MEMORY_OBJECT_CONTROL option) = 0;
};

class CmSurface3D
{
public:
    CM_RT_API virtual INT GetIndex(SurfaceIndex*& pIndex) = 0;
    CM_RT_API virtual INT ReadSurface(unsigned char* pSysMem, CmEvent* pEvent, UINT64 sysMemSize = 0xFFFFFFFFFFFFFFFFULL) = 0;
    CM_RT_API virtual INT WriteSurface(const unsigned char* pSysMem, CmEvent* pEvent, UINT64 sysMemSize = 0xFFFFFFFFFFFFFFFFULL) = 0;
    CM_RT_API virtual INT InitSurface(const DWORD initValue, CmEvent* pEvent) = 0;
    CM_RT_API virtual INT SelectMemoryObjectControlSetting(MEMORY_OBJECT_CONTROL option) = 0;
};

class CmSampler
{
public:
    CM_RT_API virtual INT GetIndex(SamplerIndex* & pIndex) = 0;
};

class CmThreadSpace
{
public:
    CM_RT_API virtual INT AssociateThread(UINT x, UINT y, CmKernel* pKernel, UINT threadId) = 0;
    CM_RT_API virtual INT SelectThreadDependencyPattern(CM_DEPENDENCY_PATTERN pattern) = 0;
};

class CmThreadGroupSpace;

class CmVebox;

class CmQueue
{
public:
    CM_RT_API virtual INT Enqueue(CmTask* pTask, CmEvent* & pEvent, const CmThreadSpace* pTS = NULL) = 0;
    CM_RT_API virtual INT DestroyEvent(CmEvent* & pEvent) = 0;
    CM_RT_API virtual INT EnqueueWithGroup(CmTask* pTask, CmEvent* & pEvent, const CmThreadGroupSpace* pTGS = NULL) = 0;
    CM_RT_API virtual INT EnqueueCopyCPUToGPU(CmSurface2D* pSurface, const unsigned char* pSysMem, CmEvent* & pEvent) = 0;
    CM_RT_API virtual INT EnqueueCopyGPUToCPU(CmSurface2D* pSurface, unsigned char* pSysMem, CmEvent* & pEvent) = 0;
    CM_RT_API virtual INT EnqueueInitSurface2D(CmSurface2D* pSurface, const DWORD initValue, CmEvent* &pEvent) = 0;
    CM_RT_API virtual INT EnqueueCopyGPUToGPU(CmSurface2D* pOutputSurface, CmSurface2D* pInputSurface, UINT option, CmEvent* & pEvent) = 0;
    CM_RT_API virtual INT EnqueueCopyCPUToCPU(unsigned char* pDstSysMem, unsigned char* pSrcSysMem, UINT size, UINT option, CmEvent* & pEvent) = 0;

    CM_RT_API virtual INT EnqueueCopyCPUToGPUFullStride(CmSurface2D* pSurface, const unsigned char* pSysMem, const UINT widthStride, const UINT heightStride, const UINT option, CmEvent* & pEvent) = 0;
    CM_RT_API virtual INT EnqueueCopyGPUToCPUFullStride(CmSurface2D* pSurface, unsigned char* pSysMem, const UINT widthStride, const UINT heightStride, const UINT option, CmEvent* & pEvent) = 0;

    CM_RT_API virtual INT EnqueueWithHints(CmTask* pTask, CmEvent* & pEvent, UINT hints = 0) = 0;
    CM_RT_API virtual INT EnqueueVebox(CmVebox* pVebox, CmEvent* & pEvent) = 0;
};

class CmDevice;

typedef void(*IMG_WALKER_FUNTYPE)(void* img, void* arg);

EXTERN_C CM_RT_API INT CMRT_GetSurfaceDetails(CmEvent* pEvent, UINT kernIndex, UINT surfBTI, CM_SURFACE_DETAILS& outDetails);
EXTERN_C CM_RT_API void CMRT_PrepareGTPinBuffers(void* ptr0, int size0InBytes, void* ptr1, int size1InBytes, void* ptr2, int size2InBytes);
EXTERN_C CM_RT_API void CMRT_SetGTPinArguments(char* commandLine, void* gtpinInvokeStruct);
EXTERN_C CM_RT_API void CMRT_EnableGTPinMarkers(void);

EXTERN_C CM_RT_API INT DestroyCmDevice(CmDevice* &pD);



EXTERN_C CM_RT_API UINT CMRT_GetKernelCount(CmEvent *pEvent);
EXTERN_C CM_RT_API INT CMRT_GetKernelName(CmEvent *pEvent, UINT index, char** KernelName);
EXTERN_C CM_RT_API INT CMRT_GetKernelThreadSpace(CmEvent *pEvent, UINT index, UINT* localWidth, UINT* localHeight, UINT* globalWidth, UINT* globalHeight);
EXTERN_C CM_RT_API INT CMRT_GetSubmitTime(CmEvent *pEvent, LARGE_INTEGER* time);
EXTERN_C CM_RT_API INT CMRT_GetHWStartTime(CmEvent *pEvent, LARGE_INTEGER* time);
EXTERN_C CM_RT_API INT CMRT_GetHWEndTime(CmEvent *pEvent, LARGE_INTEGER* time);
EXTERN_C CM_RT_API INT CMRT_GetCompleteTime(CmEvent *pEvent, LARGE_INTEGER* time);
EXTERN_C CM_RT_API INT CMRT_SetEventCallback(CmEvent* pEvent, callback_function function, void* user_data);
EXTERN_C CM_RT_API INT CMRT_Enqueue(CmQueue* pQueue, CmTask* pTask, CmEvent** pEvent, const CmThreadSpace* pTS = NULL);

EXTERN_C CM_RT_API INT CMRT_GetEnqueueTime(CmEvent *pEvent, LARGE_INTEGER* time);

EXTERN_C CM_RT_API const char* GetCmErrorString(int Code);

#if defined(CM_LINUX)
namespace CmLinux
{
    class CmDevice
    {
    public:

        CM_RT_API virtual INT CreateBuffer(UINT size, CmBuffer* & pSurface) = 0;
        CM_RT_API virtual INT CreateSurface2D(UINT width, UINT height, CM_SURFACE_FORMAT format, CmSurface2D* & pSurface) = 0;
        CM_RT_API virtual INT CreateSurface3D(UINT width, UINT height, UINT depth, CM_SURFACE_FORMAT format, CmSurface3D* & pSurface) = 0;

        CM_RT_API virtual INT CreateSurface2D(VASurfaceID iVASurface, CmSurface2D* & pSurface) = 0;
        CM_RT_API virtual INT CreateSurface2D(VASurfaceID* iVASurface, const UINT surfaceCount, CmSurface2D** pSurface) = 0;

        CM_RT_API virtual INT DestroySurface(CmBuffer* & pSurface) = 0;
        CM_RT_API virtual INT DestroySurface(CmSurface2D* & pSurface) = 0;
        CM_RT_API virtual INT DestroySurface(CmSurface3D* & pSurface) = 0;

        CM_RT_API virtual INT CreateQueue(CmQueue* & pQueue) = 0;
        CM_RT_API virtual INT LoadProgram(void* pCommonISACode, const UINT size, CmProgram*& pProgram, const char* options = NULL) = 0;
        CM_RT_API virtual INT CreateKernel(CmProgram* pProgram, const char* kernelName, CmKernel* & pKernel, const char* options = NULL) = 0;
        CM_RT_API virtual INT CreateKernel(CmProgram* pProgram, const char* kernelName, const void * fncPnt, CmKernel* & pKernel, const char* options = NULL) = 0;
        CM_RT_API virtual INT CreateSampler(const CM_SAMPLER_STATE & sampleState, CmSampler* & pSampler) = 0;

        CM_RT_API virtual INT DestroyKernel(CmKernel*& pKernel) = 0;
        CM_RT_API virtual INT DestroySampler(CmSampler*& pSampler) = 0;
        CM_RT_API virtual INT DestroyProgram(CmProgram*& pProgram) = 0;
        CM_RT_API virtual INT DestroyThreadSpace(CmThreadSpace* & pTS) = 0;

        CM_RT_API virtual INT CreateTask(CmTask *& pTask) = 0;
        CM_RT_API virtual INT DestroyTask(CmTask*& pTask) = 0;

        CM_RT_API virtual INT GetCaps(CM_DEVICE_CAP_NAME capName, size_t& capValueSize, void* pCapValue) = 0;

        CM_RT_API virtual INT CreateThreadSpace(UINT width, UINT height, CmThreadSpace* & pTS) = 0;

        CM_RT_API virtual INT CreateBufferUP(UINT size, void* pSystMem, CmBufferUP* & pSurface) = 0;
        CM_RT_API virtual INT DestroyBufferUP(CmBufferUP* & pSurface) = 0;

        CM_RT_API virtual INT GetSurface2DInfo(UINT width, UINT height, CM_SURFACE_FORMAT format, UINT & pitch, UINT & physicalSize) = 0;
        CM_RT_API virtual INT CreateSurface2DUP(UINT width, UINT height, CM_SURFACE_FORMAT format, void* pSysMem, CmSurface2DUP* & pSurface) = 0;
        CM_RT_API virtual INT DestroySurface2DUP(CmSurface2DUP* & pSurface) = 0;

        CM_RT_API virtual INT CreateVmeSurfaceG7_5(CmSurface2D* pCurSurface, CmSurface2D** pForwardSurface, CmSurface2D** pBackwardSurface, const UINT surfaceCountForward, const UINT surfaceCountBackward, SurfaceIndex* & pVmeIndex) = 0;
        CM_RT_API virtual INT DestroyVmeSurfaceG7_5(SurfaceIndex* & pVmeIndex) = 0;
        CM_RT_API virtual INT CreateSampler8x8(const CM_SAMPLER_8X8_DESCR  & smplDescr, CmSampler8x8*& psmplrState) = 0;
        CM_RT_API virtual INT DestroySampler8x8(CmSampler8x8*& pSampler) = 0;
        CM_RT_API virtual INT CreateSampler8x8Surface(CmSurface2D* p2DSurface, SurfaceIndex* & pDIIndex, CM_SAMPLER8x8_SURFACE surf_type = CM_VA_SURFACE, CM_SURFACE_ADDRESS_CONTROL_MODE = CM_SURFACE_CLAMP) = 0;
        CM_RT_API virtual INT DestroySampler8x8Surface(SurfaceIndex* & pDIIndex) = 0;

        CM_RT_API virtual INT CreateThreadGroupSpace(UINT thrdSpaceWidth, UINT thrdSpaceHeight, UINT grpSpaceWidth, UINT grpSpaceHeight, CmThreadGroupSpace*& pTGS) = 0;
        CM_RT_API virtual INT DestroyThreadGroupSpace(CmThreadGroupSpace*& pTGS) = 0;
        CM_RT_API virtual INT SetL3Config(const L3ConfigRegisterValues *l3_c) = 0;
        CM_RT_API virtual INT SetSuggestedL3Config(L3_SUGGEST_CONFIG l3_s_c) = 0;

        CM_RT_API virtual INT SetCaps(CM_DEVICE_CAP_NAME capName, size_t capValueSize, void* pCapValue) = 0;

        CM_RT_API virtual INT CreateSamplerSurface2D(CmSurface2D* p2DSurface, SurfaceIndex* & pSamplerSurfaceIndex) = 0;
        CM_RT_API virtual INT CreateSamplerSurface3D(CmSurface3D* p3DSurface, SurfaceIndex* & pSamplerSurfaceIndex) = 0;
        CM_RT_API virtual INT DestroySamplerSurface(SurfaceIndex* & pSamplerSurfaceIndex) = 0;

        CM_RT_API virtual INT InitPrintBuffer(size_t printbufsize = 1048576) = 0;
        CM_RT_API virtual INT FlushPrintBuffer() = 0;

        CM_RT_API virtual INT CreateVebox(CmVebox* & pVebox) = 0; //HSW
        CM_RT_API virtual INT DestroyVebox(CmVebox* & pVebox) = 0; //HSW

        CM_RT_API virtual INT GetVaDpy(VADisplay* & pva_dpy) = 0;
        CM_RT_API virtual INT CreateVaSurface2D(UINT width, UINT height, CM_SURFACE_FORMAT format, VASurfaceID & iVASurface, CmSurface2D* & pSurface) = 0;

        CM_RT_API virtual INT CreateBufferSVM(UINT size, void* & pSystMem, uint32_t access_flag, CmBufferSVM* & pSurface) = 0;
        CM_RT_API virtual INT DestroyBufferSVM(CmBufferSVM* & pSurface) = 0;
        CM_RT_API virtual INT CreateSamplerSurface2DUP(CmSurface2DUP* p2DUPSurface, SurfaceIndex* & pSamplerSurfaceIndex) = 0;

        CM_RT_API virtual INT CloneKernel(CmKernel * &pKernelDest, CmKernel * pKernelSrc) = 0;

        CM_RT_API virtual INT CreateSurface2DAlias(CmSurface2D* p2DSurface, SurfaceIndex* &aliasSurfaceIndex) = 0;

        CM_RT_API virtual INT CreateHevcVmeSurfaceG10(CmSurface2D* pCurSurface, CmSurface2D** pForwardSurface, CmSurface2D** pBackwardSurface, const UINT surfaceCountForward, const UINT surfaceCountBackward, SurfaceIndex* & pVmeIndex) = 0;
        CM_RT_API virtual INT DestroyHevcVmeSurfaceG10(SurfaceIndex* & pVmeIndex) = 0;
        CM_RT_API virtual INT CreateSamplerEx(const CM_SAMPLER_STATE_EX & sampleState, CmSampler* & pSampler) = 0;
        CM_RT_API virtual INT FlushPrintBufferIntoFile(const char *filename) = 0;
        CM_RT_API virtual INT CreateThreadGroupSpaceEx(UINT thrdSpaceWidth, UINT thrdSpaceHeight, UINT thrdSpaceDepth, UINT grpSpaceWidth, UINT grpSpaceHeight, UINT grpSpaceDepth, CmThreadGroupSpace*& pTGS) = 0;

        CM_RT_API virtual INT CreateSampler8x8SurfaceEx(CmSurface2D* p2DSurface, SurfaceIndex* & pDIIndex, CM_SAMPLER8x8_SURFACE surf_type = CM_VA_SURFACE, CM_SURFACE_ADDRESS_CONTROL_MODE = CM_SURFACE_CLAMP, CM_FLAG* pFlag = NULL) = 0;
        CM_RT_API virtual INT CreateSamplerSurface2DEx(CmSurface2D* p2DSurface, SurfaceIndex* & pSamplerSurfaceIndex, CM_FLAG* pFlag = NULL) = 0;
        CM_RT_API virtual INT CreateBufferAlias(CmBuffer *pBuffer, SurfaceIndex* &pAliasIndex) = 0;

        CM_RT_API virtual INT SetVmeSurfaceStateParam(SurfaceIndex* pVmeIndex, CM_VME_SURFACE_STATE_PARAM *pSSParam) = 0;

        CM_RT_API virtual int32_t GetVISAVersion(uint32_t& majorVersion, uint32_t& minorVersion) = 0;
        //adding new functions in the bottom is a must
    };
}
#endif

typedef void * AbstractSurfaceHandle;
typedef void * AbstractDeviceHandle;
typedef L3ConfigRegisterValues L3_CONFIG_REGISTER_VALUES;

class CmDevice
{
public:
    CM_RT_API virtual INT GetDevice(AbstractDeviceHandle & pDevice) = 0;
    CM_RT_API virtual INT CreateBuffer(UINT size, CmBuffer* & pSurface) = 0;
    CM_RT_API virtual INT CreateSurface2D(UINT width, UINT height, CM_SURFACE_FORMAT format, CmSurface2D* & pSurface) = 0;
    CM_RT_API virtual INT CreateSurface3D(UINT width, UINT height, UINT depth, CM_SURFACE_FORMAT format, CmSurface3D* & pSurface) = 0;
    CM_RT_API virtual INT CreateSurface2D(mfxHDLPair D3DSurfPair, CmSurface2D* & pSurface) = 0;
    CM_RT_API virtual INT CreateSurface2D(AbstractSurfaceHandle pD3DSurf, CmSurface2D* & pSurface) = 0;
    //CM_RT_API virtual INT CreateSurface2D( AbstractSurfaceHandle * pD3DSurf, const UINT surfaceCount, CmSurface2D**  pSpurface ) = 0;
    CM_RT_API virtual INT DestroySurface(CmBuffer* & pSurface) = 0;
    CM_RT_API virtual INT DestroySurface(CmSurface2D* & pSurface) = 0;
    CM_RT_API virtual INT DestroySurface(CmSurface3D* & pSurface) = 0;
    CM_RT_API virtual INT CreateQueue(CmQueue* & pQueue) = 0;
    CM_RT_API virtual INT LoadProgram(void* pCommonISACode, const UINT size, CmProgram*& pProgram, const char* options = NULL) = 0;
    CM_RT_API virtual INT CreateKernel(CmProgram* pProgram, const char* kernelName, CmKernel* & pKernel, const char* options = NULL) = 0;
    CM_RT_API virtual INT CreateKernel(CmProgram* pProgram, const char* kernelName, const void * fncPnt, CmKernel* & pKernel, const char* options = NULL) = 0;
    CM_RT_API virtual INT CreateSampler(const CM_SAMPLER_STATE & sampleState, CmSampler* & pSampler) = 0;
    CM_RT_API virtual INT DestroyKernel(CmKernel*& pKernel) = 0;
    CM_RT_API virtual INT DestroySampler(CmSampler*& pSampler) = 0;
    CM_RT_API virtual INT DestroyProgram(CmProgram*& pProgram) = 0;
    CM_RT_API virtual INT DestroyThreadSpace(CmThreadSpace* & pTS) = 0;
    CM_RT_API virtual INT CreateTask(CmTask *& pTask) = 0;
    CM_RT_API virtual INT DestroyTask(CmTask*& pTask) = 0;
    CM_RT_API virtual INT GetCaps(CM_DEVICE_CAP_NAME capName, size_t& capValueSize, void* pCapValue) = 0;
    CM_RT_API virtual INT CreateThreadSpace(UINT width, UINT height, CmThreadSpace* & pTS) = 0;
    CM_RT_API virtual INT CreateBufferUP(UINT size, void* pSystMem, CmBufferUP* & pSurface) = 0;
    CM_RT_API virtual INT DestroyBufferUP(CmBufferUP* & pSurface) = 0;
    CM_RT_API virtual INT GetSurface2DInfo(UINT width, UINT height, CM_SURFACE_FORMAT format, UINT & pitch, UINT & physicalSize) = 0;
    CM_RT_API virtual INT CreateSurface2DUP(UINT width, UINT height, CM_SURFACE_FORMAT format, void* pSysMem, CmSurface2DUP* & pSurface) = 0;
    CM_RT_API virtual INT DestroySurface2DUP(CmSurface2DUP* & pSurface) = 0;
    CM_RT_API virtual INT CreateVmeSurfaceG7_5(CmSurface2D* pCurSurface, CmSurface2D** pForwardSurface, CmSurface2D** pBackwardSurface, const UINT surfaceCountForward, const UINT surfaceCountBackward, SurfaceIndex* & pVmeIndex) = 0;
    CM_RT_API virtual INT DestroyVmeSurfaceG7_5(SurfaceIndex* & pVmeIndex) = 0;
    CM_RT_API virtual INT CreateSampler8x8(const CM_SAMPLER_8X8_DESCR  & smplDescr, CmSampler8x8*& psmplrState) = 0;
    CM_RT_API virtual INT DestroySampler8x8(CmSampler8x8*& pSampler) = 0;
    CM_RT_API virtual INT CreateSampler8x8Surface(CmSurface2D* p2DSurface, SurfaceIndex* & pDIIndex, CM_SAMPLER8x8_SURFACE surf_type = CM_VA_SURFACE, CM_SURFACE_ADDRESS_CONTROL_MODE = CM_SURFACE_CLAMP) = 0;
    CM_RT_API virtual INT DestroySampler8x8Surface(SurfaceIndex* & pDIIndex) = 0;
    CM_RT_API virtual INT CreateThreadGroupSpace(UINT thrdSpaceWidth, UINT thrdSpaceHeight, UINT grpSpaceWidth, UINT grpSpaceHeight, CmThreadGroupSpace*& pTGS) = 0;
    CM_RT_API virtual INT DestroyThreadGroupSpace(CmThreadGroupSpace*& pTGS) = 0;
    CM_RT_API virtual INT SetL3Config(const L3_CONFIG_REGISTER_VALUES *l3_c) = 0;
    CM_RT_API virtual INT SetSuggestedL3Config(L3_SUGGEST_CONFIG l3_s_c) = 0;
    CM_RT_API virtual INT SetCaps(CM_DEVICE_CAP_NAME capName, size_t capValueSize, void* pCapValue) = 0;
    CM_RT_API virtual INT CreateSamplerSurface2D(CmSurface2D* p2DSurface, SurfaceIndex* & pSamplerSurfaceIndex) = 0;
    CM_RT_API virtual INT CreateSamplerSurface3D(CmSurface3D* p3DSurface, SurfaceIndex* & pSamplerSurfaceIndex) = 0;
    CM_RT_API virtual INT DestroySamplerSurface(SurfaceIndex* & pSamplerSurfaceIndex) = 0;
    CM_RT_API virtual INT InitPrintBuffer(size_t printbufsize = 1048576) = 0;
    CM_RT_API virtual INT FlushPrintBuffer() = 0;
    CM_RT_API virtual INT CreateSurface2DSubresource(AbstractSurfaceHandle pD3D11Texture2D, UINT subresourceCount, CmSurface2D** ppSurfaces, UINT& createdSurfaceCount, UINT option = 0) = 0;
    CM_RT_API virtual INT CreateSurface2DbySubresourceIndex(AbstractSurfaceHandle pD3D11Texture2D, UINT FirstArraySlice, UINT FirstMipSlice, CmSurface2D* &pSurface) = 0;
};

INT CreateCmDevice(CmDevice* &pD, UINT& version, VADisplay va_dpy = NULL, UINT mode = CM_DEVICE_CREATE_OPTION_DEFAULT);
INT CreateCmDeviceEmu(CmDevice* &pDevice, UINT& version, VADisplay va_dpy = NULL);
INT CreateCmDeviceSim(CmDevice* &pDevice, UINT& version);

INT DestroyCmDevice(CmDevice* &pDevice);
INT DestroyCmDeviceEmu(CmDevice* &pDevice);
INT DestroyCmDeviceSim(CmDevice* &pDevice);

int ReadProgram(CmDevice * device, CmProgram *& program, const unsigned char * buffer, unsigned int len);
int ReadProgramJit(CmDevice * device, CmProgram *& program, const unsigned char * buffer, unsigned int len);
int CreateKernel(CmDevice * device, CmProgram * program, const char * kernelName, const void * fncPnt, CmKernel *& kernel, const char * options = NULL);

#ifdef _MSVC_LANG
#pragma warning(pop)
#endif

#undef LONG
#undef ULONG

#else //if CMAPIUPDATE

#define CM_1_0 100
#define CM_2_0 200
#define CM_2_1 201
#define CM_2_2 202
#define CM_2_3 203
#define CM_2_4 204
#define CM_3_0 300
#define CM_4_0 400


#define CM_MAX_1D_SURF_WIDTH 0X8000000 // 2^27

#define CM_DEVICE_CREATE_OPTION_DEFAULT                     0
#define CM_DEVICE_CREATE_OPTION_SCRATCH_SPACE_DISABLE       1
#define CM_DEVICE_CONFIG_MIDTHREADPREEMPTION_OFFSET         22
#define CM_DEVICE_CONFIG_MIDTHREADPREEMPTION_DISENABLE      (1 << CM_DEVICE_CONFIG_MIDTHREADPREEMPTION_OFFSET)

extern class CmEvent *CM_NO_EVENT;


// CM RT DLL File Version
typedef struct _CM_DLL_FILE_VERSION
{
    WORD    wMANVERSION;
    WORD    wMANREVISION;
    WORD    wSUBREVISION;
    WORD    wBUILD_NUMBER;
    //Version constructed as : "wMANVERSION.wMANREVISION.wSUBREVISION.wBUILD_NUMBER"
} CM_DLL_FILE_VERSION, *PCM_DLL_FILE_VERSION;

#define NUM_SEARCH_PATH_STATES_G6       14
#define NUM_MBMODE_SETS_G6              4

typedef struct _VME_SEARCH_PATH_LUT_STATE_G6
{
    // DWORD 0
    union
    {
        struct
        {
            DWORD   SearchPathLocation_X_0 : 4;
            DWORD   SearchPathLocation_Y_0 : 4;
            DWORD   SearchPathLocation_X_1 : 4;
            DWORD   SearchPathLocation_Y_1 : 4;
            DWORD   SearchPathLocation_X_2 : 4;
            DWORD   SearchPathLocation_Y_2 : 4;
            DWORD   SearchPathLocation_X_3 : 4;
            DWORD   SearchPathLocation_Y_3 : 4;
        };
        struct
        {
            DWORD   Value;
        };
    };
} VME_SEARCH_PATH_LUT_STATE_G6, *PVME_SEARCH_PATH_LUT_STATE_G6;

typedef struct _VME_RD_LUT_SET_G6
{
    // DWORD 0
    union
    {
        struct
        {
            DWORD   LUT_MbMode_0 : 8;
            DWORD   LUT_MbMode_1 : 8;
            DWORD   LUT_MbMode_2 : 8;
            DWORD   LUT_MbMode_3 : 8;
        };
        struct
        {
            DWORD   Value;
        };
    } DW0;

    // DWORD 1
    union
    {
        struct
        {
            DWORD   LUT_MbMode_4 : 8;
            DWORD   LUT_MbMode_5 : 8;
            DWORD   LUT_MbMode_6 : 8;
            DWORD   LUT_MbMode_7 : 8;
        };
        struct
        {
            DWORD   Value;
        };
    } DW1;

    // DWORD 2
    union
    {
        struct
        {
            DWORD   LUT_MV_0 : 8;
            DWORD   LUT_MV_1 : 8;
            DWORD   LUT_MV_2 : 8;
            DWORD   LUT_MV_3 : 8;
        };
        struct
        {
            DWORD   Value;
        };
    } DW2;

    // DWORD 3
    union
    {
        struct
        {
            DWORD   LUT_MV_4 : 8;
            DWORD   LUT_MV_5 : 8;
            DWORD   LUT_MV_6 : 8;
            DWORD   LUT_MV_7 : 8;
        };
        struct
        {
            DWORD   Value;
        };
    } DW3;
} VME_RD_LUT_SET_G6, *PVME_RD_LUT_SET_G6;


typedef struct _VME_STATE_G6
{
    // DWORD 0 - DWORD 13
    VME_SEARCH_PATH_LUT_STATE_G6    SearchPath[NUM_SEARCH_PATH_STATES_G6];

    // DWORD 14
    union
    {
        struct
        {
            DWORD   LUT_MbMode_8_0 : 8;
            DWORD   LUT_MbMode_9_0 : 8;
            DWORD   LUT_MbMode_8_1 : 8;
            DWORD   LUT_MbMode_9_1 : 8;
        };
        struct
        {
            DWORD   Value;
        };
    } DW14;

    // DWORD 15
    union
    {
        struct
        {
            DWORD   LUT_MbMode_8_2 : 8;
            DWORD   LUT_MbMode_9_2 : 8;
            DWORD   LUT_MbMode_8_3 : 8;
            DWORD   LUT_MbMode_9_3 : 8;
        };
        struct
        {
            DWORD   Value;
        };
    } DW15;

    // DWORD 16 - DWORD 31
    VME_RD_LUT_SET_G6   RdLutSet[NUM_MBMODE_SETS_G6];
} VME_STATE_G6, *PVME_STATE_G6;

#define CM_MAX_DEPENDENCY_COUNT         8

typedef enum _CM_BOUNDARY_PIXEL_MODE
{
    CM_BOUNDARY_NORMAL = 0x0,
    CM_BOUNDARY_PROGRESSIVE_FRAME = 0x2,
    CM_BOUNARY_INTERLACED_FRAME = 0x3
}CM_BOUNDARY_PIXEL_MODE;

/**************** L3/Cache ***************/
typedef enum _MEMORY_OBJECT_CONTROL {
    // SNB
    MEMORY_OBJECT_CONTROL_USE_GTT_ENTRY,
    MEMORY_OBJECT_CONTROL_NEITHER_LLC_NOR_MLC,
    MEMORY_OBJECT_CONTROL_LLC_NOT_MLC,
    MEMORY_OBJECT_CONTROL_LLC_AND_MLC,

    // IVB
    MEMORY_OBJECT_CONTROL_FROM_GTT_ENTRY = MEMORY_OBJECT_CONTROL_USE_GTT_ENTRY,                                 // Caching dependent on pte
    MEMORY_OBJECT_CONTROL_L3,                                             // Cached in L3$
    MEMORY_OBJECT_CONTROL_LLC,                                            // Cached in LLC
    MEMORY_OBJECT_CONTROL_LLC_L3,                                         // Cached in LLC & L3$

                                                                          // HSW
#ifdef CMRT_SIM
                                                                          MEMORY_OBJECT_CONTROL_USE_PTE, // Caching dependent on pte
#else
                                                                          MEMORY_OBJECT_CONTROL_USE_PTE = MEMORY_OBJECT_CONTROL_FROM_GTT_ENTRY, // Caching dependent on pte
#endif
                                                                          MEMORY_OBJECT_CONTROL_UC,                                             // Uncached
                                                                          MEMORY_OBJECT_CONTROL_LLC_ELLC,
                                                                          MEMORY_OBJECT_CONTROL_ELLC,
                                                                          MEMORY_OBJECT_CONTROL_L3_USE_PTE,
                                                                          MEMORY_OBJECT_CONTROL_L3_UC,
                                                                          MEMORY_OBJECT_CONTROL_L3_LLC_ELLC,
                                                                          MEMORY_OBJECT_CONTROL_L3_ELLC,

                                                                          MEMORY_OBJECT_CONTROL_UNKNOWN = 0xff
} MEMORY_OBJECT_CONTROL;

typedef enum _MEMORY_TYPE {
    CM_USE_PTE,
    CM_UN_CACHEABLE,
    CM_WRITE_THROUGH,
    CM_WRITE_BACK
} MEMORY_TYPE;

#define BDW_L3_CONFIG_NUM 8
#define HSW_L3_CONFIG_NUM 12
#define IVB_2_L3_CONFIG_NUM 12
#define IVB_1_L3_CONFIG_NUM 12

typedef struct _L3_CONFIG_REGISTER_VALUES {
    UINT SQCREG1_VALUE;
    UINT CNTLREG2_VALUE;
    UINT CNTLREG3_VALUE;
} L3_CONFIG_REGISTER_VALUES;

typedef enum _L3_SUGGEST_CONFIG
{
    IVB_L3_PLANE_DEFAULT,
    IVB_L3_PLANE_1,
    IVB_L3_PLANE_2,
    IVB_L3_PLANE_3,
    IVB_L3_PLANE_4,
    IVB_L3_PLANE_5,
    IVB_L3_PLANE_6,
    IVB_L3_PLANE_7,
    IVB_L3_PLANE_8,
    IVB_L3_PLANE_9,
    IVB_L3_PLANE_10,
    IVB_L3_PLANE_11,

    HSW_L3_PLANE_DEFAULT = IVB_L3_PLANE_DEFAULT,
    HSW_L3_PLANE_1,
    HSW_L3_PLANE_2,
    HSW_L3_PLANE_3,
    HSW_L3_PLANE_4,
    HSW_L3_PLANE_5,
    HSW_L3_PLANE_6,
    HSW_L3_PLANE_7,
    HSW_L3_PLANE_8,
    HSW_L3_PLANE_9,
    HSW_L3_PLANE_10,
    HSW_L3_PLANE_11,

    IVB_SLM_PLANE_DEFAULT = IVB_L3_PLANE_9,
    HSW_SLM_PLANE_DEFAULT = HSW_L3_PLANE_9
} L3_SUGGEST_CONFIG;


/*
*  AVS SAMPLER8x8 STATE
*/

#define CM_NUM_COEFF_ROWS 17
typedef struct _CM_AVS_NONPIPLINED_STATE {
    bool BypassXAF;
    bool BypassYAF;
    BYTE DefaultSharpLvl;
    BYTE maxDerivative4Pixels;
    BYTE maxDerivative8Pixels;
    BYTE transitionArea4Pixels;
    BYTE transitionArea8Pixels;
    CM_AVS_COEFF_TABLE Tbl0X[CM_NUM_COEFF_ROWS];
    CM_AVS_COEFF_TABLE Tbl0Y[CM_NUM_COEFF_ROWS];
    CM_AVS_COEFF_TABLE Tbl1X[CM_NUM_COEFF_ROWS];
    CM_AVS_COEFF_TABLE Tbl1Y[CM_NUM_COEFF_ROWS];
}CM_AVS_NONPIPLINED_STATE;

typedef struct _CM_AVS_INTERNEL_NONPIPLINED_STATE {
    bool BypassXAF;
    bool BypassYAF;
    BYTE DefaultSharpLvl;
    BYTE maxDerivative4Pixels;
    BYTE maxDerivative8Pixels;
    BYTE transitionArea4Pixels;
    BYTE transitionArea8Pixels;
    CM_AVS_INTERNEL_COEFF_TABLE Tbl0X[CM_NUM_COEFF_ROWS];
    CM_AVS_INTERNEL_COEFF_TABLE Tbl0Y[CM_NUM_COEFF_ROWS];
    CM_AVS_INTERNEL_COEFF_TABLE Tbl1X[CM_NUM_COEFF_ROWS];
    CM_AVS_INTERNEL_COEFF_TABLE Tbl1Y[CM_NUM_COEFF_ROWS];
}CM_AVS_INTERNEL_NONPIPLINED_STATE;

typedef struct _CM_AVS_STATE_MSG {
    bool AVSTYPE; //true nearest, false adaptive
    bool EightTapAFEnable; //HSW+
    bool BypassIEF; //ignored for BWL, moved to sampler8x8 payload.
    bool ShuffleOutputWriteback; //SKL mode only to be set when AVS msg sequence is 4x4 or 8x4
    unsigned short GainFactor;
    unsigned char GlobalNoiseEstm;
    unsigned char StrongEdgeThr;
    unsigned char WeakEdgeThr;
    unsigned char StrongEdgeWght;
    unsigned char RegularWght;
    unsigned char NonEdgeWght;
    //For Non-piplined states
    unsigned short stateID;
    CM_AVS_NONPIPLINED_STATE * AvsState;
} CM_AVS_STATE_MSG;

/*
*  CONVOLVE STATE DATA STRUCTURES
*/

#define CM_NUM_CONVOLVE_ROWS 16
#define CM_NUM_CONVOLVE_ROWS_SKL 32
typedef struct _CM_CONVOLVE_STATE_MSG {
    bool CoeffSize; //true 16-bit, false 8-bit
    byte SclDwnValue; //Scale down value
    byte Width; //Kernel Width
    byte Height; //Kernel Height
                 //SKL mode
    bool isVertical32Mode;
    bool isHorizontal32Mode;
    CM_CONVOLVE_COEFF_TABLE Table[CM_NUM_CONVOLVE_ROWS_SKL];
} CM_CONVOLVE_STATE_MSG;


typedef enum _CM_SAMPLER_STATE_TYPE_
{
    CM_SAMPLER8X8_AVS = 0,
    CM_SAMPLER8X8_CONV = 1,
    CM_SAMPLER8X8_MISC = 3,
    CM_SAMPLER8X8_NONE
}CM_SAMPLER_STATE_TYPE;

typedef struct _CM_SAMPLER_8X8_DESCR {
    CM_SAMPLER_STATE_TYPE stateType;
    union
    {
        CM_AVS_STATE_MSG * avs;
        CM_CONVOLVE_STATE_MSG * conv;
        CM_MISC_STATE_MSG * misc; //ERODE/DILATE/MINMAX
    };
} CM_SAMPLER_8X8_DESCR;

//!
//! CM Sampler8X8
//!
class CmSampler8x8
{
public:
    CM_RT_API virtual INT GetIndex(SamplerIndex* & pIndex) = 0;

};
/***********END SAMPLER8X8********************/
class CmEvent
{
public:
    CM_RT_API virtual INT GetStatus(CM_STATUS& status) = 0;
    CM_RT_API virtual INT GetExecutionTime(UINT64& time) = 0;
    CM_RT_API virtual INT WaitForTaskFinished(DWORD dwTimeOutMs = CM_MAX_TIMEOUT_MS) = 0;
};

class CmThreadSpace;

class CmKernel
{
public:
    CM_RT_API virtual INT SetThreadCount(UINT count) = 0;
    CM_RT_API virtual INT SetKernelArg(UINT index, size_t size, const void * pValue) = 0;

    CM_RT_API virtual INT SetThreadArg(UINT threadId, UINT index, size_t size, const void * pValue) = 0;
    CM_RT_API virtual INT SetStaticBuffer(UINT index, const void * pValue) = 0;

    CM_RT_API virtual INT SetKernelPayloadData(size_t size, const void *pValue) = 0;
    CM_RT_API virtual INT SetKernelPayloadSurface(UINT surfaceCount, SurfaceIndex** pSurfaces) = 0;
    CM_RT_API virtual INT SetSurfaceBTI(SurfaceIndex* pSurface, UINT BTIndex) = 0;

    CM_RT_API virtual INT AssociateThreadSpace(CmThreadSpace* & pTS) = 0;
};

class CmTask
{
public:
    CM_RT_API virtual INT AddKernel(CmKernel *pKernel) = 0;
    CM_RT_API virtual INT Reset(void) = 0;
    CM_RT_API virtual INT AddSync(void) = 0;
};

class CmProgram;
class SurfaceIndex;
class SamplerIndex;

class CmBuffer
{
public:
    CM_RT_API virtual INT GetIndex(SurfaceIndex*& pIndex) = 0;
    CM_RT_API virtual INT ReadSurface(unsigned char* pSysMem, CmEvent* pEvent, UINT64 sysMemSize = 0xFFFFFFFFFFFFFFFFULL) = 0;
    CM_RT_API virtual INT WriteSurface(const unsigned char* pSysMem, CmEvent* pEvent, UINT64 sysMemSize = 0xFFFFFFFFFFFFFFFFULL) = 0;
    CM_RT_API virtual INT InitSurface(const DWORD initValue, CmEvent* pEvent) = 0;
    CM_RT_API virtual INT SetMemoryObjectControl(MEMORY_OBJECT_CONTROL mem_ctrl, MEMORY_TYPE mem_type, UINT  age) = 0;
};

class CmBufferUP
{
public:
    CM_RT_API virtual INT GetIndex(SurfaceIndex*& pIndex) = 0;
    CM_RT_API virtual INT SetMemoryObjectControl(MEMORY_OBJECT_CONTROL mem_ctrl, MEMORY_TYPE mem_type, UINT  age) = 0;
};

class CmSurface2D
{
public:
    CM_RT_API virtual INT GetIndex(SurfaceIndex*& pIndex) = 0;
    CM_RT_API virtual INT ReadSurface(unsigned char* pSysMem, CmEvent* pEvent, UINT64 sysMemSize = 0xFFFFFFFFFFFFFFFFULL) = 0;
    CM_RT_API virtual INT WriteSurface(const unsigned char* pSysMem, CmEvent* pEvent, UINT64 sysMemSize = 0xFFFFFFFFFFFFFFFFULL) = 0;
    CM_RT_API virtual INT ReadSurfaceStride(unsigned char* pSysMem, CmEvent* pEvent, const UINT stride, UINT64 sysMemSize = 0xFFFFFFFFFFFFFFFFULL) = 0;
    CM_RT_API virtual INT WriteSurfaceStride(const unsigned char* pSysMem, CmEvent* pEvent, const UINT stride, UINT64 sysMemSize = 0xFFFFFFFFFFFFFFFFULL) = 0;
    CM_RT_API virtual INT InitSurface(const DWORD initValue, CmEvent* pEvent) = 0;
    CM_RT_API virtual INT SetMemoryObjectControl(MEMORY_OBJECT_CONTROL mem_ctrl, MEMORY_TYPE mem_type, UINT  age) = 0;
    CM_RT_API virtual INT SetSurfaceState(UINT iWidth, UINT iHeight, CM_SURFACE_FORMAT Format, CM_BOUNDARY_PIXEL_MODE boundaryMode) = 0;
#ifdef CM_LINUX
    CM_RT_API virtual INT GetVaSurfaceID(VASurfaceID  &iVASurface) = 0;
#endif
};

class CmSurface2DUP
{
public:
    CM_RT_API virtual INT GetIndex(SurfaceIndex*& pIndex) = 0;
    CM_RT_API virtual INT SetMemoryObjectControl(MEMORY_OBJECT_CONTROL mem_ctrl, MEMORY_TYPE mem_type, UINT  age) = 0;
};

class CmSurface3D
{
public:
    CM_RT_API virtual INT GetIndex(SurfaceIndex*& pIndex) = 0;
    CM_RT_API virtual INT ReadSurface(unsigned char* pSysMem, CmEvent* pEvent, UINT64 sysMemSize = 0xFFFFFFFFFFFFFFFFULL) = 0;
    CM_RT_API virtual INT WriteSurface(const unsigned char* pSysMem, CmEvent* pEvent, UINT64 sysMemSize = 0xFFFFFFFFFFFFFFFFULL) = 0;
    CM_RT_API virtual INT InitSurface(const DWORD initValue, CmEvent* pEvent) = 0;
    CM_RT_API virtual INT SetMemoryObjectControl(MEMORY_OBJECT_CONTROL mem_ctrl, MEMORY_TYPE mem_type, UINT  age) = 0;
};

class CmSampler
{
public:
    CM_RT_API virtual INT GetIndex(SamplerIndex* & pIndex) = 0;
};

class CmThreadSpace
{
public:
    CM_RT_API virtual INT AssociateThread(UINT x, UINT y, CmKernel* pKernel, UINT threadId) = 0;
    CM_RT_API virtual INT SelectThreadDependencyPattern(CM_DEPENDENCY_PATTERN pattern) = 0;
};

class CmThreadGroupSpace;
class CmVebox_G75;

class CmQueue
{
public:
    CM_RT_API virtual INT Enqueue(CmTask* pTask, CmEvent* & pEvent, const CmThreadSpace* pTS = NULL) = 0;
    CM_RT_API virtual INT DestroyEvent(CmEvent* & pEvent) = 0;
    CM_RT_API virtual INT EnqueueWithGroup(CmTask* pTask, CmEvent* & pEvent, const CmThreadGroupSpace* pTGS = NULL) = 0;
    CM_RT_API virtual INT EnqueueCopyCPUToGPU(CmSurface2D* pSurface, const unsigned char* pSysMem, CmEvent* & pEvent) = 0;
    CM_RT_API virtual INT EnqueueCopyGPUToCPU(CmSurface2D* pSurface, unsigned char* pSysMem, CmEvent* & pEvent) = 0;
    CM_RT_API virtual INT EnqueueCopyCPUToGPUStride(CmSurface2D* pSurface, const unsigned char* pSysMem, const UINT stride, CmEvent* & pEvent) = 0;
    CM_RT_API virtual INT EnqueueCopyGPUToCPUStride(CmSurface2D* pSurface, unsigned char* pSysMem, const UINT stride, CmEvent* & pEvent) = 0;
    CM_RT_API virtual INT EnqueueInitSurface2D(CmSurface2D* pSurface, const DWORD initValue, CmEvent* &pEvent) = 0;
    CM_RT_API virtual INT EnqueueCopyGPUToGPU(CmSurface2D* pOutputSurface, CmSurface2D* pInputSurface, CmEvent* & pEvent) = 0;
    CM_RT_API virtual INT EnqueueCopyCPUToCPU(unsigned char* pDstSysMem, unsigned char* pSrcSysMem, UINT size, CmEvent* & pEvent) = 0;

    CM_RT_API virtual INT EnqueueCopyCPUToGPUFullStride(CmSurface2D* pSurface, const unsigned char* pSysMem, const UINT widthStride, const UINT heightStride, const UINT option, CmEvent* & pEvent) = 0;
    CM_RT_API virtual INT EnqueueCopyGPUToCPUFullStride(CmSurface2D* pSurface, unsigned char* pSysMem, const UINT widthStride, const UINT heightStride, const UINT option, CmEvent* & pEvent) = 0;
};

class VmeIndex
{
public:
    CM_NOINLINE VmeIndex() { index = 0; };
    CM_NOINLINE VmeIndex(VmeIndex& _src) { index = _src.get_data(); };
    CM_NOINLINE VmeIndex(const unsigned int& _n) { index = _n; };
    CM_NOINLINE VmeIndex & operator = (const unsigned int& _n) { this->index = _n; return *this; };
    virtual unsigned int get_data(void) { return index; };
private:
    unsigned int index;
#ifdef CM_LINUX
    unsigned char extra_byte;
#endif
};

class CmVmeState
{
public:
    CM_RT_API virtual INT GetIndex(VmeIndex* & pIndex) = 0;
};

namespace CmLinux
{
    class CmDevice
    {
    public:
        CM_RT_API virtual INT CreateBuffer(UINT size, CmBuffer* & pSurface) = 0;
        CM_RT_API virtual INT CreateSurface2D(UINT width, UINT height, CM_SURFACE_FORMAT format, CmSurface2D* & pSurface) = 0;
        CM_RT_API virtual INT CreateSurface3D(UINT width, UINT height, UINT depth, CM_SURFACE_FORMAT format, CmSurface3D* & pSurface) = 0;
        CM_RT_API virtual INT CreateSurface2D(VASurfaceID iVASurface, CmSurface2D *& pSurface) = 0;
        CM_RT_API virtual INT CreateSurface2D(VASurfaceID * iVASurface, const UINT surfaceCount, CmSurface2D ** pSurface) = 0;
        CM_RT_API virtual INT DestroySurface(CmBuffer* & pSurface) = 0;
        CM_RT_API virtual INT DestroySurface(CmSurface2D* & pSurface) = 0;
        CM_RT_API virtual INT DestroySurface(CmSurface3D* & pSurface) = 0;
        CM_RT_API virtual INT CreateQueue(CmQueue* & pQueue) = 0;
        CM_RT_API virtual INT LoadProgram(void* pCommonISACode, const UINT size, CmProgram*& pProgram, const char* options = NULL) = 0;
        CM_RT_API virtual INT CreateKernel(CmProgram* pProgram, const char* kernelName, CmKernel* & pKernel, const char* options = NULL) = 0;
        CM_RT_API virtual INT CreateKernel(CmProgram* pProgram, const char* kernelName, const void * fncPnt, CmKernel* & pKernel, const char* options = NULL) = 0;
        CM_RT_API virtual INT CreateSampler(const CM_SAMPLER_STATE & sampleState, CmSampler* & pSampler) = 0;
        CM_RT_API virtual INT DestroyKernel(CmKernel*& pKernel) = 0;
        CM_RT_API virtual INT DestroySampler(CmSampler*& pSampler) = 0;
        CM_RT_API virtual INT DestroyProgram(CmProgram*& pProgram) = 0;
        CM_RT_API virtual INT DestroyThreadSpace(CmThreadSpace* & pTS) = 0;
        CM_RT_API virtual INT CreateTask(CmTask *& pTask) = 0;
        CM_RT_API virtual INT DestroyTask(CmTask*& pTask) = 0;
        CM_RT_API virtual INT GetCaps(CM_DEVICE_CAP_NAME capName, size_t& capValueSize, void* pCapValue) = 0;
        CM_RT_API virtual INT CreateVmeStateG6(const VME_STATE_G6 & vmeState, CmVmeState* & pVmeState) = 0;
        CM_RT_API virtual INT DestroyVmeStateG6(CmVmeState*& pVmeState) = 0;
        CM_RT_API virtual INT CreateVmeSurfaceG6(CmSurface2D* pCurSurface, CmSurface2D* pForwardSurface, CmSurface2D* pBackwardSurface, SurfaceIndex* & pVmeIndex) = 0;
        CM_RT_API virtual INT DestroyVmeSurfaceG6(SurfaceIndex* & pVmeIndex) = 0;
        CM_RT_API virtual INT CreateThreadSpace(UINT width, UINT height, CmThreadSpace* & pTS) = 0;
        CM_RT_API virtual INT CreateBufferUP(UINT size, void* pSystMem, CmBufferUP* & pSurface) = 0;
        CM_RT_API virtual INT DestroyBufferUP(CmBufferUP* & pSurface) = 0;
        CM_RT_API virtual INT GetSurface2DInfo(UINT width, UINT height, CM_SURFACE_FORMAT format, UINT & pitch, UINT & physicalSize) = 0;
        CM_RT_API virtual INT CreateSurface2DUP(UINT width, UINT height, CM_SURFACE_FORMAT format, void* pSysMem, CmSurface2DUP* & pSurface) = 0;
        CM_RT_API virtual INT DestroySurface2DUP(CmSurface2DUP* & pSurface) = 0;
        CM_RT_API virtual INT CreateVmeSurfaceG7_5(CmSurface2D* pCurSurface, CmSurface2D** pForwardSurface, CmSurface2D** pBackwardSurface, const UINT surfaceCountForward, const UINT surfaceCountBackward, SurfaceIndex* & pVmeIndex) = 0;
        CM_RT_API virtual INT DestroyVmeSurfaceG7_5(SurfaceIndex* & pVmeIndex) = 0;
        CM_RT_API virtual INT CreateSampler8x8(const CM_SAMPLER_8X8_DESCR  & smplDescr, CmSampler8x8*& psmplrState) = 0;
        CM_RT_API virtual INT DestroySampler8x8(CmSampler8x8*& pSampler) = 0;
        CM_RT_API virtual INT CreateSampler8x8Surface(CmSurface2D* p2DSurface, SurfaceIndex* & pDIIndex, CM_SAMPLER8x8_SURFACE surf_type = CM_VA_SURFACE, CM_SURFACE_ADDRESS_CONTROL_MODE = CM_SURFACE_CLAMP) = 0;
        CM_RT_API virtual INT DestroySampler8x8Surface(SurfaceIndex* & pDIIndex) = 0;
        CM_RT_API virtual INT CreateThreadGroupSpace(UINT thrdSpaceWidth, UINT thrdSpaceHeight, UINT grpSpaceWidth, UINT grpSpaceHeight, CmThreadGroupSpace*& pTGS) = 0;
        CM_RT_API virtual INT DestroyThreadGroupSpace(CmThreadGroupSpace*& pTGS) = 0;
        CM_RT_API virtual INT SetL3Config(L3_CONFIG_REGISTER_VALUES *l3_c) = 0;
        CM_RT_API virtual INT SetSuggestedL3Config(L3_SUGGEST_CONFIG l3_s_c) = 0;
        CM_RT_API virtual INT SetCaps(CM_DEVICE_CAP_NAME capName, size_t capValueSize, void* pCapValue) = 0;
        CM_RT_API virtual INT CreateGroupedVAPlusSurface(CmSurface2D* p2DSurface1, CmSurface2D* p2DSurface2, SurfaceIndex* & pDIIndex, CM_SURFACE_ADDRESS_CONTROL_MODE = CM_SURFACE_CLAMP) = 0;
        CM_RT_API virtual INT DestroyGroupedVAPlusSurface(SurfaceIndex* & pDIIndex) = 0;
        CM_RT_API virtual INT CreateSamplerSurface2D(CmSurface2D* p2DSurface, SurfaceIndex* & pSamplerSurfaceIndex) = 0;
        CM_RT_API virtual INT CreateSamplerSurface3D(CmSurface3D* p3DSurface, SurfaceIndex* & pSamplerSurfaceIndex) = 0;
        CM_RT_API virtual INT DestroySamplerSurface(SurfaceIndex* & pSamplerSurfaceIndex) = 0;
        CM_RT_API virtual INT GetRTDllVersion(CM_DLL_FILE_VERSION* pFileVersion) = 0;
        CM_RT_API virtual INT GetJITDllVersion(CM_DLL_FILE_VERSION* pFileVersion) = 0;
        CM_RT_API virtual INT InitPrintBuffer(size_t printbufsize = 1048576) = 0;
        CM_RT_API virtual INT FlushPrintBuffer() = 0;
    };
};

class CmDevice
{
public:
    CM_RT_API virtual INT GetDevice(AbstractDeviceHandle & pDevice) = 0;
    CM_RT_API virtual INT CreateBuffer(UINT size, CmBuffer* & pSurface) = 0;
    CM_RT_API virtual INT CreateSurface2D(UINT width, UINT height, CM_SURFACE_FORMAT format, CmSurface2D* & pSurface) = 0;
    CM_RT_API virtual INT CreateSurface3D(UINT width, UINT height, UINT depth, CM_SURFACE_FORMAT format, CmSurface3D* & pSurface) = 0;
    CM_RT_API virtual INT CreateSurface2D(mfxHDLPair D3DSurfPair, CmSurface2D* & pSurface) = 0;
    CM_RT_API virtual INT CreateSurface2D(AbstractSurfaceHandle pD3DSurf, CmSurface2D* & pSurface) = 0;
    //CM_RT_API virtual INT CreateSurface2D( AbstractSurfaceHandle * pD3DSurf, const UINT surfaceCount, CmSurface2D**  pSpurface ) = 0;
    CM_RT_API virtual INT DestroySurface(CmBuffer* & pSurface) = 0;
    CM_RT_API virtual INT DestroySurface(CmSurface2D* & pSurface) = 0;
    CM_RT_API virtual INT DestroySurface(CmSurface3D* & pSurface) = 0;
    CM_RT_API virtual INT CreateQueue(CmQueue* & pQueue) = 0;
    CM_RT_API virtual INT LoadProgram(void* pCommonISACode, const UINT size, CmProgram*& pProgram, const char* options = NULL) = 0;
    CM_RT_API virtual INT CreateKernel(CmProgram* pProgram, const char* kernelName, CmKernel* & pKernel, const char* options = NULL) = 0;
    CM_RT_API virtual INT CreateKernel(CmProgram* pProgram, const char* kernelName, const void * fncPnt, CmKernel* & pKernel, const char* options = NULL) = 0;
    CM_RT_API virtual INT CreateSampler(const CM_SAMPLER_STATE & sampleState, CmSampler* & pSampler) = 0;
    CM_RT_API virtual INT DestroyKernel(CmKernel*& pKernel) = 0;
    CM_RT_API virtual INT DestroySampler(CmSampler*& pSampler) = 0;
    CM_RT_API virtual INT DestroyProgram(CmProgram*& pProgram) = 0;
    CM_RT_API virtual INT DestroyThreadSpace(CmThreadSpace* & pTS) = 0;
    CM_RT_API virtual INT CreateTask(CmTask *& pTask) = 0;
    CM_RT_API virtual INT DestroyTask(CmTask*& pTask) = 0;
    CM_RT_API virtual INT GetCaps(CM_DEVICE_CAP_NAME capName, size_t& capValueSize, void* pCapValue) = 0;
    CM_RT_API virtual INT CreateVmeStateG6(const VME_STATE_G6 & vmeState, CmVmeState* & pVmeState) = 0;
    CM_RT_API virtual INT DestroyVmeStateG6(CmVmeState*& pVmeState) = 0;
    CM_RT_API virtual INT CreateVmeSurfaceG6(CmSurface2D* pCurSurface, CmSurface2D* pForwardSurface, CmSurface2D* pBackwardSurface, SurfaceIndex* & pVmeIndex) = 0;
    CM_RT_API virtual INT DestroyVmeSurfaceG6(SurfaceIndex* & pVmeIndex) = 0;
    CM_RT_API virtual INT CreateThreadSpace(UINT width, UINT height, CmThreadSpace* & pTS) = 0;
    CM_RT_API virtual INT CreateBufferUP(UINT size, void* pSystMem, CmBufferUP* & pSurface) = 0;
    CM_RT_API virtual INT DestroyBufferUP(CmBufferUP* & pSurface) = 0;
    CM_RT_API virtual INT GetSurface2DInfo(UINT width, UINT height, CM_SURFACE_FORMAT format, UINT & pitch, UINT & physicalSize) = 0;
    CM_RT_API virtual INT CreateSurface2DUP(UINT width, UINT height, CM_SURFACE_FORMAT format, void* pSysMem, CmSurface2DUP* & pSurface) = 0;
    CM_RT_API virtual INT DestroySurface2DUP(CmSurface2DUP* & pSurface) = 0;
    CM_RT_API virtual INT CreateVmeSurfaceG7_5(CmSurface2D* pCurSurface, CmSurface2D** pForwardSurface, CmSurface2D** pBackwardSurface, const UINT surfaceCountForward, const UINT surfaceCountBackward, SurfaceIndex* & pVmeIndex) = 0;
    CM_RT_API virtual INT DestroyVmeSurfaceG7_5(SurfaceIndex* & pVmeIndex) = 0;
    CM_RT_API virtual INT CreateSampler8x8(const CM_SAMPLER_8X8_DESCR  & smplDescr, CmSampler8x8*& psmplrState) = 0;
    CM_RT_API virtual INT DestroySampler8x8(CmSampler8x8*& pSampler) = 0;
    CM_RT_API virtual INT CreateSampler8x8Surface(CmSurface2D* p2DSurface, SurfaceIndex* & pDIIndex, CM_SAMPLER8x8_SURFACE surf_type = CM_VA_SURFACE, CM_SURFACE_ADDRESS_CONTROL_MODE = CM_SURFACE_CLAMP) = 0;
    CM_RT_API virtual INT DestroySampler8x8Surface(SurfaceIndex* & pDIIndex) = 0;
    CM_RT_API virtual INT CreateThreadGroupSpace(UINT thrdSpaceWidth, UINT thrdSpaceHeight, UINT grpSpaceWidth, UINT grpSpaceHeight, CmThreadGroupSpace*& pTGS) = 0;
    CM_RT_API virtual INT DestroyThreadGroupSpace(CmThreadGroupSpace*& pTGS) = 0;
    CM_RT_API virtual INT SetL3Config(L3_CONFIG_REGISTER_VALUES *l3_c) = 0;
    CM_RT_API virtual INT SetSuggestedL3Config(L3_SUGGEST_CONFIG l3_s_c) = 0;
    CM_RT_API virtual INT SetCaps(CM_DEVICE_CAP_NAME capName, size_t capValueSize, void* pCapValue) = 0;
    CM_RT_API virtual INT CreateGroupedVAPlusSurface(CmSurface2D* p2DSurface1, CmSurface2D* p2DSurface2, SurfaceIndex* & pDIIndex, CM_SURFACE_ADDRESS_CONTROL_MODE = CM_SURFACE_CLAMP) = 0;
    CM_RT_API virtual INT DestroyGroupedVAPlusSurface(SurfaceIndex* & pDIIndex) = 0;
    CM_RT_API virtual INT CreateSamplerSurface2D(CmSurface2D* p2DSurface, SurfaceIndex* & pSamplerSurfaceIndex) = 0;
    CM_RT_API virtual INT CreateSamplerSurface3D(CmSurface3D* p3DSurface, SurfaceIndex* & pSamplerSurfaceIndex) = 0;
    CM_RT_API virtual INT DestroySamplerSurface(SurfaceIndex* & pSamplerSurfaceIndex) = 0;
    CM_RT_API virtual INT GetRTDllVersion(CM_DLL_FILE_VERSION* pFileVersion) = 0;
    CM_RT_API virtual INT GetJITDllVersion(CM_DLL_FILE_VERSION* pFileVersion) = 0;
    CM_RT_API virtual INT InitPrintBuffer(size_t printbufsize = 1048576) = 0;
    CM_RT_API virtual INT FlushPrintBuffer() = 0;
    CM_RT_API virtual INT CreateSurface2DSubresource(AbstractSurfaceHandle pD3D11Texture2D, UINT subresourceCount, CmSurface2D** ppSurfaces, UINT& createdSurfaceCount, UINT option = 0) = 0;
    CM_RT_API virtual INT CreateSurface2DbySubresourceIndex(AbstractSurfaceHandle pD3D11Texture2D, UINT FirstArraySlice, UINT FirstMipSlice, CmSurface2D* &pSurface) = 0;
};


typedef void(*IMG_WALKER_FUNTYPE)(void* img, void* arg);

EXTERN_C CM_RT_API void  CMRT_SetHwDebugStatus(bool bInStatus);
EXTERN_C CM_RT_API bool CMRT_GetHwDebugStatus();
EXTERN_C CM_RT_API INT CMRT_GetSurfaceDetails(CmEvent* pEvent, UINT kernIndex, UINT surfBTI, CM_SURFACE_DETAILS& outDetails);
EXTERN_C CM_RT_API void CMRT_PrepareGTPinBuffers(void* ptr0, int size0InBytes, void* ptr1, int size1InBytes, void* ptr2, int size2InBytes);
EXTERN_C CM_RT_API void CMRT_SetGTPinArguments(char* commandLine, void* gtpinInvokeStruct);
EXTERN_C CM_RT_API void CMRT_EnableGTPinMarkers(void);

#define CM_CALLBACK __cdecl
typedef void (CM_CALLBACK *callback_function)(CmEvent*, void *);

EXTERN_C CM_RT_API UINT CMRT_GetKernelCount(CmEvent *pEvent);
EXTERN_C CM_RT_API INT CMRT_GetKernelName(CmEvent *pEvent, UINT index, char** KernelName);
EXTERN_C CM_RT_API INT CMRT_GetKernelThreadSpace(CmEvent *pEvent, UINT index, UINT* localWidth, UINT* localHeight, UINT* globalWidth, UINT* globalHeight);
EXTERN_C CM_RT_API INT CMRT_GetSubmitTime(CmEvent *pEvent, LARGE_INTEGER* time);
EXTERN_C CM_RT_API INT CMRT_GetHWStartTime(CmEvent *pEvent, LARGE_INTEGER* time);
EXTERN_C CM_RT_API INT CMRT_GetHWEndTime(CmEvent *pEvent, LARGE_INTEGER* time);
EXTERN_C CM_RT_API INT CMRT_GetCompleteTime(CmEvent *pEvent, LARGE_INTEGER* time);
EXTERN_C CM_RT_API INT CMRT_WaitForEventCallback(CmEvent *pEvent);
EXTERN_C CM_RT_API INT CMRT_SetEventCallback(CmEvent* pEvent, callback_function function, void* user_data);
EXTERN_C CM_RT_API INT CMRT_Enqueue(CmQueue* pQueue, CmTask* pTask, CmEvent** pEvent, const CmThreadSpace* pTS = NULL);

INT CreateCmDevice(CmDevice* &pD, UINT& version, VADisplay va_dpy = NULL, UINT mode = CM_DEVICE_CREATE_OPTION_DEFAULT);
INT CreateCmDeviceEmu(CmDevice* &pDevice, UINT& version, VADisplay va_dpy = NULL);
INT CreateCmDeviceSim(CmDevice* &pDevice, UINT& version);

INT DestroyCmDevice(CmDevice* &pDevice);
INT DestroyCmDeviceEmu(CmDevice* &pDevice);
INT DestroyCmDeviceSim(CmDevice* &pDevice);

int ReadProgram(CmDevice * device, CmProgram *& program, const unsigned char * buffer, unsigned int len);
int ReadProgramJit(CmDevice * device, CmProgram *& program, const unsigned char * buffer, unsigned int len);
int CreateKernel(CmDevice * device, CmProgram * program, const char * kernelName, const void * fncPnt, CmKernel *& kernel, const char * options = NULL);

#ifdef _MSVC_LANG
#pragma warning(pop)
#endif

#undef LONG
#undef ULONG

#endif

#if defined(__clang__)
  #pragma clang diagnostic pop
#elif defined(__GNUC__)
  #pragma GCC diagnostic pop
#endif

#endif // __CMRT_CROSS_PLATFORM_H__
