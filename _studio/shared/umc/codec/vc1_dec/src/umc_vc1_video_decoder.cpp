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

#include "umc_vc1_video_decoder.h"
#include "umc_media_data_ex.h"
#include "umc_vc1_dec_seq.h"
#include "vm_sys_info.h"
#include "umc_vc1_dec_task_store.h"

#include "umc_memory_allocator.h"
#include "umc_vc1_common.h"
#include "umc_vc1_common_tables.h"
#include "umc_vc1_common_tables_adv.h"
#include "umc_vc1_common_defs.h"
#include "umc_vc1_dec_exception.h"

#include "umc_va_base.h"
#include "mfx_trace.h"

#include <assert.h>
#include "umc_vc1_huffman.h"

using namespace UMC;
using namespace UMC::VC1Common;
using namespace UMC::VC1Exceptions;

VC1VideoDecoder::VC1VideoDecoder():m_pMemoryAllocator(0),
                                   m_pContext(NULL),
                                   m_pInitContext(),
                                   m_iThreadDecoderNum(0),
                                   m_dataBuffer(NULL),
                                   m_frameData(NULL),
                                   m_stCodes(NULL),
                                   m_decoderInitFlag(0),
                                   m_decoderFlags(0),
                                   m_iMemContextID((MemID)(-1)),
                                   m_iHeapID((MemID)(-1)),
                                   m_iFrameBufferID((MemID)(-1)),
                                   m_pts(0),
                                   m_pts_dif(0),
                                   m_iMaxFramesInProcessing(0),
                                   m_lFrameCount(0),
                                   m_bLastFrameNeedDisplay(false),
                                   m_pStore(NULL),
                                   m_va(0),
                                   m_pHeap(NULL),
                                   m_bIsReorder(true),
                                   m_pCurrentIn(NULL),
                                   m_pCurrentOut(NULL),
                                   m_bIsNeedToFlush(false),
                                   m_AllocBuffer(0),
                                   m_pExtFrameAllocator(0),
                                   m_SurfaceNum(0),
                                   m_bIsExternalFR(false),
                                   m_pDescrToDisplay(0),
                                   m_frameOrder(0),
                                   m_RMIndexToFree(-1),
                                   m_CurrIndexToFree(-1)
{
}

VC1VideoDecoder::~VC1VideoDecoder()
{
    Close();
}


Status VC1VideoDecoder::Init(BaseCodecParams *pInit)
{
    MediaData *data;
    Status umcRes = UMC_OK;
    uint32_t readSize = 0;

    Close();

    VideoDecoderParams *init = DynamicCast<VideoDecoderParams, BaseCodecParams>(pInit);
    if(!init)
        return UMC_ERR_INIT;

    m_decoderFlags = init->lFlags;
    data = init->m_pData;

    m_ClipInfo = init->info;
    if (m_ClipInfo.framerate != 0.0)
        m_bIsExternalFR = true;

    //almost all applications need reorder as default 
    if ((m_decoderFlags & FLAG_VDEC_REORDER) == FLAG_VDEC_REORDER)
        m_bIsReorder = true;
    else
        m_bIsReorder = false;

   
    uint32_t mbWidth = init->info.clip_info.width/VC1_PIXEL_IN_LUMA;
    uint32_t mbHeight= init->info.clip_info.height/VC1_PIXEL_IN_LUMA;

    m_AllocBuffer = 2*(mbHeight*VC1_PIXEL_IN_LUMA)*(mbWidth*VC1_PIXEL_IN_LUMA);

    m_SurfaceNum = init->m_SuggestedOutputSize;

    m_pMemoryAllocator = init->lpMemoryAllocator;

    // get allowed thread numbers
    int32_t nAllowedThreadNumber = init->numThreads;


    m_iThreadDecoderNum = (0 == nAllowedThreadNumber) ? (vm_sys_info_get_cpu_num()) : (nAllowedThreadNumber);

    m_iMaxFramesInProcessing = m_iThreadDecoderNum + NumBufferedFrames;

    if (UMC_OK != ContextAllocation(mbWidth, mbHeight))
        return UMC_ERR_INIT;


    //Heap allocation
    {
        uint32_t heapSize = CalculateHeapSize();
        // Need to replace with MFX allocator
        if (m_pMemoryAllocator->Alloc(&m_iHeapID,
                                      heapSize,//100000,
                                      UMC_ALLOC_PERSISTENT,
                                      16) != UMC_OK)
                                      return UMC_ERR_ALLOC;

        new(m_pHeap) VC1TSHeap((uint8_t*)(m_pMemoryAllocator->Lock(m_iHeapID)),heapSize);
    }
    if (UMC_OK != CreateFrameBuffer(m_AllocBuffer))
        return UMC_ERR_ALLOC;


    m_pContext->m_bIntensityCompensation = 0;

    // profile definition
    if ((VC1_VIDEO_RCV == init->info.stream_subtype)||
        (WMV3_VIDEO == init->info.stream_subtype))
        m_pContext->m_seqLayerHeader.PROFILE = VC1_PROFILE_MAIN;
    else if ((VC1_VIDEO_VC1 == init->info.stream_subtype)||
             (WVC1_VIDEO == init->info.stream_subtype))
             m_pContext->m_seqLayerHeader.PROFILE = VC1_PROFILE_ADVANCED;
    else
    {
        m_pContext->m_seqLayerHeader.PROFILE = VC1_PROFILE_UNKNOWN;
    }
    
    m_pContext->m_Offsets = m_frameData->GetExData()->offsets;
    m_pContext->m_values = m_frameData->GetExData()->values;

    if(data!=NULL && (int32_t*)data->GetDataPointer() != NULL) // seq header is presents
    {
        m_pCurrentIn = data;
        
        if(m_pContext->m_seqLayerHeader.PROFILE == VC1_PROFILE_UNKNOWN)
        {
            //assert(0);
            if((uint32_t)((m_pCurrentIn)&&0xFF) == 0xC5)
                m_pContext->m_seqLayerHeader.PROFILE = VC1_PROFILE_MAIN;
            else
                m_pContext->m_seqLayerHeader.PROFILE = VC1_PROFILE_ADVANCED;
        }

        //need to create buffer for swap data
        if (VC1_PROFILE_ADVANCED == m_pContext->m_seqLayerHeader.PROFILE)
        {
            umcRes = GetStartCodes((uint8_t*)data->GetDataPointer(),
                                    (uint32_t)data->GetDataSize(),
                                    m_frameData,
                                    &readSize); // parse and copy to self buffer

            SwapData((uint8_t*)m_frameData->GetDataPointer(), (uint32_t)m_frameData->GetDataSize());
            m_pContext->m_pBufferStart = (uint8_t*)m_frameData->GetDataPointer();
            umcRes = StartCodesProcessing(m_pContext->m_pBufferStart,
                                            m_pContext->m_Offsets,
                                            m_pContext->m_values,
                                            true);
            if (UMC_ERR_SYNC != umcRes)
                umcRes = UMC_OK;
            m_pCurrentIn->MoveDataPointer(readSize);
        }
        else
        {
            // simple copy data
            MFX_INTERNAL_CPY(m_dataBuffer, (uint8_t*)data->GetDataPointer(), (uint32_t)data->GetDataSize());
            SwapData((uint8_t*)m_frameData->GetDataPointer(), (uint32_t)(uint32_t)data->GetDataSize());
            m_pContext->m_FrameSize  = (uint32_t)data->GetDataSize();
        }

        m_pContext->m_bitstream.bitOffset    = 31;
        if (VC1_PROFILE_ADVANCED != m_pContext->m_seqLayerHeader.PROFILE)
        {
            umcRes = InitSMProfile();
            UMC_CHECK_STATUS(umcRes);
            readSize = (uint32_t)m_pCurrentIn->GetDataSize();
            data->MoveDataPointer(readSize);
        }

        if(m_AllocBuffer == 0)
        {
            Close();
            return UMC_ERR_SYNC;
        }

        GetFPS(m_pContext);
        GetPTS(data->GetTime());
        m_decoderInitFlag = 1;
    }
    else
    {
        //NULL input data, no sequence header
        
        // aligned width and height 
        m_pContext->m_seqLayerHeader.MAX_CODED_HEIGHT = init->info.clip_info.height/2 - 1;
        m_pContext->m_seqLayerHeader.MAX_CODED_WIDTH = init->info.clip_info.width/2 - 1;

        m_pContext->m_seqLayerHeader.widthMB = (uint16_t)(init->info.clip_info.width/VC1_PIXEL_IN_LUMA);
        m_pContext->m_seqLayerHeader.heightMB  = (uint16_t)(init->info.clip_info.height/VC1_PIXEL_IN_LUMA);

        m_pContext->m_seqLayerHeader.MaxWidthMB = m_pContext->m_seqLayerHeader.widthMB;
        m_pContext->m_seqLayerHeader.MaxHeightMB = m_pContext->m_seqLayerHeader.heightMB;

        if(init->info.interlace_type != UMC::PROGRESSIVE)
            m_pContext->m_seqLayerHeader.INTERLACE  = 1;

        if(m_AllocBuffer == 0)
        {
            Close();
            return UMC_ERR_SYNC;
        }
    }

    if (!InitVAEnvironment())
        return UMC_ERR_ALLOC;

    if (!InitAlloc(m_pContext,2*m_iMaxFramesInProcessing))
        return UMC_ERR_ALLOC;

    m_pInitContext = *m_pContext;

    //internal decoding flags
    m_lFrameCount = 0;
    //internal exception initialization
    vc1_except_profiler::GetEnvDescript(smart_recon, mbGroupLevel);

    return umcRes;
}

