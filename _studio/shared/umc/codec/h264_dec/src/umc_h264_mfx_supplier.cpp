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

#if defined (MFX_ENABLE_H264_VIDEO_DECODE)

#include <algorithm>
#include "mfxfei.h"

#include "umc_h264_mfx_supplier.h"

#include "umc_h264_frame_list.h"
#include "umc_h264_nal_spl.h"
#include "umc_h264_bitstream_headers.h"

#include "umc_h264_dec_defs_dec.h"
#include "umc_h264_dec_mfx.h"

#include "vm_sys_info.h"

#include "umc_h264_dec_debug.h"
#include "mfxpcp.h"

#include "mfx_enc_common.h"

namespace UMC
{

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////
RawHeader::RawHeader()
{
    Reset();
}

void RawHeader::Reset()
{
    m_id = -1;
    m_buffer.clear();
}

int32_t RawHeader::GetID() const
{
    return m_id;
}

size_t RawHeader::GetSize() const
{
    return m_buffer.size();
}

uint8_t * RawHeader::GetPointer()
{
    return &m_buffer[0];
}

void RawHeader::Resize(int32_t id, size_t newSize)
{
    m_id = id;
    m_buffer.resize(newSize);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RawHeaders::Reset()
{
    m_sps.Reset();
    m_pps.Reset();
}

RawHeader * RawHeaders::GetSPS()
{
    return &m_sps;
}

RawHeader * RawHeaders::GetPPS()
{
    return &m_pps;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////
MFXTaskSupplier::MFXTaskSupplier()
    : TaskSupplier()
{
    memset(&m_firstVideoParams, 0, sizeof(mfxVideoParam));
}

MFXTaskSupplier::~MFXTaskSupplier()
{
    Close();
}

Status MFXTaskSupplier::Init(VideoDecoderParams *init)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "MFXTaskSupplier::Init");
    if (NULL == init)
        return UMC_ERR_NULL_PTR;

    Close();

    m_initializationParams = *init;
    m_pMemoryAllocator = init->lpMemoryAllocator;
    m_DPBSizeEx = 0;

    m_sei_messages = new SEI_Storer();
    m_sei_messages->Init();

    int32_t nAllowedThreadNumber = init->numThreads;
    if(nAllowedThreadNumber < 0) nAllowedThreadNumber = 0;

    // calculate number of slice decoders.
    // It should be equal to CPU number
    m_iThreadNum = (0 == nAllowedThreadNumber) ? (vm_sys_info_get_cpu_num()) : (nAllowedThreadNumber);

    Status umcRes = SVC_Extension::Init();
    if (UMC_OK != umcRes)
    {
        return umcRes;
    }

    switch (m_initializationParams.info.profile) // after MVC_Extension::Init()
    {
    case 0:
        m_decodingMode = UNKNOWN_DECODING_MODE;
        break;
    case H264VideoDecoderParams::H264_PROFILE_MULTIVIEW_HIGH:
    case H264VideoDecoderParams::H264_PROFILE_STEREO_HIGH:
        m_decodingMode = MVC_DECODING_MODE;
        break;
    default:
        m_decodingMode = AVC_DECODING_MODE;
        break;
    }

    AU_Splitter::Init();
    DPBOutput::Reset(m_iThreadNum != 1);

    // create slice decoder(s)
    m_pSegmentDecoder = new H264SegmentDecoderBase *[m_iThreadNum];
    if (NULL == m_pSegmentDecoder)
        return UMC_ERR_ALLOC;
    memset(m_pSegmentDecoder, 0, sizeof(H264SegmentDecoderBase *) * m_iThreadNum);

    CreateTaskBroker();
    m_pTaskBroker->Init(m_iThreadNum);

    for (uint32_t i = 0; i < m_iThreadNum; i += 1)
    {
        if (UMC_OK != m_pSegmentDecoder[i]->Init(i))
            return UMC_ERR_INIT;
    }

    m_local_delta_frame_time = 1.0/30;
    m_frameOrder             = 0;
    m_use_external_framerate = 0 < init->info.framerate;

    if (m_use_external_framerate)
    {
        m_local_delta_frame_time = 1 / init->info.framerate;
    }

    H264VideoDecoderParams *initH264 = DynamicCast<H264VideoDecoderParams> (init);
    m_DPBSizeEx = m_iThreadNum + (initH264 ? initH264->m_bufferedFrames : 0);
    return UMC_OK;
}

void MFXTaskSupplier::Reset()
{
    TaskSupplier::Reset();

    m_Headers.Reset();

    if (m_pTaskBroker)
        m_pTaskBroker->Init(m_iThreadNum);
}

bool MFXTaskSupplier::CheckDecoding(H264DecoderFrame * outputFrame)
{
    ViewItem &view = GetView(outputFrame->m_viewId);

    if (!outputFrame->IsDecodingStarted())
        return false;

    if (!outputFrame->IsDecodingCompleted())
        return false;

    AutomaticUMCMutex guard(m_mGuard);

    int32_t count = 0;
    int32_t notDecoded = 0;
    for (H264DecoderFrame * pTmp = view.GetDPBList(0)->head(); pTmp; pTmp = pTmp->future())
    {
        if (!pTmp->m_isShortTermRef[0] &&
            !pTmp->m_isShortTermRef[1] &&
            !pTmp->m_isLongTermRef[0] &&
            !pTmp->m_isLongTermRef[1] &&
            ((pTmp->m_wasOutputted != 0) || (pTmp->IsDecoded() == 0)))
        {
            count++;
            break;
        }

        if (!pTmp->IsDecoded() && !pTmp->IsDecodingCompleted() && pTmp->IsFullFrame())
            notDecoded++;
    }

    if (count || (view.GetDPBList(0)->countAllFrames() < view.maxDecFrameBuffering + m_DPBSizeEx))
        return true;

    if (!notDecoded)
        return true;

    return false;
}

mfxStatus MFXTaskSupplier::RunThread(mfxU32 threadNumber)
{
    Status sts = m_pSegmentDecoder[threadNumber]->ProcessSegment();
    if (sts == UMC_ERR_NOT_ENOUGH_DATA)
        return MFX_TASK_BUSY;
    else if(sts == UMC_ERR_DEVICE_FAILED)
        return MFX_ERR_DEVICE_FAILED;
    else if (sts == UMC_ERR_GPU_HANG)
        return MFX_ERR_GPU_HANG;

    if (sts != UMC_OK)
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    return MFX_TASK_WORKING;
}

bool MFXTaskSupplier::ProcessNonPairedField(H264DecoderFrame * pFrame)
{
    if (pFrame && pFrame->GetAU(1)->GetStatus() == H264DecoderFrameInfo::STATUS_NOT_FILLED)
    {
        pFrame->setPicOrderCnt(pFrame->PicOrderCnt(0, 1), 1);
        pFrame->GetAU(1)->SetStatus(H264DecoderFrameInfo::STATUS_NONE);
        pFrame->m_bottom_field_flag[1] = !pFrame->m_bottom_field_flag[0];

        H264Slice * pSlice = pFrame->GetAU(0)->GetSlice(0);
        VM_ASSERT(pSlice);
        if (!pSlice)
            return false;
        pFrame->setPicNum(pSlice->GetSliceHeader()->frame_num*2 + 1, 1);

        int32_t isBottom = pSlice->IsBottomField() ? 0 : 1;
        pFrame->SetErrorFlagged(isBottom ? ERROR_FRAME_BOTTOM_FIELD_ABSENT : ERROR_FRAME_TOP_FIELD_ABSENT);
        return true;
    }

    return false;
}

Status MFXTaskSupplier::CompleteFrame(H264DecoderFrame * pFrame, int32_t field)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "MFXTaskSupplier::CompleteFrame");
    if (!pFrame)
        return UMC_OK;

    H264DecoderFrameInfo * slicesInfo = pFrame->GetAU(field);
    if (slicesInfo->GetStatus() > H264DecoderFrameInfo::STATUS_NOT_FILLED)
        return UMC_OK;

    return TaskSupplier::CompleteFrame(pFrame, field);
}

