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

#include "umc_vc1_dec_task_store.h"
#include "umc_vc1_video_decoder.h"
#include "umc_frame_data.h"

#include "mfx_trace.h"

#include "umc_va_base.h"
#include "umc_vc1_dec_frame_descr_va.h"


namespace UMC
{
    VC1TaskStore::VC1TaskStore(MemoryAllocator *pMemoryAllocator):
                                                                m_iConsumerNumber(0),
                                                                m_pDescriptorQueue(nullptr),
                                                                m_iNumFramesProcessing(0),
                                                                m_iNumDSActiveinQueue(0),
                                                                m_pGuardGet(),
                                                                pMainVC1Decoder(nullptr),
                                                                m_lNextFrameCounter(1),
                                                                m_pPrefDS(nullptr),
                                                                m_iRangeMapIndex(0),
                                                                m_pMemoryAllocator(nullptr),
                                                                m_CurrIndex(-1),
                                                                m_PrevIndex(-1),
                                                                m_NextIndex(-1),
                                                                m_BFrameIndex(-1),
                                                                m_iICIndex(-1),
                                                                m_iICIndexB(-1),
                                                                m_iTSHeapID((MemID)-1),
                                                                m_pSHeap(0),
                                                                m_bIsLastFramesMode(false)
    {
        m_pMemoryAllocator = pMemoryAllocator;
    }

    VC1TaskStore::~VC1TaskStore()
    {
        uint32_t i;

        if(m_pMemoryAllocator)
        {

            if (m_pDescriptorQueue)
            {
                for (i = 0; i < m_iNumFramesProcessing; i++)
                    m_pDescriptorQueue[i]->Release();
            }

            if (static_cast<int>(m_iTSHeapID) != -1)
            {
                m_pMemoryAllocator->Unlock(m_iTSHeapID);
                m_pMemoryAllocator->Free(m_iTSHeapID);
                m_iTSHeapID = (MemID)-1;
            }

            delete m_pSHeap;
            m_pSHeap = 0;
        }
    }

   bool VC1TaskStore::Init(uint32_t iConsumerNumber,
                           uint32_t iMaxFramesInParallel,
                           VC1VideoDecoder* pVC1Decoder)
    {
        uint32_t i;
        m_iNumDSActiveinQueue = 0;
        m_iRangeMapIndex   =  iMaxFramesInParallel + VC1NUMREFFRAMES -1;
        pMainVC1Decoder = pVC1Decoder;
        m_iConsumerNumber = iConsumerNumber;
        m_iNumFramesProcessing = iMaxFramesInParallel;

        {
            // Heap Allocation
            {
                uint32_t heapSize = CalculateHeapSize();

                if (m_pMemoryAllocator->Alloc(&m_iTSHeapID,
                    heapSize,
                    UMC_ALLOC_PERSISTENT,
                    16) != UMC_OK)
                    return false;

                delete m_pSHeap;
                m_pSHeap = new VC1TSHeap((uint8_t*)m_pMemoryAllocator->Lock(m_iTSHeapID),heapSize);
            }

            m_pGuardGet.resize(m_iNumFramesProcessing);
            for (i = 0; i < m_iNumFramesProcessing; i++)
            {
                m_pGuardGet[i].reset(new std::mutex);
            }
        }

        return true;
    }

    bool VC1TaskStore::Reset()
    {
        uint32_t i = 0;

        //close
        m_bIsLastFramesMode = false;
        ResetDSQueue();

        if(m_pMemoryAllocator)
        {
            if (m_pDescriptorQueue)
            {
                for (i = 0; i < m_iNumFramesProcessing; i++)
                    m_pDescriptorQueue[i]->Release();
            }

            if (static_cast<int>(m_iTSHeapID) != -1)
            {
                m_pMemoryAllocator->Unlock(m_iTSHeapID);
                m_pMemoryAllocator->Free(m_iTSHeapID);
                m_iTSHeapID = (MemID)-1;
            }

            m_iNumDSActiveinQueue = 0;

            delete m_pSHeap;

            // Heap Allocation
            {
                uint32_t heapSize = CalculateHeapSize();

                if (m_pMemoryAllocator->Alloc(&m_iTSHeapID,
                    heapSize,
                    UMC_ALLOC_PERSISTENT,
                    16) != UMC_OK)
                    return false;

                m_pSHeap = new VC1TSHeap((uint8_t*)m_pMemoryAllocator->Lock(m_iTSHeapID), heapSize);
            }

            {
                for (i = 0; i < m_iNumFramesProcessing; i++)
                {
                    m_pGuardGet[i].reset(new std::mutex);
                }
            }
        }

        SetBFrameIndex(-1);
        SetCurrIndex(-1);
        SetRangeMapIndex(-1);
        SetPrevIndex(-1);
        SetNextIndex(-1);

        return true;
    }

    uint32_t VC1TaskStore::CalculateHeapSize()
    {
        uint32_t Size = mfx::align2_value(sizeof(VC1FrameDescriptor*)*(m_iNumFramesProcessing));

        for (uint32_t counter = 0; counter < m_iNumFramesProcessing; counter++)
        {
                if (pMainVC1Decoder->m_va)
                {
                    Size += mfx::align2_value(sizeof(VC1FrameDescriptorVA_Linux<VC1PackerLVA>));
                }
                else
                    Size += mfx::align2_value(sizeof(VC1FrameDescriptor));
        }

        return Size;
    }

