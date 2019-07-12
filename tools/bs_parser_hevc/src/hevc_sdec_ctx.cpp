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

template <class Type> class fill_vector
{
public:
   fill_vector ( const Type& _val ) : val ( _val ) {}
   void operator () ( std::vector<Type>& el ) const  { std::fill(el.begin(), el.end(), val); }
private:
   Type val;
};

void SDecCtx::update(Slice& s){
    Bs32s i = 0, j = 0, x = 0, y = 0, p = 0, m = 0, tileIdx = 0;

    MaxNumMergeCand     = 5 - s.five_minus_max_num_merge_cand;
    QpYprev = SliceQpY  = 26 + s.pps->init_qp_minus26 + s.slice_qp_delta;

    if(    slice
        && slice->sps == s.sps
        && slice->pps == s.pps){
        bool newPicture = (slice->pic_order_cnt_lsb != s.pic_order_cnt_lsb);
        slice = &s;

        if(newPicture){
            std::for_each(vIntraPredModeY.begin(), vIntraPredModeY.end(), fill_vector<Bs8s>(-1));
            std::fill(vSliceAddrRs.begin(), vSliceAddrRs.end(), -1);
        }
        return;
    }
    slice = &s;
    ctu   = NULL;

    MinCbLog2SizeY      = s.sps->log2_min_luma_coding_block_size_minus3 + 3;
    CtbLog2SizeY        = MinCbLog2SizeY + s.sps->log2_diff_max_min_luma_coding_block_size;
    CtbSizeY            = (1  <<  CtbLog2SizeY);
    PicWidthInCtbsY     = (s.sps->pic_width_in_luma_samples + CtbSizeY - 1) / CtbSizeY;
    PicHeightInCtbsY    = (s.sps->pic_height_in_luma_samples + CtbSizeY - 1) / CtbSizeY;
    MinCbSizeY          = (1  <<  MinCbLog2SizeY);
    PicWidthInMinCbsY   = s.sps->pic_width_in_luma_samples / MinCbSizeY;
    PicHeightInMinCbsY  = s.sps->pic_height_in_luma_samples / MinCbSizeY;
    PicSizeInMinCbsY    = PicWidthInMinCbsY * PicHeightInMinCbsY;
    PicSizeInCtbsY      = PicWidthInCtbsY * PicHeightInCtbsY;
    PicSizeInSamplesY   = s.sps->pic_width_in_luma_samples * s.sps->pic_height_in_luma_samples;
    PicWidthInSamplesC  = s.sps->pic_width_in_luma_samples / SubWidthC[s.sps->chroma_format_idc + s.sps->separate_colour_plane_flag];
    PicHeightInSamplesC = s.sps->pic_height_in_luma_samples / SubHeightC[s.sps->chroma_format_idc + s.sps->separate_colour_plane_flag];
    Log2MinTrafoSize    = s.sps->log2_min_transform_block_size_minus2 + 2;
    Log2MaxTrafoSize    = Log2MinTrafoSize + s.sps->log2_diff_max_min_transform_block_size;
    Log2MinIpcmCbSizeY  = s.sps->log2_min_pcm_luma_coding_block_size_minus3 + 3;
    Log2MaxIpcmCbSizeY  = Log2MinIpcmCbSizeY + s.sps->log2_diff_max_min_pcm_luma_coding_block_size;
    Log2MinCuQpDeltaSize= CtbLog2SizeY - s.pps->diff_cu_qp_delta_depth;
    IsCuQpDeltaCoded    = 0;

    vIntraPredModeY.resize(((s.sps->pic_width_in_luma_samples+64) >> Log2MinTrafoSize),
        std::vector<Bs8s> (((s.sps->pic_height_in_luma_samples+64) >> Log2MinTrafoSize), -1));
    std::for_each(vIntraPredModeY.begin(), vIntraPredModeY.end(), fill_vector<Bs8s>(-1));

    colWidth.resize(s.pps->num_tile_columns_minus1+1);
    if( s.pps->uniform_spacing_flag ){
        for( i = 0; i  <=  s.pps->num_tile_columns_minus1; i++ ){
            colWidth[ i ] = ( ( i + 1 ) * PicWidthInCtbsY ) / ( s.pps->num_tile_columns_minus1 + 1 ) -
                            ( i * PicWidthInCtbsY ) / ( s.pps->num_tile_columns_minus1 + 1 );
        }
    } else {
        colWidth[ s.pps->num_tile_columns_minus1 ] = PicWidthInCtbsY;
        for( i = 0; i < s.pps->num_tile_columns_minus1; i++ ) {
            colWidth[ i ] = s.pps->column_width_minus1[ i ] + 1;
            colWidth[ s.pps->num_tile_columns_minus1 ]  -=  colWidth[ i ];
        }
    }

    rowHeight.resize(s.pps->num_tile_rows_minus1+1);
    if( s.pps->uniform_spacing_flag ){
        for( j = 0; j  <=  s.pps->num_tile_rows_minus1; j++ ){
            rowHeight[ j ] = ( ( j + 1 ) * PicHeightInCtbsY ) / ( s.pps->num_tile_rows_minus1 + 1 ) -
                            ( j * PicHeightInCtbsY ) / ( s.pps->num_tile_rows_minus1 + 1 );
        }
    } else {
        rowHeight[ s.pps->num_tile_rows_minus1 ] = PicHeightInCtbsY;
        for( j = 0; j < s.pps->num_tile_rows_minus1; j++ ) {;
            rowHeight[ j ] = s.pps->row_height_minus1[ j ] + 1;
            rowHeight[ s.pps->num_tile_rows_minus1 ]  -=  rowHeight[ j ];
        }
    }

    colBd.resize(s.pps->num_tile_columns_minus1+2);
    for( colBd[ 0 ] = 0, i = 0; i  <=  s.pps->num_tile_columns_minus1; i++ )
        colBd[ i + 1 ] = colBd[ i ] + colWidth[ i ];

    rowBd.resize(s.pps->num_tile_rows_minus1+2);
    for( rowBd[ 0 ] = 0, j = 0; j  <=  s.pps->num_tile_rows_minus1; j++ )
        rowBd[ j + 1 ] = rowBd[ j ] + rowHeight[ j ];

    CtbAddrRsToTs.resize(PicSizeInCtbsY);
    for( Bs16u ctbAddrRs = 0; ctbAddrRs < PicSizeInCtbsY; ctbAddrRs++ ) {
        Bs16u tbX = ctbAddrRs % PicWidthInCtbsY;
        Bs16u tbY = ctbAddrRs / PicWidthInCtbsY;
        Bs16u tileX = 0;
        Bs16u tileY = 0;

        for( i = 0; i  <=  s.pps->num_tile_columns_minus1; i++ )
            if( tbX  >=  colBd[ i ] )
                tileX = i;

        for( j = 0; j  <=  s.pps->num_tile_rows_minus1; j++ )
            if( tbY  >=  rowBd[ j ] )
                tileY = j;

        CtbAddrRsToTs[ ctbAddrRs ] = 0;

        for( i = 0; i < tileX; i++ )
            CtbAddrRsToTs[ ctbAddrRs ]  +=  rowHeight[ tileY ] * colWidth[ i ];

        for( j = 0; j < tileY; j++ )
            CtbAddrRsToTs[ ctbAddrRs ]  +=  PicWidthInCtbsY * rowHeight[ j ];

        CtbAddrRsToTs[ ctbAddrRs ]  +=  ( tbY - rowBd[ tileY ] ) * colWidth[ tileX ] + tbX - colBd[ tileX ];
    }

    CtbAddrTsToRs.resize(PicSizeInCtbsY);
    for(Bs16u ctbAddrRs = 0; ctbAddrRs < PicSizeInCtbsY; ctbAddrRs++ )
        CtbAddrTsToRs[ CtbAddrRsToTs[ ctbAddrRs ] ] = ctbAddrRs;


    TileId.resize(PicSizeInCtbsY);
    for( j = 0, tileIdx = 0; j  <=  s.pps->num_tile_rows_minus1; j++ )
        for( i = 0; i  <=  s.pps->num_tile_columns_minus1; i++, tileIdx++ )
            for( y = rowBd[ j ]; y < rowBd[ j + 1 ]; y++ )
                for( x = colBd[ i ]; x < colBd[ i + 1 ]; x++ )
                        TileId[ CtbAddrRsToTs[ y * PicWidthInCtbsY+ x ] ] = tileIdx;


    MinTbAddrZs.resize(
        ( PicWidthInCtbsY << ( CtbLog2SizeY - Log2MinTrafoSize ) ),
        std::vector<Bs32u>(( PicHeightInCtbsY << ( CtbLog2SizeY - Log2MinTrafoSize ) ))
    );
    for( y = 0; y < ( PicHeightInCtbsY  <<  ( CtbLog2SizeY - Log2MinTrafoSize ) ); y++ ){
        for( x = 0; x < ( PicWidthInCtbsY  <<  ( CtbLog2SizeY - Log2MinTrafoSize ) ); x++) {
            Bs32u tbX = ( x  <<  Log2MinTrafoSize )  >>  CtbLog2SizeY;
            Bs32u tbY = ( y  <<  Log2MinTrafoSize )  >>  CtbLog2SizeY;
            Bs32u ctbAddrRs = PicWidthInCtbsY * tbY + tbX;

            MinTbAddrZs[ x ][ y ] = CtbAddrRsToTs[ ctbAddrRs ] << ( ( CtbLog2SizeY - Log2MinTrafoSize ) * 2 );
            for( i = 0, p = 0; i < ( CtbLog2SizeY - Log2MinTrafoSize ); i++ ) {
                m = 1  <<  i;
                p  +=  ( m & x ? m * m : 0 ) + ( m & y ? 2 * m * m : 0 );
            }
            MinTbAddrZs[ x ][ y ]  +=  p;
        }
    }

    vSliceAddrRs.resize(PicSizeInCtbsY, -1);
    std::fill(vSliceAddrRs.begin(), vSliceAddrRs.end(), -1);

    for(i = 0; i < 3; i++)
        Sao[i].resize(PicWidthInCtbsY, std::vector<SAO>(PicHeightInCtbsY));

    for(Bs16u log2BlockSize = 0; log2BlockSize < 4; log2BlockSize ++){
        Bs16u blkSize = ( 1  <<  log2BlockSize );
        Bs16u sz = blkSize * blkSize;

        ScanOrder[log2BlockSize][0].resize(sz);
        i = 0; x = 0; y = 0;
        while( 1 ) {
            while( y  >=  0 ) {
                if( x < blkSize  &&  y < blkSize ) {
                    ScanOrder[log2BlockSize][0][ i ][ 0 ] = x;
                    ScanOrder[log2BlockSize][0][ i ][ 1 ] = y;
                    i++;
                }
                y--;
                x++;
            }
            y = x;
            x = 0;
            if( i  >=  sz )
                break;
        }

        ScanOrder[log2BlockSize][1].resize(sz);
        i = 0;
        for( y = 0; y  < blkSize; y++ ){
            for( x = 0; x < blkSize; x++ ) {
                ScanOrder[log2BlockSize][1][ i ][ 0 ] = x;
                ScanOrder[log2BlockSize][1][ i ][ 1 ] = y;
                i++;
            }
        }

        ScanOrder[log2BlockSize][2].resize(sz);
        i = 0;
        for( x = 0; x  < blkSize; x++ ){
            for( y = 0; y < blkSize; y++ ) {
                ScanOrder[log2BlockSize][2][ i ][ 0 ] = x;
                ScanOrder[log2BlockSize][2][ i ][ 1 ] = y;
                i++;
            }
        }
    }
}


