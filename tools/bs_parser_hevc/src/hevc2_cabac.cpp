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

using namespace BS_HEVC2;
using namespace BsReader2;

#define Clip3(_min, _max, _x) BS_MIN(BS_MAX(_min, _x), _max)

void CABAC::InitCtx()
{
    Slice& slice   = *m_cSlice;
    auto& pps = *slice.pps;
    auto& sps = *slice.sps;
    Bs16u initType = 0;
    Bs16s Clip3_0_51_SliceQpY = 26 + slice.pps->init_qp_minus26 + slice.qp_delta;
    Bs8u* state = m_ctxState;
    Bs16s preCtxState;
    Bs16u numComps = (ChromaArrayType == 0) ? 1 : 3;

    Clip3_0_51_SliceQpY = Clip3(0, 51, Clip3_0_51_SliceQpY);

    memset(StatCoeff, 0, sizeof(StatCoeff));

    if (slice.type == I)
        initType = 0;
    else if (slice.type == P)
        initType = slice.cabac_init_flag ? 2 : 1;
    else
        initType = slice.cabac_init_flag ? 1 : 2;

    for (Bs16s initValue : BS_HEVC::CtxInitTbl[initType])
    {
        preCtxState = ((((initValue >> 4) * 5 - 45) * Clip3_0_51_SliceQpY) >> 4) + ((initValue & 15) << 3) - 16;
        preCtxState = Clip3(1, 126, preCtxState);

        if (preCtxState <= 63)
            *state = ((63 - preCtxState) << 1);
        else
            *state = (((preCtxState - 64) << 1) | 1);
        state++;
    }

    Bs32u sz = (sps.palette_max_size + PaletteMaxPredictorSize);
    PredictorPaletteEntries[0].resize(sz);
    PredictorPaletteEntries[1].resize(sz);
    PredictorPaletteEntries[2].resize(sz);
    PredictorPaletteSize = 0;

    if (pps.palette_predictor_initializer_present_flag )
    {
        for (Bs16u cIdx = 0; cIdx < numComps && pps.num_palette_predictor_initializer; cIdx++)
        {
            PredictorPaletteSize = pps.num_palette_predictor_initializer;
            memmove(&PredictorPaletteEntries[cIdx][0]
                , pps.palette_predictor_initializers[cIdx]
                , pps.num_palette_predictor_initializer * sizeof(pps.palette_predictor_initializers[cIdx][0]));
        }
    }
    else if (sps.palette_predictor_initializer_present_flag)
    {
        for (Bs16u cIdx = 0; cIdx < numComps && sps.num_palette_predictor_initializer; cIdx++)
        {
            PredictorPaletteSize = sps.num_palette_predictor_initializer;
            memmove(&PredictorPaletteEntries[cIdx][0]
                , sps.palette_predictor_initializers[cIdx]
                , sps.num_palette_predictor_initializer * sizeof(sps.palette_predictor_initializers[cIdx][0]));
        }
    }

}

Bs8u CABAC::SaoTypeIdxLuma()
{
    //TR cMax = 2, cRiceParam = 0
    //1+0, bypass

    if (!DD(BS_HEVC::SAO_TYPE_IDX_LUMA))
        return 0;

    BinCount++;
    return DecodeBypass() + 1;
}

Bs8u CABAC::SaoOffsetAbs(Bs16u bitDepth)
{
    //TR cMax = (1 << (Min(bitDepth, 10) - 5)) - 1, cRiceParam = 0
    //bypass bypass bypass bypass bypass na
    Bs16u cMax = (1 << (BS_MIN(bitDepth, 10) - 5)) - 1;
    Bs8u binIdx = 0;

    while (binIdx < cMax)
    {
        if (!DecodeBypass())
        {
            BinCount += binIdx + 1;
            return binIdx;
        }

        binIdx++;
    }

    BinCount += binIdx;

    return binIdx;
}

Bs8u CABAC::SaoBandPosition()
{
    //FL cMax = 31
    //bypass bypass bypass bypass bypass bypass
    Bs8u b;

    b  = (DecodeBypass() << 4);
    b |= (DecodeBypass() << 3);
    b |= (DecodeBypass() << 2);
    b |= (DecodeBypass() << 1);
    b |=  DecodeBypass();

    BinCount += 5;

    return b;
}

