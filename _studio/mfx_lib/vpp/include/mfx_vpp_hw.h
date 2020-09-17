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

#include "mfx_common.h"

#if defined (MFX_ENABLE_VPP)

#ifndef __MFX_VPP_HW_H
#define __MFX_VPP_HW_H

#include <algorithm>
#include <set>
#include <math.h>

#include "umc_mutex.h"
#include "mfx_vpp_interface.h"
#include "mfx_vpp_defs.h"

 #include "cmrt_cross_platform.h" // Gpucopy stuff
 #if defined(MFX_ENABLE_SCENE_CHANGE_DETECTION_VPP)
  #include "asc.h"        // Scene change detection
 #endif
 #include "cm_mem_copy.h"         // Needed for mirroring kernels
 #include "genx_fcopy_gen8_isa.h" // Field copy kernels
 #include "genx_fcopy_gen9_isa.h"
 #include "genx_fcopy_gen11_isa.h"
 #include "genx_fcopy_gen11lp_isa.h"
 #include "genx_fcopy_gen12lp_isa.h"

#ifdef MFX_ENABLE_MCTF
#include "mctf_common.h"
#include "cpu_detect.h"
#include <list>
#endif
class CmDevice;

namespace MfxHwVideoProcessing
{
    enum WorkloadMode
    {
        VPP_SYNC_WORKLOAD   = 0,
        VPP_ASYNC_WORKLOAD  = 1,
    };


    enum FrcMode
    {
        FRC_DISABLED = 0x00,
        FRC_ENABLED  = 0x01,
        FRC_STANDARD = 0x02,
        FRC_DISTRIBUTED_TIMESTAMP = 0x04,
        FRC_INTERPOLATION = 0x08
    };

    enum AdvGfxMode
    {
        VARIANCE_REPORT = 0x10,
        IS_REFERENCES   = 0x20,
        COMPOSITE       = 0x40
    };


    //-----------------------------------------------------
    // Utills from HW H264 Encoder (c)
    template<class T> inline void Zero(T & obj)                { memset(&obj, 0, sizeof(obj)); }
    template<class T> inline void Zero(std::vector<T> & vec)   { if (vec.size() > 0) memset(&vec[0], 0, sizeof(T) * vec.size()); }
    template<class T> inline void Zero(T * first, size_t cnt)  { memset(first, 0, sizeof(T) * cnt); }

    class State
    {
    public:
        State()
            : m_free(true)
        {
        }

        bool IsFree() const
        {
            return m_free;
        }

        void SetFree(bool free)
        {
            m_free = free;
        }

    private:
        bool m_free;
    };


    // Helper which checks number of allocated frames and auto-free
    class MfxFrameAllocResponse : public mfxFrameAllocResponse
    {
    public:
        MfxFrameAllocResponse();

        ~MfxFrameAllocResponse();

        mfxStatus Alloc(
            VideoCORE *            core,
            mfxFrameAllocRequest & req,
            bool isCopyRequired = true);

        mfxStatus Alloc(
            VideoCORE *            core,
            mfxFrameAllocRequest & req,
            mfxFrameSurface1 **    opaqSurf,
            mfxU32                 numOpaqSurf);

        mfxStatus Free( void );

    private:
        MfxFrameAllocResponse(MfxFrameAllocResponse const &);
        MfxFrameAllocResponse & operator =(MfxFrameAllocResponse const &);

        VideoCORE * m_core;
        mfxU16      m_numFrameActualReturnedByAllocFrames;

        std::vector<mfxFrameAllocResponse> m_responseQueue;
        std::vector<mfxMemId>              m_mids;
    };

    struct ExtSurface
    {
        ExtSurface ()
            : pSurf(0)
            , timeStamp(0)
            , endTimeStamp(0)
            , resIdx(0)
            , bUpdate(false)
            , bForcedInternalAlloc(false)
        {
        }
        mfxFrameSurface1 *pSurf;

