// Copyright (c) 2019 Intel Corporation
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
#include "hevcehw_g9_va_lin.h"

#if defined(MFX_ENABLE_H265_VIDEO_ENCODE) && defined (MFX_VA_LINUX)

#include "hevcehw_g9_legacy.h"

using namespace HEVCEHW;
using namespace HEVCEHW::Gen9;
using namespace HEVCEHW::Linux;
using namespace HEVCEHW::Linux::Gen9;

static const Glob::GuidToVa::TRef DefaultGuidToVa =
{
    { DXVA2_Intel_Encode_HEVC_Main,                 VAGUID{VAProfileHEVCMain,       VAEntrypointEncSlice}}
    , { DXVA2_Intel_Encode_HEVC_Main10,             VAGUID{VAProfileHEVCMain10,     VAEntrypointEncSlice}}
    , { DXVA2_Intel_LowpowerEncode_HEVC_Main,       VAGUID{VAProfileHEVCMain,       VAEntrypointEncSliceLP}}
    , { DXVA2_Intel_LowpowerEncode_HEVC_Main10,     VAGUID{VAProfileHEVCMain10,     VAEntrypointEncSliceLP}}
    , { DXVA2_Intel_Encode_HEVC_Main422,            VAGUID{VAProfileHEVCMain422_10, VAEntrypointEncSlice}} // Unsupported by VA
    , { DXVA2_Intel_Encode_HEVC_Main422_10,         VAGUID{VAProfileHEVCMain422_10,  VAEntrypointEncSlice}}
    , { DXVA2_Intel_Encode_HEVC_Main444,            VAGUID{VAProfileHEVCMain444,     VAEntrypointEncSlice}}
    , { DXVA2_Intel_Encode_HEVC_Main444_10,         VAGUID{VAProfileHEVCMain444_10,  VAEntrypointEncSlice}}
    , { DXVA2_Intel_LowpowerEncode_HEVC_Main422,    VAGUID{VAProfileHEVCMain422_10,  VAEntrypointEncSliceLP}} // Unsupported by VA
    , { DXVA2_Intel_LowpowerEncode_HEVC_Main422_10, VAGUID{VAProfileHEVCMain422_10, VAEntrypointEncSliceLP}}
    , { DXVA2_Intel_LowpowerEncode_HEVC_Main444,    VAGUID{VAProfileHEVCMain444,    VAEntrypointEncSliceLP}}
    , { DXVA2_Intel_LowpowerEncode_HEVC_Main444_10, VAGUID{VAProfileHEVCMain444_10, VAEntrypointEncSliceLP}}
};


VAGUID DDI_VA::MapGUID(StorageR& strg, const GUID& guid)
{
    if (strg.Contains(Glob::GuidToVa::Key))
    {
        auto& g2va = Glob::GuidToVa::Get(strg);
        if (g2va.find(guid) != g2va.end())
            return g2va.at(guid);
    }

    return DefaultGuidToVa.at(guid);
}

