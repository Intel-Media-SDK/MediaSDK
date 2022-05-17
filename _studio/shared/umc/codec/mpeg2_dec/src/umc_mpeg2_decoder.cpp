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

#include "umc_defs.h"
#if defined (MFX_ENABLE_MPEG2_VIDEO_DECODE)

#include "umc_structures.h"
#include "umc_mpeg2_decoder.h"
#include "umc_mpeg2_utils.h"
#include "umc_mpeg2_slice.h"

namespace UMC_MPEG2_DECODER
{
    // Find a previous (in decoded order) reference frame
    MPEG2DecoderFrame* FindLastRefFrame(const MPEG2DecoderFrame & curr, const DPBType & dpb)
    {
        DPBType refFrames;
        std::copy_if(dpb.begin(), dpb.end(), back_inserter(refFrames),
             [&curr](MPEG2DecoderFrame const * f)
             {
                return !f->Empty() && (f != &curr) && f->IsRef();
             }
        );

        auto i = std::max_element(std::begin(refFrames), std::end(refFrames),
            [](MPEG2DecoderFrame const* f1, MPEG2DecoderFrame const* f2)
            {
                return  f1->decOrder < f2->decOrder;
            }
        );

        return (i == std::end(refFrames)) ? nullptr : *i;
    }

    // Find an unused frame in DPB
    MPEG2DecoderFrame* FindFreeFrame(const DPBType & dpb)
    {
        auto i = std::find_if(std::begin(dpb), std::end(dpb),
                [](MPEG2DecoderFrame const* frame)
                { return frame->Empty(); }
            );
        return (i != std::end(dpb)) ? *i : nullptr;
    }

    MPEG2Decoder::MPEG2Decoder()
        : m_allocator(nullptr)
        , m_counter(0)
        , m_firstMfxVideoParams()
        , m_currFrame(nullptr)
        , m_dpbSize(0)
        , m_localDeltaFrameTime(0)
        , m_useExternalFramerate(false)
        , m_localFrameTime(0)
    {
    }

    MPEG2Decoder::~MPEG2Decoder()
    {
        std::for_each(std::begin(m_dpb), std::end(m_dpb),
            std::default_delete<MPEG2DecoderFrame>()
        );
    }

    UMC::Status MPEG2Decoder::Init(UMC::BaseCodecParams* vp)
    {
        auto dp = dynamic_cast<MPEG2DecoderParams*> (vp);
        MFX_CHECK(dp, UMC::UMC_ERR_INVALID_PARAMS);
        MFX_CHECK(dp->allocator, UMC::UMC_ERR_INVALID_PARAMS);

        m_allocator = dp->allocator;

        m_useExternalFramerate = 0 < dp->info.framerate;

        m_localDeltaFrameTime = m_useExternalFramerate ? 1 / dp->info.framerate : 1.0/30;

        m_params = *dp;

        m_messages.reset(new Payload_Storage);
        m_messages->Init();

        return SetParams(vp);
    }

    UMC::Status MPEG2Decoder::Close()
    {
        return Reset();
    }

    UMC::Status MPEG2Decoder::Reset()
    {
        m_counter = 0;

        std::for_each(std::begin(m_dpb), std::end(m_dpb),
            std::default_delete<MPEG2DecoderFrame>()
        );
        m_dpb.clear();

        m_localFrameTime = 0;

        m_currFrame = nullptr;
        m_currHeaders.seqHdr.reset();
        m_currHeaders.seqExtHdr.reset();
        m_currHeaders.displayExtHdr.reset();
        m_currHeaders.qmatrix.reset();
        m_currHeaders.group.reset();
        m_currHeaders.picHdr.reset();
        m_currHeaders.picExtHdr.reset();

        m_rawHeaders.Reset();

        m_splitter.Reset();

        if (m_messages)
            m_messages->Reset();

        Skipping_MPEG2::Reset();

        m_lastSlice.reset();

        return UMC::UMC_OK;
    }

    // Set MFX video params
    void MPEG2Decoder::SetVideoParams(const mfxVideoParam & par)
    {
        m_firstMfxVideoParams = par;
    }

    UMC::Status MPEG2Decoder::GetInfo(UMC::BaseCodecParams* info)
    {
        auto vp = dynamic_cast<UMC::VideoDecoderParams*> (info);
        MFX_CHECK(vp, UMC::UMC_ERR_INVALID_PARAMS);

        *vp = m_params;
        return UMC::UMC_OK;
    }

    // Fill mfx video params from current seq/seqExt/display headers
    UMC::Status MPEG2Decoder::FillVideoParam(mfxVideoParam *par, bool /*full*/)
    {
        const auto seq = m_currHeaders.seqHdr.get();
        MFX_CHECK(seq, UMC::UMC_ERR_FAILED); // mandatory header

        const auto seqExt = m_currHeaders.seqExtHdr.get();
        MFX_CHECK(seqExt, UMC::UMC_ERR_FAILED); // mandatory header

        const auto dispExt = m_currHeaders.displayExtHdr.get(); // optional

        if (UMC_MPEG2_DECODER::MFX_Utility::FillVideoParam(*seq, seqExt, dispExt, *par) != UMC::UMC_OK)
            return UMC::UMC_ERR_FAILED;

        return UMC::UMC_OK;
    }

