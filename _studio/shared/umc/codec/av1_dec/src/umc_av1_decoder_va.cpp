// Copyright (c) 2017-2020 Intel Corporation
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
#ifdef MFX_ENABLE_AV1_VIDEO_DECODE

#include "umc_structures.h"
#include "umc_va_base.h"

#include "umc_av1_decoder_va.h"
#include "umc_av1_utils.h"
#include "umc_av1_frame.h"
#include "umc_av1_bitstream.h"
#include "umc_av1_va_packer.h"

#include "umc_frame_data.h"

#include <algorithm>

namespace UMC_AV1_DECODER
{
    AV1DecoderVA::AV1DecoderVA()
        : va(nullptr)
    {}

    UMC::Status AV1DecoderVA::SetParams(UMC::BaseCodecParams* info)
    {
        if (!info)
            return UMC::UMC_ERR_NULL_PTR;

        AV1DecoderParams* dp =
            DynamicCast<AV1DecoderParams, UMC::BaseCodecParams>(info);
        if (!dp)
            return UMC::UMC_ERR_INVALID_PARAMS;

        if (!dp->pVideoAccelerator)
            return UMC::UMC_ERR_NULL_PTR;

        va = dp->pVideoAccelerator;
        packer.reset(Packer::CreatePacker(va));

        uint32_t const dpb_size =
            params.async_depth + TOTAL_REFS + 2;
        SetDPBSize(dpb_size);
        SetRefSize(TOTAL_REFS);
        return UMC::UMC_OK;
    }

    UMC::Status AV1DecoderVA::SubmitTiles(AV1DecoderFrame& frame, bool firstSubmission)
    {
        VM_ASSERT(va);
        UMC::Status sts = UMC::UMC_OK;

        if (firstSubmission)
        {
            // it's first submission for current frame - need to call BeginFrame
            sts = va->BeginFrame(frame.GetMemID(SURFACE_RECON));
            if (sts != UMC::UMC_OK)
                return sts;

            VM_ASSERT(packer);
            packer->BeginFrame();

            frame.StartDecoding();
        }

        auto &tileSets = frame.GetTileSets();
        packer->PackAU(tileSets, frame, firstSubmission);

        const bool lastSubmission = (GetNumMissingTiles(frame) == 0);
        if (lastSubmission)
        {
            packer->EndFrame();
            // VA Execute after full frame detected to avoid duplicate submission
            // of the decode buffer when app use small decbufsize.
            sts = va->Execute();
        }
        if (sts != UMC::UMC_OK)
            sts = UMC::UMC_ERR_DEVICE_FAILED;

        if (lastSubmission) // it's last submission for current frame - need to call EndFrame
            sts = va->EndFrame();

        return sts;
    }

    void AV1DecoderVA::AllocateFrameData(UMC::VideoDataInfo const& info, UMC::FrameMemID id, AV1DecoderFrame& frame)
    {
        VM_ASSERT(id != UMC::FRAME_MID_INVALID);

        UMC::FrameData fd;
        fd.Init(&info, id, allocator);

        frame.AllocateAndLock(&fd);
    }

    inline bool InProgress(AV1DecoderFrame const& frame)
    {
        return frame.DecodingStarted() && !frame.DecodingCompleted();
    }

    bool AV1DecoderVA::QueryFrames()
    {
        std::unique_lock<std::mutex> auto_guard(guard);

        // form frame queue in decoded order
        DPBType decode_queue;
        for (DPBType::iterator frm = dpb.begin(); frm != dpb.end(); frm++)
            if (InProgress(**frm))
                decode_queue.push_back(*frm);

        std::sort(decode_queue.begin(), decode_queue.end(),
            [](AV1DecoderFrame const* f1, AV1DecoderFrame const* f2) {return f1->UID < f2->UID; });

        // below logic around "wasCompleted" was adopted from AVC/HEVC decoders
        bool wasCompleted = false;

        // iterate through frames submitted to the driver in decoded order
        for (DPBType::iterator frm = decode_queue.begin(); frm != decode_queue.end(); frm++)
        {
            AV1DecoderFrame& frame = **frm;
            uint32_t index = 0;

            VAStatus surfErr = VA_STATUS_SUCCESS;
            index = frame.GetMemID();
            auto_guard.unlock();
            UMC::Status sts =  packer->SyncTask(index, &surfErr);
            auto_guard.lock();

            frame.CompleteDecoding();
            wasCompleted = true;

            if (sts < UMC::UMC_OK)
            {
                // TODO: [Global] Add GPU hang reporting
            }
            else if (sts == UMC::UMC_OK)
            {
                switch (surfErr)
                {
                    case MFX_CORRUPTION_MAJOR:
                        frame.AddError(UMC::ERROR_FRAME_MAJOR);
                        break;

                    case MFX_CORRUPTION_MINOR:
                        frame.AddError(UMC::ERROR_FRAME_MINOR);
                        break;
                }
            }
        }

        return wasCompleted;
    }
}

#endif //MFX_ENABLE_AV1_VIDEO_DECODE
