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

#define INTERDATA_SIZE_SMALL    8
#define INTERDATA_SIZE_BIG      64   // 32x32 and 64x64 blocks
#define MVDATA_SIZE             4    // mfxI16Pair
#define SINGLE_MVDATA_SIZE      2    // mfxI16Pair
#define MBDIST_SIZE             64   // 16*mfxU32
#define DIST_SIZE               4

#define SLICE(VEC, FROM, HOWMANY, STEP) ((VEC).select<HOWMANY, STEP>(FROM))
#define SLICE1(VEC, FROM, HOWMANY) SLICE(VEC, FROM, HOWMANY, 1)
#define SELECT_N_ROWS(m, from, nrows) m.select<nrows, 1, m.COLS, 1>(from)
#define SELECT_N_COLS(m, from, ncols) m.select<m.ROWS, 1, ncols, 1>(0, from)

#define VME_SET_DWORD(dst, r, c, src)           \
    dst.row(r).format<uint>().select<1, 1>(c) = src

#define VME_Output_S2(p, t, r, c, n) p.row(r).format<t>().select<n,1>(c)
#define VME_Output_S2a(p, t, c, n) p.format<t>().select<n,1>(c)
#define VME_Output_S4(p, t, r, c, n) p.row(r).format<t>().select<n,4>(c)
#define VME_Output_S4a(p, t, c, n) p.format<t>().select<n,8>(c)
#define VME_GET_IMEOutput_Rec0_8x8_4Distortion(p, v) (v = VME_Output_S2(p, ushort, 7, 4, 4))
#define VME_GET_IMEOutput_Rec1_8x8_4Distortion(p, v) (v = VME_Output_S2(p, ushort, 9, 4, 4))
#define VME_GET_IMEOutput_Rec0_8x8_4Mv(p, v) (v = VME_Output_S2(p, uint, 8, 4, 4))
#define VME_GET_IMEOutput_Rec1_8x8_4Mv(p, v) (v = VME_Output_S2(p, uint,10, 4, 4))
#define VME_GET_IMEOutput_Rec0_16x16_Mv(p, v) (v = VME_Output_S2(p, ushort, 7, 10, 2))
#define VME_GET_IMEOutput_Rec1_16x16_Mv(p, v) (v = VME_Output_S2(p, ushort, 9, 10, 2))

#define VME_GET_FBROutput_Rec0_16x16_Mv(p, v) (v = VME_Output_S2a(p, short, 16, 2))
#define VME_GET_FBROutput_Rec1_16x16_Mv(p, v) (v = VME_Output_S2a(p, short, 18, 2))
#define VME_GET_FBROutput_Rec0_8x8_4Mv(p, v) (v = VME_Output_S4a(p, uint, 8, 4))
#define VME_GET_FBROutput_Rec1_8x8_4Mv(p, v) (v = VME_Output_S4a(p, uint, 9, 4))

#define VME_GET_FBROutput_Dist_16x16_Bi(p, v) (v = VME_Output_S2(p, ushort, 5, 0, 1))
#define VME_GET_FBROutput_Dist_8x8_Bi(p, v) (v = VME_Output_S4(p, ushort, 5, 0, 4))


