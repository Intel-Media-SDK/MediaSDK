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
#include "plugin_utils.h"

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <psapi.h>
#include <d3d9.h>
#include "d3d_allocator.h"
#include "d3d11_allocator.h"
#else
#include <stdarg.h>
#include "vaapi_allocator.h"
#endif

#include "sysmem_allocator.h"
#include "transcode_utils.h"

#include "version.h"

#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <cctype>
#include <iterator>
#include <cstring>
#include <cstdlib>
#include <algorithm>

using namespace TranscodingSample;

// parsing defines
#define IS_SEPARATOR(ch)  ((ch) <= ' ' || (ch) == '=')
#define VAL_CHECK(val, argIdx, argName) \
{ \
    if (val) \
    { \
        PrintError(MSDK_STRING("Input argument number %d \"%s\" require more parameters"), argIdx, argName); \
        return MFX_ERR_UNSUPPORTED;\
    } \
}

#define SIZE_CHECK(cond) \
{ \
    if (cond) \
    { \
        PrintError(MSDK_STRING("Buffer is too small")); \
        return MFX_ERR_UNSUPPORTED; \
    } \
}

#ifndef MFX_VERSION
#error MFX_VERSION not defined
#endif

msdk_tick TranscodingSample::GetTick()
{
    return msdk_time_get_tick();
}

mfxF64 TranscodingSample::GetTime(msdk_tick start)
{
    static msdk_tick frequency = msdk_time_get_frequency();

    return MSDK_GET_TIME(msdk_time_get_tick(), start, frequency);
}

void TranscodingSample::PrintError(const msdk_char *strErrorMessage, ...)
{
    if (strErrorMessage)
    {
        msdk_printf(MSDK_STRING("ERROR: "));
        va_list args;
        va_start(args, strErrorMessage);
        msdk_vprintf(strErrorMessage, args);
        va_end(args);
        msdk_printf(MSDK_STRING("\nUsage: sample_multi_transcode [options] [--] pipeline-description\n"));
        msdk_printf(MSDK_STRING("   or: sample_multi_transcode [options] -par ParFile\n"));
        msdk_printf(MSDK_STRING("\n"));
        msdk_printf(MSDK_STRING("Run application with -? option to get full help text.\n\n"));
    }
}

