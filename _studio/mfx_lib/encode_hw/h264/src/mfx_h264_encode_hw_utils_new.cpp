// Copyright (c) 2018-2020 Intel Corporation
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
#ifdef MFX_ENABLE_H264_VIDEO_ENCODE_HW

#include <algorithm>


#include "mfx_h264_encode_hw_utils.h"

using namespace MfxHwH264Encode;


namespace MfxHwH264Encode
{
    mfxI32 GetPicNum(ArrayDpbFrame const & dpb, mfxU8 ref)
    {
        return dpb[ref & 127].m_picNum[ref >> 7];
    }

    mfxI32 GetPicNumF(ArrayDpbFrame const & dpb, mfxU8 ref)
    {
        DpbFrame const & dpbFrame = dpb[ref & 127];
        return dpbFrame.m_refPicFlag[ref >> 7] ? dpbFrame.m_picNum[ref >> 7] : 0x20000;
    }

    mfxU8 GetLongTermPicNum(ArrayDpbFrame const & dpb, mfxU8 ref)
    {
        return dpb[ref & 127].m_longTermPicNum[ref >> 7];
    }

    mfxU32 GetLongTermPicNumF(ArrayDpbFrame const & dpb, mfxU8 ref)
    {
        DpbFrame const & dpbFrame = dpb[ref & 127];

        return dpbFrame.m_refPicFlag[ref >> 7] && dpbFrame.m_longterm
            ? dpbFrame.m_longTermPicNum[ref >> 7]
            : 0x20;
    }

    mfxI32 GetPoc(ArrayDpbFrame const & dpb, mfxU8 ref)
    {
        return dpb[ref & 127].m_poc[ref >> 7];
    }
};


namespace
{
    struct BasePredicateForRefPic
    {
        typedef ArrayDpbFrame Dpb;
        typedef mfxU8         Arg;
        typedef bool          Res;

        BasePredicateForRefPic(Dpb const & dpb) : m_dpb(dpb) {}

        void operator =(BasePredicateForRefPic const &);

        Dpb const & m_dpb;
    };

    struct RefPicNumIsGreater : public BasePredicateForRefPic
    {
        RefPicNumIsGreater(Dpb const & dpb) : BasePredicateForRefPic(dpb) {}

        bool operator ()(mfxU8 l, mfxU8 r) const
        {
            return GetPicNum(m_dpb, l) > GetPicNum(m_dpb, r);
        }
    };

    struct LongTermRefPicNumIsLess : public BasePredicateForRefPic
    {
        LongTermRefPicNumIsLess(Dpb const & dpb) : BasePredicateForRefPic(dpb) {}

        bool operator ()(mfxU8 l, mfxU8 r) const
        {
            return GetLongTermPicNum(m_dpb, l) < GetLongTermPicNum(m_dpb, r);
        }
    };

    struct RefPocIsLess : public BasePredicateForRefPic
    {
        RefPocIsLess(Dpb const & dpb) : BasePredicateForRefPic(dpb) {}

        bool operator ()(mfxU8 l, mfxU8 r) const
        {
            return GetPoc(m_dpb, l) < GetPoc(m_dpb, r);
        }
    };

    struct RefPocIsGreater : public BasePredicateForRefPic
    {
        RefPocIsGreater(Dpb const & dpb) : BasePredicateForRefPic(dpb) {}

        bool operator ()(mfxU8 l, mfxU8 r) const
        {
            return GetPoc(m_dpb, l) > GetPoc(m_dpb, r);
        }
    };

    struct RefPocIsLessThan : public BasePredicateForRefPic
    {
        RefPocIsLessThan(Dpb const & dpb, mfxI32 poc) : BasePredicateForRefPic(dpb), m_poc(poc) {}

        bool operator ()(mfxU8 r) const
        {
            return GetPoc(m_dpb, r) < m_poc;
        }

        mfxI32 m_poc;
    };

    struct RefPocIsGreaterThan : public BasePredicateForRefPic
    {
        RefPocIsGreaterThan(Dpb const & dpb, mfxI32 poc) : BasePredicateForRefPic(dpb), m_poc(poc) {}

        bool operator ()(mfxU8 r) const
        {
            return GetPoc(m_dpb, r) > m_poc;
        }

        mfxI32 m_poc;
    };

    struct RefIsShortTerm : public BasePredicateForRefPic
    {
        RefIsShortTerm(Dpb const & dpb) : BasePredicateForRefPic(dpb) {}

        bool operator ()(mfxU8 r) const
        {
            return m_dpb[r & 127].m_refPicFlag[r >> 7] && !m_dpb[r & 127].m_longterm;
        }
    };

    struct RefIsLongTerm : public BasePredicateForRefPic
    {
        RefIsLongTerm(Dpb const & dpb) : BasePredicateForRefPic(dpb) {}

        bool operator ()(mfxU8 r) const
        {
            return m_dpb[r & 127].m_refPicFlag[r >> 7] && m_dpb[r & 127].m_longterm;
        }
    };

    struct RefIsFromHigherTemporalLayer : public BasePredicateForRefPic
    {
        RefIsFromHigherTemporalLayer(Dpb const & dpb, mfxU32 tid) : BasePredicateForRefPic(dpb), m_tid(tid) {}

        bool operator ()(mfxU8 r) const
        {
            return m_dpb[r & 127].m_tid > m_tid;
        }

        mfxU32 m_tid;
    };

    bool RefListHasLongTerm(
        ArrayDpbFrame const & dpb,
        ArrayU8x33 const &    list)
    {
        return std::find_if(list.Begin(), list.End(), RefIsLongTerm(dpb)) != list.End();
    }

    template <class T, class U> struct LogicalAndHelper
    {
        typedef typename T::Arg Arg;
        typedef typename T::Res Res;
        T m_pr1;
        U m_pr2;

        LogicalAndHelper(T pr1, U pr2) : m_pr1(pr1), m_pr2(pr2) {}

        Res operator ()(Arg arg) const { return m_pr1(arg) && m_pr2(arg); }
    };

    template <class T, class U> LogicalAndHelper<T, U> LogicalAnd(T pr1, U pr2)
    {
        return LogicalAndHelper<T, U>(pr1, pr2);
    }

    template <class T> struct LogicalNotHelper
    {
        typedef typename T::argument_type Arg;
        typedef typename T::result_type   Res;
        T m_pr;

        LogicalNotHelper(T pr) : m_pr(pr) {}

        Res operator ()(Arg arg) const { return !m_pred(arg); }
    };

    template <class T> LogicalNotHelper<T> LogicalNot(T pr)
    {
        return LogicalNotHelper<T>(pr);
    }


    mfxU32 CountFutureRefs(
        ArrayDpbFrame const & dpb,
        mfxU32                currFrameOrder)
    {
        mfxU32 count = 0;
        for (mfxU32 i = 0; i < dpb.Size(); ++i)
            if (currFrameOrder < dpb[i].m_frameOrder)
                count++;
        return count;
    }
};

    void MfxHwH264Encode::UpdateDpbFrames(
        DdiTask & task,
        mfxU32    field,
        mfxU32    frameNumMax)
    {
        mfxU32 ps = task.GetPicStructForEncode();

        for (mfxU32 i = 0; i < task.m_dpb[field].Size(); i++)
        {
            DpbFrame & ref = task.m_dpb[field][i];

            if (ref.m_longTermIdxPlus1 > 0)
            {
                if (ps == MFX_PICSTRUCT_PROGRESSIVE)
                {
                    ref.m_longTermPicNum[0] = ref.m_longTermIdxPlus1 - 1;
                    ref.m_longTermPicNum[1] = ref.m_longTermIdxPlus1 - 1;
                }
                else
                {
                    ref.m_longTermPicNum[0] = 2 * (ref.m_longTermIdxPlus1 - 1) + mfxU8( !field);
                    ref.m_longTermPicNum[1] = 2 * (ref.m_longTermIdxPlus1 - 1) + mfxU8(!!field);
                }
            }
            else
            {
                ref.m_frameNumWrap = (ref.m_frameNum > task.m_frameNum)
                    ? ref.m_frameNum - frameNumMax
                    : ref.m_frameNum;

                // update picNum
                if (ps == MFX_PICSTRUCT_PROGRESSIVE)
                {
                    ref.m_picNum[0] = ref.m_frameNumWrap;
                    ref.m_picNum[1] = ref.m_frameNumWrap;
                }
                else
                {
                    ref.m_picNum[0] = 2 * ref.m_frameNumWrap + ( !field);
                    ref.m_picNum[1] = 2 * ref.m_frameNumWrap + (!!field);
                }
            }
        }
    }

namespace
{
    void ProcessFields(
        mfxU32                bottomPicFlag,
        ArrayDpbFrame const & dpb,
        ArrayU8x33 const &    picListFrm,
        ArrayU8x33 &          picListFld)
    {
        // 8.2.4.2.5 "Initialisation process for reference picture lists in fields"

        mfxU8 const * sameParity = picListFrm.Begin();
        mfxU8 const * oppParity  = picListFrm.Begin();

        picListFld.Resize(0);

        while (sameParity != picListFrm.End() || oppParity != picListFrm.End())
        {
            for (; sameParity != picListFrm.End(); sameParity++)
            {
                if (dpb[*sameParity & 127].m_refPicFlag[bottomPicFlag])
                {
                    picListFld.PushBack((*sameParity & 127) + mfxU8(bottomPicFlag << 7));
                    sameParity++;
                    break;
                }
            }
            for (; oppParity != picListFrm.End(); oppParity++)
            {
                if (dpb[*oppParity & 127].m_refPicFlag[!bottomPicFlag])
                {
                    picListFld.PushBack((*oppParity & 127) + mfxU8(!bottomPicFlag << 7));
                    oppParity++;
                    break;
                }
            }
        }
    }
};

    void MfxHwH264Encode::InitRefPicList(
        DdiTask & task,
        mfxU32    field)
    {
        ArrayU8x33 list0Frm(0xff); // list0 built like current picture is frame
        ArrayU8x33 list1Frm(0xff); // list1 built like current picture is frame

        ArrayU8x33    & list0 = task.m_list0[field];
        ArrayU8x33    & list1 = task.m_list1[field];
        ArrayDpbFrame & dpb   = task.m_dpb[field];

        mfxU32 useRefBasePicFlag = !!(task.m_type[field] & MFX_FRAMETYPE_KEYPIC);

        // build lists of reference frame
        if (task.m_type[field] & MFX_FRAMETYPE_IDR)
        {
            // in MVC P or B frame can be IDR
            // its DPB may be not empty
            // however it shouldn't have inter-frame references
        }
        else if (task.m_type[field] & MFX_FRAMETYPE_P)
        {
            // 8.2.4.2.1-2 "Initialisation process for
            // the reference picture list for P and SP slices in frames/fields"
            for (mfxU32 i = 0; i < dpb.Size(); i++)
                if (!dpb[i].m_longterm && (useRefBasePicFlag == dpb[i].m_refBase))
                    list0Frm.PushBack(mfxU8(i));

            std::sort(list0Frm.Begin(), list0Frm.End(), RefPicNumIsGreater(dpb));

            mfxU8 * firstLongTerm = list0Frm.End();

            for (mfxU32 i = 0; i < dpb.Size(); i++)
                if (dpb[i].m_longterm && (useRefBasePicFlag == dpb[i].m_refBase))
                    list0Frm.PushBack(mfxU8(i));

            std::sort(
                firstLongTerm,
                list0Frm.End(),
                LongTermRefPicNumIsLess(dpb));
        }
        else if (task.m_type[field] & MFX_FRAMETYPE_B)
        {
            // 8.2.4.2.3-4 "Initialisation process for
            // reference picture lists for B slices in frames/fields"
            for (mfxU32 i = 0; i < dpb.Size(); i++)
            {
                if (!dpb[i].m_longterm && (useRefBasePicFlag == dpb[i].m_refBase))
                {
                    if (dpb[i].m_poc[0] <= task.GetPoc(0))
                        list0Frm.PushBack(mfxU8(i));
                    else
                        list1Frm.PushBack(mfxU8(i));
                }
            }

            std::sort(list0Frm.Begin(), list0Frm.End(), RefPocIsGreater(dpb));
            std::sort(list1Frm.Begin(), list1Frm.End(), RefPocIsLess(dpb));

            // elements of list1 append list0
            // elements of list0 append list1
            mfxU32 list0Size = list0Frm.Size();
            mfxU32 list1Size = list1Frm.Size();

            for (mfxU32 ref = 0; ref < list1Size; ref++)
                list0Frm.PushBack(list1Frm[ref]);

            for (mfxU32 ref = 0; ref < list0Size; ref++)
                list1Frm.PushBack(list0Frm[ref]);

            mfxU8 * firstLongTermL0 = list0Frm.End();
            mfxU8 * firstLongTermL1 = list1Frm.End();

            for (mfxU32 i = 0; i < dpb.Size(); i++)
            {
                if (dpb[i].m_longterm && (useRefBasePicFlag == dpb[i].m_refBase))
                {
                    list0Frm.PushBack(mfxU8(i));
                    list1Frm.PushBack(mfxU8(i));
                }
            }

            std::sort(
                firstLongTermL0,
                list0Frm.End(),
                LongTermRefPicNumIsLess(dpb));

            std::sort(
                firstLongTermL1,
                list1Frm.End(),
                LongTermRefPicNumIsLess(dpb));
        }

        if (task.GetPicStructForEncode() & MFX_PICSTRUCT_PROGRESSIVE)
        {
            // just copy lists
            list0 = list0Frm;
            list1 = list1Frm;
        }
        else
        {
            // for interlaced picture we need to perform
            // 8.2.4.2.5 "Initialisation process for reference picture lists in fields"

            list0.Resize(0);
            list1.Resize(0);

            ProcessFields(field, dpb, list0Frm, list0);
            ProcessFields(field, dpb, list1Frm, list1);
        }

        // "When the reference picture list RefPicList1 has more than one entry
        // and RefPicList1 is identical to the reference picture list RefPicList0,
        // the first two entries RefPicList1[0] and RefPicList1[1] are switched"
        if (list1.Size() > 1 && list0 == list1)
        {
            std::swap(list1[0], list1[1]);
        }

        task.m_initSizeList0[field] = list0.Size();
        task.m_initSizeList1[field] = list1.Size();
    }

namespace
{
    mfxU8 * FindByExtFrameTag(
        mfxU8 *               begin,
        mfxU8 *               end,
        ArrayDpbFrame const & dpb,
        mfxU32                extFrameTag,
        mfxU32                picStruct)
    {
        mfxU8 bff = (picStruct == MFX_PICSTRUCT_FIELD_BFF) ? 1 : 0;
        for (; begin != end; ++begin)
            if (dpb[*begin & 127].m_extFrameTag == extFrameTag)
                if (picStruct == MFX_PICSTRUCT_PROGRESSIVE || bff == (*begin >> 7))
                    break;
        return begin;
    }
    mfxU8 * FindByFrameOrder(
        mfxU8 *               begin,
        mfxU8 *               end,
        ArrayDpbFrame const & dpb,
        mfxU32                frameOrder,
        mfxU32                picStruct)
    {
        mfxU8 bff = (picStruct == MFX_PICSTRUCT_FIELD_BFF) ? 1 : 0;
        for (; begin != end; ++begin)
            if (dpb[*begin & 127].m_frameOrder == frameOrder)
                if (picStruct == MFX_PICSTRUCT_PROGRESSIVE || bff == (*begin >> 7))
                    break;
        return begin;
    }


    void ReorderRefPicList(
        ArrayU8x33 &                 refPicList,
        ArrayDpbFrame const &        dpb,
        mfxExtAVCRefListCtrl const & ctrl,
        mfxU32                       numActiveRef,
        bool                         byExtFrameTag,
        bool                         bEncToolsMode)
    {
        mfxU8 * begin = refPicList.Begin();
        mfxU8 * end   = refPicList.End();

        mfxU8 * (*pFindRef)(
            mfxU8 *               begin,
            mfxU8 *               end,
            ArrayDpbFrame const & dpb,
            mfxU32                extFrameTag,
            mfxU32                picStruct) =
            ((byExtFrameTag) ?  FindByExtFrameTag :FindByFrameOrder);


        if (bEncToolsMode && begin != end)
            begin++; //the nearest frame shouldn't be reordered

        for (mfxU32 i = 0; i < 32 && ctrl.PreferredRefList[i].FrameOrder != 0xffffffff; i++)
        {
            mfxU8 * ref = pFindRef(
                begin,
                end,
                dpb,
                ctrl.PreferredRefList[i].FrameOrder,
                ctrl.PreferredRefList[i].PicStruct);

            if (ref != end)
                std::rotate(begin++, ref, ref + 1);
        }

        for (mfxU32 i = 0; i < 16 && ctrl.RejectedRefList[i].FrameOrder != 0xffffffff; i++)
        {
            mfxU8 * ref = pFindRef(
                begin,
                end,
                dpb,
                ctrl.RejectedRefList[i].FrameOrder,
                ctrl.RejectedRefList[i].PicStruct);

            if (ref != end)
                std::rotate(ref, ref + 1, end--);
        }

        refPicList.Resize((mfxU32)(end - refPicList.Begin()));
        if (numActiveRef > 0 && refPicList.Size() > numActiveRef)
            refPicList.Resize(numActiveRef);
    }

    typedef struct {
        mfxU32      FrameOrder;
        mfxU16      PicStruct;
        mfxU16      reserved[5];
    } mfxRefPic;

