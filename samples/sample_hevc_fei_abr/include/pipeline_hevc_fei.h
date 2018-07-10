/******************************************************************************\
Copyright (c) 2018, Intel Corporation
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

#include "sample_hevc_fei_defs.h"
#include "mfxplugin.h"
#include "mfxplugin++.h"
#include "plugin_utils.h"
#include "plugin_loader.h"
#include "vaapi_device.h"
#include "fei_utils.h"
#include "hevc_fei_encode.h"

class EncoderContext
{
public:
    EncoderContext(){}
    ~EncoderContext(){}

    mfxStatus PreInit(MFXFrameAllocator * mfxAllocator,
                      mfxHDL hdl,
                      std::shared_ptr<FeiBufferAllocator> & bufferAllocator,
                      const sInputParams & params,
                      const mfxFrameInfo & inFrameInfo);

    mfxStatus Query()
    {
        return m_encoder->Query();
    }
    mfxStatus Init()
    {
        return m_encoder->Init();
    }
    mfxStatus Reset(mfxVideoParam& par)
    {
        return m_encoder->Reset(par);
    }
    mfxStatus PreInit()
    {
        return m_encoder->PreInit();
    }
    void UpdateBRCStat(FrameStatData& stat)
    {
        m_encoder->UpdateBRCStat(stat);
    }
    MfxVideoParamsWrapper GetVideoParam()
    {
        return m_encoder->GetVideoParam();
    }
    mfxStatus QueryIOSurf(mfxFrameAllocRequest* request)
    {
        return m_encoder->QueryIOSurf(request);
    }
    mfxStatus SubmitFrame(std::shared_ptr<HevcTaskDSO> & task)
    {
        return m_encoder->SubmitFrame(task);
    }

    // Function generates common mfxVideoParam ENCODE
    // from user cmd line parameters and frame info from upstream component in pipeline.
    MfxVideoParamsWrapper GetEncodeParams(const sInputParams& userParam, const mfxFrameInfo& info);
    mfxStatus CreateEncoder(const sInputParams & params, const mfxFrameInfo & info);

private:
    mfxHDL                     m_hdl = nullptr;
    MFXVideoSession            m_mfxSession;

    MFXFrameAllocator *        m_MFXAllocator = nullptr;
    std::shared_ptr<FeiBufferAllocator> m_bufferAllocator;

    std::unique_ptr<MFXPlugin> m_pHEVCePlugin;

    std::unique_ptr<IEncoder>  m_encoder;
};

/**********************************************************************************/

/* This class implements a FEI pipeline */
class CFeiTranscodingPipeline
{
public:
    CFeiTranscodingPipeline(const std::vector<sInputParams> & inputParamsArray);
    ~CFeiTranscodingPipeline();

    mfxStatus Init();
    mfxStatus Execute();
    void Close();

    void PrintInfo();

private:
    const std::vector<sInputParams>          m_inParamsArray;  /* collection of user parameters, adjusted in parsing and
                                                                shouldn't be modified during processing to not lose
                                                                initial settings */

    mfxIMPL                                  m_impl;
    std::unique_ptr<CHWDevice>               m_pHWdev;

    std::unique_ptr<MFXFrameAllocator>       m_pMFXAllocator;
    SurfacesPool                             m_EncSurfPool;

    std::shared_ptr<FeiBufferAllocator>      m_bufferAllocator;
    std::shared_ptr<MVPPool>                 m_mvpPool;
    std::shared_ptr<CTUCtrlPool>             m_ctuCtrlPool;

    MFXVideoSession                          m_mfxSession;

    std::unique_ptr<MFXPlugin>               m_pDecoderPlugin;
    std::unique_ptr<MFXPlugin>               m_pHEVCePlugin;

    std::unique_ptr<IYUVSource>              m_source;
    std::unique_ptr<IYUVSource>              m_dso;
    std::list<std::unique_ptr<EncoderContext>> m_encoders;

    std::unique_ptr<HEVCEncodeParamsChecker> m_pParamChecker;

    mfxU32                                   m_FramesToProcess;
    mfxU32                                   m_processedFrames;

    // Look Ahead queue
    std::unique_ptr<LA_queue>                m_la_queue;

private:
    mfxStatus CreateHWDevice();

    mfxStatus InitComponents();

    IYUVSource* CreateYUVSource();
    IEncoder*   CreateEncode(mfxFrameInfo& in_fi);

    mfxStatus CreateAllocator();
    mfxStatus AllocFrames();

    mfxStatus CreateBufferAllocator(mfxU32 w, mfxU32 h);
    mfxStatus AllocBuffers();

    mfxStatus FillInputFrameInfo(mfxFrameInfo& fi);

    DISALLOW_COPY_AND_ASSIGN(CFeiTranscodingPipeline);
};

MfxVideoParamsWrapper GetEncodeParams(const sInputParams& user_pars, const mfxFrameInfo& in_fi);

#endif // __PIPELINE_FEI_H__
