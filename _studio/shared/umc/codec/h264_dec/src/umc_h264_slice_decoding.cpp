// Copyright (c) 2017-2018 Intel Corporation
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

#include "umc_h264_frame.h"
#include "umc_h264_slice_decoding.h"
#include "umc_h264_heap.h"

namespace UMC
{

H264Slice::H264Slice(MemoryAllocator *pMemoryAllocator)
    : m_pSeqParamSet(0)
    , m_bInited(false)
    , m_pMemoryAllocator(pMemoryAllocator)
{
    Reset();
} // H264Slice::H264Slice()

H264Slice::~H264Slice()
{
    Release();

} // H264Slice::~H264Slice(void)

void H264Slice::Reset()
{
    m_pSource.Release();

    if (m_bInited && m_pSeqParamSet)
    {
        if (m_pSeqParamSet)
            ((UMC_H264_DECODER::H264SeqParamSet*)m_pSeqParamSet)->DecrementReference();
        if (m_pPicParamSet)
            ((UMC_H264_DECODER::H264PicParamSet*)m_pPicParamSet)->DecrementReference();
        m_pSeqParamSet = 0;
        m_pPicParamSet = 0;

        if (m_pSeqParamSetEx)
        {
            ((UMC_H264_DECODER::H264SeqParamSetExtension*)m_pSeqParamSetEx)->DecrementReference();
            m_pSeqParamSetEx = 0;
        }

        if (m_pSeqParamSetMvcEx)
        {
            ((UMC_H264_DECODER::H264SeqParamSetMVCExtension*)m_pSeqParamSetMvcEx)->DecrementReference();
            m_pSeqParamSetMvcEx = 0;
        }

        if (m_pSeqParamSetSvcEx)
        {
            ((UMC_H264_DECODER::H264SeqParamSetSVCExtension*)m_pSeqParamSetSvcEx)->DecrementReference();
            m_pSeqParamSetSvcEx = 0;
        }
    }

    ZeroedMembers();
    FreeResources();
}

void H264Slice::ZeroedMembers()
{
    m_pPicParamSet = 0;
    m_pSeqParamSet = 0;
    m_pSeqParamSetEx = 0;
    m_pSeqParamSetMvcEx = 0;
    m_pSeqParamSetSvcEx = 0;

    m_iMBWidth = -1;
    m_iMBHeight = -1;

    m_pCurrentFrame = 0;

    memset(&m_AdaptiveMarkingInfo, 0, sizeof(m_AdaptiveMarkingInfo));

    m_bInited = false;
    m_isInitialized = false;
}

void H264Slice::Release()
{
    Reset();
}

int32_t H264Slice::RetrievePicParamSetNumber()
{
    if (!m_pSource.GetDataSize())
        return -1;

    memset(&m_SliceHeader, 0, sizeof(m_SliceHeader));
    m_BitStream.Reset((uint8_t *) m_pSource.GetPointer(), (uint32_t) m_pSource.GetDataSize());

    Status umcRes = UMC_OK;

    try
    {
        umcRes = m_BitStream.GetNALUnitType(m_SliceHeader.nal_unit_type,
                                            m_SliceHeader.nal_ref_idc);
        if (UMC_OK != umcRes)
            return false;

        // decode first part of slice header
        umcRes = m_BitStream.GetSliceHeaderPart1(&m_SliceHeader);
        if (UMC_OK != umcRes)
            return -1;
    } catch (...)
    {
        return -1;
    }

    return m_SliceHeader.pic_parameter_set_id;
}

void H264Slice::FreeResources()
{
}

bool H264Slice::Reset(UMC_H264_DECODER::H264NalExtension *pNalExt)
{
    int32_t iMBInFrame;
    int32_t iFieldIndex;

    m_BitStream.Reset((uint8_t *) m_pSource.GetPointer(), (uint32_t) m_pSource.GetDataSize());

    // decode slice header
    if (m_pSource.GetDataSize() && false == DecodeSliceHeader(pNalExt))
    {
        ZeroedMembers();
        return false;
    }

    m_iMBWidth  = GetSeqParam()->frame_width_in_mbs;
    m_iMBHeight = GetSeqParam()->frame_height_in_mbs;

    iMBInFrame = (m_iMBWidth * m_iMBHeight) / ((m_SliceHeader.field_pic_flag) ? (2) : (1));
    iFieldIndex = (m_SliceHeader.field_pic_flag && m_SliceHeader.bottom_field_flag) ? (1) : (0);

    // set slice variables
    m_iFirstMB = m_SliceHeader.first_mb_in_slice;
    m_iMaxMB = iMBInFrame;

    m_iFirstMBFld = m_SliceHeader.first_mb_in_slice + iMBInFrame * iFieldIndex;

    m_iAvailableMB = iMBInFrame;

    if (m_iFirstMB >= m_iAvailableMB)
        return false;

    // reset all internal variables
    m_bFirstDebThreadedCall = true;
    m_bError = false;

    // set conditional flags
    m_bDecoded = false;
    m_bPrevDeblocked = false;
    m_bDeblocked = (m_SliceHeader.disable_deblocking_filter_idc == DEBLOCK_FILTER_OFF);

    // frame is not associated yet
    m_pCurrentFrame = NULL;

    m_bInited = true;
    m_pSeqParamSet->IncrementReference();
    m_pPicParamSet->IncrementReference();
    if (m_pSeqParamSetEx)
        m_pSeqParamSetEx->IncrementReference();
    if (m_pSeqParamSetMvcEx)
        m_pSeqParamSetMvcEx->IncrementReference();
    if (m_pSeqParamSetSvcEx)
        m_pSeqParamSetSvcEx->IncrementReference();

    return true;

} // bool H264Slice::Reset(void *pSource, size_t nSourceSize, int32_t iNumber)

void H264Slice::SetSeqMVCParam(const UMC_H264_DECODER::H264SeqParamSetMVCExtension * sps)
{
    const UMC_H264_DECODER::H264SeqParamSetMVCExtension * temp = m_pSeqParamSetMvcEx;
    m_pSeqParamSetMvcEx = sps;
    if (m_pSeqParamSetMvcEx)
        m_pSeqParamSetMvcEx->IncrementReference();

    if (temp)
        ((UMC_H264_DECODER::H264SeqParamSetMVCExtension*)temp)->DecrementReference();
}

void H264Slice::SetSeqSVCParam(const UMC_H264_DECODER::H264SeqParamSetSVCExtension * sps)
{
    const UMC_H264_DECODER::H264SeqParamSetSVCExtension * temp = m_pSeqParamSetSvcEx;
    m_pSeqParamSetSvcEx = sps;
    if (m_pSeqParamSetSvcEx)
        m_pSeqParamSetSvcEx->IncrementReference();

    if (temp)
        ((UMC_H264_DECODER::H264SeqParamSetSVCExtension*)temp)->DecrementReference();
}

void H264Slice::SetSliceNumber(int32_t iSliceNumber)
{
    m_iNumber = iSliceNumber;
}

UMC_H264_DECODER::AdaptiveMarkingInfo * H264Slice::GetAdaptiveMarkingInfo()
{
    return &m_AdaptiveMarkingInfo;
}

bool H264Slice::DecodeSliceHeader(UMC_H264_DECODER::H264NalExtension *pNalExt)
{
    Status umcRes = UMC_OK;
    // Locals for additional slice data to be read into, the data
    // was read and saved from the first slice header of the picture,
    // is not supposed to change within the picture, so can be
    // discarded when read again here.
    int32_t iSQUANT;

    try
    {
        memset(&m_SliceHeader, 0, sizeof(m_SliceHeader));

        umcRes = m_BitStream.GetNALUnitType(m_SliceHeader.nal_unit_type,
                                            m_SliceHeader.nal_ref_idc);
        if (UMC_OK != umcRes)
            return false;

        if (pNalExt && (NAL_UT_CODED_SLICE_EXTENSION != m_SliceHeader.nal_unit_type))
        {
            if (pNalExt->extension_present)
                m_SliceHeader.nal_ext = *pNalExt;
            pNalExt->extension_present = 0;
        }

        // adjust slice type for auxilary slice
        if (NAL_UT_AUXILIARY == m_SliceHeader.nal_unit_type)
        {
            if (!m_pCurrentFrame || !GetSeqParamEx())
                return false;

            m_SliceHeader.nal_unit_type = m_pCurrentFrame->m_bIDRFlag ?
                                          NAL_UT_IDR_SLICE :
                                          NAL_UT_SLICE;
            m_SliceHeader.is_auxiliary = true;
        }

        // decode first part of slice header
        umcRes = m_BitStream.GetSliceHeaderPart1(&m_SliceHeader);
        if (UMC_OK != umcRes)
            return false;


        // decode second part of slice header
        umcRes = m_BitStream.GetSliceHeaderPart2(&m_SliceHeader,
                                                 m_pPicParamSet,
                                                 m_pSeqParamSet);
        if (UMC_OK != umcRes)
            return false;

        // decode second part of slice header
        umcRes = m_BitStream.GetSliceHeaderPart3(&m_SliceHeader,
                                                 m_PredWeight[0],
                                                 m_PredWeight[1],
                                                 &ReorderInfoL0,
                                                 &ReorderInfoL1,
                                                 &m_AdaptiveMarkingInfo,
                                                 &m_BaseAdaptiveMarkingInfo,
                                                 m_pPicParamSet,
                                                 m_pSeqParamSet,
                                                 m_pSeqParamSetSvcEx);
        if (UMC_OK != umcRes)
            return false;

        //7.4.3 Slice header semantics
        //If the current picture is an IDR picture, frame_num shall be equal to 0
        //If this restrictions is violated, clear LT flag to avoid ref. pic. marking process corruption
        if (m_SliceHeader.IdrPicFlag && m_SliceHeader.frame_num != 0)
            m_SliceHeader.long_term_reference_flag = 0;

        // decode 4 part of slice header
        umcRes = m_BitStream.GetSliceHeaderPart4(&m_SliceHeader, m_pSeqParamSetSvcEx);
        if (UMC_OK != umcRes)
            return false;

        m_iMBWidth = m_pSeqParamSet->frame_width_in_mbs;
        m_iMBHeight = m_pSeqParamSet->frame_height_in_mbs;

        // redundant slice, discard
        if (m_SliceHeader.redundant_pic_cnt)
            return false;

        // Set next MB.
        if (m_SliceHeader.first_mb_in_slice >= (int32_t) (m_iMBWidth * m_iMBHeight))
        {
            return false;
        }

        int32_t bit_depth_luma = m_SliceHeader.is_auxiliary ?
            GetSeqParamEx()->bit_depth_aux : GetSeqParam()->bit_depth_luma;

        iSQUANT = m_pPicParamSet->pic_init_qp +
                  m_SliceHeader.slice_qp_delta;
        if (iSQUANT < QP_MIN - 6*(bit_depth_luma - 8)
            || iSQUANT > QP_MAX)
        {
            return false;
        }

        m_SliceHeader.hw_wa_redundant_elimination_bits[2] = (uint32_t)m_BitStream.BitsDecoded();

        if (m_pPicParamSet->entropy_coding_mode)
            m_BitStream.AlignPointerRight();
    }
    catch(const h264_exception & )
    {
        return false;
    }
    catch(...)
    {
        return false;
    }

    return (UMC_OK == umcRes);

}

} // namespace UMC
#endif // MFX_ENABLE_H264_VIDEO_DECODE
