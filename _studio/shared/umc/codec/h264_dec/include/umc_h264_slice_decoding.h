// Copyright (c) 2017-2019 Intel Corporation
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
#if defined (MFX_ENABLE_H264_VIDEO_DECODE)

#ifndef __UMC_H264_SLICE_DECODING_H
#define __UMC_H264_SLICE_DECODING_H

#include <list>
#include "umc_h264_dec_defs_dec.h"
#include "umc_h264_dec.h"
#include "umc_h264_bitstream_headers.h"
#include "umc_h264_heap.h"

namespace UMC
{
struct H264RefListInfo
{
    int32_t m_iNumShortEntriesInList;
    int32_t m_iNumLongEntriesInList;
    int32_t m_iNumFramesInL0List;
    int32_t m_iNumFramesInL1List;
    int32_t m_iNumFramesInLTList;

    H264RefListInfo()
        : m_iNumShortEntriesInList(0)
        , m_iNumLongEntriesInList(0)
        , m_iNumFramesInL0List(0)
        , m_iNumFramesInL1List(0)
        , m_iNumFramesInLTList(0)
    {
    }
};

class H264MemoryPiece;
class H264DecoderFrame;
class H264DecoderFrameInfo;

struct ViewItem;
typedef std::list<ViewItem> ViewList;

class H264Slice : public HeapObject
{
    // It is OK. H264SliceStore is owner of H264Slice object.
    // He can do what he wants.
    friend class H264SegmentDecoderMultiThreaded;
    friend class TaskSupplier;
    friend class TaskBroker;
    friend class TaskBrokerTwoThread;
    friend class H264DecoderFrameInfo;
    friend void PrintInfoStatus(H264DecoderFrameInfo * info);

public:
    // Default constructor
    H264Slice(MemoryAllocator *pMemoryAllocator = 0);
    // Destructor
    virtual
    ~H264Slice(void);

    // Set slice source data
    virtual bool Reset(UMC_H264_DECODER::H264NalExtension *pNalExt);
    // Set current slice number
    void SetSliceNumber(int32_t iSliceNumber);

    virtual void FreeResources();


    int32_t RetrievePicParamSetNumber();

    //
    // method(s) to obtain slice specific information
    //

    // Obtain pointer to slice header
    const UMC_H264_DECODER::H264SliceHeader *GetSliceHeader(void) const {return &m_SliceHeader;}
    UMC_H264_DECODER::H264SliceHeader *GetSliceHeader(void) {return &m_SliceHeader;}
    // Obtain bit stream object
    H264HeadersBitstream *GetBitStream(void){return &m_BitStream;}
    // Obtain prediction weigth table
    const UMC_H264_DECODER::PredWeightTable *GetPredWeigthTable(int32_t iNum) const {return m_PredWeight[iNum & 1];}
    // Obtain first MB number
    int32_t GetFirstMBNumber(void) const {return m_iFirstMBFld;}
    int32_t GetStreamFirstMB(void) const {return m_iFirstMB;}
    void SetFirstMBNumber(int32_t x) {m_iFirstMB = x;}
    // Obtain MB width
    int32_t GetMBWidth(void) const {return m_iMBWidth;}
    // Obtain MB row width
    int32_t GetMBRowWidth(void) const {return (m_iMBWidth * (m_SliceHeader.MbaffFrameFlag + 1));}
    // Obtain MB height
    int32_t GetMBHeight(void) const {return m_iMBHeight;}
    // Obtain current picture parameter set number
    int32_t GetPicParamSet(void) const {return m_pPicParamSet->pic_parameter_set_id;}
    // Obtain current sequence parameter set number
    int32_t GetSeqParamSet(void) const {return m_pSeqParamSet->seq_parameter_set_id;}
    // Obtain current picture parameter set
    const UMC_H264_DECODER::H264PicParamSet *GetPicParam(void) const {return m_pPicParamSet;}
    void SetPicParam(const UMC_H264_DECODER::H264PicParamSet * pps) {m_pPicParamSet = pps;}
    // Obtain current sequence parameter set
    const UMC_H264_DECODER::H264SeqParamSet *GetSeqParam(void) const {return m_pSeqParamSet;}
    void SetSeqParam(const UMC_H264_DECODER::H264SeqParamSet * sps) {m_pSeqParamSet = sps;}

    // Obtain current sequence extension parameter set
    const UMC_H264_DECODER::H264SeqParamSetExtension *GetSeqParamEx(void) const {return m_pSeqParamSetEx;}
    void SetSeqExParam(const UMC_H264_DECODER::H264SeqParamSetExtension * spsex) {m_pSeqParamSetEx = spsex;}

