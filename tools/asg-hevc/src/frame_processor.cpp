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

#include "frame_processor.h"
#include "random_generator.h"


// Initialize parameters
void FrameProcessor::Init(const InputParams &params)
{
    m_CropH = params.m_height;
    m_CropW = params.m_width;

    m_CTUStr = params.m_CTUStr;

    // align by CUSize
    m_Height = MSDK_ALIGN(m_CropH, m_CTUStr.CTUSize);
    m_Width  = MSDK_ALIGN(m_CropW, m_CTUStr.CTUSize);

    m_HeightInCTU = m_Height / m_CTUStr.CTUSize;
    m_WidthInCTU  = m_Width  / m_CTUStr.CTUSize;

    m_ProcMode = params.m_ProcMode;

    m_SubPelMode = params.m_SubPixelMode;

    m_IsForceExtMVPBlockSize = params.m_bIsForceExtMVPBlockSize;
    m_ForcedExtMVPBlockSize  = params.m_ForcedExtMVPBlockSize;
    m_GenMVPBlockSize        = SetCorrectMVPBlockSize(params.m_GenMVPBlockSize);
}

// Beginning of processing of current frame. Only MOD frames are processed

void FrameProcessor::ProcessFrame(FrameChangeDescriptor & frame_descr)
{
    try
    {
        switch (frame_descr.m_changeType)
        {
        case GEN:
            return;
            break;

        case MOD:
            GenCTUParams(frame_descr);
            GenAndApplyPrediction(frame_descr);
            UnlockSurfaces(frame_descr);
            break;

        case SKIP:
            UnlockSurfaces(frame_descr);
            break;

        default:
            return;
            break;
        }
    }
    catch (std::string & e) {
        std::cout << e << std::endl;
        throw std::string("ERROR: FrameProcessor::ProcessFrame");
    }
    return;
}

bool FrameProcessor::IsSampleAvailable(mfxU32 AdrX, mfxU32 AdrY)
{
    return (AdrY < m_CropH && AdrX < m_CropW);
}

// Only I420 color format are supported
mfxU8 FrameProcessor::GetSampleI420(COLOR_COMPONENT comp, mfxU32 AdrX, mfxU32 AdrY, mfxFrameSurface1* surf)
{
    if (surf == nullptr)
    {
        throw std::string("ERROR: GetSampleI420: null pointer reference");
    }

    switch (comp)
    {
    case LUMA_Y:
        return surf->Data.Y[AdrY * surf->Data.Pitch + AdrX];
    case CHROMA_U:
        return surf->Data.U[(AdrY / 2) * (surf->Data.Pitch / 2) + (AdrX / 2)];
    case CHROMA_V:
        return surf->Data.V[(AdrY / 2) * (surf->Data.Pitch / 2) + (AdrX / 2)];
    default:
        throw std::string("ERROR: Trying to get unspecified component");
    }
}

mfxI32 FrameProcessor::GetClippedSample(COLOR_COMPONENT comp, mfxI32 X, mfxI32 Y, mfxFrameSurface1 * surf)
{
    mfxU32 clippedX = Clip3(X, 0, (mfxI32)m_CropW);
    mfxU32 clippedY = Clip3(Y, 0, (mfxI32)m_CropH);

    return GetSampleI420(comp, clippedX, clippedY, surf);
}

// These functions are to be used after applying CalculateLumaPredictionSamplePreWP
// In specification default weighted prediction is the final scaling step for sample prediction.
// predSampleL0 is an interpolated Luma sample value which is calculated in CalculateLumaPredictionSamplePreWP()
// Output is final scaled and rounded Luma value which returns in CalculateLumaPredictionSamplePreWP()
// See 8.5.3.3.4.2 "Default weighted sample prediction process" from 4.0 ITU-T H.265 (V4) 2016-12-22
mfxU8 FrameProcessor::GetDefaultWeightedPredSample(mfxI32 predSampleLx)
{
    // These shift and offset variables are defined in H265 standard for 8 bit color depth
    constexpr mfxU16 shift1 = 6;
    constexpr mfxU16 offset1 = 1 << (shift1 - 1); // 2^5 = 32

    constexpr mfxI32 upperBound = (1 << 8) - 1; // 2^8 - 1 = 255
    mfxI32 predSample = (predSampleLx + offset1) >> shift1;

    return mfxU8(Clip3(predSample, 0, upperBound));
}

// Alternative in case working with B-frames
mfxU8 FrameProcessor::GetDefaultWeightedPredSample(mfxI32 predSampleL0, mfxI32 predSampleL1)
{
    // These shift and offset variables are defined in H265 standard for 8 bit color depth
    mfxU16 shift2 = 7;
    mfxU16 offset2 = 1 << (shift2 - 1); // 2^6 = 64

    mfxI32 upperBound = (1 << 8) - 1;   // 2^8 - 1 = 255

    mfxI32 predSample = (predSampleL0 + predSampleL1 + offset2) >> shift2;

    return mfxU8(Clip3(predSample, 0, upperBound));
}

void FrameProcessor::GenRandomQuadTreeStructure(QuadTree& QT, mfxU8 minDepth, mfxU8 maxDepth)
{
    if (!QT.IsEmpty())
    {
        QT.Clear();
    }

    GenRandomQuadTreeSubstrRecur(QT.root, minDepth, maxDepth);
}

void FrameProcessor::GenRandomQuadTreeSubstrRecur(QuadTreeNode& node, mfxU8 minDepth, mfxU8 maxDepth)
{
    if (node.m_Level < minDepth ||
        (node.m_Level < maxDepth && GetRandomGen().GetRandomBit()))
    {
        node.MakeChildren();
        for (auto& child : node.m_Children)
        {
            GenRandomQuadTreeSubstrRecur(child, minDepth, maxDepth);
        }
    }
    return;
}

//Make a quad-tree structure inside ctu's QuadTree from FEI output
void FrameProcessor::GenQuadTreeInCTUWithBitMask(CTUDescriptor& CTU, mfxU32 bitMask)
{
    if (bitMask & 1)
    {
        CTU.m_CUQuadTree.root.MakeChildren();
        GenQuadTreeWithBitMaskRecur(CTU.m_CUQuadTree.root, bitMask >> 1);
    }

    return;
}

void FrameProcessor::GenQuadTreeWithBitMaskRecur(QuadTreeNode& node, mfxU32 bitMask)
{
    for (mfxU32 i = 0; i < 4; i++)
    {
        if (bitMask & (1 << i))
        {
            node.m_Children[i].MakeChildren();

            if (node.m_Children[i].m_Level < 2)
            {
                GenQuadTreeWithBitMaskRecur(node.m_Children[i], bitMask >> 4 * (i + 1));
            }
        }
    }

    return;
}

//Fills CU vector inside CTU with correct CU blocks in CTU quad-tree z-scan order
//For each CU, depending on the test type, its prediction mode is selected
//and further CU partitioning/mode selection is made (i.e. PU and TU partitioning
//and per TU intra mode selection)
void FrameProcessor::GenCUVecInCTU(CTUDescriptor& ctu, mfxU16 testType)
{
    QuadTree& QT = ctu.m_CUQuadTree;
    std::vector<BaseBlock> tmpVec;
    QT.GetQuadTreeBlocksRecur(QT.root, ctu.m_AdrX, ctu.m_AdrY, ctu.m_BHeight, tmpVec);

    for (auto& block : tmpVec)
    {
        CUBlock cu_block(block);

        if (testType & GENERATE_INTER && testType & GENERATE_INTRA)
        {
            cu_block.m_PredType = (GetRandomGen().GetRandomBit()) ?
                INTRA_PRED : INTER_PRED;
        }
        else if (testType & GENERATE_INTER)
        {
             cu_block.m_PredType = INTER_PRED;
        }
        else
        {
            cu_block.m_PredType = INTRA_PRED;
        }

        if (cu_block.m_PredType == INTRA_PRED)
        {
            MakeIntraCU(cu_block);
        }
        else if (cu_block.m_PredType == INTER_PRED)
        {
            MakeInterCU(cu_block, testType);
        }

        ctu.m_CUVec.push_back(cu_block);
    }
}

void FrameProcessor::GetRefSampleAvailFlagsForTUsInCTU(CTUDescriptor & CTU)
{
    QuadTree& QT = CTU.m_CUQuadTree;
    std::vector<RefSampleAvail> CURefSampleAvailVec;
    //CTU is set with RefSamples available in both directions here.
    //this should be made by guaranteeing that CTUs are far from each other
    //(at least one initial CTU between two substituted CTUs in both directions)
    QT.GetQuadTreeRefSampleAvailVector(QT.root, CTU, CTU, CURefSampleAvailVec);

    if (CURefSampleAvailVec.size() != CTU.m_CUVec.size())
    {
        throw std::string("ERROR: GetRefSampleAvailFlagsForTUsInCTU: mismatching CU and RefSampleAvail vector sizes");
    }

    for (mfxU32 i = 0; i < CTU.m_CUVec.size(); i++)
    {
        CUBlock & CU = CTU.m_CUVec[i];
        if (CU.m_PredType == INTRA_PRED)
        {
            QuadTree& RQT = CU.m_TUQuadTree;
            std::vector<RefSampleAvail> TURefSampleAvailVec;

            RQT.GetQuadTreeRefSampleAvailVectorRecur(RQT.root, CU, CURefSampleAvailVec[i], CTU, TURefSampleAvailVec);
            for (mfxU32 j = 0; j < CU.m_TUVec.size(); j++)
            {
                TUBlock& TU = CU.m_TUVec[j];
                TU.m_RefSampleAvail = TURefSampleAvailVec[j];
            }
        }
    }
}