Status VC1VideoDecoder::InitSMProfile()
{
    int32_t seq_size = 0;
    uint8_t* seqStart = NULL;
    uint32_t height;
    uint32_t width;
    VC1Status sts = VC1_OK;

    SwapData(m_pContext->m_pBufferStart, m_pContext->m_FrameSize);

    seqStart = m_pContext->m_pBufferStart + 4;
    seq_size  = ((*(seqStart+3))<<24) + ((*(seqStart+2))<<16) + ((*(seqStart+1))<<8) + *(seqStart);

    assert(seq_size > 0);
    assert(seq_size < 100);

    seqStart = m_pContext->m_pBufferStart + 8 + seq_size;
    height  = ((*(seqStart+3))<<24) + ((*(seqStart+2))<<16) + ((*(seqStart+1))<<8) + *(seqStart);
    seqStart+=4;
    width  = ((*(seqStart+3))<<24) + ((*(seqStart+2))<<16) + ((*(seqStart+1))<<8) + *(seqStart);

    m_pContext->m_seqLayerHeader.widthMB  = (uint16_t)((width+15)/VC1_PIXEL_IN_LUMA);
    m_pContext->m_seqLayerHeader.heightMB = (uint16_t)((height+15)/VC1_PIXEL_IN_LUMA);
    m_pContext->m_seqLayerHeader.CODED_HEIGHT  = m_pContext->m_seqLayerHeader.MAX_CODED_HEIGHT = height/2 - 1;
    m_pContext->m_seqLayerHeader.CODED_WIDTH = m_pContext->m_seqLayerHeader.MAX_CODED_WIDTH = width/2 - 1;

    m_pContext->m_seqLayerHeader.MaxWidthMB  = (uint16_t)((width+15)/VC1_PIXEL_IN_LUMA);
    m_pContext->m_seqLayerHeader.MaxHeightMB = (uint16_t)((height+15)/VC1_PIXEL_IN_LUMA);

    SwapData(m_pContext->m_pBufferStart, m_pContext->m_FrameSize);

    m_pContext->m_bitstream.pBitstream = (uint32_t*)m_pContext->m_pBufferStart + 2; // skip header
    m_pContext->m_bitstream.bitOffset = 31;

    sts = SequenceLayer(m_pContext);
    VC1_TO_UMC_CHECK_STS(sts);
    return UMC_OK;
}

