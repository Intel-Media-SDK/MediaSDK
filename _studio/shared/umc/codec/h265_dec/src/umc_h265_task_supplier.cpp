// Copyright (c) 2018-2020 Intel Corporation
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

#include "memory"

#include <limits.h> // for INT_MIN, INT_MAX on Linux
#include <functional>
#include <algorithm>

#include "umc_h265_task_supplier.h"
#include "umc_h265_frame_list.h"
#include "umc_h265_nal_spl.h"
#include "umc_h265_bitstream_headers.h"

#include "umc_h265_dec_defs.h"
#include "vm_sys_info.h"

#include "umc_h265_task_broker.h"

#include "umc_structures.h"
#include "umc_frame_data.h"
#include "umc_h265_debug.h"


#include "mfx_common.h" //  for trace routines

namespace UMC_HEVC_DECODER
{

#if (MFX_VERSION >= 1025)
inline void SetDecodeErrorTypes(NalUnitType nalUnit, mfxExtDecodeErrorReport *pDecodeErrorReport)
{
    if (!pDecodeErrorReport)
        return;

    switch (nalUnit)
    {
        case NAL_UT_SPS: pDecodeErrorReport->ErrorTypes |= MFX_ERROR_SPS; break;
        case NAL_UT_PPS: pDecodeErrorReport->ErrorTypes |= MFX_ERROR_PPS; break;
        default: break;
    };
}
#endif

const uint32_t levelIndexArray[] = {
    H265_LEVEL_1,
    H265_LEVEL_2,
    H265_LEVEL_21,

    H265_LEVEL_3,
    H265_LEVEL_31,

    H265_LEVEL_4,
    H265_LEVEL_41,

    H265_LEVEL_5,
    H265_LEVEL_51,
    H265_LEVEL_52,

    H265_LEVEL_6,
    H265_LEVEL_61,
    H265_LEVEL_62
};

/****************************************************************************************************/
// DecReferencePictureMarking_H265
/****************************************************************************************************/
DecReferencePictureMarking_H265::DecReferencePictureMarking_H265()
    : m_isDPBErrorFound(0)
    , m_frameCount(0)
{
}

void DecReferencePictureMarking_H265::Reset()
{
    m_frameCount = 0;
    m_isDPBErrorFound = 0;
}

void DecReferencePictureMarking_H265::ResetError()
{
    m_isDPBErrorFound = 0;
}

uint32_t DecReferencePictureMarking_H265::GetDPBError() const
{
    return m_isDPBErrorFound;
}

// Update DPB contents marking frames for reuse
UMC::Status DecReferencePictureMarking_H265::UpdateRefPicMarking(ViewItem_H265 &view, const H265Slice * pSlice)
{
    UMC::Status umcRes = UMC::UMC_OK;

    m_frameCount++;

    H265SliceHeader const * sliceHeader = pSlice->GetSliceHeader();

    if (sliceHeader->IdrPicFlag)
    {
        // mark all reference pictures as unused
        for (H265DecoderFrame *pCurr = view.pDPB->head(); pCurr; pCurr = pCurr->future())
        {
            if (pCurr->isShortTermRef() || pCurr->isLongTermRef())
            {
                pCurr->SetisShortTermRef(false);
                pCurr->SetisLongTermRef(false);
            }
        }
    }
    else
    {
        const ReferencePictureSet* rps = pSlice->getRPS();
        // loop through all pictures in the reference picture buffer
        for (H265DecoderFrame *pTmp = view.pDPB->head(); pTmp; pTmp = pTmp->future())
        {
            if (pTmp->isDisposable() || (!pTmp->isShortTermRef() && !pTmp->isLongTermRef()))
                continue;

            bool isReferenced = false;

            if (!pTmp->RefPicListResetCount())
            {
                // loop through all pictures in the Reference Picture Set
                // to see if the picture should be kept as reference picture
                int32_t i;
                int32_t count = rps->getNumberOfPositivePictures() + rps->getNumberOfNegativePictures();
                for(i = 0; i < count; i++)
                {
                    if (!pTmp->isLongTermRef() && pTmp->m_PicOrderCnt == pSlice->GetSliceHeader()->m_poc + rps->getDeltaPOC(i))
                    {
                        isReferenced = true;
                        pTmp->SetisShortTermRef(true);
                        pTmp->SetisLongTermRef(false);
                        pTmp->m_isUsedAsReference = true;
                    }
                }

                count = rps->getNumberOfPictures();
                for( ; i < count; i++)
                {
                    if (rps->getCheckLTMSBPresent(i))
                    {
                        if (pTmp->m_PicOrderCnt == rps->getPOC(i))
                        {
                            isReferenced = true;
                            pTmp->SetisLongTermRef(true);
                            pTmp->SetisShortTermRef(false);
                            pTmp->m_isUsedAsReference = true;
                        }
                    }
                    else
                    {
                        int const MaxPicOrderCntLsb = 1 << pSlice->GetSeqParam()->log2_max_pic_order_cnt_lsb;
                        if ((pTmp->m_PicOrderCnt & (MaxPicOrderCntLsb - 1)) == (rps->getPOC(i) & (MaxPicOrderCntLsb - 1)))
                        {
                            isReferenced = true;
                            pTmp->SetisLongTermRef(true);
                            pTmp->SetisShortTermRef(false);
                            pTmp->m_isUsedAsReference = true;
                        }
                    }
                }
            }

            // mark the picture as "unused for reference" if it is not in
            // the Reference Picture Set
            if(pTmp->m_PicOrderCnt != pSlice->GetSliceHeader()->m_poc && !isReferenced)
            {
                pTmp->SetisShortTermRef(false);
                pTmp->SetisLongTermRef(false);
                DEBUG_PRINT1((VM_STRING("Dereferencing frame with POC %d, busy = %d\n"), pTmp->m_PicOrderCnt, pTmp->GetRefCounter()));
            }
            // WA: To fix incorrect stream having same (as incoming slice) POC in DPB
            // Mark a older frame having similar POCs within DPBList as unused reference frame if similar mutiple POCs are found
            // within the DPBList. This is to prevent the DPBList from max out with unused reference frame kept as reference picture.
            else if(pTmp->m_PicOrderCnt == pSlice->GetSliceHeader()->m_poc && !isReferenced && pTmp->isShortTermRef())
            {
                pTmp->SetisShortTermRef(false);
                pTmp->SetisLongTermRef(false);
                DEBUG_PRINT1((VM_STRING("Dereferencing frame with POC %d, busy = %d\n"), pTmp->m_PicOrderCnt, pTmp->GetRefCounter()));
            }
        }
    }    // not IDR picture

    return umcRes;
}

// Check if bitstream resolution has changed
static
bool IsNeedSPSInvalidate(const H265SeqParamSet *old_sps, const H265SeqParamSet *new_sps)
{
    if (!old_sps || !new_sps)
        return false;

    //if (new_sps->no_output_of_prior_pics_flag)
      //  return true;

    if (old_sps->pic_width_in_luma_samples != new_sps->pic_width_in_luma_samples)
        return true;

    if (old_sps->pic_height_in_luma_samples != new_sps->pic_height_in_luma_samples)
        return true;

    if (old_sps->bit_depth_luma != new_sps->bit_depth_luma)
        return true;

    if (old_sps->bit_depth_chroma != new_sps->bit_depth_chroma)
        return true;

    if (old_sps->m_pcPTL.GetGeneralPTL()->profile_idc != new_sps->m_pcPTL.GetGeneralPTL()->profile_idc)
        return true;

    if (old_sps->chroma_format_idc != new_sps->chroma_format_idc)
        return true;

    if (old_sps->sps_max_dec_pic_buffering[0] < new_sps->sps_max_dec_pic_buffering[0])
        return true;

    return false;
}

/****************************************************************************************************/
// MVC_Extension_H265 class routine
/****************************************************************************************************/
MVC_Extension::MVC_Extension()
    : m_temporal_id(7)
    , m_priority_id(63)
    , HighestTid(0)
    , m_level_idc(0)
{
    Reset();
}

MVC_Extension::~MVC_Extension()
{
    Close();
}

UMC::Status MVC_Extension::Init()
{
    MVC_Extension::Close();

    m_view.pDPB.reset(new H265DBPList());
    return UMC::UMC_OK;
}

void MVC_Extension::Close()
{
    MVC_Extension::Reset();
}

void MVC_Extension::Reset()
{
    m_temporal_id = 7;
    m_priority_id = 63;
    m_level_idc = 0;
    HighestTid = 0;

    m_view.Reset();
}

ViewItem_H265 * MVC_Extension::GetView()
{
    return &m_view;
}


/****************************************************************************************************/
// Skipping_H265 class routine
/****************************************************************************************************/
Skipping_H265::Skipping_H265()
    : m_VideoDecodingSpeed(0)
    , m_SkipCycle(1)
    , m_ModSkipCycle(1)
    , m_PermanentTurnOffDeblocking(0)
    , m_NumberOfSkippedFrames(0)
{
}

Skipping_H265::~Skipping_H265()
{
}

void Skipping_H265::Reset()
{
    m_VideoDecodingSpeed = 0;
    m_SkipCycle = 0;
    m_ModSkipCycle = 0;
    m_PermanentTurnOffDeblocking = 0;
    m_NumberOfSkippedFrames = 0;
}

// Disable deblocking filter to increase performance
void Skipping_H265::PermanentDisableDeblocking(bool disable)
{
    m_PermanentTurnOffDeblocking = disable ? 3 : 0;
}

// Check if deblocking should be skipped
bool Skipping_H265::IsShouldSkipDeblocking(H265DecoderFrame *)
{
    return (IS_SKIP_DEBLOCKING_MODE_PREVENTIVE || IS_SKIP_DEBLOCKING_MODE_PERMANENT);
}

// Check if frame should be skipped to decrease decoding delays
bool Skipping_H265::IsShouldSkipFrame(H265DecoderFrame * )
{
    return false;
}

// Set decoding skip frame mode
void Skipping_H265::ChangeVideoDecodingSpeed(int32_t & num)
{
    m_VideoDecodingSpeed += num;

    if (m_VideoDecodingSpeed < 0)
        m_VideoDecodingSpeed = 0;
    if (m_VideoDecodingSpeed > 7)
        m_VideoDecodingSpeed = 7;

    num = m_VideoDecodingSpeed;

    int32_t deblocking_off = m_PermanentTurnOffDeblocking;
    if (deblocking_off == 3)
        m_PermanentTurnOffDeblocking = 3;
}

// Get current skip mode state
Skipping_H265::SkipInfo Skipping_H265::GetSkipInfo() const
{
    SkipInfo info;
    info.isDeblockingTurnedOff = (m_VideoDecodingSpeed == 5) || (m_VideoDecodingSpeed == 7);
    info.numberOfSkippedFrames = m_NumberOfSkippedFrames;
    return info;
}

/****************************************************************************************************/
// SEI_Storer_H265
/****************************************************************************************************/
SEI_Storer_H265::SEI_Storer_H265()
{
    Reset();
}

SEI_Storer_H265::~SEI_Storer_H265()
{
    Close();
}

// Initialize SEI storage
void SEI_Storer_H265::Init()
{
    Close();
    m_data.resize(MAX_BUFFERED_SIZE);
    m_payloads.resize(START_ELEMENTS);
    m_offset = 0;
    m_lastUsed = 2;
}

// Deallocate SEI storage
void SEI_Storer_H265::Close()
{
    Reset();
    m_data.clear();
    m_payloads.clear();
}

// Reset SEI storage
void SEI_Storer_H265::Reset()
{
    m_offset = 0;
    m_lastUsed = 2;
    for (uint32_t i = 0; i < m_payloads.size(); i++)
    {
        m_payloads[i].isUsed = 0;
    }
}

// Set SEI frame for stored SEI messages
void SEI_Storer_H265::SetFrame(H265DecoderFrame * frame)
{
    VM_ASSERT(frame);
    for (uint32_t i = 0; i < m_payloads.size(); i++)
    {
        if (m_payloads[i].frame == 0 && m_payloads[i].isUsed)
        {
            m_payloads[i].frame = frame;
        }
    }
}

// Set timestamp for stored SEI messages
void SEI_Storer_H265::SetTimestamp(H265DecoderFrame * frame)
{
    VM_ASSERT(frame);
    double ts = frame->m_dFrameTime;

    for (uint32_t i = 0; i < m_payloads.size(); i++)
    {
        if (m_payloads[i].frame == frame)
        {
            m_payloads[i].timestamp = ts;
            if (m_payloads[i].isUsed)
                m_payloads[i].isUsed = m_lastUsed;
        }
    }

    m_lastUsed++;
}

// Retrieve a stored SEI message which was not retrieved before
const SEI_Storer_H265::SEI_Message * SEI_Storer_H265::GetPayloadMessage()
{
    SEI_Storer_H265::SEI_Message * msg = 0;

    for (uint32_t i = 0; i < m_payloads.size(); i++)
    {
        if (m_payloads[i].isUsed > 1)
        {
            if (!msg || msg->isUsed > m_payloads[i].isUsed)
            {
                msg = &m_payloads[i];
            }
        }
    }

    if (msg)
        msg->isUsed = 0;

    return msg;
}

// Put a new SEI message to the storage
SEI_Storer_H265::SEI_Message* SEI_Storer_H265::AddMessage(UMC::MediaDataEx *nalUnit, SEI_TYPE type)
{
    size_t const size
        = nalUnit->GetDataSize();

    if (size > (m_data.size() >> 2))
        return 0;

    if (m_offset + size > m_data.size())
    {
        m_offset = 0;
    }

    // clear overwriting messages:
    for (uint32_t i = 0; i < m_payloads.size(); i++)
    {
        if (!m_payloads[i].isUsed)
            continue;

        SEI_Message & mmsg = m_payloads[i];

        if ((m_offset + size > mmsg.offset) &&
            (m_offset < mmsg.offset + mmsg.size))
        {
            m_payloads[i].isUsed = 0;
            return 0;
        }
    }

    size_t freeSlot = 0;
    //move empty (not used) payloads to the end of sequence
    std::vector<SEI_Message>::iterator
        end = std::remove_if(m_payloads.begin(), m_payloads.end(), std::mem_fn(&SEI_Message::empty));
    if (end != m_payloads.end())
    {
        //since the state of elements after new logical end is unspecified
        //we have to clear (mark as not used) them
        std::for_each(end, m_payloads.end(), std::mem_fn(&SEI_Message::clear));
        freeSlot = std::distance(m_payloads.begin(), end);
    }
    else
    {
        if (m_payloads.size() >= MAX_ELEMENTS)
            return 0;

        m_payloads.push_back(SEI_Message());
        freeSlot = m_payloads.size() - 1;
    }

    m_payloads[freeSlot].frame     = 0;
    m_payloads[freeSlot].offset    = m_offset;
    m_payloads[freeSlot].size      = size;
    m_payloads[freeSlot].data      = &m_data[m_offset];

    if (nalUnit->GetExData())
        m_payloads[freeSlot].nal_type = nalUnit->GetExData()->values[0];

    m_payloads[freeSlot].type      = type;

    m_payloads[freeSlot].isUsed = 1;

    MFX_INTERNAL_CPY(m_payloads[freeSlot].data, (uint8_t*)nalUnit->GetDataPointer(), m_payloads[freeSlot].size);

    m_offset += size;
    return &m_payloads[freeSlot];
}

ViewItem_H265::ViewItem_H265()
{
    Reset();

} // ViewItem_H265::ViewItem_H265(void)

ViewItem_H265::ViewItem_H265(const ViewItem_H265 &src)
{
    Reset();

    pDPB.reset(src.pDPB.release());
    dpbSize = src.dpbSize;
    sps_max_dec_pic_buffering = src.sps_max_dec_pic_buffering;
    sps_max_num_reorder_pics = src.sps_max_num_reorder_pics;

} // ViewItem_H265::ViewItem_H265(const ViewItem_H265 &src)

ViewItem_H265::~ViewItem_H265()
{
    Close();

} // ViewItem_H265::ViewItem_H265(void)

// Initialize the view, allocate resources
UMC::Status ViewItem_H265::Init()
{
    // release the object before initialization
    Close();

    try
    {
        // allocate DPB and POC counter
        pDPB.reset(new H265DBPList());
    }
    catch(...)
    {
        return UMC::UMC_ERR_ALLOC;
    }

    // save the ID
    localFrameTime = 0;
    pCurFrame = 0;

    return UMC::UMC_OK;

} // Status ViewItem_H265::Init(uint32_t view_id)

// Close the view and release all resources
void ViewItem_H265::Close(void)
{
    // Reset the parameters before close
    Reset();

    if (pDPB.get())
    {
        pDPB.reset();
    }

    dpbSize = 0;
    sps_max_dec_pic_buffering = 1;
    sps_max_num_reorder_pics = 0;

} // void ViewItem_H265::Close(void)

// Reset the view and reset all resource
void ViewItem_H265::Reset(void)
{
    if (pDPB.get())
    {
        pDPB->Reset();
    }

    pCurFrame = 0;
    localFrameTime = 0;
    sps_max_dec_pic_buffering = 1;
    sps_max_num_reorder_pics = 0;
    dpbSize = 0;

} // void ViewItem_H265::Reset(void)

// Reset the size of DPB for particular view item
void ViewItem_H265::SetDPBSize(H265SeqParamSet *pSps, uint32_t & level_idc)
{
    uint32_t level = level_idc ? level_idc : pSps->m_pcPTL.GetGeneralPTL()->level_idc;

    // calculate the new DPB size value

    // FIXME: should have correct temporal layer

    dpbSize = pSps->sps_max_dec_pic_buffering[pSps->sps_max_sub_layers-1];

    if (level_idc)
    {
        level_idc = level;
    }
    else
    {
        pSps->m_pcPTL.GetGeneralPTL()->level_idc = level;
    }

    // provide the new value to the DPBList
    if (pDPB.get())
    {
        pDPB->SetDPBSize(dpbSize);
    }

} // void ViewItem_H265::SetDPBSize(const H265SeqParamSet *pSps)
/****************************************************************************************************/
// TaskSupplier_H265
/****************************************************************************************************/
TaskSupplier_H265::TaskSupplier_H265()
    : m_SliceIdxInTaskSupplier(0)
    , m_pSegmentDecoder(0)
    , m_iThreadNum(0)
    , m_maxUIDWhenWasDisplayed(0)
    , m_local_delta_frame_time(0)
    , m_use_external_framerate(false)
    , m_decodedOrder(false)
    , m_checkCRAInsideResetProcess(false)
    , m_bFirstSliceInSequence(true)
    , m_pLastSlice(0)
    , m_pLastDisplayed(0)
    , m_pMemoryAllocator(0)
    , m_pFrameAllocator(0)
    , m_WaitForIDR(false)
    , m_prevSliceBroken(false)
    , m_RA_POC(0)
    , NoRaslOutputFlag(0)
    , m_IRAPType(NAL_UT_INVALID)
    , m_DPBSizeEx(0)
    , m_frameOrder(0)
    , m_pTaskBroker(0)
    , m_UIDFrameCounter(0)
    , m_sei_messages(0)
    , m_isInitialized(false)
{
}

TaskSupplier_H265::~TaskSupplier_H265()
{
    Close();
}

// Initialize task supplier and creak task broker
UMC::Status TaskSupplier_H265::Init(UMC::VideoDecoderParams *init)
{
    if (NULL == init)
        return UMC::UMC_ERR_NULL_PTR;

    Close();

    m_DPBSizeEx = 0;

    m_initializationParams = *init;

    int32_t nAllowedThreadNumber = init->numThreads;
    if(nAllowedThreadNumber < 0) nAllowedThreadNumber = 0;

    // calculate number of slice decoders.
    // It should be equal to CPU number
    m_iThreadNum = (0 == nAllowedThreadNumber) ? (vm_sys_info_get_cpu_num()) : (nAllowedThreadNumber);

    AU_Splitter_H265::Init(init);
    MVC_Extension::Init();

    // create slice decoder(s)
    m_pSegmentDecoder = new H265SegmentDecoderBase *[m_iThreadNum];
    memset(m_pSegmentDecoder, 0, sizeof(H265SegmentDecoderBase *) * m_iThreadNum);

    CreateTaskBroker();
    m_pTaskBroker->Init(m_iThreadNum);

    for (uint32_t i = 0; i < m_iThreadNum; i += 1)
    {
        if (UMC::UMC_OK != m_pSegmentDecoder[i]->Init(i))
            return UMC::UMC_ERR_INIT;
    }

    m_local_delta_frame_time = 1.0/30;
    m_frameOrder = 0;
    m_use_external_framerate = 0 < init->info.framerate;

    if (m_use_external_framerate)
    {
        m_local_delta_frame_time = 1 / init->info.framerate;
    }

    m_DPBSizeEx = m_iThreadNum;

    m_isInitialized = true;

    return UMC::UMC_OK;
}


void TaskSupplier_H265::CreateTaskBroker()
{

}

// Initialize what is necessary to decode bitstream header before the main part is initialized
UMC::Status TaskSupplier_H265::PreInit(UMC::VideoDecoderParams *init)
{
    if (m_isInitialized)
        return UMC::UMC_OK;

    if (NULL == init)
        return UMC::UMC_ERR_NULL_PTR;

    Close();

    m_DPBSizeEx = 0;

    MVC_Extension::Init();

    int32_t nAllowedThreadNumber = init->numThreads;
    if(nAllowedThreadNumber < 0) nAllowedThreadNumber = 0;

    // calculate number of slice decoders.
    // It should be equal to CPU number
    m_iThreadNum = (0 == nAllowedThreadNumber) ? (vm_sys_info_get_cpu_num()) : (nAllowedThreadNumber);

    AU_Splitter_H265::Init(init);

    m_local_delta_frame_time = 1.0/30;
    m_frameOrder             = 0;
    m_use_external_framerate = 0 < init->info.framerate;

    if (m_use_external_framerate)
    {
        m_local_delta_frame_time = 1 / init->info.framerate;
    }

    m_DPBSizeEx = m_iThreadNum;

    return UMC::UMC_OK;
}

// Release allocated resources
void TaskSupplier_H265::Close()
{
    if (m_pTaskBroker)
    {
        m_pTaskBroker->Release();
    }

// from reset
    if (GetView()->pDPB.get())
    {
        for (H265DecoderFrame *pFrame = GetView()->pDPB->head(); pFrame; pFrame = pFrame->future())
        {
            pFrame->FreeResources();
        }
    }

    if (m_pSegmentDecoder)
    {
        for (uint32_t i = 0; i < m_iThreadNum; i += 1)
        {
            delete m_pSegmentDecoder[i];
            m_pSegmentDecoder[i] = 0;
        }
    }

    MVC_Extension::Close();
        DecReferencePictureMarking_H265::Reset();

    if (m_pLastSlice)
    {
        m_pLastSlice->Release();
        m_ObjHeap.FreeObject(m_pLastSlice);
        m_pLastSlice = 0;
    }

    AU_Splitter_H265::Close();
    Skipping_H265::Reset();
    m_pocDecoding.Reset();

    m_frameOrder               = 0;

    m_decodedOrder      = false;
    m_checkCRAInsideResetProcess = false;
    m_WaitForIDR        = true;
    m_prevSliceBroken   = false;
    m_maxUIDWhenWasDisplayed = 0;

    m_RA_POC = 0;
    m_IRAPType = NAL_UT_INVALID;
    NoRaslOutputFlag = 1;

    m_pLastDisplayed = 0;

    delete m_sei_messages;
    m_sei_messages = 0;

// from reset
    delete[] m_pSegmentDecoder;
    m_pSegmentDecoder = 0;

    delete m_pTaskBroker;
    m_pTaskBroker = 0;

    m_iThreadNum = 0;

    m_DPBSizeEx = 1;

    m_isInitialized = false;

} // void TaskSupplier_H265::Close()

// Reset to default state
void TaskSupplier_H265::Reset()
{
    if (m_pTaskBroker)
        m_pTaskBroker->Reset();

    {
        for (H265DecoderFrame *pFrame = GetView()->pDPB->head(); pFrame; pFrame = pFrame->future())
        {
            pFrame->FreeResources();
        }
    }

    if (m_sei_messages)
        m_sei_messages->Reset();

    MVC_Extension::Reset();

    DecReferencePictureMarking_H265::Reset();

    if (m_pLastSlice)
    {
        m_pLastSlice->Release();
        m_ObjHeap.FreeObject(m_pLastSlice);
        m_pLastSlice = 0;
    }

    Skipping_H265::Reset();
    AU_Splitter_H265::Reset();
    m_pocDecoding.Reset();

    m_frameOrder               = 0;

    m_decodedOrder      = false;
    m_checkCRAInsideResetProcess = false;
    m_WaitForIDR        = true;
    m_prevSliceBroken   = false;
    m_maxUIDWhenWasDisplayed = 0;

    m_RA_POC = 0;
    m_IRAPType = NAL_UT_INVALID;
    NoRaslOutputFlag = 1;

    m_pLastDisplayed = 0;

    if (m_pTaskBroker)
        m_pTaskBroker->Init(m_iThreadNum);
}

// Attempt to recover after something unexpectedly went wrong
void TaskSupplier_H265::AfterErrorRestore()
{
    if (m_pTaskBroker)
        m_pTaskBroker->Reset();

    GetView()->pDPB->Reset();

    MVC_Extension::Reset();

    if (m_pLastSlice)
    {
        m_pLastSlice->Release();
        m_ObjHeap.FreeObject(m_pLastSlice);
        m_pLastSlice = 0;
    }

    Skipping_H265::Reset();
    AU_Splitter_H265::Reset();

    m_decodedOrder      = false;
    m_checkCRAInsideResetProcess = false;
    m_WaitForIDR        = true;
    m_prevSliceBroken   = false;
    m_maxUIDWhenWasDisplayed = 0;
    NoRaslOutputFlag = 1;

    m_pLastDisplayed = 0;

    if (m_pTaskBroker)
        m_pTaskBroker->Init(m_iThreadNum);
}

// Fill up current bitstream information
UMC::Status TaskSupplier_H265::GetInfo(UMC::VideoDecoderParams *lpInfo)
{
    H265SeqParamSet *sps = m_Headers.m_SeqParams.GetCurrentHeader();
    if (!sps)
    {
        return UMC::UMC_ERR_NOT_ENOUGH_DATA;
    }

    lpInfo->info.clip_info.height = sps->pic_height_in_luma_samples - (sps->conf_win_top_offset + sps->conf_win_bottom_offset);
    lpInfo->info.clip_info.width = sps->pic_width_in_luma_samples - (sps->conf_win_left_offset + sps->conf_win_right_offset);

    if (0.0 < m_local_delta_frame_time)
    {
        lpInfo->info.framerate = 1.0 / m_local_delta_frame_time;
    }
    else
    {
        lpInfo->info.framerate = 0;
    }

    lpInfo->info.stream_type = UMC::HEVC_VIDEO;

    lpInfo->profile = sps->m_pcPTL.GetGeneralPTL()->profile_idc;
    lpInfo->level = sps->m_pcPTL.GetGeneralPTL()->level_idc;

    lpInfo->numThreads = m_iThreadNum;
    lpInfo->info.color_format = GetUMCColorFormat_H265(sps->chroma_format_idc);

    lpInfo->info.profile = sps->m_pcPTL.GetGeneralPTL()->profile_idc;
    lpInfo->info.level = sps->m_pcPTL.GetGeneralPTL()->level_idc;

    lpInfo->info.aspect_ratio_width  = sps->sar_width;
    lpInfo->info.aspect_ratio_height = sps->sar_height;

    uint32_t multiplier = 1 << (6 + sps->getHrdParameters()->bit_rate_scale);
    lpInfo->info.bitrate = (sps->getHrdParameters()->GetHRDSubLayerParam(0)->bit_rate_value[0][0] - 1) * multiplier;

    lpInfo->info.interlace_type = UMC::PROGRESSIVE;
    return UMC::UMC_OK;
}

// Search DPB for a frame which may be reused
H265DecoderFrame *TaskSupplier_H265::GetFreeFrame()
{
    UMC::AutomaticUMCMutex guard(m_mGuard);
    H265DecoderFrame *pFrame = 0;

    ViewItem_H265 *pView = GetView();
    H265DBPList *pDPB = pView->pDPB.get();

    // Traverse list for next disposable frame
    if (pDPB->countAllFrames() >= pView->dpbSize + m_DPBSizeEx)
        pFrame = pDPB->GetOldestDisposable();

    //pDPB->printDPB();

    VM_ASSERT(!pFrame || pFrame->GetRefCounter() == 0);

    // Did we find one?
    if (NULL == pFrame)
    {
        if (pDPB->countAllFrames() >= pView->dpbSize + m_DPBSizeEx)
        {
            return 0;
        }

        // Didn't find one. Let's try to insert a new one
        pFrame = new H265DecoderFrame(m_pMemoryAllocator, &m_ObjHeap);
        if (NULL == pFrame)
            return 0;

        pDPB->append(pFrame);
    }

    pFrame->Reset();

    // Set current as not displayable (yet) and not outputted. Will be
    // updated to displayable after successful decode.
    pFrame->IncrementReference();

    m_UIDFrameCounter++;
    pFrame->m_UID = m_UIDFrameCounter;
    return pFrame;
}

// Decode SEI NAL unit
UMC::Status TaskSupplier_H265::DecodeSEI(UMC::MediaDataEx *nalUnit)
{
    if (m_Headers.m_SeqParams.GetCurrentID() == -1)
        return UMC::UMC_OK;

    H265HeadersBitstream bitStream;

    try
    {
        MemoryPiece mem;
        mem.SetData(nalUnit);

        MemoryPiece swappedMem;
        swappedMem.Allocate(nalUnit->GetDataSize() + DEFAULT_NU_TAIL_SIZE);

        SwapperBase * swapper = m_pNALSplitter->GetSwapper();
        swapper->SwapMemory(&swappedMem, &mem, 0);

        bitStream.Reset((uint8_t*)swappedMem.GetPointer(), (uint32_t)swappedMem.GetDataSize());
        bitStream.SetTailBsSize(DEFAULT_NU_TAIL_SIZE);

        NalUnitType nal_unit_type;
        uint32_t temporal_id;

        bitStream.GetNALUnitType(nal_unit_type, temporal_id);

        do
        {
            H265SEIPayLoad    m_SEIPayLoads;

            /*int32_t target_sps =*/ bitStream.ParseSEI(m_Headers.m_SeqParams,
                m_Headers.m_SeqParams.GetCurrentID(), &m_SEIPayLoads);

            if (m_SEIPayLoads.payLoadType == SEI_USER_DATA_REGISTERED_TYPE)
            {
                m_UserData = m_SEIPayLoads;
            }
            else
            {
                m_Headers.m_SEIParams.AddHeader(&m_SEIPayLoads);
            }

        } while (bitStream.More_RBSP_Data());

    } catch(...)
    {
        // nothing to do just catch it
    }

    return UMC::UMC_OK;
}

// Decode video parameters set NAL unit
UMC::Status TaskSupplier_H265::xDecodeVPS(H265HeadersBitstream *bs)
{
    H265VideoParamSet vps;

    UMC::Status s = bs->GetVideoParamSet(&vps);
    if(s == UMC::UMC_OK)
        m_Headers.m_VideoParams.AddHeader(&vps);

    return s;
}

// Decode sequence parameters set NAL unit
UMC::Status TaskSupplier_H265::xDecodeSPS(H265HeadersBitstream *bs)
{
    H265SeqParamSet sps;
    sps.Reset();

    UMC::Status s = bs->GetSequenceParamSet(&sps);
    if(s != UMC::UMC_OK)
        return s;

    if (sps.need16bitOutput && sps.m_pcPTL.GetGeneralPTL()->profile_idc == H265_PROFILE_MAIN10 && sps.bit_depth_luma == 8 && sps.bit_depth_chroma == 8 &&
        m_initializationParams.info.color_format == UMC::NV12)
        sps.need16bitOutput = 0;

    sps.WidthInCU = (sps.pic_width_in_luma_samples % sps.MaxCUSize) ? sps.pic_width_in_luma_samples / sps.MaxCUSize + 1 : sps.pic_width_in_luma_samples / sps.MaxCUSize;
    sps.HeightInCU = (sps.pic_height_in_luma_samples % sps.MaxCUSize) ? sps.pic_height_in_luma_samples / sps.MaxCUSize  + 1 : sps.pic_height_in_luma_samples / sps.MaxCUSize;
    sps.NumPartitionsInCUSize = 1 << sps.MaxCUDepth;
    sps.NumPartitionsInCU = 1 << (sps.MaxCUDepth << 1);
    sps.NumPartitionsInFrameWidth = sps.WidthInCU * sps.NumPartitionsInCUSize;

    uint8_t newDPBsize = (uint8_t)CalculateDPBSize(sps.getPTL()->GetGeneralPTL()->profile_idc, sps.getPTL()->GetGeneralPTL()->level_idc,
                                sps.pic_width_in_luma_samples,
                                sps.pic_height_in_luma_samples,
                                sps.sps_max_dec_pic_buffering[HighestTid]);

    for (uint32_t i = 0; i <= HighestTid; i++)
    {
        if (sps.sps_max_dec_pic_buffering[i] > newDPBsize)
            sps.sps_max_dec_pic_buffering[i] = newDPBsize;
    }

    HighestTid = sps.sps_max_sub_layers - 1;

    if (!sps.getPTL()->GetGeneralPTL()->level_idc && sps.sps_max_dec_pic_buffering[HighestTid])
    {
        uint32_t level_idc = levelIndexArray[0];
        for (size_t i = 0; i < sizeof(levelIndexArray)/sizeof(levelIndexArray[0]); i++)
        {
            level_idc = levelIndexArray[i];
            newDPBsize = (uint8_t)CalculateDPBSize(sps.getPTL()->GetGeneralPTL()->profile_idc, level_idc,
                                    sps.pic_width_in_luma_samples,
                                    sps.pic_height_in_luma_samples,
                                    sps.sps_max_dec_pic_buffering[HighestTid]);

            if (newDPBsize >= sps.sps_max_dec_pic_buffering[HighestTid])
                break;
        }

        sps.getPTL()->GetGeneralPTL()->level_idc = level_idc;
    }

    sps.sps_max_dec_pic_buffering[0] = sps.sps_max_dec_pic_buffering[HighestTid] ? sps.sps_max_dec_pic_buffering[HighestTid] : newDPBsize;

    const H265SeqParamSet * old_sps = m_Headers.m_SeqParams.GetCurrentHeader();
    bool newResolution = false;
    if (IsNeedSPSInvalidate(old_sps, &sps))
    {
        newResolution = true;
    }

    m_Headers.m_SeqParams.AddHeader(&sps);

    m_pNALSplitter->SetSuggestedSize(CalculateSuggestedSize(&sps));

    if (newResolution)
        return UMC::UMC_NTF_NEW_RESOLUTION;

    return UMC::UMC_OK;
}

// Decode picture parameters set NAL unit
UMC::Status TaskSupplier_H265::xDecodePPS(H265HeadersBitstream * bs)
{
    H265PicParamSet pps;
    pps.Reset();

    bs->GetPictureParamSetPart1(&pps);
    H265SeqParamSet *sps = m_Headers.m_SeqParams.GetHeader(pps.pps_seq_parameter_set_id);
    if (!sps)
        throw h265_exception(UMC::UMC_ERR_INVALID_STREAM);

    UMC::Status s = bs->GetPictureParamSetFull(&pps, sps);
    if(s != UMC::UMC_OK)
        return s;

    // additional PPS members checks:
    int32_t QpBdOffsetY = 6 * (sps->bit_depth_luma - 8);
    if (pps.init_qp < -QpBdOffsetY || pps.init_qp > 25 + 26)
        throw h265_exception(UMC::UMC_ERR_INVALID_STREAM);

    if (pps.cross_component_prediction_enabled_flag && sps->ChromaArrayType != CHROMA_FORMAT_444)
        throw h265_exception(UMC::UMC_ERR_INVALID_STREAM);

    if (pps.chroma_qp_offset_list_enabled_flag && sps->ChromaArrayType == CHROMA_FORMAT_400)
        throw h265_exception(UMC::UMC_ERR_INVALID_STREAM);

    if (pps.diff_cu_qp_delta_depth > sps->log2_max_luma_coding_block_size - sps->log2_min_luma_coding_block_size)
        throw h265_exception(UMC::UMC_ERR_INVALID_STREAM);

    if (pps.log2_parallel_merge_level > sps->log2_max_luma_coding_block_size)
        throw h265_exception(UMC::UMC_ERR_INVALID_STREAM);

    if (pps.log2_sao_offset_scale_luma > sps->bit_depth_luma - 10)
        throw h265_exception(UMC::UMC_ERR_INVALID_STREAM);

    if (pps.log2_sao_offset_scale_chroma  > sps->bit_depth_chroma - 10)
        throw h265_exception(UMC::UMC_ERR_INVALID_STREAM);

    //palette sanity checks
    if (pps.pps_num_palette_predictor_initializer)
    {
        unsigned const PaletteMaxPredSize
            = sps->palette_max_size + sps->delta_palette_max_predictor_size;
        if (pps.pps_num_palette_predictor_initializer > PaletteMaxPredSize)
            throw h265_exception(UMC::UMC_ERR_INVALID_STREAM);

        if (pps.monochrome_palette_flag != (sps->chroma_format_idc == 0))
            throw h265_exception(UMC::UMC_ERR_INVALID_STREAM);

        if (pps.luma_bit_depth_entry != sps->bit_depth_luma)
            throw h265_exception(UMC::UMC_ERR_INVALID_STREAM);

        if (!pps.monochrome_palette_flag && pps.chroma_bit_depth_entry != sps->bit_depth_chroma)
            throw h265_exception(UMC::UMC_ERR_INVALID_STREAM);
    }

    uint32_t WidthInLCU = sps->WidthInCU;
    uint32_t HeightInLCU = sps->HeightInCU;

    pps.m_CtbAddrRStoTS.resize(WidthInLCU * HeightInLCU + 1);
    pps.m_CtbAddrTStoRS.resize(WidthInLCU * HeightInLCU + 1);
    pps.m_TileIdx.resize(WidthInLCU * HeightInLCU + 1);

    // Calculate tiles rows and columns coordinates
    if (pps.tiles_enabled_flag)
    {
        uint32_t columns = pps.num_tile_columns;
        uint32_t rows = pps.num_tile_rows;

        if (columns > WidthInLCU)
            throw h265_exception(UMC::UMC_ERR_INVALID_STREAM);

        if (rows > HeightInLCU)
            throw h265_exception(UMC::UMC_ERR_INVALID_STREAM);

        if (pps.uniform_spacing_flag)
        {
            uint32_t lastDiv, i;

            pps.column_width.resize(columns);
            pps.row_height.resize(rows);

            lastDiv = 0;
            for (i = 0; i < columns; i++)
            {
                uint32_t tmp = ((i + 1) * WidthInLCU) / columns;
                pps.column_width[i] = tmp - lastDiv;
                lastDiv = tmp;
            }

            lastDiv = 0;
            for (i = 0; i < rows; i++)
            {
                uint32_t tmp = ((i + 1) * HeightInLCU) / rows;
                pps.row_height[i] = tmp - lastDiv;
                lastDiv = tmp;
            }
        }
        else
        {
            // Initialize last column/row values, all previous values were parsed from PPS header
            uint32_t i;
            uint32_t tmp = 0;

            for (i = 0; i < pps.num_tile_columns - 1; i++)
            {
                uint32_t column = pps.getColumnWidth(i);
                if (column > WidthInLCU)
                    throw h265_exception(UMC::UMC_ERR_INVALID_STREAM);
                tmp += column;
            }

            if (tmp >= WidthInLCU)
                throw h265_exception(UMC::UMC_ERR_INVALID_STREAM);

            pps.column_width[pps.num_tile_columns - 1] = WidthInLCU - tmp;

            tmp = 0;
            for (i = 0; i < pps.num_tile_rows - 1; i++)
            {
                uint32_t row = pps.getRowHeight(i);
                if (row > HeightInLCU)
                    throw h265_exception(UMC::UMC_ERR_INVALID_STREAM);
                tmp += row;
            }

            pps.row_height[pps.num_tile_rows - 1] = HeightInLCU - tmp;
            if (tmp >= HeightInLCU)
                throw h265_exception(UMC::UMC_ERR_INVALID_STREAM);
        }

        uint32_t *colBd = h265_new_array_throw<uint32_t>(columns);
        uint32_t *rowBd = h265_new_array_throw<uint32_t>(rows);

        colBd[0] = 0;
        for (uint32_t i = 0; i < pps.num_tile_columns - 1; i++)
        {
            colBd[i + 1] = colBd[i] + pps.getColumnWidth(i);
        }

        rowBd[0] = 0;
        for (uint32_t i = 0; i < pps.num_tile_rows - 1; i++)
        {
            rowBd[i + 1] = rowBd[i] + pps.getRowHeight(i);
        }

        uint32_t tbX = 0, tbY = 0;

        // Initialize CTB index raster to tile and inverse lookup tables
        for (uint32_t i = 0; i < WidthInLCU * HeightInLCU; i++)
        {
            uint32_t tileX = 0, tileY = 0, CtbAddrRStoTS;

            for (uint32_t j = 0; j < columns; j++)
            {
                if (tbX >= colBd[j])
                {
                    tileX = j;
                }
            }

            for (uint32_t j = 0; j < rows; j++)
            {
                if (tbY >= rowBd[j])
                {
                    tileY = j;
                }
            }

            CtbAddrRStoTS = WidthInLCU * rowBd[tileY] + pps.getRowHeight(tileY) * colBd[tileX];
            CtbAddrRStoTS += (tbY - rowBd[tileY]) * pps.getColumnWidth(tileX) + tbX - colBd[tileX];

            pps.m_CtbAddrRStoTS[i] = CtbAddrRStoTS;
            pps.m_TileIdx[i] = tileY * columns + tileX;

            tbX++;
            if (tbX == WidthInLCU)
            {
                tbX = 0;
                tbY++;
            }
        }

        for (uint32_t i = 0; i < WidthInLCU * HeightInLCU; i++)
        {
            uint32_t CtbAddrRStoTS = pps.m_CtbAddrRStoTS[i];
            pps.m_CtbAddrTStoRS[CtbAddrRStoTS] = i;
        }

        pps.m_CtbAddrRStoTS[WidthInLCU * HeightInLCU] = WidthInLCU * HeightInLCU;
        pps.m_CtbAddrTStoRS[WidthInLCU * HeightInLCU] = WidthInLCU * HeightInLCU;
        pps.m_TileIdx[WidthInLCU * HeightInLCU] = WidthInLCU * HeightInLCU;

        delete[] colBd;
        delete[] rowBd;
    }
    else
    {
        for (uint32_t i = 0; i < WidthInLCU * HeightInLCU + 1; i++)
        {
            pps.m_CtbAddrTStoRS[i] = i;
            pps.m_CtbAddrRStoTS[i] = i;
        }
        fill(pps.m_TileIdx.begin(), pps.m_TileIdx.end(), 0);
    }

    int32_t numberOfTiles = pps.tiles_enabled_flag ? pps.getNumTiles() : 0;

    pps.tilesInfo.resize(numberOfTiles);

    if (!numberOfTiles)
    {
        pps.tilesInfo.resize(1);
        pps.tilesInfo[0].firstCUAddr = 0;
        pps.tilesInfo[0].endCUAddr = WidthInLCU*HeightInLCU;
    }

    // Initialize tiles coordinates
    for (int32_t i = 0; i < numberOfTiles; i++)
    {
        int32_t tileY = i / pps.num_tile_columns;
        int32_t tileX = i % pps.num_tile_columns;

        int32_t startY = 0;
        int32_t startX = 0;

        for (int32_t j = 0; j < tileX; j++)
        {
            startX += pps.column_width[j];
        }

        for (int32_t j = 0; j < tileY; j++)
        {
            startY += pps.row_height[j];
        }

        pps.tilesInfo[i].endCUAddr = (startY + pps.row_height[tileY] - 1)* WidthInLCU + startX + pps.column_width[tileX] - 1;
        pps.tilesInfo[i].firstCUAddr = startY * WidthInLCU + startX;
        pps.tilesInfo[i].width = pps.column_width[tileX];
    }

    m_Headers.m_PicParams.AddHeader(&pps);

    return s;
}

// Decode a bitstream header NAL unit
UMC::Status TaskSupplier_H265::DecodeHeaders(UMC::MediaDataEx *nalUnit)
{
    //ViewItem_H265 *view = GetView(BASE_VIEW);
    UMC::Status umcRes = UMC::UMC_OK;

    H265HeadersBitstream bitStream;

    try
    {
        MemoryPiece mem;
        mem.SetData(nalUnit);

        MemoryPiece swappedMem;

        swappedMem.Allocate(nalUnit->GetDataSize() + DEFAULT_NU_TAIL_SIZE);

        SwapperBase * swapper = m_pNALSplitter->GetSwapper();
        swapper->SwapMemory(&swappedMem, &mem, 0);

        bitStream.Reset((uint8_t*)swappedMem.GetPointer(), (uint32_t)swappedMem.GetDataSize());
        bitStream.SetTailBsSize(DEFAULT_NU_TAIL_SIZE);

        NalUnitType nal_unit_type;
        uint32_t temporal_id;

        bitStream.GetNALUnitType(nal_unit_type, temporal_id);

        switch(nal_unit_type)
        {
        case NAL_UT_VPS:
            umcRes = xDecodeVPS(&bitStream);
            break;
        case NAL_UT_SPS:
            umcRes = xDecodeSPS(&bitStream);
            break;
        case NAL_UT_PPS:
            umcRes = xDecodePPS(&bitStream);
            break;
        default:
            break;
        }
    }
    catch(const h265_exception & ex)
    {
        return ex.GetStatus();
    }
    catch(...)
    {
        return UMC::UMC_ERR_INVALID_STREAM;
    }

    return umcRes;
}

// Set frame display time
void TaskSupplier_H265::PostProcessDisplayFrame(H265DecoderFrame *pFrame)
{
    if (!pFrame || pFrame->post_procces_complete)
        return;

    ViewItem_H265 &view = *GetView();

    pFrame->m_isOriginalPTS = pFrame->m_dFrameTime > -1.0;
    if (pFrame->m_isOriginalPTS)
    {
        view.localFrameTime = pFrame->m_dFrameTime;
    }
    else
    {
        pFrame->m_dFrameTime = view.localFrameTime;
    }

    pFrame->m_frameOrder = m_frameOrder;
    switch (pFrame->m_DisplayPictureStruct_H265)
    {
    case DPS_TOP_BOTTOM_TOP_H265:
    case DPS_BOTTOM_TOP_BOTTOM_H265:
        if (m_initializationParams.lFlags & UMC::FLAG_VDEC_TELECINE_PTS)
        {
            view.localFrameTime += (m_local_delta_frame_time / 2);
        }
        break;
    default:
        break;
    }

    view.localFrameTime += m_local_delta_frame_time;

    m_frameOrder++;

    pFrame->post_procces_complete = true;

    DEBUG_PRINT((VM_STRING("Outputted %s, pppp - %d\n"), GetFrameInfoString(pFrame), pppp++));

    m_maxUIDWhenWasDisplayed = std::max(m_maxUIDWhenWasDisplayed, pFrame->m_maxUIDWhenWasDisplayed);
}

// Find a next frame ready to be output from decoder
H265DecoderFrame *TaskSupplier_H265::GetFrameToDisplayInternal(bool force)
{
    ViewItem_H265 &view = *GetView();

    if (m_decodedOrder)
    {
        return view.pDPB->findOldestDisplayable(view.dpbSize);
    }

    for (;;)
    {
    // show oldest frame

    uint32_t countDisplayable = 0;
    int32_t maxUID = 0;
    uint32_t countDPBFullness = 0;

    view.pDPB->calculateInfoForDisplay(countDisplayable, countDPBFullness, maxUID);
    DEBUG_PRINT1((VM_STRING("GetAnyFrameToDisplay DPB displayable %d, maximum %d, force = %d\n"), countDisplayable, view.maxDecFrameBuffering, force));

    if (countDisplayable > view.sps_max_num_reorder_pics || countDPBFullness > view.sps_max_dec_pic_buffering || force)
    {
        H265DecoderFrame *pTmp = view.pDPB->findOldestDisplayable(view.dpbSize);

        if (pTmp)
        {
            if (!force && countDisplayable <= pTmp->GetAU()->GetSeqParam()->sps_max_num_reorder_pics[0] && countDPBFullness <= view.sps_max_dec_pic_buffering)
                return 0;

            if (!pTmp->m_pic_output)
            {
                DEBUG_PRINT((VM_STRING("Outputted skip frame %s\n"), GetFrameInfoString(pTmp)));
                pTmp->setWasDisplayed();
                pTmp->setWasOutputted();
                continue;
            }

            pTmp->m_maxUIDWhenWasDisplayed = maxUID;
            return pTmp;
        }
    }
    break;
    }

    return 0;
}

// Try to reset in case DPB has overflown
void TaskSupplier_H265::PreventDPBFullness()
{
    AfterErrorRestore();
}

// If a frame has all slices found, add it to asynchronous decode queue
UMC::Status TaskSupplier_H265::CompleteDecodedFrames(H265DecoderFrame ** decoded)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "TaskSupplier_H265::CompleteDecodedFrames");
    H265DecoderFrame* completed = 0;
    UMC::Status sts = UMC::UMC_OK;

    ViewItem_H265 &view = *GetView();
    for (;;) //add all ready to decoding
    {
        bool isOneToAdd = true;
        H265DecoderFrame * frameToAdd = 0;

        for (H265DecoderFrame * frame = view.pDPB->head(); frame; frame = frame->future())
        {
            int32_t const frm_error = frame->GetError();

            //we don't overwrite an error if we already got it
            if (sts == UMC::UMC_OK && frm_error < 0)
                //if we have ERROR_FRAME_DEVICE_FAILURE  bit is set then this error is  UMC::Status code
                sts = static_cast<UMC::Status>(frm_error);

            if (!frame->IsDecoded())
            {
                if (!frame->IsDecodingStarted() && frame->IsFullFrame())
                {
                    if (frameToAdd)
                    {
                        isOneToAdd = false;
                        if (frameToAdd->m_UID < frame->m_UID) // add first with min UID
                            continue;
                    }

                    frameToAdd = frame;
                }

                if (!frame->IsDecodingCompleted())
                {
                    continue;
                }

                DEBUG_PRINT((VM_STRING("Decode %s \n"), GetFrameInfoString(frame)));
                frame->OnDecodingCompleted();
                completed = frame;;
            }
        }

        if (sts != UMC::UMC_OK)
            break;

        if (frameToAdd)
        {
            if (!m_pTaskBroker->AddFrameToDecoding(frameToAdd))
                break;
        }

        if (isOneToAdd)
            break;
    }

    if (decoded)
        *decoded = completed;

    return sts;
}

