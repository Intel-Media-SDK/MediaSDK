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

#include "mfx_common.h"
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE)

#include "hevcehw_base_alloc.h"

using namespace HEVCEHW;
using namespace HEVCEHW::Base;

template<typename TR, typename ...TArgs>
inline typename CallChain<TR, TArgs...>::TInt
WrapCC(TR(MfxEncodeHW::ResPool::*pfn)(TArgs...), MfxEncodeHW::ResPool* pAlloc)
{
    return [=](typename CallChain<TR, TArgs...>::TExt, TArgs ...arg)
    {
        return (pAlloc->*pfn)(arg...);
    };
}

template<typename TR, typename ...TArgs>
inline typename CallChain<TR, TArgs...>::TInt
WrapCC(TR(MfxEncodeHW::ResPool::*pfn)(TArgs...) const, const MfxEncodeHW::ResPool* pAlloc)
{
    return [=](typename CallChain<TR, TArgs...>::TExt, TArgs ...arg)
    {
        return (pAlloc->*pfn)(arg...);
    };
}

IAllocation* Allocator::MakeAlloc(std::unique_ptr<MfxEncodeHW::ResPool>&& upAlloc)
{
    std::unique_ptr<IAllocation> pIAlloc(new IAllocation);

    bool bInvalid = !(pIAlloc && upAlloc);
    if (bInvalid)
        return nullptr;

    auto pAlloc = upAlloc.release();

#define WRAP_CC(X) pIAlloc->X.Push(WrapCC(&MfxEncodeHW::ResPool::X, pAlloc))
    WRAP_CC(Alloc);
    WRAP_CC(AllocOpaque);
    WRAP_CC(GetResponse);
    WRAP_CC(GetInfo);
    WRAP_CC(Release);
    WRAP_CC(ClearFlag);
    WRAP_CC(SetFlag);
    WRAP_CC(GetFlag);
    WRAP_CC(UnlockAll);
    WRAP_CC(Acquire);

    pIAlloc->m_pthis.reset(pAlloc);

    return pIAlloc.release();
}

void Allocator::InitAlloc(const FeatureBlocks& /*blocks*/, TPushIA Push)
{
    Push(BLK_Init
        , [](StorageRW&, StorageRW& local) -> mfxStatus
    {
        auto CreateAllocator = [](VideoCORE& core) -> IAllocation*
        {
            return MakeAlloc(std::unique_ptr<MfxEncodeHW::ResPool>(new MfxEncodeHW::ResPool(core)));
        };

        local.Insert(Tmp::MakeAlloc::Key, new Tmp::MakeAlloc::TRef(CreateAllocator));

        return MFX_ERR_NONE;
    });
}

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
