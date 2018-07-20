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

#include <cm/cm.h>

#define OUT_BLOCK   16  // output pixels computed per thread

#define CM_BUFFER       // output to CmBufferUP instead of CmSurface2DUP

//
// SubSample ibuf into obuf, using full box-filter of source pixels
// Optimized for Gen7.5 (Haswell)
//

// step_w = 6
// step_h = 7
_GENX_MAIN_ void SubSample_480p(SurfaceIndex ibuf, SurfaceIndex obuf, int out_pitch, int step_w, int step_h)
{
    int ix = get_thread_origin_x();
    int iy = get_thread_origin_y();
    int offset = ix * 6 * OUT_BLOCK;

    matrix<uchar, 7, 32> in0;
    matrix<uchar, 7, 32> in1;
    matrix<uchar, 7, 32> in2;

    vector<uint, OUT_BLOCK> acc;

    //
    // first slice
    //
    read(ibuf, offset + 0 * 32, iy * 7 + 0, in0.select<7,1,32,1>(0,0));
    read(ibuf, offset + 1 * 32, iy * 7 + 0, in1.select<7,1,32,1>(0,0));

    acc(0)  = cm_sum<ushort>(in0.select<7,1,6,1>(0,0));
    acc(1)  = cm_sum<ushort>(in0.select<7,1,6,1>(0,6));
    acc(2)  = cm_sum<ushort>(in0.select<7,1,6,1>(0,12));
    acc(3)  = cm_sum<ushort>(in0.select<7,1,6,1>(0,18));
    acc(4)  = cm_sum<ushort>(in0.select<7,1,6,1>(0,24));
    acc(5)  = cm_sum<ushort>(in0.select<7,1,2,1>(0,30));
    acc(5) += cm_sum<ushort>(in1.select<7,1,4,1>(0,0));
    acc(6)  = cm_sum<ushort>(in1.select<7,1,6,1>(0,4));
    acc(7)  = cm_sum<ushort>(in1.select<7,1,6,1>(0,10));
    acc(8)  = cm_sum<ushort>(in1.select<7,1,6,1>(0,16));
    acc(9)  = cm_sum<ushort>(in1.select<7,1,6,1>(0,22));
    acc(10)  = cm_sum<ushort>(in1.select<7,1,4,1>(0,28));

    read(ibuf, offset + 2 * 32, iy * 7 + 0, in2.select<7,1,32,1>(0,0));

    acc(10) += cm_sum<ushort>(in2.select<7,1,2,1>(0,0));
    acc(11)  = cm_sum<ushort>(in2.select<7,1,6,1>(0,2));
    acc(12)  = cm_sum<ushort>(in2.select<7,1,6,1>(0,8));
    acc(13)  = cm_sum<ushort>(in2.select<7,1,6,1>(0,14));
    acc(14)  = cm_sum<ushort>(in2.select<7,1,6,1>(0,20));
    acc(15)  = cm_sum<ushort>(in2.select<7,1,6,1>(0,26));

    vector<uchar, OUT_BLOCK> out = acc / (6 * 7);
#ifdef CM_BUFFER
    write(obuf,  iy * out_pitch + ix * OUT_BLOCK, out);     // OWORD write to CmBuffer
#else
    write(obuf, ix * OUT_BLOCK, iy, out);                   // media write to CmSurface2D
#endif
}

// step_w = 11
// step_h = 11
_GENX_MAIN_ void SubSample_720p(SurfaceIndex ibuf, SurfaceIndex obuf, int out_pitch, int step_w, int step_h)
{
    int ix = get_thread_origin_x();
    int iy = get_thread_origin_y();
    int offset = ix * 11 * OUT_BLOCK;

    matrix<uchar, 8, 32> in0;
    matrix<uchar, 8, 32> in1;
    matrix<uchar, 8, 32> in2;
    matrix<uchar, 8, 32> in3;
    matrix<uchar, 8, 32> in4;
    matrix<uchar, 8, 32> in5;

    vector<uint, OUT_BLOCK> acc;

    //
    // first slice
    //
    read(ibuf, offset + 0 * 32, iy * 11 + 0, in0.select<8,1,32,1>(0,0));
    read(ibuf, offset + 1 * 32, iy * 11 + 0, in1.select<8,1,32,1>(0,0));

    acc(0)  = cm_sum<ushort>(in0.select<8,1,11,1>(0,0));
    acc(1)  = cm_sum<ushort>(in0.select<8,1,11,1>(0,11));
    acc(2)  = cm_sum<ushort>(in0.select<8,1,10,1>(0,22));
    acc(2) += cm_sum<ushort>(in1.select<8,1,1,1>(0,0));
    acc(3)  = cm_sum<ushort>(in1.select<8,1,11,1>(0,1));
    acc(4)  = cm_sum<ushort>(in1.select<8,1,11,1>(0,12));
    acc(5)  = cm_sum<ushort>(in1.select<8,1,9,1>(0,23));

    read(ibuf, offset + 2 * 32, iy * 11 + 0, in2.select<8,1,32,1>(0,0));
    read(ibuf, offset + 3 * 32, iy * 11 + 0, in3.select<8,1,32,1>(0,0));

    acc(5) += cm_sum<ushort>(in2.select<8,1,2,1>(0,0));
    acc(6)  = cm_sum<ushort>(in2.select<8,1,11,1>(0,2));
    acc(7)  = cm_sum<ushort>(in2.select<8,1,11,1>(0,13));
    acc(8)  = cm_sum<ushort>(in2.select<8,1,8,1>(0,24));
    acc(8) += cm_sum<ushort>(in3.select<8,1,3,1>(0,0));
    acc(9)  = cm_sum<ushort>(in3.select<8,1,11,1>(0,3));
    acc(10)  = cm_sum<ushort>(in3.select<8,1,11,1>(0,14));
    acc(11)  = cm_sum<ushort>(in3.select<8,1,7,1>(0,25));

    read(ibuf, offset + 4 * 32, iy * 11 + 0, in4.select<8,1,32,1>(0,0));
    read(ibuf, offset + 5 * 32, iy * 11 + 0, in5.select<8,1,32,1>(0,0));

    acc(11) += cm_sum<ushort>(in4.select<8,1,4,1>(0,0));
    acc(12)  = cm_sum<ushort>(in4.select<8,1,11,1>(0,4));
    acc(13)  = cm_sum<ushort>(in4.select<8,1,11,1>(0,15));
    acc(14)  = cm_sum<ushort>(in4.select<8,1,6,1>(0,26));
    acc(14) += cm_sum<ushort>(in5.select<8,1,5,1>(0,0));
    acc(15)  = cm_sum<ushort>(in5.select<8,1,11,1>(0,5));

    //
    // second slice
    //
    read(ibuf, offset + 0 * 32, iy * 11 + 8, in0.select<3,1,32,1>(0,0));
    read(ibuf, offset + 1 * 32, iy * 11 + 8, in1.select<3,1,32,1>(0,0));

    acc(0) += cm_sum<ushort>(in0.select<3,1,11,1>(0,0));
    acc(1) += cm_sum<ushort>(in0.select<3,1,11,1>(0,11));
    acc(2) += cm_sum<ushort>(in0.select<3,1,10,1>(0,22));
    acc(2) += cm_sum<ushort>(in1.select<3,1,1,1>(0,0));
    acc(3) += cm_sum<ushort>(in1.select<3,1,11,1>(0,1));
    acc(4) += cm_sum<ushort>(in1.select<3,1,11,1>(0,12));
    acc(5) += cm_sum<ushort>(in1.select<3,1,9,1>(0,23));

    read(ibuf, offset + 2 * 32, iy * 11 + 8, in2.select<3,1,32,1>(0,0));
    read(ibuf, offset + 3 * 32, iy * 11 + 8, in3.select<3,1,32,1>(0,0));

    acc(5) += cm_sum<ushort>(in2.select<3,1,2,1>(0,0));
    acc(6) += cm_sum<ushort>(in2.select<3,1,11,1>(0,2));
    acc(7) += cm_sum<ushort>(in2.select<3,1,11,1>(0,13));
    acc(8) += cm_sum<ushort>(in2.select<3,1,8,1>(0,24));
    acc(8) += cm_sum<ushort>(in3.select<3,1,3,1>(0,0));
    acc(9) += cm_sum<ushort>(in3.select<3,1,11,1>(0,3));
    acc(10) += cm_sum<ushort>(in3.select<3,1,11,1>(0,14));
    acc(11) += cm_sum<ushort>(in3.select<3,1,7,1>(0,25));

    read(ibuf, offset + 4 * 32, iy * 11 + 8, in4.select<3,1,32,1>(0,0));
    read(ibuf, offset + 5 * 32, iy * 11 + 8, in5.select<3,1,32,1>(0,0));

    acc(11) += cm_sum<ushort>(in4.select<3,1,4,1>(0,0));
    acc(12) += cm_sum<ushort>(in4.select<3,1,11,1>(0,4));
    acc(13) += cm_sum<ushort>(in4.select<3,1,11,1>(0,15));
    acc(14) += cm_sum<ushort>(in4.select<3,1,6,1>(0,26));
    acc(14) += cm_sum<ushort>(in5.select<3,1,5,1>(0,0));
    acc(15) += cm_sum<ushort>(in5.select<3,1,11,1>(0,5));

    vector<uchar, OUT_BLOCK> out = acc / (11 * 11);
#ifdef CM_BUFFER
    write(obuf,  iy * out_pitch + ix * OUT_BLOCK, out);     // OWORD write to CmBuffer
#else
    write(obuf, ix * OUT_BLOCK, iy, out);                   // media write to CmSurface2D
#endif
}

