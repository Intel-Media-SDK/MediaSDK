// Copyright (c) 2017-2020 Intel Corporation
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

#include "umc_defs.h"
#ifdef MFX_ENABLE_AV1_VIDEO_DECODE

#include <algorithm>
#include <cmath>
#include "vm_debug.h"

#include "umc_av1_bitstream_utils.h"
#include "umc_av1_utils.h"
#include "umc_av1_frame.h"

//#define AV1D_LOGGING
#if defined(AV1D_LOGGING) && (defined(WIN32) || defined(WIN64))
#define AV1D_LOG(string, ...) do { printf("(AV1D log) %s ",__FUNCTION__); printf(string, ##__VA_ARGS__); printf("\n"); fflush(0); } \
                              while (0)
#else
#define AV1D_LOG(string, ...)
#endif

using UMC_VP9_DECODER::AlignPowerOfTwo;

namespace UMC_AV1_DECODER
{
    inline
    bool av1_sync_code(AV1Bitstream& bs)
    {
        return
            bs.GetBits(8) == SYNC_CODE_0 &&
            bs.GetBits(8) == SYNC_CODE_1 &&
            bs.GetBits(8) == SYNC_CODE_2
            ;
    }

    inline
    uint32_t av1_profile(AV1Bitstream& bs)
    {
        AV1D_LOG("[+]: %d", (uint32_t)bs.BitsDecoded());
        uint32_t profile = bs.GetBit();
        profile |= bs.GetBit() << 1;
        if (profile > 2)
            profile += bs.GetBit();

        AV1D_LOG("[-]: %d", (uint32_t)bs.BitsDecoded());
        return profile;
    }

    inline void av1_decoder_model_info(AV1Bitstream& bs, DecoderModelInfo& info)
    {
        info.buffer_delay_length_minus_1 = bs.GetBits(5);
        info.num_units_in_decoding_tick = bs.GetBits(32);
        info.buffer_removal_time_length_minus_1 = bs.GetBits(5);
        info.frame_presentation_time_length_minus_1 = bs.GetBits(5);
    }

    inline void av1_operating_parameters_info(AV1Bitstream& bs, OperatingParametersInfo& info, uint32_t length)
    {
        info.decoder_buffer_delay = bs.GetBits(length);
        info.encoder_buffer_delay = bs.GetBits(length);
        info.low_delay_mode_flag = bs.GetBit();
    }

    inline void av1_timing_info(AV1Bitstream& bs, TimingInfo& info)
    {
        info.num_units_in_display_tick = bs.GetBits(32);
        info.time_scale = bs.GetBits(32);
        info.equal_picture_interval = bs.GetBit();
        if (info.equal_picture_interval)
            info.num_ticks_per_picture_minus_1 = read_uvlc(bs);
    }

    static void av1_color_config(AV1Bitstream& bs, ColorConfig& config, uint32_t profile)
    {
        AV1D_LOG("[+]: %d", (uint32_t)bs.BitsDecoded());

        config.BitDepth = bs.GetBit() ? 10 : 8;
        if (profile == 2 && config.BitDepth != 8)
            config.BitDepth = bs.GetBit() ? 12 : 10;

        config.mono_chrome = profile != 1 ? bs.GetBit() : 0;

        uint32_t color_description_present_flag = bs.GetBit();
        if (color_description_present_flag)
        {
            config.color_primaries = bs.GetBits(8);
            config.transfer_characteristics = bs.GetBits(8);
            config.matrix_coefficients = bs.GetBits(8);
        }
        else
        {
            config.color_primaries = AOM_CICP_CP_UNSPECIFIED;
            config.transfer_characteristics = AOM_CICP_TC_UNSPECIFIED;
            config.matrix_coefficients = AOM_CICP_MC_UNSPECIFIED;
        }

        if (config.mono_chrome)
        {
            config.color_range = bs.GetBit();
            config.subsampling_y = config.subsampling_x = 1;
            config.chroma_sample_position = AOM_CSP_UNKNOWN;
            config.separate_uv_delta_q = 0;

            AV1D_LOG("[-]: %d", (uint32_t)bs.BitsDecoded());

            return;
        }

        if (config.color_primaries == AOM_CICP_CP_BT_709 &&
            config.transfer_characteristics == AOM_CICP_TC_SRGB &&
            config.matrix_coefficients == AOM_CICP_MC_IDENTITY)
        {
            config.color_range = AOM_CR_FULL_RANGE;
            config.subsampling_y = config.subsampling_x = 0;
            if (!(profile == 1 || (profile == 2 && config.BitDepth == 12)))
                throw av1_exception(UMC::UMC_ERR_INVALID_STREAM);
        }
        else
        {
            // [16,235] (including xvycc) vs [0,255] range
            config.color_range = bs.GetBit();
            if (profile == 0) // 420 only
                config.subsampling_x = config.subsampling_y = 1;
            else if (profile == 1) // 444 only
                config.subsampling_x = config.subsampling_y = 0;
            else if (profile == 2)
            {
                if (config.BitDepth == 12)
                {
                    config.subsampling_x = bs.GetBit();
                    if (config.subsampling_x == 0)
                        config.subsampling_y = 0;  // 444
                    else
                        config.subsampling_y = bs.GetBit();  // 422 or 420
                }
                else
                {
                    // 422
                    config.subsampling_x = 1;
                    config.subsampling_y = 0;
                }
            }
            if (config.subsampling_x == 1 && config.subsampling_y == 1)
                config.chroma_sample_position = bs.GetBits(2);
        }

        config.separate_uv_delta_q = bs.GetBit();

        AV1D_LOG("[-]: %d", (uint32_t)bs.BitsDecoded());
    }

    inline void av1_set_mi_and_sb_size(FrameHeader& fh, SequenceHeader const& sh)
    {
        // set frame width and heignt in MI
        const unsigned int alignedWidth = AlignPowerOfTwo(fh.FrameWidth, 3);
        const int alignedHeight = AlignPowerOfTwo(fh.FrameHeight, 3);
        fh.MiCols = alignedWidth >> MI_SIZE_LOG2;
        fh.MiRows = alignedHeight >> MI_SIZE_LOG2;

        // set frame width and height in SB
        const uint32_t mibSizeLog2 = sh.sbSize == BLOCK_64X64 ? 4 : 5;
        const unsigned int widthMI = AlignPowerOfTwo(fh.MiCols, mibSizeLog2);
        const unsigned int heightMI = AlignPowerOfTwo(fh.MiRows, mibSizeLog2);
        fh.sbCols = widthMI >> mibSizeLog2;
        fh.sbRows = heightMI >> mibSizeLog2;
    }

    static void av1_setup_superres(AV1Bitstream& bs, FrameHeader& fh, SequenceHeader const& sh)
    {
        AV1D_LOG("[+]: %d", (uint32_t)bs.BitsDecoded());

        fh.UpscaledWidth = fh.FrameWidth;
        fh.SuperresDenom = SCALE_NUMERATOR;

        if (!sh.enable_superres)
        {
            AV1D_LOG("[-]: %d", (uint32_t)bs.BitsDecoded());
            return;
        }

        if (bs.GetBit())
        {
            uint32_t denom = bs.GetBits(SUPERRES_SCALE_BITS);
            denom += SUPERRES_SCALE_DENOMINATOR_MIN;
            if (denom != SCALE_NUMERATOR)
                fh.FrameWidth = (fh.FrameWidth * SCALE_NUMERATOR + denom / 2) / (denom);

            fh.SuperresDenom = denom;
        }

        AV1D_LOG("[-]: %d", (uint32_t)bs.BitsDecoded());
    }

    inline
    void av1_setup_render_size(AV1Bitstream& bs, FrameHeader& fh)
    {
        AV1D_LOG("[+]: %d", (uint32_t)bs.BitsDecoded());

        fh.RenderWidth = fh.UpscaledWidth;
        fh.RenderHeight = fh.FrameHeight;

        if (bs.GetBit())
        {
            fh.RenderWidth = bs.GetBits(16) + 1;
            fh.RenderHeight = bs.GetBits(16) + 1;
        }

        AV1D_LOG("[-]: %d", (uint32_t)bs.BitsDecoded());
    }

    inline
    void av1_setup_frame_size(AV1Bitstream& bs, FrameHeader& fh, SequenceHeader const& sh)
    {
        AV1D_LOG("[+]: %d", (uint32_t)bs.BitsDecoded());

        if (fh.frame_size_override_flag)
        {
            fh.FrameWidth = bs.GetBits(sh.frame_width_bits) + 1;
            fh.FrameHeight = bs.GetBits(sh.frame_height_bits) + 1;
            if (fh.FrameWidth > sh.max_frame_width || fh.FrameHeight > sh.max_frame_height)
                throw av1_exception(UMC::UMC_ERR_INVALID_STREAM);
        }
        else
        {
            fh.FrameWidth = sh.max_frame_width;
            fh.FrameHeight = sh.max_frame_height;
        }

        av1_setup_superres(bs, fh, sh);
        av1_set_mi_and_sb_size(fh, sh);
        av1_setup_render_size(bs, fh);

        AV1D_LOG("[-]: %d", (uint32_t)bs.BitsDecoded());
    }

