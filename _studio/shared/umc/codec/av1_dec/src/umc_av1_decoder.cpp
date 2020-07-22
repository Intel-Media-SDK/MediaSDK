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
#include "umc_frame_data.h"

#include "umc_av1_decoder.h"
#include "umc_av1_utils.h"
#include "umc_av1_bitstream.h"
#include "umc_av1_frame.h"

#include <algorithm>

namespace UMC_AV1_DECODER
{
    AV1Decoder::AV1Decoder()
        : allocator(nullptr)
        , sequence_header(nullptr)
        , counter(0)
        , Poutput(new AV1DecoderFrame{})
        , Curr(new AV1DecoderFrame{})
        , Curr_temp(new AV1DecoderFrame{})
        , Repeat_show(0)
        , PreFrame_id(0)
    {
    }

    AV1Decoder::~AV1Decoder()
    {
        std::for_each(std::begin(dpb), std::end(dpb),
            std::default_delete<AV1DecoderFrame>()
        );
    }

    inline bool CheckOBUType(AV1_OBU_TYPE type)
    {
        switch (type)
        {
        case OBU_TEMPORAL_DELIMITER:
        case OBU_SEQUENCE_HEADER:
        case OBU_FRAME_HEADER:
        case OBU_REDUNDANT_FRAME_HEADER:
        case OBU_FRAME:
        case OBU_TILE_GROUP:
        case OBU_METADATA:
        case OBU_PADDING:
            return true;
        default:
            return false;
        }
    }

    UMC::Status AV1Decoder::DecodeHeader(UMC::MediaData* in, UMC_AV1_DECODER::AV1DecoderParams& par)
    {
        if (!in)
            return UMC::UMC_ERR_NULL_PTR;

        SequenceHeader sh = {};

        while (in->GetDataSize() >= MINIMAL_DATA_SIZE)
        {
            try
            {
                const auto src = reinterpret_cast<uint8_t*>(in->GetDataPointer());
                AV1Bitstream bs(src, uint32_t(in->GetDataSize()));

                OBUInfo obuInfo;
                bs.ReadOBUInfo(obuInfo);
                VM_ASSERT(CheckOBUType(obuInfo.header.obu_type)); // TODO: [clean up] Need to remove assert once decoder code is stabilized

                if (obuInfo.header.obu_type == OBU_SEQUENCE_HEADER)
                {
                    bs.ReadSequenceHeader(sh);

                    in->MoveDataPointer(static_cast<int32_t>(obuInfo.size));

                    if (FillVideoParam(sh, par) == UMC::UMC_OK)
                        return UMC::UMC_OK;
                }

                in->MoveDataPointer(static_cast<int32_t>(obuInfo.size));
            }
            catch (av1_exception const& e)
            {
                return e.GetStatus();
            }
        }

        return UMC::UMC_ERR_NOT_ENOUGH_DATA;
    }

    UMC::Status AV1Decoder::Init(UMC::BaseCodecParams* vp)
    {
        if (!vp)
            return UMC::UMC_ERR_NULL_PTR;

        AV1DecoderParams* dp =
            DynamicCast<AV1DecoderParams, UMC::BaseCodecParams>(vp);
        if (!dp)
            return UMC::UMC_ERR_INVALID_PARAMS;

        if (!dp->allocator)
            return UMC::UMC_ERR_INVALID_PARAMS;
        allocator = dp->allocator;

        params = *dp;
        return SetParams(vp);
    }

    UMC::Status AV1Decoder::GetInfo(UMC::BaseCodecParams* info)
    {
        AV1DecoderParams* vp =
            DynamicCast<AV1DecoderParams, UMC::BaseCodecParams>(info);

        if (!vp)
            return UMC::UMC_ERR_INVALID_PARAMS;

        *vp = params;
        return UMC::UMC_OK;
    }

