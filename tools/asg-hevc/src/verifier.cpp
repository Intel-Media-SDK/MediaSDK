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

#include "verifier.h"

// Processor for verification mode
void Verifier::Init()
{
    try
    {        // Add buffers to bufferReader
        if (m_InputParams.m_TestType & (GENERATE_INTER | GENERATE_INTRA))
        {
            m_BufferReader.AddBuffer(MFX_EXTBUFF_HEVCFEI_PAK_CTU_REC, m_InputParams.m_PakCtuBufferFileName);
            m_BufferReader.AddBuffer(MFX_EXTBUFF_HEVCFEI_PAK_CU_REC, m_InputParams.m_PakCuBufferFileName);
        }
    }
    catch (std::string& e) {
        std::cout << e << std::endl;
        throw std::string("ERROR: Couldn't initialize Verifier");
    }
}

// Get surface
ExtendedSurface* Verifier::PrepareSurface()
{
    ExtendedSurface* surf = GetFreeSurf();
    if (!surf)
    {
        throw std::string("ERROR: Verifier::PrepareSurface: Undefined reference to surface");
    }
    ++surf->Data.Locked;
    return surf;
}

// Save all data
void Verifier::DropFrames()
{
    // Sort data by frame order and drop in DisplayOrder

    // TODO / FIXME: Add EncodedOrder processing, it might be required for HEVC FEI PAK
    m_Surfaces.sort([](ExtendedSurface& left, ExtendedSurface& right) { return left.Data.FrameOrder < right.Data.FrameOrder; });

    for (ExtendedSurface& surf : m_Surfaces)
    {
        // If Locked surface met - stop dumping
        if (surf.Data.Locked) break;

        // If the surface has already been output, move on to the next one in frame order
        if (surf.isWritten) continue;

        surf.isWritten = true;
    }
}

void Verifier::SavePSData()
{
    if (fpPicStruct.is_open())
    {
        // sort by coding order
        std::sort(m_RefControl.RefLogInfo.begin(), m_RefControl.RefLogInfo.end(), IsInCodingOrder);

        //vector<RefState> RefLogInfo;

        while (!fpPicStruct.eof()) {
            RefState stateLd;
            if (!stateLd.Load(fpPicStruct))
                break;
            stateLd.picture.codingOrder = m_Counters.m_totalPics; // i.e. line number
            auto istate = m_RefControl.RefLogInfo.cbegin() + m_Counters.m_totalPics;
            m_Counters.m_totalPics++;
            m_Counters.m_correctRecords += (stateLd == *istate);

            //RefLogInfo.emplace_back(stateLd);
        }
    }
}

void Verifier::VerifyDesc(FrameChangeDescriptor & frameDescr)
{
    try
    {
        if (m_InputParams.m_TestType & (GENERATE_INTER | GENERATE_INTRA))
        {
            // Width/Height alignment should set as CTU size, because now we are working with bs_parser output data w/o HW requirements
            mfxU32 alignedWidth = MSDK_ALIGN(m_InputParams.m_width, m_InputParams.m_CTUStr.CTUSize);
            mfxU32 alignedHeight = MSDK_ALIGN(m_InputParams.m_height, m_InputParams.m_CTUStr.CTUSize);

            mfxU32 heightInCTU = alignedHeight / m_InputParams.m_CTUStr.CTUSize;
            mfxU32 widthInCTU = alignedWidth / m_InputParams.m_CTUStr.CTUSize;

            // For FEI compatibility
            mfxU32 maxNumCuInCtu = m_InputParams.m_CTUStr.GetMaxNumCuInCtu();

            // Buffers creations
            ExtendedBuffer tmpCtuBuff(MFX_EXTBUFF_HEVCFEI_PAK_CTU_REC, widthInCTU, heightInCTU);

            // This one has to be (maxNumCuInCtu) times larger than the CTU record buffer. For the time
            // being, implemented this via pitch adjustment
            ExtendedBuffer tmpCuBuff(MFX_EXTBUFF_HEVCFEI_PAK_CU_REC, widthInCTU*maxNumCuInCtu, heightInCTU);

            // Reading statistics from data storage
            m_BufferReader.ReadBuffer(&tmpCtuBuff.m_pakCtuRecord.Header);
            m_BufferReader.ReadBuffer(&tmpCuBuff.m_pakCuRecord.Header);
            // Structure will be in encoded order
            if (frameDescr.m_changeType == MOD)
            {
                for (auto& asgCTUDescr : frameDescr.m_vCTUdescr)
                {
                    m_Counters.m_testCTUs++;
                    mfxU32 idxYInCTU = asgCTUDescr.m_AdrYInCTU;
                    mfxU32 idxXInCTU = asgCTUDescr.m_AdrXInCTU;

                    mfxFeiHevcPakCtuRecordV0 ctuPakFromFEI = tmpCtuBuff.m_pakCtuRecord.Data[idxYInCTU * widthInCTU + idxXInCTU];

                    mfxU32 startCuOffset = maxNumCuInCtu * (idxYInCTU * widthInCTU + idxXInCTU);

                    // Prerare CTU descriptor from FEI output
                    CTUDescriptor feiCTUDescr = ConvertFeiOutInLocalStr(ctuPakFromFEI, tmpCuBuff, startCuOffset);

                    asgLog << std::endl << "Frame " << frameDescr.m_frameNumber << std::endl;
                    asgLog << "ASG " << asgCTUDescr << std::endl << "FEI " << feiCTUDescr << std::endl;

                    CompareSplits(asgCTUDescr, feiCTUDescr);
                    CompareMVs(asgCTUDescr, feiCTUDescr);
                    CountExactMatches(asgCTUDescr, feiCTUDescr);
                }
            }
        }
    }
    catch (std::string& e) {
        std::cout << e << std::endl;
        throw std::string("ERROR: Verifier::VerifyDesc");
    }
}

