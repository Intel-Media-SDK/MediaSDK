// Copyright (c) 2017-2020 Intel Corporation
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

#ifndef __MFXUTILS_H__
#define __MFXUTILS_H__

#include "mfx_config.h"

#include "mfxstructures.h"
#include "mfxplugin.h"

#include "umc_structures.h"
#include "mfx_trace.h"
#include "mfx_timing.h"

#include <cassert>
#include <cstddef>

#if defined(MFX_VA_LINUX)
#include <va/va.h>
#endif

#include "mfx_utils_defs.h"



static const mfxU32 MFX_TIME_STAMP_FREQUENCY = 90000; // will go to mfxdefs.h
static const mfxU64 MFX_TIME_STAMP_INVALID = (mfxU64)-1; // will go to mfxdefs.h
#define MFX_CHECK_UMC_STS(err)  { if (err != static_cast<int>(UMC::UMC_OK)) {return ConvertStatusUmc2Mfx(err);} }

inline
mfxStatus ConvertStatusUmc2Mfx(UMC::Status umcStatus)
{
    switch (umcStatus)
    {
    case UMC::UMC_OK: return MFX_ERR_NONE;
    case UMC::UMC_ERR_NULL_PTR: return MFX_ERR_NULL_PTR;
    case UMC::UMC_ERR_UNSUPPORTED: return MFX_ERR_UNSUPPORTED;
    case UMC::UMC_ERR_ALLOC: return MFX_ERR_MEMORY_ALLOC;
    case UMC::UMC_ERR_LOCK: return MFX_ERR_LOCK_MEMORY;
    case UMC::UMC_ERR_NOT_ENOUGH_BUFFER: return MFX_ERR_NOT_ENOUGH_BUFFER;
    case UMC::UMC_ERR_NOT_ENOUGH_DATA: return MFX_ERR_MORE_DATA;
    case UMC::UMC_ERR_SYNC: return MFX_ERR_MORE_DATA; // need to skip bad frames
    default: return MFX_ERR_ABORTED; // need general error code here
    }
}

inline
mfxF64 GetUmcTimeStamp(mfxU64 ts)
{
    return ts == MFX_TIME_STAMP_INVALID ? -1.0 : ts / (mfxF64)MFX_TIME_STAMP_FREQUENCY;
}

inline
mfxU64 GetMfxTimeStamp(mfxF64 ts)
{
    return ts < 0.0 ? MFX_TIME_STAMP_INVALID : (mfxU64)(ts * MFX_TIME_STAMP_FREQUENCY + .5);
}

inline
bool LumaIsNull(const mfxFrameSurface1 * surf)
{
#if (MFX_VERSION >= 1027)
    if (surf->Info.FourCC == MFX_FOURCC_Y410)
    {
        return !surf->Data.Y410;
    }
    else
#endif
    {
        return !surf->Data.Y;
    }
}

