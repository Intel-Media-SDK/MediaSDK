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

#ifndef __PIPELINE_ENCODE_H__
#define __PIPELINE_ENCODE_H__

#include "sample_defs.h"
#include "hw_device.h"

#ifdef D3D_SURFACES_SUPPORT
#pragma warning(disable : 4201)
#endif

#include "sample_utils.h"
#include "sample_params.h"
#include "base_allocator.h"
#include "time_statistics.h"

#include "mfxmvc.h"
#include "mfxvideo.h"
#include "mfxvideo++.h"
#include "mfxplugin.h"
#include "mfxplugin++.h"

#include <vector>
#include <memory>

#include "plugin_loader.h"

#if defined (ENABLE_V4L2_SUPPORT)
#include "v4l2_util.h"
#endif

#if (MFX_VERSION >= 1024)
#include "brc_routines.h"
#endif

msdk_tick time_get_tick(void);
msdk_tick time_get_frequency(void);

enum {
    MVC_DISABLED          = 0x0,
    MVC_ENABLED           = 0x1,
    MVC_VIEWOUTPUT        = 0x2,    // 2 output bitstreams
};

enum MemType {
    SYSTEM_MEMORY = 0x00,
    D3D9_MEMORY   = 0x01,
    D3D11_MEMORY  = 0x02,
};

enum ExtBRCType {
    EXTBRC_DEFAULT,
    EXTBRC_OFF,
    EXTBRC_ON,
    EXTBRC_IMPLICIT
};

struct sInputParams
{
    mfxU16 nTargetUsage;
    mfxU32 CodecId;
    mfxU32 FileInputFourCC;
    mfxU32 EncodeFourCC;
    mfxU16 nPicStruct;
    mfxU16 nWidth; // source picture width
    mfxU16 nHeight; // source picture height
    mfxF64 dFrameRate;
    mfxU32 nNumFrames;
    mfxU16 nBitRate;
    mfxU16 MVC_flags;
    mfxU16 nGopPicSize;
    mfxU16 nGopRefDist;
    mfxU16 nNumRefFrame;
    mfxU16 nBRefType;
    mfxU16 nPRefType;
    mfxU16 nIdrInterval;
    mfxU16 nNumRefActiveP;
    mfxU16 nNumRefActiveBL0;
    mfxU16 nNumRefActiveBL1;
    mfxU16 reserved[4];

    mfxU16 nQuality; // quality parameter for JPEG encoder

    mfxU32 numViews; // number of views for Multi-View Codec
    mfxU16 nDstWidth; // destination picture width, specified if resizing required
    mfxU16 nDstHeight; // destination picture height, specified if resizing required

    MemType memType;
    bool bUseHWLib; // true if application wants to use HW MSDK library

    std::list<msdk_string> InputFiles;

    sPluginParams pluginParams;

    std::vector<msdk_char*> dstFileBuff;

    mfxU32  HEVCPluginVersion;
    mfxU8 nRotationAngle; // if specified, enables rotation plugin in mfx pipeline
    msdk_char strPluginDLLPath[MSDK_MAX_FILENAME_LEN]; // plugin dll path and name

    mfxU16 nAsyncDepth; // depth of asynchronous pipeline, this number can be tuned to achieve better performance
    mfxU16 gpuCopy; // GPU Copy mode (three-state option)

    mfxU16 nRateControlMethod;
    mfxU16 nLADepth; // depth of the look ahead bitrate control  algorithm
    mfxU16 nMaxSliceSize; //maximum size of slice
    mfxU16 nQPI;
    mfxU16 nQPP;
    mfxU16 nQPB;

    mfxU16 nGPB;
    ExtBRCType nExtBRC;
    mfxU16 nAdaptiveMaxFrameSize;

    mfxU16 WeightedPred;
    mfxU16 WeightedBiPred;

    mfxU16 TransferMatrix;

    bool enableQSVFF;

    mfxU32 nTimeout;
    mfxU16 nMemBuf;

    mfxU16 nNumSlice;
    bool UseRegionEncode;

