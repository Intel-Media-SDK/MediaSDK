// Copyright (c) 2019-2020 Intel Corporation
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

#include "mfx_enctools.h"
#include <algorithm>
#include <math.h>
#include "mfx_enctools_aenc.h"
#include "limits.h"

#if !defined(ENABLE_ADAPTIVE_ENCODE)

mfxStatus AEncInit(mfxHDL*, AEncParam) { return MFX_ERR_UNSUPPORTED; }
mfxStatus AEncProcessFrame(mfxHDL, mfxU32, mfxU8*, mfxI32, AEncFrame*) { return MFX_ERR_UNSUPPORTED; }
void AEncUpdatePFrameBits(mfxHDL, mfxU32, mfxU32, mfxU32, mfxU32) {}
void AEncClose(mfxHDL) {}

#endif

mfxStatus AEnc_EncTool::Init(mfxEncToolsCtrl const & ctrl, mfxExtEncToolsConfig const & pConfig)
{
    mfxFrameInfo const *frameInfo = &ctrl.FrameInfo;

    m_aencPar.SrcFrameWidth = frameInfo->Width;
    m_aencPar.SrcFrameHeight = frameInfo->Height;

    if (ctrl.IOPattern & MFX_IOPATTERN_IN_SYSTEM_MEMORY)
    {
        FrameWidth_aligned  = frameInfo->Width;
        FrameHeight_aligned = frameInfo->Height;
        m_aencPar.FrameWidth = frameInfo->CropW ? frameInfo->CropW : frameInfo->Width;
        m_aencPar.FrameHeight = frameInfo->CropH ? frameInfo->CropH : frameInfo->Height;
        m_aencPar.Pitch = frameInfo->Width;
    }
    else
    {
        if (DoDownScaling(*frameInfo))
        {
            FrameWidth_aligned = ENC_TOOLS_DS_FRAME_WIDTH;
            FrameHeight_aligned = ENC_TOOLS_DS_FRAME_HEIGHT;
            m_aencPar.FrameWidth = ENC_TOOLS_DS_FRAME_WIDTH;
            m_aencPar.FrameHeight = ENC_TOOLS_DS_FRAME_HEIGHT;
            m_aencPar.Pitch = ENC_TOOLS_DS_FRAME_WIDTH;
        }
        else
        {
            FrameWidth_aligned = frameInfo->Width;
            FrameHeight_aligned = frameInfo->Height;
            m_aencPar.FrameWidth = frameInfo->CropW ? frameInfo->CropW : frameInfo->Width;
            m_aencPar.FrameHeight = frameInfo->CropH ? frameInfo->CropH : frameInfo->Height;
            m_aencPar.Pitch = frameInfo->Width;
        }
    }

    m_aencPar.ColorFormat = MFX_FOURCC_NV12;
    m_aencPar.MaxMiniGopSize = ctrl.MaxGopRefDist;
    m_aencPar.MaxGopSize = m_aencPar.GopPicSize = (ctrl.MaxGopSize!= 0) ? ctrl.MaxGopSize : 65000;
    m_aencPar.MinGopSize = std::max(4u, m_aencPar.MaxGopSize/16); //to prevent close I frames
    m_aencPar.StrictIFrame = IsOff(pConfig.AdaptiveI);
    m_aencPar.MaxIDRDist = ctrl.MaxIDRDist;
    m_aencPar.AGOP = !IsOff(pConfig.AdaptiveB);
    m_aencPar.APQ = (!IsOff(pConfig.AdaptivePyramidQuantP)) || (!IsOff(pConfig.AdaptivePyramidQuantB));
    m_aencPar.AREF = (!IsOff(pConfig.AdaptiveRefP)) || (!IsOff(pConfig.AdaptiveRefB));
    m_aencPar.ALTR = (!IsOff(pConfig.AdaptiveLTR));

    mfxStatus sts = AEncInit(&m_aenc, m_aencPar);
    MFX_CHECK_STS(sts);
    m_bInit = true;
    return sts;
}

mfxStatus AEnc_EncTool::GetInputFrameInfo(mfxFrameInfo &frameInfo)
{
    MFX_CHECK(m_bInit, MFX_ERR_NOT_INITIALIZED);
    frameInfo.FourCC = MFX_FOURCC_NV12;
    frameInfo.Width = (mfxU16)FrameWidth_aligned;
    frameInfo.Height = (mfxU16)FrameHeight_aligned;
    frameInfo.BitDepthLuma = frameInfo.BitDepthChroma = 8;
    frameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    frameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
    return MFX_ERR_NONE;
}

