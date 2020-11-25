// Copyright (c) 2017-2020 Intel Corporation
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

#ifndef __MFX_VPP_VAAPI
#define __MFX_VPP_VAAPI

#include "umc_va_base.h"
#include "mfx_vpp_interface.h"
#include "mfx_platform_headers.h"

#include <va/va.h>
#include <va/va_vpp.h>

#include <assert.h>
#include <set>

namespace MfxHwVideoProcessing
{
    class VAAPIVideoProcessing : public DriverVideoProcessing
    {
    public:

        VAAPIVideoProcessing();

        virtual ~VAAPIVideoProcessing();

        virtual mfxStatus CreateDevice(VideoCORE * core, mfxVideoParam *pParams, bool isTemporal = false);

        virtual mfxStatus ReconfigDevice(mfxU32 /*indx*/) { return MFX_ERR_NONE; }

        virtual mfxStatus DestroyDevice( void );

        virtual mfxStatus Register(mfxHDLPair* pSurfaces,
                                   mfxU32 num,
                                   BOOL bRegister);

        virtual mfxStatus QueryTaskStatus(mfxU32 taskIndex);

        virtual mfxStatus QueryCapabilities( mfxVppCaps& caps );

        virtual mfxStatus QueryVariance(
            mfxU32 /* frameIndex */,
            std::vector<mfxU32> & /*variance*/) { return MFX_ERR_UNSUPPORTED; }

        virtual BOOL IsRunning() { return m_bRunning; }

        virtual mfxStatus Execute(mfxExecuteParams *pParams);

        virtual mfxStatus Execute_Composition(mfxExecuteParams *pParams);

        virtual mfxStatus Execute_Composition_TiledVideoWall(mfxExecuteParams *pParams);

    private:

        // VideoSignalInfo contains infor for backward reference, current and forward reference
        // frames. Indices in the info array are:
        //    { <indices for bwd refs>, <current>, <indices for fwd refs> }
        // For example, in case of ADI we will have: 1 bwd ref, 1 current, 0 fwd ref; indices will be:
        //    VideoSignalInfo[0] - for bwd ref
        //    VideoSignalInfo[1] - for cur reference
        inline int GetCurFrameSignalIdx(mfxExecuteParams* pParams)
        {
            if (!pParams) return -1;
            if ((size_t)pParams->bkwdRefCount < pParams->VideoSignalInfo.size()) {
                return pParams->bkwdRefCount;
            }
            return -1;
        }

        typedef struct _compositionStreamElement
        {
            mfxU16 index;
            BOOL   active;
            mfxU16 x;
            mfxU16 y;
            _compositionStreamElement()
                : index(0)
                , active(false)
                , x(0)
                , y(0)
            {};
        } compStreamElem;

        typedef struct vppTileVW{
            mfxU32 numChannels;
            VARectangle targerRect;
            mfxU8 channelIds[8];
        } m_tiledVideoWallParams;

        BOOL    isVideoWall(mfxExecuteParams *pParams);

        BOOL m_bRunning;

        VideoCORE* m_core;

        VADisplay   m_vaDisplay;
        VAConfigID  m_vaConfig;
        VAContextID m_vaContextVPP;

        VAProcFilterCap m_denoiseCaps;
        VAProcFilterCap m_detailCaps;

        VAProcPipelineCaps           m_pipelineCaps;
        VAProcFilterCapColorBalance  m_procampCaps[VAProcColorBalanceCount];
        VAProcFilterCapDeinterlacing m_deinterlacingCaps[VAProcDeinterlacingCount];
#ifdef MFX_ENABLE_VPP_FRC
        VAProcFilterCapFrameRateConversion m_frcCaps[2]; /* only two modes, 24p->60p and 30p->60p */
        mfxU32 m_frcCyclicCounter;
#endif

        VABufferID m_denoiseFilterID;
        VABufferID m_detailFilterID;
        VABufferID m_deintFilterID;
        VABufferID m_procampFilterID;
        VABufferID m_frcFilterID;
        VABufferID m_gpuPriorityID;
        mfxU32     m_deintFrameCount;
        VASurfaceID m_refForFRC[5];

        VABufferID m_filterBufs[VAProcFilterCount];
        mfxU32 m_numFilterBufs;

        std::vector<VAProcPipelineParameterBuffer> m_pipelineParam;
        std::vector<VABufferID> m_pipelineParamID;

        std::set<mfxU32> m_cachedReadyTaskIndex;

        mfxU32 m_MaxContextPriority;

        typedef struct
        {
            VASurfaceID surface;
            mfxU32 number;
        } ExtVASurface;

        VASurfaceID* m_primarySurface4Composition ;

        std::vector<ExtVASurface> m_feedbackCache;

        UMC::Mutex m_guard;

        mfxStatus Init( _mfxPlatformAccelerationService* pVADisplay, mfxVideoParam *pParams);

        mfxStatus Close( void );

        mfxStatus RemoveBufferFromPipe(VABufferID & id);
    };

}; // namespace

#endif //__MFX_VPP_VAAPI
#endif // MFX_VA_LINUX
#endif // MFX_ENABLE_VPP

/* EOF */
