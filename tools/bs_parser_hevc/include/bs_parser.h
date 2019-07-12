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

#ifndef __BS_PARSER_H
#define __BS_PARSER_H

#include <hevc2_struct.h>

extern "C"{

    BSErr __STDCALL BS_HEVC2_Init              (BS_HEVC2::HDL& hdl, Bs32u mode);
    BSErr __STDCALL BS_HEVC2_OpenFile          (BS_HEVC2::HDL hdl, const char* file);
    BSErr __STDCALL BS_HEVC2_SetBuffer         (BS_HEVC2::HDL hdl, byte* buf, Bs32u buf_size);
    BSErr __STDCALL BS_HEVC2_ParseNextAU       (BS_HEVC2::HDL hdl, BS_HEVC2::NALU*& pAU);
    BSErr __STDCALL BS_HEVC2_Close             (BS_HEVC2::HDL hdl);
    BSErr __STDCALL BS_HEVC2_SetTraceLevel     (BS_HEVC2::HDL hdl, Bs32u level);
    BSErr __STDCALL BS_HEVC2_Lock              (BS_HEVC2::HDL hdl, void* p);
    BSErr __STDCALL BS_HEVC2_Unlock            (BS_HEVC2::HDL hdl, void* p);
    BSErr __STDCALL BS_HEVC2_GetOffset         (BS_HEVC2::HDL hdl, Bs64u& offset);
    BSErr __STDCALL BS_HEVC2_Sync              (BS_HEVC2::HDL hdl, BS_HEVC2::NALU* slice);
    Bs16u __STDCALL BS_HEVC2_GetAsyncDepth     (BS_HEVC2::HDL hdl);

}


#endif //__BS_PARSER_H
