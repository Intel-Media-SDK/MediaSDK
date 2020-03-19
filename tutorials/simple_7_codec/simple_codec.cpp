// Copyright (c) 2019 Intel Corporation
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

#include "common_utils.h"
#include <vector>

/*****************************************************************************

Tutorial--Check the codec capability of the current platform:
This program checks the hardware capabilities of the platform.
It iterates on the decoders, encoders and vpp modules on the current GPU, for
each codec, it will query and initialize the hardware through the Media SDK API
and report the results based on the returned MSDK status.

*****************************************************************************/

#if defined(_WIN32) || defined(_WIN64)
#define PRINT_ERROR(_msg, _str) \
    SetConsoleTextAttribute(hConsole, 12);      \
    printf(_msg, _str); \
    SetConsoleTextAttribute(hConsole, curInfo.wAttributes)
#else
#define PRINT_ERROR   printf
#endif

//The structure defines the entry for each checking.
typedef struct {
    char* (*fillparam_func)(mfxVideoParam *, void *);
    bool load_plugin;
    bool attach_options;
} CodecTask;

char* fillH264DecodeParams(mfxVideoParam *param, void* extOpts) {
    static char strRet[] = "decoder H.264 with resolution(4096x2304)";

    memset(param, 0, sizeof(*param));

    param->IOPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
    param->mfx.CodecId = MFX_CODEC_AVC;
    param->mfx.CodecProfile = MFX_PROFILE_AVC_HIGH;
    param->mfx.CodecLevel = MFX_LEVEL_AVC_41;

    // - Width and height of buffer must be aligned, a multiple of 32
    // - Frame surface array keeps pointers all surface planes and general frame info
    param->mfx.FrameInfo.Width = MSDK_ALIGN16(4096);
    param->mfx.FrameInfo.Height = MSDK_ALIGN16(2304);
    param->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    param->mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;

    return strRet;
}

char* fillHEVCDecodeParams(mfxVideoParam *param, void* extOpts) {
    static char strRet[] = "decoder H.265 with resolution(8192x8192)";

    memset(param, 0, sizeof(*param));

    param->IOPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
    param->mfx.CodecId = MFX_CODEC_HEVC;
    param->mfx.CodecProfile = MFX_PROFILE_HEVC_MAIN;
    param->mfx.CodecLevel = MFX_LEVEL_HEVC_4;

    // - Width and height of buffer must be aligned, a multiple of 32
    // - Frame surface array keeps pointers all surface planes and general frame info
    param->mfx.FrameInfo.Width = MSDK_ALIGN16(8192);
    param->mfx.FrameInfo.Height = MSDK_ALIGN16(8192);
    param->mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
    param->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    param->mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;

    return strRet;
}

char* fillHEVC10DecodeParams(mfxVideoParam *param, void* extOpts) {
    static char strRet[] = "decoder H.265 10bit with resolution(8192x8192); (4096x4096) on SkyLake";

    memset(param, 0, sizeof(*param));

    param->IOPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
    param->mfx.CodecId = MFX_CODEC_HEVC;
    param->mfx.CodecProfile = MFX_PROFILE_HEVC_MAIN10;
    param->mfx.CodecLevel = MFX_LEVEL_HEVC_51;

    // - Width and height of buffer must be aligned, a multiple of 32
    // - Frame surface array keeps pointers all surface planes and general frame info
    param->mfx.FrameInfo.Width = MSDK_ALIGN16(8192);
    param->mfx.FrameInfo.Height = MSDK_ALIGN16(8192);
    param->mfx.FrameInfo.FourCC = MFX_FOURCC_P010;
    param->mfx.FrameInfo.BitDepthChroma = 10;
    param->mfx.FrameInfo.BitDepthLuma = 10;
    param->mfx.FrameInfo.Shift = 1;
    param->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    param->mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;

    return strRet;
}

char* fillMPEG2DecodeParams(mfxVideoParam *param, void* extOpts) {
    static char strRet[] = "decoder MPEG-2 with resolution(2048x2048)";

    memset(param, 0, sizeof(*param));

    param->IOPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
    param->mfx.CodecId = MFX_CODEC_MPEG2;
    param->mfx.CodecProfile = MFX_PROFILE_MPEG2_MAIN;
    param->mfx.CodecLevel = MFX_LEVEL_MPEG2_HIGH;

    // - Width and height of buffer must be aligned, a multiple of 32
    // - Frame surface array keeps pointers all surface planes and general frame info
    param->mfx.FrameInfo.Width = MSDK_ALIGN16(2048);
    param->mfx.FrameInfo.Height = MSDK_ALIGN16(2048);
    param->mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
    param->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    param->mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;

    return strRet;
}