// Add a new bitstream data buffer to decoding
UMC::Status TaskSupplier_H265::AddSource(UMC::MediaData * pSource)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "TaskSupplier_H265::AddSource");
    H265DecoderFrame* completed = 0;
    UMC::Status umcRes = CompleteDecodedFrames(&completed);
    if (umcRes != UMC::UMC_OK)
        return pSource || !completed ? umcRes : UMC::UMC_OK;

    if (GetFrameToDisplayInternal(false))
        return UMC::UMC_OK;

    umcRes = AddOneFrame(pSource); // construct frame

    if (UMC::UMC_ERR_NOT_ENOUGH_BUFFER == umcRes)
    {
        ViewItem_H265 &view = *GetView();

        int32_t count = 0;
        for (H265DecoderFrame *pFrame = view.pDPB->head(); pFrame; pFrame = pFrame->future())
        {
            count++;
            // frame is being processed. Wait for asynchronous end of operation.
            if (pFrame->isDisposable() || isInDisplayngStage(pFrame) || isAlmostDisposable(pFrame))
            {
                return UMC::UMC_WRN_INFO_NOT_READY;
            }
        }

        if (count < view.pDPB->GetDPBSize())
            return UMC::UMC_WRN_INFO_NOT_READY;

        // some more hard reasons of frame lacking.
        if (!m_pTaskBroker->IsEnoughForStartDecoding(true))
        {
            umcRes = CompleteDecodedFrames(&completed);
            if (umcRes != UMC::UMC_OK)
                return umcRes;
            else if (completed)
                return UMC::UMC_WRN_INFO_NOT_READY;

            if (GetFrameToDisplayInternal(true))
                return UMC::UMC_ERR_NEED_FORCE_OUTPUT;

            PreventDPBFullness();
            return UMC::UMC_WRN_INFO_NOT_READY;
        }
    }

    return umcRes;
}

