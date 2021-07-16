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

#ifndef __UMC_VC1_DEC_FRAME_DESCR_VA_H_
#define __UMC_VC1_DEC_FRAME_DESCR_VA_H_

#include "umc_va_base.h"

#if defined(UMC_VA)
#include "umc_vc1_dec_frame_descr.h"
#include "umc_vc1_dec_exception.h"
#include "umc_vc1_dec_task_store.h"


namespace UMC
{

    class VC1PackerLVA
    {
    public:
        VC1PackerLVA():m_va(NULL),
                       m_pSliceInfo(NULL),
                       m_pPicPtr(NULL),
                       m_bIsPreviousSkip(false)
         {
        };
        ~VC1PackerLVA(){}
        static void VC1PackPicParams (VC1Context* pContext,
                                      VAPictureParameterBufferVC1* ptr,
                                      VideoAccelerator*              va);
        void VC1PackPicParamsForOneSlice(VC1Context* pContext)
        {
            VC1PackPicParams(pContext,m_pPicPtr,m_va);
            if (VC1_IS_REFERENCE(pContext->m_picLayerHeader->PTYPE))
                m_bIsPreviousSkip = false;
        }

        void VC1PackOneSlice                      (VC1Context* pContext,
                                                   SliceParams* slparams,
                                                   uint32_t SliceBufIndex,
                                                   uint32_t MBOffset,
                                                   uint32_t SliceDataSize,
                                                   uint32_t StartCodeOffset,
                                                   uint32_t ChoppingType);//compatibility with Windows code

        void  VC1PackWholeSliceSM                   (VC1Context* pContext,
                                                     uint32_t MBOffset,
                                                     uint32_t SliceDataSize);
        void   VC1PackBitStreamForOneSlice          (VC1Context* pContext, uint32_t Size);

        uint32_t   VC1PackBitStream             (VC1Context* pContext,
                                                uint32_t& Size,
                                                uint8_t* pOriginalData,
                                                uint32_t OriginalSize,
                                                uint32_t ByteOffset,
                                                uint8_t& Flag_03)
        {
            return VC1PackBitStreamAdv(pContext, Size, pOriginalData, OriginalSize, ByteOffset, Flag_03);
        }

         uint32_t VC1PackBitStreamAdv (VC1Context* pContext, uint32_t& Size,uint8_t* pOriginalData,
                                   uint32_t OriginalDataSize, uint32_t ByteOffset, uint8_t& Flag_03);

        void   VC1PackBitplaneBuffers               (VC1Context* pContext);

        void   VC1SetSliceBuffer(){};
        void   VC1SetSliceParamBuffer(uint32_t* pOffsets, uint32_t* pValues);
        void   VC1SetSliceDataBuffer(int32_t size);
        void   VC1SetBitplaneBuffer(int32_t size);
        void   VC1SetPictureBuffer();

        void   VC1SetBuffersSize                    (uint32_t SliceBufIndex);
        void   VC1SetBuffersSize() {};

        void   SetVideoAccelerator                  (VideoAccelerator*  va)
        {
            if (va)
                m_va = va;
        };
        void MarkFrameAsSkip() {m_bIsPreviousSkip = true;}
        bool IsPrevFrameIsSkipped() {return m_bIsPreviousSkip;}

        uint32_t VC1GetPicHeaderSize(uint8_t* pOriginalData, size_t Offset, uint8_t& Flag_03)
        {
            uint32_t i = 0;
            uint32_t numZero = 0;
            uint8_t* ptr = pOriginalData;
            for(i = 0; i < Offset; i++)
            {
                if(*ptr == 0)
                    numZero++;
                else
                    numZero = 0;

                ptr++;

                if((numZero) == 2 && (*ptr == 0x03))
                {
                    if(*(ptr+1) < 4)
                    {
                       ptr++;
                    }
                    numZero = 0;
                }
            }

            if((numZero == 1) && (*ptr == 0) && (*(ptr+1) == 0x03) && (*(ptr+2) < 4))
            {
                Flag_03 = 1;

                if((*(ptr+2) == 0) && (*(ptr+3) == 0) && (*(ptr+4) == 0x03) && (*(ptr+5) < 4))
                    Flag_03 = 5;

                return ((uint32_t)(ptr - pOriginalData) + 1);
            }
            else if((*ptr == 0) && (*(ptr+1) == 0) && (*(ptr+2) == 0x03)&& (*(ptr+3) < 4))
            {
                Flag_03 = 2;
                return (uint32_t)(ptr - pOriginalData);
            }
            else if((*(ptr + 1) == 0) && (*(ptr+2) == 0) && (*(ptr+3) == 0x03)&& (*(ptr+4) < 4)&& (*(ptr+5) > 3))
            {
                Flag_03 = 3;
                return (uint32_t)(ptr - pOriginalData);
            }
            else if((*(ptr + 2) == 0) && (*(ptr+3) == 0) && (*(ptr+4) == 0x03)&& (*(ptr+5) < 4))
            {
                Flag_03 = 4;
                return (uint32_t)(ptr - pOriginalData);
            }
            else
            {
                Flag_03 = 0;
                return (uint32_t)(ptr - pOriginalData);
            }

        }

    private:

        // we sure about our types
        template <class T, class T1>
        static T bit_set(T value, uint32_t offset, uint32_t count, T1 in)
        {
            return (T)(value | (((1<<count) - 1)&in) << offset);
        };

        VideoAccelerator*              m_va;
        VASliceParameterBufferVC1*     m_pSliceInfo;
        VAPictureParameterBufferVC1*   m_pPicPtr;
        bool m_bIsPreviousSkip;
    };



    template <class T>
    class VC1FrameDescriptorVA: public VC1FrameDescriptor
    {
        friend class VC1TaskStore;
        friend class VC1VideoDecoder;
    public:

        // Default constructor
        VC1FrameDescriptorVA(MemoryAllocator*      pMemoryAllocator,
                             VideoAccelerator*     va):VC1FrameDescriptor(pMemoryAllocator),
                                                       m_va(va),
                                                       m_pBitstream(NULL),
                                                       m_iSliceBufIndex(0),
                                                       m_iPicBufIndex(0),
                                                       m_bIsItarativeMode(false),
                                                       m_pBufferStart(NULL)
        {
        }

