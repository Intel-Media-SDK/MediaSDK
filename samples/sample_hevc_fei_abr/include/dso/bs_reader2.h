// Copyright (c) 2018-2020 Intel Corporation
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

#include "bs_def.h"
#include <stdio.h>
#include <vector>
#include <exception>
#include <assert.h>

namespace BsReader2
{

class Exception : public std::exception
{
private:
    BSErr m_sts;
public:
    Exception(BSErr sts)
        : std::exception()
        , m_sts(sts)
    { /*assert(!"Error in bs_parser!");*/ }
    inline BSErr Err() { return m_sts; }
};

class EndOfBuffer : public Exception
{
public:
    EndOfBuffer() : Exception(BS_ERR_MORE_DATA) {}
};

class InvalidSyntax : public Exception
{
public:
    InvalidSyntax() : Exception(BS_ERR_UNKNOWN) { /*assert(!"invalid syntax!");*/ }
};

class BufferUpdater
{
public:
    virtual bool UpdateBuffer(Bs8u*& start, Bs8u*& cur, Bs8u*& end, Bs32u keepBytes) = 0;
    virtual ~BufferUpdater() {};
};

class File : public BufferUpdater
{
private:
    FILE* m_f;
    std::vector<Bs8u> m_b;
    static const Bs32u DEFAULT_BUFF_SIZE = 2048;
public:
    File();
    ~File();

