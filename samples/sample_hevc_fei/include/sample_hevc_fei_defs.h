/******************************************************************************\
Copyright (c) 2017-2018, Intel Corporation
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

#ifndef __SAMPLE_HEVC_FEI_DEFS_H__
#define __SAMPLE_HEVC_FEI_DEFS_H__

#include <assert.h>
#include "sample_defs.h"
#include "mfxvideo.h"
#include "mfxvideo++.h"
#include "mfxfei.h"
#include "mfxfeihevc.h"
#include <algorithm>

#define CHECK_STS_AND_RETURN(X, MSG, RET) {if ((X) < MFX_ERR_NONE) {MSDK_PRINT_RET_MSG(X, MSG); return RET;}}

struct SourceFrameInfo
{
    msdk_char  strSrcFile[MSDK_MAX_FILENAME_LEN];
    mfxU32     DecodeId;       // type of input coded video
    mfxU32     ColorFormat;
    mfxU16     nPicStruct;
    mfxU16     nWidth;         // source picture width
    mfxU16     nHeight;        // source picture height
    mfxF64     dFrameRate;
    bool       fieldSplitting; // VPP field splitting

    SourceFrameInfo()
        : DecodeId(0)
        , ColorFormat(MFX_FOURCC_I420)
        , nPicStruct(MFX_PICSTRUCT_PROGRESSIVE)
        , nWidth(0)
        , nHeight(0)
        , dFrameRate(30.0)
        , fieldSplitting(false)
    {
        MSDK_ZERO_MEMORY(strSrcFile);
    }
};

struct sInputParams
{
    SourceFrameInfo input;

    msdk_char  strDstFile[MSDK_MAX_FILENAME_LEN];

    msdk_char  mbstatoutFile[MSDK_MAX_FILENAME_LEN];
    msdk_char  mvoutFile[MSDK_MAX_FILENAME_LEN];
    msdk_char  mvpInFile[MSDK_MAX_FILENAME_LEN];

    bool bENCODE;
    bool bPREENC;
    bool bEncodedOrder;        // use EncodeOrderControl for external reordering
    bool bFormattedMVout;      // use internal format for dumping MVP
    bool bFormattedMVPin;      // use internal format for reading MVP
    bool bQualityRepack;       // use quality mode in MV repack
    mfxU8  QP;
    mfxU16 dstWidth;           // destination picture width
    mfxU16 dstHeight;          // destination picture height
    mfxU32 nNumFrames;
    mfxU16 nNumSlices;
    mfxU16 nRefDist;           // distance between I- or P (or GPB) - key frames, GopRefDist = 1, there are no regular B-frames used
    mfxU16 nGopSize;           // number of frames to next I
    mfxU16 nIdrInterval;       // distance between IDR frames in GOPs
    mfxU16 BRefType;           // B-pyramid ON/OFF/UNKNOWN (let MSDK lib decide)
    mfxU16 PRefType;           // P-pyramid ON/OFF
    mfxU16 nNumRef;            // number of reference frames (DPB size)
    mfxU16 nGopOptFlag;        // STRICT | CLOSED, default is OPEN GOP
    mfxU16 GPB;                // indicates that HEVC encoder use regular P-frames or GPB
    mfxU16 NumRefActiveP;      // maximal number of references for P frames
    mfxU16 NumRefActiveBL0;    // maximal number of backward references for B frames
    mfxU16 NumRefActiveBL1;    // maximal number of forward references for B frames
    mfxU16 preencDSfactor;     // downsample input before passing to preenc (2/4/8x are supported)
    mfxU16 PicTimingSEI;       // picture timing SEI
    bool   bDisableQPOffset;   // disable qp offset per pyramid layer

    mfxExtFeiPreEncCtrl         preencCtrl;
    mfxExtFeiHevcEncFrameCtrl   encodeCtrl;

    sInputParams()
        : bENCODE(false)
        , bPREENC(false)
        , bEncodedOrder(false)
        , bFormattedMVout(false)
        , bFormattedMVPin(false)
        , bQualityRepack(false)
        , QP(26)
        , dstWidth(0)
        , dstHeight(0)
        , nNumFrames(0xffff)
        , nNumSlices(1)
        , nRefDist(0)
        , nGopSize(0)
        , nIdrInterval(0)
        , BRefType(MFX_B_REF_UNKNOWN)
        , PRefType(MFX_P_REF_SIMPLE)
        , nNumRef(0)
        , nGopOptFlag(0)               // OPEN GOP
        , GPB(MFX_CODINGOPTION_ON)     // use GPB frames
        , NumRefActiveP(3)
        , NumRefActiveBL0(3)
        , NumRefActiveBL1(1)
        , preencDSfactor(1)            // no downsampling
        , PicTimingSEI(MFX_CODINGOPTION_OFF)
        , bDisableQPOffset(false)
    {
        MSDK_ZERO_MEMORY(strDstFile);
        MSDK_ZERO_MEMORY(mbstatoutFile);
        MSDK_ZERO_MEMORY(mvoutFile);
        MSDK_ZERO_MEMORY(mvpInFile);

        MSDK_ZERO_MEMORY(preencCtrl);
        preencCtrl.Header.BufferId = MFX_EXTBUFF_FEI_PREENC_CTRL;
        preencCtrl.Header.BufferSz = sizeof(mfxExtFeiPreEncCtrl);

        preencCtrl.PictureType = MFX_PICTYPE_FRAME;
        preencCtrl.RefPictureType[0] = preencCtrl.RefPictureType[1] = preencCtrl.PictureType;
        preencCtrl.DownsampleInput = MFX_CODINGOPTION_ON;
        // PreENC works only in encoded order, so all references would be already downsampled
        preencCtrl.DownsampleReference[0] = preencCtrl.DownsampleReference[1] = MFX_CODINGOPTION_OFF;
        preencCtrl.DisableMVOutput         = 1;
        preencCtrl.DisableStatisticsOutput = 1;
        preencCtrl.SearchWindow   = 5;     // 48x40 (48 SUs)
        preencCtrl.SubMBPartMask  = 0x00;  // all enabled
        preencCtrl.SubPelMode     = 0x03;  // quarter-pixel
        preencCtrl.IntraSAD       = 0x02;  // Haar transform
        preencCtrl.InterSAD       = 0x02;  // Haar transform
        preencCtrl.IntraPartMask  = 0x00;  // all enabled

        MSDK_ZERO_MEMORY(encodeCtrl);
        encodeCtrl.Header.BufferId = MFX_EXTBUFF_HEVCFEI_ENC_CTRL;
        encodeCtrl.Header.BufferSz = sizeof(mfxExtFeiHevcEncFrameCtrl);
        encodeCtrl.SubPelMode         = 3; // quarter-pixel motion estimation
        encodeCtrl.SearchWindow       = 5; // 48 SUs 48x40 window full search
        // TODO: return default of NumFramePartitions to 4
        encodeCtrl.NumFramePartitions = 1; // number of partitions in frame that encoder processes concurrently
        // enable internal L0/L1 predictors: 1 - spatial predictors
        encodeCtrl.MultiPred[0] = encodeCtrl.MultiPred[1] = 1;
        encodeCtrl.MVPredictor = 0; // disabled MV predictors
        //set default values for NumMvPredictors
        encodeCtrl.NumMvPredictors[0] = 3;
        encodeCtrl.NumMvPredictors[1] = 1;
    }

};

/**********************************************************************************/

