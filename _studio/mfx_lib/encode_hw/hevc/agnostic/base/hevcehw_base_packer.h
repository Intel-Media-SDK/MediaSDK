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

#pragma once

#include "mfx_common.h"
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE)

#include "hevcehw_base.h"
#include "hevcehw_base_data.h"
#include <array>

namespace HEVCEHW
{
namespace Base
{
    class BitstreamWriter
        : public IBsWriter
    {
    public:
        BitstreamWriter(mfxU8* bs, mfxU32 size, mfxU8 bitOffset = 0);
        ~BitstreamWriter();

        virtual void PutBits(mfxU32 n, mfxU32 b) override;
        void PutBitsBuffer(mfxU32 n, void* b, mfxU32 offset = 0);
        virtual void PutBit(mfxU32 b) override;
        void PutGolomb(mfxU32 b);
        void PutTrailingBits(bool bCheckAligned = false);

        virtual void PutUE(mfxU32 b)  override { PutGolomb(b); }
        virtual void PutSE(mfxI32 b)  override { (b > 0) ? PutGolomb((b << 1) - 1) : PutGolomb((-b) << 1); }

        mfxU32 GetOffset() { return mfxU32(m_bs - m_bsStart) * 8 + m_bitOffset - m_bitStart; }
        mfxU8* GetStart() { return m_bsStart; }
        mfxU8* GetEnd() { return m_bsEnd; }

        void Reset(mfxU8* bs = 0, mfxU32 size = 0, mfxU8 bitOffset = 0);
        void cabacInit();
        void EncodeBin(mfxU8& ctx, mfxU8 binVal);
        void EncodeBinEP(mfxU8 binVal);
        void SliceFinish();
        void PutBitC(mfxU32 B);

        void AddInfo(mfxU32 key, mfxU32 value)
        {
            if (m_pInfo)
                m_pInfo[0][key] = value;
        }
        void SetInfo(std::map<mfxU32, mfxU32> *pInfo)
        {
            m_pInfo = pInfo;
        }

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
        std::map<mfxU32, mfxU32> *m_pInfo = nullptr;
    };

    
    struct CABACContextTable
    {
        mfxU8 CU_TRANSQUANT_BYPASS_FLAG[1];
        mfxU8 SPLIT_CODING_UNIT_FLAG[3];
        mfxU8 SKIP_FLAG[3];
        mfxU8 MERGE_IDX[1];
        mfxU8 END_OF_SLICE_FLAG[1];
    };

    struct CABACContext
        : CABACContextTable
    {
        static const CABACContextTable InitVal[3];
        void Init(const PPS& pps, const Slice& sh);
    };

