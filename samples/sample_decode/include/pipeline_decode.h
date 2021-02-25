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

#ifndef __PIPELINE_DECODE_H__
#define __PIPELINE_DECODE_H__

#include "sample_defs.h"

#if D3D_SURFACES_SUPPORT
#pragma warning(disable : 4201)
#include <d3d9.h>
#include <dxva2api.h>
#endif

#include <vector>
#include "hw_device.h"
#include "decode_render.h"
#include "mfx_buffering.h"
#include <memory>

#include "sample_utils.h"
#include "base_allocator.h"

#include "mfxmvc.h"
#include "mfxjpeg.h"
#include "mfxplugin.h"
#include "mfxplugin++.h"
#include "mfxvideo.h"
#include "mfxvideo++.h"
#include "mfxvp8.h"

#include "plugin_loader.h"
#include "general_allocator.h"

#if defined(_WIN64) || defined(_WIN32)
#include "mfxadapter.h"
#endif

#ifndef MFX_VERSION
#error MFX_VERSION not defined
#endif

enum MemType {
    SYSTEM_MEMORY = 0x01,
    D3D9_MEMORY   = 0x02,
    D3D11_MEMORY  = 0x03,
};

enum eWorkMode {
  MODE_PERFORMANCE,
  MODE_RENDERING,
  MODE_FILE_DUMP
};

#if MFX_VERSION >= 1022
enum eDecoderPostProc {
  MODE_DECODER_POSTPROC_AUTO  = 0x1,
  MODE_DECODER_POSTPROC_FORCE = 0x2
};
#endif //MFX_VERSION >= 1022

struct sInputParams
{
    mfxU32 videoType;
    eWorkMode mode;
    MemType memType;
    bool    bUseHWLib; // true if application wants to use HW mfx library
    bool    bIsMVC; // true if Multi-View Codec is in use
    bool    bLowLat; // low latency mode
    bool    bCalLat; // latency calculation
    bool    bUseFullColorRange; //whether to use full color range
    mfxU16  nMaxFPS; // limits overall fps
    mfxU32  nWallCell;
    mfxU32  nWallW; //number of windows located in each row
    mfxU32  nWallH; //number of windows located in each column
    mfxU32  nWallMonitor; //monitor id, 0,1,.. etc
    bool    bWallNoTitle; //whether to show title for each window with fps value
#if MFX_VERSION >= 1022
    mfxU16  nDecoderPostProcessing;
#endif //MFX_VERSION >= 1022

    mfxU32  numViews; // number of views for Multi-View Codec
    mfxU32  nRotation; // rotation for Motion JPEG Codec
    mfxU16  nAsyncDepth; // asyncronous queue
    mfxU16  nTimeout; // timeout in seconds
    mfxU16  gpuCopy; // GPU Copy mode (three-state option)
    bool    bSoftRobustFlag;
    mfxU16  nThreadsNum;
    mfxI32  SchedulingType;
    mfxI32  Priority;

    mfxU16  Width;
    mfxU16  Height;

    mfxU32  fourcc;
    mfxU16  chromaType;
    mfxU32  nFrames;
    mfxU16  eDeinterlace;
    bool    outI420;

    bool    bPerfMode;
    bool    bRenderWin;
    mfxU32  nRenderWinX;
    mfxU32  nRenderWinY;
#if (MFX_VERSION >= 1025)
    bool    bErrorReport;
#endif

    mfxI32  monitorType;
#if defined(LIBVA_SUPPORT)
    mfxI32  libvaBackend;
#endif // defined(MFX_LIBVA_SUPPORT)

#if defined(LINUX32) || defined(LINUX64)
    std::string strDevicePath;
#endif
#if (defined(_WIN64) || defined(_WIN32)) && (MFX_VERSION >= 1031)
    bool bPrefferdGfx;
    bool bPrefferiGfx;
#endif

#if (MFX_VERSION >= 1034)
    bool bIgnoreLevelConstrain;
#endif

    msdk_char     strSrcFile[MSDK_MAX_FILENAME_LEN];
    msdk_char     strDstFile[MSDK_MAX_FILENAME_LEN];
    sPluginParams pluginParams;

    bool bDisableFilmGrain;
};

