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

#pragma once

#include "hevc2_struct.h"

namespace BS_HEVC2
{

extern const char NaluTraceMap[64][16];
extern const char PicTypeTraceMap[3][4];
extern const char ChromaFormatTraceMap[4][12];
extern const char VideoFormatTraceMap[6][12];
extern const char PicStructTraceMap[13][12];
extern const char ScanTypeTraceMap[4][12];
extern const char SliceTypeTraceMap[4][2];
extern const char PredModeTraceMap[3][6];
extern const char PartModeTraceMap[8][12];
extern const char IntraPredModeTraceMap[35][9];
extern const char SAOTypeTraceMap[3][5];
extern const char EOClassTraceMap[4][5];

struct ProfileTraceMap
{
    static const char map[11][12];
    const char* operator[] (unsigned int i);
};

struct ARIdcTraceMap
{
    static const char map[19][16];
    const char* operator[] (unsigned int i);
};

struct SEITypeTraceMap
{
    const char* operator[] (unsigned int i);
};

struct RPLTraceMap
{
    char m_buf[80];
    const char* operator[] (const RefPic& r);
};

}