bool SDecCtx::zAvailableN(Bs16s xCurr, Bs16s yCurr, Bs16s xNbY, Bs16s yNbY){
    Bs32s minBlockAddrCurr = MinTbAddrZs[ xCurr >> Log2MinTrafoSize ][ yCurr >> Log2MinTrafoSize ];
    Bs32s minBlockAddrN = -1;
    Bs32u CtbAddrInTsCurr = CtbAddrRsToTs[(yCurr>>CtbLog2SizeY)*PicWidthInCtbsY + (xCurr>>CtbLog2SizeY)];
    Bs32u CtbAddrInTsN = 0;

    if(     xNbY >= 0 && yNbY >= 0
        && xNbY < (Bs32s)slice->sps->pic_width_in_luma_samples
        && yNbY < (Bs32s)slice->sps->pic_height_in_luma_samples){
        minBlockAddrN = MinTbAddrZs[ xNbY >> Log2MinTrafoSize ][ yNbY >> Log2MinTrafoSize ];
        CtbAddrInTsN = CtbAddrRsToTs[(yNbY>>CtbLog2SizeY)*PicWidthInCtbsY + (xNbY>>CtbLog2SizeY)];
    }

    if (   minBlockAddrN < 0
        || minBlockAddrN > minBlockAddrCurr
        || vSliceAddrRs[CtbAddrInTsN] != vSliceAddrRs[CtbAddrInTsCurr]
        || TileId[CtbAddrInTsN] != TileId[CtbAddrInTsCurr])
        return false;

    return true;
}