namespace mfx
{

template <class F>
struct TupleArgs;

template <typename TRes, typename... TArgs>
struct TupleArgs<TRes(TArgs...)>
{
    using type = std::tuple<TArgs...>;
};
template <typename TRes, typename... TArgs>
struct TupleArgs<TRes(*)(TArgs...)>
{
    using type = std::tuple<TArgs...>;
};

template<class T, T... args>
struct integer_sequence
{
    using value_type = T;
    static size_t size() { return (sizeof...(args)); }
};

template<size_t... args>
using index_sequence = mfx::integer_sequence<size_t, args...>;

template<size_t N, size_t ...S>
struct make_index_sequence_impl
    : make_index_sequence_impl<N - 1, N - 1, S...>
{};

template<size_t ...S>
struct make_index_sequence_impl<0, S...>
{
    using type = index_sequence<S...>;
};

template <class F>
struct result_of;

template <typename TRes, typename... TArgs>
struct result_of<TRes(TArgs...)> : std::result_of<TRes(TArgs...)> {};

template <typename TRes, typename... TArgs>
struct result_of<TRes(*const&)(TArgs...)>
{
    using type = TRes;
};

template<size_t S>
using make_index_sequence = typename make_index_sequence_impl<S>::type;

template<typename TFunc, typename TTuple, size_t ...S >
inline typename mfx::result_of<TFunc>::type
    apply_impl(TFunc&& fn, TTuple&& t, mfx::index_sequence<S...>)
{
    return fn(std::get<S>(t) ...);
}

template<typename TFunc, typename TTuple>
inline typename mfx::result_of<TFunc>::type
    apply(TFunc&& fn, TTuple&& t)
{
    return apply_impl(
        std::forward<TFunc>(fn)
        , std::forward<TTuple>(t)
        , typename mfx::make_index_sequence<std::tuple_size<typename std::remove_reference<TTuple>::type>::value>());
}

template<class T>
class IterStepWrapper
    : public std::iterator_traits<T>
{
public:
    using iterator_category = std::forward_iterator_tag;
    using iterator_type = IterStepWrapper;
    using reference = typename std::iterator_traits<T>::reference;
    using pointer = typename std::iterator_traits<T>::pointer;

    IterStepWrapper(T ptr, ptrdiff_t step = 1)
        : m_ptr(ptr)
        , m_step(step)
    {}
    iterator_type& operator++()
    {
        std::advance(m_ptr, m_step);
        return *this;
    }
    iterator_type operator++(int)
    {
        auto i = *this;
        ++(*this);
        return i;
    }
    reference operator*() { return *m_ptr; }
    pointer operator->() { return m_ptr; }
    bool operator==(const iterator_type& other)
    {
        return
            m_ptr == other.m_ptr
            || abs(std::distance(m_ptr, other.m_ptr)) < std::max(abs(m_step), abs(other.m_step));
    }
    bool operator!=(const iterator_type& other)
    {
        return !((*this) == other);
    }
private:
    T m_ptr;
    ptrdiff_t m_step;
};

template <class T>
inline IterStepWrapper<T> MakeStepIter(T ptr, ptrdiff_t step = 1)
{
    return IterStepWrapper<T>(ptr, step);
}

namespace options //MSDK API options verification utilities
{
    //Each Check... function return true if verification failed, false otherwise
    template <class T>
    inline bool Check(const T&)
    {
        return true;
    }

    template <class T, T val, T... other>
    inline bool Check(const T & opt)
    {
        if (opt == val)
            return false;
        return Check<T, other...>(opt);
    }

    template <class T, T val>
    inline bool CheckGE(T opt)
    {
        return !(opt >= val);
    }

    template <class T, class... U>
    inline bool Check(T & opt, T next, U... other)
    {
        if (opt == next)
            return false;
        return Check(opt, other...);
    }

    template <class T>
    inline bool CheckOrZero(T& opt)
    {
        opt = T(0);
        return true;
    }

    template <class T, T val, T... other>
    inline bool CheckOrZero(T & opt)
    {
        if (opt == val)
            return false;
        return CheckOrZero<T, other...>(opt);
    }

    template <class T, class... U>
    inline bool CheckOrZero(T & opt, T next, U... other)
    {
        if (opt == next)
            return false;
        return CheckOrZero(opt, (T)other...);
    }

    template <class T, class U>
    inline bool CheckMaxOrZero(T & opt, U max)
    {
        if (opt <= max)
            return false;
        opt = 0;
        return true;
    }

    template <class T, class U>
    inline bool CheckMinOrZero(T & opt, U min)
    {
        if (opt >= min)
            return false;
        opt = 0;
        return true;
    }

    template <class T, class U>
    inline bool CheckMaxOrClip(T & opt, U max)
    {
        if (opt <= max)
            return false;
        opt = T(max);
        return true;
    }

    template <class T, class U>
    inline bool CheckMinOrClip(T & opt, U min)
    {
        if (opt >= min)
            return false;
        opt = T(min);
        return true;
    }

    template <class T>
    inline bool CheckRangeOrSetDefault(T & opt, T min, T max, T dflt)
    {
        if (opt >= min && opt <= max)
            return false;
        opt = dflt;
        return true;
    }

    inline bool CheckTriState(mfxU16 opt)
    {
        return Check<mfxU16
            , MFX_CODINGOPTION_UNKNOWN
            , MFX_CODINGOPTION_ON
            , MFX_CODINGOPTION_OFF>(opt);
    }

