// Copyright (c) 2017 Intel Corporation
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

#if !defined(__MFX_VP8_DEC_PLUGIN_INCLUDED__)
#define __MFX_VP8_DEC_PLUGIN_INCLUDED__

#include <stdio.h>
#include <string.h>
#include <memory>
#include <iostream>
#include "mfxvideo.h"
#include "mfxplugin++.h"
#include "mfxvideo++int.h"

#if defined( AS_VP8D_PLUGIN )

class MFXVP8DecoderPlugin : public MFXDecoderPlugin
{
public:

    static const mfxPluginUID g_VP8DecoderGuid;

    virtual mfxStatus PluginInit(mfxCoreInterface *core); //Init plugin as a component (~MFXInit)
    virtual mfxStatus PluginClose();
    virtual mfxStatus GetPluginParam(mfxPluginParam *par);
    virtual mfxStatus DecodeFrameSubmit(mfxBitstream *bs, mfxFrameSurface1 *surface_work, mfxFrameSurface1 **surface_out,  mfxThreadTask *task)
    {
        return MFXVideoDECODE_DecodeFrameAsync(m_session, bs, surface_work, surface_out, (mfxSyncPoint*) task);
    }
    virtual mfxStatus Execute(mfxThreadTask task, mfxU32 , mfxU32 )
    {
        return MFXVideoCORE_SyncOperation(m_session, (mfxSyncPoint) task, MFX_INFINITE);
    }
    virtual mfxStatus FreeResources(mfxThreadTask , mfxStatus )
    {
        return MFX_ERR_NONE;
    }
    virtual mfxStatus Query(mfxVideoParam *in, mfxVideoParam *out)
    {
        return MFXVideoDECODE_Query(m_session, in, out);
    }
    virtual mfxStatus QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *, mfxFrameAllocRequest *out)
    {
        return MFXVideoDECODE_QueryIOSurf(m_session, par, out);
    }
    virtual mfxStatus Init(mfxVideoParam *par)
    {
        return MFXVideoDECODE_Init(m_session, par);
    }
    virtual mfxStatus Reset(mfxVideoParam *par)
    {
        return MFXVideoDECODE_Reset(m_session, par);
    }
    virtual mfxStatus Close()
    {
        return MFXVideoDECODE_Close(m_session);
    }
    virtual mfxStatus GetVideoParam(mfxVideoParam *par)
    {
        return MFXVideoDECODE_GetVideoParam(m_session, par);
    }
    virtual mfxStatus DecodeHeader(mfxBitstream *bs, mfxVideoParam *par)
    {
        return MFXVideoDECODE_DecodeHeader(m_session, bs, par);
    }
    virtual mfxStatus GetPayload(mfxU64 *ts, mfxPayload *payload)
    {
        return MFXVideoDECODE_GetPayload(m_session, ts, payload);
    }
    virtual void Release()
    {
        delete this;
    }
    static MFXDecoderPlugin* Create() {
        return new MFXVP8DecoderPlugin(false);
    }
    static mfxStatus CreateByDispatcher(mfxPluginUID guid, mfxPlugin* mfxPlg) {
        if (memcmp(& guid , &g_VP8DecoderGuid, sizeof(mfxPluginUID))) {
            return MFX_ERR_NOT_FOUND;
        }
        MFXVP8DecoderPlugin* tmp_pplg = 0;
        try
        {
            tmp_pplg = new MFXVP8DecoderPlugin(false);
        }
        catch(std::bad_alloc&)
        {
            return MFX_ERR_MEMORY_ALLOC;
        }
        catch(MFX_CORE_CATCH_TYPE) 
        { 
            return MFX_ERR_UNKNOWN;
        }

        try
        {
            tmp_pplg->m_adapter.reset(new MFXPluginAdapter<MFXDecoderPlugin> (tmp_pplg));
        }
        catch(std::bad_alloc&)
        {
            delete tmp_pplg;
            return MFX_ERR_MEMORY_ALLOC;
        }
        *mfxPlg = (mfxPlugin)*tmp_pplg->m_adapter;
        tmp_pplg->m_createdByDispatcher = true;
        return MFX_ERR_NONE;
    }
    virtual mfxU32 GetPluginType()
    {
        return MFX_PLUGINTYPE_VIDEO_DECODE;
    }
    virtual mfxStatus SetAuxParams(void* , int )
    {
        return MFX_ERR_UNKNOWN;
    }

protected:
    MFXVP8DecoderPlugin(bool CreateByDispatcher);
    virtual ~MFXVP8DecoderPlugin();

    mfxCoreInterface*   m_pmfxCore;

    mfxSession          m_session;
    mfxPluginParam      m_PluginParam;
    bool                m_createdByDispatcher;
    std::unique_ptr<MFXPluginAdapter<MFXDecoderPlugin> > m_adapter;
};
#endif //#if defined( AS_VP8D_PLUGIN )

#define MSDK_PLUGIN_API(ret_type) extern "C"  ret_type

#endif  // __MFX_VP8_DEC_PLUGIN_INCLUDED__
