/******************************************************************************\
Copyright (c) 2017-2019, Intel Corporation
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
#include "brc_routines.h"

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

struct PerFrameTypeCtrl
{
    mfxExtFeiHevcEncFrameCtrl   CtrlI;
    mfxExtFeiHevcEncFrameCtrl   CtrlP; // also applicable for GPB frames
    mfxExtFeiHevcEncFrameCtrl   CtrlB;

    PerFrameTypeCtrl() : CtrlI(), CtrlP(), CtrlB()
    {}
};

struct sInputParams
{
    SourceFrameInfo input;

    msdk_char  strDstFile[MSDK_MAX_FILENAME_LEN];

    msdk_char  mbstatoutFile[MSDK_MAX_FILENAME_LEN];
    msdk_char  mvoutFile[MSDK_MAX_FILENAME_LEN];
    msdk_char  mvpInFile[MSDK_MAX_FILENAME_LEN];
    msdk_char  refctrlInFile[MSDK_MAX_FILENAME_LEN];
    msdk_char  repackctrlFile[MSDK_MAX_FILENAME_LEN];
    msdk_char  repackstatFile[MSDK_MAX_FILENAME_LEN];

    bool bENCODE;
    bool bPREENC;
    bool bEncodedOrder;        // use EncodeOrderControl for external reordering
    bool bFormattedMVout;      // use internal format for dumping MVP
    bool bFormattedMVPin;      // use internal format for reading MVP
    bool bQualityRepack;       // use quality mode in MV repack
    bool bExtBRC;
    mfxU8  QP;
    mfxU16 dstWidth;           // destination picture width
    mfxU16 dstHeight;          // destination picture height
    mfxU32 nNumFrames;
    mfxU16 nNumSlices;
    mfxU16 CodecProfile;
    mfxU16 CodecLevel;
    mfxU16 TargetKbps;
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
    mfxU32 nTimeout;           // execution time in seconds

    mfxExtFeiPreEncCtrl         preencCtrl;
    mfxExtFeiHevcEncFrameCtrl   encodeCtrl;
    PerFrameTypeCtrl            frameCtrl;

    sInputParams()
        : bENCODE(false)
        , bPREENC(false)
        , bEncodedOrder(false)
        , bFormattedMVout(false)
        , bFormattedMVPin(false)
        , bQualityRepack(false)
        , bExtBRC(false)
        , QP(0)
        , dstWidth(0)
        , dstHeight(0)
        , nNumFrames(0)
        , nNumSlices(1)
        , CodecProfile(MFX_PROFILE_HEVC_MAIN)
        , CodecLevel(MFX_LEVEL_UNKNOWN)
        , TargetKbps(0)
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
        , nTimeout(0)
    {
        MSDK_ZERO_MEMORY(strDstFile);
        MSDK_ZERO_MEMORY(mbstatoutFile);
        MSDK_ZERO_MEMORY(mvoutFile);
        MSDK_ZERO_MEMORY(mvpInFile);
        MSDK_ZERO_MEMORY(refctrlInFile);
        MSDK_ZERO_MEMORY(repackctrlFile);
        MSDK_ZERO_MEMORY(repackstatFile);

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
        encodeCtrl.NumFramePartitions = 4; // number of partitions in frame that encoder processes concurrently
        // enable internal L0/L1 predictors: 1 - spatial predictors
        encodeCtrl.MultiPred[0] = encodeCtrl.MultiPred[1] = 1;
        encodeCtrl.MVPredictor = 0; // disabled MV predictors
        //set default values for NumMvPredictors
        encodeCtrl.NumMvPredictors[0] = 3;
        encodeCtrl.NumMvPredictors[1] = 1;

        frameCtrl.CtrlI = frameCtrl.CtrlP = frameCtrl.CtrlB = encodeCtrl;
        frameCtrl.CtrlI.MVPredictor = 0;
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

template<class T, class U> inline void Copy(T & dst, U const & src)
{
    static_assert(sizeof(T) == sizeof(U), "copy objects of different size");
    MSDK_MEMCPY(&dst, &src, sizeof(dst));
}

template<class T> inline void Zero(std::vector<T> & vec)  { memset(vec.data(), 0, sizeof(T) * vec.size()); }
template<class T> inline void Zero(T * first, size_t cnt) { memset(first, 0, sizeof(T) * cnt); }
template<class T> inline void Fill(T & obj, int val) { memset(&obj, val, sizeof(obj)); }
template<class T> inline void Zero(T & obj) { Fill(obj, 0); }
template<class T> inline T Clip3(T min, T max, T x) { return std::min<T>(std::max<T>(min, x), max); }

/**********************************************************************************/

typedef ExtBufHolder<mfxENCInput>   mfxENCInputWrap;
typedef ExtBufHolder<mfxENCOutput>  mfxENCOutputWrap;

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
    mfxU8        m_refPicList[2][MAX_DPB_SIZE]; // index in dpb
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

// Version for compile-time alignment arguments
template<mfxU32 alignment> inline mfxU32 align(mfxU32 val)
{
    static_assert((alignment != 0) && !(alignment & (alignment - 1)), "is not power of 2");
    return (val + alignment - 1) & ~(alignment - 1);
}

// Version for run-time alignment arguments
inline mfxU32 align(mfxU32 val, mfxU32 alignment)
{
    if ((alignment == 0) || (alignment & (alignment - 1)))
    {
        throw mfxError(MFX_ERR_UNKNOWN, "Alignment should be a power of 2");
    }
    return (val + alignment - 1) & ~(alignment - 1);
}



#endif // #define __SAMPLE_HEVC_FEI_DEFS_H__
