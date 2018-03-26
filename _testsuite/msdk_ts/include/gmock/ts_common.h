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

#include "atomic_defs.h"
#include "gtest/gtest.h"
#include "ts_trace.h"
#include "ts_plugin.h"
#include "ts_streams.h"
#include "test_sample_allocator.h"
#include "ts_platforms.h"
#include "ts_ext_buffers.h"

typedef enum {
      tsOK = 0
    , tsFAIL
    , tsSKIP
} tsRes;

typedef struct {
    mfxU32      lowpower;
    bool        sim;
    mfxU16      GPU_copy_mode;
    std::string cfg_filename;
} tsConfig;

typedef struct {
    mfxU32 BufferId;
    mfxU32 BufferSz;
    const char *string;
} BufferIdToString;

#define EXTBUF(TYPE, ID) {ID, sizeof(TYPE), #TYPE},
static BufferIdToString g_StringsOfBuffers[] =
{
#include "ts_ext_buffers_decl.h"
};
#undef EXTBUF

class MFXVideoTest : public ::testing::TestWithParam<unsigned int>
{
    void SetUp();
    void TearDown();
};

class tsStatus
{
public:
    mfxStatus m_status;
    mfxStatus m_expected;
    bool      m_failed;
    bool      m_throw_exceptions;
    bool      m_last;
    mfxU16    m_disable;

    tsStatus();
    ~tsStatus();

    void reset();

    bool check(mfxStatus status);
    bool check();
    inline void      expect (mfxStatus expected) { m_expected = expected; }
    inline mfxStatus get    ()                   { return m_status; }
    inline void      disable_next_check()        { m_disable = 1; }
    inline void      disable()                   { m_disable = 2; }
    inline void      enable()                    { m_disable = 0; }
    inline void      last(bool bLast = true)     { m_last = bLast; }
};

extern mfxIMPL      g_tsImpl;
extern mfxVersion   g_tsVersion;
extern HWType       g_tsHWtype;
extern OSFamily     g_tsOSFamily;
extern OSWinVersion g_tsWinVersion;
extern bool         g_tsIsSSW;
extern tsStatus     g_tsStatus;
extern mfxU32       g_tsTrace;
extern tsTrace      g_tsLog;
extern tsPlugin     g_tsPlugin;
extern tsStreamPool g_tsStreamPool;
extern tsConfig     g_tsConfig;


#define _TS_REG_TEST_SUITE(name, routine, n_cases, q_mode) \
    class name : public MFXVideoTest{};\
    TEST_P(name,) { g_tsStreamPool.Init(q_mode); if((routine)(GetParam())) {ADD_FAILURE();} g_tsStreamPool.Close(); }\
    INSTANTIATE_TEST_CASE_P(, name, ::testing::Range<unsigned int>(0, (n_cases)));

