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

#include "refcontrol.h"

using namespace std;

void RefState::Dump(fstream& ofs) const
{
    const char separator = '|';

    ofs << setw(3) << picture.orderCount << ' '
        << hex << showbase << setw(3 + 2) << picture.frameType << ' '
        << setw(3 + 2) << picture.picStruct << dec;
    if (ofs.fail())
        throw std::string("ERROR: PicStruct buffer writing failed");
    for (mfxU32 list = 0; list < 2; list++)
    {
        ofs << ' ' << separator;
        for (mfxU32 i = 0; i < 8; i++)
            ofs << ' ' << setw(3) << (i < RefListActive[list].size() ? RefListActive[list][i] : -1);
    }
    ofs << ' ' << separator;
    for (mfxU32 i = 0; i < 16; i++)
        ofs << ' ' << setw(3) << (i < DPB.size() ? DPB[i] : -1);
    ofs << endl;
}

bool RefState::Load(fstream& ifs)
{
    const char separator_char = '|';
    char separator;
    RefState state;

    ifs >> state.picture.orderCount >> hex >> state.picture.frameType >> state.picture.picStruct >> dec;
    if (ifs.fail())
        return false;

    for (mfxU32 list = 0; list < 2; list++)
    {
        ifs >> separator;
        if (ifs.fail() || separator != separator_char)
            return false;
        state.RefListActive[list].clear();
        for (mfxI32 poc, i = 0; i < 8 && !(ifs >> poc).fail(); i++) {
            if (poc >= 0)
                state.RefListActive[list].push_back(poc);
        }
    }

    ifs >> separator;
    if (ifs.fail() || separator != separator_char)
        return false;
    state.DPB.clear();
    for (mfxI32 poc, i = 0; i < 16 && !(ifs >> poc).fail(); i++) {
        if (poc >= 0)
            state.DPB.push_back(poc);
    }

    *this = state;
    return true;
}

void RefControl::SetParams(const InputParams& params)
{
    m_params = params;
    maxDelay = params.m_RefDist * 2 + 1; // 2 for fields, 1 from asyncDepth
    maxDPBSize = params.m_NumRef + 1; // +1 to align with common msdk rules, that current frame is not counted in DPB
}

void RefControl::Encode(mfxI32 codingOrder)
{
    if (RPB.empty())
        throw std::string("ERROR: PicStruct buffer frame is lost");

    // select next frame to encode (no reordering for I and P)
    mfxI32 RPBidx = 0;
    if (RPB[0].frameType & MFX_FRAMETYPE_B)
    {
        mfxI32 cnt, i;
        for (cnt = 1; cnt < static_cast<mfxI32>(RPB.size()) &&
            (RPB[cnt].frameType & MFX_FRAMETYPE_B) &&
            RPB[0].orderCount + cnt == RPB[cnt].orderCount;
            cnt++); // consequent B count
        if (cnt < static_cast<mfxI32>(RPB.size()) && RPB[0].orderCount + cnt == RPB[cnt].orderCount)
            RPBidx = cnt; // consequent not B is pyramid's right base
        else
        {
            for (i = 1; i < cnt; i++)
                if ((RPB[i].frameType & MFX_FRAMETYPE_REF) && std::abs(i - cnt / 2) < std::abs(RPBidx - cnt / 2))
                    RPBidx = i; // Bref closest to center of consequent B
        }
    }

    RefState state; // create state
    PictureInfo& picture = state.picture;

    picture = RPB[RPBidx];           // take from reordered
    RPB.erase(RPB.begin() + RPBidx); // and remove
    picture.codingOrder = codingOrder;

    if (picture.frameType & MFX_FRAMETYPE_IDR)
        DPB.clear();
    // current frame occupies DPB by spec, free one place
    if (DPB.size() == maxDPBSize)
        DPB.erase(std::min_element(DPB.begin(), DPB.end(), IsOlder));

    // fill list randomly from DPB
    mfxU32 RefListNum[2] = { 0, 0 };
    if (!DPB.empty())
    {
        if (picture.frameType & MFX_FRAMETYPE_P)
            RefListNum[0] = m_params.m_NumRefActiveP;
        else if (picture.frameType & MFX_FRAMETYPE_B)
        {
            RefListNum[0] = m_params.m_NumRefActiveBL0;
            RefListNum[1] = m_params.m_NumRefActiveBL1;
        }
        if (picture.frameType & MFX_FRAMETYPE_P && m_params.m_UseGPB)
        {
            picture.frameType &= ~MFX_FRAMETYPE_P;
            picture.frameType |= MFX_FRAMETYPE_B;
            RefListNum[1] = std::min(m_params.m_NumRefActiveP, m_params.m_NumRefActiveBL1);
        }
    }

    for (mfxU32 list = 0; list < 2; list++)
    {
        state.RefListActive[list].clear();
        for (mfxU32 pic = 0; pic < RefListNum[list]; pic++)
        {
            state.RefListActive[list].push_back(DPB[GetRandomGen().GetRandomNumber(0, (mfxI32)DPB.size() - 1)].orderCount);
        }
    }

    state.DPB.clear();
    for (auto pic : DPB)
        state.DPB.push_back(pic.orderCount);
    std::sort(state.DPB.begin(), state.DPB.end()); // to simplify matching

                                                   // store state and put reference picture into DPB
    RefLogInfo.emplace_back(state);
    if (picture.frameType & MFX_FRAMETYPE_REF)
        DPB.emplace_back(picture);
}

