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

#if defined(_WIN32) || defined(_WIN64)
#include <tchar.h>
#include <windows.h>
#endif
#include <numeric>
#include <ctime>
#include <algorithm>
#include "pipeline_camera.h"
#include "camera_sysmem_allocator.h"

#if defined(_WIN32) || defined(_WIN64)
#include "d3d_allocator.h"
#include "d3d11_allocator.h"
#include "d3d_device.h"
#include "d3d11_device.h"
#endif

#include "version.h"

#pragma warning(disable : 4100)
#pragma warning(disable : 4996)

//Library plugin UID
static const mfxU8 CAMERA_PIPE_UID[]  = {0x54, 0x54, 0x26, 0x16, 0x24, 0x33, 0x41, 0xe6, 0x93, 0xae, 0x89, 0x99, 0x42, 0xce, 0x73, 0x55};

#define CAM_SYSMEM_ALIGNMENT 0x1000 // CM alignment

//#define CAMP_PIPE_ITT
#ifdef CAMP_PIPE_ITT
#include "ittnotify.h"


__itt_domain* CamPipeAccel = __itt_domain_create(L"CamPipeAccel");

//__itt_string_handle* CPU_file_fread;
//__itt_string_handle* CPU_raw_unpack_;
__itt_string_handle* task1 = __itt_string_handle_create(L"submit");;
__itt_string_handle* task2 = __itt_string_handle_create(L"bmp_dump");;
__itt_string_handle* task3 = __itt_string_handle_create(L"rendering");;
__itt_string_handle* task4 = __itt_string_handle_create(L"LoadFrame");;

__itt_string_handle* task5 = __itt_string_handle_create(L"AsyncSubmits");;

#endif

mfxU16 gamma_point[64] =
{
    0,  94, 104, 114, 124, 134, 144, 154, 159, 164, 169, 174, 179, 184, 194, 199,
    204, 209, 214, 219, 224, 230, 236, 246, 256, 266, 276, 286, 296, 306, 316, 326,
    336, 346, 356, 366, 376, 386, 396, 406, 416, 426, 436, 446, 456, 466, 476, 486,
    496, 516, 526, 536, 546, 556, 566, 576, 586, 596, 606, 616, 626, 636, 646, 1023
};

mfxU16 gamma_correct[64] =
{
    0,   4,  20,  37,  56,  75,  96, 117, 128, 140, 150, 161, 171, 180, 198, 207,
    216, 224, 232, 240, 249, 258, 268, 283, 298, 310, 329, 344, 359, 374, 389, 404,
    420, 435, 451, 466, 482, 498, 515, 531, 548, 565, 582, 599, 617, 635, 653, 671,
    690, 729, 749, 769, 790, 811, 832, 854, 876, 899, 922, 945, 969, 994, 1019,1019
};



mfxStatus CCameraPipeline::InitMfxParams(sInputParams *pParams)
{
    MSDK_CHECK_POINTER(m_pmfxVPP, MFX_ERR_NULL_PTR);
    mfxStatus sts = MFX_ERR_NONE;

    if (pParams->bDoPadding)
    {
        // first 8 lines are padded data. Original image should be
        // placed starting 8,8 position into the input frame
        m_mfxVideoParams.vpp.In.CropX  = CAMERA_PADDING_SIZE;
        m_mfxVideoParams.vpp.In.CropY  = CAMERA_PADDING_SIZE;

        // Take into account padding on each edge
        m_mfxVideoParams.vpp.In.Width  = (mfxU16)pParams->frameInfo[VPP_IN].nWidth  + CAMERA_PADDING_SIZE + CAMERA_PADDING_SIZE;
        m_mfxVideoParams.vpp.In.Height = (mfxU16)pParams->frameInfo[VPP_IN].nHeight + CAMERA_PADDING_SIZE + CAMERA_PADDING_SIZE;
    }
    else
    {
        m_mfxVideoParams.vpp.In.CropX = 0;
        m_mfxVideoParams.vpp.In.CropY = 0;
        m_mfxVideoParams.vpp.In.Width  = (mfxU16)pParams->frameInfo[VPP_IN].nWidth;
        m_mfxVideoParams.vpp.In.Height = (mfxU16)pParams->frameInfo[VPP_IN].nHeight;
    }

    // Width and height values must be aligned in order to arhive maximum paerfromance
    m_mfxVideoParams.vpp.In.Width  = align_32(m_mfxVideoParams.vpp.In.Width);
    m_mfxVideoParams.vpp.In.Height = align_32(m_mfxVideoParams.vpp.In.Height);

    m_mfxVideoParams.vpp.In.CropW = (mfxU16)pParams->frameInfo[VPP_IN].CropW;
    m_mfxVideoParams.vpp.In.CropH = (mfxU16)pParams->frameInfo[VPP_IN].CropH;

    // Add additional CropX,CropY if any
    m_mfxVideoParams.vpp.In.CropX += align_32((mfxU16)pParams->frameInfo[VPP_IN].CropX);
    m_mfxVideoParams.vpp.In.CropY += align_32((mfxU16)pParams->frameInfo[VPP_IN].CropY);
    m_mfxVideoParams.vpp.In.FourCC = pParams->frameInfo[VPP_IN].FourCC;
    //Only R16 input supported now, should use chroma format monochrome
    m_mfxVideoParams.vpp.In.ChromaFormat = MFX_CHROMAFORMAT_YUV444;
    if (isBayerFormat(pParams->frameInfo[VPP_IN].FourCC))
    {
        m_mfxVideoParams.vpp.In.ChromaFormat = MFX_CHROMAFORMAT_MONOCHROME;
    }

    m_mfxVideoParams.vpp.In.BitDepthLuma = (mfxU16)pParams->bitDepth;

    // CropW of the output frame must be the same as CropW of the input. Resize is not supported
    // The same for CropH
    m_mfxVideoParams.vpp.Out.CropW = (mfxU16)pParams->frameInfo[VPP_OUT].CropW;
    m_mfxVideoParams.vpp.Out.CropH = (mfxU16)pParams->frameInfo[VPP_OUT].CropH;

    m_mfxVideoParams.vpp.Out.Width  = align_32((mfxU16)pParams->frameInfo[VPP_OUT].nWidth);
    m_mfxVideoParams.vpp.Out.Height = align_32((mfxU16)pParams->frameInfo[VPP_OUT].nHeight);
    m_mfxVideoParams.vpp.Out.CropX  = align_32((mfxU16)pParams->frameInfo[VPP_OUT].CropX);
    m_mfxVideoParams.vpp.Out.CropY  = align_32((mfxU16)pParams->frameInfo[VPP_OUT].CropY);
    m_mfxVideoParams.vpp.Out.FourCC = pParams->frameInfo[VPP_OUT].FourCC;

    if (m_mfxVideoParams.vpp.Out.FourCC == MFX_FOURCC_NV12) {
        m_mfxVideoParams.vpp.Out.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
        m_mfxVideoParams.vpp.Out.BitDepthLuma = 8;
    }
    else {
        //Only ARGB onput supported now, should use chroma format 444
        m_mfxVideoParams.vpp.Out.ChromaFormat = MFX_CHROMAFORMAT_YUV444;
        m_mfxVideoParams.vpp.Out.BitDepthLuma = pParams->frameInfo[VPP_OUT].FourCC == MFX_FOURCC_RGB4 ? 8 : m_mfxVideoParams.vpp.In.BitDepthLuma;
    }
    ConvertFrameRate(pParams->frameInfo[VPP_IN].dFrameRate, &m_mfxVideoParams.vpp.In.FrameRateExtN, &m_mfxVideoParams.vpp.In.FrameRateExtD);
    ConvertFrameRate(pParams->frameInfo[VPP_OUT].dFrameRate, &m_mfxVideoParams.vpp.Out.FrameRateExtN, &m_mfxVideoParams.vpp.Out.FrameRateExtD);

    // specify memory type
    if (m_memTypeIn != SYSTEM_MEMORY)
        m_mfxVideoParams.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY;
    else
        m_mfxVideoParams.IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY;

    if (m_memTypeOut != SYSTEM_MEMORY)
        m_mfxVideoParams.IOPattern |= MFX_IOPATTERN_OUT_VIDEO_MEMORY;
    else
        m_mfxVideoParams.IOPattern |= MFX_IOPATTERN_OUT_SYSTEM_MEMORY;

    if ( m_mfxVideoParams.vpp.Out.Width > 8*1024 && m_mfxVideoParams.vpp.Out.Height > 8*1024)
    {
        // If frame resolution is bigger than 8Kx8K, force async to 1 in order to limit memory consumption.
        pParams->asyncDepth = 1;
    }

    if (pParams->asyncDepth >= 0)
        m_mfxVideoParams.AsyncDepth = (mfxU16)pParams->asyncDepth;
    else
        m_mfxVideoParams.AsyncDepth = CAM_SAMPLE_ASYNC_DEPTH;

    b_3DLUT_Gamma = false;
    if (pParams->bGamma)
    {
        sts = AllocAndInitCamGammaCorrection(pParams);
        MSDK_CHECK_STATUS(sts, "AllocAndInitCamGammaCorrection failed");
        if ( pParams->b3DLUTGamma )
        {
            b_3DLUT_Gamma = true;
            m_ExtBuffers.push_back((mfxExtBuffer *)&m_3DLUT_GammaCorrection);
        }
        else
        {
            m_ExtBuffers.push_back((mfxExtBuffer *)&m_GammaCorrection);
        }
    }

    mfxU32 table_size = 17; // 17,33,65 otherwise error
    if (pParams->b3DLUT)
    {

        mfxU16 R_INC     = 0;
        mfxU16 G_INC     = 0;
        mfxU16 B_INC     = 0;
        mfxU16 TABLE_INC = 0;
        mfxU16 SEG       = 0;

        mfxCam3DLutEntry *table_3dlut = NULL;

        if (table_size == 17 )
        {
            table_3dlut = m_3dlut_17;
            memset(m_3dlut_17, 0, sizeof(mfxCam3DLutEntry)*MFX_CAM_3DLUT17_SIZE);
            m_3DLUT.Size = MFX_CAM_3DLUT17_SIZE;

            TABLE_INC = 4096;
            SEG = 17;
        }
        else if (table_size == 33 )
        {
            table_3dlut = m_3dlut_33;
            memset(m_3dlut_33, 0, sizeof(mfxCam3DLutEntry)*MFX_CAM_3DLUT33_SIZE);
            m_3DLUT.Size = MFX_CAM_3DLUT33_SIZE;

            TABLE_INC = 4096>>1;
            SEG = 33;
        }
        else if (table_size == 65 )
        {
            table_3dlut = m_3dlut_65;
            memset(m_3dlut_65, 0, sizeof(mfxCam3DLutEntry)*MFX_CAM_3DLUT65_SIZE);
            m_3DLUT.Size = MFX_CAM_3DLUT65_SIZE;

            TABLE_INC = 4096>>2;
            SEG = 65;
        }

        for(int i = 0; i < SEG; i++)
        {
            for(int j = 0; j < SEG; j++)
            {
                for(int k = 0; k < SEG; k++)
                {
                    table_3dlut[SEG*SEG*i+SEG*j+k].R = R_INC;
                    table_3dlut[SEG*SEG*i+SEG*j+k].G = G_INC;
                    table_3dlut[SEG*SEG*i+SEG*j+k].B = B_INC;

                    if ( k == SEG-2 )
                        B_INC += TABLE_INC-1;
                    else if ( k < (SEG-2) )
                        B_INC += TABLE_INC;
                }

                B_INC = 0;
                if ( j == SEG-2 )
                       G_INC += TABLE_INC-1;
                else
                       G_INC += TABLE_INC;

            }

            B_INC = G_INC = 0;
            if ( i == SEG-2 )
                   R_INC += TABLE_INC-1;
            else
                R_INC += TABLE_INC;

        }
        m_3DLUT.Table = table_3dlut;
        m_ExtBuffers.push_back((mfxExtBuffer *)&m_3DLUT);
    }

    if (pParams->bVignette)
    {
        sts = AllocAndInitVignetteCorrection(pParams);
        MSDK_CHECK_STATUS(sts, "AllocAndInitVignetteCorrection failed");
        m_ExtBuffers.push_back((mfxExtBuffer *)&m_Vignette);
    }

    if (pParams->bBlackLevel)
    {
        sts = AllocAndInitCamBlackLevelCorrection(pParams);
        MSDK_CHECK_STATUS(sts, "AllocAndInitCamBlackLevelCorrection failed");
        m_ExtBuffers.push_back((mfxExtBuffer *)&m_BlackLevelCorrection);
    }
#if MFX_VERSION >= 1023
    if (pParams->bTCC)
    {
        sts = AllocAndInitCamTotalColorControl(pParams);
        MSDK_CHECK_STATUS(sts, "AllocAndInitCamTotalColorControl failed");
        m_ExtBuffers.push_back((mfxExtBuffer *)&m_TotalColorControl);
    }
    if (pParams->bRGBToYUV)
    {
        sts = AllocAndInitCamRGBtoYUV(pParams);
        MSDK_CHECK_STATUS(sts, "AllocAndInitCamRGBToYUV failed");
        m_ExtBuffers.push_back((mfxExtBuffer *)&m_RGBToYUV);
    }
#endif
    if (pParams->bHP)
    {
        sts = AllocAndInitCamHotPixelRemoval(pParams);
        MSDK_CHECK_STATUS(sts, "AllocAndInitCamHotPixelRemoval failed");
        m_ExtBuffers.push_back((mfxExtBuffer *)&m_HP);
    }

    if (pParams->bBayerDenoise)
    {
        sts = AllocAndInitDenoise(pParams);
        MSDK_CHECK_STATUS(sts, "AllocAndInitDenoise failed");
        m_ExtBuffers.push_back((mfxExtBuffer *)&m_Denoise);
    }

    if (pParams->bWhiteBalance)
    {
        sts = AllocAndInitCamWhiteBalance(pParams);
        MSDK_CHECK_STATUS(sts, "AllocAndInitCamWhiteBalance failed");
        m_ExtBuffers.push_back((mfxExtBuffer *)&m_WhiteBalance);
    }

    if (pParams->bCCM)
    {
        sts = AllocAndInitCamCCM(pParams);
        MSDK_CHECK_STATUS(sts, "AllocAndInitCamCCM failed");
        m_ExtBuffers.push_back((mfxExtBuffer *)&m_CCM);
    }

    if (pParams->bLens)
    {
        sts = AllocAndInitCamLens(pParams);
        MSDK_CHECK_STATUS(sts, "AllocAndInitCamLens failed");
        m_ExtBuffers.push_back((mfxExtBuffer *)&m_Lens);
    }

    m_PipeControl.Header.BufferId = MFX_EXTBUF_CAM_PIPECONTROL;
    m_PipeControl.Header.BufferSz = sizeof(m_PipeControl);
    m_PipeControl.RawFormat = (mfxU16)pParams->inputType;
    m_ExtBuffers.push_back((mfxExtBuffer *)&m_PipeControl);

    if (pParams->bDoPadding)
    {
        m_Padding.Header.BufferId = MFX_EXTBUF_CAM_PADDING;
        m_Padding.Header.BufferSz = sizeof(m_Padding);
        m_Padding.Top = m_Padding.Bottom = m_Padding.Left = m_Padding.Right = CAMERA_PADDING_SIZE;
        m_ExtBuffers.push_back((mfxExtBuffer *)&m_Padding);
    }

    if (!m_ExtBuffers.empty())
        AttachExtParam();

    return MFX_ERR_NONE;
}


