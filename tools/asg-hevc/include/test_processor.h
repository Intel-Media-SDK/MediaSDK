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

#ifndef __ASG_HEVC_TEST_PROCESSOR_H__
#define __ASG_HEVC_TEST_PROCESSOR_H__

#include "mfxvideo.h"
#include "sample_defs.h"

#if MFX_VERSION >= MFX_VERSION_NEXT

#include <fstream>

#include "sample_utils.h"
#include "base_allocator.h"

#include "inputparameters.h"
#include "frame_processor.h"
#include "util_defs.h"
#include "asglog.h"

#include "refcontrol.h"

class TestProcessor
{
public:
    virtual ~TestProcessor() {}

    void RunTest(const InputParams & params);
    void RunPSTest(const InputParams & params);

    void RunRepackGenerate();
    void RunRepackVerify();

    static ASGLog asgLog;

protected:
    void Init(const InputParams &params);
    virtual void Init() = 0;

    explicit TestProcessor() : fpPicStruct(), fpRepackCtrl(), fpRepackStr(), fpRepackStat() {}

    ExtendedSurface* GetFreeSurf();

    // Get surface
    virtual ExtendedSurface* PrepareSurface() { return nullptr; }

    void ChangePicture(ExtendedSurface& surf);

    void PrepareDescriptor(FrameChangeDescriptor & descr, const mfxU32 frameNum);

    // Save all data
    virtual void DropFrames() {}
    virtual void DropBuffers(ExtendedSurface& surf) {}
    virtual void SavePSData() = 0;

    // Verification mode
    virtual void VerifyDesc(FrameChangeDescriptor & frame_descr) {}

    InputParams    m_InputParams;
    std::vector<FrameProcessingParam> m_vProcessingParams;    // markers for all frames
    std::list<FrameChangeDescriptor>  m_ProcessedFramesDescr; // DPB analogue
    FrameProcessor                    m_FrameProcessor;       // Class which performs processing of frame
    std::list<ExtendedSurface>        m_Surfaces;             // Surface pool

    std::fstream fpPicStruct;
    RefControl m_RefControl;

    std::fstream fpRepackCtrl;
    std::fstream fpRepackStr;
    std::fstream fpRepackStat;

private:
    //used to be global

    // used once in TestProcessor
    std::list<FrameChangeDescriptor> GetReferences(const std::list<FrameChangeDescriptor> & RecentProcessed, const std::vector<mfxU32>& ref_idx);
    // never used actually
    std::list<FrameChangeDescriptor> GetSkips(const std::list<FrameChangeDescriptor> & RecentProcessed);
};

#endif // MFX_VERSION

#endif // __ASG_HEVC_TEST_PROCESSOR_H__
