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
#ifdef MFX_ENABLE_H264_VIDEO_ENCODE_HW

#include <vector>
#include <list>
#include <memory>
#include <algorithm> /* for std::find_if on Linux/Android */
#include <mfx_brc_common.h>

#include "mfx_h264_encode_struct_vaapi.h"
#include "umc_mutex.h"
#include "umc_event.h"
#include "umc_h264_brc.h"
#include "mfx_h264_enc_common_hw.h"
#include "mfx_ext_buffers.h"
#include "mfx_h264_encode_interface.h"
#include "mfx_h264_encode_cm.h"
#include "vm_time.h"
#include "asc.h"


#ifndef _MFX_H264_ENCODE_HW_UTILS_H_
#define _MFX_H264_ENCODE_HW_UTILS_H_

#define CLIPVAL(MINVAL, MAXVAL, VAL) (MFX_MAX(MINVAL, MFX_MIN(MAXVAL, VAL)))
#define bRateControlLA(RCMethod) ((RCMethod == MFX_RATECONTROL_LA)||(RCMethod == MFX_RATECONTROL_LA_ICQ)||(RCMethod == MFX_RATECONTROL_LA_EXT)||(RCMethod == MFX_RATECONTROL_LA_HRD))
#define bIntRateControlLA(RCMethod) ((RCMethod == MFX_RATECONTROL_LA)||(RCMethod == MFX_RATECONTROL_LA_ICQ)||(RCMethod == MFX_RATECONTROL_LA_HRD))

#define MFX_H264ENC_HW_TASK_TIMEOUT 2000

#define MFX_ARRAY_SIZE(ARR) (sizeof(ARR)/sizeof(ARR[0]))
const int MFX_MAX_DIRTY_RECT_COUNT = MFX_ARRAY_SIZE(mfxExtDirtyRect::Rect);
const int MFX_MAX_MOVE_RECT_COUNT = MFX_ARRAY_SIZE(mfxExtMoveRect::Rect);


namespace MfxHwH264Encode
{
    struct VmeData;

    enum {
        STAGE_QP   = 0xf00000,
        STAGE_ENC1 = 0x0f0000,
        STAGE_PAK1 = 0x00f000,
        STAGE_ENC2 = 0x000f00,
        STAGE_PAK2 = 0x0000f0,
        STAGE_ENC  = STAGE_ENC1 | STAGE_ENC2,
        STAGE_PAK  = STAGE_PAK1 | STAGE_PAK2,
        STAGE_ALL  = 0xffffff
    };

    static const mfxU16 MFX_MEMTYPE_SYS_INT =
        MFX_MEMTYPE_FROM_ENCODE | MFX_MEMTYPE_SYSTEM_MEMORY | MFX_MEMTYPE_INTERNAL_FRAME;

    static const mfxU16 MFX_MEMTYPE_SYS_EXT =
        MFX_MEMTYPE_FROM_ENCODE | MFX_MEMTYPE_SYSTEM_MEMORY | MFX_MEMTYPE_EXTERNAL_FRAME;

    static const mfxU16 MFX_MEMTYPE_D3D_INT =
        MFX_MEMTYPE_FROM_ENCODE | MFX_MEMTYPE_DXVA2_DECODER_TARGET | MFX_MEMTYPE_INTERNAL_FRAME;

    static const mfxU16 MFX_MEMTYPE_D3D_EXT =
        MFX_MEMTYPE_FROM_ENCODE | MFX_MEMTYPE_DXVA2_DECODER_TARGET | MFX_MEMTYPE_EXTERNAL_FRAME;


    mfxU16 CalcNumFrameMin(const MfxHwH264Encode::MfxVideoParam &par);

    enum
    {
        TFIELD = 0,
        BFIELD = 1
    };

    enum
    {
        ENC = 0,
        DISP = 1
    };

    /*
        NAL unit types for internal usage
    */
    enum {
        NALU_NON_IDR = 1,
        NALU_IDR = 5,
        NALU_SEI = 6,
        NALU_SPS = 7,
        NALU_PPS = 8,
        NALU_AUD = 9,
        NALU_PREFIX = 14,
        NALU_CODED_SLICE_EXT = 20,
    };


    class DdiTask;

    struct mfxExtAvcSeiBufferingPeriod
    {
        mfxU8  seq_parameter_set_id;
        mfxU8  nal_cpb_cnt;
        mfxU8  vcl_cpb_cnt;
        mfxU8  initial_cpb_removal_delay_length;
        mfxU32 nal_initial_cpb_removal_delay[32];
        mfxU32 nal_initial_cpb_removal_delay_offset[32];
        mfxU32 vcl_initial_cpb_removal_delay[32];
        mfxU32 vcl_initial_cpb_removal_delay_offset[32];
    };

    struct mfxExtAvcSeiPicTiming
    {
        mfxU8  cpb_dpb_delays_present_flag;
        mfxU8  cpb_removal_delay_length;
        mfxU8  dpb_output_delay_length;
        mfxU8  pic_struct_present_flag;
        mfxU8  time_offset_length;

        mfxU32 cpb_removal_delay;
        mfxU32 dpb_output_delay;
        mfxU8  pic_struct;
        mfxU8  ct_type;
    };

    struct mfxExtAvcSeiDecRefPicMrkRep
    {
        mfxU8  original_idr_flag;
        mfxU16 original_frame_num;
        mfxU8  original_field_info_present_flag;
        mfxU8  original_field_pic_flag;
        mfxU8  original_bottom_field_flag;
        mfxU8  no_output_of_prior_pics_flag;
        mfxU8  long_term_reference_flag;
        mfxU8  adaptive_ref_pic_marking_mode_flag;
        mfxU32 num_mmco_entries;                // number of currently valid mmco, value pairs
        mfxU8  mmco[32];                        // memory management control operation id
        mfxU32 value[64];                       // operation-dependent data, max 2 per operation
    };

    struct mfxExtAvcSeiRecPoint
    {
        mfxU16 recovery_frame_cnt;
        mfxU8  exact_match_flag;
        mfxU8  broken_link_flag;
        mfxU8  changing_slice_group_idc;
    };


    template <typename T> struct Pair
    {
        T top;
        T bot;

        Pair()
            : top()
            , bot()
        {
        }

        template<typename U> Pair(Pair<U> const & pair)
            : top(static_cast<T>(pair.top))
            , bot(static_cast<T>(pair.bot))
        {
        }

        template<typename U> explicit Pair(U const & value)
            : top(static_cast<T>(value))
            , bot(static_cast<T>(value))
        {
        }

        template<typename U> Pair(U const & t, U const & b)
            : top(static_cast<T>(t))
            , bot(static_cast<T>(b))
        {
        }

        template<typename U>
        Pair<T> & operator =(Pair<U> const & pair)
        {
            Pair<T> tmp(pair);
            std::swap(*this, tmp);
            return *this;
        }

        T & operator[] (mfxU32 parity)
        {
            assert(parity < 2);
            return (&top)[parity & 1];
        }

        T const & operator[] (mfxU32 parity) const
        {
            assert(parity < 2);
            return (&top)[parity & 1];
        }
    };

    template <class T> Pair<T> MakePair(T const & top, T const & bot)
    {
        return Pair<T>(top, bot);
    }

    template <class T> bool operator ==(Pair<T> const & l, Pair<T> const & r)
    {
        return l.top == r.top && l.bot == r.bot;
    }

    typedef Pair<mfxU8>  PairU8;
    typedef Pair<mfxU16> PairU16;
    typedef Pair<mfxU32> PairU32;
    typedef Pair<mfxI32> PairI32;

    void PrepareSeiMessage(
        DdiTask const &               task,
        mfxU32                        nalHrdBpPresentFlag,
        mfxU32                        vclHrdBpPresentFlag,
        mfxU32                        seqParameterSetId,
        mfxExtAvcSeiBufferingPeriod & msg);

    void PrepareSeiMessage(
        DdiTask const &                task,
        mfxU32                         fieldId,
        mfxU32                         cpbDpbDelaysPresentFlag,
        mfxExtAvcSeiPicTiming &        msg);

    void PrepareSeiMessage(
        DdiTask const &               task,
        mfxU32                        fieldId,
        mfxU32                        frame_mbs_only_flag,
        mfxExtAvcSeiDecRefPicMrkRep & msg);

    void PrepareSeiMessage(
        MfxVideoParam const &   par,
        mfxExtAvcSeiRecPoint &  msg);

    mfxU32 CalculateSeiSize( mfxExtAvcSeiRecPoint const & msg);
    mfxU32 CalculateSeiSize( mfxExtAvcSeiDecRefPicMrkRep const & msg);

// MVC BD {
    mfxU32 CalculateSeiSize( mfxExtAvcSeiBufferingPeriod const & msg);
    mfxU32 CalculateSeiSize(
        mfxExtPictureTimingSEI const & extPt,
        mfxExtAvcSeiPicTiming const & msg);
// MVC BD }

    mfxStatus CheckBeforeCopy(
        mfxExtMVCSeqDesc &       dst,
        mfxExtMVCSeqDesc const & src);

    mfxStatus CheckBeforeCopyQueryLike(
        mfxExtMVCSeqDesc &       dst,
        mfxExtMVCSeqDesc const & src);

    void Copy(
        mfxExtMVCSeqDesc &       dst,
        mfxExtMVCSeqDesc const & src);

    void FastCopyBufferVid2Sys(
        void *       dstSys,
        void const * srcVid,
        mfxI32       bytes);

    void FastCopyBufferSys2Vid(
        void *       dstSys,
        void const * srcVid,
        mfxI32       bytes);

    PairU8 ExtendFrameType(
        mfxU32 type);

    mfxU8 CalcTemporalLayerIndex(
        MfxVideoParam const & video,
        mfxI32                frameOrder);

    bool CheckSubMbPartition(
        mfxExtCodingOptionDDI const * extDdi,
        mfxU8                         frameType);

    mfxU8 GetQpValue(
        MfxVideoParam const & par,
        mfxEncodeCtrl const & ctrl,
        mfxU32                frameType);

    PairU16 GetPicStruct(
        MfxVideoParam const & video,
        mfxU16                runtPs);

    PairU16 GetPicStruct(
        MfxVideoParam const & video,
        DdiTask const &       task);

    bool isBitstreamUpdateRequired(MfxVideoParam const & video,
        ENCODE_CAPS caps,
        eMFXHWType platform);
    // Helper which checks number of allocated frames and auto-free.

    class MfxFrameAllocResponse : public mfxFrameAllocResponse
    {
    public:
        MfxFrameAllocResponse();

        ~MfxFrameAllocResponse();

        mfxStatus Alloc(
            VideoCORE *            core,
            mfxFrameAllocRequest & req,
            bool isCopyRequired = true,
            bool isAllFramesRequired = false);

        mfxStatus Alloc(
            VideoCORE *            core,
            mfxFrameAllocRequest & req,
            mfxFrameSurface1 **    opaqSurf,
            mfxU32                 numOpaqSurf);

        mfxStatus AllocCmBuffers(
            CmDevice *             device,
            mfxFrameAllocRequest & req);

        mfxStatus AllocCmBuffersUp(
            CmDevice *             device,
            mfxFrameAllocRequest & req);

