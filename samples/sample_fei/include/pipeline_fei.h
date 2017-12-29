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

#ifndef __PIPELINE_FEI_H__
#define __PIPELINE_FEI_H__

#include "encoding_task_pool.h"
#include "predictors_repacking.h"
#include "fei_encode.h"
#include "fei_preenc.h"
#include "fei_encpak.h"
#include "auxiliary_interfaces.h"

#include "sysmem_allocator.h"
#include "vaapi_allocator.h"
#include "vaapi_device.h"
#include "hw_device.h"

#include "mfx_itt_trace.h"

#if (MFX_VERSION >= 1024)
#include "brc_routines.h"
#endif

/* This class implements a FEI pipeline */
class CEncodingPipeline
{
public:
    CEncodingPipeline(AppConfig* pAppConfig);
    virtual ~CEncodingPipeline();

    virtual mfxStatus Init();
    virtual mfxStatus Run();
    virtual mfxStatus ProcessBufferedFrames();
    virtual void Close();
    virtual mfxStatus ResetMFXComponents();
    virtual mfxStatus InitSessions();
    virtual mfxStatus ResetSessions();
    virtual mfxStatus ResetDevice();

    virtual void  PrintInfo();

protected:
    AppConfig   m_appCfg; // this structure holds information about current pipeline setup

    iTaskPool   m_inputTasks; // pool of input tasks, used for reordering and reflists management
    iTaskParams m_taskInitializationParams; // holds all necessary data for task initializing

    mfxU16 m_nAsyncDepth; // depth of asynchronous pipeline (FEI supports only AsyncDepth = 1 fro current limitations)
    mfxU16 m_picStruct;
    mfxU16 m_refDist;
    mfxU16 m_numRefFrame;
    mfxU16 m_gopSize;
    mfxU16 m_gopOptFlag;
    mfxU16 m_idrInterval;
    mfxU16 m_numRefActiveP;
    mfxU16 m_numRefActiveBL0;
    mfxU16 m_numRefActiveBL1;
    mfxU16 m_bRefType;
    mfxU16 m_numOfFields;
    mfxU16 m_decodePoolSize;
    mfxU16 m_heightMB;
    mfxU16 m_widthMB;
    mfxU16 m_widthMBpreenc;
    mfxU16 m_heightMBpreenc;

    mfxU16 m_maxQueueLength;
    mfxU16 m_log2frameNumMax;
    mfxU32 m_frameCount;
#if (MFX_VERSION >= 1024)
    mfxU32 m_frameCountInEncodedOrder;
#endif
    mfxU32 m_frameOrderIdrInDisplayOrder;
    PairU8 m_frameType;

    mfxFrameInfo m_commonFrameInfo; // setting for ENCODE (VPP / PreENC with DS may have own FrameInfo settings)

    bufList m_preencBufs, m_encodeBufs; // sets of extension buffers for PreENC and ENCODE(ENC+PAK)

    // decode streamout
    std::vector<mfxExtFeiDecStreamOut*> m_StreamoutBufs;
    mfxExtFeiDecStreamOut* m_pExtBufDecodeStreamout; // current streamout buffer

    // Dynamic Resolution Change workflow
    mfxU32 m_nDRC_idx;
    bool m_bNeedDRC;   //True if Dynamic Resolution Change required
    std::vector<DRCblock> m_DRCqueue;

    bool m_insertIDR; // indicates forced-IDR

    bool m_bVPPneeded; //True if we have VPP in pipeline
    bool m_bSeparatePreENCSession;

    MFXVideoSession  m_mfxSession;
    MFXVideoSession  m_preenc_mfxSession;
    MFXVideoSession* m_pPreencSession;

    FEI_PreencInterface* m_pFEI_PreENC;
    FEI_EncodeInterface* m_pFEI_ENCODE;
    FEI_EncPakInterface* m_pFEI_ENCPAK;
    MFX_VppInterface*    m_pVPP;
    MFX_DecodeInterface* m_pDECODE;
    YUVreader*           m_pYUVReader;

#if (MFX_VERSION >= 1024)
    ExtBRC               m_BRC;
#endif

    MFXFrameAllocator*  m_pMFXAllocator;
    mfxAllocatorParams* m_pmfxAllocatorParams;
    bool m_bUseHWmemory;   // indicates whether hardware or software memory used
    bool m_bExternalAlloc; // use memory allocator as external for Media SDK

    bool m_bParametersAdjusted;

    CHWDevice *m_hwdev;

    SurfStrategy m_surfPoolStrategy; // strategy to pick surfaces from pool
    ExtSurfPool  m_DecSurfaces;      // frames array for decoder input
    ExtSurfPool  m_DSSurfaces;       // frames array for downscaled surfaces for PREENC
    ExtSurfPool  m_VppSurfaces;      // frames array for vpp input
    ExtSurfPool  m_EncSurfaces;      // frames array for encoder input (vpp output)
    ExtSurfPool  m_ReconSurfaces;    // frames array for reconstructed surfaces [FEI]
    mfxFrameAllocResponse m_DecResponse;    // memory allocation response for decoder
    mfxFrameAllocResponse m_VppResponse;    // memory allocation response for VPP input
    mfxFrameAllocResponse m_dsResponse;     // memory allocation response for VPP input
    mfxFrameAllocResponse m_EncResponse;    // memory allocation response for encoder
    mfxFrameAllocResponse m_ReconResponse;  // memory allocation response for encoder for reconstructed surfaces [FEI]
    mfxU32 m_BaseAllocID;


    virtual mfxStatus AllocExtBuffers();
    virtual mfxStatus SetSequenceParameters();
    virtual mfxStatus ResetIOFiles(const AppConfig & Config);

    virtual mfxStatus CreateAllocator();
    virtual void DeleteAllocator();

    virtual mfxStatus CreateHWDevice();
    virtual void DeleteHWDevice();

    virtual mfxStatus AllocFrames();
    virtual mfxStatus FillSurfacePool(mfxFrameSurface1* & surfacesPool, mfxFrameAllocResponse* allocResponse, mfxFrameInfo* FrameInfo);
    virtual void DeleteFrames();
    virtual void ReleaseResources();

    virtual mfxStatus UpdateVideoParam();

    virtual PairU8 GetFrameType(mfxU32 pos);

    virtual mfxStatus GetOneFrame(mfxFrameSurface1* & pSurf);
    virtual mfxStatus ResizeFrame(mfxU32 frameNum, mfxU16 picstruct);

    virtual mfxStatus PreProcessOneFrame(mfxFrameSurface1* & pSurf);

    virtual mfxStatus doGPUHangRecovery();

    void ClearDecoderBuffers();
    mfxStatus ResetBuffers();
    mfxU16 GetCurPicType(mfxU32 fieldId);
};

PairU8 ExtendFrameType(mfxU32 type);

#endif // __PIPELINE_FEI_H__
