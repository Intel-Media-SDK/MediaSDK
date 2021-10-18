// Copyright (c) 2018-2020 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "asc.h"
#include "asc_defs.h"
#include "asc_cpu_dispatcher.h"
#include "libmfx_core_interface.h"
#include "genx_scd_gen8_isa.h"
#include "genx_scd_gen9_isa.h"
#include "genx_scd_gen11_isa.h"
#include "genx_scd_gen11lp_isa.h"
#include "genx_scd_gen12lp_isa.h"

#include "tree.h"
#include "iofunctions.h"
#include "motion_estimation_engine.h"
#include <limits.h>
#include <algorithm>
#include <cmath>

using std::min;
using std::max;

namespace ns_asc {
static mfxI8
    PDISTTbl2[NumTSC*NumSC] =
{
    2, 3, 3, 4, 4, 5, 5, 5, 5, 5,
    2, 2, 3, 3, 4, 4, 5, 5, 5, 5,
    1, 2, 2, 3, 3, 3, 4, 4, 5, 5,
    1, 1, 2, 2, 3, 3, 3, 4, 4, 5,
    1, 1, 2, 2, 3, 3, 3, 3, 3, 4,
    1, 1, 1, 2, 2, 3, 3, 3, 3, 3,
    1, 1, 1, 1, 2, 2, 3, 3, 3, 3,
    1, 1, 1, 1, 2, 2, 2, 3, 3, 3,
    1, 1, 1, 1, 1, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};
static mfxU32 lmt_sc2[NumSC] = { 112, 255, 512, 1536, 4096, 6144, 10752, 16384, 23040, UINT_MAX };
static mfxU32 lmt_tsc2[NumTSC] = { 24, 48, 72, 96, 128, 160, 192, 224, 256, UINT_MAX };

ASC_API ASCimageData::ASCimageData() {
    Image.data = nullptr;
    Image.Y = nullptr;
    Image.U = nullptr;
    Image.V = nullptr;
    Image.extHeight = 0;
    Image.extWidth = 0;
    Image.hBorder = 0;
    Image.wBorder = 0;
    Image.height = 0;
    Image.width = 0;
    Image.pitch = 0;
    
    pInteger = nullptr;
    gpuImage = nullptr;
    idxImage = nullptr;
    Cs = nullptr;
    Rs = nullptr;
    RsCs = nullptr;
    SAD = nullptr;
    
    CsVal = 0;
    RsVal = 0;
    avgval = 0;
    var = 0;
    jtvar = 0;
    mcjtvar = 0;
    tcor = 0;
    mcTcor = 0;
}

mfxStatus ASCimageData::InitFrame(ASCImDetails *pDetails) {
    mfxU32
        imageSpaceSize = pDetails->Extended_Height * pDetails->Extended_Width,
        mvSpaceSize = (pDetails->_cheight * pDetails->_cwidth) >> 6,
        texSpaceSize = (pDetails->_cheight * pDetails->_cwidth) >> 4;

    Image.extHeight = pDetails->Extended_Height;
    Image.extWidth = pDetails->Extended_Width;
    Image.pitch = pDetails->Extended_Width;
    Image.height = pDetails->_cheight;
    Image.width = pDetails->_cwidth;
    Image.hBorder = pDetails->vertical_pad;
    Image.wBorder = pDetails->horizontal_pad;
    Image.data = NULL;
    Image.Y = NULL;
    Image.U = NULL;
    Image.V = NULL;
    //Memory Allocation
    Image.data = (mfxU8*)memalign(0x1000, imageSpaceSize);
    SAD = (mfxU16 *)memalign(0x1000, sizeof(mfxU16) * mvSpaceSize);
    Rs = (mfxU16 *)memalign(0x1000, sizeof(mfxU16) * texSpaceSize);
    Cs = (mfxU16 *)memalign(0x1000, sizeof(mfxU16) * texSpaceSize);
    RsCs = (mfxU16 *)memalign(0x1000, sizeof(mfxU16) * texSpaceSize);
    pInteger = (ASCMVector *)memalign(0x1000, sizeof(ASCMVector)  * mvSpaceSize);
    if (Image.data == NULL)
        return MFX_ERR_MEMORY_ALLOC;
    //Pointer conf.
    memset(Image.data, 0, sizeof(mfxU8) * imageSpaceSize);
    Image.Y = Image.data + pDetails->initial_point;
    if (SAD == NULL || Rs == NULL)
        return MFX_ERR_MEMORY_ALLOC;
    memset(Rs, 0, sizeof(mfxU16) * texSpaceSize);
    if (Cs == NULL)
        return MFX_ERR_MEMORY_ALLOC;
    memset(Cs, 0, sizeof(mfxU16) * texSpaceSize);
    if (RsCs == NULL)
        return MFX_ERR_MEMORY_ALLOC;
    memset(RsCs, 0, sizeof(mfxU16) * texSpaceSize);
    if (pInteger == NULL)
        return MFX_ERR_MEMORY_ALLOC;
    memset(pInteger, 0, sizeof(ASCMVector)  * mvSpaceSize);
    return MFX_ERR_NONE;
}

mfxStatus ASCimageData::InitAuxFrame(ASCImDetails *pDetails) {
    mfxU32
        imageSpaceSize = pDetails->Extended_Height * pDetails->Extended_Width;

    Image.extHeight = pDetails->Extended_Height;
    Image.extWidth = pDetails->Extended_Width;
    Image.pitch = pDetails->Extended_Width;
    Image.height = pDetails->_cheight;
    Image.width = pDetails->_cwidth;
    Image.hBorder = pDetails->vertical_pad;
    Image.wBorder = pDetails->horizontal_pad;
    Image.data = NULL;
    Image.Y = NULL;
    Image.U = NULL;
    Image.V = NULL;
    //Memory Allocation
    Image.data = (mfxU8*)memalign(0x1000, imageSpaceSize);
    if (Image.data == NULL)
        return MFX_ERR_MEMORY_ALLOC;
    //Pointer conf.
    memset(Image.data, 0, sizeof(mfxU8) * imageSpaceSize);
    Image.Y = Image.data + pDetails->initial_point;
    return MFX_ERR_NONE;
}

void ASCimageData::Close() {
    if (Rs)
        free(Rs);
    if (Cs)
        free(Cs);
    if (RsCs)
        free(RsCs);
    if (pInteger)
        free(pInteger);
    if (SAD)
        free(SAD);
    if (Image.data)
        free(Image.data);
    Rs = nullptr;
    Cs = nullptr;
    RsCs = nullptr;
    pInteger = nullptr;
    SAD = nullptr;
    Image.data = nullptr;
    Image.Y = nullptr;
    Image.U = nullptr;
    Image.V = nullptr;
}

ASC_API ASC::ASC()
{
    m_device = nullptr;
    m_queue  = nullptr;
    m_pSurfaceCp = nullptr;
    m_pIdxSurfCp = nullptr;
    m_program    = nullptr;
    m_kernel_p = nullptr;
    m_kernel_t = nullptr;
    m_kernel_b = nullptr;
    m_kernel_cp = nullptr;
    m_threadSpace = nullptr,
    m_threadSpaceCp = nullptr;
    m_subSamplingEv = nullptr;
    m_frameCopyEv   = nullptr;
    m_task = nullptr,
    m_taskCp = nullptr;
    
    m_gpuImPitch = 0;
    m_threadsWidth = 0;
    m_threadsHeight = 0;
    m_gpuwidth = 0,
    m_gpuheight = 0;
    
    m_frameBkp = nullptr;
    m_support  = nullptr;
    m_dataIn   = nullptr;
    m_videoData = nullptr;
    
    m_dataReady = false;
    m_cmDeviceAssigned = false;
    m_is_LTR_on = false;
    m_ASCinitialized = false;
    
    m_width = 0;
    m_height = 0;
    m_pitch = 0;

    m_AVX2_available = 0;
    m_SSE4_available = 0;
    GainOffset              = nullptr;
    RsCsCalc_4x4            = nullptr;
    RsCsCalc_bound          = nullptr;
    RsCsCalc_diff           = nullptr;
    ImageDiffHistogram      = nullptr;
    ME_SAD_8x8_Block_Search = nullptr;
    Calc_RaCa_pic           = nullptr;
    resizeFunc              = nullptr;
    ME_SAD_8x8_Block        = nullptr;
    ME_VAR_8x8_Block        = nullptr;
}

void ASC::Setup_Environment() {
    m_dataIn->accuracy = 1;

    m_dataIn->layer->Original_Width = ASC_SMALL_WIDTH;
    m_dataIn->layer->Original_Height = ASC_SMALL_HEIGHT;
    m_dataIn->layer->_cwidth = ASC_SMALL_WIDTH;
    m_dataIn->layer->_cheight = ASC_SMALL_HEIGHT;

    m_dataIn->layer->block_width = 8;
    m_dataIn->layer->block_height = 8;
    m_dataIn->layer->vertical_pad = 0;
    m_dataIn->layer->horizontal_pad = 0;
    m_dataIn->layer->Extended_Height = m_dataIn->layer->vertical_pad + ASC_SMALL_HEIGHT + m_dataIn->layer->vertical_pad;
    m_dataIn->layer->Extended_Width = m_dataIn->layer->horizontal_pad + ASC_SMALL_WIDTH + m_dataIn->layer->horizontal_pad;
    m_dataIn->layer->pitch = m_dataIn->layer->Extended_Width;
    m_dataIn->layer->Height_in_blocks = m_dataIn->layer->_cheight / m_dataIn->layer->block_height;
    m_dataIn->layer->Width_in_blocks = m_dataIn->layer->_cwidth / m_dataIn->layer->block_width;
    m_dataIn->layer->sidesize = m_dataIn->layer->_cheight + (1 * m_dataIn->layer->vertical_pad);
    m_dataIn->layer->initial_point = (m_dataIn->layer->Extended_Width * m_dataIn->layer->vertical_pad) + m_dataIn->layer->horizontal_pad;
    m_dataIn->layer->MVspaceSize = (m_dataIn->layer->_cheight / m_dataIn->layer->block_height) * (m_dataIn->layer->_cwidth / m_dataIn->layer->block_width);
}

void ASC::Reset_ASCCmDevice() {
    m_cmDeviceAssigned = false;
}

void ASC::Set_ASCCmDevice() {
    m_cmDeviceAssigned = true;
}

bool ASC::Query_ASCCmDevice() {
    return m_cmDeviceAssigned;
}

mfxStatus ASC::InitGPUsurf(CmDevice* pCmDevice) {
#ifdef MFX_ENABLE_KERNELS
    INT res = CM_SUCCESS;
    m_subSamplingEv = nullptr;
    m_frameCopyEv   = nullptr;

    Reset_ASCCmDevice();
    m_device = pCmDevice;
    if (!m_device)
        res = CM_FAILURE;

    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    Set_ASCCmDevice();

    res = m_device->CreateQueue(m_queue);
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);