    class Packer
        : public FeatureBase
    {
    public:
#define DECL_BLOCK_LIST\
    DECL_BLOCK(Init) \
    DECL_BLOCK(Reset) \
    DECL_BLOCK(SubmitTask) \
    DECL_BLOCK(InsertSuffixSEI)
#define DECL_FEATURE_NAME "Base_Packer"
#include "hevcehw_decl_blocks.h"

        Packer(mfxU32 id)
            : FeatureBase(id)
        {}

    //protected:
        static const mfxU32 RBSP_SIZE = 1024;
        static const mfxU32 AUD_ES_SIZE = 8;
        static const mfxU32 VPS_ES_SIZE = 256;
        static const mfxU32 SPS_ES_SIZE = 1024;
        static const mfxU32 PPS_ES_SIZE = 128;
        static const mfxU32 SSH_ES_SIZE = MAX_SLICES * 16;
        static const mfxU32 ES_SIZE =
            AUD_ES_SIZE * 3
            + VPS_ES_SIZE
            + SPS_ES_SIZE
            + PPS_ES_SIZE
            + SSH_ES_SIZE;
        std::array<mfxU8, RBSP_SIZE> m_rbsp;
        std::array<mfxU8, ES_SIZE> m_es;
        mfxU8 *m_pRTBufBegin = nullptr
            , *m_pRTBufEnd = nullptr;
        NotNull<StorageW*> m_pGlob;


        virtual void InitAlloc(const FeatureBlocks& blocks, TPushIA Push) override;
        virtual void ResetState(const FeatureBlocks& blocks, TPushRS Push) override;
        virtual void SubmitTask(const FeatureBlocks& blocks, TPushST Push) override;
        virtual void QueryTask(const FeatureBlocks& blocks, TPushQT Push) override;

        using FeatureBase::Reset;
        mfxStatus Reset(
            const VPS& vps
            , const SPS& sps
            , const PPS& pps
            , const std::vector<SliceInfo>& si
            , PackedHeaders& ph);

        mfxU32 GetPrefixSEI(
            const ExtBuffer::Param<mfxVideoParam> & par
            , const TaskCommonPar & task
            , const SPS& sps
            , mfxU8* buf
            , mfxU32 sizeInBytes);

        mfxU32 GetSuffixSEI(
            const TaskCommonPar & task
            , mfxU8* buf
            , mfxU32 sizeInBytes);

        mfxU32 GetPSEIAndSSH(
            const ExtBuffer::Param<mfxVideoParam>& par
            , const TaskCommonPar& task
            , const SPS& sps
            , const PPS& pps
            , Slice sh
            , const std::vector<SliceInfo>& si
            , PackedHeaders& ph
            , mfxU8* buf
            , mfxU32 sizeInBytes);

        void PackBPPayload(
            BitstreamWriter& rbsp
            , const TaskCommonPar & task
            , const SPS& sps);
        void PackPTPayload(
            BitstreamWriter& rbsp
            , const ExtBuffer::Param<mfxVideoParam> & par
            , const TaskCommonPar & task
            , const SPS& sps);

        void PackNALU (BitstreamWriter& bs, NALU  const &  nalu);
        void PackAUD  (BitstreamWriter& bs, mfxU8 pic_type);
        void PackVPS  (BitstreamWriter& bs, VPS const & vps);
        void PackSPS  (BitstreamWriter& bs, SPS const & sps);
        void PackPPS  (BitstreamWriter& bs, PPS const & pps);
        void PackVUI  (BitstreamWriter& bs, VUI        const & vui, mfxU16 max_sub_layers_minus1);
        void PackHRD  (BitstreamWriter& bs, HRDInfo    const & hrd, bool commonInfPresentFlag, mfxU16 maxNumSubLayersMinus1);
        void PackPTL  (BitstreamWriter& bs, LayersInfo const & ptl, mfxU16 max_sub_layers_minus1);
        void PackSLO  (BitstreamWriter& bs, LayersInfo const & slo, mfxU16 max_sub_layers_minus1);
        mfxU32 PackSLD(BitstreamWriter& bs, ScalingList const & scl);

        void PackSSH(
            BitstreamWriter& bs
            , NALU  const & nalu
            , SPS   const & sps
            , PPS   const & pps
            , Slice const & slice
            , bool dyn_slice_size = false);

        void PackSkipSSD(
            BitstreamWriter& bs
            , SPS   const & sps
            , PPS   const & pps
            , Slice const & slice
            , mfxU32 NumLCU);

        void PackSEIPayload(BitstreamWriter& bs, VUI const & vui, BufferingPeriodSEI const & bp);
        static void PackSEIPayload(BitstreamWriter& bs, VUI const & vui, PicTimingSEI const & pt);
        static mfxU32 PackRBSP(mfxU8* dst, mfxU8* src, mfxU32 dst_size, mfxU32 src_size);
        static mfxStatus PackHeader(BitstreamWriter& rbsp, mfxU8* pESBegin, mfxU8* pESEnd, PackedData& d);

        static bool PutUE   (BitstreamWriter& bs, mfxU32 b) { bs.PutUE(b); return true; };
        static bool PutSE   (BitstreamWriter& bs, mfxI32 b) { bs.PutSE(b); return true; };
        static bool PutBit  (BitstreamWriter& bs, mfxU32 b) { bs.PutBit(!!b); return true; };
        static bool PutBits (BitstreamWriter& bs, mfxU32 n, mfxU32 b) { if (n) bs.PutBits(n, b); return !!n; };

        static bool PackSSHPWT(
            BitstreamWriter& bs
            , const SPS&     sps
            , const PPS&     pps
            , const Slice&   slice);

        static void PackSSHPartPB(
            BitstreamWriter& bs
            , const SPS   &  sps
            , const PPS   &  pps
            , const Slice &  slice);

        static void PackSSHPartIndependent(
            BitstreamWriter& bs
            , const NALU  &  nalu
            , const SPS   &  sps
            , const PPS   &  pps
            , const Slice &  slice);

        static void PackSSHPartNonIDR(
            BitstreamWriter& bs
            , const SPS   &  sps
            , const Slice &  slice);

        void PackSSHPartIdAddr(
            BitstreamWriter& bs
            , const NALU  &  nalu
            , const SPS   &  sps
            , const PPS   &  pps
            , const Slice &  slice);
    };

} //Base
} //namespace HEVCEHW

#endif