mfxStatus DDI_VA::CallVaDefault(const DDIExecParam& ep)
{
    bool bUnsupported = false;

    auto vaSts = m_ddiExec.at(VAFID(ep.Function))(ep);

    bUnsupported |= ep.Function == VAFID_CreateContext && (VA_STATUS_ERROR_RESOLUTION_NOT_SUPPORTED == vaSts);
    bUnsupported |= ep.Function == VAFID_GetConfigAttributes && (VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT == vaSts);
    bUnsupported |= ep.Function == VAFID_GetConfigAttributes && (VA_STATUS_ERROR_UNSUPPORTED_PROFILE == vaSts);

    MFX_CHECK(!bUnsupported, MFX_ERR_UNSUPPORTED);
    MFX_CHECK(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    return MFX_ERR_NONE;
}

void DDI_VA::Query1NoCaps(const FeatureBlocks& blocks, TPushQ1 Push)
{
    Push(BLK_SetCallChains
        , [this](const mfxVideoParam&, mfxVideoParam& /*par*/, StorageRW& strg) -> mfxStatus
    {
        auto& ddiExec = Glob::DDI_Execute::GetOrConstruct(strg);

        MFX_CHECK(!ddiExec, MFX_ERR_NONE);

        ddiExec.Push([&](Glob::DDI_Execute::TRef::TExt, const DDIExecParam& ep)
        {
            return CallVaDefault(ep);
        });

        m_callVa = ddiExec;

        return MFX_ERR_NONE;
    });
}

void DDI_VA::Query1WithCaps(const FeatureBlocks& /*blocks*/, TPushQ1 Push)
{
    Push(BLK_QueryCaps
        , [this](const mfxVideoParam&, mfxVideoParam& /*par*/, StorageRW& strg) -> mfxStatus
    {
        MFX_CHECK(strg.Contains(Glob::GUID::Key), MFX_ERR_UNSUPPORTED);

        auto& core = Glob::VideoCore::Get(strg);
        const GUID& guid = Glob::GUID::Get(strg);

        auto  vaGuid         = MapGUID(strg, guid);
        auto  vap            = VAProfile(vaGuid.Profile);
        auto  vaep           = VAEntrypoint(vaGuid.Entrypoint);
        bool  bNeedNewDevice = vap != m_profile || vaep != m_entrypoint;

        m_callVa = Glob::DDI_Execute::Get(strg);

        if (bNeedNewDevice)
        {
            mfxStatus sts = CreateAuxilliaryDevice(core, vap, vaep);
            MFX_CHECK_STS(sts);
        }

        Glob::EncodeCaps::GetOrConstruct(strg) = m_caps;

        return MFX_ERR_NONE;
    });
}

void DDI_VA::InitExternal(const FeatureBlocks& /*blocks*/, TPushIE Push)
{
    Push(BLK_CreateDevice
        , [this](const mfxVideoParam& /*par*/, StorageRW& strg, StorageRW&) -> mfxStatus
    {
        auto& core           = Glob::VideoCore::Get(strg);
        auto& guid           = Glob::GUID::Get(strg);
        auto  vaGuid         = MapGUID(strg, guid);
        auto  vap            = VAProfile(vaGuid.Profile);
        auto  vaep           = VAEntrypoint(vaGuid.Entrypoint);
        bool  bNeedNewDevice = !m_vaDisplay || vap != m_profile || vaep != m_entrypoint;

        m_callVa = Glob::DDI_Execute::Get(strg);

        MFX_CHECK(bNeedNewDevice, MFX_ERR_NONE);

        auto sts = CreateAuxilliaryDevice(core, vap, vaep);

        return sts;
    });
}

VABufferID DDI_VA::CreateVABuffer(
    VABufferType type
    , void* pData
    , mfxU32 size
    , mfxU32 num)
{
    VABufferID id = VA_INVALID_ID;

    if (type != VAEncMiscParameterBufferType)
    {
        auto sts = CallVA(m_callVa, VAFID_CreateBuffer
            , m_vaDisplay
            , m_vaContextEncode
            , type
            , size
            , num
            , pData
            , &id);
        ThrowIf(!!sts, MFX_ERR_DEVICE_FAILED);
        
        return id;
    }

    auto sts = CallVA(m_callVa, VAFID_CreateBuffer
        , m_vaDisplay
        , m_vaContextEncode
        , VAEncMiscParameterBufferType
        , size
        , 1
        , nullptr
        , &id);
    ThrowIf(!!sts, MFX_ERR_DEVICE_FAILED);

    VAEncMiscParameterBuffer *pMP;

    sts = CallVA(m_callVa, VAFID_MapBuffer
        , m_vaDisplay
        , id
        , (void **)&pMP);
    ThrowIf(!!sts, MFX_ERR_DEVICE_FAILED);

    pMP->type = ((VAEncMiscParameterBuffer*)pData)->type;
    mfxU8* pDst = (mfxU8*)pMP->data;
    mfxU8* pSrc = (mfxU8*)pData;

    pSrc += sizeof(VAEncMiscParameterBuffer);
    size -= sizeof(VAEncMiscParameterBuffer);

    std::copy(pSrc, pSrc + size, pDst);

    sts = CallVA(m_callVa, VAFID_UnmapBuffer
        , m_vaDisplay
        , id);
    ThrowIf(!!sts, MFX_ERR_DEVICE_FAILED);

    return id;
}

mfxStatus DDI_VA::CreateVABuffers(
    const std::list<DDIExecParam>& par
    , std::vector<VABufferID>& pool)
{
    pool.resize(par.size(), VA_INVALID_ID);

    mfxStatus sts = Catch(
        MFX_ERR_NONE
        , std::transform<
        std::list<DDIExecParam>::const_iterator
        , std::vector<VABufferID>::iterator
        , std::function<VABufferID(const DDIExecParam&)>>
        , par.begin(), par.end(), pool.begin()
        , [&](const DDIExecParam& p)
    {
        return CreateVABuffer(
            VABufferType(p.Function)
            , p.In.pData
            , p.In.Size
            , std::max<mfxU32>(p.In.Num, 1u));
    });
    MFX_CHECK_STS(sts);

    return MFX_ERR_NONE;
}

mfxStatus DDI_VA::DestroyVABuffers(std::vector<VABufferID>& pool)
{
    std::remove(pool.begin(), pool.end(), VA_INVALID_ID);

    mfxStatus sts = Catch(
        MFX_ERR_NONE
        , std::for_each<std::vector<VABufferID>::iterator, std::function<void(VABufferID)>>
        , pool.begin()
        , pool.end()
        , [&](VABufferID id)
    {
        auto sts = CallVA(m_callVa, VAFID_DestroyBuffer, m_vaDisplay, id);
        ThrowIf(!!sts, MFX_ERR_DEVICE_FAILED);
    });
    MFX_CHECK_STS(sts);

    pool.clear();

    return MFX_ERR_NONE;
}

DDI_VA::~DDI_VA()
{
    //use only class internal data in destructor
    m_callVa = [&]( const DDIExecParam& ep) { return CallVaDefault(ep); };

    DestroyVABuffers(m_perSeqPar);
    DestroyVABuffers(m_perPicPar);

    if (m_vaContextEncode != VA_INVALID_ID)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaDestroyContext");
        vaDestroyContext(m_vaDisplay, m_vaContextEncode);
        m_vaContextEncode = VA_INVALID_ID;
    }

    if (m_vaConfig != VA_INVALID_ID)
    {
        vaDestroyConfig(m_vaDisplay, m_vaConfig);
        m_vaConfig = VA_INVALID_ID;
    }
}

