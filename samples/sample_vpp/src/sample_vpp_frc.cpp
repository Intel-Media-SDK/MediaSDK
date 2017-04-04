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
#include "sample_vpp_frc.h"

#define MFX_TIME_STAMP_FREQUENCY 90000

static mfxI32 DI_DEEP = 2;

FRCChecker::FRCChecker():m_FRateExtN_In(0),
m_FRateExtD_In(1),
m_FRateExtN_Out(0),
m_FRateExtD_Out(1),
m_FramePeriod_In(0),
m_FramePeriod_Out(0),
m_Error_In(0),
m_Error_Out(0),
m_UpperEdge(0),
m_BottomEdge(0),
m_NumFrame_In(0),
m_NumFrame_Out(0),
m_MomentError(0),
m_AverageError(0),
m_asyncDeep(0)

{
}

mfxStatus FRCChecker::Init(mfxVideoParam *par, mfxU32 asyncDeep)
{
    if (!par->vpp.In.FrameRateExtD || !par->vpp.Out.FrameRateExtD)
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    m_FRateExtN_In = par->vpp.In.FrameRateExtN;
    m_FRateExtD_In = par->vpp.In.FrameRateExtD;

    m_FRateExtN_Out = par->vpp.Out.FrameRateExtN;
    m_FRateExtD_Out = par->vpp.Out.FrameRateExtD;

    m_asyncDeep = asyncDeep;

    DefinePeriods();
    DefineEdges();

    // 10% of output period
    m_Error_In = m_FramePeriod_In / 10;
    m_Error_Out = m_FramePeriod_Out /10;


    return MFX_ERR_NONE;
}

bool FRCChecker::PutInputFrameAndCheck(mfxFrameSurface1* pSurface)
{
    pSurface;

    // not enough frames for checking

    // We need to check only if fr in > fr out
    if (m_FramePeriod_Out > m_FramePeriod_In)
    {
        if(DI_DEEP > mfxI32(m_NumFrame_In - m_asyncDeep))
        {
            m_NumFrame_In++;
            return true;
        }
        // Check instantaneous error
        m_MomentError = m_NumFrame_Out / (m_NumFrame_In - m_asyncDeep);
        if (m_MomentError > m_UpperEdge || m_MomentError < m_BottomEdge)
        {
            PrintDumpInfoAboutMomentError();
            return false;
        }

    }
    else if (!((m_NumFrame_In - m_asyncDeep) % m_FramePeriod_In)) // Check periods (average) error
    {
        if(0 == m_NumFrame_Out)
        {
            m_NumFrame_In++;
            return true;
        }
        mfxU32 round_counter = (m_NumFrame_In - m_asyncDeep) / m_FramePeriod_In;
        m_AverageError = labs(m_NumFrame_Out - round_counter* m_FramePeriod_Out);
        if (m_AverageError > m_Error_Out)
        {
            PrintDumpInfoAboutAverageError();
            return false;
        }

    }
    m_NumFrame_In++;
    return true;
}

bool  FRCChecker::PutOutputFrameAndCheck(mfxFrameSurface1* pSurface)
{
    pSurface;


    if (m_FramePeriod_Out < m_FramePeriod_In)
    {
        if(0 == m_NumFrame_Out)
        {
            m_NumFrame_Out++;
            return true;
        }
        // Check instantaneous error
        m_MomentError = (m_NumFrame_In - m_asyncDeep) / m_NumFrame_Out;
        if (m_MomentError > m_UpperEdge || m_MomentError < m_BottomEdge)
        {
            PrintDumpInfoAboutMomentError();
            return false;
        }

    }
    else  if (!(m_NumFrame_Out % m_FramePeriod_Out)) // Check periods (average) error
    {
        // not enough frames for checking because of deinterlacing
        if(DI_DEEP > mfxI32(m_NumFrame_In - m_asyncDeep))
        {
            m_NumFrame_Out++;
            return true;
        }
        mfxU32 round_counter = (m_NumFrame_Out / m_FramePeriod_Out);
        m_AverageError = labs(m_NumFrame_In - m_asyncDeep - round_counter * m_FramePeriod_In);
        if (m_AverageError  > m_Error_In)
        {
            PrintDumpInfoAboutAverageError();
            return false;
        }

    }
    m_NumFrame_Out++;
    return true;

}

void FRCChecker::DefinePeriods()
{
    mfxU32 GCD;
    GCD = CalculateGCD(m_FRateExtN_In, m_FRateExtN_Out);
    m_FRateExtN_In /= GCD;
    m_FRateExtN_Out /= GCD;

    m_FramePeriod_In = m_FRateExtN_In * m_FRateExtD_Out;
    m_FramePeriod_Out = m_FRateExtN_Out * m_FRateExtD_In;

    GCD = CalculateGCD(m_FramePeriod_In, m_FramePeriod_Out);
    m_FramePeriod_In /= GCD;
    m_FramePeriod_Out /= GCD;

}

void FRCChecker::DefineEdges()
{
    m_BottomEdge = (m_FramePeriod_In > m_FramePeriod_Out)?m_FramePeriod_In / m_FramePeriod_Out:
        m_FramePeriod_Out / m_FramePeriod_In;
m_UpperEdge = m_BottomEdge + 1;
}


mfxU32 FRCChecker::CalculateGCD(mfxU32 val1, mfxU32 val2)
{
    mfxU32 shift;
    if (val1 == 0 || val2 == 0)
        return val1 | val2;

    // Let shift := lg K, where K is the greatest power of 2
    // dividing both val1 and val2.
    for (shift = 0; ((val1 | val2) & 1) == 0; ++shift)
    {
        val1 >>= 1;
        val2 >>= 1;
    }

    while ((val1 & 1) == 0)
        val1 >>= 1;

    // From here on, u is always odd.
    do
    {
        while ((val2 & 1) == 0)  // Loop X
            val2 >>= 1;

        // Now u and v are both odd, so diff(u, v) is even.
        // Let u = min(u, v), v = diff(u, v)/2.
        if (val1 < val2)
        {
            val2 -= val1;
        }
        else
        {
            mfxU32 diff = val1 - val2;
            val1 = val2;
            val2 = diff;
        }
        val2 >>= 1;
    } while (val2 != 0);

    return val1 << shift;
}

void  FRCChecker::PrintDumpInfoAboutMomentError()
{
    msdk_printf(MSDK_STRING("Error in FRC algorithm. Moment error \n"));
    msdk_printf(MSDK_STRING("Input frame number is %d\n"), m_NumFrame_In);
    msdk_printf(MSDK_STRING("Output frame number is %d\n"), m_NumFrame_Out);
    msdk_printf(MSDK_STRING("Upper edge, Bottom edge, Current value are: %d\t %d\t %d\n"), m_UpperEdge, m_BottomEdge, m_MomentError);
}

void  FRCChecker::PrintDumpInfoAboutAverageError()
{
    msdk_printf(MSDK_STRING("Error in FRC algorithm. Average error \n"));
    msdk_printf(MSDK_STRING("Input frame number is %d\n"), m_NumFrame_In);
    msdk_printf(MSDK_STRING("Output frame number is %d\n"), m_NumFrame_Out);
    msdk_printf(MSDK_STRING("Maximum error: input FRC, output FRC, Current value are: %d\t %d\t %llu\n"), m_Error_In, m_Error_Out, m_AverageError);
}

