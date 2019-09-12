/******************************************************************************\
Copyright (c) 2005-2019, Intel Corporation
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

#ifndef __SAMPLE_FEI_PRED_REPACKING_H__
#define __SAMPLE_FEI_PRED_REPACKING_H__

#include "encoding_task.h"

#ifndef MFX_VERSION
#error MFX_VERSION not defined
#endif

struct MVP_elem
{
    MVP_elem(mfxExtFeiPreEncMV::mfxExtFeiPreEncMVMB * MVMB = NULL, mfxU8 idx = 0xff, mfxU16 dist = 0xffff)
        : preenc_MVMB(MVMB)
        , refIdx(idx)
        , distortion(dist)
    {}

    mfxExtFeiPreEncMV::mfxExtFeiPreEncMVMB * preenc_MVMB;
    mfxU8  refIdx;
    mfxU16 distortion;
};

struct BestMVset
{
    BestMVset(iTask* eTask)
    {
        if (eTask)
        {
            bestL0.reserve((std::min)((std::max)(GetNumL0MVPs(*eTask, 0), GetNumL0MVPs(*eTask, 1)), MaxFeiEncMVPNum));
            bestL1.reserve((std::min)((std::max)(GetNumL1MVPs(*eTask, 0), GetNumL1MVPs(*eTask, 1)), MaxFeiEncMVPNum));
        }
        else
        {
            bestL0.reserve(MaxFeiEncMVPNum);
            bestL1.reserve(MaxFeiEncMVPNum);
        }
    }

    void Clear()
    {
        bestL0.clear();
        bestL1.clear();
    }

    std::vector<MVP_elem> bestL0;
    std::vector<MVP_elem> bestL1;
};

/*  PreEnc outputs 16 MVs per-MB, one of the way to construct predictor from this array
    is to extract median for x and y component

    preencMB - MB of motion vectors buffer
    tmpBuf   - temporary array, for inplace sorting
    xy       - indicates coordinate being processed [0-1] (0 - x coordinate, 1 - y coordinate)
    L0L1     - indicates reference list being processed [0-1] (0 - L0-list, 1 - L1-list)

    returned value - median of 16 element array
*/
inline mfxI16 get16Median(mfxExtFeiPreEncMV::mfxExtFeiPreEncMVMB* preencMB, mfxI16* tmpBuf, int xy, int L0L1)
{
    switch (xy){
    case 0:
        for (int k = 0; k < 16; k++){
            tmpBuf[k] = preencMB->MV[k][L0L1].x;
        }
        break;
    case 1:
        for (int k = 0; k < 16; k++){
            tmpBuf[k] = preencMB->MV[k][L0L1].y;
        }
        break;
    default:
        return 0;
    }

    std::sort(tmpBuf, tmpBuf + 16);
    return (tmpBuf[7] + tmpBuf[8]) / 2;
}

/*  Repacking of PreENC MVs with 4x downscale requires calculating 4-element median

    Input/output parameters are similair with get16Median

    offset - indicates position of 4 MVs which maps to current MB on full-resolution frame
    (other 12 components are correspondes to another MBs on full-resolution frame)
*/

inline mfxI16 get4Median(mfxExtFeiPreEncMV::mfxExtFeiPreEncMVMB* preencMB, mfxI16* tmpBuf, int xy, int L0L1, int offset)
{
    switch (xy){
    case 0:
        for (int k = 0; k < 4; k++){
            tmpBuf[k] = preencMB->MV[k + offset][L0L1].x;
        }
        break;
    case 1:
        for (int k = 0; k < 4; k++){
            tmpBuf[k] = preencMB->MV[k + offset][L0L1].y;
        }
        break;
    default:
        return 0;
    }

    std::sort(tmpBuf, tmpBuf + 4);
    return (tmpBuf[1] + tmpBuf[2]) / 2;
}


/* repackPreenc2Enc passes only one predictor (median of provided preenc MVs) because we dont have distortions to choose 4 best possible */

mfxStatus repackPreenc2Enc(mfxExtFeiPreEncMV::mfxExtFeiPreEncMVMB *preencMVoutMB, mfxExtFeiEncMVPredictors::mfxExtFeiEncMVPredictorsMB *EncMVPredMB, mfxU32 NumMB, mfxI16 *tmpBuf);

static inline bool compareDistortion(MVP_elem frst, MVP_elem scnd)
{
    return frst.distortion < scnd.distortion;
}

/*  PreENC may be called multiple times for reference frames and multiple MV, MBstat buffers are generated.
    Here PreENC results are sorted indipendently for L0 and L1 references in terms of distortion.
    This function is called per-MB.

    preenc_output - list of output PreENC buffers from multicall stage
    RepackingInfo - structure that holds information about resulting predictors
                    (it is shrinked to fit nPred size before return)
    nPred         - array of numbers predictors expected by next interface (ENC/ENCODE)
                    (nPred[0] - number of L0 predictors, nPred[1] - number of L1 predictors)
    fieldId       - id of field being processed (0 - first field, 1 - second field)
    MB_idx        - offset of MB being processed
*/

mfxStatus GetBestSetsByDistortion(std::list<PreEncOutput>& preenc_output,
    BestMVset & BestSet,
    mfxU32 nPred[2], mfxU32 fieldId, mfxU32 MB_idx);

#endif // __SAMPLE_FEI_PRED_REPACKING_H__