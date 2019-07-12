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

#include "mvmvp_processor.h"

// Generates MVP pool groups according to PU positioning in CTU and constructed MVP blocks grid
void MVMVPProcessor::InitMVPGridData(const CTUDescriptor& CTU, const FrameChangeDescriptor & frameDescr)
{
    mfxU32 gridSizeIn32x32Blocks = CTU.m_BWidth / (2 * MVP_BLOCK_SIZE);

    //  16x16 MVP blocks in frame; numbers denote order in file:
    //  ++++++++++++++++++++
    //  + 0 + 1 + 4 + 5 +
    //  +++++++++++++++++ ...
    //  + 2 + 3 + 6 + 7 +
    //  +++++++++++++++++
    //  +      ...
    //  Each quarter of 16x16 blocks with sequent indecies are forming a 32x32 block group

    m_mvpBlockGrid.reserve(gridSizeIn32x32Blocks * gridSizeIn32x32Blocks);

    if (!gridSizeIn32x32Blocks) // only one 16x16 block
    {
        m_mvpBlockGrid.emplace_back(CTU.m_AdrX, CTU.m_AdrY, MVP_BLOCK_SIZE, MVP_BLOCK_SIZE);
    }
    else
    {
        // Pushing MVP blocks onto the grid in zig-zag order described above
        for (mfxU32 gridIdx = 0; gridIdx < gridSizeIn32x32Blocks * gridSizeIn32x32Blocks; ++gridIdx)
        {
            for (mfxU32 elemIdx = 0; elemIdx < 4; ++elemIdx)
            {
                m_mvpBlockGrid.emplace_back(
                    CTU.m_AdrX + (2 * (gridIdx % gridSizeIn32x32Blocks) + (elemIdx % 2)) * MVP_BLOCK_SIZE,
                    CTU.m_AdrY + (2 * (gridIdx / gridSizeIn32x32Blocks) + (elemIdx / 2)) * MVP_BLOCK_SIZE,
                    MVP_BLOCK_SIZE, MVP_BLOCK_SIZE);
            }
        }
    }

    mfxI32 mvpGroupsNum = ConstructMVPPoolGroups(CTU);

    // All PUs are processed - constructing actual MVP pools per each group

    m_mvpPools.resize(mvpGroupsNum);

    for (mfxI32 groupNo = 0; groupNo < mvpGroupsNum; ++groupNo)
    {
        m_mvpPools[groupNo].reserve(MVP_PER_16x16_BLOCK);

        for (mfxU32 mvpCount = 0; mvpCount < MVP_PER_16x16_BLOCK; ++mvpCount)
        {
            m_mvpPools[groupNo].emplace_back(GenerateMVP(CTU, frameDescr));
        }
    }
}

mfxI32 MVMVPProcessor::ConstructMVPPoolGroups(const CTUDescriptor& CTU)
{
    switch (m_GenMVPBlockSize)
    {
    case 1:
        if (CTU.m_BWidth == 32 || CTU.m_BWidth == 64)
        {
            return PutPUAndMVPBlocksIn16x16MVPPoolGroups(CTU);
        }
    case 2:
        if (CTU.m_BWidth == 64)
        {
            return PutPUAndMVPBlocksIn32x32MVPPoolGroups(CTU);
        }
    case 3:
        return PutPUAndMVPBlocksInSingleMVPPoolGroup(CTU);

    default:
        throw std::string("ERROR: MVMVPProcessor: ConstructMVPPoolGroups: Incorrect m_GenMVPBlockSize used");
    }
}