Status VC1VideoDecoder::ContextAllocation(uint32_t mbWidth,uint32_t mbHeight)
{
    // need to extend for threading case
    if (!m_pContext)
    {

        uint8_t* ptr = NULL;
        ptr += mfx::align2_value(sizeof(VC1Context));
        ptr += mfx::align2_value(sizeof(VC1VLCTables));
        ptr += mfx::align2_value(sizeof(int16_t)*mbHeight*mbWidth*2*2);
        ptr += mfx::align2_value(mbHeight*mbWidth);
        ptr += mfx::align2_value(sizeof(VC1TSHeap));
        if(m_stCodes == NULL)
        {
            ptr += mfx::align2_value(START_CODE_NUMBER*2*sizeof(uint32_t)+sizeof(MediaDataEx::_MediaDataEx));
        }

        // Need to replace with MFX allocator
        if (m_pMemoryAllocator->Alloc(&m_iMemContextID,
                                      (size_t)ptr,
                                      UMC_ALLOC_PERSISTENT,
                                      16) != UMC_OK)
                                      return UMC_ERR_ALLOC;

        m_pContext = (VC1Context*)(m_pMemoryAllocator->Lock(m_iMemContextID));
        memset(m_pContext,0,(size_t)ptr);
        ptr = (uint8_t*)m_pContext;

        ptr += mfx::align2_value(sizeof(VC1Context));
        m_pContext->m_vlcTbl = (VC1VLCTables*)ptr;

        ptr += mfx::align2_value(sizeof(VC1VLCTables));
        ptr += mfx::align2_value(sizeof(int16_t)*mbHeight*mbWidth*2*2);

        ptr +=  mfx::align2_value(mbHeight*mbWidth);
        m_pHeap = (VC1TSHeap*)ptr;

        if(m_stCodes == NULL)
        {
            ptr += mfx::align2_value(sizeof(VC1TSHeap));
            m_stCodes = (MediaDataEx::_MediaDataEx *)(ptr);


            memset(reinterpret_cast<void*>(m_stCodes), 0, (START_CODE_NUMBER*2*sizeof(int32_t)+sizeof(MediaDataEx::_MediaDataEx)));
            m_stCodes->count      = 0;
            m_stCodes->index      = 0;
            m_stCodes->bstrm_pos  = 0;
            m_stCodes->offsets    = (uint32_t*)((uint8_t*)m_stCodes +
            sizeof(MediaDataEx::_MediaDataEx));
            m_stCodes->values     = (uint32_t*)((uint8_t*)m_stCodes->offsets +
            START_CODE_NUMBER*sizeof( uint32_t));
        }

    }
    return UMC_OK;
}
Status VC1VideoDecoder::StartCodesProcessing(uint8_t*   pBStream,
                                             uint32_t*  pOffsets,
                                             uint32_t*  pValues,
                                             bool     IsDataPrepare)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VC1VideoDecoder::StartCodesProcessing");
    Status umcRes = UMC_OK;
    uint32_t UnitSize;
    VC1Status sts = VC1_OK;
    while ((*pValues != 0x0D010000)&&
           (*pValues))
    {
        m_pContext->m_bitstream.pBitstream = (uint32_t*)((uint8_t*)m_frameData->GetDataPointer() + *pOffsets) + 1;//skip start code
        if(*(pOffsets + 1))
            UnitSize = *(pOffsets + 1) - *pOffsets;
        else
            UnitSize = (uint32_t)(m_pCurrentIn->GetBufferSize() - *pOffsets);

        if (!IsDataPrepare)
        {
            // copy data to self buffer
            MFX_INTERNAL_CPY(m_dataBuffer, pBStream + *pOffsets, UnitSize);
            SwapData(m_dataBuffer, mfx::align2_value(UnitSize));
            m_pContext->m_bitstream.pBitstream = (uint32_t*)m_dataBuffer + 1; //skip start code
        }
        m_pContext->m_bitstream.bitOffset  = 31;
        switch (*pValues)
        {
        case 0x0F010000:
            VC1Context context;
            size_t alignment;
           umcRes =  UMC_ERR_NOT_ENOUGH_DATA;
            context = *m_pContext;
            sts = SequenceLayer(&context);
            VC1_TO_UMC_CHECK_STS(sts);
            GetFPS(&context);
            alignment = (context.m_seqLayerHeader.INTERLACE)?32:16;


            if (mfx::align2_value(2*(context.m_seqLayerHeader.MAX_CODED_WIDTH+1)) > mfx::align2_value(2*(m_pInitContext.m_seqLayerHeader.MAX_CODED_WIDTH+1)))
                return UMC_ERR_INVALID_PARAMS;
            if (context.m_seqLayerHeader.INTERLACE != m_pInitContext.m_seqLayerHeader.INTERLACE )
                return UMC_ERR_INVALID_PARAMS;
            if (mfx::align2_value(2*(context.m_seqLayerHeader.MAX_CODED_HEIGHT+1),alignment) > mfx::align2_value(2*(m_pInitContext.m_seqLayerHeader.MAX_CODED_HEIGHT+1),alignment))
                return UMC_ERR_INVALID_PARAMS;
            // start codes are applicable for advanced profile only
            if (context.m_seqLayerHeader.PROFILE != VC1_PROFILE_ADVANCED)
                return UMC_ERR_INVALID_PARAMS;

            else if (!m_decoderInitFlag)
            {
                *m_pContext = context;
                m_decoderInitFlag = true;
            }
            else
            {
                *m_pContext = context;

                if (mfx::align2_value(2*(context.m_seqLayerHeader.MAX_CODED_WIDTH+1)) != mfx::align2_value(2*(m_pInitContext.m_seqLayerHeader.MAX_CODED_WIDTH+1))
                   || mfx::align2_value(2*(context.m_seqLayerHeader.MAX_CODED_HEIGHT+1),alignment) != mfx::align2_value(2*(m_pInitContext.m_seqLayerHeader.MAX_CODED_HEIGHT+1),alignment))
                {
                    m_pStore->SetNewSHParams(m_pContext);
                    umcRes = UMC_NTF_NEW_RESOLUTION;
                }
             }
            VC1_TO_UMC_CHECK_STS(sts);
            break;
        case 0x0A010000:
            umcRes =  UMC_ERR_NOT_ENOUGH_DATA;
            break;
        case 0x0E010000:
            umcRes = EntryPointLayer(m_pContext);
            if (UMC_OK != umcRes)
                return UMC_ERR_INVALID_PARAMS;
            umcRes = UMC_ERR_NOT_ENOUGH_DATA;
            break;
        case 0x0C010000:
        case 0x1B010000:
        case 0x1C010000:
        case 0x1D010000:
        case 0x1E010000:
        case 0x1F010000:
            umcRes = UMC_ERR_NOT_ENOUGH_DATA;
            break;
        default:
            umcRes = UMC_ERR_SYNC;
            break;

        }
        pValues++;
        pOffsets++;
    }

    if (((0x0D010000) == *pValues) && m_decoderInitFlag)// we have frame to decode
    {
        UnitSize = (uint32_t)(m_pCurrentIn->GetBufferSize() - *pOffsets);
        m_pContext->m_pBufferStart = ((uint8_t*)m_frameData->GetDataPointer() + *pOffsets); //skip start code
        if (!IsDataPrepare)
        {
            // copy frame data to self buffer
            MFX_INTERNAL_CPY(m_dataBuffer, 
                (uint8_t*)m_frameData->GetDataPointer() + *pOffsets,
                UnitSize);
            //use own buffer
            SwapData(m_dataBuffer, mfx::align2_value(UnitSize));
            m_pContext->m_pBufferStart = m_dataBuffer; //skip start code
        }
        else
            ++m_lFrameCount;

        try //work with queue of frame descriptors and process every frame
        {
            umcRes = VC1DecodeFrame(m_pCurrentIn,m_pCurrentOut);
        }
        catch (vc1_exception ex)
        {
            exception_type e_type = ex.get_exception_type();
            if ((e_type == internal_pipeline_error)||
                (e_type == mem_allocation_er))
                return UMC_ERR_FAILED;
        }
    }
    else if (m_decoderInitFlag && (!m_bIsNeedToFlush))
    {
        m_pCurrentIn->MoveDataPointer(m_pContext->m_FrameSize);
    }
    else
        umcRes = UMC_ERR_NOT_ENOUGH_DATA;

    return umcRes;
}

Status VC1VideoDecoder::SMProfilesProcessing(uint8_t* pBitstream)
{
    Status umcRes = UMC_OK;
    m_pContext->m_bitstream.pBitstream = (uint32_t*)pBitstream;
    m_pContext->m_bitstream.bitOffset  = 31;
    if ((*m_pContext->m_bitstream.pBitstream&0xFF) == 0xC5)// sequence header, exit after process.
    {
        VideoDecoderParams params;
        params.m_pData = m_pCurrentIn;
        params.lFlags = m_decoderFlags;
        params.info.stream_type = m_ClipInfo.stream_type;
        params.info.stream_subtype = m_ClipInfo.stream_subtype;
        params.numThreads = m_iThreadDecoderNum;
        params.info.clip_info.width = m_ClipInfo.clip_info.width;
        params.info.clip_info.height = m_ClipInfo.clip_info.height;
        params.m_SuggestedOutputSize = m_SurfaceNum;
        params.lpMemoryAllocator = m_pMemoryAllocator;

        bool deblocking = (int32_t)m_pStore->IsDeblockingOn();

        Close();
        umcRes = Init(&params);
        UMC_CHECK_STATUS(umcRes);

        //need to add code for restoring skip mode!
        if(!deblocking)
        {
            int32_t speed_shift = 0x22;
            m_pStore->ChangeVideoDecodingSpeed(speed_shift);
        }

        return UMC_ERR_NOT_ENOUGH_DATA;
    }

    m_pContext->m_pBufferStart  = (uint8_t*)pBitstream;
    ++m_lFrameCount;
    try //work with queue of frame descriptors and process every frame
    {
        umcRes = VC1DecodeFrame(m_pCurrentIn,m_pCurrentOut);
    }
    catch (vc1_exception ex)
    {
        exception_type e_type = ex.get_exception_type();
        if (e_type == internal_pipeline_error)
            return UMC_ERR_FAILED;
    }
    return umcRes;

}
Status VC1VideoDecoder::ParseStreamFromMediaData()
{
    Status umcRes = UMC_OK;

    // we have no start codes. Let find them
    if (VC1_PROFILE_ADVANCED == m_pContext->m_seqLayerHeader.PROFILE)
    {
        uint32_t readSize;
        m_frameData->GetExData()->count = 0;
        memset(m_frameData->GetExData()->offsets, 0,START_CODE_NUMBER*sizeof(int32_t));
        memset(m_frameData->GetExData()->values, 0,START_CODE_NUMBER*sizeof(int32_t));
        umcRes = GetStartCodes((uint8_t*)m_pCurrentIn->GetDataPointer(),
                               (uint32_t)m_pCurrentIn->GetDataSize(),
                               m_frameData,
                               &readSize); // parse and copy to self buffer

        SwapData((uint8_t*)m_frameData->GetDataPointer(), (uint32_t)m_frameData->GetDataSize());
        m_pContext->m_FrameSize = readSize;
        umcRes = StartCodesProcessing((uint8_t*)m_frameData->GetDataPointer(),
                                       m_frameData->GetExData()->offsets,
                                       m_frameData->GetExData()->values,
                                       true);

    }
    else //Simple/Main profiles pack without Start Codes
    {
        // copy data to self buffer
        MFX_INTERNAL_CPY(m_dataBuffer, 
                    (uint8_t*)m_pCurrentIn->GetDataPointer(),
                    (uint32_t)m_pCurrentIn->GetDataSize());

        m_pContext->m_FrameSize = (uint32_t)m_pCurrentIn->GetDataSize();
        m_frameData->SetDataSize(m_pContext->m_FrameSize);
        SwapData((uint8_t*)m_frameData->GetDataPointer(), (uint32_t)m_frameData->GetDataSize());
        umcRes = SMProfilesProcessing(m_dataBuffer);
    }
    return umcRes;
}

