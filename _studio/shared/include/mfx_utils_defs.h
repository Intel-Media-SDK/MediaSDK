// Copyright (c) 2019-2020 Intel Corporation
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

#ifndef __MFX_ENCTOOLS_DEFS_H__
#define __MFX_ENCTOOLS_DEFS_H__

#include "mfxdefs.h"
#include <vector>
#include <memory>
#include <assert.h>

#ifndef MFX_DEBUG_TRACE
#define MFX_STS_TRACE(sts) sts
#else
template <typename T>
static inline T mfx_print_err(T sts, const char *file, int line, const char *func)
{
    if (sts)
    {
        printf("%s: %d: %s: Error = %d\n", file, line, func, sts);
    }
    return sts;
}
#define MFX_STS_TRACE(sts) mfx_print_err(sts, __FILE__, __LINE__, __FUNCTION__)
#endif

#define MFX_SUCCEEDED(sts)      (MFX_STS_TRACE(sts) == MFX_ERR_NONE)
#define MFX_FAILED(sts)         (MFX_STS_TRACE(sts) != MFX_ERR_NONE)
#define MFX_RETURN(sts)         { return MFX_STS_TRACE(sts); }
#define MFX_CHECK(EXPR, ERR)    { if (!(EXPR)) MFX_RETURN(ERR); }

#define MFX_CHECK_NO_RET(EXPR, STS, ERR){ if (!(EXPR)) { std::ignore = MFX_STS_TRACE(ERR); STS = ERR; } }

#define MFX_CHECK_STS(sts)              MFX_CHECK(MFX_SUCCEEDED(sts), sts)
#define MFX_SAFE_CALL(FUNC)             { mfxStatus _sts = FUNC; MFX_CHECK_STS(_sts); }
#define MFX_CHECK_NULL_PTR1(pointer)    MFX_CHECK(pointer, MFX_ERR_NULL_PTR)
#define MFX_CHECK_NULL_PTR2(p1, p2)     { MFX_CHECK(p1, MFX_ERR_NULL_PTR); MFX_CHECK(p2, MFX_ERR_NULL_PTR); }
#define MFX_CHECK_NULL_PTR3(p1, p2, p3) { MFX_CHECK(p1, MFX_ERR_NULL_PTR); MFX_CHECK(p2, MFX_ERR_NULL_PTR); MFX_CHECK(p3, MFX_ERR_NULL_PTR); }
#define MFX_CHECK_STS_ALLOC(pointer)    MFX_CHECK(pointer, MFX_ERR_MEMORY_ALLOC)
#define MFX_CHECK_COND(cond)            MFX_CHECK(cond, MFX_ERR_UNSUPPORTED)
#define MFX_CHECK_INIT(InitFlag)        MFX_CHECK(InitFlag, MFX_ERR_MORE_DATA)
#define MFX_CHECK_HDL(hdl)              MFX_CHECK(hdl,      MFX_ERR_INVALID_HANDLE)

#define MFX_CHECK_UMC_ALLOC(err)     { if (err != true) {return MFX_ERR_MEMORY_ALLOC;} }
#define MFX_CHECK_EXBUF_INDEX(index) { if (index == -1) {return MFX_ERR_MEMORY_ALLOC;} }

#define MFX_CHECK_WITH_ASSERT(EXPR, ERR) {assert(EXPR); MFX_CHECK(EXPR,ERR); }
#define MFX_CHECK_WITH_THROW(EXPR, ERR, EXP)  { if (!(EXPR)) { std::ignore = MFX_STS_TRACE(ERR); throw EXP; } }

static const mfxU32 NO_INDEX = 0xffffffff;
static const mfxU8  NO_INDEX_U8 = 0xff;
static const mfxU16 NO_INDEX_U16 = 0xffff;

#if __cplusplus > 201703L
#define MFX_FALLTHROUGH [[fallthrough]]
#elif __clang__
#define MFX_FALLTHROUGH [[clang::fallthrough]]
#else
#define MFX_FALLTHROUGH
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(PTR)   { if (PTR) { PTR->Release(); PTR = NULL; } }
#endif

#ifdef MFX_ENABLE_CPLIB
    #define IS_PROTECTION_CENC(val) (MFX_PROTECTION_CENC_WV_CLASSIC == (val) || MFX_PROTECTION_CENC_WV_GOOGLE_DASH == (val))
#else
    #define IS_PROTECTION_CENC(val) (false)
#endif

#define IS_PROTECTION_ANY(val) IS_PROTECTION_CENC(val)


inline bool IsOn(mfxU32 opt)
{
    return opt == MFX_CODINGOPTION_ON;
}

inline bool IsOff(mfxU32 opt)
{
    return opt == MFX_CODINGOPTION_OFF;
}

inline bool IsAdapt(mfxU32 opt)
{
    return opt == MFX_CODINGOPTION_ADAPTIVE;
}

namespace mfx
{
    // TODO: switch to std::clamp when C++17 support will be enabled

    // Clip value v to range [lo, hi]
    template<class T>
    constexpr const T& clamp(const T& v, const T& lo, const T& hi)
    {
        return std::min(hi, std::max(v, lo));
    }

    // Comp is comparison function object with meaning of 'less' operator (i.e. std::less<> or operator<)
    template<class T, class Compare>
    constexpr const T& clamp(const T& v, const T& lo, const T& hi, Compare comp)
    {
        return comp(v, lo) ? lo : comp(hi, v) ? hi : v;
    }

    // Aligns value to next power of two
    template<class T> inline
    T align2_value(T value, size_t alignment = 16)
    {
        assert((alignment & (alignment - 1)) == 0);
        return static_cast<T> ((value + (alignment - 1)) & ~(alignment - 1));
    }

    template <class T>
    constexpr size_t size(const T& c)
    {
        return (size_t)c.size();
    }

    template <class T, size_t N>
    constexpr size_t size(const T(&)[N])
    {
        return N;
    }

    inline mfxU32 CeilLog2(mfxU32 x)
    {
        mfxU32 l = 0;
        while (x > (1U << l))
            ++l;
        return l;
    }
}

#define MFX_COPY_FIELD(Field)       buf_dst.Field = buf_src.Field
#define MFX_COPY_ARRAY_FIELD(Array) std::copy(std::begin(buf_src.Array), std::end(buf_src.Array), std::begin(buf_dst.Array))

#define MFX_EQ_FIELD(Field) l.Field == r.Field
#define MFX_EQ_ARRAY(Array, Num) std::equal(l.Array, l.Array + Num, r.Array)

#endif
