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

#ifndef __UMC_H265_SEGMENT_DECODER_DXVA_H
#define __UMC_H265_SEGMENT_DECODER_DXVA_H

#include "umc_va_base.h"

#include <memory>
#include "umc_h265_segment_decoder_base.h"
#include "umc_h265_task_broker.h"
#include "umc_h265_va_packer.h"
#include "umc_h265_frame_info.h"

#include <mfx_trace.h>

#define UMC_VA_MAX_FRAME_BUFFER 32 //max number of uncompressed buffers

namespace UMC_HEVC_DECODER
{
class TaskSupplier_H265;

class H265_DXVA_SegmentDecoderCommon : public H265SegmentDecoderBase
{
public:
    H265_DXVA_SegmentDecoderCommon(TaskSupplier_H265 * pTaskSupplier);

    UMC::VideoAccelerator *m_va;

    void SetVideoAccelerator(UMC::VideoAccelerator *va);

protected:

    TaskSupplier_H265 * m_pTaskSupplier;

private:
    H265_DXVA_SegmentDecoderCommon & operator = (H265_DXVA_SegmentDecoderCommon &)
    {
        return *this;

    }
};

class H265_DXVA_SegmentDecoder : public H265_DXVA_SegmentDecoderCommon
{
public:
    H265_DXVA_SegmentDecoder(TaskSupplier_H265 * pTaskSupplier);

    ~H265_DXVA_SegmentDecoder();

    // Initialize object
    virtual UMC::Status Init(int32_t iNumber);

    void PackAllHeaders(H265DecoderFrame * pFrame);

    virtual UMC::Status ProcessSegment(void);

    int32_t m_CurrentSliceID;

    Packer * GetPacker() { return m_Packer.get();}

protected:

    std::unique_ptr<Packer>  m_Packer;

private:
    H265_DXVA_SegmentDecoder & operator = (H265_DXVA_SegmentDecoder &)
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

    void StartDecodingFrame(H265DecoderFrame * pFrame)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "DXVASupport::StartDecodingFrame");
        if (!m_va)
            return;

        UMC::Status sts = m_va->BeginFrame(pFrame->GetFrameMID());
        if (sts != UMC::UMC_OK)
            throw h265_exception(sts);

        H265_DXVA_SegmentDecoder * dxva_sd = (H265_DXVA_SegmentDecoder*)(m_Base->m_pSegmentDecoder[0]);
        VM_ASSERT(dxva_sd);

        for (uint32_t i = 0; i < m_Base->m_iThreadNum; i++)
        {
            ((H265_DXVA_SegmentDecoder *)m_Base->m_pSegmentDecoder[i])->SetVideoAccelerator(m_va);
        }

        dxva_sd->PackAllHeaders(pFrame);
    }

    void EndDecodingFrame()
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "DXVASupport::EndDecodingFrame");
        if (!m_va)
            return;

        UMC::Status sts = m_va->EndFrame();
        if (sts != UMC::UMC_OK)
            throw h265_exception(sts);
    }

    void SetVideoHardwareAccelerator(UMC::VideoAccelerator * va)
    {
        if (va)
            m_va = (UMC::VideoAccelerator*)va;
    }

protected:
    UMC::VideoAccelerator *m_va;
    BaseClass * m_Base;
};

class TaskBrokerSingleThreadDXVA : public TaskBroker_H265
{
public:
    TaskBrokerSingleThreadDXVA(TaskSupplier_H265 * pTaskSupplier);

    virtual bool PrepareFrame(H265DecoderFrame * pFrame);

    // Get next working task
    virtual bool GetNextTaskInternal(H265Task *pTask);
    virtual void Start();

    virtual void Reset();

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

    typedef std::vector<ReportItem> Report;
    Report m_reports;
    unsigned long long m_lastCounter;
    unsigned long long m_counterFrequency;
};

} // namespace UMC_HEVC_DECODER

#endif /* __UMC_H265_SEGMENT_DECODER_DXVA_H */
#endif // MFX_ENABLE_H265_VIDEO_DECODE
