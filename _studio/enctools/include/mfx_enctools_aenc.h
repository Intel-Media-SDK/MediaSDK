// Copyright (c) 2019-2020 Intel Corporation
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

#ifndef __MFX_ENCTOOLS_AENC_H__
#define __MFX_ENCTOOLS_AENC_H__

#include "mfxdefs.h"
#include <vector>
#include <memory>
#include <algorithm>
#include "aenc.h"
#include "mfx_enctools_utils.h"

#define ENC_TOOLS_DS_FRAME_WIDTH 256
#define ENC_TOOLS_DS_FRAME_HEIGHT 128

class AEnc_EncTool
{
public:
    AEnc_EncTool() :
        m_ptmpFrame(nullptr),
        m_bInit(false)
    {
        m_aencPar = {};
    }
    ~AEnc_EncTool() { Close(); }
    mfxStatus Init(mfxEncToolsCtrl const & ctrl, mfxExtEncToolsConfig const & pConfig);
    mfxStatus GetInputFrameInfo(mfxFrameInfo &frameInfo);
    void Close();
    mfxStatus SubmitFrame(mfxFrameSurface1 *surface);
    mfxStatus ReportEncResult(mfxU32 dispOrder, mfxEncToolsBRCEncodeResult const & pEncRes);
    mfxStatus GetSCDecision(mfxU32 displayOrder, mfxEncToolsHintPreEncodeSceneChange *pPreEncSC);
    mfxStatus GetGOPDecision(mfxU32 displayOrder, mfxEncToolsHintPreEncodeGOP *pPreEncGOP);
    mfxStatus GetARefDecision(mfxU32 displayOrder, mfxEncToolsHintPreEncodeARefFrames *pPreEncARef);
    mfxStatus CompleteFrame(mfxU32 displayOrder);
    bool DoDownScaling(mfxFrameInfo const & frameInfo);

protected:
    std::vector<AEncFrame>  m_outframes;
    std::vector<AEncFrame>::iterator m_frameIt;
    mfxStatus FindOutFrame(mfxU32 displayOrder);
    mfxHDL       m_aenc;
    AEncParam    m_aencPar;
    mfxU8 *m_ptmpFrame;
    bool m_bInit;
    mfxU32 FrameWidth_aligned;
    mfxU32 FrameHeight_aligned;

};

#endif