        mfxU64 timeStamp;    // startTimeStamp or targetTimeStamp in DX9
        mfxU64 endTimeStamp; // endTimeStamp in DX9. need to ask driver team. probably can be removed
        mfxU32 resIdx;        // index corresponds _real_ video resource
        bool   bUpdate;       // should be updated in case of vid<->sys?

        // this flag being set says that a surface is allocated by calling allocator
        // via Core interface; which means even though IOMode is set to D3D (so,
        // surfaces are allocated in video memory but by external allocator),
        // to extract handle GetFrameHLD must be used;
        // this is needed for such filters as MCTF that allocate internal surfaces in video memory
        // to operate on this, in particular, in the begining it substitutes such a surface instead of
        // real surface;
        bool bForcedInternalAlloc;
    };
    // auto-lock for frames
    struct FrameLocker
    {
        FrameLocker(VideoCORE * core, mfxFrameData & data, bool external = false)
            : m_core(core)
            , m_data(data)
            , m_memId(data.MemId)
            , m_status(Lock(external))
        {
        }

        FrameLocker(VideoCORE * core, mfxFrameData & data, mfxMemId memId, bool external = false)
            : m_core(core)
            , m_data(data)
            , m_memId(memId)
            , m_status(Lock(external))
        {
        }

        ~FrameLocker() { Unlock(); }

        mfxStatus Unlock()
        {
            mfxStatus mfxSts = MFX_ERR_NONE;

            if (m_status == LOCK_INT)
                mfxSts = m_core->UnlockFrame(m_memId, &m_data);
            else if (m_status == LOCK_EXT)
                mfxSts = m_core->UnlockExternalFrame(m_memId, &m_data);

            m_status = LOCK_NO;
            return mfxSts;
        }

    protected:
        enum { LOCK_NO, LOCK_INT, LOCK_EXT };

        mfxU32 Lock(bool external)
        {
            mfxU32 status = LOCK_NO;

            if (m_data.Y == 0)
            {
                status = external
                    ? (m_core->LockExternalFrame(m_memId, &m_data) == MFX_ERR_NONE ? LOCK_EXT : LOCK_NO)
                    : (m_core->LockFrame(m_memId, &m_data) == MFX_ERR_NONE ? LOCK_INT : LOCK_NO);
            }

            return status;
        }

    private:
        FrameLocker(FrameLocker const &);
        FrameLocker & operator =(FrameLocker const &);

        VideoCORE *    m_core;
        mfxFrameData & m_data;
        mfxMemId       m_memId;
        mfxU32         m_status;
    };
    //-----------------------------------------------------


    struct ReleaseResource
    {
        mfxU32 refCount;
        std::vector<ExtSurface> surfaceListForRelease;
        std::vector<mfxU32> subTasks;
    };

    struct DdiTask : public State
    {
        DdiTask()
            : bkwdRefCount(0)
            , fwdRefCount(0)
            , input()
            , output()
#ifdef MFX_ENABLE_MCTF
            , outputForApp()
#endif
            , bAdvGfxEnable(false)
            , bVariance(false)
            , bEOS(false)
            , bRunTimeCopyPassThrough(false)
#ifdef MFX_ENABLE_MCTF
            , bMCTF(false)
            , MctfControlActive(false)
            , pOuptutSurface(nullptr)
#endif
            , taskIndex(0)
            , frameNumber(0)
            , skipQueryStatus(false)
            , pAuxData(NULL)
            , pSubResource(NULL)
        {
#ifdef MFX_ENABLE_MCTF
            memset(&MctfData, 0, sizeof(IntMctfParams));
#endif
        }

        mfxU32 bkwdRefCount;
        mfxU32 fwdRefCount;

        ExtSurface input;
        ExtSurface output;
#ifdef MFX_ENABLE_MCTF
        // this is a ext-surface that delivers result to an application
        ExtSurface outputForApp;
#endif

        bool bAdvGfxEnable;     // VarianceReport, FRC_interpolation
        bool bVariance;
        bool bEOS;
        bool bRunTimeCopyPassThrough; // based on config.m_bCopyPassThroughEnable and current runtime parameters (input / output surface.Info), if TRUE - VPP must execute task in PassThrough mode
#ifdef MFX_ENABLE_MCTF
        bool bMCTF;
        // per-frame control
        IntMctfParams MctfData;
        bool MctfControlActive;
        mfxFrameSurface1* pOuptutSurface;
#endif


