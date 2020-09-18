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
#include "../include/genx_blend_mc.h"
#include "../include/genx_sd_common.h"

extern "C" _GENX_MAIN_
void McP16_4MV_1SURF_WITH_CHR(
    SurfaceIndex SURF_CONTROL,
    SurfaceIndex SURF_REF1,
    SurfaceIndex SURF_MV16x16_1,
    SurfaceIndex SURF_SRC,
    SurfaceIndex SURF_OUT,
    uint         start_xy,
    uint         scene_nums
)
{
    vector<ushort, 2>
        start_mbXY = start_xy;
    vector<uint, 1>
        scene_numbers = scene_nums;
    uchar
        scnFref = scene_numbers.format<uchar>()[0],
        scnSrc = scene_numbers.format<uchar>()[1];
    uint
        mbX = get_thread_origin_x() + start_mbXY.format<ushort>()[0],
        mbY = get_thread_origin_y() + start_mbXY.format<ushort>()[1],
        x = mbX << 3,
        y = mbY << 3;
    vector<uchar, 96>
        control;
    read(SURF_CONTROL, 0, control);
    ushort
        width = control.format<ushort>()[30],
        height = control.format<ushort>()[31],
        th = control.format<short>()[32],
        sTh = control.format<short>()[35];
    matrix<short, 2, 4>
        mv8_g4 = 0,
        mv = 0;
    matrix<uchar, 12, 12>
        srcCh = 0,
        preFil = 0;
    matrix<uchar, 8, 8>
        src = 0,
        out = 0,
        out2 = 0,
        fil = 0;
    matrix<uchar, 4, 8>
        och = 0,
        scm = 0;
    vector<float, 2>
        RsCsT = 0.0f;
    short
        nsc = scnFref == scnSrc;
    read_plane(SURF_SRC, GENX_SURFACE_Y_PLANE, x - 2, y - 2, srcCh);
    if (th > 0)
    {
        RsCsT = Genx_RsCs_aprox_8x8Block(srcCh.select<8, 1, 8, 1>(2, 2));
        //Reference generation
        mv8_g4 = MV_Neighborhood_read(SURF_MV16x16_1, width, height, mbX, mbY);
        out = OMC_Ref_Generation(SURF_REF1, width, height, mbX, mbY, mv8_g4);
        mv = (mv8_g4 * mv8_g4) / 16 * nsc;
        int size = cm_sum<int>(mv);
        int simFactor1 = 0;
        if (nsc)
        {
            simFactor1 = mergeStrengthCalculator(out, srcCh, RsCsT, nsc, th, size);
            srcCh.select<8, 1, 8, 1>(2, 2) = mergeBlocksRef(srcCh, out, simFactor1);
        }
        fil = srcCh.select<8, 1, 8, 1>(2, 2);
        och = SpatialDenoiser_8x8_NV12_Chroma(SURF_SRC, srcCh, mbX, mbY, sTh);
    }
    else
    {
        read_plane(SURF_SRC, GENX_SURFACE_Y_PLANE, x, y, fil);
        read_plane(SURF_SRC, GENX_SURFACE_UV_PLANE, x, y >> 1, och);
    }
    write_plane(SURF_OUT, GENX_SURFACE_Y_PLANE, x, y, fil);
    write_plane(SURF_OUT, GENX_SURFACE_UV_PLANE, x, y >> 1, och);
}

#define TEST 1

