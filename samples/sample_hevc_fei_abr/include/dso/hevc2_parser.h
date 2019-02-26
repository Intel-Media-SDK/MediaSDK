// Copyright (c) 2019 Intel Corporation
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

#pragma once

#include "bs_reader2.h"
#include "bs_mem+.h"
#include "bs_thread.h"
#include "hevc2_struct.h"
#include "common_cabac.h"
#include "hevc_cabac.h"
#include <algorithm>
#include <vector>
#include <list>

namespace BS_HEVC2
{

template<class T> inline bool GetBit(T f, Bs32u i)
{
    return !!(f & (1 << (sizeof(T) * 8 - 1 - i)));
}

template<class T> inline void SetBit(T& f, Bs32u i, bool b)
{
    T mask = (T(1) << (sizeof(T) * 8 - 1 - i));
    if (b) f |= mask;
    else   f &= ~mask;
}

template<class T> Bs32u CntBits(T f, Bs32u n = 0)
{
    Bs32u c = 0, sz = sizeof(T) * 8;

    if (!n)
        n = sz;

    f >>= (sz - n);

    while (n--)
    {
        c += (f & 1);
        f >>= 1;
    }

    return c;
}

inline Bs32u CeilLog2(Bs32u x) { Bs32u l = 0; while (x > (1U << l)) l++; return l; }
inline Bs32u CountOnes(Bs32u x)
{
    x -= ((x >> 1) & 0x55555555);
    x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
    x = (((x >> 4) + x) & 0x0f0f0f0f);
    x += (x >> 8);
    x += (x >> 16);
    return (x & 0x0000003f);
}
inline Bs32u FloorLog2(Bs32u x)
{
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    return (CountOnes(x) - 1);
    //return CountOnes(x >> 1);
}

template<class T> inline T CeilDiv(T x, T y) { return (x + y - 1) / y; }

inline bool isSlice(NALU& nalu) {
    return (nalu.nal_unit_type <= CRA_NUT)
        && ((nalu.nal_unit_type < RSV_VCL_N10)
         || (nalu.nal_unit_type > RSV_VCL_R15));
}
inline bool isIRAP(NALU& nalu) {
    return ((nalu.nal_unit_type >= BLA_W_LP) && (nalu.nal_unit_type <= RSV_IRAP_VCL23));
}

inline bool isIDR(NALU& nalu) {
    return ((nalu.nal_unit_type == IDR_W_RADL) || (nalu.nal_unit_type == IDR_N_LP));
}

inline bool isCRA(NALU& nalu) {
    return (nalu.nal_unit_type == CRA_NUT);
}

inline bool isRADL(NALU& nalu) {
    return ((nalu.nal_unit_type == RADL_R) || (nalu.nal_unit_type == RADL_N));
}

inline bool isRASL(NALU& nalu) {
    return ((nalu.nal_unit_type == RASL_R) || (nalu.nal_unit_type == RASL_N));
}

inline bool isBLA(NALU& nalu) {
    return ((nalu.nal_unit_type == BLA_W_LP)
        || (nalu.nal_unit_type == BLA_W_RADL)
        || (nalu.nal_unit_type == BLA_N_LP));
}

class NoActiveSet : public BsReader2::Exception
{
public:
    NoActiveSet() : Exception(BS_ERR_WRONG_UNITS_ORDER) {}
};

struct NALUDesc
{
    NALU* p;
    Bs32u NuhBytes;
    bool  complete;
    bool  prealloc;
};

struct PocInfo
{
    Bs32s Lsb;
    Bs32s Msb;
};

extern const Bs8u ScanOrder1[4][2 * 2][2]; //[Up-right diagonal, Horizontal, Vertical, Traverse]
extern const Bs8u ScanOrder2[4][4 * 4][2]; //[Up-right diagonal, Horizontal, Vertical, Traverse]
extern const Bs8u ScanOrder3[4][8 * 8][2]; //[Up-right diagonal, Horizontal, Vertical, Traverse]
extern const Bs8u ScanTraverse4[16 * 16][2]; //(ScanOrder4[3])
extern const Bs8u ScanTraverse5[32 * 32][2]; //(ScanOrder5[3])
extern const Bs8u ScanPos1[4][2][2]; //[Up-right diagonal, Horizontal, Vertical, Traverse]
extern const Bs8u ScanPos2[4][4][4]; //[Up-right diagonal, Horizontal, Vertical, Traverse]
extern const Bs8u ScanPos3[4][8][8]; //[Up-right diagonal, Horizontal, Vertical, Traverse]

class Info
{
public:
    Info()
    {
        m_prevPOC.Lsb  = 0;
        m_prevPOC.Msb  = 0;
    }