    mfxU32 hwType = 0;
    size_t hwSize = sizeof(hwType);
    res = m_device->GetCaps(CAP_GPU_PLATFORM, hwSize, &hwType);
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);

    switch (hwType)
    {
    case PLATFORM_INTEL_BDW:
        res = m_device->LoadProgram((void *)genx_scd_gen8, sizeof(genx_scd_gen8), m_program, "nojitter");
        break;
    case PLATFORM_INTEL_SKL:
    case PLATFORM_INTEL_KBL:
    case PLATFORM_INTEL_CFL:
    case PLATFORM_INTEL_GLK:
    case PLATFORM_INTEL_BXT:
        res = m_device->LoadProgram((void *)genx_scd_gen9, sizeof(genx_scd_gen9), m_program, "nojitter");
        break;
    case PLATFORM_INTEL_ICL:
        res = m_device->LoadProgram((void *)genx_scd_gen11, sizeof(genx_scd_gen11), m_program, "nojitter");
        break;
    case PLATFORM_INTEL_ICLLP:
        res = m_device->LoadProgram((void *)genx_scd_gen11lp, sizeof(genx_scd_gen11lp), m_program, "nojitter");
        break;
    case PLATFORM_INTEL_TGLLP:
    case PLATFORM_INTEL_RKL:
    case PLATFORM_INTEL_DG1:
        res = m_device->LoadProgram((void *)genx_scd_gen12lp, sizeof(genx_scd_gen12lp), m_program, "nojitter");
        break;
    default:
        res = CM_NOT_IMPLEMENTED;
}
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);

    return MFX_ERR_NONE;
#else
    (void)pCmDevice;
    return MFX_ERR_UNSUPPORTED;
#endif
}

void ASC::Params_Init() {
    m_dataIn->accuracy  = 1;
    m_dataIn->processed_frames = 0;
    m_dataIn->total_number_of_frames = -1;
    m_dataIn->starting_frame = 0;
    m_dataIn->key_frame_frequency = INT_MAX;
    m_dataIn->limitRange = 0;
    m_dataIn->maxXrange = 32;
    m_dataIn->maxYrange = 32;
    m_dataIn->interlaceMode = 0;
    m_dataIn->StartingField = ASCTopField;
    m_dataIn->currentField = ASCTopField;
    ImDetails_Init(m_dataIn->layer);
}

mfxStatus ASC::CreateCmKernels() {
    INT res;
    m_kernel_p = NULL;
    m_kernel_t = NULL;
    m_kernel_b = NULL;
    m_threadSpace = NULL;
    m_threadSpaceCp = NULL;

    m_threadsWidth = subWidth / OUT_BLOCK;
    m_threadsHeight = subHeight;

    res = m_device->CreateThreadSpace(m_threadsWidth, m_threadsHeight, m_threadSpace);
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = m_device->CreateKernel(m_program, CM_KERNEL_FUNCTION(SubSamplePoint_p), m_kernel_p);
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = m_kernel_p->SetThreadCount(m_threadsWidth * m_threadsHeight);
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = m_device->CreateKernel(m_program, CM_KERNEL_FUNCTION(SubSamplePoint_t), m_kernel_t);
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = m_kernel_t->SetThreadCount(m_threadsWidth * m_threadsHeight);
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = m_device->CreateKernel(m_program, CM_KERNEL_FUNCTION(SubSamplePoint_b), m_kernel_b);
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = m_kernel_b->SetThreadCount(m_threadsWidth * m_threadsHeight);
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
#ifndef CMRT_EMU
    res = m_kernel_p->AssociateThreadSpace(m_threadSpace);
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = m_kernel_t->AssociateThreadSpace(m_threadSpace);
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = m_kernel_b->AssociateThreadSpace(m_threadSpace);
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
#endif
    return MFX_ERR_NONE;
}

ASC_API mfxStatus ASC::SetInterlaceMode(ASCFTS interlaceMode) {
    if (interlaceMode > ASCbotfieldFirst_frame) {
        ASC_PRINTF("\nError: Interlace Mode invalid, valid values are: 1 (progressive), 2 (TFF), 3 (BFF)\n");
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }
    else
        m_dataIn->interlaceMode = interlaceMode;

    m_dataIn->StartingField = ASCTopField;
    if (m_dataIn->interlaceMode != ASCprogressive_frame) {
        if (m_dataIn->interlaceMode == ASCbotfieldFirst_frame)
            m_dataIn->StartingField = ASCBottomField;
        resizeFunc = &ASC::SubSampleASC_ImageInt;
    }
    else {
        resizeFunc = &ASC::SubSampleASC_ImagePro;
    }
    m_dataIn->currentField = m_dataIn->StartingField;
    return MFX_ERR_NONE;
}

mfxStatus ASC::VidSample_Alloc() {
    INT res = CM_SUCCESS;

    for (mfxI32 i = 0; i < ASCVIDEOSTATSBUF; i++)
    {
        MFX_SAFE_CALL(m_videoData[i]->layer.InitFrame(m_dataIn->layer));
        if (Query_ASCCmDevice())
        {
            res = m_device->CreateSurface2DUP(m_dataIn->layer->Extended_Width, m_dataIn->layer->Extended_Height, CM_SURFACE_FORMAT_A8, (void *)m_videoData[i]->layer.Image.data, m_videoData[i]->layer.gpuImage);
            SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
            res = m_videoData[i]->layer.gpuImage->GetIndex(m_videoData[i]->layer.idxImage);
            SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
        }
        else
        {
            m_videoData[i]->layer.gpuImage = nullptr;
            m_videoData[i]->layer.idxImage = nullptr;
        }
    }

    if (Query_ASCCmDevice())
    {
        mfxU32
            physicalSize = 0;
        res = m_device->GetSurface2DInfo(m_gpuwidth, m_gpuheight, CM_SURFACE_FORMAT_NV12, m_gpuImPitch, physicalSize);
        SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
        m_frameBkp = nullptr;
        m_frameBkp = (mfxU8*)memalign(0x1000, physicalSize);
        if (m_frameBkp == nullptr)
            return MFX_ERR_MEMORY_ALLOC;
        memset(m_frameBkp, 0, physicalSize);
        res = m_device->CreateSurface2DUP(m_gpuImPitch, m_gpuheight, CM_SURFACE_FORMAT_NV12, (void *)m_frameBkp, m_pSurfaceCp);
        SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
        res = m_pSurfaceCp->GetIndex(m_pIdxSurfCp);
        SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    }
    else
    {
        m_frameBkp = nullptr;
        m_pSurfaceCp = nullptr;
        m_pIdxSurfCp = nullptr;
    }
    return MFX_ERR_NONE;
}

mfxStatus ASC::AssignResources(mfxU8 position, mfxU8 *pixelData)
{
    if (!IsASCinitialized())
        return MFX_ERR_DEVICE_FAILED;
    if (pixelData == nullptr)
        return MFX_ERR_DEVICE_FAILED;
    m_videoData[position]->layer.Image.Y = pixelData;
    return MFX_ERR_NONE;
}

mfxStatus ASC::AssignResources(mfxU8 position, CmSurface2DUP *inputFrame, mfxU8 *pixelData)
{
    if (!IsASCinitialized())
        return MFX_ERR_DEVICE_FAILED;
    if (inputFrame == nullptr)
        return MFX_ERR_DEVICE_FAILED;
    //std::swap(m_videoData[position]->layer.gpuImage, inputFrame);
    m_videoData[position]->layer.gpuImage = inputFrame;
    m_videoData[position]->layer.gpuImage->GetIndex(m_videoData[position]->layer.idxImage);
    if (pixelData == nullptr)
        return MFX_ERR_DEVICE_FAILED;
    /*std::swap(m_videoData[position]->layer.Image.data, pixelData);
    m_videoData[position]->layer.Image.Y = m_videoData[position]->layer.Image.data;*/
    m_videoData[position]->layer.Image.Y = pixelData;
    return MFX_ERR_NONE;
}

mfxStatus ASC::SwapResources(mfxU8 position, CmSurface2DUP **inputFrame, mfxU8 **pixelData)
{
    if (!IsASCinitialized())
        return MFX_ERR_DEVICE_FAILED;
    if (inputFrame == nullptr)
        return MFX_ERR_DEVICE_FAILED;
    std::swap(m_videoData[position]->layer.gpuImage, *inputFrame);
    m_videoData[position]->layer.gpuImage->GetIndex(m_videoData[position]->layer.idxImage);
    if (pixelData == nullptr)
        return MFX_ERR_DEVICE_FAILED;
    std::swap(m_videoData[position]->layer.Image.data, *pixelData);
    m_videoData[position]->layer.Image.Y = m_videoData[position]->layer.Image.data;
    return MFX_ERR_NONE;
}

void ASC::VidSample_dispose()
{
    for (mfxI32 i = ASCVIDEOSTATSBUF - 1; i >= 0; i--)
    {
        if (m_videoData[i] != nullptr)
        {
            m_videoData[i]->layer.Close();
            if (m_videoData[i]->layer.gpuImage) {
                m_device->DestroySurface2DUP(m_videoData[i]->layer.gpuImage);
                m_videoData[i]->layer.gpuImage = nullptr;
                m_videoData[i]->layer.idxImage = nullptr;
            }
            delete (m_videoData[i]);
        }
    }
    free(m_frameBkp);
}
void ASC::VidRead_dispose()
{
    if (m_support->logic != nullptr)
    {
        for (mfxI32 i = 0; i < TSCSTATBUFFER; i++)
            delete m_support->logic[i];
        delete[] m_support->logic;
    }
    if (m_support->gainCorrection.Image.data != nullptr)
        m_support->gainCorrection.Close();
}

