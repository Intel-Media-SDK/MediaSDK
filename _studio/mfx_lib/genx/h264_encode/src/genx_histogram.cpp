// Copyright (c) 2015-2018 Intel Corporation
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

#if defined(_WIN32) || defined(_WIN64)

#include "cm/cm.h"

static const ushort usSeq0_15[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
static const ushort uiSeq0_7[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };

extern "C" _GENX_MAIN_ void HistogramSLMFrame(
    SurfaceIndex INBUF,
    SurfaceIndex OUTBUF,
    uint max_h_pos,
    uint max_v_pos,
    uint off_h,
    uint off_v)
{
    const uint SLM_SIZE_INT = 256;
    const uint SLM_SIZE = SLM_SIZE_INT * sizeof(uint);

    vector<uint, 64> vUI64;
    vector<uint, 16> vUI16;
    vector<uint, 256> vHist;
    vector<ushort, 16> vOffsetsInit(usSeq0_15);
    vector<ushort, 16> vOffsets;
    vector<uint, 8> vOffsets8(uiSeq0_7);
    matrix<uchar, 8, 32> mUC8x32;

    uint local_id = cm_linear_local_id();
    uint local_num_threads = cm_linear_local_size();
    uint total_num_threads = cm_linear_global_size();
    uint h_pos = cm_linear_global_id();
    uint v_pos = 0;
    uint start4 = 64 * local_id;
    uint step4 = 64 * local_num_threads;
    int  i, j;

    cm_slm_init(SLM_SIZE);
    uint SLM = cm_slm_alloc(SLM_SIZE);

    // zero SLM
    vUI64 = 0;
    vOffsets = start4 + vOffsetsInit * 4;

    for (i = start4; i < SLM_SIZE_INT; i += step4)
    {
        cm_slm_write4(SLM, vOffsets, vUI64, SLM_ABGR_ENABLE);
        vOffsets += step4;
    }

    // Collect histogram for current thread
    vHist = 0;

    while (1)
    {
        while (h_pos >= max_h_pos)
        {
            v_pos++;
            h_pos -= max_h_pos;
        }

        if ((h_pos >= max_h_pos) || (v_pos >= max_v_pos))
        {
            break;
        }

        read(INBUF, (off_h + h_pos) * 32, (off_v + v_pos) * 8, mUC8x32);

#pragma unroll
        for (i = 0; i < 8; i++)
        {
#pragma unroll
            for (j = 0; j < 32; j++)
            {
                vHist(mUC8x32(i, j)) += 1;
            }
        }

        h_pos += total_num_threads;
    }

    // Combine thread histograms in SLM 
    vOffsets = vOffsetsInit;
    cm_barrier();

#pragma unroll
    for (i = 0; i < SLM_SIZE_INT; i += 16)
    {
        vUI16 = vHist.select<16, 1>(i);
        cm_slm_atomic(SLM, ATOMIC_ADD, vOffsets, 0, vUI16);
        vOffsets += 16;
    }

    // Combine group histograms and write
    vOffsets = start4 + vOffsetsInit * 4;
    cm_barrier();

    for (i = start4; i < SLM_SIZE_INT; i += step4)
    {
        vector<uint, 64> trans;
        vector<uint, 8>  vUI8;

        cm_slm_read4(SLM, vOffsets, vUI64, SLM_ABGR_ENABLE);

        trans.select<16, 4>(0) = vUI64.select<16, 1>(0);
        trans.select<16, 4>(1) = vUI64.select<16, 1>(16);
        trans.select<16, 4>(2) = vUI64.select<16, 1>(32);
        trans.select<16, 4>(3) = vUI64.select<16, 1>(48);

#pragma unroll
        for (j = 0; j < 64; j += 8)
        {
            vUI8 = trans.select<8, 1>(j);
            write<uint, 8>(OUTBUF, ATOMIC_ADD, i + j, vOffsets8, vUI8, 0);
        }

        vOffsets += step4;
    }

}

extern "C" _GENX_MAIN_ void HistogramSLMFields(
    SurfaceIndex INBUF,
    SurfaceIndex OUTBUF,
    uint max_h_pos,
    uint max_v_pos,
    uint off_h,
    uint off_v)
{
    const uint SLM_SIZE_INT = 512;
    const uint SLM_SIZE = SLM_SIZE_INT * sizeof(uint);

    vector<uint, 64> vUI64;
    vector<uint, 16> vUI16;
    vector<uint, 512> vHist;
    vector<ushort, 16> vOffsetsInit(usSeq0_15);
    vector<ushort, 16> vOffsets;
    vector<uint, 8> vOffsets8(uiSeq0_7);
    matrix<uchar, 8, 32> mUC8x32;

    uint local_id = cm_linear_local_id();
    uint local_num_threads = cm_linear_local_size();
    uint total_num_threads = cm_linear_global_size();
    uint h_pos = cm_linear_global_id();
    uint v_pos = 0;
    uint start4 = 64 * local_id;
    uint step4 = 64 * local_num_threads;
    int  i, j;

    cm_slm_init(SLM_SIZE);
    uint SLM = cm_slm_alloc(SLM_SIZE);

    // zero SLM
    vUI64 = 0;
    vOffsets = start4 + vOffsetsInit * 4;

    for (i = start4; i < SLM_SIZE_INT; i += step4)
    {
        cm_slm_write4(SLM, vOffsets, vUI64, SLM_ABGR_ENABLE);
        vOffsets += step4;
    }

    // Collect histogram for current thread
    vHist = 0;

    while (1)
    {
        while (h_pos >= max_h_pos)
        {
            v_pos++;
            h_pos -= max_h_pos;
        }

        if ((h_pos >= max_h_pos) || (v_pos >= max_v_pos))
        {
            break;
        }

        read(INBUF, (off_h + h_pos) * 32, (off_v + v_pos) * 8, mUC8x32);

#pragma unroll
        for (i = 0; i < 8; i++)
        {
#pragma unroll
            for (j = 0; j < 32; j += 2)
            {
                vHist(mUC8x32(i, j)) += 1;
                vHist(mUC8x32(i, j + 1) + 256) += 1;
            }
        }

        h_pos += total_num_threads;
    }

    // Combine thread histograms in SLM 
    vOffsets = vOffsetsInit;
    cm_barrier();

#pragma unroll
    for (i = 0; i < SLM_SIZE_INT; i += 16)
    {
        vUI16 = vHist.select<16, 1>(i);
        cm_slm_atomic(SLM, ATOMIC_ADD, vOffsets, 0, vUI16);
        vOffsets += 16;
    }

    // Combine group histograms and write
    vOffsets = start4 + vOffsetsInit * 4;
    cm_barrier();

    for (i = start4; i < SLM_SIZE_INT; i += step4)
    {
        vector<uint, 64> trans;
        vector<uint, 8>  vUI8;

        cm_slm_read4(SLM, vOffsets, vUI64, SLM_ABGR_ENABLE);

        // Transpose
        trans.select<16, 4>(0) = vUI64.select<16, 1>(0);
        trans.select<16, 4>(1) = vUI64.select<16, 1>(16);
        trans.select<16, 4>(2) = vUI64.select<16, 1>(32);
        trans.select<16, 4>(3) = vUI64.select<16, 1>(48);

#pragma unroll
        for (j = 0; j < 64; j += 8)
        {
            vUI8 = trans.select<8, 1>(j);
            write<uint, 8>(OUTBUF, ATOMIC_ADD, i + j, vOffsets8, vUI8, 0);
        }

        vOffsets += step4;
    }

}

#endif