    void ReorderRefPicList(
        ArrayU8x33 &                 refPicList,
        ArrayDpbFrame const &        dpb,
        mfxRefPic const *            reordRefList,
        mfxU32                       numActiveRef)
    {
        mfxU8 * begin = refPicList.Begin();
        mfxU8 * end   = refPicList.End();

        for (mfxU32 i = 0; i < 32 && reordRefList[i].FrameOrder != 0xffffffff; i++)
        {
            mfxU8 * ref = FindByExtFrameTag(
                begin,
                end,
                dpb,
                reordRefList[i].FrameOrder,
                reordRefList[i].PicStruct);

            if (ref != end)
                std::rotate(begin++, ref, ref + 1);
        }

        if (numActiveRef > 0 && refPicList.Size() > numActiveRef)
            refPicList.Resize(numActiveRef);
    }
};

    ArrayRefListMod MfxHwH264Encode::CreateRefListMod(
        ArrayDpbFrame const & dpb,
        ArrayU8x33            initList,
        ArrayU8x33 const &    modList,
        mfxU32                curViewIdx,
        mfxI32                curPicNum,
        bool                  optimize)
    {
        assert(initList.Size() == modList.Size());

        ArrayRefListMod refListMod;

        mfxI32 picNumPred     = curPicNum;
        mfxI32 picViewIdxPred = -1;

        for (mfxU32 refIdx = 0; refIdx < modList.Size(); refIdx++)
        {
            if (optimize && initList == modList)
                return refListMod;

            if (dpb[modList[refIdx] & 0x7f].m_viewIdx != curViewIdx)
            {
                // inter-view reference reordering
                mfxI32 viewIdx = mfxI32(dpb[modList[refIdx] & 0x7f].m_viewIdx);

                if (viewIdx > picViewIdxPred)
                {
                    refListMod.PushBack(RefListMod(RPLM_INTERVIEW_ADD, mfxU16(viewIdx - picViewIdxPred - 1)));
                }
                else if (viewIdx < picViewIdxPred)
                {
                    refListMod.PushBack(RefListMod(RPLM_INTERVIEW_SUB, mfxU16(picViewIdxPred - viewIdx - 1)));
                }
                else
                {
                    assert(!"can't reorder ref list");
                    break;
                }

                for (mfxU32 cIdx = initList.Size(); cIdx > refIdx; cIdx--)
                    initList[cIdx] = initList[cIdx - 1];
                initList[refIdx] = modList[refIdx];
                mfxU32 nIdx = refIdx + 1;
                for (mfxU32 cIdx = refIdx + 1; cIdx <= initList.Size(); cIdx++)
                    if (mfxI32(dpb[initList[cIdx] & 0x7f].m_viewIdx) != viewIdx)
                        initList[nIdx++] = initList[cIdx];

                picViewIdxPred = viewIdx;
            }
            else if (dpb[modList[refIdx] & 0x7f].m_longterm)
            {
                // long term reference reordering
                mfxU8 longTermPicNum = GetLongTermPicNum(dpb, modList[refIdx]);

                refListMod.PushBack(RefListMod(RPLM_LT_PICNUM, longTermPicNum));

                for (mfxU32 cIdx = initList.Size(); cIdx > refIdx; cIdx--)
                    initList[cIdx] = initList[cIdx - 1];
                initList[refIdx] = modList[refIdx];
                mfxU32 nIdx = refIdx + 1;
                for (mfxU32 cIdx = refIdx + 1; cIdx <= initList.Size(); cIdx++)
                    if (GetLongTermPicNumF(dpb, initList[cIdx]) != longTermPicNum ||
                        dpb[initList[cIdx] & 0x7f].m_viewIdx != curViewIdx)
                        initList[nIdx++] = initList[cIdx];
            }
            else
            {
                // short term reference reordering
                mfxI32 picNum = GetPicNum(dpb, modList[refIdx]);

                if (picNum > picNumPred)
                {
                    mfxU16 absDiffPicNum = mfxU16(picNum - picNumPred);
                    refListMod.PushBack(RefListMod(RPLM_ST_PICNUM_ADD, absDiffPicNum - 1));
                }
                else if (picNum < picNumPred)
                {
                    mfxU16 absDiffPicNum = mfxU16(picNumPred - picNum);
                    refListMod.PushBack(RefListMod(RPLM_ST_PICNUM_SUB, absDiffPicNum - 1));
                }
                else
                {
                    assert(!"can't reorder ref list");
                    break;
                }

                for (mfxU32 cIdx = initList.Size(); cIdx > refIdx; cIdx--)
                    initList[cIdx] = initList[cIdx - 1];
                initList[refIdx] = modList[refIdx];
                mfxU32 nIdx = refIdx + 1;
                for (mfxU32 cIdx = refIdx + 1; cIdx <= initList.Size(); cIdx++)
                    if (GetPicNumF(dpb, initList[cIdx]) != picNum ||
                        dpb[initList[cIdx] & 0x7f].m_viewIdx != curViewIdx)
                        initList[nIdx++] = initList[cIdx];

                picNumPred = picNum;
            }
        }

        return refListMod;
    }

namespace
{
    bool CheckMfxExtAVCRefListsForField(mfxExtAVCRefLists const & ctrl)
    {
        bool isCorrect = true;

        for (mfxU8 i = 0; i < 32; i ++)
        {
            if (ctrl.RefPicList0[i].FrameOrder != static_cast<mfxU32>(MFX_FRAMEORDER_UNKNOWN)
                && ctrl.RefPicList0[i].PicStruct != MFX_PICSTRUCT_FIELD_TFF
                && ctrl.RefPicList0[i].PicStruct != MFX_PICSTRUCT_FIELD_BFF)
            {
                isCorrect = false;
                break;
            }
        }

        return isCorrect;
    }

    inline mfxI8 GetIdxOfFirstSameParity(ArrayU8x33 const & refList, mfxU32 fieldId)
    {
        for (mfxU8 i = 0; i < refList.Size(); i ++)
        {
            mfxU8 refFieldId = (refList[i] & 128) >> 7;
            if (fieldId == refFieldId)
            {
                return (mfxI8)i;
            }
        }

        return -1;
    }

    inline mfxU16 GetMaxNumRefActiveBL1(const mfxU32& targetUsage, 
                                        const bool& isField)
    {
        if (isField)
        {
            mfxU16 const DEFAULT_BY_TU[] = { 0, 2, 2, 2, 2, 2, 1, 1 };
            return DEFAULT_BY_TU[targetUsage];
        }
        else
        {
            return 1;
        }
    }
};

void MfxHwH264Encode::ModifyRefPicLists(
        MfxVideoParam const & video,
        DdiTask &             task,
        mfxU32                fieldId)
    {
        ArrayDpbFrame const & dpb   = task.m_dpb[fieldId];
        ArrayU8x33 &          list0 = task.m_list0[fieldId];
        ArrayU8x33 &          list1 = task.m_list1[fieldId];
        mfxU32                ps    = task.GetPicStructForEncode();
        ArrayRefListMod &     mod0  = task.m_refPicList0Mod[fieldId];
        ArrayRefListMod &     mod1  = task.m_refPicList1Mod[fieldId];

        bool BEncToolsMode = false;
#if defined(MFX_ENABLE_ENCTOOLS)
        mfxExtEncToolsConfig * config = GetExtBuffer(video);
        BEncToolsMode = ((IsOn(config->AdaptiveRefB) ||
            IsOn(config->AdaptiveRefP) ||
            IsOn(config->AdaptiveLTR))  &&
            task.m_internalListCtrlPresent &&
            (task.GetPicStructForEncode() & MFX_PICSTRUCT_PROGRESSIVE));
#endif
        ArrayU8x33 initList0 = task.m_list0[fieldId];
        ArrayU8x33 initList1 = task.m_list1[fieldId];
        mfxI32     curPicNum = task.m_picNum[fieldId];

        mfxU32 ffid = task.GetFirstField();
        bool isField = (task.GetPicStructForEncode() & (MFX_PICSTRUCT_FIELD_TFF | MFX_PICSTRUCT_FIELD_BFF)) != 0;
        bool isIPFieldPair = (task.m_type[ ffid] & MFX_FRAMETYPE_I) && (task.m_type[!ffid] & MFX_FRAMETYPE_P);
        mfxExtAVCRefLists * advCtrl = GetExtBuffer(task.m_ctrl, fieldId);

        if ((video.mfx.GopOptFlag & MFX_GOP_CLOSED) || (task.m_frameOrderI < task.m_frameOrder && !advCtrl))
        {
            // remove references to pictures prior to first I frame in decoding order
            // if gop is closed do it for all frames in gop
            // if gop is open do it for pictures subsequent to first I frame in display order

            mfxU32 firstIntraFramePoc = 2 * (task.m_frameOrderI - task.m_frameOrderIdr);

            list0.Erase(
                std::remove_if(list0.Begin(), list0.End(),
                    LogicalAnd(RefPocIsLessThan(dpb, firstIntraFramePoc), RefIsShortTerm(dpb))),
                list0.End());

            list1.Erase(
                std::remove_if(list1.Begin(), list1.End(),
                    LogicalAnd(RefPocIsLessThan(dpb, firstIntraFramePoc), RefIsShortTerm(dpb))),
                list1.End());
        }

        mfxExtCodingOptionDDI const & extDdi  = GetExtBufferRef(video);
        mfxExtCodingOption2 const   & extOpt2 = GetExtBufferRef(video);
        const mfxU32 numMaxActiveRefL1 = GetMaxNumRefActiveBL1(video.mfx.TargetUsage, isField);
        mfxU32 numActiveRefL1 = std::min<mfxU32>(extDdi.NumActiveRefBL1, numMaxActiveRefL1);
        mfxU32 numActiveRefL0 = (task.m_type[fieldId] & MFX_FRAMETYPE_P)
            ? extDdi.NumActiveRefP
            : extDdi.NumActiveRefBL0;

        mfxExtAVCRefListCtrl * ctrl = GetExtBuffer(task.m_ctrl);
        mfxExtAVCRefListCtrl   ctrl_copied = {};

        if (ctrl == 0 && task.m_internalListCtrlPresent && !IsAdaptiveLtrOn(video))
            ctrl = &task.m_internalListCtrl;
        if (ctrl)
            ctrl_copied = *ctrl;

        if (advCtrl && isField)
        {
            // check ref list control structure for interlaced case
            // TODO: in addition WRN should be returned from sync part if passed mfxExtAVCRefLists is incorrect
            if (false == CheckMfxExtAVCRefListsForField(*advCtrl))
            {
                // just ignore incorrect mfxExtAVCRefLists structure
                advCtrl = 0;
            }
        }
        if (BEncToolsMode && ((task.m_type[0] & MFX_FRAMETYPE_B) ||
                              (task.m_type[0] & MFX_FRAMETYPE_P)))
        {

            ArrayU8x33 backupList0 = list0;
            ArrayU8x33 backupList1 = list1;

            list0.Erase(
                std::remove_if(list0.Begin(), list0.End(), RefPocIsGreaterThan(dpb, task.GetPoc(fieldId))),
                list0.End());
            std::sort(list0.Begin(), list0.End(), RefPocIsGreater(dpb));

            if (task.m_type[0] & MFX_FRAMETYPE_B)
            {
                list1.Erase(
                    std::remove_if(list1.Begin(), list1.End(), RefPocIsLessThan(dpb, task.GetPoc(fieldId))),
                    list1.End());
                std::sort(list1.Begin(), list1.End(), RefPocIsLess(dpb));
            }

            // keep at least one ref pic in lists
            if (list0.Size() == 0)
                list0.PushBack(backupList0[0]);
            if (task.m_type[0] & MFX_FRAMETYPE_B)
            {
                if (list1.Size() == 0)
                    list1.PushBack(backupList1[0]);
            }

        }

        bool bCanApplyRefCtrl = video.calcParam.numTemporalLayer == 0 || video.mfx.GopRefDist == 1;

        // try to customize ref pic list using provided mfxExtAVCRefListCtrl
        if ((ctrl || advCtrl) && bCanApplyRefCtrl)
        {
            ArrayU8x33 backupList0 = list0;
            bool bIntList = (ctrl == &task.m_internalListCtrl);
            if (task.m_type[fieldId] & MFX_FRAMETYPE_PB)
            {
                if (advCtrl) // advanced ref list control has priority
                {
                    mfxU32 numActiveRefL0Final = advCtrl->NumRefIdxL0Active ? std::min<mfxU32>(advCtrl->NumRefIdxL0Active, numActiveRefL0) : numActiveRefL0;
                    ReorderRefPicList(list0, dpb, (mfxRefPic*)(&advCtrl->RefPicList0[0]), numActiveRefL0Final);
                }
                else
                {
                    mfxU32 numActiveRefL0Final = ctrl->NumRefIdxL0Active ? std::min<mfxU32>(ctrl->NumRefIdxL0Active, numActiveRefL0) : numActiveRefL0;
                    ReorderRefPicList(list0, dpb, ctrl_copied, numActiveRefL0Final, bIntList, BEncToolsMode);
                }
            }

            if (task.m_type[fieldId] & MFX_FRAMETYPE_B)
            {
                if (advCtrl) // advanced ref list control has priority
                {
                    numActiveRefL1 = advCtrl->NumRefIdxL1Active ? std::min<mfxU32>(advCtrl->NumRefIdxL1Active,numMaxActiveRefL1) : numActiveRefL1;
                    ReorderRefPicList(list1, dpb, (mfxRefPic*)(&advCtrl->RefPicList1[0]), numActiveRefL1);
                }
                else
                {
                    numActiveRefL1 = ctrl->NumRefIdxL1Active ? std::min<mfxU32>(ctrl->NumRefIdxL1Active,numMaxActiveRefL1) : numActiveRefL1;
                    ReorderRefPicList(list1, dpb, ctrl_copied, numActiveRefL1, bIntList, BEncToolsMode);
                }
            }

            if (video.calcParam.numTemporalLayer > 1)
            {
                // remove references with higher temporal layer
                list0.Erase(
                    std::remove_if(
                        list0.Begin(),
                        list0.End(),
                        RefIsFromHigherTemporalLayer(dpb, task.m_tid)),
                    list0.End());
            }

            if (backupList0.Size() && list0.Size() == 0)
            {
                // ref list is empty after customization. Ignore custom ref list since driver can't correctly encode Inter frame with empty ref list
                list0 = backupList0; // restore ref list L0 from backup
                bCanApplyRefCtrl = false;
                assert(!"Ref list is empty after customization");
            }
        }

        // form modified ref pic list using internal MSDK logic
        if ((ctrl == 0 && advCtrl == 0) || bCanApplyRefCtrl == false)
        {
            // prepare ref list for P-field of I/P field pair
            // swap 1st and 2nd entries of L0 ref pic list to use I-field of I/P pair as reference for P-field
            if (isIPFieldPair)
            {
                if (ps != MFX_PICSTRUCT_PROGRESSIVE && fieldId != ffid && list0.Size() > 1)
                    std::swap(list0[0], list0[1]);
            }
            else if (task.m_type[fieldId] & MFX_FRAMETYPE_B)
            {
                ArrayU8x33 backupList0 = list0;
                ArrayU8x33 backupList1 = list1;

                list0.Erase(
                    std::remove_if(list0.Begin(), list0.End(), RefPocIsGreaterThan(dpb, task.GetPoc(fieldId))),
                    list0.End());

                list1.Erase(
                    std::remove_if(list1.Begin(), list1.End(), RefPocIsLessThan(dpb, task.GetPoc(fieldId))),
                    list1.End());

                // keep at least one ref pic in lists
                if (list0.Size() == 0)
                    list0.PushBack(backupList0[0]);
                if (list1.Size() == 0)
                    list1.PushBack(backupList1[0]);

                if (isField && (0 > GetIdxOfFirstSameParity(list0, fieldId)))
                {
                    // revert optimization of L0 if all fields of same parity were removed from the list
                    // it's required since driver doesn't support field of opposite parity at 1st place in ref list
                    list0 = backupList0;
                }

                if (isField && (0 > GetIdxOfFirstSameParity(list1, fieldId)))
                {
                    // revert optimization of L1 if all fields of same parity were removed from the list
                    // it's required since driver doesn't support field of opposite parity at 1st place in ref list
                    list1 = backupList1;
                }
            }

            // P and B pic's ref list need to modify if P/I flag in the DPB is true which means the sence change.
            // as the I treated as CRA, so the P and B field don't ref on the P field of the P/I in list0.
            // and the B don't ref on the I field of the P/I in list1.
            if ((ps != MFX_PICSTRUCT_PROGRESSIVE) && (task.m_type[fieldId] & MFX_FRAMETYPE_PB))
            {
                mfxU8 index = 0;

                for (index = 0; index < dpb.Size(); index++)
                {
                     if (dpb[index].m_PIFieldFlag)
                     {
                         if (task.GetPoc(fieldId) > dpb[index].m_poc[0])
                             list0.Erase(
                             std::remove_if(list0.Begin(), list0.End(), RefPocIsLessThan(dpb, dpb[index].m_poc[!ffid])),
                             list0.End());
                         else if (task.GetPoc(fieldId) < dpb[index].m_poc[0])
                             list1.Erase(
                             std::remove_if(list1.Begin(), list1.End(), RefPocIsGreaterThan(dpb, dpb[index].m_poc[ffid])),
                             list1.End());
                     }
                }
            }

            if (video.calcParam.numTemporalLayer > 0)
            {
                list0.Erase(
                    std::remove_if(list0.Begin(), list0.End(),
                        RefIsFromHigherTemporalLayer(dpb, task.m_tid)),
                    list0.End());

                list1.Erase(
                    std::remove_if(list1.Begin(), list1.End(),
                        RefIsFromHigherTemporalLayer(dpb, task.m_tid)),
                    list1.End());

                std::sort(list0.Begin(), list0.End(), RefPocIsGreater(dpb));
                std::sort(list1.Begin(), list1.End(), RefPocIsLess(dpb));

                if (video.calcParam.tempScalabilityMode)
                { // cut lists to 1 element for tempScalabilityMode
                    list0.Resize(std::min(list0.Size(), 1u));
                    list1.Resize(std::min(list1.Size(), 1u));
                }
            }
            else if (extOpt2.BRefType == MFX_B_REF_PYRAMID && (task.m_type[0] & MFX_FRAMETYPE_P))
            {
                if (task.GetPicStructForEncode() & MFX_PICSTRUCT_PROGRESSIVE) {
                    std::sort(list0.Begin(), list0.End(), RefPocIsGreater(dpb));
                } else {
                    // reorder list0 in POC descending order separately for each parity
                    for (mfxU8* l = list0.Begin(); l < list0.End(); l++)
                        for (mfxU8* r = l + 1; r < list0.End(); r++)
                            if (((*l) >> 7) == ((*r) >> 7) && GetPoc(dpb, *r) > GetPoc(dpb, *l))
                                std::swap(*l, *r);
                }
            }

            if (video.calcParam.numTemporalLayer == 0 && (task.m_type[0] & MFX_FRAMETYPE_P) && task.m_internalListCtrlRefModLTR)
            {
                mfxU8 * begin = list0.Begin();
                mfxU8 * end = list0.End();
                mfxU8 * ltr = 0;
                for (; begin != end; ++begin)
                    if (dpb[*begin & 127].m_longterm)
                            break;

                ltr = begin;
                begin = list0.Begin();
                if (ltr != end && ltr != begin)
                    std::rotate(++begin, ltr, ltr + 1);
            }

            // as driver have fixed arbitrary reference field polarity support starting BDW, so no need
            // to WA to adjust the reflist order
            bool isHwSupportArbiRef = (task.m_hwType >= MFX_HW_BDW);

            if (isField && (isIPFieldPair == false) && (isHwSupportArbiRef == false))
            {
                // after modification of ref list L0 it could happen that 1st entry of the list has opposite parity to current field
                // driver doesn't support this, and MSDK performs WA and places field of same parity to 1st place
                mfxI8 idxOf1stSameParity = GetIdxOfFirstSameParity(list0, fieldId);
                if (idxOf1stSameParity > 0)
                {
                    std::swap(list0[0], list0[idxOf1stSameParity]);
                    list0.Resize(1);
                }
                else if (idxOf1stSameParity < 0)
                {
                    assert(!"No field of same parity for reference");
                }

                if (task.m_type[fieldId] & MFX_FRAMETYPE_B)
                {
                    // after modification of ref list L1 it could happen that 1st entry of the list has opposite parity to current field
                    // driver doesn't support this, and MSDK performs WA and places field of same parity to 1st place
                    idxOf1stSameParity = GetIdxOfFirstSameParity(list1, fieldId);
                    if (idxOf1stSameParity > 0)
                    {
                        std::swap(list1[0], list1[idxOf1stSameParity]);
                        list1.Resize(1);
                    }
                    else if (idxOf1stSameParity < 0)
                    {
                        assert(!"No field of same parity for reference");
                    }
                }
            }

            // cut ref pic lists according to user's limitations
            if (numActiveRefL0 > 0 && list0.Size() > numActiveRefL0)
                list0.Resize(numActiveRefL0);
            if (numActiveRefL1 > 0 && list1.Size() > numActiveRefL1)
                list1.Resize(numActiveRefL1);
        }

        initList0.Resize(list0.Size());
        initList1.Resize(list1.Size());

        bool bNoRefListModOpt0 = (video.calcParam.tempScalabilityMode== 0) && video.calcParam.numTemporalLayer && RefListHasLongTerm(dpb, initList0); // ref list mod syntax optimization is prohibited for SVC + LTR
        bool bNoRefListModOpt1 = (video.calcParam.tempScalabilityMode == 0) && video.calcParam.numTemporalLayer && RefListHasLongTerm(dpb, initList1); // ref list mod syntax optimization is prohibited for SVC + LTR

        mod0 = CreateRefListMod(dpb, initList0, list0, task.m_viewIdx, curPicNum, !bNoRefListModOpt0);
        mod1 = CreateRefListMod(dpb, initList1, list1, task.m_viewIdx, curPicNum, !bNoRefListModOpt1);
    }