void MFXTaskSupplier::SetVideoParams(mfxVideoParam * par)
{
    m_firstVideoParams = *par;
}

Status MFXTaskSupplier::DecodeHeaders(NalUnit *nalUnit)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "MFXTaskSupplier::DecodeHeaders");
    Status sts = TaskSupplier::DecodeHeaders(nalUnit);
    if (sts != UMC_OK)
        return sts;

    UMC_H264_DECODER::H264SeqParamSet * currSPS = m_Headers.m_SeqParams.GetCurrentHeader();

    if (currSPS)
    {
        if (currSPS->chroma_format_idc > 2)
            throw h264_exception(UMC_ERR_UNSUPPORTED);

        switch (currSPS->profile_idc)
        {
        case H264VideoDecoderParams::H264_PROFILE_UNKNOWN:
        case H264VideoDecoderParams::H264_PROFILE_BASELINE:
        case H264VideoDecoderParams::H264_PROFILE_MAIN:
        case H264VideoDecoderParams::H264_PROFILE_EXTENDED:
        case H264VideoDecoderParams::H264_PROFILE_HIGH:
        case H264VideoDecoderParams::H264_PROFILE_HIGH10:
        case H264VideoDecoderParams::H264_PROFILE_HIGH422:

        case H264VideoDecoderParams::H264_PROFILE_MULTIVIEW_HIGH:
        case H264VideoDecoderParams::H264_PROFILE_STEREO_HIGH:

        case H264VideoDecoderParams::H264_PROFILE_SCALABLE_BASELINE:
        case H264VideoDecoderParams::H264_PROFILE_SCALABLE_HIGH:
            break;
        default:
            throw h264_exception(UMC_ERR_UNSUPPORTED);
        }

        DPBOutput::OnNewSps(currSPS);
    }

    {
        // save sps/pps
        uint32_t nal_unit_type = nalUnit->GetNalUnitType();
        switch(nal_unit_type)
        {
            case NAL_UT_SPS:
            case NAL_UT_PPS:
                {
                    static uint8_t start_code_prefix[] = {0, 0, 0, 1};

                    size_t size = nalUnit->GetDataSize();
                    bool isSPS = (nal_unit_type == NAL_UT_SPS);
                    RawHeader * hdr = isSPS ? GetSPS() : GetPPS();
                    int32_t id = isSPS ? m_Headers.m_SeqParams.GetCurrentID() : m_Headers.m_PicParams.GetCurrentID();
                    hdr->Resize(id, size + sizeof(start_code_prefix));
                    MFX_INTERNAL_CPY(hdr->GetPointer(), start_code_prefix,  sizeof(start_code_prefix));
                    MFX_INTERNAL_CPY(hdr->GetPointer() + sizeof(start_code_prefix), (uint8_t*)nalUnit->GetDataPointer(), (uint32_t)size);
                 }
            break;
        }
    }

    if (nalUnit->GetNalUnitType() == NAL_UT_SPS && m_firstVideoParams.mfx.FrameInfo.Width)
    {
        currSPS = m_Headers.m_SeqParams.GetCurrentHeader();

        if (currSPS)
        {
            if (m_firstVideoParams.mfx.FrameInfo.Width < (currSPS->frame_width_in_mbs * 16) ||
                m_firstVideoParams.mfx.FrameInfo.Height < (currSPS->frame_height_in_mbs * 16) ||
                (currSPS->level_idc && m_firstVideoParams.mfx.CodecLevel && m_firstVideoParams.mfx.CodecLevel < currSPS->level_idc))
            {
                return UMC_NTF_NEW_RESOLUTION;
            }
        }

        return UMC_WRN_REPOSITION_INPROGRESS;
    }

    return UMC_OK;
}

Status MFXTaskSupplier::DecodeSEI(NalUnit *nalUnit)
{
    H264HeadersBitstream bitStream;

    try
    {
        H264MemoryPiece mem;
        mem.SetData(nalUnit);

        H264MemoryPiece swappedMem;
        swappedMem.Allocate(nalUnit->GetDataSize() + DEFAULT_NU_TAIL_SIZE);

        SwapperBase * swapper = m_pNALSplitter->GetSwapper();
        swapper->SwapMemory(&swappedMem, &mem, DEFAULT_NU_HEADER_TAIL_VALUE);

        bitStream.Reset((uint8_t*)swappedMem.GetPointer(), (uint32_t)swappedMem.GetDataSize());
        bitStream.SetTailBsSize(DEFAULT_NU_TAIL_SIZE);

        NAL_Unit_Type nal_unit_type = NAL_UT_UNSPECIFIED;
        uint32_t nal_ref_idc = 0;

        bitStream.GetNALUnitType(nal_unit_type, nal_ref_idc);
        nalUnit->MoveDataPointer(1); // nal_unit_type - 8 bits

        do
        {
            UMC_H264_DECODER::H264SEIPayLoad    m_SEIPayLoads;

            size_t decoded1 = bitStream.BytesDecoded();

            bitStream.ParseSEI(m_Headers, &m_SEIPayLoads);

            if (m_SEIPayLoads.payLoadType == SEI_PIC_TIMING_TYPE)
            {
                DEBUG_PRINT((VM_STRING("debug headers SEI - %d, picstruct - %d\n"), m_SEIPayLoads.payLoadType, m_SEIPayLoads.SEI_messages.pic_timing.pic_struct));
            }
            else
            {
                DEBUG_PRINT((VM_STRING("debug headers SEI - %d\n"), m_SEIPayLoads.payLoadType));
            }

            if (m_SEIPayLoads.payLoadType == SEI_USER_DATA_REGISTERED_TYPE)
            {
                m_UserData = m_SEIPayLoads;
            }
            else
            {
                if (m_SEIPayLoads.payLoadType == SEI_RESERVED)
                    continue;

                UMC_H264_DECODER::H264SEIPayLoad* payload = m_Headers.m_SEIParams.AddHeader(&m_SEIPayLoads);
                m_accessUnit.m_payloads.AddPayload(payload);
            }

            size_t decoded2 = bitStream.BytesDecoded();

            // calculate payload size
            size_t size = decoded2 - decoded1;

            VM_ASSERT(size == m_SEIPayLoads.payLoadSize + 2 + (m_SEIPayLoads.payLoadSize / 255) + (m_SEIPayLoads.payLoadType / 255));

            if (m_sei_messages)
            {
                MediaDataEx nalUnit1;

                size_t nal_u_size = size;
                for(uint8_t *ptr = (uint8_t*)nalUnit->GetDataPointer(); ptr < (uint8_t*)nalUnit->GetDataPointer() + nal_u_size; ptr++)
                    if (ptr[0]==0 && ptr[1]==0 && ptr[2]==3)
                        nal_u_size += 1;

                nalUnit1.SetBufferPointer((uint8_t*)nalUnit->GetDataPointer(), nal_u_size);
                nalUnit1.SetDataSize(nal_u_size);
                nalUnit->MoveDataPointer((int32_t)nal_u_size);
                m_sei_messages->AddMessage(&nalUnit1, m_SEIPayLoads.payLoadType, -1);
            }

        } while (bitStream.More_RBSP_Data());

    } catch(...)
    {
        // nothing to do just catch it
    }

    return UMC_OK;
}

void MFXTaskSupplier::AddFakeReferenceFrame(H264Slice * )
{
}

} // namespace UMC


