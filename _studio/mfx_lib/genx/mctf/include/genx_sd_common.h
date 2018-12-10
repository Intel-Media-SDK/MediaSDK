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
#include <cm/cm.h>
#include <cm/cmtl.h>

_GENX_ inline
matrix<ushort, 4, 4> Mean4x4Calculator(
    matrix<uchar, 12, 12> src
)
{
    matrix<ushort, 4, 4>
        mean4x4 = 0;
    mean4x4(0, 0) = cm_sum<ushort>(src.select<4, 1, 4, 1>(2, 2));
    mean4x4(0, 1) = cm_sum<ushort>(src.select<4, 1, 4, 1>(2, 4));
    mean4x4(0, 2) = cm_sum<ushort>(src.select<4, 1, 4, 1>(2, 6));
    mean4x4(0, 3) = cm_sum<ushort>(src.select<4, 1, 4, 1>(2, 8));
    mean4x4(1, 0) = cm_sum<ushort>(src.select<4, 1, 4, 1>(4, 2));
    mean4x4(1, 1) = cm_sum<ushort>(src.select<4, 1, 4, 1>(4, 4));
    mean4x4(1, 2) = cm_sum<ushort>(src.select<4, 1, 4, 1>(4, 6));
    mean4x4(1, 3) = cm_sum<ushort>(src.select<4, 1, 4, 1>(4, 8));
    mean4x4(2, 0) = cm_sum<ushort>(src.select<4, 1, 4, 1>(6, 2));
    mean4x4(2, 1) = cm_sum<ushort>(src.select<4, 1, 4, 1>(6, 4));
    mean4x4(2, 2) = cm_sum<ushort>(src.select<4, 1, 4, 1>(6, 6));
    mean4x4(2, 3) = cm_sum<ushort>(src.select<4, 1, 4, 1>(6, 8));
    mean4x4(3, 0) = cm_sum<ushort>(src.select<4, 1, 4, 1>(8, 2));
    mean4x4(3, 1) = cm_sum<ushort>(src.select<4, 1, 4, 1>(8, 4));
    mean4x4(3, 2) = cm_sum<ushort>(src.select<4, 1, 4, 1>(8, 6));
    mean4x4(3, 3) = cm_sum<ushort>(src.select<4, 1, 4, 1>(8, 8));
    return (mean4x4 >>= 4);
}

