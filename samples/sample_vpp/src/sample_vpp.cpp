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
#include "sample_vpp_pts.h"
#include "sample_vpp_roi.h"
#include "vm/atomic_defs.h"

#define TIME_STATS 1
#include "time_statistics.h"

#define MFX_CHECK(sts) {if (sts != MFX_ERR_NONE) return sts;}

#ifndef MFX_VERSION
#error MFX_VERSION not defined
#endif

using namespace std;

void IncreaseReference(mfxFrameData *ptr)
{
    msdk_atomic_inc16((volatile mfxU16*)&ptr->Locked);
}

void DecreaseReference(mfxFrameData *ptr)
{
    msdk_atomic_dec16((volatile mfxU16*)&ptr->Locked);
}

void PutPerformanceToFile(sInputParams& Params, mfxF64 FPS)
{
    FILE *fPRF;
    MSDK_FOPEN(fPRF,Params.strPerfFile, MSDK_STRING("ab"));
    if (!fPRF)
        return;

    msdk_char *iopattern = const_cast<msdk_char*>(IOpattern2Str(Params.IOPattern));
    msdk_char   iopattern_ascii[32] = {0};
    msdk_char  *pIOP = iopattern_ascii;
    while (*iopattern)
        *pIOP++ = (msdk_char)*iopattern++;
    msdk_char *srcFileName = Params.strSrcFile;
    msdk_char   srcFileName_ascii[1024] = {0};
    msdk_char  *pFileName = srcFileName_ascii;
    while (*srcFileName)
        *pFileName++ = (msdk_char)*srcFileName++;


    msdk_tstring filters;

    if (Params.frameInfoIn[0].PicStruct != Params.frameInfoOut[0].PicStruct)
    {
        filters+=MSDK_STRING("DI ");
    }
    if (VPP_FILTER_DISABLED != Params.denoiseParam[0].mode)
    {
        filters+=MSDK_STRING("DN ");
    }
    if (filters.empty())
    {
        filters=MSDK_STRING("NoFilters ");
    }


    msdk_fprintf(fPRF, MSDK_STRING("%s, %dx%d, %dx%d, %s, %s, %f\r\n"), srcFileName_ascii,
        Params.frameInfoIn[0].nWidth,
        Params.frameInfoIn[0].nHeight,
        Params.frameInfoOut[0].nWidth,
        Params.frameInfoOut[0].nHeight,
        iopattern_ascii,
        filters.c_str(),
        FPS);
    fclose(fPRF);

} // void PutPerformanceToFile(sInputVppParams& Params, mfxF64 FPS)

static
    void vppDefaultInitParams( sInputParams* pParams, sFiltersParam* pDefaultFiltersParam )
{
    pParams->frameInfoIn.clear();  pParams->frameInfoIn.push_back ( *pDefaultFiltersParam->pOwnFrameInfo );
    pParams->frameInfoOut.clear(); pParams->frameInfoOut.push_back( *pDefaultFiltersParam->pOwnFrameInfo );

    pParams->IOPattern    = MFX_IOPATTERN_IN_SYSTEM_MEMORY|MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
    pParams->ImpLib       = MFX_IMPL_HARDWARE;
    pParams->asyncNum     = 1;
    pParams->bPerf       = false;
    pParams->isOutput     = false;
    pParams->ptsCheck     = false;
    pParams->ptsAdvanced  = false;
    pParams->ptsFR        = 0;
    pParams->vaType       = ALLOC_IMPL_VIA_SYS;
    pParams->rotate.clear(); pParams->rotate.push_back(0);
    pParams->bScaling     = false;
    pParams->scalingMode  = MFX_SCALING_MODE_DEFAULT;
    pParams->interpolationMethod = MFX_INTERPOLATION_DEFAULT;
    pParams->bChromaSiting = false;
    pParams->uChromaSiting = 0;
    pParams->numFrames    = 0;
    pParams->fccSource = MFX_FOURCC_NV12;

    // Optional video processing features
    pParams->mirroringParam.clear();        pParams->mirroringParam.push_back(      *pDefaultFiltersParam->pMirroringParam      );
    pParams->videoSignalInfoParam.clear();  pParams->videoSignalInfoParam.push_back(*pDefaultFiltersParam->pVideoSignalInfo     );
    pParams->deinterlaceParam.clear();      pParams->deinterlaceParam.push_back(    *pDefaultFiltersParam->pDIParam             );
    pParams->denoiseParam.clear();          pParams->denoiseParam.push_back(        *pDefaultFiltersParam->pDenoiseParam        );
#ifdef ENABLE_MCTF
    pParams->mctfParam.clear();             pParams->mctfParam.push_back(           *pDefaultFiltersParam->pMctfParam           );
#endif
    pParams->detailParam.clear();           pParams->detailParam.push_back(         *pDefaultFiltersParam->pDetailParam         );
    pParams->procampParam.clear();          pParams->procampParam.push_back(        *pDefaultFiltersParam->pProcAmpParam        );
    // analytics
    pParams->frcParam.clear();              pParams->frcParam.push_back(            *pDefaultFiltersParam->pFRCParam            );
    // MSDK 3.0
    pParams->multiViewParam.clear();        pParams->multiViewParam.push_back(      *pDefaultFiltersParam->pMultiViewParam      );
    // MSDK API 1.5
    pParams->gamutParam.clear();            pParams->gamutParam.push_back(          *pDefaultFiltersParam->pGamutParam          );
    pParams->tccParam.clear();              pParams->tccParam.push_back(            *pDefaultFiltersParam->pClrSaturationParam  );
    pParams->aceParam.clear();              pParams->aceParam.push_back(            *pDefaultFiltersParam->pContrastParam       );
    pParams->steParam.clear();              pParams->steParam.push_back(            *pDefaultFiltersParam->pSkinParam           );
    pParams->istabParam.clear();            pParams->istabParam.push_back(          *pDefaultFiltersParam->pImgStabParam        );

    pParams->colorfillParam.clear();        pParams->colorfillParam.push_back(      *pDefaultFiltersParam->pColorfillParam      );

    // ROI check
    pParams->roiCheckParam.mode = ROI_FIX_TO_FIX; // ROI check is disabled
    pParams->roiCheckParam.srcSeed = 0;
    pParams->roiCheckParam.dstSeed = 0;
    pParams->forcedOutputFourcc = 0;

    // plug-in GUID
    pParams->need_plugin = false;

    // Use RunFrameVPPAsyncEx
    pParams->use_extapi  = false;

    // Do not call MFXVideoVPP_Reset
    pParams->resetFrmNums.clear();

    pParams->bInitEx      = false;
    pParams->GPUCopyValue = MFX_GPUCOPY_DEFAULT;

    return;

} // void vppDefaultInitParams( sInputParams* pParams )

