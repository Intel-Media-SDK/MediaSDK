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

namespace HEVCEHW
{
namespace Base
{
    class BitstreamReader
        : public IBsReader
    {
    public:
        BitstreamReader(mfxU8* bs, mfxU32 size, mfxU8 bitOffset = 0);
        ~BitstreamReader();

        virtual mfxU32 GetBit() override;
        virtual mfxU32 GetBits(mfxU32 n) override;
        virtual mfxU32 GetUE() override;
        virtual mfxI32 GetSE() override;

        mfxU32 GetOffset() { return (mfxU32)(m_bs - m_bsStart) * 8 + m_bitOffset - m_bitStart; }
        mfxU8* GetStart() { return m_bsStart; }

        void SetEmulation(bool f) { m_emulation = f; };
        bool GetEmulation() { return m_emulation; };

        void Reset(mfxU8* bs = 0, mfxU32 size = 0, mfxU8 bitOffset = 0);

    private:
        mfxU8 * m_bsStart;
        mfxU8* m_bsEnd;
        mfxU8* m_bs;
        mfxU8  m_bitStart;
        mfxU8  m_bitOffset;
        bool   m_emulation;
    };

    class Parser
        : public FeatureBase
    {
    public:
#define DECL_BLOCK_LIST\
    DECL_BLOCK(LoadSPSPPS)
#define DECL_FEATURE_NAME "Base_Parser"
#include "hevcehw_decl_blocks.h"

        Parser(mfxU32 id)
            : FeatureBase(id)
        {}

    protected:
        std::function<bool(const PTL&)> m_needRextConstraints;
        std::function<bool(const SPS&, mfxU8, IBsReader&)> m_readSpsExt;
        std::function<bool(const PPS&, mfxU8, IBsReader&)> m_readPpsExt;

        virtual void Query1NoCaps(const FeatureBlocks& blocks, TPushQ1 Push) override;

        mfxStatus LoadSPSPPS(mfxVideoParam& par, SPS & sps, PPS & pps);

        mfxStatus ParseNALU(BitstreamReader& bs, NALU & nalu);
        mfxStatus ParseSPS(BitstreamReader& bs, SPS & sps);
        mfxStatus ParsePPS(BitstreamReader& bs, PPS & pps);

        mfxStatus SpsToMFX(const SPS& sps, mfxVideoParam& par);
        mfxStatus PpsToMFX(const PPS& pps, mfxVideoParam& par);
    };

} //Base
} //namespace HEVCEHW

#endif
