// Copyright (c) 2008-2020 Intel Corporation
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

#include <algorithm>
#include <assert.h>
#include <limits>
#include "math.h"

#include "libmfx_core.h"
#include "libmfx_core_interface.h"
#include "libmfx_core_factory.h"

#include "mfx_common_int.h"
#include "mfx_enc_common.h" // UMC::FrameRate

#include "mfx_vpp_utils.h"
#include "mfx_task.h"
#include "mfx_vpp_defs.h"
#include "mfx_vpp_hw.h"

#ifdef MFX_VA_LINUX
#include "libmfx_core_vaapi.h"
#endif

#ifdef MFX_ENABLE_MCTF
#include "vm_time.h"
#include "mctf_common.h"
#endif

using namespace MfxHwVideoProcessing;
enum
{
    CURRENT_TIME_STAMP              = 100000,
    FRAME_INTERVAL                  = 10000,
    ACCEPTED_DEVICE_ASYNC_DEPTH     = 15, //

};

enum QueryStatus
{
    VPREP_GPU_READY         =   0,
    VPREP_GPU_BUSY          =   1,
    VPREP_GPU_NOT_REACHED   =   2,
    VPREP_GPU_FAILED        =   3
};

enum
{
    DEINTERLACE_ENABLE  = 0,
    DEINTERLACE_DISABLE = 1
};

enum
{
    TFF2TFF = 0x0,
    TFF2BFF = 0x1,
    BFF2TFF = 0x2,
    BFF2BFF = 0x3,
    FIELD2TFF = 0x4,  // FIELD means half-sized frame w/ one field only
    FIELD2BFF = 0x5,
    TFF2FIELD = 0x6,
    BFF2FIELD = 0x7,
    FRAME2FRAME = 0x16,
    FROM_RUNTIME_EXTBUFF_FIELD_PROC = 0x100,
    FROM_RUNTIME_PICSTRUCT = 0x200,
};

// enum for m_scene_change states
enum
{
    NO_SCENE_CHANGE = 0x0,
    SCENE_CHANGE = 0x1, // Clean scene change = first field of new scene
    MORE_SCENE_CHANGE_DETECTED = 0x2, // Back to back scene change detected
    LAST_IN_SCENE = 0x3, // last field of old scene
    NEXT_IS_LAST_IN_SCENE = 0x4, //next field is last
};

// enum for m_surfQueue with 1 reference
enum
{
    PREVIOUS_INPUT   = 0,
    CURRENT_INPUT     = 1,
};
////////////////////////////////////////////////////////////////////////////////////
// Utils
////////////////////////////////////////////////////////////////////////////////////

static void MemSetZero4mfxExecuteParams (mfxExecuteParams *pMfxExecuteParams )
{
    memset(&pMfxExecuteParams->targetSurface, 0, sizeof(mfxDrvSurface));
    pMfxExecuteParams->pRefSurfaces = NULL ;
    pMfxExecuteParams->dstRects.clear();
    memset(&pMfxExecuteParams->customRateData, 0, sizeof(CustomRateData));
    pMfxExecuteParams->targetTimeStamp = 0;
    pMfxExecuteParams->refCount = 0;
    pMfxExecuteParams->bkwdRefCount = 0;
    pMfxExecuteParams->fwdRefCount = 0;
    pMfxExecuteParams->iDeinterlacingAlgorithm = 0;
    pMfxExecuteParams->bFMDEnable = 0;
    pMfxExecuteParams->bDenoiseAutoAdjust = 0;
    pMfxExecuteParams->bDetailAutoAdjust = 0;
    pMfxExecuteParams->denoiseFactor = 0;
    pMfxExecuteParams->detailFactor = 0;
    pMfxExecuteParams->iTargetInterlacingMode = 0;
    pMfxExecuteParams->bEnableProcAmp = false;
    pMfxExecuteParams->Brightness = 0;
    pMfxExecuteParams->Contrast = 0;
    pMfxExecuteParams->Hue = 0;
    pMfxExecuteParams->Saturation = 0;
    pMfxExecuteParams->bVarianceEnable = false;
    pMfxExecuteParams->bImgStabilizationEnable = false;
    pMfxExecuteParams->istabMode = 0;
    pMfxExecuteParams->bFRCEnable = false;
    pMfxExecuteParams->bComposite = false;
    pMfxExecuteParams->iBackgroundColor = 0;
    pMfxExecuteParams->statusReportID = 0;
    pMfxExecuteParams->bFieldWeaving = false;
    pMfxExecuteParams->mirroringExt = false;
    pMfxExecuteParams->iFieldProcessingMode = 0;
    pMfxExecuteParams->rotation = 0;
    pMfxExecuteParams->scalingMode = MFX_SCALING_MODE_DEFAULT;
#if (MFX_VERSION >= 1033)
    pMfxExecuteParams->interpolationMethod = MFX_INTERPOLATION_DEFAULT;
#endif
#if (MFX_VERSION >= 1025)
    pMfxExecuteParams->chromaSiting = MFX_CHROMA_SITING_UNKNOWN;
#endif
    pMfxExecuteParams->bEOS = false;
    pMfxExecuteParams->scene = VPP_NO_SCENE_CHANGE;
} /*void MemSetZero4mfxExecuteParams (mfxExecuteParams *pMfxExecuteParams )*/



template<typename T> mfxU32 FindFreeSurface(std::vector<T> const & vec)
{
    for (size_t j = 0; j < vec.size(); j++)
    {
        if (vec[j].IsFree())
        {
            return (mfxU32)j;
        }
    }

    return NO_INDEX;

} // template<typename T> mfxU32 FindFreeSurface(std::vector<T> const & vec)


template<typename T> void Clear(std::vector<T> & v)
{
    std::vector<T>().swap(v);

} // template<typename T> void Clear(std::vector<T> & v)


//mfxU32 GetMFXFrcMode(mfxVideoParam & videoParam);

mfxStatus CheckIOMode(mfxVideoParam *par, VideoVPPHW::IOMode mode);
mfxStatus ValidateParams(mfxVideoParam *par, mfxVppCaps *caps, VideoCORE *core, bool bCorrectionEnable = false);
mfxStatus GetWorstSts(mfxStatus sts1, mfxStatus sts2);
mfxStatus ConfigureExecuteParams(
    mfxVideoParam & videoParam, // [IN]
    mfxVppCaps & caps,          // [IN]

    mfxExecuteParams & executeParams,// [OUT]
    Config & config);           // [OUT]


mfxStatus CopyFrameDataBothFields(
        VideoCORE *          core,
        mfxFrameData /*const*/ & dst,
        mfxFrameData /*const*/ & src,
        mfxFrameInfo const & info);

////////////////////////////////////////////////////////////////////////////////////
// CpuFRC
////////////////////////////////////////////////////////////////////////////////////

mfxStatus CpuFrc::StdFrc::DoCpuFRC_AndUpdatePTS(
    mfxFrameSurface1 *input,
    mfxFrameSurface1 *output,
    mfxStatus *intSts)
{

    std::vector<mfxFrameSurface1 *>::iterator iterator;

    if (m_in_stamp + m_in_tick <= m_out_stamp)
    {
        // skip frame
        // request new one input surface
        m_in_stamp += m_in_tick;
        return MFX_ERR_MORE_DATA;
    }
    else if (m_out_stamp + m_out_tick < m_in_stamp)
    {
        // Save current input surface and request more output surfaces
        iterator = std::find(m_LockedSurfacesList.begin(), m_LockedSurfacesList.end(), input);

        if (m_LockedSurfacesList.end() == iterator)
        {
            m_LockedSurfacesList.push_back(input);
        }

        if (true == m_bDuplication)
        {
            // set output pts equal to -1, because of it is repeated frame
            output->Data.FrameOrder = (mfxU32) MFX_FRAMEORDER_UNKNOWN;
            output->Data.TimeStamp = (mfxU64) MFX_TIME_STAMP_INVALID;
        }
        else
        {
            // KW issue resolved
            if(NULL != input)
            {
                output->Data.TimeStamp = input->Data.TimeStamp;
                output->Data.FrameOrder = input->Data.FrameOrder;
            }
            else
            {
                output->Data.FrameOrder = (mfxU32) MFX_FRAMEORDER_UNKNOWN;
                output->Data.TimeStamp = (mfxU64) MFX_TIME_STAMP_INVALID;
            }
        }

        m_bDuplication = true;

        // duplicate this frame
        // request new one output surface
        *intSts = MFX_ERR_MORE_SURFACE;
    }
    else
    {
        iterator = std::find(m_LockedSurfacesList.begin(), m_LockedSurfacesList.end(), input);

        if (m_LockedSurfacesList.end() != iterator)
        {
            m_LockedSurfacesList.erase(m_LockedSurfacesList.begin());
        }
        m_in_stamp += m_in_tick;
    }

    m_out_stamp += m_out_tick;

    if (MFX_ERR_MORE_SURFACE != *intSts && NULL != input)
    {
        if (true == m_bDuplication)
        {
            output->Data.FrameOrder = (mfxU32) MFX_FRAMEORDER_UNKNOWN;
            output->Data.TimeStamp = (mfxU64) MFX_TIME_STAMP_INVALID;
            m_bDuplication = false;
        }
        else
        {
            output->Data.TimeStamp = input->Data.TimeStamp;
            output->Data.FrameOrder = input->Data.FrameOrder;
        }
    }

    return MFX_ERR_NONE;

} // mfxStatus CpuFrc::StdFrc::DoCpuFRC_AndUpdatePTS(...)


mfxStatus CpuFrc::PtsFrc::DoCpuFRC_AndUpdatePTS(
    mfxFrameSurface1 *input,
    mfxFrameSurface1 *output,
    mfxStatus *intSts)
{
    std::vector<mfxFrameSurface1 *>::iterator iterator;

    mfxU64 inputTimeStamp = input->Data.TimeStamp;

    if (false == m_bIsSetTimeOffset)
    {
        // set initial time stamp offset
        m_bIsSetTimeOffset = true;
        m_timeOffset = input->Data.TimeStamp;
    }

    // calculate expected timestamp based on output framerate

    m_expectedTimeStamp = (((mfxU64)m_numOutputFrames * m_frcRational[VPP_OUT].FrameRateExtD * MFX_TIME_STAMP_FREQUENCY) / m_frcRational[VPP_OUT].FrameRateExtN +
        m_timeOffset + m_timeStampJump);

    mfxU32 timeStampDifference = abs((mfxI32)(inputTimeStamp - m_expectedTimeStamp));


    // process irregularity
    if (m_minDeltaTime > timeStampDifference)
    {
        inputTimeStamp = m_expectedTimeStamp;
    }

    // made decision regarding frame rate conversion
    if ((input->Data.TimeStamp != (mfxU64)-1) && !m_bUpFrameRate && inputTimeStamp < m_expectedTimeStamp)
    {
        m_bDownFrameRate = true;

        // calculate output time stamp
        output->Data.TimeStamp = m_expectedTimeStamp;

        // skip frame
        // request new one input surface
        return MFX_ERR_MORE_DATA;
    }
    else if ((input->Data.TimeStamp != (mfxU64)-1) && !m_bDownFrameRate && (input->Data.TimeStamp > m_expectedTimeStamp || m_timeStampDifference))
    {
        m_bUpFrameRate = true;

        if (inputTimeStamp <= m_expectedTimeStamp)
        {
            m_upCoeff = 0;
            m_timeStampDifference = 0;

            // manage locked surfaces
            iterator = std::find(m_LockedSurfacesList.begin(), m_LockedSurfacesList.end(), input);

            if (m_LockedSurfacesList.end() != iterator)
            {
                m_LockedSurfacesList.erase(m_LockedSurfacesList.begin());
            }

            output->Data.TimeStamp = m_expectedTimeStamp;

            // save current time stamp value
            m_prevInputTimeStamp = m_expectedTimeStamp;
        }
        else
        {
            // manage locked surfaces
            iterator = std::find(m_LockedSurfacesList.begin(), m_LockedSurfacesList.end(), input);

            if (m_LockedSurfacesList.end() == iterator)
            {
                m_LockedSurfacesList.push_back(input);
            }
            else
            {
            }

            // calculate timestamp increment
            if (false == m_timeStampDifference)
            {
                m_timeStampDifference = abs((mfxI32)(m_expectedTimeStamp - m_prevInputTimeStamp));
            }

            m_upCoeff += 1;

            // calculate output time stamp
            output->Data.TimeStamp = m_prevInputTimeStamp + m_timeStampDifference * m_upCoeff;

            // duplicate this frame
            // request new one output surface
            *intSts = MFX_ERR_MORE_SURFACE;
        }
    }
    else
    {
        // manage locked surfaces
        iterator = std::find(m_LockedSurfacesList.begin(), m_LockedSurfacesList.end(), input);

        if (m_LockedSurfacesList.end() != iterator)
        {
            m_LockedSurfacesList.erase(m_LockedSurfacesList.begin());
        }

        output->Data.TimeStamp = m_expectedTimeStamp;

        // save current time stamp value
        m_prevInputTimeStamp = m_expectedTimeStamp;
    }

    output->Data.FrameOrder = m_numOutputFrames;
    // increment number of output frames
    m_numOutputFrames += 1;

    return MFX_ERR_NONE;

} // mfxStatus CpuFrc::PtsFrc::DoCpuFRC_AndUpdatePTS(...)


mfxStatus CpuFrc::DoCpuFRC_AndUpdatePTS(
    mfxFrameSurface1 *input,
    mfxFrameSurface1 *output,
    mfxStatus *intSts)
{
    if(nullptr == input) return MFX_ERR_MORE_DATA;
    if(nullptr == output) return MFX_ERR_MORE_SURFACE;
    if (FRC_STANDARD & m_frcMode) // standard FRC (input frames count -> output frames count)
    {
        return m_stdFrc.DoCpuFRC_AndUpdatePTS(input, output, intSts);
    }
    else // frame rate conversion by pts
    {
        return m_ptsFrc.DoCpuFRC_AndUpdatePTS(input, output, intSts);
    }

} // mfxStatus VideoVPPHW::DoCpuFRC_AndUpdatePTS(...)


////////////////////////////////////////////////////////////////////////////////////
// Resource Manager
////////////////////////////////////////////////////////////////////////////////////
mfxStatus ResMngr::Close(void)
{
    ReleaseSubResource(true);

    Clear(m_surf[VPP_IN]);
    Clear(m_surf[VPP_OUT]);

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

    m_inputFramesOrFieldPerCycle = 0;
    m_inputIndexCount   = 0;
    m_outputIndexCountPerCycle  = 0;

    m_fwdRefCountRequired  = 0;
    m_bkwdRefCountRequired = 0;
/*
    m_core = NULL;
    if (NULL != m_pSubResource)
    {
        m_pSubResource->refCount = 0;
        m_pSubResource->surfaceListForRelease.clear();
        m_pSubResource = NULL;
    }
    */
    m_subTaskQueue.clear();

    m_surfQueue.clear();

    return MFX_ERR_NONE;

} // mfxStatus ResMngr::Close(void)


mfxStatus ResMngr::Init(
    Config & config,
    VideoCORE* core)
{
    if( config.m_IOPattern & MFX_IOPATTERN_IN_SYSTEM_MEMORY)
    {
        m_surf[VPP_IN].resize( config.m_surfCount[VPP_IN] );
    }

    if( config.m_IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY)
    {
        m_surf[VPP_OUT].resize( config.m_surfCount[VPP_OUT] );
    }

    m_bRefFrameEnable = config.m_bRefFrameEnable;

    if( (config.m_extConfig.mode & FRC_INTERPOLATION) ||
        (config.m_extConfig.mode & VARIANCE_REPORT) ||
        (config.m_extConfig.mode & COMPOSITE) ||
        (config.m_extConfig.mode & IS_REFERENCES))
    {
        mfxU32 totalInputFramesCount = config.m_extConfig.customRateData.bkwdRefCount +
                                       1 +
                                       config.m_extConfig.customRateData.fwdRefCount;

        m_inputIndexCount   = totalInputFramesCount;
        m_outputIndexCountPerCycle = config.m_extConfig.customRateData.outputIndexCountPerCycle;
        m_bkwdRefCountRequired = config.m_extConfig.customRateData.bkwdRefCount;
        m_fwdRefCountRequired  = config.m_extConfig.customRateData.fwdRefCount;
        m_inputFramesOrFieldPerCycle = config.m_extConfig.customRateData.inputFramesOrFieldPerCycle;
        //m_core                 = core;
    }

    m_fieldWeaving = config.m_bWeave;

    m_multiBlt = config.m_multiBlt;

    m_core                 = core;

    return MFX_ERR_NONE;

} // mfxStatus ResMngr::Init(...)


mfxStatus ResMngr::DoAdvGfx(
    mfxFrameSurface1 *input,
    mfxFrameSurface1 * /*output*/,
    mfxStatus *intSts)
{
    assert( m_inputIndex  <=  m_inputIndexCount);
    assert( m_outputIndex <   m_outputIndexCountPerCycle);

    if( m_bOutputReady )// MARKER: output is ready, new out surface is need only
    {
         m_outputIndex++;

        if(  m_outputIndex ==  m_outputIndexCountPerCycle )//MARKER: produce last frame for current Task Slot
        {
             // update all state
             m_bOutputReady= false;
             m_outputIndex = 0;

            *intSts = MFX_ERR_NONE;
        }
        else
        {
            *intSts = MFX_ERR_MORE_SURFACE;
        }

         m_indxOutTimeStamp++;

        return MFX_ERR_NONE;
    }
    else // new task
    {
        m_fwdRefCount = m_fwdRefCountRequired;

        if (m_fieldWeaving)
            m_bkwdRefCount = m_bkwdRefCountRequired;


        if(input)
        {
            m_inputIndex++;

            mfxStatus sts = m_core->IncreaseReference( &(input->Data) );
            MFX_CHECK_STS(sts);

            ExtSurface surf;
            surf.pSurf = input;
            if(m_surf[VPP_IN].size() > 0) // input in system memory
            {
                surf.bUpdate = true;
                surf.resIdx = FindFreeSurface(m_surf[VPP_IN]);
                if(NO_INDEX == surf.resIdx) return MFX_WRN_DEVICE_BUSY;

                m_surf[VPP_IN][surf.resIdx].SetFree(false);// marks resource as "locked"
            }
            m_surfQueue.push_back(surf);
        }

        bool isEOSSignal = ((NULL == input) && (m_bkwdRefCount == m_bkwdRefCountRequired) && (m_inputIndex > m_bkwdRefCountRequired));

        if(isEOSSignal)
        {
            m_fwdRefCount = m_inputIndex > m_bkwdRefCount + 1 ? m_inputIndex - m_bkwdRefCount - 1 : 0;
        }

        if( (m_inputIndex - m_bkwdRefCount == m_inputIndexCount - m_bkwdRefCountRequired) ||
            isEOSSignal) // MARKER: start new Task Slot
        {
             m_actualNumber = m_actualNumber + m_inputFramesOrFieldPerCycle; // for driver PTS
             m_outputIndex++;
             m_indxOutTimeStamp = 0;

            if(  m_outputIndex ==  m_outputIndexCountPerCycle ) // MARKER: produce last frame for current Task Slot
            {
                // update all state
                m_outputIndex = 0;

                *intSts = MFX_ERR_NONE;
            }
            else
            {
                m_bOutputReady = true;

                *intSts = MFX_ERR_MORE_SURFACE;
            }

             m_pSubResource =  CreateSubResource();

            return MFX_ERR_NONE;
        }
        //-------------------------------------------------
        // MARKER: task couldn't be started.
        else
        {
            *intSts = MFX_ERR_MORE_DATA;

            return MFX_ERR_MORE_DATA;
        }
    }

} // mfxStatus ResMngr::DoAdvGfx(...)

/// This function requests for new decoded frame if needed in
/// 30i->60p mode
/*
  \param[in] input input surface
  \param[in] output output surface
  \param [out] intSts :MFX_ERR_NONE will request for decoded frame,
                       MFX_ERR_MORE_SURFACE will call VPP on previously decoded surface
 */
mfxStatus ResMngr::DoMode30i60p(
    mfxFrameSurface1 *input,
    mfxFrameSurface1 * /*output*/,
    mfxStatus *intSts)
{
    if( m_bOutputReady )// MARKER: output is ready, new out surface is need only
    {
         m_inputIndex++;

         m_bOutputReady = false;
         *intSts        = MFX_ERR_NONE;
         m_outputIndex = 0; // marker task end;

        return MFX_ERR_NONE;
    }
    else // new task
    {
        m_outputIndex = 1; // marker new task;

        if(input)
        {
            m_EOS = false;
            if(0 == m_inputIndex)
            {
                *intSts = MFX_ERR_NONE;
                m_outputIndexCountPerCycle = 3; //was 3
                m_bkwdRefCount = 0; // First frame does not have reference
            }
            else
            {
                m_bOutputReady = true;
                *intSts = MFX_ERR_MORE_SURFACE;
                m_outputIndexCountPerCycle = 2;

                if (true == m_bRefFrameEnable) // ADI
                {
                    // need one backward reference to enable motion adaptive ADI
                    m_bkwdRefCount = 1;
                }
                else
                {
                    m_bkwdRefCount = 0;
                }
            }

            mfxStatus sts = m_core->IncreaseReference( &(input->Data) );
            MFX_CHECK_STS(sts);

            ExtSurface surf;
            surf.pSurf = input;
            if(m_surf[VPP_IN].size() > 0) // input in system memory
            {
                surf.bUpdate = true;
                surf.resIdx = FindFreeSurface(m_surf[VPP_IN]);
                if(NO_INDEX == surf.resIdx) return MFX_WRN_DEVICE_BUSY;

                m_surf[VPP_IN][surf.resIdx].SetFree(false);// marks resource as "locked"
            }
            m_surfQueue.push_back(surf);

            if(1 != m_inputIndex)
            {
                m_pSubResource =  CreateSubResourceForMode30i60p();
            }

            m_inputIndex++;

            return MFX_ERR_NONE;
        }
        else
        {
            m_EOS = true;
            if (m_surfQueue.size() > 0)
            {
                *intSts = MFX_ERR_NONE;
                m_outputIndexCountPerCycle = 1;
                m_bkwdRefCount = 0;
                m_outputIndex  = 0; // marker task end;

                m_pSubResource = CreateSubResourceForMode30i60p();
                return MFX_ERR_NONE;
            }
            else
            {
                return MFX_ERR_MORE_DATA;
            }
        }
    }

} // mfxStatus ResMngr::DoMode30i60p(...)


mfxStatus ResMngr::ReleaseSubResource(bool bAll)
{
    //-----------------------------------------------------
    // Release SubResource
    // (1) if all task have been completed (refCount == 0)
    // (2) if common Close()
    std::vector<ReleaseResource*> taskToRemove;
    mfxU32 i;
    for (i = 0; i < m_subTaskQueue.size(); i++)
    {

        if (bAll || (0 == m_subTaskQueue[i]->refCount) )
        {
            for (mfxU32 resIndx = 0; resIndx <  m_subTaskQueue[i]->surfaceListForRelease.size(); resIndx++ )
            {
                ExtSurface & extSrf = m_subTaskQueue[i]->surfaceListForRelease[resIndx];
                mfxU32 freeIdx = extSrf.resIdx;
                if (NO_INDEX != freeIdx && m_surf[VPP_IN].size() > 0)
                {
                    m_surf[VPP_IN][freeIdx].SetFree(true);
                }

                if (bAll)
                {
                    // surfaceListForRelease has copies from m_surfQueue.
                    // Remove such elements from m_surfQueue.
                    std::vector<ExtSurface>::iterator it = m_surfQueue.begin();
                    while (it != m_surfQueue.end())
                    {
                        if (it->pSurf == extSrf.pSurf)
                            it = m_surfQueue.erase(it);
                        else
                            it++;
                    }
                }

                mfxStatus sts = m_core->DecreaseReference( &(extSrf.pSurf->Data) );
                MFX_CHECK_STS(sts);
            }
            m_subTaskQueue[i]->subTasks.shrink_to_fit();
            m_subTaskQueue[i]->subTasks.clear();
            taskToRemove.push_back( m_subTaskQueue[i] );
        }
    }

    size_t removeCount = taskToRemove.size();
    std::vector<ReleaseResource*>::iterator it;
    for (i = 0; i < removeCount; i++)
    {
        it = std::find(m_subTaskQueue.begin(), m_subTaskQueue.end(), taskToRemove[i] );
        if ( it != m_subTaskQueue.end())
        {
            m_subTaskQueue.erase(it);
        }

        delete taskToRemove[i];
    }

    if (bAll)
    {
        // DecreaseReference on remaining elements from m_surfQueue
        while (m_surfQueue.size())
        {
            mfxFrameSurface1 * surf = m_surfQueue.begin()->pSurf;
            m_surfQueue.erase(m_surfQueue.begin());
            m_core->DecreaseReference( &(surf->Data));
        }
    }

    return MFX_ERR_NONE;

} // mfxStatus ResMngr::ReleaseSubResource(bool bAll)


ReleaseResource* ResMngr::CreateSubResource(void)
{
    // fill resource to remove after task slot completion
    ReleaseResource* subRes = new ReleaseResource;
    subRes->refCount = 0;
    subRes->surfaceListForRelease.clear();
    subRes->subTasks.clear();

    subRes->refCount = m_outputIndexCountPerCycle;

    mfxU32 numFramesForRemove = std::min(GetNumToRemove(), mfxU32(m_surfQueue.size()));

    for(mfxU32 i = 0; i < numFramesForRemove; i++)
    {
        subRes->surfaceListForRelease.push_back( m_surfQueue[i] );
    }
    m_subTaskQueue.push_back(subRes);

    return subRes;

} // ReleaseResource* ResMngr::CreateSubResource(void)


ReleaseResource* ResMngr::CreateSubResourceForMode30i60p(void)
{
    // fill resource to remove after task slot completion
    ReleaseResource* subRes = new ReleaseResource;
    subRes->refCount = 0;
    subRes->surfaceListForRelease.clear();
    subRes->subTasks.clear();


    subRes->refCount = m_outputIndexCountPerCycle;

    mfxU32 numFramesForRemove = 1;

    for(mfxU32 i = 0; i < numFramesForRemove; i++)
    {
        subRes->surfaceListForRelease.push_back( m_surfQueue[i] );
    }
    m_subTaskQueue.push_back(subRes);

    return subRes;

} // ReleaseResource* ResMngr::CreateSubResourceForMode30i60p(void)


//---------------------------------------------------------
/// Sets up reference, current input and corresponding timeStamp
mfxStatus ResMngr::FillTaskForMode30i60p(
    DdiTask* pTask,
    mfxFrameSurface1 * /*pInSurface*/,
    mfxFrameSurface1 *pOutSurface
#ifdef MFX_ENABLE_MCTF
    , mfxFrameSurface1 * pOutSurfaceForApp
#endif
)
{
    mfxU32 refIndx = 0;
    pTask->bEOS = m_EOS;
    // bkwd
    pTask->bkwdRefCount = m_bkwdRefCount;
    mfxU32 actualNumber = m_actualNumber;

    // sets up reference frame
    for(refIndx = 0; refIndx < pTask->bkwdRefCount; refIndx++)
    {
        ExtSurface bkwdSurf = m_surfQueue[refIndx];
        bkwdSurf.timeStamp     = CURRENT_TIME_STAMP + actualNumber * FRAME_INTERVAL;
        bkwdSurf.endTimeStamp  = CURRENT_TIME_STAMP + (actualNumber + 1) * FRAME_INTERVAL;

        if(m_surf[VPP_IN].size() > 0) // input in system memory
        {
            if(m_surfQueue[refIndx].bUpdate)
            {
                m_surfQueue[refIndx].bUpdate = false;
                m_surf[VPP_IN][m_surfQueue[refIndx].resIdx].SetFree(false);
            }
        }

        pTask->m_refList.push_back(bkwdSurf);
        actualNumber++;
    }

    // Current input
    {
        pTask->input = m_surfQueue[pTask->bkwdRefCount];

        // BOB uses previous input for odd frames and current input for even frames
        if (pTask->bkwdRefCount == 0 && pTask->taskIndex > 0)
        {
            actualNumber++; // increase for BOB as there are no reference frame
            if ((pTask->taskIndex % 2) == 0)
            {
                pTask->input = m_surfQueue[CURRENT_INPUT];
            }
        }

        pTask->input.timeStamp     = CURRENT_TIME_STAMP + actualNumber * FRAME_INTERVAL;
        pTask->input.endTimeStamp  = CURRENT_TIME_STAMP + (actualNumber + 1) * FRAME_INTERVAL;
        if(m_surf[VPP_IN].size() > 0) // input in system memory
        {
            if(m_surfQueue[pTask->bkwdRefCount].bUpdate)
            {
                m_surfQueue[pTask->bkwdRefCount].bUpdate = false;
                m_surf[VPP_IN][m_surfQueue[pTask->bkwdRefCount].resIdx].SetFree(false);
            }
        }
    }

    // output
    {
#ifdef MFX_ENABLE_MCTF
        pTask->output.pSurf = pOutSurface;
        pTask->output.timeStamp = pTask->input.timeStamp;
        pTask->outputForApp.pSurf = pOutSurfaceForApp;
        pTask->outputForApp.timeStamp = pTask->input.timeStamp;
        if (m_surf[VPP_OUT].size() > 0) // out in system memory
        {
            if (pTask->outputForApp.pSurf)
                m_surf[VPP_OUT][pTask->outputForApp.resIdx].SetFree(false);
        }
#else
        pTask->output.pSurf     = pOutSurface;
        pTask->output.timeStamp = pTask->input.timeStamp;
        if(m_surf[VPP_OUT].size() > 0) // out in system memory
        {
            m_surf[VPP_OUT][pTask->output.resIdx].SetFree(false);
        }
#endif

        actualNumber++;
    }

    if(m_pSubResource)
    {
        pTask->pSubResource = m_pSubResource;
    }

    // MARKER: last frame in current Task Slot
    // after resource are filled we can update generall container and state
    if( m_outputIndex == 0 )
    {
        size_t numFramesToRemove = m_pSubResource->surfaceListForRelease.size();

        for(size_t i = 0; i < numFramesToRemove; i++)
        {
            m_surfQueue.erase( m_surfQueue.begin() );
        }

        // correct output pts for target output
        pTask->output.timeStamp = (pTask->input.timeStamp + pTask->input.endTimeStamp) >> 1;
#ifdef MFX_ENABLE_MCTF
        pTask->outputForApp.timeStamp = (pTask->input.timeStamp + pTask->input.endTimeStamp) >> 1;
#endif
    }

    // update
    if(pTask->taskIndex > 0 && !(pTask->taskIndex & 1))
    {
        m_actualNumber++;
    }


    return MFX_ERR_NONE;

} // void ResMngr::FillTaskForMode30i60p(...)
//---------------------------------------------------------


mfxStatus ResMngr::FillTask(
    DdiTask* pTask,
    mfxFrameSurface1 * /*pInSurface*/,
    mfxFrameSurface1 *pOutSurface
#ifdef MFX_ENABLE_MCTF
    , mfxFrameSurface1 * pOutSurfaceForApp
#endif
)
{
    mfxU32 refIndx = 0;

    // bkwd
    pTask->bkwdRefCount = m_bkwdRefCount;//we set real bkw frames
    mfxU32 actualNumber = m_actualNumber;
    for(refIndx = 0; refIndx < pTask->bkwdRefCount; refIndx++)
    {
        ExtSurface bkwdSurf = m_surfQueue[refIndx];
        bkwdSurf.timeStamp     = CURRENT_TIME_STAMP + actualNumber * FRAME_INTERVAL;
        bkwdSurf.endTimeStamp  = CURRENT_TIME_STAMP + (actualNumber + 1) * FRAME_INTERVAL;

        if(m_surf[VPP_IN].size() > 0) // input in system memory
        {
            if(m_surfQueue[refIndx].bUpdate)
            {
                m_surfQueue[refIndx].bUpdate = false;
                m_surf[VPP_IN][m_surfQueue[refIndx].resIdx].SetFree(false);
            }
        }
        //

        pTask->m_refList.push_back(bkwdSurf);
        actualNumber++;
    }

    // input
    {
        pTask->input = m_surfQueue[pTask->bkwdRefCount];
        pTask->input.timeStamp     = CURRENT_TIME_STAMP + actualNumber * FRAME_INTERVAL;
        pTask->input.endTimeStamp  = CURRENT_TIME_STAMP + (actualNumber + 1) * FRAME_INTERVAL;
        if(m_surf[VPP_IN].size() > 0) // input in system memory
        {
            if(m_surfQueue[pTask->bkwdRefCount].bUpdate)
            {
                m_surfQueue[pTask->bkwdRefCount].bUpdate = false;
                m_surf[VPP_IN][m_surfQueue[pTask->bkwdRefCount].resIdx].SetFree(false);
            }
        }
    }

    // output
    {
#ifdef MFX_ENABLE_MCTF
        // todo: modify by outputForApp
        pTask->output.pSurf = pOutSurface;
        pTask->output.timeStamp = pTask->input.timeStamp + m_indxOutTimeStamp * (FRAME_INTERVAL / m_outputIndexCountPerCycle);

        pTask->outputForApp.pSurf = pOutSurfaceForApp;
        pTask->outputForApp.timeStamp = pTask->output.timeStamp;

        if (m_surf[VPP_OUT].size() > 0) // out in system memory
        {
            if (pTask->outputForApp.pSurf)
                m_surf[VPP_OUT][pTask->outputForApp.resIdx].SetFree(false);
        }
#else
        pTask->output.pSurf     = pOutSurface;
        pTask->output.timeStamp = pTask->input.timeStamp + m_indxOutTimeStamp * (FRAME_INTERVAL / m_outputIndexCountPerCycle);
        if(m_surf[VPP_OUT].size() > 0) // out in system memory
        {
            m_surf[VPP_OUT][pTask->output.resIdx].SetFree(false);
        }
#endif

        actualNumber++;
    }

    // fwd
    pTask->fwdRefCount = m_fwdRefCount;
    for(refIndx = 0; refIndx < pTask->fwdRefCount; refIndx++)
    {
        mfxU32 fwdIdx = pTask->bkwdRefCount + 1 + refIndx;
        ExtSurface fwdSurf = m_surfQueue[fwdIdx];
        fwdSurf.timeStamp     = CURRENT_TIME_STAMP + actualNumber * FRAME_INTERVAL;
        fwdSurf.endTimeStamp  = CURRENT_TIME_STAMP + (actualNumber + 1) * FRAME_INTERVAL;

        if(m_surf[VPP_IN].size() > 0) // input in system memory
        {
            if(fwdSurf.bUpdate)
            {
                m_surfQueue[fwdIdx].bUpdate = false;
                m_surf[VPP_IN][m_surfQueue[fwdIdx].resIdx].SetFree(false);
            }
        }
        pTask->m_refList.push_back(fwdSurf);
        if (m_pSubResource && refIndx && m_multiBlt)
        {
            m_pSubResource->subTasks.push_back(pTask->taskIndex + refIndx);
        }
        actualNumber++;
    }

    if(m_pSubResource)
    {
        pTask->pSubResource = m_pSubResource;
    }

    // MARKER: last frame in current Task Slot
    // after resource are filled we can update generall container and state
    if( m_outputIndex == 0 )
    {
        size_t numFramesToRemove = m_pSubResource->surfaceListForRelease.size();

        for(size_t i = 0; i < numFramesToRemove; i++)
        {
            m_surfQueue.erase( m_surfQueue.begin() );
        }

        m_inputIndex -= (mfxU32)numFramesToRemove;
        m_bkwdRefCount = GetNextBkwdRefCount();
    }

    return MFX_ERR_NONE;

} // void ResMngr::FillTask(...)


