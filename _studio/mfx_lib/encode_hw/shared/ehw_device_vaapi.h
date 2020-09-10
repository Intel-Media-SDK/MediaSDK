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
#include <tuple>
#include <functional>
#include <set>
#include <map>

namespace MfxEncodeHW
{

class DeviceVAAPI
    : public Device
{
public:
    ~DeviceVAAPI()
    {
        Destroy();
    }

    union TVAGUID
    {
        GUID guid;
        struct VA
        {
            VAProfile    profile;
            VAEntrypoint entrypoint;
        } va;
    };

    mfxStatus Create(VideoCORE& core, VAProfile profile, VAEntrypoint entrypoint)
    {
        TVAGUID guid = {};
        guid.va.profile = profile;
        guid.va.entrypoint = entrypoint;
        return Create(core, guid.guid, 0, 0, false);
    }
    mfxStatus Init(
        int width
        , int height
        , int flag
        , mfxFrameAllocResponse rec
        , VAConfigAttrib* pAttr
        , int nAttr
        , std::list<DDIExecParam>* pPar = nullptr);

    virtual mfxStatus Create(
        VideoCORE&    core
        , GUID        guid
        , mfxU32      width
        , mfxU32      height
        , bool        isTemporal) override;

    virtual bool      IsValid() const override { return m_pCore && m_vaDisplay; }
    virtual mfxStatus QueryCaps(void* pCaps /*VAConfigAttrib*/, mfxU32 size) override;
    virtual mfxStatus QueryCompBufferInfo(mfxU32, mfxFrameInfo&) override;
    virtual mfxStatus Init(const std::list<DDIExecParam>*) override;
    virtual mfxStatus Execute(const DDIExecParam&) override;
    virtual mfxStatus QueryStatus(DDIFeedback& fb, mfxU32 id) override;
    virtual mfxStatus BeginPicture(mfxHDL) override;
    virtual mfxStatus EndPicture() override;
    virtual mfxU32    GetLastErr() const override { return mfxU32(m_lastErr); }

    VABufferID  CreateVABuffer(const DDIExecParam&);
    mfxStatus   DestroyVABuffer(VABufferID);
    VASurfaceID GetVASurface(mfxMemId);
    void Destroy();

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
#if VA_CHECK_VERSION(1,9,0)
        , VAFID_SyncBuffer
#endif
    };

    using TDdiExec      = std::function<VAStatus(const DDIExecParam&)>;
    using TDdiExecMfx   = std::function<mfxStatus(const DDIExecParam&)>;

    template <typename TRes, typename... TArgs>
    static inline std::tuple<TArgs...>&
        GetArgs(const DDIExecParam::Param& par, TRes(*)(TArgs...))
    {
        return par.Cast<std::tuple<TArgs...>>();
    }

    template <typename TFn>
    static TDdiExec CallDefault(TFn fn)
    {
        using TArgs = typename mfx::TupleArgs<TFn>::type;
        return [fn](const DDIExecParam& par)
        {
            return mfx::apply(fn, par.In.Cast<TArgs>());
        };
    }

    template <typename... TArgs>
    static mfxStatus CallVA(TDdiExecMfx& fn, VAFID id, TArgs... args)
    {
        auto tpl = std::make_tuple(args...);
        DDIExecParam xPar;
        xPar.Function = mfxU32(id);
        xPar.In.pData = &tpl;
        xPar.In.Size  = sizeof(tpl);
        return fn(xPar);
    }

    template <typename... TArgs>
    mfxStatus Execute(VAFID id, TArgs... args)
    {
        return CallVA(m_callVa, id, args...);
    }
    mfxStatus RenderPicture(VABufferID* pBuf, int num)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaRenderPicture");
        return Execute(VAFID_RenderPicture, m_vaDisplay, m_vaContextEncode, pBuf, num);
    }
    mfxStatus SyncSurface(VASurfaceID target)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaSyncSurface");
        return Execute(VAFID_SyncSurface, m_vaDisplay, target);
    }
#if VA_CHECK_VERSION(1,9,0)
    mfxStatus SyncBuffer(VABufferID id, uint64_t timeout_ns)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaSyncBuffer");
        return Execute(VAFID_SyncBuffer, m_vaDisplay, id, timeout_ns);
    }
#endif
    mfxStatus MapBuffer(VABufferID id, void** pBuf)
    {
        return Execute(VAFID_MapBuffer, m_vaDisplay, id, pBuf);
    }
    mfxStatus UnmapBuffer(VABufferID id)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
        return Execute(VAFID_UnmapBuffer, m_vaDisplay, id);
    }

protected:
    TDdiExecMfx               m_callVa;
    VAStatus                  m_lastErr         = VA_STATUS_SUCCESS;
    VideoCORE*                m_pCore           = nullptr;
    VAProfile                 m_profile         = VAProfileNone;
    VAEntrypoint              m_entrypoint      = VAEntrypointEncSlice;
    VADisplay                 m_vaDisplay       = nullptr;
    VAContextID               m_vaContextEncode = VA_INVALID_ID;
    VAConfigID                m_vaConfig        = VA_INVALID_ID;
    std::map<VAFID, TDdiExec> m_ddiExec         =
    {
        {VAFID_CreateConfig,              CallDefault(&vaCreateConfig)}
        , {VAFID_DestroyConfig,           CallDefault(&vaDestroyConfig)}
        , {VAFID_CreateContext,           CallDefault(&vaCreateContext)}
        , {VAFID_DestroyContext,          CallDefault(&vaDestroyContext)}
        , {VAFID_GetConfigAttributes,     CallDefault(&vaGetConfigAttributes)}
        , {VAFID_QueryConfigEntrypoints,  CallDefault(&vaQueryConfigEntrypoints)}
        , {VAFID_QueryConfigProfiles,     CallDefault(&vaQueryConfigProfiles)}
        , {VAFID_CreateBuffer,            CallDefault(&vaCreateBuffer)}
        , {VAFID_MapBuffer,               CallDefault(&vaMapBuffer)}
        , {VAFID_UnmapBuffer,             CallDefault(&vaUnmapBuffer)}
        , {VAFID_DestroyBuffer,           CallDefault(&vaDestroyBuffer)}
        , {VAFID_BeginPicture,            CallDefault(&vaBeginPicture)}
        , {VAFID_RenderPicture,           CallDefault(&vaRenderPicture)}
        , {VAFID_EndPicture,              CallDefault(&vaEndPicture)}
        , {VAFID_SyncSurface,             CallDefault(&vaSyncSurface)}
#if VA_CHECK_VERSION(1,9,0)
        , {VAFID_SyncBuffer,              CallDefault(&vaSyncBuffer)}
#endif
    };
    std::set<VABufferID> m_vaBuffers;
};

} //namespace MfxEncodeHW
#endif //defined(MFX_VA_LINUX)