Bs8u CABAC::SaoEoClassLuma()
{
    //FL cMax = 3
    //bypass bypass bypass na na na
    Bs8u b;

    b  = (DecodeBypass() << 1);
    b |=  DecodeBypass();

    BinCount += 2;

    return b;
}

bool CABAC::SplitCuFlag(Bs16s x0, Bs16s y0)
{
    //FL cMax = 1
    //9.3.4.2.2
    Bs16u ctxInc = 0;
    bool availableL = AvailableZs(x0, y0, x0 - 1, y0);
    bool availableA = AvailableZs(x0, y0, x0, y0 - 1);

    if (availableL || availableA)
    {
      CU* c0 = GetCU(x0, y0);
      if (!c0)
          throw InvalidSyntax();

      CU* c1 = nullptr;
      if (availableL)
      {
          c1 = GetCU(x0 - 1, y0);
          if (!c1)
              throw InvalidSyntax();
      }

      CU* c2 = nullptr;
      if (availableA)
      {
          c2 = GetCU(x0, y0 - 1);
          if (!c2)
              throw InvalidSyntax();
      }

        ctxInc =
              (availableL && (c1->log2CbSize < c0->log2CbSize))
            + (availableA && (c2->log2CbSize < c0->log2CbSize));
    }

    return DD(BS_HEVC::SPLIT_CU_FLAG, ctxInc);
}

bool CABAC::CuSkipFlag(Bs16s x0, Bs16s y0)
{
    //FL cMax = 1
    //9.3.4.2.2
    bool availableL = AvailableZs(x0, y0, x0 - 1, y0);
    bool availableA = AvailableZs(x0, y0, x0, y0 - 1);

    CU* c1 = nullptr;
    if (availableL)
    {
        c1 = GetCU(x0 - 1, y0);
        if (!c1)
            throw InvalidSyntax();
    }

    CU* c2 = nullptr;
    if (availableA)
    {
        c2 = GetCU(x0, y0 - 1);
        if (!c2)
            throw InvalidSyntax();
    }

    Bs16u ctxInc =
          (availableL && (c1->PredMode == MODE_SKIP))
        + (availableA && (c2->PredMode == MODE_SKIP));

    return DD(BS_HEVC::CU_SKIP_FLAG, ctxInc);
}

Bs8u CABAC::PartMode(bool intra, Bs16u log2CbSize)
{
    //9.3.3.6 (xCb, yCb ) = (x0, y0), log2CbSize
    // 0 1 (3 - (log2CbSize == MinCbLog2SizeY)) bypass
    bool b = DD(BS_HEVC::PART_MODE);

    if (intra)
        return  b ? PART_2Nx2N : PART_NxN;

    if (b)
        return PART_2Nx2N;

    b = DD(BS_HEVC::PART_MODE, 1);

    if (log2CbSize == MinCbLog2SizeY)
    {
        if (b)
            return PART_2NxN;

        if (log2CbSize == 3)
            return PART_Nx2N;

        b = DD(BS_HEVC::PART_MODE, 2);

        return b ? PART_Nx2N : PART_NxN;
    }

    if (!m_cSlice->sps->amp_enabled_flag)
        return b ? PART_2NxN : PART_Nx2N;

    bool v = b;

    b = DD(BS_HEVC::PART_MODE, 3);

    if (b)
        return v ? PART_2NxN : PART_Nx2N;

    b = DB();

    if (v)
        return b ? PART_2NxnD : PART_2NxnU;

    return b ? PART_nRx2N : PART_nLx2N;
}

Bs8u CABAC::MpmIdx()
{
    //TR cMax = 2, cRiceParam = 0
    //bypass, bypass
    BinCount++;
    if (!DecodeBypass())
        return 0;

    BinCount++;
    return DecodeBypass() + 1;
}

Bs8u CABAC::IntraChromaPredMode()
{
    //9.3.3.7
    //0 bypass bypass na na na
    if (!DD(BS_HEVC::INTRA_CHROMA_PRED_MODE))
        return 4;

    BinCount += 2;
    Bs8u b = (DecodeBypass() << 1);

    return b | DecodeBypass();
}