void Verifier::ExtractMVs(const CTUDescriptor& ctuDescr, std::vector<MVCompareBlock>& mvCmpBlocks)
{
    mfxU32 diffMaxBaseBlockLuma = m_InputParams.m_CTUStr.log2CTUSize - LOG2_MV_COMPARE_BASE_BLOCK_SIZE;
    mfxU32 widthInCompareBlocks = 1 << diffMaxBaseBlockLuma;
    constexpr mfxU32 baseBlockSize = 1 << LOG2_MV_COMPARE_BASE_BLOCK_SIZE;

    for (auto& CU : ctuDescr.m_CUVec)
    {
        switch (CU.m_PredType)
        {
        case INTER_PRED:
            for (auto& PU : CU.m_PUVec)
            {
                mfxU32 firstYBlock = (PU.m_AdrY - ctuDescr.m_AdrY) / baseBlockSize;
                mfxU32 firstXBlock = (PU.m_AdrX - ctuDescr.m_AdrX) / baseBlockSize;
                mfxU32 lastYBlock = firstYBlock + (PU.m_BHeight / baseBlockSize);
                mfxU32 lastXBlock = firstXBlock + (PU.m_BWidth / baseBlockSize);

                for (mfxU32 idxMVCBY = firstYBlock; idxMVCBY < lastYBlock; ++idxMVCBY)
                {
                    for (mfxU32 idxMVCBX = firstXBlock; idxMVCBX < lastXBlock; ++idxMVCBX)
                    {
                        MVCompareBlock& block = mvCmpBlocks.at(idxMVCBY * widthInCompareBlocks + idxMVCBX);
                        block.m_AdrX = ctuDescr.m_AdrX + baseBlockSize * idxMVCBX;
                        block.m_AdrY = ctuDescr.m_AdrY + baseBlockSize * idxMVCBY;
                        block.m_BHeight = block.m_BWidth = baseBlockSize;

                        block.MV = PU.m_MV;
                        block.usedMVPPoolNo = PU.usedMVPPoolNo;
                        block.usedMVPIndex  = PU.usedMVPIndex;

                        block.bIsIntra = false;

                        block.predFlagL0 = PU.predFlagL0;
                        block.predFlagL1 = PU.predFlagL1;
                    }
                }
            }
            break;
        case INTRA_PRED:
        {
            mfxU32 firstYBlock = (CU.m_AdrY - ctuDescr.m_AdrY) / baseBlockSize;
            mfxU32 firstXBlock = (CU.m_AdrX - ctuDescr.m_AdrX) / baseBlockSize;
            mfxU32 lastYBlock = firstYBlock + (CU.m_BHeight / baseBlockSize);
            mfxU32 lastXBlock = firstXBlock + (CU.m_BWidth / baseBlockSize);

            for (mfxU32 idxMVCBY = firstYBlock; idxMVCBY < lastYBlock; ++idxMVCBY)
            {
                for (mfxU32 idxMVCBX = firstXBlock; idxMVCBX < lastXBlock; ++idxMVCBX)
                {
                    MVCompareBlock& block = mvCmpBlocks.at(idxMVCBY * widthInCompareBlocks + idxMVCBX);
                    block.m_AdrX = ctuDescr.m_AdrX + baseBlockSize * idxMVCBX;
                    block.m_AdrY = ctuDescr.m_AdrY + baseBlockSize * idxMVCBY;
                    block.m_BHeight = block.m_BWidth = baseBlockSize;

                    block.bIsIntra = true;
                }
            }
            break;
        }
        default:
            throw std::string("ERROR: ExtractMVs: unknown CU prediction type\n");
        }
    }
}

void Verifier::CompareSplits(const CTUDescriptor& ctuDescrASG, const CTUDescriptor& ctuDescrFEI)
{
    //First, count total number of split partitions (PUs and CUs) by their size
    for (const auto& CU : ctuDescrASG.m_CUVec)
    {
        m_Counters.m_testCUSizeMapASG.AddBlockTotal(CU.m_BWidth, CU.m_BHeight);
        for (const auto& PU : CU.m_PUVec)
        {
            m_Counters.m_testPUSizeMapASG.AddBlockTotal(PU.m_BWidth, PU.m_BHeight);
        }
    }

    for (const auto& CU : ctuDescrFEI.m_CUVec)
    {
        m_Counters.m_testCUSizeMapFEI.AddBlockTotal(CU.m_BWidth, CU.m_BHeight);
        for (const auto& PU : CU.m_PUVec)
        {
            m_Counters.m_testPUSizeMapFEI.AddBlockTotal(PU.m_BWidth, PU.m_BHeight);
        }
    }

    asgLog << std::endl << "Comparing splits:" << std::endl;
    asgLog << "FEI CUs: " << ctuDescrFEI.m_CUVec.size() << ", ASG CUs: " << ctuDescrASG.m_CUVec.size() << std::endl;
    for (const auto& asgCU : ctuDescrASG.m_CUVec)
    {
        // Verificator finds equal CU
        asgLog << "Searching for CU " << asgCU;
        const auto itFeiCU = std::find_if(ctuDescrFEI.m_CUVec.begin(), ctuDescrFEI.m_CUVec.end(),
            [&asgCU](const CUBlock& cu) { return (cu.m_AdrX == asgCU.m_AdrX && cu.m_AdrY == asgCU.m_AdrY &&
                cu.m_BWidth == asgCU.m_BWidth && cu.m_BHeight == asgCU.m_BHeight); });
        if (itFeiCU != ctuDescrFEI.m_CUVec.end())
        {
            asgLog << " -- FOUND, FEI PUs: " << asgCU.m_PUVec.size()
                << ", ASG PUs: " << itFeiCU->m_PUVec.size() << std::endl;
            m_Counters.m_testCUSizeMapASG.AddBlockCorrect(itFeiCU->m_BWidth, itFeiCU->m_BHeight);
            for (auto& asgPU : asgCU.m_PUVec)
            {
                //Verificator finds equal PUs and calculates their number, while incrementing the correct count
                //for the respective PU size in the block size map
                m_Counters.m_correctPUs += (mfxU32)std::count_if(itFeiCU->m_PUVec.begin(), itFeiCU->m_PUVec.end(),
                    [&asgPU, this](const PUBlock& pu) {
                    if (pu.m_AdrX == asgPU.m_AdrX && pu.m_AdrY == asgPU.m_AdrY
                        && pu.m_BHeight == asgPU.m_BHeight && pu.m_BWidth == asgPU.m_BWidth)
                    {
                        asgLog << "\t Found matching PU: " << asgPU << std::endl;
                        m_Counters.m_testPUSizeMapASG.AddBlockCorrect(pu.m_BWidth, pu.m_BHeight);
                        return true;
                    }
                    else
                    {
                        asgLog << "\t PU mismatch -- FEI " << asgPU << ", ASG " << pu << std::endl;
                        return false;
                    }
                });
            }
        }
        else
        {
            asgLog << " -- NOT FOUND" << std::endl;
        }
    }
}