char* fillVC1DecodeParams(mfxVideoParam *param, void* extOpts) {
    static char strRet[] = "decoder VC1 with resolution(1920x1080)";

    memset(param, 0, sizeof(*param));

    param->IOPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
    param->mfx.CodecId = MFX_CODEC_VC1;
    param->mfx.CodecProfile = MFX_PROFILE_VC1_ADVANCED;

    // - Width and height of buffer must be aligned, a multiple of 32
    // - Frame surface array keeps pointers all surface planes and general frame info
    param->mfx.FrameInfo.Width = MSDK_ALIGN16(1920);
    param->mfx.FrameInfo.Height = MSDK_ALIGN16(1080);
    param->mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
    param->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    param->mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;

    return strRet;
}

char* fillMJPEGDecodeParams(mfxVideoParam *param, void* extOpts) {
    static char strRet[] = "decoder MJPEG with resolution(640x480)";

    memset(param, 0, sizeof(*param));
    param->IOPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
    param->mfx.CodecId = MFX_CODEC_JPEG;

    // - Width and height of buffer must be aligned, a multiple of 32
    // - Frame surface array keeps pointers all surface planes and general frame info
    param->mfx.FrameInfo.Width = MSDK_ALIGN16(640);
    param->mfx.FrameInfo.Height = MSDK_ALIGN16(480);
    param->mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
    param->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    param->mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
    param->mfx.FrameInfo.FrameRateExtN = 30;
    param->mfx.FrameInfo.FrameRateExtD = 1;

    return strRet;
}

char* fillVP8DecodeParams(mfxVideoParam *param, void* extOpts) {
    static char strRet[] = "decoder VP8 with resolution(1920x1080)";

    memset(param, 0, sizeof(*param));

    param->IOPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
    param->mfx.CodecId = MFX_CODEC_VP8;
    param->mfx.CodecProfile = MFX_PROFILE_VP8_3;

    // - Width and height of buffer must be aligned, a multiple of 32
    // - Frame surface array keeps pointers all surface planes and general frame info
    param->mfx.FrameInfo.Width = MSDK_ALIGN16(1920);
    param->mfx.FrameInfo.Height = MSDK_ALIGN16(1080);
    param->mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
    param->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    param->mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;

    return strRet;
}

//static mfxExtCodingOption2 extCodingOption2;

char* fillH264EncodeParams(mfxVideoParam *param, void* extOpts) {
    static char strRet[] = "encoder H.264 with resolution(4096x4096)";

    memset(param, 0, sizeof(*param));

    param->IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY;
    param->mfx.CodecId = MFX_CODEC_AVC;
    param->mfx.TargetUsage = MFX_TARGETUSAGE_BALANCED;
    param->mfx.RateControlMethod = MFX_RATECONTROL_VBR;
    param->mfx.TargetKbps = 10466;

    // For encoder, Width must be a multiple of 16
    // Height must be a multiple of 16 in case of frame picture and a multiple of 32 in case of field picture
    param->mfx.FrameInfo.Width = MSDK_ALIGN16(4096);//maximum for skylake is 4096
    param->mfx.FrameInfo.Height = MSDK_ALIGN16(4096);//maximum for skylake is 4096
    param->mfx.FrameInfo.CropW = 4096;
    param->mfx.FrameInfo.CropH = 4096;
    param->mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
    param->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    param->mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
    param->mfx.FrameInfo.FrameRateExtN = 30;
    param->mfx.FrameInfo.FrameRateExtD = 1;

    if (extOpts != 0) {
        // -------------------- init mfxExtCodingOption2 ---------------------
        mfxExtCodingOption2 *extOption2;
        extOption2 = (mfxExtCodingOption2 *)extOpts;
        memset(extOption2, 0, sizeof(mfxExtCodingOption2));

        extOption2->Header.BufferId = MFX_EXTBUFF_CODING_OPTION2;
        extOption2->Header.BufferSz = sizeof(mfxExtCodingOption2);
        extOption2->IntRefType = 1;
        extOption2->IntRefCycleSize = 8;
        extOption2->IntRefQPDelta = 0;
        // --------------------------------------------------------------------
    }

    return strRet;
}

