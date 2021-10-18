// Copyright (c) 2018-2019 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "hevc2_parser.h"
#include "hevc2_trace.h"

using namespace BS_HEVC2;
using namespace BsReader2;

#undef BS2_TRO
#define BS2_TRO\
 if (TraceOffset()) fprintf(GetLog(), "0x%016llX[%i]|%3u|%3u: ",\
     GetByteOffset(), GetBitOffset(), GetR(), GetV());

SDParser::SDParser(bool report_TC)
    : Reader()
    , CABAC((Reader&)*this)
    , report_TCLevels(report_TC)
    , m_pAllocator(nullptr)
{
    SetTraceLevel(TRACE_DEFAULT);
    SetEmulation(false);
}

bool SDParser::more_rbsp_data()
{
    Bs8u b[5];

    if (!NextBytes(b, 5))
        return !TrailingBits(true);

    if (   (b[0] == 0 && b[1] == 0 && b[2] == 1)
        || (b[0] == 0 && b[1] == 0 && b[2] == 0 && b[3] == 1))
        return false;

    if (   (b[1] == 0 && b[2] == 0 && b[3] == 1)
        || (b[1] == 0 && b[2] == 0 && b[3] == 0 && b[4] == 1))
        return !TrailingBits(true);

    return true;
}

CTU* SDParser::parseSSD(NALU& nalu, NALU* pColPic, Bs32u NumCtb)
{
    TLAuto tl(*this, TRACE_CTU);
    BS_MEM::AutoLock _cpLock(m_pAllocator, pColPic);
    auto& slice = *nalu.slice;
    auto& pps = *slice.pps;
    std::vector<Slice*> colSLices;

    if (NewPicture)
    {
        Bs32u PicSizeInMinTbY = PicSizeInMinCbsY
            * (1 << (MinCbLog2SizeY - MinTbLog2SizeY)) * (1 << (MinCbLog2SizeY - MinTbLog2SizeY));

        m_ctu.clear();
        m_cu.clear();
        m_pu.clear();
        m_tu.clear();

        TCLevels.reserve(1 << (2*MaxTbLog2SizeY));

        if (report_TCLevels)
        {
            m_TC_lvl.clear();
            m_TC_lvl.reserve(PicSizeInSamplesY);
        }
        m_ctu.reserve(PicSizeInCtbsY);
        m_cu.reserve(PicSizeInMinCbsY);
        m_pu.reserve(PicSizeInMinCbsY * 4);
        m_tu.reserve(PicSizeInMinTbY);
    }

    auto nCTU = m_ctu.size();
    auto nCU = m_cu.size();
    auto nPU = m_pu.size();
    auto nTU = m_tu.size();

    Bs16u CtbAddrInRs = slice.slice_segment_address;
    Bs16u CtbAddrInTs = CtbAddrRsToTs[CtbAddrInRs];
    Bs16u SliceAddrRs = (slice.dependent_slice_segment_flag) ? SliceAddrRsInTs[CtbAddrInTs - 1] : CtbAddrInRs;
    Bs16u xCtb = (CtbAddrInRs % PicWidthInCtbsY) << CtbLog2SizeY;
    Bs16u yCtb = (CtbAddrInRs / PicWidthInCtbsY) << CtbLog2SizeY;

    while (pColPic)
    {
        if (isSlice(*pColPic))
            colSLices.push_back(pColPic->slice);
        pColPic = pColPic->next;
    }

    if (!colSLices.empty())
    {
        ColPicSlices = &colSLices[0];
        NumColSlices = (Bs16u)colSLices.size();
    }

    SliceAddrRsInTs[CtbAddrInTs] = SliceAddrRs;

    if (   !slice.dependent_slice_segment_flag
        || TileId[CtbAddrInTs] != TileId[CtbAddrInTs-1])
    {
        InitCtx();
    }
    else if (pps.entropy_coding_sync_enabled_flag && (CtbAddrInRs % PicWidthInCtbsY) == 0)
    {
        if (AvailableZs(xCtb, yCtb, xCtb + CtbLog2SizeY, yCtb - CtbLog2SizeY))
            SyncWPP();
        else
            InitCtx();
    }
    else
    {
        SyncDS();
    }
    qPY_PREV = SliceQpY;

    InitADE();

    if (pps.entropy_coding_sync_enabled_flag && PicWidthInCtbsY == 1)
        StoreWPP();

    CTU* pCTU = Alloc<CTU>();

    for (;;)
    {
        auto& ctu = *pCTU;

        CtuInRs[CtbAddrInRs] = pCTU;

        BS2_SET(CtbAddrInRs, ctu.CtbAddrInRs);
        BS2_SET(CtbAddrInTs, ctu.CtbAddrInTs);

        if (slice.sao_luma_flag || slice.sao_chroma_flag)
            parseSAO(ctu, xCtb >> CtbLog2SizeY, yCtb >> CtbLog2SizeY);

        ctu.Cu = Alloc<CU>();
        ctu.Cu->x = xCtb;
        ctu.Cu->y = yCtb;
        parseCQT(*ctu.Cu, CtbLog2SizeY, 0);

        if (slice.Split && !--NumCtb)
            break;

        BS2_SET(EndOfSliceSegmentFlag(), ctu.end_of_slice_segment_flag);

        if (pps.entropy_coding_sync_enabled_flag && (CtbAddrInRs % PicWidthInCtbsY) == 1)
            StoreWPP();

        if (ctu.end_of_slice_segment_flag)
        {
            if (pps.dependent_slice_segments_enabled_flag)
                StoreDS();
            break;
        }

        pCTU->Next = Alloc<CTU>();
        pCTU = pCTU->Next;
        CtbAddrInTs++;
        CtbAddrInRs = CtbAddrTsToRs[CtbAddrInTs];
        xCtb = (CtbAddrInRs % PicWidthInCtbsY) << CtbLog2SizeY;
        yCtb = (CtbAddrInRs / PicWidthInCtbsY) << CtbLog2SizeY;
        SliceAddrRsInTs[CtbAddrInTs] = SliceAddrRs;

        if (   (   pps.tiles_enabled_flag
                && TileId[CtbAddrInTs] != TileId[CtbAddrInTs - 1])
            || (   pps.entropy_coding_sync_enabled_flag
                && (CtbAddrInTs % PicWidthInCtbsY == 0 || TileId[CtbAddrInTs] != TileId[CtbAddrRsToTs[CtbAddrInRs - 1]])))
        {
            if (!EndOfSubsetOneBit())
                throw InvalidSyntax();

            InitPCMBlock();

            if (GetBitOffset() && u(8 - GetBitOffset()))
                throw InvalidSyntax();
        }

        if (TileId[CtbAddrInTs] != TileId[CtbAddrInTs - 1])
        {
            InitCtx();
            InitADE();
            qPY_PREV = SliceQpY;
        }
        else if (pps.entropy_coding_sync_enabled_flag && (CtbAddrInRs % PicWidthInCtbsY) == 0)
        {
            InitPCMBlock();

            if (AvailableZs(xCtb, yCtb, xCtb + CtbLog2SizeY, yCtb - CtbLog2SizeY))
                SyncWPP();
            else
                InitCtx();
            InitADE();
            qPY_PREV = SliceQpY;
        }
    }

    if (!pCTU || (!pCTU->end_of_slice_segment_flag && !slice.Split))
        throw InvalidSyntax();

    if (m_pAllocator)
    {
        nCTU = m_ctu.size() - nCTU;
        nCU  = m_cu.size()  - nCU;
        nPU  = m_pu.size()  - nPU;
        nTU  = m_tu.size()  - nTU;

        pCTU = m_pAllocator->alloc<CTU>(0, (Bs32u)nCTU);
        auto pCU  = m_pAllocator->alloc<CU>(pCTU, (Bs32u)nCU);
        auto pPU  = m_pAllocator->alloc<PU>(pCTU, (Bs32u)nPU);
        auto pTU  = m_pAllocator->alloc<TU>(pCTU, (Bs32u)nTU);
        CTU* pCTU0 = nCTU ? &m_ctu[m_ctu.size() - nCTU] : 0;
        CU*  pCU0  = nCU  ? &m_cu[m_cu.size() - nCU]    : 0;
        PU*  pPU0  = nPU  ? &m_pu[m_pu.size() - nPU]    : 0;
        TU*  pTU0  = nTU  ? &m_tu[m_tu.size() - nTU]    : 0;

        if (pCTU)memmove(pCTU, pCTU0, sizeof(CTU) * nCTU);
        if (pCU) memmove(pCU, pCU0, sizeof(CU) * nCU);
        if (pPU) memmove(pPU, pPU0, sizeof(PU) * nPU);
        if (pTU) memmove(pTU, pTU0, sizeof(TU) * nTU);

        Bs32s* pTC_levels = nullptr;
        if (report_TCLevels)
        {
            pTC_levels = m_pAllocator->alloc<Bs32s>(pCTU, Bs32u(m_TC_lvl.size()));
            if (pTC_levels) std::copy(std::begin(m_TC_lvl), std::end(m_TC_lvl), pTC_levels);
        }

        auto pcCTU = pCTU;

        for (;;)
        {
            if (pcCTU->Cu)
            {
                auto pcCU = pcCTU->Cu = pCU + (pcCTU->Cu - pCU0);

                for (;;)
                {
                    if (pcCU->Pu)
                    {
                        auto pcPU = pcCU->Pu = pPU + (pcCU->Pu - pPU0);

                        for (;;)
                        {
                            if (!pcPU->Next)
                                break;
                            pcPU = pcPU->Next = pPU + (pcPU->Next - pPU0);
                        }
                    }

                    if (pcCU->Tu)
                    {
                        auto pcTU = pcCU->Tu = pTU + (pcCU->Tu - pTU0);

                        for (;;)
                        {
                            if (report_TCLevels && pcTU->tc_levels_luma)
                            {
                                pcTU->tc_levels_luma = pTC_levels + std::distance(m_TC_lvl.data(), pcTU->tc_levels_luma);
                            }

                            if (!pcTU->Next)
                                break;
                            pcTU = pcTU->Next = pTU + (pcTU->Next - pTU0);
                        }
                    }

                    if (!pcCU->Next)
                        break;
                    pcCU = pcCU->Next = pCU + (pcCU->Next - pCU0);
                }
            }

            if (!pcCTU->Next)
                break;
            pcCTU = pcCTU->Next = pCTU + (pcCTU->Next - pCTU0);
        }

        BS2_SET((Bs32u)nCTU, slice.NumCTU);

        //printf("NumCTU = %d\n", slice.NumCTU);
    }

    ColPicSlices = 0;
    NumColSlices = 0;

    return pCTU;
}

