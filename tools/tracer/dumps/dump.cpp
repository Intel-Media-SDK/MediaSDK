/* ****************************************************************************** *\

Copyright (C) 2012-2020 Intel Corporation.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
- Neither the name of Intel Corporation nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL INTEL CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

File Name: dump.cpp

\* ****************************************************************************** */

#include "dump.h"
#if defined(_WIN32) || defined(_WIN64)
    #include "windows.h"
#else
    #include "unistd.h"
#endif

std::string pVoidToHexString(void* x)
{
    std::ostringstream result;
    result << std::setw(16) << std::setfill('0') << std::hex <<std::uppercase << ((mfxU64)x);
    return result.str();
}

struct IdTable
{
    mfxI32 id;
    const char* str;
};

#define TABLE_ENTRY(_name) \
    { _name, #_name }


static IdTable g_BufferIdTable[] =
{
    TABLE_ENTRY(MFX_EXTBUFF_AVC_REFLIST_CTRL),
    TABLE_ENTRY(MFX_EXTBUFF_AVC_TEMPORAL_LAYERS),

    TABLE_ENTRY(MFX_EXTBUFF_CODING_OPTION),
    TABLE_ENTRY(MFX_EXTBUFF_CODING_OPTION2),
    TABLE_ENTRY(MFX_EXTBUFF_CODING_OPTION3),
    TABLE_ENTRY(MFX_EXTBUFF_CODING_OPTION_SPSPPS),


    TABLE_ENTRY(MFX_EXTBUFF_ENCODER_CAPABILITY),
    TABLE_ENTRY(MFX_EXTBUFF_ENCODED_FRAME_INFO),
    TABLE_ENTRY(MFX_EXTBUFF_ENCODER_RESET_OPTION),
    TABLE_ENTRY(MFX_EXTBUFF_ENCODER_ROI),

    TABLE_ENTRY(MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION),

    TABLE_ENTRY(MFX_EXTBUFF_PICTURE_TIMING_SEI),

    TABLE_ENTRY(MFX_EXTBUFF_VPP_AUXDATA),
    TABLE_ENTRY(MFX_EXTBUFF_VPP_COMPOSITE),
    TABLE_ENTRY(MFX_EXTBUFF_VPP_DEINTERLACING),
    TABLE_ENTRY(MFX_EXTBUFF_VPP_DENOISE),
    TABLE_ENTRY(MFX_EXTBUFF_VPP_DETAIL),
    TABLE_ENTRY(MFX_EXTBUFF_VPP_DONOTUSE),
    TABLE_ENTRY(MFX_EXTBUFF_VPP_DOUSE),
    TABLE_ENTRY(MFX_EXTBUFF_VPP_FRAME_RATE_CONVERSION),
    TABLE_ENTRY(MFX_EXTBUFF_VPP_IMAGE_STABILIZATION),
    TABLE_ENTRY(MFX_EXTBUFF_VPP_PICSTRUCT_DETECTION),
    TABLE_ENTRY(MFX_EXTBUFF_VPP_PROCAMP),
    TABLE_ENTRY(MFX_EXTBUFF_VPP_SCENE_ANALYSIS),
    TABLE_ENTRY(MFX_EXTBUFF_VPP_SCENE_CHANGE),
    TABLE_ENTRY(MFX_EXTBUFF_VPP_VIDEO_SIGNAL_INFO),

    TABLE_ENTRY(MFX_EXTBUFF_VIDEO_SIGNAL_INFO),

    TABLE_ENTRY(MFX_EXTBUFF_LOOKAHEAD_CTRL),
    TABLE_ENTRY(MFX_EXTBUFF_LOOKAHEAD_STAT),

    TABLE_ENTRY(MFX_EXTBUFF_FEI_PARAM),
    TABLE_ENTRY(MFX_EXTBUFF_FEI_PREENC_CTRL),
    TABLE_ENTRY(MFX_EXTBUFF_FEI_PREENC_MV_PRED),
    TABLE_ENTRY(MFX_EXTBUFF_FEI_PREENC_MV),
    TABLE_ENTRY(MFX_EXTBUFF_FEI_PREENC_MB),
    TABLE_ENTRY(MFX_EXTBUFF_FEI_ENC_CTRL),
    TABLE_ENTRY(MFX_EXTBUFF_FEI_ENC_MV_PRED),
    TABLE_ENTRY(MFX_EXTBUFF_FEI_ENC_MB),
    TABLE_ENTRY(MFX_EXTBUFF_FEI_ENC_QP),
    TABLE_ENTRY(MFX_EXTBUFF_FEI_ENC_MV),
    TABLE_ENTRY(MFX_EXTBUFF_FEI_ENC_MB_STAT),
    TABLE_ENTRY(MFX_EXTBUFF_FEI_PAK_CTRL),
    TABLE_ENTRY(MFX_EXTBUFF_FEI_SPS),
    TABLE_ENTRY(MFX_EXTBUFF_FEI_PPS),
    TABLE_ENTRY(MFX_EXTBUFF_FEI_SLICE),
    TABLE_ENTRY(MFX_EXTBUFF_FEI_DEC_STREAM_OUT),
    TABLE_ENTRY(MFX_EXTBUFF_FEI_REPACK_CTRL),
    TABLE_ENTRY(MFX_EXTBUF_CAM_GAMMA_CORRECTION),
    TABLE_ENTRY(MFX_EXTBUF_CAM_WHITE_BALANCE),
    TABLE_ENTRY(MFX_EXTBUF_CAM_HOT_PIXEL_REMOVAL),
    TABLE_ENTRY(MFX_EXTBUF_CAM_BLACK_LEVEL_CORRECTION),
    TABLE_ENTRY(MFX_EXTBUF_CAM_VIGNETTE_CORRECTION),
    TABLE_ENTRY(MFX_EXTBUF_CAM_BAYER_DENOISE),
    TABLE_ENTRY(MFX_EXTBUF_CAM_COLOR_CORRECTION_3X3),
    TABLE_ENTRY(MFX_EXTBUF_CAM_PADDING),
    TABLE_ENTRY(MFX_EXTBUF_CAM_PIPECONTROL),
    TABLE_ENTRY(MFX_EXTBUF_CAM_FORWARD_GAMMA_CORRECTION),
    TABLE_ENTRY(MFX_EXTBUF_CAM_LENS_GEOM_DIST_CORRECTION),
    TABLE_ENTRY(MFX_EXTBUF_CAM_TOTAL_COLOR_CONTROL),
    TABLE_ENTRY(MFX_EXTBUFF_LOOKAHEAD_CTRL),
    TABLE_ENTRY(MFX_EXTBUFF_LOOKAHEAD_STAT),
    TABLE_ENTRY(MFX_EXTBUFF_AVC_REFLIST_CTRL),
    TABLE_ENTRY(MFX_EXTBUFF_AVC_TEMPORAL_LAYERS),
    TABLE_ENTRY(MFX_EXTBUFF_ENCODED_FRAME_INFO),
    TABLE_ENTRY(MFX_EXTBUFF_AVC_REFLISTS),
    TABLE_ENTRY(MFX_EXTBUFF_JPEG_QT),
    TABLE_ENTRY(MFX_EXTBUFF_JPEG_HUFFMAN),
    TABLE_ENTRY(MFX_EXTBUFF_MVC_SEQ_DESC),
    TABLE_ENTRY(MFX_EXTBUFF_MVC_TARGET_VIEWS),
    TABLE_ENTRY(MFX_EXTBUFF_HEVC_TILES),
    TABLE_ENTRY(MFX_EXTBUFF_HEVC_PARAM),
    TABLE_ENTRY(MFX_EXTBUFF_HEVC_REGION),
    TABLE_ENTRY(MFX_EXTBUFF_DECODED_FRAME_INFO),
    TABLE_ENTRY(MFX_EXTBUFF_TIME_CODE),
    TABLE_ENTRY(MFX_EXTBUFF_THREADS_PARAM),
    TABLE_ENTRY(MFX_EXTBUFF_PRED_WEIGHT_TABLE),
    TABLE_ENTRY(MFX_EXTBUFF_DIRTY_RECTANGLES),
    TABLE_ENTRY(MFX_EXTBUFF_MOVING_RECTANGLES),
    TABLE_ENTRY(MFX_EXTBUFF_CODING_OPTION_VPS),
    TABLE_ENTRY(MFX_EXTBUFF_VPP_ROTATION),
    TABLE_ENTRY(MFX_EXTBUFF_ENCODED_SLICES_INFO),
    TABLE_ENTRY(MFX_EXTBUFF_VPP_SCALING),
    TABLE_ENTRY(MFX_EXTBUFF_FEI_CODING_OPTION),
    TABLE_ENTRY(MFX_EXTBUFF_ENCODER_IPCM_AREA),
    TABLE_ENTRY(MFX_EXTBUFF_INSERT_HEADERS),
#if (MFX_VERSION >= MFX_VERSION_NEXT)
    TABLE_ENTRY(MFX_EXTBUFF_VP9_SEGMENTATION),
    TABLE_ENTRY(MFX_EXTBUFF_VP9_TEMPORAL_LAYERS),
    TABLE_ENTRY(MFX_EXTBUFF_VP9_PARAM)
#endif
};

static IdTable tbl_impl[] = {
    TABLE_ENTRY(MFX_IMPL_SOFTWARE),
    TABLE_ENTRY(MFX_IMPL_HARDWARE),
    TABLE_ENTRY(MFX_IMPL_AUTO_ANY),
    TABLE_ENTRY(MFX_IMPL_HARDWARE_ANY),
    TABLE_ENTRY(MFX_IMPL_HARDWARE2),
    TABLE_ENTRY(MFX_IMPL_HARDWARE3),
    TABLE_ENTRY(MFX_IMPL_HARDWARE4),
    TABLE_ENTRY(MFX_IMPL_RUNTIME),
    TABLE_ENTRY(MFX_IMPL_VIA_ANY),
    TABLE_ENTRY(MFX_IMPL_VIA_D3D9),
    TABLE_ENTRY(MFX_IMPL_VIA_D3D11),
    TABLE_ENTRY(MFX_IMPL_VIA_VAAPI),
    TABLE_ENTRY(MFX_IMPL_AUDIO)
};

static IdTable tbl_fourcc[] = {
    TABLE_ENTRY(MFX_FOURCC_NV12),
    TABLE_ENTRY(MFX_FOURCC_YV12),
    TABLE_ENTRY(MFX_FOURCC_NV16),
    TABLE_ENTRY(MFX_FOURCC_YUY2),
    TABLE_ENTRY(MFX_FOURCC_RGB3),
    TABLE_ENTRY(MFX_FOURCC_RGB4),
    TABLE_ENTRY(MFX_FOURCC_P8),
    TABLE_ENTRY(MFX_FOURCC_P8_TEXTURE),
    TABLE_ENTRY(MFX_FOURCC_P010),
#if (MFX_VERSION >= MFX_VERSION_NEXT)
    TABLE_ENTRY(MFX_FOURCC_P016),
#endif
    TABLE_ENTRY(MFX_FOURCC_P210),
    TABLE_ENTRY(MFX_FOURCC_BGR4),
    TABLE_ENTRY(MFX_FOURCC_A2RGB10),
    TABLE_ENTRY(MFX_FOURCC_ARGB16),
    TABLE_ENTRY(MFX_FOURCC_R16),
    TABLE_ENTRY(MFX_FOURCC_ABGR16),
    TABLE_ENTRY(MFX_FOURCC_AYUV),
    TABLE_ENTRY(MFX_FOURCC_AYUV_RGB4),
    TABLE_ENTRY(MFX_FOURCC_UYVY),
#if (MFX_VERSION >= 1027)
    TABLE_ENTRY(MFX_FOURCC_Y210),
    TABLE_ENTRY(MFX_FOURCC_Y410),
#endif
    TABLE_ENTRY(MFX_FOURCC_NV21),
    TABLE_ENTRY(MFX_FOURCC_IYUV),
    TABLE_ENTRY(MFX_FOURCC_I010),
#if (MFX_VERSION >= MFX_VERSION_NEXT)
    TABLE_ENTRY(MFX_FOURCC_Y216),
    TABLE_ENTRY(MFX_FOURCC_Y416),
    TABLE_ENTRY(MFX_FOURCC_RGBP),
#endif
};

static IdTable tbl_sts[] = {
    TABLE_ENTRY(MFX_ERR_NONE),
    TABLE_ENTRY(MFX_ERR_UNKNOWN),
    TABLE_ENTRY(MFX_ERR_NULL_PTR),
    TABLE_ENTRY(MFX_ERR_UNSUPPORTED),
    TABLE_ENTRY(MFX_ERR_MEMORY_ALLOC),
    TABLE_ENTRY(MFX_ERR_NOT_ENOUGH_BUFFER),
    TABLE_ENTRY(MFX_ERR_INVALID_HANDLE),
    TABLE_ENTRY(MFX_ERR_LOCK_MEMORY),
    TABLE_ENTRY(MFX_ERR_NOT_INITIALIZED),
    TABLE_ENTRY(MFX_ERR_NOT_FOUND),
    TABLE_ENTRY(MFX_ERR_MORE_DATA),
    TABLE_ENTRY(MFX_ERR_MORE_SURFACE),
    TABLE_ENTRY(MFX_ERR_ABORTED),
    TABLE_ENTRY(MFX_ERR_DEVICE_LOST),
    TABLE_ENTRY(MFX_ERR_INCOMPATIBLE_VIDEO_PARAM),
    TABLE_ENTRY(MFX_ERR_INVALID_VIDEO_PARAM),
    TABLE_ENTRY(MFX_ERR_UNDEFINED_BEHAVIOR),
    TABLE_ENTRY(MFX_ERR_DEVICE_FAILED),
    TABLE_ENTRY(MFX_ERR_MORE_BITSTREAM),
    TABLE_ENTRY(MFX_ERR_INCOMPATIBLE_AUDIO_PARAM),
    TABLE_ENTRY(MFX_ERR_INVALID_AUDIO_PARAM),
    TABLE_ENTRY(MFX_WRN_IN_EXECUTION),
    TABLE_ENTRY(MFX_WRN_DEVICE_BUSY),
    TABLE_ENTRY(MFX_WRN_VIDEO_PARAM_CHANGED),
    TABLE_ENTRY(MFX_WRN_PARTIAL_ACCELERATION),
    TABLE_ENTRY(MFX_WRN_INCOMPATIBLE_VIDEO_PARAM),
    TABLE_ENTRY(MFX_WRN_VALUE_NOT_CHANGED),
    TABLE_ENTRY(MFX_WRN_OUT_OF_RANGE),
    TABLE_ENTRY(MFX_WRN_FILTER_SKIPPED),
    TABLE_ENTRY(MFX_WRN_INCOMPATIBLE_AUDIO_PARAM),
    TABLE_ENTRY(MFX_TASK_DONE),
    TABLE_ENTRY(MFX_TASK_WORKING),
    TABLE_ENTRY(MFX_TASK_BUSY),
    TABLE_ENTRY(MFX_ERR_NONE_PARTIAL_OUTPUT)
};

static IdTable tbl_codecid[] = {
    TABLE_ENTRY(MFX_CODEC_AVC),
    TABLE_ENTRY(MFX_CODEC_HEVC),
    TABLE_ENTRY(MFX_CODEC_MPEG2),
    TABLE_ENTRY(MFX_CODEC_VC1),
    TABLE_ENTRY(MFX_CODEC_CAPTURE),
    TABLE_ENTRY(MFX_CODEC_VP9),
    TABLE_ENTRY(MFX_CODEC_AV1)
};

static IdTable tbl_iopattern[] = {
    TABLE_ENTRY(MFX_IOPATTERN_IN_VIDEO_MEMORY),
    TABLE_ENTRY(MFX_IOPATTERN_IN_SYSTEM_MEMORY),
    TABLE_ENTRY(MFX_IOPATTERN_IN_OPAQUE_MEMORY),
    TABLE_ENTRY(MFX_IOPATTERN_OUT_VIDEO_MEMORY),
    TABLE_ENTRY(MFX_IOPATTERN_OUT_SYSTEM_MEMORY),
    TABLE_ENTRY(MFX_IOPATTERN_OUT_OPAQUE_MEMORY)
};


const char* DumpContext::get_bufferid_str(mfxU32 bufferid)
{
    for (size_t i = 0; i < GET_ARRAY_SIZE(g_BufferIdTable); ++i)
    {
        if (g_BufferIdTable[i].id == static_cast<int>(bufferid))
        {
            return g_BufferIdTable[i].str;
        }
    }
    return NULL;
}

std::string GetmfxIMPL(mfxIMPL impl) {

    std::basic_stringstream<char> stream;
    std::string name = "UNKNOWN";

    for (unsigned int i = 0; i < (sizeof(tbl_impl) / sizeof(tbl_impl[0])); i++)
        if (tbl_impl[i].id == (impl & (MFX_IMPL_VIA_ANY - 1)))
            name = tbl_impl[i].str;
    stream << name;

    int via_flag = impl & ~(MFX_IMPL_VIA_ANY - 1);

    if (0 != via_flag)
    {
        stream << "|";
        name = "UNKNOWN";
        for (unsigned int i = 0; i < (sizeof(tbl_impl) / sizeof(tbl_impl[0])); i++)
            if (tbl_impl[i].id == via_flag)
                name = tbl_impl[i].str;
        stream << name;
    }
    return stream.str();
}

std::string GetFourCC(mfxU32 fourcc) {

    std::basic_stringstream<char> stream;
    std::string name = "UNKNOWN";
    int j = 0;
    for (unsigned int i = 0; i < (sizeof(tbl_fourcc) / sizeof(tbl_fourcc[0])); i++)
    {
        if (tbl_fourcc[i].id == static_cast<int>(fourcc))
        {
            name = "";
            while (tbl_fourcc[i].str[j + 11] != '\0')
            {
                name = name + tbl_fourcc[i].str[j + 11];
                j++;
            }
            name = name + "\0";
            break;
        }
    }
    stream<<name;
    return stream.str();
}

std::string GetStatusString(mfxStatus sts) {

    std::basic_stringstream<char> stream;
    std::string name = "UNKNOWN_STATUS";
    for (unsigned int i = 0; i < (sizeof(tbl_sts) / sizeof(tbl_sts[0])); i++)
    {
        if (tbl_sts[i].id == sts)
        {
            name = tbl_sts[i].str;
            break;
        }

    }
    stream<<name;
    return stream.str();
}

std::string GetCodecIdString(mfxU32 id) {

    std::basic_stringstream<char> stream;
    std::string name = "UNKNOWN";
    for (unsigned int i = 0; i < (sizeof(tbl_codecid) / sizeof(tbl_codecid[0])); i++)
    {
        if (tbl_codecid[i].id == static_cast<int>(id))
        {
            name = tbl_codecid[i].str;
            break;
        }

    }
    stream<<name;
    return stream.str();
}

std::string GetIOPattern(mfxU32 io) {

    std::basic_stringstream<char> stream;
    std::string name;
    for (unsigned int i = 0; i < (sizeof(tbl_iopattern) / sizeof(tbl_iopattern[0])); i++)
    {
        if (tbl_iopattern[i].id & static_cast<int>(io))
        {
            name += tbl_iopattern[i].str;
            name += "; ";
        }

    }
    if (!name.length())
    {
        name = "UNKNOWN";
        name += "(" + ToString(io) + ")";
    }
    stream<<name;
    return stream.str();
}

bool _IsBadReadPtr(void *ptr, size_t size)
{
#if defined(_WIN32) || defined(_WIN64)
    return !!IsBadReadPtr(ptr, size);
#else
    int fb[2];
    if (pipe(fb) >= 0)
    {
        bool tmp = (write(fb[1], ptr, size) <= 0);
        close(fb[0]);
        close(fb[1]);
        return tmp;
    }
    return true;
#endif
}

//mfxcommon
DEFINE_GET_TYPE_DEF(mfxBitstream);
DEFINE_GET_TYPE_DEF(mfxExtBuffer);
DEFINE_GET_TYPE_DEF(mfxIMPL);
DEFINE_GET_TYPE_DEF(mfxInitParam);
DEFINE_GET_TYPE_DEF(mfxPriority);
DEFINE_GET_TYPE_DEF(mfxVersion);
DEFINE_GET_TYPE_DEF(mfxSyncPoint);

//mfxenc
DEFINE_GET_TYPE_DEF(mfxENCInput);
DEFINE_GET_TYPE_DEF(mfxENCOutput);

//mfxplugin
DEFINE_GET_TYPE_DEF(mfxPlugin);

//mfxstructures
DEFINE_GET_TYPE_DEF(mfxDecodeStat);
DEFINE_GET_TYPE_DEF(mfxEncodeCtrl);
DEFINE_GET_TYPE_DEF(mfxEncodeStat);
DEFINE_GET_TYPE_DEF(mfxExtCodingOption);
DEFINE_GET_TYPE_DEF(mfxExtCodingOption2);
DEFINE_GET_TYPE_DEF(mfxExtCodingOption3);
DEFINE_GET_TYPE_DEF(mfxExtEncoderResetOption);
DEFINE_GET_TYPE_DEF(mfxExtVppAuxData);
DEFINE_GET_TYPE_DEF(mfxFrameAllocRequest);
DEFINE_GET_TYPE_DEF(mfxFrameData);
DEFINE_GET_TYPE_DEF(mfxFrameId);
DEFINE_GET_TYPE_DEF(mfxFrameInfo);
DEFINE_GET_TYPE_DEF(mfxFrameSurface1);
DEFINE_GET_TYPE_DEF(mfxHandleType);
DEFINE_GET_TYPE_DEF(mfxInfoMFX);
DEFINE_GET_TYPE_DEF(mfxInfoVPP);
DEFINE_GET_TYPE_DEF(mfxPayload);
DEFINE_GET_TYPE_DEF(mfxSkipMode);
DEFINE_GET_TYPE_DEF(mfxVideoParam);
DEFINE_GET_TYPE_DEF(mfxVPPStat);
DEFINE_GET_TYPE_DEF(mfxExtVPPDoNotUse);
DEFINE_GET_TYPE_DEF(mfxExtCodingOptionVPS);
DEFINE_GET_TYPE_DEF(mfxExtVPPRotation);
DEFINE_GET_TYPE_DEF(mfxExtEncodedSlicesInfo);
DEFINE_GET_TYPE_DEF(mfxExtVPPScaling);
DEFINE_GET_TYPE_DEF(mfxExtVPPMirroring);
DEFINE_GET_TYPE_DEF(mfxExtMVOverPicBoundaries);
DEFINE_GET_TYPE_DEF(mfxExtVPPColorFill);
DEFINE_GET_TYPE_DEF(mfxExtEncoderIPCMArea);
DEFINE_GET_TYPE_DEF(mfxExtInsertHeaders);

//mfxsession
DEFINE_GET_TYPE_DEF(mfxSession);

//mfxvideo
DEFINE_GET_TYPE_DEF(mfxBufferAllocator);
DEFINE_GET_TYPE_DEF(mfxFrameAllocator);

//mfxfei
DEFINE_GET_TYPE_DEF(mfxExtFeiPreEncCtrl);
DEFINE_GET_TYPE_DEF(mfxExtFeiPreEncMV);
DEFINE_GET_TYPE_DEF(mfxExtFeiPreEncMBStat);
DEFINE_GET_TYPE_DEF(mfxExtFeiEncFrameCtrl);
DEFINE_GET_TYPE_DEF(mfxExtFeiEncMVPredictors);
DEFINE_GET_TYPE_DEF(mfxExtFeiEncQP);
DEFINE_GET_TYPE_DEF(mfxExtFeiEncMBCtrl);
DEFINE_GET_TYPE_DEF(mfxExtFeiEncMV);
DEFINE_GET_TYPE_DEF(mfxExtFeiEncMBStat);
DEFINE_GET_TYPE_DEF(mfxFeiPakMBCtrl);
DEFINE_GET_TYPE_DEF(mfxExtFeiPakMBCtrl);
DEFINE_GET_TYPE_DEF(mfxExtFeiParam);
DEFINE_GET_TYPE_DEF(mfxPAKInput);
DEFINE_GET_TYPE_DEF(mfxPAKOutput);
DEFINE_GET_TYPE_DEF(mfxExtFeiSPS);
DEFINE_GET_TYPE_DEF(mfxExtFeiPPS);
DEFINE_GET_TYPE_DEF(mfxExtFeiSliceHeader);
DEFINE_GET_TYPE_DEF(mfxExtFeiCodingOption);
DEFINE_GET_TYPE_DEF(mfxExtFeiRepackCtrl);
DEFINE_GET_TYPE_DEF(mfxFeiDecStreamOutMBCtrl);
DEFINE_GET_TYPE_DEF(mfxExtFeiDecStreamOut);


