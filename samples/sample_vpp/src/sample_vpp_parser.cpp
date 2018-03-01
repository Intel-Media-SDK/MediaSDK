/******************************************************************************\
Copyright (c) 2005-2017, Intel Corporation
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

#include "sample_vpp_utils.h"
#include <string>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <fstream>
#include "version.h"

#define VAL_CHECK(val) {if (val) return MFX_ERR_UNKNOWN;}

void vppPrintHelp(const msdk_char *strAppName, const msdk_char *strErrorMessage)
{
    msdk_printf(MSDK_STRING("Intel(R) Media SDK VPP Sample version %s\n"), GetMSDKSampleVersion().c_str());
    if (strErrorMessage)
    {
        msdk_printf(MSDK_STRING("Error: %s\n"), strErrorMessage);
    }

msdk_printf(MSDK_STRING("Usage: %s [Options] -i InputFile -o OutputFile\n"), strAppName);

msdk_printf(MSDK_STRING("Options: \n"));
msdk_printf(MSDK_STRING("   [-lib  type]        - type of used library. sw, hw (def: sw)\n\n"));
#if defined(D3D_SURFACES_SUPPORT)
msdk_printf(MSDK_STRING("   [-d3d]                - use d3d9 surfaces\n\n"));
#endif
#if MFX_D3D11_SUPPORT
msdk_printf(MSDK_STRING("   [-d3d11]              - use d3d11 surfaces\n\n"));
#endif
#ifdef LIBVA_SUPPORT
msdk_printf(MSDK_STRING("   [-vaapi]                - work with vaapi surfaces\n\n"));
#endif
msdk_printf(MSDK_STRING("   [-plugin_guid GUID]\n"));
msdk_printf(MSDK_STRING("   [-p GUID]           - use VPP plug-in with specified GUID\n\n"));
msdk_printf(MSDK_STRING("   [-extapi]           - use RunFrameVPPAsyncEx instead of RunFrameVPPAsync. Need for PTIR.\n\n"));
msdk_printf(MSDK_STRING("   [-gpu_copy]         - Specify GPU copy mode. This option triggers using of InitEX instead of Init.\n\n"));

msdk_printf(MSDK_STRING("   [-sw   width]               - width  of src video (def: 352)\n"));
msdk_printf(MSDK_STRING("   [-sh   height]              - height of src video (def: 288)\n"));
msdk_printf(MSDK_STRING("   [-scrX  x]                  - cropX  of src video (def: 0)\n"));
msdk_printf(MSDK_STRING("   [-scrY  y]                  - cropY  of src video (def: 0)\n"));
msdk_printf(MSDK_STRING("   [-scrW  w]                  - cropW  of src video (def: width)\n"));
msdk_printf(MSDK_STRING("   [-scrH  h]                  - cropH  of src video (def: height)\n"));
msdk_printf(MSDK_STRING("   [-sf   frameRate]           - frame rate of src video (def: 30.0)\n"));
#if MFX_VERSION >= MFX_VERSION_NEXT
msdk_printf(MSDK_STRING("   [-scc  format]              - format (FourCC) of src video (def: nv12. support i420|nv12|yv12|yuy2|rgb565|rgb3|rgb4|imc3|yuv400|yuv411|yuv422h|yuv422v|yuv444|uyvy|ayuv|p010)\n"));
#else
msdk_printf(MSDK_STRING("   [-scc  format]              - format (FourCC) of src video (def: nv12. support i420|nv12|yv12|yuy2|rgb3|rgb4|imc3|yuv400|yuv411|yuv422h|yuv422v|yuv444|uyvy|ayuv|p010)\n"));
#endif
msdk_printf(MSDK_STRING("   [-sbitshift 0|1]            - shift data to right or keep it the same way as in Microsoft's P010\n"));
msdk_printf(MSDK_STRING("   [-sbitdepthluma value]      - shift luma channel to right to \"16 - value\" bytes\n"));
msdk_printf(MSDK_STRING("   [-sbitdepthchroma value]    - shift chroma channel to right to \"16 - value\" bytes\n"));

msdk_printf(MSDK_STRING("   [-spic value]               - picture structure of src video\n"));
msdk_printf(MSDK_STRING("                                 0 - interlaced top    field first\n"));
msdk_printf(MSDK_STRING("                                 2 - interlaced bottom field first\n"));
msdk_printf(MSDK_STRING("                                 3 - single field\n"));
msdk_printf(MSDK_STRING("                                 1 - progressive (default)\n"));
msdk_printf(MSDK_STRING("                                -1 - unknown\n\n"));

msdk_printf(MSDK_STRING("   [-dw  width]                - width  of dst video (def: 352)\n"));
msdk_printf(MSDK_STRING("   [-dh  height]               - height of dst video (def: 288)\n"));
msdk_printf(MSDK_STRING("   [-dcrX  x]                  - cropX  of dst video (def: 0)\n"));
msdk_printf(MSDK_STRING("   [-dcrY  y]                  - cropY  of dst video (def: 0)\n"));
msdk_printf(MSDK_STRING("   [-dcrW  w]                  - cropW  of dst video (def: width)\n"));
msdk_printf(MSDK_STRING("   [-dcrH  h]                  - cropH  of dst video (def: height)\n"));
msdk_printf(MSDK_STRING("   [-df  frameRate]            - frame rate of dst video (def: 30.0)\n"));
msdk_printf(MSDK_STRING("   [-dcc format]               - format (FourCC) of dst video (def: nv12. support i420|nv12|yuy2|rgb4|yv12|ayuv)\n"));
msdk_printf(MSDK_STRING("   [-dbitshift 0|1]            - shift data to right or keep it the same way as in Microsoft's P010\n"));
msdk_printf(MSDK_STRING("   [-dbitdepthluma value]      - shift luma channel to left to \"16 - value\" bytes\n"));
msdk_printf(MSDK_STRING("   [-dbitdepthchroma value]    - shift chroma channel to left to \"16 - value\" bytes\n"));

msdk_printf(MSDK_STRING("   [-dpic value]               - picture structure of dst video\n"));
msdk_printf(MSDK_STRING("                                 0 - interlaced top    field first\n"));
msdk_printf(MSDK_STRING("                                 2 - interlaced bottom field first\n"));
msdk_printf(MSDK_STRING("                                 3 - single field\n"));
msdk_printf(MSDK_STRING("                                 1 - progressive (default)\n"));
msdk_printf(MSDK_STRING("                                -1 - unknown\n\n"));

msdk_printf(MSDK_STRING("   Video Composition\n"));
msdk_printf(MSDK_STRING("   [-composite parameters_file] - composite several input files in one output. The syntax of the parameters file is:\n"));    msdk_printf(MSDK_STRING("                                  stream=<video file name>\n"));
msdk_printf(MSDK_STRING("                                  width=<input video width>\n"));
msdk_printf(MSDK_STRING("                                  height=<input video height>\n"));
msdk_printf(MSDK_STRING("                                  cropx=<input cropX (def: 0)>\n"));
msdk_printf(MSDK_STRING("                                  cropy=<input cropY (def: 0)>\n"));
msdk_printf(MSDK_STRING("                                  cropw=<input cropW (def: width)>\n"));
msdk_printf(MSDK_STRING("                                  croph=<input cropH (def: height)>\n"));
msdk_printf(MSDK_STRING("                                  framerate=<input frame rate (def: 30.0)>\n"));
msdk_printf(MSDK_STRING("                                  fourcc=<format (FourCC) of input video (def: nv12. support nv12|rgb4)>\n"));
msdk_printf(MSDK_STRING("                                  picstruct=<picture structure of input video,\n"));
msdk_printf(MSDK_STRING("                                             0 = interlaced top    field first\n"));
msdk_printf(MSDK_STRING("                                             2 = interlaced bottom field first\n"));
msdk_printf(MSDK_STRING("                                             3 = single field\n"));
msdk_printf(MSDK_STRING("                                             1 = progressive (default)>\n"));
msdk_printf(MSDK_STRING("                                  dstx=<X coordinate of input video located in the output (def: 0)>\n"));
msdk_printf(MSDK_STRING("                                  dsty=<Y coordinate of input video located in the output (def: 0)>\n"));
msdk_printf(MSDK_STRING("                                  dstw=<width of input video located in the output (def: width)>\n"));
msdk_printf(MSDK_STRING("                                  dsth=<height of input video located in the output (def: height)>\n\n"));
msdk_printf(MSDK_STRING("                                  stream=<video file name>\n"));
msdk_printf(MSDK_STRING("                                  width=<input video width>\n"));
msdk_printf(MSDK_STRING("                                  GlobalAlphaEnable=1\n"));
msdk_printf(MSDK_STRING("                                  GlobalAlpha=<Alpha value>\n"));
msdk_printf(MSDK_STRING("                                  LumaKeyEnable=1\n"));
msdk_printf(MSDK_STRING("                                  LumaKeyMin=<Luma key min value>\n"));
msdk_printf(MSDK_STRING("                                  LumaKeyMax=<Luma key max value>\n"));
msdk_printf(MSDK_STRING("                                  ...\n"));
msdk_printf(MSDK_STRING("                                  The parameters file may contain up to 64 streams.\n\n"));
msdk_printf(MSDK_STRING("                                  For example:\n"));
msdk_printf(MSDK_STRING("                                  stream=input_720x480.yuv\n"));
msdk_printf(MSDK_STRING("                                  width=720\n"));
msdk_printf(MSDK_STRING("                                  height=480\n"));
msdk_printf(MSDK_STRING("                                  cropx=0\n"));
msdk_printf(MSDK_STRING("                                  cropy=0\n"));
msdk_printf(MSDK_STRING("                                  cropw=720\n"));
msdk_printf(MSDK_STRING("                                  croph=480\n"));
msdk_printf(MSDK_STRING("                                  dstx=0\n"));
msdk_printf(MSDK_STRING("                                  dsty=0\n"));
msdk_printf(MSDK_STRING("                                  dstw=720\n"));
msdk_printf(MSDK_STRING("                                  dsth=480\n\n"));
msdk_printf(MSDK_STRING("                                  stream=input_480x320.yuv\n"));
msdk_printf(MSDK_STRING("                                  width=480\n"));
msdk_printf(MSDK_STRING("                                  height=320\n"));
msdk_printf(MSDK_STRING("                                  cropx=0\n"));
msdk_printf(MSDK_STRING("                                  cropy=0\n"));
msdk_printf(MSDK_STRING("                                  cropw=480\n"));
msdk_printf(MSDK_STRING("                                  croph=320\n"));
msdk_printf(MSDK_STRING("                                  dstx=100\n"));
msdk_printf(MSDK_STRING("                                  dsty=100\n"));
msdk_printf(MSDK_STRING("                                  dstw=320\n"));
msdk_printf(MSDK_STRING("                                  dsth=240\n"));
msdk_printf(MSDK_STRING("                                  GlobalAlphaEnable=1\n"));
msdk_printf(MSDK_STRING("                                  GlobalAlpha=128\n"));
msdk_printf(MSDK_STRING("                                  LumaKeyEnable=1\n"));
msdk_printf(MSDK_STRING("                                  LumaKeyMin=250\n"));
msdk_printf(MSDK_STRING("                                  LumaKeyMax=255\n"));
msdk_printf(MSDK_STRING("   Video Enhancement Algorithms\n"));

msdk_printf(MSDK_STRING("   [-di_mode (mode)] - set type of deinterlace algorithm\n"));
msdk_printf(MSDK_STRING("                        12 - advanced with Scene Change Detection (SCD) \n"));
msdk_printf(MSDK_STRING("                        8 - reverse telecine for a selected telecine pattern (use -tc_pattern). For PTIR plug-in\n"));
msdk_printf(MSDK_STRING("                        2 - advanced or motion adaptive (default)\n"));
msdk_printf(MSDK_STRING("                        1 - simple or BOB\n\n"));

msdk_printf(MSDK_STRING("   [-deinterlace (type)] - enable deinterlace algorithm (alternative way: -spic 0 -dpic 1) \n"));
msdk_printf(MSDK_STRING("                         type is tff (default) or bff \n"));

msdk_printf(MSDK_STRING("   [-rotate (angle)]   - enable rotation. Supported angles: 0, 90, 180, 270.\n"));

msdk_printf(MSDK_STRING("   [-scaling_mode (mode)] - specify type of scaling to be used for resize\n"));
msdk_printf(MSDK_STRING("                            0 - default\n"));
msdk_printf(MSDK_STRING("                            1 - low power mode\n"));
msdk_printf(MSDK_STRING("                            2 - quality mode\n\n"));

msdk_printf(MSDK_STRING("   [-denoise (level)]  - enable denoise algorithm. Level is optional \n"));
msdk_printf(MSDK_STRING("                         range of  noise level is [0, 100]\n"));
msdk_printf(MSDK_STRING("   [-detail  (level)]  - enable detail enhancement algorithm. Level is optional \n"));
msdk_printf(MSDK_STRING("                         range of detail level is [0, 100]\n\n"));
msdk_printf(MSDK_STRING("   [-pa_hue  hue]        - procamp hue property.         range [-180.0, 180.0] (def: 0.0)\n"));
msdk_printf(MSDK_STRING("   [-pa_sat  saturation] - procamp satursation property. range [   0.0,  10.0] (def: 1.0)\n"));
msdk_printf(MSDK_STRING("   [-pa_con  contrast]   - procamp contrast property.    range [   0.0,  10.0] (def: 1.0)\n"));
msdk_printf(MSDK_STRING("   [-pa_bri  brightness] - procamp brightness property.  range [-100.0, 100.0] (def: 0.0)\n\n"));
msdk_printf(MSDK_STRING("   [-gamut:compression]  - enable gamut compression algorithm (xvYCC->sRGB) \n"));
msdk_printf(MSDK_STRING("   [-gamut:bt709]        - enable BT.709 matrix transform (RGB->YUV conversion)(def: BT.601)\n\n"));
msdk_printf(MSDK_STRING("   [-frc:advanced]       - enable advanced FRC algorithm (based on PTS) \n"));
msdk_printf(MSDK_STRING("   [-frc:interp]         - enable FRC based on frame interpolation algorithm\n\n"));

msdk_printf(MSDK_STRING("   [-tcc:red]            - enable color saturation algorithm (R component) \n"));
msdk_printf(MSDK_STRING("   [-tcc:green]          - enable color saturation algorithm (G component)\n"));
msdk_printf(MSDK_STRING("   [-tcc:blue]           - enable color saturation algorithm (B component)\n"));
msdk_printf(MSDK_STRING("   [-tcc:cyan]           - enable color saturation algorithm (C component)\n"));
msdk_printf(MSDK_STRING("   [-tcc:magenta]        - enable color saturation algorithm (M component)\n"));
msdk_printf(MSDK_STRING("   [-tcc:yellow]         - enable color saturation algorithm (Y component)\n\n"));

msdk_printf(MSDK_STRING("   [-ace]                - enable auto contrast enhancement algorithm \n\n"));
msdk_printf(MSDK_STRING("   [-ste (level)]        - enable Skin Tone Enhancement algorithm.  Level is optional \n"));
msdk_printf(MSDK_STRING("                           range of ste level is [0, 9] (def: 4)\n\n"));
msdk_printf(MSDK_STRING("   [-istab (mode)]       - enable Image Stabilization algorithm.  Mode is optional \n"));
msdk_printf(MSDK_STRING("                           mode of istab can be [1, 2] (def: 2)\n"));
msdk_printf(MSDK_STRING("                           where: 1 means upscale mode, 2 means croppping mode\n"));
msdk_printf(MSDK_STRING("   [-view:count value]   - enable Multi View preprocessing. range of views [1, 1024] (def: 1)\n\n"));
msdk_printf(MSDK_STRING("   [-svc id width height]- enable Scalable Video Processing mode\n"));
msdk_printf(MSDK_STRING("                           id-layerId, width/height-resolution \n\n"));
msdk_printf(MSDK_STRING("   [-ssitm (id)]         - specify YUV<->RGB transfer matrix for input surface.\n"));
msdk_printf(MSDK_STRING("   [-dsitm (id)]         - specify YUV<->RGB transfer matrix for output surface.\n"));
msdk_printf(MSDK_STRING("   [-ssinr (id)]         - specify YUV nominal range for input surface.\n"));
msdk_printf(MSDK_STRING("   [-dsinr (id)]         - specify YUV nominal range for output surface.\n\n"));
msdk_printf(MSDK_STRING("   [-mirror (mode)]      - mirror image using specified mode.\n"));

msdk_printf(MSDK_STRING("   [-n frames] - number of frames to VPP process\n\n"));

msdk_printf(MSDK_STRING("   [-iopattern IN/OUT surface type] -  IN/OUT surface type: sys_to_sys, sys_to_d3d, d3d_to_sys, d3d_to_d3d    (def: sys_to_sys)\n"));
msdk_printf(MSDK_STRING("   [-async n] - maximum number of asynchronious tasks. def: -async 1 \n"));
msdk_printf(MSDK_STRING("   [-perf_opt n m] - n: number of prefetech frames. m : number of passes. In performance mode app preallocates bufer and load first n frames,  def: no performace 1 \n"));
msdk_printf(MSDK_STRING("   [-pts_check] - checking of time stampls. Default is OFF \n"));
msdk_printf(MSDK_STRING("   [-pts_jump ] - checking of time stamps jumps. Jump for random value since 13-th frame. Also, you can change input frame rate (via pts). Default frame_rate = sf \n"));
msdk_printf(MSDK_STRING("   [-pts_fr ]   - input frame rate which used for pts. Default frame_rate = sf \n"));
msdk_printf(MSDK_STRING("   [-pts_advanced]   - enable FRC checking mode based on PTS \n"));
msdk_printf(MSDK_STRING("   [-pf file for performance data] -  file to save performance data. Default is off \n\n\n"));

msdk_printf(MSDK_STRING("   [-roi_check mode seed1 seed2] - checking of ROI processing. Default is OFF \n"));
msdk_printf(MSDK_STRING("               mode - usage model of cropping\n"));
msdk_printf(MSDK_STRING("                      var_to_fix - variable input ROI and fixed output ROI\n"));
msdk_printf(MSDK_STRING("                      fix_to_var - fixed input ROI and variable output ROI\n"));
msdk_printf(MSDK_STRING("                      var_to_var - variable input ROI and variable output ROI\n"));
msdk_printf(MSDK_STRING("               seed1 - seed for init of rand generator for src\n"));
msdk_printf(MSDK_STRING("               seed2 - seed for init of rand generator for dst\n"));
msdk_printf(MSDK_STRING("                       range of seed [1, 65535]. 0 reserved for random init\n\n"));

msdk_printf(MSDK_STRING("   [-tc_pattern (pattern)] - set telecine pattern\n"));
msdk_printf(MSDK_STRING("                        4 - provide a position inside a sequence of 5 frames where the artifacts starts. Use to -tc_pos to provide position\n"));
msdk_printf(MSDK_STRING("                        3 - 4:1 pattern\n"));
msdk_printf(MSDK_STRING("                        2 - frame repeat pattern\n"));
msdk_printf(MSDK_STRING("                        1 - 2:3:3:2 pattern\n"));
msdk_printf(MSDK_STRING("                        0 - 3:2 pattern\n\n"));

msdk_printf(MSDK_STRING("   [-tc_pos (position)] - Position inside a telecine sequence of 5 frames where the artifacts starts - Value [0 - 4]\n\n"));

msdk_printf(MSDK_STRING("   [-reset_start (frame number)] - after reaching this frame, encoder will be reset with new parameters, followed after this command and before -reset_end \n"));
msdk_printf(MSDK_STRING("   [-reset_end]                  - specifies end of reset related options \n\n"));

msdk_printf(MSDK_STRING("\n"));

msdk_printf(MSDK_STRING("Usage2: %s -sw 352 -sh 144 -scc rgb3 -dw 320 -dh 240 -dcc nv12 -denoise 32 -iopattern d3d_to_d3d -i in.rgb -o out.yuv -roi_check var_to_fix 7 7\n"), strAppName);

msdk_printf(MSDK_STRING("\n"));

} // void vppPrintHelp(msdk_char *strAppName, msdk_char *strErrorMessage)


mfxU16 GetPicStruct( mfxI8 picStruct )
{
 if ( 0 == picStruct )
    {
        return MFX_PICSTRUCT_FIELD_TFF;
    }
    else if( 2 == picStruct )
    {
        return MFX_PICSTRUCT_FIELD_BFF;
    }
    else if( 3 == picStruct )
    {
        return MFX_PICSTRUCT_FIELD_SINGLE;
    }
    else if( -1 == picStruct )
    {
        return MFX_PICSTRUCT_UNKNOWN;
    }
    else
    {
        return MFX_PICSTRUCT_PROGRESSIVE;
    }

} // mfxU16 GetPicStruct( mfxI8 picStruct )


mfxU32 Str2FourCC( msdk_char* strInput )
{
    mfxU32 fourcc = 0;//default

    if ( 0 == msdk_stricmp(strInput, MSDK_STRING("yv12")) )
    {
        fourcc = MFX_FOURCC_YV12;
    }
#if (MFX_VERSION >= MFX_VERSION_NEXT)
    else if ( 0 == msdk_stricmp(strInput, MSDK_STRING("rgb565")) )
    {
        fourcc = MFX_FOURCC_RGB565;
    }
#endif
    else if ( 0 == msdk_stricmp(strInput, MSDK_STRING("rgb3")) )
    {
        fourcc = MFX_FOURCC_RGB3;
    }
    else if ( 0 == msdk_stricmp(strInput, MSDK_STRING("rgb4")) )
    {
        fourcc = MFX_FOURCC_RGB4;
    }
    else if ( 0 == msdk_stricmp(strInput, MSDK_STRING("yuy2")) )
    {
        fourcc = MFX_FOURCC_YUY2;
    }
    else if ( 0 == msdk_stricmp(strInput, MSDK_STRING("nv12")) )
    {
        fourcc = MFX_FOURCC_NV12;
    }
    else if ( 0 == msdk_stricmp(strInput, MSDK_STRING("imc3")) )
    {
        fourcc = MFX_FOURCC_IMC3;
    }
    else if ( 0 == msdk_stricmp(strInput, MSDK_STRING("yuv400")) )
    {
        fourcc = MFX_FOURCC_YUV400;
    }
    else if ( 0 == msdk_stricmp(strInput, MSDK_STRING("yuv411")) )
    {
        fourcc = MFX_FOURCC_YUV411;
    }
    else if ( 0 == msdk_stricmp(strInput, MSDK_STRING("yuv422h")) )
    {
        fourcc = MFX_FOURCC_YUV422H;
    }
    else if ( 0 == msdk_stricmp(strInput, MSDK_STRING("yuv422v")) )
    {
        fourcc = MFX_FOURCC_YUV422V;
    }
    else if ( 0 == msdk_stricmp(strInput, MSDK_STRING("yuv444")) )
    {
        fourcc = MFX_FOURCC_YUV444;
    }
    else if ( 0 == msdk_stricmp(strInput, MSDK_STRING("p010")) )
    {
        fourcc = MFX_FOURCC_P010;
    }
    else if ( 0 == msdk_stricmp(strInput, MSDK_STRING("p210")) )
    {
        fourcc = MFX_FOURCC_P210;
    }
    else if ( 0 == msdk_stricmp(strInput, MSDK_STRING("nv16")) )
    {
        fourcc = MFX_FOURCC_NV16;
    }
    else if ( 0 == msdk_stricmp(strInput, MSDK_STRING("a2rgb10")) )
    {
        fourcc = MFX_FOURCC_A2RGB10;
    }
    else if ( 0 == msdk_stricmp(strInput, MSDK_STRING("uyvy")) )
    {
        fourcc = MFX_FOURCC_UYVY;
    }
    else if (0 == msdk_stricmp(strInput, MSDK_STRING("i420")))
    {
        fourcc = MFX_FOURCC_I420;
    }

    return fourcc;

} // mfxU32 Str2FourCC( msdk_char* strInput )


eROIMode Str2ROIMode( msdk_char* strInput )
{
    eROIMode mode;

    if ( 0 == msdk_strcmp(strInput, MSDK_STRING("var_to_fix")) )
    {
        mode = ROI_VAR_TO_FIX;
    }
    else if ( 0 == msdk_strcmp(strInput, MSDK_STRING("var_to_var")) )
    {
        mode = ROI_VAR_TO_VAR;
    }
    else if ( 0 == msdk_strcmp(strInput, MSDK_STRING("fix_to_var")) )
    {
        mode = ROI_FIX_TO_VAR;
    }
    else
    {
        mode = ROI_FIX_TO_FIX; // default mode
    }

    return mode;

} // eROIMode Str2ROIMode( msdk_char* strInput )


static mfxU16 Str2IOpattern( msdk_char* strInput )
{
    mfxU16 IOPattern = 0;

    if ( 0 == msdk_strcmp(strInput, MSDK_STRING("d3d_to_d3d")) )
    {
        IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY|MFX_IOPATTERN_OUT_VIDEO_MEMORY;
    }
    else if ( 0 == msdk_strcmp(strInput, MSDK_STRING("d3d_to_sys")) )
    {
        IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY|MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
    }
    else if ( 0 == msdk_strcmp(strInput, MSDK_STRING("sys_to_d3d")) )
    {
        IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY|MFX_IOPATTERN_OUT_VIDEO_MEMORY;
    }
    else if ( 0 == msdk_strcmp(strInput, MSDK_STRING("sys_to_sys")) )
    {
        IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY|MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
    }
    return IOPattern;

} // static mfxU16 Str2IOpattern( msdk_char* strInput )

mfxStatus vppParseResetPar(msdk_char* strInput[], mfxU8 nArgNum, mfxU8& curArg, sInputParams* pParams, mfxU32 paramID, sFiltersParam* pDefaultFiltersParam)
{
    MSDK_CHECK_POINTER(pParams,  MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(strInput, MFX_ERR_NULL_PTR);

    sOwnFrameInfo info = pParams->frameInfoIn.back();
    pParams->frameInfoIn.push_back(info);
    info = pParams->frameInfoOut.back();
    pParams->frameInfoOut.push_back(info);

    pParams->deinterlaceParam.push_back(    *pDefaultFiltersParam->pDIParam            );
    pParams->denoiseParam.push_back(        *pDefaultFiltersParam->pDenoiseParam       );
    pParams->detailParam.push_back(         *pDefaultFiltersParam->pDetailParam        );
    pParams->procampParam.push_back(        *pDefaultFiltersParam->pProcAmpParam       );
    pParams->frcParam.push_back(            *pDefaultFiltersParam->pFRCParam           );
    pParams->multiViewParam.push_back(      *pDefaultFiltersParam->pMultiViewParam     );
    pParams->gamutParam.push_back(          *pDefaultFiltersParam->pGamutParam         );
    pParams->tccParam.push_back(            *pDefaultFiltersParam->pClrSaturationParam );
    pParams->aceParam.push_back(            *pDefaultFiltersParam->pContrastParam      );
    pParams->steParam.push_back(            *pDefaultFiltersParam->pSkinParam          );
    pParams->istabParam.push_back(          *pDefaultFiltersParam->pImgStabParam       );
    pParams->videoSignalInfoParam.push_back(*pDefaultFiltersParam->pVideoSignalInfo    );
    pParams->mirroringParam.push_back(      *pDefaultFiltersParam->pMirroringParam     );
    pParams->rotate.push_back(               0                                         );

    mfxU32 readData;
    mfxU32 ioStatus;

    for (mfxU8& i = curArg; i < nArgNum; i++ )
    {
        MSDK_CHECK_POINTER(strInput[i], MFX_ERR_NULL_PTR);
        {
            if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-o")))
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;

                pParams->strDstFiles.push_back(strInput[i]);
                pParams->isOutput = true;
            }
            //-----------------------------------------------------------------------------------
            //                   Video Enhancement Algorithms
            //-----------------------------------------------------------------------------------
            else if ( 0 == msdk_strcmp(strInput[i], MSDK_STRING("-ssinr")) )
            {
                VAL_CHECK(1 + i == nArgNum);

                pParams->videoSignalInfoParam[paramID].mode = VPP_FILTER_ENABLED_CONFIGURED;

                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->videoSignalInfoParam[paramID].In.NominalRange);
            }
            else if ( 0 == msdk_strcmp(strInput[i], MSDK_STRING("-dsinr")) )
            {
                VAL_CHECK(1 + i == nArgNum);

                pParams->videoSignalInfoParam[paramID].mode = VPP_FILTER_ENABLED_CONFIGURED;

                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->videoSignalInfoParam[paramID].Out.NominalRange);
            }
            else if ( 0 == msdk_strcmp(strInput[i], MSDK_STRING("-ssitm")) )
            {
                VAL_CHECK(1 + i == nArgNum);

                pParams->videoSignalInfoParam[paramID].mode = VPP_FILTER_ENABLED_CONFIGURED;

                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->videoSignalInfoParam[paramID].In.TransferMatrix);
            }
            else if ( 0 == msdk_strcmp(strInput[i], MSDK_STRING("-dsitm")) )
            {
                VAL_CHECK(1 + i == nArgNum);

                pParams->videoSignalInfoParam[paramID].mode = VPP_FILTER_ENABLED_CONFIGURED;

                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->videoSignalInfoParam[paramID].Out.TransferMatrix);
            }
            else if ( 0 == msdk_strcmp(strInput[i], MSDK_STRING("-mirror")) )
            {
                VAL_CHECK(1 + i == nArgNum);

                pParams->mirroringParam[paramID].mode = VPP_FILTER_ENABLED_CONFIGURED;

                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->mirroringParam[paramID].Type);
            }
            else if ( 0 == msdk_strcmp(strInput[i], MSDK_STRING("-sw")) )
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->frameInfoIn.back().nWidth);
            }
            else if ( 0 == msdk_stricmp(strInput[i], MSDK_STRING("-dw")) )
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->frameInfoOut.back().nWidth);
            }
            else if ( 0 == msdk_strcmp(strInput[i], MSDK_STRING("-sh")) )
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->frameInfoIn.back().nHeight);
            }
            else if ( 0 == msdk_stricmp(strInput[i], MSDK_STRING("-dh")) )
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->frameInfoOut.back().nHeight);
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-denoise")))
            {
                pParams->denoiseParam[paramID].mode = VPP_FILTER_ENABLED_DEFAULT;

                if( i+1 < nArgNum )
                {
                    ioStatus = msdk_sscanf(strInput[i+1], MSDK_STRING("%hd"), reinterpret_cast<short int *>(&readData));
                    if ( ioStatus > 0 )
                    {
                        pParams->denoiseParam[paramID].factor = (mfxU16)readData;
                        pParams->denoiseParam[paramID].mode   = VPP_FILTER_ENABLED_CONFIGURED;
                        i++;
                    }
                }
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-di_mode")))
            {
                pParams->deinterlaceParam[paramID].mode = VPP_FILTER_ENABLED_DEFAULT;

                if( i+1 < nArgNum )
                {
                    ioStatus = msdk_sscanf(strInput[i+1], MSDK_STRING("%hd"), reinterpret_cast<short int *>(&readData));
                    if ( ioStatus > 0 )
                    {
                        pParams->deinterlaceParam[paramID].algorithm = (mfxU16)readData;
                        pParams->deinterlaceParam[paramID].mode   = VPP_FILTER_ENABLED_CONFIGURED;
                        i++;
                    }
                }
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-tc_pattern")))
            {
                if( i+1 < nArgNum )
                {
                    ioStatus = msdk_sscanf(strInput[i+1], MSDK_STRING("%hd"), reinterpret_cast<short int *>(&readData));
                    if ( ioStatus > 0 )
                    {
                        pParams->deinterlaceParam[paramID].tc_pattern   = (mfxU16)readData;
                        i++;
                    }
                }
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-tc_pos")))
            {
                //pParams->deinterlaceParam.mode = VPP_FILTER_ENABLED_DEFAULT;

                if( i+1 < nArgNum )
                {
                    ioStatus = msdk_sscanf(strInput[i+1], MSDK_STRING("%hd"), reinterpret_cast<short int *>(&readData));
                    if ( ioStatus > 0 )
                    {
                        pParams->deinterlaceParam[paramID].tc_pos   = (mfxU16)readData;
                        i++;
                    }
                }
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-detail")))
            {
                pParams->detailParam[paramID].mode = VPP_FILTER_ENABLED_DEFAULT;

                if( i+1 < nArgNum )
                {
                    ioStatus = msdk_sscanf(strInput[i+1], MSDK_STRING("%hd"), reinterpret_cast<short int *>(&readData));
                    if ( ioStatus > 0 )
                    {
                        pParams->detailParam[paramID].factor = (mfxU16)readData;
                        pParams->detailParam[paramID].mode   = VPP_FILTER_ENABLED_CONFIGURED;
                        i++;
                    }
                }
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-rotate")))
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->rotate[paramID]);
            }
            // different modes of MFX FRC
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-frc:advanced")))
            {
                pParams->frcParam[paramID].mode = VPP_FILTER_ENABLED_CONFIGURED;
                pParams->frcParam[paramID].algorithm = MFX_FRCALGM_DISTRIBUTED_TIMESTAMP;
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-frc:interp")))
            {
                pParams->frcParam[paramID].mode = VPP_FILTER_ENABLED_CONFIGURED;
                pParams->frcParam[paramID].algorithm = MFX_FRCALGM_FRAME_INTERPOLATION;
            }
            //---------------------------------------------
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-pa_hue")))
            {
                pParams->procampParam[paramID].mode = VPP_FILTER_ENABLED_CONFIGURED;
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%lf"), &pParams->procampParam[paramID].hue);
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-pa_bri")))
            {
                pParams->procampParam[paramID].mode = VPP_FILTER_ENABLED_CONFIGURED;
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%lf"), &pParams->procampParam[paramID].brightness);
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-pa_con")))
            {
                pParams->procampParam[paramID].mode = VPP_FILTER_ENABLED_CONFIGURED;
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%lf"), &pParams->procampParam[paramID].contrast);
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-pa_sat")))
            {
                pParams->procampParam[paramID].mode = VPP_FILTER_ENABLED_CONFIGURED;
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%lf"), &pParams->procampParam[paramID].saturation);
            }

            //MSDK 3.0
            else if(0 == msdk_strcmp(strInput[i], MSDK_STRING("-gamut:compression")))
            {
                pParams->gamutParam[paramID].mode = VPP_FILTER_ENABLED_CONFIGURED;
            }
            else if(0 == msdk_strcmp(strInput[i], MSDK_STRING("-gamut:bt709")))
            {
                pParams->gamutParam[paramID].bBT709 = true;
            }
            else if( 0 == msdk_strcmp(strInput[i], MSDK_STRING("-view:count")) )
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;

                mfxU16 viewCount;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &viewCount);
                if( viewCount > 1 )
                {
                    pParams->multiViewParam[paramID].viewCount = (mfxU16)viewCount;
                    pParams->multiViewParam[paramID].mode = VPP_FILTER_ENABLED_CONFIGURED;
                }
            }
            //---------------------------------------------
            // MSDK API 1.5
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-istab")))
            {
                pParams->istabParam[paramID].mode = VPP_FILTER_ENABLED_DEFAULT;

                if( i+1 < nArgNum )
                {
                    ioStatus = msdk_sscanf(strInput[i+1], MSDK_STRING("%hd"), reinterpret_cast<short int *>(&readData));
                    if ( ioStatus > 0 )
                    {
                        pParams->istabParam[paramID].istabMode = (mfxU8)readData;
                        pParams->istabParam[paramID].mode    = VPP_FILTER_ENABLED_CONFIGURED;
                        i++;

                        if( pParams->istabParam[paramID].istabMode != 1 && pParams->istabParam[paramID].istabMode != 2 )
                        {
                            vppPrintHelp(strInput[0], MSDK_STRING("Invalid IStab configuration"));
                            return MFX_ERR_UNSUPPORTED;
                        }
                    }
                }
            }
            //---------------------------------------------
            // IECP
            else if(0 == msdk_strcmp(strInput[i], MSDK_STRING("-ace")))
            {
                pParams->aceParam[paramID].mode = VPP_FILTER_ENABLED_DEFAULT;
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-ste")))
            {
                pParams->steParam[paramID].mode = VPP_FILTER_ENABLED_DEFAULT;

                if( i+1 < nArgNum )
                {
                    ioStatus = msdk_sscanf(strInput[i+1], MSDK_STRING("%hd"), reinterpret_cast<short int *>(&readData));
                    if ( ioStatus > 0 )
                    {
                        pParams->steParam[paramID].SkinToneFactor = (mfxU8)readData;
                        pParams->steParam[paramID].mode           = VPP_FILTER_ENABLED_CONFIGURED;
                        i++;
                    }
                }
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-tcc:red")))
            {
                pParams->tccParam[paramID].mode = VPP_FILTER_ENABLED_CONFIGURED;
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%lf"), reinterpret_cast<double *>(&pParams->tccParam[paramID].Red));
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-tcc:green")))
            {
                pParams->tccParam[paramID].mode = VPP_FILTER_ENABLED_CONFIGURED;
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%lf"), reinterpret_cast<double *>(&pParams->tccParam[paramID].Green));
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-tcc:blue")))
            {
                pParams->tccParam[paramID].mode = VPP_FILTER_ENABLED_CONFIGURED;
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%lf"), reinterpret_cast<double *>(&pParams->tccParam[paramID].Blue));
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-tcc:magenta")))
            {
                pParams->tccParam[paramID].mode = VPP_FILTER_ENABLED_CONFIGURED;
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%lf"), reinterpret_cast<double *>(&pParams->tccParam[paramID].Magenta));
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-tcc:yellow")))
            {
                pParams->tccParam[paramID].mode = VPP_FILTER_ENABLED_CONFIGURED;
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%lf"), reinterpret_cast<double *>(&pParams->tccParam[paramID].Yellow));
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-tcc:cyan")))
            {
                pParams->tccParam[paramID].mode = VPP_FILTER_ENABLED_CONFIGURED;
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%lf"), reinterpret_cast<double *>(&pParams->tccParam[paramID].Cyan));
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-reset_end")))
            {
                break;
            }
            else
            {
                msdk_printf(MSDK_STRING("Unknow reset option: %s\n"), strInput[i]);

                return MFX_ERR_UNKNOWN;
            }
        }
    }

    return MFX_ERR_NONE;

} // mfxStatus vppParseResetPar( ... )

mfxStatus vppParseInputString(msdk_char* strInput[], mfxU8 nArgNum, sInputParams* pParams, sFiltersParam* pDefaultFiltersParam)
{
    MSDK_CHECK_POINTER(pParams,  MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(strInput, MFX_ERR_NULL_PTR);

    mfxU32 readData;
    mfxU32 ioStatus;
    if (nArgNum < 4)
    {
        vppPrintHelp(strInput[0], MSDK_STRING("Not enough parameters"));

        return MFX_ERR_MORE_DATA;
    }

    pParams->frameInfoIn.back().CropX = 0;
    pParams->frameInfoIn.back().CropY = 0;
    pParams->frameInfoIn.back().CropW = NOT_INIT_VALUE;
    pParams->frameInfoIn.back().CropH = NOT_INIT_VALUE;
    // zeroize destination crops
    pParams->frameInfoOut.back().CropX = 0;
    pParams->frameInfoOut.back().CropY = 0;
    pParams->frameInfoOut.back().CropW = NOT_INIT_VALUE;
    pParams->frameInfoOut.back().CropH = NOT_INIT_VALUE;
    pParams->numStreams=1;

    for (mfxU8 i = 1; i < nArgNum; i++ )
    {
        MSDK_CHECK_POINTER(strInput[i], MFX_ERR_NULL_PTR);
        {
            if ( 0 == msdk_strcmp(strInput[i], MSDK_STRING("-ssinr")) )
            {
                VAL_CHECK(1 + i == nArgNum);

                pParams->videoSignalInfoParam[0].mode = VPP_FILTER_ENABLED_CONFIGURED;

                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->videoSignalInfoParam[0].In.NominalRange);
            }
            else if ( 0 == msdk_strcmp(strInput[i], MSDK_STRING("-dsinr")) )
            {
                VAL_CHECK(1 + i == nArgNum);

                pParams->videoSignalInfoParam[0].mode = VPP_FILTER_ENABLED_CONFIGURED;

                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->videoSignalInfoParam[0].Out.NominalRange);
            }
            else if ( 0 == msdk_strcmp(strInput[i], MSDK_STRING("-ssitm")) )
            {
                VAL_CHECK(1 + i == nArgNum);

                pParams->videoSignalInfoParam[0].mode = VPP_FILTER_ENABLED_CONFIGURED;

                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->videoSignalInfoParam[0].In.TransferMatrix);
            }
            else if ( 0 == msdk_strcmp(strInput[i], MSDK_STRING("-dsitm")) )
            {
                VAL_CHECK(1 + i == nArgNum);

                pParams->videoSignalInfoParam[0].mode = VPP_FILTER_ENABLED_CONFIGURED;

                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->videoSignalInfoParam[0].Out.TransferMatrix);
            }
            else if ( 0 == msdk_strcmp(strInput[i], MSDK_STRING("-mirror")) )
            {
                VAL_CHECK(1 + i == nArgNum);

                pParams->mirroringParam[0].mode = VPP_FILTER_ENABLED_CONFIGURED;

                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->mirroringParam[0].Type);
            }
            else if ( 0 == msdk_strcmp(strInput[i], MSDK_STRING("-gpu_copy")) )
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                pParams->bInitEx = true;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->GPUCopyValue);
            }
            else if ( 0 == msdk_strcmp(strInput[i], MSDK_STRING("-sw")) )
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->frameInfoIn[0].nWidth);
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-sh")))
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->frameInfoIn[0].nHeight);
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-scrX")))
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->frameInfoIn[0].CropX);
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-scrY")))
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->frameInfoIn[0].CropY);
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-scrW")))
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->frameInfoIn[0].CropW);
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-scrH")))
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->frameInfoIn[0].CropH);
            }
            else if(0 == msdk_strcmp(strInput[i], MSDK_STRING("-spic")))
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                mfxI16 tmp;
                msdk_sscanf(strInput[i], MSDK_STRING("%hd"), reinterpret_cast<short int *>(&tmp));
                pParams->frameInfoIn[0].PicStruct = GetPicStruct(static_cast<mfxI8>(tmp));
            }
            else if(0 == msdk_strcmp(strInput[i], MSDK_STRING("-sf")))
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%lf"), &pParams->frameInfoIn[0].dFrameRate);
            }
            else if (0 == msdk_stricmp(strInput[i], MSDK_STRING("-dw")) )
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->frameInfoOut[0].nWidth);
            }
            else if (0 == msdk_stricmp(strInput[i], MSDK_STRING("-dh")))
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->frameInfoOut[0].nHeight);
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-dcrX")))
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->frameInfoOut[0].CropX);
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-dcrY")))
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->frameInfoOut[0].CropY);
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-dcrW")))
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->frameInfoOut[0].CropW);
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-dcrH")))
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->frameInfoOut[0].CropH);
            }
            else if(0 == msdk_strcmp(strInput[i], MSDK_STRING("-dpic")))
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                mfxI16 tmp;
                msdk_sscanf(strInput[i], MSDK_STRING("%hd"), reinterpret_cast<short int *>(&tmp));
                pParams->frameInfoOut[0].PicStruct = GetPicStruct(static_cast<mfxI8>(tmp));
            }
            else if(0 == msdk_strcmp(strInput[i], MSDK_STRING("-df")))
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%lf"), &pParams->frameInfoOut[0].dFrameRate);
            }
            //-----------------------------------------------------------------------------------
            //                   Video Enhancement Algorithms
            //-----------------------------------------------------------------------------------
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-denoise")))
            {
                pParams->denoiseParam[0].mode = VPP_FILTER_ENABLED_DEFAULT;

                if( i+1 < nArgNum )
                {
                    ioStatus = msdk_sscanf(strInput[i+1], MSDK_STRING("%hd"), reinterpret_cast<short int *>(&readData));
                    if ( ioStatus > 0 )
                    {
                        pParams->denoiseParam[0].factor = (mfxU16)readData;
                        pParams->denoiseParam[0].mode   = VPP_FILTER_ENABLED_CONFIGURED;
                        i++;
                    }
                }
            }
            // aya: altenative and simple way to enable deinterlace
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-deinterlace")))
            {
                pParams->frameInfoOut[0].PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
                pParams->frameInfoIn[0].PicStruct  = MFX_PICSTRUCT_FIELD_TFF;

                if( i+1 < nArgNum )
                {
                    if(0 == msdk_strcmp(strInput[i+1], MSDK_STRING("bff")))
                    {
                        pParams->frameInfoOut[0].PicStruct = MFX_PICSTRUCT_FIELD_BFF;
                        i++;
                    }
                }
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-di_mode")))
            {
                pParams->deinterlaceParam[0].mode = VPP_FILTER_ENABLED_DEFAULT;

                if( i+1 < nArgNum )
                {
                    ioStatus = msdk_sscanf(strInput[i+1], MSDK_STRING("%hd"), reinterpret_cast<short int *>(&readData));
                    if ( ioStatus > 0 )
                    {
                        pParams->deinterlaceParam[0].algorithm = (mfxU16)readData;
                        pParams->deinterlaceParam[0].mode   = VPP_FILTER_ENABLED_CONFIGURED;
                        i++;
                    }
                }
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-tc_pattern")))
            {
                if( i+1 < nArgNum )
                {
                    ioStatus = msdk_sscanf(strInput[i+1], MSDK_STRING("%hd"), reinterpret_cast<short int *>(&readData));
                    if ( ioStatus > 0 )
                    {
                        pParams->deinterlaceParam[0].tc_pattern   = (mfxU16)readData;
                        i++;
                    }
                }
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-tc_pos")))
            {
                //pParams->deinterlaceParam.mode = VPP_FILTER_ENABLED_DEFAULT;

                if( i+1 < nArgNum )
                {
                    ioStatus = msdk_sscanf(strInput[i+1], MSDK_STRING("%hd"), reinterpret_cast<short int *>(&readData));
                    if ( ioStatus > 0 )
                    {
                        pParams->deinterlaceParam[0].tc_pos   = (mfxU16)readData;
                        i++;
                    }
                }
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-detail")))
            {
                pParams->detailParam[0].mode = VPP_FILTER_ENABLED_DEFAULT;

                if( i+1 < nArgNum )
                {
                    ioStatus = msdk_sscanf(strInput[i+1], MSDK_STRING("%hd"), reinterpret_cast<short int *>(&readData));
                    if ( ioStatus > 0 )
                    {
                        pParams->detailParam[0].factor = (mfxU16)readData;
                        pParams->detailParam[0].mode   = VPP_FILTER_ENABLED_CONFIGURED;
                        i++;
                    }
                }
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-rotate")))
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->rotate[0]);
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-scaling_mode")))
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                pParams->bScaling = true;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->scalingMode);
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-composite")))
            {
                if( i+1 < nArgNum )
                {
                    if (ParseCompositionParfile(strInput[i+1], pParams) != MFX_ERR_NONE)
                    {
                        vppPrintHelp(strInput[0], MSDK_STRING("Parfile for -composite has invalid data or cannot be opened\n"));
                        return MFX_ERR_UNSUPPORTED;
                    }
                    pParams->compositionParam.mode = VPP_FILTER_ENABLED_CONFIGURED;
                    i++;
                }
            }
            // different modes of MFX FRC
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-frc:advanced")))
            {
                pParams->frcParam[0].mode = VPP_FILTER_ENABLED_CONFIGURED;
                pParams->frcParam[0].algorithm = MFX_FRCALGM_DISTRIBUTED_TIMESTAMP;
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-frc:interp")))
            {
                pParams->frcParam[0].mode = VPP_FILTER_ENABLED_CONFIGURED;
                pParams->frcParam[0].algorithm = MFX_FRCALGM_FRAME_INTERPOLATION;
            }
            //---------------------------------------------
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-pa_hue")))
            {
                pParams->procampParam[0].mode = VPP_FILTER_ENABLED_CONFIGURED;
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%lf"), &pParams->procampParam[0].hue);
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-pa_bri")))
            {
                pParams->procampParam[0].mode = VPP_FILTER_ENABLED_CONFIGURED;
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%lf"), &pParams->procampParam[0].brightness);
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-pa_con")))
            {
                pParams->procampParam[0].mode = VPP_FILTER_ENABLED_CONFIGURED;
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%lf"), &pParams->procampParam[0].contrast);
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-pa_sat")))
            {
                pParams->procampParam[0].mode = VPP_FILTER_ENABLED_CONFIGURED;
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%lf"), &pParams->procampParam[0].saturation);
            }

            //MSDK 3.0
            else if(0 == msdk_strcmp(strInput[i], MSDK_STRING("-gamut:compression")))
            {
                pParams->gamutParam[0].mode = VPP_FILTER_ENABLED_CONFIGURED;
            }
            else if(0 == msdk_strcmp(strInput[i], MSDK_STRING("-gamut:bt709")))
            {
                pParams->gamutParam[0].bBT709 = true;
            }
            else if( 0 == msdk_strcmp(strInput[i], MSDK_STRING("-view:count")) )
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;

                mfxU16 viewCount;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &viewCount);
                if( viewCount > 1 )
                {
                    pParams->multiViewParam[0].viewCount = (mfxU16)viewCount;
                    pParams->multiViewParam[0].mode = VPP_FILTER_ENABLED_CONFIGURED;
                }
            }
            //---------------------------------------------
            // MSDK API 1.5
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-istab")))
            {
                pParams->istabParam[0].mode = VPP_FILTER_ENABLED_DEFAULT;

                if( i+1 < nArgNum )
                {
                    ioStatus = msdk_sscanf(strInput[i+1], MSDK_STRING("%hd"), reinterpret_cast<short int *>(&readData));
                    if ( ioStatus > 0 )
                    {
                        pParams->istabParam[0].istabMode = (mfxU8)readData;
                        pParams->istabParam[0].mode    = VPP_FILTER_ENABLED_CONFIGURED;
                        i++;

                        if( pParams->istabParam[0].istabMode != 1 && pParams->istabParam[0].istabMode != 2 )
                        {
                            vppPrintHelp(strInput[0], MSDK_STRING("Invalid IStab configuration"));
                            return MFX_ERR_UNSUPPORTED;
                        }
                    }
                }
            }
            //---------------------------------------------
            // IECP
            else if(0 == msdk_strcmp(strInput[i], MSDK_STRING("-ace")))
            {
                pParams->aceParam[0].mode = VPP_FILTER_ENABLED_DEFAULT;
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-ste")))
            {
                pParams->steParam[0].mode = VPP_FILTER_ENABLED_DEFAULT;

                if( i+1 < nArgNum )
                {
                    ioStatus = msdk_sscanf(strInput[i+1], MSDK_STRING("%hd"), reinterpret_cast<short int *>(&readData));
                    if ( ioStatus > 0 )
                    {
                        pParams->steParam[0].SkinToneFactor = (mfxU8)readData;
                        pParams->steParam[0].mode           = VPP_FILTER_ENABLED_CONFIGURED;
                        i++;
                    }
                }
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-tcc:red")))
            {
                pParams->tccParam[0].mode = VPP_FILTER_ENABLED_CONFIGURED;
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%lf"), reinterpret_cast<double *>(&pParams->tccParam[0].Red));
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-tcc:green")))
            {
                pParams->tccParam[0].mode = VPP_FILTER_ENABLED_CONFIGURED;
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%lf"), reinterpret_cast<double *>(&pParams->tccParam[0].Green));
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-tcc:blue")))
            {
                pParams->tccParam[0].mode = VPP_FILTER_ENABLED_CONFIGURED;
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%lf"), reinterpret_cast<double *>(&pParams->tccParam[0].Blue));
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-tcc:magenta")))
            {
                pParams->tccParam[0].mode = VPP_FILTER_ENABLED_CONFIGURED;
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%lf"), reinterpret_cast<double *>(&pParams->tccParam[0].Magenta));
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-tcc:yellow")))
            {
                pParams->tccParam[0].mode = VPP_FILTER_ENABLED_CONFIGURED;
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%lf"), reinterpret_cast<double *>(&pParams->tccParam[0].Yellow));
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-tcc:cyan")))
            {
                pParams->tccParam[0].mode = VPP_FILTER_ENABLED_CONFIGURED;
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%lf"), reinterpret_cast<double *>(&pParams->tccParam[0].Cyan));
            }
            //-----------------------------------------------------------------------------------
            //                   Region of Interest Testing
            //-----------------------------------------------------------------------------------
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-roi_check")))
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                pParams->roiCheckParam.mode = Str2ROIMode( strInput[i] );

                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hd"), reinterpret_cast<short int*>(&pParams->roiCheckParam.srcSeed));

                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hd"), reinterpret_cast<short int*>(&pParams->roiCheckParam.dstSeed));
            }
            //-----------------------------------------------------------------------------------
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-i")))
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_strncopy_s(pParams->strSrcFile, MSDK_MAX_FILENAME_LEN, strInput[i], MSDK_MAX_FILENAME_LEN - 1);
                pParams->strSrcFile[MSDK_MAX_FILENAME_LEN - 1] = 0;
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-o")))
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;

                pParams->strDstFiles.push_back(strInput[i]);
                pParams->isOutput = true;
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-pf")))
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_strncopy_s(pParams->strPerfFile, MSDK_MAX_FILENAME_LEN, strInput[i], MSDK_MAX_FILENAME_LEN - 1);
                pParams->strPerfFile[MSDK_MAX_FILENAME_LEN - 1] = 0;
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-scc")))
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                pParams->fccSource = pParams->frameInfoIn[0].FourCC
                    = Str2FourCC(strInput[i]);

                //if (MFX_FOURCC_I420 == pParams->frameInfoIn[0].FourCC)
                //{
                //    pParams->frameInfoIn[0].FourCC = MFX_FOURCC_YV12; // I420 input is implemented using YV12 internally
                //}

            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-dcc")))
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                pParams->frameInfoOut[0].FourCC = Str2FourCC( strInput[i] );
                pParams->forcedOutputFourcc = 0;
                if(MFX_FOURCC_I420 == pParams->frameInfoOut[0].FourCC || MFX_FOURCC_YV12 == pParams->frameInfoOut[0].FourCC)
                {
                    pParams->forcedOutputFourcc = pParams->frameInfoOut[0].FourCC;
                    pParams->frameInfoOut[0].FourCC = MFX_FOURCC_NV12; // I420 output is implemented using NV12 internally
                }
            }
            else if(0 == msdk_strcmp(strInput[i], MSDK_STRING("-dbitshift")))
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->frameInfoOut[0].Shift);
            }
            else if(0 == msdk_strcmp(strInput[i], MSDK_STRING("-dbitdepthluma")))
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->frameInfoOut[0].BitDepthLuma);
            }
            else if(0 == msdk_strcmp(strInput[i], MSDK_STRING("-dbitdepthchroma")))
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->frameInfoOut[0].BitDepthChroma);
            }
            else if(0 == msdk_strcmp(strInput[i], MSDK_STRING("-sbitshift")))
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->frameInfoIn[0].Shift);
            }
            else if(0 == msdk_strcmp(strInput[i], MSDK_STRING("-sbitdepthluma")))
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->frameInfoIn[0].BitDepthLuma);
            }
            else if(0 == msdk_strcmp(strInput[i], MSDK_STRING("-sbitdepthchroma")))
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->frameInfoIn[0].BitDepthChroma);
            }
            else if( 0 == msdk_strcmp(strInput[i], MSDK_STRING("-iopattern")) )
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                pParams->IOPattern = Str2IOpattern( strInput[i] );
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-lib")) )
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                if (0 == msdk_strcmp(strInput[i], MSDK_STRING("sw")) )
                    pParams->ImpLib = MFX_IMPL_SOFTWARE;
                else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("hw")) )
                {
                    pParams->ImpLib = MFX_IMPL_HARDWARE;
                }
            }
#if defined(D3D_SURFACES_SUPPORT)
            else if( 0 == msdk_strcmp(strInput[i], MSDK_STRING("-d3d")) )
            {
                pParams->IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY|MFX_IOPATTERN_OUT_VIDEO_MEMORY;
                pParams->ImpLib |= MFX_IMPL_VIA_D3D9;
            }
#endif
#if MFX_D3D11_SUPPORT
            else if( 0 == msdk_strcmp(strInput[i], MSDK_STRING("-d3d11")) )
            {
                pParams->IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY|MFX_IOPATTERN_OUT_VIDEO_MEMORY;
                pParams->ImpLib |= MFX_IMPL_VIA_D3D11;
            }
#endif
#ifdef LIBVA_SUPPORT
            else if( 0 == msdk_strcmp(strInput[i], MSDK_STRING("-vaapi")) || 0 == msdk_strcmp(strInput[i], MSDK_STRING("-d3d")) )
            {
                pParams->IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY|MFX_IOPATTERN_OUT_VIDEO_MEMORY;
                pParams->ImpLib |= MFX_IMPL_VIA_VAAPI;
            }
#endif
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-async")) )
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->asyncNum);

            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-perf_opt")) )
            {
                if (pParams->numFrames)
                    return MFX_ERR_UNKNOWN;

                VAL_CHECK(1 + i == nArgNum);
                pParams->bPerf = true;
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hd"), reinterpret_cast<short int*>(&pParams->numFrames));
                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hu"), &pParams->numRepeat);
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-pts_check")) )
            {
                pParams->ptsCheck = true;
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-pts_jump")) )
            {
                pParams->ptsJump = true;
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-pts_fr")) )
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%lf"), &pParams->ptsFR);
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-pts_advanced")) )
            {
                pParams->ptsAdvanced = true;
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-n")) )
            {
                if (pParams->bPerf)
                    return MFX_ERR_UNKNOWN;

                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_sscanf(strInput[i], MSDK_STRING("%hd"), reinterpret_cast<short int*>(&pParams->numFrames));

            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-p")))
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_strncopy_s(pParams->strPlgGuid, MSDK_MAX_FILENAME_LEN, strInput[i],MSDK_MAX_FILENAME_LEN-1);
                pParams->strPlgGuid[MSDK_MAX_FILENAME_LEN - 1] = 0;
                pParams->need_plugin = true;
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-plugin_guid")))
            {
                VAL_CHECK(1 + i == nArgNum);
                i++;
                msdk_strncopy_s(pParams->strPlgGuid, MSDK_MAX_FILENAME_LEN, strInput[i],MSDK_MAX_FILENAME_LEN-1);
                pParams->strPlgGuid[MSDK_MAX_FILENAME_LEN - 1] = 0; 
                pParams->need_plugin = true;
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-extapi")) )
            {
                pParams->use_extapi = true;
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-reset_start")) )
            {
                VAL_CHECK(1 + i == nArgNum);
                msdk_sscanf(strInput[i+1], MSDK_STRING("%hd"), reinterpret_cast<short int*>(&readData));
                i += 2;

                pParams->resetFrmNums.push_back((mfxU16)readData);

                if (MFX_ERR_NONE != vppParseResetPar(strInput, nArgNum, i, pParams, (mfxU32)pParams->resetFrmNums.size(), pDefaultFiltersParam))
                    return MFX_ERR_UNKNOWN;
            }
            else
            {
                msdk_printf(MSDK_STRING("Unknown option: %s\n"), strInput[i]);

                return MFX_ERR_UNKNOWN;
            }
        }
    }



    if ((pParams->ImpLib & MFX_IMPL_HARDWARE) && !(pParams->ImpLib & MFX_IMPL_VIA_D3D11))
    {
        pParams->ImpLib = MFX_IMPL_HARDWARE |
        #ifdef LIBVA_SUPPORT
                MFX_IMPL_VIA_VAAPI;
        #else
                MFX_IMPL_VIA_D3D9;
        #endif
    }

    std::vector<sOwnFrameInfo>::iterator it = pParams->frameInfoIn.begin();
    while(it != pParams->frameInfoIn.end())
    {
        if (NOT_INIT_VALUE == it->CropW)
        {
            it->CropW = it->nWidth;
        }

        if (NOT_INIT_VALUE == it->CropH)
        {
            it->CropH = it->nHeight;
        }
        it++;
    }

    it = pParams->frameInfoOut.begin();
    while(it != pParams->frameInfoOut.end())
    {
        if (NOT_INIT_VALUE == it->CropW)
        {
            it->CropW = it->nWidth;
        }

        if (NOT_INIT_VALUE == it->CropH)
        {
            it->CropH = it->nHeight;
        }
        it++;
    }

    if (0 == msdk_strlen(pParams->strSrcFile) && pParams->compositionParam.mode != VPP_FILTER_ENABLED_CONFIGURED)
    {
        vppPrintHelp(strInput[0], MSDK_STRING("Source file name not found"));
        return MFX_ERR_UNSUPPORTED;
    };

    if (1 != pParams->strDstFiles.size() && (pParams->resetFrmNums.size() + 1) != pParams->strDstFiles.size())
    {
        vppPrintHelp(strInput[0], MSDK_STRING("Destination file name should be declared once or for each parameter set (reset case)"));
        return MFX_ERR_UNSUPPORTED;
    };

    if (0 == pParams->IOPattern)
    {
        vppPrintHelp(strInput[0], MSDK_STRING("Incorrect IOPattern"));
        return MFX_ERR_UNSUPPORTED;
    }

    if ((pParams->ImpLib & MFX_IMPL_SOFTWARE) && (pParams->IOPattern & (MFX_IOPATTERN_IN_VIDEO_MEMORY|MFX_IOPATTERN_OUT_VIDEO_MEMORY)))
    {
        msdk_printf(MSDK_STRING("Warning: IOPattern has been reset to 'sys_to_sys' mode because software library implementation is selected."));
        pParams->IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY|MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
    }



return MFX_ERR_NONE;

} // mfxStatus vppParseInputString( ... )

bool CheckInputParams(msdk_char* strInput[], sInputParams* pParams )
{
    // Setting  default width and height if it was omitted. For composition case parameters should be define explicitely
    if (pParams->frameInfoOut[0].nWidth == 0)
    {
        if (pParams->compositionParam.mode == VPP_FILTER_ENABLED_CONFIGURED)
        {
            vppPrintHelp(strInput[0], MSDK_STRING("ERROR: Destination width should be set explicitely in case of composition mode.\n"));
            return false;
        }
        pParams->frameInfoOut[0].nWidth = pParams->frameInfoIn[0].nWidth;
        pParams->frameInfoOut[0].CropW = pParams->frameInfoIn[0].CropW;
        pParams->frameInfoOut[0].CropX = 0;
    }

    if (pParams->frameInfoOut[0].nHeight == 0)
    {
        if (pParams->compositionParam.mode == VPP_FILTER_ENABLED_CONFIGURED)
        {
            vppPrintHelp(strInput[0], MSDK_STRING("ERROR: Destination height should be set explicitely in case of composition mode.\n"));
            return false;
        }
        pParams->frameInfoOut[0].nHeight = pParams->frameInfoIn[0].nHeight;
        pParams->frameInfoOut[0].CropH = pParams->frameInfoIn[0].CropH;
        pParams->frameInfoOut[0].CropY = 0;
    }

    // Checking other parameters
    if (0 == pParams->asyncNum)
    {
        vppPrintHelp(strInput[0], MSDK_STRING("Incompatible parameters: [ayncronous number must exceed 0]\n"));
        return false;
    }

    for (mfxU32 i = 0; i < pParams->rotate.size(); i++)
    {
        if (pParams->rotate[i] != 0 && pParams->rotate[i] != 90 && pParams->rotate[i] != 180 && pParams->rotate[i] != 270)
        {
            vppPrintHelp(strInput[0], MSDK_STRING("Invalid -rotate parameter: supported values 0, 90, 180, 270\n"));
            return false;
        }
    }

    for (mfxU32 i = 0; i < pParams->numStreams; i++)
    {
        const mfxVPPCompInputStream& is = pParams->compositionParam.streamInfo[i].compStream;

        if ((pParams->frameInfoOut[0].nWidth < is.DstW + is.DstX) ||
            (pParams->frameInfoOut[0].nHeight < is.DstH + is.DstY))
        {
            vppPrintHelp(strInput[0], MSDK_STRING("One of composing frames cannot fit into destination frame.\n"));
            return false;
        }
    }

    return true;

} // bool CheckInputParams(msdk_char* strInput[], sInputVppParams* pParams )

// trim from start
static inline std::string &ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
    return ltrim(rtrim(s));
}

void getPair(std::string line, std::string &key, std::string &value)
{
    std::istringstream iss(line);
    getline(iss, key,   '=');
    getline(iss, value, '=');
    trim(key);
    trim(value);
}

mfxStatus ParseCompositionParfile(const msdk_char* parFileName, sInputParams* pParams)
{
    mfxStatus sts = MFX_ERR_NONE;
    if(msdk_strlen(parFileName) == 0)
        return MFX_ERR_UNKNOWN;

    MSDK_ZERO_MEMORY(pParams->inFrameInfo);

    std::string line;
    std::string key, value;
    mfxU8 nStreamInd = 0;
    mfxU8 firstStreamFound = 0;
    std::ifstream stream(parFileName);
    if (stream.fail())
        return MFX_ERR_UNKNOWN;

    while (getline(stream, line) && nStreamInd < MAX_INPUT_STREAMS)
    {
        getPair(line, key, value);
        if (key.compare("width") == 0)
        {
            pParams->inFrameInfo[nStreamInd].nWidth = (mfxU16) MSDK_ALIGN16(atoi(value.c_str()));
        }
        else if (key.compare("height") == 0)
        {
            pParams->inFrameInfo[nStreamInd].nHeight = (MFX_PICSTRUCT_PROGRESSIVE == pParams->inFrameInfo[nStreamInd].PicStruct)?
                (mfxU16) MSDK_ALIGN16(atoi(value.c_str())) : (mfxU16) MSDK_ALIGN32(atoi(value.c_str()));
        }
        else if (key.compare("cropx") == 0)
        {
            pParams->inFrameInfo[nStreamInd].CropX = (mfxU16) atoi(value.c_str());
        }
        else if (key.compare("cropy") == 0)
        {
            pParams->inFrameInfo[nStreamInd].CropY = (mfxU16) atoi(value.c_str());
        }
        else if (key.compare("cropw") == 0)
        {
            pParams->inFrameInfo[nStreamInd].CropW = (mfxU16) atoi(value.c_str());
        }
        else if (key.compare("croph") == 0)
        {
            pParams->inFrameInfo[nStreamInd].CropH = (mfxU16) atoi(value.c_str());
        }
        else if (key.compare("framerate") == 0)
        {
            pParams->inFrameInfo[nStreamInd].dFrameRate = (mfxF64) atof(value.c_str());
        }
        else if (key.compare("fourcc") == 0)
        {
            const mfxU16 len_size = 5;
            msdk_char fourcc[len_size];
            for (mfxU16 i = 0; i < (value.size() > len_size ? len_size : value.size()); i++)
                fourcc[i] = value.at(i);
            fourcc[len_size-1]=0;
            pParams->inFrameInfo[nStreamInd].FourCC = Str2FourCC(fourcc);

            if(!pParams->inFrameInfo[nStreamInd].FourCC)
            {
                msdk_printf(MSDK_STRING("Invalid fourcc parameter in par file: %s\n"),fourcc);
                return MFX_ERR_INVALID_VIDEO_PARAM;
            }
        }
        else if (key.compare("picstruct") == 0)
        {
            pParams->inFrameInfo[nStreamInd].PicStruct = GetPicStruct((mfxI8)atoi(value.c_str()));
        }
        else if (key.compare("dstx") == 0)
        {
            pParams->compositionParam.streamInfo[nStreamInd].compStream.DstX = (mfxU16) atoi(value.c_str());
        }
        else if (key.compare("dsty") == 0)
        {
            pParams->compositionParam.streamInfo[nStreamInd].compStream.DstY = (mfxU16) atoi(value.c_str());
        }
        else if (key.compare("dstw") == 0)
        {
            pParams->compositionParam.streamInfo[nStreamInd].compStream.DstW = (mfxU16) atoi(value.c_str());
        }
        else if (key.compare("dsth") == 0)
        {
            pParams->compositionParam.streamInfo[nStreamInd].compStream.DstH = (mfxU16) atoi(value.c_str());
        }
        else if (key.compare("GlobalAlphaEnable") == 0)
        {
            pParams->compositionParam.streamInfo[nStreamInd].compStream.GlobalAlphaEnable = (mfxU16) atoi(value.c_str());
        }
        else if (key.compare("GlobalAlpha") == 0)
        {
            pParams->compositionParam.streamInfo[nStreamInd].compStream.GlobalAlpha = (mfxU16) atoi(value.c_str());
        }
        else if (key.compare("PixelAlphaEnable") == 0)
        {
            pParams->compositionParam.streamInfo[nStreamInd].compStream.PixelAlphaEnable = (mfxU16) atoi(value.c_str());
        }
        else if (key.compare("LumaKeyEnable") == 0)
        {
            pParams->compositionParam.streamInfo[nStreamInd].compStream.LumaKeyEnable = (mfxU16) atoi(value.c_str());
        }
        else if (key.compare("LumaKeyMin") == 0)
        {
            pParams->compositionParam.streamInfo[nStreamInd].compStream.LumaKeyMin = (mfxU16) atoi(value.c_str());
        }
        else if (key.compare("LumaKeyMax") == 0)
        {
            pParams->compositionParam.streamInfo[nStreamInd].compStream.LumaKeyMax = (mfxU16) atoi(value.c_str());
        }
        else if ((key.compare("stream") == 0 || key.compare("primarystream") == 0) && nStreamInd < (MAX_INPUT_STREAMS - 1))
        {
            const mfxU16 len_size = MSDK_MAX_FILENAME_LEN - 1;

            if (firstStreamFound == 1)
            {
                nStreamInd ++;
            }
            else
            {
                nStreamInd = 0;
                firstStreamFound = 1;
            }
            pParams->inFrameInfo[nStreamInd].CropX = 0;
            pParams->inFrameInfo[nStreamInd].CropY = 0;
            pParams->inFrameInfo[nStreamInd].CropW = NOT_INIT_VALUE;
            pParams->inFrameInfo[nStreamInd].CropH = NOT_INIT_VALUE;
            mfxU16 i = 0;
            for (; i < (value.size() > len_size ? len_size : value.size()); i++)
                pParams->compositionParam.streamInfo[nStreamInd].streamName[i] = value.at(i);
            pParams->compositionParam.streamInfo[nStreamInd].streamName[i]=0;
            pParams->inFrameInfo[nStreamInd].dFrameRate = 30;
            pParams->inFrameInfo[nStreamInd].PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
        }
    }

    pParams->numStreams = nStreamInd + 1;

    for(int i=0;i<pParams->numStreams;i++)
    {
        if(!pParams->inFrameInfo[i].FourCC)
        {
            msdk_printf(MSDK_STRING("Fourcc parameter of stream %d in par file is invalid or missing.\n"),i);
            return MFX_ERR_INVALID_VIDEO_PARAM;
        }
    }
    return sts;
}

/* EOF */