// Choose appropriate processing action for specified NAL unit
UMC::Status TaskSupplier_H265::ProcessNalUnit(UMC::MediaDataEx *nalUnit)
{
    UMC::Status umcRes = UMC::UMC_OK;
    UMC::MediaDataEx::_MediaDataEx* pMediaDataEx = nalUnit->GetExData();
    NalUnitType unitType = (NalUnitType)pMediaDataEx->values[pMediaDataEx->index];

    switch(unitType)
    {
    case NAL_UT_CODED_SLICE_TRAIL_R:
    case NAL_UT_CODED_SLICE_TRAIL_N:
    case NAL_UT_CODED_SLICE_TLA_R:
    case NAL_UT_CODED_SLICE_TSA_N:
    case NAL_UT_CODED_SLICE_STSA_R:
    case NAL_UT_CODED_SLICE_STSA_N:
    case NAL_UT_CODED_SLICE_BLA_W_LP:
    case NAL_UT_CODED_SLICE_BLA_W_RADL:
    case NAL_UT_CODED_SLICE_BLA_N_LP:
    case NAL_UT_CODED_SLICE_IDR_W_RADL:
    case NAL_UT_CODED_SLICE_IDR_N_LP:
    case NAL_UT_CODED_SLICE_CRA:
    case NAL_UT_CODED_SLICE_RADL_R:
    case NAL_UT_CODED_SLICE_RASL_R:
        if (H265Slice * pSlice = DecodeSliceHeader(nalUnit))
            umcRes = AddSlice(pSlice, false);
        break;

    case NAL_UT_VPS:
    case NAL_UT_SPS:
    case NAL_UT_PPS:
        umcRes = DecodeHeaders(nalUnit);
        {
#if (MFX_VERSION >= 1025)
            UMC::MediaData::AuxInfo* aux = (nalUnit) ? nalUnit->GetAuxInfo(MFX_EXTBUFF_DECODE_ERROR_REPORT) : NULL;
            mfxExtDecodeErrorReport* pDecodeErrorReport = (aux) ? reinterpret_cast<mfxExtDecodeErrorReport*>(aux->ptr) : NULL;
            if (pDecodeErrorReport && umcRes == UMC::UMC_ERR_INVALID_STREAM)
                SetDecodeErrorTypes(unitType, pDecodeErrorReport);
#endif
        }

        break;

    case NAL_UT_SEI:
        umcRes = DecodeSEI(nalUnit);
        break;

    case NAL_UT_AU_DELIMITER:
        umcRes = AddSlice(0, false);
        break;

    default:
        break;
    };

    return umcRes;
}