mfxStatus CCameraPipeline::CreateHWDevice()
{
#if D3D_SURFACES_SUPPORT
    mfxStatus sts = MFX_ERR_NONE;

    HWND window = NULL;

    if (!m_bIsRender)
        window = 0;
    else
        window = m_d3dRender.GetWindowHandle();

#if MFX_D3D11_SUPPORT
    if (D3D11 == m_accelType)
        m_hwdev = new CD3D11Device();
    else
#endif // #if MFX_D3D11_SUPPORT
        m_hwdev = new CD3D9Device();

    if (NULL == m_hwdev)
        return MFX_ERR_MEMORY_ALLOC;
    sts = m_hwdev->Init(
        window,
        m_bIsRender ? 1 : 0,
        MSDKAdapter::GetNumber(m_mfxSession));
    MSDK_CHECK_STATUS(sts, "m_hwdev->Init failed");

    if (m_bIsRender)
        m_d3dRender.SetHWDevice(m_hwdev);
#endif
    return MFX_ERR_NONE;
}

#define NUM_INPUT_FRAMES_MULTIPLIER 2

mfxStatus CCameraPipeline::AllocFrames()
{
    MSDK_CHECK_POINTER(m_pmfxVPP, MFX_ERR_NULL_PTR);

    mfxStatus sts = MFX_ERR_NONE;

    mfxFrameAllocRequest Request[2];

    MSDK_ZERO_MEMORY(Request);

    // calculate number of surfaces required for camera pipe
    sts = m_pmfxVPP->QueryIOSurf(&m_mfxVideoParams, Request);
    if (MFX_WRN_PARTIAL_ACCELERATION == sts)
    {
        msdk_printf(MSDK_STRING("WARNING: partial acceleration\n"));
        MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    }
    MSDK_CHECK_STATUS(sts, "m_pmfxVPP->QueryIOSurf failed");

    // kta !!! tmp ???
    Request[VPP_IN].NumFrameSuggested *= NUM_INPUT_FRAMES_MULTIPLIER;

    if (m_memTypeIn != SYSTEM_MEMORY) {
        Request[VPP_IN].Type |= MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET;
    } else {
        Request[VPP_IN].Type |= MFX_MEMTYPE_SYSTEM_MEMORY;
    }

    if (m_memTypeOut != SYSTEM_MEMORY) {
        Request[VPP_OUT].Type |= MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET;
    } else {
        Request[VPP_OUT].Type |= MFX_MEMTYPE_SYSTEM_MEMORY;
    }

    sMemoryAllocator allocatorIn = {m_pMFXAllocatorIn, m_pmfxAllocatorParamsIn, &m_pmfxSurfacesIn, &m_mfxResponseIn};
    // alloc frames for vpp
    // [IN]
    sts = InitSurfaces(&allocatorIn, &(Request[VPP_IN]), &(m_mfxVideoParams.vpp.In), m_bExternalAllocIn);
    MSDK_CHECK_STATUS(sts, "InitSurfaces failed");

    // [OUT]
    sMemoryAllocator allocatorOut = {m_pMFXAllocatorOut, m_pmfxAllocatorParamsOut, &m_pmfxSurfacesOut, &m_mfxResponseOut};
    sts = InitSurfaces(&allocatorOut, &(Request[VPP_OUT]), &(m_mfxVideoParams.vpp.Out), m_bExternalAllocOut);
    MSDK_CHECK_STATUS(sts, "InitSurfaces failed");

    if (m_memTypeOut == SYSTEM_MEMORY && m_bIsRender) {

        mfxFrameAllocRequest req;
        MSDK_ZERO_MEMORY(req);

        req.Info = m_mfxVideoParams.vpp.Out;
        req.NumFrameMin = req.NumFrameSuggested = 1;
        req.Type = MFX_MEMTYPE_FROM_VPPOUT | MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET;

        sMemoryAllocator allocator = {m_pMFXd3dAllocator, m_pmfxd3dAllocatorParams, &m_pmfxSurfacesAux, &m_mfxResponseAux};

        sts = InitSurfaces(&allocator, &req, &(m_mfxVideoParams.vpp.Out), true);
        MSDK_CHECK_STATUS(sts, "InitSurfaces failed");
    }

    m_lastAllocRequest[VPP_IN] = Request[VPP_IN];
    m_lastAllocRequest[VPP_OUT] = Request[VPP_OUT];

    return MFX_ERR_NONE;
}


mfxStatus CCameraPipeline::ReallocFrames(mfxVideoParam *oldMfxPar)
{
    MSDK_CHECK_POINTER(m_pmfxVPP, MFX_ERR_NULL_PTR);

    mfxStatus sts = MFX_ERR_NONE;

    mfxFrameAllocRequest Request[2];
    MSDK_ZERO_MEMORY(Request);

    // calculate number of surfaces required for camera pipe
    sts = m_pmfxVPP->QueryIOSurf(&m_mfxVideoParams, Request);
    MSDK_CHECK_STATUS(sts, "m_pmfxVPP->QueryIOSurf failed");

    if (m_memTypeIn != SYSTEM_MEMORY) {
        Request[VPP_IN].Type |= MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET;
    } else {
        Request[VPP_IN].Type |= MFX_MEMTYPE_SYSTEM_MEMORY;
    }

    if (m_memTypeOut != SYSTEM_MEMORY) {
        Request[VPP_OUT].Type |= MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET;
    } else {
        Request[VPP_OUT].Type |= MFX_MEMTYPE_SYSTEM_MEMORY;
    }

    if (Request[VPP_IN].Type != m_lastAllocRequest[VPP_IN].Type ||  Request[VPP_OUT].Type != m_lastAllocRequest[VPP_OUT].Type)
        return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;

    Request[VPP_IN].NumFrameSuggested *= NUM_INPUT_FRAMES_MULTIPLIER;

    bool realloc[2] = {false, false};
    for (int i = 0; i < 2; i++) {
        if (Request[i].NumFrameSuggested > m_lastAllocRequest[i].NumFrameSuggested) {
            realloc[i] = true;
        }

        if (Request[i].Info.Width > m_lastAllocRequest[i].Info.Width || Request[i].Info.Height > m_lastAllocRequest[i].Info.Height) {
            realloc[i] = true;
        }
    }

    if (realloc[VPP_IN] || realloc[VPP_OUT]) // need to Close/Init
        return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;

    for (int j = 0; j < m_mfxResponseOut.NumFrameActual; j++) {
        m_pmfxSurfacesOut[j].Info = m_mfxVideoParams.vpp.Out;
    }

    for (int j = 0; j < m_mfxResponseIn.NumFrameActual; j++) {
        m_pmfxSurfacesIn[j].Info = m_mfxVideoParams.vpp.In;
    }

    return MFX_ERR_NONE;
}