        mfxStatus AllocCmSurfaces(
            CmDevice *             device,
            mfxFrameAllocRequest & req);

        mfxStatus AllocCmSurfacesUP(
            CmDevice *             device,
            mfxFrameAllocRequest & req);
        mfxStatus AllocFrames(
            VideoCORE *            core,
            mfxFrameAllocRequest & req);
        mfxStatus UpdateResourcePointers(
            mfxU32                 idxScd,
            void *                 memY,
            void *                 gpuSurf);

        void * GetSysmemBuffer(mfxU32 idx);

        mfxU32 Lock(mfxU32 idx);

        void Unlock();

        mfxU32 Unlock(mfxU32 idx);

        mfxU32 Locked(mfxU32 idx) const;

    private:
        MfxFrameAllocResponse(MfxFrameAllocResponse const &);
        MfxFrameAllocResponse & operator =(MfxFrameAllocResponse const &);

        static void DestroyBuffer     (CmDevice * device, void * p);
        static void DestroySurface    (CmDevice * device, void * p);
        static void DestroySurface2DUP(CmDevice * device, void * p);
        static void DestroyBufferUp   (CmDevice * device, void * p);
        
        void (*m_cmDestroy)(CmDevice *, void *);

        VideoCORE * m_core;
        CmDevice *  m_cmDevice;
        mfxU16      m_numFrameActualReturnedByAllocFrames;

        std::vector<mfxFrameAllocResponse> m_responseQueue;
        std::vector<mfxMemId>              m_mids;
        std::vector<mfxU32>                m_locked;
        std::vector<void *>                m_sysmems;
    };

    mfxU32 FindFreeResourceIndex(
        MfxFrameAllocResponse const & pool,
        mfxU32                        startingFrom = 0);

    mfxMemId AcquireResource(
        MfxFrameAllocResponse & pool,
        mfxU32                  index);

    mfxMemId AcquireResource(
        MfxFrameAllocResponse & pool);

    mfxHDLPair AcquireResourceUp(
        MfxFrameAllocResponse & pool,
        mfxU32                  index);

    mfxHDLPair AcquireResourceUp(
        MfxFrameAllocResponse & pool);

    void ReleaseResource(
        MfxFrameAllocResponse & pool,
        mfxMemId                mid);

    // add hwType param
    mfxStatus CheckEncodeFrameParam(
        MfxVideoParam const & video,
        mfxEncodeCtrl *       ctrl,
        mfxFrameSurface1 *    surface,
        mfxBitstream *        bs,
        bool                  isExternalFrameAllocator,
        ENCODE_CAPS const &   caps,
        eMFXHWType            hwType = MFX_HW_UNKNOWN);

    template<typename T> void Clear(std::vector<T> & v)
    {
        std::vector<T>().swap(v);
    }


    template<class T, mfxU32 N>
    struct FixedArray
    {
        FixedArray()
            : m_numElem(0)
        {
        }

        explicit FixedArray(T fillVal)
            : m_numElem(0)
        {
            Fill(fillVal);
        }

        void PushBack(T const & val)
        {
            assert(m_numElem < N);
            m_arr[m_numElem] = val;
            m_numElem++;
        }

        void PushFront(T const val)
        {
            assert(m_numElem < N);
            std::copy(m_arr, m_arr + m_numElem, m_arr + 1);
            m_arr[0] = val;
            m_numElem++;
        }

        void Erase(T * p)
        {
            assert(p >= m_arr && p <= m_arr + m_numElem);

            m_numElem = mfxU32(
                std::copy(p + 1, m_arr + m_numElem, p) - m_arr);
        }

        void Erase(T * b, T * e)
        {
            assert(b <= e);
            assert(b >= m_arr && b <= m_arr + m_numElem);
            assert(e >= m_arr && e <= m_arr + m_numElem);

            m_numElem = mfxU32(
                std::copy(e, m_arr + m_numElem, b) - m_arr);
        }

        void Resize(mfxU32 size, T fillVal = T())
        {
            assert(size <= N);
            for (mfxU32 i = m_numElem ; i < size; ++i)
                m_arr[i] = fillVal;
            m_numElem = size;
        }

        T * Begin()
        {
            return m_arr;
        }

        T const * Begin() const
        {
            return m_arr;
        }

        T * End()
        {
            return m_arr + m_numElem;
        }

        T const * End() const
        {
            return m_arr + m_numElem;
        }

        T & Back()
        {
            assert(m_numElem > 0);
            return m_arr[m_numElem - 1];
        }

        T const & Back() const
        {
            assert(m_numElem > 0);
            return m_arr[m_numElem - 1];
        }

        mfxU32 Size() const
        {
            return m_numElem;
        }

        mfxU32 Capacity() const
        {
            return N;
        }

        T & operator[](mfxU32 idx)
        {
            assert(idx < N);
            return m_arr[idx];
        }

        T const & operator[](mfxU32 idx) const
        {
            assert(idx < N);
            return m_arr[idx];
        }

        void Fill(T val)
        {
            for (mfxU32 i = 0; i < N; i++)
            {
                m_arr[i] = val;
            }
        }

        template<mfxU32 M>
        bool operator==(const FixedArray<T, M>& r) const
        {
            assert(Size() <= N);
            assert(r.Size() <= M);

            if (Size() != r.Size())
            {
                return false;
            }

            for (mfxU32 i = 0; i < Size(); i++)
            {
                if (m_arr[i] != r[i])
                {
                    return false;
                }
            }

            return true;
        }

    private:
        T      m_arr[N];
        mfxU32 m_numElem;
    };

    struct BiFrameLocation
    {
        BiFrameLocation() { Zero(*this); }

        mfxU32 miniGopCount;    // sequence of B frames between I/P frames
        mfxU32 encodingOrder;   // number within mini-GOP (in encoding order)
        mfxU16 refFrameFlag;    // MFX_FRAMETYPE_REF if B frame is reference
        mfxU32 level;           // level of pyramid
    };

    class FrameTypeGenerator
    {
    public:
        FrameTypeGenerator();

        void Init(MfxVideoParam const & video);

        PairU8 Get() const;

        BiFrameLocation GetBiFrameLocation() const;

        void Next();

    private:
        mfxU32 m_frameOrder;    // in display order
        mfxU16 m_gopOptFlag;
        mfxU16 m_gopPicSize;
        mfxU16 m_gopRefDist;
        mfxU16 m_refBaseDist;   // key picture distance
        mfxU16 m_biPyramid;
        mfxU32 m_idrDist;
    };

    /* Intra refresh types */
    enum {
        MFX_REFRESH_NO = 0,
        MFX_REFRESH_VERTICAL = 1,
        MFX_REFRESH_HORIZONTAL = 2,
        MFX_REFRESH_SLICE = 3
    };

    struct IntraRefreshState
    {
        IntraRefreshState() : refrType(0), IntraLocation(0), IntraSize(0), IntRefQPDelta(0), firstFrameInCycle(false) {}

        mfxU16  refrType;
        mfxU16  IntraLocation;
        mfxU16  IntraSize;
        mfxI16  IntRefQPDelta;
        bool    firstFrameInCycle;
    };

    class Surface
    {
    public:
        Surface()
            : m_free(true)
        {
        }

        bool IsFree() const
        {
            return m_free;
        }

        void SetFree(bool free)
        {
            m_free = free;
        }

    private:
        bool m_free;
    };

    class Reconstruct : public Surface
    {
    public:
        Reconstruct()
            : Surface()
            , m_yuv(0)
            , m_frameOrderIdrInDisplayOrder(0)
            , m_frameOrderIdr(0)
            , m_frameOrderI(0)
            , m_frameOrder(0)
            , m_baseLayerOrder(0)
            , m_frameOrderStartTScalStructure(0)
            , m_frameNum(0)
            , m_frameNumWrap(0)
            , m_picNum(0, 0)
            , m_longTermFrameIdx(NO_INDEX_U8)
            , m_longTermPicNum(NO_INDEX_U8, NO_INDEX_U8)
            , m_reference(false, false)
            , m_picStruct((mfxU16)MFX_PICSTRUCT_PROGRESSIVE)
            , m_extFrameTag(0)
            , m_tid(0)
            , m_tidx(0)
            , m_panicMode(0)
        {
        }

        mfxU16 GetPicStructForEncode() const
        {
            return m_picStruct[ENC];
        }

        mfxU16 GetPicStructForDisplay() const
        {
            return m_picStruct[DISP];
        }

        mfxU8 GetFirstField() const
        {
            return (GetPicStructForEncode() & MFX_PICSTRUCT_FIELD_BFF) ? 1 : 0;
        }

        mfxI32 GetPoc(mfxU32 parity) const
        {
            return 2 * ((m_frameOrder - m_frameOrderIdr) & 0x7fffffff) + (parity != GetFirstField());
        }

        PairI32 GetPoc() const
        {
            return PairI32(GetPoc(TFIELD), GetPoc(BFIELD));
        }

        mfxU32 GetPictureCount() const
        {
            return (GetPicStructForEncode() & MFX_PICSTRUCT_PROGRESSIVE) ? 1 : 2;
        }

        mfxFrameSurface1* m_yuv;
        mfxU32  m_frameOrderIdrInDisplayOrder; // most recent idr frame in display order
        mfxU32  m_frameOrderIdr;    // most recent idr frame in encoding order
        mfxU32  m_frameOrderI;      // most recent i frame in encoding order
        mfxU32  m_frameOrder;
        mfxU32  m_baseLayerOrder;
        mfxU32  m_frameOrderStartTScalStructure; // starting point of temporal scalability structure
        mfxU16  m_frameNum;
        mfxI32  m_frameNumWrap;
        PairI32 m_picNum;
        mfxU8   m_longTermFrameIdx;
        PairU8  m_longTermPicNum;
        PairU8  m_reference;        // is refrence (short or long term) or not
        PairU16 m_picStruct;
        mfxU32  m_extFrameTag;
        mfxU32  m_tid;              // temporal_id
        mfxU8  m_tidx;             // temporal layer index (in acsending order of temporal_id)
        mfxU8   m_panicMode;
    };

    struct RefListMod
    {
        RefListMod() : m_idc(3), m_diff(0) {}
        RefListMod(mfxU16 idc, mfxU16 diff) : m_idc(idc), m_diff(diff) { assert(idc < 6); }
        mfxU16 m_idc;
        mfxU16 m_diff;
    };

    struct WeightTab
    {
        mfxU8 m_lumaWeightL0Flag:1;
        mfxU8 m_chromaWeightL0Flag:1;
        mfxU8 m_lumaWeightL1Flag:1;
        mfxU8 m_chromaWeightL1Flag:1;

        mfxI8 m_lumaWeightL0;
        mfxI8 m_lumaOffsetL0;
        mfxI8 m_lumaWeightL1;
        mfxI8 m_lumaOffsetL1;
        mfxI8 m_chromaWeightL0[2];
        mfxI8 m_chromaOffsetL0[2];
        mfxI8 m_chromaWeightL1[2];
        mfxI8 m_chromaOffsetL1[2];
    };

    struct DpbFrame
    {
        DpbFrame() { Zero(*this); }

