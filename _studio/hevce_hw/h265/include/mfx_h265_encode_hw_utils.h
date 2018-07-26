// Copyright (c) 2018 Intel Corporation
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
#include "umc_mutex.h"
#include "mfxla.h"

#include "mfxbrc.h"

#include <typeinfo>
#include <vector>
#include <list>
#include <assert.h>
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
template<class T, class U> inline void Copy(T & dst, U const & src)
{
    static_assert(sizeof(T) == sizeof(U), "copy_objects_of_different_size");
    memcpy_s(&dst, sizeof(dst), &src, sizeof(dst));
}
template<class T> inline void CopyN(T* dst, const T* src, size_t N)
{
    memcpy_s(dst, sizeof(T) * N, src, sizeof(T) * N);
}
template<class T> inline T Abs  (T x)               { return (x > 0 ? x : -x); }
template<class T> inline T Min  (T x, T y)          { return MFX_MIN(x, y); }
template<class T> inline T Max  (T x, T y)          { return MFX_MAX(x, y); }
template<class T> inline T Clip3(T min, T max, T x) { return Min(Max(min, x), max); }
template<class T> inline T Align(T value, mfxU32 alignment)
{
    assert((alignment & (alignment - 1)) == 0); // should be 2^n
    return T((value + alignment - 1) & ~(alignment - 1));
}
template<class T> bool AlignDown(T& value, mfxU32 alignment)
{
    assert((alignment & (alignment - 1)) == 0); // should be 2^n
    if (!(value & (alignment - 1))) return false;
    value = value & ~(alignment - 1);
    return true;
}
template<class T> bool AlignUp(T& value, mfxU32 alignment)
{
    assert((alignment & (alignment - 1)) == 0); // should be 2^n
    if (!(value & (alignment - 1))) return false;
    value = (value + alignment - 1) & ~(alignment - 1);
    return true;
}
template<class T> bool IsAligned(T value, mfxU32 alignment)
{
    assert((alignment & (alignment - 1)) == 0); // should be 2^n
    return !(value & (alignment - 1));
}
template<class T> inline T Lsb(T val, mfxU32 maxLSB)
{
    if (val >= 0)
        return val % maxLSB;
    return (maxLSB - ((-val) % maxLSB)) % maxLSB;
}
template <class T, class U0, class U1>
bool CheckRange(T & opt, U0 min, U1 max)
{
    if (opt < min)
    {
        opt = T(min);
        return true;
    }

    if (opt > max)
    {
        opt = T(max);
        return true;
    }

    return false;
}
template<class T> inline T* AlignPtrL(void* p) { return (T*)(size_t(p) & ~(sizeof(T) - 1)); }
template<class T> inline T* AlignPtrR(void* p) { return AlignPtrL<T>(p) + !!(size_t(p) & (sizeof(T) - 1)); }
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
inline mfxU64 CeilDiv   (mfxU64 x, mfxU64 y) { return (x + y - 1) / y; }
inline mfxU32 Ceil      (mfxF64 x)           { return (mfxU32)(.999 + x); }
inline mfxU64 SwapEndian(mfxU64 val)
{
    return
        ((val << 56) & 0xff00000000000000) | ((val << 40) & 0x00ff000000000000) |
        ((val << 24) & 0x0000ff0000000000) | ((val <<  8) & 0x000000ff00000000) |
        ((val >>  8) & 0x00000000ff000000) | ((val >> 24) & 0x0000000000ff0000) |
        ((val >> 40) & 0x000000000000ff00) | ((val >> 56) & 0x00000000000000ff);
}

enum
{
    MFX_IOPATTERN_IN_MASK = MFX_IOPATTERN_IN_SYSTEM_MEMORY | MFX_IOPATTERN_IN_VIDEO_MEMORY | MFX_IOPATTERN_IN_OPAQUE_MEMORY,
    MFX_MEMTYPE_D3D_INT = MFX_MEMTYPE_FROM_ENCODE | MFX_MEMTYPE_DXVA2_DECODER_TARGET | MFX_MEMTYPE_INTERNAL_FRAME,
    MFX_MEMTYPE_D3D_EXT = MFX_MEMTYPE_FROM_ENCODE | MFX_MEMTYPE_DXVA2_DECODER_TARGET | MFX_MEMTYPE_EXTERNAL_FRAME,
    MFX_MEMTYPE_SYS_EXT = MFX_MEMTYPE_FROM_ENCODE | MFX_MEMTYPE_SYSTEM_MEMORY        | MFX_MEMTYPE_EXTERNAL_FRAME,
    MFX_MEMTYPE_SYS_INT = MFX_MEMTYPE_FROM_ENCODE | MFX_MEMTYPE_SYSTEM_MEMORY        | MFX_MEMTYPE_INTERNAL_FRAME,
};

