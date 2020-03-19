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

#ifndef UMC_VA_H265_PACKER_G9
#define UMC_VA_H265_PACKER_G9

namespace UMC_HEVC_DECODER
{
    namespace G9
    {
        template <EnumRefPicList ListX>
        using PicListT = std::integral_constant<EnumRefPicList, ListX>;

        inline
        void PackPicHeader(UMC::VideoAccelerator* va, H265DecoderFrame const* frame, H265DBPList const* dpb, VAPictureParameterBufferHEVC* pp)
        {
            H265DecoderFrameInfo const* si = frame->GetAU();
            if (!si)
                throw h265_exception(UMC::UMC_ERR_FAILED);

            auto slice = si->GetSlice(0);
            assert(slice);

            auto sps = slice->GetSeqParam();
            assert(sps);

            auto pps = slice->GetPicParam();
            assert(pps);

            pp->CurrPic.picture_id    = va->GetSurfaceID(frame->GetFrameMID());
            pp->CurrPic.pic_order_cnt = frame->m_PicOrderCnt;
            pp->CurrPic.flags         = 0;

            uint32_t numRefPicSetStCurrBefore = 0,
                    numRefPicSetStCurrAfter  = 0,
                    numRefPicSetLtCurr       = 0,
                    numRefPicSetStCurr       = 0;

            size_t count = 0;
            for (auto f = dpb->head() ; f && count < sizeof(pp->ReferenceFrames) / sizeof(pp->ReferenceFrames[0]) ; f = f->future())
            {
                int const refType = f->isShortTermRef() ?
                    SHORT_TERM_REFERENCE : (f->isLongTermRef() ? LONG_TERM_REFERENCE : NO_REFERENCE);

                if (refType == NO_REFERENCE)
                    continue;

                if (f != frame)
                {
                    pp->ReferenceFrames[count].pic_order_cnt = f->m_PicOrderCnt;
                    pp->ReferenceFrames[count].picture_id = va->GetSurfaceID(f->GetFrameMID());
                    pp->ReferenceFrames[count].flags = refType == LONG_TERM_REFERENCE ? VA_PICTURE_HEVC_LONG_TERM_REFERENCE : 0;
                    count++;
                }
            }

            if (pps->pps_curr_pic_ref_enabled_flag)
            {
                    pp->ReferenceFrames[count].pic_order_cnt = frame->m_PicOrderCnt;
                    pp->ReferenceFrames[count].picture_id = va->GetSurfaceID(frame->GetFrameMID());
                    pp->ReferenceFrames[count].flags = VA_PICTURE_HEVC_LONG_TERM_REFERENCE;
                    count++;
            }

            for (size_t n = count+numRefPicSetStCurr; n < sizeof(pp->ReferenceFrames) / sizeof(pp->ReferenceFrames[0]); n++)
            {
                pp->ReferenceFrames[n].picture_id = VA_INVALID_SURFACE;
                pp->ReferenceFrames[n].flags = VA_PICTURE_HEVC_INVALID;
            }

            ReferencePictureSet *rps = slice->getRPS();
            int32_t pocList[3*8] {};

            uint32_t index = 0;

            uint32_t NumberOfPictures = rps->getNumberOfNegativePictures();

            for(; index < NumberOfPictures; index++, numRefPicSetStCurrBefore++)
            {
                pocList[index] = pp->CurrPic.pic_order_cnt + rps->getDeltaPOC(index);
            }

            NumberOfPictures += rps->getNumberOfPositivePictures();

            for (; index < NumberOfPictures; index++, numRefPicSetStCurrAfter++)
            {
                pocList[index] = pp->CurrPic.pic_order_cnt + rps->getDeltaPOC(index);
            }

            NumberOfPictures += rps->getNumberOfLongtermPictures();

            for (; index < NumberOfPictures; index++, numRefPicSetLtCurr++)
            {
                auto poc = rps->getPOC(index);
                auto lt = dpb->findLongTermRefPic(frame, poc, sps->log2_max_pic_order_cnt_lsb, !rps->getCheckLTMSBPresent(index));

                if (lt)
                {
                    pocList[index] = lt->PicOrderCnt();
                }
                else
                {
                    pocList[index] = pp->CurrPic.pic_order_cnt + rps->getDeltaPOC(index);
                }
            }

            for (uint32_t n = 0; n < NumberOfPictures; n++)
            {
                if (!rps->getUsed(n))
                    continue;

                for (size_t k = 0; k < count; k++)
                {
                    if(pocList[n] == pp->ReferenceFrames[k].pic_order_cnt)
                    {
                        if(n < numRefPicSetStCurrBefore)
                            pp->ReferenceFrames[k].flags |= VA_PICTURE_HEVC_RPS_ST_CURR_BEFORE;
                        else if(n < numRefPicSetStCurrBefore + numRefPicSetStCurrAfter)
                            pp->ReferenceFrames[k].flags |= VA_PICTURE_HEVC_RPS_ST_CURR_AFTER;
                        else if(n < numRefPicSetStCurrBefore + numRefPicSetStCurrAfter + numRefPicSetLtCurr)
                        {
                            pp->ReferenceFrames[k].flags |= VA_PICTURE_HEVC_RPS_LT_CURR;
                        }
                    }
                }
            }

            auto sh = slice->GetSliceHeader();
            assert(sh);

            pp->pic_width_in_luma_samples                                    = (uint16_t)sps->pic_width_in_luma_samples;
            pp->pic_height_in_luma_samples                                   = (uint16_t)sps->pic_height_in_luma_samples;

            auto& pic_fields = pp->pic_fields.bits;
            pic_fields.chroma_format_idc                                     = sps->chroma_format_idc;
            pic_fields.separate_colour_plane_flag                            = sps->separate_colour_plane_flag;

            pic_fields.pcm_enabled_flag                                      = sps->pcm_enabled_flag;
            pic_fields.scaling_list_enabled_flag                             = sps->scaling_list_enabled_flag;
            pic_fields.transform_skip_enabled_flag                           = pps->transform_skip_enabled_flag;
            pic_fields.amp_enabled_flag                                      = sps->amp_enabled_flag;
            pic_fields.strong_intra_smoothing_enabled_flag                   = sps->sps_strong_intra_smoothing_enabled_flag;

            pic_fields.sign_data_hiding_enabled_flag                         = pps->sign_data_hiding_enabled_flag;
            pic_fields.constrained_intra_pred_flag                           = pps->constrained_intra_pred_flag;
            pic_fields.cu_qp_delta_enabled_flag                              = pps->cu_qp_delta_enabled_flag;
            pic_fields.weighted_pred_flag                                    = pps->weighted_pred_flag;
            pic_fields.weighted_bipred_flag                                  = pps->weighted_bipred_flag;

            pic_fields.transquant_bypass_enabled_flag                        = pps->transquant_bypass_enabled_flag;
            pic_fields.tiles_enabled_flag                                    = pps->tiles_enabled_flag;
            pic_fields.entropy_coding_sync_enabled_flag                      = pps->entropy_coding_sync_enabled_flag;
            pic_fields.pps_loop_filter_across_slices_enabled_flag            = pps->pps_loop_filter_across_slices_enabled_flag;
            pic_fields.loop_filter_across_tiles_enabled_flag                 = pps->loop_filter_across_tiles_enabled_flag;

            pic_fields.pcm_loop_filter_disabled_flag                         = sps->pcm_loop_filter_disabled_flag;
            pic_fields.NoPicReorderingFlag                                   = sps->sps_max_num_reorder_pics[sh->nuh_temporal_id] == 0 ? 1 : 0;
            pic_fields.NoBiPredFlag                                          = 0;

            pp->sps_max_dec_pic_buffering_minus1                             = (uint8_t)(sps->sps_max_dec_pic_buffering[sh->nuh_temporal_id] - 1);
            pp->bit_depth_luma_minus8                                        = (uint8_t)(sps->bit_depth_luma - 8);
            pp->bit_depth_chroma_minus8                                      = (uint8_t)(sps->bit_depth_chroma - 8);
            pp->pcm_sample_bit_depth_luma_minus1                             = (uint8_t)(sps->pcm_sample_bit_depth_luma - 1);
            pp->pcm_sample_bit_depth_chroma_minus1                           = (uint8_t)(sps->pcm_sample_bit_depth_chroma - 1);
            pp->log2_min_luma_coding_block_size_minus3                       = (uint8_t)(sps->log2_min_luma_coding_block_size- 3);
            pp->log2_diff_max_min_luma_coding_block_size                     = (uint8_t)(sps->log2_max_luma_coding_block_size - sps->log2_min_luma_coding_block_size);
            pp->log2_min_transform_block_size_minus2                         = (uint8_t)(sps->log2_min_transform_block_size - 2);
            pp->log2_diff_max_min_transform_block_size                       = (uint8_t)(sps->log2_max_transform_block_size - sps->log2_min_transform_block_size);
            pp->log2_min_pcm_luma_coding_block_size_minus3                   = (uint8_t)(sps->log2_min_pcm_luma_coding_block_size - 3);
            pp->log2_diff_max_min_pcm_luma_coding_block_size                 = (uint8_t)(sps->log2_max_pcm_luma_coding_block_size - sps->log2_min_pcm_luma_coding_block_size);
            pp->max_transform_hierarchy_depth_intra                          = (uint8_t)sps->max_transform_hierarchy_depth_intra;
            pp->max_transform_hierarchy_depth_inter                          = (uint8_t)sps->max_transform_hierarchy_depth_inter;
            pp->init_qp_minus26                                              = pps->init_qp - 26;
            pp->diff_cu_qp_delta_depth                                       = (uint8_t)pps->diff_cu_qp_delta_depth;
            pp->pps_cb_qp_offset                                             = (uint8_t)pps->pps_cb_qp_offset;
            pp->pps_cr_qp_offset                                             = (uint8_t)pps->pps_cr_qp_offset;
            pp->log2_parallel_merge_level_minus2                             = (uint8_t)(pps->log2_parallel_merge_level - 2);

            if (pps->tiles_enabled_flag)
            {
                pp->num_tile_columns_minus1 = std::min<uint8_t>(sizeof(pp->column_width_minus1) /sizeof(pp->column_width_minus1[0]), (uint8_t)(pps->num_tile_columns - 1));
                pp->num_tile_rows_minus1    = std::min<uint8_t>(sizeof(pp->row_height_minus1) /sizeof(pp->row_height_minus1[0]),     (uint8_t)(pps->num_tile_rows    - 1));

                for (uint32_t i = 0; i <= pp->num_tile_columns_minus1; i++)
                    pp->column_width_minus1[i] = (uint16_t)(pps->column_width[i] - 1);

                for (uint32_t i = 0; i <= pp->num_tile_rows_minus1; i++)
                    pp->row_height_minus1[i] = (uint16_t)(pps->row_height[i] - 1);
            }

            auto& slice_parsing_fields = pp->slice_parsing_fields.bits;
            slice_parsing_fields.lists_modification_present_flag             = pps->lists_modification_present_flag;
            slice_parsing_fields.long_term_ref_pics_present_flag             = sps->long_term_ref_pics_present_flag;
            slice_parsing_fields.sps_temporal_mvp_enabled_flag               = sps->sps_temporal_mvp_enabled_flag;
            slice_parsing_fields.cabac_init_present_flag                     = pps->cabac_init_present_flag;
            slice_parsing_fields.output_flag_present_flag                    = pps->output_flag_present_flag;
            slice_parsing_fields.dependent_slice_segments_enabled_flag       = pps->dependent_slice_segments_enabled_flag;
            slice_parsing_fields.pps_slice_chroma_qp_offsets_present_flag    = pps->pps_slice_chroma_qp_offsets_present_flag;
            slice_parsing_fields.sample_adaptive_offset_enabled_flag         = sps->sample_adaptive_offset_enabled_flag;
            slice_parsing_fields.deblocking_filter_override_enabled_flag     = pps->deblocking_filter_override_enabled_flag;
            slice_parsing_fields.pps_disable_deblocking_filter_flag          = pps->pps_deblocking_filter_disabled_flag;
            slice_parsing_fields.slice_segment_header_extension_present_flag = pps->slice_segment_header_extension_present_flag;

            slice_parsing_fields.RapPicFlag =
                (sh->nal_unit_type >= NAL_UT_CODED_SLICE_BLA_W_LP && sh->nal_unit_type <= NAL_UT_CODED_SLICE_CRA) ? 1 : 0;
            slice_parsing_fields.IdrPicFlag                                  = sh->IdrPicFlag;
            slice_parsing_fields.IntraPicFlag                                = si->IsIntraAU() ? 1 : 0;

            pp->log2_max_pic_order_cnt_lsb_minus4                            = (uint8_t)(sps->log2_max_pic_order_cnt_lsb - 4);
            pp->num_short_term_ref_pic_sets                                  = (uint8_t)sps->getRPSList()->getNumberOfReferencePictureSets();
            pp->num_long_term_ref_pic_sps                                    = (uint8_t)sps->num_long_term_ref_pics_sps;
            pp->num_ref_idx_l0_default_active_minus1                         = (uint8_t)(pps->num_ref_idx_l0_default_active - 1);
            pp->num_ref_idx_l1_default_active_minus1                         = (uint8_t)(pps->num_ref_idx_l1_default_active - 1);
            pp->pps_beta_offset_div2                                         = (int8_t)(pps->pps_beta_offset >> 1);
            pp->pps_tc_offset_div2                                           = (int8_t)(pps->pps_tc_offset >> 1);
            pp->num_extra_slice_header_bits                                  = (uint8_t)pps->num_extra_slice_header_bits;

            pp->st_rps_bits =                                                slice->GetSliceHeader()->wNumBitsForShortTermRPSInSlice;
        }

