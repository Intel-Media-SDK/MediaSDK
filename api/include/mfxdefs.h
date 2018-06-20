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
#ifndef __MFXDEFS_H__
#define __MFXDEFS_H__

#define MFX_VERSION_MAJOR 1
#define MFX_VERSION_MINOR 27

// MFX_VERSION_NEXT is always +1 from last public release
// may be enforced by MFX_VERSION_USE_LATEST define
// if MFX_VERSION_USE_LATEST is defined  MFX_VERSION is ignored

#define MFX_VERSION_NEXT (MFX_VERSION_MAJOR * 1000 + MFX_VERSION_MINOR + 1)

// MFX_VERSION - version of API that 'assumed' by build may be provided externally
// if it omitted then latest stable API derived from Major.Minor is assumed


#if !defined(MFX_VERSION)
  #if defined(MFX_VERSION_USE_LATEST)
    #define MFX_VERSION MFX_VERSION_NEXT
  #else
    #define MFX_VERSION (MFX_VERSION_MAJOR * 1000 + MFX_VERSION_MINOR)
  #endif
#else
  #undef MFX_VERSION_MINOR
  #define MFX_VERSION_MINOR ((MFX_VERSION) % 1000)
#endif


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


  #define __INT64   long long
  #define __UINT64  unsigned long long

    #define MFX_CDECL
    #define MFX_STDCALL

#define MFX_INFINITE 0xFFFFFFFF

typedef unsigned char       mfxU8;
typedef char                mfxI8;
typedef short               mfxI16;
typedef unsigned short      mfxU16;
typedef unsigned int        mfxU32;
typedef int                 mfxI32;
typedef unsigned int        mfxUL32;
typedef int                 mfxL32;
typedef float               mfxF32;
typedef double              mfxF64;
typedef __UINT64            mfxU64;
typedef __INT64             mfxI64;
typedef void*               mfxHDL;
typedef mfxHDL              mfxMemId;
typedef void*               mfxThreadTask;
typedef char                mfxChar;

typedef struct {
    mfxI16  x;
    mfxI16  y;
} mfxI16Pair;

typedef struct {
    mfxHDL first;
    mfxHDL second;
} mfxHDLPair;


/*********************************************************************************\
Error message
\*********************************************************************************/
typedef enum
{
    /* no error */
    MFX_ERR_NONE                        = 0,    /* no error */

    /* reserved for unexpected errors */
    MFX_ERR_UNKNOWN                     = -1,   /* unknown error. */

    /* error codes <0 */
    MFX_ERR_NULL_PTR                    = -2,   /* null pointer */
    MFX_ERR_UNSUPPORTED                 = -3,   /* undeveloped feature */
    MFX_ERR_MEMORY_ALLOC                = -4,   /* failed to allocate memory */
    MFX_ERR_NOT_ENOUGH_BUFFER           = -5,   /* insufficient buffer at input/output */
    MFX_ERR_INVALID_HANDLE              = -6,   /* invalid handle */
    MFX_ERR_LOCK_MEMORY                 = -7,   /* failed to lock the memory block */
    MFX_ERR_NOT_INITIALIZED             = -8,   /* member function called before initialization */
    MFX_ERR_NOT_FOUND                   = -9,   /* the specified object is not found */
    MFX_ERR_MORE_DATA                   = -10,  /* expect more data at input */
    MFX_ERR_MORE_SURFACE                = -11,  /* expect more surface at output */
    MFX_ERR_ABORTED                     = -12,  /* operation aborted */
    MFX_ERR_DEVICE_LOST                 = -13,  /* lose the HW acceleration device */
    MFX_ERR_INCOMPATIBLE_VIDEO_PARAM    = -14,  /* incompatible video parameters */
    MFX_ERR_INVALID_VIDEO_PARAM         = -15,  /* invalid video parameters */
    MFX_ERR_UNDEFINED_BEHAVIOR          = -16,  /* undefined behavior */
    MFX_ERR_DEVICE_FAILED               = -17,  /* device operation failure */
    MFX_ERR_MORE_BITSTREAM              = -18,  /* expect more bitstream buffers at output */
    MFX_ERR_INCOMPATIBLE_AUDIO_PARAM    = -19,  /* incompatible audio parameters */
    MFX_ERR_INVALID_AUDIO_PARAM         = -20,  /* invalid audio parameters */
    MFX_ERR_GPU_HANG                    = -21,  /* device operation failure caused by GPU hang */
    MFX_ERR_REALLOC_SURFACE             = -22,  /* bigger output surface required */

    /* warnings >0 */
    MFX_WRN_IN_EXECUTION                = 1,    /* the previous asynchronous operation is in execution */
    MFX_WRN_DEVICE_BUSY                 = 2,    /* the HW acceleration device is busy */
    MFX_WRN_VIDEO_PARAM_CHANGED         = 3,    /* the video parameters are changed during decoding */
    MFX_WRN_PARTIAL_ACCELERATION        = 4,    /* SW is used */
    MFX_WRN_INCOMPATIBLE_VIDEO_PARAM    = 5,    /* incompatible video parameters */
    MFX_WRN_VALUE_NOT_CHANGED           = 6,    /* the value is saturated based on its valid range */
    MFX_WRN_OUT_OF_RANGE                = 7,    /* the value is out of valid range */
    MFX_WRN_FILTER_SKIPPED              = 10,   /* one of requested filters has been skipped */
    MFX_WRN_INCOMPATIBLE_AUDIO_PARAM    = 11,   /* incompatible audio parameters */

    /* threading statuses */
    MFX_TASK_DONE = MFX_ERR_NONE,               /* task has been completed */
    MFX_TASK_WORKING                    = 8,    /* there is some more work to do */
    MFX_TASK_BUSY                       = 9,    /* task is waiting for resources */

    /* plug-in statuses */
    MFX_ERR_MORE_DATA_SUBMIT_TASK       = -10000, /* return MFX_ERR_MORE_DATA but submit internal asynchronous task */

} mfxStatus;


// Application 
#if defined(MFX_DISPATCHER_EXPOSED_PREFIX)

#include "mfxdispatcherprefixedfunctions.h"

#endif // MFX_DISPATCHER_EXPOSED_PREFIX


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MFXDEFS_H__ */