void SDParser::parseSAO(CTU& ctu, Bs16u rx, Bs16u ry)
{
    TLAuto tl(*this, TRACE_CTU);
    auto SliceAddrRs = SliceAddrRsInTs[ctu.CtbAddrInTs];
    Bs16u nc = (ChromaArrayType != 0 ? 3 : 1);

    auto ParseType1 = [&] (Bs16u cIdx)
    {
        for (Bs16u i = 0; i < 4; i++)
            if (ctu.sao[cIdx].offset[i] && SaoOffsetSign())
                ctu.sao[cIdx].offset[i] *= -1;

        BS2_SET(SaoBandPosition(), ctu.sao[cIdx].band_position);
    };

    if (rx > 0)
    {
        bool leftCtbInSliceSeg = (ctu.CtbAddrInRs > SliceAddrRs);
        bool leftCtbInTile = (TileId[ctu.CtbAddrInTs] == TileId[CtbAddrRsToTs[ctu.CtbAddrInRs - 1]]);

        if( leftCtbInSliceSeg && leftCtbInTile )
            BS2_SET(SaoMergeLeftFlag(), ctu.sao_merge_left_flag);

        if (ctu.sao_merge_left_flag)
        {
            for (Bs16u cIdx = 0; cIdx < nc; cIdx++)
                ctu.sao[cIdx] = GetSAO(cIdx, rx - 1, ry);

            goto l_exit;
        }
    }

    if (ry > 0)
    {
        bool upCtbInSliceSeg = ((ctu.CtbAddrInRs - PicWidthInCtbsY) >= SliceAddrRs);
        bool upCtbInTile = (TileId[ctu.CtbAddrInTs] == TileId[CtbAddrRsToTs[ctu.CtbAddrInRs - PicWidthInCtbsY]]);

        if( upCtbInSliceSeg && upCtbInTile )
            BS2_SET(SaoMergeUpFlag(), ctu.sao_merge_up_flag);

        if (ctu.sao_merge_up_flag)
        {
            for (Bs16u cIdx = 0; cIdx < nc; cIdx++)
                ctu.sao[cIdx] = GetSAO(cIdx, rx, ry - 1);

            goto l_exit;
        }
    }

    if (m_cSlice->sao_luma_flag)
    {
        BS2_SETM(SaoTypeIdxLuma(), ctu.sao[0].type_idx, SAOTypeTraceMap)

        if( ctu.sao[0].type_idx != 0 )
        {
            for (Bs16u i = 0; i < 4; i++)
                ctu.sao[0].offset[i] = SaoOffsetAbs(BitDepthY);

            if (ctu.sao[0].type_idx == 1)
                ParseType1(0);
            else
                BS2_SETM(SaoEoClassLuma(), ctu.sao[0].eo_class, EOClassTraceMap);

            if (ctu.sao[0].type_idx == 2)
            {
                ctu.sao[0].offset[2] *= -1;
                ctu.sao[0].offset[3] *= -1;
            }

            BS2_TRACE_ARR(ctu.sao[0].offset, 4, 0);
        }
    }

    if (m_cSlice->sao_chroma_flag && nc > 1)
    {
        BS2_SETM(SaoTypeIdxChroma(), ctu.sao[1].type_idx, SAOTypeTraceMap);

        if (ctu.sao[1].type_idx != 0)
        {
            for (Bs16u i = 0; i < 4; i++)
                ctu.sao[1].offset[i] = SaoOffsetAbs(BitDepthC);

            if (ctu.sao[1].type_idx == 1)
                ParseType1(1);
            else
                BS2_SETM(SaoEoClassChroma(), ctu.sao[1].eo_class, EOClassTraceMap);

            if (ctu.sao[1].type_idx == 2)
            {
                ctu.sao[1].offset[2] *= -1;
                ctu.sao[1].offset[3] *= -1;
            }

            BS2_TRACE_ARR(ctu.sao[1].offset, 4, 0);
        }

        BS2_SETM(ctu.sao[1].type_idx, ctu.sao[2].type_idx, SAOTypeTraceMap);

        if (ctu.sao[2].type_idx != 0)
        {
            for (Bs16u i = 0; i < 4; i++)
                ctu.sao[2].offset[i] = SaoOffsetAbs(BitDepthC);

            if (ctu.sao[2].type_idx == 1)
                ParseType1(2);
            else
                BS2_SETM(ctu.sao[1].eo_class, ctu.sao[2].eo_class, EOClassTraceMap);

            if (ctu.sao[2].type_idx == 2)
            {
                ctu.sao[2].offset[2] *= -1;
                ctu.sao[2].offset[3] *= -1;
            }

            BS2_TRACE_ARR(ctu.sao[2].offset, 4, 0);
        }
    }

l_exit:
    for (Bs16u cIdx = 0; cIdx < nc; cIdx++)
        GetSAO(cIdx, rx, ry) = ctu.sao[cIdx];
}

CU* SDParser::parseCQT(CU& cu, Bs16u log2CbSize, Bs16u cqtDepth)
{
    TLAuto tl(*this, TRACE_CQT);
    auto& sps = *m_cSlice->sps;
    auto& pps = *m_cSlice->pps;
    auto x0 = cu.x;
    auto y0 = cu.y;
    bool split_cu_flag = false;

    cu.log2CbSize = log2CbSize;

    if (   x0 + (1 << log2CbSize) <= sps.pic_width_in_luma_samples
        && y0 + (1 << log2CbSize) <= sps.pic_height_in_luma_samples
        && log2CbSize > MinCbLog2SizeY)
    {
        BS2_SET(SplitCuFlag(x0, y0), split_cu_flag);
    }
    else
    {
        BS2_SET((log2CbSize > MinCbLog2SizeY), split_cu_flag);
    }

    if (pps.cu_qp_delta_enabled_flag && log2CbSize >= Log2MinCuQpDeltaSize)
    {
        IsCuQpDeltaCoded = false;
        CuQpDeltaVal = 0;
    }

    if (   m_cSlice->cu_chroma_qp_offset_enabled_flag
        && log2CbSize >= Log2MinCuChromaQpOffsetSize )
    {
        IsCuChromaQpOffsetCoded = false;
    }

    if (split_cu_flag)
    {
        CU* pCu;
        Bs16u x1 = x0 + (1 << (log2CbSize - 1));
        Bs16u y1 = y0 + (1 << (log2CbSize - 1));

        log2CbSize--;
        cqtDepth++;

        auto ParseTree = [&] (Bs16u x, Bs16u y)
        {
            pCu->Next = Alloc<CU>();
            pCu = pCu->Next;
            pCu->x = x;
            pCu->y = y;
            pCu = parseCQT(*pCu, log2CbSize, cqtDepth);
        };

        pCu = parseCQT(cu, log2CbSize, cqtDepth);

        if (x1 < sps.pic_width_in_luma_samples)
            ParseTree(x1, y0);

        if (y1 < sps.pic_height_in_luma_samples)
            ParseTree(x0, y1);

        if (   x1 < sps.pic_width_in_luma_samples
            && y1 < sps.pic_height_in_luma_samples)
            ParseTree(x1, y1);

        return pCu;
    }

    parseCU(cu);

    return &cu;
}

struct LumaPred
{
    Bs8u prev_intra_luma_pred_flag : 1;
    Bs8u mpm_idx                   : 2;
    Bs8u rem_intra_luma_pred_mode  : 5;
};

const Bs8u ModeIdxTable[5][5] =
{
//     0  26  10   1   X
/*0*/{34,  0,  0,  0,  0},
/*1*/{26, 34, 26, 26, 26},
/*2*/{10, 10, 34, 10, 10},
/*3*/{ 1,  1,  1, 34,  1},
/*4*/{ 0, 26, 10,  1, 0xff}
};

inline Bs8u ModeIdx(Bs8u intra_chroma_pred_mode, Bs8u IntraPredModeY)
{
    Bs16u i;
    switch (IntraPredModeY)
    {
    case  0: i = 0; break;
    case 26: i = 1; break;
    case 10: i = 2; break;
    case  1: i = 3; break;
    default: i = 4; break;
    }

    if (i == 4 && intra_chroma_pred_mode == 4)
        return IntraPredModeY;

    return ModeIdxTable[intra_chroma_pred_mode][i];
}

const Bs8u ModeIdxTrans[35] =
{
     0,  1,  2,  2,  2,  2,
     3,  5,  7,  8, 10, 12,
    13, 15, 17, 18, 19, 20,
    21, 22, 23, 23, 24, 24,
    25, 25, 26, 27, 27, 28,
    28, 29, 29, 30, 31
};