char* fillHEVCEncodeParams(mfxVideoParam *param, void* extOpts) {
    static char strRet[] = "encoder H.265 with resolution(4096x2176)";

    memset(param, 0, sizeof(*param));

    param->IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY;
    param->mfx.CodecId = MFX_CODEC_HEVC;
    param->mfx.CodecProfile = MFX_PROFILE_HEVC_MAIN;

    // For encoder, Width must be a multiple of 16
    // Height must be a multiple of 16 in case of frame picture and a multiple of 32 in case of field picture
    param->mfx.FrameInfo.Width = MSDK_ALIGN16(4096);//4096
    param->mfx.FrameInfo.Height = MSDK_ALIGN16(2160);//2176
    param->mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
    param->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    param->mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
    param->mfx.FrameInfo.FrameRateExtN = 60;
    param->mfx.FrameInfo.FrameRateExtD = 1;

    return strRet;
}

char* fillHEVC10EncodeParams(mfxVideoParam *param, void* extOpts) {
    static char strRet[] = "encoder H.265 10bit with resolution(4096x2176) only on KabyLake";

    memset(param, 0, sizeof(*param));

    param->IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY;
    param->mfx.CodecId = MFX_CODEC_HEVC;
    param->mfx.CodecProfile = MFX_PROFILE_HEVC_MAIN10;
    param->mfx.CodecLevel = MFX_LEVEL_HEVC_51;

    // For encoder, Width must be a multiple of 16
    // Height must be a multiple of 16 in case of frame picture and a multiple of 32 in case of field picture
    param->mfx.FrameInfo.Width = MSDK_ALIGN16(4096);//4096
    param->mfx.FrameInfo.Height = MSDK_ALIGN16(2176);//2176
    param->mfx.FrameInfo.FourCC = MFX_FOURCC_P010;
    param->mfx.FrameInfo.BitDepthChroma = 10;
    param->mfx.FrameInfo.BitDepthLuma = 10;
    param->mfx.FrameInfo.Shift = 1;
    param->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    param->mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
    param->mfx.FrameInfo.FrameRateExtN = 60;
    param->mfx.FrameInfo.FrameRateExtD = 1;

    return strRet;
}

char* fillMPEG2EncodeParams(mfxVideoParam *param, void* extOpts) {
    static char strRet[] = "encoder MPEG-2 with resolution(1920x1080)";

    memset(param, 0, sizeof(*param));

    param->IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY;
    param->mfx.CodecId = MFX_CODEC_MPEG2;
    param->mfx.TargetKbps = 45000;
    param->mfx.EnableReallocRequest = 1;

    // For encoder, Width must be a multiple of 16
    // Height must be a multiple of 16 in case of frame picture and a multiple of 32 in case of field picture
    param->mfx.FrameInfo.Width = MSDK_ALIGN16(1920);
    param->mfx.FrameInfo.Height = MSDK_ALIGN16(1080);
    param->mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
    param->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    param->mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
    param->mfx.FrameInfo.FrameRateExtN = 30;
    param->mfx.FrameInfo.FrameRateExtD = 1;

    return strRet;
}

char* fillMJPEGEncodeParams(mfxVideoParam *param, void* extOpts) {
    static char strRet[] = "encoder MJPEG with resolution(640x480)";

    memset(param, 0, sizeof(*param));

    param->IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY;
    param->mfx.CodecId = MFX_CODEC_JPEG;
    param->mfx.JPEGChromaFormat = MFX_JPEG_COLORFORMAT_YCbCr;

    // - Width and height of buffer must be aligned, a multiple of 32
    // - Frame surface array keeps pointers all surface planes and general frame info
    param->mfx.FrameInfo.Width = MSDK_ALIGN16(640);
    param->mfx.FrameInfo.Height = MSDK_ALIGN16(480);
    param->mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
    param->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    param->mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
    param->mfx.FrameInfo.FrameRateExtN = 30;
    param->mfx.FrameInfo.FrameRateExtD = 1;

    return strRet;
}

