// Copyright (c) 2019-2021 Intel Corporation
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

#include "mfx_common.h"
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE)

#include "hevcehw_base_interlace.h"
#include "hevcehw_base_packer.h"
#include "hevcehw_base_legacy.h"
#include <numeric>

using namespace HEVCEHW;
using namespace HEVCEHW::Base;

void Interlace::Query1NoCaps(const FeatureBlocks& , TPushQ1 Push)
{
    Push(BLK_SetDefaultsCallChain,
        [this](const mfxVideoParam&, mfxVideoParam&, StorageRW& strg) -> mfxStatus
    {
        auto& defaults = Glob::Defaults::GetOrConstruct(strg);
        auto& bSet = defaults.SetForFeature[GetID()];
        MFX_CHECK(!bSet, MFX_ERR_NONE);

        defaults.GetMinRefForBPyramid.Push(
            [](Defaults::TChain<mfxU16>::TExt prev
                , const Defaults::Param& par)
        {
            bool bField = IsField(par.mvp.mfx.FrameInfo.PicStruct);
            return mfxU16(prev(par) * (1 + bField) + bField);
        });

        defaults.GetMinRefForBNoPyramid.Push(
            [](Defaults::TChain<mfxU16>::TExt prev
                , const Defaults::Param& par)
        {
            bool bField = IsField(par.mvp.mfx.FrameInfo.PicStruct);
            return mfxU16(prev(par) * (1 + bField));
        });

        defaults.GetNumReorderFrames.Push(
            [](Defaults::TChain<mfxU8>::TExt prev
                , const Defaults::Param& par)
        {
            bool bField = IsField(par.mvp.mfx.FrameInfo.PicStruct);
            return mfxU8(prev(par) * (1 + bField));
        });

        defaults.GetNonStdReordering.Push(
            [](Defaults::TChain<bool>::TExt prev
                , const Defaults::Param& par)
        {
            return
                (IsField(par.mvp.mfx.FrameInfo.PicStruct)
                    && par.mvp.mfx.EncodedOrder
                    && par.mvp.mfx.NumRefFrame
                    && par.mvp.mfx.GopRefDist > 1
                    && par.base.GetMinRefForBNoPyramid(par) > par.mvp.mfx.NumRefFrame)
                || prev(par);
        });

        defaults.GetNumRefPPyramid.Push(
            [](Defaults::TChain<mfxU16>::TExt prev
                , const Defaults::Param& par)
        {
            mfxU16 k = 1 + IsField(par.mvp.mfx.FrameInfo.PicStruct);
            return mfxU16(prev(par) * k);
        });

        defaults.GetNumRefNoPyramid.Push(
            [](Defaults::TChain<mfxU16>::TExt prev
                , const Defaults::Param& par)
        {
            return mfxU16(prev(par) * (1 + IsField(par.mvp.mfx.FrameInfo.PicStruct)));
        });

        defaults.GetFrameType.Push(
            [](Defaults::TGetFrameType::TExt prev
                , const Defaults::Param& par
                , mfxU32 fo
                , mfxU32 lastIDR)
        {
            bool   bField       = IsField(par.mvp.mfx.FrameInfo.PicStruct);
            mfxU32 dFO          = bField * (lastIDR % 2);
            mfxU32 nPicInFrame  = bField + 1;

            auto   ft = prev(par, (fo + dFO - lastIDR) / nPicInFrame,  0);

            bool   b2ndField    = bField && ((fo + dFO - lastIDR) % 2);
            bool   bForceP      = b2ndField && IsI(ft);

            ft &= ~((MFX_FRAMETYPE_I | MFX_FRAMETYPE_IDR) * bForceP);
            ft |= MFX_FRAMETYPE_P * bForceP;

            bool bForceRef = bField && !b2ndField && !IsB(ft);
            ft |= MFX_FRAMETYPE_REF * bForceRef;
            return ft;
        });

        defaults.GetPicTimingSEI.Push(
            [](Defaults::TChain<mfxU16>::TExt prev
            , const Defaults::Param& par)
        {
            return Bool2CO(IsField(par.mvp.mfx.FrameInfo.PicStruct) || IsOn(prev(par)));
        });

        defaults.GetPLayer.Push(
            [](Defaults::TGetPLayer::TExt prev
                , const Defaults::Param& par
                , mfxU32 fo)
        {
            return prev(par, fo / (1 + IsField(par.mvp.mfx.FrameInfo.PicStruct)));
        });

        defaults.GetTId.Push(
            [](Defaults::TGetTId::TExt prev
                , const Defaults::Param& par
                , mfxU32 fo)
        {
            return prev(par, fo / (1 + IsField(par.mvp.mfx.FrameInfo.PicStruct)));
        });

        defaults.GetRPL.Push( [](
            Defaults::TGetRPL::TExt prev
            , const Defaults::Param& par
            , const DpbArray &DPB
            , mfxU16 maxL0
            , mfxU16 maxL1
            , const FrameBaseInfo& cur
            , mfxU8(&RPL)[2][MAX_DPB_SIZE]) -> std::tuple<mfxU8, mfxU8>
        {
            if (!IsField(par.mvp.mfx.FrameInfo.PicStruct))
                return prev(par, DPB, maxL0, maxL1, cur, RPL);

            auto nRefLX = prev(par, DPB, MAX_DPB_SIZE, MAX_DPB_SIZE, cur, RPL);
            auto& l0    = std::get<0>(nRefLX);
            auto& l1    = std::get<1>(nRefLX);

            bool bValid = l0 <= maxL0 && l1 <= maxL1;
            if (!bValid)
            {
                std::list<mfxU8> L0(RPL[0], RPL[0] + l0);
                std::list<mfxU8> L1(RPL[1], RPL[1] + l1);
                auto CmpRef = [&](mfxU8 a, mfxU8 b)
                {
                    auto FN = [](const FrameBaseInfo& x) { return (x.POC + !x.b2ndField) / 2; };
                    auto aDist = abs(FN(DPB[a]) - FN(cur)) * 2 + (DPB[a].bBottomField != cur.bBottomField);
                    auto bDist = abs(FN(DPB[b]) - FN(cur)) * 2 + (DPB[b].bBottomField != cur.bBottomField);
                    return aDist <= bDist;
                };
                auto IsNotInL0 = [&](mfxU8 x) { return std::find(L0.begin(), L0.end(), x) == L0.end(); };
                auto IsNotInL1 = [&](mfxU8 x) { return std::find(L1.begin(), L1.end(), x) == L1.end(); };

                L0.sort(CmpRef);
                L1.sort(CmpRef);

                L0.resize(std::min<mfxU16>(maxL0, l0));
                L1.resize(std::min<mfxU16>(maxL1, l1));

                std::remove_if(RPL[0], RPL[0] + l0, IsNotInL0);
                std::remove_if(RPL[1], RPL[1] + l1, IsNotInL1);

                l0 = mfxU8(L0.size());
                l1 = mfxU8(L1.size());

                std::fill(std::next(RPL[0], l0), std::end(RPL[0]), IDX_INVALID);
                std::fill(std::next(RPL[1], l1), std::end(RPL[1]), IDX_INVALID);
            }

            return nRefLX;
        });

        defaults.GetPreReorderInfo.Push([](
            Defaults::TGetPreReorderInfo::TExt  prev
            , const Defaults::Param&            par
            , FrameBaseInfo&                    fi
            , const mfxFrameSurface1*           pSurfIn
            , const mfxEncodeCtrl*              pCtrl
            , mfxU32                            prevIDROrder
            , mfxI32                            prevIPOC
            , mfxU32                            frameOrder)
        {
            auto sts = prev(par, fi, pSurfIn, pCtrl, prevIDROrder, prevIPOC, frameOrder);
            MFX_CHECK_STS(sts);

            MFX_CHECK(IsField(par.mvp.mfx.FrameInfo.PicStruct), MFX_ERR_NONE);

            bool bInfoValid = pSurfIn && !!(pSurfIn->Info.PicStruct & (MFX_PICSTRUCT_FIELD_TFF | MFX_PICSTRUCT_FIELD_BFF));
            fi.b2ndField = !!(frameOrder & 1);
            fi.bBottomField =
                (bInfoValid && IsBFF(pSurfIn->Info.PicStruct))
                || (!bInfoValid && (IsBFF(par.mvp.mfx.FrameInfo.PicStruct) != fi.b2ndField));

            return MFX_ERR_NONE;
        });

        // min 2 active L0 refs for 2nd field
        defaults.GetFrameNumRefActive.Push([](
            Defaults::TGetFrameNumRefActive::TExt prev
            , const Defaults::Param& par
            , const FrameBaseInfo& fi)
        {
            auto  nRef      = prev(par, fi);
            auto& nL0       = std::get<0>(nRef);
            bool  bSetMinL0 = nL0 && fi.b2ndField && nL0 < 2;

            SetIf(nL0, bSetMinL0, mfxU8(2));

            return nRef;
        });

        defaults.GetSHNUT.Push([](
            Defaults::TGetSHNUT::TExt prev
            , const Defaults::Param& par
            , const TaskCommonPar& task
            , bool bRAPIntra)
        {
            return prev(par, task, bRAPIntra && !IsField(par.mvp.mfx.FrameInfo.PicStruct));
        });

        defaults.GetVPS.Push([](
            Defaults::TGetVPS::TExt prev
            , const Defaults::Param& par
            , VPS& vps)
        {
            auto sts = prev(par, vps);

            vps.general.progressive_source_flag    = !!(par.mvp.mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_PROGRESSIVE);
            vps.general.interlaced_source_flag     = !(par.mvp.mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_PROGRESSIVE);
            vps.general.frame_only_constraint_flag = !IsField(par.mvp.mfx.FrameInfo.PicStruct);

            return sts;
        });

        defaults.GetSPS.Push([](
            Defaults::TGetSPS::TExt prev
            , const Defaults::Param& par
            , const VPS& vps
            , SPS& sps)
        {
            auto  sts = prev(par, vps, sps);
            auto& mfx = par.mvp.mfx;

            MFX_CHECK(IsField(mfx.FrameInfo.PicStruct) && sts >= MFX_ERR_NONE, sts);

            sps.log2_max_pic_order_cnt_lsb_minus4 =
                mfx::clamp<mfxU32>(
                    CeilLog2(mfx.GopRefDist * 2 + sps.sub_layer[sps.max_sub_layers_minus1].max_dec_pic_buffering_minus1) - 1
                    , sps.log2_max_pic_order_cnt_lsb_minus4
                    , 12u);
            sps.vui.frame_field_info_present_flag = 1;
            sps.vui.field_seq_flag                = 1;

            return sts;
        });

        defaults.GetWeakRef.Push([](
            Defaults::TGetWeakRef::TExt prev
            , const Defaults::Param& par
            , const FrameBaseInfo  &cur
            , const DpbFrame       *begin
            , const DpbFrame       *end)
        {
            if (!IsField(par.mvp.mfx.FrameInfo.PicStruct))
            {
                return prev(par, cur, begin, end);
            }

            const mfxExtCodingOption3& CO3 = ExtBuffer::Get(par.mvp);
            auto POCLess = [](const DpbFrame& l, const DpbFrame& r) { return l.POC < r.POC; };

            if (CO3.PRefType == MFX_P_REF_PYRAMID)
            {
                auto PyrInt = par.base.GetPPyrInterval(par);
                auto FN     = [](const FrameBaseInfo& x) { return (x.POC + !x.b2ndField) / 2; };
                auto IsWeak = [&](const FrameBaseInfo& x) { return (FN(x) - FN(*begin)) % PyrInt != 0; };

                if (FN(begin[1]) == FN(begin[0]))
                {
                    return std::find_if(begin, end, IsWeak);
                }

                return begin;
            }

            return std::min_element(begin, end, POCLess);
        });

        bSet = true;

        return MFX_ERR_NONE;
    });
}

