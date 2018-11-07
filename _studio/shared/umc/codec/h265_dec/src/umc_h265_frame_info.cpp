// Copyright (c) 2017-2018 Intel Corporation
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

#include "umc_h265_frame_info.h"

namespace UMC_HEVC_DECODER
{


bool H265DecoderFrameInfo::IsCompleted() const
{
    if (GetStatus() == H265DecoderFrameInfo::STATUS_COMPLETED)
        return true;

    return false;
}

void H265DecoderFrameInfo::Reset()
{
    Free();

    m_hasTiles = false;


    m_isNeedDeblocking = false;
    m_isNeedSAO = false;

    m_isIntraAU = true;
    m_hasDependentSliceSegments = false;
    m_WA_different_disable_deblocking = false;

    m_nextAU = 0;
    m_prevAU = 0;
    m_refAU = 0;

    m_Status = STATUS_NONE;
    m_prepared = 0;
    m_IsIDR = false;

    if (m_sps)
    {
        m_sps->DecrementReference();
        m_sps = 0;
    }
}

void H265DecoderFrameInfo::Free()
{
    size_t count = m_pSliceQueue.size();
    for (size_t i = 0; i < count; i ++)
    {
        H265Slice * pCurSlice = m_pSliceQueue[i];
        pCurSlice->Release();
        pCurSlice->DecrementReference();
    }

    m_SliceCount = 0;

    m_pSliceQueue.clear();
    m_prepared = 0;
}

void H265DecoderFrameInfo::RemoveSlice(int32_t num)
{
    H265Slice * pCurSlice = GetSlice(num);

    if (!pCurSlice) // nothing to do
        return;

    for (int32_t i = num; i < m_SliceCount - 1; i++)
    {
        m_pSliceQueue[i] = m_pSliceQueue[i + 1];
    }

    m_SliceCount--;
    m_pSliceQueue[m_SliceCount] = pCurSlice;
}

// Function works with a list of slices sorted by slice_segment_address
void H265DecoderFrameInfo::EliminateErrors()
{
    if (!GetSlice(0))
        return;

    // Remove dependent slices without a corresponding independent slice
    for (uint32_t sliceId = 0; sliceId < GetSliceCount(); sliceId++)
    {
        H265Slice * slice = GetSlice(sliceId);

        if (slice->GetSliceHeader()->dependent_slice_segment_flag)
        {
            RemoveSlice(sliceId);
            sliceId = uint32_t(-1);
            continue;
        }
        else
            break;
    }

    {
        // HEVC 7.4.7.1 General slice segment header semantics
        H265Slice *baseSlice = GetSlice(0); // after the for() loop above ,the first slice is treated as 'base' slice
        bool bIndepSliceMissing = false;
        for (uint32_t sliceId = 1; sliceId < GetSliceCount(); sliceId++)
        {
            H265SliceHeader *sliceHeader = GetSlice(sliceId)->GetSliceHeader();

            if (!sliceHeader->dependent_slice_segment_flag)
                bIndepSliceMissing = false;

            bool bSpecViolation = sliceHeader->slice_temporal_mvp_enabled_flag !=
                  baseSlice->GetSliceHeader()->slice_temporal_mvp_enabled_flag;

            bool bRemoveDependent = bIndepSliceMissing && sliceHeader->dependent_slice_segment_flag;

            if (bSpecViolation || bRemoveDependent)
            {
                RemoveSlice(sliceId);
                sliceId--;
                if (false == bIndepSliceMissing)
                    bIndepSliceMissing = !sliceHeader->dependent_slice_segment_flag;
            }
        }
    }

    // Remove slices with duplicated slice_segment_address syntax
    for (uint32_t sliceId = 0; sliceId < GetSliceCount(); sliceId++)
    {
        H265Slice * slice     = m_pSliceQueue[sliceId];
        H265Slice * nextSlice = GetSlice(sliceId + 1);

        if (!nextSlice)
            break;

        if (slice->GetFirstMB() == slice->GetMaxMB())
        {
            uint32_t sliceIdToRemove;

            // Heuristic logic:
            if (slice->GetSliceHeader()->dependent_slice_segment_flag && !nextSlice->GetSliceHeader()->dependent_slice_segment_flag)
            {
                // dependent slices are prone to errors
                sliceIdToRemove = sliceId;
            }
            else
            {
                // among two independent or dependent slices, prefer to keep the first slice
                sliceIdToRemove = sliceId + 1;
            }
            RemoveSlice(sliceIdToRemove);
            sliceId = uint32_t(-1);
            continue;
        }
    }
}

void H265DecoderFrameInfo::EliminateASO()
{
    static int32_t MAX_MB_NUMBER = 0x7fffffff;

    if (!GetSlice(0))
        return;

    uint32_t count = m_SliceCount;
    for (uint32_t sliceId = 0; sliceId < count; sliceId++)
    {
        H265Slice * curSlice = m_pSliceQueue[sliceId];
        int32_t minFirst = MAX_MB_NUMBER;
        uint32_t minSlice = 0;

        for (uint32_t j = sliceId; j < count; j++)
        {
            H265Slice * slice = m_pSliceQueue[j];
            if (slice->GetFirstMB() < curSlice->GetFirstMB() && minFirst > slice->GetFirstMB())
            {
                minFirst = slice->GetFirstMB();
                minSlice = j;
            }
        }

        if (minFirst != MAX_MB_NUMBER)
        {
            H265Slice * temp = m_pSliceQueue[sliceId];
            m_pSliceQueue[sliceId] = m_pSliceQueue[minSlice];
            m_pSliceQueue[minSlice] = temp;
        }
    }

    for (uint32_t sliceId = 0; sliceId < count; sliceId++)
    {
        H265Slice * slice     = m_pSliceQueue[sliceId];
        H265Slice * nextSlice = GetSlice(sliceId + 1);

        if (!nextSlice)
            break;

        slice->SetMaxMB(nextSlice->GetFirstMB());
    }
}

} // namespace UMC_HEVC_DECODER
#endif // MFX_ENABLE_H265_VIDEO_DECODE