mfxStatus ASC::alloc() {
    return VidSample_Alloc();
}

mfxStatus ASC::InitCPU() {
    return alloc();
}

mfxStatus ASC::IO_Setup() {
    mfxStatus sts = MFX_ERR_NONE;
    sts = alloc();
    SCD_CHECK_MFX_ERR(sts);
    if (Query_ASCCmDevice())
    {
        SCD_CHECK_CM_ERR(m_device->CreateKernel(m_program, CM_KERNEL_FUNCTION(surfaceCopy_Y), m_kernel_cp), MFX_ERR_DEVICE_FAILED);
        m_threadsWidth = (UINT)ceil((double)m_gpuwidth / SCD_BLOCK_PIXEL_WIDTH);
        m_threadsHeight = (UINT)ceil((double)m_gpuheight / SCD_BLOCK_HEIGHT);
        SCD_CHECK_CM_ERR(m_device->CreateThreadSpace(m_threadsWidth, m_threadsHeight, m_threadSpaceCp), MFX_ERR_DEVICE_FAILED);
        SCD_CHECK_CM_ERR(m_kernel_cp->SetThreadCount(m_threadsWidth * m_threadsHeight), MFX_ERR_DEVICE_FAILED);
#ifndef CMRT_EMU
        SCD_CHECK_CM_ERR(m_kernel_cp->AssociateThreadSpace(m_threadSpaceCp), MFX_ERR_DEVICE_FAILED);
#endif
    }
    else
    {
        m_kernel_cp = nullptr;
        m_threadSpaceCp = nullptr;
    }
    return sts;
}

void ASC::InitStruct() {
    m_dataIn = nullptr;
    m_support = nullptr;
    m_videoData = nullptr;
    resizeFunc = nullptr;
}

mfxStatus ASC::VidRead_Init() {
    mfxStatus sts = MFX_ERR_NONE;
    m_support->control                 = 0;
    m_support->average                 = 0;
    m_support->avgSAD                  = 0;
    m_support->gopSize                 = 1;
    m_support->pendingSch              = 0;
    m_support->lastSCdetectionDistance = 0;
    m_support->detectedSch             = 0;
    m_support->frameOrder              = 0;
    m_support->PDistanceTable            = PDISTTbl2;
    m_support->size                      = ASCSmall_Size;
    m_support->firstFrame                = true;
    m_support->gainCorrection.Image.data = nullptr;
    m_support->gainCorrection.Image.Y    = nullptr;
    m_support->gainCorrection.Image.U    = nullptr;
    m_support->gainCorrection.Image.V    = nullptr;
    m_support->gainCorrection.Cs         = nullptr;
    m_support->gainCorrection.Rs         = nullptr;
    m_support->gainCorrection.RsCs       = nullptr;
    m_support->gainCorrection.pInteger   = nullptr;
    m_support->gainCorrection.SAD        = nullptr;
    try
    {
        m_support->logic = new ASCTSCstat *[TSCSTATBUFFER];
    }
    catch (...)
    {
        return MFX_ERR_MEMORY_ALLOC;
    }
    sts = ASCTSCstat_Init(m_support->logic);
    SCD_CHECK_MFX_ERR(sts);
//#if USE_SCD
    sts = m_support->gainCorrection.InitAuxFrame(m_dataIn->layer);
//#else
//    sts = m_support->gainCorrection.InitFrame(m_dataIn->layer);
//#endif
    SCD_CHECK_MFX_ERR(sts);
    return sts;
}

void ASC::VidSample_Init() {
    for(mfxI32 i = 0; i < ASCVIDEOSTATSBUF; i++) {
        nullifier(&m_videoData[i]->layer);
        imageInit(&m_videoData[i]->layer.Image);
        m_videoData[i]->frame_number = -1;
        m_videoData[i]->forward_reference = -1;
        m_videoData[i]->backward_reference = -1;
    }
}

void ASC::SetUltraFastDetection() {
    m_support->size = ASCSmall_Size;
    resizeFunc = &ASC::SubSampleASC_ImagePro;
}

mfxStatus ASC::SetWidth(mfxI32 Width) {
    if(Width < ASC_SMALL_WIDTH) {
        ASC_PRINTF("\nError: Width value is too small, it needs to be bigger than %i\n", ASC_SMALL_WIDTH);
        return MFX_ERR_UNSUPPORTED;
    }
    else
        m_width = Width;

    return MFX_ERR_NONE;
}

mfxStatus ASC::SetHeight(mfxI32 Height) {
    if(Height < ASC_SMALL_HEIGHT) {
        ASC_PRINTF("\nError: Height value is too small, it needs to be bigger than %i\n", ASC_SMALL_HEIGHT);
        return MFX_ERR_UNSUPPORTED;
    }
    else
        m_height = Height;

    return MFX_ERR_NONE;
}

mfxStatus ASC::SetPitch(mfxI32 Pitch) {
    if(m_width < ASC_SMALL_WIDTH) {
        ASC_PRINTF("\nError: Width value has not been set, init the variables first\n");
        return MFX_ERR_UNSUPPORTED;
    }

    if(Pitch < m_width) {
        ASC_PRINTF("\nError: Pitch value is too small, it needs to be bigger than %i\n", m_width);
        return MFX_ERR_UNSUPPORTED;
    }
    else
        m_pitch = Pitch;

    return MFX_ERR_NONE;
}

void ASC::SetNextField() {
    if(m_dataIn->interlaceMode != ASCprogressive_frame)
        m_dataIn->currentField = !m_dataIn->currentField;
}

mfxStatus ASC::SetDimensions(mfxI32 Width, mfxI32 Height, mfxI32 Pitch) {
    mfxStatus sts;
    sts = SetWidth(Width);
    SCD_CHECK_MFX_ERR(sts);
    sts = SetHeight(Height);
    SCD_CHECK_MFX_ERR(sts);
    sts = SetPitch(Pitch);
    SCD_CHECK_MFX_ERR(sts);
    return sts;
}


#define ASC_CPU_DISP_INIT_C(func)           (func = (func ## _C))
#define ASC_CPU_DISP_INIT_SSE4(func)        (func = (func ## _SSE4))
#define ASC_CPU_DISP_INIT_SSE4_C(func)      (m_SSE4_available ? ASC_CPU_DISP_INIT_SSE4(func) : ASC_CPU_DISP_INIT_C(func))

#define ASC_CPU_DISP_INIT_AVX2(func)        (func = (func ## _AVX2))
#define ASC_CPU_DISP_INIT_AVX2_SSE4_C(func) (m_AVX2_available ? ASC_CPU_DISP_INIT_AVX2(func) : ASC_CPU_DISP_INIT_SSE4_C(func))
#define ASC_CPU_DISP_INIT_AVX2_C(func)      (m_AVX2_available ? ASC_CPU_DISP_INIT_AVX2(func) : ASC_CPU_DISP_INIT_C(func))

ASC_API mfxStatus ASC::Init(mfxI32 Width, mfxI32 Height, mfxI32 Pitch, mfxU32 PicStruct, CmDevice* pCmDevice)
{
    mfxStatus sts = MFX_ERR_NONE;
    INT res;
    m_device = nullptr;
    m_queue = nullptr;
    m_program = nullptr;

    m_task = nullptr;
    m_taskCp = nullptr;

    m_AVX2_available = CpuFeature_AVX2();
    m_SSE4_available = CpuFeature_SSE41();

    if (!m_SSE4_available)
        return MFX_ERR_UNSUPPORTED;

    ME_SAD_8x8_Block    = ME_SAD_8x8_Block_SSE4;
    ME_VAR_8x8_Block    = ME_VAR_8x8_Block_SSE4;

    ASC_CPU_DISP_INIT_C(GainOffset);
    ASC_CPU_DISP_INIT_SSE4_C(RsCsCalc_4x4);
    ASC_CPU_DISP_INIT_C(RsCsCalc_bound);
    ASC_CPU_DISP_INIT_C(RsCsCalc_diff);
    ASC_CPU_DISP_INIT_SSE4_C(ImageDiffHistogram);
    ASC_CPU_DISP_INIT_AVX2_SSE4_C(ME_SAD_8x8_Block_Search);
    ASC_CPU_DISP_INIT_SSE4_C(Calc_RaCa_pic);

    InitStruct();
    try
    {
        m_dataIn = new ASCVidData;
    }
    catch (...)
    {
        return MFX_ERR_MEMORY_ALLOC;
    }
    m_dataIn->layer = nullptr;
    try
    {
        m_dataIn->layer = new ASCImDetails;
        m_videoData = new ASCVidSample *[ASCVIDEOSTATSBUF];
        for(mfxU8 i =0; i < ASCVIDEOSTATSBUF; i++)
            m_videoData[i] = nullptr;
        m_support = new ASCVidRead;
    }
    catch (...)
    {
        return MFX_ERR_MEMORY_ALLOC;
    }

    if (pCmDevice)
    {
        res = InitGPUsurf(pCmDevice);
        SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    }

    for (mfxI32 i = 0; i < ASCVIDEOSTATSBUF; i++)
    {
        try
        {
            m_videoData[i] = new ASCVidSample;
        }
        catch (...)
        {
            return MFX_ERR_MEMORY_ALLOC;
        }
    }
    Params_Init();

    sts = SetDimensions(Width, Height, Pitch);
    SCD_CHECK_MFX_ERR(sts);

    m_gpuwidth = Width;
    m_gpuheight = Height;

    VidSample_Init();
    Setup_Environment();

    sts = IO_Setup();
    SCD_CHECK_MFX_ERR(sts);

    sts = VidRead_Init();
    SCD_CHECK_MFX_ERR(sts);
    SetUltraFastDetection();

    if (Query_ASCCmDevice()) {
        sts = CreateCmKernels();
        SCD_CHECK_MFX_ERR(sts);
    }

    sts = SetInterlaceMode((PicStruct & MFX_PICSTRUCT_FIELD_TFF) ? ASCtopfieldfirst_frame :
        (PicStruct & MFX_PICSTRUCT_FIELD_BFF) ? ASCbotfieldFirst_frame :
        ASCprogressive_frame);
    SCD_CHECK_MFX_ERR(sts);
    m_dataReady = false;
    m_ASCinitialized = (sts == MFX_ERR_NONE);
    return sts;
}