    const UMC_H264_DECODER::H264SeqParamSetMVCExtension *GetSeqMVCParam(void) const {return m_pSeqParamSetMvcEx;}
    void SetSeqMVCParam(const UMC_H264_DECODER::H264SeqParamSetMVCExtension * sps);

    const UMC_H264_DECODER::H264SeqParamSetSVCExtension *GetSeqSVCParam(void) const {return m_pSeqParamSetSvcEx;}
    void SetSeqSVCParam(const UMC_H264_DECODER::H264SeqParamSetSVCExtension * sps);

    // Obtain current destination frame
    H264DecoderFrame *GetCurrentFrame(void){return m_pCurrentFrame;}
    void SetCurrentFrame(H264DecoderFrame * pFrame){m_pCurrentFrame = pFrame;}

    // Obtain slice number
    int32_t GetSliceNum(void) const {return m_iNumber;}
    // Obtain maximum of macroblock
    int32_t GetMaxMB(void) const {return m_iMaxMB;}
    void SetMaxMB(int32_t x) {m_iMaxMB = x;}

    int32_t GetMBCount() const { return m_iMaxMB - m_iFirstMB;}

    // Check field slice
    bool IsField() const {return m_SliceHeader.field_pic_flag != 0;}
    // Check top field slice
    bool IsTopField() const {return m_SliceHeader.bottom_field_flag == 0;}
    // Check top field slice
    bool IsBottomField() const {return !IsTopField();}

    // Check slice organization
    bool IsSliceGroups(void) const {return (1 < m_pPicParamSet->num_slice_groups);};
    // Do we require to do deblocking through slice boundaries
    bool DeblockThroughBoundaries(void) const {return (DEBLOCK_FILTER_ON_NO_SLICE_EDGES != m_SliceHeader.disable_deblocking_filter_idc);};

    // Update reference list
    virtual Status UpdateReferenceList(ViewList &views,
        int32_t dIdIndex);

    //
    // Segment decoding mode's variables
    //

    UMC_H264_DECODER::AdaptiveMarkingInfo * GetAdaptiveMarkingInfo();

    bool IsError() const {return m_bError;}

    bool IsReference() const {return m_SliceHeader.nal_ref_idc != 0;}

    void SetHeap(H264_Heap_Objects  *pObjHeap)
    {
        m_pObjHeap = pObjHeap;
    }

    void CompleteDecoding();

public:

    H264MemoryPiece m_pSource;                                 // (H264MemoryPiece *) pointer to owning memory piece
    double m_dTime;                                             // (double) slice's time stamp

public:  // DEBUG !!!! should remove dependence

    void Reset();

    virtual void Release();

    virtual void ZeroedMembers();

    // Decode slice header
    bool DecodeSliceHeader(UMC_H264_DECODER::H264NalExtension *pNalExt);

    // Reference list(s) management functions & tools
    int32_t AdjustRefPicListForFields(H264DecoderFrame **pRefPicList, ReferenceFlags *pFields, H264RefListInfo &rli);
    void ReOrderRefPicList(H264DecoderFrame **pRefPicList, ReferenceFlags *pFields, UMC_H264_DECODER::RefPicListReorderInfo *pReorderInfo, int32_t MaxPicNum, ViewList &views, int32_t dIdIndex, uint32_t listNum);

    UMC_H264_DECODER::RefPicListReorderInfo ReorderInfoL0;                        // (RefPicListReorderInfo) reference list 0 info
    UMC_H264_DECODER::RefPicListReorderInfo ReorderInfoL1;                        // (RefPicListReorderInfo) reference list 1 info

    UMC_H264_DECODER::H264SliceHeader m_SliceHeader;                              // (H264SliceHeader) slice header
    H264HeadersBitstream m_BitStream;                           // (H264Bitstream) slice bit stream

    UMC_H264_DECODER::PredWeightTable m_PredWeight[2][MAX_NUM_REF_FRAMES];        // (PredWeightTable []) prediction weight table

    const UMC_H264_DECODER::H264PicParamSet* m_pPicParamSet;                      // (H264PicParamSet *) pointer to array of picture parameters sets
    const UMC_H264_DECODER::H264SeqParamSet* m_pSeqParamSet;                      // (H264SeqParamSet *) pointer to array of sequence parameters sets
    const UMC_H264_DECODER::H264SeqParamSetExtension* m_pSeqParamSetEx;
    const UMC_H264_DECODER::H264SeqParamSetMVCExtension* m_pSeqParamSetMvcEx;
    const UMC_H264_DECODER::H264SeqParamSetSVCExtension* m_pSeqParamSetSvcEx;

    H264DecoderFrame *m_pCurrentFrame;        // (H264DecoderFrame *) pointer to destination frame