        template <EnumRefPicList ListX>
        inline
        void FillRPL(UMC::VideoAccelerator* va, H265Slice const* slice, VAPictureParameterBufferHEVC const* pp, VASliceParameterBufferHEVC* sp, PicListT<ListX>)
        {
            assert(slice);
            assert(pp);
            assert(sp);

            auto frame = slice->GetCurrentFrame();
            assert(frame);

            auto rpl = frame->GetRefPicList(slice->GetSliceNum(), ListX);
            if (!rpl)
                throw h265_exception(UMC::UMC_ERR_FAILED);

            size_t const num_active_ref = slice->getNumRefIdx(ListX);
            size_t const max_num_ref = std::extent<decltype(pp->ReferenceFrames)>::value;

            size_t index = 0;
            for (size_t i = 0; i < num_active_ref; ++i)
            {
                auto const& frameInfo = rpl->m_refPicList[i];
                if (!frameInfo.refFrame)
                    break;
                else
                {
                    auto id = static_cast<VASurfaceID>(va->GetSurfaceID(frameInfo.refFrame->GetFrameMID()));
                    auto r = std::find_if(pp->ReferenceFrames, pp->ReferenceFrames + max_num_ref,
                        [id](VAPictureHEVC const& p) { return p.picture_id == id; }
                    );

                    if (r != pp->ReferenceFrames + max_num_ref)
                    {
                        sp->RefPicList[ListX][index] = std::distance(pp->ReferenceFrames, r);
                        ++index;
                    }
                }
            }

            for (size_t i = index; i < max_num_ref; ++i)
                sp->RefPicList[ListX][i] = 0xff;
        }

