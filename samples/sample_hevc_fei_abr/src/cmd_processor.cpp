/******************************************************************************\
Copyright (c) 2018, Intel Corporation
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
#include "cmd_processor.h"

// if last option requires value, check that it passed to app
#define CHECK_NEXT_VAL(val, argName) \
{ \
    if (val) \
    { \
        msdk_printf(MSDK_STRING("ERROR: Input argument \"%s\" require more parameters\n"), argName); \
        PrintHelp(NULL); \
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

#define IS_SEPARATOR(ch)  ((ch) <= ' ' || (ch) == '=')

void PrintHelp(const msdk_char *strErrorMessage)
{
    msdk_printf(MSDK_STRING("\nIntel(R) Media SDK HEVC FEI ABR Encoding Sample Version %s\n\n"), GetMSDKSampleVersion().c_str());

    if (strErrorMessage)
    {
        msdk_printf(MSDK_STRING("ERROR: %s\n"), strErrorMessage);
    }
    msdk_printf(MSDK_STRING("Usage: sample_hevc_fei_abr [<options>] -i InputFile -o OutputEncodedFile -dso InputVideoSequenceForStreamout\n"));
    msdk_printf(MSDK_STRING("OR\n"));
    msdk_printf(MSDK_STRING("       sample_hevc_fei_abr -par ParFile\n"));
    msdk_printf(MSDK_STRING("\n"));
    msdk_printf(MSDK_STRING("Options: \n"));
    msdk_printf(MSDK_STRING("   [?] - print this message\n"));
    msdk_printf(MSDK_STRING("\n"));
    msdk_printf(MSDK_STRING("Specify input/output: \n"));
    msdk_printf(MSDK_STRING("   [-i::h265 <file-name>] - input file and decoder type\n"));
    msdk_printf(MSDK_STRING("   [-dso <file-name>]     - input stream for DSO extraction\n"));
    msdk_printf(MSDK_STRING("   [-o <file-name>]       - output h265 encoded file\n"));
    msdk_printf(MSDK_STRING("Specify pipeline in parfile (shouldn't be mixed with command line options): \n"));
    msdk_printf(MSDK_STRING("   [-par <parfile>] - specify 1:N transcoding pipelines in parfile\n"));
    msdk_printf(MSDK_STRING("   [-o::sink]       - pipeline makes decode only and put data to shared buffer (in parfile only)\n"));
    msdk_printf(MSDK_STRING("   [-i::source]     - pipeline makes encode and get data from shared buffer (in parfile only)\n"));
    msdk_printf(MSDK_STRING("Video input processing: \n"));
    msdk_printf(MSDK_STRING("   [-n number] - number of frames to process\n"));

    msdk_printf(MSDK_STRING("\n"));
    msdk_printf(MSDK_STRING("BRC: \n"));
    msdk_printf(MSDK_STRING("   [-vbr::sw]          - use MSDK SW VBR rate control\n"));
    msdk_printf(MSDK_STRING("   [-vbr::la]          - use lookahead VBR rate control\n"));
    msdk_printf(MSDK_STRING("   [-TargetKbps value] - target bitrate\n"));
    msdk_printf(MSDK_STRING("LA BRC options:\n"));
    msdk_printf(MSDK_STRING("   [-LookAheadDepth   value]     - depth of look ahead (defult is 100)\n"));
    msdk_printf(MSDK_STRING("   [-LookBackDepth    value]     - depth of look back window for taking into account encoded frames statistics (default is 100)\n"));
    msdk_printf(MSDK_STRING("   [-AdaptationLength value]     - number of frames to calculate adjustment ratio to minimize bitrate estimation error (default is 100)\n"));
    msdk_printf(MSDK_STRING("   [-Algorithm::MSE <file-name>] - use yuv file for MSE calculation (not optimized implementation)\n"));
    msdk_printf(MSDK_STRING("   [-Algorithm::NNZ]             - use number of non-zero transform coefficients for visual distortion approximation (default)\n"));
    msdk_printf(MSDK_STRING("   [-Algorithm::SSC]             - use sum of squared transform coefficients for visual distortion approximation\n"));

    msdk_printf(MSDK_STRING("\n"));
    msdk_printf(MSDK_STRING("Encoding settings: \n"));
    msdk_printf(MSDK_STRING("   [-qp qp_value]     - QP value for frames (default is 26)\n"));
    msdk_printf(MSDK_STRING("   [-DisableQPOffset] - disable QP offset per pyramid layer\n"));
    msdk_printf(MSDK_STRING("   [-l numSlices]     - number of slices \n"));
    msdk_printf(MSDK_STRING("   [-PicTimingSEI]    - inserts picture timing SEI\n"));

    msdk_printf(MSDK_STRING("GOP structure: \n"));
    msdk_printf(MSDK_STRING("   [-r (-GopRefDist) distance]       - Distance between I- or P- key frames (1 means no B-frames) (0 - by default(I frames))\n"));
    msdk_printf(MSDK_STRING("   [-num_ref (-NumRefFrame) numRefs] - number of reference frames\n"));
    msdk_printf(MSDK_STRING("   [-g size]                         - GOP size (1(default) means I-frames only)\n"));
    msdk_printf(MSDK_STRING("   [-gop_opt closed|strict]          - GOP optimization flags (can be used together)\n"));
    msdk_printf(MSDK_STRING("   [-idr_interval size]              - if IdrInterval = 0, then only first I-frame is an IDR-frame\n"));
    msdk_printf(MSDK_STRING("                                       if IdrInterval = 1, then every I - frame is an IDR - frame\n"));
    msdk_printf(MSDK_STRING("                                       if IdrInterval = 2, then every other I - frame is an IDR - frame, etc (default is 0)\n"));

    msdk_printf(MSDK_STRING("References structure: \n"));
    msdk_printf(MSDK_STRING("   [-NumRefActiveP   numRefs]  - number of maximum allowed references for P frames (up to 4)\n"));
    msdk_printf(MSDK_STRING("   [-NumRefActiveBL0 numRefs]  - number of maximum allowed backward references for B frames (up to 3)\n"));
    msdk_printf(MSDK_STRING("   [-NumRefActiveBL1 numRefs]  - number of maximum allowed forward references for B frames (up to 2)\n"));
    msdk_printf(MSDK_STRING("   [-gpb:<on,off>]             - make HEVC encoder use regular P-frames (off) or GPB (on) (on - by default)\n"));
    msdk_printf(MSDK_STRING("   [-ppyr:<on,off>]            - enables P-pyramid\n"));
    msdk_printf(MSDK_STRING("   [-bref]                     - arrange B frames in B-pyramid reference structure\n"));
    msdk_printf(MSDK_STRING("   [-nobref]                   - do not use B-pyramid (by default the decision is made by library)\n"));

    msdk_printf(MSDK_STRING("\n"));
    msdk_printf(MSDK_STRING("FEI specific settings: \n"));
    msdk_printf(MSDK_STRING("Predictors: \n"));
    msdk_printf(MSDK_STRING("   [-NumPredictorsL0 numPreds] - number of maximum L0 predictors (default - assign depending on the frame type)\n"));
    msdk_printf(MSDK_STRING("   [-NumPredictorsL1 numPreds] - number of maximum L1 predictors (default - assign depending on the frame type)\n"));
    msdk_printf(MSDK_STRING("   [-MultiPredL0 type]         - use internal L0 MV predictors (0 - no internal MV predictor, 1 - spatial internal MV predictors)\n"));
    msdk_printf(MSDK_STRING("   [-MultiPredL1 type]         - use internal L1 MV predictors (0 - no internal MV predictor, 1 - spatial internal MV predictors)\n"));
    msdk_printf(MSDK_STRING("   [-MVPBlockSize size]        - parse input MV predictor buffer (from DSO) as having: \n"));
    msdk_printf(MSDK_STRING("                                 0 - no MVP, 1 - MVP per 16x16 block, 2 - MVP per 32x32 block, 7 - MVP block size specified in the MVP structs (default) \n"));
    msdk_printf(MSDK_STRING("   [-DSOMVPBlockSize size]     - force DSO to generate MVP buffer with MVPs per block size:  \n"));
    msdk_printf(MSDK_STRING("                                 0 - no DSO MVP output, 1 - MVP per 16x16 block, 2 - MVP per 32x32 block, 7 - determined by CTU partitioning (default) \n"));
    msdk_printf(MSDK_STRING("   [-DrawMVP] - creates output YUV file with MVP overlay\n"));
    msdk_printf(MSDK_STRING("   [-DumpMVP] - dumps final per-frame MVP structures with DSO data as binary files with filenames\n"));
    msdk_printf(MSDK_STRING("                'MVPdump_encorder_frame_%%(frame_number_in_encoded_order).bin' (frame numbering starts with 1)\n"));

    msdk_printf(MSDK_STRING("Partitioning: \n"));
    msdk_printf(MSDK_STRING("   [-ForceCtuSplit]          - force splitting CTU into CU at least once\n"));
    msdk_printf(MSDK_STRING("   [-ForceCtuSplit:I] - force splitting CTU into CU at least once on I-frames only\n"));
    msdk_printf(MSDK_STRING("   [-ForceCtuSplit:P] - force splitting CTU into CU at least once on P/GPB-frames only\n"));
    msdk_printf(MSDK_STRING("   [-ForceCtuSplit:B] - force splitting CTU into CU at least once on B-frames only\n"));
    msdk_printf(MSDK_STRING("   [-NumFramePartitions num] - number of partitions in frame that encoder processes concurrently (1, 2, 4 or 8)\n"));
    msdk_printf(MSDK_STRING("   [-NFP:I num] - number of partitions in I frame that encoder processes concurrently (1, 2, 4 or 8)\n"));
    msdk_printf(MSDK_STRING("   [-NFP:P num] - number of partitions in P/GPB frame that encoder processes concurrently (1, 2, 4 or 8)\n"));
    msdk_printf(MSDK_STRING("   [-NFP:B num] - number of partitions in B frame that encoder processes concurrently (1, 2, 4 or 8)\n"));

    msdk_printf(MSDK_STRING("Motion Search: \n"));
    msdk_printf(MSDK_STRING("   [-SearchWindow value] - specifies one of the predefined search path and window size. In range [1,5] (5 is default).\n"));
    msdk_printf(MSDK_STRING("                           If zero value specified: -RefWidth / RefHeight, -LenSP are required\n"));
    msdk_printf(MSDK_STRING("   [-RefWidth width]     - width of search region (should be multiple of 4),\n"));
    msdk_printf(MSDK_STRING("                           valid range is [20, 64] for one direction and [20, 32] for bidirectional search\n"));
    msdk_printf(MSDK_STRING("   [-RefHeight height]   - height of search region (should be multiple of 4),\n"));
    msdk_printf(MSDK_STRING("                           valid range is [20, 64] for one direction and [20, 32] for bidirectional search\n"));
    msdk_printf(MSDK_STRING("   [-LenSP length]       - defines number of search units in search path. In range [1,63] (default is 57)\n"));
    msdk_printf(MSDK_STRING("   [-SearchPath value]   - defines shape of search path. 1 - diamond, 2 - full, 0 - default (full).\n"));
    msdk_printf(MSDK_STRING("   [-AdaptiveSearch]     - enables adaptive search\n"));

    msdk_printf(MSDK_STRING("Force Flags: \n"));
    msdk_printf(MSDK_STRING("   [-FastIntra:I]  - force encoder to skip HEVC-specific intra modes (use AVC modes only) on I-frames\n"));
    msdk_printf(MSDK_STRING("   [-FastIntra:P]  - force encoder to skip HEVC-specific intra modes (use AVC modes only) on P/GPB-frames\n"));
    msdk_printf(MSDK_STRING("   [-FastIntra:B]  - force encoder to skip HEVC-specific intra modes (use AVC modes only) on B-frames\n"));
    msdk_printf(MSDK_STRING("   [-ForceToIntra] - force CUs to be coded as intra using DSO information\n"));
    msdk_printf(MSDK_STRING("   [-ForceToInter] - force CUs to be coded as inter using DSO information\n"));

    msdk_printf(MSDK_STRING("\n"));
}

mfxStatus CheckOptions(const sInputParams & params)
{
    if (params.pipeMode == Full || params.pipeMode == Producer)
    {
        if (0 == msdk_strlen(params.input.strSrcFile))
        {
            PrintHelp("Source file name not found");
            return MFX_ERR_UNSUPPORTED;
        }
        if (!params.input.bDSO)
        {
            PrintHelp("DSO stream is mandatory");
            return MFX_ERR_UNSUPPORTED;
        }
    }

    if (params.pipeMode == Full || params.pipeMode == Consumer)
    {
        if (params.QP > 51)
        {
            PrintHelp("Invalid QP value (must be in range [0, 51])");
            return MFX_ERR_UNSUPPORTED;
        }
        if (params.NumRefActiveP > 4)
        {
            PrintHelp("Unsupported NumRefActiveP value (must be in range [0,4])");
            return MFX_ERR_UNSUPPORTED;
        }
        if (params.NumRefActiveBL0 > 4)
        {
            PrintHelp("Unsupported NumRefActiveBL0 value (must be in range [0,4])");
            return MFX_ERR_UNSUPPORTED;
        }
        if (params.NumRefActiveBL1 > 2)
        {
            PrintHelp("Unsupported NumRefActiveBL1 value (must be in range [0,2])");
            return MFX_ERR_UNSUPPORTED;
        }
        if (params.encodeCtrl.NumMvPredictors[0] > 4)
        {
            PrintHelp("Unsupported NumMvPredictorsL0 value (must be in range [0,4])");
            return MFX_ERR_UNSUPPORTED;
        }
        if (params.encodeCtrl.NumMvPredictors[1] > 4)
        {
            PrintHelp("Unsupported NumMvPredictorsL1 value (must be in range [0,4])");
            return MFX_ERR_UNSUPPORTED;
        }
        std::list<mfxU16> nfp_values = {1, 2, 4, 8};
        if (std::find(nfp_values.begin(), nfp_values.end(), params.frameCtrl.CtrlI.NumFramePartitions) == nfp_values.end())
        {
            PrintHelp("Unsupported NumFramePartitions value for I frames (must be 1, 2, 4 or 8)");
            return MFX_ERR_UNSUPPORTED;
        }
        if (std::find(nfp_values.begin(), nfp_values.end(), params.frameCtrl.CtrlP.NumFramePartitions) == nfp_values.end())
        {
            PrintHelp("Unsupported NumFramePartitions value for P/GPB frames (must be 1, 2, 4 or 8)");
            return MFX_ERR_UNSUPPORTED;
        }
        if (std::find(nfp_values.begin(), nfp_values.end(), params.frameCtrl.CtrlB.NumFramePartitions) == nfp_values.end())
        {
            PrintHelp("Unsupported NumFramePartitions value for B frames (must be 1, 2, 4 or 8)");
            return MFX_ERR_UNSUPPORTED;
        }
        if (0 == params.nGopSize)
        {
            PrintHelp("GopSize = 0)");
            return MFX_ERR_UNSUPPORTED;
        }
        if (params.nGopSize > 1 && 0 == params.nRefDist)
        {
            PrintHelp("Gop structure is invalid. Set GopRefDist option");
            return MFX_ERR_UNSUPPORTED;
        }
        if (params.encodeCtrl.MVPredictor != 0 && params.encodeCtrl.MVPredictor != 1 &&
            params.encodeCtrl.MVPredictor != 2 && params.encodeCtrl.MVPredictor != 7 &&
            // not specified in cmd line, will be adjusted later
            params.encodeCtrl.MVPredictor != 0xffff)
        {
            PrintHelp("Invalid MV predictor block size value.");
            return MFX_ERR_UNSUPPORTED;
        }

        if (params.encodeCtrl.SearchWindow > 8)
        {
            PrintHelp("Invalid SearchWindow value");
            return MFX_ERR_UNSUPPORTED;
        }
        if (params.encodeCtrl.SearchPath > 2)
        {
            PrintHelp("Invalid SearchPath value");
            return MFX_ERR_UNSUPPORTED;
        }
        if (params.encodeCtrl.LenSP > 63)
        {
            PrintHelp("Invalid LenSP value");
            return MFX_ERR_UNSUPPORTED;
        }

        if (params.encodeCtrl.RefWidth % 4 != 0 || params.encodeCtrl.RefHeight % 4 != 0
            // check only the highest possible limit regardless of gop structure
            || params.encodeCtrl.RefHeight > 64 || params.encodeCtrl.RefWidth > 64
            || params.encodeCtrl.RefWidth * params.encodeCtrl.RefHeight > 2048)
        {
            PrintHelp("Invalid RefWidth/RefHeight value");
            return MFX_ERR_UNSUPPORTED;
        }

        if (params.sBRCparams.eBrcType != NONE && (params.sBRCparams.TargetKbps == 0 || (params.sBRCparams.eBrcType == LOOKAHEAD && params.sBRCparams.LookAheadDepth == 0)))
        {
            if (params.sBRCparams.TargetKbps == 0)
                PrintHelp("Invalid BRC settings: TargetKbps should be set");
            else if (params.sBRCparams.eBrcType == LOOKAHEAD && params.sBRCparams.LookAheadDepth == 0)
                PrintHelp("Invalid BRC settings: LookAheadDepth should be set");

            return MFX_ERR_UNSUPPORTED;
        }

        if (!(params.input.DSOMVPBlockSize == 0 || params.input.DSOMVPBlockSize == 1 ||
              params.input.DSOMVPBlockSize == 2 || params.input.DSOMVPBlockSize == 7 ))
        {
            PrintHelp("Invalid DSOMVPBlockSize");
            return MFX_ERR_UNSUPPORTED;
        }
    }

    return MFX_ERR_NONE;
}

void AdjustOptions(sInputParams& params)
{
    if (params.pipeMode == Full || params.pipeMode == Consumer)
    {
        params.nNumSlices       = tune(params.nNumSlices, 0, 1);
        params.nIdrInterval     = tune(params.nIdrInterval, 0, 0xffff);
        mfxU16 nMinRefFrame     = std::max(params.NumRefActiveP, (mfxU16)(params.NumRefActiveBL0 + params.NumRefActiveBL1));
        if (nMinRefFrame > params.nNumRef)
            params.nNumRef      = nMinRefFrame;

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
        else if (!params.encodeCtrl.SearchWindow && (params.encodeCtrl.RefHeight < 20 || params.encodeCtrl.RefWidth < 20))
        {
            msdk_printf(MSDK_STRING("WARNING: Invalid RefWidth/RefHeight value. Adjust to 20 (minimum supported)\n"));
            if (params.encodeCtrl.RefWidth < 20)
                params.encodeCtrl.RefWidth = 20;
            if (params.encodeCtrl.RefHeight < 20)
                params.encodeCtrl.RefHeight = 20;
        }

        if (params.encodeCtrl.MVPredictor == 0xffff)
        {
            params.encodeCtrl.MVPredictor = 0;
        }
    }
}

mfxStatus CmdProcessor::ParseParamsForOneSession(mfxU32 argc, msdk_char* argv[])
{
    if (1 == argc)
    {
        PrintHelp(MSDK_STRING("Not enough input parameters"));
        return MFX_ERR_UNSUPPORTED;
    }

    sInputParams params;

    bool isParseInvalid = false; // show help once after all parsing errors will be processed

    // parse command line parameters
    for (mfxU32 i = 0; i < argc; i++)
    {
        MSDK_CHECK_POINTER(argv[i], MFX_ERR_UNDEFINED_BEHAVIOR);

        // process multi-character options
        if (0 == msdk_strcmp(argv[i], MSDK_STRING("-qp")))
        {
            CHECK_NEXT_VAL(i + 1 >= argc, argv[i]);
            PARSE_CHECK(msdk_opt_read(argv[++i], params.QP), "QP", isParseInvalid);
        }
        else if ( (0 == msdk_strncmp(argv[i], MSDK_STRING("-i::"), msdk_strlen(MSDK_STRING("-i::")))) &&
            (0 != msdk_strncmp(argv[i]+4, MSDK_STRING("source"), msdk_strlen(MSDK_STRING("source")))) )
        {
            mfxStatus sts = StrFormatToCodecFormatFourCC(argv[i] + 4, params.input.DecodeId);
            if (sts != MFX_ERR_NONE)
            {
                PrintHelp(MSDK_STRING("ERROR: Failed to extract decoder type"));
                return MFX_ERR_UNSUPPORTED;
            }
            i++;

            if (msdk_strlen(argv[i]) < MSDK_ARRAY_LEN(params.input.strSrcFile)){
                msdk_opt_read(argv[i], params.input.strSrcFile);
            }
            else{
                PrintHelp(MSDK_STRING("ERROR: Too long input filename (limit is 1023 characters)!"));
                return MFX_ERR_UNSUPPORTED;
            }

            switch (params.input.DecodeId)
            {
            case MFX_CODEC_HEVC:
                break;
            case MFX_CODEC_AVC:
                break;
            default:
                PrintHelp(MSDK_STRING("ERROR: Unsupported encoded input (only HEVC is supported)"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-dso")))
        {
            CHECK_NEXT_VAL(i + 1 >= argc, argv[i]);
            PARSE_CHECK(msdk_opt_read(argv[++i], params.input.strDsoFile), "Input DSO stream", isParseInvalid);
            params.input.bDSO = true;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-i::source")))
        {
            if (params.pipeMode != Full)
            {
                PrintHelp("-i::source cannot be used here");
                return MFX_ERR_UNSUPPORTED;
            }

            params.pipeMode = Consumer;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-o::sink")))
        {
            if (params.pipeMode != Full)
            {
                PrintHelp("-o::sink cannot be used here");
                return MFX_ERR_UNSUPPORTED;
            }

            params.pipeMode = Producer;

        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-o")))
        {
            CHECK_NEXT_VAL(i + 1 >= argc, argv[i]);
            PARSE_CHECK(msdk_opt_read(argv[++i], params.strDstFile), "Output file", isParseInvalid);
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-vbr::sw")))
        {
            params.sBRCparams.eBrcType = MSDKSW;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-vbr::la")))
        {
            params.sBRCparams.eBrcType = LOOKAHEAD;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-TargetKbps")))
        {
            if (!argv[++i] || MFX_ERR_NONE != msdk_opt_read(argv[i], params.sBRCparams.TargetKbps))
            {
                PrintHelp(MSDK_STRING("ERROR: TargetKbps is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-LookAheadDepth")))
        {
            if (!argv[++i] || MFX_ERR_NONE != msdk_opt_read(argv[i], params.sBRCparams.LookAheadDepth))
            {
                PrintHelp(MSDK_STRING("ERROR: LookAheadDepth is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-LookBackDepth")))
        {
            if (!argv[++i] || MFX_ERR_NONE != msdk_opt_read(argv[i], params.sBRCparams.LookBackDepth))
            {
                PrintHelp(MSDK_STRING("ERROR: LookBackDepth is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-AdaptationLength")))
        {
            if (!argv[++i] || MFX_ERR_NONE != msdk_opt_read(argv[i], params.sBRCparams.AdaptationLength))
            {
                PrintHelp(MSDK_STRING("ERROR: AdaptationLength is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-Algorithm::NNZ")))
        {
            params.sBRCparams.eAlgType = NNZ;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-Algorithm::SSC")))
        {
            params.sBRCparams.eAlgType = SSC;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-Algorithm::MSE")))
        {
            params.sBRCparams.eAlgType = MSE;

            CHECK_NEXT_VAL(i + 1 >= argc, argv[i]);
            PARSE_CHECK(msdk_opt_read(argv[++i], params.sBRCparams.strYUVFile), "Input YUV file", isParseInvalid);
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-n")))
        {
            CHECK_NEXT_VAL(i + 1 >= argc, argv[i]);
            PARSE_CHECK(msdk_opt_read(argv[++i], params.nNumFrames), "NumFrames", isParseInvalid);
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-g")))
        {
            CHECK_NEXT_VAL(i + 1 >= argc, argv[i]);
            PARSE_CHECK(msdk_opt_read(argv[++i], params.nGopSize), "GopSize", isParseInvalid);
        }
        else if (   0 == msdk_strcmp(argv[i], MSDK_STRING("-r"))
                 || 0 == msdk_strcmp(argv[i], MSDK_STRING("-GopRefDist")))
        {
            CHECK_NEXT_VAL(i + 1 >= argc, argv[i]);
            PARSE_CHECK(msdk_opt_read(argv[++i], params.nRefDist), "GopRefDist", isParseInvalid);
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-l")))
        {
            CHECK_NEXT_VAL(i + 1 >= argc, argv[i]);
            PARSE_CHECK(msdk_opt_read(argv[++i], params.nNumSlices), "NumSlices", isParseInvalid);
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-ppyr:on")))
        {
            params.PRefType = MFX_P_REF_PYRAMID;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-ppyr:off")))
        {
            params.PRefType = MFX_P_REF_SIMPLE;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-bref")))
        {
            params.BRefType = MFX_B_REF_PYRAMID;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-nobref")))
        {
            params.BRefType = MFX_B_REF_OFF;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-idr_interval")))
        {
            CHECK_NEXT_VAL(i + 1 >= argc, argv[i]);
            PARSE_CHECK(msdk_opt_read(argv[++i], params.nIdrInterval), "IdrInterval", isParseInvalid);
        }
        else if (   0 == msdk_strcmp(argv[i], MSDK_STRING("-num_ref"))
                 || 0 == msdk_strcmp(argv[i], MSDK_STRING("-NumRefFrame")))
        {
            CHECK_NEXT_VAL(i + 1 >= argc, argv[i]);
            PARSE_CHECK(msdk_opt_read(argv[++i], params.nNumRef), "NumRefFrame", isParseInvalid);
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-NumRefActiveP")))
        {
            CHECK_NEXT_VAL(i + 1 >= argc, argv[i]);
            PARSE_CHECK(msdk_opt_read(argv[++i], params.NumRefActiveP), "NumRefActiveP", isParseInvalid);
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-NumRefActiveBL0")))
        {
            CHECK_NEXT_VAL(i + 1 >= argc, argv[i]);
            PARSE_CHECK(msdk_opt_read(argv[++i], params.NumRefActiveBL0), "NumRefActiveBL0", isParseInvalid);
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-NumRefActiveBL1")))
        {
            CHECK_NEXT_VAL(i + 1 >= argc, argv[i]);
            PARSE_CHECK(msdk_opt_read(argv[++i], params.NumRefActiveBL1), "NumRefActiveBL1", isParseInvalid);
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-NumPredictorsL0")))
        {
            CHECK_NEXT_VAL(i + 1 >= argc, argv[i]);
            PARSE_CHECK(msdk_opt_read(argv[++i], params.encodeCtrl.NumMvPredictors[0]), "NumPredictorsL0", isParseInvalid);
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-NumPredictorsL1")))
        {
            CHECK_NEXT_VAL(i + 1 >= argc, argv[i]);
            PARSE_CHECK(msdk_opt_read(argv[++i], params.encodeCtrl.NumMvPredictors[1]), "NumPredictorsL1", isParseInvalid);
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-MultiPredL0")))
        {
            CHECK_NEXT_VAL(i + 1 >= argc, argv[i]);
            PARSE_CHECK(msdk_opt_read(argv[++i], params.encodeCtrl.MultiPred[0]), "MultiPredL0", isParseInvalid);
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-MultiPredL1")))
        {
            CHECK_NEXT_VAL(i + 1 >= argc, argv[i]);
            PARSE_CHECK(msdk_opt_read(argv[++i], params.encodeCtrl.MultiPred[1]), "MultiPredL1", isParseInvalid);
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-DSOMVPBlockSize")))
        {
            CHECK_NEXT_VAL(i + 1 >= argc, argv[i]);
            PARSE_CHECK(msdk_opt_read(argv[++i], params.input.DSOMVPBlockSize), "DSOMVPBlockSize", isParseInvalid);
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-MVPBlockSize")))
        {
            CHECK_NEXT_VAL(i + 1 >= argc, argv[i]);
            PARSE_CHECK(msdk_opt_read(argv[++i], params.encodeCtrl.MVPredictor), "MVPBlockSize", isParseInvalid);
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-ForceCtuSplit")))
        {
            params.frameCtrl.CtrlI.ForceCtuSplit = params.frameCtrl.CtrlP.ForceCtuSplit = params.frameCtrl.CtrlB.ForceCtuSplit = 1;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-ForceCtuSplit:I")))
        {
            params.frameCtrl.CtrlI.ForceCtuSplit = 1;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-ForceCtuSplit:P")))
        {
            params.frameCtrl.CtrlP.ForceCtuSplit = 1;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-ForceCtuSplit:B")))
        {
            params.frameCtrl.CtrlB.ForceCtuSplit = 1;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-NumFramePartitions")))
        {
            CHECK_NEXT_VAL(i + 1 >= argc, argv[i]);
            PARSE_CHECK(msdk_opt_read(argv[++i], params.encodeCtrl.NumFramePartitions), "NumFramePartitions", isParseInvalid);
            params.frameCtrl.CtrlI.NumFramePartitions =
                params.frameCtrl.CtrlP.NumFramePartitions =
                params.frameCtrl.CtrlB.NumFramePartitions = params.encodeCtrl.NumFramePartitions;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-NFP:I")))
        {
            CHECK_NEXT_VAL(i + 1 >= argc, argv[i]);
            PARSE_CHECK(msdk_opt_read(argv[++i], params.frameCtrl.CtrlI.NumFramePartitions), "NumFramePartitions", isParseInvalid);
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-NFP:P")))
        {
            CHECK_NEXT_VAL(i + 1 >= argc, argv[i]);
            PARSE_CHECK(msdk_opt_read(argv[++i], params.frameCtrl.CtrlP.NumFramePartitions), "NumFramePartitions", isParseInvalid);
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-NFP:B")))
        {
            CHECK_NEXT_VAL(i + 1 >= argc, argv[i]);
            PARSE_CHECK(msdk_opt_read(argv[++i], params.frameCtrl.CtrlB.NumFramePartitions), "NumFramePartitions", isParseInvalid);
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-FastIntra:I")))
        {
            params.frameCtrl.CtrlI.FastIntraMode = 1;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-FastIntra:P")))
        {
            params.frameCtrl.CtrlP.FastIntraMode = 1;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-FastIntra:B")))
        {
            params.frameCtrl.CtrlB.FastIntraMode = 1;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-gop_opt")))
        {
            CHECK_NEXT_VAL(i + 1 >= argc, argv[i]);
            ++i;
            if (0 == msdk_strcmp(argv[i], MSDK_STRING("closed")))
            {
                params.nGopOptFlag |= MFX_GOP_CLOSED;
            }
            else if (0 == msdk_strcmp(argv[i], MSDK_STRING("strict")))
            {
                params.nGopOptFlag |= MFX_GOP_STRICT;
            }
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-gpb:on")))
        {
            params.GPB = MFX_CODINGOPTION_ON;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-gpb:off")))
        {
            params.GPB = MFX_CODINGOPTION_OFF;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-PicTimingSEI")))
        {
            params.PicTimingSEI = MFX_CODINGOPTION_ON;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-SearchWindow")))
        {
            CHECK_NEXT_VAL(i + 1 >= argc, argv[i]);
            PARSE_CHECK(msdk_opt_read(argv[++i], params.encodeCtrl.SearchWindow), "SearchWindow", isParseInvalid);
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-LenSP")))
        {
            CHECK_NEXT_VAL(i + 1 >= argc, argv[i]);
            PARSE_CHECK(msdk_opt_read(argv[++i], params.encodeCtrl.LenSP), "LenSP", isParseInvalid);
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-RefWidth")))
        {
            CHECK_NEXT_VAL(i + 1 >= argc, argv[i]);
            PARSE_CHECK(msdk_opt_read(argv[++i], params.encodeCtrl.RefWidth), "RefWidth", isParseInvalid);
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-RefHeight")))
        {
            CHECK_NEXT_VAL(i + 1 >= argc, argv[i]);
            PARSE_CHECK(msdk_opt_read(argv[++i], params.encodeCtrl.RefHeight), "RefHeight", isParseInvalid);
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-SearchPath")))
        {
            CHECK_NEXT_VAL(i + 1 >= argc, argv[i]);
            PARSE_CHECK(msdk_opt_read(argv[++i], params.encodeCtrl.SearchPath), "SearchPath", isParseInvalid);
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-AdaptiveSearch")))
        {
            params.encodeCtrl.AdaptiveSearch = 1;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-DisableQPOffset")))
        {
            params.bDisableQPOffset = true;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-ForceToIntra")))
        {
            params.input.forceToIntra = 1;
            params.encodeCtrl.PerCtuInput = 1;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-ForceToInter")))
        {
            params.input.forceToInter = 1;
            params.encodeCtrl.PerCtuInput = 1;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("?")))
        {
            PrintHelp(NULL);
            return MFX_ERR_UNSUPPORTED;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-DrawMVP")))
        {
            params.drawMVP = true;
        }
        else if (0 == msdk_strcmp(argv[i], MSDK_STRING("-DumpMVP")))
        {
            params.dumpMVP = true;
        }
        else
        {
            msdk_printf(MSDK_STRING("\nERROR: Unknown option `%s`\n\n"), argv[i]);
            isParseInvalid = true;
        }
    }

    if (isParseInvalid)
    {
        PrintHelp(NULL);
        return MFX_ERR_UNSUPPORTED;
    }

    if (MFX_ERR_NONE != CheckOptions(params))
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    AdjustOptions(params);

    m_SessionArray.push_back(params);

    return MFX_ERR_NONE;
}

mfxStatus CmdProcessor::ParseCmdLine(mfxU32 argc, msdk_char *argv[])
{
    FILE *parFile = NULL;
    mfxStatus sts = MFX_ERR_UNSUPPORTED;

    if (1 == argc)
    {
        PrintHelp(MSDK_STRING("Too few parameters\n"));
        return MFX_ERR_MORE_DATA;
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
        else if (0 == msdk_strcmp(argv[0], MSDK_STRING("-?")) )
        {
            PrintHelp(NULL);
            return MFX_ERR_ABORTED;
        }
        else if (0 == msdk_strcmp(argv[0], MSDK_STRING("--")))
        {
            // just skip separator "--" which delimits cmd options and pipeline settings
            break;
        }
        else
        {
            break;
        }
        --argc;
        ++argv;
    }

    msdk_printf(MSDK_STRING("Intel(R) Media SDK HEVC FEI ABR Encoding Sample Version %s\n\n"), GetMSDKSampleVersion().c_str());

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
}

mfxStatus CmdProcessor::ParseParFile(FILE *parFile)
{
    mfxStatus sts = MFX_ERR_UNSUPPORTED;
    if (!parFile)
        return MFX_ERR_UNSUPPORTED;

    mfxU32 currPos = 0;

    // calculate file size
    fseek(parFile, 0, SEEK_END);
    mfxU32 fileSize = ftell(parFile) + 1;
    fseek(parFile, 0, SEEK_SET);

    // allocate buffer for parsing
    std::vector<msdk_char> parBuf;
    parBuf.resize(fileSize);
    msdk_char *pCur;

    while(currPos < fileSize)
    {
        pCur = msdk_fgets(parBuf.data(), fileSize, parFile);
        if (!pCur)
            return MFX_ERR_NONE;
        while(pCur[currPos] != '\n' && pCur[currPos] != 0)
        {
            currPos++;
            if  (pCur + currPos >= parBuf.data() + fileSize)
                return sts;
        }
        // zero string
        if (!currPos)
            continue;

        sts = TokenizeLine(pCur, currPos);
        MSDK_CHECK_STATUS(sts, "TokenizeLine failed");

        currPos = 0;
    }

    return MFX_ERR_NONE;
}

mfxStatus CmdProcessor::TokenizeLine(msdk_char *pLine, mfxU32 length)
{
    mfxU32 i;
    const mfxU8 maxArgNum = 255;
    msdk_char *argv[maxArgNum+1];
    mfxU32 argc = 0;
    std::vector<msdk_char> pMemLine;

    pMemLine.resize(length+2);

    msdk_char *pTempLine = pMemLine.data();
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
                msdk_printf(MSDK_STRING("Too many parameters (reached maximum of %d)"), maxArgNum);
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

bool CmdProcessor::GetNextSessionParams(sInputParams & InputParams)
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
}
