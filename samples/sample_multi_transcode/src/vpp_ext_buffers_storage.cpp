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

#include "vpp_ext_buffers_storage.h"
#include "pipeline_transcode.h"
#include "transcode_utils.h"
#include "sample_utils.h"

#define VALUE_CHECK(val, argIdx, argName) \
{ \
    if (val) \
    { \
        PrintError(MSDK_STRING("Input argument number %d \"%s\" require more parameters"), argIdx, argName); \
        return MFX_ERR_UNSUPPORTED;\
    } \
}


using namespace TranscodingSample;

CVPPExtBuffersStorage::CVPPExtBuffersStorage(void)
{
    MSDK_ZERO_MEMORY(denoiseFilter);
    MSDK_ZERO_MEMORY(detailFilter);
    MSDK_ZERO_MEMORY(frcFilter);
    MSDK_ZERO_MEMORY(deinterlaceFilter);
    MSDK_ZERO_MEMORY(vppFieldProcessingFilter);
}


CVPPExtBuffersStorage::~CVPPExtBuffersStorage(void)
{
}

mfxStatus CVPPExtBuffersStorage::Init(TranscodingSample::sInputParams* params)
{
    if(params->DenoiseLevel!=-1)
    {
        denoiseFilter.Header.BufferId=MFX_EXTBUFF_VPP_DENOISE;
        denoiseFilter.Header.BufferSz=sizeof(denoiseFilter);
        denoiseFilter.DenoiseFactor=(mfxU16)params->DenoiseLevel;

        ExtBuffers.push_back((mfxExtBuffer*)&denoiseFilter);
    }

    if(params->DetailLevel!=-1)
    {
        detailFilter.Header.BufferId=MFX_EXTBUFF_VPP_DETAIL;
        detailFilter.Header.BufferSz=sizeof(detailFilter);
        detailFilter.DetailFactor=(mfxU16)params->DetailLevel;

        ExtBuffers.push_back((mfxExtBuffer*)&detailFilter);
    }

    if(params->FRCAlgorithm)
    {
        memset(&frcFilter,0,sizeof(frcFilter));
        frcFilter.Header.BufferId=MFX_EXTBUFF_VPP_FRAME_RATE_CONVERSION;
        frcFilter.Header.BufferSz=sizeof(frcFilter);
        frcFilter.Algorithm=params->FRCAlgorithm;

        ExtBuffers.push_back((mfxExtBuffer*)&frcFilter);
    }

    if(params->bEnableDeinterlacing && params->DeinterlacingMode)
    {
        memset(&deinterlaceFilter,0,sizeof(deinterlaceFilter));
        deinterlaceFilter.Header.BufferId=MFX_EXTBUFF_VPP_DEINTERLACING;
        deinterlaceFilter.Header.BufferSz=sizeof(deinterlaceFilter);
        deinterlaceFilter.Mode=params->DeinterlacingMode;

        ExtBuffers.push_back((mfxExtBuffer*)&deinterlaceFilter);
    }

    //--- Field Copy Mode
    if (params->fieldProcessingMode)
    {
        vppFieldProcessingFilter.Header.BufferId = MFX_EXTBUFF_VPP_FIELD_PROCESSING;
        vppFieldProcessingFilter.Header.BufferSz = sizeof(vppFieldProcessingFilter);

        //--- To check first is we do copy frame of field
        vppFieldProcessingFilter.Mode = (mfxU16) (params->fieldProcessingMode==FC_FR2FR ?
            MFX_VPP_COPY_FRAME :
            MFX_VPP_COPY_FIELD);

        vppFieldProcessingFilter.InField = (mfxU16) ((params->fieldProcessingMode==FC_T2T || params->fieldProcessingMode==FC_T2B) ?
            MFX_PICSTRUCT_FIELD_TFF :
            MFX_PICSTRUCT_FIELD_BFF);

        vppFieldProcessingFilter.OutField = (mfxU16) ((params->fieldProcessingMode== FC_T2T || params->fieldProcessingMode==FC_B2T) ?
            MFX_PICSTRUCT_FIELD_TFF :
            MFX_PICSTRUCT_FIELD_BFF);

        ExtBuffers.push_back((mfxExtBuffer *)&vppFieldProcessingFilter);
    }


    return MFX_ERR_NONE;
}

void CVPPExtBuffersStorage::Clear()
{
    ExtBuffers.clear();
}

