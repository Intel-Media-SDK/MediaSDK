/******************************************************************************\
Copyright (c) 2005-2017, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This sample was distributed or derived from the Intel's Media Samples package.
The original version of this sample may be obtained from https://software.intel.com/en-us/intel-media-server-studio
or https://software.intel.com/en-us/media-client-solutions-support.
\**********************************************************************************/

#include "refListsManagement_fei.h"

mfxU8 GetNumReorderFrames(mfxU16 GopRefDist, mfxU16 BRefType)
{
    mfxU8 numReorderFrames = GopRefDist > 1 ? 1 : 0;

    if (GopRefDist > 2 && BRefType == MFX_B_REF_PYRAMID)
    {
        numReorderFrames = mfxU8((std::max)(CeilLog2(GopRefDist - 1), mfxU32(1)));
    }

    return numReorderFrames;
}

mfxU8 GetDefaultLog2MaxPicOrdCnt(mfxU16 GopRefDist, mfxU16 BRefType)
{
    mfxU32 maxPocDiff = (GetNumReorderFrames(GopRefDist, BRefType) * GopRefDist + 1) * 2;

    mfxU32 log2MaxPoc = CeilLog2(2 * maxPocDiff - 1);
    return mfxU8((std::max)(log2MaxPoc, mfxU32(4)));
}

void UpdateDpbFrames(
    iTask&    task,
    mfxU32    field,
    mfxU32    frameNumMax)
{
    mfxU32 ps = task.PicStruct;

    for (mfxU32 i = 0; i < task.m_dpb[field].Size(); i++)
    {
        DpbFrame & ref = task.m_dpb[field][i];

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
            ref.m_picNum[0] = 2 * ref.m_frameNumWrap + (!field);
            ref.m_picNum[1] = 2 * ref.m_frameNumWrap + (!!field);
        }
    }
}

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

