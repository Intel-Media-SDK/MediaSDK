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

#include "sample_vpp_utils.h"
#include "mfxvideo++.h"
#include "vm/time_defs.h"
#include "sample_utils.h"

#include "sample_vpp_pts.h"

#include "sysmem_allocator.h"

#ifdef D3D_SURFACES_SUPPORT
#include "d3d_device.h"
#include "d3d_allocator.h"
#endif
#ifdef MFX_D3D11_SUPPORT
#include "d3d11_device.h"
#include "d3d11_allocator.h"
#endif
#ifdef LIBVA_SUPPORT
#include "vaapi_device.h"
#include "vaapi_allocator.h"
#endif

#include "general_allocator.h"
#include <algorithm>

#if defined(_WIN64) || defined(_WIN32)
#include "mfxadapter.h"
#endif

#define MFX_CHECK_STS(sts) {if (MFX_ERR_NONE != sts) return sts;}

#undef min

#ifndef MFX_VERSION
#error MFX_VERSION not defined
#endif

/* ******************************************************************* */

static
    void WipeFrameProcessor(sFrameProcessor* pProcessor);

static
    void WipeMemoryAllocator(sMemoryAllocator* pAllocator);

void ownToMfxFrameInfo( sOwnFrameInfo* in, mfxFrameInfo* out, bool copyCropParams=false);

/* ******************************************************************* */

static
    const msdk_char* FourCC2Str( mfxU32 FourCC )
{
    switch ( FourCC )
    {
    case MFX_FOURCC_NV12:
        return MSDK_STRING("NV12");
    case MFX_FOURCC_YV12:
        return MSDK_STRING("YV12");
    case MFX_FOURCC_YUY2:
        return MSDK_STRING("YUY2");
#if (MFX_VERSION >= 1028)
    case MFX_FOURCC_RGB565:
        return MSDK_STRING("RGB565");
#endif
    case MFX_FOURCC_RGB3:
        return MSDK_STRING("RGB3");
    case MFX_FOURCC_RGB4:
        return MSDK_STRING("RGB4");
#if !(defined(_WIN32) || defined(_WIN64))
    case MFX_FOURCC_RGBP:
        return MSDK_STRING("RGBP");
#endif
    case MFX_FOURCC_YUV400:
        return MSDK_STRING("YUV400");
    case MFX_FOURCC_YUV411:
        return MSDK_STRING("YUV411");
    case MFX_FOURCC_YUV422H:
        return MSDK_STRING("YUV422H");
    case MFX_FOURCC_YUV422V:
        return MSDK_STRING("YUV422V");
    case MFX_FOURCC_YUV444:
        return MSDK_STRING("YUV444");
    case MFX_FOURCC_P010:
        return MSDK_STRING("P010");
    case MFX_FOURCC_P210:
        return MSDK_STRING("P210");
    case MFX_FOURCC_NV16:
        return MSDK_STRING("NV16");
    case MFX_FOURCC_A2RGB10:
        return MSDK_STRING("A2RGB10");
    case MFX_FOURCC_UYVY:
        return MSDK_STRING("UYVY");
    case MFX_FOURCC_AYUV:
        return MSDK_STRING("AYUV");
    case MFX_FOURCC_I420:
        return MSDK_STRING("I420");
#if (MFX_VERSION >= 1027)
    case MFX_FOURCC_Y210:
        return MSDK_STRING("Y210");
    case MFX_FOURCC_Y410:
        return MSDK_STRING("Y410");
#endif
#if (MFX_VERSION >= 1031)
    case MFX_FOURCC_P016:
        return MSDK_STRING("P016");
    case MFX_FOURCC_Y216:
        return MSDK_STRING("Y216");
    case MFX_FOURCC_Y416:
        return MSDK_STRING("Y416");
#endif
    default:
        return MSDK_STRING("Unknown");
    }
}

const msdk_char* IOpattern2Str( mfxU32 IOpattern)
{
    switch ( IOpattern )
    {
    case MFX_IOPATTERN_IN_SYSTEM_MEMORY|MFX_IOPATTERN_OUT_SYSTEM_MEMORY:
        return MSDK_STRING("sys_to_sys");
    case MFX_IOPATTERN_IN_SYSTEM_MEMORY|MFX_IOPATTERN_OUT_VIDEO_MEMORY:
        return MSDK_STRING("sys_to_d3d");
    case MFX_IOPATTERN_IN_VIDEO_MEMORY|MFX_IOPATTERN_OUT_SYSTEM_MEMORY:
        return MSDK_STRING("d3d_to_sys");
    case MFX_IOPATTERN_IN_VIDEO_MEMORY|MFX_IOPATTERN_OUT_VIDEO_MEMORY:
        return MSDK_STRING("d3d_to_d3d");
    default:
        return MSDK_STRING("Not defined");
    }
}

/* ******************************************************************* */

//static
const msdk_char* PicStruct2Str( mfxU16  PicStruct )
{
    switch (PicStruct)
    {
    case MFX_PICSTRUCT_PROGRESSIVE:
        return MSDK_STRING("progressive");
    case MFX_PICSTRUCT_FIELD_TFF:
        return MSDK_STRING("interlace (TFF)");
    case MFX_PICSTRUCT_FIELD_BFF:
        return MSDK_STRING("interlace (BFF)");
    case MFX_PICSTRUCT_UNKNOWN:
        return MSDK_STRING("unknown");
    default:
        return MSDK_STRING("interlace (no detail)");
    }
}

/* ******************************************************************* */

void PrintInfo(sInputParams* pParams, mfxVideoParam* pMfxParams, MFXVideoSession *pMfxSession)
{
    mfxFrameInfo Info;

    MSDK_CHECK_POINTER_NO_RET(pParams);
    MSDK_CHECK_POINTER_NO_RET(pMfxParams);

    Info = pMfxParams->vpp.In;
    msdk_printf(MSDK_STRING("Input format\t%s\n"), FourCC2Str( Info.FourCC ));
    msdk_printf(MSDK_STRING("Resolution\t%dx%d\n"), Info.Width, Info.Height);
    msdk_printf(MSDK_STRING("Crop X,Y,W,H\t%d,%d,%d,%d\n"), Info.CropX, Info.CropY, Info.CropW, Info.CropH);
    msdk_printf(MSDK_STRING("Frame rate\t%.2f\n"), (mfxF64)Info.FrameRateExtN / Info.FrameRateExtD);
    msdk_printf(MSDK_STRING("PicStruct\t%s\n"), PicStruct2Str(Info.PicStruct));

    Info = pMfxParams->vpp.Out;
    msdk_printf(MSDK_STRING("Output format\t%s\n"), FourCC2Str( Info.FourCC ));
    msdk_printf(MSDK_STRING("Resolution\t%dx%d\n"), Info.Width, Info.Height);
    msdk_printf(MSDK_STRING("Crop X,Y,W,H\t%d,%d,%d,%d\n"), Info.CropX, Info.CropY, Info.CropW, Info.CropH);
    msdk_printf(MSDK_STRING("Frame rate\t%.2f\n"), (mfxF64)Info.FrameRateExtN / Info.FrameRateExtD);
    msdk_printf(MSDK_STRING("PicStruct\t%s\n"), PicStruct2Str(Info.PicStruct));

    msdk_printf(MSDK_STRING("\n"));
    msdk_printf(MSDK_STRING("Video Enhancement Algorithms\n"));
    msdk_printf(MSDK_STRING("Deinterlace\t%s\n"), (pParams->frameInfoIn[0].PicStruct != pParams->frameInfoOut[0].PicStruct) ? MSDK_STRING("ON"): MSDK_STRING("OFF"));
    msdk_printf(MSDK_STRING("Signal info\t%s\n"),   (VPP_FILTER_DISABLED != pParams->videoSignalInfoParam[0].mode) ? MSDK_STRING("ON"): MSDK_STRING("OFF"));
    msdk_printf(MSDK_STRING("Scaling\t\t%s\n"),     (VPP_FILTER_DISABLED != pParams->bScaling) ? MSDK_STRING("ON"): MSDK_STRING("OFF"));
#if MFX_VERSION >= 1025
    msdk_printf(MSDK_STRING("CromaSiting\t\t%s\n"), (VPP_FILTER_DISABLED != pParams->bChromaSiting) ? MSDK_STRING("ON") : MSDK_STRING("OFF"));
#endif
    msdk_printf(MSDK_STRING("Denoise\t\t%s\n"),     (VPP_FILTER_DISABLED != pParams->denoiseParam[0].mode) ? MSDK_STRING("ON"): MSDK_STRING("OFF"));
#ifdef ENABLE_MCTF
    msdk_printf(MSDK_STRING("MCTF\t\t%s\n"), (VPP_FILTER_DISABLED != pParams->mctfParam[0].mode) ? MSDK_STRING("ON") : MSDK_STRING("OFF"));
#endif

    msdk_printf(MSDK_STRING("ProcAmp\t\t%s\n"),     (VPP_FILTER_DISABLED != pParams->procampParam[0].mode) ? MSDK_STRING("ON"): MSDK_STRING("OFF"));
    msdk_printf(MSDK_STRING("DetailEnh\t%s\n"),      (VPP_FILTER_DISABLED != pParams->detailParam[0].mode)  ? MSDK_STRING("ON"): MSDK_STRING("OFF"));
    if(VPP_FILTER_DISABLED != pParams->frcParam[0].mode)
    {
        if(MFX_FRCALGM_FRAME_INTERPOLATION == pParams->frcParam[0].algorithm)
        {
            msdk_printf(MSDK_STRING("FRC:Interp\tON\n"));
        }
        else if(MFX_FRCALGM_DISTRIBUTED_TIMESTAMP == pParams->frcParam[0].algorithm)
        {
            msdk_printf(MSDK_STRING("FRC:AdvancedPTS\tON\n"));
        }
        else
        {
            msdk_printf(MSDK_STRING("FRC:\t\tON\n"));
        }
    }
    //msdk_printf(MSDK_STRING("FRC:Advanced\t%s\n"),   (VPP_FILTER_DISABLED != pParams->frcParam.mode)  ? MSDK_STRING("ON"): MSDK_STRING("OFF"));
    // MSDK 3.0
    msdk_printf(MSDK_STRING("GamutMapping \t%s\n"),       (VPP_FILTER_DISABLED != pParams->gamutParam[0].mode)      ? MSDK_STRING("ON"): MSDK_STRING("OFF") );
    msdk_printf(MSDK_STRING("ColorSaturation\t%s\n"),     (VPP_FILTER_DISABLED != pParams->tccParam[0].mode)   ? MSDK_STRING("ON"): MSDK_STRING("OFF") );
    msdk_printf(MSDK_STRING("ContrastEnh  \t%s\n"),       (VPP_FILTER_DISABLED != pParams->aceParam[0].mode)   ? MSDK_STRING("ON"): MSDK_STRING("OFF") );
    msdk_printf(MSDK_STRING("SkinToneEnh  \t%s\n"),       (VPP_FILTER_DISABLED != pParams->steParam[0].mode)       ? MSDK_STRING("ON"): MSDK_STRING("OFF") );
    msdk_printf(MSDK_STRING("MVC mode    \t%s\n"),       (VPP_FILTER_DISABLED != pParams->multiViewParam[0].mode)  ? MSDK_STRING("ON"): MSDK_STRING("OFF") );
    // MSDK 6.0
    msdk_printf(MSDK_STRING("ImgStab    \t%s\n"),            (VPP_FILTER_DISABLED != pParams->istabParam[0].mode)  ? MSDK_STRING("ON"): MSDK_STRING("OFF") );
    msdk_printf(MSDK_STRING("\n"));

    msdk_printf(MSDK_STRING("IOpattern type               \t%s\n"), IOpattern2Str( pParams->IOPattern ));
    msdk_printf(MSDK_STRING("Number of asynchronious tasks\t%d\n"), pParams->asyncNum);
    if ( pParams->bInitEx )
    {
        msdk_printf(MSDK_STRING("GPU Copy mode                \t%d\n"), pParams->GPUCopyValue);
    }
    msdk_printf(MSDK_STRING("Time stamps checking         \t%s\n"), pParams->ptsCheck ? MSDK_STRING("ON"): MSDK_STRING("OFF"));

    // info about ROI testing
    if( ROI_FIX_TO_FIX == pParams->roiCheckParam.mode )
    {
        msdk_printf(MSDK_STRING("ROI checking                 \tOFF\n"));
    }
    else
    {
        msdk_printf(MSDK_STRING("ROI checking                 \tON (seed1 = %i, seed2 = %i)\n"),pParams->roiCheckParam.srcSeed, pParams->roiCheckParam.dstSeed );
    }

    msdk_printf(MSDK_STRING("\n"));

    //-------------------------------------------------------
    mfxIMPL impl;
    pMfxSession->QueryIMPL(&impl);
    bool isHWlib = (MFX_IMPL_SOFTWARE != impl) ? true:false;

    const msdk_char* sImpl = (isHWlib) ? MSDK_STRING("hw") : MSDK_STRING("sw");
    msdk_printf(MSDK_STRING("MediaSDK impl\t%s"), sImpl);

#ifndef LIBVA_SUPPORT
    if (isHWlib || (pParams->vaType & (ALLOC_IMPL_VIA_D3D9 | ALLOC_IMPL_VIA_D3D11)))
    {
        bool  isD3D11 = (( ALLOC_IMPL_VIA_D3D11 == pParams->vaType) || (pParams->ImpLib == (MFX_IMPL_HARDWARE | MFX_IMPL_VIA_D3D11))) ?  true : false;
        const msdk_char* sIface = ( isD3D11 ) ? MSDK_STRING("VIA_D3D11") : MSDK_STRING("VIA_D3D9");
        msdk_printf(MSDK_STRING(" | %s"), sIface);
    }
#endif
    msdk_printf(MSDK_STRING("\n"));
    //-------------------------------------------------------

    if (isHWlib && !pParams->bPartialAccel)
        msdk_printf(MSDK_STRING("HW accelaration is enabled\n"));
    else
        msdk_printf(MSDK_STRING("HW accelaration is disabled\n"));

#if (defined(_WIN64) || defined(_WIN32)) && (MFX_VERSION >= 1031)
    if (pParams->bPrefferdGfx)
        msdk_printf(MSDK_STRING("dGfx adapter is preffered\n"));

    if (pParams->bPrefferiGfx)
        msdk_printf(MSDK_STRING("iGfx adapter is preffered\n"));
#endif


    mfxVersion ver;
    pMfxSession->QueryVersion(&ver);
    msdk_printf(MSDK_STRING("MediaSDK ver\t%d.%d\n"), ver.Major, ver.Minor);

    return;
}