template<class T> T* get_unit(T& cqt, Bs16u x, Bs16u y){
    if(cqt.split){
        T* res = 0;

        for(Bs16u i = 0; i < 4; i++){
            T* t = get_unit(cqt.split[i], x, y);

            if(t->x > x || t->y > y){
                continue;
            }

            res = t;
        }

        if(res){
            return res;
        }
    }
    return &cqt;
}

CQT* SDecCtx::get(Bs16u x, Bs16u y){
    Bs16u CtbAddrInRs = (y>>CtbLog2SizeY)*PicWidthInCtbsY + (x>>CtbLog2SizeY);
    return get_unit(ctu[CtbAddrInRs].cqt, x, y);
}

TransTree* SDecCtx::getTU(Bs16u x, Bs16u y) {
    CQT* cqt = get(x, y);
    if (!cqt || !cqt->tu)
        return 0;
    return get_unit(*cqt->tu, x, y);
}

void SDecCtx::updateIntraPredModeY(CQT& cqt){
    Bs16u nCbS = ( 1  <<  (CtbLog2SizeY - cqt.CtDepth) );
    Bs16u pbOffset = ( PartMode(cqt)  ==  PART_NxN ) ? ( nCbS / 2 ) : nCbS;

    for(Bs16u j = 0; j < nCbS/pbOffset; j ++ ){
        for(Bs16u i = 0; i < nCbS/pbOffset; i ++ ){
            Bs16u x0 = cqt.x + j*pbOffset;
            Bs16u y0 = cqt.y + i*pbOffset;
            Bs8s mode = IntraPredModeY(x0, y0, &cqt);

            for(Bs16u xIdx = (x0>>Log2MinTrafoSize); xIdx < ((x0>>Log2MinTrafoSize) + (pbOffset>>Log2MinTrafoSize)); xIdx++){
                for(Bs16u yIdx = (y0>>Log2MinTrafoSize); yIdx < ((y0>>Log2MinTrafoSize) + (pbOffset>>Log2MinTrafoSize)); yIdx++){
                    vIntraPredModeY[xIdx][yIdx] = mode;
                }
            }
        }
    }

}

