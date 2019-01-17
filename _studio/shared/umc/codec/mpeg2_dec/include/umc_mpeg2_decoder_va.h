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

#pragma once

#include "umc_defs.h"

#if defined (MFX_ENABLE_MPEG2_VIDEO_DECODE)

#include "umc_mpeg2_decoder.h"

namespace UMC_MPEG2_DECODER
{
    class Packer;
    class MPEG2DecoderVA
        : public MPEG2Decoder
    {
        UMC::VideoAccelerator*     m_va;
        std::unique_ptr<Packer>    m_packer;

    public:

        MPEG2DecoderVA();

        UMC::Status SetParams(UMC::BaseCodecParams*) override;
        // Check for frame completeness and get decoding errors
        bool QueryFrames(MPEG2DecoderFrame&) override;

    private:

        void AllocateFrameData(UMC::VideoDataInfo const&, UMC::FrameMemID, MPEG2DecoderFrame&) override;
        // Pass picture to driver
        UMC::Status Submit(MPEG2DecoderFrame&, uint8_t) override;
    };
}

#endif // MFX_ENABLE_MPEG2_VIDEO_DECODE