ASC_API bool ASC::IsASCinitialized(){
    return m_ASCinitialized;
}

ASC_API void ASC::SetControlLevel(mfxU8 level) {
    if(level >= RF_DECISION_LEVEL) {
        ASC_PRINTF("\nWarning: Control level too high, shot change detection disabled! (%i)\n", level);
        ASC_PRINTF("Control levels 0 to %i, smaller value means more sensitive detection\n", RF_DECISION_LEVEL);
    }
    m_support->control = level;
}

ASC_API mfxStatus ASC::SetGoPSize(mfxU32 GoPSize) {
    if (GoPSize > Double_HEVC_Gop) {
        ASC_PRINTF("\nError: GoPSize is too big! (%i)\n", GoPSize);
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }
    else if (GoPSize == Forbidden_GoP) {
        ASC_PRINTF("\nError: GoPSize value cannot be zero!\n");
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }
    else if (GoPSize > HEVC_Gop && GoPSize <= Double_HEVC_Gop) {
        ASC_PRINTF("\nWarning: Your GoPSize is larger than usual! (%i)\n", GoPSize);
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }
    m_support->gopSize = GoPSize;
    m_support->pendingSch = 0;

    return MFX_ERR_NONE;
}

ASC_API void ASC::ResetGoPSize() {
    SetGoPSize(Immediate_GoP);
}

ASC_API void ASC::Close() {
    if(m_videoData != nullptr) {
        VidSample_dispose();
        delete[] m_videoData;
        m_videoData = nullptr;
    }

    if(m_support != nullptr) {
        VidRead_dispose();
        delete m_support;
        m_support = nullptr;
    }

    if(m_dataIn != nullptr) {
        delete m_dataIn->layer;
        delete m_dataIn;
        m_dataIn = nullptr;
    }

    if (m_device) {
        for (auto& surf : m_tableCmRelations2) {
            CmSurface2D *temp = surf.second;
            m_device->DestroySurface(temp);
        }
        m_tableCmRelations2.clear();
        m_tableCmIndex2.clear();

        if (m_kernel_p)  m_device->DestroyKernel(m_kernel_p);
        if (m_kernel_t)  m_device->DestroyKernel(m_kernel_t);
        if (m_kernel_b)  m_device->DestroyKernel(m_kernel_b);
        if (m_kernel_cp) m_device->DestroyKernel(m_kernel_cp);
        if (m_program)   m_device->DestroyProgram(m_program);
        if (m_pSurfaceCp) m_device->DestroySurface2DUP(m_pSurfaceCp);
        if (m_threadSpace)   m_device->DestroyThreadSpace(m_threadSpace);
        if (m_threadSpaceCp) m_device->DestroyThreadSpace(m_threadSpaceCp);
        if (m_task) m_device->DestroyTask(m_task);
        if (m_taskCp) m_device->DestroyTask(m_taskCp);
    }

    m_pSurfaceCp = nullptr;
    m_kernel_p  = nullptr;
    m_kernel_t  = nullptr;
    m_kernel_b  = nullptr;
    m_kernel_cp = nullptr;
    m_program   = nullptr;
    m_device    = nullptr;
    m_threadSpace   = nullptr;
    m_threadSpaceCp = nullptr;
}

void ASC::SubSampleASC_ImagePro(mfxU8 *frame, mfxI32 srcWidth, mfxI32 srcHeight, mfxI32 inputPitch, ASCLayers dstIdx, mfxU32 /*parity*/) {

    ASCImDetails *pIDetDst = &m_dataIn->layer[dstIdx];
    mfxU8 *pDst = m_videoData[ASCCurrent_Frame]->layer.Image.Y;
    mfxI16& avgLuma = m_videoData[ASCCurrent_Frame]->layer.avgval;

    mfxI32 dstWidth = pIDetDst->Original_Width;
    mfxI32 dstHeight = pIDetDst->Original_Height;
    mfxI32 dstPitch = pIDetDst->pitch;

    SubSample_Point(frame, srcWidth, srcHeight, inputPitch, pDst, dstWidth, dstHeight, dstPitch, avgLuma);
}

void ASC::SubSampleASC_ImageInt(mfxU8 *frame, mfxI32 srcWidth, mfxI32 srcHeight, mfxI32 inputPitch, ASCLayers dstIdx, mfxU32 parity) {

    ASCImDetails *pIDetDst = &m_dataIn->layer[dstIdx];
    mfxU8 *pDst = m_videoData[ASCCurrent_Frame]->layer.Image.Y;
    mfxI16 &avgLuma = m_videoData[ASCCurrent_Frame]->layer.avgval;

    mfxI32 dstWidth = pIDetDst->Original_Width;
    mfxI32 dstHeight = pIDetDst->Original_Height;
    mfxI32 dstPitch = pIDetDst->pitch;

    SubSample_Point(frame + (parity * inputPitch), srcWidth, srcHeight / 2, inputPitch * 2, pDst, dstWidth, dstHeight, dstPitch, avgLuma);
}

//
// SubSample pSrc into pDst, using point-sampling of source pixels
// Corrects the position on odd lines in case the input video is
// interlaced
//
void ASC::SubSample_Point(
    pmfxU8 pSrc, mfxU32 srcWidth, mfxU32 srcHeight, mfxU32 srcPitch,
    pmfxU8 pDst, mfxU32 dstWidth, mfxU32 dstHeight, mfxU32 dstPitch,
    mfxI16 &avgLuma) {
    mfxI32 step_w = srcWidth / dstWidth;
    mfxI32 step_h = srcHeight / dstHeight;

    mfxI32 need_correction = !(step_h % 2);
    mfxI32 correction = 0;
    mfxU32 sumAll = 0;
    mfxI32 y = 0;

    for (y = 0; y < (mfxI32)dstHeight; y++) {
        correction = (y % 2) & need_correction;
        for (mfxI32 x = 0; x < (mfxI32)dstWidth; x++) {

            pmfxU8 ps = pSrc + ((y * step_h + correction) * srcPitch) + (x * step_w);
            pmfxU8 pd = pDst + (y * dstPitch) + x;

            pd[0] = ps[0];
            sumAll += ps[0];
        }
    }
    avgLuma = (mfxI16)(sumAll >> 13);
}

mfxStatus ASC::RsCsCalc() {
    ASCYUV
         *pFrame = &m_videoData[ASCCurrent_Frame]->layer.Image;
    ASCImDetails
        vidCar = m_dataIn->layer[0];
    pmfxU8
        ss = pFrame->Y;
    mfxU32
        hblocks = (pFrame->height >> BLOCK_SIZE_SHIFT) /*- 2*/,
        wblocks = (pFrame->width >> BLOCK_SIZE_SHIFT) /*- 2*/;

    mfxI16
        diff = m_videoData[ASCReference_Frame]->layer.avgval - m_videoData[ASCCurrent_Frame]->layer.avgval;
    ss = m_videoData[ASCReference_Frame]->layer.Image.Y;
    if (!m_support->firstFrame && abs(diff) >= GAINDIFF_THR) {
        if (m_support->gainCorrection.Image.Y == nullptr)
            return MFX_ERR_MEMORY_ALLOC;
        GainOffset(&ss, &m_support->gainCorrection.Image.Y, (mfxU16)vidCar._cwidth, (mfxU16)vidCar._cheight, (mfxU16)vidCar.Extended_Width, diff);
    }
    ss = m_videoData[ASCCurrent_Frame]->layer.Image.Y;

    RsCsCalc_4x4(ss, pFrame->pitch, wblocks, hblocks, m_videoData[ASCCurrent_Frame]->layer.Rs, m_videoData[ASCCurrent_Frame]->layer.Cs);
    RsCsCalc_bound(m_videoData[ASCCurrent_Frame]->layer.Rs, m_videoData[ASCCurrent_Frame]->layer.Cs, m_videoData[ASCCurrent_Frame]->layer.RsCs, &m_videoData[ASCCurrent_Frame]->layer.RsVal, &m_videoData[ASCCurrent_Frame]->layer.CsVal, wblocks, hblocks);
    return MFX_ERR_NONE;
}

bool Hint_LTR_op_on(mfxU32 SC, mfxU32 TSC) {
    bool ltr = TSC *TSC < (std::max(SC, 64u) / 12);
    return ltr;
}

mfxI32 ASC::ShotDetect(ASCimageData& Data, ASCimageData& DataRef, ASCImDetails& imageInfo, ASCTSCstat *current, ASCTSCstat *reference, mfxU8 controlLevel) {
    pmfxU8
        ssFrame = Data.Image.Y,
        refFrame = DataRef.Image.Y;
    pmfxU16
        objRs = Data.Rs,
        objCs = Data.Cs,
        refRs = DataRef.Rs,
        refCs = DataRef.Cs;

    current->RsCsDiff = 0;
    current->Schg = -1;
    current->Gchg = 0;

    RsCsCalc_diff(objRs, objCs, refRs, refCs, 2*imageInfo.Width_in_blocks, 2*imageInfo.Height_in_blocks, &current->RsDiff, &current->CsDiff);
    ImageDiffHistogram(ssFrame, refFrame, imageInfo.Extended_Width, imageInfo._cwidth, imageInfo._cheight, current->histogram, &current->ssDCint, &current->refDCint);

    if(reference->Schg)
        current->last_shot_distance = 1;
    else
        current->last_shot_distance++;

    current->RsDiff >>= 9;
    current->CsDiff >>= 9;
    current->RsCsDiff      = (current->RsDiff*current->RsDiff)  + (current->CsDiff*current->CsDiff);
    current->ssDCval       = (mfxI32)current->ssDCint >> 13;
    current->refDCval      = (mfxI32)current->refDCint >> 13;
    current->gchDC         = NABS(current->ssDCval - current->refDCval);
    current->posBalance    = (current->histogram[3] + current->histogram[4]) >> 6;
    current->negBalance    = (current->histogram[0] + current->histogram[1]) >> 6;
    current->diffAFD       = current->AFD - reference->AFD;
    current->diffTSC       = current->TSC - reference->TSC;
    current->diffRsCsDiff  = current->RsCsDiff - reference->RsCsDiff;
    current->diffMVdiffVal = current->MVdiffVal - reference->MVdiffVal;
    mfxI32
        SChange = SCDetectRF(
            current->diffMVdiffVal, current->RsCsDiff,   current->MVdiffVal,
            current->Rs,            current->AFD,        current->CsDiff,
            current->diffTSC,       current->TSC,        current->gchDC,
            current->diffRsCsDiff,  current->posBalance, current->SC,
            current->TSCindex,      current->SCindex,    current->Cs,
            current->diffAFD,       current->negBalance, current->ssDCval,
            current->refDCval,      current->RsDiff,     controlLevel);

    current->ltr_flag = Hint_LTR_op_on(current->SC, current->TSC);
    return SChange;
}

