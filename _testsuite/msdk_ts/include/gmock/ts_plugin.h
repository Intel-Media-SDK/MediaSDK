// Copyright (c) 2018 Intel Corporation
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

#include "ts_trace.h"
#include <stdio.h>
#include <string>
#include <cstring>
#include <map>
#include <list>
#include <utility>
#include <vector>
#include <algorithm>
#include "mfxplugin++.h"
#include <unordered_set>

typedef enum func_list {
    // common
    Init = 1,
    QueryIOSurf,
    Query,
    Reset,
    GetVideoParam,
    Close,
    PluginInit,
    PluginClose,
    GetPluginParam,
    Execute,
    FreeResources,
    Release,
    SetAuxParams,

    // VPP
    VPPFrameSubmit,
    VPPFrameSubmitEx,

    //Encoder
    EncodeFrameSubmit,

    //Enc
    EncFrameSubmit,

    //Decoder
    DecodeHeader,
    GetPayload,
    DecodeFrameSubmit
} APIfuncs;

enum MsdkPluginType {
    MSDK_PLUGIN_TYPE_NONE = 0,
    MSDK_PLUGIN_TYPE_FEI,
};

class tsPlugin {
private:
    std::map<std::tuple<mfxU32, mfxU32, MsdkPluginType>, mfxPluginUID*> m_puid;
    std::list<mfxPluginUID> m_uid;
    std::list<mfxPluginUID> m_list;

public:
    tsPlugin() {
    }
    ~tsPlugin() {
    }

    inline mfxPluginUID* UID(mfxU32 type, mfxU32 id = 0, MsdkPluginType extType = MSDK_PLUGIN_TYPE_NONE) {
        return m_puid[std::make_tuple(type, id, extType)];
    }

    inline void Reg(mfxU32 type, mfxU32 id, const mfxPluginUID uid, MsdkPluginType extType = MSDK_PLUGIN_TYPE_NONE) {
        m_uid.push_back(uid);
        m_puid[std::make_tuple(type, id, extType)] = &m_uid.back();
    }

    inline std::list<mfxPluginUID> GetUserPlugins() {
        return m_list;
    }

    mfxU32 GetType(const mfxPluginUID& uid);
    void Init(std::string env, std::string platform);
};

void func_name(std::unordered_multiset<mfxU32>::const_iterator it);
bool check_calls(std::vector<mfxU32> real_calls, std::vector<mfxU32> expected_calls);

class FakeVPP: public MFXVPPPlugin
{
public:
    std::vector<mfxU32> m_calls;

    FakeVPP() {
    }
    virtual ~FakeVPP() {
    }

    //common func
    mfxStatus Init(mfxVideoParam*) {
        m_calls.push_back(APIfuncs::Init);
        return MFX_ERR_NONE;
    }

    mfxStatus QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *in,
            mfxFrameAllocRequest *out) {
        m_calls.push_back(APIfuncs::QueryIOSurf);
        return MFX_ERR_NONE;
    }
    mfxStatus Query(mfxVideoParam *in, mfxVideoParam *out) {
        m_calls.push_back(APIfuncs::Query);
        return MFX_ERR_NONE;
    }
    mfxStatus Reset(mfxVideoParam *par) {
        m_calls.push_back(APIfuncs::Reset);
        return MFX_ERR_NONE;
    }
    mfxStatus Close() {
        m_calls.push_back(APIfuncs::Close);
        return MFX_ERR_NONE;
    }
    mfxStatus PluginInit(mfxCoreInterface *core) {
        m_calls.push_back(APIfuncs::PluginInit);
        return MFX_ERR_NONE;
    }
    mfxStatus PluginClose() {
        m_calls.push_back(APIfuncs::PluginClose);
        return MFX_ERR_NONE;
    }
    mfxStatus GetVideoParam(mfxVideoParam *par) {
        m_calls.push_back(APIfuncs::GetVideoParam);
        return MFX_ERR_NONE;
    }
    mfxStatus GetPluginParam(mfxPluginParam *par) {
        m_calls.push_back(APIfuncs::GetPluginParam);
        return MFX_ERR_NONE;
    }
    mfxStatus Execute(mfxThreadTask task, mfxU32 uid_p, mfxU32 uid_a) {
        m_calls.push_back(APIfuncs::Execute);
        return MFX_ERR_NONE;
    }
    mfxStatus FreeResources(mfxThreadTask task, mfxStatus sts) {
        m_calls.push_back(APIfuncs::FreeResources);
        return MFX_ERR_NONE;
    }
    mfxStatus SetAuxParams(void*, int) {
        m_calls.push_back(APIfuncs::SetAuxParams);
        return MFX_ERR_NONE;
    }
    void Release() {
        m_calls.push_back(APIfuncs::Release);
        return;
    }

    //VPP
    mfxStatus VPPFrameSubmit(mfxFrameSurface1 *surface_in,
            mfxFrameSurface1 *surface_out, mfxExtVppAuxData *aux,
            mfxThreadTask *task) {
        m_calls.push_back(APIfuncs::VPPFrameSubmit);
        return MFX_ERR_NONE;
    }
    mfxStatus VPPFrameSubmitEx(mfxFrameSurface1 *in,
            mfxFrameSurface1 *surface_work, mfxFrameSurface1 **surface_out,
            mfxThreadTask *task) {
        m_calls.push_back(APIfuncs::VPPFrameSubmitEx);
        return MFX_ERR_NONE;
    }
};

