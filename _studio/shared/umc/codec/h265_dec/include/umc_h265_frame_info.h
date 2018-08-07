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

#include "umc_defs.h"
#ifdef MFX_ENABLE_H265_VIDEO_DECODE

#ifndef __UMC_H265_FRAME_INFO_H
#define __UMC_H265_FRAME_INFO_H

#include <vector>
#include "umc_h265_frame.h"
#include "umc_h265_slice_decoding.h"

namespace UMC_HEVC_DECODER
{
class H265DecoderFrame;


// Collection of slices that constitute one decoder frame
class H265DecoderFrameInfo
{
public:
    enum FillnessStatus
    {
        STATUS_NONE,
        STATUS_NOT_FILLED,
        STATUS_FILLED,
        STATUS_COMPLETED,
        STATUS_STARTED
    };

    H265DecoderFrameInfo(H265DecoderFrame * pFrame, Heap_Objects * pObjHeap)
        : m_pFrame(pFrame)
        , m_prepared(0)
        , m_sps(0)
        , m_SliceCount(0)
        , m_pObjHeap(pObjHeap)
    {
        Reset();
    }

    virtual ~H265DecoderFrameInfo()
    {
    }

    H265Slice *GetAnySlice() const
    {
        H265Slice* slice = GetSlice(0);
        if (!slice)
            throw h265_exception(UMC::UMC_ERR_FAILED);
        return slice;
    }

    const H265SeqParamSet *GetSeqParam() const
    {
        return m_sps;
    }

    void AddSlice(H265Slice * pSlice)
    {
        m_pSliceQueue.push_back(pSlice);
        m_SliceCount++;

        const H265SliceHeader &sliceHeader = *(pSlice->GetSliceHeader());

        m_isIntraAU = m_isIntraAU && (sliceHeader.slice_type == I_SLICE);
        m_IsIDR     = sliceHeader.IdrPicFlag != 0;
        m_hasDependentSliceSegments = m_hasDependentSliceSegments || sliceHeader.dependent_slice_segment_flag;
        m_isNeedDeblocking = m_isNeedDeblocking || (!sliceHeader.slice_deblocking_filter_disabled_flag);
        m_isNeedSAO = m_isNeedSAO || (sliceHeader.slice_sao_luma_flag || sliceHeader.slice_sao_chroma_flag);
        m_hasTiles = pSlice->GetPicParam()->getNumTiles() > 1;

        m_WA_different_disable_deblocking = m_WA_different_disable_deblocking || (sliceHeader.slice_deblocking_filter_disabled_flag != m_pSliceQueue[0]->GetSliceHeader()->slice_deblocking_filter_disabled_flag);

        if (!m_sps)
        {
            m_sps = (H265SeqParamSet *)pSlice->GetSeqParam();
            m_sps->IncrementReference();
        }
    }

    uint32_t GetSliceCount() const
    {
        return m_SliceCount;
    }

    H265Slice* GetSlice(int32_t num) const
    {
        if (num < 0 || num >= m_SliceCount)
            return 0;
        return m_pSliceQueue[num];
    }

    H265Slice* GetSliceByNumber(int32_t num) const
    {
        size_t count = m_pSliceQueue.size();
        for (uint32_t i = 0; i < count; i++)
        {
            if (m_pSliceQueue[i]->GetSliceNum() == num)
                return m_pSliceQueue[i];
        }

        return 0;
    }

    int32_t GetPositionByNumber(int32_t num) const
    {
        size_t count = m_pSliceQueue.size();
        for (uint32_t i = 0; i < count; i++)
        {
            if (m_pSliceQueue[i]->GetSliceNum() == num)
                return i;
        }

        return -1;
    }

    void Reset();

    void SetStatus(FillnessStatus status)
    {
        m_Status = status;
    }

    FillnessStatus GetStatus () const
    {
        return m_Status;
    }

    void Free();
    void RemoveSlice(int32_t num);

    bool IsNeedDeblocking() const
    {
        return m_isNeedDeblocking;
    }

    bool IsNeedSAO() const
    {
        return m_isNeedSAO;
    }

    bool IsCompleted() const;

    bool IsIntraAU() const
    {
        return m_isIntraAU;
    }

    bool IsReference() const
    {
        return m_pFrame->m_isUsedAsReference;
    }

    bool HasDependentSliceSegments() const
    {
        return m_hasDependentSliceSegments;
    }

    bool IsNeedWorkAroundForDeblocking() const
    {
        return m_WA_different_disable_deblocking;
    }

    // Initialize tiles and slices threading information
    void FillTileInfo();

    // reorder slices and set maxCUAddr
    void EliminateASO();

    // Remove corrupted slices
    void EliminateErrors();

    H265DecoderFrameInfo * GetNextAU() const {return m_nextAU;}
    H265DecoderFrameInfo * GetPrevAU() const {return m_prevAU;}
    H265DecoderFrameInfo * GetRefAU() const {return m_refAU;}

    void SetNextAU(H265DecoderFrameInfo *au) {m_nextAU = au;}
    void SetPrevAU(H265DecoderFrameInfo *au) {m_prevAU = au;}
    void SetRefAU(H265DecoderFrameInfo *au) {m_refAU = au;}


    bool   m_hasTiles;

    H265DecoderFrame * m_pFrame;
    int32_t m_prepared;
    bool m_IsIDR;

private:

    FillnessStatus m_Status;

    H265SeqParamSet * m_sps;
    std::vector<H265Slice*> m_pSliceQueue;

    int32_t m_SliceCount;

    Heap_Objects * m_pObjHeap;
    bool m_isNeedDeblocking;
    bool m_isNeedSAO;

    bool m_isIntraAU;
    bool m_hasDependentSliceSegments;

    bool m_WA_different_disable_deblocking;

    H265DecoderFrameInfo *m_nextAU;
    H265DecoderFrameInfo *m_prevAU;
    H265DecoderFrameInfo *m_refAU;
};

} // namespace UMC_HEVC_DECODER

#endif // __UMC_H265_FRAME_INFO_H
#endif // MFX_ENABLE_H265_VIDEO_DECODE