// step_w = 17
// step_h = 16
_GENX_MAIN_ void SubSample_1080p(SurfaceIndex ibuf, SurfaceIndex obuf, int out_pitch, int step_w, int step_h)
{
    int ix = get_thread_origin_x();
    int iy = get_thread_origin_y();
    int offset = ix * 17 * OUT_BLOCK;

    matrix<uchar, 8, 32> in0;
    matrix<uchar, 8, 32> in1;
    matrix<uchar, 8, 32> in2;
    matrix<uchar, 8, 32> in3;
    matrix<uchar, 8, 32> in4;
    matrix<uchar, 8, 32> in5;
    matrix<uchar, 8, 32> in6;
    matrix<uchar, 8, 32> in7;
    matrix<uchar, 8, 32> in8;

    vector<uint, OUT_BLOCK> acc;

    //
    // first slice
    //
    read(ibuf, offset + 0 * 32, iy * 16 + 0, in0.select<8,1,32,1>(0,0));
    read(ibuf, offset + 1 * 32, iy * 16 + 0, in1.select<8,1,32,1>(0,0));

    acc(0)  = cm_sum<ushort>(in0.select<8,1,17,1>(0,0));
    acc(1)  = cm_sum<ushort>(in0.select<8,1,15,1>(0,17));
    acc(1) += cm_sum<ushort>(in1.select<8,1,2,1>(0,0));
    acc(2)  = cm_sum<ushort>(in1.select<8,1,17,1>(0,2));
    acc(3)  = cm_sum<ushort>(in1.select<8,1,13,1>(0,19));

    read(ibuf, offset + 2 * 32, iy * 16 + 0, in2.select<8,1,32,1>(0,0));
    read(ibuf, offset + 3 * 32, iy * 16 + 0, in3.select<8,1,32,1>(0,0));

    acc(3) += cm_sum<ushort>(in2.select<8,1,4,1>(0,0));
    acc(4)  = cm_sum<ushort>(in2.select<8,1,17,1>(0,4));
    acc(5)  = cm_sum<ushort>(in2.select<8,1,11,1>(0,21));
    acc(5) += cm_sum<ushort>(in3.select<8,1,6,1>(0,0));
    acc(6)  = cm_sum<ushort>(in3.select<8,1,17,1>(0,6));
    acc(7)  = cm_sum<ushort>(in3.select<8,1,9,1>(0,23));

    read(ibuf, offset + 4 * 32, iy * 16 + 0, in4.select<8,1,32,1>(0,0));
    read(ibuf, offset + 5 * 32, iy * 16 + 0, in5.select<8,1,32,1>(0,0));

    acc(7) += cm_sum<ushort>(in4.select<8,1,8,1>(0,0));
    acc(8)  = cm_sum<ushort>(in4.select<8,1,17,1>(0,8));
    acc(9)  = cm_sum<ushort>(in4.select<8,1,7,1>(0,25));
    acc(9) += cm_sum<ushort>(in5.select<8,1,10,1>(0,0));
    acc(10)  = cm_sum<ushort>(in5.select<8,1,17,1>(0,10));
    acc(11)  = cm_sum<ushort>(in5.select<8,1,5,1>(0,27));

    read(ibuf, offset + 6 * 32, iy * 16 + 0, in6.select<8,1,32,1>(0,0));
    read(ibuf, offset + 7 * 32, iy * 16 + 0, in7.select<8,1,32,1>(0,0));

    acc(11) += cm_sum<ushort>(in6.select<8,1,12,1>(0,0));
    acc(12)  = cm_sum<ushort>(in6.select<8,1,17,1>(0,12));
    acc(13)  = cm_sum<ushort>(in6.select<8,1,3,1>(0,29));
    acc(13) += cm_sum<ushort>(in7.select<8,1,14,1>(0,0));
    acc(14)  = cm_sum<ushort>(in7.select<8,1,17,1>(0,14));
    acc(15)  = cm_sum<ushort>(in7.select<8,1,1,1>(0,31));

    read(ibuf, offset + 8 * 32, iy * 16 + 0, in8.select<8,1,32,1>(0,0));

    acc(15) += cm_sum<ushort>(in8.select<8,1,16,1>(0,0));

    //
    // second slice
    //
    read(ibuf, offset + 0 * 32, iy * 16 + 8, in0.select<8,1,32,1>(0,0));
    read(ibuf, offset + 1 * 32, iy * 16 + 8, in1.select<8,1,32,1>(0,0));

    acc(0) += cm_sum<ushort>(in0.select<8,1,17,1>(0,0));
    acc(1) += cm_sum<ushort>(in0.select<8,1,15,1>(0,17));
    acc(1) += cm_sum<ushort>(in1.select<8,1,2,1>(0,0));
    acc(2) += cm_sum<ushort>(in1.select<8,1,17,1>(0,2));
    acc(3) += cm_sum<ushort>(in1.select<8,1,13,1>(0,19));

    read(ibuf, offset + 2 * 32, iy * 16 + 8, in2.select<8,1,32,1>(0,0));
    read(ibuf, offset + 3 * 32, iy * 16 + 8, in3.select<8,1,32,1>(0,0));

    acc(3) += cm_sum<ushort>(in2.select<8,1,4,1>(0,0));
    acc(4) += cm_sum<ushort>(in2.select<8,1,17,1>(0,4));
    acc(5) += cm_sum<ushort>(in2.select<8,1,11,1>(0,21));
    acc(5) += cm_sum<ushort>(in3.select<8,1,6,1>(0,0));
    acc(6) += cm_sum<ushort>(in3.select<8,1,17,1>(0,6));
    acc(7) += cm_sum<ushort>(in3.select<8,1,9,1>(0,23));

    read(ibuf, offset + 4 * 32, iy * 16 + 8, in4.select<8,1,32,1>(0,0));
    read(ibuf, offset + 5 * 32, iy * 16 + 8, in5.select<8,1,32,1>(0,0));

    acc(7) += cm_sum<ushort>(in4.select<8,1,8,1>(0,0));
    acc(8) += cm_sum<ushort>(in4.select<8,1,17,1>(0,8));
    acc(9) += cm_sum<ushort>(in4.select<8,1,7,1>(0,25));
    acc(9) += cm_sum<ushort>(in5.select<8,1,10,1>(0,0));
    acc(10) += cm_sum<ushort>(in5.select<8,1,17,1>(0,10));
    acc(11) += cm_sum<ushort>(in5.select<8,1,5,1>(0,27));

    read(ibuf, offset + 6 * 32, iy * 16 + 8, in6.select<8,1,32,1>(0,0));
    read(ibuf, offset + 7 * 32, iy * 16 + 8, in7.select<8,1,32,1>(0,0));

    acc(11) += cm_sum<ushort>(in6.select<8,1,12,1>(0,0));
    acc(12) += cm_sum<ushort>(in6.select<8,1,17,1>(0,12));
    acc(13) += cm_sum<ushort>(in6.select<8,1,3,1>(0,29));
    acc(13) += cm_sum<ushort>(in7.select<8,1,14,1>(0,0));
    acc(14) += cm_sum<ushort>(in7.select<8,1,17,1>(0,14));
    acc(15) += cm_sum<ushort>(in7.select<8,1,1,1>(0,31));

    read(ibuf, offset + 8 * 32, iy * 16 + 8, in8.select<8,1,32,1>(0,0));

    acc(15) += cm_sum<ushort>(in8.select<8,1,16,1>(0,0));

    vector<uchar, OUT_BLOCK> out = acc / (17 * 16);
#ifdef CM_BUFFER
    write(obuf,  iy * out_pitch + ix * OUT_BLOCK, out);     // OWORD write to CmBuffer
#else
    write(obuf, ix * OUT_BLOCK, iy, out);                   // media write to CmSurface2D
#endif
}