mfxStatus ResMngr::CompleteTask(DdiTask *pTask)
{
    if(pTask->pSubResource)
    {
        pTask->pSubResource->refCount--;
    }

    mfxStatus sts =  ReleaseSubResource(false); // false mean release subResource with RefCnt == 0 only
    MFX_CHECK_STS(sts);

    return MFX_ERR_NONE;

} // mfxStatus ResMngr::CompleteTask(DdiTask *pTask)

mfxU32 ResMngr::GetSubTask(DdiTask *pTask)
{
    if (pTask && pTask->pSubResource && pTask->pSubResource->subTasks.size())
        return pTask->pSubResource->subTasks[0];
    else
        return NO_INDEX;
}

mfxStatus ResMngr::DeleteSubTask(DdiTask *pTask, mfxU32 subtaskIdx)
{
    if (pTask && pTask->pSubResource)
    {
        std::vector<mfxU32>::iterator sub_it = find(pTask->pSubResource->subTasks.begin(), pTask->pSubResource->subTasks.end(), subtaskIdx);
        if (sub_it != pTask->pSubResource->subTasks.end())
        {
            pTask->pSubResource->subTasks.erase(sub_it);
            return MFX_ERR_NONE;
        }
    }
    return MFX_ERR_NOT_FOUND;
}

bool ResMngr::IsMultiBlt()
{
    return m_multiBlt;
}

////////////////////////////////////////////////////////////////////////////////////
// TaskManager
////////////////////////////////////////////////////////////////////////////////////

TaskManager::TaskManager()
{
    m_mode30i60p.m_numOutputFrames    = 0;
    m_mode30i60p.m_prevInputTimeStamp = 0;

    m_taskIndex = 0;
    m_actualNumber = 0;
    m_core = 0;

    m_mode30i60p.SetEnable(false);

    m_extMode = 0;
#ifdef MFX_ENABLE_MCTF
    m_MCTFSurfacesInQueue = 0;
#endif

} // TaskManager::TaskManager()

TaskManager::~TaskManager()
{
    Close();

} // TaskManager::~TaskManager(void)

mfxStatus TaskManager::Init(
    VideoCORE* core,
    Config & config)
{
    m_taskIndex    = 0;
    m_actualNumber = 0;
    m_core         = core;

    // init param
    m_mode30i60p.SetEnable(config.m_bMode30i60pEnable);
    m_extMode           = config.m_extConfig.mode;

    m_mode30i60p.m_numOutputFrames    = 0;
    m_mode30i60p.m_prevInputTimeStamp = 0;

    m_cpuFrc.Reset(m_extMode, config.m_extConfig.frcRational);

    m_resMngr.Init(config, this->m_core);

    m_tasks.resize(config.m_surfCount[VPP_OUT]);

#ifdef MFX_ENABLE_MCTF
    m_MCTFSurfacesInQueue = 0;
#endif

    return MFX_ERR_NONE;

} // mfxStatus TaskManager::Init(void)


mfxStatus TaskManager::Close(void)
{
    m_actualNumber = m_taskIndex = 0;

    Clear(m_tasks);

    m_core     = NULL;

    m_mode30i60p.SetEnable(false);
    m_extMode = 0;

    RateRational frcRational[2];
    frcRational[VPP_IN].FrameRateExtN = 30;
    frcRational[VPP_IN].FrameRateExtD = 1;
    frcRational[VPP_OUT].FrameRateExtN = 30;
    frcRational[VPP_OUT].FrameRateExtD = 1;
    m_cpuFrc.Reset(m_extMode, frcRational);

    m_resMngr.Close(); // release all resource

    return MFX_ERR_NONE;

} // mfxStatus TaskManager::Close(void)


mfxStatus TaskManager::DoCpuFRC_AndUpdatePTS(
    mfxFrameSurface1 *input,
    mfxFrameSurface1 *output,
    mfxStatus *intSts)
{
    return m_cpuFrc.DoCpuFRC_AndUpdatePTS(input, output, intSts);

} // mfxStatus TaskManager::::DoCpuFRC_AndUpdatePTS(...)


mfxStatus TaskManager::DoAdvGfx(
    mfxFrameSurface1 *input,
    mfxFrameSurface1 *output,
    mfxStatus *intSts)
{
    return m_resMngr.DoAdvGfx(input, output, intSts);

} // mfxStatus TaskManager::DoAdvGfx(...)


mfxStatus TaskManager::AssignTask(
    mfxFrameSurface1 *input,
    mfxFrameSurface1 *output,
#ifdef MFX_ENABLE_MCTF
    mfxFrameSurface1 *outputForApp,
#endif
    mfxExtVppAuxData *aux,
    DdiTask*& pTask,
    mfxStatus& intSts)
{
    UMC::AutomaticUMCMutex guard(m_mutex);

    // no state machine in VPP, so transition to previous state isn't possible.
    // it is the simplest way to resolve issue with MFX_WRN_DEVICE_BUSY, but performance could be affected
    pTask = GetTask();
    if(NULL == pTask)
    {
        return MFX_WRN_DEVICE_BUSY;
    }

    pTask->input.bForcedInternalAlloc = false;
    pTask->output.bForcedInternalAlloc = false;
#ifdef MFX_ENABLE_MCTF
    pTask->outputForApp.bForcedInternalAlloc = false;
    auto mctf = pMCTF.lock();
    if (mctf)
    {
        mfxExtVppMctf* mctf_ctrl = input ? reinterpret_cast<mfxExtVppMctf *>(GetExtendedBuffer(input->Data.ExtParam, input->Data.NumExtParam, MFX_EXTBUFF_VPP_MCTF)) : nullptr;
        pTask->bMCTF = true;

        // public or private runtime control
        if (mctf_ctrl)
        {
            pTask->MctfControlActive = true;
            CMC::FillParamControl(&pTask->MctfData, mctf_ctrl);
        }
        else
        {
            CMC::QueryDefaultParams(&pTask->MctfData);
            pTask->MctfControlActive = false;
        }
    }
    else
    {
        pTask->bMCTF = false;
        CMC::QueryDefaultParams(&pTask->MctfData);
    }
#endif
    //--------------------------------------------------------
    mfxStatus sts = MFX_ERR_NONE;

    bool isAdvGfxMode = (COMPOSITE & m_extMode) || (FRC_INTERPOLATION & m_extMode) ||
                                            (VARIANCE_REPORT & m_extMode) || (IS_REFERENCES & m_extMode)? true : false;
#ifdef MFX_ENABLE_MCTF
    // calls below may change only TimeStamp & FrameOrder, so
    // no passing of outputForApp as TimeStamp & FrameOrder
    // will be copied then.
#endif
    if( isAdvGfxMode )
    {
        sts = DoAdvGfx(input, output, &intSts);
    }
    else if ((FRC_ENABLED & m_extMode) && (!m_mode30i60p.IsEnabled()) )
    {
        sts = DoCpuFRC_AndUpdatePTS(input, output, &intSts);
    }
    else if( m_mode30i60p.IsEnabled() )
    {
        if( (NULL == input) && (FRC_DISTRIBUTED_TIMESTAMP & m_extMode) )
        {
            sts = MFX_ERR_MORE_DATA;
        }
        //-------------------------------------------------
        if (MFX_ERR_MORE_DATA != sts)
            sts = m_resMngr.DoMode30i60p(input, output, &intSts);
    }
    else // simple mode
    {
        if (NULL == input)
            sts = MFX_ERR_MORE_DATA;
    }

#ifdef MFX_ENABLE_MCTF

    MFX_CHECK_NULL_PTR1(output);

    // DoCpuFRC_AndUpdatePTS updates TimeStamp & FrameOrder in output;
    // copy these values to outputForApp
    if (outputForApp != output && outputForApp)
    {
        outputForApp->Data.TimeStamp = output->Data.TimeStamp;
        outputForApp->Data.FrameOrder = output->Data.FrameOrder;
    }

    // this condition means the end of the stream; for MCTF it stands for
    // pushing everything out of an internal queue (if any)
    if (!input && MFX_ERR_MORE_DATA == sts && pTask->bMCTF)
    {
        // if MCTF is enabled & it is in a mode with > 1 ref,
        // and we approached the end, fill the task with only
        // output surface
        if (TWO_REFERENCES == mctf->MCTF_GetReferenceNumber() || FOUR_REFERENCES == mctf->MCTF_GetReferenceNumber())
        {
            sts = FillLastTasks(pTask, output, outputForApp, aux);
        }

        if (MFX_ERR_NONE == sts)
            sts = MFX_ERR_MORE_DATA;
    }
#endif
    MFX_CHECK_STS(sts);


    sts = FillTask(pTask, input, output,
#ifdef MFX_ENABLE_MCTF
        outputForApp,
#endif
        aux);
    MFX_CHECK_STS(sts);

#ifdef MFX_ENABLE_MCTF
    if (pTask->bMCTF)
    {
        // if we here, sts == MFX_ERR_NONE;
        ++m_MCTFSurfacesInQueue;
        // m_MCTFSurfacesInQueue means number of frames including the current;
        // in case of 2 refs or 4 refs, its enough to have 2 or 3 to output.
        // Effectively, it is used to count number of forward refs + current;
        // Semantic can be changed to track how many frames are in queue (it 
        // cannot be greater than 3 for 2 refs, or 5 for 4 refs.
        switch (mctf->MCTF_GetReferenceNumber())
        {
        case NO_REFERENCES:
        case ONE_REFERENCE:
            m_MCTFSurfacesInQueue = 1;
            break;
        case TWO_REFERENCES:
            if (m_MCTFSurfacesInQueue < 2) 
            {
                // MCTF cannot work with FRC if it uses 2-refs
                if (MFX_ERR_MORE_SURFACE == intSts)
                    return MFX_ERR_UNDEFINED_BEHAVIOR;

                intSts = MFX_ERR_MORE_DATA_SUBMIT_TASK;
            }
            else
                m_MCTFSurfacesInQueue = 2;
            break;
        case FOUR_REFERENCES:
            if (m_MCTFSurfacesInQueue < 3)
            {
                // MCTF cannot work with FRC if it uses 4-refs
                if (MFX_ERR_MORE_SURFACE == intSts)
                    return MFX_ERR_UNDEFINED_BEHAVIOR;
                intSts = MFX_ERR_MORE_DATA_SUBMIT_TASK;
            }
            else
                m_MCTFSurfacesInQueue = 3;
            break;
        }
    }
#endif

    if (m_mode30i60p.IsEnabled())
    {
        UpdatePTS_Mode30i60p(
            input,
            output,
            pTask->taskIndex,
            &intSts);
    }
    else if ( !(FRC_ENABLED & m_extMode) && !isAdvGfxMode )
    {
        UpdatePTS_SimpleMode(input, output);
    }
    else if (IS_REFERENCES & m_extMode) /* case for ADI with references */
    {
        UpdatePTS_SimpleMode(input, output);
    }

#ifdef MFX_ENABLE_MCTF
    // update functions may update TimeStamp & FrameOrder
    if (outputForApp != output && outputForApp)
    {
        outputForApp->Data.TimeStamp = output->Data.TimeStamp;
        outputForApp->Data.FrameOrder = output->Data.FrameOrder;
    }
#endif

    return MFX_ERR_NONE;

} // mfxStatus TaskManager::AssignTask(...)


mfxStatus TaskManager::CompleteTask(DdiTask* pTask)
{
    UMC::AutomaticUMCMutex guard(m_mutex);

#ifdef MFX_ENABLE_MCTF
    if (pTask->output.pSurf != pTask->outputForApp.pSurf && pTask->outputForApp.pSurf)
        MFX_SAFE_CALL(m_core->DecreaseReference(&(pTask->outputForApp.pSurf->Data)));
#endif

    mfxStatus sts = m_core->DecreaseReference( &(pTask->output.pSurf->Data) );
    MFX_CHECK_STS(sts);

#ifdef MFX_ENABLE_MCTF
    mfxU32 freeIdx = pTask->outputForApp.resIdx;
#else
    mfxU32 freeIdx = pTask->output.resIdx;
#endif
    if(NO_INDEX != freeIdx && m_resMngr.m_surf[VPP_OUT].size() > 0)
    {
        m_resMngr.m_surf[VPP_OUT][freeIdx].SetFree(true);
    }

    if(pTask->bAdvGfxEnable || m_mode30i60p.IsEnabled() )
    {
        sts = m_resMngr.CompleteTask(pTask);
        MFX_CHECK_STS(sts);
    }
    else // simple mode
    {
        freeIdx = pTask->input.resIdx;
        if(NO_INDEX != freeIdx && m_resMngr.m_surf[VPP_IN].size() > 0)
        {
            m_resMngr.m_surf[VPP_IN][freeIdx].SetFree(true);
        }
    }

    if (pTask->input.pSurf)
        sts = m_core->DecreaseReference( &(pTask->input.pSurf->Data) );
    MFX_CHECK_STS(sts);

    FreeTask(pTask);

    return MFX_TASK_DONE;


} // mfxStatus TaskManager::CompleteTask(DdiTask* pTask)


DdiTask* TaskManager::GetTask(void)
{
    mfxU32 indx = FindFreeSurface(m_tasks);
    if(NO_INDEX == indx) return NULL;

    m_tasks[indx].skipQueryStatus = false;

    return &(m_tasks[indx]);

} // DdiTask* TaskManager::CreateTask(void)


void TaskManager::FillTask_Mode30i60p(
    DdiTask* pTask,
    mfxFrameSurface1 *pInSurface,
    mfxFrameSurface1 *pOutSurface
#ifdef MFX_ENABLE_MCTF
    ,mfxFrameSurface1 *pOutSurfaceForApp
#endif
    )
{
    m_resMngr.FillTaskForMode30i60p(pTask, 
                                    pInSurface,
                                    pOutSurface
#ifdef MFX_ENABLE_MCTF
                                    ,pOutSurfaceForApp
#endif
                                   );

} // void TaskManager::FillTask_60i60pMode(DdiTask* pTask)


void TaskManager::FillTask_AdvGfxMode(
    DdiTask* pTask,
    mfxFrameSurface1 *pInSurface,
    mfxFrameSurface1 *pOutSurface
#ifdef MFX_ENABLE_MCTF
    ,mfxFrameSurface1 *pOutSurfaceForApp
#endif
    )
{
    m_resMngr.FillTask(pTask, pInSurface, pOutSurface
#ifdef MFX_ENABLE_MCTF
                      , pOutSurfaceForApp
#endif
    );

} // void TaskManager::FillTask_AdvGfxMode(...)


mfxStatus TaskManager::FillTask(
    DdiTask* pTask,
    mfxFrameSurface1 *pInSurface,
    mfxFrameSurface1 *pOutSurface,
#ifdef MFX_ENABLE_MCTF
    mfxFrameSurface1 *pOutSurfaceForApp,
#endif
    mfxExtVppAuxData *aux)
{
    // [0] Set param
    pTask->input.pSurf   = pInSurface;
    pTask->input.resIdx  = NO_INDEX;
    pTask->output.pSurf  = pOutSurface;
    pTask->output.resIdx = NO_INDEX;
#ifdef MFX_ENABLE_MCTF
    pTask->outputForApp.pSurf = pOutSurfaceForApp;
    pTask->outputForApp.resIdx = NO_INDEX;
#endif

    if(m_resMngr.m_surf[VPP_OUT].size() > 0) // out in system memory
    {
#ifdef MFX_ENABLE_MCTF
        // it might be nullptr, meaning MCTF works with delay & its just started
        // if it is not ready to ouptut, we cannot lock & change output surface
        // by no means.
        if (pTask->outputForApp.pSurf)
        {
            // resIdx is required to only for outputForApp as only this surface
            // is the actual surface allocated by an App; output might be an internal
            // buffer; so, lets update resIdx only in outputForApp.
            pTask->outputForApp.resIdx = FindFreeSurface(m_resMngr.m_surf[VPP_OUT]);
            if (NO_INDEX == pTask->outputForApp.resIdx) return MFX_WRN_DEVICE_BUSY;
            m_resMngr.m_surf[VPP_OUT][pTask->outputForApp.resIdx].SetFree(false);
            if (pTask->outputForApp.pSurf == pTask->output.pSurf)
                pTask->output.resIdx = pTask->outputForApp.resIdx;
        }
#else
        pTask->output.resIdx     = FindFreeSurface( m_resMngr.m_surf[VPP_OUT] );
        if( NO_INDEX == pTask->output.resIdx) return MFX_WRN_DEVICE_BUSY;
        m_resMngr.m_surf[VPP_OUT][pTask->output.resIdx].SetFree(false);
#endif
    }

    pTask->taskIndex    = m_taskIndex++;
    if ((m_taskIndex + pTask->fwdRefCount) >= NO_INDEX)
        m_taskIndex = 1;

    pTask->input.timeStamp     = CURRENT_TIME_STAMP + m_actualNumber * FRAME_INTERVAL;
    pTask->input.endTimeStamp  = CURRENT_TIME_STAMP + (m_actualNumber + 1) * FRAME_INTERVAL;
    pTask->output.timeStamp    = pTask->input.timeStamp;
#ifdef MFX_ENABLE_MCTF
    pTask->outputForApp.timeStamp = pTask->output.timeStamp;
#endif

    pTask->bAdvGfxEnable     = (COMPOSITE & m_extMode) || (FRC_INTERPOLATION & m_extMode) ||
                                                         (VARIANCE_REPORT & m_extMode) || (IS_REFERENCES & m_extMode)? true : false;

    pTask->pAuxData = aux;

    if( m_mode30i60p.IsEnabled() )
    {
        FillTask_Mode30i60p(
            pTask,
            pInSurface,
            pOutSurface
#ifdef MFX_ENABLE_MCTF
            , pOutSurfaceForApp
#endif
        );
    }
    else if(pTask->bAdvGfxEnable)
    {
        FillTask_AdvGfxMode(
            pTask,
            pInSurface,
            pOutSurface
#ifdef MFX_ENABLE_MCTF
            , pOutSurfaceForApp
#endif
        );
        if ((COMPOSITE & m_extMode) && m_resMngr.IsMultiBlt())
            m_taskIndex += pTask->fwdRefCount;

    }
    else // simple mode
    {
        if(m_resMngr.m_surf[VPP_IN].size() > 0) // input in system memory
        {
            pTask->input.bUpdate    = true;
            pTask->input.resIdx     = FindFreeSurface( m_resMngr.m_surf[VPP_IN] );
            if( NO_INDEX == pTask->input.resIdx) return MFX_WRN_DEVICE_BUSY;
            m_resMngr.m_surf[VPP_IN][pTask->input.resIdx].SetFree(false);
        }
    }

    m_actualNumber += 1; // make sense for simple mode only

    mfxStatus sts = m_core->IncreaseReference( &(pTask->input.pSurf->Data) );
    MFX_CHECK_STS(sts);

    sts = m_core->IncreaseReference( &(pTask->output.pSurf->Data) );
    MFX_CHECK_STS(sts);

#ifdef MFX_ENABLE_MCTF
    // prevent double-lock if the same surface is passed an output & outputForApp
    if (pTask->output.pSurf != pTask->outputForApp.pSurf && pTask->outputForApp.pSurf)
    {
        sts = m_core->IncreaseReference(&(pTask->outputForApp.pSurf->Data));
        MFX_CHECK_STS(sts);
    }
#endif

    pTask->SetFree(false);

    return MFX_ERR_NONE;

} // mfxStatus TaskManager::FillTask(...)

#ifdef MFX_ENABLE_MCTF
mfxStatus TaskManager::FillLastTasks(
    DdiTask* pTask,
    mfxFrameSurface1 *pOutSurface,
    mfxFrameSurface1 *pOutSurfaceForApp,
    mfxExtVppAuxData *aux
)
{
    mfxStatus sts = MFX_ERR_NONE;
    // [0] Set param
    pTask->input.pSurf = NULL;
    pTask->input.resIdx = NO_INDEX;
    pTask->output.pSurf = pOutSurface;
    pTask->output.resIdx = NO_INDEX;
    pTask->outputForApp.pSurf = pOutSurfaceForApp;
    pTask->outputForApp.resIdx = NO_INDEX;

    if (m_resMngr.m_surf[VPP_OUT].size() > 0) // out in system memory
    {
#ifdef MFX_ENABLE_MCTF
        // resIdx is required to only for outputForApp as only this surface
        // is the actual surface allocated by an App; output might be an internal
        // buffer; so, lets update resIdx only in outputForApp.
        pTask->outputForApp.resIdx = FindFreeSurface(m_resMngr.m_surf[VPP_OUT]);
        if (NO_INDEX == pTask->outputForApp.resIdx) return MFX_WRN_DEVICE_BUSY;
        m_resMngr.m_surf[VPP_OUT][pTask->outputForApp.resIdx].SetFree(false);
        if (pTask->outputForApp.pSurf == pTask->output.pSurf)
            pTask->output.resIdx = pTask->outputForApp.resIdx;
#else
        pTask->output.resIdx = FindFreeSurface(m_resMngr.m_surf[VPP_OUT]);
        if (NO_INDEX == pTask->output.resIdx) return MFX_WRN_DEVICE_BUSY;
        m_resMngr.m_surf[VPP_OUT][pTask->output.resIdx].SetFree(false);
#endif
    }

    pTask->taskIndex = m_taskIndex++;
    pTask->output.timeStamp = CURRENT_TIME_STAMP + m_actualNumber * FRAME_INTERVAL;
    pTask->outputForApp.timeStamp = pTask->output.timeStamp;
    pTask->output.pSurf->Data.TimeStamp = pTask->output.timeStamp;
    pTask->pAuxData = aux;

    m_actualNumber += 1; // make sense for simple mode only

    sts = m_core->IncreaseReference(&(pTask->output.pSurf->Data));
    MFX_CHECK_STS(sts);

    // prevent double-lock if the same surface is passed an output & outputForApp
    if (pTask->output.pSurf != pTask->outputForApp.pSurf && pTask->outputForApp.pSurf)
    {
        pTask->outputForApp.pSurf->Data.TimeStamp = pTask->output.pSurf->Data.TimeStamp;
        pTask->outputForApp.pSurf->Data.FrameOrder = pTask->output.pSurf->Data.FrameOrder;

        sts = m_core->IncreaseReference(&(pTask->outputForApp.pSurf->Data));
        MFX_CHECK_STS(sts);
    }

    pTask->SetFree(false);
    return sts;

} // mfxStatus TaskManager::FillLastTasks(...)
#endif

void TaskManager::UpdatePTS_Mode30i60p(
    mfxFrameSurface1 *input,
    mfxFrameSurface1 *output,
    mfxU32 taskIndex,
    mfxStatus *intSts)
{
    if (output == nullptr)
    {
        *intSts = MFX_ERR_MORE_SURFACE;
        return;
    }

    if (input == nullptr)
    {
        output->Data.TimeStamp = (mfxU64)MFX_TIME_STAMP_INVALID;
        output->Data.FrameOrder = (mfxU32)MFX_FRAMEORDER_UNKNOWN;
        *intSts = MFX_ERR_MORE_SURFACE;
    }
    else if ( (FRC_STANDARD & m_extMode) || (FRC_DISABLED == m_extMode) )
    {
        if (0 != taskIndex % 2)
        {
            output->Data.TimeStamp = (mfxU64) MFX_TIME_STAMP_INVALID;
            output->Data.FrameOrder = (mfxU32) MFX_FRAMEORDER_UNKNOWN;
            *intSts = MFX_ERR_MORE_SURFACE;
        }
        else
        {
            output->Data.TimeStamp = input->Data.TimeStamp;
            output->Data.FrameOrder = input->Data.FrameOrder;
            *intSts = MFX_ERR_NONE;
        }
    }
    else if (FRC_DISTRIBUTED_TIMESTAMP & m_extMode)
    {
        if (0 != taskIndex % 2)
        {
            output->Data.TimeStamp = m_mode30i60p.m_prevInputTimeStamp + (input->Data.TimeStamp - m_mode30i60p.m_prevInputTimeStamp) / 2;
            *intSts = MFX_ERR_MORE_SURFACE;
        }
        else
        {
            m_mode30i60p.m_prevInputTimeStamp = input->Data.TimeStamp;
            output->Data.TimeStamp = input->Data.TimeStamp;
            *intSts = MFX_ERR_NONE;
        }

        output->Data.FrameOrder = m_mode30i60p.m_numOutputFrames;
        m_mode30i60p.m_numOutputFrames += 1;
    }

    return;

} // void TaskManager::UpdatePTS_Mode30i60p(...)

mfxU32 TaskManager::GetSubTask(DdiTask *pTask) { return m_resMngr.GetSubTask(pTask); }
mfxStatus TaskManager::DeleteSubTask(DdiTask *pTask, mfxU32 subtaskIdx) { return m_resMngr.DeleteSubTask(pTask, subtaskIdx); }

void TaskManager::UpdatePTS_SimpleMode(
    mfxFrameSurface1 *input,
    mfxFrameSurface1 *output)
{
    if (input && output)
    {
        output->Data.TimeStamp = input->Data.TimeStamp;
        output->Data.FrameOrder = input->Data.FrameOrder;
    }
} // void UpdatePTS_SimpleMode(...)


////////////////////////////////////////////////////////////////////////////////////
// VideoHWVPP Component: platform independent code
////////////////////////////////////////////////////////////////////////////////////