template <typename T, typename T1>
// `T1& lower` is bound below which `T& val` is considered invalid
T tune(const T& val, const T1& lower, const T1& default_val)
{
    if (val <= lower)
        return default_val;
    return val;
}

#define STATIC_ASSERT(ASSERTION, MESSAGE) char MESSAGE[(ASSERTION) ? 1 : -1]; MESSAGE
template<class T, class U> inline void Copy(T & dst, U const & src)
{
    STATIC_ASSERT(sizeof(T) == sizeof(U), copy_objects_of_different_size);
    MSDK_MEMCPY(&dst, &src, sizeof(dst));
}

template<class T> inline void Zero(std::vector<T> & vec)  { memset(vec.data(), 0, sizeof(T) * vec.size()); }
template<class T> inline void Zero(T * first, size_t cnt) { memset(first, 0, sizeof(T) * cnt); }
template<class T> inline void Fill(T & obj, int val) { memset(&obj, val, sizeof(obj)); }
template<class T> inline void Zero(T & obj) { Fill(obj, 0); }
template<class T> inline T Clip3(T min, T max, T x) { return std::min<T>(std::max<T>(min, x), max); }

/**********************************************************************************/

template<>struct mfx_ext_buffer_id<mfxExtFeiParam> {
    enum { id = MFX_EXTBUFF_FEI_PARAM };
};
template<>struct mfx_ext_buffer_id<mfxExtFeiPreEncCtrl> {
    enum { id = MFX_EXTBUFF_FEI_PREENC_CTRL };
};
template<>struct mfx_ext_buffer_id<mfxExtFeiPreEncMV>{
    enum {id = MFX_EXTBUFF_FEI_PREENC_MV};
};
template<>struct mfx_ext_buffer_id<mfxExtFeiPreEncMBStat>{
    enum {id = MFX_EXTBUFF_FEI_PREENC_MB};
};
template<>struct mfx_ext_buffer_id<mfxExtFeiHevcEncFrameCtrl>{
    enum {id = MFX_EXTBUFF_HEVCFEI_ENC_CTRL};
};
template<>struct mfx_ext_buffer_id<mfxExtFeiHevcEncMVPredictors>{
    enum {id = MFX_EXTBUFF_HEVCFEI_ENC_MV_PRED};
};
template<>struct mfx_ext_buffer_id<mfxExtFeiHevcEncQP>{
    enum {id = MFX_EXTBUFF_HEVCFEI_ENC_QP};
};
template<>struct mfx_ext_buffer_id<mfxExtFeiHevcEncCtuCtrl>{
    enum {id = MFX_EXTBUFF_HEVCFEI_ENC_CTU_CTRL};
};
template<>struct mfx_ext_buffer_id<mfxExtHEVCRefLists>{
    enum {id = MFX_EXTBUFF_HEVC_REFLISTS};
};