    // MediaSDK API DecodeHeader
    UMC::Status MPEG2Decoder::DecodeHeader(mfxBitstream & bs, mfxVideoParam & par)
    {
        MFXMediaDataAdapter in(&bs);

        // The purpose of this is to remove FLAG_VIDEO_DATA_NOT_FULL_FRAME flag
        // so that we can handle partial unit (i.e. corrupted) in a proper way
        in.SetFlags(0);

        std::unique_ptr<MPEG2SequenceHeader> seq;
        std::unique_ptr<MPEG2SequenceExtension> seqExt;
        std::unique_ptr<MPEG2SequenceDisplayExtension> dispExt;

        RawHeader_MPEG2 rawHdr;
        MPEG2HeadersBitstream bitStream;

        RawHeaderIterator it;
        for (it.LoadData(&in); it != RawHeaderIterator(); ++it)
        {
            RawUnit sequence;
            RawUnit sequenceExtension;
            RawUnit sequenceDisplayExtension;

            if (SEQUENCE_HEADER == it->type)
            {
                // seq header already found
                if (seq)
                    break;

                // move point to the sequence header
                size_t headerSize = (it->end - it->begin);
                bs.DataOffset = (uint8_t*)in.GetDataPointer() - (uint8_t*)in.GetBufferPointer() - headerSize;
                bs.DataLength = in.GetDataSize() + headerSize;

                seq.reset(new MPEG2SequenceHeader);
                seqExt.reset();
                dispExt.reset();

                try
                {
                    uint8_t * begin = it->begin + prefix_size + 1;
                    bitStream.Reset(begin, it->end - begin);
                    bitStream.GetSequenceHeader(*seq.get());
                }
                // On corrupted data reset headers
                catch(const mpeg2_exception&)
                {
                    seq.reset();
                    seqExt.reset();
                    dispExt.reset();
                    continue;
                }

                rawHdr.Resize(it->end - it->begin);
                std::copy(it->begin, it->end, rawHdr.GetPointer());
            }
            else if (EXTENSION == it->type)
            {
                uint8_t ext_type = it->begin[prefix_size + 1] >> 4;
                if (SEQUENCE_EXTENSION == ext_type)
                {
                    // seq header already found
                    if (seqExt)
                        break;

                    seqExt.reset(new MPEG2SequenceExtension);
                    try
                    {
                        uint8_t * begin = it->begin + prefix_size + 1;
                        bitStream.Reset(begin, it->end - begin);
                        bitStream.Seek(4); // skip extension type
                        bitStream.GetSequenceExtension(*seqExt.get());
                    }
                    // On corrupted data reset headers
                    catch(const mpeg2_exception&)
                    {
                        seq.reset();
                        seqExt.reset();
                        dispExt.reset();
                        continue;
                    }

                    size_t oldSize = rawHdr.GetSize();
                    rawHdr.Resize(oldSize + (it->end - it->begin));
                    std::copy(it->begin, it->end, rawHdr.GetPointer() + oldSize);
                }
                else if (SEQUENCE_DISPLAY_EXTENSION == ext_type)
                {
                    dispExt.reset(new MPEG2SequenceDisplayExtension);
                    try
                    {
                        uint8_t * begin = it->begin + prefix_size + 1;
                        bitStream.Reset(begin, it->end - begin);
                        bitStream.Seek(4); // skip extension type
                        bitStream.GetSequenceDisplayExtension(*dispExt.get());
                    }
                    // On errors reset headers
                    catch(const mpeg2_exception&)
                    {
                        seq.reset();
                        seqExt.reset();
                        dispExt.reset();
                        continue;
                    }

                    size_t oldSize = rawHdr.GetSize();
                    rawHdr.Resize(oldSize + (it->end - it->begin));
                    std::copy(it->begin, it->end, rawHdr.GetPointer() + oldSize);
                }
            }
            else if (seq)
            {
                break;
            }
        }

        // Got data, fill mfx video params
        if (seq)
        {
            UMC_MPEG2_DECODER::MFX_Utility::FillVideoParam(*seq.get(), seqExt.get(), dispExt.get(), par);

            auto mfxSeq = (mfxExtCodingOptionSPSPPS *)GetExtendedBuffer(par.ExtParam, par.NumExtParam, MFX_EXTBUFF_CODING_OPTION_SPSPPS);
            if (mfxSeq)
            {
                if (rawHdr.GetSize())
                {
                    if (mfxSeq->SPSBufSize < rawHdr.GetSize())
                        return MFX_ERR_NOT_ENOUGH_BUFFER;

                    mfxSeq->SPSBufSize = rawHdr.GetSize();
                    std::copy(rawHdr.GetPointer(), rawHdr.GetPointer() + mfxSeq->SPSBufSize, mfxSeq->SPSBuffer);
                }
                else
                {
                    mfxSeq->SPSBufSize = 0;
                }
            }
            return UMC::UMC_OK;
        }

        // If didn't find headers consume all bs data
        bs.DataOffset = (uint8_t*)in.GetDataPointer() - (uint8_t*)in.GetBufferPointer();
        bs.DataLength = in.GetDataSize();

        // Request more data
        return UMC::UMC_ERR_NOT_ENOUGH_DATA;
    }

    // Add a new bitstream data buffer to decoding
    UMC::Status MPEG2Decoder::AddSource(UMC::MediaData * source)
    {
        if (GetFrameToDisplay())
            return UMC::UMC_OK;

        return AddOneFrame(source);
    }