    DPBType DPBUpdate(AV1DecoderFrame const * prevFrame)
    {
        VM_ASSERT(prevFrame);

        DPBType updatedFrameDPB;

        DPBType const& prevFrameDPB = prevFrame->frame_dpb;
        if (prevFrameDPB.empty())
            updatedFrameDPB.resize(NUM_REF_FRAMES);
        else
            updatedFrameDPB = prevFrameDPB;

        const FrameHeader& fh = prevFrame->GetFrameHeader();

        for (uint8_t i = 0; i < NUM_REF_FRAMES; i++)
        {
            if ((fh.refresh_frame_flags >> i) & 1)
            {
                if (!prevFrameDPB.empty() && prevFrameDPB[i] && prevFrameDPB[i]->UID != -1)
                    prevFrameDPB[i]->DecrementReference();

                updatedFrameDPB[i] = const_cast<AV1DecoderFrame*>(prevFrame);
                prevFrame->IncrementReference();
            }
        }

        return updatedFrameDPB;
    }

    static void GetTileLocation(
        AV1Bitstream* bs,
        FrameHeader const& fh,
        TileGroupInfo const& tgInfo,
        uint32_t idxInTG,
        size_t OBUSize,
        size_t OBUOffset,
        TileLocation& loc)
    {
        // calculate tile row and column
        const uint32_t idxInFrame = tgInfo.startTileIdx + idxInTG;
        loc.row = idxInFrame / fh.tile_info.TileCols;
        loc.col = idxInFrame - loc.row * fh.tile_info.TileCols;

        size_t tileOffsetInTG = bs->BytesDecoded();

        if (idxInTG == tgInfo.numTiles - 1)
            loc.size = OBUSize - tileOffsetInTG;  // tile is last in tile group - no explicit size signaling
        else
        {
            tileOffsetInTG += fh.tile_info.TileSizeBytes;

            // read tile size
            size_t reportedSize = 0;
            size_t actualSize = 0;
            bs->ReadTile(fh, reportedSize, actualSize);
            if (actualSize != reportedSize)
            {
                // before parsing tiles we check that tile_group_obu() is complete (bitstream has enough bytes to hold whole OBU)
                // but here we encountered incomplete tile inside this tile_group_obu() which means tile size corruption
                // [maybe] later check for complete tile_group_obu() will be removed, and thus incomplete tile will be possible
                VM_ASSERT("Tile size corruption: Incomplete tile encountered inside complete tile_group_obu()!");
                throw av1_exception(UMC::UMC_ERR_INVALID_STREAM);
            }

            loc.size = reportedSize;
        }

        loc.offset = OBUOffset + tileOffsetInTG;
    }

    inline bool CheckTileGroup(uint32_t prevNumTiles, FrameHeader const& fh, TileGroupInfo const& tgInfo)
    {
        if (prevNumTiles + tgInfo.numTiles > NumTiles(fh.tile_info))
            return false;

        if (tgInfo.numTiles == 0)
            return false;

        return true;
    }

    AV1DecoderFrame* AV1Decoder::StartFrame(FrameHeader const& fh, DPBType & frameDPB, AV1DecoderFrame* pPrevFrame)
    {
        AV1DecoderFrame* pFrame = nullptr;

        if (fh.show_existing_frame)
        {
            pFrame = frameDPB[fh.frame_to_show_map_idx];
            pFrame->IncrementReference();
            VM_ASSERT(pFrame);
            FrameHeader const& refFH = pFrame->GetFrameHeader();

            if (!refFH.showable_frame)
                throw av1_exception(UMC::UMC_ERR_INVALID_STREAM);

            FrameHeader const& Repeat_H = pFrame->GetFrameHeader();
            if (Repeat_H.frame_type == KEY_FRAME)
            {
                for (uint8_t i = 0; i < NUM_REF_FRAMES; i++)
                {
                    if ((Repeat_H.refresh_frame_flags >> i) & 1)
                    {
                        if (!frameDPB.empty() && frameDPB[i])
                           frameDPB[i]->DecrementReference();

                        frameDPB[i] = const_cast<AV1DecoderFrame*>(pFrame);
                        pFrame->IncrementReference();
                    }
                }
            }
            refs_temp = frameDPB;
            if (pPrevFrame)
            {
                DPBType & prevFrameDPB = pPrevFrame->frame_dpb;
                prevFrameDPB = frameDPB;
            }

            return pFrame;
        }
        else
            pFrame = GetFrameBuffer(fh);

        if (!pFrame)
            return nullptr;

        pFrame->SetSeqHeader(*sequence_header.get());

        if (fh.refresh_frame_flags)
            pFrame->SetRefValid(true);

        pFrame->frame_dpb = frameDPB;
        pFrame->UpdateReferenceList();

        if (!params.film_grain)
            pFrame->DisableFilmGrain();

        return pFrame;
    }