bool MFX_Utility::IsNeedPartialAcceleration(mfxVideoParam * par, eMFXHWType )
{
    if (!par)
        return false;

    if (par->mfx.SliceGroupsPresent) // Is FMO
        return true;

    if (par->mfx.FrameInfo.FourCC != MFX_FOURCC_NV12) // yuv422 in SW only
        return true;

    if (par->mfx.FrameInfo.BitDepthLuma > 8 || par->mfx.FrameInfo.BitDepthChroma > 8) // yuv422 in SW only
        return true;

    mfxExtMVCSeqDesc * points = (mfxExtMVCSeqDesc*)GetExtendedBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_MVC_SEQ_DESC);
    if (points && points->NumRefsTotal > 16)
        return true;

    return false;
}

eMFXPlatform MFX_Utility::GetPlatform(VideoCORE * core, mfxVideoParam * par)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "MFX_Utility::GetPlatform");
    eMFXPlatform platform = core->GetPlatformType();

    if (!par)
        return platform;

    eMFXHWType typeHW = MFX_HW_UNKNOWN;
    typeHW = core->GetHWType();

    if (par && IsNeedPartialAcceleration(par, typeHW) && platform != MFX_PLATFORM_SOFTWARE)
    {
        return MFX_PLATFORM_SOFTWARE;
    }

    GUID name;

    switch (typeHW)
    {
    case MFX_HW_HSW_ULT:
    case MFX_HW_HSW:
    case MFX_HW_BDW:
    case MFX_HW_SCL:
    case MFX_HW_CHT:
    case MFX_HW_KBL:
    case MFX_HW_APL:
    case MFX_HW_GLK:
    case MFX_HW_CFL:
    case MFX_HW_CNL:
    case MFX_HW_ICL:
    case MFX_HW_ICL_LP:
#if (MFX_VERSION >= 1031)	
    case MFX_HW_JSL:
    case MFX_HW_EHL:
#endif
        name = sDXVA2_ModeH264_VLD_NoFGT;
        break;
    default:
        name = sDXVA2_ModeH264_VLD_NoFGT;
        break;
    }

    if (IS_PROTECTION_CENC(par->Protected))
        name = DXVA_Intel_Decode_Elementary_Stream_AVC;

    if (MFX_ERR_NONE != core->IsGuidSupported(name, par) &&
        platform != MFX_PLATFORM_SOFTWARE)
    {
        return MFX_PLATFORM_SOFTWARE;
    }

    return platform;
}

UMC::Status MFX_Utility::FillVideoParam(UMC::TaskSupplier * supplier, mfxVideoParam *par, bool full)
{
    const UMC_H264_DECODER::H264SeqParamSet * seq = supplier->GetHeaders()->m_SeqParams.GetCurrentHeader();
    if (!seq)
        return UMC::UMC_ERR_FAILED;

    if (UMC::FillVideoParam(seq, par, full) != UMC::UMC_OK)
        return UMC::UMC_ERR_FAILED;

    const UMC_H264_DECODER::H264PicParamSet * pps = supplier->GetHeaders()->m_PicParams.GetCurrentHeader();
    if (pps)
        par->mfx.SliceGroupsPresent = pps->num_slice_groups > 1;

    return UMC::UMC_OK;
}

static inline bool IsFieldOfOneFrame(UMC::H264Slice * pSlice1, UMC::H264Slice *pSlice2)
{
    if (!pSlice2 || !pSlice1)
        return true;

    if ((pSlice1->GetSliceHeader()->nal_ref_idc && !pSlice2->GetSliceHeader()->nal_ref_idc) ||
        (!pSlice1->GetSliceHeader()->nal_ref_idc && pSlice2->GetSliceHeader()->nal_ref_idc))
        return false;

    if (pSlice1->GetSliceHeader()->field_pic_flag != pSlice2->GetSliceHeader()->field_pic_flag)
        return false;

    if (pSlice1->GetSliceHeader()->bottom_field_flag == pSlice2->GetSliceHeader()->bottom_field_flag)
        return false;

    return true;
}

class PosibleMVC
{
public:
    PosibleMVC(UMC::TaskSupplier * supplier);
    virtual ~PosibleMVC();

    virtual UMC::Status DecodeHeader(UMC::MediaData* params, mfxBitstream *bs, mfxVideoParam *out);
    virtual UMC::Status ProcessNalUnit(UMC::MediaData * data, mfxBitstream *bs);
    virtual bool IsEnough() const;


protected:
    bool m_isSPSFound;
    bool m_isPPSFound;
    bool m_isSubSPSFound;

    bool m_isFrameLooked;

    bool m_isSVC_SEIFound;

    bool m_isMVCBuffer;
    bool m_isSVCBuffer;

    bool IsAVCEnough() const;
    bool isMVCRequered() const;

    UMC::TaskSupplier * m_supplier;
    UMC::H264Slice * m_lastSlice;
};

PosibleMVC::PosibleMVC(UMC::TaskSupplier * supplier)
    : m_isSPSFound(false)
    , m_isPPSFound(false)
    , m_isSubSPSFound(false)
    , m_isFrameLooked(false)
    , m_isSVC_SEIFound(false)
    , m_isMVCBuffer(false)
    , m_isSVCBuffer(false)
    , m_supplier(supplier)
    , m_lastSlice(0)
{
}

PosibleMVC::~PosibleMVC()
{
    if (m_lastSlice)
        m_lastSlice->DecrementReference();
}


bool PosibleMVC::IsEnough() const
{
    return m_isSPSFound && m_isPPSFound && m_isSubSPSFound && m_isFrameLooked;
}

bool PosibleMVC::IsAVCEnough() const
{
    return m_isSPSFound && m_isPPSFound;
}

bool PosibleMVC::isMVCRequered() const
{
    return m_isMVCBuffer || m_isSVCBuffer;
}

UMC::Status PosibleMVC::DecodeHeader(UMC::MediaData * data, mfxBitstream *bs, mfxVideoParam *out)
{
    if (!data)
        return UMC::UMC_ERR_NULL_PTR;

    m_isMVCBuffer = GetExtendedBuffer(out->ExtParam, out->NumExtParam, MFX_EXTBUFF_MVC_SEQ_DESC) != 0;


    if (isMVCRequered())
    {
        m_isFrameLooked = !m_isSVCBuffer;
    }
    else
    {
        m_isSubSPSFound = true; // do not need to find it
    }

    m_lastSlice = 0;

    struct sps_heap_obj
    {
        UMC_H264_DECODER::H264SeqParamSet* obj;
        sps_heap_obj() : obj(0) {}
        ~sps_heap_obj() { if (obj) obj->DecrementReference(); }
        void set(UMC_H264_DECODER::H264SeqParamSet* sps) { if(sps) { obj = sps; obj->IncrementReference();} }
    } first_sps;

    UMC::Status umcRes = UMC::UMC_ERR_NOT_ENOUGH_DATA;
    for ( ; data->GetDataSize() > 3; )
    {
        m_supplier->GetNalUnitSplitter()->MoveToStartCode(data); // move data pointer to start code
        if (!m_isSPSFound && !m_isSVC_SEIFound) // move point to first start code
        {
            bs->DataOffset = (mfxU32)((mfxU8*)data->GetDataPointer() - (mfxU8*)data->GetBufferPointer());
            bs->DataLength = (mfxU32)data->GetDataSize();
        }

        umcRes = ProcessNalUnit(data, bs);
        if (umcRes == UMC::UMC_ERR_UNSUPPORTED)
            umcRes = UMC::UMC_OK;

        if (umcRes != UMC::UMC_OK)
            break;

        if (!first_sps.obj && m_isSPSFound)
            first_sps.set(m_supplier->GetHeaders()->m_SeqParams.GetCurrentHeader());

        if (IsEnough())
            break;
    }

    if (umcRes == UMC::UMC_ERR_SYNC) // move pointer
    {
        bs->DataOffset = (mfxU32)((mfxU8*)data->GetDataPointer() - (mfxU8*)data->GetBufferPointer());
        bs->DataLength = (mfxU32)data->GetDataSize();
        return UMC::UMC_ERR_NOT_ENOUGH_DATA;
    }

    if (!m_isFrameLooked && ((data->GetFlags() & UMC::MediaData::FLAG_VIDEO_DATA_NOT_FULL_FRAME) == 0))
        m_isFrameLooked = true;

    if (umcRes == UMC::UMC_ERR_NOT_ENOUGH_DATA)
    {
        bool isEOS = ((data->GetFlags() & UMC::MediaData::FLAG_VIDEO_DATA_END_OF_STREAM) != 0) ||
            ((data->GetFlags() & UMC::MediaData::FLAG_VIDEO_DATA_NOT_FULL_FRAME) == 0);
        if (isEOS)
        {
            return UMC::UMC_OK;
        }
    }

    if (!isMVCRequered())
    {
        m_isSubSPSFound = true;
        m_isFrameLooked = true;
    }

    if (IsEnough())
    {
        VM_ASSERT(first_sps.obj && "Current SPS should be valid when [m_isSPSFound]");

        UMC_H264_DECODER::H264SeqParamSet* last_sps = m_supplier->GetHeaders()->m_SeqParams.GetCurrentHeader();
        if (first_sps.obj && first_sps.obj != last_sps)
            m_supplier->GetHeaders()->m_SeqParams.AddHeader(first_sps.obj);

        return UMC::UMC_OK;
    }

    return UMC::UMC_ERR_NOT_ENOUGH_DATA;
}