    // Find units in new bitstream buffer and process them
    UMC::Status MPEG2Decoder::AddOneFrame(UMC::MediaData * source)
    {
        if (m_lastSlice)
        {
            UMC::Status sts = AddSlice(m_lastSlice.release());
            if (sts == UMC::UMC_ERR_NOT_ENOUGH_BUFFER || sts == UMC::UMC_OK)
                return sts;
        }

        // The main processing loop
        for (RawUnit unit = m_splitter.GetUnits(source); unit.begin && unit.end; unit = m_splitter.GetUnits(source))
        {
            switch (unit.type)
            {
            case SEQUENCE_HEADER:
            {
                auto sts = DecodeHeaders(unit);
                if (sts != UMC::UMC_OK)
                {
                    if (sts == UMC::UMC_NTF_NEW_RESOLUTION && source)
                    {
                        // Move to the position of sequence header start
                        source->MoveDataPointer(- (unit.end - unit.begin));
                    }

                    return sts;
                }
            }
            break;
            case PICTURE_HEADER:
            {
                auto sts = DecodeHeaders(unit);          // 1. Parse new picture header
                if ((UMC::UMC_OK == sts) && m_currFrame) // 2. Complete current picture
                {
                    bool isFullFrame = OnNewPicture();
                    if (isFullFrame)
                        return UMC::UMC_OK; // exit from the loop on full frame
                }
            }
            break;
            case EXTENSION:
            case GROUP:
                DecodeHeaders(unit);
                break;
            case USER_DATA:
                m_messages->AddMessage(unit, -1);
                break;
            default:
                break;
            };

            bool isSlice = (unit.type >= 0x1 && unit.type <= 0xAF); // slice header start codes
            if (isSlice)
            {
                auto sts = OnNewSlice(unit);
                if (sts == UMC::UMC_ERR_NOT_ENOUGH_BUFFER) // no free frames -> MFX_WRN_DEV_BUSY
                    return sts;
            }
        };

        auto sts = UMC::UMC_ERR_NOT_ENOUGH_DATA;

        // The whole bitstream has been consumed. We have to complete current _frame_ if 'complete' flag was set
        bool isCompleteFrame = source && !(source->GetFlags() & UMC::MediaData::FLAG_VIDEO_DATA_NOT_FULL_FRAME);
        if (isCompleteFrame && m_currFrame && (m_currFrame->isProgressiveSequence || m_currFrame->GetAU(0)->IsFilled()))
        {
            CompletePicture(*m_currFrame, m_currFrame->currFieldIndex);
            m_currFrame->SetFullFrame(true);

            m_currFrame = nullptr;

            sts = UMC::UMC_OK; // exit on full frame
        }
        else if (!source) // EOS handling
        {
            AddSlice(nullptr);
        }

        return sts;
    }

    // Action on new slice
    UMC::Status MPEG2Decoder::OnNewSlice(const RawUnit & data)
    {
        UMC::Status sts = UMC::UMC_ERR_NOT_ENOUGH_DATA;
        if (auto slice = DecodeSliceHeader(data))
        {
            sts = AddSlice(slice);
        }
        return sts;
    }

    // Decode slice header start, set slice links to seq and picture
    MPEG2Slice *MPEG2Decoder::DecodeSliceHeader(const RawUnit & in)
    {
        // Check availability of headers
        if (!m_currHeaders.seqHdr || !m_currHeaders.seqExtHdr || !m_currHeaders.picHdr || !m_currHeaders.picExtHdr )
        {
            return nullptr;
        }

        std::unique_ptr<MPEG2Slice> slice (new MPEG2Slice); // unique_ptr is to prevent a possible memory leak

        const size_t size = in.end - in.begin - prefix_size;
        slice->source.Alloc(size);
        if (slice->source.GetBufferSize() < size)
            throw mpeg2_exception(UMC::UMC_ERR_ALLOC);

        // It can be optimized without memory copy
        std::copy(in.begin + prefix_size, in.end, (uint8_t*)slice->source.GetDataPointer());
        slice->source.SetDataSize(size);
        slice->source.SetTime(in.pts);

        slice->SetSeqHeader(m_currHeaders.seqHdr);
        slice->SetSeqExtHeader(m_currHeaders.seqExtHdr);
        slice->SetPicHeader(m_currHeaders.picHdr);
        slice->SetPicExtHeader(m_currHeaders.picExtHdr);
        slice->SetQMatrix(m_currHeaders.qmatrix);

        // Skip slice in case of in errors slice header
        if (!slice->Reset())
        {
            return nullptr;
        }

        return slice.release();
    }

    // Add a new slice to picture
    UMC::Status MPEG2Decoder::AddSlice(MPEG2Slice * slice)
    {
        // On EOS
        if (!slice)
        {
            // output all cached frames
            {
                std::unique_lock<std::mutex> l(m_guard);
                std::for_each(m_dpb.begin(), m_dpb.end(),
                    [](MPEG2DecoderFrame* frame)
                    {
                        if (!frame->IsReadyToBeOutputted() && frame->DecodingStarted())
                            frame->SetReadyToBeOutputted();
                    }
                );
            }

            if (m_currFrame)
            {
                m_currFrame->SetFullFrame(true);
                CompletePicture(*m_currFrame, m_currFrame->currFieldIndex);

                m_currFrame->SetReadyToBeOutputted(); // Reordering finished

                m_currFrame = nullptr;

                return UMC::UMC_OK; // On full frame
            }
            return UMC::UMC_ERR_NOT_ENOUGH_DATA;
        }

        // There is no current frame.
        // Try to allocate a new one.
        if (nullptr == m_currFrame)
        {
            // Allocate a new frame, initialize it with slice's parameters.
            if (nullptr == (m_currFrame = StartFrame(slice)))
            {
                m_lastSlice.reset(slice);
                return UMC::UMC_ERR_NOT_ENOUGH_BUFFER;
            }
        }

        const auto picExt = *m_currHeaders.picExtHdr;
        m_currFrame->fieldIndex = m_currFrame->GetNumberByParity(picExt.picture_structure == BOTTOM_FLD_PICTURE);
        MPEG2DecoderFrameInfo & info = *m_currFrame->GetAU(m_currFrame->fieldIndex);

        // Add the slice to the picture
        info.AddSlice(slice);

        if (info.GetSliceCount() == 1) // Set ref frames on the first slice of the picture
        {
            std::unique_lock<std::mutex> l(m_guard);
            info.UpdateReferences(m_dpb);
        }

        return UMC::UMC_ERR_NOT_ENOUGH_DATA;
    }

