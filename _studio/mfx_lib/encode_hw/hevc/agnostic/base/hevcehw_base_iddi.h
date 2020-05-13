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
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE)

#include "hevcehw_base.h"

namespace HEVCEHW
{
namespace Base
{

class IDDI
    : public FeatureBase
{
public:
#define DECL_BLOCK_LIST\
    DECL_BLOCK(SetCallChains) \
    DECL_BLOCK(QueryCaps)     \
    DECL_BLOCK(CreateDevice)  \
    DECL_BLOCK(CreateService) \
    DECL_BLOCK(Register)      \
    DECL_BLOCK(Reset)      \
    DECL_BLOCK(SubmitTask)    \
    DECL_BLOCK(QueryTask)
#define DECL_FEATURE_NAME "Base_IDDI"
#include "hevcehw_decl_blocks.h"

    IDDI(mfxU32 FeatureId)
        : FeatureBase(FeatureId)
    {}

protected:
    virtual void Query1WithCaps(const FeatureBlocks& blocks, TPushQ1 Push) override = 0;
    virtual void InitExternal(const FeatureBlocks& blocks, TPushIE Push) override = 0;
    virtual void InitAlloc(const FeatureBlocks& blocks, TPushIA Push) override = 0;
    virtual void SubmitTask(const FeatureBlocks& blocks, TPushST Push) override = 0;
    virtual void QueryTask(const FeatureBlocks& blocks, TPushQT Push) override = 0;
    virtual void ResetState(const FeatureBlocks& blocks, TPushRS Push) override = 0;
};

} //Base
} //namespace HEVCEHW

#endif
