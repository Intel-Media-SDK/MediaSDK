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

#include <mfx_user_plugin.h>
#include <mfx_session.h>

#include <memory.h> // declaration of memset on Windows
#include <string.h> // declaration of memset on Android (and maybe on Linux)

VideoUSERPlugin::VideoUSERPlugin(void)
{
    // reset the structure(s)
    memset(&m_param, 0, sizeof(m_param));
    memset(&m_plugin, 0, sizeof(m_plugin));

    memset(&m_entryPoint, 0, sizeof(m_entryPoint));

} // VideoUSERPlugin::VideoUSERPlugin(void)

VideoUSERPlugin::~VideoUSERPlugin(void)
{
    Release();

} // VideoUSERPlugin::~VideoUSERPlugin(void)

void VideoUSERPlugin::Release(void)
{
    // call 'close' method
    if (m_plugin.PluginClose)
    {
        m_plugin.PluginClose(m_plugin.pthis);
    }

    // reset the structure(s)
    memset(&m_param, 0, sizeof(m_param));
    memset(&m_plugin, 0, sizeof(m_plugin));

    memset(&m_entryPoint, 0, sizeof(m_entryPoint));

} // void VideoUSERPlugin::Release(void)

mfxStatus VideoUSERPlugin::PluginInit(const mfxPlugin *pParam,
                                mfxSession session, mfxU32 type)
{
    mfxStatus mfxRes;

    // check error(s)
    if(MFX_PLUGINTYPE_VIDEO_GENERAL == type)
    {
        if (!pParam ||
            (0 == pParam->PluginInit) ||
            (0 == pParam->PluginClose) ||
            (0 == pParam->GetPluginParam) ||
            (0 == pParam->Submit) ||
            (0 == pParam->Execute) ||
            (0 == pParam->FreeResources))
        {
            return MFX_ERR_NULL_PTR;
        }
    }
    else if(MFX_PLUGINTYPE_VIDEO_DECODE == type)
    {
        if (!pParam ||
            (0 == pParam->PluginInit) ||
            (0 == pParam->PluginClose) ||
            (0 == pParam->GetPluginParam) ||
            (0 == pParam->Execute) ||
            (0 == pParam->FreeResources) ||
            !(pParam->Video) ||
            (0 == pParam->Video->Query) ||
            (0 == pParam->Video->QueryIOSurf) ||
            (0 == pParam->Video->Init) ||
            (0 == pParam->Video->Reset) ||
            (0 == pParam->Video->Close) ||
            (0 == pParam->Video->GetVideoParam) ||
            (0 == pParam->Video->DecodeHeader) ||
            (0 == pParam->Video->GetPayload) ||
            (0 == pParam->Video->DecodeFrameSubmit))
        {
            return MFX_ERR_NULL_PTR;
        }
    }
    else if(MFX_PLUGINTYPE_VIDEO_ENCODE == type)
    {
        if (!pParam ||
            (0 == pParam->PluginInit) ||
            (0 == pParam->PluginClose) ||
            (0 == pParam->GetPluginParam) ||
            (0 == pParam->Execute) ||
            (0 == pParam->FreeResources) ||
            !(pParam->Video) ||
            (0 == pParam->Video->Query) ||
            (0 == pParam->Video->QueryIOSurf) ||
            (0 == pParam->Video->Init) ||
            (0 == pParam->Video->Reset) ||
            (0 == pParam->Video->Close) ||
            (0 == pParam->Video->GetVideoParam) ||
            (0 == pParam->Video->EncodeFrameSubmit))
        {
            return MFX_ERR_NULL_PTR;
        }
    }
    else if(MFX_PLUGINTYPE_VIDEO_ENC == type)
    {
        if (!pParam ||
            (0 == pParam->PluginInit) ||
            (0 == pParam->PluginClose) ||
            (0 == pParam->GetPluginParam) ||
            (0 == pParam->Execute) ||
            (0 == pParam->FreeResources) ||
            !(pParam->Video) ||
            (0 == pParam->Video->Query) ||
            (0 == pParam->Video->QueryIOSurf) ||
            (0 == pParam->Video->Init) ||
            (0 == pParam->Video->Reset) ||
            (0 == pParam->Video->Close) ||
            (0 == pParam->Video->GetVideoParam) ||
            (0 == pParam->Video->ENCFrameSubmit))
        {
            return MFX_ERR_NULL_PTR;
        }
    }
    else if(MFX_PLUGINTYPE_VIDEO_VPP == type)
    {
        if (!pParam ||
            (0 == pParam->PluginInit) ||
            (0 == pParam->PluginClose) ||
            (0 == pParam->GetPluginParam) ||
            (0 == pParam->Execute) ||
            (0 == pParam->FreeResources) ||
            !(pParam->Video) ||
            (0 == pParam->Video->Query) ||
            (0 == pParam->Video->QueryIOSurf) ||
            (0 == pParam->Video->Init) ||
            (0 == pParam->Video->Reset) ||
            (0 == pParam->Video->Close) ||
            (0 == pParam->Video->GetVideoParam) ||
            ((0 == pParam->Video->VPPFrameSubmit) && (0 == pParam->Video->VPPFrameSubmitEx)))
        {
            return MFX_ERR_NULL_PTR;
        }
    }

    // release the object before initialization
    Release();

    // save the parameters
    m_plugin = *pParam;

    // initialize the plugin
    mfxRes = m_plugin.PluginInit(m_plugin.pthis, &(session->m_coreInt));
    if (MFX_ERR_NONE != mfxRes)
    {
        return mfxRes;
    }

    // get the default plugin's parameters
    mfxRes = m_plugin.GetPluginParam(m_plugin.pthis, &m_param);
    if (MFX_ERR_NONE != mfxRes)
    {
        return mfxRes;
    }

    // initialize the default 'entry point' structure
    m_entryPoint.pState = m_plugin.pthis;
    m_entryPoint.pRoutine = m_plugin.Execute;
    m_entryPoint.pCompleteProc = m_plugin.FreeResources;
    m_entryPoint.requiredNumThreads = m_param.MaxThreadNum;

    return MFX_ERR_NONE;

} // mfxStatus VideoUSERPlugin::Init(const mfxPlugin *pParam,

