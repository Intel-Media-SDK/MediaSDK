/******************************************************************************\
Copyright (c) 2018, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This sample was distributed or derived from the Intel's Media Samples package.
The original version of this sample may be obtained from https://software.intel.com/en-us/intel-media-server-studio
or https://software.intel.com/en-us/media-client-solutions-support.
\**********************************************************************************/

#ifndef __FEI_BUFFER_ALLOCATOR_H__
#define __FEI_BUFFER_ALLOCATOR_H__

#include "mfxfeihevc.h"
#include "sample_hevc_fei_defs.h"

#include <va/va.h>
#include "vaapi_utils.h"

struct BufferAllocRequest {
    mfxU32 Width;  // coded frame width
    mfxU32 Height; // coded frame height
};

const mfxU32 CTU_SIZE32 = 32;

// class for buffers allocation via direct access to vaapi buffers
class FeiBufferAllocator
{
public:
    FeiBufferAllocator(VADisplay dpy, mfxU32 width, mfxU32 height);
    ~FeiBufferAllocator();

    // T is external buffer structure
    template<typename T>
    void Alloc(T * buffer, const BufferAllocRequest& request)
    {
        if (!buffer) throw mfxError(MFX_ERR_NULL_PTR);
        if (buffer->Data != NULL) throw mfxError(MFX_ERR_UNDEFINED_BEHAVIOR, "Buffer is already allocated and using in app");

        if (!request.Width || !request.Height) throw mfxError(MFX_ERR_NOT_INITIALIZED, "BufferAllocRequest is uninitialized for buffer");

        mfxU32 va_num_elem = 1;
        mfxU32 va_data_size = sizeof(buffer->Data[0]);

        CalcBufferPitchHeight(*buffer, request, va_data_size, va_num_elem);

        VABufferType type = GetVABufferType(buffer->Header.BufferId);

        VAStatus sts = m_libva.vaCreateBuffer(m_dpy, m_context, type, va_data_size, va_num_elem, NULL, &buffer->VaBufferID);
        if (sts != VA_STATUS_SUCCESS) {
            InvalidateBuffer(buffer);
            throw mfxError(MFX_ERR_MEMORY_ALLOC, "vaCreateBuffer failed");
        }
    };

    template<typename T>
    void Lock(T * buffer)
    {
        if (!buffer) throw mfxError(MFX_ERR_NULL_PTR);

        if (VA_INVALID_ID == buffer->VaBufferID)
          throw mfxError(MFX_ERR_UNDEFINED_BEHAVIOR, "Invalid VaBufferID. Lock with unallocated buffer");

        VAStatus sts = m_libva.vaMapBuffer(m_dpy, buffer->VaBufferID, (void**)&buffer->Data);
        if (sts != VA_STATUS_SUCCESS) {
            throw mfxError(MFX_ERR_DEVICE_FAILED, "vaMapBuffer failed");
        }
        if (!buffer->Data) throw mfxError(MFX_ERR_NULL_PTR, "No pointer to data");
    }

    template<typename T>
    void Unlock(T * buffer)
    {
        if (!buffer) throw mfxError(MFX_ERR_NULL_PTR);

        if (VA_INVALID_ID == buffer->VaBufferID)
          throw mfxError(MFX_ERR_UNDEFINED_BEHAVIOR, "Invalid VaBufferID. Unlock with unallocated buffer");

        VAStatus sts = m_libva.vaUnmapBuffer(m_dpy, buffer->VaBufferID);
        if (sts != VA_STATUS_SUCCESS) {
            throw mfxError(MFX_ERR_DEVICE_FAILED, "vaUnmapBuffer failed");
        }
        buffer->Data = NULL;
    };

    template<typename T>
    void Free(T * buffer)
    {
        if (!buffer) throw mfxError(MFX_ERR_NULL_PTR);
        if (VA_INVALID_ID == buffer->VaBufferID) return;
        if (buffer->Data != NULL) Unlock(buffer);

        VAStatus sts = m_libva.vaDestroyBuffer(m_dpy, buffer->VaBufferID);
        if (sts != VA_STATUS_SUCCESS) {
            throw mfxError(MFX_ERR_DEVICE_FAILED, "vaDestroyBuffer failed");
        }
        InvalidateBuffer(buffer);
    };

private:
    VABufferType GetVABufferType(mfxU32 bufferId)
    {
        switch (bufferId)
        {
#if MFX_VERSION >= 1025
        case MFX_EXTBUFF_HEVCFEI_ENC_QP:
            return VAEncQPBufferType;
        case MFX_EXTBUFF_HEVCFEI_ENC_MV_PRED:
            return VAEncFEIMVPredictorBufferType;
        case MFX_EXTBUFF_HEVCFEI_ENC_CTU_CTRL:
            return VAEncFEIMBControlBufferType;
#endif
        default:
            throw mfxError(MFX_ERR_UNSUPPORTED, "Unsupported buffer type");
        }
    };

    template<typename T>
    void InvalidateBuffer(T * buffer)
    {
        if (buffer)
        {
            buffer->VaBufferID = VA_INVALID_ID;
            buffer->Data = NULL;
            buffer->Pitch = 0;
            buffer->Height = 0;
        }
    };

    // generic implementation for buffer of 32x32 elements
    template<typename T>
    void CalcBufferPitchHeight(T& buffer,
            const BufferAllocRequest& request,
            mfxU32& va_pitch,
            mfxU32& va_height)
    {
        buffer.Pitch = align(request.Width, CTU_SIZE32) / CTU_SIZE32;
        buffer.Height = align(request.Height, CTU_SIZE32) / CTU_SIZE32;

        // since most buffers have 1D representation in driver, so vaCreateBuffer expects
        // va_height is 1 and va_pitch is total size of buffer->Data array
        va_pitch = sizeof(buffer.Data[0]) * buffer.Pitch * buffer.Height;
        va_height = 1;
    };

    MfxLoader::VA_Proxy m_libva;
    VADisplay m_dpy;
    VAConfigID m_config;
    VAContextID m_context;
};

template<>
void FeiBufferAllocator::CalcBufferPitchHeight(mfxExtFeiHevcEncMVPredictors& buffer, const BufferAllocRequest& request,
                                                 mfxU32& va_pitch, mfxU32& va_height);

template<>
void FeiBufferAllocator::CalcBufferPitchHeight(mfxExtFeiHevcEncQP& buffer, const BufferAllocRequest& request,
                                                 mfxU32& va_pitch, mfxU32& va_height);

// T is external buffer structure
template<typename T>
class AutoBufferLocker
{
public:
    AutoBufferLocker(FeiBufferAllocator & allocator, T & buffer)
        : m_allocator(allocator)
        , m_buffer(buffer)
    {
        m_allocator.Lock(&m_buffer);
    }

    ~AutoBufferLocker()
    {
        m_allocator.Unlock(&m_buffer);
    }

protected:
    FeiBufferAllocator & m_allocator;
    T & m_buffer;

private:
    AutoBufferLocker(AutoBufferLocker const &);
    AutoBufferLocker & operator =(AutoBufferLocker const &);
};

#endif // __FEI_BUFFER_ALLOCATOR_H__
