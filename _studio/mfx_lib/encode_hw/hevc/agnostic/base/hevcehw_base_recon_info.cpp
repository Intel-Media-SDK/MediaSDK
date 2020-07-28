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

#include "hevcehw_base_recon_info.h"

using namespace HEVCEHW;
using namespace HEVCEHW::Base;

void ReconInfo::InitInternal(const FeatureBlocks& /*blocks*/, TPushII Push)
{
    Push(BLK_SetRecInfo
        , [this](StorageRW& strg, StorageRW& local) -> mfxStatus
    {
        auto& par = Glob::VideoParam::Get(strg);
        mfxFrameAllocRequest rec = {};

        if (GetRecInfo(par, ExtBuffer::Get(par), rec.Info))
        {
            Tmp::RecInfo::GetOrConstruct(local, rec);
            SetDefault(rec.NumFrameMin, GetMaxRec(par));
        }

        return MFX_ERR_NONE;
    });
}

void ReconInfo::InitAlloc(const FeatureBlocks& /*blocks*/, TPushIA Push)
{
    Push(BLK_AllocRec
        , [this](StorageRW& strg, StorageRW& local) -> mfxStatus
    {
        mfxStatus sts = MFX_ERR_NONE;
        auto& par = Glob::VideoParam::Get(strg);
        std::unique_ptr<IAllocation> pAlloc(Tmp::MakeAlloc::Get(local)(Glob::VideoCore::Get(strg)));

        MFX_CHECK(local.Contains(Tmp::RecInfo::Key), MFX_ERR_UNDEFINED_BEHAVIOR);
        auto& req = Tmp::RecInfo::Get(local);

        SetDefault(req.NumFrameMin, GetMaxRec(par));
        SetDefault(req.Type
            , mfxU16(MFX_MEMTYPE_FROM_ENCODE
                | MFX_MEMTYPE_DXVA2_DECODER_TARGET
                | MFX_MEMTYPE_INTERNAL_FRAME
                | MFX_MEMTYPE_VIDEO_MEMORY_ENCODER_TARGET));

        sts = pAlloc->Alloc(req, false);
        MFX_CHECK_STS(sts);

        strg.Insert(Glob::AllocRec::Key, std::move(pAlloc));

        return sts;
    });
}

mfxU16 ReconInfo::GetMaxRec(mfxVideoParam const & par)
{
    return par.AsyncDepth + par.mfx.NumRefFrame + (par.AsyncDepth > 1);
}

bool ReconInfo::GetRecInfo(
    const mfxVideoParam& par
    , const mfxExtCodingOption3& CO3
    , mfxFrameInfo& rec)
{
    rec = par.mfx.FrameInfo;

    auto& rModRec = ModRec[CO3.TargetBitDepthLuma == 10];
    auto  itModRec = rModRec.find(CO3.TargetChromaFormatPlus1);
    bool bUndef =
        (CO3.TargetBitDepthLuma != 8 && CO3.TargetBitDepthLuma != 10)
        || (itModRec == rModRec.end());

    if (!bUndef)
    {
        itModRec->second(rec);
    }

    rec.ChromaFormat = CO3.TargetChromaFormatPlus1 - 1;
    rec.BitDepthLuma = CO3.TargetBitDepthLuma;
    rec.BitDepthChroma = CO3.TargetBitDepthChroma;

    return true;
}

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
