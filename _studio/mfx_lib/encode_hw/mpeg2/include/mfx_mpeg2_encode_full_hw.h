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

#ifndef __MFX_MPEG2_ENCODE_HW_FULL_H__
#define __MFX_MPEG2_ENCODE_HW_FULL_H__

#include "mfx_common.h"


#if defined (MFX_ENABLE_MPEG2_VIDEO_ENCODE)

#include <umc_mutex.h>

#include "mfx_mpeg2_encode_interface.h"
#include "mfxvideo++int.h"
#include "mfx_mpeg2_encode_utils_hw.h"

//#define BRC_WA

namespace MPEG2EncoderHW
{
    class MFXVideoENCODEMPEG2_HW_DDI
    {
    private:
        VideoCORE*                              m_core;
        MfxHwMpeg2Encode::ExecuteBuffers*       m_pExecuteBuffers;
        MfxHwMpeg2Encode::DriverEncoder*        m_pDdiEncoder;
        bool                                    m_bStage2Ready;
        bool                                    m_bHWInput;


    public:
        MFXVideoENCODEMPEG2_HW_DDI(VideoCORE *core, mfxStatus *sts)
            : m_core(core)
            , m_pExecuteBuffers()
            , m_pDdiEncoder()
            , m_bStage2Ready()
            , m_bHWInput()
          {
              *sts = MFX_ERR_NONE;
          }
          virtual ~MFXVideoENCODEMPEG2_HW_DDI ()
          {
              Close();
          }
          mfxStatus Close()
          {
              if (m_pExecuteBuffers)
              {
                  m_pExecuteBuffers->Close();
                  delete m_pExecuteBuffers;
                  m_pExecuteBuffers = 0;
              }
              if (m_pDdiEncoder)
              {
                  m_pDdiEncoder->Close();
                  delete m_pDdiEncoder;
                  m_pDdiEncoder = 0;
              }
              return MFX_ERR_NONE;
          }  
          mfxStatus Reset(mfxVideoParamEx_MPEG2 *par)
          {
              mfxStatus sts = MFX_ERR_NONE;
              if (!m_pExecuteBuffers)
              {
                  m_pExecuteBuffers = new MfxHwMpeg2Encode::ExecuteBuffers;
              }
              else
              {
                  m_pExecuteBuffers->Close();
              }
              sts = m_pExecuteBuffers->Init(par,ENCODE_ENC_PAK_ID,true);
              MFX_CHECK_STS(sts);        

              if (!m_pDdiEncoder)
              {
                  m_pDdiEncoder = MfxHwMpeg2Encode::CreatePlatformMpeg2Encoder(m_core);
                  MFX_CHECK_NULL_PTR1(m_pDdiEncoder);
              }
              else
              {
                  m_pDdiEncoder->Close();       
              }            
              sts = m_pDdiEncoder->Init(m_pExecuteBuffers,
                  par->pRecFramesResponse_hw->NumFrameActual,
                  ENCODE_ENC_PAK_ID);
              MFX_CHECK_STS(sts);

              sts = m_pDdiEncoder->RegisterRefFrames(par->pRecFramesResponse_hw);
              MFX_CHECK_STS(sts);

              // in VAAPI encoder context have to be created after reference frames
              sts = m_pDdiEncoder->CreateContext(m_pExecuteBuffers,
                  par->pRecFramesResponse_hw->NumFrameActual,
                  ENCODE_ENC_PAK_ID);
              MFX_CHECK_STS(sts);

              m_bStage2Ready = false;
              m_bHWInput = (par->mfxVideoParams.IOPattern & MFX_IOPATTERN_IN_VIDEO_MEMORY) != 0;

              return sts;

          }
          mfxStatus Init(mfxVideoParamEx_MPEG2 *par)
          {
              return Reset(par);
          }  
         mfxStatus SubmitFrame (EncodeFrameTask*  pIntTask, mfxU8 *pUserData, mfxU32 userDataLen, mfxU8 qp = 0)
         {
             if (m_pExecuteBuffers->m_mbqp_data)
             {
                 m_pExecuteBuffers->m_mbqp_data[0] = 0;
             }
             m_pExecuteBuffers->m_SkipFrame = (mfxU8)pIntTask->m_sEncodeInternalParams.SkipFrame;

             mfxStatus sts = MFX_ERR_NONE;
             sts = SubmitFrame(&pIntTask->m_FrameParams, &pIntTask->m_Frames, pUserData, userDataLen, qp);
             MFX_CHECK_STS (sts);
             pIntTask->m_FeedbackNumber       = m_pExecuteBuffers->m_pps.StatusReportFeedbackNumber;
             pIntTask->m_BitstreamFrameNumber = (mfxU32)m_pExecuteBuffers->m_idxBs;
             return sts;
         }

