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

#ifndef __UMC_AV1_FRAME_H__
#define __UMC_AV1_FRAME_H__

#include "umc_frame_allocator.h"
#include "umc_av1_dec_defs.h"
#include "mfx_common_decode_int.h"

#include <memory>

namespace UMC_AV1_DECODER
{
    struct SequenceHeader;
    struct FrameHeader;

    struct TileLocation
    {
        uint32_t startIdx; // index of 1st tile in current tile group
        uint32_t endIdx; // index of last tile in current tile group
        size_t offset; // offset in the buffer
        size_t size; // size of the tile
        uint32_t row; // row in tile grid
        uint32_t col; // column in tile grid
    };

    typedef std::vector<TileLocation> TileLayout;

    class TileSet
    {
    public:

        TileSet() {};
        TileSet(UMC::MediaData*, TileLayout const&);
        ~TileSet() {};

        size_t Submit(uint8_t*, size_t, size_t, TileLayout&);
        bool Submitted() const
        { return submitted; }
        uint32_t GetTileCount() const
        { return static_cast<uint32_t>(layout.size()); }
        size_t GetSize() const
        { return source.GetDataSize(); }

    private:
        UMC::MediaData source;
        TileLayout layout;
        bool submitted = false;
    };

    inline uint32_t CalcTilesInTileSets(std::vector<TileSet> const& tileSets)
    {
        uint32_t numTiles = 0;
        for (auto& tileSet : tileSets)
            numTiles += tileSet.GetTileCount();

        return numTiles;
    }

    inline size_t BytesReadyForSubmission(std::vector<TileSet> const& tileSets)
    {
        size_t bytes = 0;
        for (auto& tileSet : tileSets)
            bytes += tileSet.Submitted() ? tileSet.GetSize() : 0;

        return bytes;
    }

    inline size_t CalcSizeOfTileSets(std::vector<TileSet> const& tileSets)
    {
        size_t size = 0;
        for (auto& tileSet : tileSets)
            size += tileSet.GetSize();

        return size;
    }

    enum
    {
        SURFACE_DISPLAY = 0,
        SURFACE_RECON = 1
    };

    class AV1DecoderFrame : public RefCounter
    {

    public:

        AV1DecoderFrame();
        ~AV1DecoderFrame();

        void Reset();
        void Reset(FrameHeader const*);
        void AllocateAndLock(UMC::FrameData const*);

        void AddSource(UMC::MediaData*);

        UMC::MediaData* GetSource()
        { return source.get(); }

        void AddTileSet(UMC::MediaData* in, TileLayout const& layout);

        std::vector<TileSet> const& GetTileSets() const
        { return tile_sets; }

        std::vector<TileSet>& GetTileSets()
        { return tile_sets; }

        int32_t GetError() const
        { return error; }

        void SetError(int32_t e)
        { error = e; }

        void AddError(int32_t e)
        { error |= e; }

        void SetSeqHeader(SequenceHeader const&);

        SequenceHeader const& GetSeqHeader() const
        { return *seq_header; }

        FrameHeader& GetFrameHeader()
        { return *header; }
        FrameHeader const& GetFrameHeader() const
        { return *header; }

        bool Empty() const;
        bool Decoded() const;

        bool Displayed() const
        { return displayed; }
        void Displayed(bool d)
        { displayed = d; }

        bool Outputted() const
        { return outputted; }
        void Outputted(bool o)
        { outputted = o; }

        bool DecodingStarted() const
        { return decoding_started; }
        void StartDecoding()
        { decoding_started = true; }

        bool DecodingCompleted() const
        { return decoding_completed; }
        void CompleteDecoding()
        { decoding_completed = true; }

        bool ShowAsExisting() const
        { return show_as_existing; }
        void ShowAsExisting(bool show)
        { show_as_existing = show; }

        bool FilmGrainDisabled() const
        { return film_grain_disabled; }
        void DisableFilmGrain()
        { film_grain_disabled = true; }

        UMC::FrameData* GetFrameData(int idx = SURFACE_DISPLAY)
        { return data[idx].get(); }
        UMC::FrameData const* GetFrameData(int idx = SURFACE_DISPLAY) const
        { return data[idx].get(); }

        UMC::FrameMemID GetMemID(int idx = SURFACE_DISPLAY) const;

        void AddReferenceFrame(AV1DecoderFrame* frm);
        void FreeReferenceFrames();
        void UpdateReferenceList();
        void OnDecodingCompleted();

        void SetRefValid(bool valid)
        { ref_valid = valid; }
        bool RefValid() const
        { return ref_valid; };

        uint32_t GetUpscaledWidth() const;
        uint32_t GetFrameHeight() const;

        void SetFrameTime(mfxF64 time)
        { frame_time = time; };
        mfxF64 FrameTime() const
        { return frame_time; };
        void SetFrameOrder(mfxU16 order)
        { frame_order = order; };
        mfxU16 FrameOrder() const
        { return frame_order; };

    public:

        int64_t          UID;
        DPBType          frame_dpb;

    protected:
        virtual void Free()
        {
            Reset();
        }

    private:
        bool                              outputted; // set in [application thread] when frame is mapped to respective output mfxFrameSurface
        bool                              displayed; // set in [scheduler thread] when frame decoding is finished and
                                                     // respective mfxFrameSurface prepared for output to application
        bool                              decoded;   // set in [application thread] to signal that frame is completed and respective reference counter decremented
                                                     // after it frame still may remain in [AV1Decoder::dpb], but only as reference

        bool                              decoding_started;     // set in [application thread] right after frame submission to the driver started
        bool                              decoding_completed;   // set in [scheduler thread] after getting driver status report for the frame

        bool                              show_as_existing;


        std::shared_ptr<UMC::FrameData>   data[2]; // if FilmGrain is applied:     data[SURFACE_DISPLAY] points to data of frame with film grain, and data[SURFACE_RECON] points to data of reconstructed frame
                                                   // if FilmGrain is not applied: both data[SURFACE_DISPLAY] and data[SURFACE_RECON] point to data of reconstructed frame
        std::unique_ptr<UMC::MediaData>   source;

        std::vector<TileSet>              tile_sets;

        int32_t                           error;

        std::unique_ptr<SequenceHeader>   seq_header;
        std::unique_ptr<FrameHeader>      header;

        DPBType                           references;

        bool                              ref_valid;

        bool                              film_grain_disabled;

        mfxF64                            frame_time;
        mfxU16                            frame_order;
    };

} // end namespace UMC_AV1_DECODER

#endif // __UMC_AV1_FRAME_H__
#endif // MFX_ENABLE_AV1_VIDEO_DECODE