        PairI32 m_poc;
        mfxU32  m_frameOrder;
        mfxU32  m_extFrameTag; // frameOrder assigned by application
        mfxU32  m_frameNum;
        mfxI32  m_frameNumWrap;
        mfxI32  m_picNum[2];
        mfxU32  m_viewIdx;
        mfxU32  m_frameIdx;
        mfxU32  m_tid;
        PairU8  m_longTermPicNum;
        PairU8  m_refPicFlag;
        mfxU8   m_longTermIdxPlus1;
        mfxU8   m_longterm; // at least one field is a long term reference
        mfxU8   m_refBase;

        mfxU8   m_PIFieldFlag; // for P/I field pair

        mfxMemId        m_midRec;
        CmSurface2D *   m_cmRaw;
        CmSurface2D *   m_cmRawLa;
        CmBufferUP *    m_cmMb;

        CmBufferUP *  m_cmHist;
        void *        m_cmHistSys;

        mfxMemId          m_midRaw; // for RefRaw mode
        mfxFrameSurface1* m_yuvRaw; // for RefRaw mode
    };

    inline bool operator ==(DpbFrame const & l, DpbFrame const & r)
    {
        return l.m_frameIdx == r.m_frameIdx && l.m_poc == r.m_poc && l.m_viewIdx == r.m_viewIdx;
    }

    typedef FixedArray<mfxU32, 16>     ArrayU32x16;
    typedef FixedArray<mfxU32, 64>     ArrayU32x64;
    typedef FixedArray<mfxU16, 16>     ArrayU16x16;
    typedef FixedArray<mfxU8, 8>       ArrayU8x8;
    typedef FixedArray<mfxU8, 16>      ArrayU8x16;
    typedef FixedArray<mfxU8, 32>      ArrayU8x32;
    typedef FixedArray<mfxU8, 33>      ArrayU8x33;
    typedef FixedArray<PairU32, 16>    ArrayPairU32x16;
    typedef FixedArray<RefListMod, 32> ArrayRefListMod;

    typedef FixedArray<mfxRoiDesc, 256>                               ArrayRoi;
    typedef FixedArray<mfxRectDesc, MFX_MAX_DIRTY_RECT_COUNT>         ArrayRect;
    typedef FixedArray<mfxMovingRectDesc, MFX_MAX_MOVE_RECT_COUNT>    ArrayMovingRect;

    struct ArrayDpbFrame : public FixedArray<DpbFrame, 16>
    {
        ArrayDpbFrame()
        : FixedArray<DpbFrame, 16>()
        {
            m_maxLongTermFrameIdxPlus1.Resize(8, 0);
        }

        ArrayU8x8 m_maxLongTermFrameIdxPlus1; // for each temporal layer
    };

    struct DecRefPicMarkingInfo
    {
        DecRefPicMarkingInfo() { Zero(*this); }

        void PushBack(mfxU8 op, mfxU32 param0, mfxU32 param1 = 0)
        {
            mmco.PushBack(op);
            value.PushBack(param0);
            value.PushBack(param1);
        }

        void Clear()
        {
            mmco.Resize(0);
            value.Resize(0);
            value.Resize(0);
        }

        mfxU8       no_output_of_prior_pics_flag;
        mfxU8       long_term_reference_flag;
        ArrayU8x32  mmco;       // memory management control operation id
        ArrayU32x64 value;      // operation-dependent data, max 2 per operation
    };

    struct DecRefPicMarkingRepeatInfo
    {
        DecRefPicMarkingRepeatInfo()
            : presentFlag(false)
            , original_idr_flag(0)
            , original_frame_num(0)
            , original_field_pic_flag(0)
            , original_bottom_field_flag(0)
            , dec_ref_pic_marking()
        {
        }

        bool                 presentFlag;
        mfxU8                original_idr_flag;
        mfxU16               original_frame_num;
        mfxU8                original_field_pic_flag;
        mfxU8                original_bottom_field_flag;
        DecRefPicMarkingInfo dec_ref_pic_marking;
    };
    struct SliceStructInfo
    {
        mfxU32 startMB;
        mfxU32 numMB;
        mfxF32 weight;
        mfxU32 cost;
    };


    class DdiTask : public Reconstruct
    {
    public:
        DdiTask()
            : Reconstruct()
            , m_pushed(0)
            , m_type(0, 0)
            , m_frcmplx(0)
            , m_dpb()
            , m_internalListCtrlPresent(false)
            , m_internalListCtrlHasPriority(true)
            , m_internalListCtrlRefModLTR(false)
            , m_initCpbRemoval(0)
            , m_initCpbRemovalOffset(0)
            , m_cpbRemoval(0, 0)
            , m_dpbOutputDelay(0)
            , m_encOrder(mfxU32(-1))
            , m_encOrderIdr(0)
            , m_encOrderI(0)

            , m_viewIdx(0)
            , m_idx(NO_INDEX)
            , m_idxBs(NO_INDEX, NO_INDEX)
            , m_idxBsOffset(0)
            , m_idxRecon(NO_INDEX)
            , m_idxInterLayerRecon(0)
            , m_idxReconOffset(0)
            , m_idrPicId(mfxU16(-1))
            , m_subMbPartitionAllowed(0, 0)
            , m_cqpValue(0, 0)
            , m_insertAud(0, 0)
            , m_insertSps(0, 0)
            , m_insertPps(0, 0)
            , m_AUStartsFromSlice(0, 0)
            , m_nalRefIdc(0, 0)
            , m_statusReportNumber(0, 0)

            , m_pid(0)
            , m_fillerSize(0, 0)
            , m_addRepackSize(0, 0)
            , m_maxIFrameSize(0)
            , m_maxPBFrameSize(0)
            , m_numMbPerSlice(0)
            , m_numSlice(0, 0)
            , m_numRoi(0)
#if MFX_VERSION > 1021
            , m_roiMode(MFX_ROI_MODE_PRIORITY)
#endif // #if MFX_VERSION > 1021
            , m_numDirtyRect(0)
            , m_numMovingRect(0)
            , m_did(0)
            , m_qid(0)
            , m_storeRefBasePicFlag(0)

            , m_bs(0)
            , m_bsDataLength(0, 0)
            , m_numLeadingFF(0, 0)
            , m_qpY(0, 0)
            , m_mad(0, 0)
            , m_minFrameSize (0)
            , m_notProtected(false)
            , m_nextLayerTask(0)
            , m_repack(0)
            , m_repackForBsDataLength(0)
            , m_fractionalQP(0)
            , m_midRaw(MID_INVALID)
            , m_midRec(MID_INVALID)
            , m_midBit(mfxMemId(MID_INVALID))
            , m_cmRaw(0)
            , m_cmRawLa(0)
            , m_cmMb(0)
            , m_cmMbSys(0)
            , m_cmRefMb(0)
            , m_cmCurbe(0)
            , m_cmRefs(0)
            , m_cmRefsLa(0)
            , m_event(0)
            , m_vmeData(0)
            , m_fwdRef(0)
            , m_bwdRef(0)
            , m_fieldPicFlag(0)
            , m_singleFieldMode(false)
            , m_fieldCounter(0)
            , m_timeStamp(0)
            , m_decodeTimeStamp(0)
            , m_minQP(0)
            , m_maxQP(0)
            , m_resetBRC(false)

            , m_idxMBQP(NO_INDEX)
            , m_midMBQP(MID_INVALID)
            , m_isMBQP(false)
            , m_isUseRawRef(false)

            , m_isMBControl(false)
            , m_midMBControl(MID_INVALID)
            , m_idxMBControl(NO_INDEX)

            , m_cmRawForHist(0)
            , m_cmHist(0)
            , m_cmHistSys(0)
            , m_isENCPAK(false)
            , m_startTime(0)
#ifdef MFX_ENABLE_MFE
            , m_beginTime(0)
            , m_endTime(0)
            , m_flushMfe(0)
            , m_mfeTimeToWait(0)
            , m_userTimeout(false)
#endif
            , m_hwType(MFX_HW_UNKNOWN)
            , m_SceneChange(0)
            , m_LowDelayPyramidLayer(0)
            , m_frameLtrOff(1)
            , m_frameLtrReassign(0)
            , m_LtrOrder(-1)
            , m_LtrQp(0)
            , m_RefOrder(-1)
            , m_RefQp(0)
            , m_idxScd(0)
            , m_wsSubSamplingEv(0)
            , m_wsSubSamplingTask(0)
            , m_wsGpuImage(0)
            , m_wsIdxGpuImage(0)
            , m_Yscd(0)
        {
            Zero(m_ctrl);
            Zero(m_internalListCtrl);
            Zero(m_handleRaw);
            Zero(m_fid);
            Zero(m_pwt);
            m_FrameName[0] = 0;
#ifndef MFX_AVC_ENCODING_UNIT_DISABLE
            m_collectUnitsInfo = false;
            m_headersCache[0].reserve(10);
            m_headersCache[1].reserve(10);
#endif
        }

        bool operator == (const DdiTask& task)
        {
            if(&task == this) return true;
            return false;
        }

        mfxU8 GetFrameType() const
        {
            return m_type[GetFirstField()];
        }

        // 0 - no skip, 1 - normal, 2 - pavp
        mfxU8 SkipFlag() const
        {
            if (    m_ctrl.SkipFrame == 0
                || (m_type.top & MFX_FRAMETYPE_I)
                || (m_type.bot & MFX_FRAMETYPE_I))
                return 0;
            return (mfxU8)m_ctrl.SkipFrame;
        }

        // Checking timeout for TDR hang detection
        bool CheckForTDRHang(mfxU32 curTime, mfxU32 timeout) const
        {
            MFX_TRACE_D(curTime);
            MFX_TRACE_D(m_startTime);
            MFX_TRACE_D(timeout);

            if (m_startTime && timeout && (curTime - m_startTime) > timeout)
            {
                MFX_TRACE_S("Possible TDR hang:");
                MFX_TRACE_D(((curTime - m_startTime) > timeout));
                return true;
            }

            return false;
        }

        mfxEncodeCtrl   m_ctrl;
        DdiTask *       m_pushed;         // task which was pushed to queue when this task was chosen for encoding
        Pair<mfxU8>     m_type;           // encoding type (one for each field)
        mfxU16          m_frcmplx;        // Frame complexity

        // all info about references
        // everything is in pair because task is a per-frame object
        Pair<ArrayDpbFrame>   m_dpb;
        ArrayDpbFrame         m_dpbPostEncoding; // dpb after encoding a frame (or 2 fields)
        Pair<ArrayU8x33>      m_list0;
        Pair<ArrayU8x33>      m_list1;
        Pair<ArrayRefListMod> m_refPicList0Mod;
        Pair<ArrayRefListMod> m_refPicList1Mod;
        Pair<mfxU32>          m_initSizeList0;
        Pair<mfxU32>          m_initSizeList1;

        // currently used for dpb control when svc temporal layers enabled
        mfxExtAVCRefListCtrl  m_internalListCtrl;
        bool                  m_internalListCtrlPresent;
        bool                  m_internalListCtrlHasPriority;
        bool                  m_internalListCtrlRefModLTR;

        mfxU32  m_initCpbRemoval;       // initial_cpb_removal_delay
        mfxU32  m_initCpbRemovalOffset; // initial_cpb_removal_delay_offset
        PairU32 m_cpbRemoval;           // one for each field
        mfxU32  m_dpbOutputDelay;       // one for entire frame
        mfxU32  m_encOrder;
        mfxU32  m_encOrderIdr;
        mfxU32  m_encOrderI;

