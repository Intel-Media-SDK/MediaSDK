// Copyright (c) 2012-2020 Intel Corporation
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
#include "../include/genx_me_common.h"
#define COMPLEX_BIDIR 1
#define INVERTMOTION  1

typedef matrix<uchar, 4, 32> UniIn;

_GENX_ inline
void SetRef(
    vector_ref<short, 2> /*source*/,     // IN:  SourceX, SourceY
    vector<short, 2>     mv_predictor,      // IN:  mv predictor
    vector_ref<char, 2> searchWindow,   // IN:  reference window w/h
    vector<uchar, 2>    /*picSize*/,    // IN:  pic size w/h
    vector_ref<short, 2> reference
)      // OUT: Ref0X, Ref0Y
{
    vector<short, 2>
        Width = (searchWindow - 16) >> 1,
        MaxMvLen,
        mask,
        res,
        otherRes;

    // set up parameters
    MaxMvLen[0] = 0x7fff / 4;
    MaxMvLen[1] = 0x7fff / 4;

    // fields and MBAFF are not supported 
    // remove quater pixel fraction
    mv_predictor = mv_predictor >> 2;

    //
    // set the reference position
    //
    reference = mv_predictor;
    reference[1] &= -2;
    reference -= Width;

    res      = MaxMvLen - Width;
    mask     = (mv_predictor > res);
    otherRes = MaxMvLen - (searchWindow - 16);
    reference.merge(otherRes, mask);

    res      = -res;
    mask     = (mv_predictor < res);
    otherRes = -MaxMvLen;
    reference.merge(otherRes, mask);
}

extern "C" _GENX_MAIN_
void MeP16_1MV_MRE(
    SurfaceIndex SURF_CONTROL,
    SurfaceIndex SURF_SRC_AND_REF,
    SurfaceIndex SURF_DIST16x16,
    SurfaceIndex SURF_MV16x16,
    uint         start_xy,
    uchar        blSize
)
{
    vector<uint, 1>
        start_mbXY = start_xy;
    uint
        mbX = get_thread_origin_x() + start_mbXY.format<ushort>()[0],
        mbY = get_thread_origin_y() + start_mbXY.format<ushort>()[1],
        x   = mbX * blSize,
        y   = mbY * blSize;

    vector<uchar, 96> control;
    read(SURF_CONTROL, 0, control);

    uchar
        maxNumSu = control.format<uchar>()[56],
        lenSp    = control.format<uchar>()[57];
    ushort
        width      = control.format<ushort>()[30],
        height     = control.format<ushort>()[31],
        mre_width  = control.format<ushort>()[33],
        mre_height = control.format<ushort>()[34],
        precision  = control.format<ushort>()[36];

    cm_assert(x > width);
    // read MB record data
    UniIn
        uniIn = 0;
    matrix<uchar, 9, 32>
        imeOut;
    matrix<uchar, 2, 32>
        imeIn = 0;
    matrix<uchar, 4, 32>
        fbrIn;

    // declare parameters for VME
    matrix<uint, 16, 2>
        costs = 0;
    vector<short, 2>
        mvPred = 0,
        mvPred2 = 0;
    uchar
        x_r = 64,
        y_r = 32;

    // load search path
    imeIn.select<2, 1, 32, 1>(0) = control.select<64, 1>(0);

    // M0.2
    VME_SET_UNIInput_SrcX(uniIn, x);
    VME_SET_UNIInput_SrcY(uniIn, y);

    // M0.3 various prediction parameters
    VME_SET_DWORD(uniIn, 0, 3, 0x76a40000); // BMEDisableFBR=1 InterSAD=2 8x8 16x16
                                            //VME_SET_DWORD(uniIn, 0, 3, 0x76a00000); // BMEDisableFBR=0 InterSAD=2 SubMbPartMask=0x76: 8x8 16x16
                                            //VME_SET_UNIInput_BMEDisableFBR(uniIn);
                                            // M1.1 MaxNumMVs
    VME_SET_UNIInput_MaxNumMVs(uniIn, 32);
    // M0.5 Reference Window Width & Height
    VME_SET_UNIInput_RefW(uniIn, x_r);//48);
    VME_SET_UNIInput_RefH(uniIn, y_r);//40);
    VME_SET_UNIInput_EarlyImeSuccessEn(uniIn);

    // M0.0 Ref0X, Ref0Y
    vector_ref<short, 2>
        sourceXY = uniIn.row(0).format<short>().select<2, 1>(4);
    vector<uchar, 2>
        widthHeight;
    widthHeight[0] = (height >> 4) - 1;
    widthHeight[1] = (width >> 4);
    vector_ref<char, 2>
        searchWindow = uniIn.row(0).format<char>().select<2, 1>(22);

    vector_ref<short, 2>
        ref0XY = uniIn.row(0).format<short>().select<2, 1>(0);
    SetRef(sourceXY, mvPred, searchWindow, widthHeight, ref0XY);

    vector_ref<short, 2>
        ref1XY = uniIn.row(0).format<short>().select<2, 1>(2);
    SetRef(sourceXY, mvPred2, searchWindow, widthHeight, ref1XY);

    // M1.0-3 Search path parameters & start centers & MaxNumMVs again!!!
    VME_SET_UNIInput_AdaptiveEn(uniIn);
    VME_SET_UNIInput_T8x8FlagForInterEn(uniIn);
    VME_SET_UNIInput_MaxNumMVs(uniIn, 0x3f);
    VME_SET_UNIInput_MaxNumSU(uniIn, maxNumSu);
    VME_SET_UNIInput_LenSP(uniIn, lenSp);
    //VME_SET_UNIInput_BiWeight(uniIn, 32);

    // M1.2 Start0X, Start0Y
    vector<char, 2>
        start0 = searchWindow;
    start0 = ((start0 - 16) >> 3) & 0x0f;
    uniIn.row(1)[10] = start0[0] | (start0[1] << 4);

    uniIn.row(1)[6] = 0x20;
    uniIn.row(1)[31] = 0x1;

    vector<short, 2>
        ref0 = uniIn.row(0).format<short>().select<2, 1>(0);
    vector<ushort, 16>
        costCenter = uniIn.row(3).format<ushort>().select<16, 1>(0);

    vector<short, 2>
        mv16;
    matrix<uint, 1, 1>
        dist16x16;
    run_vme_ime(uniIn, imeIn,
        VME_STREAM_OUT, VME_SEARCH_SINGLE_REF_SINGLE_REC_SINGLE_START,
        SURF_SRC_AND_REF, ref0XY, NULL, costCenter, imeOut);
    VME_GET_IMEOutput_Rec0_16x16_Mv(imeOut, mv16);
    VME_GET_IMEOutput_Rec0_16x16_Distortion(imeOut, dist16x16);

    // distortions calculated before updates (subpel, bidir search)
    write(SURF_DIST16x16, mbX * DIST_SIZE, mbY, dist16x16); //16x16 Forward SAD

    if (precision)
    {//QPEL
        VME_SET_UNIInput_SubPelMode(uniIn, 3);
        VME_CLEAR_UNIInput_BMEDisableFBR(uniIn);
        SLICE(fbrIn.format<uint>(), 1, 16, 2) = 0; // zero L1 motion vectors
        VME_SET_UNIInput_FBRMbModeInput(uniIn, 0);
        VME_SET_UNIInput_FBRSubMBShapeInput(uniIn, 0);
        VME_SET_UNIInput_FBRSubPredModeInput(uniIn, 3);
        matrix<uchar, 7, 32>
            fbrOut16x16;
        fbrIn.format<uint, 4, 8>().select<4, 1, 4, 2>(0, 0) = mv16.format<uint>()[0]; // motion vectors 16x16
        run_vme_fbr(uniIn, fbrIn, SURF_SRC_AND_REF, 0, 0, 0, fbrOut16x16);
        VME_GET_FBROutput_Rec0_16x16_Mv(fbrOut16x16, mv16);
        VME_GET_FBROutput_Dist_16x16_Bi(fbrOut16x16, dist16x16);
    }

    // distortions Actual complete distortion
    //write(SURF_DIST16x16, mbX * DIST_SIZE, mbY, dist16x16);

    // motion vectors
    write(SURF_MV16x16, mbX * MVDATA_SIZE, mbY, mv16);      //16x16mv Ref0
}

