// Copyright (c) 2019 Intel Corporation
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

#ifndef _MFX_CENC_H_
#define _MFX_CENC_H_

#ifdef MFX_ENABLE_CPLIB

#ifdef __cplusplus
extern "C" {
#endif

#define VACencStatusParameterBufferType    -3

#define VA_ENCRYPTION_TYPE_CENC_CBC         0x00000002
#define VA_ENCRYPTION_TYPE_CENC_CTR_LENGTH  0x00000004

#define VA_PADDING_MEDIUM       8

typedef struct {
    uint32_t  status_report_index_feedback;
    uint32_t  va_reserved[VA_PADDING_MEDIUM];
} VACencStatusParameters;

#ifdef __cplusplus
}
#endif

#endif  // #ifdef MFX_ENABLE_CPLIB
#endif  // _MFX_CENC_H_
