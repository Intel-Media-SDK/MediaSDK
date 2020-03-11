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
#ifdef MFX_ENABLE_H265_VIDEO_DECODE

#ifndef __UMC_H265_SLICE_DECODING_H
#define __UMC_H265_SLICE_DECODING_H

#include <list>
#include "umc_h265_dec_defs.h"
#include "umc_h265_bitstream_headers.h"
#include "umc_h265_heap.h"

namespace UMC_HEVC_DECODER
{
class H265DecoderFrame;
class H265DBPList;
class DecodingContext;

// Asynchronous task IDs
enum
{
    DEC_PROCESS_ID,
    REC_PROCESS_ID,
    DEB_PROCESS_ID,
    SAO_PROCESS_ID,

    LAST_PROCESS_ID
};

// Task completeness information structure
struct CUProcessInfo
{
    int32_t firstCU;
    int32_t maxCU;
    int32_t m_curCUToProcess[LAST_PROCESS_ID];
    int32_t m_processInProgress[LAST_PROCESS_ID];
    bool m_isCompleted;
    int32_t m_width;

    void Initialize(int32_t firstCUAddr, int32_t width)
    {
        for (int32_t task = 0; task < LAST_PROCESS_ID; task++)
        {
            m_curCUToProcess[task] = firstCUAddr;
            m_processInProgress[task] = 0;
        }

        firstCU = firstCUAddr;
        m_width = width;
        m_isCompleted = false;
    }
};

// Slice descriptor class
class H265Slice : public HeapObject
{
public:
    // Default constructor
    H265Slice();
    // Destructor
    virtual
    ~H265Slice(void);

    // Decode slice header and initializ slice structure with parsed values
    virtual bool Reset(PocDecoding * pocDecoding);
    // Set current slice number
    void SetSliceNumber(int32_t iSliceNumber);

    // Initialize CABAC context depending on slice type
    void InitializeContexts();

    // Parse beginning of slice header to get PPS ID
    virtual int32_t RetrievePicParamSetNumber();

    //
    // method(s) to obtain slice specific information
    //

    // Obtain pointer to slice header
    const H265SliceHeader *GetSliceHeader() const {return &m_SliceHeader;}
    H265SliceHeader *GetSliceHeader() {return &m_SliceHeader;}
    int32_t GetFirstMB() const {return m_iFirstMB;}
    // Obtain current picture parameter set
    const H265PicParamSet *GetPicParam() const {return m_pPicParamSet;}
    void SetPicParam(const H265PicParamSet * pps)
    {
        m_pPicParamSet = pps;
        if (m_pPicParamSet)
            m_pPicParamSet->IncrementReference();
    }
    // Obtain current sequence parameter set
    const H265SeqParamSet *GetSeqParam(void) const {return m_pSeqParamSet;}
    void SetSeqParam(const H265SeqParamSet * sps)
    {
        m_pSeqParamSet = sps;
        if (m_pSeqParamSet)
            m_pSeqParamSet->IncrementReference();
    }

    // Obtain current destination frame
    H265DecoderFrame *GetCurrentFrame(void) const {return m_pCurrentFrame;}
    void SetCurrentFrame(H265DecoderFrame * pFrame){m_pCurrentFrame = pFrame;}

    // Obtain slice number
    int32_t GetSliceNum(void) const {return m_iNumber;}
    // Obtain maximum of macroblock
    int32_t GetMaxMB(void) const {return m_iMaxMB;}
    void SetMaxMB(int32_t x) {m_iMaxMB = x;}

    // Build reference lists from slice reference pic set. HEVC spec 8.3.2
    virtual UMC::Status UpdateReferenceList(H265DBPList *dpb, H265DecoderFrame* curr_ref);

    bool IsError() const {return m_bError;}

    bool GetRapPicFlag() const;

public:

    MemoryPiece m_source;                                 // (MemoryPiece *) pointer to owning memory piece

public:  // DEBUG !!!! should remove dependence

    // Initialize slice structure to default values
    virtual void Reset();

    // Release resources
    virtual void Release();

    // Decoder slice header and calculate POC
    virtual bool DecodeSliceHeader(PocDecoding * pocDecoding);