        mfxU32 taskIndex;
        mfxU32 frameNumber;

        bool   skipQueryStatus;

        mfxExtVppAuxData *pAuxData;

        ReleaseResource* pSubResource;

        std::vector<ExtSurface> m_refList; //m_refList.size() == bkwdRefCount +fwdRefCount
    };

    struct ExtendedConfig
    {
        mfxU16 mode;

        CustomRateData customRateData;//for gfxFrc
        RateRational   frcRational[2];//for CpuFrc
    };

    struct Config
    {
        bool   m_bMode30i60pEnable;
        bool   m_bWeave;
        bool   m_bCopyPassThroughEnable;// if this flag is true input surface will be copied to output via DoFastCopyWrapper() without VPP
        bool   m_bRefFrameEnable;
        bool   m_multiBlt;// this flag defines mode of composition for D3D11: 1 - run few hw calls per frame (Blt), 0 - run one hw call (Blt)

        ExtendedConfig m_extConfig;
        mfxU16 m_IOPattern;
        mfxU16 m_surfCount[2];
    };

    class ResMngr
    {
    public:

        ResMngr(void)
            : m_subTaskQueue()
            , m_surfQueue()
        {
            m_bOutputReady = false;
            m_bRefFrameEnable = false;
            m_inputIndex  = 0;
            m_outputIndex = 0;
            m_bkwdRefCount = 0;
            m_fwdRefCount = 0;

            m_EOS = false;
            m_actualNumber = 0;
            m_indxOutTimeStamp = 0;
            m_fieldWeaving = false;

            m_pSubResource = NULL;

            m_inputFramesOrFieldPerCycle = 0;
            m_inputIndexCount   = 0;
            m_outputIndexCountPerCycle  = 0;

            m_fwdRefCountRequired  = 0;
            m_bkwdRefCountRequired = 0;

            m_multiBlt = 0;

            m_core = NULL;
        }

         ~ResMngr(void){}

         mfxStatus Init(
             Config & config,
             VideoCORE* core);

         mfxStatus Close(void);

        mfxStatus DoAdvGfx(
            mfxFrameSurface1 *input,
            mfxFrameSurface1 *output,
            mfxStatus *intSts);

        mfxStatus DoMode30i60p(
            mfxFrameSurface1 *input,
            mfxFrameSurface1 *output,
            mfxStatus *intSts);

        mfxStatus FillTask(
            DdiTask* pTask,
            mfxFrameSurface1 *pInSurface,
            mfxFrameSurface1 *pOutSurface
#ifdef MFX_ENABLE_MCTF
            , mfxFrameSurface1 * pOutSurfaceForApp
#endif
                          );

        mfxStatus FillTaskForMode30i60p(
            DdiTask* pTask,
            mfxFrameSurface1 *pInSurface,
            mfxFrameSurface1 *pOutSurface
#ifdef MFX_ENABLE_MCTF
            , mfxFrameSurface1 * pOutSurfaceForApp
#endif
                                       );

        mfxStatus CompleteTask(DdiTask *pTask);
        std::vector<State> m_surf[2];

        mfxU32 GetSubTask(DdiTask *pTask);
        mfxStatus DeleteSubTask(DdiTask *pTask, mfxU32 subtaskIdx);
        bool IsMultiBlt();

    private:

        mfxStatus ReleaseSubResource(bool bAll);
        ReleaseResource* CreateSubResource(void);
        ReleaseResource* CreateSubResourceForMode30i60p(void);

        //-------------------------------------------------
        mfxU32 GetNumToRemove( void )
        {
            if (m_fieldWeaving)
                return 2;

            return m_inputFramesOrFieldPerCycle - std::min(m_inputFramesOrFieldPerCycle, m_bkwdRefCountRequired - m_bkwdRefCount);
        }

