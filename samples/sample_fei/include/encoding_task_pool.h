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

#ifndef __SAMPLE_FEI_ENC_TASK_POOL_H__
#define __SAMPLE_FEI_ENC_TASK_POOL_H__

#include <mfxfei.h>
#include "refListsManagement_fei.h"

/* This class implements pool of iTasks and manage reordering and reference lists construction for income frame */
class iTaskPool
{
private:
    iTaskPool(const iTaskPool & other_pool);            // forbidden
    iTaskPool& operator= (const iTaskPool& other_pool); // forbidden

public:
    std::list<iTask*> task_pool;
    iTask* last_encoded_task;
    iTask* task_in_process;
    bool   reorder_frames;
    mfxU16 NInputLockers;
    mfxU16 GopRefDist;
    mfxU16 GopOptFlag;
    mfxU16 NumRefFrame;
    mfxU16 log2frameNumMax;
    mfxU16 refresh_limit;
    bool BPyramid;

    explicit iTaskPool(mfxU16 GopRefDist = 1, mfxU16 GopOptFlag = 0, mfxU16 NumRefFrame = 0, bool pyramid = false, mfxU16 limit = 1, mfxU16 log2frameNumMax = 8)
        : last_encoded_task(NULL)
        , task_in_process(NULL)
        , reorder_frames(true)
        , NInputLockers(0)
        , GopRefDist(GopRefDist)
        , GopOptFlag(GopOptFlag)
        , NumRefFrame(NumRefFrame)
        , log2frameNumMax(log2frameNumMax)
        , refresh_limit(limit)
        , BPyramid(pyramid)
    {}

    ~iTaskPool() { Clear(); }

    /* Update those fields that could be adjusted by MSDK internal checks */
    void Init(bool EncodedOrder, mfxU16 n_input_lockers, mfxU16 RefDist, mfxU16 OptFlag, mfxU16 NumRef, bool pyramid, mfxU16 limit, mfxU16 lg2frameNumMax)
    {
        reorder_frames  = EncodedOrder;
        NInputLockers   = n_input_lockers;
        GopRefDist      = RefDist;
        GopOptFlag      = OptFlag;
        NumRefFrame     = NumRef;
        log2frameNumMax = lg2frameNumMax;
        refresh_limit   = limit;
        BPyramid        = pyramid;
    }

    /* Release all pipeline resources */
    void Clear()
    {
        task_in_process = NULL;

        MSDK_SAFE_DELETE(last_encoded_task);

        for (std::list<iTask*>::iterator it = task_pool.begin(); it != task_pool.end(); ++it)
        {
            MSDK_SAFE_DELETE(*it);
        }

        task_pool.clear();
    }

    /* Add new income task */
    void AddTask(iTask* task){ task_pool.push_back(task); }

    /* Finish processing of current task and erase the oldest task in pool if refresh_limit is achieved */
    void UpdatePool()
    {
        if (reorder_frames)
        {
            if (!task_in_process) return;

            task_in_process->encoded = true;

            if (!last_encoded_task)
                last_encoded_task = new iTask(iTaskParams());

            *last_encoded_task = *task_in_process;

            task_in_process = NULL;
        }

        if (task_pool.size() >= refresh_limit)
        {
            RemoveProcessedTask();
        }
    }

    /* Find and erase the oldest processed task in pool that is not in DPB of last_encoded_task */
    void RemoveProcessedTask()
    {
        if (!reorder_frames)
        {
            for (std::list<iTask*>::iterator it = task_pool.begin(); it != task_pool.end(); ++it)
            {
                // If Encode in Display-Order mode, we can free the task when encoder releases input surface
                (*it)->encoded = (*it)->ENC_in.InSurface && (*it)->ENC_in.InSurface->Data.Locked == NInputLockers;

                if ((*it)->encoded)
                {
                    MSDK_SAFE_DELETE(*it);
                    task_pool.erase(it);
                    return;
                }
            }
        }
        else if (last_encoded_task)
        {
            // For all other cases, where manual reordering used
            ArrayDpbFrame & dpb = last_encoded_task->m_dpbPostEncoding;
            std::list<mfxU32> FramesInDPB;
            mfxU32 minUnencodedB = 0xffffffff;

            for (mfxU32 i = 0; i < dpb.Size(); i++)
                FramesInDPB.push_back(dpb[i].m_frameOrder);

            if (BPyramid)
            {
                for (std::list<iTask*>::iterator it = task_pool.begin(); it != task_pool.end(); ++it)
                {
                    if (!(*it)->encoded && ((*it)->m_type[(*it)->m_fid[0]] & MFX_FRAMETYPE_B))
                    {
                        minUnencodedB = (std::min)(minUnencodedB, (*it)->m_frameOrder);
                    }
                }
            }

            for (std::list<iTask*>::iterator it = task_pool.begin(); it != task_pool.end(); ++it)
            {
                bool protectRefInPyramid = BPyramid && ((*it)->m_type[(*it)->m_fid[0]] & MFX_FRAMETYPE_REF) && (minUnencodedB < (*it)->m_frameOrder);
                if (std::find(FramesInDPB.begin(), FramesInDPB.end(), (*it)->m_frameOrder) == FramesInDPB.end() && (*it)->encoded && !protectRefInPyramid) // delete task
                {
                    MSDK_SAFE_DELETE(*it);
                    task_pool.erase(it);
                    return;
                }
            }
        }
    }