bool AEnc_EncTool::DoDownScaling(mfxFrameInfo const & frameInfo)
{
    if (frameInfo.Width > ENC_TOOLS_DS_FRAME_WIDTH + (ENC_TOOLS_DS_FRAME_WIDTH >> 1) || frameInfo.Height > ENC_TOOLS_DS_FRAME_HEIGHT + (ENC_TOOLS_DS_FRAME_HEIGHT >> 1))
        return true;
    return false;
}

using namespace EncToolsUtils;

mfxStatus AEnc_EncTool::SubmitFrame(mfxFrameSurface1 *surface)
{
    MFX_CHECK_NULL_PTR1(surface);
    MFX_CHECK(m_bInit, MFX_ERR_NOT_INITIALIZED);
    mfxStatus sts = MFX_ERR_NONE;
    mfxU32 wS, hS, pitch;
    mfxU8 *pS0, *pS;

    if (surface->Info.CropH > 0 && surface->Info.CropW > 0)
    {
        wS = surface->Info.CropW;
        hS = surface->Info.CropH;
    }
    else
    {
        wS = surface->Info.Width;
        hS = surface->Info.Height;
    }
    pitch = surface->Data.Pitch;

    pS = pS0 = surface->Data.Y + surface->Info.CropX + surface->Info.CropY * pitch;

    if (wS > m_aencPar.FrameWidth || hS > m_aencPar.FrameHeight)
    {
        if (!m_ptmpFrame)
            m_ptmpFrame = new mfxU8[m_aencPar.FrameWidth * m_aencPar.FrameHeight];

        sts = DownScaleNN(*pS, wS, hS, pitch, *m_ptmpFrame, m_aencPar.FrameWidth, m_aencPar.FrameHeight, m_aencPar.FrameWidth);
        pitch = m_aencPar.FrameWidth;
        pS = m_ptmpFrame;
        MFX_CHECK_STS(sts);
    }

    AEncFrame res;
    sts = AEncProcessFrame(m_aenc, surface->Data.FrameOrder, pS, pitch, &res);
    if (MFX_ERR_NONE != sts)
        return MFX_ERR_MORE_DATA;
    //else
    //    res.print();

    m_outframes.push_back(res);

    return sts;
}


 mfxStatus AEnc_EncTool::FindOutFrame(mfxU32 displayOrder)
{
     struct CompareByDisplayOrder
     {
         mfxU32 m_DispOrder;
         CompareByDisplayOrder(mfxU32 frameOrder) : m_DispOrder(frameOrder) {}
         bool operator ()(AEncFrame const & extframe) const { return extframe.POC == m_DispOrder; }
     };
     {
         auto outframe = std::find_if(m_outframes.begin(), m_outframes.end(), CompareByDisplayOrder(displayOrder));
         if (outframe == m_outframes.end())
         {
             // EOS:output previously submitted frames
             mfxFrameSurface1 emptySrf = {};
             emptySrf.Data.FrameOrder = UINT_MAX;
             SubmitFrame(&emptySrf);
             outframe = std::find_if(m_outframes.begin(), m_outframes.end(), CompareByDisplayOrder(displayOrder));
             if (outframe == m_outframes.end())
                 return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;
         }
         m_frameIt = outframe;
     }
    return MFX_ERR_NONE;
}

mfxStatus AEnc_EncTool::ReportEncResult(mfxU32 displayOrder, mfxEncToolsBRCEncodeResult const & pEncRes)
{
     MFX_CHECK(m_bInit, MFX_ERR_NOT_INITIALIZED);
     mfxStatus sts = FindOutFrame(displayOrder);
     MFX_CHECK_STS(sts);
     //printf("Found SCD_Enctool Reporting Frame %d\n", displayOrder);
     if ((*m_frameIt).Type == MFX_FRAMETYPE_P) {
        //printf("Updating Frame %d %d %d %d\n", displayOrder, pEncRes->CodedFrameSize, pEncRes->QpY, (*m_frameIt).ClassCmplx);
        AEncUpdatePFrameBits(m_aenc, displayOrder, pEncRes.CodedFrameSize*8, pEncRes.QpY, (*m_frameIt).ClassCmplx);
     }
     return MFX_ERR_NONE;
}

