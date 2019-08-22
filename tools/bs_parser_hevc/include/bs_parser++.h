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

#ifndef __BS_PARSER_PP_H
#define __BS_PARSER_PP_H

#include <bs_parser.h>
#include <memory.h>

class BS_parser{
public:
    BS_parser(){};
    virtual BSErr open(const char* file) = 0;
    virtual BSErr set_buffer(byte* buf, Bs32u buf_size) = 0;
    virtual BSErr parse_next_unit() = 0;
    virtual BSErr reset() = 0;
    virtual BSErr lock  (void* p) = 0;
    virtual BSErr unlock(void* p) = 0;
    virtual void* get_header() = 0;
    virtual void* get_handle() = 0;
    virtual Bs64u get_offset() = 0;
    virtual void set_trace_level(Bs32u level) = 0;
    virtual BSErr sync(void* p) { (void) p; return BS_ERR_NONE; };
    virtual Bs16u async_depth() { return 0; }
    virtual ~BS_parser(){};
};

class BS_HEVC2_parser : public BS_parser{
public:
    typedef BS_HEVC2::NALU UnitType;

    BS_HEVC2_parser(Bs32u mode = 0)
    {
        BS_HEVC2_Init(hdl, mode);
        hdr = 0;
    };

    BS_HEVC2_parser(BS_HEVC2_parser const&) = delete;
    BS_HEVC2_parser& operator=(BS_HEVC2_parser const&) = delete;

    ~BS_HEVC2_parser() { BS_HEVC2_Close(hdl); };

    BSErr parse_next_au(UnitType*& pAU)         { return BS_HEVC2_ParseNextAU(hdl, pAU); };
    BSErr parse_next_unit()                     { return BS_HEVC2_ParseNextAU(hdl, hdr); };
    BSErr open(const char* file)                { return BS_HEVC2_OpenFile(hdl, file); };
    BSErr set_buffer(byte* buf, Bs32u buf_size) { return BS_HEVC2_SetBuffer(hdl, buf, buf_size); };
    BSErr lock(void* p)                         { return BS_HEVC2_Lock(hdl, p); };
    BSErr unlock(void* p)                       { return BS_HEVC2_Unlock(hdl, p); };
    BSErr sync(void* p)                         { return BS_HEVC2_Sync(hdl, (BS_HEVC2::NALU*)p); };
    Bs16u async_depth()                         { return BS_HEVC2_GetAsyncDepth(hdl); }

    void set_trace_level(Bs32u level)           { BS_HEVC2_SetTraceLevel(hdl, level); };
    void* get_header() { return hdr; };
    void* get_handle() { return hdl; };
    Bs64u get_offset() {
        Bs64u offset = -1;
        BS_HEVC2_GetOffset(hdl, offset);
        return offset;
    };

    BSErr reset()
    {
        BS_HEVC2_Close(hdl);
        hdr = 0;
        return BS_HEVC2_Init(hdl, 0);
    };
private:
    BS_HEVC2::HDL hdl;
    UnitType* hdr;
};


#endif //__BS_PARSER_PP_H
