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

#include "mfx_common.h"

#if defined(MFX_ENABLE_H265_VIDEO_ENCODE)

#pragma once
#include "mfx_h264_encode_struct_vaapi.h"
#include "hevce_ddi_main.h"
#include "mfx_h265_encode_hw_set.h"
#include "mfx_ext_buffers.h"
#include "mfxplugin++.h"
#include "mfxla.h"

#include "mfxbrc.h"

#include <list>

#define MFX_SORT_COMMON(_AR, _SZ, _COND)\
    for (mfxU32 _i = 0; _i < (_SZ); _i ++)\
        for (mfxU32 _j = _i; _j < (_SZ); _j ++)\
            if (_COND) std::swap(_AR[_i], _AR[_j]);
#define MFX_SORT(_AR, _SZ, _OP) MFX_SORT_COMMON(_AR, _SZ, _AR[_i] _OP _AR[_j])
#define MFX_SORT_STRUCT(_AR, _SZ, _M, _OP) MFX_SORT_COMMON(_AR, _SZ, _AR[_i]._M _OP _AR[_j]._M)


namespace MfxHwH265Encode
{


template <class Class1, class Class2>
    struct is_same
    { enum { value = false }; };

template<class Class1>
    struct is_same<Class1, Class1>
    { enum { value = true }; };

template<class T> inline T * Begin(std::vector<T> & t) { return &*t.begin(); }
template<class T> inline T const * Begin(std::vector<T> const & t) { return &*t.begin(); }
template<class T> inline bool Equal(T const & l, T const & r) { return memcmp(&l, &r, sizeof(T)) == 0; }
template<class T> inline void Fill(T & obj, int val)          { memset(&obj, val, sizeof(obj)); }
template<class T> inline void Zero(T & obj)                   { memset(&obj, 0, sizeof(obj)); }
template<class T> inline void Zero(std::vector<T> & vec)      { memset(&vec[0], 0, sizeof(T) * vec.size()); }
template<class T> inline void Zero(T * first, size_t cnt)     { memset(first, 0, sizeof(T) * cnt); }
template<class T> inline T Abs  (T x)               { return (x > 0 ? x : -x); }

template<class T> inline T Lsb(T val, mfxU32 maxLSB)
{
    if (val >= 0)
        return val % maxLSB;
    return (maxLSB - ((-val) % maxLSB)) % maxLSB;
}
template<class T> inline bool IsZero(T* pBegin, T* pEnd)
{
    assert(((mfxU8*)pEnd - (mfxU8*)pBegin) % sizeof(T) == 0);
    for (; pBegin < pEnd; pBegin++)
        if (*pBegin)
            return false;
    return true;
}
template<class T> inline bool IsZero(T& obj) { return IsZero((mfxU8*)&obj, (mfxU8*)&obj + sizeof(T)); }


inline mfxU32 CeilLog2  (mfxU32 x)           { mfxU32 l = 0; while(x > (1U<<l)) l++; return l; }
inline mfxU32 CeilDiv   (mfxU32 x, mfxU32 y) { return (x + y - 1) / y; }
inline mfxU32 GetAlignmentByPlatform(eMFXHWType platform)
{
    return platform >= MFX_HW_CNL ? 8 : 16;
}

enum
{
    MFX_IOPATTERN_IN_MASK = MFX_IOPATTERN_IN_SYSTEM_MEMORY | MFX_IOPATTERN_IN_VIDEO_MEMORY | MFX_IOPATTERN_IN_OPAQUE_MEMORY,
    MFX_MEMTYPE_D3D_INT = MFX_MEMTYPE_FROM_ENCODE | MFX_MEMTYPE_DXVA2_DECODER_TARGET | MFX_MEMTYPE_INTERNAL_FRAME,
    MFX_MEMTYPE_D3D_EXT = MFX_MEMTYPE_FROM_ENCODE | MFX_MEMTYPE_DXVA2_DECODER_TARGET | MFX_MEMTYPE_EXTERNAL_FRAME,
    MFX_MEMTYPE_SYS_EXT = MFX_MEMTYPE_FROM_ENCODE | MFX_MEMTYPE_SYSTEM_MEMORY        | MFX_MEMTYPE_EXTERNAL_FRAME,
    MFX_MEMTYPE_SYS_INT = MFX_MEMTYPE_FROM_ENCODE | MFX_MEMTYPE_SYSTEM_MEMORY        | MFX_MEMTYPE_INTERNAL_FRAME,
};

constexpr mfxU8    MAX_DPB_SIZE          = 15;
constexpr mfxU8    IDX_INVALID           = 0xff;

constexpr mfxU8    HW_SURF_ALIGN_W       = 16;
constexpr mfxU8    HW_SURF_ALIGN_H       = 16;

constexpr mfxU16   MAX_SLICES            = 600; // conforms to level 6 limits
constexpr mfxU8    DEFAULT_LTR_INTERVAL  = 16;
constexpr mfxU8    DEFAULT_PPYR_INTERVAL = 3;

constexpr mfxU8    MAX_NUM_ROI           = 16;
constexpr mfxU8    MAX_NUM_DIRTY_RECT    = 64;

enum
{
    FRAME_NEW           = 0x01,
    FRAME_ACCEPTED      = 0x02,
    FRAME_REORDERED     = 0x04,
    FRAME_SUBMITTED     = 0x08,
    FRAME_ENCODED       = 0x10,
    STAGE_SUBMIT        = FRAME_NEW | FRAME_ACCEPTED | FRAME_REORDERED,
    STAGE_QUERY         = STAGE_SUBMIT      | FRAME_SUBMITTED,
    STAGE_READY         = STAGE_QUERY       | FRAME_ENCODED,
};

enum
{
    CODING_TYPE_I   = 1,
    CODING_TYPE_P   = 2,
    CODING_TYPE_B   = 3, //regular B, or no reference to other B frames
    CODING_TYPE_B1  = 4, //B1, reference to only I, P or regular B frames
    CODING_TYPE_B2  = 5, //B2, references include B1
};

enum
{
    INSERT_AUD = 0x01,
    INSERT_VPS = 0x02,
    INSERT_SPS = 0x04,
    INSERT_PPS = 0x08,
    INSERT_BPSEI = 0x10,
    INSERT_PTSEI = 0x20,
    INSERT_DCVSEI = 0x40,
    INSERT_LLISEI = 0x80,
    INSERT_SEI = (INSERT_BPSEI | INSERT_PTSEI | INSERT_DCVSEI | INSERT_LLISEI)
};

struct DpbFrame
{
    mfxI32              m_poc         = -1;
    mfxU32              m_fo          = 0xffffffff; // FrameOrder
    mfxU32              m_eo          = 0xffffffff; // Encoded order
    mfxU32              m_bpo         = 0;          // B-pyramid order
    mfxU32              m_level       = 0;          // pyramid level
    mfxU8               m_tid         = 0;
    bool                m_ltr         = false; // is "long-term"
    bool                m_ldb         = false; // is "low-delay B"
    bool                m_secondField = false;
    bool                m_bottomField = false;
    mfxU8               m_codingType  = 0;
    mfxU8               m_idxRaw      = IDX_INVALID;
    mfxU8               m_idxRec      = IDX_INVALID;
    mfxMemId            m_midRec      = nullptr;
    mfxMemId            m_midRaw      = nullptr;
    mfxFrameSurface1*   m_surf        = nullptr; //input surface, may be opaque
};

typedef DpbFrame DpbArray[MAX_DPB_SIZE];

enum
{
    TASK_DPB_ACTIVE = 0, // available during task execution (modified dpb[BEFORE] )
    TASK_DPB_AFTER,      // after task execution (ACTIVE + curTask if ref)
    TASK_DPB_BEFORE,     // after previous task execution (prevTask dpb[AFTER])
    TASK_DPB_NUM
};

struct IntraRefreshState
{
    mfxU16  refrType;
    mfxU16  IntraLocation;
    mfxU16  IntraSize;
    mfxI16  IntRefQPDelta;
    bool    firstFrameInCycle;
};

struct RectData{
    mfxU32  Left;
    mfxU32  Top;
    mfxU32  Right;
    mfxU32  Bottom;
};

struct RoiData{
    mfxU32  Left;
    mfxU32  Top;
    mfxU32  Right;
    mfxU32  Bottom;

