// Copyright (c) 2018-2020 Intel Corporation
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

#include <mutex>
#include <memory>
#include <vector>
#include <deque>

#include "umc_mpeg2_defs.h"
#include "umc_mpeg2_frame.h"
#include "umc_video_decoder.h"
#include "umc_mpeg2_splitter.h"

namespace UMC
{ class FrameAllocator; }

namespace UMC_MPEG2_DECODER
{
    class Payload_Storage;
    class MPEG2HeadersBitstream;

/****************************************************************************************************/
// Skipping_MPEG2 class routine
/****************************************************************************************************/
    class Skipping_MPEG2
    {
    public:
        virtual ~Skipping_MPEG2()
        {}

        // Check if frame should be skipped to decrease decoding delays
        bool IsShouldSkipFrame(const MPEG2DecoderFrame&);
        void ChangeVideoDecodingSpeed(int32_t& num);
        void Reset();

         // Get current skip mode state
        uint32_t GetNumSkipFrames() const
        { return m_NumberOfSkippedFrames; }

    private:

        enum SkipLevel
        {
            SKIP_NONE = 0,
            SKIP_B,
            SKIP_PB,
            SKIP_ALL
        };

        int32_t m_SkipLevel = SKIP_NONE;
        uint32_t m_NumberOfSkippedFrames = 0;
    };

    class MPEG2DecoderParams
        : public UMC::VideoDecoderParams
    {
        DYNAMIC_CAST_DECL(MPEG2DecoderParams, UMC::VideoDecoderParams)

    public:

        UMC::FrameAllocator* allocator   = nullptr;
        uint32_t             async_depth = 0;
    };