mfxI32 MVMVPProcessor::PutPUAndMVPBlocksIn16x16MVPPoolGroups(const CTUDescriptor & CTU)
{
    mfxI32 extMVPGroupNo = 0;

    for (const auto& CU : CTU.m_CUVec)
    {
        if (CU.m_PredType == INTER_PRED)
        {
            // For all the inter PUs constructing a MVP block list which this PU has intersections with
            for (const auto& PU : CU.m_PUVec)
            {
                // Construct a list with all MVP blocks which have intersections with current PU

                std::list<MVPBlock> intersectedMVPBlocks;

                for (const auto& mvpBlock : m_mvpBlockGrid)
                {
                    if (PU.CheckForIntersect(mvpBlock))
                    {
                        intersectedMVPBlocks.push_back(mvpBlock);
                    }
                }

                // Checking the list validity. It should have at least 1 element
                if (intersectedMVPBlocks.empty())
                {
                    throw std::string("ERROR: MVMVPProcessor: PutPUAndMVPBlocksIn16x16MVPPoolGroups: Found a PU which doesn't intersect the MVP grid");
                }

                // Check if there is at least one block which is included in some group
                // MVP blocks should have no group or be included in the one group
                auto validGroupMVPBlock = std::find_if(intersectedMVPBlocks.begin(), intersectedMVPBlocks.end(),
                    [](const MVPBlock& block) { return block.IsAlreadyInGroup(); });

                // If it exists, put all MVP blocks intersected with current PU in this group
                if (validGroupMVPBlock != intersectedMVPBlocks.end())
                {
                    mfxI32 firstValidGroupNo = validGroupMVPBlock->GetGroupNo();

                    if (std::any_of(intersectedMVPBlocks.begin(), intersectedMVPBlocks.end(),
                        [firstValidGroupNo](const MVPBlock& block)
                        { return (block.IsAlreadyInGroup() && !block.IsInGroup(firstValidGroupNo)); }))
                    {
                        throw std::string("ERROR: PutPUAndMVPBlocksIn16x16MVPPoolGroups : Found a pair of MVP blocks intersecting with current PU which are in the different MVP groups");
                    }
                    else // include all intersecting MVP blocks in the single group
                    {
                        for (MVPBlock& block : m_mvpBlockGrid)
                        {
                            if (std::find(intersectedMVPBlocks.begin(), intersectedMVPBlocks.end(), block)
                                != intersectedMVPBlocks.end())
                            {
                                block.SetGroup(firstValidGroupNo);
                            }
                        }
                        // The list is valid - set PU pool group no. with the value for MVP blocks
                        m_PUtoMVPPoolGroupMap[PU] = firstValidGroupNo;
                    }
                }
                else // Otherwise, put all MVP blocks in the new group
                {
                    for (MVPBlock& block : m_mvpBlockGrid)
                    {
                        if (std::find(intersectedMVPBlocks.begin(), intersectedMVPBlocks.end(), block)
                            != intersectedMVPBlocks.end())
                        {
                            block.SetGroup(extMVPGroupNo);
                        }
                    }
                    // The list is valid - set PU pool group no. with the value for MVPs
                    m_PUtoMVPPoolGroupMap[PU] = extMVPGroupNo;

                    ++extMVPGroupNo;
                }
            }
        }
    }
    return extMVPGroupNo;
}

