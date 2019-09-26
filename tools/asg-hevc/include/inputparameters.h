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

#ifndef __ASG_HEVC_INPUT_PARAMETERS_H__
#define __ASG_HEVC_INPUT_PARAMETERS_H__

#include "mfxvideo.h"

#if MFX_VERSION >= MFX_VERSION_NEXT

#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

#include "base_allocator.h"
#include "hevc_defs.h"
#include "util_defs.h"

enum PROCESSING_MODE
{
    UNDEFINED_MODE = 0,
    GENERATE,
    VERIFY
};

// TODO: use std::bitset instead
enum
{
    UNDEFINED_TYPE      = 0,
    GENERATE_MV         = 1 << 0,
    GENERATE_PREDICTION = 1 << 1,
    GENERATE_SPLIT      = 1 << 2,
    GENERATE_INTRA      = 1 << 3,
    GENERATE_INTER      = 1 << 4,
    GENERATE_PICSTRUCT  = 1 << 5,
    GENERATE_REPACK_CTRL  = 1 << 6
};

enum FRAME_MARKER
{
    GEN  = 1,
    MOD  = 2,
    SKIP = 3
};

// Restrictions on CTU for generation of noisy blocks
struct CTUStructure
{
    //Below are default values that should be most relaxed
    mfxU32 log2CTUSize   = HEVC_MIN_LOG2_CTU_SIZE;
    mfxU32 CTUSize       = 1 << log2CTUSize;
    mfxU32 minLog2CUSize = HEVC_MIN_LOG2_CU_SIZE;
    mfxU32 maxLog2CUSize = HEVC_MIN_LOG2_CTU_SIZE;      //NB: can be less than CTU size inside asg-hevc
                                                        //Standard specifies that CTU size is set equal to
                                                        //1 << maxLog2CUSize, but this is for encoder only:
                                                        //inside asg-hevc, we will maintain this separate upper
                                                        //limit on max CU size

    mfxU32 minLog2TUSize = HEVC_MIN_LOG2_TU_SIZE;       //Must be less than minLog2CUSize
    mfxU32 maxLog2TUSize = HEVC_MIN_LOG2_CTU_SIZE;
    mfxU32 maxTUQTDepth  = HEVC_MAX_LOG2_CTU_SIZE - HEVC_MIN_LOG2_TU_SIZE; //Overrides min TU size while
                                                                           //building TU quadtree in CU
    mfxU32 CTUMaxNum     = 128;
    mfxU32 MVRange       = 12;
    mfxU32 CTUDist       = 3;    //Default minimum distance between generated CTU blocks in terms of CTU

    bool   bCUToPUSplit      = true;  //Whether or not CUs should be further split into PUs
    bool   bForceSymmetricPU = false; //Support only symmetric modes for CU into PU partioning

    inline mfxU32 GetMaxNumCuInCtu()
    {
        return (1 << (2 * (log2CTUSize - HEVC_MIN_LOG2_CU_SIZE)));
    }
};

struct Frame
{
    mfxU32 DisplayOrder = 0xffffffff;
    mfxU32 Type         = MFX_FRAMETYPE_UNKNOWN;
    mfxU8  NalType      = 0xff;
    mfxI32 Poc          = -1;
    mfxU32 Bpo          = 0xffffffff;
    bool   bSecondField = false;
    bool   bBottomField = false;
    mfxI32 LastRAP      = -1;
    mfxI32 IPoc         = -1;
    mfxI32 PrevIPoc     = -1;
    mfxI32 NextIPoc     = -1;
};

struct ExternalFrame
{
    mfxU32 DisplayOrder;
    mfxU32 Type;
    mfxU8  NalType;
    mfxI32 Poc;
    bool   bSecondField;
    bool   bBottomField;
    std::vector<Frame> ListX[2];
};

// Structure which used for initial stream marking

struct FrameProcessingParam
{
    FRAME_MARKER Mark   = SKIP;
    std::vector<mfxU32> ReferencesL0;
    std::vector<mfxU32> ReferencesL1;

    mfxU32 DisplayOrder = 0xffffffff;
    mfxU32 EncodedOrder = 0xffffffff;

    mfxU32 Type         = MFX_FRAMETYPE_UNKNOWN;
    std::vector<Frame> ListX[2];