mfxStatus  VideoVPPHW::CopyPassThrough(mfxFrameSurface1 *pInputSurface, mfxFrameSurface1 *pOutputSurface)
{
    mfxStatus sts = MFX_ERR_NONE;

    mfxU16 srcPattern = 0, dstPattern = 0;
    switch (m_ioMode)
    {
        case SYS_TO_SYS:
            srcPattern = MFX_MEMTYPE_SYSTEM_MEMORY;
            dstPattern = MFX_MEMTYPE_SYSTEM_MEMORY;
            break;
        case SYS_TO_D3D:
            srcPattern = MFX_MEMTYPE_SYSTEM_MEMORY;
            dstPattern = MFX_MEMTYPE_DXVA2_DECODER_TARGET;
            break;
        case D3D_TO_SYS:
            srcPattern = MFX_MEMTYPE_DXVA2_DECODER_TARGET;
            dstPattern = MFX_MEMTYPE_SYSTEM_MEMORY;
            break;
        case D3D_TO_D3D:
            srcPattern = MFX_MEMTYPE_DXVA2_DECODER_TARGET;
            dstPattern = MFX_MEMTYPE_DXVA2_DECODER_TARGET;
            break;
        default:
            return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    if (m_pCore->IsExternalFrameAllocator())
    {
        if (m_IOPattern & MFX_IOPATTERN_IN_OPAQUE_MEMORY)
            srcPattern |= MFX_MEMTYPE_INTERNAL_FRAME;
        else
            srcPattern |= MFX_MEMTYPE_EXTERNAL_FRAME;

        if (m_IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY)
            dstPattern |= MFX_MEMTYPE_INTERNAL_FRAME;
        else
            dstPattern |= MFX_MEMTYPE_EXTERNAL_FRAME;
    }
    else
    {
        srcPattern |= MFX_MEMTYPE_INTERNAL_FRAME;
        dstPattern |= MFX_MEMTYPE_INTERNAL_FRAME;
    }

    sts = m_pCore->DoFastCopyWrapper(pOutputSurface,
        dstPattern,
        pInputSurface,
        srcPattern);

    MFX_CHECK_STS(sts);

    return MFX_ERR_NONE;

} // mfxStatus VideoVPPHW::CopyPassThrough(mfxFrameSurface1 *pInputSurface, mfxFrameSurface1 *pOutputSurface)


VideoVPPHW::VideoVPPHW(IOMode mode, VideoCORE *core)
:m_asyncDepth(ACCEPTED_DEVICE_ASYNC_DEPTH)
,m_executeParams()
,m_executeSurf()
,m_pCore(core)
,m_guard()
,m_workloadMode(VPP_SYNC_WORKLOAD)
,m_IOPattern(0)
,m_ioMode(mode)
,m_taskMngr()
,m_scene_change(0)
,m_frame_num(0)
,m_critical_error(MFX_ERR_NONE)
,m_ddi(NULL)
,m_bMultiView(false)

#ifdef MFX_ENABLE_MCTF
,m_MctfIsFlushing(false)
,m_bMctfAllocatedMemory(false)
,m_pMctfCmDevice(nullptr)
#endif

// cm devices
,m_pCmCopy(NULL)
#if defined(MFX_ENABLE_SCENE_CHANGE_DETECTION_VPP)
,m_SCD()
#endif
,m_pCmDevice(NULL)
,m_pCmProgram(NULL)
,m_pCmKernel(NULL)
,m_pCmQueue(NULL)
{
    m_config.m_bRefFrameEnable = false;
    m_config.m_bMode30i60pEnable = false;
    m_config.m_bWeave = false;
    m_config.m_extConfig.mode  = FRC_DISABLED;
    m_config.m_bCopyPassThroughEnable = false;
    m_config.m_surfCount[VPP_IN]   = 1;
    m_config.m_surfCount[VPP_OUT]  = 1;
    m_config.m_IOPattern = 0;
    m_config.m_multiBlt = false;

    MemSetZero4mfxExecuteParams(&m_executeParams);
    memset(&m_params, 0, sizeof(mfxVideoParam));
#ifdef MFX_ENABLE_MCTF
    memset(&m_MctfMfxAlocResponse, 0, sizeof(m_MctfMfxAlocResponse));
    m_bMctfAllocatedMemory = false;
#endif
} // VideoVPPHW::VideoVPPHW(IOMode mode, VideoCORE *core)


VideoVPPHW::~VideoVPPHW()
{
    Close();

} // VideoVPPHW::~VideoVPPHW()


mfxStatus VideoVPPHW::GetVideoParams(mfxVideoParam *par) const
{
    MFX_CHECK_NULL_PTR1(par);

    /* Step 1: Fill filters in use in DOUSE if specified.
     *  This step is skipped since it's done on upper level already
     */
    if( NULL == par->ExtParam || 0 == par->NumExtParam)
    {
        return MFX_ERR_NONE;
    }

    for( mfxU32 i = 0; i < par->NumExtParam; i++ )
    {
        MFX_CHECK_NULL_PTR1(par->ExtParam[i]);
        mfxU32 bufferId = par->ExtParam[i]->BufferId;
        if(MFX_EXTBUFF_VPP_DEINTERLACING == bufferId)
        {
            mfxExtVPPDeinterlacing *bufDI = reinterpret_cast<mfxExtVPPDeinterlacing *>(par->ExtParam[i]);
            MFX_CHECK_NULL_PTR1(bufDI);
            bufDI->Mode = static_cast<mfxU16>(m_executeParams.iDeinterlacingAlgorithm);
        }
        else if (MFX_EXTBUFF_VPP_DENOISE == bufferId)
        {
            mfxExtVPPDenoise *bufDN = reinterpret_cast<mfxExtVPPDenoise *>(par->ExtParam[i]);
            MFX_CHECK_NULL_PTR1(bufDN);
            bufDN->DenoiseFactor = m_executeParams.denoiseFactorOriginal;
        }
#ifdef MFX_ENABLE_MCTF
        else if (MFX_EXTBUFF_VPP_MCTF == bufferId)
        {
            mfxExtVppMctf *bufMCTF = reinterpret_cast<mfxExtVppMctf *>(par->ExtParam[i]);
            MFX_CHECK_NULL_PTR1(bufMCTF);
            bufMCTF->FilterStrength = m_executeParams.MctfFilterStrength;
#ifdef MFX_ENABLE_MCTF_EXT
            bufMCTF->BitsPerPixelx100k = m_executeParams.MctfBitsPerPixelx100k;
            bufMCTF->Deblocking = m_executeParams.MctfDeblocking;
            bufMCTF->Overlap = m_executeParams.MctfOverlap;
            bufMCTF->MVPrecision = m_executeParams.MctfMVPrecision;
            bufMCTF->TemporalMode = m_executeParams.MctfTemporalMode;
#endif
        }
#endif
        else if (MFX_EXTBUFF_VPP_PROCAMP == bufferId)
        {
            mfxExtVPPProcAmp *bufPA = reinterpret_cast<mfxExtVPPProcAmp *>(par->ExtParam[i]);
            MFX_CHECK_NULL_PTR1(bufPA);
            bufPA->Brightness = m_executeParams.Brightness;
            bufPA->Contrast   = m_executeParams.Contrast;
            bufPA->Hue        = m_executeParams.Hue;
            bufPA->Saturation = m_executeParams.Saturation;
        }
        else if (MFX_EXTBUFF_VPP_DETAIL == bufferId)
        {
            mfxExtVPPDetail *bufDT = reinterpret_cast<mfxExtVPPDetail *>(par->ExtParam[i]);
            MFX_CHECK_NULL_PTR1(bufDT);
            bufDT->DetailFactor = m_executeParams.detailFactorOriginal;
        }
        else if (MFX_EXTBUFF_VIDEO_SIGNAL_INFO == bufferId)
        {
            mfxExtVPPVideoSignalInfo *bufVSI = reinterpret_cast<mfxExtVPPVideoSignalInfo *>(par->ExtParam[i]);
            MFX_CHECK_NULL_PTR1(bufVSI);
            bufVSI->In.NominalRange   = m_executeParams.VideoSignalInfoIn.NominalRange;
            bufVSI->In.TransferMatrix = m_executeParams.VideoSignalInfoIn.TransferMatrix;
            bufVSI->Out.NominalRange   = m_executeParams.VideoSignalInfoOut.NominalRange;
            bufVSI->Out.TransferMatrix = m_executeParams.VideoSignalInfoOut.TransferMatrix;
        }
        else if (MFX_EXTBUFF_VPP_COMPOSITE == bufferId)
        {
            mfxExtVPPComposite *bufComp = reinterpret_cast<mfxExtVPPComposite *>(par->ExtParam[i]);
            MFX_CHECK_NULL_PTR1(bufComp);
            MFX_CHECK_NULL_PTR1(bufComp->InputStream);
            bufComp->NumInputStream = static_cast<mfxU16>(m_executeParams.dstRects.size());
#if MFX_VERSION > 1023
            bufComp->NumTiles = static_cast<mfxU16>(m_executeParams.iTilesNum4Comp);
#endif
            for (size_t k = 0; k < m_executeParams.dstRects.size(); k++)
            {
                MFX_CHECK_NULL_PTR1(bufComp->InputStream);
                bufComp->InputStream[k].DstX = m_executeParams.dstRects[k].DstX;
                bufComp->InputStream[k].DstY = m_executeParams.dstRects[k].DstY;
                bufComp->InputStream[k].DstW = m_executeParams.dstRects[k].DstW;
                bufComp->InputStream[k].DstH = m_executeParams.dstRects[k].DstH;
#if MFX_VERSION > 1023
                bufComp->InputStream[k].TileId = (mfxU16)m_executeParams.dstRects[k].TileId;
#endif
                bufComp->InputStream[k].GlobalAlpha       = m_executeParams.dstRects[k].GlobalAlpha;
                bufComp->InputStream[k].GlobalAlphaEnable = m_executeParams.dstRects[k].GlobalAlphaEnable;
                bufComp->InputStream[k].LumaKeyEnable = m_executeParams.dstRects[k].LumaKeyEnable;
                bufComp->InputStream[k].LumaKeyMax    = m_executeParams.dstRects[k].LumaKeyMax;
                bufComp->InputStream[k].LumaKeyMin     = m_executeParams.dstRects[k].LumaKeyMin;
                bufComp->InputStream[k].PixelAlphaEnable = m_executeParams.dstRects[k].PixelAlphaEnable;
            }

            bufComp->R = (m_executeParams.iBackgroundColor >> 32 ) & 0xFF;
            bufComp->G = (m_executeParams.iBackgroundColor >> 16 ) & 0xFF;
            bufComp->B = (m_executeParams.iBackgroundColor >> 0  ) & 0xFF;
        }
        else if (MFX_EXTBUFF_VPP_FIELD_PROCESSING == bufferId)
        {
            mfxExtVPPFieldProcessing *bufFP = reinterpret_cast<mfxExtVPPFieldProcessing *>(par->ExtParam[i]);
            MFX_CHECK_NULL_PTR1(bufFP);
            mfxU32 mode = m_executeParams.iFieldProcessingMode - 1;
            if (FRAME2FRAME == mode)
            {
                bufFP->Mode     = MFX_VPP_COPY_FRAME;
            }
            else if (TFF2TFF == mode)
            {
                bufFP->Mode     = MFX_VPP_COPY_FIELD;
                bufFP->InField  = MFX_PICSTRUCT_FIELD_TFF;
                bufFP->OutField = MFX_PICSTRUCT_FIELD_TFF;
            }
            else if (TFF2BFF == mode)
            {
                bufFP->Mode     = MFX_VPP_COPY_FIELD;
                bufFP->InField  = MFX_PICSTRUCT_FIELD_TFF;
                bufFP->OutField = MFX_PICSTRUCT_FIELD_BFF;
            }
            else if (BFF2TFF == mode)
            {
                bufFP->Mode     = MFX_VPP_COPY_FIELD;
                bufFP->InField  = MFX_PICSTRUCT_FIELD_BFF;
                bufFP->OutField = MFX_PICSTRUCT_FIELD_TFF;
            }
            else if (BFF2BFF == mode)
            {
                bufFP->Mode     = MFX_VPP_COPY_FIELD;
                bufFP->InField  = MFX_PICSTRUCT_FIELD_BFF;
                bufFP->OutField = MFX_PICSTRUCT_FIELD_BFF;
            }
        }
        else if (MFX_EXTBUFF_VPP_ROTATION == bufferId)
        {
            mfxExtVPPRotation *bufRot = reinterpret_cast<mfxExtVPPRotation *>(par->ExtParam[i]);
            MFX_CHECK_NULL_PTR1(bufRot);
            bufRot->Angle = static_cast<mfxU16>(m_executeParams.rotation);
        }
        else if (MFX_EXTBUFF_VPP_SCALING == bufferId)
        {
            mfxExtVPPScaling *bufSc = reinterpret_cast<mfxExtVPPScaling *>(par->ExtParam[i]);
            MFX_CHECK_NULL_PTR1(bufSc);
            bufSc->ScalingMode = m_executeParams.scalingMode;
#if (MFX_VERSION >= 1033)
            bufSc->InterpolationMethod = m_executeParams.interpolationMethod;
#endif            
        }
#if (MFX_VERSION >= 1025)
        else if (MFX_EXTBUFF_VPP_COLOR_CONVERSION == bufferId)
        {
            mfxExtColorConversion *bufSc = reinterpret_cast<mfxExtColorConversion *>(par->ExtParam[i]);
            MFX_CHECK_NULL_PTR1(bufSc);
            bufSc->ChromaSiting = m_executeParams.chromaSiting;
        }
#endif
        else if (MFX_EXTBUFF_VPP_MIRRORING == bufferId)
        {
            mfxExtVPPMirroring *bufMir = reinterpret_cast<mfxExtVPPMirroring *>(par->ExtParam[i]);
            MFX_CHECK_NULL_PTR1(bufMir);
            bufMir->Type = static_cast<mfxU16>(m_executeParams.mirroring);
        }
        else if (MFX_EXTBUFF_VPP_FRAME_RATE_CONVERSION == bufferId)
        {
            mfxExtVPPFrameRateConversion *bufFRC = reinterpret_cast<mfxExtVPPFrameRateConversion *>(par->ExtParam[i]);
            MFX_CHECK_NULL_PTR1(bufFRC);
            bufFRC->Algorithm = m_executeParams.frcModeOrig;
        }
        else if (MFX_EXTBUFF_VPP_COLORFILL == bufferId)
        {
            mfxExtVPPColorFill *bufColorfill = reinterpret_cast<mfxExtVPPColorFill *>(par->ExtParam[i]);
            MFX_CHECK_NULL_PTR1(bufColorfill);
            bufColorfill->Enable = m_executeParams.iBackgroundColor?MFX_CODINGOPTION_ON:MFX_CODINGOPTION_OFF;
        }
    }

    return MFX_ERR_NONE;
} // mfxStatus VideoVPPHW::GetVideoParams(mfxVideoParam *par) const

mfxStatus VideoVPPHW::Query(VideoCORE *core, mfxVideoParam *par)
{
    MFX_CHECK_NULL_PTR2(par, core);

    mfxStatus sts = MFX_ERR_NONE;
    VPPHWResMng *vpp_ddi = 0;
    mfxVideoParam params = *par;
    Config config = {};
    MfxHwVideoProcessing::mfxExecuteParams executeParams;

    sts = CheckIOMode(par, ALL);
    MFX_CHECK_STS(sts);

    sts = core->CreateVideoProcessing(&params);
    MFX_CHECK_STS(sts);

    core->GetVideoProcessing((mfxHDL*)&vpp_ddi);
    if (0 == vpp_ddi)
    {
        return MFX_WRN_PARTIAL_ACCELERATION;
    }

    mfxVppCaps caps;
    caps = vpp_ddi->GetCaps();

    sts = ValidateParams(&params, &caps, core, true);
    MFX_CHECK_STS(sts);

    config.m_IOPattern = 0;
    sts = ConfigureExecuteParams(params, caps, executeParams, config);

    return sts;
}

mfxStatus  VideoVPPHW::Init(
    mfxVideoParam *par,
    bool /*isTemporal*/)
{
    mfxStatus sts = MFX_ERR_NONE;
    bool bIsFilterSkipped = false;

    m_frame_num = 0;
    m_critical_error = MFX_ERR_NONE;

    //-----------------------------------------------------
    // [1] high level check
    //-----------------------------------------------------
    MFX_CHECK_NULL_PTR1(par);

    sts = CheckIOMode(par, m_ioMode);
    MFX_CHECK_STS(sts);

    m_IOPattern = par->IOPattern;

    if(0 == par->AsyncDepth)
    {
        m_asyncDepth = MFX_AUTO_ASYNC_DEPTH_VALUE;
    }
    else if( par->AsyncDepth > MFX_MAX_ASYNC_DEPTH_VALUE )
    {
        m_asyncDepth = MFX_MAX_ASYNC_DEPTH_VALUE;
    }
    else
    {
        m_asyncDepth = par->AsyncDepth;
    }


    //-----------------------------------------------------
    // [2] device creation
    //-----------------------------------------------------
    m_params = *par;

    mfxExtBuffer* pHint = NULL;
    GetFilterParam(par, MFX_EXTBUFF_MVC_SEQ_DESC, &pHint);

    if( pHint )
    {
        /* Multi-view processing needs separate devices for each view. Using one device is not possible since
         * backward/forward references from different views will be messed up. Thus each VPPHW instance needs to
         * create its own device. Core is not able to handle this like it does for single view case.
         */
        try
        {
            m_ddi = new VPPHWResMng();
        }
        catch(std::bad_alloc&)
        {
            return MFX_WRN_PARTIAL_ACCELERATION;
        }

        sts = m_ddi->CreateDevice(m_pCore);
        MFX_CHECK_STS(sts);

        m_bMultiView = true;

    }
    else
    {
        sts = m_pCore->CreateVideoProcessing(&m_params);
        MFX_CHECK_STS(sts);

        m_pCore->GetVideoProcessing((mfxHDL*)&m_ddi);
        if (0 == m_ddi)
        {
            return MFX_WRN_PARTIAL_ACCELERATION;
        }
    }

    mfxVppCaps caps;
    caps = m_ddi->GetCaps();

    sts = ValidateParams(&m_params, &caps, m_pCore);
    if( MFX_ERR_UNSUPPORTED == sts )
    {
        sts = MFX_ERR_INVALID_VIDEO_PARAM;
    }
    if( MFX_WRN_FILTER_SKIPPED == sts )
    {
        bIsFilterSkipped = true;
        sts = MFX_ERR_NONE;
    }
    MFX_CHECK_STS(sts);

    m_config.m_IOPattern = 0;
    sts = ConfigureExecuteParams(
        m_params,
        caps,
        m_executeParams,
        m_config);

    if( MFX_WRN_FILTER_SKIPPED == sts )
    {
        bIsFilterSkipped = true;
        sts = MFX_ERR_NONE;
    }
    MFX_CHECK_STS(sts);

    if( m_executeParams.bFRCEnable &&
       (MFX_HW_D3D11 == m_pCore->GetVAType()) &&
       m_executeParams.customRateData.indexRateConversion != 0 )
    {
        sts = (*m_ddi)->ReconfigDevice(m_executeParams.customRateData.indexRateConversion);
        MFX_CHECK_STS(sts);
    }

    // allocate special structure for ::ExecuteBlt()
    {
        m_executeSurf.resize( m_config.m_surfCount[VPP_IN] );
    }

    // count of internal resources based on async_depth
    m_config.m_surfCount[VPP_OUT] = (mfxU16)(m_config.m_surfCount[VPP_OUT] * m_asyncDepth + 1);
    m_config.m_surfCount[VPP_IN]  = (mfxU16)(m_config.m_surfCount[VPP_IN]  * m_asyncDepth + 1);

    //-----------------------------------------------------
    // [3] internal frames allocation (make sense fo SYSTEM_MEMORY only)
    //-----------------------------------------------------
    mfxFrameAllocRequest request;

    if (D3D_TO_SYS == m_ioMode || SYS_TO_SYS == m_ioMode) // [OUT == SYSTEM_MEMORY]
    {
        //m_config.m_surfCount[VPP_OUT] = (mfxU16)(m_config.m_surfCount[VPP_OUT] + m_asyncDepth);

        request.Info        = par->vpp.Out;
        request.Type        = MFX_MEMTYPE_DXVA2_PROCESSOR_TARGET | MFX_MEMTYPE_FROM_VPPOUT | MFX_MEMTYPE_INTERNAL_FRAME;
        request.NumFrameMin = request.NumFrameSuggested = m_config.m_surfCount[VPP_OUT] ;

        sts = m_internalVidSurf[VPP_OUT].Alloc(m_pCore, request, par->vpp.Out.FourCC != MFX_FOURCC_YV12);
        MFX_CHECK(MFX_ERR_NONE == sts, MFX_WRN_PARTIAL_ACCELERATION);

        m_config.m_IOPattern |= MFX_IOPATTERN_OUT_SYSTEM_MEMORY;

        m_config.m_surfCount[VPP_OUT] = request.NumFrameMin;
    }

    if (SYS_TO_SYS == m_ioMode || SYS_TO_D3D == m_ioMode ) // [IN == SYSTEM_MEMORY]
    {
        //m_config.m_surfCount[VPP_IN] = (mfxU16)(m_config.m_surfCount[VPP_IN] + m_asyncDepth);

        request.Info        = par->vpp.In;
        request.Type        = MFX_MEMTYPE_DXVA2_PROCESSOR_TARGET | MFX_MEMTYPE_FROM_VPPIN | MFX_MEMTYPE_INTERNAL_FRAME;
        request.NumFrameMin = request.NumFrameSuggested = m_config.m_surfCount[VPP_IN] ;

        sts = m_internalVidSurf[VPP_IN].Alloc(m_pCore, request, par->vpp.In.FourCC != MFX_FOURCC_YV12);
        MFX_CHECK(MFX_ERR_NONE == sts, MFX_WRN_PARTIAL_ACCELERATION);

        m_config.m_IOPattern |= MFX_IOPATTERN_IN_SYSTEM_MEMORY;

        m_config.m_surfCount[VPP_IN] = request.NumFrameMin;
    }

    // sync workload mode by default
    m_workloadMode = VPP_ASYNC_WORKLOAD;

    //-----------------------------------------------------
    // [4] resource and task manager
    //-----------------------------------------------------
    sts = m_taskMngr.Init(m_pCore, m_config);
    MFX_CHECK_STS(sts);

    //-----------------------------------------------------
    // [5]  cm device
    //-----------------------------------------------------
    if(m_pCmDevice)
    {
        int res = 0;
        if (NULL == m_pCmProgram)
        {
            eMFXHWType m_platform = m_pCore->GetHWType();
            switch (m_platform)
            {
#ifdef MFX_ENABLE_KERNELS
            case MFX_HW_BDW:
            case MFX_HW_CHT:
                res = m_pCmDevice->LoadProgram((void*)genx_fcopy_gen8,sizeof(genx_fcopy_gen8),m_pCmProgram,"nojitter");
                break;
            case MFX_HW_SCL:
            case MFX_HW_APL:
            case MFX_HW_KBL:
            case MFX_HW_GLK:
            case MFX_HW_CFL:
                res = m_pCmDevice->LoadProgram((void*)genx_fcopy_gen9,sizeof(genx_fcopy_gen9),m_pCmProgram,"nojitter");
                break;
            case MFX_HW_ICL:
                res = m_pCmDevice->LoadProgram((void*)genx_fcopy_gen11,sizeof(genx_fcopy_gen11),m_pCmProgram,"nojitter");
                break;
            case MFX_HW_EHL:
            case MFX_HW_ICL_LP:
                res = m_pCmDevice->LoadProgram((void*)genx_fcopy_gen11lp,sizeof(genx_fcopy_gen11lp),m_pCmProgram,"nojitter");
                break;
            case MFX_HW_TGL_LP:
            case MFX_HW_DG1:
            case MFX_HW_RKL:
            case MFX_HW_ADL_S:
                res = m_pCmDevice->LoadProgram((void*)genx_fcopy_gen12lp,sizeof(genx_fcopy_gen12lp),m_pCmProgram,"nojitter");
                break;
#endif
            default:
                res = CM_FAILURE;
                break;
            }
            if(res != 0 ) return MFX_ERR_DEVICE_FAILED;
        }

        if (NULL == m_pCmKernel)
        {
            // MbCopyFieLd copies TOP or BOTTOM field from inSurf to OutSurf
            res = m_pCmDevice->CreateKernel(m_pCmProgram, CM_KERNEL_FUNCTION(MbCopyFieLd), m_pCmKernel);
            if(res != 0 ) return MFX_ERR_DEVICE_FAILED;
        }

        if (NULL == m_pCmQueue)
        {
            res = m_pCmDevice->CreateQueue(m_pCmQueue);
            if(res != 0 ) return MFX_ERR_DEVICE_FAILED;
        }
    }

#if defined(MFX_ENABLE_SCENE_CHANGE_DETECTION_VPP)
    if (MFX_DEINTERLACING_ADVANCED_SCD == m_executeParams.iDeinterlacingAlgorithm)
    {
        CmDevice* pCmDevice = QueryCoreInterface<CmDevice>(m_pCore, MFXICORECM_GUID);

        sts = m_SCD.Init(par->vpp.In.CropW, par->vpp.In.CropH, par->vpp.In.Width, par->vpp.In.PicStruct, pCmDevice);
        MFX_CHECK_STS(sts);

        m_SCD.SetGoPSize(ns_asc::Immediate_GoP);
    }
#endif

    /* Starting from TGL, there is support for HW mirroring, but only with d3d11.
       On platforms prior to TGL we use driver kernel for Linux with d3d_to_d3d
       and msdk kernel for other cases*/
    bool msdkMirrorIsUsed = (m_pCore->GetHWType() < MFX_HW_TGL_LP && !(m_pCore->GetVAType() == MFX_HW_VAAPI && m_ioMode == D3D_TO_D3D)) ||
                             m_pCore->GetVAType() == MFX_HW_D3D9;

    if (m_executeParams.mirroring && msdkMirrorIsUsed && !m_pCmCopy)
    {
        m_pCmCopy = QueryCoreInterface<CmCopyWrapper>(m_pCore, MFXICORECMCOPYWRAPPER_GUID);
        if ( m_pCmCopy )
        {
            sts = m_pCmCopy->Initialize(m_pCore->GetHWType());
            MFX_CHECK_STS(sts);
        }
        else
        {
            return MFX_ERR_DEVICE_FAILED;
        }
    }

#ifdef MFX_ENABLE_MCTF
    {
        if (m_executeParams.bEnableMctf)
        {
            m_pMctfCmDevice = m_pCmDevice;
            if (!m_pMctfCmDevice)
            {
                m_pMctfCmDevice = QueryCoreInterface<CmDevice>(m_pCore, MFXICORECM_GUID);
                MFX_CHECK(m_pMctfCmDevice, MFX_ERR_UNDEFINED_BEHAVIOR);
            }

            // create "Default" MCTF settings.
            IntMctfParams MctfConfig;
            CMC::QueryDefaultParams(&MctfConfig);

            // create default MCTF control
            mfxExtVppMctf MctfAPIDefault;
            CMC::QueryDefaultParams(&MctfAPIDefault);

            // control possibly passed
            mfxExtVppMctf* MctfAPIControl = NULL;
            GetFilterParam(par, MFX_EXTBUFF_VPP_MCTF, reinterpret_cast<mfxExtBuffer**>(&MctfAPIControl));

            if (m_pMCTFilter)
            {
                m_pMCTFilter->MCTF_CLOSE();
                m_pMCTFilter.reset();
            }

            MctfAPIControl = MctfAPIControl ? MctfAPIControl : &MctfAPIDefault;
            CMC::FillParamControl(&MctfConfig, MctfAPIControl);

            m_pMCTFilter = std::make_shared<CMC>();
            if (m_pMCTFilter)
                sts = InitMCTF(par->vpp.Out, MctfConfig);
            MFX_CHECK_STS(sts);
        }
    }
#endif
    return (bIsFilterSkipped) ? MFX_WRN_FILTER_SKIPPED : MFX_ERR_NONE;

} // mfxStatus VideoVPPHW::Init(mfxVideoParam *par, mfxU32 *tabUsedFiltersID, mfxU32 numOfFilters)

#ifdef MFX_ENABLE_MCTF
mfxStatus VideoVPPHW::InitMCTF(const mfxFrameInfo& info, const IntMctfParams& MctfConfig)
{
    mfxStatus sts = MFX_ERR_NONE;
    if (m_pMCTFilter)
    {
        sts = m_pMCTFilter->MCTF_INIT(m_pCore, m_pMctfCmDevice, info, &MctfConfig);
        MFX_CHECK_STS(sts);
        mfxU32 MctfQueueDepth = m_pMCTFilter->MCTF_GetQueueDepth();
        // now allocation of buffers & passing it into MCTF

        mfxFrameAllocRequest request;
        memset(&request, 0, sizeof(request));
        request.Info = info;
        request.Info.FourCC = MFX_FOURCC_NV12;

        request.Type = MFX_MEMTYPE_FROM_VPPOUT | MFX_MEMTYPE_DXVA2_PROCESSOR_TARGET;
        request.Type |= MFX_MEMTYPE_INTERNAL_FRAME;
/*
// MCTF buffers are not shared; so its not needed to set SHARED_RESOURCE status
        if (m_ioMode == IOMode::D3D_TO_SYS || m_ioMode == IOMode::SYS_TO_SYS)
        {
#ifdef MFX_VA_WIN
            request.Type |= MFX_MEMTYPE_SHARED_RESOURCE;
#endif
        }
*/
        if (m_IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY)
            request.Type |= MFX_MEMTYPE_OPAQUE_FRAME;
        request.NumFrameMin = request.NumFrameSuggested = (mfxU16)MctfQueueDepth;
        if (0 == request.NumFrameMin)
            return MFX_ERR_INVALID_VIDEO_PARAM;

        // create a pool of surfaces & map pointers to a pool of pointers
        m_pMCTFSurfacePool.resize(MctfQueueDepth);
        m_MCTFSurfacePool.resize(MctfQueueDepth);
        for (size_t idx = 0; idx < MctfQueueDepth; ++idx)
        {
            m_pMCTFSurfacePool[idx] = &(m_MCTFSurfacePool[idx]);
            memset(m_pMCTFSurfacePool[idx], 0, sizeof(mfxFrameSurface1));
        }

        if (m_bMctfAllocatedMemory)
        {
            MFX_SAFE_CALL(m_pCore->FreeFrames(&m_MctfMfxAlocResponse));
            m_bMctfAllocatedMemory = false;
        }

        memset(&m_MctfMfxAlocResponse, 0, sizeof(m_MctfMfxAlocResponse));
        // alloc & binds memory for mids
        m_MctfMids.resize(MctfQueueDepth);
        m_MctfMfxAlocResponse.mids = &(m_MctfMids[0]);

        if (request.Type & MFX_MEMTYPE_OPAQUE_FRAME)
            sts = m_pCore->AllocFrames(&request, &m_MctfMfxAlocResponse, &(m_pMCTFSurfacePool[0]), MctfQueueDepth);
        else
        {
            if (D3D_TO_SYS == m_ioMode || SYS_TO_SYS == m_ioMode) // [OUT == SYSTEM_MEMORY]
                sts = m_pCore->AllocFrames(&request, &m_MctfMfxAlocResponse, false);
            else
                sts = m_pCore->AllocFrames(&request, &m_MctfMfxAlocResponse, false);
        }

        if (m_MctfMfxAlocResponse.NumFrameActual != request.NumFrameMin)
            sts = MFX_ERR_MEMORY_ALLOC;

        MFX_CHECK(MFX_ERR_NONE == sts, MFX_ERR_MEMORY_ALLOC);
        m_bMctfAllocatedMemory = true;

        // clear & bind video-surfaces to mfxFrameSurface1 structures
        mfxMemId* pmids = m_MctfMfxAlocResponse.mids;
        for (auto it = m_pMCTFSurfacePool.begin(); it != m_pMCTFSurfacePool.end(); ++it)
        {
            (*it)->Info = request.Info;
            (*it)->Info.FourCC = MFX_FOURCC_NV12;
            (*it)->Data.MemType = request.Type;
            (*it)->Data.MemId = *pmids++;
            // set to extreamly high value to identify possible incorrectness
            // with settings
            (*it)->Data.FrameOrder = 0xFFFFFFFF;
            (*it)->Data.TimeStamp = 0xFFFFFFFF;
        }
        // Set memory, including internal buffers
        // copy surfaces from m_pMCTFSurfacePool to QfIn in MCTF:
        m_pMCTFilter->MCTF_SetMemory(m_pMCTFSurfacePool);
        m_MctfIsFlushing = false;
        m_taskMngr.SetMctf(m_pMCTFilter);

        m_MCTFtableCmRelations2.clear();
        m_MCTFtableCmIndex2.clear();
        return MFX_ERR_NONE;
    }
    else
        return MFX_ERR_UNDEFINED_BEHAVIOR;
}

mfxStatus VideoVPPHW::GetFrameHandle(mfxFrameSurface1* InFrame, mfxHDLPair& handle, bool bInternalAlloc)
{
    mfxFrameSurface1 *pSurfI = m_pCore->GetNativeSurface(InFrame);
    pSurfI = pSurfI ? pSurfI : InFrame;
    return GetFrameHandle(pSurfI->Data.MemId, handle, bInternalAlloc);
}

mfxStatus VideoVPPHW::GetFrameHandle(mfxMemId MemId, mfxHDLPair& handle, bool bInternalAlloc)
{
    handle.first = handle.second = nullptr;
    if ((IOMode::D3D_TO_D3D == m_ioMode || IOMode::SYS_TO_D3D == m_ioMode) && !bInternalAlloc)
    {
        if (m_IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY)
        {
            MFX_SAFE_CALL(m_pCore->GetFrameHDL(MemId, reinterpret_cast<mfxHDL*>(&handle)));
        }
        else
        {
            //MFX_SAFE_CALL(m_pCore->GetFrameHDL(MemId, reinterpret_cast<mfxHDL*>(&handle)));
            MFX_SAFE_CALL(m_pCore->GetExternalFrameHDL(MemId, reinterpret_cast<mfxHDL*>(&handle), false));
        }
    }
    else
    {
        // in case of system memory, for the puproses of minimization of copy ops,
        // internal d3d surface is passed to; thus we can extract handle directly from MemId
        // see VideoVPPHW::PostWorkOutSurfaceCopy for further details & examples
        MFX_SAFE_CALL(m_pCore->GetFrameHDL(MemId, reinterpret_cast<mfxHDL*>(&handle)));
        //MFX_SAFE_CALL(m_pCore->GetExternalFrameHDL(MemId, reinterpret_cast<mfxHDL*>(&handle), false));
    }

    return MFX_ERR_NONE;
} // GetFrameHandle(...

mfxStatus VideoVPPHW::ClearCmSurfaces2D()
{
    //destroy CmSurfaces2D
    if (m_pMctfCmDevice) {
        for (decltype(m_MCTFtableCmRelations2)::iterator item = m_MCTFtableCmRelations2.begin(); item != m_MCTFtableCmRelations2.end(); ++item)
            m_pMctfCmDevice->DestroySurface(item->second);
    }

    m_MCTFtableCmRelations2.clear();
    m_MCTFtableCmIndex2.clear();
    return MFX_ERR_NONE;
} // ClearCmSurfaces2D(...

mfxStatus VideoVPPHW::CreateCmSurface2D(void *pSrcHDL, CmSurface2D* & pCmSurface2D, SurfaceIndex* &pCmSrcIndex)
{
    INT cmSts = 0;
    std::map<void *, CmSurface2D *>::iterator it;
    std::map<CmSurface2D *, SurfaceIndex *>::iterator it_idx;

    if (!m_pMctfCmDevice)
        return MFX_ERR_NOT_INITIALIZED;

    it = m_MCTFtableCmRelations2.find(pSrcHDL);
    if (m_MCTFtableCmRelations2.end() == it)
    {
        //UMC::AutomaticUMCMutex guard(m_guard);
        {
            cmSts = m_pMctfCmDevice->CreateSurface2D((AbstractSurfaceHandle *)pSrcHDL, pCmSurface2D);
            MFX_CHECK((CM_SUCCESS == cmSts), MFX_ERR_DEVICE_FAILED);
            m_MCTFtableCmRelations2.insert(std::pair<void *, CmSurface2D *>(pSrcHDL, pCmSurface2D));
        }

        cmSts = pCmSurface2D->GetIndex(pCmSrcIndex);
        MFX_CHECK((CM_SUCCESS == cmSts), MFX_ERR_DEVICE_FAILED);
        m_MCTFtableCmIndex2.insert(std::pair<CmSurface2D *, SurfaceIndex *>(pCmSurface2D, pCmSrcIndex));
    }
    else
    {
        pCmSurface2D = it->second;
        it_idx = m_MCTFtableCmIndex2.find(pCmSurface2D);
        if (it_idx == m_MCTFtableCmIndex2.end())
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        else
            pCmSrcIndex = it_idx->second;
    }
    return MFX_ERR_NONE;
} // CreateCmSurface2D(...


#endif


mfxStatus VideoVPPHW::QueryCaps(VideoCORE* core, MfxHwVideoProcessing::mfxVppCaps& caps)
{

    MFX_CHECK_NULL_PTR1(core);

    VPPHWResMng* ddi = 0;
    mfxStatus sts = MFX_ERR_NONE;
    mfxVideoParam tmpPar = {};

    sts = core->CreateVideoProcessing(&tmpPar);
    MFX_CHECK_STS( sts );

    core->GetVideoProcessing((mfxHDL*)&ddi);
    if (0 == ddi)
    {
        return MFX_WRN_PARTIAL_ACCELERATION;
    }

    caps = ddi->GetCaps();

#ifdef MFX_ENABLE_MCTF
    eMFXHWType  hwType = core->GetHWType();
    caps.uMCTF = 0;
    if (hwType >= MFX_HW_BDW)
        caps.uMCTF = 1;
#endif

    return sts;

} // mfxStatus VideoVPPHW::QueryCaps(VideoCORE* core, MfxHwVideoProcessing::mfxVppCaps& caps)


mfxStatus VideoVPPHW::QueryIOSurf(
    IOMode ioMode,
    VideoCORE* core,
    mfxVideoParam* par,
    mfxFrameAllocRequest* request)
{
    mfxStatus sts = MFX_ERR_NONE;
    VPPHWResMng *vpp_ddi;
    MFX_CHECK_NULL_PTR1(par);

    sts = CheckIOMode(par, ioMode);
    MFX_CHECK_STS(sts);

    mfxVideoParam tmpPar = {};
    sts = core->CreateVideoProcessing(&tmpPar);
    MFX_CHECK_STS(sts);

    core->GetVideoProcessing((mfxHDL*)&vpp_ddi);
    if (0 == vpp_ddi)
    {
        return MFX_WRN_PARTIAL_ACCELERATION;
    }

    mfxVppCaps caps;
    caps = vpp_ddi->GetCaps();

    sts = ValidateParams(par, &caps, core);
    if( MFX_WRN_FILTER_SKIPPED == sts || MFX_WRN_INCOMPATIBLE_VIDEO_PARAM == sts)
    {
        sts = MFX_ERR_NONE;
    }
    MFX_CHECK_STS(sts);

    mfxExecuteParams executeParams;
    MemSetZero4mfxExecuteParams(&executeParams);
    Config  config = {};

    sts = ConfigureExecuteParams(
        *par,
        caps,
        executeParams,
        config);
    if (MFX_WRN_FILTER_SKIPPED == sts )
    {
        sts = MFX_ERR_NONE;
    }
    MFX_CHECK_STS(sts);

    request[VPP_IN].NumFrameMin  = request[VPP_IN].NumFrameSuggested  = config.m_surfCount[VPP_IN];
    request[VPP_OUT].NumFrameMin = request[VPP_OUT].NumFrameSuggested = config.m_surfCount[VPP_OUT];

    return sts;

} // mfxStatus VideoVPPHW::QueryIOSurf(mfxVideoParam *par, mfxU32 *tabUsedFiltersID, mfxU32 numOfFilters)


mfxStatus VideoVPPHW::Reset(mfxVideoParam *par)
{
    /* This is question: Should we free resource here or not?... */
    /* For Dynamic composition we should not to do it !!! */
    if (false == m_executeParams.bComposite)
    {
        m_taskMngr.Close();// all resource free here
    }

    /* Application should close the SDK component and then reinitialize it in case of the parameters change described below */
    if (m_params.vpp.In.PicStruct  != par->vpp.In.PicStruct ||
        m_params.vpp.Out.PicStruct != par->vpp.Out.PicStruct)
        return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;

    const mfxF64 inFrameRateCur  = (mfxF64) par->vpp.In.FrameRateExtN      / (mfxF64) par->vpp.In.FrameRateExtD,
                 outFrameRateCur = (mfxF64) par->vpp.Out.FrameRateExtN     / (mfxF64) par->vpp.Out.FrameRateExtD,
                 inFrameRate     = (mfxF64) m_params.vpp.In.FrameRateExtN  / (mfxF64) m_params.vpp.In.FrameRateExtD,
                 outFrameRate    = (mfxF64) m_params.vpp.Out.FrameRateExtN / (mfxF64) m_params.vpp.Out.FrameRateExtD;

    if ( fabs(inFrameRateCur / outFrameRateCur - inFrameRate / outFrameRate) > std::numeric_limits<mfxF64>::epsilon() )
        return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM; // Frame Rate ratio check

#if (MFX_VERSION >= 1027)
    eMFXPlatform platform = m_pCore->GetPlatformType();
    if (platform == MFX_PLATFORM_HARDWARE)
    {
        eMFXHWType type = m_pCore->GetHWType();
        if (type < MFX_HW_ICL &&
           (par->vpp.In.FourCC   == MFX_FOURCC_Y210 ||
            par->vpp.In.FourCC   == MFX_FOURCC_Y410 ||
            par->vpp.Out.FourCC  == MFX_FOURCC_Y210 ||
            par->vpp.Out.FourCC  == MFX_FOURCC_Y410))
            return MFX_ERR_INVALID_VIDEO_PARAM;

#if (MFX_VERSION >= 1031)
        if (type < MFX_HW_TGL_LP &&
           (par->vpp.In.FourCC   == MFX_FOURCC_P016 ||
            par->vpp.In.FourCC   == MFX_FOURCC_Y216 ||
            par->vpp.In.FourCC   == MFX_FOURCC_Y416 ||
            par->vpp.Out.FourCC  == MFX_FOURCC_P016 ||
            par->vpp.Out.FourCC  == MFX_FOURCC_Y216 ||
            par->vpp.Out.FourCC  == MFX_FOURCC_Y416))
            return MFX_ERR_INVALID_VIDEO_PARAM;
#endif
    }
#endif

    if (m_params.vpp.In.FourCC  != par->vpp.In.FourCC ||
        m_params.vpp.Out.FourCC != par->vpp.Out.FourCC)
        return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;

    mfxStatus sts = MFX_ERR_NONE;
    bool bIsFilterSkipped = false;

    m_frame_num = 0;
    //-----------------------------------------------------
    // [1] high level check
    //-----------------------------------------------------
    MFX_CHECK_NULL_PTR1(par);

    sts = CheckIOMode(par, m_ioMode);
    MFX_CHECK_STS(sts);

    m_IOPattern = par->IOPattern;

    if(0 == par->AsyncDepth)
    {
        m_asyncDepth = MFX_AUTO_ASYNC_DEPTH_VALUE;
    }
    else if( par->AsyncDepth > MFX_AUTO_ASYNC_DEPTH_VALUE )
    {
        m_asyncDepth = MFX_AUTO_ASYNC_DEPTH_VALUE;
    }
    else
    {
        m_asyncDepth = par->AsyncDepth;
    }

    //-----------------------------------------------------
    // [2] device check
    //-----------------------------------------------------
    m_params = *par;

    mfxVppCaps caps;
    caps = m_ddi->GetCaps();

    sts = ValidateParams(&m_params, &caps, m_pCore);
    if( MFX_WRN_FILTER_SKIPPED == sts )
    {
        bIsFilterSkipped = true;
        sts = MFX_ERR_NONE;
    }
    MFX_CHECK_STS(sts);

    m_config.m_IOPattern = 0;
    sts = ConfigureExecuteParams(
        m_params,
        caps,
        m_executeParams,
        m_config);

    if( MFX_WRN_FILTER_SKIPPED == sts )
    {
        bIsFilterSkipped = true;
        sts = MFX_ERR_NONE;
    }
    MFX_CHECK_STS(sts);

    if( m_executeParams.bFRCEnable &&
       (MFX_HW_D3D11 == m_pCore->GetVAType()) &&
       m_executeParams.customRateData.indexRateConversion != 0 )
    {
        sts = (*m_ddi)->ReconfigDevice(m_executeParams.customRateData.indexRateConversion);
        MFX_CHECK_STS(sts);
    }

    // allocate special structure for ::ExecuteBlt()
    {
        m_executeSurf.resize( m_config.m_surfCount[VPP_IN] );
    }

    // count of internal resources based on async_depth
    m_config.m_surfCount[VPP_OUT] = (mfxU16)(m_config.m_surfCount[VPP_OUT] * m_asyncDepth + 1);
    m_config.m_surfCount[VPP_IN]  = (mfxU16)(m_config.m_surfCount[VPP_IN]  * m_asyncDepth + 1);

    //-----------------------------------------------------
    // [3] internal frames allocation (make sense fo SYSTEM_MEMORY only)
    //-----------------------------------------------------
    mfxFrameAllocRequest request;

    if (D3D_TO_SYS == m_ioMode || SYS_TO_SYS == m_ioMode) // [OUT == SYSTEM_MEMORY]
    {
        //m_config.m_surfCount[VPP_OUT] = (mfxU16)(m_config.m_surfCount[VPP_OUT] + m_asyncDepth);

        request.Info        = par->vpp.Out;
        request.Type        = MFX_MEMTYPE_DXVA2_PROCESSOR_TARGET | MFX_MEMTYPE_FROM_VPPOUT | MFX_MEMTYPE_INTERNAL_FRAME;
        request.NumFrameMin = request.NumFrameSuggested = m_config.m_surfCount[VPP_OUT] ;

        if (m_config.m_surfCount[VPP_OUT] != m_internalVidSurf[VPP_OUT].NumFrameActual || m_executeParams.bComposite)
        {
            sts = m_internalVidSurf[VPP_OUT].Alloc(m_pCore, request, par->vpp.Out.FourCC != MFX_FOURCC_YV12);
            MFX_CHECK(MFX_ERR_NONE == sts, MFX_WRN_PARTIAL_ACCELERATION);
        }

        m_config.m_IOPattern |= MFX_IOPATTERN_OUT_SYSTEM_MEMORY;

        m_config.m_surfCount[VPP_OUT] = request.NumFrameMin;
    }

    if (SYS_TO_SYS == m_ioMode || SYS_TO_D3D == m_ioMode ) // [IN == SYSTEM_MEMORY]
    {
        //m_config.m_surfCount[VPP_IN] = (mfxU16)(m_config.m_surfCount[VPP_IN] + m_asyncDepth);

        request.Info        = par->vpp.In;
        request.Type        = MFX_MEMTYPE_DXVA2_PROCESSOR_TARGET | MFX_MEMTYPE_FROM_VPPIN | MFX_MEMTYPE_INTERNAL_FRAME;
        request.NumFrameMin = request.NumFrameSuggested = m_config.m_surfCount[VPP_IN] ;

        if (m_config.m_surfCount[VPP_IN] != m_internalVidSurf[VPP_IN].NumFrameActual || m_executeParams.bComposite)
        {
            sts = m_internalVidSurf[VPP_IN].Alloc(m_pCore, request, par->vpp.In.FourCC != MFX_FOURCC_YV12);
            MFX_CHECK(MFX_ERR_NONE == sts, MFX_WRN_PARTIAL_ACCELERATION);
        }

        m_config.m_IOPattern |= MFX_IOPATTERN_IN_SYSTEM_MEMORY;

        m_config.m_surfCount[VPP_IN] = request.NumFrameMin;
    }

    // sync workload mode by default
    m_workloadMode = VPP_ASYNC_WORKLOAD;

    //-----------------------------------------------------
    // [5] resource and task manager
    //-----------------------------------------------------
    sts = m_taskMngr.Init(m_pCore, m_config);
    MFX_CHECK_STS(sts);

#ifdef MFX_ENABLE_MCTF
    {
        if (m_executeParams.bEnableMctf)
        {
            // create "Default" MCTF settings.
            IntMctfParams MctfConfig;
            CMC::QueryDefaultParams(&MctfConfig);

            // create default MCTF control
            mfxExtVppMctf MctfAPIDefault;
            CMC::QueryDefaultParams(&MctfAPIDefault);

            // control possibly passed
            mfxExtVppMctf* MctfAPIControl = NULL;
            GetFilterParam(par, MFX_EXTBUFF_VPP_MCTF, reinterpret_cast<mfxExtBuffer**>(&MctfAPIControl));

            if (m_pMCTFilter)
            {
                m_pMCTFilter->MCTF_CLOSE();
                m_pMCTFilter.reset();

                m_MCTFSurfacePool.clear();
                m_pMCTFSurfacePool.clear();
            }

            MctfAPIControl = MctfAPIControl ? MctfAPIControl : &MctfAPIDefault;
            CMC::FillParamControl(&MctfConfig, MctfAPIControl);

            m_pMCTFilter = std::make_shared<CMC>();

            if (m_pMCTFilter)
                sts = InitMCTF(par->vpp.Out, MctfConfig);
            MFX_CHECK_STS(sts);
        }
    }
#endif

    return (bIsFilterSkipped) ? MFX_WRN_FILTER_SKIPPED : MFX_ERR_NONE;
} // mfxStatus VideoVPPHW::Reset(mfxVideoParam *par)


mfxStatus VideoVPPHW::Close()
{
    mfxStatus sts = MFX_ERR_NONE;

    m_internalVidSurf[VPP_IN].Free();
    m_internalVidSurf[VPP_OUT].Free();

    m_executeSurf.clear();

    m_config.m_extConfig.mode  = FRC_DISABLED;
    m_config.m_bMode30i60pEnable  = false;
    m_config.m_bRefFrameEnable = false;
    m_config.m_bCopyPassThroughEnable = false;
    m_config.m_surfCount[VPP_IN]  = 1;
    m_config.m_surfCount[VPP_OUT] = 1;

    m_IOPattern = 0;

    //m_acceptedDeviceAsyncDepth = ACCEPTED_DEVICE_ASYNC_DEPTH;

    m_taskMngr.Close();

    // sync workload mode by default
    m_workloadMode = VPP_SYNC_WORKLOAD;


#if defined (MFX_ENABLE_SCENE_CHANGE_DETECTION_VPP)
    m_SCD.Close();
#endif

    // CM device
    if(m_pCmDevice) {
        m_pCmDevice->DestroyKernel(m_pCmKernel);
        m_pCmKernel = NULL;

        m_pCmDevice->DestroyProgram(m_pCmProgram);
        m_pCmProgram = NULL;

        m_pCmQueue = NULL;
        //::DestroyCmDevice(device);
    }

#ifdef MFX_ENABLE_MCTF
    if (m_bMctfAllocatedMemory)
    {
        MFX_SAFE_CALL(m_pCore->FreeFrames(&m_MctfMfxAlocResponse));
        m_bMctfAllocatedMemory = false;
    };

    if (m_pMCTFilter)
    {
        m_pMCTFilter->MCTF_CLOSE();
        m_pMCTFilter.reset();
        ClearCmSurfaces2D();
    }
    /*
    if (m_pMctfCmDevice)
    {
        ::DestroyCmDevice(m_pMctfCmDevice);
        m_pMctfCmDevice = nullptr;
    }
    */
#endif

    /* Device close semantic is different for multi-view and single view */
    if ( m_bMultiView )
    {
        /* In case of multi-view, VPPHW created dedicated resource manager with
         * device on Init, so need to close it be deleting resource manager
         */
        delete m_ddi;
        m_bMultiView = false;
    }
    else
    {
       /* In case of single-view, VPPHW needs just close device, but does not
        * need to destroy resource manager. It's still alive inside of the core.
        * Core will take care on its removal.
        */
        m_pCore->GetVideoProcessing((mfxHDL *)&m_ddi);
        if (m_ddi->GetDevice())
        {
            m_ddi->Close();
        }
    }

    return sts;

} // mfxStatus VideoVPPHW::Close()


bool VideoVPPHW::UseCopyPassThrough(const DdiTask *pTask) const
{
    if (!m_config.m_bCopyPassThroughEnable
        || IsRoiDifferent(pTask->input.pSurf, pTask->output.pSurf))
    {
        return false;
    }

    if (m_pCore->GetVAType() != MFX_HW_VAAPI || m_ioMode != D3D_TO_D3D)
    {
        return true;
    }

    // Under VAAPI for D3D_TO_D3D either CM copy or VPP pipeline are preferred by
    // performance reasons. So, before enabling CopyPassThrough for a task we check
    // for CM availability to do fallback to VPP if there is no CM.
    // This can't be done in ConfigureExecuteParams() because CmCopy status is set later.
    const VAAPIVideoCORE *vaapiCore = dynamic_cast<VAAPIVideoCORE*>(m_pCore);
    MFX_CHECK_WITH_ASSERT(vaapiCore, false);
    return vaapiCore->CmCopy();
}

mfxStatus VideoVPPHW::VppFrameCheck(
                                    mfxFrameSurface1 *input,
                                    mfxFrameSurface1 *output,
                                    mfxExtVppAuxData *aux,
                                    MFX_ENTRY_POINT pEntryPoint[],
                                    mfxU32 &numEntryPoints)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    mfxStatus sts;
    mfxStatus intSts = MFX_ERR_NONE;

    DdiTask *pTask = NULL;

#ifdef MFX_VA_LINUX
    if(input && ((input->Info.CropW == 1) || (input->Info.CropH == 1))){
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    if(output && ((output->Info.CropW == 1) || (output->Info.CropH == 1))){
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }
#endif

    if (input &&
        m_critical_error != MFX_ERR_NONE)
        return m_critical_error;

#ifdef MFX_ENABLE_MCTF
    sts = MFX_ERR_NONE;
    mfxFrameSurface1* pWorkOutSurf = output;
    if (m_pMCTFilter)
    {
        // if input != NULL, if MCTF works with > 1 reference, & there are
        // not enough surfaces accumulated, let use temp surface as an output:
        if (input && !m_MctfIsFlushing)
        {
            MFX_SAFE_CALL(m_pMCTFilter->MCTF_GetEmptySurface(&pWorkOutSurf));
            if (!pWorkOutSurf) 
            {
                guard.Unlock();
                mfxU32 iters(0);
                const mfxU32 MAX_ITER = 1000;
                while (!pWorkOutSurf && ++iters < MAX_ITER)
                {
                    vm_time_sleep(1);
                    MFX_SAFE_CALL(m_pMCTFilter->MCTF_GetEmptySurface(&pWorkOutSurf));
                }
                if (MAX_ITER == iters)
                    return MFX_ERR_NOT_ENOUGH_BUFFER;
                guard.Lock();
            }

            // store a pointer to surface inside MCTF
            m_Surfaces2Unlock.push_back(pWorkOutSurf);

            // "disable" actual output surface for the purposes of filling of ddiTask below
            // if MCTF works with a delay and its just started (1 or 2 frames depending on a tmeporal MCTF mode
            if ((TWO_REFERENCES == m_pMCTFilter->MCTF_GetReferenceNumber() && m_taskMngr.GetMCTFSurfacesInQueue() < 1) ||
                (FOUR_REFERENCES == m_pMCTFilter->MCTF_GetReferenceNumber() && m_taskMngr.GetMCTFSurfacesInQueue() < 2))
                output = nullptr;
        };

        if (m_MctfIsFlushing)
        {
            // it means that MCTF already finished its work, but application
            // asks to continue w/o apropriate close.
            if (input)
                return MFX_ERR_UNDEFINED_BEHAVIOR;
            // this is correct path; just return MFX_ERR_MORE_DATA notifying
            // that MCTF is done and processing is finished.
            else
                if (1 == m_taskMngr.GetMCTFSurfacesInQueue())
                return MFX_ERR_MORE_DATA;
        }
        else
        {
            // this is the case when its not flushing but input is null; the situation when MCTF is not ready,
            // but an application already asks it to finalize must be correctly processed.
            if (!input)
            {
                if (!m_taskMngr.GetMCTFSurfacesInQueue())
                    return MFX_ERR_MORE_DATA;
                else if ((TWO_REFERENCES == m_pMCTFilter->MCTF_GetReferenceNumber() && m_taskMngr.GetMCTFSurfacesInQueue() <= 1) ||
                        (FOUR_REFERENCES == m_pMCTFilter->MCTF_GetReferenceNumber() && m_taskMngr.GetMCTFSurfacesInQueue() <= 2))
                    return MFX_ERR_UNDEFINED_BEHAVIOR;
            }
        }

        // AssignTask will also clear bForcedInternalAlloc
        sts = m_taskMngr.AssignTask(input,pWorkOutSurf, output, aux, pTask, intSts);

        if (MFX_WRN_DEVICE_BUSY != sts) 
        {
            if (!pTask)
                return MFX_ERR_UNDEFINED_BEHAVIOR;
            else
                // condittion below means MCTF has substited temp surface instead of output
                if (pWorkOutSurf != output)
                    pTask->output.bForcedInternalAlloc = true;

            if (MFX_ERR_MORE_DATA == sts && !input)
            {
                    m_MctfIsFlushing = true;
                // this is a completion phase. so let ask MCTF to return
                // all surfaces that are in queue (if any) but following the
                // order supported by the scheduler
                // in sync/async submission, finalization is async:
                //switch (auxMctf->TemporalMode)
                switch (m_pMCTFilter->MCTF_GetReferenceNumber())
                {
                case NO_REFERENCES:
                case ONE_REFERENCE:
                    ; //nothing as 0-reference or 1-reference does not impose any delay
                    break;
                case TWO_REFERENCES:
                case FOUR_REFERENCES:
                    if (m_taskMngr.GetMCTFSurfacesInQueue() > 1)
                    {
                        m_taskMngr.DecMCTFSurfacesInQueue();
                        sts = MFX_ERR_NONE;
                    }
                    break;
                default:
                    sts = MFX_ERR_UNDEFINED_BEHAVIOR;
                }
            }
            else
            {
                if (m_MctfIsFlushing && sts != MFX_WRN_DEVICE_BUSY)
                    return MFX_ERR_UNDEFINED_BEHAVIOR;

                if (MFX_ERR_MORE_DATA == sts)
                {
                    // let unlock just locked MCTF surface
                    if (!m_Surfaces2Unlock.empty())
                    {
                        // this is to release a surface inside MCTF pool; correposnds to MCTF_GetEmptySurface in VPPFrameCheck
                        sts = m_pCore->DecreaseReference(&(m_Surfaces2Unlock.front()->Data));
                        MFX_CHECK_STS(sts);
                        m_Surfaces2Unlock.pop_front();
                    }
                }
            }
        }
    }
    else
        sts = m_taskMngr.AssignTask(input, output, output, aux, pTask, intSts);
#else
    sts = m_taskMngr.AssignTask(input, output, aux, pTask, intSts);
#endif
    MFX_CHECK_STS(sts);

    bool useCopyPassThrough = UseCopyPassThrough(pTask);

    if (VPP_SYNC_WORKLOAD == m_workloadMode)
    {
        pTask->bRunTimeCopyPassThrough = useCopyPassThrough;

        // submit task
        SyncTaskSubmission(pTask);

        if (false == pTask->bRunTimeCopyPassThrough)
        {
            // configure entry point
            pEntryPoint[0].pRoutine = VideoVPPHW::QueryTaskRoutine;
            pEntryPoint[0].pParam = (void *) pTask;
            pEntryPoint[0].requiredNumThreads = 1;
            pEntryPoint[0].pState = (void *) this;
            pEntryPoint[0].pRoutineName = (char *)"VPP Query";

            numEntryPoints = 1;
        }
    }
    else
    {
        if (!useCopyPassThrough)
        {
            pTask->bRunTimeCopyPassThrough = false;

            // configure entry point
            pEntryPoint[1].pRoutine = VideoVPPHW::QueryTaskRoutine;
            pEntryPoint[1].pParam = (void *) pTask;
            pEntryPoint[1].requiredNumThreads = 1;
            pEntryPoint[1].pState = (void *) this;
            pEntryPoint[1].pRoutineName = (char *)"VPP Query";

            // configure entry point
            pEntryPoint[0].pRoutine = VideoVPPHW::AsyncTaskSubmission;
            pEntryPoint[0].pParam = (void *) pTask;
            pEntryPoint[0].requiredNumThreads = 1;
            pEntryPoint[0].pState = (void *) this;
            pEntryPoint[0].pRoutineName = (char *)"VPP Submit";

            numEntryPoints = 2;
        }
        else
        {
            pTask->bRunTimeCopyPassThrough = true;

            // configure entry point
            pEntryPoint[0].pRoutine = VideoVPPHW::AsyncTaskSubmission;
            pEntryPoint[0].pParam = (void *) pTask;
            pEntryPoint[0].requiredNumThreads = 1;
            pEntryPoint[0].pState = (void *) this;
            pEntryPoint[0].pRoutineName = (char *)"VPP Submit";

            numEntryPoints = 1;
        }
    }

    return intSts;

} // mfxStatus VideoVPPHW::VppFrameCheck(...)


mfxStatus VideoVPPHW::PreWorkOutSurface(ExtSurface & output)
{
    mfxHDLPair out;
    mfxHDLPair hdl;

    if (!output.pSurf)
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    if (D3D_TO_D3D == m_ioMode || SYS_TO_D3D == m_ioMode)
    {
        if ((m_IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY) || output.bForcedInternalAlloc)
        {
            MFX_SAFE_CALL(m_pCore->GetFrameHDL( output.pSurf->Data.MemId, (mfxHDL *)&hdl) );
            m_executeParams.targetSurface.memId = output.pSurf->Data.MemId;

            m_executeParams.targetSurface.bExternal = false;
        }
        else
        {
            MFX_SAFE_CALL(m_pCore->GetExternalFrameHDL(output.pSurf->Data.MemId, (mfxHDL *)&hdl, false));
            m_executeParams.targetSurface.memId = output.pSurf->Data.MemId;

            m_executeParams.targetSurface.bExternal = true;
        }
    }
    else
    {
        mfxU32 resId = output.resIdx;
        mfxMemId MemId = output.bForcedInternalAlloc ? output.pSurf->Data.MemId : m_internalVidSurf[VPP_OUT].mids[resId];

        MFX_SAFE_CALL(m_pCore->GetFrameHDL(MemId, (mfxHDL *)&hdl));
        m_executeParams.targetSurface.memId = MemId;

        m_executeParams.targetSurface.bExternal = false;
    }

    out = hdl;

    // register output surface (make sense for DX9 only)
    mfxStatus sts = (*m_ddi)->Register(&out, 1, TRUE);
    MFX_CHECK_STS(sts);

    m_executeParams.targetSurface.hdl       = static_cast<mfxHDLPair>(out);
    m_executeParams.targetSurface.frameInfo = output.pSurf->Info;
    m_executeParams.targetTimeStamp         = output.timeStamp;

    return MFX_ERR_NONE;

} // mfxStatus VideoVPPHW::PreWorkOutSurface(DdiTask* pTask)


mfxStatus VideoVPPHW::PreWorkInputSurface(std::vector<ExtSurface> & surfQueue)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxU32 numSamples = (mfxU32)surfQueue.size();
    mfxHDLPair hdl;
    mfxHDLPair in;

    for (mfxU32 i = 0 ; i < numSamples; i += 1)
    {
        bool bExternal = true;
        mfxMemId memId = 0;

        if (SYS_TO_D3D == m_ioMode || SYS_TO_SYS == m_ioMode)
        {
            mfxU32 resIdx = surfQueue[i].resIdx;

            if( surfQueue[i].bUpdate )
            {
                mfxFrameSurface1 inputVidSurf = {};
                inputVidSurf.Info = surfQueue[i].pSurf->Info;
                inputVidSurf.Data.MemId = m_internalVidSurf[VPP_IN].mids[ resIdx ];
                if (MFX_MIRRORING_HORIZONTAL == m_executeParams.mirroring && MIRROR_INPUT == m_executeParams.mirroringPosition && m_pCmCopy)
                {
                    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "HW_VPP: Mirror (sys->d3d)");

                    mfxSize roi = {surfQueue[i].pSurf->Info.Width, surfQueue[i].pSurf->Info.Height};

                    mfxHDLPair dstHandle = {};
                    mfxMemId srcMemId, dstMemId;

                    mfxFrameSurface1 srcTempSurface, dstTempSurface;

                    memset(&srcTempSurface, 0, sizeof(mfxFrameSurface1));
                    memset(&dstTempSurface, 0, sizeof(mfxFrameSurface1));

                    mfxFrameSurface1 *pDst = &inputVidSurf,
                                     *pSrc = surfQueue[i].pSurf;

                    // save original mem ids
                    srcMemId = pSrc->Data.MemId;
                    dstMemId = pDst->Data.MemId;

                    srcTempSurface.Info = pSrc->Info;
                    dstTempSurface.Info = pDst->Info;

                    bool isSrcLocked = false;

                    if (NULL == pSrc->Data.Y)
                    {
                        sts = m_pCore->LockExternalFrame(srcMemId, &srcTempSurface.Data);
                        MFX_CHECK_STS(sts);

                        isSrcLocked = true;
                    }
                    else
                    {
                        srcTempSurface.Data = pSrc->Data;
                        srcTempSurface.Data.MemId = 0;
                    }

                    sts = m_pCore->GetFrameHDL(dstMemId, (mfxHDL*)&dstHandle);
                    MFX_CHECK_STS(sts);

                    dstTempSurface.Data.MemId = &dstHandle;

                    mfxI64 verticalPitch = (mfxI64)(srcTempSurface.Data.UV - srcTempSurface.Data.Y);
                    // offset beetween Y and UV must be align to pitch
                    MFX_CHECK(!(verticalPitch % srcTempSurface.Data.Pitch), MFX_ERR_UNSUPPORTED);
                    verticalPitch /= srcTempSurface.Data.Pitch;
                    mfxU32 srcPitch = srcTempSurface.Data.PitchLow + ((mfxU32)srcTempSurface.Data.PitchHigh << 16);

                    sts = m_pCmCopy->CopyMirrorSystemToVideoMemory(dstHandle.first, 0, srcTempSurface.Data.Y, srcPitch, (mfxU32)verticalPitch, roi, MFX_FOURCC_NV12);
                    MFX_CHECK_STS(sts);

                    if (true == isSrcLocked)
                    {
                        sts = m_pCore->UnlockExternalFrame(srcMemId, &srcTempSurface.Data);
                        MFX_CHECK_STS(sts);
                    }
                }
                else
                {
                    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "HW_VPP: Copy output (sys->d3d)");

                    if (MFX_FOURCC_P010 == inputVidSurf.Info.FourCC && 0 == inputVidSurf.Info.Shift)
                        inputVidSurf.Info.Shift = 1; // internal memory`s shift should be configured to 1 to call CopyShift CM kernel

                    sts = m_pCore->DoFastCopyWrapper(
                        &inputVidSurf,
                        MFX_MEMTYPE_INTERNAL_FRAME | MFX_MEMTYPE_DXVA2_DECODER_TARGET,
                        surfQueue[i].pSurf,
                        MFX_MEMTYPE_EXTERNAL_FRAME | MFX_MEMTYPE_SYSTEM_MEMORY);
                    MFX_CHECK_STS(sts);
                }
            }

            MFX_SAFE_CALL(m_pCore->GetFrameHDL(m_internalVidSurf[VPP_IN].mids[resIdx], (mfxHDL *)&hdl));
            in = hdl;

            bExternal = false;
            memId     = m_internalVidSurf[VPP_IN].mids[resIdx];
        }
        else
        {
            if(m_IOPattern & MFX_IOPATTERN_IN_OPAQUE_MEMORY){
                MFX_SAFE_CALL(m_pCore->GetFrameHDL(surfQueue[i].pSurf->Data.MemId, (mfxHDL *)&hdl));
                bExternal = false;
            }else{
                MFX_SAFE_CALL(m_pCore->GetExternalFrameHDL(surfQueue[i].pSurf->Data.MemId, (mfxHDL *)&hdl));
                bExternal = true;
            }
            in = hdl;


            memId     = surfQueue[i].pSurf->Data.MemId;
        }

        // register input surface
        sts = (*m_ddi)->Register(&in, 1, TRUE);
        MFX_CHECK_STS(sts);

        memset(&m_executeSurf[i], 0, sizeof(mfxDrvSurface));

        // set input surface
        m_executeSurf[i].hdl = static_cast<mfxHDLPair>(in);
        m_executeSurf[i].frameInfo = surfQueue[i].pSurf->Info;

        // prepare the primary video sample
        m_executeSurf[i].startTimeStamp = surfQueue[i].timeStamp;
        m_executeSurf[i].endTimeStamp   = surfQueue[i].endTimeStamp;

        // debug info
        m_executeSurf[i].bExternal = bExternal;
        m_executeSurf[i].memId = memId;
    }

    return MFX_ERR_NONE;

} // mfxStatus VideoVPPHW::PreWorkInputSurface(...)

