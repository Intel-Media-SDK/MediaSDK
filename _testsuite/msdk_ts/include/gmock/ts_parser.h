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

#pragma once

#include "ts_common.h"
#include "bs_parser++.h"


template<class T> class tsParser : public T
{
public:
    BSErr m_sts;

    tsParser(mfxU32 mode = 0)
        : T(mode)
        , m_sts(BS_ERR_NONE)
    {
        if(!g_tsTrace)
            this->set_trace_level(0);
    }

    tsParser(mfxBitstream b, mfxU32 mode = 0)
        : T(mode)
        , m_sts(BS_ERR_NONE)
    {
        if(!g_tsTrace)
            this->set_trace_level(0);
        SetBuffer(b);
    }

    ~tsParser(){}

    typename T::UnitType* Parse(bool orDie = false)
    {
        TRACE_FUNC0(BS_parser::parse_next_unit);
        m_sts = this->parse_next_unit();
        if(m_sts && (m_sts != BS_ERR_MORE_DATA) && orDie)
        {
            g_tsLog << "FAILED in tsParser\n";
            g_tsStatus.check(MFX_ERR_UNKNOWN);
        }
        return (typename T::UnitType* ) this->get_header();
    }

    typename T::UnitType& ParseOrDie()
    {
        return *Parse(true);
    }

    void SetBuffer(mfxBitstream b)
    {
        TRACE_FUNC2(BS_parser::set_buffer, b.Data + b.DataOffset, b.DataLength + 3);
        this->set_buffer(b.Data + b.DataOffset, b.DataLength + 3);
    }

    void SetBuffer0(mfxBitstream b)
    {
        TRACE_FUNC2(BS_parser::set_buffer, b.Data + b.DataOffset, b.DataLength);
        this->set_buffer(b.Data + b.DataOffset, b.DataLength);
    }

};

typedef tsParser<BS_VP8_parser> tsParserVP8;
typedef tsParser<BS_VP9_parser> tsParserVP9;
typedef tsParser<BS_MPEG2_parser> tsParserMPEG2;
typedef tsParser<BS_HEVC_parser> tsParserHEVC;
typedef tsParser<BS_H264_parser> tsParserH264NALU;
typedef tsParser<BS_AVC2_parser> tsParserAVC2;
typedef tsParser<BS_HEVC2_parser> tsParserHEVC2;

inline bool IsHEVCSlice(Bs32u nut) { return (nut <= 21) && ((nut < 10) || (nut > 15)); }

class tsParserHEVCAU : public BS_HEVC_parser
{
public:
    typedef BS_HEVC::AU UnitType;
    UnitType dummy;
    BSErr m_sts;

    tsParserHEVCAU(mfxU32 mode = 0)
        : BS_HEVC_parser(mode)
        , m_sts(BS_ERR_NONE)
    {
        if(!g_tsTrace)
            set_trace_level(0);
    }

    tsParserHEVCAU(mfxBitstream b, mfxU32 mode = 0)
        : BS_HEVC_parser(mode)
        , m_sts(BS_ERR_NONE)
    {
        if(!g_tsTrace)
            set_trace_level(0);
        SetBuffer(b);
    }

    ~tsParserHEVCAU(){}

    UnitType* Parse(bool orDie = false)
    {
        UnitType* pAU = 0;

        TRACE_FUNC0(BS_parser::parse_next_unit);
        m_sts = parse_next_au(pAU);
        if(m_sts && (m_sts != BS_ERR_MORE_DATA) && orDie)
        {
            g_tsLog << "ERROR: FAILED in tsParser: " << m_sts << "\n";
            g_tsStatus.check(MFX_ERR_UNKNOWN);
        }
        if(pAU == NULL)
            pAU = &dummy;
        return pAU;
    }

    UnitType& ParseOrDie()
    {
        return *Parse(true);
    }

    void SetBuffer(mfxBitstream b)
    {
        TRACE_FUNC2(BS_parser::set_buffer, b.Data + b.DataOffset, b.DataLength + 3);
        set_buffer(b.Data + b.DataOffset, b.DataLength + 3);
    }
};

class tsParserH264AU : public BS_H264_parser
{
public:
    typedef BS_H264_au UnitType;
    UnitType dummy;
    BSErr m_sts;

    tsParserH264AU(mfxU32 mode = 0)
        : BS_H264_parser(mode)
        , m_sts(BS_ERR_NONE)
    {
        if(!g_tsTrace)
            set_trace_level(0);
    }

    tsParserH264AU(mfxBitstream b, mfxU32 mode = 0)
        : BS_H264_parser(mode)
        , m_sts(BS_ERR_NONE)
    {
        if(!g_tsTrace)
            set_trace_level(0);
        SetBuffer(b);
    }

    ~tsParserH264AU(){}