// step_w = 34
// step_h = 33
_GENX_MAIN_ void SubSample_2160p(SurfaceIndex ibuf, SurfaceIndex obuf, int out_pitch, int step_w, int step_h)
{
    int ix = get_thread_origin_x();
    int iy = get_thread_origin_y();
    int offset = ix * 34 * OUT_BLOCK;

    matrix<uchar, 8, 32> in0;
    matrix<uchar, 8, 32> in1;
    matrix<uchar, 8, 32> in2;
    matrix<uchar, 8, 32> in3;
    matrix<uchar, 8, 32> in4;
    matrix<uchar, 8, 32> in5;
    matrix<uchar, 8, 32> in6;
    matrix<uchar, 8, 32> in7;
    matrix<uchar, 8, 32> in8;

    vector<uint, OUT_BLOCK> acc;

    //
    // first slice
    //
    read(ibuf, offset + 0 * 32, iy * 33 + 0, in0.select<8,1,32,1>(0,0));
    read(ibuf, offset + 1 * 32, iy * 33 + 0, in1.select<8,1,32,1>(0,0));

    acc(0)  = cm_sum<ushort>(in0.select<8,1,32,1>(0,0));
    acc(0) += cm_sum<ushort>(in1.select<8,1,2,1>(0,0));
    acc(1)  = cm_sum<ushort>(in1.select<8,1,30,1>(0,2));

    read(ibuf, offset + 2 * 32, iy * 33 + 0, in2.select<8,1,32,1>(0,0));
    read(ibuf, offset + 3 * 32, iy * 33 + 0, in3.select<8,1,32,1>(0,0));

    acc(1) += cm_sum<ushort>(in2.select<8,1,4,1>(0,0));
    acc(2)  = cm_sum<ushort>(in2.select<8,1,28,1>(0,4));
    acc(2) += cm_sum<ushort>(in3.select<8,1,6,1>(0,0));
    acc(3)  = cm_sum<ushort>(in3.select<8,1,26,1>(0,6));

    read(ibuf, offset + 4 * 32, iy * 33 + 0, in4.select<8,1,32,1>(0,0));
    read(ibuf, offset + 5 * 32, iy * 33 + 0, in5.select<8,1,32,1>(0,0));

    acc(3) += cm_sum<ushort>(in4.select<8,1,8,1>(0,0));
    acc(4)  = cm_sum<ushort>(in4.select<8,1,24,1>(0,8));
    acc(4) += cm_sum<ushort>(in5.select<8,1,10,1>(0,0));
    acc(5)  = cm_sum<ushort>(in5.select<8,1,22,1>(0,10));

    read(ibuf, offset + 6 * 32, iy * 33 + 0, in6.select<8,1,32,1>(0,0));
    read(ibuf, offset + 7 * 32, iy * 33 + 0, in7.select<8,1,32,1>(0,0));

    acc(5) += cm_sum<ushort>(in6.select<8,1,12,1>(0,0));
    acc(6)  = cm_sum<ushort>(in6.select<8,1,20,1>(0,12));
    acc(6) += cm_sum<ushort>(in7.select<8,1,14,1>(0,0));
    acc(7)  = cm_sum<ushort>(in7.select<8,1,18,1>(0,14));

    read(ibuf, offset + 8 * 32, iy * 33 + 0, in8.select<8,1,32,1>(0,0));

    acc(7) += cm_sum<ushort>(in8.select<8,1,16,1>(0,0));
    acc(8)  = cm_sum<ushort>(in8.select<8,1,16,1>(0,16));

    read(ibuf, offset + 9 * 32, iy * 33 + 0, in0.select<8,1,32,1>(0,0));
    read(ibuf, offset + 10 * 32, iy * 33 + 0, in1.select<8,1,32,1>(0,0));

    acc(8) += cm_sum<ushort>(in0.select<8,1,18,1>(0,0));
    acc(9)  = cm_sum<ushort>(in0.select<8,1,14,1>(0,18));
    acc(9) += cm_sum<ushort>(in1.select<8,1,20,1>(0,0));
    acc(10)  = cm_sum<ushort>(in1.select<8,1,12,1>(0,20));

    read(ibuf, offset + 11 * 32, iy * 33 + 0, in2.select<8,1,32,1>(0,0));
    read(ibuf, offset + 12 * 32, iy * 33 + 0, in3.select<8,1,32,1>(0,0));

    acc(10) += cm_sum<ushort>(in2.select<8,1,22,1>(0,0));
    acc(11)  = cm_sum<ushort>(in2.select<8,1,10,1>(0,22));
    acc(11) += cm_sum<ushort>(in3.select<8,1,24,1>(0,0));
    acc(12)  = cm_sum<ushort>(in3.select<8,1,8,1>(0,24));

    read(ibuf, offset + 13 * 32, iy * 33 + 0, in4.select<8,1,32,1>(0,0));
    read(ibuf, offset + 14 * 32, iy * 33 + 0, in5.select<8,1,32,1>(0,0));

    acc(12) += cm_sum<ushort>(in4.select<8,1,26,1>(0,0));
    acc(13)  = cm_sum<ushort>(in4.select<8,1,6,1>(0,26));
    acc(13) += cm_sum<ushort>(in5.select<8,1,28,1>(0,0));
    acc(14)  = cm_sum<ushort>(in5.select<8,1,4,1>(0,28));

    read(ibuf, offset + 15 * 32, iy * 33 + 0, in6.select<8,1,32,1>(0,0));
    read(ibuf, offset + 16 * 32, iy * 33 + 0, in7.select<8,1,32,1>(0,0));

    acc(14) += cm_sum<ushort>(in6.select<8,1,30,1>(0,0));
    acc(15)  = cm_sum<ushort>(in6.select<8,1,2,1>(0,30));
    acc(15) += cm_sum<ushort>(in7.select<8,1,32,1>(0,0));

    //
    // second slice
    //
    read(ibuf, offset + 0 * 32, iy * 33 + 8, in0.select<8,1,32,1>(0,0));
    read(ibuf, offset + 1 * 32, iy * 33 + 8, in1.select<8,1,32,1>(0,0));

    acc(0) += cm_sum<ushort>(in0.select<8,1,32,1>(0,0));
    acc(0) += cm_sum<ushort>(in1.select<8,1,2,1>(0,0));
    acc(1) += cm_sum<ushort>(in1.select<8,1,30,1>(0,2));

    read(ibuf, offset + 2 * 32, iy * 33 + 8, in2.select<8,1,32,1>(0,0));
    read(ibuf, offset + 3 * 32, iy * 33 + 8, in3.select<8,1,32,1>(0,0));

    acc(1) += cm_sum<ushort>(in2.select<8,1,4,1>(0,0));
    acc(2) += cm_sum<ushort>(in2.select<8,1,28,1>(0,4));
    acc(2) += cm_sum<ushort>(in3.select<8,1,6,1>(0,0));
    acc(3) += cm_sum<ushort>(in3.select<8,1,26,1>(0,6));

    read(ibuf, offset + 4 * 32, iy * 33 + 8, in4.select<8,1,32,1>(0,0));
    read(ibuf, offset + 5 * 32, iy * 33 + 8, in5.select<8,1,32,1>(0,0));

    acc(3) += cm_sum<ushort>(in4.select<8,1,8,1>(0,0));
    acc(4) += cm_sum<ushort>(in4.select<8,1,24,1>(0,8));
    acc(4) += cm_sum<ushort>(in5.select<8,1,10,1>(0,0));
    acc(5) += cm_sum<ushort>(in5.select<8,1,22,1>(0,10));

    read(ibuf, offset + 6 * 32, iy * 33 + 8, in6.select<8,1,32,1>(0,0));
    read(ibuf, offset + 7 * 32, iy * 33 + 8, in7.select<8,1,32,1>(0,0));

    acc(5) += cm_sum<ushort>(in6.select<8,1,12,1>(0,0));
    acc(6) += cm_sum<ushort>(in6.select<8,1,20,1>(0,12));
    acc(6) += cm_sum<ushort>(in7.select<8,1,14,1>(0,0));
    acc(7) += cm_sum<ushort>(in7.select<8,1,18,1>(0,14));

    read(ibuf, offset + 8 * 32, iy * 33 + 8, in8.select<8,1,32,1>(0,0));

    acc(7) += cm_sum<ushort>(in8.select<8,1,16,1>(0,0));
    acc(8) += cm_sum<ushort>(in8.select<8,1,16,1>(0,16));

    read(ibuf, offset + 9 * 32, iy * 33 + 8, in0.select<8,1,32,1>(0,0));
    read(ibuf, offset + 10 * 32, iy * 33 + 8, in1.select<8,1,32,1>(0,0));

    acc(8) += cm_sum<ushort>(in0.select<8,1,18,1>(0,0));
    acc(9) += cm_sum<ushort>(in0.select<8,1,14,1>(0,18));
    acc(9) += cm_sum<ushort>(in1.select<8,1,20,1>(0,0));
    acc(10) += cm_sum<ushort>(in1.select<8,1,12,1>(0,20));

    read(ibuf, offset + 11 * 32, iy * 33 + 8, in2.select<8,1,32,1>(0,0));
    read(ibuf, offset + 12 * 32, iy * 33 + 8, in3.select<8,1,32,1>(0,0));

    acc(10) += cm_sum<ushort>(in2.select<8,1,22,1>(0,0));
    acc(11) += cm_sum<ushort>(in2.select<8,1,10,1>(0,22));
    acc(11) += cm_sum<ushort>(in3.select<8,1,24,1>(0,0));
    acc(12) += cm_sum<ushort>(in3.select<8,1,8,1>(0,24));

    read(ibuf, offset + 13 * 32, iy * 33 + 8, in4.select<8,1,32,1>(0,0));
    read(ibuf, offset + 14 * 32, iy * 33 + 8, in5.select<8,1,32,1>(0,0));

    acc(12) += cm_sum<ushort>(in4.select<8,1,26,1>(0,0));
    acc(13) += cm_sum<ushort>(in4.select<8,1,6,1>(0,26));
    acc(13) += cm_sum<ushort>(in5.select<8,1,28,1>(0,0));
    acc(14) += cm_sum<ushort>(in5.select<8,1,4,1>(0,28));

    read(ibuf, offset + 15 * 32, iy * 33 + 8, in6.select<8,1,32,1>(0,0));
    read(ibuf, offset + 16 * 32, iy * 33 + 8, in7.select<8,1,32,1>(0,0));

    acc(14) += cm_sum<ushort>(in6.select<8,1,30,1>(0,0));
    acc(15) += cm_sum<ushort>(in6.select<8,1,2,1>(0,30));
    acc(15) += cm_sum<ushort>(in7.select<8,1,32,1>(0,0));

    //
    // third slice
    //
    read(ibuf, offset + 0 * 32, iy * 33 + 16, in0.select<8,1,32,1>(0,0));
    read(ibuf, offset + 1 * 32, iy * 33 + 16, in1.select<8,1,32,1>(0,0));

    acc(0) += cm_sum<ushort>(in0.select<8,1,32,1>(0,0));
    acc(0) += cm_sum<ushort>(in1.select<8,1,2,1>(0,0));
    acc(1) += cm_sum<ushort>(in1.select<8,1,30,1>(0,2));

    read(ibuf, offset + 2 * 32, iy * 33 + 16, in2.select<8,1,32,1>(0,0));
    read(ibuf, offset + 3 * 32, iy * 33 + 16, in3.select<8,1,32,1>(0,0));

    acc(1) += cm_sum<ushort>(in2.select<8,1,4,1>(0,0));
    acc(2) += cm_sum<ushort>(in2.select<8,1,28,1>(0,4));
    acc(2) += cm_sum<ushort>(in3.select<8,1,6,1>(0,0));
    acc(3) += cm_sum<ushort>(in3.select<8,1,26,1>(0,6));

    read(ibuf, offset + 4 * 32, iy * 33 + 16, in4.select<8,1,32,1>(0,0));
    read(ibuf, offset + 5 * 32, iy * 33 + 16, in5.select<8,1,32,1>(0,0));

    acc(3) += cm_sum<ushort>(in4.select<8,1,8,1>(0,0));
    acc(4) += cm_sum<ushort>(in4.select<8,1,24,1>(0,8));
    acc(4) += cm_sum<ushort>(in5.select<8,1,10,1>(0,0));
    acc(5) += cm_sum<ushort>(in5.select<8,1,22,1>(0,10));

    read(ibuf, offset + 6 * 32, iy * 33 + 16, in6.select<8,1,32,1>(0,0));
    read(ibuf, offset + 7 * 32, iy * 33 + 16, in7.select<8,1,32,1>(0,0));

    acc(5) += cm_sum<ushort>(in6.select<8,1,12,1>(0,0));
    acc(6) += cm_sum<ushort>(in6.select<8,1,20,1>(0,12));
    acc(6) += cm_sum<ushort>(in7.select<8,1,14,1>(0,0));
    acc(7) += cm_sum<ushort>(in7.select<8,1,18,1>(0,14));

    read(ibuf, offset + 8 * 32, iy * 33 + 16, in8.select<8,1,32,1>(0,0));

    acc(7) += cm_sum<ushort>(in8.select<8,1,16,1>(0,0));
    acc(8) += cm_sum<ushort>(in8.select<8,1,16,1>(0,16));

    read(ibuf, offset + 9 * 32, iy * 33 + 16, in0.select<8,1,32,1>(0,0));
    read(ibuf, offset + 10 * 32, iy * 33 + 16, in1.select<8,1,32,1>(0,0));

    acc(8) += cm_sum<ushort>(in0.select<8,1,18,1>(0,0));
    acc(9) += cm_sum<ushort>(in0.select<8,1,14,1>(0,18));
    acc(9) += cm_sum<ushort>(in1.select<8,1,20,1>(0,0));
    acc(10) += cm_sum<ushort>(in1.select<8,1,12,1>(0,20));

    read(ibuf, offset + 11 * 32, iy * 33 + 16, in2.select<8,1,32,1>(0,0));
    read(ibuf, offset + 12 * 32, iy * 33 + 16, in3.select<8,1,32,1>(0,0));

    acc(10) += cm_sum<ushort>(in2.select<8,1,22,1>(0,0));
    acc(11) += cm_sum<ushort>(in2.select<8,1,10,1>(0,22));
    acc(11) += cm_sum<ushort>(in3.select<8,1,24,1>(0,0));
    acc(12) += cm_sum<ushort>(in3.select<8,1,8,1>(0,24));

    read(ibuf, offset + 13 * 32, iy * 33 + 16, in4.select<8,1,32,1>(0,0));
    read(ibuf, offset + 14 * 32, iy * 33 + 16, in5.select<8,1,32,1>(0,0));

    acc(12) += cm_sum<ushort>(in4.select<8,1,26,1>(0,0));
    acc(13) += cm_sum<ushort>(in4.select<8,1,6,1>(0,26));
    acc(13) += cm_sum<ushort>(in5.select<8,1,28,1>(0,0));
    acc(14) += cm_sum<ushort>(in5.select<8,1,4,1>(0,28));

    read(ibuf, offset + 15 * 32, iy * 33 + 16, in6.select<8,1,32,1>(0,0));
    read(ibuf, offset + 16 * 32, iy * 33 + 16, in7.select<8,1,32,1>(0,0));

    acc(14) += cm_sum<ushort>(in6.select<8,1,30,1>(0,0));
    acc(15) += cm_sum<ushort>(in6.select<8,1,2,1>(0,30));
    acc(15) += cm_sum<ushort>(in7.select<8,1,32,1>(0,0));

    //
    // fourth slice
    //
    read(ibuf, offset + 0 * 32, iy * 33 + 24, in0.select<8,1,32,1>(0,0));
    read(ibuf, offset + 1 * 32, iy * 33 + 24, in1.select<8,1,32,1>(0,0));

    acc(0) += cm_sum<ushort>(in0.select<8,1,32,1>(0,0));
    acc(0) += cm_sum<ushort>(in1.select<8,1,2,1>(0,0));
    acc(1) += cm_sum<ushort>(in1.select<8,1,30,1>(0,2));

    read(ibuf, offset + 2 * 32, iy * 33 + 24, in2.select<8,1,32,1>(0,0));
    read(ibuf, offset + 3 * 32, iy * 33 + 24, in3.select<8,1,32,1>(0,0));

    acc(1) += cm_sum<ushort>(in2.select<8,1,4,1>(0,0));
    acc(2) += cm_sum<ushort>(in2.select<8,1,28,1>(0,4));
    acc(2) += cm_sum<ushort>(in3.select<8,1,6,1>(0,0));
    acc(3) += cm_sum<ushort>(in3.select<8,1,26,1>(0,6));

    read(ibuf, offset + 4 * 32, iy * 33 + 24, in4.select<8,1,32,1>(0,0));
    read(ibuf, offset + 5 * 32, iy * 33 + 24, in5.select<8,1,32,1>(0,0));

    acc(3) += cm_sum<ushort>(in4.select<8,1,8,1>(0,0));
    acc(4) += cm_sum<ushort>(in4.select<8,1,24,1>(0,8));
    acc(4) += cm_sum<ushort>(in5.select<8,1,10,1>(0,0));
    acc(5) += cm_sum<ushort>(in5.select<8,1,22,1>(0,10));

    read(ibuf, offset + 6 * 32, iy * 33 + 24, in6.select<8,1,32,1>(0,0));
    read(ibuf, offset + 7 * 32, iy * 33 + 24, in7.select<8,1,32,1>(0,0));

    acc(5) += cm_sum<ushort>(in6.select<8,1,12,1>(0,0));
    acc(6) += cm_sum<ushort>(in6.select<8,1,20,1>(0,12));
    acc(6) += cm_sum<ushort>(in7.select<8,1,14,1>(0,0));
    acc(7) += cm_sum<ushort>(in7.select<8,1,18,1>(0,14));

    read(ibuf, offset + 8 * 32, iy * 33 + 24, in8.select<8,1,32,1>(0,0));

    acc(7) += cm_sum<ushort>(in8.select<8,1,16,1>(0,0));
    acc(8) += cm_sum<ushort>(in8.select<8,1,16,1>(0,16));

    read(ibuf, offset + 9 * 32, iy * 33 + 24, in0.select<8,1,32,1>(0,0));
    read(ibuf, offset + 10 * 32, iy * 33 + 24, in1.select<8,1,32,1>(0,0));

    acc(8) += cm_sum<ushort>(in0.select<8,1,18,1>(0,0));
    acc(9) += cm_sum<ushort>(in0.select<8,1,14,1>(0,18));
    acc(9) += cm_sum<ushort>(in1.select<8,1,20,1>(0,0));
    acc(10) += cm_sum<ushort>(in1.select<8,1,12,1>(0,20));

    read(ibuf, offset + 11 * 32, iy * 33 + 24, in2.select<8,1,32,1>(0,0));
    read(ibuf, offset + 12 * 32, iy * 33 + 24, in3.select<8,1,32,1>(0,0));

    acc(10) += cm_sum<ushort>(in2.select<8,1,22,1>(0,0));
    acc(11) += cm_sum<ushort>(in2.select<8,1,10,1>(0,22));
    acc(11) += cm_sum<ushort>(in3.select<8,1,24,1>(0,0));
    acc(12) += cm_sum<ushort>(in3.select<8,1,8,1>(0,24));

    read(ibuf, offset + 13 * 32, iy * 33 + 24, in4.select<8,1,32,1>(0,0));
    read(ibuf, offset + 14 * 32, iy * 33 + 24, in5.select<8,1,32,1>(0,0));

    acc(12) += cm_sum<ushort>(in4.select<8,1,26,1>(0,0));
    acc(13) += cm_sum<ushort>(in4.select<8,1,6,1>(0,26));
    acc(13) += cm_sum<ushort>(in5.select<8,1,28,1>(0,0));
    acc(14) += cm_sum<ushort>(in5.select<8,1,4,1>(0,28));

    read(ibuf, offset + 15 * 32, iy * 33 + 24, in6.select<8,1,32,1>(0,0));
    read(ibuf, offset + 16 * 32, iy * 33 + 24, in7.select<8,1,32,1>(0,0));

    acc(14) += cm_sum<ushort>(in6.select<8,1,30,1>(0,0));
    acc(15) += cm_sum<ushort>(in6.select<8,1,2,1>(0,30));
    acc(15) += cm_sum<ushort>(in7.select<8,1,32,1>(0,0));

    //
    // fifth slice
    //
    read(ibuf, offset + 0 * 32, iy * 33 + 32, in0.select<1,1,32,1>(0,0));
    read(ibuf, offset + 1 * 32, iy * 33 + 32, in1.select<1,1,32,1>(0,0));

    acc(0) += cm_sum<ushort>(in0.select<1,1,32,1>(0,0));
    acc(0) += cm_sum<ushort>(in1.select<1,1,2,1>(0,0));
    acc(1) += cm_sum<ushort>(in1.select<1,1,30,1>(0,2));

    read(ibuf, offset + 2 * 32, iy * 33 + 32, in2.select<1,1,32,1>(0,0));
    read(ibuf, offset + 3 * 32, iy * 33 + 32, in3.select<1,1,32,1>(0,0));

    acc(1) += cm_sum<ushort>(in2.select<1,1,4,1>(0,0));
    acc(2) += cm_sum<ushort>(in2.select<1,1,28,1>(0,4));
    acc(2) += cm_sum<ushort>(in3.select<1,1,6,1>(0,0));
    acc(3) += cm_sum<ushort>(in3.select<1,1,26,1>(0,6));

    read(ibuf, offset + 4 * 32, iy * 33 + 32, in4.select<1,1,32,1>(0,0));
    read(ibuf, offset + 5 * 32, iy * 33 + 32, in5.select<1,1,32,1>(0,0));

    acc(3) += cm_sum<ushort>(in4.select<1,1,8,1>(0,0));
    acc(4) += cm_sum<ushort>(in4.select<1,1,24,1>(0,8));
    acc(4) += cm_sum<ushort>(in5.select<1,1,10,1>(0,0));
    acc(5) += cm_sum<ushort>(in5.select<1,1,22,1>(0,10));

    read(ibuf, offset + 6 * 32, iy * 33 + 32, in6.select<1,1,32,1>(0,0));
    read(ibuf, offset + 7 * 32, iy * 33 + 32, in7.select<1,1,32,1>(0,0));

    acc(5) += cm_sum<ushort>(in6.select<1,1,12,1>(0,0));
    acc(6) += cm_sum<ushort>(in6.select<1,1,20,1>(0,12));
    acc(6) += cm_sum<ushort>(in7.select<1,1,14,1>(0,0));
    acc(7) += cm_sum<ushort>(in7.select<1,1,18,1>(0,14));

    read(ibuf, offset + 8 * 32, iy * 33 + 32, in8.select<1,1,32,1>(0,0));

    acc(7) += cm_sum<ushort>(in8.select<1,1,16,1>(0,0));
    acc(8) += cm_sum<ushort>(in8.select<1,1,16,1>(0,16));

    read(ibuf, offset + 9 * 32, iy * 33 + 32, in0.select<1,1,32,1>(0,0));
    read(ibuf, offset + 10 * 32, iy * 33 + 32, in1.select<1,1,32,1>(0,0));

    acc(8) += cm_sum<ushort>(in0.select<1,1,18,1>(0,0));
    acc(9) += cm_sum<ushort>(in0.select<1,1,14,1>(0,18));
    acc(9) += cm_sum<ushort>(in1.select<1,1,20,1>(0,0));
    acc(10) += cm_sum<ushort>(in1.select<1,1,12,1>(0,20));

    read(ibuf, offset + 11 * 32, iy * 33 + 32, in2.select<1,1,32,1>(0,0));
    read(ibuf, offset + 12 * 32, iy * 33 + 32, in3.select<1,1,32,1>(0,0));

    acc(10) += cm_sum<ushort>(in2.select<1,1,22,1>(0,0));
    acc(11) += cm_sum<ushort>(in2.select<1,1,10,1>(0,22));
    acc(11) += cm_sum<ushort>(in3.select<1,1,24,1>(0,0));
    acc(12) += cm_sum<ushort>(in3.select<1,1,8,1>(0,24));

    read(ibuf, offset + 13 * 32, iy * 33 + 32, in4.select<1,1,32,1>(0,0));
    read(ibuf, offset + 14 * 32, iy * 33 + 32, in5.select<1,1,32,1>(0,0));

    acc(12) += cm_sum<ushort>(in4.select<1,1,26,1>(0,0));
    acc(13) += cm_sum<ushort>(in4.select<1,1,6,1>(0,26));
    acc(13) += cm_sum<ushort>(in5.select<1,1,28,1>(0,0));
    acc(14) += cm_sum<ushort>(in5.select<1,1,4,1>(0,28));

    read(ibuf, offset + 15 * 32, iy * 33 + 32, in6.select<1,1,32,1>(0,0));
    read(ibuf, offset + 16 * 32, iy * 33 + 32, in7.select<1,1,32,1>(0,0));

    acc(14) += cm_sum<ushort>(in6.select<1,1,30,1>(0,0));
    acc(15) += cm_sum<ushort>(in6.select<1,1,2,1>(0,30));
    acc(15) += cm_sum<ushort>(in7.select<1,1,32,1>(0,0));

    vector<uchar, OUT_BLOCK> out = acc / (34 * 33);
#ifdef CM_BUFFER
    write(obuf,  iy * out_pitch + ix * OUT_BLOCK, out);     // OWORD write to CmBuffer
#else
    write(obuf, ix * OUT_BLOCK, iy, out);                   // media write to CmSurface2D
#endif
}

