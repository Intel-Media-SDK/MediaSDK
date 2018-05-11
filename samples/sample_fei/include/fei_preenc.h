/******************************************************************************\
Copyright (c) 2005-2018, Intel Corporation
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

#ifndef __SAMPLE_FEI_PREENC_INTERFACE_H__
#define __SAMPLE_FEI_PREENC_INTERFACE_H__

#include "encoding_task_pool.h"
#include "predictors_repacking.h"

#ifndef MFX_VERSION
#error MFX_VERSION not defined
#endif

class FEI_PreencInterface
{
private:
    FEI_PreencInterface(const FEI_PreencInterface& other_preenc);             // forbidden
    FEI_PreencInterface& operator= (const FEI_PreencInterface& other_preenc); // forbidden

public:
    MFXVideoSession* m_pmfxSession;
    MFXVideoENC*     m_pmfxPREENC;
    MFXVideoVPP*     m_pmfxDS;
    iTaskPool*       m_inputTasks;
    mfxU32           m_allocId;
    mfxVideoParam    m_videoParams;
    mfxVideoParam    m_DSParams;
    mfxVideoParam    m_FullResParams; // This parameter emulates mfxVideoParams of full-res surfaces (required for proper surfaces allocation for pipeline: YUV->VPP->DS->PreENC)
    bufList*         m_pExtBuffers;
    bufList*         m_pEncExtBuffers;
    AppConfig*       m_pAppConfig;
    mfxSyncPoint     m_SyncPoint;
    bool             m_bSingleFieldMode;
    mfxU8            m_DSstrength;

    bool             m_bMVout;
    bool             m_bMBStatout;

    mfxExtFeiPreEncMV::mfxExtFeiPreEncMVMB m_tmpMVMB;

    /* For I/O operations with extension buffers */
    FILE* m_pMvPred_in;
    FILE* m_pMbQP_in;
    FILE* m_pMBstat_out;
    FILE* m_pMV_out;

    std::vector<mfxExtBuffer*> m_InitExtParams;
    std::vector<mfxExtBuffer*> m_DSExtParams;

    /* Temporary memory to speed up computations */
    std::vector<mfxI16> m_tmpForMedian;

    FEI_PreencInterface(MFXVideoSession* session, iTaskPool* task_pool, mfxU32 allocId, bufList* ext_bufs, bufList* enc_ext_bufs, AppConfig* config);
    ~FEI_PreencInterface();

    mfxStatus Init();
    mfxStatus Close();
    mfxStatus Reset(mfxU16 width = 0, mfxU16 height = 0, mfxU16 crop_w = 0, mfxU16 crop_h = 0);
    mfxStatus QueryIOSurf(mfxFrameAllocRequest ds_request[2]);
    mfxVideoParam* GetCommonVideoParams();
    mfxStatus UpdateVideoParam();

    void GetRefInfo(mfxU16 & picStruct,
                    mfxU16 & refDist,
                    mfxU16 & numRefFrame,
                    mfxU16 & gopSize,
                    mfxU16 & gopOptFlag,
                    mfxU16 & idrInterval,
                    mfxU16 & numRefActiveP,
                    mfxU16 & numRefActiveBL0,
                    mfxU16 & numRefActiveBL1,
                    mfxU16 & bRefType,
                    bool   & bSigleFieldProcessing);

    mfxStatus FillDSVideoParams();
    mfxStatus FillParameters();
    mfxStatus PreencOneFrame(iTask* eTask);
    mfxStatus DownSampleInput(iTask* eTask);
    mfxStatus InitFrameParams(iTask* eTask, iTask* refTask[2][2], mfxU8 ref_fid[2][2], bool isDownsamplingNeeded);
    mfxStatus ProcessMultiPreenc(iTask* eTask);
    mfxStatus GetRefTaskEx(iTask *eTask, mfxU8 l0_idx, mfxU8 l1_idx, mfxU8 refIdx[2][2], mfxU8 ref_fid[2][2], iTask *outRefTask[2][2]);
    mfxStatus RepackPredictors(iTask* eTask);

    void UpsampleMVP(mfxExtFeiPreEncMV::mfxExtFeiPreEncMVMB * preenc_MVMB, mfxU32 MBindex_DS, mfxExtFeiEncMVPredictors* mvp, mfxU32 predIdx, mfxU8 refIdx, mfxU32 L0L1);
    mfxStatus RepackPredictorsPerf(iTask* eTask);
    mfxStatus FlushOutput(iTask* eTask);
    mfxStatus ResetState();
};

#endif // __SAMPLE_FEI_PREENC_INTERFACE_H__