extern "C" _GENX_MAIN_
void MeP16_1MV_MRE_8x8(
    SurfaceIndex SURF_CONTROL,
    SurfaceIndex SURF_SRC_AND_REF,
    SurfaceIndex SURF_DIST8x8,
    SurfaceIndex SURF_MV8x8,
    uint         start_xy,
    uchar        blSize
)
{
    vector<uint, 1>
        start_mbXY = start_xy;
    uint
        mbX = get_thread_origin_x() + start_mbXY.format<ushort>()[0],
        mbY = get_thread_origin_y() + start_mbXY.format<ushort>()[1],
        x   = mbX * blSize,
        y   = mbY * blSize;

    vector<uchar, 96>
        control;
    read(SURF_CONTROL, 0, control);

    uchar
        maxNumSu = control.format<uchar>()[56],
        lenSp = control.format<uchar>()[57];
    ushort
        width      = control.format<ushort>()[30],
        height     = control.format<ushort>()[31],
        mre_width  = control.format<ushort>()[33],
        mre_height = control.format<ushort>()[34],
        precision  = control.format<ushort>()[36];


    // read MB record data
    UniIn
        uniIn = 0;
    matrix<uchar, 9, 32>
        imeOut;
    matrix<uchar, 2, 32>
        imeIn = 0;
    matrix<uchar, 4, 32>
        fbrIn;

    // declare parameters for VME
    matrix<uint, 16, 2>
        costs = 0;
    vector<short, 2>
        mvPred = 0,
        mvPred2 = 0;
    //read(SURF_MV16x16, mbX * MVDATA_SIZE, mbY, mvPred); // these pred MVs will be updated later here
    uchar
        x_r = 64,
        y_r = 32;

    // load search path
    imeIn.select<2, 1, 32, 1>(0) = control.select<64, 1>(0);

    // M0.2
    VME_SET_UNIInput_SrcX(uniIn, x);
    VME_SET_UNIInput_SrcY(uniIn, y);

    // M0.3 various prediction parameters
    //VME_SET_DWORD(uniIn, 0, 3, 0x76a40000); // BMEDisableFBR=1 InterSAD=2 8x8 16x16
    //VME_SET_DWORD(uniIn, 0, 3, 0x76a00000); // BMEDisableFBR=0 InterSAD=2 SubMbPartMask=0x76: 8x8 16x16
    VME_SET_DWORD(uniIn, 0, 3, 0x77a00000); // BMEDisableFBR=0 InterSAD=2 SubMbPartMask=0x77: 8x8
                                            //VME_SET_UNIInput_BMEDisableFBR(uniIn);
                                            // M1.1 MaxNumMVs
    VME_SET_UNIInput_MaxNumMVs(uniIn, 32);
    // M0.5 Reference Window Width & Height
    VME_SET_UNIInput_RefW(uniIn, x_r);//48);
    VME_SET_UNIInput_RefH(uniIn, y_r);//40);

                                      // M0.0 Ref0X, Ref0Y
    vector_ref<short, 2>
        sourceXY = uniIn.row(0).format<short>().select<2, 1>(4);
    vector<uchar, 2>
        widthHeight;
    widthHeight[0] = (height >> 4) - 1;
    widthHeight[1] = (width >> 4);
    vector_ref<char, 2>
        searchWindow = uniIn.row(0).format<char>().select<2, 1>(22);

    vector_ref<short, 2>
        ref0XY = uniIn.row(0).format<short>().select<2, 1>(0);
    SetRef(sourceXY, mvPred, searchWindow, widthHeight, ref0XY);

    // M1.0-3 Search path parameters & start centers & MaxNumMVs again!!!
    VME_SET_UNIInput_AdaptiveEn(uniIn);
    VME_SET_UNIInput_T8x8FlagForInterEn(uniIn);
    VME_SET_UNIInput_MaxNumMVs(uniIn, 0x3f);
    VME_SET_UNIInput_MaxNumSU(uniIn, maxNumSu);
    VME_SET_UNIInput_LenSP(uniIn, lenSp);
    //VME_SET_UNIInput_BiWeight(uniIn, 32);

    // M1.2 Start0X, Start0Y
    vector<char, 2>
        start0 = searchWindow;
    start0 = ((start0 - 16) >> 3) & 0x0f;
    uniIn.row(1)[10] = start0[0] | (start0[1] << 4);

    uniIn.row(1)[6] = 0x20;
    uniIn.row(1)[31] = 0x1;

    vector<short, 2>
        ref0 = uniIn.row(0).format<short>().select<2, 1>(0);
    vector<ushort, 16>
        costCenter = uniIn.row(3).format<ushort>().select<16, 1>(0);

    VME_SET_UNIInput_EarlyImeSuccessEn(uniIn);
    matrix<short, 2, 4>
        mv8;
    vector<uint, 4>
        dist8;

    run_vme_ime(uniIn, imeIn,
        VME_STREAM_OUT, VME_SEARCH_SINGLE_REF_SINGLE_REC_SINGLE_START,
        SURF_SRC_AND_REF, ref0XY, NULL, costCenter, imeOut);
    mv8   = imeOut.row(8).format<short>().select<8, 1>(8); // 4 MVs
    dist8 = imeOut.row(7).format<ushort>().select<4, 1>(4);
    // distortions Integer search results
    // 8x8
    write(SURF_DIST8x8, mbX * DIST_SIZE * 2, mbY * 2, dist8.format<uint, 2, 2>());     //8x8 Forward SAD
    if (precision)
    {//QPEL
        VME_SET_UNIInput_SubPelMode(uniIn, 3);
        VME_CLEAR_UNIInput_BMEDisableFBR(uniIn);
        SLICE(fbrIn.format<uint>(), 1, 16, 2) = 0; // zero L1 motion vectors
        matrix<uchar, 7, 32> fbrOut8x8;
        VME_SET_UNIInput_FBRMbModeInput(uniIn, 3);
        VME_SET_UNIInput_FBRSubMBShapeInput(uniIn, 0);
        VME_SET_UNIInput_FBRSubPredModeInput(uniIn, 3);
        fbrIn.format<uint, 4, 8>().select<1, 1, 4, 2>(0, 0) = mv8.format<uint>()[0]; // motion vectors 8x8_0
        fbrIn.format<uint, 4, 8>().select<1, 1, 4, 2>(1, 0) = mv8.format<uint>()[1]; // motion vectors 8x8_1
        fbrIn.format<uint, 4, 8>().select<1, 1, 4, 2>(2, 0) = mv8.format<uint>()[2]; // motion vectors 8x8_2
        fbrIn.format<uint, 4, 8>().select<1, 1, 4, 2>(3, 0) = mv8.format<uint>()[3]; // motion vectors 8x8_3
        run_vme_fbr(uniIn, fbrIn, SURF_SRC_AND_REF, 3, 0, 0, fbrOut8x8);
        VME_GET_FBROutput_Rec0_8x8_4Mv(fbrOut8x8, mv8.format<uint>());
        VME_GET_FBROutput_Dist_8x8_Bi(fbrOut8x8, dist8);
    }

    // distortions actual complete distortion calculation
    // 8x8
    //write(SURF_DIST8x8  , mbX * DIST_SIZE * 2  , mbY * 2, dist8.format<uint,2,2>());     //8x8 Bidir distortions

    // motion vectors
    // 8x8
    write(SURF_MV8x8, mbX * MVDATA_SIZE * 2, mbY * 2, mv8);       //8x8mvs  Ref0
}