enum
{
    MAX_DPB_SIZE            = 15,
    IDX_INVALID             = 0xFF,

    HW_SURF_ALIGN_W         = 16,
    HW_SURF_ALIGN_H         = 16,

    CODED_PIC_ALIGN_W       = 16,
    CODED_PIC_ALIGN_H       = 16,

    MAX_SLICES              = 200,// WA for driver issue regerding CNL and older platforms
    DEFAULT_LTR_INTERVAL    = 16,
    DEFAULT_PPYR_INTERVAL   = 3,

    MAX_NUM_ROI             = 8,
    MAX_NUM_DIRTY_RECT      = 64
};


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

inline bool IsOn(mfxU32 opt)
{
    return opt == MFX_CODINGOPTION_ON;
}

inline bool IsOff(mfxU32 opt)
{
    return opt == MFX_CODINGOPTION_OFF;
}

class MfxFrameAllocResponse : public mfxFrameAllocResponse
{
public:
    MfxFrameAllocResponse();

    ~MfxFrameAllocResponse();

    mfxStatus Alloc(
        MFXCoreInterface *     core,
        mfxFrameAllocRequest & req,
        bool                   isCopyRequired);

    mfxU32 Lock(mfxU32 idx);

    void Unlock();

    mfxU32 Unlock(mfxU32 idx);

    mfxU32 Locked(mfxU32 idx) const;

    void Free();

    bool isExternal() { return m_isExternal; };

    mfxU32 FindFreeResourceIndex(mfxFrameSurface1* external_surf = 0);

    void   ClearFlag  (mfxU32 idx);
    void   SetFlag    (mfxU32 idx, mfxU32 flag);
    mfxU32 GetFlag    (mfxU32 idx);
    mfxFrameInfo               m_info;

private:
    MfxFrameAllocResponse(MfxFrameAllocResponse const &);
    MfxFrameAllocResponse & operator =(MfxFrameAllocResponse const &);

    MFXCoreInterface * m_core;
    mfxU16             m_numFrameActualReturnedByAllocFrames;

    std::vector<mfxFrameAllocResponse> m_responseQueue;
    std::vector<mfxMemId>              m_mids;
    std::vector<mfxU32>                m_locked;
    std::vector<mfxU32>                m_flag;
    bool m_isExternal;
};

typedef struct _DpbFrame
{
    mfxI32   m_poc;
    mfxU32   m_fo;  // FrameOrder
    mfxU32   m_eo;  // Encoded order
    mfxU32   m_bpo; // Bpyramid order
    mfxU32   m_level; //pyramid level
    mfxU8    m_tid;
    bool     m_ltr; // is "long-term"
    bool     m_ldb; // is "low-delay B"
    bool     m_secondField;
    bool     m_bottomField;
    mfxU8    m_codingType;
    mfxU8    m_idxRaw;
    mfxU8    m_idxRec;
    mfxMemId m_midRec;
    mfxMemId m_midRaw;
    mfxFrameSurface1*   m_surf; //input surface, may be opaque
}DpbFrame, DpbArray[MAX_DPB_SIZE];

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

    mfxI16  Priority;
};