Status VC1VideoDecoder::ParseStreamFromMediaDataEx(MediaDataEx *in_ex)
{
    Status umcRes = UMC_OK;

    if ((in_ex->GetExData())&&
        (VC1_PROFILE_ADVANCED == m_pContext->m_seqLayerHeader.PROFILE))// we have already start codes from splitter. Advance profile only
    {
        m_pContext->m_Offsets = in_ex->GetExData()->offsets;
        m_pContext->m_values = in_ex->GetExData()->values;
        MFX_INTERNAL_CPY(m_dataBuffer, 
                    (uint8_t*)m_pCurrentIn->GetDataPointer(),
                    (uint32_t)m_pCurrentIn->GetDataSize());

        m_pContext->m_FrameSize = (uint32_t)m_pCurrentIn->GetDataSize();
        m_frameData->SetDataSize(m_pContext->m_FrameSize);
        SwapData((uint8_t*)m_frameData->GetDataPointer(), m_pContext->m_FrameSize);
        umcRes = StartCodesProcessing((uint8_t*)m_frameData->GetDataPointer(),
                                        m_pContext->m_Offsets,
                                        m_pContext->m_values,
                                        true);
    }
    else
    {
        // we have no start codes. Let find them
        if (VC1_PROFILE_ADVANCED == m_pContext->m_seqLayerHeader.PROFILE)
        {
            uint32_t readSize;
            m_frameData->GetExData()->count = 0;
            memset(m_frameData->GetExData()->offsets, 0,START_CODE_NUMBER*sizeof(int32_t));
            memset(m_frameData->GetExData()->values, 0,START_CODE_NUMBER*sizeof(int32_t));
            umcRes = GetStartCodes((uint8_t*)m_pCurrentIn->GetDataPointer(),
                                    (uint32_t)m_pCurrentIn->GetDataSize(),
                                    m_frameData,
                                    &readSize); // parse and copy to self buffer

            m_pContext->m_FrameSize = readSize;
            SwapData((uint8_t*)m_frameData->GetDataPointer(), (uint32_t)m_frameData->GetDataSize());
            umcRes = StartCodesProcessing((uint8_t*)m_frameData->GetDataPointer(),
                                            m_pContext->m_Offsets,
                                            m_pContext->m_values,
                                            true);
        }
        else //Simple/Main profiles pack without Start Codes
        {
            // copy data to self buffer
            MFX_INTERNAL_CPY(m_dataBuffer,
                        (uint8_t*)m_pCurrentIn->GetDataPointer(),
                        (uint32_t)m_pCurrentIn->GetDataSize());
            m_pContext->m_FrameSize = (uint32_t)m_pCurrentIn->GetDataSize();
            m_frameData->SetDataSize(m_pContext->m_FrameSize);
            SwapData((uint8_t*)m_frameData->GetDataPointer(), m_pContext->m_FrameSize);
            umcRes = SMProfilesProcessing(m_dataBuffer);
        }
    }

    return umcRes;
}

Status VC1VideoDecoder::ParseInputBitstream()
{
    Status umcRes = UMC_OK;
    MediaDataEx *in_ex = DynamicCast<MediaDataEx,MediaData>(m_pCurrentIn);
    if (in_ex)
        umcRes = ParseStreamFromMediaDataEx(in_ex);
    else
    {
        umcRes = ParseStreamFromMediaData();
    }
    return umcRes;
}

Status VC1VideoDecoder::GetFrame(MediaData* in, MediaData* out)
{
    Status umcRes = UMC_OK;
    VideoDecoderParams params;

    VideoData *out_data = DynamicCast<VideoData, MediaData>(out);

    if (NULL == out_data)
    {
        return UMC_ERR_NULL_PTR;
    }

    if(in!=NULL && (uint32_t)in->GetDataSize() == 0)
        return UMC_ERR_NOT_ENOUGH_DATA;

    if(!m_pContext)
        return UMC_ERR_NOT_INITIALIZED;

    m_pCurrentIn = in;
    if (out_data)
    {
        m_pCurrentOut = out_data;
        m_pCurrentOut->SetFrameType(NONE_PICTURE);
    }

    if (in == NULL)
    {
        // in should be always present
        return UMC_ERR_FAILED;
    }
    else
    {
        if (m_AllocBuffer < in->GetDataSize())
            return UMC_ERR_NOT_ENOUGH_BUFFER;

        umcRes = ParseInputBitstream();

        if (UMC_OK == umcRes ||
            (UMC_ERR_NOT_ENOUGH_DATA == umcRes && m_pCurrentOut->GetFrameType() != NONE_PICTURE))
        {
            if (out_data)
            {
                if(-1.0 == in->GetTime())
                {
                    out_data->SetTime(m_pts);
                    GetPTS(in->GetTime());
                }
                else
                {
                    out_data->SetTime(in->GetTime());
                    m_pts = in->GetTime();
                }
            }
        }
    }

    return umcRes;
}

Status VC1VideoDecoder::Close(void)
{
    Status umcRes = UMC_OK;

    m_AllocBuffer = 0;

    // reset all values 
    umcRes = Reset();

    if (m_pStore)
    {
        delete m_pStore;
        m_pStore = 0;
    }
    
    FreeAlloc(m_pContext);

    if(m_pMemoryAllocator)
    {
        if (static_cast<int>(m_iMemContextID) != -1)
        {
            m_pMemoryAllocator->Unlock(m_iMemContextID);
            m_pMemoryAllocator->Free(m_iMemContextID);
            m_iMemContextID = (MemID)-1;
        }

        if (static_cast<int>(m_iHeapID) != -1)
        {
            m_pMemoryAllocator->Unlock(m_iHeapID);
            m_pMemoryAllocator->Free(m_iHeapID);
            m_iHeapID = (MemID)-1;
        }

        if (static_cast<int>(m_iFrameBufferID) != -1)
        {
            m_pMemoryAllocator->Unlock(m_iFrameBufferID);
            m_pMemoryAllocator->Free(m_iFrameBufferID);
            m_iFrameBufferID = (MemID)-1;
        }
    }

    m_pContext = NULL;
    m_dataBuffer = NULL;
    m_stCodes = NULL;
    m_frameData = NULL;
    m_pHeap = NULL;

    memset(&m_pInitContext,0,sizeof(VC1Context));

    m_pMemoryAllocator = 0;


    m_iThreadDecoderNum = 0;
    m_decoderInitFlag = 0;
    m_decoderFlags = 0;
    return umcRes;
}