void FrameProcessor::GenRandomTUQuadTreeInCU(CUBlock& cu_block)
{
    QuadTree& quadTreeTU = cu_block.m_TUQuadTree;
    mfxU32 minTUDepth = std::max(0, mfxI32(CeilLog2(cu_block.m_BHeight) - m_CTUStr.maxLog2TUSize));
    mfxU32 maxTUDepth = std::min(CeilLog2(cu_block.m_BHeight) - m_CTUStr.minLog2TUSize, m_CTUStr.maxTUQTDepth);

    GenRandomQuadTreeStructure(quadTreeTU, minTUDepth, maxTUDepth);
}


//Make a quad-tree structure inside ctu's QuadTree so that all CU blocks inside CTU
//have a size smaller than specified by maxLog2CUSize and larger than specified by minLog2CUSize
void FrameProcessor::GenRandomCUQuadTreeInCTU(CTUDescriptor& ctu)
{
    QuadTree& quadTreeCU = ctu.m_CUQuadTree;
    mfxU32 minCUDepth = std::max(0, mfxI32(CeilLog2(ctu.m_BHeight) - m_CTUStr.maxLog2CUSize));
    mfxU32 maxCUDepth = CeilLog2(ctu.m_BHeight) - m_CTUStr.minLog2CUSize;
    GenRandomQuadTreeStructure(quadTreeCU, minCUDepth, maxCUDepth);
}

//TODO:
bool FrameProcessor::IsBlockUniform(const BaseBlock& block, PatchBlock& frame)
{
    return false;
}

//TODO:
void FrameProcessor::AlterBorderSamples(const BaseBlock& block, PatchBlock& frame)
{

}

void FrameProcessor::ChooseContrastIntraMode(const BaseBlock& block, std::vector<TUBlock>& block_vec, PatchBlock& frame)
{
    if (IsBlockUniform(block, frame))
    {
        AlterBorderSamples(block, frame);
    }

    //distance between initial block and intra predicted patch should be maximized
    mfxU32 maxDist = 0; //max num of diff that can be reached is 32x32x256 = 2^18
    //initial block filled with samples from the frame
    PatchBlock refBlock(block, frame);
    //here Patch with max distance from the initBlock will be stored
    PatchBlock maxDistPatch(block);
    //mode corresponding to maxDistPatch
    INTRA_MODE maxDistMode = PLANAR;

    //choose TUs inside block
    std::vector<TUBlock> TUInCurrBlock;
    for (auto& TU : block_vec)
    {
        if (TU.IsInBlock(block))
        {
            TUInCurrBlock.emplace_back(TU);
        }
    }

    //check whether there is TU inside block with no left-down or up-right refSamples available
    //if so, we can't use corresponding modes and we limit maxModeAvail minModeAvail
    // and minAngModeAvail in appropriate manner
    //if left-down samples aren't available, modes 2 - 9 are prohibited
    //if up-right samples aren't available, modes 27 - 34 are prohibited
    //in both cases planar mode is prohibited because it uses samples p[-1][N] and p[N][-1]
    //in coordinates relative to the block, where N is the size of block
    //see HEVC algorighms and structures page 101(112)
    //Vivienne Sze Madhukar Budagavi Gary J.Sullivan High Efficiency Video Coding(HEVC) Algorithms and Architectures 2014

    mfxU32 minModeAvail    = INTRA_MODE::PLANAR;
    mfxU32 minAngModeAvail = INTRA_MODE::ANG2;
    mfxU32 maxModeAvail    = INTRA_MODE::ANG34;
    for (auto& TU : TUInCurrBlock)
    {
        if (!TU.m_RefSampleAvail.LeftDown)
        {
            minAngModeAvail = INTRA_MODE::ANG10_HOR;
            minModeAvail    = INTRA_MODE::DC;
        }
        if (!TU.m_RefSampleAvail.UpRight)
        {
            maxModeAvail = INTRA_MODE::ANG26_VER;
            minModeAvail = INTRA_MODE::DC;
        }
    }

    for (mfxU32 i = minModeAvail; i <= maxModeAvail; i++)
    {
        if (i < INTRA_MODE::ANG2 || i >= minAngModeAvail)
        {
            //iterate over TUs of current CU and save predicted TUs into frame
            for (auto& TU : TUInCurrBlock)
            {
                TU.m_IntraModeChroma = TU.m_IntraModeLuma = INTRA_MODE(i);
                MakeTUIntraPrediction(TU, frame);
            }
            //get curPatch from frame
            PatchBlock curPatch(block, frame);
            //count distance between initial block and current patch
            mfxU32 curDist = refBlock.CalcYSAD(curPatch);
            //save patch which is the farthest from the initial CU block
            if (curDist > maxDist)
            {
                maxDist = curDist;
                maxDistPatch = curPatch;
                maxDistMode = INTRA_MODE(i);
            }
        }
    }

    for (auto& TU : block_vec)
    {
        if (TU.IsInBlock(block))
        {
            //for now chroma intra mode is set equal to luma intra mode
            TU.m_IntraModeChroma = TU.m_IntraModeLuma = maxDistMode;
        }
    }

    frame.InsertAnotherPatch(maxDistPatch);

    return;
}

//Generates a quad-tree TU structure inside the cu_block and fills the TU block vector
//inside cu_block with correct TU blocks corresponding to the CU quad-tree TU structure
//Selects an intra mode for each generated TU
void FrameProcessor::MakeIntraCU(CUBlock& cu_block)
{
    QuadTree& QT = cu_block.m_TUQuadTree;
    GenRandomTUQuadTreeInCU(cu_block);

    std::vector<BaseBlock> tmpVec;
    QT.GetQuadTreeBlocksRecur(QT.root, cu_block.m_AdrX, cu_block.m_AdrY, cu_block.m_BHeight, tmpVec);
    cu_block.m_TUVec.clear();
    for (auto& block : tmpVec)
    {
        cu_block.m_TUVec.emplace_back(TUBlock(block, PLANAR, PLANAR));
    }

    mfxU32 minCUSize = 1 << m_CTUStr.minLog2CUSize;
    //Special case for CUs of size equal to minCUSize:
    //we can choose intra mode for every quarter
    //see last paragraph on p.228(238) in HEVC Algorithms and Architectures
    if (cu_block.m_BHeight == minCUSize && cu_block.m_BWidth == minCUSize
        && tmpVec.size() >= 4 && GetRandomGen().GetRandomBit())
    {
        cu_block.m_IntraPartMode = INTRA_NxN;
    }
    else
    {
        cu_block.m_IntraPartMode = INTRA_2Nx2N;
    }
}

void FrameProcessor::MakeIntraPredInCTU(CTUDescriptor& ctu, FrameChangeDescriptor & descr)
{
    ExtendedSurface& surf = *descr.m_frame;
    //save frame data in temporary patchBlock
    PatchBlock framePatchBlock(BaseBlock(0, 0, surf.Info.CropW, surf.Info.CropH), surf);
    for (auto& cu : ctu.m_CUVec)
    {
        if (cu.m_PredType == INTRA_PRED)
        {
            if (cu.m_IntraPartMode == INTRA_NxN)
            {
                std::vector<BaseBlock>  childrenBlocks;
                cu.GetChildBlock(childrenBlocks);
                for (mfxU32 i = 0; i < 4; i++)
                {
                    //choose the most contrast intra mode for every quarter of CU
                    ChooseContrastIntraMode(childrenBlocks[i], cu.m_TUVec, framePatchBlock);
                }
            }
            else
            {
                ChooseContrastIntraMode(cu, cu.m_TUVec, framePatchBlock);
            }
        }
    }
}

//Chooses the inter partitioning mode for the CU and fills the PU vector inside it with PUs
//corresponding to the chosen mode
void FrameProcessor::MakeInterCU(CUBlock& cu_block, mfxU16  testType)
{
    INTER_PART_MODE mode  = INTER_PART_MODE::INTER_NONE;

    mfxU32 max_mode_num = -1;

    if (!(testType & GENERATE_SPLIT) || !m_CTUStr.bCUToPUSplit)
    {
        mode = INTER_PART_MODE::INTER_2Nx2N; //If no split is specified or CU to PU split is forbidden,
                                             //the CU will contain a single PU
    }
    else
    {
        if (cu_block.m_BHeight == 8 && cu_block.m_BWidth == 8)
        {
            //Minimum PU size is 4x8 or 8x4, which means that only first 3 inter partitioning
            //modes are available for 8x8 CU
            //Condition when only symmetric modes are supported in case of 8x8 CU is satisfied by default
            max_mode_num = INTER_8x8CU_PART_MODE_NUM - 1;
        }
        else if (m_CTUStr.bForceSymmetricPU)
        {
            //No check for 8x8 CU case here. It is already processed
            max_mode_num = INTER_SYMM_PART_MODE_NUM - 1;
        }
        else
        {
            max_mode_num = INTER_PART_MODE_NUM - 1;
        }

        bool isCUMinSized = (CeilLog2(cu_block.m_BHeight) == m_CTUStr.minLog2CUSize);
        // CU split into 4 square PUs is only allowed for the CUs
        // of the smallest size (see p.61-62 of doi:10.1007/978-3-319-06895-4)
        do
        {
            mode = (INTER_PART_MODE) GetRandomGen().GetRandomNumber(0, max_mode_num);
        }
        while (!isCUMinSized && mode == INTER_PART_MODE::INTER_NxN);

    }

    cu_block.BuildPUsVector(mode);
    cu_block.m_InterPartMode = mode;
}

mfxU8 FrameProcessor::CeilLog2(mfxU32 size)
{
    mfxU8 ret = 0;
    while (size > 1)
    {
        size /= 2;
        ret++;
    }
    return ret;
}