    mfxI16  DeltaQP;
};

struct Task : DpbFrame
{
    mfxBitstream*     m_bs                            = nullptr;
    mfxFrameSurface1* m_surf_real                     = nullptr;
    mfxEncodeCtrl     m_ctrl                          = {};
    Slice             m_sh                            = {};

    mfxU32            m_idxBs                         = IDX_INVALID;
    mfxU32            m_idxCUQp                       = IDX_INVALID;
    bool              m_bCUQPMap                      = false;

    mfxU8             m_refPicList[2][MAX_DPB_SIZE]   = {};
    mfxU8             m_numRefActive[2]               = {};

    mfxU16            m_frameType                     = 0;
    mfxU16            m_insertHeaders                 = 0;
    mfxU8             m_shNUT                         = 0;
    mfxI8             m_qpY                           = 0;
    mfxU8             m_avgQP                         = 0;
    mfxU32            m_MAD                           = 0;
    mfxI32            m_lastIPoc                      = 0;
    mfxI32            m_lastRAP                       = 0;

    mfxU32            m_statusReportNumber            = 0;
    mfxU32            m_bsDataLength                  = 0;
    mfxU32            m_minFrameSize                  = 0;

    DpbArray          m_dpb[TASK_DPB_NUM];

    mfxMemId          m_midBs                         = nullptr;
    mfxMemId          m_midCUQp                       = nullptr;