void ASC::MotionAnalysis(ASCVidSample *videoIn, ASCVidSample *videoRef, mfxU32 *TSC, mfxU16 *AFD, mfxU32 *MVdiffVal, mfxU32 *AbsMVSize, mfxU32 *AbsMVHSize, mfxU32 *AbsMVVSize, ASCLayers lyrIdx) {
    mfxU32//24bit is enough
        valb = 0;
    mfxU32
        acc = 0;
    /*--Motion Estimation--*/
    *MVdiffVal = 0;
    *AbsMVSize = 0;
    *AbsMVHSize = 0;
    *AbsMVVSize = 0;
    mfxI16
        diff = (int)videoIn->layer.avgval - (int)videoRef->layer.avgval;

    ASCimageData
        *referenceImageIn = &videoRef->layer;

    if (abs(diff) >= GAINDIFF_THR) {
        referenceImageIn = &m_support->gainCorrection;
    }
    m_support->average = 0;
    videoIn->layer.var = 0;
    videoIn->layer.jtvar = 0;
    videoIn->layer.mcjtvar = 0;
    for (mfxU16 i = 0; i < m_dataIn->layer[lyrIdx].Height_in_blocks; i++) {
        mfxU16 prevFPos = i << 4;
        for (mfxU16 j = 0; j < m_dataIn->layer[lyrIdx].Width_in_blocks; j++) {
            mfxU16 fPos = prevFPos + j;
            acc += ME_simple(m_support, fPos, m_dataIn->layer, &videoIn->layer, referenceImageIn, true, m_dataIn, ME_SAD_8x8_Block_Search, ME_SAD_8x8_Block, ME_VAR_8x8_Block);
            valb += videoIn->layer.SAD[fPos];
            *MVdiffVal += (videoIn->layer.pInteger[fPos].x - videoRef->layer.pInteger[fPos].x) * (videoIn->layer.pInteger[fPos].x - videoRef->layer.pInteger[fPos].x);
            *MVdiffVal += (videoIn->layer.pInteger[fPos].y - videoRef->layer.pInteger[fPos].y) * (videoIn->layer.pInteger[fPos].y - videoRef->layer.pInteger[fPos].y);
            *AbsMVHSize += (videoIn->layer.pInteger[fPos].x * videoIn->layer.pInteger[fPos].x);
            *AbsMVVSize += (videoIn->layer.pInteger[fPos].y * videoIn->layer.pInteger[fPos].y);
            *AbsMVSize += (videoIn->layer.pInteger[fPos].x * videoIn->layer.pInteger[fPos].x) + (videoIn->layer.pInteger[fPos].y * videoIn->layer.pInteger[fPos].y);

        }
    }
    videoIn->layer.var = videoIn->layer.var * 10 / 128 / 64;
    videoIn->layer.jtvar = videoIn->layer.jtvar * 10 / 128 / 64;
    videoIn->layer.mcjtvar = videoIn->layer.mcjtvar * 10 / 128 / 64;
    if (videoIn->layer.var == 0)
    {
        if (videoIn->layer.jtvar == 0)
            videoIn->layer.tcor = 100;
        else
            videoIn->layer.tcor = (mfxI16)NMIN(1000 * videoIn->layer.jtvar, 2000);

        if (videoIn->layer.mcjtvar == 0)
            videoIn->layer.mcTcor = 100;
        else
            videoIn->layer.mcTcor = (mfxI16)NMIN(1000 * videoIn->layer.mcjtvar, 2000);
    }
    else
    {
        videoIn->layer.tcor = (mfxI16)(100 * videoIn->layer.jtvar / videoIn->layer.var);
        videoIn->layer.mcTcor = (mfxI16)(100 * videoIn->layer.mcjtvar / videoIn->layer.var);
    }
    *TSC = valb >> 8;
    *AFD = (mfxU16)(acc >> 13);//Picture area is 2^13, and 10 have been done before so it needs to shift 3 more.
    *MVdiffVal = *MVdiffVal >> 7;
}

mfxU32 TableLookUp(mfxU32 limit, mfxU32 *table, mfxU32 comparisonValue) {
    for (mfxU32 pos = 0; pos < limit; pos++) {
        if (comparisonValue < table[pos])
            return pos;
    }
    return limit;
}

void CorrectionForGoPSize(ASCVidRead *m_support, mfxU32 PdIndex) {
    m_support->detectedSch = 0;
    if(m_support->logic[PdIndex]->Schg) {
        if(m_support->lastSCdetectionDistance % m_support->gopSize)
            m_support->pendingSch = 1;
        else {
            m_support->lastSCdetectionDistance = 0;
            m_support->pendingSch = 0;
            m_support->detectedSch = 1;
        }
    }
    else if(m_support->pendingSch) {
        if(!(m_support->lastSCdetectionDistance % m_support->gopSize)) {
            m_support->lastSCdetectionDistance = 0;
            m_support->pendingSch = 0;
            m_support->detectedSch = 1;
        }
    }
    m_support->lastSCdetectionDistance++;
}

bool ASC::CompareStats(mfxU8 current, mfxU8 reference) {
    if (current > 2 || reference > 2 || current == reference) {
        ASC_PRINTF("Error: Invalid stats comparison\n");
        assert(!"Error: Invalid stats comparison");
    }
    mfxU8 comparison = 0;
    if (m_dataIn->interlaceMode == ASCprogressive_frame) {
        comparison += m_support->logic[current]->AFD == 0;
        comparison += m_support->logic[current]->RsCsDiff == 0;
        comparison += m_support->logic[current]->TSCindex == 0;
        comparison += m_support->logic[current]->negBalance <= 3;
        comparison += m_support->logic[current]->posBalance <= 20;
        comparison += ((m_support->logic[current]->diffAFD <= 0) && (m_support->logic[current]->diffTSC <= 0));
        comparison += (m_support->logic[current]->diffAFD <= m_support->logic[current]->diffTSC);

        if (comparison == 7)
            return Same;
    }
    else if ((m_dataIn->interlaceMode == ASCbotfieldFirst_frame) || (m_dataIn->interlaceMode == ASCtopfieldfirst_frame)) {
        comparison += m_support->logic[current]->AFD == m_support->logic[current]->TSC;
        comparison += m_support->logic[current]->AFD <= 9;
        comparison += m_support->logic[current]->gchDC <= 1;
        comparison += m_support->logic[current]->RsCsDiff <= 9;
        comparison += ((m_support->logic[current]->diffAFD <= 1) && (m_support->logic[current]->diffTSC <= 1));
        comparison += (m_support->logic[current]->diffAFD <= m_support->logic[current]->diffTSC);

        if (comparison == 6)
            return Same;
    }
    else {
        ASC_PRINTF("Error: Invalid interlace mode for stats comparison\n");
        assert(!"Error: Invalid interlace mode for stats comparison\n");
    }

    return Not_same;
}

bool ASC::DenoiseIFrameRec() {
    bool
        result = false;
    mfxF64
        c1 = 10.24346,
        c0 = -11.5751,
        x = (mfxF64)m_support->logic[ASCcurrent_frame_data]->SC,
        y = (mfxF64)m_support->logic[ASCcurrent_frame_data]->avgVal;
    result = ((c1 * std::log(x)) + c0) >= y;
    return result;
}

bool ASC::FrameRepeatCheck() {
    mfxU8 reference = ASCprevious_frame_data;
    if (m_dataIn->interlaceMode > ASCprogressive_frame)
        reference = ASCprevious_previous_frame_data;
    return(CompareStats(ASCcurrent_frame_data, reference));
}

bool ASC::DoMCTFFilteringCheck() {
    return true;
}