        Pair<DecRefPicMarkingRepeatInfo> m_decRefPicMrkRep; // for sei_message() which repeat dec_ref_pic_marking() of previous frame
        Pair<DecRefPicMarkingInfo>       m_decRefPicMrk;    // dec_ref_pic_marking() for current frame

        mfxU32  m_viewIdx;
        mfxU32  m_idx;                  // index in chain of raw surfaces (set only when sysmem at input)
        PairU32 m_idxBs;                // index of bitstream surfaces, 2 - for interlaced frame (snb only)
        mfxU32  m_idxBsOffset;          // offset for multi-view coding
        mfxU32  m_idxRecon;             // index of reconstructed surface
        mfxU32  m_idxInterLayerRecon;   // index of reconstructed surface for inter-layer prediction
        mfxU32  m_idxReconOffset;       // offset for multi-view coding
        mfxU16  m_idrPicId;
        PairU8  m_subMbPartitionAllowed;
        PairU8  m_cqpValue;
        PairU8  m_insertAud;
        PairU8  m_insertSps;
        PairU8  m_insertPps;
        PairU8  m_AUStartsFromSlice;
        PairU8  m_nalRefIdc;
        PairU32 m_statusReportNumber;
        mfxU32  m_pid;                  // priority_id
        PairU32 m_fillerSize;
// MVC BD {
        PairU32 m_addRepackSize; // for SNB/IVB: size of padding to compensate re-pack of AVC headers to MVC headers
// MVC BD }
        mfxU32  m_maxIFrameSize;
        mfxU32  m_maxPBFrameSize;

        mfxU16  m_numMbPerSlice;
        PairU16 m_numSlice;

        ArrayRoi        m_roi;
        mfxU16          m_numRoi;
#if MFX_VERSION > 1021
        mfxU16          m_roiMode;
#endif // MFX_VERSION > 1021
        ArrayRect       m_dirtyRect;
        mfxU16          m_numDirtyRect;
        ArrayMovingRect m_movingRect;
        mfxU16          m_numMovingRect;

        mfxU32  m_did;                  // dependency_id
        mfxU32  m_qid;                  // quality_id
        mfxU32  m_storeRefBasePicFlag;  // for svc key picture

        mfxBitstream *    m_bs;           // output bitstream
        PairU32           m_bsDataLength; // bitstream size reported by driver (0 - progr/top, 1 - bottom)
        PairU32           m_numLeadingFF; // driver may insert 0xff in the beginning of coded frame
        PairU8            m_qpY;          // QpY reported by driver
        PairU32           m_mad;          // MAD
        BiFrameLocation   m_loc;
        IntraRefreshState m_IRState;
        mfxU32            m_minFrameSize;

        char   m_FrameName[32];

        bool m_notProtected;             // Driver returns not protected data
        DdiTask const * m_nextLayerTask; // set to 0 if no nextLayerResolutionChange
        mfxU32  m_repack;
        mfxU16  m_repackForBsDataLength;
        mfxI32  m_fractionalQP; //if m_fractionalQP > 0 set it value in QM matrices

        mfxMemId        m_midRaw;       // self-allocated input surface (when app gives input frames in system memory)
        mfxMemId        m_midRec;       // reconstruction
        Pair<mfxMemId>  m_midBit;       // output bitstream
        mfxHDLPair      m_handleRaw;    // native handle to raw surface (self-allocated or given by app)

        CmSurface2D *   m_cmRaw;        // CM surface made of m_handleRaw
        CmSurface2D *   m_cmRawLa;      // down-sized input surface for Lookahead
        CmBufferUP *    m_cmMb;         // macroblock data, VME kernel output
        void *          m_cmMbSys;      // pointer to associated system memory buffer
        CmBufferUP *    m_cmRefMb;      // macroblock data, VME kernel output for backward ref
        CmBuffer *      m_cmCurbe;      // control structure for ME & HME kernels
        SurfaceIndex *  m_cmRefs;       // VmeSurfaceG75 for ME kernel
        SurfaceIndex *  m_cmRefsLa;     // VmeSurfaceG75 for 2X kernel
        CmEvent *       m_event;
        VmeData *       m_vmeData;
        DdiTask const * m_fwdRef;
        DdiTask const * m_bwdRef;

        mfxU8   m_fieldPicFlag;    // true for frames with interlaced content
        bool    m_singleFieldMode; // true for FEI single-field processing mode

        // m_fid is a remapper of field parity to field order and vise versa.
        // i.e. parity = m_fid[fieldId] and fieldId = m_fid[parity] (fieldId == m_fid[m_fid[fieldId]]).
        // It is useful to switch between these two representation, because DdiTask stores all information
        // according to parity, but all of the per-field extension buffers are attached according to field order
        // (the only exception is mfxExtAVCRefLists).
        mfxU8   m_fid[2];               // progressive fid=[0,0]; tff fid=[0,1]; bff fid=[1,0]
        mfxU8   m_fieldCounter;
        mfxU64  m_timeStamp;
        mfxI64  m_decodeTimeStamp;

        mfxU8   m_minQP;
        mfxU8   m_maxQP;
        std::vector<mfxU8>   m_disableDeblockingIdc[2];
        std::vector<mfxI8>   m_sliceAlphaC0OffsetDiv2[2];
        std::vector<mfxI8>   m_sliceBetaOffsetDiv2[2];

        bool m_resetBRC;

        mfxU32   m_idxMBQP;
        mfxMemId m_midMBQP;
        bool     m_isMBQP;
        bool     m_isUseRawRef;

        bool     m_isMBControl;
        mfxMemId m_midMBControl;
        mfxU32   m_idxMBControl;

        CmSurface2D *         m_cmRawForHist;
        CmBufferUP *          m_cmHist;     // Histogram data, kernel output
        void *                m_cmHistSys;
        mfxExtPredWeightTable m_pwt[2];     // obtained from fade detection

        bool     m_isENCPAK;

        std::vector<void*> m_userData;
        std::vector<SliceStructInfo> m_SliceInfo;

        mfxU32 m_startTime;
#ifdef MFX_ENABLE_MFE
        vm_tick m_beginTime;//where we start counting
        vm_tick m_endTime;//where we get bitstream
        mfxU16  m_flushMfe;//flush MFE frame buffer
        mfxU32  m_mfeTimeToWait;//set by user or used equal to frame rate latency by default
        bool m_userTimeout;
#endif
        eMFXHWType m_hwType;  // keep HW type information


#ifndef MFX_AVC_ENCODING_UNIT_DISABLE
        bool m_collectUnitsInfo;
        mutable std::vector<mfxEncodedUnitInfo> m_headersCache[2]; //Headers for every field
#endif
        mfxU32 m_SceneChange;
        mfxU32 m_LowDelayPyramidLayer;
        mfxU32 m_frameLtrOff;
        mfxU32 m_frameLtrReassign;
        mfxI32 m_LtrOrder;
        mfxI32 m_LtrQp;
        mfxI32 m_RefOrder;
        mfxI32 m_RefQp;
        mfxU32         m_idxScd;
        CmEvent       *m_wsSubSamplingEv;
        CmTask        *m_wsSubSamplingTask;
        CmSurface2DUP *m_wsGpuImage;
        SurfaceIndex  *m_wsIdxGpuImage;
        mfxU8         *m_Yscd;
    };

    typedef std::list<DdiTask>::iterator DdiTaskIter;
    typedef std::list<DdiTask>::const_iterator DdiTaskCiter;


    template <size_t N>
    class Regression
    {
    public:
        static const mfxU32 MAX_WINDOW = N;

        Regression() {
            Zero(x);
            Zero(y);
        }
        void Reset(mfxU32 size, mfxF64 initX, mfxF64 initY) {
            windowSize = size;
            normX = initX;
            std::fill_n(x, windowSize, initX);
            std::fill_n(y, windowSize, initY);
            sumxx = initX * initX * windowSize;
            sumxy = initX * initY * windowSize;
        }
        void Add(mfxF64 newx, mfxF64 newy) {
            newy = newy / newx * normX;
            newx = normX;
            sumxy += newx * newy - x[0] * y[0];
            sumxx += newx * newx - x[0] * x[0];
            std::copy(x + 1, x + windowSize, x);
            std::copy(y + 1, y + windowSize, y);
            x[windowSize - 1] = newx;
            y[windowSize - 1] = newy;
        }
        mfxF64 GetCoeff() const {
            return sumxy / sumxx;
        }

    //protected:
    public: // temporary for debugging and dumping
        mfxF64 x[N];
        mfxF64 y[N];
        mfxU32 windowSize;
        mfxF64 normX;
        mfxF64 sumxy;
        mfxF64 sumxx;
    };
    struct BRCFrameParams: mfxBRCFrameParam
    {
        mfxU16 picStruct;
    };
    inline void InitFrameParams(BRCFrameParams &par, const DdiTask *task)
    {
        memset(&par,0,sizeof(par));
        par.FrameType = task->m_type[task->m_fid[0]];
        par.picStruct = task->GetPicStructForEncode();
        par.DisplayOrder = task->m_frameOrder;
        par.EncodedOrder = task->m_encOrder;
        par.PyramidLayer = (mfxU16)task->m_loc.level;
        par.NumRecode = (mfxU16)task->m_repack;
#if (MFX_VERSION >= 1026)
        par.FrameCmplx   = task->m_frcmplx;
        par.LongTerm     = (task->m_longTermFrameIdx != NO_INDEX_U8) ? 1 : 0;
        par.SceneChange  = (mfxU16) task->m_SceneChange;
        if (!par.PyramidLayer && (task->m_type[task->m_fid[0]] & MFX_FRAMETYPE_P) && task->m_LowDelayPyramidLayer)
            par.PyramidLayer = (mfxU16) task->m_LowDelayPyramidLayer;
#endif
    }

    class BrcIface
    {
    public:
        virtual ~BrcIface() {};
        virtual mfxStatus Init(MfxVideoParam  & video) = 0;
        virtual mfxStatus Reset(MfxVideoParam  & video) = 0;
        virtual void Close() = 0;
        virtual void PreEnc(const BRCFrameParams& par, std::vector<VmeData *> const & vmeData) = 0;
        virtual mfxU8 GetQp(const BRCFrameParams& par) = 0;
        virtual mfxU8 GetQpForRecode(const BRCFrameParams& par, mfxU8 curQP) = 0;
        virtual mfxF32 GetFractionalQp(const BRCFrameParams& par) = 0;
        virtual void SetQp(const BRCFrameParams& par, mfxU32 qp) = 0;
        virtual mfxU32 Report(const BRCFrameParams& par, mfxU32 dataLength, mfxU32 userDataLength, mfxU32 maxFrameSize, mfxU32 qp) = 0;
        virtual mfxU32 GetMinFrameSize() = 0;

        virtual mfxStatus SetFrameVMEData(const mfxExtLAFrameStatistics*, mfxU32 , mfxU32 ) {return MFX_ERR_NONE;}
    };

    BrcIface * CreateBrc(MfxVideoParam const & video);

