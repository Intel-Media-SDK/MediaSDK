// Copyright (c) 1985-2018 Intel Corporation
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


/*----------------------------------------------------------------------*/
#define ROUND_UP(offset, round_to)   ( ( (offset) + (round_to) - 1) &~ ((round_to) - 1 ))
#define ROUND_DOWN(offset, round_to)   ( (offset) &~ ( ( round_to) - 1 ) )

#define BLOCK_PIXEL_WIDTH    (32)
#define BLOCK_HEIGHT        (8)
#define BLOCK_HEIGHT_NV12   (4)

#define SUB_BLOCK_PIXEL_WIDTH (8)
#define SUB_BLOCK_HEIGHT      (8)
#define SUB_BLOCK_HEIGHT_NV12 (4)

#define BLOCK_WIDTH                        (64)
#define PADDED_BLOCK_WIDTH                (128)
#define PADDED_BLOCK_WIDTH_CPU_TO_GPU    (80)

#define MIN(x, y)    (x < y ? x:y)

/*32 index*/
const ushort indexTable[32] = {0x1f,0x1e,0x1d,0x1c,0x1b,0x1a,0x19,0x18,0x17,0x16,0x15,0x14,0x13,0x12,0x11,0x10,
                               0x0f,0x0e,0x0d,0x0c,0x0b,0x0a,0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01,0x00};

extern "C" _GENX_MAIN_  void
surfaceCopy_write_NV12(SurfaceIndex INBUF_IDX, SurfaceIndex OUTBUF_IDX, int width_dword, int height,int ShiftLeftOffsetInBytes, int width_stride, int height_stride)
{
    //write Y plane
    matrix<uint, BLOCK_HEIGHT, BLOCK_PIXEL_WIDTH> inData_m;
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData0(inData_m.row(0));
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData1(inData_m.row(1));
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData2(inData_m.row(2));
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData3(inData_m.row(3));
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData4(inData_m.row(4));
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData5(inData_m.row(5));
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData6(inData_m.row(6));
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData7(inData_m.row(7));

    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> outData0;
    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> outData1;
    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> outData2;
    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> outData3;

    int horizOffset = get_thread_origin_x() * BLOCK_PIXEL_WIDTH;
    int vertOffset = get_thread_origin_y() * BLOCK_HEIGHT;

    int horizOffset_byte = horizOffset << 2;
    int sub_block_width_byte = SUB_BLOCK_PIXEL_WIDTH << 2;

    uint linear_offset_byte = (( vertOffset * (width_stride>>2) + horizOffset ) << 2) + ShiftLeftOffsetInBytes;

    read(DWALIGNED(INBUF_IDX), linear_offset_byte,                  inData0);
    read(DWALIGNED(INBUF_IDX), linear_offset_byte + width_stride,     inData1);
    read(DWALIGNED(INBUF_IDX), linear_offset_byte + width_stride * 2, inData2);
    read(DWALIGNED(INBUF_IDX), linear_offset_byte + width_stride * 3, inData3);
    read(DWALIGNED(INBUF_IDX), linear_offset_byte + width_stride * 4, inData4);
    read(DWALIGNED(INBUF_IDX), linear_offset_byte + width_stride * 5, inData5);
    read(DWALIGNED(INBUF_IDX), linear_offset_byte + width_stride * 6, inData6);
    read(DWALIGNED(INBUF_IDX), linear_offset_byte + width_stride * 7, inData7);

    outData0 = inData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 0);
    outData1 = inData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 8);
    outData2 = inData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 16);
    outData3 = inData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 24);

    write_plane(OUTBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset_byte,                          vertOffset, outData0);
    write_plane(OUTBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset_byte + sub_block_width_byte,   vertOffset, outData1);
    write_plane(OUTBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset_byte + sub_block_width_byte*2, vertOffset, outData2);
    write_plane(OUTBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset_byte + sub_block_width_byte*3, vertOffset, outData3);

    //write UV plane
    matrix<uint, BLOCK_HEIGHT_NV12, BLOCK_PIXEL_WIDTH> inData_NV12_m;
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData_NV12_0 = inData_NV12_m.row(0);
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData_NV12_1 = inData_NV12_m.row(1);
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData_NV12_2 = inData_NV12_m.row(2);
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData_NV12_3 = inData_NV12_m.row(3);

    matrix<uint, SUB_BLOCK_HEIGHT_NV12, SUB_BLOCK_PIXEL_WIDTH> outData_NV12_0;
    matrix<uint, SUB_BLOCK_HEIGHT_NV12, SUB_BLOCK_PIXEL_WIDTH> outData_NV12_1;
    matrix<uint, SUB_BLOCK_HEIGHT_NV12, SUB_BLOCK_PIXEL_WIDTH> outData_NV12_2;
    matrix<uint, SUB_BLOCK_HEIGHT_NV12, SUB_BLOCK_PIXEL_WIDTH> outData_NV12_3;

    int horizOffset_NV12 = get_thread_origin_x() * BLOCK_PIXEL_WIDTH;
    int vertOffset_NV12 = get_thread_origin_y() * BLOCK_HEIGHT_NV12;

    uint linear_offset_NV12_byte = (( (width_stride>>2) * ( height_stride + vertOffset_NV12 ) + horizOffset_NV12 ) << 2) + ShiftLeftOffsetInBytes;

    read(DWALIGNED(INBUF_IDX), linear_offset_NV12_byte,                  inData_NV12_0);
    read(DWALIGNED(INBUF_IDX), linear_offset_NV12_byte + width_stride,     inData_NV12_1);
    read(DWALIGNED(INBUF_IDX), linear_offset_NV12_byte + width_stride * 2, inData_NV12_2);
    read(DWALIGNED(INBUF_IDX), linear_offset_NV12_byte + width_stride * 3, inData_NV12_3);

    outData_NV12_0 = inData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 0);
    outData_NV12_1 = inData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 8);
    outData_NV12_2 = inData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 16);
    outData_NV12_3 = inData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 24);

    write_plane(OUTBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte,                          vertOffset >> 1, outData_NV12_0);
    write_plane(OUTBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte + sub_block_width_byte,   vertOffset >> 1, outData_NV12_1);
    write_plane(OUTBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte + sub_block_width_byte*2, vertOffset >> 1, outData_NV12_2);
    write_plane(OUTBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte + sub_block_width_byte*3, vertOffset >> 1, outData_NV12_3);

}


extern "C" _GENX_MAIN_  void
surfaceCopy_read_NV12(SurfaceIndex INBUF_IDX, SurfaceIndex OUTBUF_IDX, int width_dword, int height, int ShiftLeftOffsetInBytes, int height_stride, int width_stride)
{
    //Y plane
    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> inData0;
    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> inData1;
    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> inData2;
    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> inData3;

    matrix<uint, BLOCK_HEIGHT, BLOCK_PIXEL_WIDTH> outData_m;
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData0(outData_m.row(0));
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData1(outData_m.row(1));
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData2(outData_m.row(2));
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData3(outData_m.row(3));
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData4(outData_m.row(4));
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData5(outData_m.row(5));
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData6(outData_m.row(6));
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData7(outData_m.row(7));

    //UV plane
    matrix<uint, SUB_BLOCK_HEIGHT_NV12, SUB_BLOCK_PIXEL_WIDTH> inData_NV12_0;
    matrix<uint, SUB_BLOCK_HEIGHT_NV12, SUB_BLOCK_PIXEL_WIDTH> inData_NV12_1;
    matrix<uint, SUB_BLOCK_HEIGHT_NV12, SUB_BLOCK_PIXEL_WIDTH> inData_NV12_2;
    matrix<uint, SUB_BLOCK_HEIGHT_NV12, SUB_BLOCK_PIXEL_WIDTH> inData_NV12_3;

    matrix<uint, BLOCK_HEIGHT_NV12, BLOCK_PIXEL_WIDTH> outData_NV12_m;
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData_NV12_0 = outData_NV12_m.row(0);
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData_NV12_1 = outData_NV12_m.row(1);
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData_NV12_2 = outData_NV12_m.row(2);
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData_NV12_3 = outData_NV12_m.row(3);

    int horizOffset = get_thread_origin_x() * BLOCK_PIXEL_WIDTH;
    int vertOffset = get_thread_origin_y() * BLOCK_HEIGHT;

    int horizOffset_NV12 = get_thread_origin_x() * BLOCK_PIXEL_WIDTH;
    int vertOffset_NV12 = get_thread_origin_y() * BLOCK_HEIGHT_NV12;

    int horizOffset_byte = horizOffset << 2;
    int sub_block_width_byte = SUB_BLOCK_PIXEL_WIDTH << 2;

    uint linear_offset_dword = vertOffset * (width_stride>>2) + horizOffset + (ShiftLeftOffsetInBytes/4);
    uint linear_offset_byte = (linear_offset_dword << 2) ;

    uint linear_offset_NV12_dword = (width_stride>>2) * ( height_stride + vertOffset_NV12 ) + horizOffset_NV12 + (ShiftLeftOffsetInBytes/4);
    uint linear_offset_NV12_byte = (linear_offset_NV12_dword << 2) ;

    uint last_block_height = 8;
    if(height - vertOffset < BLOCK_HEIGHT)
    {
        last_block_height = height - vertOffset;
    }

    if (width_dword - horizOffset >= BLOCK_PIXEL_WIDTH)    // for aligned block
    {
        read_plane(INBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset_byte,                             vertOffset, inData0);
        read_plane(INBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset_byte + SUB_BLOCK_PIXEL_WIDTH*4,   vertOffset, inData1);
        read_plane(INBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset_byte + SUB_BLOCK_PIXEL_WIDTH*4*2, vertOffset, inData2);
        read_plane(INBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset_byte + SUB_BLOCK_PIXEL_WIDTH*4*3, vertOffset, inData3);

        read_plane(INBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte,                          vertOffset >> 1, inData_NV12_0);
        read_plane(INBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte + sub_block_width_byte,   vertOffset >> 1, inData_NV12_1);
        read_plane(INBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte + sub_block_width_byte*2, vertOffset >> 1, inData_NV12_2);
        read_plane(INBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte + sub_block_width_byte*3, vertOffset >> 1, inData_NV12_3);

        outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 0)  = inData0;
        outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 8)  = inData1;
        outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 16) = inData2;
        outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 24) = inData3;

        outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 0)  = inData_NV12_0;
        outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 8)  = inData_NV12_1;
        outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 16) = inData_NV12_2;
        outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 24) = inData_NV12_3;

        switch(last_block_height)
        {
        case 8:
            write(OUTBUF_IDX, linear_offset_byte,                  outData0);
            write(OUTBUF_IDX, linear_offset_byte + width_stride,     outData1);
            write(OUTBUF_IDX, linear_offset_byte + width_stride * 2, outData2);
            write(OUTBUF_IDX, linear_offset_byte + width_stride * 3, outData3);
            write(OUTBUF_IDX, linear_offset_byte + width_stride * 4, outData4);
            write(OUTBUF_IDX, linear_offset_byte + width_stride * 5, outData5);
            write(OUTBUF_IDX, linear_offset_byte + width_stride * 6, outData6);
            write(OUTBUF_IDX, linear_offset_byte + width_stride * 7, outData7);

            write(OUTBUF_IDX, linear_offset_NV12_byte,                  outData_NV12_0);
            write(OUTBUF_IDX, linear_offset_NV12_byte + width_stride,     outData_NV12_1);
            write(OUTBUF_IDX, linear_offset_NV12_byte + width_stride * 2, outData_NV12_2);
            write(OUTBUF_IDX, linear_offset_NV12_byte + width_stride * 3, outData_NV12_3);
            break;
        case 6:
            write(OUTBUF_IDX, linear_offset_byte,                  outData0);
            write(OUTBUF_IDX, linear_offset_byte + width_stride,     outData1);
            write(OUTBUF_IDX, linear_offset_byte + width_stride * 2, outData2);
            write(OUTBUF_IDX, linear_offset_byte + width_stride * 3, outData3);
            write(OUTBUF_IDX, linear_offset_byte + width_stride * 4, outData4);
            write(OUTBUF_IDX, linear_offset_byte + width_stride * 5, outData5);

            write(OUTBUF_IDX, linear_offset_NV12_byte,                  outData_NV12_0);
            write(OUTBUF_IDX, linear_offset_NV12_byte + width_stride,     outData_NV12_1);
            write(OUTBUF_IDX, linear_offset_NV12_byte + width_stride * 2, outData_NV12_2);
            break;
        case 4:
            write(OUTBUF_IDX, linear_offset_byte,                  outData0);
            write(OUTBUF_IDX, linear_offset_byte + width_stride,     outData1);
            write(OUTBUF_IDX, linear_offset_byte + width_stride * 2, outData2);
            write(OUTBUF_IDX, linear_offset_byte + width_stride * 3, outData3);

            write(OUTBUF_IDX, linear_offset_NV12_byte,              outData_NV12_0);
            write(OUTBUF_IDX, linear_offset_NV12_byte + width_stride, outData_NV12_1);
            break;
        case 2:
            write(OUTBUF_IDX, linear_offset_byte,                  outData0);
            write(OUTBUF_IDX, linear_offset_byte + width_stride,     outData1);

            write(OUTBUF_IDX, linear_offset_NV12_byte,                  outData_NV12_0);
            break;
        default:
            break;
        }
    }
    else    // for the unaligned most right blocks
    {
        uint block_width = width_dword - horizOffset;
        uint last_block_size = block_width;
        uint read_times = 1;
        read(INBUF_IDX, horizOffset_byte, vertOffset, inData0);
        read_plane(INBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte, vertOffset >> 1, inData_NV12_0);
        if (block_width > 8)
        {
            read_plane(INBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset_byte + sub_block_width_byte, vertOffset, inData1);
            read_plane(INBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte + sub_block_width_byte, vertOffset >> 1, inData_NV12_1);
            read_times = 2;
            last_block_size = last_block_size - 8;
            if (block_width > 16)
            {
                read_plane(INBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset_byte + sub_block_width_byte*2, vertOffset, inData2);
                read_plane(INBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte + sub_block_width_byte*2, vertOffset >> 1, inData_NV12_2);
                read_times = 3;
                last_block_size = last_block_size - 8;
                if (block_width > 24)
                {
                    read_plane(INBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset_byte + sub_block_width_byte*3, vertOffset, inData3);
                    read_plane(INBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte + sub_block_width_byte*3, vertOffset >> 1, inData_NV12_3);
                    read_times = 4;
                    last_block_size = last_block_size - 8;
                }
            }
        }

        vector<uint, SUB_BLOCK_PIXEL_WIDTH> elment_offset(0);

        for (uint i=0; i < last_block_size; i++)
        {
            elment_offset(i) = i;
        }

        switch (read_times)
        {
        case 4:
            outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 0)  = inData0;
            outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 8)  = inData1;
            outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 16) = inData2;
            outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 24) = inData3;

            outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 0)  = inData_NV12_0;
            outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 8)  = inData_NV12_1;
            outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 16) = inData_NV12_2;
            outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 24) = inData_NV12_3;

            //Padding unused by the first one pixel in the last sub block
            for (int i = last_block_size; i < SUB_BLOCK_PIXEL_WIDTH; i ++)
            {
                outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 24 + i) = outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 24);
                outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, 1, 1>(0, 24 + i) = outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, 1, 1>(0, 24);
            }

            switch(last_block_height)
            {
            case 8:
                //write Y plane to buffer
                write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride,     outData1.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 2, outData2.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 3, outData3.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 4, outData4.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 5, outData5.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 6, outData6.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 7, outData7.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_byte + 64,                  outData0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + width_stride,     outData1.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + width_stride * 2, outData2.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + width_stride * 3, outData3.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + width_stride * 4, outData4.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + width_stride * 5, outData5.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + width_stride * 6, outData6.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + width_stride * 7, outData7.select<8, 1>(16));

                write(OUTBUF_IDX, linear_offset_dword + 24,                   elment_offset, outData0.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + (width_stride>>2),     elment_offset, outData1.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + (width_stride>>2) * 2, elment_offset, outData2.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + (width_stride>>2) * 3, elment_offset, outData3.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + (width_stride>>2) * 4, elment_offset, outData4.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + (width_stride>>2) * 5, elment_offset, outData5.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + (width_stride>>2) * 6, elment_offset, outData6.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + (width_stride>>2) * 7, elment_offset, outData7.select<8, 1>(24));

                //write UV plane to buffer
                write(OUTBUF_IDX, linear_offset_NV12_byte,                       outData_NV12_0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + (width_stride>>2) * 4,     outData_NV12_1.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + (width_stride>>2) * 4 * 2, outData_NV12_2.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + (width_stride>>2) * 4 * 3, outData_NV12_3.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_byte + 64,                  outData_NV12_0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_byte + 64 + width_stride,     outData_NV12_1.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_byte + 64 + width_stride * 2, outData_NV12_2.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_byte + 64 + width_stride * 3, outData_NV12_3.select<8, 1>(16));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 24,                   elment_offset, outData_NV12_0.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 24 + (width_stride>>2),     elment_offset, outData_NV12_1.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 24 + (width_stride>>2) * 2, elment_offset, outData_NV12_2.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 24 + (width_stride>>2) * 3, elment_offset, outData_NV12_3.select<8, 1>(24));
                break;
            case 6:
                //write Y plane to buffer
                write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride,     outData1.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 2, outData2.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 3, outData3.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 4, outData4.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 5, outData5.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_byte + 64,                  outData0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + width_stride,     outData1.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + width_stride * 2, outData2.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + width_stride * 3, outData3.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + width_stride * 4, outData4.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + width_stride * 5, outData5.select<8, 1>(16));

                write(OUTBUF_IDX, linear_offset_dword + 24,                   elment_offset, outData0.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + (width_stride>>2),     elment_offset, outData1.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + (width_stride>>2) * 2, elment_offset, outData2.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + (width_stride>>2) * 3, elment_offset, outData3.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + (width_stride>>2) * 4, elment_offset, outData4.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + (width_stride>>2) * 5, elment_offset, outData5.select<8, 1>(24));

                //write UV plane to buffer
                write(OUTBUF_IDX, linear_offset_NV12_byte,                       outData_NV12_0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + (width_stride>>2) * 4,     outData_NV12_1.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + (width_stride>>2) * 4 * 2, outData_NV12_2.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_byte + 64,                  outData_NV12_0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_byte + 64 + width_stride,     outData_NV12_1.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_byte + 64 + width_stride * 2, outData_NV12_2.select<8, 1>(16));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 24,                   elment_offset, outData_NV12_0.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 24 + (width_stride>>2),     elment_offset, outData_NV12_1.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 24 + (width_stride>>2) * 2, elment_offset, outData_NV12_2.select<8, 1>(24));
                break;
            case 4:
                //write Y plane to buffer
                write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride,     outData1.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 2, outData2.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 3, outData3.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_byte + 64,                  outData0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + width_stride,     outData1.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + width_stride * 2, outData2.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + width_stride * 3, outData3.select<8, 1>(16));

                write(OUTBUF_IDX, linear_offset_dword + 24,                   elment_offset, outData0.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + (width_stride>>2),     elment_offset, outData1.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + (width_stride>>2) * 2, elment_offset, outData2.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + (width_stride>>2) * 3, elment_offset, outData3.select<8, 1>(24));

                //write UV plane to buffer
                write(OUTBUF_IDX, linear_offset_NV12_byte,                       outData_NV12_0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + (width_stride>>2) * 4,     outData_NV12_1.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_byte + 64,                  outData_NV12_0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_byte + 64 + width_stride,     outData_NV12_1.select<8, 1>(16));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 24,                   elment_offset, outData_NV12_0.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 24 + (width_stride>>2),     elment_offset, outData_NV12_1.select<8, 1>(24));
                break;
            case 2:
                //write Y plane to buffer
                write(OUTBUF_IDX, linear_offset_byte,              outData0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride, outData1.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_byte + 64,              outData0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + width_stride, outData1.select<8, 1>(16));

                write(OUTBUF_IDX, linear_offset_dword + 24,               elment_offset, outData0.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + (width_stride>>2), elment_offset, outData1.select<8, 1>(24));

                //write UV plane to buffer
                write(OUTBUF_IDX, linear_offset_NV12_byte, outData_NV12_0.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_byte + 64, outData_NV12_0.select<8, 1>(16));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 24, elment_offset, outData_NV12_0.select<8, 1>(24));
                break;
            default:
                break;
            }
            break;
        case 3:
            outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 0)  = inData0;
            outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 8)  = inData1;
            outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 16) = inData2;

            outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 0)  = inData_NV12_0;
            outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 8)  = inData_NV12_1;
            outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 16) = inData_NV12_2;

            //Padding unused by the first one pixel in the last sub block
            for (int i = last_block_size; i < SUB_BLOCK_PIXEL_WIDTH; i ++)
            {
                outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 16 + i) = outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 16);
                outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, 1, 1>(0, 16 + i) = outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, 1, 1>(0, 16);
            }

            switch(last_block_height)
            {
            case 8:
                //write Y plane to buffer
                write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride,     outData1.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 2, outData2.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 3, outData3.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 4, outData4.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 5, outData5.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 6, outData6.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 7, outData7.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_dword + 16,                   elment_offset, outData0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + (width_stride>>2),     elment_offset, outData1.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + (width_stride>>2) * 2, elment_offset, outData2.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + (width_stride>>2) * 3, elment_offset, outData3.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + (width_stride>>2) * 4, elment_offset, outData4.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + (width_stride>>2) * 5, elment_offset, outData5.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + (width_stride>>2) * 6, elment_offset, outData6.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + (width_stride>>2) * 7, elment_offset, outData7.select<8, 1>(16));

                //write UV plane to buffer
                write(OUTBUF_IDX, linear_offset_NV12_byte,                  outData_NV12_0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + width_stride,     outData_NV12_1.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + width_stride * 2, outData_NV12_2.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + width_stride * 3, outData_NV12_3.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 16,                   elment_offset, outData_NV12_0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 16 + (width_stride>>2),     elment_offset, outData_NV12_1.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 16 + (width_stride>>2) * 2, elment_offset, outData_NV12_2.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 16 + (width_stride>>2) * 3, elment_offset, outData_NV12_3.select<8, 1>(16));
                break;
            case 6:
                //write Y plane to buffer
                write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride,     outData1.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 2, outData2.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 3, outData3.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 4, outData4.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 5, outData5.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_dword + 16,                   elment_offset, outData0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + (width_stride>>2),     elment_offset, outData1.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + (width_stride>>2) * 2, elment_offset, outData2.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + (width_stride>>2) * 3, elment_offset, outData3.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + (width_stride>>2) * 4, elment_offset, outData4.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + (width_stride>>2) * 5, elment_offset, outData5.select<8, 1>(16));

                //write UV plane to buffer
                write(OUTBUF_IDX, linear_offset_NV12_byte,                  outData_NV12_0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + width_stride,     outData_NV12_1.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + width_stride * 2, outData_NV12_2.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 16,                   elment_offset, outData_NV12_0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 16 + (width_stride>>2),     elment_offset, outData_NV12_1.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 16 + (width_stride>>2) * 2, elment_offset, outData_NV12_2.select<8, 1>(16));
                break;
            case 4:
                //write Y plane to buffer
                write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride,     outData1.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 2, outData2.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 3, outData3.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_dword + 16,                   elment_offset, outData0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + (width_stride>>2),     elment_offset, outData1.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + (width_stride>>2) * 2, elment_offset, outData2.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + (width_stride>>2) * 3, elment_offset, outData3.select<8, 1>(16));

                //write UV plane to buffer
                write(OUTBUF_IDX, linear_offset_NV12_byte,              outData_NV12_0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + width_stride, outData_NV12_1.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 16,               elment_offset, outData_NV12_0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 16 + (width_stride>>2), elment_offset, outData_NV12_1.select<8, 1>(16));
                break;
            case 2:
                //write Y plane to buffer
                write(OUTBUF_IDX, linear_offset_byte,              outData0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride, outData1.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_dword + 16,               elment_offset, outData0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + (width_stride>>2), elment_offset, outData1.select<8, 1>(16));

                //write UV plane to buffer
                write(OUTBUF_IDX, linear_offset_NV12_byte, outData_NV12_0.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 16, elment_offset, outData_NV12_0.select<8, 1>(16));
                break;
            default:
                break;
            }
            break;
        case 2:
            outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 0) = inData0;
            outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 8) = inData1;

            outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 0) = inData_NV12_0;
            outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 8) = inData_NV12_1;

            //Padding unused by the first one pixel in the last sub block
            for (int i = last_block_size; i < SUB_BLOCK_PIXEL_WIDTH; i ++)
            {
                outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 8 + i) = outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 8);
                outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, 1, 1>(0, 8 + i) = outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, 1, 1>(0, 8);
            }
            switch(last_block_height)
            {
            case 8:
                write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride,     outData1.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 2, outData2.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 3, outData3.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 4, outData4.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 5, outData5.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 6, outData6.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 7, outData7.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_dword + 8,                   elment_offset, outData0.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + (width_stride>>2),     elment_offset, outData1.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + (width_stride>>2) * 2, elment_offset, outData2.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + (width_stride>>2) * 3, elment_offset, outData3.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + (width_stride>>2) * 4, elment_offset, outData4.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + (width_stride>>2) * 5, elment_offset, outData5.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + (width_stride>>2) * 6, elment_offset, outData6.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + (width_stride>>2) * 7, elment_offset, outData7.select<8, 1>(8));

                write(OUTBUF_IDX, linear_offset_NV12_byte,                  outData_NV12_0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + width_stride,     outData_NV12_1.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + width_stride * 2, outData_NV12_2.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + width_stride * 3, outData_NV12_3.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 8,                   elment_offset, outData_NV12_0.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 8 + (width_stride>>2),     elment_offset, outData_NV12_1.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 8 + (width_stride>>2) * 2, elment_offset, outData_NV12_2.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 8 + (width_stride>>2) * 3, elment_offset, outData_NV12_3.select<8, 1>(8));
                break;
            case 6:
                write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride,     outData1.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 2, outData2.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 3, outData3.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 4, outData4.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 5, outData5.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_dword + 8,                   elment_offset, outData0.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + (width_stride>>2),     elment_offset, outData1.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + (width_stride>>2) * 2, elment_offset, outData2.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + (width_stride>>2) * 3, elment_offset, outData3.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + (width_stride>>2) * 4, elment_offset, outData4.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + (width_stride>>2) * 5, elment_offset, outData5.select<8, 1>(8));

                write(OUTBUF_IDX, linear_offset_NV12_byte,                  outData_NV12_0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + width_stride,     outData_NV12_1.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + width_stride * 2, outData_NV12_2.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 8,                   elment_offset, outData_NV12_0.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 8 + (width_stride>>2),     elment_offset, outData_NV12_1.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 8 + (width_stride>>2) * 2, elment_offset, outData_NV12_2.select<8, 1>(8));
                break;
            case 4:
                write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride,     outData1.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 2, outData2.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride * 3, outData3.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_dword + 8,                   elment_offset, outData0.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + (width_stride>>2),     elment_offset, outData1.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + (width_stride>>2) * 2, elment_offset, outData2.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + (width_stride>>2) * 3, elment_offset, outData3.select<8, 1>(8));

                write(OUTBUF_IDX, linear_offset_NV12_byte,              outData_NV12_0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + width_stride, outData_NV12_1.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 8,               elment_offset, outData_NV12_0.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 8 + (width_stride>>2), elment_offset, outData_NV12_1.select<8, 1>(8));
                break;
            case 2:
                write(OUTBUF_IDX, linear_offset_byte,              outData0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_stride, outData1.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_dword + 8,               elment_offset, outData0.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + (width_stride>>2), elment_offset, outData1.select<8, 1>(8));

                write(OUTBUF_IDX, linear_offset_NV12_byte, outData_NV12_0.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 8, elment_offset, outData_NV12_0.select<8, 1>(8));
                break;
            default:
                break;
            }
            break;
        case 1:
            outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 0) = inData0;
            outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 0) = inData_NV12_0;

            //Padding unused by the first one pixel in the last sub block
            for (int i = last_block_size; i < SUB_BLOCK_PIXEL_WIDTH; i ++)
            {
                outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, i) = outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 0);
                outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, 1, 1>(0, i) = outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, 1, 1>(0, 0);
            }

            switch(last_block_height)
            {
            case 8:
                write(OUTBUF_IDX, linear_offset_dword,                   elment_offset, outData0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + (width_stride>>2),     elment_offset, outData1.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + (width_stride>>2) * 2, elment_offset, outData2.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + (width_stride>>2) * 3, elment_offset, outData3.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + (width_stride>>2) * 4, elment_offset, outData4.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + (width_stride>>2) * 5, elment_offset, outData5.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + (width_stride>>2) * 6, elment_offset, outData6.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + (width_stride>>2) * 7, elment_offset, outData7.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword,                   elment_offset, outData_NV12_0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_dword + (width_stride>>2),     elment_offset, outData_NV12_1.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_dword + (width_stride>>2) * 2, elment_offset, outData_NV12_2.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_dword + (width_stride>>2) * 3, elment_offset, outData_NV12_3.select<8, 1>(0));
                break;
            case 6:
                write(OUTBUF_IDX, linear_offset_dword,                   elment_offset, outData0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + (width_stride>>2),     elment_offset, outData1.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + (width_stride>>2) * 2, elment_offset, outData2.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + (width_stride>>2) * 3, elment_offset, outData3.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + (width_stride>>2) * 4, elment_offset, outData4.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + (width_stride>>2) * 5, elment_offset, outData5.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword,                   elment_offset, outData_NV12_0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_dword + (width_stride>>2),     elment_offset, outData_NV12_1.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_dword + (width_stride>>2) * 2, elment_offset, outData_NV12_2.select<8, 1>(0));
                break;
            case 4:
                write(OUTBUF_IDX, linear_offset_dword,                   elment_offset, outData0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + (width_stride>>2),     elment_offset, outData1.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + (width_stride>>2) * 2, elment_offset, outData2.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + (width_stride>>2) * 3, elment_offset, outData3.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword,               elment_offset, outData_NV12_0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_dword + (width_stride>>2), elment_offset, outData_NV12_1.select<8, 1>(0));
                break;
            case 2:
                write(OUTBUF_IDX, linear_offset_dword,               elment_offset, outData0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + (width_stride>>2), elment_offset, outData1.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword, elment_offset, outData_NV12_0.select<8, 1>(0));
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
    }
}


