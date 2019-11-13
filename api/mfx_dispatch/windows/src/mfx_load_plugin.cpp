// Copyright (c) 2013-2019 Intel Corporation
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

#include "mfx_load_plugin.h"
#include "mfx_load_dll.h"
#include "mfx_dispatcher_log.h"

#define TRACE_PLUGIN_ERROR(str, ...) DISPATCHER_LOG_ERROR((("[PLUGIN]: " str), __VA_ARGS__))
#define TRACE_PLUGIN_INFO(str, ...) DISPATCHER_LOG_INFO((("[PLUGIN]: " str), __VA_ARGS__))

#define CREATE_PLUGIN_FNC "CreatePlugin"

MFX::PluginModule::PluginModule()
    : mHmodule()
    , mCreatePluginPtr()
    , mPath()
{
}

MFX::PluginModule::PluginModule(const PluginModule & that)
    : mHmodule(mfx_dll_load(that.mPath))
    , mCreatePluginPtr(that.mCreatePluginPtr)
{
    wcscpy_s(mPath, sizeof(mPath) / sizeof(*mPath), that.mPath);
}

MFX::PluginModule & MFX::PluginModule::operator = (const MFX::PluginModule & that)
{
    if (this != &that)
    {
        Tidy();
        mHmodule = mfx_dll_load(that.mPath);
        mCreatePluginPtr = that.mCreatePluginPtr;
        wcscpy_s(mPath, sizeof(mPath) / sizeof(*mPath), that.mPath);
    }
    return *this;
}

MFX::PluginModule::PluginModule(const wchar_t * path)
    : mCreatePluginPtr()
{
    mHmodule = mfx_dll_load(path);
    if (NULL == mHmodule) {
        TRACE_PLUGIN_ERROR("Cannot load module: %S\n", path);
        return ;
    }
    TRACE_PLUGIN_INFO("Plugin loaded at: %S\n", path);

    mCreatePluginPtr = (CreatePluginPtr_t)mfx_dll_get_addr(mHmodule, CREATE_PLUGIN_FNC);
    if (NULL == mCreatePluginPtr) {
        TRACE_PLUGIN_ERROR("Cannot get procedure address: %s\n", CREATE_PLUGIN_FNC);
        return ;
    }

    wcscpy_s(mPath, sizeof(mPath) / sizeof(*mPath), path);
}

bool MFX::PluginModule::Create( mfxPluginUID uid, mfxPlugin& plg)
{
    bool result = false;
    if (mCreatePluginPtr)
    {
        mfxStatus mfxResult = mCreatePluginPtr(uid, &plg);
        result = (MFX_ERR_NONE == mfxResult);
        if (!result) {
            TRACE_PLUGIN_ERROR("\"%S::%s\" returned %d\n", mPath, CREATE_PLUGIN_FNC, mfxResult);
        } else {
            TRACE_PLUGIN_INFO("\"%S::%s\" SUCCEED\n", mPath, CREATE_PLUGIN_FNC);
        }
    }
    return result;
}

void MFX::PluginModule::Tidy()
{
    mfx_dll_free(mHmodule);
    mCreatePluginPtr = NULL;
    mHmodule = NULL;
}

MFX::PluginModule::~PluginModule(void)
{
    Tidy();
}

#if !defined(MEDIASDK_UWP_DISPATCHER)