    bool              m_resetBRC                      = false;

    mfxU32            m_initial_cpb_removal_delay     = 0;
    mfxU32            m_initial_cpb_removal_offset    = 0;
    mfxU32            m_cpb_removal_delay             = 0;
    mfxU32            m_dpb_output_delay              = 0;

    mfxU32            m_stage                         = 0;
    mfxU32            m_recode                        = 0;
    IntraRefreshState m_IRState                       = {};
    bool              m_bSkipped                      = false;

    RoiData           m_roi[MAX_NUM_ROI]              = {};
    mfxU16            m_roiMode                       = 0; // BRC only
    mfxU16            m_numRoi                        = 0;
    bool              m_bPriorityToDQPpar             = false;

#ifdef MFX_ENABLE_HEVCE_DIRTY_RECT
    RectData          m_dirtyRect[MAX_NUM_DIRTY_RECT] = {};
    mfxU16            m_numDirtyRect                  = 0;
#endif  // MFX_ENABLE_HEVCE_DIRTY_RECT

    mfxU16            m_SkipMode                      = 0;
};

enum
{
    NO_REFRESH             = 0,
    VERT_REFRESH           = 1,
    HORIZ_REFRESH          = 2
};

typedef std::list<Task> TaskList;

template <typename T>
struct remove_const
{
    typedef T type;
};

template <typename T>
struct remove_const<const T>
{
    typedef T type;
};

class MfxVideoParam;


namespace ExtBuffer
{
    #define SIZE_OF_ARRAY(ARR) (sizeof(ARR) / sizeof(ARR[0]))

    const mfxU32 allowed_buffers[] = {
         MFX_EXTBUFF_HEVC_PARAM,
         MFX_EXTBUFF_HEVC_TILES,
         MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION,
         MFX_EXTBUFF_AVC_REFLISTS,
         MFX_EXTBUFF_CODING_OPTION,
         MFX_EXTBUFF_CODING_OPTION2,
         MFX_EXTBUFF_CODING_OPTION3,
         MFX_EXTBUFF_CODING_OPTION_SPSPPS,
         MFX_EXTBUFF_AVC_REFLIST_CTRL,
         MFX_EXTBUFF_AVC_TEMPORAL_LAYERS,
         MFX_EXTBUFF_ENCODER_RESET_OPTION,
         MFX_EXTBUFF_CODING_OPTION_VPS,
         MFX_EXTBUFF_VIDEO_SIGNAL_INFO,
         MFX_EXTBUFF_LOOKAHEAD_STAT,
         MFX_EXTBUFF_BRC,
         MFX_EXTBUFF_MBQP,
         MFX_EXTBUFF_ENCODER_ROI,
         MFX_EXTBUFF_DIRTY_RECTANGLES,
#ifdef MFX_ENABLE_HEVCE_HDR_SEI
         MFX_EXTBUFF_MASTERING_DISPLAY_COLOUR_VOLUME,
         MFX_EXTBUFF_CONTENT_LIGHT_LEVEL_INFO,
#endif
         MFX_EXTBUFF_ENCODED_FRAME_INFO
    };

