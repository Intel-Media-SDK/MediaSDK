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

#ifndef __SAMPLE_VPP_ROI_H
#define __SAMPLE_VPP_ROI_H

#include "mfxvideo.h"

typedef enum
{
    ROI_FIX_TO_FIX = 0x0001,
    ROI_VAR_TO_FIX = 0x0002,
    ROI_FIX_TO_VAR = 0x0003,
    ROI_VAR_TO_VAR = 0x0004

} eROIMode;

typedef struct
{
    eROIMode mode;

    int srcSeed;

    int dstSeed;

} sROICheckParam;

/* ************************************************************************* */

class ROIGenerator
{
public:
    ROIGenerator( void );
    ~ROIGenerator( void );

    mfxStatus Init(  mfxU16  width, mfxU16  height, int seed);
    mfxStatus Close( void );

    // need to set specific ROI from usage model of generator
    mfxStatus  SetROI(mfxFrameInfo* pInfo);

    // return seed for external application
    mfxI32     GetSeed( void );

protected:

    mfxU16  m_width;
    mfxU16  m_height;

    mfxI32  m_seed;

};

#endif /* __SAMPLE_VPP_PTS_H*/