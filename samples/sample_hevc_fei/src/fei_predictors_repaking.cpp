/******************************************************************************\
Copyright (c) 2017-2018, Intel Corporation
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

#include "fei_predictors_repacking.h"
#include <algorithm>

const mfxU8 ZigzagOrder[16] = { 0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15 };

PredictorsRepaking::PredictorsRepaking() :
    m_max_fei_enc_mvp_num(4),
    m_repakingMode(PERFORMANCE),
    m_width(0),
    m_height(0),
    m_downsample_power2(0),
    m_widthCU_ds(0),
    m_heightCU_ds(0),
    m_widthCU_enc(0),
    m_heightCU_enc(0),
    m_maxNumMvPredictorsL0(0),
    m_maxNumMvPredictorsL1(0)
{}

mfxStatus PredictorsRepaking::Init(const mfxVideoParam& videoParams, mfxU16 preencDSfactor, const mfxU16 numMvPredictors[2])
{
    if (videoParams.mfx.FrameInfo.Width == 0 || videoParams.mfx.FrameInfo.Height == 0)
        return MFX_ERR_INVALID_VIDEO_PARAM;

    m_width = videoParams.mfx.FrameInfo.Width;
    m_height = videoParams.mfx.FrameInfo.Height;
    m_downsample_power2 = ConvertDSratioPower2(preencDSfactor);

    m_widthCU_ds   = (MSDK_ALIGN16((MSDK_ALIGN16(m_width) >> m_downsample_power2))) >> 4;
    m_heightCU_ds  = (MSDK_ALIGN16((MSDK_ALIGN16(m_height) >> m_downsample_power2))) >> 4;
    m_widthCU_enc  = (MSDK_ALIGN32(m_width)) >> 4;
    m_heightCU_enc = (MSDK_ALIGN32(m_height)) >> 4;

    m_maxNumMvPredictorsL0 = numMvPredictors[0];
    m_maxNumMvPredictorsL1 = numMvPredictors[1];

    return MFX_ERR_NONE;
}

mfxU8 PredictorsRepaking::ConvertDSratioPower2(mfxU8 downsample_ratio)
{
    switch (downsample_ratio)
    {
        case 1:
            return 0;
        case 2:
            return 1;
        case 4:
            return 2;
        case 8:
            return 3;
        default:
            return 0;
    }
}

mfxStatus PredictorsRepaking::RepackPredictors(const HevcTask& task, mfxExtFeiHevcEncMVPredictors& mvp, mfxU16 nMvPredictors[2])
{
    mfxStatus sts = MFX_ERR_NONE;

    switch (m_repakingMode)
    {
    case PERFORMANCE:
        sts = RepackPredictorsPerformance(task, mvp, nMvPredictors);
        break;
    case QUALITY:
        sts = RepackPredictorsQuality(task, mvp, nMvPredictors);
        break;
    default:
        return MFX_ERR_UNSUPPORTED;
    }

    return sts;
}

mfxStatus PredictorsRepaking::RepackPredictorsPerformance(const HevcTask& task, mfxExtFeiHevcEncMVPredictors& mvp, mfxU16 nMvPredictors[2])
{
    std::vector<mfxExtFeiPreEncMVExtended*> mvs_vec;
    std::vector<const RefIdxPair*>          refIdx_vec;

    mfxU8 numFinalL0Predictors = (std::min)(task.m_numRefActive[0], (mfxU8)m_maxNumMvPredictorsL0);
    mfxU8 numFinalL1Predictors = (std::min)(task.m_numRefActive[1], (mfxU8)m_maxNumMvPredictorsL1);
    mfxU8 numPredPairs = (std::min)(m_max_fei_enc_mvp_num, (std::max)(numFinalL0Predictors, numFinalL1Predictors));

    // I-frames, nothing to do
    if (numPredPairs == 0 || (task.m_frameType & MFX_FRAMETYPE_I))
        return MFX_ERR_NONE;

    mvs_vec.reserve(m_max_fei_enc_mvp_num);
    refIdx_vec.reserve(m_max_fei_enc_mvp_num);

    // PreENC parameters reading
    for (std::list<PreENCOutput>::const_iterator it = task.m_preEncOutput.begin(); it != task.m_preEncOutput.end(); ++it)
    {
        if (!it->m_mv)
            return MFX_ERR_UNDEFINED_BEHAVIOR;

        mvs_vec.push_back((*it).m_mv);
        refIdx_vec.push_back(&(*it).m_activeRefIdxPair);
    }

    // check that task has enough PreENC motion vectors dumps to create MVPredictors for Encode
    if (numPredPairs > mvs_vec.size())
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    if (m_widthCU_enc > mvp.Pitch || m_heightCU_enc > mvp.Height)
        MSDK_CHECK_STATUS(MFX_ERR_UNDEFINED_BEHAVIOR, "Invalid MVP buffer size");

    const mfxI16Pair zeroPair = { 0, 0 };

    // disable all MVP blocks at first
    std::for_each(mvp.Data, mvp.Data + mvp.Pitch * mvp.Height,
            [](mfxFeiHevcEncMVPredictors& block)
            {
                block.BlockSize = 0;
                block.RefIdx[0].RefL0 = block.RefIdx[0].RefL1 = 0xf;
                block.RefIdx[1].RefL0 = block.RefIdx[1].RefL1 = 0xf;
                block.RefIdx[2].RefL0 = block.RefIdx[2].RefL1 = 0xf;
                block.RefIdx[3].RefL0 = block.RefIdx[3].RefL1 = 0xf;
            }
         );

    // the main loop thru all blocks
    for (mfxU32 rowIdx = 0; rowIdx < m_heightCU_enc; ++rowIdx) // row index for full surface (raster-scan order)
    {
        for (mfxU32 colIdx = 0; colIdx < m_widthCU_enc; ++colIdx) // column index for full surface (raster-scan order)
        {
            // calculation of the input index for encoder after permutation from raster scan order index into 32x32 layout
            // HEVC encoder works with 32x32 layout
            mfxU32 permutEncIdx =
                ((colIdx >> 1) << 2)            // column offset;
                + (rowIdx & ~1) * m_widthCU_enc // offset for new line of 32x32 blocks layout;
                + (colIdx & 1)                  // zero or single offset depending on the number of comumn index;
                + ((rowIdx & 1) << 1);          // zero or double offset depending on the number of row index,
                                                // zero shift for top 16x16 blocks into 32x32 layout and double for bottom blocks;

            mfxFeiHevcEncMVPredictors& block = mvp.Data[permutEncIdx];

            // BlockSize is used only when mfxExtFeiHevcEncFrameCtrl::MVPredictor = 7
            // 0 - MV predictor is disabled
            // 1 - enabled per 16x16 block
            // 2 - enabled per 32x32 block (used only first 16x16 block data)
            block.BlockSize = 1; // Using finest granularity

            mfxU32 linearPreEncIdx = rowIdx * m_widthCU_ds + colIdx;
            for (mfxU32 j = 0; j < numPredPairs; ++j)
            {
                block.RefIdx[j].RefL0 = refIdx_vec[j]->RefL0;
                block.RefIdx[j].RefL1 = refIdx_vec[j]->RefL1;

                if (m_downsample_power2 == 0)// w/o VPP
                {
                    if (colIdx >= m_widthCU_ds || rowIdx >= m_heightCU_ds)
                    {
                        block.MV[j][0] = zeroPair;
                        block.MV[j][1] = zeroPair;
                    }
                    else
                    {
                        block.MV[j][0] = mvs_vec[j]->MB[linearPreEncIdx].MV[0][0];
                        block.MV[j][1] = mvs_vec[j]->MB[linearPreEncIdx].MV[0][1];
                    }
                }
                else
                {
                    mfxU32 preencCUIdx = 0; // index CU from PreENC output
                    mfxU32 rowMVIdx;        // row index for motion vector
                    mfxU32 colMVIdx;        // column index for motion vector
                    mfxU32 preencMVIdx = 0; // linear index for motion vector

                    switch (m_downsample_power2)
                    {
                    case 1:
                        preencCUIdx = (rowIdx >> 1) * m_widthCU_ds + (colIdx >> 1);
                        rowMVIdx = rowIdx & 1;
                        colMVIdx = colIdx & 1;
                        preencMVIdx = rowMVIdx * 8 + colMVIdx * 2;
                        break;
                    case 2:
                        preencCUIdx = (rowIdx >> 2) * m_widthCU_ds + (colIdx >> 2);
                        rowMVIdx = rowIdx & 3;
                        colMVIdx = colIdx & 3;
                        preencMVIdx = rowMVIdx * 4 + colMVIdx;
                        break;
                    case 3:
                        preencCUIdx = (rowIdx >> 3) * m_widthCU_ds + (colIdx >> 3);
                        rowMVIdx = rowIdx & 7;
                        colMVIdx = colIdx & 7;
                        preencMVIdx = rowMVIdx / 2 * 4 + colMVIdx / 2;
                        break;
                    default:
                        break;
                    }

                    block.MV[j][0] = mvs_vec[j]->MB[preencCUIdx].MV[ZigzagOrder[preencMVIdx]][0];
                    block.MV[j][1] = mvs_vec[j]->MB[preencCUIdx].MV[ZigzagOrder[preencMVIdx]][1];

                    block.MV[j][0].x <<= m_downsample_power2;
                    block.MV[j][0].y <<= m_downsample_power2;
                    block.MV[j][1].x <<= m_downsample_power2;
                    block.MV[j][1].y <<= m_downsample_power2;
                }
            }

            // Duplicate predictors to the first L0 reference in the first L1 MVP slot
            if (task.m_ldb)
            {
                assert(m_maxNumMvPredictorsL1 == 1);

                block.RefIdx[0].RefL1 = block.RefIdx[0].RefL0;
                block.MV[0][1] = block.MV[0][0];
                numFinalL1Predictors = 1;
            }
        }
    }

    nMvPredictors[0] = numFinalL0Predictors;
    nMvPredictors[1] = numFinalL1Predictors;

    return MFX_ERR_NONE;
}

void SelectFromMV(const mfxI16Pair(*mv)[2], mfxI32 count, mfxI16Pair(&res)[2]);

mfxStatus PredictorsRepaking::RepackPredictorsQuality(const HevcTask& task, mfxExtFeiHevcEncMVPredictors& mvp, mfxU16 nMvPredictors[2])
{
    std::vector<mfxExtFeiPreEncMVExtended*>     mvs_vec;
    std::vector<mfxExtFeiPreEncMBStatExtended*> mbs_vec;
    std::vector<const RefIdxPair*>              refIdx_vec;

    mfxU8 numFinalL0Predictors = (std::min)(task.m_numRefActive[0], (mfxU8)m_maxNumMvPredictorsL0);
    // Currently RepackPredictorsQuality() doesn't have logic to handle L1 predictors of GPB frames
    mfxU8 numFinalL1Predictors = (std::min)((mfxU8)(task.m_ldb ? 0 : task.m_numRefActive[1]), (mfxU8)m_maxNumMvPredictorsL1);
    mfxU8 numPredPairs = (std::min)(m_max_fei_enc_mvp_num, (std::max)(numFinalL0Predictors, numFinalL1Predictors));

    // I-frames, nothing to do
    if (numPredPairs == 0 || (task.m_frameType & MFX_FRAMETYPE_I))
        return MFX_ERR_NONE;

    mvs_vec.reserve(m_max_fei_enc_mvp_num);
    mbs_vec.reserve(m_max_fei_enc_mvp_num);
    refIdx_vec.reserve(m_max_fei_enc_mvp_num);

    // PreENC parameters reading
    for (std::list<PreENCOutput>::const_iterator it = task.m_preEncOutput.begin(); it != task.m_preEncOutput.end(); ++it)
    {
        if (!it->m_mv || !it->m_mb)
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        mvs_vec.push_back((*it).m_mv);
        mbs_vec.push_back((*it).m_mb);
        refIdx_vec.push_back(&(*it).m_activeRefIdxPair);
    }

    // check that task has enough PreENC motion vectors dumps to create MVPredictors for Encode
    if (numPredPairs > mvs_vec.size())
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    if (m_widthCU_enc > mvp.Pitch || m_heightCU_enc > mvp.Height)
        MSDK_CHECK_STATUS(MFX_ERR_UNDEFINED_BEHAVIOR, "Invalid MVP buffer size");

    const mfxI16Pair zeroPair = { 0, 0 };

    // disable all MVP blocks at first
    std::for_each(mvp.Data, mvp.Data + mvp.Pitch * mvp.Height,
            [](mfxFeiHevcEncMVPredictors& block)
            {
                block.BlockSize = 0;
                block.RefIdx[0].RefL0 = block.RefIdx[0].RefL1 = 0xf;
                block.RefIdx[1].RefL0 = block.RefIdx[1].RefL1 = 0xf;
                block.RefIdx[2].RefL0 = block.RefIdx[2].RefL1 = 0xf;
                block.RefIdx[3].RefL0 = block.RefIdx[3].RefL1 = 0xf;
            }
         );

    // the main loop thru all blocks
    for (mfxU32 rowIdx = 0; rowIdx < m_heightCU_enc; ++rowIdx) // row index for full surface (raster-scan order)
    {
        for (mfxU32 colIdx = 0; colIdx < m_widthCU_enc; ++colIdx) // column index for full surface (raster-scan order)
        {
            // intermediate arrays to be sorted by distortion
            mfxU8 ref[4][2];
            mfxI16Pair mv[4][2];
            mfxU16 distortion[4][2];

            // calculation of the input index for encoder after permutation from raster scan order index into 32x32 layout
            // HEVC encoder works with 32x32 layout
            mfxU32 permutEncIdx =
                ((colIdx >> 1) << 2)            // column offset;
                + (rowIdx & ~1) * m_widthCU_enc // offset for new line of 32x32 blocks layout;
                + (colIdx & 1)                  // zero or single offset depending on the number of comumn index;
                + ((rowIdx & 1) << 1);          // zero or double offset depending on the number of row index,
                                                // zero shift for top 16x16 blocks into 32x32 layout and double for bottom blocks;

            mfxFeiHevcEncMVPredictors& block = mvp.Data[permutEncIdx];

            // BlockSize is used only when mfxExtFeiHevcEncFrameCtrl::MVPredictor = 7
            // 0 - MV predictor disabled
            // 1 - enabled per 16x16 block
            // 2 - enabled per 32x32 block (used only first 16x16 block data)
            block.BlockSize = 1; // Using finest granularity

            mfxU32 linearPreEncIdx = rowIdx * m_widthCU_ds + colIdx;
            for (mfxU32 j = 0; j < numPredPairs; ++j)
            {
                ref[j][0] = refIdx_vec[j]->RefL0;
                ref[j][1] = refIdx_vec[j]->RefL1;

                if (m_downsample_power2 == 0)// w/o VPP
                {
                    if (colIdx >= m_widthCU_ds || rowIdx >= m_heightCU_ds) // TODO move check to the beginning of the loop
                    {
                        mv[j][0] = zeroPair;
                        mv[j][1] = zeroPair;
                    }
                    else
                    {
                        SelectFromMV(&mvs_vec[j]->MB[linearPreEncIdx].MV[0], 16, mv[j]);
                        distortion[j][0] = mbs_vec[j]->MB[linearPreEncIdx].Inter[0].BestDistortion;
                        distortion[j][1] = mbs_vec[j]->MB[linearPreEncIdx].Inter[1].BestDistortion;
                    }
                }
                else
                {
                    mfxU32 preencCUIdx = 0; // index CU from PreENC output
                    mfxU32 rowMVIdx;        // row index for motion vector
                    mfxU32 colMVIdx;        // column index for motion vector
                    mfxU32 preencMVIdx = 0; // linear index for motion vector

                    switch (m_downsample_power2)
                    {
                    case 1:
                        preencCUIdx = (rowIdx >> 1) * m_widthCU_ds + (colIdx >> 1);
                        rowMVIdx = rowIdx & 1;
                        colMVIdx = colIdx & 1;
                        preencMVIdx = rowMVIdx * 8 + colMVIdx * 4;
                        SelectFromMV(&mvs_vec[j]->MB[preencCUIdx].MV[preencMVIdx], 4, mv[j]);
                        break;
                    case 2:
                        preencCUIdx = (rowIdx >> 2) * m_widthCU_ds + (colIdx >> 2);
                        rowMVIdx = rowIdx & 3;
                        colMVIdx = colIdx & 3;
                        preencMVIdx = rowMVIdx * 4 + colMVIdx;
                        mv[j][0] = mvs_vec[j]->MB[preencCUIdx].MV[ZigzagOrder[preencMVIdx]][0];
                        mv[j][1] = mvs_vec[j]->MB[preencCUIdx].MV[ZigzagOrder[preencMVIdx]][1];
                        break;
                    case 3:
                        preencCUIdx = (rowIdx >> 3) * m_widthCU_ds + (colIdx >> 3);
                        rowMVIdx = rowIdx & 7;
                        colMVIdx = colIdx & 7;
                        preencMVIdx = rowMVIdx / 2 * 4 + colMVIdx / 2;
                        mv[j][0] = mvs_vec[j]->MB[preencCUIdx].MV[ZigzagOrder[preencMVIdx]][0];
                        mv[j][1] = mvs_vec[j]->MB[preencCUIdx].MV[ZigzagOrder[preencMVIdx]][1];
                        break;
                    default:
                        break;
                    }

                    mv[j][0].x <<= m_downsample_power2;
                    mv[j][0].y <<= m_downsample_power2;
                    mv[j][1].x <<= m_downsample_power2;
                    mv[j][1].y <<= m_downsample_power2;

                    distortion[j][0] = (j < numFinalL0Predictors) ? mbs_vec[j]->MB[preencCUIdx].Inter[0].BestDistortion : 0xffff;
                    distortion[j][1] = (j < numFinalL1Predictors) ? mbs_vec[j]->MB[preencCUIdx].Inter[1].BestDistortion : 0xffff;
                }
            }

            // sort predictors by ascending distortion
            if (numPredPairs < 2) // nothing to sort
            {
                block.MV[0][0] = mv[0][0];
                block.MV[0][1] = mv[0][1];
                block.RefIdx[0].RefL0 = ref[0][0];
                block.RefIdx[0].RefL1 = ref[0][1];
                continue;
            }

            // smaller idx to be first argument to be preferred if equal
            #define CMP_DIST(k,l) {                               \
                mfxU8 res0 = distortion[k][0] > distortion[l][0]; \
                mfxU8 res1 = distortion[k][1] > distortion[l][1]; \
                worse[k][0] += res0; worse[l][0] += res0 ^ 1;     \
                worse[k][1] += res1; worse[l][1] += res1 ^ 1;     \
            }

            // fill unused
            for (mfxU32 j = numPredPairs; j < 4; ++j)
            {
                distortion[j][1] = distortion[j][0] = 0xffff;
                ref[j][1] = ref[j][0] = 0xff;
                mv[j][0].y = mv[j][0].x = 0x8000;
                mv[j][1].y = mv[j][1].x = 0x8000;
            }

            mfxU8 worse[4][2] = { {0,} };
            CMP_DIST(0, 1); CMP_DIST(2, 3);
            CMP_DIST(0, 2); CMP_DIST(1, 3);
            CMP_DIST(0, 3); CMP_DIST(1, 2);
            // here 'worse' tells how many cases are better, so it is position in sorted array
            for (mfxU32 j = 0; j < 4; j++)
            {
                block.MV[worse[j][0]][0] = mv[j][0];
                block.MV[worse[j][1]][1] = mv[j][1];
                block.RefIdx[worse[j][0]].RefL0 = ref[j][0];
                block.RefIdx[worse[j][1]].RefL1 = ref[j][1];
            }
        }
    }

    nMvPredictors[0] = numFinalL0Predictors;
    nMvPredictors[1] = numFinalL1Predictors;

    return MFX_ERR_NONE;
}

// Selects best MV pair from set of consequent MV pairs
// Count is expected to be 4 or 16
// May be improved later
void SelectFromMV(const mfxI16Pair(* mv)[2], mfxI32 count, mfxI16Pair (&res)[2])
{
    for (int ref = 0; ref < 2; ref++)
    {
        mfxI32 found = 0, xsum = 0, ysum = 0;
        for (int i = 0; i < count; i++)
        {
            if (mv[i][ref].x == -0x8000) // ignore intra
                continue;
            found++;
            xsum += mv[i][ref].x;
            ysum += mv[i][ref].y;
        }
        if (!found)
            res[ref] = mv[0][ref]; // all MV are fill with 0x8000
        else {
            res[ref].x = xsum / found;
            res[ref].y = ysum / found;
        }
    }
}
