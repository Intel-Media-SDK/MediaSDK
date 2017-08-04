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

#include <mfxplugin.h>
#include <mfx_session.h>
#include <mfx_task.h>
#include <mfx_user_plugin.h>
#include <mfx_utils.h>

#define MSDK_STATIC_ASSERT(COND, MSG)  typedef char static_assertion_##MSG[ (COND) ? 1 : -1];

// static section of the file
namespace
{

    VideoCodecUSER *CreateUSERSpecificClass(mfxU32 type)
    {
        type;
        return new VideoUSERPlugin;

    } // VideoUSER *CreateUSERSpecificClass(mfxU32 type)

    struct SessionPtr
    {
    private:
        mfxSession _session;
        std::auto_ptr<VideoCodecUSER> *_ptr;
        bool _isNeedEnc;
        bool _isNeedCodec;
        bool _isNeedDeCoder;
        bool _isNeedVPP;

        mutable std::auto_ptr<VideoDECODE> _stubDecode;
        mutable std::auto_ptr<VideoENCODE> _stubEncode;
        mutable std::auto_ptr<VideoVPP> _stubVPP;
        mutable std::auto_ptr<VideoENC> _stubEnc;

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
                    MSDK_STATIC_ASSERT("This file under no conditions should appear in plugin code.");
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
        std::auto_ptr<VideoCodecUSER>& plugin()const
        {
            return *_ptr;
        }

        template <class T>
        std::auto_ptr<T>& codec()const
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
    std::auto_ptr<VideoENCODE>& SessionPtr::codec<VideoENCODE>()const
    {
        return _isNeedCodec ? _session->m_pENCODE : _stubEncode;
    }
    template <>
    std::auto_ptr<VideoENC>& SessionPtr::codec<VideoENC>()const
    {
        return _isNeedEnc ? _session->m_pENC : _stubEnc;
    }
    template <>
    std::auto_ptr<VideoDECODE>& SessionPtr::codec<VideoDECODE>()const
    {
        return _isNeedDeCoder ? _session->m_pDECODE : _stubDecode;
    }
    template <>
    std::auto_ptr<VideoVPP>& SessionPtr::codec<VideoVPP>()const
    {
        return _isNeedVPP ? _session->m_pVPP : _stubVPP;
    }


} // namespace


mfxStatus MFXVideoUSER_Register(mfxSession session, mfxU32 type,
                                const mfxPlugin *par)
{
    mfxStatus mfxRes;

    // check error(s)
    MFX_CHECK(session, MFX_ERR_INVALID_HANDLE);
    
    try
    {
        SessionPtr sessionPtr(session, type);
        std::auto_ptr<VideoCodecUSER> & pluginPtr = sessionPtr.plugin();
        std::auto_ptr<VideoENCODE> &encPtr = sessionPtr.codec<VideoENCODE>();
        std::auto_ptr<VideoDECODE> &decPtr = sessionPtr.codec<VideoDECODE>();
        std::auto_ptr<VideoVPP> &vppPtr = sessionPtr.codec<VideoVPP>();
        std::auto_ptr<VideoENC> &preEncPtr = sessionPtr.codec<VideoENC>();

        // the plugin with the same type is already exist
        if (pluginPtr.get() || decPtr.get() || encPtr.get() || preEncPtr.get())
        {
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        }
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
    catch(MFX_CORE_CATCH_TYPE)
    {
        // set the default error value
        mfxRes = MFX_ERR_UNKNOWN;
        if (0 == session)
        {
            mfxRes = MFX_ERR_INVALID_HANDLE;
        }
       /* else if (0 == registeredPlg || 0 == registeredPlg->get())
        {
            mfxRes = MFX_ERR_INVALID_VIDEO_PARAM;
        }*/
        else if (0 == par)
        {
            mfxRes = MFX_ERR_NULL_PTR;
        }
        else if (type > MFX_PLUGINTYPE_VIDEO_ENCODE)
        {
            mfxRes = MFX_ERR_UNDEFINED_BEHAVIOR;
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
        std::auto_ptr<VideoCodecUSER> & pluginPtr = sessionPtr.plugin();

        if (!pluginPtr.get())
        {
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        }

        pluginPtr->GetPlugin(*par);

        mfxRes = MFX_ERR_NONE;
    }
    catch(MFX_CORE_CATCH_TYPE)
    {
        // set the default error value
        mfxRes = MFX_ERR_UNKNOWN;
        if (0 == session)
        {
            mfxRes = MFX_ERR_INVALID_HANDLE;
        }
        else if (0 == par)
        {
            mfxRes = MFX_ERR_NULL_PTR;
        }
        else if (type > MFX_PLUGINTYPE_VIDEO_ENC)
        {
            mfxRes = MFX_ERR_UNDEFINED_BEHAVIOR;
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
        std::auto_ptr<VideoCodecUSER> & registeredPlg = sessionPtr.plugin();
        if (NULL == registeredPlg.get())
            return MFX_ERR_NOT_INITIALIZED;

        // wait until all tasks are processed
        session->m_pScheduler->WaitForTaskCompletion(registeredPlg.get());

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
    catch(MFX_CORE_CATCH_TYPE)
    {
        // set the default error value
        mfxRes = MFX_ERR_UNKNOWN;
        if (0 == session)
        {
            mfxRes = MFX_ERR_INVALID_HANDLE;
        }
        else if (type > MFX_PLUGINTYPE_VIDEO_ENCODE)
        {
            mfxRes = MFX_ERR_UNDEFINED_BEHAVIOR;
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
        std::auto_ptr<VideoCodecUSER> & registeredPlg = SessionPtr(session).plugin();

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
    catch(MFX_CORE_CATCH_TYPE)
    {
        // set the default error value
        mfxRes = MFX_ERR_UNKNOWN;
        if (0 == session)
        {
            return MFX_ERR_INVALID_HANDLE;
        }
        else if (0 == session->m_plgGen.get())
        {
            return MFX_ERR_NOT_INITIALIZED;
        }
        else if (0 == syncp)
        {
            return MFX_ERR_NULL_PTR;
        }
    }

    return mfxRes;

} // mfxStatus MFXVideoUSER_ProcessFrameAsync(mfxSession session,
