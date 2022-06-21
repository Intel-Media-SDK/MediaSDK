// Copyright (c) 2020 Intel Corporation
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
#ifdef MFX_ENABLE_H265_VIDEO_DECODE

#ifndef __UMC_H265_BITSTREAM_HEADERS_H_
#define __UMC_H265_BITSTREAM_HEADERS_H_

#include "umc_structures.h"
#include "umc_h265_dec_defs.h"

// Read N bits from 32-bit array
#define GetNBits(current_data, offset, nbits, data) \
{ \
    uint32_t x; \
 \
    VM_ASSERT((nbits) > 0 && (nbits) <= 32); \
    VM_ASSERT(offset >= 0 && offset <= 31); \
 \
    offset -= (nbits); \
 \
    if (offset >= 0) \
    { \
        x = current_data[0] >> (offset + 1); \
    } \
    else \
    { \
        offset += 32; \
 \
        x = current_data[1] >> (offset); \
        x >>= 1; \
        x += current_data[0] << (31 - offset); \
        current_data++; \
    } \
 \
    VM_ASSERT(offset >= 0 && offset <= 31); \
 \
    (data) = x & bits_data[nbits]; \
}

// Return bitstream position pointers N bits back
#define UngetNBits(current_data, offset, nbits) \
{ \
    VM_ASSERT(offset >= 0 && offset <= 31); \
 \
    offset += (nbits); \
    if (offset > 31) \
    { \
        offset -= 32; \
        current_data--; \
    } \
 \
    VM_ASSERT(offset >= 0 && offset <= 31); \
}

// Skip N bits in 32-bit array
#define SkipNBits(current_data, offset, nbits) \
{ \
    /* check error(s) */ \
    VM_ASSERT((nbits) > 0 && (nbits) <= 32); \
    VM_ASSERT(offset >= 0 && offset <= 31); \
    /* decrease number of available bits */ \
    offset -= (nbits); \
    /* normalize bitstream pointer */ \
    if (0 > offset) \
    { \
        offset += 32; \
        current_data++; \
    } \
    /* check error(s) again */ \
    VM_ASSERT(offset >= 0 && offset <= 31); \
 }

// Read 1 bit from 32-bit array
#define GetBits1(current_data, offset, data) \
{ \
    data = ((current_data[0] >> (offset)) & 1);  \
    offset -= 1; \
    if (offset < 0) \
    { \
        offset = 31; \
        current_data += 1; \
    } \
}

// Align bitstream position to byte boundary
#define ippiAlignBSPointerRight(current_data, offset) \
{ \
    if ((offset & 0x07) != 0x07) \
    { \
        offset = (offset | 0x07) - 8; \
        if (offset == -1) \
        { \
            offset = 31; \
            current_data++; \
        } \
    } \
}

// Read N bits from 32-bit array
#define PeakNextBits(current_data, bp, nbits, data) \
{ \
    uint32_t x; \
 \
    VM_ASSERT((nbits) > 0 && (nbits) <= 32); \
    VM_ASSERT(nbits >= 0 && nbits <= 31); \
 \
    int32_t offset = bp - (nbits); \
 \
    if (offset >= 0) \
    { \
        x = current_data[0] >> (offset + 1); \
    } \
    else \
    { \
        offset += 32; \
 \
        x = current_data[1] >> (offset); \
        x >>= 1; \
        x += current_data[0] << (31 - offset); \
    } \
 \
    VM_ASSERT(offset >= 0 && offset <= 31); \
 \
    (data) = x & bits_data[nbits]; \
}

