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

#include "mfx_common.h"
#include "hevcehw_base_va_lin.h"

#if defined(MFX_ENABLE_H265_VIDEO_ENCODE) && defined (MFX_VA_LINUX)

#include "hevcehw_base_legacy.h"

using namespace HEVCEHW;
using namespace HEVCEHW::Base;
using namespace HEVCEHW::Linux;
using namespace HEVCEHW::Linux::Base;

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

void DDI_VA::Query1NoCaps(const FeatureBlocks& /*blocks*/, TPushQ1 Push)
{
    Push(BLK_SetCallChains
        , [this](const mfxVideoParam&, mfxVideoParam& /*par*/, StorageRW& strg) -> mfxStatus
    {
        auto& ddiExec = Glob::DDI_Execute::GetOrConstruct(strg);

        MFX_CHECK(!ddiExec, MFX_ERR_NONE);

        ddiExec.Push([&](Glob::DDI_Execute::TRef::TExt, const DDIExecParam& ep)
        {
            return Execute(ep);
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
        bool  bNeedNewDevice = !IsValid() || vap != m_profile || vaep != m_entrypoint;

        m_callVa = Glob::DDI_Execute::Get(strg);

        if (bNeedNewDevice)
        {
            mfxStatus sts = Create(core, vap, vaep);
            MFX_CHECK_STS(sts);

            sts = QueryCaps();
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
        bool  bNeedNewDevice = !IsValid() || vap != m_profile || vaep != m_entrypoint;

        m_callVa = Glob::DDI_Execute::Get(strg);

        MFX_CHECK(bNeedNewDevice, MFX_ERR_NONE);

        mfxStatus sts = Create(core, vap, vaep);
        MFX_CHECK_STS(sts);

        sts = QueryCaps();
        MFX_CHECK_STS(sts);

        return sts;
    });
}

mfxStatus DDI_VA::CreateVABuffers(
    const std::list<DDIExecParam>& par
    , std::vector<VABufferID>& pool)
{
    pool.resize(par.size(), VA_INVALID_ID);

    std::transform(par.begin(), par.end(), pool.begin()
        , [this](const DDIExecParam& p) { return CreateVABuffer(p); });

    bool bFailed = pool.end() != std::find(pool.begin(), pool.end(), VA_INVALID_ID);
    MFX_CHECK(!bFailed, MFX_ERR_DEVICE_FAILED);

    return MFX_ERR_NONE;
}

mfxStatus DDI_VA::DestroyVABuffers(std::vector<VABufferID>& pool)
{
    bool bFailed = std::any_of(pool.begin(), pool.end()
        , [this](VABufferID id) { return !!DestroyVABuffer(id); });

    pool.clear();

    MFX_CHECK(!bFailed, MFX_ERR_DEVICE_FAILED);

    return MFX_ERR_NONE;
}


void DDI_VA::InitAlloc(const FeatureBlocks& /*blocks*/, TPushIA Push)
{
    Push(BLK_CreateService
        , [this](StorageRW& strg, StorageRW& local) -> mfxStatus
    {
        const auto&             par     = Glob::VideoParam::Get(strg);
        const mfxExtHEVCParam&  hevcPar = ExtBuffer::Get(par);
        mfxStatus sts;

        m_callVa = Glob::DDI_Execute::Get(strg);

        //Set max priority
        Glob::PriorityPar::GetOrConstruct(strg) = m_priorityPar;

        std::vector<VAConfigAttrib> attrib(2);

        attrib[0].type = VAConfigAttribRTFormat;
        attrib[1].type = VAConfigAttribRateControl;

        sts = MfxEncodeHW::DeviceVAAPI::QueryCaps(attrib.data(), attrib.size() * sizeof(VAConfigAttrib));
        MFX_CHECK_STS(sts);

        MFX_CHECK(attrib[0].value & (VA_RT_FORMAT_YUV420|VA_RT_FORMAT_YUV420_10), MFX_ERR_DEVICE_FAILED);

        uint32_t vaRCType = ConvertRateControlMFX2VAAPI(par.mfx.RateControlMethod, Legacy::IsSWBRC(par, ExtBuffer::Get(par)));

        MFX_CHECK((attrib[1].value & vaRCType), MFX_ERR_DEVICE_FAILED);

        attrib[1].value = vaRCType;

        sts = MfxEncodeHW::DeviceVAAPI::Init(
            hevcPar.PicWidthInLumaSamples
            , hevcPar.PicHeightInLumaSamples
            , VA_PROGRESSIVE
            , Glob::AllocRec::Get(strg).GetResponse()
            , attrib.data()
            , int(attrib.size()));
        MFX_CHECK_STS(sts);

        auto& info = Tmp::BSAllocInfo::GetOrConstruct(local);

        // request linear buffer
        info.Info.FourCC = MFX_FOURCC_P8;

        // context_id required for allocation video memory (tmp solution)
        info.AllocId     = m_vaContextEncode;
        info.Info.Width  = hevcPar.PicWidthInLumaSamples * 2;
        info.Info.Height = hevcPar.PicHeightInLumaSamples;

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

        sts = DestroyVABuffers(m_perPicPar);
        MFX_CHECK_STS(sts);

        sts = CreateVABuffers(Glob::DDI_SubmitParam::Get(global), m_perPicPar);
        MFX_CHECK_STS(sts);

        MFX_LTRACE_2(MFX_TRACE_LEVEL_HOTSPOTS
            , "A|ENCODE|AVC|PACKET_START|", "%p|%d"
            , m_vaContextEncode, task.StatusReportId);

        sts = BeginPicture(task.HDLRaw.first);
        MFX_CHECK_STS(sts);

        sts = RenderPicture(m_perPicPar.data(), (int)m_perPicPar.size());
        MFX_CHECK_STS(sts);

        sts = RenderPicture(m_perSeqPar.data(), (int)m_perSeqPar.size());
        MFX_CHECK_STS(sts);

        sts = EndPicture();
        MFX_CHECK_STS(sts);

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

        return QueryStatus(Glob::DDI_Feedback::Get(global), task.StatusReportId);
    });
}

mfxStatus DDI_VA::QueryCaps()
{
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
        , VAConfigAttribEncDirtyRect
        , VAConfigAttribMaxFrameSize
        , VAConfigAttribContextPriority
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

    auto sts = MfxEncodeHW::DeviceVAAPI::QueryCaps(attrs.data(), attrs.size() * sizeof(VAConfigAttrib));
    MFX_CHECK_STS(sts);

    m_caps.VCMBitRateControl = !!(AV(VAConfigAttribRateControl) & VA_RC_VCM); //Video conference mode
    m_caps.MBBRCSupport      = !!(AV(VAConfigAttribRateControl) & VA_RC_MB);
    m_caps.msdk.CBRSupport   = !!(AV(VAConfigAttribRateControl) & VA_RC_CBR);
    m_caps.msdk.VBRSupport   = !!(AV(VAConfigAttribRateControl) & VA_RC_VBR);
    m_caps.msdk.CQPSupport   = !!(AV(VAConfigAttribRateControl) & VA_RC_CQP);
    m_caps.msdk.ICQSupport   = !!(AV(VAConfigAttribRateControl) & VA_RC_ICQ);
#ifdef MFX_ENABLE_QVBR
    m_caps.QVBRBRCSupport    = !!(AV(VAConfigAttribRateControl) & VA_RC_QVBR);
#endif
#if VA_CHECK_VERSION(1, 10, 0)
    m_caps.TCBRCSupport        = !!(AV(VAConfigAttribRateControl) & VA_RC_TCBRC);
#endif
    m_caps.RollingIntraRefresh = !!(AV(VAConfigAttribEncIntraRefresh) & (~VA_ATTRIB_NOT_SUPPORTED));

    m_caps.MaxEncodedBitDepth = std::max<mfxU32>(
          !!(AV(VAConfigAttribRTFormat) & VA_RT_FORMAT_YUV420_12) * 2
        , !!(AV(VAConfigAttribRTFormat) & VA_RT_FORMAT_YUV420_10));

    m_caps.Color420Only = !(AV(VAConfigAttribRTFormat) & (VA_RT_FORMAT_YUV422 | VA_RT_FORMAT_YUV444));
    m_caps.BitDepth8Only = !(AV(VAConfigAttribRTFormat) & (VA_RT_FORMAT_YUV420_10 | VA_RT_FORMAT_YUV420_12));
    m_caps.YUV422ReconSupport = !!(AV(VAConfigAttribRTFormat) & VA_RT_FORMAT_YUV422);
    m_caps.YUV444ReconSupport = !!(AV(VAConfigAttribRTFormat) & VA_RT_FORMAT_YUV444);

    MFX_CHECK(AV(VAConfigAttribMaxPictureWidth) != VA_ATTRIB_NOT_SUPPORTED, MFX_ERR_UNSUPPORTED);
    MFX_CHECK(AV(VAConfigAttribMaxPictureHeight) != VA_ATTRIB_NOT_SUPPORTED, MFX_ERR_UNSUPPORTED);
    MFX_CHECK_COND(AV(VAConfigAttribMaxPictureWidth) && AV(VAConfigAttribMaxPictureHeight));
    m_caps.MaxPicWidth = AV(VAConfigAttribMaxPictureWidth);
    m_caps.MaxPicHeight = AV(VAConfigAttribMaxPictureHeight);

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

    if (AV(VAConfigAttribEncDirtyRect) != VA_ATTRIB_NOT_SUPPORTED &&
        AV(VAConfigAttribEncDirtyRect) != 0)
    {
        m_caps.DirtyRectSupport = 1;
        m_caps.MaxNumOfDirtyRect = mfxU16(AV(VAConfigAttribEncDirtyRect));
    }


    m_caps.TileSupport = (AV(VAConfigAttribEncTileSupport) == 1);
    m_caps.UserMaxFrameSizeSupport = !!(AV(VAConfigAttribMaxFrameSize));

    if (AV(VAConfigAttribContextPriority) != VA_ATTRIB_NOT_SUPPORTED)
    {
        m_priorityPar.m_MaxContextPriority = AV(VAConfigAttribContextPriority);
    }
    else
    {
        m_priorityPar.m_MaxContextPriority = 0;
    }

    if (AV(VAConfigAttribEncSliceStructure)!=VA_ATTRIB_NOT_SUPPORTED)
    {
        // Attribute for VAConfigAttribEncSliceStructure includes both:
        // (1) information about supported slice structure and
        // (2) indication of support for max slice size feature
        uint32_t sliceCapabilities = AV(VAConfigAttribEncSliceStructure);
        m_caps.SliceStructure = ConvertSliceStructureVAAPIToMFX(sliceCapabilities);
        // It means that GPU may further split the slice region that
        // slice control data specifies into finer slice segments based on slice size upper limit (MaxSliceSize)
        m_caps.SliceByteSizeCtrl = (sliceCapabilities & VA_ENC_SLICE_STRUCTURE_MAX_SLICE_SIZE)!=0;
    }

    return MFX_ERR_NONE;
}

uint32_t DDI_VA::ConvertRateControlMFX2VAAPI(mfxU16 rateControl, bool bSWBRC)
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

uint32_t DDI_VA::ConvertSliceStructureVAAPIToMFX(uint32_t structure)
{
    if (structure & VA_ENC_SLICE_STRUCTURE_ARBITRARY_MACROBLOCKS)
        return ARBITRARY_MB_SLICE;
    if (structure & VA_ENC_SLICE_STRUCTURE_ARBITRARY_ROWS)
        return ARBITRARY_ROW_SLICE;
    if (structure & VA_ENC_SLICE_STRUCTURE_EQUAL_ROWS ||
        structure & VA_ENC_SLICE_STRUCTURE_EQUAL_MULTI_ROWS)
        return ROWSLICE;
    if (structure & VA_ENC_SLICE_STRUCTURE_POWER_OF_TWO_ROWS)
        return POW2ROW;
    return ONESLICE;
}

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