UMC::Status PosibleMVC::ProcessNalUnit(UMC::MediaData * data, mfxBitstream * bs)
{

#if (MFX_VERSION >= 1025)
    mfxExtDecodeErrorReport * pDecodeErrorReport = (mfxExtDecodeErrorReport*)GetExtendedBuffer(bs->ExtParam, bs->NumExtParam, MFX_EXTBUFF_DECODE_ERROR_REPORT);
#endif

    try
    {
        int32_t startCode = m_supplier->GetNalUnitSplitter()->CheckNalUnitType(data);

        bool needProcess = false;

        UMC::NalUnit *nalUnit = m_supplier->GetNalUnit(data);

        switch ((UMC::NAL_Unit_Type)startCode)
        {
        case UMC::NAL_UT_IDR_SLICE:
        case UMC::NAL_UT_SLICE:
        case UMC::NAL_UT_AUXILIARY:
        case UMC::NAL_UT_CODED_SLICE_EXTENSION:
            {
                if (IsAVCEnough())
                {
                    if (!isMVCRequered())
                    {
                        m_isFrameLooked = true;
                        return UMC::UMC_OK;
                    }
                }
                else
                    break; // skip nal unit

                if (!nalUnit)
                {
                    return UMC::UMC_ERR_NOT_ENOUGH_DATA;
                }

                int32_t sps_id = m_supplier->GetHeaders()->m_SeqParams.GetCurrentID();
                int32_t sps_mvc_id = m_supplier->GetHeaders()->m_SeqParamsMvcExt.GetCurrentID();
                int32_t sps_svc_id = m_supplier->GetHeaders()->m_SeqParamsSvcExt.GetCurrentID();
                int32_t sps_pps_id = m_supplier->GetHeaders()->m_PicParams.GetCurrentID();

                UMC::H264Slice * pSlice = m_supplier->DecodeSliceHeader(nalUnit);
                if (pSlice)
                {
                    if (!m_lastSlice)
                    {
                        m_lastSlice = pSlice;
                        m_lastSlice->Release();
                        return UMC::UMC_OK;
                    }

                    if ((false == IsPictureTheSame(m_lastSlice, pSlice)))
                    {
                        if (!IsFieldOfOneFrame(pSlice, m_lastSlice))
                        {
                            m_supplier->GetHeaders()->m_SeqParams.SetCurrentID(sps_id);
                            m_supplier->GetHeaders()->m_SeqParamsMvcExt.SetCurrentID(sps_mvc_id);
                            m_supplier->GetHeaders()->m_SeqParamsSvcExt.SetCurrentID(sps_svc_id);
                            m_supplier->GetHeaders()->m_PicParams.SetCurrentID(sps_pps_id);

                            pSlice->Release();
                            pSlice->DecrementReference();
                            m_isFrameLooked = true;
                            return UMC::UMC_OK;
                        }
                    }

                    pSlice->Release();
                    m_lastSlice->DecrementReference();
                    m_lastSlice = pSlice;
                    return UMC::UMC_OK;
                }
            }
            break;

        case UMC::NAL_UT_SPS:
            needProcess = true;
            break;

        case UMC::NAL_UT_PPS:
            needProcess = true;
            break;

        case UMC::NAL_UT_SUBSET_SPS:
            needProcess = true;
            break;

        case UMC::NAL_UT_AUD:
            //if (header_was_started)
                //quite = true;
            break;

        case UMC::NAL_UT_SEI:  // SVC scalable SEI can be before SPS/PPS
        case UMC::NAL_UT_PREFIX:
            needProcess = true;
            break;

        case UMC::NAL_UT_UNSPECIFIED:
        case UMC::NAL_UT_DPA:
        case UMC::NAL_UT_DPB:
        case UMC::NAL_UT_DPC:
        case UMC::NAL_UT_END_OF_SEQ:
        case UMC::NAL_UT_END_OF_STREAM:
        case UMC::NAL_UT_FD:
        case UMC::NAL_UT_SPS_EX:
        default:
            break;
        };

        if (!nalUnit)
        {
            return UMC::UMC_ERR_NOT_ENOUGH_DATA;
        }

        if (needProcess)
        {
            try
            {
#if (MFX_VERSION >= 1025)
                UMC::Status umcRes = m_supplier->ProcessNalUnit(nalUnit, pDecodeErrorReport);
#else
                UMC::Status umcRes = m_supplier->ProcessNalUnit(nalUnit);
#endif
                if (umcRes < UMC::UMC_OK)
                {
                    return UMC::UMC_OK;
                }
            }
            catch(UMC::h264_exception &ex)
            {
                if (ex.GetStatus() != UMC::UMC_ERR_UNSUPPORTED)
                {
                    throw;
                }
            }

            switch ((UMC::NAL_Unit_Type)startCode)
            {
            case UMC::NAL_UT_SPS:
                m_isSPSFound = true;
                break;

            case UMC::NAL_UT_PPS:
                m_isPPSFound = true;
                break;

            case UMC::NAL_UT_SUBSET_SPS:
                m_isSubSPSFound = true;
                break;

            case UMC::NAL_UT_SEI:  // SVC scalable SEI can be before SPS/PPS
            case UMC::NAL_UT_PREFIX:
                if (!m_isSVC_SEIFound)
                {
                    const UMC_H264_DECODER::H264SEIPayLoad * svcPayload = m_supplier->GetHeaders()->m_SEIParams.GetHeader(UMC::SEI_SCALABILITY_INFO);
                    m_isSVC_SEIFound = svcPayload != 0;
                }
                break;

            case UMC::NAL_UT_UNSPECIFIED:
            case UMC::NAL_UT_SLICE:
            case UMC::NAL_UT_IDR_SLICE:
            case UMC::NAL_UT_AUD:
            case UMC::NAL_UT_DPA:
            case UMC::NAL_UT_DPB:
            case UMC::NAL_UT_DPC:
            case UMC::NAL_UT_END_OF_SEQ:
            case UMC::NAL_UT_END_OF_STREAM:
            case UMC::NAL_UT_FD:
            case UMC::NAL_UT_SPS_EX:
            case UMC::NAL_UT_AUXILIARY:
            case UMC::NAL_UT_CODED_SLICE_EXTENSION:
            default:
                break;
            };

            return UMC::UMC_OK;
        }
    }
    catch(const UMC::h264_exception & ex)
    {
        return ex.GetStatus();
    }

    return UMC::UMC_OK;
}