mfxI32 MVMVPProcessor::PutPUAndMVPBlocksIn32x32MVPPoolGroups(const CTUDescriptor & CTU)
{
    // Special case for 64x64 CTUs and 32x32 MVP block size
    mfxI32 extMVPGroupNo = 0;

    // Construct FOUR 32x32 block groups from the 16x16 block grid
    std::vector<MVP32x32BlockGroup> mvp32x32BlockGroups;
    mvp32x32BlockGroups.reserve(4);

    for (mfxU32 gridIdx = 0; gridIdx < m_mvpBlockGrid.size(); gridIdx += 4)
    {
        mvp32x32BlockGroups.emplace_back(BaseBlock(m_mvpBlockGrid[gridIdx].m_AdrX,
            m_mvpBlockGrid[gridIdx].m_AdrY,
            MVP_BLOCK_SIZE * 2, MVP_BLOCK_SIZE * 2), gridIdx);
    }

    for (const auto& CU : CTU.m_CUVec)
    {
        if (CU.m_PredType == INTER_PRED)
        {
            // For all the inter PUs constructing a list of 32x32 MVP block groups
            // that this PU has intersections with
            for (const auto& PU : CU.m_PUVec)
            {
                // Construct a list with all 32x32 MVP block groups which have intersections with current PU
                std::list<MVP32x32BlockGroup> intersected32x32MVPBlocks;

                for (const auto& mvp32x32Block : mvp32x32BlockGroups)
                {
                    if (PU.CheckForIntersect(mvp32x32Block.first))
                    {
                        intersected32x32MVPBlocks.push_back(mvp32x32Block);
                    }
                }

                // Checking the list validity. It should have at least 1 element
                if (intersected32x32MVPBlocks.empty())
                {
                    throw std::string("ERROR: MVMVPProcessor: PutPUAndMVPBlocksIn32x32MVPPoolGroups: Found a PU which doesn't intersect the 32x32 MVP groups grid");
                }

                // Check if there is at least one block which is included in some group
                // MVP blocks should have no group or be included in a single group
                auto validGroupMVPBlock = std::find_if(intersected32x32MVPBlocks.begin(),
                    intersected32x32MVPBlocks.end(), [this](const MVP32x32BlockGroup& block32x32)
                        { return m_mvpBlockGrid[block32x32.second].IsAlreadyInGroup(); });

                // If it exists, put all MVP blocks intersecting the current PU in this group
                if (validGroupMVPBlock != intersected32x32MVPBlocks.end())
                {
                    mfxI32 firstValidGroupNo = m_mvpBlockGrid[validGroupMVPBlock->second].GetGroupNo();

                    if (std::any_of(intersected32x32MVPBlocks.begin(),
                        intersected32x32MVPBlocks.end(), [this, firstValidGroupNo](const MVP32x32BlockGroup& block32x32)
                            { return (m_mvpBlockGrid[block32x32.second].IsAlreadyInGroup() && !m_mvpBlockGrid[block32x32.second].IsInGroup(firstValidGroupNo)); }))
                    {
                        throw std::string("ERROR: PutPUAndMVPBlocksIn32x32MVPPoolGroups : Found a pair of MVP blocks intersecting with current PU which are in the different MVP groups");
                    }
                    else // include all intersecting MVP blocks in the single group
                    {
                        for (MVPBlock& block : m_mvpBlockGrid)
                        {
                            if (std::find_if(intersected32x32MVPBlocks.begin(), intersected32x32MVPBlocks.end(),
                                [this, &block](const MVP32x32BlockGroup& block32x32)
                                    { return m_mvpBlockGrid[block32x32.second] == block; }) != intersected32x32MVPBlocks.end())
                            {
                                block.SetGroup(firstValidGroupNo);
                            }
                        }
                        // The list is valid - set PU pool group no. with the value for MVP blocks
                        m_PUtoMVPPoolGroupMap[PU] = firstValidGroupNo;
                    }
                }
                else // Otherwise, put all MVP blocks in the new group
                {
                    for (MVPBlock& block : m_mvpBlockGrid)
                    {
                        if (std::find_if(intersected32x32MVPBlocks.begin(), intersected32x32MVPBlocks.end(),
                            [this, &block](const MVP32x32BlockGroup& block32x32)
                        { return m_mvpBlockGrid[block32x32.second] == block; }) != intersected32x32MVPBlocks.end())
                        {
                            block.SetGroup(extMVPGroupNo);
                        }
                    }

                    // The list is valid - set PU pool group no. with the value for MVPs
                    m_PUtoMVPPoolGroupMap[PU] = extMVPGroupNo;

                    ++extMVPGroupNo;
                }
            }
        }
    }
    return extMVPGroupNo;
}

mfxI32 MVMVPProcessor::PutPUAndMVPBlocksInSingleMVPPoolGroup(const CTUDescriptor & CTU)
{
    const mfxI32 extGroupNo = 0;

    for (MVPBlock& block : m_mvpBlockGrid)
    {
        block.SetGroup(extGroupNo);
    }

    for (const auto& CU : CTU.m_CUVec)
    {
        if (CU.m_PredType == INTER_PRED)
        {
            for (const auto& PU : CU.m_PUVec)
            {
                m_PUtoMVPPoolGroupMap[PU] = extGroupNo;
            }
        }
    }

    return 1;
}

