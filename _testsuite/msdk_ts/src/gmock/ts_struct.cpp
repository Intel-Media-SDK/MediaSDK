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

#include "ts_struct.h"

namespace tsStruct
{

Field::Field(mfxU32 _offset, mfxU32 _size, std::string _name) 
    : name  (_name  )
    , offset(_offset)
    , size  (_size  )
{}

void check_eq(void* base, const Field& field, mfxU64 expected)
{
    mfxU64 real = 0;
    memcpy(&real, (mfxU8*)base + field.offset, field.size);

    if(expected != real)
    {
        g_tsLog << "ERROR: FAILED: expected " << field.name << "(" << real << ") == " << expected << "\n";
        ADD_FAILURE();
    }
}

void check_ne(void* base, const Field& field, mfxU64 expected)
{
    mfxU64 real = 0;
    memcpy(&real, (mfxU8*)base + field.offset, field.size);

    if(expected == real)
    {
        g_tsLog << "ERROR: FAILED: expected " << field.name << "(" << real << ") != " << expected << "\n";
        ADD_FAILURE();
    }
}

#define STRUCT(name, fields) Wrap_##name::Wrap_##name(mfxU32 base, mfxU32 size, std::string _name) : Field(base, size, _name) fields {} \
    const Wrap_##name name(0, sizeof(::name), #name);

#define FIELD_T(type, _name) , _name(offset + offsetof(base_type, _name), sizeof(::type), name + "::" + #_name)
#define FIELD_S FIELD_T

#include "ts_struct_decl.h"

#undef STRUCT
#undef FIELD_T
#undef FIELD_S

};