    inline bool CheckTriStateOrZero(mfxU16& opt)
    {
        return CheckOrZero<mfxU16
            , MFX_CODINGOPTION_UNKNOWN
            , MFX_CODINGOPTION_ON
            , MFX_CODINGOPTION_OFF>(opt);
    }

    template<class TVal, class TArg, typename std::enable_if<!std::is_constructible<TVal, TArg>::value, int>::type = 0>
    inline TVal GetOrCall(TArg val) { return val(); }

    template<class TVal, class TArg, typename = typename std::enable_if<std::is_constructible<TVal, TArg>::value>::type>
    inline TVal GetOrCall(TArg val) { return TVal(val); }

    template<typename T, typename TF>
    inline bool SetDefault(T& opt, TF get_dflt)
    {
        if (opt)
            return false;
        opt = GetOrCall<T>(get_dflt);
        return true;
    }

    template<typename T, typename TF>
    inline bool SetIf(T& opt, bool bSet, TF get)
    {
        if (!bSet)
            return false;
        opt = GetOrCall<T>(get);
        return true;
    }

    template<class T, class TF, class... TA>
    inline bool SetIf(T& opt, bool bSet, TF&& get, TA&&... arg)
    {
        if (!bSet)
            return false;
        opt = get(std::forward<TA>(arg)...);
        return true;
    }

    template <class T>
    inline bool InheritOption(T optInit, T & optReset)
    {
        if (optReset == 0)
        {
            optReset = optInit;
            return true;
        }
        return false;
    }

    template<class TSrcIt, class TDstIt>
    TDstIt InheritOptions(TSrcIt initFirst, TSrcIt initLast, TDstIt resetFirst)
    {
        while (initFirst != initLast)
        {
            InheritOption(*initFirst++, *resetFirst++);
        }
        return resetFirst;
    }

    inline mfxU16 Bool2CO(bool bOptON)
    {
        return mfxU16(MFX_CODINGOPTION_OFF - !!bOptON * MFX_CODINGOPTION_ON);
    }

    template<class T>
    inline bool AlignDown(T& value, mfxU32 alignment)
    {
        assert((alignment & (alignment - 1)) == 0); // should be 2^n
        if (!(value & (alignment - 1))) return false;
        value = value & ~(alignment - 1);
        return true;
    }

    template<class T>
    inline bool AlignUp(T& value, mfxU32 alignment)
    {
        assert((alignment & (alignment - 1)) == 0); // should be 2^n
        if (!(value & (alignment - 1))) return false;
        value = (value + alignment - 1) & ~(alignment - 1);
        return true;
    }

    namespace frametype
    {
        inline bool IsIdr(mfxU32 type)
        {
            return !!(type & MFX_FRAMETYPE_IDR);
        }
        inline bool IsI(mfxU32 type)
        {
            return !!(type & MFX_FRAMETYPE_I);
        }
        inline bool IsB(mfxU32 type)
        {
            return !!(type & MFX_FRAMETYPE_B);
        }
        inline bool IsP(mfxU32 type)
        {
            return !!(type & MFX_FRAMETYPE_P);
        }
        inline bool IsRef(mfxU32 type)
        {
            return !!(type & MFX_FRAMETYPE_REF);
        }
    }
}
}

#if defined(MFX_VA_LINUX)
inline mfxStatus CheckAndDestroyVAbuffer(VADisplay display, VABufferID & buffer_id)
{
    if (buffer_id != VA_INVALID_ID)
    {
        VAStatus vaSts = vaDestroyBuffer(display, buffer_id);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        buffer_id = VA_INVALID_ID;
    }

    return MFX_ERR_NONE;
}
#endif

#define MFX_DECL_OPERATOR_NOT_EQ(Name)                      \
static inline bool operator!=(Name const& l, Name const& r) \
{                                                           \
    return !(l == r);                                       \
}

static inline bool operator==(mfxPluginUID const& l, mfxPluginUID const& r)
{
    return MFX_EQ_ARRAY(Data, 16);
}

MFX_DECL_OPERATOR_NOT_EQ(mfxPluginUID)



#endif // __MFXUTILS_H__