    // Decode sequence header
    UMC::Status MPEG2Decoder::DecodeSeqHeader(const RawUnit & data)
    {
        auto seqHdr    = std::make_shared<MPEG2SequenceHeader>();
        auto seqExtHdr = std::make_shared<MPEG2SequenceExtension>();
        MPEG2HeadersBitstream bitStream(data.begin + prefix_size + 1, data.end - data.begin - prefix_size - 1); // "+ prefix_size + 1" is to skip start code and type

        try
        {
            bitStream.GetSequenceHeader(*seqHdr.get());

            uint8_t* seqExtBegin = RawHeaderIterator::FindStartCode(data.begin + prefix_size + bitStream.BytesDecoded(), data.end);
            MFX_CHECK(seqExtBegin, UMC::UMC_ERR_INVALID_STREAM);

            bitStream.Reset(seqExtBegin + prefix_size, data.end - seqExtBegin);
            bitStream.Seek(8 + 4); // skip data and extension types
            bitStream.GetSequenceExtension(*seqExtHdr.get());

            // Save headers
            m_currHeaders.seqHdr = std::move(seqHdr);
            m_currHeaders.seqExtHdr = std::move(seqExtHdr);

            // Reset 'dependent' headers
            m_currHeaders.displayExtHdr.reset();
            m_currHeaders.qmatrix.reset();
            m_currHeaders.group.reset();
            m_currHeaders.picHdr.reset();
            m_currHeaders.picExtHdr.reset();

            m_rawHeaders.Resize(data.end - data.begin);
            std::copy(data.begin, data.end, m_rawHeaders.GetPointer());
        }

        catch(const mpeg2_exception & ex)
        {
            return ex.GetStatus();
        }
        catch(...)
        {
            return UMC::UMC_ERR_INVALID_STREAM;
        }

        const auto currSeq = m_currHeaders.seqHdr.get();

        if (currSeq &&
            ((m_firstMfxVideoParams.mfx.FrameInfo.Width < currSeq->horizontal_size_value) || (m_firstMfxVideoParams.mfx.FrameInfo.Height < currSeq->vertical_size_value)))
        {
            return UMC::UMC_NTF_NEW_RESOLUTION;
        }

        return UMC::UMC_WRN_REPOSITION_INPROGRESS;
    }

    // Decode picture header
    UMC::Status MPEG2Decoder::DecodePicHeader(const RawUnit & data)
    {
        auto picHdr    = std::make_shared<MPEG2PictureHeader>();
        auto picExtHdr = std::make_shared<MPEG2PictureCodingExtension>();
        MPEG2HeadersBitstream bitStream(data.begin + prefix_size + 1, data.end - data.begin - prefix_size - 1); // "+ prefix_size + 1" is to skip start code and type

        try
        {
            bitStream.GetPictureHeader(*picHdr.get());

            uint8_t* picExtBegin = RawHeaderIterator::FindStartCode(data.begin + prefix_size + bitStream.BytesDecoded(), data.end); // Find begining of extension
            MFX_CHECK(picExtBegin, UMC::UMC_ERR_INVALID_STREAM);

            bitStream.Reset(picExtBegin + prefix_size, data.end - picExtBegin);
            bitStream.Seek(8 + 4); // skip data and extension types
            bitStream.GetPictureExtensionHeader(*picExtHdr.get()); // decode extension

            // Apply new header
            m_currHeaders.picHdr = std::move(picHdr);
            m_currHeaders.picExtHdr = std::move(picExtHdr);
        }

        catch(const mpeg2_exception & ex)
        {
            return ex.GetStatus();
        }
        catch(...)
        {
            return UMC::UMC_ERR_INVALID_STREAM;
        }

        return UMC::UMC_OK;
    }

    // Decode group of pictures header
    UMC::Status MPEG2Decoder::DecodeGroupHeader(const RawUnit & data)
    {
        auto group = std::make_shared<MPEG2GroupOfPictures>();
        MPEG2HeadersBitstream bitStream(data.begin + prefix_size + 1, data.end - data.begin - prefix_size - 1); // "+ prefix_size + 1" is to skip start code and type

        try
        {
            bitStream.GetGroupOfPicturesHeader(*group.get());

            m_currHeaders.group = std::move(group);
        }

        catch(...)
        {
            return UMC::UMC_ERR_INVALID_STREAM;
        }

        return UMC::UMC_OK;
    }

    // Decode display and matrix extensions
    UMC::Status MPEG2Decoder::DecodeExtension(const RawUnit & data)
    {
        MPEG2HeadersBitstream bitStream(data.begin + prefix_size + 1, data.end - data.begin - prefix_size - 1); // "+ prefix_size + 1" is to skip start code and types

        try
        {
            auto type_ext = (MPEG2_UNIT_TYPE_EXT)bitStream.GetBits(4);

            switch(type_ext)
            {
            case SEQUENCE_DISPLAY_EXTENSION:
                {
                    auto displayExtHdr = std::make_shared<MPEG2SequenceDisplayExtension>();
                    bitStream.GetSequenceDisplayExtension(*displayExtHdr.get());
                    m_currHeaders.displayExtHdr = std::move(displayExtHdr); // apply new header
                }
                break;
            case QUANT_MATRIX_EXTENSION:
                {
                    auto quantMatrix = std::make_shared<MPEG2QuantMatrix>();
                    bitStream.GetQuantMatrix(*quantMatrix.get());
                    m_currHeaders.qmatrix = std::move(quantMatrix); // apply new header
                }
                break;
            default:
                break;
            }
        }
        catch(const mpeg2_exception & ex)
        {
            return ex.GetStatus();
        }
        catch(...)
        {
            return UMC::UMC_ERR_INVALID_STREAM;
        }

        return UMC::UMC_OK;
    }

