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
#ifndef _TREE_H_
#define _TREE_H_

#include "asc_structures.h"

bool SCDetectRF( mfxI32 diffMVdiffVal, mfxU32 RsCsDiff,   mfxU32 MVDiff,   mfxU32 Rs,       mfxU32 AFD,
                 mfxU32 CsDiff,        mfxI32 diffTSC,    mfxU32 TSC,      mfxU32 gchDC,    mfxI32 diffRsCsdiff,
                 mfxU32 posBalance,    mfxU32 SC,         mfxU32 TSCindex, mfxU32 Scindex,  mfxU32 Cs,
                 mfxI32 diffAFD,       mfxU32 negBalance, mfxU32 ssDCval,  mfxU32 refDCval, mfxU32 RsDiff,
                 mfxU8 control);

#endif //_TREE_H_