void ASC::DetectShotChangeFrame() {
    m_support->logic[ASCcurrent_frame_data]->frameNum   = m_videoData[ASCCurrent_Frame]->frame_number;
    m_support->logic[ASCcurrent_frame_data]->firstFrame = m_support->firstFrame;
    m_support->logic[ASCcurrent_frame_data]->avgVal     = m_videoData[ASCCurrent_Frame]->layer.avgval;
    /*---------RsCs data--------*/
    m_support->logic[ASCcurrent_frame_data]->Rs = m_videoData[ASCCurrent_Frame]->layer.RsVal;
    m_support->logic[ASCcurrent_frame_data]->Cs = m_videoData[ASCCurrent_Frame]->layer.CsVal;
    m_support->logic[ASCcurrent_frame_data]->SC = m_videoData[ASCCurrent_Frame]->layer.RsVal + m_videoData[ASCCurrent_Frame]->layer.CsVal;
    m_support->logic[ASCcurrent_frame_data]->doFilter_flag    = DoMCTFFilteringCheck();
    m_support->logic[ASCcurrent_frame_data]->filterIntra_flag = DenoiseIFrameRec();
    if (m_support->firstFrame) {
        m_support->logic[ASCcurrent_frame_data]->TSC                = 0;
        m_support->logic[ASCcurrent_frame_data]->AFD                = 0;
        m_support->logic[ASCcurrent_frame_data]->TSCindex           = 0;
        m_support->logic[ASCcurrent_frame_data]->SCindex            = 0;
        m_support->logic[ASCcurrent_frame_data]->Schg               = 0;
        m_support->logic[ASCcurrent_frame_data]->Gchg               = 0;
        m_support->logic[ASCcurrent_frame_data]->picType            = 0;
        m_support->logic[ASCcurrent_frame_data]->lastFrameInShot    = 0;
        m_support->logic[ASCcurrent_frame_data]->pdist              = 0;
        m_support->logic[ASCcurrent_frame_data]->MVdiffVal          = 0;
        m_support->logic[ASCcurrent_frame_data]->RsCsDiff           = 0;
        m_support->logic[ASCcurrent_frame_data]->last_shot_distance = 0;
        m_support->logic[ASCcurrent_frame_data]->tcor               = 0;
        m_support->logic[ASCcurrent_frame_data]->mcTcor             = 0;
        m_support->firstFrame = false;
    }
    else {
        /*--------Motion data-------*/
        MotionAnalysis(m_videoData[ASCCurrent_Frame], m_videoData[ASCReference_Frame], &m_support->logic[ASCcurrent_frame_data]->TSC, &m_support->logic[ASCcurrent_frame_data]->AFD, &m_support->logic[ASCcurrent_frame_data]->MVdiffVal, &m_support->logic[ASCcurrent_frame_data]->AbsMVSize, &m_support->logic[ASCcurrent_frame_data]->AbsMVHSize, &m_support->logic[ASCcurrent_frame_data]->AbsMVVSize, (ASCLayers)0);
        m_support->logic[ASCcurrent_frame_data]->TSCindex = TableLookUp(NumTSC, lmt_tsc2, m_support->logic[ASCcurrent_frame_data]->TSC);
        m_support->logic[ASCcurrent_frame_data]->SCindex  = TableLookUp(NumSC, lmt_sc2, m_support->logic[ASCcurrent_frame_data]->SC);
        m_support->logic[ASCcurrent_frame_data]->pdist    = m_support->PDistanceTable[(m_support->logic[ASCcurrent_frame_data]->TSCindex * NumSC) +
                                                          m_support->logic[ASCcurrent_frame_data]->SCindex];
        m_support->logic[ASCcurrent_frame_data]->TSC >>= 5;
        m_support->logic[ASCcurrent_frame_data]->tcor = m_videoData[ASCCurrent_Frame]->layer.tcor;
        m_support->logic[ASCcurrent_frame_data]->mcTcor = m_videoData[ASCCurrent_Frame]->layer.mcTcor;
        /*------Shot Detection------*/
        m_support->logic[ASCcurrent_frame_data]->Schg = ShotDetect(m_videoData[ASCCurrent_Frame]->layer, m_videoData[ASCReference_Frame]->layer, *m_dataIn->layer, m_support->logic[ASCcurrent_frame_data], m_support->logic[ASCprevious_frame_data], m_support->control);
        m_support->logic[ASCprevious_frame_data]->lastFrameInShot = (mfxU8)m_support->logic[ASCcurrent_frame_data]->Schg;
        m_support->logic[ASCcurrent_frame_data]->repeatedFrame = FrameRepeatCheck();
    }
    m_dataIn->processed_frames++;
}

/**
***********************************************************************
* \Brief Adds LTR friendly frame decision to list
*
* Adds frame number and ltr friendly frame decision pair to list, but
* first checks if the size of the list is same or less to MAXLTRHISTORY,
* if the list is longer, then it removes the top elements of the list
* until it is MAXLTRHISTORY - 1, then it adds the new pair to the bottom
* of the list.
*
* \return none
*/
void ASC::Put_LTR_Hint() {
    mfxI16
        list_size = (mfxI16)ltr_check_history.size();
    for (mfxI16 i = 0; i < list_size - (MAXLTRHISTORY - 1); i++)
        ltr_check_history.pop_front();
    ltr_check_history.push_back(std::make_pair(m_videoData[ASCCurrent_Frame]->frame_number, m_support->logic[ASCcurrent_frame_data]->ltr_flag));
}

/**
***********************************************************************
* \Brief Checks LTR friendly decision history per frame and returns if
*        LTR operation should be turn on or off.
*
* Travels the LTR friendly decision list backwards checking for frequency
* and amount of true/false LTR friendly frame decision, based on the
* good and bad limit inputs, if bad limit condition is reached first,
* then it inmediately returns 0 (zero) which means to stop LTR operation
*
* \param goodLTRLimit      [IN] - Amount of true values to determine
*                                 if the sequence should run in LTR mode.
* \param badLTRLimit       [IN] - Amount of consecutive false values to
*                                 stop LTR mode.
*
* \return ASC_LTR_DEC to flag stop(false)/continue(true) or FORCE LTR operation
*/
ASC_LTR_DEC ASC::Continue_LTR_Mode(mfxU16 goodLTRLimit, mfxU16 badLTRLimit) {
    size_t
        goodLTRCounter = 0,
        goodLTRRelativeCount = 0,
        badLTRCounter = 0,
        list_size = ltr_check_history.size();
    std::list<std::pair<mfxI32, bool> >::iterator
        ltr_list_it = std::prev(ltr_check_history.end());
    goodLTRLimit = goodLTRLimit > MAXLTRHISTORY ? MAXLTRHISTORY : goodLTRLimit;
    //When a scene change happens, all history is discarded
    if (Get_frame_shot_Decision()) {
        ltr_check_history.resize(0);
        list_size = 0;
    }
    //If not enough history then let it be LTR
    if (list_size < badLTRLimit)
        return YES_LTR;
    //Travel trhough the list to determine if LTR operation should be kept on
    mfxU16
        bkp_size = (mfxU16)list_size;
    while ((bkp_size > 1) && (goodLTRCounter < goodLTRLimit)) {
        auto scd = ltr_list_it->second;
        if (!scd) {
            badLTRCounter++;
            goodLTRRelativeCount = 0;
        }
        if (badLTRCounter >= badLTRLimit)
            return NO_LTR;
        goodLTRCounter += (mfxU16)ltr_list_it->second;
        goodLTRRelativeCount += (mfxU16)ltr_list_it->second;
        if (goodLTRRelativeCount >= badLTRLimit)
            badLTRCounter = 0;
        ltr_list_it = std::prev(ltr_list_it);
        bkp_size--;
    }
    if (goodLTRCounter >= goodLTRLimit)
        return FORCE_LTR;
    else if (goodLTRRelativeCount >= (size_t)NMIN(badLTRLimit, list_size - 1) && badLTRCounter < goodLTRRelativeCount)
        return YES_LTR;
    else
        return NO_LTR;
}

mfxStatus ASC::SetKernel(SurfaceIndex *idxFrom, SurfaceIndex *idxTo, CmTask **subSamplingTask, mfxU32 parity) {
    mfxU32 argIdx = 0;
    INT res;
    //Progressive Point subsampling kernel

    CmKernel
        **subKernel = nullptr;

    if (m_dataIn->interlaceMode == ASCprogressive_frame) {
        subKernel = &m_kernel_p;
    }
    else {
        if (parity == ASCTopField)
            subKernel = &m_kernel_t;
        else if (parity == ASCBottomField)
            subKernel = &m_kernel_b;
        else
            return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    int
        tmp_subWidth = subWidth,
        tmp_subHeight = subHeight;

    res = (*subKernel)->SetKernelArg(argIdx++, sizeof(SurfaceIndex), idxFrom);
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = (*subKernel)->SetKernelArg(argIdx++, sizeof(SurfaceIndex), idxTo);
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = (*subKernel)->SetKernelArg(argIdx++, sizeof(int), &m_gpuwidth);
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = (*subKernel)->SetKernelArg(argIdx++, sizeof(int), &m_gpuheight);
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = (*subKernel)->SetKernelArg(argIdx++, sizeof(int), &tmp_subWidth);
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = (*subKernel)->SetKernelArg(argIdx++, sizeof(int), &tmp_subHeight);
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    /*if (*subSamplingTask)
    res = (*subSamplingTask)->Reset();
    else
    */res = m_device->CreateTask(*subSamplingTask);
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = (*subSamplingTask)->AddKernel((*subKernel));
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);

    return MFX_ERR_NONE;
}

mfxStatus ASC::SetKernel(SurfaceIndex *idxFrom, CmTask **subSamplingTask, mfxU32 parity) {
    return (SetKernel(idxFrom, m_videoData[ASCCurrent_Frame]->layer.idxImage, subSamplingTask, parity));
}

mfxStatus ASC::SetKernel(SurfaceIndex *idxFrom, mfxU32 parity) {
    return(SetKernel(idxFrom, &m_task, parity));
}

#ifndef CMRT_EMU
mfxStatus ASC::QueueFrame(SurfaceIndex *idxFrom, SurfaceIndex *idxTo, CmEvent **subSamplingEv, CmTask **subSamplingTask, mfxU32 parity)
#else
mfxStatus ASC::QueueFrame(SurfaceIndex *idxFrom, SurfaceIndex *idxTo, CmEvent **subSamplingEv, CmTask **subSamplingTask, CmThreadSpace *subThreadSpace, mfxU32 parity)
#endif
{
if (!m_ASCinitialized)
return MFX_ERR_NOT_INITIALIZED;
*subSamplingEv = NULL;// CM_NO_EVENT;
INT res;
res = SetKernel(idxFrom, idxTo, subSamplingTask, parity);
SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
#ifndef CMRT_EMU
res = m_queue->Enqueue(*subSamplingTask, *subSamplingEv);
#else
res = m_queue->Enqueue(*subSamplingTask, *subSamplingEv, subThreadSpace);
#endif
SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
return MFX_ERR_NONE;
}

#ifndef CMRT_EMU
mfxStatus ASC::QueueFrame(SurfaceIndex *idxFrom, CmEvent **subSamplingEv, CmTask **subSamplingTask, mfxU32 parity)
#else
mfxStatus ASC::QueueFrame(SurfaceIndex *idxFrom, CmEvent **subSamplingEv, CmTask **subSamplingTask, CmThreadSpace *subThreadSpace, mfxU32 parity)
#endif
{
#ifndef CMRT_EMU
    return(QueueFrame(idxFrom, m_videoData[ASCCurrent_Frame]->layer.idxImage, subSamplingEv, subSamplingTask, parity));
#else
    return(QueueFrame(idxFrom, m_videoData[ASCCurrent_Frame]->layer.idxImage, subSamplingEv, subSamplingTask, subThreadSpace, parity));
#endif
}