namespace UMC_HEVC_DECODER
{
// Bit masks for fast extraction of bits from bitstream
const uint32_t bits_data[33] =
{
    (((uint32_t)0x01 << (0)) - 1),
    (((uint32_t)0x01 << (1)) - 1),
    (((uint32_t)0x01 << (2)) - 1),
    (((uint32_t)0x01 << (3)) - 1),
    (((uint32_t)0x01 << (4)) - 1),
    (((uint32_t)0x01 << (5)) - 1),
    (((uint32_t)0x01 << (6)) - 1),
    (((uint32_t)0x01 << (7)) - 1),
    (((uint32_t)0x01 << (8)) - 1),
    (((uint32_t)0x01 << (9)) - 1),
    (((uint32_t)0x01 << (10)) - 1),
    (((uint32_t)0x01 << (11)) - 1),
    (((uint32_t)0x01 << (12)) - 1),
    (((uint32_t)0x01 << (13)) - 1),
    (((uint32_t)0x01 << (14)) - 1),
    (((uint32_t)0x01 << (15)) - 1),
    (((uint32_t)0x01 << (16)) - 1),
    (((uint32_t)0x01 << (17)) - 1),
    (((uint32_t)0x01 << (18)) - 1),
    (((uint32_t)0x01 << (19)) - 1),
    (((uint32_t)0x01 << (20)) - 1),
    (((uint32_t)0x01 << (21)) - 1),
    (((uint32_t)0x01 << (22)) - 1),
    (((uint32_t)0x01 << (23)) - 1),
    (((uint32_t)0x01 << (24)) - 1),
    (((uint32_t)0x01 << (25)) - 1),
    (((uint32_t)0x01 << (26)) - 1),
    (((uint32_t)0x01 << (27)) - 1),
    (((uint32_t)0x01 << (28)) - 1),
    (((uint32_t)0x01 << (29)) - 1),
    (((uint32_t)0x01 << (30)) - 1),
    (((uint32_t)0x01 << (31)) - 1),
    ((uint32_t)0xFFFFFFFF),
};


template <typename T> class HeaderSet;
class Headers;

// Bitstream low level parsing class
class H265BaseBitstream
{
public:

    H265BaseBitstream();
    H265BaseBitstream(uint8_t * const pb, const uint32_t maxsize);
    virtual ~H265BaseBitstream();

    // Reset the bitstream with new data pointer
    void Reset(uint8_t * const pb, const uint32_t maxsize);
    // Reset the bitstream with new data pointer and bit offset
    void Reset(uint8_t * const pb, int32_t offset, const uint32_t maxsize);

    // Align bitstream position to byte boundary
    inline void AlignPointerRight(void);

    // Read N bits from bitstream array
    inline uint32_t GetBits(uint32_t nbits);

    // Read N bits from bitstream array
    template <uint32_t nbits>
    inline uint32_t GetPredefinedBits();


    // Read variable length coded unsigned element
    uint32_t GetVLCElementU();

    // Read variable length coded signed element
    int32_t GetVLCElementS();

    // Reads one bit from the buffer.
    uint8_t Get1Bit();

    // Check that position in bitstream didn't move outside the limit
    bool CheckBSLeft();

    // Check whether more data is present
    bool More_RBSP_Data();

    // Returns number of decoded bytes since last reset
    size_t BytesDecoded() const;

    // Returns number of decoded bits since last reset
    size_t BitsDecoded() const;

    // Returns number of bytes left in bitstream array
    size_t BytesLeft() const;

    // Throw exception if left bits not enough to decode
    void CheckBitsLeft(const uint32_t nbits);

    // Returns number of bits needed for byte alignment
    unsigned getNumBitsUntilByteAligned() const;

    // Align bitstream to byte boundary
    void readOutTrailingBits();

    // Returns bitstream current buffer pointer
    const uint8_t *GetRawDataPtr() const    {
        return (const uint8_t *)m_pbs + (31 - m_bitOffset)/8;
    }

    // Return bitstream array base address and size
    void GetOrg(uint32_t **pbs, uint32_t *size) const;
    // Return current bitstream address and bit offset
    void GetState(uint32_t **pbs, uint32_t *bitOffset);
    // Set current bitstream address and bit offset
    void SetState(uint32_t *pbs, uint32_t bitOffset);

    // Set current decoding position
    void SetDecodedBytes(size_t);

    // Set dummy buffer size in bytes
    void SetTailBsSize(const uint32_t nBytes);

    size_t GetAllBitsCount()
    {
        return m_maxBsSize;
    }

