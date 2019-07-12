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

#include "hevc_cabac.h"

using namespace BS_HEVC;

HEVC_CABAC::HEVC_CABAC()
    : m_decPar(0)
    , m_lastGreater1Ctx(0)
    , m_ctxSet(0)
    , m_pCtx(NULL)
    , m_cAbsLevel(0)
    , m_cRiceParam(0)
    , m_cSBIdx(-1)
    , ivlCurrRange(510)
    , ivlOffset(0)
{
    memset(CtxState, 0, sizeof(CtxState));
    initBins();

#ifdef HEVC_CABAC_STATE_TRACE
    m_cnt = 0;
    m_n = 0;
    flog = fopen(HEVC_CABAC_STATE_TRACE, "w");
#endif //HEVC_CABAC_STATE_TRACE
};

void HEVC_CABAC::initBins(){
    //m_binFL1.clear();
    m_binFL1.push_back(Bin(0,1));
    m_binFL1.push_back(Bin(1,1));

    //m_binFL3.clear();
    m_binFL3.push_back(Bin(0,2));
    m_binFL3.push_back(Bin(1,2));
    m_binFL3.push_back(Bin(2,2));
    m_binFL3.push_back(Bin(3,2));

    //m_binFL31.clear();
    for(Bs32u synVal = 0; synVal <= 31; synVal++)
        m_binFL31.push_back(Bin(synVal, 5));

    for(Bs32u cMax = 0; cMax < binTRx0Size; cMax++){
        for(Bs32u synVal = 0; synVal <= cMax; synVal++)
            m_binTRx0[cMax].push_back(TR(synVal, cMax, 0));
    }

    //m_binICPM.clear();
    m_binICPM.push_back(Bin(4, 3));
    m_binICPM.push_back(Bin(5, 3));
    m_binICPM.push_back(Bin(6, 3));
    m_binICPM.push_back(Bin(7, 3));
    m_binICPM.push_back(Bin(0, 1));

    //m_binIPIDC0.clear();
    m_binIPIDC0.push_back(Bin(0, 2));
    m_binIPIDC0.push_back(Bin(1, 2));
    m_binIPIDC0.push_back(Bin(1, 1));
}

#define Clip3(_min, _max, _x) BS_MIN(BS_MAX(_min, _x), _max)

void HEVC_CABAC::init_ctx(){
    Slice& slice   = *m_pCtx->slice;
    Bs16u initType = 0;
    Bs16u SliceQpY = 26 + slice.pps->init_qp_minus26 + slice.slice_qp_delta;

    if( slice.type  ==  I )
        initType = 0;
    else if( slice.type  ==  P )
        initType = slice.cabac_init_flag ? 2 : 1;
    else
        initType = slice.cabac_init_flag ? 1 : 2;

    for(Bs16u idx = 0; idx < CtxTblSize; idx++){
        Bs16s initValue = CtxInitTbl[initType][idx];
        Bs16s m = (initValue >> 4) * 5 - 45;
        Bs16s n = ( (initValue & 15)  <<  3 ) - 16;
        Bs16s preCtxState = Clip3( 1, 126, ( ( m * Clip3( 0, 51, SliceQpY ) )  >>  4 ) + n );
        Bs8u  valMps = ( preCtxState  <=  63 ) ? 0 : 1;
        CtxState[idx] = ((valMps ? ( preCtxState - 64 ) : ( 63 - preCtxState )) << 1) | valMps;
    }
}

void HEVC_CABAC::init_ade(){
    ivlCurrRange = 510;
    ivlOffset    = read_bits(9);
}

HEVC_CABAC::Bin HEVC_CABAC::TR(Bs32s synVal, Bs32u cMax, Bs32u cRiceParam){
    Bin bin(0, 0);
    Bs32u prefixVal = synVal  >>  cRiceParam;
    Bs32u suffixVal = synVal - ( ( prefixVal )  <<  cRiceParam );
    Bs32u cLen = (cMax  >>  cRiceParam);

    if(prefixVal < cLen){
        bin.put(0xFFFFFFFF, prefixVal);
        bin.put(0);
    } else {
        bin.put(0xFFFFFFFF, cLen);
    }
    if(cRiceParam && cMax > (Bs32u)synVal)
        bin.put(suffixVal, cRiceParam);

    return bin;
}

