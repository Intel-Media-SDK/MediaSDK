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

#ifndef __BLOCK_STRUCTURES_H__
#define __BLOCK_STRUCTURES_H__

#include "mfxvideo.h"

#if MFX_VERSION >= MFX_VERSION_NEXT

#include "inter_test.h"
#include "intra_test.h"
#include "util_defs.h"

enum COLOR_COMPONENT {
    LUMA_Y = 0,
    CHROMA_U = 1,
    CHROMA_V = 2
};

enum PRED_TYPE {
    INTRA_PRED = 0,
    INTER_PRED = 1,
};

struct RefSampleAvail
{
    //parameter answers the question whether samples in the extention of left border to the bottom are final ones
    //and won't be overwritten by any following blocks
    bool LeftDown;
    //parameter answers the question whether samples in the extention of above border to the right are final ones
    //and won't be overwritten by any following blocks
    bool UpRight;
    RefSampleAvail(bool leftdown = true, bool upright = true) : LeftDown(leftdown), UpRight(upright) {}
};

//base block structure with size and coordinates
struct BaseBlock {
    mfxU32 m_AdrX;
    mfxU32 m_AdrY;
    mfxU32 m_BWidth;
    mfxU32 m_BHeight;

    BaseBlock(mfxU32 adrX = 0, mfxU32 adrY = 0, mfxU32 bWidth = 0, mfxU32 bHeight = 0)
        : m_AdrX(adrX),
        m_AdrY(adrY),
        m_BWidth(bWidth),
        m_BHeight(bHeight)
    {}

    // Returns true if two rectangles intersect or closer to each other than specified by x_sp and y_sp
    bool CheckForIntersect(const BaseBlock& other_block, mfxU32 x_sp = 0, mfxU32 y_sp = 0) const
    {
        return !((m_AdrX >= other_block.m_AdrX + other_block.m_BWidth + x_sp ||
            other_block.m_AdrX >= m_AdrX + m_BWidth + x_sp)    // If one rectangle is on side of other

            || (m_AdrY >= other_block.m_AdrY + other_block.m_BHeight + y_sp ||
                other_block.m_AdrY >= m_AdrY + m_BHeight + y_sp)); // If one rectangle is above other
    }

    //Returns true if the block is inside frame boundaries
    bool IsInRange(const mfxFrameInfo& info) const
    {
        return ((info.Width > 0 && info.Height > 0) &&
            (m_AdrX + m_BWidth <= (mfxU32)info.Width) &&
            (m_AdrY + m_BHeight <= (mfxU32)info.Height));
    }

    //Returns true if the block is inside another block
    bool IsInBlock(const BaseBlock& otherBlock) const
    {
        return (m_AdrX >= otherBlock.m_AdrX && m_AdrX + m_BWidth <= otherBlock.m_AdrX + otherBlock.m_BWidth
            &&  m_AdrY >= otherBlock.m_AdrY && m_AdrY + m_BHeight <= otherBlock.m_AdrY + otherBlock.m_BHeight);
    }

    void GetChildBlock(std::vector<BaseBlock>& childrenBlocks) const;
    void GetChildRefSampleInfo(const RefSampleAvail & currAvail, const BaseBlock & ctu, std::vector<RefSampleAvail>& childrenRefSampleAvail) const;

    bool operator==(const BaseBlock& rhs) const
    {
        return (m_AdrX   == rhs.m_AdrX   && m_AdrY    == rhs.m_AdrY &&
                m_BWidth == rhs.m_BWidth && m_BHeight == rhs.m_BHeight);
    }
};

//Struct representing a single node in a quad-tree
struct QuadTreeNode
{
    std::shared_ptr<QuadTreeNode*> m_pParent;
    std::vector<QuadTreeNode> m_Children;
    mfxU8 m_Level;
    QuadTreeNode() : m_pParent(nullptr), m_Level(0) {}
    QuadTreeNode(std::shared_ptr<QuadTreeNode*> pParent, mfxU8 level) :
        m_pParent(pParent),
        m_Level(level)
    {}
    void MakeChildren()
    {
        for (mfxU32 i = 0; i < 4; i++)
        {
            QuadTreeNode node(std::make_shared<QuadTreeNode*>(this), m_Level + 1);
            m_Children.push_back(node);
        }
    }
};

//Structure representing a quad-tree as a whole
struct QuadTree
{
    QuadTreeNode root;
    void Clear()
    {
        root.m_Children.clear();
    }
    bool IsEmpty()
    {
        return root.m_Children.empty();
    }

    //This recursive function fills outVec with BaseBlocks, coordinates and sizes of which correspond to
    //the quad-tree structure specified by the quad-tree root node and the base size and coordinates of
    //the "root node "
    void GetQuadTreeBlocksRecur(QuadTreeNode & node, mfxU32 baseAdrX, mfxU32 baseAdrY, mfxU32 baseSize, std::vector<BaseBlock>& outVec);
    //this function is a modification of the one above
    //it keeps track of refSamples availability for every block
    //thus it outputs outVec of IntraBlock structures and is used when intra prediction is enabled