void SDParser::parseCU(CU& cu)
{
    TLAuto tl(*this, TRACE_CU);
    auto& sps = *m_cSlice->sps;
    auto& pps = *m_cSlice->pps;
    auto& slice = *m_cSlice;
#ifdef __BS_TRACE__
    const Bs16u dim[] = { 1, 2, 2 };
#endif
    const Bs32u traceAddr = TRACE_PRED|TRACE_CU|TRACE_QP;

    TLStart(traceAddr);
    BS2_TRACE(cu.x, cu.x);
    BS2_TRACE(cu.y, cu.y);
    BS2_TRACE(cu.log2CbSize, cu.log2CbSize);
    TLEnd();

    if (   pps.cu_qp_delta_enabled_flag
        && !(cu.x & ((1 << Log2MinCuQpDeltaSize) - 1))  //only for 1st Cu in Qg
        && !(cu.y & ((1 << Log2MinCuQpDeltaSize) - 1)))
    {
        //8.6.1 Derivation process for quantization parameters
        Bs16s qPY_A, qPY_B;
        Bs16s xQg = cu.x/* - (cu.x & ((1 << Log2MinCuQpDeltaSize) - 1))*/;
        Bs16s yQg = cu.y/* - (cu.y & ((1 << Log2MinCuQpDeltaSize) - 1))*/;
        bool availableA = AvailableZs(cu.x, cu.y, xQg - 1, yQg);
        bool availableB = AvailableZs(cu.x, cu.y, xQg, yQg - 1);
        Bs16s xTmpA = (xQg - 1) >> MinTbLog2SizeY;
        Bs16s yTmpA = yQg >> MinTbLog2SizeY;
        Bs16s xTmpB = xQg >> MinTbLog2SizeY;
        Bs16s yTmpB = (yQg - 1) >> MinTbLog2SizeY;
        Bs32u ctbAddrA = availableA ? (MinTbAddrZs(xTmpA, yTmpA) >> (2 * (CtbLog2SizeY - MinTbLog2SizeY))) : 0;
        Bs32u ctbAddrB = availableB ? (MinTbAddrZs(xTmpB, yTmpB) >> (2 * (CtbLog2SizeY - MinTbLog2SizeY))) : 0;
        Bs32u CtbAddrInTs = CtbAddrRsToTs[(cu.y >> CtbLog2SizeY) * PicWidthInCtbsY + (cu.x >> CtbLog2SizeY)];

        if (!availableA || ctbAddrA != CtbAddrInTs)
            qPY_A = qPY_PREV;
        else
        {
            CU* pCU = GetCU(xQg - 1, yQg);
            if (!pCU)
                throw InvalidSyntax();
            qPY_A = pCU->QpY;
        }

        if (!availableB || ctbAddrB != CtbAddrInTs)
            qPY_B = qPY_PREV;
        else
        {
            CU* pCU = GetCU(xQg, yQg - 1);
            if (!pCU)
                throw InvalidSyntax();
            qPY_B = pCU->QpY;
        }

        qPY_PREV = ((qPY_A + qPY_B + 1) >> 1); // qPY_PREV = qPY_PRED
    }

    cu.QpY  = qPY_PREV;
    cu.QpCb = GetQpCb(cu.QpY, 0);
    cu.QpCr = GetQpCr(cu.QpY, 0);

    if (pps.transquant_bypass_enabled_flag)
        BS2_SET(CuTransquantBypassFlag(), cu.transquant_bypass_flag);

    if (slice.type != I && CuSkipFlag(cu.x, cu.y))
        BS2_SETM(MODE_SKIP, cu.PredMode, PredModeTraceMap);

    Bs16u nCbS = (1 << cu.log2CbSize);

    if (cu.PredMode == MODE_SKIP)
    {
        TLAuto tl(*this, TRACE_PU);
        cu.Pu = Alloc<PU>();
        auto& pu = *cu.Pu;

        pu.x = cu.x;
        pu.y = cu.y;
        pu.w = nCbS;
        pu.h = nCbS;

        pu.merge_flag = 1;

        if (slice.MaxNumMergeCand > 1)
            BS2_SET(MergeIdx(), pu.merge_idx);

        decodeMvLX(cu, pu, 0);

        BS2_TRACE(pu.inter_pred_idc, pu.inter_pred_idc);

        TLStart(TRACE_PRED|TRACE_PU);

        BS2_TRACE(pu.x, pu.x);
        BS2_TRACE(pu.y, pu.y);
        BS2_TRACE(pu.w, pu.w);
        BS2_TRACE(pu.h, pu.h);

        if (pu.inter_pred_idc != PRED_L1)
            BS2_TRACE(pu.ref_idx_l0, pu.ref_idx_l0);
        if (pu.inter_pred_idc != PRED_L0)
            BS2_TRACE(pu.ref_idx_l1, pu.ref_idx_l1);

        BS2_TRACE_MDARR(Bs16s, pu.MvLX, dim, 1, 0, "%i ");

        TLEnd();

        return;
    }

    if (slice.type != I)
        BS2_SETM(PredModeFlag(), cu.PredMode, PredModeTraceMap)
    else
        BS2_SETM(MODE_INTRA, cu.PredMode, PredModeTraceMap);

    if (   sps.palette_mode_enabled_flag
        && cu.PredMode == MODE_INTRA
        && cu.log2CbSize <= MaxTbLog2SizeY)
    {
        BS2_SET(PaletteModeFlag(), cu.palette_mode_flag);

        if (cu.palette_mode_flag)
        {
            parsePC(cu);

            TLStart(TRACE_QP);
            BS2_TRACE(cu.QpY, cu.QpY);
            BS2_TRACE(cu.QpCb, cu.QpCb);
            BS2_TRACE(cu.QpCr, cu.QpCr);
            TLEnd();
            return;
        }
    }

    if (cu.PredMode != MODE_INTRA || cu.log2CbSize == MinCbLog2SizeY)
        BS2_SETM(PartMode(cu.PredMode == MODE_INTRA, cu.log2CbSize), cu.PartMode, PartModeTraceMap);

    if (cu.PredMode == MODE_INTRA)
    {
        if (   cu.PartMode == PART_2Nx2N
            && sps.pcm_enabled_flag
            && cu.log2CbSize >= Log2MinIpcmCbSizeY
            && cu.log2CbSize <= Log2MaxIpcmCbSizeY)
        {
            BS2_SET(PcmFlag(), cu.pcm_flag);
        }

        if (cu.pcm_flag)
        {
            TLAuto tl1(*this, TRACE_COEF);

            InitPCMBlock();

            while (GetBitOffset())
                if (u1())
                    throw InvalidSyntax();

            Bs16u size  = (1 << (cu.log2CbSize << 1));
            Bs16u split = nCbS;
            std::ignore = split;

            if (!TLTest(traceAddr))
            {
                BS2_TRACE(cu.x, cu.x);
                BS2_TRACE(cu.y, cu.y);
                BS2_TRACE(cu.log2CbSize, cu.log2CbSize);
            }

            BS2_TRACE_ARR_VF(u(sps.pcm_sample_bit_depth_luma_minus1+1),
                pcm_sample_luma, size, split, BitDepthY > 8 ? "%04X " : "%02X ");

            if(ChromaArrayType != 0)
            {
                size = ((2 << (cu.log2CbSize << 1)) / (SubWidthC * SubHeightC));
                split /= SubWidthC;

                BS2_TRACE_ARR_VF(u(sps.pcm_sample_bit_depth_chroma_minus1+1),
                    pcm_sample_chroma, size, split, BitDepthC > 8 ? "%04X " : "%02X ");
            }

            InitADE();

            return;
        }

        LumaPred lp[2][2] = {};
        Bs16u pbOffset = (cu.PartMode == PART_NxN ) ? (nCbS / 2) : nCbS, i, j;

        memset(intra_chroma_pred_mode, 0, sizeof(intra_chroma_pred_mode));

        for (j = 0; j < nCbS / pbOffset; j++)
            for (i = 0; i < nCbS / pbOffset; i++)
                BS2_SET(PrevIntraLumaPredFlag(), lp[i][j].prev_intra_luma_pred_flag);

        for (j = 0; j < nCbS / pbOffset; j++)
        {
            for (i = 0; i < nCbS / pbOffset; i++)
            {
                if (lp[i][j].prev_intra_luma_pred_flag)
                    BS2_SET(MpmIdx(), lp[i][j].mpm_idx)
                else
                    BS2_SET(RemIntraLumaPredMode(), lp[i][j].rem_intra_luma_pred_mode);
            }
        }

        for (j = 0; j < nCbS / pbOffset; j++)
        {
            for (i = 0; i < nCbS / pbOffset; i++)
            {
                auto& clp = lp[j][i];
                auto& IntraPredModeY = cu.IntraPredModeY[j][i];
                Bs16u xPb = cu.x + j * pbOffset;
                Bs16u yPb = cu.y + i * pbOffset;
                bool  availableX[2] =
                {
                    AvailableZs(xPb, yPb, xPb - 1, yPb),
                    AvailableZs(xPb, yPb, xPb, yPb - 1)
                };
                Bs8u  candIntraPredModeX[2] = {};
                Bs8u  candModeList[3] = {};

                for (Bs16u c = 0; c < 2; c++)
                {
                    if (!availableX[c])
                        candIntraPredModeX[c] = 1;
                    else
                    {
                        CU* cuX = GetCU(xPb - !c, yPb - c);

                        if (!cuX)
                            throw InvalidSyntax();

                        if (cuX->PredMode != MODE_INTRA || cuX->pcm_flag )
                            candIntraPredModeX[c] = 1;
                        else if (c && ((yPb - c) < ((yPb >> CtbLog2SizeY) << CtbLog2SizeY)))
                            candIntraPredModeX[c] = 1;
                        else
                            candIntraPredModeX[c] = cuX->IntraPredModeY[!!(j - !c)][!!(i - c)];
                    }
                }

                if (candIntraPredModeX[0] == candIntraPredModeX[1])
                {
                    if (candIntraPredModeX[0] < 2)
                    {
                        candModeList[0] = 0;
                        candModeList[1] = 1;
                        candModeList[2] = 26;
                    }
                    else
                    {
                        candModeList[0] = candIntraPredModeX[0];
                        candModeList[1] = 2 + ((candIntraPredModeX[0] + 29) % 32);
                        candModeList[2] = 2 + ((candIntraPredModeX[0] - 2 + 1) % 32);
                    }
                }
                else
                {
                    candModeList[0] = candIntraPredModeX[0];
                    candModeList[1] = candIntraPredModeX[1];

                    if (candModeList[0] != 0 && candModeList[1] != 0)
                        candModeList[2] = 0;
                    else if (candModeList[0] != 1 && candModeList[1] != 1)
                        candModeList[2] = 1;
                    else
                        candModeList[2] = 26;
                }

                if (clp.prev_intra_luma_pred_flag)
                    IntraPredModeY = candModeList[clp.mpm_idx];
                else
                {
                    if (candModeList[0] > candModeList[1])
                        std::swap(candModeList[0], candModeList[1]);

                    if (candModeList[0] > candModeList[2])
                        std::swap(candModeList[0], candModeList[2]);

                    if (candModeList[1] > candModeList[2])
                        std::swap(candModeList[1], candModeList[2]);

                    IntraPredModeY = clp.rem_intra_luma_pred_mode;

                    for (Bs16u c = 0; c < 3; c++)
                        IntraPredModeY += (IntraPredModeY >= candModeList[c]);
                }
            }
        }

        TLStart(TRACE_PRED);

        if (nCbS == pbOffset)
        {
            cu.IntraPredModeY[0][1] = cu.IntraPredModeY[0][0];
            cu.IntraPredModeY[1][0] = cu.IntraPredModeY[0][0];
            cu.IntraPredModeY[1][1] = cu.IntraPredModeY[0][0];

            BS2_SETM(cu.IntraPredModeY[0][0], cu.IntraPredModeY[0][0], IntraPredModeTraceMap);
        }
        else if (Trace())
        {
            Bs8u IntraPredModeY[4] =
            {
                cu.IntraPredModeY[0][0], cu.IntraPredModeY[1][0],
                cu.IntraPredModeY[0][1], cu.IntraPredModeY[1][1],
            };
            BS2_SET_ARR_M(IntraPredModeY[_i], IntraPredModeY, 4, 2, " %8s(%2i)", IntraPredModeTraceMap);
        }

        if (ChromaArrayType == 3 && nCbS != pbOffset)
        {
            TLStart(TRACE_CU);
            BS2_SET(IntraChromaPredMode(), intra_chroma_pred_mode[0][0]);
            BS2_SET(IntraChromaPredMode(), intra_chroma_pred_mode[1][0]);
            BS2_SET(IntraChromaPredMode(), intra_chroma_pred_mode[0][1]);
            BS2_SET(IntraChromaPredMode(), intra_chroma_pred_mode[1][1]);
            TLEnd();

            cu.IntraPredModeC[0][0] = ModeIdx(intra_chroma_pred_mode[0][0], cu.IntraPredModeY[0][0]);
            cu.IntraPredModeC[0][1] = ModeIdx(intra_chroma_pred_mode[0][1], cu.IntraPredModeY[0][1]);
            cu.IntraPredModeC[1][0] = ModeIdx(intra_chroma_pred_mode[1][0], cu.IntraPredModeY[1][0]);
            cu.IntraPredModeC[1][1] = ModeIdx(intra_chroma_pred_mode[1][1], cu.IntraPredModeY[1][1]);

            if (Trace())
            {
                Bs8u IntraPredModeC[4] =
                {
                    cu.IntraPredModeC[0][0], cu.IntraPredModeC[1][0],
                    cu.IntraPredModeC[0][1], cu.IntraPredModeC[1][1],
                };
                BS2_SET_ARR_M(IntraPredModeC[_i], IntraPredModeC, 4, 2, " %8s(%2i)", IntraPredModeTraceMap);
            }
        }
        else if (ChromaArrayType != 0)
        {
            TLStart(TRACE_CU);
            BS2_SET(IntraChromaPredMode(), intra_chroma_pred_mode[0][0]);
            TLEnd();

            intra_chroma_pred_mode[0][1] = intra_chroma_pred_mode[0][0];
            intra_chroma_pred_mode[1][0] = intra_chroma_pred_mode[0][0];
            intra_chroma_pred_mode[1][1] = intra_chroma_pred_mode[0][0];

            cu.IntraPredModeC[0][0] = ModeIdx(intra_chroma_pred_mode[0][0], cu.IntraPredModeY[0][0]);

            if (ChromaArrayType == 2)
                cu.IntraPredModeC[0][0] = ModeIdxTrans[cu.IntraPredModeC[0][0]];

            cu.IntraPredModeC[0][1] = cu.IntraPredModeC[0][0];
            cu.IntraPredModeC[1][0] = cu.IntraPredModeC[0][0];
            cu.IntraPredModeC[1][1] = cu.IntraPredModeC[0][0];

            BS2_SETM(cu.IntraPredModeC[0][0], cu.IntraPredModeC[0][0], IntraPredModeTraceMap);
        }
        TLEnd();
    }
    else
    {
        auto x0 = cu.x;
        auto y0 = cu.y;
        Bs16u CtDepth = CtbLog2SizeY - cu.log2CbSize;

        auto PredictionUnit = [&] (Bs16u x, Bs16u y, Bs16u w, Bs16u h, Bs16u partIdx) -> PU*
        {
            PU* pPU = Alloc<PU>();
            pPU->x = x;
            pPU->y = y;
            pPU->w = w;
            pPU->h = h;

            parsePU(cu, *pPU, CtDepth, partIdx);

            return pPU;
        };

        switch (cu.PartMode)
        {
        case PART_2Nx2N:
            cu.Pu =
                PredictionUnit(x0, y0, nCbS, nCbS, 0);
            break;
        case PART_2NxN:
            cu.Pu =
                PredictionUnit(x0, y0, nCbS, nCbS / 2, 0);
            cu.Pu->Next =
                PredictionUnit(x0, y0 + nCbS / 2, nCbS, nCbS / 2, 1);
            break;
        case PART_Nx2N:
            cu.Pu =
                PredictionUnit(x0, y0, nCbS / 2, nCbS, 0);
            cu.Pu->Next =
                PredictionUnit(x0 + nCbS / 2, y0, nCbS / 2, nCbS, 1);
            break;
        case PART_2NxnU:
            cu.Pu =
                PredictionUnit(x0, y0, nCbS, nCbS / 4, 0);
            cu.Pu->Next =
                PredictionUnit(x0, y0 + nCbS / 4, nCbS, nCbS * 3 / 4, 1);
            break;
        case PART_2NxnD:
            cu.Pu =
                PredictionUnit(x0, y0, nCbS, nCbS * 3 / 4, 0);
            cu.Pu->Next =
                PredictionUnit(x0, y0 + nCbS * 3 / 4, nCbS, nCbS / 4, 1);
            break;
        case PART_nLx2N:
            cu.Pu =
                PredictionUnit(x0, y0, nCbS / 4, nCbS, 0);
            cu.Pu->Next =
                PredictionUnit(x0 + nCbS / 4, y0, nCbS * 3 / 4, nCbS, 1);
            break;
        case PART_nRx2N:
            cu.Pu =
                PredictionUnit(x0, y0, nCbS * 3 / 4, nCbS, 0);
            cu.Pu->Next =
                PredictionUnit(x0 + nCbS * 3 / 4, y0, nCbS / 4, nCbS, 1);
            break;
        case PART_NxN:
            cu.Pu =
                PredictionUnit(x0, y0, nCbS / 2, nCbS / 2, 0);
            cu.Pu->Next =
                PredictionUnit(x0 + nCbS / 2, y0, nCbS / 2, nCbS / 2, 1);
            cu.Pu->Next->Next =
                PredictionUnit(x0, y0 + nCbS / 2, nCbS / 2, nCbS / 2, 2);
            cu.Pu->Next->Next->Next =
                PredictionUnit(x0 + nCbS / 2, y0 + nCbS / 2, nCbS / 2, nCbS / 2, 3);
            break;
        default:
            throw InvalidSyntax();
        }
    }

    bool rqt_root_cbf = true;

    if (   cu.PredMode != MODE_INTRA
        && !(cu.PartMode == PART_2Nx2N && cu.Pu->merge_flag))
        BS2_SET(RqtRootCbf(), rqt_root_cbf)

    if (rqt_root_cbf)
    {
        IntraSplitFlag = cu.PredMode == MODE_INTRA && cu.PartMode == PART_NxN;
        MaxTrafoDepth = (cu.PredMode == MODE_INTRA ? (sps.max_transform_hierarchy_depth_intra + IntraSplitFlag ) : sps.max_transform_hierarchy_depth_inter);

        cu.Tu = Alloc<TU>();
        cu.Tu->log2TrafoSize = cu.log2CbSize;
        cu.Tu->x = cu.x;
        cu.Tu->y = cu.y;

        parseTT(cu, *cu.Tu, *cu.Tu, 0);
    }

    TLStart(TRACE_QP);

    BS2_TRACE(cu.QpY, cu.QpY);
    BS2_TRACE(cu.QpCb, cu.QpCb);
    BS2_TRACE(cu.QpCr, cu.QpCr);

    TLEnd();
}

