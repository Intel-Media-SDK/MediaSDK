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

#include "math.h"
#include "mfx_common.h"

#if defined (MFX_ENABLE_VPP)
#include "mfx_enc_common.h"
#include "mfx_session.h"

#include "mfx_vpp_hw.h"

#include "mfx_vpp_sw.h"
#include "mfx_vpp_utils.h"

// internal filters
#include "mfx_denoise_vpp.h"
#include "mfx_color_space_conversion_vpp.h"
#include "mfx_resize_vpp.h"
#include "mfx_shift_vpp.h"
#include "mfx_deinterlace_vpp.h"
#include "mfx_video_analysis_vpp.h"
#include "mfx_frame_rate_conversion_vpp.h"
#include "mfx_procamp_vpp.h"
#include "mfx_detail_enhancement_vpp.h"


//-----------------------------------------------------------------------------
//            independent functions
//-----------------------------------------------------------------------------

// all check must be done before call
mfxStatus GetExternalFramesCount(VideoCORE* core,
                                 mfxVideoParam* pParam,
                                 mfxU32* pListID,
                                 mfxU32 len,
                                 mfxU16 framesCountMin[2],
                                 mfxU16 framesCountSuggested[2])
{
    mfxU32 filterIndex;
    mfxU16 inputFramesCount[MAX_NUM_VPP_FILTERS]  = {0};
    mfxU16 outputFramesCount[MAX_NUM_VPP_FILTERS] = {0};

    for( filterIndex = 0; filterIndex < len; filterIndex++ )
    {
        switch( pListID[filterIndex] )
        {

            case (mfxU32)MFX_EXTBUFF_VPP_RSHIFT_IN:
            case (mfxU32)MFX_EXTBUFF_VPP_RSHIFT_OUT:
            case (mfxU32)MFX_EXTBUFF_VPP_LSHIFT_IN:
            case (mfxU32)MFX_EXTBUFF_VPP_LSHIFT_OUT:
            {
                inputFramesCount[filterIndex]  = MFXVideoVPPShift::GetInFramesCountExt();
                outputFramesCount[filterIndex] = MFXVideoVPPShift::GetOutFramesCountExt();
                break;
            }

            case (mfxU32)MFX_EXTBUFF_VPP_DENOISE:
            {
                inputFramesCount[filterIndex]  = MFXVideoVPPDenoise::GetInFramesCountExt();
                outputFramesCount[filterIndex] = MFXVideoVPPDenoise::GetOutFramesCountExt();
                break;
            }

#ifdef MFX_ENABLE_MCTF
            case (mfxU32)MFX_EXTBUFF_VPP_MCTF:
            {
                mfxU16 MctfTemporalMode = CMC::DEFAULT_REFS;
#ifdef MFX_ENABLE_MCTF_EXT
                mfxExtVppMctf* mctf_ctrl = reinterpret_cast<mfxExtVppMctf *>(GetExtendedBuffer(pParam->ExtParam, pParam->NumExtParam, MFX_EXTBUFF_VPP_MCTF));
                MctfTemporalMode = mctf_ctrl ? mctf_ctrl->TemporalMode : CMC::DEFAULT_REFS;
                // should process UNKNOWN mode here or its an error?
                if (MCTF_TEMPORAL_MODE_UNKNOWN == MctfTemporalMode)
                    MctfTemporalMode = CMC::DEFAULT_REFS;
#endif
                switch (MctfTemporalMode)
                {
                case MCTF_TEMPORAL_MODE_SPATIAL:
                    // this is for spatial filtering mode only; 1 input is enough
                    inputFramesCount[filterIndex] = 1;
                    outputFramesCount[filterIndex] = 1;
                    break;
                case MCTF_TEMPORAL_MODE_1REF:
                    // preasumably, this is for filtering mode with 1 backward refernce; 1 input is enough
                    inputFramesCount[filterIndex] = 1;
                    outputFramesCount[filterIndex] = 1;
                    break;
                case MCTF_TEMPORAL_MODE_2REF:
                    // this is bi-directional MCTF with 2 referencies; thus 2 inputs are needed
                    inputFramesCount[filterIndex] = 2;
                    outputFramesCount[filterIndex] = 2;
                    break;
                case MCTF_TEMPORAL_MODE_4REF:
                    // this is bi-directional MCTF with 4 references(its not a mistake! thus 3 inputs are needed
                    inputFramesCount[filterIndex] = 3;
                    outputFramesCount[filterIndex] = 3;
                    break;
                default:
                    return MFX_ERR_INVALID_VIDEO_PARAM;

                }
                break;
            }
#endif
            case (mfxU32)MFX_EXTBUFF_VPP_RESIZE:
            {
                inputFramesCount[filterIndex]  = MFXVideoVPPResize::GetInFramesCountExt();
                outputFramesCount[filterIndex] = MFXVideoVPPResize::GetOutFramesCountExt();
                break;
            }

            case (mfxU32)MFX_EXTBUFF_VPP_ROTATION:
            {
                inputFramesCount[filterIndex]  = 1;
                outputFramesCount[filterIndex] = 1;
                break;
            }

            case (mfxU32)MFX_EXTBUFF_VPP_SCALING:
            {
                break;
            }

#if (MFX_VERSION >= 1025)
            case (mfxU32)MFX_EXTBUFF_VPP_COLOR_CONVERSION:
            {
                break;
            }
#endif

            case (mfxU32)MFX_EXTBUFF_VPP_MIRRORING:
            {
                break;
            }

            case (mfxU32)MFX_EXTBUFF_VPP_DEINTERLACING:
            {
                break;
            }

            case (mfxU32)MFX_EXTBUFF_VPP_ITC:
            case (mfxU32)MFX_EXTBUFF_VPP_DI:
            {
                inputFramesCount[filterIndex]  = MFXVideoVPPDeinterlace::GetInFramesCountExt();
                outputFramesCount[filterIndex] = MFXVideoVPPDeinterlace::GetOutFramesCountExt();
                break;
            }

            case (mfxU32)MFX_EXTBUFF_VPP_DI_30i60p:
            {
                inputFramesCount[filterIndex]  = MFXVideoVPPDeinterlace::GetInFramesCountExt();
                outputFramesCount[filterIndex] = MFXVideoVPPDeinterlace::GetOutFramesCountExt() << 1;
                break;
            }

            case (mfxU32)MFX_EXTBUFF_VPP_DI_WEAVE:
            {
                inputFramesCount[filterIndex]  = MFXVideoVPPDeinterlace::GetInFramesCountExt() << 1;
                outputFramesCount[filterIndex] = MFXVideoVPPDeinterlace::GetOutFramesCountExt();
                break;
            }

            case (mfxU32)MFX_EXTBUFF_VPP_CSC:
            case (mfxU32)MFX_EXTBUFF_VPP_CSC_OUT_RGB4:
            case (mfxU32)MFX_EXTBUFF_VPP_CSC_OUT_A2RGB10:
            case (mfxU32)MFX_EXTBUFF_VPP_VIDEO_SIGNAL_INFO:
            {
                inputFramesCount[filterIndex]  = MFXVideoVPPColorSpaceConversion::GetInFramesCountExt();
                outputFramesCount[filterIndex] = MFXVideoVPPColorSpaceConversion::GetOutFramesCountExt();
                break;
            }

            case (mfxU32)MFX_EXTBUFF_VPP_SCENE_ANALYSIS:
            {
                inputFramesCount[filterIndex]  = MFXVideoVPPVideoAnalysis::GetInFramesCountExt();

                outputFramesCount[filterIndex] = MFXVideoVPPVideoAnalysis::GetOutFramesCountExt();
                break;
            }

            case (mfxU32)MFX_EXTBUFF_VPP_FRAME_RATE_CONVERSION:
            {
                // call Get[In/Out]FramesCountExt not correct for external application
                // result must be based on in/out frame rates
                mfxFrameInfo info;

                info = pParam->vpp.In;
                mfxF64 inFrameRate  = CalculateUMCFramerate(info.FrameRateExtN, info.FrameRateExtD);
                if (fabs(inFrameRate-0) < 0.01)
                {
                    return MFX_ERR_INVALID_VIDEO_PARAM;
                }

                info = pParam->vpp.Out;
                mfxF64 outFrameRate = CalculateUMCFramerate(info.FrameRateExtN, info.FrameRateExtD);

                outputFramesCount[ filterIndex ] = (mfxU16)(ceil(outFrameRate / inFrameRate));
                outputFramesCount[ filterIndex ] = std::max<mfxU16>(outputFramesCount[ filterIndex ], 1);//robustness

                // numInFrames = inFrameRate / inFrameRate = 1;
                inputFramesCount[ filterIndex ] = 1;

                // after analysis for correct FRC processing we require following equations
                inputFramesCount[ filterIndex ]  = std::max<mfxU16>( inputFramesCount[ filterIndex ],  MFXVideoVPPFrameRateConversion::GetInFramesCountExt() );
                outputFramesCount[ filterIndex ] = std::max<mfxU16>( outputFramesCount[ filterIndex ], MFXVideoVPPFrameRateConversion::GetOutFramesCountExt() );

                break;
            }

            case (mfxU32)MFX_EXTBUFF_VPP_PROCAMP:
            {
                inputFramesCount[filterIndex]  = MFXVideoVPPProcAmp::GetInFramesCountExt();
                outputFramesCount[filterIndex] = MFXVideoVPPProcAmp::GetOutFramesCountExt();
                break;
            }

            case (mfxU32)MFX_EXTBUFF_VPP_DETAIL:
            {
                inputFramesCount[filterIndex]  = MFXVideoVPPDetailEnhancement::GetInFramesCountExt();
                outputFramesCount[filterIndex] = MFXVideoVPPDetailEnhancement::GetOutFramesCountExt();
                break;
            }



            case (mfxU32)MFX_EXTBUFF_VPP_COMPOSITE:
            {
                for (mfxU32 i = 0; i < pParam->NumExtParam; i++)
                {
                    if (pParam->ExtParam[i]->BufferId == MFX_EXTBUFF_VPP_COMPOSITE)
                    {
                        mfxExtVPPComposite* extComp = (mfxExtVPPComposite*) pParam->ExtParam[i];
                        if (extComp->NumInputStream > MAX_NUM_OF_VPP_COMPOSITE_STREAMS)
                        {
                            return MFX_ERR_INVALID_VIDEO_PARAM;
                        }
                        else
                        {
                            if ((core->GetVAType() == MFX_HW_D3D9) && (extComp->NumInputStream > MAX_STREAMS_PER_TILE))
                                return MFX_ERR_INVALID_VIDEO_PARAM;
                            inputFramesCount[filterIndex] = extComp->NumInputStream;
                        }

                        for(mfxU32 j = 0; j < extComp->NumInputStream; j++)
                        {
                            if (pParam->vpp.Out.Width  < (extComp->InputStream[j].DstX + extComp->InputStream[j].DstW))
                                return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;
                            if (pParam->vpp.Out.Height < (extComp->InputStream[j].DstY + extComp->InputStream[j].DstH))
                                return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;
                        }
                    }
                } /*for (mfxU32 i = 0; i < pParam->NumExtParam; i++)*/

                /* for output always one */
                outputFramesCount[filterIndex] = 1;
                break;
            }

            case (mfxU32)MFX_EXTBUFF_VPP_FIELD_PROCESSING:
            {
                inputFramesCount[filterIndex]  = 1;
                outputFramesCount[filterIndex] = 1;
                break;
            }

            case (mfxU32)MFX_EXTBUFF_VPP_FIELD_WEAVING:
            {
                inputFramesCount[filterIndex]  = 2;
                outputFramesCount[filterIndex] = 1;
                break;
            }

            case (mfxU32)MFX_EXTBUFF_VPP_FIELD_SPLITTING:
            {
                inputFramesCount[filterIndex]  = 1;
                outputFramesCount[filterIndex] = 2;
                break;
            }

            default:
            {
                return MFX_ERR_INVALID_VIDEO_PARAM;
            }

        }// CASE
    }//end of

    framesCountSuggested[VPP_IN]  = vppMax_16u(inputFramesCount, len);
    framesCountSuggested[VPP_OUT] = vppMax_16u(outputFramesCount, len);


    // so, SW min frames must be equal MAX(filter0, filter1, ..., filterN-1)
    framesCountMin[VPP_IN]  = framesCountSuggested[VPP_IN];
    framesCountMin[VPP_OUT] = framesCountSuggested[VPP_OUT];

    //robustness
    if( VPP_MAX_REQUIRED_FRAMES_COUNT < framesCountMin[VPP_IN] )
    {
        framesCountMin[VPP_IN] = VPP_MAX_REQUIRED_FRAMES_COUNT;
    }

    if( VPP_MAX_REQUIRED_FRAMES_COUNT < framesCountMin[VPP_OUT] )
    {
        framesCountMin[VPP_OUT] = VPP_MAX_REQUIRED_FRAMES_COUNT;
    }

    return MFX_ERR_NONE;

} // mfxStatus GetExternalFramesCount(...)

