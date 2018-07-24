// Copyright (c) 2018 Intel Corporation
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

#include "hevc2_parser.h"
#include "hevc2_trace.h"
#include <functional>

using namespace BS_HEVC2;
using namespace BsReader2;

void Parser::parseNUH(NALU& nalu)
{
    TLAuto tl(*this, TRACE_NUH);

    BS2_TRACE_STR("----------------------------------");

    TLStart(TRACE_OFFSET);
    BS2_TRACE(nalu.StartOffset, nalu.StartOffset);
    TLEnd();

    if (u1())
        throw InvalidSyntax();

    BS2_SETM(u(6), nalu.nal_unit_type, NaluTraceMap);
    BS2_SET(u(6), nalu.nuh_layer_id);
    BS2_SET(u(3), nalu.nuh_temporal_id_plus1);
}

void Parser::parseAUD(AUD& aud)
{
    TLAuto tl(*this, TRACE_AUD);

    BS2_SETM(u(3), aud.pic_type, PicTypeTraceMap);

    if (!TrailingBits())
        throw InvalidSyntax();
}

void Parser::parseSLO(SubLayerOrderingInfo* slo, bool f, Bs32u n)
{
#define PARSE_SLO(_i)\
    if ((_i) == n || (f && (_i) <= n)) \
    { \
        BS2_SET(ue(), slo[_i].max_dec_pic_buffering_minus1); \
        BS2_SET(ue(), slo[_i].max_num_reorder_pics); \
        BS2_SET(ue(), slo[_i].max_latency_increase_plus1);  \
    }

    PARSE_SLO(0);
    PARSE_SLO(1);
    PARSE_SLO(2);
    PARSE_SLO(3);
    PARSE_SLO(4);
    PARSE_SLO(5);
    PARSE_SLO(6);
    PARSE_SLO(7);

#undef PARSE_SLO
}

inline bool PCF(Bs32u f, Bs32u i) { return !!(f & (1 << (31 - i))); }
inline bool isProfile(PTL& ptl, Bs32u profile)
{
    return (ptl.profile_idc == profile || PCF(ptl.profile_compatibility_flags, profile));
}

void Parser::parsePTL(PTL& ptl)
{
    if (ptl.profile_present_flag)
    {
        BS2_SET(u(2), ptl.profile_space);
        BS2_SET(u1(), ptl.tier_flag);
        BS2_SETM(u(5), ptl.profile_idc, ProfileTraceMap());

        ptl.profile_compatibility_flags = u(32);
        BS2_TRACE_BIN(&ptl.profile_compatibility_flags, 0, 32, ptl.profile_compatibility_flags);

        BS2_SET(u1(), ptl.progressive_source_flag);
        BS2_SET(u1(), ptl.interlaced_source_flag);
        BS2_SET(u1(), ptl.non_packed_constraint_flag);
        BS2_SET(u1(), ptl.frame_only_constraint_flag);

        if (   isProfile(ptl, REXT)
            || isProfile(ptl, REXT_HT)
            || isProfile(ptl, MAIN_MV)
            || isProfile(ptl, MAIN_SC)
            || isProfile(ptl, MAIN_3D)
            || isProfile(ptl, SCC)
            || isProfile(ptl, REXT_SC))
        {
            BS2_SET(u1(), ptl.max_12bit_constraint_flag);
            BS2_SET(u1(), ptl.max_10bit_constraint_flag);
            BS2_SET(u1(), ptl.max_8bit_constraint_flag);
            BS2_SET(u1(), ptl.max_422chroma_constraint_flag);
            BS2_SET(u1(), ptl.max_420chroma_constraint_flag);
            BS2_SET(u1(), ptl.max_monochrome_constraint_flag);
            BS2_SET(u1(), ptl.intra_constraint_flag);
            BS2_SET(u1(), ptl.one_picture_only_constraint_flag);
            BS2_SET(u1(), ptl.lower_bit_rate_constraint_flag);

            if (   isProfile(ptl, REXT_HT)
                || isProfile(ptl, SCC)
                || isProfile(ptl, REXT_SC))
            {
                BS2_SET(u1(), ptl.max_14bit_constraint_flag);
                BS2_SET(u(10), ptl.reserved_zero_33bits_0_9);
                BS2_SET(u(23), ptl.reserved_zero_33bits_10_32);
            }
            else
            {
                BS2_SET(u(11), ptl.reserved_zero_34bits_0_10);
                BS2_SET(u(23), ptl.reserved_zero_34bits_11_33);
            }
        }
        else
        {
            BS2_SET(u(20), ptl.reserved_zero_43bits_0_19);
            BS2_SET(u(23), ptl.reserved_zero_43bits_20_42);
        }

        if (   isProfile(ptl, MAIN)
            || isProfile(ptl, MAIN_10)
            || isProfile(ptl, MAIN_SP)
            || isProfile(ptl, REXT)
            || isProfile(ptl, REXT_HT)
            || isProfile(ptl, SCC))
        {
            BS2_SET(u1(), ptl.inbld_flag);
        }
        else
            BS2_SET(u1(), ptl.reserved_zero_bit);
    }

    if (ptl.level_present_flag)
        BS2_SET(u(8), ptl.level_idc);
}

void Parser::parsePTL(SubLayers& sl, Bs32u max_sub_layers_minus1)
{
    sl.general.profile_present_flag = 1;
    sl.general.level_present_flag   = 1;

    parsePTL(sl.general);

    for (Bs32u i = 0; i < max_sub_layers_minus1; i++)
    {
        BS2_SET(u1(), sl.sub_layer[i].profile_present_flag);
        BS2_SET(u1(), sl.sub_layer[i].level_present_flag);
    }

    if (max_sub_layers_minus1)
        u((8 - max_sub_layers_minus1) * 2);

    for (Bs32u i = 0; i < max_sub_layers_minus1; i++)
    {
        BS2_TRACE(i, sub_layer);
        parsePTL(sl.sub_layer[i]);
    }
}

void Parser::parseHRD(
    HRD& hrd, void* base, bool commonInfPresentFlag, Bs32u maxNumSubLayersMinus1)
{
    if (commonInfPresentFlag)
    {
        BS2_SET(u1(), hrd.nal_hrd_parameters_present_flag);
        BS2_SET(u1(), hrd.vcl_hrd_parameters_present_flag);

        if (   hrd.nal_hrd_parameters_present_flag
            || hrd.vcl_hrd_parameters_present_flag)
        {
            BS2_SET(u1(), hrd.sub_pic_hrd_params_present_flag);

            if (hrd.sub_pic_hrd_params_present_flag)
            {
                BS2_SET(u(8), hrd.tick_divisor_minus2);
                BS2_SET(u(5), hrd.du_cpb_removal_delay_increment_length_minus1);
                BS2_SET(u1(), hrd.sub_pic_cpb_params_in_pic_timing_sei_flag);
                BS2_SET(u(5), hrd.dpb_output_delay_du_length_minus1);
            }

            BS2_SET(u(4), hrd.bit_rate_scale);
            BS2_SET(u(4), hrd.cpb_size_scale);

            if (hrd.sub_pic_hrd_params_present_flag)
                BS2_SET(u(4), hrd.cpb_size_du_scale);

            BS2_SET(u(5), hrd.initial_cpb_removal_delay_length_minus1);
            BS2_SET(u(5), hrd.au_cpb_removal_delay_length_minus1);
            BS2_SET(u(5), hrd.dpb_output_delay_length_minus1);
        }
    }

    for (Bs32u i = 0; i <= maxNumSubLayersMinus1; i++)
    {
        auto& slhrd = hrd.sl[i];

        BS2_TRACE(i, SubLayer);

        BS2_SET(u1(), slhrd.fixed_pic_rate_general_flag);
        slhrd.fixed_pic_rate_within_cvs_flag = slhrd.fixed_pic_rate_general_flag;

        if (!slhrd.fixed_pic_rate_general_flag)
            BS2_SET(u1(), slhrd.fixed_pic_rate_within_cvs_flag);

        if (slhrd.fixed_pic_rate_within_cvs_flag)
        {
            BS2_SET(ue(), slhrd.elemental_duration_in_tc_minus1);
        }
        else
            BS2_SET(u1(), slhrd.low_delay_hrd_flag);

        if (!slhrd.low_delay_hrd_flag)
            BS2_SET(ue(), slhrd.cpb_cnt_minus1);

        if (hrd.nal_hrd_parameters_present_flag)
        {
            slhrd.nal = alloc<HRD::SubLayer::CPB>(base, slhrd.cpb_cnt_minus1 + 1);

            BS2_TRACE_STR("NAL HRD");

            for (i = 0; i <= slhrd.cpb_cnt_minus1; i++)
            {
                auto& cpb = slhrd.nal[i];

                BS2_SET(ue(), cpb.bit_rate_value_minus1);
                BS2_SET(ue(), cpb.cpb_size_value_minus1);

                if (hrd.sub_pic_hrd_params_present_flag)
                {
                    BS2_SET(ue(), cpb.bit_rate_du_value_minus1);
                    BS2_SET(ue(), cpb.cpb_size_du_value_minus1);
                }

                BS2_SET(u1(), cpb.cbr_flag);
            }
        }

        if (hrd.vcl_hrd_parameters_present_flag)
        {
            slhrd.vcl = alloc<HRD::SubLayer::CPB>(base, slhrd.cpb_cnt_minus1 + 1);

            BS2_TRACE_STR("VCL HRD");

            for (i = 0; i <= slhrd.cpb_cnt_minus1; i++)
            {
                auto& cpb = slhrd.vcl[i];

                BS2_SET(ue(), cpb.bit_rate_value_minus1);
                BS2_SET(ue(), cpb.cpb_size_value_minus1);

                if (hrd.sub_pic_hrd_params_present_flag)
                {
                    BS2_SET(ue(), cpb.bit_rate_du_value_minus1);
                    BS2_SET(ue(), cpb.cpb_size_du_value_minus1);
                }

                BS2_SET(u1(), cpb.cbr_flag);
            }
        }
    }
}

