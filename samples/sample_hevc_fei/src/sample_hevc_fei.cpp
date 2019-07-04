/******************************************************************************\
Copyright (c) 2017-2018, Intel Corporation
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

#include "version.h"
#include "pipeline_hevc_fei.h"

mfxStatus CheckOptions(const sInputParams& params, const msdk_char* appName);
void AdjustOptions(sInputParams& params);

// if last option requires value, check that it passed to app
#define CHECK_NEXT_VAL(val, argName, app) \
{ \
    if (val) \
    { \
        msdk_printf(MSDK_STRING("ERROR: Input argument \"%s\" require more parameters\n"), argName); \
        PrintHelp(app, NULL); \
        return MFX_ERR_UNSUPPORTED;\
    } \
}

#define PARSE_CHECK(sts, optName, isParseInvalid) \
{ \
    if (MFX_ERR_NONE != sts) \
    { \
        msdk_printf(MSDK_STRING("ERROR: %s is invalid\n"), optName); \
        isParseInvalid = true; \
    } \
}

constexpr mfxU32 MinNumberOfOptions = 6; // app_name + input + file + output + file + component

void PrintHelp(const msdk_char *strAppName, const msdk_char *strErrorMessage)
{
    msdk_printf(MSDK_STRING("\nIntel(R) Media SDK HEVC FEI Encoding Sample Version %s\n\n"), GetMSDKSampleVersion().c_str());

    if (strErrorMessage)
    {
        msdk_printf(MSDK_STRING("ERROR: %s\n"), strErrorMessage);
    }
    msdk_printf(MSDK_STRING("Usage: %s [<options>] -i InputFile -o OutputEncodedFile -w width -h height -encode\n"), strAppName);
    msdk_printf(MSDK_STRING("\n"));
    msdk_printf(MSDK_STRING("Options: \n"));
    msdk_printf(MSDK_STRING("   [?] - print this message\n"));
    msdk_printf(MSDK_STRING("\n"));
    msdk_printf(MSDK_STRING("Specify input/output: \n"));
    msdk_printf(MSDK_STRING("Video sequences: \n"));
    msdk_printf(MSDK_STRING("   [-i <file-name>]                      - input YUV file\n"));
    msdk_printf(MSDK_STRING("   [-i::h264|mpeg2|vc1|h265 <file-name>] - input file and decoder type\n"));
    msdk_printf(MSDK_STRING("   [-o <file-name>]                      - output h265 encoded file\n"));
    msdk_printf(MSDK_STRING("   [-w]        - width of input YUV file\n"));
    msdk_printf(MSDK_STRING("   [-h]        - height of input YUV file\n"));
    msdk_printf(MSDK_STRING("   [-nv12]     - input is in NV12 color format, if not specified YUV420 is expected\n"));
    msdk_printf(MSDK_STRING("   [-tff|bff]  - input stream is interlaced, top|bottom field first, if not specified progressive is expected\n"));
    msdk_printf(MSDK_STRING("Video input processing: \n"));
    msdk_printf(MSDK_STRING("   [-EncodedOrder]    - use app-level reordering to encoded order (default is display; ENCODE only)\n"));
    msdk_printf(MSDK_STRING("   [-n number]        - number of input frames to process\n"));
    msdk_printf(MSDK_STRING("                        if stream has fewer frames than requested, sample repeats frames from stream's beginning\n"));
    msdk_printf(MSDK_STRING("   [-timeout seconds] - set time to run processing in seconds\n"));

    msdk_printf(MSDK_STRING("FEI files: \n"));
    msdk_printf(MSDK_STRING("   [-mbstat <file-name>]        - use this to output per MB distortions for each frame after PreENC\n"));
    msdk_printf(MSDK_STRING("   [-mvout <file-name>]         - use this to output MV predictors after PreENC\n"));
    msdk_printf(MSDK_STRING("   [-mvout::format <file-name>] - use this to output MV predictors after PreENC in an internal format\n"));
    msdk_printf(MSDK_STRING("                                  (downsampling is not allowed with this option).\n"));
    msdk_printf(MSDK_STRING("   [-mvpin <file-name>]         - use this to input MV predictors for ENCODE (Encoded Order will be enabled automatically).\n"));
    msdk_printf(MSDK_STRING("   [-mvpin::format <file-name>] - use this to input MVs for ENCODE before repacking in internal format\n"));
    msdk_printf(MSDK_STRING("                                  (Encoded Order will be enabled automatically).\n"));
    msdk_printf(MSDK_STRING("   [-repackctrl <file-name>]    - use this to input encode repack ctrl file\n"));
    msdk_printf(MSDK_STRING("   [-repackstat <file-name>]    - use this to output encode repack stat file\n"));
    msdk_printf(MSDK_STRING("\n"));
    msdk_printf(MSDK_STRING("Specify pipeline: \n"));
    msdk_printf(MSDK_STRING("   [-encode]            - use extended FEI interface ENC+PAK (FEI ENCODE) (RC is forced to constant QP)\n"));
    msdk_printf(MSDK_STRING("   [-preenc dsStrength] - use extended FEI interface PREENC\n"));
    msdk_printf(MSDK_STRING("                          if ds_strength parameter is missed or equal 1, PREENC is used on the full resolution\n"));
    msdk_printf(MSDK_STRING("                          otherwise PREENC is used on downscaled (by VPP resize in ds_strength times) surfaces.\n"));
    msdk_printf(MSDK_STRING("                          Also forces Encoded Order\n"));

    msdk_printf(MSDK_STRING("\n"));
    msdk_printf(MSDK_STRING("BRC (optional): \n"));
    msdk_printf(MSDK_STRING("   [-ExtBRC] - enables external BRC\n"));
    msdk_printf(MSDK_STRING("   [-b kbps] - target bitrate (in Kbits per second)\n"));

    msdk_printf(MSDK_STRING("\n"));
    msdk_printf(MSDK_STRING("VPP settings (optional): \n"));
    msdk_printf(MSDK_STRING("   [-dstw width]     - destination picture width, invokes VPP resizing\n"));
    msdk_printf(MSDK_STRING("   [-dsth height]    - destination picture height, invokes VPP resizing\n"));
    msdk_printf(MSDK_STRING("   [-fieldSplitting] - use VPP field splitting (works only with interlaced input)\n"));

    msdk_printf(MSDK_STRING("\n"));
    msdk_printf(MSDK_STRING("Encoding settings: \n"));
    msdk_printf(MSDK_STRING("   [-qp qp_value]     - QP value for frames (default is 26)\n"));
    msdk_printf(MSDK_STRING("   [-DisableQPOffset] - disable QP offset per pyramid layer\n"));
    msdk_printf(MSDK_STRING("   [-l numSlices]     - number of slices \n"));
    msdk_printf(MSDK_STRING("   [-profile value]   - codec profile\n"));
    msdk_printf(MSDK_STRING("   [-level value]     - codec level\n"));
    msdk_printf(MSDK_STRING("   [-f frameRate]     - video frame rate (frames per second)\n"));
    msdk_printf(MSDK_STRING("   [-PicTimingSEI]    - inserts picture timing SEI\n"));

    msdk_printf(MSDK_STRING("GOP structure: \n"));
    msdk_printf(MSDK_STRING("   [-r (-GopRefDist) distance]       - Distance between I- or P- key frames (1 means no B-frames) (0 - by default(I frames))\n"));
    msdk_printf(MSDK_STRING("   [-num_ref (-NumRefFrame) numRefs] - number of available reference frames (DPB size)\n"));
    msdk_printf(MSDK_STRING("   [-g size]                         - GOP size (1(default) means I-frames only)\n"));
    msdk_printf(MSDK_STRING("   [-gop_opt closed|strict]          - GOP optimization flags (can be used together)\n"));
    msdk_printf(MSDK_STRING("   [-idr_interval size]              - if IdrInterval = 0, then only first I-frame is an IDR-frame\n"));
    msdk_printf(MSDK_STRING("                                       if IdrInterval = 1, then every I - frame is an IDR - frame\n"));
    msdk_printf(MSDK_STRING("                                       if IdrInterval = 2, then every other I - frame is an IDR - frame, etc (default is 0)\n"));

    msdk_printf(MSDK_STRING("References structure: \n"));
    msdk_printf(MSDK_STRING("   [-active_ref_lists_par <file-name>] - par - file for reference lists + reordering. Each line :\n"));
    msdk_printf(MSDK_STRING("                                         <POC> <FrameType> <PicStruct> | 8 <reference POC> in L0 list | then L1 | 16 DPB\n"));
    msdk_printf(MSDK_STRING("   [-NumRefActiveP   numRefs]          - number of maximum allowed references for P frames (valid range is [1, 3])\n"));
    msdk_printf(MSDK_STRING("   [-NumRefActiveBL0 numRefs]          - number of maximum allowed backward references for B frames (valid range is [1, 3])\n"));
    msdk_printf(MSDK_STRING("   [-NumRefActiveBL1 numRefs]          - number of maximum allowed forward references for B frames (only 1 is supported)\n"));
    msdk_printf(MSDK_STRING("   [-gpb:<on,off>]                     - make HEVC encoder use regular P-frames (off) or GPB (on) (on by default)\n"));
    msdk_printf(MSDK_STRING("   [-ppyr:<on,off>]                    - enables P-pyramid (on by default)\n"));
    msdk_printf(MSDK_STRING("   [-bref]                             - arrange B frames in B-pyramid reference structure\n"));
    msdk_printf(MSDK_STRING("   [-nobref]                           - do not use B-pyramid (by default the decision is made by library)\n"));

    msdk_printf(MSDK_STRING("\n"));
    msdk_printf(MSDK_STRING("FEI specific settings: \n"));
    msdk_printf(MSDK_STRING("Predictors: \n"));
    msdk_printf(MSDK_STRING("   [-NumPredictorsL0 numPreds] - number of maximum L0 predictors (default - assign depending on the frame type)\n"));
    msdk_printf(MSDK_STRING("   [-NumPredictorsL1 numPreds] - number of maximum L1 predictors (default - assign depending on the frame type)\n"));
    msdk_printf(MSDK_STRING("   [-MultiPredL0 type]         - use internal L0 MV predictors (0 - no internal MV predictor, 1 - spatial internal MV predictors)\n"));
    msdk_printf(MSDK_STRING("   [-MultiPredL1 type]         - use internal L1 MV predictors (0 - no internal MV predictor, 1 - spatial internal MV predictors)\n"));
    msdk_printf(MSDK_STRING("   [-MVPBlockSize size]        - external MV predictor block size (0 - no MVP, 1 - MVP per 16x16, 2 - MVP per 32x32, 7 - use with -mvpin)\n"));
    msdk_printf(MSDK_STRING("   [-qrep]                     - quality  MV predictors repacking before encode\n"));

    msdk_printf(MSDK_STRING("Partitioning: \n"));
    msdk_printf(MSDK_STRING("   [-ForceCtuSplit]          - force splitting CTU into CU at least once\n"));
    msdk_printf(MSDK_STRING("   [-ForceCtuSplit:I]        - force splitting CTU into CU at least once on I-frames only\n"));
    msdk_printf(MSDK_STRING("   [-ForceCtuSplit:P]        - force splitting CTU into CU at least once on P/GPB-frames only\n"));
    msdk_printf(MSDK_STRING("   [-ForceCtuSplit:B]        - force splitting CTU into CU at least once on B-frames only\n"));
    msdk_printf(MSDK_STRING("   [-NumFramePartitions num] - number of partitions in frame that encoder processes concurrently (1, 2, 4 or 8)\n"));
    msdk_printf(MSDK_STRING("   [-NFP:I num]              - number of partitions in I frame that encoder processes concurrently (1, 2, 4 or 8)\n"));
    msdk_printf(MSDK_STRING("   [-NFP:P num]              - number of partitions in P/GPB frame that encoder processes concurrently (1, 2, 4 or 8)\n"));
    msdk_printf(MSDK_STRING("   [-NFP:B num]              - number of partitions in B frame that encoder processes concurrently (1, 2, 4 or 8)\n"));

    msdk_printf(MSDK_STRING("Force Flags: \n"));
    msdk_printf(MSDK_STRING("   [-FastIntraMode] - force encoder to skip HEVC-specific intra modes (use AVC modes only)\n"));
    msdk_printf(MSDK_STRING("   [-FastIntra:I]   - force encoder to skip HEVC-specific intra modes (use AVC modes only) on I-frames\n"));
    msdk_printf(MSDK_STRING("   [-FastIntra:P]   - force encoder to skip HEVC-specific intra modes (use AVC modes only) on P/GPB-frames\n"));
    msdk_printf(MSDK_STRING("   [-FastIntra:B]   - force encoder to skip HEVC-specific intra modes (use AVC modes only) on B-frames\n"));

    msdk_printf(MSDK_STRING("Motion Search: \n"));
    msdk_printf(MSDK_STRING("   [-SearchWindow value]         - specifies one of the predefined search path and window size. In range [1,5] (5 is default).\n"));
    msdk_printf(MSDK_STRING("                                   If zero value specified: -RefWidth / RefHeight, -LenSP are required\n"));
    msdk_printf(MSDK_STRING("   [-RefWidth width]             - width of search region (should be multiple of 4),\n"));
    msdk_printf(MSDK_STRING("                                   valid range is [20, 64] for one direction and [20, 32] for bidirectional search\n"));
    msdk_printf(MSDK_STRING("   [-RefHeight height]           - height of search region (should be multiple of 4),\n"));
    msdk_printf(MSDK_STRING("                                   valid range is [20, 64] for one direction and [20, 32] for bidirectional search\n"));
    msdk_printf(MSDK_STRING("   [-LenSP length]               - defines number of search units in search path. In range [1,63] (default is 57)\n"));
    msdk_printf(MSDK_STRING("   [-SearchPath value]           - defines shape of search path. 1 - diamond, 2 - full, 0 - default (full).\n"));
    msdk_printf(MSDK_STRING("   [-AdaptiveSearch]             - enables adaptive search\n"));
    msdk_printf(MSDK_STRING("   [-preenc::SearchWindow value] - specifies one of the predefined search path and window size for PreENC.\n"));
    msdk_printf(MSDK_STRING("                                   In range [1,5] (5 is default).\n"));

    msdk_printf(MSDK_STRING("\n"));
}

mfxStatus ParseInputString(msdk_char* strInput[], mfxU32 nArgNum, sInputParams& params)
{
    if (nArgNum < MinNumberOfOptions)
    {
        PrintHelp(strInput[0], MSDK_STRING("Not enough input parameters"));
        return MFX_ERR_UNSUPPORTED;
    }

    bool isParseInvalid = false; // show help once after all parsing errors will be processed

    // parse command line parameters
    for (mfxU32 i = 1; i < nArgNum; i++)
    {
        MSDK_CHECK_POINTER(strInput[i], MFX_ERR_UNDEFINED_BEHAVIOR);

        // process multi-character options
        if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-encode")))
        {
            params.bENCODE = true;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-preenc")))
        {
            params.bPREENC = true;
            i++; // try to parse preenc DS strength
            if (i >= nArgNum || MFX_ERR_NONE != msdk_opt_read(strInput[i], params.preencDSfactor))
            {
                params.preencDSfactor = 1;
                i--;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-EncodedOrder")))
        {
            params.bEncodedOrder = true;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-nv12")))
        {
            params.input.ColorFormat = MFX_FOURCC_NV12;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-qp")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.QP), "QP", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-i")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.input.strSrcFile), "Input file", isParseInvalid);
        }
        else if (0 == msdk_strncmp(MSDK_STRING("-i::"), strInput[i], msdk_strlen(MSDK_STRING("-i::"))))
        {
            mfxStatus sts = StrFormatToCodecFormatFourCC(strInput[i] + 4, params.input.DecodeId);
            if (sts != MFX_ERR_NONE)
            {
                PrintHelp(strInput[0], MSDK_STRING("ERROR: Failed to extract decoder type"));
                return MFX_ERR_UNSUPPORTED;
            }
            i++;

            if (msdk_strlen(strInput[i]) < MSDK_ARRAY_LEN(params.input.strSrcFile)){
                msdk_opt_read(strInput[i], params.input.strSrcFile);
            }
            else{
                PrintHelp(strInput[0], MSDK_STRING("ERROR: Too long input filename (limit is 1023 characters)!"));
                return MFX_ERR_UNSUPPORTED;
            }

            switch (params.input.DecodeId)
            {
            case MFX_CODEC_MPEG2:
            case MFX_CODEC_AVC:
            case MFX_CODEC_VC1:
            case MFX_CODEC_HEVC:
                break;
            default:
                PrintHelp(strInput[0], MSDK_STRING("ERROR: Unsupported encoded input (only AVC, HEVC, MPEG2, VC1 is supported)"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-o")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.strDstFile), "Output file", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-w")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.input.nWidth), "Width", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-h")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.input.nHeight), "Height", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-dstw")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.dstWidth), "Destination width", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-dsth")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.dstHeight), "Destination height", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-fieldSplitting")))
        {
            params.input.fieldSplitting = true;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-f")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.input.dFrameRate), "FrameRate", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-n")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.nNumFrames), "NumFrames", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-profile")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.CodecProfile), "CodecProfile", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-level")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.CodecLevel), "CodecLevel", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-g")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.nGopSize), "GopSize", isParseInvalid);
        }
        else if (   0 == msdk_strcmp(strInput[i], MSDK_STRING("-r"))
                 || 0 == msdk_strcmp(strInput[i], MSDK_STRING("-GopRefDist")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.nRefDist), "GopRefDist", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-l")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.nNumSlices), "NumSlices", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-mvpin")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.mvpInFile), "MVP in File", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-mvpin::format")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.mvpInFile), "MV in formatted File", isParseInvalid);
            params.bFormattedMVPin = true;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-mvout")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.mvoutFile), "MV out File", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-mvout::format")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.mvoutFile), "MV out formatted File", isParseInvalid);
            params.bFormattedMVout = true;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-mbstat")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.mbstatoutFile), "MB stat out File", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-active_ref_lists_par")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.refctrlInFile), "RefList ctrl File", isParseInvalid);
            params.bEncodedOrder = true;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-qrep")))
        {
            params.bQualityRepack = true;                  // to enable quality mode in repack
            params.preencCtrl.DisableStatisticsOutput = 0; // to gather statistics in preenc, needed for quality repack
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-ppyr:on")))
        {
            params.PRefType = MFX_P_REF_PYRAMID;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-ppyr:off")))
        {
            params.PRefType = MFX_P_REF_SIMPLE;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-bref")))
        {
            params.BRefType = MFX_B_REF_PYRAMID;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-nobref")))
        {
            params.BRefType = MFX_B_REF_OFF;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-idr_interval")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.nIdrInterval), "IdrInterval", isParseInvalid);
        }
        else if (   0 == msdk_strcmp(strInput[i], MSDK_STRING("-num_ref"))
                 || 0 == msdk_strcmp(strInput[i], MSDK_STRING("-NumRefFrame")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.nNumRef), "NumRefFrame", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-NumRefActiveP")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.NumRefActiveP), "NumRefActiveP", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-NumRefActiveBL0")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.NumRefActiveBL0), "NumRefActiveBL0", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-NumRefActiveBL1")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.NumRefActiveBL1), "NumRefActiveBL1", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-NumPredictorsL0")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.encodeCtrl.NumMvPredictors[0]), "NumPredictorsL0", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-NumPredictorsL1")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.encodeCtrl.NumMvPredictors[1]), "NumPredictorsL1", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-MultiPredL0")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.encodeCtrl.MultiPred[0]), "MultiPredL0", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-MultiPredL1")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.encodeCtrl.MultiPred[1]), "MultiPredL1", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-MVPBlockSize")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.encodeCtrl.MVPredictor), "MVPBlockSize", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-ForceCtuSplit")))
        {
            params.encodeCtrl.ForceCtuSplit = params.frameCtrl.CtrlI.ForceCtuSplit =
                params.frameCtrl.CtrlP.ForceCtuSplit = params.frameCtrl.CtrlB.ForceCtuSplit = 1;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-ForceCtuSplit:I")))
        {
            params.frameCtrl.CtrlI.ForceCtuSplit = 1;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-ForceCtuSplit:P")))
        {
            params.frameCtrl.CtrlP.ForceCtuSplit = 1;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-ForceCtuSplit:B")))
        {
            params.frameCtrl.CtrlB.ForceCtuSplit = 1;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-NumFramePartitions")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.encodeCtrl.NumFramePartitions), "NumFramePartitions", isParseInvalid);

            params.frameCtrl.CtrlI.NumFramePartitions =
                params.frameCtrl.CtrlP.NumFramePartitions =
                params.frameCtrl.CtrlB.NumFramePartitions = params.encodeCtrl.NumFramePartitions;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-NFP:I")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.frameCtrl.CtrlI.NumFramePartitions), "NumFramePartitions", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-NFP:P")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.frameCtrl.CtrlP.NumFramePartitions), "NumFramePartitions", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-NFP:B")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.frameCtrl.CtrlB.NumFramePartitions), "NumFramePartitions", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-FastIntraMode")))
        {
            params.encodeCtrl.FastIntraMode = params.frameCtrl.CtrlI.FastIntraMode =
                params.frameCtrl.CtrlP.FastIntraMode = params.frameCtrl.CtrlB.FastIntraMode = 1;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-FastIntra:I")))
        {
            params.frameCtrl.CtrlI.FastIntraMode = 1;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-FastIntra:P")))
        {
            params.frameCtrl.CtrlP.FastIntraMode = 1;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-FastIntra:B")))
        {
            params.frameCtrl.CtrlB.FastIntraMode = 1;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-tff")))
        {
            params.input.nPicStruct = MFX_PICSTRUCT_FIELD_TFF;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-bff")))
        {
            params.input.nPicStruct = MFX_PICSTRUCT_FIELD_BFF;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-gop_opt")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            ++i;
            if (0 == msdk_strcmp(strInput[i], MSDK_STRING("closed")))
            {
                params.nGopOptFlag |= MFX_GOP_CLOSED;
            }
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("strict")))
            {
                params.nGopOptFlag |= MFX_GOP_STRICT;
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-gpb:on")))
        {
            params.GPB = MFX_CODINGOPTION_ON;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-gpb:off")))
        {
            params.GPB = MFX_CODINGOPTION_OFF;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-PicTimingSEI")))
        {
            params.PicTimingSEI = MFX_CODINGOPTION_ON;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-AdaptiveSearch")))
        {
            params.encodeCtrl.AdaptiveSearch = params.preencCtrl.AdaptiveSearch = 1;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-SearchWindow")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.encodeCtrl.SearchWindow), "SearchWindow", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-preenc::SearchWindow")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.preencCtrl.SearchWindow), "SearchWindow", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-LenSP")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.encodeCtrl.LenSP), "LenSP", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-RefWidth")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.encodeCtrl.RefWidth), "RefWidth", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-RefHeight")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.encodeCtrl.RefHeight), "RefHeight", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-SearchPath")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.encodeCtrl.SearchPath), "SearchPath", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-ExtBRC")))
        {
            params.bExtBRC = true;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-b")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.TargetKbps), "Bitrate", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-DisableQPOffset")))
        {
            params.bDisableQPOffset = true;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-timeout")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.nTimeout), "timeout", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-repackctrl")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.repackctrlFile), "Repack ctrl File", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-repackstat")))
        {
            CHECK_NEXT_VAL(i + 1 >= nArgNum, strInput[i], strInput[0]);
            PARSE_CHECK(msdk_opt_read(strInput[++i], params.repackstatFile), "Repack stat File", isParseInvalid);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("?")))
        {
            PrintHelp(strInput[0], NULL);
            return MFX_ERR_UNSUPPORTED;
        }
        else
        {
            msdk_printf(MSDK_STRING("\nERROR: Unknown option `%s`\n\n"), strInput[i]);
            isParseInvalid = true;
        }
    }

    if (isParseInvalid)
    {
        PrintHelp(strInput[0], NULL);
        return MFX_ERR_UNSUPPORTED;
    }

    if (MFX_ERR_NONE != CheckOptions(params, strInput[0]))
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    AdjustOptions(params);

    return MFX_ERR_NONE;
}

mfxStatus CheckOptions(const sInputParams& params, const msdk_char* appName)
{
    if (0 == msdk_strlen(params.input.strSrcFile))
    {
        PrintHelp(appName, "Source file name not found");
        return MFX_ERR_UNSUPPORTED;
    }
    if ((0 == params.input.DecodeId) && (0 == params.input.nWidth || 0 == params.input.nHeight))
    {
        PrintHelp(appName, "-w -h is not specified");
        return MFX_ERR_UNSUPPORTED;
    }
    if (params.QP && params.bExtBRC)
    {
        PrintHelp(appName, "Invalid bitrate control (QP + ExtBRC is unsupported)");
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    if (params.QP > 51)
    {
        PrintHelp(appName, "Invalid QP value (must be in range [0, 51])");
        return MFX_ERR_UNSUPPORTED;
    }
    if (params.NumRefActiveP == 0 || params.NumRefActiveP > 3)
    {
        PrintHelp(appName, "Unsupported NumRefActiveP value (must be in range [1,3])");
        return MFX_ERR_UNSUPPORTED;
    }
    if (params.NumRefActiveBL0 == 0 || params.NumRefActiveBL0 > 3)
    {
        PrintHelp(appName, "Unsupported NumRefActiveBL0 value (must be in range [1,3])");
        return MFX_ERR_UNSUPPORTED;
    }
    if (params.NumRefActiveBL1 == 0 || params.NumRefActiveBL1 > 1)
    {
        PrintHelp(appName, "Unsupported NumRefActiveBL1 value (only 1 is supported)");
        return MFX_ERR_UNSUPPORTED;
    }
    if (params.encodeCtrl.NumMvPredictors[0] > 4)
    {
        PrintHelp(appName, "Unsupported NumMvPredictorsL0 value (must be in range [0,4])");
        return MFX_ERR_UNSUPPORTED;
    }
    if (params.encodeCtrl.NumMvPredictors[1] > 4)
    {
        PrintHelp(appName, "Unsupported NumMvPredictorsL1 value (must be in range [0,4])");
        return MFX_ERR_UNSUPPORTED;
    }
    std::list<mfxU16> nfp_values = {1, 2, 4, 8};
    if (std::find(nfp_values.begin(), nfp_values.end(), params.frameCtrl.CtrlI.NumFramePartitions) == nfp_values.end())
    {
        PrintHelp(appName, "Unsupported NumFramePartitions value for I frames (must be 1, 2, 4 or 8)");
        return MFX_ERR_UNSUPPORTED;
    }
    if (std::find(nfp_values.begin(), nfp_values.end(), params.frameCtrl.CtrlP.NumFramePartitions) == nfp_values.end())
    {
        PrintHelp(appName, "Unsupported NumFramePartitions value for P/GPB frames (must be 1, 2, 4 or 8)");
        return MFX_ERR_UNSUPPORTED;
    }
    if (std::find(nfp_values.begin(), nfp_values.end(), params.frameCtrl.CtrlB.NumFramePartitions) == nfp_values.end())
    {
        PrintHelp(appName, "Unsupported NumFramePartitions value for B frames (must be 1, 2, 4 or 8)");
        return MFX_ERR_UNSUPPORTED;
    }
    if (!params.bENCODE && !params.bPREENC)
    {
        PrintHelp(appName, "Pipeline is unsupported. Supported pipeline: ENCODE, PreENC, PreENC + ENCODE");
        return MFX_ERR_UNSUPPORTED;
    }
    if (params.bPREENC && 0 != msdk_strlen(params.mvpInFile))
    {
        PrintHelp(appName, "MV predictors from both PreENC and input file (mvpin) are not supported in one pipeline");
        return MFX_ERR_UNSUPPORTED;
    }
    if (0 == params.nGopSize)
    {
        PrintHelp(appName, "Gop structure is not specified (GopSize = 0)");
        return MFX_ERR_UNSUPPORTED;
    }
    if (params.nGopSize > 1 && 0 == params.nRefDist)
    {
        PrintHelp(appName, "Gop structure is invalid. Set GopRefDist option");
        return MFX_ERR_UNSUPPORTED;
    }
    if (params.bPREENC && params.preencDSfactor != 1 && params.preencDSfactor != 2
        && params.preencDSfactor != 4 && params.preencDSfactor != 8)
    {
        PrintHelp(appName, "Invalid DS strength value (must be 1, 2, 4 or 8)");
        return MFX_ERR_UNSUPPORTED;
    }
    if (params.preencDSfactor != 1 && params.bFormattedMVout)
    {
        PrintHelp(appName, "Dumping of MV predictors in internal format from PreENC with downsampling is unsupported.");
        return MFX_ERR_UNSUPPORTED;
    }
    if (params.encodeCtrl.MVPredictor != 0 && params.encodeCtrl.MVPredictor != 1 &&
        params.encodeCtrl.MVPredictor != 2 && params.encodeCtrl.MVPredictor != 7)
    {
        PrintHelp(appName, "Invalid MV predictor block size value.");
        return MFX_ERR_UNSUPPORTED;
    }

    if (params.encodeCtrl.SearchWindow > 5)
    {
        PrintHelp(appName, "Invalid SearchWindow value");
        return MFX_ERR_UNSUPPORTED;
    }
    if (params.encodeCtrl.SearchPath > 2)
    {
        PrintHelp(appName, "Invalid SearchPath value");
        return MFX_ERR_UNSUPPORTED;
    }
    if (params.encodeCtrl.LenSP > 63)
    {
        PrintHelp(appName, "Invalid LenSP value");
        return MFX_ERR_UNSUPPORTED;
    }

    if (params.encodeCtrl.RefWidth % 4 != 0 || params.encodeCtrl.RefHeight % 4 != 0
        // check only the highest possible limit regardless of gop structure
        || params.encodeCtrl.RefHeight > 64 || params.encodeCtrl.RefWidth > 64
        || params.encodeCtrl.RefWidth * params.encodeCtrl.RefHeight > 2048)
    {
        PrintHelp(appName, "Invalid RefWidth/RefHeight value");
        return MFX_ERR_UNSUPPORTED;
    }

    return MFX_ERR_NONE;
}

void AdjustOptions(sInputParams& params)
{
    params.input.dFrameRate = tune(params.input.dFrameRate, 0.0, 30.0);
    params.nNumSlices       = tune(params.nNumSlices, 0, 1);
    params.nIdrInterval     = tune(params.nIdrInterval, 0, 0xffff);

    if (params.nRefDist < 2) // avoid impact of NumRefActiveBL0/L1 in gop with P/GPB only
    {
        params.NumRefActiveBL0 = params.NumRefActiveBL1 = 0;
    }
    mfxU16 nMinRefFrame     = std::max(params.NumRefActiveP, (mfxU16)(params.NumRefActiveBL0 + params.NumRefActiveBL1));
    if (nMinRefFrame > params.nNumRef)
        params.nNumRef      = nMinRefFrame;

    // PreENC works only in encoder order mode
    // ENCODE uses display order by default, but input MV predictors are in encoded order.
    if (params.bPREENC || (params.bENCODE && 0 != msdk_strlen(params.mvpInFile)))
    {
        if (!params.bEncodedOrder)
            msdk_printf(MSDK_STRING("WARNING: Encoded order enabled.\n"));
        params.bEncodedOrder = true;
    }

    if (params.encodeCtrl.SearchWindow && (params.encodeCtrl.SearchPath || params.encodeCtrl.LenSP
                                        || params.encodeCtrl.RefWidth || params.encodeCtrl.RefHeight))
    {
        msdk_printf(MSDK_STRING("WARNING: SearchWindow is specified."));
        msdk_printf(MSDK_STRING("LenSP, RefWidth, RefHeight and SearchPath will be ignored.\n"));
        params.encodeCtrl.LenSP = params.encodeCtrl.SearchPath = params.encodeCtrl.RefWidth =
            params.encodeCtrl.RefHeight = 0;
    }
    else if (!params.encodeCtrl.SearchWindow && (!params.encodeCtrl.LenSP || !params.encodeCtrl.RefWidth || !params.encodeCtrl.RefHeight))
    {
        msdk_printf(MSDK_STRING("WARNING: SearchWindow is not specified."));
        msdk_printf(MSDK_STRING("Zero-value LenSP, RefWidth and RefHeight will be set to default.\n"));
        if (!params.encodeCtrl.LenSP)
            params.encodeCtrl.LenSP = 57;
        if (!params.encodeCtrl.RefWidth)
            params.encodeCtrl.RefWidth = 32;
        if (!params.encodeCtrl.RefHeight)
            params.encodeCtrl.RefHeight = 32;
    }

    if (!params.encodeCtrl.SearchWindow && (params.encodeCtrl.RefHeight < 20 || params.encodeCtrl.RefWidth < 20))
    {
        msdk_printf(MSDK_STRING("WARNING: Invalid RefWidth/RefHeight value. Adjust to 20 (minimum supported)\n"));
        if (params.encodeCtrl.RefWidth < 20)
            params.encodeCtrl.RefWidth = 20;
        if (params.encodeCtrl.RefHeight < 20)
            params.encodeCtrl.RefHeight = 20;
    }

    if (params.encodeCtrl.MVPredictor == 0 && (params.bPREENC || 0 != msdk_strlen(params.mvpInFile)))
    {
        msdk_printf(MSDK_STRING("WARNING: MV predictor block size is invalid or unspecified. Adjust to 1 for PreENC (7 for -mvpin)\n"));
        if (params.bPREENC)
            params.encodeCtrl.MVPredictor = 1;
        if (0 != msdk_strlen(params.mvpInFile))
            params.encodeCtrl.MVPredictor = 7;
    }

    if (params.bPREENC && (!params.preencCtrl.SearchWindow || params.preencCtrl.SearchWindow > 5))
    {
        msdk_printf(MSDK_STRING("WARNING: Invalid SearchWindow for PreENC - setting to default value 5\n"));
        params.preencCtrl.SearchWindow = 5;
    }

    if (!params.bPREENC && 0 == msdk_strlen(params.mvpInFile))
    {
        if (params.encodeCtrl.MVPredictor != 0 || params.encodeCtrl.NumMvPredictors[0] != 0
            || params.encodeCtrl.NumMvPredictors[1] != 0)
        {
            msdk_printf(MSDK_STRING("WARNING: No MV predictors specified for ENCODE - setting MVPBlockSize and NumMVPredictors to 0\n"));
            params.encodeCtrl.MVPredictor = 0;
            params.encodeCtrl.NumMvPredictors[0] = 0;
            params.encodeCtrl.NumMvPredictors[1] = 0;
        }
    }

    if (!params.bENCODE && (strlen(params.repackctrlFile)||strlen(params.repackstatFile)))
    {
        msdk_printf(MSDK_STRING("WARNING: Repackctrl/Repackstat disabled for only supported in ENCODE!\n"));
        MSDK_ZERO_MEMORY(params.repackctrlFile);
        MSDK_ZERO_MEMORY(params.repackstatFile);
    }

    if (strlen(params.repackctrlFile) && !params.bEncodedOrder)
    {
        msdk_printf(MSDK_STRING("WARNING: Encoded order is enabled by force in repackctrl.\n"));
        params.bEncodedOrder = true;
    }

    if (!params.bExtBRC && params.TargetKbps)
    {
        msdk_printf(MSDK_STRING("WARNING: Target bitrate is ignored as external BRC is disabled\n"));
        params.TargetKbps = 0;
    }

    if (!params.bExtBRC && !params.QP)
    {
        msdk_printf(MSDK_STRING("WARNING: QP is not specified. Adjust to 26 (default)\n"));
        params.QP = 26;
    }

    if (!params.bEncodedOrder && !params.encodeCtrl.FastIntraMode &&
            (params.frameCtrl.CtrlI.FastIntraMode || params.frameCtrl.CtrlP.FastIntraMode || params.frameCtrl.CtrlB.FastIntraMode))
    {
        msdk_printf(MSDK_STRING("WARNING: -FastIntra:I, -FastIntra:P and -FastIntra:B are not supported in display order\n"));
        params.frameCtrl.CtrlI.FastIntraMode = params.frameCtrl.CtrlP.FastIntraMode = params.frameCtrl.CtrlB.FastIntraMode = 0;
    }
    if (!params.bEncodedOrder && !params.encodeCtrl.ForceCtuSplit &&
            (params.frameCtrl.CtrlI.ForceCtuSplit || params.frameCtrl.CtrlP.ForceCtuSplit || params.frameCtrl.CtrlB.ForceCtuSplit))
    {
        msdk_printf(MSDK_STRING("WARNING: -ForceCtuSplit:I, -ForceCtuSplit:P and -ForceCtuSplit:B are not supported in display order\n"));
        params.frameCtrl.CtrlI.ForceCtuSplit = params.frameCtrl.CtrlP.ForceCtuSplit = params.frameCtrl.CtrlB.ForceCtuSplit = 0;
    }
    if (!params.bEncodedOrder && (params.encodeCtrl.NumFramePartitions != params.frameCtrl.CtrlI.NumFramePartitions ||
            params.encodeCtrl.NumFramePartitions != params.frameCtrl.CtrlP.NumFramePartitions ||
            params.encodeCtrl.NumFramePartitions != params.frameCtrl.CtrlB.NumFramePartitions))
    {
        msdk_printf(MSDK_STRING("WARNING: -NFP:I, -NFP:P and -NFP:B are not supported in display order\n"));
        params.frameCtrl.CtrlI.NumFramePartitions = params.frameCtrl.CtrlP.NumFramePartitions =
            params.frameCtrl.CtrlB.NumFramePartitions = params.encodeCtrl.NumFramePartitions;
    }
}

int main(int argc, char *argv[])
{
    sInputParams userParams;    // parameters from command line

    mfxStatus sts = MFX_ERR_NONE;

    try
    {
        sts = ParseInputString(argv, (mfxU8)argc, userParams);
        MSDK_CHECK_PARSE_RESULT(sts, MFX_ERR_NONE, 1);

        CEncodingPipeline pipeline(userParams);

        sts = pipeline.Init();
        MSDK_CHECK_PARSE_RESULT(sts, MFX_ERR_NONE, 1);

        pipeline.PrintInfo();

        sts = pipeline.Execute();
        MSDK_CHECK_PARSE_RESULT(sts, MFX_ERR_NONE, 1);
    }
    catch(mfxError& ex)
    {
        msdk_printf("\n%s!\n", ex.what());
        return ex.GetStatus();
    }
    catch(std::exception& ex)
    {
        msdk_printf("\nUnexpected exception!! %s\n", ex.what());
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    catch(...)
    {
        msdk_printf("\nUnexpected exception!!\n");
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    return 0;
}