Status VC1VideoDecoder::Reset(void)
{
    Status umcRes = UMC_OK;

    if(m_pContext==NULL)
        return UMC_ERR_NOT_INITIALIZED;

    m_bLastFrameNeedDisplay = false;

    m_lFrameCount = 0;

    m_pContext->m_frmBuff.m_iDisplayIndex =  -1;
    m_pContext->m_frmBuff.m_iCurrIndex    =  -1;
    m_pContext->m_frmBuff.m_iPrevIndex    =  -1;
    m_pContext->m_frmBuff.m_iNextIndex    =  -1;

    m_pContext->DeblockInfo.HeightMB = 0;
    m_pContext->DeblockInfo.start_pos = 0;
    m_pContext->DeblockInfo.is_last_deblock = 1;
    m_pts = 0;
    m_bIsExternalFR = false;

    return umcRes;
}

void VC1VideoDecoder::GetPTS(double in_pts)
{
   if(in_pts == -1.0 && m_pts == -1.0)
       m_pts = 0;
   else if (in_pts == -1.0)
   {
       if(m_ClipInfo.framerate != 0.0)
       {
           m_pts = m_pts + 1.0/m_ClipInfo.framerate - m_pts_dif;
       }
       else
       {
           m_pts = m_pts + 1.0/24.0 - m_pts_dif;
       }
   }
   else
   {
       if(m_pts_dif == 0)
       {
           if(m_ClipInfo.framerate)
                m_pts = m_pts + 1.0/m_ClipInfo.framerate - m_pts_dif;
           else
                m_pts = m_pts + 1.0/24.0 - m_pts_dif;

           m_pts_dif = in_pts - m_pts;
       }
       else
       {
            if(m_ClipInfo.framerate)
                m_pts = m_pts + 1.0/m_ClipInfo.framerate - m_pts_dif;
            else
                m_pts = m_pts + 1.0/24.0 - m_pts_dif;
       }
   }
}

bool VC1VideoDecoder::GetFPS(VC1Context* pContext)
{
    // no need to modify frame rate if it set by application
    if (m_bIsExternalFR)
        return false;

    double prevFPS = m_ClipInfo.framerate;
    if((m_ClipInfo.stream_subtype == VC1_VIDEO_VC1)||(m_ClipInfo.stream_type == static_cast<int>(WVC1_VIDEO)))
    {
        m_ClipInfo.bitrate = pContext->m_seqLayerHeader.BITRTQ_POSTPROC;
        m_ClipInfo.framerate = pContext->m_seqLayerHeader.FRMRTQ_POSTPROC;

        //for advanced profile
        if ((m_ClipInfo.framerate == 0.0) && (m_ClipInfo.bitrate == 31))
            {
                //Post processing indicators for Frame Rate and Bit Rate are undefined
                m_ClipInfo.bitrate = 0;
            }
            else if ((m_ClipInfo.framerate == 0.0) && (m_ClipInfo.bitrate == 30))
            {
                m_ClipInfo.framerate =  2.0;
                m_ClipInfo.bitrate = 1952;
            }
            else if ((m_ClipInfo.framerate == 1.0) && (m_ClipInfo.bitrate == 31))
            {
                m_ClipInfo.framerate =  6.0;
                m_ClipInfo.bitrate = 2016;
            }
            else
            {
                if (m_ClipInfo.framerate == 7.0)
                    m_ClipInfo.framerate = 30.0;
                else
                    m_ClipInfo.framerate = (2.0 + m_ClipInfo.framerate*4);

                if (m_ClipInfo.bitrate == 31)
                    m_ClipInfo.bitrate = 2016;
                else
                    m_ClipInfo.bitrate = (32 + m_ClipInfo.bitrate * 64);
            }
    }
    else 
    {
        m_ClipInfo.bitrate = pContext->m_seqLayerHeader.BITRTQ_POSTPROC;
        m_ClipInfo.framerate = pContext->m_seqLayerHeader.FRMRTQ_POSTPROC;

        if (7.0 == m_ClipInfo.framerate)
            m_ClipInfo.framerate = 30.0;
        else if (m_ClipInfo.framerate != 0.0)
            m_ClipInfo.framerate = (2.0 + m_ClipInfo.framerate*4);
        else 
            m_ClipInfo.framerate = 24.0;
    }
    

    if(m_ClipInfo.framerate < 0.0)
        m_ClipInfo.framerate = 24.0;

    m_ClipInfo.framerate = MapFrameRateIntoUMC(pContext->m_seqLayerHeader.FRAMERATENR, 
                                               pContext->m_seqLayerHeader.FRAMERATEDR, 
                                               pContext->m_seqLayerHeader.FRMRTQ_POSTPROC); 

    return (prevFPS != m_ClipInfo.framerate);
}

Status VC1VideoDecoder::CreateFrameBuffer(uint32_t bufferSize)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VC1VideoDecoder::CreateFrameBuffer");
    if(m_dataBuffer == NULL)
    {
       if (m_pMemoryAllocator->Alloc(&m_iFrameBufferID,
                                     bufferSize,
                                     UMC_ALLOC_PERSISTENT,
                                     16) != UMC_OK)
                                     return UMC_ERR_ALLOC;
        m_dataBuffer = (uint8_t*)(m_pMemoryAllocator->Lock(m_iFrameBufferID));
        if(m_dataBuffer==NULL)
        {
            Close();
            return UMC_ERR_ALLOC;
        }
    }

    memset(m_dataBuffer,0,bufferSize);
    m_pContext->m_pBufferStart  = (uint8_t*)m_dataBuffer;
    m_pContext->m_bitstream.pBitstream       = (uint32_t*)(m_pContext->m_pBufferStart);

    if(m_frameData == NULL)
    {
        m_pHeap->s_new(&m_frameData);
    }

    m_frameData->SetBufferPointer(m_dataBuffer, bufferSize);
    m_frameData->SetDataSize(bufferSize);
    m_frameData->SetExData(m_stCodes);

    return UMC_OK;
}