void Parser::parseVPS(VPS& vps)
{
    TLAuto tl(*this, TRACE_VPS);

    BS2_SET(u(4), vps.video_parameter_set_id);
    BS2_SET(u1(), vps.base_layer_internal_flag);
    BS2_SET(u1(), vps.base_layer_available_flag);
    BS2_SET(u(6), vps.max_layers_minus1);
    BS2_SET(u(3), vps.max_sub_layers_minus1);
    BS2_SET(u1(), vps.temporal_id_nesting_flag);
    BS2_SET(u(16), vps.reserved_0xffff_16bits);

    parsePTL(vps, vps.max_sub_layers_minus1);

    BS2_SET(u1(), vps.sub_layer_ordering_info_present_flag);
    parseSLO(vps.slo, vps.sub_layer_ordering_info_present_flag, vps.max_sub_layers_minus1);

    BS2_SET(u(6), vps.max_layer_id);
    BS2_SET(ue(), vps.num_layer_sets_minus1);

    {
        Bs32u S = (vps.max_layer_id + 1) * (vps.num_layer_sets_minus1 + 1);

        vps.layer_id_included_flags = alloc<Bs8u>(&vps, (S + 7) / 8);
        GetBits(vps.layer_id_included_flags, 1, S - 1);

        BS2_TRACE_BIN(vps.layer_id_included_flags, 0, S, vps.layer_id_included_flags);
    }

    BS2_SET(u1(), vps.timing_info_present_flag);

    if (vps.timing_info_present_flag)
    {
        BS2_SET(u(32), vps.num_units_in_tick);
        BS2_SET(u(32), vps.time_scale);
        BS2_SET(u1(), vps.poc_proportional_to_timing_flag);

        if (vps.poc_proportional_to_timing_flag)
            BS2_SET(ue(), vps.num_ticks_poc_diff_one_minus1);

        BS2_SET(ue(), vps.num_hrd_parameters);

        vps.hrd = alloc<VPS::VPSHRD>(&vps, vps.num_hrd_parameters);

        for (Bs32u i = 0; i < vps.num_hrd_parameters; i++)
        {
            BS2_SET(ue(), vps.hrd[i].hrd_layer_set_idx);

            if (i)
                BS2_SET(u1(), vps.hrd[i].cprms_present_flag)
            else
                vps.hrd[i].cprms_present_flag = 1;

            parseHRD(vps.hrd[i], vps.hrd,
                vps.hrd[i].cprms_present_flag, vps.max_sub_layers_minus1);
        }
    }

    BS2_SET(u1(), vps.extension_flag);

    if (vps.extension_flag)
    {
        vps.ExtBits = parseEXT(vps.ExtData);

        if (vps.ExtData)
            bound(vps.ExtData, &vps);
    }

    if (!TrailingBits())
        throw InvalidSyntax();
}

Bs32u Parser::parseEXT(Bs8u*& ExtData)
{
    std::vector<Bs8u> data;
    Bs32u ExtBits = 0;
    Bs8u B = 0;
    Bs8u O = 8;

    while (more_rbsp_data())
    {
        B |= (u1() << (--O));

        if (!O)
        {
            data.push_back(B);
            B = 0;
            O = 8;
        }
    }

    if (O < 8)
        data.push_back(B);

    BS2_SET(Bs32u(data.size() * 8 - O % 8), ExtBits);

    ExtData = alloc<Bs8u>(0, (Bs32u)data.size());

    memcpy(ExtData, data.data(), data.size());

    if (ExtBits > 32 * 8)
    {
        BS2_TRACE_ARR_F(ExtData, data.size(), 16, "%02X ");
    }
    else
    {
        BS2_TRACE_BIN(ExtData, 0, ExtBits, ExtData);
    }

    return ExtBits;
}

const Bs8u DSL_[3][64] =
{
    {
        16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    },
    {
        16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 17, 16, 17, 16, 17, 18,
        17, 18, 18, 17, 18, 21, 19, 20, 21, 20, 19, 21, 24, 22, 22, 24,
        24, 22, 22, 24, 25, 25, 27, 30, 27, 25, 25, 29, 31, 35, 35, 31,
        29, 36, 41, 44, 41, 36, 47, 54, 54, 47, 65, 70, 65, 88, 88, 115
    },
    {
        16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 18,
        18, 18, 18, 18, 18, 20, 20, 20, 20, 20, 20, 20, 24, 24, 24, 24,
        24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 28, 28, 28, 28, 28,
        28, 33, 33, 33, 33, 33, 41, 41, 41, 41, 54, 54, 54, 71, 71, 91
    },
};

inline const Bs8u* DSL(Bs32u sizeId, Bs32u matrixId)
{
    if (sizeId == 0)
        return DSL_[0];
    if (matrixId > 2)
        return DSL_[2];
    return DSL_[1];
}

void Parser::parseSLD(QM& qm)
{
    Bs8u list[64] = {};
    Bs16u dim[] = { 1, 4, 4 };

    for (Bs32u sizeId = 0; sizeId < 4; sizeId++)
    {
        for (Bs32u matrixId = 0; matrixId < 6; matrixId += (sizeId == 3) ? 3 : 1)
        {
            Bs32u num = std::min(64, (1 << (4 + (sizeId << 1))));
            Bs8u dc = 0;
            Bs32u refMatrixId = matrixId;

            if (!u1()) //scaling_list_pred_mode_flag
            {
                // scaling_list_pred_matrix_id_delta
                refMatrixId = matrixId - ue() * ((sizeId == 3) ? 3 : 1);

                if (refMatrixId == matrixId)
                {
                    memcpy(list, DSL(sizeId, refMatrixId), num);
                    dc = list[0];
                }
            }
            else
            {
                Bs32s next = 8;

                if (sizeId > 1)
                {
                    next = se() + 8; //scaling_list_dc_coef_minus8[ sizeId - 2 ][ matrixId ]
                    dc = Bs8u(next);
                }

                for (Bs32u i = 0; i < num; i++)
                {
                    next = (next + se() + 256) % 256; //scaling_list_delta_coef
                    list[i] = Bs8u(next);
                }
            }

            //dim[1] = dim[2] = 4 << sizeId;

            switch (sizeId)
            {
            case 0:
                if (refMatrixId < matrixId)
                {
                    memcpy(
                        qm.ScalingFactor0[matrixId],
                        qm.ScalingFactor0[refMatrixId],
                        sizeof(qm.ScalingFactor0[0]));
                }
                else
                {
                    for (Bs32u i = 0; i < 16; i++)
                        qm.ScalingFactor0[matrixId]
                            [ScanOrder2[0][i][0]]
                            [ScanOrder2[0][i][1]]
                            = list[i];
                }
                //BS2_TRACE_MDARR(Bs8u, qm.ScalingFactor0[matrixId], dim, 1, 0, "%3d ");
                break;
            case 1:
                if (refMatrixId < matrixId)
                {
                    memcpy(
                        qm.ScalingFactor1[matrixId],
                        qm.ScalingFactor1[refMatrixId],
                        sizeof(qm.ScalingFactor1[0]));
                }
                else
                {
                    for (Bs32u i = 0; i < 64; i++)
                        qm.ScalingFactor1[matrixId]
                            [ScanOrder3[0][i][0]]
                            [ScanOrder3[0][i][1]]
                            = list[i];
                }
                //BS2_TRACE_MDARR(Bs8u, qm.ScalingFactor1[matrixId], dim, 1, 0, "%3d ");
                break;
            case 2:
                if (refMatrixId < matrixId)
                {
                    memcpy(
                        qm.ScalingFactor2[matrixId],
                        qm.ScalingFactor2[refMatrixId],
                        sizeof(qm.ScalingFactor2[0]));
                }
                else
                {
                    for (Bs32u i = 0; i < 64; i++)
                        for (Bs32u j = 0; j < 2; j++)
                            for (Bs32u k = 0; k < 2; k++)
                                qm.ScalingFactor2[matrixId]
                                    [ScanOrder3[0][i][0] * 2 + k]
                                    [ScanOrder3[0][i][1] * 2 + j]
                                    = list[i];
                    qm.ScalingFactor2[matrixId][0][0] = dc;
                }
                //BS2_TRACE_MDARR(Bs8u, qm.ScalingFactor2[matrixId], dim, 1, 0, "%3d ");

                if (matrixId % 3 == 0)
                    break;

                dim[1] = dim[2] = 32;
            case 3:
                if (refMatrixId < matrixId)
                {
                    memcpy(
                        qm.ScalingFactor3[matrixId],
                        qm.ScalingFactor3[refMatrixId],
                        sizeof(qm.ScalingFactor3[0]));
                }
                else
                {
                    for (Bs32u i = 0; i < 64; i++)
                        for (Bs32u j = 0; j < 4; j++)
                            for (Bs32u k = 0; k < 4; k++)
                                qm.ScalingFactor3[matrixId]
                                    [ScanOrder3[0][i][0] * 4 + k]
                                    [ScanOrder3[0][i][1] * 4 + j]
                                    = list[i];
                    qm.ScalingFactor3[matrixId][0][0] = dc;
                }
                //BS2_TRACE_MDARR(Bs8u, qm.ScalingFactor3[matrixId], dim, 1, 0, "%3d ");
                break;
            default:
                throw Exception(BS_ERR_UNKNOWN);
            }
        }
    }

    dim[1] = dim[2] = 4;

    BS2_TRACE_MDARR(Bs8u, qm.ScalingFactor0[QM_IntraY],  dim, 1, 0, "%3d ");
    BS2_TRACE_MDARR(Bs8u, qm.ScalingFactor0[QM_IntraCb], dim, 1, 0, "%3d ");
    BS2_TRACE_MDARR(Bs8u, qm.ScalingFactor0[QM_IntraCr], dim, 1, 0, "%3d ");
    BS2_TRACE_MDARR(Bs8u, qm.ScalingFactor0[QM_InterY],  dim, 1, 0, "%3d ");
    BS2_TRACE_MDARR(Bs8u, qm.ScalingFactor0[QM_InterCb], dim, 1, 0, "%3d ");
    BS2_TRACE_MDARR(Bs8u, qm.ScalingFactor0[QM_InterCr], dim, 1, 0, "%3d ");

    dim[1] = dim[2] = 8;
    BS2_TRACE_MDARR(Bs8u, qm.ScalingFactor1[QM_IntraY],  dim, 1, 0, "%3d ");
    BS2_TRACE_MDARR(Bs8u, qm.ScalingFactor1[QM_IntraCb], dim, 1, 0, "%3d ");
    BS2_TRACE_MDARR(Bs8u, qm.ScalingFactor1[QM_IntraCr], dim, 1, 0, "%3d ");
    BS2_TRACE_MDARR(Bs8u, qm.ScalingFactor1[QM_InterY],  dim, 1, 0, "%3d ");
    BS2_TRACE_MDARR(Bs8u, qm.ScalingFactor1[QM_InterCb], dim, 1, 0, "%3d ");
    BS2_TRACE_MDARR(Bs8u, qm.ScalingFactor1[QM_InterCr], dim, 1, 0, "%3d ");

    dim[1] = dim[2] = 16;
    BS2_TRACE_MDARR(Bs8u, qm.ScalingFactor2[QM_IntraY],  dim, 1, 0, "%3d ");
    BS2_TRACE_MDARR(Bs8u, qm.ScalingFactor2[QM_IntraCb], dim, 1, 0, "%3d ");
    BS2_TRACE_MDARR(Bs8u, qm.ScalingFactor2[QM_IntraCr], dim, 1, 0, "%3d ");
    BS2_TRACE_MDARR(Bs8u, qm.ScalingFactor2[QM_InterY],  dim, 1, 0, "%3d ");
    BS2_TRACE_MDARR(Bs8u, qm.ScalingFactor2[QM_InterCb], dim, 1, 0, "%3d ");
    BS2_TRACE_MDARR(Bs8u, qm.ScalingFactor2[QM_InterCr], dim, 1, 0, "%3d ");

    dim[1] = dim[2] = 32;
    BS2_TRACE_MDARR(Bs8u, qm.ScalingFactor3[QM_IntraY],  dim, 1, 0, "%3d ");
    BS2_TRACE_MDARR(Bs8u, qm.ScalingFactor3[QM_IntraCb], dim, 1, 0, "%3d ");
    BS2_TRACE_MDARR(Bs8u, qm.ScalingFactor3[QM_IntraCr], dim, 1, 0, "%3d ");
    BS2_TRACE_MDARR(Bs8u, qm.ScalingFactor3[QM_InterY],  dim, 1, 0, "%3d ");
    BS2_TRACE_MDARR(Bs8u, qm.ScalingFactor3[QM_InterCb], dim, 1, 0, "%3d ");
    BS2_TRACE_MDARR(Bs8u, qm.ScalingFactor3[QM_InterCr], dim, 1, 0, "%3d ");
}

