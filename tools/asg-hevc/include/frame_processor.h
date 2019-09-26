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

#ifndef __ASG_HEVC_FRAME_PROCESSOR_H__
#define __ASG_HEVC_FRAME_PROCESSOR_H__

#include "mfxvideo.h"

#if MFX_VERSION >= MFX_VERSION_NEXT

#include <algorithm>
#include <numeric>
#include <random>

#include "util_defs.h"

#include "block_structures.h"
#include "frame_change_descriptor.h"
#include "intra_test.h"
#include "inter_test.h"
#include "mvmvp_processor.h"
#include "random_generator.h"

class FrameProcessor
{
public:
    FrameProcessor() = default;

    void Init(const InputParams &params);

    void ProcessFrame(FrameChangeDescriptor & frame_descr);

    // For verification
    void GenQuadTreeInCTUWithBitMask(CTUDescriptor& CTU, mfxU32 bitMask);

private:

    //work with particular samples in the frame
    bool IsSampleAvailable(mfxU32 X, mfxU32 Y);
    mfxU8 GetSampleI420(COLOR_COMPONENT comp, mfxU32 AdrX, mfxU32 AdrY, mfxFrameSurface1* surf);

    void GenRandomQuadTreeStructure(QuadTree & QT, mfxU8 minDepth, mfxU8 maxDepth);
    void GenCUVecInCTU(CTUDescriptor & ctu, mfxU16 test_type);
    void GenRandomTUQuadTreeInCU(CUBlock & cu_block);
    void GenRandomCUQuadTreeInCTU(CTUDescriptor & ctu);
    void GetRefSampleAvailFlagsForTUsInCTU(CTUDescriptor & CTU);
    //checking if reference samples for INTRA prediction of the current block are mostly constant
    //TODO: develop flexible criterion of mostly uniform ref samples
    bool IsBlockUniform(const BaseBlock& block, PatchBlock& tempFrame);
    void AlterBorderSamples(const BaseBlock& block, PatchBlock& tempFrame);
    //counting the particurlar intra mode prediction for TU
    void MakeTUIntraPrediction(const TUBlock& refBlock, PatchBlock& targetBlock);
    //worst intra mode is found here in order to reach more contrast
    void ChooseContrastIntraMode(const BaseBlock & block, std::vector<TUBlock>& tu_block_vec, PatchBlock& frame);
    //function that makes intra prediction for refBlock of particular colorComp plane and saves it into currPlane buffer
    void GetIntraPredPlane(const BaseBlock& refBlock, INTRA_MODE currMode, const PatchBlock& frame, COLOR_COMPONENT colorComp, mfxU8* currPlane);
    //function that makes a patch from refBlock with all color components intra predicted
    PatchBlock GetIntraPatchBlock(const TUBlock & refBlock, const PatchBlock& patch);
    //intra prediction for particluar TU block and particular intra mode is made here
    void MakeIntraPredInCTU(CTUDescriptor & ctu, FrameChangeDescriptor & descr);

    //only TU tree intraPartitionMode is determined here
    void MakeIntraCU(CUBlock & cu_block);
    void MakeInterCU(CUBlock & cu_block, mfxU16  test_type);
    mfxU8 CeilLog2(mfxU32 size);
    //methods used for INTRA prediction

    //filling vector with adjacent samples
    //all coordinates and sizes here are measured in samples of colorComp
    void FillIntraRefSamples(mfxU32 cSize, mfxU32 cAdrX, mfxU32 cAdrY, const PatchBlock& frame, COLOR_COMPONENT colorComp, std::vector<mfxU8>& refSamples);

    //choosing filter for the vector of reference samples and making it if needed
    void ThreeTapFilter(std::vector<mfxU8>& RefSamples, mfxU8 size);
    void StrongFilter(std::vector<mfxU8>& RefSamples, mfxU8 size);
    FILTER_TYPE ChooseFilter(std::vector<mfxU8>& RefSamples, mfxU8 size, INTRA_MODE intra_type);
    FILTER_TYPE MakeFilter(std::vector<mfxU8>& RefSamples, mfxU8 size, INTRA_MODE type);

    //making a projection if needed
    mfxU8 MakeProjRefArray(const std::vector<mfxU8>& RefSamples, mfxU8 size, const IntraParams& IntraMode, std::vector<mfxU8>& ProjRefSamples);

    //generating prediction using a perticular mode and saving it in IntraPatch structure
    void PlanarPrediction(const std::vector<mfxU8>& RefSamples, mfxU8 size, mfxU8 * patch);
    void DCPrediction(const std::vector<mfxU8>& RefSamples, mfxU8 size, mfxU8 * patch);
    void AngularPrediction(const std::vector<mfxU8>& RefSamples, mfxU8 size, IntraParams& IntraMode, mfxU8 * patch);
    void MakePostFilter(const std::vector<mfxU8>& RefSamples, mfxU8 cSize, INTRA_MODE currMode, mfxU8* currPlane);
    void GenerateIntraPrediction(const std::vector<mfxU8>& RefSamples, mfxU8 blockSize, INTRA_MODE currMode, mfxU8* currPlane);

