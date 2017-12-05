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

#if !defined(__MFX_HEVC_ENC_PLUGIN_INCLUDED__)
#define __MFX_HEVC_ENC_PLUGIN_INCLUDED__

#include <stdio.h>
#include <string.h>
#include <memory>
#include <iostream>
#include "mfxvideo.h"
#include "mfxplugin++.h"
#include "mfx_h265_encode_api.h"
#include <mfx_task.h>

#if defined( AS_HEVCE_PLUGIN )
class MFXHEVCEncoderPlugin : public MFXEncoderPlugin
{
    static const mfxPluginUID g_HEVCEncoderGuid;
public:
    virtual mfxStatus PluginInit(mfxCoreInterface *core);
    virtual mfxStatus PluginClose();
    virtual mfxStatus GetPluginParam(mfxPluginParam *par);
    virtual mfxStatus EncodeFrameSubmit(mfxEncodeCtrl *ctrl, mfxFrameSurface1 *surface, mfxBitstream *bs, mfxThreadTask *task)
    {
        MFX_CHECK_NULL_PTR1(task);
        mfxFrameSurface1 *reordered_surface = NULL;
        mfxEncodeInternalParams internal_params;
        MFX_ENTRY_POINT *entryPoint = new MFX_ENTRY_POINT;
        mfxStatus sts = m_encoder->EncodeFrameCheck(ctrl,
                                                    surface,
                                                    bs,
                                                    &reordered_surface,
                                                    &internal_params,
                                                    entryPoint);
        if (sts < MFX_ERR_NONE && sts != MFX_ERR_MORE_DATA_SUBMIT_TASK) {
            delete entryPoint;
            entryPoint = NULL;
        }

        *task = entryPoint;
        return sts;
    }
    virtual mfxStatus Execute(mfxThreadTask task, mfxU32 uid_p, mfxU32 uid_a)
    {
        MFX_CHECK_NULL_PTR1(task);
        MFX_ENTRY_POINT *entryPoint = (MFX_ENTRY_POINT *)task;
        return entryPoint->pRoutine(entryPoint->pState, entryPoint->pParam, uid_p, uid_a);
    }
    virtual mfxStatus FreeResources(mfxThreadTask task, mfxStatus )
    {
        if (task) {
            MFX_ENTRY_POINT *entryPoint = (MFX_ENTRY_POINT *)task;
            entryPoint->pCompleteProc(entryPoint->pState, entryPoint->pParam, MFX_ERR_NONE);
            delete entryPoint;
        }
        return MFX_ERR_NONE;
    }
    virtual mfxStatus Query(mfxVideoParam *in, mfxVideoParam *out)
    {
        return m_encoder->Query(&m_mfxCore, in, out);
    }
    virtual mfxStatus QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *in, mfxFrameAllocRequest *)
    {
        return m_encoder->QueryIOSurf(&m_mfxCore, par, in);
    }
    virtual mfxStatus Init(mfxVideoParam *par)
    {
        return m_encoder->Init(par);
    }
    virtual mfxStatus Reset(mfxVideoParam *par)
    {
        return m_encoder->Reset(par);
    }
    virtual mfxStatus Close()
    {
        return m_encoder->Close();
    }
    virtual mfxStatus GetVideoParam(mfxVideoParam *par)
    {
        return m_encoder->GetVideoParam(par);
    }
    virtual void Release()
    {
        delete this;
    }
    static MFXEncoderPlugin* Create() {
        return new MFXHEVCEncoderPlugin(false);
    }
    static mfxStatus CreateByDispatcher(mfxPluginUID guid, mfxPlugin* mfxPlg) {
        if (memcmp(& guid , &g_HEVCEncoderGuid, sizeof(mfxPluginUID))) {
            return MFX_ERR_NOT_FOUND;
        }
        MFXHEVCEncoderPlugin* tmp_pplg = 0;
        try
        {
            tmp_pplg = new MFXHEVCEncoderPlugin(false);
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
            tmp_pplg->m_adapter.reset(new MFXPluginAdapter<MFXEncoderPlugin> (tmp_pplg));
        }
        catch(std::bad_alloc&)
        {
            delete tmp_pplg;
            return MFX_ERR_MEMORY_ALLOC;
        }

        *mfxPlg = tmp_pplg->m_adapter->operator mfxPlugin();
        tmp_pplg->m_createdByDispatcher = true;
        return MFX_ERR_NONE;
    }
    virtual mfxU32 GetPluginType()
    {
        return MFX_PLUGINTYPE_VIDEO_ENCODE;
    }
    virtual mfxStatus SetAuxParams(void* , int )
    {
        return MFX_ERR_UNKNOWN;
    }

protected:
    MFXHEVCEncoderPlugin(bool CreateByDispatcher);
    virtual ~MFXHEVCEncoderPlugin();

    MFXCoreInterface1   m_mfxCore;
    MFXVideoENCODEH265 *m_encoder;

    mfxPluginParam      m_PluginParam;
    bool                m_createdByDispatcher;
    std::unique_ptr<MFXPluginAdapter<MFXEncoderPlugin> > m_adapter;
};
#endif //#if defined( AS_HEVCE_PLUGIN )

#define MSDK_PLUGIN_API(ret_type) extern "C"  ret_type  

#endif  // __MFX_HEVC_ENC_PLUGIN_INCLUDED__