namespace
{
    void DecideOnRefPicFlag(
        MfxVideoParam const & video,
        DdiTask &             task)
    {
        mfxU32 numLayers = video.calcParam.numTemporalLayer;
        if (numLayers > 1)
        {
            Pair<mfxU8> & ft = task.m_type;

            mfxU32 lastLayerScale =
                video.calcParam.scale[numLayers - 1] /
                video.calcParam.scale[numLayers - 2];

            if (((ft[0] | ft[1]) & MFX_FRAMETYPE_REF) &&    // one of fields is ref pic
                numLayers > 1 &&                            // more than one temporal layer
                lastLayerScale == 2 &&                      // highest layer is dyadic
                (task.m_tidx + 1U) == numLayers)               // this is the highest layer
            {
                ft[0] &= ~MFX_FRAMETYPE_REF;
                ft[1] &= ~MFX_FRAMETYPE_REF;
            }
        }
    }


    DpbFrame const * FindOldestRef(
        ArrayDpbFrame const & dpb,
        mfxU32                tid)
    {
        DpbFrame const * oldest = 0;
        DpbFrame const * i      = dpb.Begin();
        DpbFrame const * e      = dpb.End();

        for (; i != e; ++i)
            if (i->m_tid == tid)
                oldest = i;

        for (; i != e; ++i)
            if (i->m_tid == tid && i->m_frameOrder < oldest->m_frameOrder)
                oldest = i;

        return oldest;
    }


    mfxU32 CountRefs(
        ArrayDpbFrame const & dpb,
        mfxU32                tid)
    {
        mfxU32 counter = 0;
        for (DpbFrame const * i = dpb.Begin(), * e = dpb.End(); i != e; ++i)
            if (i->m_tid == tid)
                counter++;
        return counter;
    }


    void UpdateMaxLongTermFrameIdxPlus1(ArrayU8x8 & arr, mfxU8 curTidx, mfxU8 val)
    {
        std::fill(arr.Begin() + curTidx, arr.End(), val);
    }


    struct FindInDpbByExtFrameTag
    {
        FindInDpbByExtFrameTag(mfxU32 extFrameTag) : m_extFrameTag(extFrameTag) {}

        bool operator ()(DpbFrame const & dpbFrame) const
        {
            return dpbFrame.m_extFrameTag == m_extFrameTag;
        }

        mfxU32 m_extFrameTag;
    };

    struct FindInDpbByFrameOrder
    {
        FindInDpbByFrameOrder(mfxU32 frameOrder) : m_frameOrder(frameOrder) {}

        bool operator ()(DpbFrame const & dpbFrame) const
        {
            return dpbFrame.m_frameOrder == m_frameOrder;
        }

        mfxU32 m_frameOrder;
    };



    struct FindInDpbByLtrIdx
    {
        FindInDpbByLtrIdx(mfxU32 longTermIdx) : m_LongTermIdx(longTermIdx) {}

        bool operator ()(DpbFrame const & dpbFrame) const
        {
            return dpbFrame.m_longterm &&
                dpbFrame.m_longTermIdxPlus1 == (m_LongTermIdx + 1);
        }

        mfxU32 m_LongTermIdx;
    };
};

    bool MfxHwH264Encode::OrderByFrameNumWrap(DpbFrame const & lhs, DpbFrame const & rhs)
    {
        if (!lhs.m_longterm && !rhs.m_longterm)
            if (lhs.m_refBase == rhs.m_refBase)
                return lhs.m_frameNumWrap < rhs.m_frameNumWrap;
            else
                return lhs.m_refBase > rhs.m_refBase;

        else if (!lhs.m_longterm && rhs.m_longterm)
            return true;
        else if (lhs.m_longterm && !rhs.m_longterm)
            return false;
        else // both long term
            return lhs.m_longTermPicNum[0] < rhs.m_longTermPicNum[0];
    }
    bool MfxHwH264Encode::OrderByFrameNumWrapKeyRef(DpbFrame const & lhs, DpbFrame const & rhs)
    {
        if (!lhs.m_longterm && !rhs.m_longterm)
            if (lhs.m_refBase == rhs.m_refBase)
            {
                if (lhs.m_keyRef != rhs.m_keyRef)
                    return (lhs.m_keyRef < rhs.m_keyRef);
                else
                    return lhs.m_frameNumWrap < rhs.m_frameNumWrap;
            }
            else
                return lhs.m_refBase > rhs.m_refBase;

        else if (!lhs.m_longterm && rhs.m_longterm)
            return true;
        else if (lhs.m_longterm && !rhs.m_longterm)
            return false;
        else // both long term
            return lhs.m_longTermPicNum[0] < rhs.m_longTermPicNum[0];
    }
namespace
{
    bool OrderByDisplayOrder(DpbFrame const & lhs, DpbFrame const & rhs)
    {
        if (!lhs.m_longterm && !rhs.m_longterm)
            return lhs.m_frameOrder < rhs.m_frameOrder;
        else if (!lhs.m_longterm && rhs.m_longterm) // anti logic, sort by short term first!?
            return true;
        else if (lhs.m_longterm && !rhs.m_longterm) // anti logic, sort by short term first!?
            return false;
        else // both long term
            return lhs.m_frameOrder < rhs.m_frameOrder;
    }
    bool OrderByDisplayOrderKeyRef(DpbFrame const & lhs, DpbFrame const & rhs)
    {
        if (!lhs.m_longterm && !rhs.m_longterm)
        {
            if (lhs.m_keyRef != rhs.m_keyRef)
                return (lhs.m_keyRef < rhs.m_keyRef);
            else
                return lhs.m_frameOrder < rhs.m_frameOrder;
        }
        else if (!lhs.m_longterm && rhs.m_longterm) // anti logic, sort by short term first!?
            return true;
        else if (lhs.m_longterm && !rhs.m_longterm) // anti logic, sort by short term first!?
            return false;
        else // both long term
            return lhs.m_frameOrder < rhs.m_frameOrder;
    }
    struct OrderByNearestPrev
    {
        mfxU32 m_fo;

        OrderByNearestPrev(mfxU32 displayOrder) : m_fo(displayOrder) {}
        bool operator() (DpbFrame const & l, DpbFrame const & r)
        {
            return (l.m_frameOrder < m_fo) && ((r.m_frameOrder > m_fo) || ((m_fo - l.m_frameOrder) < (m_fo - r.m_frameOrder)));
        }
    };


    void InitNewDpbFrame(
        DpbFrame &      ref,
        DdiTask const & task,
        mfxU32          fid)
    {
        ref.m_poc               = task.GetPoc();
        ref.m_frameOrder        = task.m_frameOrder;
        ref.m_extFrameTag       = task.m_extFrameTag;
        ref.m_frameNum          = task.m_frameNum;
        ref.m_frameNumWrap      = task.m_frameNumWrap;
        ref.m_viewIdx           = task.m_viewIdx;
        ref.m_longTermPicNum    = task.m_longTermPicNum;
        ref.m_longTermIdxPlus1  = task.m_longTermFrameIdx + 1;
        ref.m_frameIdx          = task.m_idxRecon;
        ref.m_longterm          = ref.m_longTermIdxPlus1 > 0;
        ref.m_tid               = task.m_tid;
        ref.m_refBase           = 0;
        ref.m_midRec            = task.m_midRec;
        ref.m_cmRaw             = task.m_cmRaw;
        ref.m_cmRawLa           = task.m_cmRawLa;
        ref.m_cmMb              = task.m_cmMb;
        ref.m_refPicFlag[ fid]  = !!(task.m_type[ fid] & MFX_FRAMETYPE_REF);
        ref.m_refPicFlag[!fid]  = !!(task.m_type[!fid] & MFX_FRAMETYPE_REF);
        if (task.m_fieldPicFlag)
            ref.m_refPicFlag[!fid] = 0;
        ref.m_keyRef = task.m_keyReference;

        ref.m_cmHist    = task.m_cmHist;
        ref.m_cmHistSys = task.m_cmHistSys;

        ref.m_midRaw = task.m_midRaw;
        ref.m_yuvRaw = task.m_yuv;
    }