void DDI_VA::InitAlloc(const FeatureBlocks& /*blocks*/, TPushIA Push)
{
    Push(BLK_CreateService
        , [this](StorageRW& strg, StorageRW& local) -> mfxStatus
    {
        auto& core = Glob::VideoCore::Get(strg);

        m_callVa = Glob::DDI_Execute::Get(strg);

        auto MidToVA = [&core](mfxMemId mid)
        {
            VASurfaceID *pSurface = nullptr;
            mfxStatus sts = core.GetFrameHDL(mid, (mfxHDL*)&pSurface);
            ThrowIf(!!sts, sts);
            return *pSurface;
        };
        auto& recResponse = Glob::AllocRec::Get(strg).Response();

        std::vector<VASurfaceID> rec(recResponse.NumFrameActual, VA_INVALID_SURFACE);

        mfxStatus sts = Catch(
            MFX_ERR_NONE
            , std::transform<mfxMemId*, VASurfaceID*, std::function<VASurfaceID(mfxMemId)>>
            , recResponse.mids
            , recResponse.mids + recResponse.NumFrameActual
            , rec.data()
            , MidToVA);
        MFX_CHECK_STS(sts);

        sts = CreateAccelerationService(Glob::VideoParam::Get(strg), rec.data(), (int)rec.size());
        MFX_CHECK_STS(sts);

        const auto& par = Glob::VideoParam::Get(strg);
        auto& fi = par.mfx.FrameInfo;
        auto pInfo = make_storable<mfxFrameAllocRequest>(mfxFrameAllocRequest{});

        // request linear buffer
        pInfo->Info.FourCC = MFX_FOURCC_P8;

        // context_id required for allocation video memory (tmp solution)
        pInfo->AllocId = m_vaContextEncode;
        pInfo->Info.Width = fi.Width * 2;
        pInfo->Info.Height = fi.Height;

        local.Insert(Tmp::BSAllocInfo::Key, std::move(pInfo));

        return MFX_ERR_NONE;
    });

    Push(BLK_Register
        , [this](StorageRW& strg, StorageRW& local) -> mfxStatus
    {
        auto& res = Glob::DDI_Resources::Get(strg);

        auto itBs = std::find_if(res.begin(), res.end()
            , [](decltype(*res.begin()) r)
        {
            return r.Function == MFX_FOURCC_P8;
        });
        MFX_CHECK(itBs != res.end(), MFX_ERR_UNDEFINED_BEHAVIOR);
        MFX_CHECK(itBs->Resource.Size == sizeof(VABufferID), MFX_ERR_UNDEFINED_BEHAVIOR);

        VABufferID* pBsBegin = (VABufferID*)itBs->Resource.pData;
        m_bs.assign(pBsBegin, pBsBegin + itBs->Resource.Num);

        mfxStatus sts = CreateVABuffers(
            Tmp::DDI_InitParam::Get(local)
            , m_perSeqPar);
        MFX_CHECK_STS(sts);

        return MFX_ERR_NONE;
    });
}