extern "C" _GENX_MAIN_  void
surfaceCopy_write_P010_shift(SurfaceIndex INBUF_IDX, SurfaceIndex OUTBUF_IDX, int stride_dword, int stride_height,int ShiftLeftOffsetInBytes, int bitshift)
{
    //write Y plane
    matrix<ushort, BLOCK_HEIGHT, BLOCK_PIXEL_WIDTH*2> inData_m;
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData0(inData_m.row(0).format<uint>());
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData1(inData_m.row(1).format<uint>());
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData2(inData_m.row(2).format<uint>());
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData3(inData_m.row(3).format<uint>());
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData4(inData_m.row(4).format<uint>());
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData5(inData_m.row(5).format<uint>());
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData6(inData_m.row(6).format<uint>());
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData7(inData_m.row(7).format<uint>());

    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> outData0;
    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> outData1;
    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> outData2;
    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> outData3;

    int horizOffset = get_thread_origin_x() * BLOCK_PIXEL_WIDTH;
    int vertOffset = get_thread_origin_y() * BLOCK_HEIGHT;

    int width_byte = stride_dword << 2;
    int horizOffset_byte = horizOffset << 2;
    int sub_block_width_byte = SUB_BLOCK_PIXEL_WIDTH << 2;

    uint linear_offset_byte = (( vertOffset * stride_dword + horizOffset ) << 2) + ShiftLeftOffsetInBytes;

    read(DWALIGNED(INBUF_IDX), linear_offset_byte,                  inData0);
    read(DWALIGNED(INBUF_IDX), linear_offset_byte + width_byte,     inData1);
    read(DWALIGNED(INBUF_IDX), linear_offset_byte + width_byte * 2, inData2);
    read(DWALIGNED(INBUF_IDX), linear_offset_byte + width_byte * 3, inData3);
    read(DWALIGNED(INBUF_IDX), linear_offset_byte + width_byte * 4, inData4);
    read(DWALIGNED(INBUF_IDX), linear_offset_byte + width_byte * 5, inData5);
    read(DWALIGNED(INBUF_IDX), linear_offset_byte + width_byte * 6, inData6);
    read(DWALIGNED(INBUF_IDX), linear_offset_byte + width_byte * 7, inData7);

    outData0.format<ushort>() = inData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 0)<<bitshift;
    outData1.format<ushort>() = inData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 16)<<bitshift;
    outData2.format<ushort>() = inData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 32)<<bitshift;
    outData3.format<ushort>() = inData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 48)<<bitshift;

    write_plane(OUTBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset_byte,                          vertOffset, outData0);
    write_plane(OUTBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset_byte + sub_block_width_byte,   vertOffset, outData1);
    write_plane(OUTBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset_byte + sub_block_width_byte*2, vertOffset, outData2);
    write_plane(OUTBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset_byte + sub_block_width_byte*3, vertOffset, outData3);

    //write UV plane
    matrix<ushort, BLOCK_HEIGHT_NV12, BLOCK_PIXEL_WIDTH*2> inData_NV12_m;
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData_NV12_0 = inData_NV12_m.row(0).format<uint>();
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData_NV12_1 = inData_NV12_m.row(1).format<uint>();
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData_NV12_2 = inData_NV12_m.row(2).format<uint>();
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData_NV12_3 = inData_NV12_m.row(3).format<uint>();

    matrix<uint, SUB_BLOCK_HEIGHT_NV12, SUB_BLOCK_PIXEL_WIDTH> outData_NV12_0;
    matrix<uint, SUB_BLOCK_HEIGHT_NV12, SUB_BLOCK_PIXEL_WIDTH> outData_NV12_1;
    matrix<uint, SUB_BLOCK_HEIGHT_NV12, SUB_BLOCK_PIXEL_WIDTH> outData_NV12_2;
    matrix<uint, SUB_BLOCK_HEIGHT_NV12, SUB_BLOCK_PIXEL_WIDTH> outData_NV12_3;

    int horizOffset_NV12 = get_thread_origin_x() * BLOCK_PIXEL_WIDTH;
    int vertOffset_NV12 = get_thread_origin_y() * BLOCK_HEIGHT_NV12;

    uint linear_offset_NV12_byte = (( stride_dword * ( stride_height + vertOffset_NV12 ) + horizOffset_NV12 ) << 2) + ShiftLeftOffsetInBytes;

    read(DWALIGNED(INBUF_IDX), linear_offset_NV12_byte,                  inData_NV12_0);
    read(DWALIGNED(INBUF_IDX), linear_offset_NV12_byte + width_byte,     inData_NV12_1);
    read(DWALIGNED(INBUF_IDX), linear_offset_NV12_byte + width_byte * 2, inData_NV12_2);
    read(DWALIGNED(INBUF_IDX), linear_offset_NV12_byte + width_byte * 3, inData_NV12_3);

    outData_NV12_0.format<ushort>() = inData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 0)<<bitshift;
    outData_NV12_1.format<ushort>() = inData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 16)<<bitshift;
    outData_NV12_2.format<ushort>() = inData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 32)<<bitshift;
    outData_NV12_3.format<ushort>() = inData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 48)<<bitshift;

    write_plane(OUTBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte,                          vertOffset >> 1, outData_NV12_0);
    write_plane(OUTBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte + sub_block_width_byte,   vertOffset >> 1, outData_NV12_1);
    write_plane(OUTBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte + sub_block_width_byte*2, vertOffset >> 1, outData_NV12_2);
    write_plane(OUTBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte + sub_block_width_byte*3, vertOffset >> 1, outData_NV12_3);

}

inline _GENX_  void
surfaceCopy_write_shift_32x8(SurfaceIndex INBUF_IDX, SurfaceIndex OUTBUF_IDX, int width_dword, int height, int srcBuffer_ShiftLeftOffsetInBytes, int bitshift, int horizOffset, int vertOffset, int dst2D_start_x, int dst2D_start_y)
{
    int srcBuffer_linear_offset_byte = ((vertOffset * width_dword + horizOffset) << 2) + srcBuffer_ShiftLeftOffsetInBytes;
    int width_byte = width_dword << 2;
    int dst2D_horizOffset_byte = dst2D_start_x + (horizOffset << 2);
    int dst2D_vertOffset_row = dst2D_start_y + vertOffset;
    int sub_block_width_byte = SUB_BLOCK_PIXEL_WIDTH << 2;
    if (srcBuffer_linear_offset_byte < width_byte * height + srcBuffer_ShiftLeftOffsetInBytes)
    {
        matrix<uint, BLOCK_HEIGHT, BLOCK_PIXEL_WIDTH> inData_m;
        vector_ref<uint, BLOCK_PIXEL_WIDTH> inData0(inData_m.row(0));
        vector_ref<uint, BLOCK_PIXEL_WIDTH> inData1(inData_m.row(1));
        vector_ref<uint, BLOCK_PIXEL_WIDTH> inData2(inData_m.row(2));
        vector_ref<uint, BLOCK_PIXEL_WIDTH> inData3(inData_m.row(3));
        vector_ref<uint, BLOCK_PIXEL_WIDTH> inData4(inData_m.row(4));
        vector_ref<uint, BLOCK_PIXEL_WIDTH> inData5(inData_m.row(5));
        vector_ref<uint, BLOCK_PIXEL_WIDTH> inData6(inData_m.row(6));
        vector_ref<uint, BLOCK_PIXEL_WIDTH> inData7(inData_m.row(7));

        matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> outData0;
        matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> outData1;
        matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> outData2;
        matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> outData3;

        read(INBUF_IDX, srcBuffer_linear_offset_byte, inData0);
        read(INBUF_IDX, srcBuffer_linear_offset_byte + width_byte, inData1);
        read(INBUF_IDX, srcBuffer_linear_offset_byte + width_byte * 2, inData2);
        read(INBUF_IDX, srcBuffer_linear_offset_byte + width_byte * 3, inData3);
        read(INBUF_IDX, srcBuffer_linear_offset_byte + width_byte * 4, inData4);
        read(INBUF_IDX, srcBuffer_linear_offset_byte + width_byte * 5, inData5);
        read(INBUF_IDX, srcBuffer_linear_offset_byte + width_byte * 6, inData6);
        read(INBUF_IDX, srcBuffer_linear_offset_byte + width_byte * 7, inData7);

        outData0.format<ushort>() = inData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 0).format<ushort>()<<bitshift;
        outData1.format<ushort>() = inData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 8).format<ushort>()<< bitshift;
        outData2.format<ushort>() = inData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 16).format<ushort>()<< bitshift;
        outData3.format<ushort>() = inData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 24).format<ushort>()<< bitshift;

        write(OUTBUF_IDX, dst2D_horizOffset_byte, dst2D_vertOffset_row, outData0);
        write(OUTBUF_IDX, dst2D_horizOffset_byte + sub_block_width_byte, dst2D_vertOffset_row, outData1);
        write(OUTBUF_IDX, dst2D_horizOffset_byte + sub_block_width_byte * 2, dst2D_vertOffset_row, outData2);
        write(OUTBUF_IDX, dst2D_horizOffset_byte + sub_block_width_byte * 3, dst2D_vertOffset_row, outData3);
    }
}

extern "C" _GENX_MAIN_  void
surfaceCopy_write_shift_32x32(SurfaceIndex INBUF_IDX, SurfaceIndex OUTBUF_IDX, int width_dword, int height, int srcBuffer_ShiftLeftOffsetInBytes, int bitshift, int threadHeight, int dst2D_start_x, int dst2D_start_y)
{
    int horizOffset = get_thread_origin_x() * BLOCK_PIXEL_WIDTH;
    int vertOffset = get_thread_origin_y() * BLOCK_HEIGHT;

    surfaceCopy_write_shift_32x8(INBUF_IDX, OUTBUF_IDX, width_dword, height, srcBuffer_ShiftLeftOffsetInBytes, bitshift, horizOffset, vertOffset, dst2D_start_x, dst2D_start_y);
    surfaceCopy_write_shift_32x8(INBUF_IDX, OUTBUF_IDX, width_dword, height, srcBuffer_ShiftLeftOffsetInBytes, bitshift, horizOffset, vertOffset + SUB_BLOCK_HEIGHT * threadHeight, dst2D_start_x, dst2D_start_y);
    surfaceCopy_write_shift_32x8(INBUF_IDX, OUTBUF_IDX, width_dword, height, srcBuffer_ShiftLeftOffsetInBytes, bitshift, horizOffset, vertOffset + SUB_BLOCK_HEIGHT * threadHeight * 2, dst2D_start_x, dst2D_start_y);
    surfaceCopy_write_shift_32x8(INBUF_IDX, OUTBUF_IDX, width_dword, height, srcBuffer_ShiftLeftOffsetInBytes, bitshift, horizOffset, vertOffset + SUB_BLOCK_HEIGHT * threadHeight * 3, dst2D_start_x, dst2D_start_y);
}