    bool ValidateLtrForTemporalScalability(
        MfxVideoParam const & video,
        DdiTask             & task)
    {
        mfxExtAVCRefListCtrl * ctrl = GetExtBuffer(task.m_ctrl);
        if (ctrl == 0)
            return true; //nothing to check

        if (task.GetPicStructForEncode() != MFX_PICSTRUCT_PROGRESSIVE
            || video.mfx.GopRefDist > 1)
        {
            // adaptive marking is supported only for progressive encoding
            // adaptive marking together with temporal scalability is supported for encoding w/o B-frames only
            return false;
        }

        bool isValid = true;
        mfxU32 temporalLayer = task.m_tidx;
        //ArrayDpbFrame const &  dpb  = task.m_dpb[fieldId];
        //mfxU32 nrfMinForTemporal = video.calcParam.numTemporalLayer > 1 ? mfxU16(1 << (video.calcParam.numTemporalLayer - 2)) : 1;

        for (mfxU32 i = 0; i < 16 && ctrl->LongTermRefList[i].FrameOrder != static_cast<mfxU32>(MFX_FRAMEORDER_UNKNOWN); i++)
        {
            isValid = false;
            if (temporalLayer)
            {
                // LTR request is attached to frame from not base layer
                assert(!"LTR is requested for frame from not base layer");
                break;
            }

            if (ctrl->LongTermRefList[i].FrameOrder == task.m_extFrameTag)
            {
                // LTR is requested for current frame
                isValid = true;
                break;
            }
        }

        if (isValid == false)
            assert(!"Current frame isn't included to LTR request");

        // allow application to remove frames from DPB on enhanced temporal layer
        // application is responsible for sending correct DPB change orders
        /*if (isValid == true &&
            ctrl->RejectedRefList[0].FrameOrder != MFX_FRAMEORDER_UNKNOWN && temporalLayer)
        {
            // dpb change request is attached to frame from not base layer
            assert(!"DPB change request is attached to frame from not base layer");
            isValid = false;
        }*/

        return isValid;
    }
};

    void MfxHwH264Encode::MarkDecodedRefPictures(
        MfxVideoParam const & video,
        DdiTask &             task,
        mfxU32                fid)
    {
        // declare shorter names
        ArrayDpbFrame const &  initDpb  = task.m_dpb[fid];
        ArrayDpbFrame &        currDpb  = (fid == task.m_fid[1]) ? task.m_dpbPostEncoding : task.m_dpb[!fid];
        ArrayU8x8 &            maxLtIdx = currDpb.m_maxLongTermFrameIdxPlus1;
        mfxU32                 type     = task.m_type[fid];
        DecRefPicMarkingInfo & marking  = task.m_decRefPicMrk[fid];

        // marking commands will be applied to dpbPostEncoding
        // initial dpb stay unchanged
        currDpb = initDpb;

        if ((type & MFX_FRAMETYPE_REF) == 0)
            return; // non-reference frames don't change dpb

        mfxExtAVCRefListCtrl const * ext_ctrl = GetExtBuffer(task.m_ctrl);
        mfxExtAVCRefListCtrl const * ctrl = (task.m_internalListCtrlPresent && (task.m_internalListCtrlHasPriority || !ext_ctrl))
            ? &task.m_internalListCtrl
            : ext_ctrl;
        bool useInternalFrameOrder = false;
        if (ctrl && ctrl == &task.m_internalListCtrl) useInternalFrameOrder = true;

        if (video.calcParam.numTemporalLayer > 0 &&
            false == ValidateLtrForTemporalScalability(video, task))
            ctrl = 0; // requested changes in dpb conflict with temporal scalability. Ignore requested dpb changes


        if (type & MFX_FRAMETYPE_IDR)
        {
            currDpb.Resize(0);
            UpdateMaxLongTermFrameIdxPlus1(maxLtIdx, 0, 0);

            marking.long_term_reference_flag = 0;

            if (ctrl
                )
            {
                for (mfxU32 i = 0; i < 16 && ctrl->LongTermRefList[i].FrameOrder != static_cast<mfxU32>(MFX_FRAMEORDER_UNKNOWN); i++)
                {
                    if (ctrl->LongTermRefList[i].FrameOrder == (useInternalFrameOrder ? task.m_frameOrder : task.m_extFrameTag))
                    {
                        marking.long_term_reference_flag = 1;
                        task.m_longTermFrameIdx = 0;
                        break;
                    }
                }
            }

            DpbFrame newDpbFrame;
            InitNewDpbFrame(newDpbFrame, task, fid);
            currDpb.PushBack(newDpbFrame);
            if (task.m_storeRefBasePicFlag)
            {
                newDpbFrame.m_refBase = 1;
                currDpb.PushBack(newDpbFrame);
            }
            UpdateMaxLongTermFrameIdxPlus1(maxLtIdx, 0, marking.long_term_reference_flag);
        }
        else
        {
            mfxU32 ffid = task.GetFirstField();

            bool currFrameIsAddedToDpb = (fid != ffid) && (task.m_type[ffid] & MFX_FRAMETYPE_REF);

            // collect used long-term frame indices
            ArrayU8x16 usedLtIdx;
            usedLtIdx.Resize(16, 0);
            for (mfxU32 i = 0; i < initDpb.Size(); i++)
                if (initDpb[i].m_longTermIdxPlus1 > 0)
                    usedLtIdx[initDpb[i].m_longTermIdxPlus1 - 1] = 1;

            // check longterm list
            // when frameOrder is sent first time corresponding 'short-term' reference is marked 'long-term'
            if (ctrl
               )

            {
                for (mfxU32 i = 0; i < 16 && ctrl->RejectedRefList[i].FrameOrder != static_cast<mfxU32>(MFX_FRAMEORDER_UNKNOWN); i++)
                {
                    DpbFrame * ref = currDpb.End();
                    if (!useInternalFrameOrder)
                    {
                        ref = std::find_if(
                            currDpb.Begin(),
                            currDpb.End(),
                            FindInDpbByExtFrameTag(ctrl->RejectedRefList[i].FrameOrder));
                    }
                    else
                    {
                        ref = std::find_if(
                            currDpb.Begin(),
                            currDpb.End(),
                            FindInDpbByFrameOrder(ctrl->RejectedRefList[i].FrameOrder));
                    }

                    if (ref != currDpb.End())
                    {
                        if (ref->m_longterm)
                        {
                            // adaptive marking for long-term references is supported only for progressive encoding
                            assert(task.GetPicStructForEncode() == MFX_PICSTRUCT_PROGRESSIVE);
                            marking.PushBack(MMCO_LT_TO_UNUSED, ref->m_longTermPicNum[0]);
                            usedLtIdx[ref->m_longTermIdxPlus1 - 1] = 0;
                        }
                        else
                        {
                            marking.PushBack(MMCO_ST_TO_UNUSED, task.m_picNum[fid] - ref->m_picNum[0] - 1);
                            if (task.GetPicStructForEncode() & (MFX_PICSTRUCT_FIELD_TFF | MFX_PICSTRUCT_FIELD_BFF))
                            {
                                // second field should be removed from DPB as well
                                marking.PushBack(MMCO_ST_TO_UNUSED, task.m_picNum[fid] - ref->m_picNum[1] - 1);
                            }

                        }
                        //printf("delete rejected from dpb for task %d: %d\n",
                        //   task.m_frameOrder, ref->m_frameOrder);

                        currDpb.Erase(ref);
                    }
                }

                for (mfxU32 i = 0; i < 16 && ctrl->LongTermRefList[i].FrameOrder != static_cast<mfxU32>(MFX_FRAMEORDER_UNKNOWN); i++)
                {
                    // adaptive marking for long-term references is supported only for progressive encoding
                    assert(task.GetPicStructForEncode() == MFX_PICSTRUCT_PROGRESSIVE);
                    DpbFrame * ref = currDpb.End();

                    if (!useInternalFrameOrder)
                    {
                        ref = std::find_if(
                            currDpb.Begin(),
                            currDpb.End(),
                            FindInDpbByExtFrameTag(ctrl->LongTermRefList[i].FrameOrder));
                    }
                    else
                    {
                        ref = std::find_if(
                            currDpb.Begin(),
                            currDpb.End(),
                            FindInDpbByFrameOrder(ctrl->LongTermRefList[i].FrameOrder));
                    }

                    if (video.calcParam.numTemporalLayer == 0 && // for temporal scalability only current frame could be marked as LTR
                        ref != currDpb.End() && ref->m_longterm == 0)
                    {
                        // find free long-term frame index
                        mfxU8 longTermIdx = mfxU8(std::find(usedLtIdx.Begin(), usedLtIdx.End(), 0) - usedLtIdx.Begin());
                        assert(longTermIdx != usedLtIdx.Size());
                        if (longTermIdx == usedLtIdx.Size())
                            break;

                        if (longTermIdx >= maxLtIdx[task.m_tidx])
                        {
                            // need to update MaxLongTermFrameIdx
                            assert(longTermIdx < video.mfx.NumRefFrame);
                            marking.PushBack(MMCO_SET_MAX_LT_IDX, longTermIdx + 1);
                            UpdateMaxLongTermFrameIdxPlus1(maxLtIdx, task.m_tidx, longTermIdx + 1);
                        }

                        marking.PushBack(MMCO_ST_TO_LT, task.m_picNum[fid] - ref->m_picNum[0] - 1, longTermIdx);
                        usedLtIdx[longTermIdx] = 1;
                        ref->m_longTermIdxPlus1 = longTermIdx + 1;

                        ref->m_longterm = 1;
                    }
                    else if (ctrl->LongTermRefList[i].FrameOrder == (useInternalFrameOrder ? task.m_frameOrder : task.m_extFrameTag))
                    {
                        // frame is not in dpb, but it is a current frame
                        // mark it as 'long-term'

                        mfxU8 longTermIdx;
                        if (ctrl->ApplyLongTermIdx != 1)
                        {
                            // first make free space in dpb if it is full
                            if (currDpb.Size() == video.mfx.NumRefFrame)
                            {
                                DpbFrame * toRemove = std::min_element(currDpb.Begin(), currDpb.End(), OrderByFrameNumWrap);

                                assert(toRemove != currDpb.End());
                                if (toRemove == currDpb.End())
                                    break;

                                if (toRemove->m_longterm == 1)
                                {
                                    // no short-term reference in dpb
                                    // remove oldest long-term
                                    toRemove = std::min_element(currDpb.Begin(), currDpb.End(), OrderByDisplayOrder);
                                    assert(toRemove->m_longterm == 1); // must be longterm ref

                                    marking.PushBack(MMCO_LT_TO_UNUSED, toRemove->m_longTermPicNum[0]);
                                    usedLtIdx[toRemove->m_longTermIdxPlus1 - 1] = 0;
                                }
                                else
                                {
                                    marking.PushBack(MMCO_ST_TO_UNUSED, task.m_picNum[fid] - toRemove->m_picNum[0] - 1);
                                }

                                currDpb.Erase(toRemove);
                            }

                            // find free long-term frame index
                            longTermIdx = mfxU8(std::find(usedLtIdx.Begin(), usedLtIdx.End(), 0) - usedLtIdx.Begin());
                            assert(longTermIdx != usedLtIdx.Size());
                            if (longTermIdx == usedLtIdx.Size())
                                break;
                        }
                        else
                        {
                            // use LTR idx provided by application
                            longTermIdx = (mfxU8)ctrl->LongTermRefList[i].LongTermIdx;

                            // don't validate input, just perform quick check
                            assert(longTermIdx < video.mfx.NumRefFrame);

                            DpbFrame * toRemove = std::find_if(
                                currDpb.Begin(),
                                currDpb.End(),
                                FindInDpbByLtrIdx(longTermIdx));

                            if (toRemove == currDpb.End() && currDpb.Size() == video.mfx.NumRefFrame)
                            {
                                toRemove = std::min_element(currDpb.Begin(), currDpb.End(), OrderByFrameNumWrap);

                                if (toRemove != currDpb.End())
                                {
                                    if (toRemove->m_longterm)
                                        toRemove = currDpb.End();
                                    else
                                        marking.PushBack(MMCO_ST_TO_UNUSED, task.m_picNum[fid] - toRemove->m_picNum[0] - 1);
                                }
                            }

                            if (toRemove != currDpb.End())
                                currDpb.Erase(toRemove);

                            assert(currDpb.Size() < video.mfx.NumRefFrame);
                        }

                        if (longTermIdx >= maxLtIdx[task.m_tidx])
                        {
                            // need to update MaxLongTermFrameIdx
                            assert(longTermIdx < video.mfx.NumRefFrame);
                            marking.PushBack(MMCO_SET_MAX_LT_IDX, longTermIdx + 1);
                            UpdateMaxLongTermFrameIdxPlus1(maxLtIdx, task.m_tidx, longTermIdx + 1);
                        }

                        marking.PushBack(MMCO_CURR_TO_LT, longTermIdx);
                        usedLtIdx[longTermIdx] = 1;
                        task.m_longTermFrameIdx = longTermIdx;

                        DpbFrame newDpbFrame;
                        InitNewDpbFrame(newDpbFrame, task, fid);
                        currDpb.PushBack(newDpbFrame);
                        assert(currDpb.Size() <= video.mfx.NumRefFrame);

                        currFrameIsAddedToDpb = true;
                    }
                }
            }

            // if first field was a reference then entire frame is already in dpb
            if (!currFrameIsAddedToDpb)
            {
                mfxExtCodingOption2 const & extOpt2 = GetExtBufferRef(video);

                for (mfxU32 refBase = 0; refBase <= task.m_storeRefBasePicFlag; refBase++)
                {
                    if (currDpb.Size() == video.mfx.NumRefFrame)
                    {
                        DpbFrame * toRemove = std::min_element(currDpb.Begin(), currDpb.End(), OrderByFrameNumWrapKeyRef);
                        DpbFrame * toRemoveDefault = std::min_element(currDpb.Begin(), currDpb.End(), OrderByFrameNumWrap);

                        if (toRemove != currDpb.End() && extOpt2.BRefType == MFX_B_REF_PYRAMID)
                        {
                            toRemove = std::min_element(currDpb.Begin(), currDpb.End(), OrderByDisplayOrderKeyRef);
                        }

                        assert(toRemove != currDpb.End());
                        if (toRemove == currDpb.End())
                            return;
                        //printf("delete from dpb for task %d: %d\n", task.m_frameOrder, toRemove->m_frameOrder);
                        if (toRemove->m_longterm == 1)
                        {
                            // no short-term reference in dpb
                            // remove oldest long-term
                            toRemove = std::min_element(currDpb.Begin(), currDpb.End(), OrderByDisplayOrder);
                            assert(toRemove->m_longterm == 1); // must be longterm ref

                            marking.PushBack(MMCO_LT_TO_UNUSED, toRemove->m_longTermPicNum[0]);
                            usedLtIdx[toRemove->m_longTermIdxPlus1 - 1] = 0;
                        }
                        else if (marking.mmco.Size() > 0 || toRemove != toRemoveDefault)
                        {
                            // already have mmco commands, sliding window will not be invoked
                            // remove oldest short-term manually
                            marking.PushBack(MMCO_ST_TO_UNUSED, task.m_picNum[fid] - toRemove->m_picNum[0] - 1);
                            if (task.m_fieldPicFlag)
                                marking.PushBack(MMCO_ST_TO_UNUSED, task.m_picNum[fid] - toRemove->m_picNum[1] - 1);
                        }

                        currDpb.Erase(toRemove);
                    }

                    DpbFrame newDpbFrame;
                    InitNewDpbFrame(newDpbFrame, task, fid);
                    newDpbFrame.m_refBase  = (mfxU8)refBase;
                    currDpb.PushBack(newDpbFrame);
                    assert(currDpb.Size() <= video.mfx.NumRefFrame);
                }
            }
        }
    }

namespace
{
    mfxStatus CreateAdditionalDpbCommands(
        MfxVideoParam const & video,
        DdiTask &             task)
    {
        if (task.m_internalListCtrlPresent &&
            video.calcParam.numTemporalLayer == 0)
            return MFX_ERR_NONE;

        task.m_internalListCtrlPresent = false;
        task.m_internalListCtrlHasPriority = true;
        task.m_internalListCtrlRefModLTR = false;
        InitExtBufHeader(task.m_internalListCtrl);

        mfxU32 numLayers  = video.calcParam.numTemporalLayer;
        mfxU32 refPicFlag = !!((task.m_type[0] | task.m_type[1]) & MFX_FRAMETYPE_REF);

        if (refPicFlag &&                                   // only ref frames occupy slot in dpb
            video.calcParam.tempScalabilityMode == 0 &&     // no long term refs in temporal scalability
            numLayers > 1 && (task.m_tidx + 1U) != numLayers)  // no dpb commands for last-not-based temporal laeyr
        {
            // find oldest ref frame from the same temporal layer
            DpbFrame const * toRemove = FindOldestRef(task.m_dpb[0], task.m_tid);

            if (toRemove == 0 && task.m_dpb[0].Size() == video.mfx.NumRefFrame)
            {
                // no ref frame from same layer but need to free dpb slot
                // look for oldest frame from the highest layer
                toRemove = FindOldestRef(task.m_dpb[0], numLayers - 1);
                assert(toRemove != 0);
            }

            if (video.mfx.GopRefDist > 1 &&                     // B frames present
                task.m_tidx == 0 &&                             // base layer
                CountRefs(task.m_dpb[0], 0) < 2 &&              // 0 or 1 refs from base layer
                task.m_dpb[0].Size() < video.mfx.NumRefFrame)   // dpb is not full yet
            {
                // this is to keep 2 references from base layer for B frames at next layer
                toRemove = 0;
            }

            if (toRemove)
            {
                task.m_internalListCtrl.RejectedRefList[0].FrameOrder = toRemove->m_frameOrder;
                task.m_internalListCtrl.RejectedRefList[0].PicStruct  = MFX_PICSTRUCT_PROGRESSIVE;
            }

            task.m_internalListCtrl.LongTermRefList[0].FrameOrder = task.m_frameOrder;
            task.m_internalListCtrl.LongTermRefList[0].PicStruct  = MFX_PICSTRUCT_PROGRESSIVE;
            task.m_internalListCtrlPresent = true;
        }
        else if (refPicFlag && IsAdaptiveLtrOn(video))
        {
            mfxExtCodingOptionDDI const * extDdi = GetExtBuffer(video);
            MFX_CHECK_NULL_PTR1(extDdi);
            mfxU32 numActiveRefL0 = (task.m_type[0] & MFX_FRAMETYPE_P)
                ? extDdi->NumActiveRefP
                : extDdi->NumActiveRefBL0;

            if ((task.m_type[0] & MFX_FRAMETYPE_IDR) && task.m_frameLtrOff == 0) {
                task.m_internalListCtrl.LongTermRefList[0].FrameOrder = task.m_frameOrder;
                task.m_internalListCtrl.LongTermRefList[0].PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
                task.m_internalListCtrlPresent = true;
                task.m_internalListCtrlHasPriority = false;
                task.m_LtrOrder = task.m_frameOrder;
            }
            else if ((task.m_type[0] & MFX_FRAMETYPE_REF) && task.m_frameLtrOff == 0 && task.m_frameLtrReassign != 1 && numActiveRefL0 > 1
                && (numActiveRefL0 < video.mfx.NumRefFrame || video.mfx.NumRefFrame > 2))
            {
                DpbFrame const * ltr = 0;
                DpbFrame const * i = task.m_dpb[0].Begin();
                DpbFrame const * e = task.m_dpb[0].End();
                DpbFrame const * p = &task.m_dpb[0].Back();

                for (; i != e; ++i) {
                    if (i->m_longterm) ltr = i;
                }

                if (ltr && ltr->m_frameOrder != p->m_frameOrder) {
                    task.m_internalListCtrlRefModLTR = true;
                }
            }
            else if (task.m_frameLtrOff == 1) {
                DpbFrame const * ltr = 0;
                DpbFrame const * i = task.m_dpb[0].Begin();
                DpbFrame const * e = task.m_dpb[0].End();

                for (; i != e; ++i) {
                    if (i->m_longterm) ltr = i;
                }

                if (ltr) {
                    task.m_internalListCtrl.RejectedRefList[0].FrameOrder = ltr->m_frameOrder;
                    task.m_internalListCtrl.RejectedRefList[0].PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
                    task.m_internalListCtrlPresent = true;
                    task.m_internalListCtrlHasPriority = false;
                    task.m_LtrOrder = -1;
                    task.m_LtrQp    = 0;
                }
            }
            else if ((task.m_type[0] & MFX_FRAMETYPE_P) && task.m_frameLtrReassign == 1 && task.m_frameLtrOff == 0)
            {
                DpbFrame const * ref = 0;
                DpbFrame const * i = task.m_dpb[0].Begin();
                DpbFrame const * e = task.m_dpb[0].End();
                DpbFrame const * p = &task.m_dpb[0].Back();

                for (; i != e; ++i) {
                    if ((mfxI32)i->m_frameOrder == task.m_RefOrder) ref = i;
                }

                if (ref) {
                    DpbFrame const * ltr = 0;
                    i = task.m_dpb[0].Begin();

                    for (; i != e; ++i) {
                        if (i->m_longterm) ltr = i;
                    }

                    if (ltr && (mfxI32)ltr->m_frameOrder == (mfxI32)task.m_LtrOrder) {
                        task.m_internalListCtrl.RejectedRefList[0].FrameOrder = ltr->m_frameOrder;
                        task.m_internalListCtrl.RejectedRefList[0].PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
                        task.m_internalListCtrl.LongTermRefList[0].FrameOrder = task.m_RefOrder;
                        task.m_internalListCtrl.LongTermRefList[0].PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
                        task.m_internalListCtrlPresent = true;
                        task.m_internalListCtrlHasPriority = false;
                        task.m_LtrOrder = task.m_RefOrder;
                        task.m_LtrQp    = task.m_RefQp;

                        if (numActiveRefL0 > 1 && (numActiveRefL0 < video.mfx.NumRefFrame || video.mfx.NumRefFrame > 2)
                            && ref->m_frameOrder != p->m_frameOrder)
                        {
                            task.m_internalListCtrlRefModLTR = true;
                        }
                    }
                }
            }
        }
		return MFX_ERR_NONE;
    }
};


DdiTaskIter MfxHwH264Encode::ReorderFrame(
    ArrayDpbFrame const & dpb,
    DdiTaskIter           begin,
    DdiTaskIter           end)
{
    // [begin, end) range of frames is in display order

    DdiTaskIter top = begin;
    while (top != end &&                                // go thru buffered frames in display order and
        (top->GetFrameType() & MFX_FRAMETYPE_B) &&      // get earliest non-B frame
        CountFutureRefs(dpb, top->m_frameOrder) == 0)   // or B frame with L1 reference
    {
        ++top;
    }

    if (top != end && (top->GetFrameType() & MFX_FRAMETYPE_B))
    {
        // special case for B frames (when B pyramid is enabled)
        DdiTaskIter i = top;
        while (++i != end &&                                    // check remaining
            (i->GetFrameType() & MFX_FRAMETYPE_B) &&            // B frames
            (i->m_loc.miniGopCount == top->m_loc.miniGopCount)) // from the same mini-gop
        {
            if (top->m_loc.encodingOrder > i->m_loc.encodingOrder)
                top = i;
        }
    }

    return top;
}


DdiTaskIter MfxHwH264Encode::ReorderFrame(
    ArrayDpbFrame const & dpb,
    DdiTaskIter           begin,
    DdiTaskIter           end,
    bool                  gopStrict,
    bool                  flush,
    bool                  closeGopForSceneChange)
{
    DdiTaskIter top = ReorderFrame(dpb, begin, end);
    DdiTaskIter prev = top;
    if(prev != end && prev != begin){            //special case for custom IDR frame
        --prev;                                  //we just change previous B-frame to P-frame
        if((top->m_ctrl.FrameType & MFX_FRAMETYPE_IDR
            || ((top->m_ctrl.FrameType & MFX_FRAMETYPE_I) && closeGopForSceneChange && top->m_SceneChange)  // Also Do it for scene change I frame
            || ((top->m_ctrl.FrameType & MFX_FRAMETYPE_P) && closeGopForSceneChange && top->m_SceneChange)) // Also Do it for scene change B-> P frame
            && prev->GetFrameType() & MFX_FRAMETYPE_B && !gopStrict)
        {
          prev->m_type[0] = MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF;
          prev->m_type[1] = MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF;
          top = prev;
        }
    }
    if (flush && top == end && begin != end)
    {
        if (gopStrict)
        {
            top = begin; // TODO: reorder remaining B frames for B-pyramid when enabled
        }
        else
        {
            top = end;
            --top;
            assert(top->GetFrameType() & MFX_FRAMETYPE_B);
            top->m_type[0] = MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF;
            top->m_type[1] = MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF;
            top = ReorderFrame(dpb, begin, end);
            assert(top != end || begin == end);
        }
    }
    else if (prev != end &&!top->m_frameLtrOff && top->m_ctrl.FrameType & MFX_FRAMETYPE_IDR)
    {
        // Mini LA
        DdiTaskIter next = top;
        ++next;
        while (next != end) {
            if (next->m_SceneChange)
                top->m_frameLtrOff = 1;
            ++next;
        }
    }
    return top;
}


