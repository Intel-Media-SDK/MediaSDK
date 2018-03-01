// Copyright (c) 2017 Intel Corporation
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

#include "mfx_common.h"

#if defined (MFX_ENABLE_VPP)
#if defined (MFX_VA_LINUX)

#include <math.h>
#include "mfx_vpp_defs.h"
#include "mfx_vpp_vaapi.h"
#include "mfx_utils.h"
#include "libmfx_core_vaapi.h"
#include <algorithm>

template<typename T>
bool SetPlaneROI(T value, T* pDst, int dstStep, mfxSize roiSize)
{
    if (!pDst || roiSize.width < 0 || roiSize.height < 0)
        return false;

    for(int h = 0; h < roiSize.height; h++ ) {
        std::fill(pDst, pDst + roiSize.width, value);
        pDst = (T *)((unsigned char*)pDst + dstStep);
    }

    return true;
}


enum QueryStatus
{
    VPREP_GPU_READY         =   0,
    VPREP_GPU_BUSY          =   1,
    VPREP_GPU_NOT_REACHED   =   2,
    VPREP_GPU_FAILED        =   3
};

//////////////////////////////////////////////////////////////////////////
using namespace MfxHwVideoProcessing;

static float convertValue(const float OldMin,const float OldMax,const float NewMin,const float NewMax,const float OldValue)
{
    if((0 == NewMin) && (0 == NewMax)) return OldValue; //workaround
    float oldRange = OldMax - OldMin;
    float newRange = NewMax - NewMin;
    return (((OldValue - OldMin) * newRange) / oldRange) + NewMin;
}

#define DEFAULT_HUE 0
#define DEFAULT_SATURATION 1
#define DEFAULT_CONTRAST 1
#define DEFAULT_BRIGHTNESS 0


#define VA_TOP_FIELD_WEAVE        0x00000004
#define VA_BOTTOM_FIELD_WEAVE     0x00000008

#define VPP_COMP_BACKGROUND_SURFACE_WIDTH  320
#define VPP_COMP_BACKGROUND_SURFACE_HEIGHT 256

VAAPIVideoProcessing::VAAPIVideoProcessing():
  m_bRunning(false)
, m_core(NULL)
, m_vaDisplay(0)
, m_vaConfig(VA_INVALID_ID)
, m_vaContextVPP(VA_INVALID_ID)
, m_denoiseFilterID(VA_INVALID_ID)
, m_detailFilterID(VA_INVALID_ID)
, m_deintFilterID(VA_INVALID_ID)
, m_procampFilterID(VA_INVALID_ID)
, m_frcFilterID(VA_INVALID_ID)
, m_deintFrameCount(0)
, m_bFakeOutputEnabled(false)
, m_frcCyclicCounter(0)
, m_numFilterBufs(0)
, m_primarySurface4Composition(NULL)
{

    for(int i = 0; i < VAProcFilterCount; i++)
        m_filterBufs[i] = VA_INVALID_ID;

    memset( (void*)&m_pipelineCaps, 0, sizeof(m_pipelineCaps));
    memset( (void*)&m_denoiseCaps, 0, sizeof(m_denoiseCaps));
    memset( (void*)&m_detailCaps, 0, sizeof(m_detailCaps));
    memset( (void*)&m_deinterlacingCaps, 0, sizeof(m_deinterlacingCaps));
#ifdef MFX_ENABLE_VPP_FRC
    memset( (void*)&m_frcCaps, 0, sizeof(m_frcCaps));
#endif
    memset( (void*)&m_procampCaps, 0, sizeof(m_procampCaps));

    m_cachedReadyTaskIndex.clear();
    m_feedbackCache.clear();

} // VAAPIVideoProcessing::VAAPIVideoProcessing()


VAAPIVideoProcessing::~VAAPIVideoProcessing()
{
    Close();

} // VAAPIVideoProcessing::~VAAPIVideoProcessing()

mfxStatus VAAPIVideoProcessing::CreateDevice(VideoCORE * core, mfxVideoParam* pParams, bool /*isTemporal*/)
{
    MFX_CHECK_NULL_PTR1( core );

    VAAPIVideoCORE* hwCore = dynamic_cast<VAAPIVideoCORE*>(core);

    MFX_CHECK_NULL_PTR1( hwCore );

    mfxStatus sts = hwCore->GetVAService( &m_vaDisplay);
    MFX_CHECK_STS( sts );

    sts = Init( &m_vaDisplay, pParams);

    MFX_CHECK_STS(sts);

    m_cachedReadyTaskIndex.clear();

    m_core = core;

    return MFX_ERR_NONE;

} // mfxStatus VAAPIVideoProcessing::CreateDevice(VideoCORE * core, mfxInitParams* pParams)


mfxStatus VAAPIVideoProcessing::DestroyDevice(void)
{
    mfxStatus sts = Close();

    return sts;

} // mfxStatus VAAPIVideoProcessing::DestroyDevice(void)


mfxStatus VAAPIVideoProcessing::Close(void)
{
    if (m_primarySurface4Composition != NULL)
    {
        vaDestroySurfaces(m_vaDisplay,m_primarySurface4Composition,1);
        free(m_primarySurface4Composition);
        m_primarySurface4Composition = NULL;
    }

    if( VA_INVALID_ID != m_denoiseFilterID )
    {
        vaDestroyBuffer(m_vaDisplay, m_denoiseFilterID);
    }

    if( VA_INVALID_ID != m_detailFilterID )
    {
        vaDestroyBuffer(m_vaDisplay, m_detailFilterID);
    }

    if( VA_INVALID_ID != m_procampFilterID )
    {
        vaDestroyBuffer(m_vaDisplay, m_procampFilterID);
    }

    if( VA_INVALID_ID != m_deintFilterID )
    {
        vaDestroyBuffer(m_vaDisplay, m_deintFilterID);
    }

    if( VA_INVALID_ID != m_frcFilterID )
    {
        vaDestroyBuffer(m_vaDisplay, m_frcFilterID);
    }

    if (m_vaContextVPP != VA_INVALID_ID)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaDestroyContext");
        vaDestroyContext( m_vaDisplay, m_vaContextVPP );
        m_vaContextVPP = VA_INVALID_ID;
    }

    if (m_vaConfig != VA_INVALID_ID)
    {
        vaDestroyConfig( m_vaDisplay, m_vaConfig );
        m_vaConfig = VA_INVALID_ID;
    }

    for(int i = 0; i < VAProcFilterCount; i++)
        m_filterBufs[i] = VA_INVALID_ID;

    m_denoiseFilterID   = VA_INVALID_ID;
    m_deintFilterID     = VA_INVALID_ID;
    m_procampFilterID   = VA_INVALID_ID;

    memset( (void*)&m_pipelineCaps, 0, sizeof(m_pipelineCaps));
    memset( (void*)&m_denoiseCaps, 0, sizeof(m_denoiseCaps));
    memset( (void*)&m_detailCaps, 0, sizeof(m_detailCaps));
    memset( (void*)&m_procampCaps,  0, sizeof(m_procampCaps));
    memset( (void*)&m_deinterlacingCaps, 0, sizeof(m_deinterlacingCaps));

    return MFX_ERR_NONE;

} // mfxStatus VAAPIVideoProcessing::Close(void)

mfxStatus VAAPIVideoProcessing::Init(_mfxPlatformAccelerationService* pVADisplay, mfxVideoParam* pParams)
{
    if(false == m_bRunning)
    {
        MFX_CHECK_NULL_PTR1( pVADisplay );
        MFX_CHECK_NULL_PTR1( pParams );

        m_cachedReadyTaskIndex.clear();

        VAEntrypoint* va_entrypoints = NULL;
        VAStatus vaSts;
        int va_max_num_entrypoints   = vaMaxNumEntrypoints(m_vaDisplay);
        if(va_max_num_entrypoints)
            va_entrypoints = new VAEntrypoint[va_max_num_entrypoints];
        else
            return MFX_ERR_DEVICE_FAILED;

        mfxI32 entrypointsCount = 0, entrypointsIndx = 0;

        vaSts = vaQueryConfigEntrypoints(m_vaDisplay,
                                            VAProfileNone,
                                            va_entrypoints,
                                            &entrypointsCount);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        for( entrypointsIndx = 0; entrypointsIndx < entrypointsCount; entrypointsIndx++ )
        {
            if( VAEntrypointVideoProc == va_entrypoints[entrypointsIndx] )
            {
                m_bRunning = true;
                break;
            }
        }
        delete[] va_entrypoints;

        if( !m_bRunning )
        {
            return MFX_ERR_DEVICE_FAILED;
        }

        // Configuration
        VAConfigAttrib va_attributes;
        vaSts = vaGetConfigAttributes(m_vaDisplay, VAProfileNone, VAEntrypointVideoProc, &va_attributes, 1);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        vaSts = vaCreateConfig( m_vaDisplay,
                                VAProfileNone,
                                VAEntrypointVideoProc,
                                &va_attributes,
                                1,
                                &m_vaConfig);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        // Context
        int width = pParams->vpp.Out.Width;
        int height = pParams->vpp.Out.Height;

        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaCreateContext");
            vaSts = vaCreateContext(m_vaDisplay,
                                    m_vaConfig,
                                    width,
                                    height,
                                    VA_PROGRESSIVE,
                                    0, 0,
                                    &m_vaContextVPP);
        }
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }

    return MFX_ERR_NONE;

} // mfxStatus VAAPIVideoProcessing::Init(_mfxPlatformAccelerationService* pVADisplay, mfxVideoParam* pParams)


mfxStatus VAAPIVideoProcessing::Register(
    mfxHDLPair* /*pSurfaces*/,
    mfxU32 /*num*/,
    BOOL /*bRegister*/)
{
    return MFX_ERR_NONE;
} // mfxStatus VAAPIVideoProcessing::Register(_mfxPlatformVideoSurface *pSurfaces, ...)