// First round of generation: choose CTU on current MOD frame
//
// FrameChangeDescriptor & frameDescr - descriptor of current MOD frame

void FrameProcessor::GenCTUParams(FrameChangeDescriptor & frame_descr)
{
    mfxI32 maxAttempt = 100;

    // Try no more than maxAttempt times to generate no more than m_CTUStr.CTUMaxNum CTUs
    for (mfxI32 i = 0; i < maxAttempt && frame_descr.m_vCTUdescr.size() < m_CTUStr.CTUMaxNum; ++i)
    {
        // TODO: this part could be improved with aggregate initialization
        CTUDescriptor tempCTUDsc;

        // Do not choose last CTU in row/column to avoid effects of alignment
        // m_WidthInCTU-1 is a coordinate of last CTU in row
        tempCTUDsc.m_AdrXInCTU = GetRandomGen().GetRandomNumber(0, m_WidthInCTU  - 2);
        tempCTUDsc.m_AdrYInCTU = GetRandomGen().GetRandomNumber(0, m_HeightInCTU - 2);

        // Calculate pixel coordinates and size
        tempCTUDsc.m_AdrX = tempCTUDsc.m_AdrXInCTU * m_CTUStr.CTUSize;
        tempCTUDsc.m_AdrY = tempCTUDsc.m_AdrYInCTU * m_CTUStr.CTUSize;
        tempCTUDsc.m_BWidth = tempCTUDsc.m_BHeight = m_CTUStr.CTUSize;

        // Checks if current CTU intersects with or is too close to any of already generated
        auto it = find_if(frame_descr.m_vCTUdescr.begin(), frame_descr.m_vCTUdescr.end(),
            [&](const CTUDescriptor& dscr){ return dscr.CheckForIntersect(tempCTUDsc, m_CTUStr.CTUSize * m_CTUStr.CTUDist, m_CTUStr.CTUSize * m_CTUStr.CTUDist); });

        if (it == frame_descr.m_vCTUdescr.end())
        {
            // If no intersection, put generated block to vector
            frame_descr.m_vCTUdescr.push_back(std::move(tempCTUDsc));
        }
    }

    return;
}

// Second round of test: Generate partitions and MVs. Write down pixels to MOD and all reference GEN frames
// Remove a CTU generated in the previous round from the test set
// if it is impossible to place it without intersecting other CTUs
//
// FrameChangeDescriptor & frameDescr - descriptor of current MOD frame

void FrameProcessor::GenAndApplyPrediction(FrameChangeDescriptor & frameDescr)
{
    // Iterate over all generated in GenCTUParams CTUs (see round 1)
    auto it_ctu = frameDescr.m_vCTUdescr.begin();

    if (!(frameDescr.m_testType & GENERATE_SPLIT))
    {
        //If no split is specified, set min/max CU size to CTU size
        //so that the CTU quad-tree only contains single node after generation
        m_CTUStr.minLog2CUSize = CeilLog2(m_CTUStr.CTUSize);
        m_CTUStr.maxLog2CUSize = CeilLog2(m_CTUStr.CTUSize);
    }


    while (it_ctu != frameDescr.m_vCTUdescr.end())
    {
        auto &CTU = *it_ctu;

        //make a tree and save CUs into the vector inside CTU
        GenRandomCUQuadTreeInCTU(CTU);
        GenCUVecInCTU(CTU, frameDescr.m_testType);
        if (frameDescr.m_testType & GENERATE_INTRA)
        {
            GetRefSampleAvailFlagsForTUsInCTU(CTU);
        }

        FrameOccRefBlockRecord bak = frameDescr.BackupOccupiedRefBlocks();

        bool bMVGenSuccess = true;
        if (frameDescr.m_testType & GENERATE_INTER)
        {
            bMVGenSuccess = MakeInterPredInCTU(CTU, frameDescr);
        }
        if (bMVGenSuccess)
        {
            //Inter prediction must be applied first
            //because intra blocks should use noise pixels from
            //adjacent inter CUs and not unchanged picture pixels
            //in the same spot
            ApplyInterPredInCTU(CTU, frameDescr);

            //most contrast intra mode is chosen here
            MakeIntraPredInCTU(CTU, frameDescr);
            ApplyIntraPredInCTU(CTU, frameDescr);
            it_ctu++;
        }
        else
        {
            //Unable to put the CTU into reference frames without intersection
            //Restore backup reference block info:
            frameDescr.RestoreOccupiedRefBlocks(bak);

            //Remove current CTU from the test block list and updating the iterator
            it_ctu = frameDescr.m_vCTUdescr.erase(it_ctu);
        }
    }

    return;
}

//Generates MV and MVP for all PUs in CTU
bool FrameProcessor::MakeInterPredInCTU(CTUDescriptor& CTU, FrameChangeDescriptor& frameDescr)
{
    // First, need to construct an MVP grid and vector pools according to CTU partioning
    MVMVPProcessor mvmvpProcessor(m_GenMVPBlockSize, m_SubPelMode);

    if (frameDescr.m_testType & GENERATE_PREDICTION)
    {
        mvmvpProcessor.InitMVPGridData(CTU, frameDescr);
    }

    for (auto& CU : CTU.m_CUVec)
    {
        if (CU.m_PredType == INTER_PRED)
        {
            for (auto& PU : CU.m_PUVec)
            {
                // Set prediction flags for each PU
                GenPredFlagsForPU(PU, frameDescr.m_frameType);

                bool bMVGenSuccess = false;
                if (frameDescr.m_testType & GENERATE_PREDICTION)
                {
                    bMVGenSuccess = mvmvpProcessor.GenValidMVMVPForPU(PU, frameDescr);
                }
                else
                {
                    bMVGenSuccess = mvmvpProcessor.GenValidMVForPU(PU, frameDescr);
                }

                if (bMVGenSuccess)
                {
                    // Store the shifted PU as a BaseBlock in the corresponding reference frame descriptor
                    if (PU.predFlagL0)
                    {
                        auto itL0 = std::next(frameDescr.m_refDescrList0.begin(), PU.m_MV.RefIdx.RefL0);
                        itL0->m_OccupiedRefBlocks.emplace_back(PU.GetShiftedBaseBlock(L0));
                    }

                    if (PU.predFlagL1)
                    {
                        auto itL1 = std::next(frameDescr.m_refDescrList1.begin(), PU.m_MV.RefIdx.RefL1);
                        itL1->m_OccupiedRefBlocks.emplace_back(PU.GetShiftedBaseBlock(L1));
                    }
                }
                else
                {
                    //MV prediction has failed; need to discard the whole CTU
                    return false;
                }
            }
        }
    }

    //MVs for all PUs in CTU have been generated

    // If predictors required, put them to ext buffer
    if (frameDescr.m_testType & GENERATE_PREDICTION && frameDescr.m_procMode == GENERATE)
    {
        //Only output predictors if CTU contains at least one inter CU
        auto it = std::find_if(CTU.m_CUVec.begin(), CTU.m_CUVec.end(),
            [](const CUBlock& CU) { return CU.m_PredType == INTER_PRED; });

        if (it != CTU.m_CUVec.end())
        {
            mvmvpProcessor.FillFrameMVPExtBuffer(frameDescr);
        }
    }

    if ((frameDescr.m_testType & GENERATE_PREDICTION) && frameDescr.m_procMode == VERIFY)
    {
        mvmvpProcessor.GetMVPPools(CTU.m_MVPGenPools);
    }

    return true;
}


void FrameProcessor::GenPredFlagsForPU(PUBlock & PU, mfxU16 frameType)
{
    PU.predFlagL0 = true; // Set prediction flag for P-frames
    if (frameType & MFX_FRAMETYPE_B)
    {
        // Each PU requires 1 or more reference
        mfxI32 maxValue = 2;
        if (PU.m_BWidth == 4 || PU.m_BHeight == 4)
            maxValue = 1;  // For PUs 8x4 or 4x8 we can use only unidirectional prediction

        switch (GetRandomGen().GetRandomNumber(0, maxValue))
        {
        case 0:
            PU.predFlagL0 = true;
            PU.predFlagL1 = false;
            break;
        case 1:
            PU.predFlagL0 = false;
            PU.predFlagL1 = true;
            break;
        case 2:
            PU.predFlagL0 = true;
            PU.predFlagL1 = true;
            break;
        }
    }

    return;
}

//Iterates over CUs in CTU and applies inter prediction for inter CUs
void FrameProcessor::ApplyInterPredInCTU(CTUDescriptor& CTU, FrameChangeDescriptor & frameDescr)
{
    for (auto& CU : CTU.m_CUVec)
    {
        if (CU.m_PredType == INTER_PRED)
        {
            for (auto& PU : CU.m_PUVec)
            {
                // Advance to reference descriptor of desired frame
                // NB: using reverse iterator here because RefIdx starts counting from the end of the list

                // Generate noisy PU in GEN frames pixels
                // This function should be called in both (generate and verify) modes, because includes work with random generator
                PutNoiseBlocksIntoFrames(PU, frameDescr);

                // Trace back those pixels (using q-pel interpolation if required) and put to current MOD frame
                // This function should be called only in the generate mode
                if (m_ProcMode == GENERATE)
                {
                    TraceBackAndPutBlockIntoFrame(PU, frameDescr);
                }
            }
        }
    }
}

// Generate and put noisy pixels to surface surf, which corresponds to block BP coordinates

