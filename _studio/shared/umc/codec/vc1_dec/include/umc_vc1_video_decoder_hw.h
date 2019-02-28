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

#ifndef __UMC_VC1_VIDEO_DECODER_HW_H_
#define __UMC_VC1_VIDEO_DECODER_HW_H_

#include "umc_vc1_video_decoder.h"
#include "umc_vc1_dec_frame_descr.h"
#include "umc_media_data_ex.h"
#include "umc_frame_allocator.h"

#include "umc_va_base.h"

#include "umc_vc1_dec_skipping.h"
#include "umc_vc1_dec_task_store.h"

class MFXVC1VideoDecoderHW;

namespace UMC
{

    class VC1TSHeap;
    class VC1VideoDecoderHW : public VC1VideoDecoder
    {
        friend class VC1TaskStore;

    public:
        // Default constructor
        VC1VideoDecoderHW();
        // Default destructor
        virtual ~VC1VideoDecoderHW();

        // Initialize for subsequent frame decoding.
        virtual Status Init(BaseCodecParams *init);

        virtual Status Reset(void);

        // Close  decoding & free all allocated resources
        virtual Status Close(void);

        void SetVideoHardwareAccelerator            (VideoAccelerator* va);


    protected:

        virtual   bool    InitAlloc                   (VC1Context* pContext, uint32_t MaxFrameNum);

        virtual Status VC1DecodeFrame                 (MediaData* in, VideoData* out_data);


        virtual uint32_t CalculateHeapSize();

        virtual bool   InitVAEnvironment            ();

        // HW i/f support
        virtual Status FillAndExecute(MediaData* in);
        void GetStartCodes_HW(MediaData* in, uint32_t &sShift);

        MediaDataEx::_MediaDataEx* m_stCodes_VA;

        template <class Descriptor>
        Status VC1DecodeFrame_VLD(MediaData* in, VideoData* out_data)
        {
            int32_t SCoffset = 0;
            Descriptor* pPackDescriptorChild = NULL;
            bool skip = false;

            if ((VC1_PROFILE_ADVANCED != m_pContext->m_seqLayerHeader.PROFILE))
                // special header (with frame size) in case of .rcv format
                SCoffset = -VC1FHSIZE;

            Status umcRes = UMC_ERR_NOT_ENOUGH_DATA;
            try
            {
                // Get FrameDescriptor for Packing Data
                m_pStore->GetReadyDS(&pPackDescriptorChild);

                if (NULL == pPackDescriptorChild)
                    throw UMC::VC1Exceptions::vc1_exception(UMC::VC1Exceptions::internal_pipeline_error);


                pPackDescriptorChild->m_pContext->m_FrameSize = (uint32_t)in->GetDataSize() + SCoffset;


                umcRes = pPackDescriptorChild->preProcData(m_pContext,
                    (uint32_t)(m_frameData->GetDataSize() + SCoffset),
                    m_lFrameCount,
                    skip);
                if (UMC_OK != umcRes)
                {

                    if (UMC_ERR_NOT_ENOUGH_DATA == umcRes)
                        throw UMC::VC1Exceptions::vc1_exception(UMC::VC1Exceptions::invalid_stream);
                    else
                        throw UMC::VC1Exceptions::vc1_exception(UMC::VC1Exceptions::internal_pipeline_error);
                }
            }
            catch (UMC::VC1Exceptions::vc1_exception ex)
            {
                if (UMC::VC1Exceptions::invalid_stream == ex.get_exception_type())
                {
                    if (UMC::VC1Exceptions::fast_err_detect == UMC::VC1Exceptions::vc1_except_profiler::GetEnvDescript().m_Profile)
                        m_pStore->AddInvalidPerformedDS(pPackDescriptorChild);
                    else if (UMC::VC1Exceptions::fast_decoding == UMC::VC1Exceptions::vc1_except_profiler::GetEnvDescript().m_Profile)
                        m_pStore->ResetPerformedDS(pPackDescriptorChild);
                    else
                    {
                        // Error - let speak about it 
                        m_pStore->ResetPerformedDS(pPackDescriptorChild);
                        // need to free surfaces
                        if (!skip)
                        {
                            m_pStore->UnLockSurface(m_pContext->m_frmBuff.m_iCurrIndex);
                            m_pStore->UnLockSurface(m_pContext->m_frmBuff.m_iToSkipCoping);
                        }
                        // for smart decoding we should decode and reconstruct frame according to standard pipeline
                    }

                    return  UMC_ERR_NOT_ENOUGH_DATA;
                }
                else
                    return UMC_ERR_FAILED;
            }

            // if no RM - lets execute
            if ((!pPackDescriptorChild->m_pContext->m_seqLayerHeader.RANGE_MAPY_FLAG) &&
                (!pPackDescriptorChild->m_pContext->m_seqLayerHeader.RANGE_MAPUV_FLAG) &&
                (!pPackDescriptorChild->m_pContext->m_seqLayerHeader.RANGERED))

            {
                umcRes = FillAndExecute(in);
            }

            // Update round ctrl field of seq header.
            m_pContext->m_seqLayerHeader.RNDCTRL = pPackDescriptorChild->m_pContext->m_seqLayerHeader.RNDCTRL;

            m_bLastFrameNeedDisplay = true;

            switch (pPackDescriptorChild->m_pContext->m_picLayerHeader->PTYPE & VC1_BI_FRAME)
            {
                case  VC1_I_FRAME:
                    out_data->SetFrameType(I_PICTURE);
                    break;
                case  VC1_P_FRAME:
                    out_data->SetFrameType(P_PICTURE);
                    break;
                case  VC1_B_FRAME:
                case  VC1_BI_FRAME:
                    out_data->SetFrameType(B_PICTURE);
                    break;
                default:// unexpected type
                    assert(0);
            }

            if (UMC_OK == umcRes)
            {
                if (1 == m_lFrameCount)
                    umcRes = UMC_ERR_NOT_ENOUGH_DATA;
            }
            return umcRes;
        }

        virtual UMC::FrameMemID     ProcessQueuesForNextFrame(bool& isSkip, mfxU16& Corrupted);
        virtual UMC::Status  SetRMSurface();
        virtual UMC::FrameMemID GetSkippedIndex(bool isIn = true);
    };

}

#endif //__UMC_VC1_VIDEO_DECODER_HW_H_
#endif //MFX_ENABLE_VC1_VIDEO_DECODE