extern "C" _GENX_MAIN_
void MeP16bi_1MV2_MRE(
    SurfaceIndex SURF_CONTROL,
    SurfaceIndex SURF_SRC_AND_REF,
    SurfaceIndex SURF_SRC_AND_REF2,
    SurfaceIndex SURF_DIST16x16,
    SurfaceIndex SURF_MV16x16,
    SurfaceIndex SURF_MV16x16_2,
    uint         start_xy,
    uchar        blSize,
    char         forwardRefDist,
    char         backwardRefDist
)
{
    vector<uint, 1>
        start_mbXY = start_xy;
    uint
        mbX = get_thread_origin_x() + start_mbXY.format<ushort>()[0],
        mbY = get_thread_origin_y() + start_mbXY.format<ushort>()[1],
        x   = mbX * blSize,
        y   = mbY * blSize;

    vector<uchar, 96>
        control;
    read(SURF_CONTROL, 0, control);

    uchar
        maxNumSu = control.format<uchar>()[56],
        lenSp = control.format<uchar>()[57];
    ushort
        width      = control.format<ushort>()[30],
        height     = control.format<ushort>()[31],
        mre_width  = control.format<ushort>()[33],
        mre_height = control.format<ushort>()[34],
        precision  = control.format<ushort>()[36];

    // read MB record data
    UniIn
        uniIn = 0;
#if COMPLEX_BIDIR
    matrix<uchar, 9, 32>
        imeOut;
#else
    matrix<uchar, 11, 32>
        imeOut;
#endif
    matrix<uchar, 2, 32>
        imeIn = 0;
    matrix<uchar, 4, 32>
        fbrIn;

    // declare parameters for VME
    matrix<uint, 16, 2> costs = 0;
    vector<short, 2>
        mvPred  = 0,
        mvPred2 = 0;
    //read(SURF_MV16x16, mbX * MVDATA_SIZE, mbY, mvPred); // these pred MVs will be updated later here

#if COMPLEX_BIDIR
    uchar
        x_r = 64,
        y_r = 32;
#else
    uchar
        x_r = 32,
        y_r = 32;
#endif

    // load search path
    imeIn.select<2, 1, 32, 1>(0) = control.select<64, 1>(0);

    // M0.2
    VME_SET_UNIInput_SrcX(uniIn, x);
    VME_SET_UNIInput_SrcY(uniIn, y);

    // M0.3 various prediction parameters
#if COMPLEX_BIDIR
    VME_SET_DWORD(uniIn, 0, 3, 0x76a40000); // BMEDisableFBR=1 InterSAD=2 8x8 16x16
#else
    VME_SET_DWORD(uniIn, 0, 3, 0x76a00000); // BMEDisableFBR=0 InterSAD=2 SubMbPartMask=0x76: 8x8 16x16
#endif
    //VME_SET_UNIInput_BMEDisableFBR(uniIn);
    // M1.1 MaxNumMVs
    VME_SET_UNIInput_MaxNumMVs(uniIn, 32);
    // M0.5 Reference Window Width & Height
    VME_SET_UNIInput_RefW(uniIn, x_r);//48);
    VME_SET_UNIInput_RefH(uniIn, y_r);//40);

    // M0.0 Ref0X, Ref0Y
    vector_ref<short, 2>
        sourceXY = uniIn.row(0).format<short>().select<2, 1>(4);
    vector<uchar, 2>
        widthHeight;
    widthHeight[0] = (height >> 4) - 1;
    widthHeight[1] = (width >> 4);
    vector_ref<char, 2>
        searchWindow = uniIn.row(0).format<char>().select<2, 1>(22);

    vector_ref<short, 2>
        ref0XY = uniIn.row(0).format<short>().select<2, 1>(0);
    SetRef(sourceXY, mvPred, searchWindow, widthHeight, ref0XY);

    vector_ref<short, 2>
        ref1XY = uniIn.row(0).format<short>().select<2, 1>(2);

    // M1.0-3 Search path parameters & start centers & MaxNumMVs again!!!
    VME_SET_UNIInput_AdaptiveEn(uniIn);
    VME_SET_UNIInput_T8x8FlagForInterEn(uniIn);
    VME_SET_UNIInput_MaxNumMVs(uniIn, 0x3f);
    VME_SET_UNIInput_MaxNumSU(uniIn, maxNumSu);
    VME_SET_UNIInput_LenSP(uniIn, lenSp);
    //VME_SET_UNIInput_BiWeight(uniIn, 32);

    // M1.2 Start0X, Start0Y
    vector<char, 2>
        start0 = searchWindow;
    start0 = ((start0 - 16) >> 3) & 0x0f;
    uniIn.row(1)[10] = start0[0] | (start0[1] << 4);

    uniIn.row(1)[6] = 0x20;
    uniIn.row(1)[31] = 0x1;

    vector<short, 2>
        ref0 = uniIn.row(0).format<short>().select<2, 1>(0);
    vector<ushort, 16>
        costCenter = uniIn.row(3).format<ushort>().select<16, 1>(0);

    VME_SET_UNIInput_EarlyImeSuccessEn(uniIn);
    vector<short, 2>
        mv16, mv16_2;
    matrix<uint, 1, 1>
        dist16x16,
        dist16x16_2;
#if COMPLEX_BIDIR
    run_vme_ime(uniIn, imeIn,
        VME_STREAM_OUT, VME_SEARCH_SINGLE_REF_SINGLE_REC_SINGLE_START,
        SURF_SRC_AND_REF, ref0XY, NULL, costCenter, imeOut);
    VME_GET_IMEOutput_Rec0_16x16_Mv(imeOut, mv16);
    VME_GET_IMEOutput_Rec0_16x16_Distortion(imeOut, dist16x16);

    mvPred2 = mv16 * backwardRefDist / forwardRefDist;
    SetRef(sourceXY, mvPred2, searchWindow, widthHeight, ref1XY);
    run_vme_ime(uniIn, imeIn,
        VME_STREAM_OUT, VME_SEARCH_SINGLE_REF_SINGLE_REC_SINGLE_START,
        SURF_SRC_AND_REF2, ref1XY, NULL, costCenter, imeOut);
    VME_GET_IMEOutput_Rec0_16x16_Mv(imeOut, mv16_2);
    VME_GET_IMEOutput_Rec0_16x16_Distortion(imeOut, dist16x16_2);
#else
    run_vme_ime(uniIn, imeIn,
        VME_STREAM_OUT, VME_SEARCH_DUAL_REF_DUAL_REC,
        SURF_SRC_AND_REF, ref0XY, ref1XY, costCenter, imeOut);

    VME_GET_IMEOutput_Rec0_16x16_Mv(imeOut, mv16);
    VME_GET_IMEOutput_Rec0_16x16_Distortion(imeOut, dist16x16);

    VME_GET_IMEOutput_Rec1_16x16_Mv(imeOut, mv16_2);
    VME_GET_IMEOutput_Rec1_16x16_Distortion(imeOut, dist16x16_2);
#endif
    // distortions calculated before updates (subpel, bidir search)
    write(SURF_DIST16x16, mbX * DIST_SIZE, mbY, dist16x16); //16x16 Forward SAD

    if (precision)//QPEL
        VME_SET_UNIInput_SubPelMode(uniIn, 3);
    else
        VME_SET_UNIInput_SubPelMode(uniIn, 0);
    VME_SET_UNIInput_BiWeight(uniIn, 32);

    VME_CLEAR_UNIInput_BMEDisableFBR(uniIn);
    SLICE(fbrIn.format<uint>(), 1, 16, 2) = 0; // zero L1 motion vectors
    VME_SET_UNIInput_FBRMbModeInput(uniIn, 0);
    VME_SET_UNIInput_FBRSubMBShapeInput(uniIn, 0);
    if (precision)//QPEL
        VME_SET_UNIInput_FBRSubPredModeInput(uniIn, 3);
    else
        VME_SET_UNIInput_FBRSubPredModeInput(uniIn, 0);

    matrix<uchar, 7, 32>
        fbrOut16x16;
    fbrIn.format<uint, 4, 8>().select<4, 1, 4, 2>(0, 0) = mv16.format<uint>()[0]; // motion vectors 16x16
    fbrIn.format<uint, 4, 8>().select<4, 1, 4, 2>(0, 1) = mv16_2.format<uint>()[0];
    run_vme_fbr(uniIn, fbrIn, SURF_SRC_AND_REF, 0, 0, 170, fbrOut16x16);
    VME_GET_FBROutput_Rec0_16x16_Mv(fbrOut16x16, mv16);
    VME_GET_FBROutput_Rec1_16x16_Mv(fbrOut16x16, mv16_2);
    VME_GET_FBROutput_Dist_16x16_Bi(fbrOut16x16, dist16x16);


    // distortions Actual complete distortion
    //write(SURF_DIST16x16, mbX * DIST_SIZE, mbY, dist16x16);

    // motion vectors
    write(SURF_MV16x16, mbX * MVDATA_SIZE, mbY, mv16);      //16x16mv Ref0
    write(SURF_MV16x16_2, mbX * MVDATA_SIZE, mbY, mv16_2);    //16x16mv Ref1
}