    H265SliceHeader m_SliceHeader;                              // (H265SliceHeader) slice header

    H265HeadersBitstream m_BitStream;                                  // (H265Bitstream) slice bit stream

    // Obtain bit stream object
    H265HeadersBitstream* GetBitStream()
    { return &m_BitStream; }
    H265HeadersBitstream const* GetBitStream() const
    { return &m_BitStream; }

protected:
    const H265PicParamSet* m_pPicParamSet;                      // (H265PicParamSet *) pointer to array of picture parameters sets
    const H265SeqParamSet* m_pSeqParamSet;                      // (H265SeqParamSet *) pointer to array of sequence parameters sets
public:
    H265DecoderFrame *m_pCurrentFrame;        // (H265DecoderFrame *) pointer to destination frame

    int32_t m_NumEmuPrevnBytesInSliceHdr;                        // (int32_t) number of emulation prevention bytes in slice head

    int32_t m_iNumber;                                           // (int32_t) current slice number
    int32_t m_iFirstMB;                                          // (int32_t) first MB number in slice
    int32_t m_iMaxMB;                                            // (int32_t) last unavailable  MB number in slice

    CUProcessInfo processInfo;

    bool m_bError;                                              // (bool) there is an error in decoding

    // memory management tools
    DecodingContext        *m_context;

public:

    ReferencePictureSet*  getRPS() { return &m_SliceHeader.m_rps; }

    const ReferencePictureSet*  getRPS() const { return &GetSliceHeader()->m_rps; }

    int getNumRefIdx(EnumRefPicList e) const    { return m_SliceHeader.m_numRefIdx[e]; }

    // Returns number of used references in RPS
    int getNumRpsCurrTempList() const;

    int32_t m_tileCount;
    uint32_t *m_tileByteLocation;

    uint32_t getTileLocationCount() const   { return m_tileCount; }
    void allocateTileLocation(int32_t val)
    {
        if (m_tileCount < val)
            delete[] m_tileByteLocation;

        m_tileCount = val;
        m_tileByteLocation = new uint32_t[val];
    }

    void setRefPOCListSliceHeader();

    // For dependent slice copy data from another slice
    void CopyFromBaseSlice(const H265Slice * slice);

    uint32_t getTileColumnWidth(uint32_t col) const;
    uint32_t getTileRowHeight(uint32_t row) const;

    uint32_t getTileXIdx() const;
    uint32_t getTileYIdx() const;
};

// Check whether two slices are from the same picture. HEVC spec 7.4.2.4.5
inline
bool IsPictureTheSame(H265Slice *pSliceOne, H265Slice *pSliceTwo)
{
    if (!pSliceOne)
        return true;

    const H265SliceHeader *pOne = pSliceOne->GetSliceHeader();
    const H265SliceHeader *pTwo = pSliceTwo->GetSliceHeader();

    if (pOne->first_slice_segment_in_pic_flag == 1 && pOne->first_slice_segment_in_pic_flag == pTwo->first_slice_segment_in_pic_flag)
        return false;

    if (pOne->slice_pic_parameter_set_id != pTwo->slice_pic_parameter_set_id)
        return false;

    if (pOne->m_poc != pTwo->m_poc)
        return false;

    return true;

} // bool IsPictureTheSame(H265SliceHeader *pOne, H265SliceHeader *pTwo)

// Returns true if slice is sublayer non-reference
inline
bool IsSubLayerNonReference(int32_t nal_unit_type)
{
    switch (nal_unit_type)
    {
    case NAL_UT_CODED_SLICE_RADL_N:
    case NAL_UT_CODED_SLICE_RASL_N:
    case NAL_UT_CODED_SLICE_STSA_N:
    case NAL_UT_CODED_SLICE_TSA_N:
    case NAL_UT_CODED_SLICE_TRAIL_N:
    //also need to add RSV_VCL_N10, RSV_VCL_N12, or RSV_VCL_N14,
        return true;
    }

    return false;

} // bool IsSubLayerNonReference(int32_t nal_unit_type)


} // namespace UMC_HEVC_DECODER

#endif // __UMC_H265_SLICE_DECODING_H
#endif // MFX_ENABLE_H265_VIDEO_DECODE
