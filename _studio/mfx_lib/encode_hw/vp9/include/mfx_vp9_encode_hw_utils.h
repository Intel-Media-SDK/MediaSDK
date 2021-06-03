// Copyright (c) 2018-2019 Intel Corporation
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

#pragma once

#include "mfx_common.h"
#include "mfx_ext_buffers.h"
#include "mfx_trace.h"
#include "umc_mutex.h"
#include <vector>
#include <list>
#include <memory>
#include "mfxstructures.h"
#include "mfx_enc_common.h"
#include "assert.h"

static inline bool operator==(mfxVP9SegmentParam const& l, mfxVP9SegmentParam const& r)
{
    return MFX_EQ_FIELD(FeatureEnabled)
        && MFX_EQ_FIELD(QIndexDelta)
        && MFX_EQ_FIELD(LoopFilterLevelDelta)
        && MFX_EQ_FIELD(ReferenceFrame);
}

namespace MfxHwVP9Encode
{

constexpr auto DPB_SIZE = 8; // DPB size by VP9 spec
constexpr auto DPB_SIZE_REAL = 3; // DPB size really used by encoder
constexpr auto MAX_SEGMENTS = 8;
constexpr auto REF_FRAMES_LOG2 = 3;
constexpr auto REF_FRAMES = (1 << REF_FRAMES_LOG2);
constexpr auto MAX_REF_LF_DELTAS = 4;
constexpr auto MAX_MODE_LF_DELTAS = 2;
constexpr auto SEG_LVL_MAX = 4;

constexpr auto IVF_SEQ_HEADER_SIZE_BYTES = 32;
constexpr auto IVF_PIC_HEADER_SIZE_BYTES = 12;
constexpr auto MAX_IVF_HEADER_SIZE = IVF_SEQ_HEADER_SIZE_BYTES + IVF_PIC_HEADER_SIZE_BYTES;

constexpr auto MAX_Q_INDEX = 255;
constexpr auto MAX_ICQ_QUALITY_INDEX = 255;
constexpr auto MAX_LF_LEVEL = 63;

constexpr auto MAX_ABS_COEFF_TYPE_Q_INDEX_DELTA = 15;

constexpr auto MAX_TASK_ID = 0xffff;
constexpr auto MAX_NUM_TEMP_LAYERS = 8;
constexpr auto MAX_NUM_TEMP_LAYERS_SUPPORTED = 4;

constexpr auto MAX_UPSCALE_RATIO = 16;
constexpr auto MAX_DOWNSCALE_RATIO = 2;

constexpr auto MIN_TILE_HEIGHT = 128;
constexpr auto MIN_TILE_WIDTH = 256;
constexpr auto MAX_TILE_WIDTH = 4096;
constexpr auto MAX_NUM_TILE_ROWS = 4;
constexpr auto MAX_NUM_TILES = 16;

const mfxU16 segmentSkipMask = 0xf0;
const mfxU16 segmentRefMask = 0x0f;

enum
{
    MFX_MEMTYPE_D3D_INT = MFX_MEMTYPE_FROM_ENCODE | MFX_MEMTYPE_DXVA2_DECODER_TARGET | MFX_MEMTYPE_INTERNAL_FRAME,
    MFX_MEMTYPE_D3D_EXT = MFX_MEMTYPE_FROM_ENCODE | MFX_MEMTYPE_DXVA2_DECODER_TARGET | MFX_MEMTYPE_EXTERNAL_FRAME,
    MFX_MEMTYPE_SYS_EXT = MFX_MEMTYPE_FROM_ENCODE | MFX_MEMTYPE_SYSTEM_MEMORY        | MFX_MEMTYPE_EXTERNAL_FRAME,
    MFX_MEMTYPE_SYS_INT = MFX_MEMTYPE_FROM_ENCODE | MFX_MEMTYPE_SYSTEM_MEMORY        | MFX_MEMTYPE_INTERNAL_FRAME
};

static const mfxU16 MFX_IOPATTERN_IN_MASK_SYS_OR_D3D =
    MFX_IOPATTERN_IN_SYSTEM_MEMORY | MFX_IOPATTERN_IN_VIDEO_MEMORY;

static const mfxU16 MFX_IOPATTERN_IN_MASK =
    MFX_IOPATTERN_IN_MASK_SYS_OR_D3D | MFX_IOPATTERN_IN_OPAQUE_MEMORY;

static const mfxU16 MFX_MEMTYPE_SYS_OR_D3D =
MFX_MEMTYPE_DXVA2_DECODER_TARGET | MFX_MEMTYPE_SYSTEM_MEMORY;

enum // identifies memory type at encoder input w/o any details
{
    INPUT_SYSTEM_MEMORY,
    INPUT_VIDEO_MEMORY
};