mfxStatus ASC::QueueFrame(SurfaceIndex *idxFrom, mfxU32 parity) {
    return(
#ifndef CMRT_EMU
        QueueFrame(idxFrom, &m_subSamplingEv, &m_task, parity)
#else
        QueueFrame(idxFrom, &m_subSamplingEv, &m_task, m_threadSpace, parity)
#endif
        );
}

void ASC::AscFrameAnalysis() {
    mfxU8
        *ss = m_videoData[ASCCurrent_Frame]->layer.Image.Y;

    mfxU32
        sumAll = 0;
    for (mfxU16 i = 0; i < m_dataIn->layer->_cheight; i++) {
        for (mfxU16 j = 0; j < m_dataIn->layer->_cwidth; j++)
            sumAll += ss[j];
        ss += m_dataIn->layer->Extended_Width;
    }
    sumAll >>= 13;
    m_videoData[ASCCurrent_Frame]->layer.avgval = (mfxU16)sumAll;
    RsCsCalc();
    DetectShotChangeFrame();
    Put_LTR_Hint();
    GeneralBufferRotation();
}

ASC_API bool ASC::Query_resize_Event() {
    return (m_subSamplingEv != nullptr);
}

ASC_API mfxStatus ASC::ProcessQueuedFrame(CmEvent **subSamplingEv, CmTask **subSamplingTask, CmSurface2DUP **inputFrame, mfxU8 **pixelData)
{
    if (!m_ASCinitialized)
        return MFX_ERR_NOT_INITIALIZED;
    INT res;
    if (*subSamplingEv) {
        res = (*subSamplingEv)->WaitForTaskFinished();
        if (res == CM_EXCEED_MAX_TIMEOUT)
            return MFX_ERR_GPU_HANG;
        SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
        if(inputFrame != nullptr && pixelData != nullptr)
            MFX_SAFE_CALL(SwapResources(ASCCurrent_Frame, inputFrame, pixelData))
        AscFrameAnalysis();
        res = m_queue->DestroyEvent(*subSamplingEv);
        SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
        res = m_device->DestroyTask(*subSamplingTask);
        SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
        m_dataReady = (res == CM_SUCCESS);
    }
    else {
        return MFX_ERR_DEVICE_FAILED;
    }
    return MFX_ERR_NONE;
}

ASC_API mfxStatus ASC::ProcessQueuedFrame()
{
    return ProcessQueuedFrame(&m_subSamplingEv, &m_task, nullptr, nullptr);
}

mfxStatus ASC::RunFrame(SurfaceIndex *idxFrom, mfxU32 parity) {
    if (!m_ASCinitialized)
        return MFX_ERR_NOT_INITIALIZED;
    CmEvent* e = NULL;// CM_NO_EVENT;
    INT res;
    res = SetKernel(idxFrom, parity);
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
#ifndef CMRT_EMU
    res = m_queue->Enqueue(m_task, e);
#else
    res = m_queue->Enqueue(m_task, e, m_threadSpace);
#endif
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = e->WaitForTaskFinished();
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = m_device->DestroyTask(m_task);
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = m_queue->DestroyEvent(e);
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);

    AscFrameAnalysis();

    return MFX_ERR_NONE;
}

mfxStatus ASC::QueueFrame(mfxHDL frameHDL, SurfaceIndex *idxTo, CmEvent **subSamplingEv, CmTask **subSamplingTask, mfxU32 parity)
{
    if (!m_ASCinitialized)
        return MFX_ERR_NOT_INITIALIZED;
    m_videoData[ASCCurrent_Frame]->frame_number = m_videoData[ASCReference_Frame]->frame_number + 1;

    CmSurface2D* p_surfaceFrom = nullptr;
    SurfaceIndex* idxFrom = nullptr;

    CreateCmSurface2D(reinterpret_cast<AbstractSurfaceHandle>(frameHDL), p_surfaceFrom, idxFrom);

    mfxStatus sts = QueueFrame(idxFrom, idxTo, subSamplingEv, subSamplingTask, parity);
    SCD_CHECK_MFX_ERR(sts);

    return MFX_ERR_NONE;
}


mfxStatus ASC::QueueFrame(mfxHDL frameHDL, CmEvent **subSamplingEv, CmTask **subSamplingTask, mfxU32 parity)
{
    if (!m_ASCinitialized)
        return MFX_ERR_NOT_INITIALIZED;
    m_videoData[ASCCurrent_Frame]->frame_number = m_videoData[ASCReference_Frame]->frame_number + 1;

    CmSurface2D* p_surfaceFrom = nullptr;
    SurfaceIndex* idxFrom = nullptr;

    CreateCmSurface2D(reinterpret_cast<AbstractSurfaceHandle>(frameHDL), p_surfaceFrom, idxFrom);

    mfxStatus sts = QueueFrame(idxFrom, subSamplingEv, subSamplingTask, parity);
    SCD_CHECK_MFX_ERR(sts);

    return MFX_ERR_NONE;
}

mfxStatus ASC::QueueFrame(mfxHDL frameHDL, mfxU32 parity) {
    if (!m_ASCinitialized)
        return MFX_ERR_NOT_INITIALIZED;
    m_videoData[ASCCurrent_Frame]->frame_number = m_videoData[ASCReference_Frame]->frame_number + 1;

    CmSurface2D* p_surfaceFrom = nullptr;
    SurfaceIndex* idxFrom = nullptr;

    CreateCmSurface2D(reinterpret_cast<AbstractSurfaceHandle>(frameHDL), p_surfaceFrom, idxFrom);

    mfxStatus sts = QueueFrame(idxFrom, parity);
    SCD_CHECK_MFX_ERR(sts);

    return MFX_ERR_NONE;
}

mfxStatus ASC::RunFrame(mfxHDL frameHDL, mfxU32 parity) {
    if (!m_ASCinitialized)
        return MFX_ERR_NOT_INITIALIZED;
    m_videoData[ASCCurrent_Frame]->frame_number = m_videoData[ASCReference_Frame]->frame_number + 1;

    CmSurface2D* p_surfaceFrom = 0;

    SurfaceIndex *idxFrom = nullptr;
    CreateCmSurface2D(reinterpret_cast<AbstractSurfaceHandle>(frameHDL), p_surfaceFrom, idxFrom);

    mfxStatus sts = RunFrame(idxFrom, parity);
    SCD_CHECK_MFX_ERR(sts);

    return MFX_ERR_NONE;
}

mfxStatus ASC::RunFrame(mfxU8 *frame, mfxU32 parity) {
    if (!m_ASCinitialized)
        return MFX_ERR_NOT_INITIALIZED;
    m_videoData[ASCCurrent_Frame]->frame_number = m_videoData[ASCReference_Frame]->frame_number + 1;
    (this->*(resizeFunc))(frame, m_width, m_height, m_pitch, (ASCLayers)0, parity);
    RsCsCalc();
    DetectShotChangeFrame();
    Put_LTR_Hint();
    GeneralBufferRotation();
    return MFX_ERR_NONE;
}

ASC_API mfxStatus ASC::QueueFrameProgressive(SurfaceIndex* idxSurf) {
    mfxStatus sts = QueueFrame(idxSurf, ASCTopField);
    return sts;
}

ASC_API mfxStatus ASC::QueueFrameProgressive(SurfaceIndex* idxSurf, CmEvent *subSamplingEv, CmTask *subSamplingTask) {
    mfxStatus sts = QueueFrame(idxSurf, &subSamplingEv, &subSamplingTask, ASCTopField);
    return sts;
}

ASC_API mfxStatus ASC::QueueFrameProgressive(mfxHDL surface, SurfaceIndex *idxTo, CmEvent **subSamplingEv, CmTask **subSamplingTask)
{
    mfxStatus sts = QueueFrame(surface, idxTo, subSamplingEv, subSamplingTask, ASCTopField);
    return sts;
}

ASC_API mfxStatus ASC::QueueFrameProgressive(mfxHDL surface, CmEvent **subSamplingEv, CmTask **subSamplingTask) {
    mfxStatus sts = QueueFrame(surface, subSamplingEv, subSamplingTask, ASCTopField);
    return sts;
}

ASC_API mfxStatus ASC::QueueFrameProgressive(mfxHDL surface) {
    mfxStatus sts = QueueFrame(surface, ASCTopField);
    return sts;
}

ASC_API mfxStatus ASC::PutFrameProgressive(SurfaceIndex* idxSurf) {
    mfxStatus sts = RunFrame(idxSurf, ASCTopField);
    m_dataReady = (sts == MFX_ERR_NONE);
    return sts;
}

ASC_API mfxStatus ASC::PutFrameProgressive(mfxHDL surface) {
    mfxStatus sts = RunFrame(surface, ASCTopField);
    m_dataReady = (sts == MFX_ERR_NONE);
    return sts;
}

ASC_API mfxStatus ASC::PutFrameProgressive(mfxU8 *frame, mfxI32 Pitch) {
    mfxStatus sts;
    if (Pitch > 0) {
        sts = SetPitch(Pitch);
        SCD_CHECK_MFX_ERR(sts);
    }

    sts = RunFrame(frame, ASCTopField);
    SCD_CHECK_MFX_ERR(sts);
    m_dataReady = (sts == MFX_ERR_NONE);
    return sts;
}

ASC_API mfxStatus ASC::PutFrameInterlaced(mfxU8 *frame, mfxI32 Pitch) {
    mfxStatus sts;

    if (Pitch > 0) {
        sts = SetPitch(Pitch);
        SCD_CHECK_MFX_ERR(sts);
    }

    sts = RunFrame(frame, m_dataIn->currentField);
    m_dataReady = (sts == MFX_ERR_NONE);
    SetNextField();
    return sts;
}

ASC_API mfxStatus ASC::QueueFrameInterlaced(SurfaceIndex* idxSurf) {
    mfxStatus sts = QueueFrame(idxSurf, m_dataIn->currentField);
    m_dataReady = (sts == MFX_ERR_NONE);
    SetNextField();
    return sts;
}

ASC_API mfxStatus ASC::PutFrameInterlaced(SurfaceIndex* idxSurf) {
    mfxStatus sts = RunFrame(idxSurf, m_dataIn->currentField);
    m_dataReady = (sts == MFX_ERR_NONE);
    SetNextField();
    return sts;
}

