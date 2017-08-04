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

#ifndef __MFX_MPEG2_ENCODE_HW_H__
#define __MFX_MPEG2_ENCODE_HW_H__

#include "mfx_common.h"


#if defined (MFX_ENABLE_MPEG2_VIDEO_ENCODE)

#include "mfxvideo++int.h"
#include "mfx_mpeg2_encode_full_hw.h"
#include "mfx_mpeg2_encode_utils_hw.h"


class MFXVideoENCODEMPEG2_HW : public VideoENCODE {
protected:

public:
    static mfxStatus Query(VideoCORE *core, mfxVideoParam *in, mfxVideoParam *out)
    {   
        return MPEG2EncoderHW::ControllerBase::Query(core,in,out);
    }
    static mfxStatus QueryIOSurf(VideoCORE *core, mfxVideoParam *par, mfxFrameAllocRequest *request)
    {
        return MPEG2EncoderHW::ControllerBase::QueryIOSurf(core,par,request);
    }
    MFXVideoENCODEMPEG2_HW(VideoCORE *core, mfxStatus *sts)
    {
        m_pCore = core;
        pEncoder = 0;
        *sts = MFX_ERR_NONE;
    }
    virtual ~MFXVideoENCODEMPEG2_HW() {Close();}
    virtual mfxStatus Init(mfxVideoParam *par)
    {
        mfxStatus     sts  = MFX_ERR_NONE;
        ENCODE_CAPS   Caps = {};
        MPEG2EncoderHW::HW_MODE  mode = MPEG2EncoderHW::UNSUPPORTED;
        if (pEncoder)
        {
            return MFX_ERR_UNDEFINED_BEHAVIOR;        
        }
        sts = MPEG2EncoderHW::CheckHwCaps(m_pCore, par, 0, &Caps);
        MFX_CHECK_STS(sts);

        mode = MPEG2EncoderHW::GetHwEncodeMode(Caps);
        if (mode == MPEG2EncoderHW::FULL_ENCODE)
        {
            pEncoder = new MPEG2EncoderHW::FullEncode(m_pCore,&sts);        
        }
        else
        {
            return MFX_WRN_PARTIAL_ACCELERATION;
        }
        sts = pEncoder->Init(par);
        if (sts == MFX_WRN_PARTIAL_ACCELERATION || sts < MFX_ERR_NONE)
        {
            Close();
            return sts;
        }
        return sts;
    }
    virtual mfxStatus Reset(mfxVideoParam *par)
    {
        if (!pEncoder)
        {
            return MFX_ERR_NOT_INITIALIZED;        
        }
        return pEncoder->Reset(par);
    }
    virtual mfxStatus Close(void)
    {
        mfxStatus sts = MFX_ERR_NONE;
        if (pEncoder)
        {
            sts = pEncoder->Close();
            delete pEncoder;
            pEncoder = 0;        
        }
        return sts;   
    }
    virtual mfxStatus GetVideoParam(mfxVideoParam *par)
    {
        if (!pEncoder)
        {
            return MFX_ERR_NOT_INITIALIZED;        
        }
        return pEncoder->GetVideoParam(par);    
    }
    virtual mfxStatus GetFrameParam(mfxFrameParam *)
    {
        return MFX_ERR_UNSUPPORTED;
    }
    virtual mfxStatus GetEncodeStat(mfxEncodeStat *stat)
    {
        if (!pEncoder)
        {
            return MFX_ERR_NOT_INITIALIZED;        
        }
        return pEncoder->GetEncodeStat(stat);       
    }
    virtual mfxStatus EncodeFrameCheck(mfxEncodeCtrl *ctrl, 
        mfxFrameSurface1 *surface, 
        mfxBitstream *bs, mfxFrameSurface1 **reordered_surface, 
        mfxEncodeInternalParams *pInternalParams)
    {
        if (!pEncoder)
        {
            return MFX_ERR_NOT_INITIALIZED;        
        }
        return pEncoder->EncodeFrameCheck(ctrl,surface,bs,reordered_surface,pInternalParams);     
    }
    virtual mfxStatus EncodeFrame(mfxEncodeCtrl *ctrl, 
        mfxEncodeInternalParams *pInternalParams, 
        mfxFrameSurface1 *surface, 
        mfxBitstream *bs)
    {
        if (!pEncoder)
        {
            return MFX_ERR_NOT_INITIALIZED;        
        }
        return pEncoder->EncodeFrame(ctrl,pInternalParams,surface,bs);     
    }

    virtual mfxStatus CancelFrame(mfxEncodeCtrl *ctrl, 
        mfxEncodeInternalParams *pInternalParams, 
        mfxFrameSurface1 *surface, 
        mfxBitstream *bs)
    {
        if (!pEncoder)
        {
            return MFX_ERR_NOT_INITIALIZED;        
        }
        return pEncoder->CancelFrame(ctrl,pInternalParams,surface,bs);     
    }
    virtual
    mfxStatus EncodeFrameCheck(mfxEncodeCtrl *ctrl,
                               mfxFrameSurface1 *surface,
                               mfxBitstream *bs,
                               mfxFrameSurface1 **reordered_surface,
                               mfxEncodeInternalParams *pInternalParams,
                               MFX_ENTRY_POINT pEntryPoints[],
                               mfxU32 &numEntryPoints)
    {
        if (!pEncoder)
        {
            return MFX_ERR_NOT_INITIALIZED;        
        }
        return pEncoder->EncodeFrameCheck(ctrl,surface,bs,reordered_surface,pInternalParams,pEntryPoints,numEntryPoints); 
    }
    virtual
    mfxTaskThreadingPolicy GetThreadingPolicy(void) 
    {
        if (!pEncoder)
        {
            return MFX_TASK_THREADING_DEFAULT;        
        }
        return pEncoder->GetThreadingPolicy();
    }
protected:


private:
    VideoCORE*                      m_pCore;
    MPEG2EncoderHW::EncoderBase*    pEncoder;

};

#endif // MFX_ENABLE_MPEG2_VIDEO_ENCODE
#endif