    template<class T> struct ExtBufferMap {};

    #define EXTBUF(TYPE, ID) template<> struct ExtBufferMap <TYPE> { enum { Id = ID}; }
        EXTBUF(mfxExtHEVCParam,             MFX_EXTBUFF_HEVC_PARAM);
        EXTBUF(mfxExtHEVCTiles,             MFX_EXTBUFF_HEVC_TILES);
        EXTBUF(mfxExtOpaqueSurfaceAlloc,    MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
        EXTBUF(mfxExtAVCRefLists,           MFX_EXTBUFF_AVC_REFLISTS);
        EXTBUF(mfxExtCodingOption,          MFX_EXTBUFF_CODING_OPTION);
        EXTBUF(mfxExtCodingOption2,         MFX_EXTBUFF_CODING_OPTION2);
        EXTBUF(mfxExtCodingOption3,         MFX_EXTBUFF_CODING_OPTION3);
        EXTBUF(mfxExtCodingOptionDDI,       MFX_EXTBUFF_DDI);
        EXTBUF(mfxExtCodingOptionSPSPPS,    MFX_EXTBUFF_CODING_OPTION_SPSPPS);
        EXTBUF(mfxExtAVCRefListCtrl,        MFX_EXTBUFF_AVC_REFLIST_CTRL);
        EXTBUF(mfxExtAvcTemporalLayers,     MFX_EXTBUFF_AVC_TEMPORAL_LAYERS);
        EXTBUF(mfxExtEncoderResetOption,    MFX_EXTBUFF_ENCODER_RESET_OPTION);
        EXTBUF(mfxExtCodingOptionVPS,       MFX_EXTBUFF_CODING_OPTION_VPS);
        EXTBUF(mfxExtVideoSignalInfo,       MFX_EXTBUFF_VIDEO_SIGNAL_INFO);
        EXTBUF(mfxExtEncoderCapability,     MFX_EXTBUFF_ENCODER_CAPABILITY);
        EXTBUF(mfxExtLAFrameStatistics,     MFX_EXTBUFF_LOOKAHEAD_STAT);
        EXTBUF(mfxExtEncoderROI,            MFX_EXTBUFF_ENCODER_ROI);
        EXTBUF(mfxExtDirtyRect,             MFX_EXTBUFF_DIRTY_RECTANGLES);
        EXTBUF(mfxExtBRC,                   MFX_EXTBUFF_BRC);
        EXTBUF(mfxExtMBQP,                  MFX_EXTBUFF_MBQP);
#ifdef MFX_ENABLE_HEVCE_WEIGHTED_PREDICTION
        EXTBUF(mfxExtPredWeightTable,       MFX_EXTBUFF_PRED_WEIGHT_TABLE);
#endif
#ifdef MFX_ENABLE_HEVCE_HDR_SEI
        EXTBUF(mfxExtMasteringDisplayColourVolume, MFX_EXTBUFF_MASTERING_DISPLAY_COLOUR_VOLUME);
        EXTBUF(mfxExtContentLightLevelInfo, MFX_EXTBUFF_CONTENT_LIGHT_LEVEL_INFO);
#endif
        EXTBUF(mfxExtAVCEncodedFrameInfo, MFX_EXTBUFF_ENCODED_FRAME_INFO);

    #undef EXTBUF

    template <class T> void CopySupportedParams(T& buf_dst, T& buf_src)
    {
        buf_dst = buf_src;
    }

    inline void CopySupportedParams(mfxExtHEVCParam& buf_dst, mfxExtHEVCParam& buf_src)
    {
        MFX_COPY_FIELD(PicWidthInLumaSamples);
        MFX_COPY_FIELD(PicHeightInLumaSamples);
        MFX_COPY_FIELD(GeneralConstraintFlags);
#if (MFX_VERSION >= 1026)
        MFX_COPY_FIELD(SampleAdaptiveOffset);
        MFX_COPY_FIELD(LCUSize);
#endif
    }

    inline void  CopySupportedParams(mfxExtHEVCTiles& buf_dst, mfxExtHEVCTiles& buf_src)
    {
        MFX_COPY_FIELD(NumTileRows);
        MFX_COPY_FIELD(NumTileColumns);
    }

    inline void CopySupportedParams (mfxExtCodingOption& buf_dst, mfxExtCodingOption& buf_src)
    {
        MFX_COPY_FIELD(PicTimingSEI);
        MFX_COPY_FIELD(VuiNalHrdParameters);
        MFX_COPY_FIELD(NalHrdConformance);
        MFX_COPY_FIELD(AUDelimiter);
    }
    inline void CopySupportedParams(mfxExtCodingOption2& buf_dst, mfxExtCodingOption2& buf_src)
    {
        MFX_COPY_FIELD(IntRefType);
        MFX_COPY_FIELD(IntRefCycleSize);
        MFX_COPY_FIELD(IntRefQPDelta);
        MFX_COPY_FIELD(MaxFrameSize);

        MFX_COPY_FIELD(MBBRC);
        MFX_COPY_FIELD(BRefType);
        MFX_COPY_FIELD(NumMbPerSlice);
        MFX_COPY_FIELD(DisableDeblockingIdc);

        MFX_COPY_FIELD(RepeatPPS);
        MFX_COPY_FIELD(MaxSliceSize);
        MFX_COPY_FIELD(ExtBRC);

        MFX_COPY_FIELD(MinQPI);
        MFX_COPY_FIELD(MaxQPI);
        MFX_COPY_FIELD(MinQPP);
        MFX_COPY_FIELD(MaxQPP);
        MFX_COPY_FIELD(MinQPB);
        MFX_COPY_FIELD(MaxQPB);
        MFX_COPY_FIELD(SkipFrame);
        MFX_COPY_FIELD(EnableMAD);
    }

    inline void CopySupportedParams(mfxExtCodingOption3& buf_dst, mfxExtCodingOption3& buf_src)
    {
        MFX_COPY_FIELD(PRefType);
        MFX_COPY_FIELD(IntRefCycleDist);
        MFX_COPY_FIELD(EnableQPOffset);
        MFX_COPY_FIELD(GPB);
        MFX_COPY_FIELD(QVBRQuality);
        MFX_COPY_FIELD(EnableMBQP);

        MFX_COPY_ARRAY_FIELD(QPOffset);
        MFX_COPY_ARRAY_FIELD(NumRefActiveP);
        MFX_COPY_ARRAY_FIELD(NumRefActiveBL0);
        MFX_COPY_ARRAY_FIELD(NumRefActiveBL1);
#if (MFX_VERSION >= 1026)
        MFX_COPY_FIELD(TransformSkip);
#endif
#if (MFX_VERSION >= 1027)
        MFX_COPY_FIELD(TargetChromaFormatPlus1);
        MFX_COPY_FIELD(TargetBitDepthLuma);
        MFX_COPY_FIELD(TargetBitDepthChroma);
#endif
        MFX_COPY_FIELD(WinBRCMaxAvgKbps);
        MFX_COPY_FIELD(WinBRCSize);
#if defined(MFX_ENABLE_HEVCE_WEIGHTED_PREDICTION)
        MFX_COPY_FIELD(WeightedPred);
        MFX_COPY_FIELD(WeightedBiPred);
#if defined(MFX_ENABLE_HEVCE_FADE_DETECTION)
        MFX_COPY_FIELD(FadeDetection);
#endif //defined(MFX_ENABLE_HEVCE_FADE_DETECTION)
#endif //defined(MFX_ENABLE_HEVCE_WEIGHTED_PREDICTION)
#if (MFX_VERSION >= 1025)
        MFX_COPY_FIELD(EnableNalUnitType);
#endif
#if MFX_VERSION >= MFX_VERSION_NEXT
        MFX_COPY_FIELD(DeblockingAlphaTcOffset);
        MFX_COPY_FIELD(DeblockingBetaOffset);
#endif
        MFX_COPY_FIELD(LowDelayBRC);
    }

    inline void  CopySupportedParams(mfxExtCodingOptionDDI& buf_dst, mfxExtCodingOptionDDI& buf_src)
    {
        MFX_COPY_FIELD(NumActiveRefBL0);
        MFX_COPY_FIELD(NumActiveRefBL1);
        MFX_COPY_FIELD(NumActiveRefP);
        MFX_COPY_FIELD(QpAdjust);
    }
    inline void  CopySupportedParams(mfxExtEncoderCapability& buf_dst, mfxExtEncoderCapability& buf_src)
    {
        MFX_COPY_FIELD(MBPerSec);
    }

    template<class T> void Init(T& buf)
    {
        Zero(buf);
        mfxExtBuffer& header = *((mfxExtBuffer*)&buf);
        header.BufferId = ExtBufferMap<T>::Id;
        header.BufferSz = sizeof(T);
    }

    template<class T> bool CheckBufferParams(T& buf, bool bFix)
    {
        bool bUnsuppoted = false;
        T buf_ref;
        Init(buf_ref);
        CopySupportedParams(buf_ref, buf);
        bUnsuppoted = (memcmp(&buf_ref, &buf, sizeof(T))!=0);
        if (bUnsuppoted && bFix)
        {
            buf = buf_ref;
        }
        return bUnsuppoted;
    }

    class Proxy
    {
    private:
        mfxExtBuffer ** m_b;
        mfxU16          m_n;
    public:
        Proxy(mfxExtBuffer ** b, mfxU16 n)
            : m_b(b)
            , m_n(n)
        {
        }

        template <class T> operator T*()
        {
            if (m_b)
                for (mfxU16 i = 0; i < m_n; i ++)
                    if (m_b[i] && m_b[i]->BufferId == ExtBufferMap<T>::Id)
                        return (T*)m_b[i];
            return 0;
        }
    };

    template <class P> Proxy Get(P & par)
    {
        static_assert(!(is_same<P, MfxVideoParam>::value), "MfxVideoParam_is_invalid_for_this_template");
        return Proxy(par.ExtParam, par.NumExtParam);
    }


    template<class T> inline void Add(T& buf, mfxExtBuffer* pBuffers[], mfxU16 &numbuffers)
    {
        pBuffers[numbuffers++] = ((mfxExtBuffer*)&buf);
    }


    template<class P, class T> bool Construct(P const & par, T& buf, mfxExtBuffer* pBuffers[], mfxU16 &numbuffers)
    {
        T  * p = Get(par);
        Add(buf,pBuffers,numbuffers);

        if (p)
        {
            buf = *p;
            return true;
        }

        Init(buf);

        return false;
    }

    template<class P, class T> bool Set(P& par, T const & buf)
    {
        T* p = Get(par);

        if (p)
        {
            *p = buf;
            return true;
        }

        return false;
    }

    bool Construct(mfxVideoParam const & par, mfxExtHEVCParam& buf);

    mfxStatus CheckBuffers(mfxVideoParam const & par, const mfxU32 allowed[], mfxU32 notDetected[], mfxU32 size);

    inline mfxStatus CheckBuffers(mfxVideoParam const & par)
    {
        // checked if all buffers are supported, not repeated
        mfxU32 notDetected[SIZE_OF_ARRAY(allowed_buffers)];
        mfxU32 size = SIZE_OF_ARRAY(allowed_buffers);

        std::copy(std::begin(allowed_buffers), std::end(allowed_buffers), notDetected);

        if (par.NumExtParam)
            return CheckBuffers(par, allowed_buffers, notDetected, size);
        else
            return MFX_ERR_NONE;

    }
    inline mfxStatus CheckBuffers(mfxVideoParam const & par1, mfxVideoParam const & par2)
    {
        // checked if all buffers are supported, not repeated, identical set for both parfiles
        mfxU32 notDetected1[SIZE_OF_ARRAY(allowed_buffers)];
        mfxU32 notDetected2[SIZE_OF_ARRAY(allowed_buffers)];
        mfxU32 size = SIZE_OF_ARRAY(allowed_buffers);

        std::copy(std::begin(allowed_buffers), std::end(allowed_buffers), notDetected1);
        std::copy(std::begin(allowed_buffers), std::end(allowed_buffers), notDetected2);

        if (par1.NumExtParam && par2.NumExtParam)
        {
            MFX_CHECK_STS(CheckBuffers(par1, allowed_buffers, notDetected1, size));
            MFX_CHECK_STS(CheckBuffers(par2, allowed_buffers, notDetected2, size));
        }
        else
        {
            if (par1.NumExtParam == par2.NumExtParam)
                return MFX_ERR_NONE;
            else
                return MFX_ERR_UNDEFINED_BEHAVIOR;
        }

        if (memcmp(notDetected1, notDetected2, size) != 0)
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        return MFX_ERR_NONE;
    }
};

class TemporalLayers
{
public:
    TemporalLayers()
        : m_numTL(1)
    {
        Zero(m_TL);
        m_TL[0].Scale = 1;
    }