    static void ReadTileGroup(TileLayout& layout, AV1Bitstream& bs, FrameHeader const& fh, size_t obuOffset, size_t obuSize)
    {
        TileGroupInfo tgInfo = {};
        bs.ReadTileGroupHeader(fh, tgInfo);

        if (!CheckTileGroup(static_cast<uint32_t>(layout.size()), fh, tgInfo))
            throw av1_exception(UMC::UMC_ERR_INVALID_STREAM);

        uint32_t idxInLayout = static_cast<uint32_t>(layout.size());

        layout.resize(layout.size() + tgInfo.numTiles,
            { tgInfo.startTileIdx, tgInfo.endTileIdx });

        for (int idxInTG = 0; idxInLayout < layout.size(); idxInLayout++, idxInTG++)
        {
            TileLocation& loc = layout[idxInLayout];
            GetTileLocation(&bs, fh, tgInfo, idxInTG, obuSize, obuOffset, loc);
        }
    }

    inline bool NextFrameDetected(AV1_OBU_TYPE obuType)
    {
        switch (obuType)
        {
        case OBU_REDUNDANT_FRAME_HEADER:
        case OBU_TILE_GROUP:
            return false;
        default:
            return true;
        }
    }

    inline bool GotFullFrame(AV1DecoderFrame const* curr_frame, FrameHeader const& fh, TileLayout const& layout)
    {
        const unsigned numMissingTiles = curr_frame ? GetNumMissingTiles(*curr_frame) : NumTiles(fh.tile_info);
        if (layout.size() == numMissingTiles) // current tile group_obu() contains all missing tiles
            return true;
        else
            return false;
    }

    inline bool HaveTilesToSubmit(AV1DecoderFrame const* curr_frame, TileLayout const& layout)
    {
        if (!layout.empty() ||
            (curr_frame && BytesReadyForSubmission(curr_frame->GetTileSets())))
            return true;

        return false;
    }

    inline bool AllocComplete(AV1DecoderFrame const& frame)
    {
        return frame.GetFrameData(SURFACE_DISPLAY) &&
            frame.GetFrameData(SURFACE_RECON);
    }

    inline bool FrameInProgress(AV1DecoderFrame const& frame)
    {
        // frame preparation to decoding is in progress
        // i.e. SDK decoder still getting tiles of the frame from application (has both arrived and missing tiles)
        //      or it got all tiles and waits for output surface to start decoding
        if (GetNumArrivedTiles(frame) == 0)
            return false;

        return GetNumMissingTiles(frame) || !AllocComplete(frame);
    }