void TranscodingSample::PrintHelp()
{
    msdk_printf(MSDK_STRING("Multi Transcoding Sample Version %s\n\n"), GetMSDKSampleVersion().c_str());
    msdk_printf(MSDK_STRING("Command line parameters\n"));

    msdk_printf(MSDK_STRING("Usage: sample_multi_transcode [options] [--] pipeline-description\n"));
    msdk_printf(MSDK_STRING("   or: sample_multi_transcode [options] -par ParFile\n"));
    msdk_printf(MSDK_STRING("\n"));
    msdk_printf(MSDK_STRING("  -stat <N>\n"));
    msdk_printf(MSDK_STRING("                Output statistic every N transcoding cycles\n"));
    msdk_printf(MSDK_STRING("  -stat-log <name>\n"));
    msdk_printf(MSDK_STRING("                Output statistic to the specified file (opened in append mode)\n"));
    msdk_printf(MSDK_STRING("  -stat-per-frame <name>\n"));
    msdk_printf(MSDK_STRING("                Output per-frame latency values to a file (opened in append mode). The file name will be for an input sesssion: <name>_input_ID_<N>.log\n"));
    msdk_printf(MSDK_STRING("                or, for output session: <name>_output_ID_<N>.log; <N> - a number of a session.\n"));

    msdk_printf(MSDK_STRING("Options:\n"));
    //                     ("  ............xx
    msdk_printf(MSDK_STRING("  -?            Print this help and exit\n"));
    msdk_printf(MSDK_STRING("  -p <file-name>\n"));
    msdk_printf(MSDK_STRING("                Collect performance statistics in specified file\n"));
    msdk_printf(MSDK_STRING("  -timeout <seconds>\n"));
    msdk_printf(MSDK_STRING("                Set time to run transcoding in seconds\n"));
    msdk_printf(MSDK_STRING("  -greedy \n"));
    msdk_printf(MSDK_STRING("                Use greedy formula to calculate number of surfaces\n"));
    msdk_printf(MSDK_STRING("\n"));
    msdk_printf(MSDK_STRING("Pipeline description (general options):\n"));
    msdk_printf(MSDK_STRING("  -i::h265|h264|mpeg2|vc1|mvc|jpeg|vp9 <file-name>\n"));
    msdk_printf(MSDK_STRING("                 Set input file and decoder type\n"));
    msdk_printf(MSDK_STRING("  -i::rgb4_frame Set input rgb4 file for compositon. File should contain just one single frame (-vpp_comp_src_h and -vpp_comp_src_w should be specified as well).\n"));
    msdk_printf(MSDK_STRING("  -o::h265|h264|mpeg2|mvc|jpeg|raw <file-name>\n"));
    msdk_printf(MSDK_STRING("                Set output file and encoder type\n"));
    msdk_printf(MSDK_STRING("  -sw|-hw|-hw_d3d11\n"));
    msdk_printf(MSDK_STRING("                SDK implementation to use: \n"));
    msdk_printf(MSDK_STRING("                      -hw - platform-specific on default display adapter (default)\n"));
    msdk_printf(MSDK_STRING("                      -hw_d3d11 - platform-specific via d3d11\n"));
    msdk_printf(MSDK_STRING("                      -sw - software\n"));
#if (MFX_VERSION >= 1025)
    msdk_printf(MSDK_STRING("  -mfe_frames <N> maximum number of frames to be combined in multi-frame encode pipeline"));
    msdk_printf(MSDK_STRING("               0 - default for platform will be used\n"));
    msdk_printf(MSDK_STRING("  -mfe_mode 0|1|2|3 multi-frame encode operation mode - should be the same for all sessions\n"));
    msdk_printf(MSDK_STRING("            0, MFE operates as DEFAULT mode, decided by SDK if MFE enabled\n"));
    msdk_printf(MSDK_STRING("            1, MFE is disabled\n"));
    msdk_printf(MSDK_STRING("            2, MFE operates as AUTO mode\n"));
    msdk_printf(MSDK_STRING("            3, MFE operates as MANUAL mode\n"));

    msdk_printf(MSDK_STRING("  -mfe_timeout <N> multi-frame encode timeout in milliseconds - set per sessions control\n"));
#endif

#ifdef ENABLE_MCTF
#if !defined ENABLE_MCTF_EXT
    msdk_printf(MSDK_STRING("  -mctf [Strength]\n"));
    msdk_printf(MSDK_STRING("        Strength is an optional value;  it is in range [0...20]\n"));
    msdk_printf(MSDK_STRING("        value 0 makes MCTF operates in auto mode;\n"));
    msdk_printf(MSDK_STRING("        Strength: integer, [0...20]. Default value is 0.Might be a CSV filename (upto 15 symbols); if a string is convertable to an integer, integer has a priority over filename\n"));
    msdk_printf(MSDK_STRING("        In fixed-strength mode, MCTF strength can be adjusted at framelevel;\n"));
    msdk_printf(MSDK_STRING("        If no Strength is given, MCTF operates in auto mode.\n"));
#else
    msdk_printf(MSDK_STRING("  -mctf MctfMode:BitsPerPixel:Strength:ME:Overlap:DB\n"));
    msdk_printf(MSDK_STRING("        every parameter may be missed; in this case default value is used.\n"));
    msdk_printf(MSDK_STRING("        MctfMode: 0 - spatial filter\n"));
    msdk_printf(MSDK_STRING("        MctfMode: 1- temporal filtering, 1 backward reference\n"));
    msdk_printf(MSDK_STRING("        MctfMode: 2- temporal filtering, 1 backward & 1 forward reference\n"));
    msdk_printf(MSDK_STRING("        MctfMode: 3- temporal filtering, 2 backward & 2 forward references\n"));
    msdk_printf(MSDK_STRING("        MctfMode:  other values: force default mode to be used\n"));
    msdk_printf(MSDK_STRING("        BitsPerPixel: float, valid range [0...12.0]; if exists, is used for automatic filter strength adaptation. Default is 0.0\n"));
    msdk_printf(MSDK_STRING("        Strength: integer, [0...20]. Default value is 0.Might be a CSV filename (upto 15 symbols); if a string is convertable to an integer, integer has a priority over filename\n"));
    msdk_printf(MSDK_STRING("        ME: Motion Estimation precision; 0 - integer ME (default); 1 - quater-pel ME\n"));
    msdk_printf(MSDK_STRING("        Overlap: 0 - do not apply overlap ME (default); 1 - to apply overlap ME\n"));
    msdk_printf(MSDK_STRING("        DB: 0 - do not apply deblock Filter (default); 1 - to apply Deblock Filter\n"));
#endif //ENABLE_MCTF_EXT
#endif //ENABLE_MCTF

    msdk_printf(MSDK_STRING("  -robust       Recover from gpu hang errors as the come (by resetting components)\n"));

    msdk_printf(MSDK_STRING("  -async        Depth of asynchronous pipeline. default value 1\n"));
    msdk_printf(MSDK_STRING("  -join         Join session with other session(s), by default sessions are not joined\n"));
    msdk_printf(MSDK_STRING("  -priority     Use priority for join sessions. 0 - Low, 1 - Normal, 2 - High. Normal by default\n"));
    msdk_printf(MSDK_STRING("  -threads num  Number of session internal threads to create\n"));
    msdk_printf(MSDK_STRING("  -n            Number of frames to transcode\n") \
        MSDK_STRING("                  (session ends after this number of frames is reached). \n") \
        MSDK_STRING("                In decoding sessions (-o::sink) this parameter limits number\n") \
        MSDK_STRING("                   of frames acquired from decoder.\n") \
        MSDK_STRING("                In encoding sessions (-o::source) and transcoding sessions \n") \
        MSDK_STRING("                  this parameter limits number of frames sent to encoder.\n"));

    msdk_printf(MSDK_STRING("  -ext_allocator    Force usage of external allocators\n"));
    msdk_printf(MSDK_STRING("  -sys          Force usage of external system allocator\n"));
    msdk_printf(MSDK_STRING("  -fps <frames per second>\n"));
    msdk_printf(MSDK_STRING("                Transcoding frame rate limit\n"));
    msdk_printf(MSDK_STRING("  -pe           Set encoding plugin for this particular session.\n"));
    msdk_printf(MSDK_STRING("                This setting overrides plugin settings defined by SET clause.\n"));
    msdk_printf(MSDK_STRING("  -pd           Set decoding plugin for this particular session.\n"));
    msdk_printf(MSDK_STRING("                This setting overrides plugin settings defined by SET clause.\n"));
    msdk_printf(MSDK_STRING("                Supported values: hevcd_sw, hevcd_hw, hevce_sw, hevce_gacc, hevce_hw, vp8d_hw, vp8e_hw, vp9d_hw, vp9e_hw, camera_hw, capture_hw, h264_la_hw, ptir_hw, hevce_fei_hw\n"));
    msdk_printf(MSDK_STRING("                Direct GUID number can be used as well\n"));
    msdk_printf(MSDK_STRING("\n"));
    msdk_printf(MSDK_STRING("Pipeline description (encoding options):\n"));
    MOD_SMT_PRINT_HELP;
    msdk_printf(MSDK_STRING("  -b <Kbits per second>\n"));
    msdk_printf(MSDK_STRING("                Encoded bit rate, valid for H.264, MPEG2 and MVC encoders\n"));
    msdk_printf(MSDK_STRING("  -bm           Bitrate multiplier. Use it when required bitrate isn't fit into 16-bit\n"));
    msdk_printf(MSDK_STRING("                Affects following parameters: InitialDelayInKB, BufferSizeInKB, TargetKbps, MaxKbps\n"));
    msdk_printf(MSDK_STRING("  -f <frames per second>\n"));
    msdk_printf(MSDK_STRING("                Video frame rate for the FRC and deinterlace options\n"));
    msdk_printf(MSDK_STRING("  -fe <frames per second>\n"));
    msdk_printf(MSDK_STRING("                Video frame rate for the FRC and deinterlace options (deprecated, will be removed in next versions).\n"));
    msdk_printf(MSDK_STRING("  -override_decoder_framerate <framerate> \n"));
    msdk_printf(MSDK_STRING("                Forces decoder output framerate to be set to provided value (overwriting actual framerate from decoder)\n"));
    msdk_printf(MSDK_STRING("  -override_encoder_framerate <framerate> \n"));
    msdk_printf(MSDK_STRING("                Overwrites framerate of stream going into encoder input with provided value (this option does not enable FRC, it just ovewrites framerate value)\n"));

    msdk_printf(MSDK_STRING("  -u <usage>    Target usage. Valid for H.265, H.264, MPEG2 and MVC encoders. Expected values:\n"));
    msdk_printf(MSDK_STRING("                veryslow(quality), slower, slow, medium(balanced), fast, faster, veryfast(speed)\n"));
    msdk_printf(MSDK_STRING("  -q <quality>  Quality parameter for JPEG encoder; in range [1,100], 100 is the best quality\n"));
    msdk_printf(MSDK_STRING("  -l numSlices  Number of slices for encoder; default value 0 \n"));
    msdk_printf(MSDK_STRING("  -mss maxSliceSize \n"));
    msdk_printf(MSDK_STRING("                Maximum slice size in bytes. Supported only with -hw and h264 codec. This option is not compatible with -l option.\n"));
    msdk_printf(MSDK_STRING("  -la           Use the look ahead bitrate control algorithm (LA BRC) for H.264 encoder. Supported only with -hw option on 4th Generation Intel Core processors. \n"));
    msdk_printf(MSDK_STRING("  -lad depth    Depth parameter for the LA BRC, the number of frames to be analyzed before encoding. In range [10,100]. \n"));
    msdk_printf(MSDK_STRING("                May be 1 in the case when -mss option is specified \n"));
    msdk_printf(MSDK_STRING("  -la_ext       Use external LA plugin (compatible with h264 & hevc encoders)\n"));
    msdk_printf(MSDK_STRING("  -vbr          Variable bitrate control\n"));
    msdk_printf(MSDK_STRING("  -hrd <KBytes> Maximum possible size of any compressed frames \n"));
    msdk_printf(MSDK_STRING("  -wb <Kbits per second>\n"));
    msdk_printf(MSDK_STRING("                Maximum bitrate for sliding window\n"));
    msdk_printf(MSDK_STRING("  -ws           Sliding window size in frames\n"));
    msdk_printf(MSDK_STRING("  -gop_size     Size of GOP structure in frames \n"));
    msdk_printf(MSDK_STRING("  -dist         Distance between I- or P- key frames \n"));
    msdk_printf(MSDK_STRING("  -num_ref      Number of reference frames\n"));
    msdk_printf(MSDK_STRING("  -bref         Arrange B frames in B pyramid reference structure\n"));
    msdk_printf(MSDK_STRING("  -bpyr         Enable B pyramid\n"));
    msdk_printf(MSDK_STRING("  -CodecProfile          - Specifies codec profile\n"));
    msdk_printf(MSDK_STRING("  -CodecLevel            - Specifies codec level\n"));
    msdk_printf(MSDK_STRING("  -GopOptFlag:closed     - Closed gop\n"));
    msdk_printf(MSDK_STRING("  -GopOptFlag:strict     - Strict gop\n"));
    msdk_printf(MSDK_STRING("  -InitialDelayInKB      - The decoder starts decoding after the buffer reaches the initial size InitialDelayInKB, \n\
                            which is equivalent to reaching an initial delay of InitialDelayInKB*8000/TargetKbps ms\n"));
    msdk_printf(MSDK_STRING("  -MaxKbps               - For variable bitrate control, specifies the maximum bitrate at which \n\
                            the encoded data enters the Video Buffering Verifier buffer\n"));
    msdk_printf(MSDK_STRING("  -gpucopy::<on,off> Enable or disable GPU copy mode\n"));
    msdk_printf(MSDK_STRING("  -repartitioncheck::<on,off> Enable or disable RepartitionCheckEnable mode\n"));
    msdk_printf(MSDK_STRING("  -cqp          Constant quantization parameter (CQP BRC) bitrate control method\n"));
    msdk_printf(MSDK_STRING("                              (by default constant bitrate control method is used), should be used along with -qpi, -qpp, -qpb.\n"));
#if (MFX_VERSION >= 1022)
    msdk_printf(MSDK_STRING("  -qpi          Constant quantizer for I frames (if bitrace control method is CQP). In range [1,51]. 0 by default, i.e.no limitations on QP.\n"));
    msdk_printf(MSDK_STRING("  -qpp          Constant quantizer for P frames (if bitrace control method is CQP). In range [1,51]. 0 by default, i.e.no limitations on QP.\n"));
    msdk_printf(MSDK_STRING("  -qpb          Constant quantizer for B frames (if bitrace control method is CQP). In range [1,51]. 0 by default, i.e.no limitations on QP.\n"));
#endif
    msdk_printf(MSDK_STRING("  -qsv-ff       Enable QSV-FF mode\n"));
#if MFX_VERSION >= 1022
    msdk_printf(MSDK_STRING("  -roi_file <roi-file-name>\n"));
    msdk_printf(MSDK_STRING("                Set Regions of Interest for each frame from <roi-file-name>\n"));
    msdk_printf(MSDK_STRING("  -roi_qpmap    Use QP map to emulate ROI for CQP mode\n"));
#endif //MFX_VERSION >= 1022
    msdk_printf(MSDK_STRING("\n"));
    msdk_printf(MSDK_STRING("Pipeline description (vpp options):\n"));
    msdk_printf(MSDK_STRING("  -deinterlace             Forces VPP to deinterlace input stream\n"));
    msdk_printf(MSDK_STRING("  -deinterlace::ADI        Forces VPP to deinterlace input stream using ADI algorithm\n"));
    msdk_printf(MSDK_STRING("  -deinterlace::ADI_SCD    Forces VPP to deinterlace input stream using ADI_SCD algorithm\n"));
    msdk_printf(MSDK_STRING("  -deinterlace::ADI_NO_REF Forces VPP to deinterlace input stream using ADI no ref algorithm\n"));
    msdk_printf(MSDK_STRING("  -deinterlace::BOB        Forces VPP to deinterlace input stream using BOB algorithm\n"));
    msdk_printf(MSDK_STRING("  -detail <level>          Enables detail (edge enhancement) filter with provided level(0..100)\n"));
    msdk_printf(MSDK_STRING("  -denoise <level>         Enables denoise filter with provided level (0..100)\n"));
    msdk_printf(MSDK_STRING("  -FRC::PT      Enables FRC filter with Preserve Timestamp algorithm\n"));
    msdk_printf(MSDK_STRING("  -FRC::DT      Enables FRC filter with Distributed Timestamp algorithm\n"));
    msdk_printf(MSDK_STRING("  -FRC::INTERP  Enables FRC filter with Frame Interpolation algorithm\n"));
    msdk_printf(MSDK_STRING("     NOTE: -FRC filters do not work with -i::sink pipelines !!!\n"));
    msdk_printf(MSDK_STRING("  -ec::nv12|rgb4|yuy2|nv16|p010|p210   Forces encoder input to use provided chroma mode\n"));
    msdk_printf(MSDK_STRING("  -dc::nv12|rgb4|yuy2   Forces decoder output to use provided chroma mode\n"));
    msdk_printf(MSDK_STRING("     NOTE: chroma transform VPP may be automatically enabled if -ec/-dc parameters are provide d\n"));
    msdk_printf(MSDK_STRING("  -angle 180    Enables 180 degrees picture rotation user module before encoding\n"));
    msdk_printf(MSDK_STRING("  -opencl       Uses implementation of rotation plugin (enabled with -angle option) through Intel(R) OpenCL\n"));
    msdk_printf(MSDK_STRING("  -w            Destination picture width, invokes VPP resize\n"));
    msdk_printf(MSDK_STRING("  -h            Destination picture height, invokes VPP resize\n"));
    msdk_printf(MSDK_STRING("  -field_processing t2t|t2b|b2t|b2b|fr2fr - Field Copy feature\n"));
    msdk_printf(MSDK_STRING("  -WeightedPred::default|implicit       Enambles weighted prediction usage\n"));
    msdk_printf(MSDK_STRING("  -WeightedBiPred::default|implicit     Enambles weighted bi-prediction usage\n"));

#if (MFX_VERSION >= 1024)
    msdk_printf(MSDK_STRING("  -extbrc:<on,off,implicit>           Enables external BRC for AVC and HEVC encoders"));
#endif
#if (MFX_VERSION >= MFX_VERSION_NEXT)
    msdk_printf(MSDK_STRING("  -ExtBrcAdaptiveLTR:<on,off>         Set AdaptiveLTR for implicit extbrc"));
#endif
    msdk_printf(MSDK_STRING("  -vpp_comp <sourcesNum>      Enables composition from several decoding sessions. Result is written to the file\n"));
    msdk_printf(MSDK_STRING("  -vpp_comp_only <sourcesNum> Enables composition from several decoding sessions. Result is shown on screen\n"));
    msdk_printf(MSDK_STRING("  -vpp_comp_num_tiles <Num>   Quantity of tiles for composition. if equal to 0 tiles processing ignored\n"));
    msdk_printf(MSDK_STRING("  -vpp_comp_dst_x             X position of this stream in composed stream (should be used in decoder session)\n"));
    msdk_printf(MSDK_STRING("  -vpp_comp_dst_y             Y position of this stream in composed stream (should be used in decoder session)\n"));
    msdk_printf(MSDK_STRING("  -vpp_comp_dst_h             Height of this stream in composed stream (should be used in decoder session)\n"));
    msdk_printf(MSDK_STRING("  -vpp_comp_dst_w             Width of this stream in composed stream (should be used in decoder session)\n"));
    msdk_printf(MSDK_STRING("  -vpp_comp_src_h             Width of this stream in composed stream (should be used in decoder session)\n"));
    msdk_printf(MSDK_STRING("  -vpp_comp_src_w             Width of this stream in composed stream (should be used in decoder session)\n"));
#if MFX_VERSION >= 1023
    msdk_printf(MSDK_STRING("  -vpp_comp_tile_id           Tile_id for current channel of composition (should be used in decoder session)\n"));
#endif
    msdk_printf(MSDK_STRING("  -vpp_comp_dump <file-name>  Dump of VPP Composition's output into file. Valid if with -vpp_comp* options\n"));
    msdk_printf(MSDK_STRING("  -vpp_comp_dump null_render  Disabling rendering after VPP Composition. This is for performance measurements\n"));
#if MFX_VERSION >= 1022
    msdk_printf(MSDK_STRING("  -dec_postproc               Resize after decoder using direct pipe (should be used in decoder session)\n"));
#endif //MFX_VERSION >= 1022
    msdk_printf(MSDK_STRING("\n"));
    msdk_printf(MSDK_STRING("ParFile format:\n"));
    msdk_printf(MSDK_STRING("  ParFile is extension of what can be achieved by setting pipeline in the command\n"));
    msdk_printf(MSDK_STRING("line. For more information on ParFile format see readme-multi-transcode.pdf\n"));
    msdk_printf(MSDK_STRING("\n"));
    msdk_printf(MSDK_STRING("Examples:\n"));
    msdk_printf(MSDK_STRING("  sample_multi_transcode -i::mpeg2 in.mpeg2 -o::h264 out.h264\n"));
    msdk_printf(MSDK_STRING("  sample_multi_transcode -i::mvc in.mvc -o::mvc out.mvc -w 320 -h 240\n"));
}

