// Copyright (c) 2017 Intel Corporation
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

#include "mfx_common.h"
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE)

#include "mfx_common.h"
#include "mfx_h265_encode_hw_set.h"
#include "mfx_h265_encode_hw_utils.h"
#include <vector>
#include <exception>

namespace MfxHwH265Encode
{

class EndOfBuffer : public std::exception
{
public:
    EndOfBuffer() : std::exception() {}
};

class BitstreamWriter
{
public:
    BitstreamWriter(mfxU8* bs, mfxU32 size, mfxU8 bitOffset = 0);
    ~BitstreamWriter();

    void PutBits        (mfxU32 n, mfxU32 b);
    void PutBitsBuffer  (mfxU32 n, void* b, mfxU32 offset = 0);
    void PutBit         (mfxU32 b);
    void PutGolomb      (mfxU32 b);
    void PutTrailingBits(bool bCheckAligned = false);

    inline void PutUE   (mfxU32 b)    { PutGolomb(b); }
    inline void PutSE   (mfxI32 b)    { (b > 0) ? PutGolomb((b<<1)-1) : PutGolomb((-b)<<1); }

    inline mfxU32 GetOffset() { return (mfxU32)(m_bs - m_bsStart) * 8 + m_bitOffset - m_bitStart; }
    inline mfxU8* GetStart () { return m_bsStart; }

    void Reset(mfxU8* bs = 0, mfxU32 size = 0, mfxU8 bitOffset = 0);
    void cabacInit();
    void EncodeBin(mfxU8 * ctx, mfxU8 binVal);
    void EncodeBinEP(mfxU8 binVal);
    void SliceFinish();
    void PutBitC(mfxU32 B);
private:
    void RenormE();
    mfxU8* m_bsStart;
    mfxU8* m_bsEnd;
    mfxU8* m_bs;
    mfxU8  m_bitStart;
    mfxU8  m_bitOffset;

    mfxU32 m_codILow;
    mfxU32 m_codIRange;
    mfxU32 m_bitsOutstanding;
    mfxU32 m_BinCountsInNALunits;
    bool   m_firstBitFlag;
};

class BitstreamReader
{
public:
    BitstreamReader(mfxU8* bs, mfxU32 size, mfxU8 bitOffset = 0);
    ~BitstreamReader();

    mfxU32 GetBit ();
    mfxU32 GetBits(mfxU32 n);
    mfxU32 GetUE  ();
    mfxI32 GetSE  ();

    inline mfxU32 GetOffset() { return (mfxU32)(m_bs - m_bsStart) * 8 + m_bitOffset - m_bitStart; }
    inline mfxU8* GetStart() { return m_bsStart; }

    inline void SetEmulation(bool f) { m_emulation = f; };
    inline bool GetEmulation() { return m_emulation; };

    void Reset(mfxU8* bs = 0, mfxU32 size = 0, mfxU8 bitOffset = 0);

private:
    mfxU8* m_bsStart;
    mfxU8* m_bsEnd;
    mfxU8* m_bs;
    mfxU8  m_bitStart;
    mfxU8  m_bitOffset;
    bool   m_emulation;
};

#define NUM_CABAC_CONTEXT ((188+63)&~63)

enum // Syntax element type for HEVC
{
    QT_CBF_HEVC = 0,
    QT_ROOT_CBF_HEVC = 1,
    LAST_X_HEVC = 2,
    LAST_Y_HEVC = 3,
    SIG_COEFF_GROUP_FLAG_HEVC = 4,
    SIG_FLAG_HEVC = 5,
    ONE_FLAG_HEVC = 6,
    ABS_FLAG_HEVC = 7,
    TRANS_SUBDIV_FLAG_HEVC = 8,
    TRANSFORMSKIP_FLAG_HEVC = 9,
    CU_TRANSQUANT_BYPASS_FLAG_CTX = 10,
    SPLIT_CODING_UNIT_FLAG_HEVC = 11,
    SKIP_FLAG_HEVC = 12,
    MERGE_FLAG_HEVC = 13,
    MERGE_IDX_HEVC = 14,
    PART_SIZE_HEVC = 15,
    AMP_SPLIT_POSITION_HEVC = 16,
    PRED_MODE_HEVC = 17,
    INTRA_LUMA_PRED_MODE_HEVC = 18,
    INTRA_CHROMA_PRED_MODE_HEVC = 19,
    INTER_DIR_HEVC = 20,
    MVD_HEVC = 21,
    REF_FRAME_IDX_HEVC = 22,
    DQP_HEVC = 23,
    MVP_IDX_HEVC = 24,
    SAO_MERGE_FLAG_HEVC = 25,
    SAO_TYPE_IDX_HEVC = 26,
    END_OF_SLICE_FLAG_HEVC = 27,

    MAIN_SYNTAX_ELEMENT_NUMBER_HEVC
};
/////
mfxU32 AddEmulationPreventionAndCopy(
    mfxU8* data,
    mfxU32 lenght,
    mfxU8*                           bsDataStart,
    mfxU8*                           bsEnd,
    bool                             bEmulationByteInsertion);


extern const mfxU32 tab_ctxIdxOffset[MAIN_SYNTAX_ELEMENT_NUMBER_HEVC];
#define CONTEXT(context_array,a) (context_array+tab_ctxIdxOffset[a])
class HeaderReader
{
public:
    HeaderReader(){};
    ~HeaderReader(){};