PairU8 MfxHwH264Encode::GetFrameType(
    MfxVideoParam const & video,
    mfxU32                frameOrder)
{
    mfxU32 gopOptFlag = video.mfx.GopOptFlag;
    mfxU32 gopPicSize = video.mfx.GopPicSize;
    mfxU32 gopRefDist = video.mfx.GopRefDist;
    mfxU32 idrPicDist = gopPicSize * (video.mfx.IdrInterval + 1);

    if (gopPicSize == 0xffff) //infinite GOP
        idrPicDist = gopPicSize = 0xffffffff;

    if (frameOrder % idrPicDist == 0)
        return ExtendFrameType(MFX_FRAMETYPE_I | MFX_FRAMETYPE_REF | MFX_FRAMETYPE_IDR);

    if (frameOrder % gopPicSize == 0)
        return ExtendFrameType(MFX_FRAMETYPE_I | MFX_FRAMETYPE_REF);

    if (frameOrder % gopPicSize % gopRefDist == 0)
        return ExtendFrameType(MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF);
    if ((gopOptFlag & MFX_GOP_STRICT) == 0)
        if (((frameOrder + 1) % gopPicSize == 0 && (gopOptFlag & MFX_GOP_CLOSED)) ||
            (frameOrder + 1) % idrPicDist == 0)
            return ExtendFrameType(MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF); // switch last B frame to P frame

    return ExtendFrameType(MFX_FRAMETYPE_B);
}

IntraRefreshState MfxHwH264Encode::GetIntraRefreshState(
    MfxVideoParam const & video,
    mfxU32                frameOrderInGopDispOrder,
    mfxEncodeCtrl const * ctrl,
    mfxU16                intraStripeWidthInMBs,
    SliceDivider &        divider,
    MFX_ENCODE_CAPS       caps)
{
    IntraRefreshState state;
    mfxExtCodingOption2 & extOpt2Init = GetExtBufferRef(video);
    mfxExtCodingOption3 & extOpt3Init = GetExtBufferRef(video);
    state.firstFrameInCycle = false;

    if (extOpt2Init.IntRefType == 0)
        return state;

    // set QP for Intra macroblocks within refreshing line
    state.IntRefQPDelta = extOpt2Init.IntRefQPDelta;
    if (ctrl)
    {
        mfxExtCodingOption2 * extOpt2Runtime = GetExtBuffer(*ctrl);
        if (extOpt2Runtime && extOpt2Runtime->IntRefQPDelta <= 51 && extOpt2Runtime->IntRefQPDelta >= -51)
            state.IntRefQPDelta = extOpt2Runtime->IntRefQPDelta;
    }
    if (extOpt2Init.IntRefType == MFX_REFRESH_SLICE)
    {
        if (mfxI32(frameOrderInGopDispOrder - 1) < 0)
        {
            // reset divider on I frames
            bool fieldCoding = (video.mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_PROGRESSIVE) == 0;
            divider = MakeSliceDivider(
                (caps.ddi_caps.SliceLevelRateCtrl) ? SliceDividerType::ARBITRARY_MB_SLICE : SliceDividerType(caps.ddi_caps.SliceStructure),
                extOpt2Init.NumMbPerSlice,
                extOpt3Init.NumSliceP,
                video.mfx.FrameInfo.Width / 16,
                video.mfx.FrameInfo.Height / 16 / (fieldCoding ? 2 : 1));
            return state;
        }

        if (mfxI32(frameOrderInGopDispOrder - 1) < 0)
            return state;

        state.firstFrameInCycle = extOpt3Init.IntRefCycleDist ? (((frameOrderInGopDispOrder - 1) % extOpt3Init.IntRefCycleDist == 0) && frameOrderInGopDispOrder) : (((frameOrderInGopDispOrder - 1) % extOpt3Init.NumSliceP == 0) && frameOrderInGopDispOrder);

        state.IntraSize = ((USHORT)divider.GetNumMbInSlice() / (video.mfx.FrameInfo.Width >> 4));
        state.IntraLocation = ((USHORT)divider.GetFirstMbInSlice() / (video.mfx.FrameInfo.Width >> 4));

        if ((state.IntraLocation == 0) && (!state.firstFrameInCycle))
        {
            state.IntraSize = 0; // no refresh between cycles
        }
        else
        {
            state.refrType = extOpt2Init.IntRefType;
            if (!divider.Next())
            {
                bool fieldCoding = (video.mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_PROGRESSIVE) == 0;
                divider = MakeSliceDivider(
                    (caps.ddi_caps.SliceLevelRateCtrl) ? SliceDividerType::ARBITRARY_MB_SLICE : SliceDividerType(caps.ddi_caps.SliceStructure),
                    extOpt2Init.NumMbPerSlice,
                    extOpt3Init.NumSliceP,
                    video.mfx.FrameInfo.Width / 16,
                    video.mfx.FrameInfo.Height / 16 / (fieldCoding ? 2 : 1));
            }
        }
        return state;
    }


    mfxU32 refreshPeriod = extOpt3Init.IntRefCycleDist ? extOpt3Init.IntRefCycleDist : extOpt2Init.IntRefCycleSize;
    mfxU32 offsetFromStartOfGop = extOpt3Init.IntRefCycleDist ? refreshPeriod : 1; // 1st refresh cycle in GOP starts with offset

    mfxI32 frameOrderMinusOffset = frameOrderInGopDispOrder - offsetFromStartOfGop;
    if (frameOrderMinusOffset < 0)
        return state; // too early to start refresh

    mfxU32 frameOrderInRefreshPeriod = frameOrderMinusOffset % refreshPeriod;
    if (frameOrderInRefreshPeriod >= extOpt2Init.IntRefCycleSize)
        return state; // for current refresh period refresh cycle is already passed

    // check if Intra refresh required for current frame
    mfxU32 refreshDimension = extOpt2Init.IntRefType == MFX_REFRESH_HORIZONTAL ? video.mfx.FrameInfo.Height >> 4 : video.mfx.FrameInfo.Width >> 4;
    mfxU32 numFramesWithoutRefresh = extOpt2Init.IntRefCycleSize - (refreshDimension + intraStripeWidthInMBs - 1) / intraStripeWidthInMBs;
    mfxU32 idxInRefreshCycle = frameOrderInRefreshPeriod;
    state.firstFrameInCycle = (idxInRefreshCycle == 0);
    mfxI32 idxInActualRefreshCycle = idxInRefreshCycle - numFramesWithoutRefresh;
    if (idxInActualRefreshCycle < 0)
        return state; // actual refresh isn't started yet within current refresh cycle, no Intra column/row required for current frame

    state.refrType = extOpt2Init.IntRefType;
    state.IntraSize = intraStripeWidthInMBs;
    state.IntraLocation = (mfxU16)idxInActualRefreshCycle * intraStripeWidthInMBs;

    return state;
}

mfxStatus MfxHwH264Encode::UpdateIntraRefreshWithoutIDR(
    MfxVideoParam const & oldPar,
    MfxVideoParam const & newPar,
    mfxU32                baseLayerOrder,
    mfxI64                oldStartFrame,
    mfxI64 &              updatedStartFrame,
    mfxU16 &              updatedStripeWidthInMBs,
    SliceDivider &        divider,
    MFX_ENCODE_CAPS       caps)
{
    MFX_CHECK_WITH_ASSERT((oldPar.mfx.FrameInfo.Width == newPar.mfx.FrameInfo.Width) && (oldPar.mfx.FrameInfo.Height == newPar.mfx.FrameInfo.Height), MFX_ERR_UNDEFINED_BEHAVIOR);
    mfxExtCodingOption2 & extOpt2Old = GetExtBufferRef(oldPar);
    mfxExtCodingOption2 & extOpt2New = GetExtBufferRef(newPar);
    mfxExtCodingOption3 & extOpt3Old = GetExtBufferRef(oldPar);
    mfxExtCodingOption3 & extOpt3New = GetExtBufferRef(newPar);

    MFX_CHECK_WITH_ASSERT(extOpt2New.IntRefType && extOpt2New.IntRefCycleSize, MFX_ERR_UNDEFINED_BEHAVIOR);
    mfxU16 refreshDimension = extOpt2New.IntRefType == MFX_REFRESH_HORIZONTAL ? newPar.mfx.FrameInfo.Height >> 4 : newPar.mfx.FrameInfo.Width >> 4;
    updatedStripeWidthInMBs = (refreshDimension + extOpt2New.IntRefCycleSize - 1) / extOpt2New.IntRefCycleSize;

    if ((!extOpt2Old.IntRefType) || (extOpt2Old.IntRefType == MFX_REFRESH_SLICE))
    {
        updatedStartFrame = baseLayerOrder - (extOpt3New.IntRefCycleDist ? extOpt3New.IntRefCycleDist : 1);
        return MFX_ERR_NONE;
    }

    mfxU16 oldRefreshDimension = extOpt2Old.IntRefType == MFX_REFRESH_HORIZONTAL ? newPar.mfx.FrameInfo.Height >> 4 : newPar.mfx.FrameInfo.Width >> 4;
    mfxU16 oldStripeWidthInMBs = (oldRefreshDimension + extOpt2Old.IntRefCycleSize - 1) / extOpt2Old.IntRefCycleSize;
    IntraRefreshState oldIRState = MfxHwH264Encode::GetIntraRefreshState(
        oldPar,
        mfxU32(baseLayerOrder - oldStartFrame),
        0,
        oldStripeWidthInMBs,
        divider,
        caps);

    if (   extOpt2New.IntRefType != extOpt2Old.IntRefType
        || (!oldIRState.IntraLocation && !oldIRState.firstFrameInCycle))
    {
        if (!extOpt3New.IntRefCycleDist)
        {
            updatedStartFrame = baseLayerOrder - 1;
        }
        else
        {
            mfxU16 oldIRProgressInFrames = extOpt3Old.IntRefCycleDist ? (baseLayerOrder % extOpt3Old.IntRefCycleDist) : 0;

            if (!oldIRProgressInFrames || oldIRState.IntraLocation)
            {
                oldIRProgressInFrames += (mfxU16)(extOpt3Old.IntRefCycleDist < baseLayerOrder ? extOpt3Old.IntRefCycleDist : baseLayerOrder);
            }

            if (extOpt3New.IntRefCycleDist < oldIRProgressInFrames || !oldIRProgressInFrames)
            {
                updatedStartFrame = baseLayerOrder - extOpt3New.IntRefCycleDist;
            }
            else
            {
                updatedStartFrame = baseLayerOrder - oldIRProgressInFrames;
            }
        }
    }
    else if (extOpt3Old.IntRefCycleDist || extOpt3New.IntRefCycleDist)
    {
        if (oldIRState.IntraLocation)
        {
            updatedStartFrame = baseLayerOrder - ((oldIRState.IntraLocation + updatedStripeWidthInMBs - 1) / updatedStripeWidthInMBs);
        }
        else //if (oldIRState.firstFrameInCycle || !extOpt3New->IntRefCycleDist)
        {
            updatedStartFrame = baseLayerOrder;
        }

        if (   extOpt3New.IntRefCycleDist > extOpt3Old.IntRefCycleDist
            && extOpt3Old.IntRefCycleDist > extOpt2Old.IntRefCycleSize)
        {
            updatedStartFrame += (extOpt3New.IntRefCycleDist - extOpt3Old.IntRefCycleDist);
        }

        if (extOpt3New.IntRefCycleDist)
        {
            updatedStartFrame = (updatedStartFrame % extOpt3New.IntRefCycleDist) - extOpt3New.IntRefCycleDist;
        }
        else
        {
            updatedStartFrame--;
        }
    }
    else
    {
        mfxU16 updatedNumFramesWithoutRefresh = extOpt2New.IntRefCycleSize - (refreshDimension + updatedStripeWidthInMBs - 1) / updatedStripeWidthInMBs;
        mfxU16 newIRProgressInFrames = oldIRState.IntraLocation / updatedStripeWidthInMBs + updatedNumFramesWithoutRefresh + 1;
        updatedStartFrame = baseLayerOrder - newIRProgressInFrames;
    }

    return MFX_ERR_NONE;
}


DdiTaskIter MfxHwH264Encode::FindFrameToStartEncode(
    MfxVideoParam const & video,
    DdiTaskIter           begin,
    DdiTaskIter           end)
{
    if (/*video.mfx.RateControlMethod != MFX_RATECONTROL_LA &&*/
        video.mfx.RateControlMethod != MFX_RATECONTROL_CQP ||
        video.AsyncDepth == 1)
        return begin;

    // in case of CQP optimization is applied
    // dependency between ENC and PAK is removed by encoding reference P frame earlier
    DdiTaskIter first = begin;
    DdiTaskIter second = ++begin;
    if (first == end || second == end)
        return first;
    if ((first ->GetFrameType() & MFX_FRAMETYPE_B) &&
        (second->GetFrameType() & MFX_FRAMETYPE_P) &&
        (second->m_frameNum == first->m_frameNum))
        return second;
    return first;
}


DdiTaskIter MfxHwH264Encode::FindFrameToWaitEncode(
    DdiTaskIter begin,
    DdiTaskIter end)
{
    if (begin == end)
        return begin;

    DdiTaskIter oldest = begin;
    for (++begin; begin != end; ++begin)
        if (oldest->m_encOrder > begin->m_encOrder)
            oldest = begin;
    return oldest;
}
DdiTaskIter MfxHwH264Encode::FindFrameToWaitEncodeNext(
    DdiTaskIter begin,
    DdiTaskIter end,
    DdiTaskIter cur)
{
    if (begin == end)
        return cur;

    DdiTaskIter oldest = cur;

    for (; begin != end; ++begin)
        if (((oldest->m_encOrder > begin->m_encOrder)&&(oldest != cur)) || ( (oldest == cur) && (begin->m_encOrder > cur->m_encOrder)))
            oldest = begin;
    return oldest;
}


namespace
{
mfxU32 GetEncodingOrder(mfxU32 displayOrder, mfxU32 begin, mfxU32 end, mfxU32 &level, mfxU32 before, bool & ref)
{
    assert(displayOrder >= begin);
    assert(displayOrder <  end);

    ref = (end - begin > 1);

    mfxU32 pivot = (begin + end) / 2;
    if (displayOrder == pivot)
        return level + before;

    level ++;
    if (displayOrder < pivot)
        return GetEncodingOrder(displayOrder, begin, pivot,  level , before, ref);
    else
        return GetEncodingOrder(displayOrder, pivot + 1, end, level, before + pivot - begin, ref);
}
};


BiFrameLocation MfxHwH264Encode::GetBiFrameLocation(
    MfxVideoParam const & video,
    mfxU32                frameOrder,
    mfxU32                currGopRefDist,
    mfxU32                miniGOPCount)
{
    mfxExtCodingOption2 const & extOpt2 = GetExtBufferRef(video);

    mfxU32 gopPicSize = video.mfx.GopPicSize;
    mfxU32 biPyramid  = extOpt2.BRefType;

    BiFrameLocation loc;

    currGopRefDist = (currGopRefDist > 0) ?
        std::min<mfxU32>(currGopRefDist, video.mfx.GopRefDist) :
        video.mfx.GopRefDist;


    if (gopPicSize == 0xffff) //infinite GOP
        gopPicSize = 0xffffffff;

    if (biPyramid != MFX_B_REF_OFF)
    {
        bool ref = false;
        mfxU32 orderInMiniGop = std::max<mfxI32>(frameOrder % gopPicSize % currGopRefDist, 1) - 1;
        loc.level = 1;
        loc.encodingOrder = GetEncodingOrder(orderInMiniGop, 0, currGopRefDist - 1, loc.level ,0, ref);
        loc.miniGopCount  = miniGOPCount;
        loc.refFrameFlag  = mfxU16(ref ? MFX_FRAMETYPE_REF : 0);
    }

    return loc;
}