    bool isV4L2InputEnabled;

    mfxU16 CodecLevel;
    mfxU16 CodecProfile;
    mfxU16 MaxKbps;
    mfxU16 BufferSizeInKB;
    mfxU16 InitialDelayInKB;
    mfxU16 GopOptFlag;
    mfxU32 nMaxFrameSize;

    mfxU16 LowDelayBRC;

    mfxU16 IntRefType;
    mfxU16 IntRefCycleSize;
    mfxU16 IntRefQPDelta;
    mfxU16 IntRefCycleDist;

    bool bUncut;
    bool shouldUseShiftedP010Enc;
    bool shouldUseShiftedP010VPP;

    msdk_char DumpFileName[MSDK_MAX_FILENAME_LEN];
    msdk_char uSEI[MSDK_MAX_USER_DATA_UNREG_SEI_LEN];

#if defined (ENABLE_V4L2_SUPPORT)
    msdk_char DeviceName[MSDK_MAX_FILENAME_LEN];
    msdk_char MipiModeName[MSDK_MAX_FILENAME_LEN];
    enum V4L2PixelFormat v4l2Format;
    int MipiPort;
    enum AtomISPMode MipiMode;
#endif

};

struct sTask
{
    mfxBitstream mfxBS;
    mfxSyncPoint EncSyncP;
    std::list<mfxSyncPoint> DependentVppTasks;
    CSmplBitstreamWriter *pWriter;

    sTask();
    mfxStatus WriteBitstream();
    mfxStatus Reset();
    mfxStatus Init(mfxU32 nBufferSize, CSmplBitstreamWriter *pWriter = NULL);
    mfxStatus Close();
};

class CEncTaskPool
{
public:
    CEncTaskPool();
    virtual ~CEncTaskPool();

    virtual mfxStatus Init(MFXVideoSession* pmfxSession, CSmplBitstreamWriter* pWriter, mfxU32 nPoolSize, mfxU32 nBufferSize, CSmplBitstreamWriter *pOtherWriter = NULL);
    virtual mfxStatus GetFreeTask(sTask **ppTask);
    virtual mfxStatus SynchronizeFirstTask();

    virtual CTimeStatistics& GetOverallStatistics() { return m_statOverall;}
    virtual CTimeStatistics& GetFileStatistics() { return m_statFile;}
    virtual void Close();
protected:
    sTask* m_pTasks;
    mfxU32 m_nPoolSize;
    mfxU32 m_nTaskBufferStart;

    MFXVideoSession* m_pmfxSession;

    CTimeStatistics m_statOverall;
    CTimeStatistics m_statFile;
    virtual mfxU32 GetFreeTaskIndex();
};

/* This class implements a pipeline with 2 mfx components: vpp (video preprocessing) and encode */
class CEncodingPipeline
{
public:
    CEncodingPipeline();
    virtual ~CEncodingPipeline();

    virtual mfxStatus Init(sInputParams *pParams);
    virtual mfxStatus Run();
    virtual void Close();
    virtual mfxStatus ResetMFXComponents(sInputParams* pParams);
    virtual mfxStatus ResetDevice();

    void SetNumView(mfxU32 numViews) { m_nNumView = numViews; }
    virtual void  PrintInfo();

    void InitV4L2Pipeline(sInputParams *pParams);
    mfxStatus CaptureStartV4L2Pipeline();
    void CaptureStopV4L2Pipeline();

    void InsertIDR(bool bIsNextFrameIDR);

#if defined (ENABLE_V4L2_SUPPORT)
    v4l2Device v4l2Pipeline;
    pthread_t m_PollThread;
#endif

protected:
    std::pair<CSmplBitstreamWriter *,CSmplBitstreamWriter *> m_FileWriters;
    CSmplYUVReader m_FileReader;
    CEncTaskPool m_TaskPool;

    MFXVideoSession m_mfxSession;
    MFXVideoENCODE* m_pmfxENC;
    MFXVideoVPP* m_pmfxVPP;

