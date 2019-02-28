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

#ifndef __UMC_VC1_VIDEO_DECODER_H_
#define __UMC_VC1_VIDEO_DECODER_H_

#include "umc_video_decoder.h"
#include "umc_vc1_dec_frame_descr.h"
#include "umc_media_data_ex.h"
#include "umc_frame_allocator.h"

#include "umc_va_base.h"

#include "umc_vc1_dec_skipping.h"

class MFXVideoDECODEVC1;
namespace UMC
{

    class VC1TSHeap;
    class VC1TaskStoreSW;

    class VC1VideoDecoder
    {
        friend class VC1TaskStore;
        friend class VC1TaskStoreSW;
        friend class ::MFXVideoDECODEVC1;

    public:
        // Default constructor
        VC1VideoDecoder();
        // Default destructor
        virtual ~VC1VideoDecoder();

        // Initialize for subsequent frame decoding.
        virtual Status Init(BaseCodecParams *init);

        // Get next frame
        virtual Status GetFrame(MediaData* in, MediaData* out);
        // Close  decoding & free all allocated resources
        virtual Status Close(void);

        // Reset decoder to initial state
        virtual Status   Reset(void);

        // Perfomance tools. Improve speed or quality.
        //Accelarate Decoder (remove some features like deblockng, smoothing or change InvTransform and Quant)
        // speed_mode - return current mode

        // Change Decoding speed
        Status ChangeVideoDecodingSpeed(int32_t& speed_shift);

        void SetExtFrameAllocator(FrameAllocator*  pFrameAllocator) {m_pExtFrameAllocator = pFrameAllocator;}

        void SetVideoHardwareAccelerator            (VideoAccelerator* va);

    protected:

        Status CreateFrameBuffer(uint32_t bufferSize);

        void      GetFrameSize                         (MediaData* in);
        void      GetPTS                               (double in_pts);
        bool      GetFPS                               (VC1Context* pContext);

        virtual void FreeTables(VC1Context* pContext);
        virtual bool InitTables(VC1Context* pContext);
        virtual bool InitAlloc(VC1Context* pContext, uint32_t MaxFrameNum) = 0;
        virtual bool InitVAEnvironment() = 0;
        void    FreeAlloc(VC1Context* pContext);

        virtual Status VC1DecodeFrame                 (MediaData* in, VideoData* out_data);


        virtual uint32_t CalculateHeapSize() = 0;

        Status GetStartCodes                  (uint8_t* pDataPointer,
                                               uint32_t DataSize,
                                               MediaDataEx* out,
                                               uint32_t* readSize);
        Status SMProfilesProcessing(uint8_t* pBitstream);
        virtual Status ContextAllocation(uint32_t mbWidth,uint32_t mbHeight);

        Status StartCodesProcessing(uint8_t*   pBStream,
                                    uint32_t*  pOffsets,
                                    uint32_t*  pValues,
                                    bool     IsDataPrepare);

        Status ParseStreamFromMediaData     ();
        Status ParseStreamFromMediaDataEx   (MediaDataEx *in_ex);
        Status ParseInputBitstream          ();
        Status InitSMProfile                ();

        uint32_t  GetCurrentFrameSize()
        {
            if (m_dataBuffer)
                return (uint32_t)m_frameData->GetDataSize();
            else
                return (uint32_t)m_pCurrentIn->GetDataSize();
        }

        Status CheckLevelProfileOnly(VideoDecoderParams *pParam);

        VideoStreamInfo         m_ClipInfo;
        MemoryAllocator        *m_pMemoryAllocator;
        
        VC1Context*                m_pContext;
        VC1Context                 m_pInitContext;
        uint32_t                     m_iThreadDecoderNum;                     // (uint32_t) number of slice decoders
        uint8_t*                     m_dataBuffer;                            //uses for swap data into decoder
        MediaDataEx*               m_frameData;                             //uses for swap data into decoder
        MediaDataEx::_MediaDataEx* m_stCodes;
        uint32_t                     m_decoderInitFlag;
        uint32_t                     m_decoderFlags;

        UMC::MemID                 m_iMemContextID;
        UMC::MemID                 m_iHeapID;
        UMC::MemID                 m_iFrameBufferID;

        double                     m_pts;
        double                     m_pts_dif;

        uint32_t                     m_iMaxFramesInProcessing;

        unsigned long long                     m_lFrameCount;
        bool                       m_bLastFrameNeedDisplay;
        VC1TaskStore*              m_pStore;
        VideoAccelerator*          m_va;
        VC1TSHeap*                 m_pHeap;

        bool                       m_bIsReorder;
        MediaData*                 m_pCurrentIn;
        VideoData*                 m_pCurrentOut;

        bool                       m_bIsNeedToFlush;
        uint32_t                     m_AllocBuffer;

        FrameAllocator*            m_pExtFrameAllocator;
        uint32_t                     m_SurfaceNum;

        bool                       m_bIsExternalFR;

        static const uint32_t        NumBufferedFrames = 0;
        static const uint32_t        NumReferenceFrames = 3;

        virtual UMC::FrameMemID     ProcessQueuesForNextFrame(bool& isSkip, mfxU16& Corrupted) = 0;

        void SetCorrupted(UMC::VC1FrameDescriptor *pCurrDescriptor, mfxU16& Corrupted);
        bool IsFrameSkipped();
        bool IsLastFrameSkipped();
        virtual FrameMemID  GetDisplayIndex(bool isDecodeOrder, bool isSamePolarSurf);
        FrameMemID  GetLastDisplayIndex();
        virtual UMC::Status  SetRMSurface();
        void UnlockSurfaces();
        virtual UMC::FrameMemID GetSkippedIndex(bool isIn = true) = 0;
        FrameMemID GetFrameOrder(bool isLast, bool isSamePolar, uint32_t & frameOrder);
        virtual Status RunThread(int /*threadNumber*/) { return UMC_OK; }

        UMC::VC1FrameDescriptor* m_pDescrToDisplay;
        mfxU32                   m_frameOrder;
        UMC::FrameMemID               m_RMIndexToFree;
        UMC::FrameMemID               m_CurrIndexToFree;

    protected:
        UMC::FrameMemID GetSkippedIndex(UMC::VC1FrameDescriptor *desc, bool isIn);
    };

}

#endif //__UMC_VC1_VIDEO_DECODER_H
#endif //MFX_ENABLE_VC1_VIDEO_DECODE

