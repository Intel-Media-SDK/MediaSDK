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
#if defined (MFX_ENABLE_MPEG2_VIDEO_DECODE)

#include <algorithm>

#include "umc_va_base.h"
#include "umc_mpeg2_decoder_va.h"
#include "umc_mpeg2_va_packer.h"

namespace UMC_MPEG2_DECODER
{
    MPEG2DecoderVA::MPEG2DecoderVA()
        : m_va(nullptr)
    {}

    UMC::Status MPEG2DecoderVA::SetParams(UMC::BaseCodecParams* info)
    {
        auto dp = dynamic_cast<MPEG2DecoderParams*>(info);
        MFX_CHECK(dp, UMC::UMC_ERR_INVALID_PARAMS);
        MFX_CHECK(dp->pVideoAccelerator, UMC::UMC_ERR_NULL_PTR);

        m_va = dp->pVideoAccelerator;
        m_packer.reset(Packer::CreatePacker(m_va));

        SetDPBSize(m_params.async_depth + NUM_REF_FRAMES);

        return UMC::UMC_OK;
    }

    // Pass picture to driver
    UMC::Status MPEG2DecoderVA::Submit(MPEG2DecoderFrame& frame, uint8_t fieldIndex)
    {
        UMC::Status sts = m_va->BeginFrame(frame.GetMemID());
        MFX_CHECK(UMC::UMC_OK == sts, sts);

        m_packer->PackAU(frame, fieldIndex);

        sts = m_va->EndFrame();

        return sts;
    }

    // Is decoding still in process
    inline bool InProgress(MPEG2DecoderFrame const& frame)
    {
        return frame.DecodingStarted() && !frame.DecodingCompleted();
    }

    // Allocate frame internals
    void MPEG2DecoderVA::AllocateFrameData(UMC::VideoDataInfo const& info, UMC::FrameMemID id, MPEG2DecoderFrame& frame)
    {
        UMC::FrameData fd;
        fd.Init(&info, id, m_allocator);

        frame.Allocate(&fd);

        auto fd2 = frame.GetFrameData();
        fd2->m_locked = true;
    }

    // Check for frame completeness and get decoding errors
    bool MPEG2DecoderVA::QueryFrames(MPEG2DecoderFrame& frame)
    {
        DPBType decode_queue;

        {
            std::unique_lock<std::mutex> l(m_guard);

            // form frame queue in decoded order
            std::for_each(m_dpb.begin(), m_dpb.end(),
                [&decode_queue, &frame](MPEG2DecoderFrame * f)
                {
                    if (InProgress(*f) && f->decOrder <= frame.decOrder) // skip frames beyong the current frame (in dec order)
                        decode_queue.push_back(f);
                }
            );
        }

        decode_queue.sort([](MPEG2DecoderFrame const* f1, MPEG2DecoderFrame const* f2) { return f1->decOrder < f2->decOrder; });

        // iterate through frames submitted to the driver in decoded order
        for (MPEG2DecoderFrame* frm : decode_queue)
        {
            uint16_t surfCorruption = 0;
            auto sts = m_packer->SyncTask(frm, &surfCorruption);

            frm->CompleteDecoding(); // complete frame even if there are errors

            if (sts < UMC::UMC_OK)
            {
                if (sts != UMC::UMC_ERR_GPU_HANG)
                    sts = UMC::UMC_ERR_DEVICE_FAILED;

                frm->SetError(sts);
            }
            else
            {
                switch (surfCorruption)
                {
                    case MFX_CORRUPTION_MAJOR:
                        frm->AddError(UMC::ERROR_FRAME_MAJOR);
                        break;

                    case MFX_CORRUPTION_MINOR:
                        frm->AddError(UMC::ERROR_FRAME_MINOR);
                        break;
                }
            }
        }

        return true;
    }
}

#endif // MFX_ENABLE_MPEG2_VIDEO_DECODE