    mfxVideoParam m_mfxEncParams;
    mfxVideoParam m_mfxVppParams;

    mfxU16 m_MVCflags; // MVC codec is in use

    mfxU32 m_InputFourCC;

    std::auto_ptr<MFXVideoUSER> m_pUserModule;
    std::auto_ptr<MFXPlugin> m_pPlugin;

    MFXFrameAllocator* m_pMFXAllocator;
    mfxAllocatorParams* m_pmfxAllocatorParams;
    MemType m_memType;
    mfxU16 m_nMemBuffer;
    bool m_bExternalAlloc; // use memory allocator as external for Media SDK

    mfxFrameSurface1* m_pEncSurfaces; // frames array for encoder input (vpp output)
    mfxFrameSurface1* m_pVppSurfaces; // frames array for vpp input
    mfxFrameAllocResponse m_EncResponse;  // memory allocation response for encoder
    mfxFrameAllocResponse m_VppResponse;  // memory allocation response for vpp

    mfxU32 m_nNumView;
    mfxU32 m_nFramesToProcess; // number of frames to process

    // for disabling VPP algorithms
    mfxExtVPPDoNotUse m_VppDoNotUse;
    // for MVC encoder and VPP configuration
    mfxExtMVCSeqDesc m_MVCSeqDesc;
    mfxExtCodingOption m_CodingOption;
    // for look ahead BRC configuration
    mfxExtCodingOption2 m_CodingOption2;
    // HEVC
    mfxExtHEVCParam m_ExtHEVCParam;
    mfxExtCodingOption3 m_CodingOption3;

    // Set up video signal information
    mfxExtVideoSignalInfo m_VideoSignalInfo;

#if (MFX_VERSION >= 1024)
    mfxExtBRC           m_ExtBRC;
#endif

    // external parameters for each component are stored in a vector
    std::vector<mfxExtBuffer*> m_VppExtParams;
    std::vector<mfxExtBuffer*> m_EncExtParams;

    std::vector<mfxPayload*> m_UserDataUnregSEI;

    CHWDevice *m_hwdev;

    bool isV4L2InputEnabled;

    mfxU32 m_nTimeout;

    bool   m_bFileWriterReset;
    mfxU32 m_nFramesRead;
    bool   m_bCutOutput;
    bool   m_bInsertIDR;
    bool   m_bTimeOutExceed;

    bool   m_bIsFieldSplitting;

    mfxEncodeCtrl m_encCtrl;

    CTimeStatisticsReal m_statOverall;
    CTimeStatisticsReal m_statFile;
    virtual mfxStatus InitMfxEncParams(sInputParams *pParams);
    virtual mfxStatus InitMfxVppParams(sInputParams *pParams);

    virtual mfxStatus InitFileWriters(sInputParams *pParams);
    virtual void FreeFileWriters();
    virtual mfxStatus InitFileWriter(CSmplBitstreamWriter **ppWriter, const msdk_char *filename);

    virtual mfxStatus AllocAndInitVppDoNotUse();
    virtual void FreeVppDoNotUse();

    virtual mfxStatus AllocAndInitMVCSeqDesc();
    virtual void FreeMVCSeqDesc();

    virtual mfxStatus CreateAllocator();
    virtual void DeleteAllocator();

    virtual mfxStatus CreateHWDevice();
    virtual void DeleteHWDevice();

    virtual mfxStatus AllocFrames();
    virtual void DeleteFrames();

    virtual mfxStatus AllocateSufficientBuffer(mfxBitstream* pBS);
    virtual mfxStatus FillBuffers();
    virtual mfxStatus LoadNextFrame(mfxFrameSurface1* pSurf);

    virtual mfxStatus GetFreeTask(sTask **ppTask);
    virtual MFXVideoSession& GetFirstSession(){return m_mfxSession;}
    virtual MFXVideoENCODE* GetFirstEncoder(){return m_pmfxENC;}

    virtual mfxU32 FileFourCC2EncFourCC(mfxU32 fcc);
};

#endif // __PIPELINE_ENCODE_H__
