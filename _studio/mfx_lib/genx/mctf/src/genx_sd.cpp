// Copyright (c) 2012-2018 Intel Corporation
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
#include "../include/genx_blend_mc.h"
#include "../include/genx_sd_common.h"

typedef matrix<uchar, 4, 32> UniIn;

extern "C" _GENX_MAIN_
void SpatialDenoiser_8x8(
    SurfaceIndex SURF_CONTROL,
    SurfaceIndex SURF_SRC,
    SurfaceIndex SURF_OUT,
    uint         start_xy
)
{
    vector<ushort, 2>
        start_mbXY = start_xy;
    uint
        mbX = get_thread_origin_x() + start_mbXY.format<ushort>()[0],
        mbY = get_thread_origin_y() + start_mbXY.format<ushort>()[1],
        x = mbX * 8,
        y = mbY * 8;
    vector<uchar, 96>
        control;
    read(SURF_CONTROL, 0, control);
    ushort
        width  = control.format<ushort>()[30],
        height = control.format<ushort>()[31];
    float
        th = control.format<short>()[32];
    ushort
        picWidthInMB  = width >> 4,
        picHeightInMB = height >> 4;
    uint
        mbIndex = picWidthInMB * mbY + mbX;

    matrix<uchar, 12, 12>
        src = 0;
    matrix<uchar, 8, 8>
        out = 0;
    read_plane(SURF_SRC, GENX_SURFACE_Y_PLANE, x - 2, y - 2, src);
    matrix<uint, 4, 4>
        Disp = DispersionCalculator(src);

    matrix<float, 4, 4>
        f_disp = Disp,
        h1 = 0.0f,
        h2 = 0.0f,
        hh = 0.0f,
        k0 = 0.0f,
        k1 = 0.0f,
        k2 = 0.0f;
    h1 = cm_exp(-(f_disp / (th / 10.0f)));
    f_disp = Disp * 2.0f;
    h2 = cm_exp(-(f_disp / (th / 10.0f)));
    hh = 1.0f + 4.0f * (h1 + h2);
    k0 = 1.0f / hh;
    k1 = h1 / hh;
    k2 = h2 / hh;

#pragma unroll
    for (uint i = 0; i < 4; i++)
#pragma unroll
        for (uint j = 0; j < 4; j++)
            out.select<2, 1, 2, 1>(i * 2, j * 2) = (src.select<2, 1, 2, 1>(2 + 2 * i, 2 + 2 * j) * k0(i, j)) + (
            (src.select<2, 1, 2, 1>(2 + 2 * i, 1 + 2 * j) + src.select<2, 1, 2, 1>(2 + 2 * i, 3 + 2 * j) + src.select<2, 1, 2, 1>(1 + 2 * i, 2 + 2 * j) + src.select<2, 1, 2, 1>(3 + 2 * i, 2 + 2 * j))*k1(i, j)) + (
                (src.select<2, 1, 2, 1>(1 + 2 * i, 1 + 2 * j) + src.select<2, 1, 2, 1>(1 + 2 * i, 3 + 2 * j) + src.select<2, 1, 2, 1>(3 + 2 * i, 1 + 2 * j) + src.select<2, 1, 2, 1>(3 + 2 * i, 3 + 2 * j))*k2(i, j)) + 0.5f;

    write_plane(SURF_OUT, GENX_SURFACE_Y_PLANE, x, y, out);
}


