// Copyright (c) 2018-2019 Intel Corporation
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
#include "mfx_utils.h"
#if defined (MFX_ENABLE_MPEG2_VIDEO_DECODE)

#include "umc_mpeg2_defs.h"
#include "umc_mpeg2_slice.h"
#include "umc_mpeg2_bitstream.h"

namespace UMC_MPEG2_DECODER
{
    // Parse sequence header
    void MPEG2HeadersBitstream::GetSequenceHeader(MPEG2SequenceHeader & seq)
    {
        // 6.2.2.1 Sequence header
        seq.horizontal_size_value = GetBits(12);
        if (!seq.horizontal_size_value)
            throw mpeg2_exception(UMC::UMC_ERR_INVALID_STREAM);

        seq.vertical_size_value = GetBits(12);
        if (!seq.vertical_size_value)
            throw mpeg2_exception(UMC::UMC_ERR_INVALID_STREAM);

        seq.aspect_ratio_information = GetBits(4);
        // seq.aspect_ratio_information should be in [1;4] range but ignore this for corrupted streams

        seq.frame_rate_code = GetBits(4);
        seq.bit_rate_value = GetBits(18);

        Seek(1); // marker_bit shall be '1' to prevent emulation of start codes. But ignore this for corrupted streams.

        seq.vbv_buffer_size_value = GetBits(10);
        seq.constrained_parameters_flag = GetBits(1);
        seq.load_intra_quantiser_matrix = GetBits(1);
        if (seq.load_intra_quantiser_matrix)
        {
            for (uint32_t i = 0; i < 64; ++i)
                seq.intra_quantiser_matrix[i] = GetBits(8);
        }
        seq.load_non_intra_quantiser_matrix = GetBits(1);
        if (seq.load_non_intra_quantiser_matrix)
        {
            for (uint32_t i = 0; i < 64; ++i)
                seq.non_intra_quantiser_matrix[i] = GetBits(8);
        }
    }

    // Parse sequence extension
    void MPEG2HeadersBitstream::GetSequenceExtension(MPEG2SequenceExtension & seqExt)
    {
        // 6.2.2.3 Sequence extension
        seqExt.profile_and_level_indication = GetBits(8);
        seqExt.progressive_sequence = GetBits(1);
        seqExt.chroma_format = GetBits(2);
        if (0 == seqExt.chroma_format)
            throw mpeg2_exception(UMC::UMC_ERR_INVALID_STREAM);

        seqExt.horizontal_size_extension = GetBits(2);
        seqExt.vertical_size_extension = GetBits(2);
        seqExt.bit_rate_extension = GetBits(12);

        Seek(1); // marker_bit shall be '1' to prevent emulation of start codes. But ignore this for corrupted streams.

        seqExt.vbv_buffer_size_extension = GetBits(8);
        seqExt.low_delay = GetBits(1);
        seqExt.frame_rate_extension_n = GetBits(2);
        seqExt.frame_rate_extension_d = GetBits(5);
    }

    // Parse sequence display extension
    void MPEG2HeadersBitstream::GetSequenceDisplayExtension(MPEG2SequenceDisplayExtension & dispExt)
    {
        // 6.2.2.4 Sequence display extension
        dispExt.video_format = GetBits(3);
        dispExt.colour_description = GetBits(1);
        if (dispExt.colour_description)
        {
            dispExt.colour_primaries = GetBits(8);
            dispExt.transfer_characteristics = GetBits(8);
            dispExt.matrix_coefficients = GetBits(8);
        }

        dispExt.display_horizontal_size = GetBits(14);

        uint8_t marker_bit = GetBits(1);
        if (0 == marker_bit) // shall be '1'. This bit prevents emulation of start codes.
            throw mpeg2_exception(UMC::UMC_ERR_INVALID_STREAM);

        dispExt.display_vertical_size = GetBits(14);
    }

    // Parse picture header
    void MPEG2HeadersBitstream::GetPictureHeader(MPEG2PictureHeader & pic)
    {
        pic.temporal_reference = GetBits(10);

        pic.picture_coding_type = GetBits(3);
        if (pic.picture_coding_type > MPEG2_B_PICTURE || pic.picture_coding_type < MPEG2_I_PICTURE)
            throw mpeg2_exception(UMC::UMC_ERR_INVALID_STREAM);

        pic.vbv_delay = GetBits(16);

        if (pic.picture_coding_type == MPEG2_P_PICTURE || pic.picture_coding_type == MPEG2_B_PICTURE)
        {
            pic.full_pel_forward_vector = GetBits(1);
            pic.forward_f_code = GetBits(3);
        }

        if (pic.picture_coding_type == MPEG2_B_PICTURE)
        {
            pic.full_pel_backward_vector = GetBits(1);
            pic.backward_f_code = GetBits(3);
        }
        /*
        while (GetBits(1))
        {
            uint8_t extra_information_picture = GetBits(8);
        }
        */
    }