mfxStatus CCameraPipeline::CreateAllocator()
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxHDL hdl = NULL;

    m_bExternalAllocOut = false;
    m_bExternalAllocIn = false;

    sts = CreateHWDevice();
    MSDK_CHECK_STATUS(sts, "CreateHWDevice failed");

    // provide device manager to MediaSDK
    //mfxHDL hdl = NULL;
    mfxHandleType hdl_t =  D3D11 == m_accelType ? MFX_HANDLE_D3D11_DEVICE : MFX_HANDLE_D3D9_DEVICE_MANAGER;

    sts = m_hwdev->GetHandle(hdl_t, &hdl);
    MSDK_CHECK_STATUS(sts, "m_hwdev->GetHandle failed");
    sts = m_mfxSession.SetHandle(hdl_t, hdl);
    MSDK_CHECK_STATUS(sts, "m_mfxSession.SetHandle failed");

    if (m_memTypeIn != SYSTEM_MEMORY || m_memTypeOut != SYSTEM_MEMORY)
    {
#if MFX_D3D11_SUPPORT
        // create D3D allocator
        if (D3D11_MEMORY == m_memTypeIn || D3D11_MEMORY == m_memTypeOut)
        {
            m_pMFXd3dAllocator = new D3D11FrameAllocator;
            MSDK_CHECK_POINTER(m_pMFXd3dAllocator, MFX_ERR_MEMORY_ALLOC);

            D3D11AllocatorParams *pd3dAllocParams = new D3D11AllocatorParams;
            MSDK_CHECK_POINTER(pd3dAllocParams, MFX_ERR_MEMORY_ALLOC);
            pd3dAllocParams->pDevice = reinterpret_cast<ID3D11Device *>(hdl);

            m_pmfxd3dAllocatorParams = pd3dAllocParams;
        }
#endif

        if (D3D9_MEMORY == m_memTypeIn || D3D9_MEMORY == m_memTypeOut)
        {
            m_pMFXd3dAllocator = new D3DFrameAllocator;
            MSDK_CHECK_POINTER(m_pMFXd3dAllocator, MFX_ERR_MEMORY_ALLOC);

            D3DAllocatorParams *pd3dAllocParams = new D3DAllocatorParams;
            MSDK_CHECK_POINTER(pd3dAllocParams, MFX_ERR_MEMORY_ALLOC);
            pd3dAllocParams->pManager = reinterpret_cast<IDirect3DDeviceManager9 *>(hdl);

            m_pmfxd3dAllocatorParams = pd3dAllocParams;
        }

        if (m_memTypeIn != SYSTEM_MEMORY)
        {
            m_pMFXAllocatorIn = m_pMFXd3dAllocator;
            m_pmfxAllocatorParamsIn = m_pmfxd3dAllocatorParams;
            m_bExternalAllocIn = true;
        }

        if (m_memTypeOut != SYSTEM_MEMORY)
        {
            m_pMFXAllocatorOut = m_pMFXd3dAllocator;
            m_pmfxAllocatorParamsOut = m_pmfxd3dAllocatorParams;
            m_bExternalAllocOut = true;
        }


        /* In case of video memory we must provide MediaSDK with external allocator
        thus we demonstrate "external allocator" usage model.
        Call SetAllocator to pass allocator to mediasdk */
        sts = m_mfxSession.SetFrameAllocator(m_pMFXd3dAllocator);
        MSDK_CHECK_STATUS(sts, "m_mfxSession.SetFrameAllocator failed");
    }

    if (m_memTypeIn == SYSTEM_MEMORY || m_memTypeOut == SYSTEM_MEMORY)
    {
        // create system memory allocator
        m_pMFXsysAllocator = new CamSysMemFrameAllocator;
        MSDK_CHECK_POINTER(m_pMFXsysAllocator, MFX_ERR_MEMORY_ALLOC);

        CamSysMemAllocatorParams *pCamSysAllocParams = new CamSysMemAllocatorParams;
        MSDK_CHECK_POINTER(pCamSysAllocParams, MFX_ERR_MEMORY_ALLOC);
        pCamSysAllocParams->alignment = CAM_SYSMEM_ALIGNMENT;
        m_pmfxsysAllocatorParams = pCamSysAllocParams;

        if (m_memTypeIn == SYSTEM_MEMORY) {
            m_pMFXAllocatorIn = m_pMFXsysAllocator;
            m_pmfxAllocatorParamsIn = m_pmfxsysAllocatorParams;
        }

        if (m_memTypeOut == SYSTEM_MEMORY) {
            m_pMFXAllocatorOut = m_pMFXsysAllocator;
            m_pmfxAllocatorParamsOut = m_pmfxsysAllocatorParams;
        }

        /* In case of system memory we demonstrate "no external allocator" usage model.
        We don't call SetAllocator, MediaSDK uses internal allocator.
        We use system memory allocator simply as a memory manager for application*/
    }

    if (m_memTypeOut == SYSTEM_MEMORY && m_bIsRender) {
#if MFX_D3D11_SUPPORT
        m_pMFXd3dAllocator = new D3D11FrameAllocator;
        MSDK_CHECK_POINTER(m_pMFXd3dAllocator, MFX_ERR_MEMORY_ALLOC);

        D3D11AllocatorParams *pd3dAllocParams = new D3D11AllocatorParams;
        MSDK_CHECK_POINTER(pd3dAllocParams, MFX_ERR_MEMORY_ALLOC);
        pd3dAllocParams->pDevice = reinterpret_cast<ID3D11Device *>(hdl);
#else
        m_pMFXd3dAllocator =  new D3DFrameAllocator;
        MSDK_CHECK_POINTER(m_pMFXd3dAllocator, MFX_ERR_MEMORY_ALLOC);

        D3DAllocatorParams *pd3dAllocParams = new D3DAllocatorParams;
        MSDK_CHECK_POINTER(pd3dAllocParams, MFX_ERR_MEMORY_ALLOC);
        pd3dAllocParams->pManager = reinterpret_cast<IDirect3DDeviceManager9 *>(hdl);
#endif
        m_pmfxd3dAllocatorParams = pd3dAllocParams;

        sts = m_pMFXd3dAllocator->Init(m_pmfxd3dAllocatorParams);
        MSDK_CHECK_STATUS(sts, "m_pMFXd3dAllocator->Init failed");

        sts = m_mfxSession.SetFrameAllocator(m_pMFXd3dAllocator);
        MSDK_CHECK_STATUS(sts, "m_mfxSession.SetFrameAllocator failed");
    }
    // initialize memory allocator(s)
    sts = m_pMFXAllocatorIn->Init(m_pmfxAllocatorParamsIn);
    MSDK_CHECK_STATUS(sts, "m_pMFXAllocatorIn->Init failed");

    if (m_pMFXAllocatorOut != m_pMFXAllocatorIn) {
        sts = m_pMFXAllocatorOut->Init(m_pmfxAllocatorParamsOut);
        MSDK_CHECK_STATUS(sts, "m_pMFXAllocatorOut->Init failed");
    }

    return MFX_ERR_NONE;
}

void CCameraPipeline::DeleteFrames()
{
    // delete surfaces array
    MSDK_SAFE_DELETE_ARRAY(m_pmfxSurfacesIn);
    MSDK_SAFE_DELETE_ARRAY(m_pmfxSurfacesOut);
    MSDK_SAFE_DELETE_ARRAY(m_pmfxSurfacesAux);

    // delete frames
    if (m_pMFXAllocatorIn)
    {
        m_pMFXAllocatorIn->Free(m_pMFXAllocatorIn->pthis, &m_mfxResponseIn);
    }
    if (m_pMFXAllocatorOut)
    {
        m_pMFXAllocatorOut->Free(m_pMFXAllocatorOut->pthis, &m_mfxResponseOut);
    }
    if (m_pMFXd3dAllocator)
    {
        m_pMFXd3dAllocator->Free(m_pMFXd3dAllocator->pthis, &m_mfxResponseAux);
    }

    return;
}

void CCameraPipeline::DeleteAllocator()
{
    // delete allocator
    MSDK_SAFE_DELETE(m_pMFXd3dAllocator);
    MSDK_SAFE_DELETE(m_pMFXsysAllocator);
    MSDK_SAFE_DELETE(m_pmfxd3dAllocatorParams);
    MSDK_SAFE_DELETE(m_pmfxsysAllocatorParams);
    MSDK_SAFE_DELETE(m_hwdev);

    m_pMFXAllocatorIn = m_pMFXAllocatorOut = NULL;
    m_pmfxAllocatorParamsIn = m_pmfxAllocatorParamsIn = NULL;
}

CCameraPipeline::CCameraPipeline()
{
    m_resetInterval = 0;
    MSDK_ZERO_MEMORY(m_Padding);
    MSDK_ZERO_MEMORY(m_PipeControl);
    b_3DLUT_Gamma=false;
    m_numberOfResets=0;
    m_nFrameLimit=0;
    m_BayerType=0;

    m_nFrameIndex = 0;
    m_nInputFileIndex = 0;
    m_pmfxVPP = NULL;
    m_pMFXd3dAllocator = NULL;
    m_pMFXsysAllocator = NULL;
    m_pMFXAllocatorIn = NULL;
    m_pMFXAllocatorOut = NULL;
    m_pmfxd3dAllocatorParams = NULL;
    m_pmfxsysAllocatorParams = NULL;
    m_pmfxAllocatorParamsIn = NULL;
    m_pmfxAllocatorParamsOut = NULL;
    m_accelType = D3D9;
    m_memTypeIn = SYSTEM_MEMORY;
    m_memTypeOut = SYSTEM_MEMORY;
    m_bExternalAllocIn = false;
    m_bExternalAllocOut = false;
    m_pmfxSurfacesIn = m_pmfxSurfacesOut = NULL;
    m_pmfxSurfacesAux = NULL;
    m_pFileReader = NULL;
    m_pBmpWriter = NULL;
    m_pRawFileWriter = NULL;
    m_bIsExtBuffers = false;
    m_bIsRender = false;
    m_bOutput = true;
    m_bEnd = false;
    m_alphaValue = -1;
    m_resetCnt = 0;

    m_3dlut_17 = (mfxCam3DLutEntry*) malloc (sizeof(mfxCam3DLutEntry)*MFX_CAM_3DLUT17_SIZE);
    m_3dlut_33 = (mfxCam3DLutEntry*) malloc (sizeof(mfxCam3DLutEntry)*MFX_CAM_3DLUT33_SIZE);
    m_3dlut_65 = (mfxCam3DLutEntry*) malloc (sizeof(mfxCam3DLutEntry)*MFX_CAM_3DLUT65_SIZE);

    m_hwdev = NULL;

    MSDK_ZERO_MEMORY(m_mfxVideoParams);
    MSDK_ZERO_MEMORY(m_mfxResponseIn);
    MSDK_ZERO_MEMORY(m_mfxResponseOut);
    MSDK_ZERO_MEMORY(m_mfxResponseAux);

    MSDK_ZERO_MEMORY(m_GammaCorrection);
    m_GammaCorrection.Header.BufferId = MFX_EXTBUF_CAM_GAMMA_CORRECTION;
    m_GammaCorrection.Header.BufferSz = sizeof(m_GammaCorrection);

    MSDK_ZERO_MEMORY(m_3DLUT_GammaCorrection);
    m_3DLUT_GammaCorrection.Header.BufferId = MFX_EXTBUF_CAM_FORWARD_GAMMA_CORRECTION;
    m_3DLUT_GammaCorrection.Header.BufferSz = sizeof(m_3DLUT_GammaCorrection);
#if MFX_VERSION >= 1023
    MSDK_ZERO_MEMORY(m_TotalColorControl);
    m_TotalColorControl.Header.BufferId = MFX_EXTBUF_CAM_TOTAL_COLOR_CONTROL;
    m_TotalColorControl.Header.BufferSz = sizeof(m_TotalColorControl);

    MSDK_ZERO_MEMORY(m_RGBToYUV);
    m_RGBToYUV.Header.BufferId = MFX_EXTBUF_CAM_CSC_YUV_RGB;
    m_RGBToYUV.Header.BufferSz = sizeof(m_RGBToYUV);
#endif
    MSDK_ZERO_MEMORY(m_BlackLevelCorrection);
    m_BlackLevelCorrection.Header.BufferId = MFX_EXTBUF_CAM_BLACK_LEVEL_CORRECTION;
    m_BlackLevelCorrection.Header.BufferSz = sizeof(m_BlackLevelCorrection);

    MSDK_ZERO_MEMORY(m_Denoise);
    m_Denoise.Header.BufferId = MFX_EXTBUFF_VPP_DENOISE;
    m_Denoise.Header.BufferSz = sizeof(m_Denoise);

    MSDK_ZERO_MEMORY(m_HP);
    m_HP.Header.BufferId = MFX_EXTBUF_CAM_HOT_PIXEL_REMOVAL;
    m_HP.Header.BufferSz = sizeof(m_HP);

    MSDK_ZERO_MEMORY(m_3DLUT);
    m_3DLUT.Header.BufferId = MFX_EXTBUF_CAM_3DLUT;
    m_3DLUT.Header.BufferSz = sizeof(m_3DLUT);

    MSDK_ZERO_MEMORY(m_WhiteBalance);
    m_WhiteBalance.Header.BufferId = MFX_EXTBUF_CAM_WHITE_BALANCE;
    m_WhiteBalance.Header.BufferSz = sizeof(m_WhiteBalance);

    MSDK_ZERO_MEMORY(m_CCM);
    m_CCM.Header.BufferId = MFX_EXTBUF_CAM_COLOR_CORRECTION_3X3;
    m_CCM.Header.BufferSz = sizeof(m_CCM);

    MSDK_ZERO_MEMORY(m_Vignette);
    m_Vignette.Header.BufferId = MFX_EXTBUF_CAM_VIGNETTE_CORRECTION;
    m_Vignette.Header.BufferSz = sizeof(m_Vignette);

    MSDK_ZERO_MEMORY(m_Lens);
    m_Lens.Header.BufferId = MFX_EXTBUF_CAM_LENS_GEOM_DIST_CORRECTION;
    m_Lens.Header.BufferSz = sizeof(m_Lens);
}