struct CPipelineStatistics
{
    CPipelineStatistics():
        m_input_count(0),
        m_output_count(0),
        m_synced_count(0),
        m_tick_overall(0),
        m_tick_fread(0),
        m_tick_fwrite(0),
        m_timer_overall(m_tick_overall)
    {
    }
    virtual ~CPipelineStatistics(){}

    mfxU32 m_input_count;     // number of received incoming packets (frames or bitstreams)
    mfxU32 m_output_count;    // number of delivered outgoing packets (frames or bitstreams)
    mfxU32 m_synced_count;

    msdk_tick m_tick_overall; // overall time passed during processing
    msdk_tick m_tick_fread;   // part of tick_overall: time spent to receive incoming data
    msdk_tick m_tick_fwrite;  // part of tick_overall: time spent to deliver outgoing data

    CAutoTimer m_timer_overall; // timer which corresponds to m_tick_overall

private:
    CPipelineStatistics(const CPipelineStatistics&);
    void operator=(const CPipelineStatistics&);
};

class CDecodingPipeline:
    public CBuffering,
    public CPipelineStatistics
{
public:
    CDecodingPipeline();
    virtual ~CDecodingPipeline();

    virtual mfxStatus Init(sInputParams *pParams);
    virtual mfxStatus RunDecoding();
    virtual void Close();
    virtual mfxStatus ResetDecoder(sInputParams *pParams);
    virtual mfxStatus ResetDevice();

    void SetMultiView();
    virtual void PrintInfo();
    mfxU64 GetTotalBytesProcessed() { return totalBytesProcessed + m_mfxBS.DataOffset; }

#if (MFX_VERSION >= 1025)
    inline void PrintDecodeErrorReport(mfxExtDecodeErrorReport *pDecodeErrorReport)
    {
        if (pDecodeErrorReport)
        {
            if (pDecodeErrorReport->ErrorTypes & MFX_ERROR_SPS)
                msdk_printf(MSDK_STRING("[Error] SPS Error detected!\n"));

            if (pDecodeErrorReport->ErrorTypes & MFX_ERROR_PPS)
                msdk_printf(MSDK_STRING("[Error] PPS Error detected!\n"));

            if (pDecodeErrorReport->ErrorTypes & MFX_ERROR_SLICEHEADER)
                msdk_printf(MSDK_STRING("[Error] SliceHeader Error detected!\n"));

            if (pDecodeErrorReport->ErrorTypes & MFX_ERROR_FRAME_GAP)
                msdk_printf(MSDK_STRING("[Error] Frame Gap Error detected!\n"));

        }
    }
#endif

protected: // functions
#if (defined(_WIN64) || defined(_WIN32)) && (MFX_VERSION >= 1031)
    mfxU32    GetPreferredAdapterNum(const mfxAdaptersInfo & adapters, const sInputParams & params);
#endif
    mfxStatus GetImpl(const sInputParams & params, mfxIMPL & impl);
    virtual mfxStatus CreateRenderingWindow(sInputParams *pParams);
    virtual mfxStatus InitMfxParams(sInputParams *pParams);

    virtual mfxStatus AllocateExtMVCBuffers();

    virtual void    DeallocateExtMVCBuffers();

    virtual mfxStatus InitVppParams();
    virtual mfxStatus InitVppFilters();
    virtual bool IsVppRequired(sInputParams *pParams);

    virtual mfxStatus CreateAllocator();
    virtual mfxStatus CreateHWDevice();
    virtual mfxStatus AllocFrames();
    virtual void DeleteFrames();
    virtual void DeleteAllocator();

    /** \brief Performs SyncOperation on the current output surface with the specified timeout.
     *
     * @return MFX_ERR_NONE Output surface was successfully synced and delivered.
     * @return MFX_ERR_MORE_DATA Array of output surfaces is empty, need to feed decoder.
     * @return MFX_WRN_IN_EXECUTION Specified timeout have elapsed.
     * @return MFX_ERR_UNKNOWN An error has occurred.
     */
    virtual mfxStatus SyncOutputSurface(mfxU32 wait);
    virtual mfxStatus DeliverOutput(mfxFrameSurface1* frame);
    virtual void PrintPerFrameStat(bool force = false);

    virtual void DeliverLoop();

    virtual mfxStatus ReallocCurrentSurface(const mfxFrameInfo & info);

protected: // variables
    CSmplYUVWriter                         m_FileWriter;
    std::unique_ptr<CSmplBitstreamReader>  m_FileReader;
    mfxBitstreamWrapper                    m_mfxBS; // contains encoded data
    mfxU64 totalBytesProcessed;

    MFXVideoSession         m_mfxSession;
    mfxIMPL                 m_impl;
    MFXVideoDECODE*         m_pmfxDEC;
    MFXVideoVPP*            m_pmfxVPP;
    MfxVideoParamsWrapper   m_mfxVideoParams;
    MfxVideoParamsWrapper   m_mfxVppVideoParams;
    std::unique_ptr<MFXVideoUSER>  m_pUserModule;
    std::unique_ptr<MFXPlugin> m_pPlugin;

    GeneralAllocator*       m_pGeneralAllocator;
    mfxAllocatorParams*     m_pmfxAllocatorParams;
    MemType                 m_memType;      // memory type of surfaces to use
    bool                    m_bExternalAlloc; // use memory allocator as external for Media SDK
    bool                    m_bDecOutSysmem; // use system memory between Decoder and VPP, if false - video memory
    mfxFrameAllocResponse   m_mfxResponse; // memory allocation response for decoder
    mfxFrameAllocResponse   m_mfxVppResponse;   // memory allocation response for vpp

    msdkFrameSurface*       m_pCurrentFreeSurface; // surface detached from free surfaces array
    msdkFrameSurface*       m_pCurrentFreeVppSurface; // VPP surface detached from free VPP surfaces array
    msdkOutputSurface*      m_pCurrentFreeOutputSurface; // surface detached from free output surfaces array
    msdkOutputSurface*      m_pCurrentOutputSurface; // surface detached from output surfaces array

    MSDKSemaphore*          m_pDeliverOutputSemaphore; // to access to DeliverOutput method
    MSDKEvent*              m_pDeliveredEvent; // to signal when output surfaces will be processed
    mfxStatus               m_error; // error returned by DeliverOutput method
    bool                    m_bStopDeliverLoop;

    eWorkMode               m_eWorkMode; // work mode for the pipeline
    bool                    m_bIsMVC; // enables MVC mode (need to support several files as an output)
    bool                    m_bIsVideoWall; // indicates special mode: decoding will be done in a loop
    bool                    m_bIsCompleteFrame;
    mfxU32                  m_fourcc; // color format of vpp out, i420 by default
    bool                    m_bPrintLatency;
    bool                    m_bOutI420;

    mfxU16                  m_vppOutWidth;
    mfxU16                  m_vppOutHeight;

    mfxU32                  m_nTimeout; // enables timeout for video playback, measured in seconds
    mfxU16                  m_nMaxFps; // limit of fps, if isn't specified equal 0.
    mfxU32                  m_nFrames; //limit number of output frames

    mfxU16                  m_diMode;
    bool                    m_bVppIsUsed;
    bool                    m_bVppFullColorRange;
    bool                    m_bSoftRobustFlag;
    std::vector<msdk_tick>  m_vLatency;

    FPSLimiter              m_fpsLimiter;

    mfxExtVPPVideoSignalInfo m_VppVideoSignalInfo;
    std::vector<mfxExtBuffer*> m_VppSurfaceExtParams;

#if defined(LINUX32) || defined(LINUX64)
    std::string              m_strDevicePath; //path to device for processing
#endif
    CHWDevice               *m_hwdev;
#if D3D_SURFACES_SUPPORT
    CDecodeD3DRender         m_d3dRender;
#endif

    bool                    m_bRenderWin;
    mfxU32                  m_nRenderWinX;
    mfxU32                  m_nRenderWinY;
    mfxU32                  m_nRenderWinW;
    mfxU32                  m_nRenderWinH;

    mfxU32                  m_export_mode;
    mfxI32                  m_monitorType;
#if defined(LIBVA_SUPPORT)
    mfxI32                  m_libvaBackend;
    bool                    m_bPerfMode;
#endif // defined(MFX_LIBVA_SUPPORT)

    bool                    m_bResetFileWriter;
    bool                    m_bResetFileReader;
private:
    CDecodingPipeline(const CDecodingPipeline&);
    void operator=(const CDecodingPipeline&);
};

#endif // __PIPELINE_DECODE_H__
