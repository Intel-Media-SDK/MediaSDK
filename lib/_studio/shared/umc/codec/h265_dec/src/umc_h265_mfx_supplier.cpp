// Copyright (c) 2017 Intel Corporation
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
#ifdef UMC_ENABLE_H265_VIDEO_DECODER

#include "umc_h265_mfx_supplier.h"

#include "umc_h265_frame_list.h"
#include "umc_h265_nal_spl.h"
#include "umc_h265_bitstream_headers.h"

#include "umc_h265_dec_defs.h"

#include "vm_sys_info.h"

#include "umc_h265_debug.h"
#include "mfx_common_int.h"

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
    return &m_buffer[0];
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
    m_sps.Reset();
    m_pps.Reset();
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

// Decode headers nal unit
UMC::Status MFXTaskSupplier_H265::DecodeHeaders(UMC::MediaDataEx *nalUnit)
{
    UMC::Status sts = TaskSupplier_H265::DecodeHeaders(nalUnit);
    if (sts != UMC::UMC_OK)
        return sts;

    {
        // save sps/pps
        uint32_t nal_unit_type = nalUnit->GetExData()->values[0];
        switch(nal_unit_type)
        {
            case NAL_UT_SPS:
            case NAL_UT_PPS:
                {
                    static const uint8_t start_code_prefix[] = {0, 0, 0, 1};

                    size_t size = nalUnit->GetDataSize();
                    bool isSPS = (nal_unit_type == NAL_UT_SPS);
                    RawHeader_H265 * hdr = isSPS ? GetSPS() : GetPPS();
                    int32_t id = isSPS ? m_Headers.m_SeqParams.GetCurrentID() : m_Headers.m_PicParams.GetCurrentID();
                    hdr->Resize(id, size + sizeof(start_code_prefix));
                    MFX_INTERNAL_CPY(hdr->GetPointer(), start_code_prefix,  sizeof(start_code_prefix));
                    MFX_INTERNAL_CPY(hdr->GetPointer() + sizeof(start_code_prefix), (uint8_t*)nalUnit->GetDataPointer(), size);
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

                m_sei_messages->AddMessage(&nalUnit1, m_SEIPayLoads.payLoadType);
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

// Check HW capabilities
bool MFX_Utility::IsNeedPartialAcceleration_H265(mfxVideoParam* par, eMFXHWType /*type*/)
{
    if (!par)
        return false;

#if defined(MFX_VA_LINUX)
    if (par->mfx.FrameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV420 && par->mfx.FrameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV400)
        return true;

    if (par->mfx.FrameInfo.FourCC == MFX_FOURCC_P210 || par->mfx.FrameInfo.FourCC == MFX_FOURCC_NV16)
        return true;
#endif

    return false;
}

inline
int MatchProfile(mfxU32 fourcc)
{
    switch (fourcc)
    {
        case MFX_FOURCC_NV12: return MFX_PROFILE_HEVC_MAIN;

        case MFX_FOURCC_P010: return MFX_PROFILE_HEVC_MAIN10;

        case MFX_FOURCC_NV16: return MFX_PROFILE_HEVC_REXT;

    }

    return MFX_PROFILE_UNKNOWN;
}

// Returns implementation platform
eMFXPlatform MFX_Utility::GetPlatform_H265(VideoCORE * core, mfxVideoParam * par)
{
    if (!par)
        return MFX_PLATFORM_SOFTWARE;

    eMFXPlatform platform = core->GetPlatformType();
    eMFXHWType typeHW = MFX_HW_UNKNOWN;
    typeHW = core->GetHWType();

    if (IsNeedPartialAcceleration_H265(par, typeHW) && platform != MFX_PLATFORM_SOFTWARE)
    {
        return MFX_PLATFORM_SOFTWARE;
    }

    if (platform != MFX_PLATFORM_SOFTWARE)
    {
        if (core->IsGuidSupported(DXVA_ModeHEVC_VLD_Main, par) != MFX_ERR_NONE)
            return MFX_PLATFORM_SOFTWARE;
    }
    return platform;
}

bool MFX_CDECL MFX_Utility::IsBugSurfacePoolApplicable(eMFXHWType hwtype, mfxVideoParam * par)
{
    if (par == NULL)
        return false;


    return false;
}

inline
mfxU32 CalculateFourcc(int codecProfile, mfxFrameInfo const* frameInfo)
{
    //map profile + chroma fmt + bit depth => fcc
    //Main   - [4:2:0], [8] bit
    //Main10 - [4:2:0], [8, 10] bit
    //Extent - [4:2:0, 4:2:2, 4:4:4], [8, 10, 12, 16]

    if (codecProfile > MFX_PROFILE_HEVC_REXT)
        return 0;

    if (frameInfo->ChromaFormat > MFX_CHROMAFORMAT_YUV420)
        return 0;

    if (frameInfo->BitDepthLuma < 8 ||
        frameInfo->BitDepthLuma > 10)
        return 0;

    //map chroma fmt & bit depth onto fourcc (NOTE: we currently don't support bit depth above 10 bit)
    mfxU32 const map[][4] =
    {
            /* 8 bit */      /* 10 bit */
        { MFX_FOURCC_NV12, MFX_FOURCC_P010, 0, 0 }, //400
        { MFX_FOURCC_NV12, MFX_FOURCC_P010, 0, 0 }, //420
    };

    if (codecProfile == MFX_PROFILE_HEVC_MAIN &&
       (frameInfo->ChromaFormat   != MFX_CHROMAFORMAT_YUV420 ||
       (frameInfo->BitDepthLuma   != 8 &&
        frameInfo->BitDepthChroma != 8)))
        return 0;
    else if (codecProfile == MFX_PROFILE_HEVC_MAIN10 &&
            (frameInfo->ChromaFormat   != MFX_CHROMAFORMAT_YUV420 ||
            (frameInfo->BitDepthLuma   > 10 &&
             frameInfo->BitDepthChroma > 10)))
        return 0;
    if (codecProfile == MFX_PROFILE_HEVC_MAINSP &&
       (frameInfo->ChromaFormat   != MFX_CHROMAFORMAT_YUV420 ||
       (frameInfo->BitDepthLuma   != 8 &&
        frameInfo->BitDepthChroma != 8)))
        //A.3.4: MFX_PROFILE_HEVC_MAINSP is restricted to
        // - chroma_format_idc equal to 1 only
        // - bit_depth_luma_minus8/bit_depth_chroma_minus8 equal to 0 only
        return 0;

    VM_ASSERT(
        (frameInfo->ChromaFormat == MFX_CHROMAFORMAT_YUV400 ||
         frameInfo->ChromaFormat == MFX_CHROMAFORMAT_YUV420 ||
         frameInfo->ChromaFormat == MFX_CHROMAFORMAT_YUV422) &&
        "Unsupported chroma format, should be validated before"
    );

    mfxU16 bit_depth =
       MFX_MAX(frameInfo->BitDepthLuma, frameInfo->BitDepthChroma);

    //align luma depth up to 2 (8-10-12 ...)
    bit_depth = (bit_depth + 2 - 1) & ~(2 - 1);

    VM_ASSERT(
        (bit_depth ==  8 ||
         bit_depth == 10) &&
        "Unsupported bit depth, should be validated before"
    );

    return
        map[frameInfo->ChromaFormat][(bit_depth - 8) / 2];
}

inline
bool CheckFourcc(mfxU32 fourcc, int codecProfile, mfxFrameInfo const* frameInfo)
{
    VM_ASSERT(frameInfo);
    mfxFrameInfo fi = *frameInfo;

    if (codecProfile == MFX_PROFILE_UNKNOWN)
        //no profile defined, derive it from FOURCC
        codecProfile = MatchProfile(fourcc);

    if (!fi.BitDepthLuma)
    {
        //no depth defined, derive it from FOURCC
        switch (fourcc)
        {
            case MFX_FOURCC_NV12:
            case MFX_FOURCC_NV16:
                fi.BitDepthLuma = 8;
                break;

            case MFX_FOURCC_P010:
            case MFX_FOURCC_P210:
                fi.BitDepthLuma = 10;
                break;

            default:
                return false;
        }
    }

    if (!fi.BitDepthChroma)
        fi.BitDepthChroma = fi.BitDepthLuma;

    if (!fi.ChromaFormat)
    {
        //no chroma defined, derive it from FOURCC
        switch (fourcc)
        {
            case MFX_FOURCC_NV12:
            case MFX_FOURCC_P010:
                fi.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
                break;

            case MFX_FOURCC_NV16:
            case MFX_FOURCC_P210:
                fi.ChromaFormat = MFX_CHROMAFORMAT_YUV422;
                break;

            default:
                return false;
        }
    }

    return
        CalculateFourcc(codecProfile, &fi) == fourcc;
}

// Initialize mfxVideoParam structure based on decoded bitstream header values
UMC::Status MFX_Utility::FillVideoParam(const H265SeqParamSet * seq, mfxVideoParam *par, bool full)
{
    par->mfx.CodecId = MFX_CODEC_HEVC;

    par->mfx.FrameInfo.Width = (mfxU16) (seq->pic_width_in_luma_samples);
    par->mfx.FrameInfo.Height = (mfxU16) (seq->pic_height_in_luma_samples);

    par->mfx.FrameInfo.Width = UMC::align_value<mfxU16>(par->mfx.FrameInfo.Width, 16);
    par->mfx.FrameInfo.Height = UMC::align_value<mfxU16>(par->mfx.FrameInfo.Height, 16);

    par->mfx.FrameInfo.BitDepthLuma = (mfxU16) (seq->bit_depth_luma);
    par->mfx.FrameInfo.BitDepthChroma = (mfxU16) (seq->bit_depth_chroma);
    par->mfx.FrameInfo.Shift = 0;

    //if (seq->frame_cropping_flag)
    {
        par->mfx.FrameInfo.CropX = (mfxU16)(seq->conf_win_left_offset + seq->def_disp_win_left_offset);
        par->mfx.FrameInfo.CropY = (mfxU16)(seq->conf_win_top_offset + seq->def_disp_win_top_offset);
        par->mfx.FrameInfo.CropH = (mfxU16)(par->mfx.FrameInfo.Height - (seq->conf_win_top_offset + seq->conf_win_bottom_offset + seq->def_disp_win_top_offset + seq->def_disp_win_bottom_offset));
        par->mfx.FrameInfo.CropW = (mfxU16)(par->mfx.FrameInfo.Width - (seq->conf_win_left_offset + seq->conf_win_right_offset + seq->def_disp_win_left_offset + seq->def_disp_win_right_offset));

        par->mfx.FrameInfo.CropH -= (mfxU16)(par->mfx.FrameInfo.Height - seq->pic_height_in_luma_samples);
        par->mfx.FrameInfo.CropW -= (mfxU16)(par->mfx.FrameInfo.Width - seq->pic_width_in_luma_samples);
    }

    par->mfx.FrameInfo.PicStruct = static_cast<mfxU16>(seq->field_seq_flag  ? MFX_PICSTRUCT_FIELD_SINGLE : MFX_PICSTRUCT_PROGRESSIVE);
    par->mfx.FrameInfo.ChromaFormat = seq->chroma_format_idc;

    if (seq->aspect_ratio_info_present_flag || full)
    {
        par->mfx.FrameInfo.AspectRatioW = (mfxU16)seq->sar_width;
        par->mfx.FrameInfo.AspectRatioH = (mfxU16)seq->sar_height;
    }
    else
    {
        par->mfx.FrameInfo.AspectRatioW = 0;
        par->mfx.FrameInfo.AspectRatioH = 0;
    }

    if (seq->getTimingInfo()->vps_timing_info_present_flag || full)
    {
        par->mfx.FrameInfo.FrameRateExtD = seq->getTimingInfo()->vps_num_units_in_tick;
        par->mfx.FrameInfo.FrameRateExtN = seq->getTimingInfo()->vps_time_scale;
    }
    else
    {
        par->mfx.FrameInfo.FrameRateExtD = 0;
        par->mfx.FrameInfo.FrameRateExtN = 0;
    }

    par->mfx.CodecProfile = (mfxU16)seq->m_pcPTL.GetGeneralPTL()->profile_idc;
    par->mfx.CodecLevel = (mfxU16)seq->m_pcPTL.GetGeneralPTL()->level_idc;
    par->mfx.MaxDecFrameBuffering = (mfxU16)seq->sps_max_dec_pic_buffering[0];

    par->mfx.FrameInfo.FourCC = CalculateFourcc(par->mfx.CodecProfile, &par->mfx.FrameInfo);

    par->mfx.DecodedOrder = 0;

    // video signal section
    mfxExtVideoSignalInfo * videoSignal = (mfxExtVideoSignalInfo *)GetExtendedBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_VIDEO_SIGNAL_INFO);
    if (videoSignal)
    {
        videoSignal->VideoFormat = (mfxU16)seq->video_format;
        videoSignal->VideoFullRange = (mfxU16)seq->video_full_range_flag;
        videoSignal->ColourDescriptionPresent = (mfxU16)seq->colour_description_present_flag;
        videoSignal->ColourPrimaries = (mfxU16)seq->colour_primaries;
        videoSignal->TransferCharacteristics = (mfxU16)seq->transfer_characteristics;
        videoSignal->MatrixCoefficients = (mfxU16)seq->matrix_coeffs;
    }

    mfxExtHEVCParam * hevcParam = (mfxExtHEVCParam *)GetExtendedBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_HEVC_PARAM);
    if (hevcParam)
    {
        hevcParam->PicWidthInLumaSamples = (mfxU16) (seq->pic_width_in_luma_samples);
        hevcParam->PicHeightInLumaSamples = (mfxU16) (seq->pic_height_in_luma_samples);

        hevcParam->GeneralConstraintFlags = 0;
        hevcParam->GeneralConstraintFlags |= seq->getPTL()->GetGeneralPTL()->max_12bit_constraint_flag ? MFX_HEVC_CONSTR_REXT_MAX_12BIT : 0;
        hevcParam->GeneralConstraintFlags |= seq->getPTL()->GetGeneralPTL()->max_10bit_constraint_flag ? MFX_HEVC_CONSTR_REXT_MAX_10BIT : 0;
        hevcParam->GeneralConstraintFlags |= seq->getPTL()->GetGeneralPTL()->max_8bit_constraint_flag ? MFX_HEVC_CONSTR_REXT_MAX_8BIT : 0;
        hevcParam->GeneralConstraintFlags |= seq->getPTL()->GetGeneralPTL()->max_422chroma_constraint_flag ? MFX_HEVC_CONSTR_REXT_MAX_422CHROMA : 0;
        hevcParam->GeneralConstraintFlags |= seq->getPTL()->GetGeneralPTL()->max_420chroma_constraint_flag ? MFX_HEVC_CONSTR_REXT_MAX_420CHROMA : 0;
        hevcParam->GeneralConstraintFlags |= seq->getPTL()->GetGeneralPTL()->max_monochrome_constraint_flag ? MFX_HEVC_CONSTR_REXT_MAX_MONOCHROME : 0;
        hevcParam->GeneralConstraintFlags |= seq->getPTL()->GetGeneralPTL()->intra_constraint_flag ? MFX_HEVC_CONSTR_REXT_INTRA : 0;
        hevcParam->GeneralConstraintFlags |= seq->getPTL()->GetGeneralPTL()->one_picture_only_constraint_flag ? MFX_HEVC_CONSTR_REXT_ONE_PICTURE_ONLY : 0;
        hevcParam->GeneralConstraintFlags |= seq->getPTL()->GetGeneralPTL()->lower_bit_rate_constraint_flag ? MFX_HEVC_CONSTR_REXT_LOWER_BIT_RATE : 0;
    }

    return UMC::UMC_OK;
}

// Initialize mfxVideoParam structure based on decoded bitstream header values
UMC::Status MFX_Utility::FillVideoParam(TaskSupplier_H265 * supplier, mfxVideoParam *par, bool full)
{
    const H265SeqParamSet * seq = supplier->GetHeaders()->m_SeqParams.GetCurrentHeader();
    if (!seq)
        return UMC::UMC_ERR_FAILED;

    if (MFX_Utility::FillVideoParam(seq, par, full) != UMC::UMC_OK)
        return UMC::UMC_ERR_FAILED;

    return UMC::UMC_OK;
}

// Helper class for gathering header NAL units
class HeadersAnalyzer
{
public:
    HeadersAnalyzer(TaskSupplier_H265 * supplier);
    virtual ~HeadersAnalyzer();

    // Decode a memory buffer looking for header NAL units in it
    virtual UMC::Status DecodeHeader(UMC::MediaData* params, mfxBitstream *bs, mfxVideoParam *out);
    // Find headers nal units and parse them
    virtual UMC::Status ProcessNalUnit(UMC::MediaData * data);
    // Returns whether necessary headers are found
    virtual bool IsEnough() const;

protected:
    bool m_isVPSFound;
    bool m_isSPSFound;
    bool m_isPPSFound;

    TaskSupplier_H265 * m_supplier;
    H265Slice * m_lastSlice;
};

HeadersAnalyzer::HeadersAnalyzer(TaskSupplier_H265 * supplier)
    : m_isVPSFound(false)
    , m_isSPSFound(false)
    , m_isPPSFound(false)
    , m_supplier(supplier)
    , m_lastSlice(0)
{
}

HeadersAnalyzer::~HeadersAnalyzer()
{
    if (m_lastSlice)
        m_lastSlice->DecrementReference();
}

// Returns whether necessary headers are found
bool HeadersAnalyzer::IsEnough() const
{
    return m_isSPSFound && m_isPPSFound;
}

// Decode a memory buffer looking for header NAL units in it
UMC::Status HeadersAnalyzer::DecodeHeader(UMC::MediaData * data, mfxBitstream *bs, mfxVideoParam *)
{
    if (!data)
        return UMC::UMC_ERR_NULL_PTR;

    m_lastSlice = 0;

    H265SeqParamSet* first_sps = 0;
    notifier0<H265SeqParamSet> sps_guard(&H265Slice::DecrementReference);

    UMC::Status umcRes = UMC::UMC_ERR_NOT_ENOUGH_DATA;
    for ( ; data->GetDataSize() > 3; )
    {
        m_supplier->GetNalUnitSplitter()->MoveToStartCode(data); // move data pointer to start code

        if (!m_isSPSFound) // move point to first start code
        {
            bs->DataOffset = (mfxU32)((mfxU8*)data->GetDataPointer() - (mfxU8*)data->GetBufferPointer());
            bs->DataLength = (mfxU32)data->GetDataSize();
        }

        umcRes = ProcessNalUnit(data);

        if (umcRes == UMC::UMC_ERR_UNSUPPORTED)
            umcRes = UMC::UMC_OK;

        if (umcRes != UMC::UMC_OK)
            break;

        if (!first_sps && m_isSPSFound)
        {
            first_sps = m_supplier->GetHeaders()->m_SeqParams.GetCurrentHeader();
            VM_ASSERT(first_sps && "Current SPS should be valid when [m_isSPSFound]");

            first_sps->IncrementReference();
            sps_guard.Reset(first_sps);
        }

        if (IsEnough())
            break;
    }

    if (umcRes == UMC::UMC_ERR_SYNC) // move pointer
    {
        bs->DataOffset = (mfxU32)((mfxU8*)data->GetDataPointer() - (mfxU8*)data->GetBufferPointer());
        bs->DataLength = (mfxU32)data->GetDataSize();
        return UMC::UMC_ERR_NOT_ENOUGH_DATA;
    }

    if (umcRes == UMC::UMC_ERR_NOT_ENOUGH_DATA)
    {
        bool isEOS = ((data->GetFlags() & UMC::MediaData::FLAG_VIDEO_DATA_END_OF_STREAM) != 0) ||
            ((data->GetFlags() & UMC::MediaData::FLAG_VIDEO_DATA_NOT_FULL_FRAME) == 0);
        if (isEOS)
        {
            return UMC::UMC_OK;
        }
    }

    if (IsEnough())
    {
        H265SeqParamSet* last_sps = m_supplier->GetHeaders()->m_SeqParams.GetCurrentHeader();
        if (first_sps && first_sps != last_sps)
            m_supplier->GetHeaders()->m_SeqParams.AddHeader(first_sps);

        return UMC::UMC_OK;
    }

    return UMC::UMC_ERR_NOT_ENOUGH_DATA;
}

// Find headers nal units and parse them
UMC::Status HeadersAnalyzer::ProcessNalUnit(UMC::MediaData * data)
{
    try
    {
        int32_t startCode = m_supplier->GetNalUnitSplitter()->CheckNalUnitType(data);

        bool needProcess = false;

        UMC::MediaDataEx *nalUnit = m_supplier->GetNalUnit(data);

        switch (startCode)
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
            {
                if (IsEnough())
                {
                    return UMC::UMC_OK;
                }
                else
                    break; // skip nal unit
            }
            break;

        case NAL_UT_VPS:
        case NAL_UT_SPS:
        case NAL_UT_PPS:
            needProcess = true;
            break;

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
                UMC::Status umcRes = m_supplier->ProcessNalUnit(nalUnit);
                if (umcRes < UMC::UMC_OK)
                {
                    return UMC::UMC_OK;
                }
            }
            catch(h265_exception ex)
            {
                if (ex.GetStatus() != UMC::UMC_ERR_UNSUPPORTED)
                {
                    throw;
                }
            }

            switch (startCode)
            {
            case NAL_UT_VPS:
                m_isVPSFound = true;
                break;

            case NAL_UT_SPS:
                m_isSPSFound = true;
                break;

            case NAL_UT_PPS:
                m_isPPSFound = true;
                break;

            default:
                break;
            };

            return UMC::UMC_OK;
        }
    }
    catch(const h265_exception & ex)
    {
        return ex.GetStatus();
    }

    return UMC::UMC_OK;
}