CCameraPipeline::~CCameraPipeline()
{
    Close();
}

bool CCameraPipeline::isBayerFormat(mfxU32 type)
{
    if (MFX_FOURCC_ABGR16 == type || MFX_FOURCC_ARGB16 == type)
        return false;

    return true;
}

mfxStatus CCameraPipeline::Init(sInputParams *pParams)
{
    MSDK_CHECK_POINTER(pParams, MFX_ERR_NULL_PTR);

    mfxStatus sts = MFX_ERR_NONE;

    if (pParams->bPerf_opt)
        m_pFileReader = new CBufferedVideoReader();
    else if ( isBayerFormat(pParams->inputType) )
        m_pFileReader = new CRawVideoReader();
    else
        m_pFileReader = new CARGB16VideoReader();

    m_nFrameLimit = pParams->nFramesToProceed;
    sts = m_pFileReader->Init(pParams);
    MSDK_CHECK_STATUS(sts, "m_pFileReader->Init failed");

    {
        if (pParams->frameInfo[VPP_IN].CropW == NOT_INIT_VALUE)
            pParams->frameInfo[VPP_IN].CropW = pParams->frameInfo[VPP_IN].nWidth;
        if (pParams->frameInfo[VPP_IN].CropH == NOT_INIT_VALUE)
            pParams->frameInfo[VPP_IN].CropH = pParams->frameInfo[VPP_IN].nHeight;

        pParams->frameInfo[VPP_IN].CropW    =  pParams->frameInfo[VPP_IN].CropW;
        pParams->frameInfo[VPP_IN].CropX    =  align(pParams->frameInfo[VPP_IN].CropX);

        pParams->frameInfo[VPP_OUT].nWidth  = pParams->frameInfo[VPP_IN].CropW;
        pParams->frameInfo[VPP_OUT].nHeight = pParams->frameInfo[VPP_IN].CropH;
        pParams->frameInfo[VPP_OUT].CropW   = pParams->frameInfo[VPP_OUT].nWidth;
        pParams->frameInfo[VPP_OUT].CropH   = pParams->frameInfo[VPP_OUT].nHeight;

        pParams->frameInfo[VPP_OUT].nWidth  = align_32(pParams->frameInfo[VPP_OUT].nWidth);
        pParams->frameInfo[VPP_IN].nWidth   = align_32(pParams->frameInfo[VPP_IN].nWidth);
        pParams->frameInfo[VPP_IN].nHeight  = align(pParams->frameInfo[VPP_IN].nHeight);
    }

    pParams->frameInfo[VPP_IN].FourCC = isBayerFormat(pParams->inputType) ? MFX_FOURCC_R16 : pParams->inputType;

    // set memory type

    m_memTypeIn  = SYSTEM_MEMORY;
    m_memTypeOut = SYSTEM_MEMORY;
    m_accelType  = pParams->accelType;
    if ( pParams->accelType == D3D9 && pParams->memTypeOut == VIDEO)
    {
            m_memTypeOut  = D3D9_MEMORY;
    }
    else if ( pParams->accelType == D3D11 && pParams->memTypeOut == VIDEO)
    {
        m_memTypeOut = D3D11_MEMORY;
    }

    if ( pParams->accelType == D3D9 && pParams->memTypeIn == VIDEO)
    {
            m_memTypeIn  = D3D9_MEMORY;
    }
    else if ( pParams->accelType == D3D11 && pParams->memTypeIn == VIDEO)
    {
        m_memTypeIn = D3D11_MEMORY;
    }

    // API version
    mfxVersion version =  {10, MFX_VERSION_MAJOR};

    // Init session
    {
        // try searching on all display adapters
        mfxIMPL impl = MFX_IMPL_HARDWARE_ANY;

        // if d3d11 surfaces are used ask the library to run acceleration through D3D11
        // feature may be unsupported due to OS or MSDK API version
        if (D3D11 == pParams->accelType)
            impl |= MFX_IMPL_VIA_D3D11;
        else if (D3D9 == pParams->accelType)
            impl |= MFX_IMPL_VIA_D3D9;

        sts = m_mfxSession.Init(impl, &version);

        // MSDK API version may not support multiple adapters - then try initialize on the default
        if (MFX_ERR_NONE != sts)
            sts = m_mfxSession.Init(impl & !MFX_IMPL_HARDWARE_ANY | MFX_IMPL_HARDWARE, &version);
    }

    MSDK_CHECK_STATUS(sts, "m_mfxSession.Init failed");

    // create VPP
    m_pmfxVPP = new MFXVideoVPP(m_mfxSession);
    MSDK_CHECK_POINTER(m_pmfxVPP, MFX_ERR_MEMORY_ALLOC);


    // Initialize rendering window
    if (pParams->bRendering)
    {
#if D3D_SURFACES_SUPPORT
        sWindowParams windowParams;

        windowParams.lpWindowName = pParams->bWallNoTitle ? NULL : MSDK_STRING("sample_camera");
        windowParams.nx           = pParams->nWallW;
        windowParams.ny           = pParams->nWallH;
        windowParams.nWidth       = CW_USEDEFAULT;
        windowParams.nHeight      = CW_USEDEFAULT;
        windowParams.ncell        = pParams->nWallCell;
        windowParams.nAdapter     = pParams->nWallMonitor;
        windowParams.nMaxFPS      = pParams->nWallFPS;

        windowParams.lpClassName  = MSDK_STRING("Render Window Class");
        windowParams.dwStyle      = WS_OVERLAPPEDWINDOW;
        windowParams.hWndParent   = NULL;
        windowParams.hMenu        = NULL;
        windowParams.hInstance    = GetModuleHandle(NULL);
        windowParams.lpParam      = NULL;
        windowParams.bFullScreen  = FALSE;

        m_d3dRender.Init(windowParams);

        SetRenderingFlag();
#endif
    }

    // create device and allocator. SetHandle must be called after session Init and before any other MSDK calls,
    // otherwise an own device will be created by MSDK

    sts = CreateAllocator();
    MSDK_CHECK_STATUS(sts, "CreateAllocator failed");

    //Load library plug-in
    {
        MSDK_MEMCPY(m_UID_Camera.Data, CAMERA_PIPE_UID, 16);
        sts = MFXVideoUSER_Load(m_mfxSession, &m_UID_Camera, pParams->CameraPluginVersion);
        MSDK_CHECK_STATUS(sts, "MFXVideoUSER_Load failed");
    }

    if (pParams->bGamma)
    {
        pParams->gamma_mode = MFX_CAM_GAMMA_LUT; // tmp ??? kta
        if (pParams->gamma_mode == MFX_CAM_GAMMA_LUT && ! pParams->bExternalGammaLUT)  {
            for (int i = 0; i < 64; i++) {
                pParams->gamma_point[i]  = gamma_point[i];
                pParams->gamma_corrected[i] = gamma_correct[i];
            }
        }
    }

    sts = InitMfxParams(pParams);
    MSDK_CHECK_STATUS(sts, "InitMfxParams failed");

    SetOutputfileFlag(pParams->bOutput);
    if (m_bOutput)
    {
        // prepare bmp file writer
        if (pParams->frameInfo[VPP_OUT].FourCC == MFX_FOURCC_ARGB16 ||
            pParams->frameInfo[VPP_OUT].FourCC == MFX_FOURCC_ABGR16 ||
            pParams->frameInfo[VPP_OUT].FourCC == MFX_FOURCC_NV12)
        {
            m_pRawFileWriter = new CRawVideoWriter;
            sts = m_pRawFileWriter->Init(pParams);
        }
        else if (pParams->frameInfo[VPP_OUT].FourCC == MFX_FOURCC_RGB4)
        {
            m_pBmpWriter = new CBmpWriter;
            sts = m_pBmpWriter->Init(pParams->strDstFile, m_mfxVideoParams.vpp.Out.CropW, m_mfxVideoParams.vpp.Out.CropH, pParams->maxNumBmpFiles);
        }
        MSDK_CHECK_STATUS(sts, "<Out format>Writer->Init failed");
    }

    sts = m_pmfxVPP->Query(&m_mfxVideoParams, &m_mfxVideoParams);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    MSDK_CHECK_STATUS(sts, "m_pmfxVPP->Query failed");

    sts = m_pmfxVPP->Init(&m_mfxVideoParams);
    if (MFX_WRN_PARTIAL_ACCELERATION == sts)
    {
        msdk_printf(MSDK_STRING("WARNING: partial acceleration\n"));
        MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    }
    MSDK_CHECK_STATUS(sts, "m_pmfxVPP->Init failed");

    // ??? need this ?
    sts = m_pmfxVPP->GetVideoParam(&m_mfxVideoParams);
    MSDK_CHECK_STATUS(sts, "m_pmfxVPP->GetVideoParam failed");

    m_alphaValue = pParams->alphaValue;
    m_BayerType = pParams->inputType;

    m_numberOfResets = (mfxU32)pParams->resetParams.size();
    m_resetInterval = pParams->resetInterval;

    sts = AllocFrames();
    MSDK_CHECK_STATUS(sts, "AllocFrames failed");

    return MFX_ERR_NONE;
}

// function for allocating a specific external buffer
template <typename Buffer>
mfxStatus CCameraPipeline::AllocateExtBuffer()
{
    std::unique_ptr<Buffer> pExtBuffer (new Buffer());

    init_ext_buffer(*pExtBuffer);

    m_ExtBuffers.push_back(reinterpret_cast<mfxExtBuffer*>(pExtBuffer.get()));
    pExtBuffer.release();

    return MFX_ERR_NONE;
}

void CCameraPipeline::AttachExtParam()
{
    m_mfxVideoParams.ExtParam = reinterpret_cast<mfxExtBuffer**>(&m_ExtBuffers[0]);
    m_mfxVideoParams.NumExtParam = static_cast<mfxU16>(m_ExtBuffers.size());
}

void CCameraPipeline::DeleteExtBuffers()
{
    for (std::vector<mfxExtBuffer *>::iterator it = m_ExtBuffers.begin(); it != m_ExtBuffers.end(); ++it)
        delete *it;
    m_ExtBuffers.clear();
}

mfxStatus CCameraPipeline::AllocAndInitCamBlackLevelCorrection(sInputParams *pParams)
{
    m_BlackLevelCorrection.B  = pParams->black_level_B;
    m_BlackLevelCorrection.G0 = pParams->black_level_G0;
    m_BlackLevelCorrection.G1 = pParams->black_level_G1;
    m_BlackLevelCorrection.R  = pParams->black_level_R;

    return MFX_ERR_NONE;
}

