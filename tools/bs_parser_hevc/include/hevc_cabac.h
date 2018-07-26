// Copyright (c) 2018 Intel Corporation
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

#ifndef __HEVC_CABAC_H
#define __HEVC_CABAC_H

#include "bs_reader.h"
#include "hevc_struct.h"
#include <vector>
#include <list>
#include <bitset>
#include <algorithm>

namespace BS_HEVC{

enum SE{
      SAO_MERGE_LEFT_FLAG
    , SAO_MERGE_UP_FLAG = SAO_MERGE_LEFT_FLAG
    , SAO_TYPE_IDX_LUMA
    , SAO_TYPE_IDX_CHROMA = SAO_TYPE_IDX_LUMA
    , SPLIT_CU_FLAG
    , CU_TRANSQUANT_BYPASS_FLAG
    , CU_SKIP_FLAG
    , PRED_MODE_FLAG
    , PART_MODE
    , PREV_INTRA_LUMA_PRED_FLAG
    , INTRA_CHROMA_PRED_MODE
    , RQT_ROOT_CBF
    , MERGE_FLAG
    , MERGE_IDX
    , INTER_PRED_IDC
    , REF_IDX_LX
    , MVP_LX_FLAG
    , SPLIT_TRANSFORM_FLAG
    , CBF_LUMA
    , CBF_CX
    , ABS_MVD_GREATER0_FLAG
    , ABS_MVD_GREATER1_FLAG
    , CU_QP_DELTA_ABS
    , TRANSFORM_SKIP_FLAG0
    , TRANSFORM_SKIP_FLAG1
    , TRANSFORM_SKIP_FLAG2 = TRANSFORM_SKIP_FLAG1
    , LAST_SIG_COEFF_X_PREFIX
    , LAST_SIG_COEFF_Y_PREFIX
    , CODED_SUB_BLOCK_FLAG
    , SIG_COEFF_FLAG
    , COEFF_ABS_LEVEL_GREATER1_FLAG
    , COEFF_ABS_LEVEL_GREATER2_FLAG
    , CU_CHROMA_QP_OFFSET_FLAG
    , CU_CHROMA_QP_OFFSET_IDX
    , LOG2_RES_SCALE_ABS_PLUS1
    , RES_SCALE_SIGN_FLAG
    , EXPLICIT_RDPCM_FLAG
    , EXPLICIT_RDPCM_DIR_FLAG
    , PALETTE_MODE_FLAG
    , TU_RESIDUAL_ACT_FLAG
    , PALETTE_RUN_PREFIX
    , COPY_ABOVE_PALETTE_INDICES_FLAG
    , COPY_ABOVE_INDICES_FOR_FINAL_RUN_FLAG
    , PALETTE_TRANSPOSE_FLAG
    , num_SE
    , END_OF_SLICE_SEGMENT_FLAG = num_SE
    , END_OF_SUB_STREAM_ONE_BIT
    , SAO_OFFSET_ABS
    , SAO_OFFSET_SIGN
    , SAO_BAND_POSITION
    , SAO_EO_CLASS_LUMA
    , SAO_EO_CLASS_CHROMA = SAO_EO_CLASS_LUMA
    , MPM_IDX
    , REM_INTRA_LUMA_PRED_MODE
    , ABS_MVD_MINUS2
    , MVD_SIGN_FLAG
    , CU_QP_DELTA_SIGN_FLAG
    , LAST_SIG_COEFF_X_SUFFIX
    , LAST_SIG_COEFF_Y_SUFFIX
    , COEFF_ABS_LEVEL_REMAINING
    , COEFF_SIGN_FLAG
    , PCM_FLAG
    , num_SE_full
};

enum PRED_MODE{
      MODE_INTER = 0
    , MODE_INTRA
    , MODE_SKIP
    , num_PRED_MODE
};

enum PART_MODE{
      PART_2Nx2N
    , PART_2NxN
    , PART_Nx2N
    , PART_NxN
    , PART_2NxnU
    , PART_2NxnD
    , PART_nLx2N
    , PART_nRx2N
    , num_PART_MODE
};

enum PRED_IDC{
      PRED_L0
    , PRED_L1
    , PRED_BI
    , num_PRED_IDC
};

static const Bs8u SubWidthC[5]  = {1,2,2,1,1};
static const Bs8u SubHeightC[5] = {1,2,1,1,1};

inline Bs32u CeilLog2(Bs32s x){
    Bs32s size = 0;
    while(x > (1<<size)) size++;
    return size;
}

template<class _T, size_t _S> class _unsafe_array{
private:
    _T arr[_S];
public:
    inline _T& operator[](size_t idx){
        return arr[idx];
    }
};

struct SDecCtx{
    void update(Slice& s);
    void updateIntraPredModeY(CQT& cqt);
    bool zAvailableN(Bs16s xCurr, Bs16s yCurr, Bs16s xNbY, Bs16s yNbY);
    CQT* get(Bs16u x, Bs16u y);
    TransTree* getTU(Bs16u x, Bs16u y);
    Bs8u IntraPredModeY(Bs16u x, Bs16u y, CQT* _cqt = 0);
    Bs8u IntraPredModeC(Bs16u x, Bs16u y, CQT* _cqt = 0);
    inline Bs16u ColumnWidthInLumaSamples(Bs16u i)   { return (colWidth[ i ] << CtbLog2SizeY); }
    inline Bs16u RowHeightInLumaSamples  (Bs16u j)   { return (rowHeight[ j ] << CtbLog2SizeY ); }
    inline Bs16u bitDepth                (Bs16u cIdx){ return cIdx ? (8+slice->sps->bit_depth_chroma_minus8) : (8+slice->sps->bit_depth_luma_minus8); }
    inline Bs16u CuPredMode              (CQT&  cqt) { return (cqt.cu_skip_flag ? 2 : cqt.pred_mode_flag); }
    inline Bs16u PartMode                (CQT&  cqt) { return ((cqt.pred_mode_flag && cqt.part_mode) ? PART_NxN : cqt.part_mode); }

