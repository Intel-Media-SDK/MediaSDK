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
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE) && defined(MFX_VA_LINUX)

#include "hevcehw_base_interlace_lin.h"
#include "hevcehw_base_va_packer_lin.h"

using namespace HEVCEHW;
using namespace HEVCEHW::Base;

void Linux::Base::Interlace::SubmitTask(const FeatureBlocks& blocks, TPushST Push)
{
    HEVCEHW::Base::Interlace::SubmitTask(blocks, Push);

    Push(BLK_PatchDDITask
        , [](StorageW& global, StorageW& /*s_task*/) -> mfxStatus
    {
        auto& par = Glob::VideoParam::Get(global);
        MFX_CHECK(IsField(par.mfx.FrameInfo.PicStruct), MFX_ERR_NONE);
        
        auto& ddiPar = Glob::DDI_SubmitParam::Get(global);
        auto it = std::find_if(std::begin(ddiPar), std::end(ddiPar)
            , [](DDIExecParam& ep) { return (ep.Function == VAEncSequenceParameterBufferType); });
        MFX_CHECK(it != std::end(ddiPar) && it->In.pData, MFX_ERR_NONE);

        auto& sps = *(VAEncSequenceParameterBufferHEVC*)it->In.pData;
        sps.intra_period     = par.mfx.GopPicSize * 2;
        sps.intra_idr_period = par.mfx.GopPicSize * par.mfx.IdrInterval * 2;
        sps.ip_period        = mfxU8(par.mfx.GopRefDist * 2);
        return MFX_ERR_NONE;
    });
}


#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