// variable step_w <= 32
_GENX_MAIN_ void SubSample_var_p(SurfaceIndex ibuf, SurfaceIndex obuf, int out_pitch, int step_w, int step_h)
{
    int ix = get_thread_origin_x();
    int iy = get_thread_origin_y();
    int offset = ix * step_w * OUT_BLOCK;

    matrix<uchar, 8, 32> in;

	matrix<ushort, 8, 32> mask;
	mask.merge(0, 1, 0xffffffff << step_w);	// mask of valid input pixels

    vector<uint, OUT_BLOCK> acc = 0;

    for (int j = 0; j < OUT_BLOCK; j++) {

        int i = 0;
        for (; i < step_h-7; i += 8) {

            // read 8x32 block
            read(ibuf, offset + j * step_w, iy * step_h + i, in);

            // mask out invalid pixels
            in.merge(in, 0, mask);

            acc(j) += cm_sum<ushort>(in);
        }
        for (; i < step_h; i++) {

            // read 1x32 block
            read(ibuf, offset + j * step_w, iy * step_h + i, in.row(0));

            // mask out invalid pixels
            in.row(0).merge(in.row(0), 0, mask.row(0));

            acc(j) += cm_sum<ushort>(in.row(0));
        }
    }

    vector<uchar, OUT_BLOCK> out = acc / (step_w * step_h);
#ifdef CM_BUFFER
    write(obuf,  iy * out_pitch + ix * OUT_BLOCK, out);     // OWORD write to CmBuffer
#else
    write(obuf, ix * OUT_BLOCK, iy, out);                   // media write to CmSurface2D
#endif
}

//
// Interlaced versions, top field.
//

// step_w = 6
// step_h = 3
_GENX_MAIN_ void SubSample_480t(SurfaceIndex ibuf, SurfaceIndex obuf, int out_pitch, int step_w, int step_h)
{
    int ix = get_thread_origin_x();
    int iy = get_thread_origin_y();
    int offset = ix * 6 * OUT_BLOCK;

    matrix<uchar, 3, 32> in0;
    matrix<uchar, 3, 32> in1;
    matrix<uchar, 3, 32> in2;

    vector<uint, OUT_BLOCK> acc;

    //
    // first slice
    //
    read(TOP_FIELD(ibuf), offset + 0 * 32, iy * 3 + 0, in0.select<3,1,32,1>(0,0));
    read(TOP_FIELD(ibuf), offset + 1 * 32, iy * 3 + 0, in1.select<3,1,32,1>(0,0));

    acc(0)  = cm_sum<ushort>(in0.select<3,1,6,1>(0,0));
    acc(1)  = cm_sum<ushort>(in0.select<3,1,6,1>(0,6));
    acc(2)  = cm_sum<ushort>(in0.select<3,1,6,1>(0,12));
    acc(3)  = cm_sum<ushort>(in0.select<3,1,6,1>(0,18));
    acc(4)  = cm_sum<ushort>(in0.select<3,1,6,1>(0,24));
    acc(5)  = cm_sum<ushort>(in0.select<3,1,2,1>(0,30));
    acc(5) += cm_sum<ushort>(in1.select<3,1,4,1>(0,0));
    acc(6)  = cm_sum<ushort>(in1.select<3,1,6,1>(0,4));
    acc(7)  = cm_sum<ushort>(in1.select<3,1,6,1>(0,10));
    acc(8)  = cm_sum<ushort>(in1.select<3,1,6,1>(0,16));
    acc(9)  = cm_sum<ushort>(in1.select<3,1,6,1>(0,22));
    acc(10)  = cm_sum<ushort>(in1.select<3,1,4,1>(0,28));
    
    read(TOP_FIELD(ibuf), offset + 2 * 32, iy * 3 + 0, in2.select<3,1,32,1>(0,0));
    
    acc(10) += cm_sum<ushort>(in2.select<3,1,2,1>(0,0));
    acc(11)  = cm_sum<ushort>(in2.select<3,1,6,1>(0,2));
    acc(12)  = cm_sum<ushort>(in2.select<3,1,6,1>(0,8));
    acc(13)  = cm_sum<ushort>(in2.select<3,1,6,1>(0,14));
    acc(14)  = cm_sum<ushort>(in2.select<3,1,6,1>(0,20));
    acc(15)  = cm_sum<ushort>(in2.select<3,1,6,1>(0,26));

    vector<uchar, OUT_BLOCK> out = acc / (6 * 3);
#ifdef CM_BUFFER
    write(obuf,  iy * out_pitch + ix * OUT_BLOCK, out);     // OWORD write to CmBuffer
#else
    write(obuf, ix * OUT_BLOCK, iy, out);                   // media write to CmSurface2D
#endif
}

