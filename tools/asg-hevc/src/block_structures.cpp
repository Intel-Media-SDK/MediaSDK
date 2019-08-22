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

#include "block_structures.h"

void BaseBlock::GetChildBlock(std::vector<BaseBlock>& childrenBlocks) const
{
    childrenBlocks.clear();
    childrenBlocks.reserve(4);

    //sizes and positions
    mfxU32 childHeight = m_BHeight / 2;
    mfxU32 childWidth = m_BWidth / 2;
    for (mfxU32 i = 0; i < 4; i++)
    {
        childrenBlocks.emplace_back(m_AdrX + childWidth * (i % 2), m_AdrY + childHeight * (i / 2), childHeight, childWidth);
    }
}

void BaseBlock::GetChildRefSampleInfo(const RefSampleAvail & currAvail, const BaseBlock & ctu, std::vector<RefSampleAvail>& childrenRefSampleAvail) const
{
    childrenRefSampleAvail.clear();
    childrenRefSampleAvail.resize(4);

    //is left down block available for children
    childrenRefSampleAvail[0].LeftDown = true;
    childrenRefSampleAvail[1].LeftDown = false;

    if (m_AdrY + m_BHeight == ctu.m_AdrY + ctu.m_BHeight)
    {
        childrenRefSampleAvail[2].LeftDown = true;
        childrenRefSampleAvail[3].LeftDown = true;
    }
    else
    {
        childrenRefSampleAvail[2].LeftDown = currAvail.LeftDown;
        childrenRefSampleAvail[3].LeftDown = false;
    }

    //is up right block available for children
    childrenRefSampleAvail[0].UpRight = true;
    childrenRefSampleAvail[2].UpRight = true;

    if (m_AdrX + m_BWidth == ctu.m_AdrX + ctu.m_BWidth)
    {
        childrenRefSampleAvail[1].UpRight = true;
        childrenRefSampleAvail[3].UpRight = true;
    }
    else
    {
        childrenRefSampleAvail[1].UpRight = currAvail.UpRight;
        childrenRefSampleAvail[3].UpRight = false;
    }
}

//This recursive function fills outVec with BaseBlocks, coordinates and sizes of which correspond to
//the quad-tree structure specified by the quad-tree root node and the base size and coordinates of
//the "root node "
void QuadTree::GetQuadTreeBlocksRecur(QuadTreeNode & node, mfxU32 baseAdrX, mfxU32 baseAdrY, mfxU32 baseSize, std::vector<BaseBlock>& outVec)
{
    if (!node.m_Children.empty())
    {
        for (mfxU32 i_y = 0; i_y < 2; i_y++)
        {
            for (mfxU32 i_x = 0; i_x < 2; i_x++)
            {
                QuadTreeNode& child = node.m_Children[2 * i_y + i_x];
                mfxU32 childSize = baseSize / 2;
                mfxU32 childAdrX = baseAdrX + i_x * childSize;
                mfxU32 childAdrY = baseAdrY + i_y * childSize;
                GetQuadTreeBlocksRecur(child, childAdrX, childAdrY, childSize, outVec);
            }
        }
    }
    else
    {
        outVec.emplace_back(baseAdrX, baseAdrY, baseSize, baseSize);
        return;
    }
}

void QuadTree::GetQuadTreeRefSampleAvailVector(QuadTreeNode & node, BaseBlock & currBlock, const BaseBlock & ctu, std::vector<RefSampleAvail>& outVec)
{
    RefSampleAvail helper(true, true);
    GetQuadTreeRefSampleAvailVectorRecur(node, currBlock, helper, ctu, outVec);
}

