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

#ifndef UMC_VA_H265_PACKER_G12
#define UMC_VA_H265_PACKER_G12

#include "umc_h265_va_packer_vaapi_g11.hpp"
#include "umc_h265_va_packer_common_g12.hpp"

namespace UMC_HEVC_DECODER
{
    namespace G12
    {
        inline
        void PackPicHeader(UMC::VideoAccelerator*, H265DecoderFrame const* frame, H265DBPList const* dpb, VAPictureParameterBufferHEVCScc* pp)
        {
            assert(frame);
            assert(dpb);
            assert(pp);

            (void)dpb;
            auto si = frame->GetAU();
            if (!si)
                throw h265_exception(UMC::UMC_ERR_FAILED);

            auto slice = si->GetSlice(0);
            assert(slice);

            auto pps = slice->GetPicParam();
            assert(pps);
            auto sps = slice->GetSeqParam();
            assert(sps);

            auto& scc_bits = pp->screen_content_pic_fields.bits;
            scc_bits.pps_curr_pic_ref_enabled_flag                   = pps->pps_curr_pic_ref_enabled_flag;
            scc_bits.palette_mode_enabled_flag                       = sps->palette_mode_enabled_flag;
            scc_bits.motion_vector_resolution_control_idc            = sps->motion_vector_resolution_control_idc;
            scc_bits.intra_boundary_filtering_disabled_flag          = sps->intra_boundary_filtering_disabled_flag;
            scc_bits.residual_adaptive_colour_transform_enabled_flag = pps->residual_adaptive_colour_transform_enabled_flag;
            scc_bits.pps_slice_act_qp_offsets_present_flag           = pps->pps_slice_act_qp_offsets_present_flag;

            pp->palette_max_size                                     = sps->palette_max_size;
            pp->delta_palette_max_predictor_size                     = sps->delta_palette_max_predictor_size;

            pp->pps_act_y_qp_offset_plus5                            = static_cast<uint8_t>(pps->pps_act_y_qp_offset  + 5);
            pp->pps_act_cb_qp_offset_plus5                           = static_cast<uint8_t>(pps->pps_act_cb_qp_offset + 5);
            pp->pps_act_cr_qp_offset_plus3                           = static_cast<uint8_t>(pps->pps_act_cr_qp_offset + 3);

            uint32_t const* palette = nullptr;
            size_t count = 0;
            uint8_t const numComps = sps->chroma_format_idc ? 3 : 1;
            if (pps->pps_palette_predictor_initializer_present_flag)
            {
                assert(pps->pps_num_palette_predictor_initializer * numComps == pps->m_paletteInitializers.size());

                count   = pps->pps_num_palette_predictor_initializer;
                palette = pps->m_paletteInitializers.data();
            }
            else if (sps->sps_palette_predictor_initializer_present_flag)
            {
                assert(sps->sps_num_palette_predictor_initializer * numComps == sps->m_paletteInitializers.size());

                count   = sps->sps_num_palette_predictor_initializer;
                palette = sps->m_paletteInitializers.data();
            }

            if (palette)
            {
                pp->predictor_palette_size = static_cast<uint8_t>(count);
                FillPaletteEntries(palette, pp->predictor_palette_entries, count, numComps);
            }
        }

        class PackerVAAPI
            : public G11::PackerVAAPI
        {
        public:

            PackerVAAPI(UMC::VideoAccelerator* va)
                : G11::PackerVAAPI::PackerVAAPI(va)
            {}

        protected:

            void CreateSliceParamBuffer(size_t count) override
            {
                if (!m_va->IsLongSliceControl() || !(m_va->m_Profile & UMC::VA_PROFILE_SCC))
                    G11::PackerVAAPI::CreateSliceParamBuffer(count);
                else
                    CreateParamsBuffer<VASliceParameterBufferHEVCExtension>(m_va, count);
            }

            void PackPicParams(H265DecoderFrame const* frame, TaskSupplier_H265* supplier) override
            {
                assert(frame);
                assert(supplier);

                if (!m_va->IsLongSliceControl() || !(m_va->m_Profile & UMC::VA_PROFILE_SCC))
                    G11::PackerVAAPI::PackPicParams(frame, supplier);
                else
                {
                    H265DBPList const* dpb = supplier->GetDPBList();
                    if (!dpb)
                        throw h265_exception(UMC::UMC_ERR_FAILED);

                    VAPictureParameterBufferHEVCExtension* pp = 0;
                    PeekParamsBuffer(m_va, &pp);

                     G9::PackPicHeader(m_va, frame, dpb, &pp->base);
                    G11::PackPicHeader(m_va, frame, dpb, &pp->rext);
                         PackPicHeader(m_va, frame, dpb, &pp->scc);
                }
            }

            VASliceParameterBufferBase* PackSliceParams(H265Slice const* slice, bool last_slice) override
            {
                VASliceParameterBufferBase* sp_base = nullptr;
                if (!(m_va->m_Profile & UMC::VA_PROFILE_SCC) ||
                    ! m_va->IsLongSliceControl())
                    sp_base = G11::PackerVAAPI::PackSliceParams(slice, last_slice);
                else
                {
                    VAPictureParameterBufferHEVC* pp = nullptr;
                    GetParamsBuffer(m_va, &pp);

                    VASliceParameterBufferHEVCExtension* sp = nullptr;
                    PeekParamsBuffer(m_va, &sp);

                    G9::PackerVAAPI::PackSliceParams(reinterpret_cast<VASliceParameterBufferBase*>(sp), slice, last_slice);

                    //for SCC we need to pack [VASliceParameterBufferHEVCRext]
                    G11::PackSliceHeader(m_va, slice, pp, &sp->rext, last_slice);
                    sp_base = reinterpret_cast<VASliceParameterBufferBase*>(sp);
                }

                auto pps = slice->GetPicParam();
                assert(pps);

                VASliceParameterBufferHEVC* sp = reinterpret_cast<VASliceParameterBufferHEVC*>(sp_base);
                sp->slice_data_num_emu_prevn_bytes = slice->m_NumEmuPrevnBytesInSliceHdr;

                if (pps->tiles_enabled_flag)
                {
                    auto p = GetEntryPoint(slice);
                    sp->num_entry_point_offsets        = p.second;
                    sp->entry_offset_to_subset_array   = p.first;
                }

                if (last_slice && pps->tiles_enabled_flag)
                    PackSubsets(slice->GetCurrentFrame());
                return sp_base;
            }

            void PackSubsets(H265DecoderFrame const* frame)
            {
                assert(frame);

                size_t const count = 22 * 20; //MaxTileRows * MaxTileCols (see ITU-T H.265 Annex A for details)
                auto p = PeekBuffer(m_va, VASubsetsParameterBufferType, count * sizeof(uint32_t));
                auto begin = reinterpret_cast<uint32_t*>(p.first);
                FillSubsets(frame->GetAU(), begin,  begin+ count);
            }
        };
    } //G12

}

#endif //UMC_VA_H265_PACKER_G12