// Find NAL units in new bitstream buffer and process them
UMC::Status TaskSupplier_H265::AddOneFrame(UMC::MediaData * pSource)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "TaskSupplier_H265::AddOneFrame");
    if (m_pLastSlice)
    {
        UMC::Status sts = AddSlice(m_pLastSlice, !pSource);
        if (sts == UMC::UMC_ERR_NOT_ENOUGH_BUFFER || sts == UMC::UMC_OK)
            return sts;
    }

    if (m_checkCRAInsideResetProcess && !pSource)
        return UMC::UMC_ERR_NOT_ENOUGH_DATA;

    size_t moveToSpsOffset = m_checkCRAInsideResetProcess ? pSource->GetDataSize() : 0;

    do
    {
        UMC::MediaDataEx *nalUnit = m_pNALSplitter->GetNalUnits(pSource);
        if (!nalUnit)
            break;

        UMC::MediaDataEx::_MediaDataEx* pMediaDataEx = nalUnit->GetExData();

#if (MFX_VERSION >= 1025)
        UMC::MediaData::AuxInfo* aux = (pSource) ? pSource->GetAuxInfo(MFX_EXTBUFF_DECODE_ERROR_REPORT) : NULL;
        mfxExtDecodeErrorReport* pDecodeErrorReport = (aux) ? reinterpret_cast<mfxExtDecodeErrorReport*>(aux->ptr) : NULL;
#endif

        for (int32_t i = 0; i < (int32_t)pMediaDataEx->count; i++, pMediaDataEx->index ++)
        {
            if (m_checkCRAInsideResetProcess)
            {
                switch ((NalUnitType)pMediaDataEx->values[i])
                {
                case NAL_UT_CODED_SLICE_RASL_N:
                case NAL_UT_CODED_SLICE_RADL_N:
                case NAL_UT_CODED_SLICE_TRAIL_R:
                case NAL_UT_CODED_SLICE_TRAIL_N:
                case NAL_UT_CODED_SLICE_TLA_R:
                case NAL_UT_CODED_SLICE_TSA_N:
                case NAL_UT_CODED_SLICE_STSA_R:
                case NAL_UT_CODED_SLICE_STSA_N:
                case NAL_UT_CODED_SLICE_BLA_W_LP:
                case NAL_UT_CODED_SLICE_BLA_W_RADL:
                case NAL_UT_CODED_SLICE_BLA_N_LP:
                case NAL_UT_CODED_SLICE_IDR_W_RADL:
                case NAL_UT_CODED_SLICE_IDR_N_LP:
                case NAL_UT_CODED_SLICE_CRA:
                case NAL_UT_CODED_SLICE_RADL_R:
                case NAL_UT_CODED_SLICE_RASL_R:
                    {
                        H265Slice * pSlice = m_ObjHeap.AllocateObject<H265Slice>();
                        pSlice->IncrementReference();

                        notifier0<H265Slice> memory_leak_preventing_slice(pSlice, &H265Slice::DecrementReference);
                        nalUnit->SetDataSize(100); // is enough for retrive

                        MemoryPiece memCopy;
                        memCopy.SetData(nalUnit);

                        pSlice->m_source.Allocate(nalUnit->GetDataSize() + DEFAULT_NU_TAIL_SIZE);

                        notifier0<MemoryPiece> memory_leak_preventing(&pSlice->m_source, &MemoryPiece::Release);
                        SwapperBase * swapper = m_pNALSplitter->GetSwapper();
                        swapper->SwapMemory(&pSlice->m_source, &memCopy, 0);

                        int32_t pps_pid = pSlice->RetrievePicParamSetNumber();
                        if (pps_pid != -1)
                            CheckCRAOrBLA(pSlice);

                        m_checkCRAInsideResetProcess = false;
                        pSource->MoveDataPointer(int32_t(pSource->GetDataSize() - moveToSpsOffset));
                        m_pNALSplitter->Reset();
                        return UMC::UMC_NTF_NEW_RESOLUTION;
                    }
                    break;

                case NAL_UT_VPS:
                case NAL_UT_SPS:
                case NAL_UT_PPS:
                    {
                        UMC::Status sts = DecodeHeaders(nalUnit);
#if (MFX_VERSION >= 1025)
                        if (pDecodeErrorReport && sts == UMC::UMC_ERR_INVALID_STREAM)
                            SetDecodeErrorTypes((NalUnitType)pMediaDataEx->values[i], pDecodeErrorReport);
#endif
                    }
                    break;

                default:
                    break;
                };

                continue;
            }

            NalUnitType nut =
                static_cast<NalUnitType>(pMediaDataEx->values[i]);
            switch (nut)
            {
            case NAL_UT_CODED_SLICE_RASL_N:
            case NAL_UT_CODED_SLICE_RADL_N:
            case NAL_UT_CODED_SLICE_TRAIL_R:
            case NAL_UT_CODED_SLICE_TRAIL_N:
            case NAL_UT_CODED_SLICE_TLA_R:
            case NAL_UT_CODED_SLICE_TSA_N:
            case NAL_UT_CODED_SLICE_STSA_R:
            case NAL_UT_CODED_SLICE_STSA_N:
            case NAL_UT_CODED_SLICE_BLA_W_LP:
            case NAL_UT_CODED_SLICE_BLA_W_RADL:
            case NAL_UT_CODED_SLICE_BLA_N_LP:
            case NAL_UT_CODED_SLICE_IDR_W_RADL:
            case NAL_UT_CODED_SLICE_IDR_N_LP:
            case NAL_UT_CODED_SLICE_CRA:
            case NAL_UT_CODED_SLICE_RADL_R:
            case NAL_UT_CODED_SLICE_RASL_R:
                if(H265Slice *pSlice = DecodeSliceHeader(nalUnit))
                {
                    UMC::Status sts = AddSlice(pSlice, !pSource);
                    if (sts == UMC::UMC_ERR_NOT_ENOUGH_BUFFER || sts == UMC::UMC_OK)
                        return sts;
                }
                break;

            case NAL_UT_SEI:
            case NAL_UT_SEI_SUFFIX:
                DecodeSEI(nalUnit);
                break;

            case NAL_UT_VPS:
            case NAL_UT_SPS:
            case NAL_UT_PPS:
                {
                    UMC::Status umsRes = DecodeHeaders(nalUnit);
                    if (umsRes != UMC::UMC_OK)
                    {
                        if (umsRes == UMC::UMC_NTF_NEW_RESOLUTION ||
                            (nut == NAL_UT_SPS && umsRes == UMC::UMC_ERR_INVALID_STREAM))
                        {

                            int32_t nalIndex = pMediaDataEx->index;
                            int32_t size = pMediaDataEx->offsets[nalIndex + 1] - pMediaDataEx->offsets[nalIndex];

                            m_checkCRAInsideResetProcess = true;

                            if (AddSlice(0, !pSource) == UMC::UMC_OK)
                            {
                                pSource->MoveDataPointer(- size - 3);
                                return UMC::UMC_OK;
                            }
                            moveToSpsOffset = pSource->GetDataSize() + size + 3;
                            continue;
                        }
#if (MFX_VERSION >= 1025)
                        if (pDecodeErrorReport && umsRes == UMC::UMC_ERR_INVALID_STREAM)
                            SetDecodeErrorTypes(nut, pDecodeErrorReport);
#endif

                        return umsRes;
                    }
                }
                break;

            case NAL_UT_AU_DELIMITER:
                if (AddSlice(0, !pSource) == UMC::UMC_OK)
                    return UMC::UMC_OK;
                break;

            case NAL_UT_EOS:
            case NAL_UT_EOB:
                m_WaitForIDR = true;
                AddSlice(0, !pSource);
                m_RA_POC = 0;
                m_IRAPType = NAL_UT_INVALID;
                GetView()->pDPB->IncreaseRefPicListResetCount(0); // for flushing DPB
                NoRaslOutputFlag = 1;
                return UMC::UMC_OK;
                break;

            default:
                break;
            };
        }

    } while ((pSource) && (MINIMAL_DATA_SIZE_H265 < pSource->GetDataSize()));

    if (pSource && m_checkCRAInsideResetProcess)
    {
        pSource->MoveDataPointer(int32_t(pSource->GetDataSize() - moveToSpsOffset));
        m_pNALSplitter->Reset();
    }

    if (!pSource)
    {
        return AddSlice(0, true);
    }
    else
    {
        uint32_t flags = pSource->GetFlags();

        if (!(flags & UMC::MediaData::FLAG_VIDEO_DATA_NOT_FULL_FRAME))
        {
            VM_ASSERT(!m_pLastSlice);
            return AddSlice(0, true);
        }
    }

    return UMC::UMC_ERR_NOT_ENOUGH_DATA;
}