void QuadTree::GetQuadTreeRefSampleAvailVectorRecur(QuadTreeNode & node, BaseBlock & currBlock, const RefSampleAvail& currAvail, const BaseBlock & ctu, std::vector<RefSampleAvail>& outVec)
{
    if (!node.m_Children.empty())
    {
        //children blocks, refSamples availability for children is counted here
        std::vector<BaseBlock> childrenBlocks;
        currBlock.GetChildBlock(childrenBlocks);
        std::vector<RefSampleAvail> childrenRefSamplesInfo;
        currBlock.GetChildRefSampleInfo(currAvail, ctu, childrenRefSamplesInfo);

        //recursive call for children
        for (mfxU32 i = 0; i < 4; i++)
        {
            GetQuadTreeRefSampleAvailVectorRecur(node.m_Children[i], childrenBlocks[i], childrenRefSamplesInfo[i], ctu, outVec);
        }
    }
    else
    {

        //put leaves into outVec in depth-first  order
        outVec.emplace_back(currAvail);
    }
    return;
}

PatchBlock::PatchBlock(const PatchBlock & rhs) : PatchBlock(static_cast<BaseBlock>(rhs))
{
    memcpy(PatchData.get(), rhs.PatchData.get(), sizeof(mfxU8) * m_BWidth * m_BHeight * 3 / 2);
}

PatchBlock & PatchBlock::operator=(const PatchBlock & rhs)
{
    BaseBlock::operator=(rhs);

    mfxU32 sq = m_BWidth * m_BHeight;
    PatchData.reset(new mfxU8[sq * 3 / 2]);

    memcpy(PatchData.get(), rhs.PatchData.get(), sizeof(mfxU8) * sq * 3 / 2);

    m_YPlane = PatchData.get();
    m_UPlane = m_YPlane + sq;
    m_VPlane = m_UPlane + (sq / 4);

    return *this;
}

PatchBlock::PatchBlock(const BaseBlock& BB) : BaseBlock(BB)
{
    mfxU32 sq = m_BWidth * m_BHeight;
    PatchData.reset(new mfxU8[sq * 3 / 2]);
    m_YPlane = PatchData.get();
    m_UPlane = m_YPlane + sq;
    m_VPlane = m_UPlane + (sq / 4);

    memset(m_YPlane, LUMA_DEFAULT, sq);
    memset(m_UPlane, CHROMA_DEFAULT, sq / 4);
    memset(m_VPlane, CHROMA_DEFAULT, sq / 4);
}

//constructor that memsets PatchBlock with the particurlar sample
PatchBlock::PatchBlock(const BaseBlock& BB, mfxU8 y_comp, mfxU8 u_comp, mfxU8 v_comp) : BaseBlock(BB)
{
    mfxU32 sq = m_BWidth * m_BHeight;
    PatchData.reset(new mfxU8[sq * 3 / 2]);
    m_YPlane = PatchData.get();
    m_UPlane = m_YPlane + sq;
    m_VPlane = m_UPlane + (sq / 4);

    memset(m_YPlane, y_comp, sq);
    memset(m_UPlane, u_comp, sq / 4);
    memset(m_VPlane, v_comp, sq / 4);
}

//constructor that fills PatchBlock with coresponding data from the srcPatch
PatchBlock::PatchBlock(const BaseBlock& dstBlock, const PatchBlock& srcBlock) : PatchBlock(dstBlock)
{
    if (!dstBlock.IsInBlock(srcBlock))
    {
        throw std::string("ERROR: PatchBlock: new PatchBlock should be located inside the old one in frame coords");
    }

    //Calculating the coords of dstBlock relative to srcBlock
    mfxU32 offsetX = dstBlock.m_AdrX - srcBlock.m_AdrX;
    mfxU32 offsetY = dstBlock.m_AdrY - srcBlock.m_AdrY;

    for (mfxU32 i = 0; i < m_BHeight; i++)
    {
        memcpy(m_YPlane + i * m_BWidth, srcBlock.m_YPlane + (offsetY + i) * srcBlock.m_BWidth + offsetX, m_BWidth);
    }
    for (mfxU32 i = 0; i < m_BHeight / 2; i++)
    {
        memcpy(m_UPlane + i * (m_BWidth / 2), srcBlock.m_UPlane + (offsetY / 2 + i) * (srcBlock.m_BWidth / 2) + offsetX / 2, m_BWidth / 2);
        memcpy(m_VPlane + i * (m_BWidth / 2), srcBlock.m_VPlane + (offsetY / 2 + i) * (srcBlock.m_BWidth / 2) + offsetX / 2, m_BWidth / 2);
    }
}