    // Decode a bitstream header
    UMC::Status MPEG2Decoder::DecodeHeaders(const RawUnit & data)
    {
        UMC::Status sts = UMC::UMC_OK;

        switch(data.type)
        {
        case SEQUENCE_HEADER:
            sts = DecodeSeqHeader(data);
            break;
        case PICTURE_HEADER:
            sts = DecodePicHeader(data);
            break;
        case EXTENSION:
            sts = DecodeExtension(data);
            break;
        case GROUP:
            sts = DecodeGroupHeader(data);
            break;
        default:
            break;
        }

        return sts;
    }

    MPEG2DecoderFrame* MPEG2Decoder::GetFrameBuffer(MPEG2Slice * /*slice*/)
    {
        CompleteDecodedFrames();
        auto frame = GetFreeFrame();
        if (!frame)
            return nullptr;

        frame->Reset();

        frame->IncrementReference();

        frame->decOrder = frame->displayOrder = m_counter++;

        UMC::VideoDataInfo info {};
        auto sts = info.Init(m_params.info.clip_info.width, m_params.info.clip_info.height, m_params.info.color_format, 0);
        if (sts != UMC::UMC_OK)
            throw mpeg2_exception(sts);

        UMC::FrameMemID id;
        sts = m_allocator->Alloc(&id, &info, 0);
        if (sts != UMC::UMC_OK)
            throw mpeg2_exception(UMC::UMC_ERR_ALLOC);

        AllocateFrameData(info, id, *frame);

        return frame;
    }

    // Initialize just allocated frame with parameters
    void InitFreeFrame(MPEG2DecoderFrame& frame, const MPEG2Slice& slice, const UMC::sVideoStreamInfo& info)
    {
        auto seq    = slice.GetSeqHeader();
        auto seqExt = slice.GetSeqExtHeader();
        auto pic    = slice.GetPicHeader();
        auto picExt = slice.GetPicExtHeader();

        frame.frameType  = (FrameType)pic.picture_coding_type;
        frame.dFrameTime = slice.source.GetTime();
        frame.isProgressiveSequence = seqExt.progressive_sequence;
        frame.isProgressiveFrame    = picExt.progressive_frame;

        if (picExt.picture_structure != FRM_PICTURE)
        {
            frame.bottom_field_flag[0] = (picExt.picture_structure == BOTTOM_FLD_PICTURE);
            frame.bottom_field_flag[1] = !(frame.bottom_field_flag[0]);

            frame.pictureStructure = FLD_PICTURE;
        }
        else
        {
            frame.bottom_field_flag[0] = 0;
            frame.bottom_field_flag[1] = 1;

            frame.pictureStructure = FRM_PICTURE;
        }

        frame.horizontalSize = seq.horizontal_size_value;
        frame.verticalSize   = seq.vertical_size_value;

        frame.aspectWidth  = info.aspect_ratio_width;
        frame.aspectHeight = info.aspect_ratio_height;

        // Display picture structure is retrieved according to 6.3.10
        {
            if (seqExt.progressive_sequence == 0)
            {
                if (picExt.picture_structure != FRM_PICTURE)
                {
                    frame.displayPictureStruct = (picExt.picture_structure == TOP_FLD_PICTURE) ? DPS_TOP : DPS_BOTTOM;
                }
                else
                {
                    if (picExt.repeat_first_field == 0)
                    {
                        frame.displayPictureStruct = picExt.top_field_first ? DPS_TOP_BOTTOM : DPS_BOTTOM_TOP;
                    }
                    else
                    {
                        frame.displayPictureStruct = picExt.top_field_first ? DPS_TOP_BOTTOM_TOP : DPS_BOTTOM_TOP_BOTTOM;
                    }
                }
            }
            else
            {
                if (picExt.repeat_first_field == 0)
                {
                    frame.displayPictureStruct = DPS_FRAME;
                }
                else // repeat_first_field == 1
                {
                    if (picExt.top_field_first == 0)
                    {
                        frame.displayPictureStruct = DPS_FRAME_DOUBLING;
                    }
                    else
                    {
                       frame.displayPictureStruct  = DPS_FRAME_TRIPLING;
                    }
                }
            }
        }

        frame.currFieldIndex = frame.GetNumberByParity(picExt.picture_structure == BOTTOM_FLD_PICTURE);
    }

