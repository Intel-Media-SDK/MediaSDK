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
#include <mfx_utils.h>

#if defined (MFX_ENABLE_H264_VIDEO_DECODE)

#ifndef __UMC_H264_BITSTREAM_HEADERS_H_
#define __UMC_H264_BITSTREAM_HEADERS_H_
#include "umc_structures.h"
#include "umc_h264_dec_defs_dec.h"

#define h264GetBits(current_data, offset, nbits, data) \
{ \
    uint32_t x; \
    offset -= (nbits); \
    if (offset >= 0) \
    { \
        x = current_data[0] >> (offset + 1); \
    } \
    else \
    { \
        offset += 32; \
        x = current_data[1] >> (offset); \
        x >>= 1; \
        x += current_data[0] << (31 - offset); \
        current_data++; \
    } \
    (data) = x & bits_data[nbits]; \
}

#define h264UngetNBits(current_data, offset, nbits) \
{ \
    offset += (nbits); \
    if (offset > 31) \
    { \
        offset -= 32; \
        current_data--; \
    } \
}

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

namespace UMC
{
extern const uint32_t bits_data[];
extern const int32_t mp_scan4x4[2][16];
extern const int32_t hp_scan8x8[2][64];

extern const uint16_t SAspectRatio[17][2];

extern const uint8_t default_intra_scaling_list4x4[16];
extern const uint8_t default_inter_scaling_list4x4[16];
extern const uint8_t default_intra_scaling_list8x8[64];
extern const uint8_t default_inter_scaling_list8x8[64];

class Headers;

class H264BaseBitstream
{
public:

    H264BaseBitstream();
    H264BaseBitstream(uint8_t * const pb, const uint32_t maxsize);
    virtual ~H264BaseBitstream();

    // Reset the bitstream with new data pointer
    void Reset(uint8_t * const pb, const uint32_t maxsize);
    void Reset(uint8_t * const pb, int32_t offset, const uint32_t maxsize);

    inline uint32_t GetBits(const uint32_t nbits);

    // Read one VLC int32_t or uint32_t value from bitstream
    inline int32_t GetVLCElement(bool bIsSigned);

    // Reads one bit from the buffer.
    inline uint8_t Get1Bit();

    inline bool IsBSLeft(size_t sizeToRead = 0);
    inline void CheckBSLeft(size_t sizeToRead = 0);
    inline bool IsBitsLeft(size_t sizeToRead = 0);
    inline void CheckBitsLeft(size_t sizeToRead = 0);

    // Check amount of data
    bool More_RBSP_Data();

    inline size_t BytesDecoded();
    inline size_t BitsDecoded();
    inline size_t BytesLeft();

    // Align bitstream pointer to the right
    inline void AlignPointerRight();

    void GetOrg(uint32_t **pbs, uint32_t *size);
    void GetState(uint32_t **pbs, uint32_t *bitOffset);
    void SetState(uint32_t *pbs, uint32_t bitOffset);

    // Set current decoding position
    void SetDecodedBytes(size_t);

    // Set dummy buffer size in bytes
    void SetTailBsSize(const uint32_t nBytes);

    size_t BytesDecodedRoundOff()
    {
        return static_cast<size_t>((uint8_t*)m_pbs - (uint8_t*)m_pbsBase);
    }

protected:

    uint32_t *m_pbs;                                              // (uint32_t *) pointer to the current position of the buffer.
    int32_t m_bitOffset;                                         // (int32_t) the bit position (0 to 31) in the dword pointed by m_pbs.
    uint32_t *m_pbsBase;                                          // (uint32_t *) pointer to the first byte of the buffer.
    uint32_t m_maxBsSize;                                         // (uint32_t) maximum buffer size in bytes.
    uint32_t m_tailBsSize; // (uint32_t) dummy buffer size in bytes at the buffer end.
};

class H264HeadersBitstream : public H264BaseBitstream
{
public:

    H264HeadersBitstream();
    H264HeadersBitstream(uint8_t * const pb, const uint32_t maxsize);


    // Decode sequence parameter set
    Status GetSequenceParamSet(UMC_H264_DECODER::H264SeqParamSet *sps, bool ignoreLevelConstrain=false);
    Status GetSequenceParamSetSvcExt(UMC_H264_DECODER::H264SeqParamSetSVCExtension *pSPSSvcExt);
    Status GetSequenceParamSetSvcVuiExt(UMC_H264_DECODER::H264SeqParamSetSVCExtension *pSPSSvcExt);

    // Decode sequence parameter set extension
    Status GetSequenceParamSetExtension(UMC_H264_DECODER::H264SeqParamSetExtension *sps_ex);
    // Decode sequence param set MVC extension
    Status GetSequenceParamSetMvcExt(UMC_H264_DECODER::H264SeqParamSetMVCExtension *pSPSMvcExt);