mfxStatus VAAPIVideoProcessing::QueryCapabilities(mfxVppCaps& caps)
{
    VAStatus vaSts;

    VAProcFilterType filters[VAProcFilterCount];
    mfxU32 num_filters = VAProcFilterCount;

    vaSts = vaQueryVideoProcFilters(m_vaDisplay, m_vaContextVPP, filters, &num_filters);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    mfxU32 num_procamp_caps = VAProcColorBalanceCount;
    vaSts = vaQueryVideoProcFilterCaps(m_vaDisplay,
                               m_vaContextVPP,
                               VAProcFilterColorBalance,
                               &m_procampCaps, &num_procamp_caps);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    mfxU32 num_denoise_caps = 1;
    vaSts = vaQueryVideoProcFilterCaps(m_vaDisplay,
                               m_vaContextVPP,
                               VAProcFilterNoiseReduction,
                               &m_denoiseCaps, &num_denoise_caps);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    mfxU32 num_detail_caps = 1;
    vaSts = vaQueryVideoProcFilterCaps(m_vaDisplay,
                               m_vaContextVPP,
                               VAProcFilterSharpening,
                               &m_detailCaps, &num_detail_caps);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    mfxU32 num_deinterlacing_caps = VAProcDeinterlacingCount;
    vaSts = vaQueryVideoProcFilterCaps(m_vaDisplay,
                               m_vaContextVPP,
                               VAProcFilterDeinterlacing,
                               &m_deinterlacingCaps, &num_deinterlacing_caps);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

#ifdef MFX_ENABLE_VPP_FRC
    /* to check is FRC enabled or not*/
    /* first need to get number of modes supported by driver*/
    VAProcFilterCapFrameRateConversion tempFRC_Caps;
    mfxU32 num_frc_caps = 1;

    tempFRC_Caps.bget_custom_rates = 1;
    vaSts = vaQueryVideoProcFilterCaps(m_vaDisplay,
                               m_vaContextVPP,
                               VAProcFilterFrameRateConversion,
                               &tempFRC_Caps, &num_frc_caps);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    if (0 != tempFRC_Caps.frc_custom_rates) /* FRC is enabled, at least one mode */
    {
        caps.uFrameRateConversion = 1 ;
        /* Again, only two modes: 24p->60p and 30p->60p is available
         * But driver report 3, but 3rd is equal to 2rd,
         * So only 2 real modes*/
        if (tempFRC_Caps.frc_custom_rates > 2)
            tempFRC_Caps.frc_custom_rates = 2;
        caps.frcCaps.customRateData.resize(tempFRC_Caps.frc_custom_rates);
        /*to get details about each mode */
        tempFRC_Caps.bget_custom_rates = 0;

        for (mfxU32 ii = 0; ii < tempFRC_Caps.frc_custom_rates; ii++)
        {
            m_frcCaps[ii].frc_custom_rates = ii + 1;
            vaSts = vaQueryVideoProcFilterCaps(m_vaDisplay,
                                                m_vaContextVPP,
                                                VAProcFilterFrameRateConversion,
                                                &m_frcCaps[ii], &num_frc_caps);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
            caps.frcCaps.customRateData[ii].inputFramesOrFieldPerCycle = m_frcCaps[ii].input_frames;
            caps.frcCaps.customRateData[ii].outputIndexCountPerCycle = m_frcCaps[ii].output_frames;
            /* out frame rate*/
            caps.frcCaps.customRateData[ii].customRate.FrameRateExtN = m_frcCaps[ii].output_fps;
            /*input frame rate */
            caps.frcCaps.customRateData[ii].customRate.FrameRateExtD = m_frcCaps[ii].input_fps;
        }
    }
#endif // #ifdef MFX_ENABLE_VPP_FRC

    for( mfxU32 filtersIndx = 0; filtersIndx < num_filters; filtersIndx++ )
    {
        if (filters[filtersIndx])
        {
            switch (filters[filtersIndx])
            {
                case VAProcFilterNoiseReduction:
                    caps.uDenoiseFilter = 1;
                    break;
                case VAProcFilterSharpening:
                    caps.uDetailFilter = 1;
                    break;
                case VAProcFilterDeinterlacing:
                    for (mfxU32 i = 0; i < num_deinterlacing_caps; i++)
                    {
                        if (VAProcDeinterlacingBob == m_deinterlacingCaps[i].type)
                            caps.uSimpleDI = 1;
                        if (VAProcDeinterlacingWeave == m_deinterlacingCaps[i].type           ||
                            VAProcDeinterlacingMotionAdaptive == m_deinterlacingCaps[i].type  ||
                            VAProcDeinterlacingMotionCompensated == m_deinterlacingCaps[i].type)
                            caps.uAdvancedDI = 1;
                    }
                    break;
                case VAProcFilterColorBalance:
                    caps.uProcampFilter = 1;
                    break;
               default:
                    break;
            }
        }
    }

#ifdef MFX_ENABLE_VPP_ROTATION
    memset(&m_pipelineCaps,  0, sizeof(VAProcPipelineCaps));
    vaSts = vaQueryVideoProcPipelineCaps(m_vaDisplay,
                                 m_vaContextVPP,
                                 NULL,
                                 0,
                                 &m_pipelineCaps);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    if (m_pipelineCaps.rotation_flags & (1 << VA_ROTATION_90 ) &&
        m_pipelineCaps.rotation_flags & (1 << VA_ROTATION_180) &&
        m_pipelineCaps.rotation_flags & (1 << VA_ROTATION_270))
    {
        caps.uRotation = 1;
    }
#endif

    /* NB! The code below should to be replaced with querying caps from driver*/
#if defined(LINUX_TARGET_PLATFORM_BXTMIN) || defined(LINUX_TARGET_PLATFORM_BXT)
    caps.uMaxWidth  = 8192;
    caps.uMaxHeight = 8192;
#else
    caps.uMaxWidth  = 4096;
    caps.uMaxHeight = 4096;
#endif

    caps.uFieldWeavingControl = 1;

    // [FourCC]
    // should be changed by libva support
    for (mfxU32 indx = 0; indx < sizeof(g_TABLE_SUPPORTED_FOURCC)/sizeof(mfxU32); indx++)
    {
        if (MFX_FOURCC_NV12   == g_TABLE_SUPPORTED_FOURCC[indx] ||
            MFX_FOURCC_YV12   == g_TABLE_SUPPORTED_FOURCC[indx] ||
            MFX_FOURCC_YUY2   == g_TABLE_SUPPORTED_FOURCC[indx] ||
            MFX_FOURCC_UYVY   == g_TABLE_SUPPORTED_FOURCC[indx] ||
            MFX_FOURCC_RGB4   == g_TABLE_SUPPORTED_FOURCC[indx] ||
#if (MFX_VERSION >= MFX_VERSION_NEXT)
            MFX_FOURCC_RGB565 == g_TABLE_SUPPORTED_FOURCC[indx] ||
#endif
            MFX_FOURCC_P010   == g_TABLE_SUPPORTED_FOURCC[indx])
            caps.mFormatSupport[g_TABLE_SUPPORTED_FOURCC[indx]] |= MFX_FORMAT_SUPPORT_INPUT;

        if (MFX_FOURCC_NV12 == g_TABLE_SUPPORTED_FOURCC[indx] ||
            MFX_FOURCC_YUY2 == g_TABLE_SUPPORTED_FOURCC[indx] ||
            MFX_FOURCC_RGB4 == g_TABLE_SUPPORTED_FOURCC[indx])
            caps.mFormatSupport[g_TABLE_SUPPORTED_FOURCC[indx]] |= MFX_FORMAT_SUPPORT_OUTPUT;
    }

    caps.uMirroring = 1;
    caps.uScaling = 1;

    return MFX_ERR_NONE;

} // mfxStatus VAAPIVideoProcessing::QueryCapabilities(mfxVppCaps& caps)

/// Setup VPP VAAPI driver parameters
/*!
 Setup VPP surfaces and VPP VAAPI parameters.

 \param[in] pParams
 structure with VPP parameters and associated input and reference surfaces.
 \param[out] deint
 VAProcFilterParameterBufferDeinterlacing structure containing deinterlace information (scene change, which field to display).
 \param[out] m_pipelineParam
 VAProcPipelineParameterBuffer structure containing input and reference frames.

 return mfxStatus
 MFX_ERR_NONE id successful
 */