Bs8u CABAC::MergeIdx()
{
    //TR cMax = MaxNumMergeCand - 1, cRiceParam = 0
    //0 bypass bypass bypass na na
    Bs32s cMax = m_cSlice->MaxNumMergeCand - 1;
    Bs8u  binIdx = 0;

    if (cMax > 0)
    {
        if (!DD(BS_HEVC::MERGE_IDX))
            return 0;
        binIdx++;
    }

    while (binIdx < cMax)
    {
        if (!DecodeBypass())
        {
            BinCount += binIdx + 1;
            return binIdx;
        }

        binIdx++;
    }

    BinCount += binIdx;

    return binIdx;
}

Bs8u CABAC::InterPredIdc(Bs16u nPbW, Bs16u nPbH, Bs16u CtDepth)
{
    //9.3.3.8 nPbW, nPbH
    //( nPbW + nPbH ) != 12 ? CtDepth[x0][y0] : 4 4 na na na na

    if (nPbW + nPbH != 12)
    {
        if (DD(BS_HEVC::INTER_PRED_IDC, CtDepth))
            return PRED_BI;
    }

    if (DD(BS_HEVC::INTER_PRED_IDC, 4))
        return PRED_L1;

    return PRED_L0;
}

Bs8u CABAC::RefIdxL0()
{
    //TR cMax = num_ref_idx_l0_active_minus1, cRiceParam = 0
    //0 1 bypass bypass bypass bypass
    Bs32s cMax = m_cSlice->num_ref_idx_l0_active - 1;
    Bs8u  binIdx = 0;

    if (cMax > 0)
    {
        if (!DD(BS_HEVC::REF_IDX_LX))
            return 0;
        binIdx++;
    }

    if (binIdx < cMax)
    {
        if (!DD(BS_HEVC::REF_IDX_LX, 1))
            return binIdx;
        binIdx++;
    }

    while (binIdx < cMax)
    {
        if (!DecodeBypass())
        {
            BinCount += binIdx;
            return binIdx;
        }
        binIdx++;
    }

    BinCount += binIdx - 1;

    return binIdx;
}

Bs8u CABAC::RefIdxL1()
{
    //TR cMax = num_ref_idx_l1_active_minus1, cRiceParam = 0
    //0 1 bypass bypass bypass bypass
    Bs32s cMax = m_cSlice->num_ref_idx_l1_active - 1;
    Bs8u  binIdx = 0;

    if (cMax > 0)
    {
        if (!DD(BS_HEVC::REF_IDX_LX))
            return 0;
        binIdx++;
    }

    if (binIdx < cMax)
    {
        if (!DD(BS_HEVC::REF_IDX_LX, 1))
            return binIdx;
        binIdx++;
    }

    while (binIdx < cMax)
    {
        if (!DecodeBypass())
        {
            BinCount += binIdx;
            return binIdx;
        }
        binIdx++;
    }

    BinCount += binIdx - 1;

    return binIdx;
}

Bs16u CABAC::CuQpDeltaAbs()
{
    //9.3.3.9
    //0 1 1 1 1 bypass
    Bs16u k = 0, v0 = 0, v1 = 0, b;

    while (v0 < 5)
    {
        if (!DecodeDecision(CtxState(BS_HEVC::CU_QP_DELTA_ABS, !!v0)))
        {
            BinCount += v0;
            return v0;
        }
        v0++;
    }

    while (DecodeBypass())
    {
        v0 += (1 << k);
        k++;
    }

    BinCount += k * 2 + 1;

    while (k--)
    {
        b = DecodeBypass();
        v1 |= (b << k);
    }

    return (v0 + v1);
}

Bs8u CABAC::CuChromaQpOffsetIdx()
{
    //TR cMax = chroma_qp_offset_list_len_minus1, cRiceParam = 0
    //0 0 0 0 0 na
    Bs32s cMax = m_cSlice->pps->chroma_qp_offset_list_len_minus1;
    Bs8u  binIdx = 0;

    while (binIdx < cMax)
    {
        if (!DecodeDecision(CtxState(BS_HEVC::CU_CHROMA_QP_OFFSET_IDX)))
        {
            BinCount += binIdx + 1;
            return binIdx;
        }

        binIdx++;
    }

    BinCount += binIdx;

    return binIdx;
}

