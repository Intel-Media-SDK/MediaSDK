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
#include "hevcehw_ddi.h"
#include "hevcehw_base_data.h"
#include <tuple>

namespace HEVCEHW
{
namespace Base
{
    enum FType { // Frame type for GetMaxNumRef tuple
        P = 0,
        BL0 = 1,
        BL1 = 2
    };

    class Legacy
        : public FeatureBase
    {
    public:

#define DECL_BLOCK_LIST\
    DECL_BLOCK(Query0               )\
    DECL_BLOCK(SetDefaultsCallChain )\
    DECL_BLOCK(PreCheckCodecId      )\
    DECL_BLOCK(PreCheckChromaFormat )\
    DECL_BLOCK(PreCheckExtBuffers   )\
    DECL_BLOCK(CopyConfigurable     )\
    DECL_BLOCK(SetLowPowerDefault   )\
    DECL_BLOCK(SetGUID              )\
    DECL_BLOCK(CheckHeaders         )\
    DECL_BLOCK(CheckLCUSize         )\
    DECL_BLOCK(CheckFormat          )\
    DECL_BLOCK(CheckLowDelayBRC     )\
    DECL_BLOCK(CheckLevel           )\
    DECL_BLOCK(CheckSurfSize        )\
    DECL_BLOCK(CheckCodedPicSize    )\
    DECL_BLOCK(CheckTiles           )\
    DECL_BLOCK(CheckTU              )\
    DECL_BLOCK(CheckTemporalLayers  )\
    DECL_BLOCK(CheckGopRefDist      )\
    DECL_BLOCK(CheckNumRefFrame     )\
    DECL_BLOCK(CheckIOPattern       )\
    DECL_BLOCK(CheckBRC             )\
    DECL_BLOCK(CheckCrops           )\
    DECL_BLOCK(CheckShift           )\
    DECL_BLOCK(CheckFrameRate       )\
    DECL_BLOCK(CheckSlices          )\
    DECL_BLOCK(CheckBPyramid        )\
    DECL_BLOCK(CheckPPyramid        )\
    DECL_BLOCK(CheckIntraRefresh    )\
    DECL_BLOCK(CheckSkipFrame       )\
    DECL_BLOCK(CheckGPB             )\
    DECL_BLOCK(CheckESPackParam     )\
    DECL_BLOCK(CheckNumRefActive    )\
    DECL_BLOCK(CheckSAO             )\
    DECL_BLOCK(CheckProfile         )\
    DECL_BLOCK(CheckLevelConstraints)\
    DECL_BLOCK(CheckVideoParam      )\
    DECL_BLOCK(SetDefaults          )\
    DECL_BLOCK(SetFrameAllocRequest )\
    DECL_BLOCK(SetVPS               )\
    DECL_BLOCK(SetSPS               )\
    DECL_BLOCK(NestSTRPS            )\
    DECL_BLOCK(SetSTRPS             )\
    DECL_BLOCK(SetPPS               )\
    DECL_BLOCK(SetSlices            )\
    DECL_BLOCK(SetReorder           )\
    DECL_BLOCK(AllocRaw             )\
    DECL_BLOCK(AllocBS              )\
    DECL_BLOCK(AllocMBQP            )\
    DECL_BLOCK(ResetInit            )\
    DECL_BLOCK(ResetCheck           )\
    DECL_BLOCK(ResetState           )\
    DECL_BLOCK(CheckSurf            )\
    DECL_BLOCK(CheckBS              )\
    DECL_BLOCK(AllocTask            )\
    DECL_BLOCK(InitTask             )\
    DECL_BLOCK(PrepareTask          )\
    DECL_BLOCK(ConfigureTask        )\
    DECL_BLOCK(SkipFrame            )\
    DECL_BLOCK(GetRawHDL            )\
    DECL_BLOCK(CopySysToRaw         )\
    DECL_BLOCK(FillCUQPSurf         )\
    DECL_BLOCK(CopyBS               )\
    DECL_BLOCK(DoPadding            )\
    DECL_BLOCK(UpdateBsInfo         )\
    DECL_BLOCK(SetRawInfo           )\
    DECL_BLOCK(FreeTask             )
#define DECL_FEATURE_NAME "Base_Legacy"
#include "hevcehw_decl_blocks.h"

        Legacy(mfxU32 id)
            : FeatureBase(id)
        {
        }

    protected:
        TaskCommonPar m_prevTask;
        mfxU32
            m_frameOrder        = mfxU32(-1)
            , m_lastIDR         = 0
            , m_baseLayerOrder  = 0
            , m_forceHeaders    = 0;
        mfxU16
            m_CUQPBlkW          = 0
            , m_CUQPBlkH        = 0;
        std::function<std::tuple<mfxU16, mfxU16, mfxU16>(const mfxVideoParam&)> m_GetMaxRef;
        std::unique_ptr<Defaults::Param> m_pQWCDefaults;
        NotNull<Defaults*> m_pQNCDefaults;
        eMFXHWType m_hw = MFX_HW_UNKNOWN;

        void ResetState()
        {
            m_frameOrder     = mfxU32(-1);
            m_lastIDR        = 0;
            m_baseLayerOrder = 0;
            Invalidate(m_prevTask);
        }

    public:
        virtual void SetSupported(ParamSupport& par) override;
        virtual void SetInherited(ParamInheritance& par) override;
        virtual void Query0(const FeatureBlocks& blocks, TPushQ0 Push) override;
        virtual void Query1NoCaps(const FeatureBlocks& blocks, TPushQ1 Push) override;
        virtual void Query1WithCaps(const FeatureBlocks& blocks, TPushQ1 Push) override;
        virtual void SetDefaults(const FeatureBlocks& blocks, TPushSD Push) override;
        virtual void QueryIOSurf(const FeatureBlocks& blocks, TPushQIS Push) override;
        virtual void InitExternal(const FeatureBlocks& blocks, TPushIE Push) override;
        virtual void InitInternal(const FeatureBlocks& blocks, TPushII Push) override;
        virtual void InitAlloc(const FeatureBlocks& blocks, TPushIA Push) override;
        virtual void FrameSubmit(const FeatureBlocks& blocks, TPushFS Push) override;
        virtual void AllocTask(const FeatureBlocks& blocks, TPushAT Push) override;
        virtual void InitTask(const FeatureBlocks& blocks, TPushIT Push) override;
        virtual void PreReorderTask(const FeatureBlocks& blocks, TPushPreRT Push) override;
        virtual void PostReorderTask(const FeatureBlocks& blocks, TPushPostRT Push) override;
        virtual void SubmitTask(const FeatureBlocks& blocks, TPushST Push) override;
        virtual void QueryTask(const FeatureBlocks& blocks, TPushQT Push) override;
        virtual void FreeTask(const FeatureBlocks& blocks, TPushFT Push) override;
        virtual void GetVideoParam(const FeatureBlocks& blocks, TPushGVP Push) override;
        virtual void Reset(const FeatureBlocks& blocks, TPushR Push) override;
        virtual void ResetState(const FeatureBlocks& blocks, TPushRS Push) override;

        static void PushDefaults(Defaults& df);

        void CheckQuery0(const ParamSupport& sprt, mfxVideoParam& par);
        mfxStatus CheckBuffers(const ParamSupport& sprt, const mfxVideoParam& in, const mfxVideoParam* out = nullptr);
        mfxStatus CopyConfigurable(const ParamSupport& sprt, const mfxVideoParam& in, mfxVideoParam& out);

        mfxStatus CheckCodedPicSize(
            mfxVideoParam & par
            , const Defaults::Param& defPar);
        mfxStatus CheckTiles(
            mfxVideoParam & par
            , const Defaults::Param& defPar);
        mfxStatus CheckTU(mfxVideoParam & par, const ENCODE_CAPS_HEVC& caps);
        mfxStatus CheckGopRefDist(mfxVideoParam & par, const ENCODE_CAPS_HEVC& caps);
        mfxStatus CheckIOPattern(mfxVideoParam & par);
        mfxStatus CheckBRC(
            mfxVideoParam & par
            , const Defaults::Param& defPar);
        mfxStatus CheckCrops(
            mfxVideoParam & par
            , const Defaults::Param& defPar);
        mfxStatus CheckShift(mfxVideoParam & par, mfxExtOpaqueSurfaceAlloc* pOSA);
        mfxStatus CheckFrameRate(mfxVideoParam & par);
        mfxStatus CheckNumRefFrame(
            mfxVideoParam & par
            , const Defaults::Param& defPar);
        mfxStatus CheckSlices(
            mfxVideoParam & par
            , const Defaults::Param& defPar);
        mfxStatus CheckBPyramid(
            mfxVideoParam & par
            , const Defaults::Param& defPar);
        mfxStatus CheckPPyramid(mfxVideoParam & par);
        mfxStatus CheckTemporalLayers(mfxVideoParam & par);
        mfxStatus CheckIntraRefresh(
            mfxVideoParam & par
            , const Defaults::Param& defPar);
        mfxStatus CheckSkipFrame(mfxVideoParam & par);
        mfxStatus CheckGPB(mfxVideoParam & par);
        mfxStatus CheckESPackParam(mfxVideoParam & par, eMFXHWType hw);
        mfxStatus CheckLevelConstraints(
            mfxVideoParam & par
            , const Defaults::Param& defPar);
        mfxStatus CheckSPS(const SPS& sps, const ENCODE_CAPS_HEVC& caps, eMFXHWType hw);
        mfxStatus CheckPPS(const PPS& sps, const ENCODE_CAPS_HEVC& caps, eMFXHWType hw);

        void SetDefaults(
            mfxVideoParam& par
            , const Defaults::Param& defPar
            , bool bExternalFrameAllocator);

        void SetSTRPS(
            const Defaults::Param& dflts
            , SPS& sps
            , const Reorderer& reorder);

        void ConfigureTask(
            TaskCommonPar & task
            , const Defaults::Param& dflts
            , const SPS& sps);
        mfxStatus GetSliceHeader(
            const ExtBuffer::Param<mfxVideoParam> & par
            , const TaskCommonPar& task
            , const SPS& sps
            , const PPS& pps
            , Slice & s);
        static mfxU32 GetRawBytes(mfxU16 w, mfxU16 h, mfxU16 ChromaFormat, mfxU16 BitDepth);
        static bool IsSWBRC(mfxVideoParam const & par, const mfxExtCodingOption2* pCO2);
        static bool IsInVideoMem(const mfxVideoParam & par, const mfxExtOpaqueSurfaceAlloc* pOSA);
        static bool IsTCBRC(const mfxVideoParam & par, mfxU16 tcbrcSupport);

        mfxU16 GetMaxRaw(const mfxVideoParam & par)
        {
            return par.AsyncDepth + (par.mfx.GopRefDist - 1) + (par.AsyncDepth > 1);
        }
        mfxU16 GetMaxBS(mfxVideoParam const & par)
        {
            return par.AsyncDepth + (par.AsyncDepth > 1);
        }
        mfxU32 GetMinBsSize(
            const mfxVideoParam & par
            , const mfxExtHEVCParam& HEVCParam
            , const mfxExtCodingOption2& CO2
            , const mfxExtCodingOption3& CO3);
        std::tuple<mfxStatus, mfxU16, mfxU16> GetCUQPMapBlockSize(
            mfxU16 frameWidth, mfxU16 frameHeight,
            mfxU16 CUQPWidth, mfxU16 CUHeight);

        void ConstructRPL(
            const Defaults::Param& dflts
            , const DpbArray & DPB
            , const FrameBaseInfo& cur
            , mfxU8(&RPL)[2][MAX_DPB_SIZE]
            , mfxU8(&numRefActive)[2]
            , const mfxExtAVCRefLists * pExtLists = nullptr
            , const mfxExtAVCRefListCtrl * pLCtrl = nullptr);

        void ConstructSTRPS(
            const DpbArray & DPB
            , const mfxU8(&RPL)[2][MAX_DPB_SIZE]
            , const mfxU8 (&numRefActive)[2]
            , mfxI32 poc
            , STRPS& rps);

        void InitDPB(
            TaskCommonPar &        task,
            TaskCommonPar const &  prevTask,
            const mfxExtAVCRefListCtrl* pLCtrl);

        mfxU16 UpdateDPB(
            const Defaults::Param& dflts
            , const DpbFrame& task
            , DpbArray & dpb
            , const mfxExtAVCRefListCtrl * pLCtrl = nullptr);

        static bool IsCurrRef(
            const DpbArray & DPB
            , const mfxU8 (&RPL)[2][MAX_DPB_SIZE]
            , const mfxU8 (&numRefActive)[2]
            , mfxI32 poc)
        {
            for (mfxU32 i = 0; i < 2; i++)
                for (mfxU32 j = 0; j < numRefActive[i]; j++)
                    if (poc == DPB[RPL[i][j]].POC)
                        return true;
            return false;
        }
        static mfxU8 GetDPBIdxByFO(DpbArray const & DPB, mfxU32 fo)
        {
            for (mfxU8 i = 0; !isDpbEnd(DPB, i); i++)
                if (DPB[i].DisplayOrder == fo)
                    return i;
            return mfxU8(MAX_DPB_SIZE);
        }
        static mfxU8 GetDPBIdxByPoc(DpbArray const & DPB, mfxI32 poc)
        {
            for (mfxU8 i = 0; !isDpbEnd(DPB, i); i++)
                if (DPB[i].POC == poc)
                    return i;
            return mfxU8(MAX_DPB_SIZE);
        }
        static mfxU32 GetBiFrameLocation(mfxU32 i, mfxU32 num, bool &ref, mfxU32 &level);
        Defaults::Param GetRTDefaults(StorageR& strg)
        {
            return Defaults::Param(
                Glob::VideoParam::Get(strg)
                , Glob::EncodeCaps::Get(strg)
                , m_hw
                , Glob::Defaults::Get(strg));
        }

        using TItWrap = TaskItWrap<FrameBaseInfo, Task::Common::Key>;
        using TItWrapIt = std::list<TItWrap>::iterator;

        static TItWrapIt BPyrReorder(TItWrapIt begin, TItWrapIt end);
        static TItWrap Reorder(
            ExtBuffer::Param<mfxVideoParam> const & par
            , DpbArray const & dpb
            , TItWrap begin
            , TItWrap end
            , bool flush);

    };

} //Base
} //namespace HEVCEHW

#endif