mfxStatus VideoVPPHW::PostWorkOutSurfaceCopy(ExtSurface & output)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_API, "VideoVPPHW::PostWorkOutSurfaceCopy");
    mfxStatus sts = MFX_ERR_NONE;

    if (!output.pSurf)
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    // code here to resolve issue in case of sync issue with emcoder in case of system memory
    // [3] Copy sys -> vid
    if(SYS_TO_SYS == m_ioMode || D3D_TO_SYS == m_ioMode)
    {
        mfxFrameSurface1 d3dSurf;
        d3dSurf.Info = output.pSurf->Info;
        if (output.bForcedInternalAlloc)
        {
            // this function is to copy from an internal buffer of system surface to actual memory
            // if bForcedInternalAlloc is true, it says that memid needs to be taken from memid of a the surface
            // but it contradicts to the fact that this surface is allocated in system memory;
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        }
        else
        {
            if (NO_INDEX != output.resIdx)
            {
                d3dSurf.Data.MemId = m_internalVidSurf[VPP_OUT].mids[output.resIdx];
            }
            else
            {
                // there is no index. its an error
                return MFX_ERR_UNDEFINED_BEHAVIOR;
            }
        }

        if (MFX_MIRRORING_HORIZONTAL == m_executeParams.mirroring && MIRROR_OUTPUT == m_executeParams.mirroringPosition && m_pCmCopy)
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "HW_VPP: Mirror (d3d->sys)");

            mfxSize roi = {output.pSurf->Info.Width, output.pSurf->Info.Height};

            mfxHDLPair srcHandle = {};
            mfxMemId srcMemId, dstMemId;

            mfxFrameSurface1 srcTempSurface, dstTempSurface;

            memset(&srcTempSurface, 0, sizeof(mfxFrameSurface1));
            memset(&dstTempSurface, 0, sizeof(mfxFrameSurface1));

            mfxFrameSurface1 *pDst = output.pSurf,
                             *pSrc = &d3dSurf;

            bool isDstLocked = false;

            // save original mem ids
            srcMemId = pSrc->Data.MemId;
            dstMemId = pDst->Data.MemId;

            srcTempSurface.Info = pSrc->Info;
            dstTempSurface.Info = pDst->Info;

            sts = m_pCore->GetFrameHDL(srcMemId, (mfxHDL*)&srcHandle);
            MFX_CHECK_STS(sts);

            if (NULL == pDst->Data.Y)
            {
                sts = m_pCore->LockExternalFrame(dstMemId, &dstTempSurface.Data);
                MFX_CHECK_STS(sts);

                isDstLocked = true;
            }
            else
            {
                dstTempSurface.Data = pDst->Data;
                dstTempSurface.Data.MemId = 0;
            }

            mfxI64 verticalPitch = (mfxI64)dstTempSurface.Data.UV - (mfxI64)dstTempSurface.Data.Y;
            // offset beetween Y and UV must be align to pitch
            MFX_CHECK(!(verticalPitch % dstTempSurface.Data.Pitch), MFX_ERR_UNSUPPORTED);
            verticalPitch /= dstTempSurface.Data.Pitch;
            mfxU32 dstPitch = dstTempSurface.Data.PitchLow + ((mfxU32)dstTempSurface.Data.PitchHigh << 16);

            sts = m_pCmCopy->CopyMirrorVideoToSystemMemory(dstTempSurface.Data.Y, dstPitch, (mfxU32)verticalPitch, srcHandle.first, 0, roi, MFX_FOURCC_NV12);
            MFX_CHECK_STS(sts);

            if (true == isDstLocked)
            {
                    sts = m_pCore->UnlockExternalFrame(dstMemId, &dstTempSurface.Data);
                    MFX_CHECK_STS(sts);
            }
        }
        else
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "HW_VPP: Copy output (d3d->sys)");

            if (MFX_FOURCC_P010 == d3dSurf.Info.FourCC && 0 == d3dSurf.Info.Shift)
                d3dSurf.Info.Shift = 1; // internal memory`s shift should be configured to 1 to call CopyShift CM kernel

            sts = m_pCore->DoFastCopyWrapper(
                output.pSurf,
                MFX_MEMTYPE_EXTERNAL_FRAME | MFX_MEMTYPE_SYSTEM_MEMORY,
                &d3dSurf,
                MFX_MEMTYPE_INTERNAL_FRAME | MFX_MEMTYPE_DXVA2_DECODER_TARGET
                );
            MFX_CHECK_STS(sts);
        }
    }
    return sts;
}

mfxStatus VideoVPPHW::PostWorkOutSurface(ExtSurface & output)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_API, "VideoVPPHW::PostWorkOutSurface");
    mfxStatus sts = MFX_ERR_NONE;
    if (SYS_TO_SYS == m_ioMode || D3D_TO_SYS == m_ioMode)
    {
#ifdef MFX_ENABLE_MCTF
        if (!m_pMCTFilter)
#endif
        {
             // the reason for this is as follows:
             // in case of sys output surface, vpp allocates internal surfaces and use them
             // to submit into driver; then results are copied back to sys surface; in case of MCTF,
             // we have to pass video surface to MCTF, so to avoid redundant copies back-and-forth,
             // copy to output surface is moved to QueryFromMctf right after
             // MCTF is ready to retun surface;
             sts = PostWorkOutSurfaceCopy(output);
             MFX_CHECK_STS(sts);
        }
    };

    // unregister output surface (make sense in case DX9 only)
    sts = (*m_ddi)->Register( &(m_executeParams.targetSurface.hdl), 1, FALSE);
    MFX_CHECK_STS(sts);

    return MFX_ERR_NONE;

} // mfxStatus VideoVPPHW::PostWorkOutSurface(ExtSurface & output)


mfxStatus VideoVPPHW::PostWorkInputSurface(mfxU32 numSamples)
{
    // unregister input surface(s) (make sense in case DX9 only)
    for (mfxU32 i = 0 ; i < numSamples; i += 1)
    {
        mfxStatus sts = (*m_ddi)->Register( &(m_executeSurf[i].hdl), 1, FALSE);
        MFX_CHECK_STS(sts);
    }

    return MFX_ERR_NONE;

} // mfxStatus VideoVPPHW::PostWorkInputSurface(mfxU32 numSamples)


#define CHECK_CM_ERR(ERR) if ((ERR) != CM_SUCCESS) return MFX_ERR_DEVICE_FAILED;

