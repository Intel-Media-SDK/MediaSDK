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

#ifndef __SAMPLE_FEI_PRED_REPACKING_H__
#define __SAMPLE_FEI_PRED_REPACKING_H__

#include "encoding_task.h"

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

static mfxStatus repackPreenc2Enc(mfxExtFeiPreEncMV::mfxExtFeiPreEncMVMB *preencMVoutMB, mfxExtFeiEncMVPredictors::mfxExtFeiEncMVPredictorsMB *EncMVPredMB, mfxU32 NumMB, mfxI16 *tmpBuf)
{
    MSDK_ZERO_ARRAY(EncMVPredMB, NumMB);
    for (mfxU32 i = 0; i<NumMB; i++)
    {
        //only one ref is used for now
        for (int j = 0; j < 4; j++){
            EncMVPredMB[i].RefIdx[j].RefL0 = 0;
            EncMVPredMB[i].RefIdx[j].RefL1 = 0;
        }

        //use only first subblock component of MV
        for (int j = 0; j < 2; j++){
            EncMVPredMB[i].MV[0][j].x = get16Median(preencMVoutMB + i, tmpBuf, 0, j);
            EncMVPredMB[i].MV[0][j].y = get16Median(preencMVoutMB + i, tmpBuf, 1, j);
        }
    } // for(mfxU32 i=0; i<NumMBAlloc; i++)

    return MFX_ERR_NONE;
}

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

static mfxStatus GetBestSetsByDistortion(std::list<PreEncOutput>& preenc_output,
    BestMVset & BestSet,
    mfxU32 nPred[2], mfxU32 fieldId, mfxU32 MB_idx)
{
    mfxStatus sts = MFX_ERR_NONE;

    // clear previous data
    BestSet.Clear();

    mfxExtFeiPreEncMV*     mvs    = NULL;
    mfxExtFeiPreEncMBStat* mbdata = NULL;

    for (std::list<PreEncOutput>::iterator it = preenc_output.begin(); it != preenc_output.end(); ++it)
    {
        mvs = reinterpret_cast<mfxExtFeiPreEncMV*> ((*it).output_bufs->PB_bufs.out.getBufById(MFX_EXTBUFF_FEI_PREENC_MV, fieldId));
        MSDK_CHECK_POINTER(mvs, MFX_ERR_NULL_PTR);

        mbdata = reinterpret_cast<mfxExtFeiPreEncMBStat*> ((*it).output_bufs->PB_bufs.out.getBufById(MFX_EXTBUFF_FEI_PREENC_MB, fieldId));
        MSDK_CHECK_POINTER(mbdata, MFX_ERR_NULL_PTR);

        /* Store all necessary info about current reference MB: pointer to MVs; reference index; distortion*/
        BestSet.bestL0.push_back(MVP_elem(&mvs->MB[MB_idx], (*it).refIdx[fieldId][0], mbdata->MB[MB_idx].Inter[0].BestDistortion));

        if (nPred[1])
        {
            BestSet.bestL1.push_back(MVP_elem(&mvs->MB[MB_idx], (*it).refIdx[fieldId][1], mbdata->MB[MB_idx].Inter[1].BestDistortion));
        }
    }

    /* find nPred best predictors by distortion */
    std::sort(BestSet.bestL0.begin(), BestSet.bestL0.end(), compareDistortion);
    BestSet.bestL0.resize(nPred[0]);

    if (nPred[1])
    {
        std::sort(BestSet.bestL1.begin(), BestSet.bestL1.end(), compareDistortion);
        BestSet.bestL1.resize(nPred[1]);
    }

    return sts;
}

#endif // __SAMPLE_FEI_PRED_REPACKING_H__