bool MFX::MFXPluginFactory::RunVerification( const mfxPlugin & plg, const PluginDescriptionRecord &dsc, mfxPluginParam &pluginParams)
{
    if (plg.PluginInit == 0)
    {
        TRACE_PLUGIN_ERROR("plg->PluginInit = 0\n", 0);
        return false;
    }
    if (plg.PluginClose == 0)
    {
        TRACE_PLUGIN_ERROR("plg->PluginClose = 0\n", 0);
        return false;
    }
    if (plg.GetPluginParam == 0)
    {
        TRACE_PLUGIN_ERROR("plg->GetPluginParam = 0\n", 0);
        return false;
    }

    if (plg.Execute == 0)
    {
        TRACE_PLUGIN_ERROR("plg->Execute = 0\n", 0);
        return false;
    }
    if (plg.FreeResources == 0)
    {
        TRACE_PLUGIN_ERROR("plg->FreeResources = 0\n", 0);
        return false;
    }

    mfxStatus sts = plg.GetPluginParam(plg.pthis, &pluginParams);
    if (sts != MFX_ERR_NONE)
    {
        TRACE_PLUGIN_ERROR("plg->GetPluginParam() returned %d\n", sts);
        return false;
    }

    if (dsc.Default)
    {
        // for default plugins there is no description, dsc.APIVersion, dsc.PluginVersion and dsc.PluginUID were set by dispatcher
        // dsc.PluginVersion == requested plugin version (parameter of MFXVideoUSER_Load); dsc.APIVersion == loaded library API
        if (dsc.PluginVersion > pluginParams.PluginVersion)
        {
            TRACE_PLUGIN_ERROR("plg->GetPluginParam() returned PluginVersion=%d, but it is smaller than requested : %d\n", pluginParams.PluginVersion, dsc.PluginVersion);
            return false;
        }
    }
    else
    {
        if (!dsc.onlyVersionRegistered && pluginParams.CodecId != dsc.CodecId)
        {
            TRACE_PLUGIN_ERROR("plg->GetPluginParam() returned CodecId=" MFXFOURCCTYPE()", but registration has CodecId=" MFXFOURCCTYPE()"\n"
                , MFXU32TOFOURCC(pluginParams.CodecId), MFXU32TOFOURCC(dsc.CodecId));
            return false;
        }

        if (!dsc.onlyVersionRegistered && pluginParams.Type != dsc.Type)
        {
            TRACE_PLUGIN_ERROR("plg->GetPluginParam() returned Type=%d, but registration has Type=%d\n", pluginParams.Type, dsc.Type);
            return false;
        }

        if (pluginParams.PluginUID !=  dsc.PluginUID)
        {
            TRACE_PLUGIN_ERROR("plg->GetPluginParam() returned UID=" MFXGUIDTYPE()", but registration has UID=" MFXGUIDTYPE()"\n"
                , MFXGUIDTOHEX(&pluginParams.PluginUID), MFXGUIDTOHEX(&dsc.PluginUID));
            return false;
        }

        if (pluginParams.PluginVersion != dsc.PluginVersion)
        {
            TRACE_PLUGIN_ERROR("plg->GetPluginParam() returned PluginVersion=%d, but registration has PlgVer=%d\n", pluginParams.PluginVersion, dsc.PluginVersion);
            return false;
        }

        if (pluginParams.APIVersion.Version != dsc.APIVersion.Version)
        {
            TRACE_PLUGIN_ERROR("plg->GetPluginParam() returned APIVersion=%d.%d, but registration has APIVer=%d.%d\n"
                , pluginParams.APIVersion.Major, pluginParams.APIVersion.Minor
                , dsc.APIVersion.Major, dsc.APIVersion.Minor);
            return false;
        }
    }

    switch(pluginParams.Type)
    {
        case MFX_PLUGINTYPE_VIDEO_DECODE:
        case MFX_PLUGINTYPE_VIDEO_ENCODE:
        case MFX_PLUGINTYPE_VIDEO_VPP:
        {
            TRACE_PLUGIN_INFO("plugin type= %d\n", pluginParams.Type);
            if (plg.Video == 0)
            {
                TRACE_PLUGIN_ERROR("plg->Video = 0\n", 0);
                return false;
            }

            if (!VerifyCodecCommon(*plg.Video))
                return false;
            break;
        }
    }

    switch(pluginParams.Type)
    {
        case MFX_PLUGINTYPE_VIDEO_DECODE:
            return VerifyDecoder(*plg.Video);
        case MFX_PLUGINTYPE_AUDIO_DECODE:
            return VerifyAudioDecoder(*plg.Audio);
        case MFX_PLUGINTYPE_VIDEO_ENCODE:
            return VerifyEncoder(*plg.Video);
        case MFX_PLUGINTYPE_AUDIO_ENCODE:
            return VerifyAudioEncoder(*plg.Audio);
        case MFX_PLUGINTYPE_VIDEO_VPP:
            return VerifyVpp(*plg.Video);
        case MFX_PLUGINTYPE_VIDEO_ENC:
            return VerifyEnc(*plg.Video);
        default:
        {
            TRACE_PLUGIN_ERROR("unsupported plugin type: %d\n", pluginParams.Type);
            return false;
        }
    }
}