    /* Find task in pool by its FrameOrder */
    iTask* GetTaskByFrameOrder(mfxU32 frame_order)
    {
        for (std::list<iTask*>::iterator it = task_pool.begin(); it != task_pool.end(); ++it)
        {
            if ((*it)->m_frameOrder == frame_order)
                return *it;
        }

        return NULL;
    }

    /* Find task in pool by its outPAK.OutSurface */
    iTask* GetTaskByPAKOutputSurface(mfxFrameSurface1 *surface)
    {
        for (std::list<iTask*>::iterator it = task_pool.begin(); it != task_pool.end(); ++it)
        {
            if ((*it)->PAK_out.OutSurface == surface)
                return *it;
        }

        return NULL;
    }

    /* Count unprocessed tasks */
    mfxU32 CountUnencodedFrames()
    {
        mfxU32 numUnencoded = 0;
        for (std::list<iTask*>::iterator it = task_pool.begin(); it != task_pool.end(); ++it){
            if (!(*it)->encoded){
                ++numUnencoded;
            }
        }

        return numUnencoded;
    }

    /* Adjust type of B frames without L1 references if not only GOP_OPT_STRICT is set */
    void ProcessLastB()
    {
        if (!(GopOptFlag & MFX_GOP_STRICT))
        {
            if (task_pool.back()->m_type[0] & MFX_FRAMETYPE_B) {
                task_pool.back()->m_type[0] = MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF;
            }
            if (task_pool.back()->m_type[1] & MFX_FRAMETYPE_B) {
                task_pool.back()->m_type[1] = MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF;
            }
        }
    }

    /* Block of functions below implements task reordering and reference lists management */

    /* Returns reordered task that could be processed or null if more frames are required for references
       (via GetReorderedTask).
       If task is found this function also fills DPB of this task */
    iTask* GetTaskToEncode(bool buffered_frames_processing)
    {
        if (!reorder_frames)
        {
            return !task_pool.empty() ? task_pool.back() : NULL;
        }

        // Reorder frame
        iTask* task = GetReorderedTask(buffered_frames_processing);

        if (!task) return NULL;

        //...........................reflist management........................................
        task->prevTask = last_encoded_task;

        // Update frame order of last I and IDR
        task->m_frameOrderIdr = (ExtractFrameType(*task) & MFX_FRAMETYPE_IDR) ? task->m_frameOrder : (task->prevTask ? task->prevTask->m_frameOrderIdr : 0);
        task->m_frameOrderI   = (ExtractFrameType(*task) & MFX_FRAMETYPE_I)   ? task->m_frameOrder : (task->prevTask ? task->prevTask->m_frameOrderI   : 0);

        // Wrap the FrameNum
        mfxU8  frameNumIncrement = (task->prevTask && (ExtractFrameType(*(task->prevTask)) & MFX_FRAMETYPE_REF || task->prevTask->m_nalRefIdc[0])) ? 1 : 0;
        task->m_frameNumWrap = task->m_frameNum = (task->prevTask && !(ExtractFrameType(*task) & MFX_FRAMETYPE_IDR)) ?
            mfxU16((task->prevTask->m_frameNum + frameNumIncrement) % (1 << log2frameNumMax)) : 0;

        // Increase IDR counter if previous frame was IDR
        task->m_frameIdrCounter = task->prevTask ? ((ExtractFrameType(*task->prevTask) & MFX_FRAMETYPE_IDR) && task->prevTask->m_frameOrder ? task->prevTask->m_frameIdrCounter + 1 : task->prevTask->m_frameIdrCounter) : 0;

        // Calculate PicNum
        task->m_picNum[1] = task->m_picNum[0] = task->m_frameNum * (task->m_fieldPicFlag + 1) + task->m_fieldPicFlag;

        // Fill DPB and RefLists
        task->m_dpb[task->m_fid[0]] = task->prevTask ? task->prevTask->m_dpbPostEncoding : ArrayDpbFrame();
        UpdateDpbFrames(*task, task->m_fid[0], 1 << log2frameNumMax);
        InitRefPicList(*task, task->m_fid[0]);
        ModifyRefPicLists(GopOptFlag, *task, task->m_fid[0]);
        MarkDecodedRefPictures(NumRefFrame, *task, task->m_fid[0]);
        if (task->m_fieldPicFlag)
        {
            UpdateDpbFrames(*task, task->m_fid[1], 1 << log2frameNumMax);
            InitRefPicList(*task, task->m_fid[1]);
            ModifyRefPicLists(GopOptFlag, *task, task->m_fid[1]);

            // mark second field of last added frame short-term ref
            task->m_dpbPostEncoding = task->m_dpb[task->m_fid[1]];
            if (task->m_reference[task->m_fid[1]])
                task->m_dpbPostEncoding.Back().m_refPicFlag[task->m_fid[1]] = 1;
        }

        // Adjust RefLists
        AdjustReflists(task);

        task_in_process = task;
        ShowDpbInfo(task, task->m_frameOrder);
        return task;
    }