extern "C" _GENX_MAIN_
void MeP16bi_1MV2_MRE_8x8(
    SurfaceIndex SURF_CONTROL,
    SurfaceIndex SURF_SRC_AND_REF,
    SurfaceIndex SURF_SRC_AND_REF2,
    SurfaceIndex SURF_DIST8x8,
    SurfaceIndex SURF_MV8x8,
    SurfaceIndex SURF_MV8x8_2,
    uint         start_xy,
    uchar        blSize,
    char         forwardRefDist,
    char         backwardRefDist
)
{
    vector<uint, 1>
        start_mbXY = start_xy;
    uint
        mbX = get_thread_origin_x() + start_mbXY.format<ushort>()[0],
        mbY = get_thread_origin_y() + start_mbXY.format<ushort>()[1],
        x = mbX * blSize,
        y = mbY * blSize;

    vector<uchar, 96>
        control;
    read(SURF_CONTROL, 0, control);

    uchar
        maxNumSu = control.format<uchar>()[56],
        lenSp    = control.format<uchar>()[57];
    ushort
        width      = control.format<ushort>()[30],
        height     = control.format<ushort>()[31],
        mre_width  = control.format<ushort>()[33],
        mre_height = control.format<ushort>()[34],
        precision  = control.format<ushort>()[36];
    // read MB record data
#if CMRT_EMU
    if (x >= width)
        return;
    cm_assert(x < width);
#endif
    UniIn
        uniIn = 0;
#if COMPLEX_BIDIR
    matrix<uchar, 9, 32>
        imeOut;
#else
    matrix<uchar, 11, 32>
        imeOut;
#endif
    matrix<uchar, 2, 32>
        imeIn = 0;
    matrix<uchar, 4, 32>
        fbrIn;

    // declare parameters for VME
    matrix<uint, 16, 2>
        costs = 0;
    vector<short, 2>
        mvPred  = 0,
        mvPred2 = 0;
    //read(SURF_MV16x16, mbX * MVDATA_SIZE, mbY, mvPred); // these pred MVs will be updated later here
#if COMPLEX_BIDIR
    uchar x_r = 48;
    uchar y_r = 40;
#else
    uchar
        x_r = 32,
        y_r = 32;
#endif

    // load search path
    imeIn.select<2, 1, 32, 1>(0) = control.select<64, 1>(0);

    // M0.2
    VME_SET_UNIInput_SrcX(uniIn, x);
    VME_SET_UNIInput_SrcY(uniIn, y);

    // M0.3 various prediction parameters
    //VME_SET_DWORD(uniIn, 0, 3, 0x76a40000); // BMEDisableFBR=1 InterSAD=2 8x8 16x16
    VME_SET_DWORD(uniIn, 0, 3, 0x76a00000); // BMEDisableFBR=0 InterSAD=2 SubMbPartMask=0x76: 8x8 16x16
    //VME_SET_DWORD(uniIn, 0, 3, 0x77a00000); // BMEDisableFBR=0 InterSAD=2 SubMbPartMask=0x77: 8x8
    //VME_SET_UNIInput_BMEDisableFBR(uniIn);
    // M1.1 MaxNumMVs
    VME_SET_UNIInput_MaxNumMVs(uniIn, 32);
    // M0.5 Reference Window Width & Height
    VME_SET_UNIInput_RefW(uniIn, x_r);//48);
    VME_SET_UNIInput_RefH(uniIn, y_r);//40);

    // M0.0 Ref0X, Ref0Y
    vector_ref<short, 2>
        sourceXY = uniIn.row(0).format<short>().select<2, 1>(4);
    vector<uchar, 2>
        widthHeight;
    widthHeight[0] = (height >> 4) - 1;
    widthHeight[1] = (width >> 4);
    vector_ref<char, 2>
        searchWindow = uniIn.row(0).format<char>().select<2, 1>(22);

    vector_ref<short, 2>
        ref0XY = uniIn.row(0).format<short>().select<2, 1>(0);
    SetRef(sourceXY, mvPred, searchWindow, widthHeight, ref0XY);

    vector_ref<short, 2>
        ref1XY = uniIn.row(0).format<short>().select<2, 1>(2);
    SetRef(sourceXY, mvPred2, searchWindow, widthHeight, ref1XY);

    // M1.0-3 Search path parameters & start centers & MaxNumMVs again!!!
    VME_SET_UNIInput_AdaptiveEn(uniIn);
    VME_SET_UNIInput_T8x8FlagForInterEn(uniIn);
    VME_SET_UNIInput_MaxNumMVs(uniIn, 0x3f);
    VME_SET_UNIInput_MaxNumSU(uniIn, maxNumSu);
    VME_SET_UNIInput_LenSP(uniIn, lenSp);
    //VME_SET_UNIInput_BiWeight(uniIn, 32);

    // M1.2 Start0X, Start0Y
    vector<char, 2>
        start0 = searchWindow;
    start0 = ((start0 - 16) >> 3) & 0x0f;
    uniIn.row(1)[10] = start0[0] | (start0[1] << 4);

    uniIn.row(1)[6] = 0x20;
    uniIn.row(1)[31] = 0x1;

    vector<short, 2>
        ref0 = uniIn.row(0).format<short>().select<2, 1>(0);
    vector<ushort, 16>
        costCenter = uniIn.row(3).format<ushort>().select<16, 1>(0);
    VME_SET_UNIInput_EarlyImeSuccessEn(uniIn);
#if COMPLEX_BIDIR
    matrix<short, 2, 4>
        mv8,
        mv8_2;
#else
    matrix<uint, 2, 2>
        mv8,
        mv8_2;
#endif
    vector<uint, 4>
        dist8,
        dist8_2;
#if COMPLEX_BIDIR
    run_vme_ime(uniIn, imeIn,
        VME_STREAM_OUT, VME_SEARCH_SINGLE_REF_SINGLE_REC_SINGLE_START,
        SURF_SRC_AND_REF, ref0XY, NULL, costCenter, imeOut);
    mv8   = imeOut.row(8).format<short>().select<8, 1>(8); // 4 MVs
    dist8 = imeOut.row(7).format<ushort>().select<4, 1>(4);
    vector<short, 2>
        mv16;
    VME_GET_IMEOutput_Rec0_16x16_Mv(imeOut, mv16);

#if !INVERTMOTION
    run_vme_ime(uniIn, imeIn,
        VME_STREAM_OUT, VME_SEARCH_SINGLE_REF_SINGLE_REC_SINGLE_START,
        SURF_SRC_AND_REF2, ref1XY, NULL, costCenter, imeOut);
    mv8_2   = imeOut.row(8).format<short>().select<8, 1>(8); // 4 MVs
    dist8_2 = imeOut.row(7).format<ushort>().select<4, 1>(4);

#else
    mvPred2 = mv16 * backwardRefDist / forwardRefDist;
    SetRef(sourceXY, mvPred2, searchWindow, widthHeight, ref1XY);
    run_vme_ime(uniIn, imeIn,
        VME_STREAM_OUT, VME_SEARCH_SINGLE_REF_SINGLE_REC_SINGLE_START,
        SURF_SRC_AND_REF2, ref1XY, NULL, costCenter, imeOut);
    mv8_2   = imeOut.row(8).format<short>().select<8, 1>(8); // 4 MVs
    dist8_2 = imeOut.row(7).format<ushort>().select<4, 1>(4);
    //mv8_2 = mv8 * -1;
#endif
#else
    run_vme_ime(uniIn, imeIn,
        VME_STREAM_OUT, VME_SEARCH_DUAL_REF_DUAL_REC,
        SURF_SRC_AND_REF, ref0XY, ref1XY, costCenter, imeOut);

    //VME_GET_IMEOutput_Rec0_16x16_Mv(imeOut, mv16);
    VME_GET_IMEOutput_Rec0_8x8_4Mv(imeOut, mv8);
    //VME_GET_IMEOutput_Rec0_16x16_Distortion(imeOut, dist16x16);
    VME_GET_IMEOutput_Rec0_8x8_4Distortion(imeOut, dist8);

    //VME_GET_IMEOutput_Rec1_16x16_Mv(imeOut, mv16_2);
    VME_GET_IMEOutput_Rec1_8x8_4Mv(imeOut, mv8_2);
    //VME_GET_IMEOutput_Rec1_16x16_Distortion(imeOut, dist16x16_2);
    VME_GET_IMEOutput_Rec1_8x8_4Distortion(imeOut, dist8_2);
#endif


    // distortions Integer search results
    // 8x8
    write(SURF_DIST8x8, mbX * DIST_SIZE * 2, mbY * 2, dist8.format<uint, 2, 2>());     //8x8 Forward SAD


    if (precision)//QPEL
        VME_SET_UNIInput_SubPelMode(uniIn, 3);
    else
        VME_SET_UNIInput_SubPelMode(uniIn, 0);
    VME_SET_UNIInput_BiWeight(uniIn, 32);
    VME_CLEAR_UNIInput_BMEDisableFBR(uniIn);
    SLICE(fbrIn.format<uint>(), 1, 16, 2) = 0; // zero L1 motion vectors
    matrix<uchar, 7, 32> fbrOut8x8;
    VME_SET_UNIInput_FBRMbModeInput(uniIn, 3);
    VME_SET_UNIInput_FBRSubMBShapeInput(uniIn, 0);
    if (precision)//QPEL
        VME_SET_UNIInput_FBRSubPredModeInput(uniIn, 3);
    else
        VME_SET_UNIInput_FBRSubPredModeInput(uniIn, 0);
    fbrIn.format<uint, 4, 8>().select<1, 1, 4, 2>(0, 0) = mv8.format<uint>()[0]; // motion vectors 8x8_0
    fbrIn.format<uint, 4, 8>().select<1, 1, 4, 2>(1, 0) = mv8.format<uint>()[1]; // motion vectors 8x8_1
    fbrIn.format<uint, 4, 8>().select<1, 1, 4, 2>(2, 0) = mv8.format<uint>()[2]; // motion vectors 8x8_2
    fbrIn.format<uint, 4, 8>().select<1, 1, 4, 2>(3, 0) = mv8.format<uint>()[3]; // motion vectors 8x8_3
    fbrIn.format<uint, 4, 8>().select<1, 1, 4, 2>(0, 1) = mv8_2.format<uint>()[0]; // motion vectors 8x8_2_0
    fbrIn.format<uint, 4, 8>().select<1, 1, 4, 2>(1, 1) = mv8_2.format<uint>()[1]; // motion vectors 8x8_2_1
    fbrIn.format<uint, 4, 8>().select<1, 1, 4, 2>(2, 1) = mv8_2.format<uint>()[2]; // motion vectors 8x8_2_2
    fbrIn.format<uint, 4, 8>().select<1, 1, 4, 2>(3, 1) = mv8_2.format<uint>()[3]; // motion vectors 8x8_2_3
    run_vme_fbr(uniIn, fbrIn, SURF_SRC_AND_REF, 3, 0, 170, fbrOut8x8);
    VME_GET_FBROutput_Rec0_8x8_4Mv(fbrOut8x8, mv8.format<uint>());
    VME_GET_FBROutput_Rec1_8x8_4Mv(fbrOut8x8, mv8_2.format<uint>());
    VME_GET_FBROutput_Dist_8x8_Bi(fbrOut8x8, dist8);


    // distortions actual complete distortion calculation
    // 8x8
    //write(SURF_DIST8x8  , mbX * DIST_SIZE * 2  , mbY * 2, dist8.format<uint,2,2>());     //8x8 Bidir distortions

    // motion vectors
    // 8x8
    write(SURF_MV8x8, mbX * MVDATA_SIZE * 2, mbY * 2, mv8);       //8x8mvs  Ref0
    write(SURF_MV8x8_2, mbX * MVDATA_SIZE * 2, mbY * 2, mv8_2);     //8x8mvs  Ref1
}