inline _GENX_  void
surfaceCopy_read_shift_32x8(SurfaceIndex INBUF_IDX, SurfaceIndex OUTBUF_IDX, int width_dword, int height_stride_in_rows, int ShiftLeftOffsetInBytes, int bitshift, int horizOffset, int vertOffset, int width_in_dword_no_padding, int height_no_padding, int src2D_start_x, int src2D_start_y)
{
    int copy_width_dword = MIN(width_in_dword_no_padding, width_dword);
    int copy_height = MIN(height_no_padding, height_stride_in_rows);
    int width_byte = width_dword << 2;
    int horizOffset_byte = src2D_start_x + (horizOffset << 2);
    int vertOffset_row = src2D_start_y + vertOffset;
    int sub_block_width_byte = SUB_BLOCK_PIXEL_WIDTH << 2;

    uint linear_offset_dword = vertOffset * width_dword + horizOffset + (ShiftLeftOffsetInBytes / 4);
    uint linear_offset_byte = (linear_offset_dword << 2);
    if (linear_offset_byte < width_byte * copy_height + ShiftLeftOffsetInBytes)
    {

        matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> inData0;
        matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> inData1;
        matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> inData2;
        matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> inData3;

        matrix<uint, BLOCK_HEIGHT, BLOCK_PIXEL_WIDTH> outData_m;
        vector_ref<uint, BLOCK_PIXEL_WIDTH> outData0 = outData_m.row(0);
        vector_ref<uint, BLOCK_PIXEL_WIDTH> outData1 = outData_m.row(1);
        vector_ref<uint, BLOCK_PIXEL_WIDTH> outData2 = outData_m.row(2);
        vector_ref<uint, BLOCK_PIXEL_WIDTH> outData3 = outData_m.row(3);
        vector_ref<uint, BLOCK_PIXEL_WIDTH> outData4 = outData_m.row(4);
        vector_ref<uint, BLOCK_PIXEL_WIDTH> outData5 = outData_m.row(5);
        vector_ref<uint, BLOCK_PIXEL_WIDTH> outData6 = outData_m.row(6);
        vector_ref<uint, BLOCK_PIXEL_WIDTH> outData7 = outData_m.row(7);
        uint last_block_height = 8;
        if (copy_height - vertOffset < BLOCK_HEIGHT)
        {
            last_block_height = copy_height - vertOffset;
        }

        if (copy_width_dword - horizOffset >= BLOCK_PIXEL_WIDTH)    // for aligned block
        {
            read(INBUF_IDX, horizOffset_byte, vertOffset_row, inData0);
            read(INBUF_IDX, horizOffset_byte + sub_block_width_byte, vertOffset_row, inData1);
            read(INBUF_IDX, horizOffset_byte + sub_block_width_byte * 2, vertOffset_row, inData2);
            read(INBUF_IDX, horizOffset_byte + sub_block_width_byte * 3, vertOffset_row, inData3);

            outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 0).format<ushort>() = inData0.format<ushort>() >> bitshift;
            outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 8).format<ushort>() = inData1.format<ushort>() >> bitshift;
            outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 16).format<ushort>() = inData2.format<ushort>() >> bitshift;
            outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 24).format<ushort>() = inData3.format<ushort>() >> bitshift;

            switch (last_block_height)
            {
            case 8:
                write(OUTBUF_IDX, linear_offset_byte, outData0);
                write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 6, outData6);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 7, outData7);
                break;

            case 7:
                write(OUTBUF_IDX, linear_offset_byte, outData0);
                write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 6, outData6);
                break;

            case 6:
                write(OUTBUF_IDX, linear_offset_byte, outData0);
                write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5);
                break;

            case 5:
                write(OUTBUF_IDX, linear_offset_byte, outData0);
                write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4);
                break;

            default:
                break;
            }
            switch (last_block_height)
            {
            case 4:
                write(OUTBUF_IDX, linear_offset_byte, outData0);
                write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3);
                break;

            case 3:
                write(OUTBUF_IDX, linear_offset_byte, outData0);
                write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2);
                break;

            case 2:
                write(OUTBUF_IDX, linear_offset_byte, outData0);
                write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1);
                break;

            case 1:
                write(OUTBUF_IDX, linear_offset_byte, outData0);
                break;

            default:
                break;
            }
        }
        else    // for the unaligned most right blocks
        {
            uint block_width = copy_width_dword - horizOffset;
            uint last_block_size = block_width;
            uint read_times = 1;
            read(INBUF_IDX, horizOffset_byte, vertOffset_row, inData0);
            if (block_width > 8)
            {
                read(INBUF_IDX, horizOffset_byte + sub_block_width_byte, vertOffset_row, inData1);
                read_times = 2;
                last_block_size = last_block_size - 8;
                if (block_width > 16)
                {
                    read(INBUF_IDX, horizOffset_byte + sub_block_width_byte * 2, vertOffset_row, inData2);
                    read_times = 3;
                    last_block_size = last_block_size - 8;
                    if (block_width > 24)
                    {
                        read(INBUF_IDX, horizOffset_byte + sub_block_width_byte * 3, vertOffset_row, inData3);
                        read_times = 4;
                        last_block_size = last_block_size - 8;
                    }
                }
            }

            vector<uint, SUB_BLOCK_PIXEL_WIDTH> elment_offset(0);

            for (uint i = 0; i < last_block_size; i++)
            {
                elment_offset(i) = i;
            }

            switch (read_times)
            {
            case 4:
                outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 0).format<ushort>() = inData0.format<ushort>() >> bitshift;
                outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 8).format<ushort>() = inData1.format<ushort>() >> bitshift;
                outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 16).format<ushort>() = inData2.format<ushort>() >> bitshift;
                outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 24).format<ushort>() = inData3.format<ushort>() >> bitshift;

                //Padding unused by the first one pixel in the last sub block
                for (int i = last_block_size; i < SUB_BLOCK_PIXEL_WIDTH; i++)
                {
                    outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 24 + i) = outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 24);
                }

                switch (last_block_height)
                {
                case 8:
                    write(OUTBUF_IDX, linear_offset_byte, outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 6, outData6.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 7, outData7.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_byte + 64, outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte, outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 2, outData2.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 3, outData3.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 4, outData4.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 5, outData5.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 6, outData6.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 7, outData7.select<8, 1>(16));

                    write(OUTBUF_IDX, linear_offset_dword + 24, elment_offset, outData0.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword, elment_offset, outData1.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 2, elment_offset, outData2.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 3, elment_offset, outData3.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 4, elment_offset, outData4.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 5, elment_offset, outData5.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 6, elment_offset, outData6.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 7, elment_offset, outData7.select<8, 1>(24));
                    break;

                case 7:
                    write(OUTBUF_IDX, linear_offset_byte, outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 6, outData6.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_byte + 64, outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte, outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 2, outData2.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 3, outData3.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 4, outData4.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 5, outData5.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 6, outData6.select<8, 1>(16));

                    write(OUTBUF_IDX, linear_offset_dword + 24, elment_offset, outData0.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword, elment_offset, outData1.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 2, elment_offset, outData2.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 3, elment_offset, outData3.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 4, elment_offset, outData4.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 5, elment_offset, outData5.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 6, elment_offset, outData6.select<8, 1>(24));
                    break;

                case 6:
                    write(OUTBUF_IDX, linear_offset_byte, outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_byte + 64, outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte, outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 2, outData2.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 3, outData3.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 4, outData4.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 5, outData5.select<8, 1>(16));

                    write(OUTBUF_IDX, linear_offset_dword + 24, elment_offset, outData0.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword, elment_offset, outData1.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 2, elment_offset, outData2.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 3, elment_offset, outData3.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 4, elment_offset, outData4.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 5, elment_offset, outData5.select<8, 1>(24));
                    break;

                case 5:
                    write(OUTBUF_IDX, linear_offset_byte, outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_byte + 64, outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte, outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 2, outData2.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 3, outData3.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 4, outData4.select<8, 1>(16));

                    write(OUTBUF_IDX, linear_offset_dword + 24, elment_offset, outData0.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword, elment_offset, outData1.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 2, elment_offset, outData2.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 3, elment_offset, outData3.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 4, elment_offset, outData4.select<8, 1>(24));
                    break;

                default:
                    break;
                }
                switch (last_block_height)
                {
                case 4:
                    write(OUTBUF_IDX, linear_offset_byte, outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_byte + 64, outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte, outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 2, outData2.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 3, outData3.select<8, 1>(16));

                    write(OUTBUF_IDX, linear_offset_dword + 24, elment_offset, outData0.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword, elment_offset, outData1.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 2, elment_offset, outData2.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 3, elment_offset, outData3.select<8, 1>(24));
                    break;

                case 3:
                    write(OUTBUF_IDX, linear_offset_byte, outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_byte + 64, outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte, outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 2, outData2.select<8, 1>(16));

                    write(OUTBUF_IDX, linear_offset_dword + 24, elment_offset, outData0.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword, elment_offset, outData1.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 2, elment_offset, outData2.select<8, 1>(24));
                    break;

                case 2:
                    write(OUTBUF_IDX, linear_offset_byte, outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_byte + 64, outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte, outData1.select<8, 1>(16));

                    write(OUTBUF_IDX, linear_offset_dword + 24, elment_offset, outData0.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword, elment_offset, outData1.select<8, 1>(24));
                    break;

                case 1:
                    write(OUTBUF_IDX, linear_offset_byte, outData0.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_byte + 64, outData0.select<8, 1>(16));

                    write(OUTBUF_IDX, linear_offset_dword + 24, elment_offset, outData0.select<8, 1>(24));
                    break;

                default:
                    break;
                }
                break;
            case 3:
                outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 0).format<ushort>() = inData0.format<ushort>() >> bitshift;
                outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 8).format<ushort>() = inData1.format<ushort>() >> bitshift;
                outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 16).format<ushort>() = inData2.format<ushort>() >> bitshift;

                //Padding unused by the first one pixel in the last sub block
                for (int i = last_block_size; i < SUB_BLOCK_PIXEL_WIDTH; i++)
                {
                    outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 16 + i) = outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 16);
                }

                switch (last_block_height)
                {
                case 8:
                    write(OUTBUF_IDX, linear_offset_byte, outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 6, outData6.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 7, outData7.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 16, elment_offset, outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword, elment_offset, outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 2, elment_offset, outData2.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 3, elment_offset, outData3.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 4, elment_offset, outData4.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 5, elment_offset, outData5.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 6, elment_offset, outData6.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 7, elment_offset, outData7.select<8, 1>(16));
                    break;
                case 7:
                    write(OUTBUF_IDX, linear_offset_byte, outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 6, outData6.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 16, elment_offset, outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword, elment_offset, outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 2, elment_offset, outData2.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 3, elment_offset, outData3.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 4, elment_offset, outData4.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 5, elment_offset, outData5.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 6, elment_offset, outData6.select<8, 1>(16));
                    break;
                case 6:
                    write(OUTBUF_IDX, linear_offset_byte, outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 16, elment_offset, outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword, elment_offset, outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 2, elment_offset, outData2.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 3, elment_offset, outData3.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 4, elment_offset, outData4.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 5, elment_offset, outData5.select<8, 1>(16));
                    break;
                case 5:
                    write(OUTBUF_IDX, linear_offset_byte, outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 16, elment_offset, outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword, elment_offset, outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 2, elment_offset, outData2.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 3, elment_offset, outData3.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 4, elment_offset, outData4.select<8, 1>(16));
                    break;
                default:
                    break;
                }
                switch (last_block_height)
                {
                case 4:
                    write(OUTBUF_IDX, linear_offset_byte, outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 16, elment_offset, outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword, elment_offset, outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 2, elment_offset, outData2.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 3, elment_offset, outData3.select<8, 1>(16));
                    break;
                case 3:
                    write(OUTBUF_IDX, linear_offset_byte, outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 16, elment_offset, outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword, elment_offset, outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 2, elment_offset, outData2.select<8, 1>(16));
                    break;
                case 2:
                    write(OUTBUF_IDX, linear_offset_byte, outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 16, elment_offset, outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword, elment_offset, outData1.select<8, 1>(16));
                    break;
                case 1:
                    write(OUTBUF_IDX, linear_offset_byte, outData0.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 16, elment_offset, outData0.select<8, 1>(16));
                    break;
                default:
                    break;
                }
                break;
            case 2:
                outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 0).format<ushort>() = inData0.format<ushort>() >> bitshift;
                outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 8).format<ushort>() = inData1.format<ushort>() >> bitshift;

                //Padding unused by the first one pixel in the last sub block
                for (int i = last_block_size; i < SUB_BLOCK_PIXEL_WIDTH; i++)
                {
                    outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 8 + i) = outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 8);
                }

                switch (last_block_height)
                {
                case 8:
                    write(OUTBUF_IDX, linear_offset_byte, outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 6, outData6.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 7, outData7.select<8, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 8, elment_offset, outData0.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword, elment_offset, outData1.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 2, elment_offset, outData2.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 3, elment_offset, outData3.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 4, elment_offset, outData4.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 5, elment_offset, outData5.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 6, elment_offset, outData6.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 7, elment_offset, outData7.select<8, 1>(8));
                    break;
                case 7:
                    write(OUTBUF_IDX, linear_offset_byte, outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 6, outData6.select<8, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 8, elment_offset, outData0.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword, elment_offset, outData1.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 2, elment_offset, outData2.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 3, elment_offset, outData3.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 4, elment_offset, outData4.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 5, elment_offset, outData5.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 6, elment_offset, outData6.select<8, 1>(8));
                    break;
                case 6:
                    write(OUTBUF_IDX, linear_offset_byte, outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5.select<8, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 8, elment_offset, outData0.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword, elment_offset, outData1.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 2, elment_offset, outData2.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 3, elment_offset, outData3.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 4, elment_offset, outData4.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 5, elment_offset, outData5.select<8, 1>(8));
                    break;
                case 5:
                    write(OUTBUF_IDX, linear_offset_byte, outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<8, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 8, elment_offset, outData0.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword, elment_offset, outData1.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 2, elment_offset, outData2.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 3, elment_offset, outData3.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 4, elment_offset, outData4.select<8, 1>(8));
                    break;
                default:
                    break;
                }
                switch (last_block_height)
                {
                case 4:
                    write(OUTBUF_IDX, linear_offset_byte, outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<8, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 8, elment_offset, outData0.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword, elment_offset, outData1.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 2, elment_offset, outData2.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 3, elment_offset, outData3.select<8, 1>(8));
                    break;
                case 3:
                    write(OUTBUF_IDX, linear_offset_byte, outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<8, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 8, elment_offset, outData0.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword, elment_offset, outData1.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 2, elment_offset, outData2.select<8, 1>(8));
                    break;
                case 2:
                    write(OUTBUF_IDX, linear_offset_byte, outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1.select<8, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 8, elment_offset, outData0.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword, elment_offset, outData1.select<8, 1>(8));
                    break;
                case 1:
                    write(OUTBUF_IDX, linear_offset_byte, outData0.select<8, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 8, elment_offset, outData0.select<8, 1>(8));
                    break;
                default:
                    break;
                }
                break;
            case 1:
                outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 0).format<ushort>() = inData0.format<ushort>() >> bitshift;

                //Padding unused by the first one pixel in the last sub block
                for (int i = last_block_size; i < SUB_BLOCK_PIXEL_WIDTH; i++)
                {
                    outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, i) = outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 0);
                }

                switch (last_block_height)
                {
                case 8:
                    write(OUTBUF_IDX, linear_offset_dword, elment_offset, outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword, elment_offset, outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 2, elment_offset, outData2.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 3, elment_offset, outData3.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 4, elment_offset, outData4.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 5, elment_offset, outData5.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 6, elment_offset, outData6.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 7, elment_offset, outData7.select<8, 1>(0));
                    break;
                case 7:
                    write(OUTBUF_IDX, linear_offset_dword, elment_offset, outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword, elment_offset, outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 2, elment_offset, outData2.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 3, elment_offset, outData3.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 4, elment_offset, outData4.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 5, elment_offset, outData5.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 6, elment_offset, outData6.select<8, 1>(0));
                    break;
                case 6:
                    write(OUTBUF_IDX, linear_offset_dword, elment_offset, outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword, elment_offset, outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 2, elment_offset, outData2.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 3, elment_offset, outData3.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 4, elment_offset, outData4.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 5, elment_offset, outData5.select<8, 1>(0));
                    break;
                case 5:
                    write(OUTBUF_IDX, linear_offset_dword, elment_offset, outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword, elment_offset, outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 2, elment_offset, outData2.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 3, elment_offset, outData3.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 4, elment_offset, outData4.select<8, 1>(0));
                    break;
                default:
                    break;
                }
                switch (last_block_height)
                {
                case 4:
                    write(OUTBUF_IDX, linear_offset_dword, elment_offset, outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword, elment_offset, outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 2, elment_offset, outData2.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 3, elment_offset, outData3.select<8, 1>(0));
                    break;
                case 3:
                    write(OUTBUF_IDX, linear_offset_dword, elment_offset, outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword, elment_offset, outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 2, elment_offset, outData2.select<8, 1>(0));
                    break;
                case 2:
                    write(OUTBUF_IDX, linear_offset_dword, elment_offset, outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword, elment_offset, outData1.select<8, 1>(0));
                    break;
                case 1:
                    write(OUTBUF_IDX, linear_offset_dword, elment_offset, outData0.select<8, 1>(0));
                    break;
                default:
                    break;
                }
                break;
            default:
                break;
            }
        }
    }
}
extern "C" _GENX_MAIN_  void
surfaceCopy_read_shift_32x32(SurfaceIndex INBUF_IDX, SurfaceIndex OUTBUF_IDX, int width_dword, int height, int ShiftLeftOffsetInBytes, int bitshift, int threadHeight, int width_in_dword_no_padding, int height_no_padding, int src2D_start_x, int src2D_start_y)
{
    int horizOffset = get_thread_origin_x() * BLOCK_PIXEL_WIDTH;
    int vertOffset = get_thread_origin_y() * BLOCK_HEIGHT;
#pragma unroll
    for (int i = 0; i< 4; i++)
    {
        surfaceCopy_read_shift_32x8(INBUF_IDX, OUTBUF_IDX, width_dword, height, ShiftLeftOffsetInBytes, bitshift, horizOffset, vertOffset + SUB_BLOCK_HEIGHT * threadHeight * i, width_in_dword_no_padding, height_no_padding, src2D_start_x, src2D_start_y);
    }
}


extern "C" _GENX_MAIN_  void
surfaceMirror_write_NV12(SurfaceIndex INBUF_IDX, SurfaceIndex OUTBUF_IDX, int stride_dword, int stride_height,int ShiftLeftOffsetInBytes, int width_dword)
{
    //write Y plane
    matrix<uint, BLOCK_HEIGHT, BLOCK_PIXEL_WIDTH> inData_m;
    vector<ushort, SUB_BLOCK_PIXEL_WIDTH*4> idx(indexTable);
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData0(inData_m.row(0));
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData1(inData_m.row(1));
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData2(inData_m.row(2));
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData3(inData_m.row(3));
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData4(inData_m.row(4));
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData5(inData_m.row(5));
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData6(inData_m.row(6));
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData7(inData_m.row(7));

    vector_ref<ushort, SUB_BLOCK_PIXEL_WIDTH*2> idxUV = idx.select<16,1>(16);
    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> outData0;
    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> outData1;
    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> outData2;
    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> outData3;

    int horizOffset = get_thread_origin_x() * BLOCK_PIXEL_WIDTH;
    int vertOffset = get_thread_origin_y() * BLOCK_HEIGHT;
    int horizOffset_NV12 = get_thread_origin_x() * BLOCK_PIXEL_WIDTH;
    int vertOffset_NV12 = get_thread_origin_y() * BLOCK_HEIGHT_NV12;
    int width_byte = stride_dword << 2;
    int horizOffset_byte = horizOffset << 2;
    int sub_block_width_byte = SUB_BLOCK_PIXEL_WIDTH << 2;
    uint linear_offset_dword = vertOffset * stride_dword + (ShiftLeftOffsetInBytes/4) + (width_dword - horizOffset) - BLOCK_PIXEL_WIDTH;
    uint linear_offset_byte = (linear_offset_dword << 2);
    uint linear_offset_NV12_dword = stride_dword * ( stride_height + vertOffset_NV12 ) + (ShiftLeftOffsetInBytes>>2) + (width_dword - horizOffset_NV12 - BLOCK_PIXEL_WIDTH);
    uint linear_offset_NV12_byte = (linear_offset_NV12_dword << 2);

    read(DWALIGNED(INBUF_IDX), linear_offset_byte,                  inData0);
    read(DWALIGNED(INBUF_IDX), linear_offset_byte + width_byte,     inData1);
    read(DWALIGNED(INBUF_IDX), linear_offset_byte + width_byte * 2, inData2);
    read(DWALIGNED(INBUF_IDX), linear_offset_byte + width_byte * 3, inData3);
    read(DWALIGNED(INBUF_IDX), linear_offset_byte + width_byte * 4, inData4);
    read(DWALIGNED(INBUF_IDX), linear_offset_byte + width_byte * 5, inData5);
    read(DWALIGNED(INBUF_IDX), linear_offset_byte + width_byte * 6, inData6);
    read(DWALIGNED(INBUF_IDX), linear_offset_byte + width_byte * 7, inData7);
    #pragma unroll
    for(int i = 0; i < SUB_BLOCK_HEIGHT; i++)
        outData0.row(i) = inData_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 0).format<uchar>().iselect(idx).format<uint>();

    #pragma unroll
    for(int i = 0; i < SUB_BLOCK_HEIGHT; i++)
        outData1.row(i) = inData_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 8).format<uchar>().iselect(idx).format<uint>();

    #pragma unroll
    for(int i = 0; i < SUB_BLOCK_HEIGHT; i++)
        outData2.row(i) = inData_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 16).format<uchar>().iselect(idx).format<uint>();

    #pragma unroll
    for(int i = 0; i < SUB_BLOCK_HEIGHT; i++)
        outData3.row(i) = inData_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 24).format<uchar>().iselect(idx).format<uint>();
    /*outData0 = inData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 0);
    outData1 = inData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 8);
    outData2 = inData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 16);
    outData3 = inData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 24);*/

    write_plane(OUTBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset_byte,                          vertOffset, outData3);
    write_plane(OUTBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset_byte + sub_block_width_byte,   vertOffset, outData2);
    write_plane(OUTBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset_byte + sub_block_width_byte*2, vertOffset, outData1);
    write_plane(OUTBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset_byte + sub_block_width_byte*3, vertOffset, outData0);

    //write UV plane
    matrix<uint, BLOCK_HEIGHT_NV12, BLOCK_PIXEL_WIDTH> inData_NV12_m;
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData_NV12_0 = inData_NV12_m.row(0);
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData_NV12_1 = inData_NV12_m.row(1);
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData_NV12_2 = inData_NV12_m.row(2);
    vector_ref<uint, BLOCK_PIXEL_WIDTH> inData_NV12_3 = inData_NV12_m.row(3);

    matrix<uint, SUB_BLOCK_HEIGHT_NV12, SUB_BLOCK_PIXEL_WIDTH> outData_NV12_0;
    matrix<uint, SUB_BLOCK_HEIGHT_NV12, SUB_BLOCK_PIXEL_WIDTH> outData_NV12_1;
    matrix<uint, SUB_BLOCK_HEIGHT_NV12, SUB_BLOCK_PIXEL_WIDTH> outData_NV12_2;
    matrix<uint, SUB_BLOCK_HEIGHT_NV12, SUB_BLOCK_PIXEL_WIDTH> outData_NV12_3;


    //uint linear_offset_NV12_byte = (( stride_dword * ( stride_height + vertOffset_NV12 ) + horizOffset_NV12 ) << 2) + ShiftLeftOffsetInBytes;

    read(DWALIGNED(INBUF_IDX), linear_offset_NV12_byte,                  inData_NV12_0);
    read(DWALIGNED(INBUF_IDX), linear_offset_NV12_byte + width_byte,     inData_NV12_1);
    read(DWALIGNED(INBUF_IDX), linear_offset_NV12_byte + width_byte * 2, inData_NV12_2);
    read(DWALIGNED(INBUF_IDX), linear_offset_NV12_byte + width_byte * 3, inData_NV12_3);
    #pragma unroll
    for(int i = 0; i < SUB_BLOCK_HEIGHT_NV12; i++)
        outData_NV12_0.row(i) = inData_NV12_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 0).format<ushort>().iselect(idxUV).format<uint>();
    #pragma unroll
    for(int i = 0; i < SUB_BLOCK_HEIGHT_NV12; i++)
        outData_NV12_1.row(i) = inData_NV12_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 8).format<ushort>().iselect(idxUV).format<uint>();
    #pragma unroll
    for(int i = 0; i < SUB_BLOCK_HEIGHT_NV12; i++)
        outData_NV12_2.row(i) = inData_NV12_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 16).format<ushort>().iselect(idxUV).format<uint>();
    #pragma unroll
    for(int i = 0; i < SUB_BLOCK_HEIGHT_NV12; i++)
        outData_NV12_3.row(i) = inData_NV12_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 24).format<ushort>().iselect(idxUV).format<uint>();
    /*outData_NV12_0 = inData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 0);
    outData_NV12_1 = inData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 8);
    outData_NV12_2 = inData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 16);
    outData_NV12_3 = inData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 24);*/

    write_plane(OUTBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte,                          vertOffset >> 1, outData_NV12_3);
    write_plane(OUTBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte + sub_block_width_byte,   vertOffset >> 1, outData_NV12_2);
    write_plane(OUTBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte + sub_block_width_byte*2, vertOffset >> 1, outData_NV12_1);
    write_plane(OUTBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte + sub_block_width_byte*3, vertOffset >> 1, outData_NV12_0);

}