void Verifier::CompareMVs(const CTUDescriptor& asgCTUDescr, const CTUDescriptor& feiCTUDescr)
{
    if (m_InputParams.m_TestType & GENERATE_PREDICTION)
    {
        mfxU32 poolNo = 0;
        asgLog << "\nGenerated MVP pools:\n";
        for (const auto& genMVPPool : asgCTUDescr.m_MVPGenPools)
        {
            asgLog << "MVP pool no. : "<< poolNo++ << "\n";
            for (mfxU32 mvpIndex = 0; mvpIndex < MVP_PER_16x16_BLOCK; ++mvpIndex)
            {
                asgLog << mvpIndex << ": " << "L0:";
                std::stringstream sstreamL0;
                sstreamL0 << " (" << genMVPPool[mvpIndex].MV[0].x << "; " <<
                    genMVPPool[mvpIndex].MV[0].y << ")";
                asgLog << std::setw(14) << sstreamL0.str();

                asgLog << ", L1:";
                std::stringstream sstreamL1;
                sstreamL1 << " (" << genMVPPool[mvpIndex].MV[1].x << "; " <<
                    genMVPPool[mvpIndex].MV[1].y << ")";
                asgLog << std::setw(14) << sstreamL1.str();
                asgLog << "\n";
            }
        }
    }
    asgLog << "\nComparing MVs:\n";

    // Tool mapping MVs on 4x4 compare blocks for checking [numCompareBlocksInCtu][List]
    mfxU32 diffMaxBaseBlockLuma = m_InputParams.m_CTUStr.log2CTUSize - LOG2_MV_COMPARE_BASE_BLOCK_SIZE;
    mfxU32 numCompareBlocksInCtu = (1 << (2 * diffMaxBaseBlockLuma));

    std::vector<MVCompareBlock> MVCmpBlocksASG(numCompareBlocksInCtu);
    std::vector<MVCompareBlock> MVCmpBlocksFEI(numCompareBlocksInCtu);

    // Extract information from FEI output
    ExtractMVs(feiCTUDescr, MVCmpBlocksFEI);

    // Extract information from ASG tree structure
    ExtractMVs(asgCTUDescr, MVCmpBlocksASG);

    for (mfxU32 idxMVCmpBlock = 0; idxMVCmpBlock < numCompareBlocksInCtu; ++idxMVCmpBlock)
    {
        const MVCompareBlock& asgMVBlock = MVCmpBlocksASG[idxMVCmpBlock];
        const MVCompareBlock& feiMVBlock = MVCmpBlocksFEI[idxMVCmpBlock];

        asgLog << std::endl << "Block " << idxMVCmpBlock + 1 << "/" << numCompareBlocksInCtu << "; "
            << BaseBlock(asgMVBlock) << std::endl;

        CompareMVBlocks(asgMVBlock, feiMVBlock);
    }

    CalculateTotalMVCmpBlocksInCTU(MVCmpBlocksASG);
}

void Verifier::CompareMVBlocks(const MVCompareBlock& asgMVCmpBlock, const MVCompareBlock& feiMVCmpBlock)
{
    const auto feiMVL0 = feiMVCmpBlock.MV.GetL0MVTuple();
    const auto feiMVL1 = feiMVCmpBlock.MV.GetL1MVTuple();

    const auto asgMVL0 = asgMVCmpBlock.MV.GetL0MVTuple();
    const auto asgMVL1 = asgMVCmpBlock.MV.GetL1MVTuple();

    if (!asgMVCmpBlock.bIsIntra)
    {
        if ((m_InputParams.m_TestType & GENERATE_PREDICTION) && asgMVCmpBlock.usedMVPIndex < 0)
        {
            throw std::string("Verifier::CompareMVs: Some PU has invalid used MVP index");
        }

        if (m_InputParams.m_TestType & GENERATE_PREDICTION)
        {
            asgLog << "MVP index used in ASG: " << asgMVCmpBlock.usedMVPIndex << " ";
        }

        if (asgMVCmpBlock.predFlagL0 && !asgMVCmpBlock.predFlagL1)
        {
            asgLog << "Uni-predicted MV compare block\n";
            asgLog << "L0 MV: " << "FEI " << std::setw(14) << feiMVL0 << ", ASG " << std::setw(14) << asgMVL0
                << ", MVP pool no. : " << asgMVCmpBlock.usedMVPPoolNo << ", MVP index : " << asgMVCmpBlock.usedMVPIndex;

            if (feiMVL0 == asgMVL0)
            {
                m_Counters.m_correctMVCmpBlocksL0++;

                if (m_InputParams.m_TestType & GENERATE_PREDICTION)
                {
                    m_Counters.m_correctMVCmpBlocksL0PerMVPIndex[asgMVCmpBlock.usedMVPIndex]++;
                }

                asgLog << " -- OK\n";
            }
            else
            {
                asgLog << " -- MISMATCH\n";
            }
        }
        else if (!asgMVCmpBlock.predFlagL0 && asgMVCmpBlock.predFlagL1)
        {
            asgLog << "Uni-predicted MV compare block\n";
            asgLog << "L1 MV: " << "FEI " << std::setw(14) << feiMVL1 << ", ASG " << std::setw(14) << asgMVL1
                << ", MVP pool no. : " << asgMVCmpBlock.usedMVPPoolNo << ", MVP index : " << asgMVCmpBlock.usedMVPIndex;

            if (feiMVL1 == asgMVL1)
            {
                m_Counters.m_correctMVCmpBlocksL1++;

                if (m_InputParams.m_TestType & GENERATE_PREDICTION)
                {
                    m_Counters.m_correctMVCmpBlocksL1PerMVPIndex[asgMVCmpBlock.usedMVPIndex]++;
                }

                asgLog << " -- OK\n";
            }
            else
            {
                asgLog << " -- MISMATCH\n";
            }
        }
        else if (asgMVCmpBlock.predFlagL0 && asgMVCmpBlock.predFlagL1)
        {
            asgLog << "Bi-predicted MV compare block\n";
            asgLog << "L0 MV: " << "FEI " << std::setw(14) << feiMVL0 << ", ASG "
                << std::setw(14) << asgMVL0 << "\n";
            asgLog << "L1 MV: " << "FEI " << std::setw(14) << feiMVL1 << ", ASG "
                << std::setw(14) << asgMVL1 << ", MVP pool no. : " << asgMVCmpBlock.usedMVPPoolNo
                << ", MVP index : " << asgMVCmpBlock.usedMVPIndex;

            if ((feiMVL0 == asgMVL0) && (feiMVL1 == asgMVL1))
            {
                m_Counters.m_correctMVCmpBlocksBi++;

                if (m_InputParams.m_TestType & GENERATE_PREDICTION)
                {
                    m_Counters.m_correctMVCmpBlocksBiPerMVPIndex[asgMVCmpBlock.usedMVPIndex]++;
                }

                asgLog << " -- OK\n";
            }
            else
            {
                asgLog << " -- MISMATCH\n";
            }

            // Won't affect test result
            if ((feiMVL0 == asgMVL0) && (feiMVL1 != asgMVL1))
            {
                m_Counters.m_correctMVsL0FromBiMVCmpBlocks++;
            }
            else if ((feiMVL0 != asgMVL0) && (feiMVL1 == asgMVL1))
            {
                m_Counters.m_correctMVsL1FromBiMVCmpBlocks++;
            }
        }
    }
    else
    {
        asgLog << " -- skipped due to being intra in ASG" << std::endl;
    }
}