// Fill the block with 4x4 noise blocks
void FrameProcessor::PutNoiseBlocksIntoFrames(const PUBlock & PU, const FrameChangeDescriptor & frameDescr, mfxU32 num_coeff, mfxU32 level)
{
    BaseBlock BPL0, BPL1;

    mfxFrameSurface1* refSurfL0 = nullptr;
    mfxFrameSurface1* refSurfL1 = nullptr;

    if (PU.predFlagL0)
    {
        if (frameDescr.m_refDescrList0.size() <= PU.m_MV.RefIdx.RefL0)
        {
            throw std::string("ERROR: PutNoiseBlockIntoFrames: incorrect reference index for list 0");
        }
        auto &refDescrL0 = *next(frameDescr.m_refDescrList0.begin(), PU.m_MV.RefIdx.RefL0);

        // Get frame of reference GEN frame
        refSurfL0 = refDescrL0.m_frame;
        if (refSurfL0 == nullptr)
        {
            throw std::string("ERROR: PutNoiseBlockIntoFrames: null pointer reference");
        }

        BPL0 = PU.GetShiftedBaseBlock(L0);
    }

    if (PU.predFlagL1)
    {
        if (frameDescr.m_refDescrList1.size() <= PU.m_MV.RefIdx.RefL1)
        {
            throw std::string("ERROR: PutNoiseBlockIntoFrames: incorrect reference index for list 1");
        }
        auto &refDescrL1 = *next(frameDescr.m_refDescrList1.begin(), PU.m_MV.RefIdx.RefL1);

        // Get frame of reference GEN frame
        refSurfL1 = refDescrL1.m_frame;
        if (refSurfL1 == nullptr)
        {
            throw std::string("ERROR: PutNoiseBlockIntoFrames: null pointer reference");
        }

        BPL1 = PU.GetShiftedBaseBlock(L1);
    }

    //Check that PU can be subdivided evenly into 4x4 blocks
    if (PU.m_BWidth % 4 || PU.m_BHeight % 4)
    {
        throw std::string("ERROR: PutNoiseBlockIntoFrame: invalid block size");
    }
    mfxU32 Block4x4NumX = PU.m_AdrX / 4;
    mfxU32 Block4x4NumY = PU.m_AdrY / 4;
    mfxU32 FrameWidthIn4x4Blocks = (m_Width + 3) / 4;
    mfxU32 FrameHeightIn4x4Blocks = (m_Height + 3) / 4;

    mfxU32 seedOff = frameDescr.m_frame->Data.FrameOrder * FrameWidthIn4x4Blocks * FrameHeightIn4x4Blocks;
    mfxU32 seed = 0;

    mfxU32 BPWidthIn4x4Blocks = PU.m_BWidth / 4;
    mfxU32 BPHeightIn4x4Blocks = PU.m_BHeight / 4;
    mfxU8 block[16] = {}, blockAdjusted[16] = {};
    mfxI8 blockDeltaL0[16] = {}, blockDeltaL1[16] = {};

    for (mfxU32 i = 0; i < BPHeightIn4x4Blocks; i++)
    {
        for (mfxU32 j = 0; j < BPWidthIn4x4Blocks; j++)
        {
            //Calculate seed from the 4x4 block position inside the PU
            seed = seedOff + FrameWidthIn4x4Blocks * (Block4x4NumY + i) + Block4x4NumX + j;
            GetRandomGen().SeedGenerator(seed);

            //Calculate noise pixel values
            FillInBlock4x4(num_coeff, level, block);

            //Calculate delta for difference between L0 and L1 references
            if (PU.predFlagL0 && PU.predFlagL1)
                FillDeltaBlocks4x4(blockDeltaL0, blockDeltaL1);

            if (m_ProcMode == GENERATE)
            {
                if (PU.predFlagL0)
                {
                    ApplyDeltaPerPixel(PU, blockDeltaL0, block, blockAdjusted);
                    PutBlock4x4(BPL0.m_AdrX + j * 4, BPL0.m_AdrY + i * 4, blockAdjusted, refSurfL0);
                }

                if (PU.predFlagL1)
                {
                    ApplyDeltaPerPixel(PU, blockDeltaL1, block, blockAdjusted);
                    PutBlock4x4(BPL1.m_AdrX + j * 4, BPL1.m_AdrY + i * 4, blockAdjusted, refSurfL1);
                }
            }
        }
    }
}

// Fill block[16] with noise pixels, using up to num_coeff first random DCT coefficients
// in the range of (-level; +level)
void FrameProcessor::FillInBlock4x4(mfxU32 num_coeff, mfxU32 level, mfxU8 block[16])
{
    if (block == nullptr)
    {
        throw std::string("ERROR: FillInBlock4x4: null pointer reference");
    }

    if (num_coeff < 1 || num_coeff > 16)
    {
        throw std::string("\nERROR: Wrong num_coeff in FrameProcessor::FillInBlock4x4");
    }
    if (level > 255)
    {
        throw std::string("\nERROR: Wrong level in FrameProcessor::FillInBlock4x4");
    }

    mfxI32 coeff[16];
    mfxI32 pixels[16];
    mfxU32 scan[16] = { 0,1,4,8,5,2,3,6,9,12,13,10,7,11,14,15 };
    memset(coeff, 0, sizeof(coeff));
    for (mfxU32 i = 0; i < num_coeff; i++)
    {
        coeff[scan[i]] = level* GetRandomGen().GetRandomNumber(0, 256) / 256 - level / 2;
    }
    if (m_ProcMode == GENERATE)
    {
        Inverse4x4(coeff, 4, pixels, 4);
        for (mfxU32 i = 0; i < 16; i++)
        {
            block[i] = ClipIntToChar(128 + pixels[i]);
        }
    }

    return;
}

void FrameProcessor::FillDeltaBlocks4x4(mfxI8 blockL0[16], mfxI8 blockL1[16])
{
    if (blockL0 == nullptr || blockL1 == nullptr)
    {
        throw std::string("ERROR: FillDeltaBlocks4x4: null pointer reference");
    }

    for (mfxU32 i = 0; i < 16; i++)
    {
        blockL0[i] = GetRandomGen().GetRandomNumber(0, DELTA_PIXEL_BI_DIRECT);
        blockL1[i] = -blockL0[i];
    }


    return;
}

void FrameProcessor::ApplyDeltaPerPixel(const PUBlock & PU, const mfxI8 deltaBlock[16], const mfxU8 inBlock[16], mfxU8 outBlock[16])
{
    if (inBlock == nullptr || outBlock == nullptr)
    {
        throw std::string("ERROR: ApplyDeltaPerPixel: null pointer reference");
    }

    for (mfxU8 i = 0; i < 16; i++)
    {
        outBlock[i] = ClipIntToChar(inBlock[i] + deltaBlock[i]);
    }

    return;
}

mfxU8 FrameProcessor::ClipIntToChar(mfxI32 x)
{
    if (x < 0)
        return 0;
    else if (x > 255)
        return 255;

    return (mfxU8)x;
}

//Perform inverse 4x4 DCT
void FrameProcessor::Inverse4x4(mfxI32 *src, mfxU32 s_pitch, mfxI32 *dst, mfxU32 d_pitch)
{
    if (src == nullptr || dst == nullptr)
    {
        throw std::string("ERROR: Inverse4x4: null pointer reference");
    }
    const mfxU32 BLOCK_SIZE = 4;
    mfxI32 tmp[16];
    mfxI32 *pTmp = tmp, *pblock;
    mfxI32 p0, p1, p2, p3;
    mfxI32 t0, t1, t2, t3;

    // Horizontal
    for (mfxU32 i = 0; i < BLOCK_SIZE; i++)
    {
        pblock = src + i*s_pitch;
        t0 = *(pblock++);
        t1 = *(pblock++);
        t2 = *(pblock++);
        t3 = *(pblock);

        p0 = t0 + t2;
        p1 = t0 - t2;
        p2 = (t1 >> 1) - t3;
        p3 = t1 + (t3 >> 1);

        *(pTmp++) = p0 + p3;
        *(pTmp++) = p1 + p2;
        *(pTmp++) = p1 - p2;
        *(pTmp++) = p0 - p3;
    }

    //  Vertical
    for (mfxU32 i = 0; i < BLOCK_SIZE; i++)
    {
        pTmp = tmp + i;
        t0 = *pTmp;
        t1 = *(pTmp += BLOCK_SIZE);
        t2 = *(pTmp += BLOCK_SIZE);
        t3 = *(pTmp += BLOCK_SIZE);

        p0 = t0 + t2;
        p1 = t0 - t2;
        p2 = (t1 >> 1) - t3;
        p3 = t1 + (t3 >> 1);

        *(dst + 0 * d_pitch + i) = p0 + p3;
        *(dst + 1 * d_pitch + i) = p1 + p2;
        *(dst + 2 * d_pitch + i) = p1 - p2;
        *(dst + 3 * d_pitch + i) = p0 - p3;
    }
}

void FrameProcessor::PutBlock4x4(mfxU32 x0, mfxU32 y0, mfxU8 block[16], mfxFrameSurface1* surf)
{
    if (surf == nullptr)
    {
        throw std::string("ERROR: PutBlock4x4: null pointer reference");
    }
    //put block in the current frame, x0, y0 pixel coordinates
    for (mfxU32 y = 0; y < 4; y++)
    {
        for (mfxU32 x = 0; x < 4; x++)
        {
            *(surf->Data.Y + (y0 + y)* surf->Data.Pitch  + x0 + x) = block[4 * y + x];
            *(surf->Data.U + ((y0 + y) / 2) * (surf->Data.Pitch / 2) + (x0 + x) / 2) = CHROMA_DEFAULT;
            *(surf->Data.V + ((y0 + y) / 2) * (surf->Data.Pitch / 2) + (x0 + x) / 2) = CHROMA_DEFAULT;
        }
    }
    return;
}