Bs8u SDecCtx::IntraPredModeY(Bs16u xPb, Bs16u yPb, CQT* _cqt){
    Bs8s& IntraPredModeY = vIntraPredModeY[xPb >> Log2MinTrafoSize][yPb >> Log2MinTrafoSize];

    if(IntraPredModeY >= 0){
        return IntraPredModeY;
    }

    CQT*  cqt = _cqt ? _cqt : get(xPb, yPb);
    Bs16u nCbS = ( 1  <<  (CtbLog2SizeY - cqt->CtDepth) );
    Bs16u pbOffset = ( PartMode(*cqt)  ==  PART_NxN ) ? ( nCbS / 2 ) : nCbS;
    Bs8u  xIdx = (xPb - cqt->x) / pbOffset;
    Bs8u  yIdx = (yPb - cqt->y) / pbOffset;
    CQT::LumaPred& lp = cqt->luma_pred[xIdx][yIdx];
    bool availableX[2] = { zAvailableN(xPb, yPb, xPb - 1, yPb), zAvailableN(xPb, yPb, xPb, yPb - 1) };
    Bs8u candIntraPredModeX[2] = {0,};
    Bs8u candModeList[3] = {0,};

    for(Bs16u i = 0; i < 2; i++){
        if(!availableX[i]){
            candIntraPredModeX[i] = 1;
        } else {
            CQT* cqtX = get(xPb - !i, yPb - i);
            if(CuPredMode(*cqtX) != MODE_INTRA || cqtX->pcm_flag ){
                candIntraPredModeX[i] = 1;
            } else if(i && ((yPb - i) < (( yPb >> CtbLog2SizeY ) << CtbLog2SizeY ))){
                candIntraPredModeX[i] = 1;
            } else {
                candIntraPredModeX[i] = this->IntraPredModeY(xPb - !i, yPb - i, cqtX);
            }
        }
    }

    if(candIntraPredModeX[0] == candIntraPredModeX[1]){
        if(candIntraPredModeX[0] < 2){
            candModeList[0] = 0;
            candModeList[1] = 1;
            candModeList[2] = 26;
        } else {
            candModeList[0] = candIntraPredModeX[0];
            candModeList[1] = 2 + ( ( candIntraPredModeX[0] + 29 ) % 32 );
            candModeList[2] = 2 + ( ( candIntraPredModeX[0] - 2 + 1 ) % 32 );
        }
    } else {
        candModeList[0] = candIntraPredModeX[0];
        candModeList[1] = candIntraPredModeX[1];

        if(candModeList[0] != 0 && candModeList[1] != 0){
            candModeList[2] = 0;
        } else if(candModeList[0] != 1 && candModeList[1] != 1){
            candModeList[2] = 1;
        } else {
            candModeList[2] = 26;
        }
    }

    if(lp.prev_intra_luma_pred_flag)
        return IntraPredModeY = candModeList[lp.mpm_idx];

    if(candModeList[0] > candModeList[1])
        std::swap(candModeList[0], candModeList[1]);

    if(candModeList[0] > candModeList[2])
        std::swap(candModeList[0], candModeList[2]);

    if(candModeList[1] > candModeList[2])
        std::swap(candModeList[1], candModeList[2]);

    IntraPredModeY = lp.rem_intra_luma_pred_mode;
    for(Bs16u i = 0; i < 3; i++)
        IntraPredModeY += (IntraPredModeY >= candModeList[i]);

    return IntraPredModeY;
}

