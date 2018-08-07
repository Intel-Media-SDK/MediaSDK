// Copyright (c) 2018 Intel Corporation
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
#if defined (MFX_ENABLE_H264_VIDEO_DECODE)

#ifndef __UMC_H264_SEGMENT_DECODER_DXVA_H
#define __UMC_H264_SEGMENT_DECODER_DXVA_H

#include "umc_va_base.h"

#include <memory>
#include "umc_h264_segment_decoder_base.h"
#include "umc_h264_task_broker.h"
#include "umc_h264_va_packer.h"
#include "umc_h264_frame.h"

#define UMC_VA_MAX_FRAME_BUFFER 32 //max number of uncompressed buffers

namespace UMC
{
class TaskSupplier;

class H264_DXVA_SegmentDecoderCommon : public H264SegmentDecoderBase
{
public:
    H264_DXVA_SegmentDecoderCommon(TaskSupplier * pTaskSupplier);

    VideoAccelerator *m_va;

    void SetVideoAccelerator(VideoAccelerator *va);

protected:
    TaskSupplier * m_pTaskSupplier;

private:
    H264_DXVA_SegmentDecoderCommon & operator = (H264_DXVA_SegmentDecoderCommon &)
    {
        return *this;

    }
};

class H264_DXVA_SegmentDecoder : public H264_DXVA_SegmentDecoderCommon
{
public:
    H264_DXVA_SegmentDecoder(TaskSupplier * pTaskSupplier);

    ~H264_DXVA_SegmentDecoder();

    // Initialize object
    virtual Status Init(int32_t iNumber);

    void PackAllHeaders(H264DecoderFrame * pFrame, int32_t field);

    virtual Status ProcessSegment(void);

    Packer * GetPacker() { return m_Packer.get();}

    virtual void Reset();

protected:

    std::unique_ptr<Packer>  m_Packer;

private:
    H264_DXVA_SegmentDecoder & operator = (H264_DXVA_SegmentDecoder &)
    {
        return *this;
    }
};

/****************************************************************************************************/
// DXVASupport class implementation
/****************************************************************************************************/
template <class BaseClass>
class DXVASupport
{
public:

    DXVASupport()
        : m_va(0)
        , m_Base(0)
    {
    }

    ~DXVASupport()
    {
    }

    void Init()
    {
        m_Base = (BaseClass*)(this);
        if (!m_va)
            return;
    }

    void Reset()
    {
        H264_DXVA_SegmentDecoder * dxva_sd = (H264_DXVA_SegmentDecoder*)(m_Base->m_pSegmentDecoder[0]);
        if (dxva_sd)
            dxva_sd->Reset();
    }

    void DecodePicture(H264DecoderFrame * pFrame, int32_t field)
    {
        if (!m_va)
            return;

        Status sts = m_va->BeginFrame(pFrame->GetFrameData()->GetFrameMID(), field);
        if (sts != UMC_OK)
            throw h264_exception(sts);

        H264_DXVA_SegmentDecoder * dxva_sd = (H264_DXVA_SegmentDecoder*)(m_Base->m_pSegmentDecoder[0]);
        VM_ASSERT(dxva_sd);

        for (uint32_t i = 0; i < m_Base->m_iThreadNum; i++)
        {
            ((H264_DXVA_SegmentDecoder *)m_Base->m_pSegmentDecoder[i])->SetVideoAccelerator(m_va);
        }

        dxva_sd->PackAllHeaders(pFrame, field);

        sts = m_va->EndFrame();
        if (sts != UMC_OK)
            throw h264_exception(sts);
    }

    void SetVideoHardwareAccelerator(VideoAccelerator * va)
    {
        if (va)
            m_va = (VideoAccelerator*)va;
    }

protected:
    VideoAccelerator *m_va;
    BaseClass * m_Base;
};

class TaskBrokerSingleThreadDXVA : public TaskBroker
{
public:
    TaskBrokerSingleThreadDXVA(TaskSupplier * pTaskSupplier);

    virtual bool PrepareFrame(H264DecoderFrame * pFrame);

    // Get next working task
    virtual bool GetNextTaskInternal(H264Task *pTask);
    virtual void Start();

    virtual void Reset();

    void DXVAStatusReportingMode(bool mode)
    {
        m_useDXVAStatusReporting = mode;
    }

protected:
    virtual void AwakeThreads();

    class ReportItem
    {
    public:
        uint32_t  m_index;
        uint32_t  m_field;
        uint8_t   m_status;

        ReportItem(uint32_t index, uint32_t field, uint8_t status)
            : m_index(index)
            , m_field(field)
            , m_status(status)
        {
        }

        bool operator == (const ReportItem & item)
        {
            return (item.m_index == m_index) && (item.m_field == m_field);
        }

        bool operator != (const ReportItem & item)
        {
            return (item.m_index != m_index) || (item.m_field != m_field);
        }
    };

    // Check cached feedback if found
    // frame removed from cache and marked as completed
    bool CheckCachedFeedbackAndComplete(H264DecoderFrameInfo * au);

    // Mark frame as Completed and update Error stasus
    // according to uiStatus reported by driver
    void SetCompletedAndErrorStatus(uint8_t uiStatus, H264DecoderFrameInfo * au);

    typedef std::vector<ReportItem> Report;
    Report m_reports;
    unsigned long long m_lastCounter;
    unsigned long long m_counterFrequency;

    bool   m_useDXVAStatusReporting;
};

} // namespace UMC

#endif /* __UMC_H264_SEGMENT_DECODER_DXVA_H */
#endif // MFX_ENABLE_H264_VIDEO_DECODE
