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

#include "umc_h265_mfx_supplier.h"

#include "umc_h265_frame_list.h"
#include "umc_h265_nal_spl.h"
#include "umc_h265_bitstream_headers.h"

#include "umc_h265_dec_defs.h"

#include "vm_sys_info.h"

#include "umc_h265_debug.h"

#include "umc_h265_mfx_utils.h"

namespace UMC_HEVC_DECODER
{

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////
RawHeader_H265::RawHeader_H265()
{
    Reset();
}

void RawHeader_H265::Reset()
{
    m_id = -1;
    m_buffer.clear();
}

int32_t RawHeader_H265::GetID() const
{
    return m_id;
}

size_t RawHeader_H265::GetSize() const
{
    return m_buffer.size();
}

uint8_t * RawHeader_H265::GetPointer()
{
    return
        m_buffer.empty() ? nullptr : &m_buffer[0];
}

void RawHeader_H265::Resize(int32_t id, size_t newSize)
{
    m_id = id;
    m_buffer.resize(newSize);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RawHeaders_H265::Reset()
{
    m_vps.Reset();
    m_sps.Reset();
    m_pps.Reset();
}

RawHeader_H265 * RawHeaders_H265::GetVPS()
{
    return &m_vps;
}

RawHeader_H265 * RawHeaders_H265::GetSPS()
{
    return &m_sps;
}

RawHeader_H265 * RawHeaders_H265::GetPPS()
{
    return &m_pps;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////
MFXTaskSupplier_H265::MFXTaskSupplier_H265()
    : TaskSupplier_H265()
{
    memset(&m_firstVideoParams, 0, sizeof(mfxVideoParam));
}

MFXTaskSupplier_H265::~MFXTaskSupplier_H265()
{
    Close();
}

// Initialize task supplier
UMC::Status MFXTaskSupplier_H265::Init(UMC::VideoDecoderParams *init)
{
    UMC::Status umcRes;

    if (NULL == init)
        return UMC::UMC_ERR_NULL_PTR;

    Close();

    m_initializationParams = *init;
    m_pMemoryAllocator = init->lpMemoryAllocator;
    m_DPBSizeEx = 0;

    m_sei_messages = new SEI_Storer_H265();
    m_sei_messages->Init();

    int32_t nAllowedThreadNumber = init->numThreads;
    if(nAllowedThreadNumber < 0) nAllowedThreadNumber = 0;

    // calculate number of slice decoders.
    // It should be equal to CPU number
    m_iThreadNum = (0 == nAllowedThreadNumber) ? (vm_sys_info_get_cpu_num()) : (nAllowedThreadNumber);

    umcRes = MVC_Extension::Init();
    if (UMC::UMC_OK != umcRes)
    {
        return umcRes;
    }

    AU_Splitter_H265::Init(init);

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
    m_frameOrder             = 0;
    m_use_external_framerate = 0 < init->info.framerate;

    if (m_use_external_framerate)
    {
        m_local_delta_frame_time = 1 / init->info.framerate;
    }

    GetView()->dpbSize = 16;
    m_DPBSizeEx = m_iThreadNum + init->info.bitrate;

    return UMC::UMC_OK;
}

// Check whether specified frame has been decoded, and if it was,
// whether it is necessary to display some frame
bool MFXTaskSupplier_H265::CheckDecoding(bool should_additional_check, H265DecoderFrame * outputFrame)
{
    ViewItem_H265 &view = *GetView();

    if (!outputFrame->IsDecodingStarted())
        return false;

    if (outputFrame->IsDecodingCompleted())
    {
        if (!should_additional_check)
            return true;

        int32_t maxReadyUID = outputFrame->m_UID;
        uint32_t inDisplayStage = 0;

        UMC::AutomaticUMCMutex guard(m_mGuard);

        for (H265DecoderFrame * pTmp = view.pDPB->head(); pTmp; pTmp = pTmp->future())
        {
            if (pTmp->m_wasOutputted != 0 && pTmp->m_wasDisplayed == 0 && pTmp->m_maxUIDWhenWasDisplayed)
            {
                inDisplayStage++; // number of outputted frames at this moment
            }

            if ((pTmp->IsDecoded() || pTmp->IsDecodingCompleted()) && maxReadyUID < pTmp->m_UID)
                maxReadyUID = pTmp->m_UID;
        }

        DEBUG_PRINT1((VM_STRING("output frame - %d, notDecoded - %u, count - %u\n"), outputFrame->m_PicOrderCnt, notDecoded, count));
        if (inDisplayStage > 1 || m_maxUIDWhenWasDisplayed <= maxReadyUID)
        {
            return true;
        }
    }

    return false;
}

// Perform decoding task for thread number threadNumber
mfxStatus MFXTaskSupplier_H265::RunThread(mfxU32 threadNumber)
{
    UMC::Status sts = m_pSegmentDecoder[threadNumber]->ProcessSegment();

    if (sts == UMC::UMC_ERR_NOT_ENOUGH_DATA)
        return MFX_TASK_BUSY;
    else if(sts == UMC::UMC_ERR_DEVICE_FAILED)
        return MFX_ERR_DEVICE_FAILED;
    else if (sts == UMC::UMC_ERR_GPU_HANG)
        return MFX_ERR_GPU_HANG;

    if (sts != UMC::UMC_OK)
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    return MFX_TASK_WORKING;
}

// Check whether all slices for the frame were found
void MFXTaskSupplier_H265::CompleteFrame(H265DecoderFrame * pFrame)
{
    if (!pFrame)
        return;

    H265DecoderFrameInfo * slicesInfo = pFrame->GetAU();
    if (slicesInfo->GetStatus() > H265DecoderFrameInfo::STATUS_NOT_FILLED)
        return;

    TaskSupplier_H265::CompleteFrame(pFrame);
}

// Set initial video params from application
void MFXTaskSupplier_H265::SetVideoParams(mfxVideoParam * par)
{
    m_firstVideoParams = *par;
    m_decodedOrder     = m_firstVideoParams.mfx.DecodedOrder != 0;
}

UMC::Status MFXTaskSupplier_H265::FillVideoParam(mfxVideoParam *par, bool full)
{
    const H265VideoParamSet * vps = GetHeaders()->m_VideoParams.GetCurrentHeader();
    const H265SeqParamSet * seq = GetHeaders()->m_SeqParams.GetCurrentHeader();

    if (!seq)
        return UMC::UMC_ERR_FAILED;

    if (MFX_Utility::FillVideoParam(vps, seq, par, full) != UMC::UMC_OK)
        return UMC::UMC_ERR_FAILED;

    return UMC::UMC_OK;
}

// Decode headers nal unit
UMC::Status MFXTaskSupplier_H265::DecodeHeaders(UMC::MediaDataEx *nalUnit)
{
    UMC::Status sts = TaskSupplier_H265::DecodeHeaders(nalUnit);
    if (sts != UMC::UMC_OK)
        return sts;

    {
        static const uint8_t start_code_prefix[] = {0, 0, 0, 1};
        // save vps/sps/pps
        uint32_t nal_unit_type = nalUnit->GetExData()->values[0];
        switch(nal_unit_type)
        {
            case NAL_UT_VPS:
                {
                    size_t const prefix_size = sizeof(start_code_prefix);

                    size_t size = nalUnit->GetDataSize();
                    RawHeader_H265 * hdr = GetVPS();
                    int32_t id = m_Headers.m_VideoParams.GetCurrentID();
                    hdr->Resize(id, size + prefix_size);
                    std::copy(start_code_prefix, start_code_prefix + prefix_size, hdr->GetPointer());
                    std::copy((uint8_t*)nalUnit->GetDataPointer(), (uint8_t*)nalUnit->GetDataPointer() + size, hdr->GetPointer() + prefix_size);
                }
            break;
            case NAL_UT_SPS:
            case NAL_UT_PPS:
                {
                    size_t const prefix_size = sizeof(start_code_prefix);

                    size_t size = nalUnit->GetDataSize();
                    bool isSPS = (nal_unit_type == NAL_UT_SPS);
                    RawHeader_H265 * hdr = isSPS ? GetSPS() : GetPPS();
                    H265SeqParamSet * currSPS = isSPS ? m_Headers.m_SeqParams.GetCurrentHeader() : nullptr;
                    H265PicParamSet * currPPS = isSPS ? nullptr : m_Headers.m_PicParams.GetCurrentHeader();
                    int32_t id = isSPS ? m_Headers.m_SeqParams.GetCurrentID() : m_Headers.m_PicParams.GetCurrentID();
                    if (hdr->GetPointer() != nullptr && hdr->GetID() == id)
                    {
                        bool changed =
                            size + prefix_size != hdr->GetSize() ||
                            !!memcmp(hdr->GetPointer() + prefix_size, nalUnit->GetDataPointer(), size);

                        if (isSPS && currSPS != nullptr)
                            currSPS->m_changed = changed;
                        else if (currPPS != nullptr)
                            currPPS->m_changed = changed;
                    }
                    hdr->Resize(id, size + prefix_size);
                    std::copy(start_code_prefix, start_code_prefix + prefix_size, hdr->GetPointer());
                    std::copy((uint8_t*)nalUnit->GetDataPointer(), (uint8_t*)nalUnit->GetDataPointer() + size, hdr->GetPointer() + prefix_size);
                }
            break;
        }
    }

    UMC::MediaDataEx::_MediaDataEx* pMediaDataEx = nalUnit->GetExData();
    if ((NalUnitType)pMediaDataEx->values[0] == NAL_UT_SPS && m_firstVideoParams.mfx.FrameInfo.Width)
    {
        H265SeqParamSet * currSPS = m_Headers.m_SeqParams.GetCurrentHeader();

        if (currSPS)
        {
            if (m_firstVideoParams.mfx.FrameInfo.Width < (currSPS->pic_width_in_luma_samples) ||
                m_firstVideoParams.mfx.FrameInfo.Height < (currSPS->pic_height_in_luma_samples) ||
                (currSPS->m_pcPTL.GetGeneralPTL()->level_idc && m_firstVideoParams.mfx.CodecLevel && m_firstVideoParams.mfx.CodecLevel < currSPS->m_pcPTL.GetGeneralPTL()->level_idc))
            {
                return UMC::UMC_NTF_NEW_RESOLUTION;
            }
        }

        return UMC::UMC_WRN_REPOSITION_INPROGRESS;
    }

    return UMC::UMC_OK;
}

// Decode SEI nal unit
UMC::Status MFXTaskSupplier_H265::DecodeSEI(UMC::MediaDataEx *nalUnit)
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
        nalUnit->MoveDataPointer(2); // skip[ [NAL unit header = 16 bits]

        do
        {
            H265SEIPayLoad    m_SEIPayLoads;

            size_t decoded1 = bitStream.BytesDecoded();

            bitStream.ParseSEI(m_Headers.m_SeqParams, m_Headers.m_SeqParams.GetCurrentID(), &m_SEIPayLoads);

            if (m_SEIPayLoads.payLoadType == SEI_USER_DATA_REGISTERED_TYPE)
            {
                m_UserData = m_SEIPayLoads;
            }
            else
            {
                if (m_SEIPayLoads.payLoadType == SEI_RESERVED)
                    continue;

                m_Headers.m_SEIParams.AddHeader(&m_SEIPayLoads);
            }

            size_t decoded2 = bitStream.BytesDecoded();

            // calculate payload size
            size_t size = decoded2 - decoded1;

            VM_ASSERT(size == m_SEIPayLoads.payLoadSize + 2 + (m_SEIPayLoads.payLoadSize / 255) + (m_SEIPayLoads.payLoadType / 255));

            if (m_sei_messages)
            {
                UMC::MediaDataEx nalUnit1;

                size_t nal_u_size = size;
                for(uint8_t *ptr = (uint8_t*)nalUnit->GetDataPointer(); ptr < (uint8_t*)nalUnit->GetDataPointer() + nal_u_size; ptr++)
                    if (ptr[0]==0 && ptr[1]==0 && ptr[2]==3)
                        nal_u_size += 1;

                nalUnit1.SetBufferPointer((uint8_t*)nalUnit->GetDataPointer(), nal_u_size);
                nalUnit1.SetDataSize(nal_u_size);
                nalUnit1.SetExData(nalUnit->GetExData());

                double start, stop;
                nalUnit->GetTime(start, stop);
                nalUnit1.SetTime(start, stop);

                nalUnit->MoveDataPointer((int32_t)nal_u_size);

                SEI_Storer_H265::SEI_Message* msg =
                    m_sei_messages->AddMessage(&nalUnit1, m_SEIPayLoads.payLoadType);
                //frame is bound to SEI prefix payloads w/ the first slice
                //here we bind SEI suffix payloads
                if (msg && msg->nal_type == NAL_UT_SEI_SUFFIX)
                    msg->frame = GetView()->pCurFrame;
            }

        } while (bitStream.More_RBSP_Data());

    } catch(...)
    {
        // nothing to do just catch it
    }

    return UMC::UMC_OK;
}

// Do something in case reference frame is missing
void MFXTaskSupplier_H265::AddFakeReferenceFrame(H265Slice * )
{
}

} // namespace UMC_HEVC_DECODER

#endif // MFX_ENABLE_H265_VIDEO_DECODE