void SDParser::parsePU(CU& cu, PU& pu, Bs16u CtDepth, Bs16u partIdx)
{
    TLAuto tl(*this, TRACE_PU);
#ifdef __BS_TRACE__
    const Bs16u dim[] = { 1, 2, 2 };
#endif
    auto& slice = *m_cSlice;
    Bs32s MvdLX[2][2] = {};
    auto mvd_coding = [&] (Bs16u refList)
    {
        bool abs_mvd_greater0_flag[2] = {};
        bool abs_mvd_greater1_flag[2] = {};
        bool mvd_sign_flag[2] = {};
        Bs32s abs_mvd_minus2[2] = {-1, -1};

        BS2_SET(AbsMvdGreater0Flag(), abs_mvd_greater0_flag[0]);
        BS2_SET(AbsMvdGreater0Flag(), abs_mvd_greater0_flag[1]);

        if (abs_mvd_greater0_flag[0])
            BS2_SET(AbsMvdGreater1Flag(), abs_mvd_greater1_flag[0]);

        if (abs_mvd_greater0_flag[1])
            BS2_SET(AbsMvdGreater1Flag(), abs_mvd_greater1_flag[1]);

        if (abs_mvd_greater0_flag[0])
        {
            if (abs_mvd_greater1_flag[0])
                BS2_SET(AbsMvdMinus2(), abs_mvd_minus2[0]);

            BS2_SET(MvdSignFlag(), mvd_sign_flag[0]);
        }

        if (abs_mvd_greater0_flag[1])
        {
            if (abs_mvd_greater1_flag[1])
                BS2_SET(AbsMvdMinus2(), abs_mvd_minus2[1]);

            BS2_SET(MvdSignFlag(), mvd_sign_flag[1]);
        }

        MvdLX[refList][0] = abs_mvd_greater0_flag[0] * (abs_mvd_minus2[0] + 2) * (1 - 2 * mvd_sign_flag[0]);
        MvdLX[refList][1] = abs_mvd_greater0_flag[1] * (abs_mvd_minus2[1] + 2) * (1 - 2 * mvd_sign_flag[1]);
    };

    TLStart(TRACE_PRED | TRACE_PU);
    BS2_TRACE(pu.x, pu.x);
    BS2_TRACE(pu.y, pu.y);
    BS2_TRACE(pu.w, pu.w);
    BS2_TRACE(pu.h, pu.h);
    TLEnd();

    BS2_SET(MergeFlag(), pu.merge_flag);

    if (pu.merge_flag)
    {
        if (slice.MaxNumMergeCand > 1)
            BS2_SET(MergeIdx(), pu.merge_idx);

        decodeMvLX(cu, pu, partIdx);

        BS2_TRACE(pu.inter_pred_idc, pu.inter_pred_idc);

        TLStart(TRACE_PRED | TRACE_PU);

        if (pu.inter_pred_idc != PRED_L1)
            BS2_TRACE(pu.ref_idx_l0, pu.ref_idx_l0);
        if (pu.inter_pred_idc != PRED_L0)
            BS2_TRACE(pu.ref_idx_l1, pu.ref_idx_l1);

        BS2_TRACE_MDARR(Bs16s, pu.MvLX, dim, 1, 0, "%i ");

        TLEnd();

        return;
    }

    if (slice.type == SLICE_TYPE::B)
        BS2_SET(InterPredIdc(pu.w, pu.h, CtDepth), pu.inter_pred_idc);

    if (pu.inter_pred_idc != PRED_L1)
    {
        if ( slice.num_ref_idx_l0_active > 1)
        {
            TLStart(TRACE_PRED | TRACE_PU);
            BS2_SET(RefIdxL0(), pu.ref_idx_l0);
            TLEnd();
        }

        mvd_coding(0);

        BS2_SET(MvpL0Flag(), pu.mvp_l0_flag);
    }

    if (pu.inter_pred_idc != PRED_L0)
    {
        if (slice.num_ref_idx_l1_active > 1)
        {
            TLStart(TRACE_PRED | TRACE_PU);
            BS2_SET(RefIdxL1(), pu.ref_idx_l1);
            TLEnd();
        }

        if (slice.mvd_l1_zero_flag && pu.inter_pred_idc == PRED_BI)
        {
            //MvdL1[ x0 ][ y0 ][ 0 ] = 0
            //MvdL1[ x0 ][ y0 ][ 1 ] = 0
        }
        else
            mvd_coding(1);

        BS2_SET(MvpL1Flag(), pu.mvp_l1_flag);
    }

    BS2_TRACE_MDARR(Bs32s, MvdLX, dim, 1, 0, "%i ");

    if (pu.inter_pred_idc != PRED_L1)
        decodeMvLX(cu, pu, 0, MvdLX[0], partIdx);

    if (pu.inter_pred_idc != PRED_L0)
        decodeMvLX(cu, pu, 1, MvdLX[1], partIdx);

    TLStart(TRACE_PRED | TRACE_PU);
    BS2_TRACE_MDARR(Bs16s, pu.MvLX, dim, 1, 0, "%i ");

    TLEnd();
}