    ~TemporalLayers(){}

    void SetTL(mfxExtAvcTemporalLayers const & tl)
    {
        m_numTL = 0;
        Zero(m_TL);
        m_TL[0].Scale = 1;

        for (mfxU8 i = 0; i < 7; i++)
        {
            if (tl.Layer[i].Scale)
            {
                m_TL[m_numTL].TId = i;
                m_TL[m_numTL].Scale = (mfxU8)tl.Layer[i].Scale;
                m_numTL++;
            }
        }

        m_numTL = std::max<mfxU8>(m_numTL, 1);
    }

    mfxU8 NumTL() const { return m_numTL; }

    mfxU8 GetTId(mfxU32 frameOrder) const
    {
        mfxU16 i;

        if (m_numTL < 1 || m_numTL > 8)
            return 0;

        for (i = 0; i < m_numTL && (frameOrder % (m_TL[m_numTL - 1].Scale / m_TL[i].Scale))  ; i++);

        return (i < m_numTL) ? m_TL[i].TId : 0;
    }

    mfxU8 HighestTId() const { return  m_TL[m_numTL - 1].TId; }

private:
    mfxU8 m_numTL;

    struct
    {
        mfxU8 TId;
        mfxU8 Scale;
    }m_TL[8];
};


class MfxVideoParam : public mfxVideoParam, public TemporalLayers
{
public:
    eMFXHWType m_platform;
    VPS m_vps;
    SPS m_sps;
    PPS m_pps;