void DDI_VA::ResetState(const FeatureBlocks& /*blocks*/, TPushRS Push)
{
    Push(BLK_Reset
        , [this](StorageRW& strg, StorageRW& local) -> mfxStatus
    {
        mfxStatus sts;

        m_callVa = Glob::DDI_Execute::Get(strg);

        sts = DestroyVABuffers(m_perSeqPar);
        MFX_CHECK_STS(sts);

        sts = CreateVABuffers(
            Tmp::DDI_InitParam::Get(local)
            , m_perSeqPar);
        MFX_CHECK_STS(sts);

        return MFX_ERR_NONE;
    });
}

void DDI_VA::SubmitTask(const FeatureBlocks& /*blocks*/, TPushST Push)
{
    Push(BLK_SubmitTask
        , [this](StorageW& global, StorageW& s_task) -> mfxStatus
    {
        mfxStatus sts;
        auto& task = Task::Common::Get(s_task);

        m_callVa = Glob::DDI_Execute::Get(global);

        MFX_CHECK((task.SkipCMD & SKIPCMD_NeedDriverCall), MFX_ERR_NONE);

       auto& par = Glob::DDI_SubmitParam::Get(global);

       sts = DestroyVABuffers(m_perPicPar);
       MFX_CHECK_STS(sts);

       sts = CreateVABuffers(par, m_perPicPar);
       MFX_CHECK_STS(sts);

       MFX_LTRACE_2(MFX_TRACE_LEVEL_HOTSPOTS
           , "A|ENCODE|AVC|PACKET_START|", "%p|%d"
           , m_vaContextEncode, task.StatusReportId);

       {
           MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaBeginPicture");

           sts = CallVA(m_callVa, VAFID_BeginPicture
               , m_vaDisplay
               , m_vaContextEncode
               , *(VASurfaceID*)task.HDLRaw.first);
           MFX_CHECK_STS(sts);
       }

       {
           MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaRenderPicture");

           sts = CallVA(m_callVa, VAFID_RenderPicture
               , m_vaDisplay
               , m_vaContextEncode
               , m_perPicPar.data()
               , (int)m_perPicPar.size());
           MFX_CHECK_STS(sts);
       }
       
       {
           MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaRenderPicture");

           sts = CallVA(m_callVa, VAFID_RenderPicture
               , m_vaDisplay
               , m_vaContextEncode
               , m_perSeqPar.data()
               , (int)m_perSeqPar.size());
           MFX_CHECK_STS(sts);
       }

       {
           MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaEndPicture");

           sts = CallVA(m_callVa, VAFID_EndPicture
               , m_vaDisplay
               , m_vaContextEncode);
           MFX_CHECK_STS(sts);
       }

       MFX_LTRACE_2(MFX_TRACE_LEVEL_HOTSPOTS
           , "A|ENCODE|AVC|PACKET_END|", "%d|%d"
           , m_vaContextEncode, task.StatusReportId);

        return MFX_ERR_NONE;
    });
}