void TranscodingSample::PrintInfo(mfxU32 session_number, sInputParams* pParams, mfxVersion *pVer)
{
    msdk_char buf[2048];
    MSDK_CHECK_POINTER_NO_RET(pVer);

    if ((MFX_IMPL_AUTO <= pParams->libType) && (MFX_IMPL_HARDWARE4 >= pParams->libType))
    {
        msdk_printf(MSDK_STRING("MFX %s Session %d API ver %d.%d parameters: \n"),
            (MFX_IMPL_SOFTWARE == pParams->libType)? MSDK_STRING("SOFTWARE") : MSDK_STRING("HARDWARE"),
                 session_number,
                 pVer->Major,
                 pVer->Minor);
    }

    if (0 == pParams->DecodeId)
        msdk_printf(MSDK_STRING("Input  video: From parent session\n"));
    else
        msdk_printf(MSDK_STRING("Input  video: %s\n"), CodecIdToStr(pParams->DecodeId).c_str());

    // means that source is parent session
    if (0 == pParams->EncodeId)
        msdk_printf(MSDK_STRING("Output video: To child session\n"));
    else
        msdk_printf(MSDK_STRING("Output video: %s\n"), CodecIdToStr(pParams->EncodeId).c_str());
    if (PrintDllInfo(buf, MSDK_ARRAY_LEN(buf), pParams))
        msdk_printf(MSDK_STRING("MFX dll: %s\n"),buf);
    msdk_printf(MSDK_STRING("\n"));
}

bool TranscodingSample::PrintDllInfo(msdk_char* buf, mfxU32 buf_size, sInputParams* pParams)
{
#if defined(_WIN32) || defined(_WIN64)
    HANDLE   hCurrent = GetCurrentProcess();
    HMODULE *pModules;
    DWORD    cbNeeded;
    int      nModules;
    if (NULL == EnumProcessModules(hCurrent, NULL, 0, &cbNeeded))
        return false;

    nModules = cbNeeded / sizeof(HMODULE);

    pModules = new HMODULE[nModules];
    if (NULL == pModules)
    {
        return false;
    }
    if (NULL == EnumProcessModules(hCurrent, pModules, cbNeeded, &cbNeeded))
    {
        delete []pModules;
        return false;
    }

    for (int i = 0; i < nModules; i++)
    {
        GetModuleFileName(pModules[i], buf, buf_size);
        if (_tcsstr(buf, MSDK_STRING("libmfxhw")) && (MFX_IMPL_SOFTWARE != pParams->libType))
        {
            delete []pModules;
            return true;
        }
        else if (_tcsstr(buf, MSDK_STRING("libmfxsw")) && (MFX_IMPL_SOFTWARE == pParams->libType))
        {
            delete []pModules;
            return true;
        }

    }
    delete []pModules;
    return false;
#else
    return false;
#endif
}

CmdProcessor::CmdProcessor()
{
    m_SessionParamId = 0;
    m_SessionArray.clear();
    m_decoderPlugins.clear();
    m_encoderPlugins.clear();
    m_PerfFILE = NULL;
    m_parName = NULL;
    m_nTimeout = 0;
    statisticsWindowSize = 0;
    statisticsLogFile = NULL;
    DumpLogFileName.clear();
    shouldUseGreedyFormula=false;
    bRobustFlag = false;

} //CmdProcessor::CmdProcessor()

CmdProcessor::~CmdProcessor()
{
    m_SessionArray.clear();
    m_decoderPlugins.clear();
    m_encoderPlugins.clear();
    if (m_PerfFILE)
        fclose(m_PerfFILE);
    if (statisticsLogFile)
        fclose(statisticsLogFile);

} //CmdProcessor::~CmdProcessor()

void CmdProcessor::PrintParFileName()
{
    if (m_parName && m_PerfFILE)
    {
        msdk_fprintf(m_PerfFILE, MSDK_STRING("Input par file: %s\n\n"), m_parName);
    }
}

msdk_string CmdProcessor::GetLine(mfxU32 n)
{
    if (m_lines.size() > n)
        return m_lines[n];
    return msdk_string();
}