class FakeEncoder: public MFXEncoderPlugin {
public:
    std::vector<mfxU32> m_calls;

    FakeEncoder() {
    }
    virtual ~FakeEncoder() {
    }

    //common func
    mfxStatus Init(mfxVideoParam*) {
        m_calls.push_back(APIfuncs::Init);
        return MFX_ERR_NONE;
    }
    mfxStatus QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *in,
            mfxFrameAllocRequest *out) {
        m_calls.push_back(APIfuncs::QueryIOSurf);
        return MFX_ERR_NONE;
    }
    mfxStatus Query(mfxVideoParam *in, mfxVideoParam *out) {
        m_calls.push_back(APIfuncs::Query);
        return MFX_ERR_NONE;
    }
    mfxStatus Reset(mfxVideoParam *par) {
        m_calls.push_back(APIfuncs::Reset);
        return MFX_ERR_NONE;
    }
    mfxStatus Close() {
        m_calls.push_back(APIfuncs::Close);
        return MFX_ERR_NONE;
    }
    mfxStatus PluginInit(mfxCoreInterface *core) {
        m_calls.push_back(APIfuncs::PluginInit);
        return MFX_ERR_NONE;
    }
    mfxStatus PluginClose() {
        m_calls.push_back(APIfuncs::PluginClose);
        return MFX_ERR_NONE;
    }
    mfxStatus GetVideoParam(mfxVideoParam *par) {
        m_calls.push_back(APIfuncs::GetVideoParam);
        return MFX_ERR_NONE;
    }
    mfxStatus GetPluginParam(mfxPluginParam *par) {
        m_calls.push_back(APIfuncs::GetPluginParam);
        return MFX_ERR_NONE;
    }
    mfxStatus Execute(mfxThreadTask task, mfxU32 uid_p, mfxU32 uid_a) {
        m_calls.push_back(APIfuncs::Execute);
        return MFX_ERR_NONE;
    }
    mfxStatus FreeResources(mfxThreadTask task, mfxStatus sts) {
        m_calls.push_back(APIfuncs::FreeResources);
        return MFX_ERR_NONE;
    }
    mfxStatus SetAuxParams(void*, int) {
        m_calls.push_back(APIfuncs::SetAuxParams);
        return MFX_ERR_NONE;
    }
    void Release() {
        m_calls.push_back(APIfuncs::Release);
        return;
    }

    //Encoder
    mfxStatus EncodeFrameSubmit(mfxEncodeCtrl *ctrl, mfxFrameSurface1 *surface,
            mfxBitstream *bs, mfxThreadTask *task) {
        m_calls.push_back(APIfuncs::EncodeFrameSubmit);
        return MFX_ERR_NONE;
    }
};

class FakeEnc: public MFXEncPlugin {
public:
    std::vector<mfxU32> m_calls;

    FakeEnc() {
    }
    virtual ~FakeEnc() {
    }