HEVC_CABAC::Bin HEVC_CABAC::EGk(Bs32s synVal, Bs32u k){
    Bin bin(0, 0);
    Bs32u absV = BS_ABS(synVal);
    Bs8u  stopLoop = 0;

    do {
        if( absV  >=  Bs32u( 1  <<  k ) ) {
            bin.put( 1 );
            absV = absV - ( 1  <<  k );
            k++;
        } else {
            bin.put( 0 );
            while( k-- )
                bin.put( ( absV  >>  k )  &  1 );
            stopLoop = 1;
        }
    } while( !stopLoop );

    return bin;
}

HEVC_CABAC::Bin HEVC_CABAC::FL(Bs32s synVal, Bs32u cMax){
    Bs32u fixedLength = CeilLog2( cMax + 1 );
    Bin bin(synVal, fixedLength);
    return bin;
}

HEVC_CABAC::Bins& HEVC_CABAC::PartModeBin(bool isIntra, Bs32u log2CbSize){
    Bins& b = m_binDefault;
    SPS& sps = *m_pCtx->slice->sps;

    b.clear();
    b.push_back(Bin(1, 1));

    if(isIntra){
        b.push_back(Bin(0, 1));
    } else {
        bool isMinSz = (log2CbSize == (sps.log2_min_luma_coding_block_size_minus3 + 3));

        if(!isMinSz && sps.amp_enabled_flag){
            b.push_back(Bin(3, 3));
        } else {
            b.push_back(Bin(1, 2));
        }

        if(    (isMinSz  && log2CbSize == 3)
            || (!isMinSz && !sps.amp_enabled_flag)){
            b.push_back(Bin(0, 2));
        } else {
            b.push_back(Bin(1, 3));
        }
        if(isMinSz && log2CbSize > 3){
            b.push_back(Bin(0, 3));
        } else {
            b.push_back(Bin(0xFF, 16));//invalid
        }
        b.push_back(Bin(4, 4));
        b.push_back(Bin(5, 4));
        b.push_back(Bin(0, 4));
        b.push_back(Bin(1, 4));
    }

    return b;
}

HEVC_CABAC::Bins& HEVC_CABAC::CUQPDAbsBin (){
    Bins& b = m_binDefault;
    Bs32s maxVal = 26 + ((6 * m_pCtx->slice->sps->bit_depth_luma_minus8) / 2);

    b.clear();

    for(Bs32s i = 0; i <= maxVal; i++){
        Bs32u val = BS_MIN(i, 5);
        b.push_back( TR( val, 5, 0 ) );

        if(val > 4){
            val = i - 5;
            Bin suffix = EGk(val, 0);
            b.back().put(suffix.get(), suffix.n);
        }
    }

    return b;
}

Bs32u HEVC_CABAC::decCAbsLvlR(Bs32s i, Bs32s baseLevel){
    Bs32u synVal = 0;
    Bin   v(0,0);

    if(i != m_cSBIdx){
        m_cAbsLevel  = 0;
        m_cRiceParam = 0;
        m_cSBIdx = i;
    }

    m_cRiceParam = BS_MIN( m_cRiceParam + ( m_cAbsLevel > ( 3 * ( 1  <<  m_cRiceParam ) ) ? 1 : 0 ), 4 );
    Bs32u cMax   = (4 << m_cRiceParam);

    while(v.n <= Bin::capacity){
        Bin b = TR( BS_MIN(cMax, synVal), cMax, m_cRiceParam );

        if(b.n == 4 && b.get() == 0x0F){
            Bin suffix = EGk( synVal - cMax, m_cRiceParam + 1);
            b.put(suffix.get(), suffix.n);
        }

        while(v.n < b.n){
            v.put(DecodeBypass());
        }

        if(v.get() == b.get()){
            m_cAbsLevel  = baseLevel + synVal;
            return synVal;
        }

        synVal ++;
    }

    return last_err = BS_ERR_UNKNOWN;
}