Bs8u CABAC::Log2ResScaleAbsPlus1(Bs16u c)
{
    //TR cMax = 4, cRiceParam = 0
    //4 * c + 0, 4 * c + 1, 4 * c + 2, 4 * c + 3, na, na
    Bs8u  binIdx = 0;

    while (binIdx < 4)
    {
        if (!DecodeDecision(CtxState(BS_HEVC::LOG2_RES_SCALE_ABS_PLUS1, 4 * c + binIdx)))
        {
            BinCount += binIdx + 1;
            return binIdx;
        }
        binIdx++;
    }

    BinCount += binIdx;

    return binIdx;
}

Bs8u CABAC::LastSigCoeffXPrefix(Bs16u cIdx, Bs16u log2TrafoSize)
{
    //TR cMax = ( log2TrafoSize << 1 ) - 1, cRiceParam = 0
    //0..17 (clause 9.3.4.2.3)
    Bs32u cMax = (log2TrafoSize << 1) - 1;
    Bs32u binIdx = 0;
    Bs16u ctxOffset;
    Bs16u ctxShift;

    m_g1I = -1;
    m_rI  = -1;

    if (cIdx == 0)
    {
        ctxOffset = 3 * (log2TrafoSize - 2) + ((log2TrafoSize - 1) >> 2);
        ctxShift  = ((log2TrafoSize + 1) >> 2);
    }
    else
    {
        ctxOffset = 15;
        ctxShift  = log2TrafoSize - 2;
    }

    while (binIdx < cMax)
    {
        if (!DecodeDecision(CtxState(BS_HEVC::LAST_SIG_COEFF_X_PREFIX, (binIdx >> ctxShift) + ctxOffset)))
        {
            BinCount += binIdx + 1;
            return binIdx;
        }

        binIdx++;
    }

    BinCount += binIdx;

    return binIdx;
}

Bs8u CABAC::LastSigCoeffYPrefix(Bs16u cIdx, Bs16u log2TrafoSize)
{
    //TR cMax = ( log2TrafoSize << 1 ) - 1, cRiceParam = 0
    //0..17 (clause 9.3.4.2.3)
    Bs32u cMax = (log2TrafoSize << 1) - 1;
    Bs32u binIdx = 0;
    Bs16u ctxOffset;
    Bs16u ctxShift;

    if (cIdx == 0)
    {
        ctxOffset = 3 * (log2TrafoSize - 2) + ((log2TrafoSize - 1) >> 2);
        ctxShift  = ((log2TrafoSize + 1) >> 2);
    }
    else
    {
        ctxOffset = 15;
        ctxShift  = log2TrafoSize - 2;
    }

    while (binIdx < cMax)
    {
        if (!DecodeDecision(CtxState(BS_HEVC::LAST_SIG_COEFF_Y_PREFIX, (binIdx >> ctxShift) + ctxOffset)))
        {
            BinCount += binIdx + 1;
            return binIdx;
        }

        binIdx++;
    }

    BinCount += binIdx;

    return binIdx;
}

Bs32u CABAC::LastSigCoeffXSuffix(Bs16u prefix)
{
    //FL cMax = ( 1 << ( ( last_sig_coeff_x_prefix >> 1 ) - 1 ) - 1 )
    //bypass bypass bypass bypass bypass bypass
    Bs32u b = 0;

    prefix = CeilLog2(1 << ((prefix >> 1) - 1));

    BinCount += prefix;

    while (prefix--)
        b = (b << 1) | DecodeBypass();

    return b;
}