        inline
        void PackSliceHeader(UMC::VideoAccelerator*, H265Slice const*, VAPictureParameterBufferHEVC const*, VASliceParameterBufferBase*, bool)
        { }

        inline
        void PackSliceHeader(UMC::VideoAccelerator* va, H265Slice const* slice, VAPictureParameterBufferHEVC const* pp, VASliceParameterBufferHEVC* sp, bool last_slice)
        {
            do {
            if (!slice) break;
            if (!pp) break;
            if (!sp) break;

            auto sh = slice->GetSliceHeader();
            if (!sh) break;

            sp->slice_data_byte_offset = uint32_t(slice->m_BitStream.BytesDecoded() + 3 /* start code */);
            sp->slice_segment_address  = sh->slice_segment_address;

            auto frame = slice->GetCurrentFrame();
            if (!frame) break;

            FillRPL(va, slice, pp, sp, PicListT<REF_PIC_LIST_0>{});
            FillRPL(va, slice, pp, sp, PicListT<REF_PIC_LIST_1>{});

            auto& LongSliceFlags = sp->LongSliceFlags.fields;
            LongSliceFlags.LastSliceOfPic = last_slice ? 1 : 0;
            LongSliceFlags.dependent_slice_segment_flag                 = sh->dependent_slice_segment_flag;
            LongSliceFlags.slice_type                                   = sh->slice_type;
            LongSliceFlags.color_plane_id                               = sh->colour_plane_id;
            LongSliceFlags.slice_sao_luma_flag                          = sh->slice_sao_luma_flag;
            LongSliceFlags.slice_sao_chroma_flag                        = sh->slice_sao_chroma_flag;
            LongSliceFlags.mvd_l1_zero_flag                             = sh->mvd_l1_zero_flag;
            LongSliceFlags.cabac_init_flag                              = sh->cabac_init_flag;
            LongSliceFlags.slice_temporal_mvp_enabled_flag              = sh->slice_temporal_mvp_enabled_flag;
            LongSliceFlags.slice_deblocking_filter_disabled_flag        = sh->slice_deblocking_filter_disabled_flag;
            LongSliceFlags.collocated_from_l0_flag                      = sh->collocated_from_l0_flag;
            LongSliceFlags.slice_loop_filter_across_slices_enabled_flag = sh->slice_loop_filter_across_slices_enabled_flag;

            auto pps = slice->GetPicParam();
            if (!pps) break;

            sp->collocated_ref_idx =
                (uint8_t)((sh->slice_type != I_SLICE) ?  sh->collocated_ref_idx : -1);
            sp->num_ref_idx_l0_active_minus1                            = (uint8_t)(slice->getNumRefIdx(REF_PIC_LIST_0) - 1);
            sp->num_ref_idx_l1_active_minus1                            = (uint8_t)(slice->getNumRefIdx(REF_PIC_LIST_1) - 1);
            sp->slice_qp_delta                                          = (int8_t)(sh->SliceQP - pps->init_qp);
            sp->slice_cb_qp_offset                                      = (int8_t)sh->slice_cb_qp_offset;
            sp->slice_cr_qp_offset                                      = (int8_t)sh->slice_cr_qp_offset;
            sp->slice_beta_offset_div2                                  = (int8_t)(sh->slice_beta_offset >> 1);
            sp->slice_tc_offset_div2                                    = (int8_t)(sh->slice_tc_offset >> 1);

            sp->luma_log2_weight_denom                                  = (uint8_t)sh->luma_log2_weight_denom;
            sp->delta_chroma_log2_weight_denom                          = (uint8_t)(sh->chroma_log2_weight_denom - sh->luma_log2_weight_denom);

            for (int32_t l = 0; l < 2; l++)
            {
                EnumRefPicList eRefPicList = ( l == 1 ? REF_PIC_LIST_1 : REF_PIC_LIST_0 );
                for (int32_t iRefIdx = 0; iRefIdx < slice->getNumRefIdx(eRefPicList); iRefIdx++)
                {
                    wpScalingParam const* wp = sh->pred_weight_table[eRefPicList][iRefIdx];

                    if (eRefPicList == REF_PIC_LIST_0)
                    {
                        sp->luma_offset_l0[iRefIdx]       = (int8_t)wp[0].offset;
                        sp->delta_luma_weight_l0[iRefIdx] = (int8_t)wp[0].delta_weight;
                        for(int chroma=0;chroma < 2;chroma++)
                        {
                            sp->delta_chroma_weight_l0[iRefIdx][chroma] = (int8_t)wp[1 + chroma].delta_weight;
                            sp->ChromaOffsetL0        [iRefIdx][chroma] = (int8_t)wp[1 + chroma].offset;
                        }
                    }
                    else
                    {
                        sp->luma_offset_l1[iRefIdx]       = (int8_t)wp[0].offset;
                        sp->delta_luma_weight_l1[iRefIdx] = (int8_t)wp[0].delta_weight;
                        for(int chroma=0;chroma < 2;chroma++)
                        {
                            sp->delta_chroma_weight_l1[iRefIdx][chroma] = (int8_t)wp[1 + chroma].delta_weight;
                            sp->ChromaOffsetL1        [iRefIdx][chroma] = (int8_t)wp[1 + chroma].offset;
                        }
                    }
                }
            }

            sp->five_minus_max_num_merge_cand = (uint8_t)(5 - sh->max_num_merge_cand);

            } while (0);
        }