    size_t BytesDecodedRoundOff()
    {
        return static_cast<size_t>((uint8_t*)m_pbs - (uint8_t*)m_pbsBase);
    }

protected:

    uint32_t *m_pbs;                                              // (uint32_t *) pointer to the current position of the buffer.
    int32_t m_bitOffset;                                          // (int32_t) the bit position (0 to 31) in the dword pointed by m_pbs.
    uint32_t *m_pbsBase;                                          // (uint32_t *) pointer to the first byte of the buffer.
    uint32_t m_maxBsSize;                                         // (uint32_t) maximum buffer size in bytes.
    uint32_t m_tailBsSize;                                        // (uint32_t) dummy buffer size in bytes at the buffer end.
};

class H265ScalingList;
class H265VideoParamSet;
struct H265SeqParamSet;
class H265Slice;

// Bitstream headers parsing class
class H265HeadersBitstream : public H265BaseBitstream
{
public:

    H265HeadersBitstream();
    H265HeadersBitstream(uint8_t * const pb, const uint32_t maxsize);

    // Read and return NAL unit type and NAL storage idc.
    // Bitstream position is expected to be at the start of a NAL unit.
    UMC::Status GetNALUnitType(NalUnitType &nal_unit_type, uint32_t &nuh_temporal_id);
    // Read optional access unit delimiter from bitstream.
    UMC::Status GetAccessUnitDelimiter(uint32_t &PicCodType);

    // Parse SEI message
    int32_t ParseSEI(const HeaderSet<H265SeqParamSet> & sps, int32_t current_sps, H265SEIPayLoad *spl);

    // Parse remaining of slice header after GetSliceHeaderPart1
    void decodeSlice(H265Slice *, const H265SeqParamSet *, const H265PicParamSet *, PocDecoding *);
    // Parse slice header part which contains PPS ID
    UMC::Status GetSliceHeaderPart1(H265SliceHeader * sliceHdr);
    // Parse full slice header
    UMC::Status GetSliceHeaderFull(H265Slice *, const H265SeqParamSet *, const H265PicParamSet *, PocDecoding *);

    // Parse scaling list information in SPS or PPS
    void parseScalingList(H265ScalingList *);
    // Reserved for future header extensions
    bool MoreRbspData();

    // Part VPS header
    UMC::Status GetVideoParamSet(H265VideoParamSet *vps);

    // Parse SPS header
    UMC::Status GetSequenceParamSet(H265SeqParamSet *sps);

    // Parse PPS header
    void GetPictureParamSetPart1(H265PicParamSet *pps);
    UMC::Status GetPictureParamSetFull(H265PicParamSet  *pps, H265SeqParamSet const*);
    UMC::Status GetWPPTileInfo(H265SliceHeader *hdr,
                            const H265PicParamSet *pps,
                            const H265SeqParamSet *sps);

    void parseShortTermRefPicSet(const H265SeqParamSet* sps, ReferencePictureSet* pRPS, uint32_t idx);

protected:

    // Parse video usability information block in SPS
    void parseVUI(H265SeqParamSet *sps);

    // Parse weighted prediction table in slice header
    void xParsePredWeightTable(const H265SeqParamSet *sps, H265SliceHeader * sliceHdr);
    // Parse scaling list data block
    void xDecodeScalingList(H265ScalingList *scalingList, unsigned sizeId, unsigned listId);
    // Parse HRD information in VPS or in VUI block of SPS
    void parseHrdParameters(H265HRD *hrd, uint8_t commonInfPresentFlag, uint32_t vps_max_sub_layers);

    // Parse profile tier layers header part in VPS or SPS
    void  parsePTL(H265ProfileTierLevel *rpcPTL, int maxNumSubLayersMinus1);
    // Parse one profile tier layer
    void  parseProfileTier(H265PTL *ptl);