mfxStatus VAAPIVideoProcessing::Execute(mfxExecuteParams *pParams)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VAAPIVideoProcessing::Execute");

    bool bIsFirstField = true;
    bool bUseReference = false;
    VAStatus vaSts = VA_STATUS_SUCCESS;

    VAAPIVideoCORE* hwCore = dynamic_cast<VAAPIVideoCORE*>(m_core);
    eMFXHWType hwType = hwCore->GetHWType();

    // NOTE the following variables should be visible till vaRenderPicture/vaEndPicture,
    // not till vaCreateBuffer as the data they hold are passed to the driver via a pointer
    // to the memory and driver simply copies the pointer till further dereference in the
    // functions noted above. Thus, safer to simply make sure these variable are visible
    // thru the whole scope of the function.
    VARectangle input_region, output_region;

    MFX_CHECK_NULL_PTR1( pParams );
    MFX_CHECK_NULL_PTR1( pParams->targetSurface.hdl.first );
    MFX_CHECK_NULL_PTR1( pParams->pRefSurfaces );
    MFX_CHECK_NULL_PTR1( pParams->pRefSurfaces[0].hdl.first );

    /* There is a special case for composition */
    mfxStatus mfxSts = MFX_ERR_NONE;
    if (pParams->bComposite)
    {
        if ((pParams->iTilesNum4Comp > 0) || (isVideoWall(pParams)) )
        {
            mfxSts = Execute_Composition_TiledVideoWall(pParams);
            if (MFX_ERR_NONE == mfxSts)
                return mfxSts;
        }
        /* if there is errors in params for tiling usage it may work for usual composition */
        else
            mfxSts = Execute_Composition(pParams);
        return mfxSts;
    }


    // This works for ADI 30i->30p for now
    // Better way may involve enabling bEOS for 30i->30p mode
    if ((1 == pParams->refCount) && (pParams->bDeinterlace30i60p == false))
        m_deintFrameCount = 0;

    if (VA_INVALID_ID == m_deintFilterID)
    {
        if (pParams->iDeinterlacingAlgorithm)
        {
            VAProcFilterParameterBufferDeinterlacing deint;
            deint.type  = VAProcFilterDeinterlacing;
            deint.flags = 0;

            if (MFX_DEINTERLACING_BOB == pParams->iDeinterlacingAlgorithm)
            {
                deint.algorithm = VAProcDeinterlacingBob;
            }
            else if(0 == pParams->iDeinterlacingAlgorithm) // skip DI for progressive frames
            {
                deint.algorithm = VAProcDeinterlacingNone;
            }
            else
            {
                deint.algorithm = VAProcDeinterlacingMotionAdaptive;
            }

            // Get picture structure of ouput frame
            mfxDrvSurface* pRefSurf_frameInfo = &(pParams->pRefSurfaces[0]); // previous input frame
            mfxDrvSurface *pCurSurf_frameInfo = &(pParams->pRefSurfaces[pParams->bkwdRefCount]); // current input frame

            // PicStruc can take values: MFX_PICSTRUCT_PROGRESSIVE | MFX_PICSTRUCT_FIELD_TFF | MFX_PICSTRUCT_FIELD_REPEATED=0x10
            // default deint.flags = 0 is for top field in TFF frame.
            if (pCurSurf_frameInfo->frameInfo.PicStruct == MFX_PICSTRUCT_PROGRESSIVE)
                deint.flags = VA_DEINTERLACING_ONE_FIELD;
            else if (pCurSurf_frameInfo->frameInfo.PicStruct & MFX_PICSTRUCT_FIELD_BFF)
                deint.flags = VA_DEINTERLACING_BOTTOM_FIELD_FIRST | VA_DEINTERLACING_BOTTOM_FIELD;

            /* Deinterlacer:
             * BOB is used in first output frame (pParams->refCount = 1) and
             * when scene change flag is set (pParams->scene > 0) ADI is
             * used otherwise.
             * ADI output is late by one field. Current and reference field
             * are fed into de-interlacer.
             **/

            // ADI 30i->30p: To get first field of current frame , set deint.flags second output field.
            if(deint.algorithm == VAProcDeinterlacingMotionAdaptive && (pParams->refCount > 1))
            {
                if (MFX_PICSTRUCT_FIELD_TFF & pCurSurf_frameInfo->frameInfo.PicStruct)
                    deint.flags = VA_DEINTERLACING_BOTTOM_FIELD;
                else /* For BFF, second field is Top */
                    deint.flags = VA_DEINTERLACING_BOTTOM_FIELD_FIRST;

                #if defined(LINUX_TARGET_PLATFORM_BXT) || defined(LINUX_TARGET_PLATFORM_BXTMIN)
                if (MFX_PICSTRUCT_FIELD_TFF & pCurSurf_frameInfo->frameInfo.PicStruct)
                    deint.flags = VA_DEINTERLACING_ONE_FIELD;
                else /* For BFF case required to set all bits  */
                    deint.flags = VA_DEINTERLACING_BOTTOM_FIELD_FIRST | VA_DEINTERLACING_BOTTOM_FIELD | VA_DEINTERLACING_ONE_FIELD;
                #endif // defined(LINUX_TARGET_PLATFORM_BXTMIN) || defined(LINUX_TARGET_PLATFORM_BXT)
            }

            /* For 30i->60p case we have to indicate
             * to driver which field Top or Bottom we a going to send.
             * ADI uses reference frame and start after first frame:
             *   m_deintFrameCount==1, if TFF, TOP=second field of reference, BOTTOM=first field of current
             * BOB, ADI_no_ref do not use reference frame and are used on first frame:
             *   m_deintFrameCount==0, if TFF, TOP=first field of current, BOTTOM=seconf field of current.
             */
            if (pParams->bDeinterlace30i60p == true)
            {
                mfxU32 refFramePicStruct = pRefSurf_frameInfo->frameInfo.PicStruct;
                mfxU32 currFramePicStruct = pCurSurf_frameInfo->frameInfo.PicStruct;
                bool isCurrentProgressive = false;
                bool isPreviousProgressive = false;
                bool isSameParity = false;

                // Deinterlace with reference can be used after first frame is processed
                if(pParams->refCount > 1 && m_deintFrameCount)
                    bUseReference = true;

                // Check if previous is progressive
                if ((refFramePicStruct == MFX_PICSTRUCT_PROGRESSIVE) ||
                (refFramePicStruct & MFX_PICSTRUCT_FIELD_REPEATED) ||
                (refFramePicStruct & MFX_PICSTRUCT_FRAME_DOUBLING) ||
                (refFramePicStruct & MFX_PICSTRUCT_FRAME_TRIPLING))
                {
                    isPreviousProgressive = true;
                }

                if ((currFramePicStruct == MFX_PICSTRUCT_PROGRESSIVE) ||
                    (currFramePicStruct & MFX_PICSTRUCT_FIELD_REPEATED) ||
                    (currFramePicStruct & MFX_PICSTRUCT_FRAME_DOUBLING) ||
                    (currFramePicStruct & MFX_PICSTRUCT_FRAME_TRIPLING))
                {
                    isCurrentProgressive = true;
                }

                // Use BOB when scene change occur
                if ( MFX_DEINTERLACING_ADVANCED_SCD == pParams->iDeinterlacingAlgorithm &&
                ( pParams->scene != VPP_NO_SCENE_CHANGE))
                    bUseReference = false;

                // Set up wich field to display for 30i->60p mode
                // ADI uses second Field on even frames
                // BOB, ADI_no_ref uses second Field on odd frames
                if (0 == (m_deintFrameCount %2) && bUseReference)
                {
                    bIsFirstField = false;
                }
                else if (1 == (m_deintFrameCount %2) && ((!bUseReference)))
                {
                    bIsFirstField = false;
                }

                // Set deinterlace flag depending on parity and field to display
                if (bIsFirstField) // output is second field of previous input
                {
                    if (MFX_PICSTRUCT_FIELD_TFF & pRefSurf_frameInfo->frameInfo.PicStruct)
                        deint.flags = 0;
                    else
                        deint.flags = VA_DEINTERLACING_BOTTOM_FIELD_FIRST | VA_DEINTERLACING_BOTTOM_FIELD;
                }
                else // output is first field of current input
                {
                    if (MFX_PICSTRUCT_FIELD_TFF & pCurSurf_frameInfo->frameInfo.PicStruct)
                        deint.flags = VA_DEINTERLACING_BOTTOM_FIELD;
                    else
                        deint.flags = VA_DEINTERLACING_BOTTOM_FIELD_FIRST;
                }

                // case where previous and current have different parity -> use ADI no ref on current
                // To avoid frame duplication

                if((refFramePicStruct & MFX_PICSTRUCT_FIELD_TFF) && (currFramePicStruct & MFX_PICSTRUCT_FIELD_TFF))
                    isSameParity = true;
                else if((refFramePicStruct & MFX_PICSTRUCT_FIELD_BFF) && (currFramePicStruct & MFX_PICSTRUCT_FIELD_BFF))
                    isSameParity = true;

                if((!isPreviousProgressive) && (!isCurrentProgressive) && (!isSameParity))
                {
                    // Current is BFF
                    if(!bIsFirstField && (currFramePicStruct & MFX_PICSTRUCT_FIELD_BFF))
                    {
                        deint.flags = VA_DEINTERLACING_BOTTOM_FIELD_FIRST | VA_DEINTERLACING_BOTTOM_FIELD;
                        pParams->refCount =1; // Force ADI no ref on first Field of current frame
                    }

                    // Current is TFF
                    if(!bIsFirstField && (currFramePicStruct & MFX_PICSTRUCT_FIELD_TFF))
                    {
                        deint.flags = 0;
                        pParams->refCount =1; // Force ADI no ref on first Field of current frame
                    }
                }
            } //  if ((30i->60p) && (pParams->refCount > 1)) /* 30i->60p mode only*/

            /* Process special case and scene change flag*/
            if ( MFX_DEINTERLACING_ADVANCED_SCD == pParams->iDeinterlacingAlgorithm &&
                ( pParams->scene != VPP_NO_SCENE_CHANGE))
            {
                if(pParams->bDeinterlace30i60p) // 30i->60p mode
                {
                    // In case of multiple scene changes, use BOB with same field to avoid out of frame order
                    if(VPP_MORE_SCENE_CHANGE_DETECTED == pParams->scene)
                    {
                        if (MFX_PICSTRUCT_FIELD_TFF & pCurSurf_frameInfo->frameInfo.PicStruct)
                            deint.flags = 0;
                        else /* BFF */
                            deint.flags = VA_DEINTERLACING_BOTTOM_FIELD_FIRST | VA_DEINTERLACING_BOTTOM_FIELD;
                    }
                }
                else /* BOB 30i->30p use First Field to generate output*/
                {
                    if (MFX_PICSTRUCT_FIELD_TFF & pCurSurf_frameInfo->frameInfo.PicStruct)
                        deint.flags = 0;
                    else /* Frame is BFF */
                        deint.flags = VA_DEINTERLACING_BOTTOM_FIELD_FIRST | VA_DEINTERLACING_BOTTOM_FIELD;
                }

                deint.flags |= 0x0010; // Use BOB when Scene Change occur.  There will be a special define in va_vpp.h
            }

            vaSts = vaCreateBuffer(m_vaDisplay,
                                   m_vaContextVPP,
                                   VAProcFilterParameterBufferType,
                                   sizeof(deint), 1,
                                   &deint, &m_deintFilterID);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

            m_filterBufs[m_numFilterBufs++] = m_deintFilterID;
        }
    }

    if (VA_INVALID_ID != m_procampFilterID && pParams->bEnableProcAmp)
    {
        /* Buffer was created earlier and it's time to refresh its value */
        MFX_CHECK_STS(RemoveBufferFromPipe(m_procampFilterID));
        m_procampFilterID = VA_INVALID_ID;
    }

    if (VA_INVALID_ID == m_procampFilterID)
    {
        if ( pParams->bEnableProcAmp )
        {
            VAProcFilterParameterBufferColorBalance procamp[4];

            procamp[0].type   = VAProcFilterColorBalance;
            procamp[0].attrib = VAProcColorBalanceBrightness;
            procamp[0].value  = 0;

            if ( pParams->Brightness)
            {
                procamp[0].value = convertValue(-100,
                                100,
                                m_procampCaps[VAProcColorBalanceBrightness-1].range.min_value,
                                m_procampCaps[VAProcColorBalanceBrightness-1].range.max_value,
                                pParams->Brightness);
            }

            procamp[1].type   = VAProcFilterColorBalance;
            procamp[1].attrib = VAProcColorBalanceContrast;
            procamp[1].value  = 1;

            if ( pParams->Contrast != 1 )
            {
                procamp[1].value = convertValue(0,
                                10,
                                m_procampCaps[VAProcColorBalanceContrast-1].range.min_value,
                                m_procampCaps[VAProcColorBalanceContrast-1].range.max_value,
                                pParams->Contrast);
            }

            procamp[2].type   = VAProcFilterColorBalance;
            procamp[2].attrib = VAProcColorBalanceHue;
            procamp[2].value  = 0;

            if ( pParams->Hue)
            {
                procamp[2].value = convertValue(-180,
                                180,
                                m_procampCaps[VAProcColorBalanceHue-1].range.min_value,
                                m_procampCaps[VAProcColorBalanceHue-1].range.max_value,
                                pParams->Hue);
            }

            procamp[3].type   = VAProcFilterColorBalance;
            procamp[3].attrib = VAProcColorBalanceSaturation;
            procamp[3].value  = 1;

            if ( pParams->Saturation != 1 )
            {
                procamp[3].value = convertValue(0,
                                10,
                                m_procampCaps[VAProcColorBalanceSaturation-1].range.min_value,
                                m_procampCaps[VAProcColorBalanceSaturation-1].range.max_value,
                                pParams->Saturation);
            }

            vaSts = vaCreateBuffer((void*)m_vaDisplay,
                                          m_vaContextVPP,
                                          VAProcFilterParameterBufferType,
                                          sizeof(procamp), 4,
                                          &procamp, &m_procampFilterID);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

            m_filterBufs[m_numFilterBufs++] = m_procampFilterID;

            /* Clear enable flag. Since appropriate buffer has been created,
             * it will be used in consequent Execute calls.
             * If application will Reset or call Init/Close with setting procamp
             * with new value, this flag will be raised again thus new buffer with
             * new value will be created. */
            pParams->bEnableProcAmp = false;
        }
    }

    if (VA_INVALID_ID != m_denoiseFilterID && (pParams->denoiseFactor || true == pParams->bDenoiseAutoAdjust))
    {
        /* Buffer was created earlier and it's time to refresh its value */
        MFX_CHECK_STS(RemoveBufferFromPipe(m_denoiseFilterID));
        m_denoiseFilterID = VA_INVALID_ID;
    }

    if (VA_INVALID_ID == m_denoiseFilterID)
    {
        if (pParams->denoiseFactor || true == pParams->bDenoiseAutoAdjust)
        {
            VAProcFilterParameterBuffer denoise;
            denoise.type  = VAProcFilterNoiseReduction;
            if(pParams->bDenoiseAutoAdjust)
                denoise.value = m_denoiseCaps.range.default_value;
            else
                denoise.value = (mfxU16)floor(
                                            convertValue(0,
                                              100,
                                              m_denoiseCaps.range.min_value,
                                              m_denoiseCaps.range.max_value,
                                              pParams->denoiseFactor) + 0.5);
            vaSts = vaCreateBuffer((void*)m_vaDisplay,
                          m_vaContextVPP,
                          VAProcFilterParameterBufferType,
                          sizeof(denoise), 1,
                          &denoise, &m_denoiseFilterID);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

            m_filterBufs[m_numFilterBufs++] = m_denoiseFilterID;
            pParams->denoiseFactor = 0;
            pParams->bDenoiseAutoAdjust = false;
        }
    }

    if (VA_INVALID_ID != m_detailFilterID && (pParams->detailFactor || true == pParams->bDetailAutoAdjust))
    {
        /* Buffer was created earlier and it's time to refresh its value */
        MFX_CHECK_STS(RemoveBufferFromPipe(m_detailFilterID));
        m_detailFilterID = VA_INVALID_ID;
    }

    if (VA_INVALID_ID == m_detailFilterID)
    {
        if (pParams->detailFactor || true == pParams->bDetailAutoAdjust)
        {
            VAProcFilterParameterBuffer detail;
            detail.type  = VAProcFilterSharpening;
            if(pParams->bDetailAutoAdjust)
                detail.value = m_detailCaps.range.default_value;
            else
                detail.value = (mfxU16)floor(
                                           convertValue(0,
                                             100,
                                             m_detailCaps.range.min_value,
                                             m_detailCaps.range.max_value,
                                             pParams->detailFactor) + 0.5);
            vaSts = vaCreateBuffer((void*)m_vaDisplay,
                          m_vaContextVPP,
                          VAProcFilterParameterBufferType,
                          sizeof(detail), 1,
                          &detail, &m_detailFilterID);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

            m_filterBufs[m_numFilterBufs++] = m_detailFilterID;
            pParams->detailFactor = 0;
            pParams->bDetailAutoAdjust = false;
        }
    }

    if (VA_INVALID_ID == m_frcFilterID)
    {
        if (pParams->bFRCEnable)
        {
#ifdef MFX_ENABLE_VPP_FRC
          VAProcFilterParameterBufferFrameRateConversion frcParams;

          memset(&frcParams, 0, sizeof(VAProcFilterParameterBufferFrameRateConversion));
          frcParams.type = VAProcFilterFrameRateConversion;
          if (30 == pParams->customRateData.customRate.FrameRateExtD)
          {
              frcParams.input_fps = 30;
              frcParams.output_fps = 60;
              frcParams.num_output_frames = 2;
              frcParams.repeat_frame = 0;
          }
          else if (24 == pParams->customRateData.customRate.FrameRateExtD)
          {
              frcParams.input_fps = 24;
              frcParams.output_fps = 60;
              frcParams.num_output_frames = 5;
              frcParams.repeat_frame = 0;
          }

          frcParams.cyclic_counter = m_frcCyclicCounter++;
          if ((frcParams.input_fps == 30) && (m_frcCyclicCounter == 2))
                m_frcCyclicCounter = 0;
          if ((frcParams.input_fps == 24) && (m_frcCyclicCounter == 5))
                m_frcCyclicCounter = 0;
          frcParams.output_frames = (VASurfaceID*)(pParams->targetSurface.hdl.first);
          vaSts = vaCreateBuffer((void*)m_vaDisplay,
                        m_vaContextVPP,
                        VAProcFilterParameterBufferType,
                        sizeof(frcParams), 1,
                        &frcParams, &m_frcFilterID);
          MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

          m_filterBufs[m_numFilterBufs++] = m_frcFilterID;
#else
          return MFX_ERR_UNSUPPORTED;
#endif
        }
    }

    /* pParams->refCount is total number of processing surfaces:
     * in case of composition this is primary + sub streams*/

    m_pipelineParam.resize(pParams->refCount);
    m_pipelineParamID.resize(pParams->refCount, VA_INVALID_ID);

    mfxDrvSurface* pRefSurf = &(pParams->pRefSurfaces[0]);
    memset(&m_pipelineParam[0], 0, sizeof(m_pipelineParam[0]));

    if (pParams->bFieldWeaving || ( (pParams->refCount > 1) && (0 != pParams->iDeinterlacingAlgorithm )))
    {
        m_pipelineParam[0].num_backward_references = 1;
        mfxDrvSurface* pRefSurf_1 = NULL;
        /* in pRefSurfaces
            * first is backward references
            * then current src surface
            * and only after this is forward references
            * */
        pRefSurf_1 = &(pParams->pRefSurfaces[0]); // point to previous frame
        VASurfaceID *ref_srf = (VASurfaceID *)(pRefSurf_1->hdl.first);
        m_pipelineParam[0].backward_references = ref_srf;
    }
    /* FRC Interpolated case */
    if (0 != pParams->bFRCEnable)
    {
        if (30 == pParams->customRateData.customRate.FrameRateExtD)
            m_pipelineParam[0].num_forward_references = 2;
        else if (24 == pParams->customRateData.customRate.FrameRateExtD)
            m_pipelineParam[0].num_forward_references = 3;

        if (2 == pParams->refCount) /* may be End of Stream case */
        {
            mfxDrvSurface* pRefSurf_frc1;
            pRefSurf_frc1 = &(pParams->pRefSurfaces[1]);
            m_refForFRC[0] = *(VASurfaceID*)(pRefSurf_frc1->hdl.first);
            m_refForFRC[2] = m_refForFRC[1] = m_refForFRC[0];
        }
        if (3 == pParams->refCount)
        {
            mfxDrvSurface* pRefSurf_frc1;
            pRefSurf_frc1 = &(pParams->pRefSurfaces[1]);
            m_refForFRC[0] = *(VASurfaceID*)(pRefSurf_frc1->hdl.first);
            mfxDrvSurface* pRefSurf_frc2;
            pRefSurf_frc2 = &(pParams->pRefSurfaces[2]);
            m_refForFRC[1] = *(VASurfaceID*) (pRefSurf_frc2->hdl.first);
        }
        if (4 == pParams->refCount)
        {
            mfxDrvSurface* pRefSurf_frc1;
            pRefSurf_frc1 = &(pParams->pRefSurfaces[1]);
            m_refForFRC[0] = *(VASurfaceID*)(pRefSurf_frc1->hdl.first);
            mfxDrvSurface* pRefSurf_frc2;
            pRefSurf_frc2 = &(pParams->pRefSurfaces[2]);
            m_refForFRC[1] = *(VASurfaceID*) (pRefSurf_frc2->hdl.first);
            mfxDrvSurface* pRefSurf_frc3;
            pRefSurf_frc3 = &(pParams->pRefSurfaces[3]);
            m_refForFRC[2] = *(VASurfaceID*) (pRefSurf_frc3->hdl.first);
        }
        /* to pass ref list to pipeline */
        m_pipelineParam[0].forward_references = m_refForFRC;
    } /*if (0 != pParams->bFRCEnable)*/

    /* SRC surface */
    mfxDrvSurface* pSrcInputSurf = &pRefSurf[pParams->bkwdRefCount];
    /* in 30i->60p mode, when scene change occurs, BOB is used and needs current input frame.
        * Current frame = ADI reference for odd output frames
        * Current frame = ADI input for even output frames
        * when mixed picture is progressive, even frame use reference frame for output due to ADI delay
    **/
    if(pParams->bDeinterlace30i60p && ((VPP_SCENE_NEW == pParams->scene)|| (pParams->iDeinterlacingAlgorithm == 0)))
    {
        if(m_deintFrameCount % 2)
            pSrcInputSurf = &(pParams->pRefSurfaces[0]); // point to reference frame
    }

    VASurfaceID *srf = (VASurfaceID *)(pSrcInputSurf->hdl.first);
    m_pipelineParam[0].surface = *srf;

