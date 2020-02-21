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
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE) && defined (MFX_VA_LINUX) && (MFX_VERSION >= 1031)

#include "hevcehw_g12_rext_lin.h"
#include "va/va.h"

using namespace HEVCEHW;
using namespace HEVCEHW::Gen12;
using namespace HEVCEHW::Linux;

void Linux::Gen12::RExt::Query1NoCaps(const FeatureBlocks& blocks, TPushQ1 Push)
{
    HEVCEHW::Gen12::RExt::Query1NoCaps(blocks, Push);

    Push(BLK_SetGUID
        , [this](const mfxVideoParam&, mfxVideoParam& par, StorageRW& strg) -> mfxStatus
    {
        auto& g2va = Glob::GuidToVa::GetOrConstruct(strg);
        g2va[DXVA2_Intel_Encode_HEVC_Main12]     = { VAProfileHEVCMain12,     VAEntrypointEncSlice };
        g2va[DXVA2_Intel_Encode_HEVC_Main422_12] = { VAProfileHEVCMain422_12, VAEntrypointEncSlice };
        g2va[DXVA2_Intel_Encode_HEVC_Main444_12] = { VAProfileHEVCMain444_12, VAEntrypointEncSlice };

        //don't change GUID in Reset
        MFX_CHECK(!strg.Contains(Glob::RealState::Key), MFX_ERR_NONE);

        return SetGuid(par, strg);
    });
}


#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