        mfxU32   GetNextBkwdRefCount( void )
        {
            if(m_bkwdRefCount == m_bkwdRefCountRequired)
            {
                return m_bkwdRefCount;
            }

            mfxU32 numBkwdRef = m_bkwdRefCount + (m_inputFramesOrFieldPerCycle - GetNumToRemove());

            return std::min(numBkwdRef,  m_bkwdRefCountRequired);
        }

        //-------------------------------------------------
        bool  m_bOutputReady;
        bool  m_bRefFrameEnable;

        // counters
        mfxU32 m_inputIndex;
        mfxU32 m_outputIndex;
        mfxU32 m_bkwdRefCount;
        mfxU32 m_fwdRefCount;

        mfxU32 m_actualNumber;
        mfxU32 m_indxOutTimeStamp;
        bool   m_EOS;
        bool   m_fieldWeaving;

        std::vector<ReleaseResource*> m_subTaskQueue;
        ReleaseResource*              m_pSubResource;
        std::vector<ExtSurface> m_surfQueue;//container for multi-input in case of advanced processing

        // init params
        mfxU32 m_inputFramesOrFieldPerCycle;//it is number of input frames which will be processed during 1 FRC task slot
        mfxU32 m_inputIndexCount;
        mfxU32 m_outputIndexCountPerCycle; // how many output during 1 task slot

        mfxU32 m_fwdRefCountRequired;
        mfxU32 m_bkwdRefCountRequired;

        bool m_multiBlt;// this flag defines mode of composition for D3D11: 1 - run few hw calls per frame (Blt), 0 - run one hw call (Blt)

        VideoCORE* m_core;
    };


    class CpuFrc
    {
    public:

        CpuFrc(void): m_frcMode(0) {}

         ~CpuFrc(void){}

         void Reset(
             mfxU16 frcMode,
             RateRational frcRational[2])
         {
             m_stdFrc.Reset(frcRational);
             m_ptsFrc.Reset(frcRational);
             m_frcMode = frcMode;
         }

         mfxStatus DoCpuFRC_AndUpdatePTS(
             mfxFrameSurface1 *input,
             mfxFrameSurface1 *output,
             mfxStatus *intSts);

    private:

        struct StdFrc
        {
            StdFrc(void)
            {
                Clear();
            }
            void Reset(RateRational frcRational[2])
            {
                Clear();

                mfxF64 inRate;
                mfxF64 outRate;
                bool frcUp;
                mfxU32 high;
                mfxU32 low;

                m_frcRational[VPP_IN]  = frcRational[VPP_IN];
                m_frcRational[VPP_OUT] = frcRational[VPP_OUT];

                inRate  = 100*(((mfxF64)m_frcRational[VPP_IN].FrameRateExtN / (mfxF64)m_frcRational[VPP_IN].FrameRateExtD));
                outRate = 100*(((mfxF64)m_frcRational[VPP_OUT].FrameRateExtN / (mfxF64)m_frcRational[VPP_OUT].FrameRateExtD));

                m_inFrameTime  = 1000.0 / inRate;
                m_outFrameTime = 1000.0 / outRate;

                mfxU32 nInRate  = (mfxU32)inRate;
                mfxU32 nOutRate = (mfxU32)outRate;
                if ( fabs(inRate - (mfxF64)nInRate) > 0.5 )
                    nInRate++;

                if ( fabs(outRate - (mfxF64)nOutRate) > 0.5 )
                    nOutRate++;

                frcUp = (nInRate < nOutRate) ? true : false;
                high  = frcUp ? nOutRate : nInRate;
                low   = frcUp ? nInRate  : nOutRate;
                m_in_tick  = m_out_tick  = 1;
                m_in_stamp = m_out_stamp = 0;

                if ( nInRate == nOutRate || 0 == low)
                    return;

                mfxF64 mul;
                mfxU32 rate = 1;
                mfxU32 multiplier = 1;
                bool   bFoldRatio = false;

                if ( frcUp )
                {
                    mul = outRate/inRate;
                }
                else
                {
                    mul = inRate/outRate;
                }

                if (  fabs(mul - (mfxU32)mul) < 0.001 )
                {
                    // Ratio between framerates is good enough, no need in
                    // searching of common factors
                    rate = (mfxU32)mul;
                    bFoldRatio = true;
                }
                else
                {
                    // Ratio between framerates is fractional, need to find
                    // common factors
                    mfxU32 tmp = high;
                    while(multiplier<100000)
                    {
                        tmp = high *multiplier;
                        rate  = tmp / low;
                        if (rate*low == tmp)
                            break;
                        multiplier++;
                    }
                }

                if ( frcUp )
                {
                    m_in_tick   = rate;
                    m_out_tick  = multiplier;
                    m_out_stamp = 0;
                    m_in_stamp  = 0;
                }
                else
                {
                    m_in_tick   = multiplier;
                    m_out_tick  = rate;
                    m_out_stamp = m_out_tick;
                    m_in_stamp  = ! bFoldRatio ? 0 : m_out_stamp - m_in_tick;
                }

                m_in_stamp += m_in_tick;

                // calculate time interval between input and output frames
                m_timeFrameInterval = m_inFrameTime - m_outFrameTime;
            }