Bs32u HEVC_CABAC::decAbsMvdMinus2(){
    Bin   v(0,0);

    for(Bs32u synVal = 0; synVal <= ((1<<15) - 2); synVal++){
        Bin b = EGk(synVal, 1);

        while(v.n < b.n)
            v.put(DecodeBypass());

        if(v.get() == b.get())
            return synVal;
    }

    return last_err = BS_ERR_UNKNOWN;
}

#define FL1(_bins, _val, _cmax)      _bins.clear(); _bins.push_back( FL(_val, _cmax) );
#define FL_FLAG(_bins)               _bins.clear(); FL1(_bins, 0, 1); FL1(_bins, 1, 1);
#define FL_RANGE(_bins, _max)        _bins.clear(); for(Bs32s _i = 0; _i <= (_max); _i++){ _bins.push_back( FL(_i, (_max)) ); }
#define TR_RANGE(_bins, _max, _rice) _bins.clear(); for(Bs32s _i = 0; _i <= (_max); _i++){ _bins.push_back( TR(_i, (_max), (_rice)) ); }
#define EGK_RANGE(_bins, _max, _k)   _bins.clear(); for(Bs32s _i = 0; _i <= (_max); _i++){ _bins.push_back( EGk(_i, (_k)) ); }
#define TR_RANGE0(_bins, _max)       if((Bs32s)(_max) < (Bs32s)binTRx0Size){ return m_binTRx0[(_max)]; } TR_RANGE(_bins, _max, 0)

HEVC_CABAC::Bins& HEVC_CABAC::GetBinarization(Bs16u se){
    Bins& b = m_binDefault;

    switch(se){
    case END_OF_SLICE_SEGMENT_FLAG     :
    case SAO_MERGE_LEFT_FLAG           :
    /* = SAO_MERGE_UP_FLAG            */
    case SAO_OFFSET_SIGN               :
    case SPLIT_CU_FLAG                 :
    case CU_TRANSQUANT_BYPASS_FLAG     :
    case CU_SKIP_FLAG                  :
    case PRED_MODE_FLAG                :
    case PCM_FLAG                      :
    case RQT_ROOT_CBF                  :
    case PREV_INTRA_LUMA_PRED_FLAG     :
    case MERGE_FLAG                    :
    case MVP_LX_FLAG                   :
    case SPLIT_TRANSFORM_FLAG          :
    case CBF_LUMA                      :
    case CBF_CX                        :
    case ABS_MVD_GREATER0_FLAG         :
    case ABS_MVD_GREATER1_FLAG         :
    case MVD_SIGN_FLAG                 :
    case CU_QP_DELTA_SIGN_FLAG         :
    case TRANSFORM_SKIP_FLAG0          :
    case TRANSFORM_SKIP_FLAG1          :
    case CODED_SUB_BLOCK_FLAG          :
    case SIG_COEFF_FLAG                :
    case COEFF_ABS_LEVEL_GREATER1_FLAG :
    case COEFF_ABS_LEVEL_GREATER2_FLAG :
    case COEFF_SIGN_FLAG               : return m_binFL1;
    case SAO_EO_CLASS_LUMA             :
    /* = SAO_EO_CLASS_CHROMA          */ return m_binFL3;
    case SAO_TYPE_IDX_LUMA             :
    /* = SAO_TYPE_IDX_CHROMA          */
    case MPM_IDX                       : return m_binTRx0[2];
    case SAO_BAND_POSITION             :
    case REM_INTRA_LUMA_PRED_MODE      : return m_binFL31;
    case INTRA_CHROMA_PRED_MODE        : return m_binICPM;
    case INTER_PRED_IDC                : return (m_decPar&1) ? m_binFL1 : m_binIPIDC0; //9.3.3.7 m_decPar = (( nPbW + nPbH )  ==  12)
    case SAO_OFFSET_ABS                : TR_RANGE0(b, ( 1  <<  ( BS_MIN( m_decPar/*bitDepth*/, 10 ) - 5 ) ) - 1); break;
    case MERGE_IDX                     : TR_RANGE0(b, m_decPar/*MaxNumMergeCand*/ - 1); break;
    case REF_IDX_LX                    : TR_RANGE0(b, m_decPar/*num_ref_idx_lx_active_minus1*/); break;
    case ABS_MVD_MINUS2                : EGK_RANGE(b, (1<<15) - 2, 1); break;
    case LAST_SIG_COEFF_X_PREFIX       : TR_RANGE0(b, ((m_decPar>>2)/*log2TrafoSize*/<<1) - 1); break;
    case LAST_SIG_COEFF_Y_PREFIX       : TR_RANGE0(b, ((m_decPar>>2)/*log2TrafoSize*/<<1) - 1); break;
    case LAST_SIG_COEFF_X_SUFFIX       : FL_RANGE(b, (1<<((m_decPar/*last_sig_coeff_x_prefix*/>>1) - 1)) - 1); break;
    case LAST_SIG_COEFF_Y_SUFFIX       : FL_RANGE(b, (1<<((m_decPar/*last_sig_coeff_y_prefix*/>>1) - 1)) - 1); break;
    case PART_MODE                     : return PartModeBin(!!(m_decPar&1), (m_decPar>>1)); //9.3.3.5 ( xCb, yCb ) = ( x0, y0), log2CbSize
    case CU_QP_DELTA_ABS               : return CUQPDAbsBin(); //9.3.3.8
    //case COEFF_ABS_LEVEL_REMAINING     : return CAbsLvlRBin( (m_decPar>>2), (m_decPar&3) ); //9.3.3.9 current sub-block scan index i, baseLevel
    case END_OF_SUB_STREAM_ONE_BIT     : FL1(b, 1, 1); break;
    default:
        last_err = BS_ERR_NOT_IMPLEMENTED;
    }

    return b;
};