    Slice* slice;
    CTU*   ctu;

    Bs16u RawBits;
    Bs16u MinCbLog2SizeY;
    Bs16u CtbLog2SizeY;
    Bs16u CtbSizeY;
    Bs16u PicWidthInCtbsY;
    Bs16u PicHeightInCtbsY;
    Bs16u MinCbSizeY;
    Bs16u PicWidthInMinCbsY;
    Bs16u PicHeightInMinCbsY;
    Bs16u PicSizeInMinCbsY;
    Bs16u PicSizeInCtbsY;
    Bs16u PicSizeInSamplesY;
    Bs16u PicWidthInSamplesC;
    Bs16u PicHeightInSamplesC;
    Bs16u Log2MinTrafoSize;
    Bs16u Log2MaxTrafoSize;
    Bs16u Log2MinIpcmCbSizeY;
    Bs16u Log2MaxIpcmCbSizeY;
    Bs16u Log2MinCuQpDeltaSize;
    Bs8u  IsCuQpDeltaCoded;
    Bs8s  CuQpDeltaVal;
    Bs8s  SliceQpY;
    Bs8s  QpYprev;

    Bs16u MaxTrafoDepth;
    Bs16u MaxNumMergeCand;
    bool  IntraSplitFlag;

    std::vector<Bs16u> colWidth;
    std::vector<Bs16u> rowHeight;
    std::vector<Bs16u> colBd;
    std::vector<Bs16u> rowBd;
    std::vector<Bs16u> CtbAddrRsToTs;
    std::vector<Bs16u> CtbAddrTsToRs;
    std::vector<Bs16u> TileId;

    std::vector<Bs16s> vSliceAddrRs;
    std::vector<std::vector<Bs32u> > MinTbAddrZs;
    std::vector<std::vector<SAO> > Sao[3];
    std::vector<_unsafe_array<Bs16u,2> > ScanOrder[4][3];
    std::vector<std::vector<Bs8s> > vIntraPredModeY;
};

const Bs16u CtxTblSize = 185;
extern const Bs8u CtxInitTbl[3][CtxTblSize];
extern const Bs8u CtxOffset[num_SE + 1];
extern const Bs8u rangeTabLpsT[4][64];
extern const Bs8u transIdxLps[64];
extern const Bs8u transIdxMps[64];

class HEVC_CABAC : public BS_Reader{
public:
    HEVC_CABAC();
    ~HEVC_CABAC(){
    };


    void  Init  (SDecCtx& ctx, Bs32s CtbAddrInRs);
    void  Store (bool is2ndCtuInRow, bool endOfSliceSegment);
    Bs32u ae    (Bs16u se, Bs32s par = 0);

    inline void NewBlock() { m_cSBIdx = -1; };
    inline void PCMEnd  () { init_ade(); };

