/******************************************************************************\
Copyright (c) 2017, Intel Corporation
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

#ifndef __SAMPLE_FEI_ENCODE_H__
#define __SAMPLE_FEI_ENCODE_H__

#include <vector>
#include "sample_hevc_fei_defs.h"
#include "ref_list_manager.h"
#include "fei_buffer_allocator.h"
#include "fei_predictors_repacking.h"

class FEI_Encode
{
public:
    FEI_Encode(MFXVideoSession* session, mfxHDL hdl, MfxVideoParamsWrapper& encode_pars,
        const mfxExtFeiHevcEncFrameCtrl& def_ctrl, const msdk_char* dst_output,
        const msdk_char* mvpInFile, PredictorsRepaking* rpck);

    ~FEI_Encode();

    mfxStatus Init();
    mfxStatus Query();
    mfxStatus Reset(mfxVideoParam& par);
    mfxStatus QueryIOSurf(mfxFrameAllocRequest* request);

    // prepare required internal resources (e.g. buffer allocation) for component initialization
    mfxStatus PreInit();

    MfxVideoParamsWrapper   GetVideoParam();

    mfxStatus EncodeFrame(mfxFrameSurface1* pSurf);
    mfxStatus EncodeFrame(HevcTask* task);

private:
    MFXVideoSession*        m_pmfxSession;
    MFXVideoENCODE          m_mfxENCODE;

    FeiBufferAllocator    m_buf_allocator;

    MfxVideoParamsWrapper m_videoParams;
    mfxEncodeCtrlWrap     m_encodeCtrl;
    mfxBitstream          m_bitstream;
    mfxSyncPoint          m_syncPoint;

    std::string           m_dstFileName;
    CSmplBitstreamWriter  m_FileWriter; // bitstream writer

    std::auto_ptr<PredictorsRepaking> m_repacker;

    mfxExtFeiHevcEncFrameCtrl m_defFrameCtrl; // contain default per-frame options including user-specified

    mfxStatus SyncOperation();
    mfxStatus AllocateSufficientBuffer();
    mfxStatus SetCtrlParams(const HevcTask& task); // for encoded order
    mfxStatus ResetExtBuffers(const MfxVideoParamsWrapper & videoParams);

    /* For I/O operations with extension buffers */
    std::auto_ptr<FileHandler> m_pFile_MVP_in;

    DISALLOW_COPY_AND_ASSIGN(FEI_Encode);
};

#endif // __SAMPLE_FEI_ENCODE_H__
