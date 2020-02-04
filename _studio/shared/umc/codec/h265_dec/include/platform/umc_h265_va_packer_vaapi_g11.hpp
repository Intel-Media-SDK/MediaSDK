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

//This file is included from [umc_h265_va_packer_vaapi.cpp]
//DO NOT include it into the project manually

#ifndef UMC_VA_H265_PACKER_G11
#define UMC_VA_H265_PACKER_G11

#include "umc_h265_va_packer_vaapi_g9.hpp"

namespace UMC_HEVC_DECODER
{
    template <>
    struct Type2Buffer<VAPictureParameterBufferHEVCExtension> : std::integral_constant<int32_t, VAPictureParameterBufferType> {};
    template <>
    struct Type2Buffer<VASliceParameterBufferHEVCExtension> : std::integral_constant<int32_t, VASliceParameterBufferType> {};

    namespace G11
    {
        inline
        void PackPicHeader(UMC::VideoAccelerator*, H265DecoderFrame const* frame, H265DBPList const* dpb, VAPictureParameterBufferHEVCRext* pp)
        {
            assert(frame);
            (void) dpb;
            assert(pp);

            auto si = frame->GetAU();
            if (!si)
                throw h265_exception(UMC::UMC_ERR_FAILED);

            auto slice = si->GetSlice(0);
            assert(slice);

            auto pps = slice->GetPicParam();
            assert(pps);
            auto sps = slice->GetSeqParam();
            assert(sps);

            auto& pic_fields = pp->range_extension_pic_fields.bits;
            pic_fields.transform_skip_rotation_enabled_flag        = sps->transform_skip_rotation_enabled_flag;
            pic_fields.transform_skip_context_enabled_flag         = sps->transform_skip_context_enabled_flag;
            pic_fields.implicit_rdpcm_enabled_flag                 = sps->implicit_residual_dpcm_enabled_flag;
            pic_fields.explicit_rdpcm_enabled_flag                 = sps->explicit_residual_dpcm_enabled_flag;
            pic_fields.extended_precision_processing_flag          = sps->extended_precision_processing_flag;
            pic_fields.intra_smoothing_disabled_flag               = sps->intra_smoothing_disabled_flag;
            pic_fields.high_precision_offsets_enabled_flag         = sps->high_precision_offsets_enabled_flag;;
            pic_fields.persistent_rice_adaptation_enabled_flag     = sps->fast_rice_adaptation_enabled_flag;
            pic_fields.cabac_bypass_alignment_enabled_flag         = sps->cabac_bypass_alignment_enabled_flag;
            pic_fields.cross_component_prediction_enabled_flag     = pps->cross_component_prediction_enabled_flag;
            pic_fields.chroma_qp_offset_list_enabled_flag          = pps->chroma_qp_offset_list_enabled_flag;

            pp->diff_cu_chroma_qp_offset_depth                     = (uint8_t)pps->diff_cu_chroma_qp_offset_depth;
            pp->chroma_qp_offset_list_len_minus1 = pps->chroma_qp_offset_list_enabled_flag ?
                (uint8_t)pps->chroma_qp_offset_list_len - 1 : 0;
            pp->log2_sao_offset_scale_luma                         = (uint8_t)pps->log2_sao_offset_scale_luma;
            pp->log2_sao_offset_scale_chroma                       = (uint8_t)pps->log2_sao_offset_scale_chroma;
            pp->log2_max_transform_skip_block_size_minus2 = pps->pps_range_extensions_flag && pps->transform_skip_enabled_flag ?
                (uint8_t)pps->log2_max_transform_skip_block_size_minus2 : 0;

            for (uint32_t i = 0; i < pps->chroma_qp_offset_list_len; i++)
            {
                pp->cb_qp_offset_list[i] = (int8_t)pps->cb_qp_offset_list[i + 1];
                pp->cr_qp_offset_list[i] = (int8_t)pps->cr_qp_offset_list[i + 1];
            }
        }

