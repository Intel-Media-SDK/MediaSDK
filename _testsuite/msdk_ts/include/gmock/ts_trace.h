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

#pragma once

#include "mfxvideo.h"
#include "mfxvp8.h"
#include "mfxvp9.h"
#include "mfxmvc.h"
#include "mfxjpeg.h"
#include "mfxplugin.h"
#include "mfxcamera.h"
#include "mfxfei.h"
#include "mfxfeihevc.h"
#include "mfxla.h"
#include "mfxsc.h"
#include "mfxbrc.h"

#include <iostream>
#include "ts_typedef.h"

class tsAutoTrace : public std::ostream
{
public:
    std::string m_off;

    tsAutoTrace(std::streambuf* sb)
        : std::ostream(sb)
        , m_off("")
    {}

    inline void inc_offset() { m_off += "  "; }
    inline void dec_offset() { m_off.erase(m_off.size() - 2, 2); }

    inline tsAutoTrace& operator<<(void* p) { (std::ostream&)*this << p; return *this; }
    inline tsAutoTrace& operator<<(mfxU8* p) { *this << (void*)p; return *this; }
    inline tsAutoTrace& operator<<(mfxEncryptedData*& p){ *this << (void*)p; return *this; }
    inline tsAutoTrace& operator<<(mfx4CC& p)
    {
        for(size_t i = 0; i < 4; ++i)
        {
            if(p.c[i])  (std::ostream&)*this << p.c[i];
            else        (std::ostream&)*this << "\\0";
        }
        (std::ostream&)*this << std::hex << "(0x" << p.n << ')' << std::dec;
        return *this;
    }

#define STRUCT(name, fields) virtual tsAutoTrace &operator << (const name& p); virtual tsAutoTrace &operator << (const name* p);
#define FIELD_T(type, _name)
#define FIELD_S(type, _name)

#include "ts_struct_decl.h"

#undef STRUCT
#undef FIELD_T
#undef FIELD_S
};

//printing "\0" symbol in unicode causes text file to be mailformed
inline tsAutoTrace& operator<<(tsAutoTrace& os, const char p)
{
    if(p)  (std::ostream&)os << p;
    else   (std::ostream&)os << "\\0";
    (std::ostream&)os << " (" << +p << ')';  //operator+(T v) casts v to printable numerical type
    return os;
}
inline tsAutoTrace& operator<<(tsAutoTrace& os, const unsigned char p)
{
    if(p)  (std::ostream&)os << p;
    else   (std::ostream&)os << "\\0";
    (std::ostream&)os << " (" << +p << ')';
    return os;
}
inline tsAutoTrace& operator<<(tsAutoTrace& os, const char* p)              { (std::ostream&)os << p; return os; }
inline tsAutoTrace& operator<<(tsAutoTrace& os, const unsigned char* p)     { (std::ostream&)os << p; return os; }
inline tsAutoTrace& operator<<(tsAutoTrace& os, const short p)              { (std::ostream&)os << p; return os; }
inline tsAutoTrace& operator<<(tsAutoTrace& os, const unsigned short p)     { (std::ostream&)os << p; return os; }
inline tsAutoTrace& operator<<(tsAutoTrace& os, const int p)                { (std::ostream&)os << p; return os; }
inline tsAutoTrace& operator<<(tsAutoTrace& os, const unsigned int p)       { (std::ostream&)os << p; return os; }
inline tsAutoTrace& operator<<(tsAutoTrace& os, const long p)               { (std::ostream&)os << p; return os; }
inline tsAutoTrace& operator<<(tsAutoTrace& os, const unsigned long p)      { (std::ostream&)os << p; return os; }
inline tsAutoTrace& operator<<(tsAutoTrace& os, const long long p)          { (std::ostream&)os << p; return os; }
inline tsAutoTrace& operator<<(tsAutoTrace& os, const unsigned long long p) { (std::ostream&)os << p; return os; }
inline tsAutoTrace& operator<<(tsAutoTrace& os, const float p)              { (std::ostream&)os << p; return os; }
inline tsAutoTrace& operator<<(tsAutoTrace& os, const double p)             { (std::ostream&)os << p; return os; }
inline tsAutoTrace& operator<<(tsAutoTrace& os, const std::string p)        { (std::ostream&)os << p.c_str(); return os; }

class tsTrace : public tsAutoTrace
{
private:
    bool    m_interpret_ext_buf;
    mfxU32  m_flags;
public:
    tsTrace(std::streambuf* sb) : tsAutoTrace(sb), m_interpret_ext_buf(true) {}


    tsTrace& operator << (const mfxExtBuffer& p);
    tsTrace& operator << (const mfxVideoParam& p);
    tsTrace& operator << (const mfxEncodeCtrl& p);
    tsTrace& operator << (const mfxExtAVCRefLists& p);
    tsTrace& operator << (const mfxExtEncoderROI& p);
    tsTrace& operator << (const mfxExtDirtyRect& p);
    tsTrace& operator << (const mfxExtMoveRect& p);
    tsTrace& operator << (const mfxExtDirtyRect_Entry& p);
    tsTrace& operator << (const mfxExtMoveRect_Entry& p);
    tsTrace& operator << (const mfxExtCamGammaCorrection& p);
    tsTrace& operator << (const mfxExtCamTotalColorControl& p);
    tsTrace& operator << (const mfxExtCamCscYuvRgb& p);
    tsTrace& operator << (const mfxExtCamVignetteCorrection& p);
    tsTrace& operator << (const mfxInfoMFX& p);
    tsTrace& operator << (const mfxPluginUID& p);
    tsTrace& operator << (const mfxFrameData& p);
    tsTrace& operator << (mfxStatus& p);
    tsTrace& operator << (const mfxStatus& p){
        return operator<<(const_cast<mfxStatus&>(p));
    }
    tsTrace& operator << (const mfxExtFeiEncMV& p);

    template<typename T> tsTrace& operator << (T& p) { (tsAutoTrace&)*this << p; return *this; }
    template<typename T> tsTrace& operator << (T* p) { (tsAutoTrace&)*this << p; return *this; }
};

typedef struct
{
    unsigned char* data;
    unsigned int size;
} rawdata;

rawdata hexstr(const void* d, unsigned int s);
std::ostream &operator << (std::ostream &os, rawdata p);