    enum eTaskStatus
    {
        TASK_FREE = 0,
        TASK_INITIALIZED,
        TASK_SUBMITTED
    };
    enum
    {
        REF_LAST  = 0,
        REF_GOLD  = 1,
        REF_ALT   = 2,
        REF_TOTAL = 3
    };

    enum {
        KEY_FRAME       = 0,
        INTER_FRAME     = 1,
        NUM_FRAME_TYPES
    };

    enum {
        UNKNOWN_COLOR_SPACE = 0,
        BT_601              = 1,
        BT_709              = 2,
        SMPTE_170           = 3,
        SMPTE_240           = 4,
        BT_2020             = 5,
        RESERVED            = 6,
        SRGB                = 7
    };

    enum {
        BITDEPTH_8 = 8,
        BITDEPTH_10 = 10,
        BITDEPTH_12 = 12
    };

    enum {
        PROFILE_0 = 0,
        PROFILE_1 = 1,
        PROFILE_2 = 2,
        PROFILE_3 = 3,
        MAX_PROFILES
    };

    // Sequence level parameters should contain only parameters that will not change during encoding
    struct VP9SeqLevelParam
    {
        mfxU8  profile;
        mfxU8  bitDepth;;
        mfxU8  colorSpace;
        mfxU8  colorRange;
        mfxU8  subsamplingX;
        mfxU8  subsamplingY;

        mfxU8  frameParallelDecoding;
    };

    enum
    {
        NO_SEGMENTATION = 0,
        APP_SEGMENTATION = 1,
        BRC_SEGMENTATION = 2
    };

    struct VP9FrameLevelParam
    {
        mfxU8  frameType;
        mfxU8  baseQIndex;
        mfxI8  qIndexDeltaLumaDC;
        mfxI8  qIndexDeltaChromaAC;
        mfxI8  qIndexDeltaChromaDC;

        mfxU32 width;
        mfxU32 height;
        mfxU32 renderWidth;
        mfxU32 renderHeight;

        mfxU8  refreshFrameContext;
        mfxU8  resetFrameContext;
        mfxU8  refList[REF_TOTAL]; // indexes of last, gold, alt refs for current frame
        mfxU8  refBiases[REF_TOTAL];
        mfxU8  refreshRefFrames[DPB_SIZE]; // which reference frames are refreshed with current reconstructed frame
        mfxU16 modeInfoCols;
        mfxU16 modeInfoRows;
        mfxU8  allowHighPrecisionMV;

        mfxU8  showFrame;
        mfxU8  intraOnly;

        mfxU8  lfLevel;
        mfxI8  lfRefDelta[4];
        mfxI8  lfModeDelta[2];
        mfxU8  modeRefDeltaEnabled;
        mfxU8  modeRefDeltaUpdate;

        mfxU8  sharpness;
        mfxU8  segmentation;
        mfxU8  segmentationUpdateMap;
        mfxU8  segmentationUpdateData;
        mfxU8  segmentationTemporalUpdate;
        mfxU16  segmentIdBlockSize;
        mfxU8  errorResilentMode;
        mfxU8  interpFilter;
        mfxU8  frameContextIdx;
        mfxU8  log2TileCols;
        mfxU8  log2TileRows;

        mfxU16  temporalLayer;
        mfxU16  nextTemporalLayer;
    };

    struct sFrameEx
    {
        mfxFrameSurface1 *pSurface;
        mfxU32            idInPool;
        mfxU32            frameOrder;
        mfxU8             refCount;
    };