    // Parse picture extension
    void MPEG2HeadersBitstream::GetPictureExtensionHeader(MPEG2PictureCodingExtension & picExt)
    {
        picExt.f_code[0] = GetBits(4); // forward horizontal
        picExt.f_code[1] = GetBits(4); // forward vertical
        picExt.f_code[2] = GetBits(4); // backward horizontal
        picExt.f_code[3] = GetBits(4); // backward vertical
        picExt.intra_dc_precision = GetBits(2);

        picExt.picture_structure = GetBits(2);
        if (picExt.picture_structure == 0)
            throw mpeg2_exception(UMC::UMC_ERR_INVALID_STREAM);

        picExt.top_field_first = GetBits(1);
        picExt.frame_pred_frame_dct = GetBits(1);
        picExt.concealment_motion_vectors = GetBits(1);
        picExt.q_scale_type = GetBits(1);
        picExt.intra_vlc_format = GetBits(1);
        picExt.alternate_scan = GetBits(1);
        picExt.repeat_first_field = GetBits(1);
        picExt.chroma_420_type = GetBits(1);
        picExt.progressive_frame = GetBits(1);
        picExt.composite_display_flag = GetBits(1);
        if (picExt.composite_display_flag)
        {
            picExt.v_axis = GetBits(1);
            picExt.field_sequence = GetBits(3);
            picExt.sub_carrier = GetBits(1);
            picExt.burst_amplitude = GetBits(7);
            picExt.sub_carrier_phase = GetBits(8);
        }
    }

    // Parse quant matrix extension
    void MPEG2HeadersBitstream::GetQuantMatrix(MPEG2QuantMatrix & qm)
    {
        qm.load_intra_quantiser_matrix = GetBits(1);
        if (qm.load_intra_quantiser_matrix)
        {
            for (uint8_t i= 0; i < 64; ++i)
            {
                qm.intra_quantiser_matrix[i] = GetBits(8);
            }
        }
        qm.load_non_intra_quantiser_matrix = GetBits(1);
        if (qm.load_non_intra_quantiser_matrix)
        {
            for (uint8_t i= 0; i < 64; ++i)
            {
                qm.non_intra_quantiser_matrix[i] = GetBits(8);
            }
        }
        qm.load_chroma_intra_quantiser_matrix = GetBits(1);
        if (qm.load_chroma_intra_quantiser_matrix)
        {
            for (uint8_t i= 0; i < 64; ++i)
            {
                qm.chroma_intra_quantiser_matrix[i] = GetBits(8);
            }
        }
        qm.load_chroma_non_intra_quantiser_matrix = GetBits(1);
        if (qm.load_chroma_non_intra_quantiser_matrix)
        {
            for (uint8_t i= 0; i < 64; ++i)
            {
                qm.chroma_non_intra_quantiser_matrix[i] = GetBits(8);
            }
        }
    }

    // Parse quant matrix extension (Table 6-11)
    void MPEG2HeadersBitstream::GetGroupOfPicturesHeader(MPEG2GroupOfPictures& g)
    {
        g.drop_frame_flag    = GetBits(1);
        g.time_code_hours    = GetBits(5);

        if (g.time_code_hours > 23)
            throw mpeg2_exception(UMC::UMC_ERR_INVALID_STREAM);

        g.time_code_minutes  = GetBits(6);
        if (g.time_code_minutes > 59)
            throw mpeg2_exception(UMC::UMC_ERR_INVALID_STREAM);

        Seek(1); // marker_bit

        g.time_code_seconds  = GetBits(6);
        if (g.time_code_seconds > 59)
            throw mpeg2_exception(UMC::UMC_ERR_INVALID_STREAM);

        g.time_code_pictures = GetBits(6);
        if (g.time_code_pictures > 59)
            throw mpeg2_exception(UMC::UMC_ERR_INVALID_STREAM);

        g.closed_gop         = GetBits(1);
        g.broken_link        = GetBits(1);
    }

