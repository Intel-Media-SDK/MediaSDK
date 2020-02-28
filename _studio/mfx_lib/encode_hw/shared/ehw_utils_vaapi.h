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

#if defined(MFX_VA_LINUX)
#include "ehw_device.h"
#include "va/va.h"
#include <vector>
#include <list>

namespace MfxEncodeHW
{

template<class T>
T& AddVaMisc(
    VAEncMiscParameterType type
    , std::list<std::vector<mfxU8>>& buf)
{
    const size_t szH = sizeof(VAEncMiscParameterBuffer);
    const size_t szB = sizeof(T);
    buf.push_back(std::vector<mfxU8>(szH + szB, 0));
    auto pMisc = (VAEncMiscParameterBuffer*)buf.back().data();
    pMisc->type = type;
    return *(T*)(buf.back().data() + szH);
}

class VAAPIParPacker
{
public:
    enum eResId
    {
        RES_BS = 0
        , RES_REF
        , NUM_RES
    };

    static DDIExecParam PackVaMiscPar(std::vector<mfxU8>& data);

    mfxStatus Register(
        VideoCORE& core
        , const mfxFrameAllocResponse& response
        , mfxU32 type);

    void PackResources(DDIExecParam::Param& par, mfxU32 type);

    void SetMaxBs(mfxU32 sizeInBytes)
    {
        m_maxBsSize = sizeInBytes;
    }

    void InitFeedback(mfxU16 cacheSize)
    {
        m_feedback.resize(cacheSize);
    }

    mfxStatus ReadFeedback(const VACodedBufferSegment& fb, mfxU32& bsSize);

    void GetFeedbackInterface(DDIFeedback& fb);
    
    bool AddPackedHeaderIf(
        bool cond
        , const PackedData pd
        , std::list<DDIExecParam>& par
        , uint32_t type = VAEncPackedHeaderRawData);

    const std::vector<VAGenericID>& GetResources(mfxU32 type)
    {
        return m_resources.at(type);
    }

    static DDIExecParam PackVaBuffer(VABufferType type, void* pData, mfxU32 size, mfxU32 num = 1);

    template<class T>
    static DDIExecParam PackVaBuffer(VABufferType type, T& src)
    {
        return PackVaBuffer(type, &src, mfxU32(sizeof(T)), 1);
    }

    void SetFeedback(mfxU32 fbId, VASurfaceID sId, VABufferID bsId);

protected:
    using TVaSS = mfx::TupleArgs<decltype(vaSyncSurface)>::type;
    using TVaMB = mfx::TupleArgs<decltype(vaMapBuffer)>::type;
    struct Feedback
    {
        TVaSS ssPar;
        TVaMB mbPar;
        VACodedBufferSegment csb = {};
    };

    mfxU32                                      m_maxBsSize = mfxU32(-1);
    std::map<mfxU32, std::vector<VAGenericID>>  m_resources;
    Feedback                                    m_feedbackTmp;
    std::vector<Feedback>                       m_feedback;
    std::map<mfxU32, bool>                      m_fbReady;
    std::list<VAEncPackedHeaderParameterBuffer> m_vaPackedHeaders;

    mfxU32 FeedbackIDWrap(mfxU32 id) { return (id % m_feedback.size()); }
    bool&  FeedbackReady(mfxU32 id) { return m_fbReady[FeedbackIDWrap(id)]; }
};

} //namespace MfxEncodeHW

#endif //defined(MFX_VA_LINUX)