        virtual bool Init                        (uint32_t         DescriporID,
                                                  VC1Context*    pContext,
                                                  VC1TaskStore*  pStore,
                                                  int16_t*)
        {
            VC1SequenceLayerHeader* seqLayerHeader = &pContext->m_seqLayerHeader;
            m_pStore = pStore;
            m_pPacker.SetVideoAccelerator(m_va);

            if (m_va->m_Profile == VC1_VLD)
            {
                uint32_t buffSize =  (seqLayerHeader->heightMB*VC1_PIXEL_IN_LUMA + 128)*
                    (seqLayerHeader->widthMB*VC1_PIXEL_IN_LUMA + 128)+
                    ((seqLayerHeader->heightMB*VC1_PIXEL_IN_CHROMA + 64)*
                    (seqLayerHeader->widthMB*VC1_PIXEL_IN_CHROMA + 64))*2;


                if(m_pContext == NULL)
                {
                    if (m_pMemoryAllocator->Alloc(&m_ipContextID,
                                                    sizeof(VC1Context),
                                                    UMC_ALLOC_PERSISTENT,
                                                    16) != UMC_OK)
                                                    return false;
                    m_pContext = (VC1Context*)(m_pMemoryAllocator->Lock(m_ipContextID));
                    if(m_pContext==NULL)
                    {
                        Release();
                        return false;
                    }
                }

                memset(m_pContext, 0, sizeof(VC1Context));


                //buf size should be divisible by 4
                if(buffSize & 0x00000003)
                    buffSize = (buffSize&0xFFFFFFFC) + 4;

                if(m_pContext->m_pBufferStart == NULL)
                {
                    if (m_pMemoryAllocator->Alloc(&m_ipBufferStartID,
                                                    buffSize*sizeof(uint8_t),
                                                    UMC_ALLOC_PERSISTENT,
                                                    16) != UMC_OK)
                                                    return false;
                    m_pContext->m_pBufferStart = (uint8_t*)(m_pMemoryAllocator->Lock(m_ipBufferStartID));
                    if(m_pContext->m_pBufferStart==NULL)
                    {
                        Release();
                        return false;
                    }
                }

                memset(m_pContext->m_pBufferStart, 0, buffSize);

                m_pContext->m_vlcTbl = pContext->m_vlcTbl;
                m_pContext->pRefDist = &pContext->RefDist;

                m_pContext->m_frmBuff.m_pFrames = pContext->m_frmBuff.m_pFrames;


                m_pContext->m_frmBuff.m_iDisplayIndex =  -1;
                m_pContext->m_frmBuff.m_iCurrIndex    =  -1;
                m_pContext->m_frmBuff.m_iPrevIndex    =  -1;
                m_pContext->m_frmBuff.m_iNextIndex    =  -1;
                m_pContext->m_frmBuff.m_iToFreeIndex = -1;
                m_pContext->m_frmBuff.m_iRangeMapIndex   =  pContext->m_frmBuff.m_iRangeMapIndex;

                m_pContext->m_seqLayerHeader = pContext->m_seqLayerHeader;

                if(m_pContext->m_picLayerHeader == NULL)
                {
                    if (m_pMemoryAllocator->Alloc(&m_iPicHeaderID,
                                                    sizeof(VC1PictureLayerHeader)*VC1_MAX_SLICE_NUM,
                                                    UMC_ALLOC_PERSISTENT,
                                                    16) != UMC_OK)
                                                    return false;
                    m_pContext->m_picLayerHeader = (VC1PictureLayerHeader*)(m_pMemoryAllocator->Lock(m_iPicHeaderID));
                    if(m_pContext->m_picLayerHeader==NULL)
                    {
                        Release();
                        return false;
                    }
                }

                m_pContext->m_InitPicLayer = m_pContext->m_picLayerHeader;

                memset(m_pContext->m_picLayerHeader, 0, sizeof(VC1PictureLayerHeader)*VC1_MAX_SLICE_NUM);

                //Bitplane pool
                if(m_pContext->m_pBitplane.m_databits == NULL)
                {
                    if (m_pMemoryAllocator->Alloc(&m_iBitplaneID,
                                                    sizeof(uint8_t)*seqLayerHeader->heightMB*
                                                    seqLayerHeader->widthMB*
                                                    VC1_MAX_BITPANE_CHUNCKS,
                                                    UMC_ALLOC_PERSISTENT,
                                                    16) != UMC_OK)
                                                    return false;
                    m_pContext->m_pBitplane.m_databits = (uint8_t*)(m_pMemoryAllocator->Lock(m_iBitplaneID));
                    if(m_pContext->m_pBitplane.m_databits==NULL)
                    {
                        Release();
                        return false;
                    }
                }

                memset(m_pContext->m_pBitplane.m_databits, 0,sizeof(uint8_t)*seqLayerHeader->heightMB*
                    seqLayerHeader->widthMB*
                    VC1_MAX_BITPANE_CHUNCKS);

                m_iSelfID = DescriporID;
            }
            return true;
        }

        virtual void Release()
        {
            if (m_va->m_Profile == VC1_VLD)
            {
                if(m_pMemoryAllocator)
                {
                    if (m_iPicHeaderID != (MemID)-1)
                    {
                        m_pMemoryAllocator->Unlock(m_iPicHeaderID);
                        m_pMemoryAllocator->Free(m_iPicHeaderID);
                        m_iPicHeaderID = (MemID)-1;
                    }
                    m_pContext->m_InitPicLayer = NULL;

                    if (m_iBitplaneID != (MemID)-1)
                    {
                        m_pMemoryAllocator->Unlock(m_iBitplaneID);
                        m_pMemoryAllocator->Free(m_iBitplaneID);
                        m_iBitplaneID = (MemID)-1;
                    }
                    m_pContext->m_pBitplane.m_databits = NULL;

                    if (m_ipBufferStartID != (MemID)-1)
                    {
                        m_pMemoryAllocator->Unlock(m_ipBufferStartID);
                        m_pMemoryAllocator->Free(m_ipBufferStartID);
                        m_ipBufferStartID = (MemID)-1;
                    }
                    m_pContext->m_pBufferStart = NULL;

                    if (m_ipContextID != (MemID)-1)
                    {
                        m_pMemoryAllocator->Unlock(m_ipContextID);
                        m_pMemoryAllocator->Free(m_ipContextID);
                        m_ipContextID = (MemID)-1;
                    }
                    m_pContext = NULL;

                }
            }
        }
        void VC1PackSlices    (uint32_t*  pOffsets,
                               uint32_t*  pValues,
                               uint8_t*   pOriginalData,
                               MediaDataEx::_MediaDataEx* pOrigStCodes)