UMC::Status MFX_Utility::DecodeHeader(UMC::TaskSupplier * supplier, UMC::H264VideoDecoderParams* lpInfo, mfxBitstream *bs, mfxVideoParam *out)
{
    UMC::Status umcRes = UMC::UMC_OK;

    if (!lpInfo->m_pData)
        return UMC::UMC_ERR_NULL_PTR;

    if (!lpInfo->m_pData->GetDataSize())
        return UMC::UMC_ERR_NOT_ENOUGH_DATA;

#if (MFX_VERSION >= 1035)
    lpInfo->m_ignore_level_constrain = out->mfx.IgnoreLevelConstrain;
#endif
    umcRes = supplier->PreInit(lpInfo);
    if (umcRes != UMC::UMC_OK)
        return UMC::UMC_ERR_FAILED;

    PosibleMVC headersDecoder(supplier);
    umcRes = headersDecoder.DecodeHeader(lpInfo->m_pData, bs, out);

    if (umcRes != UMC::UMC_OK)
        return umcRes;

    umcRes = supplier->GetInfo(lpInfo);
    if (umcRes == UMC::UMC_OK)
    {

        FillVideoParam(supplier, out, false);
    }
    else
    {
    }

    return umcRes;
}


UMC::Status MFX_Utility::FillVideoParamMVCEx(UMC::TaskSupplier * supplier, ::mfxVideoParam *par)
{

    const UMC_H264_DECODER::H264SeqParamSetMVCExtension * seqMVC = supplier->GetHeaders()->m_SeqParamsMvcExt.GetCurrentHeader();
    mfxExtMVCSeqDesc * points = (mfxExtMVCSeqDesc*)GetExtendedBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_MVC_SEQ_DESC);

    if (!seqMVC)
        return UMC::UMC_OK;
    par->mfx.CodecProfile = seqMVC->profile_idc;
    par->mfx.CodecLevel = (mfxU16)supplier->GetLevelIDC();
    mfxU16 maxDecPicBuffering = seqMVC->vui.bitstream_restriction_flag ? seqMVC->vui.max_dec_frame_buffering : 0;;
    if (par->mfx.MaxDecFrameBuffering && maxDecPicBuffering)
        par->mfx.MaxDecFrameBuffering = std::max(maxDecPicBuffering, par->mfx.MaxDecFrameBuffering);

    if (!points)
        return UMC::UMC_OK;

    uint32_t numRefFrames = (seqMVC->num_ref_frames + 1) * (seqMVC->num_views_minus1 + 1);
    points->NumRefsTotal = (mfxU16)numRefFrames;


    UMC::Status sts = FillVideoParamExtension(seqMVC, par);
    par->mfx.CodecProfile = seqMVC->profile_idc;
    par->mfx.CodecLevel = (mfxU16)supplier->GetLevelIDC();
    return sts;
}

void CheckCrops(const mfxFrameInfo &in, mfxFrameInfo &out, mfxStatus & sts)
{
    mfxU32 maskW = 1;
    mfxU32 maskH = 1;
    if (in.ChromaFormat >= MFX_CHROMAFORMAT_MONOCHROME && in.ChromaFormat <= MFX_CHROMAFORMAT_YUV444)
    {
        maskW = UMC::SubWidthC[in.ChromaFormat];
        maskH = UMC::SubHeightC[in.ChromaFormat];
        if (in.PicStruct != MFX_PICSTRUCT_PROGRESSIVE)
            maskH <<= 1;
    }
    maskW--;
    maskH--;

    out.CropX = in.CropX;
    out.CropY = in.CropY;
    out.CropW = in.CropW;
    out.CropH = in.CropH;

    if ((in.CropX & maskW) || in.CropX + in.CropW > in.Width)
        out.CropX = 0;
    if ((in.CropY & maskH) || in.CropY + in.CropH > in.Height)
        out.CropY = 0;
    if ((in.CropW & maskW) || in.CropX + in.CropW > in.Width)
        out.CropW = 0;
    if ((in.CropH & maskH) || in.CropY + in.CropH > in.Height)
        out.CropH = 0;

    if (out.CropX != in.CropX || out.CropY != in.CropY || out.CropW != in.CropW || out.CropH != in.CropH)
        sts = MFX_ERR_UNSUPPORTED;
}

template <typename T>
void CheckDimensions(T &info_in, T &info_out, mfxStatus & sts)
{
    if (info_in.Width % 16 == 0 && info_in.Width <= 16384)
        info_out.Width = info_in.Width;
    else
    {
        info_out.Width = 0;
        sts = MFX_ERR_UNSUPPORTED;
    }

    if (info_in.Height % 16 == 0 && info_in.Height <= 16384)
        info_out.Height = info_in.Height;
    else
    {
        info_out.Height = 0;
        sts = MFX_ERR_UNSUPPORTED;
    }

    if ((info_in.Width || info_in.Height) && !(info_in.Width && info_in.Height))
    {
        info_out.Width = 0;
        info_out.Height = 0;
        sts = MFX_ERR_UNSUPPORTED;
   }
}

