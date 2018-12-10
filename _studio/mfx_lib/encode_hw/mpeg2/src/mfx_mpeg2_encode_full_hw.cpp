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

#include "mfx_common.h"

#if defined (MFX_ENABLE_MPEG2_VIDEO_ENCODE)
#include "mfx_mpeg2_encode_full_hw.h"
#include "mfx_task.h"
#include "mfx_enc_common.h"

using namespace MPEG2EncoderHW;

#define HLEN 22
#define AVBR_WA true


static 
mfxStatus RunSeqHeader(mfxVideoParam *par, mfxU8* pBuffer, mfxU16 maxLen, mfxU16 &len)
{
    MFX_CHECK_NULL_PTR2(par, pBuffer);
    MFX_CHECK(maxLen - len >= HLEN,MFX_ERR_NOT_ENOUGH_BUFFER);
    MFX_CHECK(par->mfx.FrameInfo.ChromaFormat == MFX_CHROMAFORMAT_YUV420,MFX_ERR_UNSUPPORTED);

    mfxU8 *pHeader = pBuffer + len;
    mfxU32  h = (par->mfx.FrameInfo.CropH!=0)? par->mfx.FrameInfo.CropH:par->mfx.FrameInfo.Height;
    mfxU32  w = (par->mfx.FrameInfo.CropW!=0)? par->mfx.FrameInfo.CropW:par->mfx.FrameInfo.Width;

    mfxI32 fr_code = 0, fr_codeN = 0, fr_codeD = 0;
    mfxU32 aw = (par->mfx.FrameInfo.AspectRatioW != 0) ? par->mfx.FrameInfo.AspectRatioW * w :w;
    mfxU32 ah = (par->mfx.FrameInfo.AspectRatioH != 0) ? par->mfx.FrameInfo.AspectRatioH * h :h;

    mfxU16 ar_code = GetAspectRatioCode (aw, ah);
    ConvertFrameRateMPEG2(par->mfx.FrameInfo.FrameRateExtD, par->mfx.FrameInfo.FrameRateExtN, fr_code, fr_codeN, fr_codeD);
    
    mfxU32 bitrate      = 0;
    mfxU32 buffer_size  = 0;

    if (par->mfx.RateControlMethod != MFX_RATECONTROL_CQP) 
    {
        bitrate      = (par->mfx.TargetKbps * 5 + 1)>>1;
        buffer_size  =  par->mfx.BufferSizeInKB>>1;   
    }
    int32_t  prog_seq = (par->mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_PROGRESSIVE) ? 1 : 0;
    int32_t  chroma_format_code = 1;

    uint32_t profile = 4;
    uint32_t level = 8;

    switch (par->mfx.CodecProfile)
    {
    case MFX_PROFILE_MPEG2_SIMPLE:
        profile = 5;     break;
    case MFX_PROFILE_MPEG2_MAIN:
        profile = 4;     break;
    case MFX_PROFILE_MPEG2_HIGH:
        profile = 1;     break;
    }
    switch (par->mfx.CodecLevel)
    {
    case MFX_LEVEL_MPEG2_LOW:
        level = 10;   break;
    case MFX_LEVEL_MPEG2_MAIN:
        level = 8;    break;
    case MFX_LEVEL_MPEG2_HIGH:
        level = 4;    break;
    case MFX_LEVEL_MPEG2_HIGH1440:
        level = 6;    break;
    }
    
    memset (pHeader, 0, HLEN*sizeof(mfxU8));
    
    pHeader[2] = 0x01;
    pHeader[3] = 0xB3;

    pHeader[4] = mfxU8  ((w >> 4) & 0xFF);
    pHeader[5] = mfxU8  (((w & 0x0F) << 4)|((h >> 8)&0x0F));
    pHeader[6] = mfxU8  (h & 0xFF);
    pHeader[7] = mfxU8  (((ar_code << 4) & 0xF0)|(fr_code&0x0F));
    pHeader[8] = mfxU8  ((bitrate >> 10) & 0xFF);
    pHeader[9] = mfxU8  ((bitrate >> 2)  & 0xFF);
    pHeader[10] = mfxU8 (((bitrate & 0x03) <<6)|(1<<5)| ((buffer_size >> 5) & 0x1F));
    pHeader[11] = mfxU8 ((buffer_size & 0x1F)<<3);

    //SequenceExtension

    pHeader[14] = 0x01;
    pHeader[15] = 0xB5;   
    pHeader[16] = mfxU8 ((1<<4)| (profile & 0x0F));
    pHeader[17] = mfxU8 ((level << 4)| (prog_seq <<3) | (chroma_format_code << 1) | ((w >>13) & 0x01));
    pHeader[18] = mfxU8 (((w >>(12 - 7)) & 0x80) | ((h>>(12 - 5)) & 0x60 )| ((bitrate >> 18)& 0x1F));
    pHeader[19] = mfxU8 (((bitrate >>(18 + 5 - 1))&0xFE)|0x01);
    pHeader[20] = mfxU8 ((buffer_size >> 11) &0xFF);

    pHeader[21] = mfxU8 (((fr_codeN & 0x03) << 5) | (fr_codeD & 0x1F));

    len = len +  HLEN;

    return MFX_ERR_NONE;
}
#undef HLEN