        {
            uint32_t PicHeaderFlag;
            uint32_t* pPictHeader = m_pContext->m_bitstream.pBitstream;
            uint32_t temp_value = 0;
            uint32_t* bitstream;
            int32_t bitoffset = 31;
            uint32_t SliceSize = 0;

            uint32_t* pOriginalOffsets = pOrigStCodes->offsets;

            uint32_t StartCodeOffset = 0;

            // need in case of fields
            uint32_t* pFirstFieldStartCode = m_pContext->m_bitstream.pBitstream;
            uint32_t Offset;

            uint32_t RemBytesInSlice = 0;

            bool isSecondField = false;
            m_iSliceBufIndex = 0;
            m_iPicBufIndex = 0;

            uint32_t MBEndRowOfAlreadyExec = 0;

            SliceParams slparams;
            uint8_t  Flag_03 = 0;

            memset(&slparams,0,sizeof(SliceParams));
            slparams.MBStartRow = 0;
            slparams.MBEndRow = m_pContext->m_seqLayerHeader.heightMB;
            m_pContext->m_picLayerHeader->CurrField = 0;
            if (VC1_IS_SKIPPED(m_pContext->m_picLayerHeader->PTYPE))
            {
                m_bIsSkippedFrame = true;
                m_bIsReadyToProcess = false;
                return;
            }
            else
            {
                m_bIsSkippedFrame = false;
            }

            if (VC1_PROFILE_ADVANCED == m_pContext->m_seqLayerHeader.PROFILE)
            {

                //skip start codes till Frame Header
                while ((*pValues)&&(*pValues != 0x0D010000))
                {
                    ++pOffsets;
                    ++pValues;
                }
                DecodePicHeader(m_pContext);
            }
            else
            {
                Decode_PictureLayer(m_pContext);
            }

            m_pPacker.VC1PackPicParamsForOneSlice(m_pContext);


            if (m_pContext->m_seqLayerHeader.PROFILE != VC1_PROFILE_ADVANCED)
            {
                Offset = (uint32_t) (sizeof(uint32_t)*(m_pContext->m_bitstream.pBitstream - m_pBufferStart)); // offset in words
                SliceSize = m_pContext->m_FrameSize - VC1FHSIZE;

                {
                    uint32_t chopType = 1;
                    uint32_t BytesAlreadyExecuted = 0;
                    RemBytesInSlice  = m_pPacker.VC1PackBitStream(m_pContext,
                        SliceSize,
                        pOriginalData,
                        0,
                        false, Flag_03);
                    slparams.m_bitOffset = m_pContext->m_bitstream.bitOffset;
                    if (RemBytesInSlice)
                    {
                        uint32_t ByteFilledInSlice = SliceSize - RemBytesInSlice;
                        MBEndRowOfAlreadyExec = slparams.MBEndRow;

                        // fill slices
                        while (RemBytesInSlice) // we don't have enough buffer - execute it
                        {
                            m_pPacker.VC1PackOneSlice(m_pContext,
                                                     &slparams,
                                                     m_iSliceBufIndex,
                                                     Offset,
                                                     ByteFilledInSlice,
                                                     StartCodeOffset,
                                                     chopType);

                            m_pPacker.VC1SetBuffersSize(m_iSliceBufIndex+1);

                            m_va->Execute();
                            BytesAlreadyExecuted += ByteFilledInSlice;

                            m_pPacker.VC1SetSliceBuffer();
                            m_pPacker.VC1SetPictureBuffer();
                            StartCodeOffset = 0;
                            m_iSliceBufIndex = 0;

                            m_pPacker.VC1PackPicParamsForOneSlice(m_pContext);
                            RemBytesInSlice  = m_pPacker.VC1PackBitStream(m_pContext,
                                RemBytesInSlice,
                                pOriginalData + BytesAlreadyExecuted,
                                StartCodeOffset,
                                false, Flag_03);
                            ByteFilledInSlice = SliceSize - BytesAlreadyExecuted - RemBytesInSlice;
                            chopType = 3;
                            slparams.m_bitOffset = 0;
                            Offset = 0;
                        }

                        m_pPacker.VC1PackOneSlice(m_pContext,
                            &slparams,
                            m_iSliceBufIndex,
                            Offset,
                            SliceSize - BytesAlreadyExecuted,
                            StartCodeOffset,
                            2);

                        m_pPacker.VC1SetBuffersSize(m_iSliceBufIndex+1);

                        m_pPacker.VC1PackPicParamsForOneSlice(m_pContext);
                        if (UMC_OK != m_va->Execute())
                            throw VC1Exceptions::vc1_exception(VC1Exceptions::internal_pipeline_error);
                        StartCodeOffset = 0;
                        m_iSliceBufIndex = 0;


                    }
                    else
                    {
                        m_pPacker.VC1PackOneSlice(m_pContext,
                            &slparams,
                            m_iSliceBufIndex,
                            Offset,
                            SliceSize,
                            StartCodeOffset,
                            Flag_03);
                        m_pPacker.VC1SetBuffersSize(m_iSliceBufIndex+1);
                        if (UMC_OK != m_va->Execute())
                            throw VC1Exceptions::vc1_exception(VC1Exceptions::internal_pipeline_error);

                    }
                }
                return;
            }
            if (*(pValues+1) == 0x0B010000)
            {
                bitstream = reinterpret_cast<uint32_t*>(m_pContext->m_pBufferStart + *(pOffsets+1));
                VC1BitstreamParser::GetNBits(bitstream,bitoffset,32, temp_value);
                bitoffset = 31;
                VC1BitstreamParser::GetNBits(bitstream,bitoffset,9, slparams.MBEndRow);     //SLICE_ADDR
                m_pContext->m_picLayerHeader->is_slice = 1;
            }
            else if(*(pValues+1) == 0x0C010000)
            {
                slparams.MBEndRow = (m_pContext->m_seqLayerHeader.heightMB+1)/2;
            }


            slparams.MBRowsToDecode = slparams.MBEndRow-slparams.MBStartRow;
            slparams.MBRowsToDecode = slparams.MBEndRow-slparams.MBStartRow;
            slparams.m_bitOffset = m_pContext->m_bitstream.bitOffset;
            Offset = (uint32_t) (sizeof(uint32_t)*(m_pContext->m_bitstream.pBitstream - pPictHeader)); // offset in bytes

            if (*(pOffsets+1))
                SliceSize = *(pOriginalOffsets+1) - *pOriginalOffsets;
            else
                SliceSize = m_pContext->m_FrameSize;

            {
                uint32_t chopType = 1;
                uint32_t BytesAlreadyExecuted = 0;
                RemBytesInSlice  = m_pPacker.VC1PackBitStream(m_pContext,
                                                              SliceSize,
                                                              pOriginalData,
                                                              StartCodeOffset,
                                                              true, Flag_03);
                if (RemBytesInSlice)
                {
                    uint32_t ByteFilledInSlice = SliceSize - RemBytesInSlice;
                    MBEndRowOfAlreadyExec = slparams.MBEndRow;

                    // fill slices
                    while (RemBytesInSlice) // we don't have enough buffer - execute it
                    {
                        m_pPacker.VC1PackOneSlice(m_pContext,
                                                  &slparams,
                                                  m_iSliceBufIndex,
                                                  Offset,
                                                  ByteFilledInSlice,
                                                  StartCodeOffset,
                                                  chopType);

                        m_pPacker.VC1SetBuffersSize(m_iSliceBufIndex+1);

                        if (UMC_OK != m_va->Execute())
                            throw VC1Exceptions::vc1_exception(VC1Exceptions::internal_pipeline_error);
                        BytesAlreadyExecuted += ByteFilledInSlice;

                        m_pPacker.VC1SetSliceBuffer();
                        m_pPacker.VC1SetPictureBuffer();
                        StartCodeOffset = 0;
                        m_iSliceBufIndex = 0;

                        m_pPacker.VC1PackPicParamsForOneSlice(m_pContext);
                        RemBytesInSlice  = m_pPacker.VC1PackBitStream(m_pContext,
                                                                      RemBytesInSlice,
                                                                      pOriginalData + BytesAlreadyExecuted,
                                                                      StartCodeOffset,
                                                                      false, Flag_03);
                        ByteFilledInSlice = SliceSize - BytesAlreadyExecuted - RemBytesInSlice;
                        chopType = 3;
                        slparams.m_bitOffset = 0;
                        Offset = 0;
                    }

                    m_pPacker.VC1PackOneSlice(m_pContext,
                                             &slparams,
                                              m_iSliceBufIndex,
                                              Offset,
                                              SliceSize - BytesAlreadyExecuted,
                                              StartCodeOffset,
                                              2);

                    m_pPacker.VC1SetBuffersSize(m_iSliceBufIndex+1);

                    m_pPacker.VC1PackPicParamsForOneSlice(m_pContext);
                    if (UMC_OK != m_va->Execute())
                        throw VC1Exceptions::vc1_exception(VC1Exceptions::internal_pipeline_error);
                    StartCodeOffset = 0;
                    m_iSliceBufIndex = 0;

                    // we need receive buffers from drv for future processing
                    if (MBEndRowOfAlreadyExec != m_pContext->m_seqLayerHeader.heightMB)
                    {
                        m_pPacker.VC1SetSliceBuffer();
                        m_pPacker.VC1SetPictureBuffer();
                    }

                }
                else
                {
                    m_pPacker.VC1PackOneSlice(m_pContext,
                                              &slparams,
                                              m_iSliceBufIndex,
                                              Offset,
                                              SliceSize,
                                              StartCodeOffset,
                                              0);
                    // let execute in case of fields
                    if (*(pValues+1)== 0x0C010000)
                    {
                        // Execute first slice
                        if (m_va->m_Platform == VA_LINUX)
                            m_pPacker.VC1PackBitplaneBuffers(m_pContext);
                        m_pPacker.VC1SetBuffersSize(m_iSliceBufIndex + 1);

                        if (UMC_OK != m_va->Execute())
                            throw VC1Exceptions::vc1_exception(VC1Exceptions::internal_pipeline_error);
                        StartCodeOffset = 0;
                        m_iSliceBufIndex = 0;
                    }
                    else
                    {
                        StartCodeOffset += SliceSize - RemBytesInSlice;
                        ++m_iSliceBufIndex;
                    }
                }
            }
            ++pOffsets;
            ++pValues;
            ++pOriginalOffsets;


            while (*pOffsets)
            {
                //Fields
                if (*(pValues) == 0x0C010000)
                {
                    // Execute first slice
                    if (m_va->m_Platform == VA_LINUX)
                        m_pPacker.VC1PackBitplaneBuffers(m_pContext);

                    m_pContext->m_bitstream.pBitstream = pFirstFieldStartCode;



                    if (UMC_OK != m_va->EndFrame())
                        throw VC1Exceptions::vc1_exception(VC1Exceptions::internal_pipeline_error);
                    if (UMC_OK != m_va->BeginFrame(m_pContext->m_frmBuff.m_iCurrIndex))
                        throw VC1Exceptions::vc1_exception(VC1Exceptions::internal_pipeline_error);


                    if (*(pOffsets+1))
                        SliceSize = *(pOriginalOffsets+1) - *pOriginalOffsets;
                    else
                        SliceSize = m_pContext->m_FrameSize - *pOriginalOffsets;

                    StartCodeOffset = 0;
                    m_pPacker.VC1SetSliceBuffer();
                    m_pPacker.VC1SetPictureBuffer();
                    //m_iSliceBufIndex = 0;
                    //m_iPicBufIndex = 0;
                    isSecondField = true;
                    // set swap bitstream for parsing
                    m_pContext->m_bitstream.pBitstream = reinterpret_cast<uint32_t*>(m_pContext->m_pBufferStart + *pOffsets);
                    m_pContext->m_bitstream.pBitstream += 1; // skip start code
                    m_pContext->m_bitstream.bitOffset = 31;
                    pPictHeader = m_pContext->m_bitstream.pBitstream;

                    ++m_pContext->m_picLayerHeader;
                    *m_pContext->m_picLayerHeader = *m_pContext->m_InitPicLayer;
                    m_pContext->m_picLayerHeader->BottomField = (uint8_t)m_pContext->m_InitPicLayer->TFF;
                    m_pContext->m_picLayerHeader->PTYPE = m_pContext->m_InitPicLayer->PTypeField2;
                    m_pContext->m_picLayerHeader->CurrField = 1;

                    m_pContext->m_picLayerHeader->is_slice = 0;
                    DecodePicHeader(m_pContext);
                    slparams.MBStartRow = (m_pContext->m_seqLayerHeader.heightMB+1)/2;

                    if (*(pOffsets+1) && *(pValues+1) == 0x0B010000)
                    {
                        bitstream = reinterpret_cast<uint32_t*>(m_pContext->m_pBufferStart + *(pOffsets+1));
                        VC1BitstreamParser::GetNBits(bitstream,bitoffset,32, temp_value);
                        bitoffset = 31;
                        VC1BitstreamParser::GetNBits(bitstream,bitoffset,9, slparams.MBEndRow);
                    } else
                        slparams.MBEndRow = m_pContext->m_seqLayerHeader.heightMB;

                    m_pPacker.VC1PackPicParamsForOneSlice(m_pContext);
                    ++m_iPicBufIndex;
                    slparams.MBRowsToDecode = slparams.MBEndRow-slparams.MBStartRow;
                    slparams.m_bitOffset = m_pContext->m_bitstream.bitOffset;
                    Offset = (uint32_t) (sizeof(uint32_t)*(m_pContext->m_bitstream.pBitstream - pPictHeader)); // offset in bytes
                    slparams.MBStartRow = 0; //we should decode fields steb by steb

                    {
                        uint32_t chopType = 1;
                        uint32_t BytesAlreadyExecuted = 0;
                        RemBytesInSlice  = m_pPacker.VC1PackBitStream(m_pContext,
                            SliceSize,
                            pOriginalData + *pOriginalOffsets,
                            StartCodeOffset,
                            false, Flag_03);
                        if (RemBytesInSlice)
                        {
                            uint32_t ByteFilledInSlice = SliceSize - RemBytesInSlice;
                            MBEndRowOfAlreadyExec = slparams.MBEndRow;

                            // fill slices
                            while (RemBytesInSlice) // we don't have enough buffer - execute it
                            {
                                m_pPacker.VC1PackOneSlice(m_pContext,
                                    &slparams,
                                    m_iSliceBufIndex,
                                    Offset,
                                    ByteFilledInSlice,
                                    StartCodeOffset,
                                    chopType);

                                m_pPacker.VC1SetBuffersSize(m_iSliceBufIndex+1);

                                m_va->Execute();
                                BytesAlreadyExecuted += ByteFilledInSlice;

                                m_pPacker.VC1SetSliceBuffer();
                                m_pPacker.VC1SetPictureBuffer();
                                StartCodeOffset = 0;
                                m_iSliceBufIndex = 0;

                                m_pPacker.VC1PackPicParamsForOneSlice(m_pContext);
                                RemBytesInSlice  = m_pPacker.VC1PackBitStream(m_pContext,
                                    RemBytesInSlice,
                                    pOriginalData + BytesAlreadyExecuted,
                                    StartCodeOffset,
                                    false, Flag_03);
                                ByteFilledInSlice = SliceSize - BytesAlreadyExecuted - RemBytesInSlice;
                                chopType = 3;
                                slparams.m_bitOffset = 0;
                                Offset = 0;
                            }

                            m_pPacker.VC1PackOneSlice(m_pContext,
                                &slparams,
                                m_iSliceBufIndex,
                                Offset,
                                SliceSize - BytesAlreadyExecuted,
                                StartCodeOffset,
                                2);

                            m_pPacker.VC1SetBuffersSize(m_iSliceBufIndex+1);

                            m_pPacker.VC1PackPicParamsForOneSlice(m_pContext);
                            if (UMC_OK != m_va->Execute())
                                throw VC1Exceptions::vc1_exception(VC1Exceptions::internal_pipeline_error);
                            StartCodeOffset = 0;
                            m_iSliceBufIndex = 0;

                            // we need receive buffers from drv for future processing
                            if (MBEndRowOfAlreadyExec != m_pContext->m_seqLayerHeader.heightMB)
                            {
                                m_pPacker.VC1SetSliceBuffer();
                                m_pPacker.VC1SetPictureBuffer();
                            }

                        }
                        else
                        {
                            m_pPacker.VC1PackOneSlice(m_pContext,
                                &slparams,
                                m_iSliceBufIndex,
                                Offset,
                                SliceSize,
                                StartCodeOffset,
                                0);
                            StartCodeOffset += SliceSize - RemBytesInSlice;
                            ++m_iSliceBufIndex;
                        }
                    }

                    ++pOffsets;
                    ++pValues;
                    ++pOriginalOffsets;
                }
                //Slices
                else if (*(pValues) == 0x0B010000)
                {
                    m_pContext->m_bitstream.pBitstream = reinterpret_cast<uint32_t*>(m_pContext->m_pBufferStart + *pOffsets);
                    VC1BitstreamParser::GetNBits(m_pContext->m_bitstream.pBitstream,m_pContext->m_bitstream.bitOffset,32, temp_value);
                    m_pContext->m_bitstream.bitOffset = 31;
                    pPictHeader = m_pContext->m_bitstream.pBitstream;

                    VC1BitstreamParser::GetNBits(m_pContext->m_bitstream.pBitstream,m_pContext->m_bitstream.bitOffset,9, slparams.MBStartRow);     //SLICE_ADDR
                    VC1BitstreamParser::GetNBits(m_pContext->m_bitstream.pBitstream,m_pContext->m_bitstream.bitOffset,1, PicHeaderFlag);            //PIC_HEADER_FLAG

                    if (PicHeaderFlag == 1)                //PIC_HEADER_FLAG
                    {
                        ++m_pContext->m_picLayerHeader;
                        if (isSecondField)
                            m_pContext->m_picLayerHeader->CurrField = 1;
                        else
                            m_pContext->m_picLayerHeader->CurrField = 0;

                        DecodePictureHeader_Adv(m_pContext);
                        DecodePicHeader(m_pContext);
                        ++m_iPicBufIndex;
                    }
                    m_pContext->m_picLayerHeader->is_slice = 1;

                    if (*(pOffsets+1) && *(pValues+1) == 0x0B010000)
                    {
                        bitstream = reinterpret_cast<uint32_t*>(m_pContext->m_pBufferStart + *(pOffsets+1));
                        VC1BitstreamParser::GetNBits(bitstream,bitoffset,32, temp_value);
                        bitoffset = 31;
                        VC1BitstreamParser::GetNBits(bitstream,bitoffset,9, slparams.MBEndRow);
                    }
                    else if(*(pValues+1) == 0x0C010000)
                        slparams.MBEndRow = (m_pContext->m_seqLayerHeader.heightMB+1)/2;
                    else
                        slparams.MBEndRow = m_pContext->m_seqLayerHeader.heightMB;

                    slparams.MBRowsToDecode = slparams.MBEndRow-slparams.MBStartRow;
                    slparams.MBRowsToDecode = slparams.MBEndRow-slparams.MBStartRow;
                    slparams.m_bitOffset = m_pContext->m_bitstream.bitOffset;
                    Offset = (uint32_t) (sizeof(uint32_t)*(m_pContext->m_bitstream.pBitstream - pPictHeader)); // offset in words

                    if (isSecondField)
                        slparams.MBStartRow -= (m_pContext->m_seqLayerHeader.heightMB+1)/2;

                    if (*(pOffsets+1))
                        SliceSize = *(pOriginalOffsets+1) - *pOriginalOffsets;
                    else
                        //SliceSize = m_pContext->m_FrameSize - StartCodeOffset;
                        SliceSize = m_pContext->m_FrameSize - *pOriginalOffsets;

                    {
                        uint32_t chopType = 1;
                        RemBytesInSlice  = m_pPacker.VC1PackBitStream(m_pContext,
                                                                      SliceSize,
                                                                      pOriginalData + *pOriginalOffsets,
                                                                      StartCodeOffset,
                                                                      false, Flag_03);
                        if (RemBytesInSlice)
                        {
                            uint32_t ByteFilledInSlice = SliceSize - RemBytesInSlice;
                            MBEndRowOfAlreadyExec = slparams.MBEndRow;
                            uint32_t BytesAlreadyExecuted = 0;

                            // fill slices
                            while (RemBytesInSlice) // we don't have enough buffer - execute it
                            {
                                m_pPacker.VC1PackOneSlice(m_pContext,
                                                          &slparams,
                                                          m_iSliceBufIndex,
                                                          Offset,
                                                          ByteFilledInSlice,
                                                          StartCodeOffset,
                                                          chopType);

                                m_pPacker.VC1SetBuffersSize(m_iSliceBufIndex+1);

                                if (UMC_OK != m_va->Execute())
                                    throw VC1Exceptions::vc1_exception(VC1Exceptions::internal_pipeline_error);
                                BytesAlreadyExecuted += ByteFilledInSlice;

                                m_pPacker.VC1SetSliceBuffer();
                                m_pPacker.VC1SetPictureBuffer();

                                StartCodeOffset = 0;
                                m_iSliceBufIndex = 0;

                                m_pPacker.VC1PackPicParamsForOneSlice(m_pContext);
                                RemBytesInSlice  = m_pPacker.VC1PackBitStream(m_pContext,
                                                                              RemBytesInSlice,
                                                                              pOriginalData + *pOriginalOffsets + BytesAlreadyExecuted,
                                                                              StartCodeOffset,
                                                                              false, Flag_03);

                                ByteFilledInSlice = SliceSize - BytesAlreadyExecuted - RemBytesInSlice;
                                chopType = 3;
                                slparams.m_bitOffset = 0;
                                Offset = 0;
                            }

                            m_pPacker.VC1PackOneSlice(m_pContext,
                                                     &slparams,
                                                      m_iSliceBufIndex,
                                                      Offset,
                                                      SliceSize - BytesAlreadyExecuted,
                                                      StartCodeOffset,
                                                      2);
                            m_pPacker.VC1SetBuffersSize(m_iSliceBufIndex+1);
                            m_pPacker.VC1PackPicParamsForOneSlice(m_pContext);
                            if (UMC_OK != m_va->Execute())
                                throw VC1Exceptions::vc1_exception(VC1Exceptions::internal_pipeline_error);

                            StartCodeOffset = 0;
                            m_iSliceBufIndex = 0;

                            // we need receive buffers from drv for future processing
                            if (MBEndRowOfAlreadyExec != m_pContext->m_seqLayerHeader.heightMB)
                            {
                                m_pPacker.VC1SetSliceBuffer();
                                m_pPacker.VC1SetPictureBuffer();
                            }
                        }
                        else
                        {
                            m_pPacker.VC1PackOneSlice(m_pContext,
                                &slparams,
                                m_iSliceBufIndex,
                                Offset,
                                SliceSize,
                                StartCodeOffset,
                                0);
                            // let execute in case of fields
                            if (*(pValues+1)== 0x0C010000)
                            {
                                // Execute first slice
                                if (m_va->m_Platform == VA_LINUX)
                                    m_pPacker.VC1PackBitplaneBuffers(m_pContext);

                                m_pPacker.VC1SetBuffersSize(m_iSliceBufIndex + 1);

                                if (UMC_OK != m_va->Execute())
                                    throw VC1Exceptions::vc1_exception(VC1Exceptions::internal_pipeline_error);
                                StartCodeOffset = 0;
                                m_iSliceBufIndex = 0;
                            }
                            else
                            {
                                StartCodeOffset += SliceSize - RemBytesInSlice;
                                ++m_iSliceBufIndex;
                            }
                        }
                    }

                    ++pOffsets;
                    ++pValues;
                    ++pOriginalOffsets;

                }
                else
                {
                    ++pOffsets;
                    ++pValues;
                    ++pOriginalOffsets;
                }

            }
            //need to execute last slice
            if (MBEndRowOfAlreadyExec != m_pContext->m_seqLayerHeader.heightMB)
            {
                m_pPacker.VC1SetBuffersSize(m_iSliceBufIndex);
                if (m_va->m_Platform == VA_LINUX)
                    m_pPacker.VC1PackBitplaneBuffers(m_pContext);
                if (UMC_OK != m_va->Execute())
                    throw VC1Exceptions::vc1_exception(VC1Exceptions::internal_pipeline_error);
            }
         }



