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

#pragma once

#include "mfx_common.h"

#include <array>
#include <memory>
#include <map>
#include <exception>
#include <algorithm>

namespace MfxExtBuffer
{
    template<class T> struct IdMap {};
    template<mfxU32 ID> struct TypeMap {};
#define EXTBUF(TYPE, ID)\
    template<> struct IdMap <TYPE> : std::integral_constant<mfxU32, ID> {};\
    template<> struct TypeMap <ID> { using Type = TYPE; };

    #include "../mediasdk_structures/ts_ext_buffers_decl.h"
#undef EXTBUF

    const mfxU32 IdSizePairs[][2] = {
#define EXTBUF(TYPE, ID) {mfxU32(ID), mfxU32(sizeof(TYPE))},
    #include "../mediasdk_structures/ts_ext_buffers_decl.h"
#undef EXTBUF
    };

    inline mfxU32 IdToSize(mfxU32 Id)
    {
        auto IsIdEq = [Id](const mfxU32 (&p)[2]) { return p[0] == Id; };
        auto pIt    = std::find_if(std::begin(IdSizePairs), std::end(IdSizePairs), IsIdEq);
        if (pIt == std::end(IdSizePairs))
            throw std::logic_error("unknown ext. buffer Id");
        return pIt[0][1];
    }

    template<class T> void Init(T& buf)
    {
        buf = T{};
        mfxExtBuffer& header = *((mfxExtBuffer*)&buf);
        header.BufferId = IdMap<T>::value;
        header.BufferSz = sizeof(T);
    }

    class ParamBase
    {
    public:
        using TEBMap = std::map<mfxU32, std::unique_ptr<mfxU8[]>>;
        using TEBIt  = TEBMap::iterator;

        ParamBase() = default;

        ParamBase(mfxExtBuffer** ExtParam, mfxU32 NumExtParam)
        {
            std::for_each(ExtParam, ExtParam + NumExtParam
                , [&](mfxExtBuffer* p) { _ConstructEB(p); });
        }

        mfxExtBuffer* Get(mfxU32 id) const
        {
            auto it = m_eb.find(id);
            return (it == m_eb.end()) ? nullptr : (mfxExtBuffer*)m_eb.at(id).get();
        }

    protected:
        TEBMap m_eb;

        std::pair<TEBIt, bool> _AllocEB(mfxU32 id, mfxU32 sz = 0, bool bReset = true)
        {
            auto it = m_eb.find(id);
            mfxU8* p;

            if (m_eb.end() == it)
            {
                if (!sz)
                    sz = IdToSize(id);

                p = new mfxU8[sz];

                memset(p, 0, sz);
                *(mfxExtBuffer*)p = { id, sz };

                return m_eb.emplace(id, std::unique_ptr<mfxU8[]>(p));
            }
            else if (bReset)
            {
                p = it->second.get();
                sz = ((mfxExtBuffer*)p)->BufferSz;
                memset(p, 0, sz);
                *(mfxExtBuffer*)p = { id, sz };
            }

            return std::make_pair(it, false);
        }

        std::pair<TEBIt, bool> _ConstructEB(const mfxExtBuffer* src)
        {
            if (src)
            {
                auto pair = _AllocEB(src->BufferId, src->BufferSz, false);
                auto dst = pair.first->second.get();
                if ((mfxU8*)src != dst)
                    std::copy_n((mfxU8*)src, src->BufferSz, dst);
                return pair;
            }
            return std::make_pair(m_eb.end(), false);
        }

    };

    template<class T>
    class Param
        : public ParamBase
        , public T
    {
    public:
        Param()
            : T({})
        {
            T::ExtParam = m_ExtParam.data();
        }

        Param(const T& par)
            : ParamBase(par.ExtParam, par.NumExtParam)
            , T(par)
        {
            T::NumExtParam = mfxU16(m_eb.size());
            T::ExtParam = m_ExtParam.data();
            auto GetEBPtr = [](TEBMap::reference ref) { return (mfxExtBuffer*)ref.second.get(); };
            std::transform(m_eb.begin(), m_eb.end(), m_ExtParam.begin(), GetEBPtr);
        }

        mfxExtBuffer* NewEB(mfxU32 id, bool bReset = true)
        {
            auto res = _AllocEB(id, 0, bReset);
            auto pEB = (mfxExtBuffer*)res.first->second.get();

            if (res.second)
                m_ExtParam.at(T::NumExtParam++) = pEB;

            return pEB;
        }
    protected:
        std::array<mfxExtBuffer*, 64> m_ExtParam;
    };

    class CastExtractor
    {
    private:
        mfxExtBuffer**      m_b;
        mfxU16              m_n;
        const ParamBase*    m_p;

        mfxExtBuffer* _Get(mfxU32 id) const
        {
            if (m_p)
                return m_p->Get(id);

            if (m_b)
            {
                auto pIt = std::find_if(m_b, m_b + m_n
                    , [id](mfxExtBuffer* p) { return p && p->BufferId == id; });

                if (pIt != (m_b + m_n))
                    return *pIt;
            }

            return nullptr;
        }

    public:
        CastExtractor(const ParamBase& par)
            : m_b(nullptr)
            , m_n(0)
            , m_p(&par)
        {}

        CastExtractor(mfxExtBuffer ** b, mfxU16 n)
            : m_b(b)
            , m_n(n)
            , m_p(nullptr)
        {}

        template <class T>
        operator T*()
        {
            return (T*)_Get(IdMap<typename std::remove_cv<T>::type>::value);
        }

        template <class T>
        operator const T*() const 
        {
            return (const T*)_Get(IdMap<typename std::remove_cv<T>::type>::value);
        }

        template <class T>
        operator T&()
        {
            T* p = (T*)_Get(IdMap<typename std::remove_cv<T>::type>::value);
            if (!p)
                throw std::logic_error("ext. buffer expected to be always attached");
            return *p;
        }

        template <class T>
        operator const T&() const
        {
            const T* p = (const T*)_Get(IdMap<typename std::remove_cv<T>::type>::value);
            if (!p)
                throw std::logic_error("ext. buffer expected to be always attached");
            return *p;
        }
    };

    template <class P>
    CastExtractor Get(P & par) { return CastExtractor(par.ExtParam, par.NumExtParam); }
    template <class P>
    const CastExtractor Get(const P & par) { return CastExtractor(par.ExtParam, par.NumExtParam); }

    template <class P>
    CastExtractor Get(Param<P>& par) { return CastExtractor(par); }
    template <class P>
    const CastExtractor Get(const Param<P>& par) { return CastExtractor(par); }

    template <class P>
    mfxExtBuffer* Get(P& par, mfxU32 BufferId)
    {
        if (par.ExtParam)
        {
            auto pIt = std::find_if(par.ExtParam, par.ExtParam + par.NumExtParam
                , [BufferId](mfxExtBuffer* p) { return p && p->BufferId == BufferId; });

            if (pIt != (par.ExtParam + par.NumExtParam))
                return *pIt;
        }

        return nullptr;
    }
}; //namespace MfxExtBuffer