    inline void ResetLog() {
#ifdef HEVC_CABAC_STATE_TRACE
        if(m_cnt > 0x40000){
            char name[256];
            sprintf(name, "%s%d", HEVC_CABAC_STATE_TRACE, ++m_n);
            m_cnt = 0;
            fclose(flog);
            flog = fopen( name, "w" );
        }
#endif //HEVC_CABAC_STATE_TRACE
    }

private:
    Bs32s m_decPar;
    Bs16u m_lastGreater1Ctx;
    Bs16u m_ctxSet;
    SDecCtx* m_pCtx;

    inline Bs16u idxStart(Bs16u se) { return CtxOffset[se];}
    inline Bs16u idxEnd  (Bs16u se) { return CtxOffset[se+1];}

    void init_ctx(); // 9.3.2.2
    void init_ade(); // 9.4.2.5

    enum CTX_INC{
          ERROR = -4
        , EXTERNAL
        , TERMINATE
        , BYPASS
    };
    Bs8s ctxInc(Bs16u se, Bs16u binIdx);

    Bs32u decCAbsLvlR(Bs32s i, Bs32s baseLevel);
    Bs32u decAbsMvdMinus2();

    ///////////////////////////////////////////////////
    struct Bin{
        static const Bs16u capacity = 32;
        static const Bs32u ff = 0xFFFFFFFF;

        Bin(Bs32u value, Bs8u size) :
            n(size)
          , b(value & ( ff >> (capacity-size))) {
        }
        bool operator () (const Bin &bin) const {
            return (bin.n == n && bin.b.to_ulong() == b.to_ulong());
        }
        void  put (Bs32u bits, Bs8u num = 1) {
            if(!num) return;
            b <<= num;
            b |= std::bitset<capacity>(bits & ( ff >> (capacity-num)));
            n += num;
        }
        Bs32u get() {
            return b.to_ulong();
        }

        Bs8u n;
        std::bitset<capacity> b;
    };
    typedef std::list<Bin> Bins;
    static const Bs32u binTRx0Size = 32;

    Bs32s m_cAbsLevel;
    Bs32s m_cRiceParam;
    Bs32s m_cSBIdx;

    Bins m_binFL1;
    Bins m_binFL3;
    Bins m_binFL31;
    Bins m_binICPM;
    Bins m_binIPIDC0;
    Bins m_binDefault;
    Bins m_binTRx0[binTRx0Size];

    Bins& GetBinarization(Bs16u se);

    void  initBins    ();
    Bins& PartModeBin (bool isIntra, Bs32u log2CbSize);
    Bins& CUQPDAbsBin ();
    Bin   TR          (Bs32s synVal, Bs32u cMax, Bs32u cRiceParam);
    Bin   EGk         (Bs32s synVal, Bs32u k);
    Bin   FL          (Bs32s synVal, Bs32u cMax);

    ///////////////////////////////////////////////////
    static const Bs8s  CtxIncTbl[num_SE_full][6];

    Bs8u  CtxState   [CtxTblSize];
    Bs8u  CtxStateWpp[CtxTblSize];
    Bs8u  CtxStateDs [CtxTblSize];

    Bs16u ivlCurrRange;
    Bs16u ivlOffset;

    Bs8u DecodeBin       (Bs8u* ctxTable, Bs16u ctxIdx, bool bypassFlag);
    Bs8u DecodeDecision  (Bs8u* ctxTable, Bs16u ctxIdx);
    Bs8u DecodeBypass    ();
    Bs8u DecodeTerminate ();

    inline void sync (Bs8u* t) { memcpy(CtxState, t, CtxTblSize); }
    inline void store(Bs8u* t) { memcpy(t, CtxState, CtxTblSize); }
    inline void RenormD() {
        while(ivlCurrRange < 256){
            ivlCurrRange <<= 1;
            ivlOffset     =  ((ivlOffset << 1) | read_1_bit());
            m_pCtx->RawBits ++;
        }
    }

    inline void trace(Bs8u b = 7) {
#ifdef HEVC_CABAC_STATE_TRACE
        fprintf(flog, "Count: %d \tValue: %d \tRange: %d \tBin: %d\n", m_cnt++, ivlOffset, ivlCurrRange, b);
        fflush(flog);
#else
        (void)b;
#endif //HEVC_CABAC_STATE_TRACE
    };

#ifdef HEVC_CABAC_STATE_TRACE
    Bs32u m_cnt;
    Bs32u m_n;
    FILE* flog;
#endif //HEVC_CABAC_STATE_TRACE

};
}// namespace BS_HEVC;

#endif // __HEVC_CABAC_H