struct DPocSort
{
    bool operator () (Bs16s l, Bs16s r)
    {
        if (l < 0 && r > 0) return true;
        if (r < 0 && l > 0) return false;
        if (l < 0) return (l > r);
        return (l < r);
    }
};

void Parser::ParseSTRPS(STRPS* strps, Bs32u num, Bs32u idx)
{
    STRPS& cur = strps[idx];
    Bs16u& N = cur.NumDeltaPocs;

    if (idx == num)
        memset(&cur, 0, sizeof(cur));

    if (idx && u1()) //inter_ref_pic_set_prediction_flag
    {
        std::map<Bs16s, bool> Used;
        Bs32s dIdx = 1, dRPS = 0;

        if (idx == num)
            dIdx += ue(); //delta_idx_minus1

        dRPS = (1 - 2 * u1()) * (ue() + 1); //delta_rps_sign, abs_delta_rps_minus1

        STRPS& ref = strps[idx - dIdx];

        for (Bs32u i = 0; i <= ref.NumDeltaPocs; i++)
        {
            bool used = !!u1(); //used_by_curr_pic_flag

            if (used || u1()) //use_delta_flag
            {
                //SetBit(cur.UsedByCurrPicFlags, N, used);
                cur.DeltaPoc[N] = (i < ref.NumDeltaPocs ? ref.DeltaPoc[i] : 0) + dRPS;
                Used[cur.DeltaPoc[N]] = used;
                N++;
            }
        }

        std::sort(cur.DeltaPoc, cur.DeltaPoc + cur.NumDeltaPocs, DPocSort());

        for (Bs16u i = 0; i < N; i++)
            SetBit(cur.UsedByCurrPicFlags, i, Used[cur.DeltaPoc[i]]);
    }
    else
    {
        Bs16u neg = ue(); //num_negative_pics;
        N = neg + ue(); //num_positive_pics

        for (Bs32u i = 0; i < N; i++)
        {
            cur.DeltaPoc[i] = (1 - 2 * (i < neg)) * (ue() + 1); //delta_poc_sX_minus1
            if (i > 0 && i != neg)
                cur.DeltaPoc[i] += cur.DeltaPoc[i - 1];
            SetBit(cur.UsedByCurrPicFlags, i, !!u1()); //used_by_curr_pic_sX_flag
        }
    }
    //BS2_TRACE(cur.NumDeltaPocs, strps[idx].NumDeltaPocs);
    BS2_TRACE_BIN(&cur.UsedByCurrPicFlags, 0, cur.NumDeltaPocs, strps[idx].UsedByCurrPicFlags);
    BS2_TRACE_ARR(strps[idx].DeltaPoc, cur.NumDeltaPocs, 0);
}

void Parser::parseVUI(VUI& vui, Bs32u maxNumSubLayersMinus1)
{
    BS2_SET(u1(), vui.aspect_ratio_info_present_flag);

    if (vui.aspect_ratio_info_present_flag)
    {
        BS2_SETM(u(8), vui.aspect_ratio_idc, ARIdcTraceMap());

        if (vui.aspect_ratio_idc == Extended_SAR)
        {
            BS2_SET(u(16), vui.sar_width);
            BS2_SET(u(16), vui.sar_height);
        }
    }

    BS2_SET(u1(), vui.overscan_info_present_flag);

    if (vui.overscan_info_present_flag)
        BS2_SET(u1(), vui.overscan_appropriate_flag);

    BS2_SET(u1(), vui.video_signal_type_present_flag);

    if (vui.video_signal_type_present_flag)
    {
        BS2_SETM(u(3), vui.video_format, VideoFormatTraceMap);
        BS2_SET(u1(), vui.video_full_range_flag);
        BS2_SET(u1(), vui.colour_description_present_flag);

        if (vui.colour_description_present_flag)
        {
            BS2_SET(u(8), vui.colour_primaries);
            BS2_SET(u(8), vui.transfer_characteristics);
            BS2_SET(u(8), vui.matrix_coeffs);
        }
    }

    BS2_SET(u1(), vui.chroma_loc_info_present_flag);

    if (vui.chroma_loc_info_present_flag)
    {
        BS2_SET(ue(), vui.chroma_sample_loc_type_top_field);
        BS2_SET(ue(), vui.chroma_sample_loc_type_bottom_field);
    }

    BS2_SET(u1(), vui.neutral_chroma_indication_flag);
    BS2_SET(u1(), vui.field_seq_flag);
    BS2_SET(u1(), vui.frame_field_info_present_flag);
    BS2_SET(u1(), vui.default_display_window_flag);

    if (vui.default_display_window_flag)
    {
        BS2_SET(ue(), vui.def_disp_win_left_offset);
        BS2_SET(ue(), vui.def_disp_win_right_offset);
        BS2_SET(ue(), vui.def_disp_win_top_offset);
        BS2_SET(ue(), vui.def_disp_win_bottom_offset);
    }

    BS2_SET(u1(), vui.timing_info_present_flag);

    if (vui.timing_info_present_flag)
    {
        BS2_SET(u(32), vui.num_units_in_tick);
        BS2_SET(u(32), vui.time_scale);
        BS2_SET(u1(), vui.poc_proportional_to_timing_flag);

        if (vui.poc_proportional_to_timing_flag)
            BS2_SET(ue(), vui.num_ticks_poc_diff_one_minus1);

        BS2_SET(u1(), vui.hrd_parameters_present_flag);

        if (vui.hrd_parameters_present_flag)
        {
            vui.hrd = alloc<HRD>(&vui);
            parseHRD(*vui.hrd, vui.hrd, 1, maxNumSubLayersMinus1);
        }
    }

    BS2_SET(u1(), vui.bitstream_restriction_flag);

    if (vui.bitstream_restriction_flag)
    {
        BS2_SET(u1(), vui.tiles_fixed_structure_flag);
        BS2_SET(u1(), vui.motion_vectors_over_pic_boundaries_flag);
        BS2_SET(u1(), vui.restricted_ref_pic_lists_flag);
        BS2_SET(ue(), vui.min_spatial_segmentation_idc);
        BS2_SET(ue(), vui.max_bytes_per_pic_denom);
        BS2_SET(ue(), vui.max_bits_per_min_cu_denom);
        BS2_SET(ue(), vui.log2_max_mv_length_horizontal);
        BS2_SET(ue(), vui.log2_max_mv_length_vertical);
    }
}