struct CmpExtBufById
{
    mfxU32 id;

    CmpExtBufById(mfxU32 _id)
        : id(_id)
    { };

    bool operator () (mfxExtBuffer* b)
    {
        return  (b && b->BufferId == id);
    };
};

/** ExtBufWrapper is an utility class which
 *  provide interface for mfxExtBuffer objects management in any mfx structure (e.g. mfxVideoParam)
 */
template<typename T>
class ExtBufWrapper: public T
{
public:
    ExtBufWrapper()
    {
        T& base = *this;
        Zero(base);
    }

    ~ExtBufWrapper() // only buffers allocated by wrapper can be released
    {
        for(ExtBufIterator it = m_ext_buf.begin(); it != m_ext_buf.end(); it++ )
        {
            delete [] (mfxU8*)(*it);
        }
    }

    ExtBufWrapper(const ExtBufWrapper& ref)
    {
        *this = ref; // call to operator=
    }

    ExtBufWrapper& operator=(const ExtBufWrapper& ref)
    {
        const T* src_base = &ref;
        return operator=(*src_base);
    }

    explicit ExtBufWrapper(const T& ref)
    {
        *this = ref; // call to operator=
    }

    ExtBufWrapper& operator=(const T& ref)
    {
        // copy content of main structure type T
        T* dst_base = this;
        const T* src_base = &ref;
        Copy(*dst_base, *src_base);

        //remove all existing extended buffers
        ClearBuffers();

        //reproduce list of extended buffers and copy its content
        m_ext_buf.reserve(ref.NumExtParam);
        for (size_t i = 0; i < ref.NumExtParam; ++i)
        {
            const mfxExtBuffer* src_buf = ref.ExtParam[i];
            if (!src_buf) throw mfxError(MFX_ERR_NULL_PTR, "Null pointer attached to source ExtParam");
            if (!IsCopyAllowed(src_buf->BufferId)) throw mfxError(MFX_ERR_UNDEFINED_BEHAVIOR, "Copying buffer with pointers not allowed");

            mfxExtBuffer* dst_buf = AddExtBuffer(src_buf->BufferId, src_buf->BufferSz);
            // copy buffer content w/o restoring its type
            memcpy((void*)dst_buf, (void*)src_buf, src_buf->BufferSz);
        }

        return *this;
    }