    static mfxStatus ReadNALU(BitstreamReader& bs, NALU & nalu);
    static mfxStatus ReadSPS (BitstreamReader& bs, SPS  & sps);
    static mfxStatus ReadPPS (BitstreamReader& bs, PPS  & pps);

};

class HeaderPacker
{
public:

    HeaderPacker();
    ~HeaderPacker();

    mfxStatus Reset(MfxVideoParam const & par);
    mfxStatus ResetPPS(MfxVideoParam const & par);

    inline void GetAUD(mfxU8*& buf, mfxU32& len, mfxU8 pic_type){assert(pic_type < 3); buf = m_bs_aud[pic_type%3]; len = 7;}
    inline void GetVPS(mfxU8*& buf, mfxU32& len){buf = m_bs_vps; len = m_sz_vps;}
    inline void GetSPS(mfxU8*& buf, mfxU32& len){buf = m_bs_sps; len = m_sz_sps;}
    inline void GetPPS(mfxU8*& buf, mfxU32& len){buf = m_bs_pps; len = m_sz_pps;}
    void GetPrefixSEI(Task const & task, mfxU8*& buf, mfxU32& len);
    void GetSuffixSEI(Task const & task, mfxU8*& buf, mfxU32& len);
    void GetSSH(Task const & task,
        mfxU32 id,
        mfxU8*& buf,
        mfxU32& len,
        mfxU32* qpd_offset,
        bool dyn_slice_size = false,
        mfxU32* sao_offset = 0,
        mfxU16* ssh_start_len = 0,
        mfxU32* ssh_offset = 0,
        mfxU32* pwt_offset = 0,
        mfxU32* pwt_length = 0);
    void GetSkipSlice(Task const & task, mfxU32 id, mfxU8*& buf, mfxU32& len, mfxU32* qpd_offset = 0);
    void codingTree(mfxU32 xCtu, mfxU32 yCtu, mfxU32 log2CtuSize, BitstreamWriter& bs, const Slice& slice, mfxU32 x0, mfxU32 y0, mfxU8* tabl);
    static void PackNALU (BitstreamWriter& bs, NALU  const &  nalu);
    static void PackAUD  (BitstreamWriter& bs, mfxU8 pic_type);
    static void PackVPS  (BitstreamWriter& bs, VPS   const &  vps);
    static void PackSPS  (BitstreamWriter& bs, SPS   const &  sps);
    static void PackPPS  (BitstreamWriter& bs, PPS   const &  pps);
    static void PackSSH  (BitstreamWriter& bs,
                          NALU  const &     nalu,
                          SPS   const &     sps,
                          PPS   const &     pps,
                          Slice const &     slice,
                          mfxU32* qpd_offset,
                          bool    dyn_slice_size = false,
                          mfxU32* sao_offset = 0,
                          mfxU32* pwt_offset = 0,
                          mfxU32* pwt_length = 0);
    static void PackVUI  (BitstreamWriter& bs, VUI        const & vui, mfxU16 max_sub_layers_minus1);
    static void PackHRD  (BitstreamWriter& bs, HRDInfo    const & hrd, bool commonInfPresentFlag, mfxU16 maxNumSubLayersMinus1);
    static void PackPTL  (BitstreamWriter& bs, LayersInfo const & ptl, mfxU16 max_sub_layers_minus1);
    static void PackSLO  (BitstreamWriter& bs, LayersInfo const & slo, mfxU16 max_sub_layers_minus1);
    static void PackSTRPS(BitstreamWriter& bs, const STRPS * h, mfxU32 num, mfxU32 idx);

    static void PackSEIPayload(BitstreamWriter& bs, VUI const & vui, BufferingPeriodSEI const & bp);
    static void PackSEIPayload(BitstreamWriter& bs, VUI const & vui, PicTimingSEI const & pt);
    static mfxStatus PackRBSP(mfxU8* dst, mfxU8* src, mfxU32& dst_size, mfxU32 src_size);


private:
    static const mfxU32 RBSP_SIZE   = 1024;
    static const mfxU32 AUD_BS_SIZE = 8;
    static const mfxU32 VPS_BS_SIZE = 256;
    static const mfxU32 SPS_BS_SIZE = 256;
    static const mfxU32 PPS_BS_SIZE = 128;
    static const mfxU32 SSH_BS_SIZE = MAX_SLICES * 16;

    mfxU8  m_rbsp[RBSP_SIZE];
    mfxU8  m_bs_aud[3][AUD_BS_SIZE];
    mfxU8  m_bs_vps[VPS_BS_SIZE];
    mfxU8  m_bs_sps[SPS_BS_SIZE];
    mfxU8  m_bs_pps[PPS_BS_SIZE];
    mfxU8  m_bs_ssh[SSH_BS_SIZE];
    mfxU32 m_sz_vps;
    mfxU32 m_sz_sps;
    mfxU32 m_sz_pps;
    mfxU32 m_sz_ssh;

    const MfxVideoParam * m_par;
    BitstreamWriter       m_bs;
};

} //MfxHwH265Encode

#endif