typedef struct _Task : DpbFrame
{
    mfxBitstream*       m_bs;
    mfxFrameSurface1*   m_surf_real;
    mfxEncodeCtrl       m_ctrl;
    Slice               m_sh;

    mfxU32 m_idxBs;
    mfxU8  m_idxCUQp;
    bool   m_bCUQPMap;

    mfxU8 m_refPicList[2][MAX_DPB_SIZE];
    mfxU8 m_numRefActive[2];

    mfxU16 m_frameType;
    mfxU16 m_insertHeaders;
    mfxU8  m_shNUT;
    mfxI8  m_qpY;
    mfxI32 m_lastIPoc;
    mfxI32 m_lastRAP;

    mfxU32 m_statusReportNumber;
    mfxU32 m_bsDataLength;
    mfxU32 m_minFrameSize;

    DpbArray m_dpb[TASK_DPB_NUM];

    mfxMemId m_midBs;
    mfxMemId m_midCUQp;

    bool m_resetBRC;

    mfxU32 m_initial_cpb_removal_delay;
    mfxU32 m_initial_cpb_removal_offset;
    mfxU32 m_cpb_removal_delay;
    mfxU32 m_dpb_output_delay;

    mfxU32 m_stage;
    mfxU32 m_recode;
    IntraRefreshState m_IRState;
    bool m_bSkipped;

    RoiData       m_roi[MAX_NUM_ROI];
    mfxU16        m_roiMode;    // BRC only
    mfxU16        m_numRoi;
    bool          m_bPriorityToDQPpar;

#ifdef MFX_ENABLE_HEVCE_DIRTY_RECT
    RectData      m_dirtyRect[MAX_NUM_DIRTY_RECT];
    mfxU16        m_numDirtyRect;
#endif  // MFX_ENABLE_HEVCE_DIRTY_RECT

    mfxU16        m_SkipMode;


}Task;

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
#if defined(MFX_ENABLE_HEVCE_WEIGHTED_PREDICTION)
        EXTBUF(mfxExtPredWeightTable,       MFX_EXTBUFF_PRED_WEIGHT_TABLE);
