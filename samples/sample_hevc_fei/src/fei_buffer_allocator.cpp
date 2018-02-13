/******************************************************************************\
Copyright (c) 2016-2018, Intel Corporation
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

#if defined(LIBVA_SUPPORT)

#include "fei_buffer_allocator.h"

FeiBufferAllocator::FeiBufferAllocator(VADisplay dpy, mfxU32 width, mfxU32 height) :
    m_dpy(dpy),
    m_config(VA_INVALID_ID),
    m_context(VA_INVALID_ID)
{
    VAStatus vaSts = VA_STATUS_SUCCESS;
    VAConfigAttrib attrib;
    attrib.type = VAConfigAttribRateControl;
    attrib.value = VA_RC_CQP;

    vaSts = m_libva.vaCreateConfig(m_dpy, VAProfileHEVCMain, VAEntrypointFEI, &attrib, 1, &m_config);
    if (VA_STATUS_SUCCESS != vaSts) {
        throw mfxError(MFX_ERR_DEVICE_FAILED, "Failed to create VA config for buffer allocator");
    }

    // width and height should be max of required resolutions for encoding
    vaSts = m_libva.vaCreateContext(m_dpy, m_config, width, height, VA_PROGRESSIVE, NULL, 0, &m_context);
    if (VA_STATUS_SUCCESS != vaSts) {
        throw mfxError(MFX_ERR_DEVICE_FAILED, "Failed to create VA context for buffer allocator");
    }
}

FeiBufferAllocator::~FeiBufferAllocator()
{
    if (m_context != VA_INVALID_ID) {
        m_libva.vaDestroyContext(m_dpy, m_context);
    }
    if (m_config != VA_INVALID_ID) {
        m_libva.vaDestroyConfig(m_dpy, m_config);
    }
}

template<>
void FeiBufferAllocator::CalcBufferPitchHeight(mfxExtFeiHevcEncMVPredictors& buffer,
        const BufferAllocRequest& request,
        mfxU32& va_pitch,
        mfxU32& va_height)
{
    /* Driver uses blocks of 16x16 pixels for CTUs representation.
       The layout of data inside those blocks is related to CTU size,
       but the buffer size itself - doesn't.
    */
    const mfxU32 element_size = 16; // Buffers granularity is always 16x16 blocks
    buffer.Pitch = align<CTU_SIZE32>(request.Width) / element_size;
    buffer.Height = align<CTU_SIZE32>(request.Height) / element_size;

    // buffer has 1D representation in driver, so vaCreateBuffer expects
    // va_height is 1 and va_pitch is total size of buffer->Data array
    va_pitch = sizeof(buffer.Data[0]) * buffer.Pitch * buffer.Height;
    va_height = 1;
}

template<>
void FeiBufferAllocator::CalcBufferPitchHeight(mfxExtFeiHevcEncQP& buffer,
        const BufferAllocRequest& request,
        mfxU32& va_pitch,
        mfxU32& va_height)
{
    // driver requirement for per block QP buffer: width of picture in CTB unit * 1 byte should be
    // multiple of 64 bytes, height of picture in CTB unit * 1 byte - multiple of 4 bytes

    mfxU32 w_num_elem = align<CTU_SIZE32>(request.Width) / CTU_SIZE32;
    const mfxU32 num_w_blocks_alignment = 64;
    buffer.Pitch = align<num_w_blocks_alignment>(w_num_elem);

    mfxU32 h_num_elem = align<CTU_SIZE32>(request.Height) / CTU_SIZE32;
    const mfxU32 num_h_blocks_alignment = 4;
    buffer.Height = align<num_h_blocks_alignment>(h_num_elem);

    // buffer has 2D representation in driver
    va_pitch = buffer.Pitch;
    va_height = buffer.Height;
}

#endif // #if defined(LIBVA_SUPPORT)