    // Parse slice header
    UMC::Status MPEG2HeadersBitstream::GetSliceHeader(MPEG2SliceHeader & sliceHdr, const MPEG2SequenceHeader &seq, const MPEG2SequenceExtension & seqExt)
    {
        sliceHdr.slice_vertical_position = GetBits(8);

        uint16_t vertical_size = seq.vertical_size_value | (seqExt.vertical_size_extension << 14);

        if (vertical_size > 2800)
        {
            if (sliceHdr.slice_vertical_position > 128) // 6.3.16
                throw mpeg2_exception(UMC::UMC_ERR_INVALID_STREAM);

            sliceHdr.slice_vertical_position_extension = GetBits(3);
        }
        else
        {
            if (sliceHdr.slice_vertical_position > 175) // 6.3.16
                throw mpeg2_exception(UMC::UMC_ERR_INVALID_STREAM);
        }

        sliceHdr.quantiser_scale_code = GetBits(5);
        if (!sliceHdr.quantiser_scale_code)
            throw mpeg2_exception(UMC::UMC_ERR_INVALID_STREAM);

        if (Check1Bit())
        {
            sliceHdr.intra_slice_flag = GetBits(1);
            sliceHdr.intra_slice = GetBits(1);
            Seek(7); // reserved_bits

            while (Check1Bit())
            {
                Seek(1);  // extra_bit_slice
                Seek(8);  // extra_information_slice
            }
        }

        Seek(1);  // extra_bit_slice, shall be 0

        return UMC::UMC_OK;
    }

    // Parse full slice header
    UMC::Status MPEG2HeadersBitstream::GetSliceHeaderFull(MPEG2Slice & slice, const MPEG2SequenceHeader & seq, const MPEG2SequenceExtension & seqExt)
    {
        UMC::Status sts = GetSliceHeader(slice.GetSliceHeader(), seq, seqExt);
        if (UMC::UMC_OK != sts)
            return sts;

        slice.sliceHeader.mbOffset = (uint32_t)BitsDecoded();

        slice.sliceHeader.macroblockAddressIncrement = 1;
        if (0 == Check1Bit())
        {
            DecodeMBAddress(slice.sliceHeader);
        }
        --slice.sliceHeader.macroblockAddressIncrement;

        if (CheckBSLeft())
            throw mpeg2_exception(UMC::UMC_ERR_INVALID_STREAM);

        uint32_t width_in_MBs  = mfx::align2_value<uint32_t>(slice.GetSeqHeader().horizontal_size_value, 16) / 16u;
        uint32_t height_in_MBs = mfx::align2_value<uint32_t>(slice.GetSeqHeader().vertical_size_value, 16) / 16u;

        // invalid slice - skipping it
        if (slice.sliceHeader.slice_vertical_position > height_in_MBs)
            throw mpeg2_exception(UMC::UMC_ERR_INVALID_PARAMS);

        slice.sliceHeader.numberMBsInSlice = width_in_MBs - slice.sliceHeader.macroblockAddressIncrement;

        return UMC::UMC_OK;
    }

    struct VLCEntry
    {
        int8_t value;
        uint8_t length;
    };

    static const VLCEntry MBAddrIncrTabB1_1[16] =
    {
        { -2 + 1, 48 - 48 }, { -2 + 1, 67 - 67 },
        { 2 + 5,   3 + 2 },  { 2 + 4,   3 + 2 },
        { 0 + 5,   1 + 3 },  { 4 + 1,   2 + 2 },
        { 3 + 1,   2 + 2 },  { 0 + 4,   3 + 1 },
        { 2 + 1,   2 + 1 },  { 0 + 3,   0 + 3 },
        { 1 + 2,   2 + 1 },  { 1 + 2,   1 + 2 },
        { 0 + 2,   2 + 1 },  { 1 + 1,   2 + 1 },
        { 0 + 2,   2 + 1 },  { 0 + 2,   2 + 1 },
    };