FullEncode::FullEncode(VideoCORE *core, mfxStatus *sts)
{
    m_pCore = core;
    m_pController = 0;

    m_pENCODE  = 0;

    m_pFrameStore = 0;
    m_pFrameTasks = 0;
    m_nFrameTasks = 0;

    m_pBRC = 0;
    m_nCurrTask   = 0;
    m_pExtTasks    = 0;
    m_runtimeErr = MFX_ERR_NONE;

    *sts = (core ? MFX_ERR_NONE : MFX_ERR_NULL_PTR);
}

mfxStatus FullEncode::Query(VideoCORE * core, mfxVideoParam *in, mfxVideoParam *out)
{
    return ControllerBase::Query(core,in, out, AVBR_WA);
}

mfxStatus FullEncode::QueryIOSurf(VideoCORE * core, mfxVideoParam *par, mfxFrameAllocRequest *request)
{
    return ControllerBase::QueryIOSurf(core,par,request);
}

mfxStatus FullEncode::Init(mfxVideoParam *par)
{
   MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_API, "FullEncode::Init");
    mfxStatus sts  = MFX_ERR_NONE;
    mfxStatus sts1 = MFX_ERR_NONE;

    m_runtimeErr = MFX_ERR_NONE;

    if (is_initialized())
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    m_pController = new ControllerBase(m_pCore, AVBR_WA);

    sts = m_pController->Reset(par, false);

    if (sts == MFX_WRN_PARTIAL_ACCELERATION || sts < MFX_ERR_NONE)
    {
        Close();
        return sts;
    }
    sts1 = ResetImpl();
    if (MFX_ERR_NONE != sts1)
    {
        Close();
        return sts1;  
    }

    return sts;
}

mfxStatus FullEncode::Reset(mfxVideoParam *par)
{
    mfxStatus sts  = MFX_ERR_NONE;
    mfxStatus sts1 = MFX_ERR_NONE;

    MFX_CHECK(is_initialized(), MFX_ERR_NOT_INITIALIZED);

    sts = m_pController->Reset(par, false);

    if (sts == MFX_WRN_PARTIAL_ACCELERATION || sts < MFX_ERR_NONE)
    {
        return sts;
    }
    sts1 = ResetImpl();
    MFX_CHECK_STS(sts1);

    return sts;
}

mfxStatus FullEncode::ResetImpl()
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "FullEncode::ResetImpl");
    mfxStatus sts = MFX_ERR_NONE;
    mfxVideoParamEx_MPEG2* paramsEx = m_pController->getVideoParamsEx();

    m_nCurrTask     = 0;

    if (!m_pFrameTasks)
    {
        m_nFrameTasks = paramsEx->mfxVideoParams.mfx.GopRefDist + paramsEx->mfxVideoParams.AsyncDepth;
        m_pFrameTasks = new EncodeFrameTask[m_nFrameTasks]; 
    }

    for (mfxU32 i=0; i<m_nFrameTasks; i++)
    {
        sts = m_pFrameTasks[i].Reset(m_pCore);
        MFX_CHECK_STS(sts);
    }
    if (!m_pFrameStore)
    {
        m_pFrameStore = new FrameStore(m_pCore);
    }

    sts = m_pFrameStore->Reset(m_pController->isRawFrames(),
                               m_pController->GetInputFrameType(),true,
                               m_nFrameTasks,&paramsEx->mfxVideoParams.mfx.FrameInfo,paramsEx->mfxVideoParams.Protected!=0);
    MFX_CHECK_STS(sts);  

    paramsEx->pRecFramesResponse_hw = m_pFrameStore->GetFrameAllocResponse();
    
    if (!m_pBRC)
    {
        m_pBRC = new MPEG2BRC_HW (m_pCore);
        sts = m_pBRC->Init(&paramsEx->mfxVideoParams);
        MFX_CHECK_STS(sts);  
    }
    else
    {
        sts = m_pBRC->Reset(&paramsEx->mfxVideoParams);
        MFX_CHECK_STS(sts);  
    }

    if (!m_pENCODE)
    {
        m_pENCODE = new MFXVideoENCODEMPEG2_HW_DDI(m_pCore,&sts);
        sts = m_pENCODE->Init(paramsEx);
        MFX_CHECK_STS(sts);
    }
    else
    {
        sts = m_pENCODE->Reset(paramsEx);
        MFX_CHECK_STS(sts);
    }

    if (!m_pExtTasks)
    {
        m_pExtTasks = new clExtTasks2;
    }
    m_UDBuff.Reset(&paramsEx->mfxVideoParams);
    return sts;
}