    // Decoding SEI message functions
    int32_t sei_message(const HeaderSet<H265SeqParamSet> & sps,int32_t current_sps,H265SEIPayLoad *spl);
    // Parse SEI payload data
    int32_t sei_payload(const HeaderSet<H265SeqParamSet> & sps, int32_t current_sps, H265SEIPayLoad *spl);
    // Parse pic timing SEI data
    int32_t pic_timing(const HeaderSet<H265SeqParamSet> & sps, int32_t current_sps, H265SEIPayLoad *spl);
    // Parse recovery point SEI data
    int32_t recovery_point(const HeaderSet<H265SeqParamSet> & sps, int32_t current_sps, H265SEIPayLoad *spl);
    // Parse mastering display colour volume SEI data
    int32_t mastering_display_colour_volume(const HeaderSet<H265SeqParamSet>& sps, int32_t current_sps, H265SEIPayLoad* spl);
    // Parse content light level info SEI data
    int32_t content_light_level_info(const HeaderSet<H265SeqParamSet>& sps, int32_t current_sps, H265SEIPayLoad* spl);

    // Skip unrecognized SEI message payload
    int32_t reserved_sei_message(const HeaderSet<H265SeqParamSet> & sps, int32_t current_sps, H265SEIPayLoad *spl);
};


// Read N bits from bitstream array
inline
uint32_t H265BaseBitstream::GetBits(const uint32_t nbits)
{
    uint32_t w, n = nbits;

    CheckBitsLeft(n);
    GetNBits(m_pbs, m_bitOffset, n, w);
    return(w);
}

// Read N bits from bitstream array
template <uint32_t nbits>
inline uint32_t H265BaseBitstream::GetPredefinedBits()
{
    uint32_t w, n = nbits;

    CheckBitsLeft(n);
    GetNBits(m_pbs, m_bitOffset, n, w);
    return(w);
}

inline bool DecodeExpGolombOne_H265_1u32s (uint32_t **ppBitStream,
                                                      int32_t *pBitOffset,
                                                      int32_t *pDst,
                                                      int32_t remainingBits,
                                                      int32_t isSigned)
{
    uint32_t code;
    uint32_t info     = 0;
    int32_t length   = 1;            /* for first bit read above*/
    uint32_t thisChunksLength = 0;
    uint32_t sval;

    /* check error(s) */

    /* Fast check for element = 0 */
    if ((remainingBits < 1))
    {
        throw h265_exception(UMC::UMC_ERR_NOT_ENOUGH_DATA);
    }
    GetNBits((*ppBitStream), (*pBitOffset), 1, code);
    remainingBits -= 1;

    if (code)
    {
        *pDst = 0;
        return true;
    }

    if ((remainingBits < 8))
    {
        throw h265_exception(UMC::UMC_ERR_NOT_ENOUGH_DATA);
    }
    GetNBits((*ppBitStream), (*pBitOffset), 8, code);
    length += 8;
    remainingBits -= 8;

    /* find nonzero byte */
    while (code == 0 && 32 > length)
    {
        if ((remainingBits < 8))
        {
            throw h265_exception(UMC::UMC_ERR_NOT_ENOUGH_DATA);
        }
        GetNBits((*ppBitStream), (*pBitOffset), 8, code);
        length += 8;
        remainingBits -= 8;
    }

    /* find leading '1' */
    while ((code & 0x80) == 0 && 32 > thisChunksLength)
    {
        code <<= 1;
        thisChunksLength++;
    }
    length -= 8 - thisChunksLength;

    UngetNBits((*ppBitStream), (*pBitOffset),8 - (thisChunksLength + 1));
    remainingBits += 8 - (thisChunksLength + 1);

    /* skipping very long codes, let's assume what the code is corrupted */
    if (32 <= length || 32 <= thisChunksLength)
    {
        uint32_t dwords;
        length -= (*pBitOffset + 1);
        dwords = length/32;
        length -= (32*dwords);
        *ppBitStream += (dwords + 1);
        *pBitOffset = 31 - length;
        *pDst = 0;
        return false;
    }

    /* Get info portion of codeword */
    if (length)
    {
        if ((remainingBits < length))
        {
            throw h265_exception(UMC::UMC_ERR_NOT_ENOUGH_DATA);
        }
        GetNBits((*ppBitStream), (*pBitOffset),length, info);
        remainingBits -= length;
    }

    sval = ((1 << (length)) + (info) - 1);
    if (isSigned)
    {
        if (sval & 1)
            *pDst = (int32_t) ((sval + 1) >> 1);
        else
            *pDst = -((int32_t) (sval >> 1));
    }
    else
        *pDst = (int32_t) sval;

    return true;
}

// Read variable length coded unsigned element
inline uint32_t H265BaseBitstream::GetVLCElementU()
{
    int32_t sval = 0;
    int32_t remainingbits = (int32_t)((m_maxBsSize + m_tailBsSize) * 8) - (int32_t)BitsDecoded();

    bool res = DecodeExpGolombOne_H265_1u32s(&m_pbs, &m_bitOffset, &sval, remainingbits, false);

    if (!res)
        throw h265_exception(UMC::UMC_ERR_INVALID_STREAM);

    return (uint32_t)sval;
}

// Read variable length coded signed element
inline int32_t H265BaseBitstream::GetVLCElementS()
{
    int32_t sval = 0;
    int32_t remainingbits = (int32_t)((m_maxBsSize + m_tailBsSize) * 8) - (int32_t)BitsDecoded();

    bool res = DecodeExpGolombOne_H265_1u32s(&m_pbs, &m_bitOffset, &sval, remainingbits, true);

    if (!res)
        throw h265_exception(UMC::UMC_ERR_INVALID_STREAM);

    return sval;
}

// Read one bit
inline uint8_t H265BaseBitstream::Get1Bit()
{
    uint32_t w;

    CheckBitsLeft(1);
    GetBits1(m_pbs, m_bitOffset, w);
    return (uint8_t)w;

} // H265Bitstream::Get1Bit()

// Returns number of decoded bytes since last reset
inline size_t H265BaseBitstream::BytesDecoded() const
{
    return static_cast<size_t>((uint8_t*)m_pbs - (uint8_t*)m_pbsBase) +
            ((31 - m_bitOffset) >> 3);
}

// Returns number of decoded bits since last reset
inline size_t H265BaseBitstream::BitsDecoded() const
{
    return static_cast<size_t>((uint8_t*)m_pbs - (uint8_t*)m_pbsBase) * 8 +
        (31 - m_bitOffset);
}

// Returns number of bytes left in bitstream array
inline size_t H265BaseBitstream::BytesLeft() const
{
    return (int32_t)m_maxBsSize - (int32_t) BytesDecoded();
}

// Throw exception if left bits not enough to decode
inline void H265BaseBitstream::CheckBitsLeft(const uint32_t nbits)
{
    if ((int32_t)m_maxBsSize * 8 - BitsDecoded() < nbits)
        throw h265_exception(UMC::UMC_ERR_NOT_ENOUGH_DATA);
}

// Returns number of bits needed for byte alignment
inline unsigned H265BaseBitstream::getNumBitsUntilByteAligned() const
{
    return ((m_bitOffset + 1) % 8);
}

// Align bitstream to byte boundary
inline void H265BaseBitstream::readOutTrailingBits()
{
    Get1Bit();
    //VM_ASSERT(1 == uVal);

    uint32_t bits = getNumBitsUntilByteAligned();

    if (bits)
    {
        GetBits(bits);
        //VM_ASSERT(0 == uVal);
    }
}

// Align bitstream position to byte boundary
inline void H265BaseBitstream::AlignPointerRight(void)
{
    if ((m_bitOffset & 0x07) != 0x07)
    {
        m_bitOffset = (m_bitOffset | 0x07) - 8;
        if (m_bitOffset == -1)
        {
            m_bitOffset = 31;
            m_pbs++;
        } 
    }
}

inline void H265BaseBitstream::SetTailBsSize(const uint32_t nBytes)
{
    m_tailBsSize = nBytes;
}

} // namespace UMC_HEVC_DECODER


#endif // __UMC_H265_BITSTREAM_HEADERS_H_
#endif // MFX_ENABLE_H265_VIDEO_DECODE