    UMC::Status AV1Decoder::GetFrame(UMC::MediaData* in, UMC::MediaData*)
    {
        if (!in)
            return UMC::UMC_ERR_NULL_PTR;

        FrameHeader fh = {};

        AV1DecoderFrame* pPrevFrame = FindFrameByUID(counter - 1);
        AV1DecoderFrame* pFrameInProgress = FindFrameInProgress();
        DPBType updated_refs;
        UMC::MediaData tmper = *in;

        if ((tmper.GetDataSize() >= MINIMAL_DATA_SIZE) && pPrevFrame && !pFrameInProgress)
        {
            if (!Repeat_show)
            {
                if (Curr_temp != Curr)
                {
                    updated_refs = DPBUpdate(pPrevFrame);
                    refs_temp = updated_refs;
                }
                else
                {
                    updated_refs = refs_temp;
                }

            }
            else
            {
                DPBType const& prevFrameDPB = pPrevFrame->frame_dpb;
                updated_refs = prevFrameDPB;
                Repeat_show = 0;
            }
            Curr_temp = Curr;
        }
        if (!pPrevFrame)
        {
            updated_refs = refs_temp;
            Curr_temp = Curr;
        }

        bool gotFullFrame = false;
        bool repeatedFrame = false;

        AV1DecoderFrame* pCurrFrame = nullptr;

        if (pFrameInProgress && GetNumMissingTiles(*pFrameInProgress) == 0)
        {
            /* this code is executed if and only if whole frame (all tiles) was already got from applicaiton during previous calls of DecodeFrameAsync
               but there were no sufficient surfaces to start decoding (e.g. to apply film_grain)
               in this case reading from bitstream must be skipped, and code should proceed to frame submission to the driver */

            VM_ASSERT(!AllocComplete(*pFrameInProgress));
            pCurrFrame = pFrameInProgress;
            gotFullFrame = true;
        }
        else
        {
            /*do bitstream parsing*/

            bool gotFrameHeader = false;
            uint32_t OBUOffset = 0;
            TileLayout layout;

            UMC::MediaData tmp = *in; // use local copy of [in] for OBU header parsing to not move data pointer in original [in] prematurely

            while (tmp.GetDataSize() >= MINIMAL_DATA_SIZE && gotFullFrame == false && repeatedFrame == false)
            {
                const auto src = reinterpret_cast<uint8_t*>(tmp.GetDataPointer());
                AV1Bitstream bs(src, uint32_t(tmp.GetDataSize()));

                OBUInfo obuInfo;
                bs.ReadOBUInfo(obuInfo);
                const AV1_OBU_TYPE obuType = obuInfo.header.obu_type;
                VM_ASSERT(CheckOBUType(obuType)); // TODO: [clean up] Need to remove assert once decoder code is stabilized

                if (tmp.GetDataSize() < obuInfo.size) // not enough data left in the buffer to hold full OBU unit
                    break;

                if (pFrameInProgress && NextFrameDetected(obuType))
                {
                    VM_ASSERT(!"Current frame was interrupted unexpectedly!");
                    throw av1_exception(UMC::UMC_ERR_INVALID_STREAM);
                    // TODO: [robust] add support for cases when series of tile_group_obu() interrupted by other OBU type before end of frame was reached
                }

                switch (obuType)
                {
                case OBU_SEQUENCE_HEADER:
                    if (!sequence_header.get())
                        sequence_header.reset(new SequenceHeader);
                    *sequence_header = SequenceHeader{};
                    bs.ReadSequenceHeader(*sequence_header);
                    break;
                case OBU_FRAME_HEADER:
                case OBU_REDUNDANT_FRAME_HEADER:
                case OBU_FRAME:
                    if (!sequence_header.get())
                        break; // bypass frame header if there is no active seq header
                    if (!gotFrameHeader)
                    {
                        // we read only first entry of uncompressed header in the frame
                        // each subsequent copy of uncompressed header (i.e. OBU_REDUNDANT_FRAME_HEADER) must be exact copy of first entry by AV1 spec
                        // TODO: [robust] maybe need to add check that OBU_REDUNDANT_FRAME_HEADER contains copy of OBU_FRAME_HEADER
                        bs.ReadUncompressedHeader(fh, *sequence_header, updated_refs, obuInfo.header, PreFrame_id);
                        gotFrameHeader = true;
                    }
                    if (obuType != OBU_FRAME)
                    {
                        if (fh.show_existing_frame)
                            repeatedFrame = true;
                        break;
                    }
                    bs.ReadByteAlignment();
                case OBU_TILE_GROUP:
                {
                    FrameHeader const* pFH = nullptr;
                    if (pFrameInProgress)
                        pFH = &(pFrameInProgress->GetFrameHeader());
                    else if (gotFrameHeader)
                        pFH = &fh;

                    if (pFH) // bypass tile group if there is no respective frame header
                    {
                        ReadTileGroup(layout, bs, *pFH, OBUOffset, obuInfo.size);
                        gotFullFrame = GotFullFrame(pFrameInProgress, *pFH, layout);
                        break;
                    }
                }
                default:
                    break;
                }

                OBUOffset += static_cast<uint32_t>(obuInfo.size);
                tmp.MoveDataPointer(static_cast<int32_t>(obuInfo.size));
            }

            if (!HaveTilesToSubmit(pFrameInProgress, layout) && !repeatedFrame)
                return UMC::UMC_ERR_NOT_ENOUGH_DATA;

            pCurrFrame = pFrameInProgress ?
                pFrameInProgress : StartFrame(fh, updated_refs, pPrevFrame);

            CompleteDecodedFrames(fh, pCurrFrame, pPrevFrame);

            if (!pCurrFrame)
                return UMC::UMC_ERR_NOT_ENOUGH_BUFFER;

            if (!layout.empty())
                pCurrFrame->AddTileSet(in, layout);

            if (!layout.empty() || repeatedFrame)
                in->MoveDataPointer(OBUOffset);

            if (repeatedFrame)
            {
                pCurrFrame->ShowAsExisting(true);
                return UMC::UMC_OK;
            }

        }

        bool firstSubmission = false;

        if (!AllocComplete(*pCurrFrame))
        {
            AddFrameData(*pCurrFrame);

            if (!AllocComplete(*pCurrFrame))
                return UMC::UMC_OK;

            firstSubmission = true;
        }

        UMC::Status umcRes = SubmitTiles(*pCurrFrame, firstSubmission);
        if (umcRes != UMC::UMC_OK)
            return umcRes;

        if (!gotFullFrame)
            return UMC::UMC_ERR_NOT_ENOUGH_DATA;

        return UMC::UMC_OK;
    }