         static void QuantIntoScaleTypeAndCode (int32_t quant_value, int32_t &q_scale_type, int32_t &quantiser_scale_code)
         {
             if(quant_value > 7 && quant_value <= 62) 
             {
                 q_scale_type = 0;
                 quantiser_scale_code = (quant_value + 1) >> 1;
             } 
             else 
             { // non-linear quantizer
                 q_scale_type = 1;
                 if(quant_value <= 8) 
                 {
                     quantiser_scale_code = quant_value;
                 }
                 else if (quant_value > 62) 
                 {
                     quantiser_scale_code = 25+((quant_value-64+4)>>3);
                 }
             }
             if(quantiser_scale_code < 1) 
             {
                 quantiser_scale_code = 1;
             }
             if(quantiser_scale_code > 31) 
             {
                 quantiser_scale_code = 31;
             } 
         }

         mfxStatus SubmitFrameMBQP (EncodeFrameTask*  pIntTask, mfxU8 *pUserData, mfxU32 userDataLen, mfxU8* mbqp, mfxU32 numMB, mfxU8 qp)
         {
             mfxStatus sts = MFX_ERR_NONE;

//             int32_t scale_type = 0; 
//             int32_t scale_code = 0;
//             QuantIntoScaleTypeAndCode(qp, scale_type, scale_code);
//             pIntTask->m_FrameParams.QuantScaleType = (uint8_t)scale_type;
             m_pExecuteBuffers->m_SkipFrame = (mfxU8)pIntTask->m_sEncodeInternalParams.SkipFrame;

             sts = SubmitFrame(&pIntTask->m_FrameParams, &pIntTask->m_Frames, pUserData, userDataLen, qp, mbqp, numMB);
             MFX_CHECK_STS (sts);
             pIntTask->m_FeedbackNumber       = m_pExecuteBuffers->m_pps.StatusReportFeedbackNumber;
             pIntTask->m_BitstreamFrameNumber = (mfxU32)m_pExecuteBuffers->m_idxBs;
             return sts;
         }