void MfxHwH264Encode::ConfigureTask(
    DdiTask &                 task,
    DdiTask const &           prevTask,
    MfxVideoParam const &     video,
    MFX_ENCODE_CAPS const &   caps)
{
    mfxExtCodingOption const &      extOpt         = GetExtBufferRef(video);
    mfxExtCodingOption2 const &     extOpt2        = GetExtBufferRef(video);
    mfxExtCodingOption2 const *     extOpt2Runtime = GetExtBuffer(task.m_ctrl);
    mfxExtCodingOptionDDI const &   extDdi         = GetExtBufferRef(video);
    mfxExtSpsHeader const &         extSps         = GetExtBufferRef(video);
    mfxExtAvcTemporalLayers const & extTemp        = GetExtBufferRef(video);
    mfxExtEncoderROI const &        extRoi         = GetExtBufferRef(video);
    mfxExtEncoderROI const *        extRoiRuntime  = GetExtBuffer(task.m_ctrl);
    mfxExtCodingOption3 const &     extOpt3        = GetExtBufferRef(video);
    mfxExtDirtyRect const *    extDirtyRect        = GetExtBuffer(video);
    mfxExtDirtyRect const *    extDirtyRectRuntime = GetExtBuffer(task.m_ctrl);
    mfxExtMoveRect const *     extMoveRect         = GetExtBuffer(video);
    mfxExtMoveRect const *     extMoveRectRuntime  = GetExtBuffer(task.m_ctrl);

    mfxU32 const FRAME_NUM_MAX = 1 << (extSps.log2MaxFrameNumMinus4 + 4);

    mfxU32 ffid = task.m_fid[0];
    mfxU32 sfid = !ffid;

    // in order not to affect original GetFrameType, add P/I field flag
    bool isPIFieldPair      = (task.m_type[ffid] & MFX_FRAMETYPE_P) && (task.m_type[!ffid] & MFX_FRAMETYPE_I);
    bool isPrevPIFieldPair  = (prevTask.m_type[ffid] & MFX_FRAMETYPE_P) && (prevTask.m_type[!ffid] & MFX_FRAMETYPE_I);

    mfxU32 numReorderFrames = GetNumReorderFrames(video);
    mfxU32 prevsfid         = prevTask.m_fid[1];
    mfxU8  idrPicFlag       = !!(task.GetFrameType() & MFX_FRAMETYPE_IDR);
    mfxU8  intraPicFlag     = !!(task.GetFrameType() & MFX_FRAMETYPE_I) ? 1 : (isPIFieldPair ? 1 : 0);
    mfxU8  prevIdrFrameFlag = !!(prevTask.GetFrameType() & MFX_FRAMETYPE_IDR);
    mfxU8  prevIFrameFlag   = !!(prevTask.GetFrameType() & MFX_FRAMETYPE_I) ? 1 : (isPrevPIFieldPair ? 1 : 0);
    mfxU8  prevRefPicFlag   = !!(prevTask.GetFrameType() & MFX_FRAMETYPE_REF);
    mfxU8  prevIdrPicFlag   = !!(prevTask.m_type[prevsfid] & MFX_FRAMETYPE_IDR);

    mfxU8  frameNumIncrement = (prevRefPicFlag || prevTask.m_nalRefIdc[0]) ? 1 : 0;

    mfxU16 SkipMode = extOpt2Runtime ? extOpt2Runtime->SkipFrame : extOpt2.SkipFrame;

    task.m_frameOrderIdr = idrPicFlag ? task.m_frameOrder : prevTask.m_frameOrderIdr;
    task.m_frameOrderI   = intraPicFlag ? task.m_frameOrder : prevTask.m_frameOrderI;
    task.m_encOrder      = prevTask.m_encOrder + 1;
    task.m_encOrderIdr   = prevIdrFrameFlag ? prevTask.m_encOrder : prevTask.m_encOrderIdr;
    task.m_encOrderI     = prevIFrameFlag ? prevTask.m_encOrder : prevTask.m_encOrderI;


    task.m_isUseRawRef = IsOn(extOpt2.UseRawRef);
    task.m_isSkipped = false;
    if (task.m_isUseRawRef && extOpt2Runtime)
      task.m_isUseRawRef = !IsOff(extOpt2Runtime->UseRawRef);

    task.m_frameNum = mfxU16((prevTask.m_frameNum + frameNumIncrement) % FRAME_NUM_MAX);
    if (idrPicFlag)
        task.m_frameNum = 0;

    task.m_picNum[0] = task.m_frameNum * (task.m_fieldPicFlag + 1) + task.m_fieldPicFlag;
    task.m_picNum[1] = task.m_picNum[0];

    task.m_idrPicId = prevTask.m_idrPicId + idrPicFlag;

    mfxU32 prevBPSeiFrameEncOrder = (extOpt2.BufferingPeriodSEI == MFX_BPSEI_IFRAME) ? task.m_encOrderI : task.m_encOrderIdr;
    mfxU32 insertBPSei = (extOpt2.BufferingPeriodSEI == MFX_BPSEI_IFRAME && intraPicFlag) || (idrPicFlag);
    task.m_dpbOutputDelay   = 2 * (task.m_frameOrder + numReorderFrames - task.m_encOrder);
    task.m_cpbRemoval[ffid] = 2 * (task.m_encOrder - prevBPSeiFrameEncOrder);
    task.m_cpbRemoval[sfid] = (insertBPSei) ? 1 : task.m_cpbRemoval[ffid] + 1;

    if (video.calcParam.tempScalabilityMode)
        task.m_tidx = CalcTemporalLayerIndex(video, task.m_frameOrder - task.m_frameOrderStartTScalStructure);
    else
        task.m_tidx = CalcTemporalLayerIndex(video, task.m_frameOrder - task.m_frameOrderIdrInDisplayOrder);
    task.m_tid  = video.calcParam.tid[task.m_tidx];
    task.m_pid  = task.m_tidx + extTemp.BaseLayerPID;

    DecideOnRefPicFlag(video, task); // for temporal layers

    if (task.m_ctrl.SkipFrame != 0 && SkipMode != MFX_SKIPFRAME_BRC_ONLY)
    {
        task.m_ctrl.SkipFrame = (extOpt2.SkipFrame) ? 1 : 0;
        if (video.mfx.GopRefDist > 1 && ((task.m_type.top & MFX_FRAMETYPE_REF) || (task.m_type.bot & MFX_FRAMETYPE_REF)))
        {
            task.m_ctrl.SkipFrame = 0;
        }

        if (task.SkipFlag() != 0)
        {
            task.m_type.top &= ~MFX_FRAMETYPE_REF;
            task.m_type.bot &= ~MFX_FRAMETYPE_REF;
        }
    }
    else
    {
        task.m_ctrl.SkipFrame = std::min<mfxU16>(0xFF, task.m_ctrl.SkipFrame);
    }

    task.m_reference[ffid]  = !!(task.m_type[ffid] & MFX_FRAMETYPE_REF);
    task.m_reference[sfid]  = !!(task.m_type[sfid] & MFX_FRAMETYPE_REF);

    task.m_decRefPicMrkRep[ffid].presentFlag                = prevIdrPicFlag || prevTask.m_decRefPicMrk[prevsfid].mmco.Size();
    task.m_decRefPicMrkRep[ffid].original_idr_flag          = prevIdrPicFlag;
    task.m_decRefPicMrkRep[ffid].original_frame_num         = prevTask.m_frameNum;
    task.m_decRefPicMrkRep[ffid].original_field_pic_flag    = prevTask.m_fieldPicFlag;
    task.m_decRefPicMrkRep[ffid].original_bottom_field_flag = prevTask.m_fid[prevsfid];
    task.m_decRefPicMrkRep[ffid].dec_ref_pic_marking        = prevTask.m_decRefPicMrk[prevsfid];

    task.m_subMbPartitionAllowed[0] = CheckSubMbPartition(&extDdi, task.m_type[0]);
    task.m_subMbPartitionAllowed[1] = CheckSubMbPartition(&extDdi, task.m_type[1]);

    task.m_insertAud[ffid] = IsOn(extOpt.AUDelimiter);
    task.m_insertAud[sfid] = IsOn(extOpt.AUDelimiter);
    task.m_insertSps[ffid] = intraPicFlag;
    task.m_insertSps[sfid] = 0;
    task.m_insertPps[ffid] = task.m_insertSps[ffid] || IsOn(extOpt2.RepeatPPS);
    task.m_insertPps[sfid] = task.m_insertSps[sfid] || IsOn(extOpt2.RepeatPPS);
    task.m_nalRefIdc[ffid] = task.m_reference[ffid];
    task.m_nalRefIdc[sfid] = task.m_reference[sfid];

    // process roi
    mfxExtEncoderROI const * pRoi = &extRoi;
    if (extRoiRuntime)
    {
        pRoi = extRoiRuntime;
    }

    task.m_numRoi = 0;

    if (pRoi && pRoi->NumROI)
    {
        mfxU16 numRoi = pRoi->NumROI <= task.m_roi.Capacity() ? pRoi->NumROI : (mfxU16)task.m_roi.Capacity();


        if (numRoi > caps.ddi_caps.MaxNumOfROI)
        {
            numRoi = caps.ddi_caps.MaxNumOfROI;
        }

        if (pRoi->ROIMode != MFX_ROI_MODE_QP_DELTA && pRoi->ROIMode != MFX_ROI_MODE_PRIORITY)
        {
            numRoi = 0;
        }

        if (caps.ddi_caps.ROIBRCDeltaQPLevelSupport == 0)
        {
            numRoi = 0;
        }

        for (mfxU16 i = 0; i < numRoi; i ++)
        {
            if (extRoiRuntime)
            {
                mfxRoiDesc task_roi = {pRoi->ROI[i].Left,  pRoi->ROI[i].Top,
                                       pRoi->ROI[i].Right, pRoi->ROI[i].Bottom,
                                       (mfxI16)((pRoi->ROIMode == MFX_ROI_MODE_PRIORITY ? (-1) : 1) * pRoi->ROI[i].DeltaQP) };

                // check runtime ROI
                mfxStatus sts = CheckAndFixRoiQueryLike(video, &task_roi, extRoiRuntime->ROIMode);

                if (sts != MFX_ERR_UNSUPPORTED) {
                    task.m_roi[task.m_numRoi] = task_roi;
                    task.m_numRoi++;
                }
            }
            else
            {
                task.m_roi[task.m_numRoi] = {pRoi->ROI[i].Left,  pRoi->ROI[i].Top,
                                             pRoi->ROI[i].Right, pRoi->ROI[i].Bottom,
                                             (mfxI16)((pRoi->ROIMode == MFX_ROI_MODE_PRIORITY ? (-1) : 1) * pRoi->ROI[i].DeltaQP) };
                task.m_numRoi ++;
            }
        }
        task.m_roiMode = MFX_ROI_MODE_QP_DELTA;
    }

    mfxExtDirtyRect const * pDirtyRect = extDirtyRectRuntime ? extDirtyRectRuntime : extDirtyRect;

    task.m_numDirtyRect = 0;

    if (pDirtyRect && pDirtyRect->NumRect)
    {
        mfxU16 numRect = pDirtyRect->NumRect <= task.m_dirtyRect.Capacity() ? pDirtyRect->NumRect : (mfxU16)task.m_dirtyRect.Capacity();

        for (mfxU16 i = 0; i < numRect; i ++)
        {
            task.m_dirtyRect[task.m_numDirtyRect] = {pDirtyRect->Rect[i].Left,  pDirtyRect->Rect[i].Top,
                                                     pDirtyRect->Rect[i].Right, pDirtyRect->Rect[i].Bottom};

            if (extDirtyRectRuntime)
            {
                // check runtime dirty rectangle
                mfxStatus sts = CheckAndFixRectQueryLike(video, &(task.m_dirtyRect[task.m_numDirtyRect]));
                if (sts != MFX_ERR_UNSUPPORTED)
                    task.m_numDirtyRect ++;
            }
            else
                task.m_numDirtyRect ++;
        }
    }

    mfxExtMoveRect const * pMoveRect = extMoveRectRuntime ? extMoveRectRuntime : extMoveRect;

    task.m_numMovingRect = 0;

    if (pMoveRect && pMoveRect->NumRect)
    {
        mfxU16 numRect = pMoveRect->NumRect <= task.m_movingRect.Capacity() ? pMoveRect->NumRect : (mfxU16)task.m_movingRect.Capacity();

        for (mfxU16 i = 0; i < numRect; i ++)
        {
            task.m_movingRect[task.m_numMovingRect] = {pMoveRect->Rect[i].DestLeft,   pMoveRect->Rect[i].DestTop,
                                                       pMoveRect->Rect[i].DestRight,  pMoveRect->Rect[i].DestBottom,
                                                       pMoveRect->Rect[i].SourceLeft, pMoveRect->Rect[i].SourceTop};

            if (extMoveRectRuntime)
            {
                // check runtime moving rectangle
                mfxStatus sts = CheckAndFixMovingRectQueryLike(video, &(task.m_movingRect[task.m_numMovingRect]));
                if (sts != MFX_ERR_UNSUPPORTED)
                    task.m_numMovingRect ++;
            }
            else
                task.m_numMovingRect ++;
        }
    }

    if (extOpt3.MaxFrameSizeI)
    {
        task.m_maxIFrameSize  = extOpt3.MaxFrameSizeI;
        task.m_maxPBFrameSize = extOpt3.MaxFrameSizeP;
    }
    else
    {
        task.m_maxIFrameSize = extOpt2Runtime ? extOpt2Runtime->MaxFrameSize : extOpt2.MaxFrameSize;
        task.m_maxPBFrameSize = 0;
    }

    task.m_numMbPerSlice = extOpt2.NumMbPerSlice;
    task.m_numSlice[ffid] = (task.m_type[ffid] & MFX_FRAMETYPE_I) ? extOpt3.NumSliceI :
        (task.m_type[ffid] & MFX_FRAMETYPE_P) ? extOpt3.NumSliceP : extOpt3.NumSliceB;
    task.m_numSlice[!ffid] = (task.m_type[!ffid] & MFX_FRAMETYPE_I) ? extOpt3.NumSliceI :
        (task.m_type[!ffid] & MFX_FRAMETYPE_P) ? extOpt3.NumSliceP : extOpt3.NumSliceB;

    if (video.calcParam.tempScalabilityMode)
    {
        task.m_insertPps[ffid] = task.m_insertSps[ffid];
        task.m_insertPps[sfid] = task.m_insertSps[sfid];
        task.m_nalRefIdc[ffid] = idrPicFlag ? 3 : (task.m_reference[ffid] ? 2 : 0);
        task.m_nalRefIdc[sfid] = task.m_reference[sfid] ? 2 : 0;
    }

    task.m_cqpValue[0] = GetQpValue(task, video, task.m_type[0]);
    task.m_cqpValue[1] = GetQpValue(task, video, task.m_type[1]);

    task.m_statusReportNumber[0] = 2 * task.m_encOrder;
    task.m_statusReportNumber[1] = 2 * task.m_encOrder + 1;

    task.m_dpb[ffid] = prevTask.m_dpbPostEncoding;

    CreateAdditionalDpbCommands(video, task); // for svc temporal layers & LTR

    UpdateDpbFrames(task, ffid, FRAME_NUM_MAX);
    InitRefPicList(task, ffid);
    ModifyRefPicLists(video, task, ffid);
    MarkDecodedRefPictures(video, task, ffid);

    if (task.m_fieldPicFlag)
    {
        UpdateDpbFrames(task, sfid, FRAME_NUM_MAX);
        InitRefPicList(task, sfid);
        ModifyRefPicLists(video, task, sfid);

        // mark second field of last added frame short-term ref
        task.m_dpbPostEncoding = task.m_dpb[sfid];
        if (task.m_reference[sfid])
            task.m_dpbPostEncoding.Back().m_refPicFlag[sfid] = 1;

        // mark the P/I field pair flag
        if ((task.m_type[ffid] & MFX_FRAMETYPE_P)    &&
            (task.m_type[ffid] & MFX_FRAMETYPE_REF)  &&
            (task.m_type[sfid] & MFX_FRAMETYPE_I)    &&
            (task.m_type[sfid] & MFX_FRAMETYPE_REF))
            task.m_dpbPostEncoding.Back().m_PIFieldFlag = 1;
    }

#if defined(MFX_ENABLE_MFE)
    //if previous frame finished later than current submitted(e.g. reordering or async) - use sync-sync time for counting timeout for better performance
    //otherwise if current frame submitted later than previous finished - use submit->sync time for better performance handling
    task.m_beginTime = std::max(task.m_beginTime, prevTask.m_endTime);
#endif
    const mfxExtCodingOption2* extOpt2Cur = (extOpt2Runtime ? extOpt2Runtime : &extOpt2);

    if (task.m_type[ffid] & MFX_FRAMETYPE_I)
    {
        task.m_minQP = extOpt2Cur->MinQPI;
        task.m_maxQP = extOpt2Cur->MaxQPI;
    }
    else if (task.m_type[ffid] & MFX_FRAMETYPE_P)
    {
        task.m_minQP = extOpt2Cur->MinQPP;
        task.m_maxQP = extOpt2Cur->MaxQPP;
    }
    else if (task.m_type[ffid] & MFX_FRAMETYPE_B)
    {
        task.m_minQP = extOpt2Cur->MinQPB;
        task.m_maxQP = extOpt2Cur->MaxQPB;
    }

    if (task.m_maxQP > 51)
        task.m_maxQP = 0;

    if (task.m_minQP > 51 || (task.m_maxQP && task.m_minQP > task.m_maxQP))
        task.m_minQP = 0;

    mfxU32 fieldMaxCount = video.mfx.FrameInfo.PicStruct == MFX_PICSTRUCT_PROGRESSIVE ? 1 : 2;
    for (mfxU32 field = 0; field < fieldMaxCount; field++)
    {
        mfxExtFeiSliceHeader * extFeiSlice = GetExtBuffer(task.m_ctrl, field);

        if (NULL == extFeiSlice)
        {
            // take default buffer (from Init) if not provided in runtime
            extFeiSlice = GetExtBuffer(video, field);
        }

        // Fill deblocking parameters
        mfxU8 disableDeblockingIdc   = (mfxU8)extOpt2Cur->DisableDeblockingIdc;
        mfxI8 sliceAlphaC0OffsetDiv2 = 0;
        mfxI8 sliceBetaOffsetDiv2    = 0;

        for (mfxU32 i = 0; i < task.m_numSlice[task.m_fid[field]]; i++)
        {
            if (extFeiSlice && NULL != extFeiSlice->Slice && i < extFeiSlice->NumSlice)
            {
                // If only one buffer was passed on init, that value will be propagated for entire frame
                disableDeblockingIdc   = (mfxU8)extFeiSlice->Slice[i].DisableDeblockingFilterIdc;
                sliceAlphaC0OffsetDiv2 = (mfxI8)extFeiSlice->Slice[i].SliceAlphaC0OffsetDiv2;
                sliceBetaOffsetDiv2    = (mfxI8)extFeiSlice->Slice[i].SliceBetaOffsetDiv2;
            }

            // Store per-slice values in task
            task.m_disableDeblockingIdc[task.m_fid[field]].push_back(disableDeblockingIdc);
            task.m_sliceAlphaC0OffsetDiv2[task.m_fid[field]].push_back(sliceAlphaC0OffsetDiv2);
            task.m_sliceBetaOffsetDiv2[task.m_fid[field]].push_back(sliceBetaOffsetDiv2);
        } // for (mfxU32 i = 0; i < task.m_numSlice[field]; i++)
    } // for (mfxU32 field = 0; field < fieldMaxCount; field++)

#ifndef MFX_AVC_ENCODING_UNIT_DISABLE
    task.m_collectUnitsInfo = IsOn(extOpt3.EncodedUnitsInfo);
#endif

    task.m_TCBRCTargetFrameSize = video.calcParam.TCBRCTargetFrameSize;
}


