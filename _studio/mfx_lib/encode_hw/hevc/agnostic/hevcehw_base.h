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

#include "mfxvideo.h"
#include "mfxla.h"
#include "mfxbrc.h"
#include "mfx_ext_buffers.h"
#include "mfxvideo++int.h"
#include "mfx_utils_extbuf.h"
#include "feature_blocks/mfx_feature_blocks_init_macros.h"
#include "feature_blocks/mfx_feature_blocks_base.h"
#include "hevcehw_utils.h"

namespace HEVCEHW
{
using namespace MfxFeatureBlocks;

namespace ExtBuffer
{
    using namespace MfxExtBuffer;
};

class BlockTracer
    : public ID
{
public:
    using TFeatureTrace = std::pair<std::string, const std::map<mfxU32, const std::string>>;

    BlockTracer(
        ID id
        , const char* fName = nullptr
        , const char* bName = nullptr);

    ~BlockTracer();

    const char* m_featureName;
    const char* m_blockName;
};

struct FeatureBlocks
    : FeatureBlocksCommon<BlockTracer>
{
MFX_FEATURE_BLOCKS_DECLARE_BQ_UTILS_IN_FEATURE_BLOCK
#define DEF_BLOCK_Q MFX_FEATURE_BLOCKS_DECLARE_QUEUES_IN_FEATURE_BLOCK
#include "hevcehw_block_queues.h"
#undef DEF_BLOCK_Q

    virtual const char* GetFeatureName(mfxU32 featureID) override;
    virtual const char* GetBlockName(ID id) override;

    std::map<mfxU32, const BlockTracer::TFeatureTrace*> m_trace;
};

#define DEF_BLOCK_Q MFX_FEATURE_BLOCKS_DECLARE_QUEUES_EXTERNAL
    #include "hevcehw_block_queues.h"
#undef DEF_BLOCK_Q

class FeatureBase
    : public FeatureBaseCommon<FeatureBlocks>
{
public:
    FeatureBase();
    virtual ~FeatureBase() {}

    virtual void Init(
        mfxU32 mode/*eFeatureMode*/
        , FeatureBlocks& blocks) override;

protected:
#define DEF_BLOCK_Q MFX_FEATURE_BLOCKS_DECLARE_QUEUES_IN_FEATURE_BASE
#include "hevcehw_block_queues.h"
#undef DEF_BLOCK_Q
    typedef TPushQ1NC TPushQ1;

    FeatureBase(mfxU32 id)
        : FeatureBaseCommon<FeatureBlocks>(id)
    {}

    virtual const BlockTracer::TFeatureTrace* GetTrace() { return nullptr; }
    virtual void SetTraceName(std::string&& /*name*/) {}
};

enum eImplMode
{
    IMPL_MODE_DEFAULT = 0
    , IMPL_MODE_FEI
};

class ImplBase
    : virtual public VideoENCODE
{
public:
    virtual mfxStatus InternalQuery(
        VideoCORE& core
        , mfxVideoParam *in
        , mfxVideoParam& out) = 0;

    virtual mfxStatus InternalQueryIOSurf(
        VideoCORE& core
        , mfxVideoParam& par
        , mfxFrameAllocRequest& request) = 0;

    virtual ImplBase* ApplyMode(mfxU32 /*mode*/)
    {
        return this;
    }
};

}; //namespace HEVCEHW

#endif