    UnitType* Parse(bool orDie = false)
    {
        UnitType* pAU = 0;

        TRACE_FUNC0(BS_parser::parse_next_unit);
        m_sts = parse_next_au(pAU);
        if(m_sts && (m_sts != BS_ERR_MORE_DATA) && orDie)
        {
            g_tsLog << "ERROR: FAILED in tsParser: " << m_sts << "\n";
            g_tsStatus.check(MFX_ERR_UNKNOWN);
        }
        if(pAU == NULL)
            pAU = &dummy;
        return pAU;
    }

    UnitType& ParseOrDie()
    {
        return *Parse(true);
    }

    void SetBuffer(mfxBitstream b)
    {
        TRACE_FUNC2(BS_parser::set_buffer, b.Data + b.DataOffset, b.DataLength + 3);
        set_buffer(b.Data + b.DataOffset, b.DataLength + 3);
    }
};

class tsParserMPEG2AU : public BS_MPEG2_parser
{
    BS_MPEG2_au* pAU;
    std::vector<slice_params> slices;
    mfxBitstream* pBs;
    void init()
    {
        pAU = new BS_MPEG2_au;
        pAU->pic_hdr = new picture_header;
        memset(pAU->pic_hdr, 0, sizeof(picture_header));
        pAU->seq_hdr = new sequence_header;
        memset(pAU->seq_hdr, 0, sizeof(sequence_header));
        pAU->seq_ext = new sequence_extension;
        memset(pAU->seq_ext, 0, sizeof(sequence_extension));
        pAU->seq_displ_ext = new sequence_display_extension;
        memset(pAU->seq_displ_ext, 0, sizeof(sequence_display_extension));
        pAU->pic_coding_ext = new picture_coding_extension;
        memset(pAU->pic_coding_ext, 0, sizeof(picture_coding_extension));
        pAU->gop_hdr = new group_of_pictures_header;
        memset(pAU->gop_hdr, 0, sizeof(group_of_pictures_header));
        pAU->seq_scalable_ext = new sequence_scalable_extension;
        memset(pAU->seq_scalable_ext, 0, sizeof(sequence_scalable_extension));
        pAU->NumSlice = 0;
        pAU->slice = 0;
    }
public:
    typedef BS_MPEG2_au UnitType;
    BSErr m_sts;
    Bs32u last_offset;

    tsParserMPEG2AU(mfxU32 mode = 0)
        : BS_MPEG2_parser(mode)
        , m_sts(BS_ERR_NONE)
        , last_offset(0)
    {
        if(!g_tsTrace)
            set_trace_level(0);
        init();
    }

    tsParserMPEG2AU(mfxBitstream b, mfxU32 mode = 0)
        : BS_MPEG2_parser(mode)
        , m_sts(BS_ERR_NONE)
        , last_offset(0)
    {
        if(!g_tsTrace)
            set_trace_level(0);
        SetBuffer(b);
        pBs = &b;
        init();
    }

    ~tsParserMPEG2AU()
    {
        if (pAU->pic_hdr) delete pAU->pic_hdr;
        if (pAU->seq_hdr) delete pAU->seq_hdr;
        if (pAU->seq_ext) delete pAU->seq_ext;
        if (pAU->seq_displ_ext) delete pAU->seq_displ_ext;
        if (pAU->pic_coding_ext) delete pAU->pic_coding_ext;
        if (pAU->gop_hdr) delete pAU->gop_hdr;
        if (pAU->seq_scalable_ext) delete pAU->seq_scalable_ext;
        for (size_t i = 0; i < slices.size(); i++) {
            if (slices[i].mb) delete slices[i].mb;
        }
        // delete pAU->slice; - deleted in loop above
        slices.clear();
        delete pAU;
    }