// Find bitstream header NAL units, parse them and fill application parameters structure
UMC::Status MFX_Utility::DecodeHeader(TaskSupplier_H265 * supplier, UMC::VideoDecoderParams* params, mfxBitstream *bs, mfxVideoParam *out)
{
    UMC::Status umcRes = UMC::UMC_OK;

    if (!params->m_pData)
        return UMC::UMC_ERR_NULL_PTR;

    if (!params->m_pData->GetDataSize())
        return UMC::UMC_ERR_NOT_ENOUGH_DATA;

    umcRes = supplier->PreInit(params);
    if (umcRes != UMC::UMC_OK)
        return UMC::UMC_ERR_FAILED;

    HeadersAnalyzer headersDecoder(supplier);
    umcRes = headersDecoder.DecodeHeader(params->m_pData, bs, out);

    if (umcRes != UMC::UMC_OK)
        return umcRes;

    return
        umcRes = supplier->GetInfo(params);
}

// MediaSDK DECODE_Query API function
mfxStatus MFX_CDECL MFX_Utility::Query_H265(VideoCORE *core, mfxVideoParam *in, mfxVideoParam *out, eMFXHWType type)
{
    MFX_CHECK_NULL_PTR1(out);
    mfxStatus  sts = MFX_ERR_NONE;

    if (in == out)
    {
        mfxVideoParam in1;
        MFX_INTERNAL_CPY(&in1, in, sizeof(mfxVideoParam));
        return MFX_Utility::Query_H265(core, &in1, out, type);
    }

    memset(&out->mfx, 0, sizeof(mfxInfoMFX));

    if (in)
    {
        if (in->mfx.CodecId == MFX_CODEC_HEVC)
            out->mfx.CodecId = in->mfx.CodecId;

        if (MFX_PROFILE_UNKNOWN == in->mfx.CodecProfile ||
            MFX_PROFILE_HEVC_MAIN == in->mfx.CodecProfile ||
            MFX_PROFILE_HEVC_MAIN10 == in->mfx.CodecProfile ||
            MFX_PROFILE_HEVC_MAINSP == in->mfx.CodecProfile ||
            MFX_PROFILE_HEVC_REXT == in->mfx.CodecProfile)
            out->mfx.CodecProfile = in->mfx.CodecProfile;
        else
        {
            sts = MFX_ERR_UNSUPPORTED;
        }

        switch (in->mfx.CodecLevel)
        {
        case MFX_LEVEL_UNKNOWN:
        case MFX_LEVEL_HEVC_1:
        case MFX_LEVEL_HEVC_2:
        case MFX_LEVEL_HEVC_21:
        case MFX_LEVEL_HEVC_3:
        case MFX_LEVEL_HEVC_31:
        case MFX_LEVEL_HEVC_4:
        case MFX_LEVEL_HEVC_41:
        case MFX_LEVEL_HEVC_5:
        case MFX_LEVEL_HEVC_51:
        case MFX_LEVEL_HEVC_52:
        case MFX_LEVEL_HEVC_6:
        case MFX_LEVEL_HEVC_61:
        case MFX_LEVEL_HEVC_62:
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
            // mfxFrameInfo
            if (in->mfx.FrameInfo.FourCC == MFX_FOURCC_NV12 ||
                in->mfx.FrameInfo.FourCC == MFX_FOURCC_P010 ||
                in->mfx.FrameInfo.FourCC == MFX_FOURCC_P210
                )
                out->mfx.FrameInfo.FourCC = in->mfx.FrameInfo.FourCC;
            else
                sts = MFX_ERR_UNSUPPORTED;
        }

        if (in->mfx.FrameInfo.ChromaFormat == MFX_CHROMAFORMAT_YUV400 ||
            in->mfx.FrameInfo.ChromaFormat == MFX_CHROMAFORMAT_YUV420
            )
            out->mfx.FrameInfo.ChromaFormat = in->mfx.FrameInfo.ChromaFormat;
        else
            sts = MFX_ERR_UNSUPPORTED;

        if (in->mfx.FrameInfo.Width % 16 == 0 && in->mfx.FrameInfo.Width <= 16384)
            out->mfx.FrameInfo.Width = in->mfx.FrameInfo.Width;
        else
        {
            out->mfx.FrameInfo.Width = 0;
            sts = MFX_ERR_UNSUPPORTED;
        }

        if (in->mfx.FrameInfo.Height % 16 == 0 && in->mfx.FrameInfo.Height <= 16384)
            out->mfx.FrameInfo.Height = in->mfx.FrameInfo.Height;
        else
        {
            out->mfx.FrameInfo.Height = 0;
            sts = MFX_ERR_UNSUPPORTED;
        }

        if ((in->mfx.FrameInfo.Width || in->mfx.FrameInfo.Height) && !(in->mfx.FrameInfo.Width && in->mfx.FrameInfo.Height))
        {
            out->mfx.FrameInfo.Width = 0;
            out->mfx.FrameInfo.Height = 0;
            sts = MFX_ERR_UNSUPPORTED;
        }

        /*if (in->mfx.FrameInfo.CropX <= out->mfx.FrameInfo.Width)
            out->mfx.FrameInfo.CropX = in->mfx.FrameInfo.CropX;

        if (in->mfx.FrameInfo.CropY <= out->mfx.FrameInfo.Height)
            out->mfx.FrameInfo.CropY = in->mfx.FrameInfo.CropY;

        if (out->mfx.FrameInfo.CropX + in->mfx.FrameInfo.CropW <= out->mfx.FrameInfo.Width)
            out->mfx.FrameInfo.CropW = in->mfx.FrameInfo.CropW;

        if (out->mfx.FrameInfo.CropY + in->mfx.FrameInfo.CropH <= out->mfx.FrameInfo.Height)
            out->mfx.FrameInfo.CropH = in->mfx.FrameInfo.CropH;*/

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

        out->mfx.FrameInfo.BitDepthLuma = in->mfx.FrameInfo.BitDepthLuma;

        if (in->mfx.FrameInfo.BitDepthLuma && (in->mfx.FrameInfo.BitDepthLuma < 8 || in->mfx.FrameInfo.BitDepthLuma > 16))
        {
            out->mfx.FrameInfo.BitDepthLuma = 0;
            sts = MFX_ERR_UNSUPPORTED;
        }

        out->mfx.FrameInfo.BitDepthChroma = in->mfx.FrameInfo.BitDepthChroma;
        if (in->mfx.FrameInfo.BitDepthChroma && (in->mfx.FrameInfo.BitDepthChroma < 8 || in->mfx.FrameInfo.BitDepthChroma > 16))
        {
            out->mfx.FrameInfo.BitDepthChroma = 0;
            sts = MFX_ERR_UNSUPPORTED;
        }

        if (in->mfx.FrameInfo.FourCC &&
            !CheckFourcc(in->mfx.FrameInfo.FourCC, in->mfx.CodecProfile, &in->mfx.FrameInfo))
        {
            out->mfx.FrameInfo.FourCC = 0;
            sts = MFX_ERR_UNSUPPORTED;
        }

        out->mfx.FrameInfo.Shift = in->mfx.FrameInfo.Shift;
        if (in->mfx.FrameInfo.FourCC == MFX_FOURCC_P010 || in->mfx.FrameInfo.FourCC == MFX_FOURCC_P210)
        {
            if (in->mfx.FrameInfo.Shift > 1)
            {
                out->mfx.FrameInfo.Shift = 0;
                sts = MFX_ERR_UNSUPPORTED;
            }
        }
        else
        {
            if (in->mfx.FrameInfo.Shift)
            {
                out->mfx.FrameInfo.Shift = 0;
                sts = MFX_ERR_UNSUPPORTED;
            }
        }

        switch (in->mfx.FrameInfo.PicStruct)
        {
        case MFX_PICSTRUCT_UNKNOWN:
        case MFX_PICSTRUCT_PROGRESSIVE:
        case MFX_PICSTRUCT_FIELD_SINGLE:
            out->mfx.FrameInfo.PicStruct = in->mfx.FrameInfo.PicStruct;
            break;
        default:
            sts = MFX_ERR_UNSUPPORTED;
            break;
        }

        mfxStatus stsExt = CheckDecodersExtendedBuffers(in);
        if (stsExt < MFX_ERR_NONE)
            sts = MFX_ERR_UNSUPPORTED;


        if (GetPlatform_H265(core, out) != core->GetPlatformType() && sts == MFX_ERR_NONE)
        {
            VM_ASSERT(GetPlatform_H265(core, out) == MFX_PLATFORM_SOFTWARE);
            sts = MFX_ERR_UNSUPPORTED;
        }
    }
    else
    {
        out->mfx.CodecId = MFX_CODEC_HEVC;
        out->mfx.CodecProfile = 1;
        out->mfx.CodecLevel = 1;

        out->mfx.NumThread = 1;

        out->mfx.DecodedOrder = 1;

        out->mfx.SliceGroupsPresent = 1;
        out->mfx.ExtendedPicStruct = 1;
        out->AsyncDepth = 1;

        // mfxFrameInfo
        out->mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
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

        out->mfx.FrameInfo.BitDepthLuma = 8;
        out->mfx.FrameInfo.BitDepthChroma = 8;
        out->mfx.FrameInfo.Shift = 0;

        out->Protected = 0;

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

// Validate input parameters
bool MFX_CDECL MFX_Utility::CheckVideoParam_H265(mfxVideoParam *in, eMFXHWType type)
{
    if (!in)
        return false;

    type = type;

    if (MFX_CODEC_HEVC != in->mfx.CodecId)
        return false;

    // FIXME: Add check that width is multiple of minimal CU size
    if (in->mfx.FrameInfo.Width > 16384 /* || (in->mfx.FrameInfo.Width % in->mfx.FrameInfo.reserved[0]) */)
        return false;

    // FIXME: Add check that height is multiple of minimal CU size
    if (in->mfx.FrameInfo.Height > 16384 /* || (in->mfx.FrameInfo.Height % in->mfx.FrameInfo.reserved[0]) */)
        return false;


    if (in->mfx.FrameInfo.FourCC != MFX_FOURCC_NV12 &&
        in->mfx.FrameInfo.FourCC != MFX_FOURCC_NV16 &&
        in->mfx.FrameInfo.FourCC != MFX_FOURCC_P010 &&
        in->mfx.FrameInfo.FourCC != MFX_FOURCC_P210
        )
        return false;

    // both zero or not zero
    if ((in->mfx.FrameInfo.AspectRatioW || in->mfx.FrameInfo.AspectRatioH) && !(in->mfx.FrameInfo.AspectRatioW && in->mfx.FrameInfo.AspectRatioH))
        return false;

    if (in->mfx.CodecProfile != MFX_PROFILE_HEVC_MAIN &&
        in->mfx.CodecProfile != MFX_PROFILE_HEVC_MAIN10 &&
        in->mfx.CodecProfile != MFX_PROFILE_HEVC_MAINSP &&
        in->mfx.CodecProfile != MFX_PROFILE_HEVC_REXT)
        return false;

    if ((in->mfx.FrameInfo.BitDepthLuma) && (in->mfx.FrameInfo.BitDepthLuma < 8 || in->mfx.FrameInfo.BitDepthLuma > 16))
        return false;

    if ((in->mfx.FrameInfo.BitDepthChroma) && (in->mfx.FrameInfo.BitDepthChroma < 8 || in->mfx.FrameInfo.BitDepthChroma > 16))
        return false;

    if (!CheckFourcc(in->mfx.FrameInfo.FourCC, in->mfx.CodecProfile, &in->mfx.FrameInfo))
        return false;

    if (in->mfx.FrameInfo.FourCC == MFX_FOURCC_P010 ||
        in->mfx.FrameInfo.FourCC == MFX_FOURCC_P210
        )
    {
        if (in->mfx.FrameInfo.Shift > 1)
            return false;
    }
    else
    {
        if (in->mfx.FrameInfo.Shift)
            return false;

        if (in->mfx.FrameInfo.BitDepthLuma && in->mfx.FrameInfo.BitDepthLuma != 8)
            return false;

        if (in->mfx.FrameInfo.BitDepthChroma && in->mfx.FrameInfo.BitDepthChroma != 8)
            return false;
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
    case MFX_PICSTRUCT_FIELD_SINGLE:
        break;
    default:
        return false;
    }

    if (in->mfx.FrameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV400 &&
        in->mfx.FrameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV420 &&
        in->mfx.FrameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV422
        )
        return false;

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

} // namespace UMC_HEVC_DECODER

#endif // UMC_ENABLE_H265_VIDEO_DECODER
