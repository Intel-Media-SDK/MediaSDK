// Copyright (c) 2018-2020 Intel Corporation
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

#ifndef __MFX_BRC_COMMON_H__
#define __MFX_BRC_COMMON_H__

#include "mfx_enctools_brc_defs.h"
#include "mfx_common.h"
#ifdef MFX_ENABLE_ENCTOOLS
#include "mfxenctools-int.h"
#else
#include "mfxbrc.h"
#endif
#include <vector>
#include <memory>
#include <algorithm>

#if defined (MFX_ENABLE_H264_VIDEO_ENCODE) || defined (MFX_ENABLE_MPEG2_VIDEO_ENCODE)
#define UMC_ENABLE_VIDEO_BRC
#define MFX_ENABLE_VIDEO_BRC_COMMON

#include "umc_video_brc.h"

mfxStatus ConvertVideoParam_Brc(const mfxVideoParam *parMFX, UMC::VideoBrcParams *parUMC);
#endif

#if defined (MFX_ENABLE_H264_VIDEO_ENCODE) || defined (MFX_ENABLE_H265_VIDEO_ENCODE)


namespace MfxHwH265EncodeBRC
{


class ExtBRC
{
private:
    cBRCParams m_par;
    std::unique_ptr < HRDCodecSpec> m_hrdSpec;

    bool       m_bInit;
    bool       m_bDynamicInit;
    BRC_Ctx    m_ctx;
    std::unique_ptr<AVGBitrate> m_avg;
    mfxU32     m_SkipCount;
    mfxU32     m_ReEncodeCount;

public:
    ExtBRC():
        m_par(),
        m_hrdSpec(),
        m_bInit(false),
        m_bDynamicInit(false),
        m_SkipCount(0),
        m_ReEncodeCount(0)
    {
        memset(&m_ctx, 0, sizeof(m_ctx));

    }
    mfxStatus Init (mfxVideoParam* par);
    mfxStatus Reset(mfxVideoParam* par);
    mfxStatus Close () {
        //printf("\nFrames skipped: %i \n", m_SkipCount);
        //printf("\nNumber of re-encodes: %i \n", m_ReEncodeCount);
        m_bInit = false;
        return MFX_ERR_NONE;
    }
    mfxStatus GetFrameCtrl (mfxBRCFrameParam* par, mfxBRCFrameCtrl* ctrl);
    mfxStatus Update (mfxBRCFrameParam* par, mfxBRCFrameCtrl* ctrl, mfxBRCFrameStatus* status);

protected:
    mfxI32 GetCurQP(mfxU32 type, mfxI32 layer, mfxU16 isRef, mfxU16 clsAPQ);             // Get QP for current frame
    mfxI32 GetSeqQP(mfxI32 qp, mfxU32 type, mfxI32 layer, mfxU16 isRef, mfxU16 clsAPQ);  // Get P-QP from QP of given frame
    mfxI32 GetPicQP(mfxI32 qp, mfxU32 type, mfxI32 layer, mfxU16 isRef, mfxU16 clsAPQ);  // Get QP for given frame from P-QP
    mfxF64 ResetQuantAb(mfxI32 qp, mfxU32 type, mfxI32 layer, mfxU16 isRef, mfxF64 fAbLong, mfxU32 eo, bool bIdr, mfxU16 clsAPQ);
};
}
namespace HEVCExtBRC
{
    inline mfxStatus Init  (mfxHDL pthis, mfxVideoParam* par)
    {
        MFX_CHECK_NULL_PTR1(pthis);
        return ((MfxHwH265EncodeBRC::ExtBRC*)pthis)->Init(par) ;
    }
    inline mfxStatus Reset (mfxHDL pthis, mfxVideoParam* par)
    {
        MFX_CHECK_NULL_PTR1(pthis);
        return ((MfxHwH265EncodeBRC::ExtBRC*)pthis)->Reset(par) ;
    }
    inline mfxStatus Close (mfxHDL pthis)
    {
        MFX_CHECK_NULL_PTR1(pthis);
        return ((MfxHwH265EncodeBRC::ExtBRC*)pthis)->Close() ;
    }
    inline mfxStatus GetFrameCtrl (mfxHDL pthis, mfxBRCFrameParam* par, mfxBRCFrameCtrl* ctrl)
    {
       MFX_CHECK_NULL_PTR1(pthis);
       return ((MfxHwH265EncodeBRC::ExtBRC*)pthis)->GetFrameCtrl(par,ctrl) ;
    }
    inline mfxStatus Update       (mfxHDL pthis, mfxBRCFrameParam* par, mfxBRCFrameCtrl* ctrl, mfxBRCFrameStatus* status)
    {
        MFX_CHECK_NULL_PTR1(pthis);
        return ((MfxHwH265EncodeBRC::ExtBRC*)pthis)->Update(par,ctrl, status) ;
    }
    inline mfxStatus Create(mfxExtBRC & m_BRC)
    {
        MFX_CHECK(m_BRC.pthis == NULL, MFX_ERR_UNDEFINED_BEHAVIOR);
        m_BRC.pthis = new MfxHwH265EncodeBRC::ExtBRC;
        m_BRC.Init = Init;
        m_BRC.Reset = Reset;
        m_BRC.Close = Close;
        m_BRC.GetFrameCtrl = GetFrameCtrl;
        m_BRC.Update = Update;
        return MFX_ERR_NONE;
    }
    inline mfxStatus Destroy(mfxExtBRC & m_BRC)
    {

        MFX_CHECK(m_BRC.pthis != NULL, MFX_ERR_NONE);
        delete (MfxHwH265EncodeBRC::ExtBRC*)m_BRC.pthis;

        m_BRC.pthis = 0;
        m_BRC.Init = 0;
        m_BRC.Reset = 0;
        m_BRC.Close = 0;
        m_BRC.GetFrameCtrl = 0;
        m_BRC.Update = 0;
        return MFX_ERR_NONE;
    }
}

#endif
#endif
