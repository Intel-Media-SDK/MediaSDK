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

#include "mfx_common.h"

#if defined (MFX_ENABLE_H264_VIDEO_ENCODE_HW)

#ifndef __MFX_H264_ENCODE_INTERFACE__H
#define __MFX_H264_ENCODE_INTERFACE__H

#include <vector>
#include <assert.h>
#include "mfxdefs.h"
#include "mfxvideo++int.h"

#include "mfx_h264_enc_common_hw.h"

// structures will be abstracted (ENCODE_CAPS, D3DDDIFORMAT)
#if defined(MFX_VA_LINUX)
    #include "mfx_h264_encode_struct_vaapi.h"
#endif

#ifdef MFX_ENABLE_MFE
#include "mfx_mfe_adapter.h"
#endif

namespace MfxHwH264Encode
{
    class PreAllocatedVector
    {
    public:
        PreAllocatedVector()
            : m_size(0)
        {
        }

        void Alloc(mfxU32 capacity)
        {
            m_storage.resize(capacity);
            m_size = 0;
        }

        void SetSize(mfxU32 size)
        {
            assert(size <= m_storage.size());
            m_size = size;
        }

        mfxU32 Size() const
        {
            return m_size;
        }

        mfxU32 Capacity() const
        {
            return (mfxU32)m_storage.size();
        }

        mfxU8 * Buffer()
        {
            assert(m_storage.size() > 0);
            return &m_storage[0];
        }

        mfxU8 const * Buffer() const
        {
            assert(m_storage.size() > 0);
            return &m_storage[0];
        }

    private:
        std::vector<mfxU8>  m_storage;
        mfxU32              m_size;
    };


    class DriverEncoder
    {
    public:

        virtual ~DriverEncoder(){}

        virtual
        mfxStatus CreateAuxilliaryDevice(
            VideoCORE * core,
            GUID        guid,
            mfxU32      width,
            mfxU32      height,
            bool        isTemporal = false) = 0;

        virtual
        mfxStatus CreateAccelerationService(
            MfxVideoParam const & par) = 0;

        virtual
        mfxStatus Reset(
            MfxVideoParam const & par) = 0;

        virtual 
        mfxStatus Register(
            mfxFrameAllocResponse & response,
            D3DDDIFORMAT            type) = 0;

        virtual 
        mfxStatus Execute(
            mfxHDLPair                 pair,
            DdiTask const &            task,
            mfxU32                     fieldId,
            PreAllocatedVector const & sei) = 0;

        virtual
        mfxStatus QueryCompBufferInfo(
            D3DDDIFORMAT           type,
            mfxFrameAllocRequest & request) = 0;

        virtual
        mfxStatus QueryEncodeCaps(
            MFX_ENCODE_CAPS & caps) = 0;

        virtual
        mfxStatus QueryMbPerSec(
            mfxVideoParam const & par,
            mfxU32              (&mbPerSec)[16]) = 0;

        virtual
        mfxStatus QueryStatus(
            DdiTask & task,
            mfxU32    fieldId) = 0;

        virtual
        mfxStatus Destroy() = 0;

        virtual
        void ForceCodingFunction (mfxU16 codingFunction) = 0;

        virtual
        mfxStatus QueryHWGUID(
            VideoCORE * core,
            GUID        guid,
            bool        isTemporal) = 0;

        virtual
        mfxStatus QueryEncCtrlCaps(
            ENCODE_ENC_CTRL_CAPS & /*caps*/) { return MFX_ERR_UNSUPPORTED; };

        virtual
        mfxStatus GetEncCtrlCaps(
            ENCODE_ENC_CTRL_CAPS & /*caps*/) { return MFX_ERR_UNSUPPORTED; };

        virtual
        mfxStatus SetEncCtrlCaps(
            ENCODE_ENC_CTRL_CAPS const & /*caps*/) { return MFX_ERR_UNSUPPORTED; };
    };

    DriverEncoder* CreatePlatformH264Encoder( VideoCORE* core ); 

#if defined(MFX_ENABLE_MFE)
    MFEVAAPIEncoder* CreatePlatformMFEEncoder( VideoCORE* core );
#endif

}; // namespace

#endif // __MFX_H264_ENCODE_INTERFACE__H
#endif // #if defined (MFX_ENABLE_H264_VIDEO_ENCODE_HW) && defined(MFX_VA_WIN)