            mfxStatus DoCpuFRC_AndUpdatePTS(
                mfxFrameSurface1 *input,
                mfxFrameSurface1 *output,
                mfxStatus *intSts);

        private:

            void Clear()
            {
                m_inFrameTime = 0;
                m_outFrameTime = 0;
                m_externalDeltaTime = 0;
                m_timeFrameInterval = 0;
                m_bDuplication = 0;

                m_in_stamp = m_in_tick = m_out_stamp = m_out_tick = 0;

                m_LockedSurfacesList.clear();
                memset(m_frcRational, 0, sizeof(m_frcRational));
            }

            std::vector<mfxFrameSurface1 *> m_LockedSurfacesList;

            mfxU32 m_in_tick;
            mfxU32 m_out_tick;
            mfxU32 m_out_stamp;
            mfxU32 m_in_stamp;

            mfxF64 m_inFrameTime;
            mfxF64 m_outFrameTime;
            mfxF64 m_externalDeltaTime;
            mfxF64 m_timeFrameInterval;
            bool   m_bDuplication;

            RateRational m_frcRational[2];

        } m_stdFrc;

        struct PtsFrc
        {
            PtsFrc(void)
            {
                Clear();
            }

            void Reset(RateRational frcRational[2])
            {
                Clear();

                m_frcRational[VPP_IN]  = frcRational[VPP_IN];
                m_frcRational[VPP_OUT] = frcRational[VPP_OUT];

                m_minDeltaTime = std::min(uint64_t(m_frcRational[VPP_IN].FrameRateExtD  * MFX_TIME_STAMP_FREQUENCY) / (mfxU64(2) * m_frcRational[VPP_IN].FrameRateExtN),
                                          uint64_t(m_frcRational[VPP_OUT].FrameRateExtD * MFX_TIME_STAMP_FREQUENCY) / (mfxU64(2) * m_frcRational[VPP_OUT].FrameRateExtN));
            }

            mfxStatus DoCpuFRC_AndUpdatePTS(
                mfxFrameSurface1 *input,
                mfxFrameSurface1 *output,
                mfxStatus *intSts);

        private:

            void Clear()
            {
                m_bIsSetTimeOffset = false;
                m_bDownFrameRate = false;
                m_bUpFrameRate = false;
                m_timeStampDifference = 0;
                m_expectedTimeStamp = 0;
                m_timeStampJump = 0;
                m_prevInputTimeStamp = 0;
                m_timeOffset = 0;
                m_upCoeff = 0;
                m_numOutputFrames = 0;
                m_minDeltaTime = 0;
                m_LockedSurfacesList.clear();
                memset(m_frcRational, 0, sizeof(m_frcRational));
            }

            std::vector<mfxFrameSurface1 *> m_LockedSurfacesList;

