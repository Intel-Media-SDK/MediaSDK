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

#ifndef __SAMPLE_MULTI_TRANSCODE_H__
#define __SAMPLE_MULTI_TRANSCODE_H__

#include "transcode_utils.h"
#include "pipeline_transcode.h"
#include "sample_utils.h"

#include "d3d_allocator.h"
#include "d3d11_allocator.h"
#include "general_allocator.h"
#include "hw_device.h"
#include "d3d_device.h"
#include "d3d11_device.h"

#ifdef LIBVA_SUPPORT
#include "vaapi_device.h"
#include "vaapi_utils.h"
#include "vaapi_allocator.h"
#endif

namespace TranscodingSample
{
    class Launcher
    {
    public:
        Launcher();
        virtual ~Launcher();

        virtual mfxStatus Init(int argc, msdk_char *argv[]);
        virtual void      Run();
        virtual mfxStatus ProcessResult();

    protected:
        virtual mfxStatus VerifyCrossSessionsOptions();
        virtual mfxStatus CreateSafetyBuffers();

        virtual void Close();

        // command line parser
        CmdProcessor m_parser;
        // sessions to process playlist
        std::vector<ThreadTranscodeContext*> m_pSessionArray;
        // handles
        typedef std::list<MSDKThread*>::iterator MSDKThreadsIterator;
        std::list<MSDKThread*>             m_HDLArray;
        // allocator for each session
        std::vector<GeneralAllocator*>       m_pAllocArray;
        // input parameters for each session
        std::vector<sInputParams>            m_InputParamsArray;
        // safety buffers
        // needed for heterogeneous pipeline
        std::vector<SafetySurfaceBuffer*>    m_pBufferArray;

        std::vector<FileBitstreamProcessor*> m_pExtBSProcArray;
        std::auto_ptr<mfxAllocatorParams>    m_pAllocParam;
        std::auto_ptr<CHWDevice>             m_hwdev;
        msdk_tick                            m_StartTime;
        // need to work with HW pipeline
        mfxHandleType                        m_eDevType;

        std::vector<sVppCompDstRect>         m_VppDstRects;

    private:
        DISALLOW_COPY_AND_ASSIGN(Launcher);

    };
}

#endif