#ifdef MFX_ENABLE_VPP_ROTATION
    switch (pParams->rotation)
    {
    case MFX_ANGLE_90:
        if ( m_pipelineCaps.rotation_flags & (1 << VA_ROTATION_90))
        {
            m_pipelineParam[0].rotation_state = VA_ROTATION_90;
        }
        break;
    case MFX_ANGLE_180:
        if ( m_pipelineCaps.rotation_flags & (1 << VA_ROTATION_180))
        {
            m_pipelineParam[0].rotation_state = VA_ROTATION_180;
        }
        break;
    case MFX_ANGLE_270:
        if ( m_pipelineCaps.rotation_flags & (1 << VA_ROTATION_270))
        {
            m_pipelineParam[0].rotation_state = VA_ROTATION_270;
        }
        break;
    }
#endif

    // source cropping
    mfxFrameInfo *inInfo = &(pRefSurf->frameInfo);
    input_region.y   = inInfo->CropY;
    input_region.x   = inInfo->CropX;
    input_region.height = inInfo->CropH;
    input_region.width  = inInfo->CropW;
    m_pipelineParam[0].surface_region = &input_region;

    // destination cropping
    mfxFrameInfo *outInfo = &(pParams->targetSurface.frameInfo);
    output_region.y  = outInfo->CropY;
    output_region.x   = outInfo->CropX;
    output_region.height= outInfo->CropH;
    output_region.width  = outInfo->CropW;
    m_pipelineParam[0].output_region = &output_region;

    m_pipelineParam[0].output_background_color = 0xff000000; // black for ARGB

#ifdef MFX_ENABLE_VPP_VIDEO_SIGNAL
#define ENABLE_VPP_VIDEO_SIGNAL(X) X
#else
#define ENABLE_VPP_VIDEO_SIGNAL(X)
#endif

    mfxU32  refFourcc = pRefSurf->frameInfo.FourCC;
    switch (refFourcc)
    {
    case MFX_FOURCC_RGB4:
        m_pipelineParam[0].surface_color_standard = VAProcColorStandardNone;
        ENABLE_VPP_VIDEO_SIGNAL(m_pipelineParam[0].input_color_properties.color_range = VA_SOURCE_RANGE_FULL);
        break;
    case MFX_FOURCC_NV12:
    default:
        m_pipelineParam[0].surface_color_standard = VAProcColorStandardBT601;
        ENABLE_VPP_VIDEO_SIGNAL(m_pipelineParam[0].input_color_properties.color_range = VA_SOURCE_RANGE_REDUCED);
        break;
    }

    mfxU32  targetFourcc = pParams->targetSurface.frameInfo.FourCC;
    switch (targetFourcc)
    {
    case MFX_FOURCC_RGB4:
        m_pipelineParam[0].output_color_standard = VAProcColorStandardNone;
        ENABLE_VPP_VIDEO_SIGNAL(m_pipelineParam[0].output_color_properties.color_range = VA_SOURCE_RANGE_FULL);
        break;
    case MFX_FOURCC_NV12:
    default:
        m_pipelineParam[0].output_color_standard = VAProcColorStandardBT601;
        ENABLE_VPP_VIDEO_SIGNAL(m_pipelineParam[0].output_color_properties.color_range = VA_SOURCE_RANGE_REDUCED);
        break;
    }

    m_pipelineParam[0].input_color_properties.chroma_sample_location  = VA_CHROMA_SITING_UNKNOWN;
    m_pipelineParam[0].output_color_properties.chroma_sample_location = VA_CHROMA_SITING_UNKNOWN;

    /* It needs interlaced flag passed only for
        * deinterlacing and scaling. All other filters must
        * use progressive even for interlaced content.
        */
    bool forceProgressive = true;
    if (pParams->iDeinterlacingAlgorithm ||
        inInfo->CropH != outInfo->CropH    ||
        inInfo->CropW != outInfo->CropW)
    {
        forceProgressive = false;
    }

    switch (pRefSurf->frameInfo.PicStruct)
    {
        case MFX_PICSTRUCT_PROGRESSIVE:
            m_pipelineParam[0].filter_flags = VA_FRAME_PICTURE;
            break;
        case MFX_PICSTRUCT_FIELD_TFF:
            m_pipelineParam[0].filter_flags = forceProgressive ? VA_FRAME_PICTURE : VA_TOP_FIELD;
            break;
        case MFX_PICSTRUCT_FIELD_BFF:
            m_pipelineParam[0].filter_flags = forceProgressive ? VA_FRAME_PICTURE : VA_BOTTOM_FIELD;
            break;
    }

    if (pParams->bFieldWeaving)
    {
        // Field weaving needs flags that are different from usual pipeline
        switch (pRefSurf->frameInfo.PicStruct)
        {
            case MFX_PICSTRUCT_FIELD_TFF:
                m_pipelineParam[0].filter_flags = VA_TOP_FIELD_WEAVE;
                break;
            case MFX_PICSTRUCT_FIELD_BFF:
                m_pipelineParam[0].filter_flags = VA_BOTTOM_FIELD_WEAVE;
                break;
        }
    }

    m_pipelineParam[0].filters      = m_filterBufs;
    m_pipelineParam[0].num_filters  = m_numFilterBufs;

    int index = GetCurFrameSignalIdx(pParams);
    if ((index >= 0) && pParams->VideoSignalInfo[index].enabled)
    {
#ifdef MFX_ENABLE_VPP_VIDEO_SIGNAL
        if(pParams->VideoSignalInfo[index].TransferMatrix != MFX_TRANSFERMATRIX_UNKNOWN)
        {
            m_pipelineParam[0].surface_color_standard = (MFX_TRANSFERMATRIX_BT709 == pParams->VideoSignalInfo[index].TransferMatrix ? VAProcColorStandardBT709 : VAProcColorStandardBT601);
        }

        if(pParams->VideoSignalInfo[index].NominalRange != MFX_NOMINALRANGE_UNKNOWN)
        {
            m_pipelineParam[0].input_color_properties.color_range = (MFX_NOMINALRANGE_0_255 == pParams->VideoSignalInfo[index].NominalRange) ? VA_SOURCE_RANGE_FULL : VA_SOURCE_RANGE_REDUCED;
        }
    }

    if (pParams->VideoSignalInfoOut.enabled)
    {
        if(pParams->VideoSignalInfoOut.TransferMatrix != MFX_TRANSFERMATRIX_UNKNOWN)
        {
            m_pipelineParam[0].output_color_standard = (MFX_TRANSFERMATRIX_BT709 == pParams->VideoSignalInfoOut.TransferMatrix ? VAProcColorStandardBT709 : VAProcColorStandardBT601);
        }

        if(pParams->VideoSignalInfoOut.NominalRange != MFX_NOMINALRANGE_UNKNOWN)
        {
            m_pipelineParam[0].output_color_properties.color_range = (MFX_NOMINALRANGE_0_255 == pParams->VideoSignalInfoOut.NominalRange) ? VA_SOURCE_RANGE_FULL : VA_SOURCE_RANGE_REDUCED;
        }
#else
        return MFX_ERR_UNSUPPORTED;
#endif // #ifdef MFX_ENABLE_VPP_VIDEO_SIGNAL
    }

    m_pipelineParam[0].input_color_properties.chroma_sample_location  = VA_CHROMA_SITING_UNKNOWN;
    m_pipelineParam[0].output_color_properties.chroma_sample_location = VA_CHROMA_SITING_UNKNOWN;

    /* Scaling params */
    switch (pParams->scalingMode)
    {
    case MFX_SCALING_MODE_LOWPOWER:
        /* VA_FILTER_SCALING_DEFAULT means the following:
            *  First priority is HW fixed function scaling engine. If it can't work, revert to AVS
            *  If scaling ratio between 1/8...8 -> HW fixed function
            *  If scaling ratio between 1/16...1/8 or larger than 8 -> AVS
            *  If scaling ratio is less than 1/16 -> bilinear
            */
        m_pipelineParam[0].filter_flags |= VA_FILTER_SCALING_DEFAULT;
        break;
    case MFX_SCALING_MODE_QUALITY:
        /*  VA_FILTER_SCALING_HQ means the following:
            *  If scaling ratio is less than 1/16 -> bilinear
            *  For all other cases, AVS is used.
            */
        m_pipelineParam[0].filter_flags |= VA_FILTER_SCALING_HQ;
        break;
    case MFX_SCALING_MODE_DEFAULT:
    default:
        if(MFX_HW_APL == hwType)
        {
            /* Use HW fixed function scaling engine by default on APL due to power consumption considerations */
            m_pipelineParam[0].filter_flags |= VA_FILTER_SCALING_DEFAULT;
        }
        else
        {
            /* Force AVS by default for all platforms except BXT */
            m_pipelineParam[0].filter_flags |= VA_FILTER_SCALING_HQ;
        }
        break;
    }

    vaSts = vaCreateBuffer(m_vaDisplay,
                        m_vaContextVPP,
                        VAProcPipelineParameterBufferType,
                        sizeof(VAProcPipelineParameterBuffer),
                        1,
                        &m_pipelineParam[0],
                        &m_pipelineParamID[0]);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    // increase deinterlacer frame count after frame has been processed
    m_deintFrameCount ++;

    // for ADI 30i->60p, EOS is received on second field after vpp_reset
    // reset m_deintFrameCount to zero after processing second field
    if((pParams->bEOS) && (pParams->bDeinterlace30i60p == true))
        m_deintFrameCount = 0;

