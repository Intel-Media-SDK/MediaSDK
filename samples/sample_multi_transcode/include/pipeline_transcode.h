/******************************************************************************\
Copyright (c) 2005-2018, Intel Corporation
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

#ifndef __SAMPLE_PIPELINE_TRANSCODE_H__
#define __SAMPLE_PIPELINE_TRANSCODE_H__

#include <stddef.h>

#include "mfxplugin.h"
#include "mfxplugin++.h"

#include "vm/atomic_defs.h"
#include "vm/thread_defs.h"

#include <memory>
#include <vector>
#include <list>
#include <ctime>
#include <map>

#include "sample_defs.h"
#include "sample_utils.h"
#include "base_allocator.h"
#include "sysmem_allocator.h"
#include "rotate_plugin_api.h"
#include "mfx_multi_vpp.h"

#include "mfxvideo.h"
#include "mfxvideo++.h"
#include "mfxmvc.h"
#include "mfxjpeg.h"
#include "mfxla.h"
#include "mfxvp8.h"

#include "hw_device.h"
#include "plugin_loader.h"
#include "sample_defs.h"
#include "plugin_utils.h"

#if (MFX_VERSION >= 1024)
#include "brc_routines.h"
#endif

#include "vpp_ext_buffers_storage.h"

#define TIME_STATS 1 // Enable statistics processing
#include "time_statistics.h"

#if defined(_WIN32) || defined(_WIN64)
#include "decode_render.h"
#endif

#if defined(_WIN32) || defined(_WIN64)
    #define MSDK_CPU_ROTATE_PLUGIN  MSDK_STRING("sample_rotate_plugin.dll")
    #define MSDK_OCL_ROTATE_PLUGIN  MSDK_STRING("sample_plugin_opencl.dll")
#else
    #define MSDK_CPU_ROTATE_PLUGIN  MSDK_STRING("libsample_rotate_plugin.so")
    #define MSDK_OCL_ROTATE_PLUGIN  MSDK_STRING("libsample_plugin_opencl.so")
#endif

#define MAX_PREF_LEN    256

#ifndef MFX_VERSION
#error MFX_VERSION not defined
#endif

#ifdef ENABLE_MCTF
const mfxU16  MAX_NUM_OF_ATTACHED_BUFFERS_FOR_IN_SUFACE = 2;
const mfxU16  MCTF_MID_FILTER_STRENGTH = 10;
const mfxF64  MCTF_LOSSLESS_BPP = 0.0;
#endif

namespace TranscodingSample
{
    extern mfxU32 MFX_STDCALL TranscodeRoutine(void   *pObj);

    enum PipelineMode
    {
        Native = 0,        // means that pipeline is based depends on the cmd parameters (decode/encode/transcode)
        Sink,              // means that pipeline makes decode only and put data to shared buffer
        Source,            // means that pipeline makes vpp + encode and get data from shared buffer
        VppComp,           // means that pipeline makes vpp composition + encode and get data from shared buffer
        VppCompOnly,       // means that pipeline makes vpp composition and get data from shared buffer
        VppCompOnlyEncode  // means that pipeline makes vpp composition + encode and get data from shared buffer
    };

    enum VppCompDumpMode
    {
        NULL_RENDER_VPP_COMP = 1,
        DUMP_FILE_VPP_COMP = 2
    };

    enum EFieldCopyMode
    {
        FC_NONE=0,
        FC_T2T=1,
        FC_T2B=2,
        FC_B2T=4,
        FC_B2B=8,
        FC_FR2FR=16
    };

    struct sVppCompDstRect
    {
        mfxU32 DstX;
        mfxU32 DstY;
        mfxU32 DstW;
        mfxU32 DstH;
        mfxU16 TileId;
    };

#ifdef ENABLE_MCTF
    typedef enum
    {
        VPP_FILTER_DISABLED = 0,
        VPP_FILTER_ENABLED_DEFAULT = 1,
        VPP_FILTER_ENABLED_CONFIGURED = 7

    } VPPFilterMode;

    // this is a structure with mctf-parameteres
    // that can be changed in run-time;
    struct sMctfRunTimeParam
    {
        mfxU16 FilterStrength;
    };

    struct sMctfRunTimeParams
    {
        sMctfRunTimeParams() : CurIdx(0)
        {}

        mfxU32 CurIdx;
        std::vector<sMctfRunTimeParam> RunTimeParams;
        // returns rt-param corresponding to CurIdx or NULL if
        // CurIdx is behind available info
        const sMctfRunTimeParam* GetCurParam();
        // move CurIdx forward
        void MoveForward();
        // set CurIdx to the begining; restart indexing;
        void Restart();
        // reset vector & index
        void Reset();
        // test for emptiness
        bool Empty() { return RunTimeParams.empty(); };
    };

    struct sMCTFParam
    {
        sMctfRunTimeParams   rtParams;
        mfxExtVppMctf        params;
        VPPFilterMode        mode;
    };
#endif

    enum ExtBRCType {
        EXTBRC_DEFAULT,
        EXTBRC_OFF,
        EXTBRC_ON,
        EXTBRC_IMPLICIT
    };

    struct __sInputParams
    {
        // session parameters
        bool         bIsJoin;
        mfxPriority  priority;
        // common parameters
        mfxIMPL libType;  // Type of used mediaSDK library
        bool   bIsPerf;   // special performance mode. Use pre-allocated bitstreams, output
        mfxU16 nThreadsNum; // number of internal session threads number
        bool bRobustFlag;   // Robust transcoding mode. Allows auto-recovery after hardware errors

        mfxU32 EncodeId; // type of output coded video
        mfxU32 DecodeId; // type of input coded video

        msdk_char  strSrcFile[MSDK_MAX_FILENAME_LEN]; // source bitstream file
        msdk_char  strDstFile[MSDK_MAX_FILENAME_LEN]; // destination bitstream file
        msdk_char  strDumpVppCompFile[MSDK_MAX_FILENAME_LEN]; // VPP composition output dump file
        msdk_char  strMfxParamsDumpFile[MSDK_MAX_FILENAME_LEN];

        // specific encode parameters
        mfxU16 nTargetUsage;
        mfxF64 dDecoderFrameRateOverride;
        mfxF64 dEncoderFrameRateOverride;
        mfxF64 dVPPOutFramerate;
        mfxU16 nBitRate;
        mfxU16 nBitRateMultiplier;
        mfxU16 nQuality; // quality parameter for JPEG encoder
        mfxU16 nDstWidth;  // destination picture width, specified if resizing required
        mfxU16 nDstHeight; // destination picture height, specified if resizing required

        bool bEnableDeinterlacing;
        mfxU16 DeinterlacingMode;
        int DenoiseLevel;
        int DetailLevel;
        mfxU16 FRCAlgorithm;
        EFieldCopyMode fieldProcessingMode;

        mfxU16 nAsyncDepth; // asyncronous queue

        PipelineMode eMode;
        PipelineMode eModeExt;

        mfxU32 FrameNumberPreference; // how many surfaces user wants
        mfxU32 MaxFrameNumber; // maximum frames for transcoding
        mfxU32 numSurf4Comp;
        mfxU16 numTiles4Comp;

        mfxU16 nSlices; // number of slices for encoder initialization
        mfxU16 nMaxSliceSize; //maximum size of slice

        mfxU16 WinBRCMaxAvgKbps;
        mfxU16 WinBRCSize;
        mfxU16 BufferSizeInKB;
        mfxU16 GopPicSize;
        mfxU16 GopRefDist;
        mfxU16 NumRefFrame;
        mfxU16 nBRefType;
        mfxU16 RepartitionCheckMode;
        mfxU16 GPB;

        mfxU16 CodecLevel;
        mfxU16 CodecProfile;
        mfxU16 MaxKbps;
        mfxU16 InitialDelayInKB;
        mfxU16 GopOptFlag;

        mfxU16 WeightedPred;
        mfxU16 WeightedBiPred;
        mfxU16 ExtBrcAdaptiveLTR;

        bool   bExtMBQP;

        // MVC Specific Options
        bool   bIsMVC; // true if Multi-View-Codec is in use
        mfxU32 numViews; // number of views for Multi-View-Codec

        mfxU16 nRotationAngle; // if specified, enables rotation plugin in mfx pipeline
        msdk_char strVPPPluginDLLPath[MSDK_MAX_FILENAME_LEN]; // plugin dll path and name

        sPluginParams decoderPluginParams;
        sPluginParams encoderPluginParams;

        mfxU32 nTimeout; // how long transcoding works in seconds
        mfxU32 nFPS; // limit transcoding to the number of frames per second

        mfxU32 statisticsWindowSize;
        FILE* statisticsLogFile;

        bool bLABRC; // use look ahead bitrate control algorithm
        mfxU16 nLADepth; // depth of the look ahead bitrate control  algorithm
        bool bEnableExtLA;
        bool bEnableBPyramid;
        mfxU16 nRateControlMethod;
        mfxU16 nQPI;
        mfxU16 nQPP;
        mfxU16 nQPB;
        bool bDisableQPOffset;

        bool bOpenCL;
        mfxU16 reserved[4];

        mfxU16 nVppCompDstX;
        mfxU16 nVppCompDstY;
        mfxU16 nVppCompDstW;
        mfxU16 nVppCompDstH;
        mfxU16 nVppCompSrcW;
        mfxU16 nVppCompSrcH;
        mfxU16 nVppCompTileId;

        mfxU32 DecoderFourCC;
        mfxU32 EncoderFourCC;

        sVppCompDstRect* pVppCompDstRects;

        bool bUseOpaqueMemory;
        bool bForceSysMem;
        mfxU16 VppOutPattern;
        mfxU16 nGpuCopyMode;

        mfxU16 nRenderColorForamt; /*0 NV12 - default, 1 is ARGB*/

        mfxI32  monitorType;
        bool shouldUseGreedyFormula;
        bool enableQSVFF;
        bool bSingleTexture;

        ExtBRCType nExtBRC;