    iTask* GetReorderedTask(bool buffered_frames_processing)
    {
        std::list<iTask*> unencoded_queue;
        for (std::list<iTask*>::iterator it = task_pool.begin(); it != task_pool.end(); ++it){
            if (!(*it)->encoded){
                unencoded_queue.push_back(*it);
            }
        }

        std::list<iTask*>::iterator top = ReorderFrame(unencoded_queue), begin = unencoded_queue.begin(), end = unencoded_queue.end();

        bool flush = unencoded_queue.size() < GopRefDist && buffered_frames_processing;

        if (flush && top == end && begin != end)
        {
            if (!!(GopOptFlag & MFX_GOP_STRICT))
            {
                top = begin; // TODO: reorder remaining B frames for B-pyramid when enabled
            }
            else
            {
                top = end;
                --top;
                //assert((*top)->frameType & MFX_FRAMETYPE_B);
                (*top)->m_type[0] = MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF;
                (*top)->m_type[1] = MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF;
                top = ReorderFrame(unencoded_queue);
                //assert(top != end || begin == end);
            }
        }

        if (top == end)
            return NULL; //skip B without encoded refs

        return *top;
    }

    /* Finds earliest unencoded frame in display order which references are encoded */
    std::list<iTask*>::iterator ReorderFrame(std::list<iTask*>& unencoded_queue)
    {
        std::list<iTask*>::iterator top = unencoded_queue.begin(), end = unencoded_queue.end();
        while (top != end &&                                     // go through buffered frames in display order and
            (((ExtractFrameType(**top) & MFX_FRAMETYPE_B) &&     // get earliest non-B frame
            CountFutureRefs((*top)->m_frameOrder) == 0) ||       // or B frame with L1 reference
            (*top)->encoded))                                    // which is not encoded yet
        {
            ++top;
        }

        if (top != end && (ExtractFrameType(**top) & MFX_FRAMETYPE_B))
        {
            // special case for B frames (when B pyramid is enabled)
            std::list<iTask*>::iterator i = top;
            while (++i != end &&                                          // check remaining
                (ExtractFrameType(**i) & MFX_FRAMETYPE_B) &&              // B frames
                ((*i)->m_loc.miniGopCount == (*top)->m_loc.miniGopCount)) // from the same mini-gop
            {
                if (!(*i)->encoded && (*top)->m_loc.encodingOrder > (*i)->m_loc.encodingOrder)
                    top = i;
            }
        }

        return top;
    }

    /* Counts encoded future references of frame with given display order */
    mfxU32 CountFutureRefs(mfxU32 frameOrder)
    {
        mfxU32 count = 0;

        for (std::list<iTask*>::iterator it = task_pool.begin(); it != task_pool.end(); ++it)
            if (frameOrder < (*it)->m_frameOrder && (*it)->encoded)
                ++count;

        return count;
    }