void Parser::parseSPS(SPS& sps)
{
    TLAuto tl(*this, TRACE_SPS);

    BS2_SET(u(4), sps.video_parameter_set_id);
    BS2_SET(u(3), sps.max_sub_layers_minus1);
    BS2_SET(u1(), sps.temporal_id_nesting_flag);

    parsePTL(sps, sps.max_sub_layers_minus1);

    BS2_SET(ue(), sps.seq_parameter_set_id);
    BS2_SETM(ue(), sps.chroma_format_idc, ChromaFormatTraceMap);

    if (sps.chroma_format_idc == CHROMA_444)
        BS2_SET(u1(), sps.temporal_id_nesting_flag);

    BS2_SET(ue(), sps.pic_width_in_luma_samples);
    BS2_SET(ue(), sps.pic_height_in_luma_samples);

    BS2_SET(u1(), sps.conformance_window_flag);

    if (sps.conformance_window_flag)
    {
        BS2_SET(ue(), sps.conf_win_left_offset);
        BS2_SET(ue(), sps.conf_win_right_offset);
        BS2_SET(ue(), sps.conf_win_top_offset);
        BS2_SET(ue(), sps.conf_win_bottom_offset);
    }

    BS2_SET(ue(), sps.bit_depth_luma_minus8);
    BS2_SET(ue(), sps.bit_depth_chroma_minus8);
    BS2_SET(ue(), sps.log2_max_pic_order_cnt_lsb_minus4);

    BS2_SET(u1(), sps.sub_layer_ordering_info_present_flag);

    parseSLO(sps.slo, sps.sub_layer_ordering_info_present_flag, sps.max_sub_layers_minus1);

    BS2_SET(ue(), sps.log2_min_luma_coding_block_size_minus3);
    BS2_SET(ue(), sps.log2_diff_max_min_luma_coding_block_size);
    BS2_SET(ue(), sps.log2_min_luma_transform_block_size_minus2);
    BS2_SET(ue(), sps.log2_diff_max_min_luma_transform_block_size);
    BS2_SET(ue(), sps.max_transform_hierarchy_depth_inter);
    BS2_SET(ue(), sps.max_transform_hierarchy_depth_intra);

    BS2_SET(u1(), sps.scaling_list_enabled_flag);

    if (sps.scaling_list_enabled_flag)
    {
        BS2_SET(u1(), sps.scaling_list_data_present_flag);

        if (sps.scaling_list_data_present_flag)
        {
            sps.qm = alloc<QM>(&sps);
            parseSLD(*sps.qm);
        }
    }

    BS2_SET(u1(), sps.amp_enabled_flag);
    BS2_SET(u1(), sps.sample_adaptive_offset_enabled_flag);
    BS2_SET(u1(), sps.pcm_enabled_flag);

    if (sps.pcm_enabled_flag)
    {
        BS2_SET(u(4), sps.pcm_sample_bit_depth_luma_minus1);
        BS2_SET(u(4), sps.pcm_sample_bit_depth_chroma_minus1);
        BS2_SET(ue(), sps.log2_min_pcm_luma_coding_block_size_minus3);
        BS2_SET(ue(), sps.log2_diff_max_min_pcm_luma_coding_block_size);
        BS2_SET(u1(), sps.pcm_loop_filter_disabled_flag);
    }

    BS2_SET(ue(), sps.num_short_term_ref_pic_sets);
    sps.strps = alloc<STRPS>(&sps, sps.num_short_term_ref_pic_sets + 1);

    if (sps.num_short_term_ref_pic_sets)
    {
        for (Bs32u i = 0; i < sps.num_short_term_ref_pic_sets; i++)
            ParseSTRPS(sps.strps, sps.num_short_term_ref_pic_sets, i);
    }

    BS2_SET(u1(), sps.long_term_ref_pics_present_flag);

    if (sps.long_term_ref_pics_present_flag)
    {
        BS2_SET(ue(), sps.num_long_term_ref_pics);

        if (sps.num_long_term_ref_pics)
        {
            sps.lt_ref_pic_poc_lsb = alloc<Bs16u>(&sps, sps.num_long_term_ref_pics);

            for (Bs32u i = 0; i < sps.num_long_term_ref_pics; i++)
            {
                sps.lt_ref_pic_poc_lsb[i] = u(sps.log2_max_pic_order_cnt_lsb_minus4 + 4);
                SetBit(sps.used_by_curr_pic_lt_flags, i, !!u1());
            }

            BS2_TRACE_BIN(&sps.used_by_curr_pic_lt_flags, 0, sps.num_long_term_ref_pics, sps.used_by_curr_pic_lt_flags);
            BS2_TRACE_ARR(sps.lt_ref_pic_poc_lsb, sps.num_long_term_ref_pics, 0);
        }
    }

    BS2_SET(u1(), sps.temporal_mvp_enabled_flag);
    BS2_SET(u1(), sps.strong_intra_smoothing_enabled_flag);
    BS2_SET(u1(), sps.vui_parameters_present_flag);

    if (sps.vui_parameters_present_flag)
    {
        sps.vui = alloc<VUI>(&sps);
        parseVUI(*sps.vui, sps.max_sub_layers_minus1);
    }

    BS2_SET(u1(), sps.extension_present_flag);

    if (sps.extension_present_flag)
    {
        BS2_SET(u1(), sps.range_extension_flag);
        BS2_SET(u1(), sps.multilayer_extension_flag);
        BS2_SET(u1(), sps.extension_3d_flag);
        BS2_SET(u1(), sps.scc_extension_flag);
        BS2_SET(u(4), sps.extension_xbits);
    }

    if (sps.range_extension_flag)
    {
        BS2_SET(u1(), sps.transform_skip_rotation_enabled_flag);
        BS2_SET(u1(), sps.transform_skip_context_enabled_flag);
        BS2_SET(u1(), sps.implicit_rdpcm_enabled_flag);
        BS2_SET(u1(), sps.explicit_rdpcm_enabled_flag);
        BS2_SET(u1(), sps.extended_precision_processing_flag);
        BS2_SET(u1(), sps.intra_smoothing_disabled_flag);
        BS2_SET(u1(), sps.high_precision_offsets_enabled_flag);
        BS2_SET(u1(), sps.persistent_rice_adaptation_enabled_flag);
        BS2_SET(u1(), sps.cabac_bypass_alignment_enabled_flag);
    }

    if (sps.multilayer_extension_flag)
        BS2_SET(u1(), sps.inter_view_mv_vert_constraint_flag);

    if (sps.extension_3d_flag)
    {
        //sps_3d_extension() /* specified in Annex I */
        BS2_TRACE(u1(), iv_di_mc_enabled_flag[0]);
        BS2_TRACE(u1(), iv_mv_scal_enabled_flag[0]);
        BS2_TRACE(ue(), log2_ivmc_sub_pb_size_minus3[0]);
        BS2_TRACE(u1(), iv_res_pred_enabled_flag[0]);
        BS2_TRACE(u1(), depth_ref_enabled_flag[0]);
        BS2_TRACE(u1(), vsp_mc_enabled_flag[0]);
        BS2_TRACE(u1(), dbbp_enabled_flag[0]);
        BS2_TRACE(u1(), iv_di_mc_enabled_flag[1]);
        BS2_TRACE(u1(), iv_mv_scal_enabled_flag[1]);
        BS2_TRACE(u1(), tex_mc_enabled_flag[1]);
        BS2_TRACE(ue(), log2_texmc_sub_pb_size_minus3[1]);
        BS2_TRACE(u1(), intra_contour_enabled_flag[1]);
        BS2_TRACE(u1(), intra_dc_only_wedge_enabled_flag[1]);
        BS2_TRACE(u1(), cqt_cu_part_pred_enabled_flag[1]);
        BS2_TRACE(u1(), inter_dc_only_enabled_flag[1]);
        BS2_TRACE(u1(), skip_intra_enabled_flag[1]);
    }

    if (sps.scc_extension_flag)
    {
        BS2_SET(u1(), sps.curr_pic_ref_enabled_flag);
        BS2_SET(u1(), sps.palette_mode_enabled_flag);

        if (sps.palette_mode_enabled_flag)
        {
            BS2_SET(ue(), sps.palette_max_size);
            BS2_SET((Bs32s)ue(), sps.delta_palette_max_predictor_size);
            BS2_SET(u1(), sps.palette_predictor_initializer_present_flag);

            if (sps.palette_predictor_initializer_present_flag)
            {
                BS2_SET(ue() + 1, sps.num_palette_predictor_initializer);
                Bs16u numComps = (sps.chroma_format_idc == CHROMA_400) ? 1 : 3;

                sps.palette_predictor_initializers[0] = alloc<Bs16u>(&sps, sps.num_palette_predictor_initializer * numComps);
                BS2_SET_ARR(u(sps.bit_depth_luma_minus8 + 8)
                    , sps.palette_predictor_initializers[0]
                    , sps.num_palette_predictor_initializer, 0);

                if (numComps > 1)
                {
                    sps.palette_predictor_initializers[1] = sps.palette_predictor_initializers[0] + sps.num_palette_predictor_initializer;
                    sps.palette_predictor_initializers[2] = sps.palette_predictor_initializers[1] + sps.num_palette_predictor_initializer;

                    BS2_SET_ARR(u(sps.bit_depth_chroma_minus8 + 8)
                        , sps.palette_predictor_initializers[1]
                        , sps.num_palette_predictor_initializer, 0);
                    BS2_SET_ARR(u(sps.bit_depth_chroma_minus8 + 8)
                        , sps.palette_predictor_initializers[2]
                        , sps.num_palette_predictor_initializer, 0);
                }
            }
        }

        BS2_SET(u(2), sps.motion_vector_resolution_control_idc);
        BS2_SET(u1(), sps.intra_boundary_filtering_disabled_flag);
    }

    if (sps.extension_xbits)
    {
        sps.ExtBits = parseEXT(sps.ExtData);

        if (sps.ExtData)
            bound(sps.ExtData, &sps);
    }

    if (!TrailingBits())
        throw InvalidSyntax();
}