mfxStatus MfxHwH264Encode::CopyRawSurfaceToVideoMemory(
    VideoCORE &           core,
    MfxVideoParam const & video,
    DdiTask const &       task)
{
    mfxExtOpaqueSurfaceAlloc const & extOpaq = GetExtBufferRef(video);

    mfxFrameSurface1 * surface = task.m_yuv;

    if (video.IOPattern == MFX_IOPATTERN_IN_SYSTEM_MEMORY ||
        (video.IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY && (extOpaq.In.Type & MFX_MEMTYPE_SYSTEM_MEMORY)))
    {
        if (video.IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY)
        {
            surface = core.GetNativeSurface(task.m_yuv);
            if (surface == 0)
                return Error(MFX_ERR_UNDEFINED_BEHAVIOR);

            surface->Info            = task.m_yuv->Info;
            surface->Data.TimeStamp  = task.m_yuv->Data.TimeStamp;
            surface->Data.FrameOrder = task.m_yuv->Data.FrameOrder;
            surface->Data.Corrupted  = task.m_yuv->Data.Corrupted;
            surface->Data.DataFlag   = task.m_yuv->Data.DataFlag;
        }

        mfxFrameData d3dSurf = {};
        mfxFrameData sysSurf = surface->Data;

        //FrameLocker lock1(&core, d3dSurf, task.m_midRaw);
        FrameLocker lock2(&core, sysSurf, true);
        d3dSurf.MemId = task.m_midRaw;

        if (sysSurf.Y == 0)
            return Error(MFX_ERR_LOCK_MEMORY);

        mfxStatus sts = MFX_ERR_NONE;
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_INTERNAL, "Copy input (sys->d3d)");
            sts = CopyFrameDataBothFields(&core, d3dSurf, sysSurf, video.mfx.FrameInfo);
            if (sts != MFX_ERR_NONE)
                return Error(sts);
        }

        sts = lock2.Unlock();
        if (sts != MFX_ERR_NONE)
            return Error(sts);

        //sts = lock1.Unlock();
        //if (sts != MFX_ERR_NONE)
        //    return Error(sts);
    }

    return MFX_ERR_NONE;
}
bool MfxHwH264Encode::IsFrameToSkip(DdiTask&  task, MfxFrameAllocResponse & poolRec, std::vector<mfxU32> fo, bool bSWBRC)
{
    if (task.m_isSkipped)
        return true;

    mfxI32 fid = task.m_fid[0];
    if (task.m_list1[fid].Size() && bSWBRC && (task.GetFrameType() & MFX_FRAMETYPE_B))
    {
        mfxU8 ind = task.m_list1[fid][0] & 127;
        if (ind < 15 )
        {
            ArrayDpbFrame & dpb = task.m_dpb[0];
            mfxU32 idxRec = mfxU32(std::find(fo.begin(), fo.end(), dpb[ind].m_frameOrder) - fo.begin());
            return ((poolRec.GetFlag(idxRec) & H264_FRAME_FLAG_SKIPPED) != 0);
        }
    }
    return false;
}
mfxStatus MfxHwH264Encode::CodeAsSkipFrame(     VideoCORE &            core,
                                                MfxVideoParam const &  video,
                                                DdiTask&       task,
                                                MfxFrameAllocResponse & pool,
                                                MfxFrameAllocResponse & poolRec)
{
    mfxStatus sts = MFX_ERR_NONE;

    if (task.m_midRaw == NULL)
    {

        task.m_idx    = FindFreeResourceIndex(pool);
        task.m_midRaw = AcquireResource(pool, task.m_idx);
        MFX_CHECK_NULL_PTR1(task.m_midRaw);

        sts = core.GetFrameHDL(task.m_midRaw, &task.m_handleRaw.first);
        MFX_CHECK_STS(sts);
    }


    if (task.GetFrameType() & MFX_FRAMETYPE_I)
    {
        mfxFrameData curr = {};
        curr.MemId = task.m_midRaw;
        FrameLocker lock1(&core, curr, task.m_midRaw);
        mfxU32 size = curr.Pitch*video.mfx.FrameInfo.Height;
        memset(curr.Y, 0, size);

        switch (video.mfx.FrameInfo.FourCC)
        {
            case MFX_FOURCC_NV12:
                memset(curr.UV, 128, size >> 1);
                break;
            case MFX_FOURCC_YV12:
                memset(curr.U, 128, size >> 2);
                memset(curr.V, 128, size >> 2);
                break;
            default:
                return MFX_ERR_UNDEFINED_BEHAVIOR;
        }
    }
    else
    {
        mfxI32 fid = task.m_fid[0];
        mfxU8 ind = ((task.GetFrameType() & MFX_FRAMETYPE_B)  && task.m_list1[fid].Size() != 0) ? task.m_list1[fid][0] : task.m_list0[fid][0];
        MFX_CHECK(ind < 15, MFX_ERR_UNDEFINED_BEHAVIOR);

        DpbFrame& refFrame = task.m_dpb[0][ind];
        mfxFrameData curr = {};
        mfxFrameData ref = {};
        curr.MemId = task.m_midRaw;
        ref.MemId  = refFrame.m_midRec;

        mfxFrameSurface1 surfSrc = { {0,}, video.mfx.FrameInfo, ref  };
        mfxFrameSurface1 surfDst = { {0,}, video.mfx.FrameInfo, curr };
        if ((poolRec.GetFlag(refFrame.m_frameIdx) & H264_FRAME_FLAG_READY) != 0)
        {
            sts = core.DoFastCopyWrapper(&surfDst, MFX_MEMTYPE_INTERNAL_FRAME | MFX_MEMTYPE_DXVA2_DECODER_TARGET | MFX_MEMTYPE_FROM_ENCODE, &surfSrc, MFX_MEMTYPE_INTERNAL_FRAME | MFX_MEMTYPE_DXVA2_DECODER_TARGET | MFX_MEMTYPE_FROM_ENCODE);

            if (ind != 0)
                poolRec.SetFlag(refFrame.m_frameIdx, H264_FRAME_FLAG_SKIPPED);
        }
        else
            return MFX_WRN_DEVICE_BUSY;
    }
    poolRec.SetFlag(task.m_idxRecon, H264_FRAME_FLAG_SKIPPED);
    return sts;
}




mfxStatus MfxHwH264Encode::GetNativeHandleToRawSurface(
    VideoCORE &           core,
    MfxVideoParam const & video,
    DdiTask const &       task,
    mfxHDLPair &          handle)
{
    mfxStatus sts = MFX_ERR_NONE;

    mfxExtOpaqueSurfaceAlloc const & extOpaq = GetExtBufferRef(video);

    Zero(handle);
    mfxHDL * nativeHandle = &handle.first;

    mfxFrameSurface1 * surface = task.m_yuv;

    if (video.IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY)
    {
        surface = core.GetNativeSurface(task.m_yuv);
        if (surface == 0)
            return Error(MFX_ERR_UNDEFINED_BEHAVIOR);

        surface->Info            = task.m_yuv->Info;
        surface->Data.TimeStamp  = task.m_yuv->Data.TimeStamp;
        surface->Data.FrameOrder = task.m_yuv->Data.FrameOrder;
        surface->Data.Corrupted  = task.m_yuv->Data.Corrupted;
        surface->Data.DataFlag   = task.m_yuv->Data.DataFlag;
    }

    if (video.IOPattern == MFX_IOPATTERN_IN_SYSTEM_MEMORY ||
        (video.IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY && (extOpaq.In.Type & MFX_MEMTYPE_SYSTEM_MEMORY)))
        sts = core.GetFrameHDL(task.m_midRaw, nativeHandle);
    else if (video.IOPattern == MFX_IOPATTERN_IN_VIDEO_MEMORY)
        sts = core.GetExternalFrameHDL(surface->Data.MemId, nativeHandle);
    else if (video.IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY) // opaq with internal video memory
        sts = core.GetFrameHDL(surface->Data.MemId, nativeHandle);
    else
        return Error(MFX_ERR_UNDEFINED_BEHAVIOR);

    return sts;
}


mfxHDL MfxHwH264Encode::ConvertMidToNativeHandle(
    VideoCORE & core,
    mfxMemId    mid,
    bool        external)
{
    mfxHDL handle = 0;

    mfxStatus sts = (external)
        ? core.GetExternalFrameHDL(mid, &handle)
        : core.GetFrameHDL(mid, &handle);
    assert(sts == MFX_ERR_NONE);

    return (sts == MFX_ERR_NONE) ? handle : 0;
}


namespace MfxHwH264EncodeHW
{
    void DivideCost(std::vector<MbData> & mb, mfxI32 width, mfxI32 height, mfxU32 cost, mfxI32 x, mfxI32 y)
    {
        mfxI32 mbx = x >> 4;
        mfxI32 mby = y >> 4;
        mfxI32 xx  = x & 15;
        mfxI32 yy  = y & 15;

        if (mbx + 0 < width && mby + 0 < height && mbx + 0 >= 0 && mby + 0 >= 0)
            mb[width * (mby + 0) + mbx + 0].propCost += mfxU32(cost * (16 - xx) * (16 - yy) / 256);

        if (mbx + 1 < width && mby + 0 < height && mbx + 1 >= 0 && mby + 0 >= 0)
            mb[width * (mby + 0) + mbx + 1].propCost += mfxU32(cost * (     xx) * (16 - yy) / 256);

        if (mbx + 0 < width && mby + 1 < height && mbx + 0 >= 0 && mby + 1 >= 0)
            mb[width * (mby + 1) + mbx + 0].propCost += mfxU32(cost * (16 - xx) * (     yy) / 256);

        if (mbx + 1 < width && mby + 1 < height && mbx + 1 >= 0 && mby + 1 >= 0)
            mb[width * (mby + 1) + mbx + 1].propCost += mfxU32(cost * (     xx) * (     yy) / 256);
    }
};
using namespace MfxHwH264EncodeHW;


void MfxHwH264Encode::AnalyzeVmeData(DdiTaskIter begin, DdiTaskIter end, mfxU32 width, mfxU32 height)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_INTERNAL, "AnalyzeVmeData");

    mfxI32 w = width  >> 4;
    mfxI32 h = height >> 4;

    for (DdiTaskIter task = begin; task != end; task++)
    {
        task->m_vmeData->propCost = 0;
        for (size_t i = 0; i < task->m_vmeData->mb.size(); i++)
            task->m_vmeData->mb[i].propCost = 0;
    }

    DdiTaskIter task = end;
    for (--task; task != begin; --task)
    {
        VmeData * cur = task->m_vmeData;
        VmeData * l0  = 0;
        VmeData * l1  = 0;

        if (task->m_fwdRef && task->m_fwdRef->m_encOrder >= begin->m_encOrder)
            l0 = task->m_fwdRef->m_vmeData;
        if (task->m_bwdRef && task->m_bwdRef->m_encOrder >= begin->m_encOrder)
            l1 = task->m_bwdRef->m_vmeData;

        for (mfxI32 y = 0; y < h; y++)
        {
            MbData const * mb = &cur->mb[y * w];
            for (mfxI32 x = 0; x < w; x++, mb++)
            {
                if (!mb->intraMbFlag)
                {
                    mfxU32 amount   = mb->intraCost - mb->interCost;
                    mfxF64 frac     = amount / mfxF64(mb->intraCost);
                    mfxU32 propCost = mfxU32(amount + mb->propCost * frac + 0.5);

                    if (mb->mbType == MBTYPE_BP_L0_16x16)
                    {
                        if (l0)
                            DivideCost(l0->mb, w, h, propCost,
                                (x << 4) + ((mb->mv[0].x + 2) >> 2),
                                (y << 4) + ((mb->mv[0].y + 2) >> 2));
                    }
                    else if (mb->mbType == MBTYPE_B_L1_16x16)
                    {
                        if (l1)
                            DivideCost(l1->mb, w, h, propCost,
                                (x << 4) + ((mb->mv[1].x + 2) >> 2),
                                (y << 4) + ((mb->mv[1].y + 2) >> 2));
                    }
                    else if (mb->mbType == MBTYPE_B_Bi_16x16)
                    {
                        if (l0)
                            DivideCost(l0->mb, w, h, (propCost * mb->w0 + 32) >> 6,
                                (x << 4) + ((mb->mv[0].x + 2) >> 2),
                                (y << 4) + ((mb->mv[0].y + 2) >> 2));
                        if (l1)
                            DivideCost(l1->mb, w, h, (propCost * mb->w1 + 32) >> 6,
                                (x << 4) + ((mb->mv[1].x + 2) >> 2),
                                (y << 4) + ((mb->mv[1].y + 2) >> 2));
                    }
                    else
                    {
                        assert(!"invalid mb mode");
                    }
                }
            }
        }

        cur->propCost = 0;
        for (size_t i = 0; i < cur->mb.size(); i++)
            cur->propCost += cur->mb[i].propCost;
    }

    begin->m_vmeData->propCost = 0;
    for (size_t i = 0; i < begin->m_vmeData->mb.size(); i++)
        begin->m_vmeData->propCost += begin->m_vmeData->mb[i].propCost;
}

namespace FadeDetectionHistLSE
{
    mfxU16 GetSegments(
        mfxU32 const * Hist,
        mfxU16 SDiv,
        mfxU16 const * SMult,
        mfxU16 * Segment,
        mfxU16 NumSegments,
        mfxU16 range,
        mfxU32 * SumPixels = 0)
    {
        mfxU16 i, j, PeakPos = 0;
        mfxU32 N = 0, curr = 0, prev = 0, target;

        for (i = 0; i < range; i++)
        {
            N += Hist[i];

            if (Hist[i] > Hist[PeakPos])
                PeakPos = i;
        }

        for (i = 0; i < range; i++)
        {
            curr += Hist[i];

            for (j = 0; j < NumSegments; j++)
            {
                target = N * SMult[j] / SDiv;

                if (prev < target && curr >= target)
                    Segment[j] = i;
            }
            prev = curr;
        }

        if (SumPixels)
            *SumPixels = N;

        return PeakPos;
    }

    void TransformHist(
        mfxU32 const * refHist,
        mfxU32 * transHist,
        mfxI16 weight,
        mfxI16 offset,
        mfxU16 logWDc,
        mfxU16 range)
    {
        mfxI32 fract, add0, add1;
        mfxI16 i, map0, map1;

        for (i = 0; i < range; i++)
        {
            fract = (i * weight) & ((1 << logWDc) - 1);
            add1 = (refHist[i] * fract) >> logWDc;
            add0 = refHist[i] - add1;
            map0 = ((i * weight) >> logWDc) + offset;
            map1 = map0 + 1;

            if (map0 >= 0 && map0 < range)
                transHist[map0] += add0;

            if (map1 >= 0 && map1 < range)
                transHist[map1] += add1;
        }
    }

    mfxF64 HistDiffThrCoeff(mfxF64 CheckRange)
    {
        if (CheckRange <  20) return 10;
        if (CheckRange <  30) return 8.5;
        if (CheckRange <  40) return 8;
        if (CheckRange <  50) return 7.5;
        if (CheckRange <  60) return 7;
        if (CheckRange <  70) return 6.5;
        if (CheckRange <  80) return 6;
        if (CheckRange <  90) return 5.5;
        if (CheckRange < 100) return 5;
        return 4.5;
    }

