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
#include "vm/time_defs.h"
#include "sample_vpp_pts.h"


#define MFX_TIME_STAMP_FREQUENCY 90000

mfxF64 MaxTimeDifference = 0.015; // 15 ms is an edge

PTSMaker::PTSMaker():m_pFRCChecker(0),
m_FRateExtN_In(0),
m_FRateExtD_In(1),
m_FRateExtN_Out(0),
m_FRateExtD_Out(1),
m_TimeOffset(0),
m_CurrTime(0),
m_MaxDiff(MaxTimeDifference),
m_CurrDiff(0),
m_NumFrame_In(0),
m_NumFrame_Out(0),
m_IsJump(false),
m_bIsAdvancedMode(false)
{
}

mfxStatus PTSMaker::Init(mfxVideoParam *par, mfxU32 asyncDeep, bool isAdvancedMode, bool isFrameCorrespond)
{
    if (!par->vpp.In.FrameRateExtD || !par->vpp.Out.FrameRateExtD)
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    m_FRateExtN_In = par->vpp.In.FrameRateExtN;
    m_FRateExtD_In = par->vpp.In.FrameRateExtD;

    m_FRateExtN_Out = par->vpp.Out.FrameRateExtN;
    m_FRateExtD_Out = par->vpp.Out.FrameRateExtD;

    if (isFrameCorrespond)
    {
        m_pFRCChecker.reset(new FRCChecker);
        m_pFRCChecker.get()->Init(par, asyncDeep);
    }

    m_bIsAdvancedMode = isAdvancedMode; // aya: since MSDK 3.0 m_bIsAdvancedMode - only for advanced FRC (PTS based alg)

    if (m_bIsAdvancedMode)
    {
        m_pFRCChecker.reset(new FRCAdvancedChecker);
        m_pFRCChecker.get()->Init(par, asyncDeep);
    }

    if (m_bIsAdvancedMode)
    {
        // offest is needed only for pts mode
        msdk_tick tick = msdk_time_get_tick();
        srand((unsigned int)(tick&0xFFFFFFFF));
        m_TimeOffset = rand();
    }
    else
    {
        m_TimeOffset = 0;
    }

    return MFX_ERR_NONE;
}

bool  PTSMaker::SetPTS(mfxFrameSurface1 *pSurface)
{
    mfxF64 ts;
    mfxF64 pts_noise = 0; // 10% of timing noise

    // -- should be replaced by more complicated distribution
    if (m_bIsAdvancedMode)
    {
        pts_noise = (mfxF64)0.1*m_FRateExtD_In/m_FRateExtN_In * rand()/RAND_MAX;
    }

    if (m_IsJump)
    {
        m_CurrTime = rand();
        m_IsJump = false;
    }

    ts = (mfxF64)m_NumFrame_In * m_FRateExtD_In/m_FRateExtN_In + m_TimeOffset + m_CurrTime + pts_noise;
    // -- end

    pSurface->Data.TimeStamp = (mfxU64)(ts * MFX_TIME_STAMP_FREQUENCY + .5);
    m_NumFrame_In++;
    m_ptsList.push_back(pSurface->Data.TimeStamp);

    if (m_pFRCChecker.get())
    {
        return m_pFRCChecker.get()->PutInputFrameAndCheck(pSurface);
    }

    return true;
}

bool  PTSMaker::CheckPTS(mfxFrameSurface1 *pSurface)
{
    return m_bIsAdvancedMode?CheckAdvancedPTS(pSurface):CheckBasicPTS(pSurface);
}

bool PTSMaker::CheckBasicPTS(mfxFrameSurface1 *pSurface)
{
    std::list<mfxU64>::iterator it;
    // -1 valid value
    if (-1 == static_cast<int>(pSurface->Data.TimeStamp))
        return true;

    mfxU64 ts = pSurface->Data.TimeStamp;
    if (0 == m_ptsList.size())
    {
        m_CurrDiff = -1;
        PrintDumpInfo();
        return false;
    }
    for (it = m_ptsList.begin(); it != m_ptsList.end(); it++)
    {
        if (ts == *it)
        {
            if (m_pFRCChecker.get())
                m_pFRCChecker.get()->PutOutputFrameAndCheck(pSurface);

            return true;
        }

    }
    m_CurrDiff = (mfxF64)(ts-m_ptsList.front())/MFX_TIME_STAMP_FREQUENCY;
    PrintDumpInfo();
    return false;
}
bool PTSMaker::CheckAdvancedPTS(mfxFrameSurface1 *pSurface)
{
    mfxF64 ts = (mfxF64)pSurface->Data.TimeStamp / MFX_TIME_STAMP_FREQUENCY;
    mfxF64 ref = (mfxF64)m_NumFrame_Out * m_FRateExtD_Out/m_FRateExtN_Out + m_TimeOffset + m_CurrTime;
    m_CurrDiff = fabs(ref - ts);

    if( !m_bIsAdvancedMode )
    {
        if (m_CurrDiff >  m_MaxDiff)
        {
            if (m_CurrTime)
            {
                // it can be right if pts jump occurs
                mfxF64 possible_error = (m_FRateExtN_In*m_FRateExtN_Out)/(m_FRateExtD_In*m_FRateExtD_Out);
                if (m_CurrDiff <=  m_MaxDiff + possible_error)
                {
                    m_NumFrame_Out++;
                    if (m_pFRCChecker.get())
                    {
                        return m_pFRCChecker.get()->PutOutputFrameAndCheck(pSurface);
                    }
                    return true;
                }
            }
            PrintDumpInfo();
            return false;
        }
        else
        {
            m_NumFrame_Out++;
            if (m_pFRCChecker.get())
            {
                return m_pFRCChecker.get()->PutOutputFrameAndCheck(pSurface);
            }
            return true;
        }
    }
    else
    {
        m_NumFrame_Out++;
        if (m_pFRCChecker.get())
        {
            return m_pFRCChecker.get()->PutOutputFrameAndCheck(pSurface);
        }
        return true;
    }
}

void  PTSMaker::JumpPTS()
{
    if (m_bIsAdvancedMode)
        m_IsJump = true;
}

void  PTSMaker::PrintDumpInfo()
{
    msdk_printf(MSDK_STRING("Error in PTS setting \n"));
    msdk_printf(MSDK_STRING("Input frame number is %d\n"), m_NumFrame_In);
    msdk_printf(MSDK_STRING("Output frame number is %d\n"), m_NumFrame_Out);
    msdk_printf(MSDK_STRING("Initial time offset is %f\n"), m_TimeOffset);
    msdk_printf(MSDK_STRING("PTS difference is %f\n"), m_CurrDiff);
}




/***************************************************************************/