 protected:
     mfxStatus SubmitFrame(mfxFrameParamMPEG2*  pParams, FramesSet* pFrames, mfxU8 *pUserData, mfxU32 userDataLen, mfxU8 qp = 0, mfxU8* mbqp = 0, mfxU32 numMB = 0)
          {
              mfxStatus sts = MFX_ERR_NONE;
              if (!m_pExecuteBuffers || !m_pDdiEncoder)
              {
                  return MFX_ERR_NOT_INITIALIZED;
              }

              sts = m_pExecuteBuffers->InitPictureParameters(pParams,pFrames->m_nFrame);
              MFX_CHECK_STS(sts);

              sts = m_pExecuteBuffers->InitSliceParameters(qp, pParams->QuantScaleType, mbqp, numMB);
              MFX_CHECK_STS(sts);

              m_pExecuteBuffers->InitFramesSet (pFrames->m_pInputFrame->Data.MemId, 
                  m_bHWInput , 
                  (pFrames->m_pRecFrame)? pFrames->m_pRecFrame->Data.MemId:0, 
                  (pFrames->m_pRefFrame[0])? pFrames->m_pRefFrame[0]->Data.MemId:0,
                  (pFrames->m_pRefFrame[1])? pFrames->m_pRefFrame[1]->Data.MemId:0);
              MFX_CHECK_STS(sts);

              sts = m_pDdiEncoder->SetFrames(m_pExecuteBuffers);
              MFX_CHECK_STS(sts);

              sts = m_pDdiEncoder->Execute(m_pExecuteBuffers, pUserData,userDataLen);
              MFX_CHECK_STS(sts);
              m_bStage2Ready = true;

              return sts;
          }
public:
          mfxStatus QueryFrame(EncodeFrameTask*  pIntTask)
          {
              mfxStatus sts = MFX_ERR_NONE;
              if (!m_pExecuteBuffers || !m_pDdiEncoder)
              {
                  return MFX_ERR_NOT_INITIALIZED;
              }

              sts = m_pDdiEncoder->FillBSBuffer(pIntTask->m_FeedbackNumber, pIntTask->m_BitstreamFrameNumber,pIntTask->m_pBitstream, &m_pExecuteBuffers->m_encrypt);
              MFX_CHECK_STS(sts);


              return sts;
          }

          static inline mfxU16 GetIOPattern ()
          {
              return MFX_IOPATTERN_IN_VIDEO_MEMORY;
          }

    private:
        // Declare private copy constructor to avoid accidental assignment
        // and klocwork complaining.
        MFXVideoENCODEMPEG2_HW_DDI(const MFXVideoENCODEMPEG2_HW_DDI &);
        MFXVideoENCODEMPEG2_HW_DDI & operator = (const MFXVideoENCODEMPEG2_HW_DDI &);
    };
    class UserDataBuffer
    {
    private:
        mfxU8* m_pBuffer;
        mfxU32 m_bufSize;
        mfxU32 m_dataSize;    
    
    protected:
        mfxStatus AddUserData(mfxU8* pUserData, mfxU32 len);
        UserDataBuffer(const UserDataBuffer &);
        void operator=(const UserDataBuffer &);

    public:
        UserDataBuffer(): m_pBuffer(0), m_bufSize (0), m_dataSize (0) {}
        virtual ~UserDataBuffer() { Close();}
        inline void Reset(mfxVideoParam *par) 
        {
            if (!m_pBuffer)
            {
                m_bufSize =  par->mfx.FrameInfo.Width*par->mfx.FrameInfo.Height * 3;   
                m_pBuffer = new mfxU8 [m_bufSize];
            }
            m_dataSize   = 0;            
        }
        inline void Close()
        {
            if (m_pBuffer)
            {
                delete [] m_pBuffer;
                m_pBuffer = 0;            
            }   
            m_dataSize = m_bufSize = 0;
        }
        inline mfxStatus AddUserData(mfxEncodeInternalParams* pIntParams)
        {
            m_dataSize = 0;

            MFX_CHECK(pIntParams != 0 && pIntParams->NumPayload != 0 && pIntParams->Payload != 0, MFX_ERR_NONE);

            for (mfxI32 i = 0; i < pIntParams->NumPayload; i++)
            {
                if (pIntParams->Payload[i] && 
                    pIntParams->Payload[i]->Type == 0x1B2L && 
                    pIntParams->Payload[i]->NumBit &&
                    pIntParams->Payload[i]->Data)
                {
                    MFX_CHECK_STS (AddUserData(pIntParams->Payload[i]->Data, (pIntParams->Payload[i]->NumBit+7)>>3));                
                }   
            }
            return MFX_ERR_NONE;
        } 
        inline mfxU8* GetUserDataBuffer() {return m_dataSize ? m_pBuffer : 0;}
        inline mfxI32 GetUserDataSize() {return m_dataSize;}
    };

 

    class FullEncode: public EncoderBase
    {
    public:

        static mfxStatus Query(VideoCORE *core, mfxVideoParam *in, mfxVideoParam *out);
        static mfxStatus QueryIOSurf(VideoCORE *core, mfxVideoParam *par, mfxFrameAllocRequest *request);

        FullEncode(VideoCORE *core, mfxStatus *sts);
        virtual ~FullEncode() {Close();}

        virtual mfxStatus Init(mfxVideoParam *par);
        virtual mfxStatus Reset(mfxVideoParam *par);
        virtual mfxStatus Close(void);

        virtual mfxStatus GetVideoParam(mfxVideoParam *par);
        virtual mfxStatus GetFrameParam(mfxFrameParam *par);
        virtual mfxStatus GetEncodeStat(mfxEncodeStat *stat);

        virtual mfxStatus EncodeFrameCheck(mfxEncodeCtrl *ctrl, 
            mfxFrameSurface1 *surface, 
            mfxBitstream *bs, 
            mfxFrameSurface1 **reordered_surface, 
            mfxEncodeInternalParams *pInternalParams);


        virtual mfxStatus CancelFrame(mfxEncodeCtrl *ctrl, 
            mfxEncodeInternalParams *pInternalParams, 
            mfxFrameSurface1 *surface, 
            mfxBitstream *bs);


        virtual mfxStatus EncodeFrameCheck(
            mfxEncodeCtrl *           ctrl,
            mfxFrameSurface1 *        surface,
            mfxBitstream *            bs,
            mfxFrameSurface1 **       reordered_surface,
            mfxEncodeInternalParams * internalParams,
            MFX_ENTRY_POINT           entryPoints[],
            mfxU32 &                  numEntryPoints);

        virtual mfxStatus EncodeFrameCheck(mfxEncodeCtrl *,
            mfxFrameSurface1 *,
            mfxBitstream *,
            mfxFrameSurface1 **,
            mfxEncodeInternalParams *,
            MFX_ENTRY_POINT *) 
        {return MFX_ERR_UNSUPPORTED;} 

        virtual mfxStatus EncodeFrame(mfxEncodeCtrl *, 
            mfxEncodeInternalParams *, 
            mfxFrameSurface1 *, mfxBitstream *)
        {return MFX_ERR_UNSUPPORTED;} 

        virtual mfxStatus SubmitFrame(sExtTask2 *pTask);
        virtual mfxStatus QueryFrame (sExtTask2 *pTask);

        clExtTasks2*                m_pExtTasks;


        virtual
        mfxTaskThreadingPolicy GetThreadingPolicy(void) {return MFX_TASK_THREADING_INTRA;}
     


    protected:
        inline bool is_initialized() {return m_pController!=0;}
        mfxStatus ResetImpl();


        mfxU32  GetFreeIntTask() 
        {
            UMC::AutomaticUMCMutex lock(m_guard);
            for (mfxU32 i = 0; i< m_nFrameTasks; i++)
            {
                if (m_pFrameTasks[i].m_taskStatus == NOT_STARTED)
                {
                    return i+1;
                }
            }
            return 0;
        }
        EncodeFrameTask*  GetIntTask(mfxU32 num) 
        {
            if (num <= m_nFrameTasks && num > 0)
            {
                return &(m_pFrameTasks[num - 1]);
            }
            return 0;
        }


    private:
        UMC::Mutex                       m_guard;

        VideoCORE *                      m_pCore;
        ControllerBase*                  m_pController;
        MFXVideoENCODEMPEG2_HW_DDI*      m_pENCODE;  
        MPEG2BRC_HW*                     m_pBRC;

        FrameStore*                      m_pFrameStore;
        EncodeFrameTask*                 m_pFrameTasks;
        mfxU32                           m_nFrameTasks;
        mfxU32                           m_nCurrTask;
        UserDataBuffer                   m_UDBuff;
        mfxStatus                        m_runtimeErr;
    };
}

#endif // MFX_ENABLE_MPEG2_VIDEO_ENCODE
#endif