#endif //defined(MFX_ENABLE_HEVCE_WEIGHTED_PREDICTION)

    #undef EXTBUF

    #define _CopyPar(dst, src, PAR) dst.PAR = src.PAR;
    #define _CopyPar1(PAR) _CopyPar(buf_dst, buf_src, PAR);
    #define _CopyStruct1(PAR) Copy(buf_dst.PAR, buf_src.PAR);
    #define _NO_CHECK()  buf_dst = buf_src;

    template <class T> void CopySupportedParams(T& buf_dst, T& buf_src)
    {
        _NO_CHECK();
    }

    inline void CopySupportedParams(mfxExtHEVCParam& buf_dst, mfxExtHEVCParam& buf_src)
    {
        _CopyPar1(PicWidthInLumaSamples);
        _CopyPar1(PicHeightInLumaSamples);
#if (MFX_VERSION >= 1026)
        _CopyPar1(LCUSize);
#endif
    }

    inline void  CopySupportedParams(mfxExtHEVCTiles& buf_dst, mfxExtHEVCTiles& buf_src)
    {
        _CopyPar1(NumTileRows);
        _CopyPar1(NumTileColumns);
    }

    inline void CopySupportedParams (mfxExtCodingOption& buf_dst, mfxExtCodingOption& buf_src)
    {
        _CopyPar1(PicTimingSEI);
        _CopyPar1(VuiNalHrdParameters);
        _CopyPar1(NalHrdConformance);
        _CopyPar1(AUDelimiter);
    }
    inline void  CopySupportedParams(mfxExtCodingOption2& buf_dst, mfxExtCodingOption2& buf_src)
    {
        _CopyPar1(IntRefType);
        _CopyPar1(IntRefCycleSize);
        _CopyPar1(IntRefQPDelta);
        _CopyPar1(MaxFrameSize);

        _CopyPar1(MBBRC);
        _CopyPar1(BRefType);
        _CopyPar1(NumMbPerSlice);
        _CopyPar1(DisableDeblockingIdc);

        _CopyPar1(RepeatPPS);
        _CopyPar1(MaxSliceSize);
        _CopyPar1(ExtBRC);

        _CopyPar1(MinQPI);
        _CopyPar1(MaxQPI);
        _CopyPar1(MinQPP);
        _CopyPar1(MaxQPP);
        _CopyPar1(MinQPB);
        _CopyPar1(MaxQPB);
        _CopyPar1(SkipFrame);
    }

    inline void  CopySupportedParams(mfxExtCodingOption3& buf_dst, mfxExtCodingOption3& buf_src)
    {
        _CopyPar1(PRefType);
        _CopyPar1(IntRefCycleDist);
        _CopyPar1(EnableQPOffset);
        _CopyPar1(GPB);
        _CopyStruct1(QPOffset);
        _CopyStruct1(NumRefActiveP);
        _CopyStruct1(NumRefActiveBL0);
        _CopyStruct1(NumRefActiveBL1);
        _CopyStruct1(QVBRQuality);
        _CopyPar1(EnableMBQP);
        _CopyPar1(WinBRCMaxAvgKbps);
        _CopyPar1(WinBRCSize);
#if defined(MFX_ENABLE_HEVCE_WEIGHTED_PREDICTION)
        _CopyPar1(WeightedPred);
        _CopyPar1(WeightedBiPred);
#if defined(MFX_ENABLE_HEVCE_FADE_DETECTION)
        _CopyPar1(FadeDetection);
#endif //defined(MFX_ENABLE_HEVCE_FADE_DETECTION)
#endif //defined(MFX_ENABLE_HEVCE_WEIGHTED_PREDICTION)
#if (MFX_VERSION >= 1025)
        _CopyPar1(EnableNalUnitType);
#endif
    }

    inline void  CopySupportedParams(mfxExtCodingOptionDDI& buf_dst, mfxExtCodingOptionDDI& buf_src)
    {
        _CopyPar1(NumActiveRefBL0);
        _CopyPar1(NumActiveRefBL1);
        _CopyPar1(NumActiveRefP);
        _CopyPar1(LCUSize);
        _CopyPar1(QpAdjust);
    }
    inline void  CopySupportedParams(mfxExtEncoderCapability& buf_dst, mfxExtEncoderCapability& buf_src)
    {
        _CopyPar1(MBPerSec);
    }

    #undef _CopyPar
    #undef _CopyPar1
    #undef _CopyStruct1
    #undef _NO_CHECK

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
            memcpy_s(&buf, sizeof(T), &buf_ref, sizeof(T));
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

        memcpy_s(notDetected, sizeof(notDetected), allowed_buffers, sizeof(allowed_buffers));

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

        memcpy_s(notDetected1, sizeof(notDetected1), allowed_buffers, sizeof(allowed_buffers));
        memcpy_s(notDetected2, sizeof(notDetected2), allowed_buffers, sizeof(allowed_buffers));

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

        m_numTL = Max<mfxU8>(m_numTL, 1);
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
    mfxPlatform m_platform;
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
    bool   HRDConformance;
    bool   RawRef;
    bool   bROIViaMBQP;
    bool   bMBQPInput;
    bool   RAPIntra;
    bool   bFieldReord;
    bool   bNonStandardReord; // Possible in encoding order only. No NumRefFrames limitation.


    MfxVideoParam();
    MfxVideoParam(MfxVideoParam const & par);
    MfxVideoParam(mfxVideoParam const & par);

    MfxVideoParam & operator = (mfxVideoParam const &);
    MfxVideoParam & operator = (MfxVideoParam const &);

    void SyncVideoToCalculableParam();
    void SyncCalculableToVideoParam();
    void AlignCalcWithBRCParamMultiplier();
    void SyncMfxToHeadersParam(mfxU32 numSlicesForSTRPSOpt = 0);
    void SyncHeadersToMfxParam();

    mfxStatus FillPar(mfxVideoParam& par, bool query = false);

    mfxStatus GetSliceHeader(Task const & task, Task const & prevTask, ENCODE_CAPS_HEVC const & caps, Slice & s) const;

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

class HRD
{
public:
    HRD():
        m_bIsHrdRequired(false)
        , m_cbrFlag(false)
        , m_bitrate(0)
        , m_maxCpbRemovalDelay(0)
        , m_clockTick(0)
        , m_cpbSize90k(0)
        , m_initCpbRemovalDelay(0)
        , m_prevAuCpbRemovalDelayMinus1(0)
        , m_prevAuCpbRemovalDelayMsb(0)
        , m_prevAuFinalArrivalTime(0)
        , m_prevBpAuNominalRemovalTime(0)
        , m_prevBpEncOrder(0)
    {}
    void Init(const SPS &sps, mfxU32 InitialDelayInKB);
    void Reset(SPS const & sps);
    void Update(mfxU32 sizeInbits, const Task &pic);
    mfxU32 GetInitCpbRemovalDelay(const Task &pic);

    /*inline mfxU32 GetInitCpbRemovalDelay() const
    {
        return (mfxU32)m_initCpbRemovalDelay;
    }*/