int RunGpu(
    void* inD3DSurf, void* outD3DSurf,
    int fieldMask,
    CmDevice *device, CmQueue *queue, CmKernel *kernel, int WIDTH, int HEIGHT)
{
    int res;
    CmSurface2D *input = 0;
    res = device->CreateSurface2D( inD3DSurf, input);
    CHECK_CM_ERR(res);

    CmSurface2D *output = 0;
    res = device->CreateSurface2D(outD3DSurf, output);
    CHECK_CM_ERR(res);

    unsigned int tsWidth = WIDTH / 16;
    unsigned int tsHeight = HEIGHT / 16;
    res = kernel->SetThreadCount(tsWidth * tsHeight);
    CHECK_CM_ERR(res);

    CmThreadSpace * threadSpace = 0;
    res = device->CreateThreadSpace(tsWidth, tsHeight, threadSpace);
    CHECK_CM_ERR(res);

    SurfaceIndex *idxInput = 0;
    SurfaceIndex *idxOutput = 0;
    res = input->GetIndex(idxInput);
    CHECK_CM_ERR(res);
    res = output->GetIndex(idxOutput);
    CHECK_CM_ERR(res);

    res = kernel->SetKernelArg(0, sizeof(*idxInput), idxInput);
    CHECK_CM_ERR(res);
    res = kernel->SetKernelArg(1, sizeof(*idxOutput), idxOutput);
    CHECK_CM_ERR(res);
    res = kernel->SetKernelArg(2, sizeof(int), &fieldMask);
    CHECK_CM_ERR(res);

    CmTask * task = 0;
    res = device->CreateTask(task);
    CHECK_CM_ERR(res);

    res = task->AddKernel(kernel);
    CHECK_CM_ERR(res);

    //CmQueue *queue = 0;
    //res = device->CreateQueue(queue);
    //CHECK_CM_ERR(res);

    CmEvent * e = 0;
    res = queue->Enqueue(task, e, threadSpace);

    // wait result here!!!
    INT sts;
    mfxStatus status = MFX_ERR_NONE;
    if (CM_SUCCESS == res && e)
    {
        sts = e->WaitForTaskFinished();
        if(sts == CM_EXCEED_MAX_TIMEOUT)
            status = MFX_ERR_GPU_HANG;
    } else {
        status = MFX_ERR_DEVICE_FAILED;
    }
    std::ignore = status;

    if(e) queue->DestroyEvent(e);

    device->DestroyThreadSpace(threadSpace);
    device->DestroyTask(task);

    //queue->DestroyEvent(e);
    device->DestroySurface(input);
    device->DestroySurface(output);

    return CM_SUCCESS;
}

// it is expected video->video, external memory only!!!
mfxStatus VideoVPPHW::ProcessFieldCopy(mfxHDL in, mfxHDL out, mfxU32 fieldMask)
{
    MFX_CHECK_NULL_PTR1(in);
    MFX_CHECK_NULL_PTR1(out);

    if (fieldMask > BFF2FIELD) //valid kernel mask [0, 1, 2, 3, 4, 5, 6, 7]
    {
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    int sts = RunGpu(
        in, out, fieldMask,
        m_pCmDevice, m_pCmQueue, m_pCmKernel, m_executeSurf[0].frameInfo.Width, m_executeSurf[0].frameInfo.Height);

    CHECK_CM_ERR(sts);

    return MFX_ERR_NONE;

}



mfxStatus VideoVPPHW::MergeRuntimeParams(const DdiTask *pTask, MfxHwVideoProcessing::mfxExecuteParams *execParams)
{
    mfxStatus sts = MFX_ERR_NONE;
    MFX_CHECK_NULL_PTR1(pTask);
    MFX_CHECK_NULL_PTR1(execParams);
    mfxU32 numSamples;
    std::vector<mfxFrameSurface1*> inputSurfs;
    size_t i, indx;

    numSamples = pTask->bkwdRefCount + 1 + pTask->fwdRefCount;
    inputSurfs.resize(numSamples);
    execParams->VideoSignalInfo.assign(numSamples, m_executeParams.VideoSignalInfoIn);

    // bkwdFrames
    for(i = 0; i < pTask->bkwdRefCount; i++)
    {
        indx = i;
        inputSurfs[indx] = pTask->m_refList[i].pSurf;
    }

    // cur Frame
    indx = pTask->bkwdRefCount;
    inputSurfs[indx] = pTask->input.pSurf;

    // fwd Frames
    for( i = 0; i < pTask->fwdRefCount; i++ )
    {
        indx = pTask->bkwdRefCount + 1 + i;
        inputSurfs[indx] = pTask->m_refList[pTask->bkwdRefCount + i].pSurf;
    }

    if(m_IOPattern & MFX_IOPATTERN_IN_OPAQUE_MEMORY)
    {
        for (i = 0; i < numSamples; ++i)
        {
            inputSurfs[i] = m_pCore->GetOpaqSurface(inputSurfs[i]->Data.MemId);
            MFX_CHECK(inputSurfs[i], MFX_ERR_NULL_PTR);
        }
    }

    mfxExtVPPVideoSignalInfo *vsi;
    for (i = 0; i < numSamples; i++)
    {
        // Update Video Signal info params for output
        vsi = reinterpret_cast<mfxExtVPPVideoSignalInfo *>(GetExtendedBuffer(inputSurfs[i]->Data.ExtParam,
                                                                             inputSurfs[i]->Data.NumExtParam,
                                                                             MFX_EXTBUFF_VPP_VIDEO_SIGNAL_INFO));
        if (vsi)
        {
            /* Check params */
            if (vsi->TransferMatrix != MFX_TRANSFERMATRIX_BT601 &&
                vsi->TransferMatrix != MFX_TRANSFERMATRIX_BT709 &&
                vsi->TransferMatrix != MFX_TRANSFERMATRIX_UNKNOWN)
            {
                return MFX_ERR_INVALID_VIDEO_PARAM;
            }

            if (vsi->NominalRange != MFX_NOMINALRANGE_0_255 &&
                vsi->NominalRange != MFX_NOMINALRANGE_16_235 &&
                vsi->NominalRange != MFX_NOMINALRANGE_UNKNOWN)
            {
                return MFX_ERR_INVALID_VIDEO_PARAM;
            }

            /* Params look good */
            execParams->VideoSignalInfo[i].enabled        = true;
            execParams->VideoSignalInfo[i].NominalRange   = vsi->NominalRange;
            execParams->VideoSignalInfo[i].TransferMatrix = vsi->TransferMatrix;
        }
    }


    mfxFrameSurface1 * outputSurf = m_IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY ? m_pCore->GetOpaqSurface(pTask->output.pSurf->Data.MemId): pTask->output.pSurf;
    MFX_CHECK(outputSurf, MFX_ERR_NULL_PTR);
    // Update Video Signal info params for output
    vsi = reinterpret_cast<mfxExtVPPVideoSignalInfo *>(GetExtendedBuffer(outputSurf->Data.ExtParam,
                                                                         outputSurf->Data.NumExtParam,
                                                                         MFX_EXTBUFF_VPP_VIDEO_SIGNAL_INFO));
    if (vsi)
    {
            /* Check params */
            if (vsi->TransferMatrix != MFX_TRANSFERMATRIX_BT601 &&
                vsi->TransferMatrix != MFX_TRANSFERMATRIX_BT709 &&
                vsi->TransferMatrix != MFX_TRANSFERMATRIX_UNKNOWN)
            {
                return MFX_ERR_INVALID_VIDEO_PARAM;
            }

            if (vsi->NominalRange != MFX_NOMINALRANGE_0_255 &&
                vsi->NominalRange != MFX_NOMINALRANGE_16_235 &&
                vsi->NominalRange != MFX_NOMINALRANGE_UNKNOWN)
            {
                return MFX_ERR_INVALID_VIDEO_PARAM;
            }

            /* Params look good */
            execParams->VideoSignalInfoOut.enabled        = true;
            execParams->VideoSignalInfoOut.NominalRange   = vsi->NominalRange;
            execParams->VideoSignalInfoOut.TransferMatrix = vsi->TransferMatrix;
     }

#ifdef MFX_ENABLE_VPP_RUNTIME_HSBC
    // Update ProcAmp parameters for output
    mfxExtVPPProcAmp *procamp = reinterpret_cast<mfxExtVPPProcAmp *>(GetExtendedBuffer(outputSurf->Data.ExtParam,
                                                                     outputSurf->Data.NumExtParam,
                                                                     MFX_EXTBUFF_VPP_PROCAMP));

    if (procamp)
    {
        execParams->Brightness     = procamp->Brightness;
        execParams->Saturation     = procamp->Saturation;
        execParams->Hue            = procamp->Hue;
        execParams->Contrast       = procamp->Contrast;
        execParams->bEnableProcAmp = true;
    }
#endif

    return sts;
}

/// Main VPP processing entry point
/*!
  This is where all VPP processing happen.
  This function prepares input/output surface and parameters to pass to driver.

  DEINTERLACE:
  ADI with Scene change detector uses SceneChangeDetector::Get_frame_shot_Decision() and SceneChangeDetector::Get_frame_last_in_scene() to detect scene change.
  It sets up the variable m_executeParams.scene to notify the scene change status to vaapi interface VAAPIVideoProcessing::Execute() function.

  \param[in] pTask
  Pointer to task thread
  \param[out] m_executeParams
  structure passed to driver VAAPI interface
  \return[out] mfxStatus
  MFX_ERR_NONE if there is no error
 */
mfxStatus VideoVPPHW::SyncTaskSubmission(DdiTask* pTask)
{
    mfxStatus sts = MFX_ERR_NONE;
#ifdef MFX_ENABLE_MCTF
    if (pTask->outputForApp.pSurf)
    {
        if (true == pTask->bRunTimeCopyPassThrough)
        {
            sts = CopyPassThrough(pTask->input.pSurf, pTask->outputForApp.pSurf);
            m_taskMngr.CompleteTask(pTask);
            MFX_CHECK_STS(sts);

            return MFX_ERR_NONE;
        }
    }
    else
    {
        // null output surface is possible IFF MCTF enabled & is used.
        if (!m_pMCTFilter || !pTask->bMCTF)
            return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
#else
    if (true == pTask->bRunTimeCopyPassThrough)
    {
        sts = CopyPassThrough(pTask->input.pSurf, pTask->output.pSurf);
        m_taskMngr.CompleteTask(pTask);
        MFX_CHECK_STS(sts);

        return MFX_ERR_NONE;
    }
#endif

    mfxU32 i = 0;
    mfxU32 numSamples   = pTask->bkwdRefCount + 1 + pTask->fwdRefCount;

    std::vector<ExtSurface> surfQueue(numSamples);

    mfxU32 indx = 0;
    mfxU32 deinterlaceAlgorithm = 0;
    bool FMDEnable = 0;

    // bkwdFrames
    for(i = 0; i < pTask->bkwdRefCount; i++)
    {
        indx = i;
        surfQueue[indx] = pTask->m_refList[i];
    }

    // cur Frame
    indx = pTask->bkwdRefCount;
    surfQueue[indx] = pTask->input;

    // fwd Frames
    for( i = 0; i < pTask->fwdRefCount; i++ )
    {
        indx = pTask->bkwdRefCount + 1 + i;
        surfQueue[indx] = pTask->m_refList[pTask->bkwdRefCount + i];
    }

    /* in VPP Field Copy mode we just copy field only */
    mfxU32 imfxFPMode = 0xffffffff;
    if (m_executeParams.iFieldProcessingMode != 0)
    {
        mfxFrameSurface1 * pInputSurface = m_IOPattern & MFX_IOPATTERN_IN_OPAQUE_MEMORY ?
                                            m_pCore->GetOpaqSurface(surfQueue[0].pSurf->Data.MemId):
                                            surfQueue[0].pSurf;
        MFX_CHECK(pInputSurface, MFX_ERR_NULL_PTR);

        /* Mean filter was configured as DOUSE, but no ExtBuf in VPP Init()
         * And ... no any parameters in runtime. This is an error! */
        if (((m_executeParams.iFieldProcessingMode -1) == FROM_RUNTIME_EXTBUFF_FIELD_PROC) && (pInputSurface->Data.NumExtParam == 0))
        {
            return MFX_ERR_INVALID_VIDEO_PARAM;
        }

        if ((m_executeParams.iFieldProcessingMode -1 ) == FROM_RUNTIME_PICSTRUCT)
        {
            mfxU16 srcPicStruct = m_params.vpp.In.PicStruct == MFX_PICSTRUCT_UNKNOWN ?
                                    pInputSurface->Info.PicStruct :
                                    m_params.vpp.In.PicStruct;

            mfxU16 dstPicStruct = m_params.vpp.Out.PicStruct == MFX_PICSTRUCT_UNKNOWN ?
                                    pTask->output.pSurf->Info.PicStruct :
                                    m_params.vpp.Out.PicStruct;

            if (srcPicStruct == MFX_PICSTRUCT_UNKNOWN || dstPicStruct == MFX_PICSTRUCT_UNKNOWN)
            {
                return MFX_ERR_INVALID_VIDEO_PARAM;
            }

            if (srcPicStruct & MFX_PICSTRUCT_FIELD_SINGLE)
            {
                if (dstPicStruct & MFX_PICSTRUCT_FIELD_TFF)
                {
                    imfxFPMode = FIELD2TFF;
                }
                else if (dstPicStruct & MFX_PICSTRUCT_FIELD_BFF)
                {
                    imfxFPMode = FIELD2BFF;
                }
                else
                    return MFX_ERR_INVALID_VIDEO_PARAM;
            }
            else if (dstPicStruct & MFX_PICSTRUCT_FIELD_SINGLE)
            {
                if (srcPicStruct & MFX_PICSTRUCT_FIELD_TFF)
                {
                    imfxFPMode = TFF2FIELD;
                }
                else if (srcPicStruct & MFX_PICSTRUCT_FIELD_BFF)
                {
                    imfxFPMode = BFF2FIELD;
                }
                else
                    return MFX_ERR_INVALID_VIDEO_PARAM;
            }
            imfxFPMode++;
        }
        /* If there is no ExtBuffer in RunTime we can use value from Init() */
        else if ((pInputSurface->Data.NumExtParam == 0) || (pInputSurface->Data.ExtParam == NULL))
        {
            imfxFPMode = m_executeParams.iFieldProcessingMode;
        }
        else /* So on looks like ExtBuf is present in runtime */
        {
            mfxExtVPPFieldProcessing * vppFieldProcessingParams = NULL;
            /* So on, we skip m_executeParams.iFieldProcessingMode from Init() */
            for ( mfxU32 jj = 0; jj < pInputSurface->Data.NumExtParam; jj++ )
            {
                if ( (pInputSurface->Data.ExtParam[jj]->BufferId == MFX_EXTBUFF_VPP_FIELD_PROCESSING) &&
                     (pInputSurface->Data.ExtParam[jj]->BufferSz == sizeof(mfxExtVPPFieldProcessing)) )
                {
                    vppFieldProcessingParams = (mfxExtVPPFieldProcessing *)(pInputSurface->Data.ExtParam[jj]);

                    if (vppFieldProcessingParams->Mode == MFX_VPP_COPY_FIELD)
                    {
                        if(vppFieldProcessingParams->InField ==  MFX_PICSTRUCT_FIELD_TFF)
                        {
                            imfxFPMode = (vppFieldProcessingParams->OutField == MFX_PICSTRUCT_FIELD_TFF) ?
                                            TFF2TFF : TFF2BFF;
                        } else
                        {
                            imfxFPMode = (vppFieldProcessingParams->OutField == MFX_PICSTRUCT_FIELD_TFF) ?
                                    BFF2TFF : BFF2BFF;
                        }
                    } /* if (vppFieldProcessingParams->Mode == MFX_VPP_COPY_FIELD)*/

                    if (vppFieldProcessingParams->Mode == MFX_VPP_COPY_FRAME)
                        imfxFPMode = FRAME2FRAME;
                }
            }

            // "-1" to get valid kernel values is done internally
            // !!! kernel uses "0"  as a "valid" data, but HW_VPP doesn't
            // to prevent issue we increment here and decrement before kernel call
            imfxFPMode++;
        }
    }

    if ((m_executeParams.iFieldProcessingMode != 0) && /* If Mode is enabled*/
        ((imfxFPMode - 1) != (mfxU32)FRAME2FRAME)) /* And we don't do copy frame to frame lets call our FieldCopy*/
    /* And remember our previous line imfxFPMode++;*/
    {
        imfxFPMode--;
        pTask->skipQueryStatus = true; // do not submit any tasks to driver in this mode

        sts = PreWorkOutSurface(pTask->output);
        MFX_CHECK_STS(sts);

        sts = PreWorkInputSurface(surfQueue);
        MFX_CHECK_STS(sts);

        if ((imfxFPMode == TFF2FIELD) || (imfxFPMode == BFF2FIELD))
        {
            if (pTask->taskIndex % 2)
            {
                // switch between TFF2FIELD<->BFF2FIELD for each second iteration
                imfxFPMode = (imfxFPMode == TFF2FIELD) ? BFF2FIELD : TFF2FIELD;
            }
        }
        sts = ProcessFieldCopy(m_executeSurf[0].hdl.first, m_executeParams.targetSurface.hdl.first, imfxFPMode);
        MFX_CHECK_STS(sts);

        if ((imfxFPMode == FIELD2TFF) || (imfxFPMode == FIELD2BFF))
        {
            if (numSamples != 2)
            {
                return MFX_ERR_UNDEFINED_BEHAVIOR;
            }

            // copy the second field to frame
            imfxFPMode = (imfxFPMode == FIELD2TFF) ? FIELD2BFF : FIELD2TFF;
            sts = ProcessFieldCopy(m_executeSurf[1].hdl.first, m_executeParams.targetSurface.hdl.first, imfxFPMode);
            MFX_CHECK_STS(sts);
        }

        m_executeParams.pRefSurfaces = &m_executeSurf[0];
#ifdef MFX_ENABLE_MCTF
        // this is correct that outputForApp is used:
        // provided that MCTF is not used, and system memory is an output,
        // PostWorkOutSurfaceCopy will copy from an internal buffer to real
        // output surface; if MCTF is used, PostWorkOutSurfaceCopy will be 
        // called but in QueryFromMCTF; for D3D output surface no specific 
        // processing is need: if MCTF is not used, outputForApp must be
        // the same as output; if MCTF is used, "copy" to an output surface is done
        // by MCTF when it makes filtration (no copy, just result of filtering 
        // is placed in to the output suface)
        // NB: if MCTF is not used, and system memory is an output surface,
        // its important ot have bForcedInternalAlloc == false,
        if (pTask->outputForApp.pSurf)
            sts = PostWorkOutSurface(pTask->outputForApp);
#else
        sts = PostWorkOutSurface(pTask->output);
#endif
        MFX_CHECK_STS(sts);

        sts = PostWorkInputSurface(numSamples);
        MFX_CHECK_STS(sts);

        // what should be done if pTask->outputForApp.pSurf == nullptr??
        //if (pTask->outputForApp.pSurf)
        m_executeParams.targetSurface.memId = pTask->output.pSurf->Data.MemId;
        m_executeParams.statusReportID = pTask->taskIndex;
        return sts;
    }

    if (MFX_MIRRORING_HORIZONTAL == m_executeParams.mirroring && MIRROR_WO_EXEC == m_executeParams.mirroringPosition && m_pCmCopy)
    {
        /* Temporal solution for mirroring that makes nothing but mirroring
         * TODO: merge mirroring into pipeline
         * UPD: d3d->sys, sys->d3d and sys->sys IO Patterns already merged
         */

        sts = PreWorkOutSurface(pTask->output);
        MFX_CHECK_STS(sts);

        sts = PreWorkInputSurface(surfQueue);
        MFX_CHECK_STS(sts);

        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "HW_VPP: Mirror (d3d->d3d)");
        mfxSize roi = {pTask->output.pSurf->Info.Width, pTask->output.pSurf->Info.Height};
        sts = m_pCmCopy->CopyMirrorVideoToVideoMemory(m_executeParams.targetSurface.hdl.first, m_executeSurf[0].hdl.first, roi, MFX_FOURCC_NV12);
        MFX_CHECK_STS(sts);
#ifdef MFX_ENABLE_MCTF
        // this is correct that outputForApp is used:
        // provided that MCTF is not used, and system memory is an output,
        // PostWorkOutSurfaceCopy will copy from an internal buffer to real
        // output surface; if MCTF is used, PostWorkOutSurfaceCopy will be 
        // called but in QueryFromMCTF; for D3D output surface no specific 
        // processing is need: if MCTF is not used, outputForApp must be
        // the same as output; if MCTF is used, "copy" to an output surface is done
        // by MCTF when it makes filtration (no copy, just result of filtering 
        // is placed in to the output suface)
        // NB: if MCTF is not used, and system memory is an output surface,
        // its important ot have bForcedInternalAlloc == false,
        if (pTask->outputForApp.pSurf)
            sts = PostWorkOutSurface(pTask->outputForApp);
#else
        sts = PostWorkOutSurface(pTask->output);
#endif
        MFX_CHECK_STS(sts);

        sts = PostWorkInputSurface(numSamples);
        MFX_CHECK_STS(sts);

        m_executeParams.targetSurface.memId = pTask->output.pSurf->Data.MemId;
        m_executeParams.statusReportID = pTask->taskIndex;
        return sts;
    }

    sts = PreWorkOutSurface(pTask->output);
    MFX_CHECK_STS(sts);

    sts = PreWorkInputSurface(surfQueue);
    MFX_CHECK_STS(sts);

#if defined(MFX_ENABLE_SCENE_CHANGE_DETECTION_VPP)
    /* In MFX_DEINTERLACING_ADVANCED_SCD (also called ADI_SCD) the current field
     * uses information from previous field and next field (if it is
     * present in current frame) in ADI mode.
     * BOB is used when previous or next field can not be used
     *   1. To deinterlace first frame
     *   2. When scene change occur (first of new scene and last of previous scene)
     *   3. To deinterlace last frame for 30i->60p mode
     * SCD engine detects if a picture belongs to a new scene
     */
    if (MFX_DEINTERLACING_ADVANCED_SCD == m_executeParams.iDeinterlacingAlgorithm)
    {
        mfxU32 scene_change = 0;
        mfxU32 sc_in_first_field = 0;
        mfxU32 sc_in_second_field = 0;
        mfxU32  sc_detected = 0;
        BOOL is30i60pConversion = 0;

        mfxU32 frameIndex = pTask->bkwdRefCount; // index of current frame
        mfxHDL frameHandle;
        mfxFrameSurface1 frameSurface;
        memset(&frameSurface, 0, sizeof(mfxFrameSurface1));

        if (m_executeParams.bDeinterlace30i60p)
        {
            is30i60pConversion = 1;
        }

        /* Feed scene change detector with input frame */
        mfxFrameSurface1 *inFrame = surfQueue[frameIndex].pSurf;
        MFX_CHECK_NULL_PTR1(inFrame);
        frameSurface.Info = inFrame->Info;


        if (SYS_TO_D3D == m_ioMode || SYS_TO_SYS == m_ioMode)
        {
            sts = m_pCore->GetFrameHDL(m_internalVidSurf[VPP_IN].mids[surfQueue[frameIndex].resIdx], &frameHandle);
            MFX_CHECK_STS(sts);
        }
        else
        {
            if(m_IOPattern & MFX_IOPATTERN_IN_OPAQUE_MEMORY)
            {
                MFX_SAFE_CALL(m_pCore->GetFrameHDL(surfQueue[frameIndex].pSurf->Data.MemId, (mfxHDL *)&frameHandle));
            }
            else
            {
                MFX_SAFE_CALL(m_pCore->GetExternalFrameHDL(surfQueue[frameIndex].pSurf->Data.MemId, (mfxHDL *)&frameHandle));
            }
        }
        frameSurface.Data.MemId = frameHandle;

        // Set input frame parity in SCD
        if(frameSurface.Info.PicStruct & MFX_PICSTRUCT_FIELD_TFF)
        {
            m_SCD.SetParityTFF();
        }
        else if (frameSurface.Info.PicStruct & MFX_PICSTRUCT_FIELD_BFF)
        {
            m_SCD.SetParityBFF();
        }
        // Check for scene change
        // Do it only when new input frame are fed to deinterlacer.
        // For 30i->60p, this happens every odd frames as:
        // m_frame_num = 0: input0 -> BOB -> ouput0 but we need to feed SCD engine with first reference frame
        // m_frame_num = 1: input1 + reference input 0 -> ADI -> output1
        // m_frame_num = 2: input1 + reference input 0 -> ADI -> output2 (no need to check as same input is used)
        // m_frame_num = 3: input2 + reference input 1 -> ADI -> output3

        if (is30i60pConversion == 0 || (m_frame_num % 2) == 1 || m_frame_num == 0)
        {
            if (m_SCD.Query_ASCCmDevice())
            {
                if (SYS_TO_D3D == m_ioMode || SYS_TO_SYS == m_ioMode)
                {
                    sts = m_pCore->GetFrameHDL(m_internalVidSurf[VPP_IN].mids[surfQueue[frameIndex].resIdx], &frameHandle);
                    MFX_CHECK_STS(sts);
                }
                else
                {
                    if (m_IOPattern & MFX_IOPATTERN_IN_OPAQUE_MEMORY)
                    {
                        MFX_SAFE_CALL(m_pCore->GetFrameHDL(surfQueue[frameIndex].pSurf->Data.MemId, &frameHandle));
                    }
                    else
                    {
                        MFX_SAFE_CALL(m_pCore->GetExternalFrameHDL(surfQueue[frameIndex].pSurf->Data.MemId, &frameHandle));
                    }
                }
            // SCD detects scene change for field to be display.
            // 30i->30p displays only first of current (frame N = field 2N)
            // for 30i->60p, check happends on odd output frame 2N +1
            // 30i->60p display output 2N+1, 2N+2
            // input fields to SCD detection engine are field 2N + 2 and 2N + 3

            // SCD needs field input in display order
            // SCD detects if input field is the first field of a new Scene 
            // based on current and previous stream statistics
            sts = m_SCD.PutFrameInterlaced(frameHandle);
            MFX_CHECK_STS(sts);

            // detect scene change in first field of current 2N + 2
            sc_in_first_field = m_SCD.Get_frame_shot_Decision();

            // Feed SCD detector with second field of current frame
            sts = m_SCD.PutFrameInterlaced(frameHandle);
            }
            else
            {
                mfxFrameData
                    data = surfQueue[frameIndex].pSurf->Data;
                FrameLocker
                    lock2(m_pCore, data, true);
                if (data.Y == 0)
                    return MFX_ERR_LOCK_MEMORY;
                mfxI32
                    pitch = (mfxI32)(data.PitchLow + (data.PitchHigh << 16));
 
                sts = m_SCD.PutFrameInterlaced(data.Y, pitch);
                MFX_CHECK_STS(sts);

                // detect scene change in first field of current 2N + 2
                sc_in_first_field = m_SCD.Get_frame_shot_Decision();

                // Feed SCD detector with second field of current frame
                sts = m_SCD.PutFrameInterlaced(data.Y, pitch);
            }
            MFX_CHECK_STS(sts);

            // check second field of current
            sc_in_second_field = m_SCD.Get_frame_shot_Decision();//m_SCD.Get_frame_last_in_scene();

            sc_detected = sc_in_first_field + sc_in_second_field;
            scene_change += sc_detected;

            /* Check next state depending on value of previous
             * m_scene_change and detected scene change.
             * Typically, use BOB for last field before scene change and
             * first field of a new scene.
             * For 30i->30p: use BOB + first field of input frame to compute
             * output. For 30i->60p: use BOB + field location when scene
             * change occurs for the first time. Use BOB and first field
             * only for reference when more scene change are detected to
             * avoid out of frame order.
             */
            switch (m_scene_change) // previous status
            {
                case NO_SCENE_CHANGE:
                    // for 30i->60p we display second of reference (field 2N+1) here
                    // displayed field is is last if next is new scene
                    if (is30i60pConversion && sc_in_first_field)
                        m_scene_change = LAST_IN_SCENE;
                    else if (is30i60pConversion && sc_in_second_field)
                        // notify that second field of reference display by ADI will be last
                        m_scene_change = NEXT_IS_LAST_IN_SCENE;
                    else if (scene_change)
                         m_scene_change = SCENE_CHANGE;
                    break;
                case SCENE_CHANGE:
                    if (scene_change)
                        m_scene_change = MORE_SCENE_CHANGE_DETECTED;
                    else
                        m_scene_change = NO_SCENE_CHANGE;
                    break;
                case MORE_SCENE_CHANGE_DETECTED:
                    if (scene_change)
                        m_scene_change = MORE_SCENE_CHANGE_DETECTED;
                    else
                        m_scene_change = NO_SCENE_CHANGE;
                    break;
                default:
                    break;
            } //end switch

        } // end (is30i60pConversion == 0 || m_frame_num % 2 == 1)


        // set up m_executeParams.scene for vaapi interface;
        switch (m_scene_change)
        {
            case NO_SCENE_CHANGE:
                m_executeParams.scene = VPP_NO_SCENE_CHANGE;
                break;
            case SCENE_CHANGE:
                m_executeParams.scene = VPP_SCENE_NEW;
                break;
            case MORE_SCENE_CHANGE_DETECTED:
                m_executeParams.scene = VPP_MORE_SCENE_CHANGE_DETECTED;
                break;
            case LAST_IN_SCENE:
                m_executeParams.scene = VPP_SCENE_NEW;
                m_scene_change = SCENE_CHANGE;
                break;
            case NEXT_IS_LAST_IN_SCENE:
                if (m_frame_num % 2 == 0) // 2N+2 field processed
                {
                    m_scene_change = LAST_IN_SCENE;
                    m_executeParams.scene = VPP_SCENE_NEW;
                }
                break;
            default:
                break;
        } //end switch
    }  // if (MFX_DEINTERLACING_ADVANCED_SCD == m_executeParams.iDeinterlacingAlgorithm)

#endif

    m_executeParams.refCount     = numSamples;
    m_executeParams.bkwdRefCount = pTask->bkwdRefCount;
    m_executeParams.fwdRefCount = pTask->fwdRefCount;
    m_executeParams.pRefSurfaces = &m_executeSurf[0];
    m_executeParams.bEOS         = pTask->bEOS;

    m_executeParams.statusReportID = pTask->taskIndex;

    m_executeParams.iTargetInterlacingMode = DEINTERLACE_ENABLE;

    if( !(m_executeParams.targetSurface.frameInfo.PicStruct & (MFX_PICSTRUCT_PROGRESSIVE)))
    {
        m_executeParams.iTargetInterlacingMode = DEINTERLACE_DISABLE;
        m_executeParams.iDeinterlacingAlgorithm = 0;
        m_executeParams.bFMDEnable = false;
    }

    // Check for progressive frames in interlace streams
    deinterlaceAlgorithm = m_executeParams.iDeinterlacingAlgorithm;
    FMDEnable = m_executeParams.bFMDEnable;

    mfxU32 currFramePicStruct = m_executeSurf[pTask->bkwdRefCount].frameInfo.PicStruct;
    mfxU32 refFramePicStruct = m_executeSurf[0].frameInfo.PicStruct;
    bool isFirstField = true;
    bool isCurrentProgressive = false;
    bool isPreviousProgressive = false;


    // check for progressive frames marked as progressive or pict_struct=5,6,7,8 in H.264
    if ((currFramePicStruct == MFX_PICSTRUCT_PROGRESSIVE) ||
        (currFramePicStruct & MFX_PICSTRUCT_FIELD_REPEATED) ||
        (currFramePicStruct & MFX_PICSTRUCT_FRAME_DOUBLING) ||
        (currFramePicStruct & MFX_PICSTRUCT_FRAME_TRIPLING))
    {
        isCurrentProgressive = true;
    }

    if ((refFramePicStruct == MFX_PICSTRUCT_PROGRESSIVE) ||
        (refFramePicStruct & MFX_PICSTRUCT_FIELD_REPEATED) ||
        (refFramePicStruct & MFX_PICSTRUCT_FRAME_DOUBLING) ||
        (refFramePicStruct & MFX_PICSTRUCT_FRAME_TRIPLING))
    {
        isPreviousProgressive = true;
    }

    // Process progressive frame
    if (isCurrentProgressive)
    {
        m_executeParams.iDeinterlacingAlgorithm = 0;
        m_executeParams.bFMDEnable = false;
    }

    if (m_executeParams.mirroring && m_pCore->GetHWType() >= MFX_HW_TGL_LP && m_pCore->GetVAType() != MFX_HW_D3D9)
        m_executeParams.mirroringExt = true;

    // Need special handling for progressive frame in 30i->60p ADI mode
    if ((pTask->bkwdRefCount == 1) && m_executeParams.bDeinterlace30i60p) {
        if ((m_frame_num % 2) == 0)
            isFirstField = false; // 30i->60p ADI second field correspond to even output frame except for first and last

        // Process progressive current frame
        if (isCurrentProgressive)
        {
            // First field comes from interlace reference
            if ((!isPreviousProgressive) && isFirstField)
            {
                m_executeParams.iDeinterlacingAlgorithm = deinterlaceAlgorithm;
                m_executeParams.bFMDEnable = FMDEnable;
            }
            else
            {
                m_executeParams.iDeinterlacingAlgorithm = 0; // Disable DI for current frame
                m_executeParams.bFMDEnable = false;
            }
        }

        // Disable DI when previous frame is progressive and current frame is interlace
        if (isPreviousProgressive && (!isCurrentProgressive) && isFirstField)
        {
            m_executeParams.iDeinterlacingAlgorithm = 0; // Disable DI for first output frame
            m_executeParams.bFMDEnable = false;
        }
    }


    MfxHwVideoProcessing::mfxExecuteParams  execParams = m_executeParams;
    sts = MergeRuntimeParams(pTask, &execParams);
    MFX_CHECK_STS(sts);

    if (execParams.bComposite &&
        (MFX_HW_D3D11 == m_pCore->GetVAType() && execParams.refCount > MAX_STREAMS_PER_TILE))
    {
        mfxU32 NumExecute = execParams.refCount;
        for (mfxU32 execIdx = 0; execIdx < NumExecute; execIdx++)
        {
            execParams.execIdx = execIdx;
            sts = (*m_ddi)->Execute(&execParams);
            if (sts != MFX_ERR_NONE)
                break;
        }
    }
    else
    {
        sts = (*m_ddi)->Execute(&execParams);
    }

    if (sts != MFX_ERR_NONE)
    {
        pTask->SetFree(true);
        return sts;
    }

    MFX_CHECK_STS(sts);

    //wa for variance
    if(execParams.bVarianceEnable)
    {
        //pTask->frameNumber = m_executeParams.frameNumber;
        pTask->bVariance   = true;
    }
#ifdef MFX_ENABLE_MCTF
    // this is correct that outputForApp is used:
    // provided that MCTF is not used, and system memory is an output,
    // PostWorkOutSurfaceCopy will copy from an internal buffer to real
    // output surface; if MCTF is used, PostWorkOutSurfaceCopy will be 
    // called but in QueryFromMCTF; for D3D output surface no specific 
    // processing is need: if MCTF is not used, outputForApp must be
    // the same as output; if MCTF is used, "copy" to an output surface is done
    // by MCTF when it makes filtration (no copy, just result of filtering 
    // is placed in to the output suface)
    // NB: if MCTF is not used, and system memory is an output surface,
    // its important ot have bForcedInternalAlloc == false,
    //if (pTask->outputForApp.pSurf)
        sts = PostWorkOutSurface(pTask->outputForApp);
#else
    sts = PostWorkOutSurface(pTask->output);
#endif
    MFX_CHECK_STS(sts);

    sts = PostWorkInputSurface(numSamples);
    MFX_CHECK_STS(sts);

    // restore value for m_executeParams.iDeinterlacingAlgorithm
    m_executeParams.iDeinterlacingAlgorithm = deinterlaceAlgorithm;
    m_executeParams.bFMDEnable = FMDEnable;
    m_frame_num++; // used to derive first or second field
    return MFX_ERR_NONE;

} // mfxStatus VideoVPPHW::SyncTaskSubmission(DdiTask* pTask)


mfxStatus VideoVPPHW::AsyncTaskSubmission(void *pState, void *pParam, mfxU32 threadNumber, mfxU32 callNumber)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_API, "VideoVPPHW::AsyncTaskSubmission");
    mfxStatus sts;

    // touch unreferenced parameters(s)
    (void)threadNumber;
    (void)callNumber;

