// Copyright (c) 2018-2019 Intel Corporation
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

#include "mfxvideo.h"

#if MFX_VERSION >= MFX_VERSION_NEXT


#include "inputparameters.h"

using namespace std;

void PrintHelp()
{
    printf("Usage:");
    printf(" -i InputYUVFile -o OutputYUVFile -w width -h height -n number_of_frames_to_process\n");
    printf(" The above params are required. If -pic_file or repack parameters are enabled, next parameters:-i InputYUVFile -o OutputYUVFile -w width -h height\n");
    printf(" Can be removed\n");
    //i/o/log
    printf("    [-g num]                - GOP size\n");
    printf("    [-r num]                - number of B frames plus 1\n");
    printf("    [-x num]                - DPB size in frames\n");
    printf("    [-idr_interval num]     - IDR interval in frames\n");
    printf("    [-num_active_P num]     - number of active references for P frames\n");
    printf("    [-num_active_BL0 num]   - number of active List 0 references for B frames\n");
    printf("    [-num_active_BL1 num]   - number of active List 1 references for B frames\n");
    printf("    [-generate]             - run ASG in test stream generation mode\n"); // required
    printf("    [-verify]               - run ASG in test results verification mode\n"); // required
    printf("    [-gen_split]            - enable CTU splitting\n");
    printf("    [-no_cu_to_pu_split]    - disable CU splitting into PUs\n");
    printf("    [-force_extbuf_mvp_block_size num] - force mfxFeiHevcEncMVPredictors::BlockSize field\n");
    printf("                              in MVP output buffer to a specified value. *ALL* output \n");
    printf("                              mfxFeiHevcEncMVPredictors structs will be affected.\n");
    printf("                              Supported values are 0, 1, 2 and 3\n");
    printf("                              See MVPredictor description in HEVC FEI manual for details\n");
    printf("    [-mvp_block_size num]   - actual MVP block size used in actual generation algorithm.\n");
    printf("                              If -force_extbuf_mvp_block_size is not specified,\n");
    printf("                              this value is used in output mfxFeiHevcEncMVPredictors::BlockSize\n");
    printf("                              only for the structures for which MVPs were actually generated\n");
    printf("                              When -force_extbuf_mvp_block_size is specified and -mvp_block_size is not\n");
    printf("                              default algorithm for MVP generation is used : MVP block size equals to CTU size.\n");
    printf("                              When both -force_extbuf_mvp_block_size and -mvp_block_size are specified : the 1st one\n");
    printf("                              value is used in the output ExtBuffer regardless to actual MVP block size\n");
    printf("                              Supported values are 0, 1, 2 and 3\n");
    printf("    [-force_symm_cu_part]   - forces using only symmetric CU into PU partioning modes\n");
    printf("                              for inter prediction test\n");
    printf("    [-gen_inter]            - generate inter CUs (inter prediction test)\n");
    printf("    [-gen_intra]            - generate intra CUs (intra prediction test)\n");
    printf("    [-gen_pred]             - generate MV predictors (inter prediction test)\n");
    printf("    [-gen_mv]               - generate motion vectors inside search window (inter prediction test)\n"); // name != sense
    printf("                              If -gen_mv is not specified,\n");
    printf("                              then resulting MVs for PUs will be generated outside\n");
    printf("                              the search window only\n");
    printf("    [-gen_repack_ctrl]      - generate/verify repack control data\n");
    printf("    [-pred_file File]       - output file for MV predictors\n");
    printf("    [-pic_file File]        - output file for pictures' structure\n");
    printf("    [-log2_ctu_size num]    - log2 CTU size to be used for CU quad-tree structure\n");
    printf("                              Default is 4");
    printf("                              Cannot be less than min_log2_tu_size (described below)\n");
    printf("    [-min_log2_tu_size num] - minimum log2 TU size to be used for quad-tree structure\n");
    printf("                              Must be less than min_log2_cu_size (described below), default is 2\n");
    printf("    [-max_log2_tu_size num] - maximum log2 TU size to be used for quad-tree structure\n");
    printf("                              Must be less than or equal to Min(log2_ctu_size, 5), default is 4\n");
    printf("    [-max_tu_qt_depth num]  - maximum TU quad-tree depth inside CU\n");
    printf("                              Overrrides min_log2_tu_size, default is 4\n");
    printf("    [-min_log2_cu_size num] - minimum log2 CU size to be used for quad-tree structure.\n");
    printf("                              Cannot be less than max_log2_tu_size, default is 3\n");
    printf("    [-max_log2_cu_size num] - maximum log2 CU size to be used for quad-tree structure.\n");
    printf("                              Cannot be larger than log2_ctu_size, default is 4\n");
    printf("    [-block_size_mask num]  - bit mask specifying possible partition sizes\n");
    printf("    [-ctu_distance num]     - minimum distance between generated CTUs\n");
    printf("                              (in units of CTU), default is 3\n");
    printf("    [-gpb_off]              - specifies that regular P frames should be used, not GPB frames\n");
    printf("    [-bref]                 - arrange B frames in B pyramid reference structure\n");
    printf("    [-nobref]               - do not use B-pyramid\n");
    printf("    [-pak_ctu_file File]    - input file with per CTU information\n");
    printf("    [-pak_cu_file File]     - input file with per CU information\n");
    printf("    [-repack_ctrl_file File]- output/input file with repack control data for repack control generation/verify\n");
    printf("    [-repack_stat_file File]- input file with repack stat data for repack control verify\n");
    printf("    [-repack_str_file File] - input file with multiPakStr data for repack control generation/verify\n");
    printf("    [-log File]             - log output file\n");
    printf("    [-csv File]             - file to output statistics in CSV format\n"); // ignored
    printf("    [-config  ConfigFile]   - input configuration file\n"); // ignored
    printf("    [-sub_pel_mode num]     - specifies sub pixel precision for motion vectors. 0 - integer, 1 - half, 3 - quarter (0 is default)\n");
    printf("    [-mv_thres num]         - threshold for motion vectors in percents (0 is default)\n");
    printf("    [-numpredictors num]    - number of MV predictors enabled. Used in verification mode to check NumMvPredictors FEI control works correctly\n");
    printf("                            - Valid values are in range [1; 4] (4 is default)\n");
    printf("    [-split_thres num]      - thresholds for partitions in percents (0 is default)\n");
    printf("    [-DeltaQP value(s)]     - array of delta QP values for repack ctrl generation, separated by a space (8 values at max)\n");
    printf("    [-InitialQP value]      - the initial QP value for repack ctrl verify (26 is default)\n");
}