    /* By standard L0/L1 lists contains of identical set of references.
       This function removes excessive entries from RefLists */
    void AdjustReflists(iTask* task)
    {
        if (!task) return;

        mfxU32 fid, n_l0, n_l1, numOfFields = task->m_fieldPicFlag ? 2 : 1;

        for (mfxU32 fieldId = 0; fieldId < numOfFields; fieldId++)
        {
            fid = task->m_fid[fieldId];

            /* in some cases l0 and l1 lists are equal, if so same ref lists for l0 and l1 should be used*/
            n_l0 = task->GetNBackward(fieldId);
            n_l1 = task->GetNForward(fieldId);

            if (!n_l0 && task->m_list0[fid].Size() && !(task->m_type[fid] & MFX_FRAMETYPE_I))
            {
                n_l0 = task->m_list0[fid].Size();
            }

            if (!n_l1 && task->m_list1[fid].Size() && (task->m_type[fid] & MFX_FRAMETYPE_B))
            {
                n_l1 = task->m_list1[fid].Size();
            }

            task->m_list0[fid].Resize(n_l0);
            task->m_list1[fid].Resize(n_l1);
        }
    }

    /* These functions prints out state of DPB and reference lists for given frame.
    It is disabled by default in release build */

    const char* getFrameType(mfxU8 type)
    {
        switch (type & (MFX_FRAMETYPE_IPB | MFX_FRAMETYPE_IDR | MFX_FRAMETYPE_REF))
        {
        case MFX_FRAMETYPE_I:
            if (type & MFX_FRAMETYPE_IDR) {
                return "IDR";
            }
            else {
                return "I";
            }
        case MFX_FRAMETYPE_P:
            return "P";
        case MFX_FRAMETYPE_B:
            if (type & MFX_FRAMETYPE_REF) {
                return "B-ref";
            }
            else {
                return "B";
            }
        default:
            return "?";
        }
    }

    void ShowDpbInfo(iTask *task, int frame_order)
    {
        mdprintf(stderr, "\n\n--------------Show DPB Info of frame %d-------\n", frame_order);
        mfxU32 numOfFields = task->m_fieldPicFlag ? 2 : 1;
        for (mfxU32 j = 0; j < numOfFields; j++) {
            mdprintf(stderr, "\t[%d]: List dpb frame of frame %d in (frame_order, frame_num, POC):\n\t\tDPB:", j, task->m_frameOrder);
            for (mfxU32 i = 0; i < task->m_dpb[task->m_fid[j]].Size(); i++) {
                DpbFrame & ref = task->m_dpb[task->m_fid[j]][i];
                mdprintf(stderr, "(%d, %d, %d) ", ref.m_frameOrder, ref.m_frameNum, ref.m_poc[j]);
            }
            mdprintf(stderr, "\n\t\tL0: ");
            for (mfxU32 i = 0; i < task->m_list0[task->m_fid[j]].Size(); i++) {
                mdprintf(stderr, "%d ", task->m_list0[task->m_fid[j]][i]);
            }
            mdprintf(stderr, "\n\t\tL1: ");
            for (mfxU32 i = 0; i < task->m_list1[task->m_fid[j]].Size(); i++) {
                mdprintf(stderr, "%d ", task->m_list1[task->m_fid[j]][i]);
            }
            //mdprintf(stderr, "\n\t\tm_dpbPostEncoding: ");
            //for (mfxU32 i = 0; i < task->m_dpbPostEncoding.Size(); i++) {
            //    DpbFrame & ref = task->m_dpbPostEncoding[i];
            //    mdprintf(stderr, "(%d, %d, %d) ", ref.m_frameOrder, ref.m_frameNum, ref.m_poc[j]);
            //}
            mdprintf(stderr, "\n-------------------------------------------\n");
        }
    }
};

#if MFX_VERSION >= 1023

struct RefInfo
{
    std::vector<mfxFrameSurface1*> reference_frames; // to fill mfxPAKInput::L0Surface array

    // In this implementation DPB_after is not changed after encoding of second field
    // so we don't need two structures to hold the state of DPB_after
    std::vector<mfxExtFeiPPS::mfxExtFeiPpsDPB> DPB_before, DPB_after;

    std::vector<mfxExtFeiSliceHeader::mfxSlice::mfxSliceRef> L0[2], L1[2];

    void Clear()
    {
        reference_frames.clear();
        DPB_before.clear();
        DPB_after.clear();

        L0[0].clear(); L0[1].clear();
        L1[0].clear(); L1[1].clear();
    }

