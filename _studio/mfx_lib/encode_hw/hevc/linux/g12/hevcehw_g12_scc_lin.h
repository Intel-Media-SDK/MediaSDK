// Copyright (c) 2020 Intel Corporation
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

#include "mfx_common.h"
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE) && defined (MFX_VA_LINUX)

#include "hevcehw_g12_scc.h"
#include "va/va.h"
#include "hevcehw_base_va_packer_lin.h"

namespace HEVCEHW
{
namespace Linux
{
namespace Gen12
{
class SCC
    : public HEVCEHW::Gen12::SCC
{
public:
    SCC(mfxU32 FeatureId)
        : HEVCEHW::Gen12::SCC(FeatureId)
    {}
protected:
    virtual void SetExtraGUIDs(StorageRW& strg) override
    {
        auto& g2va = HEVCEHW::Gen12::Glob::GuidToVa::GetOrConstruct(strg);
        g2va[DXVA2_Intel_LowpowerEncode_HEVC_SCC_Main]       = { VAProfileHEVCSccMain,    VAEntrypointEncSliceLP };
        g2va[DXVA2_Intel_LowpowerEncode_HEVC_SCC_Main10]     = { VAProfileHEVCSccMain10,  VAEntrypointEncSliceLP };
        g2va[DXVA2_Intel_LowpowerEncode_HEVC_SCC_Main444]    = { VAProfileHEVCSccMain444, VAEntrypointEncSliceLP };
        g2va[DXVA2_Intel_LowpowerEncode_HEVC_SCC_Main444_10] = { VAProfileHEVCSccMain444_10, VAEntrypointEncSliceLP };
    };

    void SubmitTask(const FeatureBlocks& /*blocks*/, TPushST Push) override
    {
        Push(BLK_PatchDDITask
            , [this](StorageW& global, StorageW& /*s_task*/) -> mfxStatus
        {
            MFX_CHECK(m_bPatchNextDDITask || m_bPatchDDISlices, MFX_ERR_NONE);
            auto& ddiPar = HEVCEHW::Gen12::Glob::DDI_SubmitParam::Get(global);
            auto  itPPS  = std::find_if(std::begin(ddiPar), std::end(ddiPar)
                , [](HEVCEHW::Base::DDIExecParam& ep) { return (ep.Function == VAEncPictureParameterBufferType); });
            MFX_CHECK(itPPS != std::end(ddiPar) && itPPS->In.pData, MFX_ERR_NOT_FOUND);
            auto& ddiPPS    = *(VAEncPictureParameterBufferHEVC*)itPPS->In.pData;

            if(m_bPatchNextDDITask)
            {
                m_bPatchNextDDITask = false;
                auto  itSPS   = std::find_if(std::begin(ddiPar), std::end(ddiPar)
                    , [](HEVCEHW::Base::DDIExecParam& ep) { return (ep.Function == VAEncSequenceParameterBufferType); });
                MFX_CHECK(itSPS != std::end(ddiPar) && itSPS->In.pData, MFX_ERR_NOT_FOUND);
                auto& ddiSPS    = *(VAEncSequenceParameterBufferHEVC*)itSPS->In.pData;

                ddiSPS.scc_fields.bits.palette_mode_enabled_flag = SpsExt::Get(global).palette_mode_enabled_flag;
                ddiPPS.scc_fields.bits.pps_curr_pic_ref_enabled_flag = PpsExt::Get(global).curr_pic_ref_enabled_flag;
            }
            return MFX_ERR_NONE;
        });
    }
};
} //Gen12
} //Linux
} //namespace HEVCEHW

#endif
