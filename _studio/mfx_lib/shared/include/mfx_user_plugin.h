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

#if !defined(__MFX_USER_PLUGIN_H)
#define __MFX_USER_PLUGIN_H

#include <mfxvideo++int.h>

#include <mfxplugin.h>
#include <mfx_task.h>
#include "mfx_enc_ext.h"

class VideoUSERPlugin : public VideoCodecUSER
{
public:
    // Default constructor, codec id != for decoder, and encoder plugins
    VideoUSERPlugin();
    // Destructor
    ~VideoUSERPlugin(void);

    // Initialize the user's plugin
    mfxStatus PluginInit(const mfxPlugin *pParam,
                   mfxSession session,
                   mfxU32 type = MFX_PLUGINTYPE_VIDEO_GENERAL);
    // Release the user's plugin
    mfxStatus PluginClose(void);
    // Get the plugin's threading policy
    mfxTaskThreadingPolicy GetThreadingPolicy(void);

    // Check the parameters to start a new tasl
    mfxStatus Check(const mfxHDL *in, mfxU32 in_num,
                    const mfxHDL *out, mfxU32 out_num,
                    MFX_ENTRY_POINT *pEntryPoint);

    mfxStatus QueryIOSurf(VideoCORE *core, mfxVideoParam *par, mfxFrameAllocRequest *in, mfxFrameAllocRequest *out);
    mfxStatus Query(VideoCORE *core, mfxVideoParam *in, mfxVideoParam *out);
    mfxStatus DecodeHeader(VideoCORE *core, mfxBitstream *bs, mfxVideoParam *par);

    mfxStatus Init(mfxVideoParam *par) ;
    mfxStatus Close(void);
    mfxStatus Reset(mfxVideoParam *par) ;
    mfxStatus GetFrameParam(mfxFrameParam *par) ;
    mfxStatus GetVideoParam(mfxVideoParam *par) ;
    mfxStatus GetEncodeStat(mfxEncodeStat *stat) ;
    mfxStatus GetDecodeStat(mfxDecodeStat *stat) ;
    mfxStatus GetVPPStat(mfxVPPStat *stat) ;
    mfxStatus DecodeFrameCheck(mfxBitstream *bs, mfxFrameSurface1 *surface_work, mfxFrameSurface1 **surface_out, MFX_ENTRY_POINT * ep) ;
    mfxStatus SetSkipMode(mfxSkipMode mode) ;
    mfxStatus GetPayload(mfxU64 *ts, mfxPayload *payload) ;
    
    mfxStatus EncodeFrameCheck(mfxEncodeCtrl *ctrl,
        mfxFrameSurface1 *surface,
        mfxBitstream *bs,
        /*mfxFrameSurface1 **reordered_surface,
        mfxEncodeInternalParams *pInternalParams,*/
        MFX_ENTRY_POINT *pEntryPoint) ;

    mfxStatus VPPFrameCheck(mfxFrameSurface1 *in, mfxFrameSurface1 *out, mfxExtVppAuxData *aux, MFX_ENTRY_POINT *ep) ;
    mfxStatus VPPFrameCheckEx(mfxFrameSurface1 *in, mfxFrameSurface1 *work, mfxFrameSurface1 **out, MFX_ENTRY_POINT *ep) ;
    mfxStatus EncFrameCheck(mfxENCInput *in, mfxENCOutput *out, MFX_ENTRY_POINT *pEntryPoint);

    mfxStatus EncodeFrame(mfxEncodeCtrl *ctrl, mfxEncodeInternalParams *pInternalParams, mfxFrameSurface1 *surface, mfxBitstream *bs) ;
    mfxStatus CancelFrame(mfxEncodeCtrl *ctrl, mfxEncodeInternalParams *pInternalParams, mfxFrameSurface1 *surface, mfxBitstream *bs) ;
    mfxStatus EncFrame(mfxENCInput *in, mfxENCOutput *out);

    //expose new encoder/decoder view
    VideoENCODE* GetEncodePtr();
    VideoDECODE* GetDecodePtr();
    VideoVPP* GetVPPPtr();
    VideoENC* GetEncPtr();