Bs8s HEVC_CABAC::ctxInc(Bs16u se, Bs16u binIdx){
    Bs8s inc = CtxIncTbl[se][BS_MIN(binIdx, 5)];

    if(EXTERNAL == inc){
        switch(se){
        case SPLIT_CU_FLAG                 :
        case CU_SKIP_FLAG                  : {
            Bs16u x0 = Bs16u(m_decPar&0xFFFF);
            Bs16u y0 = Bs16u(m_decPar >> 16);
            bool availableL = m_pCtx->zAvailableN( x0, y0, x0 - 1, y0 );
            bool availableA = m_pCtx->zAvailableN( x0, y0, x0, y0 - 1 );

            if(se == SPLIT_CU_FLAG){
                CQT* c0 = m_pCtx->get(x0, y0);
                return (( availableL && (m_pCtx->get(x0 - 1, y0)->CtDepth > c0->CtDepth) )
                      + ( availableA && (m_pCtx->get(x0, y0 - 1)->CtDepth > c0->CtDepth) ));
            } else /*if(se == CU_SKIP_FLAG)*/{
                return (( availableL && (m_pCtx->get(x0 - 1, y0)->cu_skip_flag) )
                      + ( availableA && (m_pCtx->get(x0, y0 - 1)->cu_skip_flag) ));
            }
                                             }
        case PART_MODE                     :
            return (3 - (/*log2CbSize*/(m_decPar>>1) == m_pCtx->MinCbLog2SizeY));
        case INTER_PRED_IDC                :
            return (m_decPar&1) ? 4 : (m_decPar>>1);
            //return ERROR; //( nPbW + nPbH ) != 12 ? CtDepth[ x0 ][ y0 ] : 4
        case SPLIT_TRANSFORM_FLAG          :
            return (5 - m_decPar/*log2TrafoSize*/);
        case CBF_LUMA                      :
            return !m_decPar/*trafoDepth*/;
        case CBF_CX                        :
            return BS_MIN(m_decPar/*trafoDepth*/,5);
        case LAST_SIG_COEFF_X_PREFIX       :
        case LAST_SIG_COEFF_Y_PREFIX       : {
            //9.3.4.2.3
            Bs32u cIdx = (m_decPar & 3);
            Bs32u log2TrafoSize = (m_decPar>>2);
            if(!cIdx)
                return ( binIdx  >>  (( log2TrafoSize + 1 ) >> 2) ) + ( 3 * ( log2TrafoSize - 2 ) + ( ( log2TrafoSize - 1 ) >> 2 ) );
            return ( binIdx  >>  ( log2TrafoSize - 2 ) ) + 15;
                                             }
        case CODED_SUB_BLOCK_FLAG          :
        case SIG_COEFF_FLAG                : return m_decPar;
        case COEFF_ABS_LEVEL_GREATER1_FLAG :{
            //9.3.4.2.6
            Bs16u i = (m_decPar&15);
            //Bs16u n = ((m_decPar>>4)&15);
            Bs16u cIdx = ((m_decPar>>8)&3);
            bool  is1stSB = ((m_decPar>>10)&1);
            bool  is1stTB = ((m_decPar>>11)&1);
            bool  lastGreater1Flag = ((m_decPar>>12)&1);
            Bs16u lastGreater1Ctx = 0;

            if(is1stSB){
                m_ctxSet = 0;
                if(i && !cIdx){
                    m_ctxSet = 2;
                }
                if(is1stTB){
                    lastGreater1Ctx = 1;
                } else {
                    lastGreater1Ctx = m_lastGreater1Ctx;
                    if(lastGreater1Ctx){
                        if(lastGreater1Flag){
                            lastGreater1Ctx = 0;
                        } else {
                            lastGreater1Ctx ++;
                        }
                    }
                }
                m_ctxSet += (lastGreater1Ctx == 0);
                m_lastGreater1Ctx = 1;
            } else {
                if(m_lastGreater1Ctx){
                    if(lastGreater1Flag){
                        m_lastGreater1Ctx = 0;
                    } else {
                        m_lastGreater1Ctx ++;
                    }
                }
            }
            return (( m_ctxSet * 4 ) + BS_MIN( 3, m_lastGreater1Ctx ) + 16 * !!cIdx);
                                            }
        case COEFF_ABS_LEVEL_GREATER2_FLAG :
            //9.3.4.2.7
            //Bs16u cIdx = m_decPar&3;
            return (m_ctxSet + 4 * !!m_decPar);
        default: return ERROR;
        }
    }

    return inc;
}