extern "C" _GENX_MAIN_  void
surfaceCopy_read_P010_shift(SurfaceIndex INBUF_IDX, SurfaceIndex OUTBUF_IDX, int width_dword, int height, int ShiftLeftOffsetInBytes, int bitshift, int stride_dword, int stride_height)
{
    //Y plane
    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> inData0;
    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> inData1;
    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> inData2;
    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> inData3;

    matrix<ushort, BLOCK_HEIGHT, BLOCK_PIXEL_WIDTH*2> outData_m;
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData0(outData_m.row(0).format<uint>());
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData1(outData_m.row(1).format<uint>());
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData2(outData_m.row(2).format<uint>());
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData3(outData_m.row(3).format<uint>());
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData4(outData_m.row(4).format<uint>());
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData5(outData_m.row(5).format<uint>());
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData6(outData_m.row(6).format<uint>());
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData7(outData_m.row(7).format<uint>());

    //UV plane
    matrix<uint, SUB_BLOCK_HEIGHT_NV12, SUB_BLOCK_PIXEL_WIDTH> inData_NV12_0;
    matrix<uint, SUB_BLOCK_HEIGHT_NV12, SUB_BLOCK_PIXEL_WIDTH> inData_NV12_1;
    matrix<uint, SUB_BLOCK_HEIGHT_NV12, SUB_BLOCK_PIXEL_WIDTH> inData_NV12_2;
    matrix<uint, SUB_BLOCK_HEIGHT_NV12, SUB_BLOCK_PIXEL_WIDTH> inData_NV12_3;

    matrix<ushort, BLOCK_HEIGHT_NV12, BLOCK_PIXEL_WIDTH*2> outData_NV12_m;
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData_NV12_0 = outData_NV12_m.row(0).format<uint>();
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData_NV12_1 = outData_NV12_m.row(1).format<uint>();
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData_NV12_2 = outData_NV12_m.row(2).format<uint>();
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData_NV12_3 = outData_NV12_m.row(3).format<uint>();

    int vertOffset = get_thread_origin_y() * BLOCK_HEIGHT;
    int horizOffset = get_thread_origin_x() * BLOCK_PIXEL_WIDTH;

    int horizOffset_NV12 = get_thread_origin_x() * BLOCK_PIXEL_WIDTH;
    int vertOffset_NV12 = get_thread_origin_y() * BLOCK_HEIGHT_NV12;

    int width_byte = stride_dword << 2;
    int horizOffset_byte = horizOffset << 2;
    int sub_block_width_byte = SUB_BLOCK_PIXEL_WIDTH << 2;

    uint linear_offset_dword = vertOffset * stride_dword + horizOffset + (ShiftLeftOffsetInBytes/4);
    uint linear_offset_byte = (linear_offset_dword << 2);

    uint linear_offset_NV12_dword = stride_dword * ( stride_height + vertOffset_NV12 ) + horizOffset_NV12 + (ShiftLeftOffsetInBytes/4);
    uint linear_offset_NV12_byte = (linear_offset_NV12_dword << 2);

    uint last_block_height = 8;
    if(height - vertOffset < BLOCK_HEIGHT)
    {
        last_block_height = height - vertOffset;
    }

    if (width_dword - horizOffset >= BLOCK_PIXEL_WIDTH)    // for aligned block
    {
        read_plane(INBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset_byte,                             vertOffset, inData0);
        read_plane(INBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset_byte + SUB_BLOCK_PIXEL_WIDTH*4,   vertOffset, inData1);
        read_plane(INBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset_byte + SUB_BLOCK_PIXEL_WIDTH*4*2, vertOffset, inData2);
        read_plane(INBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset_byte + SUB_BLOCK_PIXEL_WIDTH*4*3, vertOffset, inData3);

        read_plane(INBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte,                          vertOffset >> 1, inData_NV12_0);
        read_plane(INBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte + sub_block_width_byte,   vertOffset >> 1, inData_NV12_1);
        read_plane(INBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte + sub_block_width_byte*2, vertOffset >> 1, inData_NV12_2);
        read_plane(INBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte + sub_block_width_byte*3, vertOffset >> 1, inData_NV12_3);

        outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 0)  = inData0.format<ushort>()>>bitshift;
        outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 16) = inData1.format<ushort>()>>bitshift;
        outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 32) = inData2.format<ushort>()>>bitshift;
        outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 48) = inData3.format<ushort>()>>bitshift;

        outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 0)  = inData_NV12_0.format<ushort>()>>bitshift;
        outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 16) = inData_NV12_1.format<ushort>()>>bitshift;
        outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 32) = inData_NV12_2.format<ushort>()>>bitshift;
        outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 48) = inData_NV12_3.format<ushort>()>>bitshift;

        switch(last_block_height)
        {
        case 8:
            write(OUTBUF_IDX, linear_offset_byte,                  outData0);
            write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1);
            write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2);
            write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3);
            write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4);
            write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5);
            write(OUTBUF_IDX, linear_offset_byte + width_byte * 6, outData6);
            write(OUTBUF_IDX, linear_offset_byte + width_byte * 7, outData7);

            write(OUTBUF_IDX, linear_offset_NV12_byte,                  outData_NV12_0);
            write(OUTBUF_IDX, linear_offset_NV12_byte + width_byte,     outData_NV12_1);
            write(OUTBUF_IDX, linear_offset_NV12_byte + width_byte * 2, outData_NV12_2);
            write(OUTBUF_IDX, linear_offset_NV12_byte + width_byte * 3, outData_NV12_3);
            break;
        case 6:
            write(OUTBUF_IDX, linear_offset_byte,                  outData0);
            write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1);
            write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2);
            write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3);
            write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4);
            write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5);

            write(OUTBUF_IDX, linear_offset_NV12_byte,                  outData_NV12_0);
            write(OUTBUF_IDX, linear_offset_NV12_byte + width_byte,     outData_NV12_1);
            write(OUTBUF_IDX, linear_offset_NV12_byte + width_byte * 2, outData_NV12_2);
            break;
        case 4:
            write(OUTBUF_IDX, linear_offset_byte,                  outData0);
            write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1);
            write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2);
            write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3);

            write(OUTBUF_IDX, linear_offset_NV12_byte,              outData_NV12_0);
            write(OUTBUF_IDX, linear_offset_NV12_byte + width_byte, outData_NV12_1);
            break;
        case 2:
            write(OUTBUF_IDX, linear_offset_byte,                  outData0);
            write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1);

            write(OUTBUF_IDX, linear_offset_NV12_byte,                  outData_NV12_0);
            break;
        default:
            break;
        }
    }
    else    // for the unaligned most right blocks
    {
        uint block_width = width_dword - horizOffset;
        uint last_block_size = block_width;
        uint read_times = 1;
        read(INBUF_IDX, horizOffset_byte, vertOffset, inData0);
        read_plane(INBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte, vertOffset >> 1, inData_NV12_0);
        if (block_width > 8)
        {
            read_plane(INBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset_byte + sub_block_width_byte, vertOffset, inData1);
            read_plane(INBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte + sub_block_width_byte, vertOffset >> 1, inData_NV12_1);
            read_times = 2;
            last_block_size = last_block_size - 8;
            if (block_width > 16)
            {
                read_plane(INBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset_byte + sub_block_width_byte*2, vertOffset, inData2);
                read_plane(INBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte + sub_block_width_byte*2, vertOffset >> 1, inData_NV12_2);
                read_times = 3;
                last_block_size = last_block_size - 8;
                if (block_width > 24)
                {
                    read_plane(INBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset_byte + sub_block_width_byte*3, vertOffset, inData3);
                    read_plane(INBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte + sub_block_width_byte*3, vertOffset >> 1, inData_NV12_3);
                    read_times = 4;
                    last_block_size = last_block_size - 8;
                }
            }
        }

        vector<uint, SUB_BLOCK_PIXEL_WIDTH> elment_offset(0);

        for (uint i=0; i < last_block_size; i++)
        {
            elment_offset(i) = i;
        }

        switch (read_times)
        {
        case 4:
            outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 0)  = inData0.format<ushort>()>>bitshift;
            outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 16) = inData1.format<ushort>()>>bitshift;
            outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 32) = inData2.format<ushort>()>>bitshift;
            outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 48) = inData3.format<ushort>()>>bitshift;

            outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 0)  = inData_NV12_0.format<ushort>()>>bitshift;
            outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 16) = inData_NV12_1.format<ushort>()>>bitshift;
            outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 32) = inData_NV12_2.format<ushort>()>>bitshift;
            outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 48) = inData_NV12_3.format<ushort>()>>bitshift;

            //Padding unused by the first one pixel in the last sub block
            for (int i = last_block_size; i < SUB_BLOCK_PIXEL_WIDTH; i ++)
            {
                outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 24 + i) = outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 24);
                outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, 1, 1>(0, 24 + i) = outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, 1, 1>(0, 24);
            }

            switch(last_block_height)
            {
            case 8:
                //write Y plane to buffer
                write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 6, outData6.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 7, outData7.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_byte + 64,                  outData0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte,     outData1.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 2, outData2.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 3, outData3.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 4, outData4.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 5, outData5.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 6, outData6.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 7, outData7.select<8, 1>(16));

                write(OUTBUF_IDX, linear_offset_dword + 24,                   elment_offset, outData0.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + stride_dword,     elment_offset, outData1.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + stride_dword * 2, elment_offset, outData2.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + stride_dword * 3, elment_offset, outData3.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + stride_dword * 4, elment_offset, outData4.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + stride_dword * 5, elment_offset, outData5.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + stride_dword * 6, elment_offset, outData6.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + stride_dword * 7, elment_offset, outData7.select<8, 1>(24));

                //write UV plane to buffer
                write(OUTBUF_IDX, linear_offset_NV12_byte,                       outData_NV12_0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + stride_dword * 4,     outData_NV12_1.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + stride_dword * 4 * 2, outData_NV12_2.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + stride_dword * 4 * 3, outData_NV12_3.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_byte + 64,                  outData_NV12_0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_byte + 64 + width_byte,     outData_NV12_1.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_byte + 64 + width_byte * 2, outData_NV12_2.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_byte + 64 + width_byte * 3, outData_NV12_3.select<8, 1>(16));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 24,                   elment_offset, outData_NV12_0.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 24 + stride_dword,     elment_offset, outData_NV12_1.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 24 + stride_dword * 2, elment_offset, outData_NV12_2.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 24 + stride_dword * 3, elment_offset, outData_NV12_3.select<8, 1>(24));
                break;
            case 6:
                //write Y plane to buffer
                write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_byte + 64,                  outData0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte,     outData1.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 2, outData2.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 3, outData3.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 4, outData4.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 5, outData5.select<8, 1>(16));

                write(OUTBUF_IDX, linear_offset_dword + 24,                   elment_offset, outData0.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + stride_dword,     elment_offset, outData1.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + stride_dword * 2, elment_offset, outData2.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + stride_dword * 3, elment_offset, outData3.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + stride_dword * 4, elment_offset, outData4.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + stride_dword * 5, elment_offset, outData5.select<8, 1>(24));

                //write UV plane to buffer
                write(OUTBUF_IDX, linear_offset_NV12_byte,                       outData_NV12_0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + stride_dword * 4,     outData_NV12_1.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + stride_dword * 4 * 2, outData_NV12_2.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_byte + 64,                  outData_NV12_0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_byte + 64 + width_byte,     outData_NV12_1.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_byte + 64 + width_byte * 2, outData_NV12_2.select<8, 1>(16));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 24,                   elment_offset, outData_NV12_0.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 24 + stride_dword,     elment_offset, outData_NV12_1.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 24 + stride_dword * 2, elment_offset, outData_NV12_2.select<8, 1>(24));
                break;
            case 4:
                //write Y plane to buffer
                write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_byte + 64,                  outData0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte,     outData1.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 2, outData2.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 3, outData3.select<8, 1>(16));

                write(OUTBUF_IDX, linear_offset_dword + 24,                   elment_offset, outData0.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + stride_dword,     elment_offset, outData1.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + stride_dword * 2, elment_offset, outData2.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + stride_dword * 3, elment_offset, outData3.select<8, 1>(24));

                //write UV plane to buffer
                write(OUTBUF_IDX, linear_offset_NV12_byte,                       outData_NV12_0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + stride_dword * 4,     outData_NV12_1.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_byte + 64,                  outData_NV12_0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_byte + 64 + width_byte,     outData_NV12_1.select<8, 1>(16));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 24,                   elment_offset, outData_NV12_0.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 24 + stride_dword,     elment_offset, outData_NV12_1.select<8, 1>(24));
                break;
            case 2:
                //write Y plane to buffer
                write(OUTBUF_IDX, linear_offset_byte,              outData0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_byte + 64,              outData0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte, outData1.select<8, 1>(16));

                write(OUTBUF_IDX, linear_offset_dword + 24,               elment_offset, outData0.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + stride_dword, elment_offset, outData1.select<8, 1>(24));

                //write UV plane to buffer
                write(OUTBUF_IDX, linear_offset_NV12_byte, outData_NV12_0.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_byte + 64, outData_NV12_0.select<8, 1>(16));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 24, elment_offset, outData_NV12_0.select<8, 1>(24));
                break;
            default:
                break;
            }
            break;
        case 3:
            outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 0) = inData0.format<ushort>()>>bitshift;
            outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 16) = inData1.format<ushort>()>>bitshift;
            outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 32) = inData2.format<ushort>()>>bitshift;

            outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 0)  = inData_NV12_0.format<ushort>()>>bitshift;
            outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 16) = inData_NV12_1.format<ushort>()>>bitshift;
            outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 32) = inData_NV12_2.format<ushort>()>>bitshift;

            //Padding unused by the first one pixel in the last sub block
            for (int i = last_block_size; i < SUB_BLOCK_PIXEL_WIDTH; i ++)
            {
                outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 16 + i) = outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 16);
                outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, 1, 1>(0, 16 + i) = outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, 1, 1>(0, 16);
            }

            switch(last_block_height)
            {
            case 8:
                //write Y plane to buffer
                write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 6, outData6.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 7, outData7.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_dword + 16,                   elment_offset, outData0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + stride_dword,     elment_offset, outData1.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + stride_dword * 2, elment_offset, outData2.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + stride_dword * 3, elment_offset, outData3.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + stride_dword * 4, elment_offset, outData4.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + stride_dword * 5, elment_offset, outData5.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + stride_dword * 6, elment_offset, outData6.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + stride_dword * 7, elment_offset, outData7.select<8, 1>(16));

                //write UV plane to buffer
                write(OUTBUF_IDX, linear_offset_NV12_byte,                  outData_NV12_0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + width_byte,     outData_NV12_1.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + width_byte * 2, outData_NV12_2.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + width_byte * 3, outData_NV12_3.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 16,                   elment_offset, outData_NV12_0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 16 + stride_dword,     elment_offset, outData_NV12_1.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 16 + stride_dword * 2, elment_offset, outData_NV12_2.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 16 + stride_dword * 3, elment_offset, outData_NV12_3.select<8, 1>(16));
                break;
            case 6:
                //write Y plane to buffer
                write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_dword + 16,                   elment_offset, outData0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + stride_dword,     elment_offset, outData1.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + stride_dword * 2, elment_offset, outData2.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + stride_dword * 3, elment_offset, outData3.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + stride_dword * 4, elment_offset, outData4.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + stride_dword * 5, elment_offset, outData5.select<8, 1>(16));

                //write UV plane to buffer
                write(OUTBUF_IDX, linear_offset_NV12_byte,                  outData_NV12_0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + width_byte,     outData_NV12_1.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + width_byte * 2, outData_NV12_2.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 16,                   elment_offset, outData_NV12_0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 16 + stride_dword,     elment_offset, outData_NV12_1.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 16 + stride_dword * 2, elment_offset, outData_NV12_2.select<8, 1>(16));
                break;
            case 4:
                //write Y plane to buffer
                write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_dword + 16,                   elment_offset, outData0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + stride_dword,     elment_offset, outData1.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + stride_dword * 2, elment_offset, outData2.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + stride_dword * 3, elment_offset, outData3.select<8, 1>(16));

                //write UV plane to buffer
                write(OUTBUF_IDX, linear_offset_NV12_byte,              outData_NV12_0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + width_byte, outData_NV12_1.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 16,               elment_offset, outData_NV12_0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 16 + stride_dword, elment_offset, outData_NV12_1.select<8, 1>(16));
                break;
            case 2:
                //write Y plane to buffer
                write(OUTBUF_IDX, linear_offset_byte,              outData0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_dword + 16,               elment_offset, outData0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + stride_dword, elment_offset, outData1.select<8, 1>(16));

                //write UV plane to buffer
                write(OUTBUF_IDX, linear_offset_NV12_byte, outData_NV12_0.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 16, elment_offset, outData_NV12_0.select<8, 1>(16));
                break;
            default:
                break;
            }
            break;
        case 2:
            outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 0) = inData0.format<ushort>()>>bitshift;
            outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 16) = inData1.format<ushort>()>>bitshift;

            outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 0) = inData_NV12_0.format<ushort>()>>bitshift;
            outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 16) = inData_NV12_1.format<ushort>()>>bitshift;

            //Padding unused by the first one pixel in the last sub block
            for (int i = last_block_size; i < SUB_BLOCK_PIXEL_WIDTH; i ++)
            {
                outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 8 + i) = outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 8);
                outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, 1, 1>(0, 8 + i) = outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, 1, 1>(0, 8);
            }
            switch(last_block_height)
            {
            case 8:
                write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 6, outData6.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 7, outData7.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_dword + 8,                   elment_offset, outData0.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + stride_dword,     elment_offset, outData1.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + stride_dword * 2, elment_offset, outData2.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + stride_dword * 3, elment_offset, outData3.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + stride_dword * 4, elment_offset, outData4.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + stride_dword * 5, elment_offset, outData5.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + stride_dword * 6, elment_offset, outData6.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + stride_dword * 7, elment_offset, outData7.select<8, 1>(8));

                write(OUTBUF_IDX, linear_offset_NV12_byte,                  outData_NV12_0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + width_byte,     outData_NV12_1.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + width_byte * 2, outData_NV12_2.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + width_byte * 3, outData_NV12_3.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 8,                   elment_offset, outData_NV12_0.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 8 + stride_dword,     elment_offset, outData_NV12_1.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 8 + stride_dword * 2, elment_offset, outData_NV12_2.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 8 + stride_dword * 3, elment_offset, outData_NV12_3.select<8, 1>(8));
                break;
            case 6:
                write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_dword + 8,                   elment_offset, outData0.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + stride_dword,     elment_offset, outData1.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + stride_dword * 2, elment_offset, outData2.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + stride_dword * 3, elment_offset, outData3.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + stride_dword * 4, elment_offset, outData4.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + stride_dword * 5, elment_offset, outData5.select<8, 1>(8));

                write(OUTBUF_IDX, linear_offset_NV12_byte,                  outData_NV12_0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + width_byte,     outData_NV12_1.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + width_byte * 2, outData_NV12_2.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 8,                   elment_offset, outData_NV12_0.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 8 + stride_dword,     elment_offset, outData_NV12_1.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 8 + stride_dword * 2, elment_offset, outData_NV12_2.select<8, 1>(8));
                break;
            case 4:
                write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_dword + 8,                   elment_offset, outData0.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + stride_dword,     elment_offset, outData1.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + stride_dword * 2, elment_offset, outData2.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + stride_dword * 3, elment_offset, outData3.select<8, 1>(8));

                write(OUTBUF_IDX, linear_offset_NV12_byte,              outData_NV12_0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + width_byte, outData_NV12_1.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 8,               elment_offset, outData_NV12_0.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 8 + stride_dword, elment_offset, outData_NV12_1.select<8, 1>(8));
                break;
            case 2:
                write(OUTBUF_IDX, linear_offset_byte,              outData0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_dword + 8,               elment_offset, outData0.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + stride_dword, elment_offset, outData1.select<8, 1>(8));

                write(OUTBUF_IDX, linear_offset_NV12_byte, outData_NV12_0.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 8, elment_offset, outData_NV12_0.select<8, 1>(8));
                break;
            default:
                break;
            }
            break;
        case 1:
            outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 0) = inData0.format<ushort>()>>bitshift;
            outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, SUB_BLOCK_PIXEL_WIDTH*2, 1>(0, 0) = inData_NV12_0.format<ushort>()>>bitshift;

            //Padding unused by the first one pixel in the last sub block
            for (int i = last_block_size; i < SUB_BLOCK_PIXEL_WIDTH; i ++)
            {
                outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, i) = outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 0);
                outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, 1, 1>(0, i) = outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, 1, 1>(0, 0);
            }

            switch(last_block_height)
            {
            case 8:
                write(OUTBUF_IDX, linear_offset_dword,                   elment_offset, outData0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + stride_dword,     elment_offset, outData1.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + stride_dword * 2, elment_offset, outData2.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + stride_dword * 3, elment_offset, outData3.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + stride_dword * 4, elment_offset, outData4.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + stride_dword * 5, elment_offset, outData5.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + stride_dword * 6, elment_offset, outData6.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + stride_dword * 7, elment_offset, outData7.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword,                   elment_offset, outData_NV12_0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_dword + stride_dword,     elment_offset, outData_NV12_1.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_dword + stride_dword * 2, elment_offset, outData_NV12_2.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_dword + stride_dword * 3, elment_offset, outData_NV12_3.select<8, 1>(0));
                break;
            case 6:
                write(OUTBUF_IDX, linear_offset_dword,                   elment_offset, outData0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + stride_dword,     elment_offset, outData1.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + stride_dword * 2, elment_offset, outData2.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + stride_dword * 3, elment_offset, outData3.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + stride_dword * 4, elment_offset, outData4.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + stride_dword * 5, elment_offset, outData5.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword,                   elment_offset, outData_NV12_0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_dword + stride_dword,     elment_offset, outData_NV12_1.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_dword + stride_dword * 2, elment_offset, outData_NV12_2.select<8, 1>(0));
                break;
            case 4:
                write(OUTBUF_IDX, linear_offset_dword,                   elment_offset, outData0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + stride_dword,     elment_offset, outData1.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + stride_dword * 2, elment_offset, outData2.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + stride_dword * 3, elment_offset, outData3.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword,               elment_offset, outData_NV12_0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_dword + stride_dword, elment_offset, outData_NV12_1.select<8, 1>(0));
                break;
            case 2:
                write(OUTBUF_IDX, linear_offset_dword,               elment_offset, outData0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + stride_dword, elment_offset, outData1.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword, elment_offset, outData_NV12_0.select<8, 1>(0));
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
    }
}
//*/
extern "C" _GENX_MAIN_  void
surfaceMirror_read_NV12(SurfaceIndex INBUF_IDX, SurfaceIndex OUTBUF_IDX, int stride_dword, int height,int ShiftLeftOffsetInBytes, int width_dword, int height_stride)
{
    //Y plane
    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> inData0;
    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> inData1;
    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> inData2;
    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> inData3;
    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> inData4;
    vector<ushort, SUB_BLOCK_PIXEL_WIDTH*4> idx(indexTable);
    matrix<uint, BLOCK_HEIGHT, BLOCK_PIXEL_WIDTH> outData_m;
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData0(outData_m.row(0));
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData1(outData_m.row(1));
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData2(outData_m.row(2));
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData3(outData_m.row(3));
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData4(outData_m.row(4));
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData5(outData_m.row(5));
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData6(outData_m.row(6));
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData7(outData_m.row(7));
    vector_ref<ushort, SUB_BLOCK_PIXEL_WIDTH*2> idxUV = idx.select<16,1>(16);

    //UV plane
    matrix<uint, SUB_BLOCK_HEIGHT_NV12, SUB_BLOCK_PIXEL_WIDTH> inData_NV12_0;
    matrix<uint, SUB_BLOCK_HEIGHT_NV12, SUB_BLOCK_PIXEL_WIDTH> inData_NV12_1;
    matrix<uint, SUB_BLOCK_HEIGHT_NV12, SUB_BLOCK_PIXEL_WIDTH> inData_NV12_2;
    matrix<uint, SUB_BLOCK_HEIGHT_NV12, SUB_BLOCK_PIXEL_WIDTH> inData_NV12_3;

    matrix<uint, BLOCK_HEIGHT_NV12, BLOCK_PIXEL_WIDTH> outData_NV12_m;
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData_NV12_0 = outData_NV12_m.row(0);
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData_NV12_1 = outData_NV12_m.row(1);
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData_NV12_2 = outData_NV12_m.row(2);
    vector_ref<uint, BLOCK_PIXEL_WIDTH> outData_NV12_3 = outData_NV12_m.row(3);

    int horizOffset = get_thread_origin_x() * BLOCK_PIXEL_WIDTH;
    int vertOffset = get_thread_origin_y() * BLOCK_HEIGHT;

    int horizOffset_NV12 = get_thread_origin_x() * BLOCK_PIXEL_WIDTH;
    int vertOffset_NV12 = get_thread_origin_y() * BLOCK_HEIGHT_NV12;

    int width_byte = width_dword << 2;
    int stride_byte = stride_dword << 2;
    int horizOffset_byte = horizOffset << 2;
    int sub_block_width_byte = SUB_BLOCK_PIXEL_WIDTH << 2;

    uint last_block_height = 8;
    if(height - vertOffset < BLOCK_HEIGHT)
    {
        last_block_height = height - vertOffset;
    }

    if (width_dword - horizOffset >= BLOCK_PIXEL_WIDTH)    // for aligned block
    {
        uint linear_offset_dword = vertOffset * stride_dword + (ShiftLeftOffsetInBytes/4) + (width_dword - horizOffset) - BLOCK_PIXEL_WIDTH;
        uint linear_offset_byte = (linear_offset_dword << 2);
        uint linear_offset_NV12_dword = stride_dword * ( height_stride + vertOffset_NV12 ) + (ShiftLeftOffsetInBytes/4) + (width_dword - horizOffset_NV12 - BLOCK_PIXEL_WIDTH);
        uint linear_offset_NV12_byte = (linear_offset_NV12_dword << 2);
        read_plane(INBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset_byte,                             vertOffset, inData0);
        read_plane(INBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset_byte + SUB_BLOCK_PIXEL_WIDTH*4,   vertOffset, inData1);
        read_plane(INBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset_byte + SUB_BLOCK_PIXEL_WIDTH*4*2, vertOffset, inData2);
        read_plane(INBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset_byte + SUB_BLOCK_PIXEL_WIDTH*4*3, vertOffset, inData3);

        read_plane(INBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte,                          vertOffset >> 1, inData_NV12_0);
        read_plane(INBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte + sub_block_width_byte,   vertOffset >> 1, inData_NV12_1);
        read_plane(INBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte + sub_block_width_byte*2, vertOffset >> 1, inData_NV12_2);
        read_plane(INBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte + sub_block_width_byte*3, vertOffset >> 1, inData_NV12_3);

        #pragma unroll
        for(int i = 0; i < SUB_BLOCK_HEIGHT; i++)
            outData_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 24) = inData0.row(i).format<uchar>().iselect(idx).format<uint>();

        #pragma unroll
        for(int i = 0; i < SUB_BLOCK_HEIGHT; i++)
            outData_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 16) = inData1.row(i).format<uchar>().iselect(idx).format<uint>();

        #pragma unroll
        for(int i = 0; i < SUB_BLOCK_HEIGHT; i++)
            outData_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 8) = inData2.row(i).format<uchar>().iselect(idx).format<uint>();

        #pragma unroll
        for(int i = 0; i < SUB_BLOCK_HEIGHT; i++)
            outData_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 0) = inData3.row(i).format<uchar>().iselect(idx).format<uint>();

        #pragma unroll
        for(int i = 0; i < SUB_BLOCK_HEIGHT_NV12; i++)
            outData_NV12_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 24) = inData_NV12_0.row(i).format<ushort>().iselect(idxUV).format<uint>();
        #pragma unroll
        for(int i = 0; i < SUB_BLOCK_HEIGHT_NV12; i++)
            outData_NV12_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 16) = inData_NV12_1.row(i).format<ushort>().iselect(idxUV).format<uint>();
        #pragma unroll
        for(int i = 0; i < SUB_BLOCK_HEIGHT_NV12; i++)
            outData_NV12_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 8) = inData_NV12_2.row(i).format<ushort>().iselect(idxUV).format<uint>();
        #pragma unroll
        for(int i = 0; i < SUB_BLOCK_HEIGHT_NV12; i++)
            outData_NV12_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 0) = inData_NV12_3.row(i).format<ushort>().iselect(idxUV).format<uint>();

        switch(last_block_height)
        {
        case 8:
            write(OUTBUF_IDX, linear_offset_byte,                  outData0);
            write(OUTBUF_IDX, linear_offset_byte + stride_byte,     outData1);
            write(OUTBUF_IDX, linear_offset_byte + stride_byte * 2, outData2);
            write(OUTBUF_IDX, linear_offset_byte + stride_byte * 3, outData3);
            write(OUTBUF_IDX, linear_offset_byte + stride_byte * 4, outData4);
            write(OUTBUF_IDX, linear_offset_byte + stride_byte * 5, outData5);
            write(OUTBUF_IDX, linear_offset_byte + stride_byte * 6, outData6);
            write(OUTBUF_IDX, linear_offset_byte + stride_byte * 7, outData7);

            write(OUTBUF_IDX, linear_offset_NV12_byte,                  outData_NV12_0);
            write(OUTBUF_IDX, linear_offset_NV12_byte + stride_byte,     outData_NV12_1);
            write(OUTBUF_IDX, linear_offset_NV12_byte + stride_byte * 2, outData_NV12_2);
            write(OUTBUF_IDX, linear_offset_NV12_byte + stride_byte * 3, outData_NV12_3);
            break;
        case 6:
            write(OUTBUF_IDX, linear_offset_byte,                  outData0);
            write(OUTBUF_IDX, linear_offset_byte + stride_byte,     outData1);
            write(OUTBUF_IDX, linear_offset_byte + stride_byte * 2, outData2);
            write(OUTBUF_IDX, linear_offset_byte + stride_byte * 3, outData3);
            write(OUTBUF_IDX, linear_offset_byte + stride_byte * 4, outData4);
            write(OUTBUF_IDX, linear_offset_byte + stride_byte * 5, outData5);

            write(OUTBUF_IDX, linear_offset_NV12_byte,                  outData_NV12_0);
            write(OUTBUF_IDX, linear_offset_NV12_byte + stride_byte,     outData_NV12_1);
            write(OUTBUF_IDX, linear_offset_NV12_byte + stride_byte * 2, outData_NV12_2);
            break;
        case 4:
            write(OUTBUF_IDX, linear_offset_byte,                  outData0);
            write(OUTBUF_IDX, linear_offset_byte + stride_byte,     outData1);
            write(OUTBUF_IDX, linear_offset_byte + stride_byte * 2, outData2);
            write(OUTBUF_IDX, linear_offset_byte + stride_byte * 3, outData3);

            write(OUTBUF_IDX, linear_offset_NV12_byte,              outData_NV12_0);
            write(OUTBUF_IDX, linear_offset_NV12_byte + stride_byte, outData_NV12_1);
            break;
        case 2:
            write(OUTBUF_IDX, linear_offset_byte,                  outData0);
            write(OUTBUF_IDX, linear_offset_byte + stride_byte,     outData1);

            write(OUTBUF_IDX, linear_offset_NV12_byte,                  outData_NV12_0);
            break;
        default:
            break;
        }
    }
    else    // for the unaligned most right blocks
    {
        //for mirroring most right read being written to most left border, so no pixel offset required.
        uint linear_offset_dword = vertOffset * stride_dword + (ShiftLeftOffsetInBytes/4);
        uint linear_offset_byte = (linear_offset_dword << 2);
        uint linear_offset_NV12_dword = stride_dword * ( height_stride + vertOffset_NV12 ) + (ShiftLeftOffsetInBytes/4);
        uint linear_offset_NV12_byte = (linear_offset_NV12_dword << 2);
        uint block_width = width_dword - horizOffset;
        uint last_block_size = block_width;
        uint read_times = 1;
        read(INBUF_IDX, horizOffset_byte, vertOffset, inData0);
        read_plane(INBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte, vertOffset >> 1, inData_NV12_0);
        if (block_width > 8)
        {
            read_plane(INBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset_byte + sub_block_width_byte, vertOffset, inData1);
            read_plane(INBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte + sub_block_width_byte, vertOffset >> 1, inData_NV12_1);
            read_times = 2;
            last_block_size = last_block_size - 8;
            if (block_width > 16)
            {
                read_plane(INBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset_byte + sub_block_width_byte*2, vertOffset, inData2);
                read_plane(INBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte + sub_block_width_byte*2, vertOffset >> 1, inData_NV12_2);
                read_times = 3;
                last_block_size = last_block_size - 8;
                if (block_width > 24)
                {
                    read_plane(INBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset_byte + sub_block_width_byte*3, vertOffset, inData3);
                    read_plane(INBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset_byte + sub_block_width_byte*3, vertOffset >> 1, inData_NV12_3);
                    read_times = 4;
                    last_block_size = last_block_size - 8;
                }
            }
        }

        vector<uint, SUB_BLOCK_PIXEL_WIDTH> elment_offset(0);

        for (uint i=0; i < last_block_size; i++)
        {
            elment_offset(i) = i;
        }

        switch (read_times)
        {
        case 4:
            #pragma unroll
            for(int i = 0; i < SUB_BLOCK_HEIGHT; i++)
                outData_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 24) = inData0.row(i).format<uchar>().iselect(idx).format<uint>();

            #pragma unroll
            for(int i = 0; i < SUB_BLOCK_HEIGHT; i++)
                outData_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 16) = inData1.row(i).format<uchar>().iselect(idx).format<uint>();

            #pragma unroll
            for(int i = 0; i < SUB_BLOCK_HEIGHT; i++)
                outData_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 8) = inData2.row(i).format<uchar>().iselect(idx).format<uint>();

            #pragma unroll
            for(int i = 0; i < SUB_BLOCK_HEIGHT; i++)
                outData_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 0) = inData3.row(i).format<uchar>().iselect(idx).format<uint>();

            #pragma unroll
            for(int i = 0; i < SUB_BLOCK_HEIGHT_NV12; i++)
                outData_NV12_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 24) = inData_NV12_0.row(i).format<ushort>().iselect(idxUV).format<uint>();
            #pragma unroll
            for(int i = 0; i < SUB_BLOCK_HEIGHT_NV12; i++)
                outData_NV12_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 16) = inData_NV12_1.row(i).format<ushort>().iselect(idxUV).format<uint>();
            #pragma unroll
            for(int i = 0; i < SUB_BLOCK_HEIGHT_NV12; i++)
                outData_NV12_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 8) = inData_NV12_2.row(i).format<ushort>().iselect(idxUV).format<uint>();
            #pragma unroll
            for(int i = 0; i < SUB_BLOCK_HEIGHT_NV12; i++)
                outData_NV12_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 0) = inData_NV12_3.row(i).format<ushort>().iselect(idxUV).format<uint>();

            //Padding unused by the first one pixel in the last sub block
            for (int i = 0; i < last_block_height; i ++)
            {
                outData_m.row(i).select<24, 1>(0) = outData_m.row(i).select<24, 1>(last_block_size);
            }
            for (int i = 0; i < last_block_height/2; i ++)
                outData_NV12_m.row(i).select<24, 1>(0) = outData_NV12_m.row(i).select<24, 1>(last_block_size);

            for (int i = 24; i < 24+last_block_size; i ++)
            {
                outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, i) = outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, i+last_block_size);
                outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, 1, 1>(0, i) = outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, 1, 1>(0, i+last_block_size);
            }

            switch(last_block_height)
            {
            case 8:
                //write Y plane to buffer
                write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte,     outData1.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 2, outData2.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 3, outData3.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 4, outData4.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 5, outData5.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 6, outData6.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 7, outData7.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_byte + 64,                  outData0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + stride_byte,     outData1.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + stride_byte * 2, outData2.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + stride_byte * 3, outData3.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + stride_byte * 4, outData4.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + stride_byte * 5, outData5.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + stride_byte * 6, outData6.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + stride_byte * 7, outData7.select<8, 1>(16));

                write(OUTBUF_IDX, linear_offset_dword + 24,                   elment_offset, outData0.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + stride_dword,     elment_offset, outData1.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + stride_dword * 2, elment_offset, outData2.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + stride_dword * 3, elment_offset, outData3.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + stride_dword * 4, elment_offset, outData4.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + stride_dword * 5, elment_offset, outData5.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + stride_dword * 6, elment_offset, outData6.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + stride_dword * 7, elment_offset, outData7.select<8, 1>(24));

                //write UV plane to buffer
                write(OUTBUF_IDX, linear_offset_NV12_byte,                       outData_NV12_0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + stride_dword * 4,     outData_NV12_1.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + stride_dword * 4 * 2, outData_NV12_2.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + stride_dword * 4 * 3, outData_NV12_3.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_byte + 64,                  outData_NV12_0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_byte + 64 + stride_byte,     outData_NV12_1.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_byte + 64 + stride_byte * 2, outData_NV12_2.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_byte + 64 + stride_byte * 3, outData_NV12_3.select<8, 1>(16));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 24,                   elment_offset, outData_NV12_0.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 24 + stride_dword,     elment_offset, outData_NV12_1.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 24 + stride_dword * 2, elment_offset, outData_NV12_2.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 24 + stride_dword * 3, elment_offset, outData_NV12_3.select<8, 1>(24));
                break;
            case 6:
                //write Y plane to buffer
                write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte,     outData1.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 2, outData2.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 3, outData3.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 4, outData4.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 5, outData5.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_byte + 64,                  outData0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + stride_byte,     outData1.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + stride_byte * 2, outData2.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + stride_byte * 3, outData3.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + stride_byte * 4, outData4.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + stride_byte * 5, outData5.select<8, 1>(16));

                write(OUTBUF_IDX, linear_offset_dword + 24,                   elment_offset, outData0.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + stride_dword,     elment_offset, outData1.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + stride_dword * 2, elment_offset, outData2.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + stride_dword * 3, elment_offset, outData3.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + stride_dword * 4, elment_offset, outData4.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + stride_dword * 5, elment_offset, outData5.select<8, 1>(24));

                //write UV plane to buffer
                write(OUTBUF_IDX, linear_offset_NV12_byte,                       outData_NV12_0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + stride_dword * 4,     outData_NV12_1.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + stride_dword * 4 * 2, outData_NV12_2.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_byte + 64,                  outData_NV12_0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_byte + 64 + stride_byte,     outData_NV12_1.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_byte + 64 + stride_byte * 2, outData_NV12_2.select<8, 1>(16));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 24,                   elment_offset, outData_NV12_0.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 24 + stride_dword,     elment_offset, outData_NV12_1.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 24 + stride_dword * 2, elment_offset, outData_NV12_2.select<8, 1>(24));
                break;
            case 4:
                //write Y plane to buffer
                write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte,     outData1.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 2, outData2.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 3, outData3.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_byte + 64,                  outData0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + stride_byte,     outData1.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + stride_byte * 2, outData2.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + stride_byte * 3, outData3.select<8, 1>(16));

                write(OUTBUF_IDX, linear_offset_dword + 24,                   elment_offset, outData0.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + stride_dword,     elment_offset, outData1.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + stride_dword * 2, elment_offset, outData2.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + stride_dword * 3, elment_offset, outData3.select<8, 1>(24));

                //write UV plane to buffer
                write(OUTBUF_IDX, linear_offset_NV12_byte,                       outData_NV12_0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + stride_dword * 4,     outData_NV12_1.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_byte + 64,                  outData_NV12_0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_byte + 64 + stride_byte,     outData_NV12_1.select<8, 1>(16));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 24,                   elment_offset, outData_NV12_0.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 24 + stride_dword,     elment_offset, outData_NV12_1.select<8, 1>(24));
                break;
            case 2:
                //write Y plane to buffer
                write(OUTBUF_IDX, linear_offset_byte,              outData0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte, outData1.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_byte + 64,              outData0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_byte + 64 + stride_byte, outData1.select<8, 1>(16));

                write(OUTBUF_IDX, linear_offset_dword + 24,               elment_offset, outData0.select<8, 1>(24));
                write(OUTBUF_IDX, linear_offset_dword + 24 + stride_dword, elment_offset, outData1.select<8, 1>(24));

                //write UV plane to buffer
                write(OUTBUF_IDX, linear_offset_NV12_byte, outData_NV12_0.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_byte + 64, outData_NV12_0.select<8, 1>(16));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 24, elment_offset, outData_NV12_0.select<8, 1>(24));
                break;
            default:
                break;
            }
            break;
        case 3:

            #pragma unroll
            for(int i = 0; i < SUB_BLOCK_HEIGHT; i++)
                outData_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 16) = inData0.row(i).format<uchar>().iselect(idx).format<uint>();

            #pragma unroll
            for(int i = 0; i < SUB_BLOCK_HEIGHT; i++)
                outData_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 8) = inData1.row(i).format<uchar>().iselect(idx).format<uint>();

            #pragma unroll
            for(int i = 0; i < SUB_BLOCK_HEIGHT; i++)
                outData_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 0) = inData2.row(i).format<uchar>().iselect(idx).format<uint>();

            #pragma unroll
            for(int i = 0; i < SUB_BLOCK_HEIGHT_NV12; i++)
                outData_NV12_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 16) = inData_NV12_0.row(i).format<ushort>().iselect(idxUV).format<uint>();
            #pragma unroll
            for(int i = 0; i < SUB_BLOCK_HEIGHT_NV12; i++)
                outData_NV12_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 8) = inData_NV12_1.row(i).format<ushort>().iselect(idxUV).format<uint>();
            #pragma unroll
            for(int i = 0; i < SUB_BLOCK_HEIGHT_NV12; i++)
                outData_NV12_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 0) = inData_NV12_2.row(i).format<ushort>().iselect(idxUV).format<uint>();

            if(last_block_size<8){
                for (int i = 0; i < last_block_height; i ++)
                {
                    outData_m.row(i).select<16, 1>(0) = outData_m.row(i).select<16, 1>(last_block_size);
                }
                for (int i = 0; i < last_block_height/2; i ++)
                    outData_NV12_m.row(i).select<16, 1>(0) = outData_NV12_m.row(i).select<16, 1>(last_block_size);

                for (int i = 16; i < 16+last_block_size; i ++)
                {
                    outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, i) = outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, i+last_block_size);
                    outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, 1, 1>(0, i) = outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, 1, 1>(0, i+last_block_size);
                }
            }
            switch(last_block_height)
            {
            case 8:
                //write Y plane to buffer
                write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte,     outData1.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 2, outData2.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 3, outData3.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 4, outData4.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 5, outData5.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 6, outData6.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 7, outData7.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_dword + 16,                   elment_offset, outData0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + stride_dword,     elment_offset, outData1.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + stride_dword * 2, elment_offset, outData2.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + stride_dword * 3, elment_offset, outData3.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + stride_dword * 4, elment_offset, outData4.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + stride_dword * 5, elment_offset, outData5.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + stride_dword * 6, elment_offset, outData6.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + stride_dword * 7, elment_offset, outData7.select<8, 1>(16));

                //write UV plane to buffer
                write(OUTBUF_IDX, linear_offset_NV12_byte,                  outData_NV12_0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + stride_byte,     outData_NV12_1.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + stride_byte * 2, outData_NV12_2.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + stride_byte * 3, outData_NV12_3.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 16,                   elment_offset, outData_NV12_0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 16 + stride_dword,     elment_offset, outData_NV12_1.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 16 + stride_dword * 2, elment_offset, outData_NV12_2.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 16 + stride_dword * 3, elment_offset, outData_NV12_3.select<8, 1>(16));
                break;
            case 6:
                //write Y plane to buffer
                write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte,     outData1.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 2, outData2.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 3, outData3.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 4, outData4.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 5, outData5.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_dword + 16,                   elment_offset, outData0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + stride_dword,     elment_offset, outData1.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + stride_dword * 2, elment_offset, outData2.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + stride_dword * 3, elment_offset, outData3.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + stride_dword * 4, elment_offset, outData4.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + stride_dword * 5, elment_offset, outData5.select<8, 1>(16));

                //write UV plane to buffer
                write(OUTBUF_IDX, linear_offset_NV12_byte,                  outData_NV12_0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + stride_byte,     outData_NV12_1.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + stride_byte * 2, outData_NV12_2.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 16,                   elment_offset, outData_NV12_0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 16 + stride_dword,     elment_offset, outData_NV12_1.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 16 + stride_dword * 2, elment_offset, outData_NV12_2.select<8, 1>(16));
                break;
            case 4:
                //write Y plane to buffer
                write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte,     outData1.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 2, outData2.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 3, outData3.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_dword + 16,                   elment_offset, outData0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + stride_dword,     elment_offset, outData1.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + stride_dword * 2, elment_offset, outData2.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + stride_dword * 3, elment_offset, outData3.select<8, 1>(16));

                //write UV plane to buffer
                write(OUTBUF_IDX, linear_offset_NV12_byte,              outData_NV12_0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + stride_byte, outData_NV12_1.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 16,               elment_offset, outData_NV12_0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 16 + stride_dword, elment_offset, outData_NV12_1.select<8, 1>(16));
                break;
            case 2:
                //write Y plane to buffer
                write(OUTBUF_IDX, linear_offset_byte,              outData0.select<16, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte, outData1.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_dword + 16,               elment_offset, outData0.select<8, 1>(16));
                write(OUTBUF_IDX, linear_offset_dword + 16 + stride_dword, elment_offset, outData1.select<8, 1>(16));

                //write UV plane to buffer
                write(OUTBUF_IDX, linear_offset_NV12_byte, outData_NV12_0.select<16, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 16, elment_offset, outData_NV12_0.select<8, 1>(16));
                break;
            default:
                break;
            }
            break;
        case 2:
            #pragma unroll
            for(int i = 0; i < SUB_BLOCK_HEIGHT; i++)
                outData_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 8) = inData0.row(i).format<uchar>().iselect(idx).format<uint>();

            #pragma unroll
            for(int i = 0; i < SUB_BLOCK_HEIGHT; i++)
                outData_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 0) = inData1.row(i).format<uchar>().iselect(idx).format<uint>();

            #pragma unroll
            for(int i = 0; i < SUB_BLOCK_HEIGHT_NV12; i++)
                outData_NV12_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 8) = inData_NV12_0.row(i).format<ushort>().iselect(idxUV).format<uint>();
            #pragma unroll
            for(int i = 0; i < SUB_BLOCK_HEIGHT_NV12; i++)
                outData_NV12_m.select<1, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(i, 0) = inData_NV12_1.row(i).format<ushort>().iselect(idxUV).format<uint>();

            if(last_block_size<8){
                for (int i = 0; i < last_block_height; i ++)
                {
                    outData_m.row(i).select<8, 1>(0) = outData_m.row(i).select<8, 1>(last_block_size);
                }
                for (int i = 0; i < last_block_height/2; i ++)
                    outData_NV12_m.row(i).select<8, 1>(0) = outData_NV12_m.row(i).select<8, 1>(last_block_size);

                for (int i = 8; i < 8+last_block_size; i ++)
                {
                    outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, i) = outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, i+last_block_size);
                    outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, 1, 1>(0, i) = outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, 1, 1>(0, i+last_block_size);
                }
            }
            switch(last_block_height)
            {
            case 8:
                write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte,     outData1.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 2, outData2.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 3, outData3.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 4, outData4.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 5, outData5.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 6, outData6.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 7, outData7.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_dword + 8,                   elment_offset, outData0.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + stride_dword,     elment_offset, outData1.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + stride_dword * 2, elment_offset, outData2.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + stride_dword * 3, elment_offset, outData3.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + stride_dword * 4, elment_offset, outData4.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + stride_dword * 5, elment_offset, outData5.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + stride_dword * 6, elment_offset, outData6.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + stride_dword * 7, elment_offset, outData7.select<8, 1>(8));

                write(OUTBUF_IDX, linear_offset_NV12_byte,                  outData_NV12_0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + stride_byte,     outData_NV12_1.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + stride_byte * 2, outData_NV12_2.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + stride_byte * 3, outData_NV12_3.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 8,                   elment_offset, outData_NV12_0.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 8 + stride_dword,     elment_offset, outData_NV12_1.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 8 + stride_dword * 2, elment_offset, outData_NV12_2.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 8 + stride_dword * 3, elment_offset, outData_NV12_3.select<8, 1>(8));
                break;
            case 6:
                write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte,     outData1.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 2, outData2.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 3, outData3.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 4, outData4.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 5, outData5.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_dword + 8,                   elment_offset, outData0.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + stride_dword,     elment_offset, outData1.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + stride_dword * 2, elment_offset, outData2.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + stride_dword * 3, elment_offset, outData3.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + stride_dword * 4, elment_offset, outData4.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + stride_dword * 5, elment_offset, outData5.select<8, 1>(8));

                write(OUTBUF_IDX, linear_offset_NV12_byte,                  outData_NV12_0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + stride_byte,     outData_NV12_1.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + stride_byte * 2, outData_NV12_2.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 8,                   elment_offset, outData_NV12_0.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 8 + stride_dword,     elment_offset, outData_NV12_1.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 8 + stride_dword * 2, elment_offset, outData_NV12_2.select<8, 1>(8));
                break;
            case 4:
                write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte,     outData1.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 2, outData2.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte * 3, outData3.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_dword + 8,                   elment_offset, outData0.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + stride_dword,     elment_offset, outData1.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + stride_dword * 2, elment_offset, outData2.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + stride_dword * 3, elment_offset, outData3.select<8, 1>(8));

                write(OUTBUF_IDX, linear_offset_NV12_byte,              outData_NV12_0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_byte + stride_byte, outData_NV12_1.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 8,               elment_offset, outData_NV12_0.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_NV12_dword + 8 + stride_dword, elment_offset, outData_NV12_1.select<8, 1>(8));
                break;
            case 2:
                write(OUTBUF_IDX, linear_offset_byte,              outData0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_byte + stride_byte, outData1.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_dword + 8,               elment_offset, outData0.select<8, 1>(8));
                write(OUTBUF_IDX, linear_offset_dword + 8 + stride_dword, elment_offset, outData1.select<8, 1>(8));

                write(OUTBUF_IDX, linear_offset_NV12_byte, outData_NV12_0.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword + 8, elment_offset, outData_NV12_0.select<8, 1>(8));
                break;
            default:
                break;
            }
            break;
        case 1:
            if(last_block_size<8){
                for (int i = 0; i < last_block_size; i ++)
                {
                    outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, i) = outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, i+last_block_size);
                    outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, 1, 1>(0, i) = outData_NV12_m.select<SUB_BLOCK_HEIGHT_NV12, 1, 1, 1>(0, i+last_block_size);
                }
            }
            switch(last_block_height)
            {
            case 8:
                write(OUTBUF_IDX, linear_offset_dword,                   elment_offset, outData0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + stride_dword,     elment_offset, outData1.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + stride_dword * 2, elment_offset, outData2.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + stride_dword * 3, elment_offset, outData3.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + stride_dword * 4, elment_offset, outData4.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + stride_dword * 5, elment_offset, outData5.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + stride_dword * 6, elment_offset, outData6.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + stride_dword * 7, elment_offset, outData7.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword,                   elment_offset, outData_NV12_0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_dword + stride_dword,     elment_offset, outData_NV12_1.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_dword + stride_dword * 2, elment_offset, outData_NV12_2.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_dword + stride_dword * 3, elment_offset, outData_NV12_3.select<8, 1>(0));
                break;
            case 6:
                write(OUTBUF_IDX, linear_offset_dword,                   elment_offset, outData0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + stride_dword,     elment_offset, outData1.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + stride_dword * 2, elment_offset, outData2.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + stride_dword * 3, elment_offset, outData3.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + stride_dword * 4, elment_offset, outData4.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + stride_dword * 5, elment_offset, outData5.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword,                   elment_offset, outData_NV12_0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_dword + stride_dword,     elment_offset, outData_NV12_1.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_dword + stride_dword * 2, elment_offset, outData_NV12_2.select<8, 1>(0));
                break;
            case 4:
                write(OUTBUF_IDX, linear_offset_dword,                   elment_offset, outData0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + stride_dword,     elment_offset, outData1.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + stride_dword * 2, elment_offset, outData2.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + stride_dword * 3, elment_offset, outData3.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword,               elment_offset, outData_NV12_0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_NV12_dword + stride_dword, elment_offset, outData_NV12_1.select<8, 1>(0));
                break;
            case 2:
                write(OUTBUF_IDX, linear_offset_dword,               elment_offset, outData0.select<8, 1>(0));
                write(OUTBUF_IDX, linear_offset_dword + stride_dword, elment_offset, outData1.select<8, 1>(0));

                write(OUTBUF_IDX, linear_offset_NV12_dword, elment_offset, outData_NV12_0.select<8, 1>(0));
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
    }
}