/* ******************************************************************* */

mfxStatus ParseGUID(msdk_char strPlgGuid[MSDK_MAX_FILENAME_LEN], mfxU8 DataGUID[16])
{
    const msdk_char *uid = strPlgGuid;
    mfxU32 i   = 0;
    mfxU32 hex = 0;
    for(i = 0; i != 16; i++)
    {
        hex = 0;
#if defined(_WIN32) || defined(_WIN64)
        if (1 != _stscanf_s(uid + 2*i, L"%2x", &hex))
#else
        if (1 != sscanf(uid + 2*i, "%2x", &hex))
#endif
        {
            msdk_printf(MSDK_STRING("Failed to parse plugin uid: %s"), uid);
            return MFX_ERR_UNKNOWN;
        }
#if defined(_WIN32) || defined(_WIN64)
        if (hex == 0 && (uid + 2*i != _tcsstr(uid + 2*i, L"00")))
#else
        if (hex == 0 && (uid + 2*i != strstr(uid + 2*i, "00")))
#endif
        {
            msdk_printf(MSDK_STRING("Failed to parse plugin uid: %s"), uid);
            return MFX_ERR_UNKNOWN;
        }
        DataGUID[i] = (mfxU8)hex;
    }

    return MFX_ERR_NONE;
}

mfxStatus InitParamsVPP(MfxVideoParamsWrapper* pParams, sInputParams* pInParams, mfxU32 paramID)
{
    MSDK_CHECK_POINTER(pParams,    MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(pInParams,  MFX_ERR_NULL_PTR);

    if (pInParams->compositionParam.mode != VPP_FILTER_ENABLED_CONFIGURED && (pInParams->frameInfoIn[paramID].nWidth == 0 || pInParams->frameInfoIn[paramID].nHeight == 0)){
        vppPrintHelp(MSDK_STRING("sample_vpp"), MSDK_STRING("ERROR: Source width is not defined.\n"));
        return MFX_ERR_UNSUPPORTED;
    }
    if (pInParams->frameInfoOut[paramID].nWidth == 0 || pInParams->frameInfoOut[paramID].nHeight == 0 ){
        vppPrintHelp(MSDK_STRING("sample_vpp"), MSDK_STRING("ERROR: Source height is not defined.\n"));
        return MFX_ERR_UNSUPPORTED;
    }
    *pParams = MfxVideoParamsWrapper();
    /* input data */
    pParams->vpp.In.Shift           = pInParams->frameInfoIn[paramID].Shift;
    pParams->vpp.In.BitDepthLuma    = pInParams->frameInfoIn[paramID].BitDepthLuma;
    pParams->vpp.In.BitDepthChroma  = pInParams->frameInfoIn[paramID].BitDepthChroma;
    pParams->vpp.In.FourCC          = pInParams->frameInfoIn[paramID].FourCC;
    pParams->vpp.In.ChromaFormat    = MFX_CHROMAFORMAT_YUV420;

    pParams->vpp.In.CropX = pInParams->frameInfoIn[paramID].CropX;
    pParams->vpp.In.CropY = pInParams->frameInfoIn[paramID].CropY;
    pParams->vpp.In.CropW = pInParams->frameInfoIn[paramID].CropW;
    pParams->vpp.In.CropH = pInParams->frameInfoIn[paramID].CropH;
    pParams->vpp.In.Width = MSDK_ALIGN16(pInParams->frameInfoIn[paramID].nWidth);
    pParams->vpp.In.Height = (MFX_PICSTRUCT_PROGRESSIVE == pInParams->frameInfoIn[paramID].PicStruct)?
                MSDK_ALIGN16(pInParams->frameInfoIn[paramID].nHeight) : MSDK_ALIGN32(pInParams->frameInfoIn[paramID].nHeight);

    // width must be a multiple of 16
    // height must be a multiple of 16 in case of frame picture and
    // a multiple of 32 in case of field picture
    mfxU16 maxWidth = 0, maxHeight = 0;
    if(pInParams->compositionParam.mode == VPP_FILTER_ENABLED_CONFIGURED)
    {
        for (mfxU16 i = 0; i < pInParams->numStreams; i++)
        {
            pInParams->inFrameInfo[i].nWidth = MSDK_ALIGN16(pInParams->inFrameInfo[i].nWidth);
            pInParams->inFrameInfo[i].nHeight = (MFX_PICSTRUCT_PROGRESSIVE == pInParams->inFrameInfo[i].PicStruct)?
                MSDK_ALIGN16(pInParams->inFrameInfo[i].nHeight) : MSDK_ALIGN32(pInParams->inFrameInfo[i].nHeight);
            if (pInParams->inFrameInfo[i].nWidth > maxWidth)
                maxWidth = pInParams->inFrameInfo[i].nWidth;
            if (pInParams->inFrameInfo[i].nHeight > maxHeight)
                maxHeight = pInParams->inFrameInfo[i].nHeight;
        }

        pParams->vpp.In.Width = maxWidth;
        pParams->vpp.In.Height= maxHeight;
        pParams->vpp.In.CropX = 0;
        pParams->vpp.In.CropY = 0;
        pParams->vpp.In.CropW = maxWidth;
        pParams->vpp.In.CropH = maxHeight;

    }
    pParams->vpp.In.PicStruct = pInParams->frameInfoIn[paramID].PicStruct;

    ConvertFrameRate(pInParams->frameInfoIn[paramID].dFrameRate,
        &pParams->vpp.In.FrameRateExtN,
        &pParams->vpp.In.FrameRateExtD);

    /* output data */
    pParams->vpp.Out.Shift           = pInParams->frameInfoOut[paramID].Shift;
    pParams->vpp.Out.BitDepthLuma    = pInParams->frameInfoOut[paramID].BitDepthLuma;
    pParams->vpp.Out.BitDepthChroma  = pInParams->frameInfoOut[paramID].BitDepthChroma;
    pParams->vpp.Out.FourCC          = pInParams->frameInfoOut[paramID].FourCC;
    pParams->vpp.Out.ChromaFormat    = MFX_CHROMAFORMAT_YUV420;

    pParams->vpp.Out.CropX = pInParams->frameInfoOut[paramID].CropX;
    pParams->vpp.Out.CropY = pInParams->frameInfoOut[paramID].CropY;
    pParams->vpp.Out.CropW = pInParams->frameInfoOut[paramID].CropW;
    pParams->vpp.Out.CropH = pInParams->frameInfoOut[paramID].CropH;

    // width must be a multiple of 16
    // height must be a multiple of 16 in case of frame picture and
    // a multiple of 32 in case of field picture
    pParams->vpp.Out.Width = MSDK_ALIGN16(pInParams->frameInfoOut[paramID].nWidth);
    pParams->vpp.Out.Height= (MFX_PICSTRUCT_PROGRESSIVE == pInParams->frameInfoOut[paramID].PicStruct)?
        MSDK_ALIGN16(pInParams->frameInfoOut[paramID].nHeight) : MSDK_ALIGN32(pInParams->frameInfoOut[paramID].nHeight);
    if(pInParams->need_plugin)
    {
        mfxPluginUID mfxGuid;
        ParseGUID(pInParams->strPlgGuid, mfxGuid.Data);
        if(!memcmp(&mfxGuid,&MFX_PLUGINID_ITELECINE_HW,sizeof(mfxPluginUID)))
        {
            //CM PTIR require equal input and output frame sizes
            pParams->vpp.Out.Height = pParams->vpp.In.Height;
        }
    }

    pParams->vpp.Out.PicStruct = pInParams->frameInfoOut[paramID].PicStruct;

    ConvertFrameRate(pInParams->frameInfoOut[paramID].dFrameRate,
        &pParams->vpp.Out.FrameRateExtN,
        &pParams->vpp.Out.FrameRateExtD);

    pParams->IOPattern = pInParams->IOPattern;

    // async depth
    pParams->AsyncDepth = pInParams->asyncNum;

    return MFX_ERR_NONE;
}

/* ******************************************************************* */

#if (defined(_WIN64) || defined(_WIN32)) && (MFX_VERSION >= 1031)
mfxU32 GetPreferredAdapterNum(const mfxAdaptersInfo & adapters, const sInputParams & params)
{
    if (adapters.NumActual == 0 || !adapters.Adapters)
        return 0;

    if (params.bPrefferdGfx)
    {
        // Find dGfx adapter in list and return it's index

        auto idx = std::find_if(adapters.Adapters, adapters.Adapters + adapters.NumActual,
            [](const mfxAdapterInfo info)
        {
            return info.Platform.MediaAdapterType == mfxMediaAdapterType::MFX_MEDIA_DISCRETE;
        });

        // No dGfx in list
        if (idx == adapters.Adapters + adapters.NumActual)
        {
            msdk_printf(MSDK_STRING("Warning: No dGfx detected on machine. Will pick another adapter\n"));
            return 0;
        }

        return static_cast<mfxU32>(std::distance(adapters.Adapters, idx));
    }

    if (params.bPrefferiGfx)
    {
        // Find iGfx adapter in list and return it's index

        auto idx = std::find_if(adapters.Adapters, adapters.Adapters + adapters.NumActual,
            [](const mfxAdapterInfo info)
        {
            return info.Platform.MediaAdapterType == mfxMediaAdapterType::MFX_MEDIA_INTEGRATED;
        });

        // No iGfx in list
        if (idx == adapters.Adapters + adapters.NumActual)
        {
            msdk_printf(MSDK_STRING("Warning: No iGfx detected on machine. Will pick another adapter\n"));
            return 0;
        }

        return static_cast<mfxU32>(std::distance(adapters.Adapters, idx));
    }

    // Other ways return 0, i.e. best suitable detected by dispatcher
    return 0;
}

mfxStatus GetImpl(const mfxVideoParam & params, mfxIMPL & impl, const sInputParams & cmd_params)
{
    if (!(impl & MFX_IMPL_HARDWARE))
        return MFX_ERR_NONE;

    mfxU32 num_adapters_available;

    mfxStatus sts = MFXQueryAdaptersNumber(&num_adapters_available);
    MSDK_CHECK_STATUS(sts, "MFXQueryAdaptersNumber failed");

    mfxComponentInfo interface_request = { mfxComponentType::MFX_COMPONENT_VPP };
    interface_request.Requirements.vpp = params.vpp;

    std::vector<mfxAdapterInfo> displays_data(num_adapters_available);
    mfxAdaptersInfo adapters = { displays_data.data(), mfxU32(displays_data.size()), 0u };

    sts = MFXQueryAdapters(&interface_request, &adapters);
    if (sts == MFX_ERR_NOT_FOUND)
    {
        msdk_printf(MSDK_STRING("ERROR: No suitable adapters found for this workload\n"));
    }
    MSDK_CHECK_STATUS(sts, "MFXQueryAdapters failed");

    impl &= ~MFX_IMPL_HARDWARE;

    mfxU32 idx = GetPreferredAdapterNum(adapters, cmd_params);
    switch (adapters.Adapters[idx].Number)
    {
    case 0:
        impl |= MFX_IMPL_HARDWARE;
        break;
    case 1:
        impl |= MFX_IMPL_HARDWARE2;
        break;
    case 2:
        impl |= MFX_IMPL_HARDWARE3;
        break;
    case 3:
        impl |= MFX_IMPL_HARDWARE4;
        break;

    default:
        // Try searching on all display adapters
        impl |= MFX_IMPL_HARDWARE_ANY;
        break;
    }

    return MFX_ERR_NONE;
}
#endif // (defined(_WIN64) || defined(_WIN32)) && (MFX_VERSION >= 1031)

mfxStatus CreateFrameProcessor(sFrameProcessor* pProcessor, mfxVideoParam* pParams, sInputParams* pInParams)
{
    mfxStatus  sts = MFX_ERR_NONE;
    mfxVersion version = { {10, 1} };
    mfxIMPL    impl    = pInParams->ImpLib;

    MSDK_CHECK_POINTER(pProcessor, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(pParams,    MFX_ERR_NULL_PTR);

    WipeFrameProcessor(pProcessor);

#if (defined(_WIN64) || defined(_WIN32)) && (MFX_VERSION >= 1031)
    sts = GetImpl(*pParams, impl, *pInParams);
    MSDK_CHECK_STATUS(sts, "GetImpl failed");
#endif

    //MFX session
    if ( ! pInParams->bInitEx )
        sts = pProcessor->mfxSession.Init(impl, &version);
    else
    {
        mfxInitParamlWrap initParams;
        initParams.ExternalThreads = 0;
        initParams.GPUCopy         = pInParams->GPUCopyValue;
        initParams.Implementation  = impl;
        initParams.Version         = version;
        initParams.NumExtParam     = 0;
        sts = pProcessor->mfxSession.InitEx(initParams);
    }
    MSDK_CHECK_STATUS_SAFE(sts, "pProcessor->mfxSession.Init failed", {WipeFrameProcessor(pProcessor);});

    // Plug-in
    if ( pInParams->need_plugin )
    {
        pProcessor->plugin = true;
        ParseGUID(pInParams->strPlgGuid, pProcessor->mfxGuid.Data);
    }

    if ( pProcessor->plugin )
    {
        sts = MFXVideoUSER_Load(pProcessor->mfxSession, &(pProcessor->mfxGuid), 1);
        if (MFX_ERR_NONE != sts)
        {
            msdk_printf(MSDK_STRING("Failed to load plugin\n"));
            return sts;
        }
    }

    // VPP
    pProcessor->pmfxVPP = new MFXVideoVPP(pProcessor->mfxSession);

    return MFX_ERR_NONE;
}

/* ******************************************************************* */

mfxStatus InitFrameProcessor(sFrameProcessor* pProcessor, mfxVideoParam* pParams)
{
    mfxStatus sts = MFX_ERR_NONE;

    MSDK_CHECK_POINTER(pProcessor,          MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(pParams,             MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(pProcessor->pmfxVPP, MFX_ERR_NULL_PTR);

    // close VPP in case it was initialized
    sts = pProcessor->pmfxVPP->Close();
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_NOT_INITIALIZED);
    MSDK_CHECK_STATUS(sts, "pProcessor->pmfxVPP->Close failed");


    // init VPP
    sts = pProcessor->pmfxVPP->Init(pParams);
    return sts;
}

/* ******************************************************************* */

mfxStatus InitSurfaces(
    sMemoryAllocator* pAllocator,
    mfxFrameAllocRequest* pRequest,
    bool isInput,
    int streamIndex)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxU16    nFrames, i;

    mfxFrameAllocResponse& response = isInput ? pAllocator->responseIn[streamIndex] : pAllocator->responseOut;
    mfxFrameSurfaceWrap*& pSurfaces = isInput ? pAllocator->pSurfacesIn[streamIndex] : pAllocator->pSurfacesOut;

    sts = pAllocator->pMfxAllocator->Alloc(pAllocator->pMfxAllocator->pthis, pRequest, &response);
    MSDK_CHECK_STATUS_SAFE(sts, "pAllocator->pMfxAllocator->Alloc failed", {WipeMemoryAllocator(pAllocator);});

    nFrames = response.NumFrameActual;
    pSurfaces = new mfxFrameSurfaceWrap [nFrames];

    for (i = 0; i < nFrames; i++)
    {
        pSurfaces[i].Info = pRequest->Info;
        pSurfaces[i].Data.MemId = response.mids[i];
    }

    return sts;
}

/* ******************************************************************* */

mfxStatus InitMemoryAllocator(
    sFrameProcessor* pProcessor,
    sMemoryAllocator* pAllocator,
    mfxVideoParam* pParams,
    sInputParams* pInParams)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameAllocRequest request[2];// [0] - in, [1] - out
    //mfxFrameInfo requestFrameInfoRGB;

    MSDK_CHECK_POINTER(pProcessor,          MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(pAllocator,          MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(pParams,             MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(pProcessor->pmfxVPP, MFX_ERR_NULL_PTR);

    MSDK_ZERO_MEMORY(request);

    pAllocator->pMfxAllocator =  new GeneralAllocator;

    bool isHWLib       = (MFX_IMPL_HARDWARE & pInParams->ImpLib) ? true : false;

    if(isHWLib)
    {
        if((pInParams->ImpLib & IMPL_VIA_MASK) == MFX_IMPL_VIA_D3D9)
        {
#ifdef D3D_SURFACES_SUPPORT
            // prepare device manager
            pAllocator->pDevice = new CD3D9Device();
            sts = pAllocator->pDevice->Init(0, 1, MSDKAdapter::GetNumber(pProcessor->mfxSession));
            MSDK_CHECK_STATUS_SAFE(sts, "pAllocator->pDevice->Init failed", WipeMemoryAllocator(pAllocator));

            mfxHDL hdl = 0;
            sts = pAllocator->pDevice->GetHandle(MFX_HANDLE_D3D9_DEVICE_MANAGER, &hdl);
            MSDK_CHECK_STATUS_SAFE(sts, "pAllocator->pDevice->GetHandle failed", WipeMemoryAllocator(pAllocator));
            sts = pProcessor->mfxSession.SetHandle(MFX_HANDLE_D3D9_DEVICE_MANAGER, hdl);
            MSDK_CHECK_STATUS_SAFE(sts, "pAllocator->pDevice->SetHandle failed", WipeMemoryAllocator(pAllocator));

            // prepare allocator
            D3DAllocatorParams *pd3dAllocParams = new D3DAllocatorParams;

            pd3dAllocParams->pManager = (IDirect3DDeviceManager9*)hdl;
            pAllocator->pAllocatorParams = pd3dAllocParams;
#endif
        }
        else if((pInParams->ImpLib & IMPL_VIA_MASK) == MFX_IMPL_VIA_D3D11)
        {
#if MFX_D3D11_SUPPORT
            pAllocator->pDevice = new CD3D11Device();

            sts = pAllocator->pDevice->Init(0, 1, MSDKAdapter::GetNumber(pProcessor->mfxSession));
            MSDK_CHECK_STATUS_SAFE(sts, "pAllocator->pDevice->Init failed", WipeMemoryAllocator(pAllocator));

            mfxHDL hdl = 0;
            sts = pAllocator->pDevice->GetHandle(MFX_HANDLE_D3D11_DEVICE, &hdl);
            MSDK_CHECK_STATUS_SAFE(sts, "pAllocator->pDevice->GetHandle failed", WipeMemoryAllocator(pAllocator));
            sts = pProcessor->mfxSession.SetHandle(MFX_HANDLE_D3D11_DEVICE, hdl);
            MSDK_CHECK_STATUS_SAFE(sts, "pAllocator->pDevice->SetHandle failed", WipeMemoryAllocator(pAllocator));

            // prepare allocator
            D3D11AllocatorParams *pd3d11AllocParams = new D3D11AllocatorParams;

            pd3d11AllocParams->pDevice = (ID3D11Device*)hdl;
            pAllocator->pAllocatorParams = pd3d11AllocParams;
#endif
        }
        else if ((pInParams->ImpLib & IMPL_VIA_MASK) == MFX_IMPL_VIA_VAAPI)
        {
#ifdef LIBVA_SUPPORT
            pAllocator->pDevice = CreateVAAPIDevice(pInParams->strDevicePath);
            MSDK_CHECK_POINTER(pAllocator->pDevice, MFX_ERR_NULL_PTR);

            sts = pAllocator->pDevice->Init(0, 1, MSDKAdapter::GetNumber(pProcessor->mfxSession));
            MSDK_CHECK_STATUS_SAFE(sts, "pAllocator->pDevice->Init failed", WipeMemoryAllocator(pAllocator));

            mfxHDL hdl = 0;
            sts = pAllocator->pDevice->GetHandle(MFX_HANDLE_VA_DISPLAY, &hdl);
            MSDK_CHECK_STATUS_SAFE(sts, "pAllocator->pDevice->GetHandle failed", WipeMemoryAllocator(pAllocator));
            sts = pProcessor->mfxSession.SetHandle(MFX_HANDLE_VA_DISPLAY, hdl);
            MSDK_CHECK_STATUS_SAFE(sts, "pAllocator->pDevice->SetHandle failed", WipeMemoryAllocator(pAllocator));

            // prepare allocator
            vaapiAllocatorParams *pVaapiAllocParams = new vaapiAllocatorParams;

            pVaapiAllocParams->m_dpy = (VADisplay)hdl;
            pAllocator->pAllocatorParams = pVaapiAllocParams;

#endif
        }
    }
    else
    {
#ifdef LIBVA_SUPPORT
        //in case of system memory allocator we also have to pass MFX_HANDLE_VA_DISPLAY to HW library
        mfxIMPL impl;
        pProcessor->mfxSession.QueryIMPL(&impl);

        if(MFX_IMPL_HARDWARE == MFX_IMPL_BASETYPE(impl))
        {
            pAllocator->pDevice = CreateVAAPIDevice(pInParams->strDevicePath);
            if (!pAllocator->pDevice) sts = MFX_ERR_MEMORY_ALLOC;
            MSDK_CHECK_STATUS_SAFE(sts, "pAllocator->pDevice creation failed", WipeMemoryAllocator(pAllocator));

            mfxHDL hdl = 0;
            sts = pAllocator->pDevice->GetHandle(MFX_HANDLE_VA_DISPLAY, &hdl);
            MSDK_CHECK_STATUS_SAFE(sts, "pAllocator->pDevice->GetHandle failed", WipeMemoryAllocator(pAllocator));

            sts = pProcessor->mfxSession.SetHandle(MFX_HANDLE_VA_DISPLAY, hdl);
            MSDK_CHECK_STATUS_SAFE(sts, "pAllocator->pDevice->SetHandle failed", WipeMemoryAllocator(pAllocator));
        }
#endif

    }
    /* This sample uses external memory allocator model for both system and HW memory */
    sts = pProcessor->mfxSession.SetFrameAllocator(pAllocator->pMfxAllocator);
    MSDK_CHECK_STATUS_SAFE(sts, "pProcessor->mfxSession.SetFrameAllocator failed", WipeMemoryAllocator(pAllocator));
    pAllocator->bUsedAsExternalAllocator = true;

    sts = pAllocator->pMfxAllocator->Init(pAllocator->pAllocatorParams);
    MSDK_CHECK_STATUS_SAFE(sts, "pAllocator->pMfxAllocator->Init failed", WipeMemoryAllocator(pAllocator));

    mfxVideoParam tmpParam={0};
    tmpParam.ExtParam = pParams->ExtParam;
    tmpParam.NumExtParam = pParams->NumExtParam;
    sts = pProcessor->pmfxVPP->Query(pParams, &tmpParam);
    *pParams=tmpParam;
    MSDK_CHECK_STATUS_SAFE(sts, "pProcessor->pmfxVPP->Query failed", WipeMemoryAllocator(pAllocator));

    sts = pProcessor->pmfxVPP->QueryIOSurf(pParams, request);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    MSDK_CHECK_STATUS_SAFE(sts, "pProcessor->pmfxVPP->QueryIOSurf failed", WipeMemoryAllocator(pAllocator));

    // alloc frames for vpp
    // [IN]
    // If we have only one input stream - allocate as many surfaces as were requested. Otherwise (in case of composition) - allocate 1 surface per input
    // Modify frame info as well
    if(pInParams->compositionParam.mode != VPP_FILTER_ENABLED_CONFIGURED)
    {
        sts = InitSurfaces(pAllocator, &(request[VPP_IN]),true,0);
       MSDK_CHECK_STATUS_SAFE(sts, "InitSurfaces failed", WipeMemoryAllocator(pAllocator));
    }
    else
    {
        for(int i=0;i<pInParams->numStreams;i++)
        {
            ownToMfxFrameInfo(&pInParams->inFrameInfo[i],&request[VPP_IN].Info,true);
            request[VPP_IN].NumFrameSuggested = 1;
            request[VPP_IN].NumFrameMin = request[VPP_IN].NumFrameSuggested;
            sts = InitSurfaces(pAllocator, &(request[VPP_IN]),true,i);
            MSDK_CHECK_STATUS_SAFE(sts, "InitSurfaces failed", WipeMemoryAllocator(pAllocator));
        }
    }

    // [OUT]
    sts = InitSurfaces(pAllocator, &(request[VPP_OUT]), false,0);
    MSDK_CHECK_STATUS_SAFE(sts, "InitSurfaces failed", WipeMemoryAllocator(pAllocator));

    return MFX_ERR_NONE;

} // mfxStatus InitMemoryAllocator(...)}

/* ******************************************************************* */

mfxStatus InitResources(sAppResources* pResources, mfxVideoParam* pParams, sInputParams* pInParams)
{
    mfxStatus sts = MFX_ERR_NONE;

    MSDK_CHECK_POINTER(pResources, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(pParams,    MFX_ERR_NULL_PTR);
    sts = CreateFrameProcessor(pResources->pProcessor, pParams, pInParams);
    MSDK_CHECK_STATUS_SAFE(sts, "CreateFrameProcessor failed", { WipeResources(pResources); WipeParams(pInParams);});

    sts = InitMemoryAllocator(pResources->pProcessor, pResources->pAllocator, pParams, pInParams);
    MSDK_CHECK_STATUS_SAFE(sts, "InitMemoryAllocator failed", { WipeResources(pResources); WipeParams(pInParams);});

    sts = InitFrameProcessor(pResources->pProcessor, pParams);

    if (MFX_WRN_PARTIAL_ACCELERATION == sts || MFX_WRN_FILTER_SKIPPED == sts)
        return sts;
    else
    {
        MSDK_CHECK_STATUS_SAFE(sts, "InitFrameProcessor failed", { WipeResources(pResources); WipeParams(pInParams);});
    }

    return sts;
}

/* ******************************************************************* */

void WipeFrameProcessor(sFrameProcessor* pProcessor)
{
    MSDK_CHECK_POINTER_NO_RET(pProcessor);

    MSDK_SAFE_DELETE(pProcessor->pmfxVPP);

    if ( pProcessor->plugin )
    {
        MFXVideoUSER_UnLoad(pProcessor->mfxSession, &(pProcessor->mfxGuid));
    }

    pProcessor->mfxSession.Close();
}

void WipeMemoryAllocator(sMemoryAllocator* pAllocator)
{
    MSDK_CHECK_POINTER_NO_RET(pAllocator);

    for(int i=0;i<MAX_INPUT_STREAMS;i++)
    {
        MSDK_SAFE_DELETE_ARRAY(pAllocator->pSurfacesIn[i]);
    }
    //    MSDK_SAFE_DELETE_ARRAY(pAllocator->pSurfaces[VPP_IN_RGB]);
    MSDK_SAFE_DELETE_ARRAY(pAllocator->pSurfacesOut);

    mfxU32 did;
    for(did = 0; did < 8; did++)
    {
        MSDK_SAFE_DELETE_ARRAY(pAllocator->pSvcSurfaces[did]);
    }

    // delete frames
    if (pAllocator->pMfxAllocator)
    {
        for(int i=0;i<MAX_INPUT_STREAMS;i++)
        {
            if(pAllocator->responseIn[i].NumFrameActual)
            {
                pAllocator->pMfxAllocator->Free(pAllocator->pMfxAllocator->pthis, &pAllocator->responseIn[i]);
            }
        }
        pAllocator->pMfxAllocator->Free(pAllocator->pMfxAllocator->pthis, &pAllocator->responseOut);

        for(did = 0; did < 8; did++)
        {
            pAllocator->pMfxAllocator->Free(pAllocator->pMfxAllocator->pthis, &pAllocator->svcResponse[did]);
        }
    }

    // delete allocator
    MSDK_SAFE_DELETE(pAllocator->pMfxAllocator);
    MSDK_SAFE_DELETE(pAllocator->pDevice);

    // delete allocator parameters
    MSDK_SAFE_DELETE(pAllocator->pAllocatorParams);

} // void WipeMemoryAllocator(sMemoryAllocator* pAllocator)


void WipeConfigParam( sAppResources* pResources )
{
    auto multiViewConfig = pResources->pVppParams->GetExtBuffer<mfxExtMVCSeqDesc>();
    if (multiViewConfig)
    {
        delete [] multiViewConfig->View;
    }
} // void WipeConfigParam( sAppResources* pResources )


void WipeResources(sAppResources* pResources)
{
    MSDK_CHECK_POINTER_NO_RET(pResources);

    WipeFrameProcessor(pResources->pProcessor);

    WipeMemoryAllocator(pResources->pAllocator);

    for (int i = 0; i < pResources->numSrcFiles; i++)
    {
        if (pResources->pSrcFileReaders[i])
        {
            pResources->pSrcFileReaders[i]->Close();
        }
    }
    pResources->numSrcFiles=0;

    if (pResources->pDstFileWriters)
    {
        for (mfxU32 i = 0; i < pResources->dstFileWritersN; i++)
        {
            pResources->pDstFileWriters[i].Close();
        }
        delete[] pResources->pDstFileWriters;
        pResources->dstFileWritersN = 0;
        pResources->pDstFileWriters = NULL;
    }

    auto compositeConfig = pResources->pVppParams->GetExtBuffer<mfxExtVPPComposite>();
    if(compositeConfig)
    {
        delete[] compositeConfig->InputStream;
        compositeConfig->InputStream=nullptr;
    }

    WipeConfigParam( pResources );

} // void WipeResources(sAppResources* pResources)

/* ******************************************************************* */

void WipeParams(sInputParams* pParams)
{
    pParams->strDstFiles.clear();

} // void WipeParams(sInputParams* pParams)

/* ******************************************************************* */

CRawVideoReader::CRawVideoReader()
{
    m_fSrc = 0;
    m_isPerfMode = false;
    m_Repeat = 0;
    m_pPTSMaker = 0;
    m_initFcc = 0;
}

mfxStatus CRawVideoReader::Init(const msdk_char *strFileName, PTSMaker *pPTSMaker, mfxU32 fcc)
{
    Close();

    MSDK_CHECK_POINTER(strFileName, MFX_ERR_NULL_PTR);

    MSDK_FOPEN(m_fSrc,strFileName, MSDK_STRING("rb"));
    MSDK_CHECK_POINTER(m_fSrc, MFX_ERR_ABORTED);

    m_pPTSMaker = pPTSMaker;
    m_initFcc = fcc;
    return MFX_ERR_NONE;
}

CRawVideoReader::~CRawVideoReader()
{
    Close();
}

void CRawVideoReader::Close()
{
    if (m_fSrc != 0)
    {
        fclose(m_fSrc);
        m_fSrc = 0;
    }
    m_SurfacesList.clear();

}

mfxStatus CRawVideoReader::LoadNextFrame(mfxFrameData* pData, mfxFrameInfo* pInfo)
{
    MSDK_CHECK_POINTER(pData, MFX_ERR_NOT_INITIALIZED);
    MSDK_CHECK_POINTER(pInfo, MFX_ERR_NOT_INITIALIZED);

    // Only (I420|YV12) -> NV12 in-place conversion supported
    if (pInfo->FourCC != m_initFcc &&
        (pInfo->FourCC != MFX_FOURCC_NV12 ||
        (m_initFcc != MFX_FOURCC_I420 && m_initFcc != MFX_FOURCC_YV12) ) )
    {
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    mfxU32 w, h, i, pitch;
    mfxU32 nBytesRead;
    mfxU8 *ptr;

    if (pInfo->CropH > 0 && pInfo->CropW > 0)
    {
        w = pInfo->CropW;
        h = pInfo->CropH;
    }
    else
    {
        w = pInfo->Width;
        h = pInfo->Height;
    }

    pitch = ((mfxU32)pData->PitchHigh << 16) + pData->PitchLow;

    if(pInfo->FourCC == MFX_FOURCC_YV12 || pInfo->FourCC == MFX_FOURCC_I420)
    {
        ptr = pData->Y + pInfo->CropX + pInfo->CropY * pitch;

        // read luminance plane
        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, w, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
        }

        w     >>= 1;
        h     >>= 1;
        pitch >>= 1;
        // load U/V
        ptr = (pInfo->FourCC == MFX_FOURCC_I420 ? pData->U : pData->V) + (pInfo->CropX >> 1) + (pInfo->CropY >> 1) * pitch;
        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, w, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
        }
        // load V/U
        ptr  = (pInfo->FourCC == MFX_FOURCC_I420 ? pData->V : pData->U) + (pInfo->CropX >> 1) + (pInfo->CropY >> 1) * pitch;
        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, w, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
        }
    }
    else   if(pInfo->FourCC == MFX_FOURCC_YUV400)
    {
        ptr = pData->Y + pInfo->CropX + pInfo->CropY * pitch;

        // read luminance plane
        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, w, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
        }
    }
    else if(pInfo->FourCC == MFX_FOURCC_YUV411)
    {
        ptr = pData->Y + pInfo->CropX + pInfo->CropY * pitch;

        // read luminance plane
        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, w, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
        }

        w /= 4;

        // load U
        ptr  = pData->U + (pInfo->CropX >> 1) + (pInfo->CropY >> 1) * pitch;
        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, w, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
        }
        // load V
        ptr  = pData->V + (pInfo->CropX >> 1) + (pInfo->CropY >> 1) * pitch;
        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, w, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
        }
    }
    else if(pInfo->FourCC == MFX_FOURCC_YUV422H)
    {
        ptr = pData->Y + pInfo->CropX + pInfo->CropY * pitch;

        // read luminance plane
        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, w, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
        }

        w     >>= 1;

        // load U
        ptr  = pData->U + (pInfo->CropX >> 1) + (pInfo->CropY >> 1) * pitch;
        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, w, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
        }
        // load V
        ptr  = pData->V + (pInfo->CropX >> 1) + (pInfo->CropY >> 1) * pitch;
        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, w, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
        }
    }
    else if(pInfo->FourCC == MFX_FOURCC_YUV422V)
    {
        ptr = pData->Y + pInfo->CropX + pInfo->CropY * pitch;

        // read luminance plane
        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, w, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
        }

        h     >>= 1;

        // load U
        ptr  = pData->U + (pInfo->CropX >> 1) + (pInfo->CropY >> 1) * pitch;
        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, w, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
        }
        // load V
        ptr  = pData->V + (pInfo->CropX >> 1) + (pInfo->CropY >> 1) * pitch;
        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, w, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
        }
    }
    else if(pInfo->FourCC == MFX_FOURCC_YUV444)
    {
        ptr = pData->Y + pInfo->CropX + pInfo->CropY * pitch;

        // read luminance plane
        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, w, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
        }

        // load U
        ptr  = pData->U + (pInfo->CropX >> 1) + (pInfo->CropY >> 1) * pitch;
        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, w, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
        }
        // load V
        ptr  = pData->V + (pInfo->CropX >> 1) + (pInfo->CropY >> 1) * pitch;
        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, w, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
        }
    }
    else if (pInfo->FourCC == MFX_FOURCC_NV12)
    {
        ptr = pData->Y + pInfo->CropX + pInfo->CropY * pitch;

        // read luminance plane
        for (i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, w, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
        }

        switch (m_initFcc)
        {
            case MFX_FOURCC_NV12:
            {
                // load UV
                h >>= 1;
                ptr = pData->UV + pInfo->CropX + (pInfo->CropY >> 1) * pitch;
                for (i = 0; i < h; i++)
                {
                    nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, w, m_fSrc);
                    IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
                }
                break;
            }
            case MFX_FOURCC_I420:
            case MFX_FOURCC_YV12:
            {
                mfxU8 buf[2048]; // maximum supported chroma width for nv12
                mfxU32 j, dstOffset[2];
                w /= 2;
                h /= 2;
                ptr = pData->UV + pInfo->CropX + (pInfo->CropY / 2) * pitch;
                if (w > 2048)
                {
                    return MFX_ERR_UNSUPPORTED;
                }

                if (m_initFcc == MFX_FOURCC_I420) {
                    dstOffset[0] = 0;
                    dstOffset[1] = 1;
                }
                else {
                    dstOffset[0] = 1;
                    dstOffset[1] = 0;
                }

                // load first chroma plane: U (input == I420) or V (input == YV12)
                for (i = 0; i < h; i++)
                {
                    nBytesRead = (mfxU32)fread(buf, 1, w, m_fSrc);
                    if (w != nBytesRead)
                    {
                        return MFX_ERR_MORE_DATA;
                    }
                    for (j = 0; j < w; j++)
                    {
                        ptr[i * pitch + j * 2 + dstOffset[0]] = buf[j];
                    }
                }

                // load second chroma plane: V (input == I420) or U (input == YV12)
                for (i = 0; i < h; i++)
                {

                    nBytesRead = (mfxU32)fread(buf, 1, w, m_fSrc);

                    if (w != nBytesRead)
                    {
                        return MFX_ERR_MORE_DATA;
                    }
                    for (j = 0; j < w; j++)
                    {
                        ptr[i * pitch + j * 2 + dstOffset[1]] = buf[j];
                    }
                }
                break;
            }
        }
    }
    else if( pInfo->FourCC == MFX_FOURCC_NV16 )
    {
        ptr = pData->Y + pInfo->CropX + pInfo->CropY * pitch;

        // read luminance plane
        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, w, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
        }

        // load UV
        ptr = pData->UV + pInfo->CropX + (pInfo->CropY >> 1) * pitch;
        for (i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, w, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
        }
    }
    else if( pInfo->FourCC == MFX_FOURCC_P010
#if (MFX_VERSION >= 1031)
          || pInfo->FourCC == MFX_FOURCC_P016
#endif
            )
    {
        ptr = pData->Y + pInfo->CropX * 2 + pInfo->CropY * pitch;

        // read luminance plane
        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, w * 2, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, w*2, MFX_ERR_MORE_DATA);
        }

        // load UV
        h     >>= 1;
        ptr = pData->UV + pInfo->CropX + (pInfo->CropY >> 1) * pitch;
        for (i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, w*2, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, w*2, MFX_ERR_MORE_DATA);
        }
    }
    else if( pInfo->FourCC == MFX_FOURCC_P210 )
    {
        ptr = pData->Y + pInfo->CropX * 2 + pInfo->CropY * pitch;

        // read luminance plane
        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, w * 2, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, w*2, MFX_ERR_MORE_DATA);
        }

        // load UV
        ptr = pData->UV + pInfo->CropX + (pInfo->CropY >> 1) * pitch;
        for (i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, w*2, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, w*2, MFX_ERR_MORE_DATA);
        }
    }
