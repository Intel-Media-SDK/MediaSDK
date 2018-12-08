// Copyright (c) 2017-2019 Intel Corporation
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

#include "mfx_h264la_plugin.h"
#include "mfx_session.h"
#include "vm_sys_info.h"

//defining module template for decoder plugin
#include "mfx_plugin_module.h"

#if defined( AS_H264LA_PLUGIN )

PluginModuleTemplate g_PluginModule = {
    NULL,
    NULL,
    NULL,
    NULL,
    &MFXH264LAPlugin::CreateByDispatcher,
    &MFXH264LAPlugin::Create
};

MSDK_PLUGIN_API(MFXEncPlugin*) mfxCreateEncPlugin() {
    if (!g_PluginModule.CreateEncPlugin) {
        return 0;
    }
    return g_PluginModule.CreateEncPlugin();
}

MSDK_PLUGIN_API(MFXPlugin*) CreatePlugin(mfxPluginUID uid, mfxPlugin* plugin) {
    if (!g_PluginModule.CreatePlugin) {
        return 0;
    }
    return (MFXPlugin*) g_PluginModule.CreatePlugin(uid, plugin);
}


MFXH264LAPlugin::MFXH264LAPlugin(bool CreateByDispatcher)
{
    m_session = 0;
    m_pmfxCore = 0;
    memset(&m_PluginParam, 0, sizeof(mfxPluginParam));

    m_PluginParam.CodecId =MFX_CODEC_AVC;
    m_PluginParam.ThreadPolicy = MFX_THREADPOLICY_SERIAL;
    m_PluginParam.MaxThreadNum = 1;
    m_PluginParam.APIVersion.Major = MFX_VERSION_MAJOR;
    m_PluginParam.APIVersion.Minor = MFX_VERSION_MINOR;
    m_PluginParam.PluginUID = MFX_PLUGINID_H264LA_HW;
    m_PluginParam.Type = MFX_PLUGINTYPE_VIDEO_ENC;
    m_PluginParam.PluginVersion = 1;
    m_createdByDispatcher = CreateByDispatcher;
}

MFXH264LAPlugin::~MFXH264LAPlugin()
{
    if (m_session)
    {
        PluginClose();
    }
}

mfxStatus MFXH264LAPlugin::PluginInit(mfxCoreInterface *core)
{
    if (!core)
        return MFX_ERR_NULL_PTR;
    mfxCoreParam par;
    mfxStatus mfxRes = MFX_ERR_NONE;

    m_pmfxCore = core;
    mfxRes = m_pmfxCore->GetCoreParam(m_pmfxCore->pthis, &par);
    MFX_CHECK_STS(mfxRes);


    mfxRes = MFXInit(par.Impl, &par.Version, &m_session);
    MFX_CHECK_STS(mfxRes);

    mfxRes = MFXInternalPseudoJoinSession((mfxSession) m_pmfxCore->pthis, m_session);
    MFX_CHECK_STS(mfxRes);

    return mfxRes;
}

mfxStatus MFXH264LAPlugin::PluginClose()
{
    mfxStatus mfxRes = MFX_ERR_NONE;
    mfxStatus mfxRes2 = MFX_ERR_NONE;
    if (m_session)
    {
        //The application must ensure there is no active task running in the session before calling this (MFXDisjoinSession) function.
        mfxRes = MFXVideoENC_Close(m_session);
        //Return the first met wrn or error
        if(mfxRes != MFX_ERR_NONE && mfxRes != MFX_ERR_NOT_INITIALIZED)
            mfxRes2 = mfxRes;
        mfxRes = MFXInternalPseudoDisjoinSession(m_session);
        if(mfxRes != MFX_ERR_NONE && mfxRes != MFX_ERR_NOT_INITIALIZED && mfxRes2 == MFX_ERR_NONE)
            mfxRes2 = mfxRes;
        mfxRes = MFXClose(m_session);
        if(mfxRes != MFX_ERR_NONE && mfxRes != MFX_ERR_NOT_INITIALIZED && mfxRes2 == MFX_ERR_NONE)
            mfxRes2 = mfxRes;
        m_session = 0;
    }
    if (m_createdByDispatcher) {
        delete this;
    }

    return mfxRes2;
}

mfxStatus MFXH264LAPlugin::GetPluginParam(mfxPluginParam *par)
{
    if (!par)
        return MFX_ERR_NULL_PTR;
    *par = m_PluginParam;

    return MFX_ERR_NONE;
}
#endif