    //common func
    mfxStatus Init(mfxVideoParam*) {
        m_calls.push_back(APIfuncs::Init);
        return MFX_ERR_NONE;
    }
    mfxStatus QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *in,
            mfxFrameAllocRequest *out) {
        m_calls.push_back(APIfuncs::QueryIOSurf);
        return MFX_ERR_NONE;
    }
    mfxStatus Query(mfxVideoParam *in, mfxVideoParam *out) {
        m_calls.push_back(APIfuncs::Query);
        return MFX_ERR_NONE;
    }
    mfxStatus Reset(mfxVideoParam *par) {
        m_calls.push_back(APIfuncs::Reset);
        return MFX_ERR_NONE;
    }
    mfxStatus Close() {
        m_calls.push_back(APIfuncs::Close);
        return MFX_ERR_NONE;
    }
    mfxStatus PluginInit(mfxCoreInterface *core) {
        m_calls.push_back(APIfuncs::PluginInit);
        return MFX_ERR_NONE;
    }
    mfxStatus PluginClose() {
        m_calls.push_back(APIfuncs::PluginClose);
        return MFX_ERR_NONE;
    }
    mfxStatus GetVideoParam(mfxVideoParam *par) {
        m_calls.push_back(APIfuncs::GetVideoParam);
        return MFX_ERR_NONE;
    }
    mfxStatus GetPluginParam(mfxPluginParam *par) {
        m_calls.push_back(APIfuncs::GetPluginParam);
        return MFX_ERR_NONE;
    }
    mfxStatus Execute(mfxThreadTask task, mfxU32 uid_p, mfxU32 uid_a) {
        m_calls.push_back(APIfuncs::Execute);
        return MFX_ERR_NONE;
    }
    mfxStatus FreeResources(mfxThreadTask task, mfxStatus sts) {
        m_calls.push_back(APIfuncs::FreeResources);
        return MFX_ERR_NONE;
    }
    mfxStatus SetAuxParams(void*, int) {
        m_calls.push_back(APIfuncs::SetAuxParams);
        return MFX_ERR_NONE;
    }
    void Release() {
        m_calls.push_back(APIfuncs::Release);
        return;
    }

    //Enc
    mfxStatus EncFrameSubmit(mfxENCInput *in, mfxENCOutput *out, mfxThreadTask *task)
    {
        m_calls.push_back(APIfuncs::EncFrameSubmit);
        return MFX_ERR_NONE;
    }
};


class FakeDecoder: public MFXDecoderPlugin {
public:
    std::vector<mfxU32> m_calls;

    FakeDecoder() {
    }
    virtual ~FakeDecoder() {
    }

    //common func
    mfxStatus Init(mfxVideoParam*) {
        m_calls.push_back(APIfuncs::Init);
        return MFX_ERR_NONE;
    }
    mfxStatus QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *in,
            mfxFrameAllocRequest *out) {
        m_calls.push_back(APIfuncs::QueryIOSurf);
        return MFX_ERR_NONE;
    }
    mfxStatus Query(mfxVideoParam *in, mfxVideoParam *out) {
        m_calls.push_back(APIfuncs::Query);
        return MFX_ERR_NONE;
    }
    mfxStatus Reset(mfxVideoParam *par) {
        m_calls.push_back(APIfuncs::Reset);
        return MFX_ERR_NONE;
    }
    mfxStatus Close() {
        m_calls.push_back(APIfuncs::Close);
        return MFX_ERR_NONE;
    }
    mfxStatus PluginInit(mfxCoreInterface *core) {
        m_calls.push_back(APIfuncs::PluginInit);
        return MFX_ERR_NONE;
    }
    mfxStatus PluginClose() {
        m_calls.push_back(APIfuncs::PluginClose);
        return MFX_ERR_NONE;
    }
    mfxStatus GetVideoParam(mfxVideoParam *par) {
        m_calls.push_back(APIfuncs::GetVideoParam);
        return MFX_ERR_NONE;
    }
    mfxStatus GetPluginParam(mfxPluginParam *par) {
        m_calls.push_back(APIfuncs::GetPluginParam);
        return MFX_ERR_NONE;
    }
    mfxStatus Execute(mfxThreadTask task, mfxU32 uid_p, mfxU32 uid_a) {
        m_calls.push_back(APIfuncs::Execute);
        return MFX_ERR_NONE;
    }
    mfxStatus FreeResources(mfxThreadTask task, mfxStatus sts) {
        m_calls.push_back(APIfuncs::FreeResources);
        return MFX_ERR_NONE;
    }
    mfxStatus SetAuxParams(void*, int) {
        m_calls.push_back(APIfuncs::SetAuxParams);
        return MFX_ERR_NONE;
    }
    void Release() {
        m_calls.push_back(APIfuncs::Release);
        return;
    }

    //Decoder
    mfxStatus DecodeHeader(mfxBitstream *bs, mfxVideoParam *par) {
        m_calls.push_back(APIfuncs:: DecodeHeader);
        return MFX_ERR_NONE;
    }
    mfxStatus GetPayload(mfxU64 *ts, mfxPayload *payload) {
        m_calls.push_back(APIfuncs:: GetPayload);
        return MFX_ERR_NONE;
    }
    mfxStatus DecodeFrameSubmit(mfxBitstream *bs,
            mfxFrameSurface1 *surface_work, mfxFrameSurface1 **surface_out,
            mfxThreadTask *task) {
        m_calls.push_back(APIfuncs:: DecodeFrameSubmit);
        return MFX_ERR_NONE;
    }
};