        template <EnumRefPicList ListX>
        inline
        void SanitizeReferenceFrames(H265Slice const* slice, VASliceParameterBufferHEVC const* sp, VAPictureParameterBufferHEVC* pp, PicListT<ListX>)
        {
            assert(slice);
            assert(sp);
            assert(pp);

            size_t const max_num_ref = std::extent<decltype(pp->ReferenceFrames)>::value;
            size_t const num_valid_ref = std::distance(
                sp->RefPicList[ListX],
                std::find(sp->RefPicList[ListX], sp->RefPicList[ListX] + max_num_ref, 0xff)
            );

            size_t const num_active_ref = slice->getNumRefIdx(ListX);
            if (num_valid_ref >= num_active_ref)
                return;

            for (auto i = num_valid_ref; i < num_active_ref; ++i)
            {
                //try to find 'Foll' reference
                auto r = std::find_if(pp->ReferenceFrames, pp->ReferenceFrames + max_num_ref,
                    [](VAPictureHEVC const& p) { return p.picture_id != VA_INVALID_SURFACE && p.flags == 0; }
                );

                if (r == pp->ReferenceFrames + max_num_ref)
                    //no 'Foll' reference found, can't do anymore
                    break;

                //make 'Foll' to be 'Before/After' reference
                r->flags |=
                    ListX == REF_PIC_LIST_0 ? VA_PICTURE_HEVC_RPS_ST_CURR_BEFORE : VA_PICTURE_HEVC_RPS_ST_CURR_AFTER;
            }
        }