mfxStatus CVPPExtBuffersStorage::ParseCmdLine(msdk_char *argv[],mfxU32 argc,mfxU32& index,TranscodingSample::sInputParams* params,mfxU32& skipped)
{
    if (0 == msdk_strcmp(argv[index], MSDK_STRING("-denoise")))
    {
        VALUE_CHECK(index+1 == argc, index, argv[index]);
        index++;
        if (MFX_ERR_NONE != msdk_opt_read(argv[index], params->DenoiseLevel) || !(params->DenoiseLevel>=0 && params->DenoiseLevel<=100))
        {
            PrintError(NULL, MSDK_STRING("-denoise \"%s\" is invalid"), argv[index]);
            return MFX_ERR_UNSUPPORTED;
        }
        skipped+=2;
        return MFX_ERR_NONE;
    }
    else if (0 == msdk_strcmp(argv[index], MSDK_STRING("-detail")))
    {
        VALUE_CHECK(index+1 == argc, index, argv[index]);
        index++;
        if (MFX_ERR_NONE != msdk_opt_read(argv[index], params->DetailLevel) || !(params->DetailLevel>=0 && params->DetailLevel<=100))
        {
            PrintError(NULL, MSDK_STRING("-detail \"%s\" is invalid"), argv[index]);
            return MFX_ERR_UNSUPPORTED;
        }
        skipped+=2;
        return MFX_ERR_NONE;
    }
    else if (0 == msdk_strcmp(argv[index], MSDK_STRING("-FRC::PT")))
    {
        params->FRCAlgorithm=MFX_FRCALGM_PRESERVE_TIMESTAMP;
        return MFX_ERR_NONE;
    }
    else if (0 == msdk_strcmp(argv[index], MSDK_STRING("-FRC::DT")))
    {
        params->FRCAlgorithm=MFX_FRCALGM_DISTRIBUTED_TIMESTAMP;
        return MFX_ERR_NONE;
    }
    else if (0 == msdk_strcmp(argv[index], MSDK_STRING("-FRC::INTERP")))
    {
        params->FRCAlgorithm=MFX_FRCALGM_FRAME_INTERPOLATION;
        return MFX_ERR_NONE;
    }
    else if (0 == msdk_strcmp(argv[index], MSDK_STRING("-deinterlace")))
    {
        params->bEnableDeinterlacing = true;
        params->DeinterlacingMode=0;
        return MFX_ERR_NONE;
    }
    else if (0 == msdk_strcmp(argv[index], MSDK_STRING("-deinterlace::ADI")))
    {
        params->bEnableDeinterlacing = true;
        params->DeinterlacingMode=MFX_DEINTERLACING_ADVANCED;
        return MFX_ERR_NONE;
    }
    else if (0 == msdk_strcmp(argv[index], MSDK_STRING("-deinterlace::ADI_SCD")))
    {
        params->bEnableDeinterlacing = true;
        params->DeinterlacingMode=MFX_DEINTERLACING_ADVANCED_SCD;
        return MFX_ERR_NONE;
    }
    else if (0 == msdk_strcmp(argv[index], MSDK_STRING("-deinterlace::BOB")))
    {
        params->bEnableDeinterlacing = true;
        params->DeinterlacingMode=MFX_DEINTERLACING_BOB;
        return MFX_ERR_NONE;
    }
    else if (0 == msdk_strcmp(argv[index], MSDK_STRING("-deinterlace::ADI_NO_REF")))
    {
        params->bEnableDeinterlacing = true;
        params->DeinterlacingMode=MFX_DEINTERLACING_ADVANCED_NOREF;
        return MFX_ERR_NONE;
    }
    else if (0 == msdk_strcmp(argv[index], MSDK_STRING("-ec::rgb4")))
    {
        params->EncoderFourCC = MFX_FOURCC_RGB4;
        return MFX_ERR_NONE;
    }
    else if (0 == msdk_strcmp(argv[index], MSDK_STRING("-ec::yuy2")))
    {
        params->EncoderFourCC = MFX_FOURCC_YUY2;
        return MFX_ERR_NONE;
    }
    else if (0 == msdk_strcmp(argv[index], MSDK_STRING("-ec::nv12")))
    {
        params->EncoderFourCC = MFX_FOURCC_NV12;
        return MFX_ERR_NONE;
    }
    else if (0 == msdk_strcmp(argv[index], MSDK_STRING("-ec::nv16")))
    {
        params->EncoderFourCC = MFX_FOURCC_NV16;
        return MFX_ERR_NONE;
    }
    else if (0 == msdk_strcmp(argv[index], MSDK_STRING("-ec::p010")))
    {
        params->EncoderFourCC = MFX_FOURCC_P010;
        return MFX_ERR_NONE;
    }
    else if (0 == msdk_strcmp(argv[index], MSDK_STRING("-ec::p210")))
    {
        params->EncoderFourCC = MFX_FOURCC_P210;
        return MFX_ERR_NONE;
    }
    else if (0 == msdk_strcmp(argv[index], MSDK_STRING("-dc::rgb4")))
    {
        params->DecoderFourCC = MFX_FOURCC_RGB4;
        return MFX_ERR_NONE;
    }
    else if (0 == msdk_strcmp(argv[index], MSDK_STRING("-dc::yuy2")))
    {
        params->DecoderFourCC = MFX_FOURCC_YUY2;
        return MFX_ERR_NONE;
    }
    else if (0 == msdk_strcmp(argv[index], MSDK_STRING("-dc::nv12")))
    {
        params->DecoderFourCC = MFX_FOURCC_NV12;
        return MFX_ERR_NONE;
    }
    else if (0 == msdk_strcmp(argv[index], MSDK_STRING("-field_processing")) )
    {
        VALUE_CHECK(index+1 == argc, index, argv[index]);
        index++;
        if (0 == msdk_strcmp(argv[index], MSDK_STRING("t2t")) )
            params->fieldProcessingMode = FC_T2T;
        else if (0 == msdk_strcmp(argv[index], MSDK_STRING("t2b")) )
            params->fieldProcessingMode = FC_T2B;
        else if (0 == msdk_strcmp(argv[index], MSDK_STRING("b2t")) )
            params->fieldProcessingMode = FC_B2T;
        else if (0 == msdk_strcmp(argv[index], MSDK_STRING("b2b")) )
            params->fieldProcessingMode = FC_B2B;
        else if (0 == msdk_strcmp(argv[index], MSDK_STRING("fr2fr")) )
            params->fieldProcessingMode = FC_FR2FR;
        else
        {
            PrintError(NULL, MSDK_STRING("-field_processing \"%s\" is invalid"), argv[index]);
            return MFX_ERR_UNSUPPORTED;
        }
        return MFX_ERR_NONE;
    }

    return MFX_ERR_MORE_DATA;
}
