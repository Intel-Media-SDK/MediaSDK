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

#ifndef __ASG_FRAME_REORDER_H__
#define __ASG_FRAME_REORDER_H__

#include "mfxvideo.h"

#if MFX_VERSION >= MFX_VERSION_NEXT

#include <vector>
#include <algorithm>
#include <functional>
#include "inputparameters.h"

enum NALU_TYPE
{
    TRAIL_N = 0,
    TRAIL_R,
    RASL_N,
    RASL_R,
    IDR_W_RADL,
    CRA_NUT
};

inline bool isBFF(mfxVideoParam const & video)
{
    return ((video.mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_FIELD_BOTTOM) == MFX_PICSTRUCT_FIELD_BOTTOM);
}

mfxI32 GetFrameNum(bool bField, mfxI32 Poc, bool bSecondField);

mfxU8 GetFrameType(
    mfxVideoParam const & video,
    mfxU32                pictureOrder,
    bool                  isPictureOfLastFrame);

class FrameReorder
{
public:
    FrameReorder(const InputParams & params) :
        m_nMaxFrames(params.m_numFrames),
        m_BRefType(params.m_BRefType),
        m_GopOptFlag(params.m_GopOptFlag),
        m_NumRef(params.m_NumRef),
        m_UseGPB(params.m_UseGPB),
        m_NumRefActiveP(params.m_NumRefActiveP),
        m_NumRefActiveBL0(params.m_NumRefActiveBL0),
        m_NumRefActiveBL1(params.m_NumRefActiveBL1),
        m_lastIdr(0),
        m_anchorPOC(-1)
    {};
    ExternalFrame CreateExternalFrame(mfxI32 order, const mfxVideoParam& param);
    inline size_t GetBufferedQueueSize() const
    {
        return m_queue.size();
    };
private:
    mfxU32 m_nMaxFrames;
    mfxU16 m_BRefType;
    mfxU16 m_GopOptFlag;
    mfxU16 m_NumRef;
    bool   m_UseGPB;
    mfxU16 m_NumRefActiveP;
    mfxU16 m_NumRefActiveBL0;
    mfxU16 m_NumRefActiveBL1;

    class LastReorderedFieldInfo
    {
    public:
        mfxI32     m_poc;
        bool       m_bReference;
        bool       m_bFirstField;

        LastReorderedFieldInfo() :
            m_poc(-1),
            m_bReference(false),
            m_bFirstField(false) {}

        void Reset()
        {
            m_poc = -1;
            m_bReference = false;
            m_bFirstField = false;
        }
        void SaveInfo(Frame const & frame)
        {
            m_poc = frame.Poc;
            m_bReference = ((frame.Type & MFX_FRAMETYPE_REF) != 0);
            m_bFirstField = !frame.bSecondField;
        }
        void CorrectFrameInfo(Frame & frame)
        {
            if (!isCorrespondSecondField(frame))
                return;
            // copy params to the 2nd field
            if (m_bReference)
                frame.Type |= MFX_FRAMETYPE_REF;
        }
        bool isCorrespondSecondField(Frame const & frame)
        {
            if (m_poc + 1 != frame.Poc || !frame.bSecondField || !m_bFirstField)
                return false;
            return true;
        }
        bool bFirstField() { return m_bFirstField; }
    };

    typedef std::vector<Frame>           FrameArray;
    typedef std::vector<Frame>::iterator FrameIterator;
    FrameArray                  m_queue;
    LastReorderedFieldInfo      m_lastFieldInfo;
    FrameArray                  m_dpb;
    mfxI32                      m_lastIdr;  // for POC calculation
    mfxI32                      m_anchorPOC; // for P-Pyramid
    Frame                       m_lastFrame;

    mfxU8 GetNALUType(Frame const & frame, bool isRAPIntra);
    bool HasL1(mfxI32 poc);
    mfxU32 BRefOrder(mfxU32 displayOrder, mfxU32 begin, mfxU32 end, mfxU32 counter, bool & ref);
    mfxU32 GetBiFrameLocation(mfxU32 displayOrder, mfxU32 num, bool &ref);
    mfxU32 BPyrReorder(const std::vector<FrameIterator> & bframes);
    FrameIterator Reorder(bool flush, bool bFields);
    FrameIterator Reorder(FrameIterator begin, FrameIterator end, bool flush, bool bFields);
};

#endif // MFX_VERSION

#endif // __ASG_FRAME_REORDER_H__
