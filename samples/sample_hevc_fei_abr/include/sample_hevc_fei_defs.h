/******************************************************************************\
Copyright (c) 2018, Intel Corporation
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

#include <algorithm>
#include <assert.h>

#include "mfxvideo.h"
#include "mfxvideo++.h"
#include "mfxfei.h"
#include "mfxfeihevc.h"

#include "sample_defs.h"

struct SourceFrameInfo
{
    msdk_char  strSrcFile[MSDK_MAX_FILENAME_LEN];
    mfxU32     DecodeId;       // type of input coded video

    bool       bDSO;
    msdk_char  strDsoFile[MSDK_MAX_FILENAME_LEN];
    bool       forceToIntra;
    bool       forceToInter;

    mfxU16 DSOMVPBlockSize;

    SourceFrameInfo()
        : DecodeId(0)
        , bDSO(false)
        , forceToIntra(false)
        , forceToInter(false)
        , DSOMVPBlockSize(7)
    {
        MSDK_ZERO_MEMORY(strSrcFile);
        MSDK_ZERO_MEMORY(strDsoFile);
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

enum BRC_TYPE
{
    NONE      = 0,
    LOOKAHEAD = 1,
    MSDKSW    = 2
};

enum DIST_EST_ALGO
{
    MSE = 0, // Mean Squared Error
    NNZ = 1, // Number of Non-Zero transform coefficients
    SSC = 2  // Sum of Squared Coefficients
};

struct sBrcParams
{
    BRC_TYPE eBrcType       = NONE; // bitrate control type
    mfxU16 TargetKbps       = 0;    // valid if BRC used
    // Following controls are valid for LA BRC
    mfxU16 LookAheadDepth   = 0;    // Frames to Look Ahead
    mfxU16 LookBackDepth    = 0;    // Frames to take into account from past
    mfxU16 AdaptationLength = 0;    // Frames from past to compute bitrate adjustment ratio
    DIST_EST_ALGO eAlgType  = NNZ;  // Algorithm to estimate Distortion

    msdk_char strYUVFile[MSDK_MAX_FILENAME_LEN] = {0}; // YUV file for MSE calculation
};

enum PipelineMode
{
    Full = 0,
    Producer, // decode/DSO
    Consumer,
};

struct sInputParams
{
    SourceFrameInfo input;

    msdk_char  strDstFile[MSDK_MAX_FILENAME_LEN];

    bool bEncodedOrder;        // use EncodeOrderControl for external reordering
    mfxU8  QP;
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
    mfxU16 PicTimingSEI;       // picture timing SEI
    bool   bDisableQPOffset;   // disable qp offset per pyramid layer
    bool   drawMVP;
    bool   dumpMVP;

    PipelineMode pipeMode;

    sBrcParams sBRCparams;

    mfxExtFeiHevcEncFrameCtrl   encodeCtrl;
    PerFrameTypeCtrl            frameCtrl;

    sInputParams()
        : bEncodedOrder(true)
        , QP(26)
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
        , PicTimingSEI(MFX_CODINGOPTION_OFF)
        , bDisableQPOffset(false)
        , drawMVP(false)
        , dumpMVP(false)
        , pipeMode(Full)
        , sBRCparams()
    {
        MSDK_ZERO_MEMORY(strDstFile);

        MSDK_ZERO_MEMORY(encodeCtrl);
        encodeCtrl.Header.BufferId = MFX_EXTBUFF_HEVCFEI_ENC_CTRL;
        encodeCtrl.Header.BufferSz = sizeof(mfxExtFeiHevcEncFrameCtrl);
        encodeCtrl.SubPelMode         = 3; // quarter-pixel motion estimation
        encodeCtrl.SearchWindow       = 5; // 48 SUs 48x40 window full search
        encodeCtrl.NumFramePartitions = 4; // number of partitions in frame that encoder processes concurrently
        // enable internal L0/L1 predictors: 1 - spatial predictors
        encodeCtrl.MultiPred[0] = encodeCtrl.MultiPred[1] = 1;
        encodeCtrl.MVPredictor = 7;

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

template<class T> inline void Zero(std::vector<T> & vec)  { std::fill(std::begin(vec), std::end(vec), 0); }
template<class T> inline void Zero(T * first, size_t cnt) { std::fill(first, first + cnt, 0); }
template<class T> inline void Fill(T & obj, int val) { memset(&obj, val, sizeof(obj)); }
template<class T> inline void Zero(T & obj) { Fill(obj, 0); }
template<class T> inline T Clip3(T min, T max, T x) { return std::min(std::max(min, x), max); }

/**********************************************************************************/

inline mfxU32 align(const mfxU32 val, const mfxU32 alignment)
{
    assert((alignment != 0) && ((alignment & (alignment - 1)) == 0));
    return (val + alignment - 1) & ~(alignment - 1);
}

#endif // #define __SAMPLE_HEVC_FEI_DEFS_H__
