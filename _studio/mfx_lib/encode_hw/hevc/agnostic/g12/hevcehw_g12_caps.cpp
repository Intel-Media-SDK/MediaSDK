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

#include "hevcehw_g12_caps.h"

using namespace HEVCEHW;
using namespace HEVCEHW::Gen12;

void Caps::Query1NoCaps(const FeatureBlocks& /*blocks*/, TPushQ1 Push)
{
    Push(BLK_SetDefaultsCallChain,
        [this](const mfxVideoParam&, mfxVideoParam&, StorageRW& strg) -> mfxStatus
    {
        auto& defaults = Glob::Defaults::GetOrConstruct(strg);
        auto& bSet = defaults.SetForFeature[GetID()];
        MFX_CHECK(!bSet, MFX_ERR_NONE);

        defaults.GetMaxNumRef.Push([](
            Base::Defaults::TChain<std::tuple<mfxU16, mfxU16, mfxU16>>::TExt
            , const Base::Defaults::Param& dpar)
        {
            const mfxU16 nRef[2][3][7] =
            {
                {   // VME
                    { 4, 4, 3, 3, 3, 1, 1 },
                    { 4, 4, 3, 3, 3, 1, 1 },
                    { 2, 2, 1, 1, 1, 1, 1 }
                },
                {   // VDENC
                    { 3, 3, 2, 2, 2, 1, 1 },
                    { 2, 2, 1, 1, 1, 1, 1 },
                    { 1, 1, 1, 1, 1, 1, 1 }
                }
            };
            bool    bVDEnc = IsOn(dpar.mvp.mfx.LowPower);
            mfxU16  tu     = dpar.mvp.mfx.TargetUsage;

            CheckRangeOrSetDefault<mfxU16>(tu, 1, 7, 4);
            --tu;

            /* Same way like on Base or Gen11 platforms */
            mfxU16 numRefFrame = dpar.mvp.mfx.NumRefFrame + !dpar.mvp.mfx.NumRefFrame * 16;

            return std::make_tuple(
                std::min<mfxU16>(nRef[bVDEnc][0][tu], std::min<mfxU16>(dpar.caps.MaxNum_Reference0, numRefFrame))
                , std::min<mfxU16>(nRef[bVDEnc][1][tu], std::min<mfxU16>(dpar.caps.MaxNum_Reference0, numRefFrame))
                , std::min<mfxU16>(nRef[bVDEnc][2][tu], std::min<mfxU16>(dpar.caps.MaxNum_Reference1, numRefFrame)));
        });

        bSet = true;

        return MFX_ERR_NONE;
    });
}

void Caps::Query1WithCaps(const FeatureBlocks& /*blocks*/, TPushQ1 Push)
{
    Push(BLK_HardcodeCaps
        , [this](const mfxVideoParam&, mfxVideoParam& par, StorageRW& strg) -> mfxStatus
    {
        auto& caps = Glob::EncodeCaps::Get(strg);

        caps.SliceIPOnly                = IsOn(par.mfx.LowPower) && par.mfx.CodecProfile == MFX_PROFILE_HEVC_SCC;
        caps.msdk.bSingleSliceMultiTile = false;

        caps.YUV422ReconSupport &= !caps.Color420Only;

        SetSpecificCaps(caps);

        return MFX_ERR_NONE;
    });
}

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
