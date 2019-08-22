/******************************************************************************\
Copyright (c) 2017-2019, Intel Corporation
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
#include "hevc_fei_preenc.h"
#include "ref_list_manager.h"

/* This class implements a FEI pipeline */
class CEncodingPipeline
{
public:
    CEncodingPipeline(sInputParams& userInput);
    ~CEncodingPipeline();

    mfxStatus Init();
    mfxStatus Execute();
    void Close();

    void PrintInfo();

private:
    const sInputParams m_inParams; /* collection of user parameters, adjusted in parsing and
                                      shouldn't be modified during processing to not lose
                                      initial settings */

    mfxIMPL                                  m_impl;
    std::unique_ptr<CHWDevice>               m_pHWdev;

    std::unique_ptr<mfxAllocatorParams>      m_pMFXAllocatorParams;
    std::unique_ptr<MFXFrameAllocator>       m_pMFXAllocator;

    MFXVideoSession                          m_mfxSession;
    std::unique_ptr<MFXPlugin>               m_pDecoderPlugin;
    std::unique_ptr<MFXPlugin>               m_pHEVCePlugin;

    SurfacesPool                             m_EncSurfPool;

    std::unique_ptr<IYUVSource>              m_pYUVSource; // source of raw YUV data for encoder (e.g. YUV file reader, decoder, etc)
    std::unique_ptr<EncodeOrderControl>      m_pOrderCtrl;
    std::unique_ptr<IPreENC>                 m_pFEI_PreENC;
    std::unique_ptr<FEI_Encode>              m_pFEI_Encode;

    std::unique_ptr<HEVCEncodeParamsChecker> m_pParamChecker;

private:
    mfxStatus LoadFEIPlugin();
    mfxStatus CreateAllocator();
    mfxStatus CreateHWDevice();
    mfxStatus FillInputFrameInfo(mfxFrameInfo& fi);
    mfxStatus AllocFrames();
    mfxStatus InitComponents();
    mfxStatus DrainBufferedFrames();

    IYUVSource* CreateYUVSource();
    IPreENC*    CreatePreENC(mfxFrameInfo& in_fi);
    FEI_Encode* CreateEncode(mfxFrameInfo& in_fi);

    DISALLOW_COPY_AND_ASSIGN(CEncodingPipeline);
};

enum PIPELINE_COMPONENT {
    PREENC,
    ENCODE
};

MfxVideoParamsWrapper GetEncodeParams(const sInputParams& user_pars, const mfxFrameInfo& in_fi, PIPELINE_COMPONENT component);

#endif // __PIPELINE_FEI_H__