    //now algorithm works fine assuming that CTUs cannot lie next to each other
    //but it limits some cases for intra/inter mix

    void GetQuadTreeRefSampleAvailVector(QuadTreeNode & node, BaseBlock & currBlock, const BaseBlock & ctu, std::vector<RefSampleAvail>& outVec);

    //TODO: develop this function taking into account that INTER coded noise blocks are written prior to INTRA prediction
    //and include available refSamples for it
    void GetQuadTreeRefSampleAvailVectorRecur(QuadTreeNode & node, BaseBlock& currBlock, const RefSampleAvail& currAvail, const BaseBlock& CTU, std::vector<RefSampleAvail>& outVec);
};

//block handling patch data which was generated via intra prediction
struct PatchBlock : BaseBlock
{
    std::unique_ptr<mfxU8[]> PatchData;
    mfxU8 *m_YPlane = nullptr;
    mfxU8 *m_UPlane = nullptr;
    mfxU8 *m_VPlane = nullptr;

    //Copy constructor
    PatchBlock(const PatchBlock& rhs);

    //Assignment operator
    PatchBlock& operator=(const PatchBlock& rhs);

    PatchBlock(const BaseBlock& BB);

    //constructor that memsets PatchBlock with the particurlar sample
    PatchBlock(const BaseBlock& BB, mfxU8 y_comp, mfxU8 u_comp, mfxU8 v_comp);

    //constructor that fills PatchBlock with coresponding data from the otherPatch
    PatchBlock(const BaseBlock& dstBlock, const PatchBlock& srcBlock);

    //constructor that filles PatchBlock with coresponding data from the surface
    PatchBlock(const BaseBlock& BB, const ExtendedSurface& surf);

    //distance between current PatchBlock and other_patch counted as a sum of abs differences between luma components over all samples
    mfxU32 CalcYSAD(const PatchBlock& otherPatch) const;

    //AdrX and AdrY are given in the luma plane space (i.e. raw frame coords)
    mfxU8 GetSampleI420(COLOR_COMPONENT comp, mfxU32 AdrX, mfxU32 AdrY) const;

    //Inserts refPatch into this PatchBlock, if refPatch is located inside this PatchBlock
    void InsertAnotherPatch(const PatchBlock & refPatch);
};

//Structure representing a prediction unit
struct PUBlock : BaseBlock
{
    PUBlock(mfxU32 adrX = 0, mfxU32 adrY = 0, mfxU32 bWidth = 0, mfxU32 bHeight = 0) :
        BaseBlock(adrX, adrY, bWidth, bHeight),
        m_MV(),
        m_MVP(),
        predFlagL0(false),
        predFlagL1(false)
    {}

    // Attention: MV and MVP are given in quarter-sample units. Consider scaling when working with PUMotionVector.
    PUMotionVector m_MV;  //Contains complete motion vector (i.e. predictor + additional vector inside SW)
    PUMotionVector m_MVP; //Contains motion vector predictor only

    // Index of this PU's MVP inside the generation pool
    // 4 predictors are in pool, valid values are in range [0, 3]
    // -1 indicates no MVP was applied to this PU's MV
    mfxI16 usedMVPIndex = -1;

    // Index of the generation pool used to choose MVP from for this PU
    // -1 indicates no MVP was applied to this PU's MV
    mfxI16  usedMVPPoolNo = -1;

    // Prediction flags
    bool predFlagL0;
    bool predFlagL1;

    //Returns a BaseBlock with coordinates that are shifted by the m_MV L0 motion vector
    //relative to the current PU position
    BaseBlock GetShiftedBaseBlock(REF_LIST_INDEX list) const;

    //Returns true if both the block itself and blocks shifted by
    //L0 and L1 motion vectors are inside frame boundaries
    bool IsPUInRange(const mfxFrameInfo& info) const
    {
        mfxI64 posXL0 = (mfxI64)m_AdrX + (m_MV.MV[0].x >> 2);
        mfxI64 posYL0 = (mfxI64)m_AdrY + (m_MV.MV[0].y >> 2);

        mfxI64 posXL1 = (mfxI64)m_AdrX + (m_MV.MV[1].x >> 2);
        mfxI64 posYL1 = (mfxI64)m_AdrY + (m_MV.MV[1].y >> 2);

        //TODO: This may lead to slow performance for tightly packed test CTUs
        return (IsInRange(info)                                   &&
                posXL0 >= 0 && (posXL0 + m_BWidth  <= info.Width) &&
                posYL0 >= 0 && (posYL0 + m_BHeight <= info.Height)&&
                posXL1 >= 0 && (posXL1 + m_BWidth  <= info.Width) &&
                posYL1 >= 0 && (posYL1 + m_BHeight <= info.Height));
    }