void Parser::parsePPS(PPS& pps)
{
    TLAuto tl(*this, TRACE_PPS);

    BS2_SET(ue(), pps.pic_parameter_set_id);
    BS2_SET(ue(), pps.seq_parameter_set_id);

    SPS& sps = *m_sps[pps.seq_parameter_set_id];

    BS2_SET(u1(), pps.dependent_slice_segments_enabled_flag);
    BS2_SET(u1(), pps.output_flag_present_flag);
    BS2_SET(u(3), pps.num_extra_slice_header_bits);
    BS2_SET(u1(), pps.sign_data_hiding_enabled_flag);
    BS2_SET(u1(), pps.cabac_init_present_flag);
    BS2_SET(ue(), pps.num_ref_idx_l0_default_active_minus1);
    BS2_SET(ue(), pps.num_ref_idx_l1_default_active_minus1);
    BS2_SET(se(), pps.init_qp_minus26);
    BS2_SET(u1(), pps.constrained_intra_pred_flag);
    BS2_SET(u1(), pps.transform_skip_enabled_flag);
    BS2_SET(u1(), pps.cu_qp_delta_enabled_flag);

    if (pps.cu_qp_delta_enabled_flag)
        BS2_SET(ue(), pps.diff_cu_qp_delta_depth);

    BS2_SET(se(), pps.cb_qp_offset);
    BS2_SET(se(), pps.cr_qp_offset);

    BS2_SET(u1(), pps.slice_chroma_qp_offsets_present_flag);
    BS2_SET(u1(), pps.weighted_pred_flag);
    BS2_SET(u1(), pps.weighted_bipred_flag);
    BS2_SET(u1(), pps.transquant_bypass_enabled_flag);
    BS2_SET(u1(), pps.tiles_enabled_flag);
    BS2_SET(u1(), pps.entropy_coding_sync_enabled_flag);

    if (pps.tiles_enabled_flag)
    {
        BS2_SET(ue(), pps.num_tile_columns_minus1);
        BS2_SET(ue(), pps.num_tile_rows_minus1);
        BS2_SET(u1(), pps.uniform_spacing_flag);

        pps.column_width_minus1 = alloc<Bs16u>(&pps, pps.num_tile_columns_minus1 + 1);
        pps.row_height_minus1 = alloc<Bs16u>(&pps, pps.num_tile_rows_minus1 + 1);

        if (!pps.uniform_spacing_flag)
        {
            BS2_SET_ARR(ue(), pps.column_width_minus1, pps.num_tile_columns_minus1, 0);
            BS2_SET_ARR(ue(), pps.row_height_minus1, pps.num_tile_rows_minus1, 0);
        }
        else
        {
            Bs16u LCUSize = (1 << (sps.log2_min_luma_coding_block_size_minus3 + 3 + sps.log2_diff_max_min_luma_coding_block_size));
            Bs16u nCol = CeilDiv<Bs16u>(sps.pic_width_in_luma_samples, LCUSize);
            Bs16u nRow = CeilDiv<Bs16u>(sps.pic_height_in_luma_samples, LCUSize);
            Bs16u nTCol = pps.num_tile_columns_minus1 + 1;
            Bs16u nTRow = pps.num_tile_rows_minus1 + 1;

            for (Bs16u i = 0; i < nTCol - 1; i++)
                pps.column_width_minus1[i] = ((i + 1) * nCol) / nTCol - (i * nCol) / nTCol - 1;
            for (Bs16u j = 0; j < nTRow - 1; j++)
                pps.row_height_minus1[j] = ((j + 1) * nRow) / nTRow - (j * nRow) / nTRow - 1;
        }

        BS2_SET(u1(), pps.loop_filter_across_tiles_enabled_flag);
    }

    BS2_SET(u1(), pps.loop_filter_across_slices_enabled_flag);
    BS2_SET(u1(), pps.deblocking_filter_control_present_flag);

    if (pps.deblocking_filter_control_present_flag)
    {
        BS2_SET(u1(), pps.deblocking_filter_override_enabled_flag);
        BS2_SET(u1(), pps.deblocking_filter_disabled_flag);

        if (!pps.deblocking_filter_disabled_flag)
        {
            BS2_SET(se(), pps.beta_offset_div2);
            BS2_SET(se(), pps.tc_offset_div2);
        }
    }

    BS2_SET(u1(), pps.scaling_list_data_present_flag);

    if (pps.scaling_list_data_present_flag)
    {
        pps.qm = alloc<QM>(&pps);
        parseSLD(*pps.qm);
    }

    BS2_SET(u1(), pps.lists_modification_present_flag);
    BS2_SET(ue(), pps.log2_parallel_merge_level_minus2);
    BS2_SET(u1(), pps.slice_segment_header_extension_present_flag);
    BS2_SET(u1(), pps.extension_present_flag);

    if (pps.extension_present_flag)
    {
        BS2_SET(u1(), pps.range_extension_flag);
        BS2_SET(u1(), pps.multilayer_extension_flag);
        BS2_SET(u1(), pps.extension_3d_flag);
        BS2_SET(u1(), pps.scc_extension_flag);
        BS2_SET(u(4), pps.extension_xbits);
    }

    if (pps.range_extension_flag)
    {
        if (pps.transform_skip_enabled_flag)
            BS2_SET(ue(), pps.log2_max_transform_skip_block_size_minus2);

        BS2_SET(u1(), pps.cross_component_prediction_enabled_flag);
        BS2_SET(u1(), pps.chroma_qp_offset_list_enabled_flag);

        if (pps.chroma_qp_offset_list_enabled_flag)
        {

            BS2_SET(ue(), pps.diff_cu_chroma_qp_offset_depth);
            BS2_SET(ue(), pps.chroma_qp_offset_list_len_minus1);

            for (Bs32u i = 0; i <= pps.chroma_qp_offset_list_len_minus1; i++)
            {
                pps.cb_qp_offset_list[i] = se();
                pps.cr_qp_offset_list[i] = se();
            }

            BS2_TRACE_ARR(pps.cb_qp_offset_list, pps.chroma_qp_offset_list_len_minus1 + 1, 0);
            BS2_TRACE_ARR(pps.cr_qp_offset_list, pps.chroma_qp_offset_list_len_minus1 + 1, 0);
        }

        BS2_SET(ue(), pps.log2_sao_offset_scale_luma);
        BS2_SET(ue(), pps.log2_sao_offset_scale_chroma);
    }

    if (pps.multilayer_extension_flag)
    {
        //pps_multilayer_extension() /* specified in Annex F */
        parseMLE(pps);
    }

    if (pps.extension_3d_flag)
    {
        //pps_3d_extension() /* specified in Annex I */
        parse3DE(pps);
    }

    if (pps.scc_extension_flag)
    {
        BS2_SET(u1(), pps.curr_pic_ref_enabled_flag);
        BS2_SET(u1(), pps.residual_adaptive_colour_transform_enabled_flag);

        if (pps.residual_adaptive_colour_transform_enabled_flag)
        {
            BS2_SET(u1(), pps.slice_act_qp_offsets_present_flag);
            BS2_SET(se() - 5, pps.ActQpOffsetY);
            BS2_SET(se() - 5, pps.ActQpOffsetCb);
            BS2_SET(se() - 3, pps.ActQpOffsetCr);
        }

        BS2_SET(u1(), pps.palette_predictor_initializer_present_flag);

        if (pps.palette_predictor_initializer_present_flag)
        {
            BS2_SET(ue(), pps.num_palette_predictor_initializer);

            if (pps.num_palette_predictor_initializer)
            {
                BS2_SET(u1(), pps.monochrome_palette_flag);
                BS2_SET(ue(), pps.luma_bit_depth_entry_minus8);

                if (!pps.monochrome_palette_flag)
                    BS2_SET(ue(), pps.chroma_bit_depth_entry_minus8);

                Bs16u numComps = pps.monochrome_palette_flag ? 1 : 3;

                pps.palette_predictor_initializers[0] = alloc<Bs16u>(&pps, pps.num_palette_predictor_initializer * numComps);
                BS2_SET_ARR(u(pps.luma_bit_depth_entry_minus8 + 8)
                    , pps.palette_predictor_initializers[0]
                    , pps.num_palette_predictor_initializer, 0);

                if (numComps > 1)
                {
                    pps.palette_predictor_initializers[1] = pps.palette_predictor_initializers[0] + pps.num_palette_predictor_initializer;
                    pps.palette_predictor_initializers[2] = pps.palette_predictor_initializers[1] + pps.num_palette_predictor_initializer;

                    BS2_SET_ARR(u(pps.chroma_bit_depth_entry_minus8 + 8)
                        , pps.palette_predictor_initializers[1]
                        , pps.num_palette_predictor_initializer, 0);
                    BS2_SET_ARR(u(pps.chroma_bit_depth_entry_minus8 + 8)
                        , pps.palette_predictor_initializers[2]
                        , pps.num_palette_predictor_initializer, 0);
                }
            }
        }
    }

    if (pps.extension_xbits)
    {
        pps.ExtBits = parseEXT(pps.ExtData);

        if (pps.ExtData)
            bound(pps.ExtData, &pps);
    }

    if (!TrailingBits())
        throw InvalidSyntax();
}

void Parser::parseMLE(PPS& pps)
{
    (void)pps;

    Bs32s pps_infer_scaling_list_flag
        , num_ref_loc_offsets
        , tmp
        , cm_octant_depth
        , PartNumY
        , CMResLSBits = 10; //Max(0, (10 + BitDepthCmInputY - BitDepthCmOutputY - cm_res_quant_bits -(cm_delta_flc_bits_minus1 + 1)))

    std::function<void(Bs32s, Bs32s, Bs32s, Bs32s, Bs32s)> colour_mapping_octants = [&](
        Bs32s inpDepth
        , Bs32s idxY
        , Bs32s idxCb
        , Bs32s idxCr
        , Bs32s inpLength) -> void
    {
        Bs32s split_octant_flag = 0, i, j, c, m, n, k;

        if (inpDepth < cm_octant_depth)
            BS2_SET(u1(), split_octant_flag);

        if (split_octant_flag)
        {
            for (k = 0; k < 2; k++)
                for (m = 0; m < 2; m++)
                    for (n = 0; n < 2; n++)
                        colour_mapping_octants(
                            inpDepth + 1
                            , idxY + PartNumY * k * inpLength / 2
                            , idxCb + m * inpLength / 2
                            , idxCr + n * inpLength / 2
                            , inpLength / 2);
        }
        else
        {
            for (i = 0; i < PartNumY; i++)
            {
                //idxShiftY = idxY + ((i << (cm_octant_depth - inpDepth))
                for (j = 0; j < 4; j++)
                {
                    BS2_TRACE(tmp = u1(), coded_res_flag[idxShiftY][idxCb][idxCr][j]);
                    if (!tmp)
                        continue;

                    for (c = 0; c < 3; c++)
                    {
                        BS2_TRACE(m = ue(), res_coeff_q[idxShiftY][idxCb][idxCr][j][c]);
                        BS2_TRACE(n = u(CMResLSBits), res_coeff_r[idxShiftY][idxCb][idxCr][j][c]);
                        if (m || n)
                            BS2_TRACE(u1(), res_coeff_s[idxShiftY][idxCb][idxCr][j][c]);
                    }
                }
            }
        }
    };

    BS2_TRACE(u1(), poc_reset_info_present_flag);
    BS2_SET(u1(), pps_infer_scaling_list_flag);

    if (pps_infer_scaling_list_flag)
        BS2_TRACE(u(6), pps_scaling_list_ref_layer_id);

    BS2_SET(ue(), num_ref_loc_offsets);

    for (Bs32s i = 0; i < num_ref_loc_offsets; i++)
    {
        BS2_TRACE(u(6), ref_loc_offset_layer_id[i]);
        BS2_TRACE(tmp = u1(), scaled_ref_layer_offset_present_flag[i]);
        if (tmp)
        {
            BS2_TRACE(se(), scaled_ref_layer_left_offset[ref_loc_offset_layer_id[i]]);
            BS2_TRACE(se(), scaled_ref_layer_top_offset[ref_loc_offset_layer_id[i]]);
            BS2_TRACE(se(), scaled_ref_layer_right_offset[ref_loc_offset_layer_id[i]]);
            BS2_TRACE(se(), scaled_ref_layer_bottom_offset[ref_loc_offset_layer_id[i]]);
        }
        BS2_TRACE(tmp = u1(), ref_region_offset_present_flag[i]);
        if (tmp)
        {
            BS2_TRACE(se(), ref_region_left_offset[ref_loc_offset_layer_id[i]]);
            BS2_TRACE(se(), ref_region_top_offset[ref_loc_offset_layer_id[i]]);
            BS2_TRACE(se(), ref_region_right_offset[ref_loc_offset_layer_id[i]]);
            BS2_TRACE(se(), ref_region_bottom_offset[ref_loc_offset_layer_id[i]]);
        }
        BS2_TRACE(tmp = u1(), resample_phase_set_present_flag[i]);
        if (tmp)
        {
            BS2_TRACE(ue(), phase_hor_luma[ref_loc_offset_layer_id[i]]);
            BS2_TRACE(ue(), phase_ver_luma[ref_loc_offset_layer_id[i]]);
            BS2_TRACE(ue(), phase_hor_chroma_plus8[ref_loc_offset_layer_id[i]]);
            BS2_TRACE(ue(), phase_ver_chroma_plus8[ref_loc_offset_layer_id[i]]);
        }
    }

    BS2_TRACE(tmp = u1(), colour_mapping_enabled_flag[i]);
    if (tmp)
    {
        //colour_mapping_table( )
        BS2_TRACE(tmp = ue(), num_cm_ref_layers_minus1);
        BS2_TRACE_ARR_VF(u(6), cm_ref_layer_id, Bs32u(tmp+1), 0, " %d,");
        BS2_SET(u(2), cm_octant_depth);
        BS2_TRACE(tmp = u(2), cm_y_part_num_log2);
        PartNumY = (1 << tmp);
        BS2_TRACE(tmp = ue(), luma_bit_depth_cm_input_minus8);
        CMResLSBits += (8 + tmp); //BitDepthCmInputY
        BS2_TRACE(ue(), chroma_bit_depth_cm_input_minus8);
        BS2_TRACE(tmp = ue(), luma_bit_depth_cm_output_minus8);
        CMResLSBits -= (8 + tmp); //BitDepthCmOutputY
        BS2_TRACE(ue(), chroma_bit_depth_cm_output_minus8);
        BS2_TRACE(tmp = u(2), cm_res_quant_bits);
        CMResLSBits -= tmp;
        BS2_TRACE(tmp = u(2), cm_delta_flc_bits_minus1);
        CMResLSBits -= (tmp + 1);
        CMResLSBits = BS_MAX(0, CMResLSBits);

        if (cm_octant_depth == 1)
        {
            BS2_TRACE(se(), cm_adapt_threshold_u_delta);
            BS2_TRACE(se(), cm_adapt_threshold_v_delta);
        }

        colour_mapping_octants(0, 0, 0, 0, 1 << cm_octant_depth);
    }
}

