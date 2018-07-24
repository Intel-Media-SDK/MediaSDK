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

#include "mfx_stub_dec_plugin.h"

//defining module template for decoder plugin
#include "mfx_plugin_module.h"

#include <mfx_trace.h>

#include "mfx_session.h"
#include "mfx_utils.h"

static
mfxVideoCodecPlugin* GetVideoCodecInstance()
{
    static mfxVideoCodecPlugin instance{};
    return &instance;
}

mfxStatus MFXStubDecoderPlugin::CreateByDispatcher(mfxPluginUID, mfxPlugin* mfxPlg)
{
    MFX_CHECK_NULL_PTR1(mfxPlg);

    memset(mfxPlg, 0, sizeof(mfxPlugin));

    mfxPlg->PluginInit     = _PluginInit;
    mfxPlg->PluginClose    = _PluginClose;
    mfxPlg->Submit         = _Submit;
    mfxPlg->Execute        = _Execute;
    mfxPlg->FreeResources  = _FreeResources;

    mfxPlg->Video                = GetVideoCodecInstance();
    mfxPlg->Video->Query         = _Query;
    mfxPlg->Video->QueryIOSurf   = _QueryIOSurf;
    mfxPlg->Video->Init          = _Init;
    mfxPlg->Video->Reset         = _Reset;
    mfxPlg->Video->Close         = _Close;
    mfxPlg->Video->GetVideoParam = _GetVideoParam;

    mfxPlg->Video->DecodeHeader      =  _DecodeHeader;
    mfxPlg->Video->GetPayload        =  _GetPayload;
    mfxPlg->Video->DecodeFrameSubmit =  _DecodeFrameSubmit;

    return MFX_ERR_NONE;
}