// step_w = 11
// step_h = 5
_GENX_MAIN_ void SubSample_720t(SurfaceIndex ibuf, SurfaceIndex obuf, int out_pitch, int step_w, int step_h)
{
    int ix = get_thread_origin_x();
    int iy = get_thread_origin_y();
    int offset = ix * 11 * OUT_BLOCK;

    matrix<uchar, 5, 32> in0;
    matrix<uchar, 5, 32> in1;
    matrix<uchar, 5, 32> in2;
    matrix<uchar, 5, 32> in3;
    matrix<uchar, 5, 32> in4;
    matrix<uchar, 5, 32> in5;

    vector<uint, OUT_BLOCK> acc;

    //
    // first slice
    //
    read(TOP_FIELD(ibuf), offset + 0 * 32, iy * 5 + 0, in0.select<5,1,32,1>(0,0));
    read(TOP_FIELD(ibuf), offset + 1 * 32, iy * 5 + 0, in1.select<5,1,32,1>(0,0));

    acc(0)  = cm_sum<ushort>(in0.select<5,1,11,1>(0,0));
    acc(1)  = cm_sum<ushort>(in0.select<5,1,11,1>(0,11));
    acc(2)  = cm_sum<ushort>(in0.select<5,1,10,1>(0,22));
    acc(2) += cm_sum<ushort>(in1.select<5,1,1,1>(0,0));
    acc(3)  = cm_sum<ushort>(in1.select<5,1,11,1>(0,1));
    acc(4)  = cm_sum<ushort>(in1.select<5,1,11,1>(0,12));
    acc(5)  = cm_sum<ushort>(in1.select<5,1,9,1>(0,23));
    
    read(TOP_FIELD(ibuf), offset + 2 * 32, iy * 5 + 0, in2.select<5,1,32,1>(0,0));
    read(TOP_FIELD(ibuf), offset + 3 * 32, iy * 5 + 0, in3.select<5,1,32,1>(0,0));
    
    acc(5) += cm_sum<ushort>(in2.select<5,1,2,1>(0,0));
    acc(6)  = cm_sum<ushort>(in2.select<5,1,11,1>(0,2));
    acc(7)  = cm_sum<ushort>(in2.select<5,1,11,1>(0,13));
    acc(8)  = cm_sum<ushort>(in2.select<5,1,8,1>(0,24));
    acc(8) += cm_sum<ushort>(in3.select<5,1,3,1>(0,0));
    acc(9)  = cm_sum<ushort>(in3.select<5,1,11,1>(0,3));
    acc(10)  = cm_sum<ushort>(in3.select<5,1,11,1>(0,14));
    acc(11)  = cm_sum<ushort>(in3.select<5,1,7,1>(0,25));
    
    read(TOP_FIELD(ibuf), offset + 4 * 32, iy * 5 + 0, in4.select<5,1,32,1>(0,0));
    read(TOP_FIELD(ibuf), offset + 5 * 32, iy * 5 + 0, in5.select<5,1,32,1>(0,0));
    
    acc(11) += cm_sum<ushort>(in4.select<5,1,4,1>(0,0));
    acc(12)  = cm_sum<ushort>(in4.select<5,1,11,1>(0,4));
    acc(13)  = cm_sum<ushort>(in4.select<5,1,11,1>(0,15));
    acc(14)  = cm_sum<ushort>(in4.select<5,1,6,1>(0,26));
    acc(14) += cm_sum<ushort>(in5.select<5,1,5,1>(0,0));
    acc(15)  = cm_sum<ushort>(in5.select<5,1,11,1>(0,5));

    vector<uchar, OUT_BLOCK> out = acc / (11 * 5);
#ifdef CM_BUFFER
    write(obuf,  iy * out_pitch + ix * OUT_BLOCK, out);     // OWORD write to CmBuffer
#else
    write(obuf, ix * OUT_BLOCK, iy, out);                   // media write to CmSurface2D
#endif
}

// step_w = 17
// step_h = 8
_GENX_MAIN_ void SubSample_1080t(SurfaceIndex ibuf, SurfaceIndex obuf, int out_pitch, int step_w, int step_h)
{
    int ix = get_thread_origin_x();
    int iy = get_thread_origin_y();
    int offset = ix * 17 * OUT_BLOCK;

    matrix<uchar, 8, 32> in0;
    matrix<uchar, 8, 32> in1;
    matrix<uchar, 8, 32> in2;
    matrix<uchar, 8, 32> in3;
    matrix<uchar, 8, 32> in4;
    matrix<uchar, 8, 32> in5;
    matrix<uchar, 8, 32> in6;
    matrix<uchar, 8, 32> in7;
    matrix<uchar, 8, 32> in8;

    vector<uint, OUT_BLOCK> acc;

    //
    // first slice
    //
    read(TOP_FIELD(ibuf), offset + 0 * 32, iy * 8 + 0, in0.select<8,1,32,1>(0,0));
    read(TOP_FIELD(ibuf), offset + 1 * 32, iy * 8 + 0, in1.select<8,1,32,1>(0,0));

    acc(0)  = cm_sum<ushort>(in0.select<8,1,17,1>(0,0));
    acc(1)  = cm_sum<ushort>(in0.select<8,1,15,1>(0,17));
    acc(1) += cm_sum<ushort>(in1.select<8,1,2,1>(0,0));
    acc(2)  = cm_sum<ushort>(in1.select<8,1,17,1>(0,2));
    acc(3)  = cm_sum<ushort>(in1.select<8,1,13,1>(0,19));
    
    read(TOP_FIELD(ibuf), offset + 2 * 32, iy * 8 + 0, in2.select<8,1,32,1>(0,0));
    read(TOP_FIELD(ibuf), offset + 3 * 32, iy * 8 + 0, in3.select<8,1,32,1>(0,0));
    
    acc(3) += cm_sum<ushort>(in2.select<8,1,4,1>(0,0));
    acc(4)  = cm_sum<ushort>(in2.select<8,1,17,1>(0,4));
    acc(5)  = cm_sum<ushort>(in2.select<8,1,11,1>(0,21));
    acc(5) += cm_sum<ushort>(in3.select<8,1,6,1>(0,0));
    acc(6)  = cm_sum<ushort>(in3.select<8,1,17,1>(0,6));
    acc(7)  = cm_sum<ushort>(in3.select<8,1,9,1>(0,23));
    
    read(TOP_FIELD(ibuf), offset + 4 * 32, iy * 8 + 0, in4.select<8,1,32,1>(0,0));
    read(TOP_FIELD(ibuf), offset + 5 * 32, iy * 8 + 0, in5.select<8,1,32,1>(0,0));
    
    acc(7) += cm_sum<ushort>(in4.select<8,1,8,1>(0,0));
    acc(8)  = cm_sum<ushort>(in4.select<8,1,17,1>(0,8));
    acc(9)  = cm_sum<ushort>(in4.select<8,1,7,1>(0,25));
    acc(9) += cm_sum<ushort>(in5.select<8,1,10,1>(0,0));
    acc(10)  = cm_sum<ushort>(in5.select<8,1,17,1>(0,10));
    acc(11)  = cm_sum<ushort>(in5.select<8,1,5,1>(0,27));
    
    read(TOP_FIELD(ibuf), offset + 6 * 32, iy * 8 + 0, in6.select<8,1,32,1>(0,0));
    read(TOP_FIELD(ibuf), offset + 7 * 32, iy * 8 + 0, in7.select<8,1,32,1>(0,0));
    
    acc(11) += cm_sum<ushort>(in6.select<8,1,12,1>(0,0));
    acc(12)  = cm_sum<ushort>(in6.select<8,1,17,1>(0,12));
    acc(13)  = cm_sum<ushort>(in6.select<8,1,3,1>(0,29));
    acc(13) += cm_sum<ushort>(in7.select<8,1,14,1>(0,0));
    acc(14)  = cm_sum<ushort>(in7.select<8,1,17,1>(0,14));
    acc(15)  = cm_sum<ushort>(in7.select<8,1,1,1>(0,31));
    
    read(TOP_FIELD(ibuf), offset + 8 * 32, iy * 8 + 0, in8.select<8,1,32,1>(0,0));
    
    acc(15) += cm_sum<ushort>(in8.select<8,1,16,1>(0,0));

    vector<uchar, OUT_BLOCK> out = acc / (17 * 8);
#ifdef CM_BUFFER
    write(obuf,  iy * out_pitch + ix * OUT_BLOCK, out);     // OWORD write to CmBuffer
#else
    write(obuf, ix * OUT_BLOCK, iy, out);                   // media write to CmSurface2D
#endif
}

