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

#pragma once

#include "umc_av1_dec_defs.h"

#ifdef MFX_ENABLE_AV1_VIDEO_DECODE

#ifndef __UMC_AV1_BITSTREAM_H_
#define __UMC_AV1_BITSTREAM_H_

#include "umc_vp9_bitstream.h"

namespace UMC_AV1_DECODER
{
    class AV1DecoderFrame;

    class AV1Bitstream
        : public UMC_VP9_DECODER::VP9Bitstream
    {
    public:

        void ReadOBUInfo(OBUInfo&);
        void ReadTileGroupHeader(FrameHeader const&, TileGroupInfo&);
        void ReadTile(FrameHeader const&, size_t&, size_t&);
        void ReadByteAlignment();
        uint64_t GetLE(uint32_t);
        void ReadSequenceHeader(SequenceHeader&);
        void ReadUncompressedHeader(FrameHeader&, SequenceHeader const&, DPBType const&, OBUHeader const&, uint32_t&);

        using UMC_VP9_DECODER::VP9Bitstream::VP9Bitstream;

        uint8_t GetBit()
        {
            return static_cast<uint8_t>(UMC_VP9_DECODER::VP9Bitstream::GetBit());
        }
    };
}

#endif // __UMC_AV1_BITSTREAM_H_

#endif // MFX_ENABLE_AV1_VIDEO_DECODE
