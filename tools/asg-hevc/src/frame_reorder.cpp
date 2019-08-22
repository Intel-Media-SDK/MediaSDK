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

#include "frame_reorder.h"

mfxI32 GetFrameNum(bool bField, mfxI32 Poc, bool bSecondField)
{
    return bField ? (Poc + (!bSecondField)) / 2 : Poc;
}

mfxU8 GetFrameType(
    mfxVideoParam const & video,
    mfxU32                pictureOrder, // Picture order after last IDR
    bool                  isPictureOfLastFrame)
{
    mfxU32 gopOptFlag = video.mfx.GopOptFlag;
    mfxU32 gopPicSize = video.mfx.GopPicSize;
    mfxU32 gopRefDist = video.mfx.GopRefDist;
    mfxU32 idrPicDist = gopPicSize * (video.mfx.IdrInterval);

    if (gopPicSize == 0xffff)
        idrPicDist = gopPicSize = 0xffffffff;

    bool bFields = !!(video.mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_FIELD_SINGLE);

    mfxU32 frameOrder = bFields ? pictureOrder / 2 : pictureOrder;

    bool   bSecondField = bFields && (pictureOrder & 1);
    bool   bIdr = (idrPicDist ? frameOrder % idrPicDist : frameOrder) == 0;

    if (bIdr)
        return bSecondField ? (MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF) : (MFX_FRAMETYPE_I | MFX_FRAMETYPE_REF | MFX_FRAMETYPE_IDR);

    if (frameOrder % gopPicSize == 0)
        return (mfxU8)(bSecondField ? MFX_FRAMETYPE_P : MFX_FRAMETYPE_I) | MFX_FRAMETYPE_REF;

    if (frameOrder % gopPicSize % gopRefDist == 0)
        return (MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF);

    if ((gopOptFlag & MFX_GOP_STRICT) == 0)
    {
        if (((frameOrder + 1) % gopPicSize == 0 && (gopOptFlag & MFX_GOP_CLOSED)) ||
            (idrPicDist && (frameOrder + 1) % idrPicDist == 0) ||
            isPictureOfLastFrame)
        {
            return (MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF);
        }
    }

    return MFX_FRAMETYPE_B;
}

