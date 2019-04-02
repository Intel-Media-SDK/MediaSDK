/******************************************************************************\
Copyright (c) 2005-2019, Intel Corporation
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

#ifndef __PIPELINE_DECODE_H__
#define __PIPELINE_DECODE_H__

#include "sample_defs.h"

#if D3D_SURFACES_SUPPORT
#pragma warning(disable : 4201)
#include <d3d9.h>
#include <dxva2api.h>
#endif

#include <vector>
#include "hw_device.h"
#include "camera_render.h"
#include <memory>

#include "sample_camera_utils.h"

#include "sample_defs.h"
#include "sample_utils.h"
#include "base_allocator.h"

#include "mfxplugin.h"
#include "mfxplugin++.h"
#include "mfxvideo.h"
#include "mfxvideo++.h"

#include "vm/thread_defs.h"

// The only supported padding size is 8
#define CAMERA_PADDING_SIZE 8

#define align(value) ((0x10) * ( (value) / (0x10) + (((value) % (0x10)) ? 1 : 0)))
#define align_32(value) ((0x20) * ( (value) / (0x20) + (((value) % (0x20)) ? 1 : 0)))
#define align_64(value) ((0x40) * ( (value) / (0x40) + (((value) % (0x40)) ? 1 : 0)))

class CCameraPipeline
{
public:

    CCameraPipeline();
    virtual ~CCameraPipeline();

    virtual mfxStatus Init(sInputParams *pParams);
    virtual mfxStatus Run();
    virtual void Close();
    virtual mfxStatus Reset(sInputParams *pParams);

    void SetExtBuffersFlag()       { m_bIsExtBuffers = true; }
    void SetRenderingFlag()        { m_bIsRender = true; }
    void SetOutputfileFlag(bool b) { m_bOutput = b; }
    virtual void PrintInfo();

    mfxU32 GetNumberProcessedFrames() {return m_nFrameIndex;}

    mfxStatus PrepareInputSurfaces();

protected:

    bool isBayerFormat(mfxU32 type);

    mfxCam3DLutEntry*       m_3dlut_17;
    mfxCam3DLutEntry*       m_3dlut_33;
    mfxCam3DLutEntry*       m_3dlut_65;
    CBmpWriter*             m_pBmpWriter;
    CVideoReader*           m_pFileReader;
    CRawVideoWriter*        m_pRawFileWriter;
    mfxU32                  m_nInputFileIndex;
    mfxU32                  m_nFrameIndex; // index of processed frame
    mfxU32                  m_nFrameLimit; // limit number of frames to proceed

    MFXVideoSession                m_mfxSession;
    MFXVideoVPP*                   m_pmfxVPP;
    mfxVideoParam                  m_mfxVideoParams;
    std::unique_ptr<MFXVideoUSER>  m_pUserModule;
    std::unique_ptr<MFXPlugin>     m_pCamera_plugin;
    mfxPluginUID                   m_UID_Camera;

    mfxExtCamVignetteCorrection   m_Vignette;
    mfxExtCamBayerDenoise         m_Denoise;
    mfxExtCamHotPixelRemoval      m_HP;
    mfxExtCamGammaCorrection      m_GammaCorrection;
    mfxExtCamFwdGamma             m_3DLUT_GammaCorrection;
#if MFX_VERSION >= 1023
    mfxExtCamTotalColorControl    m_TotalColorControl;
    mfxExtCamCscYuvRgb             m_RGBToYUV;
#endif
    mfxExtCamBlackLevelCorrection m_BlackLevelCorrection;
    mfxExtCamWhiteBalance         m_WhiteBalance;
    mfxExtCamColorCorrection3x3   m_CCM;
    mfxExtCam3DLut                m_3DLUT;
    mfxExtCamLensGeomDistCorrection m_Lens;
    mfxExtCamPipeControl     m_PipeControl;
    mfxExtCamPadding         m_Padding;

    std::vector<mfxExtBuffer *> m_ExtBuffers;

    MFXFrameAllocator*      m_pMFXd3dAllocator;
    mfxAllocatorParams*     m_pmfxd3dAllocatorParams;
    MFXFrameAllocator*      m_pMFXsysAllocator;
    mfxAllocatorParams*     m_pmfxsysAllocatorParams;

    MFXFrameAllocator*      m_pMFXAllocatorIn;
    mfxAllocatorParams*     m_pmfxAllocatorParamsIn;
    MFXFrameAllocator*      m_pMFXAllocatorOut;
    mfxAllocatorParams*     m_pmfxAllocatorParamsOut;
    MemType                 m_memTypeOut;         // memory type of output surfaces to use
    MemType                 m_memTypeIn;          // memory type of input surfaces to use
    AccelType               m_accelType;
    bool                    m_bExternalAllocIn;  // use memory allocator as external for Media SDK
    bool                    m_bExternalAllocOut;  // use memory allocator as external for Media SDK
    mfxFrameSurface1*       m_pmfxSurfacesIn; // frames array
    mfxFrameSurface1*       m_pmfxSurfacesOut; // frames array
    mfxFrameAllocResponse   m_mfxResponseIn;  // memory allocation response
    mfxFrameAllocResponse   m_mfxResponseOut;  // memory allocation response

    mfxFrameAllocResponse   m_mfxResponseAux;  // memory allocation response
    mfxFrameSurface1*       m_pmfxSurfacesAux; // frames array

    mfxFrameAllocRequest    m_lastAllocRequest[2];

    bool                    m_bIsExtBuffers; // indicates if external buffers were allocated
    bool                    m_bIsRender; // enables rendering mode
    bool                    m_bOutput; // enables/disables output file
    bool                    m_bEnd;
    bool                    b_3DLUT_Gamma;

    mfxI32                 m_alphaValue;
    mfxU32                 m_BayerType;

    mfxU32                 m_numberOfResets;
    mfxU32                 m_resetCnt;
    mfxU32                 m_resetInterval;

    CHWDevice               *m_hwdev;
#if D3D_SURFACES_SUPPORT
    CCameraD3DRender         m_d3dRender;
#endif
    virtual mfxStatus InitMfxParams(sInputParams *pParams);

    // function for allocating a specific external buffer
    template <typename Buffer>
    mfxStatus AllocateExtBuffer();
    virtual void DeleteExtBuffers();

    virtual void AttachExtParam();

    virtual mfxStatus AllocAndInitDenoise(sInputParams *pParams);
    virtual mfxStatus AllocAndInitVignetteCorrection(sInputParams *pParams);
    virtual mfxStatus AllocAndInitCamGammaCorrection(sInputParams *pParams);
    virtual mfxStatus AllocAndInitCamWhiteBalance(sInputParams *pParams);
    virtual mfxStatus AllocAndInitCamCCM(sInputParams *pParams);
    virtual mfxStatus AllocAndInitCamLens(sInputParams *pParams);
#if MFX_VERSION >= 1023
    virtual mfxStatus AllocAndInitCamTotalColorControl(sInputParams *pParams);
    virtual mfxStatus AllocAndInitCamRGBtoYUV(sInputParams *pParams);
#endif
    virtual mfxStatus AllocAndInitCamBlackLevelCorrection(sInputParams *pParams);
    virtual mfxStatus AllocAndInitCamHotPixelRemoval(sInputParams *pParams);
    virtual void FreeCamGammaCorrection();
    virtual void FreeVignetteCorrection();

    virtual mfxStatus CreateAllocator();
    virtual mfxStatus CreateHWDevice();
    virtual mfxStatus AllocFrames();
    virtual mfxStatus ReallocFrames(mfxVideoParam *par);
    virtual void DeleteFrames();
    virtual void DeleteAllocator();

    //virtual mfxStatus PrepareInSurface(mfxFrameSurface1  **ppSurface, mfxU32 indx = 0);
};

#endif // __PIPELINE_DECODE_H__