    template<typename TB>
    TB* AddExtBuffer()
    {
        mfxExtBuffer* b = AddExtBuffer(mfx_ext_buffer_id<TB>::id, sizeof(TB));
        return (TB*)b;
    }

    void RemoveExtBuffer(mfxU32 id)
    {
        ExtBufIterator it = std::find_if(m_ext_buf.begin(), m_ext_buf.end(), CmpExtBufById(id));
        if (it != m_ext_buf.end())
        {
            delete [] (mfxU8*)(*it);
            m_ext_buf.erase(it);
            RefreshBuffers();
        }
    }

    template <typename TB>
    TB* GetExtBuffer()
    {
        ExtBufIterator it = std::find_if(m_ext_buf.begin(), m_ext_buf.end(), CmpExtBufById(mfx_ext_buffer_id<TB>::id));
        if (it != m_ext_buf.end())
        {
            return (TB*)*it;
        }
        return 0;
    }

    template <typename TB>
    operator TB*()
    {
        return (TB*)GetExtBuffer(mfx_ext_buffer_id<TB>::id);
    }

    template <typename TB>
    operator TB*() const
    {
        return (TB*)GetExtBuffer(mfx_ext_buffer_id<TB>::id);
    }

private:
    mfxExtBuffer* AddExtBuffer(mfxU32 id, mfxU32 size)
    {
        if(!size || !id)
            return 0;

        // Limitation: only one ExtBuffer instance can be stored
        ExtBufIterator it = std::find_if(m_ext_buf.begin(), m_ext_buf.end(), CmpExtBufById(id));
        if (it == m_ext_buf.end())
        {
            mfxExtBuffer* buf = (mfxExtBuffer*)new mfxU8[size];
            memset(buf, 0, size);
            m_ext_buf.push_back(buf);

            mfxExtBuffer& ext_buf = *buf;
            ext_buf.BufferId = id;
            ext_buf.BufferSz = size;
            RefreshBuffers();

            return m_ext_buf.back();
        }

        return *it;
    }

    mfxExtBuffer* GetExtBuffer(mfxU32 id) const
    {
        constExtBufIterator it = std::find_if(m_ext_buf.begin(), m_ext_buf.end(), CmpExtBufById(id));
        if (it != m_ext_buf.end())
        {
            return *it;
        }
        return 0;
    }

    void RefreshBuffers()
    {
        this->NumExtParam = (mfxU32)m_ext_buf.size();
        this->ExtParam    = this->NumExtParam ? m_ext_buf.data() : 0;
    }

    void ClearBuffers()
    {
        if (0 != m_ext_buf.size())
        {
            for (ExtBufIterator it = m_ext_buf.begin(); it != m_ext_buf.end(); it++ )
            {
                delete [] (mfxU8*)(*it);
            }
            m_ext_buf.clear();
        }
        RefreshBuffers();
    }

    bool IsCopyAllowed(mfxU32 buf_id)
    {
        static mfxU32 allowed[] = {
            MFX_EXTBUFF_CODING_OPTION,
            MFX_EXTBUFF_CODING_OPTION2,
            MFX_EXTBUFF_CODING_OPTION3,
            MFX_EXTBUFF_FEI_PARAM,
        };

        for (mfxU32 i = 0; i < sizeof(allowed)/sizeof(allowed[0]); i++)
        {
            if (buf_id == allowed[i])
                return true;
        }

        return false;
    }

    typedef std::vector<mfxExtBuffer*>::iterator ExtBufIterator;
    typedef std::vector<mfxExtBuffer*>::const_iterator constExtBufIterator;
    std::vector<mfxExtBuffer*> m_ext_buf;
};

