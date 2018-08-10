// Copyright (c) 2017 Intel Corporation
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
#ifdef MFX_ENABLE_VP9_VIDEO_DECODE

#include "vm_debug.h"

#include "umc_structures.h"
#include "umc_vp9_bitstream.h"
#include "umc_vp9_frame.h"

namespace UMC_VP9_DECODER
{

VP9Bitstream::VP9Bitstream()
{
    Reset(0, 0);
}

VP9Bitstream::VP9Bitstream(uint8_t * const pb, const uint32_t maxsize)
{
    Reset(pb, maxsize);
}

// Reset the bitstream with new data pointer
void VP9Bitstream::Reset(uint8_t * const pb, const uint32_t maxsize)
{
    Reset(pb, 0, maxsize);
}

// Reset the bitstream with new data pointer and bit offset
void VP9Bitstream::Reset(uint8_t * const pb, int32_t offset, const uint32_t maxsize)
{
    m_pbs       = pb;
    m_pbsBase   = pb;
    m_bitOffset = offset;
    m_maxBsSize = maxsize;

}

// Return bitstream array base address and size
void VP9Bitstream::GetOrg(uint8_t **pbs, uint32_t *size)
{
    *pbs       = m_pbsBase;
    *size      = m_maxBsSize; 
}

uint32_t VP9Bitstream::GetBits(uint32_t nbits)
{
    mfxU32 bits = 0;
    for (; nbits > 0; --nbits)
    {
        bits <<= 1;
        bits |= GetBit();
    }

    return bits;
}

uint32_t VP9Bitstream::GetUe()
{
    uint32_t zeroes = 0;
    while (GetBit() == 0)
        ++zeroes;

    return zeroes == 0 ?
        0 : ((1 << zeroes) | GetBits(zeroes)) - 1; 
}

int32_t VP9Bitstream::GetSe()
{
    int32_t val = GetUe();
    uint32_t sign = (val & 1);
    val = (val + 1) >> 1;

    return
        sign ? val : -int32_t(val); 
}

void GetFrameSize(VP9Bitstream* bs, VP9DecoderFrame* frame)
{
    frame->width = bs->GetBits(16) + 1;
    frame->height = bs->GetBits(16) + 1;

    GetDisplaySize(bs, frame);
}

void GetDisplaySize(VP9Bitstream* bs, VP9DecoderFrame* frame)
{
    frame->displayWidth = frame->width;
    frame->displayHeight = frame->height;

    if (bs->GetBit())
    {
        frame->displayWidth = bs->GetBits(16) + 1;
        frame->displayHeight = bs->GetBits(16) + 1;
    }
}

void GetBitDepthAndColorSpace(VP9Bitstream* bs, VP9DecoderFrame* frame)
{
    if (frame->profile >= 2)
        frame->bit_depth = bs->GetBit() ? 12 : 10;
    else
        frame->bit_depth = 8;

    if (frame->frameType != KEY_FRAME && frame->intraOnly && frame->profile == 0)
    {
        frame->subsamplingY = frame->subsamplingX = 1;
        return;
    }

    uint32_t const colorspace
        = bs->GetBits(3);

    if (colorspace != SRGB)
    {
        bs->GetBit(); // 0: [16, 235] (i.e. xvYCC), 1: [0, 255]
        if (1 == frame->profile || 3 == frame->profile)
        {
            frame->subsamplingX = bs->GetBit();
            frame->subsamplingY = bs->GetBit();
            bs->GetBit(); // reserved bit
        }
        else
            frame->subsamplingY = frame->subsamplingX = 1;
    }
    else
    {
        if (1 == frame->profile || 3 == frame->profile)
        {
            frame->subsamplingX = 0;
            frame->subsamplingY = 0;
            bs->GetBit(); // reserved bit
        }
        else
            throw vp9_exception(UMC::UMC_ERR_INVALID_STREAM);
    }
}

void GetFrameSizeWithRefs(VP9Bitstream* bs, VP9DecoderFrame* frame)
{
    bool bFound = false;
    for (uint8_t i = 0; i < REFS_PER_FRAME; ++i)
    {
        if (bs->GetBit())
        {
            bFound = true;
            frame->width = frame->sizesOfRefFrame[frame->activeRefIdx[i]].width;
            frame->height = frame->sizesOfRefFrame[frame->activeRefIdx[i]].height;
            break;
        }
    }

    if (!bFound)
    {
        frame->width = bs->GetBits(16) + 1;
        frame->height = bs->GetBits(16) + 1;
    }

    GetDisplaySize(bs, frame);
}

void SetupLoopFilter(VP9Bitstream* bs, Loopfilter* filter)
{
    filter->filterLevel = bs->GetBits(6);
    filter->sharpnessLevel = bs->GetBits(3);

    filter->modeRefDeltaUpdate = 0;

    mfxI8 value = 0;

    filter->modeRefDeltaEnabled = (uint8_t)bs->GetBit();
    if (filter->modeRefDeltaEnabled)
    {
        filter->modeRefDeltaUpdate = (uint8_t)bs->GetBit();

        if (filter->modeRefDeltaUpdate)
        {
            for (uint32_t i = 0; i < MAX_REF_LF_DELTAS; i++)
            {
                if (bs->GetBit())
                {
                    value = (int8_t)bs->GetBits(6);
                    filter->refDeltas[i] = bs->GetBit() ? -value : value;
                }
            }

            for (uint32_t i = 0; i < MAX_MODE_LF_DELTAS; i++)
            {
                if (bs->GetBit())
                {
                    value = (int8_t)bs->GetBits(6);
                    filter->modeDeltas[i] = bs->GetBit() ? -value : value;
                }
            }
        }
    }
}

} // namespace UMC_VP9_DECODER
#endif // MFX_ENABLE_VP9_VIDEO_DECODE
