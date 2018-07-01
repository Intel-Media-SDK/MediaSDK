/******************************************************************************\
Copyright (c) 2017, Intel Corporation
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

#ifndef __FEI_PREDICTORS_REPACKING_H__
#define __FEI_PREDICTORS_REPACKING_H__

#include "sample_hevc_fei_defs.h"
#include "mfxfeihevc.h"

class PredictorsRepaking
{
public:
    PredictorsRepaking();
    ~PredictorsRepaking() {}

    mfxStatus Init(const mfxVideoParam& videoParams, mfxU16 preencDSfactor, const mfxU16 numMvPredictors[2]);
    mfxStatus RepackPredictors(const HevcTask& task, mfxExtFeiHevcEncMVPredictors& mvp, mfxU16 nMvPredictors[2]);

    enum
    {
        PERFORMANCE = 0,
        QUALITY     = 1
    };

    inline void SetPerfomanceRepackingMode() { m_repakingMode = PERFORMANCE; }
    inline void SetQualityRepackingMode() { m_repakingMode = QUALITY; }

private:
    const mfxU8  m_max_fei_enc_mvp_num;// maximum number of predictors for encoder
    //variables
    mfxU8  m_repakingMode;         // repaking type: PERFORMANCE or QUALITY
    mfxU16 m_width;                // input surface width
    mfxU16 m_height;               // input surface height
    mfxU8  m_downsample_power2;    // power of 2 for down-sampling: 0..3
    mfxU16 m_widthCU_ds;           // width in CU (16x16) of ds surface
    mfxU16 m_heightCU_ds;          // height in CU (16x16) of ds surface
    mfxU16 m_widthCU_enc;          // width in CU (16x16) for encoder
    mfxU16 m_heightCU_enc;         // height in CU (16x16) for encoder
    mfxU16 m_maxNumMvPredictorsL0;
    mfxU16 m_maxNumMvPredictorsL1;

    //functions
    mfxStatus RepackPredictorsPerformance(const HevcTask& task, mfxExtFeiHevcEncMVPredictors& mvp, mfxU16 nMvPredictors[2]);
    mfxStatus RepackPredictorsQuality(const HevcTask& task, mfxExtFeiHevcEncMVPredictors& mvp, mfxU16 nMvPredictors[2]);
    mfxU8 ConvertDSratioPower2(mfxU8 DSfactor);

    DISALLOW_COPY_AND_ASSIGN(PredictorsRepaking);
};

#endif // #define __FEI_PREDICTORS_REPACKING_H__