            bool   m_bIsSetTimeOffset;
            bool   m_bDownFrameRate;
            bool   m_bUpFrameRate;
            mfxU64 m_timeStampDifference;
            mfxU64 m_expectedTimeStamp;
            mfxU64 m_timeStampJump;
            mfxU64 m_prevInputTimeStamp;
            mfxU64 m_timeOffset;
            mfxU32 m_upCoeff;
            mfxU32 m_numOutputFrames;
            mfxU64 m_minDeltaTime;

            RateRational m_frcRational[2];

        } m_ptsFrc;

        mfxU16 m_frcMode;
    };


    class TaskManager
    {
    public:

        TaskManager(void);
        ~TaskManager(void);

        mfxStatus Init(
            VideoCORE* core,
            Config & config);

        mfxStatus Close(void);

        mfxStatus AssignTask(
            mfxFrameSurface1 *input,
            mfxFrameSurface1 *output,
#ifdef MFX_ENABLE_MCTF
            mfxFrameSurface1 *outputForApp,
#endif
            mfxExtVppAuxData *aux,
            DdiTask*& pTask,
            mfxStatus & intSts);

        mfxStatus CompleteTask(DdiTask* pTask);

#ifdef MFX_ENABLE_MCTF
        mfxU32 GetMCTFSurfacesInQueue() { return m_MCTFSurfacesInQueue; };
        void DecMCTFSurfacesInQueue() { if (m_MCTFSurfacesInQueue) --m_MCTFSurfacesInQueue; };
        void SetMctf(std::shared_ptr<CMC>& mctf) { pMCTF = mctf; }
#endif
        mfxU32 GetSubTask(DdiTask *pTask);
        mfxStatus DeleteSubTask(DdiTask *pTask, mfxU32 subtaskIdx);

    private:
#ifdef MFX_ENABLE_MCTF
        std::weak_ptr<CMC> pMCTF;
#endif
        mfxStatus DoCpuFRC_AndUpdatePTS(
            mfxFrameSurface1 *input,
            mfxFrameSurface1 *output,
            mfxStatus *intSts);

        mfxStatus DoAdvGfx(
            mfxFrameSurface1 *input,
            mfxFrameSurface1 *output,
            mfxStatus *intSts);

        DdiTask* GetTask(void);

        void FreeTask(DdiTask *pTask)
        {
            pTask->m_refList.clear();
            pTask->SetFree(true);
        }

        // fill task param
        mfxStatus FillTask(
            DdiTask* pTask,
            mfxFrameSurface1 *pInSurface,
            mfxFrameSurface1 *pOutSurface,
#ifdef MFX_ENABLE_MCTF
            mfxFrameSurface1 *pOutSurfaceForApp,
#endif
            mfxExtVppAuxData *aux
        );

#ifdef MFX_ENABLE_MCTF
        // fill task param; its only for cases
        // when there is no input surfaces
        // but we still need to process output
        // example: MCTF
        mfxStatus FillLastTasks(
            DdiTask* pTask,
            mfxFrameSurface1 *pOutSurface,
            mfxFrameSurface1 *pOutSurfaceForApp,
            mfxExtVppAuxData *aux
        );
#endif

        void FillTask_Mode30i60p(
            DdiTask* pTask,
            mfxFrameSurface1 *pInSurface,
            mfxFrameSurface1 *pOutSurface
#ifdef MFX_ENABLE_MCTF
            ,mfxFrameSurface1 *pOutSurfaceForApp
#endif
        );

        void FillTask_AdvGfxMode(
            DdiTask* pTask,
            mfxFrameSurface1 *pInSurface,
            mfxFrameSurface1 *pOutSurface
#ifdef MFX_ENABLE_MCTF
            ,mfxFrameSurface1 *pOutSurfaceForApp
#endif
        );

        void UpdatePTS_Mode30i60p(
            mfxFrameSurface1 *input,
            mfxFrameSurface1 *ouput,
            mfxU32 taskIndex,
            mfxStatus *intSts);

        void UpdatePTS_SimpleMode(
            mfxFrameSurface1 *input,
            mfxFrameSurface1 *ouput);