        virtual void PrepareVLDVABuffers(uint32_t*  pOffsets,
                                         uint32_t*  pValues,
                                         uint8_t*   pOriginalData,
                                         MediaDataEx::_MediaDataEx* pOrigStCodes)
        {
            if (VC1_IS_SKIPPED(m_pContext->m_picLayerHeader->PTYPE))
            {
                m_bIsSkippedFrame = true;
                m_bIsReadyToProcess = false;
                m_pPacker.MarkFrameAsSkip();
            }
            else
            {
                m_pPacker.VC1SetSliceBuffer();
                m_pPacker.VC1SetPictureBuffer();
                VC1PackSlices(pOffsets,pValues,pOriginalData,pOrigStCodes);
            }

            if ((m_iFrameCounter > 1) ||
                (!VC1_IS_REFERENCE(m_pContext->m_picLayerHeader->PTYPE)))
            {
                m_pStore->SetFirstBusyDescriptorAsReady();
            }
        }

        bool IsNeedToExecute()
        {
            return !m_bIsItarativeMode;
        }

        virtual ~VC1FrameDescriptorVA()
        {
        }
        virtual void SetVideoHardwareAccelerator(VideoAccelerator* va)
        {
            if (va)
            {
                m_va = va;
                m_pPacker.SetVideoAccelerator(m_va);
            }

        }
        virtual Status preProcData(VC1Context*            pContext,
                                   uint32_t                 bufferSize,
                                   unsigned long long                 frameCount,
                                   bool& skip)
        {
            Status vc1Sts = UMC_OK;

            uint32_t Ptype;
            uint8_t* pbufferStart = pContext->m_pBufferStart;

            m_iFrameCounter = frameCount;

            // there is self header before each frame in simple/main profile
            if (m_pContext->m_seqLayerHeader.PROFILE != VC1_PROFILE_ADVANCED)
            {
                bufferSize += VC1FHSIZE;
                m_pContext->m_FrameSize = bufferSize;
            }



            if (m_va->m_Platform == VA_LINUX)
            {
                if (m_iFrameCounter == 1)
                    m_pContext->m_frmBuff.m_iCurrIndex = -1;
            }


            MFX_INTERNAL_CPY(m_pContext->m_pBufferStart, pbufferStart, (bufferSize & 0xFFFFFFF8) + 8); // (bufferSize & 0xFFFFFFF8) + 8 - skip frames

            m_pContext->m_bitstream.pBitstream = (uint32_t*)m_pContext->m_pBufferStart;

            m_pContext->m_bitstream.pBitstream += 1;

            m_pContext->m_bitstream.bitOffset = 31;
            m_pContext->m_picLayerHeader = m_pContext->m_InitPicLayer;
            m_pContext->m_seqLayerHeader = pContext->m_seqLayerHeader;
            m_bIsSpecialBSkipFrame = false;


            if (m_pContext->m_seqLayerHeader.PROFILE == VC1_PROFILE_ADVANCED)
            {
                m_pBufferStart = m_pContext->m_bitstream.pBitstream;
                vc1Sts = GetNextPicHeader_Adv(m_pContext);
                UMC_CHECK_STATUS(vc1Sts);
                Ptype = m_pContext->m_picLayerHeader->PTYPE|m_pContext->m_picLayerHeader->PTypeField1;
                {
                    vc1Sts = SetPictureIndices(Ptype,skip);
                }

            }
            else
            {
                m_pContext->m_bitstream.pBitstream = (uint32_t*)m_pContext->m_pBufferStart + 2;

                m_pBufferStart = m_pContext->m_bitstream.pBitstream;
                vc1Sts = GetNextPicHeader(m_pContext, false);
                UMC_CHECK_STATUS(vc1Sts);
                vc1Sts = SetPictureIndices(m_pContext->m_picLayerHeader->PTYPE,skip);
                UMC_CHECK_STATUS(vc1Sts);

            }
            if (m_pStore->IsNeedPostProcFrame(m_pContext->m_picLayerHeader->PTYPE))
                m_pContext->DeblockInfo.isNeedDbl = 1;
            else
                m_pContext->DeblockInfo.isNeedDbl = 0;

            m_pContext->m_bIntensityCompensation = 0;
            return vc1Sts;
        }