TU* SDParser::parseTT(CU& cu, TU& tu, TU& tuBase, Bs16u blkIdx)
{
    TLAuto tl(*this, TRACE_TT);
    Bs16u trafoDepth = cu.log2CbSize - tu.log2TrafoSize;
    bool split_transform_flag;

    if (   tu.log2TrafoSize <= MaxTbLog2SizeY
        && tu.log2TrafoSize >  MinTbLog2SizeY
        && trafoDepth < MaxTrafoDepth
        && !(IntraSplitFlag && (trafoDepth == 0)))
    {
        BS2_SET(SplitTransformFlag(tu.log2TrafoSize), split_transform_flag);
    }
    else
    {
        bool interSplitFlag =
               (m_cSlice->sps->max_transform_hierarchy_depth_inter == 0)
            && (cu.PredMode == MODE_INTER)
            && (cu.PartMode != PART_2Nx2N)
            && (trafoDepth == 0);
        BS2_SET(
               (tu.log2TrafoSize > MaxTbLog2SizeY)
            || (IntraSplitFlag && 0 == trafoDepth)
            || interSplitFlag
            , split_transform_flag);
    }

    if ((tu.log2TrafoSize > 2 && ChromaArrayType != 0 ) || ChromaArrayType == 3)
    {
        if (trafoDepth == 0 || tuBase.cbf_cb)
        {
            BS2_SET(CbfCb(trafoDepth), tu.cbf_cb);

            if (ChromaArrayType == 2 && (!split_transform_flag || tu.log2TrafoSize == 3))
                BS2_SET(CbfCb(trafoDepth), tu.cbf_cb1)
        }

        if (trafoDepth == 0 || tuBase.cbf_cr)
        {
            BS2_SET(CbfCr(trafoDepth), tu.cbf_cr);

            if (ChromaArrayType == 2 && (!split_transform_flag || tu.log2TrafoSize == 3))
                BS2_SET(CbfCr(trafoDepth), tu.cbf_cr1)
        }
    }

    if (split_transform_flag)
    {
        TU tu0 = tu, *pTU = &tu;
        Bs16u x0 = tu0.x;
        Bs16u y0 = tu0.y;
        Bs16u x1 = x0 + (1 << (tu0.log2TrafoSize - 1));
        Bs16u y1 = y0 + (1 << (tu0.log2TrafoSize - 1));
        auto transform_tree = [&] (Bs16u x, Bs16u y, Bs16u blk)
        {
            pTU->x = x;
            pTU->y = y;
            pTU->log2TrafoSize = tu0.log2TrafoSize - 1;
            pTU = parseTT(cu, *pTU, tu0, blk);
        };

        tu = TU();
        transform_tree(x0, y0, 0);

        pTU->Next = Alloc<TU>();
        pTU = pTU->Next;
        transform_tree(x1, y0, 1);

        pTU->Next = Alloc<TU>();
        pTU = pTU->Next;
        transform_tree(x0, y1, 2);

        pTU->Next = Alloc<TU>();
        pTU = pTU->Next;
        transform_tree(x1, y1, 3);

        return pTU;
    }

    if (   cu.PredMode == MODE_INTRA
        || trafoDepth != 0
        || tu.cbf_cb
        || tu.cbf_cr
        || tu.cbf_cb1
        || tu.cbf_cr1)
    {
        BS2_SET(CbfLuma(trafoDepth), tu.cbf_luma);
    }
    else
        BS2_SET(1, tu.cbf_luma);

    parseTU(cu, tu, tuBase, blkIdx);

    return &tu;
}

void SDParser::parseDQP(CU& cu)
{
    auto& pps = *m_cSlice->pps;

    if (pps.cu_qp_delta_enabled_flag && !IsCuQpDeltaCoded)
    {
        CuQpDeltaVal = CuQpDeltaAbs();

        if (CuQpDeltaVal && CuQpDeltaSignFlag())
            CuQpDeltaVal *= -1;

        IsCuQpDeltaCoded = true;

        BS2_TRACE(CuQpDeltaVal, CuQpDeltaVal);
        cu.QpY = ((qPY_PREV + CuQpDeltaVal + 52 + 2 * QpBdOffsetY) % (52 + QpBdOffsetY)) - QpBdOffsetY;
        cu.QpCb = GetQpCb(cu.QpY, 0);
        cu.QpCr = GetQpCr(cu.QpY, 0);

        qPY_PREV = cu.QpY;
    }
}

void SDParser::parseCQPO(CU& cu)
{
    auto& pps = *m_cSlice->pps;
    auto& slice = *m_cSlice;

    if (slice.cu_chroma_qp_offset_enabled_flag && !IsCuChromaQpOffsetCoded)
    {
        BS2_SET(CuChromaQpOffsetFlag(), cu.chroma_qp_offset_flag);

        if (cu.chroma_qp_offset_flag && pps.chroma_qp_offset_list_len_minus1 > 0)
        {
            BS2_SET(CuChromaQpOffsetIdx(), cu.chroma_qp_offset_idx);

            cu.QpCb = GetQpCb(cu.QpY, pps.cb_qp_offset_list[cu.chroma_qp_offset_idx]);
            cu.QpCr = GetQpCr(cu.QpY, pps.cr_qp_offset_list[cu.chroma_qp_offset_idx]);
        }

        IsCuChromaQpOffsetCoded = true;
    }
}

