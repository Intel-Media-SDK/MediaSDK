// Copyright (c) 2017-2018 Intel Corporation
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
#ifdef MFX_ENABLE_H265_VIDEO_DECODE

#include "umc_h265_bitstream_headers.h"
#include "umc_h265_headers.h"

namespace UMC_HEVC_DECODER
{

// Parse SEI message
int32_t H265HeadersBitstream::ParseSEI(const HeaderSet<H265SeqParamSet> & sps, int32_t current_sps, H265SEIPayLoad *spl)
{
    return sei_message(sps,current_sps,spl);
}

// Parse SEI message
int32_t H265HeadersBitstream::sei_message(const HeaderSet<H265SeqParamSet> & sps, int32_t current_sps, H265SEIPayLoad *spl)
{
    uint32_t code;
    int32_t payloadType = 0;

    CheckBitsLeft(8);
    PeakNextBits(m_pbs, m_bitOffset, 8, code);
    while (code  ==  0xFF)
    {
        /* fixed-pattern bit string using 8 bits written equal to 0xFF */
        CheckBitsLeft(8);
        GetNBits(m_pbs, m_bitOffset, 8, code);
        payloadType += 255;
        CheckBitsLeft(8);
        PeakNextBits(m_pbs, m_bitOffset, 8, code);
    }

    int32_t last_payload_type_byte;    //uint32_t integer using 8 bits
    CheckBitsLeft(8);
    GetNBits(m_pbs, m_bitOffset, 8, last_payload_type_byte);

    payloadType += last_payload_type_byte;

    int32_t payloadSize = 0;

    PeakNextBits(m_pbs, m_bitOffset, 8, code);
    while( code  ==  0xFF )
    {
        /* fixed-pattern bit string using 8 bits written equal to 0xFF */
        CheckBitsLeft(8);
        GetNBits(m_pbs, m_bitOffset, 8, code);
        payloadSize += 255;
        CheckBitsLeft(8);
        PeakNextBits(m_pbs, m_bitOffset, 8, code);
    }

    int32_t last_payload_size_byte;    //uint32_t integer using 8 bits

    CheckBitsLeft(8);
    GetNBits(m_pbs, m_bitOffset, 8, last_payload_size_byte);
    payloadSize += last_payload_size_byte;
    spl->Reset();
    spl->payLoadSize = payloadSize;

    if (payloadType < 0 || payloadType > SEI_RESERVED)
        payloadType = SEI_RESERVED;

    spl->payLoadType = (SEI_TYPE)payloadType;

    if (spl->payLoadSize > BytesLeft())
    {
        throw h265_exception(UMC::UMC_ERR_INVALID_STREAM);
    }

    uint32_t * pbs;
    uint32_t bitOffsetU;
    int32_t bitOffset;

    GetState(&pbs, &bitOffsetU);
    bitOffset = bitOffsetU;

    int32_t ret = sei_payload(sps, current_sps, spl);

    for (uint32_t i = 0; i < spl->payLoadSize; i++)
    {
        CheckBitsLeft(8);
        SkipNBits(pbs, bitOffset, 8);
    }

    SetState(pbs, bitOffset);

    return ret;
}

// Parse SEI payload data
int32_t H265HeadersBitstream::sei_payload(const HeaderSet<H265SeqParamSet> & sps, int32_t current_sps, H265SEIPayLoad *spl)
{
    uint32_t payloadType = spl->payLoadType;
    switch(payloadType)
    {
    case SEI_PIC_TIMING_TYPE:
        return pic_timing(sps,current_sps,spl);
    case SEI_RECOVERY_POINT_TYPE:
        return recovery_point(sps,current_sps,spl);
    case SEI_MASTERING_DISPLAY_COLOUR_VOLUME:
        return mastering_display_colour_volume(sps, current_sps, spl);
    case SEI_CONTENT_LIGHT_LEVEL_INFO:
        return content_light_level_info(sps, current_sps, spl);
    }

    return reserved_sei_message(sps,current_sps,spl);
}

// Parse pic timing SEI data
int32_t H265HeadersBitstream::pic_timing(const HeaderSet<H265SeqParamSet> & sps, int32_t current_sps, H265SEIPayLoad * spl)
{
    const H265SeqParamSet *csps = sps.GetHeader(current_sps);

    if (!csps)
        throw h265_exception(UMC::UMC_ERR_INVALID_STREAM);

    if (csps->frame_field_info_present_flag)
    {
        spl->SEI_messages.pic_timing.pic_struct = (DisplayPictureStruct_H265)GetBits(4);
        GetBits(2); // source_scan_type
        GetBits(1); // duplicate_flag
    }

    if (csps->m_hrdParameters.nal_hrd_parameters_present_flag || csps->m_hrdParameters.vcl_hrd_parameters_present_flag)
    {
        GetBits(csps->m_hrdParameters.au_cpb_removal_delay_length); // au_cpb_removal_delay_minus1
        spl->SEI_messages.pic_timing.pic_dpb_output_delay = GetBits(csps->m_hrdParameters.dpb_output_delay_length);

        if (csps->m_hrdParameters.sub_pic_hrd_params_present_flag)
        {
            GetBits(csps->m_hrdParameters.dpb_output_delay_du_length); //pic_dpb_output_du_delay
        }

        if (csps->m_hrdParameters.sub_pic_hrd_params_present_flag && csps->m_hrdParameters.sub_pic_cpb_params_in_pic_timing_sei_flag)
        {
            uint32_t num_decoding_units_minus1 = GetVLCElementU();

            if (num_decoding_units_minus1 > csps->WidthInCU * csps->HeightInCU)
                throw h265_exception(UMC::UMC_ERR_INVALID_STREAM);

            uint8_t du_common_cpb_removal_delay_flag = (uint8_t)GetBits(1);
            if (du_common_cpb_removal_delay_flag)
            {
                GetBits(csps->m_hrdParameters.du_cpb_removal_delay_increment_length); //du_common_cpb_removal_delay_increment_minus1
            }

            for (uint32_t i = 0; i <= num_decoding_units_minus1; i++)
            {
                GetVLCElementU(); // num_nalus_in_du_minus1
                if (!du_common_cpb_removal_delay_flag && i < num_decoding_units_minus1)
                {
                    GetBits(csps->m_hrdParameters.du_cpb_removal_delay_increment_length); // du_cpb_removal_delay_increment_minus1
                }
            }
        }
    }

    AlignPointerRight();

    return current_sps;
}

// Parse recovery point SEI data
int32_t H265HeadersBitstream::recovery_point(const HeaderSet<H265SeqParamSet> & , int32_t current_sps, H265SEIPayLoad *spl)
{
    H265SEIPayLoad::SEIMessages::RecoveryPoint * recPoint = &(spl->SEI_messages.recovery_point);

    recPoint->recovery_poc_cnt = GetVLCElementS();

    recPoint->exact_match_flag = (uint8_t)Get1Bit();
    recPoint->broken_link_flag = (uint8_t)Get1Bit();

    return current_sps;
}

// Parse mastering_display_colour_volume
int32_t H265HeadersBitstream::mastering_display_colour_volume(const HeaderSet<H265SeqParamSet>&, int32_t current_sps, H265SEIPayLoad* spl)
{
    H265SEIPayLoad::SEIMessages::MasteringDisplay* masterDisplay = &(spl->SEI_messages.mastering_display);
    int i = 0;
    // Mastering primaries
    for (; i < 3; ++i)
    {
        masterDisplay->display_primaries[i][0] = (uint16_t)GetBits(16);
        masterDisplay->display_primaries[i][1] = (uint16_t)GetBits(16);
    }
    // White point(x, y)
    masterDisplay->white_point[0] = (uint16_t)GetBits(16);
    masterDisplay->white_point[1] = (uint16_t)GetBits(16);

    // Max and min luminance of mastering display
    masterDisplay->max_luminance = (uint32_t)GetBits(32);
    masterDisplay->min_luminance = (uint32_t)GetBits(32);

    return current_sps;
}

// Parse mastering_display_colour_volume
int32_t H265HeadersBitstream::content_light_level_info(const HeaderSet<H265SeqParamSet>&, int32_t current_sps, H265SEIPayLoad* spl)
{
    H265SEIPayLoad::SEIMessages::ContentLightLevelInfo * contentLight = &(spl->SEI_messages.content_light_level_info);

    // Max and average light levels
    contentLight->max_content_light_level = (uint16_t)GetBits(16);
    contentLight->max_pic_average_light_level = (uint16_t)GetBits(16);

    return current_sps;
}

// Skip unrecognized SEI message payload
int32_t H265HeadersBitstream::reserved_sei_message(const HeaderSet<H265SeqParamSet> & , int32_t current_sps, H265SEIPayLoad *spl)
{
    for(uint32_t i = 0; i < spl->payLoadSize; i++)
    {
        CheckBitsLeft(8);
        SkipNBits(m_pbs, m_bitOffset, 8);
    }

    AlignPointerRight();
    return current_sps;
}

}//namespace UMC_HEVC_DECODER
#endif // MFX_ENABLE_H265_VIDEO_DECODE