    inline mfxU32 GetInitCpbRemovalDelayOffset() const
    {
        return mfxU32(m_cpbSize90k - m_initCpbRemovalDelay);
    }

    /*inline mfxU32 GetAuCpbRemovalDelayMinus1(const Task &pic) const
    {
        return (pic.m_eo == 0) ? 0 : (pic.m_eo - m_prevBpEncOrder) - 1;
    }*/

    inline bool Enabled() { return m_bIsHrdRequired; }

protected:
    bool   m_bIsHrdRequired;
    bool   m_cbrFlag;
    mfxU32 m_bitrate;
    mfxU32 m_maxCpbRemovalDelay;
    mfxF64 m_clockTick;
    mfxF64 m_cpbSize90k;
    mfxF64 m_initCpbRemovalDelay;

    mfxI32 m_prevAuCpbRemovalDelayMinus1;
    mfxU32 m_prevAuCpbRemovalDelayMsb;
    mfxF64 m_prevAuFinalArrivalTime;
    mfxF64 m_prevBpAuNominalRemovalTime;
    mfxU32 m_prevBpEncOrder;
};


class TaskManager
{
public:
    TaskManager();
    void  Reset     (bool bFieldMode , mfxU32 numTask = 0, mfxU16 resetHeaders = 0);
    Task* New       ();
    Task* Reorder   (MfxVideoParam const & par, DpbArray const & dpb, bool flush);
    void  Submit    (Task* task);
    Task* GetTaskForSubmit(bool bRealTask = false);
    void  SubmitForQuery(Task* task);
    bool  isSubmittedForQuery(Task* task);
    Task* GetTaskForQuery();
    void  Ready     (Task* task);
    void  SkipTask  (Task* task);
    Task* GetNewTask();
    mfxStatus PutTasksForRecode(Task* pTask);

private:
    bool       m_bFieldMode;
    TaskList   m_free;
    TaskList   m_reordering;
    TaskList   m_encoding;
    TaskList   m_querying;
    UMC::Mutex m_listMutex;
    mfxU16     m_resetHeaders;

};

class FrameLocker : public mfxFrameData
{
public:
    FrameLocker(MFXCoreInterface* core, mfxMemId mid)
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
    mfxStatus Lock()   { return m_core->FrameAllocator().Lock  (m_core->FrameAllocator().pthis, m_mid, this); }
    mfxStatus Unlock() { return m_core->FrameAllocator().Unlock(m_core->FrameAllocator().pthis, m_mid, this); }

private:
    MFXCoreInterface* m_core;
    mfxMemId m_mid;
};

inline bool isValid(DpbFrame const & frame) { return IDX_INVALID !=  frame.m_idxRec; }
inline bool isDpbEnd(DpbArray const & dpb, mfxU32 idx) { return idx >= MAX_DPB_SIZE || !isValid(dpb[idx]); }

mfxU8 GetFrameType(MfxVideoParam const & video, mfxU32 frameOrder);


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

void ConfigureTask(
    Task &                   task,
    Task const &             prevTask,
    MfxVideoParam const &    video,
    ENCODE_CAPS_HEVC const & caps,
    mfxU32 &                 baseLayerOrder);

mfxI64 CalcDTSFromPTS(
    mfxFrameInfo const & info,
    mfxU16               dpbOutputDelay,
    mfxU64               timeStamp);

mfxU32 FindFreeResourceIndex(
    MfxFrameAllocResponse const & pool,
    mfxU32                        startingFrom = 0);

mfxMemId AcquireResource(
    MfxFrameAllocResponse & pool,
    mfxU32                  index);

void ReleaseResource(
    MfxFrameAllocResponse & pool,
    mfxMemId                mid);

mfxStatus GetNativeHandleToRawSurface(
    MFXCoreInterface &    core,
    MfxVideoParam const & video,
    Task const &          task,
    mfxHDLPair &          handle);

mfxStatus CopyRawSurfaceToVideoMemory(
    MFXCoreInterface &    core,
    MfxVideoParam const & video,
    Task const &          task);

IntraRefreshState GetIntraRefreshState(
    MfxVideoParam const & video,
    mfxU32                frameOrderInGopDispOrder,
    mfxEncodeCtrl const * ctrl);