    AV1DecoderFrame* AV1Decoder::FindFrameByMemID(UMC::FrameMemID id)
    {
        return FindFrame(
            [id](AV1DecoderFrame const* f)
            { return f->GetMemID() == id; }
        );
    }

    AV1DecoderFrame* AV1Decoder::GetFrameToDisplay()
    {
        return FindFrame(
            [](AV1DecoderFrame const* f)
            {
                FrameHeader const& h = f->GetFrameHeader();
                bool regularShowFrame = h.show_frame && !FrameInProgress(*f) && !f->Outputted();
                return regularShowFrame || f->ShowAsExisting();
            }
        );
    }

    AV1DecoderFrame* AV1Decoder::FindFrameByUID(int64_t uid)
    {
        return FindFrame(
            [uid](AV1DecoderFrame const* f)
        { return f->UID == uid; }
        );
    }

    AV1DecoderFrame* AV1Decoder::FindFrameInProgress()
    {
        return FindFrame(
            [](AV1DecoderFrame const* f)
        { return FrameInProgress(*f); }
        );
    }

    UMC::Status AV1Decoder::FillVideoParam(SequenceHeader const& sh, UMC_AV1_DECODER::AV1DecoderParams& par)
    {
        par.info.stream_type = UMC::AV1_VIDEO;
        par.info.profile = sh.seq_profile;

        par.info.clip_info = { int32_t(sh.max_frame_width), int32_t(sh.max_frame_height) };
        par.info.disp_clip_info = par.info.clip_info;

        if (!sh.color_config.subsampling_x && !sh.color_config.subsampling_y)
            par.info.color_format = UMC::YUV444;
        else if (sh.color_config.subsampling_x && !sh.color_config.subsampling_y)
            par.info.color_format = UMC::YUY2;
        else if (sh.color_config.subsampling_x && sh.color_config.subsampling_y)
            par.info.color_format = UMC::NV12;

        if (sh.color_config.BitDepth == 8 && par.info.color_format == UMC::YUV444)
            par.info.color_format = UMC::AYUV;
        if (sh.color_config.BitDepth == 10)
        {
            switch (par.info.color_format)
            {
            case UMC::NV12:   par.info.color_format = UMC::P010; break;
            case UMC::YUY2:   par.info.color_format = UMC::Y210; break;
            case UMC::YUV444: par.info.color_format = UMC::Y410; break;

            default:
                VM_ASSERT(!"Unknown subsampling");
                return UMC::UMC_ERR_UNSUPPORTED;
            }
        }
        else if (sh.color_config.BitDepth == 12)
        {
            switch (par.info.color_format)
            {
            case UMC::NV12:   par.info.color_format = UMC::P016; break;
            case UMC::YUY2:   par.info.color_format = UMC::Y216; break;
            case UMC::YUV444: par.info.color_format = UMC::Y416; break;

            default:
                VM_ASSERT(!"Unknown subsampling");
                return UMC::UMC_ERR_UNSUPPORTED;
            }
        }

        par.info.interlace_type = UMC::PROGRESSIVE;
        par.info.aspect_ratio_width = par.info.aspect_ratio_height = 1;

        par.lFlags = 0;

        par.film_grain = sh.film_grain_param_present;

        return UMC::UMC_OK;
    }

