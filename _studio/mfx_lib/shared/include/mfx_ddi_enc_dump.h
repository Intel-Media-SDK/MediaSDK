// Copyright (c) 2017 Intel Corporation
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

typedef struct tagENCODE_SET_SEQUENCE_PARAMETERS_H264 ENCODE_SET_SEQUENCE_PARAMETERS_H264;
typedef struct tagENCODE_SET_PICTURE_PARAMETERS_H264  ENCODE_SET_PICTURE_PARAMETERS_H264;
typedef struct tagENCODE_SET_SLICE_HEADER_H264        ENCODE_SET_SLICE_HEADER_H264;

void ddiDumpH264SPS(FILE *f,
                    ENCODE_SET_SEQUENCE_PARAMETERS_H264* sps);

void ddiDumpH264PPS(FILE *f,
                    ENCODE_SET_PICTURE_PARAMETERS_H264* pps);

void ddiDumpH264SliceHeader(FILE *f,
                            ENCODE_SET_SLICE_HEADER_H264* sh,
                            int NumSlice);

void ddiDumpH264MBData(FILE     *fText,
                       mfxI32   _PicNum,
                       mfxI32   PicType,
                       mfxI32   PicWidth,
                       mfxI32   PicHeight,
                       mfxI32   NumSlicesPerPic,
                       mfxI32   NumMBs,
                       void     *pMBData,
                       void     *pMVData);
