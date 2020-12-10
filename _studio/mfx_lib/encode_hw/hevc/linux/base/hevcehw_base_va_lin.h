// Copyright (c) 2019-2020 Intel Corporation
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
#include "hevcehw_base.h"

#if defined(MFX_ENABLE_H265_VIDEO_ENCODE) && defined (MFX_VA_LINUX)

#include "hevcehw_ddi.h"
#include "hevcehw_base_data.h"
#include "hevcehw_base_iddi.h"
#include "ehw_device_vaapi.h"

namespace HEVCEHW
{
namespace Linux
{
namespace Base
{
using namespace HEVCEHW::Base;

class DDI_VA
    : public IDDI
    , public MfxEncodeHW::DeviceVAAPI
{
public:
    DDI_VA(mfxU32 FeatureId)
        : IDDI(FeatureId)
    {
        SetTraceName("Base_DDI_VA");
    }

    static VAGUID MapGUID(StorageR& strg, const GUID& guid);

protected:
    virtual void Query1NoCaps(const FeatureBlocks& blocks, TPushQ1 Push) override;
    virtual void Query1WithCaps(const FeatureBlocks& blocks, TPushQ1 Push) override;
    virtual void InitExternal(const FeatureBlocks& blocks, TPushIE Push) override;
    virtual void InitAlloc(const FeatureBlocks& blocks, TPushIA Push) override;
    virtual void SubmitTask(const FeatureBlocks& blocks, TPushST Push) override;
    virtual void QueryTask(const FeatureBlocks& blocks, TPushQT Push) override;
    virtual void ResetState(const FeatureBlocks& blocks, TPushRS Push) override;

    mfxStatus CreateVABuffers(
        const std::list<DDIExecParam>& par
        , std::vector<VABufferID>& pool);

    mfxStatus DestroyVABuffers(std::vector<VABufferID>& pool);

    using MfxEncodeHW::DeviceVAAPI::QueryCaps;
    mfxStatus QueryCaps();
    uint32_t  ConvertRateControlMFX2VAAPI(mfxU16 rateControl, bool bSWBRC);
    uint32_t  ConvertSliceStructureVAAPIToMFX(uint32_t structure);

    EncodeCapsHevc          m_caps;
    std::vector<VABufferID> m_perSeqPar;
    std::vector<VABufferID> m_perPicPar;
    std::vector<VABufferID> m_bs;
    PriorityParam           m_priorityPar;
};

} //Base
} //Linux
} //namespace HEVCEHW

#endif