extern "C" _GENX_MAIN_  void
SurfaceMirror_2DTo2D_NV12(SurfaceIndex INBUF_IDX, SurfaceIndex OUTBUF_IDX, int width, int height)
{
    //copy Y plane
    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> outData1;
    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> outData2;
    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> outData3;
    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> outData4;
    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> outData5;
    vector<ushort, SUB_BLOCK_PIXEL_WIDTH*4> idx(indexTable);
    int horizOffset = get_thread_origin_x() * BLOCK_PIXEL_WIDTH;
    int vertOffset = get_thread_origin_y() * BLOCK_HEIGHT;

    read_plane(INBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset*4,                             vertOffset, outData1);
    read_plane(INBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset*4 + SUB_BLOCK_PIXEL_WIDTH*4,   vertOffset, outData2);
    read_plane(INBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset*4 + SUB_BLOCK_PIXEL_WIDTH*4*2, vertOffset, outData3);
    read_plane(INBUF_IDX, GENX_SURFACE_Y_PLANE, horizOffset*4 + SUB_BLOCK_PIXEL_WIDTH*4*3, vertOffset, outData4);

    #pragma unroll
    for(int i = 0; i < SUB_BLOCK_HEIGHT; i++)
        outData5.row(i) = outData1.row(i).format<uchar>().iselect(idx).format<uint>();

    #pragma unroll
    for(int i = 0; i < SUB_BLOCK_HEIGHT; i++)
        outData1.row(i) = outData2.row(i).format<uchar>().iselect(idx).format<uint>();

    #pragma unroll
    for(int i = 0; i < SUB_BLOCK_HEIGHT; i++)
        outData2.row(i) = outData3.row(i).format<uchar>().iselect(idx).format<uint>();

    #pragma unroll
    for(int i = 0; i < SUB_BLOCK_HEIGHT; i++)
        outData3.row(i) = outData4.row(i).format<uchar>().iselect(idx).format<uint>();

    write_plane(OUTBUF_IDX, GENX_SURFACE_Y_PLANE, width -  horizOffset*4 - BLOCK_PIXEL_WIDTH,                              vertOffset, outData5);
    write_plane(OUTBUF_IDX, GENX_SURFACE_Y_PLANE, width - (horizOffset*4 + SUB_BLOCK_PIXEL_WIDTH*4) - BLOCK_PIXEL_WIDTH,   vertOffset, outData1);
    write_plane(OUTBUF_IDX, GENX_SURFACE_Y_PLANE, width - (horizOffset*4 + SUB_BLOCK_PIXEL_WIDTH*4*2) - BLOCK_PIXEL_WIDTH, vertOffset, outData2);
    write_plane(OUTBUF_IDX, GENX_SURFACE_Y_PLANE, width - (horizOffset*4 + SUB_BLOCK_PIXEL_WIDTH*4*3) - BLOCK_PIXEL_WIDTH, vertOffset, outData3);

    //copy UV plane
    vector_ref<ushort, SUB_BLOCK_PIXEL_WIDTH*2> idxUV = idx.select<16,1>(16);
    matrix<uint, SUB_BLOCK_HEIGHT_NV12, SUB_BLOCK_PIXEL_WIDTH> outData_NV12_1;
    matrix<uint, SUB_BLOCK_HEIGHT_NV12, SUB_BLOCK_PIXEL_WIDTH> outData_NV12_2;
    matrix<uint, SUB_BLOCK_HEIGHT_NV12, SUB_BLOCK_PIXEL_WIDTH> outData_NV12_3;
    matrix<uint, SUB_BLOCK_HEIGHT_NV12, SUB_BLOCK_PIXEL_WIDTH> outData_NV12_4;
    matrix<uint, SUB_BLOCK_HEIGHT_NV12, SUB_BLOCK_PIXEL_WIDTH> outData_NV12_5;

    read_plane(INBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset*4,                             vertOffset/2, outData_NV12_1);
    read_plane(INBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset*4 + SUB_BLOCK_PIXEL_WIDTH*4,   vertOffset/2, outData_NV12_2);
    read_plane(INBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset*4 + SUB_BLOCK_PIXEL_WIDTH*4*2, vertOffset/2, outData_NV12_3);
    read_plane(INBUF_IDX, GENX_SURFACE_UV_PLANE, horizOffset*4 + SUB_BLOCK_PIXEL_WIDTH*4*3, vertOffset/2, outData_NV12_4);

    #pragma unroll
    for(int i = 0; i < SUB_BLOCK_HEIGHT_NV12; i++)
        outData_NV12_5.row(i) = outData_NV12_1.row(i).format<ushort>().iselect(idxUV).format<uint>();

    #pragma unroll
    for(int i = 0; i < SUB_BLOCK_HEIGHT_NV12; i++)
        outData_NV12_1.row(i) = outData_NV12_2.row(i).format<ushort>().iselect(idxUV).format<uint>();

    #pragma unroll
    for(int i = 0; i < SUB_BLOCK_HEIGHT_NV12; i++)
        outData_NV12_2.row(i) = outData_NV12_3.row(i).format<ushort>().iselect(idxUV).format<uint>();

    #pragma unroll
    for(int i = 0; i < SUB_BLOCK_HEIGHT_NV12; i++)
        outData_NV12_3.row(i) = outData_NV12_4.row(i).format<ushort>().iselect(idxUV).format<uint>();

    write_plane(OUTBUF_IDX, GENX_SURFACE_UV_PLANE, width -  horizOffset*4 - BLOCK_PIXEL_WIDTH,                              vertOffset/2, outData_NV12_5);
    write_plane(OUTBUF_IDX, GENX_SURFACE_UV_PLANE, width - (horizOffset*4 + SUB_BLOCK_PIXEL_WIDTH*4) - BLOCK_PIXEL_WIDTH,   vertOffset/2, outData_NV12_1);
    write_plane(OUTBUF_IDX, GENX_SURFACE_UV_PLANE, width - (horizOffset*4 + SUB_BLOCK_PIXEL_WIDTH*4*2) - BLOCK_PIXEL_WIDTH, vertOffset/2, outData_NV12_2);
    write_plane(OUTBUF_IDX, GENX_SURFACE_UV_PLANE, width - (horizOffset*4 + SUB_BLOCK_PIXEL_WIDTH*4*3) - BLOCK_PIXEL_WIDTH, vertOffset/2, outData_NV12_3);
}
//*/