mfxStatus AEnc_EncTool::GetSCDecision(mfxU32 displayOrder, mfxEncToolsHintPreEncodeSceneChange *pPreEncSC)
{
    MFX_CHECK(m_bInit, MFX_ERR_NOT_INITIALIZED);

    mfxStatus sts = FindOutFrame(displayOrder);
    MFX_CHECK_STS(sts);
    pPreEncSC->SceneChangeFlag = static_cast<mfxU16>((*m_frameIt).SceneChanged);
    pPreEncSC->RepeatedFrameFlag = static_cast<mfxU16>((*m_frameIt).RepeatedFrame);
    pPreEncSC->TemporalComplexity = static_cast<mfxU16>((*m_frameIt).TemporalComplexity);

    return sts;
}

mfxStatus AEnc_EncTool::GetGOPDecision(mfxU32 displayOrder, mfxEncToolsHintPreEncodeGOP *pPreEncGOP)
{
    MFX_CHECK(m_bInit, MFX_ERR_NOT_INITIALIZED);
    MFX_CHECK_NULL_PTR1(pPreEncGOP);

    mfxStatus sts = FindOutFrame(displayOrder);
    MFX_CHECK_STS(sts);
    mfxU16 miniGOP = (mfxU16)(*m_frameIt).MiniGopSize;
    pPreEncGOP->MiniGopSize = miniGOP;

    switch ((*m_frameIt).Type)
    {
    case MFX_FRAMETYPE_IDR:
        pPreEncGOP->FrameType = MFX_FRAMETYPE_IDR | MFX_FRAMETYPE_I | MFX_FRAMETYPE_REF;
        break;
    case MFX_FRAMETYPE_I:
        pPreEncGOP->FrameType = MFX_FRAMETYPE_I | MFX_FRAMETYPE_REF;
        break;
    case MFX_FRAMETYPE_P:
        pPreEncGOP->FrameType = MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF;
        break;
    case MFX_FRAMETYPE_B:
        pPreEncGOP->FrameType = MFX_FRAMETYPE_B;
        break;
    default:
        pPreEncGOP->FrameType = MFX_FRAMETYPE_UNKNOWN;
    }
   pPreEncGOP->QPDelta = (mfxI16)(*m_frameIt).DeltaQP;

    switch ((mfxI16)(*m_frameIt).ClassAPQ)
    {
    case 1:
        pPreEncGOP->QPModulation = MFX_QP_MODULATION_HIGH;
        break;
    case 2:
        pPreEncGOP->QPModulation = MFX_QP_MODULATION_MEDIUM;
        break;
    case 3:
        pPreEncGOP->QPModulation = MFX_QP_MODULATION_LOW;
        break;
    default:
        pPreEncGOP->QPModulation = MFX_QP_MODULATION_NOT_DEFINED;
    }

   //printf("%d\n", pPreEncGOP->QPDelta);
    return sts;
}


mfxStatus AEnc_EncTool::GetARefDecision(mfxU32 displayOrder, mfxEncToolsHintPreEncodeARefFrames *pPreEncARef)
{
    MFX_CHECK(m_bInit, MFX_ERR_NOT_INITIALIZED);
    MFX_CHECK_NULL_PTR1(pPreEncARef);

    mfxStatus sts = FindOutFrame(displayOrder);
    MFX_CHECK_STS(sts);
    pPreEncARef->CurrFrameType = (mfxU16)((*m_frameIt).LTR ? MFX_REF_FRAME_TYPE_LTR :
        ((*m_frameIt).KeepInDPB ? MFX_REF_FRAME_TYPE_KEY : MFX_REF_FRAME_NORMAL));


    pPreEncARef->RejectedRefListSize = std::min<mfxU16>((mfxU16)(*m_frameIt).RemoveFromDPBSize, 16);
    std::copy((*m_frameIt).RemoveFromDPB, (*m_frameIt).RemoveFromDPB + pPreEncARef->RejectedRefListSize, pPreEncARef->RejectedRefList);

    pPreEncARef->PreferredRefListSize = std::min<mfxU16>((mfxU16)(*m_frameIt).RefListSize, 16);
    std::copy((*m_frameIt).RefList, (*m_frameIt).RefList + pPreEncARef->PreferredRefListSize, pPreEncARef->PreferredRefList);

    return sts;
}



mfxStatus AEnc_EncTool::CompleteFrame(mfxU32 displayOrder)
{
    MFX_CHECK(m_bInit, MFX_ERR_NOT_INITIALIZED);

    mfxStatus sts = FindOutFrame(displayOrder);
    MFX_CHECK_STS(sts);
    m_outframes.erase(m_frameIt);
    return MFX_ERR_NONE;
}


void AEnc_EncTool::Close()
{
    if (m_bInit)
    {
        AEncClose(m_aenc);
        delete m_ptmpFrame;
        m_bInit = false;
    }
}