#if (MFX_VERSION >= 1028)
    else if (pInfo->FourCC == MFX_FOURCC_RGB565)
    {
        MSDK_CHECK_POINTER(pData->R, MFX_ERR_NOT_INITIALIZED);
        MSDK_CHECK_POINTER(pData->G, MFX_ERR_NOT_INITIALIZED);
        MSDK_CHECK_POINTER(pData->B, MFX_ERR_NOT_INITIALIZED);

        ptr = pData->B;
        ptr = ptr + pInfo->CropX + pInfo->CropY * pitch;

        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, 2*w, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, 2*w, MFX_ERR_MORE_DATA);
        }
    }
#endif
    else if (pInfo->FourCC == MFX_FOURCC_RGB3)
    {
        MSDK_CHECK_POINTER(pData->R, MFX_ERR_NOT_INITIALIZED);
        MSDK_CHECK_POINTER(pData->G, MFX_ERR_NOT_INITIALIZED);
        MSDK_CHECK_POINTER(pData->B, MFX_ERR_NOT_INITIALIZED);

        ptr = std::min(std::min(pData->R, pData->G), pData->B );
        ptr = ptr + pInfo->CropX + pInfo->CropY * pitch;

        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, 3*w, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, 3*w, MFX_ERR_MORE_DATA);
        }
    }
    else if (pInfo->FourCC == MFX_FOURCC_RGB4 || pInfo->FourCC == MFX_FOURCC_A2RGB10)
    {
        MSDK_CHECK_POINTER(pData->R, MFX_ERR_NOT_INITIALIZED);
        MSDK_CHECK_POINTER(pData->G, MFX_ERR_NOT_INITIALIZED);
        MSDK_CHECK_POINTER(pData->B, MFX_ERR_NOT_INITIALIZED);
        // there is issue with A channel in case of d3d, so A-ch is ignored
        //MSDK_CHECK_POINTER(pData->A, MFX_ERR_NOT_INITIALIZED);

        ptr = std::min(std::min(pData->R, pData->G), pData->B );
        ptr = ptr + pInfo->CropX + pInfo->CropY * pitch;

        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, 4*w, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, 4*w, MFX_ERR_MORE_DATA);
        }
    }
    else if (pInfo->FourCC == MFX_FOURCC_YUY2)
    {
        ptr = pData->Y + pInfo->CropX + pInfo->CropY * pitch;

        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, 2*w, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, 2*w, MFX_ERR_MORE_DATA);
        }
    }
    else if (pInfo->FourCC == MFX_FOURCC_UYVY)
    {
        ptr = pData->U + pInfo->CropX + pInfo->CropY * pitch;

        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, 2*w, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, 2*w, MFX_ERR_MORE_DATA);
        }
    }
    else if (pInfo->FourCC == MFX_FOURCC_IMC3)
    {
        ptr = pData->Y + pInfo->CropX + pInfo->CropY * pitch;

        // read luminance plane
        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, w, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
        }

        h     >>= 1;

        // load U
        ptr  = pData->V + (pInfo->CropX >> 1) + (pInfo->CropY >> 1) * pitch;
        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, w, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
        }
        // load V
        ptr  = pData->U + (pInfo->CropX >> 1) + (pInfo->CropY >> 1) * pitch;
        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, w, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
        }
    }
    else if (pInfo->FourCC == MFX_FOURCC_AYUV)
    {
        ptr = std::min( std::min(pData->Y, pData->U), std::min(pData->V, pData->A) );
        ptr = ptr + pInfo->CropX + pInfo->CropY * pitch;

        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, 4*w, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, 4*w, MFX_ERR_MORE_DATA);
        }
    }