    static const VLCEntry MBAddrIncrTabB1_2[104] =
    {
        { 18 + 15, 5 + 6 }, { 18 + 14, 0 + 11 }, { 12 + 19, 7 + 4 }, { 6 + 24, 7 + 4 },
        { 4 + 25, 4 + 7 },  { 1 + 27, 0 + 11 },  { 4 + 23, 2 + 9 },  { 15 + 11, 3 + 8 },
        { 22 + 3, 1 + 10 }, { 15 + 9, 2 + 9 },   { 4 + 19, 5 + 6 },  { 0 + 22, 7 + 4 },
        { 2 + 19, 3 + 7 },  { 2 + 19, 7 + 3 },   { 1 + 19, 0 + 10 }, { 15 + 5, 6 + 4 },
        { 11 + 8, 4 + 6 },  { 0 + 19, 7 + 3 },   { 9 + 9, 8 + 2 },   { 10 + 8, 4 + 6 },
        { 8 + 9, 7 + 3 },   { 0 + 17, 6 + 4 },   { 5 + 11, 4 + 6 },  { 12 + 4, 3 + 7 },
        { 8 + 7, 3 + 5 },   { 2 + 13, 1 + 7 },   { 13 + 2, 2 + 6 },  { 1 + 14, 4 + 4 },
        { 0 + 15, 6 + 2 },  { 5 + 10, 5 + 3 },   { 4 + 11, 0 + 8 },  { 6 + 9, 3 + 5 },
        { 5 + 9, 4 + 4 },   { 5 + 9, 4 + 4 },    { 12 + 2, 3 + 5 },  { 5 + 9, 2 + 6 },
        { 9 + 5, 2 + 6 },   { 1 + 13, 7 + 1 },   { 1 + 13, 3 + 5 },  { 5 + 9, 4 + 4 },
        { 0 + 13, 1 + 7 },  { 12 + 1, 5 + 3 },   { 1 + 12, 7 + 1 },  { 2 + 11, 4 + 4 },
        { 1 + 12, 6 + 2 },  { 3 + 10, 3 + 5 },   { 6 + 7, 2 + 6 },   { 9 + 4, 6 + 2 },
        { 7 + 5, 2 + 6 },   { 11 + 1, 0 + 8 },   { 6 + 6, 5 + 3 },   { 3 + 9, 0 + 8 },
        { 4 + 8, 2 + 6 },   { 4 + 8, 7 + 1 },    { 10 + 2, 1 + 7 },  { 4 + 8, 6 + 2 },
        { 4 + 7, 2 + 6 },   { 4 + 7, 7 + 1 },    { 9 + 2, 6 + 2 },   { 8 + 3, 0 + 8 },
        { 7 + 4, 6 + 2 },   { 5 + 6, 3 + 5 },    { 9 + 2, 3 + 5 },   { 8 + 3, 0 + 8 },
        { 3 + 7, 5 + 3 },   { 7 + 3, 4 + 4 },    { 9 + 1, 4 + 4 },   { 8 + 2, 3 + 5 },
        { 3 + 7, 5 + 3 },   { 9 + 1, 6 + 2 },    { 7 + 3, 0 + 8 },   { 7 + 3, 2 + 6 },
        { 6 + 3, 6 + 1 },   { 3 + 6, 1 + 6 },    { 7 + 2, 6 + 1 },   { 8 + 1, 3 + 4 },
        { 4 + 5, 5 + 2 },   { 7 + 2, 1 + 6 },    { 2 + 7, 3 + 4 },   { 7 + 2, 1 + 6 },
        { 7 + 2, 0 + 7 },   { 7 + 2, 5 + 2 },    { 8 + 1, 0 + 7 },   { 4 + 5, 3 + 4 },
        { 0 + 9, 1 + 6 },   { 0 + 9, 4 + 3 },    { 4 + 5, 2 + 5 },   { 8 + 1, 5 + 2 },
        { 5 + 3, 0 + 7 },   { 6 + 2, 1 + 6 },    { 6 + 2, 6 + 1 },   { 4 + 4, 1 + 6 },
        { 0 + 8, 6 + 1 },   { 2 + 6, 1 + 6 },    { 2 + 6, 3 + 4 },   { 3 + 5, 6 + 1 },
        { 2 + 6, 2 + 5 },   { 3 + 5, 6 + 1 },    { 2 + 6, 6 + 1 },   { 6 + 2, 4 + 3 },
        { 3 + 5, 0 + 7 },   { 0 + 8, 0 + 7 },    { 6 + 2, 3 + 4 },   { 3 + 5, 1 + 6 },
    };

    // Get macroblock address increment
    void MPEG2HeadersBitstream::DecodeMBAddress(MPEG2SliceHeader & sliceHdr)
    {
        uint32_t macroblock_address_increment = 0;
        uint32_t cc;
        uint8_t bitsToRead;

        for (;;)
        {
            bitsToRead = BitsLeft() <= 11 ? BitsLeft() : 11;

            cc = GetBits(bitsToRead);
            if (11 - bitsToRead) // if left bits are less than 11
                cc <<= (11 - bitsToRead);

            Seek(-bitsToRead);

            if (cc >= 24)
                break;

            if (cc != 15)    // if not macroblock_stuffing
            {
                if (cc != 8) // if not macroblock_escape
                {
                    sliceHdr.macroblockAddressIncrement = 1;
                    return;
                }

                macroblock_address_increment += 33;
            }

            Seek(bitsToRead);
        }

        if (cc >= 1024)
        {
            Seek(1);
            sliceHdr.macroblockAddressIncrement = macroblock_address_increment + 1;
            return;
        }

        uint32_t cc1;
        uint32_t length;

        if (cc >= 128)
        {
            cc >>= 6;
            length = MBAddrIncrTabB1_1[cc].length;
            cc1 = GetBits(length);
            sliceHdr.macroblockAddressIncrement = macroblock_address_increment + MBAddrIncrTabB1_1[cc].value;
            return;
        }

        cc -= 24;
        length = MBAddrIncrTabB1_2[cc].length;
        cc1 = GetBits(length);
        sliceHdr.macroblockAddressIncrement = macroblock_address_increment + MBAddrIncrTabB1_2[cc].value;
        std::ignore = cc1;
    }

}
#endif // MFX_ENABLE_MPEG2_VIDEO_DECODE