        inline
        void PackSliceHeader(UMC::VideoAccelerator*, H265Slice const* slice, VAPictureParameterBufferHEVC const* pp, VASliceParameterBufferHEVCRext* sp, bool last_slice)
        {
            assert(slice);
            assert(pp);
            assert(sp);
            (void) last_slice;
            (void) pp;

            auto sh = slice->GetSliceHeader();
            assert(sh);

            for (int l = 0; l < 2; l++)
            {
                EnumRefPicList eRefPicList = ( l == 1 ? REF_PIC_LIST_1 : REF_PIC_LIST_0 );

                for (int32_t iRefIdx = 0; iRefIdx < slice->getNumRefIdx(eRefPicList); iRefIdx++)
                {
                    wpScalingParam const* wp = sh->pred_weight_table[eRefPicList][iRefIdx];

                    if (eRefPicList == REF_PIC_LIST_0)
                    {
                        sp->luma_offset_l0[iRefIdx] = (int16_t)wp[0].offset;
                        for(int chroma=0;chroma < 2;chroma++)
                            sp->ChromaOffsetL0[iRefIdx][chroma] = (int16_t)wp[1 + chroma].offset;
                    }
                    else
                    {
                        sp->luma_offset_l1[iRefIdx] = (int16_t)wp[0].offset;
                        for(int chroma=0; chroma < 2; chroma++)
                            sp->ChromaOffsetL1[iRefIdx][chroma] = (int16_t)wp[1 + chroma].offset;
                    }
                }
            }

            auto& slice_ext_flags = sp->slice_ext_flags.bits;
            slice_ext_flags.cu_chroma_qp_offset_enabled_flag = sh->cu_chroma_qp_offset_enabled_flag;
            slice_ext_flags.use_integer_mv_flag              = (uint8_t)sh->use_integer_mv_flag;

            sp->slice_act_y_qp_offset  = (int8_t)sh->slice_act_y_qp_offset;
            sp->slice_act_cb_qp_offset = (int8_t)sh->slice_act_cb_qp_offset;
            sp->slice_act_cr_qp_offset = (int8_t)sh->slice_act_cr_qp_offset;
        }

        class PackerVAAPI
            : public G9::PackerVAAPI
        {
        public:

            PackerVAAPI(UMC::VideoAccelerator* va)
                : G9::PackerVAAPI::PackerVAAPI(va)
            {}

        protected:

            void CreateSliceParamBuffer(size_t count) override
            {
                ! m_va->IsLongSliceControl() ||
                !(m_va->m_Profile & UMC::VA_PROFILE_REXT) ?
                    G9::PackerVAAPI::CreateSliceParamBuffer(count) :
                    CreateParamsBuffer<VASliceParameterBufferHEVCExtension>(m_va, count)
                ;
            }

            void PackPicParams(H265DecoderFrame const* frame, TaskSupplier_H265* supplier) override
            {
                assert(frame);
                assert(supplier);

                if (!(m_va->m_Profile & UMC::VA_PROFILE_REXT))
                    G9::PackerVAAPI::PackPicParams(frame, supplier);
                else
                {
                    H265DBPList const* dpb = supplier->GetDPBList();
                    if (!dpb)
                        throw h265_exception(UMC::UMC_ERR_FAILED);

                    VAPictureParameterBufferHEVCExtension* pp = 0;
                    PeekParamsBuffer(m_va, &pp);

                    G9::PackPicHeader(m_va, frame, dpb, &pp->base);
                        PackPicHeader(m_va, frame, dpb, &pp->rext);
                }
            }

            VASliceParameterBufferBase* PackSliceParams(H265Slice const* slice, bool last_slice) override
            {
                if (!(m_va->m_Profile & UMC::VA_PROFILE_REXT) ||
                    ! m_va->IsLongSliceControl())
                    return G9::PackerVAAPI::PackSliceParams(slice, last_slice);

                VAPictureParameterBufferHEVC* pp = nullptr;
                GetParamsBuffer(m_va, &pp);

                VASliceParameterBufferHEVCExtension* sp = nullptr;
                PeekParamsBuffer(m_va, &sp);

                G9::PackerVAAPI::PackSliceParams(reinterpret_cast<VASliceParameterBufferBase*>(sp), slice, last_slice);
                PackSliceHeader(m_va, slice, pp, &sp->rext, last_slice);
                return reinterpret_cast<VASliceParameterBufferBase*>(sp);
            }
        };
    } //G11
}

#endif //UMC_VA_H265_PACKER_G11
