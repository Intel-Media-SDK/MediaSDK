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

#include "hevc_sw_dso.h"

#include <map>
#include <string>
#include <fstream>
#include <numeric>

using namespace BS_HEVC2;

mfxStatus HevcSwDso::Init()
{
    m_DisplayOrderSinceLastIDR =  0;
    m_previousMaxDisplayOrder  = -1;

    BSErr bsSts = m_parser.open(m_inPars.strDsoFile);

    if (bsSts != BS_ERR_NONE)
    {
        msdk_printf(MSDK_STRING("\nCan not open %s\n"), m_inPars.strDsoFile);
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    if (m_ctuCtrlPool.get())
    {
        std::list<CTUCtrlPool::Type> list;
        while (!m_ctuCtrlPool->empty())
        {
            auto buffer = m_ctuCtrlPool->GetBuffer();
            {
                AutoBufferLocker<mfxExtFeiHevcEncCtuCtrl> lock(*m_buffAlloc.get(), *buffer);

                std::for_each(buffer->Data, buffer->Data + buffer->Pitch * buffer->Height,
                              [](mfxFeiHevcEncCtuCtrl& ctrl) { memset(&ctrl, 0, sizeof (ctrl)); }
                             );
            }

            list.emplace_back(std::move(buffer));
        }
    }
    return MFX_ERR_NONE;
}

void DumpMVPs(mfxExtFeiHevcEncMVPredictors & mvp, mfxU32 encorder)
{
    std::string fname = "MVPdump_encorder_frame_" + std::to_string(encorder + 1) + ".bin";
    std::fstream out_file(fname, std::ios::out | std::ios::binary);
    out_file.write((char*) mvp.Data, sizeof(mfxFeiHevcEncMVPredictors) * mvp.Pitch * mvp.Height);
    out_file.close();
}


mfxStatus HevcSwDso::GetFrame(HevcTaskDSO & task)
{
    BSErr bs_sts = m_parser.parse_next_unit();

    if (bs_sts != BS_ERR_NONE)
    {
        msdk_printf(MSDK_STRING("\nERROR : HevcSwDso::GetFrame : m_parser.parse_next_unit failed with code %d\n"), bs_sts);
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    auto * hdr = (BS_HEVC2::NALU*) m_parser.get_header();

    FillFrameTask(hdr, task);

    if (!(task.m_frameType & MFX_FRAMETYPE_IDR || task.m_frameType & MFX_FRAMETYPE_I))
    {
        auto mvp = m_mvpPool->GetBuffer();
        {
            AutoBufferLocker<mfxExtFeiHevcEncMVPredictors> lock(*m_buffAlloc.get(), *mvp);

            FillMVP(hdr, *mvp, task.m_nMvPredictors);

            if (m_bDumpFinalMVPs)
            {
                DumpMVPs(*mvp, m_ProcessedFrames);
            }
        }

        task.m_mvp = std::move(mvp);

        if (m_ctuCtrlPool.get()) // "ForceTo" is optional
        {
            auto ctuCtrl = m_ctuCtrlPool->GetBuffer();
            {
                AutoBufferLocker<mfxExtFeiHevcEncCtuCtrl> lock(*m_buffAlloc.get(), *ctuCtrl);

                FillCtuControls(hdr, *ctuCtrl);
            }
            task.m_ctuCtrl = std::move(ctuCtrl);
        }
    }
    else
    {
        task.m_nMvPredictors[0] = task.m_nMvPredictors[1] = 0;
    }

    m_ProcessedFrames++;

    return MFX_ERR_NONE;
}

inline bool IsHEVCSlice(mfxU32 nut)
{
    return (nut <= 21) && ((nut < 10) || (nut > 15));
}

void HevcSwDso::FillFrameTask(const BS_HEVC2::NALU* header, HevcTaskDSO & task)
{
    for (auto pNALU = header; pNALU; pNALU = pNALU->next)
    {
        if (!IsHEVCSlice(pNALU->nal_unit_type))
            continue;

        if (!pNALU->slice)
            throw mfxError(MFX_ERR_NULL_PTR, "ERROR : HevcSwDso::FillFrameTask : pNALU->slice pointer is null");

        auto& slice = *pNALU->slice;

        task.m_frameType = 0;

        if (pNALU->nal_unit_type == NALU_TYPE::IDR_W_RADL || pNALU->nal_unit_type == NALU_TYPE::IDR_N_LP) // IDR, REF
        {
            task.m_frameType |= MFX_FRAMETYPE_IDR | MFX_FRAMETYPE_REF;

            m_DisplayOrderSinceLastIDR = m_previousMaxDisplayOrder + 1;
        }
        if (pNALU->nal_unit_type >= NALU_TYPE::BLA_W_LP && pNALU->nal_unit_type <= NALU_TYPE::CRA_NUT) // other IRAP -> REF
        {
            task.m_frameType |= MFX_FRAMETYPE_REF;
        }
        else if (pNALU->nal_unit_type <= NALU_TYPE::RASL_R && (pNALU->nal_unit_type & 1)) // !IRAP, REF
            task.m_frameType |= MFX_FRAMETYPE_REF;

        task.m_frameType |= (slice.type == SLICE_TYPE::I) ? MFX_FRAMETYPE_I :
                                ((slice.type == SLICE_TYPE::P) ? MFX_FRAMETYPE_P : MFX_FRAMETYPE_B);

        task.m_statData.POC = slice.POC;

        m_previousMaxDisplayOrder = std::max(m_previousMaxDisplayOrder, m_DisplayOrderSinceLastIDR + slice.POC);

        for (mfxU32 i = 0; i < slice.strps.NumDeltaPocs; i++)
            task.m_dpb.push_back(slice.DPB[i].POC);
        for (mfxU32 i = 0; i < slice.num_ref_idx_l0_active; i++)
            task.m_refListActive[0].push_back(slice.L0[i].POC);
        for (mfxU32 i = 0; i < slice.num_ref_idx_l1_active; i++)
            task.m_refListActive[1].push_back(slice.L1[i].POC);

        if (!slice.pps)
            throw mfxError(MFX_ERR_NULL_PTR, "ERROR : HevcSwDso::FillFrameTask : slice.pps pointer is null");

        task.m_statData.QP = slice.qp_delta + slice.pps->init_qp_minus26 + 26;

        break;
    }

    task.m_surf->Data.FrameOrder = task.m_frameOrder = task.m_statData.DisplayOrder = m_DisplayOrderSinceLastIDR + task.m_statData.POC;

    task.m_statData.FrameType = task.m_frameType;

    // Detect GPB frames
    if ((task.m_frameType & MFX_FRAMETYPE_B) &&
        std::none_of(std::begin(task.m_refListActive[1]), std::end(task.m_refListActive[1]), [&task](mfxI32 poc) { return poc > task.m_statData.POC; }))
    {
        task.m_isGPBFrame = true;
    }

    // Calculate some statistics for LA BRC
    if (m_bCalcBRCStat)
    {
        FillBRCParams(header, task);
    }
}

void CountRefPixels(const BS_HEVC2::CU & cu, HevcTaskDSO & task, mfxI32 base)
{
    for (auto pu = cu.Pu; pu; pu = pu->Next)
    {
        switch (pu->inter_pred_idc)
        {
        case BS_HEVC2::INTER_PRED::PRED_L0:
            task.m_statData.AddPxlCount(base + task.m_refListActive[0][pu->ref_idx_l0], pu->w*pu->h);
            break;
        case BS_HEVC2::INTER_PRED::PRED_L1:
            task.m_statData.AddPxlCount(base + task.m_refListActive[1][pu->ref_idx_l1], pu->w*pu->h);
            break;
        case BS_HEVC2::INTER_PRED::PRED_BI:
            task.m_statData.AddPxlCount(base + task.m_refListActive[0][pu->ref_idx_l0], (pu->w*pu->h) >> 1);
            task.m_statData.AddPxlCount(base + task.m_refListActive[1][pu->ref_idx_l1], (pu->w*pu->h) >> 1);
            break;
        default:
            throw mfxError(MFX_ERR_UNDEFINED_BEHAVIOR, "ERROR : CountRefPixels : invalid inter_pred_idc");
        }
    }
}

void HevcSwDso::FillBRCParams(const BS_HEVC2::NALU* header, HevcTaskDSO & task)
{
    // Convert GPB frame to P in case of LA BRC
    if (task.m_isGPBFrame)
    {
        task.m_statData.FrameType = MFX_FRAMETYPE_P;
    }

    task.m_statData.FrameSize = 0;

    task.m_statData.NPixelsInFrame = task.m_surf->Info.CropW * task.m_surf->Info.CropH;

    mfxU32 numPixelsIntra = 0;

    for (auto pNALU = header; pNALU; pNALU = pNALU->next)
    {
        task.m_statData.FrameSize += pNALU->NumBytesInNalUnit << 3;

         if (!IsHEVCSlice(pNALU->nal_unit_type))
             continue;

         if (!pNALU->slice)
             throw mfxError(MFX_ERR_NULL_PTR, "ERROR : HevcSwDso::FillBRCParams : pNALU->slice pointer is null");

         for (auto ctu = pNALU->slice->ctu; ctu; ctu = ctu->Next)
         {
             for (auto cu = ctu->Cu; cu; cu = cu->Next)
             {
                 if (m_bEstimateDistortion)
                 {
                     for (auto tu = cu->Tu; tu; tu = tu->Next)
                     {
                         if (!tu->tc_levels_luma)
                             continue;

                         switch (m_AlgorithmType)
                         {
                         case NNZ:
                             // Count Number of Non-Zero luma transform coefficients
                             task.m_statData.VisualDistortion += std::count_if(tu->tc_levels_luma, tu->tc_levels_luma + (1 << (tu->log2TrafoSize << 1)),
                                                                                    [](mfxI32 coeff) { return coeff != 0; });
                             break;
                         case SSC:
                             // Count Sum of Squared luma transform Coefficients
                             task.m_statData.VisualDistortion += std::accumulate(tu->tc_levels_luma, tu->tc_levels_luma + (1 << (tu->log2TrafoSize << 1)), mfxU64(0),
                                                                                    [](mfxU64 sum_sq, mfxI32 coeff) { return sum_sq + coeff*coeff; });
                             break;
                         default:
                             break;
                         }
                     }
                 }

                 CountRefPixels(*cu, task, m_DisplayOrderSinceLastIDR);

                 if (cu->PredMode == MODE_INTRA)
                 {
                     // Number of pixels in CU is (2^log2CbSize)^2 = 2^(2*log2CbSize)
                     numPixelsIntra += 1 << (cu->log2CbSize << 1);
                 }
             }
         }
    }

    if (task.m_statData.NPixelsInFrame)
    {
        task.m_statData.ShareIntra = mfxF64(numPixelsIntra) / task.m_statData.NPixelsInFrame;
    }

    task.m_statData.FillProp();
}

inline void PU2MVP(const PU & pu, mfxFeiHevcEncMVPredictors & mvpBlock, mfxU8 idx)
{
    if (PRED_L0 == pu.inter_pred_idc || PRED_BI == pu.inter_pred_idc)
    {
        mvpBlock.RefIdx[idx].RefL0 = pu.ref_idx_l0;
        mvpBlock.MV[idx][0].x = pu.MvLX[0][0];
        mvpBlock.MV[idx][0].y = pu.MvLX[0][1];
    }
    if (PRED_L1 == pu.inter_pred_idc || PRED_BI == pu.inter_pred_idc)
    {
        mvpBlock.RefIdx[idx].RefL1 = pu.ref_idx_l1;
        mvpBlock.MV[idx][1].x = pu.MvLX[1][0];
        mvpBlock.MV[idx][1].y = pu.MvLX[1][1];
    }
}

inline bool isIntersecting(const PU& pu, mfxU32 x, mfxU32 y, mfxU32 w, mfxU32 h)
{
    return !((pu.x >= x + w || x >= pu.x + pu.w) || // If one rectangle is on side of other
             (pu.y >= y + h || y >= pu.y + pu.h));  // If one rectangle is above other
}

class PUSizeComparator
{
public:
    bool operator()(const PU* pu1, const PU* pu2)
    {
        if (pu1 == nullptr || pu2 == nullptr)
        {
            throw std::string("ERROR: PUSizeComparator - null pointer reference");
        }
        return ((pu1->w * pu1->h) < (pu2->w * pu2->h));
    }
};

void HevcSwDso::FillMVP(const BS_HEVC2::NALU* header, mfxExtFeiHevcEncMVPredictors & mvps, mfxU32 nMvPredictors[2])
{
    std::for_each(mvps.Data, mvps.Data + mvps.Pitch * mvps.Height,
            [](mfxFeiHevcEncMVPredictors& mvp)
            {
                mvp.BlockSize = 0;
                mvp.RefIdx[0].RefL0 = mvp.RefIdx[0].RefL1 = 0xf;
                mvp.RefIdx[1].RefL0 = mvp.RefIdx[1].RefL1 = 0xf;
                mvp.RefIdx[2].RefL0 = mvp.RefIdx[2].RefL1 = 0xf;
                mvp.RefIdx[3].RefL0 = mvp.RefIdx[3].RefL1 = 0xf;
            }
         );

    if (m_inPars.DSOMVPBlockSize == 0)
    {
        return; // No DSO MVPs requested
    }

    bool isFirstSlice = true;
    for (auto pNALU = header; pNALU; pNALU = pNALU->next)
    {
        if (!IsHEVCSlice(pNALU->nal_unit_type))
            continue;

        if (!pNALU->slice)
            throw mfxError(MFX_ERR_NULL_PTR, "ERROR : HevcSwDso::FillMVP : pNALU->slice pointer is null");

        auto& slice = *pNALU->slice;

        mfxU32 ctuAddr = 0;
        mfxU32 lastProcessed16x16BlockIdx = 0;
        std::vector<PU*> pusFrom8x8CUs;        // For DSOBlockSize 1 (16x16), need to keep the PUs
                                               // from 8x8 CUs here to populate the target 16x16 block
                                               // with MVPs from PUs in descending order of size

        for (auto ctu = slice.ctu; ctu; ctu = ctu->Next, ctuAddr++)
        {
            std::map<mfxU32, mfxU8> usedBlockEntries; // serves to check how many MVP entries inside 16x16 are used. Used for handling CU 8x8

            for (auto cu = ctu->Cu; cu; cu = cu->Next)
            {
                if (cu->PredMode == MODE_INTRA)
                    continue;

                mfxU32 colIdx = cu->x >> 4;
                mfxU32 rowIdx = cu->y >> 4;

                mfxU32 baseBlockIdx =
                    ((colIdx >> 1) << 2)            // column offset;
                    + (rowIdx & ~1) * mvps.Pitch;   // offset for new line of 32x32 blocks layout;

                mfxU32 mvpBlockIdx =
                    baseBlockIdx                    // column offset;
                                                    // offset for new line of 32x32 blocks layout;
                    + (colIdx & 1)                  // zero or single offset depending on the number of comumn index;
                    + ((rowIdx & 1) << 1);          // zero or double offset depending on the number of row index,
                                                    // zero shift for top 16x16 blocks into 32x32 layout and double for bottom blocks;

                mfxFeiHevcEncMVPredictors & baseBlock = mvps.Data[baseBlockIdx];
                mfxFeiHevcEncMVPredictors & mvpBlock  = mvps.Data[mvpBlockIdx];

                if (m_inPars.DSOMVPBlockSize == 1) // Repacked MVPs will be per 16x16 block
                {
                    if (lastProcessed16x16BlockIdx != mvpBlockIdx)
                    {   // Finished processing the last 16x16 block; if previous 16x16 block
                        // had 8x8 CUs, need to write the corresponding MVPs to destination now
                        if (!pusFrom8x8CUs.empty())
                        {
                            std::sort(pusFrom8x8CUs.rbegin(), pusFrom8x8CUs.rend(), PUSizeComparator());
                            mfxU32 mvpsToFill = std::min(pusFrom8x8CUs.size(), (size_t)4);

                            for (mfxU32 mvpCounter = 0; mvpCounter < mvpsToFill; mvpCounter++)
                            {
                                PU& pu = *(pusFrom8x8CUs[mvpCounter]);
                                PU2MVP(pu, mvps.Data[lastProcessed16x16BlockIdx], mvpCounter);
                                mvps.Data[lastProcessed16x16BlockIdx].BlockSize = 1;
                            }

                            pusFrom8x8CUs.clear();
                        }

                        lastProcessed16x16BlockIdx = mvpBlockIdx;
                    }

                    constexpr mfxU32 targetSize = 16;

                    std::vector<PU*> puVec;
                    puVec.reserve(4); // 4 PUs in a CU max
                    for (auto pu = cu->Pu; pu; pu = pu->Next)
                    {
                        puVec.emplace_back(pu);
                    }
                    std::sort(puVec.rbegin(), puVec.rend(), PUSizeComparator()); // Sort source PUs in descending order of size

                    if (5 == cu->log2CbSize) // 32x32 - iterate over destination 16x16 blocks and intersect
                    {                        // them with source PUs; put 1 MV from each src intersected PU to dest
                        for (mfxU32 k = baseBlockIdx; k < baseBlockIdx + 4; ++k)
                        {
                            mfxU32 xpos = (k % 2);
                            mfxU32 ypos = (k % 4) / 2;

                            for (PU* pPu : puVec)
                            {
                                PU& pu = *pPu;
                                if (isIntersecting(pu, cu->x + xpos * targetSize, cu->y + ypos * targetSize, targetSize, targetSize))
                                {
                                    PU2MVP(pu, mvps.Data[k], usedBlockEntries[k]);
                                    mvps.Data[k].BlockSize = 1;
                                    ++usedBlockEntries[k];
                                }
                            }
                        }
                    }
                    else if (4 == cu->log2CbSize) // 16x16 - put MVs from each PU of the source CU into the target 16x16 block
                    {                             // starting from the largest PU
                        for (PU* pPu : puVec)
                        {
                            PU& pu = *pPu;
                            PU2MVP(pu, mvps.Data[mvpBlockIdx], usedBlockEntries[mvpBlockIdx]);
                            mvps.Data[mvpBlockIdx].BlockSize = 1;
                            ++usedBlockEntries[mvpBlockIdx];
                        }
                    }
                    else // 8x8 - just add the PUs into an auxiliary vector here, populating the
                    {    // destination 16x16 block occurs after the whole current 16x16 block has
                         // been processed
                        pusFrom8x8CUs.insert(pusFrom8x8CUs.end(), puVec.begin(), puVec.end());
                    }
                }
                else if (m_inPars.DSOMVPBlockSize == 2)
                {
                    // TODO: implement this
                    throw std::string("ERROR: DSOMVPBlockSize 2 not implemented yet");
                }
                else if (m_inPars.DSOMVPBlockSize == 7) // Repack MVPs as closely to the DSO MVs as possible
                {
                    if (5 == cu->log2CbSize) // 32x32 - MVs from each PU (up to 4 in total) will be written
                    {                        // into the "base" 16x16 MVP structure
                        mvpBlock.BlockSize = 2;

                        mfxU32 i = 0;
                        for (auto pu = cu->Pu; pu; pu = pu->Next, ++i)
                        {
                            PU2MVP(*pu, mvpBlock, i);
                        }
                    }
                    else if (4 == cu->log2CbSize) // 16x16
                    {
                        baseBlock.BlockSize = 1; // enable the 'base' 16x16 block
                        mvpBlock.BlockSize  = 1;

                        mfxU32 i = 0;
                        for (auto pu = cu->Pu; pu; pu = pu->Next, ++i)
                        {
                            PU2MVP(*pu, mvpBlock, i);
                        }
                    }
                    else // 8x8
                    {
                        baseBlock.BlockSize = 1;
                        mvpBlock.BlockSize  = 1;

                        auto pu = cu->Pu;
                        // Take only the first PU in 8x8 CU, ignore the other if any
                        if (pu)
                        {
                            auto & entryIdx = usedBlockEntries[mvpBlockIdx];

                            // Since we take only the first PU, entryIdx can't be bigger than 3 so MVP overflow can't happen
                            PU2MVP(*pu, mvpBlock, entryIdx);
                            entryIdx++;
                        }
                    }
                }
            }
        }

        if (isFirstSlice)
        {
            nMvPredictors[0] = slice.num_ref_idx_l0_active;
            nMvPredictors[1] = slice.num_ref_idx_l1_active;
            isFirstSlice = false;
        }
    }
}

void HevcSwDso::FillCtuControls(const BS_HEVC2::NALU* header, mfxExtFeiHevcEncCtuCtrl & ctuCtrls)
{
    if (!m_inPars.forceToIntra && !m_inPars.forceToInter)
        return;

    for (auto pNALU = header; pNALU; pNALU = pNALU->next)
    {
        if (!IsHEVCSlice(pNALU->nal_unit_type))
            continue;

        if (!pNALU->slice)
            throw mfxError(MFX_ERR_NULL_PTR, "ERROR : HevcSwDso::FillCtuControls : pNALU->slice pointer is null");

        auto& slice = *pNALU->slice;

        for (auto ctu = slice.ctu; ctu; ctu = ctu->Next)
        {
            mfxU32 nCU = 0, nInter = 0, nIntra = 0;

            for (auto cu = ctu->Cu; cu; cu = cu->Next)
            {
                // Count number of CUs of intra or inter/skip types
                switch (cu->PredMode)
                {
                case MODE_INTRA:
                    ++nIntra;
                    break;
                case MODE_INTER:
                case MODE_SKIP:
                    ++nInter;
                    break;
                }
                ++nCU;
            }

            mfxFeiHevcEncCtuCtrl & ctrl = ctuCtrls.Data[ctu->CtbAddrInRs];

            // Force to INTRA/INTER if all CUs have the same type
            ctrl.ForceToIntra = false;
            ctrl.ForceToInter = false;

            if (m_inPars.forceToIntra && nIntra == nCU)
            {
                ctrl.ForceToIntra = true;
            }
            else if (m_inPars.forceToInter && nInter == nCU)
            {
                ctrl.ForceToInter = true;
            }
        }
    }
}
