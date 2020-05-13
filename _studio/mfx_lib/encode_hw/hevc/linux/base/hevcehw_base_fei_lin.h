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
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE) && defined (MFX_VA_LINUX)

#include "hevcehw_base.h"
#include "hevcehw_base_data.h"
#include "hevcehw_base_iddi_packer.h"
#include "va/va.h"

namespace HEVCEHW
{
namespace Linux
{
namespace Base
{
using namespace HEVCEHW::Base;

class FEI
    : public virtual FeatureBase
{
public:
#define DECL_BLOCK_LIST \
    DECL_BLOCK(CheckAndFix) \
    DECL_BLOCK(SetGuidMap) \
    DECL_BLOCK(SetVaCallChain) \
    DECL_BLOCK(FrameCheck) \
    DECL_BLOCK(UpdatePPS) \
    DECL_BLOCK(SetTaskVaParam) \
    DECL_BLOCK(SetFeedbackCallChain) \
    DECL_BLOCK(CancelTasks)
#define DECL_FEATURE_NAME "G11FEI_FEI"
#include "hevcehw_decl_blocks.h"

    FEI(mfxU32 FeatureId) : FeatureBase(FeatureId) {}

protected:
    virtual void Query1NoCaps(const FeatureBlocks& blocks, TPushQ1 Push) override;
    virtual void Query1WithCaps(const FeatureBlocks& blocks, TPushQ1 Push) override;
    virtual void Reset(const FeatureBlocks& blocks, TPushR Push) override;
    virtual void FrameSubmit(const FeatureBlocks& blocks, TPushFS Push) override;
    virtual void PostReorderTask(const FeatureBlocks& blocks, TPushPostRT Push) override;
    virtual void InitAlloc(const FeatureBlocks& blocks, TPushIA Push) override;

    mfxU32 m_lastIDR     = 0;
    mfxI32 m_prevIPoc    = 0;
    mfxU32 m_frameOrder  = 0;
};

} //Base
} //Linux
} //namespace HEVCEHW

#endif
