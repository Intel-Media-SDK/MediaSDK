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

#ifndef __SAMPLE_VPP_PTS_H
#define __SAMPLE_VPP_PTS_H

#include "mfxvideo.h"
#include <memory>
#include <list>

#include "sample_vpp_frc.h"
#include "sample_vpp_frc_adv.h"

/* ************************************************************************* */
class PTSMaker
{
public:
    PTSMaker();
    virtual ~PTSMaker(){};
    // isAdvancedMode - enable/disable FRC checking based on PTS
    mfxStatus Init(mfxVideoParam *par, mfxU32 asyncDeep, bool isAdvancedMode = false, bool isFrameCorrespond = false);
    // need to set current time stamp for input surface
    bool  SetPTS(mfxFrameSurface1 *pSurface);
    // need to check
    bool  CheckPTS(mfxFrameSurface1 *pSurface);
    // sometimes need to pts jumping
    void  JumpPTS();

protected:
    void    PrintDumpInfo();

    // FRC based on Init parameters
    bool    CheckBasicPTS(mfxFrameSurface1 *pSurface);
    // FRC based on pts
    bool    CheckAdvancedPTS(mfxFrameSurface1 *pSurface);

    std::auto_ptr<BaseFRCChecker>  m_pFRCChecker;

    mfxU32  m_FRateExtN_In;
    mfxU32  m_FRateExtD_In;
    mfxU32  m_FRateExtN_Out;
    mfxU32  m_FRateExtD_Out;

    // we can offset initial time stamp
    mfxF64  m_TimeOffset;

    // starting value for random offset
    mfxF64  m_CurrTime;

    // maximum difference from reference
    mfxF64  m_MaxDiff;
    mfxF64  m_CurrDiff;


    // frame counter for input frames
    mfxU32  m_NumFrame_In;
    // frame counter for output frames
    mfxU32  m_NumFrame_Out;

    bool    m_IsJump;

    // FRC based on PTS mode
    bool    m_bIsAdvancedMode;

    std::list<mfxU64> m_ptsList;

};

#endif /* __SAMPLE_VPP_PTS_H*/