    inline void av1_setup_frame_size_with_refs(AV1Bitstream& bs, FrameHeader& fh, DPBType const& frameDpb, SequenceHeader const& sh)
    {
        AV1D_LOG("[+]: %d", (uint32_t)bs.BitsDecoded());

        bool bFound = false;
        for (uint8_t i = 0; i < INTER_REFS; ++i)
        {
            if (bs.GetBit())
            {
                bFound = true;
                FrameHeader const& refHrd = frameDpb[fh.ref_frame_idx[i]]->GetFrameHeader();
                fh.FrameWidth = refHrd.UpscaledWidth;
                fh.FrameHeight = refHrd.FrameHeight;
                fh.RenderWidth = refHrd.RenderWidth;
                fh.RenderHeight = refHrd.RenderHeight;
                av1_setup_superres(bs, fh, sh);
                break;
            }
        }

        if (!bFound)
        {
            // here AV1 spec 1.0 contradicts to reference implementation:
            // AV1 spec calls frame_size() which internally checks frame_size_override_flag before reading width/heignt from bs
            // reference implementation reads width/height from bs right away w/o preliminary check of frame_size_override_flag
            // current code mimics behavior of reference implementation
            fh.FrameWidth = bs.GetBits(sh.frame_width_bits) + 1;
            fh.FrameHeight = bs.GetBits(sh.frame_height_bits) + 1;
            av1_setup_superres(bs, fh, sh);
            av1_setup_render_size(bs, fh);
        }

        av1_set_mi_and_sb_size(fh, sh);

        AV1D_LOG("[-]: %d", (uint32_t)bs.BitsDecoded());
    }

    inline
    void av1_setup_loop_filter(AV1Bitstream& bs, LoopFilterParams& params, FrameHeader const& fh)
    {
        AV1D_LOG("[+]: %d", (uint32_t)bs.BitsDecoded());

        if (fh.CodedLossless || fh.allow_intrabc)
        {
            SetDefaultLFParams(params);
            AV1D_LOG("[-]: %d", (uint32_t)bs.BitsDecoded());
            return;
        }

        params.loop_filter_level[0] = bs.GetBits(6);
        params.loop_filter_level[1] = bs.GetBits(6);

        if (fh.NumPlanes > 1)
        {
            if (params.loop_filter_level[0] || params.loop_filter_level[1])
            {
                params.loop_filter_level[2] = bs.GetBits(6);
                params.loop_filter_level[3] = bs.GetBits(6);
            }
        }

        params.loop_filter_sharpness = bs.GetBits(3);

        params.loop_filter_delta_update = 0;

        params.loop_filter_delta_enabled = (uint8_t)bs.GetBit();
        if (params.loop_filter_delta_enabled)
        {
            params.loop_filter_delta_update = (uint8_t)bs.GetBit();

            if (params.loop_filter_delta_update)
            {
                uint8_t bits = 6;
                for (uint32_t i = 0; i < TOTAL_REFS; i++)
                {
                    if (bs.GetBit())
                    {
                        const uint8_t nbits = sizeof(unsigned) * 8 - bits - 1;
                        const uint32_t value = (unsigned)bs.GetBits(bits + 1) << nbits;
                        params.loop_filter_ref_deltas[i] = (int8_t)(((int32_t)value) >> nbits);
                    }
                }

                for (uint32_t i = 0; i < MAX_MODE_LF_DELTAS; i++)
                {
                    if (bs.GetBit())
                    {
                        const uint8_t nbits = sizeof(unsigned) * 8 - bits - 1;
                        const uint32_t value = (unsigned)bs.GetBits(bits + 1) << nbits;
                        params.loop_filter_mode_deltas[i] = (int8_t)(((int32_t)value) >> nbits);
                    }
                }
            }
        }

        AV1D_LOG("[-]: %d", (uint32_t)bs.BitsDecoded());
    }

    inline
    void av1_read_cdef(AV1Bitstream& bs, CdefParams& params, SequenceHeader const& sh, FrameHeader const& fh)
    {
        AV1D_LOG("[+]: %d", (uint32_t)bs.BitsDecoded());

        if (fh.CodedLossless || fh.allow_intrabc || !sh.enable_cdef)
        {
            params.cdef_damping = 3;
            std::fill_n(params.cdef_y_strength, std::extent<decltype(params.cdef_y_strength)>::value, 0);
            std::fill_n(params.cdef_uv_strength, std::extent<decltype(params.cdef_uv_strength)>::value, 0);

            AV1D_LOG("[-]: %d", (uint32_t)bs.BitsDecoded());

            return;
        }

        params.cdef_damping = bs.GetBits(2) + 3;
        params.cdef_bits = bs.GetBits(2);
        uint32_t numStrengths = 1 << params.cdef_bits;
        for (uint8_t i = 0; i < numStrengths; i++)
        {
            params.cdef_y_pri_strength[i] = bs.GetBits(4);
            params.cdef_y_sec_strength[i] = bs.GetBits(2);

            if (fh.NumPlanes > 1)
            {
                params.cdef_uv_pri_strength[i] = bs.GetBits(4);
                params.cdef_uv_sec_strength[i] = bs.GetBits(2);
            }
        }

        AV1D_LOG("[-]: %d", (uint32_t)bs.BitsDecoded());
    }

    inline
    void av1_read_lr_params(AV1Bitstream& bs, LRParams& params, SequenceHeader const& sh, FrameHeader const& fh)
    {
        AV1D_LOG("[+]: %d", (uint32_t)bs.BitsDecoded());

        if (fh.AllLossless || fh.allow_intrabc || !sh.enable_restoration)
        {
            AV1D_LOG("[-]: %d", (uint32_t)bs.BitsDecoded());
            return;
        }

        bool usesLR = false;
        bool usesChromaLR = false;

        for (uint8_t p = 0; p < fh.NumPlanes; ++p)
        {
            if (bs.GetBit()) {
                params.lr_type[p] =
                    bs.GetBit() ? RESTORE_SGRPROJ : RESTORE_WIENER;
            }
            else
            {
                params.lr_type[p] =
                    bs.GetBit() ? RESTORE_SWITCHABLE : RESTORE_NONE;
            }
            if (params.lr_type[p] != RESTORE_NONE)
            {
                usesLR = true;
                if (p != 0)
                    usesChromaLR = true;
            }
        }

        if (usesLR)
        {
            if (sh.sbSize == BLOCK_128X128)
                params.lr_unit_shift = bs.GetBit() + 1;
            else
            {
                params.lr_unit_shift = bs.GetBit();
                if (params.lr_unit_shift)
                    params.lr_unit_shift += bs.GetBit(); // lr_unit_extra_shift
            }

            if (sh.color_config.subsampling_x && sh.color_config.subsampling_y && usesChromaLR)
                params.lr_uv_shift = bs.GetBit();
        }

        AV1D_LOG("[-]: %d", (uint32_t)bs.BitsDecoded());
    }

    inline
    void av1_read_tx_mode(AV1Bitstream& bs, FrameHeader& fh, bool allLosless)
    {
        AV1D_LOG("[+]: %d", (uint32_t)bs.BitsDecoded());

        if (allLosless)
            fh.TxMode = ONLY_4X4;
        else
            fh.TxMode = bs.GetBit() ? TX_MODE_SELECT : TX_MODE_LARGEST;

        AV1D_LOG("[-]: %d", (uint32_t)bs.BitsDecoded());
    }


    inline
    void av1_read_frame_reference_mode(AV1Bitstream& bs, FrameHeader& fh, DPBType const&)
    {
        AV1D_LOG("[+]: %d", (uint32_t)bs.BitsDecoded());

        fh.reference_mode = SINGLE_REFERENCE;

        if (!FrameIsIntra(fh))
        {
            fh.reference_mode = bs.GetBit() ? REFERENCE_MODE_SELECT :
                SINGLE_REFERENCE;
        }

        AV1D_LOG("[-]: %d", (uint32_t)bs.BitsDecoded());
    }

    inline
    void av1_read_compound_tools(AV1Bitstream& bs, FrameHeader& fh)
    {
        AV1D_LOG("[+]: %d", (uint32_t)bs.BitsDecoded());

        if (!FrameIsIntra(fh))
            fh.enable_interintra_compound = bs.GetBit();

        if (!FrameIsIntra(fh) && fh.reference_mode != SINGLE_REFERENCE)
            fh.enable_masked_compound = bs.GetBit();

        AV1D_LOG("[-]: %d", (uint32_t)bs.BitsDecoded());
    }

    inline int32_t av1_get_relative_dist(SequenceHeader const& sh, const uint32_t a, const uint32_t b)
    {
        if (!sh.enable_order_hint)
            return 0;

        const uint32_t bits = sh.order_hint_bits_minus1 + 1;

        int32_t diff = a - b;
        int32_t m = 1 << (bits - 1);
        diff = (diff & (m - 1)) - (diff & m);
        return diff;
    }

