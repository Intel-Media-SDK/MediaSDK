/******************************************************************************\
Copyright (c) 2005-2018, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This sample was distributed or derived from the Intel's Media Samples package.
The original version of this sample may be obtained from https://software.intel.com/en-us/intel-media-server-studio
or https://software.intel.com/en-us/media-client-solutions-support.
\**********************************************************************************/

#include "mfx_samples_config.h"

#include <memory>
#include "pipeline_encode.h"
#include "pipeline_user.h"
#include "pipeline_region_encode.h"
#include <stdarg.h>
#include <string>
#include "version.h"

#define VAL_CHECK(val, argIdx, argName) \
{ \
    if (val) \
    { \
    PrintHelp(NULL, MSDK_STRING("Input argument number %d \"%s\" require more parameters"), argIdx, argName); \
    return MFX_ERR_UNSUPPORTED;\
    } \
}


#ifndef MFX_VERSION
#error MFX_VERSION not defined
#endif

void PrintHelp(msdk_char *strAppName, const msdk_char *strErrorMessage, ...)
{
    msdk_printf(MSDK_STRING("Encoding Sample Version %s\n\n"), GetMSDKSampleVersion().c_str());

    if (strErrorMessage)
    {
        va_list args;
        msdk_printf(MSDK_STRING("ERROR: "));
        va_start(args, strErrorMessage);
        msdk_vprintf(strErrorMessage, args);
        va_end(args);
        msdk_printf(MSDK_STRING("\n\n"));
    }

    msdk_printf(MSDK_STRING("Usage: %s <msdk-codecid> [<options>] -i InputYUVFile -o OutputEncodedFile -w width -h height\n"), strAppName);
    msdk_printf(MSDK_STRING("\n"));
    msdk_printf(MSDK_STRING("Supported codecs, <msdk-codecid>:\n"));
    msdk_printf(MSDK_STRING("   <codecid>=h264|mpeg2|vc1|mvc|jpeg - built-in Media SDK codecs\n"));
    msdk_printf(MSDK_STRING("   <codecid>=h265|vp9                - in-box Media SDK plugins (may require separate downloading and installation)\n"));
    msdk_printf(MSDK_STRING("   If codecid is jpeg, -q option is mandatory.)\n"));
    msdk_printf(MSDK_STRING("Options: \n"));
    msdk_printf(MSDK_STRING("   [-nv12|yuy2|ayuv|rgb4|p010|y210|y410|a2rgb10] - input color format (by default YUV420 is expected).\n"));
    msdk_printf(MSDK_STRING("   [-msb10] - 10-bit color format is expected to have data in Most Significant Bits of words.\n                 (LSB data placement is expected by default).\n                 This option also disables data shifting during file reading.\n"));
    msdk_printf(MSDK_STRING("   [-ec::p010] - force usage of P010 surfaces for encoder (conversion will be made if necessary). Use for 10 bit HEVC encoding\n"));
    msdk_printf(MSDK_STRING("   [-tff|bff] - input stream is interlaced, top|bottom fielf first, if not specified progressive is expected\n"));
    msdk_printf(MSDK_STRING("   [-bref] - arrange B frames in B pyramid reference structure\n"));
    msdk_printf(MSDK_STRING("   [-nobref] -  do not use B-pyramid (by default the decision is made by library)\n"));
    msdk_printf(MSDK_STRING("   [-idr_interval size] - idr interval, default 0 means every I is an IDR, 1 means every other I frame is an IDR etc\n"));
    msdk_printf(MSDK_STRING("   [-f frameRate] - video frame rate (frames per second)\n"));
    msdk_printf(MSDK_STRING("   [-n number] - number of frames to process\n"));
    msdk_printf(MSDK_STRING("   [-b bitRate] - encoded bit rate (Kbits per second), valid for H.264, H.265, MPEG2 and MVC encoders \n"));
    msdk_printf(MSDK_STRING("   [-u usage] - target usage, valid for H.265, H.264, H.265, MPEG2 and MVC encoders. Expected values:\n"));
    msdk_printf(MSDK_STRING("                veryslow(quality), slower, slow, medium(balanced), fast, faster, veryfast(speed)\n"));
    msdk_printf(MSDK_STRING("   [-q quality] - mandatory quality parameter for JPEG encoder. In range [1,100]. 100 is the best quality. \n"));
    msdk_printf(MSDK_STRING("   [-r distance] - Distance between I- or P- key frames (1 means no B-frames) \n"));
    msdk_printf(MSDK_STRING("   [-g size] - GOP size (default 256)\n"));
    msdk_printf(MSDK_STRING("   [-x numRefs]   - number of reference frames\n"));
    msdk_printf(MSDK_STRING("   [-num_active_P numRefs]   - number of maximum allowed references for P frames (for HEVC only)\n"));
    msdk_printf(MSDK_STRING("   [-num_active_BL0 numRefs] - number of maximum allowed references for B frames in L0 (for HEVC only)\n"));
    msdk_printf(MSDK_STRING("   [-num_active_BL1 numRefs] - number of maximum allowed references for B frames in L1 (for HEVC only)\n"));
    msdk_printf(MSDK_STRING("   [-la] - use the look ahead bitrate control algorithm (LA BRC) (by default constant bitrate control method is used)\n"));
    msdk_printf(MSDK_STRING("           for H.264, H.265 encoder. Supported only with -hw option on 4th Generation Intel Core processors. \n"));
    msdk_printf(MSDK_STRING("   [-lad depth] - depth parameter for the LA BRC, the number of frames to be analyzed before encoding. In range [10,100].\n"));
    msdk_printf(MSDK_STRING("            may be 1 in the case when -mss option is specified \n"));
    msdk_printf(MSDK_STRING("   [-dstw width] - destination picture width, invokes VPP resizing\n"));
    msdk_printf(MSDK_STRING("   [-dsth height] - destination picture height, invokes VPP resizing\n"));
    msdk_printf(MSDK_STRING("   [-hw] - use platform specific SDK implementation (default)\n"));
    msdk_printf(MSDK_STRING("   [-sw] - use software implementation, if not specified platform specific SDK implementation is used\n"));
    msdk_printf(MSDK_STRING("   [-p plugin] - encoder plugin. Supported values: hevce_sw, hevce_gacc, hevce_hw, vp8e_hw, vp9e_hw, hevce_fei_hw\n"));
    msdk_printf(MSDK_STRING("                              (optional for Media SDK in-box plugins, required for user-encoder ones)\n"));
    msdk_printf(MSDK_STRING("   [-path path] - path to plugin (valid only in pair with -p option)\n"));
    msdk_printf(MSDK_STRING("   [-async]                 - depth of asynchronous pipeline. default value is 4. must be between 1 and 20.\n"));
    msdk_printf(MSDK_STRING("   [-gpucopy::<on,off>] Enable or disable GPU copy mode\n"));
    msdk_printf(MSDK_STRING("   [-vbr]                   - variable bitrate control\n"));
    msdk_printf(MSDK_STRING("   [-cbr]                   - constant bitrate control\n"));
    msdk_printf(MSDK_STRING("   [-qvbr quality]          - variable bitrate control algorithm with constant quality. Quality in range [1,51]. 1 is the best quality.\n"));
    msdk_printf(MSDK_STRING("   [-cqp]                   - constant quantization parameter (CQP BRC) bitrate control method\n"));
    msdk_printf(MSDK_STRING("                              (by default constant bitrate control method is used), should be used along with -qpi, -qpp, -qpb.\n"));
    msdk_printf(MSDK_STRING("   [-qpi]                   - constant quantizer for I frames (if bitrace control method is CQP). In range [1,51]. 0 by default, i.e.no limitations on QP.\n"));
    msdk_printf(MSDK_STRING("   [-qpp]                   - constant quantizer for P frames (if bitrace control method is CQP). In range [1,51]. 0 by default, i.e.no limitations on QP.\n"));
    msdk_printf(MSDK_STRING("   [-qpb]                   - constant quantizer for B frames (if bitrace control method is CQP). In range [1,51]. 0 by default, i.e.no limitations on QP.\n"));
    msdk_printf(MSDK_STRING("   [-round_offset_in file]  - use this file to set per frame inter/intra rounding offset(for AVC only)\n"));
    msdk_printf(MSDK_STRING("   [-qsv-ff]       Enable QSV-FF mode\n"));
    msdk_printf(MSDK_STRING("   [-ir_type]               - Intra refresh type. 0 - no refresh, 1 - vertical refresh, 2 - horisontal refresh, 3 - slice refresh\n"));
    msdk_printf(MSDK_STRING("   [-ir_cycle_size]         - Number of pictures within refresh cycle starting from 2\n"));
    msdk_printf(MSDK_STRING("   [-ir_qp_delta]           - QP difference for inserted intra MBs. This is signed value in [-51, 51] range\n"));
    msdk_printf(MSDK_STRING("   [-ir_cycle_dist]         - Distance between the beginnings of the intra-refresh cycles in frames\n"));
    msdk_printf(MSDK_STRING("   [-gpb:<on,off>]          - Turn this option OFF to make HEVC encoder use regular P-frames instead of GPB\n"));
    msdk_printf(MSDK_STRING("   [-ppyr:<on,off>]         - Turn this option ON to enable P-pyramid (by default the decision is made by library)\n"));
    msdk_printf(MSDK_STRING("   [-num_slice]             - number of slices in each video frame. 0 by default.\n"));
    msdk_printf(MSDK_STRING("                              If num_slice equals zero, the encoder may choose any slice partitioning allowed by the codec standard.\n"));
    msdk_printf(MSDK_STRING("   [-mss]                   - maximum slice size in bytes. Supported only with -hw and h264 codec. This option is not compatible with -num_slice option.\n"));
    msdk_printf(MSDK_STRING("   [-mfs]                   - maximum frame size in bytes. Supported only with h264 and hevc codec for VBR mode.\n"));
    msdk_printf(MSDK_STRING("   [-re]                    - enable region encode mode. Works only with h265 encoder\n"));
    msdk_printf(MSDK_STRING("   [-CodecProfile]          - specifies codec profile\n"));
    msdk_printf(MSDK_STRING("   [-CodecLevel]            - specifies codec level\n"));
    msdk_printf(MSDK_STRING("   [-GopOptFlag:closed]     - closed gop\n"));
    msdk_printf(MSDK_STRING("   [-GopOptFlag:strict]     - strict gop\n"));
    msdk_printf(MSDK_STRING("   [-InitialDelayInKB]      - the decoder starts decoding after the buffer reaches the initial size InitialDelayInKB, \
                            which is equivalent to reaching an initial delay of InitialDelayInKB*8000/TargetKbps ms\n"));
    msdk_printf(MSDK_STRING("   [-BufferSizeInKB ]       - represents the maximum possible size of any compressed frames\n"));
    msdk_printf(MSDK_STRING("   [-MaxKbps ]              - for variable bitrate control, specifies the maximum bitrate at which \
                            the encoded data enters the Video Buffering Verifier buffer\n"));
    msdk_printf(MSDK_STRING("   [-amfs:<on,off>]         - adaptive max frame size. If set on, P or B frame size can exceed MaxFrameSize when the scene change is detected.\
                            It can benefit the video quality \n"));
#if (MFX_VERSION >= 1023)
    msdk_printf(MSDK_STRING("   [-LowDelayBRC]           - strictly obey average frame size set by MaxKbps\n"));
#endif
    msdk_printf(MSDK_STRING("   [-signal:tm ]            - represents transfer matrix coefficients for mfxExtVideoSignalInfo. 0 - unknown, 1 - BT709, 2 - BT601\n"));
    msdk_printf(MSDK_STRING("   [-WeightedPred:default|implicit ]   - enables weighted prediction mode\n"));
    msdk_printf(MSDK_STRING("   [-WeightedBiPred:default|implicit ] - enables weighted bi-prediction mode\n"));
    msdk_printf(MSDK_STRING("   [-timeout]               - encoding in cycle not less than specific time in seconds\n"));
    msdk_printf(MSDK_STRING("   [-membuf]                - size of memory buffer in frames\n"));
    msdk_printf(MSDK_STRING("   [-uncut]                 - do not cut output file in looped mode (in case of -timeout option)\n"));
    msdk_printf(MSDK_STRING("   [-dump fileName]         - dump MSDK components configuration to the file in text form\n"));
    msdk_printf(MSDK_STRING("   [-usei]                  - insert user data unregistered SEI. eg: 7fc92488825d11e7bb31be2e44b06b34:0:MSDK (uuid:type<0-preifx/1-suffix>:message)\n"));
    msdk_printf(MSDK_STRING("                              the suffix SEI for HEVCe can be inserted when CQP used or HRD disabled\n"));
#if (MFX_VERSION >= 1024)
    msdk_printf(MSDK_STRING("   [-extbrc:<on,off,implicit>] - External BRC for AVC and HEVC encoders\n"));
#endif
#if (MFX_VERSION >= 1026)
    msdk_printf(MSDK_STRING("   [-ExtBrcAdaptiveLTR:<on,off>] - Set AdaptiveLTR for implicit extbrc"));
#endif
    msdk_printf(MSDK_STRING("Example: %s h265 -i InputYUVFile -o OutputEncodedFile -w width -h height -hw -p 2fca99749fdb49aeb121a5b63ef568f7\n"), strAppName);
#if D3D_SURFACES_SUPPORT
    msdk_printf(MSDK_STRING("   [-d3d] - work with d3d surfaces\n"));
    msdk_printf(MSDK_STRING("   [-d3d11] - work with d3d11 surfaces\n"));
    msdk_printf(MSDK_STRING("Example: %s h264|h265|mpeg2|jpeg -i InputYUVFile -o OutputEncodedFile -w width -h height -d3d -hw \n"), strAppName);
    msdk_printf(MSDK_STRING("Example for MVC: %s mvc -i InputYUVFile_1 -i InputYUVFile_2 -o OutputEncodedFile -w width -h height \n"), strAppName);
#endif
#ifdef LIBVA_SUPPORT
    msdk_printf(MSDK_STRING("   [-vaapi] - work with vaapi surfaces\n"));
    msdk_printf(MSDK_STRING("Example: %s h264|mpeg2|mvc -i InputYUVFile -o OutputEncodedFile -w width -h height -angle 180 -g 300 -r 1 \n"), strAppName);
#endif
#if defined (ENABLE_V4L2_SUPPORT)
    msdk_printf(MSDK_STRING("   [-d]                            - Device video node (eg: /dev/video0)\n"));
    msdk_printf(MSDK_STRING("   [-p]                            - Mipi Port number (eg: Port 0)\n"));
    msdk_printf(MSDK_STRING("   [-m]                            - Mipi Mode Configuration [PREVIEW/CONTINUOUS/STILL/VIDEO]\n"));
    msdk_printf(MSDK_STRING("   [-uyvy]                        - Input Raw format types V4L2 Encode\n"));
    msdk_printf(MSDK_STRING("   [-YUY2]                        - Input Raw format types V4L2 Encode\n"));
    msdk_printf(MSDK_STRING("   [-i::v4l2]                        - To enable v4l2 option\n"));
    msdk_printf(MSDK_STRING("Example: %s h264|mpeg2|mvc -i::v4l2 -o OutputEncodedFile -w width -h height -d /dev/video0 -uyvy -m preview -p 0\n"), strAppName);
#endif
    msdk_printf(MSDK_STRING("   [-viewoutput] - instruct the MVC encoder to output each view in separate bitstream buffer. Depending on the number of -o options behaves as follows:\n"));
    msdk_printf(MSDK_STRING("                   1: two views are encoded in single file\n"));
    msdk_printf(MSDK_STRING("                   2: two views are encoded in separate files\n"));
    msdk_printf(MSDK_STRING("                   3: behaves like 2 -o opitons was used and then one -o\n\n"));
    msdk_printf(MSDK_STRING("Example: %s mvc -i InputYUVFile_1 -i InputYUVFile_2 -o OutputEncodedFile_1 -o OutputEncodedFile_2 -viewoutput -w width -h height \n"), strAppName);
    // user module options
    msdk_printf(MSDK_STRING("User module options: \n"));
    msdk_printf(MSDK_STRING("   [-angle 180] - enables 180 degrees picture rotation before encoding, CPU implementation by default. Rotation requires NV12 input. Options -tff|bff, -dstw, -dsth, -d3d are not effective together with this one, -nv12 is required.\n"));
    msdk_printf(MSDK_STRING("   [-opencl] - rotation implementation through OPENCL\n"));
    msdk_printf(MSDK_STRING("Example: %s h264|h265|mpeg2|mvc|jpeg -i InputYUVFile -o OutputEncodedFile -w width -h height -angle 180 -opencl \n"), strAppName);

    msdk_printf(MSDK_STRING("\n"));
}

