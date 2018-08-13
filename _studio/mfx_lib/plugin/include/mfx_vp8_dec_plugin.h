// Copyright (c) 2017-2018 Intel Corporation
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

#include "mfx_stub_dec_plugin.h"

class MFXVP8DecoderPlugin : MFXStubDecoderPlugin
{
public:

    static const mfxPluginUID g_VP8DecoderGuid;

    static mfxStatus _GetPluginParam(mfxHDL /*pthis*/, mfxPluginParam *);

    static MFXDecoderPlugin* Create();
    static mfxStatus CreateByDispatcher(mfxPluginUID, mfxPlugin*);

    mfxStatus GetPluginParam(mfxPluginParam* par) override;
};

#define MSDK_PLUGIN_API(ret_type) extern "C"  ret_type

#endif  // __MFX_VP8_DEC_PLUGIN_INCLUDED__