    mfxStatus FillRefList(iTask* eTask, iTaskPool* inputTasks, ArrayU8x33* task_list, std::vector<mfxExtFeiSliceHeader::mfxSlice::mfxSliceRef> * struct_list)
    {
        iTask* ref_task = NULL;
        mfxFrameSurface1* ref_surface = NULL;
        std::vector<mfxFrameSurface1*>::iterator rslt;
        mfxU16 cur_PicType, fid;

        for (mfxU32 fieldId = 0; fieldId < mfxU32(1 + eTask->m_fieldPicFlag); ++fieldId)
        {
            fid = eTask->m_fid[fieldId];

            if (!(eTask->m_type[fid] & MFX_FRAMETYPE_I))
            {
                for (mfxU8 const * instance = task_list[fid].Begin(); instance != task_list[fid].End(); ++instance)
                {
                    ref_task = inputTasks->GetTaskByFrameOrder(eTask->m_dpb[fid][*instance & 127].m_frameOrder);
                    MSDK_CHECK_POINTER(ref_task, MFX_ERR_NULL_PTR);

                    ref_surface = ref_task->PAK_out.OutSurface; // this is shared output surface for ENC reference / PAK reconstruct
                    MSDK_CHECK_POINTER(ref_surface, MFX_ERR_NULL_PTR);

                    cur_PicType = PicStructToFrameTypeFieldBased(ref_surface->Info.PicStruct & 0x0f, eTask->m_fieldPicFlag, (*instance) >> 7);
                    MSDK_CHECK_ERROR(cur_PicType, MFX_PICTYPE_UNKNOWN, MFX_ERR_UNSUPPORTED);

                    rslt = std::find(reference_frames.begin(), reference_frames.end(), ref_surface);
                    MSDK_CHECK_ERROR(rslt, reference_frames.end(), MFX_ERR_UNSUPPORTED); // surface from reflist not in DPB (should never happen)

                    mfxExtFeiSliceHeader::mfxSlice::mfxSliceRef newEntry = { cur_PicType, static_cast<mfxU16>(std::distance(reference_frames.begin(), rslt)) };
                    struct_list[fieldId].push_back(newEntry);
                }
            }
        }

        return MFX_ERR_NONE;
    }

    mfxStatus Fill(iTask* eTask, iTaskPool* inputTasks)
    {
        MSDK_CHECK_POINTER(eTask,                     MFX_ERR_NULL_PTR);
        MSDK_CHECK_POINTER(eTask->PAK_out.OutSurface, MFX_ERR_NULL_PTR);
        MSDK_CHECK_POINTER(inputTasks,                MFX_ERR_NULL_PTR);

        mfxStatus sts = MFX_ERR_NONE;

        Clear();

        iTask* ref_task = NULL;
        mfxFrameSurface1* ref_surface = NULL;
        std::vector<mfxFrameSurface1*>::iterator rslt;
        mfxU16 cur_PicType;

        // Fill shared list of reference surfaces and DPB states: before and after encoding
        for (mfxU32 i = 0; i < 2; ++i)
        {
            ArrayDpbFrame & curr_task_DPB = i ? eTask->m_dpbPostEncoding : eTask->m_dpb[eTask->m_fid[0]];
            std::vector<mfxExtFeiPPS::mfxExtFeiPpsDPB> & curr_struct_DPB = i ? DPB_after : DPB_before;

            for (DpbFrame* instance = curr_task_DPB.Begin(); instance != curr_task_DPB.End(); ++instance)
            {
                ref_task = inputTasks->GetTaskByFrameOrder(instance->m_frameOrder);
                MSDK_CHECK_POINTER(ref_task, MFX_ERR_NULL_PTR);

                ref_surface = ref_task->PAK_out.OutSurface; // this is shared output surface for ENC reference / PAK reconstruct
                MSDK_CHECK_POINTER(ref_surface, MFX_ERR_NULL_PTR);

                rslt = std::find(reference_frames.begin(), reference_frames.end(), ref_surface);

                if (rslt == reference_frames.end())
                {
                    reference_frames.push_back(ref_surface);
                    rslt = reference_frames.begin() + reference_frames.size() - 1;
                }

                cur_PicType = PicStructToFrameType(ref_surface->Info.PicStruct & 0x0f);
                MSDK_CHECK_ERROR(cur_PicType, MFX_PICTYPE_UNKNOWN, MFX_ERR_UNSUPPORTED);

                mfxExtFeiPPS::mfxExtFeiPpsDPB newFrame = { mfxU16(std::distance(reference_frames.begin(), rslt)), cur_PicType, instance->m_frameNumWrap, 0xffff };
                curr_struct_DPB.push_back(newFrame);
            }
        }

        // Fill reflists for SliceHeader
        sts = FillRefList(eTask, inputTasks, eTask->m_list0, L0);
        MSDK_CHECK_STATUS(sts, "FillRefList L0 failed");

        sts = FillRefList(eTask, inputTasks, eTask->m_list1, L1);
        MSDK_CHECK_STATUS(sts, "FillRefList L1 failed");

        return sts;
    }
};
#endif // MFX_VERSION >= 1023

#endif // __SAMPLE_FEI_ENC_TASK_POOL_H__