Status VC1VideoDecoder::GetStartCodes (uint8_t* pDataPointer,
                                       uint32_t DataSize,
                                       MediaDataEx* out,
                                       uint32_t* readSize)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VC1VideoDecoder::GetStartCodes");
    uint8_t* readPos = pDataPointer;
    uint32_t readBufSize =  DataSize;
    uint8_t* readBuf = pDataPointer;

    uint8_t* currFramePos = (uint8_t*)out->GetBufferPointer();
    uint32_t frameSize = 0;
    uint32_t frameBufSize = (uint32_t)out->GetBufferSize();
    MediaDataEx::_MediaDataEx *stCodes = out->GetExData();

    uint32_t size = 0;
    uint8_t* ptr = NULL;
    uint32_t readDataSize = 0;
    uint32_t zeroNum = 0;
    uint32_t a = 0x0000FF00 | (*readPos);
    uint32_t b = 0xFFFFFFFF;
    uint32_t FrameNum = 0;
    
    uint32_t shift = 0;

    bool isWriteSlice = false;

    memset(stCodes->offsets, 0,START_CODE_NUMBER*sizeof(int32_t));
    memset(stCodes->values, 0,START_CODE_NUMBER*sizeof(int32_t));

    while(readPos < (readBuf + readBufSize))
    {
        if (stCodes->count > 512)
            return UMC_ERR_INVALID_STREAM;
        //find sequence of 0x000001 or 0x000003
        while(!( b == 0x00000001 || b == 0x00000003 )
            &&(++readPos < (readBuf + readBufSize)))
        {
            a = (a<<8)| (int32_t)(*readPos);
            b = a & 0x00FFFFFF;
        }

        // check small bitstream
        if (stCodes->count == 0 && readBufSize < 4)
        {
            return UMC_ERR_NOT_ENOUGH_DATA;
        }

        //check end of read buffer
        if(readPos < (readBuf + readBufSize - 1))
        {
            if(*readPos == 0x01)
            {
                if ((IS_VC1_DATA_SC(*(readPos + 1)) || IS_VC1_USER_DATA(*(readPos + 1))) &&
                    ( ((stCodes->count > 0 &&
                      IS_VC1_DATA_SC(stCodes->values[0] >> 24)) ||
                      isWriteSlice)))
                {
                    isWriteSlice = false;
                    readPos+=2;
                    ptr = readPos - 5;
                    if (stCodes->count) // if not first start code
                    {
                        //trim zero bytes
                        while ( (*ptr==0) && (ptr > readBuf) )
                            ptr--;
                    }

                    //slice or field size
                    size = (uint32_t)(ptr - readBuf - readDataSize+1);

                    if(frameSize + size > frameBufSize)
                        return UMC_ERR_NOT_ENOUGH_BUFFER;

                    MFX_INTERNAL_CPY(currFramePos, readBuf + readDataSize, size);

                    currFramePos = currFramePos + size;
                    frameSize = frameSize + size;

                    zeroNum = frameSize - 4*((frameSize)/4);
                    if(zeroNum!=0)
                        zeroNum = 4 - zeroNum;

                    memset(currFramePos, 0, zeroNum);

                    //set write parameters
                    currFramePos = currFramePos + zeroNum;
                    frameSize = frameSize + zeroNum;

                    stCodes->offsets[stCodes->count] = frameSize;
                    stCodes->values[stCodes->count]  = ((*(readPos-1))<<24) + ((*(readPos-2))<<16) + ((*(readPos-3))<<8) + (*(readPos-4));

                    readDataSize = (uint32_t)(readPos - readBuf - 4);

                    a = 0x00010b00 |(int32_t)(*readPos);
                    b = a & 0x00FFFFFF;

                    zeroNum = 0;
                    stCodes->count++;
                }
                else
                {
                    if (stCodes->count && IS_VC1_USER_DATA(stCodes->values[stCodes->count - 1] >> 24))
                    {
                        if (IS_VC1_DATA_SC(*(readPos + 1)))
                            isWriteSlice = true;
                        else
                            isWriteSlice = false;

                        readPos+=2;
                        a = (a<<8)| (int32_t)(*readPos);
                        b = a & 0x00FFFFFF;
                        readDataSize = (uint32_t)(readPos - readBuf - 4);
                        continue;
                    }
                    else if(FrameNum)
                    {
                        readPos+=2;
                        ptr = readPos - 5;
                        //trim zero bytes
                        if (stCodes->count) // if not first start code
                        {
                            while ( (*ptr==0) && (ptr > readBuf) )
                                ptr--;
                        }

                        //slice or field size
                        size = (uint32_t)(readPos - readBuf - readDataSize - 4);

                        if(frameSize + size > frameBufSize)
                            return UMC_ERR_NOT_ENOUGH_BUFFER;

                        MFX_INTERNAL_CPY(currFramePos, readBuf + readDataSize, size);

                        //currFramePos = currFramePos + size;
                        frameSize = frameSize + size;

                        stCodes->offsets[stCodes->count] = frameSize;
                        stCodes->values[stCodes->count]  = ((*(readPos-1))<<24) + ((*(readPos-2))<<16) + ((*(readPos-3))<<8) + (*(readPos-4));

                        stCodes->count++;

                        out->SetDataSize(frameSize + shift);
                        readDataSize = readDataSize + size;
                        *readSize = readDataSize;
                        return UMC_OK;
                    }
                    else 
                    {
                        //beginning of frame
                        readPos++;
                        a = 0x00000100 |(int32_t)(*readPos);
                        b = a & 0x00FFFFFF;

                        //end of seguence
                        if((((*(readPos))<<24) + ((*(readPos-1))<<16) + ((*(readPos-2))<<8) + (*(readPos-3))) == 0x0A010000)
                        {
                            *readSize = 4;
                            stCodes->offsets[stCodes->count] = (uint32_t)(0);
                            stCodes->values[stCodes->count]  = ((*(readPos))<<24) + ((*(readPos-1))<<16) + ((*(readPos-2))<<8) + (*(readPos-3));
                            stCodes->count++;
                            out->SetDataSize(4);      
                            MFX_INTERNAL_CPY(currFramePos, readBuf + readDataSize, 4);
                            return UMC_OK;
                        }

                        stCodes->offsets[stCodes->count] = (uint32_t)(0);
                        stCodes->values[stCodes->count]  = ((*(readPos))<<24) + ((*(readPos-1))<<16) + ((*(readPos-2))<<8) + (*(readPos-3));

                        stCodes->count++;
                        zeroNum = 0;
                        FrameNum++;
                    }
                }
            }
            else //if(*readPos == 0x03)
            {
                //000003
                if((*(readPos + 1) <  0x04))
                {
                    size = (uint32_t)(readPos - readBuf - readDataSize);
                    if(frameSize + size > frameBufSize)
                        return UMC_ERR_NOT_ENOUGH_BUFFER;

                    MFX_INTERNAL_CPY(currFramePos, readBuf + readDataSize, size);
                    frameSize = frameSize + size;
                    currFramePos = currFramePos + size;
                    zeroNum = 0;

                    readPos++;
                    a = (a<<8)| (int32_t)(*readPos);
                    b = a & 0x00FFFFFF;

                    readDataSize = readDataSize + size + 1;
                }
                else
                {
                    readPos++;
                    a = (a<<8)| (int32_t)(*readPos);
                    b = a & 0x00FFFFFF;
                }
            }
        }
        else
        {
            // last portion pf user data
            if (!IS_VC1_USER_DATA(stCodes->values[stCodes->count - 1] >> 24) || isWriteSlice)
            {
                //end of stream
                size = (uint32_t)(readPos- readBuf - readDataSize);

                if(frameSize + size > frameBufSize)
                {
                    return UMC_ERR_NOT_ENOUGH_BUFFER;
                }

                MFX_INTERNAL_CPY(currFramePos, readBuf + readDataSize, size);
            }
            else
            {
                stCodes->values[stCodes->count - 1] = 0;
                stCodes->offsets[stCodes->count - 1] = 0;
            }

            out->SetDataSize(frameSize + size + shift);
            readDataSize = readDataSize + size;
            *readSize = readDataSize;
            return UMC_OK;
        }
    }
    return UMC_OK;
}


void VC1VideoDecoder::GetFrameSize(MediaData* in)
{
    uint32_t i = 0;
    uint32_t* offset = m_pContext->m_Offsets;
    uint32_t* value = m_pContext->m_values;

    m_pContext->m_FrameSize = (uint32_t)in->GetDataSize();

    if(m_pContext->m_seqLayerHeader.PROFILE == VC1_PROFILE_ADVANCED)
    {
        value++;
        i++;
        while((*value == 0x0B010000)||(*value == 0x0C010000))
        {
            value ++;
            i++;
        }
        if(*value != 0x00000000)
        {
            m_pContext->m_FrameSize = offset[i];
        }
    }
}