    inline mfxStatus LockSurface(sFrameEx*  pFrame, VideoCORE* pCore)
    {
        return (pFrame) ? pCore->IncreaseReference(&pFrame->pSurface->Data) : MFX_ERR_NONE;
    }
    inline mfxStatus FreeSurface(sFrameEx* &pFrame, VideoCORE* pCore)
    {
        mfxStatus sts = MFX_ERR_NONE;
        if (pFrame && pFrame->pSurface)
        {
            sts = pCore->DecreaseReference(&pFrame->pSurface->Data);
            pFrame = 0;
        }
        return sts;
    }
    inline bool isFreeSurface (sFrameEx* pFrame)
    {
        return (pFrame->pSurface->Data.Locked == 0);
    }
    inline void IncreaseRef(sFrameEx* &pFrame)
    {
        pFrame->refCount++;
    }
    inline mfxStatus DecreaseRef(sFrameEx* &pFrame, VideoCORE* pCore)
    {
        if (pFrame->refCount)
        {
            pFrame->refCount--;
            if (pFrame->refCount == 0)
            {
                mfxStatus sts = FreeSurface(pFrame, pCore);
                MFX_CHECK_STS(sts);
            }
        }

        return MFX_ERR_NONE;
    }

    template<class T> inline void Zero(T & obj)                { obj = T(); }
    template<class T, size_t N> inline void Zero(T (&obj)[N])  { std::fill_n(obj, N, T()); }
    template<class T> inline void Zero(std::vector<T> & vec)   { vec.assign(vec.size(), T()); }
    template<class T> inline void ZeroExtBuffer(T & obj)
    {
        mfxExtBuffer header = obj.Header;
        obj = T();
        obj.Header = header;
    }

    inline mfxU32 CeilDiv(mfxU32 x, mfxU32 y) { return (x + y - 1) / y; }
    inline mfxU32 CeilLog2(mfxU32 x) { mfxU32 l = 0; while (x > (1U << l)) l++; return l; }
    inline mfxU32 FloorLog2(mfxU32 x) { mfxU32 l = 0; while (x >= (1U << (l + 1))) l++; return l; }