    int32_t m_iMBWidth;                                          // (int32_t) width in macroblock units
    int32_t m_iMBHeight;                                         // (int32_t) height in macroblock units

    int32_t m_iNumber;                                           // (int32_t) current slice number
    int32_t m_iFirstMB;                                          // (int32_t) first MB number in slice
    int32_t m_iMaxMB;                                            // (int32_t) last unavailable  MB number in slice

    int32_t m_iFirstMBFld;                                       // (int32_t) first MB number in slice

    int32_t m_iAvailableMB;                                      // (int32_t) available number of macroblocks (used in "unknown mode")

    bool m_bFirstDebThreadedCall;                               // (bool) "first threaded deblocking call" flag
    bool m_bPermanentTurnOffDeblocking;                         // (bool) "disable deblocking" flag
    bool m_bError;                                              // (bool) there is an error in decoding
    bool m_isInitialized;

    UMC_H264_DECODER::AdaptiveMarkingInfo     m_AdaptiveMarkingInfo;
    UMC_H264_DECODER::AdaptiveMarkingInfo     m_BaseAdaptiveMarkingInfo;

    bool m_bDecoded;                                            // (bool) "slice has been decoded" flag
    bool m_bPrevDeblocked;                                      // (bool) "previous slice has been deblocked" flag
    bool m_bDeblocked;                                          // (bool) "slice has been deblocked" flag
    bool m_bInited;

    // memory management tools
    MemoryAllocator *m_pMemoryAllocator;                        // (MemoryAllocator *) pointer to memory allocation tool

    H264_Heap_Objects           *m_pObjHeap;
};

inline
bool IsPictureTheSame(H264Slice *pSliceOne, H264Slice *pSliceTwo)
{
    if (!pSliceOne)
        return true;

    const UMC_H264_DECODER::H264SliceHeader *pOne = pSliceOne->GetSliceHeader();
    const UMC_H264_DECODER::H264SliceHeader *pTwo = pSliceTwo->GetSliceHeader();

    // this function checks two slices are from same picture or not
    // 7.4.1.2.4 and G.7.4.1.2.4 parts of h264 standard

    if (pOne->nal_ext.mvc.view_id != pTwo->nal_ext.mvc.view_id)
    {
        return false;
    }

    if (pOne->nal_ext.svc.dependency_id < pTwo->nal_ext.svc.dependency_id)
        return true;

    if (pOne->nal_ext.svc.dependency_id > pTwo->nal_ext.svc.dependency_id)
        return false;

    if (pOne->nal_ext.svc.quality_id < pTwo->nal_ext.svc.quality_id)
        return true;

    if (pOne->nal_ext.svc.quality_id > pTwo->nal_ext.svc.quality_id)
        return false;

    if ((pOne->frame_num != pTwo->frame_num) ||
        //(pOne->first_mb_in_slice == pTwo->first_mb_in_slice) || // need to remove in case of duplicate slices !!!
        (pOne->pic_parameter_set_id != pTwo->pic_parameter_set_id) ||
        (pOne->field_pic_flag != pTwo->field_pic_flag) ||
        (pOne->bottom_field_flag != pTwo->bottom_field_flag))
        return false;

    if ((pOne->nal_ref_idc != pTwo->nal_ref_idc) &&
        (0 == std::min(pOne->nal_ref_idc, pTwo->nal_ref_idc)))
        return false;

    if (0 == pSliceTwo->GetSeqParam()->pic_order_cnt_type)
    {
        if ((pOne->pic_order_cnt_lsb != pTwo->pic_order_cnt_lsb) ||
            (pOne->delta_pic_order_cnt_bottom != pTwo->delta_pic_order_cnt_bottom))
            return false;
    }
    else
    {
        if ((pOne->delta_pic_order_cnt[0] != pTwo->delta_pic_order_cnt[0]) ||
            (pOne->delta_pic_order_cnt[1] != pTwo->delta_pic_order_cnt[1]))
            return false;
    }

    if (pOne->IdrPicFlag != pTwo->IdrPicFlag)
    {
        return false;
    }

    if (pOne->IdrPicFlag)
    {
        if (pOne->idr_pic_id != pTwo->idr_pic_id)
            return false;
    }

    return true;

} // bool IsPictureTheSame(H264SliceHeader *pOne, H264SliceHeader *pTwo)


// Declare function to swapping memory
extern void SwapMemoryAndRemovePreventingBytes(void *pDst, size_t &nDstSize, void *pSrc, size_t nSrcSize);

} // namespace UMC

#endif // __UMC_H264_SLICE_DECODING_H
#endif // MFX_ENABLE_H264_VIDEO_DECODE