    static bool av1_is_skip_mode_allowed(FrameHeader const& fh, SequenceHeader const& sh, DPBType const& frameDpb)
    {
        if (!sh.enable_order_hint
            || FrameIsIntra(fh)
            || fh.reference_mode == SINGLE_REFERENCE)
            return false;

        const int32_t MAX_VALUE = (std::numeric_limits<int32_t>::max)();
        const int32_t INVALID_IDX = -1;

        int32_t refFrameOffset[2] = { -1, MAX_VALUE };
        int refIdx[2] = { INVALID_IDX, INVALID_IDX };

        // Identify the nearest forward and backward references.
        for (int32_t i = 0; i < INTER_REFS; ++i)
        {
            FrameHeader const& refHdr = frameDpb[fh.ref_frame_idx[i]]->GetFrameHeader();
            const uint32_t refOrderHint = refHdr.order_hint;

            if (av1_get_relative_dist(sh, refOrderHint, fh.order_hint) < 0)
            {
                // Forward reference
                if (refFrameOffset[0] == -1 ||
                    av1_get_relative_dist(sh, refOrderHint, refFrameOffset[0]) > 0)
                {
                    refFrameOffset[0] = refOrderHint;
                    refIdx[0] = i;
                }
            }
            else if (av1_get_relative_dist(sh, refOrderHint, fh.order_hint) > 0)
            {
                // Backward reference
                if (refFrameOffset[1] == MAX_VALUE ||
                    av1_get_relative_dist(sh, refOrderHint, refFrameOffset[1]) < 0)
                {
                    refFrameOffset[1] = refOrderHint;
                    refIdx[1] = i;
                }
            }
        }

        if (refIdx[0] != INVALID_IDX && refIdx[1] != INVALID_IDX)
        {
            return true;
        }
        else if (refIdx[0] != INVALID_IDX && refIdx[1] == INVALID_IDX)
        {
            // == Forward prediction only ==
            // Identify the second nearest forward reference.
            refFrameOffset[1] = -1;
            for (int i = 0; i < INTER_REFS; ++i)
            {
                FrameHeader const& refHdr = frameDpb[fh.ref_frame_idx[i]]->GetFrameHeader();
                const uint32_t refOrderHint = refHdr.order_hint;

                if ((refFrameOffset[0] >= 0 &&
                    av1_get_relative_dist(sh, refOrderHint, refFrameOffset[0]) < 0) &&
                    (refFrameOffset[1] < 0 ||
                        av1_get_relative_dist(sh, refOrderHint, refFrameOffset[1]) > 0))
                {
                    // Second closest forward reference
                    refFrameOffset[1] = refOrderHint;
                    refIdx[1] = i;
                }
            }

            if (refFrameOffset[1] >= 0)
                return true;
        }

        return false;
    }

    inline
    int32_t av1_read_q_delta(AV1Bitstream& bs)
    {
        return (bs.GetBit()) ?
            read_inv_signed_literal(bs, 6) : 0;
    }

    inline
    void av1_setup_quantization(AV1Bitstream& bs, QuantizationParams& params, SequenceHeader const& sh, int numPlanes)
    {
        AV1D_LOG("[+]: %d", (uint32_t)bs.BitsDecoded());

        params.base_q_idx = bs.GetBits(UMC_VP9_DECODER::QINDEX_BITS);

        params.DeltaQYDc = av1_read_q_delta(bs);

        if (numPlanes > 1)
        {
            int32_t diffUVDelta = 0;
            if (sh.color_config.separate_uv_delta_q)
                diffUVDelta = bs.GetBit();

            params.DeltaQUDc = av1_read_q_delta(bs);
            params.DeltaQUAc = av1_read_q_delta(bs);

            if (diffUVDelta)
            {
                params.DeltaQVDc = av1_read_q_delta(bs);
                params.DeltaQVAc = av1_read_q_delta(bs);
            }
            else
            {
                params.DeltaQVDc = params.DeltaQUDc;
                params.DeltaQVAc = params.DeltaQUAc;
            }
        }

        params.using_qmatrix = bs.GetBit();
        if (params.using_qmatrix) {
            params.qm_y = bs.GetBits(QM_LEVEL_BITS);
            params.qm_u = bs.GetBits(QM_LEVEL_BITS);

            if (!sh.color_config.separate_uv_delta_q)
                params.qm_v = params.qm_u;
            else
                params.qm_v = bs.GetBits(QM_LEVEL_BITS);
        }

        AV1D_LOG("[-]: %d", (uint32_t)bs.BitsDecoded());
    }

    inline
    bool av1_setup_segmentation(AV1Bitstream& bs, SegmentationParams& params, FrameHeader const& fh)
    {
        AV1D_LOG("[+]: %d", (uint32_t)bs.BitsDecoded());

        bool segmentQuantizerActive = false;
        params.segmentation_update_map = 0;
        params.segmentation_update_data = 0;

        params.segmentation_enabled = bs.GetBit();
        if (params.segmentation_enabled)
        {
            if (fh.primary_ref_frame == PRIMARY_REF_NONE)
            {
                params.segmentation_update_map = 1;
                params.segmentation_temporal_update = 0;
                params.segmentation_update_data = 1;
            }
            else
            {
                params.segmentation_update_map = bs.GetBit();
                if (params.segmentation_update_map)
                    params.segmentation_temporal_update = bs.GetBit();
                params.segmentation_update_data = bs.GetBit();
            }

            if (params.segmentation_update_data)
            {
                ClearAllSegFeatures(params);

                for (uint8_t i = 0; i < VP9_MAX_NUM_OF_SEGMENTS; ++i)
                {
                    for (uint8_t j = 0; j < SEG_LVL_MAX; ++j)
                    {
                        int32_t data = 0;
                        if (bs.GetBit()) // feature_enabled
                        {
                            if (j == SEG_LVL_ALT_Q)
                                segmentQuantizerActive = true;
                            EnableSegFeature(params, i, (SEG_LVL_FEATURES)j);

                            const uint32_t nBits = UMC_VP9_DECODER::GetUnsignedBits(SEG_FEATURE_DATA_MAX[j]);
                            const uint32_t isSigned = IsSegFeatureSigned((SEG_LVL_FEATURES)j);
                            const int32_t dataMax = SEG_FEATURE_DATA_MAX[j];
                            const int32_t dataMin = -dataMax;

                            if (isSigned)
                                data = read_inv_signed_literal(bs, nBits);
                            else
                                data = bs.GetBits(nBits);

                            data = mfx::clamp(data, dataMin, dataMax);
                        }

                        SetSegData(params, i, (SEG_LVL_FEATURES)j, data);
                    }
                }
            }
        }
        else
            ClearAllSegFeatures(params);

        AV1D_LOG("[-]: %d", (uint32_t)bs.BitsDecoded());
        return segmentQuantizerActive;
    }

    struct TileLimits
    {
        uint32_t maxTileWidthSB;
        uint32_t maxTileHeightSB;
        uint32_t maxTileAreaInSB;

        uint32_t maxlog2_tile_cols;
        uint32_t maxlog2_tile_rows;

        uint32_t minlog2_tile_cols;
        uint32_t minlog2_tile_rows;

        uint32_t minLog2Tiles;
    };

    inline uint32_t av1_tile_log2(uint32_t blockSize, uint32_t target)
    {
        uint32_t k;
        for (k = 0; (blockSize << k) < target; k++) {}
        return k;
    }

    inline void av1_get_tile_limits(FrameHeader const& fh, uint32_t sbSize, TileLimits& limits)
    {
        const uint32_t mibSizeLog2 = sbSize == BLOCK_64X64 ? 4 : 5;
        const uint32_t sbSizeLog2 = mibSizeLog2 + MI_SIZE_LOG2;

        limits.maxTileWidthSB = MAX_TILE_WIDTH >> sbSizeLog2;
        limits.maxTileAreaInSB = MAX_TILE_AREA >> (2 * sbSizeLog2);

        limits.minlog2_tile_cols = av1_tile_log2(limits.maxTileWidthSB,fh.sbCols);
        limits.maxlog2_tile_cols = av1_tile_log2(1, std::min(fh.sbCols, MAX_TILE_COLS));
        limits.maxlog2_tile_rows = av1_tile_log2(1, std::min(fh.sbRows, MAX_TILE_ROWS));
        limits.minLog2Tiles = av1_tile_log2(limits.maxTileAreaInSB, fh.sbCols * fh.sbRows);
        limits.minLog2Tiles = std::max(limits.minLog2Tiles, limits.minlog2_tile_cols);
    }

    static void av1_calculate_tile_cols(TileInfo& info, TileLimits& limits, FrameHeader const& fh)
    {
        uint32_t i;

        if (info.uniform_tile_spacing_flag)
        {
            uint32_t startSB;
            unsigned int sizeSB = AlignPowerOfTwo(fh.sbCols, info.TileColsLog2);
            sizeSB >>= info.TileColsLog2;
            VM_ASSERT(sizeSB > 0);
            for (i = 0, startSB = 0; startSB < fh.sbCols; i++)
            {
                info.SbColStarts[i] = startSB;
                startSB += sizeSB;
            }
            info.TileCols = i;
            info.SbColStarts[i] = fh.sbCols;
            limits.minlog2_tile_rows = std::max(static_cast<int32_t>(limits.minLog2Tiles - info.TileColsLog2), 0);
            limits.maxTileHeightSB = fh.sbRows >> limits.minlog2_tile_rows;
        }
        else
        {
            limits.maxTileAreaInSB = (fh.sbRows * fh.sbCols);
            uint32_t widestTileSB = 1;
            info.TileColsLog2 = av1_tile_log2(1, info.TileCols);
            for (i = 0; i < info.TileCols; i++)
            {
                uint32_t sizeSB = info.SbColStarts[i + 1] - info.SbColStarts[i];
                widestTileSB = std::max(widestTileSB, sizeSB);
            }
            if (limits.minLog2Tiles)
                limits.maxTileAreaInSB >>= (limits.minLog2Tiles + 1);
            limits.maxTileHeightSB = std::max(limits.maxTileAreaInSB / widestTileSB, 1u);
        }
    }

    static void av1_calculate_tile_rows(TileInfo& info, FrameHeader const& fh)
    {
        uint32_t startSB;
        uint32_t sizeSB;
        uint32_t i;

        if (info.uniform_tile_spacing_flag)
        {
            sizeSB = AlignPowerOfTwo(fh.sbRows, info.TileRowsLog2);
            sizeSB >>= info.TileRowsLog2;
            VM_ASSERT(sizeSB > 0);
            for (i = 0, startSB = 0; startSB < fh.sbRows; i++)
            {
                info.SbRowStarts[i] = startSB;
                startSB += sizeSB;
            }
            info.TileRows = i;
            info.SbRowStarts[i] = fh.sbRows;
        }
        else
            info.TileRowsLog2 = av1_tile_log2(1, info.TileRows);
    }