bool CABAC::CoeffAbsLevelGreater1Flag(Bs16u cIdx, Bs16u i)
{
    //FL cMax = 1
    //9.3.4.2.6
    auto& greater1Ctx = lastGreater1Ctx;
    Bs16s ctxInc;

    if (m_g1I != i)
    {
        if (i == 0 || cIdx > 0)
            ctxSet = 0;
        else
            ctxSet = 2;

        if (m_g1I < 0)
        {
            lastGreater1Ctx = 1;
        }
        else if (lastGreater1Ctx > 0)
        {
            if (lastGreater1Flag)
                lastGreater1Ctx = 0;
            else
                lastGreater1Ctx++;
        }

        if (lastGreater1Ctx == 0)
            ctxSet++;

        greater1Ctx = 1;
    }
    else
    {
        if (greater1Ctx > 0)
        {
            if (lastGreater1Flag)
                greater1Ctx = 0;
            else
                greater1Ctx++;
        }
    }

    m_g1I = i;
    ctxInc = (ctxSet * 4) + BS_MIN(3, greater1Ctx);

    if (cIdx > 0)
        ctxInc += 16;

    lastGreater1Flag = DD(BS_HEVC::COEFF_ABS_LEVEL_GREATER1_FLAG, ctxInc);

    return lastGreater1Flag;
}

Bs32u CABAC::CoeffAbsLevelRemaining(Bs16u i, Bs16u baseLevel, Bs16u cIdx, CU& cu, TU& tu)
{
    //9.3.3.10 current sub-block scan index i, baseLevel
    //bypass bypass bypass bypass bypass bypass
    auto& sps = *m_cSlice->sps;
    auto& cAbsLevel  = cLastAbsLevel;
    auto& cRiceParam = cLastRiceParam;
    Bs16u initRiceValue, sbType = 0;
    Bs32u b = 0, cMax, k, v0, v1;

    if (sps.persistent_rice_adaptation_enabled_flag)
    {
        if (   (tu.transform_skip_flag & (1 << cIdx)) == 0
            && cu.transquant_bypass_flag == 0)
        {
            sbType = 2 * !cIdx;
        }
        else
        {
            sbType = 2 * !cIdx + 1;
        }

        initRiceValue = StatCoeff[sbType] / 4;
    }
    else
    {
        initRiceValue = 0;
    }

    if (i != m_rI)
    {
        cLastAbsLevel  = 0;
        cLastRiceParam = initRiceValue;
    }

    cRiceParam = cLastRiceParam + (cLastAbsLevel > (3u * (1u << cLastRiceParam)));

    if (sps.persistent_rice_adaptation_enabled_flag == 0)
        cRiceParam = BS_MIN(cRiceParam, 4);

    cMax = 4 << cRiceParam;

    while (b < 4)
    {
        if (!DecodeBypass())
        {
            BinCount++;
            break;
        }

        b++;
    }

    BinCount += b;

    if (b < 4)
    {
        for (Bs16u c = 0; c < cRiceParam; c++)
            b = ((b << 1) | DecodeBypass());

        BinCount += cRiceParam;
    }
    else if (sps.persistent_rice_adaptation_enabled_flag == 0)
    {
        b = EGkBypass(cRiceParam + 1) + cMax;
    }
    else
    {
        //Limited EGk
        Bs16u log2TransformRange = (cIdx == 0) ? BS_MAX(15, BitDepthY + 6) : BS_MAX(15, BitDepthC + 6);
        Bs16u maxPrefixExtensionLength = 28 - log2TransformRange;

        k = cRiceParam + 1;
        b = 0;
        v0 = 0;
        v1 = 0;

        while (b < maxPrefixExtensionLength && DecodeBypass())
            b++;

        v0 = (((1 << b) - 1) << k);

        if (b == maxPrefixExtensionLength)
        {
            k = log2TransformRange;
            BinCount += b + k;
        }
        else
        {
            k += b;
            BinCount += b + k + 1;
        }

        while (k--)
        {
            b = DecodeBypass();
            v1 |= (b << k);
        }

        b = v0 + v1 + cMax;
    }

    if (sps.persistent_rice_adaptation_enabled_flag && i != m_rI)
    {
        if (b >= (3u << (StatCoeff[sbType] / 4)))
            StatCoeff[sbType]++;
        else if (2 * b < (1u << (StatCoeff[sbType] / 4)) && StatCoeff[sbType] > 0)
            StatCoeff[sbType]--;
    }

    m_rI = i;
    cAbsLevel = baseLevel + b;

    return b;
}