void SaveRealInfoForSvcOut(sSVCLayerDescr in[8], mfxFrameInfo out[8], mfxU32 fourcc)
{
    for(mfxU32 did = 0; did < 8; did++)
    {
        out[did].Width  = in[did].width;
        out[did].Height = in[did].height;

        //temp solution for crop
        out[did].CropX = 0;
        out[did].CropY = 0;
        out[did].CropW = out[did].Width;
        out[did].CropH = out[did].Height;

        // additional
        out[did].FourCC = fourcc;
    }

} // void SaveRealInfoForSvcOut(sSVCLayerDescr in[8], mfxFrameInfo out[8])


mfxStatus OutputProcessFrame(
    sAppResources Resources,
    mfxFrameInfo* pOutFrameInfo,
    mfxU32 &nFrames,
    mfxU32 paramID)
{
    mfxStatus sts;
    mfxFrameSurfaceWrap*   pProcessedSurface;

    for(;!Resources.pSurfStore->m_SyncPoints.empty();Resources.pSurfStore->m_SyncPoints.pop_front())
    {
        sts = Resources.pProcessor->mfxSession.SyncOperation(
            Resources.pSurfStore->m_SyncPoints.front().first,
            MSDK_VPP_WAIT_INTERVAL);
        if(sts==MFX_WRN_IN_EXECUTION)
        {
            msdk_printf(MSDK_STRING("SyncOperation wait interval exceeded\n"));
        }
        MSDK_CHECK_NOT_EQUAL(sts, MFX_ERR_NONE, sts);

        pProcessedSurface = Resources.pSurfStore->m_SyncPoints.front().second.pSurface;

        GeneralWriter* writer = (1 == Resources.dstFileWritersN) ? &Resources.pDstFileWriters[0] : &Resources.pDstFileWriters[paramID];
        sts = writer->PutNextFrame(
            Resources.pAllocator,
            pOutFrameInfo,
            pProcessedSurface);

        DecreaseReference(&pProcessedSurface->Data);

        if(sts)
            msdk_printf(MSDK_STRING("Failed to write frame to disk\n"));
        MSDK_CHECK_NOT_EQUAL(sts, MFX_ERR_NONE, MFX_ERR_ABORTED);

        nFrames++;

        //VPP progress
        if (!Resources.pParams->bPerf)
        {
            msdk_printf(MSDK_STRING("Frame number: %d\r"), nFrames);
        }
        else
        {
            if (!(nFrames % 100))
                msdk_printf(MSDK_STRING("."));
        }
    }
    return MFX_ERR_NONE;

} // mfxStatus OutputProcessFrame(


void ownToMfxFrameInfo( sOwnFrameInfo* in, mfxFrameInfo* out, bool copyCropParams=false)
{
    out->Width          = in->nWidth;
    out->Height         = in->nHeight;

    if(copyCropParams)
    {
        out->CropX          = in->CropX;
        out->CropY          = in->CropY;
        out->CropW          = (in->CropW == NOT_INIT_VALUE) ? in->nWidth : in->CropW;
        out->CropH          = (in->CropH == NOT_INIT_VALUE) ? in->nHeight : in->CropH;
    }
    else
    {
        out->CropX          = 0;
        out->CropY          = 0;
        out->CropW          = in->nWidth;
        out->CropH          = in->nHeight;
    }
    out->FourCC = in->FourCC;

    out->PicStruct      = in->PicStruct;
    out->BitDepthLuma   = in->BitDepthLuma;
    out->BitDepthChroma = in->BitDepthChroma;

    ConvertFrameRate(in->dFrameRate, &out->FrameRateExtN, &out->FrameRateExtD);

    return;

}

