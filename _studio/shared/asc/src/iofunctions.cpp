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
#include "../include/iofunctions.h"
#include "asc_structures.h"
#include "asc_defs.h"

using namespace ns_asc;

void TimeStart(ASCTime* /*timer*/) {
}

void TimeStart(ASCTime* /*timer*/, int /*index*/) {
}

void TimeStop(ASCTime* /*timer*/) {
}

mfxF64 CatchTime(ASCTime* timer, const char* message)
{
    (void)message;

    mfxF64
        timeval = 0.0;
    timeval = TimeMeasurement(timer->tStart, timer->tStop, timer->tFrequency);
    ASC_PRINTF("%s %0.3f ms.\n", message, timeval);
    return timeval;
}

mfxF64 CatchTime(ASCTime* /*timer*/, int /*index*/, const char* /*message*/) {
    return 0.0;
}

mfxF64 CatchTime(ASCTime* /*timer*/, int /*indexInit*/, int /*indexEnd*/, const char* /*message*/) {
    return 0.0;
}



void imageInit(ASCYUV* buffer) {
    memset(buffer, 0, sizeof(ASCYUV));
}

void nullifier(ASCimageData* Buffer) {
    imageInit(&Buffer->Image);
    memset(&Buffer->pInteger, 0, sizeof(ASCMVector));
    memset(&Buffer->Cs, 0, sizeof(Buffer->Cs));
    memset(&Buffer->Rs, 0, sizeof(Buffer->Rs));
    memset(&Buffer->RsCs, 0, sizeof(Buffer->RsCs));
    memset(&Buffer->SAD, 0, sizeof(Buffer->SAD));
    Buffer->CsVal = 0;
    Buffer->RsVal = 0;
}

void ImDetails_Init(ASCImDetails* Rdata) {
    memset(Rdata, 0, sizeof(ASCImDetails));
}

mfxStatus ASCTSCstat_Init(ASCTSCstat** logic) {
    for(int i = 0; i < TSCSTATBUFFER; i++)
    {
        try
        {
            logic[i] = new ASCTSCstat;
        }
        catch (...)
        {
            return MFX_ERR_MEMORY_ALLOC;
        }
        memset(logic[i],0,sizeof(ASCTSCstat));
    }
    return MFX_ERR_NONE;
}
