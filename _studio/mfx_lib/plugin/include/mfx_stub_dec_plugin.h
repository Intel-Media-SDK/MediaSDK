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

#if !defined(__MFX_STUB_DEC_PLUGIN_INCLUDED__)
#define __MFX_STUB_DEC_PLUGIN_INCLUDED__

#include <stdio.h>
#include <string.h>
#include <memory>
#include <iostream>
#include "mfxvideo.h"
#include "mfxplugin++.h"
#include "mfxvideo++int.h"

struct MFXStubDecoderPlugin : MFXDecoderPlugin
{
    /* mfxPlugin */
    static mfxStatus _PluginInit(mfxHDL /*pthis*/, mfxCoreInterface*) {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    static mfxStatus _PluginClose(mfxHDL /*pthis*/) {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    static mfxStatus _Submit(mfxHDL /*pthis*/, const mfxHDL* /*in*/, mfxU32 /*in_num*/, const mfxHDL* /*out*/, mfxU32 /*out_num*/, mfxThreadTask*) {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    static mfxStatus _Execute(mfxHDL /*pthis*/, mfxThreadTask, mfxU32 /*thread_id*/, mfxU32 /*call_count*/) {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    static mfxStatus _FreeResources(mfxHDL /*pthis*/, mfxThreadTask, mfxStatus) {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    /* mfxVideoCodecPlugin */
    static mfxStatus _Query(mfxHDL /*pthis*/, mfxVideoParam* /*in*/, mfxVideoParam* /*out*/) {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    static mfxStatus _QueryIOSurf(mfxHDL /*pthis*/, mfxVideoParam*, mfxFrameAllocRequest* /*in*/, mfxFrameAllocRequest* /*out*/) {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    static mfxStatus _Init(mfxHDL /*pthis*/, mfxVideoParam*) {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    static mfxStatus _Reset(mfxHDL /*pthis*/, mfxVideoParam*) {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    static mfxStatus _Close(mfxHDL /*pthis*/) {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    static mfxStatus _GetVideoParam(mfxHDL /*pthis*/, mfxVideoParam*) {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    static mfxStatus _DecodeHeader(mfxHDL /*pthis*/, mfxBitstream*, mfxVideoParam*) {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    static mfxStatus _GetPayload(mfxHDL /*pthis*/, mfxU64* /*ts*/, mfxPayload*) {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    static mfxStatus _DecodeFrameSubmit(mfxHDL /*pthis*/, mfxBitstream*, mfxFrameSurface1* /*surface_work*/, mfxFrameSurface1** /*surface_out*/,  mfxThreadTask*) {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    static mfxStatus CreateByDispatcher(mfxPluginUID, mfxPlugin*);

private:

    /* MFXPlugin */
    mfxStatus PluginInit(mfxCoreInterface* core) override
    {
        return _PluginInit(this, core);
    }
    mfxStatus PluginClose() override
    {
        return _PluginClose(this);
    }

    mfxStatus Execute(mfxThreadTask task, mfxU32 thread_id, mfxU32 call_count) override
    {
        return _Execute(this, task, thread_id, call_count);
    }
    mfxStatus FreeResources(mfxThreadTask task, mfxStatus status) override
    {
        return _FreeResources(this, task, status);
    }
    void Release() override
    {}
    mfxStatus Close() override
    {
        return _Close(this);
    }
    mfxStatus SetAuxParams(void* , int ) override
    {
        return MFX_ERR_UNKNOWN;
    }

    /* MFXCodecPlugin */
    mfxStatus Init(mfxVideoParam *par) override
    {
        return _Init(this, par);
    }
    mfxStatus QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *in, mfxFrameAllocRequest *out) override
    {
        return _QueryIOSurf(this, par, in, out);
    }
    mfxStatus Query(mfxVideoParam *in, mfxVideoParam *out) override
    {
        return _Query(this, in, out);
    }
    mfxStatus Reset(mfxVideoParam *par) override
    {
        return _Reset(this, par);
    }
    mfxStatus GetVideoParam(mfxVideoParam *par) override
    {
        return _GetVideoParam(this, par);
    }

    mfxStatus DecodeHeader(mfxBitstream *bs, mfxVideoParam *par) override
    {
        return _DecodeHeader(this, bs, par);
    }
    mfxStatus GetPayload(mfxU64 *ts, mfxPayload *payload) override
    {
        return _GetPayload(this, ts, payload);
    }
    mfxStatus DecodeFrameSubmit(mfxBitstream *bs, mfxFrameSurface1 *surface_work, mfxFrameSurface1 **surface_out,  mfxThreadTask *task) override
    {
        return _DecodeFrameSubmit(this, bs, surface_work, surface_out, task);
    }
};

#endif  // __MFX_STUB_DEC_PLUGIN_INCLUDED__