mfxStatus CmdProcessor::ParseCmdLine(int argc, msdk_char *argv[])
{
    FILE *parFile = NULL;
    mfxStatus sts = MFX_ERR_UNSUPPORTED;

    if (1 == argc)
    {
       PrintError(MSDK_STRING("Too few parameters"), NULL);
       return MFX_ERR_UNSUPPORTED;
    }

    --argc;
    ++argv;

    while (argv[0])
    {
        if (0 == msdk_strcmp(argv[0], MSDK_STRING("-par")))
        {
            --argc;
            ++argv;
            if (!argv[0]) {
                msdk_printf(MSDK_STRING("error: no argument given for '-par' option\n"));
            }
            m_parName = argv[0];
        }
        else if (0 == msdk_strcmp(argv[0], MSDK_STRING("-timeout")))
        {
            --argc;
            ++argv;
            if (!argv[0]) {
                msdk_printf(MSDK_STRING("error: no argument given for '-timeout' option\n"));
            }
            if (MFX_ERR_NONE != msdk_opt_read(argv[0], m_nTimeout))
            {
                msdk_printf(MSDK_STRING("error: -timeout \"%s\" is invalid"), argv[0]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(argv[0], MSDK_STRING("-robust")))
        {
            bRobustFlag = true;
        }
        else if (0 == msdk_strcmp(argv[0], MSDK_STRING("-?")) )
        {
            PrintHelp();
            return MFX_WRN_OUT_OF_RANGE;
        }
        else if (0 == msdk_strcmp(argv[0], MSDK_STRING("-greedy")))
        {
            shouldUseGreedyFormula=true;
        }
        else if (0 == msdk_strcmp(argv[0], MSDK_STRING("-p")))
        {
            if (m_PerfFILE)
            {
                msdk_printf(MSDK_STRING("error: only one performance file is supported"));
                return MFX_ERR_UNSUPPORTED;
            }
            --argc;
            ++argv;
            if (!argv[0]) {
                msdk_printf(MSDK_STRING("error: no argument given for '-p' option\n"));
            }
            MSDK_FOPEN(m_PerfFILE, argv[0], MSDK_STRING("w"));
            if (NULL == m_PerfFILE)
            {
                msdk_printf(MSDK_STRING("error: performance file \"%s\" not found"), argv[0]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(argv[0], MSDK_STRING("--")))
        {
            // just skip separator "--" which delimits cmd options and pipeline settings
            break;
        }
        else if (0 == msdk_strcmp(argv[0], MSDK_STRING("-stat")))
        {
            --argc;
            ++argv;
            if (!argv[0]) {
                msdk_printf(MSDK_STRING("error: no argument given for 'stat' option\n"));
            }
            if (MFX_ERR_NONE != msdk_opt_read(argv[0], statisticsWindowSize))
            {
                msdk_printf(MSDK_STRING("error: stat \"%s\" is invalid"), argv[0]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(argv[0], MSDK_STRING("-stat-log")))
        {
            if (statisticsLogFile)
            {
                msdk_printf(MSDK_STRING("error: only one statistics file is supported"));
                return MFX_ERR_UNSUPPORTED;
            }
            --argc;
            ++argv;
            if (!argv[0]) {
                msdk_printf(MSDK_STRING("error: no argument given for 'stat-log' option\n"));
            }
            MSDK_FOPEN(statisticsLogFile, argv[0], MSDK_STRING("w"));
            if (NULL == statisticsLogFile)
            {
                msdk_printf(MSDK_STRING("error: statistics file \"%s\" not found"), argv[0]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(argv[0], MSDK_STRING("-stat-per-frame")))
        {
            if (!DumpLogFileName.empty()){
                msdk_printf(MSDK_STRING("error: only one dump file is supported"));
                return MFX_ERR_UNSUPPORTED;
            }
            --argc;
            ++argv;
            if (!argv[0])
            {
                msdk_printf(MSDK_STRING("error: no argument given for 'stat-dump' option\n"));
            }
            DumpLogFileName = argv[0];
        }
        else
        {
            break;
        }
        --argc;
        ++argv;
    }

    msdk_printf(MSDK_STRING("Multi Transcoding Sample Version %s\n\n"), GetMSDKSampleVersion().c_str());

    //Read pipeline from par file
    if (m_parName && !argv[0])
    {
        MSDK_FOPEN(parFile, m_parName, MSDK_STRING("r"));
        if (NULL == parFile)
        {
            msdk_printf(MSDK_STRING("error: ParFile \"%s\" not found\n"), m_parName);
            return MFX_ERR_UNSUPPORTED;
        }

        if (NULL != m_parName) msdk_printf(MSDK_STRING("Par file is: %s\n\n"), m_parName);

        sts = ParseParFile(parFile);

        if (MFX_ERR_NONE != sts)
        {
            fclose(parFile);
            return sts;
        }

        fclose(parFile);
    }
    //Read pipeline from cmd line
    else if (!argv[0])
    {
        msdk_printf(MSDK_STRING ("error: pipeline description not found\n"));
        return MFX_ERR_UNSUPPORTED;
    }
    else if (argv[0] && m_parName)
    {
        msdk_printf(MSDK_STRING ("error: simultaneously enabling parfile and description pipeline from command line forbidden\n"));
        return MFX_ERR_UNSUPPORTED;
    }
    else
    {
        sts = ParseParamsForOneSession(argc, argv);
        if (MFX_ERR_NONE != sts)
        {
            msdk_printf(MSDK_STRING("error: pipeline description is invalid\n"));
            return sts;
        }
    }

    return sts;

} //mfxStatus CmdProcessor::ParseCmdLine(int argc, msdk_char *argv[])

mfxStatus CmdProcessor::ParseParFile(FILE *parFile)
{
    mfxStatus sts = MFX_ERR_UNSUPPORTED;
    if (!parFile)
        return MFX_ERR_UNSUPPORTED;

    mfxU32 currPos = 0;
    mfxU32 lineIndex = 0;

    // calculate file size
    fseek(parFile, 0, SEEK_END);
    mfxU32 fileSize = ftell(parFile) + 1;
    fseek(parFile, 0, SEEK_SET);

    // allocate buffer for parsing
    s_ptr<msdk_char, false> parBuf;
    parBuf.reset(new msdk_char[fileSize]);
    msdk_char *pCur;

    while(currPos < fileSize)
    {
        pCur = /*_fgetts*/msdk_fgets(parBuf.get(), fileSize, parFile);
        if (!pCur)
            return MFX_ERR_NONE;
        while(pCur[currPos] != '\n' && pCur[currPos] != 0)
        {
            currPos++;
            if  (pCur + currPos >= parBuf.get() + fileSize)
                return sts;
        }
        // zero string
        if (!currPos)
            continue;

        sts = TokenizeLine(pCur, currPos);
        MSDK_CHECK_STATUS(sts, "TokenizeLine failed");

        currPos = 0;
        lineIndex++;
    }

    return MFX_ERR_NONE;

} //mfxStatus CmdProcessor::ParseParFile(FILE *parFile)

mfxStatus CmdProcessor::TokenizeLine(msdk_char *pLine, mfxU32 length)
{
    mfxU32 i;
    const mfxU8 maxArgNum = 255;
    msdk_char *argv[maxArgNum+1];
    mfxU32 argc = 0;
    s_ptr<msdk_char, false> pMemLine;

    pMemLine.reset(new msdk_char[length+2]);

    msdk_char *pTempLine = pMemLine.get();
    pTempLine[0] = ' ';
    pTempLine++;

    MSDK_MEMCPY_BUF(pTempLine,0 , length*sizeof(msdk_char), pLine, length*sizeof(msdk_char));

    // parse into command streams
    for (i = 0; i < length ; i++)
    {
        // check if separator
        if (IS_SEPARATOR(pTempLine[-1]) && !IS_SEPARATOR(pTempLine[0]))
        {
            argv[argc++] = pTempLine;
            if (argc > maxArgNum)
            {
                PrintError(MSDK_STRING("Too many parameters (reached maximum of %d)"), maxArgNum);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        if (*pTempLine == ' ' || *pTempLine == '\r' || *pTempLine == '\n')
        {
            *pTempLine = 0;
        }
        pTempLine++;
    }

    // EOL for last parameter
    pTempLine[0] = 0;

    return ParseParamsForOneSession(argc, argv);
}

#if MFX_VERSION >= 1022
bool CmdProcessor::isspace(char a) { return (std::isspace(a) != 0); }

bool CmdProcessor::is_not_allowed_char(char a) {
    return (std::isdigit(a) != 0) && (std::isspace(a) != 0) && (a != ';') && (a != '-');
}

bool CmdProcessor::ParseROIFile(const msdk_char *roi_file_name, std::vector<mfxExtEncoderROI>& m_ROIData)
{
    FILE *roi_file = NULL;
    MSDK_FOPEN(roi_file, roi_file_name, MSDK_STRING("rb"));

    m_ROIData.clear();

    if (roi_file)
    {

        // read file to buffer
        fseek(roi_file, 0, SEEK_END);
        long file_size = ftell(roi_file);
        rewind (roi_file);
        std::vector<char> buffer(file_size);
        char *roi_data = &buffer[0];
        if (file_size<0 || (size_t)file_size != fread(roi_data, 1, file_size, roi_file))
        {
            fclose(roi_file);
            return false;
        }
        fclose(roi_file);

        // search for not allowed characters
        char *not_allowed_char = std::find_if(roi_data, roi_data + file_size,
            is_not_allowed_char);
        if (not_allowed_char != (roi_data + file_size))
        {
            return false;
        }

        // get unformatted roi data
        std::string unformatted_roi_data;
        unformatted_roi_data.clear();
        std::remove_copy_if(roi_data, roi_data + file_size,
            std::inserter(unformatted_roi_data, unformatted_roi_data.end()),
            isspace);

        // split data to items
        std::stringstream unformatted_roi_data_ss(unformatted_roi_data);
        std::vector<std::string> items;
        items.clear();
        std::string item;
        while (std::getline(unformatted_roi_data_ss, item, ';'))
        {
            items.push_back(item);
        }

        // parse data and store roi data for each frame
        unsigned int item_ind = 0;
        while (1)
        {
            if (item_ind >= items.size()) break;

            mfxExtEncoderROI frame_roi;
            std::memset(&frame_roi, 0, sizeof(frame_roi));
            frame_roi.Header.BufferId = MFX_EXTBUFF_ENCODER_ROI;
#if (MFX_VERSION >= 1022)
            frame_roi.ROIMode = MFX_ROI_MODE_QP_DELTA;
#endif

            int roi_num = std::atoi(items[item_ind].c_str());
            if (roi_num < 0 || roi_num > (int)(sizeof(frame_roi.ROI) / sizeof(frame_roi.ROI[0])))
            {
                m_ROIData.clear();
                return false;
            }
            if ((item_ind + 5 * roi_num) >= items.size())
            {
                m_ROIData.clear();
                return false;
            }

            for (int i = 0; i < roi_num; i++)
            {
                // do not handle out of range integer errors
                frame_roi.ROI[i].Left = std::atoi(items[item_ind + i * 5 + 1].c_str());
                frame_roi.ROI[i].Top = std::atoi(items[item_ind + i * 5 + 2].c_str());
                frame_roi.ROI[i].Right = std::atoi(items[item_ind + i * 5 + 3].c_str());
                frame_roi.ROI[i].Bottom = std::atoi(items[item_ind + i * 5 + 4].c_str());
#if (MFX_VERSION >= 1022)
                frame_roi.ROI[i].DeltaQP = (mfxI16) std::atoi(items[item_ind +i * 5 + 5].c_str());
#endif
            }
            frame_roi.NumROI = (mfxU16) roi_num;
            m_ROIData.push_back(frame_roi);
            item_ind = item_ind + roi_num * 5 + 1;
        }
    }
    else {
        return false;
    }
    return true;
}
#endif //MFX_VERSION >= 1022
#ifdef ENABLE_MCTF

// returns a pointer to start of argument with a number argn;
// if failes to find argn, returns NULL
msdk_char* ParseArgn(msdk_char* pIn, mfxU32 argn, msdk_char separator) {

    msdk_char* pstr = pIn;
    if (!argn)
        return pIn;
    else {
        for (mfxU32 n = 0; n != argn; ++n) {
            while (separator != *pstr && msdk_char('\0') != *pstr)
                ++pstr;
            if (msdk_char('\0') == *pstr)
                return NULL;
            else
                ++pstr;
        }
        return pstr;
    }
};

template <typename T>
bool ArgConvert(msdk_char* pIn, mfxU32 argn, const msdk_char* pattern, T* pArg, const T& ArgDefault, mfxU32& NumOfGoodConverts) {
    bool bConvertIsOk = false;
    msdk_char* pargs = ParseArgn(pIn, argn, msdk_char(':'));
    if (pargs)
    {
        if (!msdk_sscanf(pargs, pattern, pArg))
            *pArg = ArgDefault;
        else {
            ++NumOfGoodConverts;
            bConvertIsOk = true;
        }
    };
    return bConvertIsOk;
}

//template <typename T=msdk_string>
bool ArgConvert(msdk_char* pIn, mfxU32 argn, const msdk_char* pattern, msdk_char* pArg, mfxU32 MaxChars2Read, mfxU32& NumOfGoodConverts) {
    bool bConvertIsOk = false;
    msdk_char* pargs = ParseArgn(pIn, argn, msdk_char(':'));
    if (pargs)
    {
        // lets calculate length of potential name:
        msdk_char* temp(pargs);
        while (*temp != msdk_char(':') && *temp != msdk_char('\0'))
            ++temp;
        std::iterator_traits<msdk_char*>::difference_type distance = std::distance(pargs, temp);
        if (distance < std::iterator_traits<msdk_char*>::difference_type(MaxChars2Read))
        {
            if (msdk_sscanf(pargs, pattern, pArg, MaxChars2Read))
            {
                ++NumOfGoodConverts;
                bConvertIsOk = true;
            }
        };
    };
    return bConvertIsOk;
}

void ParseMCTFParams(msdk_char* strInput[], mfxU32 nArgNum, mfxU32& curArg, sInputParams * pParams)
{
    mfxU32& i = curArg;
    if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-mctf")))
    {
        pParams->mctfParam.mode = VPP_FILTER_ENABLED_DEFAULT;
        pParams->mctfParam.params.FilterStrength = 0;
        pParams->mctfParam.rtParams.Reset();
        bool bFSByValue = true;
#if defined ENABLE_MCTF_EXT
        pParams->mctfParam.params.TemporalMode = MFX_MCTF_TEMPORAL_MODE_2REF; // default
        pParams->mctfParam.params.BitsPerPixelx100k = 0;
        pParams->mctfParam.params.Deblocking = MFX_CODINGOPTION_OFF;
        pParams->mctfParam.params.Overlap = MFX_CODINGOPTION_OFF;
        pParams->mctfParam.params.MVPrecision = MFX_MVPRECISION_INTEGER;
#endif


        if (i + 1 < nArgNum)
        {
            mfxU16 _strength(0);
            mfxU32 strength_idx = 0;
            mfxU32 ParsedArgsNumber = 0;
            const mfxU32 max_name_len = 15;
            const mfxU32 max_elems_in_file = 10000;
            msdk_stringstream file_pattern;
            file_pattern << MSDK_STRING("%") << max_name_len << MSDK_STRING("ls:%*c") << std::ends;
            msdk_char _tmp_str[max_name_len + 1];
            memset(_tmp_str, 0, sizeof(_tmp_str));
#if defined ENABLE_MCTF_EXT
            strength_idx = 2;
#endif
            //the order of arguments is:
            // MctfMode:BitsPerPixel:Strength:ME:Overlap:DB
            // try to read fs defined as a value:
            bool res = ArgConvert(strInput[i + 1], strength_idx, MSDK_STRING("%hd:%*c"), &_strength, _strength, ParsedArgsNumber);
            if (!res)
            {
                bFSByValue = false;
                // if it was not possible, try to get a file-name (upto 15 chars):
                    res = ArgConvert(strInput[i + 1], strength_idx, file_pattern.str().c_str(), &(_tmp_str[0]), max_name_len, ParsedArgsNumber);
                if (res)
                {
                    msdk_fstream fs_file(_tmp_str, std::ios_base::in);
                    if (!fs_file.is_open())
                    {
                        msdk_printf(MSDK_STRING("MCTF Filter-strength file is not exist; decay to default FS value;.\n"));
                        bFSByValue = true;
                    }
                    else
                    {
                        mfxU32 nOfRTParams(0);
                        for (msdk_string line; std::getline(fs_file, line, msdk_char(',')) && nOfRTParams < max_elems_in_file; ++nOfRTParams)
                        {
                            // currently, there is just 1 param in the file;
                            sMctfRunTimeParam tmp;
                            if(msdk_sscanf(line.c_str(), MSDK_STRING("%hd:%*c"), &(tmp.FilterStrength)))
                                pParams->mctfParam.rtParams.RunTimeParams.push_back(tmp);
                            else
                            {
                                msdk_printf(MSDK_STRING("there was an error met during parsing FS file;.only a few values were parsed.\n"));
                                break;
                            }
                        }
                    }
                }
                else
                    bFSByValue = true;
            }
#if defined ENABLE_MCTF_EXT
            mfxU16 _refnum(2);
            mfxF64 _bitsperpixel(0.0);
            mfxU16 _me_precision(0);
            mfxU16 _overlap(0);
            mfxU16 _deblock(0);

            ArgConvert(strInput[i + 1], 0, MSDK_STRING("%hd:%*c"), &_refnum, _refnum, ParsedArgsNumber);
            ArgConvert(strInput[i + 1], 1, MSDK_STRING("%lf:%*c"), &_bitsperpixel, _bitsperpixel, ParsedArgsNumber);
            ArgConvert(strInput[i + 1], 3, MSDK_STRING("%hd:%*c"), &_me_precision, _me_precision, ParsedArgsNumber);
            ArgConvert(strInput[i + 1], 4, MSDK_STRING("%hd:%*c"), &_overlap, _overlap, ParsedArgsNumber);
            ArgConvert(strInput[i + 1], 5, MSDK_STRING("%hd:%*c"), &_deblock, _deblock, ParsedArgsNumber);
#endif
            if (0 == ParsedArgsNumber)
            {
                pParams->mctfParam.mode = VPP_FILTER_ENABLED_DEFAULT;
                msdk_printf(MSDK_STRING("MCTF works in default mode; no parameters are passed.\n"));
            }
            else
            {
                pParams->mctfParam.mode = VPP_FILTER_ENABLED_CONFIGURED;
                pParams->mctfParam.rtParams.Restart();
                if (bFSByValue)
                {
                    pParams->mctfParam.params.FilterStrength = _strength;
                }
                else
                {
                    // take very first FS value from the file and use it as a value for FilterStrength
                    pParams->mctfParam.params.FilterStrength = pParams->mctfParam.rtParams.GetCurParam()->FilterStrength;
                }
#if defined ENABLE_MCTF_EXT
                pParams->mctfParam.params.BitsPerPixelx100k = mfxU32(_bitsperpixel*MCTF_BITRATE_MULTIPLIER);
                switch (_refnum) {
                case 0:
                    pParams->mctfParam.params.TemporalMode = MFX_MCTF_TEMPORAL_MODE_SPATIAL;
                    break;
                case 1:
                    pParams->mctfParam.params.TemporalMode = MFX_MCTF_TEMPORAL_MODE_1REF;
                    break;
                case 2:
                    pParams->mctfParam.params.TemporalMode = MFX_MCTF_TEMPORAL_MODE_2REF;
                    break;
                case 3:
                    pParams->mctfParam.params.TemporalMode = MFX_MCTF_TEMPORAL_MODE_4REF;
                    break;
                default:
                    pParams->mctfParam.params.TemporalMode = MFX_MCTF_TEMPORAL_MODE_UNKNOWN;
                };
                switch (_deblock) {
                case 0:
                    pParams->mctfParam.params.Deblocking = MFX_CODINGOPTION_OFF;
                    break;
                case 1:
                    pParams->mctfParam.params.Deblocking = MFX_CODINGOPTION_ON;
                    break;
                default:
                    pParams->mctfParam.params.Deblocking = MFX_CODINGOPTION_UNKNOWN;
                };
                switch (_overlap) {
                case 0:
                    pParams->mctfParam.params.Overlap = MFX_CODINGOPTION_OFF;
                    break;
                case 1:
                    pParams->mctfParam.params.Overlap = MFX_CODINGOPTION_ON;
                    break;
                default:
                    pParams->mctfParam.params.Overlap = MFX_CODINGOPTION_UNKNOWN;
                };
                switch (_me_precision) {
                case 0:
                    pParams->mctfParam.params.MVPrecision = MFX_MVPRECISION_INTEGER;
                    break;
                case 1:
                    pParams->mctfParam.params.MVPrecision = MFX_MVPRECISION_QUARTERPEL;
                    break;
                default:
                    pParams->mctfParam.params.MVPrecision = MFX_MVPRECISION_UNKNOWN;
                };
#endif
            }
            if(ParsedArgsNumber)
                i++;
        }
        else
        {
            msdk_printf(MSDK_STRING("MCTF works in default mode; no parameters are passed.\n"));
        }
    }
}
#endif

mfxStatus CmdProcessor::ParseParamsForOneSession(mfxU32 argc, msdk_char *argv[])
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxStatus stsExtBuf = MFX_ERR_NONE;
    mfxU32 skipped = 0;

    // save original cmd line for debug purpose
    msdk_stringstream cmd;
    for (mfxU32 i = 0; i <argc; i++)
        cmd <<argv[i] << MSDK_STRING(" ");
    m_lines.push_back(cmd.str());

    TranscodingSample::sInputParams InputParams;
    if (m_nTimeout)
        InputParams.nTimeout = m_nTimeout;
    if (bRobustFlag)
        InputParams.bRobustFlag = bRobustFlag;

    InputParams.shouldUseGreedyFormula = shouldUseGreedyFormula;

    InputParams.statisticsWindowSize = statisticsWindowSize;
    InputParams.statisticsLogFile = statisticsLogFile;

    //bind to a dump-log-file name
    InputParams.DumpLogFileName = DumpLogFileName;

    if (0 == msdk_strcmp(argv[0], MSDK_STRING("set")))
    {
        if (argc != 3) {
            msdk_printf(MSDK_STRING("error: number of arguments for 'set' options is wrong"));
            return MFX_ERR_UNSUPPORTED;
        }
        sts = ParseOption__set(argv[1], argv[2]);
        return sts;
    }
    // default implementation
    InputParams.libType = MFX_IMPL_HARDWARE_ANY;
    InputParams.bUseOpaqueMemory = true;
    InputParams.eModeExt = Native;

    for (mfxU32 i = 0; i < argc; i++)
    {
        // process multi-character options
        if ( (0 == msdk_strncmp(MSDK_STRING("-i::"), argv[i], msdk_strlen(MSDK_STRING("-i::"))))
          && (0 != msdk_strncmp(argv[i]+4, MSDK_STRING("source"), msdk_strlen(MSDK_STRING("source")))) )
        {
            sts = StrFormatToCodecFormatFourCC(argv[i]+4, InputParams.DecodeId);
            if (sts != MFX_ERR_NONE)
            {
                return MFX_ERR_UNSUPPORTED;
            }
            VAL_CHECK(i+1 == argc, i, argv[i]);
            i++;
            SIZE_CHECK((msdk_strlen(argv[i])+1) > MSDK_ARRAY_LEN(InputParams.strSrcFile));
            msdk_opt_read(argv[i], InputParams.strSrcFile);
            if (InputParams.eMode == Source)
            {
                switch(InputParams.DecodeId)
                {
                    case MFX_CODEC_MPEG2:
                    case MFX_CODEC_HEVC:
                    case MFX_CODEC_AVC:
                    case MFX_CODEC_VC1:
                    case MFX_CODEC_VP9:
                    case CODEC_MVC:
                    case MFX_CODEC_JPEG:
                        return MFX_ERR_UNSUPPORTED;
                }
            }
            if (InputParams.DecodeId == CODEC_MVC)
            {
                InputParams.DecodeId = MFX_CODEC_AVC;
                InputParams.bIsMVC = true;
            }
        }
        else if ( (0 == msdk_strncmp(MSDK_STRING("-o::"), argv[i], msdk_strlen(MSDK_STRING("-o::"))))
               && (0 != msdk_strncmp(argv[i]+4, MSDK_STRING("sink"), msdk_strlen(MSDK_STRING("sink")))) )
        {

            sts = StrFormatToCodecFormatFourCC(argv[i]+4, InputParams.EncodeId);

            if (sts != MFX_ERR_NONE)
            {
                return MFX_ERR_UNSUPPORTED;
            }
            VAL_CHECK(i+1 == argc, i, argv[i]);
            i++;
            SIZE_CHECK((msdk_strlen(argv[i])+1) > MSDK_ARRAY_LEN(InputParams.strDstFile));
            msdk_opt_read(argv[i], InputParams.strDstFile);
            if (InputParams.eMode == Sink || InputParams.bIsMVC)
            {
                switch(InputParams.EncodeId)
                {
                    case MFX_CODEC_MPEG2:
                    case MFX_CODEC_HEVC:
                    case MFX_CODEC_AVC:
                    case MFX_CODEC_JPEG:
                    case MFX_CODEC_DUMP:
                        return MFX_ERR_UNSUPPORTED;
                }
            }
            if (InputParams.EncodeId == CODEC_MVC)
            {
                if (InputParams.eMode == Sink)
                    return MFX_ERR_UNSUPPORTED;

                InputParams.EncodeId = MFX_CODEC_AVC;
                InputParams.bIsMVC = true;
            }
        }
#if MFX_VERSION >= 1022
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-roi_file")))
        {
            VAL_CHECK(i+1 == argc, i, argv[i]);
            i++;

            msdk_char strRoiFile[MSDK_MAX_FILENAME_LEN];
            SIZE_CHECK((msdk_strlen(argv[i])+1) > MSDK_ARRAY_LEN(strRoiFile));
            msdk_opt_read(argv[i], strRoiFile);

            if(!ParseROIFile(strRoiFile, InputParams.m_ROIData)) {
                PrintError(MSDK_STRING("Incorrect ROI file: \"%s\" "), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-p")))
        {
            if (m_PerfFILE)
            {
                msdk_printf(MSDK_STRING("error: only one performance file is supported"));
                return MFX_ERR_UNSUPPORTED;
            }
            --argc;
            ++argv;
            if (!argv[i]) {
                msdk_printf(MSDK_STRING("error: no argument given for '-p' option\n"));
            }
            MSDK_FOPEN(m_PerfFILE, argv[i], MSDK_STRING("w"));
            if (NULL == m_PerfFILE)
            {
                msdk_printf(MSDK_STRING("error: performance file \"%s\" not found"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-roi_qpmap")))
        {
            InputParams.bROIasQPMAP = true;
        }
#endif //MFX_VERSION >= 1022
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-sw")))
        {
            InputParams.libType = MFX_IMPL_SOFTWARE;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-hw")))
        {
            InputParams.libType = MFX_IMPL_HARDWARE_ANY;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-hw_d3d11")))
        {
            InputParams.libType = MFX_IMPL_HARDWARE_ANY | MFX_IMPL_VIA_D3D11;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-sys")))
        {
            InputParams.bUseOpaqueMemory = false;
            InputParams.bForceSysMem = true;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-perf_opt")))
        {
            InputParams.bIsPerf = true;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-robust")))
        {
            InputParams.bRobustFlag = true;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-threads")))
        {
            VAL_CHECK(i+1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.nThreadsNum))
            {
                PrintError(MSDK_STRING("Threads number is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if(0 == msdk_strcmp(argv[i], MSDK_STRING("-f")))
        {
            VAL_CHECK(i+1 == argc, i, argv[i]);
            i++;
            // Temporary check for giving priority to -fe option
            if(!InputParams.dVPPOutFramerate)
            {
                if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.dVPPOutFramerate))
                {
                    PrintError(MSDK_STRING("FrameRate \"%s\" is invalid"), argv[i]);
                    return MFX_ERR_UNSUPPORTED;
                }
            }
        }
        else if(0 == msdk_strcmp(argv[i], MSDK_STRING("-fe")))
        {
            VAL_CHECK(i+1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.dVPPOutFramerate))
            {
                PrintError(MSDK_STRING("FrameRate \"%s\" is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if(0 == msdk_strcmp(argv[i], MSDK_STRING("-fps")))
        {
            VAL_CHECK(i+1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.nFPS))
            {
                PrintError(MSDK_STRING("FPS limit \"%s\" is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if(0 == msdk_strcmp(argv[i], MSDK_STRING("-b")))
        {
            VAL_CHECK(i+1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.nBitRate))
            {
                PrintError(MSDK_STRING("BitRate \"%s\" is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-bm")))
        {
            VAL_CHECK(i + 1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.nBitRateMultiplier))
            {
                PrintError(MSDK_STRING("Bitrate multiplier \"%s\" is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if(0 == msdk_strcmp(argv[i], MSDK_STRING("-wb")))
        {
            VAL_CHECK(i+1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.WinBRCMaxAvgKbps))
            {
                PrintError(MSDK_STRING("Maximum bitrate for sliding window \"%s\" is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if(0 == msdk_strcmp(argv[i], MSDK_STRING("-ws")))
        {
            VAL_CHECK(i+1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.WinBRCSize))
            {
                PrintError(MSDK_STRING("Sliding window size \"%s\" is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if(0 == msdk_strcmp(argv[i], MSDK_STRING("-hrd")))
        {
            VAL_CHECK(i+1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.BufferSizeInKB))
            {
                PrintError(MSDK_STRING("Frame buffer size \"%s\" is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if(0 == msdk_strcmp(argv[i], MSDK_STRING("-dist")))
        {
            VAL_CHECK(i+1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.GopRefDist))
            {
                PrintError(MSDK_STRING("GOP reference distance \"%s\" is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if(0 == msdk_strcmp(argv[i], MSDK_STRING("-gop_size")))
        {
            VAL_CHECK(i+1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.GopPicSize))
            {
                PrintError(MSDK_STRING("GOP size \"%s\" is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-num_ref")))
        {
            VAL_CHECK(i + 1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.NumRefFrame))
            {
                PrintError(MSDK_STRING("Number of reference frames \"%s\" is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-CodecLevel")))
        {
            VAL_CHECK(i + 1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.CodecLevel))
            {
                PrintError(MSDK_STRING("CodecLevel \"%s\" is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-CodecProfile")))
        {
            VAL_CHECK(i + 1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.CodecProfile))
            {
                PrintError(MSDK_STRING("CodecProfile \"%s\" is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-MaxKbps")))
        {
            VAL_CHECK(i + 1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.MaxKbps))
            {
                PrintError(MSDK_STRING("MaxKbps \"%s\" is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-InitialDelayInKB")))
        {
            VAL_CHECK(i + 1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.InitialDelayInKB))
            {
                PrintError(MSDK_STRING("InitialDelayInKB \"%s\" is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-GopOptFlag:closed")))
        {
            InputParams.GopOptFlag = MFX_GOP_CLOSED;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-GopOptFlag:strict")))
        {
            InputParams.GopOptFlag = MFX_GOP_STRICT;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-bref")))
        {
            InputParams.nBRefType = MFX_B_REF_PYRAMID;
        }
        else if(0 == msdk_strcmp(argv[i], MSDK_STRING("-u")))
        {
            VAL_CHECK(i+1 == argc, i, argv[i]);
            i++;
            InputParams.nTargetUsage = StrToTargetUsage(argv[i]);
            if (!InputParams.nTargetUsage)
            {
                PrintError(MSDK_STRING(" \"%s\" target usage is invalid. Balanced will be used."), argv[i]);
                InputParams.nTargetUsage = MFX_TARGETUSAGE_BALANCED;
            }
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-WeightedPred::default")))
        {
            InputParams.WeightedPred = MFX_WEIGHTED_PRED_DEFAULT;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-WeightedPred::implicit")))
        {
            InputParams.WeightedPred = MFX_WEIGHTED_PRED_IMPLICIT;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-WeightedBiPred::default")))
        {
            InputParams.WeightedBiPred = MFX_WEIGHTED_PRED_DEFAULT;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-WeightedBiPred::implicit")))
        {
            InputParams.WeightedBiPred = MFX_WEIGHTED_PRED_IMPLICIT;
        }
        else if(0 == msdk_strcmp(argv[i], MSDK_STRING("-q")))
        {
            VAL_CHECK(i+1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.nQuality))
            {
                PrintError(MSDK_STRING(" \"%s\" quality is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-w")))
        {
            VAL_CHECK(i+1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.nDstWidth))
            {
                PrintError(MSDK_STRING("width \"%s\" is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-h")))
        {
            VAL_CHECK(i+1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.nDstHeight))
            {
                PrintError(MSDK_STRING("height \"%s\" is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-l")))
        {
            VAL_CHECK(i+1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.nSlices))
            {
                PrintError(MSDK_STRING("numSlices \"%s\" is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-mss")))
        {
            VAL_CHECK(i+1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.nMaxSliceSize))
            {
                PrintError(MSDK_STRING("maxSliceSize \"%s\" is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-async")))
        {
            VAL_CHECK(i+1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.nAsyncDepth))
            {
                PrintError(MSDK_STRING("async \"%s\" is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-join")))
        {
           InputParams.bIsJoin = true;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-priority")))
        {
            VAL_CHECK(i+1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.priority))
            {
                PrintError(MSDK_STRING("priority \"%s\" is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-i::source")))
        {
            if (InputParams.eMode != Native)
            {
                PrintError(NULL,"-i::source cannot be used here");
                return MFX_ERR_UNSUPPORTED;
            }

            InputParams.eMode = Source;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-o::sink")))
        {
             if (InputParams.eMode != Native)
             {
                 PrintError(NULL,"-o::sink cannot be used here");
                 return MFX_ERR_UNSUPPORTED;
             }

            InputParams.eMode = Sink;

        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-vpp_comp")))
        {
            VAL_CHECK(i + 1 == argc, i, argv[i]);
            i++;
            /* NB! numSurf4Comp should be equal to Number of decoding session */
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.numSurf4Comp))
            {
                PrintError(MSDK_STRING("-n \"%s\" is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
            /* This is can init early */
            if (InputParams.eModeExt == Native)
                InputParams.eModeExt = VppComp;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-vpp_comp_only")))
        {
            /* for VPP comp with rendering we have to use ext allocator */
#ifdef LIBVA_SUPPORT
            InputParams.libvaBackend = MFX_LIBVA_DRM;
#endif
            InputParams.bUseOpaqueMemory = false;

            VAL_CHECK(i + 1 == argc, i, argv[i]);
            i++;
            /* NB! numSurf4Comp should be equal to Number of decoding session */
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.numSurf4Comp))
            {
                PrintError(MSDK_STRING("-n \"%s\" is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
            /* This is can init early */
            if (InputParams.eModeExt == Native)
                InputParams.eModeExt = VppCompOnly;

            bool bOpaqueFlagChanged = false;
            for (mfxU32 j = 0; j < m_SessionArray.size(); j++)
            {
                if (m_SessionArray[j].bUseOpaqueMemory)
                {
                    m_SessionArray[j].bUseOpaqueMemory = false;
                    bOpaqueFlagChanged = true;
                }
            }

            if (bOpaqueFlagChanged)
            {
                msdk_printf(MSDK_STRING("WARNING: internal allocators were disabled because of composition+rendering requirement \n\n"));
            }
        }
        else if (0 == msdk_strncmp(MSDK_STRING("-vpp_comp_dump"), argv[i], msdk_strlen(MSDK_STRING("-vpp_comp_dump"))))
        {
            VAL_CHECK(i+1 == argc, i, argv[i]);
            i++;
            SIZE_CHECK((msdk_strlen(argv[i])+1) > MSDK_ARRAY_LEN(InputParams.strDumpVppCompFile));
            msdk_opt_read(argv[i], InputParams.strDumpVppCompFile);
        }
#if defined(LIBVA_X11_SUPPORT)
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-rx11")))
        {
            InputParams.libvaBackend = MFX_LIBVA_X11;
        }
#endif

#if defined(LIBVA_WAYLAND_SUPPORT)
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-rwld")))
        {
            InputParams.nRenderWinX = 0;
            InputParams.nRenderWinY = 0;
            InputParams.bPerfMode = false;
            InputParams.libvaBackend = MFX_LIBVA_WAYLAND;
        }
#endif

#if defined(LIBVA_DRM_SUPPORT)
        else if (0 == msdk_strncmp(argv[i], MSDK_STRING("-rdrm"), 5))
        {
            InputParams.libvaBackend = MFX_LIBVA_DRM_MODESET;
            InputParams.monitorType = getMonitorType(&argv[i][5]);
            if (argv[i][5]) {
                if (argv[i][5] != '-') {
                    PrintError(MSDK_STRING("unsupported monitor type"));
                    return MFX_ERR_UNSUPPORTED;
                }
                InputParams.monitorType = getMonitorType(&argv[i][6]);
                if (InputParams.monitorType >= MFX_MONITOR_MAXNUMBER) {
                    PrintError(MSDK_STRING("unsupported monitor type"));
                    return MFX_ERR_UNSUPPORTED;
                }
            } else {
                InputParams.monitorType = MFX_MONITOR_AUTO; // that's case of "-rdrm" pure option
            }
        }
#endif
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-ext_allocator")))
        {
            InputParams.bUseOpaqueMemory = false;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-vpp::sys")))
        {
            InputParams.VppOutPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-vpp::vid")))
        {
            InputParams.VppOutPattern = MFX_IOPATTERN_OUT_VIDEO_MEMORY;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-vpp_comp_dst_x")))
        {
            VAL_CHECK(i + 1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.nVppCompDstX))
            {
                PrintError(MSDK_STRING("vpp_comp_dst_x %s is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
            if (InputParams.eModeExt != VppComp)
                InputParams.eModeExt = VppComp;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-vpp_comp_dst_y")))
        {
            VAL_CHECK(i + 1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.nVppCompDstY))
            {
                PrintError(MSDK_STRING("-vpp_comp_dst_y %s is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
            if (InputParams.eModeExt != VppComp)
                InputParams.eModeExt = VppComp;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-vpp_comp_dst_w")))
        {
            VAL_CHECK(i + 1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.nVppCompDstW))
            {
                PrintError(MSDK_STRING("-vpp_comp_dst_w %s is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
            if (InputParams.eModeExt != VppComp)
                InputParams.eModeExt = VppComp;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-vpp_comp_dst_h")))
        {
            VAL_CHECK(i + 1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.nVppCompDstH))
            {
                PrintError(MSDK_STRING("-vpp_comp_dst_h %s is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
            if (InputParams.eModeExt != VppComp)
                InputParams.eModeExt = VppComp;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-vpp_comp_src_w")))
        {
            VAL_CHECK(i + 1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.nVppCompSrcW))
            {
                PrintError(MSDK_STRING("-vpp_comp_src_w %s is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
            if (InputParams.eModeExt != VppComp)
                InputParams.eModeExt = VppComp;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-vpp_comp_src_h")))
        {
            VAL_CHECK(i + 1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.nVppCompSrcH))
            {
                PrintError(MSDK_STRING("-vpp_comp_src_h %s is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
            if (InputParams.eModeExt != VppComp)
                InputParams.eModeExt = VppComp;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-vpp_comp_num_tiles")))
        {
            VAL_CHECK(i + 1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.numTiles4Comp))
            {
                PrintError(MSDK_STRING("-vpp_comp_num_tiles %s is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
            if (InputParams.eModeExt != VppCompOnly)
                InputParams.eModeExt = VppCompOnly;
        }
#if MFX_VERSION >= 1023
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-vpp_comp_tile_id")))
        {
            VAL_CHECK(i + 1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.nVppCompTileId))
            {
                PrintError(MSDK_STRING("-vpp_comp_tile_id %s is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
#endif
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-vpp_comp_render")))
        {
            if (InputParams.eModeExt != VppComp)
                InputParams.eModeExt = VppComp;
        }
#if MFX_VERSION >= 1022
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-dec_postproc")))
        {
            InputParams.bDecoderPostProcessing = true;
            if (InputParams.eModeExt != VppComp)
                InputParams.eModeExt = VppComp;
        }
#endif //MFX_VERSION >= 1022
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-n")))
        {
            VAL_CHECK(i+1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.MaxFrameNumber))
            {
                PrintError(MSDK_STRING("-n %s is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-angle")))
        {
            VAL_CHECK(i+1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.nRotationAngle))
            {
                PrintError(MSDK_STRING("-angle %s is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
            if (InputParams.strVPPPluginDLLPath[0] == '\0') {
                msdk_opt_read(MSDK_CPU_ROTATE_PLUGIN, InputParams.strVPPPluginDLLPath);
            }
        }
#if (MFX_VERSION >= 1025)
        else if(0 == msdk_strcmp(argv[i], MSDK_STRING("-mfe_frames")))
        {
            VAL_CHECK(i+1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.numMFEFrames))
            {
                PrintError(MSDK_STRING("-mfe_frames %s num frames is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if(0 == msdk_strcmp(argv[i], MSDK_STRING("-mfe_mode")))
        {
            VAL_CHECK(i+1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.MFMode))
            {
                PrintError(MSDK_STRING("-mfe_mode %s is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if(0 == msdk_strcmp(argv[i], MSDK_STRING("-mfe_timeout")))
        {
            VAL_CHECK(i+1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.mfeTimeout))
            {
                PrintError(MSDK_STRING("-mfe_timeout %s is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
#endif
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-timeout")))
        {
            VAL_CHECK(i+1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.nTimeout))
            {
                PrintError(MSDK_STRING("-timeout %s is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
            skipped+=2;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-dump")))
        {
            VAL_CHECK(i + 1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.strMfxParamsDumpFile))
            {
                PrintError(MSDK_STRING("Dump file name \"%s\" is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-robust")))
        {
            InputParams.bRobustFlag = true;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-opencl")))
        {
            msdk_opt_read(MSDK_OCL_ROTATE_PLUGIN, InputParams.strVPPPluginDLLPath);
            InputParams.bOpenCL = true;
        }

        // output PicStruct
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-la_ext")))
        {
            InputParams.bEnableExtLA = true;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-la")))
        {
            InputParams.bLABRC = true;
            InputParams.nRateControlMethod = MFX_RATECONTROL_LA;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-vbr")))
        {
            InputParams.nRateControlMethod = MFX_RATECONTROL_VBR;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-bpyr")))
        {
            InputParams.bEnableBPyramid = true;
        }

        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-lad")))
        {
            VAL_CHECK(i+1 == argc, i, argv[i]);
            InputParams.nRateControlMethod = MFX_RATECONTROL_LA;
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.nLADepth))
            {
                PrintError(MSDK_STRING("look ahead depth \"%s\" is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-pe")))
        {
            VAL_CHECK(i + 1 == argc, i, argv[i]);
            InputParams.encoderPluginParams = ParsePluginGuid(argv[i + 1]);
            if (AreGuidsEqual(InputParams.encoderPluginParams.pluginGuid, MSDK_PLUGINGUID_NULL))
            {
                PrintError(MSDK_STRING("Invalid encoder guid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
            i++;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-pd")))
        {
            VAL_CHECK(i + 1 == argc, i, argv[i]);
            InputParams.decoderPluginParams = ParsePluginGuid(argv[i + 1]);
            if (AreGuidsEqual(InputParams.encoderPluginParams.pluginGuid, MSDK_PLUGINGUID_NULL))
            {
                PrintError(MSDK_STRING("Invalid decoder guid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
            i++;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-override_decoder_framerate")))
        {
            VAL_CHECK(i+1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.dDecoderFrameRateOverride))
            {
                PrintError(MSDK_STRING("-n \"%s\" is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-override_encoder_framerate")))
        {
            VAL_CHECK(i+1 == argc, i, argv[i]);
            i++;
            if (MFX_ERR_NONE != msdk_opt_read(argv[i], InputParams.dEncoderFrameRateOverride))
            {
                PrintError(MSDK_STRING("-n \"%s\" is invalid"), argv[i]);
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-gpucopy::on")))
        {
            InputParams.nGpuCopyMode = MFX_GPUCOPY_ON;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-gpucopy::off")))
        {
            InputParams.nGpuCopyMode = MFX_GPUCOPY_OFF;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-repartitioncheck::on")))
        {
            InputParams.RepartitionCheckMode = MFX_CODINGOPTION_ON;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-repartitioncheck::off")))
        {
            InputParams.RepartitionCheckMode = MFX_CODINGOPTION_OFF;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-cqp")))
        {
            InputParams.nRateControlMethod = MFX_RATECONTROL_CQP;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-qpi")))
        {
            VAL_CHECK(i + 1 == argc, i, argv[i]);
            if (MFX_ERR_NONE != msdk_opt_read(argv[++i], InputParams.nQPI))
            {
                PrintError(MSDK_STRING("Quantizer for I frames is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-qpp")))
        {
            VAL_CHECK(i + 1 == argc, i, argv[i]);
            if (MFX_ERR_NONE != msdk_opt_read(argv[++i], InputParams.nQPP))
            {
                PrintError(MSDK_STRING("Quantizer for P frames is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-qpb")))
        {
            VAL_CHECK(i + 1 == argc, i, argv[i]);
            if (MFX_ERR_NONE != msdk_opt_read(argv[++i], InputParams.nQPB))
            {
                PrintError(MSDK_STRING("Quantizer for B frames is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-qsv-ff")))
        {
            InputParams.enableQSVFF=true;
        }
#if (MFX_VERSION >= 1024)
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-extbrc::on")))
        {
            InputParams.nExtBRC = EXTBRC_ON;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-extbrc::off")))
        {
            InputParams.nExtBRC = EXTBRC_OFF;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-extbrc::implicit")))
        {
            InputParams.nExtBRC = EXTBRC_IMPLICIT;
        }
#endif
#if (MFX_VERSION >= MFX_VERSION_NEXT)
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-ExtBrcAdaptiveLTR:on")))
        {
            InputParams.ExtBrcAdaptiveLTR = MFX_CODINGOPTION_ON;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-ExtBrcAdaptiveLTR:off")))
        {
            InputParams.ExtBrcAdaptiveLTR = MFX_CODINGOPTION_OFF;
        }

#endif
        MOD_SMT_PARSE_INPUT
        else if((stsExtBuf = CVPPExtBuffersStorage::ParseCmdLine(argv,argc,i,&InputParams,skipped))
            !=MFX_ERR_MORE_DATA)
        {
            if(stsExtBuf==MFX_ERR_UNSUPPORTED)
            {
                return MFX_ERR_UNSUPPORTED;
            }
        }
#ifdef ENABLE_MCTF
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-mctf")))
        {
            ParseMCTFParams(argv, argc, i, &InputParams);
        }
#endif
        else
        {
            PrintError(MSDK_STRING("Invalid input argument number %d %s"), i, argv[i]);
            return MFX_ERR_UNSUPPORTED;
        }
    }

    if (skipped < argc)
    {
        sts = VerifyAndCorrectInputParams(InputParams);
        MSDK_CHECK_STATUS(sts, "VerifyAndCorrectInputParams failed");
        m_SessionArray.push_back(InputParams);
    }

    return MFX_ERR_NONE;

} //mfxStatus CmdProcessor::ParseParamsForOneSession(msdk_char *pLine, mfxU32 length)

mfxStatus CmdProcessor::ParseOption__set(msdk_char* strCodecType, msdk_char* strPluginGuid)
{
    mfxU32 codecid = 0;
    mfxU32 type = 0;
    sPluginParams pluginParams;

    //Parse codec type - decoder or encoder
    if (0 == msdk_strncmp(MSDK_STRING("-i::"), strCodecType, 4))
    {
        type = MSDK_VDECODE;
    }
    else if (0 == msdk_strncmp(MSDK_STRING("-o::"), strCodecType, 4))
    {
        type = MSDK_VENCODE;
    }
    else
    {
        msdk_printf(MSDK_STRING("error: incorrect definition codec type (must be -i:: or -o::)\n"));
        return MFX_ERR_UNSUPPORTED;
    }

    if (StrFormatToCodecFormatFourCC(strCodecType+4, codecid) != MFX_ERR_NONE)
    {
        msdk_printf(MSDK_STRING("error: codec is unknown\n"));
        return MFX_ERR_UNSUPPORTED;
    }

    if (!IsPluginCodecSupported(codecid))
    {
        msdk_printf(MSDK_STRING("error: codec is unsupported\n"));
        return MFX_ERR_UNSUPPORTED;
    }

    pluginParams = ParsePluginGuid(strPluginGuid);
    if (AreGuidsEqual(pluginParams.pluginGuid, MSDK_PLUGINGUID_NULL))
    {
        msdk_printf(MSDK_STRING("error: invalid codec guid\n"));
        return MFX_ERR_UNSUPPORTED;
    }

    if (type == MSDK_VDECODE)
        m_decoderPlugins.insert(std::pair<mfxU32, sPluginParams>(codecid, pluginParams));
    else
        m_encoderPlugins.insert(std::pair<mfxU32, sPluginParams>(codecid, pluginParams));

    return MFX_ERR_NONE;
};

mfxStatus CmdProcessor::VerifyAndCorrectInputParams(TranscodingSample::sInputParams &InputParams)
{
    if (0 == msdk_strlen(InputParams.strSrcFile) && (InputParams.eMode == Sink || InputParams.eMode == Native))
    {
        PrintError(MSDK_STRING("Source file name not found"));
        return MFX_ERR_UNSUPPORTED;
    };

    if ( 0 == msdk_strlen(InputParams.strDstFile)
      && (InputParams.eMode == Source || InputParams.eMode == Native || InputParams.eMode == VppComp) && InputParams.eModeExt != VppCompOnly)
    {
        PrintError(MSDK_STRING("Destination file name not found"));
        return MFX_ERR_UNSUPPORTED;
    };

    if (MFX_CODEC_JPEG != InputParams.EncodeId && MFX_CODEC_MPEG2 != InputParams.EncodeId &&
        MFX_CODEC_AVC != InputParams.EncodeId && MFX_CODEC_HEVC != InputParams.EncodeId &&
        MFX_CODEC_VP9 != InputParams.EncodeId && MFX_CODEC_DUMP != InputParams.EncodeId &&
        InputParams.eMode != Sink && InputParams.eModeExt != VppCompOnly)
    {
        PrintError(MSDK_STRING("Unknown encoder\n"));
        return MFX_ERR_UNSUPPORTED;
    }

    if (MFX_CODEC_MPEG2 != InputParams.DecodeId &&
       MFX_CODEC_AVC != InputParams.DecodeId &&
       MFX_CODEC_HEVC != InputParams.DecodeId &&
       MFX_CODEC_VC1 != InputParams.DecodeId &&
       MFX_CODEC_JPEG != InputParams.DecodeId &&
       MFX_CODEC_VP9 != InputParams.DecodeId &&
       MFX_CODEC_RGB4 != InputParams.DecodeId &&
       InputParams.eMode != Source)
    {
        PrintError(MSDK_STRING("Unknown decoder\n"));
        return MFX_ERR_UNSUPPORTED;
    }

    if (MFX_CODEC_RGB4 == InputParams.DecodeId &&
        ( !InputParams.nVppCompSrcH ||
         !InputParams.nVppCompSrcW ))
    {
        PrintError(MSDK_STRING("VppCompSrcH and VppCompSrcW must be specified in case of -i::rgb4_frame\n"));
        return MFX_ERR_UNSUPPORTED;
    }

    if((!InputParams.FRCAlgorithm && !InputParams.bEnableDeinterlacing) && InputParams.dVPPOutFramerate)
    {
        msdk_printf(MSDK_STRING("-f option is ignored, it can be used with FRC or deinterlace options only. \n"));
        InputParams.dVPPOutFramerate=0;
    }

    if(InputParams.FRCAlgorithm && InputParams.bEnableExtLA)
    {
        PrintError(MSDK_STRING("-la_ext and FRC options cannot be used together\n"));
        return MFX_ERR_UNSUPPORTED;
    }

    if (InputParams.nQuality && InputParams.EncodeId && (MFX_CODEC_JPEG != InputParams.EncodeId))
    {
        PrintError(MSDK_STRING("-q option is supported only for JPEG encoder\n"));
        return MFX_ERR_UNSUPPORTED;
    }

    if ((InputParams.nTargetUsage || InputParams.nBitRate) && (MFX_CODEC_JPEG == InputParams.EncodeId))
    {
        PrintError(MSDK_STRING("-b and -u options are supported only for H.264, MPEG2 and MVC encoders. For JPEG encoder use -q\n"));
        return MFX_ERR_UNSUPPORTED;
    }

    // valid target usage range is: [MFX_TARGETUSAGE_BEST_QUALITY .. MFX_TARGETUSAGE_BEST_SPEED] (at the moment [1..7])
    if ((InputParams.nTargetUsage < MFX_TARGETUSAGE_BEST_QUALITY) ||
        (InputParams.nTargetUsage > MFX_TARGETUSAGE_BEST_SPEED) )
    {
        if (InputParams.nTargetUsage == MFX_TARGETUSAGE_UNKNOWN)
        {
            // if user did not specified target usage - use balanced
            InputParams.nTargetUsage = MFX_TARGETUSAGE_BALANCED;
        }
        else
        {
            PrintError(NULL,"Unsupported target usage");
            // report error if unsupported target usage was set
            return MFX_ERR_UNSUPPORTED;
        }
    }

    // Ignoring user-defined Async Depth for LA
    if (InputParams.nMaxSliceSize)
    {
        InputParams.nAsyncDepth = 1;
    }

    if (InputParams.bLABRC && !(InputParams.libType & MFX_IMPL_HARDWARE_ANY))
    {
        PrintError(MSDK_STRING("Look ahead BRC is supported only with -hw option!"));
        return MFX_ERR_UNSUPPORTED;
    }

    if (InputParams.bLABRC && (InputParams.EncodeId != MFX_CODEC_AVC) && (InputParams.eMode == Source))
    {
        PrintError(MSDK_STRING("Look ahead BRC is supported only with H.264 encoder!"));
        return MFX_ERR_UNSUPPORTED;
    }

    if (InputParams.nLADepth && (InputParams.nLADepth < 10))
    {
        if ((InputParams.nLADepth != 1) || (!InputParams.nMaxSliceSize))
        {
            PrintError(MSDK_STRING("Unsupported value of -lad parameter, must be >= 10, or 1 in case of -mss option!"));
            return MFX_ERR_UNSUPPORTED;
        }
    }

    if (InputParams.nRateControlMethod == 0)
    {
        InputParams.nRateControlMethod = MFX_RATECONTROL_CBR;
    }

    if ((InputParams.nMaxSliceSize) && !(InputParams.libType & MFX_IMPL_HARDWARE_ANY))
    {
        PrintError(MSDK_STRING("MaxSliceSize option is supported only with -hw option!"));
        return MFX_ERR_UNSUPPORTED;
    }
    if ((InputParams.nMaxSliceSize) && (InputParams.nSlices))
    {
        PrintError(MSDK_STRING("-mss and -l options are not compatible!"));
    }
    if ((InputParams.nMaxSliceSize) && (InputParams.EncodeId != MFX_CODEC_AVC))
    {
        PrintError(MSDK_STRING("MaxSliceSize option is supported only with H.264 encoder!"));
        return MFX_ERR_UNSUPPORTED;
    }

    if(InputParams.enableQSVFF && InputParams.eMode == Sink)
    {
        msdk_printf(MSDK_STRING("WARNING: -qsv-ff option is not valid for decoder-only sessions, this parameter will be ignored.\n"));
    }

    std::map<mfxU32, sPluginParams>::iterator it;

    // Set decoder plugins parameters only if they were not set before
    if (!memcmp(InputParams.decoderPluginParams.pluginGuid.Data, MSDK_PLUGINGUID_NULL.Data, sizeof(MSDK_PLUGINGUID_NULL)) &&
         !strcmp(InputParams.decoderPluginParams.strPluginPath,""))
    {
        it = m_decoderPlugins.find(InputParams.DecodeId);
        if (it != m_decoderPlugins.end())
            InputParams.decoderPluginParams = it->second;
    }
    else
    {
        // Decoding plugin was set manually, so let's check if codec supports plugins
        if (!IsPluginCodecSupported(InputParams.DecodeId))
        {
            msdk_printf(MSDK_STRING("error: decoder does not support plugins\n"));
            return MFX_ERR_UNSUPPORTED;
        }

    }

    // Set encoder plugins parameters only if they were not set before
    if (!memcmp(InputParams.encoderPluginParams.pluginGuid.Data, MSDK_PLUGINGUID_NULL.Data, sizeof(MSDK_PLUGINGUID_NULL)) &&
        !strcmp(InputParams.encoderPluginParams.strPluginPath, ""))
    {
        it = m_encoderPlugins.find(InputParams.EncodeId);
        if (it != m_encoderPlugins.end())
            InputParams.encoderPluginParams = it->second;
    }
    else
    {
        // Decoding plugin was set manually, so let's check if codec supports plugins
        if (!IsPluginCodecSupported(InputParams.DecodeId))
        {
            msdk_printf(MSDK_STRING("error: encoder does not support plugins\n"));
            return MFX_ERR_UNSUPPORTED;
        }

    }

    if(InputParams.EncoderFourCC && InputParams.eMode == Sink)
    {
        msdk_printf(MSDK_STRING("WARNING: -ec option is used in session without encoder, this parameter will be ignored \n"));
    }

    if(InputParams.DecoderFourCC && InputParams.eMode != Native && InputParams.eMode != Sink)
    {
        msdk_printf(MSDK_STRING("WARNING: -dc option is used in session without decoder, this parameter will be ignored \n"));
    }

    if(InputParams.FRCAlgorithm && InputParams.eMode == Sink)
    {
        PrintError(NULL,MSDK_STRING("-FRC option should not be used in -o::sink pipelines \n"));
        return MFX_ERR_UNSUPPORTED;
    }

    if(InputParams.EncoderFourCC && InputParams.EncoderFourCC != MFX_FOURCC_NV12 &&
        InputParams.EncoderFourCC != MFX_FOURCC_RGB4 && InputParams.EncoderFourCC != MFX_FOURCC_YUY2 &&
        InputParams.EncodeId == MFX_CODEC_DUMP)
    {
        PrintError(MSDK_STRING("-o::raw option can be used with NV12, RGB4 and YUY2 color formats only.\n"));
        return MFX_ERR_UNSUPPORTED;
    }

    return MFX_ERR_NONE;
} //mfxStatus CmdProcessor::VerifyAndCorrectInputParams(TranscodingSample::sInputParams &InputParams)

bool  CmdProcessor::GetNextSessionParams(TranscodingSample::sInputParams &InputParams)
{
    if (!m_SessionArray.size())
        return false;
    if (m_SessionParamId == m_SessionArray.size())
    {
        return false;
    }
    InputParams = m_SessionArray[m_SessionParamId];

    m_SessionParamId++;
    return true;

} //bool  CmdProcessor::GetNextSessionParams(TranscodingSample::sInputParams &InputParams)