_GENX_ inline
matrix<uint, 4, 4> DispersionCalculator(
    matrix<uchar, 12, 12> src
)
{
    matrix<uint, 4, 4>
        disper = 0;
    matrix<ushort, 16, 16>
        var = 0;
    matrix<ushort, 4, 4>
        mean4x4 = Mean4x4Calculator(src);

    var.select<4, 1, 4, 1>(0, 0) = cm_abs<ushort>(src.select<4, 1, 4, 1>(0, 0) - mean4x4(0, 0));
    var.select<4, 1, 4, 1>(0, 4) = cm_abs<ushort>(src.select<4, 1, 4, 1>(0, 2) - mean4x4(0, 1));
    var.select<4, 1, 4, 1>(0, 8) = cm_abs<ushort>(src.select<4, 1, 4, 1>(0, 4) - mean4x4(0, 2));
    var.select<4, 1, 4, 1>(0, 12) = cm_abs<ushort>(src.select<4, 1, 4, 1>(0, 6) - mean4x4(0, 3));
    var.select<4, 1, 4, 1>(4, 0) = cm_abs<ushort>(src.select<4, 1, 4, 1>(2, 0) - mean4x4(1, 0));
    var.select<4, 1, 4, 1>(4, 4) = cm_abs<ushort>(src.select<4, 1, 4, 1>(2, 2) - mean4x4(1, 1));
    var.select<4, 1, 4, 1>(4, 8) = cm_abs<ushort>(src.select<4, 1, 4, 1>(2, 4) - mean4x4(1, 2));
    var.select<4, 1, 4, 1>(4, 12) = cm_abs<ushort>(src.select<4, 1, 4, 1>(2, 6) - mean4x4(1, 3));
    var.select<4, 1, 4, 1>(8, 0) = cm_abs<ushort>(src.select<4, 1, 4, 1>(4, 0) - mean4x4(2, 0));
    var.select<4, 1, 4, 1>(8, 4) = cm_abs<ushort>(src.select<4, 1, 4, 1>(4, 2) - mean4x4(2, 1));
    var.select<4, 1, 4, 1>(8, 8) = cm_abs<ushort>(src.select<4, 1, 4, 1>(4, 4) - mean4x4(2, 2));
    var.select<4, 1, 4, 1>(8, 12) = cm_abs<ushort>(src.select<4, 1, 4, 1>(4, 6) - mean4x4(2, 3));
    var.select<4, 1, 4, 1>(12, 0) = cm_abs<ushort>(src.select<4, 1, 4, 1>(6, 0) - mean4x4(3, 0));
    var.select<4, 1, 4, 1>(12, 4) = cm_abs<ushort>(src.select<4, 1, 4, 1>(6, 2) - mean4x4(3, 1));
    var.select<4, 1, 4, 1>(12, 8) = cm_abs<ushort>(src.select<4, 1, 4, 1>(6, 4) - mean4x4(3, 2));
    var.select<4, 1, 4, 1>(12, 12) = cm_abs<ushort>(src.select<4, 1, 4, 1>(6, 6) - mean4x4(3, 3));
    var *= var;

    disper(0, 0) = cm_sum<uint>(var.select<4, 1, 4, 1>(0, 0));
    disper(0, 1) = cm_sum<uint>(var.select<4, 1, 4, 1>(0, 4));
    disper(0, 2) = cm_sum<uint>(var.select<4, 1, 4, 1>(0, 8));
    disper(0, 3) = cm_sum<uint>(var.select<4, 1, 4, 1>(0, 12));
    disper(1, 0) = cm_sum<uint>(var.select<4, 1, 4, 1>(4, 0));
    disper(1, 1) = cm_sum<uint>(var.select<4, 1, 4, 1>(4, 4));
    disper(1, 2) = cm_sum<uint>(var.select<4, 1, 4, 1>(4, 8));
    disper(1, 3) = cm_sum<uint>(var.select<4, 1, 4, 1>(4, 12));
    disper(2, 0) = cm_sum<uint>(var.select<4, 1, 4, 1>(8, 0));
    disper(2, 1) = cm_sum<uint>(var.select<4, 1, 4, 1>(8, 4));
    disper(2, 2) = cm_sum<uint>(var.select<4, 1, 4, 1>(8, 8));
    disper(2, 3) = cm_sum<uint>(var.select<4, 1, 4, 1>(8, 12));
    disper(3, 0) = cm_sum<uint>(var.select<4, 1, 4, 1>(12, 0));
    disper(3, 1) = cm_sum<uint>(var.select<4, 1, 4, 1>(12, 4));
    disper(3, 2) = cm_sum<uint>(var.select<4, 1, 4, 1>(12, 8));
    disper(3, 3) = cm_sum<uint>(var.select<4, 1, 4, 1>(12, 12));

    return(disper >>= 4);
}

_GENX_ inline
matrix<uchar, 4, 8> SpatialDenoiser_8x8_NV12_Chroma(
    SurfaceIndex          SURF_SRC,
    matrix<uchar, 12, 12> src,
    uint                  mbX,
    uint                  mbY,
    short                 th
)
{
    uint
        x = mbX * 8,
        y = mbY * 8,
        xch = mbX * 8,
        ych = mbY * 4;

    matrix<uchar, 6, 12>
        scm = 0;
    matrix<uchar, 8, 8>
        out = 0;
    matrix<uchar, 4, 8>
        och = 0;
    if (th > 0)
    {
        float
            stVal = th;
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
        h1 = cm_exp(-(f_disp / (stVal / 10.0f)));
        f_disp = Disp * 2.0f;
        h2 = cm_exp(-(f_disp / (stVal / 10.0f)));
        hh = 1.0f + 4.0f * (h1 + h2);
        k0 = 1.0f / hh;
        k1 = h1 / hh;
        k2 = h2 / hh;

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
        read_plane(SURF_SRC, GENX_SURFACE_UV_PLANE, xch, ych, och);

    return och;
}

_GENX_ inline
matrix<uchar, 8, 8> SpatialDenoiser_8x8_Y(
    matrix<uchar, 12, 12> src,
    short                 th
)
{
    if (th > 0)
    {
        float
            stVal = th;
        matrix<uchar, 8, 8>
            out = 0;
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
        h1 = cm_exp(-(f_disp / (stVal / 10.0f)));
        f_disp = Disp * 2.0f;
        h2 = cm_exp(-(f_disp / (stVal / 10.0f)));
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

        return out;
    }
    else
        return src.select<8, 1, 8, 1>(2, 2);
}