void Verifier::CalculateTotalMVCmpBlocksInCTU(std::vector<MVCompareBlock>& mvCmpBlocks)
{
    mfxU32 testMVCmpBlocksCountL0 = (mfxU32)std::count_if(mvCmpBlocks.begin(), mvCmpBlocks.end(),
        [](const MVCompareBlock& block) { return !block.bIsIntra && block.predFlagL0 && !block.predFlagL1; });

    mfxU32 testMVCmpBlocksCountL1 = (mfxU32)std::count_if(mvCmpBlocks.begin(), mvCmpBlocks.end(),
        [](const MVCompareBlock& block) { return !block.bIsIntra && !block.predFlagL0 && block.predFlagL1; });

    mfxU32 testMVCmpBlocksCountBi = (mfxU32)std::count_if(mvCmpBlocks.begin(), mvCmpBlocks.end(),
        [](const MVCompareBlock& block) { return !block.bIsIntra && block.predFlagL0 && block.predFlagL1; });

    m_Counters.m_totalMVCmpBlocksL0 += testMVCmpBlocksCountL0;
    m_Counters.m_totalMVCmpBlocksL1 += testMVCmpBlocksCountL1;
    m_Counters.m_totalMVCmpBlocksBi += testMVCmpBlocksCountBi;

    if (m_InputParams.m_TestType & GENERATE_PREDICTION)
    {
        for (mfxI32 mvpIndex = 0; mvpIndex < MVP_PER_16x16_BLOCK; mvpIndex++)
        {
            mfxU32 testMVCmpBlocksCountL0 = (mfxU32)std::count_if(mvCmpBlocks.begin(), mvCmpBlocks.end(),
                [mvpIndex](const MVCompareBlock& block) { return !block.bIsIntra && block.predFlagL0
                && !block.predFlagL1 && block.usedMVPIndex == mvpIndex; });

            mfxU32 testMVCmpBlocksCountL1 = (mfxU32)std::count_if(mvCmpBlocks.begin(), mvCmpBlocks.end(),
                [mvpIndex](const MVCompareBlock& block) { return !block.bIsIntra && !block.predFlagL0
                && block.predFlagL1 && block.usedMVPIndex == mvpIndex; });

            mfxU32 testMVCmpBlocksCountBi = (mfxU32)std::count_if(mvCmpBlocks.begin(), mvCmpBlocks.end(),
                [mvpIndex](const MVCompareBlock& block) { return !block.bIsIntra && block.predFlagL0
                && block.predFlagL1 && block.usedMVPIndex == mvpIndex; });

            m_Counters.m_totalMVCmpBlocksL0PerMVPIndex[mvpIndex] += testMVCmpBlocksCountL0;
            m_Counters.m_totalMVCmpBlocksL1PerMVPIndex[mvpIndex] += testMVCmpBlocksCountL1;
            m_Counters.m_totalMVCmpBlocksBiPerMVPIndex[mvpIndex] += testMVCmpBlocksCountBi;
        }
    }
}

void Verifier::CountExactMatches(const CTUDescriptor& asgCTUDescr, const CTUDescriptor& feiCTUDescr)
{
    std::vector<PUBlock> asgPUBlocksOfCTU = asgCTUDescr.GetTotalPUsVec();
    std::vector<PUBlock> feiPUBlocksOfCTU = feiCTUDescr.GetTotalPUsVec();

    mfxU32 exactPUMatches = 0;

    for (const auto& asgPU : asgPUBlocksOfCTU)
    {
        exactPUMatches += (mfxU32)std::count(feiPUBlocksOfCTU.begin(), feiPUBlocksOfCTU.end(), asgPU);
    }

    mfxU32 exactCUMatches = 0;
    //TODO: optimize so that PUs don't get compared twice
    for (const auto& asgCU : asgCTUDescr.m_CUVec)
    {
        exactCUMatches += (mfxU32)std::count(feiCTUDescr.m_CUVec.begin(), feiCTUDescr.m_CUVec.end(), asgCU);
    }

    if (exactCUMatches == asgCTUDescr.m_CUVec.size() && exactCUMatches == feiCTUDescr.m_CUVec.size() &&
        exactPUMatches == asgPUBlocksOfCTU.size() &&
        exactPUMatches == feiPUBlocksOfCTU.size())
    {
        asgLog << std::endl << "CTUs are exactly matching" << std::endl;
        m_Counters.m_exactCTUs++;
    }
    else
    {
        asgLog << std::endl << "CTUs are not exactly matching" << std::endl;
        asgLog << "CUs (exact/ASG total): " << exactCUMatches << '/' << asgPUBlocksOfCTU.size() << std::endl;
        asgLog << "PUs (exact/ASG total): " << exactPUMatches << '/' << asgPUBlocksOfCTU.size() << std::endl;
    }
    m_Counters.m_exactCUs += exactCUMatches;
    m_Counters.m_exactPUs += exactPUMatches;
}