void InitRefPicList(
    iTask&    task,
    mfxU32    field)
{
    ArrayU8x33 list0Frm(0xff); // list0 built like current picture is frame
    ArrayU8x33 list1Frm(0xff); // list1 built like current picture is frame

    ArrayU8x33    & list0 = task.m_list0[field];
    ArrayU8x33    & list1 = task.m_list1[field];
    ArrayDpbFrame & dpb   = task.m_dpb[field];

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
            if (!dpb[i].m_longterm)
                list0Frm.PushBack(mfxU8(i));

        std::sort(list0Frm.Begin(), list0Frm.End(), RefPicNumIsGreater(dpb));

        mfxU8 * firstLongTerm = list0Frm.End();

        for (mfxU32 i = 0; i < dpb.Size(); i++)
            if (dpb[i].m_longterm)
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
            if (!dpb[i].m_longterm)
            {
                if (dpb[i].m_poc[0] <= GetPoc(task, 0))
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
            if (dpb[i].m_longterm)
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

    if (task.PicStruct & MFX_PICSTRUCT_PROGRESSIVE)
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


void ModifyRefPicLists(
    mfxU16    GopOptFlag,
    iTask&    task,
    mfxU32    fieldId)
{
    ArrayDpbFrame const & dpb = task.m_dpb[fieldId];
    ArrayU8x33 &          list0 = task.m_list0[fieldId];
    ArrayU8x33 &          list1 = task.m_list1[fieldId];
    mfxU32                ps = task.PicStruct;

    ArrayU8x33 initList0 = task.m_list0[fieldId];
    ArrayU8x33 initList1 = task.m_list1[fieldId];

    mfxU32 ffid = GetFirstField(task);
    bool isField = (task.PicStruct & (MFX_PICSTRUCT_FIELD_TFF | MFX_PICSTRUCT_FIELD_BFF)) != 0;
    bool isIPFieldPair = (task.m_type[ffid] & MFX_FRAMETYPE_I) && (task.m_type[!ffid] & MFX_FRAMETYPE_P);

    if ((GopOptFlag & MFX_GOP_CLOSED) || (task.m_frameOrderI < task.m_frameOrder))
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

    mfxU32 numActiveRefL1 = task.NumRefActiveBL1;
    mfxU32 numActiveRefL0 = (task.m_type[fieldId] & MFX_FRAMETYPE_P)
        ? task.NumRefActiveP
        : task.NumRefActiveBL0;


    //bool bCanApplyRefCtrl = video.calcParam.numTemporalLayer == 0 || video.mfx.GopRefDist == 1;

    // form modified ref pic list using internal MSDK logic
    {
        // prepare ref list for P-field of I/P field pair
        // swap 1st and 2nd entries of L0 ref pic list to use I-field of I/P pair as reference for P-field
        if (isIPFieldPair)
        {
            if (ps != MFX_PICSTRUCT_PROGRESSIVE && fieldId != ffid && list0.Size() > 1)
                std::swap(list0[0], list0[1]);
        }

        if (task.BRefType == MFX_B_REF_PYRAMID && (task.m_type[0] & MFX_FRAMETYPE_P))
        {
            if (task.PicStruct & MFX_PICSTRUCT_PROGRESSIVE)
                std::sort(list0.Begin(), list0.End(), RefPocIsGreater(dpb));
            else
            {
                // reorder list0 in POC descending order separately for each parity
                for (mfxU8* l = list0.Begin(); l < list0.End(); l++)
                    for (mfxU8* r = l + 1; r < list0.End(); r++)
                        if (((*l) >> 7) == ((*r) >> 7) && GetPoc(dpb, *r) > GetPoc(dpb, *l))
                            std::swap(*l, *r);
            }
        }

        if (isField && isIPFieldPair == false)
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
                //assert(!"No field of same parity for reference");
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
                    //assert(!"No field of same parity for reference");
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
}

void MarkDecodedRefPictures(
    mfxU16    NumRefFrame,
    iTask &   task,
    mfxU32    fid)
{
    // declare shorter names
    ArrayDpbFrame const &  initDpb = task.m_dpb[fid];
    ArrayDpbFrame &        currDpb = (fid == task.m_fid[1]) ? task.m_dpbPostEncoding : task.m_dpb[!fid];
    mfxU32                 type    = task.m_type[fid];
    DecRefPicMarkingInfo & marking = task.m_decRefPicMrk[fid];

    // marking commands will be applied to dpbPostEncoding
    // initial dpb stay unchanged
    currDpb = initDpb;

    if ((type & MFX_FRAMETYPE_REF) == 0)
        return; // non-reference frames don't change dpb


    if (type & MFX_FRAMETYPE_IDR)
    {
        currDpb.Resize(0);

        marking.long_term_reference_flag = 0;

        DpbFrame newDpbFrame;
        InitNewDpbFrame(newDpbFrame, task, fid);
        currDpb.PushBack(newDpbFrame);
    }
    else
    {
        mfxU32 ffid = GetFirstField(task);

        bool currFrameIsAddedToDpb = (fid != ffid) && (task.m_type[ffid] & MFX_FRAMETYPE_REF);

        // if first field was a reference then entire frame is already in dpb
        if (!currFrameIsAddedToDpb)
        {

            {
                if (currDpb.Size() == NumRefFrame)
                {
                    DpbFrame * toRemove = std::min_element(currDpb.Begin(), currDpb.End(), OrderByFrameNumWrap);
                    DpbFrame * toRemoveDefault = toRemove;

                    if (toRemove != currDpb.End()
                        && task.m_fieldPicFlag
                        && task.BRefType == MFX_B_REF_PYRAMID
                        && toRemove == std::min_element(currDpb.Begin(), currDpb.End(), OrderByNearestPrev(task.m_frameOrder)))
                    {
                        toRemove = std::min_element(currDpb.Begin(), currDpb.End(), OrderByDisplayOrder);
                    }

                    //assert(toRemove != currDpb.End());
                    if (toRemove == currDpb.End())
                        return;

                    if (toRemove != toRemoveDefault)
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
                currDpb.PushBack(newDpbFrame);
                //assert(currDpb.Size() <= video.mfx.NumRefFrame);
            }
        }
    }
}