        std::vector<DdiTask> m_tasks;
        VideoCORE*            m_core;

        mfxI32 m_taskIndex;
        mfxU32 m_actualNumber;

        struct Mode30i60p
        {
             mfxU64 m_prevInputTimeStamp;
             mfxU32 m_numOutputFrames;

             void SetEnable(bool mode)
             {
                m_isEnabled = mode;
             }

             bool IsEnabled( void ) const
             {
                 return m_isEnabled;
             }

        private:
            bool m_isEnabled;

        } m_mode30i60p;

        // init params
        mfxU16 m_extMode;

        CpuFrc  m_cpuFrc;
        ResMngr m_resMngr;

        UMC::Mutex m_mutex;

#ifdef MFX_ENABLE_MCTF
        mfxU32  m_MCTFSurfacesInQueue;
#endif
    }; // class TaskManager

    class VideoVPPHW
    {
    public:

        enum  IOMode
        {
            D3D_TO_D3D  = 0x1,
            D3D_TO_SYS  = 0x2,
            SYS_TO_D3D  = 0x4,
            SYS_TO_SYS  = 0x8,
            ALL         = 0x10,
            MODES_MASK  = 0x1F
        };

        VideoVPPHW(IOMode, VideoCORE *core);
        virtual ~VideoVPPHW(void);

        mfxStatus Init(
            mfxVideoParam *par,
            bool isTemporal = false);

        mfxStatus GetVideoParams(mfxVideoParam *par) const;

        static
        mfxStatus QueryIOSurf(
            IOMode ioMode,
            VideoCORE* core,
            mfxVideoParam *par,
            mfxFrameAllocRequest* request);

        static
        mfxStatus QueryCaps(VideoCORE* core, MfxHwVideoProcessing::mfxVppCaps& caps);

        mfxStatus Reset(mfxVideoParam *par);

        mfxStatus Close(void);

        static
        mfxStatus Query(VideoCORE* core,mfxVideoParam *par);

        static
        mfxStatus QueryTaskRoutine(void *pState, void *pParam, mfxU32 threadNumber, mfxU32 callNumber);
        static
        mfxStatus AsyncTaskSubmission(void *pState, void *pParam, mfxU32 threadNumber, mfxU32 callNumber);

        mfxStatus SyncTaskSubmission(DdiTask* pTask);

        mfxStatus VppFrameCheck(
            mfxFrameSurface1 *input,
            mfxFrameSurface1 *output,
            mfxExtVppAuxData *aux,
            MFX_ENTRY_POINT pEntryPoint[],
            mfxU32 &numEntryPoints);

        mfxStatus RunFrameVPP(mfxFrameSurface1 * /*in*/, mfxFrameSurface1 * /*out*/, mfxExtVppAuxData * /*aux*/)
        {
            //in = in; out = out; aux = aux;
            return MFX_ERR_UNSUPPORTED;
        };

        static
        IOMode GetIOMode(
            mfxVideoParam *par,
            mfxFrameAllocRequest* opaqReq);
    private:

        mfxStatus MergeRuntimeParams(const DdiTask* pTask,  MfxHwVideoProcessing::mfxExecuteParams *execParams);

        mfxStatus QuerySceneChangeResults(
            mfxExtVppAuxData *pAuxData,
            mfxU32 frameIndex);

        mfxStatus CopyPassThrough(
            mfxFrameSurface1 *pInputSurface,
            mfxFrameSurface1 *pOutputSurface);

        bool UseCopyPassThrough(const DdiTask *pTask) const;

        mfxStatus PreWorkOutSurface(ExtSurface & output);
        mfxStatus PreWorkInputSurface(std::vector<ExtSurface> & surfQueue);

        mfxStatus PostWorkOutSurfaceCopy(ExtSurface & output);
        mfxStatus PostWorkOutSurface(ExtSurface & output);
        mfxStatus PostWorkInputSurface(mfxU32 numSamples);

        mfxStatus ProcessFieldCopy(mfxHDL in, mfxHDL out, mfxU32 fieldMask);

#ifdef MFX_ENABLE_MCTF