    class Brc : public BrcIface
    {
    public:
        Brc(BrcIface * impl = 0)
        {
            m_impl.reset(impl);
        }
        void SetImpl(BrcIface * impl)
        {
            m_impl.reset(impl);
        }
        ~Brc()
        {
        }
        mfxStatus Init(MfxVideoParam  & video)
        {
            return m_impl->Init(video);
        }
        mfxStatus Reset(MfxVideoParam  & video)
        {
            return m_impl->Reset(video);
        }
        void Close()
        {
            m_impl->Close();
        }
        void PreEnc(const BRCFrameParams& par, std::vector<VmeData *> const & vmeData)
        {
            m_impl->PreEnc(par, vmeData);
        }
        mfxU8 GetQp(const BRCFrameParams& par)
        {
            return m_impl->GetQp(par);
        }
        mfxU8 GetQpForRecode(const BRCFrameParams& par, mfxU8 curQP)
        {
            return m_impl->GetQpForRecode(par, curQP);
        }
        mfxF32 GetFractionalQp(const BRCFrameParams& par)
        {
            return m_impl->GetFractionalQp(par);
        }
        void SetQp(const BRCFrameParams& par, mfxU32 qp)
        {
            m_impl->SetQp(par, qp);
        }
        mfxU32 Report(const BRCFrameParams& par, mfxU32 dataLength, mfxU32 userDataLength, mfxU32 maxFrameSize, mfxU32 qp)
        {
            return m_impl->Report(par, dataLength, userDataLength, maxFrameSize, qp);
        }
        mfxU32 GetMinFrameSize()
        {
            return m_impl->GetMinFrameSize();
        }
        mfxStatus SetFrameVMEData(const mfxExtLAFrameStatistics * pLAOutput, mfxU32 width, mfxU32 height)
        {
            return m_impl->SetFrameVMEData(pLAOutput,width,height);
        }
    private:
        std::unique_ptr<BrcIface> m_impl;
    };

    class UmcBrc : public BrcIface
    {
    public:
        ~UmcBrc() { Close(); }

        mfxStatus Init(MfxVideoParam  & video);
        mfxStatus Reset(MfxVideoParam  & ) { return MFX_ERR_NONE; };
        void Close();

        mfxU8 GetQp(const BRCFrameParams& par);
        mfxU8 GetQpForRecode(const BRCFrameParams& par, mfxU8 curQP);

        mfxF32 GetFractionalQp(const BRCFrameParams& par);

        void SetQp(const BRCFrameParams& par, mfxU32 qp);

        void PreEnc(const BRCFrameParams& par, std::vector<VmeData *> const & vmeData);

        mfxU32 Report(const BRCFrameParams& par, mfxU32 dataLength, mfxU32 userDataLength, mfxU32 maxFrameSize, mfxU32 qp);

        mfxU32 GetMinFrameSize();

    private:
        UMC::H264BRC m_impl;
        mfxU32 m_lookAhead;
    };



    class LookAheadBrc2 : public BrcIface
    {
    public:
        ~LookAheadBrc2() { Close(); }

        mfxStatus Init (MfxVideoParam  & video);
        mfxStatus Reset(MfxVideoParam  & ) { return MFX_ERR_NONE; };

        void Close();

        mfxU8 GetQp(const BRCFrameParams& par);
        mfxU8 GetQpForRecode(const BRCFrameParams& par, mfxU8 curQP);
        mfxF32 GetFractionalQp(const BRCFrameParams& ) { assert(0); return 26.0f; }


        void PreEnc(const BRCFrameParams& par, std::vector<VmeData *> const & vmeData);

        mfxU32 Report(const BRCFrameParams& par, mfxU32 dataLength, mfxU32 userDataLength, mfxU32 maxFrameSize, mfxU32 qp);

        mfxU32 GetMinFrameSize() { return 0; }
        void  SetQp(const BRCFrameParams& par, mfxU32 qp);


    public:
        struct LaFrameData
        {
            mfxU32  encOrder;
            mfxI32  poc;
            mfxI32  deltaQp;
            mfxF64  estRate[52];
            mfxF64  estRateTotal[52];
            mfxU32  interCost;
            mfxU32  intraCost;
            mfxU32  propCost;
            mfxU32  bframe;
        };

    protected:
        mfxU32  m_lookAhead;
        mfxU32  m_lookAheadDep;
        mfxU16  m_LaScaleFactor;
        mfxU32  m_strength;
        mfxU32  m_totNumMb;
        mfxF64  m_initTargetRate;
        mfxF64  m_targetRateMin;
        mfxF64  m_targetRateMax;
        mfxU32  m_framesBehind;
        mfxF64  m_bitsBehind;
        mfxI32  m_curBaseQp;
        mfxI32  m_curQp;
        mfxU16  m_qpUpdateRange;
        //mfxF32  m_coef;
        mfxF64  m_fr;
        mfxU16  m_AsyncDepth;
        mfxU16  m_first;
        mfxU16  m_skipped;
        mfxU8  m_QPMin[3]; // for I, P and B
        mfxU8  m_QPMax[3]; // for I, P and B

        bool        m_bControlMaxFrame;
        AVGBitrate* m_AvgBitrate;

        std::vector<LaFrameData>    m_laData;
        Regression<20>              m_rateCoeffHistory[52];
    };

    class VMEBrc : public BrcIface
    {
    public:
        ~VMEBrc() { Close(); }

        mfxStatus Init(MfxVideoParam  & video);
        mfxStatus Reset(MfxVideoParam  & ) { return MFX_ERR_NONE; }
        void Close();

        mfxU8 GetQp(const BRCFrameParams& par);
        mfxU8 GetQpForRecode(const BRCFrameParams& par, mfxU8 curQP);

        mfxF32 GetFractionalQp(const BRCFrameParams& ) { assert(0); return 26.0f; }

        void SetQp(const BRCFrameParams& , mfxU32 ) { assert(0); }

        void PreEnc(const BRCFrameParams& par, std::vector<VmeData *> const & vmeData);

        mfxU32 Report(const BRCFrameParams& par, mfxU32 dataLength, mfxU32 userDataLength,  mfxU32 maxFrameSize, mfxU32 qp);

        mfxU32 GetMinFrameSize() { return 0; }

        mfxStatus SetFrameVMEData(const mfxExtLAFrameStatistics *, mfxU32 widthMB, mfxU32 heightMB );


    public:
        struct LaFrameData
        {
            mfxU32  encOrder;
            mfxU32  dispOrder;
            mfxI32  poc;
            mfxI32  deltaQp;
            mfxF64  estRate[52];
            mfxF64  estRateTotal[52];
            mfxU32  interCost;
            mfxU32  intraCost;
            mfxU32  propCost;
            mfxU32  bframe;
            bool    bNotUsed;
        };

    protected:
        mfxU32  m_lookAhead;
        mfxU16  m_LaScaleFactor;
        mfxU32  m_strength;
        mfxU32  m_totNumMb;
        mfxF64  m_initTargetRate;
        mfxF64  m_targetRateMin;
        mfxF64  m_targetRateMax;
        mfxU32  m_framesBehind;
        mfxF64  m_bitsBehind;
        mfxI32  m_curBaseQp;
        mfxI32  m_curQp;
        mfxU16  m_qpUpdateRange;
        mfxF64  m_fr;
        mfxU16  m_skipped;
        mfxU8   m_QPMin[3]; // for I, P and B
        mfxU8   m_QPMax[3]; // for I, P and B

        AVGBitrate* m_AvgBitrate;

        std::list <LaFrameData>     m_laData;
        Regression<20>              m_rateCoeffHistory[52];
    };

    class LookAheadCrfBrc : public BrcIface
    {
    public:
        ~LookAheadCrfBrc() { Close(); }

        mfxStatus Init(MfxVideoParam  & video);
        mfxStatus Reset(MfxVideoParam  & ) {return MFX_ERR_NONE;};

        void Close() {}

        mfxU8 GetQp(const BRCFrameParams& par);
        mfxU8 GetQpForRecode(const BRCFrameParams& par, mfxU8 /*curQP */);
        mfxF32 GetFractionalQp(const BRCFrameParams& /*par*/) { assert(0); return 26.0f; }

        void SetQp(const BRCFrameParams& /*par*/, mfxU32 /*qp*/) { assert(0); }

        void PreEnc(const BRCFrameParams& par, std::vector<VmeData *> const & vmeData);

        mfxU32 Report(const BRCFrameParams& par, mfxU32 dataLength, mfxU32 userDataLength, mfxU32 maxFrameSize, mfxU32 qp);

        mfxU32 GetMinFrameSize() { return 0; }

    protected:
        mfxU32  m_lookAhead;
        mfxI32  m_crfQuality;
        mfxI32  m_curQp;
        mfxU32  m_totNumMb;
        mfxU32  m_intraCost;
        mfxU32  m_interCost;
        mfxU32  m_propCost;
        mfxU8   m_QPMin[3]; // for I, P and B
        mfxU8   m_QPMax[3]; // for I, P and B
    };
    class H264SWBRC : public BrcIface
    {
    public:
        H264SWBRC():
            m_minSize(0),
            m_pBRC(0)
        {
            memset(&m_BRCLocal,0, sizeof(m_BRCLocal));
        }    
        virtual ~H264SWBRC()
        {
            Close();
        }


        mfxStatus   Init(MfxVideoParam &video)
        {
            mfxStatus sts = MFX_ERR_NONE;
            mfxExtBRC * extBRC = GetExtBuffer(video);
            

            if (extBRC->pthis)
            {
                m_pBRC = extBRC;
            }
            else
            {
                sts = HEVCExtBRC::Create(m_BRCLocal);
                m_pBRC = &m_BRCLocal;
            }
            sts = m_pBRC->Init(m_pBRC->pthis, &video);
            return sts;
        }
        void  Close()
        {
            m_pBRC->Close(m_pBRC->pthis);
            HEVCExtBRC::Destroy(m_BRCLocal);
        }
        mfxStatus  Reset(MfxVideoParam &video )
        {
            return m_pBRC->Reset(m_pBRC->pthis,&video);    
        }
        mfxU32 Report(const BRCFrameParams& par, mfxU32 dataLength, mfxU32 /*userDataLength*/, mfxU32 /*maxFrameSize*/, mfxU32 qp)
        {
            mfxBRCFrameParam frame_par={};
            mfxBRCFrameCtrl  frame_ctrl={};
            mfxBRCFrameStatus frame_sts = {};
        
            frame_par  = *((mfxBRCFrameParam *)&par);
            frame_ctrl.QpY = qp;
            frame_par.CodedFrameSize = dataLength;  // Size of frame in bytes after encoding


            mfxStatus sts = m_pBRC->Update(m_pBRC->pthis,&frame_par, &frame_ctrl, &frame_sts);
            MFX_CHECK(sts == MFX_ERR_NONE, (mfxU32)UMC::BRC_ERROR); // BRC_ERROR

            m_minSize = frame_sts.MinFrameSize;

            switch (frame_sts.BRCStatus)
            {
            case ::MFX_BRC_OK:
                return  UMC::BRC_OK;
            case ::MFX_BRC_BIG_FRAME: 
                return UMC::BRC_ERR_BIG_FRAME;
            case ::MFX_BRC_SMALL_FRAME: 
                return UMC::BRC_ERR_SMALL_FRAME;
            case ::MFX_BRC_PANIC_BIG_FRAME: 
                return UMC::BRC_ERR_BIG_FRAME |  UMC::BRC_NOT_ENOUGH_BUFFER;
            case ::MFX_BRC_PANIC_SMALL_FRAME:  
                return UMC::BRC_ERR_SMALL_FRAME| UMC::BRC_NOT_ENOUGH_BUFFER;
            }
            return MFX_BRC_OK;    
        }
        mfxU8  GetQp(const BRCFrameParams& par)
        {
            mfxBRCFrameParam frame_par={};
            mfxBRCFrameCtrl  frame_ctrl={};

            frame_par = *((mfxBRCFrameParam*)&par);
            m_pBRC->GetFrameCtrl(m_pBRC->pthis,&frame_par, &frame_ctrl);
            return (mfxU8)CLIPVAL(1, 51, frame_ctrl.QpY);
        }
        mfxU8 GetQpForRecode(const BRCFrameParams& par, mfxU8 /*curQP */)
        {
            return GetQp(par);
        }
        