#if MFX_VERSION >= 1023
mfxStatus CCameraPipeline::AllocAndInitCamRGBtoYUV(sInputParams *pParams)
{
    for (int i = 0; i < 3; i++)
        m_RGBToYUV.PreOffset[i] = pParams->pre[i];
    for (int i = 0; i < 3; i++)
        m_RGBToYUV.PostOffset[i] = pParams->post[i];
    //Coefs of CSC (RGB to YUV) are hardcoded but they can be handled by app through command line as pre and post offsets if it will be needed
    m_RGBToYUV.Matrix[0][0] = 0.299f;
    m_RGBToYUV.Matrix[0][1] = 0.587f;
    m_RGBToYUV.Matrix[0][2] = 0.114f;
    m_RGBToYUV.Matrix[1][0] = -0.147f;
    m_RGBToYUV.Matrix[1][1] = -0.289f;
    m_RGBToYUV.Matrix[1][2] = 0.436f;
    m_RGBToYUV.Matrix[2][0] = 0.615f;
    m_RGBToYUV.Matrix[2][1] = -0.515f;
    m_RGBToYUV.Matrix[2][2] = -0.100f;

    return MFX_ERR_NONE;
}

mfxStatus CCameraPipeline::AllocAndInitCamTotalColorControl(sInputParams *pParams)
{
    m_TotalColorControl.R = pParams->tcc_red;
    m_TotalColorControl.G = pParams->tcc_green;
    m_TotalColorControl.B = pParams->tcc_blue;
    m_TotalColorControl.C = pParams->tcc_cyan;
    m_TotalColorControl.M = pParams->tcc_magenta;
    m_TotalColorControl.Y = pParams->tcc_yellow;

    return MFX_ERR_NONE;
}
#endif

mfxStatus CCameraPipeline::AllocAndInitDenoise(sInputParams *pParams)
{
    m_Denoise.Threshold = pParams->denoiseThreshold;
    return MFX_ERR_NONE;
}

mfxStatus CCameraPipeline::AllocAndInitCamHotPixelRemoval(sInputParams *pParams)
{
    m_HP.PixelCountThreshold       = pParams->hp_num;
    m_HP.PixelThresholdDifference  = pParams->hp_diff;

    return MFX_ERR_NONE;
}

mfxStatus CCameraPipeline::AllocAndInitCamWhiteBalance(sInputParams *pParams)
{
    m_WhiteBalance.Mode = MFX_CAM_WHITE_BALANCE_MANUAL;
    m_WhiteBalance.B    = pParams->white_balance_B;
    m_WhiteBalance.G0   = pParams->white_balance_G0;
    m_WhiteBalance.G1   = pParams->white_balance_G1;
    m_WhiteBalance.R    = pParams->white_balance_R;

    return MFX_ERR_NONE;
}

mfxStatus CCameraPipeline::AllocAndInitCamLens(sInputParams *pParams)
{
    m_Lens.a[0] = pParams->lens_aR;
    m_Lens.a[1] = pParams->lens_aG;
    m_Lens.a[2] = pParams->lens_aB;
    m_Lens.b[0] = pParams->lens_bR;
    m_Lens.b[1] = pParams->lens_bG;
    m_Lens.b[2] = pParams->lens_bB;
    m_Lens.c[0] = pParams->lens_cR;
    m_Lens.c[1] = pParams->lens_cG;
    m_Lens.c[2] = pParams->lens_cB;
    m_Lens.d[0] = pParams->lens_dR;
    m_Lens.d[1] = pParams->lens_dG;
    m_Lens.d[2] = pParams->lens_dB;
    return MFX_ERR_NONE;
}

mfxStatus CCameraPipeline::AllocAndInitCamCCM(sInputParams *pParams)
{
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
           m_CCM.CCM[i][j] = pParams->CCM[i][j];

    return MFX_ERR_NONE;
}

mfxStatus CCameraPipeline::AllocAndInitVignetteCorrection(sInputParams *pParams)
{
    FILE *maskFile;
    mfxU32 i, nBytesRead;

    /* Vignette correction mask must be ( 1/4 height + 1 ) (1/4 width + 1) dimension */
    /* If height is not divisible by 4, add extra +1 to result of division - that's equivalent to ceil() */
    m_Vignette.Height = pParams->frameInfo->nHeight/4 + (pParams->frameInfo->nHeight%4 ? 2 : 1);
    m_Vignette.Width  = align_64((pParams->frameInfo->nWidth/4 + (pParams->frameInfo->nWidth%4 ? 2 : 1) )
        * 4 /* for each channel */ * 2 /* 2 bytes each */);

    /* Vignette correction mask data must have 64-aligned pitch */
    m_Vignette.Pitch  = align_64(m_Vignette.Width);

    m_Vignette.CorrectionMap = (mfxCamVignetteCorrectionParam *)malloc(m_Vignette.Pitch * m_Vignette.Height);
    memset(m_Vignette.CorrectionMap, 0, m_Vignette.Pitch * m_Vignette.Height);

    /* Load vignette mask from file */
    MSDK_FOPEN(maskFile, pParams->strVignetteMaskFile, MSDK_STRING("rb"));
    MSDK_CHECK_POINTER(maskFile, MFX_ERR_ABORTED);
    for (i = 0; i < m_Vignette.Height; i++)
    {
#if defined(_WIN32) || defined(_WIN64)
        nBytesRead = (mfxU32)fread_s((mfxU8 *)m_Vignette.CorrectionMap + i * m_Vignette.Pitch, m_Vignette.Pitch, 1, m_Vignette.Width, maskFile);
#else
        nBytesRead = (mfxU32)fread((mfxU8 *)m_Vignette.CorrectionMap + i * m_Vignette.Pitch, 1, m_Vignette.Width, maskFile);
#endif
         IOSTREAM_CHECK_NOT_EQUAL_SAFE(nBytesRead, m_Vignette.Width, MFX_ERR_MORE_DATA,fclose(maskFile));
    }
    fclose(maskFile);
    return MFX_ERR_NONE;
}

void  CCameraPipeline::FreeVignetteCorrection()
{
    MSDK_SAFE_DELETE(m_Vignette.CorrectionMap);
}

mfxStatus CCameraPipeline::AllocAndInitCamGammaCorrection(sInputParams *pParams)
{
    if ( pParams->b3DLUTGamma )
    {
        m_3DLUT_GammaCorrection.NumSegments = 64;
        m_3DLUT_GammaCorrection.Segment = new mfxCamFwdGammaSegment[64];
        MSDK_CHECK_POINTER(m_3DLUT_GammaCorrection.Segment, MFX_ERR_ABORTED);
        for ( int i = 0; i < 64; i++)
        {
            // Use the same curve for R/G/B
            m_3DLUT_GammaCorrection.Segment[i].Pixel = pParams->gamma_point[i];
            m_3DLUT_GammaCorrection.Segment[i].Red   = pParams->gamma_corrected[i];
            m_3DLUT_GammaCorrection.Segment[i].Green = pParams->gamma_corrected[i];
            m_3DLUT_GammaCorrection.Segment[i].Blue  = pParams->gamma_corrected[i];
        }
    }
    else
    {
        m_GammaCorrection.Mode = pParams->gamma_mode;
        if (m_GammaCorrection.Mode == MFX_CAM_GAMMA_LUT)
        {
            m_GammaCorrection.NumPoints = 64;
            MSDK_MEMCPY(m_GammaCorrection.GammaPoint, pParams->gamma_point, m_GammaCorrection.NumPoints*sizeof(mfxU16));
            MSDK_MEMCPY(m_GammaCorrection.GammaCorrected, pParams->gamma_corrected, m_GammaCorrection.NumPoints*sizeof(mfxU16));
        }
        else if (m_GammaCorrection.Mode == MFX_CAM_GAMMA_VALUE)
        {
            m_GammaCorrection.GammaValue = pParams->gamma_value;
        }
    }

    return MFX_ERR_NONE;
}

void  CCameraPipeline::FreeCamGammaCorrection()
{
    if (b_3DLUT_Gamma)
    {
        if ( m_3DLUT_GammaCorrection.Segment )
            delete [] m_3DLUT_GammaCorrection.Segment;
    }
}

void CCameraPipeline::Close()
{
    if (m_pmfxVPP)
        m_pmfxVPP->Close();
    MSDK_SAFE_DELETE(m_pmfxVPP);

    DeleteFrames();

    FreeVignetteCorrection();
    FreeCamGammaCorrection();
    m_ExtBuffers.clear();

    m_pCamera_plugin.reset();
    MFXVideoUSER_UnLoad(m_mfxSession, &m_UID_Camera);
    m_mfxSession.Close();

    if (m_pFileReader) {
        m_pFileReader->Close();
        MSDK_SAFE_DELETE(m_pFileReader);
    }

    if (m_pBmpWriter) {
        MSDK_SAFE_DELETE(m_pBmpWriter);
    }

    if (m_pRawFileWriter) {
        MSDK_SAFE_DELETE(m_pRawFileWriter);
    }

#if D3D_SURFACES_SUPPORT
   if (m_bIsRender)
       m_d3dRender.Close();
#endif

    MSDK_SAFE_FREE(m_3dlut_17);
    MSDK_SAFE_FREE(m_3dlut_33);
    MSDK_SAFE_FREE(m_3dlut_65);

    // allocator if used as external for MediaSDK must be deleted after decoder
    DeleteAllocator();



    return;
}

mfxStatus CCameraPipeline::Reset(sInputParams *pParams)
{
    mfxStatus sts = MFX_ERR_NONE;

    mfxVideoParam oldMfxParams = m_mfxVideoParams;

    m_alphaValue = pParams->alphaValue;
    m_BayerType = pParams->inputType;


//    m_memTypeIn = pParams->memTypeIn;
//    m_memTypeOut = pParams->memTypeOut;
//
//    if (m_memTypeOut == UNDEFINED_MEMORY) {
//        if (pParams->bRendering) {
//#if MFX_D3D11_SUPPORT
//            m_memTypeOut  = D3D11_MEMORY;
//#else
//            m_memTypeOut  = D3D9_MEMORY;
//#endif
//        } else
//            m_memTypeOut  = SYSTEM_MEMORY;
//    }
//
//    m_memType = (MemType)((m_memTypeIn | m_memTypeOut) & (UNDEFINED_MEMORY - 1));
//    if (m_memType & D3D11_MEMORY)
//        m_memType = D3D11_MEMORY;


    MSDK_ZERO_MEMORY(m_mfxVideoParams);
    m_ExtBuffers.clear();

    sts = m_pFileReader->Init(pParams);
    MSDK_CHECK_STATUS(sts, "m_pFileReader->Init failed");

    sts = InitMfxParams(pParams);
    MSDK_CHECK_STATUS(sts, "InitMfxParams failed");

    sts =  m_pmfxVPP->Reset(&m_mfxVideoParams);
    MSDK_CHECK_STATUS(sts, "m_pmfxVPP->Reset failed");

    sts = ReallocFrames(&oldMfxParams);
    MSDK_CHECK_STATUS(sts, "ReallocFrames failed");

    if (m_bOutput)
    {
        // prepare bmp file writer
        if (pParams->frameInfo[VPP_OUT].FourCC == MFX_FOURCC_ARGB16 ||
            pParams->frameInfo[VPP_OUT].FourCC == MFX_FOURCC_ABGR16 ||
            pParams->frameInfo[VPP_OUT].FourCC == MFX_FOURCC_NV12) {
            sts = m_pRawFileWriter->Init(pParams);
        } else {
            sts = m_pBmpWriter->Init(pParams->strDstFile, m_mfxVideoParams.vpp.Out.CropW, m_mfxVideoParams.vpp.Out.CropH, pParams->maxNumBmpFiles);
        }
        MSDK_CHECK_STATUS(sts, "<Out format>Writer->Init failed");
    }

    return MFX_ERR_NONE;
}