Status VC1VideoDecoder::VC1DecodeFrame(MediaData* , VideoData* )
{
    return UMC_ERR_FAILED;
}

bool VC1VideoDecoder::InitTables(VC1Context* pContext)
{
    //BITPLANE
    if (0 != HuffmanTableInitAlloc(
        VC1_Bitplane_IMODE_tbl,
        &pContext->m_vlcTbl->m_Bitplane_IMODE))
        return false;

    if (0 != HuffmanTableInitAlloc(
        VC1_BitplaneTaledbitsTbl,
        &pContext->m_vlcTbl->m_BitplaneTaledbits))
        return false;

    //BFRACTION
    if (0 != HuffmanRunLevelTableInitAlloc(
        VC1_BFraction_tbl,
        &pContext->m_vlcTbl->BFRACTION))
        return false;

    //REFDIST
    if (0 != HuffmanTableInitAlloc(
        VC1_FieldRefdistTable,
        &pContext->m_vlcTbl->REFDIST_TABLE))
        return false;

    return true;
}

void VC1VideoDecoder::FreeTables(VC1Context* pContext)
{
    if (pContext->m_vlcTbl->m_Bitplane_IMODE)
    {
        HuffmanTableFree(pContext->m_vlcTbl->m_Bitplane_IMODE);
        pContext->m_vlcTbl->m_Bitplane_IMODE = NULL;
    }

    if (pContext->m_vlcTbl->m_BitplaneTaledbits)
    {
        HuffmanTableFree(pContext->m_vlcTbl->m_BitplaneTaledbits);
        pContext->m_vlcTbl->m_BitplaneTaledbits = NULL;
    }

    if (pContext->m_vlcTbl->REFDIST_TABLE)
    {
        HuffmanTableFree(pContext->m_vlcTbl->REFDIST_TABLE);
        pContext->m_vlcTbl->REFDIST_TABLE = NULL;
    }

    if (pContext->m_vlcTbl->BFRACTION)
    {
        HuffmanTableFree(pContext->m_vlcTbl->BFRACTION);
        pContext->m_vlcTbl->BFRACTION = NULL;
    }
}

void   VC1VideoDecoder::FreeAlloc(VC1Context* pContext)
{
    if(pContext)
    {
        FreeTables(pContext);
    }
}

Status VC1VideoDecoder::ChangeVideoDecodingSpeed(int32_t& speed_shift)
{
    if (m_pStore->ChangeVideoDecodingSpeed(speed_shift))
        return UMC_OK;
    else
        return UMC_ERR_FAILED;
}

Status VC1VideoDecoder::CheckLevelProfileOnly(VideoDecoderParams *pParam)
{
    uint32_t Profile;
    // we can init without data. Hence we cannot check profile
    UMC_CHECK_PTR(pParam->m_pData);
    uint32_t* pData = (uint32_t*)pParam->m_pData->GetDataPointer();
    // we can init without data. Hence we cannot check profile
    UMC_CHECK_PTR(pData);
    if ((WVC1_VIDEO == pParam->info.stream_subtype)||
        ((VC1_VIDEO_VC1) == pParam->info.stream_subtype))
    {
        //First double word can be start code
        if  (0x0F010000 == *pData)
            pData += 1;
    }
    // May be need to add pParam->profile too. Chech first two bits
    Profile = ((*pData)&0xC0) >> 6;
    if (VC1_IS_VALID_PROFILE(Profile))
        return UMC_OK;
    else
        return UMC_ERR_UNSUPPORTED;
}

void VC1VideoDecoder::SetCorrupted(UMC::VC1FrameDescriptor *pCurrDescriptor, mfxU16& Corrupted)
{
    Corrupted = 0;

    if (NULL == pCurrDescriptor)
    {
        pCurrDescriptor = m_pStore->GetLastDS();
    }
    mfxU32 Ptype = pCurrDescriptor->m_pContext->m_picLayerHeader->PTYPE;

    if (VC1_IS_PRED(Ptype) || VC1_IS_SKIPPED(Ptype))
    {
        if (pCurrDescriptor->m_pContext->m_frmBuff.m_iPrevIndex > -1)
        {
            if (pCurrDescriptor->m_pContext->m_frmBuff.m_pFrames[pCurrDescriptor->m_pContext->m_frmBuff.m_iPrevIndex].corrupted)
                pCurrDescriptor->m_pContext->m_frmBuff.m_pFrames[pCurrDescriptor->m_pContext->m_frmBuff.m_iCurrIndex].corrupted |= MFX_CORRUPTION_REFERENCE_FRAME;
        }

        if (pCurrDescriptor->m_pContext->m_frmBuff.m_iNextIndex > -1)
        {
            if (pCurrDescriptor->m_pContext->m_frmBuff.m_pFrames[pCurrDescriptor->m_pContext->m_frmBuff.m_iNextIndex].corrupted)
                pCurrDescriptor->m_pContext->m_frmBuff.m_pFrames[pCurrDescriptor->m_pContext->m_frmBuff.m_iCurrIndex].corrupted |= MFX_CORRUPTION_REFERENCE_FRAME;
        }
    }

    if (pCurrDescriptor->m_pContext->m_frmBuff.m_iDisplayIndex > -1)
    {
        Corrupted = pCurrDescriptor->m_pContext->m_frmBuff.m_pFrames[pCurrDescriptor->m_pContext->m_frmBuff.m_iDisplayIndex].corrupted;
    }
}

bool VC1VideoDecoder::IsFrameSkipped()
{
    UMC::VC1FrameDescriptor *pCurrDescriptor = m_pStore->GetFirstDS();
    if (pCurrDescriptor)
    {
        return (VC1_IS_SKIPPED(pCurrDescriptor->m_pContext->m_picLayerHeader->PTYPE));
    }
    else
        return false;
}

bool VC1VideoDecoder::IsLastFrameSkipped()
{
    UMC::VC1FrameDescriptor *pCurrDescriptor = m_pStore->GetLastDS();
    if (pCurrDescriptor)
    {
        return (VC1_IS_SKIPPED(pCurrDescriptor->m_pContext->m_picLayerHeader->PTYPE));
    }
    else
        return false;

}

FrameMemID  VC1VideoDecoder::GetDisplayIndex(bool isDecodeOrder, bool )
{
    UMC::VC1FrameDescriptor *pCurrDescriptor = 0;
    pCurrDescriptor = m_pStore->GetLastDS();
    if (!pCurrDescriptor)
        return -1;

    if ((m_pContext->m_seqLayerHeader.RANGE_MAPY_FLAG) ||
        (m_pContext->m_seqLayerHeader.RANGE_MAPUV_FLAG) ||
        (pCurrDescriptor->m_pContext->m_seqLayerHeader.RANGERED))
    {
        if (isDecodeOrder)
            return m_pStore->GetIdx(pCurrDescriptor->m_pContext->m_frmBuff.m_iRangeMapIndex);

        if (VC1_IS_REFERENCE(pCurrDescriptor->m_pContext->m_picLayerHeader->PTYPE))
        {
            if (VC1_IS_SKIPPED(pCurrDescriptor->m_pContext->m_picLayerHeader->PTYPE))
                return m_pStore->GetIdx(pCurrDescriptor->m_pContext->m_frmBuff.m_iRangeMapIndex);
            else
                return m_pStore->GetIdx(pCurrDescriptor->m_pContext->m_frmBuff.m_iRangeMapIndexPrev);
        }
        else
            return m_pStore->GetIdx(pCurrDescriptor->m_pContext->m_frmBuff.m_iRangeMapIndex);
    }
    else
    {
        return (isDecodeOrder) ? (m_pStore->GetIdx(pCurrDescriptor->m_pContext->m_frmBuff.m_iCurrIndex)) :
            (m_pStore->GetIdx(pCurrDescriptor->m_pContext->m_frmBuff.m_iDisplayIndex));
    }
}