    MPEG2DecoderFrame* MPEG2Decoder::StartFrame(MPEG2Slice * slice)
    {
        auto frame = GetFrameBuffer(slice);
        if (!frame)
            return nullptr;

        InitFreeFrame(*frame, *slice, m_params.info);

        frame->group = m_currHeaders.group;

        if (m_messages)
            m_messages->SetAUID(frame->currFieldIndex);

        // 6.1.1.11 - Reordering:
        // - If a current frame is a B-frame, then output it immediately.
        // - if a current frame is a I-frame or P-frame, then output previous (in decoded order) I-frame or P-frame.
        if (frame->frameType == MPEG2_B_PICTURE)
        {
            frame->SetReadyToBeOutputted();

            std::unique_lock<std::mutex> l(m_guard);
            auto last = FindLastRefFrame(*frame, m_dpb);
            if (last)
            {
                // Here we're building display order in case of reordered frames:
                frame->displayOrder = last->displayOrder; // 1. Output B frames sooner than their decoded order
                last->displayOrder  = frame->decOrder;    // 2. Delay displaying I/P frames accordingly
            }
        }
        else
        {
            std::unique_lock<std::mutex> l(m_guard);
            auto last = FindLastRefFrame(*frame, m_dpb);
            if (last)
                last->SetReadyToBeOutputted();
        }

        if (m_messages)
        {
            m_messages->SetFrame(frame, frame->currFieldIndex);
        }

        return frame;
    }

    bool MPEG2Decoder::IsFieldOfCurrentFrame() const
    {
        const auto firstFrameSlice = m_currFrame->GetAU(m_currFrame->fieldIndex)->GetSlice(0);
        if (!firstFrameSlice)
        {
            throw mpeg2_exception(UMC::UMC_ERR_NULL_PTR);
        }

        const auto picHdr = firstFrameSlice->GetPicHeader();
        const auto picExtHdr = firstFrameSlice->GetPicExtHeader();
        const auto newPicHdr = *m_currHeaders.picHdr.get();
        const auto newPicExtHdr = *m_currHeaders.picExtHdr.get();

        // this is a workaround (and actually not by spec) to handle invalid streams where an II or IP pair has different temporal_reference
        if (m_currFrame->frameType != MPEG2_I_PICTURE || m_currFrame->currFieldIndex != 0)
        {
            if (picHdr.temporal_reference != newPicHdr.temporal_reference)  // 6.3.9
                return false;
        }

        if (picExtHdr.picture_structure == newPicExtHdr.picture_structure) // the same type fields
            return false;

        return true;
    }

    bool MPEG2Decoder::OnNewPicture()
    {
        // Progressive picture
        if (FRM_PICTURE == m_currFrame->pictureStructure)
        {
            m_currFrame->SetFullFrame(true);
            CompletePicture(*m_currFrame, m_currFrame->currFieldIndex);

            m_currFrame = nullptr;

            return true; // Full frame
        }

        const auto newPicExtHdr = *m_currHeaders.picExtHdr.get();

        if (IsFieldOfCurrentFrame())
        {
            CompletePicture(*m_currFrame, m_currFrame->currFieldIndex); // complete the first field of the current frame

            m_currFrame->currFieldIndex = m_currFrame->GetNumberByParity(newPicExtHdr.picture_structure == BOTTOM_FLD_PICTURE);
            if (m_messages)
                m_messages->SetAUID(m_currFrame->currFieldIndex);
        }
        else
        {
            m_currFrame->SetFullFrame(true);
            CompletePicture(*m_currFrame, m_currFrame->currFieldIndex); // complete current field of interlace frame
            m_currFrame = nullptr;
            return true;
        }

        return false;
    }

    void MPEG2Decoder::EliminateSliceErrors(MPEG2DecoderFrame& frame, uint8_t fieldIndex)
    {
        MPEG2DecoderFrameInfo & frameInfo = *frame.GetAU(fieldIndex);
        size_t sliceCount = frameInfo.GetSliceCount();

        for (size_t sliceNum = 0; sliceNum < sliceCount; sliceNum++)
        {
            auto slice = frameInfo.GetSlice(sliceNum);
            auto sliceHeader = slice->GetSliceHeader();

            // Slice may cover just a part of a row (not full row), this is indicated by macroblockAddressIncrement
            // Looping over slices, we heed to check if next slice has macroblockAddressIncrement > 0,
            // this means that we need to recalculate numberMBsInSlice for previous slice
            if (sliceNum > 0 && sliceHeader.macroblockAddressIncrement > 0)
            {
                auto prevSlice = frameInfo.GetSlice(sliceNum - 1); //take previous
                auto& prevSliceHeader = prevSlice->GetSliceHeader();

                // Check if this parts are located at the same row
                if (sliceHeader.slice_vertical_position == prevSliceHeader.slice_vertical_position)
                    prevSliceHeader.numberMBsInSlice = sliceHeader.macroblockAddressIncrement - prevSliceHeader.macroblockAddressIncrement;
            }
        }
        return;
    }

    UMC::Status MPEG2Decoder::CompletePicture(MPEG2DecoderFrame& frame, uint8_t fieldIndex)
    {
        frame.StartDecoding();

        MPEG2DecoderFrameInfo & frameInfo = *frame.GetAU(fieldIndex);
        frameInfo.SetFilled();

        UpdateDPB(frame, fieldIndex);

        // skipping algorithm
        const MPEG2DecoderFrameInfo & info = *frame.GetAU(fieldIndex);
        if (((info.IsField() && fieldIndex) || !info.IsField()) && IsShouldSkipFrame(frame))
        {
            frame.SetIsRef(false);
            frame.SetSkipped(true);
            frame.OnDecodingCompleted();
            return UMC::UMC_OK;
        }

        size_t sliceCount = frameInfo.GetSliceCount();

        if (sliceCount)
        {
            EliminateSliceErrors(frame, fieldIndex);
            Submit(frame, fieldIndex);
        }
        else
        {
            if (((info.IsField() && fieldIndex) || !info.IsField()))
                frame.OnDecodingCompleted();
        }

        return UMC::UMC_OK;
    }