// assign types in display order
// don't care about fields - must work
void RefControl::Add(mfxI32 frameOrder)
{
    PictureInfo picture;
    picture.codingOrder = -1; // randomly decided later
    picture.orderCount = frameOrder;
    picture.picStruct = GetRandomGen().GetRandomPicField();

    mfxU16 type = 0;
    mfxI32 poc = frameOrder - m_lastIDR;
    mfxI32 nextIDRpoc = m_params.m_nIdrInterval*m_params.m_GopSize;
    mfxI32 pocEnd = std::min(nextIDRpoc, static_cast<mfxI32>(m_params.m_numFrames) - m_lastIDR); // last before IDR or last picture

    if (poc == 0 || poc == nextIDRpoc)
    {
        type = MFX_FRAMETYPE_IDR | MFX_FRAMETYPE_REF | MFX_FRAMETYPE_I;
        m_lastIDR = frameOrder;
    }
    else if (poc % m_params.m_GopSize == 0)
        type = MFX_FRAMETYPE_REF | MFX_FRAMETYPE_I;
    else  if (poc % m_params.m_GopSize % m_params.m_RefDist == 0 || poc + 1 == pocEnd) // also last before IDR or last picture
        type = MFX_FRAMETYPE_REF | MFX_FRAMETYPE_P;
    else
    {
        type = MFX_FRAMETYPE_B;
        if (m_params.m_BRefType != MFX_B_REF_OFF)
        {
            mfxI32 bord = poc % m_params.m_GopSize % m_params.m_RefDist; // pos in consequent B [1; dist-1]
            mfxI32 s = (pocEnd - (poc - bord) > m_params.m_RefDist) ? m_params.m_RefDist : pocEnd - (poc - bord) - 1; // real num B
            for (; bord > 1 && s > 2; ) // 1: next to refpoint, 2: max span w/o ref
            {
                mfxI32 nexts = (s + 1) / 2; // middle (ref) point; left span is bigger for odds
                if (bord == nexts) // if in the middle - mark as ref
                {
                    type |= MFX_FRAMETYPE_REF;
                    break;
                }
                else if (bord > nexts) // shift to right span
                {
                    bord -= nexts;
                    s -= nexts;
                }
                else {
                    s = nexts;
                }
            }
        }
    }

    picture.frameType = type;

    RPB.emplace_back(picture);
}

bool IsOlder(const PictureInfo& a, const PictureInfo& b) { return a.orderCount < b.orderCount; }

bool IsInCodingOrder(const RefState& a, const RefState& b) { return a.picture.codingOrder < b.picture.codingOrder; }

#endif // MFX_VERSION