extern "C" _GENX_MAIN_
void MeP16_1ME_2BiRef_MRE_8x8(
    SurfaceIndex SURF_CONTROL,
    SurfaceIndex SURF_SRC_AND_REF,
    SurfaceIndex SURF_SRC_AND_REF2,
    SurfaceIndex SURF_DIST8x8,
    SurfaceIndex SURF_MV8x8,
    SurfaceIndex SURF_MV8x8_2,
    uint         start_xy,
    uchar        blSize,
    char         forwardRefDist,
    char         backwardRefDist
)
{
    vector<uint, 1>
        start_mbXY = start_xy;
    uint
        mbX = get_thread_origin_x() + start_mbXY.format<ushort>()[0],
        mbY = get_thread_origin_y() + start_mbXY.format<ushort>()[1],
        x = mbX * blSize,
        y = mbY * blSize;

    vector<uchar, 96>
        control;
    read(SURF_CONTROL, 0, control);

    uchar
        maxNumSu = control.format<uchar>()[56],
        lenSp = control.format<uchar>()[57];
    ushort
        width = control.format<ushort>()[30],
        height = control.format<ushort>()[31],
        mre_width = control.format<ushort>()[33],
        mre_height = control.format<ushort>()[34],
        precision = control.format<ushort>()[36];
    // read MB record data
#if CMRT_EMU
    if (x >= width)
        return;
    cm_assert(x < width);
#endif
    UniIn
        uniIn = 0;
#if COMPLEX_BIDIR
    matrix<uchar, 9, 32>
        imeOut;
#else
    matrix<uchar, 11, 32>
        imeOut;
#endif
    matrix<uchar, 2, 32>
        imeIn = 0;
    matrix<uchar, 4, 32>
        fbrIn;

    // declare parameters for VME
    matrix<uint, 16, 2>
        costs = 0;
    vector<short, 2>
        mvPred = 0,
        mvPred2 = 0;
    //read(SURF_MV16x16, mbX * MVDATA_SIZE, mbY, mvPred); // these pred MVs will be updated later here
#if COMPLEX_BIDIR
    uchar x_r = 48;
    uchar y_r = 40;
#else
    uchar
        x_r = 32,
        y_r = 32;
#endif

    // load search path
    imeIn.select<2, 1, 32, 1>(0) = control.select<64, 1>(0);

    // M0.2
    VME_SET_UNIInput_SrcX(uniIn, x);
    VME_SET_UNIInput_SrcY(uniIn, y);

    // M0.3 various prediction parameters
    //VME_SET_DWORD(uniIn, 0, 3, 0x76a40000); // BMEDisableFBR=1 InterSAD=2 8x8 16x16
    VME_SET_DWORD(uniIn, 0, 3, 0x76a00000); // BMEDisableFBR=0 InterSAD=2 SubMbPartMask=0x76: 8x8 16x16
    //VME_SET_DWORD(uniIn, 0, 3, 0x77a00000); // BMEDisableFBR=0 InterSAD=2 SubMbPartMask=0x77: 8x8
    //VME_SET_UNIInput_BMEDisableFBR(uniIn);
    // M1.1 MaxNumMVs
    VME_SET_UNIInput_MaxNumMVs(uniIn, 32);
    // M0.5 Reference Window Width & Height
    VME_SET_UNIInput_RefW(uniIn, x_r);//48);
    VME_SET_UNIInput_RefH(uniIn, y_r);//40);

    // M0.0 Ref0X, Ref0Y
    vector_ref<short, 2>
        sourceXY = uniIn.row(0).format<short>().select<2, 1>(4);
    vector<uchar, 2>
        widthHeight;
    widthHeight[0] = (height >> 4) - 1;
    widthHeight[1] = (width >> 4);
    vector_ref<char, 2>
        searchWindow = uniIn.row(0).format<char>().select<2, 1>(22);

    vector_ref<short, 2>
        ref0XY = uniIn.row(0).format<short>().select<2, 1>(0);
    SetRef(sourceXY, mvPred, searchWindow, widthHeight, ref0XY);

    vector_ref<short, 2>
        ref1XY = uniIn.row(0).format<short>().select<2, 1>(2);
    SetRef(sourceXY, mvPred2, searchWindow, widthHeight, ref1XY);

    // M1.0-3 Search path parameters & start centers & MaxNumMVs again!!!
    VME_SET_UNIInput_AdaptiveEn(uniIn);
    VME_SET_UNIInput_T8x8FlagForInterEn(uniIn);
    VME_SET_UNIInput_MaxNumMVs(uniIn, 0x3f);
    VME_SET_UNIInput_MaxNumSU(uniIn, maxNumSu);
    VME_SET_UNIInput_LenSP(uniIn, lenSp);
    //VME_SET_UNIInput_BiWeight(uniIn, 32);

    // M1.2 Start0X, Start0Y
    vector<char, 2>
        start0 = searchWindow;
    start0 = ((start0 - 16) >> 3) & 0x0f;
    uniIn.row(1)[10] = start0[0] | (start0[1] << 4);

    uniIn.row(1)[6] = 0x20;
    uniIn.row(1)[31] = 0x1;

    vector<short, 2>
        ref0 = uniIn.row(0).format<short>().select<2, 1>(0);
    vector<ushort, 16>
        costCenter = uniIn.row(3).format<ushort>().select<16, 1>(0);
    VME_SET_UNIInput_EarlyImeSuccessEn(uniIn);

    matrix<short, 2, 4>
        mv8,
        mv8_2;

    vector<uint, 4>
        dist8,
        dist8_2;

    run_vme_ime(uniIn, imeIn,
        VME_STREAM_OUT, VME_SEARCH_SINGLE_REF_SINGLE_REC_SINGLE_START,
        SURF_SRC_AND_REF, ref0XY, NULL, costCenter, imeOut);
    mv8 = imeOut.row(8).format<short>().select<8, 1>(8); // 4 MVs
    dist8 = imeOut.row(7).format<ushort>().select<4, 1>(4);
    vector<short, 2>
        mv16;
    VME_GET_IMEOutput_Rec0_16x16_Mv(imeOut, mv16);


#if 0
    mvPred2 = -mv16;
    // M0.5 Reference Window Width & Height
    VME_SET_UNIInput_RefW(uniIn, 32);//48);
    VME_SET_UNIInput_RefH(uniIn, 32);//40);
    SetRef(sourceXY, mvPred2, searchWindow, widthHeight, ref1XY);
    run_vme_ime(uniIn, imeIn,
        VME_STREAM_OUT, VME_SEARCH_SINGLE_REF_SINGLE_REC_SINGLE_START,
        SURF_SRC_AND_REF2, ref1XY, NULL, costCenter, imeOut);
    mv8_2 = imeOut.row(8).format<short>().select<8, 1>(8); // 4 MVs
    dist8_2 = imeOut.row(7).format<ushort>().select<4, 1>(4);
#else
    mv8_2 = -mv8; // 4 MVs
#endif
    // distortions Integer search results
    // 8x8
    write(SURF_DIST8x8, mbX * DIST_SIZE * 2, mbY * 2, dist8.format<uint, 2, 2>());     //8x8 Forward SAD


    if (precision)//QPEL
        VME_SET_UNIInput_SubPelMode(uniIn, 3);
    else
        VME_SET_UNIInput_SubPelMode(uniIn, 0);
    VME_SET_UNIInput_BiWeight(uniIn, 32);
    VME_CLEAR_UNIInput_BMEDisableFBR(uniIn);
    SLICE(fbrIn.format<uint>(), 1, 16, 2) = 0; // zero L1 motion vectors
    matrix<uchar, 7, 32> fbrOut8x8;
    VME_SET_UNIInput_FBRMbModeInput(uniIn, 3);
    VME_SET_UNIInput_FBRSubMBShapeInput(uniIn, 0);
    if (precision)//QPEL
        VME_SET_UNIInput_FBRSubPredModeInput(uniIn, 3);
    else
        VME_SET_UNIInput_FBRSubPredModeInput(uniIn, 0);
    fbrIn.format<uint, 4, 8>().select<1, 1, 4, 2>(0, 0) = mv8.format<uint>()[0]; // motion vectors 8x8_0
    fbrIn.format<uint, 4, 8>().select<1, 1, 4, 2>(1, 0) = mv8.format<uint>()[1]; // motion vectors 8x8_1
    fbrIn.format<uint, 4, 8>().select<1, 1, 4, 2>(2, 0) = mv8.format<uint>()[2]; // motion vectors 8x8_2
    fbrIn.format<uint, 4, 8>().select<1, 1, 4, 2>(3, 0) = mv8.format<uint>()[3]; // motion vectors 8x8_3
    fbrIn.format<uint, 4, 8>().select<1, 1, 4, 2>(0, 1) = mv8_2.format<uint>()[0]; // motion vectors 8x8_2_0
    fbrIn.format<uint, 4, 8>().select<1, 1, 4, 2>(1, 1) = mv8_2.format<uint>()[1]; // motion vectors 8x8_2_1
    fbrIn.format<uint, 4, 8>().select<1, 1, 4, 2>(2, 1) = mv8_2.format<uint>()[2]; // motion vectors 8x8_2_2
    fbrIn.format<uint, 4, 8>().select<1, 1, 4, 2>(3, 1) = mv8_2.format<uint>()[3]; // motion vectors 8x8_2_3
    run_vme_fbr(uniIn, fbrIn, SURF_SRC_AND_REF, 3, 0, 170, fbrOut8x8);
    VME_GET_FBROutput_Rec0_8x8_4Mv(fbrOut8x8, mv8.format<uint>());
    VME_GET_FBROutput_Rec1_8x8_4Mv(fbrOut8x8, mv8_2.format<uint>());
    VME_GET_FBROutput_Dist_8x8_Bi(fbrOut8x8, dist8);


    // distortions actual complete distortion calculation
    // 8x8
    //write(SURF_DIST8x8  , mbX * DIST_SIZE * 2  , mbY * 2, dist8.format<uint,2,2>());     //8x8 Bidir distortions

    // motion vectors
    // 8x8
    write(SURF_MV8x8, mbX * MVDATA_SIZE * 2, mbY * 2, mv8);       //8x8mvs  Ref0
    write(SURF_MV8x8_2, mbX * MVDATA_SIZE * 2, mbY * 2, mv8_2);     //8x8mvs  Ref1
}