// Decode slice header start, set slice links to SPS and PPS and correct tile offsets table if needed
H265Slice *TaskSupplier_H265::DecodeSliceHeader(UMC::MediaDataEx *nalUnit)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "TaskSupplier_H265::DecodeSliceHeader");
    if ((0 > m_Headers.m_SeqParams.GetCurrentID()) ||
        (0 > m_Headers.m_PicParams.GetCurrentID()))
    {
        return 0;
    }

    H265Slice * pSlice = m_ObjHeap.AllocateObject<H265Slice>();
    pSlice->IncrementReference();

    notifier0<H265Slice> memory_leak_preventing_slice(pSlice, &H265Slice::DecrementReference);

    MemoryPiece memCopy;
    memCopy.SetData(nalUnit);

    pSlice->m_source.Allocate(nalUnit->GetDataSize() + DEFAULT_NU_TAIL_SIZE);

    notifier0<MemoryPiece> memory_leak_preventing(&pSlice->m_source, &MemoryPiece::Release);

    std::vector<uint32_t> removed_offsets(0);
    SwapperBase * swapper = m_pNALSplitter->GetSwapper();
    swapper->SwapMemory(&pSlice->m_source, &memCopy, &removed_offsets);

    int32_t pps_pid = pSlice->RetrievePicParamSetNumber();
    if (pps_pid == -1)
    {
        return 0;
    }

    pSlice->SetPicParam(m_Headers.m_PicParams.GetHeader(pps_pid));
    H265PicParamSet const* pps = pSlice->GetPicParam();
    if (!pps)
    {
        return 0;
    }

    int32_t seq_parameter_set_id = pps->pps_seq_parameter_set_id;

    pSlice->SetSeqParam(m_Headers.m_SeqParams.GetHeader(seq_parameter_set_id));
    if (!pSlice->GetSeqParam())
    {
        return 0;
    }

    // do not need vps
    //H265VideoParamSet * vps = m_Headers.m_VideoParams.GetHeader(pSlice->GetSeqParam()->sps_video_parameter_set_id);

    m_Headers.m_SeqParams.SetCurrentID(pps->pps_seq_parameter_set_id);
    m_Headers.m_PicParams.SetCurrentID(pps->pps_pic_parameter_set_id);

    // calculate max entrypoints number
    H265SeqParamSet const* sps = pSlice->GetSeqParam();
    if (pps->entropy_coding_sync_enabled_flag)
    {
        int NumOfMaxEntryPoints = 0;
        int PicHeightInCtbsY = sps->HeightInCU;

        if (pps->tiles_enabled_flag == 0)
            NumOfMaxEntryPoints = PicHeightInCtbsY;
        else
            NumOfMaxEntryPoints = PicHeightInCtbsY * (pps->num_tile_columns);

        //reallocate memory for slice header
        if (NumOfMaxEntryPoints > DEFAULT_MAX_ENETRY_POINT_NUM)
        {
            //offset_len_minus1[0-31], assume maximum 31[4 bytes]
            int newsize = 2048 + NumOfMaxEntryPoints * 4 + DEFAULT_NU_TAIL_SIZE;
            pSlice->m_source.Allocate(newsize);
            nalUnit->SetDataSize(newsize);

            //swap buffer again, since buffer is small at first time
            removed_offsets.clear();
            memCopy.SetData(nalUnit);
            swapper->SwapMemory(&pSlice->m_source, &memCopy, &removed_offsets);
        }
    }

    pSlice->m_pCurrentFrame = NULL;

    memory_leak_preventing.ClearNotification();

    bool ready = pSlice->Reset(&m_pocDecoding);
    if (!ready)
    {
        m_prevSliceBroken = pSlice->IsError();
        return 0;
    }

    H265SliceHeader * sliceHdr = pSlice->GetSliceHeader();
    VM_ASSERT(sliceHdr);

    if (m_prevSliceBroken && sliceHdr->dependent_slice_segment_flag)
    {
        //Prev. slice contains errors. There is no relayable way to infer parameters for dependent slice
        return 0;
    }

    m_prevSliceBroken = false;

    if (m_WaitForIDR)
    {
        if (pps->pps_curr_pic_ref_enabled_flag)
        {
            ReferencePictureSet const* rps = pSlice->getRPS();
            VM_ASSERT(rps);

            uint32_t const numPicTotalCurr = rps->getNumberOfUsedPictures();
            if (numPicTotalCurr)
                return 0;
        }
        else if (sliceHdr->slice_type != I_SLICE)
        {
            return 0;
        }
    }

    ActivateHeaders(const_cast<H265SeqParamSet *>(pSlice->GetSeqParam()), const_cast<H265PicParamSet *>(pps));

    uint32_t currOffset = sliceHdr->m_HeaderBitstreamOffset;
    uint32_t currOffsetWithEmul = currOffset;

    uint32_t headersEmuls = 0;
    for (; headersEmuls < removed_offsets.size(); headersEmuls++)
    {
        if (removed_offsets[headersEmuls] < currOffsetWithEmul)
            currOffsetWithEmul++;
        else
            break;
    }

    pSlice->m_NumEmuPrevnBytesInSliceHdr = headersEmuls;

    // Update entry points
    size_t offsets = removed_offsets.size();
    if ((pSlice->GetPicParam()->tiles_enabled_flag || pSlice->GetPicParam()->entropy_coding_sync_enabled_flag) &&
        pSlice->getTileLocationCount() > 0 && offsets > 0)
    {
        uint32_t removed_bytes = 0;
        std::vector<uint32_t>::iterator it = removed_offsets.begin();
        uint32_t emul_offset = *it;

        for (uint32_t tile = 0; tile < pSlice->getTileLocationCount(); tile++)
        {
            while ((pSlice->m_tileByteLocation[tile] + currOffsetWithEmul >= emul_offset) && removed_bytes < offsets)
            {
                removed_bytes++;
                if (removed_bytes < offsets)
                {
                    it++;
                    emul_offset = *it;
                }
                else
                    break;
            }

            // 1st tile start offset is length of slice header, it should not be corrected because it is
            // not specified in slice header, it is calculated by parsing a modified bitstream instead
            if (0 == tile)
            {
                offsets -= removed_bytes;
                removed_bytes = 0;
            }
        }
    }

    for (uint32_t tile = 0; tile < pSlice->getTileLocationCount(); tile++)
    {
        pSlice->m_tileByteLocation[tile] = pSlice->m_tileByteLocation[tile] + currOffset;
    }

    m_WaitForIDR = false;
    memory_leak_preventing_slice.ClearNotification();

    //for SliceIdx m_SliceIdx m_iNumber
    pSlice->m_iNumber = m_SliceIdxInTaskSupplier;
    m_SliceIdxInTaskSupplier++;
    return pSlice;
}

