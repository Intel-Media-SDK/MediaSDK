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

#pragma once

#include "mfx_common.h"
#include "hevcehw_base.h"

#if defined(MFX_ENABLE_H265_VIDEO_ENCODE) && defined (MFX_VA_LINUX)

#include "hevcehw_ddi.h"
#include "hevcehw_g9_data.h"
#include "hevcehw_g9_iddi.h"
#include "va/va.h"

namespace HEVCEHW
{
namespace Linux
{
namespace Gen9
{
using namespace HEVCEHW::Gen9;

class DDI_VA
    : public virtual FeatureBase
    , protected IDDI
{
public:
    DDI_VA(mfxU32 FeatureId)
        : FeatureBase(FeatureId)
        , IDDI(FeatureId)
    {
        SetTraceName("G9_DDI_VA");
    }

    ~DDI_VA();

    static VAGUID MapGUID(StorageR& strg, const GUID& guid);

    enum VAFID
    {
        VAFID_CreateConfig = 1
        , VAFID_DestroyConfig
        , VAFID_CreateContext
        , VAFID_DestroyContext
        , VAFID_GetConfigAttributes
        , VAFID_QueryConfigEntrypoints
        , VAFID_QueryConfigProfiles
        , VAFID_CreateBuffer
        , VAFID_MapBuffer
        , VAFID_UnmapBuffer
        , VAFID_DestroyBuffer
        , VAFID_BeginPicture
        , VAFID_RenderPicture
        , VAFID_EndPicture
        , VAFID_SyncSurface
    };

    using TDdiExec = std::function<VAStatus(const DDIExecParam&)>;
    using TDdiExecMfx = std::function<mfxStatus(const DDIExecParam&)>;

    template <typename TFn>
    static TDdiExec CallDefault(TFn fn)
    {
        using TArgs = decltype(TupleArgs(fn));
        return [fn](const DDIExecParam& par) { return CallWithTupleArgs(fn, Deref<TArgs>(par.In)); };
    }

    template <typename... TArgs>
    static mfxStatus CallVA(TDdiExecMfx& fn, VAFID id, TArgs... args)
    {
        DDIExecParam xPar;
        auto         tpl = std::make_tuple(args...);
        xPar.Function = mfxU32(id);
        xPar.In.pData = &tpl;
        xPar.In.Size  = sizeof(tpl);
        return fn(xPar);
    }

protected:
    virtual void Query1NoCaps(const FeatureBlocks& blocks, TPushQ1 Push) override;
    virtual void Query1WithCaps(const FeatureBlocks& blocks, TPushQ1 Push) override;
    virtual void InitExternal(const FeatureBlocks& blocks, TPushIE Push) override;
    virtual void InitAlloc(const FeatureBlocks& blocks, TPushIA Push) override;
    virtual void SubmitTask(const FeatureBlocks& blocks, TPushST Push) override;
    virtual void QueryTask(const FeatureBlocks& blocks, TPushQT Push) override;
    virtual void ResetState(const FeatureBlocks& blocks, TPushRS Push) override;

    mfxStatus CreateAuxilliaryDevice(
        VideoCORE&  core
        , VAProfile profile
        , VAEntrypoint entrypoint);

    mfxStatus CreateAccelerationService(
        const mfxVideoParam & par
        , VASurfaceID* pRec
        , int nRec);

    VABufferID CreateVABuffer(
        VABufferType type
        , void* pData
        , mfxU32 size
        , mfxU32 num);

    mfxStatus CreateVABuffers(
        const std::list<DDIExecParam>& par
        , std::vector<VABufferID>& pool);

    mfxStatus DestroyVABuffers(std::vector<VABufferID>& pool);

    mfxStatus CallVaDefault(const DDIExecParam& ep);

    VideoCORE*                  m_pCore            = nullptr;
    VAProfile                   m_profile          = VAProfileNone;
    VAEntrypoint                m_entrypoint       = VAEntrypointEncSlice;
    VADisplay                   m_vaDisplay        = nullptr;
    VAContextID                 m_vaContextEncode  = VA_INVALID_ID;
    VAConfigID                  m_vaConfig         = VA_INVALID_ID;
    EncodeCapsHevc              m_caps;
    std::vector<VABufferID>     m_perSeqPar;
    std::vector<VABufferID>     m_perPicPar;
    std::vector<VABufferID>     m_bs;
    TDdiExecMfx                 m_callVa;
    std::map<VAFID, TDdiExec>   m_ddiExec =
    {
        {VAFID_CreateConfig,              CallDefault(&vaCreateConfig)}
        , {VAFID_DestroyConfig,           CallDefault(&vaDestroyConfig)}
        , {VAFID_CreateContext,           CallDefault(&vaCreateContext)}
        , {VAFID_DestroyContext,          CallDefault(&vaDestroyContext)}
        , {VAFID_GetConfigAttributes,     CallDefault(&vaGetConfigAttributes)}
        , {VAFID_QueryConfigEntrypoints,  CallDefault(&vaQueryConfigEntrypoints)}
        , {VAFID_QueryConfigProfiles,     CallDefault(&vaQueryConfigProfiles)}
        , {VAFID_GetConfigAttributes,     CallDefault(&vaGetConfigAttributes)}
        , {VAFID_CreateBuffer,            CallDefault(&vaCreateBuffer)}
        , {VAFID_MapBuffer,               CallDefault(&vaMapBuffer)}
        , {VAFID_UnmapBuffer,             CallDefault(&vaUnmapBuffer)}
        , {VAFID_DestroyBuffer,           CallDefault(&vaDestroyBuffer)}
        , {VAFID_BeginPicture,            CallDefault(&vaBeginPicture)}
        , {VAFID_RenderPicture,           CallDefault(&vaRenderPicture)}
        , {VAFID_EndPicture,              CallDefault(&vaEndPicture)}
        , {VAFID_SyncSurface,             CallDefault(&vaSyncSurface)}
    };
};

} //Gen9
} //Linux
} //namespace HEVCEHW

#endif
