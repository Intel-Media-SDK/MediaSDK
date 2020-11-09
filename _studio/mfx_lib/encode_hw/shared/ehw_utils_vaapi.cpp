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

#include "mfx_common.h"

#if defined(MFX_VA_LINUX)
#include "ehw_utils_vaapi.h"
#include "ehw_device_vaapi.h"

namespace MfxEncodeHW
{

DDIExecParam VAAPIParPacker::PackVaMiscPar(std::vector<mfxU8>& data)
{
    DDIExecParam par;
    par.Function = VAEncMiscParameterBufferType;
    par.In.pData = data.data();
    par.In.Size  = (mfxU32)data.size();
    par.In.Num   = 1;
    return par;
}

void VAAPIParPacker::PackResources(DDIExecParam::Param& par, mfxU32 type)
{
    auto& res = m_resources.at(type);
    par.pData = res.data();
    par.Size  = sizeof(VAGenericID);
    par.Num   = mfxU32(res.size());
}

mfxStatus VAAPIParPacker::Register(
    VideoCORE& core
    , const mfxFrameAllocResponse& response
    , mfxU32 type)
{
    auto MidToVA = [&](mfxMemId mid) -> VASurfaceID
    {
        VASurfaceID *pSurface = nullptr;
        auto sts = core.GetFrameHDL(mid, (mfxHDL*)&pSurface);
        MFX_CHECK(!sts, VA_INVALID_SURFACE);
        return *pSurface;
    };
    auto& res = m_resources[type];

    res.resize(response.NumFrameActual);

    std::transform(response.mids, response.mids + response.NumFrameActual, res.begin(), MidToVA);

    bool bFailed = std::find(res.begin(), res.end(), VA_INVALID_SURFACE) != res.end();
    MFX_CHECK(!bFailed, MFX_ERR_DEVICE_FAILED);

    return MFX_ERR_NONE;
}

mfxStatus VAAPIParPacker::ReadFeedback(const VACodedBufferSegment& fb, mfxU32& bsSize)
{
    MFX_CHECK(!(fb.status & VA_CODED_BUF_STATUS_BAD_BITSTREAM), MFX_ERR_GPU_HANG);
    MFX_CHECK(fb.buf && fb.size, MFX_ERR_DEVICE_FAILED);
    MFX_CHECK(m_maxBsSize >= fb.size, MFX_ERR_DEVICE_FAILED);

    bsSize = fb.size;

    return MFX_ERR_NONE;
}

void VAAPIParPacker::SetFeedback(mfxU32 fbId, VASurfaceID sId, VABufferID bsId)
{
    auto& fb = m_feedback.at(FeedbackIDWrap(fbId));
    fb.csb = {};
    std::get<1>(fb.ssPar) = sId;
    std::get<1>(fb.mbPar) = bsId;
}

void VAAPIParPacker::GetFeedbackInterface(DDIFeedback& fb)
{
    fb = DDIFeedback();

    DDIExecParam xPar;

    xPar.Function = mfxU32(DeviceVAAPI::VAFID_SyncSurface);
    fb.ExecParam.push_back(xPar);

    auto pSS = &fb.ExecParam.back();

    xPar.Function   = mfxU32(DeviceVAAPI::VAFID_MapBuffer);
    xPar.Out.pData  = &m_feedbackTmp.csb;
    xPar.Out.Size   = sizeof(VACodedBufferSegment);
    xPar.Out.Num    = 1;
    fb.ExecParam.push_back(xPar);

    auto pMB = &fb.ExecParam.back();

    fb.Get.Push([this, pMB, pSS](DDIFeedback::TGet::TExt, mfxU32 id) -> const void*
    {
        if (!FeedbackReady(id))
        {
            auto& currFB = m_feedback.at(FeedbackIDWrap(id));

            pSS->In.pData = &currFB.ssPar;
            pSS->In.Size  = sizeof(currFB.ssPar);
            pSS->In.Num   = 1;

            pMB->In.pData = &currFB.mbPar;
            pMB->In.Size  = sizeof(currFB.mbPar);
            pMB->In.Num   = 1;

            return nullptr;
        }
        return &m_feedback.at(FeedbackIDWrap(id)).csb;
    });
    fb.Update.Push([this](DDIFeedback::TUpdate::TExt, mfxU32 id)
    {
        MFX_CHECK(!FeedbackReady(id), MFX_ERR_UNDEFINED_BEHAVIOR);
        m_feedback.at(FeedbackIDWrap(id)).csb = m_feedbackTmp.csb;
        m_feedbackTmp.csb = {};
        FeedbackReady(id) = true;
        return MFX_ERR_NONE;
    });
    fb.Remove.Push([this](DDIFeedback::TUpdate::TExt, mfxU32 id)
    {
        FeedbackReady(id) = false;
        return MFX_ERR_NONE;
    });
}

bool VAAPIParPacker::AddPackedHeaderIf(
    bool cond
    , const PackedData pd
    , std::list<DDIExecParam>& par
    , uint32_t type)
{
    if (!cond)
        return false;

    VAEncPackedHeaderParameterBuffer vaPh = {};
    vaPh.type                = type;
    vaPh.bit_length          = pd.BitLen;
    vaPh.has_emulation_bytes = pd.bHasEP;
    m_vaPackedHeaders.push_back(vaPh);

    DDIExecParam xPar;
    xPar.Function = VAEncPackedHeaderParameterBufferType;
    xPar.In.pData = &m_vaPackedHeaders.back();
    xPar.In.Size  = sizeof(VAEncPackedHeaderParameterBuffer);
    xPar.In.Num   = 1;
    par.push_back(xPar);

    xPar.Function = VAEncPackedHeaderDataBufferType;
    xPar.In.pData = pd.pData;
    xPar.In.Size  = (pd.BitLen + 7) / 8;
    xPar.In.Num   = 1;
    par.push_back(xPar);

    return true;
}

DDIExecParam VAAPIParPacker::PackVaBuffer(VABufferType type, void* pData, mfxU32 size, mfxU32 num)
{
    DDIExecParam xPar;
    xPar.Function = type;
    xPar.In.pData = pData;
    xPar.In.Size  = size;
    xPar.In.Num   = num;
    return xPar;
}

} //namespace MfxEncodeHW

#endif //defined(MFX_VA_LINUX)