//virtual mfxStatus Close(void); // same name
mfxStatus FullEncode::Close(void)
{

    if(!is_initialized())
        return MFX_ERR_NOT_INITIALIZED;

    if (m_pController)
    {
        m_pController->Close();
        delete m_pController;
        m_pController = 0; 
    }
    if (m_pENCODE)
    {
        m_pENCODE->Close();
        delete m_pENCODE;
        m_pENCODE = 0;  
    }
    if (m_pFrameTasks)
    {
        delete [] m_pFrameTasks;  
        m_nFrameTasks = 0;
    }
    if (m_pFrameStore)
    {
        delete m_pFrameStore;
        m_pFrameStore = 0; 
    }
    if (m_pExtTasks)
    {
        delete m_pExtTasks;
        m_pExtTasks = 0;
    }
    if(m_pBRC)
    {
        m_pBRC->Close();
        delete m_pBRC;
    }
    m_UDBuff.Close();

    return MFX_ERR_NONE;
}

mfxStatus FullEncode::GetVideoParam(mfxVideoParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;

    if(!is_initialized())
        return MFX_ERR_NOT_INITIALIZED;

    sts = m_pController->GetVideoParam(par);
    MFX_CHECK_STS(sts);

    if (mfxExtCodingOptionSPSPPS* ext = MPEG2EncoderHW::GetExtCodingOptionsSPSPPS(par->ExtParam, par->NumExtParam))
    {
        mfxU16 len = 0;
        sts = RunSeqHeader(par, ext->SPSBuffer, ext->SPSBufSize,len);
        MFX_CHECK_STS(sts);
        ext->SPSBufSize = len;
        ext->PPSBufSize = 0; // pps is n/a for mpeg2
        ext->SPSId      = 0;
        ext->PPSId      = 0;
    }
    return MFX_ERR_NONE;
}

mfxStatus FullEncode::GetFrameParam(mfxFrameParam *par)
{
    MFX_CHECK_NULL_PTR1(par)

        return MFX_ERR_UNSUPPORTED ;
}

mfxStatus FullEncode::GetEncodeStat(mfxEncodeStat *stat)
{
    if(!is_initialized())
        return MFX_ERR_NOT_INITIALIZED;

    return  m_pController->GetEncodeStat(stat);
}


mfxStatus FullEncode::EncodeFrameCheck(
                                       mfxEncodeCtrl *ctrl,
                                       mfxFrameSurface1 *surface,
                                       mfxBitstream *bs,
                                       mfxFrameSurface1 **reordered_surface,
                                       mfxEncodeInternalParams *pInternalParams)
{
    if(!is_initialized())
        return MFX_ERR_NOT_INITIALIZED;

    if (m_runtimeErr)
        return m_runtimeErr;


    return  m_pController->EncodeFrameCheck(ctrl,surface,bs,reordered_surface,pInternalParams);
}
mfxStatus FullEncode::CancelFrame(mfxEncodeCtrl * /*ctrl*/, mfxEncodeInternalParams * /*pInternalParams*/, mfxFrameSurface1 *surface, mfxBitstream * /*bs*/)
{
    MFX_CHECK_NULL_PTR1(surface)
        m_pController->FinishFrame(0);
    return m_pCore->DecreaseReference(&surface->Data);
}