    static void av1_read_tile_info_max_tile(AV1Bitstream& bs, TileInfo& info, FrameHeader const& fh, uint32_t sbSize)
    {
        TileLimits limits = {};
        av1_get_tile_limits(fh, sbSize, limits);

        info.uniform_tile_spacing_flag = bs.GetBit();

        // Read tile columns
        if (info.uniform_tile_spacing_flag)
        {
            info.TileColsLog2 = limits.minlog2_tile_cols;
            while (info.TileColsLog2 < limits.maxlog2_tile_cols)
            {
                if (!bs.GetBit())
                    break;
                info.TileColsLog2++;
            }
        }
        else
        {
            uint32_t i;
            uint32_t startSB;
            uint32_t sbCols = fh.sbCols;
            for (i = 0, startSB = 0; sbCols > 0 && i < MAX_TILE_COLS; i++)
            {
                const uint32_t sizeInSB =
                    1 + read_uniform(bs, std::min(sbCols, limits.maxTileWidthSB));
                info.SbColStarts[i] = startSB;
                startSB += sizeInSB;
                sbCols -= sizeInSB;
            }
            info.TileCols = i;
            info.SbColStarts[i] = startSB + sbCols;
        }
        av1_calculate_tile_cols(info, limits, fh);

        if (!info.TileCols)
            throw av1_exception(UMC::UMC_ERR_INVALID_STREAM);

        // Read tile rows
        if (info.uniform_tile_spacing_flag)
        {
            info.TileRowsLog2 = limits.minlog2_tile_rows;
            while (info.TileRowsLog2 < limits.maxlog2_tile_rows)
            {
                if (!bs.GetBit())
                    break;
                info.TileRowsLog2++;
            }
        }
        else
        {
            uint32_t i;
            uint32_t startSB;
            uint32_t sbRows = fh.sbRows;
            for (i = 0, startSB = 0; sbRows > 0 && i < MAX_TILE_ROWS; i++)
            {
                const uint32_t sizeSB =
                    1 + read_uniform(bs, std::min(sbRows, limits.maxTileHeightSB));
                info.SbRowStarts[i] = startSB;
                startSB += sizeSB;
                sbRows -= sizeSB;
            }
            info.TileRows = i;
            info.SbRowStarts[i] = startSB + sbRows;
        }
        av1_calculate_tile_rows(info, fh);
    }

    inline
    void av1_read_tile_info(AV1Bitstream& bs, TileInfo& info, FrameHeader const& fh, uint32_t sbSize)
    {
        AV1D_LOG("[+]: %d", (uint32_t)bs.BitsDecoded());
        const bool large_scale_tile = 0; // this parameter isn't taken from the bitstream. Looks like decoder gets it from outside (e.g. container or some environment).
        if (large_scale_tile)
        {
            // TODO: [Rev0.85] add support of large scale tile
            AV1D_LOG("[-]: %d", (uint32_t)bs.BitsDecoded());
            return;
        }
        av1_read_tile_info_max_tile(bs, info, fh, sbSize);

        if (info.TileColsLog2 || info.TileRowsLog2)
            info.context_update_tile_id = bs.GetBits(info.TileColsLog2 + info.TileRowsLog2);

        if (NumTiles(info) > 1)
            info.TileSizeBytes = bs.GetBits(2) + 1;

        AV1D_LOG("[-]: %d", (uint32_t)bs.BitsDecoded());
    }

    const GlobalMotionParams default_warp_params = {
        IDENTITY,
        { 0, 0, (1 << WARPEDMODEL_PREC_BITS), 0, 0, (1 << WARPEDMODEL_PREC_BITS), 0,
        0 },
        0, 0, 0, 0, 0
    };

    inline
    bool av1_read_global_motion_params(AV1Bitstream& bs, GlobalMotionParams& params,
                                       GlobalMotionParams const& ref_params, FrameHeader const& fh)
    {
        AV1D_LOG("[+]: %d", (uint32_t)bs.BitsDecoded());

        const uint8_t SUBEXPFIN_K = 3;
        const uint8_t GM_TRANS_PREC_BITS = 6;
        const uint8_t GM_ABS_TRANS_BITS = 12;
        const uint8_t GM_ABS_TRANS_ONLY_BITS = (GM_ABS_TRANS_BITS - GM_TRANS_PREC_BITS + 3);
        const uint8_t GM_TRANS_PREC_DIFF = WARPEDMODEL_PREC_BITS - GM_TRANS_PREC_BITS;
        const uint8_t GM_TRANS_ONLY_PREC_DIFF = WARPEDMODEL_PREC_BITS - 3;
        const uint16_t GM_TRANS_DECODE_FACTOR = 1 << GM_TRANS_PREC_DIFF;
        const uint16_t GM_TRANS_ONLY_DECODE_FACTOR = 1 << GM_TRANS_ONLY_PREC_DIFF;

        const uint8_t GM_ALPHA_PREC_BITS = 15;
        const uint8_t GM_ABS_ALPHA_BITS = 12;
        const int8_t GM_ALPHA_PREC_DIFF = WARPEDMODEL_PREC_BITS - GM_ALPHA_PREC_BITS;
        const uint8_t GM_ALPHA_DECODE_FACTOR = 1 << GM_ALPHA_PREC_DIFF;

        const uint16_t GM_ALPHA_MAX = 1 << GM_ABS_ALPHA_BITS;

        TRANSFORMATION_TYPE type;
        type = static_cast<TRANSFORMATION_TYPE>(bs.GetBit());
        if (type != IDENTITY) {
            if (bs.GetBit())
                type = ROTZOOM;
            else
                type = bs.GetBit() ? TRANSLATION : AFFINE;
        }

        params = default_warp_params;
        params.wmtype = type;

        if (type >= ROTZOOM)
        {
            params.wmmat[2] = read_signed_primitive_refsubexpfin(
                bs, GM_ALPHA_MAX + 1, SUBEXPFIN_K,
                static_cast<int16_t>(ref_params.wmmat[2] >> GM_ALPHA_PREC_DIFF) -
                (1 << GM_ALPHA_PREC_BITS)) *
                GM_ALPHA_DECODE_FACTOR +
                (1 << WARPEDMODEL_PREC_BITS);
            params.wmmat[3] = read_signed_primitive_refsubexpfin(
                bs, GM_ALPHA_MAX + 1, SUBEXPFIN_K,
                static_cast<int16_t>(ref_params.wmmat[3] >> GM_ALPHA_PREC_DIFF)) *
                GM_ALPHA_DECODE_FACTOR;
        }

        if (type >= AFFINE)
        {
            params.wmmat[4] = read_signed_primitive_refsubexpfin(
                bs, GM_ALPHA_MAX + 1, SUBEXPFIN_K,
                static_cast<int16_t>(ref_params.wmmat[4] >> GM_ALPHA_PREC_DIFF)) *
                GM_ALPHA_DECODE_FACTOR;
            params.wmmat[5] = read_signed_primitive_refsubexpfin(
                bs, GM_ALPHA_MAX + 1, SUBEXPFIN_K,
                static_cast<int16_t>(ref_params.wmmat[5] >> GM_ALPHA_PREC_DIFF) -
                (1 << GM_ALPHA_PREC_BITS)) *
                GM_ALPHA_DECODE_FACTOR +
                (1 << WARPEDMODEL_PREC_BITS);
        }
        else
        {
            params.wmmat[4] = -params.wmmat[3];
            params.wmmat[5] = params.wmmat[2];
        }

        if (type >= TRANSLATION)
        {
            const uint8_t disallowHP = fh.allow_high_precision_mv ? 0 : 1;
            const int32_t trans_bits = (type == TRANSLATION)
                ? GM_ABS_TRANS_ONLY_BITS - disallowHP
                : GM_ABS_TRANS_BITS;
            const int32_t trans_dec_factor =
                (type == TRANSLATION) ? GM_TRANS_ONLY_DECODE_FACTOR * (1 << disallowHP)
                : GM_TRANS_DECODE_FACTOR;
            const int32_t trans_prec_diff = (type == TRANSLATION)
                ? GM_TRANS_ONLY_PREC_DIFF + disallowHP
                : GM_TRANS_PREC_DIFF;
            params.wmmat[0] = read_signed_primitive_refsubexpfin(
                bs, (1 << trans_bits) + 1, SUBEXPFIN_K,
                static_cast<int16_t>(ref_params.wmmat[0] >> trans_prec_diff)) *
                trans_dec_factor;
            params.wmmat[1] = read_signed_primitive_refsubexpfin(
                bs, (1 << trans_bits) + 1, SUBEXPFIN_K,
                static_cast<int16_t>(ref_params.wmmat[1] >> trans_prec_diff)) *
                trans_dec_factor;
        }

        if (params.wmtype <= AFFINE)
        {
            const bool goodParams = av1_get_shear_params(params);
            if (!goodParams)
                return false;
        }

        AV1D_LOG("[-]: %d", (uint32_t)bs.BitsDecoded());

        return true;
    }