bool MFX::MFXPluginFactory::VerifyVpp( const mfxVideoCodecPlugin &vpp )
{
    if (vpp.VPPFrameSubmit == 0)
    {
        TRACE_PLUGIN_ERROR("plg->Video->VPPFrameSubmit = 0\n", 0);
        return false;
    }

    return true;

}

bool MFX::MFXPluginFactory::VerifyEncoder( const mfxVideoCodecPlugin &encoder )
{
    if (encoder.EncodeFrameSubmit == 0)
    {
        TRACE_PLUGIN_ERROR("plg->Video->EncodeFrameSubmit = 0\n", 0);
        return false;
    }

    return true;
}

bool MFX::MFXPluginFactory::VerifyAudioEncoder( const mfxAudioCodecPlugin &encoder )
{
    if (encoder.EncodeFrameSubmit == 0)
    {
        TRACE_PLUGIN_ERROR("plg->Audio->EncodeFrameSubmit = 0\n", 0);
        return false;
    }

    return true;
}

bool MFX::MFXPluginFactory::VerifyEnc( const mfxVideoCodecPlugin &videoEnc )
{
    if (videoEnc.ENCFrameSubmit == 0)
    {
        TRACE_PLUGIN_ERROR("plg->Video->EncodeFrameSubmit = 0\n", 0);
        return false;
    }

    return true;
}

bool MFX::MFXPluginFactory::VerifyDecoder( const mfxVideoCodecPlugin &decoder )
{
    if (decoder.DecodeHeader == 0)
    {
        TRACE_PLUGIN_ERROR("plg->Video->DecodeHeader = 0\n", 0);
        return false;
    }
    if (decoder.GetPayload == 0)
    {
        TRACE_PLUGIN_ERROR("plg->Video->GetPayload = 0\n", 0);
        return false;
    }
    if (decoder.DecodeFrameSubmit == 0)
    {
        TRACE_PLUGIN_ERROR("plg->Video->DecodeFrameSubmit = 0\n", 0);
        return false;
    }

    return true;
}

bool MFX::MFXPluginFactory::VerifyAudioDecoder( const mfxAudioCodecPlugin &decoder )
{
    if (decoder.DecodeHeader == 0)
    {
        TRACE_PLUGIN_ERROR("plg->Audio->DecodeHeader = 0\n", 0);
        return false;
    }
//    if (decoder.GetPayload == 0)
    {
  //      TRACE_PLUGIN_ERROR("plg->Audio->GetPayload = 0\n", 0);
    //    return false;
    }
    if (decoder.DecodeFrameSubmit == 0)
    {
        TRACE_PLUGIN_ERROR("plg->Audio->DecodeFrameSubmit = 0\n", 0);
        return false;
    }

    return true;
}

bool MFX::MFXPluginFactory::VerifyCodecCommon( const mfxVideoCodecPlugin & videoCodec )
{
    if (videoCodec.Query == 0)
    {
        TRACE_PLUGIN_ERROR("plg->Video->Query = 0\n", 0);
        return false;
    }
    //todo: remove
    if (videoCodec.Query == 0)
    {
        TRACE_PLUGIN_ERROR("plg->Video->Query = 0\n", 0);
        return false;
    }
    if (videoCodec.QueryIOSurf == 0)
    {
        TRACE_PLUGIN_ERROR("plg->Video->QueryIOSurf = 0\n", 0);
        return false;
    }
    if (videoCodec.Init == 0)
    {
        TRACE_PLUGIN_ERROR("plg->Video->Init = 0\n", 0);
        return false;
    }
    if (videoCodec.Reset == 0)
    {
        TRACE_PLUGIN_ERROR("plg->Video->Reset = 0\n", 0);
        return false;
    }
    if (videoCodec.Close == 0)
    {
        TRACE_PLUGIN_ERROR("plg->Video->Close = 0\n", 0);
        return false;
    }
    if (videoCodec.GetVideoParam == 0)
    {
        TRACE_PLUGIN_ERROR("plg->Video->GetVideoParam = 0\n", 0);
        return false;
    }

    return true;
}