CTUDescriptor Verifier::ConvertFeiOutInLocalStr(const mfxFeiHevcPakCtuRecordV0& ctuPakFromFEI, const ExtendedBuffer& tmpCuBuff, const mfxU32 startCuOffset)
{
    try
    {
        CTUDescriptor tmpDescr(ctuPakFromFEI.CtuAddrX, ctuPakFromFEI.CtuAddrY,
            ctuPakFromFEI.CtuAddrX * m_InputParams.m_CTUStr.CTUSize, ctuPakFromFEI.CtuAddrY * m_InputParams.m_CTUStr.CTUSize,
            m_InputParams.m_CTUStr.CTUSize, m_InputParams.m_CTUStr.CTUSize);

        mfxU32 bitMask = 0;
        mfxU32 baseSplitLevel2Shift = 4;
        bitMask |= ctuPakFromFEI.SplitLevel0;
        bitMask |= ctuPakFromFEI.SplitLevel1 << 1;
        bitMask |= ctuPakFromFEI.SplitLevel2Part0 << (baseSplitLevel2Shift + 1);
        bitMask |= ctuPakFromFEI.SplitLevel2Part1 << (2 * baseSplitLevel2Shift + 1);
        bitMask |= ctuPakFromFEI.SplitLevel2Part2 << (3 * baseSplitLevel2Shift + 1);
        bitMask |= ctuPakFromFEI.SplitLevel2Part3 << (4 * baseSplitLevel2Shift + 1);

        m_FrameProcessor.GenQuadTreeInCTUWithBitMask(tmpDescr, bitMask);
        std::vector<BaseBlock> tmpVec;
        tmpDescr.m_CUQuadTree.GetQuadTreeBlocksRecur(tmpDescr.m_CUQuadTree.root, tmpDescr.m_AdrX, tmpDescr.m_AdrY, tmpDescr.m_BHeight, tmpVec);

        if (tmpVec.size() != mfxU32(ctuPakFromFEI.CuCountMinus1 + 1))
            throw std::string("ERROR: Verifier::ConvertFeiOutInLocalStr. Incorrect number of the CUs into tree");

        mfxU32 idxCUInCTU = 0;
        for (const auto& block : tmpVec)
        {
            CUBlock cu_block(block);
            mfxFeiHevcPakCuRecordV0 cuPakFromFEI = tmpCuBuff.m_pakCuRecord.Data[startCuOffset + idxCUInCTU];

            if (cuPakFromFEI.PredMode)
            {
                cu_block.m_PredType = INTER_PRED;
                cu_block.BuildPUsVector((INTER_PART_MODE)cuPakFromFEI.PartMode);

                mfxU32 idxPUInCU = 0;
                for (auto& PU : cu_block.m_PUVec)
                {
                    mfxU8 interpredIdcPU = (cuPakFromFEI.InterpredIdc >> (2 * idxPUInCU)) & 0x03;

                    switch (interpredIdcPU)
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
                    case 3:
                        throw std::string("ERROR: Verifier::ConvertFeiOutInLocalStr: incorrect value for InterpredIdc");
                    }

                    for (mfxU32 idxList = 0; idxList < 2; ++idxList)
                    {
                        PU.m_MV.MV[idxList].x = cuPakFromFEI.MVs[idxList].x[idxPUInCU];
                        PU.m_MV.MV[idxList].y = cuPakFromFEI.MVs[idxList].y[idxPUInCU];
                    }

                    switch (idxPUInCU)
                    {
                    case 0:
                        PU.m_MV.RefIdx.RefL0 = cuPakFromFEI.RefIdx[0].Ref0;
                        PU.m_MV.RefIdx.RefL1 = cuPakFromFEI.RefIdx[1].Ref0;
                        break;
                    case 1:
                        PU.m_MV.RefIdx.RefL0 = cuPakFromFEI.RefIdx[0].Ref1;
                        PU.m_MV.RefIdx.RefL1 = cuPakFromFEI.RefIdx[1].Ref1;
                        break;
                    case 2:
                        PU.m_MV.RefIdx.RefL0 = cuPakFromFEI.RefIdx[0].Ref2;
                        PU.m_MV.RefIdx.RefL1 = cuPakFromFEI.RefIdx[1].Ref2;
                        break;
                    case 3:
                        PU.m_MV.RefIdx.RefL0 = cuPakFromFEI.RefIdx[0].Ref3;
                        PU.m_MV.RefIdx.RefL1 = cuPakFromFEI.RefIdx[1].Ref3;
                        break;
                    }

                    ++idxPUInCU;
                }
            }
            else
            {
                cu_block.m_PredType = INTRA_PRED;
            }

            tmpDescr.m_CUVec.push_back(cu_block);
            ++idxCUInCTU;
        }

        return tmpDescr;
    }

    catch (std::string& e) {
        std::cout << e << std::endl;
        throw std::string("ERROR: Verifier::ConvertFeiOutInLocalStr");
    }
}

void Verifier::CheckStatistics(Thresholds thres)
{
    m_Counters.m_totalCUsASG = m_Counters.m_testCUSizeMapASG.GetTotalCount();
    m_Counters.m_totalPUsASG = m_Counters.m_testPUSizeMapASG.GetTotalCount();

    m_Counters.m_totalCUsFEI = m_Counters.m_testCUSizeMapFEI.GetTotalCount();
    m_Counters.m_totalPUsFEI = m_Counters.m_testPUSizeMapFEI.GetTotalCount();

    m_Counters.m_correctCUs = m_Counters.m_testCUSizeMapASG.GetCorrectCount();
    m_Counters.m_correctPUs = m_Counters.m_testPUSizeMapASG.GetCorrectCount();

    std::cout << "Total CTUs tested: " << m_Counters.m_testCTUs << std::endl;
    std::cout << "Exact matches: CTUs " << m_Counters.m_exactCTUs << '/' << m_Counters.m_testCTUs << '(';
    CheckLowerThreshold(m_Counters.m_exactCTUs, m_Counters.m_testCTUs);
    std::cout << "), CUs " << m_Counters.m_exactCUs << '/' << m_Counters.m_totalCUsASG << '(';
    CheckLowerThreshold(m_Counters.m_exactCUs, m_Counters.m_totalCUsASG);
    std::cout << "), PUs " << m_Counters.m_exactPUs << '/' << m_Counters.m_totalPUsASG << '(';
    CheckLowerThreshold(m_Counters.m_exactPUs, m_Counters.m_totalPUsASG);
    std::cout << ")" << std::endl;

    if (m_InputParams.m_TestType & GENERATE_INTER)
    {
        CheckMVs(thres);
    }

    if (m_InputParams.m_TestType & (GENERATE_INTER | GENERATE_INTRA))
    {
        CheckSplits(thres);
    }

    if (m_InputParams.m_TestType == GENERATE_PICSTRUCT)
    {
        CheckPicStruct();
    }

    if (!m_errorMsgList.empty())
    {
        std::cout << std::endl <<"TEST FAILED" << std::endl;

        //Output errors in order of occurence as the last lines of output
        //so that the test system recognizes test failure
        for (auto& message : m_errorMsgList)
        {
            std::cout << message << std::endl;
        }

        throw ASG_HEVC_ERR_LOW_PASS_RATE;
    }
}