    bool Open(const char* file, Bs32u buffSize = DEFAULT_BUFF_SIZE);
    void Close();
    bool UpdateBuffer(Bs8u*& start, Bs8u*& cur, Bs8u*& end, Bs32u keepBytes);
};

#ifdef __BS_TRACE__

#define BS2_TRO { if (TraceOffset()) fprintf(GetLog(), "0x%016llX[%i]: ", GetByteOffset(), GetBitOffset()); }

#define BS2_SET(val, var)                         \
{                                                 \
    if (Trace()) {                                \
        BS2_TRO;                                  \
        var = (val);                              \
        fprintf(GetLog(), std::is_same<Bs64u, decltype(var)>::value ? "%s = %lli\n" : "%s = %i\n", #var, var);\
        fflush(GetLog());                         \
    } else { var = (val); }                       \
}

#define BS2_SETM(val, var, map)                                  \
{                                                                \
    if (Trace()) {                                               \
        BS2_TRO;                                                 \
        var = (val);                                             \
        fprintf(GetLog(), "%s = %s(%d)\n", #var, map[var], var); \
        fflush(GetLog());                                        \
    } else { var = (val); }                                      \
}

#define BS2_TRACE(val, var)                         \
{                                                   \
    if (Trace()) {                                  \
        BS2_TRO;                                    \
        fprintf(GetLog(), std::is_same<Bs64u, decltype(val)>::value ? "%s = %lli\n" : "%s = %i\n", #var, (val));\
        fflush(GetLog()); }                         \
    else { (val); }                                 \
}

#define BS2_TRACE_STR(str)\
    { if (Trace()) { BS2_TRO; fprintf(GetLog(), "%s\n", (str)); fflush(GetLog());} }

#define BS2_SET_ARR_F(val, var, sz, split, format)                    \
{                                                                     \
    if (Trace()){                                                     \
            BS2_TRO;                                                  \
            fprintf(GetLog(), "%s = { ", #var);                       \
            for (Bs32u _i = 0; _i < Bs32u(sz); _i++){                 \
                if ((split) != 0 && _i % (split) == 0)                \
                    fprintf(GetLog(), "\n");                          \
                (var)[_i] = (val);                                    \
                fprintf(GetLog(), format, (var)[_i]);                 \
            }                                                         \
            fprintf(GetLog(), "}\n"); fflush(GetLog());               \
    } else { for (Bs32u _i = 0; _i < (sz); _i++) (var)[_i] = (val); } \
}

#define BS2_SET_ARR_M(val, var, sz, split, format, map)              \
{                                                                    \
    if (Trace()){                                                    \
            BS2_TRO;                                                 \
            fprintf(GetLog(), "%s = { ", #var);                      \
            for (Bs32u _i = 0; _i < Bs32u(sz); _i++) {               \
                if ((split) != 0 && _i % (split) == 0)               \
                    fprintf(GetLog(), "\n");                         \
                (var)[_i] = (val);                                   \
                fprintf(GetLog(), format, map[(var)[_i]], (var)[_i]);\
            }                                                        \
            fprintf(GetLog(), "}\n"); fflush(GetLog());              \
    }                                                                \
    else { for (Bs32u _i = 0; _i < (sz); _i++) (var)[_i] = (val); }  \
}

#define BS2_TRACE_ARR_VF(val, var, sz, split, format)  \
{                                                      \
    if (Trace()){                                      \
            BS2_TRO;                                   \
            fprintf(GetLog(), "%s = { ", #var);        \
            for (Bs32u _i = 0; _i < Bs32u(sz); _i++) { \
                if ((split) != 0&& _i % (split) == 0)  \
                    fprintf(GetLog(), "\n");           \
                fprintf(GetLog(), format, (val));      \
            }                                          \
            fprintf(GetLog(), "}\n"); fflush(GetLog());\
    }                                                  \
    else { for (Bs32u _i = 0; _i < (sz); _i++) (val); }\
}

#define BS2_TRACE_MDARR(type, arr, dim, split, split2, format)\
{                                                    \
    if (Trace()){                                    \
        BS2_TRO;                                     \
        fprintf(GetLog(), "%s = \n", #arr);          \
        traceMDArr<type, sizeof(dim) / sizeof(Bs16u)>\
                (arr, dim, split, format, split2);   \
        fflush(GetLog());                            \
    }                                                \
}

#define BS2_TRACE_BIN(pval, off, sz, var)    \
{                                            \
    if (Trace()) {                           \
            BS2_TRO;                         \
            fprintf(GetLog(), "%s = ", #var);\
            traceBin(pval, off, sz);         \
            fprintf(GetLog(), "\n");         \
            fflush(GetLog());                \
    }                                        \
    else { (pval); }                         \
}

#else

#define BS2_TRO
#define BS2_SET(val, var) { var = (val); }
#define BS2_TRACE(val, var) {(void)(val);}
#define BS2_TRACE_STR(str) {(void)(str);}
#define BS2_SET_ARR_F(val, var, sz, split, format) \
    { for (Bs32u _i = 0; _i < (Bs32u)(sz); _i++) (var)[_i] = (val); }
#define BS2_TRACE_ARR_VF(val, var, sz, split, format) \
    { for (Bs32u _i = 0; _i < (Bs32u)(sz); _i++) (void)(val); }
#define BS2_TRACE_MDARR(type, arr, dim, split, split2, format) {(void)(arr);}
#define BS2_SET_ARR_M(val, var, sz, split, format, map) BS2_SET_ARR_F(val, var, sz, split, format)
#define BS2_SETM(val, var, map) BS2_SET(val, var)
#define BS2_TRACE_BIN(pval, off, sz, var) { (void)(pval); }

#endif

#define BS2_SET_ARR(val, var, sz, split) BS2_SET_ARR_F(val, var, sz, split, "%i ")
#define BS2_TRACE_ARR_F(var, sz, split, format) BS2_TRACE_ARR_VF(var[_i], var, sz, split, format)
#define BS2_TRACE_ARR(var, sz, split) BS2_TRACE_ARR_F(var, sz, split, "%i ")

struct State
{
    BufferUpdater* m_updater;
    Bs8u* m_bsStart;
    Bs8u* m_bsEnd;
    Bs8u* m_bs;
    Bs8u  m_bitStart;
    Bs8u  m_bitOffset;
    bool  m_emulation;
    Bs32u m_emuBytes;
    Bs64u m_startOffset;
    bool  m_trace;
    bool  m_traceOffset;
    Bs32u m_traceLevel;
    FILE* m_log;
};

class Reader : private State
{
public:
    Reader();
    ~Reader();

    Bs32u GetBit();
    Bs32u GetBits(Bs32u n);
    void  GetBits(void* b, Bs32u o, Bs32u n);
    Bs32u GetUE();
    Bs32s GetSE();
    Bs32u GetByte(); // ignore bit offset
    Bs32u GetBytes(Bs32u n, bool nothrow = false); // ignore bit offset

    void ReturnByte();
    void ReturnBits(Bs32u n);

    bool NextStartCode(bool stopBefore);

    bool TrailingBits(bool stopBefore = false);
    bool NextBytes(Bs8u* b, Bs32u n);

    inline Bs32u NextBits(Bs32u n)
    {
        Bs32u b = GetBits(n);
        ReturnBits(n);
        return b;
    }

    Bs32u ExtractRBSP(Bs8u* buf, Bs32u size);
    Bs32u ExtractData(Bs8u* buf, Bs32u size);

    inline Bs64u GetByteOffset() { return m_startOffset + (m_bs - m_bsStart); }
    inline Bs16u GetBitOffset() { return m_bitOffset; }

    inline void SetEmulation(bool f) { m_emulation = f; };
    inline bool GetEmulation() { return m_emulation; };

    inline void SetEmuBytes(Bs32u f) { m_emuBytes = f; };
    inline Bs32u GetEmuBytes() { return m_emuBytes; };

    void Reset(Bs8u* bs = 0, Bs32u size = 0, Bs8u bitOffset = 0);
    void Reset(BufferUpdater* reader) { m_updater = reader; m_bsEnd = 0; m_startOffset = 0; }

    void SaveState(State& st){ st = *this; };
    void LoadState(State& st){ *(State*)this = st; };

#ifdef __BS_TRACE__
    inline bool Trace() { return m_trace; }
    inline bool TraceOffset() { return m_traceOffset; }
    inline void TLStart(Bs32u level) { m_trace = !!(m_traceLevel & level); m_tln++; m_tla |= (Bs64u(m_trace) << m_tln); };
    inline void TLEnd() { m_tla &= ~(Bs64u(1) << m_tln); m_tln--; m_trace = !!(1 & (m_tla >> m_tln)); };
    inline bool TLTest(Bs32u tl) { return !!(m_traceLevel & tl); }
    inline void SetTraceLevel(Bs32u level) { m_traceLevel = level; m_traceOffset = !!(level & 0x80000000); };
#else
    inline bool Trace() { return false; }
    inline bool TraceOffset() { return false; }
    inline void TLStart(Bs32u /*level*/) { };
    inline void TLEnd() {};
    inline bool TLTest(Bs32u /*tl*/) { return false; }
    inline void SetTraceLevel(Bs32u /*level*/) { };
#endif

    inline FILE* GetLog() { return m_log; }
    inline void SetLog(FILE* log) { m_log = log; }

    template<class T, Bs32u depth> void traceMDArr(
        const void* arr,
        const Bs16u* sz,
        Bs16u split,
        const char* format,
        Bs16u split2 = 0)
    {
        Bs16u off[depth];
        for (Bs32u i = 0; i < depth; i++)
        {
            off[i] = 1;
            for (Bs32u j = i + 1; j < depth; j++)
                off[i] *= sz[j];
        }
        for (Bs32u i = 0, j = 0; i < Bs32u(off[0] * sz[0]); i++)
        {
            while (i % off[j] == 0)
            {
                fprintf(GetLog(), "{ ");

                if (j < (split))
                {
                    fprintf(GetLog(), "\n");
                    for (Bs32u c = 0; c <= j; c++)
                        fprintf(GetLog(), "  ");
                }
                if (j == depth - 2)
                    break;
                j++;
            }
            fprintf(GetLog(), format, ((const T*)arr)[i]);
            if (split2 && (i + 1) % split2 == 0
                && (i + 1) / split2 > 0
                && (i + 1) % off[j] != 0)
            {
                fprintf(GetLog(), "\n");
                for (Bs32u c = 0; c <= j; c++)
                    fprintf(GetLog(), "  ");
            }
            while ((i + 1) % off[j] == 0)
            {
                bool f = j ? (i + 1) % off[--j] == 0 : !!j--;
                fprintf(GetLog(), "} ");
                if (j < (split))
                {
                    fprintf(GetLog(), "\n");
                    for (Bs32u c = 0; c + f <= j; c++)
                        fprintf(GetLog(), "  ");
                }
                if (!f)
                {
                    j++;
                    break;
                }
            }
        }
        fprintf(GetLog(), "\n");
    }

    template<class T> void traceBin(T* p, Bs32u off, Bs32u sz)
    {
        Bs32u S0 = sizeof(T) * 8;
        Bs32u O = off;
        T* P0 = p;

        if (off + sz > BS_MAX(S0, 32))
        {
            fprintf(GetLog(), "\n");

            for (Bs32u i = 0; i < off; i++)
                fprintf(GetLog(), " ");
        }

        while (sz)
        {
            Bs32u S = S0 - O;
            Bs32u S1 = S0 - (O + BS_MIN(sz, S));

            sz -= (S - S1);

            while (S-- > S1)
                fprintf(GetLog(), "%d", !!(*p & (1 << S)));

            p++;
            O = 0;

            if (sz)
            {
                if ((p - P0) * S0 % 32 == 0)
                    fprintf(GetLog(), "\n");
                else
                    fprintf(GetLog(), " ");
            }
        }
    }

private:
    Bs64u m_tla;
    Bs32u m_tln;

    void MoreData(Bs32u keepBytes = 4);
    bool MoreDataNoThrow(Bs32u keepBytes = 4);
};

class TLAuto
{
private:
    Reader& m_r;
public:
    TLAuto(Reader& r, Bs32u tl) : m_r(r) { m_r.TLStart(tl); }
    ~TLAuto() { m_r.TLEnd(); }
};

class StateSwitcher
{
private:
    State m_st;
    Reader& m_r;
public:
    StateSwitcher(Reader& r)
        : m_r(r)
    {
        m_r.SaveState(m_st);
    }

    ~StateSwitcher()
    {
        m_r.LoadState(m_st);
    }
};

}