#if defined(LINUX_TARGET_PLATFORM_BXTMIN) || defined(LINUX_TARGET_PLATFORM_BXT)
// It looks like only BXT supports this at the moment
#define VPP_NO_COLORFILL
#endif

    VASurfaceID *outputSurface = (VASurfaceID*)(pParams->targetSurface.hdl.first);

#if defined(VPP_NO_COLORFILL)
    /* Explicitly define regions in output surface
     * By default, driver assumes that the whole output surface should be used
     * and in case width/height of the surface are different from input dest region,
     * it may cause undesired driver behavior like additional forced colorfill
     */
    VAProcPipelineParameterBuffer outputParam = {0};
    VABufferID  outputParamBuf = VA_INVALID_ID;

    outputParam.surface = *outputSurface;
    outputParam.surface_region = &output_region;
    outputParam.output_region  = &output_region;

    vaSts = vaCreateBuffer(m_vaDisplay,
                           m_vaContextVPP,
                           VAProcPipelineParameterBufferType,
                           sizeof(VAProcPipelineParameterBuffer),
                           1,
                           &outputParam,
                           &outputParamBuf);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
#endif

    MFX_LTRACE_2(MFX_TRACE_LEVEL_HOTSPOTS, "A|VPP|FILTER|PACKET_START|", "%d|%d", m_vaContextVPP, 0);
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaBeginPicture");
        vaSts = vaBeginPicture(m_vaDisplay,
                            m_vaContextVPP,
                            *outputSurface);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }


#if defined(VPP_NO_COLORFILL)
    if(0 == output_region.x && 0 == output_region.y) // Do not disable colorfill if letterboxing is used
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaRenderPicture");
        vaSts = vaRenderPicture(m_vaDisplay, m_vaContextVPP, &outputParamBuf, 1);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }
#endif

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaRenderPicture");
        vaSts = vaRenderPicture(m_vaDisplay, m_vaContextVPP, &m_pipelineParamID[0], 1);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }

    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaEndPicture");
        vaSts = vaEndPicture(m_vaDisplay, m_vaContextVPP);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }
    MFX_LTRACE_2(MFX_TRACE_LEVEL_HOTSPOTS, "A|VPP|FILTER|PACKET_END|", "%d|%d", m_vaContextVPP, 0);

    for( mfxU32 refIdx = 0; refIdx < m_pipelineParamID.size(); refIdx++ )
    {
        if ( m_pipelineParamID[refIdx] != VA_INVALID_ID)
        {
            vaSts = vaDestroyBuffer(m_vaDisplay, m_pipelineParamID[refIdx]);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
            m_pipelineParamID[refIdx] = VA_INVALID_ID;
        }
    }

#if defined(VPP_NO_COLORFILL)
    vaSts = vaDestroyBuffer(m_vaDisplay, outputParamBuf);
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
#endif

    if (m_deintFilterID != VA_INVALID_ID)
    {
        vaSts = vaDestroyBuffer(m_vaDisplay, m_deintFilterID);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        std::remove(m_filterBufs, m_filterBufs + m_numFilterBufs, m_deintFilterID);
        m_deintFilterID = VA_INVALID_ID;
        m_numFilterBufs--;
        m_filterBufs[m_numFilterBufs] = VA_INVALID_ID;
    }

    // (3) info needed for sync operation
    //-------------------------------------------------------
    {
        UMC::AutomaticUMCMutex guard(m_guard);

        ExtVASurface currentFeedback; // {surface & number_of_task}
        currentFeedback.surface = *outputSurface;
        currentFeedback.number = pParams->statusReportID;
        m_feedbackCache.push_back(currentFeedback);
    }


    if (m_frcFilterID != VA_INVALID_ID)
    {
        vaSts = vaDestroyBuffer(m_vaDisplay, m_frcFilterID);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        std::remove(m_filterBufs, m_filterBufs + m_numFilterBufs, m_frcFilterID);
        m_frcFilterID = VA_INVALID_ID;
        m_filterBufs[m_numFilterBufs] = VA_INVALID_ID;
        m_numFilterBufs--;
    }

    return MFX_ERR_NONE;
} // mfxStatus VAAPIVideoProcessing::Execute(FASTCOMP_BLT_PARAMS *pVideoCompositingBlt)

mfxStatus VAAPIVideoProcessing::RemoveBufferFromPipe(VABufferID id)
{
    VAStatus vaSts;
    if (id != VA_INVALID_ID)
    {
        vaSts = vaDestroyBuffer(m_vaDisplay, id);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        std::remove(m_filterBufs, m_filterBufs + m_numFilterBufs, id);
        m_filterBufs[m_numFilterBufs] = VA_INVALID_ID;
        m_numFilterBufs--;
    }

    return MFX_ERR_NONE;
}




BOOL    VAAPIVideoProcessing::isVideoWall(mfxExecuteParams *pParams)
{
    BOOL result = false;
    mfxU16 outputWidth;
    mfxU16 outputHeight;
    mfxU16 layerWidth  = 0;
    mfxU16 layerHeight = 0;
    mfxU16 numX = 0;
    mfxU16 numY = 0;
    std::vector <mfxU32> indexVW;
    std::map<mfxU16, compStreamElem> layout;

    mfxU32 layerCount = (mfxU32) pParams->fwdRefCount + 1;

    if ( layerCount % MAX_STREAMS_PER_TILE != 0 )
    {
        /* Number of streams must be multiple of 8. That's just for simplicity.
         * TODO: need to handle the case when number is not multiple of 8*/
        return result;
    }

    indexVW.reserve(layerCount);
    outputWidth  = pParams->targetSurface.frameInfo.CropW;
    outputHeight = pParams->targetSurface.frameInfo.CropH;

    /**/
    layerWidth = pParams->dstRects[0].DstW;
    numX = outputWidth / layerWidth;
    layerHeight = pParams->dstRects[0].DstH;
    numY = outputHeight / layerHeight;

    // Set up perfect layout of the video wall
    for ( unsigned int j = 0; j < layerCount; j++ )
    {
        mfxU16 render_order = (j/numX)*numX + (j % numX);
        compStreamElem element;
        element.x  = (j % numX)*layerWidth;
        element.y  = (j/numX)*layerHeight;
        layout.insert(std::pair<mfxU16, compStreamElem>(render_order, element));
    }

    for ( unsigned int i = 0; i < layerCount; i++)
    {
        std::map<mfxU16, compStreamElem>::iterator it;
        DstRect rect = pParams->dstRects[i];

        if ( outputWidth % rect.DstW != 0 )
        {
            /* Layer width should fit output */
            return result;
        }

        if ( layerWidth != rect.DstW)
        {
            /* All layers must have the same width */
            return result;
        }

        if ( outputHeight % rect.DstH != 0 )
        {
            /* Layer height should fit output */
            return result;
        }

        if ( layerHeight != rect.DstH)
        {
            /* All layers must have the same height */
            return result;
        }

        mfxU16 render_order = 0;
        render_order  = rect.DstX/(outputWidth/numX);
        render_order += numX * (rect.DstY/(outputHeight/numY));
        it = layout.find(render_order);
        if ( layout.end() == it )
        {
            /* All layers must have the same height */
            return result;
        }
        else if (it->second.active)
        {
            /* Slot is busy with another layer already */
            return result;
        }
        else if (it->second.x != rect.DstX || it->second.y != rect.DstY)
        {
            /* Layers should be ordered in proper way allowing partial rendering on output */
            return result;
        }
        else
        {
            it->second.active = true;
            it->second.index  = i;
            indexVW.push_back(render_order);
        }
    }

    if ( numX > 1 && numX % 2 != 0 )
    {
        /* Number of layers on X-axis is not correct */
        return result;
    }

    if ( numY % 2 != 0 )
    {
        /* Number of layers on Y-axis is not correct */
        return result;
    }

    result = true;

    pParams->iTilesNum4Comp = layerCount / MAX_STREAMS_PER_TILE;
    for ( unsigned int i = 0; i < layerCount; i++)
    {
        pParams->dstRects[i].TileId = indexVW[i] / MAX_STREAMS_PER_TILE;
    }

    return result;
}