    UnitType* Parse(bool orDie = false)
    {
        memset(pAU->pic_hdr, 0, sizeof(picture_header));
        memset(pAU->seq_hdr, 0, sizeof(sequence_header));
        memset(pAU->seq_ext, 0, sizeof(sequence_extension));
        memset(pAU->seq_displ_ext, 0, sizeof(sequence_display_extension));
        memset(pAU->pic_coding_ext, 0, sizeof(picture_coding_extension));
        memset(pAU->gop_hdr, 0, sizeof(group_of_pictures_header));
        memset(pAU->seq_scalable_ext, 0, sizeof(sequence_scalable_extension));
        for (size_t i = 0; i < slices.size(); i++) {
            if (slices[i].mb) delete slices[i].mb;
        }
        slices.clear();

        while (1)
        {
            TRACE_FUNC0(BS_parser::parse_next_unit);
            m_sts = parse_next_unit();
            if(m_sts && orDie)
            {
                if (m_sts == BS_ERR_MORE_DATA && (pAU->NumSlice || slices.size())) {
                    last_offset = 0;
                    pAU->slice = &slices[0];
                    pAU->NumSlice = (Bs32u)slices.size();
                    return pAU;
                }

                g_tsLog << "ERROR: FAILED in tsParser: " << m_sts << "\n";
                g_tsStatus.check(MFX_ERR_UNKNOWN);
            }

            BS_MPEG2_header* pHdr = (BS_MPEG2_header*)get_header();

            bool is_slice = ((pHdr->start_code >= 0x00000101 && pHdr->start_code < 0x000001B0) &&
                (pHdr->start_code_id >= 0x01 && pHdr->start_code_id <= 0xAF));

            if (slices.size() && !is_slice) {  // end of AU
                Bs32u len = pBs->DataLength + 3 - last_offset;
                set_buffer(pBs->Data + pBs->DataOffset + last_offset, len);
                pAU->slice = &slices[0];
                pAU->NumSlice = (Bs32u)slices.size();
                return pAU;
            }

            if (is_slice) {  // Slice
                slice_params s = {0};
                memcpy(&s, pHdr->slice, sizeof(slice_params));
                s.mb = new mpeg2_macroblock[pHdr->slice->NumMb];
                memcpy(s.mb, pHdr->slice->mb, sizeof(mpeg2_macroblock) * pHdr->slice->NumMb);
                slices.push_back(s);

                last_offset = (Bs32u)get_offset();

                continue;
            }

            switch (pHdr->start_code) {
                case 0x00000100:
                    memcpy(pAU->pic_hdr, pHdr->pic_hdr, sizeof(picture_header));
                    break;
                case 0x000001B3:
                    memcpy(pAU->seq_hdr, pHdr->seq_hdr, sizeof(sequence_header));
                    break;
                case 0x000001B5: /*extension_start_code*/
                    if (pHdr->start_code_id == 5)   // Table 6-2. extension_start_code_identifier codes.
                    {
                        memcpy(pAU->seq_scalable_ext, pHdr->seq_scalable_ext, sizeof(sequence_scalable_extension));
                        break;
                    }
                    switch (pHdr->start_code_id)
                    {
                        case 0x1:
                            memcpy(pAU->seq_ext, pHdr->seq_ext, sizeof(sequence_extension));
                            break;
                        case 0x2:
                            memcpy(pAU->seq_displ_ext, pHdr->seq_displ_ext, sizeof(sequence_display_extension));
                            break;
                        case 0x8:
                            memcpy(pAU->pic_coding_ext, pHdr->pic_coding_ext, sizeof(picture_coding_extension));
                            break;
                        default:
                            break;
                    }
                    break;
                /*case 0x000001B7:
                    trace_off();
                    BS_TRACE( 1, ================ Sequence End ================= );
                    break;*/
                case 0x000001B8:
                    memcpy(pAU->gop_hdr, pHdr->gop_hdr, sizeof(group_of_pictures_header));
                    break;
                default:
                    break;
            }
        }

        return pAU;
    }

    UnitType& ParseOrDie()
    {
        return *Parse(true);
    }

    void SetBuffer(mfxBitstream b)
    {
        TRACE_FUNC2(BS_parser::set_buffer, b.Data + b.DataOffset, b.DataLength + 3);
        set_buffer(b.Data + b.DataOffset, b.DataLength + 3);
    }

    BSErr sset_buffer(byte* buf, Bs32u buf_size)
    {
        Bs32u shift = 0;
        Bs32u curr_offset = (Bs32u)get_offset();
        if (last_offset && curr_offset > last_offset) {
            shift = curr_offset - last_offset;
            buf_size += shift;
        }

        return BS_MPEG2_parser::set_buffer(buf - shift, buf_size);
    }
};

class H264AUWrapper
{
protected:
    BS_H264_au const & m_au;
    mfxI32 m_idx;
    slice_header* m_sh;
    macro_block* m_mb;
public:

    H264AUWrapper(BS_H264_au const & au)
        : m_au(au)
        , m_idx(-1)
        , m_sh(0)
        , m_mb(0)
    {
    }
    ~H264AUWrapper()
    {
    }

    slice_header* NextSlice()
    {
        m_sh = 0;

        while ((mfxU32)++m_idx < m_au.NumUnits)
        {
            if (m_au.NALU[m_idx].nal_unit_type == 1
                || m_au.NALU[m_idx].nal_unit_type == 5)
            {
                m_sh = m_au.NALU[m_idx].slice_hdr;
                break;
            }
        }
        return m_sh;
    }

    macro_block* NextMB()
    {
        if (!m_sh)
        {
            m_sh = NextSlice();
            if (!m_sh)
                return 0;
        }

        if (!m_mb)
            m_mb = m_sh->mb;
        else
        {
            m_mb++;

            if (m_mb >= m_sh->mb + m_sh->NumMb)
            {
                m_mb = 0;
                m_sh = 0;
                return NextMB();
            }
        }

        return m_mb;
    }
};