//constructor that filles PatchBlock with coresponding data from the surface
PatchBlock::PatchBlock(const BaseBlock & BB, const ExtendedSurface & surf) : PatchBlock(BB)
{
    if (!BB.IsInBlock(BaseBlock(0, 0, surf.Info.CropW, surf.Info.CropH)))
    {
        throw std::string("ERROR: PatchBlock: new patchBlock should be located inside the surface");
    }

    for (mfxU32 i = 0; i < m_BHeight; i++)
    {
        memcpy(m_YPlane + i * m_BWidth, surf.Data.Y + (m_AdrY + i) * surf.Data.Pitch + m_AdrX, m_BWidth);
    }
    for (mfxU32 i = 0; i < m_BHeight / 2; i++)
    {
        memcpy(m_UPlane + i * (m_BWidth / 2), surf.Data.U + (m_AdrY / 2 + i) * (surf.Data.Pitch / 2) + m_AdrX / 2, m_BWidth / 2);
        memcpy(m_VPlane + i * (m_BWidth / 2), surf.Data.V + (m_AdrY / 2 + i) * (surf.Data.Pitch / 2) + m_AdrX / 2, m_BWidth / 2);
    }
}

//distance between current PatchBlock and other_patch counted as a sum of abs differences between luma components over all samples
mfxU32 PatchBlock::CalcYSAD(const PatchBlock & otherPatch) const
{
    mfxU32 curDiff = 0;
    if (m_BHeight == otherPatch.m_BHeight && m_BWidth == otherPatch.m_BWidth)
    {
        for (mfxU32 i = 0; i < m_BHeight*m_BWidth; i++)
        {
            curDiff += abs(m_YPlane[i] - otherPatch.m_YPlane[i]);
        }
    }
    else
    {
        throw std::string("ERROR: CalcYSAD: block size mismatch");
    }
    return curDiff;
}

//compAdrX and compAdrY are given in the comp color space
mfxU8 PatchBlock::GetSampleI420(COLOR_COMPONENT comp, mfxU32 compAdrX, mfxU32 compAdrY) const
{
    switch (comp)
    {
    case LUMA_Y:
        return m_YPlane[compAdrY * m_BWidth + compAdrX];
    case CHROMA_U:
        return m_UPlane[compAdrY * (m_BWidth / 2) + compAdrX];
    case CHROMA_V:
        return m_VPlane[compAdrY * (m_BWidth / 2) + compAdrX];
    default:
        throw std::string("ERROR: Trying to get unspecified component");
    }
}

//Inserts refPatch into this PatchBlock, if refPatch is located inside this PatchBlock
void PatchBlock::InsertAnotherPatch(const PatchBlock & refPatch)
{
    if (!refPatch.IsInBlock(*this))
    {
        throw std::string("ERROR: InsertAnotherPatch: refPatch should be inside targetPatch\n");
    }

    //Calculating coordinate offset of refPatch relative to this PatchBlock
    mfxU32 offsetX = refPatch.m_AdrX - m_AdrX;
    mfxU32 offsetY = refPatch.m_AdrY - m_AdrY;

    //luma
    for (mfxU32 i = 0; i < refPatch.m_BHeight; i++)
    {
        memcpy(m_YPlane + (offsetY + i) * m_BWidth + offsetX, refPatch.m_YPlane + i * refPatch.m_BWidth, refPatch.m_BWidth);
    }
    //chroma U, V
    for (mfxU32 i = 0; i < refPatch.m_BHeight / 2; ++i)
    {
        memcpy(m_UPlane + (offsetY / 2 + i) * m_BWidth / 2 + offsetX / 2, refPatch.m_UPlane + i * (refPatch.m_BWidth / 2), refPatch.m_BWidth / 2);
        memcpy(m_VPlane + (offsetY / 2 + i) * m_BWidth / 2 + offsetX / 2, refPatch.m_VPlane + i * (refPatch.m_BWidth / 2), refPatch.m_BWidth / 2);
    }
    return;
}