    // Decoding picture's parameter set functions
    Status GetPictureParamSetPart1(UMC_H264_DECODER::H264PicParamSet *pps);
    Status GetPictureParamSetPart2(UMC_H264_DECODER::H264PicParamSet *pps, UMC_H264_DECODER::H264SeqParamSet const*);

    // Decode NAL unit prefix
    Status GetNalUnitPrefix(UMC_H264_DECODER::H264NalExtension *pExt, uint32_t NALRef_idc);

    // Decode NAL unit extension parameters
    Status GetNalUnitExtension(UMC_H264_DECODER::H264NalExtension *pExt);

    // Decoding slice header functions
    Status GetSliceHeaderPart1(UMC_H264_DECODER::H264SliceHeader *pSliceHeader);
    Status GetSliceHeaderPart2(UMC_H264_DECODER::H264SliceHeader *pSliceHeader,
                               const UMC_H264_DECODER::H264PicParamSet *pps,
                               const UMC_H264_DECODER::H264SeqParamSet *sps);
    Status GetSliceHeaderPart3(UMC_H264_DECODER::H264SliceHeader *pSliceHeader,
                               UMC_H264_DECODER::PredWeightTable *pPredWeight_L0,
                               UMC_H264_DECODER::PredWeightTable *pPredWeight_L1,
                               UMC_H264_DECODER::RefPicListReorderInfo *pReorderInfo_L0,
                               UMC_H264_DECODER::RefPicListReorderInfo *pReorderInfo_L1,
                               UMC_H264_DECODER::AdaptiveMarkingInfo *pAdaptiveMarkingInfo,
                               UMC_H264_DECODER::AdaptiveMarkingInfo *pBaseAdaptiveMarkingInfo,
                               const UMC_H264_DECODER::H264PicParamSet *pps,
                               const UMC_H264_DECODER::H264SeqParamSet *sps,
                               const UMC_H264_DECODER::H264SeqParamSetSVCExtension *spsSvcExt);
    Status GetSliceHeaderPart4(UMC_H264_DECODER::H264SliceHeader *hdr,
                                const UMC_H264_DECODER::H264SeqParamSetSVCExtension *spsSvcExt); // from slice header in
                                                                               // scalable extension NAL unit


    Status GetNALUnitType(NAL_Unit_Type &nal_unit_type, uint32_t &nal_ref_idc);
    // SEI part
    int32_t ParseSEI(const Headers & headers, UMC_H264_DECODER::H264SEIPayLoad *spl);
    int32_t sei_message(const Headers & headers, int32_t current_sps, UMC_H264_DECODER::H264SEIPayLoad *spl);
    int32_t sei_payload(const Headers & headers, int32_t current_sps, UMC_H264_DECODER::H264SEIPayLoad *spl);
    int32_t buffering_period(const Headers & headers, int32_t, UMC_H264_DECODER::H264SEIPayLoad *spl);
    int32_t pic_timing(const Headers & headers, int32_t current_sps, UMC_H264_DECODER::H264SEIPayLoad *spl);
    void user_data_registered_itu_t_t35(UMC_H264_DECODER::H264SEIPayLoad *spl);
    void recovery_point(UMC_H264_DECODER::H264SEIPayLoad *spl);
    int32_t dec_ref_pic_marking_repetition(const Headers & headers, int32_t current_sps, UMC_H264_DECODER::H264SEIPayLoad *spl);
    void unparsed_sei_message(UMC_H264_DECODER::H264SEIPayLoad *spl);
    void scalability_info(UMC_H264_DECODER::H264SEIPayLoad *spl);

protected:

    Status DecRefBasePicMarking(UMC_H264_DECODER::AdaptiveMarkingInfo *pAdaptiveMarkingInfo,
        uint8_t &adaptive_ref_pic_marking_mode_flag);

    Status DecRefPicMarking(UMC_H264_DECODER::H264SliceHeader *hdr, UMC_H264_DECODER::AdaptiveMarkingInfo *pAdaptiveMarkingInfo);

    void GetScalingList4x4(H264ScalingList4x4 *scl, uint8_t *def, uint8_t *scl_type);
    void GetScalingList8x8(H264ScalingList8x8 *scl, uint8_t *def, uint8_t *scl_type);

    Status GetVUIParam(UMC_H264_DECODER::H264SeqParamSet *sps, UMC_H264_DECODER::H264VUI *vui);
    Status GetHRDParam(UMC_H264_DECODER::H264SeqParamSet *sps, UMC_H264_DECODER::H264VUI *vui);