#if (MFX_VERSION >= 1027)
    else if (pInfo->FourCC == MFX_FOURCC_Y210
#if (MFX_VERSION >= 1031)
    || pInfo->FourCC == MFX_FOURCC_Y216
#endif
)
    {
        ptr = (mfxU8*)(pData->Y16 + pInfo->CropX * 2) + pInfo->CropY * pitch;

        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, 4*w, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, 4*w, MFX_ERR_MORE_DATA);
        }
    }
    else if (pInfo->FourCC == MFX_FOURCC_Y410)
    {
        ptr = (mfxU8*)(pData->Y410 + pInfo->CropX) + pInfo->CropY * pitch;

        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, 4*w, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, 4*w, MFX_ERR_MORE_DATA);
        }
    }
#endif
#if (MFX_VERSION >= 1031)
    else if (pInfo->FourCC == MFX_FOURCC_Y416)
    {
        ptr = (mfxU8*)(pData->U16 + pInfo->CropX * 4) + pInfo->CropY * pitch;

        for (i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, 8 * w, m_fSrc);
            IOSTREAM_MSDK_CHECK_NOT_EQUAL(nBytesRead, 8 * w, MFX_ERR_MORE_DATA);
        }
    }
#endif
    else
    {
        return MFX_ERR_UNSUPPORTED;
    }

    return MFX_ERR_NONE;
}


