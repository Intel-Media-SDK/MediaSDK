/******************************************************************************\
Copyright (c) 2005-2017, Intel Corporation
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

#ifndef __SAMPLE_FEI_ENCPAK_INTERFACE_H__
#define __SAMPLE_FEI_ENCPAK_INTERFACE_H__

#include "encoding_task_pool.h"
#include "predictors_repacking.h"

class FEI_EncPakInterface
{
private:
    FEI_EncPakInterface(const FEI_EncPakInterface& other_encpak);             // forbidden
    FEI_EncPakInterface& operator= (const FEI_EncPakInterface& other_encpak); // forbidden

public:
    MFXVideoSession*  m_pmfxSession;
    MFXVideoENC*      m_pmfxENC;
    MFXVideoPAK*      m_pmfxPAK;
    iTaskPool*        m_inputTasks;
    mfxVideoParam     m_videoParams_ENC;
    mfxVideoParam     m_videoParams_PAK;
    mfxU32            m_allocId;
    bufList*          m_pExtBuffers;
    AppConfig*        m_pAppConfig;
    mfxBitstream      m_mfxBS;
    mfxSyncPoint      m_SyncPoint;
    bool              m_bSingleFieldMode;
    RefInfo           m_RefInfo;

    /* Bitstream writer */
    CSmplBitstreamWriter m_FileWriter;

    /* For I/O operations with extension buffers */
    FILE* m_pMvPred_in;
    FILE* m_pENC_MBCtrl_in;
    FILE* m_pMbQP_in;
    FILE* m_pRepackCtrl_in;
    FILE* m_pMBstat_out;
    FILE* m_pMV_out;
    FILE* m_pMBcode_out;

    std::vector<mfxExtBuffer*> m_InitExtParams_ENC, m_InitExtParams_PAK;

    /* Temporary memory to speed up computations */
    std::vector<mfxI16> m_tmpForMedian;
    std::vector<mfxExtFeiPreEncMV::mfxExtFeiPreEncMVMB> m_tmpForReading;

    mfxExtFeiEncMV::mfxExtFeiEncMVMB m_tmpMBencMV;

    FEI_EncPakInterface(MFXVideoSession* session, iTaskPool* task_pool, mfxU32 allocId, bufList* ext_bufs, AppConfig* config);
    ~FEI_EncPakInterface();

    mfxStatus Init();
    mfxStatus Close();
    mfxStatus Reset(mfxU16 width = 0, mfxU16 height = 0, mfxU16 crop_w = 0, mfxU16 crop_h = 0);
    mfxStatus QueryIOSurf(mfxFrameAllocRequest* request);
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

    mfxStatus FillParameters();
#if MFX_VERSION < 1023
    mfxStatus FillRefInfo(iTask* eTask);
#endif // MFX_VERSION < 1023
    mfxStatus InitFrameParams(iTask* eTask);
    mfxStatus AllocateSufficientBuffer();
    mfxStatus EncPakOneFrame(iTask* eTask);
    mfxStatus FlushOutput(iTask* eTask);
    mfxStatus ResetState();

    // Decode StreamOut -> PAK
    mfxStatus PakOneStreamoutFrame(iTask *eTask, mfxU8 QP, iTaskPool *pTaskList);
};

#endif // __SAMPLE_FEI_ENCPAK_INTERFACE_H__