    template<class T> struct ExtBufTypeToId {};

#define BIND_EXTBUF_TYPE_TO_ID(TYPE, ID) template<> struct ExtBufTypeToId<TYPE> { enum { id = ID }; }
    BIND_EXTBUF_TYPE_TO_ID (mfxExtVP9Param,  MFX_EXTBUFF_VP9_PARAM);
    BIND_EXTBUF_TYPE_TO_ID (mfxExtOpaqueSurfaceAlloc,MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
    BIND_EXTBUF_TYPE_TO_ID (mfxExtCodingOption2, MFX_EXTBUFF_CODING_OPTION2);
    BIND_EXTBUF_TYPE_TO_ID (mfxExtCodingOption3, MFX_EXTBUFF_CODING_OPTION3);
    BIND_EXTBUF_TYPE_TO_ID (mfxExtCodingOptionDDI, MFX_EXTBUFF_DDI);
    BIND_EXTBUF_TYPE_TO_ID (mfxExtVP9Segmentation, MFX_EXTBUFF_VP9_SEGMENTATION);
    BIND_EXTBUF_TYPE_TO_ID (mfxExtVP9TemporalLayers, MFX_EXTBUFF_VP9_TEMPORAL_LAYERS);
#undef BIND_EXTBUF_TYPE_TO_ID

    template <class T> inline void InitExtBufHeader(T & extBuf)
    {
        Zero(extBuf);
        extBuf.Header.BufferId = ExtBufTypeToId<T>::id;
        extBuf.Header.BufferSz = sizeof(T);
    }

    template <class T> struct GetPointedType {};
    template <class T> struct GetPointedType<T *> { typedef T Type; };
    template <class T> struct GetPointedType<T const *> { typedef T Type; };

    struct mfxExtBufferProxy
    {
    public:
        template <typename T> operator T()
        {
            mfxExtBuffer * p = GetExtBuffer(
                m_extParam,
                m_numExtParam,
                ExtBufTypeToId<typename GetPointedType<T>::Type>::id);
            return reinterpret_cast<T>(p);
        }

        template <typename T> friend mfxExtBufferProxy GetExtBuffer(const T & par);

    protected:
        mfxExtBufferProxy(mfxExtBuffer ** extParam, mfxU32 numExtParam)
            : m_extParam(extParam)
            , m_numExtParam(numExtParam)
        {
        }

    private:
        mfxExtBuffer ** m_extParam;
        mfxU32          m_numExtParam;
    };

    template <typename T> mfxExtBufferProxy GetExtBuffer(T const & par)
    {
        return mfxExtBufferProxy(par.ExtParam, par.NumExtParam);
    }

struct mfxExtBufferRefProxy{
public:
    template <typename T> operator T&()
    {
        mfxExtBuffer * p = GetExtBuffer(
            m_extParam,
            m_numExtParam,
            ExtBufTypeToId<typename GetPointedType<T*>::Type>::id);
        assert(p);
        return *(reinterpret_cast<T*>(p));
    }

    template <typename T> friend mfxExtBufferRefProxy GetExtBufferRef(const T & par);

protected:
    mfxExtBufferRefProxy(mfxExtBuffer ** extParam, mfxU32 numExtParam)
        : m_extParam(extParam)
        , m_numExtParam(numExtParam)
    {
    }
private:
    mfxExtBuffer ** m_extParam;
    mfxU32          m_numExtParam;
};

template <typename T> mfxExtBufferRefProxy GetExtBufferRef(T const & par)
{
    return mfxExtBufferRefProxy(par.ExtParam, par.NumExtParam);
}

class VP9MfxVideoParam;

struct ActualExtBufferExtractor {
public:
    template <typename T> operator T&()
    {
        mfxExtBuffer * p = GetExtBuffer(
            m_newParam,
            m_newNum,
            ExtBufTypeToId<typename GetPointedType<T*>::Type>::id);
        if (p)
        {
            return *(reinterpret_cast<T*>(p));
        }
        else
        {
            p = GetExtBuffer(
                m_basicParam,
                m_basicNum,
                ExtBufTypeToId<typename GetPointedType<T*>::Type>::id);
            assert(p);
            return *(reinterpret_cast<T*>(p));
        }
    }

    template <typename T> friend ActualExtBufferExtractor GetActualExtBufferRef(const VP9MfxVideoParam & basicPar, const T & newPar);

protected:
    ActualExtBufferExtractor(mfxExtBuffer ** basicParam, mfxU32 basicNum,
        mfxExtBuffer ** newParam, mfxU32 newNum)
        : m_basicParam(basicParam)
        , m_basicNum(basicNum)
        , m_newParam(newParam)
        , m_newNum(newNum)

    {
    }
private:
    mfxExtBuffer ** m_basicParam;
    mfxU32          m_basicNum;
    mfxExtBuffer ** m_newParam;
    mfxU32          m_newNum;
};

// remove first entry of extended buffer
template <typename T> mfxStatus RemoveExtBuffer(T & par, mfxU32 id)
{
    if (par.ExtParam == 0)
    {
        return MFX_ERR_NONE;
    }

    for (mfxU16 i = 0; i < par.NumExtParam; i++)
    {
        mfxExtBuffer* pBuf = par.ExtParam[i];
        MFX_CHECK_NULL_PTR1(pBuf);
        if (pBuf->BufferId == id)
        {
            for (mfxU16 j = i + 1; j < par.NumExtParam; j++)
            {
                par.ExtParam[j - 1] = par.ExtParam[j];
                par.ExtParam[j] = 0;
            }
            par.NumExtParam--;
            break;
        }
    }

    return MFX_ERR_NONE;
}

    class MfxFrameAllocResponse : public mfxFrameAllocResponse
    {
    public:
        MfxFrameAllocResponse();

        MfxFrameAllocResponse(MfxFrameAllocResponse const &) = delete;
        MfxFrameAllocResponse & operator =(MfxFrameAllocResponse const &) = delete;

        ~MfxFrameAllocResponse();

        mfxStatus Alloc(
            VideoCORE* pCore,
            mfxFrameAllocRequest & req,
            bool isCopyRequired);

        mfxStatus Release();

        mfxFrameInfo               m_info;

    private:
        VideoCORE* m_pCore;
        mfxU16      m_numFrameActualReturnedByAllocFrames;

        std::vector<mfxFrameAllocResponse> m_responseQueue;
        std::vector<mfxMemId>              m_mids;
    };

    struct FrameLocker
    {
        FrameLocker(VideoCORE * pCore, mfxFrameData & data)
            : m_core(pCore)
            , m_data(data)
            , m_memId(data.MemId)
            , m_status(Lock())
        {
        }

        FrameLocker(VideoCORE * pCore, mfxFrameData & data, mfxMemId memId)
            : m_core(pCore)
            , m_data(data)
            , m_memId(memId)
            , m_status(Lock())
        {
        }

        ~FrameLocker() { Unlock(); }

        mfxStatus Unlock()
        {
            mfxStatus mfxSts = MFX_ERR_NONE;
            mfxSts = m_core->UnlockFrame(m_memId, &m_data);

            m_status = LOCK_NO;
            return mfxSts;
        }

    protected:
        enum { LOCK_NO, LOCK_DONE };

        mfxU32 Lock()
        {
            mfxU32 status = LOCK_NO;

            if (m_data.Y == 0 &&
                MFX_ERR_NONE == m_core->LockFrame(m_memId, &m_data))
                status = LOCK_DONE;

            return status;
        }

    private:
        FrameLocker(FrameLocker const &);
        FrameLocker & operator =(FrameLocker const &);

        VideoCORE*         m_core;
        mfxFrameData&      m_data;
        mfxMemId           m_memId;
        mfxU32             m_status;
    };

    struct TempLayerParam
    {
        mfxU16 Scale;
        mfxU32 targetKbps;
    };

constexpr auto NUM_OF_SUPPORTED_EXT_BUFFERS = 7; // mfxExtVP9Param, mfxExtOpaqueSurfaceAlloc, mfxExtCodingOption2, mfxExtCodingOption3, mfxExtCodingOptionDDI, mfxExtVP9Segmentation, mfxExtVP9TemporalLayers

    class VP9MfxVideoParam : public mfxVideoParam
    {
    public:
        VP9MfxVideoParam();
        VP9MfxVideoParam(VP9MfxVideoParam const &);
        VP9MfxVideoParam(mfxVideoParam const &);
        VP9MfxVideoParam(mfxVideoParam const & par, eMFXHWType const & platform);

        VP9MfxVideoParam & operator = (VP9MfxVideoParam const &);
        VP9MfxVideoParam & operator = (mfxVideoParam const &);

        eMFXHWType m_platform;
        mfxU16 m_inMemType;
        mfxU32 m_targetKbps;
        mfxU32 m_maxKbps;
        mfxU32 m_bufferSizeInKb;
        mfxU32 m_initialDelayInKb;

        TempLayerParam m_layerParam[MAX_NUM_TEMP_LAYERS];

        bool m_segBufPassed;

        bool m_tempLayersBufPassed;
        bool m_webRTCMode;
        mfxU16 m_numLayers;

        void CalculateInternalParams();
        void SyncInternalParamToExternal();

    protected:
        void Construct(mfxVideoParam const & par);

    private:
        mfxExtBuffer*               m_extParam[NUM_OF_SUPPORTED_EXT_BUFFERS];
        mfxExtVP9Param              m_extPar;
        mfxExtOpaqueSurfaceAlloc    m_extOpaque;
        mfxExtCodingOption2         m_extOpt2;
        mfxExtCodingOption3         m_extOpt3;
        mfxExtCodingOptionDDI       m_extOptDDI;
        mfxExtVP9Segmentation       m_extSeg;
        mfxExtVP9TemporalLayers     m_extTempLayers;
    };

    template <typename T> ActualExtBufferExtractor GetActualExtBufferRef(VP9MfxVideoParam const & basicPar, T const & newPar)
    {
        return  ActualExtBufferExtractor(basicPar.ExtParam, basicPar.NumExtParam, newPar.ExtParam, newPar.NumExtParam);
    }

    class Task;
    mfxStatus SetFramesParams(VP9MfxVideoParam const &par,
                              Task const & task,
                              mfxU8 frameType,
                              VP9FrameLevelParam &frameParam,
                              eMFXHWType platform);

    mfxStatus InitVp9SeqLevelParam(VP9MfxVideoParam const &video, VP9SeqLevelParam &param);

    mfxStatus DecideOnRefListAndDPBRefresh(VP9MfxVideoParam const & par,
                                           Task *pTask,
                                           std::vector<sFrameEx*>& dpb,
                                           VP9FrameLevelParam &frameParam,
                                           mfxU32 prevFrameOrderInRefStructure);

    mfxStatus UpdateDpb(VP9FrameLevelParam &frameParam,
                        sFrameEx *pRecFrame,
                        std::vector<sFrameEx*>&dpb,
                        VideoCORE *pCore);

    class ExternalFrames
    {
    protected:
        std::list<sFrameEx>  m_frames;
    public:
        ExternalFrames() {}
        void Init(mfxU32 numFrames);
        mfxStatus GetFrame(mfxFrameSurface1 *pInFrame, sFrameEx *&pOutFrame);
     };

    class InternalFrames
    {
    protected:
        std::vector<sFrameEx>           m_frames;
        MfxFrameAllocResponse           m_response;
        std::vector<mfxFrameSurface1>   m_surfaces;
    public:
        InternalFrames() {}
        mfxStatus Init(VideoCORE *pCore, mfxFrameAllocRequest *pAllocReq, bool isCopyRequired);
        sFrameEx * GetFreeFrame();
        mfxStatus  GetFrame(mfxU32 numFrame, sFrameEx * &Frame);
        mfxStatus Release();

        inline mfxU16 Height()
        {
            return m_response.m_info.Height;
        }
        inline mfxU16 Width()
        {
            return m_response.m_info.Width;
        }
        inline mfxU32 Num()
        {
            return m_response.NumFrameActual;
        }
        inline MfxFrameAllocResponse& GetFrameAllocReponse()  {return m_response;}
    };

    bool isVideoSurfInput(mfxVideoParam const & video);

    inline bool isOpaq(mfxVideoParam const & video)
    {
        return (video.IOPattern & MFX_IOPATTERN_IN_OPAQUE_MEMORY)!=0;
    }

    inline mfxU32 CalcNumTasks(mfxVideoParam const & video)
    {
        return video.AsyncDepth;
    }

    inline mfxU32 CalcNumSurfRecon(mfxVideoParam const & video)
    {
        return video.mfx.NumRefFrame + CalcNumTasks(video);
    }

    inline mfxU32 CalcNumSurfRaw(mfxVideoParam const & video)
    {
        // number of input surfaces is same for VIDEO and SYSTEM memory
        // because so far encoder doesn't support LookAhead and B-frames
        return video.AsyncDepth + ((video.AsyncDepth > 1)? 1: 0);
    }

    class Task
    {
    public:

        sFrameEx*         m_pRawFrame;
        sFrameEx*         m_pRawLocalFrame;
        mfxBitstream*     m_pBitsteam;

        VP9FrameLevelParam m_frameParam;
        sFrameEx*         m_pRecFrame;
        sFrameEx*         m_pRecRefFrames[3];
        sFrameEx*         m_pOutBs;
        sFrameEx*         m_pSegmentMap;
        mfxU32            m_frameOrder;
        mfxU64            m_timeStamp;
        mfxU32            m_taskIdForDriver;
        mfxU32            m_frameOrderInGop;
        mfxU32            m_frameOrderInRefStructure;

        mfxEncodeCtrl     m_ctrl;

        mfxU32 m_bsDataLength;

        VP9MfxVideoParam* m_pParam;

        bool m_insertIVFSeqHeader;
        bool m_resetBrc;

        mfxExtVP9Segmentation const * m_pPrevSegment;

        Task ():
              m_pRawFrame(NULL),
              m_pRawLocalFrame(NULL),
              m_pBitsteam(0),
              m_pRecFrame(NULL),
              m_pOutBs(NULL),
              m_pSegmentMap(NULL),
              m_frameOrder(0),
              m_timeStamp(0),
              m_taskIdForDriver(0),
              m_frameOrderInGop(0),
              m_frameOrderInRefStructure(0),
              m_bsDataLength(0),
              m_pParam(NULL),
              m_insertIVFSeqHeader(false),
              m_resetBrc(false),
              m_pPrevSegment(NULL)
          {
              Zero(m_pRecRefFrames);
              Zero(m_frameParam);
              Zero(m_ctrl);
          }

          ~Task() {};
    };

    inline mfxStatus FreeTask(VideoCORE *pCore, Task &task)
    {
        mfxStatus sts = FreeSurface(task.m_pRawFrame, pCore);
        MFX_CHECK_STS(sts);
        sts = FreeSurface(task.m_pRawLocalFrame, pCore);
        MFX_CHECK_STS(sts);
        sts = FreeSurface(task.m_pOutBs, pCore);
        MFX_CHECK_STS(sts);
        sts = FreeSurface(task.m_pSegmentMap, pCore);
        MFX_CHECK_STS(sts);
        sts = DecreaseRef(task.m_pRecFrame, pCore);
        MFX_CHECK_STS(sts);
        task.m_pRecFrame = 0;

        const VP9MfxVideoParam& curMfxPar = *task.m_pParam;
        if (curMfxPar.m_numLayers && task.m_frameParam.temporalLayer == curMfxPar.m_numLayers - 1 &&
            curMfxPar.m_numLayers > curMfxPar.mfx.NumRefFrame)
        {
            // if last temporal layer is non-reference, need to free reconstructed surface right away.
            sts = FreeSurface(task.m_pRecFrame, pCore);
            MFX_CHECK_STS(sts);
        }

        task.m_pBitsteam = 0;
        Zero(task.m_frameParam);
        Zero(task.m_ctrl);

        return MFX_ERR_NONE;
    }

    inline bool IsBufferBasedBRC(mfxU16 brcMethod)
    {
        return brcMethod == MFX_RATECONTROL_CBR
            || brcMethod == MFX_RATECONTROL_VBR;
    }

    inline bool IsBitrateBasedBRC(mfxU16 brcMethod)
    {
        return IsBufferBasedBRC(brcMethod);
    }

    inline bool isBrcResetRequired(VP9MfxVideoParam const & parBefore, VP9MfxVideoParam const & parAfter)
    {
        mfxU16 brc = parAfter.mfx.RateControlMethod;
        if (false == IsBitrateBasedBRC(brc))
        {
            return false;
        }

        double frameRateBefore = (double)parBefore.mfx.FrameInfo.FrameRateExtN / (double)parBefore.mfx.FrameInfo.FrameRateExtD;
        double frameRateAfter = (double)parAfter.mfx.FrameInfo.FrameRateExtN / (double)parAfter.mfx.FrameInfo.FrameRateExtD;

        mfxExtVP9Param const & extParBefore = GetExtBufferRef(parBefore);
        mfxExtVP9Param const & extParAfter = GetExtBufferRef(parAfter);

        if (parBefore.m_targetKbps != parAfter.m_targetKbps
            || ((brc == MFX_RATECONTROL_VBR) && (parBefore.m_maxKbps != parAfter.m_maxKbps))
            || frameRateBefore != frameRateAfter
            || (IsBufferBasedBRC(brc) && (parBefore.m_bufferSizeInKb != parAfter.m_bufferSizeInKb))
            || ((brc == MFX_RATECONTROL_ICQ) && (parBefore.mfx.ICQQuality != parAfter.mfx.ICQQuality))
            || extParBefore.FrameWidth != extParAfter.FrameWidth
            || extParBefore.FrameHeight != extParAfter.FrameHeight)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    inline mfxU16 CalcTemporalLayerIndex(VP9MfxVideoParam const & par, mfxU32 frameOrder)
    {
        mfxU16 i = 0;
        mfxExtVP9TemporalLayers const & tl = GetExtBufferRef(par);

        if (par.m_numLayers > 0)
        {
            mfxU32 maxScale = tl.Layer[par.m_numLayers - 1].FrameRateScale;
            for (; i < par.m_numLayers; i++)
                if (frameOrder % (maxScale / tl.Layer[i].FrameRateScale) == 0)
                    break;
        }

        return i;
    }

    // for TS-encoding: context refreshing should be disabled for the 1st frame in each layer
    //  in case if this frame can appear next to the key-frame after layers extraction
    // this is demand by kernel processing logic
    inline bool IsNeedDisableRefreshForFrameTS(VP9MfxVideoParam const & par, mfxU32 frameOrderInGop)
    {
        if (par.m_numLayers > 1 && frameOrderInGop >= 2)
        {
            mfxExtVP9TemporalLayers const & tl = GetExtBufferRef(par);
            mfxU32 maxScale = tl.Layer[par.m_numLayers - 1].FrameRateScale;
            if (frameOrderInGop > maxScale)
            {
                // not applicable outside of the 1st GOP
                return false;
            }
            else if (frameOrderInGop == maxScale)
            {
                // the last affected frame is the first frame in the 2nd GOP
                return true;
            }
            else if (frameOrderInGop <= (maxScale / 2))
            {
                // inside the first GOP affected all frames in the beginning of Layer-2 and Layer-3
                // (Layer-0 and Layer-1 not affected)
                if (CalcTemporalLayerIndex(par, frameOrderInGop) < CalcTemporalLayerIndex(par, frameOrderInGop - 1))
                {
                    return true;
                }
            }
        }

        return false;
    }

    mfxStatus GetRealSurface(
        VideoCORE *pCore,
        VP9MfxVideoParam const &par,
        Task const &task,
        mfxFrameSurface1 *& pSurface);

    mfxStatus GetInputSurface(
        VideoCORE *pCore,
        VP9MfxVideoParam const &par,
        Task const &task,
        mfxFrameSurface1 *& pSurface);

    mfxStatus CopyRawSurfaceToVideoMemory(
        VideoCORE *pCore,
        VP9MfxVideoParam const &par,
        Task const &task);

/*mfxStatus ReleaseDpbFrames(VideoCORE* pCore, std::vector<sFrameEx*> & dpb)
{
    for (mfxU8 refIdx = 0; refIdx < dpb.size(); refIdx ++)
    {
        if (dpb[refIdx])
        {
            dpb[refIdx]->refCount = 0;
            MFX_CHECK_STS(FreeSurface(dpb[refIdx], pCore));
        }
    }

    return MFX_ERR_NONE;
}*/

struct FindTaskByRawSurface
{
    FindTaskByRawSurface(mfxFrameSurface1 * pSurf) : m_pSurface(pSurf) {}

    bool operator ()(Task const & task)
    {
        return task.m_pRawFrame->pSurface == m_pSurface;
    }

    mfxFrameSurface1* m_pSurface;
};

struct FindTaskByMfxVideoParam
{
    FindTaskByMfxVideoParam(mfxVideoParam* pPar) : m_pPar(pPar) {}

    bool operator ()(Task const & task)
    {
        return task.m_pParam == m_pPar;
    }

    mfxVideoParam* m_pPar;
};

// full list of essential segmentation parametes is provided
inline bool AllMandatorySegMapParams(mfxExtVP9Segmentation const & seg)
{
    return seg.SegmentIdBlockSize &&
        seg.NumSegmentIdAlloc && seg.SegmentId;
}

// at least one essential segmentation parameter is provided
inline bool AnyMandatorySegMapParam(mfxExtVP9Segmentation const & seg)
{
    return seg.SegmentIdBlockSize ||
        seg.NumSegmentIdAlloc || seg.SegmentId;
}

inline mfxU16 MapIdToBlockSize(mfxU16 id)
{
    return id;
}

inline bool CompareSegmentMaps(mfxExtVP9Segmentation const & first, mfxExtVP9Segmentation const & second)
{
    if (!first.SegmentId || !second.SegmentId ||
        first.SegmentIdBlockSize != second.SegmentIdBlockSize ||
        !first.NumSegmentIdAlloc || !second.NumSegmentIdAlloc)
        return false;

    return std::equal(first.SegmentId, first.SegmentId + std::min(first.NumSegmentIdAlloc, second.NumSegmentIdAlloc), second.SegmentId);
}

inline bool CompareSegmentParams(mfxExtVP9Segmentation const & first, mfxExtVP9Segmentation const & second)
{
    if (first.NumSegments != second.NumSegments)
        return false;

    return std::equal(first.Segment, first.Segment + first.NumSegments, second.Segment);
}

inline void CopySegmentationBuffer(mfxExtVP9Segmentation & dst, mfxExtVP9Segmentation const & src)
{
    mfxU8* tmp = dst.SegmentId;
    ZeroExtBuffer(dst);
    dst = src;
    dst.SegmentId = tmp;
    if (dst.SegmentId && src.SegmentId && dst.NumSegmentIdAlloc)
    {
        std::copy(src.SegmentId, src.SegmentId + dst.NumSegmentIdAlloc, dst.SegmentId);
    }
}

inline void CombineInitAndRuntimeSegmentBuffers(mfxExtVP9Segmentation & runtime, mfxExtVP9Segmentation const & init)
{
    if (runtime.SegmentId == 0 && init.SegmentId != 0)
    {
        runtime.SegmentId = init.SegmentId;
        runtime.SegmentIdBlockSize = init.SegmentIdBlockSize;
        runtime.NumSegmentIdAlloc = init.NumSegmentIdAlloc;
    }
}

enum
{
    FEAT_QIDX = 0,
    FEAT_LF_LVL = 1,
    FEAT_REF = 2,
    FEAT_SKIP = 3
};

inline bool IsFeatureEnabled(mfxU16 features, mfxU8 feature)
{
    return (features & (1 << feature)) != 0;
}

mfxStatus GetNativeHandleToRawSurface(
    VideoCORE & core,
    mfxMemId mid,
    mfxHDL *handle,
    VP9MfxVideoParam const & video);

} // MfxHwVP9Encode

template<class T, class I>
inline bool Clamp(T & opt, I min, I max)
{
    if (opt < static_cast<T>(min))
    {
        opt = static_cast<T>(min);
        return false;
    }

    if (opt > static_cast<T>(max))
    {
        opt = static_cast<T>(max);
        return false;
    }

    return true;
}
