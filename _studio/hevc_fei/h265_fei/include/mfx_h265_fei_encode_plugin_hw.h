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

namespace MfxHwH265FeiEncode
{

    static const mfxPluginUID  MFX_PLUGINID_HEVC_FEI_ENCODE = { { 0x54, 0x18, 0xa7, 0x06, 0x66, 0xf9, 0x4d, 0x5c, 0xb4, 0xf7, 0xb1, 0xca, 0xee, 0x86, 0x33, 0x9b } };

    class H265FeiEncodePlugin : public MFXEncoderPlugin
    {
    public:
        static MFXEncoderPlugin* Create()
        {
            return GetInstance();
        }

        static mfxStatus CreateByDispatcher(mfxPluginUID guid, mfxPlugin* mfxPlg)
        {
            if (guid != MFX_PLUGINID_HEVC_FEI_ENCODE)
            {
                return MFX_ERR_NOT_FOUND;
            }

            H265FeiEncodePlugin* tmp_pplg = nullptr;

            try
            {
                tmp_pplg = GetInstance();
            }
            catch (...)
            {
                return MFX_ERR_UNKNOWN;
            }

            *mfxPlg = tmp_pplg->m_adapter;
            tmp_pplg->m_createdByDispatcher = true;

            return MFX_ERR_NONE;
        }

        virtual mfxStatus PluginInit(mfxCoreInterface *core) override
        {
            return MFX_ERR_NONE;
        }

        virtual mfxStatus PluginClose() override
        {
            if (m_createdByDispatcher)
                Release();

            return MFX_ERR_NONE;
        }

        virtual mfxStatus GetPluginParam(mfxPluginParam *par) override
        {
            MFX_CHECK_NULL_PTR1(par);

            par->PluginUID        = MFX_PLUGINID_HEVC_FEI_ENCODE;
            par->PluginVersion    = 1;
            par->ThreadPolicy     = MFX_THREADPOLICY_SERIAL;
            par->MaxThreadNum     = 1;
            par->APIVersion.Major = MFX_VERSION_MAJOR;
            par->APIVersion.Minor = MFX_VERSION_MINOR;
            par->Type             = MFX_PLUGINTYPE_VIDEO_ENCODE;
            par->CodecId          = MFX_CODEC_HEVC;

            return MFX_ERR_NONE;
        }

        virtual void Release() override
        {
            return;
        }

        virtual mfxStatus Close() override
        {
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        }

        virtual mfxStatus Execute(mfxThreadTask , mfxU32 , mfxU32 ) override
        {
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        }

        virtual mfxStatus FreeResources(mfxThreadTask , mfxStatus) override
        {
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        }

        virtual mfxStatus QueryIOSurf(mfxVideoParam *, mfxFrameAllocRequest *, mfxFrameAllocRequest * ) override
        {
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        }

        virtual mfxStatus Query(mfxVideoParam *, mfxVideoParam *) override
        {
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        }

        virtual mfxStatus Init(mfxVideoParam *) override
        {
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        }

        virtual mfxStatus Reset(mfxVideoParam *) override
        {
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        }

        virtual mfxStatus GetVideoParam(mfxVideoParam *) override
        {
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        }

        virtual mfxStatus EncodeFrameSubmit(mfxEncodeCtrl *, mfxFrameSurface1 *, mfxBitstream *, mfxThreadTask *) override
        {
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        }

        virtual mfxStatus SetAuxParams(void*, int) override
        {
            return MFX_ERR_UNSUPPORTED;
        }
    protected:
        explicit H265FeiEncodePlugin(bool CreateByDispatcher)
            : m_createdByDispatcher(CreateByDispatcher)
            , m_adapter(this)
        {}

        ~H265FeiEncodePlugin()
        {}

        static H265FeiEncodePlugin* GetInstance()
        {
            static H265FeiEncodePlugin instance(false);
            return &instance;
        }

        bool                               m_createdByDispatcher;
        MFXPluginAdapter<MFXEncoderPlugin> m_adapter;
    };
}
