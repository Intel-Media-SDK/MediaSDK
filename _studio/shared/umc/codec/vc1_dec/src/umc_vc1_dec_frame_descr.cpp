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

#if defined (MFX_ENABLE_VC1_VIDEO_DECODE)

#include "umc_vc1_dec_seq.h"

#include "umc_vc1_dec_task_store.h"
#include "umc_vc1_common.h"
#include "umc_vc1_dec_frame_descr.h"
#include "umc_vc1_dec_exception.h"
#include "mfx_trace.h"


using namespace UMC;
using namespace UMC::VC1Exceptions;


bool VC1FrameDescriptor::Init(uint32_t         DescriporID,
                              VC1Context*    pContext,
                              VC1TaskStore*  pStore,
                              int16_t*        pResidBuf)
{
    VC1SequenceLayerHeader* seqLayerHeader = &pContext->m_seqLayerHeader;
    uint32_t HeightMB = pContext->m_seqLayerHeader.MaxHeightMB;
    uint32_t WidthMB = pContext->m_seqLayerHeader.MaxWidthMB;

    if(seqLayerHeader->INTERLACE)
        HeightMB = HeightMB + (HeightMB & 1); //in case of field with odd height

    if (!m_pContext)
    {
        uint8_t* ptr = NULL;
        ptr += mfx::align2_value(sizeof(VC1Context));
        ptr += mfx::align2_value(sizeof(VC1PictureLayerHeader)*VC1_MAX_SLICE_NUM);
        ptr += mfx::align2_value((HeightMB*seqLayerHeader->MaxWidthMB*VC1_MAX_BITPANE_CHUNCKS));

        // Need to replace with MFX allocator
        if (m_pMemoryAllocator->Alloc(&m_iMemContextID,
                                      (size_t)ptr,
                                      UMC_ALLOC_PERSISTENT,
                                      16) != UMC_OK)
                                      return false;

        m_pContext = (VC1Context*)(m_pMemoryAllocator->Lock(m_iMemContextID));
        memset(m_pContext,0,size_t(ptr));
        m_pContext->bp_round_count = -1;
        ptr = (uint8_t*)m_pContext;

        ptr += mfx::align2_value(sizeof(VC1Context));
        m_pContext->m_picLayerHeader = (VC1PictureLayerHeader*)ptr;
        m_pContext->m_InitPicLayer = m_pContext->m_picLayerHeader;

        ptr += mfx::align2_value((sizeof(VC1PictureLayerHeader)*VC1_MAX_SLICE_NUM));
        m_pContext->m_pBitplane.m_databits = ptr;

    }
    uint32_t buffSize =  2*(HeightMB*VC1_PIXEL_IN_LUMA)*(WidthMB*VC1_PIXEL_IN_LUMA);

    //buf size should be divisible by 4
    if(buffSize & 0x00000003)
        buffSize = (buffSize&0xFFFFFFFC) + 4;

    // Need to replace with MFX allocator
    if (m_pMemoryAllocator->Alloc(&m_iInernBufferID,
                                  buffSize,
                                  UMC_ALLOC_PERSISTENT,
                                  16) != UMC_OK)
                                  return false;

    m_pContext->m_pBufferStart = (uint8_t*)m_pMemoryAllocator->Lock(m_iInernBufferID);
    memset(m_pContext->m_pBufferStart, 0, buffSize);

    // memory for diffs for each FrameDescriptor
    if (!m_pDiffMem)
    {
        if (!pResidBuf)
        {
            if(m_pMemoryAllocator->Alloc(&m_iDiffMemID,
                                         sizeof(int16_t)*WidthMB*HeightMB*8*8*6,
                                         UMC_ALLOC_PERSISTENT, 16) != UMC_OK )
            {
                return false;
            }
            m_pDiffMem = (int16_t*)m_pMemoryAllocator->Lock(m_iDiffMemID);
        }
        else
            m_pDiffMem = pResidBuf;
    }

    // Pointers to common pContext
    m_pStore = pStore;
    m_pContext->m_vlcTbl = pContext->m_vlcTbl;
    m_pContext->pRefDist = &pContext->RefDist;
    m_pContext->m_frmBuff.m_pFrames = pContext->m_frmBuff.m_pFrames;
    m_pContext->m_frmBuff.m_iDisplayIndex =  0;
    m_pContext->m_frmBuff.m_iCurrIndex    =  0;
    m_pContext->m_frmBuff.m_iPrevIndex    =  0;
    m_pContext->m_frmBuff.m_iNextIndex    =  1;
    m_pContext->m_frmBuff.m_iRangeMapIndex   =  pContext->m_frmBuff.m_iRangeMapIndex;
    m_pContext->m_frmBuff.m_iToFreeIndex = -1;

    m_pContext->m_seqLayerHeader = pContext->m_seqLayerHeader;
    m_iSelfID = DescriporID;
    return true;
}