#ifdef MFX_ENABLE_MCTF
    VideoVPPHW *pHwVpp = (VideoVPPHW *)pState;
    DdiTask *pTask = (DdiTask*)pParam;
    bool bMCTFinUse = pTask->bMCTF && pHwVpp->m_pMCTFilter;
    if (bMCTFinUse)
        if (!pTask->input.pSurf)
            return MFX_TASK_DONE;
#endif
    sts = ((VideoVPPHW *)pState)->SyncTaskSubmission((DdiTask *)pParam);
    MFX_CHECK_STS(sts);

    return MFX_TASK_DONE;

} // mfxStatus VideoVPPHW::AsyncTaskSubmission(void *pState, void *pParam, mfxU32 threadNumber, mfxU32 callNumber)

mfxStatus VideoVPPHW::QueryTaskRoutine(void *pState, void *pParam, mfxU32 threadNumber, mfxU32 callNumber)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_API, "VideoVPPHW::QueryTaskRoutine");
    mfxStatus sts = MFX_ERR_NONE;

    (void)threadNumber;
    (void)callNumber;

    VideoVPPHW *pHwVpp = (VideoVPPHW *) pState;
    DdiTask *pTask     = (DdiTask*) pParam;

    UMC::AutomaticUMCMutex guard(pHwVpp->m_guard);

    mfxU32 currentTaskIdx = pTask->taskIndex;

#ifdef MFX_ENABLE_MCTF
    bool InputSurfaceIsProcessed = true;

    bool bMCTFinUse = pTask->bMCTF && pHwVpp->m_pMCTFilter;
    if (bMCTFinUse && !pTask->input.pSurf)
        InputSurfaceIsProcessed = false;

    if (InputSurfaceIsProcessed)
    {
#endif
        if (!pTask->skipQueryStatus && !pHwVpp->m_executeParams.mirroring) {
            mfxU32 currSubTaskIdx = pHwVpp->m_taskMngr.GetSubTask(pTask);
            while (currSubTaskIdx != NO_INDEX)
            {
                sts = (*pHwVpp->m_ddi)->QueryTaskStatus(currSubTaskIdx);
                if (sts == MFX_TASK_DONE || sts == MFX_ERR_NONE)
                {
                    pHwVpp->m_taskMngr.DeleteSubTask(pTask, currSubTaskIdx);
                    currSubTaskIdx = pHwVpp->m_taskMngr.GetSubTask(pTask);
                }
                else
                    currSubTaskIdx = NO_INDEX;
            }
            if (sts == MFX_TASK_DONE || sts == MFX_ERR_NONE)
                sts = (*pHwVpp->m_ddi)->QueryTaskStatus(currentTaskIdx);
            if (sts == MFX_ERR_DEVICE_FAILED ||
                sts == MFX_ERR_GPU_HANG)
            {
                sts = MFX_ERR_GPU_HANG;
                pHwVpp->m_critical_error = sts;
                pHwVpp->m_taskMngr.CompleteTask(pTask);
            }

            MFX_CHECK_STS(sts);
        }

        //[2] Variance
        if (pTask->bVariance)
        {
            // windows part
            std::vector<UINT> variance;

            sts = (*pHwVpp->m_ddi)->QueryVariance(
                pTask->taskIndex,
                variance);
            MFX_CHECK_STS(sts);

            if (pTask->pAuxData)
            {
                if (MFX_EXTBUFF_VPP_AUXDATA == pTask->pAuxData->Header.BufferId)
                {
                    pTask->pAuxData->PicStruct = EstimatePicStruct(
                        &variance[0],
                        pHwVpp->m_params.vpp.Out.Width,
                        pHwVpp->m_params.vpp.Out.Height);
                }
            }
        }
#ifdef MFX_ENABLE_MCTF
    }

    if (pTask->bMCTF && pHwVpp->m_pMCTFilter)
    {
        guard.Unlock();
        bool bMctfReadyToReturn(false);
        sts = SubmitToMctf(pState, pParam, &bMctfReadyToReturn);
        MFX_CHECK_STS(sts);
        sts = QueryFromMctf(pState, pParam, bMctfReadyToReturn);
        MFX_CHECK_STS(sts);
        guard.Lock();

        // it might be empty if MCTF works with delay and it is finalizing the stream;
        // in this case no locked internal surfaces
        if (!pHwVpp->m_Surfaces2Unlock.empty())
        {
            // this is to release a surface inside MCTF pool; correposnds to MCTF_GetEmptySurface in VPPFrameCheck
            sts = pHwVpp->m_pCore->DecreaseReference(&(pHwVpp->m_Surfaces2Unlock.front()->Data));
            MFX_CHECK_STS(sts);
            pHwVpp->m_Surfaces2Unlock.pop_front();
        }
    }
#endif
        // [4] Complete task
        sts = pHwVpp->m_taskMngr.CompleteTask(pTask);
    return sts;

} // mfxStatus VideoVPPHW::QueryTaskRoutine(void *pState, void *pParam, mfxU32 threadNumber, mfxU32 callNumber)


#ifdef MFX_ENABLE_MCTF
mfxStatus VideoVPPHW::SubmitToMctf(void *pState, void *pParam, bool* bMctfReadyToReturn)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_API, "VideoVPPHW::SubmitToMctf");
    mfxStatus sts = MFX_ERR_NONE;

    VideoVPPHW *pHwVpp = (VideoVPPHW *)pState;
    DdiTask *pTask = (DdiTask*)pParam;

    if (pTask->bMCTF && pHwVpp->m_pMCTFilter)
    {
        *bMctfReadyToReturn = true;
        if (pTask->input.pSurf)
        {
           // actual output surface in case of system memory has internal buffering
           // which should substitute outputsurface itself for the purposes of
           // forceing MCTF to filter directly into output

            mfxFrameSurface1 d3dSurf;
            bool bOutForcedInternalAlloc = pTask->outputForApp.bForcedInternalAlloc;
            memset(&d3dSurf, 0, sizeof(mfxFrameSurface1));
            mfxFrameSurface1* pd3dSurf = pTask->outputForApp.pSurf ? &d3dSurf : nullptr;
            if (pd3dSurf)
            {
                d3dSurf.Info = pTask->outputForApp.pSurf->Info;
                d3dSurf.Data.MemId = pTask->outputForApp.pSurf->Data.MemId;

                // if an output surface in system memory, MCTF will put results into
                // an internal buffer associated with system memory that then will be
               // copied to actual surface.
                if (SYS_TO_SYS == pHwVpp->m_ioMode || D3D_TO_SYS == pHwVpp->m_ioMode)
                {
                    if (NO_INDEX != pTask->outputForApp.resIdx)
                    {
                        d3dSurf.Data.MemId = pHwVpp->m_internalVidSurf[VPP_OUT].mids[pTask->outputForApp.resIdx];
                    }
                    else
                    {
                        // if its here, it means there is no internal buffer can be identified for an output surface;
                        // cannot proceed. exit.
                        return MFX_ERR_UNDEFINED_BEHAVIOR;
                    }
                }
            }
            
            mfxFrameSurface1* pSurf = pTask->output.pSurf;
            bool bInForcedInternalAlloc = pTask->output.bForcedInternalAlloc;
            // take control out of input surface:
            IntMctfParams* MctfData = pTask->MctfControlActive ? &pTask->MctfData : nullptr;
            // that's Ok to as local variable d3dSurf as it will not be copied; only actual handle will
            // be taken & CmSurface2D will be created (or found from the pool)


  
            mfxHDLPair handle = {};
            CmSurface2D* pSurfCm(nullptr), *pSurfOutCm(nullptr);
            SurfaceIndex* pSurfIdxCm(nullptr), *pSurfOutIdxCm(nullptr);

            MFX_SAFE_CALL(pHwVpp->GetFrameHandle(pSurf, handle, bInForcedInternalAlloc));
            MFX_SAFE_CALL(pHwVpp->CreateCmSurface2D(reinterpret_cast<AbstractSurfaceHandle>(handle.first), pSurfCm, pSurfIdxCm));

            if (pd3dSurf) 
            {
                handle = {};
                MFX_SAFE_CALL(pHwVpp->GetFrameHandle(pd3dSurf, handle, bOutForcedInternalAlloc));
                MFX_SAFE_CALL(pHwVpp->CreateCmSurface2D(reinterpret_cast<AbstractSurfaceHandle>(handle.first), pSurfOutCm, pSurfOutIdxCm));
            }
            
            //sts = pHwVpp->m_pMCTFilter->MCTF_PUT_FRAME(MctfData, pSurf, pd3dSurf, bInForcedInternalAlloc, bOutForcedInternalAlloc);

            sts = pHwVpp->m_pMCTFilter->MCTF_PUT_FRAME(MctfData, pSurfCm, pSurfOutCm);

            // --- access to the internal MCTF queue to increase buffer_count: no need to protect by mutex, as 1 writer & 1 reader
            MFX_SAFE_CALL(pHwVpp->m_pMCTFilter->MCTF_UpdateBufferCount());
            
            // filtering itself
            MFX_SAFE_CALL(pHwVpp->m_pMCTFilter->MCTF_DO_FILTERING());

            *bMctfReadyToReturn = pHwVpp->m_pMCTFilter->MCTF_ReadyToOutput();
        }
    }
    else
        if (pTask->bMCTF && !pHwVpp->m_pMCTFilter)
            return MFX_ERR_UNDEFINED_BEHAVIOR;

    return sts;
}
mfxStatus VideoVPPHW::QueryFromMctf(void *pState, void *pParam, bool bMctfReadyToReturn, bool bEoF)
{
    (void)bEoF;

    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_API, "VideoVPPHW::SubmitToMctf");
    mfxStatus sts = MFX_ERR_NONE;

    VideoVPPHW *pHwVpp = (VideoVPPHW *)pState;
    DdiTask *pTask = (DdiTask*)pParam;

    if (!pTask)
        return sts;

    if (bMctfReadyToReturn)
    {

        mfxFrameSurface1 d3dSurf;
        memset(&d3dSurf, 0, sizeof(mfxFrameSurface1));
        mfxFrameSurface1* pSurf = pTask->outputForApp.pSurf;

        bool bForcedInternalAlloc = pTask->outputForApp.bForcedInternalAlloc;
        if (SYS_TO_SYS == pHwVpp->m_ioMode || D3D_TO_SYS == pHwVpp->m_ioMode)
        {
            //bForcedInternalAlloc = true; // this is true in case of *_TO_SYS ioMode
            // borrowed from  postworkoutput

            if (NO_INDEX != pTask->outputForApp.resIdx) 
            {
                d3dSurf.Info = pTask->outputForApp.pSurf->Info;
                d3dSurf.Data.MemId = pHwVpp->m_internalVidSurf[VPP_OUT].mids[pTask->outputForApp.resIdx];
            }
            else
            {
                // if its here, it means there is no internal buffer allocated for a system surface;
                // cannot proceed.
                return MFX_ERR_UNDEFINED_BEHAVIOR;
            }
            pSurf = &d3dSurf;
        }

        mfxHDLPair handle = {};
        CmSurface2D* pSurfCm(nullptr);
        SurfaceIndex* pSurfIdxCm(nullptr);

        MFX_SAFE_CALL(pHwVpp->GetFrameHandle(pSurf, handle, bForcedInternalAlloc));
        MFX_SAFE_CALL(pHwVpp->CreateCmSurface2D(reinterpret_cast<AbstractSurfaceHandle>(handle.first), pSurfCm, pSurfIdxCm));

        pHwVpp->m_pMCTFilter->MCTF_GET_FRAME(pSurfCm);
        pHwVpp->m_pMCTFilter->MCTF_TrackTimeStamp(pSurf);

        //pHwVpp->m_pMCTFilter->MCTF_GET_FRAME(pSurf, bForcedInternalAlloc);
        // for an outputForApp surface bForcedInternalAlloc must be false at all times;
        if (pTask->outputForApp.bForcedInternalAlloc)
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        // this function will copy from m_internalVidSurf to system memory only in case of SYS output
        sts = pHwVpp->PostWorkOutSurfaceCopy(pTask->outputForApp);
        MFX_CHECK_STS(sts);
    }

    /*
    if (MFX_ERR_NONE != sts)
        wprintf(L"VideoVPPHW::QueryFromMctf: MFX_ERR_NONE != sts\n");
    */
    return sts;
}
#endif