mfxU8 GetNumReorderFrames(
    mfxU32 BFrameRate,
    bool BPyramid,
    bool bField,
    bool bFieldReord);



bool IsFrameToSkip(
    Task&  task,
    MfxFrameAllocResponse & poolRec,
    bool bSWBRC);

mfxStatus CodeAsSkipFrame(
    MFXCoreInterface & core,
    MfxVideoParam const & video,
    Task & task,
    MfxFrameAllocResponse & poolSkip,
    MfxFrameAllocResponse & poolRec);

mfxStatus CheckAndFixRoi(
    MfxVideoParam const & par,
    ENCODE_CAPS_HEVC const & caps,
    mfxExtEncoderROI * ROI,
    bool &bROIViaMBQP);

mfxStatus CheckAndFixDirtyRect(
    ENCODE_CAPS_HEVC const & caps,
    MfxVideoParam const & par,
    mfxExtDirtyRect * DirtyRect);
mfxStatus GetCUQPMapBlockSize(mfxU16 frameWidth, mfxU16 frameHeight,
    mfxU16 CUQPWidth, mfxU16 CUHeight,
    mfxU16 &blkWidth, mfxU16 &blkHeight);



enum
{
    HEVC_SKIPFRAME_NO                        = 0,
    HEVC_SKIPFRAME_INSERT_DUMMY              = 1,
    HEVC_SKIPFRAME_INSERT_DUMMY_PROTECTED    = 2,
    HEVC_SKIPFRAME_INSERT_NOTHING            = 3,
    HEVC_SKIPFRAME_BRC_ONLY                  = 4,
    HEVC_SKIPFRAME_EXT_PSEUDO                = 5,
};

class HevcSkipMode
{
public:
    HevcSkipMode(): m_mode(0)
    {};
    HevcSkipMode(mfxU16 mode): m_mode(mode)
    {};
private:
    mfxU16 m_mode;
public:
    void SetMode(mfxU16 skipModeMFX, bool bProtected)
    {
        switch (skipModeMFX)
        {
        case MFX_SKIPFRAME_INSERT_DUMMY :
            m_mode = (mfxU16)(bProtected ? HEVC_SKIPFRAME_INSERT_DUMMY_PROTECTED : HEVC_SKIPFRAME_INSERT_DUMMY);
            break;
        case MFX_SKIPFRAME_INSERT_NOTHING:
            m_mode = HEVC_SKIPFRAME_INSERT_NOTHING;
            break;
        case MFX_SKIPFRAME_BRC_ONLY:
            m_mode = HEVC_SKIPFRAME_BRC_ONLY;
            break;
        default:
            m_mode = HEVC_SKIPFRAME_NO;
            break;
        }
    }
    void SetPseudoSkip ()
    {
        m_mode = HEVC_SKIPFRAME_EXT_PSEUDO;
    }
    mfxU16 GetMode() {return m_mode;}

    bool NeedInputReplacement()
    {
        return m_mode == HEVC_SKIPFRAME_EXT_PSEUDO;
    }
    bool NeedDriverCall()
    {
        return m_mode == HEVC_SKIPFRAME_INSERT_DUMMY_PROTECTED || m_mode == HEVC_SKIPFRAME_EXT_PSEUDO || m_mode == HEVC_SKIPFRAME_NO || m_mode == HEVC_SKIPFRAME_EXT_PSEUDO;
    }
    bool NeedSkipSliceGen()
    {
        return m_mode == HEVC_SKIPFRAME_INSERT_DUMMY_PROTECTED || m_mode == HEVC_SKIPFRAME_INSERT_DUMMY ;
    }
    bool NeedCurrentFrameSkipping()
    {
        return m_mode == HEVC_SKIPFRAME_INSERT_DUMMY_PROTECTED || m_mode == HEVC_SKIPFRAME_INSERT_DUMMY || m_mode == HEVC_SKIPFRAME_INSERT_NOTHING;
    }
    bool NeedNumSkipAdding()
    {
         return m_mode == HEVC_SKIPFRAME_BRC_ONLY;
    }
};
inline mfxI32 GetFrameNum(bool bField, mfxI32 Poc, bool bSecondField)
{
    return bField ? (Poc + (!bSecondField)) / 2 : Poc;
}

}; //namespace MfxHwH265Encode
#endif