mfxStatus MFX_Utility::Query(VideoCORE *core, mfxVideoParam *in, mfxVideoParam *out, eMFXHWType type)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "MFX_Utility::Query");
    MFX_CHECK_NULL_PTR1(out);
    mfxStatus  sts = MFX_ERR_NONE;

    if (in == out)
    {
        mfxVideoParam in1;
        MFX_INTERNAL_CPY(&in1, in, sizeof(mfxVideoParam));
        return MFX_Utility::Query(core, &in1, out, type);
    }

    memset(&out->mfx, 0, sizeof(mfxInfoMFX));

    if (in)
    {
        out->mfx.MaxDecFrameBuffering = in->mfx.MaxDecFrameBuffering;

        if (in->mfx.CodecId == MFX_CODEC_AVC)
            out->mfx.CodecId = in->mfx.CodecId;

        if ((MFX_PROFILE_UNKNOWN == in->mfx.CodecProfile) ||
            (MFX_PROFILE_AVC_BASELINE == in->mfx.CodecProfile) ||
            (MFX_PROFILE_AVC_MAIN == in->mfx.CodecProfile) ||
            (MFX_PROFILE_AVC_HIGH == in->mfx.CodecProfile) ||
            (MFX_PROFILE_AVC_EXTENDED == in->mfx.CodecProfile) ||
            (MFX_PROFILE_AVC_MULTIVIEW_HIGH == in->mfx.CodecProfile) ||
            (MFX_PROFILE_AVC_STEREO_HIGH == in->mfx.CodecProfile)
            )
            out->mfx.CodecProfile = in->mfx.CodecProfile;
        else
        {
            sts = MFX_ERR_UNSUPPORTED;
        }

        if (core->GetVAType() == MFX_HW_VAAPI)
        {
            int codecProfile = in->mfx.CodecProfile & 0xFF;
            if (    (codecProfile == MFX_PROFILE_AVC_STEREO_HIGH) ||
                    (codecProfile == MFX_PROFILE_AVC_MULTIVIEW_HIGH))
                return MFX_ERR_UNSUPPORTED;

        }

#if (MFX_VERSION >= 1035)
        out->mfx.IgnoreLevelConstrain = in->mfx.IgnoreLevelConstrain;
#endif

        switch (in->mfx.CodecLevel)
        {
        case MFX_LEVEL_UNKNOWN:
        case MFX_LEVEL_AVC_1:
        case MFX_LEVEL_AVC_1b:
        case MFX_LEVEL_AVC_11:
        case MFX_LEVEL_AVC_12:
        case MFX_LEVEL_AVC_13:
        case MFX_LEVEL_AVC_2:
        case MFX_LEVEL_AVC_21:
        case MFX_LEVEL_AVC_22:
        case MFX_LEVEL_AVC_3:
        case MFX_LEVEL_AVC_31:
        case MFX_LEVEL_AVC_32:
        case MFX_LEVEL_AVC_4:
        case MFX_LEVEL_AVC_41:
        case MFX_LEVEL_AVC_42:
        case MFX_LEVEL_AVC_5:
        case MFX_LEVEL_AVC_51:
        case MFX_LEVEL_AVC_52:
#if (MFX_VERSION >= 1035)
        case MFX_LEVEL_AVC_6:
        case MFX_LEVEL_AVC_61:
        case MFX_LEVEL_AVC_62:
#endif
            out->mfx.CodecLevel = in->mfx.CodecLevel;
            break;
        default:
            sts = MFX_ERR_UNSUPPORTED;
            break;
        }

        if (in->mfx.NumThread < 128)
        {
            out->mfx.NumThread = in->mfx.NumThread;
        }
        else
        {
            sts = MFX_ERR_UNSUPPORTED;
        }

        out->AsyncDepth = in->AsyncDepth;

        out->mfx.DecodedOrder = in->mfx.DecodedOrder;

        if (in->mfx.DecodedOrder > 1)
        {
            sts = MFX_ERR_UNSUPPORTED;
            out->mfx.DecodedOrder = 0;
        }

        if (in->mfx.SliceGroupsPresent)
        {
            if (in->mfx.SliceGroupsPresent == 1)
                out->mfx.SliceGroupsPresent = in->mfx.SliceGroupsPresent;
            else
                sts = MFX_ERR_UNSUPPORTED;
        }

        if (in->mfx.TimeStampCalc)
        {
            if (in->mfx.TimeStampCalc == 1)
                in->mfx.TimeStampCalc = out->mfx.TimeStampCalc;
            else
                sts = MFX_ERR_UNSUPPORTED;
        }

        if (in->mfx.ExtendedPicStruct)
        {
            if (in->mfx.ExtendedPicStruct == 1)
                in->mfx.ExtendedPicStruct = out->mfx.ExtendedPicStruct;
            else
                sts = MFX_ERR_UNSUPPORTED;
        }

        if ((in->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY) || (in->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) ||
            (in->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY))
        {
            uint32_t mask = in->IOPattern & 0xf0;
            if (mask == MFX_IOPATTERN_OUT_VIDEO_MEMORY || mask == MFX_IOPATTERN_OUT_SYSTEM_MEMORY || mask == MFX_IOPATTERN_OUT_OPAQUE_MEMORY)
                out->IOPattern = in->IOPattern;
            else
                sts = MFX_ERR_UNSUPPORTED;
        }

        if (in->mfx.FrameInfo.FourCC)
        {
            if (in->mfx.FrameInfo.FourCC == MFX_FOURCC_NV12)
                out->mfx.FrameInfo.FourCC = in->mfx.FrameInfo.FourCC;
            else
                sts = MFX_ERR_UNSUPPORTED;
        }

        if (in->mfx.FrameInfo.ChromaFormat == MFX_CHROMAFORMAT_YUV420 || in->mfx.FrameInfo.ChromaFormat == MFX_CHROMAFORMAT_YUV400 || in->mfx.FrameInfo.ChromaFormat == MFX_CHROMAFORMAT_YUV422)
            out->mfx.FrameInfo.ChromaFormat = in->mfx.FrameInfo.ChromaFormat;
        else
            sts = MFX_ERR_UNSUPPORTED;

        CheckDimensions(in->mfx.FrameInfo, out->mfx.FrameInfo, sts);

        CheckCrops(in->mfx.FrameInfo, out->mfx.FrameInfo, sts);

        out->mfx.FrameInfo.FrameRateExtN = in->mfx.FrameInfo.FrameRateExtN;
        out->mfx.FrameInfo.FrameRateExtD = in->mfx.FrameInfo.FrameRateExtD;

        if ((in->mfx.FrameInfo.FrameRateExtN || in->mfx.FrameInfo.FrameRateExtD) && !(in->mfx.FrameInfo.FrameRateExtN && in->mfx.FrameInfo.FrameRateExtD))
        {
            out->mfx.FrameInfo.FrameRateExtN = 0;
            out->mfx.FrameInfo.FrameRateExtD = 0;
            sts = MFX_ERR_UNSUPPORTED;
        }

        out->mfx.FrameInfo.AspectRatioW = in->mfx.FrameInfo.AspectRatioW;
        out->mfx.FrameInfo.AspectRatioH = in->mfx.FrameInfo.AspectRatioH;

        if ((in->mfx.FrameInfo.AspectRatioW || in->mfx.FrameInfo.AspectRatioH) && !(in->mfx.FrameInfo.AspectRatioW && in->mfx.FrameInfo.AspectRatioH))
        {
            out->mfx.FrameInfo.AspectRatioW = 0;
            out->mfx.FrameInfo.AspectRatioH = 0;
            sts = MFX_ERR_UNSUPPORTED;
        }

        switch (in->mfx.FrameInfo.PicStruct)
        {
        case MFX_PICSTRUCT_UNKNOWN:
        case MFX_PICSTRUCT_PROGRESSIVE:
        case MFX_PICSTRUCT_FIELD_TFF:
        case MFX_PICSTRUCT_FIELD_BFF:
        case MFX_PICSTRUCT_FIELD_REPEATED:
        case MFX_PICSTRUCT_FRAME_DOUBLING:
        case MFX_PICSTRUCT_FRAME_TRIPLING:
            out->mfx.FrameInfo.PicStruct = in->mfx.FrameInfo.PicStruct;
            break;
        default:
            sts = MFX_ERR_UNSUPPORTED;
            break;
        }

        mfxStatus stsExt = CheckDecodersExtendedBuffers(in);
        if (stsExt < MFX_ERR_NONE)
            sts = MFX_ERR_UNSUPPORTED;

        if (in->Protected)
        {
            out->Protected = in->Protected;

            if (type == MFX_HW_UNKNOWN)
            {
                sts = MFX_ERR_UNSUPPORTED;
                out->Protected = 0;
            }

            if (!IS_PROTECTION_ANY(in->Protected))
            {
                sts = MFX_ERR_UNSUPPORTED;
                out->Protected = 0;
            }

            if (!(in->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY))
            {
                out->IOPattern = 0;
                sts = MFX_ERR_UNSUPPORTED;
            }
        }

        mfxExtMVCSeqDesc * mvcPointsIn = (mfxExtMVCSeqDesc *)GetExtendedBuffer(in->ExtParam, in->NumExtParam, MFX_EXTBUFF_MVC_SEQ_DESC);
        mfxExtMVCSeqDesc * mvcPointsOut = (mfxExtMVCSeqDesc *)GetExtendedBuffer(out->ExtParam, out->NumExtParam, MFX_EXTBUFF_MVC_SEQ_DESC);

        if (mvcPointsIn && mvcPointsOut)
        {
            if (mvcPointsIn->NumView <= UMC::H264_MAX_NUM_VIEW && mvcPointsIn->NumView <= mvcPointsIn->NumViewAlloc && mvcPointsOut->NumViewAlloc >= mvcPointsIn->NumView)
            {
                mvcPointsOut->NumView = mvcPointsIn->NumView;

                for (uint32_t i = 0; i < mvcPointsIn->NumView; i++)
                {
                    if (mvcPointsIn->View[i].ViewId >= UMC::H264_MAX_NUM_VIEW)
                        continue;

                    mvcPointsOut->View[i].ViewId = mvcPointsIn->View[i].ViewId;

                    if (mvcPointsIn->View[i].NumAnchorRefsL0 <= UMC::H264_MAX_NUM_VIEW_REF)
                    {
                        mvcPointsOut->View[i].NumAnchorRefsL0 = mvcPointsIn->View[i].NumAnchorRefsL0;
                        MFX_INTERNAL_CPY(mvcPointsOut->View[i].AnchorRefL0, mvcPointsIn->View[i].AnchorRefL0, sizeof(mvcPointsIn->View[i].AnchorRefL0));
                    }
                    else
                        sts = MFX_ERR_UNSUPPORTED;

                    if (mvcPointsIn->View[i].NumAnchorRefsL1 <= UMC::H264_MAX_NUM_VIEW_REF)
                    {
                        mvcPointsOut->View[i].NumAnchorRefsL1 = mvcPointsIn->View[i].NumAnchorRefsL1;
                        MFX_INTERNAL_CPY(mvcPointsOut->View[i].AnchorRefL1, mvcPointsIn->View[i].AnchorRefL1, sizeof(mvcPointsIn->View[i].AnchorRefL1));
                    }
                    else
                        sts = MFX_ERR_UNSUPPORTED;

                    if (mvcPointsIn->View[i].NumNonAnchorRefsL0 <= UMC::H264_MAX_NUM_VIEW_REF)
                    {
                        mvcPointsOut->View[i].NumNonAnchorRefsL0 = mvcPointsIn->View[i].NumNonAnchorRefsL0;
                        MFX_INTERNAL_CPY(mvcPointsOut->View[i].NonAnchorRefL0, mvcPointsIn->View[i].NonAnchorRefL0, sizeof(mvcPointsIn->View[i].NonAnchorRefL0));
                    }
                    else
                        sts = MFX_ERR_UNSUPPORTED;

                    if (mvcPointsIn->View[i].NumNonAnchorRefsL1 <= UMC::H264_MAX_NUM_VIEW_REF)
                    {
                        mvcPointsOut->View[i].NumNonAnchorRefsL1 = mvcPointsIn->View[i].NumNonAnchorRefsL1;
                        MFX_INTERNAL_CPY(mvcPointsOut->View[i].NonAnchorRefL1, mvcPointsIn->View[i].NonAnchorRefL1, sizeof(mvcPointsIn->View[i].NonAnchorRefL1));
                    }
                    else
                        sts = MFX_ERR_UNSUPPORTED;
                }
            }

            if (mvcPointsIn->NumOP <= mvcPointsIn->NumOPAlloc && mvcPointsOut->NumOPAlloc >= mvcPointsIn->NumOP &&
                mvcPointsIn->NumViewId <= mvcPointsIn->NumViewIdAlloc && mvcPointsOut->NumViewIdAlloc >= mvcPointsIn->NumViewId)
            {
                mfxU16 * targetViews = mvcPointsOut->ViewId;

                mvcPointsOut->NumOP = mvcPointsIn->NumOP;
                for (uint32_t i = 0; i < mvcPointsIn->NumOP; i++)
                {
                    if (mvcPointsIn->OP[i].TemporalId <= 7)
                        mvcPointsOut->OP[i].TemporalId = mvcPointsIn->OP[i].TemporalId;
                    else
                        sts = MFX_ERR_UNSUPPORTED;

                    switch (mvcPointsIn->OP[i].LevelIdc)
                    {
                    case MFX_LEVEL_UNKNOWN:
                    case MFX_LEVEL_AVC_1:
                    case MFX_LEVEL_AVC_1b:
                    case MFX_LEVEL_AVC_11:
                    case MFX_LEVEL_AVC_12:
                    case MFX_LEVEL_AVC_13:
                    case MFX_LEVEL_AVC_2:
                    case MFX_LEVEL_AVC_21:
                    case MFX_LEVEL_AVC_22:
                    case MFX_LEVEL_AVC_3:
                    case MFX_LEVEL_AVC_31:
                    case MFX_LEVEL_AVC_32:
                    case MFX_LEVEL_AVC_4:
                    case MFX_LEVEL_AVC_41:
                    case MFX_LEVEL_AVC_42:
                    case MFX_LEVEL_AVC_5:
                    case MFX_LEVEL_AVC_51:
                    case MFX_LEVEL_AVC_52:
#if (MFX_VERSION >= 1035)
                    case MFX_LEVEL_AVC_6:
                    case MFX_LEVEL_AVC_61:
                    case MFX_LEVEL_AVC_62:
#endif
                        mvcPointsOut->OP[i].LevelIdc = mvcPointsIn->OP[i].LevelIdc;
                        break;
                    default:
                        sts = MFX_ERR_UNSUPPORTED;
                    }

                    if (mvcPointsIn->OP[i].NumViews <= UMC::H264_MAX_NUM_VIEW)
                        mvcPointsOut->OP[i].NumViews = mvcPointsIn->OP[i].NumViews;
                    else
                        sts = MFX_ERR_UNSUPPORTED;

                    if (mvcPointsIn->OP[i].NumTargetViews <= mvcPointsIn->OP[i].NumViews && mvcPointsIn->OP[i].NumTargetViews <= UMC::H264_MAX_NUM_VIEW)
                    {
                        mvcPointsOut->OP[i].TargetViewId = targetViews;
                        mvcPointsOut->OP[i].NumTargetViews = mvcPointsIn->OP[i].NumTargetViews;
                        for (uint32_t j = 0; j < mvcPointsIn->OP[i].NumTargetViews; j++)
                        {

                            if (mvcPointsIn->OP[i].TargetViewId[j] <= UMC::H264_MAX_NUM_VIEW)
                                mvcPointsOut->OP[i].TargetViewId[j] = mvcPointsIn->OP[i].TargetViewId[j];
                            else
                                sts = MFX_ERR_UNSUPPORTED;
                        }

                        targetViews += mvcPointsIn->OP[i].NumTargetViews;
                    }
                }
            }

            //if (mvcPointsIn->CompatibilityMode > 0 && mvcPointsIn->CompatibilityMode < 2)
            //    mvcPointsOut->CompatibilityMode = mvcPointsIn->CompatibilityMode;
        }
        else
        {
            if (mvcPointsIn || mvcPointsOut)
            {
                sts = MFX_ERR_UNDEFINED_BEHAVIOR;
            }
        }

        mfxExtMVCTargetViews * targetViewsIn = (mfxExtMVCTargetViews *)GetExtendedBuffer(in->ExtParam, in->NumExtParam, MFX_EXTBUFF_MVC_TARGET_VIEWS);
        mfxExtMVCTargetViews * targetViewsOut = (mfxExtMVCTargetViews *)GetExtendedBuffer(out->ExtParam, out->NumExtParam, MFX_EXTBUFF_MVC_TARGET_VIEWS);

        if (targetViewsIn && targetViewsOut)
        {
            if (targetViewsIn->TemporalId <= 7)
                targetViewsOut->TemporalId = targetViewsIn->TemporalId;
            else
                sts = MFX_ERR_UNSUPPORTED;

            if (targetViewsIn->NumView <= 1024)
                targetViewsOut->NumView = targetViewsIn->NumView;
            else
                sts = MFX_ERR_UNSUPPORTED;

            MFX_INTERNAL_CPY(targetViewsOut->ViewId, targetViewsIn->ViewId, sizeof(targetViewsIn->ViewId));
        }
        else
        {
            if (targetViewsIn || targetViewsOut)
            {
                sts = MFX_ERR_UNDEFINED_BEHAVIOR;
            }
        }


        if (GetPlatform(core, out) != core->GetPlatformType() && sts == MFX_ERR_NONE)
        {
            VM_ASSERT(GetPlatform(core, out) == MFX_PLATFORM_SOFTWARE);
            sts = MFX_WRN_PARTIAL_ACCELERATION;
        }
        /*SFC*/
        mfxExtDecVideoProcessing * videoProcessingTargetIn = (mfxExtDecVideoProcessing *)GetExtendedBuffer(in->ExtParam, in->NumExtParam, MFX_EXTBUFF_DEC_VIDEO_PROCESSING);
        mfxExtDecVideoProcessing * videoProcessingTargetOut = (mfxExtDecVideoProcessing *)GetExtendedBuffer(out->ExtParam, out->NumExtParam, MFX_EXTBUFF_DEC_VIDEO_PROCESSING);
        if (videoProcessingTargetIn && videoProcessingTargetOut)
        {
            if ( (MFX_HW_VAAPI == (core->GetVAType())) &&
                  (MFX_PICSTRUCT_PROGRESSIVE == in->mfx.FrameInfo.PicStruct) &&
                  (in->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) )
            {
                /* Check Input cropping */
                if (!((videoProcessingTargetIn->In.CropX <= videoProcessingTargetIn->In.CropW) &&
                     (videoProcessingTargetIn->In.CropW <= in->mfx.FrameInfo.CropW) &&
                     (videoProcessingTargetIn->In.CropY <= videoProcessingTargetIn->In.CropH) &&
                     (videoProcessingTargetIn->In.CropH <= in->mfx.FrameInfo.CropH) ))
                    sts = MFX_ERR_UNSUPPORTED;

                /* Check output cropping */
                if (!((videoProcessingTargetIn->Out.CropX <= videoProcessingTargetIn->Out.CropW) &&
                     (videoProcessingTargetIn->Out.CropW <= videoProcessingTargetIn->Out.Width) &&
                     ((videoProcessingTargetIn->Out.CropX + videoProcessingTargetIn->Out.CropW)
                                                        <= videoProcessingTargetIn->Out.Width) &&
                     (videoProcessingTargetIn->Out.CropY <= videoProcessingTargetIn->Out.CropH) &&
                     (videoProcessingTargetIn->Out.CropH <= videoProcessingTargetIn->Out.Height) &&
                     ((videoProcessingTargetIn->Out.CropY + videoProcessingTargetIn->Out.CropH )
                                                         <= videoProcessingTargetIn->Out.Height) ))
                    sts = MFX_ERR_UNSUPPORTED;
                if (MFX_ERR_UNSUPPORTED != sts)
                    *videoProcessingTargetOut = *videoProcessingTargetIn;
            }
            else
            {
                sts = MFX_ERR_UNSUPPORTED;
            }
        }

    }
    else
    {
        out->mfx.CodecId = MFX_CODEC_AVC;
        out->mfx.CodecProfile = 1;
        out->mfx.CodecLevel = 1;

        out->mfx.NumThread = 1;

        out->mfx.DecodedOrder = 1;

        out->mfx.TimeStampCalc = 1;
        out->mfx.SliceGroupsPresent = 1;
        out->mfx.ExtendedPicStruct = 1;
        out->AsyncDepth = 1;

        // mfxFrameInfo
        out->mfx.FrameInfo.FourCC = 1;
        out->mfx.FrameInfo.Width = 16;
        out->mfx.FrameInfo.Height = 16;

        //out->mfx.FrameInfo.CropX = 1;
        //out->mfx.FrameInfo.CropY = 1;
        //out->mfx.FrameInfo.CropW = 1;
        //out->mfx.FrameInfo.CropH = 1;

        out->mfx.FrameInfo.FrameRateExtN = 1;
        out->mfx.FrameInfo.FrameRateExtD = 1;

        out->mfx.FrameInfo.AspectRatioW = 1;
        out->mfx.FrameInfo.AspectRatioH = 1;

        out->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;

        out->Protected = 0;

        mfxExtMVCTargetViews * targetViewsOut = (mfxExtMVCTargetViews *)GetExtendedBuffer(out->ExtParam, out->NumExtParam, MFX_EXTBUFF_MVC_TARGET_VIEWS);
        if (targetViewsOut)
        {
            targetViewsOut->TemporalId = 1;
            targetViewsOut->NumView = 1;
        }

        mfxExtMVCSeqDesc * mvcPointsOut = (mfxExtMVCSeqDesc *)GetExtendedBuffer(out->ExtParam, out->NumExtParam, MFX_EXTBUFF_MVC_SEQ_DESC);
        if (mvcPointsOut)
        {
        }

        mfxExtOpaqueSurfaceAlloc * opaqueOut = (mfxExtOpaqueSurfaceAlloc *)GetExtendedBuffer(out->ExtParam, out->NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
        if (opaqueOut)
        {
        }

        out->mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;

        if (type == MFX_HW_UNKNOWN)
        {
            out->IOPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
        }
        else
        {
            out->IOPattern = MFX_IOPATTERN_OUT_VIDEO_MEMORY;
        }
    }

    return sts;
}