// Trace back pixels from block BP shifted by MV coordinates on GEN frame surf_from to MOD frame surf_to
//
// bp - block on MOD frame
// mv - it's shift on GEN frame
// surf_from - GEN frame surf
// surf_to - MOD frame surf

void FrameProcessor::TraceBackAndPutBlockIntoFrame(const PUBlock & PU, FrameChangeDescriptor & descr)
{
    std::pair<mfxU32, mfxU32> fractOffsetL0(PU.m_MV.MV[0].x & 3, PU.m_MV.MV[0].y & 3);
    std::pair<mfxU32, mfxU32> fractOffsetL1(PU.m_MV.MV[1].x & 3, PU.m_MV.MV[1].y & 3);

    auto itL0 = std::next(descr.m_refDescrList0.begin(), PU.m_MV.RefIdx.RefL0);
    auto itL1 = std::next(descr.m_refDescrList1.begin(), PU.m_MV.RefIdx.RefL1);

    mfxFrameSurface1* surfDest = nullptr;
    mfxFrameSurface1* surfL0Ref = nullptr;
    mfxFrameSurface1* surfL1Ref = nullptr;

    surfDest = descr.m_frame;

    if (itL0 != descr.m_refDescrList0.end())
    {
        surfL0Ref = itL0->m_frame;
    }
    else
    {
        throw("ERROR: TraceBackAndPutBlockIntoFrame: L0 ref not found");
    }

    if (descr.m_frameType & MFX_FRAMETYPE_B)
    {
        if (itL1 != descr.m_refDescrList1.end())
        {
            surfL1Ref = itL1->m_frame;
        }
        else
        {
            throw("ERROR: TraceBackAndPutBlockIntoFrame: L1 ref not found");
        }
    }

    PatchBlock outPatch(PU);
    InterpolWorkBlock workBlockL0;
    InterpolWorkBlock workBlockL1;

    if (PU.predFlagL0 && PU.predFlagL1)
    {
        workBlockL0 = GetInterpolWorkBlockPreWP(PU.GetShiftedBaseBlock(L0), fractOffsetL0, surfL0Ref);
        workBlockL1 = GetInterpolWorkBlockPreWP(PU.GetShiftedBaseBlock(L1), fractOffsetL1, surfL1Ref);
        outPatch = ApplyDefaultWeightedPrediction(workBlockL0, workBlockL1);
    }
    else if (PU.predFlagL0)
    {
        workBlockL0 = GetInterpolWorkBlockPreWP(PU.GetShiftedBaseBlock(L0), fractOffsetL0, surfL0Ref);
        outPatch = ApplyDefaultWeightedPrediction(workBlockL0);
    }
    else if (PU.predFlagL1)
    {
        workBlockL1 = GetInterpolWorkBlockPreWP(PU.GetShiftedBaseBlock(L1), fractOffsetL1, surfL1Ref);
        outPatch = ApplyDefaultWeightedPrediction(workBlockL1);
    }
    else
        throw("ERROR: TraceBackAndPutBlockIntoFrame: predFlagL0 and predFlagL1 are equal 0");

    //Adjust outPatch coords so that they correspond to the unshifted PU
    outPatch.m_AdrX = PU.m_AdrX;
    outPatch.m_AdrY = PU.m_AdrY;

    PutPatchIntoFrame(outPatch, *surfDest);

    return;
}


InterpolWorkBlock FrameProcessor::GetInterpolWorkBlockPreWP(const BaseBlock & blockFrom, std::pair<mfxU32, mfxU32> fractOffset, mfxFrameSurface1* surfFrom)
{
    if (surfFrom == nullptr)
    {
        throw std::string("ERROR: GetInterpolWorkBlockPreWP: null pointer reference");
    }
    InterpolWorkBlock workBlock(blockFrom);

    //Luma
    for (mfxU32 i = 0; i < blockFrom.m_BHeight; i++)
    {
        for (mfxU32 j = 0; j < blockFrom.m_BWidth; j++)
        {
            mfxU32 offset = i * blockFrom.m_BWidth + j;
            workBlock.m_YArr[offset] = CalculateLumaPredictionSamplePreWP(
                std::make_pair(blockFrom.m_AdrX + j, blockFrom.m_AdrY + i), fractOffset, surfFrom);
        }
    }

    // Chroma(YV12 / I420 only) - TODO: enable correct interpolation for chroma
    //NB: MFX_FOURCC_YV12 is an umbrella designation for both YV12 and I420 here, as
    //the process of copying pixel values in memory is the same

    //TODO: implement proper chroma interpolation
    if (surfFrom->Info.FourCC == MFX_FOURCC_YV12)
    {
        for (mfxU32 i = 0; i < blockFrom.m_BHeight / 2; ++i)
        {
            for (mfxU32 j = 0; j < blockFrom.m_BWidth / 2; j++)
            {
                mfxU32 offsetChr = i * blockFrom.m_BWidth / 2 + j;
                workBlock.m_UArr[offsetChr] = surfFrom->Data.U[
                    (blockFrom.m_AdrY / 2 + i) * surfFrom->Data.Pitch / 2 + blockFrom.m_AdrX / 2 + j];
                workBlock.m_VArr[offsetChr] = surfFrom->Data.V[
                    (blockFrom.m_AdrY / 2 + i) * surfFrom->Data.Pitch / 2 + blockFrom.m_AdrX / 2 + j];

                //For now, just scale uninterpolated chroma so that we can call default weighted prediction
                //on chroma components the same way we do with luma
                workBlock.m_UArr[offsetChr] <<= 6;
                workBlock.m_VArr[offsetChr] <<= 6;
            }
        }
    }

    return workBlock;
}

PatchBlock FrameProcessor::ApplyDefaultWeightedPrediction(InterpolWorkBlock & workBlockLx)
{
    PatchBlock outPatch(static_cast<BaseBlock>(workBlockLx));

    //Luma
    for (mfxU32 i = 0; i < outPatch.m_BHeight * outPatch.m_BWidth; i++)
    {
        outPatch.m_YPlane[i] = GetDefaultWeightedPredSample(workBlockLx.m_YArr[i]);
    }

    //Chroma
    for (mfxU32 i = 0; i < outPatch.m_BHeight / 2 * outPatch.m_BWidth / 2; i++)
    {
        outPatch.m_UPlane[i] = GetDefaultWeightedPredSample(workBlockLx.m_UArr[i]);
        outPatch.m_VPlane[i] = GetDefaultWeightedPredSample(workBlockLx.m_VArr[i]);
    }
    return outPatch;
}

PatchBlock FrameProcessor::ApplyDefaultWeightedPrediction(InterpolWorkBlock & workBlockL0, InterpolWorkBlock & workBlockL1)
{
    if (workBlockL0.m_BHeight != workBlockL1.m_BHeight || workBlockL0.m_BWidth != workBlockL1.m_BWidth)
    {
        throw std::string("ERROR: ApplyDefaultWeightedPrediction: InterpolWorkBlocks for bi-prediction must have same size");
    }
    PatchBlock outPatch(static_cast<BaseBlock>(workBlockL0));
    for (mfxU32 i = 0; i < outPatch.m_BHeight * outPatch.m_BWidth; i++)
    {
        outPatch.m_YPlane[i] = GetDefaultWeightedPredSample(workBlockL0.m_YArr[i], workBlockL1.m_YArr[i]);
    }

    for (mfxU32 i = 0; i < outPatch.m_BHeight / 2 * outPatch.m_BWidth / 2; i++)
    {
        outPatch.m_UPlane[i] = GetDefaultWeightedPredSample(workBlockL0.m_UArr[i], workBlockL1.m_UArr[i]);
        outPatch.m_VPlane[i] = GetDefaultWeightedPredSample(workBlockL0.m_VArr[i], workBlockL1.m_VArr[i]);
     }

    return outPatch;
}

mfxU8 FrameProcessor::SetCorrectMVPBlockSize(mfxU8 mvpBlockSizeParam)
{
    if (!mvpBlockSizeParam)
    {
        switch (m_CTUStr.CTUSize)
        {
        case 16:
            return 1;
        case 32:
            return 2;
        case 64:
            return 3;
        default:
            break;
        }
    }
    return mvpBlockSizeParam;
}

// Returns predicted luma value (Y) for sample with provided location on reference frame given in quarter-pixel units
//
// refSamplePositionFull - (xInt,yInt) Luma location on the reference frame given in full-sample units. Assumed (x,y) has correct value.
// refSamplePositionFract - (xFract,yFract) Luma location on the reference frame given in quarter-sample units.
// refSurface - reference frame, containing luma samples
// Luma interpolation process described in H265 standard (p.163 - 165)