Bs8u HEVC_CABAC::DecodeDecision(Bs8u* ctxTable, Bs16u ctxIdx){
    Bs8u pStateIdx   = (ctxTable[ctxIdx] >> 1);
    Bs8u valMps      = (ctxTable[ctxIdx]  & 1);
    Bs8u qRangeIdx   = ((ivlCurrRange >> 6) & 3);
    Bs8u ivlLpsRange = rangeTabLpsT[ qRangeIdx ][ pStateIdx ];
    Bs8u binVal      = 0;

    ivlCurrRange -= ivlLpsRange;

    if(ivlOffset >= ivlCurrRange){
        binVal        = !valMps;
        ivlOffset    -= ivlCurrRange;
        ivlCurrRange  = ivlLpsRange;

        if( pStateIdx  ==  0 )
              valMps = !valMps;

         pStateIdx = transIdxLps[pStateIdx];
    } else {
        binVal = valMps;
        pStateIdx = transIdxMps[pStateIdx];
    }

#ifdef HEVC_CABAC_STATE_TRACE
    fprintf(flog, "%3d -> %3d;\n", /*ctxIdx,*/ ctxTable[ctxIdx], ((pStateIdx << 1) | valMps));
    fflush(flog);
#endif // HEVC_CABAC_STATE_TRACE

    ctxTable[ctxIdx] = ((pStateIdx << 1) | valMps);

    RenormD();

    trace(binVal);

    return binVal;
}

Bs8u HEVC_CABAC::DecodeBypass(){
    ivlOffset =  ((ivlOffset << 1) | read_1_bit());
    m_pCtx->RawBits ++;

    if(ivlOffset >= ivlCurrRange){
        ivlOffset -= ivlCurrRange;
        trace(1);
        return 1;
    }

    trace(0);
    return 0;
}