// Fills output mfxExtFeiHevcEncMVPredictors::Data with generated MVP Grid data depending on the m_GenMVPBlockSize value
//
// frameDescr - MOD frame descriptor. It holds ext buffers
void MVMVPProcessor::FillFrameMVPExtBuffer(FrameChangeDescriptor& frameDescr)
{
    ExtendedSurface& surf = *frameDescr.m_frame;

    mfxExtFeiHevcEncMVPredictors* mvpBuf =
        reinterpret_cast<mfxExtFeiHevcEncMVPredictors*>(surf.GetBuffer(MFX_EXTBUFF_HEVCFEI_ENC_MV_PRED));

    m_DoUseIntra = !!(frameDescr.m_testType & GENERATE_INTRA);

    switch (m_GenMVPBlockSize)
    {
    case 0:
        // No MVP specified. Left buffer unchanged
        break;
    case 1:
        // MVP for 16x16 blocks
        // Iterating over all generated blocks
        for (const auto& block : m_mvpBlockGrid)
        {
            PutMVPIntoExtBuffer(block, mvpBuf);
        }
        break;
    case 2:
        // MVP for 32x32 blocks
        // Iterating over each upper-left block in the 32x32 group
        // | 0* 1 | 4* 5 |
        // | 2  3 | 6  7 |
        for (std::size_t gridIdx = 0; gridIdx < m_mvpBlockGrid.size(); gridIdx += 4)
        {
            PutMVPIntoExtBuffer(m_mvpBlockGrid[gridIdx], mvpBuf);
        }
        break;
    case 3:
        //  MVP for 64x64 blocks
        //  16x16 blocks marked with X should be the same in the output predictors buffer
        //  ++++++++++++++++++++
        //  + X +   + X +   +
        //  +++++++++++++++++ ...
        //  +   +   +   +   +
        //  +++++++++++++++++
        //  + X +   + X +   + ...
        //  +++++++++++++++++
        //  +   +   +   +   +
        //  +++++++++++++++++
        //  +      ...
        PutMVPIntoExtBuffer(m_mvpBlockGrid[0], mvpBuf);
        break;
    default:
        break;
    }
}


bool MVMVPProcessor::GenValidMVMVPForPU(PUBlock & PU, const FrameChangeDescriptor & frameDescr)
{
    const auto mvpGroupNo = m_PUtoMVPPoolGroupMap.find(PU);

    if (mvpGroupNo != m_PUtoMVPPoolGroupMap.end())
    {
        if (mvpGroupNo->second < 0)
        {
            throw std::string("ERROR: MVMVPProcessor: GenValidMVMVPForPU: Some PU has invalid MVP pool no.");
        }
    }
    else
    {
        throw std::string("ERROR: MVMVPProcessor: GenValidMVMVPForPU: Some PU hasn't been included in MVP pool group map");
    }

    // Inter prediction test
    // Try 100 times max to generate valid references
    for (mfxU32 i = 0; i < MAX_GEN_MV_ATTEMPTS; ++i)
    {
        PUMotionVector mvSW;
        if (frameDescr.m_testType & GENERATE_MV)
        {
            // If GENERATE_MV is specified, generate a vector inside SW; else, leave mvSW a zero vector
            // Always create mv inside SW; mvp - outside SW
            mvSW = GenerateMV(frameDescr);
        }

        std::vector<PUMotionVector> mvpPoolForPU(m_mvpPools[mvpGroupNo->second]);

        while (!mvpPoolForPU.empty())
        {
            mfxU32 localMvpIndex = GetRandomGen().GetRandomNumber(0, (mfxI32)mvpPoolForPU.size() - 1);

            PU.m_MVP = mvpPoolForPU[localMvpIndex];

            // Final MV; operator + used here
            // Important: m_MV inherits refIdx from m_MVP. It is done so because MVP would be written to file if required.
            //            MV is never reported outside.
            PU.m_MV = PU.m_MVP + mvSW;

            if (frameDescr.IsNewPUValid(PU))
            {
                // Get index of the MVP applied to this PU from the original MVP generation pool
                for (mfxU32 originalMvpIndex = 0; originalMvpIndex < MVP_PER_16x16_BLOCK; ++originalMvpIndex)
                {
                    if (m_mvpPools[mvpGroupNo->second][originalMvpIndex] == PU.m_MVP)
                    {
                        PU.usedMVPPoolNo  = mvpGroupNo->second;
                        PU.usedMVPIndex   = originalMvpIndex;
                    }
                }
                return true; // MV generation for current PU succeeded
            }
            else
            {
                mvpPoolForPU.erase(mvpPoolForPU.begin() + localMvpIndex);
            }
        }
    }
    return false; //MV generation for current PU failed
}