#ifdef MFX_ENABLE_VPP_COMPOSITION
mfxStatus VAAPIVideoProcessing::Execute_Composition_TiledVideoWall(mfxExecuteParams *pParams)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VAAPIVideoProcessing::Execute_Composition_TiledVideoWall");

    VAStatus vaSts = VA_STATUS_SUCCESS;
    VASurfaceAttrib attrib;
    std::vector<VABlendState> blend_state;

    MFX_CHECK_NULL_PTR1( pParams );
    MFX_CHECK_NULL_PTR1( pParams->targetSurface.hdl.first );
    MFX_CHECK_NULL_PTR1( pParams->pRefSurfaces );
    MFX_CHECK_NULL_PTR1( pParams->pRefSurfaces[0].hdl.first );

    if ( 0 == pParams->iTilesNum4Comp )
    {
        return MFX_ERR_UNKNOWN;
    }
    mfxU32 SampleCount = 1;
    mfxU32 layerCount = (mfxU32) pParams->fwdRefCount + 1;

    std::vector<m_tiledVideoWallParams> tilingParams;
    tilingParams.resize(pParams->iTilesNum4Comp);
    for ( unsigned int i = 0; i < tilingParams.size(); i++)
    {
        tilingParams[i].targerRect.x = tilingParams[i].targerRect.y = 0x7fff;
        tilingParams[i].targerRect.width = tilingParams[i].targerRect.height = 0;
    }

    m_pipelineParam.resize(pParams->refCount + 1);
    m_pipelineParamID.resize(pParams->refCount + 1, VA_INVALID_ID);
    blend_state.resize(pParams->refCount + 1);

    std::vector<VARectangle> input_region;
    input_region.resize(pParams->refCount + 1);
    std::vector<VARectangle> output_region;
    output_region.resize(pParams->refCount + 1);

    /* Initial set up for layers */
    for ( unsigned int i = 0; i < layerCount; i++)
    {
        mfxDrvSurface* pRefSurf = &(pParams->pRefSurfaces[i]);
        VASurfaceID* srf = (VASurfaceID*)(pRefSurf->hdl.first);

        m_pipelineParam[i].surface = *srf;

        // source cropping
        mfxU32  refFourcc = pRefSurf->frameInfo.FourCC;
        switch (refFourcc)
        {
            case MFX_FOURCC_RGB4:
                m_pipelineParam[i].surface_color_standard = VAProcColorStandardNone;
                break;
            case MFX_FOURCC_NV12:
            default:
                m_pipelineParam[i].surface_color_standard = VAProcColorStandardBT601;
                break;
        }

        mfxU32  targetFourcc = pParams->targetSurface.frameInfo.FourCC;
        switch (targetFourcc)
        {
            case MFX_FOURCC_RGB4:
                m_pipelineParam[i].output_color_standard = VAProcColorStandardNone;
                break;
            case MFX_FOURCC_NV12:
            default:
                m_pipelineParam[i].output_color_standard = VAProcColorStandardBT601;
                break;
        }

        switch (pRefSurf->frameInfo.PicStruct)
        {
            case MFX_PICSTRUCT_PROGRESSIVE:
                m_pipelineParam[i].filter_flags = VA_FRAME_PICTURE;
                break;
            case MFX_PICSTRUCT_FIELD_TFF:
                m_pipelineParam[i].filter_flags = VA_TOP_FIELD;
                break;
            case MFX_PICSTRUCT_FIELD_BFF:
                m_pipelineParam[i].filter_flags = VA_BOTTOM_FIELD;
                break;
        }

        /* to process input parameters of sub stream:
         * crop info and original size*/
        mfxFrameInfo *inInfo = &(pRefSurf->frameInfo);
        input_region[i].y       = inInfo->CropY;
        input_region[i].x       = inInfo->CropX;
        input_region[i].height     = inInfo->CropH;
        input_region[i].width      = inInfo->CropW;
        m_pipelineParam[i].surface_region = &input_region[i];

        /* to process output parameters of sub stream:
         *  position and destination size */
        output_region[i].y      = pParams->dstRects[i].DstY;
        output_region[i].x       = pParams->dstRects[i].DstX;
        output_region[i].height    = pParams->dstRects[i].DstH;
        output_region[i].width  = pParams->dstRects[i].DstW;
        m_pipelineParam[i].output_region = &output_region[i];

        mfxU32 currTileId = pParams->dstRects[i].TileId;

        if (((currTileId > (pParams->iTilesNum4Comp - 1) )) ||
            (tilingParams[currTileId].numChannels >=8))
        {
            // Maximum number channels in tile is 8
            // fallback case
            return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;
        }
        else
        {
            tilingParams[currTileId].channelIds[tilingParams[currTileId].numChannels] = i;
            tilingParams[currTileId].numChannels++;
            /* lets define tiles working rectangle */
            if (tilingParams[currTileId].targerRect.x > output_region[i].x)
                tilingParams[currTileId].targerRect.x =  output_region[i].x;
            if (tilingParams[currTileId].targerRect.y > output_region[i].y)
                tilingParams[currTileId].targerRect.y =  output_region[i].y;
            if (tilingParams[currTileId].targerRect.width <
                    (output_region[i].x + output_region[i].width) )
                tilingParams[currTileId].targerRect.width =  output_region[i].x + output_region[i].width;
            if (tilingParams[currTileId].targerRect.height <
                    (output_region[i].y + output_region[i].height) )
                tilingParams[currTileId].targerRect.height =  output_region[i].y + output_region[i].height;
        }

        /* Global alpha and luma key can not be enabled together*/
        if (pParams->dstRects[i].GlobalAlphaEnable !=0)
        {
            blend_state[i].flags = VA_BLEND_GLOBAL_ALPHA;
            blend_state[i].global_alpha = ((float)pParams->dstRects[i].GlobalAlpha) /255;
        }
        /* Luma color key  for YUV surfaces only.
         * And Premultiplied alpha blending for RGBA surfaces only.
         * So, these two flags can't combine together  */
        if ((pParams->dstRects[i].LumaKeyEnable    != 0) &&
            (pParams->dstRects[i].PixelAlphaEnable == 0) )
        {
            blend_state[i].flags |= VA_BLEND_LUMA_KEY;
            blend_state[i].min_luma = ((float)pParams->dstRects[i].LumaKeyMin/255);
            blend_state[i].max_luma = ((float)pParams->dstRects[i].LumaKeyMax/255);
        }
        if ((pParams->dstRects[i].LumaKeyEnable    == 0 ) &&
            (pParams->dstRects[i].PixelAlphaEnable != 0 ) )
        {
            blend_state[i].flags |= VA_BLEND_PREMULTIPLIED_ALPHA;
        }
        if ((pParams->dstRects[i].GlobalAlphaEnable != 0) ||
            (pParams->dstRects[i].LumaKeyEnable     != 0) ||
            (pParams->dstRects[i].PixelAlphaEnable  != 0))
        {
            m_pipelineParam[i].blend_state = &blend_state[i];
        }

        m_pipelineParam[i].pipeline_flags |= VA_PROC_PIPELINE_SUBPICTURES;

#if defined(LINUX_TARGET_PLATFORM_BXT) || defined(LINUX_TARGET_PLATFORM_BXTMIN)
        m_pipelineParam[i].pipeline_flags |= VA_PROC_PIPELINE_SUBPICTURES;
        m_pipelineParam[i].filter_flags   |= VA_FILTER_SCALING_HQ;
#else
        m_pipelineParam[i].pipeline_flags  |= VA_PROC_PIPELINE_FAST;
#endif
        m_pipelineParam[i].filters      = 0;
        m_pipelineParam[i].num_filters  = 0;

        vaSts = vaCreateBuffer(m_vaDisplay,
                            m_vaContextVPP,
                            VAProcPipelineParameterBufferType,
                            sizeof(VAProcPipelineParameterBuffer),
                            1,
                            &m_pipelineParam[i],
                            &m_pipelineParamID[i]);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }

    VASurfaceID *outputSurface = (VASurfaceID*)(pParams->targetSurface.hdl.first);
    VAProcPipelineParameterBuffer outputparam = {0};
    VABufferID vpp_pipeline_outbuf = VA_INVALID_ID;

    MFX_CHECK_NULL_PTR1(outputSurface);

    /* Process by groups. Video wall case assumes
     * that surfaces has the same output dimensions
     * We split output by several horizontal tiles */
    for (unsigned int currTile = 0; currTile < tilingParams.size(); currTile++)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaBeginPicture");
        vaSts = vaBeginPicture(m_vaDisplay,
                               m_vaContextVPP,
                               *outputSurface);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        outputparam.surface = *outputSurface;
        // The targerRect.width and targerRect.height here actually storing the x2 and y2
        // value. Deduct x and y respectively to get the exact targerRect.width and
        // targerRect.height
        tilingParams[currTile].targerRect.width -= tilingParams[currTile].targerRect.x;
        tilingParams[currTile].targerRect.height -= tilingParams[currTile].targerRect.y;

        outputparam.output_region  = &tilingParams[currTile].targerRect;
        outputparam.surface_region = &tilingParams[currTile].targerRect;

        vaSts = vaCreateBuffer(m_vaDisplay,
                                  m_vaContextVPP,
                                   VAProcPipelineParameterBufferType,
                                   sizeof(VAProcPipelineParameterBuffer),
                                   1,
                                   &outputparam,
                                   &vpp_pipeline_outbuf);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        vaSts = vaRenderPicture(m_vaDisplay, m_vaContextVPP, &vpp_pipeline_outbuf, 1);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        for (unsigned int i = 0; i < tilingParams[currTile].numChannels; i++)
        {
            vaSts = vaRenderPicture(m_vaDisplay, m_vaContextVPP, &m_pipelineParamID[tilingParams[currTile].channelIds[i]], 1);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        }
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaEndPicture");
            vaSts = vaEndPicture(m_vaDisplay, m_vaContextVPP);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        }
        vaSts = vaDestroyBuffer(m_vaDisplay, vpp_pipeline_outbuf);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }

    for( unsigned int i = 0; i < m_pipelineParamID.size(); i++ )
    {
        if ( m_pipelineParamID[i] != VA_INVALID_ID)
        {
            vaSts = vaDestroyBuffer(m_vaDisplay, m_pipelineParamID[i]);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
            m_pipelineParamID[i] = VA_INVALID_ID;
        }
    }


    // (3) info needed for sync operation
    //-------------------------------------------------------
    {
        UMC::AutomaticUMCMutex guard(m_guard);

        ExtVASurface currentFeedback; // {surface & number_of_task}
        currentFeedback.surface = *outputSurface;
        currentFeedback.number = pParams->statusReportID;
        m_feedbackCache.push_back(currentFeedback);
    }

    return MFX_ERR_NONE;
} // mfxStatus VAAPIVideoProcessing::Execute_Composition_TileVideoWall(mfxExecuteParams *pParams)