mfxStatus CRawVideoReader::GetNextInputFrame(sMemoryAllocator* pAllocator, mfxFrameInfo* pInfo, mfxFrameSurfaceWrap** pSurface, mfxU16 streamIndex)
{
    mfxStatus sts;
    if (!m_isPerfMode)
    {
        sts = GetFreeSurface(pAllocator->pSurfacesIn[streamIndex], pAllocator->responseIn[streamIndex].NumFrameActual, pSurface);
        MSDK_CHECK_STATUS(sts,"GetFreeSurface failed");

        mfxFrameSurfaceWrap* pCurSurf = *pSurface;
        if (pCurSurf->Data.MemId || pAllocator->bUsedAsExternalAllocator)
        {
            // get YUV pointers
            sts = pAllocator->pMfxAllocator->Lock(pAllocator->pMfxAllocator->pthis, pCurSurf->Data.MemId, &pCurSurf->Data);
            MFX_CHECK_STS(sts);
            sts = LoadNextFrame(&pCurSurf->Data, pInfo);
            MFX_CHECK_STS(sts);
            sts = pAllocator->pMfxAllocator->Unlock(pAllocator->pMfxAllocator->pthis, pCurSurf->Data.MemId, &pCurSurf->Data);
            MFX_CHECK_STS(sts);
        }
        else
        {
            sts = LoadNextFrame( &pCurSurf->Data, pInfo);
            MFX_CHECK_STS(sts);
        }
    }
    else
    {
        sts = GetPreAllocFrame(pSurface);
        MFX_CHECK_STS(sts);
    }

    if (m_pPTSMaker)
    {
        if (!m_pPTSMaker->SetPTS(*pSurface))
            return MFX_ERR_UNKNOWN;
    }

    return MFX_ERR_NONE;
 }