bool MFX_Utility::CheckVideoParam(mfxVideoParam *in, eMFXHWType type)
{
    if (!in)
        return false;

    if (in->Protected)
    {
        if (type == MFX_HW_UNKNOWN || !IS_PROTECTION_ANY(in->Protected))
            return false;
    }

    if (MFX_CODEC_AVC != in->mfx.CodecId)
        return false;

    uint32_t profile_idc = UMC::ExtractProfile(in->mfx.CodecProfile);
    switch (profile_idc)
    {
    case MFX_PROFILE_UNKNOWN:
    case MFX_PROFILE_AVC_BASELINE:
    case MFX_PROFILE_AVC_MAIN:
    case MFX_PROFILE_AVC_EXTENDED:
    case MFX_PROFILE_AVC_HIGH:
    case MFX_PROFILE_AVC_STEREO_HIGH:
    case MFX_PROFILE_AVC_MULTIVIEW_HIGH:
        break;
    default:
        return false;
    }



    mfxExtMVCTargetViews * targetViews = (mfxExtMVCTargetViews *)GetExtendedBuffer(in->ExtParam, in->NumExtParam, MFX_EXTBUFF_MVC_TARGET_VIEWS);
    if (targetViews)
    {
        if (targetViews->TemporalId > 7)
            return false;

        if (targetViews->NumView > 1024)
            return false;
    }

    if (in->mfx.FrameInfo.Width > 16384 || (in->mfx.FrameInfo.Width % 16))
        return false;

    if (in->mfx.FrameInfo.Height > 16384 || (in->mfx.FrameInfo.Height % 16))
        return false;


    if (in->mfx.FrameInfo.FourCC != MFX_FOURCC_NV12 && in->mfx.FrameInfo.FourCC != MFX_FOURCC_NV16 &&
        in->mfx.FrameInfo.FourCC != MFX_FOURCC_P010 && in->mfx.FrameInfo.FourCC != MFX_FOURCC_P210)
        return false;

    // both zero or not zero
    if ((in->mfx.FrameInfo.AspectRatioW || in->mfx.FrameInfo.AspectRatioH) && !(in->mfx.FrameInfo.AspectRatioW && in->mfx.FrameInfo.AspectRatioH))
        return false;

    switch (in->mfx.FrameInfo.PicStruct)
    {
    case MFX_PICSTRUCT_UNKNOWN:
    case MFX_PICSTRUCT_PROGRESSIVE:
    case MFX_PICSTRUCT_FIELD_TFF:
    case MFX_PICSTRUCT_FIELD_BFF:
    case MFX_PICSTRUCT_FIELD_REPEATED:
    case MFX_PICSTRUCT_FRAME_DOUBLING:
    case MFX_PICSTRUCT_FRAME_TRIPLING:
        break;
    default:
        return false;
    }

    if (in->mfx.FrameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV420 && in->mfx.FrameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV400 && in->mfx.FrameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV422)
        return false;

    if (in->mfx.FrameInfo.ChromaFormat == MFX_CHROMAFORMAT_YUV422)
    {
        if (in->mfx.FrameInfo.FourCC != MFX_FOURCC_P210 && in->mfx.FrameInfo.FourCC != MFX_FOURCC_NV16)
            return false;
    }

    if (!(in->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) && !(in->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY) && !(in->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY))
        return false;

    if ((in->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) && (in->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY))
        return false;

    if ((in->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY) && (in->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY))
        return false;

    if ((in->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY) && (in->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY))
        return false;

    return true;
}

#endif // MFX_ENABLE_H264_VIDEO_DECODE