// step_w = 34
// step_h = 16
_GENX_MAIN_ void SubSample_2160t(SurfaceIndex ibuf, SurfaceIndex obuf, int out_pitch, int step_w, int step_h)
{
    int ix = get_thread_origin_x();
    int iy = get_thread_origin_y();
    int offset = ix * 34 * OUT_BLOCK;

    matrix<uchar, 8, 32> in0;
    matrix<uchar, 8, 32> in1;
    matrix<uchar, 8, 32> in2;
    matrix<uchar, 8, 32> in3;
    matrix<uchar, 8, 32> in4;
    matrix<uchar, 8, 32> in5;
    matrix<uchar, 8, 32> in6;
    matrix<uchar, 8, 32> in7;
    matrix<uchar, 8, 32> in8;

    vector<uint, OUT_BLOCK> acc;

    //
    // first slice
    //
    read(TOP_FIELD(ibuf), offset + 0 * 32, iy * 16 + 0, in0.select<8,1,32,1>(0,0));
    read(TOP_FIELD(ibuf), offset + 1 * 32, iy * 16 + 0, in1.select<8,1,32,1>(0,0));

    acc(0)  = cm_sum<ushort>(in0.select<8,1,32,1>(0,0));
    acc(0) += cm_sum<ushort>(in1.select<8,1,2,1>(0,0));
    acc(1)  = cm_sum<ushort>(in1.select<8,1,30,1>(0,2));
    
    read(TOP_FIELD(ibuf), offset + 2 * 32, iy * 16 + 0, in2.select<8,1,32,1>(0,0));
    read(TOP_FIELD(ibuf), offset + 3 * 32, iy * 16 + 0, in3.select<8,1,32,1>(0,0));
    
    acc(1) += cm_sum<ushort>(in2.select<8,1,4,1>(0,0));
    acc(2)  = cm_sum<ushort>(in2.select<8,1,28,1>(0,4));
    acc(2) += cm_sum<ushort>(in3.select<8,1,6,1>(0,0));
    acc(3)  = cm_sum<ushort>(in3.select<8,1,26,1>(0,6));
    
    read(TOP_FIELD(ibuf), offset + 4 * 32, iy * 16 + 0, in4.select<8,1,32,1>(0,0));
    read(TOP_FIELD(ibuf), offset + 5 * 32, iy * 16 + 0, in5.select<8,1,32,1>(0,0));
    
    acc(3) += cm_sum<ushort>(in4.select<8,1,8,1>(0,0));
    acc(4)  = cm_sum<ushort>(in4.select<8,1,24,1>(0,8));
    acc(4) += cm_sum<ushort>(in5.select<8,1,10,1>(0,0));
    acc(5)  = cm_sum<ushort>(in5.select<8,1,22,1>(0,10));
    
    read(TOP_FIELD(ibuf), offset + 6 * 32, iy * 16 + 0, in6.select<8,1,32,1>(0,0));
    read(TOP_FIELD(ibuf), offset + 7 * 32, iy * 16 + 0, in7.select<8,1,32,1>(0,0));
    
    acc(5) += cm_sum<ushort>(in6.select<8,1,12,1>(0,0));
    acc(6)  = cm_sum<ushort>(in6.select<8,1,20,1>(0,12));
    acc(6) += cm_sum<ushort>(in7.select<8,1,14,1>(0,0));
    acc(7)  = cm_sum<ushort>(in7.select<8,1,18,1>(0,14));
    
    read(TOP_FIELD(ibuf), offset + 8 * 32, iy * 16 + 0, in0.select<8,1,32,1>(0,0));
    read(TOP_FIELD(ibuf), offset + 9 * 32, iy * 16 + 0, in1.select<8,1,32,1>(0,0));
    
    acc(7) += cm_sum<ushort>(in0.select<8,1,16,1>(0,0));
    acc(8)  = cm_sum<ushort>(in0.select<8,1,16,1>(0,16));
    acc(8) += cm_sum<ushort>(in1.select<8,1,18,1>(0,0));
    acc(9)  = cm_sum<ushort>(in1.select<8,1,14,1>(0,18));
    
    read(TOP_FIELD(ibuf), offset + 10 * 32, iy * 16 + 0, in2.select<8,1,32,1>(0,0));
    read(TOP_FIELD(ibuf), offset + 11 * 32, iy * 16 + 0, in3.select<8,1,32,1>(0,0));
    
    acc(9) += cm_sum<ushort>(in2.select<8,1,20,1>(0,0));
    acc(10)  = cm_sum<ushort>(in2.select<8,1,12,1>(0,20));
    acc(10) += cm_sum<ushort>(in3.select<8,1,22,1>(0,0));
    acc(11)  = cm_sum<ushort>(in3.select<8,1,10,1>(0,22));
    
    read(TOP_FIELD(ibuf), offset + 12 * 32, iy * 16 + 0, in4.select<8,1,32,1>(0,0));
    read(TOP_FIELD(ibuf), offset + 13 * 32, iy * 16 + 0, in5.select<8,1,32,1>(0,0));
    
    acc(11) += cm_sum<ushort>(in4.select<8,1,24,1>(0,0));
    acc(12)  = cm_sum<ushort>(in4.select<8,1,8,1>(0,24));
    acc(12) += cm_sum<ushort>(in5.select<8,1,26,1>(0,0));
    acc(13)  = cm_sum<ushort>(in5.select<8,1,6,1>(0,26));
    
    read(TOP_FIELD(ibuf), offset + 14 * 32, iy * 16 + 0, in6.select<8,1,32,1>(0,0));
    read(TOP_FIELD(ibuf), offset + 15 * 32, iy * 16 + 0, in7.select<8,1,32,1>(0,0));
    
    acc(13) += cm_sum<ushort>(in6.select<8,1,28,1>(0,0));
    acc(14)  = cm_sum<ushort>(in6.select<8,1,4,1>(0,28));
    acc(14) += cm_sum<ushort>(in7.select<8,1,30,1>(0,0));
    acc(15)  = cm_sum<ushort>(in7.select<8,1,2,1>(0,30));
    
    read(TOP_FIELD(ibuf), offset + 16 * 32, iy * 16 + 0, in8.select<8,1,32,1>(0,0));
    
    acc(15) += cm_sum<ushort>(in8.select<8,1,32,1>(0,0));

    //
    // second slice
    //
    read(TOP_FIELD(ibuf), offset + 0 * 32, iy * 16 + 8, in0.select<8,1,32,1>(0,0));
    read(TOP_FIELD(ibuf), offset + 1 * 32, iy * 16 + 8, in1.select<8,1,32,1>(0,0));

    acc(0) += cm_sum<ushort>(in0.select<8,1,32,1>(0,0));
    acc(0) += cm_sum<ushort>(in1.select<8,1,2,1>(0,0));
    acc(1) += cm_sum<ushort>(in1.select<8,1,30,1>(0,2));
    
    read(TOP_FIELD(ibuf), offset + 2 * 32, iy * 16 + 8, in2.select<8,1,32,1>(0,0));
    read(TOP_FIELD(ibuf), offset + 3 * 32, iy * 16 + 8, in3.select<8,1,32,1>(0,0));
    
    acc(1) += cm_sum<ushort>(in2.select<8,1,4,1>(0,0));
    acc(2) += cm_sum<ushort>(in2.select<8,1,28,1>(0,4));
    acc(2) += cm_sum<ushort>(in3.select<8,1,6,1>(0,0));
    acc(3) += cm_sum<ushort>(in3.select<8,1,26,1>(0,6));
    
    read(TOP_FIELD(ibuf), offset + 4 * 32, iy * 16 + 8, in4.select<8,1,32,1>(0,0));
    read(TOP_FIELD(ibuf), offset + 5 * 32, iy * 16 + 8, in5.select<8,1,32,1>(0,0));
    
    acc(3) += cm_sum<ushort>(in4.select<8,1,8,1>(0,0));
    acc(4) += cm_sum<ushort>(in4.select<8,1,24,1>(0,8));
    acc(4) += cm_sum<ushort>(in5.select<8,1,10,1>(0,0));
    acc(5) += cm_sum<ushort>(in5.select<8,1,22,1>(0,10));
    
    read(TOP_FIELD(ibuf), offset + 6 * 32, iy * 16 + 8, in6.select<8,1,32,1>(0,0));
    read(TOP_FIELD(ibuf), offset + 7 * 32, iy * 16 + 8, in7.select<8,1,32,1>(0,0));
    
    acc(5) += cm_sum<ushort>(in6.select<8,1,12,1>(0,0));
    acc(6) += cm_sum<ushort>(in6.select<8,1,20,1>(0,12));
    acc(6) += cm_sum<ushort>(in7.select<8,1,14,1>(0,0));
    acc(7) += cm_sum<ushort>(in7.select<8,1,18,1>(0,14));
    
    read(TOP_FIELD(ibuf), offset + 8 * 32, iy * 16 + 8, in0.select<8,1,32,1>(0,0));
    read(TOP_FIELD(ibuf), offset + 9 * 32, iy * 16 + 8, in1.select<8,1,32,1>(0,0));
    
    acc(7) += cm_sum<ushort>(in0.select<8,1,16,1>(0,0));
    acc(8) += cm_sum<ushort>(in0.select<8,1,16,1>(0,16));
    acc(8) += cm_sum<ushort>(in1.select<8,1,18,1>(0,0));
    acc(9) += cm_sum<ushort>(in1.select<8,1,14,1>(0,18));
    
    read(TOP_FIELD(ibuf), offset + 10 * 32, iy * 16 + 8, in2.select<8,1,32,1>(0,0));
    read(TOP_FIELD(ibuf), offset + 11 * 32, iy * 16 + 8, in3.select<8,1,32,1>(0,0));
    
    acc(9) += cm_sum<ushort>(in2.select<8,1,20,1>(0,0));
    acc(10) += cm_sum<ushort>(in2.select<8,1,12,1>(0,20));
    acc(10) += cm_sum<ushort>(in3.select<8,1,22,1>(0,0));
    acc(11) += cm_sum<ushort>(in3.select<8,1,10,1>(0,22));
    
    read(TOP_FIELD(ibuf), offset + 12 * 32, iy * 16 + 8, in4.select<8,1,32,1>(0,0));
    read(TOP_FIELD(ibuf), offset + 13 * 32, iy * 16 + 8, in5.select<8,1,32,1>(0,0));
    
    acc(11) += cm_sum<ushort>(in4.select<8,1,24,1>(0,0));
    acc(12) += cm_sum<ushort>(in4.select<8,1,8,1>(0,24));
    acc(12) += cm_sum<ushort>(in5.select<8,1,26,1>(0,0));
    acc(13) += cm_sum<ushort>(in5.select<8,1,6,1>(0,26));
    
    read(TOP_FIELD(ibuf), offset + 14 * 32, iy * 16 + 8, in6.select<8,1,32,1>(0,0));
    read(TOP_FIELD(ibuf), offset + 15 * 32, iy * 16 + 8, in7.select<8,1,32,1>(0,0));
    
    acc(13) += cm_sum<ushort>(in6.select<8,1,28,1>(0,0));
    acc(14) += cm_sum<ushort>(in6.select<8,1,4,1>(0,28));
    acc(14) += cm_sum<ushort>(in7.select<8,1,30,1>(0,0));
    acc(15) += cm_sum<ushort>(in7.select<8,1,2,1>(0,30));
    
    read(TOP_FIELD(ibuf), offset + 16 * 32, iy * 16 + 8, in8.select<8,1,32,1>(0,0));
    
    acc(15) += cm_sum<ushort>(in8.select<8,1,32,1>(0,0));

    vector<uchar, OUT_BLOCK> out = acc / (34 * 16);
#ifdef CM_BUFFER
    write(obuf,  iy * out_pitch + ix * OUT_BLOCK, out);     // OWORD write to CmBuffer
#else
    write(obuf, ix * OUT_BLOCK, iy, out);                   // media write to CmSurface2D
#endif
}

// variable step_w <= 32
_GENX_MAIN_ void SubSample_var_t(SurfaceIndex ibuf, SurfaceIndex obuf, int out_pitch, int step_w, int step_h)
{
    int ix = get_thread_origin_x();
    int iy = get_thread_origin_y();
    int offset = ix * step_w * OUT_BLOCK;

    matrix<uchar, 8, 32> in;

	matrix<ushort, 8, 32> mask;
	mask.merge(0, 1, 0xffffffff << step_w);	// mask of valid input pixels

    vector<uint, OUT_BLOCK> acc = 0;

    for (int j = 0; j < OUT_BLOCK; j++) {

        int i = 0;
        for (; i < step_h-7; i += 8) {

            // read 8x32 block
            read(TOP_FIELD(ibuf), offset + j * step_w, iy * step_h + i, in);

            // mask out invalid pixels
            in.merge(in, 0, mask);

            acc(j) += cm_sum<ushort>(in);
        }
        for (; i < step_h; i++) {

            // read 1x32 block
            read(TOP_FIELD(ibuf), offset + j * step_w, iy * step_h + i, in.row(0));

            // mask out invalid pixels
            in.row(0).merge(in.row(0), 0, mask.row(0));

            acc(j) += cm_sum<ushort>(in.row(0));
        }
    }

    vector<uchar, OUT_BLOCK> out = acc / (step_w * step_h);
#ifdef CM_BUFFER
    write(obuf,  iy * out_pitch + ix * OUT_BLOCK, out);     // OWORD write to CmBuffer
#else
    write(obuf, ix * OUT_BLOCK, iy, out);                   // media write to CmSurface2D
#endif
}

//
// Interlaced versions, bottom field.
//

// step_w = 6
// step_h = 3
_GENX_MAIN_ void SubSample_480b(SurfaceIndex ibuf, SurfaceIndex obuf, int out_pitch, int step_w, int step_h)
{
    int ix = get_thread_origin_x();
    int iy = get_thread_origin_y();
    int offset = ix * 6 * OUT_BLOCK;

    matrix<uchar, 3, 32> in0;
    matrix<uchar, 3, 32> in1;
    matrix<uchar, 3, 32> in2;

    vector<uint, OUT_BLOCK> acc;

    //
    // first slice
    //
    read(BOTTOM_FIELD(ibuf), offset + 0 * 32, iy * 3 + 0, in0.select<3,1,32,1>(0,0));
    read(BOTTOM_FIELD(ibuf), offset + 1 * 32, iy * 3 + 0, in1.select<3,1,32,1>(0,0));

    acc(0)  = cm_sum<ushort>(in0.select<3,1,6,1>(0,0));
    acc(1)  = cm_sum<ushort>(in0.select<3,1,6,1>(0,6));
    acc(2)  = cm_sum<ushort>(in0.select<3,1,6,1>(0,12));
    acc(3)  = cm_sum<ushort>(in0.select<3,1,6,1>(0,18));
    acc(4)  = cm_sum<ushort>(in0.select<3,1,6,1>(0,24));
    acc(5)  = cm_sum<ushort>(in0.select<3,1,2,1>(0,30));
    acc(5) += cm_sum<ushort>(in1.select<3,1,4,1>(0,0));
    acc(6)  = cm_sum<ushort>(in1.select<3,1,6,1>(0,4));
    acc(7)  = cm_sum<ushort>(in1.select<3,1,6,1>(0,10));
    acc(8)  = cm_sum<ushort>(in1.select<3,1,6,1>(0,16));
    acc(9)  = cm_sum<ushort>(in1.select<3,1,6,1>(0,22));
    acc(10)  = cm_sum<ushort>(in1.select<3,1,4,1>(0,28));
    
    read(BOTTOM_FIELD(ibuf), offset + 2 * 32, iy * 3 + 0, in2.select<3,1,32,1>(0,0));
    
    acc(10) += cm_sum<ushort>(in2.select<3,1,2,1>(0,0));
    acc(11)  = cm_sum<ushort>(in2.select<3,1,6,1>(0,2));
    acc(12)  = cm_sum<ushort>(in2.select<3,1,6,1>(0,8));
    acc(13)  = cm_sum<ushort>(in2.select<3,1,6,1>(0,14));
    acc(14)  = cm_sum<ushort>(in2.select<3,1,6,1>(0,20));
    acc(15)  = cm_sum<ushort>(in2.select<3,1,6,1>(0,26));

    vector<uchar, OUT_BLOCK> out = acc / (6 * 3);
#ifdef CM_BUFFER
    write(obuf,  iy * out_pitch + ix * OUT_BLOCK, out);     // OWORD write to CmBuffer
#else
    write(obuf, ix * OUT_BLOCK, iy, out);                   // media write to CmSurface2D
#endif
}