mfxStatus  CRawVideoReader::GetPreAllocFrame(mfxFrameSurfaceWrap **pSurface)
{
    if (m_it == m_SurfacesList.end())
    {
        m_Repeat--;
        m_it = m_SurfacesList.begin();
    }

    if (m_it->Data.Locked)
        return MFX_ERR_ABORTED;

    *pSurface = &(*m_it);
    m_it++;
    if (0 == m_Repeat)
        return MFX_ERR_MORE_DATA;

    return MFX_ERR_NONE;

}


mfxStatus  CRawVideoReader::PreAllocateFrameChunk(mfxVideoParam* pVideoParam,
    sInputParams* pParams,
    MFXFrameAllocator* pAllocator)
{
    mfxStatus sts;
    mfxFrameAllocRequest  request;
    mfxFrameAllocResponse response;
    mfxFrameSurfaceWrap      surface;
    m_isPerfMode = true;
    m_Repeat = pParams->numRepeat;
    request.Info = pVideoParam->vpp.In;
    request.Type = (pParams->IOPattern & MFX_IOPATTERN_IN_VIDEO_MEMORY)?(MFX_MEMTYPE_FROM_VPPIN|MFX_MEMTYPE_INTERNAL_FRAME|MFX_MEMTYPE_DXVA2_PROCESSOR_TARGET):
        (MFX_MEMTYPE_FROM_VPPIN|MFX_MEMTYPE_INTERNAL_FRAME|MFX_MEMTYPE_SYSTEM_MEMORY);
    request.NumFrameSuggested = request.NumFrameMin = (mfxU16)pParams->numFrames;
    sts = pAllocator->Alloc(pAllocator, &request, &response);
    MFX_CHECK_STS(sts);
    for(;m_SurfacesList.size() < pParams->numFrames;)
    {
        surface.Data.Locked = 0;
        surface.Data.MemId = response.mids[m_SurfacesList.size()];
        surface.Info = pVideoParam->vpp.In;
        memset(surface.reserved, 0, sizeof(surface.reserved));
        sts = pAllocator->Lock(pAllocator->pthis, surface.Data.MemId, &surface.Data);
        MFX_CHECK_STS(sts);
        sts = LoadNextFrame(&surface.Data, &pVideoParam->vpp.In);
        MFX_CHECK_STS(sts);
        sts = pAllocator->Unlock(pAllocator->pthis, surface.Data.MemId, &surface.Data);
        MFX_CHECK_STS(sts);
        m_SurfacesList.push_back(surface);
    }
    m_it = m_SurfacesList.begin();
    return MFX_ERR_NONE;
}
/* ******************************************************************* */

CRawVideoWriter::CRawVideoWriter()
{
    m_fDst = 0;
    m_pPTSMaker = 0;
    m_forcedOutputFourcc = 0;
    return;
}

mfxStatus CRawVideoWriter::Init(const msdk_char *strFileName, PTSMaker *pPTSMaker, mfxU32 forcedOutputFourcc)
{
    Close();

    m_pPTSMaker = pPTSMaker;
    // no need to generate output
    if (0 == strFileName)
        return MFX_ERR_NONE;

    //CHECK_POINTER(strFileName, MFX_ERR_NULL_PTR);

    MSDK_FOPEN(m_fDst,strFileName, MSDK_STRING("wb"));
    MSDK_CHECK_POINTER(m_fDst, MFX_ERR_ABORTED);
    m_forcedOutputFourcc = forcedOutputFourcc;

    return MFX_ERR_NONE;
}

CRawVideoWriter::~CRawVideoWriter()
{
    Close();

    return;
}

void CRawVideoWriter::Close()
{
    if (m_fDst != 0){

        fclose(m_fDst);
        m_fDst = 0;
    }

    return;
}