inline _GENX_  void
swapchannels(vector_ref<uchar,BLOCK_PIXEL_WIDTH*4> in,vector_ref<uchar,BLOCK_PIXEL_WIDTH*4> temp,int pixel_size)
{
    temp=in;
    if(pixel_size == 4)
    {
        in.select<BLOCK_PIXEL_WIDTH,4>(0)=temp.select<BLOCK_PIXEL_WIDTH,4>(2);
        in.select<BLOCK_PIXEL_WIDTH,4>(2)=temp.select<BLOCK_PIXEL_WIDTH,4>(0);
        in.select<BLOCK_PIXEL_WIDTH*2,2>(1)=temp.select<BLOCK_PIXEL_WIDTH*2,2>(1);
    }
    else
    {
        vector_ref<ushort,BLOCK_PIXEL_WIDTH*2> in16 = in.format<ushort>();
        vector_ref<ushort,BLOCK_PIXEL_WIDTH*2> tmp16 = temp.format<ushort>();
        in16.select<BLOCK_PIXEL_WIDTH/2,4>(0)=tmp16.select<BLOCK_PIXEL_WIDTH/2,4>(2);
        in16.select<BLOCK_PIXEL_WIDTH/2,4>(2)=tmp16.select<BLOCK_PIXEL_WIDTH/2,4>(0);
        in16.select<BLOCK_PIXEL_WIDTH,2>(1)=tmp16.select<BLOCK_PIXEL_WIDTH,2>(1);
    }
}

inline _GENX_  void
SurfaceCopySwap_2DTo2D_32x8(SurfaceIndex INBUF_IDX, SurfaceIndex OUTBUF_IDX, int horizOffset, int vertOffset, int pixel_size)
{
    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> outData1;
    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> outData2;
    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> outData3;
    matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> outData4;
    vector<uchar, BLOCK_PIXEL_WIDTH*4> temp;
    vector_ref<uchar, BLOCK_PIXEL_WIDTH*4> in1 = outData1.select<SUB_BLOCK_HEIGHT/2,1,SUB_BLOCK_PIXEL_WIDTH,1>(0,0).format<uchar>();
    vector_ref<uchar, BLOCK_PIXEL_WIDTH*4> in2 = outData1.select<SUB_BLOCK_HEIGHT/2,1,SUB_BLOCK_PIXEL_WIDTH,1>(SUB_BLOCK_HEIGHT/2,0).format<uchar>();
    vector_ref<uchar, BLOCK_PIXEL_WIDTH*4> in3 = outData2.select<SUB_BLOCK_HEIGHT/2,1,SUB_BLOCK_PIXEL_WIDTH,1>(0,0).format<uchar>();
    vector_ref<uchar, BLOCK_PIXEL_WIDTH*4> in4 = outData2.select<SUB_BLOCK_HEIGHT/2,1,SUB_BLOCK_PIXEL_WIDTH,1>(SUB_BLOCK_HEIGHT/2,0).format<uchar>();
    vector_ref<uchar, BLOCK_PIXEL_WIDTH*4> in5 = outData3.select<SUB_BLOCK_HEIGHT/2,1,SUB_BLOCK_PIXEL_WIDTH,1>(0,0).format<uchar>();
    vector_ref<uchar, BLOCK_PIXEL_WIDTH*4> in6 = outData3.select<SUB_BLOCK_HEIGHT/2,1,SUB_BLOCK_PIXEL_WIDTH,1>(SUB_BLOCK_HEIGHT/2,0).format<uchar>();
    vector_ref<uchar, BLOCK_PIXEL_WIDTH*4> in7 = outData4.select<SUB_BLOCK_HEIGHT/2,1,SUB_BLOCK_PIXEL_WIDTH,1>(0,0).format<uchar>();
    vector_ref<uchar, BLOCK_PIXEL_WIDTH*4> in8 = outData4.select<SUB_BLOCK_HEIGHT/2,1,SUB_BLOCK_PIXEL_WIDTH,1>(SUB_BLOCK_HEIGHT/2,0).format<uchar>();

    read(INBUF_IDX, horizOffset*4, vertOffset, outData1);
    read(INBUF_IDX, horizOffset*4 + SUB_BLOCK_PIXEL_WIDTH*4,   vertOffset, outData2);
    read(INBUF_IDX, horizOffset*4 + SUB_BLOCK_PIXEL_WIDTH*4*2, vertOffset, outData3);
    read(INBUF_IDX, horizOffset*4 + SUB_BLOCK_PIXEL_WIDTH*4*3, vertOffset, outData4);

    swapchannels(in1,temp,pixel_size);
    swapchannels(in2,temp,pixel_size);
    swapchannels(in3,temp,pixel_size);
    swapchannels(in4,temp,pixel_size);
    swapchannels(in5,temp,pixel_size);
    swapchannels(in6,temp,pixel_size);
    swapchannels(in7,temp,pixel_size);
    swapchannels(in8,temp,pixel_size);

    write(OUTBUF_IDX, horizOffset*4, vertOffset, outData1);
    write(OUTBUF_IDX, horizOffset*4 + SUB_BLOCK_PIXEL_WIDTH*4,   vertOffset, outData2);
    write(OUTBUF_IDX, horizOffset*4 + SUB_BLOCK_PIXEL_WIDTH*4*2, vertOffset, outData3);
    write(OUTBUF_IDX, horizOffset*4 + SUB_BLOCK_PIXEL_WIDTH*4*3, vertOffset, outData4);
}

extern "C" _GENX_MAIN_  void
SurfaceCopySwap_2DTo2D_32x32(SurfaceIndex INBUF_IDX, SurfaceIndex OUTBUF_IDX, int threadHeight, int pixel_size)
{
    int horizOffset = get_thread_origin_x() * BLOCK_PIXEL_WIDTH;
    int vertOffset = get_thread_origin_y() * BLOCK_HEIGHT;

    SurfaceCopySwap_2DTo2D_32x8(INBUF_IDX, OUTBUF_IDX, horizOffset, vertOffset,pixel_size);
    SurfaceCopySwap_2DTo2D_32x8(INBUF_IDX, OUTBUF_IDX, horizOffset, vertOffset + SUB_BLOCK_HEIGHT * threadHeight, pixel_size);
    SurfaceCopySwap_2DTo2D_32x8(INBUF_IDX, OUTBUF_IDX, horizOffset, vertOffset + SUB_BLOCK_HEIGHT * threadHeight * 2, pixel_size);
    SurfaceCopySwap_2DTo2D_32x8(INBUF_IDX, OUTBUF_IDX, horizOffset, vertOffset + SUB_BLOCK_HEIGHT * threadHeight * 3, pixel_size);
}

inline _GENX_  void
surfaceCopy_writeswap_32x8(SurfaceIndex INBUF_IDX, SurfaceIndex OUTBUF_IDX, int width_dword, int height, int srcBuffer_ShiftLeftOffsetInBytes, int horizOffset, int vertOffset, int pixel_size, int dst2D_start_x, int dst2D_start_y)
{
    int srcBuffer_linear_offset_byte = (( vertOffset * width_dword + horizOffset ) << 2) +  srcBuffer_ShiftLeftOffsetInBytes;
    int width_byte = width_dword << 2;
    int dst2D_horizOffset_byte = dst2D_start_x + (horizOffset << 2);
    int dst2D_vertOffset_row = dst2D_start_y + vertOffset;
    int sub_block_width_byte = SUB_BLOCK_PIXEL_WIDTH << 2;
    if (srcBuffer_linear_offset_byte < width_byte * height + srcBuffer_ShiftLeftOffsetInBytes)
    {
        matrix<uint, BLOCK_HEIGHT, BLOCK_PIXEL_WIDTH> inData_m;
        vector<uchar, BLOCK_PIXEL_WIDTH*4> temp;
        vector_ref<uint, BLOCK_PIXEL_WIDTH> inData0(inData_m.row(0));
        vector_ref<uint, BLOCK_PIXEL_WIDTH> inData1(inData_m.row(1));
        vector_ref<uint, BLOCK_PIXEL_WIDTH> inData2(inData_m.row(2));
        vector_ref<uint, BLOCK_PIXEL_WIDTH> inData3(inData_m.row(3));
        vector_ref<uint, BLOCK_PIXEL_WIDTH> inData4(inData_m.row(4));
        vector_ref<uint, BLOCK_PIXEL_WIDTH> inData5(inData_m.row(5));
        vector_ref<uint, BLOCK_PIXEL_WIDTH> inData6(inData_m.row(6));
        vector_ref<uint, BLOCK_PIXEL_WIDTH> inData7(inData_m.row(7));

        vector_ref<uchar, BLOCK_PIXEL_WIDTH*4> inDataSwap0 = inData0.format<uchar>();
        vector_ref<uchar, BLOCK_PIXEL_WIDTH*4> inDataSwap1 = inData1.format<uchar>();
        vector_ref<uchar, BLOCK_PIXEL_WIDTH*4> inDataSwap2 = inData2.format<uchar>();
        vector_ref<uchar, BLOCK_PIXEL_WIDTH*4> inDataSwap3 = inData3.format<uchar>();
        vector_ref<uchar, BLOCK_PIXEL_WIDTH*4> inDataSwap4 = inData4.format<uchar>();
        vector_ref<uchar, BLOCK_PIXEL_WIDTH*4> inDataSwap5 = inData5.format<uchar>();
        vector_ref<uchar, BLOCK_PIXEL_WIDTH*4> inDataSwap6 = inData6.format<uchar>();
        vector_ref<uchar, BLOCK_PIXEL_WIDTH*4> inDataSwap7 = inData7.format<uchar>();

        matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> outData0;
        matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> outData1;
        matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> outData2;
        matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> outData3;

        read(INBUF_IDX, srcBuffer_linear_offset_byte,                  inData0);
        read(INBUF_IDX, srcBuffer_linear_offset_byte + width_byte,     inData1);
        read(INBUF_IDX, srcBuffer_linear_offset_byte + width_byte * 2, inData2);
        read(INBUF_IDX, srcBuffer_linear_offset_byte + width_byte * 3, inData3);
        read(INBUF_IDX, srcBuffer_linear_offset_byte + width_byte * 4, inData4);
        read(INBUF_IDX, srcBuffer_linear_offset_byte + width_byte * 5, inData5);
        read(INBUF_IDX, srcBuffer_linear_offset_byte + width_byte * 6, inData6);
        read(INBUF_IDX, srcBuffer_linear_offset_byte + width_byte * 7, inData7);

        swapchannels(inDataSwap0,temp,pixel_size);
        swapchannels(inDataSwap1,temp,pixel_size);
        swapchannels(inDataSwap2,temp,pixel_size);
        swapchannels(inDataSwap3,temp,pixel_size);
        swapchannels(inDataSwap4,temp,pixel_size);
        swapchannels(inDataSwap5,temp,pixel_size);
        swapchannels(inDataSwap6,temp,pixel_size);
        swapchannels(inDataSwap7,temp,pixel_size);

        outData0 = inData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 0);
        outData1 = inData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 8);
        outData2 = inData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 16);
        outData3 = inData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 24);

        write(OUTBUF_IDX, dst2D_horizOffset_byte,                          dst2D_vertOffset_row, outData0);
        write(OUTBUF_IDX, dst2D_horizOffset_byte + sub_block_width_byte,   dst2D_vertOffset_row, outData1);
        write(OUTBUF_IDX, dst2D_horizOffset_byte + sub_block_width_byte*2, dst2D_vertOffset_row, outData2);
        write(OUTBUF_IDX, dst2D_horizOffset_byte + sub_block_width_byte*3, dst2D_vertOffset_row, outData3);
    }
}

extern "C" _GENX_MAIN_  void
surfaceCopy_writeswap_32x32(SurfaceIndex INBUF_IDX, SurfaceIndex OUTBUF_IDX, int width_dword, int height, int srcBuffer_ShiftLeftOffsetInBytes, int threadHeight, int pixel_size, int dst2D_start_x, int dst2D_start_y)
{
    int horizOffset = get_thread_origin_x() * BLOCK_PIXEL_WIDTH;
    int vertOffset = get_thread_origin_y() * BLOCK_HEIGHT;

    surfaceCopy_writeswap_32x8(INBUF_IDX, OUTBUF_IDX, width_dword, height, srcBuffer_ShiftLeftOffsetInBytes, horizOffset, vertOffset, pixel_size, dst2D_start_x, dst2D_start_y);
    surfaceCopy_writeswap_32x8(INBUF_IDX, OUTBUF_IDX, width_dword, height, srcBuffer_ShiftLeftOffsetInBytes, horizOffset, vertOffset + SUB_BLOCK_HEIGHT * threadHeight, pixel_size, dst2D_start_x, dst2D_start_y);
    surfaceCopy_writeswap_32x8(INBUF_IDX, OUTBUF_IDX, width_dword, height, srcBuffer_ShiftLeftOffsetInBytes, horizOffset, vertOffset + SUB_BLOCK_HEIGHT * threadHeight * 2, pixel_size, dst2D_start_x, dst2D_start_y);
    surfaceCopy_writeswap_32x8(INBUF_IDX, OUTBUF_IDX, width_dword, height, srcBuffer_ShiftLeftOffsetInBytes, horizOffset, vertOffset + SUB_BLOCK_HEIGHT * threadHeight * 3, pixel_size, dst2D_start_x, dst2D_start_y);
}

