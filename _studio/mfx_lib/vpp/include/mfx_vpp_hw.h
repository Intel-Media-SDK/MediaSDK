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
  #include "mfx_scd.h"        // Scene change detection
 #endif
 #include "cm_mem_copy.h"         // Needed for mirroring kernels
#include "genx_fcopy_cmcode_isa.cpp" // Field copy kernel

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
        {
        }
        mfxFrameSurface1 *pSurf;

        mfxU64 timeStamp;    // startTimeStamp or targetTimeStamp in DX9
        mfxU64 endTimeStamp; // endTimeStamp in DX9. need to ask driver team. probably can be removed
        mfxU32 resIdx;        // index corresponds _real_ video resource
        bool   bUpdate;       // should be updated in case of vid<->sys?
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
            , bAdvGfxEnable(false)
            , bVariance(false)
            , bEOS(false)
            , taskIndex(0)
            , frameNumber(0)
            , skipQueryStatus(false)
            , pAuxData(NULL)
            , pSubResource(NULL)
        {}

        mfxU32 bkwdRefCount;
        mfxU32 fwdRefCount;

        ExtSurface input;
        ExtSurface output;

        bool bAdvGfxEnable;     // VarianceReport, FRC_interpolation
        bool bVariance;
        bool bEOS;

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
        bool   m_bPassThroughEnable;
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
            mfxFrameSurface1 *pOutSurface);

        mfxStatus FillTaskForMode30i60p(
            DdiTask* pTask,
            mfxFrameSurface1 *pInSurface,
            mfxFrameSurface1 *pOutSurface);

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
        mfxU32   GetNumToRemove( void )
        {
            mfxU32 numFramesToRemove = m_inputFramesOrFieldPerCycle - MFX_MIN(m_inputFramesOrFieldPerCycle, m_bkwdRefCountRequired - m_bkwdRefCount);

            if (m_fieldWeaving)
                numFramesToRemove = 2;

            return numFramesToRemove;
        }

        mfxU32   GetNextBkwdRefCount( void )
        {
            if(m_bkwdRefCount == m_bkwdRefCountRequired)
            {
                return m_bkwdRefCount;
            }

            mfxU32 numBkwdRef = m_bkwdRefCount + (m_inputFramesOrFieldPerCycle - GetNumToRemove());
            numBkwdRef = MFX_MIN(numBkwdRef,  m_bkwdRefCountRequired);

            return numBkwdRef;
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

                m_minDeltaTime = MFX_MIN((__UINT64) (m_frcRational[VPP_IN].FrameRateExtD * MFX_TIME_STAMP_FREQUENCY) / (2 * m_frcRational[VPP_IN].FrameRateExtN),
                    (__UINT64) (m_frcRational[VPP_OUT].FrameRateExtD * MFX_TIME_STAMP_FREQUENCY) / (2 * m_frcRational[VPP_OUT].FrameRateExtN));
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
            mfxExtVppAuxData *aux,
            DdiTask*& pTask,
            mfxStatus & intSts);

        mfxStatus CompleteTask(DdiTask* pTask);

        mfxU32 GetSubTask(DdiTask *pTask);
        mfxStatus DeleteSubTask(DdiTask *pTask, mfxU32 subtaskIdx);

    private:

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
            mfxExtVppAuxData *aux);

        void FillTask_Mode30i60p(
            DdiTask* pTask,
            mfxFrameSurface1 *pInSurface,
            mfxFrameSurface1 *pOutSurface);

        void FillTask_AdvGfxMode(
            DdiTask* pTask,
            mfxFrameSurface1 *pInSurface,
            mfxFrameSurface1 *pOutSurface);

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

        mfxStatus PreWorkOutSurface(ExtSurface & output);
        mfxStatus PreWorkInputSurface(std::vector<ExtSurface> & surfQueue);

        mfxStatus PostWorkOutSurface(ExtSurface & output);
        mfxStatus PostWorkInputSurface(mfxU32 numSamples);

        mfxStatus ProcessFieldCopy(mfxHDL in, mfxHDL out, mfxU32 fieldMask);

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

        // Not an unique_ptr anymore since core owns create/delete semantic now.
        VPPHWResMng * m_ddi;
        bool          m_bMultiView;

        CmCopyWrapper *m_pCmCopy;

#if defined(MFX_ENABLE_SCENE_CHANGE_DETECTION_VPP)
        SceneChangeDetector m_SCD;
#endif

        CmDevice  *m_pCmDevice;
        CmProgram *m_pCmProgram;
        CmKernel  *m_pCmKernel;
        CmQueue   *m_pCmQueue;

        public:
            void SetCmDevice(CmDevice * device) { m_pCmDevice = device; }
    };
}; // namespace MfxHwVideoProcessing


#endif // __MFX_VPP_HW_H
//#endif // MFX_VA
#endif // MFX_ENABLE_VPP
