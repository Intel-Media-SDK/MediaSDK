// Copyright (c) 2012-2020 Intel Corporation
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

#include "umc_av1_decoder.h"

#ifndef __UMC_AV1_DECODER_VA_H
#define __UMC_AV1_DECODER_VA_H

namespace UMC_AV1_DECODER
{
    class Packer;
    class AV1DecoderVA
        : public AV1Decoder
    {
        UMC::VideoAccelerator*     va;
        std::unique_ptr<Packer>    packer;

    public:

        AV1DecoderVA();

        UMC::Status SetParams(UMC::BaseCodecParams*) override;
        bool QueryFrames() override;

    private:

        void AllocateFrameData(UMC::VideoDataInfo const&, UMC::FrameMemID, AV1DecoderFrame&) override;
        UMC::Status SubmitTiles(AV1DecoderFrame&, bool) override;

    private:
        std::vector<ReportItem> reports;
    };
}

#endif // __UMC_AV1_DECODER_VA_H
#endif // MFX_ENABLE_AV1_VIDEO_DECODE