void Interlace::Query1WithCaps(const FeatureBlocks&, TPushQ1 Push)
{
    Push(BLK_CheckPicStruct,
        [](const mfxVideoParam&, mfxVideoParam& par, StorageRW&) -> mfxStatus
    {
        mfxExtCodingOption2* pCO2 = ExtBuffer::Get(par);
        bool bInterlaceAllowed =
            par.mfx.RateControlMethod == MFX_RATECONTROL_CQP
            || (   pCO2
                && IsOn(pCO2->ExtBRC)
                && (   par.mfx.RateControlMethod == MFX_RATECONTROL_CBR
                    || par.mfx.RateControlMethod == MFX_RATECONTROL_VBR));

        bool bChanged = CheckOrZero<mfxU16>(
            par.mfx.FrameInfo.PicStruct
            , mfxU16(MFX_PICSTRUCT_UNKNOWN)
            , mfxU16(MFX_PICSTRUCT_PROGRESSIVE)
            , mfxU16(bInterlaceAllowed * MFX_PICSTRUCT_FIELD_TOP)
            , mfxU16(bInterlaceAllowed * MFX_PICSTRUCT_FIELD_BOTTOM)
            , mfxU16(bInterlaceAllowed * MFX_PICSTRUCT_FIELD_SINGLE)
            , mfxU16(bInterlaceAllowed * MFX_PICSTRUCT_FIELD_TFF)
            , mfxU16(bInterlaceAllowed * MFX_PICSTRUCT_FIELD_BFF));

        MFX_CHECK(!bChanged, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
        return MFX_ERR_NONE;
    });
}

void Interlace::QueryIOSurf(const FeatureBlocks&, TPushQIS Push)
{
    Push(BLK_QueryIOSurf
        , [](const mfxVideoParam&, mfxFrameAllocRequest& req, StorageRW& strg) -> mfxStatus
    {
        auto& par  = Glob::VideoParam::Get(strg);
        mfxU16 nExtraRaw = IsField(par.mfx.FrameInfo.PicStruct) * (par.mfx.GopRefDist - 1);

        req.NumFrameMin += nExtraRaw;
        req.NumFrameSuggested += nExtraRaw;

        return MFX_ERR_NONE;
    });
}

using TTaskItWrap = TaskItWrap<FrameBaseInfo, Task::Common::Key>;

static bool IsL1Ready(
    DpbArray const & dpb,
    mfxI32 poc,
    TTaskItWrap begin,
    TTaskItWrap end,
    bool flush)
{
    std::list<const DpbFrame*> bwd(Size(dpb), nullptr);
    std::iota(bwd.begin(), bwd.end(), std::begin(dpb));

    bwd.remove_if([&](const DpbFrame* p) { return !isValid(*p) || p->POC < poc; });

    if (bwd.size() != 1)
        return !bwd.empty();

    auto top = std::find_if_not(begin, end
        , [&](const FrameBaseInfo& f) { return f.POC <= bwd.back()->POC; });

    bool bRes = (top != end && IsB(top->FrameType))
        || (top == end && flush);

    return bRes;
}

template <class T>
static T IntBPyrReorder(T begin, T end)
{
    typedef typename std::iterator_traits<T>::reference TRef;

    mfxU32 num = mfxU32(std::distance(begin, end));
    bool bSetOrder = num && (*begin)->BPyramidOrder == mfxU32(MFX_FRAMEORDER_UNKNOWN);

    if (bSetOrder)
    {
        mfxU32 i = 0;
        std::for_each(begin, end, [&](TRef bref)
        {
            bool bRef = false;
            bref->BPyramidOrder = Legacy::GetBiFrameLocation(i++ / 2, num / 2, bRef, bref->PyramidLevel);
            bref->PyramidLevel *= 2;
            bref->FrameType |= mfxU16(MFX_FRAMETYPE_REF * (bRef || !bref->b2ndField));
        });
    }

    return std::min_element(begin, end
        , [](TRef a, TRef b) { return a->BPyramidOrder < b->BPyramidOrder; });
}

static TTaskItWrap IntReorder(
    mfxVideoParam const & par,
    DpbArray const & dpb,
    TTaskItWrap begin,
    TTaskItWrap end,
    bool flush)
{
    const mfxExtCodingOption2& CO2 = ExtBuffer::Get(par);
    bool bBPyramid = (CO2.BRefType == MFX_B_REF_PYRAMID);
    auto top = begin;
    std::list<TTaskItWrap> brefs;

    auto IsB      = [] (FrameBaseInfo& f){ return !!(f.FrameType & MFX_FRAMETYPE_B); };
    auto NoL1     = [&](TTaskItWrap f)   { return !IsL1Ready(dpb, f->POC, begin, end, flush); };
    auto NextBRef = [&]()                { return *IntBPyrReorder(brefs.begin(), brefs.end()); };

    std::generate_n(
        std::back_inserter(brefs)
        , std::distance(begin, std::find_if_not(begin, end, IsB))
        , [&]() { return top++; });

    brefs.remove_if(NoL1);

    bool bNoPyramidB = !bBPyramid && !brefs.empty();

    if (bNoPyramidB)
    {
        auto            topB        = brefs.begin();
        FrameBaseInfo*  pPrevB      = nullptr;
        auto            IsNextBRef  = [&](TTaskItWrap& f)
        {
            bool bRes = IsRef(f->FrameType) && (!pPrevB || ((f->POC - pPrevB->POC) < 3));
            pPrevB = &*f;
            return bRes;
        };
        auto ref = std::find_if(brefs.begin(), brefs.end(), IsNextBRef);

        SetIf(topB, ref != brefs.end(), ref);

        return *topB;
    }

    SetIf(top, !brefs.empty(), NextBRef);

    bool bForcePRef = flush && top == end && begin != end;
    while (bForcePRef)
    {
        --top;
        top->FrameType = mfxU16(MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF);
        bForcePRef = top != begin && top->b2ndField;
    }

    return top;
}

void Interlace::InitInternal(const FeatureBlocks&, TPushII Push)
{
    Push(BLK_SetReorder
        , [](StorageRW& strg, StorageRW&) -> mfxStatus
    {
        auto& par = Glob::VideoParam::Get(strg);
        MFX_CHECK(IsField(par.mfx.FrameInfo.PicStruct), MFX_ERR_NONE);

        auto& rdr = Glob::Reorder::Get(strg);
        rdr.BufferSize += par.mfx.GopRefDist - 1;
        rdr.MaxReorder += par.mfx.GopRefDist;
        rdr.Push([&](Reorderer::TExt, const DpbArray& DPB, TTaskIt begin, TTaskIt end, bool bFlush)
        {
            return IntReorder(par, DPB, begin, end, bFlush).it;
        });

        return MFX_ERR_NONE;
    });

    Push(BLK_PatchRawInfo
        , [](StorageRW& strg, StorageRW& local) -> mfxStatus
    {
        auto& par = Glob::VideoParam::Get(strg);
        MFX_CHECK(IsField(par.mfx.FrameInfo.PicStruct), MFX_ERR_NONE);

        Tmp::RawInfo::Get(local).NumFrameMin += par.mfx.GopRefDist - 1;

        return MFX_ERR_NONE;
    });
}

void Interlace::SubmitTask(const FeatureBlocks& , TPushST Push)
{
    Push(BLK_InsertPTSEI
        , [this](StorageW& global, StorageW& s_task) -> mfxStatus
    {
        auto& par = Glob::VideoParam::Get(global);
        auto PS = par.mfx.FrameInfo.PicStruct;
        MFX_CHECK(IsField(PS), MFX_ERR_NONE);

        auto& task = Task::Common::Get(s_task);
        MFX_CHECK(task.InsertHeaders & INSERT_PTSEI, MFX_ERR_NONE);

        auto usrPlEnd = task.ctrl.Payload + task.ctrl.NumPayload;
        bool bUserPTSEI = usrPlEnd != std::find_if(task.ctrl.Payload, usrPlEnd
            , [](const mfxPayload* pPl) { return pPl && pPl->Type == 1; });
        MFX_CHECK(!bUserPTSEI, MFX_ERR_NONE);

        auto& sps = Glob::SPS::Get(global);
        static const std::map<mfxU16, mfxU8> PicStructMFX2STD =
        {
              {mfxU16(MFX_PICSTRUCT_FIELD_TOP), mfxU8(1)}
            , {mfxU16(MFX_PICSTRUCT_FIELD_BOTTOM), mfxU8(2)}
            , {mfxU16(MFX_PICSTRUCT_FIELD_TOP | MFX_PICSTRUCT_FIELD_PAIRED_PREV), mfxU8(9)}
            , {mfxU16(MFX_PICSTRUCT_FIELD_TOP | MFX_PICSTRUCT_FIELD_PAIRED_NEXT), mfxU8(11)}
            , {mfxU16(MFX_PICSTRUCT_FIELD_BOTTOM | MFX_PICSTRUCT_FIELD_PAIRED_PREV), mfxU8(10)}
            , {mfxU16(MFX_PICSTRUCT_FIELD_BOTTOM | MFX_PICSTRUCT_FIELD_PAIRED_NEXT), mfxU8(12)}
        };
        PicTimingSEI pt = {};
        pt.pic_struct = 1 + task.bBottomField;

        if (PicStructMFX2STD.count(task.pSurfIn->Info.PicStruct))
            pt.pic_struct = PicStructMFX2STD.at(task.pSurfIn->Info.PicStruct);

        pt.source_scan_type = 0;
        pt.duplicate_flag = 0;

        pt.au_cpb_removal_delay_minus1 = std::max<mfxU32>(task.cpb_removal_delay, 1) - 1;
        pt.pic_dpb_output_delay =
            task.DisplayOrder
            + sps.sub_layer[sps.max_sub_layers_minus1].max_num_reorder_pics
            - task.EncodedOrder;

        BitstreamWriter bs(m_buf.data(), (mfxU32)m_buf.size(), 0);
        mfxPayload pl = {};
        pl.Type = 1;
        pl.Data = bs.GetStart();

        bs.PutBits(8, 1);    //payload type
        bs.PutBits(8, 0xff); //place for payload size

        Packer::PackSEIPayload(bs, sps.vui, pt);

        pl.NumBit  = bs.GetOffset();
        pl.BufSize = mfxU16(CeilDiv(pl.NumBit, 8u));

        assert(pl.BufSize < 256);
        pl.Data[1] = (mfxU8)pl.BufSize - 2; //payload size

        task.PLInternal.push_back(pl);

        return MFX_ERR_NONE;
    });

    Push(BLK_SkipFrame
        , [this](StorageW& global, StorageW& s_task) -> mfxStatus
    {
        auto& par = Glob::VideoParam::Get(global);
        auto& task = Task::Common::Get(s_task);
        auto& allocRec = Glob::AllocRec::Get(global);

        //skip second field when the first one is skipped
        bool b2ndFieldSkip =
            !task.bSkip
            && task.b2ndField
            && Legacy::IsSWBRC(par, ExtBuffer::Get(par))
            && !!(allocRec.GetFlag(task.DPB.Active[task.RefPicList[0][0]].Rec.Idx) & REC_SKIPPED);

        task.bSkip |= b2ndFieldSkip;
        m_b2ndFieldRecode = b2ndFieldSkip && !(!!(allocRec.GetFlag(task.DPB.Active[task.RefPicList[0][0]].Rec.Idx) & REC_READY));

        return MFX_ERR_NONE;
    });
}

void Interlace::QueryTask(const FeatureBlocks&, TPushQT Push)
{
    Push(BLK_QueryTask
        , [this](StorageW& /*global*/, StorageW& s_task) -> mfxStatus
    {
        MFX_CHECK(m_b2ndFieldRecode, MFX_ERR_NONE);

        m_b2ndFieldRecode = false;
        auto& task = Task::Common::Get(s_task);
        task.bRecode = true;

        return MFX_ERR_NONE;
    });
}


#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