    void decodeSSH  (NALU& nalu, bool bNewSequence);
    void decodePOC  (NALU& nalu);
    void decodeRPL  (NALU& nalu);

    bool  NoRaslOutputFlag            = false;
    bool  NoBackwardPredFlag          = false;
    bool  NewPicture                  = false;
    bool  TwoVersionsOfCurrDecPicFlag = false;
    Bs32s MaxPicOrderCntLsb           = 0;
    Bs16u MinCbLog2SizeY              = 0;
    Bs16u CtbLog2SizeY                = 0;
    Bs16u CtbSizeY                    = 0;
    Bs16u PicWidthInCtbsY             = 0;
    Bs16u PicHeightInCtbsY            = 0;
    Bs16u MinCbSizeY                  = 0;
    Bs16u PicWidthInMinCbsY           = 0;
    Bs16u PicHeightInMinCbsY          = 0;
    Bs32u PicSizeInMinCbsY            = 0;
    Bs32u PicSizeInCtbsY              = 0;
    Bs32u PicSizeInSamplesY           = 0;
    Bs16u PicWidthInSamplesC          = 0;
    Bs16u PicHeightInSamplesC         = 0;
    Bs16u MinTbLog2SizeY              = 0;
    Bs16u MaxTbLog2SizeY              = 0;
    Bs16u Log2MinIpcmCbSizeY          = 0;
    Bs16u Log2MaxIpcmCbSizeY          = 0;
    Bs16u Log2MinCuQpDeltaSize        = 0;
    Bs16u Log2ParMrgLevel             = 0;
    Bs16s SliceQpY                    = 0;
    Bs16u BitDepthY                   = 0;
    Bs16u BitDepthC                   = 0;
    Bs16u ChromaArrayType             = 0;
    Bs16u Log2MinCuChromaQpOffsetSize = 0;
    Bs16u SubWidthC                   = 0;
    Bs16u SubHeightC                  = 0;
    Bs16u CtbWidthC                   = 0;
    Bs16u CtbHeightC                  = 0;
    Bs32u RawCtuBits                  = 0;
    Bs16s QpBdOffsetY                 = 0;
    Bs16s QpBdOffsetC                 = 0;
    Bs16u PaletteMaxPredictorSize     = 0;

    Slice**            ColPicSlices   = nullptr;
    Bs16u              NumColSlices   = 0;
    std::vector<Bs16u> colWidth;
    std::vector<Bs16u> rowHeight;
    std::vector<Bs16u> colBd;
    std::vector<Bs16u> rowBd;
    std::vector<Bs16u> CtbAddrRsToTs;
    std::vector<Bs16u> CtbAddrTsToRs;
    std::vector<Bs16u> TileId;
    std::vector<Bs16s> SliceAddrRsInTs;
    std::vector<CTU*>  CtuInRs;

    PocInfo               m_prevPOC;
    Bs32s                 m_prevSlicePOC = 0;
    Slice*                m_cSlice = nullptr;
    std::map<Bs32s, bool> m_DPB;
    std::map<Bs32s, bool> m_DPBafter;
    std::vector<Bs32s>    m_MinTbAddrZs;
    Bs32u                 m_MinTbAddrZsPitch = 0;
    std::vector<SAO>      m_sao[3];