// step_w = 11
// step_h = 5
_GENX_MAIN_ void SubSample_720b(SurfaceIndex ibuf, SurfaceIndex obuf, int out_pitch, int step_w, int step_h)
{
    int ix = get_thread_origin_x();
    int iy = get_thread_origin_y();
    int offset = ix * 11 * OUT_BLOCK;

    matrix<uchar, 5, 32> in0;
    matrix<uchar, 5, 32> in1;
    matrix<uchar, 5, 32> in2;
    matrix<uchar, 5, 32> in3;
    matrix<uchar, 5, 32> in4;
    matrix<uchar, 5, 32> in5;

    vector<uint, OUT_BLOCK> acc;

    //
    // first slice
    //
    read(BOTTOM_FIELD(ibuf), offset + 0 * 32, iy * 5 + 0, in0.select<5,1,32,1>(0,0));
    read(BOTTOM_FIELD(ibuf), offset + 1 * 32, iy * 5 + 0, in1.select<5,1,32,1>(0,0));

    acc(0)  = cm_sum<ushort>(in0.select<5,1,11,1>(0,0));
    acc(1)  = cm_sum<ushort>(in0.select<5,1,11,1>(0,11));
    acc(2)  = cm_sum<ushort>(in0.select<5,1,10,1>(0,22));
    acc(2) += cm_sum<ushort>(in1.select<5,1,1,1>(0,0));
    acc(3)  = cm_sum<ushort>(in1.select<5,1,11,1>(0,1));
    acc(4)  = cm_sum<ushort>(in1.select<5,1,11,1>(0,12));
    acc(5)  = cm_sum<ushort>(in1.select<5,1,9,1>(0,23));
    
    read(BOTTOM_FIELD(ibuf), offset + 2 * 32, iy * 5 + 0, in2.select<5,1,32,1>(0,0));
    read(BOTTOM_FIELD(ibuf), offset + 3 * 32, iy * 5 + 0, in3.select<5,1,32,1>(0,0));
    
    acc(5) += cm_sum<ushort>(in2.select<5,1,2,1>(0,0));
    acc(6)  = cm_sum<ushort>(in2.select<5,1,11,1>(0,2));
    acc(7)  = cm_sum<ushort>(in2.select<5,1,11,1>(0,13));
    acc(8)  = cm_sum<ushort>(in2.select<5,1,8,1>(0,24));
    acc(8) += cm_sum<ushort>(in3.select<5,1,3,1>(0,0));
    acc(9)  = cm_sum<ushort>(in3.select<5,1,11,1>(0,3));
    acc(10)  = cm_sum<ushort>(in3.select<5,1,11,1>(0,14));
    acc(11)  = cm_sum<ushort>(in3.select<5,1,7,1>(0,25));
    
    read(BOTTOM_FIELD(ibuf), offset + 4 * 32, iy * 5 + 0, in4.select<5,1,32,1>(0,0));
    read(BOTTOM_FIELD(ibuf), offset + 5 * 32, iy * 5 + 0, in5.select<5,1,32,1>(0,0));
    
    acc(11) += cm_sum<ushort>(in4.select<5,1,4,1>(0,0));
    acc(12)  = cm_sum<ushort>(in4.select<5,1,11,1>(0,4));
    acc(13)  = cm_sum<ushort>(in4.select<5,1,11,1>(0,15));
    acc(14)  = cm_sum<ushort>(in4.select<5,1,6,1>(0,26));
    acc(14) += cm_sum<ushort>(in5.select<5,1,5,1>(0,0));
    acc(15)  = cm_sum<ushort>(in5.select<5,1,11,1>(0,5));

    vector<uchar, OUT_BLOCK> out = acc / (11 * 5);
#ifdef CM_BUFFER
    write(obuf,  iy * out_pitch + ix * OUT_BLOCK, out);     // OWORD write to CmBuffer
#else
    write(obuf, ix * OUT_BLOCK, iy, out);                   // media write to CmSurface2D
#endif
}

// step_w = 17
// step_h = 8
_GENX_MAIN_ void SubSample_1080b(SurfaceIndex ibuf, SurfaceIndex obuf, int out_pitch, int step_w, int step_h)
{
    int ix = get_thread_origin_x();
    int iy = get_thread_origin_y();
    int offset = ix * 17 * OUT_BLOCK;

    matrix<uchar, 8, 32> in0;
    matrix<uchar, 8, 32> in1;
    matrix<uchar, 8, 32> in2;
    matrix<uchar, 8, 32> in3;
    matrix<uchar, 8, 32> in4;
    matrix<uchar, 8, 32> in5;
    matrix<uchar, 8, 32> in6;
    matrix<uchar, 8, 32> in7;
    matrix<uchar, 8, 32> in8;

    vector<uint, OUT_BLOCK> acc;

    //
    // first slice
    //
    read(BOTTOM_FIELD(ibuf), offset + 0 * 32, iy * 8 + 0, in0.select<8,1,32,1>(0,0));
    read(BOTTOM_FIELD(ibuf), offset + 1 * 32, iy * 8 + 0, in1.select<8,1,32,1>(0,0));

    acc(0)  = cm_sum<ushort>(in0.select<8,1,17,1>(0,0));
    acc(1)  = cm_sum<ushort>(in0.select<8,1,15,1>(0,17));
    acc(1) += cm_sum<ushort>(in1.select<8,1,2,1>(0,0));
    acc(2)  = cm_sum<ushort>(in1.select<8,1,17,1>(0,2));
    acc(3)  = cm_sum<ushort>(in1.select<8,1,13,1>(0,19));
    
    read(BOTTOM_FIELD(ibuf), offset + 2 * 32, iy * 8 + 0, in2.select<8,1,32,1>(0,0));
    read(BOTTOM_FIELD(ibuf), offset + 3 * 32, iy * 8 + 0, in3.select<8,1,32,1>(0,0));
    
    acc(3) += cm_sum<ushort>(in2.select<8,1,4,1>(0,0));
    acc(4)  = cm_sum<ushort>(in2.select<8,1,17,1>(0,4));
    acc(5)  = cm_sum<ushort>(in2.select<8,1,11,1>(0,21));
    acc(5) += cm_sum<ushort>(in3.select<8,1,6,1>(0,0));
    acc(6)  = cm_sum<ushort>(in3.select<8,1,17,1>(0,6));
    acc(7)  = cm_sum<ushort>(in3.select<8,1,9,1>(0,23));
    
    read(BOTTOM_FIELD(ibuf), offset + 4 * 32, iy * 8 + 0, in4.select<8,1,32,1>(0,0));
    read(BOTTOM_FIELD(ibuf), offset + 5 * 32, iy * 8 + 0, in5.select<8,1,32,1>(0,0));
    
    acc(7) += cm_sum<ushort>(in4.select<8,1,8,1>(0,0));
    acc(8)  = cm_sum<ushort>(in4.select<8,1,17,1>(0,8));
    acc(9)  = cm_sum<ushort>(in4.select<8,1,7,1>(0,25));
    acc(9) += cm_sum<ushort>(in5.select<8,1,10,1>(0,0));
    acc(10)  = cm_sum<ushort>(in5.select<8,1,17,1>(0,10));
    acc(11)  = cm_sum<ushort>(in5.select<8,1,5,1>(0,27));
    
    read(BOTTOM_FIELD(ibuf), offset + 6 * 32, iy * 8 + 0, in6.select<8,1,32,1>(0,0));
    read(BOTTOM_FIELD(ibuf), offset + 7 * 32, iy * 8 + 0, in7.select<8,1,32,1>(0,0));
    
    acc(11) += cm_sum<ushort>(in6.select<8,1,12,1>(0,0));
    acc(12)  = cm_sum<ushort>(in6.select<8,1,17,1>(0,12));
    acc(13)  = cm_sum<ushort>(in6.select<8,1,3,1>(0,29));
    acc(13) += cm_sum<ushort>(in7.select<8,1,14,1>(0,0));
    acc(14)  = cm_sum<ushort>(in7.select<8,1,17,1>(0,14));
    acc(15)  = cm_sum<ushort>(in7.select<8,1,1,1>(0,31));
    
    read(BOTTOM_FIELD(ibuf), offset + 8 * 32, iy * 8 + 0, in8.select<8,1,32,1>(0,0));
    
    acc(15) += cm_sum<ushort>(in8.select<8,1,16,1>(0,0));

    vector<uchar, OUT_BLOCK> out = acc / (17 * 8);
#ifdef CM_BUFFER
    write(obuf,  iy * out_pitch + ix * OUT_BLOCK, out);     // OWORD write to CmBuffer
#else
    write(obuf, ix * OUT_BLOCK, iy, out);                   // media write to CmSurface2D
#endif
}