    struct SliceInfo
    {
        mfxU32 SegmentAddress;
        mfxU32 NumLCU;
    };
    std::vector<SliceInfo> m_slice;
#ifdef MFX_ENABLE_HEVCE_DIRTY_RECT
    ENCODE_RECT m_dirtyRects[MAX_NUM_DIRTY_RECT];
#endif

    struct
    {
        mfxExtHEVCParam             HEVCParam;
        mfxExtHEVCTiles             HEVCTiles;
        mfxExtOpaqueSurfaceAlloc    Opaque;
        mfxExtCodingOption          CO;
        mfxExtCodingOption2         CO2;
        mfxExtCodingOption3         CO3;
        mfxExtCodingOptionDDI       DDI;
        mfxExtAvcTemporalLayers     AVCTL;
        mfxExtVideoSignalInfo       VSI;
        mfxExtBRC                   extBRC;
        mfxExtEncoderROI            ROI;
        mfxExtDirtyRect             DirtyRect;
#ifdef MFX_ENABLE_HEVCE_HDR_SEI
        mfxExtMasteringDisplayColourVolume   DisplayColour;
        mfxExtContentLightLevelInfo LightLevel;
#endif
        mfxExtEncoderResetOption   ResetOpt;
        mfxExtBuffer *              m_extParam[SIZE_OF_ARRAY(ExtBuffer::allowed_buffers)];
    } m_ext;

