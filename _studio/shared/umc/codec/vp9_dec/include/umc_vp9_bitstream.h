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

#pragma once

#include "umc_vp9_dec_defs.h"

#ifdef MFX_ENABLE_VP9_VIDEO_DECODE

#ifndef __UMC_VP9_BITSTREAM_H_
#define __UMC_VP9_BITSTREAM_H_

namespace UMC_VP9_DECODER
{
    class VP9DecoderFrame;
    struct Loopfilter;

    class VP9Bitstream
    {

    public:

        VP9Bitstream();
        VP9Bitstream(uint8_t * const pb, const uint32_t maxsize);

        // Reset the bitstream with new data pointer 
        void Reset(uint8_t * const pb, const uint32_t maxsize);
        // Reset the bitstream with new data pointer and bit offset
        void Reset(uint8_t * const pb, int32_t offset, const uint32_t maxsize); 
        
        // Returns number of decoded bytes since last reset
        size_t BytesDecoded() const
        {
            return
                static_cast<size_t>(m_pbs - m_pbsBase) + (m_bitOffset / 8); 
        }

        // Returns number of decoded bits since last reset
        size_t BitsDecoded() const
        {
            return
                static_cast<size_t>(m_pbs - m_pbsBase) * 8 + m_bitOffset;
        }

        // Returns number of bytes left in bitstream array
        size_t BytesLeft() const
        {
            return
                (int32_t)m_maxBsSize - (int32_t) BytesDecoded(); 
        }
 
        // Return bitstream array base address and size
        void GetOrg(uint8_t **pbs, uint32_t *size);

        uint32_t GetBit()
        {
            if (m_pbs >= m_pbsBase + m_maxBsSize)
                throw vp9_exception(UMC::UMC_ERR_NOT_ENOUGH_DATA);

            uint32_t const bit = (*m_pbs >> (7 - m_bitOffset)) & 1;
            if (++m_bitOffset == 8)
            {
                ++m_pbs;
                m_bitOffset = 0;
            }
            
            return bit;
        }

        uint32_t GetBits(uint32_t nbits);
        uint32_t GetUe();
        int32_t GetSe();

    protected:

        uint8_t* m_pbs;                                              // pointer to the current position of the buffer.
        int32_t m_bitOffset;                                        // the bit position (0 to 31) in the dword pointed by m_pbs.
        uint8_t* m_pbsBase;                                          // pointer to the first byte of the buffer.
        uint32_t m_maxBsSize;                                        // maximum buffer size in bytes. 
    };

    inline
    bool CheckSyncCode(VP9Bitstream* bs)
    {
        return 
            bs->GetBits(8) == VP9_SYNC_CODE_0 &&
            bs->GetBits(8) == VP9_SYNC_CODE_1 &&
            bs->GetBits(8) == VP9_SYNC_CODE_2
            ;
    }

    void GetDisplaySize(VP9Bitstream*, VP9DecoderFrame*);
    void GetFrameSize(VP9Bitstream*, VP9DecoderFrame*);
    void GetBitDepthAndColorSpace(VP9Bitstream*, VP9DecoderFrame*);
    void GetFrameSizeWithRefs(VP9Bitstream*, VP9DecoderFrame*);
    void SetupLoopFilter(VP9Bitstream *pBs, Loopfilter*);

} // namespace UMC_VP9_DECODER

#endif // __UMC_VP9_BITSTREAM_H_

#endif // MFX_ENABLE_VP9_VIDEO_DECODE