    void CalcWeight(
        mfxU32 const * curHist,
        mfxU32 const * refHist,
        mfxU32 list,
        mfxU32 idx,
        mfxExtPredWeightTable & pwt)
    {
        const mfxU16 range = 256;
        const mfxU16 cmpHistOffset = 10;
        const mfxU16 cmpHistRange = range - cmpHistOffset - 16;
        const mfxU16 numSegments = 8;
        const mfxU16 SegmentMultiplier[numSegments] = { 2, 4, 6, 7, 8, 10, 12, 14 };
        const mfxU16 SegmentDivider = 16;
        const mfxU16 logWDc = 6;
        const mfxU16 rounding = (1 << (logWDc - 1));
        const mfxI16 maxWO = 127;

        if (idx > 31)
            return;

        list = !!list;

        pwt.LumaLog2WeightDenom = logWDc;
        pwt.ChromaLog2WeightDenom = 0;

        pwt.Weights[list][idx][0][0] = (1 << logWDc);
        pwt.Weights[list][idx][0][1] = 0;
        pwt.Weights[list][idx][1][0] = 0;
        pwt.Weights[list][idx][1][1] = 0;
        pwt.Weights[list][idx][2][0] = 0;
        pwt.Weights[list][idx][2][1] = 0;

        if (!curHist || !refHist)
            return;

        mfxU16 x[numSegments] = {};
        mfxU16 y[numSegments] = {};
        mfxI16 CurPeakPos = 0, RefPeakPos = 0, TransformedPeakPos = 0;
        mfxI32 sx = 0, sy = 0, sxx = 0, syy = 0, sxy = 0;
        mfxU32 CheckRange = 0, VarSampleDiff = 0;
        mfxI32 AveSampleDiff = 0;
        mfxU32 HistBuff[range] = {};
        mfxU32(&ScaledHist)[range] = HistBuff;
        mfxI16 Weight = 0, Offset = 0;
        mfxU32 HistDiffThr = 0;
        mfxU32 NumSamples = 0, NumSamplesRef = 0;
        mfxU32 sumClosedPixelsDiff = 0, numClosedPixels = 0;
        bool   stopCountClosedPixels = false;
        mfxU32 sumPixelHistDiff0 = 0, sumPixelHistDiff1 = 0, sumPixelHistDiff2 = 0;
        mfxI32 maxPixelHistDiff0 = 0, maxPixelHistDiff1 = 0, maxPixelHistDiff2 = 0;
        mfxU32 AveDiff = 0;
        mfxU32 const * cmpHist;

        RefPeakPos = GetSegments(refHist, SegmentDivider, SegmentMultiplier, x, numSegments, range, &NumSamplesRef);
        CurPeakPos = GetSegments(curHist, SegmentDivider, SegmentMultiplier, y, numSegments, range, &NumSamples);

        if (NumSamples == 0 || NumSamples != NumSamplesRef)
            return; // invalid or incomparable histograms

        for (mfxU16 i = 0; i < numSegments; i++)
        {
            sx += x[i];
            sy += y[i];
            sxx += x[i] * x[i];
            syy += y[i] * y[i];
            sxy += x[i] * y[i];

            mfxI32 diff = y[i] - x[i];

            AveSampleDiff += diff;
            VarSampleDiff += diff * diff;

            if (!stopCountClosedPixels)
            {
                if (abs(diff) <= 1)
                {
                    sumClosedPixelsDiff += abs(diff);
                    numClosedPixels++;
                }
                else
                    stopCountClosedPixels = true;
            }
        }

        VarSampleDiff = (VarSampleDiff * numSegments - AveSampleDiff * AveSampleDiff) / (numSegments * numSegments);
        AveSampleDiff = AveSampleDiff > 0 ? ((AveSampleDiff + numSegments / 2) / numSegments) : -((abs(AveSampleDiff) + numSegments / 2) / numSegments);
        CheckRange = y[numSegments - 1] - y[0];

        Weight = mfxI16(((sxy - ((sx * sy + numSegments / 2) / numSegments)) << logWDc) / std::max(1, (sxx - ((sx * sx + numSegments / 2) / numSegments))));
        Offset = mfxI16(sy - ((Weight * sx + rounding) >> logWDc));
        Offset = (Offset + (Offset > 0 ? numSegments / 2 : -numSegments / 2)) / numSegments;

        if (abs(Weight) < 4)
            return;

        if (abs(Weight) <= (1 << logWDc))
        {
            TransformHist(refHist, ScaledHist, Weight, Offset, logWDc, range);
            cmpHist = curHist;
        }
        else
        {
            mfxI16 Weight1 = mfxI16(((sxy - ((sx * sy + numSegments / 2) / numSegments)) << logWDc) / std::max(1, (syy - ((sy * sy + numSegments / 2) / numSegments))));
            mfxI16 Offset1 = mfxI16(sx - ((Weight1 * sy + rounding) >> logWDc));
            Offset1 = (Offset1 + (Offset1 > 0 ? numSegments / 2 : -numSegments / 2)) / numSegments;

            if (abs(Weight1) < 4)
                return;

            TransformHist(curHist, ScaledHist, Weight1, Offset1, logWDc, range);
            cmpHist = refHist;
        }

        for (mfxI32 i = cmpHistOffset, j, diff; i < (cmpHistOffset + cmpHistRange); i++)
        {
            j = i + AveSampleDiff;
            if (j > 0 && j < range)
            {
                diff = (mfxI32)curHist[j] - (mfxI32)refHist[i];
                sumPixelHistDiff0 += abs(diff);
                if (abs(diff) > maxPixelHistDiff0)
                    maxPixelHistDiff0 = abs(diff);
            }

            diff = (mfxI32)cmpHist[i] - (mfxI32)ScaledHist[i];
            sumPixelHistDiff1 += abs(diff);
            if (abs(diff) > maxPixelHistDiff1)
                maxPixelHistDiff1 = abs(diff);

            diff = (mfxI32)curHist[i] - (mfxI32)refHist[i];
            sumPixelHistDiff2 += abs(diff);
            if (abs(diff) > maxPixelHistDiff2)
                maxPixelHistDiff2 = abs(diff);
        }

        if (sumPixelHistDiff0 <= sumPixelHistDiff1 || abs(Weight) > maxWO || abs(Offset) > maxWO)
        {
            sumPixelHistDiff1 = sumPixelHistDiff0;
            maxPixelHistDiff1 = maxPixelHistDiff0;
            Weight = (1 << logWDc);
            Offset = mfxI16(AveSampleDiff);
        }

        if (sumPixelHistDiff2 < sumPixelHistDiff1)
        {
            sumPixelHistDiff1 = sumPixelHistDiff2;
            maxPixelHistDiff1 = maxPixelHistDiff2;
            Weight = (1 << logWDc);
            Offset = 0;
        }

        mfxI32 maxdiff = 0, orgmaxdiff = 0, transformedClosedPixelsDiff = 0, transformMaxDiff = 0;
        mfxI32 sumdiff = 0;

        for (mfxU16 i = 0; i < numSegments; i++)
        {
            mfxI16 transformedClosedPixel = (((x[i] * Weight + rounding) >> logWDc) + Offset);

            transformedClosedPixel = mfx::clamp<mfxI16>(transformedClosedPixel, 0, range - 1);

            if (i < numClosedPixels)
            {
                transformedClosedPixelsDiff += abs(y[i] - transformedClosedPixel);

                if (transformMaxDiff < abs(y[i] - transformedClosedPixel))
                    transformMaxDiff = abs(y[i] - transformedClosedPixel);
            }

            if (maxdiff < abs(y[i] - transformedClosedPixel))
                maxdiff = abs(y[i] - transformedClosedPixel);

            sumdiff += abs(y[i] - transformedClosedPixel);
        }

        pwt.LumaWeightFlag[list][idx] = 1;

        HistDiffThr = mfxU32(HistDiffThrCoeff(CheckRange) * NumSamples / 16);
        TransformedPeakPos = ((RefPeakPos * Weight + rounding) >> logWDc) + Offset;
        AveDiff = (sumdiff) / numSegments;

        if (    sumPixelHistDiff1 > HistDiffThr
            || (Weight == (1 << logWDc) && Offset == 0)
            || (abs(Weight) > maxWO || abs(Offset) > maxWO)
            || ((CheckRange > 35) && abs(AveSampleDiff) <= 1 && VarSampleDiff <= 1)
            || ((CheckRange > 35) && Weight != (1 << logWDc) && abs(AveSampleDiff) <= 1 && VarSampleDiff == 0)
            || ((CheckRange > 25) && abs(RefPeakPos - CurPeakPos) <= 1 && abs(TransformedPeakPos - CurPeakPos) > abs(RefPeakPos - CurPeakPos) + 2)
            || (numClosedPixels >= (numSegments / 2) && (CheckRange > 35) && (transformedClosedPixelsDiff - sumClosedPixelsDiff > 2 || transformMaxDiff >= 2))
            || (Weight == (1 << logWDc) && ((abs(AveSampleDiff) <= 2 && AveDiff > 1) || (AveDiff * AveDiff) > std::max(1u, VarSampleDiff / 2)))
            || (Weight != (1 << logWDc) && (maxdiff > 3 * abs(AveSampleDiff) || sumdiff >= 8 * abs(AveSampleDiff))))
        {
            pwt.LumaWeightFlag[list][idx] = 0;
        }


        if (pwt.LumaWeightFlag[list][idx])
        {
            bool fwFadeDetected = true, bwFadeDetected = true;

            for (mfxI16 i = 0; i < numSegments; i++)
            {
                mfxI16 curdiff = y[i] - x[i];

                if (i != 0)
                {
                    if (   (orgmaxdiff * curdiff < 0)
                        || (abs(orgmaxdiff) - abs(curdiff) > (abs(AveSampleDiff) > 15 ? 4 : 1)))
                    {
                        fwFadeDetected = false;
                    }
                }
                if (abs(orgmaxdiff) < abs(curdiff))
                    orgmaxdiff = curdiff;
            }

            orgmaxdiff = 0;

            for (mfxI16 i = numSegments - 1; i >= 0; i--)
            {
                mfxI16 curdiff = y[i] - x[i];

                if (i != numSegments - 1)
                {
                    if (   (orgmaxdiff * curdiff < 0)
                        || (abs(orgmaxdiff) - abs(curdiff) > (abs(AveSampleDiff) > 15 ? 4 : 1)))
                    {
                        bwFadeDetected = false;
                    }
                }
                if (abs(orgmaxdiff) < abs(curdiff))
                    orgmaxdiff = curdiff;
            }

            pwt.LumaWeightFlag[list][idx] = mfxU16(fwFadeDetected | bwFadeDetected);
        }

        if (pwt.LumaWeightFlag[list][idx])
        {
            pwt.Weights[list][idx][0][0] = Weight;
            pwt.Weights[list][idx][0][1] = Offset;
        }
    }
};


void MfxHwH264Encode::CalcPredWeightTable(
    DdiTask & task,
    mfxU32 MaxNum_WeightedPredL0,
    mfxU32 MaxNum_WeightedPredL1)
{
    //MaxNum_WeightedPredL0 = MaxNum_WeightedPredL1 = 1;
    mfxU32* curHist = (mfxU32*)task.m_cmHistSys;
    mfxU32* refHist = 0;
    mfxU32 MaxWPLX[2] = { MaxNum_WeightedPredL0, MaxNum_WeightedPredL1 };

    //if (curHist)
    //{
    //    for (mfxI32 i = 0; i < 256 * (task.m_fieldPicFlag + 1); i++)
    //        fprintf(stderr, "%3d ", curHist[i]);
    //    fprintf(stderr, "\n"); fflush(stderr);
    //}

    for (mfxU32 field = 0; field <= task.m_fieldPicFlag; field++)
    {
        mfxU32 fid = task.m_fid[field];
        Zero(task.m_pwt[fid]);
        mfxExtPredWeightTable const * external = GetExtBuffer(task.m_ctrl, field);
        bool fade = true;

        if (external)
        {
            if ((task.m_hwType >= MFX_HW_KBL) &&
                ((external->LumaLog2WeightDenom && external->LumaLog2WeightDenom != 6) ||
                (external->ChromaLog2WeightDenom && external->ChromaLog2WeightDenom != 6)))
            {
                task.m_pwt[fid] = *external;
                // ajust weights for Denom == 6
                if (external->LumaLog2WeightDenom && (external->LumaLog2WeightDenom != 6))
                {
                    for (mfxU8 i = 0; i < 2; i++)//list
                        for (mfxU8 j = 0; j < 32; j++)//entry
                        {
                            if (task.m_pwt[fid].LumaWeightFlag[i][j])
                            {
                                if (task.m_pwt[fid].LumaLog2WeightDenom <= 6)
                                    task.m_pwt[fid].Weights[i][j][0][0] = task.m_pwt[fid].Weights[i][j][0][0] * (1 << (6 - task.m_pwt[fid].LumaLog2WeightDenom));//2^(6-k)
                                else
                                    task.m_pwt[fid].Weights[i][j][0][0] /= (1 << (task.m_pwt[fid].LumaLog2WeightDenom - 6));//2^(-1)
                            }
                        }
                    task.m_pwt[fid].LumaLog2WeightDenom = 6;
                }
                if (external->ChromaLog2WeightDenom && (external->ChromaLog2WeightDenom != 6))
                {
                    for (mfxU8 i = 0; i < 2; i++)//list
                        for (mfxU8 j = 0; j < 32; j++)//entry
                        {
                            if (task.m_pwt[fid].ChromaWeightFlag[i][j])
                            {
                                if (task.m_pwt[fid].ChromaLog2WeightDenom <= 6)
                                {
                                    task.m_pwt[fid].Weights[i][j][1][0] = task.m_pwt[fid].Weights[i][j][1][0] * (1 << (6 - task.m_pwt[fid].ChromaLog2WeightDenom));//2^(6-k)
                                    task.m_pwt[fid].Weights[i][j][2][0] = task.m_pwt[fid].Weights[i][j][2][0] * (1 << (6 - task.m_pwt[fid].ChromaLog2WeightDenom));//2^(6-k)
                                }
                                else
                                {
                                    task.m_pwt[fid].Weights[i][j][1][0] /= (1 << (task.m_pwt[fid].ChromaLog2WeightDenom - 6));//2^(-1)
                                    task.m_pwt[fid].Weights[i][j][2][0] /= (1 << (task.m_pwt[fid].ChromaLog2WeightDenom - 6));//2^(-1)
                                }
                            }
                        }
                    task.m_pwt[fid].ChromaLog2WeightDenom = 6;
                }
            }
            continue;
        }

        //fprintf(stderr, "FO: %04d POC %04d:\n", task.m_frameOrder, task.GetPoc()[fid]); fflush(stderr);

        for (mfxU32 l = 0; l < 2; l++)
        {
            Pair<ArrayU8x33> const & ListX = l ? task.m_list1 : task.m_list0;

            for (mfxU32 i = 0; i < ListX[fid].Size() && i < MaxWPLX[l] && fade; i++)
            {
                mfxU8 refIdx = (ListX[fid][i] & 0x7F);
                mfxU8 refField = (ListX[fid][i] >> 7);

                refHist = (mfxU32*)task.m_dpb[fid][refIdx].m_cmHistSys;

                FadeDetectionHistLSE::CalcWeight(
                    curHist ? curHist + 256 * fid : 0,
                    refHist ? refHist + 256 * refField : 0,
                    l,
                    i,
                    task.m_pwt[fid]);

               //fprintf(stderr, "%04d[%d] vs. %04d[%d] ", task.m_frameOrder, fid, task.m_dpb[fid][refIdx].m_frameOrder, (mfxU32)refField); fflush(stderr);
               //fprintf(stderr, "LWF: %d W: %4d O: %4d\n",
               //    task.m_pwt[fid].LumaWeightFlag[l][i],
               //    task.m_pwt[fid].Weights[l][i][0][0],
               //    task.m_pwt[fid].Weights[l][i][0][1]); fflush(stderr);

                fade &= !!task.m_pwt[fid].LumaWeightFlag[l][i];
            }
        }

        if (!fade)
        {
            //fprintf(stderr, "There Is No Fade!\n"); fflush(stderr);

            for (mfxU32 l = 0; l < 2; l++)
            {
                for (mfxU32 i = 0; i < MaxWPLX[l]; i++)
                {
                    task.m_pwt[fid].LumaWeightFlag[l][i] = 0;
                    task.m_pwt[fid].Weights[l][i][0][0] = (1 << task.m_pwt[fid].LumaLog2WeightDenom);
                    task.m_pwt[fid].Weights[l][i][0][1] = 0;
                }
            }
        }
    }
}

AsyncRoutineEmulator::AsyncRoutineEmulator()
{
    std::fill(Begin(m_stageGreediness), End(m_stageGreediness), 1);
    Zero(m_queueFullness);
    Zero(m_queueFlush);
}

AsyncRoutineEmulator::AsyncRoutineEmulator(MfxVideoParam const & video, mfxU32  adaptGopDelay)
{
    Init(video, adaptGopDelay);
}

void AsyncRoutineEmulator::Init(MfxVideoParam const & video, mfxU32  adaptGopDelay)
{
    mfxExtCodingOption2 const & extOpt2 = GetExtBufferRef(video);


    switch (video.mfx.RateControlMethod)
    {
    case MFX_RATECONTROL_CQP:
        m_stageGreediness[STG_ACCEPT_FRAME] = 1;
        m_stageGreediness[STG_START_SCD] = 1;
        m_stageGreediness[STG_WAIT_SCD] = 1 + adaptGopDelay;
        m_stageGreediness[STG_START_MCTF]   = 1;
        m_stageGreediness[STG_WAIT_MCTF]    = IsMctfSupported(video) ? 2 : 1;
        m_stageGreediness[STG_START_LA    ] = video.mfx.EncodedOrder ? 1 : video.mfx.GopRefDist;
        m_stageGreediness[STG_WAIT_LA     ] = 1;
        m_stageGreediness[STG_START_HIST  ] = 1;
        m_stageGreediness[STG_WAIT_HIST   ] = 1;
        m_stageGreediness[STG_START_ENCODE] = 1;
        m_stageGreediness[STG_WAIT_ENCODE ] = 1 + !!(video.AsyncDepth > 1);
        if (video.AsyncDepth > 1)
        { // for parallel ENC/PAK optimization
            m_stageGreediness[STG_START_ENCODE] += !!(video.mfx.GopRefDist > 1);
            m_stageGreediness[STG_WAIT_ENCODE ] += !!(video.mfx.GopRefDist > 1) + !!(video.AsyncDepth > 2 && video.mfx.GopRefDist > 2);
        }
        break;
    case MFX_RATECONTROL_LA:
    case MFX_RATECONTROL_LA_ICQ:
    case MFX_RATECONTROL_LA_HRD:
        m_stageGreediness[STG_ACCEPT_FRAME] = 1;
        m_stageGreediness[STG_START_SCD] = 1;
        m_stageGreediness[STG_WAIT_SCD] = 1 + adaptGopDelay;
        m_stageGreediness[STG_START_MCTF]   = 1;
        m_stageGreediness[STG_WAIT_MCTF]    = IsMctfSupported(video) ? 2 : 1;
        m_stageGreediness[STG_START_LA    ] = video.mfx.EncodedOrder ? 1 : video.mfx.GopRefDist;
        m_stageGreediness[STG_WAIT_LA     ] = 1 + !!(video.AsyncDepth > 1);
        m_stageGreediness[STG_START_HIST  ] = 1;
        m_stageGreediness[STG_WAIT_HIST   ] = 1;
        m_stageGreediness[STG_START_ENCODE] = extOpt2.LookAheadDepth;
        m_stageGreediness[STG_WAIT_ENCODE ] = 1 + !!(video.AsyncDepth > 1);
        break;
    default:
        m_stageGreediness[STG_ACCEPT_FRAME] = 1;
        m_stageGreediness[STG_START_SCD] = 1;
        m_stageGreediness[STG_WAIT_SCD] = (IsExtBrcSceneChangeSupported(video) && IsCmNeededForSCD(video) ? 1 + !!(video.AsyncDepth > 1) : 1) + adaptGopDelay;
        m_stageGreediness[STG_START_MCTF]   = 1;
        m_stageGreediness[STG_WAIT_MCTF]    = IsMctfSupported(video) ? 2 : 1;
        m_stageGreediness[STG_START_LA    ] = video.mfx.EncodedOrder ? 1 : video.mfx.GopRefDist;
        m_stageGreediness[STG_WAIT_LA     ] = 1;
        m_stageGreediness[STG_START_HIST  ] = 1;
        m_stageGreediness[STG_WAIT_HIST   ] = 1;
        m_stageGreediness[STG_START_ENCODE] = 1;
        m_stageGreediness[STG_WAIT_ENCODE ] = 1 + !!(video.AsyncDepth > 1);
        break;
    }

    Zero(m_queueFullness);
    Zero(m_queueFlush);
}


mfxU32 AsyncRoutineEmulator::GetTotalGreediness() const
{
    mfxU32 greediness = 0;
    for (mfxU32 i = 0; i < STG_COUNT; i++)
        greediness += m_stageGreediness[i] - 1;
    return greediness + 1;
}

mfxU32 AsyncRoutineEmulator::GetStageGreediness(mfxU32 stage) const
{
    return (stage < STG_COUNT) ? m_stageGreediness[stage] : 0;
}

mfxU32 AsyncRoutineEmulator::CheckStageOutput(mfxU32 stage)
{
    mfxU32 in  = stage;
    mfxU32 out = stage + 1;
    mfxU32 hasOutput = 0;
    if (m_queueFullness[in] >= m_stageGreediness[stage] ||
        (m_queueFullness[in] > 0 && m_queueFlush[in]))
    {
        --m_queueFullness[in];
        ++m_queueFullness[out];
        hasOutput = 1;
    }

    m_queueFlush[out] = (m_queueFlush[in] && m_queueFullness[in] == 0);
    return hasOutput;
}


mfxU32 AsyncRoutineEmulator::Go(bool hasInput)
{
    if (hasInput)
        ++m_queueFullness[STG_ACCEPT_FRAME];
    else
        m_queueFlush[STG_ACCEPT_FRAME] = 1;

    mfxU32 stages = 0;
    for (mfxU32 i = 0; i < STG_COUNT; i++)
        stages += CheckStageOutput(i) << i;

    if (!hasInput && stages != 0 && (stages & STG_BIT_WAIT_ENCODE) == 0)
        stages += STG_BIT_RESTART;

    return stages;
}


#endif // MFX_ENABLE_H264_VIDEO_ENCODE_HW