// step_w = 34
// step_h = 16
_GENX_MAIN_ void SubSample_2160b(SurfaceIndex ibuf, SurfaceIndex obuf, int out_pitch, int step_w, int step_h)
{
    int ix = get_thread_origin_x();
    int iy = get_thread_origin_y();
    int offset = ix * 34 * OUT_BLOCK;

    matrix<uchar, 8, 32> in0;
    matrix<uchar, 8, 32> in1;
    matrix<uchar, 8, 32> in2;
    matrix<uchar, 8, 32> in3;
    matrix<uchar, 8, 32> in4;
    matrix<uchar, 8, 32> in5;
    matrix<uchar, 8, 32> in6;
    matrix<uchar, 8, 32> in7;
    matrix<uchar, 8, 32> in8;

    vector<uint, OUT_BLOCK> acc;

    //
    // first slice
    //
    read(BOTTOM_FIELD(ibuf), offset + 0 * 32, iy * 16 + 0, in0.select<8,1,32,1>(0,0));
    read(BOTTOM_FIELD(ibuf), offset + 1 * 32, iy * 16 + 0, in1.select<8,1,32,1>(0,0));

    acc(0)  = cm_sum<ushort>(in0.select<8,1,32,1>(0,0));
    acc(0) += cm_sum<ushort>(in1.select<8,1,2,1>(0,0));
    acc(1)  = cm_sum<ushort>(in1.select<8,1,30,1>(0,2));
    
    read(BOTTOM_FIELD(ibuf), offset + 2 * 32, iy * 16 + 0, in2.select<8,1,32,1>(0,0));
    read(BOTTOM_FIELD(ibuf), offset + 3 * 32, iy * 16 + 0, in3.select<8,1,32,1>(0,0));
    
    acc(1) += cm_sum<ushort>(in2.select<8,1,4,1>(0,0));
    acc(2)  = cm_sum<ushort>(in2.select<8,1,28,1>(0,4));
    acc(2) += cm_sum<ushort>(in3.select<8,1,6,1>(0,0));
    acc(3)  = cm_sum<ushort>(in3.select<8,1,26,1>(0,6));
    
    read(BOTTOM_FIELD(ibuf), offset + 4 * 32, iy * 16 + 0, in4.select<8,1,32,1>(0,0));
    read(BOTTOM_FIELD(ibuf), offset + 5 * 32, iy * 16 + 0, in5.select<8,1,32,1>(0,0));
    
    acc(3) += cm_sum<ushort>(in4.select<8,1,8,1>(0,0));
    acc(4)  = cm_sum<ushort>(in4.select<8,1,24,1>(0,8));
    acc(4) += cm_sum<ushort>(in5.select<8,1,10,1>(0,0));
    acc(5)  = cm_sum<ushort>(in5.select<8,1,22,1>(0,10));
    
    read(BOTTOM_FIELD(ibuf), offset + 6 * 32, iy * 16 + 0, in6.select<8,1,32,1>(0,0));
    read(BOTTOM_FIELD(ibuf), offset + 7 * 32, iy * 16 + 0, in7.select<8,1,32,1>(0,0));
    
    acc(5) += cm_sum<ushort>(in6.select<8,1,12,1>(0,0));
    acc(6)  = cm_sum<ushort>(in6.select<8,1,20,1>(0,12));
    acc(6) += cm_sum<ushort>(in7.select<8,1,14,1>(0,0));
    acc(7)  = cm_sum<ushort>(in7.select<8,1,18,1>(0,14));
    
    read(BOTTOM_FIELD(ibuf), offset + 8 * 32, iy * 16 + 0, in0.select<8,1,32,1>(0,0));
    read(BOTTOM_FIELD(ibuf), offset + 9 * 32, iy * 16 + 0, in1.select<8,1,32,1>(0,0));
    
    acc(7) += cm_sum<ushort>(in0.select<8,1,16,1>(0,0));
    acc(8)  = cm_sum<ushort>(in0.select<8,1,16,1>(0,16));
    acc(8) += cm_sum<ushort>(in1.select<8,1,18,1>(0,0));
    acc(9)  = cm_sum<ushort>(in1.select<8,1,14,1>(0,18));
    
    read(BOTTOM_FIELD(ibuf), offset + 10 * 32, iy * 16 + 0, in2.select<8,1,32,1>(0,0));
    read(BOTTOM_FIELD(ibuf), offset + 11 * 32, iy * 16 + 0, in3.select<8,1,32,1>(0,0));
    
    acc(9) += cm_sum<ushort>(in2.select<8,1,20,1>(0,0));
    acc(10)  = cm_sum<ushort>(in2.select<8,1,12,1>(0,20));
    acc(10) += cm_sum<ushort>(in3.select<8,1,22,1>(0,0));
    acc(11)  = cm_sum<ushort>(in3.select<8,1,10,1>(0,22));
    
    read(BOTTOM_FIELD(ibuf), offset + 12 * 32, iy * 16 + 0, in4.select<8,1,32,1>(0,0));
    read(BOTTOM_FIELD(ibuf), offset + 13 * 32, iy * 16 + 0, in5.select<8,1,32,1>(0,0));
    
    acc(11) += cm_sum<ushort>(in4.select<8,1,24,1>(0,0));
    acc(12)  = cm_sum<ushort>(in4.select<8,1,8,1>(0,24));
    acc(12) += cm_sum<ushort>(in5.select<8,1,26,1>(0,0));
    acc(13)  = cm_sum<ushort>(in5.select<8,1,6,1>(0,26));
    
    read(BOTTOM_FIELD(ibuf), offset + 14 * 32, iy * 16 + 0, in6.select<8,1,32,1>(0,0));
    read(BOTTOM_FIELD(ibuf), offset + 15 * 32, iy * 16 + 0, in7.select<8,1,32,1>(0,0));
    
    acc(13) += cm_sum<ushort>(in6.select<8,1,28,1>(0,0));
    acc(14)  = cm_sum<ushort>(in6.select<8,1,4,1>(0,28));
    acc(14) += cm_sum<ushort>(in7.select<8,1,30,1>(0,0));
    acc(15)  = cm_sum<ushort>(in7.select<8,1,2,1>(0,30));
    
    read(BOTTOM_FIELD(ibuf), offset + 16 * 32, iy * 16 + 0, in8.select<8,1,32,1>(0,0));
    
    acc(15) += cm_sum<ushort>(in8.select<8,1,32,1>(0,0));

    //
    // second slice
    //
    read(BOTTOM_FIELD(ibuf), offset + 0 * 32, iy * 16 + 8, in0.select<8,1,32,1>(0,0));
    read(BOTTOM_FIELD(ibuf), offset + 1 * 32, iy * 16 + 8, in1.select<8,1,32,1>(0,0));

    acc(0) += cm_sum<ushort>(in0.select<8,1,32,1>(0,0));
    acc(0) += cm_sum<ushort>(in1.select<8,1,2,1>(0,0));
    acc(1) += cm_sum<ushort>(in1.select<8,1,30,1>(0,2));
    
    read(BOTTOM_FIELD(ibuf), offset + 2 * 32, iy * 16 + 8, in2.select<8,1,32,1>(0,0));
    read(BOTTOM_FIELD(ibuf), offset + 3 * 32, iy * 16 + 8, in3.select<8,1,32,1>(0,0));
    
    acc(1) += cm_sum<ushort>(in2.select<8,1,4,1>(0,0));
    acc(2) += cm_sum<ushort>(in2.select<8,1,28,1>(0,4));
    acc(2) += cm_sum<ushort>(in3.select<8,1,6,1>(0,0));
    acc(3) += cm_sum<ushort>(in3.select<8,1,26,1>(0,6));
    
    read(BOTTOM_FIELD(ibuf), offset + 4 * 32, iy * 16 + 8, in4.select<8,1,32,1>(0,0));
    read(BOTTOM_FIELD(ibuf), offset + 5 * 32, iy * 16 + 8, in5.select<8,1,32,1>(0,0));
    
    acc(3) += cm_sum<ushort>(in4.select<8,1,8,1>(0,0));
    acc(4) += cm_sum<ushort>(in4.select<8,1,24,1>(0,8));
    acc(4) += cm_sum<ushort>(in5.select<8,1,10,1>(0,0));
    acc(5) += cm_sum<ushort>(in5.select<8,1,22,1>(0,10));
    
    read(BOTTOM_FIELD(ibuf), offset + 6 * 32, iy * 16 + 8, in6.select<8,1,32,1>(0,0));
    read(BOTTOM_FIELD(ibuf), offset + 7 * 32, iy * 16 + 8, in7.select<8,1,32,1>(0,0));
    
    acc(5) += cm_sum<ushort>(in6.select<8,1,12,1>(0,0));
    acc(6) += cm_sum<ushort>(in6.select<8,1,20,1>(0,12));
    acc(6) += cm_sum<ushort>(in7.select<8,1,14,1>(0,0));
    acc(7) += cm_sum<ushort>(in7.select<8,1,18,1>(0,14));
    
    read(BOTTOM_FIELD(ibuf), offset + 8 * 32, iy * 16 + 8, in0.select<8,1,32,1>(0,0));
    read(BOTTOM_FIELD(ibuf), offset + 9 * 32, iy * 16 + 8, in1.select<8,1,32,1>(0,0));
    
    acc(7) += cm_sum<ushort>(in0.select<8,1,16,1>(0,0));
    acc(8) += cm_sum<ushort>(in0.select<8,1,16,1>(0,16));
    acc(8) += cm_sum<ushort>(in1.select<8,1,18,1>(0,0));
    acc(9) += cm_sum<ushort>(in1.select<8,1,14,1>(0,18));
    
    read(BOTTOM_FIELD(ibuf), offset + 10 * 32, iy * 16 + 8, in2.select<8,1,32,1>(0,0));
    read(BOTTOM_FIELD(ibuf), offset + 11 * 32, iy * 16 + 8, in3.select<8,1,32,1>(0,0));
    
    acc(9) += cm_sum<ushort>(in2.select<8,1,20,1>(0,0));
    acc(10) += cm_sum<ushort>(in2.select<8,1,12,1>(0,20));
    acc(10) += cm_sum<ushort>(in3.select<8,1,22,1>(0,0));
    acc(11) += cm_sum<ushort>(in3.select<8,1,10,1>(0,22));
    
    read(BOTTOM_FIELD(ibuf), offset + 12 * 32, iy * 16 + 8, in4.select<8,1,32,1>(0,0));
    read(BOTTOM_FIELD(ibuf), offset + 13 * 32, iy * 16 + 8, in5.select<8,1,32,1>(0,0));
    
    acc(11) += cm_sum<ushort>(in4.select<8,1,24,1>(0,0));
    acc(12) += cm_sum<ushort>(in4.select<8,1,8,1>(0,24));
    acc(12) += cm_sum<ushort>(in5.select<8,1,26,1>(0,0));
    acc(13) += cm_sum<ushort>(in5.select<8,1,6,1>(0,26));
    
    read(BOTTOM_FIELD(ibuf), offset + 14 * 32, iy * 16 + 8, in6.select<8,1,32,1>(0,0));
    read(BOTTOM_FIELD(ibuf), offset + 15 * 32, iy * 16 + 8, in7.select<8,1,32,1>(0,0));
    
    acc(13) += cm_sum<ushort>(in6.select<8,1,28,1>(0,0));
    acc(14) += cm_sum<ushort>(in6.select<8,1,4,1>(0,28));
    acc(14) += cm_sum<ushort>(in7.select<8,1,30,1>(0,0));
    acc(15) += cm_sum<ushort>(in7.select<8,1,2,1>(0,30));
    
    read(BOTTOM_FIELD(ibuf), offset + 16 * 32, iy * 16 + 8, in8.select<8,1,32,1>(0,0));
    
    acc(15) += cm_sum<ushort>(in8.select<8,1,32,1>(0,0));

    vector<uchar, OUT_BLOCK> out = acc / (34 * 16);
#ifdef CM_BUFFER
    write(obuf,  iy * out_pitch + ix * OUT_BLOCK, out);     // OWORD write to CmBuffer
#else
    write(obuf, ix * OUT_BLOCK, iy, out);                   // media write to CmSurface2D
#endif
}

// variable step_w <= 32
_GENX_MAIN_ void SubSample_var_b(SurfaceIndex ibuf, SurfaceIndex obuf, int out_pitch, int step_w, int step_h)
{
    int ix = get_thread_origin_x();
    int iy = get_thread_origin_y();
    int offset = ix * step_w * OUT_BLOCK;

    matrix<uchar, 8, 32> in;

	matrix<ushort, 8, 32> mask;
	mask.merge(0, 1, 0xffffffff << step_w);	// mask of valid input pixels

    vector<uint, OUT_BLOCK> acc = 0;

    for (int j = 0; j < OUT_BLOCK; j++) {

        int i = 0;
        for (; i < step_h-7; i += 8) {

            // read 8x32 block
            read(BOTTOM_FIELD(ibuf), offset + j * step_w, iy * step_h + i, in);

            // mask out invalid pixels
            in.merge(in, 0, mask);

            acc(j) += cm_sum<ushort>(in);
        }
        for (; i < step_h; i++) {

            // read 1x32 block
            read(BOTTOM_FIELD(ibuf), offset + j * step_w, iy * step_h + i, in.row(0));

            // mask out invalid pixels
            in.row(0).merge(in.row(0), 0, mask.row(0));

            acc(j) += cm_sum<ushort>(in.row(0));
        }
    }

    vector<uchar, OUT_BLOCK> out = acc / (step_w * step_h);
#ifdef CM_BUFFER
    write(obuf,  iy * out_pitch + ix * OUT_BLOCK, out);     // OWORD write to CmBuffer
#else
    write(obuf, ix * OUT_BLOCK, iy, out);                   // media write to CmSurface2D
#endif
}
