// Copyright (c) 2017-2020 Intel Corporation
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

#ifndef _MFX_FRAMES_H_
#define _MFX_FRAMES_H_

#include <memory.h>
#include <mutex>
#include "mfxdefs.h"
#include "mfx_common_int.h"

#define NUM_TASKS 256

    /*------- Task  between sync/async encoder parts (1 entry point in async part)-------------------------*/

    struct sExtTask1
    {
      mfxEncodeInternalParams   m_inputInternalParams;
      mfxFrameSurface1          *m_pInput_surface;
      mfxBitstream              *m_pBs;

      sExtTask1()
          : m_inputInternalParams()
      {
          m_pInput_surface = 0;
          m_pBs = 0;
      }
      virtual ~sExtTask1()
      {
      }
    };

    /*------- Task  between sync/async encoder parts (2 entry points in async part: submit and query)------------*/

    struct sExtTask2 : public sExtTask1
    {
        mfxU32            m_nInternalTask;

        sExtTask2(): 
            sExtTask1(), 
            m_nInternalTask(0)  
        {
        }
        virtual ~sExtTask2()
        {
        }
    };

    /*------- Tasks between sync/async encoder parts (1 entry point in async part)-------------------------*/

    class clExtTasks1
    {
    public:
        clExtTasks1()
        {
           m_numTasks          = 0;
           m_currTask          = 0;
           m_maxTasks          = NUM_TASKS;
           m_pTasks            = new sExtTask1[m_maxTasks];    
        }
        virtual ~clExtTasks1()
        {
            if (m_pTasks)
            {
                delete [] m_pTasks;
            }
        }
      
        mfxStatus AddTask ( mfxEncodeInternalParams *pParams,
                            mfxFrameSurface1 *input_surface, 
                            mfxBitstream *bs, 
                            sExtTask1 **pOutTask)
        {
            std::lock_guard<std::mutex> guard(m_mGuard);
            if (m_numTasks < m_maxTasks)
            {
                sExtTask1 *pTask = &m_pTasks[(m_currTask + m_numTasks)% m_maxTasks];
                if (pParams)
                {
                    pTask->m_inputInternalParams = *pParams;
                }
                pTask->m_pInput_surface = input_surface;
                pTask->m_pBs = bs;
                *pOutTask = pTask;
                m_numTasks ++;
                return MFX_ERR_NONE;
            }
            return MFX_WRN_IN_EXECUTION;    
        }
        mfxStatus CheckTask (sExtTask1 *pInputTask)
        {
            mfxStatus sts = MFX_ERR_MORE_DATA;
            std::lock_guard<std::mutex> guard(m_mGuard);
            if (m_numTasks > 0)
            {
                /* task for current frame or task for the same frame */
                if (pInputTask == &m_pTasks[m_currTask])
                {
                    sts = MFX_ERR_NONE;
                }
                else
                {
                    m_currTask = (m_currTask + 1)% m_maxTasks;
                    m_numTasks = m_numTasks - 1;
                    if (m_numTasks > 0 && pInputTask == &m_pTasks[m_currTask])
                    {
                        sts = MFX_ERR_NONE;                
                    }          
                }         
            }
            return sts;    
        }

    private:
        sExtTask1* m_pTasks;
        std::mutex      m_mGuard;

        mfxU32          m_maxTasks;
        mfxU32          m_numTasks;
        mfxU32          m_currTask;
        
      
        // Declare private copy constructor to avoid accidental assignment
        // and klocwork complaining.

        clExtTasks1(const clExtTasks1 &);
        clExtTasks1 & operator = (const clExtTasks1 &);
    };
    
    /*------- Tasks between sync/async encoder parts (2 entry points in async part: submit and query)------------*/

    class clExtTasks2
    {
    public:
        clExtTasks2()
        {
           m_numTasks          = 0;
           m_currTask          = 0;
           m_maxTasks          = NUM_TASKS;
           m_pTasks            = new sExtTask2[m_maxTasks];
           m_numSubmittedTasks = 0;
        }
        virtual ~clExtTasks2()
        {
            if (m_pTasks)
            {
                delete [] m_pTasks;
            }
        }

        mfxStatus AddTask ( mfxEncodeInternalParams *pParams,
                            mfxFrameSurface1 *input_surface, 
                            mfxBitstream *bs, 
                            sExtTask2 **pOutTask)
        {
            std::lock_guard<std::mutex> guard(m_mGuard);
            if (m_numTasks < m_maxTasks)
            {
                sExtTask2 *pTask = &m_pTasks[(m_currTask + m_numTasks)% m_maxTasks];
                if (pParams)
                {
                    pTask->m_inputInternalParams = *pParams;
                }
                pTask->m_pInput_surface = input_surface;
                pTask->m_pBs = bs;
                pTask->m_nInternalTask = 0;

                *pOutTask = pTask;
                m_numTasks ++;
                return MFX_ERR_NONE;
            }
            return MFX_WRN_IN_EXECUTION;    
        } 

        mfxStatus CheckTaskForSubmit(sExtTask2 *pInputTask)
        {
            mfxStatus sts = MFX_ERR_MORE_DATA;
            std::lock_guard<std::mutex> guard(m_mGuard);
            if (m_numTasks >= m_numSubmittedTasks)
            {
                if (pInputTask == m_pTasks + ((m_currTask + m_numSubmittedTasks - 1)% m_maxTasks))
                {
                    sts = MFX_ERR_NONE;
                }
                else if (m_numTasks > m_numSubmittedTasks)
                {
                    m_numSubmittedTasks ++;
                    mfxU32 nTask = (m_currTask + m_numSubmittedTasks - 1)% m_maxTasks;
                    sts  = ( pInputTask == &m_pTasks[nTask]) ? MFX_ERR_NONE : MFX_ERR_UNDEFINED_BEHAVIOR;
                }         
            }
            return sts;    
        }
        sExtTask2* GetTaskForQuery ()
        {
            std::lock_guard<std::mutex> guard(m_mGuard);
            sExtTask2* task =  (m_numTasks > 0 && m_numSubmittedTasks > 0) ? &m_pTasks[m_currTask] : 0;
            return task;
        }
        mfxStatus NextFrame ()
        {
            mfxStatus sts = MFX_ERR_UNDEFINED_BEHAVIOR;
            std::lock_guard<std::mutex> guard(m_mGuard);
            if (m_numTasks > 0 && m_numSubmittedTasks > 0)
            {
                m_currTask  = (m_currTask + 1)% m_maxTasks;
                m_numTasks  = m_numTasks - 1;
                m_numSubmittedTasks = m_numSubmittedTasks - 1;
                sts = MFX_ERR_NONE;
            }
            return sts;
        }
       

    private:

        sExtTask2* m_pTasks;
        std::mutex      m_mGuard;

        mfxU32          m_maxTasks;
        mfxU32          m_numTasks;
        mfxU32          m_currTask;
        mfxU32          m_numSubmittedTasks;

        // Declare private copy constructor to avoid accidental assignment
        // and klocwork complaining.

        clExtTasks2(const clExtTasks2 &);
        clExtTasks2 & operator = (const clExtTasks2 &);
    };

    struct sFrameEx
    {
        mfxFrameSurface1*       m_pFrame;
        mfxU16                  m_FrameType;
        mfxU32                  m_FrameOrder;
        mfxEncodeInternalParams m_sEncodeInternalParams;
        bool                    m_bAddHeader;
        bool                    m_bAddEOS;
        bool                    m_bOnlyBwdPrediction;
        bool                    m_bOnlyFwdPrediction;
    };

    inline void GetFrameTypeMpeg2 (mfxI32 FrameOrder, mfxI32 GOPSize, mfxI32 IPDist, bool bClosedGOP, mfxU16* FrameType)
    {
        mfxI32 pos = (GOPSize)? (FrameOrder %(GOPSize)):FrameOrder;

        if (pos == 0 || IPDist == 0 || (*FrameType & MFX_FRAMETYPE_IDR) != 0)
        {
            *FrameType = MFX_FRAMETYPE_I | MFX_FRAMETYPE_REF | MFX_FRAMETYPE_IDR;
        }
        else
        {
            pos = (bClosedGOP && pos == GOPSize-1)? 0 : pos % (IPDist);
            *FrameType  = ((pos != 0) && ((*FrameType & MFX_FRAMETYPE_REF) == 0)) ?  (mfxU16)MFX_FRAMETYPE_B : (mfxU16)(MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF);
        }
    }
    inline mfxStatus ReleasePlane(mfxFrameSurface1* pFrame)
    {
        if (pFrame)
        {
            //pFrame->Data.Locked= 0;
        }

        return MFX_ERR_NONE;
    }

    inline bool isReferenceFrame(mfxU16 FrameType)
    {
        return ((FrameType & MFX_FRAMETYPE_I) ||
                (FrameType & MFX_FRAMETYPE_P) ||
                (FrameType & MFX_FRAMETYPE_REF));
    }
    inline bool isIntraFrame(mfxU16 FrameType)
    {
        return (FrameType & MFX_FRAMETYPE_I);
    }

    inline bool isReferenceFrame(mfxU16 FrameType, mfxU16 FrameType2)
    {
        if(!FrameType2)
        {
            return ((FrameType & MFX_FRAMETYPE_I) ||
                    (FrameType & MFX_FRAMETYPE_P) ||
                    (FrameType & MFX_FRAMETYPE_REF));

        }
        else
        {
            return (((FrameType & MFX_FRAMETYPE_I) ||
                    (FrameType & MFX_FRAMETYPE_P) ||
                    (FrameType & MFX_FRAMETYPE_REF))
                    &&
                   ((FrameType2 & MFX_FRAMETYPE_I) ||
                    (FrameType2 & MFX_FRAMETYPE_P) ||
                    (FrameType2 & MFX_FRAMETYPE_REF)) );
        }
    }

    inline bool isPredictedFrame(mfxU16 FrameType)
    {
         return (FrameType & MFX_FRAMETYPE_P) != 0;
    }

    inline bool isPredictedFrame(mfxU16 FrameType, mfxU16 FrameType2)
    {
         return ((FrameType & MFX_FRAMETYPE_P) || (FrameType2 & MFX_FRAMETYPE_P)) != 0;
    }

    inline bool isBPredictedFrame(mfxU16 FrameType)
    {
         return (FrameType & MFX_FRAMETYPE_B) != 0;
    }

    inline bool isBPredictedFrame(mfxU16 FrameType, mfxU16 FrameType2)
    {
        return ((FrameType & MFX_FRAMETYPE_B) || (FrameType2 & MFX_FRAMETYPE_B)) != 0;
    }


    class MFXGOP
    {
    public:
        MFXGOP():
          m_maxB(0),
          m_pFrames(0),
          m_numBuffB(0),
          m_iCurr(0),
          m_bClosedGOP(0),
          m_nGOPsInSeq(0),
          m_bAddEOS(0),
          m_bEncodedOrder(0),
          m_nGOPs (0)
          {};

        virtual ~MFXGOP()
        {
            Close();
        }

        inline mfxU32 GetNumberOfB()
        {
            return m_numBuffB;
        }
    protected:
        mfxI32               m_maxB;         
        sFrameEx           * m_pFrames; /*  0     - forward  reference frame,
                                            1     - backward reference frame
                                            other - B frames*/

        mfxI32               m_numBuffB;
        mfxI32               m_iCurr;

        bool                 m_bClosedGOP;
        mfxI32               m_nGOPsInSeq;
        bool                 m_bAddEOS;
        bool                 m_bEncodedOrder;

        mfxI32               m_nGOPs;


    protected:


        inline bool NewGOP ()
        {
            mfxStatus       MFXSts = MFX_ERR_NONE;

            MFXSts = ReleasePlane(m_pFrames[0].m_pFrame);
            if(MFXSts != MFX_ERR_NONE)
                return false;

            m_pFrames[0]   = m_pFrames[1];

            memset (&m_pFrames[1], 0, sizeof(sFrameEx));

            m_iCurr     = 1;
            m_numBuffB  = 0;

            return true;
        }

        inline virtual bool AddBFrame(sFrameEx* pFrameEx)
        {
            if (m_numBuffB < m_maxB && m_pFrames[0].m_pFrame != 0 && ((m_bEncodedOrder && m_pFrames[1].m_pFrame != 0)|| (!m_bEncodedOrder && m_pFrames[1].m_pFrame == 0)))
            {
                m_pFrames[m_numBuffB+2] = *pFrameEx;
                m_numBuffB ++;
                return true;
            }
            return false;
        }  

        inline virtual bool AddReferenceFrame(sFrameEx* pFrameEx)
        {
            if (m_pFrames[0].m_pFrame && m_pFrames[1].m_pFrame ) return false;

            if (isIntraFrame(pFrameEx->m_FrameType))
            {
                if (!m_nGOPs)
                {
                    pFrameEx->m_bAddHeader = true;
                }
                m_nGOPs ++;
                if (m_nGOPsInSeq!=0 && m_nGOPs >= m_nGOPsInSeq)
                {
                    m_nGOPs = 0;
                }
            }
            if (m_pFrames[0].m_pFrame == 0)
            {
                m_pFrames[0]= *pFrameEx;
                return true;
            }
            else if (m_pFrames[1].m_pFrame == 0)
            {
                m_pFrames[1]= *pFrameEx;
                return true;
            }
            return false;
        }

    public:

        inline bool AddFrame(sFrameEx* pFrameEx)
        {
            if (isReferenceFrame(pFrameEx->m_FrameType))
            {
               if (m_iCurr>= m_numBuffB+2 && m_bEncodedOrder)
               {
                    NewGOP();
               }
               return AddReferenceFrame(pFrameEx);
            }
            else
            {
                return AddBFrame(pFrameEx);
            }
        }

        inline virtual mfxStatus Init(mfxU32 maxB, bool bClosedGOP, mfxI32 nGOPsInSeq, bool bAddEOS, bool bEncodedOrder)
        {
            Close();

            m_maxB         = maxB;
 
            m_pFrames = new sFrameEx [m_maxB+2];
            
            Reset(bClosedGOP,nGOPsInSeq,bAddEOS, bEncodedOrder);
            return MFX_ERR_NONE;
        }
        inline mfxI32 GetMaxBFrames ()
        {
            return m_maxB;
        }

        inline virtual void Reset(bool bClosedGOP,mfxI32 nGOPsInSeq, bool bAddEOS,  bool bEncodedOrder)
        {
            if (m_pFrames)
            {
                memset(m_pFrames,0,sizeof(sFrameEx)*(m_maxB+2));
            }

            m_iCurr = 0;
            m_numBuffB = 0;
            m_nGOPs    = 0;
            
            m_bClosedGOP   = bClosedGOP;
            m_nGOPsInSeq   = nGOPsInSeq;
            m_bAddEOS      = bAddEOS;
            m_bEncodedOrder = bEncodedOrder;

        };
        inline bool GetFrameExForDecoding(sFrameEx* pFrame, bool bNextRefIntra, bool bNextB, bool bLast)
        {
            if (m_iCurr < m_numBuffB + 2 && m_pFrames[m_iCurr].m_pFrame)
            {
                *pFrame = m_pFrames[m_iCurr];
                if ((m_iCurr > 1) && isIntraFrame(m_pFrames[1].m_FrameType) && (m_bClosedGOP))
                {
                    pFrame->m_bOnlyBwdPrediction = true;
                }
                if (m_bAddEOS && ((bNextRefIntra && m_nGOPs == 0) || bLast) &&
                    ((m_bEncodedOrder && !bNextB ) || (!m_bEncodedOrder && m_iCurr == 2 + m_numBuffB - 1)))
                {
                    pFrame->m_bAddEOS = true;
                }

                return true;
            }
            if (m_iCurr < m_numBuffB + 2 && !m_pFrames[m_iCurr].m_pFrame && bLast)  // check if all next fwd_only && no refs for this one
            {
                m_iCurr++;
                *pFrame = m_pFrames[m_iCurr];
                if (m_bAddEOS
                 && ((m_bEncodedOrder && !bNextB) || (!m_bEncodedOrder && m_iCurr == 2 + m_numBuffB - 1)))
                {
                    pFrame->m_bAddEOS = true;
                }

                return true;
            }

            return false;
        }

        inline  virtual void ReleaseCurrentFrame()
        {
            if (!m_pFrames[m_iCurr].m_pFrame)
                return;

            if (m_iCurr>1)
            {
                ReleasePlane(m_pFrames[m_iCurr].m_pFrame);
                memset (&m_pFrames[m_iCurr], 0, sizeof(sFrameEx));
            }
            m_iCurr++;
            if (m_iCurr>= m_numBuffB+2 && (!m_bEncodedOrder))
            {
                NewGOP();
            }
        }

        // with strictGop make B w/only forward reference. without - change type to P
        inline  virtual void CloseGop(bool strictGop)
        {
            if (m_pFrames[1].m_pFrame == 0 && m_numBuffB>0)
            {
                mfxU16       pictureType = 0;

                if (!strictGop)
                {
                    sFrameEx *  pFrame = &m_pFrames[2 + m_numBuffB-1];
                    m_pFrames[1]= *pFrame;
                    pictureType = (mfxU16)(MFX_FRAMETYPE_P  | MFX_FRAMETYPE_REF);

                    memset (pFrame, 0, sizeof (sFrameEx));

                    m_numBuffB --;

                    m_pFrames[1].m_FrameType = pictureType;
                    m_pFrames[1].m_sEncodeInternalParams.FrameType = pictureType;
                    m_pFrames[1].m_bOnlyFwdPrediction = true;

                    if (m_bAddEOS && (m_numBuffB == 0))
                    {
                        m_pFrames[1].m_bAddEOS = true;
                    }
                }
                else
                {
                    for (int i = 0; i < m_numBuffB; i++)
                    {
                        m_pFrames[2 + i].m_bOnlyFwdPrediction = true;
                    }
                }
            }
            if (m_bAddEOS && (!strictGop || (m_numBuffB != 0)))
            {
                m_pFrames[2 + m_numBuffB - 1].m_bAddEOS = true;
            }

        }
        inline bool GetFrameExReference(sFrameEx* pFrame, bool bBackward = false)
        {
            mfxU32 n = (bBackward)? 1:0;
            if (m_pFrames[n].m_pFrame)
            {
                *pFrame = m_pFrames[n];
                return true;
            }
            return false;
        }  
   
        void Close()
        {
            if (m_pFrames)
            {
                delete [] m_pFrames;
                m_pFrames = 0;
            }
            m_iCurr = 0;
            m_numBuffB = 0;
            m_maxB = 0;
            m_nGOPs = 0;
        }

    private:
        // Declare private copy constructor to avoid accidental assignment
        // and klocwork complaining.
        MFXGOP(const MFXGOP &);
        MFXGOP & operator = (const MFXGOP &);
    };


    class MFXWaitingList
    {
    public:
        MFXWaitingList():
            m_maxN(0),
            m_curIndex(0),
            m_nFrames(0),
            m_pFrames(0),
            m_minN(0),
            m_delay(0)
            {};
        ~MFXWaitingList()
        {
            Close();
        }
        void Close()
        {
            if (m_pFrames)
            {
                delete [] m_pFrames;
                m_pFrames = 0;
            }

            m_maxN = 0;
            m_curIndex = 0;
            m_nFrames = 0;
            m_minN=0;
            m_delay = 0;
        }

        mfxStatus Init(mfxU32 maxN, mfxU32 minN, mfxU32 delay)
        {
            Close();

            m_maxN  = maxN;
            m_minN  = minN;
            m_delay = delay;

            if (m_minN > m_maxN)
                return MFX_ERR_UNSUPPORTED;

            m_pFrames = new sFrameEx [ m_maxN];
            if (!m_pFrames)
                return MFX_ERR_MEMORY_ALLOC;

            memset (m_pFrames,0, sizeof(sFrameEx)*m_maxN);

            return MFX_ERR_NONE;
        }

        mfxI32 GetMaxFrames ()
        {
            return m_maxN;
        }
        mfxI32 GetDelay()
        {
            return m_delay;        
        }

        void Reset(mfxI32 minN, mfxU32 delay)
        {
            mfxU32 i = 0;

            if(m_pFrames)
            {
                for(i = 0; i < m_maxN; i++)
                {
                     ReleasePlane(m_pFrames[i].m_pFrame);

                } 
                memset (m_pFrames,0, sizeof(sFrameEx)* m_maxN);
            }         

            m_curIndex = 0;
            m_nFrames = 0;
            m_minN = minN;
            m_delay = delay;
        }

        bool AddFrame(mfxFrameSurface1* frame, mfxEncodeInternalParams *pInInternalParams)
        {
            if (m_nFrames < m_maxN)
            {
                mfxU32 ind = (m_curIndex + m_nFrames)%m_maxN;

                m_pFrames[ind].m_pFrame     = frame;
                m_pFrames[ind].m_FrameOrder = pInInternalParams->FrameOrder;
                m_pFrames[ind].m_FrameType  = pInInternalParams->FrameType;

                m_pFrames[ind].m_bAddEOS            = false;
                m_pFrames[ind].m_bAddHeader         = false;
                m_pFrames[ind].m_bOnlyBwdPrediction = false;

                m_pFrames[ind].m_sEncodeInternalParams  = *pInInternalParams;
                m_nFrames ++;
                return true;
            }
            return false;
        } 
        bool GetFrameEx(sFrameEx* pFrameEx, bool bEOF = false)
        {
            if (m_nFrames>0 && (bEOF || (m_nFrames > m_minN && m_nFrames > m_delay)))
            {
                *pFrameEx = m_pFrames[m_curIndex];
                m_delay = 0;
                return true;
            }
            return false;       
        }

        bool MoveOnNextFrame()
        {
            if (m_nFrames>0)
            {
                m_curIndex = (1+m_curIndex)%m_maxN;
                m_nFrames--;
                return true;
            }
            return false;
        }
  
        bool isNextReferenceIntra()
        {
            for (mfxU32 i=0; i < m_nFrames; i++)
            {
                mfxU32 ind = (m_curIndex+i)%m_maxN;
                mfxU16 type = m_pFrames[ind].m_FrameType;
                if (isReferenceFrame(type))
                {
                    return isIntraFrame(type)? true:false;
                }           
            }
            return false;       
        }
        bool isLastFrame()
        {
            return (m_nFrames < 1);
        }
        bool isNextBFrame()
        {
            if (m_nFrames != 0)
            {
                mfxU32 ind = (m_curIndex)%m_maxN;
                mfxU16 type = m_pFrames[ind].m_FrameType;
                return (isReferenceFrame (type))? false:true;            
            }
            return false;      
        }
    
    private:
        mfxU32              m_maxN;        
        mfxU32              m_curIndex;
        mfxU32              m_nFrames;
        sFrameEx*           m_pFrames;
        mfxU32              m_minN;
        mfxU32              m_delay;

        // Declare private copy constructor to avoid accidental assignment
        // and klocwork complaining.
        MFXWaitingList(const MFXWaitingList &);
        MFXWaitingList & operator = (const MFXWaitingList &);
    };

#endif //_MFX_FRAMES_H_