extern "C" _GENX_MAIN_
void SpatialDenoiser_8x8_NV12(
    SurfaceIndex SURF_CONTROL,
    SurfaceIndex SURF_SRC,
    SurfaceIndex SURF_OUT,
    uint         start_xy
)
{
    vector<ushort, 2>
        start_mbXY = start_xy;
    uint
        mbX = get_thread_origin_x() + start_mbXY.format<ushort>()[0],
        mbY = get_thread_origin_y() + start_mbXY.format<ushort>()[1],
        x = mbX << 3,
        y = mbY << 3,
        xch = mbX << 3,
        ych = mbY << 2;
    vector<uchar, 96>
        control;
    read(SURF_CONTROL, 0, control);
    ushort
        width = control.format<ushort>()[30],
        height = control.format<ushort>()[31];
    float
        sTh = control.format<short>()[35];
    ushort
        picWidthInMB  = width >> 4,
        picHeightInMB = height >> 4;
    uint
        mbIndex = picWidthInMB * mbY + mbX;

    matrix<uchar, 12, 12>
        src = 0;
    matrix<uchar, 6, 12>
        scm = 0;
    matrix<uchar, 8, 8>
        out = 0;
    matrix<uchar, 4, 8>
        och = 0;
    if (sTh > 0)
    {
        read_plane(SURF_SRC, GENX_SURFACE_Y_PLANE, x - 2, y - 2, src);
        read_plane(SURF_SRC, GENX_SURFACE_UV_PLANE, xch - 2, ych - 1, scm);
        matrix<uint, 4, 4>
            Disp = DispersionCalculator(src);

        matrix<float, 4, 4>
            f_disp = Disp,
            h1 = 0.0f,
            h2 = 0.0f,
            hh = 0.0f,
            k0 = 0.0f,
            k1 = 0.0f,
            k2 = 0.0f;
        h1 = cm_exp(-(f_disp / sTh));
        f_disp = Disp * 2.0f;
        h2 = cm_exp(-(f_disp / sTh));
        hh = 1.0f + 4.0f * (h1 + h2);
        k0 = 1.0f / hh;
        k1 = h1 / hh;
        k2 = h2 / hh;

#pragma unroll
        for (uint i = 0; i < 4; i++)
#pragma unroll
            for (uint j = 0; j < 4; j++)
                out.select<2, 1, 2, 1>(i * 2, j * 2) = (src.select<2, 1, 2, 1>(2 + 2 * i, 2 + 2 * j) * k0(i, j)) + (
                    (src.select<2, 1, 2, 1>(2 + 2 * i, 1 + 2 * j) + src.select<2, 1, 2, 1>(2 + 2 * i, 3 + 2 * j) + src.select<2, 1, 2, 1>(1 + 2 * i, 2 + 2 * j) + src.select<2, 1, 2, 1>(3 + 2 * i, 2 + 2 * j))*k1(i, j)) + (
                    (src.select<2, 1, 2, 1>(1 + 2 * i, 1 + 2 * j) + src.select<2, 1, 2, 1>(1 + 2 * i, 3 + 2 * j) + src.select<2, 1, 2, 1>(3 + 2 * i, 1 + 2 * j) + src.select<2, 1, 2, 1>(3 + 2 * i, 3 + 2 * j))*k2(i, j)) + 0.5f;


        matrix<float, 4, 8>
            k0c = 0.0f,
            k1c = 0.0f,
            k2c = 0.0f;
        k0c.select<4, 1, 4, 2>(0, 0) = k0c.select<4, 1, 4, 2>(0, 1) = k0;
        k1c.select<4, 1, 4, 2>(0, 0) = k1c.select<4, 1, 4, 2>(0, 1) = k1;
        k2c.select<4, 1, 4, 2>(0, 0) = k2c.select<4, 1, 4, 2>(0, 1) = k2;

        och = (scm.select<4, 1, 8, 1>(1, 2) * k0c) + (
            (scm.select<4, 1, 8, 1>(1, 0) + scm.select<4, 1, 8, 1>(1, 4) + scm.select<4, 1, 8, 1>(0, 2) + scm.select<4, 1, 8, 1>(2, 2)) * k1c) + (
            (scm.select<4, 1, 8, 1>(0, 0) + scm.select<4, 1, 8, 1>(0, 4) + scm.select<4, 1, 8, 1>(2, 0) + scm.select<4, 1, 8, 1>(2, 4)) * k2c) + 0.5f;
    }
    else
    {
        read_plane(SURF_SRC, GENX_SURFACE_Y_PLANE, x, y, out);
        read_plane(SURF_SRC, GENX_SURFACE_UV_PLANE, xch, ych, och);
    }
    write_plane(SURF_OUT, GENX_SURFACE_Y_PLANE, x, y, out);
    write_plane(SURF_OUT, GENX_SURFACE_UV_PLANE, xch, ych, och);
}