bool Verifier::CheckLowerThreshold(mfxU32 numerator, mfxU32 denominator, mfxU32 threshold)
{
    if (denominator > 0)
    {
        std::cout.precision(1);
        std::cout.setf(std::ios::fixed);
        std::cout << 100 * ((mfxF32)numerator / denominator) << "%";
        return (100 * ((mfxF32)numerator / denominator) >= (mfxF32)threshold);
    }
    else
    {
        std::cout << "n/a";
        return true;
    }
}

void Verifier::PrintPercentRatio(mfxU32 numerator, mfxU32 denominator)
{
    if (denominator > 0)
    {
        std::cout.precision(1);
        std::cout.setf(std::ios::fixed);
        std::cout << 100 * ((mfxF32)numerator / denominator) << "%";
    }
    else
    {
        std::cout << "n/a";
    }
}

void Verifier::CheckMVs(Thresholds threshold)
{
    std::cout << std::endl << "MOTION VECTOR TESTING:\n";

    CheckL0MVs(threshold);

    CheckL1MVs(threshold);

    CheckBiMVs(threshold);
}

void Verifier::CheckL0MVs(Thresholds threshold)
{
    if (m_InputParams.m_TestType & GENERATE_PREDICTION)
    {
        CheckL0MVsPerMVPIndex(threshold);
    }
    else
    {
        std::cout << "Correct uni-predicted (L0) MV " << MV_CMP_BLOCK_SIZE << "x" << MV_CMP_BLOCK_SIZE << " compare blocks: "
            << m_Counters.m_correctMVCmpBlocksL0 << '/' << m_Counters.m_totalMVCmpBlocksL0 << " (";

        bool isOverallCheckPassed = CheckLowerThreshold(m_Counters.m_correctMVCmpBlocksL0,
            m_Counters.m_totalMVCmpBlocksL0, threshold.mvThres);

        std::cout << "); threshold: " << threshold.mvThres << '%' << std::endl;

        if (isOverallCheckPassed)
        {
            std::cout << "MVs[Uni, L0] test passed" << std::endl;
        }
        else
        {
            m_errorMsgList.emplace_back("ERROR: Low MV pass-rate for uni-predicted L0 blocks");
        }
    }
}

void Verifier::CheckL0MVsPerMVPIndex(Thresholds threshold)
{
    bool isIncorrectPredCheckPassed = true;
    bool isCorrectPredCheckPassed = true;

    mfxU32 expectedCorrectTotal = 0;
    mfxU32 expectedCorrectCount = 0;
    mfxU32 expectedIncorrectTotal = 0;
    mfxU32 expectedIncorrectCount = 0;

    for (mfxU32 mvpIndex = 0; mvpIndex < MVP_PER_16x16_BLOCK; mvpIndex++)
    {
        std::cout << "MVP index: " << mvpIndex << " Correct uni-predicted (L0) MV "
            << MV_CMP_BLOCK_SIZE << "x" << MV_CMP_BLOCK_SIZE << " compare blocks: "
            << m_Counters.m_correctMVCmpBlocksL0PerMVPIndex[mvpIndex] << '/'
            << m_Counters.m_totalMVCmpBlocksL0PerMVPIndex[mvpIndex] << " (";
        PrintPercentRatio(m_Counters.m_correctMVCmpBlocksL0PerMVPIndex[mvpIndex],
            m_Counters.m_totalMVCmpBlocksL0PerMVPIndex[mvpIndex]);
        std::cout << ")" << std::endl;

        // Calculating incorrectly predicted blocks per disabled MVP indexes
        if (mvpIndex / m_InputParams.m_NumMVPredictors)
        {
            expectedIncorrectTotal += m_Counters.m_totalMVCmpBlocksL0PerMVPIndex[mvpIndex];
            expectedIncorrectCount += (m_Counters.m_totalMVCmpBlocksL0PerMVPIndex[mvpIndex]
                - m_Counters.m_correctMVCmpBlocksL0PerMVPIndex[mvpIndex]);
        }
        else // Calculating correctly predicted blocks per enabled MVP indexes
        {
            expectedCorrectTotal += m_Counters.m_totalMVCmpBlocksL0PerMVPIndex[mvpIndex];
            expectedCorrectCount += m_Counters.m_correctMVCmpBlocksL0PerMVPIndex[mvpIndex];
        }
    }

    std::cout << "Correctly   uni-predicted (L0) MV "
        << MV_CMP_BLOCK_SIZE << "x" << MV_CMP_BLOCK_SIZE << " compare blocks on enabled  EMVP: "
        << expectedCorrectCount << "/" << expectedCorrectTotal << " (";
    isCorrectPredCheckPassed = CheckLowerThreshold(expectedCorrectCount, expectedCorrectTotal,
        threshold.mvThres);
    std::cout << "); threshold: " << threshold.mvThres << '%' << std::endl;

    if (!isCorrectPredCheckPassed)
    {
        m_errorMsgList.emplace_back("ERROR: Low MV pass-rate for correctly uni-predicted L0 blocks");
    }

    std::cout << "Incorrectly uni-predicted (L0) MV "
        << MV_CMP_BLOCK_SIZE << "x" << MV_CMP_BLOCK_SIZE << " compare blocks on disabled EMVP: "
        << expectedIncorrectCount << "/" << expectedIncorrectTotal << " (";
    isIncorrectPredCheckPassed = CheckLowerThreshold(expectedIncorrectCount, expectedIncorrectTotal,
        threshold.mvThres);
    std::cout << "); threshold: " << threshold.mvThres << '%' << std::endl;

    if (!isIncorrectPredCheckPassed)
    {
        m_errorMsgList.emplace_back("ERROR: Low MV pass-rate for incorrectly uni-predicted L0 blocks");
    }

    if (isCorrectPredCheckPassed && isIncorrectPredCheckPassed)
    {
        std::cout << "MVs[Uni, L0] test passed" << std::endl;
    }
}