ExternalFrame FrameReorder::CreateExternalFrame(mfxI32 order, const mfxVideoParam& param)
{
    mfxI32 poc = -1;
    ExternalFrame out = { 0xffffffff, 0, 0, poc, false, false };
    FrameIterator itOut = m_queue.end();

    bool bIsFieldCoding = !!(param.mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_FIELD_SINGLE);

    //=================1. Get type, poc, put frame in the queue=====================

    if (order >= 0)
    {

        bool isPictureOfLastFrameInGOP = ((mfxU32)order >> (mfxU32)bIsFieldCoding) == ((m_nMaxFrames - 1) >> (mfxU32)bIsFieldCoding); // EOS

        Frame frame;

        frame.DisplayOrder = (mfxU32)order;

        frame.Type = GetFrameType(param, order - m_lastIdr, isPictureOfLastFrameInGOP); // If we will use IDR not only for first field
                                                                                        // We need to change this logic

        if(frame.Type & MFX_FRAMETYPE_IDR && (order & 1))
            throw std::string("ERROR: FrameReorder::CreateExternalFrame: Idr isn't first field");

        if (frame.Type & MFX_FRAMETYPE_IDR)
            m_lastIdr = order;

        frame.Poc = order - m_lastIdr;
        frame.bSecondField = bIsFieldCoding && (order & 1);
        frame.bBottomField = false;
        if (bIsFieldCoding)
        {
            frame.bBottomField = isBFF(param) != frame.bSecondField;
        }

        if (frame.Type & MFX_FRAMETYPE_I)
        {
            m_anchorPOC = frame.Poc;
        }

        m_queue.emplace_back(frame);
    }

    //=================2. Reorder frames, fill output frame poc, type, etc=====================
    itOut = Reorder(order < 0, bIsFieldCoding);
    if (itOut == m_queue.end())
        return out;

    out.DisplayOrder = itOut->DisplayOrder;

    bool isIdr = !!(itOut->Type & MFX_FRAMETYPE_IDR);
    bool isRef = !!(itOut->Type & MFX_FRAMETYPE_REF);
    bool isI = !!(itOut->Type & MFX_FRAMETYPE_I);
    bool isB = !!(itOut->Type & MFX_FRAMETYPE_B);

    itOut->LastRAP = m_lastFrame.LastRAP;

    if (isI)
    {
        itOut->IPoc = itOut->Poc;
        itOut->PrevIPoc = m_lastFrame.IPoc;
        itOut->NextIPoc = -1;
    }
    else
    {
        if (itOut->Poc >= m_lastFrame.IPoc)
        {
            itOut->IPoc = m_lastFrame.IPoc;
            itOut->PrevIPoc = m_lastFrame.PrevIPoc;
            itOut->NextIPoc = m_lastFrame.NextIPoc;
        }
        else
        {
            itOut->IPoc = m_lastFrame.PrevIPoc;
            itOut->PrevIPoc = -1;
            itOut->NextIPoc = m_lastFrame.IPoc;
        }
    }

    out.Poc = itOut->Poc;
    out.Type = itOut->Type;
    out.bSecondField = itOut->bSecondField;
    out.bBottomField = itOut->bBottomField;

    //=================3. Update DPB=====================
    if (isIdr)
        m_dpb.clear();

    if (itOut->Poc > itOut->LastRAP &&
        m_lastFrame.Poc <= m_lastFrame.LastRAP)
    {
        const mfxI32 & lastRAP = itOut->LastRAP;
        // On the 1st TRAIL remove all except IRAP
        m_dpb.erase(std::remove_if(m_dpb.begin(), m_dpb.end(),
            [&lastRAP](Frame const & entry) { return entry.Poc != lastRAP; }),
            m_dpb.end());
    }

    //=================4. Construct RPL=====================
    std::sort(m_dpb.begin(), m_dpb.end(), [](const Frame & lhs_frame, const Frame & rhs_frame)
    {
        return lhs_frame.Poc < rhs_frame.Poc;
    });

    std::vector<Frame> & L0 = out.ListX[0];
    std::vector<Frame> & L1 = out.ListX[1];

    L0.clear();
    L1.clear();

    if (!isI)
    {
        // Fill L0/L1
        for (auto it = m_dpb.begin(); it != m_dpb.end(); it++)
        {
            bool list = it->Poc > out.Poc;
            out.ListX[list].push_back(*it);
        }

        auto preferSamePolarity = [&out](const Frame & lhs_frame, const Frame & rhs_frame)
        {
            mfxI32 currFrameNum = GetFrameNum(true, out.Poc, out.bSecondField);

            mfxU32 lhs_distance = std::abs(GetFrameNum(true, lhs_frame.Poc, lhs_frame.bSecondField) - currFrameNum) * 2 + ((lhs_frame.bBottomField == out.bBottomField) ? 0 : 1);

            mfxU32 rhs_distance = std::abs(GetFrameNum(true, rhs_frame.Poc, rhs_frame.bSecondField) - currFrameNum) * 2 + ((rhs_frame.bBottomField == out.bBottomField) ? 0 : 1);

            return lhs_distance <= rhs_distance;
        };

        auto distance = [&out](const Frame & lhs_frame, const Frame & rhs_frame)
        {
            mfxU32 lhs_distance = std::abs(lhs_frame.Poc - out.Poc);
            mfxU32 rhs_distance = std::abs(rhs_frame.Poc - out.Poc);

            return lhs_distance < rhs_distance;
        };

        if (bIsFieldCoding)
        {
            std::sort(L0.begin(), L0.end(), preferSamePolarity);
            std::sort(L1.begin(), L1.end(), preferSamePolarity);
        }
        else
        {
            std::sort(L0.begin(), L0.end(), distance);
            std::sort(L1.begin(), L1.end(), distance);
        }

        if ((param.mfx.GopOptFlag & MFX_GOP_CLOSED))
        {
            const mfxI32 & IPoc = itOut->IPoc;
            {
                // Remove L0 refs beyond GOP
                L0.erase(std::remove_if(L0.begin(), L0.end(),
                    [&IPoc](const Frame & frame) { return frame.Poc < IPoc; }),
                    L0.end());
            }

            const mfxI32 & nextIPoc = itOut->NextIPoc;
            if (nextIPoc != -1)
            {
                // Remove L1 refs beyond GOP
                L1.erase(std::remove_if(L1.begin(), L1.end(),
                    [&nextIPoc](const Frame & frame) { return frame.Poc >= nextIPoc; }),
                    L1.end());
            }
        }

        // if B's L1 is zero (e.g. in case of closed gop)
        if (isB && !L1.size() && L0.size())
            L1.push_back(L0[0]);

        if (!isB && m_UseGPB)
        {
            L1 = L0;
            std::sort(L1.begin(), L1.end(), distance);
        }

        // Remove extra entries
        if (L0.size() > (isB ? m_NumRefActiveBL0 : m_NumRefActiveP))
            L0.resize(isB ? m_NumRefActiveBL0 : m_NumRefActiveP);

        if (L1.size() > m_NumRefActiveBL1)
            L1.resize(m_NumRefActiveBL1);

        std::sort(L0.begin(), L0.end(), distance);
        std::sort(L1.begin(), L1.end(), distance);
    }

    //=================5. Save current frame in DPB=====================
    if (isRef)
    {
        if (m_dpb.size() == m_NumRef)
        {
            auto toRemove = m_dpb.begin();

            m_dpb.erase(toRemove);
        }
        m_dpb.push_back(*itOut);
    }

    itOut->NalType = GetNALUType(*itOut, !bIsFieldCoding);

    if (itOut->NalType == CRA_NUT || itOut->NalType == IDR_W_RADL)
        itOut->LastRAP = itOut->Poc;

    m_lastFrame = *itOut;

    m_queue.erase(itOut);

    return out;
}