// Initialize scaling list data if needed
void TaskSupplier_H265::ActivateHeaders(H265SeqParamSet *sps, H265PicParamSet *pps)
{
    for (uint32_t i = 0; i < sps->MaxCUDepth - sps->AddCUDepth; i++)
    {
        sps->m_AMPAcc[i] = sps->amp_enabled_flag;
    }
    for (uint32_t i = sps->MaxCUDepth - sps->AddCUDepth; i < sps->MaxCUDepth; i++)
    {
        sps->m_AMPAcc[i] = 0;
    }

    if (sps->scaling_list_enabled_flag)
    {
        if (pps->pps_scaling_list_data_present_flag)
        {
            if (!pps->getScalingList()->is_initialized())
            {
                pps->getScalingList()->init();
                pps->getScalingList()->calculateDequantCoef();
            }
        }
        else if (sps->sps_scaling_list_data_present_flag)
        {
            if (!sps->getScalingList()->is_initialized())
            {
                sps->getScalingList()->init();
                sps->getScalingList()->calculateDequantCoef();
            }
        }
        else
        {
            if (!pps->getScalingList()->is_initialized())
            {
                pps->getScalingList()->initFromDefaultScalingList();
            }
        }
    }
}

// Calculate NoRaslOutputFlag flag for specified slice
void TaskSupplier_H265::CheckCRAOrBLA(const H265Slice *pSlice)
{
    uint8_t no_output_of_prior_pics_flag = pSlice->GetSliceHeader()->no_output_of_prior_pics_flag;

    if (pSlice->GetRapPicFlag())
    {
        if ((pSlice->GetSliceHeader()->nal_unit_type >= NAL_UT_CODED_SLICE_BLA_W_LP &&
             pSlice->GetSliceHeader()->nal_unit_type <= NAL_UT_CODED_SLICE_IDR_N_LP) ||
            (pSlice->GetSliceHeader()->nal_unit_type == NAL_UT_CODED_SLICE_CRA && m_bFirstSliceInSequence))
        {
            NoRaslOutputFlag = true;
        }

        if (pSlice->m_SliceHeader.nal_unit_type == NAL_UT_CODED_SLICE_CRA && m_IRAPType != NAL_UT_INVALID)
        {
            NoRaslOutputFlag = 0;
        }

        if (NoRaslOutputFlag)
        {
            m_RA_POC = pSlice->m_SliceHeader.m_poc;
        }

        //the inference for NoOutputPriorPicsFlag
        if (!m_bFirstSliceInSequence && NoRaslOutputFlag)
        {
            if (pSlice->GetSliceHeader()->nal_unit_type == NAL_UT_CODED_SLICE_CRA)
            {
                no_output_of_prior_pics_flag = true;
            }
        }
        else
        {
            no_output_of_prior_pics_flag = false;
        }

        m_IRAPType = pSlice->m_SliceHeader.nal_unit_type;
    }

    m_bFirstSliceInSequence = false;

    //Check NoOutputPriorPics
    if (pSlice->GetRapPicFlag() && no_output_of_prior_pics_flag)
    {

        for (H265DecoderFrame *pCurr = GetView()->pDPB->head(); pCurr; pCurr = pCurr->future())
        {
            pCurr->m_pic_output = false;
            pCurr->SetisDisplayable(false);
        }
    }
}

