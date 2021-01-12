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
#include "ehw_device_vaapi.h"
#include <algorithm>
#include "feature_blocks/mfx_feature_blocks_utils.h"

namespace MfxEncodeHW
{
using namespace MfxFeatureBlocks;

mfxStatus DeviceVAAPI::Create(
    VideoCORE&    core
    , GUID        guid
    , mfxU32      /*width*/
    , mfxU32      /*height*/
    , bool        /*isTemporal*/)
{
    mfxStatus sts = core.GetHandle(MFX_HANDLE_VA_DISPLAY, (mfxHDL*)&m_vaDisplay);
    MFX_CHECK_STS(sts);

    TVAGUID vaguid = {};
    vaguid.guid = guid;

    m_profile    = vaguid.va.profile;
    m_entrypoint = vaguid.va.entrypoint;
    m_pCore      = &core;

    m_callVa = [this](const DDIExecParam& par) { return Execute(par); };

    return MFX_ERR_NONE;
}

void DeviceVAAPI::Destroy()
{
    m_callVa = [this](const DDIExecParam& par) { return Execute(par); };

    std::set<VABufferID> vaBuffers;
    vaBuffers.swap(m_vaBuffers);

    for (auto id : vaBuffers)
    {
        DestroyVABuffer(id);
    }

    if (m_vaContextEncode != VA_INVALID_ID)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaDestroyContext");
        m_lastErr = Execute(VAFID_DestroyContext, m_vaDisplay, m_vaContextEncode);
        m_vaContextEncode = VA_INVALID_ID;
    }

    if (m_vaConfig != VA_INVALID_ID)
    {
        m_lastErr = Execute(VAFID_DestroyConfig, m_vaDisplay, m_vaConfig);
        m_vaConfig = VA_INVALID_ID;
    }

    m_vaDisplay = nullptr;
}

mfxStatus DeviceVAAPI::QueryCaps(void* pCaps, mfxU32 size)
{
    MFX_CHECK(IsValid(), MFX_ERR_NOT_INITIALIZED);

    int num = (size / sizeof(VAConfigAttrib));

    auto sts = Execute(
        VAFID_GetConfigAttributes
        , m_vaDisplay
        , m_profile
        , m_entrypoint
        , (VAConfigAttrib*)pCaps
        , num);
    MFX_CHECK_STS(sts);

    return MFX_ERR_NONE;
}

mfxStatus DeviceVAAPI::QueryCompBufferInfo(mfxU32 /*type*/, mfxFrameInfo& /*info*/)
{
    return MFX_ERR_UNSUPPORTED;
}

mfxStatus DeviceVAAPI::Init(const std::list<DDIExecParam>* pPar)
{
    MFX_CHECK(IsValid(), MFX_ERR_NOT_INITIALIZED);
    MFX_CHECK(pPar, MFX_ERR_UNDEFINED_BEHAVIOR);

    auto itCreateConfig = std::find_if(pPar->begin(), pPar->end(), DDIExecParam::IsFunction<VAFID_CreateConfig>);
    MFX_CHECK(itCreateConfig != pPar->end(), MFX_ERR_UNDEFINED_BEHAVIOR);

    auto itCreateContext = std::find_if(pPar->begin(), pPar->end(), DDIExecParam::IsFunction<VAFID_CreateContext>);
    MFX_CHECK(itCreateContext != pPar->end(), MFX_ERR_UNDEFINED_BEHAVIOR);

    mfxI32 numEntrypoints = vaMaxNumEntrypoints(m_vaDisplay);
    MFX_CHECK(numEntrypoints, MFX_ERR_DEVICE_FAILED);

    std::vector<VAEntrypoint> ep_list(numEntrypoints);
    std::vector<VAProfile> profile_list(vaMaxNumProfiles(m_vaDisplay), VAProfileNone);
    mfxI32 num_profiles = 0;
    mfxStatus sts = MFX_ERR_NONE;

    sts = Execute(
        VAFID_QueryConfigProfiles
        , m_vaDisplay
        , profile_list.data()
        , &num_profiles);
    MFX_CHECK_STS(sts);
    MFX_CHECK(std::find(profile_list.begin(), profile_list.end(), m_profile) != profile_list.end(), MFX_ERR_DEVICE_FAILED);

    sts = Execute(
        VAFID_QueryConfigEntrypoints
        , m_vaDisplay
        , m_profile
        , ep_list.data()
        , &numEntrypoints);
    MFX_CHECK_STS(sts);
    MFX_CHECK(std::find(ep_list.begin(), ep_list.end(), m_entrypoint) != ep_list.end(), MFX_ERR_DEVICE_FAILED);

    auto& cfgPar = GetArgs(itCreateConfig->In, vaCreateConfig);
    std::get<0>(cfgPar) = m_vaDisplay;
    std::get<1>(cfgPar) = m_profile;
    std::get<2>(cfgPar) = m_entrypoint;
    std::get<5>(cfgPar) = &m_vaConfig;

    sts = Execute(*itCreateConfig);
    MFX_CHECK_STS(sts);

    auto& ctxPar = GetArgs(itCreateContext->In, vaCreateContext);
    std::get<0>(ctxPar) = m_vaDisplay;
    std::get<1>(ctxPar) = m_vaConfig;
    std::get<7>(ctxPar) = &m_vaContextEncode;

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaCreateContext");
        sts = Execute(*itCreateContext);
    }
    MFX_CHECK_STS(sts);

    return MFX_ERR_NONE;
}

