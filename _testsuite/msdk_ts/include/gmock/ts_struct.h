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

#include "ts_common.h"

namespace tsStruct
{

class Field
{
public:
    Field(){}
    Field(mfxU32 _offset, mfxU32 _size, std::string _name);

    std::string name;
    mfxU32      offset;
    mfxU32      size;

private:
    //Don't copy, use references or pointers
    Field(const Field&);
};

template <class T, mfxU32 N> class Array : public T
{
private:
    T m_fields[N];
public:
    Array(){}
    Array(mfxU32 _offset, mfxU32 _size, std::string _name)
        : T(_offset, _size * N, _name)
    {
        for(mfxU32 i = 0; i < N; i ++)
        {
            m_fields[i] = T(_offset + _size * i, _size, _name + "[" + std::to_string(i) + "]");
        }
    }

    inline const T& operator[] (mfxU32 i) const { assert(i < N); return m_fields[i]; }
    inline operator const T&() const { return *this; }
};

void check_eq(void* base, const Field& field, mfxU64 expected);
void check_ne(void* base, const Field& field, mfxU64 expected);
inline void set(void* base, const Field& field, mfxU64 value) { memcpy((mfxU8*)base + field.offset, &value, field.size); }
inline mfxU64 get(void* base, const Field& field) { mfxU64 value = 0; memcpy(&value, (mfxU8*)base + field.offset, field.size); return value; }

#define STRUCT(name, fields)                        \
    class Wrap_##name : public Field                \
    {                                               \
    private:                                        \
        typedef ::name base_type;                   \
    public:                                         \
        Wrap_##name() {}                            \
        Wrap_##name(mfxU32 base, mfxU32 size, std::string _name);\
        fields                                      \
    };                                              \
    extern Wrap_##name const name;
#define FIELD_T(type, name) Array<Field, (sizeof(((base_type*)0)->name)/sizeof(::type))> name;
#define FIELD_S(type, name) Array<Wrap_##type, (sizeof(((base_type*)0)->name)/sizeof(::type))> name;

#include "ts_struct_decl.h"

#undef STRUCT
#undef FIELD_T
#undef FIELD_S

template <typename T, typename TFP>
void SetParamIfStage(tsExtBufType<T>& base, const TFP& fpair, const mfxU32 stage = 0)
{
    if(0 != fpair.f && fpair.stage == stage)
        return SetParam(base, fpair.f->name, fpair.f->offset, fpair.f->size, fpair.v);
    else
        return;
}

template <typename TFP>
void SetParamIfStage(::mfxFrameSurface1* base, const TFP& fpair, const mfxU32 stage = 0)
{
    if(0 != fpair.f && fpair.stage == stage && fpair.f->name.find("mfxFrameSurface1") != std::string::npos)
        return SetParam((void*) base, fpair.f->name, fpair.f->offset, fpair.f->size, fpair.v);
    else
        return;
}

template <typename T, typename TB>
void SetPars(tsExtBufType<TB>& base, const T& tc, const mfxU32 stage = 0)
{
    const size_t npars = sizeof(tc.set_par) / sizeof(tc.set_par[0]);
    for(size_t i = 0; i < npars; ++i)
    {
        SetParamIfStage(base, tc.set_par[i], stage);
    }
}

template <typename T, typename TB>
void SetPars(const tsExtBufType<TB>*& base, const T& tc, const mfxU32 stage = 0)
{
    assert(0 != base);
    if(base)    return SetPars(*base, tc, stage);
}

template <typename T>
void SetPars(::mfxFrameSurface1* base, const T& tc, const mfxU32 stage = 0)
{
    assert(0 != base);
    const size_t npars = sizeof(tc.set_par) / sizeof(tc.set_par[0]);
    for(size_t i = 0; i < npars; ++i)
    {
        SetParamIfStage(base, tc.set_par[i], stage);
    }
}

};