//BufferUP-->Surface2D
inline _GENX_  void
surfaceCopy_write_32x8(SurfaceIndex INBUF_IDX, SurfaceIndex OUTBUF_IDX, int width_dword, int height, int srcBuffer_ShiftLeftOffsetInBytes, int horizOffset, int vertOffset, int dst2D_start_x, int dst2D_start_y)
{
    int srcBuffer_linear_offset_byte = (( vertOffset * width_dword + horizOffset ) << 2) +  srcBuffer_ShiftLeftOffsetInBytes;
    int width_byte = width_dword << 2;
    int dst2D_horizOffset_byte = dst2D_start_x + (horizOffset << 2);
    int dst2D_vertOffset_row = dst2D_start_y + vertOffset;
    int sub_block_width_byte = SUB_BLOCK_PIXEL_WIDTH << 2;
    if (srcBuffer_linear_offset_byte < width_byte * height + srcBuffer_ShiftLeftOffsetInBytes)
    {
        matrix<uint, BLOCK_HEIGHT, BLOCK_PIXEL_WIDTH> inData_m;
        vector_ref<uint, BLOCK_PIXEL_WIDTH> inData0(inData_m.row(0));
        vector_ref<uint, BLOCK_PIXEL_WIDTH> inData1(inData_m.row(1));
        vector_ref<uint, BLOCK_PIXEL_WIDTH> inData2(inData_m.row(2));
        vector_ref<uint, BLOCK_PIXEL_WIDTH> inData3(inData_m.row(3));
        vector_ref<uint, BLOCK_PIXEL_WIDTH> inData4(inData_m.row(4));
        vector_ref<uint, BLOCK_PIXEL_WIDTH> inData5(inData_m.row(5));
        vector_ref<uint, BLOCK_PIXEL_WIDTH> inData6(inData_m.row(6));
        vector_ref<uint, BLOCK_PIXEL_WIDTH> inData7(inData_m.row(7));

        matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> outData0;
        matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> outData1;
        matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> outData2;
        matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> outData3;

        read(INBUF_IDX, srcBuffer_linear_offset_byte,                  inData0);
        read(INBUF_IDX, srcBuffer_linear_offset_byte + width_byte,     inData1);
        read(INBUF_IDX, srcBuffer_linear_offset_byte + width_byte * 2, inData2);
        read(INBUF_IDX, srcBuffer_linear_offset_byte + width_byte * 3, inData3);
        read(INBUF_IDX, srcBuffer_linear_offset_byte + width_byte * 4, inData4);
        read(INBUF_IDX, srcBuffer_linear_offset_byte + width_byte * 5, inData5);
        read(INBUF_IDX, srcBuffer_linear_offset_byte + width_byte * 6, inData6);
        read(INBUF_IDX, srcBuffer_linear_offset_byte + width_byte * 7, inData7);

        outData0 = inData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 0);
        outData1 = inData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 8);
        outData2 = inData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 16);
        outData3 = inData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 24);

        write(OUTBUF_IDX, dst2D_horizOffset_byte,                          dst2D_vertOffset_row, outData0);
        write(OUTBUF_IDX, dst2D_horizOffset_byte + sub_block_width_byte,   dst2D_vertOffset_row, outData1);
        write(OUTBUF_IDX, dst2D_horizOffset_byte + sub_block_width_byte*2, dst2D_vertOffset_row, outData2);
        write(OUTBUF_IDX, dst2D_horizOffset_byte + sub_block_width_byte*3, dst2D_vertOffset_row, outData3);
    }
}

extern "C" _GENX_MAIN_  void
surfaceCopy_write_32x32(SurfaceIndex INBUF_IDX, SurfaceIndex OUTBUF_IDX, int width_dword, int height, int srcBuffer_ShiftLeftOffsetInBytes, int threadHeight, int dst2D_start_x, int dst2D_start_y)
{
    int horizOffset = get_thread_origin_x() * BLOCK_PIXEL_WIDTH;
    int vertOffset = get_thread_origin_y() * BLOCK_HEIGHT;

    surfaceCopy_write_32x8(INBUF_IDX, OUTBUF_IDX, width_dword, height, srcBuffer_ShiftLeftOffsetInBytes, horizOffset, vertOffset, dst2D_start_x, dst2D_start_y);
    surfaceCopy_write_32x8(INBUF_IDX, OUTBUF_IDX, width_dword, height, srcBuffer_ShiftLeftOffsetInBytes, horizOffset, vertOffset + SUB_BLOCK_HEIGHT * threadHeight, dst2D_start_x, dst2D_start_y);
    surfaceCopy_write_32x8(INBUF_IDX, OUTBUF_IDX, width_dword, height, srcBuffer_ShiftLeftOffsetInBytes, horizOffset, vertOffset + SUB_BLOCK_HEIGHT * threadHeight * 2, dst2D_start_x, dst2D_start_y);
    surfaceCopy_write_32x8(INBUF_IDX, OUTBUF_IDX, width_dword, height, srcBuffer_ShiftLeftOffsetInBytes, horizOffset, vertOffset + SUB_BLOCK_HEIGHT * threadHeight * 3, dst2D_start_x, dst2D_start_y);
}

/*
Height must be 8-row aligned
Width_byte  must be 128 aligned (Width_byte = width_dword *4)
*/
/*inline _GENX_  void
surfaceCopy_read_aligned_32x8(SurfaceIndex INBUF_IDX, SurfaceIndex OUTBUF_IDX, int width_dword, int height, int ShiftLeftOffsetInBytes, int horizOffset, int vertOffset, int src2D_start_x, int src2D_start_y)
{
    int width_byte = width_dword << 2;
    int horizOffset_byte = src2D_start_x + (horizOffset << 2);
    int vertOffset_row = src2D_start_y + vertOffset;
    int sub_block_width_byte = SUB_BLOCK_PIXEL_WIDTH << 2;

    uint linear_offset_dword = vertOffset * width_dword + horizOffset + (ShiftLeftOffsetInBytes/4);
    uint linear_offset_byte = (linear_offset_dword << 2) ;
    if (linear_offset_byte < width_byte * height + ShiftLeftOffsetInBytes)
    {
        matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> inData0;
        matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> inData1;
        matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> inData2;
        matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> inData3;

        matrix<uint, BLOCK_HEIGHT, BLOCK_PIXEL_WIDTH> outData_m;
        vector_ref<uint, BLOCK_PIXEL_WIDTH> outData0(outData_m.row(0));
        vector_ref<uint, BLOCK_PIXEL_WIDTH> outData1(outData_m.row(1));
        vector_ref<uint, BLOCK_PIXEL_WIDTH> outData2(outData_m.row(2));
        vector_ref<uint, BLOCK_PIXEL_WIDTH> outData3(outData_m.row(3));
        vector_ref<uint, BLOCK_PIXEL_WIDTH> outData4(outData_m.row(4));
        vector_ref<uint, BLOCK_PIXEL_WIDTH> outData5(outData_m.row(5));
        vector_ref<uint, BLOCK_PIXEL_WIDTH> outData6(outData_m.row(6));
        vector_ref<uint, BLOCK_PIXEL_WIDTH> outData7(outData_m.row(7));

        read(INBUF_IDX, horizOffset_byte,                          vertOffset_row, inData0);
        read(INBUF_IDX, horizOffset_byte + sub_block_width_byte,   vertOffset_row, inData1);
        read(INBUF_IDX, horizOffset_byte + sub_block_width_byte*2, vertOffset_row, inData2);
        read(INBUF_IDX, horizOffset_byte + sub_block_width_byte*3, vertOffset_row, inData3);

        outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 0)  = inData0;
        outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 8)  = inData1;
        outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 16) = inData2;
        outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 24) = inData3;

        write(OUTBUF_IDX, linear_offset_byte,                  outData0);
        write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1);
        write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2);
        write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3);
        write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4);
        write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5);
        write(OUTBUF_IDX, linear_offset_byte + width_byte * 6, outData6);
        write(OUTBUF_IDX, linear_offset_byte + width_byte * 7, outData7);
    }
}

extern "C" _GENX_MAIN_  void
surfaceCopy_read_aligned_32x32(SurfaceIndex INBUF_IDX, SurfaceIndex OUTBUF_IDX, int width_dword, int height, int ShiftLeftOffsetInBytes, int threadHeight, int width_in_dword_no_padding, int height_no_padding, int src2D_start_x, int src2D_start_y)
{
    int horizOffset = get_thread_origin_x() * BLOCK_PIXEL_WIDTH;
    int vertOffset = get_thread_origin_y() * BLOCK_HEIGHT;
#pragma unroll
    for (int i = 0; i< 4; i ++)
    {
        surfaceCopy_read_aligned_32x8(INBUF_IDX, OUTBUF_IDX, width_dword, height_no_padding, ShiftLeftOffsetInBytes, horizOffset, vertOffset + SUB_BLOCK_HEIGHT * threadHeight * i, src2D_start_x, src2D_start_y);
    }
}
*/
inline _GENX_  void
surfaceCopy_readswap_32x8(SurfaceIndex INBUF_IDX, SurfaceIndex OUTBUF_IDX, int width_dword, int height_stride_in_rows, int ShiftLeftOffsetInBytes, int horizOffset, int vertOffset, int width_in_dword_no_padding, int height_no_padding, int pixel_size, int src2D_start_x, int src2D_start_y)
{
    int copy_width_dword = MIN(width_in_dword_no_padding, width_dword);
    int copy_height = MIN(height_no_padding, height_stride_in_rows);
    int width_byte = width_dword << 2;
    int horizOffset_byte = src2D_start_x + (horizOffset << 2);
    int vertOffset_row = src2D_start_y + vertOffset;
    int sub_block_width_byte = SUB_BLOCK_PIXEL_WIDTH << 2;

    uint linear_offset_dword = vertOffset * width_dword + horizOffset + (ShiftLeftOffsetInBytes/4);
    uint linear_offset_byte = (linear_offset_dword << 2) ;
    if (linear_offset_byte < width_byte * copy_height + ShiftLeftOffsetInBytes)
    {

        matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> inData0;
        matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> inData1;
        matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> inData2;
        matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> inData3;

        matrix<uint, BLOCK_HEIGHT, BLOCK_PIXEL_WIDTH> outData_m;
        vector<uchar,BLOCK_PIXEL_WIDTH*4> tempSwappedData;
        vector_ref<uint, BLOCK_PIXEL_WIDTH> outData0 = outData_m.row(0);
        vector_ref<uint, BLOCK_PIXEL_WIDTH> outData1 = outData_m.row(1);
        vector_ref<uint, BLOCK_PIXEL_WIDTH> outData2 = outData_m.row(2);
        vector_ref<uint, BLOCK_PIXEL_WIDTH> outData3 = outData_m.row(3);
        vector_ref<uint, BLOCK_PIXEL_WIDTH> outData4 = outData_m.row(4);
        vector_ref<uint, BLOCK_PIXEL_WIDTH> outData5 = outData_m.row(5);
        vector_ref<uint, BLOCK_PIXEL_WIDTH> outData6 = outData_m.row(6);
        vector_ref<uint, BLOCK_PIXEL_WIDTH> outData7 = outData_m.row(7);
        vector_ref<uchar,BLOCK_PIXEL_WIDTH*4> outSwappedData0 = outData0.format<uchar>();
        vector_ref<uchar,BLOCK_PIXEL_WIDTH*4> outSwappedData1 = outData1.format<uchar>();
        vector_ref<uchar,BLOCK_PIXEL_WIDTH*4> outSwappedData2 = outData2.format<uchar>();
        vector_ref<uchar,BLOCK_PIXEL_WIDTH*4> outSwappedData3 = outData3.format<uchar>();
        vector_ref<uchar,BLOCK_PIXEL_WIDTH*4> outSwappedData4 = outData4.format<uchar>();
        vector_ref<uchar,BLOCK_PIXEL_WIDTH*4> outSwappedData5 = outData5.format<uchar>();
        vector_ref<uchar,BLOCK_PIXEL_WIDTH*4> outSwappedData6 = outData6.format<uchar>();
        vector_ref<uchar,BLOCK_PIXEL_WIDTH*4> outSwappedData7 = outData7.format<uchar>();
        uint last_block_height = 8;
        if(copy_height - vertOffset < BLOCK_HEIGHT)
        {
            last_block_height = copy_height - vertOffset;
        }

        if (copy_width_dword - horizOffset >= BLOCK_PIXEL_WIDTH)    // for aligned block
        {
            read(INBUF_IDX, horizOffset_byte,                          vertOffset_row, inData0);
            read(INBUF_IDX, horizOffset_byte + sub_block_width_byte,   vertOffset_row, inData1);
            read(INBUF_IDX, horizOffset_byte + sub_block_width_byte*2, vertOffset_row, inData2);
            read(INBUF_IDX, horizOffset_byte + sub_block_width_byte*3, vertOffset_row, inData3);

            outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 0)  = inData0;
            outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 8)  = inData1;
            outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 16) = inData2;
            outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 24) = inData3;

            swapchannels(outSwappedData0,tempSwappedData, pixel_size);
            swapchannels(outSwappedData1,tempSwappedData, pixel_size);
            swapchannels(outSwappedData2,tempSwappedData, pixel_size);
            swapchannels(outSwappedData3,tempSwappedData, pixel_size);
            swapchannels(outSwappedData4,tempSwappedData, pixel_size);
            swapchannels(outSwappedData5,tempSwappedData, pixel_size);
            swapchannels(outSwappedData6,tempSwappedData, pixel_size);
            swapchannels(outSwappedData7,tempSwappedData, pixel_size);
            switch(last_block_height)
            {
            case 8:
                write(OUTBUF_IDX, linear_offset_byte,                  outData0);
                write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 6, outData6);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 7, outData7);
                break;

            case 7:
                write(OUTBUF_IDX, linear_offset_byte,                  outData0);
                write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 6, outData6);
                break;

            case 6:
                write(OUTBUF_IDX, linear_offset_byte,                  outData0);
                write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5);
                break;

            case 5:
                write(OUTBUF_IDX, linear_offset_byte,                  outData0);
                write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4);
                break;

            default:
                break;
            }
            switch(last_block_height)
            {
            case 4:
                write(OUTBUF_IDX, linear_offset_byte,                  outData0);
                write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3);
                break;

            case 3:
                write(OUTBUF_IDX, linear_offset_byte,                  outData0);
                write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2);
                break;

            case 2:
                write(OUTBUF_IDX, linear_offset_byte,              outData0);
                write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1);
                break;

            case 1:
                write(OUTBUF_IDX, linear_offset_byte, outData0);
                break;

            default:
                break;
            }
        }
        else    // for the unaligned most right blocks
        {
            uint block_width = copy_width_dword - horizOffset;
            uint last_block_size = block_width;
            uint read_times = 1;
            read(INBUF_IDX, horizOffset_byte, vertOffset_row, inData0);
            if (block_width > 8)
            {
                read(INBUF_IDX, horizOffset_byte + sub_block_width_byte, vertOffset_row, inData1);
                read_times = 2;
                last_block_size = last_block_size - 8;
                if (block_width > 16)
                {
                    read(INBUF_IDX, horizOffset_byte + sub_block_width_byte*2, vertOffset_row, inData2);
                    read_times = 3;
                    last_block_size = last_block_size - 8;
                    if (block_width > 24)
                    {
                        read(INBUF_IDX, horizOffset_byte + sub_block_width_byte*3, vertOffset_row, inData3);
                        read_times = 4;
                        last_block_size = last_block_size - 8;
                    }
                }
            }

            vector<uint, SUB_BLOCK_PIXEL_WIDTH> elment_offset(0);

            for (uint i=0; i < last_block_size; i++)
            {
                elment_offset(i) = i;
            }

            switch (read_times)
            {
            case 4:
                outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 0)  = inData0;
                outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 8)  = inData1;
                outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 16) = inData2;
                outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 24) = inData3;

                swapchannels(outSwappedData0,tempSwappedData, pixel_size);
                swapchannels(outSwappedData1,tempSwappedData, pixel_size);
                swapchannels(outSwappedData2,tempSwappedData, pixel_size);
                swapchannels(outSwappedData3,tempSwappedData, pixel_size);
                swapchannels(outSwappedData4,tempSwappedData, pixel_size);
                swapchannels(outSwappedData5,tempSwappedData, pixel_size);
                swapchannels(outSwappedData6,tempSwappedData, pixel_size);
                swapchannels(outSwappedData7,tempSwappedData, pixel_size);
                //Padding unused by the first one pixel in the last sub block
                for (int i = last_block_size; i < SUB_BLOCK_PIXEL_WIDTH; i ++)
                {
                    outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 24 + i) = outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 24);
                }
                switch(last_block_height)
                {
                case 8:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 6, outData6.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 7, outData7.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_byte + 64,                  outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte,     outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 2, outData2.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 3, outData3.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 4, outData4.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 5, outData5.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 6, outData6.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 7, outData7.select<8, 1>(16));

                    write(OUTBUF_IDX, linear_offset_dword + 24,                   elment_offset, outData0.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword,     elment_offset, outData1.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 2, elment_offset, outData2.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 3, elment_offset, outData3.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 4, elment_offset, outData4.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 5, elment_offset, outData5.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 6, elment_offset, outData6.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 7, elment_offset, outData7.select<8, 1>(24));
                    break;

                case 7:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 6, outData6.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_byte + 64,                  outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte,     outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 2, outData2.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 3, outData3.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 4, outData4.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 5, outData5.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 6, outData6.select<8, 1>(16));

                    write(OUTBUF_IDX, linear_offset_dword + 24,                   elment_offset, outData0.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword,     elment_offset, outData1.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 2, elment_offset, outData2.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 3, elment_offset, outData3.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 4, elment_offset, outData4.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 5, elment_offset, outData5.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 6, elment_offset, outData6.select<8, 1>(24));
                    break;

                case 6:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_byte + 64,                  outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte,     outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 2, outData2.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 3, outData3.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 4, outData4.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 5, outData5.select<8, 1>(16));

                    write(OUTBUF_IDX, linear_offset_dword + 24,                   elment_offset, outData0.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword,     elment_offset, outData1.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 2, elment_offset, outData2.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 3, elment_offset, outData3.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 4, elment_offset, outData4.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 5, elment_offset, outData5.select<8, 1>(24));
                    break;

                case 5:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_byte + 64,                  outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte,     outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 2, outData2.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 3, outData3.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 4, outData4.select<8, 1>(16));

                    write(OUTBUF_IDX, linear_offset_dword + 24,                   elment_offset, outData0.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword,     elment_offset, outData1.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 2, elment_offset, outData2.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 3, elment_offset, outData3.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 4, elment_offset, outData4.select<8, 1>(24));
                    break;

                default:
                    break;
                }
                switch(last_block_height)
                {
                case 4:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_byte + 64,                  outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte,     outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 2, outData2.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 3, outData3.select<8, 1>(16));

                    write(OUTBUF_IDX, linear_offset_dword + 24,                   elment_offset, outData0.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword,     elment_offset, outData1.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 2, elment_offset, outData2.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 3, elment_offset, outData3.select<8, 1>(24));
                    break;

                case 3:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_byte + 64,                  outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte,     outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 2, outData2.select<8, 1>(16));

                    write(OUTBUF_IDX, linear_offset_dword + 24,                   elment_offset, outData0.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword,     elment_offset, outData1.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 2, elment_offset, outData2.select<8, 1>(24));
                    break;

                case 2:
                    write(OUTBUF_IDX, linear_offset_byte,              outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_byte + 64,              outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte, outData1.select<8, 1>(16));

                    write(OUTBUF_IDX, linear_offset_dword + 24,               elment_offset, outData0.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword, elment_offset, outData1.select<8, 1>(24));
                    break;

                case 1:
                    write(OUTBUF_IDX, linear_offset_byte, outData0.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_byte + 64, outData0.select<8, 1>(16));

                    write(OUTBUF_IDX, linear_offset_dword + 24, elment_offset, outData0.select<8, 1>(24));
                    break;

                default:
                    break;
                }
                break;
            case 3:
                outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 0) = inData0;
                outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 8) = inData1;
                outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 16) = inData2;

                //Padding unused by the first one pixel in the last sub block
                for (int i = last_block_size; i < SUB_BLOCK_PIXEL_WIDTH; i ++)
                {
                    outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 16 + i) = outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 16);
                }

                swapchannels(outSwappedData0,tempSwappedData, pixel_size);
                swapchannels(outSwappedData1,tempSwappedData, pixel_size);
                swapchannels(outSwappedData2,tempSwappedData, pixel_size);
                swapchannels(outSwappedData3,tempSwappedData, pixel_size);
                swapchannels(outSwappedData4,tempSwappedData, pixel_size);
                swapchannels(outSwappedData5,tempSwappedData, pixel_size);
                swapchannels(outSwappedData6,tempSwappedData, pixel_size);
                swapchannels(outSwappedData7,tempSwappedData, pixel_size);
                switch(last_block_height)
                {
                case 8:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 6, outData6.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 7, outData7.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 16,                   elment_offset, outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword,     elment_offset, outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 2, elment_offset, outData2.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 3, elment_offset, outData3.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 4, elment_offset, outData4.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 5, elment_offset, outData5.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 6, elment_offset, outData6.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 7, elment_offset, outData7.select<8, 1>(16));
                    break;
                case 7:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 6, outData6.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 16,                   elment_offset, outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword,     elment_offset, outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 2, elment_offset, outData2.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 3, elment_offset, outData3.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 4, elment_offset, outData4.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 5, elment_offset, outData5.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 6, elment_offset, outData6.select<8, 1>(16));
                    break;
                case 6:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 16,                   elment_offset, outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword,     elment_offset, outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 2, elment_offset, outData2.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 3, elment_offset, outData3.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 4, elment_offset, outData4.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 5, elment_offset, outData5.select<8, 1>(16));
                    break;
                case 5:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 16,                   elment_offset, outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword,     elment_offset, outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 2, elment_offset, outData2.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 3, elment_offset, outData3.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 4, elment_offset, outData4.select<8, 1>(16));
                    break;
                default:
                    break;
                }
                switch(last_block_height)
                {
                case 4:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 16,                   elment_offset, outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword,     elment_offset, outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 2, elment_offset, outData2.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 3, elment_offset, outData3.select<8, 1>(16));
                    break;
                case 3:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 16,                   elment_offset, outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword,     elment_offset, outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 2, elment_offset, outData2.select<8, 1>(16));
                    break;
                case 2:
                    write(OUTBUF_IDX, linear_offset_byte,              outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 16,               elment_offset, outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword, elment_offset, outData1.select<8, 1>(16));
                    break;
                case 1:
                    write(OUTBUF_IDX, linear_offset_byte,       outData0.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 16, elment_offset, outData0.select<8, 1>(16));
                    break;
                default:
                    break;
                }
                break;
            case 2:
                outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 0) = inData0;
                outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 8) = inData1;

                //Padding unused by the first one pixel in the last sub block
                for (int i = last_block_size; i < SUB_BLOCK_PIXEL_WIDTH; i ++)
                {
                    outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 8 + i) = outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 8);
                }

                swapchannels(outSwappedData0,tempSwappedData, pixel_size);
                swapchannels(outSwappedData1,tempSwappedData, pixel_size);
                swapchannels(outSwappedData2,tempSwappedData, pixel_size);
                swapchannels(outSwappedData3,tempSwappedData, pixel_size);
                swapchannels(outSwappedData4,tempSwappedData, pixel_size);
                swapchannels(outSwappedData5,tempSwappedData, pixel_size);
                swapchannels(outSwappedData6,tempSwappedData, pixel_size);
                swapchannels(outSwappedData7,tempSwappedData, pixel_size);
                switch(last_block_height)
                {
                case 8:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 6, outData6.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 7, outData7.select<8, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 8,                   elment_offset, outData0.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword,     elment_offset, outData1.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 2, elment_offset, outData2.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 3, elment_offset, outData3.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 4, elment_offset, outData4.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 5, elment_offset, outData5.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 6, elment_offset, outData6.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 7, elment_offset, outData7.select<8, 1>(8));
                    break;
                case 7:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 6, outData6.select<8, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 8,                   elment_offset, outData0.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword,     elment_offset, outData1.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 2, elment_offset, outData2.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 3, elment_offset, outData3.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 4, elment_offset, outData4.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 5, elment_offset, outData5.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 6, elment_offset, outData6.select<8, 1>(8));
                    break;
                case 6:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5.select<8, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 8,                   elment_offset, outData0.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword,     elment_offset, outData1.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 2, elment_offset, outData2.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 3, elment_offset, outData3.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 4, elment_offset, outData4.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 5, elment_offset, outData5.select<8, 1>(8));
                    break;
                case 5:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<8, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 8,                   elment_offset, outData0.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword,     elment_offset, outData1.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 2, elment_offset, outData2.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 3, elment_offset, outData3.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 4, elment_offset, outData4.select<8, 1>(8));
                    break;
                default:
                    break;
                }
                switch(last_block_height)
                {
                case 4:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<8, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 8,                   elment_offset, outData0.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword,     elment_offset, outData1.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 2, elment_offset, outData2.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 3, elment_offset, outData3.select<8, 1>(8));
                    break;
                case 3:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<8, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 8,                   elment_offset, outData0.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword,     elment_offset, outData1.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 2, elment_offset, outData2.select<8, 1>(8));
                    break;
                case 2:
                    write(OUTBUF_IDX, linear_offset_byte,              outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1.select<8, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 8,               elment_offset, outData0.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword, elment_offset, outData1.select<8, 1>(8));
                    break;
                case 1:
                    write(OUTBUF_IDX, linear_offset_byte, outData0.select<8, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 8, elment_offset, outData0.select<8, 1>(8));
                    break;
                default:
                    break;
                }
                break;
            case 1:
                outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 0) = inData0;

                //Padding unused by the first one pixel in the last sub block
                for (int i = last_block_size; i < SUB_BLOCK_PIXEL_WIDTH; i ++)
                {
                    outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, i) = outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 0);
                }

                swapchannels(outSwappedData0,tempSwappedData, pixel_size);
                swapchannels(outSwappedData1,tempSwappedData, pixel_size);
                swapchannels(outSwappedData2,tempSwappedData, pixel_size);
                swapchannels(outSwappedData3,tempSwappedData, pixel_size);
                swapchannels(outSwappedData4,tempSwappedData, pixel_size);
                swapchannels(outSwappedData5,tempSwappedData, pixel_size);
                swapchannels(outSwappedData6,tempSwappedData, pixel_size);
                swapchannels(outSwappedData7,tempSwappedData, pixel_size);
                switch(last_block_height)
                {
                case 8:
                    write(OUTBUF_IDX, linear_offset_dword,                   elment_offset, outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword,     elment_offset, outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 2, elment_offset, outData2.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 3, elment_offset, outData3.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 4, elment_offset, outData4.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 5, elment_offset, outData5.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 6, elment_offset, outData6.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 7, elment_offset, outData7.select<8, 1>(0));
                    break;
                case 7:
                    write(OUTBUF_IDX, linear_offset_dword,                   elment_offset, outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword,     elment_offset, outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 2, elment_offset, outData2.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 3, elment_offset, outData3.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 4, elment_offset, outData4.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 5, elment_offset, outData5.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 6, elment_offset, outData6.select<8, 1>(0));
                    break;
                case 6:
                    write(OUTBUF_IDX, linear_offset_dword,                   elment_offset, outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword,     elment_offset, outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 2, elment_offset, outData2.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 3, elment_offset, outData3.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 4, elment_offset, outData4.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 5, elment_offset, outData5.select<8, 1>(0));
                    break;
                case 5:
                    write(OUTBUF_IDX, linear_offset_dword,                   elment_offset, outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword,     elment_offset, outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 2, elment_offset, outData2.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 3, elment_offset, outData3.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 4, elment_offset, outData4.select<8, 1>(0));
                    break;
                default:
                    break;
                }
                switch(last_block_height)
                {
                case 4:
                    write(OUTBUF_IDX, linear_offset_dword,                   elment_offset, outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword,     elment_offset, outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 2, elment_offset, outData2.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 3, elment_offset, outData3.select<8, 1>(0));
                    break;
                case 3:
                    write(OUTBUF_IDX, linear_offset_dword,                   elment_offset, outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword,     elment_offset, outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 2, elment_offset, outData2.select<8, 1>(0));
                    break;
                case 2:
                    write(OUTBUF_IDX, linear_offset_dword,               elment_offset, outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword, elment_offset, outData1.select<8, 1>(0));
                    break;
                case 1:
                    write(OUTBUF_IDX, linear_offset_dword, elment_offset, outData0.select<8, 1>(0));
                    break;
                default:
                    break;
                }
                break;
            default:
                break;
            }
        }
    }
}