// Async algorithm
mfxStatus FullEncode::SubmitFrame(sExtTask2 *pExtTask)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_API, "FullEncode::SubmitFrame");
    mfxStatus         sts           = MFX_ERR_NONE;

    if(!is_initialized())
        return MFX_ERR_NOT_INITIALIZED;

    mfxEncodeInternalParams *   pInputInternalParams = &pExtTask->m_inputInternalParams;
    mfxFrameSurface1 *          input_surface = pExtTask->m_pInput_surface;
    mfxBitstream *              bs = pExtTask->m_pBs;
    mfxFrameSurface1 *          surface = 0;
    FramesSet                   Frames; 
    mfxVideoParamEx_MPEG2*      pParams     = m_pController->getVideoParamsEx();
    bool                        bConstQuant = (pParams->mfxVideoParams.mfx.RateControlMethod == MFX_RATECONTROL_CQP);

    MFX_CHECK_NULL_PTR1(bs);

    EncodeFrameTask*  pIntTask = GetIntTask(pExtTask->m_nInternalTask);
    MFX_CHECK(pIntTask, MFX_ERR_UNDEFINED_BEHAVIOR);

    mfxEncodeInternalParams* pInternalParams = &pIntTask->m_sEncodeInternalParams;
    if (input_surface)
    {
        sts = m_pController->CheckFrameType(pInputInternalParams);
        MFX_CHECK_STS(sts);
    }
    if ((sts = m_pController->ReorderFrame(pInputInternalParams,m_pController->GetOriginalSurface(input_surface),pInternalParams,&surface)) == MFX_ERR_MORE_DATA)
    {

        return MFX_ERR_NONE;
    }
    MFX_CHECK_STS(sts);

    sts = m_pFrameStore->NextFrame( surface,
                                    pInternalParams->FrameOrder,
                                    pInternalParams->FrameType,
                                    pInternalParams->InternalFlags,
                                    &Frames);

    MFX_CHECK_STS(sts);

    pIntTask->SetFrames(&Frames, pInternalParams);
    pIntTask->m_pBitstream = bs;

    bs->FrameType = pInternalParams->FrameType;
    bs->TimeStamp = pIntTask->m_Frames.m_pInputFrame->Data.TimeStamp;
    bs->DecodeTimeStamp = (pInternalParams->FrameType & MFX_FRAMETYPE_B)
        ? CalcDTSForNonRefFrameMpeg2(bs->TimeStamp)
        : CalcDTSForRefFrameMpeg2(bs->TimeStamp,
        Frames.m_nFrame - Frames.m_nRefFrame[0],
        pParams->mfxVideoParams.mfx.GopRefDist,
        CalculateUMCFramerate(pParams->mfxVideoParams.mfx.FrameInfo.FrameRateExtN, pParams->mfxVideoParams.mfx.FrameInfo.FrameRateExtD));

    sts = pIntTask->FillFrameParams((mfxU8)pInternalParams->FrameType,
                                        m_pController->getVideoParamsEx(),
                                        surface->Info.PicStruct,
                                        (pInternalParams->InternalFlags & MFX_IFLAG_BWD_ONLY) ? 1:0,
                                        (pInternalParams->InternalFlags & MFX_IFLAG_FWD_ONLY) ? 1:0,
                                        (pInternalParams->InternalFlags & MFX_IFLAG_ADD_HEADER) ? 1:0,
                                        (pInternalParams->InternalFlags & MFX_IFLAG_ADD_EOS) ? 1:0);
    MFX_CHECK_STS(sts);

    sts = m_pCore->DecreaseReference(&surface->Data);
    MFX_CHECK_STS(sts);

   // coding
    {
        mfxU8  qp = 0;
        mfxU8 * mbqpdata = 0;
        mfxU32 mbqpNumMB = 0;
        if (bConstQuant)
        {
            m_pBRC->SetQuant(pInternalParams->QP, pInternalParams->FrameType);
            m_pBRC->SetQuantDCPredAndDelay(&pIntTask->m_FrameParams,&qp);

            if (pParams->bMbqpMode)
            {
                const mfxExtMBQP *mbqp = (mfxExtMBQP *)GetExtBuffer(pInternalParams->ExtParam, pInternalParams->NumExtParam, MFX_EXTBUFF_MBQP);

                mfxU32 wMB = (pParams->mfxVideoParams.mfx.FrameInfo.CropW + 15) / 16;
                mfxU32 hMB = (pParams->mfxVideoParams.mfx.FrameInfo.CropH + 15) / 16;

                bool isMBQP = mbqp && mbqp->QP && mbqp->NumQPAlloc >= wMB * hMB;

                if (isMBQP)
                {
                    mbqpdata = mbqp->QP;
                    mbqpNumMB = wMB * hMB;
                }
            }
        }


        MFX_CHECK_STS (m_UDBuff.AddUserData(pInternalParams));

        if (mbqpdata)
        {
            sts = m_pENCODE->SubmitFrameMBQP(pIntTask, m_UDBuff.GetUserDataBuffer(), m_UDBuff.GetUserDataSize(), mbqpdata, mbqpNumMB, qp);
        }
        else
        {
            sts = m_pENCODE->SubmitFrame(pIntTask, m_UDBuff.GetUserDataBuffer(), m_UDBuff.GetUserDataSize(), qp);
        }

        MFX_CHECK_STS(sts);
    }

    return MFX_ERR_NONE;
}
mfxStatus FullEncode::QueryFrame(sExtTask2 *pExtTask)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_API, "FullEncode::QueryFrame");
    mfxStatus         sts       = MFX_ERR_NONE;
    mfxU32            dataLen   = 0;
    EncodeFrameTask*  pIntTask  = 0;
    mfxBitstream *    bs        = 0; 


    if(!is_initialized())
        return MFX_ERR_NOT_INITIALIZED;

    pIntTask = GetIntTask(pExtTask->m_nInternalTask);
    MFX_CHECK_NULL_PTR1(pIntTask);

    if (pIntTask->m_pBitstream == 0)
    {
        return MFX_ERR_NONE;
    }

    bs = pIntTask->m_pBitstream;

    dataLen = bs->DataLength;

    sts = m_pENCODE->QueryFrame(pIntTask);

    if (sts != MFX_WRN_DEVICE_BUSY)
    {
        UMC::AutomaticUMCMutex lock(m_guard);
        pIntTask->m_taskStatus = NOT_STARTED;
        pIntTask->m_Frames.ReleaseFrames(m_pCore);
        m_pController->FinishFrame(bs->DataLength - dataLen);
    }

    if (sts < 0)
        m_runtimeErr = sts;

    return sts;
}

