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

#ifndef __VERIFIER_H__
#define __VERIFIER_H__

#include "mfxvideo.h"

#if MFX_VERSION >= MFX_VERSION_NEXT

#include "refcontrol.h"
#include "test_processor.h"

class Verifier : public TestProcessor {
public:
    explicit Verifier() {}

    ~Verifier() {};

    // Verification mode
    void CheckStatistics(Thresholds thres);

private:
    struct MVCompareBlock : BaseBlock
    {
        bool predFlagL0 = false;
        bool predFlagL1 = false;
        bool bIsIntra = false;
        mfxI16 usedMVPIndex  = -1;
        mfxI16 usedMVPPoolNo = -1;
        PUMotionVector MV;
    };

    const mfxU32 MV_CMP_BLOCK_SIZE = (1 << LOG2_MV_COMPARE_BASE_BLOCK_SIZE);

    std::list<std::string> m_errorMsgList;

    void Init() override;

    // Get surface
    ExtendedSurface* PrepareSurface() override;

    // Save all data
    void DropFrames() override;
    virtual void SavePSData() override;

    // Verification mode
    void VerifyDesc(FrameChangeDescriptor & frame_descr) override;
    void ExtractMVs(const CTUDescriptor& ctuDescr, std::vector<MVCompareBlock>& mvCmpBlocks);
    void CompareSplits(const CTUDescriptor& ctuDescrASG, const CTUDescriptor& ctuDescrFEI);

    void CompareMVs(const CTUDescriptor& asgCTUDescriptor, const CTUDescriptor& feiCTUDescriptor);
    void CompareMVBlocks(const MVCompareBlock& asgMVCmpBlock, const MVCompareBlock& feiMVCmpBlock);
    void CalculateTotalMVCmpBlocksInCTU(std::vector<MVCompareBlock>& mvCmpBlocks);

    void CountExactMatches(const CTUDescriptor& asgCTUDescriptor, const CTUDescriptor& feiCTUDescriptor);
    CTUDescriptor ConvertFeiOutInLocalStr(const mfxFeiHevcPakCtuRecordV0& ctuPakFromFEI, const ExtendedBuffer& tmpCuBuff, const mfxU32 startCuOffset);

    void PrintPercentRatio(mfxU32 numerator, mfxU32 denominator);
    bool CheckLowerThreshold(mfxU32 numerator, mfxU32 denominator, mfxU32 threshold = 100);

    void CheckMVs(Thresholds threshold);
    void CheckL0MVs(Thresholds threshold);
    void CheckL1MVs(Thresholds threshold);
    void CheckBiMVs(Thresholds threshold);

    void CheckL0MVsPerMVPIndex(Thresholds threshold);
    void CheckL1MVsPerMVPIndex(Thresholds threshold);
    void CheckBiMVsPerMVPIndex(Thresholds threshold);

    void CheckSplits(Thresholds threshold);
    void CheckPicStruct();

    BufferReader m_BufferReader;
    Counters m_Counters;
};

#endif // MFX_VERSION

#endif // __VERIFIER_H__
