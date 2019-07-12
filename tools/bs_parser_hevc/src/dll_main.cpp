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

#include <hevc2_parser.h>

extern "C" {

// For Perl compatibility only:
Bs32u BS_GetVal( void* hdr, Bs32u* element_idx ){
    if( !hdr         ) return (Bs32u)BS_ERR_WRONG_UNITS_ORDER;
    if( !element_idx ) return (Bs32u)BS_ERR_INVALID_PARAMS;

    Bs32u val = 0;
    void* addr = hdr;
    Bs32s nesting_level = *element_idx;
    while( nesting_level-- ){
        addr = *(void**)((Bs64s)addr + *(++element_idx));
    }
    addr = (void*)((Bs64s)addr + *(++element_idx));
    for( Bs32u i = 0; i<(*(element_idx+1));i++ ){
        val += ((*((byte*)addr+i))<<(i<<3));
    }
    return val;
}


BSErr __STDCALL BS_HEVC2_Init(BS_HEVC2::HDL& hdl, Bs32u mode){
    hdl = NULL;
    hdl = new BS_HEVC2::Parser(mode);
    if (hdl)
        return BS_ERR_NONE;
    return BS_ERR_UNKNOWN;
}

BSErr __STDCALL BS_HEVC2_OpenFile(BS_HEVC2::HDL hdl, const char* file){
    if (!hdl) return BS_ERR_BAD_HANDLE;
    return hdl->open(file);
}

BSErr __STDCALL BS_HEVC2_SetBuffer(BS_HEVC2::HDL hdl, byte* buf, Bs32u buf_size){
    if (!hdl) return BS_ERR_BAD_HANDLE;
    hdl->set_buffer(buf, buf_size);
    return BS_ERR_NONE;
}

BSErr __STDCALL BS_HEVC2_ParseNextAU(BS_HEVC2::HDL hdl, BS_HEVC2::NALU*& header){
    if (!hdl) return BS_ERR_BAD_HANDLE;
    return hdl->parse_next_au(header);
}

BSErr __STDCALL BS_HEVC2_Close(BS_HEVC2::HDL hdl){
    if (!hdl) return BS_ERR_BAD_HANDLE;
    delete (hdl);
    hdl = NULL;
    return BS_ERR_NONE;
}

BSErr __STDCALL BS_HEVC2_SetTraceLevel(BS_HEVC2::HDL hdl, Bs32u level){
    if (!hdl) return BS_ERR_BAD_HANDLE;
    hdl->set_trace_level(level);
    return BS_ERR_NONE;
}

BSErr __STDCALL BS_HEVC2_Lock(BS_HEVC2::HDL hdl, void* p){
    if (!hdl) return BS_ERR_BAD_HANDLE;
    return hdl->Lock(p);
}

BSErr __STDCALL BS_HEVC2_Unlock(BS_HEVC2::HDL hdl, void* p){
    if (!hdl) return BS_ERR_BAD_HANDLE;
    return hdl->Unlock(p);
}

BSErr __STDCALL BS_HEVC2_GetOffset(BS_HEVC2::HDL hdl, Bs64u& offset){
    if (!hdl) return BS_ERR_BAD_HANDLE;
    offset = hdl->get_cur_pos();
    return BS_ERR_NONE;
}

BSErr __STDCALL BS_HEVC2_Sync(BS_HEVC2::HDL hdl, BS_HEVC2::NALU* slice){
    if (!hdl) return BS_ERR_BAD_HANDLE;
    return hdl->sync(slice);
}

Bs16u __STDCALL BS_HEVC2_GetAsyncDepth(BS_HEVC2::HDL hdl){
    if (!hdl) return 0;
    return hdl->get_async_depth();
}

} // extern "C"