// all check must be done before call
bool IsCompositionMode(mfxVideoParam* pParam)
{
    if (pParam->ExtParam && pParam->NumExtParam > 0)
    {
        for (mfxU32 i = 0; i < pParam->NumExtParam; ++i)
        {
            mfxExtBuffer *pExtBuffer = pParam->ExtParam[i];
            if (pExtBuffer->BufferId == (mfxU32)MFX_EXTBUFF_VPP_COMPOSITE)
                return true;
        }
    }

    return false;
}

mfxStatus ExtendedQuery(VideoCORE * core, mfxU32 filterName, mfxExtBuffer* pHint)
{
    mfxStatus sts = MFX_ERR_NONE;

    if( MFX_EXTBUFF_VPP_DENOISE == filterName )
    {
        sts = MFXVideoVPPDenoise::Query( pHint );
    }
#ifdef MFX_ENABLE_MCTF
    else if (MFX_EXTBUFF_VPP_MCTF == filterName)
    {
        sts = CMC::CheckAndFixParams((mfxExtVppMctf*)pHint);
    }
#endif
    else if( MFX_EXTBUFF_VPP_DETAIL == filterName )
    {
        sts = MFXVideoVPPDetailEnhancement::Query( pHint );
    }
    else if( MFX_EXTBUFF_VPP_PROCAMP == filterName )
    {
        sts = MFXVideoVPPProcAmp::Query( pHint );
    }
    else if( MFX_EXTBUFF_VPP_FRAME_RATE_CONVERSION == filterName )
    {
        sts = MFXVideoVPPFrameRateConversion::Query( pHint );
    }
    else if( MFX_EXTBUFF_VPP_SCENE_ANALYSIS == filterName )
    {
        sts = MFX_ERR_UNSUPPORTED;
    }
    else if( MFX_EXTBUFF_VPP_COMPOSITE == filterName )
    {
        if ((NULL != core) && (MFX_PLATFORM_SOFTWARE == core->GetPlatformType()))
        {
            sts = MFX_ERR_UNSUPPORTED;
        }
        else
        {
            sts = MFX_ERR_NONE;
        }
    }
    else if( MFX_EXTBUFF_VPP_FIELD_PROCESSING == filterName )
    {
        if ((NULL != core) && (MFX_PLATFORM_SOFTWARE == core->GetPlatformType()))
        {
            sts = MFX_ERR_UNSUPPORTED;
        }
        else
        {
            sts = MFX_ERR_NONE;
        }
    }
    else if (MFX_EXTBUFF_VPP_SCALING == filterName)
    {
        sts = CheckScalingParam(pHint);
    }
    else // ignore
    {
        sts = MFX_ERR_NONE;
    }

    return sts;

} // mmfxStatus ExtendedQuery(VideoCORE * core, mfxU32 filterName, mfxExtBuffer* pHint)

#endif // MFX_ENABLE_VPP
/* EOF */