        void   SetQp(const BRCFrameParams& /*par*/,  mfxU32 /*qp*/)
        {
        }
        mfxU32 GetMinFrameSize()
        {
            return m_minSize;
    
        }
        mfxF32 GetFractionalQp(const BRCFrameParams& /*par*/) { assert(0); return 26.0f; }

        virtual void        PreEnc(const BRCFrameParams& /*par*/, std::vector<VmeData *> const & /* vmeData */) {}
        virtual mfxStatus SetFrameVMEData(const mfxExtLAFrameStatistics*, mfxU32 , mfxU32 )
        {
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        }
        virtual bool IsVMEBRC()  {return false;}

    private:
        mfxU32      m_minSize;
        mfxExtBRC * m_pBRC;
        mfxExtBRC   m_BRCLocal;

};
    class Hrd
    {
    public:
        Hrd();

        void Setup(MfxVideoParam const & par);

        void Reset(MfxVideoParam const & par);

        void RemoveAccessUnit(
            mfxU32 size,
            mfxU32 interlace,
            mfxU32 bufferingPeriod);

        mfxU32 GetInitCpbRemovalDelay() const;

        mfxU32 GetInitCpbRemovalDelayOffset() const;
        mfxU32 GetMaxFrameSize(mfxU32 bufferingPeriod) const;

    private:
        mfxU32 m_bitrate;
        mfxU32 m_rcMethod;
        mfxU32 m_hrdIn90k;  // size of hrd buffer in 90kHz units

        double m_tick;      // clock tick
        double m_trn_cur;   // nominal removal time
        double m_taf_prv;   // final arrival time of prev unit

        bool m_bIsHrdRequired;
    };

    class DdiTask2ndField
    {
    public:
        DdiTask * m_1stFieldTask;
        DdiTask   m_2ndFieldTask;
    };

    // should be called from one thread
    // yields tasks in cyclic manner
    class CyclicTaskPool
    {
    public:
        void Init(mfxU32 size);

        DdiTask2ndField * GetFreeTask();

    private:
        std::vector<DdiTask2ndField>           m_pool;
        std::vector<DdiTask2ndField>::iterator m_next;
    };

    struct MbData
    {
        mfxU32      intraCost;
        mfxU32      interCost;
        mfxU32      propCost;
        mfxU8       w0;
        mfxU8       w1;
        mfxU16      dist;
        mfxU16      rate;
        mfxU16      lumaCoeffSum[4];
        mfxU8       lumaCoeffCnt[4];
        mfxI16Pair  costCenter0;
        mfxI16Pair  costCenter1;
        struct
        {
            mfxU32  intraMbFlag     : 1;
            mfxU32  skipMbFlag      : 1;
            mfxU32  mbType          : 5;
            mfxU32  reserved0       : 1;
            mfxU32  subMbShape      : 8;
            mfxU32  subMbPredMode   : 8;
            mfxU32  reserved1       : 8;
        };
        mfxI16Pair  mv[2]; // in sig-sag scan
    };

    class CmContext;

    struct VmeData
    {
        VmeData()
            : used(false)
            , poc(mfxU32(-1))
            , pocL0(mfxU32(-1))
            , pocL1(mfxU32(-1))
            , intraCost(0)
            , interCost(0)
            , propCost(0) { }

        bool                used;
        mfxU32              poc;
        mfxU32              pocL0;
        mfxU32              pocL1;
        mfxU32              encOrder;
        mfxU32              intraCost;
        mfxU32              interCost;
        mfxU32              propCost;
        std::vector<MbData> mb;
    };


    class AsyncRoutineEmulator
    {
    public:
        enum {
            STG_ACCEPT_FRAME,
            STG_START_SCD,
            STG_WAIT_SCD,
            STG_START_LA,
            STG_WAIT_LA,
            STG_START_HIST,
            STG_WAIT_HIST,
            STG_START_ENCODE,
            STG_WAIT_ENCODE,
            STG_COUNT
        };

        enum {
            STG_BIT_CALL_EMULATOR = 0,
            STG_BIT_ACCEPT_FRAME  = 1 << STG_ACCEPT_FRAME,
            STG_BIT_START_SCD     = 1 << STG_START_SCD,
            STG_BIT_WAIT_SCD      = 1 << STG_WAIT_SCD,
            STG_BIT_START_LA      = 1 << STG_START_LA,
            STG_BIT_WAIT_LA       = 1 << STG_WAIT_LA,
            STG_BIT_START_HIST    = 1 << STG_START_HIST,
            STG_BIT_WAIT_HIST     = 1 << STG_WAIT_HIST,
            STG_BIT_START_ENCODE  = 1 << STG_START_ENCODE,
            STG_BIT_WAIT_ENCODE   = 1 << STG_WAIT_ENCODE,
            STG_BIT_RESTART       = 1 << STG_COUNT
        };

        AsyncRoutineEmulator();

        AsyncRoutineEmulator(MfxVideoParam const & video);

        void Init(MfxVideoParam const & video);

        mfxU32 GetTotalGreediness() const;

        mfxU32 GetStageGreediness(mfxU32 i) const;

        mfxU32 Go(bool hasInput);

    protected:
        mfxU32 CheckStageOutput(mfxU32 stage);

    private:
        mfxU32 m_stageGreediness[STG_COUNT];
        mfxU32 m_queueFullness[STG_COUNT + 1];
        mfxU32 m_queueFlush[STG_COUNT + 1];
    };

    struct SVCPAKObject;

    using ns_asc::ASC;

    class ImplementationAvc : public VideoENCODE
    {
    public:
        struct FrameTypeAdapt
        {
            FrameTypeAdapt(CmDevice* cmDevice, int width, int height)
                : m_frameNum(0)
                , m_isAdapted(0)
                , m_surface(0)
                , m_cmDevice(cmDevice)

                , intraCost(0)
                , interCost(0)
                , intraDist(0)
                , interDist(0)
                , totalDist(0)
                , numIntraMb(0)
            {
                int numMB = width*height/256;
                mb.resize(numMB);

                m_surface4X.Reset(m_cmDevice, width/4, height/4, CM_SURFACE_FORMAT_NV12);
            }

            mfxU32              m_frameNum;
            mfxU32              m_isAdapted;
            mfxU16              m_frameType;
            mfxEncodeCtrl*      m_ctrl;
            mfxFrameSurface1*   m_surface;
            CmSurface           m_surface4X;
            CmDevice*           m_cmDevice;

            mfxU32              intraCost;
            mfxU32              interCost;
            mfxU32              intraDist;
            mfxU32              interDist;
            mfxU32              totalDist;
            mfxU32              numIntraMb;
            std::vector<MbData> mb;
            //std::vector<SVCPAKObject> m_mb;
        };

    public:
        static mfxStatus Query(
            VideoCORE *     core,
            mfxVideoParam * in,
            mfxVideoParam * out,
            void          * state = 0);

        static mfxStatus QueryIOSurf(
            VideoCORE *            core,
            mfxVideoParam *        par,
            mfxFrameAllocRequest * request);

        ImplementationAvc(VideoCORE * core);

        virtual ~ImplementationAvc();

        virtual mfxStatus Init(mfxVideoParam * par);

        virtual mfxStatus Close() { return MFX_ERR_NONE; }

        virtual mfxStatus Reset(mfxVideoParam * par);

        virtual mfxStatus GetVideoParam(mfxVideoParam * par);

        virtual mfxStatus GetFrameParam(mfxFrameParam * par);

        virtual mfxStatus GetEncodeStat(mfxEncodeStat * stat);

        virtual mfxStatus EncodeFrameCheck(
            mfxEncodeCtrl *,
            mfxFrameSurface1 *,
            mfxBitstream *,
            mfxFrameSurface1 **,
            mfxEncodeInternalParams *)
        {
            return MFX_ERR_UNSUPPORTED;
        }

        virtual mfxStatus EncodeFrameCheck(
            mfxEncodeCtrl *           ctrl,
            mfxFrameSurface1 *        surface,
            mfxBitstream *            bs,
            mfxFrameSurface1 **       reordered_surface,
            mfxEncodeInternalParams * internalParams,
            MFX_ENTRY_POINT *         entryPoints,
            mfxU32 &                  numEntryPoints);

        virtual mfxStatus EncodeFrameCheckNormalWay(
            mfxEncodeCtrl *           ctrl,
            mfxFrameSurface1 *        surface,
            mfxBitstream *            bs,
            mfxFrameSurface1 **       reordered_surface,
            mfxEncodeInternalParams * internalParams,
            MFX_ENTRY_POINT *         entryPoints,
            mfxU32 &                  numEntryPoints);

        virtual mfxStatus EncodeFrame(
            mfxEncodeCtrl *,
            mfxEncodeInternalParams *,
            mfxFrameSurface1 *,
            mfxBitstream *)
        {
            return MFX_ERR_UNSUPPORTED;
        }

        virtual mfxStatus CancelFrame(
            mfxEncodeCtrl *,
            mfxEncodeInternalParams *,
            mfxFrameSurface1 *,
            mfxBitstream *)
        {
            return MFX_ERR_UNSUPPORTED;
        }

    protected:
        ASC       amtScd;
        mfxStatus SCD_Put_Frame(
            DdiTask & newTask);
        mfxStatus SCD_Get_FrameType(
            DdiTask & newTask);
        mfxStatus CalculateFrameCmplx(
            DdiTask const &task,
            mfxU16 &raca128);
        mfxStatus Prd_LTR_Operation(
            DdiTask & task);
        void      AssignFrameTypes(
            DdiTask & newTask);
        void      AssignDecodeTimeStamp(
            DdiTask & task);

        mfxStatus UpdateBitstream(
            DdiTask & task,
            mfxU32    fid); // 0 - top/progressive, 1 - bottom

#ifndef MFX_AVC_ENCODING_UNIT_DISABLE
        void FillEncodingUnitsInfo(
            DdiTask &task,
            mfxU8 *sbegin,
            mfxU8 *send,
            mfxExtEncodedUnitsInfo *encUnitsInfo,
            mfxU32 fid);
#endif

        mfxStatus AsyncRoutine(
            mfxBitstream * bs);