//Attempts to generate MVs for PUs inside the CU and store the information about the blocks occupied
//by newly generated PUs in corresponding reference frame descriptors
bool MVMVPProcessor::GenValidMVForPU(PUBlock & PU, const FrameChangeDescriptor & frameDescr)
{
    if (!(frameDescr.m_testType & GENERATE_MV))
    {
        return true; //No MVs requested
    }

    // Inter prediction test
    // Try 100 times max to generate valid references
    for (mfxU32 i = 0; i < MAX_GEN_MV_ATTEMPTS; ++i)
    {
        // Always create mv inside SW; mvp - outside SW (if predictors are not required, mvp is zero vector)
        PUMotionVector mvSW = GenerateMV(frameDescr);

        PU.m_MVP = PUMotionVector(
            mfxU8(GetRandomGen().GetRandomNumber(0, std::max(0, mfxU8(frameDescr.m_refDescrList0.size()) - 1))),
            mfxU8(GetRandomGen().GetRandomNumber(0, std::max(0, mfxU8(frameDescr.m_refDescrList1.size()) - 1))),
            mfxI16(0), mfxI16(0), mfxI16(0), mfxI16(0));

        // Final MV; operator + used here
        // Important: m_MV inherits refIdx from m_MVP. It is done so because MVP would be written to file if required.
        //            MV is never reported outside.
        PU.m_MV = PU.m_MVP + mvSW;

        // Check whether the generated PU is located inside frame boundaries
        // Check generated PU for intersection with previous PUs pointing to other frames
        if (frameDescr.IsNewPUValid(PU))
        {
            return true; //MV generation for current PU succeeded
        }
    }

    return false; //MV generation for current PU failed
}