mfxStatus CCameraPipeline::PrepareInputSurfaces()
{
    mfxStatus           sts = MFX_ERR_NONE;
    mfxU32 asyncDepth = std::min({m_mfxVideoParams.AsyncDepth, m_mfxResponseIn.NumFrameActual, m_mfxResponseOut.NumFrameActual});

    mfxU32 frnum = m_nInputFileIndex;

    m_pFileReader->SetStartFileNumber((mfxI32)frnum);

    for (;;) {
        mfxFrameSurface1 *pSurface = 0;
        for (mfxU32 i = 0; i < asyncDepth; i++) {

            if (m_nFrameLimit > 0 && frnum >= m_nFrameLimit)
                return MFX_ERR_ABORTED;

            for (;;) {
                if (m_bEnd)
                    return MFX_ERR_ABORTED;
                pSurface = &m_pmfxSurfacesIn[i];
                if (pSurface->Data.Locked == 0)
                    break;
                pSurface = &m_pmfxSurfacesIn[i + asyncDepth];
                if (pSurface->Data.Locked == 0)
                    break;
                MSDK_SLEEP(1);
            }

            if (pSurface->Data.MemId)
            {
#ifdef CAMP_PIPE_ITT
    __itt_task_begin(CamPipeAccel, __itt_null, __itt_null, task4);
#endif
#if D3D_SURFACES_SUPPORT
                if (m_memTypeIn == D3D11_MEMORY)
                    sts = m_pMFXAllocatorIn->Lock(m_pMFXAllocatorIn->pthis, MFXReadWriteMid(pSurface->Data.MemId, MFXReadWriteMid::write), &pSurface->Data);
                else
#endif
                    sts = m_pMFXAllocatorIn->Lock(m_pMFXAllocatorIn->pthis, pSurface->Data.MemId, &pSurface->Data);
                if (MFX_ERR_NONE != sts)
                    break;
                sts = m_pFileReader->LoadNextFrame(&pSurface->Data, &m_mfxVideoParams.vpp.In, m_BayerType);
                if (MFX_ERR_NONE != sts)
                    break;
#if D3D_SURFACES_SUPPORT
                if (m_memTypeIn == D3D11_MEMORY)
                    sts = m_pMFXAllocatorIn->Unlock(m_pMFXAllocatorIn->pthis, MFXReadWriteMid(pSurface->Data.MemId, MFXReadWriteMid::write), &pSurface->Data);
                else
#endif
                    sts = m_pMFXAllocatorIn->Unlock(m_pMFXAllocatorIn->pthis, pSurface->Data.MemId, &pSurface->Data);
                if (MFX_ERR_NONE != sts)
                    break;
#ifdef CAMP_PIPE_ITT
    __itt_task_end(CamPipeAccel);
#endif

            }
            else
            {

#ifdef CAMP_PIPE_ITT
                __itt_task_begin(CamPipeAccel, __itt_null, __itt_null, task4);
#endif

                sts = m_pFileReader->LoadNextFrame(&pSurface->Data, &m_mfxVideoParams.vpp.In, m_BayerType);

#ifdef CAMP_PIPE_ITT
                __itt_task_end(CamPipeAccel);
#endif

                if (MFX_ERR_NONE != sts)
                    break;

                camera_printf("to surface %p \n", pSurface->Data.Y16);
                camera_fflush(stdout);
            }

            if (m_bEnd)
                return MFX_ERR_ABORTED;

            msdk_atomic_inc16((volatile mfxU16*)&(pSurface->Data.Locked));

            frnum++;
        }
        if (sts != MFX_ERR_NONE) {
            //if (pSurface)
            //    msdk_atomic_dec16((volatile mfxU16*)&(pSurface->Data.Locked));
            return sts;
        }
    }
}


mfxU32 MFX_STDCALL InputThreadRoutine(void* par)
{
    CCameraPipeline *pPipe = (CCameraPipeline *)par;

    pPipe->PrepareInputSurfaces();

    return 0;

}