mfxStatus DeviceVAAPI::Execute(const DDIExecParam& ep)
{
    bool bUnsupported = false;

    m_lastErr = m_ddiExec.at(VAFID(ep.Function))(ep);

    bUnsupported |= ep.Function == VAFID_CreateContext && (VA_STATUS_ERROR_RESOLUTION_NOT_SUPPORTED == m_lastErr);
    bUnsupported |= ep.Function == VAFID_GetConfigAttributes && (VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT == m_lastErr);
    bUnsupported |= ep.Function == VAFID_GetConfigAttributes && (VA_STATUS_ERROR_UNSUPPORTED_PROFILE == m_lastErr);

    MFX_CHECK(!bUnsupported, MFX_ERR_UNSUPPORTED);
    MFX_CHECK(VA_STATUS_SUCCESS == m_lastErr, MFX_ERR_DEVICE_FAILED);

    return MFX_ERR_NONE;
}

mfxStatus DeviceVAAPI::BeginPicture(mfxHDL hdl)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaBeginPicture");

    return Execute(VAFID_BeginPicture
        , m_vaDisplay
        , m_vaContextEncode
        , *(VASurfaceID*)hdl);
}

mfxStatus DeviceVAAPI::EndPicture()
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaEndPicture");

    return Execute(VAFID_EndPicture
        , m_vaDisplay
        , m_vaContextEncode);
}

VABufferID DeviceVAAPI::CreateVABuffer(const DDIExecParam& ep)
{
    MFX_CHECK(IsValid(), MFX_ERR_NOT_INITIALIZED);

    VABufferID   id   = VA_INVALID_ID;
    VABufferType type = VABufferType(ep.Function);
    mfxStatus    sts;

    if (type != VAEncMiscParameterBufferType)
    {
        sts = Execute(
            VAFID_CreateBuffer
            , m_vaDisplay
            , m_vaContextEncode
            , type
            , ep.In.Size
            , std::max<mfxU32>(ep.In.Num, 1)
            , ep.In.pData
            , &id);
        MFX_CHECK(!sts, VA_INVALID_ID);

        m_vaBuffers.insert(id);

        return id;
    }

    sts = Execute(
        VAFID_CreateBuffer
        , m_vaDisplay
        , m_vaContextEncode
        , VAEncMiscParameterBufferType
        , ep.In.Size
        , 1
        , nullptr
        , &id);
    MFX_CHECK(!sts, VA_INVALID_ID);

    VAEncMiscParameterBuffer *pMP;

    sts = Execute(
        VAFID_MapBuffer
        , m_vaDisplay
        , id
        , (void **)&pMP);
    MFX_CHECK(!sts, VA_INVALID_ID);

    pMP->type = ((VAEncMiscParameterBuffer*)ep.In.pData)->type;
    mfxU8* pDst = (mfxU8*)pMP->data;
    mfxU8* pSrc = (mfxU8*)ep.In.pData + sizeof(VAEncMiscParameterBuffer);

    std::copy(pSrc, pSrc + ep.In.Size - sizeof(VAEncMiscParameterBuffer), pDst);

    sts = Execute(
        VAFID_UnmapBuffer
        , m_vaDisplay
        , id);
    MFX_CHECK(!sts, VA_INVALID_ID);

    m_vaBuffers.insert(id);

    return id;
}

mfxStatus DeviceVAAPI::DestroyVABuffer(VABufferID id)
{
    m_vaBuffers.erase(id);
    return Execute(VAFID_DestroyBuffer, m_vaDisplay, id);
}