FrameMemID  VC1VideoDecoder::GetLastDisplayIndex()
{
    VC1FrameDescriptor *pCurrDescriptor = NULL;
    pCurrDescriptor = m_pStore->GetLastDS();
    if (m_bLastFrameNeedDisplay)
    {
        FrameMemID dispIndex;
        m_bLastFrameNeedDisplay = false;

        if ((!VC1_IS_REFERENCE(pCurrDescriptor->m_pContext->m_picLayerHeader->PTYPE)) ||
            (VC1_IS_SKIPPED(pCurrDescriptor->m_pContext->m_picLayerHeader->PTYPE)))
            dispIndex = pCurrDescriptor->m_pContext->m_frmBuff.m_iNextIndex;
        else
            dispIndex = pCurrDescriptor->m_pContext->m_frmBuff.m_iCurrIndex;

        if ((m_pContext->m_seqLayerHeader.RANGE_MAPY_FLAG) ||
            (m_pContext->m_seqLayerHeader.RANGE_MAPUV_FLAG) ||
            (pCurrDescriptor->m_pContext->m_seqLayerHeader.RANGERED))
        {
            if (!VC1_IS_REFERENCE(pCurrDescriptor->m_pContext->m_picLayerHeader->PTYPE))
                return m_pStore->GetIdx(pCurrDescriptor->m_pContext->m_frmBuff.m_iRangeMapIndexPrev);
            else
                return m_pStore->GetIdx(pCurrDescriptor->m_pContext->m_frmBuff.m_iRangeMapIndex);
        }
        else
            return m_pStore->GetIdx(dispIndex);
    }
    else
        return -2;
}

Status  VC1VideoDecoder::SetRMSurface()
{
    FrameMemID RMindex = -1;
    VC1FrameDescriptor *pCurrDescriptor = NULL;
    pCurrDescriptor = m_pStore->GetLastDS();
    m_pStore->LockSurface(&RMindex);
    if (RMindex < 0)
        return UMC_ERR_ALLOC;

    if (VC1_IS_REFERENCE(pCurrDescriptor->m_pContext->m_picLayerHeader->PTYPE))
    {
        pCurrDescriptor->m_pContext->m_frmBuff.m_iRangeMapIndexPrev = m_pStore->GetRangeMapIndex();
        pCurrDescriptor->m_pContext->m_frmBuff.m_iRangeMapIndex = RMindex;
        m_pStore->SetRangeMapIndex(RMindex);
    }
    else
    {
        pCurrDescriptor->m_pContext->m_frmBuff.m_iRangeMapIndexPrev = m_pStore->GetRangeMapIndex();
        pCurrDescriptor->m_pContext->m_frmBuff.m_iRangeMapIndex = RMindex;
    }
    return UMC_OK;
}

void VC1VideoDecoder::UnlockSurfaces()
{
    if (m_CurrIndexToFree > -1)
        m_pStore->UnLockSurface(m_CurrIndexToFree);
    if (m_RMIndexToFree > -1)
        m_pStore->UnLockSurface(m_RMIndexToFree);
}

UMC::FrameMemID VC1VideoDecoder::GetSkippedIndex(UMC::VC1FrameDescriptor *desc, bool isIn)
{
    UMC::VC1FrameDescriptor *pCurrDescriptor = desc;
    if (!pCurrDescriptor)
        return -1;
    if (!VC1_IS_SKIPPED(pCurrDescriptor->m_pContext->m_picLayerHeader->PTYPE))
        return -1;
    if (isIn)
    {
        if ((m_pContext->m_seqLayerHeader.RANGE_MAPY_FLAG) ||
            (m_pContext->m_seqLayerHeader.RANGE_MAPUV_FLAG) ||
            (pCurrDescriptor->m_pContext->m_seqLayerHeader.RANGERED))
            return m_pStore->GetIdx(pCurrDescriptor->m_pContext->m_frmBuff.m_iRangeMapIndex);
        else
            return m_pStore->GetIdx(pCurrDescriptor->m_pContext->m_frmBuff.m_iDisplayIndex);
    }
    else
        return m_pStore->GetIdx(pCurrDescriptor->m_pContext->m_frmBuff.m_iToSkipCoping);
}

FrameMemID VC1VideoDecoder::GetFrameOrder(bool isLast, bool isSamePolar, uint32_t & frameOrder)
{
    UMC::VC1FrameDescriptor *pCurrDescriptor = 0;
    mfxI32 idx;
    pCurrDescriptor = m_pStore->GetLastDS();

    if (0xFFFFFFFE == m_frameOrder)
        m_frameOrder = 0;

    bool rmap = false;
    if ((m_pContext->m_seqLayerHeader.RANGE_MAPY_FLAG) ||
        (m_pContext->m_seqLayerHeader.RANGE_MAPUV_FLAG) ||
        (pCurrDescriptor->m_pContext->m_seqLayerHeader.RANGERED))
        rmap = true;

    // non last frame
    if (!isLast)
    {
        if (VC1_IS_REFERENCE(pCurrDescriptor->m_pContext->m_InitPicLayer->PTYPE))
        {
            if (1 == pCurrDescriptor->m_iFrameCounter)
            {
                idx = rmap ? pCurrDescriptor->m_pContext->m_frmBuff.m_iRangeMapIndex : pCurrDescriptor->m_pContext->m_frmBuff.m_iCurrIndex;
                // Only if external surfaces
                m_frameOrder = 0;
                frameOrder = 0xFFFFFFFF;
                return m_pStore->GetIdx(idx);
            }
            else
            {
                idx = rmap ? pCurrDescriptor->m_pContext->m_frmBuff.m_iRangeMapIndexPrev : pCurrDescriptor->m_pContext->m_frmBuff.m_iPrevIndex;

                if (VC1_IS_SKIPPED(pCurrDescriptor->m_pContext->m_picLayerHeader->PTYPE))
                {
                    if (isSamePolar)
                        idx = pCurrDescriptor->m_pContext->m_frmBuff.m_iToSkipCoping;
                    else
                        idx = rmap ? pCurrDescriptor->m_pContext->m_frmBuff.m_iRangeMapIndex : pCurrDescriptor->m_pContext->m_frmBuff.m_iCurrIndex; //!!!!!!!!!Curr
                }
            }
        }
        else
        {
            idx = rmap ? pCurrDescriptor->m_pContext->m_frmBuff.m_iRangeMapIndex : pCurrDescriptor->m_pContext->m_frmBuff.m_iCurrIndex;
        }
    }
    else
    {
        if (!VC1_IS_REFERENCE(pCurrDescriptor->m_pContext->m_picLayerHeader->PTYPE))
            idx = rmap ? pCurrDescriptor->m_pContext->m_frmBuff.m_iRangeMapIndexPrev : pCurrDescriptor->m_pContext->m_frmBuff.m_iNextIndex;
        else
            idx = rmap ? pCurrDescriptor->m_pContext->m_frmBuff.m_iRangeMapIndex : pCurrDescriptor->m_pContext->m_frmBuff.m_iCurrIndex;
    }

    frameOrder = m_frameOrder;
    m_frameOrder++;
    return m_pStore->GetIdx(idx);
}


#endif //MFX_ENABLE_VC1_VIDEO_DECODE