        mfxStatus InitMCTF(const mfxFrameInfo&, const IntMctfParams&);

        // help-function to get a handle based on MemId
        mfxStatus GetFrameHandle(mfxFrameSurface1* InFrame, mfxHDLPair& handle, bool bInternalAlloc);
        // help-function to get a handle based on MemId
        mfxStatus GetFrameHandle(mfxMemId MemId, mfxHDLPair& handle, bool bInternalAlloc);

        // creates or extracts CmSurface2D from Hanlde
        mfxStatus CreateCmSurface2D(void *pSrcHDL, CmSurface2D* & pCmSurface2D, SurfaceIndex* &pCmSrcIndex);

        // clear(destroy) all surfaces (if any) stored inside m_tableCmRelations2
        mfxStatus ClearCmSurfaces2D();

        // this function is to take a surface into MCTF for further processing
        static mfxStatus SubmitToMctf(void *pState, void *pParam, bool* bMctfReadyToReturn);

        // this is to return a surface out of MCTF
        static mfxStatus QueryFromMctf(void *pState, void *pParam, bool bMctfReadyToReturn, bool bEoF = false);
#endif

        mfxU16 m_asyncDepth;

        MfxHwVideoProcessing::mfxExecuteParams  m_executeParams;
        std::vector<MfxHwVideoProcessing::mfxDrvSurface> m_executeSurf;

        MfxFrameAllocResponse   m_internalVidSurf[2];

        VideoCORE *m_pCore;
        UMC::Mutex m_guard;
        WorkloadMode m_workloadMode;

        mfxU16 m_IOPattern;
        IOMode m_ioMode;

        Config        m_config;
        mfxVideoParam m_params;
        TaskManager   m_taskMngr;

        mfxU32        m_scene_change;
        mfxU32        m_frame_num;
        mfxStatus     m_critical_error;

        // Not a smart pointer anymore since core owns create/delete semantic now.
        VPPHWResMng * m_ddi;
        bool          m_bMultiView;

#ifdef MFX_ENABLE_MCTF
        bool m_MctfIsFlushing;

        // responce for MCTF internal surfaces
        mfxFrameAllocResponse m_MctfMfxAlocResponse;
        // boolean flag to track if surfaces are allocated
        bool m_bMctfAllocatedMemory;
        // storage for mids in responce object
        std::vector<mfxMemId> m_MctfMids;
        // MCTF object
        std::shared_ptr<CMC>    m_pMCTFilter;
        // to separate it from m_pCmDevice
        CmDevice  *m_pMctfCmDevice;
        // list that tracks surfaces needed fort unlock
        std::list<mfxFrameSurface1*> m_Surfaces2Unlock;
        // pool of surfaces & pointers for MCTF
        std::vector<mfxFrameSurface1> m_MCTFSurfacePool;
        std::vector<mfxFrameSurface1*> m_pMCTFSurfacePool;

        // these maps are used to track cm-surfaces for MCTF
        // CmCopyWrapper also has similar maps, but it implements
        // additional functionallity which is not required for MCTF
        std::map<void *, CmSurface2D *> m_MCTFtableCmRelations2;
        std::map<CmSurface2D *, SurfaceIndex *> m_MCTFtableCmIndex2;
#endif

        CmCopyWrapper *m_pCmCopy;

#if defined(MFX_ENABLE_SCENE_CHANGE_DETECTION_VPP)
       ns_asc::ASC m_SCD;
#endif

        CmDevice  *m_pCmDevice;
        CmProgram *m_pCmProgram;
        CmKernel  *m_pCmKernel;
        CmQueue   *m_pCmQueue;

        public:
            void SetCmDevice(CmDevice * device) { m_pCmDevice = device; }
    };

    class VpUnsupportedError : public std::exception
    {
    public:
        VpUnsupportedError() : std::exception() { assert(!"VpUnsupportedError"); }
    };

}; // namespace MfxHwVideoProcessing


#endif // __MFX_VPP_HW_H
#endif // MFX_ENABLE_VPP