mfxStatus VideoUSERPlugin::PluginClose(void)
{
    Release();

    return MFX_ERR_NONE;

} // mfxStatus VideoUSERPlugin::Close(void)


mfxTaskThreadingPolicy VideoUSERPlugin::GetThreadingPolicy(void)
{
    mfxTaskThreadingPolicy threadingPolicy;

    switch (m_param.ThreadPolicy)
    {
    case MFX_THREADPOLICY_PARALLEL:
        threadingPolicy = MFX_TASK_THREADING_INTER;
        break;
    case  MFX_THREADPOLICY_SERIAL:
        threadingPolicy = m_param.MaxThreadNum > 1 ?  MFX_TASK_THREADING_INTRA : MFX_TASK_THREADING_DEDICATED ;
        break;
    default:
        threadingPolicy = MFX_TASK_THREADING_INTRA;
        break;
    }

    return threadingPolicy;

} // mfxTaskThreadingPolicy VideoUSERPlugin::GetThreadingPolicy(void)

mfxStatus VideoUSERPlugin::Check(const mfxHDL *in, mfxU32 in_num,
                                 const mfxHDL *out, mfxU32 out_num,
                                 MFX_ENTRY_POINT *pEntryPoint)
{
    mfxStatus mfxRes;
    mfxThreadTask userParam;

    // check the parameters with user object
    mfxRes = m_plugin.Submit(m_plugin.pthis,
                             in, in_num,
                             out, out_num,
                             &userParam);
    if (MFX_ERR_NONE != mfxRes)
    {
        return mfxRes;
    }

    // fill the 'entry point' structure
    *pEntryPoint = m_entryPoint;
    pEntryPoint->pParam = userParam;

    return MFX_ERR_NONE;

} // mfxStatus VideoUSERPlugin::Check(const mfxHDL *in, mfxU32 in_num,
#define U32TOFOURCC(mfxu32)\
(char)(mfxu32 & 0xFF), \
(char)((mfxu32 >> 8) & 0xFF),\
(char)((mfxu32 >> 16) & 0xFF),\
(char)((mfxu32 >> 24) & 0xFF)

