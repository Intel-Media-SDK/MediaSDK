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

#ifndef __SAMPLE_VPP_FRC_H
#define __SAMPLE_VPP_FRC_H

#include "mfxvideo.h"
#include <stdio.h>

class BaseFRCChecker
{
public:
    //BaseFRCChecker();
    virtual ~BaseFRCChecker(){}

    virtual mfxStatus Init(mfxVideoParam *par, mfxU32 asyncDeep) = 0;

    // notify FRCChecker about one more input frame
    virtual bool  PutInputFrameAndCheck(mfxFrameSurface1* pSurface) = 0;

    // notify FRCChecker about one more output frame and check result
    virtual bool  PutOutputFrameAndCheck(mfxFrameSurface1* pSurface) = 0;
};


class FRCChecker : public BaseFRCChecker
{
public:
    FRCChecker();
    virtual ~FRCChecker(){};

    virtual mfxStatus Init(mfxVideoParam *par, mfxU32 asyncDeep);

    // notify FRCChecker about one more input frame
    virtual bool  PutInputFrameAndCheck(mfxFrameSurface1* pSurface);

    // notify FRCChecker about one more output frame and check result
    virtual bool  PutOutputFrameAndCheck(mfxFrameSurface1* pSurface);

protected:
    // calculate greatest common divisor
    mfxU32  CalculateGCD(mfxU32 val1, mfxU32 val2);
    // calculate period
    void    DefinePeriods();
    void    DefineEdges();

    bool    CheckSurfaceCorrespondance(){return true;}
    void    PrintDumpInfoAboutMomentError();
    void    PrintDumpInfoAboutAverageError();

    mfxU32  m_FRateExtN_In;
    mfxU32  m_FRateExtD_In;
    mfxU32  m_FRateExtN_Out;
    mfxU32  m_FRateExtD_Out;

    // analyzing periods
    mfxU32  m_FramePeriod_In;
    mfxU32  m_FramePeriod_Out;

    // admissible error on period in frames
    mfxU32  m_Error_In;
    mfxU32  m_Error_Out;

    // edges in times between input and output
    mfxU32  m_UpperEdge;
    mfxU32  m_BottomEdge;

    // frame counter in the period for input frames
    mfxU32  m_NumFrame_In;
    // frame counter in the period  for output frames
    mfxU32  m_NumFrame_Out;

    mfxU32  m_MomentError;
    mfxU64  m_AverageError;

    mfxU32  m_asyncDeep;

};

#endif /* __SAMPLE_VPP_PTS_H*/