    mfxU32 BufferSizeInKB;
    mfxU32 InitialDelayInKB;
    mfxU32 TargetKbps;
    mfxU32 MaxKbps;
    mfxU32 LTRInterval;
    mfxU32 PPyrInterval;
    mfxU32 LCUSize;
    mfxU32 CodedPicAlignment;
    bool   HRDConformance;
    bool   RawRef;
    bool   bROIViaMBQP;
    bool   bMBQPInput;
    bool   RAPIntra;
    bool   bFieldReord;
    bool   bNonStandardReord; // Possible in encoding order only. No NumRefFrames limitation.


    MfxVideoParam();
    MfxVideoParam(MfxVideoParam const & par);
    MfxVideoParam(mfxVideoParam const & par, eMFXHWType const & platform);

    MfxVideoParam & operator = (mfxVideoParam const &);
    MfxVideoParam & operator = (MfxVideoParam const &);

    void SyncVideoToCalculableParam();
    void SyncCalculableToVideoParam();
    void AlignCalcWithBRCParamMultiplier();
    void SyncMfxToHeadersParam(mfxU32 numSlicesForSTRPSOpt = 0);
    void SyncHeadersToMfxParam();

    mfxStatus FillPar(mfxVideoParam& par, bool query = false);

    mfxStatus GetSliceHeader(Task const & task, Task const & prevTask, MFX_ENCODE_CAPS_HEVC const & caps, Slice & s) const;