// Generate Motion Vector Predictor, i.e such a vector that all HW_SEARCH_ELEMENT_SIZE-sized square blocks
// inside the CTU shifted by this vector will be located outside their own search windows (which have same
// size but are centered each on its own search element block), and the CTU itself is still located
// inside the frame
// If the frame size is smaller than the search window size, generate MVP with zero spatial coordinates.
// Function generates predictors for list 1 only on B frames (not on GPB)
//
// const CTUDescriptor & CTU - current CTU which should be located inside the frame with predictor applied as MV
// const FrameChangeDescriptor & frameChangeDescr - descriptor which contains these fields:
// mfxU8 numl0_refs / numl1_refs - current limits on number of active references
// mfxU32 surf_width / surf_height - size of current surface
PUMotionVector MVMVPProcessor::GenerateMVP(const CTUDescriptor & CTU, const FrameChangeDescriptor & frameChangeDescr)
{
    const auto& numl0_refs = frameChangeDescr.m_refDescrList0;
    const auto& numl1_refs = frameChangeDescr.m_refDescrList1;

    bool hasList1 = !numl1_refs.empty();

    if (hasList1 && !frameChangeDescr.m_bUseBiDirSW)
    {
        throw std::string("ERROR: GenerateMVP: attempted to generate MVPs for a B-frame/GPB using unidirectional search window");
    }

    bool isB = !!(frameChangeDescr.m_frameType & MFX_FRAMETYPE_B);

    const mfxU32* current_lim = frameChangeDescr.m_bUseBiDirSW ? SKL_SW_LIM_BD : SKL_SW_LIM_OD;

    mfxU32 x_lim_SW = current_lim[0] / 2;
    mfxU32 y_lim_SW = current_lim[1] / 2;

    mfxI32 x_coord[2] = { 0 };
    mfxI32 y_coord[2] = { 0 };

    // Precomputing min and max possible MVP coordinates so that the MVP points outside SW but inside
    // the frame (i.e. the CTU with applied MVP should be located outside SW and inside the frame,
    // if possible)
    mfxU32 x_min = x_lim_SW + HW_SEARCH_ELEMENT_SIZE / 2;        //When determining min absolute shift, one should use
                                                                 //the search element size, not the CTU size. With max
                                                                 //CTU size is still used so that it stays inside the frame

    mfxU32 x_max_pos = frameChangeDescr.GetFrameCropW()
        - (CTU.m_BWidth + CTU.m_AdrX); //Max absolute value of MVP coord for positive
    mfxU32 x_max_neg = CTU.m_AdrX;     //and negative coordinates respectively.

    mfxU32 y_min = y_lim_SW + HW_SEARCH_ELEMENT_SIZE / 2;

    mfxU32 y_max_pos = frameChangeDescr.GetFrameCropH() - (CTU.m_BWidth + CTU.m_AdrY);
    mfxU32 y_max_neg = CTU.m_AdrY;

    //Crop the max coords due to hardware limitations
    x_max_pos = std::min(x_max_pos, ((mfxU32)((MVMVP_SIZE_LIMIT >> 2) - x_lim_SW)));
    x_max_neg = std::min(x_max_neg, ((mfxU32)((MVMVP_SIZE_LIMIT >> 2) - x_lim_SW)));
    y_max_pos = std::min(y_max_pos, ((mfxU32)((MVMVP_SIZE_LIMIT >> 2) - y_lim_SW)));
    y_max_neg = std::min(y_max_neg, ((mfxU32)((MVMVP_SIZE_LIMIT >> 2) - y_lim_SW)));

    enum SHIFT { X_POS, X_NEG, Y_POS, Y_NEG };

    std::vector<SHIFT> availShifts;
    availShifts.reserve(4);

    if (x_min < x_max_pos) availShifts.push_back(X_POS);
    if (x_min < x_max_neg) availShifts.push_back(X_NEG);
    if (y_min < y_max_pos) availShifts.push_back(Y_POS);
    if (y_min < y_max_neg) availShifts.push_back(Y_NEG);

    if (!availShifts.empty())
    {
        for (mfxI32 i = 0; i < (isB ? 2 : 1); i++)
        {
            SHIFT shift = availShifts[GetRandomGen().GetRandomNumber(0, (mfxI32)(availShifts.size() - 1))];
            switch (shift)
            {
            case X_POS:
                x_coord[i] = GetRandomMVComponent(x_min, x_max_pos);
                break;
            case X_NEG:
                x_coord[i] = -GetRandomMVComponent(x_min, x_max_neg);
                break;
            case Y_POS:
                y_coord[i] = GetRandomMVComponent(y_min, y_max_pos);
                break;
            case Y_NEG:
                y_coord[i] = -GetRandomMVComponent(y_min, y_max_neg);
                break;
            }

            //The complementary MVP coordinate should be subjected to the HW limit as well
            if (shift == X_POS || shift == X_NEG)
            {
                mfxU32 max_pos_shift_y = std::min(frameChangeDescr.GetFrameCropH() - CTU.m_BHeight - CTU.m_AdrY,
                    (mfxU32)MVMVP_SIZE_LIMIT >> 2);
                mfxU32 max_neg_shift_y = std::min(CTU.m_AdrY, (mfxU32)MVMVP_SIZE_LIMIT >> 2);
                y_coord[i] = GetRandomMVComponent(-(mfxI32)max_neg_shift_y, max_pos_shift_y);
            }
            else
            {
                mfxU32 max_pos_shift_x = std::min(frameChangeDescr.GetFrameCropW() - CTU.m_BWidth - CTU.m_AdrX,
                    (mfxU32)MVMVP_SIZE_LIMIT >> 2);
                mfxU32 max_neg_shift_x = std::min(CTU.m_AdrX, (mfxU32)MVMVP_SIZE_LIMIT >> 2);
                x_coord[i] = GetRandomMVComponent(-(mfxI32)max_neg_shift_x, max_pos_shift_x);
            }
        }
    }

    return PUMotionVector(
        (mfxI32)!numl0_refs.empty() ? mfxU8(GetRandomGen().GetRandomNumber(0, (mfxI32)numl0_refs.size() - 1)) : 0,
        (mfxI32)!numl1_refs.empty() ? mfxU8(GetRandomGen().GetRandomNumber(0, (mfxI32)numl1_refs.size() - 1)) : 0, // RefIdx struct
        x_coord[0],                            // MV[0].x
        y_coord[0],                            // MV[0].y
        isB ? x_coord[1] : 0,                  // MV[1].x
        isB ? y_coord[1] : 0                   // MV[1].y
    );
}

