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

//Some consts and enums for intra prediction

#ifndef __INTER_TEST_H__
#define __INTER_TEST_H__

#include "mfxvideo.h"

#if MFX_VERSION >= MFX_VERSION_NEXT

#include "mfxdefs.h"

enum INTER_PART_MODE {
    INTER_NONE  = -1,
    INTER_2Nx2N = 0,
    INTER_2NxN,
    INTER_Nx2N,
    INTER_NxN,
    INTER_2NxnU,
    INTER_2NxnD,
    INTER_nLx2N,
    INTER_nRx2N
};
const mfxU32 INTER_PART_MODE_NUM       = 8;
const mfxU32 INTER_8x8CU_PART_MODE_NUM = 3; //For 8x8 CUs only first 3 modes are available
const mfxU32 INTER_SYMM_PART_MODE_NUM  = 4; //First 4 modes are symmetric

#endif // MFX_VERSION

#endif //__INTER_TEST_H__