mfxI32 FrameProcessor::CalculateLumaPredictionSamplePreWP(const std::pair<mfxU32, mfxU32>& refSamplePositionFull,
    const std::pair<mfxU32, mfxU32>& refSamplePositionFract, mfxFrameSurface1 * refSurface)
{
    mfxU32 xFull = refSamplePositionFull.first;
    mfxU32 yFull = refSamplePositionFull.second;
    mfxU32 xFract = refSamplePositionFract.first;
    mfxU32 yFract = refSamplePositionFract.second;

    // These shift variables used below are specified in H265 spec for 8 bit Luma depth
    // shift1 := 0
    // shift2 := 6
    // shift3 := 6

    // Stores output of the sub-sample filtering process
    mfxI32 interpolatedSample = 0;

    /*
    // Integer and quarter sample positions used for interpolation

    A-10 O O O | A00 a00 b00 c00 | A10 O O O A20
    d-10 O O O | d00 e00 f00 g00 | d10 O O O d20
    h-10 O O O | h00 i00 j00 k00 | h10 O O O h20
    n-10 O O O | n00 p00 q00 r00 | n10 O O O n20
    */

    switch (xFract)
    {
    case 0:
        switch (yFract)
        {
        case 0:
            // A << shift3
            interpolatedSample = ApplyVerticalSubSampleLumaFilter(xFull, yFull,
                refSurface, LUMA_SUBSAMPLE_FILTER_COEFF[0]) * 64;
            break;
        case 1:
            // d00 := (-A(0,-3) + 4*A(0,-2) - 10*A(0,-1) + 58*A(0,0) + 17*A(0,1) - 5*A(0,2) + A(0,3)) >> shift1
            interpolatedSample = ApplyVerticalSubSampleLumaFilter(xFull, yFull,
                refSurface, LUMA_SUBSAMPLE_FILTER_COEFF[1]);
            break;
        case 2:
            // h00 := (-A(0,-3) + 4*A(0,-2) - 11*A(0,-1) + 40*A(0,0) + 40*A(0,1) - 11*A(0,2) + 4*A(0,3) - A(0,4)) >> shift1
            interpolatedSample = ApplyVerticalSubSampleLumaFilter(xFull, yFull,
                refSurface, LUMA_SUBSAMPLE_FILTER_COEFF[2]);
            break;
        case 3:
            // n00 := (A(0,-2) - 5*A(0,-1) + 17*A(0,0) + 58*A(0,1) - 10*A(0,2) + 4*A(0,3) - A(0,4)) >> shift1
            interpolatedSample = ApplyVerticalSubSampleLumaFilter(xFull, yFull,
                refSurface, LUMA_SUBSAMPLE_FILTER_COEFF[3]);
            break;
        default:
            break;
        }
        break;
    case 1:
    {
        // a0i, where i = -3..4
        // a0i = [a(0,-3) a(0,-2) a(0,-1) a(0,0) a(0,1) a(0,2) a(0,3) a(0,4)]
        std::vector<mfxI32> fractUtilSamples;
        fractUtilSamples.reserve(LUMA_TAPS_NUMBER);

        for (mfxI32 i = 0; i < LUMA_TAPS_NUMBER; i++)
        {
            // a0i := (-A(-3,i) + 4*A(-2,i) - 10*A(-1,i) + 58*A(0,i) + 17*A(1,i) - 5*A(2,i) + A(3,i) >> shift1)
            fractUtilSamples.push_back(ApplyHorizontalSubSampleLumaFilter(xFull,
                yFull + (i - 3), refSurface, LUMA_SUBSAMPLE_FILTER_COEFF[1]));
        }

        switch (yFract)
        {
        case 0:
            // a00 := a(0,0)
            interpolatedSample = std::inner_product(fractUtilSamples.begin(), fractUtilSamples.end(),
                LUMA_SUBSAMPLE_FILTER_COEFF[0], 0);
            break;
        case 1:
            // e00 := (-a(0,-3) + 4*a(0,-2) - 10*a(0,-1) + 58*a(0,0) + 17*a(0,1) - 5*a(0,2) + a(0,3) >> shift2)
            interpolatedSample = std::inner_product(fractUtilSamples.begin(),
                fractUtilSamples.end(), LUMA_SUBSAMPLE_FILTER_COEFF[1], 0) / 64;
            break;
        case 2:
            // i00 := (-a(0,-3) + 4*a(0,-2) - 11*a(0,-1) + 40*a(0,0) + 40*a(0,1) - 11*a(0,2) + 4*a(0,3) - a(0,4) >> shift2)
            interpolatedSample = std::inner_product(fractUtilSamples.begin(),
                fractUtilSamples.end(), LUMA_SUBSAMPLE_FILTER_COEFF[2], 0) / 64;
            break;
        case 3:
            // p00 := (a(0,-2) - 5*a(0,-1) + 17*a(0,0) + 58*a(0,1) - 10*a(0,2) + 4*a(0,3) - a(0,4) >> shift2)
            interpolatedSample = std::inner_product(fractUtilSamples.begin(), fractUtilSamples.end(),
                LUMA_SUBSAMPLE_FILTER_COEFF[3], 0) / 64;
            break;
        default:
            break;
        }
        break;
    }
    case 2:
    {
        // b0i, where i = -3..4
        // b0i = [b(0,-3) b(0,-2) b(0,-1) b(0,0) b(0,1) b(0,2) b(0,3) b(0,4)]
        std::vector<mfxI32> fractUtilSamples;
        fractUtilSamples.reserve(LUMA_TAPS_NUMBER);

        for (mfxI32 i = 0; i < LUMA_TAPS_NUMBER; i++)
        {
            // b0i := (-A(-3,i) + 4*A(-2,i) - 11*A(-1,i) + 40*A(0,i) + 40*A(1,i) - 11*A(2,i) + 4*A(3,i) - A(4,i) >> shift1)
            fractUtilSamples.push_back(ApplyHorizontalSubSampleLumaFilter(xFull, yFull + (i - 3),
                refSurface, LUMA_SUBSAMPLE_FILTER_COEFF[2]));
        }

        switch (yFract)
        {
        case 0:
            // b00 := (-A(-3,0) + 4*A(-2,0) - 11*A(-1,0) + 40*A(0,0) + 40*A(1,0) - 11*A(2,0) + 4*A(3,0) - A(4,0) >> shift1)
            interpolatedSample = std::inner_product(fractUtilSamples.begin(), fractUtilSamples.end(),
                LUMA_SUBSAMPLE_FILTER_COEFF[0], 0);
            break;
        case 1:
            // f00 := (-b(0,-3) + 4*b(0,-2) - 10*b(0,-1) + 58*b(0,0) + 17*b(0,1) - 5*b(0,2) + b(0,3) >> shift2)
            interpolatedSample = std::inner_product(fractUtilSamples.begin(), fractUtilSamples.end(),
                LUMA_SUBSAMPLE_FILTER_COEFF[1], 0) / 64;
            break;
        case 2:
            // j00 := (-b(0,-3) + 4*b(0,-2) - 11*b(0,-1) + 40*b(0,0) + 40*b(0,1) - 11*b(0,2) + 4*b(0,3) - b(0,4) >> shift2)
            interpolatedSample = std::inner_product(fractUtilSamples.begin(), fractUtilSamples.end(),
                LUMA_SUBSAMPLE_FILTER_COEFF[2], 0) / 64;
            break;
        case 3:
            // q00 := (b(0,-2) - 5*b(0,-1) + 17*b(0,0) + 58*b(0,1) - 10*b(0,2) + 4*b(0,3) - b(0,4) >> shift2)
            interpolatedSample = std::inner_product(fractUtilSamples.begin(), fractUtilSamples.end(),
                LUMA_SUBSAMPLE_FILTER_COEFF[3], 0) / 64;
            break;
        default:
            break;
        }
        break;
    }
    case 3:
    {
        // c0i, where i = -3..4
        // c0i = [c(0,-3) c(0,-2) c(0,-1) c(0,0) c(0,1) c(0,2) c(0,3) c(0,4)]
        std::vector<mfxI32> fractUtilSamples;
        fractUtilSamples.reserve(LUMA_TAPS_NUMBER);

        for (mfxI32 i = 0; i < LUMA_TAPS_NUMBER; i++)
        {
            // c0i := (A(-2,i) - 5*A(-1,i) + 17*A(0,i) + 58*A(1,i) - 10*A(2,i) + 4*A(3,i) - A(4,i) >> shift1)
            fractUtilSamples.push_back(ApplyHorizontalSubSampleLumaFilter(xFull, yFull + (i - 3),
                refSurface, LUMA_SUBSAMPLE_FILTER_COEFF[3]));
        }

        switch (yFract)
        {
        case 0:
            // c00 := c(0,0)
            interpolatedSample = std::inner_product(fractUtilSamples.begin(), fractUtilSamples.end(),
                LUMA_SUBSAMPLE_FILTER_COEFF[0], 0);
            break;
        case 1:
            // g00 := (-c(0,-3) + 4*c(0,-2) - 10*c(0,-1) + 58*c(0,0) + 17*c(0,1) - 5*c(0,2) + c(0,3) >> shift2)
            interpolatedSample = std::inner_product(fractUtilSamples.begin(), fractUtilSamples.end(),
                LUMA_SUBSAMPLE_FILTER_COEFF[1], 0) / 64;
            break;
        case 2:
            // k00 := (-c(0,-3) + 4*c(0,-2) - 11*c(0,-1) + 40*c(0,0) + 40*c(0,1) - 11*c(0,2) + 4*c(0,3) - c(0,4) >> shift2)
            interpolatedSample = std::inner_product(fractUtilSamples.begin(), fractUtilSamples.end(),
                LUMA_SUBSAMPLE_FILTER_COEFF[2], 0) / 64;
            break;
        case 3:
            // r00 := (c(0,-2) - 5*c(0,-1) + 17*c(0,0) + 58*c(0,1) - 10*c(0,2) + 4*c(0,3) - c(0,4) >> shift2)
            interpolatedSample = std::inner_product(fractUtilSamples.begin(), fractUtilSamples.end(),
                LUMA_SUBSAMPLE_FILTER_COEFF[3], 0) / 64;
            break;
        default:
            break;
        }
        break;
    }
    default:
        break;
    }

    return interpolatedSample;
}

mfxI32 FrameProcessor::ApplyVerticalSubSampleLumaFilter(mfxU32 x, mfxU32 y, mfxFrameSurface1 * refSurface, const mfxI32 * coeff)
{
    mfxI32 sum = 0;

    for (mfxI32 i = -3; i <= 4; i++)
    {
        sum += coeff[i + 3] * GetClippedSample(COLOR_COMPONENT::LUMA_Y, (mfxI32)x, (mfxI32)y + i, refSurface);
    }

    return sum;
}