typedef ExtBufWrapper<mfxVideoParam> MfxVideoParamsWrapper;

typedef ExtBufWrapper<mfxEncodeCtrl> mfxEncodeCtrlWrap;

typedef ExtBufWrapper<mfxENCInput>   mfxENCInputWrap;

typedef ExtBufWrapper<mfxENCOutput>  mfxENCOutputWrap;

/**********************************************************************************/

enum
{
    MAX_DPB_SIZE = 15,
    IDX_INVALID = 0xFF,
};

enum
{
    TASK_DPB_ACTIVE = 0, // available during task execution (modified dpb[BEFORE] )
    TASK_DPB_AFTER,      // after task execution (ACTIVE + curTask if ref)
    TASK_DPB_BEFORE,     // after previous task execution (prevTask dpb[AFTER])
    TASK_DPB_NUM
};

template<typename T>
struct ExtBufferWithCounter: public T
{
    ExtBufferWithCounter() : T()
    , m_locked(0)
    {}

    mfxU16  m_locked;
};

typedef ExtBufferWithCounter<mfxExtFeiPreEncMV>     mfxExtFeiPreEncMVExtended;
typedef ExtBufferWithCounter<mfxExtFeiPreEncMBStat> mfxExtFeiPreEncMBStatExtended;

struct RefIdxPair
{
    mfxU8 RefL0;
    mfxU8 RefL1;
};

struct PreENCOutput
{
    mfxExtFeiPreEncMVExtended     * m_mv;
    mfxExtFeiPreEncMBStatExtended * m_mb;
    RefIdxPair m_activeRefIdxPair;
};

struct HevcDpbFrame
{
    mfxI32   m_poc;
    mfxU32   m_fo;    // FrameOrder
    mfxU32   m_eo;    // Encoded order
    mfxU32   m_bpo;   // Bpyramid order
    mfxU32   m_level; // Pyramid level
    mfxU8    m_tid;
    bool     m_ltr;   // is "long-term"
    bool     m_ldb;   // is "low-delay B"
    bool     m_secondField;
    bool     m_bottomField;
    mfxU8    m_codingType;
    mfxU8    m_idxRec;

    mfxFrameSurface1 *  m_surf;     // full resolution input surface
    mfxFrameSurface1 ** m_ds_surf;  // pointer to input dowsampled surface, used in preenc

    void Reset()
    {
        Fill(*this, 0);
    }
};

typedef HevcDpbFrame HevcDpbArray[MAX_DPB_SIZE];

struct HevcTask : HevcDpbFrame
{
    mfxU16       m_frameType;
    mfxU8        m_refPicList[2][MAX_DPB_SIZE];
    mfxU8        m_numRefActive[2]; // L0 and L1 lists
    mfxU8        m_shNUT;           // NALU type
    mfxI32       m_lastIPoc;
    mfxI32       m_lastRAP;
    HevcDpbArray m_dpb[TASK_DPB_NUM];
    std::list<PreENCOutput> m_preEncOutput;


    void Reset()
    {
        HevcDpbFrame::Reset();
        m_frameType = 0;
        Fill(m_refPicList, 0);
        Fill(m_numRefActive, 0);
        m_lastIPoc = 0;
        Fill(m_dpb[TASK_DPB_ACTIVE], 0);
        Fill(m_dpb[TASK_DPB_AFTER],  0);
        Fill(m_dpb[TASK_DPB_BEFORE], 0);
        m_preEncOutput.clear();
    }
};

inline bool operator ==(HevcTask const & l, HevcTask const & r)
{
    return l.m_poc == r.m_poc;
}

/**********************************************************************************/

template<mfxU32 alignment> inline mfxU32 align(mfxU32 val)
{
    STATIC_ASSERT((alignment != 0) && !(alignment & (alignment - 1)), is_power_of_2);
    return (val + alignment - 1) & ~(alignment - 1);
}



#endif // #define __SAMPLE_HEVC_FEI_DEFS_H__
