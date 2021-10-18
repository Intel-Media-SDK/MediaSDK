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
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE) && defined(MFX_ENABLE_HEVCE_WEIGHTED_PREDICTION) && defined (MFX_VA_LINUX)

#include "hevcehw_base_weighted_prediction_lin.h"
#include "hevcehw_base_va_packer_lin.h"
#include "va/va.h"

using namespace HEVCEHW;
using namespace HEVCEHW::Base;

void Linux::Base::WeightPred::SubmitTask(const FeatureBlocks& /*blocks*/, TPushST Push)
{
    Push(BLK_PatchDDITask
            , [](StorageW& global, StorageW& s_task) -> mfxStatus
    {
        auto& esSlice = Task::SSH::Get(s_task);
        auto& ddiPar  = Glob::DDI_SubmitParam::Get(global);
        auto  itPPS   = std::find_if(std::begin(ddiPar), std::end(ddiPar)
            , [](DDIExecParam& ep) { return (ep.Function == VAEncPictureParameterBufferType); });

        MFX_CHECK(itPPS != std::end(ddiPar) && itPPS->In.pData, MFX_ERR_UNKNOWN);

        auto& ddiPPS    = *(VAEncPictureParameterBufferHEVC*)itPPS->In.pData;
        const mfxExtCodingOption3& CO3 = ExtBuffer::Get(Glob::VideoParam::Get(global));

        ddiPPS.pic_fields.bits.enable_gpu_weighted_prediction = IsOn(CO3.FadeDetection); // should be set for I as well
        bool bNeedPWT   = (esSlice.type != 2
            && (ddiPPS.pic_fields.bits.weighted_bipred_flag || ddiPPS.pic_fields.bits.weighted_pred_flag));
        MFX_CHECK(bNeedPWT, MFX_ERR_NONE);

        auto& ph    = Glob::PackedHeaders::Get(global);
        auto  itSSH = ph.SSH.begin();

        auto SetPWT = [&](VAEncSliceParameterBufferHEVC& slice)
        {
            const mfxU16 Y = 0, Cb = 1, Cr = 2, Weight = 0, Offset = 1;

            slice.luma_log2_weight_denom         = (mfxU8)esSlice.luma_log2_weight_denom;
            slice.delta_chroma_log2_weight_denom = (mfxI8)(esSlice.chroma_log2_weight_denom - slice.luma_log2_weight_denom);
            slice.pred_weight_table_bit_offset   = itSSH->PackInfo.at(PACK_PWTOffset);
            slice.pred_weight_table_bit_length   = itSSH->PackInfo.at(PACK_PWTLength);

            mfxI16 wY = (1 << slice.luma_log2_weight_denom);
            mfxI16 wC = (1 << esSlice.chroma_log2_weight_denom);

            for (mfxU16 i = 0; i < 15; i++)
            {
                slice.luma_offset_l0[i]            = (mfxI8)esSlice.pwt[0][i][Y][Offset];
                slice.delta_luma_weight_l0[i]      = (mfxI8)(esSlice.pwt[0][i][Y][Weight] - wY);
                slice.chroma_offset_l0[i][0]       = (mfxI8)esSlice.pwt[0][i][Cb][Offset];
                slice.chroma_offset_l0[i][1]       = (mfxI8)esSlice.pwt[0][i][Cr][Offset];
                slice.delta_chroma_weight_l0[i][0] = (mfxI8)(esSlice.pwt[0][i][Cb][Weight] - wC);
                slice.delta_chroma_weight_l0[i][1] = (mfxI8)(esSlice.pwt[0][i][Cr][Weight] - wC);
                slice.luma_offset_l1[i]            = (mfxI8)esSlice.pwt[1][i][Y][Offset];
                slice.delta_luma_weight_l1[i]      = (mfxI8)(esSlice.pwt[1][i][Y][Weight] - wY);
                slice.chroma_offset_l1[i][0]       = (mfxI8)esSlice.pwt[1][i][Cb][Offset];
                slice.chroma_offset_l1[i][1]       = (mfxI8)esSlice.pwt[1][i][Cr][Offset];
                slice.delta_chroma_weight_l1[i][0] = (mfxI8)(esSlice.pwt[1][i][Cb][Weight] - wC);
                slice.delta_chroma_weight_l1[i][1] = (mfxI8)(esSlice.pwt[1][i][Cr][Weight] - wC);
            }
        };

        std::for_each(std::begin(ddiPar), std::end(ddiPar)
            , [&](DDIExecParam& ep)
        {
            if (ep.Function != VAEncSliceParameterBufferType)
                return;

            auto pBegin = (VAEncSliceParameterBufferHEVC*)ep.In.pData;
            auto pEnd   = pBegin + (std::max<mfxU32>(ep.In.Num, 1) * !!pBegin);

            assert(ep.In.Size == sizeof(VAEncSliceParameterBufferHEVC));

            std::for_each(pBegin, pEnd, SetPWT);
        });

        return MFX_ERR_NONE;
    });
}

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
