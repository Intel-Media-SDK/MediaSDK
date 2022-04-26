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

#ifndef __UMC_AV1_DECODER_H
#define __UMC_AV1_DECODER_H

#include "umc_video_decoder.h"
#include "umc_frame_allocator.h"
#include "umc_av1_dec_defs.h"

#include <mutex>
#include <memory>
#include <vector>
#include <deque>

namespace UMC
{ class FrameAllocator; }

namespace UMC_AV1_DECODER
{
    struct SequenceHeader;
    struct FrameHeader;
    class AV1DecoderFrame;

    class AV1DecoderParams
        : public UMC::VideoDecoderParams
    {
        DYNAMIC_CAST_DECL(AV1DecoderParams, UMC::VideoDecoderParams)

    public:

        AV1DecoderParams()
            : allocator(nullptr)
            , async_depth(0)
            , film_grain(0)
            , io_pattern(0)
        {}

    public:

        UMC::FrameAllocator* allocator;
        uint32_t             async_depth;
        uint32_t             film_grain;
        uint32_t             io_pattern;
    };

    class ReportItem // adopted from HEVC/AVC decoders
    {
    public:
        uint32_t  m_index;
        uint8_t   m_status;

        ReportItem(uint32_t index, uint8_t status)
            : m_index(index)
            , m_status(status)
        {
        }

        bool operator == (const ReportItem & item) const
        {
            return (item.m_index == m_index);
        }

        bool operator != (const ReportItem & item) const
        {
            return (item.m_index != m_index);
        }
    };

    class AV1Decoder
        : public UMC::VideoDecoder
    {

    public:

        AV1Decoder();
        ~AV1Decoder();

    public:

        static UMC::Status DecodeHeader(UMC::MediaData*, UMC_AV1_DECODER::AV1DecoderParams&);

        /* UMC::BaseCodec interface */
        UMC::Status Init(UMC::BaseCodecParams*) override;
        UMC::Status GetFrame(UMC::MediaData* in, UMC::MediaData* out) override;

        virtual UMC::Status Reset() override
        { return UMC::UMC_ERR_NOT_IMPLEMENTED; }
        UMC::Status GetInfo(UMC::BaseCodecParams*) override;

    public:

        /* UMC::VideoDecoder interface */
        virtual UMC::Status ResetSkipCount() override
        { return UMC::UMC_ERR_NOT_IMPLEMENTED; }
        virtual UMC::Status SkipVideoFrame(int32_t) override
        { return UMC::UMC_ERR_NOT_IMPLEMENTED; }
        virtual uint32_t GetNumOfSkippedFrames() override
        { return 0; }

    public:

        AV1DecoderFrame* FindFrameByMemID(UMC::FrameMemID);
        AV1DecoderFrame* GetFrameToDisplay();
        AV1DecoderFrame* FindFrameByUID(int64_t uid);
        AV1DecoderFrame* FindFrameInProgress();
        AV1DecoderFrame* GetCurrFrame()
        { return Curr; }
        void SetInFrameRate(mfxF64 rate)
        { in_framerate = rate; }

        virtual bool QueryFrames() = 0;

        AV1DecoderParams* GetAv1DecoderParams() {return &params;}

    protected:

        static UMC::Status FillVideoParam(SequenceHeader const&, UMC_AV1_DECODER::AV1DecoderParams&);

        virtual void SetDPBSize(uint32_t);
        virtual void SetRefSize(uint32_t);
        virtual AV1DecoderFrame* GetFreeFrame();
        virtual AV1DecoderFrame* GetFrameBuffer(FrameHeader const&);
        virtual void AddFrameData(AV1DecoderFrame&);

        virtual void AllocateFrameData(UMC::VideoDataInfo const&, UMC::FrameMemID, AV1DecoderFrame&) = 0;
        virtual void CompleteDecodedFrames(FrameHeader const&, AV1DecoderFrame*, AV1DecoderFrame*);
        virtual UMC::Status SubmitTiles(AV1DecoderFrame&, bool) = 0;

    private:

        template <typename F>
        AV1DecoderFrame* FindFrame(F pred);
        AV1DecoderFrame* StartFrame(FrameHeader const&, DPBType &, AV1DecoderFrame*);

        void CalcFrameTime(AV1DecoderFrame*);

    protected:

        std::mutex                      guard;

        UMC::FrameAllocator*            allocator;

        std::unique_ptr<SequenceHeader> sequence_header;
        DPBType                         dpb;     // store of decoded frames

        uint32_t                        counter;
        AV1DecoderParams                params;
        std::vector<AV1DecoderFrame*>   outputed_frames; // tore frames need to be output
        AV1DecoderFrame*                Curr; // store current frame for Poutput
        AV1DecoderFrame*                Curr_temp; // store current frame insist double updateDPB
        uint32_t                        Repeat_show; // show if current frame is repeated frame
        uint32_t                        PreFrame_id;//id of previous frame
        uint32_t                        OldPreFrame_id;//old id of previous frame. When decode multi tile-group clip, need this for parsing twice
        DPBType                         refs_temp; // previous updated frameDPB
        mfxU16                          frame_order;
        mfxF64                          in_framerate;
    };
}

#endif // __UMC_AV1_DECODER_H
#endif // MFX_ENABLE_AV1_VIDEO_DECODE