// Generates an MV so that a PU_width x PU_height-sized block shifted by this MV
// will be located inside(sic!) SW. It doesn't generate refIdx. refIdx is generated in GenerateMVP
// Function generates MVs for list 1 only on B frames (not on GPB)
//
// mfxU32 PU_width/PU_height - size of current PU
PUMotionVector MVMVPProcessor::GenerateMV(const FrameChangeDescriptor & frameChangeDescr)
{
    const auto& numl1_refs = frameChangeDescr.m_refDescrList1;

    bool hasList1 = !numl1_refs.empty();

    if (hasList1 && !frameChangeDescr.m_bUseBiDirSW)
    {
        throw std::string("ERROR: GenerateMV: attempted to generate MVs for a B-frame/GPB using unidirectional search window");
    }

    bool isB = !!(frameChangeDescr.m_frameType & MFX_FRAMETYPE_B);

    const mfxU32* current_lim = frameChangeDescr.m_bUseBiDirSW ? SKL_SW_LIM_BD : SKL_SW_LIM_OD;

    // Just half of current SW width / height
    mfxI32 x_lim = 0, y_lim = 0;

    constexpr mfxU32 searchElementWidth = HW_SEARCH_ELEMENT_SIZE;
    constexpr mfxU32 searchElementHeight = HW_SEARCH_ELEMENT_SIZE;

    //Leave limits at zero if PU size exceeds the search window
    if (current_lim[0] >= searchElementWidth && current_lim[1] >= searchElementHeight)
    {
        x_lim = (current_lim[0] - searchElementWidth) / 2;
        y_lim = (current_lim[1] - searchElementHeight) / 2;
    }

    // Using RVO here
    return PUMotionVector(
        0,
        0,                                             // RefIdx struct
        GetRandomMVComponent(-x_lim, x_lim),           // MV[0].x
        GetRandomMVComponent(-y_lim, y_lim),           // MV[0].y
        isB ? GetRandomMVComponent(-x_lim, x_lim) : 0, // MV[1].x
        isB ? GetRandomMVComponent(-y_lim, y_lim) : 0  // MV[1].y
    );
}

// Randomly generates single MV-component within [lower; upper] accordigly to the sub pixel precision mode
// Where lower and upper given in full-pixel units.
// Output component is in quarter-pixel units.
//
// { a = lower * 4 }
// { b = upper * 4 }
//
// [a * # * a+1 * # * a+2 ... b-2 * # * b-1 * # * b]
// For mode 0: only full-pixels a, a+1, ... b-2, b-1, b are allowed
// For mode 1: full and half-pixels # are allowed
// For mode 3: every quarter-pixel value is allowed
mfxI32 MVMVPProcessor::GetRandomMVComponent(mfxI32 lower, mfxI32 upper)
{
    ASGRandomGenerator& randomGen = GetRandomGen();
    switch (m_SubPelMode)
    {
    case 0:
        return 4 * lower + randomGen.GetRandomNumber(0, upper - lower) * 4;
    case 1:
        return 4 * lower + randomGen.GetRandomNumber(0, 2 * (upper - lower)) * 2;
    case 3:
        return randomGen.GetRandomNumber(4 * lower, 4 * upper);
    default:
        throw std::string("ERROR: MVMVPProcessor: GetRandomMVComponent: Incorrect m_SubPelMode used");
    }
}

mfxU32 MVMVPProcessor::CalculateOffsetInMVPredictorsBuffer(mfxU32 bufferPitch, const MVPBlock & mvpBlock)
{
    // Calculating a no. of 32x32 block corresponding to left upper pixel of the CTU MVP grid
    // In sum below: first addendum is offset by vertical axis, second addendum is offset by horizontal axis
    mfxU32 offsetBig = (bufferPitch / 2) * (mvpBlock.m_AdrY / (2 * MVP_BLOCK_SIZE))
        + (mvpBlock.m_AdrX / (2 * MVP_BLOCK_SIZE));

    // Calculating a postion for 16x16 block inside parent 32x32 block
    mfxU32 offsetSmall = 2 * ((mvpBlock.m_AdrY % (2 * MVP_BLOCK_SIZE)) / MVP_BLOCK_SIZE)
        + ((mvpBlock.m_AdrX % (2 * MVP_BLOCK_SIZE)) / MVP_BLOCK_SIZE);

    return 4 * offsetBig + offsetSmall;
}

