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

#include <mfxplugin.h>
#include <mfx_session.h>
#include <mfx_task.h>
#include <mfx_user_plugin.h>
#include <mfx_utils.h>
#include <libmfx_core_interface.h>
// static section of the file
namespace
{

    VideoCodecUSER *CreateUSERSpecificClass(mfxU32 /*type*/)
    {
        return new VideoUSERPlugin;

    } // VideoUSER *CreateUSERSpecificClass(mfxU32 type)

    struct SessionPtr
    {
    private:
        mfxSession _session;
        std::unique_ptr<VideoCodecUSER> *_ptr;
        bool _isNeedEnc;
        bool _isNeedCodec;
        bool _isNeedDeCoder;
        bool _isNeedVPP;

        mutable std::unique_ptr<VideoDECODE> _stubDecode;
        mutable std::unique_ptr<VideoENCODE> _stubEncode;
        mutable std::unique_ptr<VideoVPP> _stubVPP;
        mutable std::unique_ptr<VideoENC> _stubEnc;

    public:
        SessionPtr(mfxSession session, mfxU32 type = MFX_PLUGINTYPE_VIDEO_GENERAL)
            : _session(session)
        {
            switch(type)
            {
            case MFX_PLUGINTYPE_VIDEO_DECODE :
                _ptr = &_session->m_plgDec;
                _isNeedCodec = false;
                _isNeedDeCoder = true;
                _isNeedVPP = false;
                _isNeedEnc = false;
                break;
            case MFX_PLUGINTYPE_VIDEO_ENCODE:
                _ptr =&_session->m_plgEnc;
                _isNeedCodec = true;
                _isNeedDeCoder = false;
                _isNeedVPP = false;
                _isNeedEnc = false;
                break;
            case MFX_PLUGINTYPE_VIDEO_VPP :
                _ptr = &_session->m_plgVPP;
                _isNeedCodec = false;
                _isNeedDeCoder = false;
                _isNeedVPP = true;
                _isNeedEnc = false;
                break;
            case MFX_PLUGINTYPE_VIDEO_ENC :
                {
#if defined (MFX_PLUGIN_FILE_VERSION) || defined(MFX_PLUGIN_PRODUCT_VERSION)
                    static_assert(false, "This file under no conditions should appear in plugin code.");
#endif
                    // we know that this conversion is safe as this is library-only code
                    // _mfxSession_1_10 - should be used always to get versioned session instance
                    // interface MFXISession_1_10/MFXISession_1_10_GUID may differs
                    // here we use MFXISession_1_10 because it is first version which introduces Pre-Enc plugins
                    _mfxSession_1_10 * versionedSession = (_mfxSession_1_10 *)(session);
                    MFXIPtr<MFXISession_1_10> newSession(versionedSession->QueryInterface(MFXISession_1_10_GUID));
                    if (newSession)
                    {
                        _ptr = &newSession->GetPreEncPlugin();
                        _isNeedCodec = false;
                        _isNeedDeCoder = false;
                        _isNeedVPP = false;
                        _isNeedEnc = true;
                    }
                    else
                    {
                        throw MFX_ERR_UNDEFINED_BEHAVIOR;
                    }
                }
                break;
            case MFX_PLUGINTYPE_VIDEO_GENERAL :
                _ptr = &_session->m_plgGen;
                _isNeedCodec = false;
                _isNeedDeCoder = false;
                _isNeedVPP = false;
                _isNeedEnc = false;
                break;
            default :
                //unknown plugin type
                throw MFX_ERR_UNDEFINED_BEHAVIOR;
            }
        }
        std::unique_ptr<VideoCodecUSER>& plugin()const
        {
            return *_ptr;
        }

        template <class T>
        std::unique_ptr<T>& codec()const
        {
        }
        bool isNeedEnc()const
        {
            return _isNeedEnc ;
        }

        bool isNeedEncoder()const
        {
            return _isNeedCodec ;
        }
        bool isNeedDecoder()const
        {
            return _isNeedDeCoder;
        }
        bool isNeedVPP()const
        {
            return _isNeedVPP;
        }
    };



    template <>
    std::unique_ptr<VideoENCODE>& SessionPtr::codec<VideoENCODE>()const
    {
        return _isNeedCodec ? _session->m_pENCODE : _stubEncode;
    }
    template <>
    std::unique_ptr<VideoENC>& SessionPtr::codec<VideoENC>()const
    {
        return _isNeedEnc ? _session->m_pENC : _stubEnc;
    }
    template <>
    std::unique_ptr<VideoDECODE>& SessionPtr::codec<VideoDECODE>()const
    {
        return _isNeedDeCoder ? _session->m_pDECODE : _stubDecode;
    }
    template <>
    std::unique_ptr<VideoVPP>& SessionPtr::codec<VideoVPP>()const
    {
        return _isNeedVPP ? _session->m_pVPP : _stubVPP;
    }


} // namespace