void Parser::parse3DE(PPS& pps)
{
    (void)pps;

    Bs32s tmp, i, j, k
        , pps_depth_layers_minus1
        , depthMaxValue
        , dlBD
        , num_val_delta_dlt
        , dlt_val_flags_present_flag
        , max_diff
        , min_diff_minus1 = 0;

    auto delta_dlt = [&](Bs32s)
    {
        BS2_SET(u(dlBD), num_val_delta_dlt);
        if (num_val_delta_dlt > 0)
        {
            max_diff = 0;

            if (num_val_delta_dlt > 1)
                BS2_SET(u(dlBD), max_diff);

            if (num_val_delta_dlt > 2 && max_diff > 0)
                BS2_SET(u(CeilLog2(max_diff + 1)), min_diff_minus1)
            else
                min_diff_minus1 = max_diff - 1;

            BS2_TRACE(u(dlBD), delta_dlt_val0);

            if (max_diff > (min_diff_minus1 + 1))
                for (k = 1; k < num_val_delta_dlt; k++)
                    BS2_TRACE(u(CeilLog2(max_diff - min_diff_minus1)), delta_val_diff_minus_min[k]);
        }
    };

    BS2_TRACE(tmp = u(1), dlts_present_flag);
    if (tmp)
    {
        BS2_SET(u(6), pps_depth_layers_minus1);
        BS2_TRACE(tmp = u(4), pps_bit_depth_for_depth_layers_minus8);
        dlBD = (tmp + 8);
        depthMaxValue = (1 << dlBD) - 1;

        for (i = 0; i <= pps_depth_layers_minus1; i++)
        {
            BS2_TRACE(tmp = u(1), dlt_flag[i]);
            if (tmp)
            {
                dlt_val_flags_present_flag = 0;

                BS2_TRACE(tmp = u(1), dlt_pred_flag[i]);
                if (!tmp)
                    BS2_TRACE(dlt_val_flags_present_flag = u(1), dlt_val_flags_present_flag[i]);

                if (dlt_val_flags_present_flag)
                    for (j = 0; j <= depthMaxValue; j++)
                        BS2_TRACE(u(1), dlt_value_flag[i][j])
                else
                    delta_dlt(i);
            }
        }
    }
}

void Parser::parseSEIPL(APS_SEI& aps)
{
    BS2_SET(u(4), aps.active_video_parameter_set_id);
    BS2_SET(u1(), aps.self_contained_cvs_flag);
    BS2_SET(u1(), aps.no_parameter_set_update_flag);
    BS2_SET(ue(), aps.num_sps_ids_minus1);

    aps.active_seq_parameter_set_id = alloc<Bs8u>(&aps, aps.num_sps_ids_minus1 + 1);

    BS2_SET_ARR(ue(), aps.active_seq_parameter_set_id, aps.num_sps_ids_minus1 + 1, 0);

    if (!m_vps[aps.active_video_parameter_set_id])
        throw NoActiveSet();

    VPS& vps = *m_vps[aps.active_video_parameter_set_id];

    aps.layer_sps_idx = alloc<Bs8u>(&aps, vps.max_layers_minus1 + 1);

    for (Bs32u i = vps.base_layer_internal_flag; i <= vps.max_layers_minus1; i++)
        aps.layer_sps_idx[i] = ue();

    BS2_TRACE_ARR(aps.layer_sps_idx, vps.max_layers_minus1 + 1, 0);

    m_activeSPS = m_sps[aps.layer_sps_idx[0]];
}

void Parser::parseSEIPL(BP_SEI& bp)
{
    BS2_SET(ue(), bp.seq_parameter_set_id);

    if (!m_sps[bp.seq_parameter_set_id])
        throw NoActiveSet();

    SPS& sps = *m_sps[bp.seq_parameter_set_id];
    HRD hrd = {};
    hrd.au_cpb_removal_delay_length_minus1 = 23;

    if (   sps.vui_parameters_present_flag
        && sps.vui->hrd_parameters_present_flag)
        hrd = *sps.vui->hrd;

    if (!hrd.sub_pic_hrd_params_present_flag)
        BS2_SET(u1(), bp.irap_cpb_params_present_flag);

    if (bp.irap_cpb_params_present_flag)
    {
        BS2_SET(u(hrd.au_cpb_removal_delay_length_minus1 + 1), bp.cpb_delay_offset);
        BS2_SET(u(hrd.dpb_output_delay_length_minus1 + 1), bp.dpb_delay_offset);
    }

    BS2_SET(u1(), bp.concatenation_flag);
    BS2_SET(u(hrd.au_cpb_removal_delay_length_minus1 + 1), bp.au_cpb_removal_delay_delta_minus1);

    if (hrd.nal_hrd_parameters_present_flag)
    {
        bp.nal = alloc<BP_SEI::CPB>(&bp, hrd.sl[0].cpb_cnt_minus1 + 1);

        for (Bs32u i = 0; i < (Bs32u)hrd.sl[0].cpb_cnt_minus1 + 1; i++)
        {
            BS2_SET(u(hrd.initial_cpb_removal_delay_length_minus1 + 1),
                bp.nal[i].initial_cpb_removal_delay);
            BS2_SET(u(hrd.initial_cpb_removal_delay_length_minus1 + 1),
                bp.nal[i].initial_cpb_removal_offset);

            if (   hrd.sub_pic_hrd_params_present_flag
                || bp.irap_cpb_params_present_flag)
            {
                BS2_SET(u(hrd.initial_cpb_removal_delay_length_minus1 + 1),
                    bp.nal[i].initial_alt_cpb_removal_delay);
                BS2_SET(u(hrd.initial_cpb_removal_delay_length_minus1 + 1),
                    bp.nal[i].initial_alt_cpb_removal_offset);
            }
        }
    }

    if (hrd.vcl_hrd_parameters_present_flag)
    {
        bp.vcl = alloc<BP_SEI::CPB>(&bp, hrd.sl[0].cpb_cnt_minus1 + 1);

        for (Bs32u i = 0; i < (Bs32u)hrd.sl[0].cpb_cnt_minus1 + 1; i++)
        {
            BS2_SET(u(hrd.initial_cpb_removal_delay_length_minus1 + 1),
                bp.vcl[i].initial_cpb_removal_delay);
            BS2_SET(u(hrd.initial_cpb_removal_delay_length_minus1 + 1),
                bp.vcl[i].initial_cpb_removal_offset);

            if (hrd.sub_pic_hrd_params_present_flag
                || bp.irap_cpb_params_present_flag)
            {
                BS2_SET(u(hrd.initial_cpb_removal_delay_length_minus1 + 1),
                    bp.vcl[i].initial_alt_cpb_removal_delay);
                BS2_SET(u(hrd.initial_cpb_removal_delay_length_minus1 + 1),
                    bp.vcl[i].initial_alt_cpb_removal_offset);
            }
        }
    }

    try
    {
        Bs32u o = (8 - GetBitOffset());
        Bs32u B = u(o);

        if (B != (1u << o))
            BS2_SET((B >> (o - 1)), bp.use_alt_cpb_params_flag);
    }
    catch (EndOfBuffer&)
    {
        //ignore
    }
}

void Parser::parseSEIPL(PT_SEI& pt)
{
    if (!m_activeSPS)
        throw NoActiveSet();

    SPS& sps = *m_activeSPS;

    if (!sps.vui_parameters_present_flag)
        return;

    VUI& vui = *sps.vui;

    if (vui.frame_field_info_present_flag)
    {
        BS2_SETM(u(4), pt.pic_struct, PicStructTraceMap);
        BS2_SETM(u(2), pt.source_scan_type, ScanTypeTraceMap);
        BS2_SET(u1(), pt.duplicate_flag);
    }

    if (   vui.hrd_parameters_present_flag
        && (vui.hrd->nal_hrd_parameters_present_flag
         || vui.hrd->vcl_hrd_parameters_present_flag))
    {
        HRD& hrd = *vui.hrd;

        BS2_SET(u(hrd.au_cpb_removal_delay_length_minus1 + 1), pt.au_cpb_removal_delay_minus1);
        BS2_SET(u(hrd.dpb_output_delay_length_minus1 + 1), pt.pic_dpb_output_delay);

        if (hrd.sub_pic_hrd_params_present_flag)
            BS2_SET(u(hrd.dpb_output_delay_du_length_minus1 + 1), pt.pic_dpb_output_du_delay);

        if (   hrd.sub_pic_hrd_params_present_flag
            && hrd.sub_pic_cpb_params_in_pic_timing_sei_flag)
        {
            BS2_SET(ue(), pt.num_decoding_units_minus1);
            BS2_SET(u1(), pt.du_common_cpb_removal_delay_flag);

            if (pt.du_common_cpb_removal_delay_flag)
                BS2_SET(u(hrd.du_cpb_removal_delay_increment_length_minus1 + 1),
                    pt.du_common_cpb_removal_delay_increment_minus1);

            pt.num_nalus_in_du_minus1 = alloc<Bs32u>(&pt, pt.num_decoding_units_minus1 + 1);

            if (!pt.du_common_cpb_removal_delay_flag)
                pt.du_cpb_removal_delay_increment_minus1 =
                    alloc<Bs32u>(&pt, pt.num_decoding_units_minus1);

            for (Bs32u i = 0; i < pt.num_decoding_units_minus1; i++)
            {
                pt.num_nalus_in_du_minus1[i] = ue();

                if (!pt.du_common_cpb_removal_delay_flag)
                    pt.du_cpb_removal_delay_increment_minus1[i] =
                        u(hrd.du_cpb_removal_delay_increment_length_minus1 + 1);
            }

            pt.num_nalus_in_du_minus1[pt.num_decoding_units_minus1] = ue();

            BS2_TRACE_ARR(pt.num_nalus_in_du_minus1, pt.num_decoding_units_minus1 + 1, 0);

            if (!pt.du_common_cpb_removal_delay_flag)
                BS2_TRACE_ARR(pt.du_cpb_removal_delay_increment_minus1,
                    pt.num_decoding_units_minus1, 0);
        }
    }
}

void Parser::parseSEIPL(RP_SEI& rp)
{
    BS2_SET(se(), rp.recovery_poc_cnt);
    BS2_SET(u1(), rp.exact_match_flag);
    BS2_SET(u1(), rp.broken_link_flag);
}