void InputParams::ParseInputString(msdk_char **strInput, mfxU8 nArgNum)
{
    try {

        //parse command line parameters
        for (mfxU8 i = 1; i < nArgNum; ++i)
        {
            //Verbose mode, output to command line. Missed in Help
            if (msdk_strcmp(strInput[i], MSDK_STRING("-verbose")) == 0
                || msdk_strcmp(strInput[i], MSDK_STRING("-v")) == 0)
                m_bVerbose = true;

            //I/O YUV files
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-i")) == 0)
                m_InputFileName = GetStringArgument(strInput, ++i, nArgNum);
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-o")) == 0)
                m_OutputFileName = GetStringArgument(strInput, ++i, nArgNum);

            //width
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-w")) == 0)
                m_width = GetIntArgument(strInput, ++i, nArgNum);

            //height
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-h")) == 0)
                m_height = GetIntArgument(strInput, ++i, nArgNum);

            //frame number
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-n")) == 0)
                m_numFrames = GetIntArgument(strInput, ++i, nArgNum);

            // Encoding structure

            // GOP size
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-g")) == 0)
                m_GopSize = GetIntArgument(strInput, ++i, nArgNum);

            // Number of B frames + 1
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-r")) == 0)
                m_RefDist = GetIntArgument(strInput, ++i, nArgNum);

            // DPB size
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-x")) == 0)
            {
                m_NumRef = GetIntArgument(strInput, ++i, nArgNum);
                if (m_NumRef > 16) throw std::string("ERROR: Invalid DPB size");
            }

            else if (msdk_strcmp(strInput[i], MSDK_STRING("-idr_interval")) == 0)
                m_nIdrInterval = GetIntArgument(strInput, ++i, nArgNum);

            // Number active references for P frames
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-num_active_P")) == 0)
            {
                m_NumRefActiveP = GetIntArgument(strInput, ++i, nArgNum);
                if (m_NumRefActiveP > 4) throw std::string("ERROR: Invalid num_active_P");
            }

            // Number of backward references for B frames
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-num_active_BL0")) == 0)
            {
                m_NumRefActiveBL0 = GetIntArgument(strInput, ++i, nArgNum);
                if (m_NumRefActiveBL0 > 4) throw std::string("ERROR: Invalid num_active_BL0");
            }

            // Number of forward references for B frames
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-num_active_BL1")) == 0)
            {
                m_NumRefActiveBL1 = GetIntArgument(strInput, ++i, nArgNum);
                if (m_NumRefActiveBL1 > 1) throw std::string("ERROR: Invalid num_active_BL1");
            }

            // Set parameters for b-pyramid
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-bref")) == 0)
            {
                m_BRefType = MFX_B_REF_PYRAMID;
            }
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-nobref")) == 0)
            {
                m_BRefType = MFX_B_REF_OFF;
            }

            // processing mode
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-generate")) == 0)
                m_ProcMode = GENERATE;
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-verify")) == 0)
                m_ProcMode = VERIFY;

            // test type

            // Enable split of CTU
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-gen_split")) == 0)
                m_TestType |= GENERATE_SPLIT;

            // Generate MVs, Inter-prediction test
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-gen_mv")) == 0)
                m_TestType |= GENERATE_MV;

            // Generate Intra CUs, Intra-prediction test
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-gen_intra")) == 0)
                m_TestType |= GENERATE_INTRA;

            // Generate Inter CUs, Inter-prediction test
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-gen_inter")) == 0)
                m_TestType |= GENERATE_INTER;

            // Generate long MVs which are outside of SW, Inter-prediction test
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-gen_pred")) == 0)
                m_TestType |= GENERATE_PREDICTION;

            // Generate or verify multi-repack control data
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-gen_repack_ctrl")) == 0)
                m_TestType |= GENERATE_REPACK_CTRL;

            // Drop data to file
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-pred_file")) == 0)
                m_PredBufferFileName = GetStringArgument(strInput, ++i, nArgNum);

            // Drop pictures structure to file
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-pic_file")) == 0) {
                m_PicStructFileName = GetStringArgument(strInput, ++i, nArgNum);
                m_TestType |= GENERATE_PICSTRUCT;
            }

            // Minimum log CU size to be used for quad-tree structure
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-min_log2_cu_size")) == 0)
                m_CTUStr.minLog2CUSize = GetIntArgument(strInput, ++i, nArgNum);

            // Maximum log CU size to be used for quad-tree structure
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-max_log2_cu_size")) == 0)
                m_CTUStr.maxLog2CUSize = GetIntArgument(strInput, ++i, nArgNum);

            // Disable CU splitting into PUs
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-no_cu_to_pu_split")) == 0)
                m_CTUStr.bCUToPUSplit = false;

            // Force per-MVP block size in the MVP output file to a supported value
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-force_extbuf_mvp_block_size")) == 0)
            {
                m_ForcedExtMVPBlockSize = GetIntArgument(strInput, ++i, nArgNum);
                m_bIsForceExtMVPBlockSize = true;
            }

            // Actual per-MVP block size used in generation algorithm
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-mvp_block_size")) == 0)
            {
                m_GenMVPBlockSize = GetIntArgument(strInput, ++i, nArgNum);
            }

            // Force only symmetric CU into PU partioning
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-force_symm_cu_part")) == 0)
            {
                m_CTUStr.bForceSymmetricPU = true;
            }

            // Log2 CTU size to be used for quad-tree structure. Also the maximum
            // possible CU size
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-log2_ctu_size")) == 0)
            {
                m_CTUStr.log2CTUSize = GetIntArgument(strInput, ++i, nArgNum);
                m_CTUStr.CTUSize = (mfxU32)(1 << m_CTUStr.log2CTUSize);
            }

            // Minimum log TU size to be used for quad-tree structure
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-min_log2_tu_size")) == 0)
                m_CTUStr.minLog2TUSize = GetIntArgument(strInput, ++i, nArgNum);

            // Maximum log TU size to be used for quad-tree structure
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-max_log2_tu_size")) == 0)
                m_CTUStr.maxLog2TUSize = GetIntArgument(strInput, ++i, nArgNum);

            // Maximum TU quad-tree depth inside CU
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-max_tu_qt_depth")) == 0)
                m_CTUStr.maxTUQTDepth = GetIntArgument(strInput, ++i, nArgNum);

            // which blocks to use in partitions (bitmask)
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-block_size_mask")) == 0)
                m_block_size_mask = GetIntArgument(strInput, ++i, nArgNum);

            // Minimum distance between generated CTUs in terms of CTU
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-ctu_distance")) == 0)
                m_CTUStr.CTUDist = GetIntArgument(strInput, ++i, nArgNum);

            // Read CTU data from file
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-pak_ctu_file")) == 0)
                m_PakCtuBufferFileName = GetStringArgument(strInput, ++i, nArgNum);

            // Read CU data from file
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-pak_cu_file")) == 0)
                m_PakCuBufferFileName = GetStringArgument(strInput, ++i, nArgNum);

            // Whether GPB frames should be used instead of regular P frames
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-gpb_off")) == 0)
                m_UseGPB = false;

            // Thresholds
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-mv_thres")) == 0)
                m_Thresholds.mvThres = GetIntArgument(strInput, ++i, nArgNum);
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-split_thres")) == 0)
                m_Thresholds.splitThres = GetIntArgument(strInput, ++i, nArgNum);
            // Number of enabled MV predictors. For disabled predictors upper MV-threshold is ON
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-numpredictors")) == 0)
                m_NumMVPredictors = GetIntArgument(strInput, ++i, nArgNum);

            // Motion prediction precision mode
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-sub_pel_mode")) == 0)
            {
                if (++i < nArgNum)
                {
                    m_SubPixelMode = ParseSubPixelMode(strInput[i]);
                }
                else
                {
                    throw std::string("ERROR: sub_pel_mode require an argument");
                }
            }

            // The multi-repack control data file
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-repack_ctrl_file")) == 0)
                m_RepackCtrlFileName = GetStringArgument(strInput, ++i, nArgNum);

            // The multiPakStr data file
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-repack_str_file")) == 0)
                m_RepackStrFileName = GetStringArgument(strInput, ++i, nArgNum);

            // The multi-repack stat data file
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-repack_stat_file")) == 0)
                m_RepackStatFileName = GetStringArgument(strInput, ++i, nArgNum);

            //Repack control num passes and delta QP
            else if((msdk_strcmp(strInput[i], MSDK_STRING("-DeltaQP")) == 0))
            {
                mfxU32 idxQP;
                for(idxQP = 0; idxQP < m_NumAddPasses; idxQP++)
                {
                    if(msdk_strncmp(strInput[i+1], MSDK_STRING("-"), 1) == 0)
                        break;

                    m_DeltaQP[idxQP] = (mfxU8)GetIntArgument(strInput, ++i, nArgNum);
                }
                m_NumAddPasses = idxQP;
            }

            //Repack control initial QP
            else if((msdk_strcmp(strInput[i], MSDK_STRING("-InitialQP")) == 0))
                m_InitialQP = GetIntArgument(strInput, ++i, nArgNum);

            else if (msdk_strcmp(strInput[i], MSDK_STRING("-log")) == 0)
            {
                m_bUseLog = true;
                m_LogFileName = GetStringArgument(strInput, ++i, nArgNum);
            }
            //help
            else if (msdk_strcmp(strInput[i], MSDK_STRING("-help")) == 0
                || msdk_strcmp(strInput[i], MSDK_STRING("--help")) == 0)
            {
                m_bPrintHelp = true;
            }
            else
                throw std::string("ERROR: Unknown input argument");
        }

        if (m_bPrintHelp)
        {
            PrintHelp();
            return;
        }

        if (m_TestType == UNDEFINED_TYPE)
            throw std::string("ERROR: Undefined test type");

        if (m_ProcMode == UNDEFINED_MODE)
            throw std::string("ERROR: Undefined mode");

        if (m_numFrames == 0)
            throw std::string("ERROR: Invalid number of frames");

        if ((m_TestType & GENERATE_PICSTRUCT) && m_TestType != GENERATE_PICSTRUCT)
            throw std::string("ERROR: Improper arguments mix with pic_struct");

        if (m_TestType == GENERATE_PICSTRUCT)
            return;

        //Repack control generation checking
        if (m_TestType & GENERATE_REPACK_CTRL)
        {
            if (m_TestType != GENERATE_REPACK_CTRL)
                throw std::string("ERROR: Improper arguments mix with gen_repack_ctrl");

            if (m_ProcMode == GENERATE)
            {
                if (m_NumAddPasses == 0 || m_NumAddPasses > HEVC_MAX_NUMPASSES)
                    throw std::string("ERROR: Wrong NumAddPasses value");

                if (!m_RepackCtrlFileName.empty() && !m_RepackStrFileName.empty())
                    return;
                else
                    throw std::string("ERROR: repack ctrl and str files required");
            }
            else if (m_ProcMode == VERIFY)
            {
                if (!m_RepackCtrlFileName.empty()
                    && !m_RepackStrFileName.empty()
                    && !m_RepackStatFileName.empty())
                    return;
                else
                    throw std::string("ERROR: repack ctrl, str and stat files required");
            }
            else
            {
                throw std::string("ERROR: Wrong proc mode");
            }
        }

        if (m_ProcMode == GENERATE && (m_TestType & (GENERATE_INTER | GENERATE_INTRA)) && (m_InputFileName.length() == 0 || m_OutputFileName.length() == 0))
            throw std::string("ERROR: input and output YUV files required");

        if (m_ProcMode == GENERATE && (m_TestType & GENERATE_PREDICTION)
            && m_PredBufferFileName.length() == 0)
            throw std::string("ERROR: To generate predictors output file is required");

        if ((m_ProcMode & VERIFY) && (m_TestType & (GENERATE_INTER | GENERATE_INTRA)) && m_PakCtuBufferFileName.length() == 0)
            throw std::string("ERROR: PAK CTU input file is required");

        if ((m_ProcMode & VERIFY) && (m_TestType & (GENERATE_INTER | GENERATE_INTRA)) && m_PakCuBufferFileName.length() == 0)
            throw std::string("ERROR: PAK CU input file is required");

        if (m_width == 0 || m_height == 0)
            throw std::string("ERROR: Invalid width or/and height values");

        if (m_CTUStr.CTUSize != 16 && m_CTUStr.CTUSize != 32 && m_CTUStr.CTUSize != 64)
            throw std::string("ERROR: Invalid CTU size specified");

        if (m_block_size_mask > 3 || m_block_size_mask == 0)
            // 64x64 blocks are unsupported on SKL
            throw std::string("ERROR: Incorrect block_size_mask");

        if (m_bIsForceExtMVPBlockSize && m_ForcedExtMVPBlockSize > 3)
            throw std::string("ERROR: Invalid forced MVP block size specified");

        if (m_GenMVPBlockSize > 3)
            throw std::string("ERROR: Invalid actual MVP block size specified");

        //Checking actual MVP block size and CTU size compatibily
        if (m_GenMVPBlockSize != 0)
        {
            if ((m_CTUStr.CTUSize == 16 && m_GenMVPBlockSize > 1))
            {
                throw std::string("ERROR: For 16x16 CTU actual buffer MVP block size should be less or equal than CTU size");
            }
            else if (m_CTUStr.CTUSize == 32 && m_GenMVPBlockSize > 2)
            {
                throw std::string("ERROR: For 32x32 CTU actual buffer MVP block size should be less or equal than CTU size");
            }
        }

        if ((m_TestType & (GENERATE_MV | GENERATE_PREDICTION)) && !(m_TestType & GENERATE_INTER))
            throw std::string("ERROR: MVs can't be generated w/o -gen_inter option");

        if ((m_TestType & GENERATE_SPLIT) && !(m_TestType & (GENERATE_INTER | GENERATE_INTRA)))
            throw std::string("ERROR: Splits can't be generated w/o -gen_inter or -gen_intra option");


        if ((m_ProcMode & VERIFY) && (m_TestType & GENERATE_INTER) && m_Thresholds.mvThres > 100)
            throw std::string("ERROR: Incorrect threshold for MVs in the verification mode");

        if ((m_ProcMode & VERIFY) && (m_TestType & (GENERATE_INTER | GENERATE_INTRA)) && m_Thresholds.splitThres > 100)
            throw std::string("ERROR: Incorrect threshold for splits in the verification mode");

        if ((m_ProcMode & VERIFY) && (m_TestType & (GENERATE_INTER | GENERATE_PREDICTION)) && m_NumMVPredictors > 4)
            throw std::string("ERROR: Incorrect number of enabled MV predictors in the verification mode");

        if (m_CTUStr.maxLog2CUSize < m_CTUStr.minLog2CUSize)
            throw std::string("ERROR: max_log2_cu_size should be greater than or equal to min_log2_tu_size");

        if ((mfxU32) 1 << m_CTUStr.maxLog2CUSize > m_CTUStr.CTUSize)
            throw std::string("ERROR: max_log2_cu_size should be less than or equal to log2_ctu_size");

        if (m_CTUStr.maxLog2TUSize < m_CTUStr.minLog2TUSize)
            throw std::string("ERROR: max_log2_tu_size should be greater than or equal to min_log2_tu_size");

        if (m_CTUStr.minLog2CUSize <= m_CTUStr.minLog2TUSize)
            throw std::string("ERROR: min_log2_cu_size should be greater than min_log2_tu_size");

        if (m_CTUStr.maxLog2TUSize > (std::min)(m_CTUStr.maxLog2CUSize, (mfxU32) 5))
            throw std::string("ERROR: max_log2_tu_size should be less than or equal to Min( log2_ctu_size, 5)");
    }
    catch (std::string& e) {
        cout << e << endl;
        throw std::string("ERROR: InputParams::ParseInputString");
    }

    return;
}

mfxU16 InputParams::ParseSubPixelMode(msdk_char * strRawSubPelMode)
{
    mfxU16 pelMode = msdk_atoi(strRawSubPelMode);

    switch (pelMode)
    {
    case 0:
    case 1:
    case 3:
        return pelMode;
    default:
        throw std::string("ERROR: Incorrect sub_pel_mode value");
    }
}

int InputParams::GetIntArgument(msdk_char **strInput, mfxU8 index, mfxU8 nArgNum)
{
    if (strInput == nullptr || *strInput == nullptr)
    {
        throw std::string("ERROR: GetIntArgument: null pointer reference");
    }
    if (index < nArgNum)
    {
       return msdk_atoi(strInput[index]);
    }
    else
    {
        throw std::string("ERROR: missing secondary integer argument");
    }
}

msdk_string InputParams::GetStringArgument(msdk_char **strInput, mfxU8 index, mfxU8 nArgNum)
{
    if (strInput == nullptr || *strInput == nullptr)
    {
        throw std::string("ERROR: GetStringArgument: null pointer reference");
    }
    if (index < nArgNum)
    {
        return msdk_string(strInput[index]);
    }
    else
    {
        throw std::string("ERROR: missing secondary string argument");
    }
}

#endif // MFX_VERSION