const mfxPluginUID NativePlugins[] =
{
    MFX_PLUGINID_HEVCD_HW,
    MFX_PLUGINID_VP8D_HW,
    MFX_PLUGINID_VP9D_HW,
    MFX_PLUGINID_HEVCE_HW,
    MFX_PLUGINID_VP9E_HW,
#if MFX_VERSION >= 1027
    MFX_PLUGINID_HEVC_FEI_ENCODE
#endif
};

mfxStatus MFXVideoUSER_Register(mfxSession session, mfxU32 type,
                                const mfxPlugin *par)
{
    mfxStatus mfxRes;

    // check error(s)
    MFX_CHECK(session, MFX_ERR_INVALID_HANDLE);
    MFX_CHECK_NULL_PTR1(par);
    MFX_CHECK_NULL_PTR1(par->GetPluginParam);

    try
    {
        SessionPtr sessionPtr(session, type);
        std::unique_ptr<VideoCodecUSER> & pluginPtr = sessionPtr.plugin();
        std::unique_ptr<VideoENCODE> &encPtr = sessionPtr.codec<VideoENCODE>();
        std::unique_ptr<VideoDECODE> &decPtr = sessionPtr.codec<VideoDECODE>();
        std::unique_ptr<VideoVPP> &vppPtr = sessionPtr.codec<VideoVPP>();
        std::unique_ptr<VideoENC> &preEncPtr = sessionPtr.codec<VideoENC>();
        mfxPluginParam pluginParam = {};

        // the plugin with the same type is already exist
        if (pluginPtr.get() || decPtr.get() || encPtr.get() || preEncPtr.get())
        {
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        }

#ifndef MFX_ENABLE_USER_VPP
        if (sessionPtr.isNeedVPP()) {
            return MFX_ERR_UNSUPPORTED;
        }
#endif

        //check is this plugin was included into MSDK lib as a native component
        mfxRes = par->GetPluginParam(par->pthis, &pluginParam);
        MFX_CHECK_STS(mfxRes);
#if defined (MFX_ENABLE_HEVC_VIDEO_FEI_ENCODE)
        if (MFX_PLUGINID_HEVC_FEI_ENCODE == pluginParam.PluginUID)
        {
            //WA!! to keep backward compatibility with FEI plugin, need to save it's registration
            //so able to select between HEVC FEI and regular HEVC encode
            //main issue for FEI plugin - it doesn't need any specific buffers at initialization
            //the same time it is separate class inherited from regular encode.
            bool * enableFEI = (bool*)(session->m_pCORE.get()->QueryCoreInterface(MFXIFEIEnabled_GUID));
            MFX_CHECK_NULL_PTR1(enableFEI);

            *enableFEI = true;
            return MFX_ERR_NONE;
        }
#endif
        if (std::find(std::begin(NativePlugins), std::end(NativePlugins), pluginParam.PluginUID) != std::end(NativePlugins))
            return MFX_ERR_NONE; //do nothing

        // create a new plugin's instance
        pluginPtr.reset(CreateUSERSpecificClass(type));
        MFX_CHECK(pluginPtr.get(), MFX_ERR_INVALID_VIDEO_PARAM);

        if (sessionPtr.isNeedDecoder()) {
            decPtr.reset(pluginPtr->GetDecodePtr());
        }

        if (sessionPtr.isNeedEncoder()) {
            encPtr.reset(pluginPtr->GetEncodePtr());
        }

        if (sessionPtr.isNeedVPP()) {
            vppPtr.reset(pluginPtr->GetVPPPtr());
        }
        if (sessionPtr.isNeedEnc()) {
            preEncPtr.reset(pluginPtr->GetEncPtr());
        }

        // initialize the plugin
        mfxRes = pluginPtr->PluginInit(par, session, type);
    }
    catch(...)
    {
        // set the default error value
        if (type > MFX_PLUGINTYPE_VIDEO_ENCODE)
        {
            mfxRes = MFX_ERR_UNDEFINED_BEHAVIOR;
        }
        else
        {
            mfxRes = MFX_ERR_UNKNOWN;
        }
    }

    return mfxRes;

} // mfxStatus MFXVideoUSER_Register(mfxSession session, mfxU32 type,

