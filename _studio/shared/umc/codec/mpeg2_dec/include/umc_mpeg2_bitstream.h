// Copyright (c) 2018 Intel Corporation
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

namespace UMC_MPEG2_DECODER
{
    struct MPEG2SequenceHeader;
    struct MPEG2SequenceExtension;
    struct MPEG2SequenceDisplayExtension;
    struct MPEG2PictureHeader;
    struct MPEG2PictureCodingExtension;
    struct MPEG2QuantMatrix;
    class  MPEG2Slice;

    // Bitstream low level parsing class
    class MPEG2BaseBitstream
    {
    public:

        MPEG2BaseBitstream()
        {
            Reset(nullptr, 0);
        }

        MPEG2BaseBitstream(uint8_t * pb, uint32_t maxsize)
        {
            Reset(pb, maxsize);
        }

        virtual ~MPEG2BaseBitstream()
        {
        }

        // Reset the bitstream with new data pointer
        void Reset(uint8_t * pb, uint32_t maxsize)
        {
            Reset(pb, 0, maxsize);
        }

        // Reset the bitstream with new data pointer and bit offset
        void Reset(uint8_t * pb, int32_t offset, uint32_t maxsize)
        {
            m_pbs       = pb;
            m_pbsBase   = pb;
            m_bitOffset = offset;
            m_maxBsSize = maxsize;
        }

        // Reads one bit from the buffer.
        inline uint32_t GetBit()
        {
            if (m_pbs >= m_pbsBase + m_maxBsSize)
                throw mpeg2_exception(UMC::UMC_ERR_NOT_ENOUGH_DATA);

            uint32_t const bit = (*m_pbs >> (7 - m_bitOffset)) & 1;
            if (++m_bitOffset == 8)
            {
                ++m_pbs;
                m_bitOffset = 0;
            }

            return bit;
        }

        // Read N bits from bitstream array
        inline uint32_t GetBits(uint32_t nbits)
        {
            uint32_t bits = 0;
            for (; nbits > 0; --nbits)
            {
                bits <<= 1;
                bits |= GetBit();
            }

            return bits;
        }

        // Move bitstream position pointer to N bits forward or backward
        inline void Seek(int32_t nbits)
        {
            int32_t bitsOffset = m_bitOffset + nbits;
            m_pbs += bitsOffset >> 3;
            m_bitOffset = bitsOffset & 7;
        }

        // Check one bit without shifting byte offset
        inline uint32_t Check1Bit() const
        {
            if (m_pbs >= m_pbsBase + m_maxBsSize)
                throw mpeg2_exception(UMC::UMC_ERR_NOT_ENOUGH_DATA);

            uint32_t const bit = (*m_pbs >> (7 - m_bitOffset)) & 1;

            return bit;
        }

        // Return a number of bits left
        inline size_t BitsLeft() const
        {
            return static_cast<size_t> (m_maxBsSize*8 - BitsDecoded());
        }

        // Check that position in bitstream didn't move outside the limit
        inline bool CheckBSLeft() const
        {
            size_t bitsDecoded = BitsDecoded();
            return (bitsDecoded > m_maxBsSize*8);
        }

        // Returns number of decoded bits since last reset
        inline size_t BitsDecoded() const
        {
            return static_cast<size_t>(m_pbs - m_pbsBase) * 8 + m_bitOffset;
        }

        // Returns number of decoded bytes since last reset
        inline size_t BytesDecoded() const
        {
            return static_cast<size_t>(m_pbs - m_pbsBase) + (m_bitOffset / 8);
        }

        // Returns number of bytes left in bitstream array
        inline size_t BytesLeft() const
        {
            return (int32_t)m_maxBsSize - (int32_t) BytesDecoded();
        }

        // Return bitstream array base address and size
        void GetOrg(uint8_t * & pbs, uint32_t & size) const
        {
            pbs       = m_pbsBase;
            size      = m_maxBsSize;
        }

        size_t GetAllBitsCount()
        {
            return m_maxBsSize;
        }

    protected:

        uint8_t const * m_pbs; // pointer to the current position of the buffer
        int32_t  m_bitOffset;  // the bit position (0 to 7) in the char pointed by m_pbs
        uint8_t* m_pbsBase;    // pointer to the first byte of the buffer
        uint32_t m_maxBsSize;  // maximum buffer size in bytes
    };

    // MPEG2 bitstream headers parsing class
    class MPEG2HeadersBitstream : public MPEG2BaseBitstream
    {
    public:

        MPEG2HeadersBitstream()
            : MPEG2BaseBitstream()
        {
        }

        MPEG2HeadersBitstream(uint8_t * const pb, const uint32_t maxsize)
            : MPEG2BaseBitstream(pb, maxsize)
        {
        }

        // Parse sequence header
        void GetSequenceHeader(MPEG2SequenceHeader&);

        // Parse sequence extension
        void GetSequenceExtension(MPEG2SequenceExtension&);

        // Parse sequence display extension
        void GetSequenceDisplayExtension(MPEG2SequenceDisplayExtension&);

        // Parse picture header
        void GetPictureHeader(MPEG2PictureHeader&);

        // Parse picture extension
        void GetPictureExtensionHeader(MPEG2PictureCodingExtension&);

        // Parse quant matrix extension
        void GetQuantMatrix(MPEG2QuantMatrix&);

        // Parse quant matrix extension
        void GetGroupOfPicturesHeader(MPEG2GroupOfPictures&);

        // Get macroblock address increment
        void DecodeMBAddress(MPEG2SliceHeader&);

        // Parse slice header
        UMC::Status GetSliceHeader(MPEG2SliceHeader&, const MPEG2SequenceHeader&, const MPEG2SequenceExtension&);
        // Parse full slice header
        UMC::Status GetSliceHeaderFull(MPEG2Slice&, const MPEG2SequenceHeader&, const MPEG2SequenceExtension&);
    };
}

#endif // MFX_ENABLE_MPEG2_VIDEO_DECODE