void DDI_VA::QueryTask(const FeatureBlocks& /*blocks*/, TPushQT Push)
{
    Push(BLK_QueryTask
        , [this](StorageW& global, StorageW& s_task) -> mfxStatus
    {
        auto& task = Task::Common::Get(s_task);

        m_callVa = Glob::DDI_Execute::Get(global);

        MFX_CHECK((task.SkipCMD & SKIPCMD_NeedDriverCall), MFX_ERR_NONE);

        auto& ddiFB = Glob::DDI_Feedback::Get(global);

        MFX_CHECK(!ddiFB.Get(task.StatusReportId), MFX_ERR_NONE);
        mfxStatus sts;

        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaSyncSurface");
            sts = CallVA(m_callVa, VAFID_SyncSurface, m_vaDisplay, *(VASurfaceID*)task.HDLRaw.first);
            MFX_CHECK_STS(sts);
        }

        VACodedBufferSegment* pCBS = nullptr;
        {
            sts = CallVA(m_callVa, VAFID_MapBuffer, m_vaDisplay, m_bs.at(task.BS.Idx), (void **)&pCBS);
            MFX_CHECK_STS(sts);
            MFX_CHECK(pCBS, MFX_ERR_DEVICE_FAILED);
        }

        MFX_CHECK(ddiFB.Out.Size >= sizeof(VACodedBufferSegment), MFX_ERR_UNDEFINED_BEHAVIOR);
        *(VACodedBufferSegment*)ddiFB.Out.pData = *pCBS;

        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
            sts = CallVA(m_callVa, VAFID_UnmapBuffer, m_vaDisplay, m_bs.at(task.BS.Idx));
        }
        MFX_CHECK_STS(sts);

        ddiFB.bNotReady = false;

        return ddiFB.Update(task.StatusReportId);
    });
}