    void MPEG2Decoder::UpdateDPB(MPEG2DecoderFrame& frame, uint8_t fieldIndex)
    {
        if (frame.frameType == MPEG2_B_PICTURE || fieldIndex) // Update DPB on reference pictures, the first field for field pictures
            return;

        auto numRefFrames = std::count_if(m_dpb.begin(), m_dpb.end(),
                                    [&frame](MPEG2DecoderFrame const * f)
                                    {
                                        return (&frame != f) && f->IsRef();
                                    }
                                );

        if (NUM_REF_FRAMES == numRefFrames)
        {
            auto oldest = std::min_element(std::begin(m_dpb), std::end(m_dpb),
                [](MPEG2DecoderFrame const* f1, MPEG2DecoderFrame const* f2)
                {
                    uint32_t const id1 = f1->IsRef() ? f1->decOrder : (std::numeric_limits<uint32_t>::max)();
                    uint32_t const id2 = f2->IsRef() ? f2->decOrder : (std::numeric_limits<uint32_t>::max)();

                    return  id1 < id2;
                }
            );

            if(oldest != std::end(m_dpb))
                (*oldest)->SetIsRef(false); // Release the oldest reference frame ...
        }

        frame.SetIsRef(true);            // ... and increase a ref counter for a current frame
    }

    void MPEG2Decoder::SetDPBSize(uint32_t size)
    {
        m_dpbSize = size;
    }

    void MPEG2Decoder::CompleteDecodedFrames()
    {
        std::unique_lock<std::mutex> l(m_guard);

        std::for_each(m_dpb.begin(), m_dpb.end(),
            [](MPEG2DecoderFrame* frame)
            {
                if (frame->IsOutputted() && frame->IsDisplayed() && !frame->IsDecoded())
                    frame->OnDecodingCompleted();
            }
        );
    }

    MPEG2DecoderFrame* MPEG2Decoder::GetFreeFrame()
    {
        std::unique_lock<std::mutex> l(m_guard);

        MPEG2DecoderFrame* frame = (m_dpb.size() >= m_dpbSize) ? FindFreeFrame(m_dpb) : nullptr;

        // If nothing found
        if (!frame)
        {
            // Can not allocate more
            if (m_dpb.size() >= m_dpbSize)
            {
                return nullptr;
            }

            // Didn't find any. Let's create a new one
            frame = new MPEG2DecoderFrame;

            // Add to DPB
            m_dpb.push_back(frame);
        }

        return frame;
    }

    // Find a next frame ready to be output from decoder
    MPEG2DecoderFrame* MPEG2Decoder::GetFrameToDisplay()
    {
        std::unique_lock<std::mutex> l(m_guard);

        DPBType displayable = m_dpb;
        displayable.remove_if(
            [](MPEG2DecoderFrame const * f)
            {
                return !f->IsReadyToBeOutputted() || !f->IsFullFrame() || !f->DecodingStarted() || f->IsOutputted() || f->IsDisplayed();
            }
        );

        // Oldest frame
        auto i = std::min_element(std::begin(displayable), std::end(displayable),
            [](MPEG2DecoderFrame const* f1, MPEG2DecoderFrame const* f2)
            {
                return  f1->displayOrder < f2->displayOrder;
            }
        );

        return (i == std::end(displayable)) ? nullptr : *i;
    }

    void MPEG2Decoder::PostProcessDisplayFrame(MPEG2DecoderFrame *frame)
    {
        if (!frame || frame->isPostProccesComplete)
            return;

        frame->isOriginalPTS = frame->dFrameTime > -1.0;
        if (frame->isOriginalPTS)
        {
            m_localFrameTime = frame->dFrameTime;
        }
        else
        {
            frame->dFrameTime = m_localFrameTime;
        }

        switch (frame->displayPictureStruct)
        {
        case DPS_TOP_BOTTOM_TOP:
        case DPS_BOTTOM_TOP_BOTTOM:
            if (m_params.lFlags & UMC::FLAG_VDEC_TELECINE_PTS)
            {
                m_localFrameTime += (m_localDeltaFrameTime / 2);
            }
            break;
        case DPS_FRAME_DOUBLING:
            if (m_params.lFlags & UMC::FLAG_VDEC_TELECINE_PTS)
            {
                m_localFrameTime += m_localDeltaFrameTime;
            }
            break;
        case DPS_FRAME_TRIPLING:
            if (m_params.lFlags & UMC::FLAG_VDEC_TELECINE_PTS)
            {
                m_localFrameTime += (m_localDeltaFrameTime * 2);
            }
            break;
        default:
            break;
        }

        m_localFrameTime += m_localDeltaFrameTime;

        frame->isPostProccesComplete = true;
    }

    template <typename F>
    MPEG2DecoderFrame* FindFrame(F pred, const DPBType & dpb)
    {
        auto i = std::find_if(std::begin(dpb), std::end(dpb), pred);
        return i != std::end(dpb) ? (*i) : nullptr;
    }

    MPEG2DecoderFrame* MPEG2Decoder::FindFrameByMemID(UMC::FrameMemID id)
    {
        std::unique_lock<std::mutex> l(m_guard);
        return FindFrame(
            [id](MPEG2DecoderFrame const* f)
            { return f->GetMemID() == id; }, m_dpb
        );
    }

    uint8_t MPEG2Decoder::GetNumCachedFrames() const
    {
        return std::count_if(m_dpb.begin(), m_dpb.end(),
                                    [](MPEG2DecoderFrame const * f) { return (f->DecodingStarted() && !f->IsOutputted()); }
                            );
    }

/****************************************************************************************************/
// Payload_Storage
/****************************************************************************************************/
    Payload_Storage::Payload_Storage()
    {
        m_offset = 0;
        Reset();
    }

    Payload_Storage::~Payload_Storage()
    {
        Close();
    }