char* fillVPPParams(mfxVideoParam *param, void* extOpts) {
    static char strRet[] = "VPP with resolution(4096x4096)";

    memset(param, 0, sizeof(*param));

    param->IOPattern =
        MFX_IOPATTERN_IN_SYSTEM_MEMORY | MFX_IOPATTERN_OUT_SYSTEM_MEMORY;

    // Input stream parameters
    param->vpp.In.FourCC = MFX_FOURCC_NV12;
    param->vpp.In.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
    param->vpp.In.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    param->vpp.In.FrameRateExtN = 30;
    param->vpp.In.FrameRateExtD = 1;
    // width must be a multiple of 16
    // height must be a multiple of 16 in case of frame picture and a multiple of 32 in case of field picture
    param->vpp.In.Width = MSDK_ALIGN16(4096);
    param->vpp.In.Height =MSDK_ALIGN16(4096);

    // Output stream parameters
    param->vpp.Out.FourCC = param->vpp.In.FourCC;
    param->vpp.Out.ChromaFormat = param->vpp.In.ChromaFormat;
    param->vpp.Out.PicStruct = param->vpp.In.PicStruct;
    param->vpp.Out.FrameRateExtN = param->vpp.In.FrameRateExtN;
    param->vpp.Out.FrameRateExtD = param->vpp.In.FrameRateExtD;
    param->vpp.Out.Width = param->vpp.In.Width;
    param->vpp.Out.Height = param->vpp.In.Height;

    return strRet;
}

//The task list for the decoder checking
CodecTask dec_tasks[] = {
    { fillH264DecodeParams, false, false },
    { fillHEVCDecodeParams, true, false }, //H.265 plugin must be loaded during the runtime.
    { fillHEVC10DecodeParams, true, false }, //H.265 plugin must be loaded during the runtime.
    { fillMPEG2DecodeParams, false, false },
    { fillVC1DecodeParams, false, false },
    { fillMJPEGDecodeParams, false, false },
    { fillVP8DecodeParams, true, false } //VP8 plugin must be loaded during the runtime.
};

//The task list for the encoder checking
CodecTask enc_tasks[] = {
    { fillH264EncodeParams, false, false }, //Set "attach_option" to true to attached the extCodingOption2 parameters.
    { fillHEVCEncodeParams, true, false }, //H.265 plugin must be loaded during the runtime.
    { fillHEVC10EncodeParams, true, false }, //H.265 plugin must be loaded during the runtime.
    { fillMPEG2EncodeParams, false, false },
    { fillMJPEGEncodeParams, false, false }
};