    void GetPlugin(mfxPlugin& plugin);

protected: 
    class VideoENCDECImpl : public VideoENCODE, public VideoDECODE, public VideoVPP,  public VideoENC_Ext
    {
        VideoUSERPlugin *m_plg;
    public:
        VideoENCDECImpl(VideoUSERPlugin * plg)
            : m_plg (plg) {
        }
        mfxStatus Init(mfxVideoParam *par) {return m_plg->Init(par);}
        mfxStatus Reset(mfxVideoParam *par) {return m_plg->Reset(par);}
        mfxStatus Close() {  return m_plg->Close(); }
        mfxTaskThreadingPolicy GetThreadingPolicy(void) {
            return m_plg->GetThreadingPolicy();
        }
        mfxStatus GetVideoParam(mfxVideoParam *par) {
            return m_plg->GetVideoParam(par);
        }
        //encode
        mfxStatus GetFrameParam(mfxFrameParam *par) {
            return m_plg->GetFrameParam(par);
        }
        mfxStatus GetEncodeStat(mfxEncodeStat *stat) {
            return m_plg->GetEncodeStat(stat);
        }
        virtual
            mfxStatus EncodeFrameCheck(mfxEncodeCtrl *ctrl,
            mfxFrameSurface1 *surface,
            mfxBitstream *bs,
            mfxFrameSurface1 ** /*reordered_surface*/,
            mfxEncodeInternalParams * /*pInternalParams*/,
            MFX_ENTRY_POINT *pEntryPoint)
        {
            return m_plg->EncodeFrameCheck(ctrl, surface, bs, pEntryPoint);
        }
        virtual mfxStatus EncodeFrameCheck(mfxEncodeCtrl * /*ctrl*/
            , mfxFrameSurface1 * /*surface*/
            , mfxBitstream * /*bs*/
            , mfxFrameSurface1 ** /*reordered_surface*/
            , mfxEncodeInternalParams * /*pInternalParams*/) {
            return MFX_ERR_NONE;
        }

        virtual
        mfxStatus RunFrameVmeENCCheck(  mfxENCInput *in, 
                                        mfxENCOutput *out,
                                        MFX_ENTRY_POINT* pEntryPoint) 
        {
            return m_plg->EncFrameCheck(in,out,pEntryPoint);
        }


        mfxStatus EncodeFrame(mfxEncodeCtrl *ctrl, mfxEncodeInternalParams *pInternalParams, mfxFrameSurface1 *surface, mfxBitstream *bs) {
            return m_plg->EncodeFrame(ctrl, pInternalParams, surface, bs);
        }
 
        mfxStatus CancelFrame(mfxEncodeCtrl *ctrl, mfxEncodeInternalParams *pInternalParams, mfxFrameSurface1 *surface, mfxBitstream *bs) {
            return m_plg->CancelFrame(ctrl, pInternalParams, surface, bs);
        }
        //decode
        mfxStatus GetDecodeStat(mfxDecodeStat *stat) {return m_plg->GetDecodeStat(stat);}
        mfxStatus DecodeFrameCheck(mfxBitstream *bs,
            mfxFrameSurface1 *surface_work,
            mfxFrameSurface1 **surface_out,
            MFX_ENTRY_POINT *pEntryPoint) {
            return m_plg->DecodeFrameCheck(bs, surface_work, surface_out, pEntryPoint);
        }

        mfxStatus SetSkipMode(mfxSkipMode mode) {return m_plg->SetSkipMode(mode);} 
        mfxStatus GetPayload(mfxU64 *ts, mfxPayload *payload) {return m_plg->GetPayload(ts, payload);}
        //vpp
        mfxStatus GetVPPStat(mfxVPPStat *stat) {
            return m_plg->GetVPPStat(stat);
        }
        virtual
            mfxStatus VPPFrameCheck(mfxFrameSurface1 *in,
            mfxFrameSurface1 *out,
            mfxExtVppAuxData *aux,
            MFX_ENTRY_POINT *ep)
        {
            return m_plg->VPPFrameCheck(in, out, aux, ep);
        }
        virtual
            mfxStatus VPPFrameCheckEx(mfxFrameSurface1 *in,
            mfxFrameSurface1 *work,
            mfxFrameSurface1 **out,
            MFX_ENTRY_POINT *ep)
        {
            return m_plg->VPPFrameCheckEx(in, work, out, ep);
        }
        virtual 
            mfxStatus VppFrameCheck(
            mfxFrameSurface1 *, 
            mfxFrameSurface1 *)
        {
            return MFX_ERR_UNSUPPORTED;
        }
        virtual 
            mfxStatus RunFrameVPP(
            mfxFrameSurface1 *, 
            mfxFrameSurface1 *,
            mfxExtVppAuxData *)
        {
            return MFX_ERR_UNSUPPORTED;
        }
    };

    void Release(void);

    // User's plugin parameters
    mfxPluginParam m_param;
    mfxPlugin m_plugin;

    // Default entry point structure
    MFX_ENTRY_POINT m_entryPoint;
};

#endif // __MFX_USER_PLUGIN_H