extern "C" _GENX_MAIN_
void SpatialDenoiser_8x8_YV12(
    SurfaceIndex SURF_CONTROL,
    SurfaceIndex SURF_SRC,
    SurfaceIndex SURF_OUT,
    uint         start_xy
)
{
    vector<ushort, 2>
        start_mbXY = start_xy;
    uint
        mbX = get_thread_origin_x() + start_mbXY.format<ushort>()[0],
        mbY = get_thread_origin_y() + start_mbXY.format<ushort>()[1];

    vector<uchar, 96> control;
    read(SURF_CONTROL, 0, control);
    ushort
        width = control.format<ushort>()[30],
        height = control.format<ushort>()[31];
    float
        th = control.format<short>()[32];
    ushort
        picWidthInMB = width >> 4,
        picHeightInMB = height >> 4;
    uint
        mbIndex = picWidthInMB * mbY + mbX;
    uint
        offsetY = height >> 4;
    uint
        x = mbX << 3,
        y = mbY << 3,
        xcu = mbX << 2,
        ycu = mbY << 2,
        xcv = mbX << 2,
        ycv = offsetY + (mbY << 2);

    matrix<uchar, 12, 12>
        src = 0;
    matrix<uchar, 6, 6>
        scu = 0,
        scv = 0;
    matrix<uchar, 8, 8>
        out = 0;
    matrix<uchar, 4, 4>
        ocu = 0,
        ocv = 0;
    read_plane(SURF_SRC, GENX_SURFACE_Y_PLANE, x - 2, y - 2, src);
    read_plane(SURF_SRC, GENX_SURFACE_UV_PLANE, xcu - 1, ycu - 1, scu);
    read_plane(SURF_SRC, GENX_SURFACE_UV_PLANE, xcv - 1, ycv - 1, scv);
    matrix<uint, 4, 4>
        Disp = DispersionCalculator(src);
    matrix<float, 4, 4>
        f_disp = Disp,
        h1 = 0.0f,
        h2 = 0.0f,
        hh = 0.0f,
        k0 = 0.0f,
        k1 = 0.0f,
        k2 = 0.0f;
    h1 = cm_exp(-(f_disp / (th / 10.0f)));
    f_disp = Disp * 2.0f;
    h2 = cm_exp(-(f_disp / (th / 10.0f)));
    hh = 1.0f + 4.0f * (h1 + h2);
    k0 = 1.0f / hh;
    k1 = h1 / hh;
    k2 = h2 / hh;

#pragma unroll
    for (uint i = 0; i < 4; i++)
#pragma unroll
        for (uint j = 0; j < 4; j++)
            out.select<2, 1, 2, 1>(i * 2, j * 2) = (src.select<2, 1, 2, 1>(2 + 2 * i, 2 + 2 * j) * k0(i, j)) + (
                (src.select<2, 1, 2, 1>(2 + 2 * i, 1 + 2 * j) + src.select<2, 1, 2, 1>(2 + 2 * i, 3 + 2 * j) + src.select<2, 1, 2, 1>(1 + 2 * i, 2 + 2 * j) + src.select<2, 1, 2, 1>(3 + 2 * i, 2 + 2 * j))*k1(i, j)) + (
                (src.select<2, 1, 2, 1>(1 + 2 * i, 1 + 2 * j) + src.select<2, 1, 2, 1>(1 + 2 * i, 3 + 2 * j) + src.select<2, 1, 2, 1>(3 + 2 * i, 1 + 2 * j) + src.select<2, 1, 2, 1>(3 + 2 * i, 3 + 2 * j))*k2(i, j)) + 0.5f;

    ocu = (scu.select<4, 1, 4, 1>(1, 1) * k0) + (
        (scu.select<4, 1, 4, 1>(1, 0) + scu.select<4, 1, 4, 1>(1, 2) + scu.select<4, 1, 4, 1>(0, 1) + scu.select<4, 1, 4, 1>(2, 1)) * k1) + (
        (scu.select<4, 1, 4, 1>(0, 0) + scu.select<4, 1, 4, 1>(0, 2) + scu.select<4, 1, 4, 1>(2, 0) + scu.select<4, 1, 4, 1>(2, 2)) * k2) + 0.5f;

    ocv = (scv.select<4, 1, 4, 1>(1, 1) * k0) + (
        (scv.select<4, 1, 4, 1>(1, 0) + scv.select<4, 1, 4, 1>(1, 2) + scv.select<4, 1, 4, 1>(0, 1) + scv.select<4, 1, 4, 1>(2, 1)) * k1) + (
        (scv.select<4, 1, 4, 1>(0, 0) + scv.select<4, 1, 4, 1>(0, 2) + scv.select<4, 1, 4, 1>(2, 0) + scv.select<4, 1, 4, 1>(2, 2)) * k2) + 0.5f;

    write_plane(SURF_OUT, GENX_SURFACE_Y_PLANE, x, y, out);
    write_plane(SURF_OUT, GENX_SURFACE_UV_PLANE, xcu, ycu, ocu);
    write_plane(SURF_OUT, GENX_SURFACE_UV_PLANE, xcv, ycv, ocv);
}

