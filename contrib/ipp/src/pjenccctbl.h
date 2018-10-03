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

/*
//
//  Purpose:
//    Color conversions tables
//    (Forward transform)
//
*/

#ifndef __PJENCCCTBL_H__
#define __PJENCCCTBL_H__

#ifndef __OWNJ_H__
#include "ownj.h"
#endif


/* -------------------- macros for JPEG color conversions ----------------- */

#define MAXCSAMPLE 255

#define R_Y_OFFS  0                   /* offset to R->Y section */
#define G_Y_OFFS  (1*(MAXCSAMPLE+1))  /* offset to G->Y section */
#define B_Y_OFFS  (2*(MAXCSAMPLE+1))  /* etc */
#define R_CB_OFF  (3*(MAXCSAMPLE+1))
#define G_CB_OFF  (4*(MAXCSAMPLE+1))
#define B_CB_OFF  (5*(MAXCSAMPLE+1))
#define R_CR_OFF  B_CB_OFF            /* B->Cb, R->Cr are the same */
#define G_CR_OFF  (6*(MAXCSAMPLE+1))
#define B_CR_OFF  (7*(MAXCSAMPLE+1))

#define CTBL_SIZE (8*(MAXCSAMPLE+1))


extern const int mfxcc_table[];


#endif /* __PJENCCCTBL_H__ */