mfxStatus MFXVideoUSER_GetPlugin(mfxSession session, mfxU32 type, mfxPlugin *par)
{
    mfxStatus mfxRes;

    // check error(s)
    MFX_CHECK(session, MFX_ERR_INVALID_HANDLE);
    MFX_CHECK_NULL_PTR1(par);

    try
    {
        SessionPtr sessionPtr(session, type);
        std::unique_ptr<VideoCodecUSER> & pluginPtr = sessionPtr.plugin();

        if (!pluginPtr.get())
        {
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        }

        pluginPtr->GetPlugin(*par);

        mfxRes = MFX_ERR_NONE;
    }
    catch(...)
    {
        // set the default error value
        if (type > MFX_PLUGINTYPE_VIDEO_ENC)
        {
            mfxRes = MFX_ERR_UNDEFINED_BEHAVIOR;
        }
        else
        {
            mfxRes = MFX_ERR_UNKNOWN;
        }
    }

    return mfxRes;

} // mfxStatus MFXVideoUSER_GetPlugin(mfxSession session, mfxU32 type, mfxPlugin *par)

mfxStatus MFXVideoUSER_Unregister(mfxSession session, mfxU32 type)
{
    mfxStatus mfxRes;

    MFX_CHECK(session, MFX_ERR_INVALID_HANDLE);
    try
    {
        SessionPtr sessionPtr(session, type);
        std::unique_ptr<VideoCodecUSER> & registeredPlg = sessionPtr.plugin();
        if (NULL == registeredPlg.get())
            return MFX_ERR_NONE;

        // wait until all tasks are processed
        session->m_pScheduler->WaitForAllTasksCompletion(registeredPlg.get());

        // deinitialize the plugin
        mfxRes = registeredPlg->PluginClose();

        // delete the plugin's instance
        registeredPlg.reset();
        //delete corresponding codec instance
        if (sessionPtr.isNeedDecoder()) {
            sessionPtr.codec<VideoDECODE>().reset();
        }
        if (sessionPtr.isNeedEncoder()) {
            sessionPtr.codec<VideoENCODE>().reset();
        }
        if (sessionPtr.isNeedVPP()) {
            sessionPtr.codec<VideoVPP>().reset();
        }
        if (sessionPtr.isNeedEnc()) {
            sessionPtr.codec<VideoENC>().reset();
        }
    }
    catch(...)
    {
        // set the default error value
        if (type > MFX_PLUGINTYPE_VIDEO_ENCODE)
        {
            mfxRes = MFX_ERR_UNDEFINED_BEHAVIOR;
        }
        else
        {
            mfxRes = MFX_ERR_UNKNOWN;
        }
    }

    return mfxRes;

} // mfxStatus MFXVideoUSER_Unregister(mfxSession session, mfxU32 type)

mfxStatus MFXVideoUSER_ProcessFrameAsync(mfxSession session,
                                         const mfxHDL *in, mfxU32 in_num,
                                         const mfxHDL *out, mfxU32 out_num,
                                         mfxSyncPoint *syncp)
{
    mfxStatus mfxRes;

    MFX_CHECK(session, MFX_ERR_INVALID_HANDLE);
    MFX_CHECK(session->m_plgGen.get(), MFX_ERR_NOT_INITIALIZED);
    MFX_CHECK(syncp, MFX_ERR_NULL_PTR);
    MFX_CHECK(in_num <= MFX_TASK_NUM_DEPENDENCIES && out_num <= MFX_TASK_NUM_DEPENDENCIES, MFX_ERR_UNSUPPORTED);
    try
    {
        //generic plugin function
        std::unique_ptr<VideoCodecUSER> & registeredPlg = SessionPtr(session).plugin();

        mfxSyncPoint syncPoint = NULL;
        MFX_TASK task;

        memset(&task, 0, sizeof(MFX_TASK));
        mfxRes = registeredPlg->Check(in, in_num, out, out_num, &task.entryPoint);
        // source data is OK, go forward
        if (MFX_ERR_NONE == mfxRes)
        {
            mfxU32 i;

            task.pOwner = registeredPlg.get();
            task.priority = session->m_priority;
            task.threadingPolicy = registeredPlg->GetThreadingPolicy();
            // fill dependencies
            for (i = 0; i < in_num; i += 1)
            {
                task.pSrc[i] = in[i];
            }
            for (i = 0; i < out_num; i += 1)
            {
                task.pDst[i] = out[i];
            }

            // register input and call the task
            mfxRes = session->m_pScheduler->AddTask(task, &syncPoint);
        }

        // return pointer to synchronization point
        *syncp = syncPoint;
    }
    catch(...)
    {
        // set the default error value
        mfxRes = MFX_ERR_UNKNOWN;
    }

    return mfxRes;

} // mfxStatus MFXVideoUSER_ProcessFrameAsync(mfxSession session,