        class PackerVAAPI
            : public UMC_HEVC_DECODER::PackerVAAPI
        {
        public:

            PackerVAAPI(UMC::VideoAccelerator* va)
                : UMC_HEVC_DECODER::PackerVAAPI(va)
            {}

        protected:

            void CreateSliceParamBuffer(size_t count) override
            {
                m_va->IsLongSliceControl() ?
                    CreateParamsBuffer<VASliceParameterBufferHEVC>(m_va, count) :
                    CreateParamsBuffer<VASliceParameterBufferBase>(m_va, count)
                    ;
            }

            void PackPicParams(H265DecoderFrame const* frame, TaskSupplier_H265* supplier) override
            {
                assert(frame);
                assert(supplier);

                H265DBPList const* dpb = supplier->GetDPBList();
                if (!dpb)
                    throw h265_exception(UMC::UMC_ERR_FAILED);

                VAPictureParameterBufferHEVC* pp = nullptr;
                PeekParamsBuffer(m_va, &pp);
                PackPicHeader(m_va, frame, dpb, pp);
            }

            VASliceParameterBufferBase* PackSliceParams(H265Slice const* slice, bool last_slice) override
            {
                assert(slice);
                VASliceParameterBufferBase* sp_base = nullptr;
                if (!m_va->IsLongSliceControl())
                    PeekParamsBuffer(m_va, &sp_base);
                else
                {
                    VASliceParameterBufferHEVC* sp = nullptr;
                    PeekParamsBuffer(m_va, &sp);
                    sp_base = reinterpret_cast<VASliceParameterBufferBase*>(sp);
                }

                PackSliceParams(sp_base, slice, last_slice);

                return sp_base;
            }

            void PackSliceParams(VASliceParameterBufferBase* sp_base, H265Slice const* slice, bool last_slice) override
            {
                assert(sp_base);
                assert(slice);

                if (m_va->IsLongSliceControl())
                {
                    VAPictureParameterBufferHEVC* pp = nullptr;
                    GetParamsBuffer(m_va, &pp);

                    auto sp = reinterpret_cast<VASliceParameterBufferHEVC*>(sp_base);
                    PackSliceHeader(m_va, slice, pp, sp, last_slice);

                    SanitizeReferenceFrames(slice, sp, pp, PicListT<REF_PIC_LIST_0>{});
                    SanitizeReferenceFrames(slice, sp, pp, PicListT<REF_PIC_LIST_1>{});
                }

                auto bs = slice->GetBitStream();
                assert(bs);

                uint32_t size = 0;
                uint32_t* ptr = 0;
                bs->GetOrg(&ptr, &size);
                auto src = reinterpret_cast<uint8_t const*>(ptr);

                static uint8_t constexpr start_code[] = { 0, 0, 1 };
                uint8_t* dst = nullptr;
                auto offset = PeekSliceDataBuffer(m_va, &dst, size + sizeof(start_code));
                if (!dst)
                    throw h265_exception(UMC::UMC_ERR_FAILED);

                // auto xxx = dst;
                // copy slice data to slice data buffer
                std::copy(start_code, start_code + sizeof(start_code), dst);
                dst += sizeof(start_code);
                std::copy(src, src + size, dst);

                sp_base->slice_data_size   = size + sizeof(start_code);
                sp_base->slice_data_offset = offset;
                sp_base->slice_data_flag   = VA_SLICE_DATA_FLAG_ALL;

            }
        };
    } //G9
}
#endif //UMC_VA_H265_PACKER_G9