#if defined(_WIN32) || defined(_WIN64)
int _tmain(int argc, TCHAR *argv[])
#else
int main(int argc, msdk_char *argv[])
#endif
{
    mfxStatus           sts = MFX_ERR_NONE;
    mfxU32              nFrames = 0;
    mfxU16              nInStreamInd = 0;

    CRawVideoReader     yuvReaders[MAX_INPUT_STREAMS];

    CTimeStatistics     statTimer;

    sFrameProcessor     frameProcessor;
    sMemoryAllocator    allocator{};

    sInputParams          Params;
    MfxVideoParamsWrapper mfxParamsVideo;

    // to prevent incorrect read/write of image in case of CropW/H != width/height
    mfxFrameInfo        realFrameInfoIn[MAX_INPUT_STREAMS]; // Inputs+output
    mfxFrameInfo        realFrameInfoOut;
    sAppResources       Resources;

    mfxFrameSurfaceWrap*   pInSurf[MAX_INPUT_STREAMS]={};
    mfxFrameSurfaceWrap*   pOutSurf = nullptr;
    mfxFrameSurfaceWrap*   pWorkSurf = nullptr;

    mfxSyncPoint        syncPoint;

    bool                bFrameNumLimit = false;
    mfxU32              numGetFrames = 0;

    SurfaceVPPStore     surfStore;

    unique_ptr<PTSMaker> ptsMaker;

    /* generators for ROI testing */
    ROIGenerator       inROIGenerator;
    ROIGenerator       outROIGenerator;
    bool               bROITest[2] = {false, false};

#ifdef ENABLE_VPP_RUNTIME_HSBC
    /* a counter of output frames*/
    mfxU32             nOutFrames = 0;
#endif

    //mfxU16              argbSurfaceIndex = 0xffff;

    /* default parameters */
    sOwnFrameInfo             defaultOwnFrameInfo         = { 0, 0, 0, 0, 0, 0, 0, 0, 0, MFX_FOURCC_NV12, MFX_PICSTRUCT_PROGRESSIVE, 30.0 };
    sDIParam                  defaultDIParam              = { 0, 0, 0, VPP_FILTER_DISABLED };
    sProcAmpParam             defaultProcAmpParam         = { 0.0, 1.0, 1.0, 0.0, VPP_FILTER_DISABLED };
    sDetailParam              defaultDetailParam          = { 1,  VPP_FILTER_DISABLED };
    sDenoiseParam             defaultDenoiseParam         = { 1,  VPP_FILTER_DISABLED };
#ifdef ENABLE_MCTF
    sMCTFParam                defaultMctfParam;
    defaultMctfParam.mode = VPP_FILTER_DISABLED;
    defaultMctfParam.params.FilterStrength = 0;
#ifdef ENABLE_MCTF_EXT
    defaultMctfParam.params.BitsPerPixelx100k = 0;
    defaultMctfParam.params.Deblocking = MFX_CODINGOPTION_OFF;
    defaultMctfParam.params.Overlap = MFX_CODINGOPTION_OFF;
    defaultMctfParam.params.TemporalMode = MFX_MCTF_TEMPORAL_MODE_2REF;
    defaultMctfParam.params.MVPrecision = MFX_MVPRECISION_INTEGER;
#endif
#endif
    sVideoAnalysisParam       defaultVAParam              = { VPP_FILTER_DISABLED };
    sIDetectParam             defaultIDetectParam         = { VPP_FILTER_DISABLED };
    sFrameRateConversionParam defaultFRCParam             = { MFX_FRCALGM_PRESERVE_TIMESTAMP, VPP_FILTER_DISABLED };
    //MSDK 3.0
    sMultiViewParam           defaultMultiViewParam       = { 1, VPP_FILTER_DISABLED };
    //MSDK API 1.5
    sGamutMappingParam        defaultGamutParam           = { false,  VPP_FILTER_DISABLED };
    sTccParam                 defaultClrSaturationParam   = { 160, 160, 160, 160, VPP_FILTER_DISABLED };
    sAceParam                 defaultContrastParam        = { VPP_FILTER_DISABLED };
    sSteParam                 defaultSkinParam            = { 4, VPP_FILTER_DISABLED };
    sIStabParam               defaultImgStabParam         = { MFX_IMAGESTAB_MODE_BOXING, VPP_FILTER_DISABLED };
    sSVCParam                 defaultSVCParam             = { {}, VPP_FILTER_DISABLED };
    sVideoSignalInfoParam     defaultVideoSignalInfoParam;
    sMirroringParam           defaultMirroringParam;
    sColorFillParam           defaultColorfillParam;

    sFiltersParam             defaultFiltersParam =
    { &defaultOwnFrameInfo,
    &defaultDIParam,
    &defaultProcAmpParam,
    &defaultDetailParam,
    &defaultDenoiseParam,
#ifdef ENABLE_MCTF
    &defaultMctfParam,
#endif
    &defaultVAParam,
    &defaultIDetectParam,
    &defaultFRCParam,
    &defaultMultiViewParam,
    &defaultGamutParam,
    &defaultClrSaturationParam,
    &defaultContrastParam,
    &defaultSkinParam,
    &defaultImgStabParam,
    &defaultSVCParam,
    &defaultVideoSignalInfoParam,
    &defaultMirroringParam,
    &defaultColorfillParam};


    //reset pointers to the all internal resources
    MSDK_ZERO_MEMORY(Resources);
    MSDK_ZERO_MEMORY(realFrameInfoIn);
    MSDK_ZERO_MEMORY(realFrameInfoOut);

    for (int i = 0; i < Resources.numSrcFiles; i++)
    {
        Resources.pSrcFileReaders[i] = &yuvReaders[i];
    }

    Resources.pProcessor        = &frameProcessor;
    Resources.pAllocator        = &allocator;
    Resources.pVppParams        = &mfxParamsVideo;
    Resources.pParams           = &Params;
    Resources.pSurfStore        = &surfStore;

    vppDefaultInitParams( &Params, &defaultFiltersParam );

    //parse input string
    sts = vppParseInputString(argv, (mfxU8)argc, &Params, &defaultFiltersParam);
    if (MFX_ERR_NONE != sts)
    {
        vppPrintHelp(argv[0], MSDK_STRING("Parameters parsing error"));
        return 1;
    }
    MSDK_CHECK_STATUS(sts, "vppParseInputString failed");

    // to check time stamp settings
    if (Params.ptsFR)
    {
        Params.frameInfoIn[0].dFrameRate = Params.ptsFR;
    }

    if (!CheckInputParams(argv, &Params))
    {
        return 1;
    }

    if (Params.ptsCheck)
    {
        ptsMaker.reset(new PTSMaker);
    }

       //prepare file reader (YUV/RGB file)
    Resources.numSrcFiles = 1;
    if (Params.compositionParam.mode == VPP_FILTER_ENABLED_CONFIGURED)
    {
        Resources.numSrcFiles = Params.numStreams > MAX_INPUT_STREAMS ? MAX_INPUT_STREAMS : Params.numStreams;
        for (int i = 0; i < Resources.numSrcFiles; i++)
        {
            ownToMfxFrameInfo(&(Params.inFrameInfo[i]), &(realFrameInfoIn[i]), true);
            // Set ptsMaker for the first stream only - it will store PTSes
            sts = yuvReaders[i].Init(Params.compositionParam.streamInfo[i].streamName, i == 0 ? ptsMaker.get() : NULL, realFrameInfoIn[i].FourCC);

            // In-place conversion check - I420 and YV12+D3D11 should be converted in reader and processed as NV12
            bool shouldConvert = false;
            if (realFrameInfoIn[i].FourCC == MFX_FOURCC_I420 ||
                ((Params.ImpLib & 0x0f00) == MFX_IMPL_VIA_D3D11 && realFrameInfoIn[i].FourCC == MFX_FOURCC_YV12))
            {
                realFrameInfoIn[i].FourCC = MFX_FOURCC_YV12;
                shouldConvert = true;
            }
            if (shouldConvert)
            {
                msdk_printf(MSDK_STRING("[WARNING] D3D11 does not support YV12 and I420 surfaces. Input will be converted to NV12 by file reader.\n"));
            }


            MSDK_CHECK_STATUS(sts, "yuvReaders[i].Init failed");
        }
    }
    else
    {
        // D3D11 does not support I420 and YV12 surfaces. So file reader will convert them into nv12.
        // It may be slower than using vpp
        if (Params.fccSource == MFX_FOURCC_I420 ||
            ((Params.ImpLib & 0x0f00) == MFX_IMPL_VIA_D3D11 && Params.fccSource == MFX_FOURCC_YV12))
        {
            msdk_printf(MSDK_STRING("[WARNING] D3D11 does not support YV12 and I420 surfaces. Input will be converted to NV12 by file reader.\n"));
            Params.inFrameInfo[0].FourCC = MFX_FOURCC_NV12;
            Params.frameInfoIn[0].FourCC = MFX_FOURCC_NV12;
        }

        ownToMfxFrameInfo( &(Params.frameInfoIn[0]),  &realFrameInfoIn[0]);
        sts = yuvReaders[VPP_IN].Init(Params.strSrcFile,ptsMaker.get(), Params.fccSource);
        MSDK_CHECK_STATUS(sts, "yuvReaders[VPP_IN].Init failed");
    }
    ownToMfxFrameInfo( &(Params.frameInfoOut[0]), &realFrameInfoOut);


    //prepare file writers (YUV file)
    Resources.dstFileWritersN = (mfxU32)Params.strDstFiles.size();
    Resources.pDstFileWriters = new GeneralWriter[Resources.dstFileWritersN];
    const msdk_char* istream;
    for (mfxU32 i = 0; i < Resources.dstFileWritersN; i++)
    {
        istream = Params.isOutput ? Params.strDstFiles[i].c_str() : NULL;
        sts = Resources.pDstFileWriters[i].Init(
            istream,
            ptsMaker.get(),
            NULL,
            Params.forcedOutputFourcc);
        MSDK_CHECK_STATUS_SAFE(sts, "Resources.pDstFileWriters[i].Init failed", {WipeResources(&Resources); WipeParams(&Params);});
    }

//#ifdef LIBVA_SUPPORT
//    if(!(Params.ImpLib & MFX_IMPL_SOFTWARE))
//        allocator.libvaKeeper.reset(CreateLibVA());
//#endif

    //prepare mfxParams
    sts = InitParamsVPP(&mfxParamsVideo, &Params, 0);
            MSDK_CHECK_STATUS_SAFE(sts, "InitParamsVPP failed", {WipeResources(&Resources); WipeParams(&Params);});

    // prepare pts Checker
    if (ptsMaker.get())
    {
        sts = ptsMaker.get()->Init(&mfxParamsVideo, Params.asyncNum - 1, Params.ptsAdvanced);
                MSDK_CHECK_STATUS_SAFE(sts, "ptsMaker.get()->Init failed", {WipeResources(&Resources); WipeParams(&Params);});
    }

    // prepare ROI generator
    if( ROI_VAR_TO_FIX == Params.roiCheckParam.mode || ROI_VAR_TO_VAR == Params.roiCheckParam.mode )
    {
        inROIGenerator.Init(realFrameInfoIn[0].Width, realFrameInfoIn[0].Height, Params.roiCheckParam.srcSeed);
        Params.roiCheckParam.srcSeed = inROIGenerator.GetSeed();
        bROITest[VPP_IN] = true;

    }
    if( ROI_FIX_TO_VAR == Params.roiCheckParam.mode || ROI_VAR_TO_VAR == Params.roiCheckParam.mode )
    {
        outROIGenerator.Init(realFrameInfoIn[0].Width, realFrameInfoIn[0].Height, Params.roiCheckParam.dstSeed);
        Params.roiCheckParam.dstSeed = outROIGenerator.GetSeed();
        bROITest[VPP_OUT] = true;
    }

    sts = ConfigVideoEnhancementFilters(&Params, &Resources, 0);
    MSDK_CHECK_STATUS_SAFE(sts, "ConfigVideoEnhancementFilters failed", {WipeResources(&Resources); WipeParams(&Params);});

    sts = InitResources(&Resources, &mfxParamsVideo, &Params);
    if(MFX_WRN_FILTER_SKIPPED == sts)
    {
        msdk_printf(MSDK_STRING("\nVPP_WRN: some filter(s) skipped\n"));
        MSDK_IGNORE_MFX_STS(sts, MFX_WRN_FILTER_SKIPPED);

    }
    MSDK_CHECK_STATUS_SAFE(sts, "InitResources failed", {WipeResources(&Resources); WipeParams(&Params);});

    if (MFX_WRN_PARTIAL_ACCELERATION == sts)
    {
        Params.bPartialAccel = true;
    }
    else
    {
        Params.bPartialAccel = false;
    }


    if (Params.bPerf)
    {
        for (int i = 0; i < Resources.numSrcFiles; i++)
        {
            sts = yuvReaders[i].PreAllocateFrameChunk(&mfxParamsVideo, &Params, allocator.pMfxAllocator);
        }
        MSDK_CHECK_STATUS_SAFE(sts, "yuvReaders[i].PreAllocateFrameChunk failed", {WipeResources(&Resources); WipeParams(&Params);});
    }
    else if (Params.numFrames)
    {
        bFrameNumLimit = true;
    }

    // print parameters to console
    PrintInfo(&Params, &mfxParamsVideo, &Resources.pProcessor->mfxSession);
    PrintDllInfo();

    sts = MFX_ERR_NONE;
    nFrames = 0;

    msdk_printf(MSDK_STRING("VPP started\n"));
    statTimer.StartTimeMeasurement();

    mfxU32 StartJumpFrame = 13;

    // need to correct jumping parameters due to async mode
    if (ptsMaker.get() && Params.ptsJump && (Params.asyncNum > 1))
    {
        if (Params.asyncNum > StartJumpFrame)
        {
            StartJumpFrame = Params.asyncNum;
        }
        else
        {
            StartJumpFrame = StartJumpFrame/Params.asyncNum*Params.asyncNum;
        }
    }


    bool bDoNotUpdateIn = false;
    if(Params.use_extapi)
        bDoNotUpdateIn = true;

    // pre-multi-view preparation
    bool bMultiView   = (VPP_FILTER_ENABLED_CONFIGURED == Params.multiViewParam[0].mode) ? true : false;
    vector<bool> bMultipleOutStore(Params.multiViewParam[0].viewCount, false);
    mfxFrameSurfaceWrap* viewSurfaceStore[MULTI_VIEW_COUNT_MAX];

    ViewGenerator  viewGenerator( Params.multiViewParam[0].viewCount );

    if( bMultiView )
    {
        if( bFrameNumLimit )
        {
            Params.numFrames *= Params.multiViewParam[0].viewCount;
        }
    }

    bool bNeedReset = false;
    mfxU16 paramID = 0;
    mfxU32 nextResetFrmNum = (Params.resetFrmNums.size() > 0) ? Params.resetFrmNums[0] : NOT_INIT_VALUE;

    //---------------------------------------------------------
    do
    {
        if (bNeedReset)
        {
            paramID++;
            bNeedReset = false;
            nextResetFrmNum = (Params.resetFrmNums.size() > paramID) ? Params.resetFrmNums[paramID] : NOT_INIT_VALUE;

            //prepare mfxParams
            sts = InitParamsVPP(&mfxParamsVideo, &Params, paramID);
            MSDK_CHECK_STATUS_SAFE(sts, "InitParamsVPP failed", {WipeResources(&Resources); WipeParams(&Params);});

            sts = ConfigVideoEnhancementFilters(&Params, &Resources, paramID);
            MSDK_CHECK_STATUS_SAFE(sts, "ConfigVideoEnchancementFilters failed", {WipeResources(&Resources); WipeParams(&Params);});

            sts = Resources.pProcessor->pmfxVPP->Reset(Resources.pVppParams);
            MSDK_CHECK_STATUS_SAFE(sts, "Resources.pProcessor->pmfxVPP->Reset failed", {WipeResources(&Resources); WipeParams(&Params);});

            ownToMfxFrameInfo( &(Params.frameInfoIn[paramID]),  &realFrameInfoIn[0]);
            ownToMfxFrameInfo( &(Params.frameInfoOut[paramID]), &realFrameInfoOut);
            UpdateSurfacePool(mfxParamsVideo.vpp.Out, allocator.responseOut.NumFrameActual, allocator.pSurfacesOut);
            if(Params.numStreams==1)
            {
                UpdateSurfacePool(mfxParamsVideo.vpp.In,  allocator.responseIn[0].NumFrameActual,  allocator.pSurfacesIn[0]);
            }
            else
            {
                for(int i=0;i<Params.numStreams;i++)
                {
                    ownToMfxFrameInfo(&(Params.inFrameInfo[i]),  &realFrameInfoIn[i]);

                    UpdateSurfacePool(mfxParamsVideo.vpp.In, allocator.responseIn[i].NumFrameActual, allocator.pSurfacesIn[i]);
                }
            }

            msdk_printf(MSDK_STRING("VPP reseted at frame number %d\n"), numGetFrames);
        }

        while (MFX_ERR_NONE <= sts || MFX_ERR_MORE_DATA == sts || bDoNotUpdateIn )
        {
            mfxU16 viewID = 0;
            mfxU16 viewIndx = 0;

            if (nInStreamInd >= MAX_INPUT_STREAMS)
            {
                sts = MFX_ERR_UNKNOWN;
                break;
            }

            // update for multiple output of multi-view
            if( bMultiView )
            {
                viewID = viewGenerator.GetNextViewID();
                viewIndx = viewGenerator.GetViewIndx( viewID );

                if( bMultipleOutStore[viewIndx] )
                {
                    pInSurf[nInStreamInd] = viewSurfaceStore[viewIndx];
                    bDoNotUpdateIn = true;

                    bMultipleOutStore[viewIndx] = false;
                }
                else
                {
                    bDoNotUpdateIn = false;
                }
            }

            if( !bDoNotUpdateIn )
            {
                if (nextResetFrmNum == numGetFrames)
                {
                    bNeedReset = true;
                    sts = MFX_ERR_MORE_DATA;
                    break;
                }

                // if we share allocator with mediasdk we need to call Lock to access surface data and after we're done call Unlock
                sts = yuvReaders[nInStreamInd].GetNextInputFrame(&allocator,&realFrameInfoIn[nInStreamInd],&pInSurf[nInStreamInd],nInStreamInd);
                MSDK_BREAK_ON_ERROR(sts);

                // Set input timestamps according to input framerate
                mfxU64 expectedPTS = (((mfxU64)(numGetFrames) * mfxParamsVideo.vpp.In.FrameRateExtD * 90000) / mfxParamsVideo.vpp.In.FrameRateExtN);
                pInSurf[nInStreamInd]->Data.TimeStamp = expectedPTS;

                if( bMultiView )
                {
                    pInSurf[nInStreamInd]->Info.FrameId.ViewId = viewID;
                }

                if (numGetFrames++ == Params.numFrames && bFrameNumLimit)
                {
                    sts = MFX_ERR_MORE_DATA;
                    break;
                }
            }

            // VPP processing
            bDoNotUpdateIn = false;

            sts = GetFreeSurface(
                allocator.pSurfacesOut,
                allocator.responseOut.NumFrameActual ,
                (Params.use_extapi ? &pWorkSurf : &pOutSurf));
            MSDK_BREAK_ON_ERROR(sts);

            if( bROITest[VPP_IN] )
            {
                inROIGenerator.SetROI(  &(pInSurf[nInStreamInd]->Info) );
            }
            if( bROITest[VPP_OUT] )
            {
                outROIGenerator.SetROI((Params.use_extapi ? &pWorkSurf->Info : &pOutSurf->Info));
            }

            if ( Params.use_extapi )
            {
                mfxFrameSurface1 * out_surface = nullptr;
                do
                {
                    sts = frameProcessor.pmfxVPP->RunFrameVPPAsyncEx(
                        pInSurf[nInStreamInd],
                        pWorkSurf,
                        //pExtData,
                        &out_surface,
                        &syncPoint);
                } while (MFX_WRN_DEVICE_BUSY == sts);

                pOutSurf = static_cast<mfxFrameSurfaceWrap*>(out_surface);
                if(MFX_ERR_MORE_DATA != sts)
                    bDoNotUpdateIn = true;
            }
            else
            {
#ifdef ENABLE_VPP_RUNTIME_HSBC
                if (Params.rtHue.isEnabled || Params.rtSaturation.isEnabled ||
                    Params.rtBrightness.isEnabled || Params.rtContrast.isEnabled)
                {
                    auto procAmp = pOutSurf->AddExtBuffer<mfxExtVPPProcAmp>();
                    // set default values for ProcAmp filters
                    procAmp->Brightness = 0.0F;
                    procAmp->Contrast   = 1.0F;
                    procAmp->Hue        = 0.0F;
                    procAmp->Saturation = 1.0F;

                    if (Params.rtHue.isEnabled)
                    {
                        procAmp->Hue = ((nOutFrames / Params.rtHue.interval & 0x1) == 0) ? Params.rtHue.value1 : Params.rtHue.value2;
                    }

                    if (Params.rtSaturation.isEnabled)
                    {
                        procAmp->Saturation = ((nOutFrames / Params.rtSaturation.interval & 0x1) == 0) ? Params.rtSaturation.value1 : Params.rtSaturation.value2;
                    }

                    if (Params.rtBrightness.isEnabled)
                    {
                        procAmp->Brightness = ((nOutFrames / Params.rtBrightness.interval & 0x1) == 0) ? Params.rtBrightness.value1 : Params.rtBrightness.value2;
                    }

                    if (Params.rtContrast.isEnabled)
                    {
                        procAmp->Contrast = ((nOutFrames / Params.rtContrast.interval & 0x1) == 0) ? Params.rtContrast.value1 : Params.rtContrast.value2;
                    }
                }
                nOutFrames++;
#endif

                sts = frameProcessor.pmfxVPP->RunFrameVPPAsync(
                    pInSurf[nInStreamInd],
                    pOutSurf,
                    NULL,
                    &syncPoint);
            }

            nInStreamInd++;
            if (nInStreamInd == Resources.numSrcFiles)
                nInStreamInd = 0;


            if (MFX_ERR_MORE_DATA == sts)
            {
                continue;
            }

            //MFX_ERR_MORE_SURFACE (&& !use_extapi) means output is ready but need more surface
            //because VPP produce multiple out. example: Frame Rate Conversion 30->60
            //MFX_ERR_MORE_SURFACE && use_extapi means that component need more work surfaces
            if (MFX_ERR_MORE_SURFACE == sts)
            {
                sts = MFX_ERR_NONE;

                if( bMultiView )
                {
                    if( viewSurfaceStore[viewIndx] )
                    {
                        DecreaseReference( &(viewSurfaceStore[viewIndx]->Data) );
                    }

                    viewSurfaceStore[viewIndx] = pInSurf[nInStreamInd];
                    IncreaseReference( &(viewSurfaceStore[viewIndx]->Data) );

                    bMultipleOutStore[viewIndx] = true;
                }
                else
                {
                    bDoNotUpdateIn = true;
                }

                if ( Params.use_extapi )
                {
                    // RunFrameAsyncEx is used
                    continue;
                }
            }
            else if (MFX_ERR_NONE == sts && !((nFrames+1)% StartJumpFrame) && ptsMaker.get() && Params.ptsJump) // pts jump
            {
                ptsMaker.get()->JumpPTS();
            }
            else if ( MFX_ERR_NONE == sts && bMultiView )
            {
                if( viewSurfaceStore[viewIndx] )
                {
                    DecreaseReference( &(viewSurfaceStore[viewIndx]->Data) );
                    viewSurfaceStore[viewIndx] = NULL;
                }
            }

            MSDK_CHECK_STATUS_NO_RET(sts, "RunFrameVPPAsync(Ex) failed")
            MSDK_BREAK_ON_ERROR(sts);
            surfStore.m_SyncPoints.push_back(SurfaceVPPStore::SyncPair(syncPoint,pOutSurf));
            IncreaseReference(&pOutSurf->Data);
            if (surfStore.m_SyncPoints.size() != (size_t)(Params.asyncNum * Params.multiViewParam[paramID].viewCount))
            {
                continue;
            }
            sts = OutputProcessFrame(Resources, &realFrameInfoOut, nFrames, paramID);
            MSDK_BREAK_ON_ERROR(sts);

        } // main while loop
        //---------------------------------------------------------

        //process remain sync points
        if (MFX_ERR_MORE_DATA == sts)
        {
            sts = OutputProcessFrame(Resources, &realFrameInfoOut, nFrames, paramID);
            MSDK_CHECK_STATUS_SAFE(sts, "OutputProcessFrame failed", {WipeResources(&Resources); WipeParams(&Params);});
        }

        // means that file has ended, need to go to buffering loop
        MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
        // exit in case of other errors
        MSDK_CHECK_STATUS_SAFE(sts, "OutputProcessFrame failed", {WipeResources(&Resources); WipeParams(&Params);});

        // loop to get buffered frames from VPP
        while (MFX_ERR_NONE <= sts)
        {
            sts = GetFreeSurface(
                allocator.pSurfacesOut,
                allocator.responseOut.NumFrameActual ,
                (Params.use_extapi ? &pWorkSurf : &pOutSurf));
            MSDK_BREAK_ON_ERROR(sts);

            bDoNotUpdateIn = false;

            if ( Params.use_extapi )
            {
                mfxFrameSurface1 * out_surface = nullptr;
                do
                {
                    sts = frameProcessor.pmfxVPP->RunFrameVPPAsyncEx(
                        NULL,
                        pWorkSurf,
                        &out_surface,
                        &syncPoint );
                } while (MFX_WRN_DEVICE_BUSY == sts);

                pOutSurf = static_cast<mfxFrameSurfaceWrap*>(out_surface);
            }
            else
            {
#ifdef ENABLE_VPP_RUNTIME_HSBC
                if (Params.rtHue.isEnabled || Params.rtSaturation.isEnabled ||
                    Params.rtBrightness.isEnabled || Params.rtContrast.isEnabled)
                {
                    auto procAmp = pOutSurf->AddExtBuffer<mfxExtVPPProcAmp>();
                    // set default values for ProcAmp filters
                    procAmp->Brightness = 0.0F;
                    procAmp->Contrast   = 1.0F;
                    procAmp->Hue        = 0.0F;
                    procAmp->Saturation = 1.0F;

                    if (Params.rtHue.isEnabled)
                    {
                        procAmp->Hue = ((nOutFrames / Params.rtHue.interval & 0x1) == 0) ? Params.rtHue.value1 : Params.rtHue.value2;
                    }

                    if (Params.rtSaturation.isEnabled)
                    {
                        procAmp->Saturation = ((nOutFrames / Params.rtSaturation.interval & 0x1) == 0) ? Params.rtSaturation.value1 : Params.rtSaturation.value2;
                    }

                    if (Params.rtBrightness.isEnabled)
                    {
                        procAmp->Brightness = ((nOutFrames / Params.rtBrightness.interval & 0x1) == 0) ? Params.rtBrightness.value1 : Params.rtBrightness.value2;
                    }

                    if (Params.rtContrast.isEnabled)
                    {
                        procAmp->Contrast = ((nOutFrames / Params.rtContrast.interval & 0x1) == 0) ? Params.rtContrast.value1 : Params.rtContrast.value2;
                    }
                }
                nOutFrames++;
#endif

                sts = frameProcessor.pmfxVPP->RunFrameVPPAsync(
                    NULL,
                    pOutSurf,
                    NULL,
                    &syncPoint );
            }
            MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_SURFACE);
            MSDK_BREAK_ON_ERROR(sts);

            sts = Resources.pProcessor->mfxSession.SyncOperation(
                syncPoint,
                MSDK_VPP_WAIT_INTERVAL);
            if(sts)
                msdk_printf(MSDK_STRING("SyncOperation wait interval exceeded\n"));
            MSDK_BREAK_ON_ERROR(sts);

            GeneralWriter* writer = (1 == Resources.dstFileWritersN) ? &Resources.pDstFileWriters[0] : &Resources.pDstFileWriters[paramID];
            sts = writer->PutNextFrame(
                Resources.pAllocator,
                &realFrameInfoOut,
                pOutSurf);
            if(sts)
                msdk_printf(MSDK_STRING("Failed to write frame to disk\n"));
            MSDK_CHECK_NOT_EQUAL(sts, MFX_ERR_NONE, MFX_ERR_ABORTED);

            nFrames++;

            //VPP progress
            if (!Params.bPerf)
                msdk_printf(MSDK_STRING("Frame number: %d\r"), nFrames);
            else
            {
                if (!(nFrames % 100))
                    msdk_printf(MSDK_STRING("."));
            }
        }
    } while (bNeedReset);

    statTimer.StopTimeMeasurement();

    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);

    // report any errors that occurred
    MSDK_CHECK_STATUS_SAFE(sts, "Unexpected error", {WipeResources(&Resources); WipeParams(&Params);});

    msdk_printf(MSDK_STRING("\nVPP finished\n"));
    msdk_printf(MSDK_STRING("\n"));

    msdk_printf(MSDK_STRING("Total frames %d \n"), nFrames);
    msdk_printf(MSDK_STRING("Total time %.2f sec \n"), statTimer.GetTotalTime());
    msdk_printf(MSDK_STRING("Frames per second %.3f fps \n"), nFrames / statTimer.GetTotalTime());

    PutPerformanceToFile(Params, nFrames / statTimer.GetTotalTime());

    WipeResources(&Resources);
    WipeParams(&Params);

    return 0; /* OK */

} // int _tmain(int argc, msdk_char *argv[])
/* EOF */