mfxStatus VideoUSERPlugin::QueryIOSurf(VideoCORE * /*core*/, mfxVideoParam *par, mfxFrameAllocRequest *in, mfxFrameAllocRequest *out)
{
    if (m_param.CodecId != par->mfx.CodecId)
    {
        //printf("ERROR: VideoUSERPlugin::QueryIOSurf, plugin_codec_id=%c%c%c%c, query_codec_id=%c%c%c%c\n", U32TOFOURCC(m_param.CodecId), U32TOFOURCC(par->mfx.CodecId));
        return MFX_ERR_UNSUPPORTED;
    }

    return m_plugin.Video->QueryIOSurf(m_plugin.pthis, par, in, out);
}

mfxStatus VideoUSERPlugin::Query(VideoCORE * /*core*/, mfxVideoParam *in, mfxVideoParam *out)
{
    if (m_param.CodecId != out->mfx.CodecId)
    {
        //printf("ERROR: VideoUSERPlugin::Query, plugin_codec_id=%c%c%c%c, query_codec_id=%c%c%c%c\n", U32TOFOURCC(m_param.CodecId), U32TOFOURCC(out->mfx.CodecId));
        return MFX_ERR_UNSUPPORTED;
    }

    return m_plugin.Video->Query(m_plugin.pthis, in, out);
}

mfxStatus VideoUSERPlugin::DecodeHeader(VideoCORE * /*core*/, mfxBitstream *bs, mfxVideoParam *par)
{
    if (m_param.CodecId != par->mfx.CodecId)
    {
        return MFX_ERR_UNSUPPORTED;
    }

    return m_plugin.Video->DecodeHeader(m_plugin.pthis, bs, par);
}

mfxStatus VideoUSERPlugin::DecodeFrameCheck(mfxBitstream *bs, mfxFrameSurface1 *surface_work, mfxFrameSurface1 **surface_out, MFX_ENTRY_POINT * ep) {

    mfxStatus mfxRes;
    mfxThreadTask userParam;

    // check the parameters with user object
    mfxRes = m_plugin.Video->DecodeFrameSubmit(m_plugin.pthis, bs, surface_work, surface_out,  &userParam);
    if (MFX_ERR_NONE != mfxRes)
    {
        return mfxRes;
    }

    // fill the 'entry point' structure
    *ep = m_entryPoint;
    ep->pParam = userParam;

    return MFX_ERR_NONE;
}

mfxStatus VideoUSERPlugin::EncodeFrameCheck(mfxEncodeCtrl *ctrl, mfxFrameSurface1 *surface, mfxBitstream *bs, MFX_ENTRY_POINT *ep) {
    mfxStatus mfxRes;
    mfxThreadTask userParam;

    // check the parameters with user object
    mfxRes = m_plugin.Video->EncodeFrameSubmit(m_plugin.pthis, ctrl, surface, bs, &userParam);
    if ( mfxRes >= MFX_ERR_NONE  || mfxRes == MFX_ERR_MORE_DATA_SUBMIT_TASK)
    {
        *ep = m_entryPoint;
        ep->pParam = userParam;
    }

    return mfxRes;
}
mfxStatus VideoUSERPlugin::EncFrameCheck(mfxENCInput *in, mfxENCOutput *out, MFX_ENTRY_POINT *ep)
{
    mfxStatus mfxRes;
    mfxThreadTask userParam;

    // check the parameters with user object
    mfxRes = m_plugin.Video->ENCFrameSubmit(m_plugin.pthis, in,out, &userParam);
    if (MFX_ERR_NONE != mfxRes)
    {
        return mfxRes;
    }

    // fill the 'entry point' structure
    *ep = m_entryPoint;
    ep->pParam = userParam;

    return MFX_ERR_NONE;
}

mfxStatus VideoUSERPlugin::VPPFrameCheck(mfxFrameSurface1 *in, mfxFrameSurface1 *out, mfxExtVppAuxData *aux, MFX_ENTRY_POINT *ep){
    mfxStatus mfxRes;
    mfxThreadTask userParam;

    // check the parameters with user object
    if(0 == m_plugin.Video->VPPFrameSubmit)
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    mfxRes =  m_plugin.Video->VPPFrameSubmit(m_plugin.pthis, in, out, aux, &userParam);
    if (!(MFX_ERR_NONE == mfxRes || MFX_ERR_MORE_SURFACE == mfxRes))
    {
        return mfxRes;
    }
    // fill the 'entry point' structure
    *ep = m_entryPoint;
    ep->pParam = userParam;

    return mfxRes;
}