mfxStatus CRawVideoWriter::PutNextFrame(
    sMemoryAllocator* pAllocator,
    mfxFrameInfo* pInfo,
    mfxFrameSurfaceWrap* pSurface)
{
    mfxStatus sts;
    if (m_fDst)
    {
        if (pSurface->Data.MemId)
        {
            // get YUV pointers
            sts = pAllocator->pMfxAllocator->Lock(pAllocator->pMfxAllocator->pthis, pSurface->Data.MemId, &(pSurface->Data));
            MSDK_CHECK_NOT_EQUAL(sts, MFX_ERR_NONE, MFX_ERR_ABORTED);

            sts = WriteFrame( &(pSurface->Data), pInfo);
            MSDK_CHECK_NOT_EQUAL(sts, MFX_ERR_NONE, MFX_ERR_ABORTED);

            sts = pAllocator->pMfxAllocator->Unlock(pAllocator->pMfxAllocator->pthis, pSurface->Data.MemId, &(pSurface->Data));
            MSDK_CHECK_NOT_EQUAL(sts, MFX_ERR_NONE, MFX_ERR_ABORTED);
        }
        else
        {
            sts = WriteFrame( &(pSurface->Data), pInfo);
            MSDK_CHECK_NOT_EQUAL(sts, MFX_ERR_NONE, MFX_ERR_ABORTED);
        }
    }
    else // performance mode
    {
        if (pSurface->Data.MemId)
        {
            sts = pAllocator->pMfxAllocator->Lock(pAllocator->pMfxAllocator->pthis, pSurface->Data.MemId, &(pSurface->Data));
            MSDK_CHECK_NOT_EQUAL(sts, MFX_ERR_NONE, MFX_ERR_ABORTED);
            sts = pAllocator->pMfxAllocator->Unlock(pAllocator->pMfxAllocator->pthis, pSurface->Data.MemId, &(pSurface->Data));
            MSDK_CHECK_NOT_EQUAL(sts, MFX_ERR_NONE, MFX_ERR_ABORTED);
        }
    }
    if (m_pPTSMaker)
        return m_pPTSMaker->CheckPTS(pSurface)?MFX_ERR_NONE:MFX_ERR_ABORTED;

    return MFX_ERR_NONE;
}
mfxStatus CRawVideoWriter::WriteFrame(
    mfxFrameData* pData,
    mfxFrameInfo* pInfo)
{
    mfxI32 nBytesRead   = 0;

    mfxI32 i, pitch;
    mfxU16 h, w;
    mfxU8* ptr;

    MSDK_CHECK_POINTER(pData, MFX_ERR_NOT_INITIALIZED);
    MSDK_CHECK_POINTER(pInfo, MFX_ERR_NOT_INITIALIZED);
    //-------------------------------------------------------
    mfxFrameData outData = *pData;

    if (pInfo->CropH > 0 && pInfo->CropW > 0)
    {
        w = pInfo->CropW;
        h = pInfo->CropH;
    }
    else
    {
        w = pInfo->Width;
        h = pInfo->Height;
    }

    pitch = outData.Pitch;

    if(pInfo->FourCC == MFX_FOURCC_YV12 || pInfo->FourCC == MFX_FOURCC_I420)
    {

        ptr   = outData.Y + (pInfo->CropX ) + (pInfo->CropY ) * pitch;

        for (i = 0; i < h; i++)
        {
            MSDK_CHECK_NOT_EQUAL(fwrite(ptr+ i * pitch, 1, w, m_fDst), w, MFX_ERR_UNDEFINED_BEHAVIOR);
        }

        w     >>= 1;
        h     >>= 1;
        pitch >>= 1;

        ptr  = (pInfo->FourCC == MFX_FOURCC_I420 ? outData.U : outData.V) + (pInfo->CropX >> 1) + (pInfo->CropY >> 1) * pitch;
        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fwrite(ptr + i * pitch, 1, w, m_fDst);
            MSDK_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
        }

        ptr  = (pInfo->FourCC == MFX_FOURCC_I420 ? outData.V : outData.U) + (pInfo->CropX >> 1) + (pInfo->CropY >> 1) * pitch;
        for(i = 0; i < h; i++)
        {
            MSDK_CHECK_NOT_EQUAL( fwrite(ptr+ i * pitch, 1, w, m_fDst), w, MFX_ERR_UNDEFINED_BEHAVIOR);
        }
    }
    else if(pInfo->FourCC == MFX_FOURCC_YUV400)
    {
        ptr   = pData->Y + (pInfo->CropX ) + (pInfo->CropY ) * pitch;

        for (i = 0; i < h; i++)
        {
            MSDK_CHECK_NOT_EQUAL( fwrite(ptr+ i * pitch, 1, w, m_fDst), w, MFX_ERR_UNDEFINED_BEHAVIOR);
        }

        w     >>= 1;
        h     >>= 1;
        pitch >>= 1;

        ptr  = pData->U + (pInfo->CropX >> 1) + (pInfo->CropY >> 1) * pitch;
        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fwrite(ptr + i * pitch, 1, w, m_fDst);
            MSDK_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
        }

        ptr  = pData->V + (pInfo->CropX >> 1) + (pInfo->CropY >> 1) * pitch;
        for(i = 0; i < h; i++)
        {
            MSDK_CHECK_NOT_EQUAL( fwrite(ptr+ i * pitch, 1, w, m_fDst), w, MFX_ERR_UNDEFINED_BEHAVIOR);
        }
    }
    else if(pInfo->FourCC == MFX_FOURCC_YUV411)
    {
        ptr   = pData->Y + (pInfo->CropX ) + (pInfo->CropY ) * pitch;

        for (i = 0; i < h; i++)
        {
            MSDK_CHECK_NOT_EQUAL( fwrite(ptr+ i * pitch, 1, w, m_fDst), w, MFX_ERR_UNDEFINED_BEHAVIOR);
        }

        w     /= 4;
        //pitch /= 4;

        ptr  = pData->U + (pInfo->CropX >> 1) + (pInfo->CropY >> 1) * pitch;
        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fwrite(ptr + i * pitch, 1, w, m_fDst);
            MSDK_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
        }

        ptr  = pData->V + (pInfo->CropX >> 1) + (pInfo->CropY >> 1) * pitch;
        for(i = 0; i < h; i++)
        {
            MSDK_CHECK_NOT_EQUAL( fwrite(ptr+ i * pitch, 1, w, m_fDst), w, MFX_ERR_UNDEFINED_BEHAVIOR);
        }
    }
    else if(pInfo->FourCC == MFX_FOURCC_YUV422H)
    {
        ptr   = pData->Y + (pInfo->CropX ) + (pInfo->CropY ) * pitch;

        for (i = 0; i < h; i++)
        {
            MSDK_CHECK_NOT_EQUAL( fwrite(ptr+ i * pitch, 1, w, m_fDst), w, MFX_ERR_UNDEFINED_BEHAVIOR);
        }

        w     >>= 1;
        //pitch >>= 1;

        ptr  = pData->U + (pInfo->CropX >> 1) + (pInfo->CropY >> 1) * pitch;
        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fwrite(ptr + i * pitch, 1, w, m_fDst);
            MSDK_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
        }

        ptr  = pData->V + (pInfo->CropX >> 1) + (pInfo->CropY >> 1) * pitch;
        for(i = 0; i < h; i++)
        {
            MSDK_CHECK_NOT_EQUAL( fwrite(ptr+ i * pitch, 1, w, m_fDst), w, MFX_ERR_UNDEFINED_BEHAVIOR);
        }
    }
    else if(pInfo->FourCC == MFX_FOURCC_YUV422V)
    {
        ptr   = pData->Y + (pInfo->CropX ) + (pInfo->CropY ) * pitch;

        for (i = 0; i < h; i++)
        {
            MSDK_CHECK_NOT_EQUAL( fwrite(ptr+ i * pitch, 1, w, m_fDst), w, MFX_ERR_UNDEFINED_BEHAVIOR);
        }

        h     >>= 1;

        ptr  = pData->U + (pInfo->CropX >> 1) + (pInfo->CropY >> 1) * pitch;
        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fwrite(ptr + i * pitch, 1, w, m_fDst);
            MSDK_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
        }

        ptr  = pData->V + (pInfo->CropX >> 1) + (pInfo->CropY >> 1) * pitch;
        for(i = 0; i < h; i++)
        {
            MSDK_CHECK_NOT_EQUAL( fwrite(ptr+ i * pitch, 1, w, m_fDst), w, MFX_ERR_UNDEFINED_BEHAVIOR);
        }
    }
    else if(pInfo->FourCC == MFX_FOURCC_YUV444)
    {
        ptr   = pData->Y + (pInfo->CropX ) + (pInfo->CropY ) * pitch;

        for (i = 0; i < h; i++)
        {
            MSDK_CHECK_NOT_EQUAL( fwrite(ptr+ i * pitch, 1, w, m_fDst), w, MFX_ERR_UNDEFINED_BEHAVIOR);
        }

        ptr  = pData->U + (pInfo->CropX >> 1) + (pInfo->CropY >> 1) * pitch;
        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fwrite(ptr + i * pitch, 1, w, m_fDst);
            MSDK_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
        }

        ptr  = pData->V + (pInfo->CropX >> 1) + (pInfo->CropY >> 1) * pitch;
        for(i = 0; i < h; i++)
        {
            MSDK_CHECK_NOT_EQUAL( fwrite(ptr+ i * pitch, 1, w, m_fDst), w, MFX_ERR_UNDEFINED_BEHAVIOR);
        }
    }
    else if (pInfo->FourCC == MFX_FOURCC_NV12)
    {
        ptr = pData->Y + (pInfo->CropX) + (pInfo->CropY) * pitch;

        for (i = 0; i < h; i++)
        {
            MSDK_CHECK_NOT_EQUAL(fwrite(ptr + i * pitch, 1, w, m_fDst), w, MFX_ERR_UNDEFINED_BEHAVIOR);
        }

        switch (m_forcedOutputFourcc)
        {
        case MFX_FOURCC_I420:
        {
            int j = 0;

            // write U plane first, then V plane
            h >>= 1;
            w >>= 1;
            ptr = pData->UV + (pInfo->CropX) + (pInfo->CropY >> 1) * pitch;

            for (i = 0; i < h; i++)
            {
                for (j = 0; j < w; j++)
                {
                    fputc(ptr[i*pitch + j * 2], m_fDst);
                }
            }
            for (i = 0; i < h; i++)
            {
                for (j = 0; j < w; j++)
                {
                    fputc(ptr[i*pitch + j * 2 + 1], m_fDst);
                }
            }
        }
        break;

        case MFX_FOURCC_YV12:
        {
            int j = 0;

            // write V plane first, then U plane
            h >>= 1;
            w >>= 1;
            ptr = pData->UV + (pInfo->CropX) + (pInfo->CropY >> 1) * pitch;

            for (i = 0; i < h; i++)
            {
                for (j = 0; j < w; j++)
                {
                    fputc(ptr[i*pitch + j * 2 + 1], m_fDst);
                }
            }
            for (i = 0; i < h; i++)
            {
                for (j = 0; j < w; j++)
                {
                    fputc(ptr[i*pitch + j * 2], m_fDst);
                }
            }
        }
        break;

        default:
        {
            // write UV data
            h >>= 1;
            ptr = pData->UV + (pInfo->CropX) + (pInfo->CropY >> 1) * pitch;

            for (i = 0; i < h; i++)
            {
                MSDK_CHECK_NOT_EQUAL(fwrite(ptr + i * pitch, 1, w, m_fDst), w, MFX_ERR_UNDEFINED_BEHAVIOR);
            }
        }
        break;
        }
    }
    else if( pInfo->FourCC == MFX_FOURCC_NV16 )
    {
        ptr   = pData->Y + (pInfo->CropX ) + (pInfo->CropY ) * pitch;

        for (i = 0; i < h; i++)
        {
            MSDK_CHECK_NOT_EQUAL( fwrite(ptr+ i * pitch, 1, w, m_fDst), w, MFX_ERR_UNDEFINED_BEHAVIOR);
        }

        // write UV data
        ptr  = pData->UV + (pInfo->CropX ) + (pInfo->CropY >> 1) * pitch;

        for(i = 0; i < h; i++)
        {
            MSDK_CHECK_NOT_EQUAL( fwrite(ptr+ i * pitch, 1, w, m_fDst), w, MFX_ERR_UNDEFINED_BEHAVIOR);
        }
    }
    else if(    pInfo->FourCC == MFX_FOURCC_P010
#if (MFX_VERSION >= 1031)
             || pInfo->FourCC == MFX_FOURCC_P016
#endif
           )
    {
        ptr   = pData->Y + (pInfo->CropX ) + (pInfo->CropY ) * pitch;

        for (i = 0; i < h; i++)
        {
            MSDK_CHECK_NOT_EQUAL( fwrite(ptr+ i * pitch, 1, w * 2, m_fDst), w * 2u, MFX_ERR_UNDEFINED_BEHAVIOR);
        }

        // write UV data
        h     >>= 1;
        ptr  = pData->UV + (pInfo->CropX ) + (pInfo->CropY >> 1) * pitch ;

        for(i = 0; i < h; i++)
        {
            MSDK_CHECK_NOT_EQUAL( fwrite(ptr+ i * pitch, 1, w*2, m_fDst), w*2u, MFX_ERR_UNDEFINED_BEHAVIOR);
        }
    }
    else if( pInfo->FourCC == MFX_FOURCC_P210 )
    {
        ptr   = pData->Y + (pInfo->CropX ) + (pInfo->CropY ) * pitch;

        for (i = 0; i < h; i++)
        {
            MSDK_CHECK_NOT_EQUAL( fwrite(ptr+ i * pitch, 1, w * 2, m_fDst), w * 2u, MFX_ERR_UNDEFINED_BEHAVIOR);
        }

        // write UV data
        ptr  = pData->UV + (pInfo->CropX ) + (pInfo->CropY >> 1) * pitch ;

        for(i = 0; i < h; i++)
        {
            MSDK_CHECK_NOT_EQUAL( fwrite(ptr+ i * pitch, 1, w*2, m_fDst), w*2u, MFX_ERR_UNDEFINED_BEHAVIOR);
        }
    }
    else if( pInfo->FourCC == MFX_FOURCC_YUY2 )
    {
        ptr = pData->Y + pInfo->CropX + pInfo->CropY * pitch;

        for(i = 0; i < h; i++)
        {
            MSDK_CHECK_NOT_EQUAL( fwrite(ptr+ i * pitch, 1, 2*w, m_fDst), 2u*w, MFX_ERR_UNDEFINED_BEHAVIOR);
        }
    }
    else if( pInfo->FourCC == MFX_FOURCC_UYVY )
    {
        ptr = pData->U + pInfo->CropX + pInfo->CropY * pitch;

        for(i = 0; i < h; i++)
        {
            MSDK_CHECK_NOT_EQUAL( fwrite(ptr+ i * pitch, 1, 2*w, m_fDst), 2u*w, MFX_ERR_UNDEFINED_BEHAVIOR);
        }
    }
    else if ( pInfo->FourCC == MFX_FOURCC_IMC3 )
    {
        ptr   = pData->Y + (pInfo->CropX ) + (pInfo->CropY ) * pitch;

        for (i = 0; i < h; i++)
        {
            MSDK_CHECK_NOT_EQUAL( fwrite(ptr+ i * pitch, 1, w, m_fDst), w, MFX_ERR_UNDEFINED_BEHAVIOR);
        }

        w     >>= 1;
        h     >>= 1;

        ptr  = pData->U + (pInfo->CropX >> 1) + (pInfo->CropY >> 1) * pitch;
        for(i = 0; i < h; i++)
        {
            nBytesRead = (mfxU32)fwrite(ptr + i * pitch, 1, w, m_fDst);
            MSDK_CHECK_NOT_EQUAL(nBytesRead, w, MFX_ERR_MORE_DATA);
        }

        ptr  = pData->V + (pInfo->CropX >> 1) + (pInfo->CropY >> 1) * pitch;
        for(i = 0; i < h; i++)
        {
            MSDK_CHECK_NOT_EQUAL( fwrite(ptr+ i * pitch, 1, w, m_fDst), w, MFX_ERR_UNDEFINED_BEHAVIOR);
        }
    }
    else if (pInfo->FourCC == MFX_FOURCC_RGB4 || pInfo->FourCC == MFX_FOURCC_A2RGB10)
    {
        MSDK_CHECK_POINTER(pData->R, MFX_ERR_NOT_INITIALIZED);
        MSDK_CHECK_POINTER(pData->G, MFX_ERR_NOT_INITIALIZED);
        MSDK_CHECK_POINTER(pData->B, MFX_ERR_NOT_INITIALIZED);
        // there is issue with A channel in case of d3d, so A-ch is ignored
        //MSDK_CHECK_POINTER(pData->A, MFX_ERR_NOT_INITIALIZED);

        ptr = std::min(std::min(pData->R, pData->G), pData->B );
        ptr = ptr + pInfo->CropX + pInfo->CropY * pitch;

        for(i = 0; i < h; i++)
        {
            MSDK_CHECK_NOT_EQUAL( fwrite(ptr + i * pitch, 1, 4*w, m_fDst), 4u*w, MFX_ERR_UNDEFINED_BEHAVIOR);
        }
    }