// Check whether this slice should be skipped because of random access conditions. HEVC spec 3.111
bool TaskSupplier_H265::IsSkipForCRAorBLA(const H265Slice *pSlice)
{
    if (NoRaslOutputFlag)
    {
        if (pSlice->m_SliceHeader.m_poc == m_RA_POC)
            return false;

        if (pSlice->m_SliceHeader.m_poc < m_RA_POC &&
            (pSlice->m_SliceHeader.nal_unit_type == NAL_UT_CODED_SLICE_RASL_R || pSlice->m_SliceHeader.nal_unit_type == NAL_UT_CODED_SLICE_RASL_N))
        {
            return true;
        }

        if (pSlice->m_SliceHeader.nal_unit_type != NAL_UT_CODED_SLICE_RADL_R && pSlice->m_SliceHeader.nal_unit_type != NAL_UT_CODED_SLICE_RADL_N)
            NoRaslOutputFlag = 0;
    }

    return false;
}

// Add a new slice to frame
UMC::Status TaskSupplier_H265::AddSlice(H265Slice * pSlice, bool )
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "TaskSupplier_H265::AddSlice");
    m_pLastSlice = 0;

    if (!pSlice) // complete frame
    {
        UMC::Status umcRes = UMC::UMC_ERR_NOT_ENOUGH_DATA;

        if (NULL == GetView()->pCurFrame)
        {
            return umcRes;
        }

        CompleteFrame(GetView()->pCurFrame);

        OnFullFrame(GetView()->pCurFrame);
        GetView()->pCurFrame = NULL;
        umcRes = UMC::UMC_OK;
        return umcRes;
    }

    ViewItem_H265 &view = *GetView();
    H265DecoderFrame * pFrame = view.pCurFrame;

    if (pFrame)
    {
        H265Slice *firstSlice = pFrame->GetAU()->GetSlice(0);
        VM_ASSERT(firstSlice);

        if (pSlice->GetSliceHeader()->dependent_slice_segment_flag)
        {
            H265Slice *pLastFrameSlice = pFrame->GetAU()->GetSlice(pFrame->GetAU()->GetSliceCount() - 1);
            pSlice->CopyFromBaseSlice(pLastFrameSlice);
        }

        // Accord. to ITU-T H.265 7.4.2.4 Any SPS/PPS w/ id equal to active one
        // shall have the same content, unless it follows the last VCL NAL of the coded picture
        // and precedes the first VCL NAL unit of another coded picture
        // if the slices belong to different AUs or SPS/PPS was changed,
        // close the current AU and start new one.

        const H265SeqParamSet * sps = m_Headers.m_SeqParams.GetHeader(pSlice->GetSeqParam()->GetID());
        const H265PicParamSet * pps = m_Headers.m_PicParams.GetHeader(pSlice->GetPicParam()->GetID());

        if (!sps || !pps) // undefined behavior
            return UMC::UMC_ERR_FAILED;

        bool changed =
            sps->m_changed ||
            pps->m_changed ||
            !IsPictureTheSame(firstSlice, pSlice);

        if (changed)
        {
            CompleteFrame(view.pCurFrame);
            OnFullFrame(view.pCurFrame);
            view.pCurFrame = NULL;
            m_pLastSlice = pSlice;
            return UMC::UMC_OK;
        }
    }
    // there is no free frames.
    // try to allocate a new frame.
    else
    {
        H265SeqParamSet * sps = m_Headers.m_SeqParams.GetHeader(pSlice->GetSeqParam()->GetID());
        H265PicParamSet * pps = m_Headers.m_PicParams.GetHeader(pSlice->GetPicParam()->GetID());

        if (!sps || !pps) // undefined behavior
            return UMC::UMC_ERR_FAILED;

        // clear change flags when get first VCL NAL
        sps->m_changed = false;
        pps->m_changed = false;

        // allocate a new frame, initialize it with slice's parameters.
        pFrame = AllocateNewFrame(pSlice);
        if (!pFrame)
        {
            view.pCurFrame = NULL;
            m_pLastSlice = pSlice;
            return UMC::UMC_ERR_NOT_ENOUGH_BUFFER;
        }

        // set the current being processed frame
        view.pCurFrame = pFrame;
    }

    // add the next slice to the initialized frame.
    pSlice->m_pCurrentFrame = pFrame;
    AddSliceToFrame(pFrame, pSlice);

    if (pSlice->m_SliceHeader.slice_type != I_SLICE)
    {
        uint32_t NumShortTermRefs = 0, NumLongTermRefs = 0;
        view.pDPB->countActiveRefs(NumShortTermRefs, NumLongTermRefs);

        if (NumShortTermRefs + NumLongTermRefs == 0)
            AddFakeReferenceFrame(pSlice);
    }

    H265PicParamSet const* pps = pSlice->GetPicParam();
    VM_ASSERT(pps);

    H265DecoderFrame* curr_ref = pps->pps_curr_pic_ref_enabled_flag ?
        AddSelfReferenceFrame(pSlice) : nullptr;

    // Set reference list
    pSlice->UpdateReferenceList(GetView()->pDPB.get(), curr_ref);

    return UMC::UMC_ERR_NOT_ENOUGH_DATA;
}

// Not implemented
void TaskSupplier_H265::AddFakeReferenceFrame(H265Slice *)
{
// need to add absent ref frame logic
}

H265DecoderFrame* TaskSupplier_H265::AddSelfReferenceFrame(H265Slice* slice)
{
    VM_ASSERT(slice);

    return
        slice->GetCurrentFrame();
}


// Mark frame as full with slices
void TaskSupplier_H265::OnFullFrame(H265DecoderFrame * pFrame)
{
    pFrame->SetFullFrame(true);

    if (!pFrame->GetAU()->GetSlice(0)) // seems that it was skipped and slices was dropped
        return;

    pFrame->SetisDisplayable(pFrame->GetAU()->GetSlice(0)->GetSliceHeader()->pic_output_flag != 0);

    if (pFrame->GetAU()->GetSlice(0)->GetSliceHeader()->IdrPicFlag && !(pFrame->GetError() & UMC::ERROR_FRAME_DPB))
    {
        DecReferencePictureMarking_H265::ResetError();
    }

    if (DecReferencePictureMarking_H265::GetDPBError())
    {
        pFrame->SetErrorFlagged(UMC::ERROR_FRAME_DPB);
    }
}

// Check whether all slices for the frame were found
void TaskSupplier_H265::CompleteFrame(H265DecoderFrame * pFrame)
{
    if (!pFrame)
        return;

    H265DecoderFrameInfo * slicesInfo = pFrame->GetAU();

    if (slicesInfo->GetStatus() > H265DecoderFrameInfo::STATUS_NOT_FILLED)
        return;

    DEBUG_PRINT((VM_STRING("Complete frame POC - (%d) type - %d, count - %d, m_uid - %d, IDR - %d\n"), pFrame->m_PicOrderCnt, pFrame->m_FrameType, slicesInfo->GetSliceCount(), pFrame->m_UID, slicesInfo->GetAnySlice()->GetSliceHeader()->IdrPicFlag));

    slicesInfo->EliminateASO();
    slicesInfo->EliminateErrors();
    m_prevSliceBroken = false;

    // skipping algorithm
    const H265Slice *slice = slicesInfo->GetSlice(0);
    if (!slice || IsShouldSkipFrame(pFrame) || IsSkipForCRAorBLA(slice))
    {
        slicesInfo->SetStatus(H265DecoderFrameInfo::STATUS_COMPLETED);

        pFrame->SetisShortTermRef(false);
        pFrame->SetisLongTermRef(false);
        pFrame->OnDecodingCompleted();
        pFrame->SetisDisplayable(false);
        pFrame->m_pic_output = false;
        DEBUG_PRINT((VM_STRING("Skip frame ForCRAorBLA - %s\n"), GetFrameInfoString(pFrame)));
        return;
    }

    slicesInfo->SetStatus(H265DecoderFrameInfo::STATUS_FILLED);
}

// Initialize just allocated frame with slice parameters
UMC::Status TaskSupplier_H265::InitFreeFrame(H265DecoderFrame * pFrame, const H265Slice *pSlice)
{
    UMC::Status umcRes = UMC::UMC_OK;
    const H265SeqParamSet *pSeqParam = pSlice->GetSeqParam();

//    int32_t iMBCount = pSeqParam->frame_width_in_mbs * pSeqParam->frame_height_in_mbs;
    //int32_t iCUCount = pSeqParam->WidthInCU * pSeqParam->HeightInCU;
    //pFrame->m_CodingData->m_NumCUsInFrame = iCUCount;

    pFrame->m_FrameType   = SliceTypeToFrameType(pSlice->GetSliceHeader()->slice_type);
    pFrame->m_dFrameTime  = pSlice->m_source.GetTime();
    pFrame->m_crop_left   = pSeqParam->conf_win_left_offset + pSeqParam->def_disp_win_left_offset;
    pFrame->m_crop_right  = pSeqParam->conf_win_right_offset + pSeqParam->def_disp_win_right_offset;
    pFrame->m_crop_top    = pSeqParam->conf_win_top_offset + pSeqParam->def_disp_win_top_offset;
    pFrame->m_crop_bottom = pSeqParam->conf_win_bottom_offset + pSeqParam->def_disp_win_bottom_offset;
    pFrame->m_crop_flag   = pSeqParam->conformance_window_flag;

    pFrame->m_aspect_width  = pSeqParam->sar_width;
    pFrame->m_aspect_height = pSeqParam->sar_height;

    int32_t chroma_format_idc = pSeqParam->chroma_format_idc;

    uint8_t bit_depth_luma, bit_depth_chroma;
    bit_depth_luma = (uint8_t) pSeqParam->bit_depth_luma;
    bit_depth_chroma = (uint8_t) pSeqParam->bit_depth_chroma;

    int32_t bit_depth = std::max(bit_depth_luma, bit_depth_chroma);

//    int32_t iMBWidth = pSeqParam->frame_width_in_mbs;
    //int32_t iCUWidth = pSeqParam->WidthInCU;
//    int32_t iMBHeight = pSeqParam->frame_height_in_mbs;
    //int32_t iCUHeight = pSeqParam->HeightInCU;
//    mfxSize dimensions = {iMBWidth * 16, iMBHeight * 16};
    //mfxSize dimensions = {iCUWidth * pSeqParam->MaxCUWidth, iCUHeight * pSeqParam->MaxCUHeight};
    mfxSize dimensions = {static_cast<int>(pSeqParam->pic_width_in_luma_samples), static_cast<int>(pSeqParam->pic_height_in_luma_samples)};

    UMC::ColorFormat cf = GetUMCColorFormat_H265(chroma_format_idc);

    if (cf == UMC::YUV420) //  msdk !!!
        cf = UMC::NV12;

    UMC::VideoDataInfo info;
    info.Init(dimensions.width, dimensions.height, cf, bit_depth);

    pFrame->Init(&info);

    return umcRes;
}