bool VC1FrameDescriptor::SetNewSHParams(VC1Context* pContext)
{
    m_pContext->m_seqLayerHeader = pContext->m_seqLayerHeader;
    return true;
}
void VC1FrameDescriptor::Reset()
{
    m_iFrameCounter = 0;
    m_iRefFramesDst = 0;
    m_bIsReadyToLoad = true;
    m_iSelfID = 0;
    m_iRefFramesDst = 0;
    m_iBFramesDst = 0;
    m_bIsReferenceReady = false;
    m_bIsReadyToDisplay = false;
    m_bIsSkippedFrame = false;
    m_bIsReadyToProcess = false;
    m_bIsBusy = false;
}
void VC1FrameDescriptor::Release()
{
    if(m_pMemoryAllocator)
    {
        if (static_cast<int>(m_iDiffMemID) != -1)
        {
            m_pMemoryAllocator->Unlock(m_iDiffMemID);
            m_pMemoryAllocator->Free(m_iDiffMemID);
            m_iDiffMemID = (MemID)-1;
        }

        if (static_cast<int>(m_iInernBufferID) != -1)
        {
            m_pMemoryAllocator->Unlock(m_iInernBufferID);
            m_pMemoryAllocator->Free(m_iInernBufferID);
            m_iInernBufferID = (MemID)-1;
        }
        if (static_cast<int>(m_iMemContextID) != -1)
        {
            m_pMemoryAllocator->Unlock(m_iMemContextID);
            m_pMemoryAllocator->Free(m_iMemContextID);
            m_iMemContextID = (MemID)-1;
        }
    }

}