extern "C" _GENX_MAIN_
void McP16_4MV_2SURF_WITH_CHR(
    SurfaceIndex SURF_CONTROL,
    SurfaceIndex SURF_REF1,
    SurfaceIndex SURF_MV16x16_1,
    SurfaceIndex SURF_REF2,
    SurfaceIndex SURF_MV16x16_2,
    SurfaceIndex SURF_SRC,
    SurfaceIndex SURF_OUT, 
    uint         start_xy,
    uint         scene_nums
)
{
    vector<ushort, 2>
        start_mbXY = start_xy;
    vector<uint,1>
        scene_numbers  = scene_nums;
    uchar
        scnFref = scene_numbers.format<uchar>()[0],
        scnBref = scene_numbers.format<uchar>()[2],
        scnSrc  = scene_numbers.format<uchar>()[1],
        run     = 0;
    uint
        mbX  = get_thread_origin_x() + start_mbXY.format<ushort>()[0],
        mbY  = get_thread_origin_y() + start_mbXY.format<ushort>()[1],
        x    = mbX << 3,
        y    = mbY << 3;
    vector<uchar, 96>
        control;
    read(SURF_CONTROL, 0, control);
    ushort
        width  = control.format<ushort>()[30],
        height = control.format<ushort>()[31],
        th     = control.format<short>()[32],
        sTh    = control.format<short>()[35];
    matrix<short,2,4>
        mv8_g4 = 0,
        mv = 0;
#if TEST
    matrix<uchar, 12, 16>
        srcCh = 0,
        preFil = 0;
#else
    matrix<uchar, 12, 12>
        srcCh  = 0,
        preFil = 0;
#endif
    matrix<uchar, 8, 8>
        src  = 0,
        out  = 0,
        out2 = 0,
        out3 = 0,
        out4 = 0,
        fil  = 0;
    matrix<uchar, 4, 8>
        och = 0;
    matrix<uchar, 4, 8>
        scm = 0;
    vector<float, 2>
        RsCsT = 0.0f,
        RsCsT1 = 0.0f,
        RsCsT2 = 0.0f;
    short
        dif1 = scnFref == scnSrc,
        dif2 = scnSrc == scnBref,
        dift = !dif1 + !dif2;
    read_plane(SURF_SRC, GENX_SURFACE_UV_PLANE, x, y >> 1, och);
    if (th > 0)
    {
        read_plane(SURF_SRC, GENX_SURFACE_Y_PLANE, x - 2, y - 2, srcCh);
        RsCsT = Genx_RsCs_aprox_8x8Block(srcCh.select<8, 1, 8, 1>(2, 2));
        //First reference
        mv8_g4 = MV_Neighborhood_read(SURF_MV16x16_1, width, height, mbX, mbY);
        out = OMC_Ref_Generation(SURF_REF1, width, height, mbX, mbY, mv8_g4);
        mv = (mv8_g4 * mv8_g4) / 16 * dif1;
        int size1 = cm_sum<int>(mv);
        //Second reference
        mv8_g4 = MV_Neighborhood_read(SURF_MV16x16_2, width, height, mbX, mbY);
        out2 = OMC_Ref_Generation(SURF_REF2, width, height, mbX, mbY, mv8_g4);
        mv = (mv8_g4 * mv8_g4) / 16 * dif2;
        int size2 = cm_sum<int>(mv);

        int size = ((size1 * dif1) + (size2 * dif2));
        if (dif1 + dif2)
            size /= (dif1 + dif2);

        if (size >= DISTANCETH || dift)
        {
            int simFactor1 = mergeStrengthCalculator(out,  srcCh, RsCsT, dif1, th, size1);
            int simFactor2 = mergeStrengthCalculator(out2, srcCh, RsCsT, dif2, th, size2);
            fil = mergeBlocks2Ref(srcCh, out, simFactor1, out2, simFactor2);
        }
        else
        {
            out = MedianIdx_8x8_3ref(out, srcCh.select<8, 1, 8, 1>(2, 2), out2);
            int simFactor1 = mergeStrengthCalculator(out, srcCh, RsCsT, true, th, size);
            fil = mergeBlocksRef(srcCh, out, simFactor1);
        }
    }
    else
    {
        read_plane(SURF_SRC, GENX_SURFACE_Y_PLANE, x, y, fil);
    }
    write_plane(SURF_OUT, GENX_SURFACE_Y_PLANE, x, y, fil);
    write_plane(SURF_OUT, GENX_SURFACE_UV_PLANE, x, y >> 1, och);
}