ASC_API mfxStatus ASC::QueueFrameInterlaced(mfxHDL surface) {
    mfxStatus sts = QueueFrame(surface, m_dataIn->currentField);
    SetNextField();
    return sts;
}

ASC_API mfxStatus ASC::PutFrameInterlaced(mfxHDL surface) {
    mfxStatus sts = RunFrame(surface, m_dataIn->currentField);
    m_dataReady = (sts == MFX_ERR_NONE);
    SetNextField();
    return sts;
}


ASC_API mfxStatus ASC::calc_RaCa_pic(mfxU8 *pSrc, mfxI32 width, mfxI32 height, mfxI32 pitch, mfxF64 &RsCs) {
    if (!m_ASCinitialized)
        return MFX_ERR_NOT_INITIALIZED;
    return Calc_RaCa_pic(pSrc, width, height, pitch, RsCs);
}

ASC_API mfxStatus ASC::calc_RaCa_Surf(mfxHDL surface, mfxF64 &rscs) {
    if (!Query_ASCCmDevice())
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    mfxStatus sts = CopyFrameSurface(surface);
    SCD_CHECK_MFX_ERR(sts);
    
    sts = calc_RaCa_pic(m_frameBkp, m_gpuwidth, m_gpuheight, m_gpuImPitch, rscs);
    SCD_CHECK_MFX_ERR(sts);

    return sts;
}

mfxStatus ASC::CreateCmSurface2D(void *pSrcD3D, CmSurface2D* & pCmSurface2D, SurfaceIndex* &pCmSrcIndex)
{
    INT cmSts = 0;
    std::map<void *, CmSurface2D *>::iterator it;
    std::map<CmSurface2D *, SurfaceIndex *>::iterator it_idx;
    it = m_tableCmRelations2.find(pSrcD3D);
    if (m_tableCmRelations2.end() == it)
    {
        //UMC::AutomaticUMCMutex guard(m_guard);
        {
            cmSts = m_device->CreateSurface2D((AbstractSurfaceHandle *)pSrcD3D, pCmSurface2D);
            SCD_CHECK_CM_ERR(cmSts, MFX_ERR_DEVICE_FAILED);
            m_tableCmRelations2.insert(std::pair<void *, CmSurface2D *>(pSrcD3D, pCmSurface2D));
        }

        cmSts = pCmSurface2D->GetIndex(pCmSrcIndex);
        SCD_CHECK_CM_ERR(cmSts, MFX_ERR_DEVICE_FAILED);
        m_tableCmIndex2.insert(std::pair<CmSurface2D *, SurfaceIndex *>(pCmSurface2D, pCmSrcIndex));
    }
    else
    {
        pCmSurface2D = it->second;
        it_idx = m_tableCmIndex2.find(pCmSurface2D);
        if (it_idx == m_tableCmIndex2.end())
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        else
            pCmSrcIndex = it_idx->second;
    }

    return MFX_ERR_NONE;
}

mfxStatus ASC::CopyFrameSurface(mfxHDL frameHDL) {
    if (!m_ASCinitialized)
        return MFX_ERR_NOT_INITIALIZED;
    CmSurface2D* p_surfaceFrom = 0;
    mfxStatus sts;
    INT res;
    SurfaceIndex *idxFrom;

    sts = CreateCmSurface2D(reinterpret_cast<AbstractSurfaceHandle>(frameHDL), p_surfaceFrom, idxFrom);
    SCD_CHECK_MFX_ERR(sts);

    m_frameCopyEv = NULL;// CM_NO_EVENT;
    mfxU32 argIdx = 0;
    //Copy pixels kernel
    res = m_kernel_cp->SetKernelArg(argIdx++, sizeof(SurfaceIndex), idxFrom);
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = m_kernel_cp->SetKernelArg(argIdx++, sizeof(SurfaceIndex), m_pIdxSurfCp);
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);

    mfxU32
        width_dword = (UINT)ceil((double)m_gpuwidth / 4);
    res = m_kernel_cp->SetKernelArg(argIdx++, sizeof(mfxU32), &width_dword);
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = m_kernel_cp->SetKernelArg(argIdx++, sizeof(mfxI32), &m_gpuheight);
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = m_kernel_cp->SetKernelArg(argIdx++, sizeof(mfxU32), &m_gpuImPitch);
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);

    res = m_device->CreateTask(m_taskCp);
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = m_taskCp->AddKernel(m_kernel_cp);      //progressive
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
#ifndef CMRT_EMU
    res = m_queue->Enqueue(m_taskCp, m_frameCopyEv);
#else
    res = m_queue->Enqueue(m_taskCp, m_frameCopyEv, m_threadSpaceCp);
#endif
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = m_frameCopyEv->WaitForTaskFinished();
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = m_device->DestroyTask(m_taskCp);
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    res = m_queue->DestroyEvent(m_frameCopyEv);
    SCD_CHECK_CM_ERR(res, MFX_ERR_DEVICE_FAILED);
    return sts;
}

ASC_API bool ASC::Get_Last_frame_Data() {
    if(m_dataReady)
        GeneralBufferRotation();
    else
        ASC_PRINTF("Warning: Trying to grab data not ready\n");
    return(m_dataReady);
}

ASC_API mfxU16 ASC::Get_asc_subsampling_width()
{
    return mfxU16(subWidth);
}

ASC_API mfxU16 ASC::Get_asc_subsampling_height()
{
    return mfxU16(subHeight);
}

ASC_API mfxU32 ASC::Get_starting_frame_number() {
    return m_dataIn->starting_frame;
}

ASC_API mfxU32 ASC::Get_frame_number() {
    if(m_dataReady)
        return m_support->logic[ASCprevious_frame_data]->frameNum;
    else
        return 0;
}

ASC_API mfxU32 ASC::Get_frame_shot_Decision() {
    if(m_dataReady)
        return m_support->logic[ASCprevious_frame_data]->Schg;
    else
        return 0;
}

ASC_API mfxU32 ASC::Get_frame_last_in_scene() {
    if(m_dataReady)
        return m_support->logic[ASCprevious_frame_data]->lastFrameInShot;
    else
        return 0;
}

ASC_API bool ASC::Get_GoPcorrected_frame_shot_Decision() {
    if(m_dataReady)
        return (m_support->detectedSch > 0);
    else
        return 0;
}
ASC_API mfxI32 ASC::Get_frame_Spatial_complexity() {
    if(m_dataReady)
        return m_support->logic[ASCprevious_frame_data]->SCindex;
    else
        return 0;
}

ASC_API mfxI32 ASC::Get_frame_Temporal_complexity() {
    if(m_dataReady)
        return m_support->logic[ASCprevious_frame_data]->TSCindex;
    else
        return 0;
}

ASC_API bool ASC::Get_intra_frame_denoise_recommendation() {
    return m_support->logic[ASCprevious_frame_data]->filterIntra_flag;
}

ASC_API mfxU32 ASC::Get_PDist_advice() {
    if (m_dataReady)
        return m_support->logic[ASCprevious_frame_data]->pdist;
    else
        return NULL;
}

ASC_API bool ASC::Get_LTR_advice() {
    if (m_dataReady)
        return m_support->logic[ASCprevious_frame_data]->ltr_flag;
    else
        return NULL;
}

ASC_API bool ASC::Get_RepeatedFrame_advice() {
    if (m_dataReady)
        return m_support->logic[ASCprevious_frame_data]->repeatedFrame;
    else
        return NULL;
}
ASC_API bool ASC::Get_Filter_advice() {
    return m_support->logic[ASCprevious_frame_data]->doFilter_flag;
}

/**
***********************************************************************
* \Brief Tells if LTR mode should be on/off or forced on.
*
* \return  ASC_LTR_DEC& to flag stop(false)/continue(true) or force (2)
*          LTR operation
*/
ASC_API mfxStatus ASC::get_LTR_op_hint(ASC_LTR_DEC& scd_LTR_hint) {
    if (!m_ASCinitialized)
        return MFX_ERR_NOT_INITIALIZED;
    scd_LTR_hint = Continue_LTR_Mode(50, 5);
    return MFX_ERR_NONE;
}

ASC_API bool ASC::Check_last_frame_processed(mfxU32 frameOrder) {
    if (m_support->frameOrder <= frameOrder && (m_support->frameOrder == frameOrder && frameOrder > 0))
        return 0;
    else
        m_support->frameOrder = frameOrder;
    return 1;
}

ASC_API void ASC::Reset_last_frame_processed() {
    m_support->frameOrder = 0;
}

ASC_API mfxI32 ASC::Get_CpuFeature_AVX2() {
    return CpuFeature_AVX2();
}
ASC_API mfxI32 ASC::Get_CpuFeature_SSE41() {
    return CpuFeature_SSE41();
}

void bufferRotation(void *Buffer1, void *Buffer2) {
    void
        *transfer;
    transfer = Buffer2;
    Buffer2  = Buffer1;
    Buffer1  = transfer;
}

void ASC::GeneralBufferRotation() {
    ASCVidSample
        *videoTransfer;
    ASCTSCstat
        *metaTransfer;

    if (m_support->logic[ASCcurrent_frame_data]->repeatedFrame) {
        m_videoData[ASCReference_Frame]->frame_number = m_videoData[ASCCurrent_Frame]->frame_number;
        m_support->logic[ASCprevious_frame_data]->frameNum = m_support->logic[ASCcurrent_frame_data]->frameNum;
        m_support->logic[ASCcurrent_frame_data]->Schg = 0;
        m_support->logic[ASCprevious_frame_data]->Schg = 0;
        m_support->logic[ASCprevious_frame_data]->repeatedFrame = true;
        m_support->logic[ASCprevious_previous_frame_data]->Schg = 0;
    }
    else {
        videoTransfer = m_videoData[0];
        m_videoData[0] = m_videoData[1];
        m_videoData[1] = videoTransfer;

        metaTransfer = m_support->logic[ASCprevious_previous_frame_data];
        m_support->logic[ASCprevious_previous_frame_data] = m_support->logic[ASCprevious_frame_data];
        m_support->logic[ASCprevious_frame_data] = m_support->logic[ASCcurrent_frame_data];
        m_support->logic[ASCcurrent_frame_data] = metaTransfer;
    }
}
}