mfxStatus CCameraPipeline::Run()
{
    mfxStatus           sts = MFX_ERR_NONE;
    m_bEnd = false;
    bool quitOnFrameLimit = false;

    mfxU32 asyncDepth = std::min({m_mfxVideoParams.AsyncDepth, m_mfxResponseIn.NumFrameActual, m_mfxResponseOut.NumFrameActual});

    mfxSyncPoint       *syncpoints = new mfxSyncPoint[asyncDepth];
    mfxFrameSurface1   **ppOutSurf = new mfxFrameSurface1*[asyncDepth];
    mfxFrameSurface1   **ppInSurf = new mfxFrameSurface1*[asyncDepth];

    MSDKThread * pthread = new MSDKThread(sts, InputThreadRoutine, (void*)this);

    mfxI32 asdepth;
    mfxI32 tail_start = 0, tail_len = (mfxI32)asyncDepth - 1;

    mfxU8 *syncFlags = new mfxU8[asyncDepth];
    memset(syncFlags, 0, asyncDepth);

#ifdef CAMP_PIPE_ITT
    __itt_task_begin(CamPipeAccel, __itt_null, __itt_null, task5);
#endif

    for (asdepth = 0; asdepth < (mfxI32)asyncDepth; asdepth++)
    {
        mfxFrameSurface1 *pInSurf, *pOutSurf;

        pInSurf = &m_pmfxSurfacesIn[asdepth];
        while (pInSurf->Data.Locked != 1) {
            MSDK_SLEEP(1);
            if (pthread->GetExitCode() != MFX_TASK_WORKING) {
                if (pInSurf->Data.Locked != 1) {
                    tail_len = asdepth;
                    sts = MFX_ERR_MORE_DATA;
                    break;
                }
            }
        }
        MSDK_BREAK_ON_ERROR(sts);

        ppInSurf[asdepth] = pInSurf;

        //mfxU16 idx = GetFreeSurface(m_pmfxSurfacesOut, m_mfxResponseOut.NumFrameActual);
        //pOutSurf = &m_pmfxSurfacesOut[idx];
        //pOutSurf->Data.Locked++;

        pOutSurf = &m_pmfxSurfacesOut[asdepth];
        //sts = GetFreeSurface(m_pmfxSurfacesOut, m_mfxResponseOut.NumFrameActual, &pOutSurf);
        MSDK_BREAK_ON_ERROR(sts);

        ppOutSurf[asdepth] = pOutSurf;

#ifdef CAMP_PIPE_ITT
    __itt_task_begin(CamPipeAccel, __itt_null, __itt_null, task1);
#endif

        pOutSurf->Data.FrameOrder = pInSurf->Data.FrameOrder;

        sts = m_pmfxVPP->RunFrameVPPAsync(pInSurf, pOutSurf,
                                                    NULL,
                                                    &syncpoints[asdepth]);
        syncFlags[asdepth] = 1;
        camera_printf("vpp_async %d in %p out %p  %d  \n", asdepth, pInSurf->Data.Y16, pOutSurf->Data.B, pInSurf->Data.FrameOrder);
        camera_fflush(stdout);


#ifdef CAMP_PIPE_ITT
    __itt_task_end(CamPipeAccel);
#endif

        MSDK_BREAK_ON_ERROR(sts);

    }

#ifdef CAMP_PIPE_ITT
    __itt_task_end(CamPipeAccel);
#endif


    while (MFX_ERR_NONE <= sts)
    {
        if (MFX_WRN_DEVICE_BUSY == sts)
        {
            MSDK_SLEEP(1); // just wait and then repeat the same call to VPPFrameAsync (??? kta)
        }

        mfxFrameSurface1 *pInSurf, *pOutSurf;

        for (asdepth = 0; asdepth < (mfxI32)asyncDepth; asdepth++)
        {
            camera_printf("sync --- %d %p %d %d \n", asdepth, ppInSurf[asdepth]->Data.Y16, ppInSurf[asdepth]->Data.Locked, ppInSurf[asdepth]->Data.FrameOrder);

            sts = m_mfxSession.SyncOperation(syncpoints[asdepth], MSDK_VPP_WAIT_INTERVAL);

            MSDK_BREAK_ON_ERROR(sts);

            syncFlags[asdepth] = 0;

            ReleaseSurface(ppInSurf[asdepth]);

            camera_printf("-------- %p %d %d \n", ppInSurf[asdepth]->Data.Y16, ppInSurf[asdepth]->Data.Locked, ppInSurf[asdepth]->Data.FrameOrder); camera_fflush(stdout);

            if (m_bIsRender)
            {
#ifdef CAMP_PIPE_ITT
    __itt_task_begin(CamPipeAccel, __itt_null, __itt_null, task3);
#endif

#if D3D_SURFACES_SUPPORT

                if (m_memTypeOut == SYSTEM_MEMORY) {
                    // slow tmp solution !!! kta
#if MFX_D3D11_SUPPORT
                    if (m_accelType == D3D9)
                        sts = m_pMFXd3dAllocator->Lock(m_pMFXd3dAllocator->pthis, m_pmfxSurfacesAux->Data.MemId, &m_pmfxSurfacesAux->Data);
                    else
                        sts = m_pMFXd3dAllocator->Lock(m_pMFXd3dAllocator->pthis, MFXReadWriteMid(m_pmfxSurfacesAux->Data.MemId, MFXReadWriteMid::write), &m_pmfxSurfacesAux->Data);
                    MSDK_BREAK_ON_ERROR(sts);
                    if (ppOutSurf[asdepth]->Info.CropW * 4 == ppOutSurf[asdepth]->Data.Pitch && ppOutSurf[asdepth]->Data.Pitch  == m_pmfxSurfacesAux->Data.Pitch)
                        memcpy_s(m_pmfxSurfacesAux->Data.B, m_pmfxSurfacesAux->Data.Pitch*m_pmfxSurfacesAux->Info.Height, ppOutSurf[asdepth]->Data.B, ppOutSurf[asdepth]->Info.CropW*ppOutSurf[asdepth]->Info.CropH*4);
                    else {
                        for (int i = 0; i < ppOutSurf[asdepth]->Info.CropH; i++)
                            memcpy_s(m_pmfxSurfacesAux->Data.B + i * m_pmfxSurfacesAux->Data.Pitch, m_pmfxSurfacesAux->Data.Pitch, ppOutSurf[asdepth]->Data.B + i*ppOutSurf[asdepth]->Data.Pitch, ppOutSurf[asdepth]->Info.CropW*4);
                    }
                    if (m_accelType == D3D9)
                        sts = m_pMFXd3dAllocator->Unlock(m_pMFXd3dAllocator->pthis, m_pmfxSurfacesAux->Data.MemId, &m_pmfxSurfacesAux->Data);
                    else
                        sts = m_pMFXd3dAllocator->Unlock(m_pMFXd3dAllocator->pthis, MFXReadWriteMid(m_pmfxSurfacesAux->Data.MemId, MFXReadWriteMid::write), &m_pmfxSurfacesAux->Data);
#else
                    sts = m_pMFXd3dAllocator->Lock(m_pMFXd3dAllocator->pthis, m_pmfxSurfacesAux->Data.MemId, &m_pmfxSurfacesAux->Data);
                    MSDK_BREAK_ON_ERROR(sts);
                    if (ppOutSurf[asdepth]->Info.CropW * 4 == ppOutSurf[asdepth]->Data.Pitch && ppOutSurf[asdepth]->Data.Pitch  == m_pmfxSurfacesAux->Data.Pitch)
                        memcpy_s(m_pmfxSurfacesAux->Data.B, m_pmfxSurfacesAux->Data.Pitch*m_pmfxSurfacesAux->Info.Height, ppOutSurf[asdepth]->Data.B, ppOutSurf[asdepth]->Info.CropW*ppOutSurf[asdepth]->Info.CropH*4);
                    else {
                        for (int i = 0; i < ppOutSurf[asdepth]->Info.CropH; i++)
                            memcpy_s(m_pmfxSurfacesAux->Data.B + i * m_pmfxSurfacesAux->Data.Pitch, m_pmfxSurfacesAux->Data.Pitch, ppOutSurf[asdepth]->Data.B + i*ppOutSurf[asdepth]->Data.Pitch, ppOutSurf[asdepth]->Info.CropW*4);
                    }
                    sts = m_pMFXd3dAllocator->Unlock(m_pMFXd3dAllocator->pthis, m_pmfxSurfacesAux->Data.MemId, &m_pmfxSurfacesAux->Data);
#endif
                    MSDK_BREAK_ON_ERROR(sts);
                    sts = m_d3dRender.RenderFrame(m_pmfxSurfacesAux, m_pMFXd3dAllocator);

                } else
                    sts = m_d3dRender.RenderFrame(ppOutSurf[asdepth], m_pMFXd3dAllocator);
#endif

#ifdef CAMP_PIPE_ITT
    __itt_task_end(CamPipeAccel);
#endif

                if (sts == MFX_ERR_NULL_PTR)
                    sts = MFX_ERR_NONE;
                MSDK_BREAK_ON_ERROR(sts);
            }

            if (m_bOutput) {

                if (ppOutSurf[asdepth]->Data.MemId) {
#if D3D_SURFACES_SUPPORT
                    if (m_memTypeOut == D3D11_MEMORY)
                        sts = m_pMFXAllocatorOut->Lock(m_pMFXAllocatorOut->pthis, MFXReadWriteMid(ppOutSurf[asdepth]->Data.MemId, MFXReadWriteMid::read), &ppOutSurf[asdepth]->Data);
                    else
#endif
                        sts = m_pMFXAllocatorOut->Lock(m_pMFXAllocatorOut->pthis, ppOutSurf[asdepth]->Data.MemId, &ppOutSurf[asdepth]->Data);
                    MSDK_BREAK_ON_ERROR(sts);
                }

                if (m_alphaValue >= 0) {
                    if (ppOutSurf[asdepth]->Info.FourCC == MFX_FOURCC_ARGB16) {
                        int stride = ppOutSurf[asdepth]->Data.Pitch >> 1;
                        for (int i = 0; i < ppOutSurf[asdepth]->Info.Height; i++) {
                            for (int j = 0; j < ppOutSurf[asdepth]->Info.Width; j++) {
                                *(ppOutSurf[asdepth]->Data.V16 + 3 + j*4 + i*stride) = (mfxU16)m_alphaValue;
                            }
                        }
                    }
                    else if (ppOutSurf[asdepth]->Info.FourCC == MFX_FOURCC_ABGR16) {
                        int stride = ppOutSurf[asdepth]->Data.Pitch >> 1;
                        for (int i = 0; i < ppOutSurf[asdepth]->Info.Height; i++) {
                            for (int j = 0; j < ppOutSurf[asdepth]->Info.Width; j++) {
                                *(ppOutSurf[asdepth]->Data.Y16 + 3 + j * 4 + i*stride) = (mfxU16)m_alphaValue;
                            }
                        }
                    }
                    else if (ppOutSurf[asdepth]->Info.FourCC == MFX_FOURCC_RGB4) {
                        int stride = ppOutSurf[asdepth]->Data.Pitch;
                        for (int i = 0; i < ppOutSurf[asdepth]->Info.Height; i++) {
                            for (int j = 0; j < ppOutSurf[asdepth]->Info.Width; j++) {
                                *(ppOutSurf[asdepth]->Data.A + j*4 + i*stride) = (mfxU8)m_alphaValue;
                            }
                        }
                    }
                }
#ifdef CAMP_PIPE_ITT
    __itt_task_begin(CamPipeAccel, __itt_null, __itt_null, task2);
#endif

                camera_printf("writing frame %d \n", ppOutSurf[asdepth]->Data.FrameOrder);

                if (m_pRawFileWriter) {
                    if (ppOutSurf[asdepth]->Info.FourCC == MFX_FOURCC_ARGB16 ||
                        ppOutSurf[asdepth]->Info.FourCC == MFX_FOURCC_ABGR16)
                        m_pRawFileWriter->WriteFrameARGB16(&ppOutSurf[asdepth]->Data, 0, &ppOutSurf[asdepth]->Info);
                    else if (ppOutSurf[asdepth]->Info.FourCC == MFX_FOURCC_NV12)
                        m_pRawFileWriter->WriteFrameNV12(&ppOutSurf[asdepth]->Data, 0, &ppOutSurf[asdepth]->Info);
                } else if (m_pBmpWriter)
                    m_pBmpWriter->WriteFrame(&ppOutSurf[asdepth]->Data, 0, &ppOutSurf[asdepth]->Info);

#ifdef CAMP_PIPE_ITT
    __itt_task_end(CamPipeAccel);
#endif

                if (ppOutSurf[asdepth]->Data.MemId) {
#if D3D_SURFACES_SUPPORT
                    if (m_memTypeOut == D3D11_MEMORY)
                        sts = m_pMFXAllocatorOut->Unlock(m_pMFXAllocatorOut->pthis,  MFXReadWriteMid(ppOutSurf[asdepth]->Data.MemId, MFXReadWriteMid::read), &ppOutSurf[asdepth]->Data);
                    else
#endif
                        sts = m_pMFXAllocatorOut->Unlock(m_pMFXAllocatorOut->pthis,  ppOutSurf[asdepth]->Data.MemId, &ppOutSurf[asdepth]->Data);
                    MSDK_BREAK_ON_ERROR(sts);
                }
            }

            //ReleaseSurface(pInSurf);
            ReleaseSurface(ppOutSurf[asdepth]);

            ++m_nFrameIndex;
            if (m_resetInterval > 0 && ((m_nInputFileIndex == m_resetInterval - 1)) && m_resetCnt < m_numberOfResets) {
                m_nInputFileIndex = 0;
                quitOnFrameLimit = true;
                m_resetCnt++;
                tail_start = asdepth + 1;
                sts = MFX_WRN_VIDEO_PARAM_CHANGED;
                break;
            }
            ++m_nInputFileIndex;
            {
                //progress
                msdk_printf(MSDK_STRING("Frame number: %d\r"), m_nFrameIndex);
            }
            if (m_nFrameLimit > 0 && m_nFrameIndex >= m_nFrameLimit)
            {
                //m_bEnd = true;
                quitOnFrameLimit = true;
                sts = MFX_ERR_MORE_DATA;
                break;
            }

//            sts = PrepareInSurface(&pInSurf, asdepth);
//            MSDK_BREAK_ON_ERROR(sts);

            pInSurf = &m_pmfxSurfacesIn[asdepth];
            mfxFrameSurface1 *pInSurf1 = &m_pmfxSurfacesIn[asdepth + asyncDepth];

            for (;;) {
                camera_printf("%p %p  %d %d  %d %d \n", pInSurf->Data.Y16, pInSurf1->Data.Y16, pInSurf->Data.Locked, pInSurf1->Data.Locked, pInSurf->Data.FrameOrder, pInSurf1->Data.FrameOrder);
                camera_fflush(stdout);

                if (pInSurf->Data.Locked == 1 || pInSurf1->Data.Locked == 1) {
                    if ((pInSurf->Data.Locked != 1) || ((pInSurf1->Data.Locked == 1) && (pInSurf1->Data.FrameOrder < pInSurf->Data.FrameOrder)))
                        pInSurf = pInSurf1;
                    break;
                } else if (pthread->GetExitCode() != MFX_TASK_WORKING) {
                    tail_start = asdepth + 1;
                    sts = MFX_ERR_MORE_DATA;
                    break;
                }
                MSDK_SLEEP(1);
            }
            MSDK_BREAK_ON_ERROR(sts);

            ppInSurf[asdepth] = pInSurf;

            pOutSurf = &m_pmfxSurfacesOut[asdepth];
            //sts = GetFreeSurface(m_pmfxSurfacesOut, m_mfxResponseOut.NumFrameActual, &pOutSurf);
            //MSDK_BREAK_ON_ERROR(sts);

            ppOutSurf[asdepth] = pOutSurf;

#ifdef CAMP_PIPE_ITT
    __itt_task_begin(CamPipeAccel, __itt_null, __itt_null, task1);
#endif

            pOutSurf->Data.FrameOrder = pInSurf->Data.FrameOrder;

            sts = m_pmfxVPP->RunFrameVPPAsync(pInSurf, pOutSurf,
                                                        NULL,
                                                        &syncpoints[asdepth]);
            syncFlags[asdepth] = 1;


#ifdef CAMP_PIPE_ITT
    __itt_task_end(CamPipeAccel);
#endif

            MSDK_BREAK_ON_ERROR(sts);

            camera_printf("vpp_async %d in %p out %p  %d  \n", asdepth, pInSurf->Data.Y16, pOutSurf->Data.B, pInSurf->Data.FrameOrder);
            camera_fflush(stdout);
        }
        MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_SURFACE);
        MSDK_BREAK_ON_ERROR(sts);
    }
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);

    camera_printf("tail_start %d  tail_len %d \n", tail_start, tail_len);

    if (sts == MFX_ERR_NONE || sts == MFX_WRN_VIDEO_PARAM_CHANGED) {
        mfxI32 tail_asdepth, ii;
        mfxI32 frameIdx = m_nFrameIndex;

        mfxStatus local_sts = MFX_ERR_NONE;

        tail_asdepth = tail_start;
        for (ii = 0; ii < tail_len; ii++, tail_asdepth++)
        {
            if (tail_asdepth == (mfxI32)asyncDepth)
                tail_asdepth = 0;

            if (syncFlags[tail_asdepth] == 0)
                continue;

            camera_printf("sync tail --- %d %p %d %d \n", tail_asdepth, ppInSurf[tail_asdepth]->Data.Y16, ppInSurf[tail_asdepth]->Data.Locked, ppInSurf[tail_asdepth]->Data.FrameOrder);

            local_sts = m_mfxSession.SyncOperation(syncpoints[tail_asdepth], MSDK_VPP_WAIT_INTERVAL);
            MSDK_BREAK_ON_ERROR(local_sts);
            syncFlags[tail_asdepth] = 0;

            if (m_bIsRender && !quitOnFrameLimit)
            {
#if D3D_SURFACES_SUPPORT

                if (m_memTypeOut == SYSTEM_MEMORY) {
#if MFX_D3D11_SUPPORT
                    if (m_accelType == D3D9)
                        local_sts = m_pMFXd3dAllocator->Lock(m_pMFXd3dAllocator->pthis, m_pmfxSurfacesAux->Data.MemId, &m_pmfxSurfacesAux->Data);
                    else
                        local_sts = m_pMFXd3dAllocator->Lock(m_pMFXd3dAllocator->pthis, MFXReadWriteMid(m_pmfxSurfacesAux->Data.MemId, MFXReadWriteMid::write), &m_pmfxSurfacesAux->Data);
                    MSDK_BREAK_ON_ERROR(local_sts);
                    if (ppOutSurf[tail_asdepth]->Info.CropW * 4 == ppOutSurf[tail_asdepth]->Data.Pitch && ppOutSurf[tail_asdepth]->Data.Pitch  == m_pmfxSurfacesAux->Data.Pitch)
                        memcpy_s(m_pmfxSurfacesAux->Data.B, m_pmfxSurfacesAux->Data.Pitch*m_pmfxSurfacesAux->Info.Height, ppOutSurf[tail_asdepth]->Data.B, ppOutSurf[tail_asdepth]->Info.CropW*ppOutSurf[tail_asdepth]->Info.CropH*4);
                    else {
                        for (int i = 0; i < ppOutSurf[tail_asdepth]->Info.CropH; i++)
                            memcpy_s(m_pmfxSurfacesAux->Data.B + i * m_pmfxSurfacesAux->Data.Pitch, m_pmfxSurfacesAux->Data.Pitch, ppOutSurf[tail_asdepth]->Data.B + i*ppOutSurf[tail_asdepth]->Data.Pitch, ppOutSurf[tail_asdepth]->Info.CropW*4);
                    }

                    if (m_accelType == D3D9)
                        local_sts = m_pMFXd3dAllocator->Unlock(m_pMFXd3dAllocator->pthis, m_pmfxSurfacesAux->Data.MemId, &m_pmfxSurfacesAux->Data);
                    else
                        local_sts = m_pMFXd3dAllocator->Unlock(m_pMFXd3dAllocator->pthis, MFXReadWriteMid(m_pmfxSurfacesAux->Data.MemId, MFXReadWriteMid::write), &m_pmfxSurfacesAux->Data);
#else
                    local_sts = m_pMFXd3dAllocator->Lock(m_pMFXd3dAllocator->pthis, m_pmfxSurfacesAux->Data.MemId, &m_pmfxSurfacesAux->Data);
                    MSDK_BREAK_ON_ERROR(local_sts);
                    if (ppOutSurf[tail_asdepth]->Info.CropW * 4 == ppOutSurf[tail_asdepth]->Data.Pitch && ppOutSurf[tail_asdepth]->Data.Pitch  == m_pmfxSurfacesAux->Data.Pitch)
                        memcpy_s(m_pmfxSurfacesAux->Data.B, m_pmfxSurfacesAux->Data.Pitch*m_pmfxSurfacesAux->Info.Height, ppOutSurf[tail_asdepth]->Data.B, ppOutSurf[tail_asdepth]->Info.CropW*ppOutSurf[tail_asdepth]->Info.CropH*4);
                    else {
                        for (int i = 0; i < ppOutSurf[tail_asdepth]->Info.CropH; i++)
                            memcpy_s(m_pmfxSurfacesAux->Data.B + i * m_pmfxSurfacesAux->Data.Pitch, m_pmfxSurfacesAux->Data.Pitch, ppOutSurf[tail_asdepth]->Data.B + i*ppOutSurf[tail_asdepth]->Data.Pitch, ppOutSurf[tail_asdepth]->Info.CropW*4);
                    }
                    local_sts = m_pMFXd3dAllocator->Unlock(m_pMFXd3dAllocator->pthis, m_pmfxSurfacesAux->Data.MemId, &m_pmfxSurfacesAux->Data);
#endif
                    MSDK_BREAK_ON_ERROR(local_sts);
                    local_sts = m_d3dRender.RenderFrame(m_pmfxSurfacesAux, m_pMFXd3dAllocator);

                } else
                    local_sts = m_d3dRender.RenderFrame(ppOutSurf[tail_asdepth], m_pMFXd3dAllocator);
#endif

                if (local_sts == MFX_ERR_NULL_PTR)
                    local_sts = MFX_ERR_NONE;
                MSDK_BREAK_ON_ERROR(local_sts);
            }


            if (m_bOutput && !quitOnFrameLimit) {

                if (ppOutSurf[tail_asdepth]->Data.MemId) {
#if D3D_SURFACES_SUPPORT
                    if (m_memTypeOut == D3D11_MEMORY)
                        local_sts = m_pMFXAllocatorOut->Lock(m_pMFXAllocatorOut->pthis, MFXReadWriteMid(ppOutSurf[tail_asdepth]->Data.MemId, MFXReadWriteMid::read), &ppOutSurf[tail_asdepth]->Data);
                    else
#endif
                        local_sts = m_pMFXAllocatorOut->Lock(m_pMFXAllocatorOut->pthis, ppOutSurf[tail_asdepth]->Data.MemId, &ppOutSurf[tail_asdepth]->Data);
                }
                camera_printf("--writing frame %d \n", ppOutSurf[tail_asdepth]->Data.FrameOrder);

                if (m_alphaValue >= 0) {
                    if (ppOutSurf[tail_asdepth]->Info.FourCC == MFX_FOURCC_ARGB16) {
                        int stride = ppOutSurf[tail_asdepth]->Data.Pitch >> 1;
                        for (int i = 0; i < ppOutSurf[tail_asdepth]->Info.Height; i++) {
                            for (int j = 0; j < ppOutSurf[tail_asdepth]->Info.Width; j++) {
                                *(ppOutSurf[tail_asdepth]->Data.V16 + 3 + j*4 + i*stride) = (mfxU16)m_alphaValue;
                            }
                        }
                    }
                    else if (ppOutSurf[tail_asdepth]->Info.FourCC == MFX_FOURCC_ABGR16) {
                        int stride = ppOutSurf[tail_asdepth]->Data.Pitch >> 1;
                        for (int i = 0; i < ppOutSurf[tail_asdepth]->Info.Height; i++) {
                            for (int j = 0; j < ppOutSurf[tail_asdepth]->Info.Width; j++) {
                                *(ppOutSurf[tail_asdepth]->Data.Y16 + 3 + j * 4 + i*stride) = (mfxU16)m_alphaValue;
                            }
                        }
                    }
                    else if (ppOutSurf[tail_asdepth]->Info.FourCC == MFX_FOURCC_RGB4) {
                        int stride = ppOutSurf[tail_asdepth]->Data.Pitch;
                        for (int i = 0; i < ppOutSurf[tail_asdepth]->Info.Height; i++) {
                            for (int j = 0; j < ppOutSurf[tail_asdepth]->Info.Width; j++) {
                                *(ppOutSurf[tail_asdepth]->Data.A + j*4 + i*stride) = (mfxU8)m_alphaValue;
                            }
                        }
                    }
                }
                if (m_pRawFileWriter) {
                    if (ppOutSurf[tail_asdepth]->Info.FourCC == MFX_FOURCC_ARGB16 ||
                        ppOutSurf[tail_asdepth]->Info.FourCC == MFX_FOURCC_ABGR16)
                        m_pRawFileWriter->WriteFrameARGB16(&ppOutSurf[tail_asdepth]->Data, 0, &ppOutSurf[tail_asdepth]->Info);
                    else if (ppOutSurf[tail_asdepth]->Info.FourCC == MFX_FOURCC_NV12)
                        m_pRawFileWriter->WriteFrameNV12(&ppOutSurf[tail_asdepth]->Data, 0, &ppOutSurf[tail_asdepth]->Info);
                }
                else if (m_pBmpWriter)
                    m_pBmpWriter->WriteFrame(&ppOutSurf[tail_asdepth]->Data, 0, &ppOutSurf[tail_asdepth]->Info);

                if (ppOutSurf[tail_asdepth]->Data.MemId) {
#if D3D_SURFACES_SUPPORT
                    if (m_memTypeOut == D3D11_MEMORY)
                        local_sts = m_pMFXAllocatorOut->Unlock(m_pMFXAllocatorOut->pthis, MFXReadWriteMid(ppOutSurf[tail_asdepth]->Data.MemId, MFXReadWriteMid::read), &ppOutSurf[tail_asdepth]->Data);
                    else
#endif
                        local_sts = m_pMFXAllocatorOut->Unlock(m_pMFXAllocatorOut->pthis, ppOutSurf[tail_asdepth]->Data.MemId, &ppOutSurf[tail_asdepth]->Data);
                }
            }

            ReleaseSurface(ppInSurf[tail_asdepth]);
            ReleaseSurface(ppOutSurf[tail_asdepth]);
            ++frameIdx;
            msdk_printf(MSDK_STRING("Frame number: %d\r"), frameIdx);
        }
        if (!quitOnFrameLimit)
            m_nFrameIndex = frameIdx;

        sts = (local_sts != MFX_ERR_NONE) ? local_sts : sts;
    }

    m_bEnd = true;
    pthread->Wait();
    MSDK_SAFE_DELETE(pthread);

    MSDK_SAFE_DELETE_ARRAY(syncpoints);
    MSDK_SAFE_DELETE_ARRAY(ppOutSurf);
    MSDK_SAFE_DELETE_ARRAY(ppInSurf);
    MSDK_SAFE_DELETE_ARRAY(syncFlags);

    for (int i = 0; i <  m_mfxResponseIn.NumFrameActual; i++)
        m_pmfxSurfacesIn[i].Data.Locked = 0;

    return sts;
}