mfxStatus DDI_VA::CreateAuxilliaryDevice(
    VideoCORE&  core
    , VAProfile profile
    , VAEntrypoint entrypoint)
{
    mfxStatus sts = core.GetHandle(MFX_HANDLE_VA_DISPLAY, (mfxHDL*)&m_vaDisplay);
    MFX_CHECK_STS(sts);

    m_caps = {};

    std::map<VAConfigAttribType, int> idx_map;
    VAConfigAttribType attr_types[] =
    {
        VAConfigAttribRTFormat
        , VAConfigAttribRateControl
        , VAConfigAttribEncQuantization
        , VAConfigAttribEncIntraRefresh
        , VAConfigAttribMaxPictureHeight
        , VAConfigAttribMaxPictureWidth
        , VAConfigAttribEncParallelRateControl
        , VAConfigAttribEncMaxRefFrames
        , VAConfigAttribEncSliceStructure
        , VAConfigAttribEncROI
        , VAConfigAttribEncTileSupport
    };
    std::vector<VAConfigAttrib> attrs;
    auto AV = [&](VAConfigAttribType t) { return attrs[idx_map[t]].value; };

    mfxI32 i = 0;
    std::for_each(std::begin(attr_types), std::end(attr_types)
        , [&](decltype(*std::begin(attr_types)) type)
    {
        attrs.push_back({ type, 0 });
        idx_map[type] = i;
        ++i;
    });

    sts = CallVA(
        m_callVa
        , VAFID_GetConfigAttributes
        , m_vaDisplay
        , profile
        , entrypoint
        , attrs.data()
        , (int)attrs.size());
    MFX_CHECK_STS(sts);

    m_profile = profile;
    m_entrypoint = entrypoint;

    m_caps.VCMBitRateControl = !!(AV(VAConfigAttribRateControl) & VA_RC_VCM); //Video conference mode
    m_caps.MBBRCSupport      = !!(AV(VAConfigAttribRateControl) & VA_RC_MB);
    m_caps.msdk.CBRSupport   = !!(AV(VAConfigAttribRateControl) & VA_RC_CBR);
    m_caps.msdk.VBRSupport   = !!(AV(VAConfigAttribRateControl) & VA_RC_VBR);
    m_caps.msdk.CQPSupport   = !!(AV(VAConfigAttribRateControl) & VA_RC_CQP);
    m_caps.msdk.ICQSupport   = !!(AV(VAConfigAttribRateControl) & VA_RC_ICQ);
#ifdef MFX_ENABLE_QVBR
    m_caps.QVBRBRCSupport    = !!(AV(VAConfigAttribRateControl) & VA_RC_QVBR);
#endif

    m_caps.RollingIntraRefresh = !!(AV(VAConfigAttribEncIntraRefresh) & (~VA_ATTRIB_NOT_SUPPORTED));

    m_caps.MaxEncodedBitDepth = std::max<mfxU32>(
          !!(AV(VAConfigAttribRTFormat) & VA_RT_FORMAT_YUV420_12) * 2
        , !!(AV(VAConfigAttribRTFormat) & VA_RT_FORMAT_YUV420_10));

    m_caps.Color420Only = !(AV(VAConfigAttribRTFormat) & (VA_RT_FORMAT_YUV422 | VA_RT_FORMAT_YUV444));
    m_caps.BitDepth8Only = !(AV(VAConfigAttribRTFormat) & (VA_RT_FORMAT_YUV420_10 | VA_RT_FORMAT_YUV420_12));
    m_caps.YUV422ReconSupport = !!(AV(VAConfigAttribRTFormat) & VA_RT_FORMAT_YUV422);
    m_caps.YUV444ReconSupport = !!(AV(VAConfigAttribRTFormat) & VA_RT_FORMAT_YUV444);

    m_caps.MaxPicWidth = AV(VAConfigAttribMaxPictureWidth) & ~VA_ATTRIB_NOT_SUPPORTED;
    SetDefault(m_caps.MaxPicWidth, 1920u);
    m_caps.MaxPicHeight = AV(VAConfigAttribMaxPictureHeight) & ~VA_ATTRIB_NOT_SUPPORTED;
    SetDefault(m_caps.MaxPicHeight, 1088u);

    if (AV(VAConfigAttribEncMaxRefFrames) != VA_ATTRIB_NOT_SUPPORTED)
    {
        m_caps.MaxNum_Reference0 = mfxU8(AV(VAConfigAttribEncMaxRefFrames));
        m_caps.MaxNum_Reference1 = mfxU8(AV(VAConfigAttribEncMaxRefFrames) >> 16);

        SetDefault(m_caps.MaxNum_Reference1, m_caps.MaxNum_Reference0);
        CheckMaxOrClip(m_caps.MaxNum_Reference1, m_caps.MaxNum_Reference0);
    }
    else
    {
        m_caps.MaxNum_Reference0 = 3;
        m_caps.MaxNum_Reference1 = 1;
    }

    if (AV(VAConfigAttribEncROI) != VA_ATTRIB_NOT_SUPPORTED) // VAConfigAttribEncROI
    {
        auto roi = *(VAConfigAttribValEncROI*)&attrs[idx_map[VAConfigAttribEncROI]].value;

        assert(roi.bits.num_roi_regions < 32);
        m_caps.MaxNumOfROI                  = roi.bits.num_roi_regions;
        m_caps.ROIBRCPriorityLevelSupport   = roi.bits.roi_rc_priority_support;
        m_caps.ROIDeltaQPSupport            = roi.bits.roi_rc_qp_delta_support;
    }

    m_caps.TileSupport = (AV(VAConfigAttribEncTileSupport) == 1);

    return MFX_ERR_NONE;
}