void SDParser::parseTU(CU& cu, TU& tu, TU& tuBase, Bs16u blkIdx)
{
    TLAuto tl(*this, TRACE_TU);
    Bs16u log2TrafoSizeC = BS_MAX(2, tu.log2TrafoSize - (ChromaArrayType == 3 ? 0 : 1));
    auto& pps = *m_cSlice->pps;
    auto& slice = *m_cSlice;
    bool cbfLuma = tu.cbf_luma, cbfChroma;
    Bs16u x0 = tu.x;
    Bs16u y0 = tu.y;
    Bs16u xBase = tuBase.x;
    Bs16u yBase = tuBase.y;
    Bs16u xP = (x0 >> MinCbLog2SizeY) << MinCbLog2SizeY;
    Bs16u yP = (y0 >> MinCbLog2SizeY) << MinCbLog2SizeY;
    Bs16u nCbS = (1 << MinCbLog2SizeY);
    Bs16u log2TrafoSize = tu.log2TrafoSize;
    auto ICPM = [&](Bs16u x, Bs16u y)
    {
        return intra_chroma_pred_mode[(x - cu.x) >= (1 << (cu.log2CbSize - 1))][(y - cu.y) >= (1 << (cu.log2CbSize - 1))];
    };

    BS2_TRACE(tu.x, tu.x);
    BS2_TRACE(tu.y, tu.y);
    BS2_TRACE(tu.log2TrafoSize, tu.log2TrafoSize);

    if (ChromaArrayType != 3 && tu.log2TrafoSize == 2)
        cbfChroma = tuBase.cbf_cb || tuBase.cbf_cr || tuBase.cbf_cb1 || tuBase.cbf_cr1;
    else
        cbfChroma = tu.cbf_cb || tu.cbf_cr || tu.cbf_cb1 || tu.cbf_cr1;

    if (!cbfLuma && !cbfChroma)
        return;

    if (   pps.residual_adaptive_colour_transform_enabled_flag
        && (    cu.PredMode == MODE_INTER
            || (cu.PartMode == PART_2Nx2N && ICPM(x0, y0) == 4)
            || (   4 == ICPM(xP, yP)
                && 4 == ICPM(xP + nCbS / 2, yP)
                && 4 == ICPM(xP, yP + nCbS / 2)
                && 4 == ICPM(xP + nCbS / 2, yP + nCbS / 2))))
    {
        BS2_SET(TuResidualActFlag(), tu.tu_residual_act_flag);
    }

    parseDQP(cu);

    if (cbfChroma && !cu.transquant_bypass_flag)
        parseCQPO(cu);

    if (pps.residual_adaptive_colour_transform_enabled_flag)
    {
        tu.QP[0] = cu.QpY + Bs16s((pps.ActQpOffsetY + slice.act_y_qp_offset) * tu.tu_residual_act_flag);
        tu.QP[1] = GetQpCb(cu.QpY
            , Bs16s(pps.cb_qp_offset_list[cu.chroma_qp_offset_idx] * cu.chroma_qp_offset_flag)
            + Bs16s((pps.ActQpOffsetCb + slice.act_cb_qp_offset) * tu.tu_residual_act_flag));
        tu.QP[2] = GetQpCr(cu.QpY
            , Bs16s(pps.cr_qp_offset_list[cu.chroma_qp_offset_idx] * cu.chroma_qp_offset_flag)
            + Bs16s((pps.ActQpOffsetCr + slice.act_cr_qp_offset) * tu.tu_residual_act_flag));

        TLStart(TRACE_QP);
        BS2_TRACE_ARR(tu.QP, 3, 0);
        TLEnd();
    }
    else
    {
        tu.QP[0] = cu.QpY;
        tu.QP[1] = cu.QpCb;
        tu.QP[2] = cu.QpCr;
    }

    if (cbfLuma)
    {
        parseResidual(cu, tu, x0, y0, log2TrafoSize, 0);
    }

    if (tu.log2TrafoSize > 2 || ChromaArrayType == 3)
    {
        bool cross_comp_pred =
                pps.cross_component_prediction_enabled_flag
            && cbfLuma
            && (cu.PredMode == MODE_INTER || ICPM(x0, y0) == 4);

        if (cross_comp_pred)
        {
            BS2_SET(Log2ResScaleAbsPlus1(0), tu.log2_res_scale_abs_plus1_0);

            if (tu.log2_res_scale_abs_plus1_0)
                BS2_SET(ResScaleSignFlag(0), tu.res_scale_sign_flag_0);
        }

        if (tu.cbf_cb)
            parseResidual(cu, tu, x0, y0, log2TrafoSizeC, 1);

        if (tu.cbf_cb1)
            parseResidual(cu, tu, x0, y0 + (1 << log2TrafoSizeC), log2TrafoSizeC, 1);

        if (cross_comp_pred)
        {
            BS2_SET(Log2ResScaleAbsPlus1(1), tu.log2_res_scale_abs_plus1_1);

            if (tu.log2_res_scale_abs_plus1_1)
                BS2_SET(ResScaleSignFlag(1), tu.res_scale_sign_flag_1);
        }

        if (tu.cbf_cr)
            parseResidual(cu, tu, x0, y0, log2TrafoSizeC, 2);

        if (tu.cbf_cr1)
            parseResidual(cu, tu, x0, y0 + (1 << log2TrafoSizeC), log2TrafoSizeC, 2);
    }
    else if (blkIdx == 3)
    {
        if (tuBase.cbf_cb)
            parseResidual(cu, tuBase, xBase, yBase, log2TrafoSizeC, 1);

        if (tuBase.cbf_cb1)
            parseResidual(cu, tuBase, xBase, yBase + (1 << log2TrafoSizeC), log2TrafoSizeC, 1);

        if (tuBase.cbf_cr)
            parseResidual(cu, tuBase, xBase, yBase, log2TrafoSizeC, 2);

        if (tuBase.cbf_cr1)
            parseResidual(cu, tuBase, xBase, yBase + (1 << log2TrafoSizeC), log2TrafoSizeC, 2);
    }
}

const Bs8u ctxIdxMap[15] = { 0, 1, 4, 5, 2, 3, 4, 5, 6, 6, 8, 8, 7, 7, 8 };