extern "C" _GENX_MAIN_  void
surfaceCopy_readswap_32x32(SurfaceIndex INBUF_IDX, SurfaceIndex OUTBUF_IDX, int width_dword, int height, int ShiftLeftOffsetInBytes, int threadHeight, int width_in_dword_no_padding, int height_no_padding, int pixel_size, int src2D_start_x, int src2D_start_y)
{
    int horizOffset = get_thread_origin_x() * BLOCK_PIXEL_WIDTH;
    int vertOffset = get_thread_origin_y() * BLOCK_HEIGHT;
#pragma unroll
    for (int i = 0; i< 4; i ++)
    {
        surfaceCopy_readswap_32x8(INBUF_IDX, OUTBUF_IDX, width_dword, height, ShiftLeftOffsetInBytes, horizOffset, vertOffset + SUB_BLOCK_HEIGHT * threadHeight * i, width_in_dword_no_padding, height_no_padding, pixel_size, src2D_start_x, src2D_start_y);
    }
}

inline _GENX_  void
surfaceCopy_read_32x8(SurfaceIndex INBUF_IDX, SurfaceIndex OUTBUF_IDX, int width_dword, int height_stride_in_rows, int ShiftLeftOffsetInBytes, int horizOffset, int vertOffset, int width_in_dword_no_padding, int height_no_padding, int src2D_start_x, int src2D_start_y)
{
    int copy_width_dword = MIN(width_in_dword_no_padding, width_dword);
    int copy_height = MIN(height_no_padding, height_stride_in_rows);
    int width_byte = width_dword << 2;
    int horizOffset_byte = src2D_start_x + (horizOffset << 2);
    int vertOffset_row = src2D_start_y + vertOffset;
    int sub_block_width_byte = SUB_BLOCK_PIXEL_WIDTH << 2;

    uint linear_offset_dword = vertOffset * width_dword + horizOffset + (ShiftLeftOffsetInBytes/4);
    uint linear_offset_byte = (linear_offset_dword << 2) ;
    if (linear_offset_byte < width_byte * copy_height + ShiftLeftOffsetInBytes)
    {

        matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> inData0;
        matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> inData1;
        matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> inData2;
        matrix<uint, SUB_BLOCK_HEIGHT, SUB_BLOCK_PIXEL_WIDTH> inData3;

        matrix<uint, BLOCK_HEIGHT, BLOCK_PIXEL_WIDTH> outData_m;
        vector_ref<uint, BLOCK_PIXEL_WIDTH> outData0 = outData_m.row(0);
        vector_ref<uint, BLOCK_PIXEL_WIDTH> outData1 = outData_m.row(1);
        vector_ref<uint, BLOCK_PIXEL_WIDTH> outData2 = outData_m.row(2);
        vector_ref<uint, BLOCK_PIXEL_WIDTH> outData3 = outData_m.row(3);
        vector_ref<uint, BLOCK_PIXEL_WIDTH> outData4 = outData_m.row(4);
        vector_ref<uint, BLOCK_PIXEL_WIDTH> outData5 = outData_m.row(5);
        vector_ref<uint, BLOCK_PIXEL_WIDTH> outData6 = outData_m.row(6);
        vector_ref<uint, BLOCK_PIXEL_WIDTH> outData7 = outData_m.row(7);
        uint last_block_height = 8;
        if(copy_height - vertOffset < BLOCK_HEIGHT)
        {
            last_block_height = copy_height - vertOffset;
        }

        if (copy_width_dword - horizOffset >= BLOCK_PIXEL_WIDTH)    // for aligned block
        {
            read(INBUF_IDX, horizOffset_byte,                          vertOffset_row, inData0);
            read(INBUF_IDX, horizOffset_byte + sub_block_width_byte,   vertOffset_row, inData1);
            read(INBUF_IDX, horizOffset_byte + sub_block_width_byte*2, vertOffset_row, inData2);
            read(INBUF_IDX, horizOffset_byte + sub_block_width_byte*3, vertOffset_row, inData3);

            outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 0)  = inData0;
            outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 8)  = inData1;
            outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 16) = inData2;
            outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 24) = inData3;

            switch(last_block_height)
            {
            case 8:
                write(OUTBUF_IDX, linear_offset_byte,                  outData0);
                write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 6, outData6);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 7, outData7);
                break;

            case 7:
                write(OUTBUF_IDX, linear_offset_byte,                  outData0);
                write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 6, outData6);
                break;

            case 6:
                write(OUTBUF_IDX, linear_offset_byte,                  outData0);
                write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5);
                break;

            case 5:
                write(OUTBUF_IDX, linear_offset_byte,                  outData0);
                write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4);
                break;

            default:
                break;
            }
            switch(last_block_height)
            {
            case 4:
                write(OUTBUF_IDX, linear_offset_byte,                  outData0);
                write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3);
                break;

            case 3:
                write(OUTBUF_IDX, linear_offset_byte,                  outData0);
                write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1);
                write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2);
                break;

            case 2:
                write(OUTBUF_IDX, linear_offset_byte,              outData0);
                write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1);
                break;

            case 1:
                write(OUTBUF_IDX, linear_offset_byte, outData0);
                break;

            default:
                break;
            }
        }
        else    // for the unaligned most right blocks
        {
            uint block_width = copy_width_dword - horizOffset;
            uint last_block_size = block_width;
            uint read_times = 1;
            read(INBUF_IDX, horizOffset_byte, vertOffset_row, inData0);
            if (block_width > 8)
            {
                read(INBUF_IDX, horizOffset_byte + sub_block_width_byte, vertOffset_row, inData1);
                read_times = 2;
                last_block_size = last_block_size - 8;
                if (block_width > 16)
                {
                    read(INBUF_IDX, horizOffset_byte + sub_block_width_byte*2, vertOffset_row, inData2);
                    read_times = 3;
                    last_block_size = last_block_size - 8;
                    if (block_width > 24)
                    {
                        read(INBUF_IDX, horizOffset_byte + sub_block_width_byte*3, vertOffset_row, inData3);
                        read_times = 4;
                        last_block_size = last_block_size - 8;
                    }
                }
            }

            vector<uint, SUB_BLOCK_PIXEL_WIDTH> elment_offset(0);

            for (uint i=0; i < last_block_size; i++)
            {
                elment_offset(i) = i;
            }

            switch (read_times)
            {
            case 4:
                outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 0)  = inData0;
                outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 8)  = inData1;
                outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 16) = inData2;
                outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 24) = inData3;

                //Padding unused by the first one pixel in the last sub block
                for (int i = last_block_size; i < SUB_BLOCK_PIXEL_WIDTH; i ++)
                {
                    outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 24 + i) = outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 24);
                }

                switch(last_block_height)
                {
                case 8:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 6, outData6.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 7, outData7.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_byte + 64,                  outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte,     outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 2, outData2.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 3, outData3.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 4, outData4.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 5, outData5.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 6, outData6.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 7, outData7.select<8, 1>(16));

                    write(OUTBUF_IDX, linear_offset_dword + 24,                   elment_offset, outData0.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword,     elment_offset, outData1.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 2, elment_offset, outData2.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 3, elment_offset, outData3.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 4, elment_offset, outData4.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 5, elment_offset, outData5.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 6, elment_offset, outData6.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 7, elment_offset, outData7.select<8, 1>(24));
                    break;

                case 7:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 6, outData6.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_byte + 64,                  outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte,     outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 2, outData2.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 3, outData3.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 4, outData4.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 5, outData5.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 6, outData6.select<8, 1>(16));

                    write(OUTBUF_IDX, linear_offset_dword + 24,                   elment_offset, outData0.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword,     elment_offset, outData1.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 2, elment_offset, outData2.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 3, elment_offset, outData3.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 4, elment_offset, outData4.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 5, elment_offset, outData5.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 6, elment_offset, outData6.select<8, 1>(24));
                    break;

                case 6:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_byte + 64,                  outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte,     outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 2, outData2.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 3, outData3.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 4, outData4.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 5, outData5.select<8, 1>(16));

                    write(OUTBUF_IDX, linear_offset_dword + 24,                   elment_offset, outData0.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword,     elment_offset, outData1.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 2, elment_offset, outData2.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 3, elment_offset, outData3.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 4, elment_offset, outData4.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 5, elment_offset, outData5.select<8, 1>(24));
                    break;

                case 5:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_byte + 64,                  outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte,     outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 2, outData2.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 3, outData3.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 4, outData4.select<8, 1>(16));

                    write(OUTBUF_IDX, linear_offset_dword + 24,                   elment_offset, outData0.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword,     elment_offset, outData1.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 2, elment_offset, outData2.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 3, elment_offset, outData3.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 4, elment_offset, outData4.select<8, 1>(24));
                    break;

                default:
                    break;
                }
                switch(last_block_height)
                {
                case 4:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_byte + 64,                  outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte,     outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 2, outData2.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 3, outData3.select<8, 1>(16));

                    write(OUTBUF_IDX, linear_offset_dword + 24,                   elment_offset, outData0.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword,     elment_offset, outData1.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 2, elment_offset, outData2.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 3, elment_offset, outData3.select<8, 1>(24));
                    break;

                case 3:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_byte + 64,                  outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte,     outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte * 2, outData2.select<8, 1>(16));

                    write(OUTBUF_IDX, linear_offset_dword + 24,                   elment_offset, outData0.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword,     elment_offset, outData1.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword * 2, elment_offset, outData2.select<8, 1>(24));
                    break;

                case 2:
                    write(OUTBUF_IDX, linear_offset_byte,              outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_byte + 64,              outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_byte + 64 + width_byte, outData1.select<8, 1>(16));

                    write(OUTBUF_IDX, linear_offset_dword + 24,               elment_offset, outData0.select<8, 1>(24));
                    write(OUTBUF_IDX, linear_offset_dword + 24 + width_dword, elment_offset, outData1.select<8, 1>(24));
                    break;

                case 1:
                    write(OUTBUF_IDX, linear_offset_byte, outData0.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_byte + 64, outData0.select<8, 1>(16));

                    write(OUTBUF_IDX, linear_offset_dword + 24, elment_offset, outData0.select<8, 1>(24));
                    break;

                default:
                    break;
                }
                break;
            case 3:
                outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 0) = inData0;
                outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 8) = inData1;
                outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 16) = inData2;

                //Padding unused by the first one pixel in the last sub block
                for (int i = last_block_size; i < SUB_BLOCK_PIXEL_WIDTH; i ++)
                {
                    outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 16 + i) = outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 16);
                }

                switch(last_block_height)
                {
                case 8:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 6, outData6.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 7, outData7.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 16,                   elment_offset, outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword,     elment_offset, outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 2, elment_offset, outData2.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 3, elment_offset, outData3.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 4, elment_offset, outData4.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 5, elment_offset, outData5.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 6, elment_offset, outData6.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 7, elment_offset, outData7.select<8, 1>(16));
                    break;
                case 7:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 6, outData6.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 16,                   elment_offset, outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword,     elment_offset, outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 2, elment_offset, outData2.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 3, elment_offset, outData3.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 4, elment_offset, outData4.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 5, elment_offset, outData5.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 6, elment_offset, outData6.select<8, 1>(16));
                    break;
                case 6:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 16,                   elment_offset, outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword,     elment_offset, outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 2, elment_offset, outData2.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 3, elment_offset, outData3.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 4, elment_offset, outData4.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 5, elment_offset, outData5.select<8, 1>(16));
                    break;
                case 5:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 16,                   elment_offset, outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword,     elment_offset, outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 2, elment_offset, outData2.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 3, elment_offset, outData3.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 4, elment_offset, outData4.select<8, 1>(16));
                    break;
                default:
                    break;
                }
                switch(last_block_height)
                {
                case 4:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 16,                   elment_offset, outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword,     elment_offset, outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 2, elment_offset, outData2.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 3, elment_offset, outData3.select<8, 1>(16));
                    break;
                case 3:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 16,                   elment_offset, outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword,     elment_offset, outData1.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword * 2, elment_offset, outData2.select<8, 1>(16));
                    break;
                case 2:
                    write(OUTBUF_IDX, linear_offset_byte,              outData0.select<16, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 16,               elment_offset, outData0.select<8, 1>(16));
                    write(OUTBUF_IDX, linear_offset_dword + 16 + width_dword, elment_offset, outData1.select<8, 1>(16));
                    break;
                case 1:
                    write(OUTBUF_IDX, linear_offset_byte,       outData0.select<16, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 16, elment_offset, outData0.select<8, 1>(16));
                    break;
                default:
                    break;
                }
                break;
            case 2:
                outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 0) = inData0;
                outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 8) = inData1;

                //Padding unused by the first one pixel in the last sub block
                for (int i = last_block_size; i < SUB_BLOCK_PIXEL_WIDTH; i ++)
                {
                    outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 8 + i) = outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 8);
                }

                switch(last_block_height)
                {
                case 8:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 6, outData6.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 7, outData7.select<8, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 8,                   elment_offset, outData0.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword,     elment_offset, outData1.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 2, elment_offset, outData2.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 3, elment_offset, outData3.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 4, elment_offset, outData4.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 5, elment_offset, outData5.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 6, elment_offset, outData6.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 7, elment_offset, outData7.select<8, 1>(8));
                    break;
                case 7:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 6, outData6.select<8, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 8,                   elment_offset, outData0.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword,     elment_offset, outData1.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 2, elment_offset, outData2.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 3, elment_offset, outData3.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 4, elment_offset, outData4.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 5, elment_offset, outData5.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 6, elment_offset, outData6.select<8, 1>(8));
                    break;
                case 6:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 5, outData5.select<8, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 8,                   elment_offset, outData0.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword,     elment_offset, outData1.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 2, elment_offset, outData2.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 3, elment_offset, outData3.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 4, elment_offset, outData4.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 5, elment_offset, outData5.select<8, 1>(8));
                    break;
                case 5:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 4, outData4.select<8, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 8,                   elment_offset, outData0.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword,     elment_offset, outData1.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 2, elment_offset, outData2.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 3, elment_offset, outData3.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 4, elment_offset, outData4.select<8, 1>(8));
                    break;
                default:
                    break;
                }
                switch(last_block_height)
                {
                case 4:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 3, outData3.select<8, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 8,                   elment_offset, outData0.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword,     elment_offset, outData1.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 2, elment_offset, outData2.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 3, elment_offset, outData3.select<8, 1>(8));
                    break;
                case 3:
                    write(OUTBUF_IDX, linear_offset_byte,                  outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte,     outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte * 2, outData2.select<8, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 8,                   elment_offset, outData0.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword,     elment_offset, outData1.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword * 2, elment_offset, outData2.select<8, 1>(8));
                    break;
                case 2:
                    write(OUTBUF_IDX, linear_offset_byte,              outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_byte + width_byte, outData1.select<8, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 8,               elment_offset, outData0.select<8, 1>(8));
                    write(OUTBUF_IDX, linear_offset_dword + 8 + width_dword, elment_offset, outData1.select<8, 1>(8));
                    break;
                case 1:
                    write(OUTBUF_IDX, linear_offset_byte, outData0.select<8, 1>(0));

                    write(OUTBUF_IDX, linear_offset_dword + 8, elment_offset, outData0.select<8, 1>(8));
                    break;
                default:
                    break;
                }
                break;
            case 1:
                outData_m.select<SUB_BLOCK_HEIGHT, 1, SUB_BLOCK_PIXEL_WIDTH, 1>(0, 0) = inData0;

                //Padding unused by the first one pixel in the last sub block
                for (int i = last_block_size; i < SUB_BLOCK_PIXEL_WIDTH; i ++)
                {
                    outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, i) = outData_m.select<SUB_BLOCK_HEIGHT, 1, 1, 1>(0, 0);
                }

                switch(last_block_height)
                {
                case 8:
                    write(OUTBUF_IDX, linear_offset_dword,                   elment_offset, outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword,     elment_offset, outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 2, elment_offset, outData2.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 3, elment_offset, outData3.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 4, elment_offset, outData4.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 5, elment_offset, outData5.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 6, elment_offset, outData6.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 7, elment_offset, outData7.select<8, 1>(0));
                    break;
                case 7:
                    write(OUTBUF_IDX, linear_offset_dword,                   elment_offset, outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword,     elment_offset, outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 2, elment_offset, outData2.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 3, elment_offset, outData3.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 4, elment_offset, outData4.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 5, elment_offset, outData5.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 6, elment_offset, outData6.select<8, 1>(0));
                    break;
                case 6:
                    write(OUTBUF_IDX, linear_offset_dword,                   elment_offset, outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword,     elment_offset, outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 2, elment_offset, outData2.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 3, elment_offset, outData3.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 4, elment_offset, outData4.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 5, elment_offset, outData5.select<8, 1>(0));
                    break;
                case 5:
                    write(OUTBUF_IDX, linear_offset_dword,                   elment_offset, outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword,     elment_offset, outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 2, elment_offset, outData2.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 3, elment_offset, outData3.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 4, elment_offset, outData4.select<8, 1>(0));
                    break;
                default:
                    break;
                }
                switch(last_block_height)
                {
                case 4:
                    write(OUTBUF_IDX, linear_offset_dword,                   elment_offset, outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword,     elment_offset, outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 2, elment_offset, outData2.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 3, elment_offset, outData3.select<8, 1>(0));
                    break;
                case 3:
                    write(OUTBUF_IDX, linear_offset_dword,                   elment_offset, outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword,     elment_offset, outData1.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword * 2, elment_offset, outData2.select<8, 1>(0));
                    break;
                case 2:
                    write(OUTBUF_IDX, linear_offset_dword,               elment_offset, outData0.select<8, 1>(0));
                    write(OUTBUF_IDX, linear_offset_dword + width_dword, elment_offset, outData1.select<8, 1>(0));
                    break;
                case 1:
                    write(OUTBUF_IDX, linear_offset_dword, elment_offset, outData0.select<8, 1>(0));
                    break;
                default:
                    break;
                }
                break;
            default:
                break;
            }
        }
    }
}
extern "C" _GENX_MAIN_  void
surfaceCopy_read_32x32(SurfaceIndex INBUF_IDX, SurfaceIndex OUTBUF_IDX, int width_dword, int height, int ShiftLeftOffsetInBytes, int threadHeight, int width_in_dword_no_padding, int height_no_padding, int src2D_start_x, int src2D_start_y)
{
    int horizOffset = get_thread_origin_x() * BLOCK_PIXEL_WIDTH;
    int vertOffset = get_thread_origin_y() * BLOCK_HEIGHT;
#pragma unroll
    for (int i = 0; i< 4; i ++)
    {
        surfaceCopy_read_32x8(INBUF_IDX, OUTBUF_IDX, width_dword, height, ShiftLeftOffsetInBytes, horizOffset, vertOffset + SUB_BLOCK_HEIGHT * threadHeight * i, width_in_dword_no_padding, height_no_padding, src2D_start_x, src2D_start_y);
    }
}