//Returns a BaseBlock with coordinates that are shifted by the m_MV motion vector
//from the corresponding list relative to the current PU position
BaseBlock PUBlock::GetShiftedBaseBlock(REF_LIST_INDEX list) const
{
    mfxI64 posX = (mfxI64) m_AdrX + (m_MV.MV[list].x >> 2);
    mfxI64 posY = (mfxI64) m_AdrY + (m_MV.MV[list].y >> 2);

    if (posX < 0 && posY < 0)
    {
        throw std::string("ERROR: GetShiftedBaseBlock: negative resulting block coords");
    }

    return BaseBlock((mfxU32) posX, (mfxU32) posY, m_BWidth, m_BHeight);
}

void CUBlock::BuildPUsVector(INTER_PART_MODE mode)
{
    switch (mode)
    {
    case INTER_2NxN:
        // UP PU
        m_PUVec.emplace_back(PUBlock(m_AdrX, m_AdrY, m_BWidth, m_BHeight / 2));
        // DOWN PU
        m_PUVec.emplace_back(PUBlock(m_AdrX, m_AdrY + m_BHeight / 2, m_BWidth, m_BHeight / 2));
        break;

    case INTER_Nx2N:
        // LEFT PU
        m_PUVec.emplace_back(PUBlock(m_AdrX, m_AdrY, m_BWidth / 2, m_BHeight));
        // RIGHT PU
        m_PUVec.emplace_back(PUBlock(m_AdrX + m_BWidth / 2, m_AdrY, m_BWidth / 2, m_BHeight));
        break;

    case INTER_NxN:
        // TOP LEFT PU
        m_PUVec.emplace_back(PUBlock(m_AdrX, m_AdrY, m_BWidth / 2, m_BHeight / 2));
        // TOP RIGHT PU
        m_PUVec.emplace_back(PUBlock(m_AdrX + m_BWidth / 2, m_AdrY, m_BWidth / 2, m_BHeight / 2));
        // BOT LEFT PU
        m_PUVec.emplace_back(PUBlock(m_AdrX, m_AdrY + m_BHeight / 2, m_BWidth / 2, m_BHeight / 2));
        // BOT RIGHT PU
        m_PUVec.emplace_back(PUBlock(m_AdrX + m_BWidth / 2, m_AdrY + m_BHeight / 2, m_BWidth / 2, m_BHeight / 2));
        break;

    case INTER_2NxnU:
        // UP SMALL PU
        m_PUVec.emplace_back(PUBlock(m_AdrX, m_AdrY, m_BWidth, m_BHeight / 4));
        // DOWN LARGE PU
        m_PUVec.emplace_back(PUBlock(m_AdrX, m_AdrY + m_BHeight / 4, m_BWidth, (m_BHeight * 3) / 4));
        break;

    case INTER_2NxnD:
        // UP LARGE PU
        m_PUVec.emplace_back(PUBlock(m_AdrX, m_AdrY, m_BWidth, (m_BHeight * 3) / 4));
        // DOWN SMALL PU
        m_PUVec.emplace_back(PUBlock(m_AdrX, m_AdrY + (m_BHeight * 3) / 4, m_BWidth, m_BHeight / 4));
        break;

    case INTER_nLx2N:
        // LEFT SMALL PU
        m_PUVec.emplace_back(PUBlock(m_AdrX, m_AdrY, m_BWidth / 4, m_BHeight));
        // RIGHT LARGE PU
        m_PUVec.emplace_back(PUBlock(m_AdrX + m_BWidth / 4, m_AdrY, (m_BWidth * 3) / 4, m_BHeight));
        break;

    case INTER_nRx2N:
        // LEFT LARGE PU
        m_PUVec.emplace_back(PUBlock(m_AdrX, m_AdrY, (m_BWidth * 3) / 4, m_BHeight));
        // RIGHT SMALL PU
        m_PUVec.emplace_back(PUBlock(m_AdrX + (m_BWidth * 3) / 4, m_AdrY, m_BWidth / 4, m_BHeight));
        break;

    case INTER_2Nx2N:
    default:
        m_PUVec.emplace_back(PUBlock(m_AdrX, m_AdrY, m_BWidth, m_BHeight));
        break;
    }
    return;
}

#endif // MFX_VERSION
