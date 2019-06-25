// Copyright (c) 2018-2019 Intel Corporation
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
#include "mfxplugin++.h"

namespace MfxHwH265Encode
{

static const mfxPluginUID  MFX_PLUGINID_HEVCE_HW = { { 0x6f, 0xad, 0xc7, 0x91, 0xa0, 0xc2, 0xeb, 0x47, 0x9a, 0xb6, 0xdc, 0xd5, 0xea, 0x9d, 0xa3, 0x47 } };

class Plugin : public MFXEncoderPlugin
{
public:
    static MFXEncoderPlugin* Create()
    {
        return GetInstance();
    }
    static mfxStatus CreateByDispatcher(mfxPluginUID guid, mfxPlugin* mfxPlg)
    {
        if (!mfxPlg)
        {
            return MFX_ERR_NULL_PTR;
        }
        if (guid != MFX_PLUGINID_HEVCE_HW)
        {
            return MFX_ERR_NOT_FOUND;
        }

        Plugin* tmp_pplg = nullptr;

        try
        {
            tmp_pplg = GetInstance();
        }
        catch (std::bad_alloc&)
        {
            return MFX_ERR_MEMORY_ALLOC;
        }
        catch (...)
        {
            return MFX_ERR_UNKNOWN;
        }

        *mfxPlg = tmp_pplg->m_adapter;
        tmp_pplg->m_createdByDispatcher = true;

        return MFX_ERR_NONE;
    }
    virtual mfxStatus PluginInit(mfxCoreInterface * /*core*/)
    {
        return MFX_ERR_NONE;
    }
    virtual mfxStatus PluginClose()
    {
        if (m_createdByDispatcher)
            Release();

        return MFX_ERR_NONE;
    }
    virtual mfxStatus GetPluginParam(mfxPluginParam *par)
    {
        MFX_CHECK_NULL_PTR1(par);

        par->PluginUID        = MFX_PLUGINID_HEVCE_HW;
        par->PluginVersion    = 1;
        par->ThreadPolicy     = MFX_THREADPOLICY_SERIAL;
        par->MaxThreadNum     = 1;
        par->APIVersion.Major = MFX_VERSION_MAJOR;
        par->APIVersion.Minor = MFX_VERSION_MINOR;
        par->Type             = MFX_PLUGINTYPE_VIDEO_ENCODE;
        par->CodecId          = MFX_CODEC_HEVC;

        return MFX_ERR_NONE;
    }
    virtual mfxStatus Execute(mfxThreadTask /*task*/, mfxU32 /*uid_p*/, mfxU32 /*uid_a*/)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    virtual mfxStatus FreeResources(mfxThreadTask /*task*/, mfxStatus /*sts*/)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    virtual mfxStatus Init(mfxVideoParam * /*par*/)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    virtual mfxStatus QueryIOSurf(mfxVideoParam * /*par*/, mfxFrameAllocRequest * /*in*/, mfxFrameAllocRequest * /*out*/)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    virtual mfxStatus Query(mfxVideoParam * /*in*/, mfxVideoParam * /*out*/)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    virtual mfxStatus Reset(mfxVideoParam * /*par*/)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    virtual mfxStatus GetVideoParam(mfxVideoParam * /*par*/)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    virtual mfxStatus EncodeFrameSubmit(mfxEncodeCtrl * /*ctrl*/, mfxFrameSurface1 * /*surface*/, mfxBitstream * /*bs*/, mfxThreadTask * /*task*/)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    virtual void Release()
    {
        return;
    }

    virtual mfxStatus Close()
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    virtual mfxStatus SetAuxParams(void*, int)
    {
        return MFX_ERR_UNSUPPORTED;
    }
protected:
    explicit Plugin(bool CreateByDispatcher)
        : m_createdByDispatcher(CreateByDispatcher)
        , m_adapter(this)
    {}

    virtual ~Plugin()
    {}

    static Plugin* GetInstance()
    {
        static Plugin instance(false);
        return &instance;
    }

    bool m_createdByDispatcher;
    MFXPluginAdapter<MFXEncoderPlugin> m_adapter;
};
} //MfxHwH265Encode