mfxStatus VAAPIVideoProcessing::Execute_Composition(mfxExecuteParams *pParams)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VAAPIVideoProcessing::Execute_Composition");

    VAStatus vaSts = VA_STATUS_SUCCESS;
    VASurfaceAttrib attrib;
    std::vector<VABlendState> blend_state;
    VAImage imagePrimarySurface;
    mfxU8* pPrimarySurfaceBuffer;

    MFX_CHECK_NULL_PTR1( pParams );
    MFX_CHECK_NULL_PTR1( pParams->targetSurface.hdl.first );
    MFX_CHECK_NULL_PTR1( pParams->pRefSurfaces );
    MFX_CHECK_NULL_PTR1( pParams->pRefSurfaces[0].hdl.first );

    mfxU32 refCount = (mfxU32) pParams->fwdRefCount;
    bool hasResize = false;

    for(mfxU32 i = 0; i < refCount; i++)
    {
        // Check if there is a resize for input streams
        mfxFrameInfo *surf_info = &(pParams->pRefSurfaces[i].frameInfo);
        if (surf_info->CropW != pParams->dstRects[i].DstW ||
            surf_info->CropH != pParams->dstRects[i].DstH){
            hasResize = true;
            break;
        }
    }

    if ((m_primarySurface4Composition == NULL) && (pParams->bBackgroundRequired))
    {
        mfxDrvSurface* pRefSurf = &(pParams->targetSurface);
        mfxFrameInfo *inInfo = &(pRefSurf->frameInfo);

        m_primarySurface4Composition = (VASurfaceID*)calloc(1,1*sizeof(VASurfaceID));
        /* required to check, is memory allocated o not  */
        if (m_primarySurface4Composition == NULL)
            return MFX_ERR_MEMORY_ALLOC;

        attrib.type = VASurfaceAttribPixelFormat;
        attrib.value.type = VAGenericValueTypeInteger;

        unsigned int rt_format;

        // default format is NV12
        if (inInfo->FourCC == MFX_FOURCC_RGB4)
        {
            attrib.value.value.i = VA_FOURCC_ARGB;
            rt_format = VA_RT_FORMAT_RGB32;
        }
        else
        {
            attrib.value.value.i = VA_FOURCC_NV12;
            rt_format = VA_RT_FORMAT_YUV420;
        }
        attrib.flags = VA_SURFACE_ATTRIB_SETTABLE;

        // Check what resolution is better. If input surfaces are going to be resized, then
        // it's better to allocate small surface for background. If there is no resize for
        // input streams, then it's better to allocate surface with the resolution equal to
        // the output stream to eliminate resize for background surface.
        mfxU32 width  = hasResize ? VPP_COMP_BACKGROUND_SURFACE_WIDTH  : inInfo->Width;
        mfxU32 height = hasResize ? VPP_COMP_BACKGROUND_SURFACE_HEIGHT : inInfo->Height;

        vaSts = vaCreateSurfaces(m_vaDisplay, rt_format, width, height,
                m_primarySurface4Composition, 1, &attrib, 1);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        for ( int iPrSurfCount = 0; iPrSurfCount < 1; iPrSurfCount++)
        {
            vaSts = vaDeriveImage(m_vaDisplay, m_primarySurface4Composition[iPrSurfCount], &imagePrimarySurface);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

            vaSts = vaMapBuffer(m_vaDisplay, imagePrimarySurface.buf, (void **) &pPrimarySurfaceBuffer);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

            mfxSize roiSize;
            roiSize.width = imagePrimarySurface.width;
            roiSize.height = imagePrimarySurface.height;

            /* We need to fill up empty surface by background color...
             * iBackgroundColor is now U64, with 16 bits per channel (see mfx_vpp_hw.cpp)
             * it is easy for ARGB format as Initial background value ARGB*/
            if (imagePrimarySurface.format.fourcc == VA_FOURCC_ARGB)
            {
                uint32_t A, R, G, B;
                uint32_t iBackgroundColorRGBA;

                A = (uint32_t)((pParams->iBackgroundColor >> 48) & 0x00ff);
                R = (uint32_t)((pParams->iBackgroundColor >> 32) & 0x00ff);
                G = (uint32_t)((pParams->iBackgroundColor >> 16) & 0x00ff);
                B = (uint32_t)((pParams->iBackgroundColor >>  0) & 0x00ff);

                iBackgroundColorRGBA = (A << 24) | (R << 16) | (G << 8) | (B << 0);

                bool setPlaneSts = SetPlaneROI<uint32_t>(iBackgroundColorRGBA, (uint32_t *)pPrimarySurfaceBuffer, imagePrimarySurface.pitches[0], roiSize);
                MFX_CHECK(setPlaneSts, MFX_ERR_DEVICE_FAILED);
            }
            /* A bit more complicated for NV12 as you need to do conversion ARGB => NV12 */
            if (imagePrimarySurface.format.fourcc == VA_FOURCC_NV12)
            {
                uint32_t Y = (uint32_t)((pParams->iBackgroundColor >> 32) & 0x00ff);
                uint32_t U = (uint32_t)((pParams->iBackgroundColor >> 16) & 0x00ff);
                uint32_t V = (uint32_t)((pParams->iBackgroundColor >>  0) & 0x00ff);

                uint8_t valueY = (uint8_t) Y;
                int16_t valueUV = (int16_t)((U<<8)  + V);

                bool setPlaneSts = SetPlaneROI<uint8_t>(valueY, pPrimarySurfaceBuffer, imagePrimarySurface.pitches[0], roiSize);
                MFX_CHECK(setPlaneSts, MFX_ERR_DEVICE_FAILED);

                // NV12 format -> need to divide height 2 times less
                roiSize.height = roiSize.height/2;
                // "UV" this is short (16 bit) value already
                // so need to divide width 2 times less too!
                roiSize.width = roiSize.width/2;
                setPlaneSts = SetPlaneROI<int16_t>(valueUV, (int16_t *)(pPrimarySurfaceBuffer + imagePrimarySurface.offsets[1]),
                                                imagePrimarySurface.pitches[1], roiSize);
                MFX_CHECK(setPlaneSts, MFX_ERR_DEVICE_FAILED);
            }

            vaSts = vaUnmapBuffer(m_vaDisplay, imagePrimarySurface.buf);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
            vaSts = vaDestroyImage(m_vaDisplay, imagePrimarySurface.image_id);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        } // for ( int iPrSurfCount = 0; iPrSurfCount < 3; iPrSurfCount++)
    }

    /* pParams->refCount is total number of processing surfaces:
     * in case of composition this is primary + sub streams*/

    mfxU32 SampleCount = 1;
    mfxU32 refIdx = 0;

    m_pipelineParam.resize(pParams->refCount + 1);
    m_pipelineParamID.resize(pParams->refCount + 1, VA_INVALID_ID);
    blend_state.resize(pParams->refCount + 1);

    std::vector<VARectangle> input_region;
    input_region.resize(pParams->refCount + 1);
    std::vector<VARectangle> output_region;
    output_region.resize(pParams->refCount + 1);
    VASurfaceID *outputSurface = (VASurfaceID*)(pParams->targetSurface.hdl.first);

    for( refIdx = 0; refIdx < SampleCount; refIdx++ )
    {
        mfxDrvSurface* pRefSurf = &(pParams->targetSurface);
        memset(&m_pipelineParam[refIdx], 0, sizeof(m_pipelineParam[refIdx]));

        //VASurfaceID* srf_1 = (VASurfaceID*)(pRefSurf->hdl.first);
        //m_pipelineParam[refIdx].surface = *srf_1;
        /* First "primary" surface should be our allocated empty surface filled by background color.
         * Else we can not process first input surface as usual one */
        if (pParams->bBackgroundRequired)
            m_pipelineParam[refIdx].surface = m_primarySurface4Composition[0];
        //VASurfaceID *outputSurface = (VASurfaceID*)(pParams->targetSurface.hdl.first);
        //m_pipelineParam[refIdx].surface = *outputSurface;

        // source cropping
        //mfxFrameInfo *inInfo = &(pRefSurf->frameInfo);
        mfxFrameInfo *outInfo = &(pParams->targetSurface.frameInfo);
        input_region[refIdx].y   = 0;
        input_region[refIdx].x   = 0;
        input_region[refIdx].height = outInfo->CropH;
        input_region[refIdx].width  = outInfo->CropW;
        m_pipelineParam[refIdx].surface_region = &input_region[refIdx];

        // destination cropping
        //mfxFrameInfo *outInfo = &(pParams->targetSurface.frameInfo);
        output_region[refIdx].y  = 0; //outInfo->CropY;
        output_region[refIdx].x   = 0; //outInfo->CropX;
        output_region[refIdx].height= outInfo->CropH;
        output_region[refIdx].width  = outInfo->CropW;
        m_pipelineParam[refIdx].output_region = &output_region[refIdx];

        /* Actually as background color managed by "m_primarySurface4Composition" surface
         * this param will not make sense */
        //m_pipelineParam[refIdx].output_background_color = pParams->iBackgroundColor;

        mfxU32  refFourcc = pRefSurf->frameInfo.FourCC;
        switch (refFourcc)
        {
        case MFX_FOURCC_RGB4:
            m_pipelineParam[refIdx].surface_color_standard = VAProcColorStandardNone;
            break;
        case MFX_FOURCC_NV12:
        default:
            m_pipelineParam[refIdx].surface_color_standard = VAProcColorStandardBT601;
            break;
        }

        mfxU32  targetFourcc = pParams->targetSurface.frameInfo.FourCC;
        switch (targetFourcc)
        {
        case MFX_FOURCC_RGB4:
            m_pipelineParam[refIdx].output_color_standard = VAProcColorStandardNone;
            break;
        case MFX_FOURCC_NV12:
        default:
            m_pipelineParam[refIdx].output_color_standard = VAProcColorStandardBT601;
            break;
        }

        if(refIdx > 0 && pParams->VideoSignalInfo[refIdx-1].enabled)
        {
            if(pParams->VideoSignalInfo[refIdx-1].TransferMatrix != MFX_TRANSFERMATRIX_UNKNOWN)
            {
                m_pipelineParam[refIdx].surface_color_standard = (MFX_TRANSFERMATRIX_BT709 == pParams->VideoSignalInfo[refIdx-1].TransferMatrix) ? VAProcColorStandardBT709 : VAProcColorStandardBT601;
            }

            if(pParams->VideoSignalInfo[refIdx-1].NominalRange != MFX_NOMINALRANGE_UNKNOWN)
            {
                m_pipelineParam[refIdx].input_color_properties.color_range = (MFX_NOMINALRANGE_0_255 == pParams->VideoSignalInfo[refIdx-1].NominalRange) ? VA_SOURCE_RANGE_FULL : VA_SOURCE_RANGE_REDUCED;
            }
        }

        if (pParams->VideoSignalInfoOut.enabled)
        {
            if(pParams->VideoSignalInfoOut.TransferMatrix != MFX_TRANSFERMATRIX_UNKNOWN)
            {
                m_pipelineParam[refIdx].output_color_standard = (MFX_TRANSFERMATRIX_BT709 == pParams->VideoSignalInfoOut.TransferMatrix ? VAProcColorStandardBT709 : VAProcColorStandardBT601);
            }

            if(pParams->VideoSignalInfoOut.NominalRange != MFX_NOMINALRANGE_UNKNOWN)
            {
                m_pipelineParam[refIdx].output_color_properties.color_range = (MFX_NOMINALRANGE_0_255 == pParams->VideoSignalInfoOut.NominalRange) ? VA_SOURCE_RANGE_FULL : VA_SOURCE_RANGE_REDUCED;
            }
        }
    m_pipelineParam[refIdx].input_color_properties.chroma_sample_location  = VA_CHROMA_SITING_UNKNOWN;
    m_pipelineParam[refIdx].output_color_properties.chroma_sample_location = VA_CHROMA_SITING_UNKNOWN;

        switch (pRefSurf->frameInfo.PicStruct)
        {
            case MFX_PICSTRUCT_PROGRESSIVE:
            default:
                m_pipelineParam[refIdx].filter_flags = VA_FRAME_PICTURE;
                break;
        }

        m_pipelineParam[refIdx].filters  = m_filterBufs;
        m_pipelineParam[refIdx].num_filters  = m_numFilterBufs;
        /* Special case for composition:
         * as primary surface processed as sub-stream
         * pipeline and filter properties should be *_FAST */
        if (pParams->bComposite)
        {
            m_pipelineParam[refIdx].num_filters  = 0;
#if defined(LINUX_TARGET_PLATFORM_BXT) || defined(LINUX_TARGET_PLATFORM_BXTMIN)
            m_pipelineParam[refIdx].pipeline_flags |= VA_PROC_PIPELINE_SUBPICTURES;
            m_pipelineParam[refIdx].filter_flags   |= VA_FILTER_SCALING_HQ;
#else
            m_pipelineParam[refIdx].pipeline_flags  |= VA_PROC_PIPELINE_FAST;
#endif
        }
    }

    {
        MFX_LTRACE_2(MFX_TRACE_LEVEL_HOTSPOTS, "A|VPP|COMP|PACKET_START|", "%d|%d", m_vaContextVPP, 0);
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_SCHED, "vaBeginPicture");
            vaSts = vaBeginPicture(m_vaDisplay,
                                m_vaContextVPP,
                                *outputSurface);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        }
    }

    if (pParams->bBackgroundRequired)
    {
        refIdx = 0;
        vaSts = vaCreateBuffer(m_vaDisplay,
                            m_vaContextVPP,
                            VAProcPipelineParameterBufferType,
                            sizeof(VAProcPipelineParameterBuffer),
                            1,
                            &m_pipelineParam[refIdx],
                            &m_pipelineParamID[refIdx]);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_SCHED, "vaRenderPicture");
            for( refIdx = 0; refIdx < SampleCount; refIdx++ )
            {
                vaSts = vaRenderPicture(m_vaDisplay, m_vaContextVPP, &m_pipelineParamID[refIdx], 1);
                MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
            }
        }
    } // if (pParams->bBackgroundRequired)

    unsigned int uBeginPictureCounter = 0;
    std::vector<VAProcPipelineParameterBuffer> m_pipelineParamComp;
    std::vector<VABufferID> m_pipelineParamCompID;
    /* for new buffers for Begin Picture*/
    m_pipelineParamComp.resize(pParams->fwdRefCount/7);
    m_pipelineParamCompID.resize(pParams->fwdRefCount/7, VA_INVALID_ID);

    /* pParams->fwdRefCount actually is number of sub stream*/
    for( refIdx = 1; refIdx <= (refCount + 1); refIdx++ )
    {
        /*for frames 8, 15, 22, 29,... */
        unsigned int uLastPass = (refCount + 1) - ( (refIdx /7) *7);
        if ((refIdx != 1) && ((refIdx %7) == 1) )
        {
            {
                vaSts = vaBeginPicture(m_vaDisplay,
                                    m_vaContextVPP,
                                    *outputSurface);
            }
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
            /*to copy initial properties of primary surface... */
            m_pipelineParamComp[uBeginPictureCounter] = m_pipelineParam[0];
            /* ... and to In-place output*/
            //m_pipelineParamComp[uBeginPictureCounter].surface = m_primarySurface4Composition[uInputIndex];
            m_pipelineParamComp[uBeginPictureCounter].surface = *outputSurface;
            //m_pipelineParam[0].surface = *outputSurface;
            /* As used IN-PLACE variant of Composition
             * this values does not used*/
            //uOutputIndex++;
            //uInputIndex++;
            //if (uOutputIndex > 2)
            //    uOutputIndex = 0;
            //if (uInputIndex > 2)
            //    uInputIndex = 0;

            switch (pParams->targetSurface.frameInfo.FourCC)
            {
            case MFX_FOURCC_RGB4:
                m_pipelineParamComp[uBeginPictureCounter].surface_color_standard = VAProcColorStandardNone;
                break;
            case MFX_FOURCC_NV12:
            default:
                m_pipelineParamComp[uBeginPictureCounter].surface_color_standard = (MFX_TRANSFERMATRIX_BT709 == pParams->VideoSignalInfoOut.TransferMatrix) ? VAProcColorStandardBT709 : VAProcColorStandardBT601;
                break;
            }

            vaSts = vaCreateBuffer(m_vaDisplay,
                                m_vaContextVPP,
                                VAProcPipelineParameterBufferType,
                                sizeof(VAProcPipelineParameterBuffer),
                                1,
                                &m_pipelineParamComp[uBeginPictureCounter],
                                &m_pipelineParamCompID[uBeginPictureCounter]);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_SCHED, "vaBeginPicture");
            vaSts = vaRenderPicture(m_vaDisplay, m_vaContextVPP, &m_pipelineParamCompID[uBeginPictureCounter], 1);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

            uBeginPictureCounter++;
        }

        m_pipelineParam[refIdx] = m_pipelineParam[0];

        mfxDrvSurface* pRefSurf = &(pParams->pRefSurfaces[refIdx-1]);

        VASurfaceID* srf_2 = (VASurfaceID*)(pRefSurf->hdl.first);

        m_pipelineParam[refIdx].surface = *srf_2;

        mfxU32  refFourcc = pRefSurf->frameInfo.FourCC;
        switch (refFourcc)
        {
        case MFX_FOURCC_RGB4:
            m_pipelineParam[refIdx].surface_color_standard = VAProcColorStandardNone;
            break;
        case MFX_FOURCC_NV12:
        default:
            m_pipelineParam[refIdx].surface_color_standard = VAProcColorStandardBT601;
            break;
        }

        if(refIdx > 0 && pParams->VideoSignalInfo[refIdx-1].enabled)
        {
            if(pParams->VideoSignalInfo[refIdx-1].TransferMatrix != MFX_TRANSFERMATRIX_UNKNOWN)
            {
                m_pipelineParam[refIdx].surface_color_standard = (MFX_TRANSFERMATRIX_BT709 == pParams->VideoSignalInfo[refIdx-1].TransferMatrix ? VAProcColorStandardBT709 : VAProcColorStandardBT601);
            }

            if(pParams->VideoSignalInfo[refIdx-1].NominalRange != MFX_NOMINALRANGE_UNKNOWN)
            {
                m_pipelineParam[refIdx].input_color_properties.color_range = (MFX_NOMINALRANGE_0_255 == pParams->VideoSignalInfo[refIdx-1].NominalRange) ? VA_SOURCE_RANGE_FULL : VA_SOURCE_RANGE_REDUCED;
            }
        }

        if (pParams->VideoSignalInfoOut.enabled)
        {
            if(pParams->VideoSignalInfoOut.TransferMatrix != MFX_TRANSFERMATRIX_UNKNOWN)
            {
                m_pipelineParam[refIdx].output_color_standard = (MFX_TRANSFERMATRIX_BT709 == pParams->VideoSignalInfoOut.TransferMatrix ? VAProcColorStandardBT709 : VAProcColorStandardBT601);
            }

            if(pParams->VideoSignalInfoOut.NominalRange != MFX_NOMINALRANGE_UNKNOWN)
            {
                m_pipelineParam[refIdx].output_color_properties.color_range = (MFX_NOMINALRANGE_0_255 == pParams->VideoSignalInfoOut.NominalRange) ? VA_SOURCE_RANGE_FULL : VA_SOURCE_RANGE_REDUCED;
            }
        }
        m_pipelineParam[refIdx].input_color_properties.chroma_sample_location  = VA_CHROMA_SITING_UNKNOWN;
        m_pipelineParam[refIdx].output_color_properties.chroma_sample_location = VA_CHROMA_SITING_UNKNOWN;

        /* to process input parameters of sub stream:
         * crop info and original size*/
        mfxFrameInfo *inInfo = &(pRefSurf->frameInfo);
        input_region[refIdx].y   = inInfo->CropY;
        input_region[refIdx].x   = inInfo->CropX;
        input_region[refIdx].height = inInfo->CropH;
        input_region[refIdx].width  = inInfo->CropW;
        m_pipelineParam[refIdx].surface_region = &input_region[refIdx];

        /* to process output parameters of sub stream:
         *  position and destination size */
        output_region[refIdx].y  = pParams->dstRects[refIdx-1].DstY;
        output_region[refIdx].x   = pParams->dstRects[refIdx-1].DstX;
        output_region[refIdx].height= pParams->dstRects[refIdx-1].DstH;
        output_region[refIdx].width  = pParams->dstRects[refIdx-1].DstW;
        m_pipelineParam[refIdx].output_region = &output_region[refIdx];

        /* Global alpha and luma key can not be enabled together*/
        /* Global alpha and luma key can not be enabled together*/
        if (pParams->dstRects[refIdx-1].GlobalAlphaEnable !=0)
        {
            blend_state[refIdx].flags = VA_BLEND_GLOBAL_ALPHA;
            blend_state[refIdx].global_alpha = ((float)pParams->dstRects[refIdx-1].GlobalAlpha) /255;
        }
        /* Luma color key  for YUV surfaces only.
         * And Premultiplied alpha blending for RGBA surfaces only.
         * So, these two flags can't combine together  */
        if ((pParams->dstRects[refIdx-1].LumaKeyEnable != 0) &&
            (pParams->dstRects[refIdx-1].PixelAlphaEnable == 0) )
        {
            blend_state[refIdx].flags |= VA_BLEND_LUMA_KEY;
            blend_state[refIdx].min_luma = ((float)pParams->dstRects[refIdx-1].LumaKeyMin/255);
            blend_state[refIdx].max_luma = ((float)pParams->dstRects[refIdx-1].LumaKeyMax/255);
        }
        if ((pParams->dstRects[refIdx-1].LumaKeyEnable == 0 ) &&
            (pParams->dstRects[refIdx-1].PixelAlphaEnable != 0 ) )
        {
            /* Per-pixel alpha case. Having VA_BLEND_PREMULTIPLIED_ALPHA as a parameter
             * leads to using BLEND_PARTIAL approach by driver that may produce
             * "white line"-like artifacts on transparent-opaque borders.
             * Setting nothing here triggers using a BLEND_SOURCE approach that is used on
             * Windows and looks to be free of such kind of artifacts */
            blend_state[refIdx].flags |= 0;
        }
        if ((pParams->dstRects[refIdx-1].GlobalAlphaEnable != 0) ||
                (pParams->dstRects[refIdx-1].LumaKeyEnable != 0) ||
                (pParams->dstRects[refIdx-1].PixelAlphaEnable != 0))
        {
            m_pipelineParam[refIdx].blend_state = &blend_state[refIdx];
        }

        //m_pipelineParam[refIdx].pipeline_flags = ?? //VA_PROC_PIPELINE_FAST or VA_PROC_PIPELINE_SUBPICTURES
        m_pipelineParam[refIdx].pipeline_flags  |= VA_PROC_PIPELINE_FAST;
        m_pipelineParam[refIdx].filter_flags    |= VA_FILTER_SCALING_FAST;

        m_pipelineParam[refIdx].filters  = m_filterBufs;
        m_pipelineParam[refIdx].num_filters  = 0;

        vaSts = vaCreateBuffer(m_vaDisplay,
                            m_vaContextVPP,
                            VAProcPipelineParameterBufferType,
                            sizeof(VAProcPipelineParameterBuffer),
                            1,
                            &m_pipelineParam[refIdx],
                            &m_pipelineParamID[refIdx]);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_SCHED, "vaRenderPicture");
        vaSts = vaRenderPicture(m_vaDisplay, m_vaContextVPP, &m_pipelineParamID[refIdx], 1);
        MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

        /*for frames 7, 14, 21, ...
         * or for the last frame*/
        if ( ((refIdx % 7) ==0) || ((refCount + 1) == refIdx) )
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_SCHED, "vaEndPicture");
            vaSts = vaEndPicture(m_vaDisplay, m_vaContextVPP);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
        }
    } /* for( refIdx = 1; refIdx <= (pParams->fwdRefCount); refIdx++ )*/
    MFX_LTRACE_2(MFX_TRACE_LEVEL_HOTSPOTS, "A|VPP|COMP|PACKET_END|", "%d|%d", m_vaContextVPP, 0);

    for( refIdx = 0; refIdx < m_pipelineParamCompID.size(); refIdx++ )
    {
        if ( m_pipelineParamCompID[refIdx] != VA_INVALID_ID)
        {
            vaSts = vaDestroyBuffer(m_vaDisplay, m_pipelineParamCompID[refIdx]);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
            m_pipelineParamCompID[refIdx] = VA_INVALID_ID;
        }
    }

    for( refIdx = 0; refIdx < m_pipelineParamID.size(); refIdx++ )
    {
        if ( m_pipelineParamID[refIdx] != VA_INVALID_ID)
        {
            vaSts = vaDestroyBuffer(m_vaDisplay, m_pipelineParamID[refIdx]);
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
            m_pipelineParamID[refIdx] = VA_INVALID_ID;
        }
    }

    // (3) info needed for sync operation
    //-------------------------------------------------------
    {
        UMC::AutomaticUMCMutex guard(m_guard);

        ExtVASurface currentFeedback; // {surface & number_of_task}
        currentFeedback.surface = *outputSurface;
        currentFeedback.number = pParams->statusReportID;
        m_feedbackCache.push_back(currentFeedback);
    }

    return MFX_ERR_NONE;
} // mfxStatus VAAPIVideoProcessing::Execute_Composition(mfxExecuteParams *pParams)
#else
mfxStatus VAAPIVideoProcessing::Execute_Composition_TiledVideoWall(mfxExecuteParams *pParams)
{
    return MFX_ERR_UNSUPPORTED;
}

