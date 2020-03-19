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

#pragma once

#include "umc_defs.h"

#if defined (MFX_ENABLE_MPEG2_VIDEO_DECODE)

#include <vector>
#include <stdexcept>
#include <list>
#include <memory>
#include "umc_structures.h"

namespace UMC_MPEG2_DECODER
{
    class MPEG2DecoderFrame;
    typedef std::list<MPEG2DecoderFrame*> DPBType;

    constexpr uint8_t start_code_prefix[] = {0, 0, 1};
    constexpr size_t prefix_size = sizeof(start_code_prefix);

    constexpr uint16_t MFX_PROFILE_MPEG1 = 8; // Just a magic value

    // Table 6-1
    enum MPEG2_UNIT_TYPE
    {
        PICTURE_HEADER  = 0x00,
        USER_DATA       = 0xb2,
        SEQUENCE_HEADER = 0xb3,
        SEQUENCE_ERROR  = 0xb4,
        EXTENSION       = 0xb5,
        SEQUENCE_END    = 0xb7,
        GROUP           = 0xb8,
    };

    // Table 6-5
    enum
    {
        CHROMA_FORMAT_420       = 1,
        CHROMA_FORMAT_422       = 2,
        CHROMA_FORMAT_444       = 3
    };

    // Table 6-12
    enum FrameType
    {
        MPEG2_I_PICTURE = 1,
        MPEG2_P_PICTURE = 2,
        MPEG2_B_PICTURE = 3
    };

    // Table 6-14
    enum
    {
        FLD_PICTURE        = 1,
        TOP_FLD_PICTURE    = 1,
        BOTTOM_FLD_PICTURE = 2,
        FRM_PICTURE        = 3,
    };

    enum DisplayPictureStruct
    {
        DPS_UNKNOWN   = 100,
        DPS_FRAME     = 0,
        DPS_TOP,         // one field
        DPS_BOTTOM,      // one field
        DPS_TOP_BOTTOM,
        DPS_BOTTOM_TOP,
        DPS_TOP_BOTTOM_TOP,
        DPS_BOTTOM_TOP_BOTTOM,
        DPS_FRAME_DOUBLING,
        DPS_FRAME_TRIPLING
    };

    // Table 6-2. Extension_start_code_identifier codes
    enum MPEG2_UNIT_TYPE_EXT
    {
        SEQUENCE_EXTENSION                  = 1,
        SEQUENCE_DISPLAY_EXTENSION          = 2,
        QUANT_MATRIX_EXTENSION              = 3,
        COPYRIGHT_EXTENSION                 = 4,
        SEQUENCE_SCALABLE_EXTENSION         = 5,
        PICTURE_DISPLAY_EXTENSION           = 7,
        PICTURE_CODING_EXTENSION            = 8,
        PICTURE_SPARTIAL_SCALABLE_EXTENSION = 9,
        PICTURE_TEMPORAL_SCALABLE_EXTENSION = 10,
    };

    struct MPEG2SequenceHeader
    {
        uint32_t  horizontal_size_value           = 0;
        uint32_t  vertical_size_value             = 0;
        uint32_t  aspect_ratio_information        = 0;
        uint32_t  frame_rate_code                 = 0;
        uint32_t  bit_rate_value                  = 0;
        uint32_t  vbv_buffer_size_value           = 0;
        uint8_t   constrained_parameters_flag     = 0;
        uint8_t   load_intra_quantiser_matrix     = 0;
        uint8_t   intra_quantiser_matrix[64]      = {};
        uint8_t   load_non_intra_quantiser_matrix = 0;
        uint8_t   non_intra_quantiser_matrix[64]  = {};

        void Reset()
        {
            *this = {};
        }
    };

    struct MPEG2SequenceExtension
    {
        uint8_t   profile_and_level_indication = 0;
        uint8_t   progressive_sequence         = 0;
        uint8_t   chroma_format                = 0;
        uint8_t   horizontal_size_extension    = 0;
        uint8_t   vertical_size_extension      = 0;
        uint32_t  bit_rate_extension           = 0;
        uint32_t  vbv_buffer_size_extension    = 0;
        uint8_t   low_delay                    = 0;
        uint8_t   frame_rate_extension_n       = 0;
        uint8_t   frame_rate_extension_d       = 0;

        void Reset()
        {
            *this = {};
        }
    };

    struct MPEG2SequenceDisplayExtension
    {
        uint8_t   video_format             = 0;
        uint8_t   colour_description       = 0;
        uint8_t   colour_primaries         = 0;
        uint8_t   transfer_characteristics = 0;
        uint8_t   matrix_coefficients      = 0;
        uint16_t  display_horizontal_size  = 0;
        uint16_t  display_vertical_size    = 0;