Status VC1FrameDescriptor::SetPictureIndices(uint32_t PTYPE, bool& skip)
{
    Status vc1Sts = VC1_OK;
    FrameMemID CheckIdx = 0;

    switch(PTYPE)
    {
    case VC1_I_FRAME:
    case VC1_P_FRAME:
        if (m_pStore->IsNeedSkipFrame(PTYPE))
        {
            skip = true;
            return UMC_ERR_NOT_ENOUGH_DATA;
        }

        m_pContext->m_frmBuff.m_iPrevIndex = m_pStore->GetPrevIndex();
        m_pContext->m_frmBuff.m_iNextIndex = m_pStore->GetNextIndex();

        if (m_pContext->m_frmBuff.m_iPrevIndex == -1)
        {
            CheckIdx = m_pStore->LockSurface(&m_pContext->m_frmBuff.m_iPrevIndex);
            m_pContext->m_frmBuff.m_iCurrIndex = m_pContext->m_frmBuff.m_iPrevIndex;
        }
        else if (m_pContext->m_frmBuff.m_iNextIndex == -1)
        {
            CheckIdx = m_pStore->LockSurface(&m_pContext->m_frmBuff.m_iNextIndex);
            m_pContext->m_frmBuff.m_iCurrIndex = m_pContext->m_frmBuff.m_iNextIndex;
        }
        else
        {
            m_pContext->m_frmBuff.m_iToFreeIndex = m_pContext->m_frmBuff.m_iPrevIndex;
            CheckIdx = m_pStore->LockSurface(&m_pContext->m_frmBuff.m_iPrevIndex);

            m_pContext->m_frmBuff.m_iCurrIndex = m_pContext->m_frmBuff.m_iPrevIndex;
            m_pContext->m_frmBuff.m_iPrevIndex = m_pContext->m_frmBuff.m_iNextIndex;
            m_pContext->m_frmBuff.m_iNextIndex = m_pContext->m_frmBuff.m_iCurrIndex;
        }

        // for Range mapping get prev index
        m_pContext->m_frmBuff.m_iDisplayIndex = m_pStore->GetNextIndex();
        if (-1 == m_pContext->m_frmBuff.m_iDisplayIndex)
            m_pContext->m_frmBuff.m_iDisplayIndex = m_pStore->GetPrevIndex();

        m_pStore->SetNextIndex(m_pContext->m_frmBuff.m_iNextIndex);
        m_pStore->SetCurrIndex(m_pContext->m_frmBuff.m_iCurrIndex);
        m_pStore->SetPrevIndex(m_pContext->m_frmBuff.m_iPrevIndex);

        m_pContext->m_frmBuff.m_pFrames[m_pContext->m_frmBuff.m_iCurrIndex].corrupted= 0;
        break;
    case VC1_B_FRAME:
        if (m_pStore->IsNeedSkipFrame(PTYPE))
        {
            skip = true;
            return UMC_ERR_NOT_ENOUGH_DATA;
        }
        m_pContext->m_frmBuff.m_iPrevIndex = m_pStore->GetPrevIndex();
        m_pContext->m_frmBuff.m_iNextIndex = m_pStore->GetNextIndex();
        m_pContext->m_frmBuff.m_iBFrameIndex = m_pStore->GetBFrameIndex();
        CheckIdx = m_pStore->LockSurface(&m_pContext->m_frmBuff.m_iBFrameIndex);
        m_pContext->m_frmBuff.m_iCurrIndex = m_pContext->m_frmBuff.m_iBFrameIndex;

        m_pStore->SetCurrIndex(m_pContext->m_frmBuff.m_iCurrIndex);
        m_pStore->SetBFrameIndex(m_pContext->m_frmBuff.m_iBFrameIndex);
        // possible situation in case of SKIP frames
        if (m_pContext->m_frmBuff.m_iNextIndex == -1)
        {
            m_pContext->m_frmBuff.m_iNextIndex = m_pContext->m_frmBuff.m_iPrevIndex;
            m_pContext->m_frmBuff.m_iDisplayIndex = m_pContext->m_frmBuff.m_iCurrIndex;
            m_pContext->m_frmBuff.m_pFrames[m_pContext->m_frmBuff.m_iCurrIndex].corrupted = ERROR_FRAME_MAJOR;
        }
        else
        {
            m_pContext->m_frmBuff.m_iDisplayIndex = m_pContext->m_frmBuff.m_iCurrIndex;
            m_pContext->m_frmBuff.m_pFrames[m_pContext->m_frmBuff.m_iCurrIndex].corrupted= 0;
        }

        m_pContext->m_frmBuff.m_iDisplayIndex = m_pContext->m_frmBuff.m_iCurrIndex;
        break;
    case VC1_BI_FRAME:
        if (m_pStore->IsNeedSkipFrame(PTYPE))
        {
            skip = true;
            return UMC_ERR_NOT_ENOUGH_DATA;
        }
        m_pContext->m_frmBuff.m_iPrevIndex = m_pStore->GetPrevIndex();
        m_pContext->m_frmBuff.m_iNextIndex = m_pStore->GetNextIndex();
        m_pContext->m_frmBuff.m_iBFrameIndex = m_pStore->GetBFrameIndex();
        CheckIdx = m_pStore->LockSurface(&m_pContext->m_frmBuff.m_iBFrameIndex);
        m_pContext->m_frmBuff.m_iCurrIndex = m_pContext->m_frmBuff.m_iBFrameIndex;
        m_pStore->SetCurrIndex(m_pContext->m_frmBuff.m_iCurrIndex);
        m_pStore->SetBFrameIndex(m_pContext->m_frmBuff.m_iBFrameIndex);
        m_pContext->m_frmBuff.m_iDisplayIndex = m_pContext->m_frmBuff.m_iCurrIndex;
        m_pContext->m_frmBuff.m_pFrames[m_pContext->m_frmBuff.m_iCurrIndex].corrupted= 0;
        break;
    default:
        break;
    }

    if (VC1_IS_SKIPPED(PTYPE))
    {
        m_pContext->m_frmBuff.m_iCurrIndex = m_pContext->m_frmBuff.m_iNextIndex =  m_pContext->m_frmBuff.m_iDisplayIndex = m_pStore->GetNextIndex();
        if (-1 == m_pContext->m_frmBuff.m_iCurrIndex)
            m_pContext->m_frmBuff.m_iCurrIndex = m_pStore->GetPrevIndex();
        if (-1 == m_pContext->m_frmBuff.m_iDisplayIndex)
            m_pContext->m_frmBuff.m_iDisplayIndex = m_pStore->GetPrevIndex();
        if (m_pContext->m_seqLayerHeader.RANGE_MAPY_FLAG ||
            m_pContext->m_seqLayerHeader.RANGE_MAPUV_FLAG ||
            m_pContext->m_seqLayerHeader.RANGERED)
            m_pContext->m_frmBuff.m_iRangeMapIndex = m_pStore->GetRangeMapIndex();

        m_pContext->m_frmBuff.m_iPrevIndex = m_pStore->GetPrevIndex();
        CheckIdx = m_pStore->LockSurface(&m_pContext->m_frmBuff.m_iToSkipCoping, true);
        m_pContext->m_frmBuff.m_pFrames[m_pContext->m_frmBuff.m_iCurrIndex].corrupted= 0;
     }
        
    if (-1 == CheckIdx)
        return VC1_FAIL;

    if ((VC1_P_FRAME == PTYPE) || (VC1_IS_SKIPPED(PTYPE)))
    {
        if (m_pContext->m_frmBuff.m_iPrevIndex == -1)
            return UMC_ERR_NOT_ENOUGH_DATA;
    } 
    else if (VC1_B_FRAME == PTYPE)
    {
        if ((m_pContext->m_frmBuff.m_iPrevIndex == -1)||
            (m_pContext->m_frmBuff.m_iNextIndex == -1))
            return UMC_ERR_NOT_ENOUGH_DATA;
    }

    m_pContext->m_frmBuff.m_pFrames[m_pContext->m_frmBuff.m_iCurrIndex].FCM = m_pContext->m_picLayerHeader->FCM;


    m_pContext->m_frmBuff.m_pFrames[m_pContext->m_frmBuff.m_iCurrIndex].ICFieldMask = 0;

    if (m_pContext->m_frmBuff.m_iPrevIndex > -1)
        m_pContext->PrevFCM = m_pContext->m_frmBuff.m_pFrames[m_pContext->m_frmBuff.m_iPrevIndex].FCM;
    else
        m_pContext->PrevFCM = m_pContext->m_picLayerHeader->FCM;


    if (m_pContext->m_frmBuff.m_iNextIndex > -1)
        m_pContext->NextFCM = m_pContext->m_frmBuff.m_pFrames[m_pContext->m_frmBuff.m_iNextIndex].FCM;
    else
        m_pContext->NextFCM = m_pContext->m_picLayerHeader->FCM;

    return vc1Sts;
}


#endif