    inline void av1_read_global_motion(AV1Bitstream& bs, FrameHeader& fh, DPBType const& frameDpb)
    {
        AV1D_LOG("[+]: %d", (uint32_t)bs.BitsDecoded());

        for (int i = 0; i < TOTAL_REFS; i++)
        {
            fh.global_motion_params[i] = default_warp_params;
        }

        if (FrameIsIntra(fh))
        {
            AV1D_LOG("[-]: %d", (uint32_t)bs.BitsDecoded());
            return;
        }

        int frame;
        for (frame = LAST_FRAME; frame <= ALTREF_FRAME; ++frame)
        {
            FrameHeader const* refFH = nullptr;
            if (fh.primary_ref_frame != PRIMARY_REF_NONE)
                refFH = &frameDpb[fh.ref_frame_idx[fh.primary_ref_frame]]->GetFrameHeader();

            GlobalMotionParams const& ref_params = fh.error_resilient_mode || !refFH ?
                default_warp_params : refFH->global_motion_params[frame];

            GlobalMotionParams& params = fh.global_motion_params[frame];
            const bool goodParams = av1_read_global_motion_params(bs, params, ref_params, fh);
            if (!goodParams)
                params.invalid = 1;
        }

        AV1D_LOG("[-]: %d", (uint32_t)bs.BitsDecoded());
    }

    static void av1_read_film_grain_params(AV1Bitstream& bs, FilmGrainParams& params, SequenceHeader const& sh, FrameHeader const& fh, DPBType const& frameDpb)
    {
        AV1D_LOG("[+]: %d", (uint32_t)bs.BitsDecoded());

        params.apply_grain = bs.GetBit();
        if (!params.apply_grain)
        {
            params = FilmGrainParams{};
            AV1D_LOG("[-]: %d", (uint32_t)bs.BitsDecoded());
            return;
        }

        params.grain_seed = bs.GetBits(16);
        if (fh.frame_type == INTER_FRAME)
            params.update_grain = bs.GetBit();
        else
            params.update_grain = 1;

        if (!params.update_grain)
        {
            // inherit parameters from a previous reference frame
            int film_grain_params_ref_idx = bs.GetBits(3);
            FrameHeader const* refFH = nullptr;
            refFH = &frameDpb[film_grain_params_ref_idx]->GetFrameHeader();
            uint32_t random_seed = params.grain_seed;
            params = refFH->film_grain_params;
            params.grain_seed = random_seed;
            AV1D_LOG("[-]: %d", (uint32_t)bs.BitsDecoded());
            return;
        }

        // Scaling functions parameters
        params.num_y_points = bs.GetBits(4);  // max 14
        if (params.num_y_points > MAX_POINTS_IN_SCALING_FUNCTION_LUMA)
            throw av1_exception(UMC::UMC_ERR_INVALID_STREAM);

        for (int i = 0; i < params.num_y_points; i++)
        {
            params.point_y_value[i] = bs.GetBits(8);
            if (i && params.point_y_value[i - 1] >= params.point_y_value[i])
                throw av1_exception(UMC::UMC_ERR_INVALID_STREAM);
            params.point_y_scaling[i] = bs.GetBits(8);
        }

        if (!sh.color_config.mono_chrome)
            params.chroma_scaling_from_luma = bs.GetBit();

        if (sh.color_config.mono_chrome || params.chroma_scaling_from_luma ||
            (sh.color_config.subsampling_x == 1 &&
             sh.color_config.subsampling_y == 1 &&
                params.num_y_points == 0))
        {
            params.num_cb_points = 0;
            params.num_cr_points = 0;
        }
        else
        {
            params.num_cb_points = bs.GetBits(4);  // max 10
            if (params.num_cb_points > MAX_POINTS_IN_SCALING_FUNCTION_CHROMA)
                throw av1_exception(UMC::UMC_ERR_INVALID_STREAM);

            for (int i = 0; i < params.num_cb_points; i++)
            {
                params.point_cb_value[i] = bs.GetBits(8);
                if (i &&
                    params.point_cb_value[i - 1] >= params.point_cb_value[i])
                    throw av1_exception(UMC::UMC_ERR_INVALID_STREAM);
                params.point_cb_scaling[i] = bs.GetBits(8);
            }

            params.num_cr_points = bs.GetBits(4);  // max 10
            if (params.num_cr_points > MAX_POINTS_IN_SCALING_FUNCTION_CHROMA)
                throw av1_exception(UMC::UMC_ERR_INVALID_STREAM);

            for (int i = 0; i < params.num_cr_points; i++)
            {
                params.point_cr_value[i] = bs.GetBits(8);
                if (i &&
                    params.point_cr_value[i - 1] >= params.point_cr_value[i])
                        throw av1_exception(UMC::UMC_ERR_INVALID_STREAM);
                params.point_cr_scaling[i] = bs.GetBits(8);
            }
        }

        params.grain_scaling = bs.GetBits(2) + 8;  // 8 + value

        // AR coefficients
        // Only sent if the corresponsing scaling function has
        // more than 0 points

        params.ar_coeff_lag = bs.GetBits(2);

        int num_pos_luma = 2 * params.ar_coeff_lag * (params.ar_coeff_lag + 1);
        int num_pos_chroma = num_pos_luma;
        if (params.num_y_points > 0) ++num_pos_chroma;

        if (params.num_y_points)
            for (int i = 0; i < num_pos_luma; i++)
                params.ar_coeffs_y[i] = bs.GetBits(8) - 128;

        if (params.num_cb_points || params.chroma_scaling_from_luma)
            for (int i = 0; i < num_pos_chroma; i++)
                params.ar_coeffs_cb[i] = bs.GetBits(8) - 128;

        if (params.num_cr_points || params.chroma_scaling_from_luma)
            for (int i = 0; i < num_pos_chroma; i++)
                params.ar_coeffs_cr[i] = bs.GetBits(8) - 128;

        params.ar_coeff_shift = bs.GetBits(2) + 6;  // 6 + value

        params.grain_scale_shift = bs.GetBits(2);

        if (params.num_cb_points) {
            params.cb_mult = bs.GetBits(8);
            params.cb_luma_mult = bs.GetBits(8);
            params.cb_offset = bs.GetBits(9);
        }

        if (params.num_cr_points) {
            params.cr_mult = bs.GetBits(8);
            params.cr_luma_mult = bs.GetBits(8);
            params.cr_offset = bs.GetBits(9);
        }

        params.overlap_flag = bs.GetBit();

        params.clip_to_restricted_range = bs.GetBit();

        AV1D_LOG("[-]: %d", (uint32_t)bs.BitsDecoded());
    }

    static void av1_read_film_grain(AV1Bitstream& bs, SequenceHeader const& sh, FrameHeader& fh, DPBType const& frameDpb)
    {
        AV1D_LOG("[+]: %d", (uint32_t)bs.BitsDecoded());

        if (fh.show_frame || fh.showable_frame)
        {
            if (sh.film_grain_param_present)
                av1_read_film_grain_params(bs, fh.film_grain_params, sh, fh, frameDpb);
            else
                fh.film_grain_params = FilmGrainParams{};

            fh.film_grain_params.BitDepth = sh.color_config.BitDepth;
        }

        AV1D_LOG("[-]: %d", (uint32_t)bs.BitsDecoded());
    }

    inline
    void av1_read_obu_size(AV1Bitstream& bs, size_t& size, size_t& length)
    {
        size_t start = bs.BytesDecoded();
        size = read_leb128(bs);
        length = bs.BytesDecoded() - start;
    }

    OBUHeader av1_read_obu_header(AV1Bitstream& bs)
    {
        bs.GetBit(); // first bit is obu_forbidden_bit (0) - hello and thanks to Dima G. :)

        OBUHeader header = {};

        header.obu_type = (AV1_OBU_TYPE)bs.GetBits(4);
        const int obu_extension_flag = bs.GetBit();
        header.obu_has_size_field = bs.GetBit();
        bs.GetBit();  // reserved

        if (obu_extension_flag) // if has extension
        {
            header.temporal_id = bs.GetBits(3);
            header.spatial_id = bs.GetBits(2);
            bs.GetBits(3);  // reserved
        }

        return header;
    }

    void AV1Bitstream::ReadOBUInfo(OBUInfo& info)
    {
        size_t sizeFieldLength = 0;
        size_t obu_size = 0;

        const size_t start = BytesDecoded();
        info.header = av1_read_obu_header(*this);
        size_t headerSize = BytesDecoded() - start;

        if (info.header.obu_has_size_field)
            av1_read_obu_size(*this, obu_size, sizeFieldLength);
        else if (info.header.obu_type != OBU_TEMPORAL_DELIMITER)
            throw av1_exception(UMC::UMC_ERR_NOT_IMPLEMENTED); // no support for OBUs w/o size field so far

        info.size = headerSize + sizeFieldLength + obu_size;
    }

    void AV1Bitstream::ReadByteAlignment()
    {
        if (m_bitOffset)
        {
            const uint32_t bitsToRead = 8 - m_bitOffset;
            const uint32_t bits = GetBits(bitsToRead);
            if (bits)
                throw av1_exception(UMC::UMC_ERR_INVALID_STREAM);
        }
    }

    void AV1Bitstream::ReadTileGroupHeader(FrameHeader const& fh, TileGroupInfo& info)
    {
        uint8_t tile_start_and_end_present_flag = 0;
        if (!fh.large_scale_tile && NumTiles(fh.tile_info) > 1)
            tile_start_and_end_present_flag = GetBit();

        if (tile_start_and_end_present_flag)
        {
            const uint32_t log2Tiles = fh.tile_info.TileColsLog2 + fh.tile_info.TileRowsLog2;
            info.startTileIdx = GetBits(log2Tiles);
            info.endTileIdx = GetBits(log2Tiles);
            info.numTiles = info.endTileIdx - info.startTileIdx + 1;
        }
        else
        {
            info.numTiles = NumTiles(fh.tile_info);
            info.startTileIdx = 0;
            info.endTileIdx = info.numTiles - 1;
        }

        ReadByteAlignment();
    }