        void Reset()
        {
            *this = {};
        }
    };

    struct MPEG2QuantMatrix
    {
        uint8_t load_intra_quantiser_matrix            = 0;
        uint8_t intra_quantiser_matrix[64]             = {};
        uint8_t load_non_intra_quantiser_matrix        = 0;
        uint8_t non_intra_quantiser_matrix[64]         = {};
        uint8_t load_chroma_intra_quantiser_matrix     = 0;
        uint8_t chroma_intra_quantiser_matrix[64]      = {};
        uint8_t load_chroma_non_intra_quantiser_matrix = 0;
        uint8_t chroma_non_intra_quantiser_matrix[64]  = {};

        void Reset()
        {
            *this = {};
        }
    };

    struct MPEG2GroupOfPictures
    {
        uint8_t  drop_frame_flag    = 0;
        uint8_t  time_code_hours    = 0;
        uint8_t  time_code_minutes  = 0;
        uint8_t  time_code_seconds  = 0;
        uint8_t  time_code_pictures = 0;
        uint8_t  closed_gop         = 0;
        uint8_t  broken_link        = 0;

        void Reset()
        {
            *this = {};
        }
    };

    struct MPEG2PictureCodingExtension
    {
        uint8_t f_code[4]                  = {};
        uint8_t intra_dc_precision         = 0;
        uint8_t picture_structure          = 0;
        uint8_t top_field_first            = 0;
        uint8_t frame_pred_frame_dct       = 0;
        uint8_t concealment_motion_vectors = 0;
        uint8_t q_scale_type               = 0;
        uint8_t intra_vlc_format           = 0;
        uint8_t alternate_scan             = 0;
        uint8_t repeat_first_field         = 0;
        uint8_t chroma_420_type            = 0;
        uint8_t progressive_frame          = 0;
        uint8_t composite_display_flag     = 0;
        uint8_t v_axis                     = 0;
        uint8_t field_sequence             = 0;
        uint8_t sub_carrier                = 0;
        uint8_t burst_amplitude            = 0;
        uint8_t sub_carrier_phase          = 0;

        void Reset()
        {
            *this = {};
        }
    };

    struct MPEG2PictureHeader
    {
        uint16_t    temporal_reference       = 0;
        uint8_t     picture_coding_type      = 0;
        uint16_t    vbv_delay                = 0;
        uint8_t     full_pel_forward_vector  = 0;
        uint8_t     forward_f_code           = 0;
        uint8_t     full_pel_backward_vector = 0;
        uint8_t     backward_f_code          = 0;

        void Reset()
        {
            *this = {};
        }
    };

    struct MPEG2SliceHeader
    {
        uint8_t slice_vertical_position           = 0;
        uint8_t slice_vertical_position_extension = 0;
        uint8_t priority_breakpoint               = 0;
        uint8_t quantiser_scale_code              = 0;
        uint8_t intra_slice_flag                  = 0;
        uint8_t intra_slice                       = 0;

        // calculated params
        uint32_t mbOffset                         = 0;
        uint32_t macroblockAddressIncrement       = 0;
        uint32_t numberMBsInSlice                 = 0; // only for DXVA
    };

    enum
    {
        NUM_REF_FRAMES = 2,
    };

    // Error exception
    class mpeg2_exception
    {
    public:
        mpeg2_exception(int32_t status = -1)
            : m_Status(status)
        {
        }

        virtual ~mpeg2_exception()
        {
        }

        int32_t GetStatus() const
        {
            return m_Status;
        }

    private:
        int32_t m_Status;
    };

    // Descriptor of raw MPEG2 elementary unit
    struct RawUnit
    {
        RawUnit() : begin(nullptr), end(nullptr), type(-1), pts(-1)
        {}

        RawUnit(uint8_t * b, uint8_t * e, uint8_t t, double ts) : begin(b), end(e), type(t), pts(ts)
        {}

        uint8_t * begin;
        uint8_t * end;
        int16_t   type;
        double    pts; // time stamp
    };

    // Container for raw header binary data
    class RawHeader_MPEG2
    {
    public:

        void Reset()
        { m_buffer.clear(); }

        size_t GetSize() const
        { return m_buffer.size(); }

        uint8_t * GetPointer()
        { return m_buffer.empty() ? nullptr : m_buffer.data(); }

        void Resize(size_t newSize)
        { m_buffer.resize(newSize); }

    protected:
        typedef std::vector<uint8_t> BufferType;
        BufferType  m_buffer;
    };
}

#endif // MFX_ENABLE_MPEG2_VIDEO_DECODE
