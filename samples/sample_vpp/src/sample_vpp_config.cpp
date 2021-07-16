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

#ifndef MFX_VERSION
#error MFX_VERSION not defined
#endif

mfxStatus ConfigVideoEnhancementFilters( sInputParams* pParams, sAppResources* pResources, mfxU32 paramID )
{
    MfxVideoParamsWrapper*   pVppParam = pResources->pVppParams;
    mfxU32 enhFilterCount = 0;

    // [1] video enhancement algorithms can be enabled with default parameters
    if (VPP_FILTER_DISABLED != pParams->denoiseParam[paramID].mode)
    {
        pResources->tabDoUseAlg[enhFilterCount++] = MFX_EXTBUFF_VPP_DENOISE;
    }
#ifdef ENABLE_MCTF
    if (VPP_FILTER_ENABLED_DEFAULT == pParams->mctfParam[paramID].mode)
    {
        pResources->tabDoUseAlg[enhFilterCount++] = MFX_EXTBUFF_VPP_MCTF;
    }
#endif
    if (VPP_FILTER_DISABLED != pParams->procampParam[paramID].mode)
    {
        pResources->tabDoUseAlg[enhFilterCount++] = MFX_EXTBUFF_VPP_PROCAMP;
    }
    if (VPP_FILTER_DISABLED != pParams->detailParam[paramID].mode)
    {
        pResources->tabDoUseAlg[enhFilterCount++] = MFX_EXTBUFF_VPP_DETAIL;
    }
    // MSDK API 2013
    if (VPP_FILTER_ENABLED_DEFAULT == pParams->istabParam[paramID].mode)
    {
        pResources->tabDoUseAlg[enhFilterCount++] = MFX_EXTBUFF_VPP_IMAGE_STABILIZATION;
    }
    /*if( VPP_FILTER_DISABLED != pParams->aceParam.mode )
    {
        pResources->tabDoUseAlg[enhFilterCount++] = MFX_EXTBUFF_VPP_AUTO_CONTRAST;
    }
    if( VPP_FILTER_DISABLED != pParams->steParam.mode )
    {
        pResources->tabDoUseAlg[enhFilterCount++] = MFX_EXTBUFF_VPP_SKIN_TONE;
    }
    if( VPP_FILTER_DISABLED != pParams->tccParam.mode )
    {
        pResources->tabDoUseAlg[enhFilterCount++] = MFX_EXTBUFF_VPP_COLOR_SATURATION_LEVEL;
    }*/

    if (enhFilterCount > 0)
    {
        auto doUse = pVppParam->AddExtBuffer<mfxExtVPPDoUse>();
        doUse->NumAlg = enhFilterCount;
        doUse->AlgList = pResources->tabDoUseAlg;
    }

    // [2] video enhancement algorithms can be configured
    if (VPP_FILTER_ENABLED_CONFIGURED == pParams->denoiseParam[paramID].mode)
    {
        auto denoiseConfig           = pVppParam->AddExtBuffer<mfxExtVPPDenoise>();
        denoiseConfig->DenoiseFactor = pParams->denoiseParam[paramID].factor;
    }
#ifdef ENABLE_MCTF
    if (VPP_FILTER_ENABLED_CONFIGURED == pParams->mctfParam[paramID].mode)
    {
        auto mctfConfig               = pVppParam->AddExtBuffer<mfxExtVppMctf>();
        mctfConfig->FilterStrength    = pParams->mctfParam[paramID].params.FilterStrength;
#if defined (ENABLE_MCTF_EXT)
        mctfConfig->Overlap           = pParams->mctfParam[paramID].params.Overlap;
        mctfConfig->TemporalMode      = pParams->mctfParam[paramID].params.TemporalMode;
        mctfConfig->MVPrecision       = pParams->mctfParam[paramID].params.MVPrecision;
        mctfConfig->BitsPerPixelx100k = pParams->mctfParam[paramID].params.BitsPerPixelx100k;
        mctfConfig->Deblocking        = pParams->mctfParam[paramID].params.Deblocking;
#endif
        //enable the filter
    }
#endif
    if( VPP_FILTER_ENABLED_CONFIGURED == pParams->frcParam[paramID].mode )
    {
        auto frcConfig       = pVppParam->AddExtBuffer<mfxExtVPPFrameRateConversion>();
        frcConfig->Algorithm = (mfxU16)pParams->frcParam[paramID].algorithm;//MFX_FRCALGM_DISTRIBUTED_TIMESTAMP;
    }

    if( VPP_FILTER_ENABLED_CONFIGURED == pParams->videoSignalInfoParam[paramID].mode )
    {
        auto videoSignalInfoConfig = pVppParam->AddExtBuffer<mfxExtVPPVideoSignalInfo>();
        videoSignalInfoConfig->In  = pParams->videoSignalInfoParam[paramID].In;
        videoSignalInfoConfig->Out  = pParams->videoSignalInfoParam[paramID].Out;
        videoSignalInfoConfig->NominalRange  = pParams->videoSignalInfoParam[paramID].NominalRange;
        videoSignalInfoConfig->TransferMatrix  = pParams->videoSignalInfoParam[paramID].TransferMatrix;
    }

    if( VPP_FILTER_ENABLED_CONFIGURED == pParams->mirroringParam[paramID].mode )
    {
        auto mirroringConfig  = pVppParam->AddExtBuffer<mfxExtVPPMirroring>();
        mirroringConfig->Type = pParams->mirroringParam[paramID].Type;
    }

    if( VPP_FILTER_ENABLED_CONFIGURED == pParams->colorfillParam[paramID].mode )
    {
        auto colorfillConfig = pVppParam->AddExtBuffer<mfxExtVPPColorFill>();
        colorfillConfig      = &pParams->colorfillParam[paramID];
        std::ignore = colorfillConfig;
    }

    if( VPP_FILTER_ENABLED_CONFIGURED == pParams->procampParam[paramID].mode )
    {
        auto procampConfig        = pVppParam->AddExtBuffer<mfxExtVPPProcAmp>();
        procampConfig->Hue        = pParams->procampParam[paramID].hue;
        procampConfig->Saturation = pParams->procampParam[paramID].saturation;
        procampConfig->Contrast   = pParams->procampParam[paramID].contrast;
        procampConfig->Brightness = pParams->procampParam[paramID].brightness;
    }
    if( VPP_FILTER_ENABLED_CONFIGURED == pParams->detailParam[paramID].mode )
    {
        auto detailConfig = pVppParam->AddExtBuffer<mfxExtVPPDetail>();
        detailConfig->DetailFactor   = pParams->detailParam[paramID].factor;
    }
    if( VPP_FILTER_ENABLED_CONFIGURED == pParams->deinterlaceParam[paramID].mode )
    {
        auto deinterlaceConfig              = pVppParam->AddExtBuffer<mfxExtVPPDeinterlacing>();
        deinterlaceConfig->Mode             = pParams->deinterlaceParam[paramID].algorithm;
        deinterlaceConfig->TelecinePattern  = pParams->deinterlaceParam[paramID].tc_pattern;
        deinterlaceConfig->TelecineLocation = pParams->deinterlaceParam[paramID].tc_pos;
    }
    if( 0 != pParams->rotate[paramID] )
    {
        auto rotationConfig   = pVppParam->AddExtBuffer<mfxExtVPPRotation>();
        rotationConfig->Angle = pParams->rotate[paramID];
    }
    if( pParams->bScaling )
    {
        auto scalingConfig         = pVppParam->AddExtBuffer<mfxExtVPPScaling>();
        scalingConfig->ScalingMode = pParams->scalingMode;
#if MFX_VERSION >= 1033
        scalingConfig->InterpolationMethod = pParams->interpolationMethod;
#endif
    }
#if MFX_VERSION >= 1025
    if (pParams->bChromaSiting)
    {
        auto chromaSitingConfig          = pVppParam->AddExtBuffer<mfxExtColorConversion>();
        chromaSitingConfig->ChromaSiting = pParams->uChromaSiting;
    }
#endif
    //if( VPP_FILTER_ENABLED_CONFIGURED == pParams->gamutParam.mode )
    //{
    //    pResources->gamutConfig.Header.BufferId = MFX_EXTBUFF_VPP_GAMUT_MAPPING;
    //    pResources->gamutConfig.Header.BufferSz = sizeof(mfxExtVPPGamutMapping);

    //    //pResources->detailConfig.DetailFactor   = pParams->detailParam.factor;
    //    if( pParams->gamutParam.bBT709 )
    //    {
    //        pResources->gamutConfig.InTransferMatrix  = MFX_TRASNFERMATRIX_XVYCC_BT709;
    //        pResources->gamutConfig.OutTransferMatrix = MFX_TRANSFERMATRIX_BT709;
    //    }
    //    else
    //    {
    //        pResources->gamutConfig.InTransferMatrix  = MFX_TRANSFERMATRIX_XVYCC_BT601;
    //        pResources->gamutConfig.OutTransferMatrix = MFX_TRANSFERMATRIX_BT601;
    //    }

    //    pVppParam->ExtParam[pVppParam->NumExtParam++] = (mfxExtBuffer*)&(pResources->gamutConfig);
    //}
    // MSDK 1.5 -------------------------------------------
    //if( VPP_FILTER_ENABLED_CONFIGURED == pParams->tccParam.mode )
    //{
    //    pResources->tccConfig.Header.BufferId = MFX_EXTBUFF_VPP_COLOR_SATURATION_LEVEL;
    //    pResources->tccConfig.Header.BufferSz = sizeof(mfxExtVPPColorSaturationLevel);

    //    pResources->tccConfig.Red     = pParams->tccParam.Red;
    //    pResources->tccConfig.Green   = pParams->tccParam.Green;
    //    pResources->tccConfig.Blue    = pParams->tccParam.Blue;
    //    pResources->tccConfig.Magenta = pParams->tccParam.Magenta;
    //    pResources->tccConfig.Yellow  = pParams->tccParam.Yellow;
    //    pResources->tccConfig.Cyan    = pParams->tccParam.Cyan;

    //    pVppParam->ExtParam[pVppParam->NumExtParam++] = (mfxExtBuffer*)&(pResources->tccConfig);
    //}
    //if( VPP_FILTER_ENABLED_CONFIGURED == pParams->aceParam.mode )
    //{
    //    // to do
    //}
    //if( VPP_FILTER_ENABLED_CONFIGURED == pParams->steParam.mode )
    //{
    //    pResources->steConfig.Header.BufferId = MFX_EXTBUFF_VPP_SKIN_TONE;
    //    pResources->steConfig.Header.BufferSz = sizeof(mfxExtVPPSkinTone);
    //    pResources->steConfig.SkinToneFactor  = pParams->steParam.SkinToneFactor;

    //    pVppParam->ExtParam[pVppParam->NumExtParam++] = (mfxExtBuffer*)&(pResources->steConfig);
    //}
    if( VPP_FILTER_ENABLED_CONFIGURED == pParams->istabParam[paramID].mode )
    {
        auto istabConfig  = pVppParam->AddExtBuffer<mfxExtVPPImageStab>();
        istabConfig->Mode = pParams->istabParam[paramID].istabMode;
    }

    // ----------------------------------------------------
    // MVC
    if( VPP_FILTER_ENABLED_CONFIGURED == pParams->multiViewParam[paramID].mode )
    {
        auto multiViewConfig     = pVppParam->AddExtBuffer<mfxExtMVCSeqDesc>();
        multiViewConfig->NumView = pParams->multiViewParam[paramID].viewCount;
        multiViewConfig->View    = new mfxMVCViewDependency [ pParams->multiViewParam[paramID].viewCount ];

        ViewGenerator viewGenerator( pParams->multiViewParam[paramID].viewCount );

        for( mfxU16 viewIndx = 0; viewIndx < pParams->multiViewParam[paramID].viewCount; viewIndx++ )
        {
            multiViewConfig->View[viewIndx].ViewId = viewGenerator.GetNextViewID();
        }
    }

    // Composition
    if( VPP_FILTER_ENABLED_CONFIGURED == pParams->compositionParam.mode )
    {
        auto compositeConfig             = pVppParam->AddExtBuffer<mfxExtVPPComposite>();
        compositeConfig->NumInputStream  = pParams->numStreams;
        compositeConfig->InputStream     = new mfxVPPCompInputStream[compositeConfig->NumInputStream]();

        for (int i = 0; i < compositeConfig->NumInputStream; i++)
        {
            compositeConfig->InputStream[i].DstX = pParams->compositionParam.streamInfo[i].compStream.DstX;
            compositeConfig->InputStream[i].DstY = pParams->compositionParam.streamInfo[i].compStream.DstY;
            compositeConfig->InputStream[i].DstW = pParams->compositionParam.streamInfo[i].compStream.DstW;
            compositeConfig->InputStream[i].DstH = pParams->compositionParam.streamInfo[i].compStream.DstH;
            if (pParams->compositionParam.streamInfo[i].compStream.GlobalAlphaEnable != 0 )
            {
                compositeConfig->InputStream[i].GlobalAlphaEnable = pParams->compositionParam.streamInfo[i].compStream.GlobalAlphaEnable;
                compositeConfig->InputStream[i].GlobalAlpha = pParams->compositionParam.streamInfo[i].compStream.GlobalAlpha;
            }
            if (pParams->compositionParam.streamInfo[i].compStream.LumaKeyEnable != 0 )
            {
                compositeConfig->InputStream[i].LumaKeyEnable = pParams->compositionParam.streamInfo[i].compStream.LumaKeyEnable;
                compositeConfig->InputStream[i].LumaKeyMin = pParams->compositionParam.streamInfo[i].compStream.LumaKeyMin;
                compositeConfig->InputStream[i].LumaKeyMax = pParams->compositionParam.streamInfo[i].compStream.LumaKeyMax;
            }
            if (pParams->compositionParam.streamInfo[i].compStream.PixelAlphaEnable != 0 )
            {
                compositeConfig->InputStream[i].PixelAlphaEnable = pParams->compositionParam.streamInfo[i].compStream.PixelAlphaEnable;
            }
        } // for (int i = 0; i < compositeConfig->NumInputStream; i++)

    }

    return MFX_ERR_NONE;

} // mfxStatus ConfigVideoEnhancementFilters( sAppResources* pResources, mfxVideoParam* pParams )
/* EOF */