uint32_t ConvertRateControlMFX2VAAPI(mfxU16 rateControl, bool bSWBRC)
{
    if (bSWBRC)
        return VA_RC_CQP;

    static const std::map<mfxU16, uint32_t> RCMFX2VAAPI =
    {
        { mfxU16(MFX_RATECONTROL_CQP)   , uint32_t(VA_RC_CQP) },
        { mfxU16(MFX_RATECONTROL_LA_EXT), uint32_t(VA_RC_CQP) },
        { mfxU16(MFX_RATECONTROL_CBR)   , uint32_t(VA_RC_CBR | VA_RC_MB) },
        { mfxU16(MFX_RATECONTROL_VBR)   , uint32_t(VA_RC_VBR | VA_RC_MB) },
        { mfxU16(MFX_RATECONTROL_ICQ)   , uint32_t(VA_RC_ICQ) },
        { mfxU16(MFX_RATECONTROL_VCM)   , uint32_t(VA_RC_VCM) },
#ifdef MFX_ENABLE_QVBR
        { mfxU16(MFX_RATECONTROL_QVBR)  , uint32_t(VA_RC_QVBR) },
#endif
    };

    auto itRC = RCMFX2VAAPI.find(rateControl);
    if (itRC != RCMFX2VAAPI.end())
    {
        return itRC->second;
    }

    assert(!"Unsupported RateControl");
    return 0;
}

mfxStatus DDI_VA::CreateAccelerationService(
    const mfxVideoParam & par
    , VASurfaceID* pRec
    , int nRec)
{
    MFX_CHECK(m_vaDisplay, MFX_ERR_NOT_INITIALIZED);

    mfxI32 numEntrypoints = vaMaxNumEntrypoints(m_vaDisplay);
    MFX_CHECK(numEntrypoints, MFX_ERR_DEVICE_FAILED);

    std::vector<VAEntrypoint> ep_list(numEntrypoints);
    std::vector<VAProfile> profile_list(vaMaxNumProfiles(m_vaDisplay), VAProfileNone);
    mfxI32 num_profiles = 0;
    mfxStatus sts = MFX_ERR_NONE;

    sts = CallVA(
        m_callVa
        , VAFID_QueryConfigProfiles
        , m_vaDisplay
        , profile_list.data()
        , &num_profiles);
    MFX_CHECK_STS(sts);
    MFX_CHECK(std::find(profile_list.begin(), profile_list.end(), m_profile) != profile_list.end(), MFX_ERR_DEVICE_FAILED);

    sts = CallVA(
        m_callVa
        , VAFID_QueryConfigEntrypoints
        , m_vaDisplay
        , m_profile
        , ep_list.data()
        , &numEntrypoints);
    MFX_CHECK_STS(sts);
    MFX_CHECK(std::find(ep_list.begin(), ep_list.end(), m_entrypoint) != ep_list.end(), MFX_ERR_DEVICE_FAILED);

    // Configuration
    std::vector<VAConfigAttrib> attrib(2);

    attrib[0].type = VAConfigAttribRTFormat;
    attrib[1].type = VAConfigAttribRateControl;

    sts = CallVA(
        m_callVa
        , VAFID_GetConfigAttributes
        , m_vaDisplay
        , m_profile
        , m_entrypoint
        , attrib.data()
        , (mfxI32)attrib.size());
    MFX_CHECK_STS(sts);

    MFX_CHECK(
           (attrib[0].value & VA_RT_FORMAT_YUV420)
#if (MFX_VERSION >= 1027)
        || (attrib[0].value & VA_RT_FORMAT_YUV420_10)
#endif
        , MFX_ERR_DEVICE_FAILED);

    uint32_t vaRCType = ConvertRateControlMFX2VAAPI(par.mfx.RateControlMethod, Legacy::IsSWBRC(par, ExtBuffer::Get(par)));

    MFX_CHECK((attrib[1].value & vaRCType), MFX_ERR_DEVICE_FAILED);

    attrib[1].value = vaRCType;

    sts = CallVA(
        m_callVa
        , VAFID_CreateConfig
        , m_vaDisplay
        , m_profile
        , m_entrypoint
        , attrib.data()
        , (mfxI32)attrib.size()
        , &m_vaConfig);
    MFX_CHECK_STS(sts);

    // Encoder create
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaCreateContext");
        sts = CallVA(
            m_callVa
            , VAFID_CreateContext
            , m_vaDisplay
            , m_vaConfig
            , (int)par.mfx.FrameInfo.Width
            , (int)par.mfx.FrameInfo.Height
            , (int)VA_PROGRESSIVE
            , pRec
            , nRec
            , &m_vaContextEncode
        );
    }
    MFX_CHECK_STS(sts);

    return MFX_ERR_NONE;
}

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