        void OnNewFrame();
        void SubmitScd();
        void OnScdQueried();
        void OnScdFinished();


        void OnLookaheadSubmitted(DdiTaskIter task);

        void OnLookaheadQueried();

        void OnHistogramSubmitted();

        void OnHistogramQueried();

        void OnEncodingSubmitted(DdiTaskIter task);

        void OnEncodingQueried(DdiTaskIter task);

        void BrcPreEnc(DdiTask const & task);

        static mfxStatus AsyncRoutineHelper(
            void * state,
            void * param,
            mfxU32 threadNumber,
            mfxU32 callNumber);

        void RunPreMe(
            MfxVideoParam const & video,
            DdiTask const &       task);

        void SubmitLookahead(
            DdiTask & task);

        mfxStatus QueryLookahead(
            DdiTask & task);

        mfxStatus QueryStatus(
            DdiTask & task,
            mfxU32    ffid);


        mfxStatus AdaptiveGOP(
            mfxEncodeCtrl**           ctrl,
            mfxFrameSurface1**        surface,
            mfxU16* requiredFrameType);

        mfxStatus AdaptiveGOP1(
            mfxEncodeCtrl**           ctrl,
            mfxFrameSurface1**        surface,
            mfxU16* requiredFrameType);

        mfxStatus ProcessAndCheckNewParameters(
            MfxVideoParam & newPar,
            bool & isBRCReset,
            bool & isIdrRequired,
            mfxVideoParam const * newParIn = 0);

        void DestroyDanglingCmResources();

        VideoCORE *         m_core;
        CmDevicePtr         m_cmDevice;
        MfxVideoParam       m_video;
        MfxVideoParam       m_videoInit;  // m_video may change by Reset, m_videoInit doesn't change
        mfxEncodeStat       m_stat;

        std::list<std::pair<mfxBitstream *, mfxU32> > m_listOfPairsForFieldOutputMode;

        AsyncRoutineEmulator m_emulatorForSyncPart;
        AsyncRoutineEmulator m_emulatorForAsyncPart;

        SliceDivider        m_sliceDivider;

        std::list<DdiTask>  m_free;
        std::list<DdiTask>  m_incoming;
        std::list<DdiTask>  m_ScDetectionStarted;
        std::list<DdiTask>  m_ScDetectionFinished;
        std::list<DdiTask>  m_reordering;
        std::list<DdiTask>  m_lookaheadStarted;
        std::list<DdiTask>  m_lookaheadFinished;
        std::list<DdiTask>  m_histRun;
        std::list<DdiTask>  m_histWait;
        std::list<DdiTask>  m_encoding;
        std::list<mfxU64>   m_timeStamps;
        UMC::Mutex          m_listMutex;
        DdiTask             m_lastTask;
        mfxU32              m_stagesToGo;

        mfxU32      m_fieldCounter;
        mfxStatus   m_1stFieldStatus;
        mfxU32      m_frameOrder;
        mfxU32      m_baseLayerOrder;
        mfxU32      m_frameOrderIdrInDisplayOrder;    // frame order of last IDR frame (in display order)
        mfxU32      m_frameOrderIntraInDisplayOrder;  // frame order of last I frame (in display order)
        mfxU32      m_frameOrderStartTScalStructure; // starting point of temporal scalability structure

        // parameters for Intra refresh
        mfxI64      m_baseLayerOrderStartIntraRefresh; // starting point of Intra refresh cycles (could be negative)
        mfxU16      m_intraStripeWidthInMBs; // width of Intra MB stripe (column or row depending on refresh type)

        mfxU32      m_enabledSwBrc;
        Brc         m_brc;
        Hrd         m_hrd;
        mfxU32      m_maxBsSize;

        std::unique_ptr<DriverEncoder>    m_ddi;

        std::vector<mfxU32>     m_recFrameOrder;

        mfxU32 m_recNonRef[2];

        MfxFrameAllocResponse   m_scd;
        MfxFrameAllocResponse   m_raw;
        MfxFrameAllocResponse   m_rawSkip;
        MfxFrameAllocResponse   m_rawLa;
        MfxFrameAllocResponse   m_mb;
        MfxFrameAllocResponse   m_curbe;
        MfxFrameAllocResponse   m_rawSys;
        MfxFrameAllocResponse   m_rec;
        MfxFrameAllocResponse   m_bit;
        MfxFrameAllocResponse   m_opaqResponse;     // Response for opaq
        MfxFrameAllocResponse   m_histogram;

        ENCODE_CAPS             m_caps;
        mfxStatus               m_failedStatus;
        mfxU32                  m_inputFrameType;
        mfxU32                  m_NumSlices;

        MfxFrameAllocResponse   m_mbqp;
        bool                    m_useMBQPSurf;

        MfxFrameAllocResponse   m_mbControl;
        bool                    m_useMbControlSurfs;

        std::vector<mfxU8>  m_tmpBsBuf;
        PreAllocatedVector  m_sei;

        eMFXHWType  m_currentPlatform;
        eMFXVAType  m_currentVaType;
        bool        m_useWAForHighBitrates;
        bool        m_isENCPAK;
        bool        m_resetBRC;

        // bitrate reset for SNB

        std::unique_ptr<CmContext>    m_cmCtx;
        std::vector<VmeData>        m_vmeDataStorage;
        std::vector<VmeData *>      m_tmpVmeData;


        mfxU32      m_LowDelayPyramidLayer;
        mfxI32      m_LtrQp;
        mfxI32      m_LtrOrder;
        mfxI32      m_RefQp;
        mfxI32      m_RefOrder;

    };


    struct NalUnit
    {
        NalUnit() : begin(0), end(0), type(0), numZero(0)
        {}

        NalUnit(mfxU8 * b, mfxU8 * e, mfxU8 t, mfxU8 z) : begin(b), end(e), type(t), numZero(z)
        {}

        mfxU8 * begin;
        mfxU8 * end;
        mfxU8   type;
        mfxU32  numZero;
    };

    NalUnit GetNalUnit(mfxU8 * begin, mfxU8 * end);

    class NaluIterator
    {
    public:
        NaluIterator()
            : m_begin(0)
            , m_end(0)
        {}

        NaluIterator(mfxU8 * begin, mfxU8 * end)
            : m_nalu(GetNalUnit(begin, end))
            , m_begin(m_nalu.end)
            , m_end(end)
        {
        }

        NaluIterator(NalUnit const & nalu, mfxU8 * end)
            : m_nalu(nalu)
            , m_begin(nalu.end)
            , m_end(end)
        {
        }

        NalUnit & operator *()
        {
            return m_nalu;
        }

        NalUnit * operator ->()
        {
            return &m_nalu;
        }

        NaluIterator & operator++()
        {
            m_nalu = GetNalUnit(m_begin, m_end);
            m_begin = m_nalu.end;
            return *this;
        }

        NaluIterator operator++(int)
        {
            NaluIterator tmp;
            ++*this;
            return tmp;
        }

        bool operator ==(NaluIterator const & right) const
        {
            return m_nalu.begin == right.m_nalu.begin && m_nalu.end == right.m_nalu.end;
        }

        bool operator !=(NaluIterator const & right) const
        {
            return !(*this == right);
        }

    private:
        NalUnit m_nalu;
        mfxU8 * m_begin;
        mfxU8 * m_end;
    };

    struct BitstreamDesc
    {
        BitstreamDesc() : begin(0), end(0)
        {}

        mfxU8 * begin;
        mfxU8 * end;
        NalUnit aud;   // byte range within [begin, end)
        NalUnit sps;   // byte range within [begin, end)
        NalUnit pps;   // byte range within [begin, end)
        NalUnit sei;   // first sei if multi sei nal units present
        NalUnit slice; // first slice if multi-sliced
    };


    class InputBitstream
    {
    public:
        InputBitstream(
            mfxU8 const * buf,
            size_t        size,
            bool          hasStartCode = true,
            bool          doEmulationControl = true);

        InputBitstream(
            mfxU8 const * buf,
            mfxU8 const * bufEnd,
            bool          hasStartCode = true,
            bool          doEmulationControl = true);

        mfxU32 NumBitsRead() const;
        mfxU32 NumBitsLeft() const;

        mfxU32 GetBit();
        mfxU32 GetBits(mfxU32 nbits);
        mfxU32 GetUe();
        mfxI32 GetSe();

    private:
        mfxU8 const * m_buf;
        mfxU8 const * m_ptr;
        mfxU8 const * m_bufEnd;
        mfxU32        m_bitOff;
        bool          m_doEmulationControl;
    };

    class OutputBitstream
    {
    public:
        OutputBitstream(mfxU8 * buf, size_t size, bool emulationControl = true);
        OutputBitstream(mfxU8 * buf, mfxU8 * bufEnd, bool emulationControl = true);

        mfxU32 GetNumBits() const;

        void PutBit(mfxU32 bit);
        void PutBits(mfxU32 val, mfxU32 nbits);
        void PutUe(mfxU32 val);
        void PutSe(mfxI32 val);
        void PutRawBytes(mfxU8 const * begin, mfxU8 const * end); // startcode emulation is not controlled
        void PutFillerBytes(mfxU8 filler, mfxU32 nbytes);         // startcode emulation is not controlled
        void PutTrailingBits();

    private:
        mfxU8 * m_buf;
        mfxU8 * m_ptr;
        mfxU8 * m_bufEnd;
        mfxU32  m_bitOff;
        bool    m_emulationControl;
    };

    class CabacPackerSimple : public OutputBitstream
    {
    public:
        CabacPackerSimple(mfxU8 * buf, mfxU8 * bufEnd, bool emulationControl = true);
        void EncodeBin(mfxU8  * ctx, mfxU8 bin);
        void TerminateEncode();
    private:
        void PutBitC(mfxU32 B);
        void RenormE();

        mfxU32 m_codILow;
        mfxU32 m_codIRange;
        mfxU32 m_bitsOutstanding;
        mfxU32 m_BinCountsInNALunits;
        bool   m_firstBitFlag;
    };

    void PutSeiHeader(
        OutputBitstream & bs,
        mfxU32            payloadType,
        mfxU32            payloadSize);

    void PutSeiMessage(
        OutputBitstream &                   bs,
        mfxExtAvcSeiBufferingPeriod const & msg);

    void PutSeiMessage(
        OutputBitstream &              bs,
        mfxExtPictureTimingSEI const & extPt,
        mfxExtAvcSeiPicTiming const &  msg);

    void PutSeiMessage(
        OutputBitstream &                   bs,
        mfxExtAvcSeiDecRefPicMrkRep const & msg);

    void PutSeiMessage(
        OutputBitstream &    bs,
        mfxExtAvcSeiRecPoint const & msg);

    mfxU32 PutScalableInfoSeiMessage(
        OutputBitstream &     obs,
        MfxVideoParam const & par);

// MVC BD {
    // Put MVC scalable nested SEI
    void PutSeiMessage(
        OutputBitstream &                   bs,
        mfxU32 needBufferingPeriod,
        mfxU32 needPicTimingSei,
        mfxU32 fillerSize,
        MfxVideoParam const & video,
        mfxExtAvcSeiBufferingPeriod const & msg_bp,
        mfxExtPictureTimingSEI const & extPt,
        mfxExtAvcSeiPicTiming const &  msg_pt);
// MVC BD }



