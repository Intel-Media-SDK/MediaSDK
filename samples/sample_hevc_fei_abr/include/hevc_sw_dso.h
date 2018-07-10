/******************************************************************************\
Copyright (c) 2017-2018, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This sample was distributed or derived from the Intel's Media Samples package.
The original version of this sample may be obtained from https://software.intel.com/en-us/intel-media-server-studio
or https://software.intel.com/en-us/media-client-solutions-support.
\**********************************************************************************/

#ifndef __HEVC_SW_DSO_H__
#define __HEVC_SW_DSO_H__

#include "task.h"
#include "fei_utils.h"
#include "bs_parser++.h"

class HevcSwDso : public IYUVSource
{
public:
    HevcSwDso(const SourceFrameInfo& inPars,
              SurfacesPool* sp,
              std::shared_ptr<MVPPool> & mvpPool,
              std::shared_ptr<CTUCtrlPool> & ctuCtrlPool,
              bool calc_BRC_stat = false,
              bool dump_mvp      = false,
              bool est_dist      = false,
              DIST_EST_ALGO alg  = NNZ)
        : IYUVSource(inPars, sp)
        , m_inPars(inPars)
        , m_parser(est_dist ? BS_HEVC2::PARSE_SSD_TC : BS_HEVC2::PARSE_SSD)
        , m_mvpPool(mvpPool)
        , m_ctuCtrlPool(ctuCtrlPool)
        , m_bCalcBRCStat(calc_BRC_stat)
        , m_bDumpFinalMVPs(dump_mvp)
        , m_bEstimateDistortion(est_dist)
        , m_AlgorithmType(alg)
    {
    }

    virtual ~HevcSwDso()
    {
    }

    virtual mfxStatus SetBufferAllocator(std::shared_ptr<FeiBufferAllocator> & bufferAlloc) override
    {
        m_buffAlloc = bufferAlloc;

        return MFX_ERR_NONE;
    }

    virtual mfxStatus QueryIOSurf(mfxFrameAllocRequest* request) override { return MFX_ERR_UNSUPPORTED; }
    virtual mfxStatus PreInit() override { return MFX_ERR_UNSUPPORTED; }
    virtual mfxStatus Init()    override;
    virtual void      Close()   override { return; }

    virtual mfxStatus GetActualFrameInfo(mfxFrameInfo & info) override { return MFX_ERR_UNSUPPORTED; }
    virtual mfxStatus GetFrame(mfxFrameSurface1* & pSurf)     override { return MFX_ERR_UNSUPPORTED; }
    virtual mfxStatus GetFrame(HevcTaskDSO & task)            override;

protected:
    void FillFrameTask(const BS_HEVC2::NALU* header, HevcTaskDSO & task);
    void FillMVP(const BS_HEVC2::NALU* header, mfxExtFeiHevcEncMVPredictors & mvp, mfxU32 nMvPredictors[2]);
    void FillCtuControls(const BS_HEVC2::NALU* header, mfxExtFeiHevcEncCtuCtrl & ctuCtrls);

    void FillBRCParams(const BS_HEVC2::NALU* header, HevcTaskDSO & task);

protected:
    const SourceFrameInfo               m_inPars;
    BS_HEVC2_parser                     m_parser;

    std::shared_ptr<FeiBufferAllocator> m_buffAlloc;
    std::shared_ptr<MVPPool>            m_mvpPool;
    std::shared_ptr<CTUCtrlPool>        m_ctuCtrlPool;

private:
    bool m_bCalcBRCStat        = false;
    bool m_bDumpFinalMVPs      = false;
    bool m_bEstimateDistortion = false;

    DIST_EST_ALGO m_AlgorithmType = NNZ;

    mfxI32 m_DisplayOrderSinceLastIDR = 0, m_previousMaxDisplayOrder = -1;
    mfxU32 m_ProcessedFrames = 0;

    DISALLOW_COPY_AND_ASSIGN(HevcSwDso);
};

#endif // #define __HEVC_SW_DSO_H__