mfxStatus ParseInputString(msdk_char* strInput[], mfxU8 nArgNum, sInputParams* pParams)
{

    if (1 == nArgNum)
    {
        PrintHelp(strInput[0], NULL);
        return MFX_ERR_UNSUPPORTED;
    }

    MSDK_CHECK_POINTER(pParams, MFX_ERR_NULL_PTR);
    msdk_opt_read(MSDK_CPU_ROTATE_PLUGIN, pParams->strPluginDLLPath);

    // default implementation
    pParams->bUseHWLib = true;
    pParams->isV4L2InputEnabled = false;
    pParams->nNumFrames = 0;
    pParams->FileInputFourCC = MFX_FOURCC_I420;
    pParams->EncodeFourCC = MFX_FOURCC_NV12;
    pParams->nPRefType = MFX_P_REF_DEFAULT;
    pParams->RoundingOffsetFile = NULL;
#if defined (ENABLE_V4L2_SUPPORT)
    pParams->MipiPort = -1;
    pParams->MipiMode = NONE;
    pParams->v4l2Format = NO_FORMAT;
#endif

    // parse command line parameters
    for (mfxU8 i = 1; i < nArgNum; i++)
    {
        MSDK_CHECK_POINTER(strInput[i], MFX_ERR_NULL_PTR);

        if (MSDK_CHAR('-') != strInput[i][0])
        {
            mfxStatus sts = StrFormatToCodecFormatFourCC(strInput[i], pParams->CodecId);
            if (sts != MFX_ERR_NONE)
            {
                PrintHelp(strInput[0], MSDK_STRING("Unknown codec"));
                return MFX_ERR_UNSUPPORTED;
            }
            if (!IsEncodeCodecSupported(pParams->CodecId))
            {
                PrintHelp(strInput[0], MSDK_STRING("Unsupported codec"));
                return MFX_ERR_UNSUPPORTED;
            }
            if (pParams->CodecId == CODEC_MVC)
            {
                pParams->CodecId = MFX_CODEC_AVC;
                pParams->MVC_flags |= MVC_ENABLED;
            }
            continue;
        }

        // process multi-character options
        if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-dstw")))
        {
            VAL_CHECK(i+1 >= nArgNum, i, strInput[i]);
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->nDstWidth))
            {
                PrintHelp(strInput[0], MSDK_STRING("Destination picture Width is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-dsth")))
        {
            VAL_CHECK(i+1 >= nArgNum, i, strInput[i]);
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->nDstHeight))
            {
                PrintHelp(strInput[0], MSDK_STRING("Destination picture Height is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-sw")))
        {
            pParams->bUseHWLib = false;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-hw")))
        {
            pParams->bUseHWLib = true;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-yuy2")))
        {
#if defined (ENABLE_V4L2_SUPPORT)
            pParams->v4l2Format = YUY2;
#endif
            pParams->FileInputFourCC = MFX_FOURCC_YUY2;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-nv12")))
        {
            pParams->FileInputFourCC = MFX_FOURCC_NV12;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-rgb4")))
        {
            pParams->FileInputFourCC = MFX_FOURCC_RGB4;
            pParams->EncodeFourCC = MFX_FOURCC_RGB4;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-p010")))
        {
            pParams->FileInputFourCC = MFX_FOURCC_P010;
            pParams->EncodeFourCC = MFX_FOURCC_P010;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-ayuv")))
        {
            pParams->FileInputFourCC = MFX_FOURCC_AYUV;
            pParams->EncodeFourCC = MFX_FOURCC_AYUV;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-yuy2")))
        {
            pParams->FileInputFourCC = MFX_FOURCC_YUY2;
            pParams->EncodeFourCC = MFX_FOURCC_YUY2;
        }
#if (MFX_VERSION >= 1027)
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-y210")))
        {
            pParams->FileInputFourCC = MFX_FOURCC_Y210;
            pParams->EncodeFourCC = MFX_FOURCC_Y210;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-y410")))
        {
            pParams->FileInputFourCC = MFX_FOURCC_Y410;
            pParams->EncodeFourCC = MFX_FOURCC_Y410;
        }
#endif
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-a2rgb10")))
        {
            pParams->FileInputFourCC = MFX_FOURCC_A2RGB10;
            pParams->EncodeFourCC = MFX_FOURCC_A2RGB10;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-ec::p010")))
        {
            pParams->EncodeFourCC = MFX_FOURCC_P010;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-tff")))
        {
            pParams->nPicStruct = MFX_PICSTRUCT_FIELD_TFF;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-bff")))
        {
            pParams->nPicStruct = MFX_PICSTRUCT_FIELD_BFF;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-bref")))
        {
            pParams->nBRefType = MFX_B_REF_PYRAMID;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-nobref")))
        {
            pParams->nBRefType = MFX_B_REF_OFF;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-idr_interval")))
        {
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->nIdrInterval))
            {
                PrintHelp(strInput[0], MSDK_STRING("IdrInterval is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-msb10")))
        {
            pParams->IsSourceMSB = true;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-angle")))
        {
            VAL_CHECK(i+1 >= nArgNum, i, strInput[i]);
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->nRotationAngle))
            {
                PrintHelp(strInput[0], MSDK_STRING("Rotation Angle is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-opencl")))
        {
            msdk_opt_read(MSDK_OCL_ROTATE_PLUGIN, pParams->strPluginDLLPath);
            pParams->nRotationAngle = 180;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-viewoutput")))
        {
            if (!(MVC_ENABLED & pParams->MVC_flags))
            {
                PrintHelp(strInput[0], MSDK_STRING("-viewoutput option is supported only when mvc codec specified"));
                return MFX_ERR_UNSUPPORTED;
            }
            pParams->MVC_flags |= MVC_VIEWOUTPUT;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-la")))
        {
            pParams->nRateControlMethod = MFX_RATECONTROL_LA;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-lad")))
        {
            VAL_CHECK(i+1 >= nArgNum, i, strInput[i]);
            pParams->nRateControlMethod = MFX_RATECONTROL_LA;
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->nLADepth))
            {
                PrintHelp(strInput[0], MSDK_STRING("Look Ahead Depth is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-vbr")))
        {
            pParams->nRateControlMethod = MFX_RATECONTROL_VBR;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-cbr")))
        {
            pParams->nRateControlMethod = MFX_RATECONTROL_CBR;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-qvbr")))
        {
            pParams->nRateControlMethod = MFX_RATECONTROL_QVBR;
            VAL_CHECK(i + 1 >= nArgNum, i, strInput[i]);

            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->QVBRQuality))
            {
                PrintHelp(strInput[0], MSDK_STRING("QVBRQuality param is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-mss")))
        {
            VAL_CHECK(i+1 >= nArgNum, i, strInput[i]);
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->nMaxSliceSize))
            {
                PrintHelp(strInput[0], MSDK_STRING("MaxSliceSize is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-mfs")))
       {
            VAL_CHECK(i + 1 >= nArgNum, i, strInput[i]);
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->nMaxFrameSize))
            {
                PrintHelp(strInput[0], MSDK_STRING("MaxFrameSize is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
       }
#if D3D_SURFACES_SUPPORT
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-d3d")))
        {
            pParams->memType = D3D9_MEMORY;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-d3d11")))
        {
            pParams->memType = D3D11_MEMORY;
        }
#endif
#ifdef LIBVA_SUPPORT
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-vaapi")))
        {
            pParams->memType = D3D9_MEMORY;
        }
#endif
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-async")))
        {
            VAL_CHECK(i+1 >= nArgNum, i, strInput[i]);

            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->nAsyncDepth))
            {
                PrintHelp(strInput[0], MSDK_STRING("Async Depth is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-CodecLevel")))
        {
            VAL_CHECK(i+1 >= nArgNum, i, strInput[i]);
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->CodecLevel))
            {
                PrintHelp(strInput[0], MSDK_STRING("CodecLevel is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-CodecProfile")))
        {
            VAL_CHECK(i+1 >= nArgNum, i, strInput[i]);
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->CodecProfile))
            {
                PrintHelp(strInput[0], MSDK_STRING("CodecProfile is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-GopOptFlag:closed")))
        {
            pParams->GopOptFlag = MFX_GOP_CLOSED;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-GopOptFlag:strict")))
        {
            pParams->GopOptFlag = MFX_GOP_STRICT;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-InitialDelayInKB")))
        {
            VAL_CHECK(i+1 >= nArgNum, i, strInput[i]);
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->InitialDelayInKB))
            {
                PrintHelp(strInput[0], MSDK_STRING("InitialDelayInKB is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-MaxKbps")))
        {
            VAL_CHECK(i+1 >= nArgNum, i, strInput[i]);
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->MaxKbps))
            {
                PrintHelp(strInput[0], MSDK_STRING("MaxKbps is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-LowDelayBRC")))
        {
            pParams->LowDelayBRC = MFX_CODINGOPTION_ON;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-BufferSizeInKB")))
        {
            VAL_CHECK(i+1 >= nArgNum, i, strInput[i]);
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->BufferSizeInKB))
            {
                PrintHelp(strInput[0], MSDK_STRING("BufferSizeInKB is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-timeout")))
        {
            VAL_CHECK(i+1 >= nArgNum, i, strInput[i]);

            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->nTimeout))
            {
                PrintHelp(strInput[0], MSDK_STRING("Timeout is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-signal:tm")))
        {
            VAL_CHECK(i + 1 >= nArgNum, i, strInput[i]);

            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->TransferMatrix))
            {
                PrintHelp(strInput[0], MSDK_STRING("Transfer matrix param is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-ir_type")))
        {
            VAL_CHECK(i + 1 >= nArgNum, i, strInput[i]);

            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->IntRefType))
            {
                PrintHelp(strInput[0], MSDK_STRING("Intra refresh type is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-ir_cycle_size")))
        {
            VAL_CHECK(i + 1 >= nArgNum, i, strInput[i]);

            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->IntRefCycleSize))
            {
                PrintHelp(strInput[0], MSDK_STRING("IR refresh cycle size param is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-ir_qp_delta")))
        {
            VAL_CHECK(i + 1 >= nArgNum, i, strInput[i]);

            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->IntRefQPDelta))
            {
                PrintHelp(strInput[0], MSDK_STRING("IR QP delta param is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-ir_cycle_dist")))
        {
            VAL_CHECK(i + 1 >= nArgNum, i, strInput[i]);

            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->IntRefCycleDist))
            {
                PrintHelp(strInput[0], MSDK_STRING("IR cycle distance param is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-membuf")))
        {
            VAL_CHECK(i+1 >= nArgNum, i, strInput[i]);

            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->nMemBuf))
            {
                PrintHelp(strInput[0], MSDK_STRING("membuf is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-WeightedPred:default")))
        {
            pParams->WeightedPred = MFX_WEIGHTED_PRED_DEFAULT;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-WeightedPred:implicit")))
        {
            pParams->WeightedPred = MFX_WEIGHTED_PRED_IMPLICIT;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-WeightedBiPred:default")))
        {
            pParams->WeightedBiPred = MFX_WEIGHTED_PRED_DEFAULT;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-WeightedBiPred:implicit")))
        {
            pParams->WeightedBiPred = MFX_WEIGHTED_PRED_IMPLICIT;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-uncut")))
        {
            pParams->bUncut = true;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-gpucopy::on")))
        {
            pParams->gpuCopy = MFX_GPUCOPY_ON;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-gpucopy::off")))
        {
            pParams->gpuCopy = MFX_GPUCOPY_OFF;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-dump")))
        {
            VAL_CHECK(i+1 >= nArgNum, i, strInput[i]);
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->DumpFileName))
            {
                PrintHelp(strInput[0], MSDK_STRING("File Name for dumping MSDK library configuration should be provided"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-usei")))
        {
            VAL_CHECK(i+1 >= nArgNum, i, strInput[i]);
            if (msdk_strlen(strInput[i + 1]) > MSDK_MAX_USER_DATA_UNREG_SEI_LEN)
            {
                PrintHelp(strInput[0], MSDK_STRING("error: '-usei' arguments is too long\n"));
                return MFX_ERR_UNSUPPORTED;
            }
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->uSEI))
            {
                PrintHelp(strInput[0], MSDK_STRING("error: option '-usei' expects arguments\n"));
                return MFX_ERR_UNSUPPORTED;
            }
            if (msdk_strlen(pParams->uSEI) < 32)
            {
                PrintHelp(strInput[0], MSDK_STRING("error: option '-usei' expects at least 32 char uuid\n"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-cqp")))
        {
            pParams->nRateControlMethod = MFX_RATECONTROL_CQP;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-qpi")))
        {
            VAL_CHECK(i+1 >= nArgNum, i, strInput[i]);
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->nQPI))
            {
                PrintHelp(strInput[0], MSDK_STRING("Quantizer for I frames is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-qpp")))
        {
            VAL_CHECK(i+1 >= nArgNum, i, strInput[i]);
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->nQPP))
            {
                PrintHelp(strInput[0], MSDK_STRING("Quantizer for P frames is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-qpb")))
        {
            VAL_CHECK(i+1 >= nArgNum, i, strInput[i]);
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->nQPB))
            {
                PrintHelp(strInput[0], MSDK_STRING("Quantizer for B frames is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-round_offset_in")))
        {
            VAL_CHECK(i+1 >= nArgNum, i, strInput[i]);
            pParams->RoundingOffsetFile = strInput[++i];
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-gpb:on")))
        {
            pParams->nGPB = MFX_CODINGOPTION_ON;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-gpb:off")))
        {
            pParams->nGPB = MFX_CODINGOPTION_OFF;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-ppyr:on")))
        {
            pParams->nPRefType = MFX_P_REF_PYRAMID;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-ppyr:off")))
        {
            pParams->nPRefType = MFX_P_REF_SIMPLE;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-num_active_P")))
        {
            VAL_CHECK(i+1 >= nArgNum, i, strInput[i]);
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->nNumRefActiveP))
            {
                PrintHelp(strInput[0], MSDK_STRING("Number of active reference for P frames is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-num_active_BL0")))
        {
            VAL_CHECK(i+1 >= nArgNum, i, strInput[i]);
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->nNumRefActiveBL0))
            {
                PrintHelp(strInput[0], MSDK_STRING("Number of active reference for B frames (L0) is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-num_active_BL1")))
        {
            VAL_CHECK(i+1 >= nArgNum, i, strInput[i]);
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->nNumRefActiveBL1))
            {
                PrintHelp(strInput[0], MSDK_STRING("Number of active reference for B frames (L1) is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
#if (MFX_VERSION >= 1024)
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-extbrc:on")))
        {
            pParams->nExtBRC= EXTBRC_ON;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-extbrc:off")))
        {
            pParams->nExtBRC = EXTBRC_OFF;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-extbrc:implicit")))
        {
            pParams->nExtBRC = EXTBRC_IMPLICIT;
        }
#endif
#if (MFX_VERSION >= 1026)
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-ExtBrcAdaptiveLTR:on")))
        {
            pParams->ExtBrcAdaptiveLTR = MFX_CODINGOPTION_ON;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-ExtBrcAdaptiveLTR:off")))
        {
            pParams->ExtBrcAdaptiveLTR = MFX_CODINGOPTION_OFF;;
        }
#endif
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-amfs:on")))
        {
            pParams->nAdaptiveMaxFrameSize = MFX_CODINGOPTION_ON;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-amfs:off")))
        {
            pParams->nAdaptiveMaxFrameSize = MFX_CODINGOPTION_OFF;
        }

        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-qsv-ff")))
        {
            pParams->enableQSVFF=true;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-num_slice")))
        {
            VAL_CHECK(i+1 >= nArgNum, i, strInput[i]);
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->nNumSlice))
            {
                PrintHelp(strInput[0], MSDK_STRING("Number of slices is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-n")))
        {

            VAL_CHECK(i + 1 >= nArgNum, i, strInput[i]);
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->nNumFrames))
            {
                PrintHelp(strInput[0], MSDK_STRING("Number of frames is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-w")))
        {

            VAL_CHECK(i + 1 >= nArgNum, i, strInput[i]);
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->nWidth))
            {
                PrintHelp(strInput[0], MSDK_STRING("Width is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-h")))
        {

            VAL_CHECK(i + 1 >= nArgNum, i, strInput[i]);
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->nHeight))
            {
                PrintHelp(strInput[0], MSDK_STRING("Height of frames is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-b")))
        {

            VAL_CHECK(i + 1 >= nArgNum, i, strInput[i]);
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->nBitRate))
            {
                PrintHelp(strInput[0], MSDK_STRING("Bitrate is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-f")))
        {

            VAL_CHECK(i + 1 >= nArgNum, i, strInput[i]);
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->dFrameRate))
            {
                PrintHelp(strInput[0], MSDK_STRING("Framerate is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-x")))
        {

            VAL_CHECK(i + 1 >= nArgNum, i, strInput[i]);
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->nNumRefFrame))
            {
                PrintHelp(strInput[0], MSDK_STRING("NumRefFrames is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-g")))
        {

            VAL_CHECK(i + 1 >= nArgNum, i, strInput[i]);
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->nGopPicSize))
            {
                PrintHelp(strInput[0], MSDK_STRING("GopSize is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-r")))
        {
            VAL_CHECK(i + 1 >= nArgNum, i, strInput[i]);
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->nGopRefDist))
            {
                PrintHelp(strInput[0], MSDK_STRING("GopRefDist is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-i")))
        {
            VAL_CHECK(i + 1 >= nArgNum, i, strInput[i]);
            pParams->InputFiles.push_back(strInput[++i]);

        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-o")))
        {
            VAL_CHECK(i + 1 >= nArgNum, i, strInput[i]);
            pParams->dstFileBuff.push_back(strInput[++i]);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-p")))
        {
            pParams->pluginParams = ParsePluginGuid(strInput[++i]);
            if (AreGuidsEqual(pParams->pluginParams.pluginGuid, MSDK_PLUGINGUID_NULL))
            {
                msdk_printf(MSDK_STRING("error:  invalid encoder guid\n"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-q")))
        {
            VAL_CHECK(i + 1 >= nArgNum, i, strInput[i]);
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->nQuality))
            {
                PrintHelp(strInput[0], MSDK_STRING("Quality is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-?")))
        {
            PrintHelp(strInput[0], NULL);
            return MFX_ERR_UNSUPPORTED;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-path")))
        {
            i++;
            pParams->pluginParams = ParsePluginPath(strInput[i]);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-re")))
        {
            pParams->UseRegionEncode = true;
        }
#if defined (ENABLE_V4L2_SUPPORT)
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-d")))
        {
            VAL_CHECK(i+1 >= nArgNum, i, strInput[i]);
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->DeviceName))
            {
                PrintHelp(strInput[0], MSDK_STRING("Device name is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-uyvy")))
        {
            pParams->v4l2Format = UYVY;

        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-p")))
        {
            VAL_CHECK(i+1 >= nArgNum, i, strInput[i]);
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->MipiPort))
            {
                PrintHelp(strInput[0], MSDK_STRING("Mipi-port is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-m")))
        {
            VAL_CHECK(i+1 >= nArgNum, i, strInput[i]);
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->MipiModeName))
            {
                PrintHelp(strInput[0], MSDK_STRING("Device name is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }

            if(strcasecmp(pParams->MipiModeName,"STILL") == 0)
                pParams->MipiMode = STILL;
            else if(strcasecmp(pParams->MipiModeName,"VIDEO") == 0)
                pParams->MipiMode = VIDEO;
            else if(strcasecmp(pParams->MipiModeName,"PREVIEW") == 0)
                pParams->MipiMode = PREVIEW;
            else if(strcasecmp(pParams->MipiModeName,"CONTINUOUS") == 0)
                pParams->MipiMode = CONTINUOUS;
            else
                pParams->MipiMode = NONE;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-i::v4l2")))
        {
            pParams->isV4L2InputEnabled = true;
        }
#endif
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-u")))
        {
            VAL_CHECK(i + 1 >= nArgNum, i, strInput[i]);
            pParams->nTargetUsage = StrToTargetUsage(strInput[++i]);
            if (!pParams->nTargetUsage)
            {
                msdk_printf(MSDK_STRING("error: wrong '-u' parameter. Balanced will be used.\n"));
                pParams->nTargetUsage = MFX_TARGETUSAGE_BALANCED;
            }
        }
        else
        {
            msdk_printf(MSDK_STRING("Unknown option: %s\n"), strInput[i]);
            PrintHelp(strInput[0], NULL);
            return MFX_ERR_UNSUPPORTED;
        }
    }

#if defined (ENABLE_V4L2_SUPPORT)
    if (pParams->isV4L2InputEnabled)
    {
        if (0 == msdk_strlen(pParams->DeviceName))
        {
            PrintHelp(strInput[0], MSDK_STRING("Device Name not found"));
            return MFX_ERR_UNSUPPORTED;
        }

        if ((pParams->MipiPort > -1 && pParams->MipiMode == NONE) ||
            (pParams->MipiPort < 0 && pParams->MipiMode != NONE))
        {
            PrintHelp(strInput[0], MSDK_STRING("Invalid Mipi Configuration\n"));
            return MFX_ERR_UNSUPPORTED;
        }

        if (pParams->v4l2Format == NO_FORMAT)
        {
            PrintHelp(strInput[0], MSDK_STRING("NO input v4l2 format\n"));
            return MFX_ERR_UNSUPPORTED;
        }
    }
#endif

    // check if all mandatory parameters were set
    if (!pParams->InputFiles.size() && !pParams->isV4L2InputEnabled)
    {
        PrintHelp(strInput[0], MSDK_STRING("Source file name not found"));
        return MFX_ERR_UNSUPPORTED;
    };


    if (0 == pParams->nWidth || 0 == pParams->nHeight)
    {
        PrintHelp(strInput[0], MSDK_STRING("-w, -h must be specified"));
        return MFX_ERR_UNSUPPORTED;
    }

    if (MFX_CODEC_MPEG2 != pParams->CodecId &&
        MFX_CODEC_AVC != pParams->CodecId &&
        MFX_CODEC_JPEG != pParams->CodecId &&
        MFX_CODEC_HEVC != pParams->CodecId && 
        MFX_CODEC_VP9 != pParams->CodecId)
    {
        PrintHelp(strInput[0], MSDK_STRING("Unknown codec"));
        return MFX_ERR_UNSUPPORTED;
    }

    if (MFX_CODEC_JPEG != pParams->CodecId && MFX_CODEC_HEVC != pParams->CodecId &&
        pParams->FileInputFourCC == MFX_FOURCC_YUY2 &&
        !pParams->isV4L2InputEnabled)
    {
        PrintHelp(strInput[0], MSDK_STRING("-yuy2 option is supported only for JPEG or HEVC encoder"));
        return MFX_ERR_UNSUPPORTED;
    }

    if (MFX_CODEC_HEVC != pParams->CodecId && MFX_CODEC_VP9 != pParams->CodecId && (pParams->EncodeFourCC == MFX_FOURCC_P010) )
    {
        PrintHelp(strInput[0], MSDK_STRING("P010 surfaces are supported only for HEVC and VP9 encoder"));
        return MFX_ERR_UNSUPPORTED;
    }

    // check parameters validity
    if (pParams->nRotationAngle != 0 && pParams->nRotationAngle != 180)
    {
        PrintHelp(strInput[0], MSDK_STRING("Angles other than 180 degrees are not supported."));
        return MFX_ERR_UNSUPPORTED; // other than 180 are not supported
    }

    if (pParams->nQuality && (MFX_CODEC_JPEG != pParams->CodecId))
    {
        PrintHelp(strInput[0], MSDK_STRING("-q option is supported only for JPEG encoder"));
        return MFX_ERR_UNSUPPORTED;
    }

    if ((pParams->nTargetUsage || pParams->nBitRate) && (MFX_CODEC_JPEG == pParams->CodecId))
    {
        PrintHelp(strInput[0], MSDK_STRING("-u and -b options are supported only for H.264, MPEG2 and MVC encoders. For JPEG encoder use -q"));
        return MFX_ERR_UNSUPPORTED;
    }

    if (!pParams->nQuality && (MFX_CODEC_JPEG == pParams->CodecId))
    {
        PrintHelp(strInput[0], MSDK_STRING("-q must be specified for JPEG encoder"));
        return MFX_ERR_UNSUPPORTED;
    }

    if (MFX_TRANSFERMATRIX_UNKNOWN != pParams->TransferMatrix &&
        MFX_TRANSFERMATRIX_BT601 != pParams->TransferMatrix &&
        MFX_TRANSFERMATRIX_BT709 != pParams->TransferMatrix)
    {
        PrintHelp(strInput[0], MSDK_STRING("Invalid transfer matrix type"));
        return MFX_ERR_UNSUPPORTED;
    }

    // set default values for optional parameters that were not set or were set incorrectly
    mfxU32 nviews = (mfxU32)pParams->InputFiles.size();
    if ((nviews <= 1) || (nviews > 2))
    {
        if (!(MVC_ENABLED & pParams->MVC_flags))
        {
            pParams->numViews = 1;
        }
        else
        {
            PrintHelp(strInput[0], MSDK_STRING("Only 2 views are supported right now in this sample."));
            return MFX_ERR_UNSUPPORTED;
        }
    }
    else
    {
        pParams->numViews = nviews;
    }

    if (MFX_TARGETUSAGE_BEST_QUALITY != pParams->nTargetUsage && MFX_TARGETUSAGE_BEST_SPEED != pParams->nTargetUsage)
    {
        pParams->nTargetUsage = MFX_TARGETUSAGE_BALANCED;
    }

    if (pParams->dFrameRate <= 0)
    {
        pParams->dFrameRate = 30;
    }

    // if no destination picture width or height wasn't specified set it to the source picture size
    if (pParams->nDstWidth == 0)
    {
        pParams->nDstWidth = pParams->nWidth;
    }

    if (pParams->nDstHeight == 0)
    {
        pParams->nDstHeight = pParams->nHeight;
    }

    // calculate default bitrate based on the resolution (a parameter for encoder, so Dst resolution is used)
    if (pParams->nBitRate == 0)
    {
        pParams->nBitRate = CalculateDefaultBitrate(pParams->CodecId, pParams->nTargetUsage, pParams->nDstWidth,
            pParams->nDstHeight, pParams->dFrameRate);
    }

    if (!pParams->nPicStruct)
    {
        pParams->nPicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    }

    if ((pParams->nRateControlMethod == MFX_RATECONTROL_LA) && (!pParams->bUseHWLib))
    {
        PrintHelp(strInput[0], MSDK_STRING("Look ahead BRC is supported only with -hw option!"));
        return MFX_ERR_UNSUPPORTED;
    }

    if ((pParams->nMaxSliceSize) && (!pParams->bUseHWLib))
    {
        PrintHelp(strInput[0], MSDK_STRING("MaxSliceSize option is supported only with -hw option!"));
        return MFX_ERR_UNSUPPORTED;
    }

    if ((pParams->nMaxSliceSize) && (pParams->nNumSlice))
    {
        PrintHelp(strInput[0], MSDK_STRING("-mss and -num_slice options are not compatible!"));
        return MFX_ERR_UNSUPPORTED;
    }

    //if ((pParams->nRateControlMethod == MFX_RATECONTROL_LA) && (pParams->CodecId != MFX_CODEC_AVC))
    //{
    //    PrintHelp(strInput[0], MSDK_STRING("Look ahead BRC is supported only with H.264 encoder!"));
    //    return MFX_ERR_UNSUPPORTED;
    //}

    if ((pParams->nMaxSliceSize) && (pParams->CodecId != MFX_CODEC_AVC))
    {
        PrintHelp(strInput[0], MSDK_STRING("MaxSliceSize option is supported only with H.264 encoder!"));
        return MFX_ERR_UNSUPPORTED;
    }

    if (pParams->nLADepth && (pParams->nLADepth < 10))
    {
        if ((pParams->nLADepth != 1) || (!pParams->nMaxSliceSize))
        {
            PrintHelp(strInput[0], MSDK_STRING("Unsupported value of -lad parameter, must be >= 10, or 1 in case of -mss option!"));
            return MFX_ERR_UNSUPPORTED;
        }
    }

    if (pParams->nNumRefActiveP || pParams->nNumRefActiveBL0 || pParams->nNumRefActiveBL1)
    {
        if (pParams->CodecId != MFX_CODEC_HEVC)
        {
            PrintHelp(strInput[0], MSDK_STRING("NumRefActiveP/BL0/BL1 are supported only with HEVC encoder"));
            return MFX_ERR_UNSUPPORTED;
        }
    }

    // not all options are supported if rotate plugin is enabled
    if (pParams->nRotationAngle == 180 && (
        MFX_PICSTRUCT_PROGRESSIVE != pParams->nPicStruct ||
        pParams->nDstWidth != pParams->nWidth ||
        pParams->nDstHeight != pParams->nHeight ||
        MVC_ENABLED & pParams->MVC_flags ||
        pParams->nRateControlMethod == MFX_RATECONTROL_LA))
    {
        PrintHelp(strInput[0], MSDK_STRING("Some of the command line options are not supported with rotation plugin!"));
        return MFX_ERR_UNSUPPORTED;
    }

    if (pParams->nAsyncDepth == 0)
    {
        pParams->nAsyncDepth = 4; //set by default;
    }

    // Ignoring user-defined Async Depth for LA
    if (pParams->nMaxSliceSize)
    {
        pParams->nAsyncDepth = 1;
    }

    if (pParams->nRateControlMethod == 0)
    {
        pParams->nRateControlMethod = MFX_RATECONTROL_CBR;
    }

    if(pParams->UseRegionEncode)
    {
        if(pParams->CodecId != MFX_CODEC_HEVC)
        {
            msdk_printf(MSDK_STRING("Region encode option is compatible with h265(HEVC) encoder only.\nRegion encoding is disabled\n"));
            pParams->UseRegionEncode=false;
        }
        if (pParams->nWidth  != pParams->nDstWidth ||
            pParams->nHeight != pParams->nDstHeight ||
            pParams->nRotationAngle!=0)
        {
            msdk_printf(MSDK_STRING("Region encode option is not compatible with VPP processing and rotation plugin.\nRegion encoding is disabled\n"));
            pParams->UseRegionEncode=false;
        }
    }

    if (pParams->dstFileBuff.size() == 0)
    {
        msdk_printf(MSDK_STRING("File output is disabled as -o option isn't specified\n"));
    }

    return MFX_ERR_NONE;
}

CEncodingPipeline* CreatePipeline(const sInputParams& params)
{
    if(params.UseRegionEncode)
    {
        return new CRegionEncodingPipeline;
    }
    else if(params.nRotationAngle)
    {
        return new CUserPipeline;
    }
    else
    {
        return new CEncodingPipeline;
    }
}


int main(int argc, char *argv[])
{
    sInputParams Params = {};   // input parameters from command line
    std::unique_ptr<CEncodingPipeline>  pPipeline;

    mfxStatus sts = MFX_ERR_NONE; // return value check

    sts = ParseInputString(argv, (mfxU8)argc, &Params);
    MSDK_CHECK_PARSE_RESULT(sts, MFX_ERR_NONE, 1);

    // Choosing which pipeline to use
    pPipeline.reset(CreatePipeline(Params));

    MSDK_CHECK_POINTER(pPipeline.get(), MFX_ERR_MEMORY_ALLOC);

    if (MVC_ENABLED & Params.MVC_flags)
    {
        pPipeline->SetNumView(Params.numViews);
    }

    sts = pPipeline->Init(&Params);
    MSDK_CHECK_STATUS(sts, "pPipeline->Init failed");

    pPipeline->PrintInfo();

    msdk_printf(MSDK_STRING("Processing started\n"));

    if (pPipeline->CaptureStartV4L2Pipeline() != MFX_ERR_NONE)
    {
        msdk_printf(MSDK_STRING("V4l2 failure terminating the program\n"));
        return 0;
    }

    for (;;)
    {
        sts = pPipeline->Run();

        if (MFX_ERR_DEVICE_LOST == sts || MFX_ERR_DEVICE_FAILED == sts)
        {
            msdk_printf(MSDK_STRING("\nERROR: Hardware device was lost or returned an unexpected error. Recovering...\n"));
            sts = pPipeline->ResetDevice();
            MSDK_CHECK_STATUS(sts, "pPipeline->ResetDevice failed");

            sts = pPipeline->ResetMFXComponents(&Params);
            MSDK_CHECK_STATUS(sts, "pPipeline->ResetMFXComponents failed");
            continue;
        }
        else
        {
            MSDK_CHECK_STATUS(sts, "pPipeline->Run failed");
            break;
        }
    }

    pPipeline->CaptureStopV4L2Pipeline();

    pPipeline->Close();

    msdk_printf(MSDK_STRING("\nProcessing finished\n"));

    return 0;
}
