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

#include "umc_av1_dec_defs.h"
#include "umc_av1_frame.h"
#include "umc_av1_utils.h"
#include <limits>
#include <algorithm>

#include "umc_frame_data.h"
#include "umc_media_data.h"

namespace UMC_AV1_DECODER
{
    TileSet::TileSet(UMC::MediaData* in, TileLayout const& layout_)
        : layout(layout_)
    {
        if (!in)
            throw av1_exception(UMC::UMC_ERR_NULL_PTR);

        TileLocation const& lastTile = layout.back();
        size_t const size = lastTile.offset + lastTile.size;
        source.Alloc(size);
        if (source.GetBufferSize() < size)
            throw av1_exception(UMC::UMC_ERR_ALLOC);

        const uint8_t *src = reinterpret_cast <const uint8_t*> (in->GetDataPointer());
        uint8_t *dst = reinterpret_cast <uint8_t*> (source.GetDataPointer());
        std::copy(src, src + size, dst);

        source.SetDataSize(size);
    }

    size_t TileSet::Submit(uint8_t* bsBuffer, size_t spaceInBuffer, size_t offsetInBuffer, TileLayout& layoutWithOffset)
    {
        if (submitted)
            return 0;

        if (!bsBuffer)
            throw av1_exception(UMC::UMC_ERR_NULL_PTR);

        uint8_t* data = static_cast<uint8_t*>(source.GetDataPointer());
        const size_t length = std::min<size_t>(source.GetDataSize(), spaceInBuffer);

        MFX_INTERNAL_CPY(bsBuffer + offsetInBuffer, data, length);
        source.Close();

        for (auto& loc : layout)
        {
            layoutWithOffset.emplace_back(loc);
            layoutWithOffset.back().offset += offsetInBuffer;
        }

        submitted = true;

        return length;
    }

    AV1DecoderFrame::AV1DecoderFrame()
        : seq_header(new SequenceHeader{})
        , header(new FrameHeader{})
    {
        Reset();
    }

    AV1DecoderFrame::~AV1DecoderFrame()
    {
        //VM_ASSERT(Empty());
    }

    void AV1DecoderFrame::Reset()
    {
        error = 0;
        displayed = false;
        outputted = false;
        decoded   = false;
        decoding_started = false;
        decoding_completed = false;
        show_as_existing = false;
        data[SURFACE_DISPLAY].reset();
        data[SURFACE_RECON].reset();
        tile_sets.resize(0);

        *seq_header.get() = SequenceHeader{};
        *header.get() = FrameHeader{};
        header->display_frame_id = (std::numeric_limits<uint32_t>::max)();

        ResetRefCounter();
        FreeReferenceFrames();
        frame_dpb.clear();

        SetRefValid(false);

        film_grain_disabled = false;

        UID = -1;

        frame_time = -1;
        frame_order = 0;
    }

    void AV1DecoderFrame::Reset(FrameHeader const* fh)
    {
        if (!fh)
            throw av1_exception(UMC::UMC_ERR_NULL_PTR);

        int64_t id = UID;
        Reset();
        UID = id;

        *header = *fh;
    }

    void AV1DecoderFrame::AllocateAndLock(UMC::FrameData const* fd)
    {
        if (!fd)
            throw av1_exception(UMC::UMC_ERR_NULL_PTR);

        if (header->film_grain_params.apply_grain && !film_grain_disabled)
        {
            /* film grain is applied - two output surfaces required */

            VM_ASSERT(!data[SURFACE_DISPLAY].get() || !data[SURFACE_RECON].get());

            int surf = SURFACE_RECON;
            if (!data[SURFACE_DISPLAY].get())
                surf = SURFACE_DISPLAY;

            data[surf].reset(new UMC::FrameData{});
            *data[surf] = *fd;
        }
        else
        {
            /* film grain not applied - single output surface required
               both data[SURFACE_DISPLAY] and data[SURFACE_RECON] point to same FrameData */

            VM_ASSERT(!data[SURFACE_DISPLAY].get());
            VM_ASSERT(!data[SURFACE_RECON].get());

            data[SURFACE_DISPLAY].reset(new UMC::FrameData{});
            *data[SURFACE_DISPLAY] = *fd;
            data[SURFACE_RECON] = data[SURFACE_DISPLAY];
        }
    }

    void AV1DecoderFrame::AddSource(UMC::MediaData* in)
    {
        if (!in)
            throw av1_exception(UMC::UMC_ERR_NULL_PTR);

        size_t const size = in->GetDataSize();
        source->Alloc(size);
        if (source->GetBufferSize() < size)
            throw av1_exception(UMC::UMC_ERR_ALLOC);

        const uint8_t *src = reinterpret_cast <const uint8_t*> (in->GetDataPointer());
        uint8_t *dst = reinterpret_cast <uint8_t*> (source->GetDataPointer());
        std::copy(src, src + size, dst);
        source->SetDataSize(size);
    }

    void AV1DecoderFrame::AddTileSet(UMC::MediaData* in, TileLayout const& layout)
    {
        if (!in)
            throw av1_exception(UMC::UMC_ERR_NULL_PTR);

        tile_sets.emplace_back(in, layout);
    }

    void AV1DecoderFrame::SetSeqHeader(SequenceHeader const& sh)
    {
        *seq_header = sh;
    }

    bool AV1DecoderFrame::Empty() const
    {
        return !data[SURFACE_DISPLAY].get();
    }

    bool AV1DecoderFrame::Decoded() const
    {
        return decoded;
    }

    UMC::FrameMemID AV1DecoderFrame::GetMemID(int idx) const
    {
        if (!data[idx].get())
        {
            return -1;
        }
        return data[idx]->GetFrameMID();
    }

    void AV1DecoderFrame::AddReferenceFrame(AV1DecoderFrame * frm)
    {
        if (!frm || frm == this)
            return;

        if (std::find(references.begin(), references.end(), frm) != references.end())
            return;

        references.push_back(frm);
    }

    void AV1DecoderFrame::FreeReferenceFrames()
    {
        references.clear();
    }

    void AV1DecoderFrame::UpdateReferenceList()
    {
        if (header->frame_type == KEY_FRAME)
            return;

        for (uint8_t i = 0; i < INTER_REFS; ++i)
        {
            int32_t refIdx = header->ref_frame_idx[i];
            AddReferenceFrame(frame_dpb[refIdx]);
        }
    }

    void AV1DecoderFrame::OnDecodingCompleted()
    {
        DecrementReference();
        FreeReferenceFrames();
        decoded = true;
    }

    uint32_t AV1DecoderFrame::GetUpscaledWidth() const
    {
        return header->UpscaledWidth;
    }

    uint32_t AV1DecoderFrame::GetFrameHeight() const
    {
        return header->FrameHeight;
    }
}

#endif //MFX_ENABLE_AV1_VIDEO_DECODE