int main(int argc, char** argv)
{
    mfxStatus sts = MFX_ERR_NONE;

#if defined(_WIN32) || defined(_WIN64)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO curInfo;

    GetConsoleScreenBufferInfo(hConsole, &curInfo);

    //1. Show the processor information from the current platform
    showCPUInfo();
#endif

    //2. Initialize Intel Media SDK session with the HW acceleration if available (on any adapter)
    // - Version 1.0 is selected for greatest backwards compatibility.
    // OS specific notes
    // - On Windows both SW and HW libraries may present
    // - On Linux HW library only is available
    // If more recent API features are needed, change the version accordingly
    mfxVersion ver = { { 0, 1 } };
    MFXVideoSession session;
    sts = Initialize(MFX_IMPL_HARDWARE, ver, &session, NULL);

    // In general this macro will ignore all the warning status, the user should implement
    // the logic if he wants to handle the warning case.
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Query the Media SDK version
    sts = session.QueryVersion(&ver);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    printf("\nThe Media SDK API Version: %d.%d\n", ver.Major, ver.Minor);

    // Start calling the video codec API to check the availabilities, the check will only check
    // the error status and ignore all the warning status. This assumes no error is successful
    mfxVideoParam inParam;

    //Check decoder capability
    int len = sizeof(dec_tasks) / sizeof(CodecTask);
    bool success = true;
    char* msg;
    mfxPluginUID codecUID;
    //3. The loop on each registered decoder task.
    for (int idx = 0; idx < len; idx++) {
        success = true;
        //3.1 Fill the decoder video parameters.
        msg = dec_tasks[idx].fillparam_func(&inParam, NULL);

        printf("\nCheck %s\n", msg);

        //3.2 For the decoders from the plugin, get the UID and load the plugin
        if (dec_tasks[idx].load_plugin) {
            codecUID = msdkGetPluginUID(MFX_IMPL_HARDWARE, MSDK_VDECODE, inParam.mfx.CodecId);
            if (AreGuidsEqual(codecUID, MSDK_PLUGINGUID_NULL)) {
                PRINT_ERROR("Get Plugin UID for %s is failed.\n", msg);
                success = false;
            }

            //On the success of get the UID, load the plugin
            if (success) {
                sts = MFXVideoUSER_Load(session, &codecUID, ver.Major);
                if (sts < MFX_ERR_NONE) {
                    PRINT_ERROR("Loading %s plugin failed\n", msg);
                    success = false;
                }
            }
        }

        //3.3 Start query and initialize the decoder to check the availability
        if (success) {
            sts = MFXVideoDECODE_Query(session, &inParam, &inParam);

            if (sts < MFX_ERR_NONE) printf("Query %s failed\n", msg);

            sts = MFXVideoDECODE_Init(session, &inParam);
            if (sts < MFX_ERR_NONE) {
                PRINT_ERROR("The device doesn't support %s, initialization failed\n", msg);
                success = false;
            }
        }

        //3.4 Close the decoder, unload the plugin and report.
        sts = MFXVideoDECODE_Close(session);
        if (sts < MFX_ERR_NONE) printf("The decoder wasn't closed successfully.\n");

        if (dec_tasks[idx].load_plugin) sts = MFXVideoUSER_UnLoad(session, (const mfxPluginUID *)&codecUID);

        if (success) printf("The device supports %s\n", msg);
    }

    //Following variables are used for the optional encoder parameters.
    mfxExtCodingOption2 extCodingOption2;
    std::vector<mfxExtBuffer *> videoExtParam;
    videoExtParam.push_back((mfxExtBuffer*)&extCodingOption2);

    //4. Check the encoder capability
    len = sizeof(enc_tasks) / sizeof(CodecTask);
    for (int idx = 0; idx < len; idx++) {
        success = true;
        //4.1 Fill the encoder video parameters
        msg = enc_tasks[idx].fillparam_func(&inParam, &extCodingOption2);

        //Inject the extCodingOption for the optional parameters,
        //the parameter function will fill it if needed.
        if (enc_tasks[idx].attach_options) {
            inParam.ExtParam = &videoExtParam[0];
            inParam.NumExtParam = videoExtParam.size();
        }

        printf("\nCheck %s\n", msg);

        //4.2 For the encoders from the plugin, get the UID and load the plugin
        if (enc_tasks[idx].load_plugin) {
            codecUID = msdkGetPluginUID(MFX_IMPL_HARDWARE, MSDK_VENCODE, inParam.mfx.CodecId);
            if (AreGuidsEqual(codecUID, MSDK_PLUGINGUID_NULL)) {
                PRINT_ERROR("Get Plugin UID for %s is failed.\n", msg);
                success = false;
            }

            // On the success of get the UID, load the plugin
            if (success) {
                sts = MFXVideoUSER_Load(session, &codecUID, ver.Major);
                if (sts < MFX_ERR_NONE) {
                    PRINT_ERROR("Loading %s plugin failed\n", msg);
                    success = false;
                }
            }
        }

        //4.3 Start query and initialize the encoder to check the availability
        if (success) {
            sts = MFXVideoENCODE_Query(session, &inParam, &inParam);

            if (sts < MFX_ERR_NONE) printf("Query failed\n");

            sts = MFXVideoENCODE_Init(session, &inParam);
            if (sts < MFX_ERR_NONE) {
                PRINT_ERROR("The device doesn't support %s, initialization failed\n", msg);
                success = false;
            }
            MFXVideoENCODE_GetVideoParam(session, &inParam);
        }

        //4.4 Close the encoder, unload the plugin and report
        sts = MFXVideoENCODE_Close(session);
        if (sts < MFX_ERR_NONE) printf("The encoder wasn't closed successfully.\n");

        if (enc_tasks[idx].load_plugin) sts = MFXVideoUSER_UnLoad(session, (const mfxPluginUID *)&codecUID);

        if (success) printf("The device supports %s\n", msg);
    }

    //Check the VPP support
    mfxVideoParam inVPPParams, outVPPParams;
    msg = fillVPPParams(&inVPPParams, NULL);
    memset(&outVPPParams, 0, sizeof(outVPPParams));

    sts = MFXVideoVPP_Query(session, &inVPPParams, &outVPPParams);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    sts = MFXVideoVPP_Init(session, &inVPPParams);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    sts = MFXVideoVPP_Close(session);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    printf("\nThe device supports the %s\n", msg);

    session.Close();

    return 0;
}