Bs32u CABAC::EGkBypass(Bs32u k)
{
    //EGk
    //bypass bypass bypass bypass bypass bypass
    Bs32u v0 = 0, v1 = 0, b;

    while (DecodeBypass())
    {
        v0 |= (1 << k);
        k++;
        BinCount++;
    }

    BinCount += k;

    while (k--)
    {
        b = DecodeBypass();
        v1 |= (b << k);
    }

    return (v0 + v1);
}

Bs16u CABAC::NewPaletteEntries(Bs16u cIdx)
{
    //FL cMax = cIdx == 0 ? ((1 << BitDepthY) - 1) : ((1 << BitDepthC) - 1)
    //bypass bypass bypass bypass bypass bypass
    Bs16u b = 0;
    Bs32u n = (cIdx == 0) ? BitDepthY : BitDepthC;

    BinCount += n;

    while (n--)
        b = (b << 1) | DecodeBypass();

    return b;
}

Bs16u CABAC::NumPaletteIndices(Bs16u MaxPaletteIndex)
{
    //9.3.3.14
    //bypass bypass bypass bypass bypass bypass
    Bs16u cRiceParam = 3 + ((MaxPaletteIndex + 1) >> 3);
    Bs16u cMax = 4 << cRiceParam;
    Bs16u prefixVal = 0;

    m_paletteIdxCnt = 0;

    while (prefixVal < 4 && DecodeBypass())
    {
        prefixVal++;
        BinCount++;
    }

    if (prefixVal == 4)
        return EGkBypass(cRiceParam + 1) + cMax + 1;

    BinCount += cRiceParam;

    while (cRiceParam--)
        prefixVal = (prefixVal << 1) | DecodeBypass();

    return prefixVal + 1;
}

Bs32u CABAC::TBBypass(Bs32u cMax)
{
    //TB
    //bypass bypass bypass bypass bypass bypass
    Bs32u n = cMax + 1;
    Bs32u k = (Bs16u)FloorLog2(n);
    Bs32u u = (1 << (k + 1)) - n;
    Bs32u v = 0;

    BinCount += k;

    while (k--)
        v = (v << 1) | DecodeBypass();

    if (v < u)
        return v;

    BinCount++;
    v = (v << 1) | DecodeBypass();

    return (v - u);
}

Bs16u CABAC::PaletteRunPrefix(Bs16u PaletteMaxRun, bool copy_above_palette_indices_flag, Bs16u palette_index_idc)
{
    //TR cMax = Floor(Log2(PaletteMaxRun)) + 1, cRiceParam = 0
    //0..7 (clause 9.3.4.2.8)
    Bs16u cMax = FloorLog2(PaletteMaxRun) + 1;
    Bs16u v = 0;
    auto NextBin = [&](Bs16u ctxInc) -> bool
    {
        if (!DD(BS_HEVC::PALETTE_RUN_PREFIX, ctxInc))
            return false;
        return (++v < cMax);
    };

    if (copy_above_palette_indices_flag)
    {
        //5 6 6 7 7 bypass
        if (!NextBin(5)) return v;
        if (!NextBin(6)) return v;
        if (!NextBin(6)) return v;
        if (!NextBin(7)) return v;
        if (!NextBin(7)) return v;
    }
    else
    {
        //(0,1,2) 3 3 4 4 bypass
        if (!NextBin((palette_index_idc < 1) ? 0 : ((palette_index_idc < 3) ? 1 : 2)))
            return v;
        if (!NextBin(3)) return v;
        if (!NextBin(3)) return v;
        if (!NextBin(4)) return v;
        if (!NextBin(4)) return v;
    }

    while (DecodeBypass() && (++v < cMax));

    return v;
}

Bs16u CABAC::PaletteEscapeVal(Bs16u cIdx, bool cu_transquant_bypass_flag)
{
    //9.3.3.12 cIdx, cu_transquant_bypass_flag
    //bypass bypass bypass bypass bypass bypass

    if (cu_transquant_bypass_flag)
    {
        //FL ( 1 << bitdepth ) - 1
        Bs16u bd = (cIdx == 0) ? BitDepthY : BitDepthC;
        Bs16u v = 0;

        BinCount += bd;

        while (bd--)
            v = (v << 1) | DecodeBypass();

        return v;
    }

    return (Bs16u)EGkBypass(3);
}