mfxU8 FrameReorder::GetNALUType(Frame const & frame, bool isRAPIntra)
{
    const bool isI = !!(frame.Type & MFX_FRAMETYPE_I);
    const bool isRef = !!(frame.Type & MFX_FRAMETYPE_REF);
    const bool isIDR = !!(frame.Type & MFX_FRAMETYPE_IDR);

    if (isIDR)
        return IDR_W_RADL;

    if (isI && isRAPIntra)
    {
        return CRA_NUT;
    }

    if (frame.Poc > frame.LastRAP)
    {
        return isRef ? TRAIL_R : TRAIL_N;
    }

    if (isRef)
        return RASL_R;
    return RASL_N;
}

bool FrameReorder::HasL1(mfxI32 poc)
{
    for (auto it = m_dpb.begin(); it < m_dpb.end(); it++)
        if (it->Poc > poc)
            return true;
    return false;
}

mfxU32 FrameReorder::BRefOrder(mfxU32 displayOrder, mfxU32 begin, mfxU32 end, mfxU32 counter, bool & ref)
{
    ref = (end - begin > 1);

    mfxU32 pivot = (begin + end) / 2;
    if (displayOrder == pivot)
        return counter;
    else if (displayOrder < pivot)
        return BRefOrder(displayOrder, begin, pivot, counter + 1, ref);
    else
        return BRefOrder(displayOrder, pivot + 1, end, counter + 1 + pivot - begin, ref);
}

mfxU32 FrameReorder::GetBiFrameLocation(mfxU32 displayOrder, mfxU32 num, bool &ref)
{
    ref = false;
    return BRefOrder(displayOrder, 0, num, 0, ref);
}

mfxU32 FrameReorder::BPyrReorder(const std::vector<FrameIterator> & bframes)
{
    mfxU32 num = (mfxU32)bframes.size();
    if (bframes[0]->Bpo == (mfxU32)MFX_FRAMEORDER_UNKNOWN)
    {
        bool bRef = false;

        for (mfxU32 i = 0; i < (mfxU32)bframes.size(); i++)
        {
            bframes[i]->Bpo = GetBiFrameLocation(i, num, bRef);
            if (bRef)
                bframes[i]->Type |= MFX_FRAMETYPE_REF;
        }
    }
    mfxU32 minBPO = (mfxU32)MFX_FRAMEORDER_UNKNOWN;
    mfxU32 ind = 0;
    for (mfxU32 i = 0; i < (mfxU32)bframes.size(); i++)
    {
        if (bframes[i]->Bpo < minBPO)
        {
            ind = i;
            minBPO = bframes[i]->Bpo;
        }
    }
    return ind;
}

FrameReorder::FrameIterator FrameReorder::Reorder(bool flush, bool bFields)
{
    FrameIterator begin = m_queue.begin();
    FrameIterator end = m_queue.begin();

    while (end != m_queue.end())
    {
        if ((end != begin) && (end->Type & MFX_FRAMETYPE_IDR))
        {
            flush = true;
            break;
        }
        end++;
    }

    if (bFields && m_lastFieldInfo.bFirstField())
    {
        while (begin != end && !m_lastFieldInfo.isCorrespondSecondField(*begin))
            begin++;

        if (begin != end)
        {
            m_lastFieldInfo.CorrectFrameInfo(*begin);
            return begin;
        }
        else
            begin = m_queue.begin();
    }

    FrameIterator top = Reorder(begin, end, flush, bFields);

    if (top == end)
    {
        return top;
    }

    if (bFields)
    {
        m_lastFieldInfo.SaveInfo(*top);
    }

    return top;
}

FrameReorder::FrameIterator FrameReorder::Reorder(FrameIterator begin, FrameIterator end, bool flush, bool bFields)
{
    FrameIterator top = begin;
    FrameIterator b0 = end; // 1st non-ref B with L1 > 0
    std::vector<FrameIterator> bframes;

    bool isBPyramid = !!(m_BRefType == MFX_B_REF_PYRAMID);

    while (top != end && (top->Type & MFX_FRAMETYPE_B))
    {
        if (HasL1(top->Poc) && (!top->bSecondField))
        {
            if (isBPyramid)
                bframes.push_back(top);
            else if (top->Type & MFX_FRAMETYPE_REF)
            {
                if (b0 == end || (top->Poc - b0->Poc < bFields + 2))
                    return top;
            }
            else if (b0 == end)
                b0 = top;
        }
        top++;
    }

    if (!bframes.empty())
    {
        return bframes[BPyrReorder(bframes)];
    }

    if (b0 != end)
        return b0;

    bool strict = !!(m_GopOptFlag & MFX_GOP_STRICT);
    if (flush && top == end && begin != end)
    {
        top--;
        if (strict)
            top = begin;
        else
            top->Type = MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF;

        if (top->bSecondField && top != begin)
        {
            top--;
            if (strict)
                top = begin;
            else
                top->Type = MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF;
        }
    }

    return top;
}
#endif // MFX_VERSION