    uint64_t AV1Bitstream::GetLE(uint32_t n)
    {
        VM_ASSERT(m_bitOffset == 0);
        VM_ASSERT(n <= 8);

        uint64_t t = 0;
        for (uint32_t i = 0; i < n; i++)
            t += (*m_pbs++) << (i * 8);

        return t;
    }

    void AV1Bitstream::ReadTile(FrameHeader const& fh, size_t& reportedSize, size_t& actualSize)
    {
        const size_t tile_size_minus_1 = static_cast<size_t>(GetLE(fh.tile_info.TileSizeBytes));
        actualSize = reportedSize = tile_size_minus_1 + 1;

        if (BytesLeft() < reportedSize)
            actualSize = BytesLeft();

        m_pbs += actualSize;
    }

    void AV1Bitstream::ReadSequenceHeader(SequenceHeader& sh)
    {
        AV1D_LOG("[+]: %d", (uint32_t)BitsDecoded());

        sh.seq_profile = GetBits(3);
        sh.still_picture = GetBit();
        sh.reduced_still_picture_header = GetBit();

        if (sh.reduced_still_picture_header)
            GetBits(5); // seq_level_idx[0]
        else
        {
            sh.timing_info_present_flag = GetBit();
            if (sh.timing_info_present_flag)
            {
                av1_timing_info(*this, sh.timing_info);
                sh.decoder_model_info_present_flag = GetBit();
                if (sh.decoder_model_info_present_flag)
                    av1_decoder_model_info(*this, sh.decoder_model_info);
            }

            const int initial_display_delay_present_flag = GetBit();
            sh.operating_points_cnt_minus_1 = GetBits(5);
            for (unsigned int i = 0; i <= sh.operating_points_cnt_minus_1; i++)
            {
                sh.operating_point_idc[i] = GetBits(12);
                sh.seq_level_idx[i] = GetBits(5);
                if (sh.seq_level_idx[i] > 7)
                    sh.seq_tier[i] = GetBit();

                if (sh.decoder_model_info_present_flag)
                {
                    sh.decoder_model_present_for_this_op[i] = GetBit();
                    if (sh.decoder_model_present_for_this_op[i])
                        av1_operating_parameters_info(*this,
                            sh.operating_parameters_info[i],
                            sh.decoder_model_info.buffer_delay_length_minus_1 + 1);
                }

                const int BufferPoolMaxSize = 10; // av1 spec E.2
                sh.initial_display_delay_minus_1[i] = BufferPoolMaxSize - 1;
                if (initial_display_delay_present_flag)
                {
                    if (GetBit()) // initial_display_delay_present_for_this_op[i]
                        sh.initial_display_delay_minus_1[i] = GetBits(4);
                }
            }

        }

        sh.frame_width_bits = GetBits(4) + 1;
        sh.frame_height_bits = GetBits(4) + 1;

        sh.max_frame_width = GetBits(sh.frame_width_bits) + 1;
        sh.max_frame_height = GetBits(sh.frame_height_bits) + 1;

        if (sh.reduced_still_picture_header)
        {
            sh.frame_id_numbers_present_flag = 0;
        }
        else
        {
            sh.frame_id_numbers_present_flag = GetBit();
        }

        if (sh.frame_id_numbers_present_flag) {
            sh.delta_frame_id_length = GetBits(4) + 2;
            sh.idLen = GetBits(3) + sh.delta_frame_id_length + 1;
        }

        sh.sbSize = GetBit() ? BLOCK_128X128 : BLOCK_64X64;

        sh.enable_filter_intra = GetBit();
        sh.enable_intra_edge_filter = GetBit();

        if (sh.reduced_still_picture_header)
        {
            sh.seq_force_screen_content_tools = SELECT_SCREEN_CONTENT_TOOLS;
            sh.seq_force_integer_mv = SELECT_INTEGER_MV;
            sh.order_hint_bits_minus1 = -1;
        }
        else
        {
            sh.enable_interintra_compound = GetBit();
            sh.enable_masked_compound = GetBit();
            sh.enable_warped_motion = GetBit();
            sh.enable_dual_filter = GetBit();
            sh.enable_order_hint = GetBit();
            if (sh.enable_order_hint)
            {
                sh.enable_jnt_comp = GetBit();
                sh.enable_ref_frame_mvs = GetBit();
            }

            if (GetBit()) // seq_choose_screen_content_tools
                sh.seq_force_screen_content_tools = SELECT_SCREEN_CONTENT_TOOLS;
            else
                sh.seq_force_screen_content_tools = GetBit();

            if (sh.seq_force_screen_content_tools > 0)
            {
                if (GetBit()) // seq_choose_integer_mv
                    sh.seq_force_integer_mv = 2;
                else
                    sh.seq_force_integer_mv = GetBit();
            }
            else
                sh.seq_force_integer_mv = 2;

            sh.order_hint_bits_minus1 =
                sh.enable_order_hint ? GetBits(3) : -1;
        }

        sh.enable_superres = GetBit();
        sh.enable_cdef = GetBit();
        sh.enable_restoration = GetBit();

        av1_color_config(*this, sh.color_config, sh.seq_profile);

        sh.film_grain_param_present = GetBit();

        AV1D_LOG("[-]: %d", (uint32_t)BitsDecoded());
    }

    struct RefFrameInfo {
        uint32_t mapIdx;
        uint32_t shiftedOrderHint;
    };

    inline int32_t av1_compare_ref_frame_info(void const* left, void const* right)
    {
        VM_ASSERT(left);
        VM_ASSERT(right);

        RefFrameInfo const* a = (RefFrameInfo*)left;
        RefFrameInfo const* b = (RefFrameInfo*)right;

        if (a->shiftedOrderHint < b->shiftedOrderHint)
            return -1;

        if (a->shiftedOrderHint > b->shiftedOrderHint)
            return 1;

        return (a->mapIdx < b->mapIdx)
            ? -1
            : ((a->mapIdx > b->mapIdx) ? 1 : 0);
    }