    mfxStatus GetExtBuffers(mfxVideoParam& par, bool query = false);
    bool CheckExtBufferParam();

    bool isBPyramid() const { return m_ext.CO2.BRefType == MFX_B_REF_PYRAMID; }
    bool isLowDelay() const { return ((m_ext.CO3.PRefType == MFX_P_REF_PYRAMID) && !isTL()); }
    bool isTL()       const { return NumTL() > 1; }
    bool isSWBRC()    const {return  (IsOn(m_ext.CO2.ExtBRC) && (mfx.RateControlMethod == MFX_RATECONTROL_CBR || mfx.RateControlMethod == MFX_RATECONTROL_VBR))|| mfx.RateControlMethod == MFX_RATECONTROL_LA_EXT ;}
    bool isField()    const { return  !!(mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_FIELD_SINGLE); }
    bool isBFF()      const { return  ((mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_FIELD_BOTTOM) == MFX_PICSTRUCT_FIELD_BOTTOM); }

private:
    void Construct(mfxVideoParam const & par);
    void CopyCalcParams(MfxVideoParam const & par);
};

class FrameLocker : public mfxFrameData
{
public:
    FrameLocker(VideoCORE* core, mfxMemId mid)
        : m_core(core)
        , m_mid(mid)
    {
        Zero((mfxFrameData&)*this);
        Lock();
    }
    ~FrameLocker()
    {
        Unlock();
    }
    mfxStatus Lock()   { return m_core != nullptr ? m_core->LockFrame(m_mid, this): MFX_ERR_NULL_PTR; }
    mfxStatus Unlock() { return m_core != nullptr ? m_core->UnlockFrame(m_mid, this): MFX_ERR_NULL_PTR;
    }

private:
    VideoCORE* m_core;
    mfxMemId m_mid;
};

inline bool isValid(DpbFrame const & frame) { return IDX_INVALID !=  frame.m_idxRec; }
inline bool isDpbEnd(DpbArray const & dpb, mfxU32 idx) { return idx >= MAX_DPB_SIZE || !isValid(dpb[idx]); }

mfxU8 GetFrameType(MfxVideoParam const & video, mfxU32 frameOrder);

mfxU16 GetMaxBitDepth(mfxU32 FourCC);

void ConstructSTRPS(
    DpbArray const & DPB,
    mfxU8 const (&RPL)[2][MAX_DPB_SIZE],
    mfxU8 const (&numRefActive)[2],
    mfxI32 poc,
    STRPS& rps);

void ConstructRPL(
    MfxVideoParam const & par,
    DpbArray const & DPB,
    bool isB,
    mfxI32 poc,
    mfxU8  tid,
    mfxU32 level,
    bool  bSecondField,
    bool  bBottomField,
    mfxU8 (&RPL)[2][MAX_DPB_SIZE],
    mfxU8 (&numRefActive)[2],
    mfxExtAVCRefLists * pExtLists,
    mfxExtAVCRefListCtrl * pLCtrl = 0);

inline void ConstructRPL(
    MfxVideoParam const & par,
    DpbArray const & DPB,
    bool isB,
    mfxI32 poc,
    mfxU8  tid,
    mfxU32 level,
    bool  bSecondField,
    bool  bBottomField,
    mfxU8(&RPL)[2][MAX_DPB_SIZE],
    mfxU8(&numRefActive)[2])
{
    ConstructRPL(par, DPB, isB, poc, tid, level, bSecondField, bBottomField, RPL, numRefActive, 0, 0);
}

void UpdateDPB(
    MfxVideoParam const & par,
    DpbFrame const & task,
    DpbArray & dpb,
    mfxExtAVCRefListCtrl * pLCtrl = 0);

bool isLTR(
    DpbArray const & dpb,
    mfxU32 LTRInterval,
    mfxI32 poc);

inline mfxI32 GetFrameNum(bool bField, mfxI32 Poc, bool bSecondField)
{
    return bField ? (Poc + (!bSecondField)) / 2 : Poc;
}

}; //namespace MfxHwH265Encode
#endif