    void Payload_Storage::Init()
    {
        Close();
        m_data.resize(MAX_BUFFERED_SIZE);
        m_payloads.resize(START_ELEMENTS);
        m_offset = 0;
        m_lastUsed = 2;
    }

    void Payload_Storage::Close()
    {
        Reset();
        m_data.clear();
        m_payloads.clear();
    }

    void Payload_Storage::Reset()
    {
        m_lastUsed = 2;
        std::for_each(m_payloads.begin(), m_payloads.end(), [](Message& m) { m.isUsed = 0; } );
    }

    void Payload_Storage::SetFrame(MPEG2DecoderFrame * frame, int32_t auIndex)
    {
        for (uint32_t i = 0; i < m_payloads.size(); i++)
        {
            if (m_payloads[i].frame == 0 && m_payloads[i].isUsed && m_payloads[i].auID == auIndex)
            {
                m_payloads[i].frame = frame;
            }
        }
    }

    void Payload_Storage::SetAUID(int32_t auIndex)
    {
        for (uint32_t i = 0; i < m_payloads.size(); i++)
        {
            if (m_payloads[i].isUsed && m_payloads[i].auID == -1)
            {
                m_payloads[i].auID = auIndex;
            }
        }
    }

    void Payload_Storage::SetTimestamp(MPEG2DecoderFrame * frame)
    {
        double ts = frame->dFrameTime;

        for (uint32_t i = 0; i < m_payloads.size(); i++)
        {
            if (m_payloads[i].frame == frame)
            {
                m_payloads[i].timestamp = ts;
                if (m_payloads[i].isUsed)
                    m_payloads[i].isUsed = m_lastUsed;
            }
        }

        m_lastUsed++;
    }

    const Payload_Storage::Message * Payload_Storage::GetPayloadMessage()
    {
        Payload_Storage::Message * msg = nullptr;

        for (uint32_t i = 0; i < m_payloads.size(); i++)
        {
            if (m_payloads[i].isUsed > 1)
            {
                if (!msg || msg->isUsed > m_payloads[i].isUsed || msg->inputID > m_payloads[i].inputID)
                {
                    msg = &m_payloads[i];
                }
            }
        }

        if (msg)
        {
            msg->isUsed  = 0;
            msg->frame   = nullptr;
            msg->auID    = 0;
            msg->inputID = 0;
        }

        return msg;
    }

    Payload_Storage::Message* Payload_Storage::AddMessage(const RawUnit & data, int32_t auIndex)
    {
        size_t sz = data.end - data.begin;

        if (sz > (m_data.size() >> 2))
            return nullptr;

        if (m_offset + sz > m_data.size())
        {
            m_offset = 0;
        }

        // clear overwriting messages:
        for (uint32_t i = 0; i < m_payloads.size(); i++)
        {
            if (!m_payloads[i].isUsed)
                continue;

            Message & mmsg = m_payloads[i];

            if ((m_offset + sz > mmsg.offset) &&
                (m_offset < mmsg.offset + mmsg.msg_size))
            {
                m_payloads[i].isUsed = 0;
                return nullptr;
            }
        }

        size_t freeSlot = 0;
        for (uint32_t i = 0; i < m_payloads.size(); i++)
        {
            if (!m_payloads[i].isUsed)
            {
                freeSlot = i;
                break;
            }
        }

        if (m_payloads[freeSlot].isUsed)
        {
            if (m_payloads.size() >= MAX_ELEMENTS)
                return nullptr;

            m_payloads.emplace_back(Message());
            freeSlot = m_payloads.size() - 1;
        }

        m_payloads[freeSlot].msg_size = sz;
        m_payloads[freeSlot].offset = m_offset;
        m_payloads[freeSlot].timestamp = 0;
        m_payloads[freeSlot].frame = nullptr;
        m_payloads[freeSlot].isUsed = 1;
        m_payloads[freeSlot].inputID = m_lastUsed++;
        m_payloads[freeSlot].data = m_data.data() + m_offset;
        m_payloads[freeSlot].auID = auIndex;

        std::copy(data.begin, data.end, m_data.data() + m_offset);

        m_offset += sz;
        return &m_payloads[freeSlot];
    }

/****************************************************************************************************/
// Skipping_MPEG2 class routine
/****************************************************************************************************/
    void Skipping_MPEG2::Reset()
    {
        m_SkipLevel = SKIP_NONE;
        m_NumberOfSkippedFrames = 0;
    }

    // Check if frame should be skipped to decrease decoding delays
    bool Skipping_MPEG2::IsShouldSkipFrame(const MPEG2DecoderFrame & frame)
    {
        bool isShouldSkip = false;

        switch (m_SkipLevel)
        {
            case SKIP_ALL:
                isShouldSkip = true;
                break;

            case SKIP_B:
                isShouldSkip = (frame.frameType == MPEG2_B_PICTURE);
                break;

            case SKIP_PB:
                isShouldSkip = (frame.frameType != MPEG2_I_PICTURE);
                break;
            default:  // SKIP_NONE
                break;
        }

        m_NumberOfSkippedFrames += isShouldSkip;

        return isShouldSkip;
    }

    // Set decoding skip frame mode
    void Skipping_MPEG2::ChangeVideoDecodingSpeed(int32_t & val)
    {
        m_SkipLevel += val;

        if (m_SkipLevel < SKIP_NONE)
            m_SkipLevel = SKIP_NONE;
        if (m_SkipLevel > SKIP_ALL)
            m_SkipLevel = SKIP_ALL;

        val = m_SkipLevel;
    }
}
#endif // MFX_ENABLE_MPEG2_VIDEO_DECODE