void Verifier::CheckL1MVs(Thresholds threshold)
{
    if (m_InputParams.m_TestType & GENERATE_PREDICTION)
    {
        CheckL1MVsPerMVPIndex(threshold);
    }
    else
    {
        std::cout << "Correct uni-predicted (L1) MV " << MV_CMP_BLOCK_SIZE << "x" << MV_CMP_BLOCK_SIZE
            << " compare blocks: " << m_Counters.m_correctMVCmpBlocksL1 << '/'
            << m_Counters.m_totalMVCmpBlocksL1 << " (";

        bool isOverallPassed = CheckLowerThreshold(m_Counters.m_correctMVCmpBlocksL1,
            m_Counters.m_totalMVCmpBlocksL1, threshold.mvThres);

        std::cout << "); threshold: " << threshold.mvThres << '%' << std::endl;

        if (isOverallPassed)
        {
            std::cout << "MVs[Uni, L1] test passed" << std::endl;
        }
        else
        {
            m_errorMsgList.emplace_back("ERROR: Low MV pass-rate for uni-predicted L1 blocks");
        }
    }
}

void Verifier::CheckL1MVsPerMVPIndex(Thresholds threshold)
{
    bool isIncorrectPredCheckPassed = true;
    bool isCorrectPredCheckPassed = true;

    mfxU32 expectedCorrectTotal   = 0;
    mfxU32 expectedCorrectCount   = 0;
    mfxU32 expectedIncorrectTotal = 0;
    mfxU32 expectedIncorrectCount = 0;

    std::cout << std::endl;

    for (mfxU32 mvpIndex = 0; mvpIndex < MVP_PER_16x16_BLOCK; mvpIndex++)
    {
        std::cout << "MVP index: " << mvpIndex << " Correct uni-predicted (L1) MV "
            << MV_CMP_BLOCK_SIZE << "x" << MV_CMP_BLOCK_SIZE << " compare blocks: "
            << m_Counters.m_correctMVCmpBlocksL1PerMVPIndex[mvpIndex] << '/'
            << m_Counters.m_totalMVCmpBlocksL1PerMVPIndex[mvpIndex] << " (";
        PrintPercentRatio(m_Counters.m_correctMVCmpBlocksL1PerMVPIndex[mvpIndex],
            m_Counters.m_totalMVCmpBlocksL1PerMVPIndex[mvpIndex]);
        std::cout << ")" << std::endl;

        // Calculating incorrectly predicted blocks per disabled MVP indexes
        if (mvpIndex / m_InputParams.m_NumMVPredictors)
        {
            expectedIncorrectTotal += m_Counters.m_totalMVCmpBlocksL1PerMVPIndex[mvpIndex];
            expectedIncorrectCount += (m_Counters.m_totalMVCmpBlocksL1PerMVPIndex[mvpIndex]
                - m_Counters.m_correctMVCmpBlocksL1PerMVPIndex[mvpIndex]);
        }
        else // Calculating correctly predicted blocks per enabled MVP indexes
        {
            expectedCorrectTotal += m_Counters.m_totalMVCmpBlocksL1PerMVPIndex[mvpIndex];
            expectedCorrectCount += m_Counters.m_correctMVCmpBlocksL1PerMVPIndex[mvpIndex];
        }
    }

    std::cout << "Correctly   uni-predicted (L1) MV "
        << MV_CMP_BLOCK_SIZE << "x" << MV_CMP_BLOCK_SIZE << " compare blocks on enabled  EMVP: "
        << expectedCorrectCount << "/" << expectedCorrectTotal << " (";
    isCorrectPredCheckPassed = CheckLowerThreshold(expectedCorrectCount,
        expectedCorrectTotal, threshold.mvThres);
    std::cout << "); threshold: " << threshold.mvThres << '%' << std::endl;

    if (!isCorrectPredCheckPassed)
    {
        m_errorMsgList.emplace_back("ERROR: Low MV pass-rate for correctly uni-predicted L1 blocks");
    }

    std::cout << "Incorrectly uni-predicted (L1) MV "
        << MV_CMP_BLOCK_SIZE << "x" << MV_CMP_BLOCK_SIZE << " compare blocks on disabled EMVP: "
        << expectedIncorrectCount << "/" << expectedIncorrectTotal << " (";
    isIncorrectPredCheckPassed = CheckLowerThreshold(expectedIncorrectCount,
        expectedIncorrectTotal, threshold.mvThres);
    std::cout << "); threshold: " << threshold.mvThres << '%' << std::endl;

    if (!isIncorrectPredCheckPassed)
    {
        m_errorMsgList.emplace_back("ERROR: Low MV pass-rate for incorrectly uni-predicted L1 blocks");
    }

    if (isCorrectPredCheckPassed && isIncorrectPredCheckPassed)
    {
        std::cout << "MVs[Uni, L1] test passed" << std::endl;
    }
}

void Verifier::CheckBiMVs(Thresholds threshold)
{
    if (m_InputParams.m_TestType & GENERATE_PREDICTION)
    {
        CheckBiMVsPerMVPIndex(threshold);
    }
    else
    {
        std::cout << "Correct bi-predicted (Bi) MVs " << MV_CMP_BLOCK_SIZE << "x"
            << MV_CMP_BLOCK_SIZE << " compare blocks: "
            << m_Counters.m_correctMVCmpBlocksBi << '/' << m_Counters.m_totalMVCmpBlocksBi << " (";

        bool isOverallPassed = CheckLowerThreshold(m_Counters.m_correctMVCmpBlocksBi,
            m_Counters.m_totalMVCmpBlocksBi, threshold.mvThres);

        std::cout << "); threshold: " << threshold.mvThres << '%' << std::endl;

        if (isOverallPassed)
        {
            std::cout << "MVs[Bi] test passed" << std::endl;
        }
        else
        {
            m_errorMsgList.emplace_back("ERROR: Low MV pass-rate for bi-predicted blocks");
        }
    }

    // Won't affect test result
    std::cout << std::endl << "Bi MVs with correct L0 and incorrect L1: "
        << m_Counters.m_correctMVsL0FromBiMVCmpBlocks << std::endl;
    std::cout << "Bi MVs with correct L1 and incorrect L0: "
        << m_Counters.m_correctMVsL1FromBiMVCmpBlocks << std::endl;
}