mfxI32 FrameProcessor::ApplyHorizontalSubSampleLumaFilter(mfxU32 x, mfxU32 y, mfxFrameSurface1 * refSurface, const mfxI32 * coeff)
{
    mfxI32 sum = 0;

    for (mfxI32 i = -3; i <= 4; i++)
    {
        sum += coeff[i + 3] * GetClippedSample(COLOR_COMPONENT::LUMA_Y, (mfxI32)x + i, (mfxI32)y, refSurface);
    }

    return sum;
}

// Release processed surfaces

void FrameProcessor::UnlockSurfaces(FrameChangeDescriptor & frame_descr)
{
    frame_descr.m_frame->Data.Locked = 0;

    // Unlock all reference GEN frames recursively
    if (frame_descr.m_refDescrList0.empty() && frame_descr.m_refDescrList1.empty()) return;
    else
    {
        for (auto & ref_descr : frame_descr.m_refDescrList0)
        {
            UnlockSurfaces(ref_descr);
        }
        for (auto & ref_descr : frame_descr.m_refDescrList1)
        {
            UnlockSurfaces(ref_descr);
        }
    }

    return;
}

//this function has the same behavior for any color component
//it is more convenient to have input in the coordinates of colorComp component space
void FrameProcessor::FillIntraRefSamples(mfxU32 cSize, mfxU32 cAdrX, mfxU32 cAdrY, const PatchBlock& frame, COLOR_COMPONENT colorComp, std::vector<mfxU8>& refSamples)
{
    refSamples.clear();
    const mfxU32 NO_SAMPLES_AVAILABLE = 0xffffffff;
    mfxU8 prevSampleAvail = 128; //default ref sample value is 128 if no real ref samples are available
    mfxU32 firstSampleAvailPos = NO_SAMPLES_AVAILABLE; //position of the first available ref sample in refSamples

    //fill vertical part
    mfxU32 currCAdrX = cAdrX - 1;
    mfxU32 currCAdrY = cAdrY + 2 * cSize - 1;

    for (mfxU32 i = 0; i < 2 * cSize + 1; i++, currCAdrY--)
    {
        if (IsSampleAvailable(currCAdrX, currCAdrY))
        {
            prevSampleAvail = frame.GetSampleI420(colorComp, currCAdrX, currCAdrY);
            refSamples.push_back(prevSampleAvail);
            if (firstSampleAvailPos == NO_SAMPLES_AVAILABLE)
            {
                firstSampleAvailPos = i;
            }
        }
        else
        {
            refSamples.push_back(prevSampleAvail);
        }
    }
    currCAdrX = cAdrX;
    currCAdrY = cAdrY - 1;

    //fill horizontal part
    for (mfxU32 i = 2 * cSize + 1; i < 4 * cSize + 1; i++, currCAdrX++)
    {
        if (IsSampleAvailable(currCAdrX, currCAdrY))
        {
            prevSampleAvail = frame.GetSampleI420(colorComp, currCAdrX, currCAdrY);
            refSamples.push_back(prevSampleAvail);
            if (firstSampleAvailPos == NO_SAMPLES_AVAILABLE)
            {
                firstSampleAvailPos = i;
            }
        }
        else
        {
            refSamples.push_back(prevSampleAvail);
        }
    }
    //fill initial part with with first available ref sample value
    if (firstSampleAvailPos != NO_SAMPLES_AVAILABLE)
    {
        std::fill(refSamples.begin(), refSamples.begin() + firstSampleAvailPos, refSamples[firstSampleAvailPos]);
    }
}

FILTER_TYPE FrameProcessor::ChooseFilter(std::vector<mfxU8>& RefSamples, mfxU8 size, INTRA_MODE mode) {
    FILTER_TYPE filter = NO_FILTER;
    if (mode == DC || size == 4)
        return filter;
    switch (size) {
    case 8:
        if (mode == 2 || mode == 18 || mode == 34)
            filter = THREE_TAP_FILTER;
        break;
    case 16:
        filter = THREE_TAP_FILTER;
        if ((mode > 8 && mode < 12) || (mode > 24 && mode < 28))
            filter = NO_FILTER;
        break;
    case 32:
        filter = THREE_TAP_FILTER;
        if (mode == ANG10_HOR || mode == ANG26_VER)
            filter = NO_FILTER;
        else if (std::abs(RefSamples[0] + RefSamples[2 * size] - 2 * RefSamples[size]) < 8 &&
            std::abs(RefSamples[2 * size] + RefSamples[4 * size] - 2 * RefSamples[3 * size]) < 8)
            filter = STRONG_INTRA_SMOOTHING_FILTER;
        break;
    default:
        break;
    }
    return filter;
}

void FrameProcessor::ThreeTapFilter(std::vector<mfxU8>& RefSamples, mfxU8 size) {
    for (mfxU8 i = 1; i < (size << 2); i++)
        RefSamples[i] = (RefSamples[i - 1] + 2 * RefSamples[i] + RefSamples[i + 1] + 2) >> 2;
}

void FrameProcessor::StrongFilter(std::vector<mfxU8>& RefSamples, mfxU8 size) {
    for (mfxU8 i = 1; i < 2 * size; i++)
        RefSamples[i] = (i * RefSamples[2 * size] + (2 * size - i) * RefSamples[0] + 32) >> 6;
    for (mfxU8 i = 1; i < 2 * size; i++)
        RefSamples[2 * size + i] = ((2 * size - i) * RefSamples[2 * size] + i * RefSamples[4 * size] + 32) >> 6;
}

FILTER_TYPE FrameProcessor::MakeFilter(std::vector<mfxU8>& RefSamples, mfxU8 size, INTRA_MODE mode) {
    FILTER_TYPE filter = ChooseFilter(RefSamples, size, mode);
    switch (filter) {
    case NO_FILTER:
        break;
    case THREE_TAP_FILTER:
        ThreeTapFilter(RefSamples, size);
        break;
    case STRONG_INTRA_SMOOTHING_FILTER:
        StrongFilter(RefSamples, size);
        break;
    default:
        break;
    }
    return filter;
}

mfxU8 FrameProcessor::MakeProjRefArray(const std::vector<mfxU8>& RefSamples, mfxU8 size, const IntraParams& IntraMode, std::vector<mfxU8>& ProjRefSamples)
{
    mfxU8 NumProj = 0;

    if (IntraMode.direction == HORIZONTAL)
    {
        ProjRefSamples.insert(ProjRefSamples.end(), RefSamples.begin(), RefSamples.begin() + 2 * size + 1);
        if (IntraMode.intraPredAngle < 0)
        {
            if (IntraMode.invAngle == 0)
            {
                throw std::string("ERROR: MakeProjRefArray: invAngle == 0 for angular mode with intraPredAngle < 0");
            }
            mfxI8 y = -1;
            mfxI32 sampleForProjectionPos = 2 * size + ((y * IntraMode.invAngle + 128) >> 8);
            while (sampleForProjectionPos < 4 * size + 1)
            {
                ProjRefSamples.push_back(RefSamples[sampleForProjectionPos]);
                sampleForProjectionPos = 2 * size + ((--y * IntraMode.invAngle + 128) >> 8);
            }
        }
        std::reverse(ProjRefSamples.begin(), ProjRefSamples.end());
        NumProj = (mfxU8)(ProjRefSamples.size() - 2 * size - 1);
    }
    else if (IntraMode.direction == VERTICAL)
    {
        if (IntraMode.intraPredAngle < 0)
        {
            if (IntraMode.invAngle == 0)
            {
                throw std::string("ERROR: MakeProjRefArray: invAngle == 0 for angular mode with intraPredAngle < 0");
            }
            mfxI8 x = -1;
            mfxI32 sampleForProjectionPos = 2 * size - ((x * IntraMode.invAngle + 128) >> 8);

            while (sampleForProjectionPos > -1)
            {
                ProjRefSamples.push_back(RefSamples[sampleForProjectionPos]);
                sampleForProjectionPos = 2 * size - ((--x * IntraMode.invAngle + 128) >> 8);
            }

            std::reverse(ProjRefSamples.begin(), ProjRefSamples.end());
        }

        NumProj = (mfxU8)ProjRefSamples.size();
        ProjRefSamples.insert(ProjRefSamples.end(), RefSamples.begin() + 2 * size, RefSamples.end());
    }
    return NumProj;
}

void FrameProcessor::PlanarPrediction(const std::vector<mfxU8>& RefSamples, mfxU8 size, mfxU8 * patch)
{
    if (patch == nullptr)
    {
        throw std::string("ERROR: PlanarPrediction: pointer to buffer is null\n");
    }

    for (mfxI32 y = 0; y < size; y++)
    {
        for (mfxI32 x = 0; x < size; x++)
        {
            patch[y * size + x] = (
                (size - 1 - x) * RefSamples[2 * size - 1 - y]
                + (x + 1) * RefSamples[3 * size + 1]
                + (size - 1 - y) * RefSamples[2 * size + 1 + x]
                + (y + 1) * RefSamples[size - 1]
                + size) / (size * 2);
        }
    }

}

void FrameProcessor::DCPrediction(const std::vector<mfxU8>& RefSamples, mfxU8 size, mfxU8 * patch)
{
    if (patch == nullptr)
    {
        throw std::string("ERROR: DCPrediction: pointer to buffer is null\n");
    }

    mfxU32 DCValue = size;
    for (mfxI32 i = 0; i < size; i++)
    {
        DCValue += RefSamples[2 * size - 1 - i] + RefSamples[i + 2 * size + 1];
    }
    DCValue /= 2 * size;
    memset(patch, DCValue, size*size);
}