    static void av1_set_frame_refs(SequenceHeader const& sh, FrameHeader& fh, DPBType const& frameDpb, uint32_t last_frame_idx, uint32_t gold_frame_idx)
    {
        const uint32_t curFrameHint = 1 << sh.order_hint_bits_minus1;

        RefFrameInfo refFrameInfo[NUM_REF_FRAMES]; // RefFrameInfo structure contains
                                                   // (1) shiftedOrderHint
                                                   // (2) index in DPB (allows to correct sorting of frames having equal shiftedOrderHint)
        uint32_t refFlagList[INTER_REFS] = {};

        for (int i = 0; i < NUM_REF_FRAMES; ++i)
        {
            const uint32_t mapIdx = i;

            refFrameInfo[i].mapIdx = mapIdx;

            FrameHeader const& refHdr = frameDpb[i]->GetFrameHeader();
            refFrameInfo[i].shiftedOrderHint =
                curFrameHint + av1_get_relative_dist(sh, refHdr.order_hint, fh.order_hint);
        }

        const uint32_t lastOrderHint = refFrameInfo[last_frame_idx].shiftedOrderHint;
        const uint32_t goldOrderHint = refFrameInfo[gold_frame_idx].shiftedOrderHint;

        // Confirm both LAST_FRAME and GOLDEN_FRAME are valid forward reference frames
        if (lastOrderHint >= curFrameHint)
        {
            VM_ASSERT("lastOrderHint is not less than curFrameHint!");
            throw av1_exception(UMC::UMC_ERR_INVALID_STREAM);
        }

        if (goldOrderHint >= curFrameHint)
        {
            VM_ASSERT("goldOrderHint is not less than curFrameHint!");
            throw av1_exception(UMC::UMC_ERR_INVALID_STREAM);
        }

        // Sort ref frames based on their shiftedOrderHint values.
        qsort(refFrameInfo, NUM_REF_FRAMES, sizeof(RefFrameInfo),
            av1_compare_ref_frame_info);

        // Identify forward and backward reference frames.
        // Forward  reference: ref order_hint < fh.order_hint
        // Backward reference: ref order_hint >= fh.order_hint
        int fwdStartIdx = 0;
        int fwdEndIdx = NUM_REF_FRAMES - 1;

        for (uint32_t i = 0; i < NUM_REF_FRAMES; i++)
        {
            if (refFrameInfo[i].shiftedOrderHint >= curFrameHint)
            {
                fwdEndIdx = i - 1;
                break;
            }
        }

        int bwdStartIdx = fwdEndIdx + 1;
        int bwdEndIdx = NUM_REF_FRAMES - 1;

        // === Backward Reference Frames ===

        // == ALTREF_FRAME ==
        if (bwdStartIdx <= bwdEndIdx)
        {
            fh.ref_frame_idx[ALTREF_FRAME - LAST_FRAME] = refFrameInfo[bwdEndIdx].mapIdx;
            refFlagList[ALTREF_FRAME - LAST_FRAME] = 1;
            bwdEndIdx--;
        }

        // == BWDREF_FRAME ==
        if (bwdStartIdx <= bwdEndIdx)
        {
            fh.ref_frame_idx[BWDREF_FRAME - LAST_FRAME] = refFrameInfo[bwdStartIdx].mapIdx;
            refFlagList[BWDREF_FRAME - LAST_FRAME] = 1;
            bwdStartIdx++;
        }

        // == ALTREF2_FRAME ==
        if (bwdStartIdx <= bwdEndIdx)
        {
            fh.ref_frame_idx[ALTREF2_FRAME - LAST_FRAME] = refFrameInfo[bwdStartIdx].mapIdx;
            refFlagList[ALTREF2_FRAME - LAST_FRAME] = 1;
        }

        // === Forward Reference Frames ===

        for (int i = fwdStartIdx; i <= fwdEndIdx; ++i)
        {
            // == LAST_FRAME ==
            if (refFrameInfo[i].mapIdx == last_frame_idx)
            {
                fh.ref_frame_idx[LAST_FRAME - LAST_FRAME] = refFrameInfo[i].mapIdx;
                refFlagList[LAST_FRAME - LAST_FRAME] = 1;
            }

            // == GOLDEN_FRAME ==
            if (refFrameInfo[i].mapIdx == gold_frame_idx)
            {
                fh.ref_frame_idx[GOLDEN_FRAME - LAST_FRAME] = refFrameInfo[i].mapIdx;
                refFlagList[GOLDEN_FRAME - LAST_FRAME] = 1;
            }
        }

        VM_ASSERT(refFlagList[LAST_FRAME - LAST_FRAME] == 1 &&
            refFlagList[GOLDEN_FRAME - LAST_FRAME] == 1);

        // == LAST2_FRAME ==
        // == LAST3_FRAME ==
        // == BWDREF_FRAME ==
        // == ALTREF2_FRAME ==
        // == ALTREF_FRAME ==

        // Set up the reference frames in the anti-chronological order.
        static const MV_REFERENCE_FRAME ref_frame_list[INTER_REFS - 2] =
        { LAST2_FRAME, LAST3_FRAME, BWDREF_FRAME, ALTREF2_FRAME, ALTREF_FRAME };

        uint32_t refIdx;
        for (refIdx = 0; refIdx < (INTER_REFS - 2); refIdx++)
        {
            const MV_REFERENCE_FRAME refFrame = ref_frame_list[refIdx];

            if (refFlagList[refFrame - LAST_FRAME] == 1) continue;

            while (fwdStartIdx <= fwdEndIdx &&
                (refFrameInfo[fwdEndIdx].mapIdx == last_frame_idx ||
                    refFrameInfo[fwdEndIdx].mapIdx == gold_frame_idx))
            {
                fwdEndIdx--;
            }
            if (fwdStartIdx > fwdEndIdx) break;

            fh.ref_frame_idx[refFrame - LAST_FRAME] = refFrameInfo[fwdEndIdx].mapIdx;
            refFlagList[refFrame - LAST_FRAME] = 1;

            fwdEndIdx--;
        }

        // Assign all the remaining frame(s), if any, to the earliest reference frame.
        for (; refIdx < (INTER_REFS - 2); refIdx++)
        {
            const MV_REFERENCE_FRAME refFrame = ref_frame_list[refIdx];
			if (refFlagList[refFrame - LAST_FRAME] == 1) continue;
            fh.ref_frame_idx[refFrame - LAST_FRAME] = refFrameInfo[fwdStartIdx].mapIdx;
            refFlagList[refFrame - LAST_FRAME] = 1;
        }

        for (int i = 0; i < INTER_REFS; i++)
            VM_ASSERT(refFlagList[i] == 1);
    }

    static void av1_mark_ref_frames(SequenceHeader const& sh, FrameHeader& fh, DPBType const& frameDpb)
    {
        const uint32_t idLen = sh.idLen;
        const uint32_t diffLen = sh.delta_frame_id_length;
        for (int32_t i = 0; i < NUM_REF_FRAMES; i++)
        {
            FrameHeader const& refHdr = frameDpb[i]->GetFrameHeader();
            if ((static_cast<int32_t>(fh.current_frame_id) - (1 << diffLen)) > 0)
            {
                if (refHdr.current_frame_id > fh.current_frame_id ||
                    refHdr.current_frame_id < fh.current_frame_id - (1 << diffLen))
                    frameDpb[i]->SetRefValid(false);
            }
            else
            {
                if (refHdr.current_frame_id > fh.current_frame_id &&
                    refHdr.current_frame_id < ((1 << idLen) + fh.current_frame_id - (1 << diffLen)))
                    frameDpb[i]->SetRefValid(false);
            }
        }
    }

    inline void av1_read_timing_point_info(AV1Bitstream& bs, SequenceHeader const& sh, FrameHeader& fh)
    {
        AV1D_LOG("[+]: %d", (uint32_t)bs.BitsDecoded());

        if (sh.decoder_model_info_present_flag && !sh.timing_info.equal_picture_interval)
        {
            const int n = sh.decoder_model_info.frame_presentation_time_length_minus_1 + 1;
            fh.frame_presentation_time = bs.GetBits(n);
        }

        AV1D_LOG("[-]: %d", (uint32_t)bs.BitsDecoded());
    }

    inline void av1_read_interpolation_filter(AV1Bitstream& bs, FrameHeader& fh)
    {
        AV1D_LOG("[+]: %d", (uint32_t)bs.BitsDecoded());

        if (bs.GetBit()) // is_filter_switchable
            fh.interpolation_filter = SWITCHABLE;
        else
            fh.interpolation_filter = static_cast<INTERP_FILTER>(bs.GetBits(LOG2_SWITCHABLE_FILTERS));

        AV1D_LOG("[-]: %d", (uint32_t)bs.BitsDecoded());
    }

    inline void av1_read_delta_q_params(AV1Bitstream& bs, FrameHeader& fh)
    {
        AV1D_LOG("[+]: %d", (uint32_t)bs.BitsDecoded());

        // here AV1 spec 1.0 contradicts to reference implementation:
        // spec sets fh.delta_q_res to 0 and reference sets it to 1
        // current code mimics behavior of reference implementation
        fh.delta_q_res = 1;
        if (fh.quantization_params.base_q_idx > 0)
            fh.delta_q_present = bs.GetBit();

        if (fh.delta_q_present)
            fh.delta_q_res = 1 << bs.GetBits(2);

        AV1D_LOG("[-]: %d", (uint32_t)bs.BitsDecoded());
    }

    inline void av1_read_delta_lf_params(AV1Bitstream& bs, FrameHeader& fh)
    {
        AV1D_LOG("[+]: %d", (uint32_t)bs.BitsDecoded());

        // here AV1 spec 1.0 contradicts to reference implementation:
        // spec sets fh.delta_lf_res to 0 and reference sets it to 1
        // current code mimics behavior of reference implementation
        fh.delta_lf_res = 1;
        if (fh.delta_q_present)
        {
            if (!fh.allow_intrabc)
                fh.delta_lf_present = bs.GetBit();

            if (fh.delta_lf_present)
            {
                fh.delta_lf_res = 1 << bs.GetBits(2);
                fh.delta_lf_multi = bs.GetBit();
            }
        }

        AV1D_LOG("[-]: %d", (uint32_t)bs.BitsDecoded());
    }