// Allocate frame internals
UMC::Status TaskSupplier_H265::AllocateFrameData(H265DecoderFrame * pFrame, mfxSize dimensions, const H265SeqParamSet* pSeqParamSet, const H265PicParamSet *pPicParamSet)
{
    UMC::ColorFormat color_format = pFrame->GetColorFormat();
        //(ColorFormat) pSeqParamSet->chroma_format_idc;
    UMC::VideoDataInfo info;
    int32_t bit_depth = pSeqParamSet->need16bitOutput ? 10 : 8;
    info.Init(dimensions.width, dimensions.height, color_format, bit_depth);

    UMC::FrameMemID frmMID;
    UMC::Status sts = m_pFrameAllocator->Alloc(&frmMID, &info, 0);

    if (sts != UMC::UMC_OK)
    {
        throw h265_exception(UMC::UMC_ERR_ALLOC);
    }

    const UMC::FrameData *frmData = m_pFrameAllocator->Lock(frmMID);

    if (!frmData)
        throw h265_exception(UMC::UMC_ERR_LOCK);

    pFrame->allocate(frmData, &info);
    pFrame->m_index = frmMID;

    (void)pPicParamSet;

    return UMC::UMC_OK;
}

// Try to find a reusable frame or allocate a new one and initialize it with slice parameters
H265DecoderFrame * TaskSupplier_H265::AllocateNewFrame(const H265Slice *pSlice)
{
    H265DecoderFrame *pFrame = NULL;

    if (!pSlice)
    {
        return NULL;
    }

    ViewItem_H265 &view = *GetView();
    view.SetDPBSize(const_cast<H265SeqParamSet*>(pSlice->GetSeqParam()), m_level_idc);
    view.sps_max_dec_pic_buffering = pSlice->GetSeqParam()->sps_max_dec_pic_buffering[pSlice->GetSliceHeader()->nuh_temporal_id] ?
                                    pSlice->GetSeqParam()->sps_max_dec_pic_buffering[pSlice->GetSliceHeader()->nuh_temporal_id] :
                                    view.dpbSize;

    view.sps_max_num_reorder_pics = std::min(pSlice->GetSeqParam()->sps_max_num_reorder_pics[HighestTid], view.sps_max_dec_pic_buffering);

    DPBUpdate(pSlice);

    pFrame = GetFreeFrame();
    if (NULL == pFrame)
    {
        return NULL;
    }

    CheckCRAOrBLA(pSlice);

    pFrame->m_pic_output = pSlice->GetSliceHeader()->pic_output_flag != 0;
    pFrame->SetisShortTermRef(true);

    UMC::Status umcRes = InitFreeFrame(pFrame, pSlice);
    if (umcRes != UMC::UMC_OK)
    {
        return 0;
    }

    umcRes = AllocateFrameData(pFrame, pFrame->lumaSize(), pSlice->GetSeqParam(), pSlice->GetPicParam());
    if (umcRes != UMC::UMC_OK)
    {
        return 0;
    }

    if (m_UserData.user_data.size())
    {
        pFrame->m_UserData = m_UserData;
        m_UserData.user_data.clear();
    }

    if (m_sei_messages)
    {
        m_sei_messages->SetFrame(pFrame);
    }

    H265SEIPayLoad * pic_timing_payload = m_Headers.m_SEIParams.GetHeader(SEI_PIC_TIMING_TYPE);
    if (pic_timing_payload && pSlice->GetSeqParam()->frame_field_info_present_flag)
    {
        pFrame->m_DisplayPictureStruct_H265 = pic_timing_payload->SEI_messages.pic_timing.pic_struct;
    }
    else
    {
        pFrame->m_DisplayPictureStruct_H265 = DPS_FRAME_H265;
    }

    H265SEIPayLoad * mastering_display_payload = m_Headers.m_SEIParams.GetHeader(SEI_MASTERING_DISPLAY_COLOUR_VOLUME);
    H265SEIPayLoad * content_light_payload = m_Headers.m_SEIParams.GetHeader(SEI_CONTENT_LIGHT_LEVEL_INFO);

    if (mastering_display_payload)
    {
        pFrame->m_mastering_display = *mastering_display_payload;
    }
    if (content_light_payload)
    {
        pFrame->m_content_light_level_info = *content_light_payload;
    }

    InitFrameCounter(pFrame, pSlice);
    return pFrame;

} // H265DecoderFrame * TaskSupplier_H265::AllocateNewFrame(const H265Slice *pSlice)

// Initialize frame's counter and corresponding parameters
void TaskSupplier_H265::InitFrameCounter(H265DecoderFrame * pFrame, const H265Slice *pSlice)
{
    const H265SliceHeader *sliceHeader = pSlice->GetSliceHeader();
    ViewItem_H265 &view = *GetView();

    if (sliceHeader->IdrPicFlag ||
        sliceHeader->nal_unit_type == NAL_UT_CODED_SLICE_BLA_N_LP || sliceHeader->nal_unit_type == NAL_UT_CODED_SLICE_BLA_W_RADL ||
        sliceHeader->nal_unit_type == NAL_UT_CODED_SLICE_BLA_W_LP)
    {
        view.pDPB->IncreaseRefPicListResetCount(pFrame);
    }

    pFrame->setPicOrderCnt(sliceHeader->m_poc);

    DEBUG_PRINT((VM_STRING("Init frame %s\n"), GetFrameInfoString(pFrame)));

    pFrame->InitRefPicListResetCount();

} // void TaskSupplier_H265::InitFrameCounter(H265DecoderFrame * pFrame, const H265Slice *pSlice)

// Include a new slice into a set of frame slices
void TaskSupplier_H265::AddSliceToFrame(H265DecoderFrame *pFrame, H265Slice *pSlice)
{
    if (pFrame->m_FrameType < SliceTypeToFrameType(pSlice->GetSliceHeader()->slice_type))
        pFrame->m_FrameType = SliceTypeToFrameType(pSlice->GetSliceHeader()->slice_type);

    pFrame->AddSlice(pSlice);;
}

// Update DPB contents marking frames for reuse
void TaskSupplier_H265::DPBUpdate(const H265Slice * slice)
{
    ViewItem_H265 &view = *GetView();
    DecReferencePictureMarking_H265::UpdateRefPicMarking(view, slice);
}

// Find a decoder frame instance with specified surface ID
H265DecoderFrame * TaskSupplier_H265::FindSurface(UMC::FrameMemID id)
{
    UMC::AutomaticUMCMutex guard(m_mGuard);

    H265DecoderFrame *pFrame = GetView()->pDPB->head();
    for (; pFrame; pFrame = pFrame->future())
    {
        if (pFrame->GetFrameData()->GetFrameMID() == id)
            return pFrame;
    }

    return 0;
}

// Start asynchronous decoding
UMC::Status TaskSupplier_H265::RunDecoding()
{
    UMC::Status umcRes = CompleteDecodedFrames(0);
    if (umcRes != UMC::UMC_OK)
        return umcRes;

    H265DecoderFrame *pFrame = GetView()->pDPB->head();

    for (; pFrame; pFrame = pFrame->future())
    {
        if (!pFrame->IsDecodingCompleted())
        {
            break;
        }
    }

    m_pTaskBroker->Start();

    if (!pFrame)
        return UMC::UMC_OK;

    //DEBUG_PRINT((VM_STRING("Decode POC - %d\n"), pFrame->m_PicOrderCnt[0]));

    return UMC::UMC_OK;
}

// Retrieve decoded SEI data with SEI_USER_DATA_REGISTERED_TYPE type
UMC::Status TaskSupplier_H265::GetUserData(UMC::MediaData * pUD)
{
    if(!pUD)
        return UMC::UMC_ERR_NULL_PTR;

    if (!m_pLastDisplayed)
        return UMC::UMC_ERR_NOT_ENOUGH_DATA;

    if (m_pLastDisplayed->m_UserData.user_data.size() && m_pLastDisplayed->m_UserData.payLoadSize &&
        m_pLastDisplayed->m_UserData.payLoadType == SEI_USER_DATA_REGISTERED_TYPE)
    {
        pUD->SetTime(m_pLastDisplayed->m_dFrameTime);
        pUD->SetBufferPointer(&m_pLastDisplayed->m_UserData.user_data[0],
            m_pLastDisplayed->m_UserData.payLoadSize);
        pUD->SetDataSize(m_pLastDisplayed->m_UserData.payLoadSize);
        //m_pLastDisplayed->m_UserData.Reset();
        return UMC::UMC_OK;
    }

    return UMC::UMC_ERR_NOT_ENOUGH_DATA;
}

bool TaskSupplier_H265::IsShouldSuspendDisplay()
{
    UMC::AutomaticUMCMutex guard(m_mGuard);

    if (m_iThreadNum <= 1)
        return true;

    ViewItem_H265 &view = *GetView();

    if (view.pDPB->GetDisposable() || view.pDPB->countAllFrames() < view.dpbSize + m_DPBSizeEx)
        return false;

    return true;
}

// Find an index of specified level
uint32_t GetLevelIDCIndex(uint32_t level_idc)
{
    for (uint32_t i = 0; i < sizeof(levelIndexArray)/sizeof(levelIndexArray[0]); i++)
    {
        if (levelIndexArray[i] == level_idc)
            return i;
    }

    return sizeof(levelIndexArray)/sizeof(levelIndexArray[0]) - 1;
}

// Calculate maximum DPB size based on level and resolution
int32_t CalculateDPBSize(uint32_t /*profile_idc*/, uint32_t &level_idc, int32_t width, int32_t height, uint32_t num_ref_frames)
{
    // can increase level_idc to hold num_ref_frames
    uint32_t lumaPsArray[] = { 36864, 122880, 245760, 552960, 983040, 2228224, 2228224, 8912896, 8912896, 8912896, 35651584, 35651584, 35651584 };
    uint32_t MaxDpbSize = 16;

    for (;;)
    {
        uint32_t index = GetLevelIDCIndex(level_idc);

        uint32_t MaxLumaPs = lumaPsArray[index];
        uint32_t const maxDpbPicBuf =
#ifndef MFX_VA
            profile_idc != H265_PROFILE_SCC ? 6 : 7;
#else
            6;//HW handles second version of current reference (twoVersionsOfCurrDecPicFlag) itself
#endif

        uint32_t PicSizeInSamplesY = width * height;

        if (PicSizeInSamplesY  <=  (MaxLumaPs  >>  2 ))
            MaxDpbSize = std::min(4 * maxDpbPicBuf, 16u);
        else if (PicSizeInSamplesY  <=  (MaxLumaPs  >>  1 ))
            MaxDpbSize = std::min(2 * maxDpbPicBuf, 16u);
        else if (PicSizeInSamplesY  <=  ((3 * MaxLumaPs)  >>  2 ))
            MaxDpbSize = std::min((4 * maxDpbPicBuf) / 3, 16u);
        else
            MaxDpbSize = maxDpbPicBuf;

        if (num_ref_frames <= MaxDpbSize)
            break;


        if (index >= sizeof(levelIndexArray)/sizeof(levelIndexArray[0]) - 1)
            break;

        level_idc = levelIndexArray[index + 1];

    }

    return MaxDpbSize;
}


} // namespace UMC_HEVC_DECODER
#endif // MFX_ENABLE_H265_VIDEO_DECODE