static const Bs8s PredModeY2C[5][5] = {
    { 0, 34,  0,  0,  0},
    {26, 26, 34, 26, 26},
    {10, 10, 10, 34, 10},
    { 1,  1,  1,  1, 34},
    {-1,  0, 26, 10,  1}
};

inline Bs8u IntraPredModeC(Bs8u IntraPredModeY, Bs8u intra_chroma_pred_mode){
    Bs8s IntraPredModeC = PredModeY2C[intra_chroma_pred_mode][
          1 * (IntraPredModeY ==  0)
        + 2 * (IntraPredModeY == 26)
        + 3 * (IntraPredModeY == 10)
        + 4 * (IntraPredModeY ==  1)
    ];
    return (IntraPredModeC < 0) ? IntraPredModeY : IntraPredModeC;
}

Bs8u SDecCtx::IntraPredModeC(Bs16u xPb, Bs16u yPb, CQT* _cqt){
    CQT* cqt = _cqt ? _cqt : get(xPb, yPb);
    bool xShift = (2 == SubWidthC[slice->sps->chroma_format_idc]);
    bool yShift = (2 == SubHeightC[slice->sps->chroma_format_idc]);

    return ::IntraPredModeC(
        IntraPredModeY(xShift ? cqt->x : xPb,  yShift ? cqt->y : yPb, cqt), //FIXME: map luma (xPb, yPb) to chroma
        cqt->intra_chroma_pred_mode );
}