    //function generating INTRA prediction for TU leaves of the tree
    void ApplyTUIntraPrediction(const TUBlock & block, ExtendedSurface& surf);
    void ApplyIntraPredInCTU(const CTUDescriptor & CTU, FrameChangeDescriptor & frame_descr);
    void PutPatchIntoFrame(const PatchBlock & BP, mfxFrameSurface1& surf);
    //end of intra methods

    //methods used for INTER prediction
    void GenPredFlagsForPU(PUBlock & PU, mfxU16 frameType);
    void ApplyInterPredInCTU(CTUDescriptor & CTU, FrameChangeDescriptor & frame_descr);

    void GenCTUParams(FrameChangeDescriptor & frame_descr);
    void GenAndApplyPrediction(FrameChangeDescriptor & frame_descr);
    bool MakeInterPredInCTU(CTUDescriptor & CTU, FrameChangeDescriptor & frameDescr);

    void PutNoiseBlocksIntoFrames(const PUBlock & pu, const FrameChangeDescriptor & frameDescr, mfxU32 num_coeff = 12, mfxU32 level = 48);
    void FillInBlock4x4(mfxU32 num_coeff, mfxU32 level, mfxU8 block[16]);
    void FillDeltaBlocks4x4(mfxI8 blockL0[16], mfxI8 blockL1[16]);
    void ApplyDeltaPerPixel(const PUBlock & PU, const mfxI8 deltaBlock[16], const mfxU8 inBlock[16], mfxU8 outBlock[16]);
    void Inverse4x4(mfxI32 *src, mfxU32 s_pitch, mfxI32 *dst, mfxU32 d_pitch);
    void PutBlock4x4(mfxU32 x0, mfxU32 y0, mfxU8 block[16], mfxFrameSurface1 * surf);
    mfxU8 ClipIntToChar(mfxI32 x);
    void TraceBackAndPutBlockIntoFrame(const PUBlock & PU, FrameChangeDescriptor & descr);
    InterpolWorkBlock GetInterpolWorkBlockPreWP(const BaseBlock & PU, std::pair<mfxU32, mfxU32> fractOffset, mfxFrameSurface1 * surfTo);
    PatchBlock ApplyDefaultWeightedPrediction(InterpolWorkBlock & workBlockL0);
    PatchBlock ApplyDefaultWeightedPrediction(InterpolWorkBlock & workBlockL0, InterpolWorkBlock & workBlockL1);

    mfxU8 SetCorrectMVPBlockSize(mfxU8 mvpBlockSizeParam);

    void UnlockSurfaces(FrameChangeDescriptor & frame_descr);

    void GenRandomQuadTreeSubstrRecur(QuadTreeNode & node, mfxU8 minDepth, mfxU8 maxDepth);
    // For verification
    void GenQuadTreeWithBitMaskRecur(QuadTreeNode& node, mfxU32 bitMask);
    // Used only in quarter-pixel interpolation. Luma sample value should fit in mfxI32.
    mfxI32 GetClippedSample(COLOR_COMPONENT comp, mfxI32 AdrX, mfxI32 AdrY, mfxFrameSurface1* surf);

    mfxI32 CalculateLumaPredictionSamplePreWP(const std::pair<mfxU32, mfxU32>& refSampleLocationFull,
        const std::pair<mfxU32, mfxU32>& refSampleLocationFract, mfxFrameSurface1* refSurface);

    mfxI32 ApplyVerticalSubSampleLumaFilter(mfxU32 x, mfxU32 y, mfxFrameSurface1 * refSurface, const mfxI32* coeff);
    mfxI32 ApplyHorizontalSubSampleLumaFilter(mfxU32 x, mfxU32 y, mfxFrameSurface1 * refSurface, const mfxI32* coeff);

    // These function used in only in CalculateLumaPredictionSamplePreWP
    // In specifictation default weighted prediction is the final scaling step for sample prediction. (p.168)
    mfxU8 GetDefaultWeightedPredSample(mfxI32 predSampleLX);
    mfxU8 GetDefaultWeightedPredSample(mfxI32 predSampleL0, mfxI32 predSampleL1);

    mfxU32 m_Height      = 0; // in pixels
    mfxU32 m_Width       = 0;
    mfxU32 m_HeightInCTU = 0; // in CTUs
    mfxU32 m_WidthInCTU  = 0;
    mfxU32 m_CropH       = 0; // Unaligned size
    mfxU32 m_CropW       = 0;

    mfxU16 m_SubPelMode  = 0; // Valid values are: 0, 1 or 3

    bool    m_IsForceExtMVPBlockSize = false;
    mfxU32  m_ForcedExtMVPBlockSize  = 0;
    mfxU32  m_GenMVPBlockSize        = 0;

    CTUStructure m_CTUStr; // Some parameters related to CTU generation, i.e. restrictions on CTUs

    PROCESSING_MODE m_ProcMode = UNDEFINED_MODE; // processing mode
};

#endif // MFX_VERSION

#endif // __ASG_HEVC_FRAME_PROCESSOR_H__