#if (MFX_VERSION >= 1025)
        mfxU16 numMFEFrames;
        mfxU16 MFMode;
        mfxU32 mfeTimeout;
#endif

#if defined(LIBVA_WAYLAND_SUPPORT)
        mfxU16 nRenderWinX;
        mfxU16 nRenderWinY;
        bool  bPerfMode;
#endif

#if defined(LIBVA_SUPPORT)
        mfxI32  libvaBackend;
#endif // defined(MFX_LIBVA_SUPPORT)

        CHWDevice             *m_hwdev;
    };

    struct sInputParams: public __sInputParams
    {
        sInputParams();
        msdk_string DumpLogFileName;
#if MFX_VERSION >= 1022
        std::vector<mfxExtEncoderROI> m_ROIData;

        bool bDecoderPostProcessing;
        bool bROIasQPMAP;
#endif //MFX_VERSION >= 1022
#ifdef ENABLE_MCTF
        sMCTFParam mctfParam;
#endif
    };

    struct PreEncAuxBuffer
    {
       mfxEncodeCtrl     encCtrl;
       mfxU16            Locked;
       mfxENCInput       encInput;
       mfxENCOutput      encOutput;
    };

    struct ExtendedSurface
    {
        mfxFrameSurface1 *pSurface;
        PreEncAuxBuffer  *pAuxCtrl;
        mfxEncodeCtrl    *pEncCtrl;
        mfxSyncPoint      Syncp;
    };

    struct ExtendedBS
    {
        ExtendedBS(): IsFree(true), Syncp(NULL), pCtrl(NULL)
        {
            MSDK_ZERO_MEMORY(Bitstream);
        };
        bool IsFree;
        mfxBitstream Bitstream;
        mfxSyncPoint Syncp;
        PreEncAuxBuffer* pCtrl;
    };

    class CIOStat : public CTimeStatistics
    {
        public:
            CIOStat()
              : CTimeStatistics()
              , ofile(stdout)
            {
                MSDK_ZERO_MEMORY(bufDir);
                DumpLogFileName.clear();
            }

            CIOStat(const msdk_char *dir)
              : CTimeStatistics()
              , ofile(stdout)
            {
                msdk_strncopy_s(bufDir, MAX_PREF_LEN, dir, MAX_PREF_LEN - 1);
                bufDir[MAX_PREF_LEN - 1] = 0;
            }

            ~CIOStat()
            {
            }

            inline void SetOutputFile(FILE *file)
            {
                ofile = file;
            }

            inline void SetDumpName(const msdk_char* name)
            {
                DumpLogFileName = name;
                if (!DumpLogFileName.empty())
                {
                    TurnOnDumping();
                }
                else
                {
                    TurnOffDumping();
                }
            }

            inline void SetDirection(const msdk_char *dir)
            {
                if (dir)
                {
                    msdk_strncopy_s(bufDir, MAX_PREF_LEN, dir, MAX_PREF_LEN - 1);
                    bufDir[MAX_PREF_LEN - 1] = 0;
                }
            }

            inline void PrintStatistics(mfxU32 numPipelineid, mfxF64 target_framerate = -1 /*default stands for infinite*/)
            {
                // print timings in ms
                msdk_fprintf(   ofile, MSDK_STRING("stat[%d.%llu]: %s=%d;Framerate=%.3f;Total=%.3lf;Samples=%lld;StdDev=%.3lf;Min=%.3lf;Max=%.3lf;Avg=%.3lf\n"),
                                msdk_get_current_pid(), rdtsc(),
                                bufDir, numPipelineid,
                                target_framerate,
                                GetTotalTime(false), GetNumMeasurements(),
                                GetTimeStdDev(false), GetMinTime(false), GetMaxTime(false), GetAvgTime(false));
                fflush(ofile);

                if(!DumpLogFileName.empty())
                {
                    msdk_char buf[MSDK_MAX_FILENAME_LEN];
                    msdk_sprintf(buf, MSDK_STRING("%s_ID_%d.log"), DumpLogFileName.c_str(), numPipelineid);
                    DumpDeltas(buf);
                }
            }

            inline void DumpDeltas(msdk_char* file_name)
            {
                if (m_time_deltas.empty())
                    return;

                FILE* dump_file = NULL;
                if (!MSDK_FOPEN(dump_file, file_name, MSDK_STRING("a")))
                {
                    for (std::vector<mfxF64>::const_iterator it = m_time_deltas.begin(); it != m_time_deltas.end(); ++it)
                    {
                        fprintf(dump_file, "%.3f, ", (*it));
                    }
                    fclose(dump_file);
                }
                else
                {
                    perror("DumpDeltas: file cannot be open");
                }
            }
        protected:
            msdk_tstring DumpLogFileName;
            FILE*     ofile;
            msdk_char bufDir[MAX_PREF_LEN];
    };


    class ExtendedBSStore
    {
    public:
        explicit ExtendedBSStore(mfxU32 size)
        {
            m_pExtBS.resize(size);
        }
        virtual ~ExtendedBSStore()
        {
            for (mfxU32 i=0; i < m_pExtBS.size(); i++)
                MSDK_SAFE_DELETE_ARRAY(m_pExtBS[i].Bitstream.Data);
            m_pExtBS.clear();

        }
        ExtendedBS* GetNext()
        {
            for (mfxU32 i=0; i < m_pExtBS.size(); i++)
            {
                if (m_pExtBS[i].IsFree)
                {
                    m_pExtBS[i].IsFree = false;
                    return &m_pExtBS[i];
                }
            }
            return NULL;
        }
        void Release(ExtendedBS* pBS)
        {
            for (mfxU32 i=0; i < m_pExtBS.size(); i++)
            {
                if (&m_pExtBS[i] == pBS)
                {
                    m_pExtBS[i].IsFree = true;
                    return;
                }
            }
            return;
        }
        void ReleaseAll()
        {
            for (mfxU32 i = 0; i < m_pExtBS.size(); i++)
            {
                m_pExtBS[i].IsFree = true;
            }
            return;
        }
        void FlushAll()
        {
            for (mfxU32 i = 0; i < m_pExtBS.size(); i++)
            {
                m_pExtBS[i].Bitstream.DataLength = 0;
                m_pExtBS[i].Bitstream.DataOffset = 0;
            }
            return;
        }
    protected:
        std::vector<ExtendedBS> m_pExtBS;

    private:
        DISALLOW_COPY_AND_ASSIGN(ExtendedBSStore);
    };

    class CTranscodingPipeline;
    // thread safety buffer heterogeneous pipeline
    // only for join sessions
    class SafetySurfaceBuffer
    {
    public:
        struct SurfaceDescriptor
        {
            ExtendedSurface   ExtSurface;
            mfxU32            Locked;
        };

        SafetySurfaceBuffer(SafetySurfaceBuffer *pNext);
        virtual ~SafetySurfaceBuffer();

        mfxU32            GetLength();
        mfxStatus         WaitForSurfaceRelease(mfxU32 msec);
        mfxStatus         WaitForSurfaceInsertion(mfxU32 msec);
        void              AddSurface(ExtendedSurface Surf);
        mfxStatus         GetSurface(ExtendedSurface &Surf);
        mfxStatus         ReleaseSurface(mfxFrameSurface1* pSurf);
        mfxStatus         ReleaseSurfaceAll();
        void              CancelBuffering();

        SafetySurfaceBuffer               *m_pNext;

    protected:

        MSDKMutex                 m_mutex;
        std::list<SurfaceDescriptor>       m_SList;
        bool m_IsBufferingAllowed;
        MSDKEvent* pRelEvent;
        MSDKEvent* pInsEvent;
    private:
        DISALLOW_COPY_AND_ASSIGN(SafetySurfaceBuffer);
    };

    class FileBitstreamProcessor
    {
    public:
        FileBitstreamProcessor();
        virtual ~FileBitstreamProcessor();
        virtual mfxStatus SetReader(std::unique_ptr<CSmplBitstreamReader>& reader);
        virtual mfxStatus SetReader(std::unique_ptr<CSmplYUVReader>& reader);
        virtual mfxStatus SetWriter(std::unique_ptr<CSmplBitstreamWriter>& writer);
        virtual mfxStatus GetInputBitstream(mfxBitstream **pBitstream);
        virtual mfxStatus GetInputFrame(mfxFrameSurface1 *pSurface);
        virtual mfxStatus ProcessOutputBitstream(mfxBitstream* pBitstream);
        virtual mfxStatus ResetInput();
        virtual mfxStatus ResetOutput();

    protected:
        std::unique_ptr<CSmplBitstreamReader> m_pFileReader;
        std::unique_ptr<CSmplYUVReader> m_pYUVFileReader;
        // for performance options can be zero
        std::unique_ptr<CSmplBitstreamWriter> m_pFileWriter;
        mfxBitstream m_Bitstream;
    private:
        DISALLOW_COPY_AND_ASSIGN(FileBitstreamProcessor);
    };

    // Bitstream is external via BitstreamProcessor
    class CTranscodingPipeline
    {
    public:
        CTranscodingPipeline();
        virtual ~CTranscodingPipeline();

        virtual mfxStatus Init(sInputParams *pParams,
                               MFXFrameAllocator *pMFXAllocator,
                               void* hdl,
                               CTranscodingPipeline *pParentPipeline,
                               SafetySurfaceBuffer  *pBuffer,
                               FileBitstreamProcessor   *pBSProc);

        // frames allocation is suspended for heterogeneous pipeline
        virtual mfxStatus CompleteInit();
        virtual void      Close();
        virtual mfxStatus Reset();
        virtual mfxStatus Join(MFXVideoSession *pChildSession);
        virtual mfxStatus Run();
        virtual mfxStatus FlushLastFrames(){return MFX_ERR_NONE;}

        mfxU32 GetProcessFrames() {return m_nProcessedFramesNum;}

        bool   GetJoiningFlag() {return m_bIsJoinSession;}

        mfxStatus QueryMFXVersion(mfxVersion *version)
        { MSDK_CHECK_POINTER(m_pmfxSession.get(), MFX_ERR_NULL_PTR); return m_pmfxSession->QueryVersion(version); };
        inline mfxU32 GetPipelineID(){return m_nID;}
        inline void SetPipelineID(mfxU32 id){m_nID = id;}
        void StopSession();
        bool IsOverlayUsed();
        size_t GetRobustFlag();

        msdk_string GetSessionText()
        {
            msdk_stringstream ss;
            ss << m_pmfxSession->operator mfxSession();

            return ss.str();
        }
    protected:
        virtual mfxStatus CheckRequiredAPIVersion(mfxVersion& version, sInputParams *pParams);

        virtual mfxStatus Decode();
        virtual mfxStatus Encode();
        virtual mfxStatus Transcode();
        virtual mfxStatus DecodeOneFrame(ExtendedSurface *pExtSurface);
        virtual mfxStatus DecodeLastFrame(ExtendedSurface *pExtSurface);
        virtual mfxStatus VPPOneFrame(ExtendedSurface *pSurfaceIn, ExtendedSurface *pExtSurface);
        virtual mfxStatus EncodeOneFrame(ExtendedSurface *pExtSurface, mfxBitstream *pBS);
        virtual mfxStatus PreEncOneFrame(ExtendedSurface *pInSurface, ExtendedSurface *pOutSurface);

        virtual mfxStatus DecodePreInit(sInputParams *pParams);
        virtual mfxStatus VPPPreInit(sInputParams *pParams);
        virtual mfxStatus EncodePreInit(sInputParams *pParams);
        virtual mfxStatus PreEncPreInit(sInputParams *pParams);

        mfxVideoParam GetDecodeParam();
        mfxExtOpaqueSurfaceAlloc GetDecOpaqueAlloc() const { return m_pmfxVPP.get() ? m_VppOpaqueAlloc: m_DecOpaqueAlloc; };

        mfxExtMVCSeqDesc GetDecMVCSeqDesc() const {return m_MVCSeqDesc;}

        // alloc frames for all component
        mfxStatus AllocFrames(mfxFrameAllocRequest  *pRequest, bool isDecAlloc);
        mfxStatus AllocFrames();

        mfxStatus CorrectPreEncAuxPool(mfxU32 num_frames_in_pool);
        mfxStatus AllocPreEncAuxPool();

        // need for heterogeneous pipeline
        mfxStatus CalculateNumberOfReqFrames(mfxFrameAllocRequest  &pRequestDecOut, mfxFrameAllocRequest  &pRequestVPPOut);
        void      CorrectNumberOfAllocatedFrames(mfxFrameAllocRequest  *pNewReq);
        void      FreeFrames();

        void      FreePreEncAuxPool();
        mfxStatus LoadStaticSurface();

        mfxFrameSurface1* GetFreeSurface(bool isDec, mfxU64 timeout);
        mfxU32 GetFreeSurfacesCount(bool isDec);
        PreEncAuxBuffer*  GetFreePreEncAuxBuffer();
        void SetEncCtrlRT(ExtendedSurface& extSurface, bool bInsertIDR);

        // parameters configuration functions
        mfxStatus InitDecMfxParams(sInputParams *pInParams);
        mfxStatus InitVppMfxParams(sInputParams *pInParams);
        virtual mfxStatus InitEncMfxParams(sInputParams *pInParams);
        mfxStatus InitPluginMfxParams(sInputParams *pInParams);
        mfxStatus InitPreEncMfxParams(sInputParams *pInParams);

        mfxStatus AllocAndInitVppDoNotUse(sInputParams *pInParams);
        mfxStatus AllocMVCSeqDesc();
        mfxStatus InitOpaqueAllocBuffers();

        void FreeVppDoNotUse();
        void FreeMVCSeqDesc();

        mfxStatus AllocateSufficientBuffer(mfxBitstream* pBS);
        mfxStatus PutBS();

        mfxStatus DumpSurface2File(mfxFrameSurface1* pSurface);
        mfxStatus Surface2BS(ExtendedSurface* pSurf,mfxBitstream* pBS, mfxU32 fourCC);
        mfxStatus NV12toBS(mfxFrameSurface1* pSurface,mfxBitstream* pBS);
        mfxStatus NV12asI420toBS(mfxFrameSurface1* pSurface, mfxBitstream* pBS);
        mfxStatus RGB4toBS(mfxFrameSurface1* pSurface,mfxBitstream* pBS);
        mfxStatus YUY2toBS(mfxFrameSurface1* pSurface,mfxBitstream* pBS);

        void NoMoreFramesSignal();
        mfxStatus AddLaStreams(mfxU16 width, mfxU16 height);

        void LockPreEncAuxBuffer(PreEncAuxBuffer* pBuff);
        void UnPreEncAuxBuffer  (PreEncAuxBuffer* pBuff);

        mfxU32 GetNumFramesForReset();
        void   SetNumFramesForReset(mfxU32 nFrames);

        void   HandlePossibleGpuHang(mfxStatus& sts);

        mfxStatus   SetAllocatorAndHandleIfRequired();
        mfxStatus   LoadGenericPlugin();

        mfxBitstream        *m_pmfxBS;  // contains encoded input data

        mfxVersion m_Version; // real API version with which library is initialized

        std::unique_ptr<MFXVideoSession>  m_pmfxSession;
        std::unique_ptr<MFXVideoDECODE>   m_pmfxDEC;
        std::unique_ptr<MFXVideoENCODE>   m_pmfxENC;
        std::unique_ptr<MFXVideoMultiVPP> m_pmfxVPP; // either VPP or VPPPlugin which wraps [VPP]-Plugin-[VPP] pipeline
        std::unique_ptr<MFXVideoENC>      m_pmfxPreENC;
        std::unique_ptr<MFXVideoUSER>     m_pUserDecoderModule;
        std::unique_ptr<MFXVideoUSER>     m_pUserEncoderModule;
        std::unique_ptr<MFXVideoUSER>     m_pUserEncModule;
        std::unique_ptr<MFXPlugin>        m_pUserDecoderPlugin;
        std::unique_ptr<MFXPlugin>        m_pUserEncoderPlugin;
        std::unique_ptr<MFXPlugin>        m_pUserEncPlugin;

        mfxFrameAllocResponse           m_mfxDecResponse;  // memory allocation response for decoder
        mfxFrameAllocResponse           m_mfxEncResponse;  // memory allocation response for encoder

        MFXFrameAllocator              *m_pMFXAllocator;
        void*                           m_hdl; // Diret3D device manager
        bool                            m_bIsInterOrJoined;

        mfxU32                          m_numEncoders;
        mfxU32                          m_encoderFourCC;

        CSmplYUVWriter                  m_dumpVppCompFileWriter;
        mfxU32                          m_vppCompDumpRenderMode;

#if defined(_WIN32) || defined(_WIN64)
        CDecodeD3DRender*               m_hwdev4Rendering;
#else
        CHWDevice*                      m_hwdev4Rendering;
#endif

        typedef std::vector<mfxFrameSurface1*> SurfPointersArray;
        SurfPointersArray  m_pSurfaceDecPool;
        SurfPointersArray  m_pSurfaceEncPool;
        mfxU16 m_EncSurfaceType; // actual type of encoder surface pool
        mfxU16 m_DecSurfaceType; // actual type of decoder surface pool

        typedef std::vector<PreEncAuxBuffer>    PreEncAuxArray;
        PreEncAuxArray                          m_pPreEncAuxPool;

        // transcoding pipeline specific
        typedef std::list<ExtendedBS*>       BSList;
        BSList  m_BSPool;

        mfxInitParam                   m_initPar;
        mfxExtThreadsParam             m_threadsPar;

        volatile bool                  m_bForceStop;

        sPluginParams                  m_decoderPluginParams;
        sPluginParams                  m_encoderPluginParams;

        mfxVideoParam                  m_mfxDecParams;
        mfxVideoParam                  m_mfxEncParams;
        mfxVideoParam                  m_mfxVppParams;
        mfxVideoParam                  m_mfxPluginParams;
        bool                           m_bIsVpp; // true if there's VPP in the pipeline
        bool                           m_bIsFieldWeaving;
        bool                           m_bIsFieldSplitting;
        bool                           m_bIsPlugin; //true if there's Plugin in the pipeline
        RotateParam                    m_RotateParam;
        mfxVideoParam                  m_mfxPreEncParams;
        mfxU32                         m_nTimeout;
        bool                           m_bUseOverlay;

        bool                           m_bROIasQPMAP;
        bool                           m_bExtMBQP;
        // various external buffers
        // for disabling VPP algorithms
        mfxExtVPPDoNotUse m_VppDoNotUse;
        // for MVC decoder and encoder configuration
        mfxExtMVCSeqDesc m_MVCSeqDesc;
        bool m_bOwnMVCSeqDescMemory; // true if the pipeline owns memory allocated for MVCSeqDesc structure fields

        mfxExtVPPComposite       m_VppCompParams;
#if MFX_VERSION >= 1022
        mfxExtDecVideoProcessing m_decPostProcessing;
#endif //MFX_VERSION >= 1022

        mfxExtLAControl          m_ExtLAControl;
        // for setting MaxSliceSize
        mfxExtCodingOption2      m_CodingOption2;

        mfxExtCodingOption3      m_CodingOption3;

        // HEVC
        mfxExtHEVCParam          m_ExtHEVCParam;

#if (MFX_VERSION >= 1024)
        mfxExtBRC                m_ExtBRC;
#endif

#if (MFX_VERSION >= 1025)
        // MFE mode and number of frames
        mfxExtMultiFrameParam    m_ExtMFEParam;
        // here we pass general timeout per session.
        mfxExtMultiFrameControl  m_ExtMFEControl;
#endif

        // for opaque memory
        mfxExtOpaqueSurfaceAlloc m_EncOpaqueAlloc;
        mfxExtOpaqueSurfaceAlloc m_VppOpaqueAlloc;
        mfxExtOpaqueSurfaceAlloc m_DecOpaqueAlloc;
        mfxExtOpaqueSurfaceAlloc m_PluginOpaqueAlloc;
        mfxExtOpaqueSurfaceAlloc m_PreEncOpaqueAlloc;

        // external parameters for each component are stored in a vector
        CVPPExtBuffersStorage        m_VppExtParamsStorage;
        std::vector<mfxExtBuffer*> m_EncExtParams;
        std::vector<mfxExtBuffer*> m_DecExtParams;
        std::vector<mfxExtBuffer*> m_PluginExtParams;
        std::vector<mfxExtBuffer*> m_PreEncExtParams;

        mfxU32         m_nID;
        mfxU16         m_AsyncDepth;
        mfxU32         m_nProcessedFramesNum;

        bool           m_bIsJoinSession;

        bool           m_bDecodeEnable;
        bool           m_bEncodeEnable;
        mfxU32         m_nVPPCompEnable;
        mfxI32         m_libvaBackend;

        bool           m_bUseOpaqueMemory; // indicates if opaque memory is used in the pipeline

        mfxSyncPoint   m_LastDecSyncPoint;

        SafetySurfaceBuffer   *m_pBuffer;
        CTranscodingPipeline  *m_pParentPipeline;

        mfxFrameAllocRequest   m_Request;
        bool                   m_bIsInit;

        mfxU32          m_NumFramesForReset;
        MSDKMutex       m_mReset;
        MSDKMutex       m_mStopSession;
        bool            m_bRobustFlag;

        bool isHEVCSW;

        bool m_bInsertIDR;

        std::unique_ptr<ExtendedBSStore>        m_pBSStore;

        mfxU32                                m_FrameNumberPreference;
        mfxU32                                m_MaxFramesForTranscode;

        // pointer to already extended bs processor
        FileBitstreamProcessor                   *m_pBSProcessor;

        msdk_tick m_nReqFrameTime; // time required to transcode one frame

        mfxU32    statisticsWindowSize; // Sliding window size for Statistics
        mfxU32    m_nOutputFramesNum;

        CIOStat inputStatistics;
        CIOStat outputStatistics;

        bool shouldUseGreedyFormula;

#if MFX_VERSION >= 1022
        // ROI data
        std::vector<mfxExtEncoderROI> m_ROIData;
        mfxU32         m_nSubmittedFramesNum;

        // ROI with MBQP map data
        bool              m_bUseQPMap;

        std::map<void*, mfxExtMBQP> m_bufExtMBQP;
        std::map<void*, std::vector<mfxU8> > m_qpMapStorage;
        std::map<void*, std::vector<mfxExtBuffer*> > m_extBuffPtrStorage;
        std::map<void*, mfxEncodeCtrl > encControlStorage;

        mfxU32            m_QPmapWidth;
        mfxU32            m_QPmapHeight;
        mfxU32            m_GOPSize;
        mfxU32            m_QPforI;
        mfxU32            m_QPforP;

        msdk_string       m_sGenericPluginPath;
        mfxU16            m_nRotationAngle;

        msdk_string       m_strMfxParamsDumpFile;

        void FillMBQPBuffer(mfxExtMBQP &qpMap, mfxU16 pictStruct);
#endif //MFX_VERSION >= 1022

#ifdef  ENABLE_MCTF
        sMctfRunTimeParams   m_MctfRTParams;
#endif
    private:
        DISALLOW_COPY_AND_ASSIGN(CTranscodingPipeline);

    };

    struct ThreadTranscodeContext
    {
        // Pointer to the session's pipeline
        std::unique_ptr<CTranscodingPipeline> pPipeline;
        // Pointer to bitstream handling object
        FileBitstreamProcessor *pBSProcessor;
        // Session implementation type
        mfxIMPL implType;

        // Session's starting status
        mfxStatus startStatus;
        // Session's working time
        mfxF64 working_time;

        // Number of processed frames
        mfxU32 numTransFrames;
        // Status of the finished session
        mfxStatus transcodingSts;

        ThreadTranscodeContext()
        {
            pBSProcessor = NULL;
            implType = MFX_IMPL_AUTO;
            startStatus = MFX_ERR_NONE;
            working_time = 0;
            numTransFrames = 0;
            transcodingSts = MFX_ERR_NONE;
        }
    };
}

#endif