mfxStatus VAAPIVideoProcessing::Execute_Composition(mfxExecuteParams *pParams)
{
    return MFX_ERR_UNSUPPORTED;
}
#endif //#ifdef MFX_ENABLE_VPP_COMPOSITION

mfxStatus VAAPIVideoProcessing::QueryTaskStatus(mfxU32 taskIndex)
{
    VAStatus vaSts;

    VASurfaceID waitSurface = VA_INVALID_SURFACE;
    mfxU32 indxSurf = 0;

    // (1) find params (sutface & number) are required by feedbackNumber
    //-----------------------------------------------
    {
        UMC::AutomaticUMCMutex guard(m_guard);

        for (indxSurf = 0; indxSurf < m_feedbackCache.size(); indxSurf++)
        {
            if (m_feedbackCache[indxSurf].number == taskIndex)
            {
                waitSurface = m_feedbackCache[indxSurf].surface;
                break;
            }
        }
        if (VA_INVALID_SURFACE == waitSurface)
        {
            return MFX_ERR_UNKNOWN;
        }

        m_feedbackCache.erase(m_feedbackCache.begin() + indxSurf);
    }

#if !defined(ANDROID)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaSyncSurface");
        vaSts = vaSyncSurface(m_vaDisplay, waitSurface);
        if (vaSts == VA_STATUS_ERROR_HW_BUSY)
            return MFX_ERR_GPU_HANG;
        else
            MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
    }
#endif

    return MFX_TASK_DONE;
} // mfxStatus VAAPIVideoProcessing::QueryTaskStatus(mfxU32 taskIndex)

#endif // #if defined (MFX_VA_LINUX)
#endif // #if defined (MFX_VPP_ENABLE)
/* EOF */