#if !(defined(_WIN32) || defined(_WIN64))
    else if (pInfo->FourCC == MFX_FOURCC_RGBP)
    {
        MSDK_CHECK_POINTER(pData->R, MFX_ERR_NOT_INITIALIZED);
        MSDK_CHECK_POINTER(pData->G, MFX_ERR_NOT_INITIALIZED);
        MSDK_CHECK_POINTER(pData->B, MFX_ERR_NOT_INITIALIZED);

        ptr = pData->R + pInfo->CropX + pInfo->CropY * pitch;
        for(i = 0; i < h; i++)
        {
            MSDK_CHECK_NOT_EQUAL( fwrite(ptr + i * pitch, 1, w, m_fDst), w, MFX_ERR_UNDEFINED_BEHAVIOR);
        }
        ptr = pData->G + pInfo->CropX + pInfo->CropY * pitch;
        for(i = 0; i < h; i++)
        {
            MSDK_CHECK_NOT_EQUAL( fwrite(ptr + i * pitch, 1, w, m_fDst), w, MFX_ERR_UNDEFINED_BEHAVIOR);
        }
        ptr = pData->B + pInfo->CropX + pInfo->CropY * pitch;
        for(i = 0; i < h; i++)
        {
            MSDK_CHECK_NOT_EQUAL( fwrite(ptr + i * pitch, 1, w, m_fDst), w, MFX_ERR_UNDEFINED_BEHAVIOR);
        }
    }
#endif
    else if (pInfo->FourCC == MFX_FOURCC_AYUV)
    {
        ptr = std::min( std::min(pData->Y, pData->U), std::min(pData->V, pData->A) );
        ptr = ptr + pInfo->CropX + pInfo->CropY * pitch;

        for(i = 0; i < h; i++)
        {
            MSDK_CHECK_NOT_EQUAL( fwrite(ptr + i * pitch, 1, 4*w, m_fDst), 4u*w, MFX_ERR_UNDEFINED_BEHAVIOR);
        }
    }
#if (MFX_VERSION >= 1027)
    else if (pInfo->FourCC == MFX_FOURCC_Y210
#if (MFX_VERSION >= 1031)
    || pInfo->FourCC == MFX_FOURCC_Y216
#endif
)
    {
        ptr = pData->Y + pInfo->CropX + pInfo->CropY * pitch;

        for(i = 0; i < h; i++)
        {
            MSDK_CHECK_NOT_EQUAL(fwrite(ptr + i * pitch, 1, 4*w, m_fDst), w * 4u, MFX_ERR_UNDEFINED_BEHAVIOR);
        }
    }
    else if (pInfo->FourCC == MFX_FOURCC_Y410)
    {
        ptr = (mfxU8*)pData->Y410 + pInfo->CropX + pInfo->CropY * pitch;

        for(i = 0; i < h; i++)
        {
            MSDK_CHECK_NOT_EQUAL(fwrite(ptr + i * pitch, 1, 4*w, m_fDst), w * 4u, MFX_ERR_UNDEFINED_BEHAVIOR);
        }
    }
#endif
#if (MFX_VERSION >= 1031)
    else if (pInfo->FourCC == MFX_FOURCC_Y416)
    {
        ptr = (mfxU8*)(pData->U16 + pInfo->CropX * 4) + pInfo->CropY * pitch;

        for (i = 0; i < h; i++)
        {
            MSDK_CHECK_NOT_EQUAL(fwrite(ptr + i * pitch, 1, 8 * w, m_fDst), w * 8u, MFX_ERR_UNDEFINED_BEHAVIOR);
        }
    }
#endif
    else
    {
        return MFX_ERR_UNSUPPORTED;
    }

    return MFX_ERR_NONE;
}

/* ******************************************************************* */

GeneralWriter::GeneralWriter() :
    m_svcMode(false)
{
};


GeneralWriter::~GeneralWriter()
{
    Close();
};


void GeneralWriter::Close()
{
    for(mfxU32 did = 0; did < 8; did++)
    {
        m_ofile[did].reset();
    }
};


mfxStatus GeneralWriter::Init(
    const msdk_char *strFileName,
    PTSMaker *pPTSMaker,
    sSVCLayerDescr*  pDesc,
    mfxU32 forcedOutputFourcc)
{
    mfxStatus sts = MFX_ERR_UNKNOWN;

    mfxU32 didCount = (pDesc) ? 8 : 1;
    m_svcMode = (pDesc) ? true : false;

    for(mfxU32 did = 0; did < didCount; did++)
    {
        if( (1 == didCount) || (pDesc[did].active) )
        {
            m_ofile[did].reset(new CRawVideoWriter() );
            if(0 == m_ofile[did].get())
            {
                return MFX_ERR_UNKNOWN;
            }

            msdk_char out_buf[MSDK_MAX_FILENAME_LEN*4+20];
            msdk_char fname[MSDK_MAX_FILENAME_LEN];

#if defined(_WIN32) || defined(_WIN64)
            {
                msdk_char drive[MSDK_MAX_FILENAME_LEN];
                msdk_char dir[MSDK_MAX_FILENAME_LEN];
                msdk_char ext[MSDK_MAX_FILENAME_LEN];

                _tsplitpath_s(
                    strFileName,
                    drive,
                    dir,
                    fname,
                    ext);

                msdk_sprintf(out_buf, MSDK_STRING("%s%s%s_layer%i.yuv"), drive, dir, fname, did);
            }
#else
            {
                msdk_strncopy_s(fname, MSDK_MAX_FILENAME_LEN, strFileName, MSDK_MAX_FILENAME_LEN - 1);
                fname[MSDK_MAX_FILENAME_LEN - 1] = 0;
                char* pFound = strrchr(fname,'.');
                if(pFound)
                {
                    *pFound=0;
                }
                msdk_sprintf(out_buf, MSDK_STRING("%s_layer%i.yuv"), fname, did);
            }
#endif

            sts = m_ofile[did]->Init(
                (1 == didCount) ? strFileName : out_buf,
                pPTSMaker,
                forcedOutputFourcc);

            if(sts != MFX_ERR_NONE) break;
        }
    }

    return sts;
};

mfxStatus  GeneralWriter::PutNextFrame(
    sMemoryAllocator* pAllocator,
    mfxFrameInfo* pInfo,
    mfxFrameSurfaceWrap* pSurface)
{
    mfxU32 did = (m_svcMode) ? pSurface->Info.FrameId.DependencyId : 0;//aya: for MVC we have 1 out file only

    mfxStatus sts = m_ofile[did]->PutNextFrame(pAllocator, pInfo, pSurface);

    return sts;
};

/* ******************************************************************* */

mfxStatus UpdateSurfacePool(mfxFrameInfo SurfacesInfo, mfxU16 nPoolSize, mfxFrameSurfaceWrap* pSurface)
{
    MSDK_CHECK_POINTER(pSurface,     MFX_ERR_NULL_PTR);
    if (pSurface)
    {
        for (mfxU16 i = 0; i < nPoolSize; i++)
        {
            pSurface[i].Info = SurfacesInfo;
        }
    }
    return MFX_ERR_NONE;
}

mfxStatus GetFreeSurface(mfxFrameSurfaceWrap* pSurfacesPool, mfxU16 nPoolSize, mfxFrameSurfaceWrap** ppSurface)
{
    MSDK_CHECK_POINTER(pSurfacesPool, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(ppSurface,     MFX_ERR_NULL_PTR);

    mfxU32 timeToSleep = 10; // milliseconds
    mfxU32 numSleeps = MSDK_SURFACE_WAIT_INTERVAL / timeToSleep + 1; // at least 1

    mfxU32 i = 0;

    //wait if there's no free surface
    while ((MSDK_INVALID_SURF_IDX == GetFreeSurfaceIndex(pSurfacesPool, nPoolSize)) && (i < numSleeps))
    {
        MSDK_SLEEP(timeToSleep);
        i++;
    }

    mfxU16 index = GetFreeSurfaceIndex(pSurfacesPool, nPoolSize);

    if (index < nPoolSize)
    {
        *ppSurface = &(pSurfacesPool[index]);
        return MFX_ERR_NONE;
    }

    return MFX_ERR_NOT_ENOUGH_BUFFER;
}

//---------------------------------------------------------

void PrintDllInfo()
{
#if defined(_WIN32) || defined(_WIN64)
    HANDLE   hCurrent = GetCurrentProcess();
    HMODULE *pModules;
    DWORD    cbNeeded;
    int      nModules;
    if (NULL == EnumProcessModules(hCurrent, NULL, 0, &cbNeeded))
        return;

    nModules = cbNeeded / sizeof(HMODULE);

    pModules = new HMODULE[nModules];
    if (NULL == pModules)
    {
        return;
    }
    if (NULL == EnumProcessModules(hCurrent, pModules, cbNeeded, &cbNeeded))
    {
        delete []pModules;
        return;
    }

    for (int i = 0; i < nModules; i++)
    {
        msdk_char buf[2048];
        GetModuleFileName(pModules[i], buf, ARRAYSIZE(buf));
        if (_tcsstr(buf, MSDK_STRING("libmfx")))
        {
            msdk_printf(MSDK_STRING("MFX dll         %s\n"),buf);
        }
    }
    delete []pModules;
#endif
} // void PrintDllInfo()

/* ******************************************************************* */

/* EOF */