extern "C" _GENX_MAIN_
void MC_MERGE4(
    SurfaceIndex SURF_REF1,
    SurfaceIndex SURF_REF2,
    uint         start_xy
)
{
    vector<ushort, 2>
        start_mbXY = start_xy;
    uint
        mbX = get_thread_origin_x() + start_mbXY.format<ushort>()[0],
        mbY = get_thread_origin_y() + start_mbXY.format<ushort>()[1],
        x = mbX << 4,
        y = mbY << 4;
    matrix<uchar, 16, 16>
        ref1 = 0,
        ref2 = 0;

    read_plane(SURF_REF1, GENX_SURFACE_Y_PLANE, x, y, ref1);
    read_plane(SURF_REF2, GENX_SURFACE_Y_PLANE, x, y, ref2);

    ref1 = (ref1 + ref2 + 1) >> 1;
    write_plane(SURF_REF1, GENX_SURFACE_Y_PLANE, x, y, ref1);
}

#define VAR_SC_DATA_SIZE 8 //2 * size of float in bytes
extern "C" _GENX_MAIN_
void MC_VAR_SC_CALC(
    SurfaceIndex SURF_SRC,
    SurfaceIndex SURF_NOISE,
    uint         start_xy
)
{
    vector<ushort, 2>
        start_mbXY = start_xy;
    int
        mbX = get_thread_origin_x() + start_mbXY.format<ushort>()[0],
        mbY = get_thread_origin_y() + start_mbXY.format<ushort>()[1],
        x = mbX * 16,
        y = mbY * 16;
    matrix<uchar, 17, 32>
        src = 0;
    matrix<short, 16, 16>
        tmp = 0;
    matrix<float, 1, 2>
        var_sc = 0.0f;

    read_plane(SURF_SRC, GENX_SURFACE_Y_PLANE, x - 1, y - 1, src.select<8, 1, 32, 1>(0, 0));
    read_plane(SURF_SRC, GENX_SURFACE_Y_PLANE, x - 1, y + 7, src.select<8, 1, 32, 1>(8, 0));
    read_plane(SURF_SRC, GENX_SURFACE_Y_PLANE, x - 1, y + 15, src.select<1, 1, 32, 1>(16, 0));
    matrix<ushort, 4, 4>
        rs4x4,
        cs4x4;
    matrix<short, 16, 16>
        tmpRs = src.select<16, 1, 16, 1>(0, 1) - src.select<16, 1, 16, 1>(1, 1),
        tmpCs = src.select<16, 1, 16, 1>(1, 0) - src.select<16, 1, 16, 1>(1, 1);
#pragma unroll
    for (uchar i = 0; i < 4; i++)
    {
#pragma unroll
        for (uchar j = 0; j < 4; j++)
        {
            rs4x4[i][j] = cm_shr<ushort>(cm_sum<uint>(cm_mul<ushort>(tmpRs.select<4, 1, 4, 1>(i << 2, j << 2), tmpRs.select<4, 1, 4, 1>(i << 2, j << 2))), 4, SAT);
            cs4x4[i][j] = cm_shr<ushort>(cm_sum<uint>(cm_mul<ushort>(tmpCs.select<4, 1, 4, 1>(i << 2, j << 2), tmpCs.select<4, 1, 4, 1>(i << 2, j << 2))), 4, SAT);
        }
    }
    float
        average = cm_sum<float>(src.select<16, 1, 16, 1>(1, 1)) / 256.0f,
        square = cm_sum<float>(cm_mul<ushort>(src.select<16, 1, 16, 1>(1, 1), src.select<16, 1, 16, 1>(1, 1))) / 256.0f;
    var_sc(0, 0) = (square - average * average);//Variance value
    float
        RsFull = cm_sum<float>(rs4x4),
        CsFull = cm_sum<float>(cs4x4);
    var_sc(0, 1) = (RsFull + CsFull) / 16.0f;     //RsCs value

    write(SURF_NOISE, mbX * VAR_SC_DATA_SIZE, mbY, var_sc);
}