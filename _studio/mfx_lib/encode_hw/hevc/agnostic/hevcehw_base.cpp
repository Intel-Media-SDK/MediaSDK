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

#include "mfx_common.h"
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE)

#include "hevcehw_base.h"
#include <thread>
#include <sstream>

#if defined(_DEBUG)
    //#define HEVCE_LOG_FUNCTION_CALLS
#endif

namespace HEVCEHW
{

BlockTracer::BlockTracer(
    ID id
    , const char* fName
    , const char* bName)
    : ID(id)
    , m_featureName(fName)
    , m_blockName(bName)
{
#if defined(HEVCE_LOG_FUNCTION_CALLS)
    std::stringstream thid;
    thid << std::this_thread::get_id();
    printf("HEVCEHW TH#%s: Enter %d::%d -> %s::%s\n"
        , thid.str().c_str()
        , FeatureID, BlockID, m_featureName, m_blockName);
    fflush(stdout);
#endif
}

BlockTracer::~BlockTracer()
{
#if defined(HEVCE_LOG_FUNCTION_CALLS)
    std::stringstream thid;
    thid << std::this_thread::get_id();
    printf("HEVCEHW TH#%s: Exit  %d::%d -> %s::%s\n"
        , thid.str().c_str()
        , FeatureID, BlockID, m_featureName, m_blockName);
    fflush(stdout);
#endif
}

const char* FeatureBlocks::GetFeatureName(mfxU32 featureID)
{
#if defined(HEVCE_LOG_FUNCTION_CALLS)
    return m_trace.at(featureID)->first.c_str();
#else
    std::ignore = featureID;
    return nullptr;
#endif
}
const char* FeatureBlocks::GetBlockName(ID id)
{
#if defined(HEVCE_LOG_FUNCTION_CALLS)
    return m_trace.at(id.FeatureID)->second.at(id.BlockID).c_str();
#else
    std::ignore = id;
    return nullptr;
#endif
}

void FeatureBase::Init(
    mfxU32 mode/*eFeatureMode*/
    , FeatureBlocks& blocks)
{
    mfxU32 prevMode = blocks.Init(m_id, mode);

    if (!prevMode)
    {
        SetSupported(blocks);

        ThrowAssert(blocks.m_trace.find(m_id) != blocks.m_trace.end(), "FeatureID must be unique");

        blocks.m_trace[m_id] = GetTrace();
    }

    mode &= ~prevMode;

    bool   bNeedQ0  = !!(mode & QUERY0);
    bool   bNeedQ1  = (mode & (QUERY1 | QUERY_IO_SURF | INIT)) && !(prevMode & (QUERY1 | QUERY_IO_SURF | INIT));
    bool   bNeedQIS = !!(mode & QUERY_IO_SURF);
    bool   bNeedIX  = !!(mode & INIT);
    bool   bNeedSD  = bNeedQIS || bNeedIX;
    bool   bNeedRT  = !!(mode & RUNTIME);
    mfxU32 nQ = 0;

    nQ += bNeedQ0  && InitQueue<Q0>(&FeatureBase::Query0, blocks);
    nQ += bNeedQ1  && InitQueue<Q1NC>(&FeatureBase::Query1NoCaps, blocks);
    nQ += bNeedQ1  && InitQueue<Q1WC>(&FeatureBase::Query1WithCaps, blocks);
    nQ += bNeedQIS && InitQueue<QIS>(&FeatureBase::QueryIOSurf, blocks);
    nQ += bNeedSD  && InitQueue<SD>(&FeatureBase::SetDefaults, blocks);

    if (bNeedIX)
    {
        nQ += InitQueue<IE>(&FeatureBase::InitExternal, blocks);
        nQ += InitQueue<II>(&FeatureBase::InitInternal, blocks);
        nQ += InitQueue<IA>(&FeatureBase::InitAlloc, blocks);
        nQ += InitQueue<AT>(&FeatureBase::AllocTask, blocks);
    }

    if (bNeedRT)
    {
        SetInherited(blocks);

        nQ += InitQueue<R>(&FeatureBase::Reset, blocks);
        nQ += InitQueue<RS>(&FeatureBase::ResetState, blocks);
        nQ += InitQueue<FS>(&FeatureBase::FrameSubmit, blocks);
        nQ += InitQueue<AR>(&FeatureBase::AsyncRoutine, blocks);
        nQ += InitQueue<IT>(&FeatureBase::InitTask, blocks);
        nQ += InitQueue<PreRT>(&FeatureBase::PreReorderTask, blocks);
        nQ += InitQueue<PostRT>(&FeatureBase::PostReorderTask, blocks);
        nQ += InitQueue<ST>(&FeatureBase::SubmitTask, blocks);
        nQ += InitQueue<QT>(&FeatureBase::QueryTask, blocks);
        nQ += InitQueue<FT>(&FeatureBase::FreeTask, blocks);
        nQ += InitQueue<CLS>(&FeatureBase::Close, blocks);
        nQ += InitQueue<GVP>(&FeatureBase::GetVideoParam, blocks);
    }

    assert(nQ > 0);
}

}; //namespace HEVCEHW

#endif