    mfxU8 const * SkipStartCode(mfxU8 const * begin, mfxU8 const * end);
    mfxU8 *       SkipStartCode(mfxU8 *       begin, mfxU8 *       end);

    ArrayRefListMod CreateRefListMod(
        ArrayDpbFrame const &            dpb,
        std::vector<Reconstruct> const & recons,
        ArrayU8x33                       initList,
        ArrayU8x33 const &               modList,
        mfxU32                           curViewIdx,
        mfxI32                           curPicNum,
        bool                             optimize = true);

    mfxU8 * CheckedMFX_INTERNAL_CPY(
        mfxU8 *       dbegin,
        mfxU8 *       dend,
        mfxU8 const * sbegin,
        mfxU8 const * send);

    mfxU8 * CheckedMemset(
        mfxU8 * dbegin,
        mfxU8 * dend,
        mfxU8   value,
        mfxU32  size);

    void ReadRefPicListModification(InputBitstream & reader);

    void ReadDecRefPicMarking(
        InputBitstream & reader,
        bool             idrPicFlag);

    void WriteRefPicListModification(
        OutputBitstream &       writer,
        ArrayRefListMod const & refListMod);

    void WriteDecRefPicMarking(
        OutputBitstream &            writer,
        DecRefPicMarkingInfo const & marking,
        bool                         idrPicFlag);

    mfxU8 * RePackSlice(
        mfxU8 *               dbegin,
        mfxU8 *               dend,
        mfxU8 *               sbegin,
        mfxU8 *               send,
        MfxVideoParam const & par,
        DdiTask const &       task,
        mfxU32                fieldId);

    enum
    {
        RPLM_ST_PICNUM_SUB  = 0,
        RPLM_ST_PICNUM_ADD  = 1,
        RPLM_LT_PICNUM      = 2,
        RPLM_END            = 3,
        RPLM_INTERVIEW_SUB  = 4,
        RPLM_INTERVIEW_ADD  = 5,
    };

    enum
    {
        MMCO_END            = 0,
        MMCO_ST_TO_UNUSED   = 1,
        MMCO_LT_TO_UNUSED   = 2,
        MMCO_ST_TO_LT       = 3,
        MMCO_SET_MAX_LT_IDX = 4,
        MMCO_ALL_TO_UNUSED  = 5,
        MMCO_CURR_TO_LT     = 6,
    };


    void PrepareSeiMessageBuffer(
        MfxVideoParam const & video,
        DdiTask const &       task,
        mfxU32                fieldId, // 0 - top/progressive, 1 - bottom
        PreAllocatedVector &  sei);

    void PrepareSeiMessageBufferDepView(
        MfxVideoParam const & video,
        DdiTask const &       task,
        mfxU32                fieldId, // 0 - top/progressive, 1 - bottom
        PreAllocatedVector &  sei);


    bool IsInplacePatchNeeded(
        MfxVideoParam const & par,
        DdiTask const &       task,
        mfxU32                fieldId);

    bool IsSlicePatchNeeded(
        DdiTask const & task,
        mfxU32          fieldId);

    mfxStatus  CopyBitstream(
                VideoCORE           & core,
                MfxVideoParam const & video,
                DdiTask const       & task,
                mfxU32              fieldId,
                mfxU8 *             bsData,
                mfxU32              bsSizeAvail);

    mfxU32 GetMaxSliceSize(
        mfxU8 *               sbegin, // contents of source buffer may be modified
        mfxU8 *               send,
        mfxU32                &num);


    mfxStatus UpdateSliceInfo(
        mfxU8 *               sbegin, // contents of source buffer may be modified
        mfxU8 *               send,
        mfxU32                maxSliceSize,
        DdiTask&              task,
        bool&                 bRecoding);

    mfxU8 * PatchBitstream(
        MfxVideoParam const & video,
        DdiTask const &       task,
        mfxU32                fieldId,
        mfxU8 *               sbegin, // contents of source buffer may be modified
        mfxU8 *               send,
        mfxU8 *               dbegin,
        mfxU8 *               dend);
    mfxU8 * InsertSVCNAL(
        DdiTask const &       task,
        mfxU32                fieldId,
        mfxU8 *               sbegin, // contents of source buffer may be modified
        mfxU8 *               send,
        mfxU8 *               dbegin,
        mfxU8 *               dend);

    mfxU8 * AddEmulationPreventionAndCopy(
        mfxU8 *               sbegin,
        mfxU8 *               send,
        mfxU8 *               dbegin,
        mfxU8 *               dend);

    mfxStatus FillSliceInfo(
        DdiTask &           task,
        mfxU32              MaxSliceSize,
        mfxU32              FrameSize,
        mfxU32              widthLa,
        mfxU32              heightLa);

    mfxStatus CorrectSliceInfo(
        DdiTask &           task,
        mfxU32              sliceWeight,
        mfxU32              widthLa,
        mfxU32              heightLa);

    mfxStatus CorrectSliceInfoForsed(
        DdiTask &           task,
        mfxU32              widthLa,
        mfxU32              heightLa);



    mfxU32 CalcBiWeight(
        DdiTask const & task,
        mfxU32 indexL0,
        mfxU32 indexL1);


    mfxI32 GetPicNum(
        ArrayDpbFrame const & dpb,
        mfxU8                 ref);

    mfxI32 GetPicNumF(
        ArrayDpbFrame const & dpb,
        mfxU8                 ref);

    mfxU8 GetLongTermPicNum(
        ArrayDpbFrame const & dpb,
        mfxU8                 ref);

    mfxU32 GetLongTermPicNumF(
        ArrayDpbFrame const & dpb,
        mfxU8                 ref);

    mfxI32 GetPoc(
        ArrayDpbFrame const & dpb,
        mfxU8                 ref);

    DdiTaskIter ReorderFrame(
        ArrayDpbFrame const & dpb,
        DdiTaskIter           begin,
        DdiTaskIter           end);

    DdiTaskIter ReorderFrame(
        ArrayDpbFrame const & dpb,
        DdiTaskIter           begin,
        DdiTaskIter           end,
        bool                  gopStrict,
        bool                  flush,
        bool                  closeGopForSceneChange);

    DdiTaskIter FindFrameToStartEncode(
        MfxVideoParam const & video,
        DdiTaskIter           begin,
        DdiTaskIter           end);

    DdiTaskIter FindFrameToWaitEncode(
        DdiTaskIter begin,
        DdiTaskIter end);
    DdiTaskIter FindFrameToWaitEncodeNext(
        DdiTaskIter begin,
        DdiTaskIter end,
        DdiTaskIter cur);

    PairU8 GetFrameType(
        MfxVideoParam const & video,
        mfxU32                frameOrder);

    IntraRefreshState GetIntraRefreshState(
        MfxVideoParam const & video,
        mfxU32                frameOrderInGop,
        mfxEncodeCtrl const * ctrl,
        mfxU16                intraStripeWidthInMBs,
        SliceDivider &  divider,
        ENCODE_CAPS     caps);

    mfxStatus UpdateIntraRefreshWithoutIDR(
        MfxVideoParam const & oldPar,
        MfxVideoParam const & newPar,
        mfxU32                frameOrder,
        mfxI64                oldStartFrame,
        mfxI64 &              updatedStartFrame,
        mfxU16 &              updatedStripeWidthInMBs,
        SliceDivider &  divider,
        ENCODE_CAPS     caps);

    BiFrameLocation GetBiFrameLocation(
        MfxVideoParam const & video,
        mfxU32                frameOrder);

    void UpdateDpbFrames(
        DdiTask & task,
        mfxU32    field,
        mfxU32    frameNumMax);

    void InitRefPicList(
        DdiTask & task,
        mfxU32    field);

    void ModifyRefPicLists(
        MfxVideoParam const & video,
        DdiTask &             task,
        mfxU32                fieldId);

    void MarkDecodedRefPictures(
        MfxVideoParam const & video,
        DdiTask &             task,
        mfxU32                fid);

    ArrayRefListMod CreateRefListMod(
        ArrayDpbFrame const & dpb,
        ArrayU8x33            initList,
        ArrayU8x33 const &    modList,
        mfxU32                curViewIdx,
        mfxI32                curPicNum,
        bool                  optimize);

    void ConfigureTask(
        DdiTask &             task,
        DdiTask const &       prevTask,
        MfxVideoParam const & video,
        ENCODE_CAPS const &   caps);

    mfxStatus GetNativeHandleToRawSurface(
        VideoCORE &           core,
        MfxVideoParam const & video,
        DdiTask const &       task,
        mfxHDLPair &          handle);

    mfxStatus CodeAsSkipFrame(  VideoCORE&            core,
                                MfxVideoParam const & video,
                                DdiTask&       task,
                                MfxFrameAllocResponse & pool);
    mfxStatus CopyRawSurfaceToVideoMemory(
        VideoCORE &           core,
        MfxVideoParam const & video,
        DdiTask const &       task);

    mfxHDL ConvertMidToNativeHandle(
        VideoCORE & core,
        mfxMemId    mid,
        bool        external = false);

    void AnalyzeVmeData(
        DdiTaskIter begin,
        DdiTaskIter end,
        mfxU32      width,
        mfxU32      height);

    void CalcPredWeightTable(
        DdiTask & task,
        mfxU32 MaxNum_WeightedPredL0,
        mfxU32 MaxNum_WeightedPredL1);


    struct FindByFrameOrder
    {
        FindByFrameOrder(mfxU32 frameOrder) : m_frameOrder(frameOrder) {}

        template <class T> bool operator ()(T const & task) const
        {
            return task.m_frameOrder == m_frameOrder;
        }

        mfxU32 m_frameOrder;
    };

    bool OrderByFrameNumWrap(DpbFrame const & lhs, DpbFrame const & rhs);

    template <class T, class P> typename T::pointer find_if_ptr(T & container, P pred)
    {
        typename T::iterator i = std::find_if(container.begin(), container.end(), pred);
        return (i == container.end()) ? 0 : &*i;
    }

    template <class T, class P> typename T::pointer find_if_ptr2(T & container1, T & container2, P pred)
    {
        typename T::pointer p = find_if_ptr(container1, pred);
        if (p == 0)
            p = find_if_ptr(container2, pred);
        return p;
    }
    template <class T, class P> typename T::pointer find_if_ptr3(T & container1, T & container2, T & container3, P pred)
    {
        typename T::pointer p = find_if_ptr(container1, pred);
        if (p == 0)
            p = find_if_ptr(container2, pred);
        if (p == 0)
            p = find_if_ptr(container3, pred);

        return p;
    }
    template <class T, class P> typename T::pointer find_if_ptr4(T & container1, T & container2, T & container3, T & container4, P pred)
    {
        typename T::pointer p = find_if_ptr(container1, pred);
        if (p == 0)
            p = find_if_ptr(container2, pred);
        if (p == 0)
            p = find_if_ptr(container3, pred);
        if (p == 0)
            p = find_if_ptr(container4, pred);

        return p;
    }
}; // namespace MfxHwH264Encode

#endif // _MFX_H264_ENCODE_HW_UTILS_H_
#endif // MFX_ENABLE_H264_VIDEO_ENCODE_HW