    Status GetPredWeightTable(UMC_H264_DECODER::H264SliceHeader *hdr, const UMC_H264_DECODER::H264SeqParamSet *sps,
        UMC_H264_DECODER::PredWeightTable *pPredWeight_L0, UMC_H264_DECODER::PredWeightTable *pPredWeight_L1);
};

void SetDefaultScalingLists(UMC_H264_DECODER::H264SeqParamSet * sps);

inline
void FillFlatScalingList4x4(H264ScalingList4x4 *scl)
{
    for (int32_t i=0;i<16;i++)
        scl->ScalingListCoeffs[i] = 16;
}

inline void FillFlatScalingList8x8(H264ScalingList8x8 *scl)
{
    for (int32_t i=0;i<64;i++)
        scl->ScalingListCoeffs[i] = 16;
}

inline void FillScalingList4x4(H264ScalingList4x4 *scl_dst, const uint8_t *coefs_src)
{
    for (int32_t i=0;i<16;i++)
        scl_dst->ScalingListCoeffs[i] = coefs_src[i];
}

inline void FillScalingList8x8(H264ScalingList8x8 *scl_dst, const uint8_t *coefs_src)
{
    for (int32_t i=0;i<64;i++)
        scl_dst->ScalingListCoeffs[i] = coefs_src[i];
}

inline bool DecodeExpGolombOne_H264_1u32s (uint32_t **ppBitStream,
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
        throw h264_exception(UMC::UMC_ERR_INVALID_STREAM);
    }
    h264GetBits((*ppBitStream), (*pBitOffset), 1, code);
    remainingBits -= 1;

    if (code)
    {
        *pDst = 0;
        return true;
    }

    if ((remainingBits < 8))
    {
        throw h264_exception(UMC::UMC_ERR_INVALID_STREAM);
    }
    h264GetBits((*ppBitStream), (*pBitOffset), 8, code);
    length += 8;
    remainingBits -= 8;

    /* find nonzero byte */
    while (code == 0 && 32 > length)
    {
        if ((remainingBits < 8))
        {
            throw h264_exception(UMC::UMC_ERR_INVALID_STREAM);
        }
        h264GetBits((*ppBitStream), (*pBitOffset), 8, code);
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

    h264UngetNBits((*ppBitStream), (*pBitOffset),8 - (thisChunksLength + 1));
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
            throw h264_exception(UMC::UMC_ERR_INVALID_STREAM);
        }
        h264GetBits((*ppBitStream), (*pBitOffset),length, info)
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

inline bool H264BaseBitstream::IsBSLeft(size_t sizeToRead)
{
    size_t bitsDecoded = BitsDecoded();
    return (bitsDecoded + sizeToRead  *8 <= m_maxBsSize * 8);
}

inline void H264BaseBitstream::CheckBSLeft(size_t sizeToRead)
{
    if (!IsBSLeft(sizeToRead))
        throw h264_exception(UMC_ERR_INVALID_STREAM);
}

inline bool H264BaseBitstream::IsBitsLeft(size_t sizeToRead)
{
    size_t bitsDecoded = BitsDecoded();
    return (bitsDecoded + sizeToRead <= m_maxBsSize * 8);
}

inline void H264BaseBitstream::CheckBitsLeft(size_t sizeToRead)
{
    if (!IsBitsLeft(sizeToRead))
        throw h264_exception(UMC_ERR_INVALID_STREAM);
}

inline uint32_t H264BaseBitstream::GetBits(const uint32_t nbits)
{
    uint32_t w, n = nbits;

    CheckBitsLeft(n);
    h264GetBits(m_pbs, m_bitOffset, n, w);
    return(w);
}

inline int32_t H264BaseBitstream::GetVLCElement(bool bIsSigned)
{
    int32_t sval = 0;
    int32_t remainingbits = (int32_t)((m_maxBsSize + m_tailBsSize) * 8) - (int32_t)BitsDecoded();

    bool res = DecodeExpGolombOne_H264_1u32s(&m_pbs, &m_bitOffset, &sval, remainingbits, bIsSigned);

    if (!res)
        throw h264_exception(UMC_ERR_INVALID_STREAM);
    return sval;
}

inline uint8_t H264BaseBitstream::Get1Bit()
{
    uint32_t w;

    CheckBitsLeft(1);
    GetBits1(m_pbs, m_bitOffset, w);
    return (uint8_t)w;
}

inline size_t H264BaseBitstream::BytesDecoded()
{
    return static_cast<size_t>((uint8_t*)m_pbs - (uint8_t*)m_pbsBase) +
            ((31 - m_bitOffset) >> 3);
}

inline size_t H264BaseBitstream::BitsDecoded()
{
    return static_cast<size_t>((uint8_t*)m_pbs - (uint8_t*)m_pbsBase) * 8 +
        (31 - m_bitOffset);
}

inline size_t H264BaseBitstream::BytesLeft()
{
    return((int32_t)m_maxBsSize - (int32_t) BytesDecoded());
}

inline void H264BaseBitstream::AlignPointerRight()
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

Status InitializePictureParamSet(UMC_H264_DECODER::H264PicParamSet *pps, const UMC_H264_DECODER::H264SeqParamSet *sps, bool isExtension);

} // namespace UMC

#endif // __UMC_H264_BITSTREAM_HEADERS_H_

#endif // MFX_ENABLE_H264_VIDEO_DECODE
