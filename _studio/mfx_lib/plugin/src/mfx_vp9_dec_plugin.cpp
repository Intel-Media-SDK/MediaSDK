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

#include "mfx_vp9_dec_plugin.h"

//defining module template for decoder plugin
#include "mfx_plugin_module.h"

#include "mediasdk_version.h"

#include "mfx_utils.h"

#ifndef UNIFIED_PLUGIN 

MSDK_PLUGIN_API(MFXDecoderPlugin*) mfxCreateDecoderPlugin() {
    return MFXVP9DecoderPlugin::Create();
}

MSDK_PLUGIN_API(mfxStatus) CreatePlugin(mfxPluginUID uid, mfxPlugin* plugin) {
    return MFXVP9DecoderPlugin::CreateByDispatcher(uid, plugin);
}

#endif

const mfxPluginUID MFXVP9DecoderPlugin::g_VP9DecoderGuid = { 0xa9, 0x22, 0x39, 0x4d, 0x8d, 0x87, 0x45, 0x2f, 0x87, 0x8c, 0x51, 0xf2, 0xfc, 0x9b, 0x41, 0x31 };
// a922394d8d87452f878c51f2fc9b4131

static
MFXVP9DecoderPlugin* GetVP9DecoderInstance()
{
    static MFXVP9DecoderPlugin instance{};
    return &instance;
}

MFXDecoderPlugin* MFXVP9DecoderPlugin::Create()
{ return GetVP9DecoderInstance(); }

mfxStatus  MFXVP9DecoderPlugin::_GetPluginParam(mfxHDL /*pthis*/, mfxPluginParam *par)
{
    MFX_CHECK_NULL_PTR1(par);

    memset(par, 0, sizeof(mfxPluginParam));

    par->CodecId = MFX_CODEC_VP9;
    par->ThreadPolicy = MFX_THREADPOLICY_SERIAL;
    par->MaxThreadNum = 1;
    par->APIVersion.Major = MFX_VERSION_MAJOR;
    par->APIVersion.Minor = MFX_VERSION_MINOR;
    par->PluginUID = g_VP9DecoderGuid;
    par->Type = MFX_PLUGINTYPE_VIDEO_DECODE;
    par->PluginVersion = 1;


    return MFX_ERR_NONE;
}

mfxStatus MFXVP9DecoderPlugin::GetPluginParam(mfxPluginParam* par)
{
    return _GetPluginParam(this, par);
}

mfxStatus MFXVP9DecoderPlugin::CreateByDispatcher(mfxPluginUID guid, mfxPlugin* mfxPlg)
{
    if (guid != g_VP9DecoderGuid) {
        return MFX_ERR_NOT_FOUND;
    }

    MFXStubDecoderPlugin::CreateByDispatcher(guid, mfxPlg);
    mfxPlg->pthis          = reinterpret_cast<mfxHDL>(MFXVP9DecoderPlugin::Create());
    mfxPlg->GetPluginParam = _GetPluginParam;

    return MFX_ERR_NONE;
}