    //For VERIFY purposes. Will only work correctly if ASG PU with correct predFlags is on the left-hand side
    bool operator==(const PUBlock& rhs) const
    {
        return (static_cast<BaseBlock>(*this) == static_cast<BaseBlock>(rhs)     &&
               (predFlagL0 == rhs.predFlagL0) && (predFlagL1 == rhs.predFlagL1)  &&
               (!predFlagL0 || m_MV.GetL0MVTuple() == rhs.m_MV.GetL0MVTuple())   &&
               (!predFlagL1 || m_MV.GetL1MVTuple() == rhs.m_MV.GetL1MVTuple()));
    }
};

//Structure representing a transform unit
struct TUBlock : BaseBlock
{
    //different intra modes can be set for luma and chroma
    INTRA_MODE m_IntraModeLuma = INTRA_MODE::DC;
    INTRA_MODE m_IntraModeChroma = INTRA_MODE::DC;
    RefSampleAvail m_RefSampleAvail;

    TUBlock(const BaseBlock& base) : BaseBlock(base) {}
    TUBlock(const BaseBlock& base, INTRA_MODE intraModeLuma, INTRA_MODE intraModeChroma) :
        BaseBlock(base),
        m_IntraModeLuma(intraModeLuma),
        m_IntraModeChroma(intraModeChroma)
    {}
};

//Structure representing a coding unit
struct CUBlock : BaseBlock
{
    // Quadtree representing TU structure
    QuadTree m_TUQuadTree;
    PRED_TYPE m_PredType            = INTRA_PRED;         //intra or inter
    INTRA_PART_MODE m_IntraPartMode = INTRA_NONE; //partitioning into different intra modes
    INTER_PART_MODE m_InterPartMode = INTER_NONE; //partitioning into PUs
    std::vector<PUBlock> m_PUVec; //PU and TU partitioning can be
    std::vector<TUBlock> m_TUVec; //performed independently

    CUBlock(const BaseBlock& base) : BaseBlock(base) {}

    void BuildPUsVector(INTER_PART_MODE mode);

    //For VERIFY purposes
    bool operator==(const CUBlock& rhs) const
    {
        return (static_cast<BaseBlock>(*this) == static_cast<BaseBlock>(rhs) &&
                m_PUVec                       == rhs.m_PUVec);
    }
};

// Descriptor of CTU
struct CTUDescriptor : BaseBlock
{
    // Coordinates in CTUs
    mfxU32 m_AdrXInCTU = 0;
    mfxU32 m_AdrYInCTU = 0;

    // Quadtree representing CU structure
    QuadTree m_CUQuadTree;

    // Contains CU blocks according to the m_CUQuadTree structure
    // Gets filled in GetCUVecInCTU
    std::vector<CUBlock> m_CUVec;

    // Contains MVP pools used in MV generator for PUs inside this CTU
    // Gets filled in DumpMVPPools
    std::vector<std::vector<PUMotionVector>> m_MVPGenPools;

    CTUDescriptor(mfxU32 addr_x = 0, mfxU32 addr_y = 0, mfxU32 b_addr_x = 0, mfxU32 b_addr_y = 0, mfxU32 b_width = 0, mfxU32 b_height = 0)
        : BaseBlock(b_addr_x, b_addr_y, b_width, b_height)
        , m_AdrXInCTU(addr_x)
        , m_AdrYInCTU(addr_y)
    {};

    bool operator==(const CTUDescriptor& rhs) const
    {
        return (m_AdrXInCTU == rhs.m_AdrXInCTU &&
                m_AdrYInCTU == rhs.m_AdrYInCTU &&
                m_CUVec     == rhs.m_CUVec);
    }

    std::vector<PUBlock> GetTotalPUsVec() const
    {
        std::vector<PUBlock> retVec;
        for (auto& CU : m_CUVec)
        {
            retVec.insert(retVec.end(), CU.m_PUVec.begin(), CU.m_PUVec.end());
        }
        return retVec;
    }
};

using FrameOccRefBlockRecord = std::map<mfxU32, std::vector<BaseBlock>>;

struct InterpolWorkBlock : BaseBlock
{
    InterpolWorkBlock() {};
    InterpolWorkBlock(const BaseBlock& block) :
        BaseBlock(block),
        m_YArr(block.m_BHeight * block.m_BWidth, LUMA_DEFAULT),
        m_UArr(block.m_BHeight / 2 * block.m_BWidth / 2, CHROMA_DEFAULT),
        m_VArr(block.m_BHeight / 2 * block.m_BWidth / 2, CHROMA_DEFAULT)
    {}
    std::vector<mfxI32> m_YArr;
    std::vector<mfxI32> m_UArr;
    std::vector<mfxI32> m_VArr;
};

#endif // MFX_VERSION

#endif //__BLOCK_STRUCTURES_H__
