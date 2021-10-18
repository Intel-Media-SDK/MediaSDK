/******************************************************************************\
Copyright (c) 2005-2019, Intel Corporation
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

#include "pipeline_decode.h"
#include <sstream>
#include "version.h"

#ifndef MFX_VERSION
#error MFX_VERSION not defined
#endif

void PrintHelp(msdk_char *strAppName, const msdk_char *strErrorMessage)
{
    msdk_printf(MSDK_STRING("Decoding Sample Version %s\n\n"), GetMSDKSampleVersion().c_str());

    if (strErrorMessage)
    {
        msdk_printf(MSDK_STRING("Error: %s\n"), strErrorMessage);
    }

    msdk_printf(MSDK_STRING("Usage: %s <codecid> [<options>] -i InputBitstream\n"), strAppName);
    msdk_printf(MSDK_STRING("   or: %s <codecid> [<options>] -i InputBitstream -r\n"), strAppName);
    msdk_printf(MSDK_STRING("   or: %s <codecid> [<options>] -i InputBitstream -o OutputYUVFile\n"), strAppName);
    msdk_printf(MSDK_STRING("\n"));
    msdk_printf(MSDK_STRING("Supported codecs (<codecid>):\n"));
    msdk_printf(MSDK_STRING("   <codecid>=h264|mpeg2|vc1|mvc|jpeg|vp9|av1 - built-in Media SDK codecs\n"));
    msdk_printf(MSDK_STRING("   <codecid>=h265|vp9|capture            - in-box Media SDK plugins (may require separate downloading and installation)\n"));
    msdk_printf(MSDK_STRING("\n"));
    msdk_printf(MSDK_STRING("Work models:\n"));
    msdk_printf(MSDK_STRING("  1. Performance model: decoding on MAX speed, no screen rendering, no YUV dumping (no -r or -o option)\n"));
    msdk_printf(MSDK_STRING("  2. Rendering model: decoding with rendering on the screen (-r option)\n"));
    msdk_printf(MSDK_STRING("  3. Dump model: decoding with YUV dumping (-o option)\n"));
    msdk_printf(MSDK_STRING("\n"));
    msdk_printf(MSDK_STRING("Options:\n"));
    msdk_printf(MSDK_STRING("   [-?]                      - print help\n"));
    msdk_printf(MSDK_STRING("   [-hw]                     - use platform specific SDK implementation (default)\n"));
    msdk_printf(MSDK_STRING("   [-sw]                     - use software implementation, if not specified platform specific SDK implementation is used\n"));
    msdk_printf(MSDK_STRING("   [-p plugin]               - decoder plugin. Supported values: hevcd_sw, hevcd_hw, vp8d_hw, vp9d_hw, camera_hw, capture_hw\n"));
    msdk_printf(MSDK_STRING("   [-path path]              - path to plugin (valid only in pair with -p option)\n"));
    msdk_printf(MSDK_STRING("                               (optional for Media SDK in-box plugins, required for user-decoder ones)\n"));
    msdk_printf(MSDK_STRING("   [-fps]                    - limits overall fps of pipeline\n"));
    msdk_printf(MSDK_STRING("   [-w]                      - output width\n"));
    msdk_printf(MSDK_STRING("   [-h]                      - output height\n"));
    msdk_printf(MSDK_STRING("   [-di bob/adi]             - enable deinterlacing BOB/ADI\n"));
    msdk_printf(MSDK_STRING("   [-scaling_mode value]     - specify scaling mode (lowpower/quality) for VPP\n"));
#if (MFX_VERSION >= 1025)
    msdk_printf(MSDK_STRING("   [-d]                      - enable decode error report\n"));
#endif
#if (defined(_WIN64) || defined(_WIN32)) && (MFX_VERSION >= 1031)
    msdk_printf(MSDK_STRING("   [-dGfx]                   - preffer processing on dGfx (by default system decides)\n"));
    msdk_printf(MSDK_STRING("   [-iGfx]                   - preffer processing on iGfx (by default system decides)\n"));
#endif
#if defined(LINUX32) || defined(LINUX64)
    msdk_printf(MSDK_STRING("   [-device /path/to/device] - set graphics device for processing\n"));
    msdk_printf(MSDK_STRING("                                 For example: '-device /dev/dri/card0'\n"));
    msdk_printf(MSDK_STRING("                                              '-device /dev/dri/renderD128'\n"));
    msdk_printf(MSDK_STRING("                                 If not specified, defaults to the first Intel device found on the system\n"));
#endif
#if (MFX_VERSION >= 1035)   
    msdk_printf(MSDK_STRING("   [-ignore_level_constrain] - ignore level constrain\n"));
#endif
    msdk_printf(MSDK_STRING("   [-disable_film_grain] - disable film grain application(valid only for av1)\n"));
    msdk_printf(MSDK_STRING("\n"));
    msdk_printf(MSDK_STRING("JPEG Chroma Type:\n"));
    msdk_printf(MSDK_STRING("   [-jpeg_rgb] - RGB Chroma Type\n"));
    msdk_printf(MSDK_STRING("Output format parameters:\n"));
    msdk_printf(MSDK_STRING("   [-i420] - pipeline output format: NV12, output file format: I420\n"));
    msdk_printf(MSDK_STRING("   [-nv12] - pipeline output format: NV12, output file format: NV12\n"));
    msdk_printf(MSDK_STRING("   [-yuy2] - pipeline output format: YUV2, output file format: YUV2\n"));
    msdk_printf(MSDK_STRING("   [-uyvy] - pipeline output format: UYVY, output file format: UYVY\n"));
    msdk_printf(MSDK_STRING("   [-rgb4] - pipeline output format: RGB4, output file format: RGB4\n"));
    msdk_printf(MSDK_STRING("   [-rgb4_fcr] - pipeline output format: RGB4 in full color range, output file format: RGB4 in full color range\n"));
    msdk_printf(MSDK_STRING("   [-ayuv] - pipeline output format: AYUV, output file format: AYUV\n"));
    msdk_printf(MSDK_STRING("   [-p010] - pipeline output format: P010, output file format: P010\n"));
    msdk_printf(MSDK_STRING("   [-a2rgb10] - pipeline output format: A2RGB10, output file format: A2RGB10\n"));
#if (MFX_VERSION >= 1031)
    msdk_printf(MSDK_STRING("   [-p016] - pipeline output format: P010, output file format: P016\n"));
    msdk_printf(MSDK_STRING("   [-y216] - pipeline output format: Y216, output file format: Y216\n"));
    msdk_printf(MSDK_STRING("   [-y416] - pipeline output format: Y416, output file format: Y416\n"));
#endif
    msdk_printf(MSDK_STRING("\n"));
    msdk_printf(MSDK_STRING("   [-sys]                    - work with linear buffer in system memory\n"));
#if D3D_SURFACES_SUPPORT
    msdk_printf(MSDK_STRING("   [-d3d]                    - work with d3d9 surfaces\n"));
    msdk_printf(MSDK_STRING("   [-d3d11]                  - work with d3d11 surfaces\n"));
    msdk_printf(MSDK_STRING("   [-r]                      - render decoded data in a separate window \n"));
    msdk_printf(MSDK_STRING("   [-wall w h n m t tmo]     - same as -r, and positioned rendering window in a particular cell on specific monitor \n"));
    msdk_printf(MSDK_STRING("       w                     - number of columns of video windows on selected monitor\n"));
    msdk_printf(MSDK_STRING("       h                     - number of rows of video windows on selected monitor\n"));
    msdk_printf(MSDK_STRING("       n(0,.,w*h-1)          - order of video window in table that will be rendered\n"));
    msdk_printf(MSDK_STRING("       m(0,1..)              - monitor id \n"));
    msdk_printf(MSDK_STRING("       t(0/1)                - enable/disable window's title\n"));
    msdk_printf(MSDK_STRING("       tmo                   - timeout for -wall option\n"));
    msdk_printf(MSDK_STRING("\n"));
#endif
#if defined(LIBVA_SUPPORT)
    msdk_printf(MSDK_STRING("   [-vaapi]                  - work with vaapi surfaces\n"));
#endif
#if defined(LIBVA_X11_SUPPORT)
    msdk_printf(MSDK_STRING("   [-r]                      - render decoded data in a separate X11 window \n"));
#endif
#if defined(LIBVA_WAYLAND_SUPPORT)
    msdk_printf(MSDK_STRING("   [-rwld]                   - render decoded data in a Wayland window \n"));
    msdk_printf(MSDK_STRING("   [-perf]                   - turn on asynchronous flipping for Wayland rendering \n"));
#endif
#if defined(LIBVA_DRM_SUPPORT)
    msdk_printf(MSDK_STRING("   [-rdrm]                   - render decoded data in a thru DRM frame buffer\n"));
    msdk_printf(MSDK_STRING("   [-window x y w h]         - set render window position and size\n"));
#endif
    msdk_printf(MSDK_STRING("   [-low_latency]            - configures decoder for low latency mode (supported only for H.264 and JPEG codec)\n"));
    msdk_printf(MSDK_STRING("   [-calc_latency]           - calculates latency during decoding and prints log (supported only for H.264 and JPEG codec)\n"));
    msdk_printf(MSDK_STRING("   [-async]                  - depth of asynchronous pipeline. default value is 4. must be between 1 and 20\n"));
    msdk_printf(MSDK_STRING("   [-gpucopy::<on,off>] Enable or disable GPU copy mode\n"));
    msdk_printf(MSDK_STRING("   [-robust:soft]            - GPU hang recovery by inserting an IDR frame\n"));
    msdk_printf(MSDK_STRING("   [-timeout]                - timeout in seconds\n"));
#if MFX_VERSION >= 1022
    msdk_printf(MSDK_STRING("   [-dec_postproc force/auto] - resize after decoder using direct pipe\n"));
    msdk_printf(MSDK_STRING("                  force: instruct to use decoder-based post processing\n"));
    msdk_printf(MSDK_STRING("                         or fail if the decoded stream is unsupported\n"));
    msdk_printf(MSDK_STRING("                  auto: instruct to use decoder-based post processing for supported streams \n"));
    msdk_printf(MSDK_STRING("                        or perform VPP operation through separate pipeline component for unsupported streams\n"));

#endif //MFX_VERSION >= 1022
#if !defined(_WIN32) && !defined(_WIN64)
    msdk_printf(MSDK_STRING("   [-threads_num]            - number of mediasdk task threads\n"));
    msdk_printf(MSDK_STRING("   [-threads_schedtype]      - scheduling type of mediasdk task threads\n"));
    msdk_printf(MSDK_STRING("   [-threads_priority]       - priority of mediasdk task threads\n"));
    msdk_printf(MSDK_STRING("\n"));
    msdk_thread_printf_scheduling_help();
#endif
#if defined(_WIN32) || defined(_WIN64)
    msdk_printf(MSDK_STRING("   [-jpeg_rotate n]          - rotate jpeg frame n degrees \n"));
    msdk_printf(MSDK_STRING("       n(90,180,270)         - number of degrees \n"));

    msdk_printf(MSDK_STRING("\nFeatures: \n"));
    msdk_printf(MSDK_STRING("   Press 1 to toggle fullscreen rendering on/off\n"));
#endif
    msdk_printf(MSDK_STRING("\n"));
    msdk_printf(MSDK_STRING("Example:\n"));
    msdk_printf(MSDK_STRING("  %s h265 -i in.bit -o out.yuv -p 15dd936825ad475ea34e35f3f54217a6\n"), strAppName);
}

mfxStatus ParseInputString(msdk_char* strInput[], mfxU8 nArgNum, sInputParams* pParams)
{
    if (1 == nArgNum)
    {
        PrintHelp(strInput[0], NULL);
        return MFX_ERR_UNSUPPORTED;
    }

    MSDK_CHECK_POINTER(pParams, MFX_ERR_NULL_PTR);

    // set default implementation
    pParams->bUseHWLib = true;
    pParams->bUseFullColorRange = false;
#if defined(LIBVA_SUPPORT)
    pParams->libvaBackend = MFX_LIBVA_DRM;
#endif

    for (mfxU8 i = 1; i < nArgNum; i++)
    {
        if (MSDK_CHAR('-') != strInput[i][0])
        {
            mfxStatus sts = StrFormatToCodecFormatFourCC(strInput[i], pParams->videoType);
            if (sts != MFX_ERR_NONE)
            {
                PrintHelp(strInput[0], MSDK_STRING("Unknown codec"));
                return MFX_ERR_UNSUPPORTED;
            }
            if (!IsDecodeCodecSupported(pParams->videoType))
            {
                PrintHelp(strInput[0], MSDK_STRING("Unsupported codec"));
                return MFX_ERR_UNSUPPORTED;
            }
            if (pParams->videoType == CODEC_MVC)
            {
                pParams->videoType = MFX_CODEC_AVC;
                pParams->bIsMVC = true;
            }
            continue;
        }

        if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-?")))
        {
            PrintHelp(strInput[0], MSDK_STRING(""));
            return MFX_ERR_ABORTED;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-sw")))
        {
            pParams->bUseHWLib = false;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-hw")))
        {
            pParams->bUseHWLib = true;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-sys")))
        {
            pParams->memType = SYSTEM_MEMORY;
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
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-r")))
        {
            pParams->mode = MODE_RENDERING;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-wall")))
        {
            if(i + 6 >= nArgNum)
            {
                PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for -wall key"));
                return MFX_ERR_UNSUPPORTED;
            }

            pParams->mode = MODE_RENDERING;

            msdk_opt_read(strInput[++i], pParams->nWallW);
            msdk_opt_read(strInput[++i], pParams->nWallH);
            msdk_opt_read(strInput[++i], pParams->nWallCell);
            msdk_opt_read(strInput[++i], pParams->nWallMonitor);

            mfxU32 nTitle;
            msdk_opt_read(strInput[++i], nTitle);

            pParams->bWallNoTitle = 0 == nTitle;

            msdk_opt_read(strInput[++i], pParams->nTimeout);
        }
#endif
#if defined(LIBVA_SUPPORT)
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-vaapi")))
        {
            pParams->memType = VAAPI_MEMORY;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-r")))
        {
            pParams->mode = MODE_RENDERING;
            pParams->libvaBackend = MFX_LIBVA_X11;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-rwld")))
        {
            pParams->mode = MODE_RENDERING;
            pParams->libvaBackend = MFX_LIBVA_WAYLAND;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-perf")))
        {
            pParams->bPerfMode = true;
        }
        else if (0 == msdk_strncmp(strInput[i], MSDK_STRING("-rdrm"), 5))
        {
            pParams->mode = MODE_RENDERING;
            pParams->libvaBackend = MFX_LIBVA_DRM_MODESET;
            if (strInput[i][5]) {
                if (strInput[i][5] != '-') {
                    PrintHelp(strInput[0], MSDK_STRING("unsupported monitor type"));
                    return MFX_ERR_UNSUPPORTED;
                }
                pParams->monitorType = getMonitorType(&strInput[i][6]);
                if (pParams->monitorType >= MFX_MONITOR_MAXNUMBER) {
                    PrintHelp(strInput[0], MSDK_STRING("unsupported monitor type"));
                    return MFX_ERR_UNSUPPORTED;
                }
            } else {
                pParams->monitorType = MFX_MONITOR_AUTO; // that's case of "-rdrm" pure option
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-window")))
        {
            if(i +4 >= nArgNum)
            {
                PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for -window key"));
                return MFX_ERR_UNSUPPORTED;
            }
            msdk_opt_read(strInput[++i], pParams->nRenderWinX);
            msdk_opt_read(strInput[++i], pParams->nRenderWinY);
            msdk_opt_read(strInput[++i], pParams->Width);
            msdk_opt_read(strInput[++i], pParams->Height);

            if (0 == pParams->Width)
                pParams->Width = 320;
            if (0 == pParams->Height)
                pParams->Height = 240;

            pParams->bRenderWin = true;
        }
#endif
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-low_latency")))
        {
            switch (pParams->videoType)
            {
                case MFX_CODEC_HEVC:
                case MFX_CODEC_AVC:
                case MFX_CODEC_JPEG:
                {
                    pParams->bLowLat = true;
                    if (!pParams->bIsMVC)
                        break;
                }
                default:
                {
                     PrintHelp(strInput[0], MSDK_STRING("-low_latency mode is suppoted only for H.264 and JPEG codecs"));
                     return MFX_ERR_UNSUPPORTED;
                }
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-jpeg_rotate")))
        {
            if(MFX_CODEC_JPEG != pParams->videoType)
                return MFX_ERR_UNSUPPORTED;

            if(i + 1 >= nArgNum)
            {
                PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for -jpeg_rotate key"));
                return MFX_ERR_UNSUPPORTED;
            }

            msdk_opt_read(strInput[++i], pParams->nRotation);
            if((pParams->nRotation != 90)&&(pParams->nRotation != 180)&&(pParams->nRotation != 270))
            {
                PrintHelp(strInput[0], MSDK_STRING("-jpeg_rotate is supported only for 90, 180 and 270 angles"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-calc_latency")))
        {
            switch (pParams->videoType)
            {
                case MFX_CODEC_HEVC:
                case MFX_CODEC_AVC:
                case MFX_CODEC_JPEG:
                {
                    pParams->bCalLat = true;
                    if (!pParams->bIsMVC)
                        break;
                }
                default:
                {
                     PrintHelp(strInput[0], MSDK_STRING("-calc_latency mode is suppoted only for H.264 and JPEG codecs"));
                     return MFX_ERR_UNSUPPORTED;
                }
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-async")))
        {
            if(i + 1 >= nArgNum)
            {
                PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for -async key"));
                return MFX_ERR_UNSUPPORTED;
            }
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->nAsyncDepth))
            {
                PrintHelp(strInput[0], MSDK_STRING("async is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-timeout")))
        {
            if(i + 1 >= nArgNum)
            {
                PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for -timeout key"));
                return MFX_ERR_UNSUPPORTED;
            }
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->nTimeout))
            {
                PrintHelp(strInput[0], MSDK_STRING("timeout is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-di")))
        {
            if(i + 1 >= nArgNum)
            {
                PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for -di key"));
                return MFX_ERR_UNSUPPORTED;
            }
            msdk_char diMode[32] = {};
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], diMode))
            {
                PrintHelp(strInput[0], MSDK_STRING("deinterlace value is not set"));
                return MFX_ERR_UNSUPPORTED;
            }

            if (0 == msdk_strcmp(diMode, MSDK_CHAR("bob")))
            {
                pParams->eDeinterlace = MFX_DEINTERLACING_BOB;
            }
            else if (0 == msdk_strcmp(diMode, MSDK_CHAR("adi")))
            {
                pParams->eDeinterlace = MFX_DEINTERLACING_ADVANCED;
            }
            else
            {
                PrintHelp(strInput[0], MSDK_STRING("deinterlace value is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-scaling_mode")))
        {
            if(i + 1 >= nArgNum)
            {
                PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for -scaling_mode key"));
                return MFX_ERR_UNSUPPORTED;
            }
            msdk_char diMode[32] = {};
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], diMode))
            {
                PrintHelp(strInput[0], MSDK_STRING("mode type is not set"));
                return MFX_ERR_UNSUPPORTED;
            }
            if (0 == msdk_strcmp(diMode, MSDK_CHAR("lowpower")))
            {
                pParams->ScalingMode = MFX_SCALING_MODE_LOWPOWER;
            }
            else if (0 == msdk_strcmp(diMode, MSDK_CHAR("quality")))
            {
                pParams->ScalingMode = MFX_SCALING_MODE_QUALITY;
            }
            else
            {
                PrintHelp(strInput[0], MSDK_STRING("deinterlace value is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-gpucopy::on")))
        {
            pParams->gpuCopy = MFX_GPUCOPY_ON;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-gpucopy::off")))
        {
            pParams->gpuCopy = MFX_GPUCOPY_OFF;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-robust:soft")))
        {
            pParams->bSoftRobustFlag = true;
        }
#if (defined(LINUX32) || defined(LINUX64))
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-device")))
        {
            if (!pParams->strDevicePath.empty())
            {
                msdk_printf(MSDK_STRING("error: you can specify only one device\n"));
                return MFX_ERR_UNSUPPORTED;
            }
            if(i + 1 >= nArgNum)
            {
                PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for -device key"));
                return MFX_ERR_UNSUPPORTED;
            }
            pParams->strDevicePath = strInput[++i];
        }
#endif
#if (defined(_WIN64) || defined(_WIN32)) && (MFX_VERSION >= 1031)
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-dGfx")))
        {
            pParams->bPrefferdGfx = true;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-iGfx")))
        {
            pParams->bPrefferiGfx = true;
        }
#endif
#if !defined(_WIN32) && !defined(_WIN64)
#if (MFX_VERSION >= 1025)
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-d")))
        {
            pParams->bErrorReport = true;
        }
#endif
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-threads_num")))
        {
            if(i + 1 >= nArgNum)
            {
                PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for -threads_num key"));
                return MFX_ERR_UNSUPPORTED;
            }
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->nThreadsNum))
            {
                PrintHelp(strInput[0], MSDK_STRING("threads_num is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-threads_schedtype")))
        {
            if(i + 1 >= nArgNum)
            {
                PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for -threads_schedtype key"));
                return MFX_ERR_UNSUPPORTED;
            }
            if (MFX_ERR_NONE != msdk_thread_get_schedtype(strInput[++i], pParams->SchedulingType))
            {
                PrintHelp(strInput[0], MSDK_STRING("threads_schedtype is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-threads_priority")))
        {
            if(i + 1 >= nArgNum)
            {
                PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for -threads_priority key"));
                return MFX_ERR_UNSUPPORTED;
            }
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->Priority))
            {
                PrintHelp(strInput[0], MSDK_STRING("threads_priority is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
#endif // #if !defined(_WIN32) && !defined(_WIN64)
#if MFX_VERSION >= 1022
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-dec_postproc")))
        {
            if(i + 1 >= nArgNum)
            {
                PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for \"-dec_postproc\", right is  \"--dec_postproc force//auto\""));
                return MFX_ERR_UNSUPPORTED;
            }
            msdk_char postProcMode[32] = {};
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], postProcMode))
            {
                PrintHelp(strInput[0], MSDK_STRING("dec_postproc value is not set"));
                return MFX_ERR_UNSUPPORTED;
            }
            if (0 == msdk_strcmp(postProcMode, MSDK_STRING("auto")))
            {
                pParams->nDecoderPostProcessing = MODE_DECODER_POSTPROC_AUTO;
            }
            else if (0 == msdk_strcmp(postProcMode, MSDK_STRING("force")))
            {
                pParams->nDecoderPostProcessing = MODE_DECODER_POSTPROC_FORCE;
            }
            else
            {
                PrintHelp(strInput[0], MSDK_STRING("dec_postproc is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
#endif //MFX_VERSION >= 1022
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-fps")))
        {
            if(i + 1 >= nArgNum)
            {
                PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for -fps key"));
                return MFX_ERR_UNSUPPORTED;
            }
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->nMaxFPS))
            {
                PrintHelp(strInput[0], MSDK_STRING("overall fps is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-w")))
        {
            if (i + 1 >= nArgNum)
            {
                PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for -w key"));
                return MFX_ERR_UNSUPPORTED;
            }
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->Width))
            {
                PrintHelp(strInput[0], MSDK_STRING("width is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-h")))
        {
            if (i + 1 >= nArgNum)
            {
                PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for -h key"));
                return MFX_ERR_UNSUPPORTED;
            }
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->Height))
            {
                PrintHelp(strInput[0], MSDK_STRING("height is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-n")))
        {
            if(i + 1 >= nArgNum)
            {
                PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for -n key"));
                return MFX_ERR_UNSUPPORTED;
            }
            if (MFX_ERR_NONE != msdk_opt_read(strInput[++i], pParams->nFrames))
            {
                PrintHelp(strInput[0], MSDK_STRING("rendering frame rate is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-jpeg_rgb")))
        {
            if(MFX_CODEC_JPEG == pParams->videoType)
            {
               pParams->chromaType = MFX_JPEG_COLORFORMAT_RGB;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-i420")))
        {
            pParams->fourcc = MFX_FOURCC_NV12;
            pParams->outI420 = true;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-nv12")))
        {
            pParams->fourcc = MFX_FOURCC_NV12;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-rgb4")))
        {
            pParams->fourcc = MFX_FOURCC_RGB4;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-ayuv")))
        {
            pParams->fourcc = MFX_FOURCC_AYUV;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-yuy2")))
        {
            pParams->fourcc = MFX_FOURCC_YUY2;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-uyvy")))
        {
            pParams->fourcc = MFX_FOURCC_UYVY;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-rgb4_fcr")))
        {
            pParams->fourcc = MFX_FOURCC_RGB4;
            pParams->bUseFullColorRange = true;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-p010")))
        {
            pParams->fourcc = MFX_FOURCC_P010;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-a2rgb10")))
        {
            pParams->fourcc = MFX_FOURCC_A2RGB10;
        }
#if (MFX_VERSION >= 1031)
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-p016")))
        {
            pParams->fourcc = MFX_FOURCC_P016;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-y216")))
        {
            pParams->fourcc = MFX_FOURCC_Y216;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-y416")))
        {
            pParams->fourcc = MFX_FOURCC_Y416;
        }
#endif
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-path")))
        {
            i++;
            pParams->pluginParams = ParsePluginPath(strInput[i]);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-i:null")))
        {
            ;
        }
#if (MFX_VERSION >= 1034)
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-ignore_level_constrain")))
        {
            pParams->bIgnoreLevelConstrain = true;
        }
#endif
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-disable_film_grain")))
        {
            pParams->bDisableFilmGrain = true;
        }
        else // 1-character options
        {
            switch (strInput[i][1])
            {
            case MSDK_CHAR('p'):
                if (++i < nArgNum) {
                    pParams->pluginParams = ParsePluginGuid(strInput[i]);
                    if (AreGuidsEqual(pParams->pluginParams.pluginGuid, MSDK_PLUGINGUID_NULL))
                    {
                        msdk_printf(MSDK_STRING("error: invalid decoder plugin\n"));
                        return MFX_ERR_UNSUPPORTED;
                    }
                 }
                break;
            case MSDK_CHAR('i'):
                if (++i < nArgNum) {
                    msdk_opt_read(strInput[i], pParams->strSrcFile);
                }
                else {
                    msdk_printf(MSDK_STRING("error: option '-i' expects an argument\n"));
                }
                break;
            case MSDK_CHAR('o'):
                if (++i < nArgNum) {
                    pParams->mode = MODE_FILE_DUMP;
                    msdk_opt_read(strInput[i], pParams->strDstFile);
                }
                else {
                    msdk_printf(MSDK_STRING("error: option '-o' expects an argument\n"));
                }
                break;
            case MSDK_CHAR('?'):
                PrintHelp(strInput[0], NULL);
                return MFX_ERR_UNSUPPORTED;
            default:
                {
                    std::basic_stringstream<msdk_char> stream;
                    stream << MSDK_STRING("Unknown option: ") << strInput[i];
                    PrintHelp(strInput[0], stream.str().c_str());
                    return MFX_ERR_UNSUPPORTED;
                }
            }
        }
    }

    if (0 == msdk_strlen(pParams->strSrcFile))
    {
        msdk_printf(MSDK_STRING("error: source file name not found"));
        return MFX_ERR_UNSUPPORTED;
    }

    if ((pParams->mode == MODE_FILE_DUMP) && (0 == msdk_strlen(pParams->strDstFile)))
    {
        msdk_printf(MSDK_STRING("error: destination file name not found"));
        return MFX_ERR_UNSUPPORTED;
    }

    if (MFX_CODEC_MPEG2   != pParams->videoType &&
        MFX_CODEC_AVC     != pParams->videoType &&
        MFX_CODEC_HEVC    != pParams->videoType &&
        MFX_CODEC_VC1     != pParams->videoType &&
        MFX_CODEC_JPEG    != pParams->videoType &&
        MFX_CODEC_VP8     != pParams->videoType &&
        MFX_CODEC_VP9     != pParams->videoType &&
        MFX_CODEC_AV1     != pParams->videoType)
    {
        PrintHelp(strInput[0], MSDK_STRING("Unknown codec"));
        return MFX_ERR_UNSUPPORTED;
    }

    if (pParams->nAsyncDepth == 0)
    {
        pParams->nAsyncDepth = 4; //set by default;
    }

#if (defined(_WIN64) || defined(_WIN32)) && (MFX_VERSION >= 1031)
    if (pParams->bPrefferdGfx && pParams->bPrefferiGfx)
    {
        msdk_printf(MSDK_STRING("Warning: both dGfx and iGfx flags set. iGfx will be preffered"));
        pParams->bPrefferdGfx = false;
    }
#endif

    if(pParams->mode == MODE_RENDERING && pParams->memType == SYSTEM_MEMORY)
    {
        msdk_printf(MSDK_STRING("error: rendering model is unsupported for system memory"));
        return MFX_ERR_UNSUPPORTED;
    }

    return MFX_ERR_NONE;
}

#if defined(_WIN32) || defined(_WIN64)
int _tmain(int argc, TCHAR *argv[])
#else
int main(int argc, char *argv[])
#endif
{
    sInputParams        Params = {};   // input parameters from command line
    CDecodingPipeline   Pipeline; // pipeline for decoding, includes input file reader, decoder and output file writer

    mfxStatus sts = MFX_ERR_NONE; // return value check

    sts = ParseInputString(argv, (mfxU8)argc, &Params);
    if (sts == MFX_ERR_ABORTED)
    {
        // No error, just need to close app normally
        return MFX_ERR_NONE;
    }
    MSDK_CHECK_PARSE_RESULT(sts, MFX_ERR_NONE, 1);

    if (Params.bIsMVC)
        Pipeline.SetMultiView();

    sts = Pipeline.Init(&Params);
    MSDK_CHECK_STATUS(sts, "Pipeline.Init failed");

    // print stream info
    Pipeline.PrintInfo();

    msdk_printf(MSDK_STRING("Decoding started\n"));

    mfxU64 prevResetBytesCount = 0xFFFFFFFFFFFFFFFF;
    for (;;)
    {
        sts = Pipeline.RunDecoding();

        if (MFX_ERR_INCOMPATIBLE_VIDEO_PARAM == sts || MFX_ERR_DEVICE_LOST == sts || MFX_ERR_DEVICE_FAILED == sts)
        {
            if (prevResetBytesCount == Pipeline.GetTotalBytesProcessed())
            {
                msdk_printf(MSDK_STRING("\nERROR: No input data was consumed since last reset. Quitting to avoid looping forever.\n"));
                break;
            }
            prevResetBytesCount = Pipeline.GetTotalBytesProcessed();


            if (MFX_ERR_INCOMPATIBLE_VIDEO_PARAM == sts)
            {
                msdk_printf(MSDK_STRING("\nERROR: Incompatible video parameters detected. Recovering...\n"));
            }
            else
            {
                msdk_printf(MSDK_STRING("\nERROR: Hardware device was lost or returned unexpected error. Recovering...\n"));
                sts = Pipeline.ResetDevice();
                MSDK_CHECK_STATUS(sts, "Pipeline.ResetDevice failed");
            }

            sts = Pipeline.ResetDecoder(&Params);
            MSDK_CHECK_STATUS(sts, "Pipeline.ResetDecoder failed");
            continue;
        }
        else
        {
            MSDK_CHECK_STATUS(sts, "Pipeline.RunDecoding failed");
            break;
        }
    }

    msdk_printf(MSDK_STRING("\nDecoding finished\n"));

    return 0;
}
