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

#ifndef __ASG_HEVC_MVMVP_PROCESSOR_H__
#define __ASG_HEVC_MVMVP_PROCESSOR_H__

#include "mfxvideo.h"

#if MFX_VERSION >= MFX_VERSION_NEXT

#include <functional>
#include <vector>
#include <unordered_map>

#include "block_structures.h"
#include "frame_change_descriptor.h"
#include "random_generator.h"

class MVMVPProcessor
{
public:
    MVMVPProcessor() = delete;

    MVMVPProcessor(mfxU32 extGenMVPBlockSize, mfxU16 extSubPelMode) :
        m_GenMVPBlockSize(extGenMVPBlockSize),
        m_SubPelMode(extSubPelMode), m_DoUseIntra(false) {}

    // Initiates an MVMVPProcessor with data from CTU
    void InitMVPGridData(const CTUDescriptor& CTU, const FrameChangeDescriptor & frameDescr);

    bool GenValidMVMVPForPU(PUBlock & PU, const FrameChangeDescriptor & frameDescr);

    bool GenValidMVForPU(PUBlock & PU, const FrameChangeDescriptor & frameDescr);

    void FillFrameMVPExtBuffer(FrameChangeDescriptor& frameDescr);

    void GetMVPPools(std::vector<std::vector<PUMotionVector>> &outMVPPools);

private:

    //Structure representing a single MVP block
    struct MVPBlock : BaseBlock
    {
        MVPBlock(mfxU32 adrX = 0, mfxU32 adrY = 0, mfxU32 bWidth = 0, mfxU32 bHeight = 0) :
            BaseBlock(adrX, adrY, bWidth, bHeight), mvpPoolGroupNo(MVP_INVALID_POOL_GROUP) {}

        // No. of generation MVP pool group
        // For each two MVP blocks from the same group
        // MVP pools should be equal
        mfxI32 mvpPoolGroupNo = MVP_INVALID_POOL_GROUP;

        bool IsAlreadyInGroup() const { return mvpPoolGroupNo != MVP_INVALID_POOL_GROUP; }

        bool IsInGroup(mfxI32 extGroupNo) const { return mvpPoolGroupNo == extGroupNo; }

        void SetGroup(mfxI32 extGroupNo) { mvpPoolGroupNo = extGroupNo; }

        mfxI32 GetGroupNo() const { return mvpPoolGroupNo; }
    };

    static constexpr mfxI32 MVP_INVALID_POOL_GROUP = -1;
    mfxU32 m_GenMVPBlockSize = 0;
    mfxU16 m_SubPelMode = 0;

    std::vector<MVPBlock> m_mvpBlockGrid;

    // Type to store 32x32 MVP blocks. First value is actual BaseBlock
    // Second one is no. (m_mvpBlockGrid) of left upper 16x16 block in the group
    typedef std::pair<BaseBlock, mfxU32> MVP32x32BlockGroup;

    std::vector<std::vector<PUMotionVector>> m_mvpPools;

    // Checking test case type
    bool m_DoUseIntra = false;

    // Custom hasher used in unordered_map
    struct BaseBlockHasher
    {
        std::size_t operator() (const BaseBlock& key) const
        {
            return std::hash<mfxU32>()(key.m_AdrX) ^ ((std::hash<mfxU32>()(key.m_AdrY) << 1) >> 1)
                ^ (std::hash<mfxU32>()(key.m_BHeight) << 1) ^ (std::hash<mfxU32>()(key.m_BWidth) >> 1);
        }
    };

    std::unordered_map<BaseBlock, mfxI32, BaseBlockHasher> m_PUtoMVPPoolGroupMap;

    PUMotionVector GenerateMVP(const CTUDescriptor& CTU, const FrameChangeDescriptor& frameChangeDescr);

    PUMotionVector GenerateMV(const FrameChangeDescriptor& frameDescr);

    mfxI32 GetRandomMVComponent(mfxI32 lower, mfxI32 upper);

    // Functions used to put all PUs in CTU and MVP blocks in the correct MVP pool groups
    // according to INTER splitting and MVPBlockSize parameter
    // All these functions return actual number of MVP pool groups to generate
    mfxI32 ConstructMVPPoolGroups(const CTUDescriptor& CTU);

    mfxI32 PutPUAndMVPBlocksIn16x16MVPPoolGroups(const CTUDescriptor& CTU);
    mfxI32 PutPUAndMVPBlocksIn32x32MVPPoolGroups(const CTUDescriptor& CTU);
    mfxI32 PutPUAndMVPBlocksInSingleMVPPoolGroup(const CTUDescriptor& CTU);

    // Functions used in MVP Ext buffer dumping

    // Gets offset in mfxExtFeiHevcEncMVPredictors::Data buffer for the provided MVP block element
    mfxU32 CalculateOffsetInMVPredictorsBuffer(mfxU32 bufferPitch, const MVPBlock& mvpBlock);

    // Fill one mfxFeiHevcEncMVPredictors element in mfxExtFeiHevcEncMVPredictors::Data buffer
    // associated with provided MVP block
    void PutMVPIntoExtBuffer(const MVPBlock& mvpBlock, mfxExtFeiHevcEncMVPredictors* outputBuf);
};

#endif // MFX_VERSION

#endif // __ASG_HEVC_MVMVP_PROCESSOR_H__