mfxStatus ValidateParams(mfxVideoParam *par, mfxVppCaps *caps, VideoCORE *core, bool bCorrectionEnable)
{
    MFX_CHECK_NULL_PTR3(par, caps, core);

    mfxStatus sts = MFX_ERR_NONE;

#ifdef MFX_VA_LINUX
    if((par->vpp.In.CropW  == 1) ||
       (par->vpp.In.CropH  == 1) ||
       (par->vpp.Out.CropW == 1) ||
       (par->vpp.Out.CropH == 1)){
        sts = GetWorstSts(sts, MFX_ERR_UNSUPPORTED);
    }
#endif

    std::vector<mfxU32> pipelineList;
    mfxStatus internalSts = GetPipelineList(par, pipelineList, true);
    mfxU32 pLen = (mfxU32)pipelineList.size();
    mfxU32* pList = (pLen > 0) ? (mfxU32*)&pipelineList[0] : NULL;
    MFX_CHECK_STS(internalSts);

    /* 1. Check ext param */
    for (mfxU32 i = 0; i < par->NumExtParam; i++)
    {
        mfxU32 id    = par->ExtParam[i]->BufferId;
        void  *data  = par->ExtParam[i];
        MFX_CHECK_NULL_PTR1(data);

        switch(id)
        {
        case MFX_EXTBUFF_VPP_MIRRORING:
        {
            mfxExtVPPMirroring* extMir = (mfxExtVPPMirroring*)data;
            bool isOnlyHorizontalMirroringSupported = true;
            bool isOnlyVideoMemory = false;

            switch (par->IOPattern)
            {
            case MFX_IOPATTERN_IN_VIDEO_MEMORY  | MFX_IOPATTERN_OUT_VIDEO_MEMORY:
            case MFX_IOPATTERN_IN_VIDEO_MEMORY  | MFX_IOPATTERN_OUT_OPAQUE_MEMORY:
            case MFX_IOPATTERN_IN_OPAQUE_MEMORY | MFX_IOPATTERN_OUT_VIDEO_MEMORY:
            case MFX_IOPATTERN_IN_OPAQUE_MEMORY | MFX_IOPATTERN_OUT_OPAQUE_MEMORY:
            {
                isOnlyVideoMemory = true;
                break;
            }
            default:
                break;
            }

            // On Linux with d3d_to_d3d memory type, mirroring performs through driver kernel
            // There is support for both modes
            if (isOnlyVideoMemory && core->GetVAType() == MFX_HW_VAAPI)
                isOnlyHorizontalMirroringSupported = false;

            // Starting from TGL, mirroring performs through driver
            // Driver supports horizontal and vertical modes
            if (core->GetHWType() >= MFX_HW_TGL_LP && core->GetVAType() != MFX_HW_D3D9)
                isOnlyHorizontalMirroringSupported = false;

            // Only SW mirroring has these limitations
            if (isOnlyHorizontalMirroringSupported && (par->vpp.In.FourCC != MFX_FOURCC_NV12 || par->vpp.Out.FourCC != MFX_FOURCC_NV12))
                sts = GetWorstSts(sts, MFX_ERR_UNSUPPORTED);

            // SW mirroring does not support crop X and Y
            if (isOnlyHorizontalMirroringSupported && (par->vpp.In.CropX || par->vpp.In.CropY || par->vpp.Out.CropX || par->vpp.Out.CropY))
                sts = GetWorstSts(sts, MFX_ERR_UNSUPPORTED);

            if (extMir->Type < 0 || (extMir->Type==MFX_MIRRORING_VERTICAL && isOnlyHorizontalMirroringSupported))
                sts = GetWorstSts(sts, MFX_ERR_UNSUPPORTED);

            // SW d3d->d3d mirroring does not support resize
            if (isOnlyHorizontalMirroringSupported && isOnlyVideoMemory &&
               (par->vpp.In.Width != par->vpp.Out.Width || par->vpp.In.Height != par->vpp.Out.Height))
                sts = GetWorstSts(sts, MFX_ERR_UNSUPPORTED);

            // If pipeline contains resize, SW mirroring and other, VPP skips other filters
            if (isOnlyHorizontalMirroringSupported && isOnlyVideoMemory && pLen > 2)
                sts = GetWorstSts(sts, MFX_WRN_FILTER_SKIPPED);

            break;
        }

        case MFX_EXTBUFF_VPP_FIELD_PROCESSING:
        {
            mfxExtVPPFieldProcessing* extFP = (mfxExtVPPFieldProcessing*)data;

            if (extFP->Mode != MFX_VPP_COPY_FRAME && extFP->Mode != MFX_VPP_COPY_FIELD)
            {
                // Unsupported mode
                sts = GetWorstSts(sts, MFX_ERR_UNSUPPORTED);
            }

            if (extFP->Mode == MFX_VPP_COPY_FIELD)
            {
                if( (extFP->InField != MFX_PICSTRUCT_FIELD_TFF  && extFP->InField != MFX_PICSTRUCT_FIELD_BFF)
                 || (extFP->OutField != MFX_PICSTRUCT_FIELD_TFF && extFP->OutField != MFX_PICSTRUCT_FIELD_BFF) )
                {
                    // Copy field needs specific type of interlace
                    sts = GetWorstSts(sts, MFX_ERR_UNSUPPORTED);
                }
            }
            break;
        } //case MFX_EXTBUFF_VPP_FIELD_PROCESSING

        case MFX_EXTBUFF_VPP_DEINTERLACING:
        {
            mfxExtVPPDeinterlacing* extDI = (mfxExtVPPDeinterlacing*) data;

            if (extDI->Mode != MFX_DEINTERLACING_ADVANCED &&
#if defined(MFX_ENABLE_SCENE_CHANGE_DETECTION_VPP)
                extDI->Mode != MFX_DEINTERLACING_ADVANCED_SCD &&
#endif
                extDI->Mode != MFX_DEINTERLACING_ADVANCED_NOREF &&
                extDI->Mode != MFX_DEINTERLACING_BOB &&
                extDI->Mode != MFX_DEINTERLACING_FIELD_WEAVING)
            {
                sts = GetWorstSts(sts, MFX_ERR_UNSUPPORTED);
            }

            if (core->GetHWType() < MFX_HW_ICL && (
#if (MFX_VERSION >= 1027)
                (MFX_FOURCC_Y210 == par->vpp.In.FourCC || MFX_FOURCC_Y210 == par->vpp.Out.FourCC) ||
                (MFX_FOURCC_Y410 == par->vpp.In.FourCC || MFX_FOURCC_Y410 == par->vpp.Out.FourCC) ||
#endif
#if (MFX_VERSION >= 1031)
                (MFX_FOURCC_P016 == par->vpp.In.FourCC || MFX_FOURCC_P016 == par->vpp.Out.FourCC) ||
#endif
                (MFX_FOURCC_AYUV == par->vpp.In.FourCC || MFX_FOURCC_AYUV == par->vpp.Out.FourCC) ||
                (MFX_FOURCC_P010 == par->vpp.In.FourCC || MFX_FOURCC_P010 == par->vpp.Out.FourCC) ))
            {
                sts = GetWorstSts(sts, MFX_ERR_UNSUPPORTED);
            }

#if defined(MFX_ENABLE_SCENE_CHANGE_DETECTION_VPP)
            if (extDI->Mode == MFX_DEINTERLACING_ADVANCED_SCD &&
                par->vpp.Out.FourCC != MFX_FOURCC_NV12 &&
                par->vpp.Out.FourCC != MFX_FOURCC_YV12)
            {
                // SCD kernels support only planar 8-bit formats
                sts = GetWorstSts(sts, MFX_ERR_UNSUPPORTED);
            }
#endif
            break;
        } //case MFX_EXTBUFF_VPP_DEINTERLACING
        case MFX_EXTBUFF_VPP_DOUSE:
        {
            mfxExtVPPDoUse*   extDoUse  = (mfxExtVPPDoUse*)data;
            MFX_CHECK_NULL_PTR1(extDoUse);
            for( mfxU32 algIdx = 0; algIdx < extDoUse->NumAlg; algIdx++ )
            {
                if( !CheckDoUseCompatibility( extDoUse->AlgList[algIdx] ) )
                {
                    sts = GetWorstSts(sts, MFX_ERR_UNSUPPORTED);
                    continue; // stop working with ExtParam[i]
                }

                if(MFX_EXTBUFF_VPP_COMPOSITE == extDoUse->AlgList[algIdx])
                {
                    sts = GetWorstSts(sts, MFX_ERR_UNSUPPORTED);
                    continue; // stop working with ExtParam[i]
                }

                if(MFX_EXTBUFF_VPP_FIELD_PROCESSING == extDoUse->AlgList[algIdx])
                {
                    /* NOTE:
                    * It's legal to use DOUSE for field processing,
                    * but application must attach appropriate ext buffer to mfxFrameData for each input surface
                    */
                    //sts = MFX_ERR_INVALID_VIDEO_PARAM;
                    //continue; // stop working with ExtParam[i]
                }


                if(MFX_EXTBUFF_VPP_SCENE_ANALYSIS == extDoUse->AlgList[algIdx])
                {
                    sts = GetWorstSts(sts, MFX_ERR_UNSUPPORTED);
                    continue; // stop working with ExtParam[i]
                }
            }
            break;
        } //case MFX_EXTBUFF_VPP_DOUSE
        case MFX_EXTBUFF_VPP_COMPOSITE:
        {
            mfxExtVPPComposite* extComp = (mfxExtVPPComposite*)data;
            MFX_CHECK_NULL_PTR1(extComp);
            if (extComp->NumInputStream > MAX_NUM_OF_VPP_COMPOSITE_STREAMS)
                sts = GetWorstSts(sts, MFX_ERR_UNSUPPORTED);
            if (MFX_HW_D3D9 == core->GetVAType() && extComp->NumInputStream)
            {
                // AlphaBlending & LumaKey filters are not supported at first sub-stream on DX9, DX9 can't process more than 8 channels
                if (extComp->InputStream[0].GlobalAlphaEnable ||
                    extComp->InputStream[0].LumaKeyEnable ||
                    extComp->InputStream[0].PixelAlphaEnable ||
                    (extComp->NumInputStream > MAX_STREAMS_PER_TILE))
                {
                    sts = GetWorstSts(sts, MFX_ERR_UNSUPPORTED);
                }
            }

            if (core->GetHWType() < MFX_HW_CNL && (
#if (MFX_VERSION >= 1027)
                (MFX_FOURCC_Y210 == par->vpp.In.FourCC || MFX_FOURCC_Y210 == par->vpp.Out.FourCC) ||
                (MFX_FOURCC_Y410 == par->vpp.In.FourCC || MFX_FOURCC_Y410 == par->vpp.Out.FourCC) ||
#endif
#if (MFX_VERSION >= 1031)
                (MFX_FOURCC_P016 == par->vpp.In.FourCC || MFX_FOURCC_P016 == par->vpp.Out.FourCC) ||
#endif
                (MFX_FOURCC_AYUV == par->vpp.In.FourCC || MFX_FOURCC_AYUV == par->vpp.Out.FourCC) ||
                (                                         MFX_FOURCC_P010 == par->vpp.Out.FourCC) ))
            {
                sts = GetWorstSts(sts, MFX_ERR_UNSUPPORTED);
            }
            else if (core->GetHWType() <= MFX_HW_BDW && MFX_FOURCC_P010 == par->vpp.In.FourCC)
            {
                sts = GetWorstSts(sts, MFX_ERR_UNSUPPORTED);
            }

            break;
        } //case MFX_EXTBUFF_VPP_COMPOSITE
        } // switch
    }

    /* 2. p010 video memory should be shifted */
    if ((MFX_FOURCC_P010 == par->vpp.In.FourCC  && 0 == par->vpp.In.Shift  && par->IOPattern & MFX_IOPATTERN_IN_VIDEO_MEMORY) ||
        (MFX_FOURCC_P010 == par->vpp.Out.FourCC && 0 == par->vpp.Out.Shift && par->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY))
        sts = GetWorstSts(sts, MFX_ERR_UNSUPPORTED);


    /* 3. Check single field cases */
    if ( (par->vpp.In.PicStruct & MFX_PICSTRUCT_FIELD_SINGLE) && !(par->vpp.Out.PicStruct & MFX_PICSTRUCT_FIELD_SINGLE) )
    {
        if (!IsFilterFound(pList, pLen, MFX_EXTBUFF_VPP_FIELD_WEAVING) || // FIELD_WEAVING filter must be there
            (IsFilterFound(pList, pLen, MFX_EXTBUFF_VPP_RESIZE) &&
             pLen > 2) ) // there is another filter except implicit RESIZE
        {
            sts = (MFX_ERR_UNSUPPORTED < sts) ? MFX_ERR_UNSUPPORTED : sts;
        }

        // VPP SyncTaskSubmission returns MFX_ERR_UNSUPPORTED when output picstructure has no parity
        if (!(par->vpp.Out.PicStruct & (MFX_PICSTRUCT_FIELD_BFF | MFX_PICSTRUCT_FIELD_TFF)) && !(par->vpp.Out.PicStruct == MFX_PICSTRUCT_UNKNOWN))
        {
            sts = GetWorstSts(sts, MFX_ERR_UNSUPPORTED);
        }
    }

    if( !(par->vpp.In.PicStruct & MFX_PICSTRUCT_FIELD_SINGLE) && (par->vpp.Out.PicStruct & MFX_PICSTRUCT_FIELD_SINGLE) )
    {
        // Input can not be progressive
        if (!(par->vpp.In.PicStruct & MFX_PICSTRUCT_FIELD_TFF) && !(par->vpp.In.PicStruct & MFX_PICSTRUCT_FIELD_BFF) && !(par->vpp.In.PicStruct == MFX_PICSTRUCT_UNKNOWN))
        {
            sts = GetWorstSts(sts, MFX_ERR_UNSUPPORTED);
        }

        if (!IsFilterFound(pList, pLen, MFX_EXTBUFF_VPP_FIELD_SPLITTING) || // FIELD_SPLITTING filter must be there
            (IsFilterFound(pList, pLen, MFX_EXTBUFF_VPP_RESIZE)                &&
             pLen > 2) ) // there are other filters except implicit RESIZE
        {
            sts = (MFX_ERR_UNSUPPORTED < sts) ? MFX_ERR_UNSUPPORTED : sts;
        }
    }

    /* 4. Check size */
    if (par->vpp.In.Width > caps->uMaxWidth  || par->vpp.In.Height  > caps->uMaxHeight ||
        par->vpp.Out.Width > caps->uMaxWidth || par->vpp.Out.Height > caps->uMaxHeight)
        sts = GetWorstSts(sts, MFX_WRN_PARTIAL_ACCELERATION);

    /* 5. Check fourcc */
    if ( !(caps->mFormatSupport[par->vpp.In.FourCC] & MFX_FORMAT_SUPPORT_INPUT) || !(caps->mFormatSupport[par->vpp.Out.FourCC] & MFX_FORMAT_SUPPORT_OUTPUT) )
        sts = GetWorstSts(sts, MFX_WRN_PARTIAL_ACCELERATION);

    /* 6. BitDepthLuma and BitDepthChroma should be configured for p010 format */
    if (MFX_FOURCC_P010 == par->vpp.In.FourCC
#if (MFX_VERSION >= 1027)
        || par->vpp.In.FourCC == MFX_FOURCC_Y210
        || par->vpp.In.FourCC == MFX_FOURCC_Y410
#endif
        )
    {
        if (0 == par->vpp.In.BitDepthLuma)
        {
            par->vpp.In.BitDepthLuma = 10;
            sts = GetWorstSts(sts, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
        }
        if (0 == par->vpp.In.BitDepthChroma)
        {
            par->vpp.In.BitDepthChroma = 10;
            sts = GetWorstSts(sts, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
        }
    }

    if (MFX_FOURCC_P010 == par->vpp.Out.FourCC
#if (MFX_VERSION >= 1027)
        || par->vpp.Out.FourCC == MFX_FOURCC_Y210
        || par->vpp.Out.FourCC == MFX_FOURCC_Y410
#endif
        )
    {
        if (0 == par->vpp.Out.BitDepthLuma)
        {
            par->vpp.Out.BitDepthLuma = 10;
            sts = GetWorstSts(sts, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
        }
        if (0 == par->vpp.Out.BitDepthChroma)
        {
            par->vpp.Out.BitDepthChroma = 10;
            sts = GetWorstSts(sts, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
        }
    }

#if (MFX_VERSION >= 1031)
    if (   par->vpp.In.FourCC == MFX_FOURCC_P016
        || par->vpp.In.FourCC == MFX_FOURCC_Y216
        || par->vpp.In.FourCC == MFX_FOURCC_Y416
        )
    {
        if (0 == par->vpp.In.BitDepthLuma)
        {
            par->vpp.In.BitDepthLuma = 12;
            sts = GetWorstSts(sts, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
        }
        if (0 == par->vpp.In.BitDepthChroma)
        {
            par->vpp.In.BitDepthChroma = 12;
            sts = GetWorstSts(sts, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
        }
    }

    if (   par->vpp.Out.FourCC == MFX_FOURCC_P016
        || par->vpp.Out.FourCC == MFX_FOURCC_Y216
        || par->vpp.Out.FourCC == MFX_FOURCC_Y416
        )
    {
        if (0 == par->vpp.Out.BitDepthLuma)
        {
            par->vpp.Out.BitDepthLuma = 12;
            sts = GetWorstSts(sts, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
        }
        if (0 == par->vpp.Out.BitDepthChroma)
        {
            par->vpp.Out.BitDepthChroma = 12;
            sts = GetWorstSts(sts, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
        }
    }
#endif

    /* 8. Check unsupported filters on RGB */
    if( par->vpp.In.FourCC == MFX_FOURCC_RGB4)
    {
        if(IsFilterFound(pList, pLen, MFX_EXTBUFF_VPP_DENOISE)            ||
           IsFilterFound(pList, pLen, MFX_EXTBUFF_VPP_DETAIL)             ||
           IsFilterFound(pList, pLen, MFX_EXTBUFF_VPP_PROCAMP)            ||
           IsFilterFound(pList, pLen, MFX_EXTBUFF_VPP_SCENE_CHANGE)       ||
           IsFilterFound(pList, pLen, MFX_EXTBUFF_VPP_IMAGE_STABILIZATION) )
        {
            sts = GetWorstSts(sts, MFX_WRN_FILTER_SKIPPED);
            if(bCorrectionEnable)
            {
                std::vector <mfxU32> tmpZeroDoUseList; //zero
                SignalPlatformCapabilities(*par, tmpZeroDoUseList);
            }
        }
    }

#ifdef MFX_ENABLE_MCTF
    {
        // mctf cannot work with CC others than NV12; also interlace is not supported
        if (par->vpp.Out.FourCC != MFX_FOURCC_NV12 || par->vpp.Out.PicStruct != MFX_PICSTRUCT_PROGRESSIVE)
        {
            if (IsFilterFound(pList, pLen, MFX_EXTBUFF_VPP_MCTF))
            {
                sts = GetWorstSts(sts, MFX_ERR_UNSUPPORTED);
            }
        }

        if ((par->vpp.Out.FrameRateExtN * par->vpp.In.FrameRateExtD != par->vpp.Out.FrameRateExtD * par->vpp.In.FrameRateExtN) ||
            IsFilterFound(pList, pLen, MFX_EXTBUFF_VPP_FRAME_RATE_CONVERSION) || IsFilterFound(pList, pLen, MFX_EXTBUFF_VPP_COMPOSITE))
        {
            if (IsFilterFound(pList, pLen, MFX_EXTBUFF_VPP_MCTF))
            {
                bool bHasFrameDelay(true);
#ifdef MFX_ENABLE_MCTF_EXT
                // mctf cannot work properly with FRC if 1 or 2 frame-delay mode is enabled
                // if no MctfBuffer in ExtParam, it means MCTF is enabled via DoUse list
                mfxExtVppMctf * pMctfBuf = reinterpret_cast<mfxExtVppMctf *>(GetExtendedBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_VPP_MCTF));
                if (pMctfBuf)
                {
                    if (MCTF_TEMPORAL_MODE_SPATIAL == pMctfBuf->TemporalMode ||
                        MCTF_TEMPORAL_MODE_1REF == pMctfBuf->TemporalMode)
                        bHasFrameDelay = false;
                }
#endif
                if (bHasFrameDelay)
                    sts = GetWorstSts(sts, MFX_ERR_UNSUPPORTED);
            }
        }
    }
#endif

    return sts;
} // mfxStatus VideoVPPHW::ValidateParams(mfxVideoParam *par, mfxVppCaps *caps, VideoCORE *core, bool bCorrectionEnable = false)


//---------------------------------------------------------------------------------
// Check DI mode set by user, if not set use advanced as default (if supported)
// Returns: 0 - cannot set due to HW limitations or wrong range, 1 - BOB, 2 - ADI )
//---------------------------------------------------------------------------------
mfxI32 GetDeinterlaceMode( const mfxVideoParam& videoParam, const mfxVppCaps& caps )
{
    mfxI32 deinterlacingMode = 0;

    // look for user defined deinterlacing mode
    for (mfxU32 i = 0; i < videoParam.NumExtParam; i++)
    {
        if (videoParam.ExtParam[i] && videoParam.ExtParam[i]->BufferId == MFX_EXTBUFF_VPP_DEINTERLACING)
        {
            mfxExtVPPDeinterlacing* extDI = (mfxExtVPPDeinterlacing*) videoParam.ExtParam[i];
            if (extDI->Mode != MFX_DEINTERLACING_ADVANCED &&
#if defined(MFX_ENABLE_SCENE_CHANGE_DETECTION_VPP)
                extDI->Mode != MFX_DEINTERLACING_ADVANCED_SCD &&
#endif
                extDI->Mode != MFX_DEINTERLACING_ADVANCED_NOREF &&
                extDI->Mode != MFX_DEINTERLACING_BOB &&
                extDI->Mode != MFX_DEINTERLACING_FIELD_WEAVING)
            {
                return 0;
            }
            /* To check if driver support desired DI mode*/
            if ((MFX_DEINTERLACING_ADVANCED == extDI->Mode) && (caps.uAdvancedDI) )
                deinterlacingMode = extDI->Mode;
#if defined(MFX_ENABLE_SCENE_CHANGE_DETECTION_VPP)
            if ((MFX_DEINTERLACING_ADVANCED_SCD == extDI->Mode) && (caps.uAdvancedDI) )
                deinterlacingMode = extDI->Mode;
#endif
            if ((MFX_DEINTERLACING_ADVANCED_NOREF == extDI->Mode) && (caps.uAdvancedDI) )
                deinterlacingMode = extDI->Mode;

            if ((MFX_DEINTERLACING_BOB == extDI->Mode) && (caps.uSimpleDI) )
                deinterlacingMode = extDI->Mode;

            if ((MFX_DEINTERLACING_FIELD_WEAVING == extDI->Mode) && (caps.uFieldWeavingControl))
                deinterlacingMode = extDI->Mode;
            /**/
            break;
        }
    }
    /* IF there is no DI ext buffer */
    if (caps.uAdvancedDI)
    {
        if (0 == deinterlacingMode)
        {
            deinterlacingMode = MFX_DEINTERLACING_ADVANCED;
        }
    }
    else if (caps.uSimpleDI)
    {
        if (0 == deinterlacingMode)
        {
            deinterlacingMode = MFX_DEINTERLACING_BOB;
        }
    }
    else
    {
        deinterlacingMode = 0;
    }

    return deinterlacingMode;
}

/* functions to detect: does required to set background in composition or not */
template <class T> inline T max_local1(const T &x, const T &y) {return (((x) > (y)) ? (x) : (y)); }
template <class T> inline T min_local1(const T &x, const T &y) {return (((x) < (y)) ? (x) : (y)); }
template <class T> inline T mid(const T &x, const T &y, const T &z) {return (min_local1(max_local1((x), (y)), (z))); }

template <class T> class cRect {
public:
    T m_x1, m_y1;
    T m_x2, m_y2;
    int     m_frag_lvl;
    int     m_surf_idx;
    int     reserved[2];
public:
    cRect(): m_x1(0), m_y1(0), m_x2(0), m_y2(0), m_frag_lvl(1), m_surf_idx(-1) {};
    cRect(T x1, T y1, T x2, T y2, int fragl, int s_id): m_x1(x1), m_y1(y1), m_x2(x2), m_y2(y2), m_frag_lvl(fragl), m_surf_idx(s_id) {};

    inline bool overlap(cRect &B) const {
        return !((m_x2 <= B.m_x1) || (m_x1 >= B.m_x2) || (m_y2 <= B.m_y1) || (m_y1 >= B.m_y2) );
    };
    inline T area(void) const { return (m_x2-m_x1) * (m_y2-m_y1); };
};

template <class T> void fragment(const cRect<T> a, const cRect<T> &b, std::vector< cRect<T> > &stack) {
    T   xa = mid(a.m_x1, b.m_x1, a.m_x2);
    T   xb = mid(a.m_x1, b.m_x2, a.m_x2);
    T   ya = mid(a.m_y1, b.m_y1, a.m_y2);
    T   yb = mid(a.m_y1, b.m_y2, a.m_y2);
    int nidx = a.m_frag_lvl + 1;
    int s_id = a.m_surf_idx;

    if(a.m_x2 != xb)
        stack.push_back(cRect<T>(xb, ya, a.m_x2, yb, nidx, s_id));

    if(a.m_x1 != xa)
        stack.push_back(cRect<T>(a.m_x1, ya, xa, yb, nidx, s_id));

    if(a.m_y2 != yb)
        stack.push_back(cRect<T>(a.m_x1, yb, a.m_x2, a.m_y2, nidx, s_id));

    if(a.m_y1 != ya)
        stack.push_back(cRect<T>(a.m_x1, a.m_y1, a.m_x2, ya, nidx, s_id));
}

template <class T> void add_unique_fragments(const cRect<T> &r, std::vector< cRect<T> > &fragments)
{
    std::vector< cRect<T> > stack;
    int frag_cnt = (int)fragments.size();

    stack.reserve(128);
    stack.push_back(r);

    while(!stack.empty()) {
        cRect<T> &cr = stack.back();

        if(cr.m_frag_lvl == frag_cnt) {
            stack.pop_back();
            fragments.push_back(cr);
        } else {
            const cRect<T> &cf = fragments[cr.m_frag_lvl];
            if (cf.overlap(cr)) {
                stack.pop_back();
                fragment(cr, cf, stack);
            }
            else {
                cr.m_frag_lvl++;
            }
        }
    }
}

template <class T> T get_total_area(std::vector< cRect<T> > &rects) {
    std::vector< cRect<T> > fragments;
    size_t                     i;

    fragments.reserve(4*rects.size());
    fragments.push_back(rects[0]);

    for (i = 1; i < rects.size(); i++)
        add_unique_fragments(rects[i], fragments);

    T area = 0;
    for (i = 0; i < fragments.size(); i++)
        area += fragments[i].area();

    return area;
}

inline
mfxU64 make_back_color_yuv(mfxU16 bit_depth, mfxU16 Y, mfxU16 U, mfxU16 V)
{
    if (bit_depth<8)
            throw MfxHwVideoProcessing::VpUnsupportedError();

    mfxU64 const shift = bit_depth - 8;
    mfxU64 const max_val = (1 << bit_depth) - 1;

    return
        ((mfxU64) max_val << 48) |
        ((mfxU64) mfx::clamp<mfxI32>(Y, (16 << shift), (235 << shift)) << 32) |
        ((mfxU64) mfx::clamp<mfxI32>(U, (16 << shift), (240 << shift)) << 16) |
        ((mfxU64) mfx::clamp<mfxI32>(V, (16 << shift), (240 << shift)) <<  0);
}

inline
mfxU64 make_back_color_argb(mfxU16 bit_depth, mfxU16 R, mfxU16 G, mfxU16 B)
{
    if (bit_depth<8)
            throw MfxHwVideoProcessing::VpUnsupportedError();

    mfxU64 const max_val = (1 << bit_depth) - 1;

    return ((mfxU64) max_val << 48) |
           (mfx::clamp<mfxU64>(R, 0, max_val) << 32) |
           (mfx::clamp<mfxU64>(G, 0, max_val) << 16) |
           (mfx::clamp<mfxU64>(B, 0, max_val) << 0);
}

inline
mfxU64 make_def_back_color_yuv(mfxU16 bit_depth)
{
    if (bit_depth<8)
            throw MfxHwVideoProcessing::VpUnsupportedError();

    mfxU64 const shift = bit_depth - 8;
    mfxU16 const min_val = 16 << shift, max_val = 256 << shift;
    return make_back_color_yuv(bit_depth, min_val, max_val / 2, max_val / 2);
}

inline
mfxU64 make_def_back_color_argb(mfxU16 bit_depth)
{
    return make_back_color_argb(bit_depth, 0, 0, 0);
}

static
mfxU64 get_background_color(const mfxVideoParam & videoParam)
{
    for (mfxU32 i = 0; i < videoParam.NumExtParam; i++)
    {
        if (videoParam.ExtParam[i]->BufferId == MFX_EXTBUFF_VPP_COLORFILL)
        {
            mfxExtVPPColorFill *extCF = reinterpret_cast<mfxExtVPPColorFill *>(videoParam.ExtParam[i]);
            if (!IsOn(extCF->Enable))
                return 0;
        }
        if (videoParam.ExtParam[i]->BufferId == MFX_EXTBUFF_VPP_COMPOSITE)
        {
            mfxExtVPPComposite *extComp = (mfxExtVPPComposite *) videoParam.ExtParam[i];
            switch (videoParam.vpp.Out.FourCC) {
                case MFX_FOURCC_NV12:
                case MFX_FOURCC_YV12:
                case MFX_FOURCC_NV16:
                case MFX_FOURCC_YUY2:
                case MFX_FOURCC_AYUV:
#if defined(MFX_VA_LINUX)
                case MFX_FOURCC_UYVY:
#endif
                    return make_back_color_yuv(8, extComp->Y, extComp->U, extComp->V);
                case MFX_FOURCC_P010:
#if (MFX_VERSION >= 1027)
                case MFX_FOURCC_Y210:
                case MFX_FOURCC_Y410:
#endif
                case MFX_FOURCC_P210:
                    return make_back_color_yuv(10, extComp->Y, extComp->U, extComp->V);
                case MFX_FOURCC_RGB4:
                case MFX_FOURCC_BGR4:
                    return make_back_color_argb(8, extComp->R, extComp->G, extComp->B);
                case MFX_FOURCC_A2RGB10:
                    return make_back_color_argb(10, extComp->R, extComp->G, extComp->B);
#if (MFX_VERSION >= 1031)
                case MFX_FOURCC_P016:
                case MFX_FOURCC_Y216:
                case MFX_FOURCC_Y416:
                {
                    mfxU16 depth = videoParam.vpp.Out.BitDepthLuma ? videoParam.vpp.Out.BitDepthLuma : 12;
                    return make_back_color_yuv(depth, extComp->Y, extComp->U, extComp->V);
                }
#endif
                default:
                    break;
            }
        }
    }

    switch (videoParam.vpp.Out.FourCC)
    {
        case MFX_FOURCC_NV12:
        case MFX_FOURCC_YV12:
        case MFX_FOURCC_NV16:
        case MFX_FOURCC_YUY2:
        case MFX_FOURCC_AYUV:
#if defined(MFX_VA_LINUX)
        case MFX_FOURCC_UYVY:
#endif
            return make_def_back_color_yuv(8);
        case MFX_FOURCC_P010:
#if (MFX_VERSION >= 1027)
        case MFX_FOURCC_Y210:
        case MFX_FOURCC_Y410:
#endif
        case MFX_FOURCC_P210:
            return make_def_back_color_yuv(10);
        case MFX_FOURCC_RGB4:
        case MFX_FOURCC_BGR4:
#ifdef MFX_ENABLE_RGBP
        case MFX_FOURCC_RGBP:
#endif
            return make_def_back_color_argb(8);
        case MFX_FOURCC_A2RGB10:
            return make_def_back_color_argb(10);
#if (MFX_VERSION >= 1031)
        case MFX_FOURCC_P016:
        case MFX_FOURCC_Y216:
        case MFX_FOURCC_Y416:
            return make_def_back_color_yuv(videoParam.vpp.Out.BitDepthLuma ? videoParam.vpp.Out.BitDepthLuma : 12);
#endif
        default:
            break;
    }
    /* Default case */
    return make_def_back_color_argb(8);
}

//---------------------------------------------------------
// Do internal configuration
//---------------------------------------------------------
mfxStatus ConfigureExecuteParams(
    mfxVideoParam & videoParam, // [IN]
    mfxVppCaps & caps,          // [IN]
    mfxExecuteParams & executeParams, // [OUT]
    Config & config)                  // [OUT]
{
    //----------------------------------------------
    std::vector<mfxU32> pipelineList;
    mfxStatus sts = GetPipelineList( &videoParam, pipelineList, true );
    MFX_CHECK_STS(sts);

    mfxF64 inDNRatio = 0, outDNRatio = 0;
    bool bIsFilterSkipped = false;

    // default
    config.m_extConfig.frcRational[VPP_IN].FrameRateExtN = videoParam.vpp.In.FrameRateExtN;
    config.m_extConfig.frcRational[VPP_IN].FrameRateExtD = videoParam.vpp.In.FrameRateExtD;
    config.m_extConfig.frcRational[VPP_OUT].FrameRateExtN = videoParam.vpp.Out.FrameRateExtN;
    config.m_extConfig.frcRational[VPP_OUT].FrameRateExtD = videoParam.vpp.Out.FrameRateExtD;

    config.m_surfCount[VPP_IN]  = 1;
    config.m_surfCount[VPP_OUT] = 1;

    executeParams.iBackgroundColor = get_background_color(videoParam);

    // 16 Bit modes are not currently supported by MSDK
    if(videoParam.vpp.In.BitDepthLuma == 16 || videoParam.vpp.In.BitDepthChroma == 16 ||
        videoParam.vpp.Out.BitDepthLuma==16 || videoParam.vpp.Out.BitDepthChroma == 16)
        return MFX_ERR_INVALID_VIDEO_PARAM;

    //-----------------------------------------------------
    for (mfxU32 j = 0; j < pipelineList.size(); j += 1)
    {
        mfxU32 curFilterId = pipelineList[j];

        switch(curFilterId)
        {
            case MFX_EXTBUFF_VPP_DEINTERLACING:
            case MFX_EXTBUFF_VPP_DI:
            {
                /* this function also take into account is DI ext buffer present or does not */
                executeParams.iDeinterlacingAlgorithm = GetDeinterlaceMode( videoParam, caps );
                if (0 == executeParams.iDeinterlacingAlgorithm)
                {
                    /* Tragically case,
                     * Something wrong. User requested DI but MSDK can't do it */
                    return MFX_ERR_UNKNOWN;
                }
                if(MFX_DEINTERLACING_ADVANCED     == executeParams.iDeinterlacingAlgorithm
#if defined(MFX_ENABLE_SCENE_CHANGE_DETECTION_VPP)
                || MFX_DEINTERLACING_ADVANCED_SCD == executeParams.iDeinterlacingAlgorithm
#endif
                )
                {
                    // use motion adaptive ADI with reference frame (quality)
                    config.m_bRefFrameEnable = true;
                    config.m_extConfig.customRateData.fwdRefCount = 0;
                    config.m_extConfig.customRateData.bkwdRefCount = 1; /* ref frame */
                    config.m_extConfig.customRateData.inputFramesOrFieldPerCycle= 1;
                    config.m_extConfig.customRateData.outputIndexCountPerCycle  = 1;
                    config.m_surfCount[VPP_IN]  = std::max<mfxU16>(2, config.m_surfCount[VPP_IN]);
                    config.m_surfCount[VPP_OUT] = std::max<mfxU16>(1, config.m_surfCount[VPP_OUT]);
                    config.m_extConfig.mode = IS_REFERENCES;
                }
                else if (MFX_DEINTERLACING_ADVANCED_NOREF == executeParams.iDeinterlacingAlgorithm)
                {
                    executeParams.iDeinterlacingAlgorithm = MFX_DEINTERLACING_ADVANCED_NOREF;
                    config.m_bRefFrameEnable = false;
                    config.m_extConfig.customRateData.fwdRefCount = 0;
                    config.m_extConfig.customRateData.bkwdRefCount = 0; /* ref frame */
                    config.m_extConfig.customRateData.inputFramesOrFieldPerCycle= 1;
                    config.m_extConfig.customRateData.outputIndexCountPerCycle  = 1;
                    config.m_surfCount[VPP_IN]  = std::max<mfxU16>(2, config.m_surfCount[VPP_IN]);
                    config.m_surfCount[VPP_OUT] = std::max<mfxU16>(1, config.m_surfCount[VPP_OUT]);
                    config.m_extConfig.mode = 0;
                }
                else if (0 == executeParams.iDeinterlacingAlgorithm)
                {
                    bIsFilterSkipped = true;
                }


                break;
            }

            case MFX_EXTBUFF_VPP_DI_WEAVE:
            {
                /* this function also take into account is DI ext buffer present or does not */
                executeParams.iDeinterlacingAlgorithm = GetDeinterlaceMode( videoParam, caps );
                if (0 == executeParams.iDeinterlacingAlgorithm)
                {
                    /* Tragically case,
                     * Something wrong. User requested DI but MSDK can't do it */
                    return MFX_ERR_UNKNOWN;
                }

                config.m_bWeave = true;
                executeParams.bFieldWeaving = true;
                if(MFX_DEINTERLACING_FIELD_WEAVING == executeParams.iDeinterlacingAlgorithm)
                {
                    // use motion adaptive ADI with reference frame (quality)
                    config.m_bRefFrameEnable = true;
                    config.m_extConfig.customRateData.fwdRefCount  = 0;
                    config.m_extConfig.customRateData.bkwdRefCount = 1; /* ref frame */
                    config.m_extConfig.customRateData.inputFramesOrFieldPerCycle= 1;
                    config.m_extConfig.customRateData.outputIndexCountPerCycle  = 1;
                    config.m_surfCount[VPP_IN]  = std::max<mfxU16>(2, config.m_surfCount[VPP_IN]);
                    config.m_surfCount[VPP_OUT] = std::max<mfxU16>(1, config.m_surfCount[VPP_OUT]);
                    config.m_extConfig.mode = IS_REFERENCES;
                }
                else
                {
                    executeParams.iDeinterlacingAlgorithm = 0;
                    bIsFilterSkipped = true;
                }

                break;
            }

            case MFX_EXTBUFF_VPP_DI_30i60p:
            {
                /* this function also take into account is DI ext buffer present or does not */
                executeParams.iDeinterlacingAlgorithm = GetDeinterlaceMode( videoParam, caps );
                if (0 == executeParams.iDeinterlacingAlgorithm)
                {
                    /* Tragically case,
                     * Something wrong. User requested DI but MSDK can't do it */
                    return MFX_ERR_UNKNOWN;
                }

                config.m_bMode30i60pEnable = true;
                if(MFX_DEINTERLACING_ADVANCED     == executeParams.iDeinterlacingAlgorithm
#if defined(MFX_ENABLE_SCENE_CHANGE_DETECTION_VPP)
                || MFX_DEINTERLACING_ADVANCED_SCD == executeParams.iDeinterlacingAlgorithm
#endif
                )
                {
                    // use motion adaptive ADI with reference frame (quality)
                    config.m_bRefFrameEnable = true;
                    config.m_extConfig.customRateData.bkwdRefCount = 1;
                    config.m_surfCount[VPP_IN]  = std::max<mfxU16>(2, config.m_surfCount[VPP_IN]);
                    config.m_surfCount[VPP_OUT] = std::max<mfxU16>(2, config.m_surfCount[VPP_OUT]);
                    executeParams.bFMDEnable = true;
                    executeParams.bDeinterlace30i60p = true;
                }
                else if(MFX_DEINTERLACING_BOB == executeParams.iDeinterlacingAlgorithm)
                {
                    // use ADI with spatial info, no reference frame (speed)
                    config.m_bRefFrameEnable = false;
                    config.m_surfCount[VPP_IN]  = std::max<mfxU16>(1, config.m_surfCount[VPP_IN]);
                    config.m_surfCount[VPP_OUT] = std::max<mfxU16>(2, config.m_surfCount[VPP_OUT]);
                    executeParams.bFMDEnable = false;
                    executeParams.bDeinterlace30i60p = true;
                }
                else if (MFX_DEINTERLACING_ADVANCED_NOREF == executeParams.iDeinterlacingAlgorithm)
                {
                    // use ADI with spatial info, no reference frame (speed)
                    executeParams.iDeinterlacingAlgorithm = MFX_DEINTERLACING_ADVANCED_NOREF;
                    config.m_bRefFrameEnable = false;
                    config.m_surfCount[VPP_IN]  = std::max<mfxU16>(1, config.m_surfCount[VPP_IN]);
                    config.m_surfCount[VPP_OUT] = std::max<mfxU16>(2, config.m_surfCount[VPP_OUT]);
                    executeParams.bFMDEnable = false;
                    executeParams.bDeinterlace30i60p = true;
                }
                else
                {
                    executeParams.iDeinterlacingAlgorithm = 0;
                    bIsFilterSkipped = true;
                }

                break;
            }

            case MFX_EXTBUFF_VPP_DENOISE:
            {
                if (caps.uDenoiseFilter)
                {
                    executeParams.bDenoiseAutoAdjust = TRUE;
                    // set denoise settings
                    for (mfxU32 i = 0; i < videoParam.NumExtParam; i++)
                    {
                        if (videoParam.ExtParam[i]->BufferId == MFX_EXTBUFF_VPP_DENOISE)
                        {
                            mfxExtVPPDenoise *extDenoise= (mfxExtVPPDenoise*) videoParam.ExtParam[i];

                            executeParams.denoiseFactor = MapDNFactor(extDenoise->DenoiseFactor );
                            executeParams.denoiseFactorOriginal = extDenoise->DenoiseFactor;
                            executeParams.bDenoiseAutoAdjust = FALSE;
                        }
                    }
                }
                else
                {
                    bIsFilterSkipped = true;
                }

                break;
            }
            case MFX_EXTBUFF_VPP_ROTATION:
            {
                if (caps.uRotation)
                {
                    for (mfxU32 i = 0; i < videoParam.NumExtParam; i++)
                    {
                        if (videoParam.ExtParam[i]->BufferId == MFX_EXTBUFF_VPP_ROTATION)
                        {
                            mfxExtVPPRotation *extRotate = (mfxExtVPPRotation*) videoParam.ExtParam[i];

                            if ( (MFX_ANGLE_0   != extRotate->Angle) &&
                                 (MFX_ANGLE_90  != extRotate->Angle) &&
                                 (MFX_ANGLE_180 != extRotate->Angle) &&
                                 (MFX_ANGLE_270 != extRotate->Angle) )
                                return MFX_ERR_INVALID_VIDEO_PARAM;

                            if ( ( MFX_PICSTRUCT_FIELD_TFF & videoParam.vpp.Out.PicStruct ) ||
                                 ( MFX_PICSTRUCT_FIELD_BFF & videoParam.vpp.Out.PicStruct ) )
                                return MFX_ERR_INVALID_VIDEO_PARAM;

                            executeParams.rotation = extRotate->Angle;
                        }
                    }
                }
                else
                {
                    bIsFilterSkipped = true;
                }

                break;
            }
            case MFX_EXTBUFF_VPP_SCALING:
            {
                if (caps.uScaling)
                {
                    for (mfxU32 i = 0; i < videoParam.NumExtParam; i++)
                    {
                        if (videoParam.ExtParam[i]->BufferId == MFX_EXTBUFF_VPP_SCALING)
                        {
                            mfxExtVPPScaling *extScaling = (mfxExtVPPScaling*) videoParam.ExtParam[i];
                            executeParams.scalingMode = extScaling->ScalingMode;
#if (MFX_VERSION >= 1033)
                            executeParams.interpolationMethod = extScaling->InterpolationMethod;
#endif
                        }
                    }
                }
                else
                {
                    bIsFilterSkipped = true;
                }

                break;
            }
#if (MFX_VERSION >= 1025)
            case MFX_EXTBUFF_VPP_COLOR_CONVERSION:
            {
                if (caps.uChromaSiting )
                {
                    for (mfxU32 i = 0; i < videoParam.NumExtParam; i++)
                    {
                        if (videoParam.ExtParam[i]->BufferId == MFX_EXTBUFF_VPP_COLOR_CONVERSION)
                        {
                            mfxExtColorConversion *extCC = (mfxExtColorConversion*)videoParam.ExtParam[i];

                            switch (extCC->ChromaSiting)
                            {
                            case MFX_CHROMA_SITING_HORIZONTAL_LEFT | MFX_CHROMA_SITING_VERTICAL_TOP:
                            case MFX_CHROMA_SITING_HORIZONTAL_LEFT | MFX_CHROMA_SITING_VERTICAL_CENTER:
                            case MFX_CHROMA_SITING_HORIZONTAL_LEFT | MFX_CHROMA_SITING_VERTICAL_BOTTOM:
                            case MFX_CHROMA_SITING_HORIZONTAL_CENTER | MFX_CHROMA_SITING_VERTICAL_CENTER:
                            case MFX_CHROMA_SITING_HORIZONTAL_CENTER | MFX_CHROMA_SITING_VERTICAL_TOP:
                            case MFX_CHROMA_SITING_HORIZONTAL_CENTER | MFX_CHROMA_SITING_VERTICAL_BOTTOM:
                            case MFX_CHROMA_SITING_UNKNOWN:
                                // valid values
                                break;
                            default:
                                //bad combination of flags
                                return MFX_ERR_INVALID_VIDEO_PARAM;
                            }

                            executeParams.chromaSiting = extCC->ChromaSiting;
                        }
                    }
                }
                else
                {
                    bIsFilterSkipped = true;
                }

                break;
            }
#endif
            case MFX_EXTBUFF_VPP_MIRRORING:
            {
                if (caps.uMirroring)
                {
                    for (mfxU32 i = 0; i < videoParam.NumExtParam; i++)
                    {
                        if (videoParam.ExtParam[i]->BufferId == MFX_EXTBUFF_VPP_MIRRORING)
                        {
                            mfxExtVPPMirroring *extMirroring = (mfxExtVPPMirroring*) videoParam.ExtParam[i];

                            executeParams.mirroring = extMirroring->Type;

                            switch (videoParam.IOPattern)
                            {
                            case MFX_IOPATTERN_IN_VIDEO_MEMORY  | MFX_IOPATTERN_OUT_VIDEO_MEMORY:
                            case MFX_IOPATTERN_IN_VIDEO_MEMORY  | MFX_IOPATTERN_OUT_OPAQUE_MEMORY:
                            case MFX_IOPATTERN_IN_OPAQUE_MEMORY | MFX_IOPATTERN_OUT_VIDEO_MEMORY:
                            case MFX_IOPATTERN_IN_OPAQUE_MEMORY | MFX_IOPATTERN_OUT_OPAQUE_MEMORY:
                                executeParams.mirroringPosition = MIRROR_WO_EXEC;

                                break;
                            case MFX_IOPATTERN_IN_SYSTEM_MEMORY | MFX_IOPATTERN_OUT_VIDEO_MEMORY:
                            case MFX_IOPATTERN_IN_SYSTEM_MEMORY | MFX_IOPATTERN_OUT_OPAQUE_MEMORY:
                                executeParams.mirroringPosition = MIRROR_INPUT;
                                break;
                            case MFX_IOPATTERN_IN_VIDEO_MEMORY  | MFX_IOPATTERN_OUT_SYSTEM_MEMORY:
                            case MFX_IOPATTERN_IN_OPAQUE_MEMORY | MFX_IOPATTERN_OUT_SYSTEM_MEMORY:
                                executeParams.mirroringPosition = MIRROR_OUTPUT;
                                break;
                            case MFX_IOPATTERN_IN_SYSTEM_MEMORY | MFX_IOPATTERN_OUT_SYSTEM_MEMORY:
                                executeParams.mirroringPosition = (videoParam.vpp.In.Width * videoParam.vpp.In.Height < videoParam.vpp.Out.Width * videoParam.vpp.Out.Height)
                                                                ? MIRROR_INPUT
                                                                : MIRROR_OUTPUT;
                                break;
                            default:
                                return MFX_ERR_INVALID_VIDEO_PARAM;
                            }
                        }
                    }
                }
                else
                {
                    bIsFilterSkipped = true;
                }

                break;
            }
            case MFX_EXTBUFF_VPP_DETAIL:
            {
                if(caps.uDetailFilter)
                {
                    executeParams.bDetailAutoAdjust = TRUE;
                    for (mfxU32 i = 0; i < videoParam.NumExtParam; i++)
                    {
                        executeParams.detailFactor = 32;// default
                        if (videoParam.ExtParam[i]->BufferId == MFX_EXTBUFF_VPP_DETAIL)
                        {
                            mfxExtVPPDetail *extDetail = (mfxExtVPPDetail*) videoParam.ExtParam[i];
                            executeParams.detailFactor = MapDNFactor(extDetail->DetailFactor);
                            executeParams.detailFactorOriginal = extDetail->DetailFactor;
                        }
                        executeParams.bDetailAutoAdjust = FALSE;
                    }
                }
                else
                {
                    bIsFilterSkipped = true;
                }

                break;
            }

            case MFX_EXTBUFF_VPP_PROCAMP:
            {
                if(caps.uProcampFilter)
                {
                    for (mfxU32 i = 0; i < videoParam.NumExtParam; i++)
                    {
                        if (videoParam.ExtParam[i]->BufferId == MFX_EXTBUFF_VPP_PROCAMP)
                        {
                            mfxExtVPPProcAmp *extProcAmp = (mfxExtVPPProcAmp*) videoParam.ExtParam[i];

                            executeParams.Brightness = extProcAmp->Brightness;
                            executeParams.Saturation = extProcAmp->Saturation;
                            executeParams.Hue        = extProcAmp->Hue;
                            executeParams.Contrast   = extProcAmp->Contrast;
                            executeParams.bEnableProcAmp = true;
                        }
                    }
                }
                else
                {
                    bIsFilterSkipped = true;
                }

                break;
            }

            case MFX_EXTBUFF_VPP_RESIZE:
            {
                break;// make sense for SW_VPP only
            }

            case MFX_EXTBUFF_VPP_CSC:
            case MFX_EXTBUFF_VPP_CSC_OUT_RGB4:
            case MFX_EXTBUFF_VPP_CSC_OUT_A2RGB10:
            {
                break;// make sense for SW_VPP only
            }

            case MFX_EXTBUFF_VPP_RSHIFT_IN:
            case MFX_EXTBUFF_VPP_LSHIFT_OUT:
            {
                break;// make sense for SW_VPP only
            }

            case MFX_EXTBUFF_VPP_FRAME_RATE_CONVERSION:
            {
                //default mode
                config.m_extConfig.mode = FRC_ENABLED | FRC_STANDARD;
                config.m_extConfig.frcRational[VPP_IN].FrameRateExtN = videoParam.vpp.In.FrameRateExtN;
                config.m_extConfig.frcRational[VPP_IN].FrameRateExtD = videoParam.vpp.In.FrameRateExtD;
                config.m_extConfig.frcRational[VPP_OUT].FrameRateExtN = videoParam.vpp.Out.FrameRateExtN;
                config.m_extConfig.frcRational[VPP_OUT].FrameRateExtD = videoParam.vpp.Out.FrameRateExtD;

                config.m_surfCount[VPP_IN]  = std::max<mfxU16>(2, config.m_surfCount[VPP_IN]);
                config.m_surfCount[VPP_OUT] = std::max<mfxU16>(2, config.m_surfCount[VPP_OUT]);
                executeParams.frcModeOrig = static_cast<mfxU16>(GetMFXFrcMode(videoParam));


                inDNRatio = (mfxF64) videoParam.vpp.In.FrameRateExtD / videoParam.vpp.In.FrameRateExtN;
                outDNRatio = (mfxF64) videoParam.vpp.Out.FrameRateExtD / videoParam.vpp.Out.FrameRateExtN;

                break;
            }

            case MFX_EXTBUFF_VPP_ITC:
            {
                // simulating inverse telecine by simple frame skipping and bob deinterlacing
                config.m_extConfig.mode = FRC_ENABLED | FRC_STANDARD;

                config.m_extConfig.frcRational[VPP_IN].FrameRateExtN = videoParam.vpp.In.FrameRateExtN;
                config.m_extConfig.frcRational[VPP_IN].FrameRateExtD = videoParam.vpp.In.FrameRateExtD;
                config.m_extConfig.frcRational[VPP_OUT].FrameRateExtN = videoParam.vpp.Out.FrameRateExtN;
                config.m_extConfig.frcRational[VPP_OUT].FrameRateExtD = videoParam.vpp.Out.FrameRateExtD;

                inDNRatio  = (mfxF64) videoParam.vpp.In.FrameRateExtD / videoParam.vpp.In.FrameRateExtN;
                outDNRatio = (mfxF64) videoParam.vpp.Out.FrameRateExtD / videoParam.vpp.Out.FrameRateExtN;

                executeParams.iDeinterlacingAlgorithm = MFX_DEINTERLACING_BOB;
                config.m_surfCount[VPP_IN]  = std::max<mfxU16>(2, config.m_surfCount[VPP_IN]);
                config.m_surfCount[VPP_OUT] = std::max<mfxU16>(2, config.m_surfCount[VPP_OUT]);

                break;
            }


            case MFX_EXTBUFF_VPP_COMPOSITE:
            {
                mfxU32 StreamCount = 0;
                for (mfxU32 i = 0; i < videoParam.NumExtParam; i++)
                {
                    if (videoParam.ExtParam[i]->BufferId == MFX_EXTBUFF_VPP_COMPOSITE)
                    {
                        mfxExtVPPComposite* extComp = (mfxExtVPPComposite*) videoParam.ExtParam[i];
                        StreamCount = extComp->NumInputStream;

                        if (executeParams.dstRects.empty())
                        {
                            executeParams.initialStreamNum = StreamCount;
                        }
                        else
                        {
                            MFX_CHECK(StreamCount <= executeParams.initialStreamNum, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
                        }
                        executeParams.dstRects.resize(StreamCount);

#if MFX_VERSION > 1023
                        executeParams.iTilesNum4Comp = extComp->NumTiles;
#endif

                        for (mfxU32 cnt = 0; cnt < StreamCount; ++cnt)
                        {
                            DstRect rec = {};
                            rec.DstX = extComp->InputStream[cnt].DstX;
                            rec.DstY = extComp->InputStream[cnt].DstY;
                            rec.DstW = extComp->InputStream[cnt].DstW;
                            rec.DstH = extComp->InputStream[cnt].DstH;
#if MFX_VERSION > 1023
                            rec.TileId = extComp->InputStream[cnt].TileId;
#endif
                            if ((videoParam.vpp.Out.Width < (rec.DstX + rec.DstW)) || (videoParam.vpp.Out.Height < (rec.DstY + rec.DstH)))
                                return MFX_ERR_INVALID_VIDEO_PARAM; // sub-stream is out of range
                            if (extComp->InputStream[cnt].GlobalAlphaEnable != 0)
                            {
                                rec.GlobalAlphaEnable = extComp->InputStream[cnt].GlobalAlphaEnable;
                                rec.GlobalAlpha = extComp->InputStream[cnt].GlobalAlpha;
                            }
                            if (extComp->InputStream[cnt].LumaKeyEnable !=0)
                            {
                                rec.LumaKeyEnable = extComp->InputStream[cnt].LumaKeyEnable;
                                rec.LumaKeyMin = extComp->InputStream[cnt].LumaKeyMin;
                                rec.LumaKeyMax = extComp->InputStream[cnt].LumaKeyMax;
                            }
                            if (extComp->InputStream[cnt].PixelAlphaEnable != 0)
                                rec.PixelAlphaEnable = 1;
                            executeParams.dstRects[cnt]= rec ;
                        }
                    }
                }

                config.m_surfCount[VPP_IN]  = std::max(mfxU16(StreamCount), config.m_surfCount[VPP_IN]);
                config.m_surfCount[VPP_OUT] = std::max(mfxU16(1),           config.m_surfCount[VPP_OUT]);

                config.m_extConfig.mode = COMPOSITE;
                config.m_extConfig.customRateData.bkwdRefCount = 0;
                config.m_extConfig.customRateData.fwdRefCount  = StreamCount-1; // count only secondary streams
                config.m_extConfig.customRateData.inputFramesOrFieldPerCycle= StreamCount;
                config.m_extConfig.customRateData.outputIndexCountPerCycle  = 1;

                config.m_multiBlt = false; // not applicable for Linux

                executeParams.bComposite = true;
                executeParams.bBackgroundRequired = true; /* by default background required */

                /* Let's check: Does background required or not?
                 * If tiling enabled VPP composition will skip background setting
                 * bBackgroundRequired flag ignored on ddi level if tiling enabled
                 * */
                if (0 == executeParams.iTilesNum4Comp)
                {
                    mfxU32 tCropW = videoParam.vpp.Out.CropW;
                    mfxU32 tCropH = videoParam.vpp.Out.CropH;
                    mfxU32 refCount = config.m_extConfig.customRateData.fwdRefCount;
                    cRect<mfxU32> t_rect;
                    std::vector< cRect<mfxU32> > rects;
                    rects.reserve(refCount);

                    for(mfxU32 i = 0; i < refCount+1; i++)
                    {
                            mfxI32 x1 = mid((mfxU32)0, executeParams.dstRects[i].DstX, tCropW);
                            mfxI32 y1 = mid((mfxU32)0, executeParams.dstRects[i].DstY, tCropH);
                            mfxI32 x2 = mid((mfxU32)0, executeParams.dstRects[i].DstX + executeParams.dstRects[i].DstW, tCropW);
                            mfxI32 y2 = mid((mfxU32)0, executeParams.dstRects[i].DstY + executeParams.dstRects[i].DstH, tCropW);

                            t_rect = cRect<mfxU32>(x1, y1, x2, y2, 0, i);
                            if (t_rect.area() > 0)
                                    rects.push_back(t_rect);
                    }

                    if(get_total_area(rects) == tCropW*tCropH)
                        executeParams.bBackgroundRequired = false;
                } // if (executeParams.iTilesNum4Comp > 0)

                break;
            }
            case MFX_EXTBUFF_VPP_FIELD_PROCESSING:
            {
                /* As this filter for now can be configured via DOUSE way */
                {
                    executeParams.iFieldProcessingMode = FROM_RUNTIME_EXTBUFF_FIELD_PROC;
                }
                for (mfxU32 jj = 0; jj < videoParam.NumExtParam; jj++)
                {
                    if (videoParam.ExtParam[jj]->BufferId == MFX_EXTBUFF_VPP_FIELD_PROCESSING)
                    {
                        mfxExtVPPFieldProcessing* extFP = (mfxExtVPPFieldProcessing*) videoParam.ExtParam[jj];

                        if (extFP->Mode == MFX_VPP_COPY_FRAME)
                        {
                            executeParams.iFieldProcessingMode = FRAME2FRAME;
                        }

                        if (extFP->Mode == MFX_VPP_COPY_FIELD)
                        {
                            if(extFP->InField ==  MFX_PICSTRUCT_FIELD_TFF)
                            {
                                executeParams.iFieldProcessingMode = (extFP->OutField == MFX_PICSTRUCT_FIELD_TFF) ? TFF2TFF : TFF2BFF;
                            }
                            else
                            {
                                executeParams.iFieldProcessingMode = (extFP->OutField == MFX_PICSTRUCT_FIELD_TFF) ? BFF2TFF : BFF2BFF;
                            }
                        }

                        // kernel uses "0"  as a "valid" data, but HW_VPP doesn't
                        // to prevent issue we increment here and decrement before kernel call
                        executeParams.iFieldProcessingMode++;
                    }
                }
                break;
            } //case MFX_EXTBUFF_VPP_FIELD_PROCESSING:

            case MFX_EXTBUFF_VPP_VIDEO_SIGNAL_INFO:
                for (mfxU32 jj = 0; jj < videoParam.NumExtParam; jj++)
                {
                    if (videoParam.ExtParam[jj]->BufferId == MFX_EXTBUFF_VPP_VIDEO_SIGNAL_INFO)
                    {
                        mfxExtVPPVideoSignalInfo* extVSI = (mfxExtVPPVideoSignalInfo*) videoParam.ExtParam[jj];
                        MFX_CHECK_NULL_PTR1(extVSI);

                        /* Check params */
                        if (extVSI->In.TransferMatrix != MFX_TRANSFERMATRIX_BT601 &&
                            extVSI->In.TransferMatrix != MFX_TRANSFERMATRIX_BT709 &&
                            extVSI->In.TransferMatrix != MFX_TRANSFERMATRIX_UNKNOWN)
                        {
                            return MFX_ERR_INVALID_VIDEO_PARAM;
                        }

                        if (extVSI->Out.TransferMatrix != MFX_TRANSFERMATRIX_BT601 &&
                            extVSI->Out.TransferMatrix != MFX_TRANSFERMATRIX_BT709 &&
                            extVSI->Out.TransferMatrix != MFX_TRANSFERMATRIX_UNKNOWN)
                        {
                            return MFX_ERR_INVALID_VIDEO_PARAM;
                        }

                        if (extVSI->In.NominalRange != MFX_NOMINALRANGE_0_255 &&
                            extVSI->In.NominalRange != MFX_NOMINALRANGE_16_235 &&
                            extVSI->In.NominalRange != MFX_NOMINALRANGE_UNKNOWN)
                        {
                            return MFX_ERR_INVALID_VIDEO_PARAM;
                        }

                        if (extVSI->Out.NominalRange != MFX_NOMINALRANGE_0_255 &&
                            extVSI->Out.NominalRange != MFX_NOMINALRANGE_16_235 &&
                            extVSI->Out.NominalRange != MFX_NOMINALRANGE_UNKNOWN)
                        {
                            return MFX_ERR_INVALID_VIDEO_PARAM;
                        }

                        /* Params look good */
                        executeParams.VideoSignalInfoIn.NominalRange    = extVSI->In.NominalRange;
                        executeParams.VideoSignalInfoIn.TransferMatrix  = extVSI->In.TransferMatrix;
                        executeParams.VideoSignalInfoIn.enabled         = true;

                        executeParams.VideoSignalInfoOut.NominalRange   = extVSI->Out.NominalRange;
                        executeParams.VideoSignalInfoOut.TransferMatrix = extVSI->Out.TransferMatrix;
                        executeParams.VideoSignalInfoOut.enabled        = true;
                        break;
                    }
                }

                break;

            case MFX_EXTBUFF_VPP_FIELD_WEAVING:
            {
                if (videoParam.vpp.In.PicStruct & MFX_PICSTRUCT_FIELD_SINGLE)
                {
                    if (videoParam.vpp.Out.PicStruct & MFX_PICSTRUCT_FIELD_TFF)
                    {
                        executeParams.iFieldProcessingMode = FIELD2TFF;
                    }
                    else if (videoParam.vpp.Out.PicStruct & MFX_PICSTRUCT_FIELD_BFF)
                    {
                        executeParams.iFieldProcessingMode = FIELD2BFF;
                    }
                    else
                    {
                        executeParams.iFieldProcessingMode = FROM_RUNTIME_PICSTRUCT;
                    }
                }

                config.m_bWeave = true;
                config.m_bRefFrameEnable = true;
                config.m_extConfig.customRateData.bkwdRefCount = 1;
                config.m_extConfig.customRateData.fwdRefCount  = 0;
                config.m_extConfig.customRateData.inputFramesOrFieldPerCycle = 1;
                config.m_extConfig.customRateData.outputIndexCountPerCycle   = 1;

                config.m_surfCount[VPP_IN]   = std::max<mfxU16>(2, config.m_surfCount[VPP_IN]);
                config.m_surfCount[VPP_OUT]  = std::max<mfxU16>(1, config.m_surfCount[VPP_OUT]);
                config.m_extConfig.mode = IS_REFERENCES;
                executeParams.bFieldWeaving = true;
                // kernel uses "0"  as a "valid" data, but HW_VPP doesn't.
                // To prevent an issue we increment here and decrement before kernel call.
                executeParams.iFieldProcessingMode++;
                break;
            }

            case MFX_EXTBUFF_VPP_FIELD_SPLITTING:
            {
                if (videoParam.vpp.Out.PicStruct & MFX_PICSTRUCT_FIELD_SINGLE)
                {
                    if (videoParam.vpp.In.PicStruct & MFX_PICSTRUCT_FIELD_TFF)
                    {
                        executeParams.iFieldProcessingMode = TFF2FIELD;
                    }
                    else if (videoParam.vpp.In.PicStruct & MFX_PICSTRUCT_FIELD_BFF)
                    {
                        executeParams.iFieldProcessingMode = BFF2FIELD;
                    }
                    else
                    {
                        executeParams.iFieldProcessingMode = FROM_RUNTIME_PICSTRUCT;
                    }
                }

                config.m_extConfig.customRateData.bkwdRefCount = 0;
                config.m_extConfig.customRateData.fwdRefCount  = 0;
                config.m_extConfig.customRateData.inputFramesOrFieldPerCycle = 1;
                config.m_extConfig.customRateData.outputIndexCountPerCycle   = 2;

                config.m_surfCount[VPP_IN]   = std::max<mfxU16>(1, config.m_surfCount[VPP_IN]);
                config.m_surfCount[VPP_OUT]  = std::max<mfxU16>(2, config.m_surfCount[VPP_OUT]);
                config.m_extConfig.mode = IS_REFERENCES;
                // kernel uses "0"  as a "valid" data, but HW_VPP doesn't.
                // To prevent an issue we increment here and decrement before kernel call.
                executeParams.iFieldProcessingMode++;
                break;
            }

#ifdef MFX_ENABLE_MCTF
            case MFX_EXTBUFF_VPP_MCTF:
            {
                executeParams.bEnableMctf = true;
                // need to analyze uMCTF?
                mfxU16 MCTF_requiredFrames_In(1), MCTF_requiredFrames_Out(1);
                for (mfxU32 i = 0; i < videoParam.NumExtParam; i++)
                {
                    if (videoParam.ExtParam[i]->BufferId == MFX_EXTBUFF_VPP_MCTF)
                    {
                        mfxExtVppMctf *extMCTF = (mfxExtVppMctf*)videoParam.ExtParam[i];
                        //  if number of min requred frames for MCTF is 2, but async-depth == 1, 
                        // should we increase number of input / output surfaces to prevent hangs?
                        executeParams.MctfFilterStrength = extMCTF->FilterStrength;
                        //switch below must work in case MFX_ENABLE_MCTF_EXT is not defined;
                        // thus constants w/o MFX_ prefix are used
#ifdef MFX_ENABLE_MCTF_EXT
                        executeParams.MctfOverlap = extMCTF->Overlap;
                        executeParams.MctfBitsPerPixelx100k = extMCTF->BitsPerPixelx100k;
                        executeParams.MctfDeblocking = extMCTF->Deblocking;
                        executeParams.MctfTemporalMode = extMCTF->TemporalMode;
                        executeParams.MctfMVPrecision = extMCTF->MVPrecision;

                        mfxU16 TemporalMode = extMCTF->TemporalMode;
                        if (MCTF_TEMPORAL_MODE_UNKNOWN == TemporalMode)
                            TemporalMode = CMC::DEFAULT_REFS;
                        switch (TemporalMode)
#else
                        switch (CMC::DEFAULT_REFS)
#endif
                        {
                        case MCTF_TEMPORAL_MODE_SPATIAL:
                        case MCTF_TEMPORAL_MODE_1REF:
                        {
                            MCTF_requiredFrames_In = 1;
                            MCTF_requiredFrames_Out = 1;
                            break;
                        }
                        case MCTF_TEMPORAL_MODE_2REF:
                        {
                            MCTF_requiredFrames_In = 2;
                            MCTF_requiredFrames_Out = 2;
                            break;
                        }

                        case MCTF_TEMPORAL_MODE_4REF:
                        {
                            MCTF_requiredFrames_In = 3;
                            MCTF_requiredFrames_Out = 3;
                            break;
                        }
                        default:
                            return MFX_ERR_UNDEFINED_BEHAVIOR;
                            break;

                        }
                        config.m_surfCount[VPP_IN]  = std::max<mfxU16>(MCTF_requiredFrames_In, config.m_surfCount[VPP_IN]);
                        config.m_surfCount[VPP_OUT] = std::max<mfxU16>(MCTF_requiredFrames_Out, config.m_surfCount[VPP_OUT]);
                        break;
                    }
                }
                break;
            }
#endif

            default:
            {
                // there is no such capabilities
                bIsFilterSkipped = true;
                break;
            }
        }//switch(curFilterId)
    } // for (mfxU32 i = 0; i < filtersNum; i += 1)
    //-----------------------------------------------------

    // disabling filters
    for (mfxU32 i = 0; i < videoParam.NumExtParam; i++)
    {
        if (MFX_EXTBUFF_VPP_DONOTUSE == videoParam.ExtParam[i]->BufferId)
        {
            mfxExtVPPDoNotUse* extDoNotUse = (mfxExtVPPDoNotUse*) videoParam.ExtParam[i];
            MFX_CHECK_NULL_PTR1(extDoNotUse);

            for (mfxU32 j = 0; j < extDoNotUse->NumAlg; j++)
            {
                mfxU32 bufferId = extDoNotUse->AlgList[j];
                if (MFX_EXTBUFF_VPP_DENOISE == bufferId)
                {
                    executeParams.bDenoiseAutoAdjust    = false;
                    executeParams.denoiseFactor         = 0;
                    executeParams.denoiseFactorOriginal = 0;
                }
                else if (MFX_EXTBUFF_VPP_SCENE_ANALYSIS == bufferId)
                {
                }
                else if (MFX_EXTBUFF_VPP_DETAIL == bufferId)
                {
                    executeParams.bDetailAutoAdjust    = false;
                    executeParams.detailFactor         = 0;
                    executeParams.detailFactorOriginal = 0;

                }
                else if (MFX_EXTBUFF_VPP_PROCAMP == bufferId)
                {
                    executeParams.bEnableProcAmp = false;
                    executeParams.Brightness     = 0;
                    executeParams.Contrast       = 0;
                    executeParams.Hue            = 0;
                    executeParams.Saturation     = 0;
                }
                else if (MFX_EXTBUFF_VIDEO_SIGNAL_INFO == bufferId)
                {
                    executeParams.VideoSignalInfoIn.NominalRange    = MFX_NOMINALRANGE_UNKNOWN;
                    executeParams.VideoSignalInfoIn.TransferMatrix  = MFX_TRANSFERMATRIX_UNKNOWN;
                    executeParams.VideoSignalInfoOut.NominalRange   = MFX_NOMINALRANGE_UNKNOWN;
                    executeParams.VideoSignalInfoOut.TransferMatrix = MFX_TRANSFERMATRIX_UNKNOWN;

                }
                else if (MFX_EXTBUFF_VPP_COMPOSITE == bufferId)
                {
                    executeParams.bComposite = false;
                    executeParams.dstRects.clear();
                }
                else if (MFX_EXTBUFF_VPP_FIELD_PROCESSING == bufferId)
                {
                    executeParams.iFieldProcessingMode = 0;
                }
                else if (MFX_EXTBUFF_VPP_ROTATION == bufferId)
                {
                    executeParams.rotation = MFX_ANGLE_0;
                }
                else if (MFX_EXTBUFF_VPP_SCALING == bufferId)
                {
                    executeParams.scalingMode = MFX_SCALING_MODE_DEFAULT;
                }
#if (MFX_VERSION >= 1025)
                else if (MFX_EXTBUFF_VPP_COLOR_CONVERSION == bufferId)
                {
                    executeParams.chromaSiting = MFX_CHROMA_SITING_UNKNOWN;
                }
#endif
                else if (MFX_EXTBUFF_VPP_MIRRORING == bufferId)
                {
                    executeParams.mirroring = MFX_MIRRORING_DISABLED;
                }
                else if (MFX_EXTBUFF_VPP_IMAGE_STABILIZATION == bufferId)
                {
                    executeParams.bImgStabilizationEnable = false;
                    executeParams.istabMode               = 0;
                }
                else if (MFX_EXTBUFF_VPP_VIDEO_SIGNAL_INFO == bufferId)
                {
                    executeParams.VideoSignalInfoIn.NominalRange    = MFX_NOMINALRANGE_UNKNOWN;
                    executeParams.VideoSignalInfoIn.TransferMatrix  = MFX_TRANSFERMATRIX_UNKNOWN;
                    executeParams.VideoSignalInfoIn.enabled         = false;

                    executeParams.VideoSignalInfoOut.NominalRange   = MFX_NOMINALRANGE_UNKNOWN;
                    executeParams.VideoSignalInfoOut.TransferMatrix = MFX_TRANSFERMATRIX_UNKNOWN;
                    executeParams.VideoSignalInfoOut.enabled        = false;

                }
#ifdef MFX_ENABLE_MCTF
                else if (MFX_EXTBUFF_VPP_MCTF == bufferId)
                {
                    executeParams.bEnableMctf = false;
                }
#endif
                else
                {
                    // filter cannot be disabled
                    return MFX_ERR_INVALID_VIDEO_PARAM;
                }
            }
        }
    }

    // Post Correction Params
    if(MFX_FRCALGM_DISTRIBUTED_TIMESTAMP == GetMFXFrcMode(videoParam))
    {
        config.m_extConfig.mode = FRC_ENABLED | FRC_DISTRIBUTED_TIMESTAMP;

        if (config.m_bMode30i60pEnable)
        {
            config.m_extConfig.mode = FRC_DISABLED | FRC_DISTRIBUTED_TIMESTAMP;
        }
    }

    if ( (0 == memcmp(&videoParam.vpp.In, &videoParam.vpp.Out, sizeof(mfxFrameInfo))) &&
         executeParams.IsDoNothing() )
    {
        config.m_bCopyPassThroughEnable = true;
    }
    else
    {
        config.m_bCopyPassThroughEnable = false;// after Reset() parameters may be changed,
                                            // flag should be disabled
    }

    if (inDNRatio == outDNRatio && !executeParams.bVarianceEnable && !executeParams.bComposite &&
            !(config.m_extConfig.mode == IS_REFERENCES) )
    {
        // work around
        config.m_extConfig.mode  = FRC_DISABLED;
        //config.m_bCopyPassThroughEnable = false;
    }

    if (true == executeParams.bComposite && 0 == executeParams.dstRects.size()) // composition was enabled via DO USE
        return MFX_ERR_INVALID_VIDEO_PARAM;

    // A2RGB10 input supported only to copy pass thru
    MFX_CHECK(!(!config.m_bCopyPassThroughEnable && videoParam.vpp.In.FourCC == MFX_FOURCC_A2RGB10), MFX_ERR_INVALID_VIDEO_PARAM);

    return (bIsFilterSkipped) ? MFX_WRN_FILTER_SKIPPED : MFX_ERR_NONE;

} // mfxStatus ConfigureExecuteParams(...)


//---------------------------------------------------------
// UTILS
//---------------------------------------------------------
VideoVPPHW::IOMode   VideoVPPHW::GetIOMode(
    mfxVideoParam *par,
    mfxFrameAllocRequest* opaqReq)
{
    IOMode mode = VideoVPPHW::ALL;

    switch (par->IOPattern)
    {
    case MFX_IOPATTERN_IN_SYSTEM_MEMORY | MFX_IOPATTERN_OUT_SYSTEM_MEMORY:

        mode = VideoVPPHW::SYS_TO_SYS;
        break;

    case MFX_IOPATTERN_IN_VIDEO_MEMORY | MFX_IOPATTERN_OUT_SYSTEM_MEMORY:

        mode = VideoVPPHW::D3D_TO_SYS;
        break;

    case MFX_IOPATTERN_IN_SYSTEM_MEMORY | MFX_IOPATTERN_OUT_VIDEO_MEMORY:

        mode = VideoVPPHW::SYS_TO_D3D;
        break;

    case MFX_IOPATTERN_IN_VIDEO_MEMORY | MFX_IOPATTERN_OUT_VIDEO_MEMORY:

        mode = VideoVPPHW::D3D_TO_D3D;
        break;

        // OPAQ support: we suggest that OPAQ means D3D for HW_VPP by default
    case MFX_IOPATTERN_IN_OPAQUE_MEMORY | MFX_IOPATTERN_OUT_SYSTEM_MEMORY:
        {
            mode = VideoVPPHW::D3D_TO_SYS;

            if( opaqReq[VPP_IN].Type & MFX_MEMTYPE_SYSTEM_MEMORY )
            {
                mode = VideoVPPHW::SYS_TO_SYS;
            }
            break;
        }

    case MFX_IOPATTERN_IN_OPAQUE_MEMORY | MFX_IOPATTERN_OUT_VIDEO_MEMORY:
        {
            mode = VideoVPPHW::D3D_TO_D3D;

            if( opaqReq[VPP_IN].Type & MFX_MEMTYPE_SYSTEM_MEMORY )
            {
                mode = VideoVPPHW::SYS_TO_D3D;
            }

            break;
        }

    case MFX_IOPATTERN_IN_OPAQUE_MEMORY | MFX_IOPATTERN_OUT_OPAQUE_MEMORY:
        {
            mode = VideoVPPHW::D3D_TO_D3D;

            if( (opaqReq[VPP_IN].Type & MFX_MEMTYPE_SYSTEM_MEMORY) && (opaqReq[VPP_OUT].Type & MFX_MEMTYPE_SYSTEM_MEMORY) )
            {
                mode = VideoVPPHW::SYS_TO_SYS;
            }
            else if( opaqReq[VPP_IN].Type & MFX_MEMTYPE_SYSTEM_MEMORY )
            {
                mode = VideoVPPHW::SYS_TO_D3D;
            }
            else if( opaqReq[VPP_OUT].Type & MFX_MEMTYPE_SYSTEM_MEMORY )
            {
                mode = VideoVPPHW::D3D_TO_SYS;
            }

            break;
        }

    case MFX_IOPATTERN_IN_SYSTEM_MEMORY | MFX_IOPATTERN_OUT_OPAQUE_MEMORY:
        {
            mode = VideoVPPHW::SYS_TO_D3D;

            if( opaqReq[VPP_OUT].Type & MFX_MEMTYPE_SYSTEM_MEMORY )
            {
                mode = VideoVPPHW::SYS_TO_SYS;
            }

            break;
        }

    case MFX_IOPATTERN_IN_VIDEO_MEMORY | MFX_IOPATTERN_OUT_OPAQUE_MEMORY:
        {
            mode = VideoVPPHW::D3D_TO_D3D;

            if( opaqReq[VPP_OUT].Type & MFX_MEMTYPE_SYSTEM_MEMORY )
            {
                mode = VideoVPPHW::D3D_TO_SYS;
            }

            break;
        }
    }// switch ioPattern

    return mode;

} // IOMode VideoVPPHW::GetIOMode(...)


mfxStatus CheckIOMode(mfxVideoParam *par, VideoVPPHW::IOMode mode)
{
    switch (mode)
    {
    case VideoVPPHW::D3D_TO_D3D:

        if (par->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY ||
            par->IOPattern & MFX_IOPATTERN_IN_SYSTEM_MEMORY)
            return MFX_ERR_UNSUPPORTED;
        break;

    case VideoVPPHW::D3D_TO_SYS:

        if (par->IOPattern & MFX_IOPATTERN_IN_SYSTEM_MEMORY ||
            par->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY)
            return MFX_ERR_UNSUPPORTED;
        break;

    case VideoVPPHW::SYS_TO_D3D:

        if (par->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY ||
            par->IOPattern & MFX_IOPATTERN_IN_VIDEO_MEMORY)
            return MFX_ERR_UNSUPPORTED;
        break;

    case VideoVPPHW::SYS_TO_SYS:

        if (par->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY ||
            par->IOPattern & MFX_IOPATTERN_IN_VIDEO_MEMORY)
            return MFX_ERR_UNSUPPORTED;
        break;

    case VideoVPPHW::ALL:
            return MFX_ERR_NONE;
        break;

    default:

        return MFX_ERR_UNSUPPORTED;
    }

    return MFX_ERR_NONE;

} // mfxStatus CheckIOMode(mfxVideoParam *par, IOMode mode)


mfxU32 GetMFXFrcMode(const mfxVideoParam & videoParam)
{
    for (mfxU32 i = 0; i < videoParam.NumExtParam; i++)
    {
        if (videoParam.ExtParam[i]->BufferId == MFX_EXTBUFF_VPP_FRAME_RATE_CONVERSION)
        {
            mfxExtVPPFrameRateConversion *extFrc = (mfxExtVPPFrameRateConversion *) videoParam.ExtParam[i];
            return extFrc->Algorithm;
        }
    }

    for (mfxU32 i = 0; i < videoParam.NumExtParam; i++)
    {
        if (videoParam.ExtParam[i]->BufferId == MFX_EXTBUFF_VPP_DOUSE)
        {
            mfxExtVPPDoUse *extDoUse = (mfxExtVPPDoUse *) videoParam.ExtParam[i];

            for (mfxU32 j = 0; j < extDoUse->NumAlg; j++)
            {
                if (extDoUse->AlgList[j] == MFX_EXTBUFF_VPP_FRAME_RATE_CONVERSION)
                {
                    return MFX_FRCALGM_PRESERVE_TIMESTAMP;
                }
            }
        }
    }

    return 0;//EMPTY

} //  mfxU32 GetMFXFrcMode(mfxVideoParam & videoParam)


mfxStatus SetMFXFrcMode(const mfxVideoParam & videoParam, mfxU32 mode)
{
    for (mfxU32 i = 0; i < videoParam.NumExtParam; i++)
    {
        if (videoParam.ExtParam[i]->BufferId == MFX_EXTBUFF_VPP_FRAME_RATE_CONVERSION)
        {
            mfxExtVPPFrameRateConversion *extFrc = (mfxExtVPPFrameRateConversion *) videoParam.ExtParam[i];
            extFrc->Algorithm = (mfxU16)mode;

            return MFX_ERR_NONE;
        }
    }

    return MFX_WRN_VALUE_NOT_CHANGED;

} // mfxStatus SetMFXFrcMode(const mfxVideoParam & videoParam, mfxU32 mode)


mfxStatus SetMFXISMode(const mfxVideoParam & videoParam, mfxU32 mode)
{
    for (mfxU32 i = 0; i < videoParam.NumExtParam; i++)
    {
        if (videoParam.ExtParam[i]->BufferId == MFX_EXTBUFF_VPP_IMAGE_STABILIZATION)
        {
            mfxExtVPPImageStab *extFrc = (mfxExtVPPImageStab *) videoParam.ExtParam[i];
            extFrc->Mode = (mfxU16)mode;

            return MFX_ERR_NONE;
        }
    }

    return MFX_WRN_VALUE_NOT_CHANGED;
} // mfxStatus SetMFXISMode(const mfxVideoParam & videoParam, mfxU32 mode)


mfxU32 GetDeinterlacingMode(const mfxVideoParam & videoParam)
{
    for (mfxU32 i = 0; i < videoParam.NumExtParam; i++)
    {
        if (videoParam.ExtParam[i]->BufferId == MFX_EXTBUFF_VPP_DEINTERLACING)
        {
            mfxExtVPPDeinterlacing *extDeinterlacing = (mfxExtVPPDeinterlacing *) videoParam.ExtParam[i];
            return extDeinterlacing->Mode;
        }
    }

    return 0;//EMPTY

} // mfxStatus GetDeinterlacingMode(const mfxVideoParam & videoParam, mfxU32 mode)


mfxStatus SetDeinterlacingMode(const mfxVideoParam & videoParam, mfxU32 mode)
{
    for (mfxU32 i = 0; i < videoParam.NumExtParam; i++)
    {
        if (videoParam.ExtParam[i]->BufferId == MFX_EXTBUFF_VPP_DEINTERLACING)
        {
            mfxExtVPPDeinterlacing *extDeinterlacing = (mfxExtVPPDeinterlacing *) videoParam.ExtParam[i];
            extDeinterlacing->Mode = (mfxU16)mode;

            return MFX_ERR_NONE;
        }
    }

    return MFX_WRN_VALUE_NOT_CHANGED;

} // mfxStatus SetDeinterlacingMode(const mfxVideoParam & videoParam, mfxU32 mode)


mfxStatus SetSignalInfo(const mfxVideoParam & videoParam, mfxU32 trMatrix, mfxU32 Range)
{
    for (mfxU32 i = 0; i < videoParam.NumExtParam; i++)
    {
        if (videoParam.ExtParam[i]->BufferId == MFX_EXTBUFF_VPP_VIDEO_SIGNAL_INFO)
        {
            mfxExtVPPVideoSignalInfo *extDeinterlacing = (mfxExtVPPVideoSignalInfo *) videoParam.ExtParam[i];

            extDeinterlacing->In.TransferMatrix  = (mfxU16)trMatrix;
            extDeinterlacing->In.NominalRange    = (mfxU16)Range;
            extDeinterlacing->Out.TransferMatrix = (mfxU16)trMatrix;
            extDeinterlacing->Out.NominalRange   = (mfxU16)Range;

            return MFX_ERR_NONE;
        }
    }

    return MFX_WRN_VALUE_NOT_CHANGED;

} // mfxStatus SetMFXFrcMode(const mfxVideoParam & videoParam, mfxU32 mode)


MfxFrameAllocResponse::MfxFrameAllocResponse()
    : m_core (0)
    , m_numFrameActualReturnedByAllocFrames(0)
{
    Zero(dynamic_cast<mfxFrameAllocResponse&>(*this));
} // MfxFrameAllocResponse::MfxFrameAllocResponse()


mfxStatus MfxFrameAllocResponse::Free( void )
{
    if (m_core == 0)
        return MFX_ERR_NONE;

    if (MFX_HW_D3D11  == m_core->GetVAType())
    {
        for (size_t i = 0; i < m_responseQueue.size(); i++)
        {
            m_core->FreeFrames(&m_responseQueue[i]);
        }

        m_responseQueue.clear();
    }
    else
    {
        if (mids)
        {
            NumFrameActual = m_numFrameActualReturnedByAllocFrames;
            m_core->FreeFrames(this);
        }
    }

    return MFX_ERR_NONE;

} // mfxStatus MfxFrameAllocResponse::Free( void )


MfxFrameAllocResponse::~MfxFrameAllocResponse()
{
    Free();

} // MfxFrameAllocResponse::~MfxFrameAllocResponse()


mfxStatus MfxFrameAllocResponse::Alloc(
    VideoCORE *            core,
    mfxFrameAllocRequest & req, bool isCopyReqiured)
{
    req.NumFrameSuggested = req.NumFrameMin; // no need in 2 different NumFrames

    {
        mfxStatus sts = core->AllocFrames(&req, this, isCopyReqiured);
        MFX_CHECK_STS(sts);
    }

    if (NumFrameActual < req.NumFrameMin)
        return MFX_ERR_MEMORY_ALLOC;

    m_core = core;
    m_numFrameActualReturnedByAllocFrames = NumFrameActual;
    NumFrameActual = req.NumFrameMin; // no need in redundant frames
    return MFX_ERR_NONE;
}

mfxStatus MfxFrameAllocResponse::Alloc(
    VideoCORE *            core,
    mfxFrameAllocRequest & req,
    mfxFrameSurface1 **    opaqSurf,
    mfxU32                 numOpaqSurf)
{
    req.NumFrameSuggested = req.NumFrameMin; // no need in 2 different NumFrames

    mfxStatus sts = core->AllocFrames(&req, this, opaqSurf, numOpaqSurf);
    MFX_CHECK_STS(sts);

    if (NumFrameActual < req.NumFrameMin)
        return MFX_ERR_MEMORY_ALLOC;

    m_core = core;
    m_numFrameActualReturnedByAllocFrames = NumFrameActual;
    NumFrameActual = req.NumFrameMin; // no need in redundant frames
    return MFX_ERR_NONE;
}


mfxStatus CopyFrameDataBothFields(
    VideoCORE *          core,
    mfxFrameData /*const*/ & dst,
    mfxFrameData /*const*/ & src,
    mfxFrameInfo const & info)
{
    dst.MemId = 0;
    src.MemId = 0;
    mfxFrameSurface1 surfSrc = { {0,}, info, src };
    mfxFrameSurface1 surfDst = { {0,}, info, dst };

    return core->DoFastCopyExtended(&surfDst, &surfSrc);

} // mfxStatus CopyFrameDataBothFields(


mfxStatus GetWorstSts(mfxStatus sts1, mfxStatus sts2)
{
    mfxStatus statuses[] = {MFX_ERR_UNSUPPORTED, MFX_ERR_INVALID_VIDEO_PARAM, MFX_WRN_PARTIAL_ACCELERATION, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM, MFX_WRN_FILTER_SKIPPED, MFX_ERR_NONE};
    std::vector<mfxStatus> stsPriority(statuses, statuses + sizeof(statuses) / sizeof(mfxStatus)); // first - the worst, last - the best

    std::vector<mfxStatus>::iterator it1 = std::find(stsPriority.begin(), stsPriority.end(), sts1);
    std::vector<mfxStatus>::iterator it2 = std::find(stsPriority.begin(), stsPriority.end(), sts2);

    if (stsPriority.end() == it1 || stsPriority.end() == it2)
        return MFX_ERR_UNKNOWN;

    return (it1 < it2) ? *it1 : *it2;
} // mfxStatus GetWorstSts(mfxStatus sts1, mfxStatus sts2)

#endif // MFX_ENABLE_VPP
/* EOF */
