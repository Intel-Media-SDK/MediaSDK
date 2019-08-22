// Copyright (c) 2018-2019 Intel Corporation
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

#include "mfxvideo.h"

#if MFX_VERSION >= MFX_VERSION_NEXT

#include "util_defs.h"

std::string ConvertMfxStatusToString(mfxStatus sts)
{
    switch (sts)
    {

    case MFX_ERR_NONE:                     /* no error */
        return std::string("MFX_ERR_NONE");

        /* error codes <0 */
    case MFX_ERR_NULL_PTR:                 /* null pointer */
        return std::string("MFX_ERR_NULL_PTR");
    case MFX_ERR_UNSUPPORTED:              /* undeveloped feature */
        return std::string("MFX_ERR_UNSUPPORTED");
    case MFX_ERR_MEMORY_ALLOC:             /* failed to allocate memory */
        return std::string("MFX_ERR_MEMORY_ALLOC");
    case MFX_ERR_NOT_ENOUGH_BUFFER:        /* insufficient buffer at input/output */
        return std::string("MFX_ERR_NOT_ENOUGH_BUFFER");
    case MFX_ERR_INVALID_HANDLE:           /* invalid handle */
        return std::string("MFX_ERR_INVALID_HANDLE");
    case MFX_ERR_LOCK_MEMORY:              /* failed to lock the memory block */
        return std::string("MFX_ERR_LOCK_MEMORY");
    case MFX_ERR_NOT_INITIALIZED:          /* member function called before initialization */
        return std::string("MFX_ERR_NOT_INITIALIZED");
    case MFX_ERR_NOT_FOUND:                /* the specified object is not found */
        return std::string("MFX_ERR_NOT_FOUND");
    case MFX_ERR_MORE_DATA:                /* expect more data at input */
        return std::string("MFX_ERR_MORE_DATA");
    case MFX_ERR_MORE_SURFACE:             /* expect more surface at output */
        return std::string("MFX_ERR_MORE_SURFACE");
    case MFX_ERR_ABORTED:                  /* operation aborted */
        return std::string("MFX_ERR_ABORTED");
    case MFX_ERR_DEVICE_LOST:              /* lose the HW acceleration device */
        return std::string("MFX_ERR_DEVICE_LOST");
    case MFX_ERR_INCOMPATIBLE_VIDEO_PARAM: /* incompatible video parameters */
        return std::string("MFX_ERR_INCOMPATIBLE_VIDEO_PARAM");
    case MFX_ERR_INVALID_VIDEO_PARAM:      /* invalid video parameters */
        return std::string("MFX_ERR_INVALID_VIDEO_PARAM");
    case MFX_ERR_UNDEFINED_BEHAVIOR:       /* undefined behavior */
        return std::string("MFX_ERR_UNDEFINED_BEHAVIOR");
    case MFX_ERR_DEVICE_FAILED:            /* device operation failure */
        return std::string("MFX_ERR_DEVICE_FAILED");
    case MFX_ERR_MORE_BITSTREAM:           /* expect more bitstream buffers at output */
        return std::string("MFX_ERR_MORE_BITSTREAM");
    case MFX_ERR_INCOMPATIBLE_AUDIO_PARAM: /* incompatible audio parameters */
        return std::string("MFX_ERR_INCOMPATIBLE_AUDIO_PARAM");
    case MFX_ERR_INVALID_AUDIO_PARAM:      /* invalid audio parameters */
        return std::string("MFX_ERR_INVALID_AUDIO_PARAM");
    case MFX_ERR_GPU_HANG:                 /* device operation failure caused by GPU hang */
        return std::string("MFX_ERR_GPU_HANG");
    case MFX_ERR_REALLOC_SURFACE:          /* bigger output surface required */
        return std::string("MFX_ERR_REALLOC_SURFACE");

        /* warnings >0 */
    case MFX_WRN_IN_EXECUTION:             /* the previous asynchronous operation is in execution */
        return std::string("MFX_WRN_IN_EXECUTION");
    case MFX_WRN_DEVICE_BUSY:              /* the HW acceleration device is busy */
        return std::string("MFX_WRN_DEVICE_BUSY");
    case MFX_WRN_VIDEO_PARAM_CHANGED:      /* the video parameters are changed during decoding */
        return std::string("MFX_WRN_VIDEO_PARAM_CHANGED");
    case MFX_WRN_PARTIAL_ACCELERATION:     /* SW is used */
        return std::string("MFX_WRN_PARTIAL_ACCELERATION");
    case MFX_WRN_INCOMPATIBLE_VIDEO_PARAM: /* incompatible video parameters */
        return std::string("MFX_WRN_INCOMPATIBLE_VIDEO_PARAM");
    case MFX_WRN_VALUE_NOT_CHANGED:        /* the value is saturated based on its valid range */
        return std::string("MFX_WRN_VALUE_NOT_CHANGED");
    case MFX_WRN_OUT_OF_RANGE:             /* the value is out of valid range */
        return std::string("MFX_WRN_OUT_OF_RANGE");
    case MFX_WRN_FILTER_SKIPPED:           /* one of requested filters has been skipped */
        return std::string("MFX_WRN_FILTER_SKIPPED");
    case MFX_WRN_INCOMPATIBLE_AUDIO_PARAM: /* incompatible audio parameters */
        return std::string("MFX_WRN_INCOMPATIBLE_AUDIO_PARAM");

        /* threading statuses */
        //case MFX_TASK_DONE: == ERR_NONE      /* task has been completed */
    case MFX_TASK_WORKING:                 /* there is some more work to do */
        return std::string("MFX_TASK_WORKING");
    case MFX_TASK_BUSY:                    /* task is waiting for resources */
        return std::string("MFX_TASK_BUSY");

        /* plug-in statuses */
    case MFX_ERR_MORE_DATA_SUBMIT_TASK:    /* return MFX_ERR_MORE_DATA but submit internal asynchronous task */
        return std::string("MFX_ERR_MORE_DATA_SUBMIT_TASK");

    case MFX_ERR_UNKNOWN:                  /* unknown error. */
    default:
        return std::string("MFX_ERR_UNKNOWN");
    }
}

#endif // MFX_VERSION
