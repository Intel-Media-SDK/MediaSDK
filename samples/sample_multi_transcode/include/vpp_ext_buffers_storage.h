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

#ifndef __VPP_EXT_BUFFERS_STORAGE_H__
#define __VPP_EXT_BUFFERS_STORAGE_H__

#include "mfxvideo.h"
#include "mfxvideo++.h"
#include "mfxstructures.h"
#include "sample_defs.h"
#include <vector>

#ifndef MFX_VERSION
#error MFX_VERSION not defined
#endif

namespace TranscodingSample
{

// to enable to-do list
// number of video enhancement filters (denoise, procamp, detail, video_analysis, multi_view, ste, istab, tcc, ace, svc)
#define ENH_FILTERS_COUNT                (20)

    struct sInputParams;

    class CVPPExtBuffersStorage
    {
    public:
        CVPPExtBuffersStorage(void);
        ~CVPPExtBuffersStorage(void);
        mfxStatus Init(TranscodingSample::sInputParams* params);
        void Clear();

        std::vector<mfxExtBuffer*> ExtBuffers;
        /* VPP extension */
        mfxExtVppAuxData*   pExtVPPAuxData;
        mfxExtVPPDoUse      extDoUse;
        mfxU32              tabDoUseAlg[ENH_FILTERS_COUNT];
        mfxExtBuffer*       pExtBuf[1 + ENH_FILTERS_COUNT];


        static mfxStatus ParseCmdLine(msdk_char *argv[],mfxU32 argc,mfxU32& index,TranscodingSample::sInputParams* params,mfxU32& skipped);

    protected:
#ifdef ENABLE_MCTF
        mfxExtVppMctf mctfFilter;
#endif
        mfxExtVPPDenoise denoiseFilter;
        mfxExtVPPDetail detailFilter;
        mfxExtVPPFrameRateConversion frcFilter;
        mfxExtVPPDeinterlacing deinterlaceFilter;
        mfxExtVPPFieldProcessing vppFieldProcessingFilter;
    };
}

#endif