void Parser::parseSEIPL(SEI& sei)
{
    TLAuto tl(*this, TRACE_SEI);
    StateSwitcher st(*this);

    SetEmulation(false);
    Reset(sei.rawData, sei.payloadSize);

    if (!m_postponedSEI.empty() && m_postponedSEI.front() == &sei)
    {
        BS2_TRACE_STR("----------------------------------");
        BS2_SETM(sei.payloadType, sei.payloadType, SEITypeTraceMap());
    }

    try
    {
        switch (sei.payloadType)
        {
        case SEI_ACTIVE_PARAMETER_SETS:
            if (!sei.aps)
                sei.aps = alloc<APS_SEI>(&sei);
            parseSEIPL(*sei.aps);
            break;
        case SEI_BUFFERING_PERIOD:
            if (!sei.bp)
                sei.bp = alloc<BP_SEI>(&sei);
            parseSEIPL(*sei.bp);
            break;
        case SEI_PICTURE_TIMING:
            if (!sei.pt)
                sei.pt = alloc<PT_SEI>(&sei);
            parseSEIPL(*sei.pt);
            break;
        case SEI_RECOVERY_POINT:
            if (!sei.rp)
                sei.rp = alloc<RP_SEI>(&sei);
            parseSEIPL(*sei.rp);
            break;
        default:
            break;
        }
    }
    catch (EndOfBuffer&)
    {
        throw InvalidSyntax();
    }
}

void Parser::parseSEI(SEI& first)
{
    TLAuto tl(*this, TRACE_SEI);

    SEI* next = &first;

    for (;;)
    {
        Bs8u lastByte;
        SEI& sei = *next;

        do
        {
            lastByte = u8();
            sei.payloadType += lastByte;
        } while (lastByte == 0xff);

        BS2_SETM(sei.payloadType, sei.payloadType, SEITypeTraceMap());

        do
        {
            lastByte = u8();
            sei.payloadSize += lastByte;
        } while (lastByte == 0xff);

        BS2_TRACE(sei.payloadSize, sei.payloadSize);

        sei.rawData = alloc<Bs8u>(&sei, sei.payloadSize);

        BS2_SET_ARR_F(GetByte(), sei.rawData, sei.payloadSize,
            16 * (sei.payloadSize > 16), "%02X ");

        if (GetBitOffset() && !TrailingBits())
            throw InvalidSyntax();

        try
        {
            parseSEIPL(sei);
        }
        catch (NoActiveSet&)
        {
            m_postponedSEI.push_back(&sei);
        }

        if (!more_rbsp_data())
            break;

        next = sei.next = alloc<SEI>(&sei);
    }

    if (!TrailingBits())
        throw InvalidSyntax();

}

void Parser::parseSSH(NALU& nalu)
{
    TLAuto tl(*this, TRACE_SSH);
    Slice& slice = *nalu.slice;
    Bs32u NUT = nalu.nal_unit_type;
    Bs32u i, sz, NumPicTotalCurr;

    BS2_SET(u1(), slice.first_slice_segment_in_pic_flag);

    if (NUT >= BLA_W_LP && NUT <= RSV_IRAP_VCL23)
        BS2_SET(u1(), slice.no_output_of_prior_pics_flag);

    BS2_SET(ue(), slice.pic_parameter_set_id);

    slice.pps = m_pps[slice.pic_parameter_set_id];

    if (!slice.pps)
        throw NoActiveSet();

    slice.sps = m_activeSPS = m_sps[slice.pps->seq_parameter_set_id];

    if (!slice.sps)
        throw NoActiveSet();

    SPS& sps = *slice.sps;
    PPS& pps = *slice.pps;

    bound(&sps, &slice);
    bound(&pps, &slice);

    if (!slice.first_slice_segment_in_pic_flag)
    {
        if (pps.dependent_slice_segments_enabled_flag)
            BS2_SET(u1(), slice.dependent_slice_segment_flag);

        Bs16u MaxCU = (1 << (sps.log2_min_luma_coding_block_size_minus3 + 3 + sps.log2_diff_max_min_luma_coding_block_size));
        Bs32u PicSizeInCtbsY = CeilDiv(sps.pic_width_in_luma_samples, MaxCU)
            * CeilDiv(sps.pic_height_in_luma_samples, MaxCU);

        BS2_SET(u(CeilLog2(PicSizeInCtbsY)), slice.slice_segment_address);
    }

    if (slice.dependent_slice_segment_flag)
        goto l_ep_offsets;

    if (pps.num_extra_slice_header_bits)
    {
        slice.reserved_flags = u(pps.num_extra_slice_header_bits) << (8 - pps.num_extra_slice_header_bits);
        Bs8u flags = (Bs8u)slice.reserved_flags;
        BS2_TRACE_BIN(&flags, 0, pps.num_extra_slice_header_bits, slice.reserved_flags);
    }

    BS2_SETM(ue(), slice.type, SliceTypeTraceMap);

    if (pps.output_flag_present_flag)
        BS2_SET(u1(), slice.pic_output_flag);

    if (sps.separate_colour_plane_flag)
        BS2_SET(u(2), slice.colour_plane_id);

    if (NUT == IDR_W_RADL || NUT == IDR_N_LP)
        goto l_after_dpb;

    BS2_SET(u(sps.log2_max_pic_order_cnt_lsb_minus4 + 4), slice.pic_order_cnt_lsb);
    BS2_SET(u1(), slice.short_term_ref_pic_set_sps_flag);

    if (!slice.short_term_ref_pic_set_sps_flag)
    {
        ParseSTRPS(sps.strps, sps.num_short_term_ref_pic_sets, sps.num_short_term_ref_pic_sets);
        slice.strps = sps.strps[sps.num_short_term_ref_pic_sets];
    }
    else if (sps.num_short_term_ref_pic_sets > 1)
    {
        BS2_SET(u(CeilLog2(sps.num_short_term_ref_pic_sets)), slice.short_term_ref_pic_set_idx)
        slice.strps = sps.strps[slice.short_term_ref_pic_set_idx];
    }
    else if (sps.num_short_term_ref_pic_sets == 1)
    {
        slice.strps = sps.strps[0];
    }

    if (!sps.long_term_ref_pics_present_flag)
        goto l_after_lt;

    if (sps.num_long_term_ref_pics)
        BS2_SET(ue(), slice.num_long_term_sps);

    BS2_SET(ue(), slice.num_long_term_pics);

    sz = slice.num_long_term_sps + slice.num_long_term_pics;
    slice.poc_lsb_lt = alloc<Bs16u>(&slice, sz);
    slice.DeltaPocMsbCycleLt = alloc<Bs16u>(&slice, sz);

    for (i = 0; i < sz; i++)
    {
        if (i < slice.num_long_term_sps)
        {
            Bs32u lt_idx_sps =
                (sps.num_long_term_ref_pics > 0) ? u(CeilLog2(sps.num_long_term_ref_pics)) : 0;

            slice.poc_lsb_lt[i] = sps.lt_ref_pic_poc_lsb[lt_idx_sps];
            SetBit(slice.used_by_curr_pic_lt_flags, i,
                BS_HEVC2::GetBit(sps.used_by_curr_pic_lt_flags, lt_idx_sps));
        }
        else
        {
            slice.poc_lsb_lt[i] = u(sps.log2_max_pic_order_cnt_lsb_minus4 + 4);
            SetBit(slice.used_by_curr_pic_lt_flags, i, !!u1());
        }

        if (u1()) // delta_poc_msb_present_flag
            slice.DeltaPocMsbCycleLt[i] = ue();

        if (i && i != slice.num_long_term_sps)
            slice.DeltaPocMsbCycleLt[i] += slice.DeltaPocMsbCycleLt[i - 1];
    }

    if (sz)
    {
        BS2_TRACE_BIN(&slice.used_by_curr_pic_lt_flags, 0, sz, slice.used_by_curr_pic_lt_flags);
        BS2_TRACE_ARR(slice.poc_lsb_lt, sz, 0);
        BS2_TRACE_ARR(slice.DeltaPocMsbCycleLt, sz, 0);
    }

l_after_lt:
    if (sps.temporal_mvp_enabled_flag)
        BS2_SET(u1(), slice.temporal_mvp_enabled_flag);

l_after_dpb:

    slice.DPB = alloc<RefPic>(&slice, slice.strps.NumDeltaPocs + slice.num_long_term_sps + slice.num_long_term_pics);

    if (sps.sample_adaptive_offset_enabled_flag)
    {
        BS2_SET(u1(), slice.sao_luma_flag);

        if (sps.chroma_format_idc != 0)
            BS2_SET(u1(), slice.sao_chroma_flag);
    }

    if (slice.type == I)
        goto l_qp;

    BS2_SET(u1(), slice.num_ref_idx_active_override_flag);

    if (slice.num_ref_idx_active_override_flag)
    {
        BS2_SET(ue() + 1, slice.num_ref_idx_l0_active);

        if (slice.type == SLICE_TYPE::B)
            BS2_SET(ue() + 1, slice.num_ref_idx_l1_active);
    }
    else
    {
        slice.num_ref_idx_l0_active = 1 + pps.num_ref_idx_l0_default_active_minus1;

        if (slice.type == SLICE_TYPE::B)
            slice.num_ref_idx_l1_active = (1 + pps.num_ref_idx_l1_default_active_minus1);
    }

    slice.L0 = alloc<RefPic>(&slice, slice.num_ref_idx_l0_active);
    slice.L1 = alloc<RefPic>(&slice, slice.num_ref_idx_l1_active);

    NumPicTotalCurr = CntBits(slice.strps.UsedByCurrPicFlags) + CntBits(slice.used_by_curr_pic_lt_flags);

    if (pps.lists_modification_present_flag && NumPicTotalCurr > 1)
    {
        sz = CeilLog2(NumPicTotalCurr);

        BS2_SET(u1(), slice.ref_pic_list_modification_flag_l0);

        if (slice.ref_pic_list_modification_flag_l0)
            BS2_SET_ARR(u(sz), slice.list_entry_lx[0], slice.num_ref_idx_l0_active, 0);

        if (slice.type == SLICE_TYPE::B)
        {
            BS2_SET(u1(), slice.ref_pic_list_modification_flag_l1);

            if (slice.ref_pic_list_modification_flag_l1)
                BS2_SET_ARR(u(sz), slice.list_entry_lx[1], slice.num_ref_idx_l1_active, 0);
        }
    }

    if (slice.type == SLICE_TYPE::B)
        BS2_SET(u1(), slice.mvd_l1_zero_flag);

    if (pps.cabac_init_present_flag)
        BS2_SET(u1(), slice.cabac_init_flag);

    if (slice.temporal_mvp_enabled_flag)
    {
        if (slice.type == SLICE_TYPE::B)
            BS2_SET(u1(), slice.collocated_from_l0_flag)
        else
            slice.collocated_from_l0_flag = 1;

        if (    (slice.collocated_from_l0_flag && slice.num_ref_idx_l0_active > 1)
            || (!slice.collocated_from_l0_flag && slice.num_ref_idx_l1_active > 1))
            BS2_SET(ue(), slice.collocated_ref_idx);
    }

#define Clip3(_min, _max, _x) std::min(std::max(_min, _x), _max)

    if (   (pps.weighted_pred_flag && slice.type == SLICE_TYPE::P)
        || (pps.weighted_bipred_flag && slice.type == SLICE_TYPE::B))
    {
        Bs16s WpOffsetHalfRangeC = 1 << (sps.high_precision_offsets_enabled_flag ? (sps.bit_depth_chroma_minus8 + 8 - 1) : 7);
        Bs16u dim[] = {1, 16, 3, 2};
        slice.pwt = alloc<PWT>(&slice, 1 + (slice.type == SLICE_TYPE::B));

        BS2_SET(ue(), slice.luma_log2_weight_denom);
        if (sps.chroma_format_idc != 0)
            BS2_SET(slice.luma_log2_weight_denom + se(), slice.chroma_log2_weight_denom);

        if (pps.curr_pic_ref_enabled_flag)
        {
            //have to decode RPL right now
            NoRaslOutputFlag = m_bNewSequence || isIDR(nalu) || isBLA(nalu);
            MaxPicOrderCntLsb = (1 << (sps.log2_max_pic_order_cnt_lsb_minus4 + 4));

            decodePOC(nalu);

            if (   m_bNewSequence || !m_cSlice || (m_cSlice->POC != slice.POC)
                || (m_cSlice->POC == slice.POC && CtuInRs[slice.slice_segment_address]))
                m_DPB.swap(m_DPBafter);

            m_prevSlicePOC = slice.POC;

            decodeRPL(nalu);

            m_bRPLDecoded = true;
        }

        for (Bs32u l = 0; l < 2; l++)
        {
            auto& LX = l ? slice.L1 : slice.L0;
            Bs32u nCPR = 0;

            sz = l ? slice.num_ref_idx_l1_active : slice.num_ref_idx_l0_active;

            if (pps.curr_pic_ref_enabled_flag)
                for (i = 0; i < sz; i++)
                    nCPR += (LX[i].POC == slice.POC);

            Bs16u lumaw   = u(sz - nCPR) << nCPR;
            Bs16u chromaw = 0;

            if (sps.chroma_format_idc != 0)
                chromaw = u(sz - nCPR) << nCPR;

            for (i = 0; i < sz; i++)
            {
                auto& entry = slice.pwt[l][i];

                entry[0][0] = (1 << slice.luma_log2_weight_denom);
                entry[1][0] = (1 << slice.chroma_log2_weight_denom);
                entry[2][0] = (1 << slice.chroma_log2_weight_denom);

                if (pps.curr_pic_ref_enabled_flag && LX[i].POC == slice.POC)
                {
                    lumaw >>= 1;
                    chromaw >>= 1;
                    continue;
                }

                if (lumaw & (1 << (sz - i - 1)))
                {
                    entry[0][0] += se();
                    entry[0][1] = se();
                }

                if (chromaw & (1 << (sz - i - 1)))
                {
                    entry[1][0] += se();
                    entry[1][1] = Clip3(-WpOffsetHalfRangeC, WpOffsetHalfRangeC - 1,
                        (WpOffsetHalfRangeC + se() - ((WpOffsetHalfRangeC * entry[1][0]) >> slice.chroma_log2_weight_denom)));
                    entry[2][0] += se();
                    entry[2][1] = Clip3(-WpOffsetHalfRangeC, WpOffsetHalfRangeC - 1,
                        (WpOffsetHalfRangeC + se() - ((WpOffsetHalfRangeC * entry[2][0]) >> slice.chroma_log2_weight_denom)));
                }
            }
        }

        dim[1] = slice.num_ref_idx_l0_active;
        BS2_TRACE_MDARR(Bs16s, slice.pwt[0], dim, 1, 0, "%4i ");

        if (slice.type == SLICE_TYPE::B)
        {
            dim[1] = slice.num_ref_idx_l1_active;
            BS2_TRACE_MDARR(Bs16s, slice.pwt[1], dim, 1, 0, "%4i ");
        }
    }

    BS2_SET(5 - ue(), slice.MaxNumMergeCand);

    if (sps.motion_vector_resolution_control_idc == 2)
        BS2_SET(u1(), slice.use_integer_mv_flag)
    else
        slice.use_integer_mv_flag = sps.motion_vector_resolution_control_idc;

l_qp:
    BS2_SET(se(), slice.qp_delta);

    if (pps.slice_chroma_qp_offsets_present_flag)
    {
        BS2_SET(se(), slice.cb_qp_offset);
        BS2_SET(se(), slice.cr_qp_offset);
    }

    if (pps.slice_act_qp_offsets_present_flag)
    {
        BS2_SET(se(), slice.act_y_qp_offset);
        BS2_SET(se(), slice.act_cb_qp_offset);
        BS2_SET(se(), slice.act_cr_qp_offset);
    }

    if (pps.chroma_qp_offset_list_enabled_flag)
        BS2_SET(u1(), slice.cu_chroma_qp_offset_enabled_flag);

    if (pps.deblocking_filter_override_enabled_flag)
        BS2_SET(u1(), slice.deblocking_filter_override_flag);

    if (slice.deblocking_filter_override_flag)
    {
        BS2_SET(u1(), slice.deblocking_filter_disabled_flag);

        if (!slice.deblocking_filter_disabled_flag)
        {
            BS2_SET(se(), slice.beta_offset_div2);
            BS2_SET(se(), slice.tc_offset_div2);
        }
    }
    else
        slice.deblocking_filter_disabled_flag = pps.deblocking_filter_disabled_flag;

    if (pps.loop_filter_across_slices_enabled_flag
        &&  (slice.sao_luma_flag
          || slice.sao_chroma_flag
          || !slice.deblocking_filter_disabled_flag))
        BS2_SET(u1(), slice.loop_filter_across_slices_enabled_flag);

l_ep_offsets:
    if (pps.tiles_enabled_flag || pps.entropy_coding_sync_enabled_flag)
    {
        BS2_SET(ue(), slice.num_entry_point_offsets);

        if (slice.num_entry_point_offsets)
        {
            slice.entry_point_offset_minus1 = alloc<Bs32u>(&slice, slice.num_entry_point_offsets);

            BS2_SET(ue(), slice.offset_len_minus1);
            BS2_SET_ARR(
                u(slice.offset_len_minus1 + 1),
                slice.entry_point_offset_minus1,
                slice.num_entry_point_offsets, 0);
        }
    }

    if (pps.slice_segment_header_extension_present_flag)
    {
        BS2_SET(ue() * 8, slice.ExtBits);

        if (slice.ExtBits)
        {
            slice.ExtData = alloc<Bs8u>(&slice, slice.ExtBits / 8);

            BS2_SET_ARR_F(u8(), slice.ExtData, slice.ExtBits / 8, 16, "%02X ");
        }
    }

    if (!TrailingBits())
        throw InvalidSyntax();
}