VASurfaceID DeviceVAAPI::GetVASurface(mfxMemId mid)
{
    MFX_CHECK(IsValid(), VA_INVALID_SURFACE);

    VASurfaceID *pSurface = nullptr;
    mfxStatus sts = m_pCore->GetFrameHDL(mid, (mfxHDL*)&pSurface);
    MFX_CHECK(!sts && pSurface, VA_INVALID_SURFACE);

    return *pSurface;
}

mfxStatus DeviceVAAPI::Init(
    int width
    , int height
    , int flag
    , mfxFrameAllocResponse recResponse
    , VAConfigAttrib* pAttr
    , int nAttr
    , std::list<DDIExecParam>* pPar)
{
    std::vector<VASurfaceID> rec(recResponse.NumFrameActual, VA_INVALID_SURFACE);
    std::list<DDIExecParam> xPar;
    mfx::TupleArgs<decltype(vaCreateConfig)>::type argsCfg;
    mfx::TupleArgs<decltype(vaCreateContext)>::type argsCtx;

    if (!pPar)
        pPar = &xPar;

    bool bNeedCreateConfig  = pPar->end() == std::find_if(pPar->begin(), pPar->end(), DDIExecParam::IsFunction<VAFID_CreateConfig>);
    bool bNeedCreateContext = pPar->end() == std::find_if(pPar->begin(), pPar->end(), DDIExecParam::IsFunction<VAFID_CreateContext>);

    if (bNeedCreateConfig)
    {
        std::get<3>(argsCfg) = pAttr;
        std::get<4>(argsCfg) = nAttr;

        DDIExecParam xPar;
        xPar.Function = VAFID_CreateConfig;
        xPar.In.pData = &argsCfg;
        xPar.In.Size  = sizeof(argsCfg);

        pPar->push_back(xPar);
    }

    if (bNeedCreateContext)
    {
        std::transform(
            recResponse.mids
            , recResponse.mids + recResponse.NumFrameActual
            , rec.begin()
            , [this](mfxMemId mid) { return GetVASurface(mid); });

        MFX_CHECK(std::find(rec.begin(), rec.end(), VA_INVALID_SURFACE) == rec.end(), MFX_ERR_DEVICE_FAILED);

        std::get<2>(argsCtx) = width;
        std::get<3>(argsCtx) = height;
        std::get<4>(argsCtx) = flag;
        std::get<5>(argsCtx) = rec.data();
        std::get<6>(argsCtx) = (int)rec.size();

        DDIExecParam xPar;
        xPar.Function = VAFID_CreateContext;
        xPar.In.pData = &argsCtx;
        xPar.In.Size  = sizeof(argsCtx);

        pPar->push_back(xPar);
    }

    return Init(pPar);
}

mfxStatus DeviceVAAPI::QueryStatus(DDIFeedback& ddiFB, mfxU32 id)
{
    MFX_CHECK(!ddiFB.Get(id), MFX_ERR_NONE);
    auto itMB = std::find_if(ddiFB.ExecParam.begin(), ddiFB.ExecParam.end(), DDIExecParam::IsFunction<VAFID_MapBuffer>);
    MFX_CHECK(itMB != ddiFB.ExecParam.end(), MFX_ERR_UNDEFINED_BEHAVIOR);

    auto& mbPar = GetArgs(itMB->In, vaMapBuffer);
    // Synchronization using vaSyncBuffer is temporarily disabled
#if 0
    auto sts = SyncBuffer(std::get<1>(mbPar), VA_TIMEOUT_INFINITE);
#else
    auto itSS = std::find_if(ddiFB.ExecParam.begin(), ddiFB.ExecParam.end(), DDIExecParam::IsFunction<VAFID_SyncSurface>);
    MFX_CHECK(itSS != ddiFB.ExecParam.end(), MFX_ERR_UNDEFINED_BEHAVIOR);

    auto& ssPar = GetArgs(itSS->In, vaSyncSurface);

    auto sts = SyncSurface(std::get<1>(ssPar));
#endif
    MFX_CHECK_STS(sts);

    VACodedBufferSegment* pCBS = nullptr;
    sts = MapBuffer(std::get<1>(mbPar), (void **)&pCBS);
    MFX_CHECK_STS(sts);
    MFX_CHECK(pCBS, MFX_ERR_DEVICE_FAILED);

    itMB->Out.Cast<VACodedBufferSegment>() = *pCBS;

    sts = UnmapBuffer(std::get<1>(mbPar));
    MFX_CHECK_STS(sts);

    ddiFB.bNotReady = false;

    return ddiFB.Update(id);
}

} //namespace MfxEncodeHW
#endif //defined(MFX_VA_LINUX)