void CCameraPipeline::PrintInfo()
{
    msdk_printf(MSDK_STRING("Intel(R) Camera SDK Sample Version %s\n\n"), GetMSDKSampleVersion().c_str());

    mfxFrameInfo Info = m_mfxVideoParams.vpp.In;
    msdk_printf(MSDK_STRING("Input format\t%s\n"), FourCC2Str( Info.FourCC ));
    msdk_printf(MSDK_STRING("Resolution\t%dx%d\n"), Info.Width, Info.Height);
    msdk_printf(MSDK_STRING("Crop X,Y,W,H\t%d,%d,%d,%d\n"), Info.CropX, Info.CropY, Info.CropW, Info.CropH);
    msdk_printf(MSDK_STRING("Frame rate\t%.2f\n"), (mfxF64)Info.FrameRateExtN / Info.FrameRateExtD);

    Info = m_mfxVideoParams.vpp.Out;
    msdk_printf(MSDK_STRING("Output format\t%s\n"), FourCC2Str( Info.FourCC ));
    msdk_printf(MSDK_STRING("Resolution\t%dx%d\n"), Info.Width, Info.Height);
    msdk_printf(MSDK_STRING("Crop X,Y,W,H\t%d,%d,%d,%d\n"), Info.CropX, Info.CropY, Info.CropW, Info.CropH);
    msdk_printf(MSDK_STRING("Frame rate\t%.2f\n"), (mfxF64)Info.FrameRateExtN / Info.FrameRateExtD);


    //mfxFrameInfo Info = m_mfxVideoParams.mfx.FrameInfo;
    //msdk_printf(MSDK_STRING("Resolution\t%dx%d\n"), Info.Width, Info.Height);
    //msdk_printf(MSDK_STRING("Crop X,Y,W,H\t%d,%d,%d,%d\n"), Info.CropX, Info.CropY, Info.CropW, Info.CropH);

    //mfxF64 dFrameRate = CalculateFrameRate(Info.FrameRateExtN, Info.FrameRateExtD);
    //msdk_printf(MSDK_STRING("Frame rate\t%.2f\n"), dFrameRate);

    const msdk_char* sMemTypeIn = m_memTypeIn == D3D9_MEMORY  ? MSDK_STRING("d3d")
                             : (m_memTypeIn == D3D11_MEMORY ? MSDK_STRING("d3d11")
                                                          : MSDK_STRING("system"));
    msdk_printf(MSDK_STRING("Input memory type\t\t%s\n"), sMemTypeIn);
    const msdk_char* sMemTypeOut = m_memTypeOut == D3D9_MEMORY  ? MSDK_STRING("d3d")
                             : (m_memTypeOut == D3D11_MEMORY ? MSDK_STRING("d3d11")
                                                          : MSDK_STRING("system"));
    msdk_printf(MSDK_STRING("Output memory type\t\t%s\n"), sMemTypeOut);

    mfxIMPL impl;
    m_mfxSession.QueryIMPL(&impl);

    const msdk_char* sImpl = (MFX_IMPL_VIA_D3D11 == MFX_IMPL_VIA_MASK(impl)) ? MSDK_STRING("hw_d3d11")
                           : (MFX_IMPL_HARDWARE & impl)  ? MSDK_STRING("hw")
                                                         : MSDK_STRING("sw");
    msdk_printf(MSDK_STRING("MediaSDK impl\t\t%s\n"), sImpl);

    mfxVersion ver;
    m_mfxSession.QueryVersion(&ver);
    msdk_printf(MSDK_STRING("MediaSDK version\t%d.%d\n"), ver.Major, ver.Minor);

    msdk_printf(MSDK_STRING("\n"));

    return;
}