void Parser::parseRBSP(NALU& nalu)
{
    switch (nalu.nal_unit_type)
    {
    case AUD_NUT:
        nalu.aud = alloc<AUD>(&nalu);
        parseAUD(*nalu.aud);
        break;
    case VPS_NUT:
        nalu.vps = alloc<VPS>(&nalu);
        parseVPS(*nalu.vps);
        lock(nalu.vps);
        unlock(m_vps[nalu.vps->video_parameter_set_id]);
        m_vps[nalu.vps->video_parameter_set_id] = nalu.vps;
        break;
    case SPS_NUT:
        nalu.sps = alloc<SPS>(&nalu);
        parseSPS(*nalu.sps);
        lock(nalu.sps);
        unlock(m_sps[nalu.sps->seq_parameter_set_id]);
        m_sps[nalu.sps->seq_parameter_set_id] = nalu.sps;
        break;
    case PPS_NUT:
        nalu.pps = alloc<PPS>(&nalu);
        parsePPS(*nalu.pps);
        lock(nalu.pps);
        unlock(m_pps[nalu.pps->pic_parameter_set_id]);
        m_pps[nalu.pps->pic_parameter_set_id] = nalu.pps;
        break;
    case PREFIX_SEI_NUT:
    case SUFFIX_SEI_NUT:
        nalu.sei = alloc<SEI>(&nalu);
        parseSEI(*nalu.sei);
        break;
    case TRAIL_N:
    case TRAIL_R:
    case TSA_N:
    case TSA_R:
    case STSA_N:
    case STSA_R:
    case RADL_N:
    case RADL_R:
    case RASL_N:
    case RASL_R:
    case BLA_W_LP:
    case BLA_W_RADL:
    case BLA_N_LP:
    case IDR_W_RADL:
    case IDR_N_LP:
    case CRA_NUT:
        nalu.slice = alloc<Slice>(&nalu);
        parseSSH(nalu);
        {
            auto& slice = *nalu.slice;
            auto prevSlice = m_cSlice;

            Lock(&slice);
            decodeSSH(nalu, m_bNewSequence);
            Unlock(prevSlice);

            if (!slice.pps->CtbAddrRsToTs && !CtbAddrRsToTs.empty())
            {
                slice.pps->CtbAddrRsToTs = alloc<Bs16u>(slice.pps, (Bs32u)CtbAddrRsToTs.size());
                memmove(slice.pps->CtbAddrRsToTs, &CtbAddrRsToTs[0], CtbAddrRsToTs.size() * sizeof(CtbAddrRsToTs[0]));
            }

            TLStart(TRACE_REF);

            BS2_TRACE(slice.POC, slice.POC);

//ignore raw value in printf() params in BS2_SET_ARR_M(), print only mapped one
#if defined(__GNUC__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wformat-extra-args"
#elif defined(__clang__)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wformat-extra-args"
#else
  #pragma warning(disable:4474)
#endif
            BS2_SET_ARR_M(slice.DPB[_i], slice.DPB,
                slice.strps.NumDeltaPocs + slice.num_long_term_sps + slice.num_long_term_pics, 1, "%s ", RPLTraceMap());

            if (slice.num_ref_idx_l0_active)
                BS2_SET_ARR_M(slice.L0[_i], slice.L0, slice.num_ref_idx_l0_active, 1, "%s ", RPLTraceMap());
            if (slice.num_ref_idx_l1_active)
                BS2_SET_ARR_M(slice.L1[_i], slice.L1, slice.num_ref_idx_l1_active, 1, "%s ", RPLTraceMap());
#if defined(__GNUC__)
  #pragma GCC diagnostic pop
#elif defined(__clang__)
  #pragma clang diagnostic pop
#else
  #pragma warning(default:4474)
#endif

            TLEnd();
        }
        m_bNewSequence = false;
        break;
    case EOS_NUT:
    case EOB_NUT:
        m_bNewSequence = true;
        break;
    default:
        break;
    }
}