void Verifier::CheckBiMVsPerMVPIndex(Thresholds threshold)
{
    bool isIncorrectPredCheckPassed = true;
    bool isCorrectPredCheckPassed = true;

    mfxU32 expectedCorrectTotal = 0;
    mfxU32 expectedCorrectCount = 0;
    mfxU32 expectedIncorrectTotal = 0;
    mfxU32 expectedIncorrectCount = 0;

    std::cout << std::endl;

    for (mfxU32 mvpIndex = 0; mvpIndex < MVP_PER_16x16_BLOCK; mvpIndex++)
    {
        std::cout << "MVP index: " << mvpIndex << " Correct bi-predicted (Bi) MVs "
            << MV_CMP_BLOCK_SIZE << "x" << MV_CMP_BLOCK_SIZE << " compare blocks: "
            << m_Counters.m_correctMVCmpBlocksBiPerMVPIndex[mvpIndex] << '/'
            << m_Counters.m_totalMVCmpBlocksBiPerMVPIndex[mvpIndex] << " (";
        PrintPercentRatio(m_Counters.m_correctMVCmpBlocksBiPerMVPIndex[mvpIndex],
            m_Counters.m_totalMVCmpBlocksBiPerMVPIndex[mvpIndex]);
        std::cout << ")" << std::endl;

        if (mvpIndex / m_InputParams.m_NumMVPredictors)
        {
            expectedIncorrectTotal += m_Counters.m_totalMVCmpBlocksBiPerMVPIndex[mvpIndex];
            expectedIncorrectCount += (m_Counters.m_totalMVCmpBlocksBiPerMVPIndex[mvpIndex]
                - m_Counters.m_correctMVCmpBlocksBiPerMVPIndex[mvpIndex]);
        }
        else
        {
            expectedCorrectTotal += m_Counters.m_totalMVCmpBlocksBiPerMVPIndex[mvpIndex];
            expectedCorrectCount += m_Counters.m_correctMVCmpBlocksBiPerMVPIndex[mvpIndex];
        }
    }

    std::cout << "Correctly   bi-predicted (Bi) MV "
        << MV_CMP_BLOCK_SIZE << "x" << MV_CMP_BLOCK_SIZE << " compare blocks on enabled  EMVP: "
        << expectedCorrectCount << "/" << expectedCorrectTotal << " (";
    isCorrectPredCheckPassed = CheckLowerThreshold(expectedCorrectCount,
        expectedCorrectTotal, threshold.mvThres);
    std::cout << "); threshold: " << threshold.mvThres << '%' << std::endl;

    if (!isCorrectPredCheckPassed)
    {
        m_errorMsgList.emplace_back("ERROR: Low MV pass-rate for correctly bi-predicted blocks");
    }

    std::cout << "Incorrectly bi-predicted (Bi) MV "
        << MV_CMP_BLOCK_SIZE << "x" << MV_CMP_BLOCK_SIZE << " compare blocks on disabled EMVP: "
        << expectedIncorrectCount << "/" << expectedIncorrectTotal << " (";
    isIncorrectPredCheckPassed = CheckLowerThreshold(expectedIncorrectCount,
        expectedIncorrectTotal, threshold.mvThres);
    std::cout << "); threshold: " << threshold.mvThres << '%' << std::endl;

    if (!isIncorrectPredCheckPassed)
    {
        m_errorMsgList.emplace_back("ERROR: Low MV pass-rate for incorrect bi-predicted blocks");
    }

    if (isCorrectPredCheckPassed && isIncorrectPredCheckPassed)
    {
        std::cout << "MVs[Bi] test passed" << std::endl;
    }
}

void Verifier::CheckSplits(Thresholds threshold)
{
    bool isStagePassed = true;

    std::cout << std::endl << "SPLIT TESTING (PU-based):";

    std::cout << std::endl << "Total FEI CU partitions by size (for test CTUs):\n";
    m_Counters.m_testCUSizeMapFEI.PrintTable();
    std::cout << "Total FEI PU partitions by size (for test CTUs):\n";
    m_Counters.m_testPUSizeMapFEI.PrintTable();

    std::cout << "Correct FEI CU partitions by size (actual/maximum):\n";
    m_Counters.m_testCUSizeMapASG.PrintTable();
    std::cout << "Correct FEI PU partitions by size (actual/maximum):\n";
    m_Counters.m_testPUSizeMapASG.PrintTable();

    std::cout << "\nTotal partition counts: FEI CUs -- " << m_Counters.m_totalCUsFEI
        << ", FEI PUs -- " << m_Counters.m_totalPUsFEI << ", ASG CUs -- " << m_Counters.m_totalCUsASG
        << ", ASG PUs -- " << m_Counters.m_totalPUsASG << std::endl;

    std::cout << "Correct partition counts: CUs -- " << m_Counters.m_correctCUs
        << '/' << m_Counters.m_totalCUsASG << "(";

    CheckLowerThreshold(m_Counters.m_correctCUs, m_Counters.m_totalCUsASG);

    std::cout << "), PUs -- " << m_Counters.m_correctPUs << '/' << m_Counters.m_totalPUsASG
        << "(";

    isStagePassed = CheckLowerThreshold(m_Counters.m_correctPUs, m_Counters.m_totalPUsASG, threshold.splitThres);

    std::cout << "); PU threshold: " << threshold.splitThres << '%' << std::endl;

    if (isStagePassed)
    {
        std::cout << "Split test passed" << std::endl;
    }
    else
    {
        m_errorMsgList.emplace_back("ERROR: Low pass-rate for splits");
    }
}

void Verifier::CheckPicStruct()
{
    if (m_Counters.m_totalPics == m_Counters.m_correctRecords)
    {
        std::cout << "Picture Structure test passed" << std::endl;
    }
    else
    {
        m_errorMsgList.emplace_back("ERROR: Picture Structure test failed");
    }
}

#endif // MFX_VERSION