    void AV1Bitstream::ReadUncompressedHeader(FrameHeader& fh, SequenceHeader const& sh, DPBType const& frameDpb, OBUHeader const& obuHeader, uint32_t& PreFrame_id)
    {
        using UMC_VP9_DECODER::REF_FRAMES_LOG2;

        AV1D_LOG("[+]: %d", (uint32_t)BitsDecoded());

        if (sh.reduced_still_picture_header)
        {
            fh.frame_type = KEY_FRAME;
            fh.show_frame = 1;
        }
        else
        {
            fh.show_existing_frame = GetBit();
            if (fh.show_existing_frame)
            {
                fh.frame_to_show_map_idx = GetBits(3);
                av1_read_timing_point_info(*this, sh, fh);
                FrameHeader & refHdr = frameDpb[fh.frame_to_show_map_idx]->GetFrameHeader();

                if (sh.frame_id_numbers_present_flag)
                {
                    fh.display_frame_id = GetBits(sh.idLen);

                    // check that there is no confilct between display_frame_id and respective frame in DPB
                    if ((fh.display_frame_id != refHdr.current_frame_id ||
                        false == frameDpb[fh.frame_to_show_map_idx]->RefValid()))
                    {
                        VM_ASSERT("Frame_to_show is absent in DPB or invalid!");
                    }

                    fh.current_frame_id = fh.display_frame_id;
                }

                if (refHdr.frame_type == KEY_FRAME)
                {
                    refHdr.refresh_frame_flags = 255;
                }

                // get frame resolution
                fh.FrameWidth = refHdr.FrameWidth;
                fh.FrameHeight = refHdr.FrameHeight;

                fh.show_frame = 1;

                AV1D_LOG("[-]: %d", (uint32_t)BitsDecoded());

                return;
            }

            fh.frame_type = static_cast<FRAME_TYPE>(GetBits(2));
            fh.show_frame = GetBit();
            if (fh.show_frame)
            {
                av1_read_timing_point_info(*this, sh, fh);
                fh.showable_frame = (fh.frame_type != KEY_FRAME);
            }
            else
                fh.showable_frame = GetBit();

            if (fh.frame_type == SWITCH_FRAME ||
                (fh.frame_type == KEY_FRAME && fh.show_frame))
                fh.error_resilient_mode = 1;
            else
                fh.error_resilient_mode = GetBit();
        }

        if (fh.frame_type == KEY_FRAME && !frameDpb.empty())
        {
            for (int i = 0; i < NUM_REF_FRAMES; i++)
                frameDpb[i]->SetRefValid(false);
        }

        if (fh.frame_type != KEY_FRAME)
        {
            VM_ASSERT(frameDpb.size() == NUM_REF_FRAMES);
        }

        fh.disable_cdf_update = GetBit();
        if (sh.seq_force_screen_content_tools == 2)
            fh.allow_screen_content_tools = GetBit();
        else
            fh.allow_screen_content_tools = sh.seq_force_screen_content_tools;

        if (fh.allow_screen_content_tools)
        {
            if (sh.seq_force_integer_mv == 2)
                fh.force_integer_mv = GetBit();
            else
                fh.force_integer_mv = sh.seq_force_integer_mv;
        }
        else
            fh.force_integer_mv = 0;

        if (FrameIsIntra(fh))
            fh.force_integer_mv = 1;

        if (sh.frame_id_numbers_present_flag)
        {
            fh.current_frame_id = GetBits(sh.idLen);

            if (fh.frame_type != KEY_FRAME)
            {
                // check current_frame_id as described in AV1 spec 6.8.2
                const uint32_t idLen = sh.idLen;
                const int32_t diffFrameId = fh.current_frame_id > PreFrame_id ?
                    fh.current_frame_id - PreFrame_id :
                    (1 << idLen) + fh.current_frame_id - PreFrame_id;

                if (fh.current_frame_id == PreFrame_id ||
                    diffFrameId >= (1 << (idLen - 1)))
                {
                    VM_ASSERT("current_frame_id is incompliant to AV1 spec!");
                    throw av1_exception(UMC::UMC_ERR_INVALID_STREAM);
                }

                //  check and mark ref frames as not valid as described in "Reference frame marking process" AV1 spec 5.9.4
                av1_mark_ref_frames(sh, fh, frameDpb);
            }
            PreFrame_id = fh.current_frame_id;
        }

        if (fh.frame_type == SWITCH_FRAME)
            fh.frame_size_override_flag = 1;
        else if (sh.reduced_still_picture_header)
            fh.frame_size_override_flag = 0;
        else
            fh.frame_size_override_flag = GetBits(1);

        fh.order_hint = GetBits(sh.order_hint_bits_minus1 + 1);

        const uint8_t allFrames = (1 << NUM_REF_FRAMES) - 1;

        if (FrameIsResilient(fh))
            fh.primary_ref_frame = PRIMARY_REF_NONE;
        else
            fh.primary_ref_frame = GetBits(PRIMARY_REF_BITS);

        if (sh.decoder_model_info_present_flag)
        {
            const int buffer_removal_time_present_flag = GetBit();
            if (buffer_removal_time_present_flag)
            {
                for (uint32_t opNum = 0; opNum <= sh.operating_points_cnt_minus_1; opNum++)
                {
                    if (sh.decoder_model_present_for_this_op[opNum])
                    {
                        const int opPtIdc = sh.operating_point_idc[opNum];
                        const int inTemporalLayer = (opPtIdc >> obuHeader.temporal_id) & 1;
                        const int inSpatialLayer = (opPtIdc >> (obuHeader.spatial_id + 8)) & 1;
                        if (opPtIdc == 0 || (inTemporalLayer && inSpatialLayer))
                        {
                            const int n = sh.decoder_model_info.buffer_removal_time_length_minus_1 + 1;
                            GetBits(n); // buffer_removal_time
                        }
                    }
                }

            }
        }

        if (fh.frame_type == SWITCH_FRAME ||
            (fh.frame_type == KEY_FRAME && fh.show_frame))
            fh.refresh_frame_flags = allFrames;
        else
            fh.refresh_frame_flags = static_cast<uint8_t>(GetBits(NUM_REF_FRAMES));

        if (!FrameIsIntra(fh) || fh.refresh_frame_flags != allFrames)
        {
            if (fh.error_resilient_mode && sh.enable_order_hint)
            {
                for (int i = 0; i < NUM_REF_FRAMES; i++)
                {
                    const int OrderHintBits = sh.order_hint_bits_minus1 + 1;
                    fh.ref_order_hint[i] = GetBits(OrderHintBits);
                    FrameHeader const& refHdr = frameDpb[i]->GetFrameHeader();
                    if (fh.ref_order_hint[i] != refHdr.order_hint)
                        frameDpb[i]->SetRefValid(false);
                }
            }
        }

        if (KEY_FRAME == fh.frame_type)
        {
            for (uint8_t i = 0; i < INTER_REFS; ++i)
            {
                fh.ref_frame_idx[i] = -1;
            }

            av1_setup_frame_size(*this, fh, sh);

            if (fh.allow_screen_content_tools && fh.FrameWidth == fh.UpscaledWidth)
                fh.allow_intrabc = GetBit();
        }
        else
        {
            if (FrameIsIntraOnly(fh))
            {
                av1_setup_frame_size(*this, fh, sh);

                if (fh.allow_screen_content_tools && fh.FrameWidth == fh.UpscaledWidth)
                    fh.allow_intrabc = GetBit();
            }
            else
            {
                uint32_t frame_refs_short_signalling = 0;

                if (sh.enable_order_hint)
                    frame_refs_short_signalling = GetBit();

                if (frame_refs_short_signalling)
                {
                    const uint32_t last_frame_idx = GetBits(REF_FRAMES_LOG2);
                    const uint32_t gold_frame_idx = GetBits(REF_FRAMES_LOG2);

                    // set last and gold reference frames
                    fh.ref_frame_idx[LAST_FRAME - LAST_FRAME] = last_frame_idx;
                    fh.ref_frame_idx[GOLDEN_FRAME - LAST_FRAME] = gold_frame_idx;

                    // compute other active references as specified in "Set frame refs process" AV1 spec 7.8
                    av1_set_frame_refs(sh, fh, frameDpb, last_frame_idx, gold_frame_idx);
                }

                for (uint8_t i = 0; i < INTER_REFS; ++i)
                {
                    if (!frame_refs_short_signalling)
                        fh.ref_frame_idx[i] = GetBits(REF_FRAMES_LOG2);

                    if (sh.frame_id_numbers_present_flag)
                    {
                        const int32_t deltaFrameId = GetBits(sh.delta_frame_id_length) + 1;

                        // compute expected reference frame id from delta_frame_id and check that it's equal to one saved in DPB
                        const uint32_t idLen = sh.idLen;
                        const uint32_t expectedFrameId = ((fh.current_frame_id - deltaFrameId + (1 << idLen))
                            % (1 << idLen));

                        AV1DecoderFrame const* refFrm = frameDpb[fh.ref_frame_idx[i]];
                        FrameHeader const& refHdr = refFrm->GetFrameHeader();

                        if (expectedFrameId != refHdr.current_frame_id ||
                            false == refFrm->RefValid())
                        {
                            VM_ASSERT("Active reference frame is absent in DPB or invalid!");
                        }
                    }
                }

                if (fh.error_resilient_mode == 0 && fh.frame_size_override_flag)
                    av1_setup_frame_size_with_refs(*this, fh, frameDpb, sh);
                else
                    av1_setup_frame_size(*this, fh, sh);

                if (fh.force_integer_mv)
                    fh.allow_high_precision_mv = 0;
                else
                    fh.allow_high_precision_mv = GetBit();

                av1_read_interpolation_filter(*this, fh);
                fh.is_motion_mode_switchable = GetBit();

                if (FrameMightUsePrevFrameMVs(fh, sh))
                {
                    fh.use_ref_frame_mvs = GetBit();
                }
            }
        }

        if (!fh.FrameWidth || !fh.FrameHeight)
            throw av1_exception(UMC::UMC_ERR_FAILED);

        const int mightBwdAdapt = !sh.reduced_still_picture_header && !fh.disable_cdf_update;
        if (mightBwdAdapt)
            fh.disable_frame_end_update_cdf = GetBit();
        else
            fh.disable_frame_end_update_cdf = 1;

        if (fh.primary_ref_frame == PRIMARY_REF_NONE)
            SetupPastIndependence(fh);
        else
            Av1LoadPrevious(fh, frameDpb);

        av1_read_tile_info(*this, fh.tile_info, fh, sh.sbSize);

        fh.NumPlanes = GetNumPlanes(sh);

        av1_setup_quantization(*this, fh.quantization_params, sh, fh.NumPlanes);

        av1_setup_segmentation(*this, fh.segmentation_params, fh);

        av1_read_delta_q_params(*this, fh);
        av1_read_delta_lf_params(*this, fh);

        fh.CodedLossless = IsCodedLossless(fh);
        fh.AllLossless = fh.CodedLossless && (fh.FrameWidth == fh.UpscaledWidth);

        av1_setup_loop_filter(*this, fh.loop_filter_params, fh);

        av1_read_cdef(*this, fh.cdef_params, sh, fh);

        av1_read_lr_params(*this, fh.lr_params, sh, fh);

        av1_read_tx_mode(*this, fh, fh.CodedLossless);

        av1_read_frame_reference_mode(*this, fh, frameDpb);

        fh.skip_mode_present = av1_is_skip_mode_allowed(fh, sh, frameDpb) ? GetBit() : 0;

        if (!FrameIsResilient(fh) && sh.enable_warped_motion)
            fh.allow_warped_motion = GetBit();

        fh.reduced_tx_set = GetBit();

        av1_read_global_motion(*this, fh, frameDpb);

        av1_read_film_grain(*this, sh, fh, frameDpb);

        AV1D_LOG("[-]: %d", (uint32_t)BitsDecoded());
    }

} // namespace UMC_AV1_DECODER

#endif // MFX_ENABLE_AV1_VIDEO_DECODE