mfxStatus VideoUSERPlugin::VPPFrameCheckEx(mfxFrameSurface1 *in, mfxFrameSurface1 *work, mfxFrameSurface1 **out, MFX_ENTRY_POINT *ep){
    mfxStatus mfxRes;
    mfxThreadTask userParam = 0;

    // check the parameters with user object
    if(0 == m_plugin.Video->VPPFrameSubmitEx)
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    mfxRes =  m_plugin.Video->VPPFrameSubmitEx(m_plugin.pthis, in, work, out, &userParam);
    if (!(MFX_ERR_NONE == mfxRes || MFX_ERR_MORE_SURFACE == mfxRes))
    {
        return mfxRes;
    }
    // fill the 'entry point' structure if required
    if(0 != userParam)
    {
        *ep = m_entryPoint;
        ep->pParam = userParam;
    }

    return mfxRes;
}

mfxStatus VideoUSERPlugin::GetVideoParam(mfxVideoParam *par) {

    return m_plugin.Video->GetVideoParam(m_plugin.pthis, par);
}

mfxStatus VideoUSERPlugin::Init(mfxVideoParam *par) {
    return m_plugin.Video->Init(m_plugin.pthis, par);
}

mfxStatus VideoUSERPlugin::Reset(mfxVideoParam *par) {
    return m_plugin.Video->Reset(m_plugin.pthis, par);
}

mfxStatus VideoUSERPlugin::Close(void) {
    if (m_plugin.pthis) return m_plugin.Video->Close(m_plugin.pthis);
    return MFX_ERR_NONE;
}

mfxStatus VideoUSERPlugin::GetPayload(mfxU64 *ts, mfxPayload *payload) {
    return m_plugin.Video->GetPayload(m_plugin.pthis, ts, payload);
}

#ifdef _MSVC_LANG
#pragma warning (disable: 4100)
#endif

mfxStatus VideoUSERPlugin::GetFrameParam(mfxFrameParam * /* par */) {
    return MFX_ERR_UNSUPPORTED;
}

mfxStatus VideoUSERPlugin::GetEncodeStat(mfxEncodeStat * /* stat */) {
    return MFX_ERR_UNSUPPORTED;
}
mfxStatus VideoUSERPlugin::GetDecodeStat(mfxDecodeStat * /* stat */) {
    return MFX_ERR_UNSUPPORTED;
}
mfxStatus VideoUSERPlugin::GetVPPStat(mfxVPPStat * /* stat */) {
    return MFX_ERR_UNSUPPORTED;
}
mfxStatus VideoUSERPlugin::SetSkipMode(mfxSkipMode /* mode */) {
    return MFX_ERR_UNSUPPORTED;
}

mfxStatus VideoUSERPlugin::EncodeFrame(mfxEncodeCtrl * /* ctrl */, mfxEncodeInternalParams * /* pInternalParams */, mfxFrameSurface1 * /* surface */, mfxBitstream * /* bs */) {
    return MFX_ERR_UNSUPPORTED;
}
mfxStatus VideoUSERPlugin::CancelFrame(mfxEncodeCtrl * /* ctrl */, mfxEncodeInternalParams * /* pInternalParams */, mfxFrameSurface1 * /* surface */, mfxBitstream * /* bs */) {
    return MFX_ERR_UNSUPPORTED;
}
mfxStatus VideoUSERPlugin::EncFrame(mfxENCInput * /* in */, mfxENCOutput * /* out */)
{
    return MFX_ERR_UNSUPPORTED;
}



VideoENCODE* VideoUSERPlugin::GetEncodePtr()
{
    return new VideoENCDECImpl(this);
}

VideoENC* VideoUSERPlugin::GetEncPtr()
{
    return new VideoENCDECImpl(this);
}

VideoDECODE* VideoUSERPlugin::GetDecodePtr()
{
    return new VideoENCDECImpl(this);
}

VideoVPP* VideoUSERPlugin::GetVPPPtr()
{
    return new VideoENCDECImpl(this);
}



void VideoUSERPlugin::GetPlugin(mfxPlugin& plugin)
{
    plugin = m_plugin;
}
