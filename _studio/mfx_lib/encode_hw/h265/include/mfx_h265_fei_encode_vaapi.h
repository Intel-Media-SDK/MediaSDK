// Copyright (c) 2017-2018 Intel Corporation
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

#pragma once
#include "mfx_config.h"
#if defined(MFX_ENABLE_HEVC_VIDEO_FEI_ENCODE)

#include "mfxfeihevc.h"
#include "mfx_h265_encode_vaapi.h"
#include "mfx_h265_encode_hw_utils.h"

#include <va/va_fei_hevc.h>

namespace MfxHwH265FeiEncode
{

    template <typename T> mfxExtBuffer* GetBufById(T & par, mfxU32 id)
    {
        if (par.NumExtParam && !par.ExtParam)
            return nullptr;

        for (mfxU16 i = 0; i < par.NumExtParam; ++i)
        {
            if (par.ExtParam[i] && par.ExtParam[i]->BufferId == id)
            {
                return par.ExtParam[i];
            }
        }

        return nullptr;
    }

    template <typename T> mfxExtBuffer* GetBufById(T * par, mfxU32 id)
    {
        return par ? GetBufById(*par, id) : nullptr;
    }

    enum VA_BUFFER_STORAGE_ID
    {
        VABID_FEI_FRM_CTRL = MfxHwH265Encode::VABuffersHandler::VABID_END_OF_LIST + 1
    };

    class VAAPIh265FeiEncoder : public MfxHwH265Encode::VAAPIEncoder
    {
    public:
        VAAPIh265FeiEncoder();
        virtual ~VAAPIh265FeiEncoder();

        void SoftReset(const MfxHwH265Encode::MfxVideoParam& par)
        {
            m_videoParam.m_pps.cu_qp_delta_enabled_flag =
                m_pps.pic_fields.bits.cu_qp_delta_enabled_flag = par.m_pps.cu_qp_delta_enabled_flag;

            DDIHeaderPacker::ResetPPS(m_videoParam);
        }

    protected:
        virtual mfxStatus PreSubmitExtraStage(MfxHwH265Encode::Task const & task) override;

        virtual mfxStatus PostQueryExtraStage(MfxHwH265Encode::Task const & task, mfxU32 codedStatus) override;

        virtual VAEntrypoint GetVAEntryPoint() override
        {
            return VAEntrypointFEI;
        }

        virtual mfxStatus ConfigureExtraVAattribs(std::vector<VAConfigAttrib> & attrib) override;

        virtual mfxStatus CheckExtraVAattribs(std::vector<VAConfigAttrib> & attrib) override;
    };
}

#endif