static
mfxStatus TaskRoutineSubmit(void *pState, void *param, mfxU32 /*n*/, mfxU32 /*callNumber*/)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_API, "TaskRoutineSubmit");

    mfxStatus sts   = MFX_ERR_NONE;
    FullEncode* th  = (FullEncode*)pState;
    sExtTask2 *pExtTask = (sExtTask2 *)param;

    sts = th->m_pExtTasks->CheckTaskForSubmit(pExtTask);
    MFX_CHECK_STS(sts);   

    sts = th->SubmitFrame(pExtTask);
    MFX_CHECK_STS(sts);  


    return MFX_TASK_DONE;
}
static
mfxStatus TaskRoutineQuery(void *pState, void *param, mfxU32 /*n*/, mfxU32 /*callNumber*/)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_API, "TaskRoutineQuery");

    mfxStatus sts   = MFX_ERR_NONE;
    FullEncode* th  = (FullEncode*)pState;

    sExtTask2 *pExtTask = (sExtTask2 *)param;
    sExtTask2 *pCurrTask = th->m_pExtTasks->GetTaskForQuery();
    
    MFX_CHECK(pExtTask == pCurrTask, MFX_ERR_UNDEFINED_BEHAVIOR);

    sts = th->QueryFrame(pExtTask);
    MFX_CHECK (sts != MFX_WRN_DEVICE_BUSY, MFX_TASK_BUSY);
    MFX_CHECK_STS (th->m_pExtTasks->NextFrame());
    MFX_CHECK_STS(sts);
        
    return MFX_TASK_DONE;
}
    