    void AV1Decoder::SetDPBSize(uint32_t size)
    {
        VM_ASSERT(size > 0);
        VM_ASSERT(size < 128);

        dpb.resize(size);
        std::generate(std::begin(dpb), std::end(dpb),
            [] { return new AV1DecoderFrame{}; }
        );
    }
    void AV1Decoder::SetRefSize(uint32_t size)
    {
        VM_ASSERT(size > 0);
        VM_ASSERT(size < 128);

        refs_temp.resize(size);
        std::generate(std::begin(refs_temp), std::end(refs_temp),
           [] { return new AV1DecoderFrame{}; }
        );
    }

    void AV1Decoder::CompleteDecodedFrames(FrameHeader const& fh, AV1DecoderFrame* pCurrFrame, AV1DecoderFrame* pPrevFrame)
    {
        if (pPrevFrame && Curr)
        {
            FrameHeader const& FH_OutTemp = Curr->GetFrameHeader();
            if (Poutput->UID == -1)
            {
               Poutput = pPrevFrame;
            }
            else
            {
                if (Repeat_show || FH_OutTemp.show_frame)
                {
                    Poutput ->DecrementReference();
                    Poutput = Curr;
                }
                else
                {
                    Curr->DecrementReference();
                }
            }
        }

        Curr = pCurrFrame;
        if (fh.show_existing_frame)
        {
            Repeat_show = 1;
        }
        else
        {
            Repeat_show = 0;
        }
    }

    AV1DecoderFrame* AV1Decoder::GetFreeFrame()
    {
        std::unique_lock<std::mutex> l(guard);

        auto i = std::find_if(std::begin(dpb), std::end(dpb),
            [](AV1DecoderFrame const* frame)
            { return frame->Empty(); }
        );

        AV1DecoderFrame* frame =
            i != std::end(dpb) ? *i : nullptr;

        if (frame)
            frame->UID = counter++;

        return frame;
    }

    AV1DecoderFrame* AV1Decoder::GetFrameBuffer(FrameHeader const& fh)
    {
        AV1DecoderFrame* frame = GetFreeFrame();
        if (!frame)
        {
           return nullptr;
        }

        frame->Reset(&fh);

        // increase ref counter when we get empty frame from DPB
        frame->IncrementReference();

        return frame;
    }

    void AV1Decoder::AddFrameData(AV1DecoderFrame& frame)
    {
        FrameHeader const& fh = frame.GetFrameHeader();

        if (fh.show_existing_frame)
            throw av1_exception(UMC::UMC_ERR_NOT_IMPLEMENTED);

        if (!allocator)
            throw av1_exception(UMC::UMC_ERR_NOT_INITIALIZED);

        UMC::VideoDataInfo info{};
        UMC::Status sts = info.Init(params.info.clip_info.width, params.info.clip_info.height, params.info.color_format, 0);
        if (sts != UMC::UMC_OK)
            throw av1_exception(sts);

        UMC::FrameMemID id;
        sts = allocator->Alloc(&id, &info, 0);
        if (sts != UMC::UMC_OK)
            throw av1_exception(UMC::UMC_ERR_ALLOC);

        AllocateFrameData(info, id, frame);
    }

    template <typename F>
    AV1DecoderFrame* AV1Decoder::FindFrame(F pred)
    {
        std::unique_lock<std::mutex> l(guard);

        auto i = std::find_if(std::begin(dpb), std::end(dpb),pred);
        return
            i != std::end(dpb) ? (*i) : nullptr;
    }
}

#endif //MFX_ENABLE_AV1_VIDEO_DECODE