    bool VC1TaskStore::CreateDSQueue(VC1Context* pContext,
                                     VideoAccelerator* va)
    {
        if (!va)
            return false;

        m_pSHeap->s_new(&m_pDescriptorQueue,m_iNumFramesProcessing);
        for (uint32_t i = 0; i < m_iNumFramesProcessing; i++)
        {
            uint8_t* pBuf;
            pBuf = m_pSHeap->s_alloc<VC1FrameDescriptorVA_Linux<VC1PackerLVA> >();
            m_pDescriptorQueue[i] = new(pBuf) VC1FrameDescriptorVA_Linux<VC1PackerLVA>(m_pMemoryAllocator,va);

            if (!m_pDescriptorQueue[i])
                return false;

            m_pDescriptorQueue[i]->Init(i,
                                        pContext,
                                        this,
                                        0);

        }
        m_pPrefDS =  m_pDescriptorQueue[0];
        return true;
    }

    bool VC1TaskStore::SetNewSHParams(VC1Context* pContext)
    {
        for (uint32_t i = 0; i < m_iNumFramesProcessing; i++)
        {
            m_pDescriptorQueue[i]->SetNewSHParams(pContext);
        }

        return true;
    }

    void VC1TaskStore::ResetDSQueue()
    {
        uint32_t i;
        for (i = 0; i < m_iNumFramesProcessing; i++)
            m_pDescriptorQueue[i]->Reset();
        m_lNextFrameCounter = 1;
    }

    VC1FrameDescriptor* VC1TaskStore::GetLastDS()
    {
        uint32_t i;
        VC1FrameDescriptor* pCurrDescriptor = m_pDescriptorQueue[0];
        for (i = 0; i < m_iNumFramesProcessing-1; i++)
        {
            if (pCurrDescriptor->m_iFrameCounter < m_pDescriptorQueue[i+1]->m_iFrameCounter)
                pCurrDescriptor = m_pDescriptorQueue[i+1];

        }
        return pCurrDescriptor;
    }

    VC1FrameDescriptor* VC1TaskStore::GetFirstDS()
    {
        uint32_t i;
        for (i = 0; i < m_iNumFramesProcessing; i++)
        {
            if (m_pDescriptorQueue[i]->m_iFrameCounter == m_lNextFrameCounter)
                return m_pDescriptorQueue[i];
        }
        return NULL;
    }

    FrameMemID VC1TaskStore::LockSurface(FrameMemID* mid, bool isSkip)
    {
        // B frames TBD
        Status sts;
        int32_t Idx;
        VideoDataInfo Info;
        uint32_t h = pMainVC1Decoder->m_pCurrentOut->GetHeight();
        uint32_t w = pMainVC1Decoder->m_pCurrentOut->GetWidth();
        Info.Init(w, h, YUV420);
        sts = pMainVC1Decoder->m_pExtFrameAllocator->Alloc(mid, &Info, 0);
        if (UMC_OK != sts)
            throw  VC1Exceptions::vc1_exception( VC1Exceptions::mem_allocation_er);
        sts = pMainVC1Decoder->m_pExtFrameAllocator->IncreaseReference(*mid);
        if (UMC_OK != sts)
            throw  VC1Exceptions::vc1_exception( VC1Exceptions::mem_allocation_er);
        Idx = LockAndAssocFreeIdx(*mid);
        if (Idx < 0)
            return Idx;

        if (!pMainVC1Decoder->m_va)
            throw  VC1Exceptions::vc1_exception(VC1Exceptions::internal_pipeline_error);

        if (!isSkip)
        {
            if (pMainVC1Decoder->m_va->m_Platform != VA_LINUX) // on Linux we call BeginFrame() inside VC1PackPicParams()
                sts = pMainVC1Decoder->m_va->BeginFrame(*mid);

            if (UMC_OK != sts)
                return -1;

        }

        *mid = Idx;
        return 0;
    }

    void VC1TaskStore::UnLockSurface(int32_t memID)
    {
        if (pMainVC1Decoder->m_va &&  memID > -1)
            pMainVC1Decoder->m_pExtFrameAllocator->DecreaseReference(memID);
    }

    int32_t VC1TaskStore::LockAndAssocFreeIdx(FrameMemID mid)
    {
        return mid;
    }

    FrameMemID   VC1TaskStore::UnLockIdx(uint32_t Idx)
    {
        return Idx;
    }

    FrameMemID  VC1TaskStore::GetIdx(uint32_t Idx)
    {
        return Idx;
    }

    FrameMemID VC1TaskStore::GetPrevIndex(void)
    {
        return m_PrevIndex;
    }
    FrameMemID VC1TaskStore::GetNextIndex(void)
    {
        return m_NextIndex;
    }
    FrameMemID VC1TaskStore::GetBFrameIndex(void)
    {
        return m_BFrameIndex;
    }
    FrameMemID VC1TaskStore::GetRangeMapIndex(void)
    {
        return m_iICIndex;
    }
    void VC1TaskStore::SetCurrIndex(FrameMemID Index)
    {
        m_CurrIndex = Index;
    }
    void VC1TaskStore::SetPrevIndex(FrameMemID Index)
    {
        m_PrevIndex = Index;
    }
    void VC1TaskStore::SetNextIndex(FrameMemID Index)
    {
        m_NextIndex = Index;
    }
    void VC1TaskStore::SetBFrameIndex(FrameMemID Index)
    {
        m_BFrameIndex = Index;
    }
    void VC1TaskStore::SetRangeMapIndex(FrameMemID Index)
    {
        m_iICIndex = Index;
    }


}// namespace UMC
#endif