mfxStatus FullEncode::EncodeFrameCheck(
                                       mfxEncodeCtrl *           ctrl,
                                       mfxFrameSurface1 *        surface,
                                       mfxBitstream *            bs,
                                       mfxFrameSurface1 **       reordered_surface,
                                       mfxEncodeInternalParams * internalParams,
                                       MFX_ENTRY_POINT           entryPoints[],
                                       mfxU32 &                  numEntryPoints)
{
    mfxStatus      sts_ret  = MFX_ERR_NONE;
    mfxStatus      sts      = MFX_ERR_NONE;
    sExtTask2     *pExtTask = 0;

    if (surface)
    {
        MFX_CHECK((surface->Data.Y == 0 && surface->Data.MemId == 0) || !m_pController->isOpaq(), MFX_ERR_UNDEFINED_BEHAVIOR);
    }

    mfxFrameSurface1 *pOriginalSurface = m_pController->GetOriginalSurface(surface);

    if (pOriginalSurface != surface)
    {
        if (pOriginalSurface == 0 || surface == 0)
            return MFX_ERR_UNDEFINED_BEHAVIOR;

        pOriginalSurface->Info = surface->Info;
        pOriginalSurface->Data.Corrupted = surface->Data.Corrupted;
        pOriginalSurface->Data.DataFlag = surface->Data.DataFlag;
        pOriginalSurface->Data.TimeStamp = surface->Data.TimeStamp;
        pOriginalSurface->Data.FrameOrder = surface->Data.FrameOrder;
    }

    // check availability of an internal task in sync part
    mfxU32 nIntTask = GetFreeIntTask();
    MFX_CHECK(nIntTask != 0, MFX_WRN_DEVICE_BUSY);

    EncodeFrameTask* pIntTask = GetIntTask(nIntTask);
    MFX_CHECK(pIntTask, MFX_WRN_DEVICE_BUSY);

    sts_ret = EncodeFrameCheck(ctrl, pOriginalSurface, bs, reordered_surface, internalParams);

    if (sts_ret != MFX_ERR_NONE && sts_ret !=(mfxStatus)MFX_ERR_MORE_DATA_SUBMIT_TASK && sts_ret<0)
        return sts_ret;


    sts = m_pExtTasks->AddTask( internalParams,*reordered_surface, bs, &pExtTask);
    MFX_CHECK_STS(sts);

    pExtTask->m_nInternalTask = nIntTask;
    {
        UMC::AutomaticUMCMutex lock(m_guard);
        pIntTask->m_taskStatus = ENC_STARTED;
    }

    entryPoints[0].pState = this;
    entryPoints[0].pParam = pExtTask;
    entryPoints[0].pRoutine = TaskRoutineSubmit;
    entryPoints[0].pCompleteProc = 0;
    entryPoints[0].pGetSubTaskProc = 0;
    entryPoints[0].pCompleteSubTaskProc = 0;
    entryPoints[0].requiredNumThreads = 1;

    entryPoints[1].pState = this;
    entryPoints[1].pParam = pExtTask;
    entryPoints[1].pRoutine = TaskRoutineQuery;
    entryPoints[1].pCompleteProc = 0;
    entryPoints[1].pGetSubTaskProc = 0;
    entryPoints[1].pCompleteSubTaskProc = 0;
    entryPoints[1].requiredNumThreads = 1;

    numEntryPoints = 2;

    return sts_ret;
}

mfxStatus UserDataBuffer::AddUserData(mfxU8* pUserData, mfxU32 len)
{
    mfxU8* pCurr = pUserData;
    bool bFirstUDSCode = false;
    bool bProhibitedSymbols = false;
    mfxU32 size = 0;
    // analyze user data 
    while (pCurr < pUserData + len - 2)
    {
        if (pCurr[0] == 0 && pCurr[1] == 0 &&  (pCurr[2] & 0xFE) == 0)
        {
            if (pCurr < (pUserData + len - 4) && pCurr[2]== 1 && pCurr[3] == 0xB2)
            {
                //user data start code
                if (pCurr == pUserData) bFirstUDSCode = true; 
                pCurr += 3;                    
            }
            else
            {
                bProhibitedSymbols = true;
                break;                    
            }         

        }
        pCurr ++;  
    } 
    size = bProhibitedSymbols ? static_cast<mfxU32>(pCurr - pUserData) : len ;            
    // copy into buffer
    if (size > 0)
    {
        pCurr = m_pBuffer + m_dataSize;
        m_dataSize += size + (bFirstUDSCode ? 0 : 4) ;

        MFX_CHECK( m_dataSize < m_bufSize, MFX_ERR_UNDEFINED_BEHAVIOR);                
        if (!bFirstUDSCode)
        {
            *(pCurr++) = 0; 
            *(pCurr++) = 0; 
            *(pCurr++) = 1;
            *(pCurr++) = 0xB2;           
        }
        std::copy(pUserData, pUserData + std::min(m_bufSize - m_dataSize, size), pCurr);
    }
    return MFX_ERR_NONE;
}






#endif // MFX_ENABLE_MPEG2_VIDEO_ENCODE