mfxStatus MFX::MFXPluginFactory::Create(const PluginDescriptionRecord & rec)
{
    PluginModule plgModule(rec.sPath);
    mfxPlugin plg = {};
    mfxPluginParam plgParams;

    if (!plgModule.Create(rec.PluginUID, plg))
    {
        return MFX_ERR_UNKNOWN;
    }

    if (!RunVerification(plg, rec, plgParams))
    {
        //will do not call plugin close since it is not safe to do that until structure is corrected
        return MFX_ERR_UNKNOWN;
    }


    if (rec.Type == MFX_PLUGINTYPE_AUDIO_DECODE ||
        rec.Type == MFX_PLUGINTYPE_AUDIO_ENCODE)
    {
        mfxStatus sts = MFXAudioUSER_Register(mSession, plgParams.Type, &plg);
        if (MFX_ERR_NONE != sts)
        {
            TRACE_PLUGIN_ERROR(" MFXAudioUSER_Register returned %d\n", sts);
            return sts;
        }
    }
    else
    {
        mfxStatus sts = MFXVideoUSER_Register(mSession, plgParams.Type, &plg);
        if (MFX_ERR_NONE != sts)
        {
            TRACE_PLUGIN_ERROR(" MFXVideoUSER_Register returned %d\n", sts);
            return sts;
        }
    }

    mPlugins.push_back(FactoryRecord(plgParams, plgModule, plg));

    return MFX_ERR_NONE;
}

MFX::MFXPluginFactory::~MFXPluginFactory()
{
    Close();
}

MFX::MFXPluginFactory::MFXPluginFactory( mfxSession session ) :
    mPlugins()
{
    mSession = session;
    nPlugins = 0;
}

bool MFX::MFXPluginFactory::Destroy( const mfxPluginUID & uidToDestroy)
{
    for (MFXVector<FactoryRecord >::iterator i = mPlugins.begin(); i!= mPlugins.end(); i++)
    {
        if (i->plgParams.PluginUID == uidToDestroy)
        {
            DestroyPlugin(*i);
            //dll unload should happen here
            //todo: check that dll_free fail is traced
            mPlugins.erase(i);
            return  true;
        }
    }
    return false;
}

void MFX::MFXPluginFactory::Close()
{
    for (MFXVector<FactoryRecord>::iterator i = mPlugins.begin(); i!= mPlugins.end(); i++)
    {
        DestroyPlugin(*i);
    }
    mPlugins.clear();
}

void MFX::MFXPluginFactory::DestroyPlugin( FactoryRecord & record)
{
    mfxStatus sts;
    if (record.plgParams.Type == MFX_PLUGINTYPE_AUDIO_DECODE ||
        record.plgParams.Type == MFX_PLUGINTYPE_AUDIO_ENCODE)
    {
        sts = MFXAudioUSER_Unregister(mSession, record.plgParams.Type);
        TRACE_PLUGIN_INFO(" MFXAudioUSER_Unregister for Type=%d, returned %d\n", record.plgParams.Type, sts);
    }
    else
    {
        sts = MFXVideoUSER_Unregister(mSession, record.plgParams.Type);
        TRACE_PLUGIN_INFO(" MFXVideoUSER_Unregister for Type=%d, returned %d\n", record.plgParams.Type, sts);
    }
}

#endif //!defined(MEDIASDK_UWP_DISPATCHER)