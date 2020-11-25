// Copyright (c) 2017-2020 Intel Corporation
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

#ifndef __MFX_MJPEG_ENCODE_VAAPI_H__
#define __MFX_MJPEG_ENCODE_VAAPI_H__

#include "mfx_common.h"

#if defined (MFX_ENABLE_MJPEG_VIDEO_ENCODE) && defined (MFX_VA_LINUX)

#include <vector>
#include <assert.h>
#include <va/va.h>
#include <va/va_enc_jpeg.h>
#include "umc_mutex.h"

#include "mfx_ext_buffers.h"

#include "mfx_mjpeg_encode_hw_utils.h"
#include "mfx_mjpeg_encode_interface.h"

namespace MfxHwMJpegEncode
{
    typedef struct
    {
        VASurfaceID surface;
        mfxU32 number;
        mfxU32 idxBs;
        mfxU32 size; // valid only if Surface ID == VA_INVALID_SURFACE (skipped frames)
    } ExtVASurface;

    class VAAPIEncoder : public DriverEncoder
    {
    public:
        VAAPIEncoder();

        virtual
        ~VAAPIEncoder();

        virtual
        mfxStatus CreateAuxilliaryDevice(
            VideoCORE * core,
            mfxU32      width,
            mfxU32      height,
            bool        isTemporal = false) override;

        virtual
        mfxStatus CreateAccelerationService(
            mfxVideoParam const & par) override;

        virtual
        mfxStatus RegisterBitstreamBuffer(
            mfxFrameAllocResponse & response) override;

        virtual
        mfxStatus Execute(DdiTask &task, mfxHDL surface) override;

        virtual
        mfxStatus QueryBitstreamBufferInfo(
            mfxFrameAllocRequest & request) override;

        virtual
        mfxStatus QueryEncodeCaps(
            JpegEncCaps & caps) override;

        virtual
        mfxStatus QueryStatus(
            DdiTask & task) override;

        virtual
        mfxStatus UpdateBitstream(
            mfxMemId    MemId,
            DdiTask   & task) override;

        virtual
        mfxStatus Destroy() override;

        VAAPIEncoder(VAAPIEncoder const &) = delete;
        VAAPIEncoder & operator =(VAAPIEncoder const &) = delete;

    private:
        mfxStatus DestroyBuffers();
        mfxStatus FillPriorityBuffer(mfxPriority&);

        VideoCORE       * m_core;
        mfxU32            m_width;
        mfxU32            m_height;
        JpegEncCaps       m_caps;
        VADisplay         m_vaDisplay;
        VAContextID       m_vaContextEncode;
        VAConfigID        m_vaConfig;

        UMC::Mutex        m_guard;
        std::vector<ExtVASurface> m_feedbackCache;
        std::vector<ExtVASurface> m_bsQueue;

        VABufferID  m_qmBufferId;
        VABufferID  m_htBufferId;
        VABufferID  m_scanBufferId;
        VABufferID  m_ppsBufferId;
        VABufferID  m_priorityBufferId;
        std::vector<VABufferID>  m_appBufferIds;;

        VAContextParameterUpdateBuffer   m_priorityBuffer;
        mfxU32                           m_MaxContextPriority;
    };

}; // namespace

#endif // #if defined (MFX_ENABLE_MJPEG_VIDEO_ENCODE) && defined (MFX_VA_LINUX)
#endif // __MFX_MJPEG_ENCODE_VAAPI_H__