void SDParser::parseResidual(CU& cu, TU& tu, Bs16u x0, Bs16u y0, Bs16u log2TrafoSize, Bs16u cIdx)
{
    TLAuto tl(*this, TRACE_RESIDUAL);
    auto& sps = *m_cSlice->sps;
    auto& pps = *m_cSlice->pps;
    bool  transform_skip_flag = false
        , explicit_rdpcm_flag = false
        , explicit_rdpcm_dir_flag = false
        , coded_sub_block_flag[16][16] = {}
        , sig_coeff_flag
        ;
    Bs32s LastSignificantCoeffX
        , LastSignificantCoeffY
        , predModeIntra = -1
        , xS
        , yS
        , xC
        , yC
        , scanIdx = 0
        , lastScanPos
        , lastSubBlock
        ;
    Bs32s sig_coeff[16], nSC;

    TLStart(TRACE_RESIDUAL | TRACE_COEF);
    BS2_TRACE(x0, x0);
    BS2_TRACE(y0, y0);
    BS2_TRACE(log2TrafoSize, log2TrafoSize);
    BS2_TRACE(cIdx, cIdx);
    TLEnd();

    Bs32s* pTCLevels = nullptr;
    Bs16u  nCoeff    = 1 << (2 * log2TrafoSize);

    if (report_TCLevels && cIdx == 0)
    {
        pTCLevels = tu.tc_levels_luma = Alloc<Bs32s>(nCoeff);
    }
    else
    {
        TCLevels.resize(nCoeff);
        std::fill(std::begin(TCLevels), std::end(TCLevels), 0);

        pTCLevels = TCLevels.data();
    }

    if (   cu.PredMode == MODE_INTRA
        && (log2TrafoSize == 2 || (log2TrafoSize == 3 && (cIdx == 0 || ChromaArrayType == 3))))
    {
        bool icpmX = (x0 - cu.x) >= (1 << (cu.log2CbSize - 1));
        bool icpmY = (y0 - cu.y) >= (1 << (cu.log2CbSize - 1));

        predModeIntra = cIdx ? cu.IntraPredModeC[icpmX][icpmY] : cu.IntraPredModeY[icpmX][icpmY];

        BS2_SETM(predModeIntra, predModeIntra, IntraPredModeTraceMap);

        if(predModeIntra >= 6 && predModeIntra <= 14)
            scanIdx = 2;
        else if(predModeIntra >= 22 && predModeIntra <= 30)
            scanIdx = 1;
    }

    if (   pps.transform_skip_enabled_flag
        && !cu.transquant_bypass_flag
        && (log2TrafoSize <= (pps.log2_max_transform_skip_block_size_minus2 + 2)))
    {
        BS2_SET(TransformSkipFlag(cIdx), transform_skip_flag);

        if (transform_skip_flag)
            tu.transform_skip_flag |= (1 << cIdx);
    }

    if (   cu.PredMode == MODE_INTER
        && sps.explicit_rdpcm_enabled_flag
        && (transform_skip_flag || cu.transquant_bypass_flag))
    {
        BS2_SET(ExplicitRdpcmFlag(cIdx), explicit_rdpcm_flag);

        if (explicit_rdpcm_flag)
            BS2_SET(ExplicitRdpcmDirFlag(cIdx), explicit_rdpcm_dir_flag);
            std::ignore = explicit_rdpcm_dir_flag;
    }

    BS2_SET(LastSigCoeffXPrefix(cIdx, log2TrafoSize), LastSignificantCoeffX);
    BS2_SET(LastSigCoeffYPrefix(cIdx, log2TrafoSize), LastSignificantCoeffY);

    if (LastSignificantCoeffX > 3)
        BS2_SET((1 << ((LastSignificantCoeffX >> 1) - 1)) * (2 + (LastSignificantCoeffX & 1))
            + LastSigCoeffXSuffix((Bs16u)LastSignificantCoeffX), LastSignificantCoeffX);


    if (LastSignificantCoeffY > 3)
        BS2_SET((1 << ((LastSignificantCoeffY >> 1) - 1)) * (2 + (LastSignificantCoeffY & 1))
            + LastSigCoeffYSuffix((Bs16u)LastSignificantCoeffY), LastSignificantCoeffY);

    if (2 == scanIdx)
        std::swap(LastSignificantCoeffX, LastSignificantCoeffY);

    bool  signHiddenON =
       !(   cu.transquant_bypass_flag
        || (   cu.PredMode == MODE_INTRA
            && sps.implicit_rdpcm_enabled_flag
            && transform_skip_flag
            && (predModeIntra == 10 || predModeIntra == 26))
        || explicit_rdpcm_flag);
    const Bs8u *pSO, *pSO2 = ScanOrder2[scanIdx][0];

#define ScanOrder(_p0, _p1) pSO[((_p0) << 1) + (_p1)]
#define ScanOrder_2(_p0, _p1) pSO2[((_p0) << 1) + (_p1)]

    lastScanPos = ScanPos2[scanIdx][LastSignificantCoeffX & 3][LastSignificantCoeffY & 3];

    if (log2TrafoSize == 4)
    {
        pSO = ScanOrder2[scanIdx][0];
        lastSubBlock = ScanPos2[scanIdx][LastSignificantCoeffX >> 2][LastSignificantCoeffY >> 2];
    }
    else if (log2TrafoSize == 5)
    {
        pSO = ScanOrder3[scanIdx][0];
        lastSubBlock = ScanPos3[scanIdx][LastSignificantCoeffX >> 2][LastSignificantCoeffY >> 2];
    }
    else if (log2TrafoSize == 3)
    {
        pSO = ScanOrder1[scanIdx][0];
        lastSubBlock = ScanPos1[scanIdx][LastSignificantCoeffX >> 2][LastSignificantCoeffY >> 2];
    }
    else/* if (log2TrafoSize == 2)*/
    {
        pSO = ScanOrder1[scanIdx][0];
        lastSubBlock = 0;
    }

    for (Bs32s i = lastSubBlock; i >= 0; i--)
    {
        bool coeff_abs_level_greater1_flag[16] = {};
        bool coeff_abs_level_greater2_flag[16] = {};
        bool coeff_sign_flag[16] = {};
        bool inferSbDcSigCoeffFlag = false;
        bool signHidden;

        nSC = 0;

        xS = ScanOrder(i, 0);
        yS = ScanOrder(i, 1);

        if ((i < lastSubBlock) && (i > 0))
        {
            Bs16u csbfCtx = 0;

            if (xS < (1 << (log2TrafoSize - 2)) - 1)
                csbfCtx += coded_sub_block_flag[xS + 1][yS];
            if (yS < (1 << (log2TrafoSize - 2)) - 1)
                csbfCtx += coded_sub_block_flag[xS][yS + 1];

            BS2_SET(CodedSubBlockFlag(BS_MIN(csbfCtx, 1) + 2 * !!cIdx), coded_sub_block_flag[xS][yS]);
            inferSbDcSigCoeffFlag = true;
        }
        else
            coded_sub_block_flag[xS][yS] = !(xS || yS)
                || ((xS == (LastSignificantCoeffX >> 2)) && (yS == (LastSignificantCoeffY >> 2)));

        for (Bs32s n = 15; n >= 0; n--)
        {
            xC = (xS << 2) + ScanOrder2[scanIdx][n][0];
            yC = (yS << 2) + ScanOrder2[scanIdx][n][1];

            if (   ((i != lastSubBlock) || n <= (lastScanPos - 1))
                && coded_sub_block_flag[xS][yS]
                && (n > 0 || !inferSbDcSigCoeffFlag))
            {
                Bs16u sigCtx;

                if (sps.transform_skip_context_enabled_flag && (transform_skip_flag || cu.transquant_bypass_flag))
                    sigCtx = (cIdx == 0) ? 42 : 16;
                else if (log2TrafoSize == 2)
                    sigCtx = ctxIdxMap[(yC << 2) + xC];
                else if (xC + yC == 0)
                    sigCtx = 0;
                else
                {
                    Bs16u prevCsbf = 0;
                    Bs16u xP = (xC & 3);
                    Bs16u yP = (yC & 3);

                    if (xS < (1 << (log2TrafoSize - 2)) - 1)
                        prevCsbf += coded_sub_block_flag[xS + 1][yS];
                    if (yS < (1 << (log2TrafoSize - 2)) - 1)
                        prevCsbf += ((Bs16u)coded_sub_block_flag[xS][yS + 1] << 1);

                    if (prevCsbf == 0)
                        sigCtx = (xP + yP == 0) ? 2 : (xP + yP < 3) ? 1: 0;
                    else if (prevCsbf == 1)
                        sigCtx = (yP == 0) ? 2 : (yP == 1) ? 1 : 0;
                    else if (prevCsbf == 2)
                        sigCtx = (xP == 0) ? 2 : (xP == 1) ? 1 : 0;
                    else
                        sigCtx = 2;

                    if (cIdx == 0)
                    {
                        if (xS + yS)
                            sigCtx += 3;

                        if (log2TrafoSize == 3)
                            sigCtx += (scanIdx == 0) ? 9 : 15;
                        else
                            sigCtx += 21;
                    }
                    else
                    {
                        if (log2TrafoSize == 3)
                            sigCtx += 9;
                        else
                            sigCtx += 12;
                    }
                }

                BS2_SET(SigCoeffFlag(cIdx ? 27 + sigCtx : sigCtx), sig_coeff_flag);

                if (sig_coeff_flag)
                {
                    inferSbDcSigCoeffFlag = false;
                    sig_coeff[nSC++] = n;
                }
            }
            else if ((xC == LastSignificantCoeffX && yC == LastSignificantCoeffY)
                || (!((xC & 3) || (yC & 3)) && inferSbDcSigCoeffFlag && coded_sub_block_flag[xS][yS]))
                sig_coeff[nSC++] = n;
        }

        Bs32s firstSigScanPos     = 16;
        Bs32s lastSigScanPos      = -1;
        Bs32s numGreater1Flag     =  0;
        Bs32s lastGreater1ScanPos = -1;

        for (Bs32s ii = 0; ii < nSC; ii++)
        {
            auto n = sig_coeff[ii];

            if (numGreater1Flag < 8)
            {
                BS2_SET(CoeffAbsLevelGreater1Flag(cIdx, i), coeff_abs_level_greater1_flag[n]);

                numGreater1Flag++;

                if (coeff_abs_level_greater1_flag[n] && lastGreater1ScanPos == -1)
                    lastGreater1ScanPos = n;
            }

            if (lastSigScanPos == -1)
                lastSigScanPos = n;
            firstSigScanPos = n;
        }

        signHidden = signHiddenON && (lastSigScanPos - firstSigScanPos > 3);

        if (lastGreater1ScanPos != -1)
        {
            BS2_SET(CoeffAbsLevelGreater2Flag(cIdx), coeff_abs_level_greater2_flag[lastGreater1ScanPos]);
        }

        for (Bs32s ii = 0; ii < nSC; ii++)
        {
            auto n = sig_coeff[ii];

            if (   !pps.sign_data_hiding_enabled_flag
                || !signHidden
                || (n != firstSigScanPos))
            {
                BS2_SET(CoeffSignFlag(), coeff_sign_flag[n]);
            }
        }

        Bs32s numSigCoeff = 0;
        Bs32s sumAbsLevel = 0;

        for (Bs32s ii = 0; ii < nSC; ii++)
        {
            auto n = sig_coeff[ii];
            Bs32s coeff_abs_level_remaining = 0;
            Bs16u baseLevel = 1 + coeff_abs_level_greater1_flag[n] + coeff_abs_level_greater2_flag[n];

            if (baseLevel == ((numSigCoeff < 8) ? ((n == lastGreater1ScanPos) ? 3 : 2) : 1))
            {
                BS2_SET(CoeffAbsLevelRemaining(i, baseLevel, cIdx, cu, tu), coeff_abs_level_remaining);
            }

            if ((report_TCLevels && cIdx == 0) || TLTest(TRACE_COEF))
            {
                xC = (xS << 2) + ScanOrder_2(n, 0);
                yC = (yS << 2) + ScanOrder_2(n, 1);

                auto& TransCoeffLevel = pTCLevels[(yC << log2TrafoSize) + xC];
                TransCoeffLevel = (coeff_abs_level_remaining + baseLevel) * (1 - 2 * coeff_sign_flag[n]);

                if (pps.sign_data_hiding_enabled_flag && signHidden)
                {
                    sumAbsLevel += (coeff_abs_level_remaining + baseLevel);

                    if ((n == firstSigScanPos) && ((sumAbsLevel % 2) == 1))
                        TransCoeffLevel *= -1;
                }

                BS2_TRACE(TransCoeffLevel, TransCoeffLevel);
            }

            numSigCoeff++;
        }
    }

    TLStart(TRACE_COEF);
    if (Trace())
    {
        auto TransCoeffLevel = pTCLevels;
        Bs16u side = (1 << log2TrafoSize);
        Bs16u size = side * side;
        BS2_TRACE_ARR_F(TransCoeffLevel, size, side, "%4i ");
    }
    TLEnd();
}

template <class U>
class ArrayWrap
{
protected:
    U* pBase;
    Bs32u offset;
    Bs32u n;
    std::vector<Bs32u> d;

    inline void DD(U& _Base)
    {
        pBase = &_Base;
        d.push_back(1);
    }

    template<class T, Bs32u N>
    inline void DD(T(&_pBase)[N])
    {
        d.push_back(N);
        DD(_pBase[0]);
    }

public:
    ArrayWrap() { offset = 0; n = 0; pBase = nullptr; }

    template<class T, Bs32u N>
    inline ArrayWrap& operator=(T(&_pBase)[N])
    {
        offset = 0;
        n = 0;
        d.resize(0);
        DD(_pBase);
        return *this;
    }

    inline ArrayWrap& operator[] (Bs32u x)
    {
        if (!n)
            offset = 0;
        offset += x * d[++n];
        return *this;
    }

    inline operator U() { n = 0; return pBase[offset]; }
};