#define TS_REG_TEST_SUITE(name, routine, n_cases) \
    _TS_REG_TEST_SUITE(name, routine, n_cases, false)\
    _TS_REG_TEST_SUITE(query_streams_##name, routine, n_cases, true)

#define _TS_REG_TEST_SUITE_CLASS(name, routine, n_cases, q_mode)\
    class name : public MFXVideoTest{};\
    TEST_P(name,) { g_tsStreamPool.Init(q_mode); TestSuite ts; if(ts.routine(GetParam())) {ADD_FAILURE();} g_tsStreamPool.Close(); }\
    INSTANTIATE_TEST_CASE_P(, name, ::testing::Range<unsigned int>(0, TestSuite::n_cases));

#define QUERY_STREAMS(name)  query_streams_##name

#define TS_REG_TEST_SUITE_CLASS_ROUTINE(name, routine, n_cases)\
    _TS_REG_TEST_SUITE_CLASS(name, routine, n_cases, false)\
    _TS_REG_TEST_SUITE_CLASS(QUERY_STREAMS(name), routine, n_cases, true)

#define TS_REG_TEST_SUITE_CLASS(name)\
    TS_REG_TEST_SUITE_CLASS_ROUTINE(name, RunTest, n_cases)

#define INC_PADDING() g_tsLog.inc_offset();
#define DEC_PADDING() g_tsLog.dec_offset();
#define TRACE_FUNCN(n, name, p1, p2, p3, p4, p5, p6, p7)\
    g_tsLog << "---------------------------------START-----------------------------------------\n";\
    if(g_tsTrace){\
        g_tsLog << "CALL: " << #name << "\n";\
        INC_PADDING();\
        if((n) > 0) g_tsLog << g_tsLog.m_off << #p1 " = " << p1 << "\n";\
        if((n) > 1) g_tsLog << g_tsLog.m_off << #p2 " = " << p2 << "\n";\
        if((n) > 2) g_tsLog << g_tsLog.m_off << #p3 " = " << p3 << "\n";\
        if((n) > 3) g_tsLog << g_tsLog.m_off << #p4 " = " << p4 << "\n";\
        if((n) > 4) g_tsLog << g_tsLog.m_off << #p5 " = " << p5 << "\n";\
        if((n) > 5) g_tsLog << g_tsLog.m_off << #p6 " = " << p6 << "\n";\
        if((n) > 8) g_tsLog << g_tsLog.m_off << #p7 " = " << p6 << "\n";\
        DEC_PADDING();\
    }\
    g_tsLog << "CALL: " << #name << "(";\
    if((n) > 0) g_tsLog << #p1;\
    if((n) > 1) g_tsLog << ", " << #p2;\
    if((n) > 2) g_tsLog << ", " << #p3;\
    if((n) > 3) g_tsLog << ", " << #p4;\
    if((n) > 4) g_tsLog << ", " << #p5;\
    if((n) > 5) g_tsLog << ", " << #p6;\
    if((n) > 6) g_tsLog << ", " << #p7;\
    g_tsLog << ");" << "\n";
#define TRACE_FUNC6(name, p1, p2, p3, p4, p5, p6) TRACE_FUNCN(6, name, p1, p2, p3, p4, p5, p6, 0)
#define TRACE_FUNC5(name, p1, p2, p3, p4, p5) TRACE_FUNCN(5, name, p1, p2, p3, p4, p5, 0, 0)
#define TRACE_FUNC4(name, p1, p2, p3, p4) TRACE_FUNCN(4, name, p1, p2, p3, p4, 0, 0, 0)
#define TRACE_FUNC3(name, p1, p2, p3) TRACE_FUNCN(3, name, p1, p2, p3, 0, 0, 0, 0)
#define TRACE_FUNC2(name, p1, p2) TRACE_FUNCN(2, name, p1, p2, 0, 0, 0, 0, 0)
#define TRACE_FUNC1(name, p1) TRACE_FUNCN(1, name, p1, 0, 0, 0, 0, 0, 0)
#define TRACE_FUNC0(name) TRACE_FUNCN(0, name, 0, 0, 0, 0, 0, 0, 0)
#define TS_TRACE(par)   if(g_tsTrace) {INC_PADDING(); g_tsLog << g_tsLog.m_off << #par " = " << par << "\n"; DEC_PADDING();}
#define TS_CHECK        { if(g_tsStatus.m_failed) return 1; }
#define TS_CHECK_MFX    { if(g_tsStatus.m_failed) return g_tsStatus.get(); }
#define TS_FAIL_TEST(err, sts) { g_tsLog << "ERROR: " << err << "\n"; ADD_FAILURE(); return sts; }
#define TS_MAX(x,y) ((x)>(y) ? (x) : (y))
#define TS_MIN(x,y) ((x)<(y) ? (x) : (y))
#define CEIL_DIV(x,y) ((x + y - 1) / y)
#define TS_START try {
#define TS_END   } catch(tsRes r)                             \
    {                                                         \
        if (r == tsSKIP)                                      \
        {                                                     \
            g_tsLog << "[  SKIPPED ] test-case was skipped\n";\
            return 0;                                         \
        }                                                     \
        return r;                                             \
    }

#define TS_HW_ALLOCATOR_TYPE (!!((g_tsImpl) & 0xF00) ? frame_allocator::HARDWARE_DX11 : frame_allocator::HARDWARE)

#define MAX_NPARS 40

#define SETPARS(p, type)                                                                                        \
for(mfxU32 i = 0; i < MAX_NPARS; i++)                                                                           \
{                                                                                                               \
    if(tc.set_par[i].f && tc.set_par[i].ext_type == type)                                                       \
    {                                                                                                           \
        SetParam(p, tc.set_par[i].f->name, tc.set_par[i].f->offset, tc.set_par[i].f->size, tc.set_par[i].v);    \
    }                                                                                                           \
}

std::string ENV(const char* name, const char* def);

void set_brc_params(tsExtBufType<mfxVideoParam>* p);
void set_chromaformat(mfxFrameInfo& frameinfo);
void set_chromaformat_mfx(tsExtBufType<mfxVideoParam>* p);
void set_chromaformat_vpp(tsExtBufType<mfxVideoParam>* p);

bool operator == (const mfxFrameInfo&, const mfxFrameInfo&);
bool operator == (const mfxFrameData&, const mfxFrameData&);

void GetBufferIdSz(const std::string& name, mfxU32& bufId, mfxU32& bufSz);

void SetParam(void* base, const std::string name, const mfxU32 offset, const mfxU32 size, mfxU64 value);

template <typename T>
void SetParam(tsExtBufType<T> *base, const std::string name, const mfxU32 offset, const mfxU32 size, mfxU64 value)
{
    assert(0 != base);
    if(base)    return SetParam(*base, name, offset, size, value);
}
template <typename T>
void SetParam(tsExtBufType<T>& base, const std::string name, const mfxU32 offset, const mfxU32 size, mfxU64 value);


namespace tsStruct
{
    class Field;
}

enum CompareLog
{
    LOG_NOTHING,
    LOG_MISMATCH,
    LOG_EVERYTHING
};

bool CompareParam(void* base, const tsStruct::Field& field, mfxU64 value);

template <typename T>
bool CompareParam(tsExtBufType<T> *base, const tsStruct::Field& field, mfxU64 value, CompareLog log = LOG_MISMATCH);

template <typename T>
bool CompareParam(tsExtBufType<T>& base, const tsStruct::Field& field, mfxU64 value, CompareLog log = LOG_MISMATCH);

template <typename TC_STRUCT, typename TYPE_ENUM, typename T>
bool CompareParams(TC_STRUCT tc, T p, TYPE_ENUM type, CompareLog log = LOG_MISMATCH)
{
    bool match = true;
    for (mfxU32 i = 0; i < sizeof(tc.set_par)/sizeof(tc.set_par[0]); i++)
    {
        if (tc.set_par[i].f && tc.set_par[i].ext_type == type)
        {
            bool m = CompareParam(p, *(tc.set_par[i].f), tc.set_par[i].v, log);
            if (log == LOG_EVERYTHING || (m == false && log == LOG_MISMATCH))
            {
                g_tsLog << "WARNING: CompareParams[" << i << "]." << tc.set_par[i].f->name << ( m ? "==":"!=" ) << tc.set_par[i].v << "\n";
            }
            match = match && m;
        }
    }
    return match;
}

#define COMPAREPARS(p, type)  CompareParams(tc, p, type)

//Interrupt test-case execution and return FAILED immediately if at least one check failed (EXPECT_EQ etc.)
inline void BreakOnFailure() { if (testing::Test::HasFailure()) throw tsFAIL; }