    protected:
        VideoAccelerator*     m_va;
        uint8_t*                m_pBitstream;
        uint32_t m_iSliceBufIndex;
        uint32_t m_iPicBufIndex;
        bool  m_bIsItarativeMode;
        uint32_t* m_pBufferStart;
        T m_pPacker;
    };

    template <class T>
    class VC1FrameDescriptorVA_Linux: public VC1FrameDescriptorVA<T>
    {
    public:
        // Default constructor
        VC1FrameDescriptorVA_Linux(MemoryAllocator*      pMemoryAllocator,
                                       VideoAccelerator*     va):VC1FrameDescriptorVA<T>(pMemoryAllocator, va)
        {
        }

        virtual ~VC1FrameDescriptorVA_Linux(){};

        void PrepareVLDVABuffers(uint32_t*  pOffsets,
                                 uint32_t*  pValues,
                                 uint8_t*   pOriginalData,
                                 MediaDataEx::_MediaDataEx* pOrigStCodes)
        {
            if (VC1_IS_SKIPPED(this->m_pContext->m_picLayerHeader->PTYPE))
            {
                this->m_bIsSkippedFrame = true;
                this->m_bIsReadyToProcess = false;
                this->m_pPacker.MarkFrameAsSkip();
            }
            else
            {
                this->m_pPacker.VC1SetPictureBuffer();
                this->m_pPacker.VC1SetSliceParamBuffer(pOffsets, pValues);
                this->m_pPacker.VC1SetSliceDataBuffer(this->m_pContext->m_FrameSize); // 4 - header ?
                VC1PackSlices(pOffsets,pValues,pOriginalData,pOrigStCodes);
            }

            if ((this->m_iFrameCounter > 1) ||
                (!VC1_IS_REFERENCE(this->m_pContext->m_picLayerHeader->PTYPE)))
            {
                this->m_pStore->SetFirstBusyDescriptorAsReady();
            }
        }
        virtual void VC1PackSlices    (uint32_t*  pOffsets,
                                       uint32_t*  pValues,
                                       uint8_t*   pOriginalData,
                                       MediaDataEx::_MediaDataEx* pOrigStCodes)
       {
            uint32_t PicHeaderFlag;
            uint32_t* pPictHeader = this->m_pContext->m_bitstream.pBitstream;
            uint32_t temp_value = 0;
            uint32_t* bitstream;
            int32_t bitoffset = 31;
            uint32_t SliceSize = 0;
            uint32_t DataSize = 0;
            uint32_t BitstreamDataSize = 0;

            // need in case of fields
            uint32_t* pFirstFieldStartCode = this->m_pContext->m_bitstream.pBitstream;
            uint32_t Offset = 0;

            uint32_t PicHeaderSize = 0;
            uint8_t  Flag_03 = 0; //for search 00 00 03 on the end of pic header

            uint32_t* p_CurOriginalOffsets = pOrigStCodes->offsets;
            uint32_t* p_NextOriginalOffsets = pOrigStCodes->offsets;

            uint32_t*  p_CurOffsets = pOffsets;
            uint32_t*  p_CurValues = pValues;
            uint8_t*   p_CurOriginalData = pOriginalData;
            
            uint32_t*  p_NextOffsets = pOffsets;
            uint32_t*  p_NextValues = pValues;
            

            bool isSecondField = false;
            this->m_iSliceBufIndex = 0;
            this->m_iPicBufIndex = 0;

            uint32_t MBEndRowOfAlreadyExec = 0;

            SliceParams slparams;
            memset(&slparams,0,sizeof(SliceParams));
            slparams.MBStartRow = 0;
            slparams.MBEndRow = this->m_pContext->m_seqLayerHeader.heightMB;
            this->m_pContext->m_picLayerHeader->CurrField = 0;

            if (VC1_IS_SKIPPED(this->m_pContext->m_picLayerHeader->PTYPE))
            {
                this->m_bIsSkippedFrame = true;
                this->m_bIsReadyToProcess = false;
                return;
            }
            else
            {
                this->m_bIsSkippedFrame = false;
            }

            //skip start codes till Frame Header
            while ((*p_CurValues)&&(*p_CurValues != 0x0D010000))
            {
                ++p_CurOffsets;
                ++p_CurValues;
                ++p_CurOriginalOffsets;
            }

            //move next st code data pointers
            p_NextOffsets = p_CurOffsets + 1;
            p_NextValues = p_CurValues + 1;
            p_NextOriginalOffsets = p_CurOriginalOffsets + 1;

            if (this->m_pContext->m_seqLayerHeader.PROFILE == VC1_PROFILE_ADVANCED)
                DecodePicHeader(this->m_pContext);
            else
                Decode_PictureLayer(this->m_pContext);
            if (this->m_pContext->m_seqLayerHeader.PROFILE == VC1_PROFILE_ADVANCED)
            {
                PicHeaderSize = (this->m_pPacker.VC1GetPicHeaderSize(p_CurOriginalData + 4,
                    (uint32_t)((size_t)sizeof(uint32_t)*(this->m_pContext->m_bitstream.pBitstream - pPictHeader)), Flag_03));
            }
            else
            {
                PicHeaderSize = (uint32_t)((size_t)sizeof(uint32_t)*(this->m_pContext->m_bitstream.pBitstream - pPictHeader));
            }

            this->m_pPacker.VC1PackPicParamsForOneSlice(this->m_pContext);

            slparams.MBRowsToDecode = slparams.MBEndRow-slparams.MBStartRow;
            slparams.m_bitOffset    = this->m_pContext->m_bitstream.bitOffset;
            if (VC1_PROFILE_ADVANCED == this->m_pContext->m_seqLayerHeader.PROFILE)
                Offset = PicHeaderSize + 4; // offset in bytes
            else
                Offset = PicHeaderSize;

            if (*(p_CurOffsets+1) && p_CurOriginalOffsets && (p_CurOriginalOffsets + 1))
                SliceSize = *(p_CurOriginalOffsets+1) - *p_CurOriginalOffsets - Offset;
            else
                SliceSize = this->m_pContext->m_FrameSize - Offset;

            //skip user data
            while(*(p_NextValues) == 0x1B010000 || *(p_NextValues) == 0x1D010000 || *(p_NextValues) == 0x1C010000)
            {
                p_NextOffsets++;
                p_NextValues++;
                p_NextOriginalOffsets++;
            }

            if (*p_NextValues == 0x0B010000)
            {
                //slice
                bitstream = reinterpret_cast<uint32_t*>(this->m_pContext->m_pBufferStart + *p_NextOffsets);
                VC1BitstreamParser::GetNBits(bitstream, bitoffset,32, temp_value);
                bitoffset = 31;
                VC1BitstreamParser::GetNBits(bitstream,bitoffset,9, slparams.MBEndRow);     //SLICE_ADDR
                this->m_pContext->m_picLayerHeader->is_slice = 1;
            }
            else if(*p_NextValues == 0x0C010000)
            {
                //field
                slparams.MBEndRow = (this->m_pContext->m_seqLayerHeader.heightMB+1)/2;
            }

            VM_ASSERT(SliceSize<0x0FFFFFFF);

            {
                this->m_pPacker.VC1PackBitStream(this->m_pContext,
                                            DataSize,
                                            p_CurOriginalData + Offset,
                                            SliceSize,
                                            0, Flag_03);

                this->m_pPacker.VC1PackOneSlice(this->m_pContext, &slparams, this->m_iSliceBufIndex,
                                            0, DataSize, 0, 0);
                // let execute in case of fields
                if (*p_NextValues== 0x0C010000)
                {
                    // Execute first slice
                    this->m_pPacker.VC1PackBitplaneBuffers(this->m_pContext);
                    this->m_pPacker.VC1SetBuffersSize(this->m_iSliceBufIndex + 1);

                    if (UMC_OK != this->m_va->Execute())
                        throw VC1Exceptions::vc1_exception(VC1Exceptions::internal_pipeline_error);
                    this->m_iSliceBufIndex = 0;
                }
                else
                {
                    BitstreamDataSize += DataSize;
                    ++this->m_iSliceBufIndex;
                }
            }

            //move current st code data pointers
            p_CurOffsets = p_NextOffsets;
            p_CurValues  = p_NextValues;
            p_CurOriginalOffsets = p_NextOriginalOffsets;

            //move next st code data pointers
            p_NextOffsets++;
            p_NextValues++;
            p_NextOriginalOffsets++;

            while (*p_CurOffsets)
            {
                //Fields
                if (*(p_CurValues) == 0x0C010000)
                {
                    this->m_pContext->m_bitstream.pBitstream = pFirstFieldStartCode;

                    if (UMC_OK != this->m_va->EndFrame())
                        throw VC1Exceptions::vc1_exception(VC1Exceptions::internal_pipeline_error);

                    this->m_pPacker.VC1SetPictureBuffer();
                    this->m_pPacker.VC1SetSliceParamBuffer(pOffsets, pValues);
                    this->m_pPacker.VC1SetSliceDataBuffer(this->m_pContext->m_FrameSize); //TODO: what is a valid size of a buffer here?
                    isSecondField = true;
                    // set swap bitstream for parsing
                    this->m_pContext->m_bitstream.pBitstream = reinterpret_cast<uint32_t*>(this->m_pContext->m_pBufferStart + *p_CurOffsets);
                    this->m_pContext->m_bitstream.pBitstream += 1; // skip start code
                    this->m_pContext->m_bitstream.bitOffset = 31;
                    pPictHeader = this->m_pContext->m_bitstream.pBitstream;
                    PicHeaderSize = 0;
                    BitstreamDataSize = 0;

                    ++this->m_pContext->m_picLayerHeader;
                    *this->m_pContext->m_picLayerHeader = *this->m_pContext->m_InitPicLayer;
                    this->m_pContext->m_picLayerHeader->BottomField = (uint8_t)this->m_pContext->m_InitPicLayer->TFF;
                    this->m_pContext->m_picLayerHeader->PTYPE = this->m_pContext->m_InitPicLayer->PTypeField2;
                    this->m_pContext->m_picLayerHeader->CurrField = 1;

                    this->m_pContext->m_picLayerHeader->is_slice = 0;
                    DecodePicHeader(this->m_pContext);

                    PicHeaderSize = (uint32_t)this->m_pPacker.VC1GetPicHeaderSize(p_CurOriginalData + 4 + *p_CurOriginalOffsets,
                        (uint32_t)((size_t)sizeof(uint32_t)*(this->m_pContext->m_bitstream.pBitstream - pPictHeader)), Flag_03);

                    slparams.MBStartRow = (this->m_pContext->m_seqLayerHeader.heightMB+1)/2;

                    if (*(p_CurOffsets+1))
                        SliceSize = *(p_CurOriginalOffsets + 1) - *p_CurOriginalOffsets - 4 - PicHeaderSize;
                    else
                        SliceSize = this->m_pContext->m_FrameSize - *p_CurOriginalOffsets - 4 - PicHeaderSize;

                    //skip user data
                    while(*p_NextValues == 0x1B010000 || *p_NextValues == 0x1D010000 || *p_NextValues == 0x1C010000)
                    {
                        p_NextOffsets++;
                        p_NextValues++;
                        p_NextOriginalOffsets++;
                    }   

                    if (*p_NextOffsets && *p_NextValues == 0x0B010000)
                    {
                        bitstream = reinterpret_cast<uint32_t*>(this->m_pContext->m_pBufferStart + *p_NextOffsets);
                        VC1BitstreamParser::GetNBits(bitstream,bitoffset,32, temp_value);
                        bitoffset = 31;
                        VC1BitstreamParser::GetNBits(bitstream,bitoffset,9, slparams.MBEndRow);
                    } else
                        slparams.MBEndRow = this->m_pContext->m_seqLayerHeader.heightMB;

                    this->m_pPacker.VC1PackPicParamsForOneSlice(this->m_pContext);
                    ++this->m_iPicBufIndex;
                    slparams.MBRowsToDecode = slparams.MBEndRow-slparams.MBStartRow;
                    slparams.m_bitOffset = this->m_pContext->m_bitstream.bitOffset;
                    Offset = PicHeaderSize + 4; // offset in bytes
                    slparams.MBStartRow = 0; //we should decode fields steb by steb

                    {
                        this->m_pPacker.VC1PackBitStream(this->m_pContext, DataSize,
                                                    pOriginalData + *p_CurOriginalOffsets + Offset,SliceSize,
                                                    0, Flag_03);

                        this->m_pPacker.VC1PackOneSlice(this->m_pContext,
                                                    &slparams,
                                                    this->m_iSliceBufIndex,
                                                    0,
                                                    DataSize,
                                                    0, 0);
                        BitstreamDataSize += DataSize;
                        ++this->m_iSliceBufIndex;
                    }

                    //move current st code data pointers
                    p_CurOffsets = p_NextOffsets;
                    p_CurValues  = p_NextValues;
                    p_CurOriginalOffsets = p_NextOriginalOffsets;

                    //move next st code data pointers
                    p_NextOffsets++;
                    p_NextValues++;
                    p_NextOriginalOffsets++;
                }
                //Slices
                else if (*p_CurValues == 0x0B010000)
                {
                    this->m_pContext->m_bitstream.pBitstream = reinterpret_cast<uint32_t*>(this->m_pContext->m_pBufferStart + *p_CurOffsets);
                    VC1BitstreamParser::GetNBits(this->m_pContext->m_bitstream.pBitstream,this->m_pContext->m_bitstream.bitOffset,32, temp_value);
                    this->m_pContext->m_bitstream.bitOffset = 31;
                    pPictHeader = this->m_pContext->m_bitstream.pBitstream;
                    PicHeaderSize = 0;

                    VC1BitstreamParser::GetNBits(this->m_pContext->m_bitstream.pBitstream,this->m_pContext->m_bitstream.bitOffset,9, slparams.MBStartRow);     //SLICE_ADDR
                    VC1BitstreamParser::GetNBits(this->m_pContext->m_bitstream.pBitstream,this->m_pContext->m_bitstream.bitOffset,1, PicHeaderFlag);            //PIC_HEADER_FLAG

                    if (PicHeaderFlag == 1)                //PIC_HEADER_FLAG
                    {
                        ++this->m_pContext->m_picLayerHeader;
                        if (isSecondField)
                            this->m_pContext->m_picLayerHeader->CurrField = 1;
                        else
                            this->m_pContext->m_picLayerHeader->CurrField = 0;

                        DecodePictureHeader_Adv(this->m_pContext);
                        DecodePicHeader(this->m_pContext);
                        PicHeaderSize = this->m_pPacker.VC1GetPicHeaderSize(p_CurOriginalData + 4 + *p_CurOriginalOffsets,
                            (uint32_t)((size_t)sizeof(uint32_t)*(this->m_pContext->m_bitstream.pBitstream - pPictHeader)), Flag_03);

                        ++this->m_iPicBufIndex;
                    }
                    this->m_pContext->m_picLayerHeader->is_slice = 1;
                    //skip user data
                    while(*p_NextValues == 0x1B010000 || *p_NextValues == 0x1D010000 || *p_NextValues == 0x1C010000)
                    {
                        p_NextOffsets++;
                        p_NextValues++;
                        p_NextOriginalOffsets++;
                    }  

                    if (*p_NextOffsets && *p_NextValues == 0x0B010000)
                    {
                        bitstream = reinterpret_cast<uint32_t*>(this->m_pContext->m_pBufferStart + *(pOffsets+1));
                        VC1BitstreamParser::GetNBits(bitstream,bitoffset,32, temp_value);
                        bitoffset = 31;
                        VC1BitstreamParser::GetNBits(bitstream,bitoffset,9, slparams.MBEndRow);
                    }
                    else if(*p_NextValues == 0x0C010000)
                        slparams.MBEndRow = (this->m_pContext->m_seqLayerHeader.heightMB+1)/2;
                    else
                        slparams.MBEndRow = this->m_pContext->m_seqLayerHeader.heightMB;

                    slparams.MBRowsToDecode = slparams.MBEndRow-slparams.MBStartRow;
                    slparams.m_bitOffset = this->m_pContext->m_bitstream.bitOffset;
                    Offset = PicHeaderSize + 4;

                    if (isSecondField)
                        slparams.MBStartRow -= (this->m_pContext->m_seqLayerHeader.heightMB+1)/2;

                    if (*(p_CurOffsets+1))
                        SliceSize = *(p_CurOriginalOffsets+1) - *p_CurOriginalOffsets - Offset;
                    else
                        SliceSize = this->m_pContext->m_FrameSize - *p_CurOriginalOffsets - Offset;

                    {
                        this->m_pPacker.VC1PackBitStream(this->m_pContext,
                                                                      DataSize,
                                                                      p_CurOriginalData + *p_CurOriginalOffsets + Offset,
                                                                      SliceSize,
                                                                      BitstreamDataSize, Flag_03);

                        this->m_pPacker.VC1PackOneSlice(this->m_pContext,
                                                        &slparams,
                                                        this->m_iSliceBufIndex,   0,
                                                        DataSize, BitstreamDataSize, 0);

                        //skip user data
                        while(*p_NextValues == 0x1B010000 || *p_NextValues == 0x1D010000 || *p_NextValues == 0x1C010000)
                        {
                            p_NextOffsets++;
                            p_NextValues++;
                            p_NextOriginalOffsets++;
                        }  
                        // let execute in case of fields
                            if (*p_NextValues== 0x0C010000)
                        {
                            // Execute first slice
                            this->m_pPacker.VC1PackBitplaneBuffers(this->m_pContext);

                            this->m_pPacker.VC1SetBuffersSize(this->m_iSliceBufIndex + 1);

                            if (UMC_OK != this->m_va->Execute())
                                throw VC1Exceptions::vc1_exception(VC1Exceptions::internal_pipeline_error);
                            this->m_iSliceBufIndex = 0;
                        }
                        else
                        {
                            BitstreamDataSize += DataSize/*SliceSize - RemBytesInSlice*/;
                            ++this->m_iSliceBufIndex;
                        }
                    }
                    //move current st code data pointers
                    p_CurOffsets = p_NextOffsets;
                    p_CurValues  = p_NextValues;
                    p_CurOriginalOffsets = p_NextOriginalOffsets;

                    //move next st code data pointers
                    p_NextOffsets++;
                    p_NextValues++;
                    p_NextOriginalOffsets++;

                }
                else
                {
                    //move current st code data pointers
                    p_CurOffsets = p_NextOffsets;
                    p_CurValues  = p_NextValues;
                    p_CurOriginalOffsets = p_NextOriginalOffsets;

                    //move next st code data pointers
                    p_NextOffsets++;
                    p_NextValues++;
                    p_NextOriginalOffsets++;
                }

            }
            //need to execute last slice
            if (MBEndRowOfAlreadyExec != this->m_pContext->m_seqLayerHeader.heightMB)
            {
                this->m_pPacker.VC1PackBitplaneBuffers(this->m_pContext);
                this->m_pPacker.VC1SetBuffersSize(this->m_iSliceBufIndex);
                if (UMC_OK != this->m_va->Execute())
                    throw VC1Exceptions::vc1_exception(VC1Exceptions::internal_pipeline_error);
            }
         }
    };


} // namespace UMC

#endif // #if defined(UMC_VA)

#endif //__UMC_VC1_DEC_FRAME_DESCR_VA_H_
#endif //MFX_ENABLE_VC1_VIDEO_DECODE