extern "C" _GENX_MAIN_
void SpatialDenoiser_4x4(
    SurfaceIndex SURF_CONTROL,
    SurfaceIndex SURF_SRC,
    SurfaceIndex SURF_OUT,
    uint start_xy
)
{
    vector<ushort, 2>
        start_mbXY = start_xy;
    uint
        mbX = get_thread_origin_x() + start_mbXY.format<ushort>()[0],
        mbY = get_thread_origin_y() + start_mbXY.format<ushort>()[1],
        x = mbX << 2,
        y = mbY << 2;
    vector<uchar, 96>
        control;
    read(SURF_CONTROL, 0, control);
    ushort
        width  = control.format<ushort>()[30],
        height = control.format<ushort>()[31];
    float
        th = control.format<short>()[32] / 10.0f;
    ushort
        picWidthInMB  = width >> 4,
        picHeightInMB = height >> 4;
    uint
        mbIndex = picWidthInMB * mbY + mbX;

    matrix<uchar, 6, 6>
        src = 0;
    matrix<uchar, 4, 4>
        out = 0;
    read_plane(SURF_SRC, GENX_SURFACE_Y_PLANE, x - 1, y - 1, src);
    ushort
        mean4x4 = cm_sum<ushort>(src.select<4, 1, 4, 1>(1, 1));
    mean4x4 >>= 4;

    matrix<uint, 2, 2>
        Disp = 0;
    matrix<ushort, 4, 4>
        test2 = 0;
    int
        data = 0;

    test2 = cm_abs<ushort>(src.select<4, 1, 4, 1>(0, 0) - mean4x4);
    test2 *= test2;

#pragma unroll
    for (int i = 0; i < 2; i++)
#pragma unroll
        for (int j = 0; j < 2; j++)
            Disp(i, j) = cm_sum<uint>(test2.select<2, 1, 2, 1>(i * 2, j * 2));

    Disp = Disp >> 4;
    matrix<float, 2, 2>
        f_disp = Disp,
        h1 = 0.0f,
        h2 = 0.0f,
        hh = 0.0f,
        k0 = 0.0f,
        k1 = 0.0f,
        k2 = 0.0f;
    h1 = cm_exp(-(f_disp / th));
    f_disp = Disp * 2.0f;
    h2 = cm_exp(-(f_disp / th));
    hh = 1.0f + 4.0f * (h1 + h2);
    k0 = 1.0f / hh;
    k1 = h1 / hh;
    k2 = h2 / hh;

#pragma unroll
    for (uint i = 0; i < 2; i++)
#pragma unroll
        for (uint j = 0; j < 2; j++)
            out.select<2, 1, 2, 1>(i * 2, j * 2) = (src.select<2, 1, 2, 1>(1 + 2 * i, 1 + 2 * j) * k0(i, j)) + (
                (src.select<2, 1, 2, 1>(1 + 2 * i, 2 * j) + src.select<2, 1, 2, 1>(1 + 2 * i, 2 + 2 * j) + src.select<2, 1, 2, 1>(2 * i, 1 + 2 * j) + src.select<2, 1, 2, 1>(2 + 2 * i, 1 + 2 * j))*k1(i, j)) + (
                (src.select<2, 1, 2, 1>(2 * i, 2 * j) + src.select<2, 1, 2, 1>(2 * i, 2 + 2 * j) + src.select<2, 1, 2, 1>(2 + 2 * i, 2 * j) + src.select<2, 1, 2, 1>(2 + 2 * i, 2 + 2 * j))*k2(i, j)) + 0.5f;

    write_plane(SURF_OUT, GENX_SURFACE_Y_PLANE, x, y, out);
}