Bs8u HEVC_CABAC::DecodeTerminate(){
    ivlCurrRange -= 2;

    if(ivlOffset >= ivlCurrRange){
        trace(1);
        return 1;
    }

    RenormD();

    trace(0);
    return 0;
}


Bs8u HEVC_CABAC::DecodeBin(Bs8u* ctxTable, Bs16u ctxIdx, bool bypassFlag){
    if(bypassFlag)
        return DecodeBypass();

    if(ctxTable == 0)
        return DecodeTerminate();

    return DecodeDecision( ctxTable, ctxIdx );
}

 Bs32u HEVC_CABAC::ae( Bs16u se, Bs32s par) {
     m_decPar = par;

     if(se == COEFF_ABS_LEVEL_REMAINING)
         return decCAbsLvlR( (m_decPar>>2), (m_decPar&3) );
     else if(se == ABS_MVD_MINUS2)
         return decAbsMvdMinus2();

     Bin v(0,0);
     Bins& b = GetBinarization(se);
     Bins::iterator it, begin = b.begin(), end = b.end();
     Bs32u synValBase = 0;

     for(Bs32s binIdx = 0; binIdx <= Bin::capacity; binIdx++){
         Bs32u synValInc = 0;
         Bs8s  inc = ctxInc(se, binIdx);

         switch(inc){
         case ERROR:
             last_err = BS_ERR_UNKNOWN;
             return -1;
         case BYPASS:
             v.put(DecodeBypass());
             break;
         case TERMINATE:
             v.put(DecodeTerminate());
             break;
         default:
             v.put(DecodeDecision(CtxState+idxStart(se), inc));
             break;
         }

         it = begin;
         while(it != end && it->n < v.n){ it++; synValBase++; }
         begin = it;
         while(it != end && !(it->n == v.n && it->b.to_ulong() == v.b.to_ulong())){ it++; synValInc++; }

         if(end != it){
             return synValBase + synValInc;
         }
     }

     last_err = BS_ERR_UNKNOWN;
     return -2;
 }

void HEVC_CABAC::Init(SDecCtx& ctx, Bs32s CtbAddrInRs){
    m_pCtx = &ctx;
    Bs16u CtbAddrInTs = ctx.CtbAddrRsToTs[CtbAddrInRs];

    if( CtbAddrInTs && ctx.TileId[CtbAddrInTs] != ctx.TileId[CtbAddrInTs-1] ){ //first coding tree unit in a tile
        init_ctx();
    } else if(m_pCtx->slice->pps->entropy_coding_sync_enabled_flag && 0 == (CtbAddrInRs % m_pCtx->PicWidthInCtbsY) ){
        Bs16s x0 = ( CtbAddrInRs %  m_pCtx->PicWidthInCtbsY ) <<  m_pCtx->CtbLog2SizeY;
        Bs16s y0 = ( CtbAddrInRs /  m_pCtx->PicWidthInCtbsY ) <<  m_pCtx->CtbLog2SizeY;

        if( m_pCtx->zAvailableN(x0, y0, x0 + m_pCtx->CtbLog2SizeY, y0 - m_pCtx->CtbLog2SizeY) ){
            sync(CtxStateWpp);
        } else {
            init_ctx();
        }
    } else if(m_pCtx->slice->dependent_slice_segment_flag && CtbAddrInRs == (Bs32s)m_pCtx->slice->segment_address){
        sync(CtxStateDs);
    } else {
        init_ctx();
    }
    init_ade();
}

void HEVC_CABAC::Store(bool is2ndCtuInRow, bool endOfSliceSegment){
    if(is2ndCtuInRow && m_pCtx->slice->pps->entropy_coding_sync_enabled_flag){
        store(CtxStateWpp);
    }

    if(m_pCtx->slice->pps->dependent_slice_segments_enabled_flag && endOfSliceSegment){
        store(CtxStateDs);
    }
}
