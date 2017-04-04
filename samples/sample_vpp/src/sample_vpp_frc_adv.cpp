/******************************************************************************\
Copyright (c) 2005-2017, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This sample was distributed or derived from the Intel's Media Samples package.
The original version of this sample may be obtained from https://software.intel.com/en-us/intel-media-server-studio
or https://software.intel.com/en-us/media-client-solutions-support.
\**********************************************************************************/

#include <math.h>
#include "vm/strings_defs.h"
#include "sample_vpp_frc_adv.h"
#include <algorithm>

static const mfxU32 MFX_TIME_STAMP_FREQUENCY = 90000; // will go to mfxdefs.h


bool FRCAdvancedChecker::IsTimeStampsNear( mfxU64 timeStampRef, mfxU64 timeStampTst,  mfxU64 eps)
{
    mfxU32 absDiff = abs( (mfxI32)(timeStampTst - (timeStampRef)) );
    if(  absDiff <= eps )
    {
        return true;
    }
    else
    {
        msdk_printf(MSDK_STRING("\n\nError in FRC Advanced algorithm. \n"));

        msdk_printf(MSDK_STRING("Output frame number is %d\n"), m_numOutputFrames-1);

        int iPTS_Ref = (int)timeStampRef;
        int iPTS_Tst = (int)timeStampTst;
        int iAbsDiff = (int)absDiff;
        int iEps     = (int)eps;

        msdk_printf(MSDK_STRING("Error: refTimeStamp, tstTimeStamp, Diff, Delta are: %d %d %u %d\n"),
            iPTS_Ref, iPTS_Tst, iAbsDiff, iEps);

        return false;
    }

} // bool IsTimeStampsNear( mfxU64 timeStampTst, mfxU64 timeStampRef,  mfxU64 eps)


FRCAdvancedChecker::FRCAdvancedChecker()
{
    m_minDeltaTime = 0;

    m_bIsSetTimeOffset = false;

    m_timeOffset = 0;
    m_expectedTimeStamp = 0;
    m_timeStampJump   = 0;
    m_numOutputFrames = 0;

    m_bReadyOutput = false;
    m_defferedInputTimeStamp = 0;

    memset(&m_videoParam, 0, sizeof(m_videoParam));
} // FRCAdvancedChecker::FRCAdvancedChecker()


mfxStatus FRCAdvancedChecker::Init(mfxVideoParam *par, mfxU32 asyncDeep)
{
    par;
    asyncDeep;

    m_videoParam = *par;

    m_minDeltaTime = std::min((__UINT64) (m_videoParam.vpp.In.FrameRateExtD * MFX_TIME_STAMP_FREQUENCY) / (2 * m_videoParam.vpp.In.FrameRateExtN),
        (__UINT64) (m_videoParam.vpp.Out.FrameRateExtD * MFX_TIME_STAMP_FREQUENCY) / (2 * m_videoParam.vpp.Out.FrameRateExtN));

    return MFX_ERR_NONE;

} // mfxStatus FRCAdvancedChecker::Init(mfxVideoParam *par, mfxU32 asyncDeep)


bool FRCAdvancedChecker::PutInputFrameAndCheck(mfxFrameSurface1* pSurface)
{
    if( pSurface )
    {
        m_ptsList.push_back( pSurface->Data.TimeStamp );
    }

    return true;

} // bool FRCAdvancedChecker::PutInputFrameAndCheck(mfxFrameSurface1* pSurface)


bool  FRCAdvancedChecker::PutOutputFrameAndCheck(mfxFrameSurface1* pSurface)
{
    bool res;

    if( NULL == pSurface )
    {
        return false;
    }

    mfxU64 timeStampTst =  pSurface->Data.TimeStamp;

    bool bRepeatAnalysis = false;
    do
    {

        //------------------------------------------------
        //           ReadyOutput
        //------------------------------------------------
        if( m_bReadyOutput )
        {
            m_expectedTimeStamp = GetExpectedPTS(m_numOutputFrames, m_timeOffset, m_timeStampJump);

            m_numOutputFrames++;

            res = IsTimeStampsNear( m_expectedTimeStamp, timeStampTst,  m_minDeltaTime);

            return res;
        }
        else
        {
            //------------------------------------------------
            //           standard processing
            //------------------------------------------------
            if( 0 == m_ptsList.size() )
            {
                if( m_numOutputFrames > 0 ) // last frame processing
                {
                    m_expectedTimeStamp = GetExpectedPTS(m_numOutputFrames, m_timeOffset, m_timeStampJump);

                    m_numOutputFrames++;

                    res = IsTimeStampsNear( m_expectedTimeStamp, timeStampTst, m_minDeltaTime);

                    return res;
                }
                else
                {
                    return false;//?
                }
            }

            mfxU64 inputTimeStamp = m_ptsList.front();
            m_ptsList.pop_front();

            if( false == m_bIsSetTimeOffset )
            {
                m_bIsSetTimeOffset = true;
                m_timeOffset = inputTimeStamp;
            }

            m_expectedTimeStamp = GetExpectedPTS(m_numOutputFrames, m_timeOffset, m_timeStampJump);

            mfxU32 timeStampDifference = abs((mfxI32)(inputTimeStamp - m_expectedTimeStamp));

            // process irregularity
            if (m_minDeltaTime > timeStampDifference)
            {
                inputTimeStamp = m_expectedTimeStamp;
            }

            if ( inputTimeStamp < m_expectedTimeStamp )
            {
                m_bReadyOutput     = false;

                // skip frame
                // request new one input surface
                //return MFX_ERR_MORE_DATA;
                bRepeatAnalysis = true;
            }
            else if ( inputTimeStamp == m_expectedTimeStamp ) // see above (minDelta)
            {
                m_bReadyOutput     = false;

                m_numOutputFrames++;

                res = IsTimeStampsNear( m_expectedTimeStamp, timeStampTst, m_minDeltaTime);

                return res;
            }
            else // inputTimeStampParam > ptr->expectedTimeStamp
            {
                m_numOutputFrames++;

                res = IsTimeStampsNear( m_expectedTimeStamp, timeStampTst,  m_minDeltaTime);

                return res;
            }
        }
    } while(bRepeatAnalysis);

    return false;

} // bool  FRCAdvancedChecker::PutOutputFrameAndCheck(mfxFrameSurface1* pSurface)


mfxU64  FRCAdvancedChecker::GetExpectedPTS( mfxU32 frameNumber, mfxU64 timeOffset, mfxU64 timeJump )
{
    mfxU64 expectedPTS = (((mfxU64)frameNumber * m_videoParam.vpp.Out.FrameRateExtD * MFX_TIME_STAMP_FREQUENCY) / m_videoParam.vpp.Out.FrameRateExtN +
        timeOffset + timeJump);

    return expectedPTS;

} // mfxU64  FRCAdvancedChecker::GetExpectedPTS( mfxU32 frameNumber, mfxU64 timeOffset, mfxU64 timeJump )

/* EOF */