void SDParser::parsePC(CU& cu)
{
    TLAuto tl(*this, TRACE_RESIDUAL);
    auto& sps = *m_cSlice->sps;
    Bs16u NumPredictedPaletteEntries = 0
        , num_signalled_palette_entries = 0
        , CurrentPaletteSize
        , numComps = (ChromaArrayType == 0) ? 1 : 3
        , i, cIdx
        , palette_escape_val_present_flag = 1
        , num_palette_indices = 1
        , palette_transpose_flag = 0
        , log2BlockSize = cu.log2CbSize
        , nCbS = (1 << cu.log2CbSize)
        , xC, yC, xR, yR
        , xcPrev = 0, ycPrev = 0
        //, x0 = cu.x
        //, y0 = cu.y
        , PaletteRun
        , currNumIndices
        , CurrPaletteIndex
        , palette_run_prefix
        , palette_run_suffix
        , runPos, sPos
        , adjustedRefPaletteIndex;
    Bs16s MaxPaletteIndex
        , remainingNumIndices
        , PaletteScanPos
        , PaletteMaxRun;
    bool copy_above_indices_for_final_run_flag = false;
    auto& newPredictorPaletteEntries = CurrentPaletteEntries;
    auto& newPredictorPaletteSize = CurrentPaletteSize;
    ArrayWrap<std::remove_reference<decltype(ScanOrder1[0][0][0])>::type> ScanOrder3;

    std::fill(PalettePredictorEntryReuseFlags.begin(), PalettePredictorEntryReuseFlags.end(), 0);

    for (Bs16u predictorEntryIdx = 0;
        predictorEntryIdx < PredictorPaletteSize
        && NumPredictedPaletteEntries < sps.palette_max_size;
        predictorEntryIdx++)
    {
        Bs16u palette_predictor_run;
        BS2_SET(PalettePredictorRun(), palette_predictor_run);

        if (palette_predictor_run != 1)
        {
            if (palette_predictor_run > 1)
                predictorEntryIdx += palette_predictor_run - 1;
            PalettePredictorEntryReuseFlags[predictorEntryIdx] = 1;

            for (cIdx = 0; cIdx < numComps; cIdx++)
                CurrentPaletteEntries[cIdx][NumPredictedPaletteEntries]
                = PredictorPaletteEntries[cIdx][predictorEntryIdx];

            NumPredictedPaletteEntries++;
        }
        else
            break;
    }

    if (NumPredictedPaletteEntries < sps.palette_max_size)
        BS2_SET(NumSignalledPaletteEntries(), num_signalled_palette_entries);

    CurrentPaletteSize = NumPredictedPaletteEntries + num_signalled_palette_entries;

    for (cIdx = 0; cIdx < numComps; cIdx++)
        for (i = 0; i < num_signalled_palette_entries; i++)
            BS2_TRACE(CurrentPaletteEntries[cIdx][NumPredictedPaletteEntries + i]
                = NewPaletteEntries(cIdx), new_palette_entries[cIdx][i]);

    if (CurrentPaletteSize != 0)
        BS2_SET(PaletteEscapeValPresentFlag(), palette_escape_val_present_flag);

    MaxPaletteIndex = CurrentPaletteSize - 1 + palette_escape_val_present_flag;

    if (MaxPaletteIndex > 0)
    {
        BS2_SET(NumPaletteIndices(MaxPaletteIndex), num_palette_indices);

        std::fill_n(Info::PaletteIndexIdc, num_palette_indices, 0);

        Bs16u adjust = 0;

        for (i = 0; i < num_palette_indices; i++)
        {
            if (MaxPaletteIndex - adjust > 0)
                BS2_TRACE(Info::PaletteIndexIdc[i] = CABAC::PaletteIndexIdc(MaxPaletteIndex), palette_index_idc);

            adjust = 1;
        }
        BS2_TRACE_ARR(Info::PaletteIndexIdc, num_palette_indices, 32);
        BS2_SET(CopyAboveIndicesForFinalRunFlag(), copy_above_indices_for_final_run_flag);
        BS2_SET(PaletteTransposeFlag(), palette_transpose_flag);
    }
    else
        std::fill_n(Info::PaletteIndexIdc, num_palette_indices, 0);

    if (palette_escape_val_present_flag)
    {
        parseDQP(cu);
        if (!cu.transquant_bypass_flag)
            parseCQPO(cu);
    }

    remainingNumIndices = num_palette_indices;
    PaletteScanPos = 0;
    CurrPaletteIndex = 0;

    switch (log2BlockSize)
    {
    case 5: ScanOrder3 = BS_HEVC2::ScanTraverse5; break;
    case 4: ScanOrder3 = BS_HEVC2::ScanTraverse4; break;
    case 3: ScanOrder3 = BS_HEVC2::ScanOrder3[3]; break;
    case 2: ScanOrder3 = BS_HEVC2::ScanOrder2[3]; break;
    case 1: ScanOrder3 = BS_HEVC2::ScanOrder1[3]; break;
    default: throw InvalidSyntax();
    }

    while (PaletteScanPos < nCbS * nCbS)
    {
        xC = /*x0 + */ScanOrder3[PaletteScanPos][0];
        yC = /*y0 + */ScanOrder3[PaletteScanPos][1];
        if (PaletteScanPos > 0)
        {
            xcPrev = /*x0 + */ScanOrder3[PaletteScanPos - 1][0];
            ycPrev = /*y0 + */ScanOrder3[PaletteScanPos - 1][1];
        }

        PaletteRun = nCbS * nCbS - PaletteScanPos - 1;
        CopyAboveIndicesFlag[xC][yC] = 0;

        if (MaxPaletteIndex > 0)
        {
            if (PaletteScanPos >= nCbS && CopyAboveIndicesFlag[xcPrev][ycPrev] == 0)
            {
                if (remainingNumIndices > 0 && PaletteScanPos < nCbS * nCbS - 1)
                    BS2_TRACE(CopyAboveIndicesFlag[xC][yC] = CABAC::CopyAbovePaletteIndicesFlag(),
                        copy_above_palette_indices_flag)
                else if (PaletteScanPos == nCbS * nCbS - 1 && remainingNumIndices > 0)
                    CopyAboveIndicesFlag[xC][yC] = 0;
                else
                    CopyAboveIndicesFlag[xC][yC] = 1;
            }
        }

        if (CopyAboveIndicesFlag[xC][yC] == 0)
        {
            currNumIndices = num_palette_indices - remainingNumIndices;
            CurrPaletteIndex = Info::PaletteIndexIdc[currNumIndices];
        }

        if (MaxPaletteIndex > 0)
        {
            if (CopyAboveIndicesFlag[xC][yC] == 0)
                remainingNumIndices -= 1;

            PaletteMaxRun = nCbS * nCbS - PaletteScanPos - 1 - remainingNumIndices - copy_above_indices_for_final_run_flag;

            if (remainingNumIndices > 0 || CopyAboveIndicesFlag[xC][yC] != copy_above_indices_for_final_run_flag)
            {
                if (PaletteMaxRun > 0)
                {
                    BS2_SET(PaletteRunPrefix(PaletteMaxRun, CopyAboveIndicesFlag[xC][yC], CurrPaletteIndex), palette_run_prefix);

                    if ((palette_run_prefix > 1))
                    {
                        PaletteRun = 1 << (palette_run_prefix - 1);

                        if ((PaletteMaxRun != PaletteRun))
                        {
                            BS2_SET(PaletteRunSuffix(PaletteMaxRun, PaletteRun), palette_run_suffix);
                            PaletteRun += palette_run_suffix;
                        }
                    }
                    else
                        PaletteRun = palette_run_prefix;
                }
            }
            else
                PaletteMaxRun += copy_above_indices_for_final_run_flag;

            PaletteRun = BS_MIN(PaletteRun, PaletteMaxRun);
        }

        if (CopyAboveIndicesFlag[xC][yC] == 0)
        {
            adjustedRefPaletteIndex = MaxPaletteIndex + 1;

            if (PaletteScanPos > 0)
            {
                if (CopyAboveIndicesFlag[xcPrev][ycPrev] == 0)
                    adjustedRefPaletteIndex = PaletteIndexMap[xcPrev][ycPrev];
                else
                    adjustedRefPaletteIndex = PaletteIndexMap[xC][yC - 1];
            }

            if (CurrPaletteIndex >= adjustedRefPaletteIndex)
                CurrPaletteIndex++;
        }

        runPos = 0;
        while (runPos <= PaletteRun)
        {
            xR = /*x0 + */ScanOrder3[PaletteScanPos][0];
            yR = /*y0 + */ScanOrder3[PaletteScanPos][1];

            if (CopyAboveIndicesFlag[xC][yC] == 0)
            {
                CopyAboveIndicesFlag[xR][yR] = 0;
                PaletteIndexMap[xR][yR] = CurrPaletteIndex;
            }
            else
            {
                CopyAboveIndicesFlag[xR][yR] = 1;
                PaletteIndexMap[xR][yR] = PaletteIndexMap[xR][yR - 1];
            }
            runPos++;
            PaletteScanPos++;
        }
    }

    if (palette_escape_val_present_flag)
    {
        for (cIdx = 0; cIdx < numComps; cIdx++)
        {
            for (sPos = 0; sPos < nCbS * nCbS; sPos++)
            {
                xC = /*x0 + */ScanOrder3[sPos][0];
                yC = /*y0 + */ScanOrder3[sPos][1];

                if (PaletteIndexMap[xC][yC] == MaxPaletteIndex)
                {
                    if (cIdx == 0
                        || (xC % 2 == 0 && yC % 2 == 0 && ChromaArrayType == 1)
                        || (xC % 2 == 0 && !palette_transpose_flag && ChromaArrayType == 2)
                        || (yC % 2 == 0 && palette_transpose_flag && ChromaArrayType == 2)
                        || ChromaArrayType == 3)
                    {
                        BS2_TRACE(PaletteEscapeVal(cIdx, cu.transquant_bypass_flag), palette_escape_val);
                        //PaletteEscapeVal[cIdx][xC][yC] = palette_escape_val;
                    }
                }
            }
        }
    }

    TLStart(TRACE_COEF);
    if (Trace())
    {
#ifdef __BS_TRACE__
        const char* fmt = (CurrentPaletteSize > 10) ? "%2i " : "%i ";
#endif
        BS2_TRACE_ARR_F(CurrentPaletteEntries[0], CurrentPaletteSize, 0, "%5i ");
        BS2_TRACE_ARR_F(CurrentPaletteEntries[1], CurrentPaletteSize, 0, "%5i ");
        BS2_TRACE_ARR_F(CurrentPaletteEntries[2], CurrentPaletteSize, 0, "%5i ");

        if (palette_transpose_flag)
        {
            for (i = 0; i < nCbS; i++)
                BS2_TRACE_ARR_F(PaletteIndexMap[i], nCbS, 0, fmt);
        }
        else
        {
            for (i = 0; i < nCbS; i++)
                BS2_TRACE_ARR_VF(PaletteIndexMap[_i][i], PaletteIndexMap[i], nCbS, 0, fmt);
        }
    }
    TLEnd();

    newPredictorPaletteSize = BS_MIN(newPredictorPaletteSize, PaletteMaxPredictorSize);

    for (i = 0; i < PredictorPaletteSize && newPredictorPaletteSize < PaletteMaxPredictorSize; i++)
    {
        if (!PalettePredictorEntryReuseFlags[i])
        {
            for (cIdx = 0; cIdx < numComps; cIdx++)
                newPredictorPaletteEntries[cIdx][newPredictorPaletteSize] = PredictorPaletteEntries[cIdx][i];
            newPredictorPaletteSize++;
        }
    }

    for (cIdx = 0; cIdx < numComps; cIdx++)
    {
        PredictorPaletteEntries[cIdx].swap(newPredictorPaletteEntries[cIdx]);
        PredictorPaletteSize = newPredictorPaletteSize;
    }
}
