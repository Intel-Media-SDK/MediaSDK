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

#ifndef __TRANSCODE_UTILS_H__
#define __TRANSCODE_UTILS_H__

#if defined(_WIN32) || defined(_WIN64)
#include <process.h>
#pragma warning(disable : 4201)
#include <d3d9.h>
#include <dxva2api.h>
#endif

#include <vector>
#include <map>
#include "pipeline_transcode.h"

struct D3DAllocatorParams;

#ifdef _MSC_VER
#pragma warning(disable: 4127) // constant expression
#endif

#ifndef MFX_VERSION
#error MFX_VERSION not defined
#endif

namespace TranscodingSample
{
    struct sInputParams;

    msdk_tick GetTick();
    mfxF64 GetTime(msdk_tick start);

    void PrintHelp();
    void PrintError(const msdk_char *strErrorMessage, ...);
    void PrintInfo(mfxU32 session_number, sInputParams* pParams, mfxVersion *pVer);

    bool PrintDllInfo(msdk_char *buf, mfxU32 buf_size, sInputParams* pParams);

    class CmdProcessor
    {
    public:
        CmdProcessor();
        virtual ~CmdProcessor();
        mfxStatus ParseCmdLine(int argc, msdk_char *argv[]);
        bool GetNextSessionParams(TranscodingSample::sInputParams &InputParams);
        FILE*     GetPerformanceFile() {return m_PerfFILE;};
        void      PrintParFileName();
        msdk_string GetLine(mfxU32 n);
    protected:
        mfxStatus ParseParFile(FILE* file);
        mfxStatus TokenizeLine(msdk_char *pLine, mfxU32 length);
        mfxU32    GetStringLength(msdk_char *pTempLine, mfxU32 length);

#if MFX_VERSION >= 1022
        static bool isspace(char a);
        static bool is_not_allowed_char(char a);
        bool ParseROIFile(msdk_char const *roi_file_name, std::vector<mfxExtEncoderROI>& m_ROIData);
#endif //MFX_VERSION >= 1022

        mfxStatus ParseParamsForOneSession(mfxU32 argc, msdk_char *argv[]);
        mfxStatus ParseOption__set(msdk_char* strCodecType, msdk_char* strPluginPath);
        mfxStatus VerifyAndCorrectInputParams(TranscodingSample::sInputParams &InputParams);
        mfxU32                                       m_SessionParamId;
        std::vector<TranscodingSample::sInputParams> m_SessionArray;
        std::map<mfxU32, sPluginParams>              m_decoderPlugins;
        std::map<mfxU32, sPluginParams>              m_encoderPlugins;
        FILE                                         *m_PerfFILE;
        msdk_char                                    *m_parName;
        mfxU32                                       statisticsWindowSize;
        FILE                                         *statisticsLogFile;
        //store a name of a Logfile
        msdk_tstring                                 DumpLogFileName;
        mfxU32                                       m_nTimeout;
        bool                                         bRobustFlag;
        bool                                         bSoftRobustFlag;
        bool                                         shouldUseGreedyFormula;
        std::vector<msdk_string>                     m_lines;
    private:
        DISALLOW_COPY_AND_ASSIGN(CmdProcessor);

    };
}
#endif //__TRANSCODE_UTILS_H__