void FrameProcessor::AngularPrediction(const std::vector<mfxU8>& RefSamples, mfxU8 size, IntraParams& params, mfxU8 * patch) {
    if (patch == nullptr)
    {
        throw std::string("ERROR: AngularPrediction: pointer to buffer is null\n");
    }

    std::vector<mfxU8> ProjRefSamples;
    mfxU8 NumProj = MakeProjRefArray(RefSamples, size, params, ProjRefSamples);
    if (params.direction == HORIZONTAL)
        for (mfxI32 y = 0; y < size; y++)
            for (mfxI32 x = 0; x < size; x++) {
                mfxI32 f = ((x + 1) * params.intraPredAngle) & 31;
                mfxI32 i = ((x + 1) * params.intraPredAngle) >> 5;
                if (f != 0)
                    patch[y * size + x] = ((32 - f) * ProjRefSamples[y + i + 1 + NumProj] + f * ProjRefSamples[y + i + 2 + NumProj] + 16) >> 5;
                else
                    patch[y * size + x] = ProjRefSamples[y + i + 1 + NumProj];
            }
    else
        for (mfxI32 y = 0; y < size; y++)
            for (mfxI32 x = 0; x < size; x++) {
                mfxI32 f = ((y + 1) * params.intraPredAngle) & 31;
                mfxI32 i = ((y + 1) * params.intraPredAngle) >> 5;
                if (f != 0)
                    patch[y * size + x] = ((32 - f) * ProjRefSamples[x + i + 1 + NumProj] + f * ProjRefSamples[x + i + 2 + NumProj] + 16) >> 5;
                else
                    patch[y * size + x] = ProjRefSamples[x + i + 1 + NumProj];
            }

    return;
}

void FrameProcessor::GenerateIntraPrediction(const std::vector<mfxU8>& RefSamples, mfxU8 blockSize, INTRA_MODE currMode, mfxU8* currPlane)
{
    if (currPlane == nullptr)
    {
        throw std::string("ERROR: GenerateIntraPrediction: pointer to buffer is null\n");
    }

    IntraParams params(currMode);

    switch (params.intraMode)
    {
    case PLANAR:
        PlanarPrediction(RefSamples, blockSize, currPlane);
        break;
    case DC:
        DCPrediction(RefSamples, blockSize, currPlane);
        break;
    default:
        AngularPrediction(RefSamples, blockSize, params, currPlane);
        break;
    }
    return;
}

void FrameProcessor::MakePostFilter(const std::vector<mfxU8>& RefSamples, mfxU8 size, INTRA_MODE currMode, mfxU8* lumaPlane)
{
    mfxU32 DCValue = lumaPlane[0];

    switch (currMode)
    {
    case DC:
        lumaPlane[0] = (RefSamples[2 * size - 1] + 2 * DCValue + RefSamples[2 * size + 1] + 2) >> 2;
        for (mfxI32 x = 1; x < size; x++)
        {
            lumaPlane[x] = (RefSamples[2 * size + 1 + x] + 3 * DCValue + 2) >> 2;
        }
        for (mfxI32 y = 1; y < size; y++)
        {
            lumaPlane[y * size] = (RefSamples[2 * size - 1 - y] + 3 * DCValue + 2) >> 2;
        }
        break;
    case ANG10_HOR:
        for (mfxI32 x = 0; x < size; x++)
        {
            lumaPlane[x] = ClipIntToChar(lumaPlane[x] + ((RefSamples[2 * size + 1 + x] - RefSamples[2 * size]) >> 1));
        }
        break;
    case ANG26_VER:
        for (mfxI32 y = 0; y < size; y++)
        {
            lumaPlane[y * size] = ClipIntToChar(lumaPlane[y * size] + ((RefSamples[2 * size - 1 - y] - RefSamples[2 * size]) >> 1));
        }
        break;
    default:
        return;
    }
}

void  FrameProcessor::PutPatchIntoFrame(const PatchBlock & Patch, mfxFrameSurface1& surf) {
    //luma
    for (mfxU32 i = 0; i < Patch.m_BHeight; i++)
        memcpy(surf.Data.Y + (Patch.m_AdrY + i) * surf.Data.Pitch + Patch.m_AdrX, Patch.m_YPlane + i * Patch.m_BWidth, Patch.m_BWidth);
    //chroma U
    for (mfxU32 i = 0; i < Patch.m_BHeight / 2; ++i)
        memcpy(surf.Data.U + (Patch.m_AdrY / 2 + i) * surf.Data.Pitch / 2 + Patch.m_AdrX / 2, Patch.m_UPlane + i * (Patch.m_BWidth / 2), Patch.m_BWidth / 2);
    //chroma V
    for (mfxU32 i = 0; i < Patch.m_BHeight / 2; ++i)
        memcpy(surf.Data.V + (Patch.m_AdrY / 2 + i) * surf.Data.Pitch / 2 + Patch.m_AdrX / 2, Patch.m_VPlane + i * (Patch.m_BWidth / 2), Patch.m_BWidth / 2);
}

void FrameProcessor::GetIntraPredPlane(const BaseBlock& refBlock, INTRA_MODE currMode, const PatchBlock& frame, COLOR_COMPONENT colorComp, mfxU8* currPlane)
{
    if (currPlane == nullptr)
    {
        throw std::string("ERROR: GetIntraPredPlane: pointer to buffer is null\n");
    }
    //here refBlock parameters are measured in samples of corresponding colorComp
    //get reference samples for current TU
    std::vector<mfxU8> RefSamples;
    //size and coords of block in current color component
    mfxU32 cSize = (colorComp == LUMA_Y) ? refBlock.m_BHeight : (refBlock.m_BHeight / 2);
    mfxU32 cAdrX = (colorComp == LUMA_Y) ? refBlock.m_AdrX : (refBlock.m_AdrX / 2);
    mfxU32 cAdrY = (colorComp == LUMA_Y) ? refBlock.m_AdrY : (refBlock.m_AdrY / 2);

    FillIntraRefSamples(cSize, cAdrX, cAdrY, frame, colorComp, RefSamples);

    // get filter, write it into buffer and make it
    MakeFilter(RefSamples, cSize, currMode);

    //generate Prediction and return the output patch
    GenerateIntraPrediction(RefSamples, cSize, currMode, currPlane);

    if (colorComp == LUMA_Y && cSize < 32)
    {
        MakePostFilter(RefSamples, cSize, currMode, currPlane);
    }
}


PatchBlock FrameProcessor::GetIntraPatchBlock(const TUBlock& refBlock, const PatchBlock& frame)
{
    PatchBlock patch(refBlock);
    //get intra prediction for Luma plane
    GetIntraPredPlane(refBlock, refBlock.m_IntraModeLuma, frame, LUMA_Y, patch.m_YPlane);
    //if luma TB size > 4, fill chroma TBs of size / 2
    if (refBlock.m_BHeight != 4)
    {
        GetIntraPredPlane(refBlock, refBlock.m_IntraModeChroma, frame, CHROMA_U, patch.m_UPlane);
        GetIntraPredPlane(refBlock, refBlock.m_IntraModeChroma, frame, CHROMA_V, patch.m_VPlane);
        return patch;
    }

    // else luma TB has size = 4, we have one chroma TB corresponding to four 4x4 luma TBs
    //if refBlock is the lower-right block among four brothers in the RQT,
    //fill extendedPatch of size 8x8 in luma samples with:
    //see last paragraph before new chapter, p. 65(76) in HEVC Algorithms and Architectures
    if (refBlock.m_AdrX % 8 == 4 && refBlock.m_AdrY % 8 == 4)
    {
        //three luma blocks 4x4 already put into targetBlock
        PatchBlock extendedPatch = PatchBlock(BaseBlock(refBlock.m_AdrX - 4, refBlock.m_AdrY - 4, 8, 8), frame);
        //luma component of size 4x4 taken from patch
        extendedPatch.InsertAnotherPatch(patch);
        //chroma components of size 4x4 corresponding to the union of four luma blocks mentioned above
        GetIntraPredPlane(extendedPatch, refBlock.m_IntraModeChroma, frame, CHROMA_U, extendedPatch.m_UPlane);
        GetIntraPredPlane(extendedPatch, refBlock.m_IntraModeChroma, frame, CHROMA_V, extendedPatch.m_VPlane);
        return extendedPatch;
    }

    //else return only luma prediction
    return patch;
}

void FrameProcessor::MakeTUIntraPrediction(const TUBlock& refBlock, PatchBlock& targetPatch)
{
    PatchBlock patch(refBlock);
    //now the most contrast mode is determined only for luma component, chroma mode is set equal to luma mode
    GetIntraPredPlane(refBlock, refBlock.m_IntraModeLuma, targetPatch, LUMA_Y, patch.m_YPlane);
    //write Patch into targetPatch
    targetPatch.InsertAnotherPatch(patch);
}

void FrameProcessor::ApplyTUIntraPrediction(const TUBlock & block, ExtendedSurface& surf)
{
    PatchBlock framePatchBlock(BaseBlock(0, 0, surf.Info.CropW, surf.Info.CropH), surf);
    PatchBlock patch = GetIntraPatchBlock(block, framePatchBlock);
    //write Patch into frame
    PutPatchIntoFrame(patch, surf);
}

//Iterates over CUs in CTU and applies intra prediction for intra CUs inside it
void FrameProcessor::ApplyIntraPredInCTU(const CTUDescriptor & CTU, FrameChangeDescriptor & frame_descr)
{
    for (auto& CU : CTU.m_CUVec)
    {
        if (CU.m_PredType == INTRA_PRED)
        {
            for (auto& TU : CU.m_TUVec)
            {
                ApplyTUIntraPrediction(TU, *frame_descr.m_frame);
            }
        }
    }
}

#endif // MFX_VERSION