    class MPEG2Decoder
        : public UMC::VideoDecoder, public Skipping_MPEG2
    {

    public:

        MPEG2Decoder();
        ~MPEG2Decoder() override;

    public:

        static UMC::Status DecodeHeader(mfxBitstream&, mfxVideoParam&);

        /* UMC::BaseCodec interface */
        UMC::Status Init(UMC::BaseCodecParams*) override;

        // Close all codec resources
        UMC::Status Close(void) override;

        UMC::Status GetFrame(UMC::MediaData*, UMC::MediaData*) override
        { return UMC::UMC_ERR_NOT_IMPLEMENTED; }

        UMC::Status Reset() override;

        UMC::Status GetInfo(UMC::BaseCodecParams*) override;

    public:

        /* UMC::VideoDecoder interface */
        UMC::Status ResetSkipCount() override
        { return UMC::UMC_ERR_NOT_IMPLEMENTED; }
        UMC::Status SkipVideoFrame(int32_t) override
        { return UMC::UMC_ERR_NOT_IMPLEMENTED; }
        uint32_t GetNumOfSkippedFrames() override
        { return 0; }

    public:

        // Find NAL units in new bitstream buffer and process them
        virtual UMC::Status AddOneFrame(UMC::MediaData * source);

        // Add a new bitstream data buffer to decoding
        virtual UMC::Status AddSource(UMC::MediaData * source);

        // Decode slice header start, set slice links to seq and picture
        virtual MPEG2Slice * DecodeSliceHeader(const RawUnit & unit);

        // Add a new slice to picture
        virtual UMC::Status AddSlice(MPEG2Slice*);

        // Decode a bitstream header
        virtual UMC::Status DecodeHeaders(const RawUnit &);

        // Decode sequence header (and extension)
        virtual UMC::Status DecodeSeqHeader(const RawUnit &);

        // Decode picture header (and extension)
        virtual UMC::Status DecodePicHeader(const RawUnit &);

        // Decode group of pictures header
        virtual UMC::Status DecodeGroupHeader(const RawUnit &);

        // Decode display and matrix extensions
        virtual UMC::Status DecodeExtension(const RawUnit &);

        virtual MPEG2DecoderFrame* FindFrameByMemID(UMC::FrameMemID);

        // Find a next frame ready to be output from decoder
        virtual MPEG2DecoderFrame* GetFrameToDisplay();

        // Check for frame completeness and get decoding errors
        virtual bool QueryFrames(MPEG2DecoderFrame&) = 0;

        // Initialize mfxVideoParam structure based on decoded bitstream header values
        virtual UMC::Status FillVideoParam(mfxVideoParam *par, bool full);

        // Return raw sequence and sequence extension headers
        virtual RawHeader_MPEG2 * GetSeqAndSeqExtHdr()
        { return &m_rawHeaders; }

        // Return a number of cached frames
        virtual uint8_t GetNumCachedFrames() const;

        // Set frame display time
        virtual void PostProcessDisplayFrame(MPEG2DecoderFrame*);

        // Set MFX video params
        virtual void SetVideoParams(const mfxVideoParam&);

        Payload_Storage * GetPayloadStorage() const { return m_messages.get();}

        MPEG2DecoderParams* GetMpeg2DecoderParams() {return &m_params;}

    protected:

        virtual void SetDPBSize(uint32_t);
        virtual MPEG2DecoderFrame* GetFreeFrame();
        virtual MPEG2DecoderFrame* GetFrameBuffer(MPEG2Slice*);

        // Action on new slice
        virtual UMC::Status OnNewSlice(const RawUnit & data);

        // Action on new picture
        virtual bool OnNewPicture(); // returns true on full frame
        virtual void EliminateSliceErrors(MPEG2DecoderFrame& frame, uint8_t fieldIndex);
        virtual UMC::Status CompletePicture(MPEG2DecoderFrame& frame, uint8_t fieldIndex);
        bool IsFieldOfCurrentFrame() const;

        // DPB update
        virtual void UpdateDPB(MPEG2DecoderFrame&, uint8_t);

        // Allocate frame internals
        virtual void AllocateFrameData(UMC::VideoDataInfo const&, UMC::FrameMemID, MPEG2DecoderFrame&) = 0;
        virtual void CompleteDecodedFrames();
        // Pass picture to driver
        virtual UMC::Status Submit(MPEG2DecoderFrame&, uint8_t) = 0;

    private:

        // Find a reusable frame initialize it with slice parameters
        MPEG2DecoderFrame* StartFrame(MPEG2Slice * slice);

    protected:

        std::mutex                      m_guard;
        UMC::FrameAllocator*            m_allocator;
        uint32_t                        m_counter;
        MPEG2DecoderParams              m_params;
        mfxVideoParam                   m_firstMfxVideoParams;

        DPBType                         m_dpb;     // storage of decoded frames

        MPEG2DecoderFrame*              m_currFrame;

        struct Headers
        {
            std::shared_ptr<MPEG2SequenceHeader>           seqHdr;
            std::shared_ptr<MPEG2SequenceExtension>        seqExtHdr;
            std::shared_ptr<MPEG2SequenceDisplayExtension> displayExtHdr;
            std::shared_ptr<MPEG2QuantMatrix>              qmatrix;
            std::shared_ptr<MPEG2GroupOfPictures>          group;
            std::shared_ptr<MPEG2PictureHeader>            picHdr;
            std::shared_ptr<MPEG2PictureCodingExtension>   picExtHdr;
        };

        Headers                         m_currHeaders; // current active stream headers
        RawHeader_MPEG2                 m_rawHeaders;  // raw binary headers

        uint32_t                        m_dpbSize;

        Splitter                        m_splitter;

        double                          m_localDeltaFrameTime;
        bool                            m_useExternalFramerate;
        double                          m_localFrameTime;

        std::unique_ptr<Payload_Storage> m_messages;

        std::unique_ptr<MPEG2Slice>     m_lastSlice; // slice which could't be processed
    };

    class Payload_Storage
    {
    public:

        struct Message
        {
            size_t             msg_size  = 0;
            size_t             offset    = 0;
            uint8_t            * data    = nullptr;
            double             timestamp = 0.0;
            int32_t             isUsed   = 0;
            int32_t             auID     = 0;
            int32_t             inputID  = 0;
            MPEG2DecoderFrame * frame    = nullptr;
        };

        Payload_Storage();

        ~Payload_Storage();

        void Init();

        void Close();

        void Reset();

        void SetTimestamp(MPEG2DecoderFrame * frame);

        Message* AddMessage(const RawUnit & data, int32_t auIndex);

        const Message * GetPayloadMessage();

        void SetFrame(MPEG2DecoderFrame * frame, int32_t auIndex);
        void SetAUID(int32_t auIndex);

    private:

        enum
        {
            MAX_BUFFERED_SIZE = 16 * 1024, // 16 kb
            START_ELEMENTS = 10,
            MAX_ELEMENTS = 128
        };

        std::vector<uint8_t>  m_data;
        std::vector<Message>  m_payloads;

        size_t m_offset;
        int32_t m_lastUsed;
    };
}

#endif // MFX_ENABLE_MPEG2_VIDEO_DECODE
