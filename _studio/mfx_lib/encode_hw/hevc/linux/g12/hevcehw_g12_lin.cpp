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
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE) && defined (MFX_VA_LINUX)

#include "hevcehw_g12_lin.h"
#if (MFX_VERSION >= 1031)
#include "hevcehw_g12_rext_lin.h"
#endif
#include "hevcehw_g12_caps_lin.h"
#include "hevcehw_g12_sao.h"
#include "hevcehw_g11_legacy.h"
#include "hevcehw_g11_iddi_packer.h"
#include "hevcehw_g11_iddi.h"

namespace HEVCEHW
{
namespace Linux
{
namespace Gen12
{
using namespace HEVCEHW::Gen12;

MFXVideoENCODEH265_HW::MFXVideoENCODEH265_HW(
    VideoCORE& core
    , mfxStatus& status
    , eFeatureMode mode)
    : TBaseGen(core, status, mode)
{
    TFeatureList newFeatures;

#if (MFX_VERSION >= 1031)
    newFeatures.emplace_back(new RExt(FEATURE_REXT));
#endif
    newFeatures.emplace_back(new Caps(FEATURE_CAPS));
    newFeatures.emplace_back(new SAO(FEATURE_SAO));

    InternalInitFeatures(status, mode, newFeatures);
}

void MFXVideoENCODEH265_HW::InternalInitFeatures(
    mfxStatus& status
    , eFeatureMode mode
    , TFeatureList& newFeatures)
{
    status = MFX_ERR_UNKNOWN;

    for (auto& pFeature : newFeatures)
        pFeature->Init(mode, *this);

    TBaseGen::m_features.splice(TBaseGen::m_features.end(), newFeatures);

    if (mode & (QUERY1 | QUERY_IO_SURF | INIT))
    {
        auto& qnc = FeatureBlocks::BQ<FeatureBlocks::BQ_Query1NoCaps>::Get(*this);

        FeatureBlocks::Reorder(
            qnc
            , { HEVCEHW::Gen11::FEATURE_LEGACY, HEVCEHW::Gen11::Legacy::BLK_SetLowPowerDefault }
            , { FEATURE_CAPS, Caps::BLK_SetDefaultsCallChain });

        auto& qwc = FeatureBlocks::BQ<FeatureBlocks::BQ_Query1WithCaps>::Get(*this);
#if (MFX_VERSION >= 1031)
        FeatureBlocks::Reorder(
            qwc
            , { HEVCEHW::Gen11::FEATURE_DDI_PACKER, HEVCEHW::Gen11::IDDIPacker::BLK_HardcodeCaps }
            , { FEATURE_REXT, RExt::BLK_HardcodeCaps });
#endif
        FeatureBlocks::Reorder(
            qwc
            , { HEVCEHW::Gen11::FEATURE_DDI_PACKER, HEVCEHW::Gen11::IDDIPacker::BLK_HardcodeCaps }
            , { FEATURE_CAPS, Caps::BLK_HardcodeCaps });
    }

    status = MFX_ERR_NONE;
}

} //namespace Linux
} //namespace Gen12
} //namespace HEVCEHW

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
