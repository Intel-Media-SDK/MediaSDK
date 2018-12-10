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
#ifndef _IOFUNCTIONS_H_
#define _IOFUNCTIONS_H_

#include "asc_structures.h"
#include "asc.h"

static inline mfxF64 TimeMeasurement(LARGE_INTEGER start, LARGE_INTEGER stop, LARGE_INTEGER frequency) {
    return((stop.QuadPart - start.QuadPart) * mfxF64(1000.0) / frequency.QuadPart);
}

void TimeStart(ns_asc::ASCTime* timer);
void TimeStart(ns_asc::ASCTime* timer, int index);
void TimeStop(ns_asc::ASCTime* timer);
mfxF64 CatchTime(ns_asc::ASCTime *timer, const char* message);
mfxF64 CatchTime(ns_asc::ASCTime *timer, int index, const char* message);
mfxF64 CatchTime(ns_asc::ASCTime *timer, int indexInit, int indexEnd, const char* message);

void imageInit(ns_asc::ASCYUV *buffer);
void nullifier(ns_asc::ASCimageData *Buffer);
void ImDetails_Init(ns_asc::ASCImDetails *Rdata);
mfxStatus ASCTSCstat_Init(ns_asc::ASCTSCstat **logic);


#endif //_IOFUNCTIONS_H_