    std::vector<Bs8u>   PalettePredictorEntryReuseFlags;
    Bs16u               PaletteIndexIdc[32 * 32]     = {};
    bool                CopyAboveIndicesFlag[32][32] = {};
    Bs16u               PaletteIndexMap[32][32]      = {};
    std::vector<Bs16u>  CurrentPaletteEntries[3];
    bool                m_bRPLDecoded = false;

    inline Bs32s& MinTbAddrZs(Bs32u x, Bs32u y) { return m_MinTbAddrZs[y * m_MinTbAddrZsPitch + x]; }
    inline SAO& GetSAO(Bs16u cIdx, Bs16u rx, Bs16u ry) { return m_sao[cIdx][ry * PicWidthInCtbsY + rx]; }

    bool AvailableZs(Bs16s xCurr, Bs16s yCurr, Bs16s xNbY, Bs16s yNbY);
    bool AvailablePb(Bs16s xCb, Bs16s yCb, Bs16u nCbS, Bs16s xPb, Bs16s yPb,
        Bs16s nPbW, Bs16s nPbH, Bs16u partIdx, Bs16s xNbY, Bs16s yNbY);
    CU*   GetCU(Bs16s x, Bs16s y);
    PU*   GetPU(Bs16s x, Bs16s y);
    TU*   GetTU(CU& cu, Bs16s x, Bs16s y);
    Bs16s GetQpCb(Bs16s QpY, Bs16s CuQpOffsetCb);
    Bs16s GetQpCr(Bs16s QpY, Bs16s CuQpOffsetCr);
    void  decodeMvLX(CU& cu, PU&pu, Bs16u X, Bs32s (&MvdLX)[2], Bs16u partIdx);
    void  decodeMvLX(CU& cu, PU&pu, Bs16u partIdx);
    bool  decodeMvLXCol(Bs16u xPb, Bs16u yPb, Bs16u nPbW, Bs16u nPbH, Bs16u X, Bs16u refIdxLX, Bs16s (&mvLXCol)[2]);
};

struct CabacCtx
{
    CabacCtx() = default;

