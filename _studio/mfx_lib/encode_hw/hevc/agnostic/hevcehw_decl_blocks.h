// Copyright (c) 2019 Intel Corporation
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

//use customized version w/trace support instead of default one
#define DECL_BLOCK_CLEANUP_DISABLE
#include "feature_blocks/mfx_feature_blocks_decl_blocks.h"

#if defined(_DEBUG)
BlockTracer::TFeatureTrace m_trace =
{
    DECL_FEATURE_NAME,
    {
    #define DECL_BLOCK(NAME) {BLK_##NAME, #NAME},
        DECL_BLOCK_LIST
    #undef DECL_BLOCK
    }
};

virtual const BlockTracer::TFeatureTrace* GetTrace() override { return &m_trace; }
virtual void SetTraceName(std::string&& name) override { m_trace.first = std::move(name); }
#endif // defined(_DEBUG)

#undef DECL_BLOCK_LIST
#undef DECL_FEATURE_NAME
#undef DECL_BLOCK_CLEANUP_DISABLE