void MVMVPProcessor::PutMVPIntoExtBuffer(const MVPBlock& mvpBlock, mfxExtFeiHevcEncMVPredictors* outputMVPBuf)
{
    if (!outputMVPBuf)
    {
        throw std::string("ERROR: MVMVPProcessor: PutMVPIntoExtBuffer: Output mfxExtFeiHevcEncMVPredictors buffer is null");
    }

    mfxU32 offset = CalculateOffsetInMVPredictorsBuffer(outputMVPBuf->Pitch, mvpBlock);

    mfxFeiHevcEncMVPredictors& actualPredictor = outputMVPBuf->Data[offset];

    mfxI32 mvpGroupNo = mvpBlock.GetGroupNo();

    // In case of INTRAxINTER test some MVP blocks
    // may not intersect with PUs inside CTU
    // Otherwise, all MVP blocks should be assigned with a valid group no.
    if (mvpGroupNo < 0)
    {
        if (m_DoUseIntra)
        {
            // For such intra MVP block filling corresponding element in the buffer with ignored values
            for (mfxU32 mvpIdx = 0; mvpIdx < MVP_PER_16x16_BLOCK; mvpIdx++)
            {
                actualPredictor.MV[mvpIdx][0].x = actualPredictor.MV[mvpIdx][0].y = (mfxI16)0x8000;
                actualPredictor.MV[mvpIdx][1].x = actualPredictor.MV[mvpIdx][1].y = (mfxI16)0x8000;

                actualPredictor.RefIdx[mvpIdx].RefL0 = (mfxU8)0xf;
                actualPredictor.RefIdx[mvpIdx].RefL1 = (mfxU8)0xf;
            }

            // Nevertheless, setting BlockSize appropriately
            actualPredictor.BlockSize = m_GenMVPBlockSize;

            return;
        }

        // Otherwise, something definitely went wrong
        throw std::string("ERROR: PutMVPIntoExtBuffer: Some MVP block has invalid MVP pool no.");
    }

    for (mfxU32 mvpIdx = 0; mvpIdx < MVP_PER_16x16_BLOCK; mvpIdx++)
    {
        const auto& genMVPred = m_mvpPools[mvpGroupNo][mvpIdx];

        if (genMVPred.CheckMVPExceedsSizeLimits())
        {
            throw std::string("ERROR: PutMVPIntoExtBuffer: generated MVP size exceeds hardware limit");
        }

        actualPredictor.RefIdx[mvpIdx].RefL0 = genMVPred.RefIdx.RefL0;
        actualPredictor.RefIdx[mvpIdx].RefL1 = genMVPred.RefIdx.RefL1;
        actualPredictor.MV[mvpIdx][0] = genMVPred.MV[0];
        actualPredictor.MV[mvpIdx][1] = genMVPred.MV[1];
    }

    // Assuming that, m_GenMVPBlockSize has correct value
    actualPredictor.BlockSize = m_GenMVPBlockSize;

    // Special case for 64x64 blocks - copying initialized data to the neighbour blocks
    if (m_GenMVPBlockSize == 3)
    {
        if (outputMVPBuf->Pitch > 0)
        {
            outputMVPBuf->Data[offset + 4] = actualPredictor;
            outputMVPBuf->Data[offset + outputMVPBuf->Pitch] = actualPredictor;
            outputMVPBuf->Data[offset + outputMVPBuf->Pitch + 4] = actualPredictor;
        }
        else
        {
            throw std::string("ERROR: PutMVPIntoExtBuffer: mfxExtFeiHevcEncMVPredictors have zero pitch");
        }
    }
}

void MVMVPProcessor::GetMVPPools(std::vector<std::vector<PUMotionVector>>& outMVPPools)
{
    outMVPPools = m_mvpPools;
}

#endif // MFX_VERSION