    Bs8u  m_ctxState[BS_HEVC::CtxTblSize];
    Bs16u StatCoeff[4];
    std::vector<Bs16u> PredictorPaletteEntries[3];
    Bs16u PredictorPaletteSize;
};

class CABAC
    : private COMMON_CABAC::ADE
    , public virtual Info
    , public CabacCtx
{
private:
    Bs16s m_g1I             = 0;
    Bs16s m_rI              = 0;
    CabacCtx m_ctxWPP;
    CabacCtx m_ctxDS;
    Bs16u ctxSet            = 0;
    Bs16u lastGreater1Ctx   = 0;
    bool  lastGreater1Flag  = false;
    Bs32u cLastAbsLevel     = 0;
    Bs16u cLastRiceParam    = 0;
    Bs16u m_paletteIdxCnt   = 0;

    inline Bs8u DecodeBin(Bs16s off, Bs16s inc)
    {
        BinCount++;
        if (off < 0)   inc = off;
        if (inc == -1) return DecodeBypass();
        if (inc == -2) return DecodeTerminate();
        if (inc < 0)   throw BsReader2::InvalidSyntax();
        return DecodeDecision(m_ctxState[off + inc]);
    }
    Bs32u EGkBypass(Bs32u k);
    Bs32u TBBypass(Bs32u cMax);
    inline Bs8u& CtxState(Bs16u se, Bs16s inc = 0) { return m_ctxState[BS_HEVC::CtxOffset[se] + inc]; }
    inline bool DD(Bs16u se, Bs16s inc = 0) { BinCount++; return !!DecodeDecision(CtxState(se, inc)); }
    inline bool DT() { BinCount++; return !!DecodeTerminate(); }
    inline bool DB() { BinCount++; return !!DecodeBypass(); }

public:
    Bs64u BinCount = 0;

    CABAC(BsReader2::Reader& r)
        : ADE(r)
    {}

    void InitCtx();
    inline void InitADE()      { ADE::Init(); }
    inline void InitPCMBlock() { ADE::InitPCMBlock(); }

    inline void SyncWPP () { (CabacCtx&)(*this) = m_ctxWPP; }
    inline void SyncDS  () { (CabacCtx&)(*this) = m_ctxDS; }
    inline void StoreWPP() { m_ctxWPP = (CabacCtx&)(*this); }
    inline void StoreDS () { m_ctxDS = (CabacCtx&)(*this); }

    inline Bs16u GetR()     { return ADE::GetR(); }
    inline Bs16u GetV()     { return ADE::GetV(); }
    inline Bs64u BitCount() { return ADE::BitCount(); }

    // slice_segment_data()
    inline bool EndOfSliceSegmentFlag() {return DT(); }
    inline bool EndOfSubsetOneBit()     {return DT(); }

    // sao()
    inline bool SaoMergeLeftFlag() { return DD(BS_HEVC::SAO_MERGE_LEFT_FLAG); }
    inline bool SaoMergeUpFlag()   { return DD(BS_HEVC::SAO_MERGE_UP_FLAG); }
    inline Bs8u SaoTypeIdxChroma() { return SaoTypeIdxLuma(); }
           Bs8u SaoTypeIdxLuma();
           Bs8u SaoOffsetAbs(Bs16u bitDepth);
    inline bool SaoOffsetSign()    { return DB(); }
           Bs8u SaoBandPosition();
           Bs8u SaoEoClassLuma();
    inline Bs8u SaoEoClassChroma() { return SaoEoClassLuma(); }

    //coding_quadtree()
    bool SplitCuFlag(Bs16s x0, Bs16s y0);

    //coding_unit()
    inline bool CuTransquantBypassFlag()    { return DD(BS_HEVC::CU_TRANSQUANT_BYPASS_FLAG); }
           bool CuSkipFlag(Bs16s x0, Bs16s y0);
    inline bool PaletteModeFlag()           { return DD(BS_HEVC::PALETTE_MODE_FLAG); }
    inline bool PredModeFlag()              { return DD(BS_HEVC::PRED_MODE_FLAG); }
           Bs8u PartMode(bool intra, Bs16u log2CbSize);
    inline bool PcmFlag()                   { return DT(); }
    inline bool PrevIntraLumaPredFlag()     { return DD(BS_HEVC::PREV_INTRA_LUMA_PRED_FLAG); }
           Bs8u MpmIdx();
    inline Bs8u RemIntraLumaPredMode()      { return SaoBandPosition(); }
           Bs8u IntraChromaPredMode();
    inline bool RqtRootCbf()                { return DD(BS_HEVC::RQT_ROOT_CBF); }

    //prediction_unit()
    inline bool MergeFlag() { return DD(BS_HEVC::MERGE_FLAG); }
           Bs8u MergeIdx();
           Bs8u InterPredIdc(Bs16u nPbW, Bs16u nPbH, Bs16u CtDepth);
           Bs8u RefIdxL0();
           Bs8u RefIdxL1();
    inline bool MvpL0Flag() { return DD(BS_HEVC::MVP_LX_FLAG); }
    inline bool MvpL1Flag() { return MvpL0Flag(); }

    //transform_tree()
    inline bool SplitTransformFlag(Bs16u log2TrafoSize) { return DD(BS_HEVC::SPLIT_TRANSFORM_FLAG, 5 - log2TrafoSize); }
    inline bool CbfLuma(Bs16u trafoDepth)               { return DD(BS_HEVC::CBF_LUMA, !trafoDepth); }
    inline bool CbfCb(Bs16u trafoDepth)                 { return DD(BS_HEVC::CBF_CX, trafoDepth); }
    inline bool CbfCr(Bs16u trafoDepth)                 { return CbfCb(trafoDepth); }

    //mvd_coding()
    inline bool  AbsMvdGreater0Flag() { return DD(BS_HEVC::ABS_MVD_GREATER0_FLAG); }
    inline bool  AbsMvdGreater1Flag() { return DD(BS_HEVC::ABS_MVD_GREATER1_FLAG); }
    inline bool  MvdSignFlag()        { return DB(); }
    inline Bs16u AbsMvdMinus2()       { return (Bs16u)EGkBypass(1); }

    //transform_unit()
    inline bool  TuResidualActFlag()    { return DD(BS_HEVC::TU_RESIDUAL_ACT_FLAG); }
    //delta_qp()
           Bs16u CuQpDeltaAbs();
    inline bool  CuQpDeltaSignFlag()    { return DB(); }
    //chroma_qp_offset()
    inline bool  CuChromaQpOffsetFlag() { return DD(BS_HEVC::CU_CHROMA_QP_OFFSET_FLAG); }
           Bs8u  CuChromaQpOffsetIdx();

    //cross_comp_pred()
           Bs8u Log2ResScaleAbsPlus1(Bs16u c);
    inline bool ResScaleSignFlag(Bs16u c) { return DD(BS_HEVC::RES_SCALE_SIGN_FLAG, c); }

    //residual_coding()
    inline bool  TransformSkipFlag(Bs16u cIdx)         { return DD(BS_HEVC::TRANSFORM_SKIP_FLAG0, !!cIdx); }
    inline bool  ExplicitRdpcmFlag(Bs16u cIdx)         { return DD(BS_HEVC::EXPLICIT_RDPCM_FLAG, !!cIdx); }
    inline bool  ExplicitRdpcmDirFlag(Bs16u cIdx)      { return DD(BS_HEVC::EXPLICIT_RDPCM_DIR_FLAG, !!cIdx); }
           Bs8u  LastSigCoeffXPrefix(Bs16u cIdx, Bs16u log2TrafoSize);
           Bs8u  LastSigCoeffYPrefix(Bs16u cIdx, Bs16u log2TrafoSize);
           Bs32u LastSigCoeffXSuffix(Bs16u prefix);
    inline Bs32u LastSigCoeffYSuffix(Bs16u prefix)     { return LastSigCoeffXSuffix(prefix); }
    inline bool  CodedSubBlockFlag(Bs16u ctxInc)       { return DD(BS_HEVC::CODED_SUB_BLOCK_FLAG, ctxInc); }
    inline bool  SigCoeffFlag(Bs16u ctxInc)            { return DD(BS_HEVC::SIG_COEFF_FLAG, ctxInc); }
           bool  CoeffAbsLevelGreater1Flag(Bs16u cIdx, Bs16u i);
    inline bool  CoeffAbsLevelGreater2Flag(Bs16u cIdx) { return DD(BS_HEVC::COEFF_ABS_LEVEL_GREATER2_FLAG, ctxSet + 4 * !!cIdx); }
           Bs32u CoeffAbsLevelRemaining(Bs16u i, Bs16u baseLevel, Bs16u cIdx, CU& cu, TU& tu);
    inline bool  CoeffSignFlag()                       { return DB(); }

    //palette_coding()
    inline Bs8u  PalettePredictorRun()              { return (Bs8u)EGkBypass(0); }
    inline Bs8u  NumSignalledPaletteEntries()       { return (Bs8u)EGkBypass(0); }
           Bs16u NewPaletteEntries(Bs16u cIdx);
    inline bool  PaletteEscapeValPresentFlag()      { return DB(); }
           Bs16u NumPaletteIndices(Bs16u MaxPaletteIndex);
    inline Bs16u PaletteIndexIdc(Bs16u MaxPaletteIndex) { return (Bs16u)TBBypass(MaxPaletteIndex - !!(m_paletteIdxCnt++)); };
    inline bool  CopyAboveIndicesForFinalRunFlag()  { return DD(BS_HEVC::COPY_ABOVE_INDICES_FOR_FINAL_RUN_FLAG); }
    inline bool  PaletteTransposeFlag()             { return DD(BS_HEVC::PALETTE_TRANSPOSE_FLAG); }
    inline bool  CopyAbovePaletteIndicesFlag()      { return DD(BS_HEVC::COPY_ABOVE_PALETTE_INDICES_FLAG); }
           Bs16u PaletteRunPrefix(Bs16u PaletteMaxRun, bool copy_above_palette_indices_flag, Bs16u palette_index_idc);
    inline Bs16u PaletteRunSuffix(Bs16u PaletteMaxRun, Bs16u PrefixOffset)
                 { return (Bs16u)TBBypass((PrefixOffset << 1) > PaletteMaxRun ? (PaletteMaxRun - PrefixOffset) : (PrefixOffset - 1)); }
           Bs16u PaletteEscapeVal(Bs16u cIdx, bool cu_transquant_bypass_flag);
};

class SDParser //Slice data parser
    : public  BsReader2::Reader
    , private CABAC
    , public virtual Info
{
private:
    std::vector<CTU>   m_ctu;
    std::vector<CU>    m_cu;
    std::vector<PU>    m_pu;
    std::vector<TU>    m_tu;
    std::vector<Bs32s> m_TC_lvl;

    bool  IntraSplitFlag          = false;
    Bs16u MaxTrafoDepth           = 0;
    Bs16s CuQpDeltaVal            = 0;
    Bs16s qPY_PREV                = 0;
    bool  IsCuQpDeltaCoded        = false;
    bool  IsCuChromaQpOffsetCoded = false;
    Bs8u  intra_chroma_pred_mode[2][2] = {};

    bool report_TCLevels = false;
    std::vector<Bs32s> TCLevels;

    template<class T> T* Alloc(Bs16u n_elem = 1)
    {
        if (std::is_same<CTU, T>::value)
        {
            assert(m_ctu.capacity() >= m_ctu.size() + 1);
            CTU z{};
            m_ctu.push_back(z);
            return (T*)&m_ctu.back();
        }
        if (std::is_same<CU, T>::value)
        {
            assert(m_cu.capacity() >= m_cu.size() + 1);
            CU z{};
            m_cu.push_back(z);
            return (T*)&m_cu.back();
        }
        if (std::is_same<PU, T>::value)
        {
            assert(m_pu.capacity() >= m_pu.size() + 1);
            PU z{};
            m_pu.push_back(z);
            return (T*)&m_pu.back();
        }
        if (std::is_same<TU, T>::value)
        {
            assert(m_tu.capacity() >= m_tu.size() + 1);
            TU z{};
            m_tu.push_back(z);
            return (T*)&m_tu.back();
        }
        if (std::is_same<Bs32s, T>::value)
        {
            assert(m_TC_lvl.capacity() >= m_TC_lvl.size() + n_elem);
            m_TC_lvl.insert(m_TC_lvl.end(), n_elem, 0);
            return (T*)&m_TC_lvl[m_TC_lvl.size() - n_elem];
        }
        throw std::bad_alloc();
        return 0;
    }

    void parseSAO(CTU& ctu, Bs16u rx, Bs16u ry);
    CU*  parseCQT(CU& cu, Bs16u log2CbSize, Bs16u cqtDepth);
    void parseCU (CU& cu);
    void parsePC (CU& cu);
    void parsePU (CU& cu, PU& pu, Bs16u CtDepth, Bs16u partIdx);
    TU*  parseTT (CU& cu, TU& tu, TU& tuBase, Bs16u blkIdx);
    void parseTU (CU& cu, TU& tu, TU& tuBase, Bs16u blkIdx);
    void parseResidual(CU& cu, TU& tu, Bs16u x0, Bs16u y0, Bs16u log2TrafoSize, Bs16u cIdx);
    void parseDQP (CU& cu);
    void parseCQPO(CU& cu);

public:
    BS_MEM::Allocator* m_pAllocator;

    SDParser(bool report_TC = false);

    inline Bs32u u(Bs32u n)  { return GetBits(n); };
    inline Bs32u u1()        { return GetBit(); };
    inline Bs32u u8()        { return GetBits(8); };
    inline Bs32u ue()        { return GetUE(); };
    inline Bs32s se()        { return GetSE(); };

    bool more_rbsp_data();

    CTU* parseSSD(NALU& nalu, NALU* pColPic, Bs32u NumCtb = -1);
};

struct SDDesc
{
    BsThread::SyncPoint sp;
    BsThread::State state;
    NALU* Slice;
};

struct SDPar
{
    NALU* Slice;
    NALU* ColPic;
    Bs32u HeaderSize;
    Bs32u NumCtb;
    bool  Emulation;
    bool  NewPicture;
    SDDesc* pTask;
};

struct SDThread
{
    SDParser p;
    std::vector<Bs8u> rbsp;
    std::list<SDPar> par;
    Bs32u locked;
    Bs32u RBSPSize;
    Bs32u RBSPOffset;
    std::mutex mtx;
};

struct SyncPoint
{
    BsThread::SyncPoint AU; // headers parsed, slice tasks submitted
    BsThread::SyncPoint AUDone;
    BSErr sts;
    std::list<SDDesc> SD;
    BS_MEM::Allocator* pAllocator;
};

class Parser
    : private BS_MEM::Allocator
    , private BsReader2::File
    , private SDParser
{
private:
    Bs32u    m_mode;
    NALUDesc m_lastNALU;
    NALU*    m_au;
    SPS*     m_activeSPS;
    VPS*     m_vps[16];
    SPS*     m_sps[16];
    PPS*     m_pps[64];
    bool     m_bNewSequence;
    std::list<SEI*> m_postponedSEI;
    std::map<Bs32s, NALU*> m_ColPic;

    void parseNUH(NALU& nalu);
    void parseRBSP(NALU& nalu);

    void parseAUD(AUD& aud);
    void parseVPS(VPS& vps);
    void parseSPS(SPS& sps);
    void parsePPS(PPS& pps);
    void parseSEI(SEI& sei);
    void parseSSH(NALU& nalu);
    void parseVUI(VUI& vui, Bs32u maxNumSubLayersMinus1);
    void parsePTL(PTL& ptl);
    void parsePTL(SubLayers& sl, Bs32u max_sub_layers_minus1);
    void parseHRD(HRD& hrd, void* base, bool commonInfPresentFlag, Bs32u maxNumSubLayersMinus1);
    void parseSLO(SubLayerOrderingInfo* slo, bool f, Bs32u n);
    void parseSLD(QM& qm);
    void ParseSTRPS(STRPS* strps, Bs32u num, Bs32u idx);
    void parseSEIPL(SEI& sei);
    void parseSEIPL(APS_SEI& aps);
    void parseSEIPL(BP_SEI& bp);
    void parseSEIPL(PT_SEI& pt);
    void parseSEIPL(RP_SEI& rp);

    void parseMLE(PPS& pps);
    void parse3DE(PPS& pps);

    Bs32u parseEXT(Bs8u*& ExtData);

    NALU* GetColPic(Slice& slice);
    void  UpdateColPics(NALU* AU, Slice& slice);

////////////////////////////////////////////////////
    /*static*/ BsThread::Scheduler m_thread;
    std::list<SDThread> m_sdt;
    std::mutex m_mtx;
    std::map<NALU*, SyncPoint> m_spAuToId;
    std::vector<Bs8u> m_rbsp;
    NALU* m_prevSP;
    BSErr m_auErr;
    Bs16u m_asyncAUMax;
    Bs16u m_asyncAUCnt;

    static BsThread::State ParallelAU(void* self, unsigned int);
    static BsThread::State ParallelSD(void* self, unsigned int);
    static BsThread::State AUDone(void* self, unsigned int);
    static BsThread::State SDTWait(void* self, unsigned int);

    BSErr ParseNextAuSubmit(NALU*& pAU);
    BSErr ParseNextAu(NALU*& pAU);
    Bs32u ParseSSDSubmit(SDThread*& pSDT, NALU* AU);

public:
    Parser(Bs32u mode = 0);

    ~Parser();

    BSErr open(const char* file_name);
    void close() { Close(); }

    void set_buffer(Bs8u* buf, Bs32u buf_size) { Reset(buf, buf_size, 0); }

    BSErr parse_next_au(NALU*& pAU);
    BSErr sync(NALU* pAU);

    BSErr Lock(void* p);
    BSErr Unlock(void* p);

    void set_trace_level(Bs32u level);
    inline Bs64u get_cur_pos() { return GetByteOffset(); }
    inline Bs16u get_async_depth() { return m_asyncAUMax; }

};

};
