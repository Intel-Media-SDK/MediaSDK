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

#include "sample_vpp_utils.h"

mfxStatus ConfigVideoEnhancementFilters( sInputParams* pParams, sAppResources* pResources, mfxU32 paramID )
{
    mfxVideoParam*   pVppParam = pResources->pVppParams;
    mfxU32  enhFilterCount = 0;

    // [0] common tuning params
    pVppParam->NumExtParam = 0;
    // to simplify logic
    pVppParam->ExtParam    = (mfxExtBuffer**)pResources->pExtBuf;

    pResources->extDoUse.Header.BufferId = MFX_EXTBUFF_VPP_DOUSE;
    pResources->extDoUse.Header.BufferSz = sizeof(mfxExtVPPDoUse);
    pResources->extDoUse.NumAlg  = 0;
    pResources->extDoUse.AlgList = NULL;

    // [1] video enhancement algorithms can be enabled with default parameters
    if( VPP_FILTER_DISABLED != pParams->denoiseParam[paramID].mode )
    {
        pResources->tabDoUseAlg[enhFilterCount++] = MFX_EXTBUFF_VPP_DENOISE;
    }
    if( VPP_FILTER_DISABLED != pParams->procampParam[paramID].mode )
    {
        pResources->tabDoUseAlg[enhFilterCount++] = MFX_EXTBUFF_VPP_PROCAMP;
    }
    if( VPP_FILTER_DISABLED != pParams->detailParam[paramID].mode )
    {
        pResources->tabDoUseAlg[enhFilterCount++] = MFX_EXTBUFF_VPP_DETAIL;
    }
    // MSDK API 2013
    if( VPP_FILTER_ENABLED_DEFAULT == pParams->istabParam[paramID].mode)
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

    if( enhFilterCount > 0 )
    {
        pResources->extDoUse.NumAlg  = enhFilterCount;
        pResources->extDoUse.AlgList = pResources->tabDoUseAlg;
        pVppParam->ExtParam[pVppParam->NumExtParam++] = (mfxExtBuffer*)&(pResources->extDoUse);
    }

    // [2] video enhancement algorithms can be configured
    if( VPP_FILTER_ENABLED_CONFIGURED == pParams->denoiseParam[paramID].mode )
    {
        pResources->denoiseConfig.Header.BufferId = MFX_EXTBUFF_VPP_DENOISE;
        pResources->denoiseConfig.Header.BufferSz = sizeof(mfxExtVPPDenoise);

        pResources->denoiseConfig.DenoiseFactor   = pParams->denoiseParam[paramID].factor;

        pVppParam->ExtParam[pVppParam->NumExtParam++] = (mfxExtBuffer*)&(pResources->denoiseConfig);
    }
    if( VPP_FILTER_ENABLED_CONFIGURED == pParams->frcParam[paramID].mode )
    {
        pResources->frcConfig.Header.BufferId = MFX_EXTBUFF_VPP_FRAME_RATE_CONVERSION;
        pResources->frcConfig.Header.BufferSz = sizeof(mfxExtVPPFrameRateConversion);

        pResources->frcConfig.Algorithm   = (mfxU16)pParams->frcParam[paramID].algorithm;//MFX_FRCALGM_DISTRIBUTED_TIMESTAMP;

        pVppParam->ExtParam[pVppParam->NumExtParam++] = (mfxExtBuffer*)&(pResources->frcConfig);
    }

    if( VPP_FILTER_ENABLED_CONFIGURED == pParams->videoSignalInfoParam[paramID].mode )
    {
        pResources->videoSignalInfoConfig = pParams->videoSignalInfoParam[paramID];
        pVppParam->ExtParam[pVppParam->NumExtParam++] = (mfxExtBuffer*)&(pResources->videoSignalInfoConfig);
    }

    if( VPP_FILTER_ENABLED_CONFIGURED == pParams->mirroringParam[paramID].mode )
    {
        pResources->mirroringConfig = pParams->mirroringParam[paramID];
        pVppParam->ExtParam[pVppParam->NumExtParam++] = (mfxExtBuffer*)&(pResources->mirroringConfig);
    }

    if( VPP_FILTER_ENABLED_CONFIGURED == pParams->procampParam[paramID].mode )
    {
        pResources->procampConfig.Header.BufferId = MFX_EXTBUFF_VPP_PROCAMP;
        pResources->procampConfig.Header.BufferSz = sizeof(mfxExtVPPProcAmp);

        pResources->procampConfig.Hue        = pParams->procampParam[paramID].hue;
        pResources->procampConfig.Saturation = pParams->procampParam[paramID].saturation;
        pResources->procampConfig.Contrast   = pParams->procampParam[paramID].contrast;
        pResources->procampConfig.Brightness = pParams->procampParam[paramID].brightness;

        pVppParam->ExtParam[pVppParam->NumExtParam++] = (mfxExtBuffer*)&(pResources->procampConfig);
    }
    if( VPP_FILTER_ENABLED_CONFIGURED == pParams->detailParam[paramID].mode )
    {
        pResources->detailConfig.Header.BufferId = MFX_EXTBUFF_VPP_DETAIL;
        pResources->detailConfig.Header.BufferSz = sizeof(mfxExtVPPDetail);

        pResources->detailConfig.DetailFactor   = pParams->detailParam[paramID].factor;

        pVppParam->ExtParam[pVppParam->NumExtParam++] = (mfxExtBuffer*)&(pResources->detailConfig);
    }
    if( VPP_FILTER_ENABLED_CONFIGURED == pParams->deinterlaceParam[paramID].mode )
    {
        pResources->deinterlaceConfig.Header.BufferId = MFX_EXTBUFF_VPP_DEINTERLACING;
        pResources->deinterlaceConfig.Header.BufferSz = sizeof(mfxExtVPPDeinterlacing);
        pResources->deinterlaceConfig.Mode = pParams->deinterlaceParam[paramID].algorithm;
        pResources->deinterlaceConfig.TelecinePattern  = pParams->deinterlaceParam[paramID].tc_pattern;
        pResources->deinterlaceConfig.TelecineLocation = pParams->deinterlaceParam[paramID].tc_pos;

        pVppParam->ExtParam[pVppParam->NumExtParam++] = (mfxExtBuffer*)&(pResources->deinterlaceConfig);
    }
    if( 0 != pParams->rotate[paramID] )
    {
        pResources->rotationConfig.Header.BufferId = MFX_EXTBUFF_VPP_ROTATION;
        pResources->rotationConfig.Header.BufferSz = sizeof(mfxExtVPPRotation);
        pResources->rotationConfig.Angle           = pParams->rotate[paramID];

        pVppParam->ExtParam[pVppParam->NumExtParam++] = (mfxExtBuffer*)&(pResources->rotationConfig);
    }
    if( pParams->bScaling )
    {
        pResources->scalingConfig.Header.BufferId = MFX_EXTBUFF_VPP_SCALING;
        pResources->scalingConfig.Header.BufferSz = sizeof(mfxExtVPPScaling);
        pResources->scalingConfig.ScalingMode     = pParams->scalingMode;

        pVppParam->ExtParam[pVppParam->NumExtParam++] = (mfxExtBuffer*)&(pResources->scalingConfig);
    }
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
        pResources->istabConfig.Header.BufferId = MFX_EXTBUFF_VPP_IMAGE_STABILIZATION;
        pResources->istabConfig.Header.BufferSz = sizeof(mfxExtVPPImageStab);
        pResources->istabConfig.Mode            = pParams->istabParam[paramID].istabMode;

        pVppParam->ExtParam[pVppParam->NumExtParam++] = (mfxExtBuffer*)&(pResources->istabConfig);
    }

    // ----------------------------------------------------
    // MVC
    if( VPP_FILTER_ENABLED_CONFIGURED == pParams->multiViewParam[paramID].mode )
    {
        pResources->multiViewConfig.Header.BufferId = MFX_EXTBUFF_MVC_SEQ_DESC;
        pResources->multiViewConfig.Header.BufferSz = sizeof(mfxExtMVCSeqDesc);

        pResources->multiViewConfig.NumView = pParams->multiViewParam[paramID].viewCount;
        pResources->multiViewConfig.View    = new mfxMVCViewDependency [ pParams->multiViewParam[paramID].viewCount ];

        ViewGenerator viewGenerator( pParams->multiViewParam[paramID].viewCount );

        for( mfxU16 viewIndx = 0; viewIndx < pParams->multiViewParam[paramID].viewCount; viewIndx++ )
        {
            pResources->multiViewConfig.View[viewIndx].ViewId = viewGenerator.GetNextViewID();
        }

        pVppParam->ExtParam[pVppParam->NumExtParam++] = (mfxExtBuffer*)&(pResources->multiViewConfig);
    }

    // Composition
    if( VPP_FILTER_ENABLED_CONFIGURED == pParams->compositionParam.mode )
    {
        pResources->compositeConfig.Header.BufferId = MFX_EXTBUFF_VPP_COMPOSITE;
        pResources->compositeConfig.Header.BufferSz = sizeof(mfxExtVPPComposite);
        pResources->compositeConfig.NumInputStream  = pParams->numStreams;
        pResources->compositeConfig.InputStream     = new mfxVPPCompInputStream[pResources->compositeConfig.NumInputStream];
        memset(pResources->compositeConfig.InputStream, 0, sizeof(mfxVPPCompInputStream) * pResources->compositeConfig.NumInputStream);

        for (int i = 0; i < pResources->compositeConfig.NumInputStream; i++)
        {
            pResources->compositeConfig.InputStream[i].DstX = pParams->compositionParam.streamInfo[i].compStream.DstX;
            pResources->compositeConfig.InputStream[i].DstY = pParams->compositionParam.streamInfo[i].compStream.DstY;
            pResources->compositeConfig.InputStream[i].DstW = pParams->compositionParam.streamInfo[i].compStream.DstW;
            pResources->compositeConfig.InputStream[i].DstH = pParams->compositionParam.streamInfo[i].compStream.DstH;
            if (pParams->compositionParam.streamInfo[i].compStream.GlobalAlphaEnable != 0 )
            {
                pResources->compositeConfig.InputStream[i].GlobalAlphaEnable = pParams->compositionParam.streamInfo[i].compStream.GlobalAlphaEnable;
                pResources->compositeConfig.InputStream[i].GlobalAlpha = pParams->compositionParam.streamInfo[i].compStream.GlobalAlpha;
            }
            if (pParams->compositionParam.streamInfo[i].compStream.LumaKeyEnable != 0 )
            {
                pResources->compositeConfig.InputStream[i].LumaKeyEnable = pParams->compositionParam.streamInfo[i].compStream.LumaKeyEnable;
                pResources->compositeConfig.InputStream[i].LumaKeyMin = pParams->compositionParam.streamInfo[i].compStream.LumaKeyMin;
                pResources->compositeConfig.InputStream[i].LumaKeyMax = pParams->compositionParam.streamInfo[i].compStream.LumaKeyMax;
            }
            if (pParams->compositionParam.streamInfo[i].compStream.PixelAlphaEnable != 0 )
            {
                pResources->compositeConfig.InputStream[i].PixelAlphaEnable = pParams->compositionParam.streamInfo[i].compStream.PixelAlphaEnable;
            }
        } // for (int i = 0; i < pResources->compositeConfig.NumInputStream; i++)

        pVppParam->ExtParam[pVppParam->NumExtParam++] = (mfxExtBuffer*)&(pResources->compositeConfig);
    }

    // confirm configuration
    if( 0 == pVppParam->NumExtParam )
    {
        pVppParam->ExtParam = NULL;
    }

    return MFX_ERR_NONE;

} // mfxStatus ConfigVideoEnhancementFilters( sAppResources* pResources, mfxVideoParam* pParams )
/* EOF */