    FrameProcessingParam & operator= (const ExternalFrame& rhs)
    {
        DisplayOrder = rhs.DisplayOrder;
        Type = rhs.Type;
        ListX[0] = rhs.ListX[0];
        ListX[1] = rhs.ListX[1];

        return *this;
    }
};

struct Thresholds
{
    // asg should exit with non-zero if MV/split-passrate is lower than threshold
    mfxU32 mvThres      = 0;
    mfxU32 splitThres   = 0;
};

class InputParams
{
public:

    void ParseInputString(msdk_char **strInput, mfxU8 nArgNum);

    mfxU16          m_TestType = UNDEFINED_TYPE;
    PROCESSING_MODE m_ProcMode = UNDEFINED_MODE;

    mfxU32 m_width           = 0;
    mfxU32 m_height          = 0;
    mfxU32 m_numFrames       = 0;

    mfxU16 m_PicStruct       = MFX_PICSTRUCT_PROGRESSIVE;
    mfxU16 m_BRefType        = MFX_B_REF_OFF;
    mfxU16 m_GopOptFlag      = 0;
    mfxU16 m_GopSize         = 1;
    mfxU16 m_RefDist         = 1;
    mfxU16 m_NumRef          = 1;
    mfxU16 m_nIdrInterval    = 0xffff;
    mfxU16 m_NumRefActiveP   = 3;
    mfxU16 m_NumRefActiveBL0 = 3;
    mfxU16 m_NumRefActiveBL1 = 1;
    bool   m_UseGPB          = true;

    // Which blocks to use (it might be possible to forbid CUs of some sizes)
    // TODO: extend and add to code
    mfxU16 m_block_size_mask = 3; // 0 - invalid, 001 - 16x16, 010 - 32x32, 100 - 64x64

    // Specifies which sub pixel precision mode is used in motion prediction.
    // Valid values are: 0 ; 1 ; 3 (integer, half, quarter). 0x0 is default
    mfxU16 m_SubPixelMode = 0;

    bool m_bVerbose       = false;
    bool m_bPrintHelp     = false;
    bool m_bUseLog        = false;

    // Specifies the external MVP block size which is written to mfxFeiHevcEncMVPredictors::BlockSize
    // Valid values are: 0 ; 1; 2; 3 (no MVP, MVP enabled for 16x16/32x32/64x64 block)
    bool m_bIsForceExtMVPBlockSize = false;
    mfxU32 m_ForcedExtMVPBlockSize = 0;

    // Specifies MVP block size used in the actual generation
    // Valid values are the same as for m_ForcedExtMVPBlockSize
    mfxU32 m_GenMVPBlockSize = 0;

    // For repack ctrl
    mfxU32 m_NumAddPasses = 8; // Number of additional passes w/o first pass with clear QP value
    mfxU8 m_DeltaQP[8] = {1,2,3,3,4,4,4,4};
    mfxU8 m_InitialQP = 26;

    msdk_string m_InputFileName;
    msdk_string m_OutputFileName;

    msdk_string m_LogFileName;

    msdk_string m_PredBufferFileName;   // filename for predictors ext buffer output
    msdk_string m_PicStructFileName;    // filename for pictures structure output

    msdk_string m_PakCtuBufferFileName; // filename for PAK CTU ext buffer input
    msdk_string m_PakCuBufferFileName;  // filename for PAK CU ext buffer input

    msdk_string m_RepackCtrlFileName;   // filename for repack constrol output/input
    msdk_string m_RepackStrFileName;    // filename for multiPakStr input
    msdk_string m_RepackStatFileName;   // filename for repack stat input

    CTUStructure m_CTUStr;

    Thresholds   m_Thresholds;
    // Actual number of MV predictors enabled in FEI ENCODE. Used in verification mode
    mfxU16       m_NumMVPredictors = 4;

    std::vector<FrameProcessingParam> m_vProcessingParams; // FrameProcessingParam for entire stream

private:
    mfxU16 ParseSubPixelMode(msdk_char * strRawSubPelMode);
    int GetIntArgument(msdk_char ** strInput, mfxU8 index, mfxU8 nArgNum);
    msdk_string GetStringArgument(msdk_char ** strInput, mfxU8 index, mfxU8 nArgNum);
};

#endif // MFX_VERSION

#endif // __ASG_HEVC_INPUT_PARAMETERS_H__
