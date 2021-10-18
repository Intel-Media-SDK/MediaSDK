// Copyright (c) 2020 Intel Corporation
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

#include <vector>
#include "mfx_common.h"

#ifndef MFX_ENABLE_MCTF_EXT
// all internal logic is based on these constants
// if they are not defined, the logic of all checks,
// switches & branches gets too complicated (as modes are
// exist in MCTF and it is needed to somehow introduce them.
enum {
    MCTF_TEMPORAL_MODE_UNKNOWN = 0,
    MCTF_TEMPORAL_MODE_SPATIAL = 1,
    MCTF_TEMPORAL_MODE_1REF    = 2,
    MCTF_TEMPORAL_MODE_2REF    = 3,
    MCTF_TEMPORAL_MODE_4REF    = 4
};
#else
enum {
    MCTF_TEMPORAL_MODE_UNKNOWN = MFX_MCTF_TEMPORAL_MODE_UNKNOWN,
    MCTF_TEMPORAL_MODE_SPATIAL = MFX_MCTF_TEMPORAL_MODE_SPATIAL,
    MCTF_TEMPORAL_MODE_1REF = MFX_MCTF_TEMPORAL_MODE_1REF,
    MCTF_TEMPORAL_MODE_2REF = MFX_MCTF_TEMPORAL_MODE_2REF,
    MCTF_TEMPORAL_MODE_4REF = MFX_MCTF_TEMPORAL_MODE_4REF
};
#endif

// this is an internal structure to represent MCTF controls;
// is not exposed outside;
typedef struct {
    mfxU16       Overlap;
    mfxU16       Deblocking;
    mfxU16       TemporalMode;
    mfxU16       subPelPrecision;
    mfxU16       FilterStrength; // [0...20]
    mfxU32       BitsPerPixelx100k;
    mfxU16       reserved[6];
} IntMctfParams;

// multiplier to translate float bpp -->fixed-integer
enum { MCTF_BITRATE_MULTIPLIER = 100000 };

// this macro enables updates of runtime params at each frame
#undef MCTF_UPDATE_RTPARAMS_ON_EACH_FRAME

// MCTF models adopts for x264 or h265 following by MCTF
//#define MCTF_MODEL_FOR_x264_OR_x265

// MCTF models adopts for MSDK following by MCTF
#define MCTF_MODEL_FOR_MSDK

//#define MFX_MCTF_DEBUG_PRINT
#include <memory>
#include "cmrt_cross_platform.h"
#include "../../../vpp/include/mfx_vpp_defs.h"
#include "libmfx_core_interface.h"
#include "asc.h"

#include <cassert>
#define CHROMABASE      80
#define MAXCHROMA       100
#define MCTFSTRENGTH    3
#define MCTFNOFILTER    0
#define MCTFADAPTIVE    21
#define MCTFADAPTIVEVAL 1050
#define MCTF_CHROMAOFF  0

#ifndef __MFXDEFS_H__
typedef char mfxI8;
typedef char int8_t;
typedef unsigned char mfxU8;
typedef unsigned char uint8_t;
typedef short mfxI16;
typedef short int16_t;
typedef unsigned short mfxU16;
typedef unsigned short uint16_t;
typedef int mfxI32;
typedef int int32_t;
typedef unsigned int mfxU32;
typedef unsigned int uint32_t;
typedef __int64 mfxI64;
typedef __int64 long long;
typedef unsigned __int64 mfxU64;
typedef unsigned __int64 unsigned long long;
typedef double mfx64F;
typedef double double;
typedef float mfx32F;
typedef float float;

typedef struct {
    mfxI16
        x,
        y;
} mfxI16Pair;
#endif //__MFXDEFS_H__

typedef struct {
    mfxU16
        x,
        y;
} mfxU16Pair;
typedef struct {
    mfxU8
        scn1,
        scn2,
        scn3,
        scn4;
} mfxU8quad;


#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define ABS(a)    (((a) < 0) ? (-(a)) : (a))

#define MCTF_CHECK_CM_ERR(STS, ERR) if ((STS) != CM_SUCCESS) { ASC_PRINTF("FAILED at file: %s, line: %d, cmerr: %d\n", __FILE__, __LINE__, STS); return ERR; }

#define CHECK_ERR(ERR) if ((ERR) != PASSED) { ASC_PRINTF("FAILED at file: %s, line: %d\n", __FILE__, __LINE__); return (ERR); }
#define DIVUP(a, b) ((a+b-1)/b)
#define ROUNDUP(a, b) DIVUP(a,b)*b

#define BORDER                  4
#define SUBMREDIM               4
#define WIDTHB                  (WIDTH + BORDER*2)
#define HEIGHTB                 (HEIGHT + BORDER*2)

#define CROP_BLOCK_ALIGNMENT    16
#define VMEBLSIZE               16
#define TEST_MAIN               1
#define MINHEIGHT               120 //it determines which picture size will use 8x8 or 16x16 blocks, smaller than this 8x8, bigger 16x16.

enum
{ 
    PASSED,
    FAILED 
};
enum
{ 
    AMCTF_NOT_READY,
    AMCTF_READY
};
typedef enum AMCTF_OP_MODE
{
    SEPARATE_REG_OP,
    OVERLAP_REG_OP,
    SEPARATE_ADA_OP,
    OVERLAP_ADA_OP
}AMCTF_OMODE;

typedef enum _OFFSET_OV_MODE
{
    NO_OVERLAP_OFFSET = 0,
    OVERLAP_OFFSET = 2
}OFFSET_OV_MODE;

typedef enum _DENOISER_RUN_TYPE
{
    DEN_CLOSE_RUN,
    DEN_FAR_RUN
}DRT;

typedef enum _NUMBER_OF_REFERENCES
{
    NO_REFERENCES,
    ONE_REFERENCE,
    TWO_REFERENCES,
    THREE_REFERENCES,
    FOUR_REFERENCES
}NUOR;

enum class  MCTF_MODE
{
    MCTF_MANUAL_MODE, // MCTF operates based on filter-strength passed by application
    MCTF_AUTO_MODE,   // MCTF automatically ajusts filter-strength and uses noise-estimations
    MCTF_NOT_INITIALIZED_MODE
};

enum class MCTF_CONFIGURATION
{
    MCTF_MAN_NCA_NBA,         // Manual, not Content-adaptive, not bitrate-adaptive
    MCTF_AUT_CA_NBA,          // Auto (noise-estimator is used), content-adaptive, not bitrate-adaptive
    MCTF_AUT_NCA_NBA,         // Auto (noise-estimator is used), not Content - adaptive, not bitrate - adaptive
    MCTF_AUT_CA_BA,           // Auto (noise-estimator is used), Content - adaptive, bitrate - adaptive
    MCTF_NOT_CONFIGURED       // not configured yet
};

typedef enum _SUBPELPRECISION
{
    INTEGER_PEL,
    QUARTER_PEL
}SPP;

struct VmeSearchPath // sizeof=58
{
    mfxU8 sp[56];
    mfxU8 maxNumSu;
    mfxU8 lenSp;
};

struct spatialNoiseAnalysis
{
    mfxF32
        var;
    mfxF32
        SCpp;
};

struct MeControlSmall // sizeof=96
{
    VmeSearchPath searchPath;
    mfxU8  reserved[2];
    mfxU16 width;
    mfxU16 height;
    mfxU16 th;
    mfxU16 mre_width;
    mfxU16 mre_height;
    mfxU16 sTh;
    mfxU16 subPrecision;
    mfxU16  CropX;
    mfxU16  CropY;
    mfxU16  CropW;
    mfxU16  CropH;
    mfxU8  reserved2[14];
};
static_assert(sizeof(MeControlSmall) == 96,"size of MeControlSmall != 96");
struct frameData
{
    std::vector<mfxU8>
        *frame;
    mfxU8
        scene_number;
};

struct gpuFrameData
{
    CmSurface2D
        * frameData,
        *fOut;
    SurfaceIndex
        * fIdx,
        * fIdxOut;
    mfxFrameSurface1
        * mfxFrame;
    CmSurface2D
        * magData;
    SurfaceIndex
        * idxMag;
    mfxU8
        sc,
        tc,
        stc;
    mfxU32
        scene_idx,
        frame_number,
        frame_relative_position,
        noise_count;
    mfxU16
        filterStrength;
    mfxF64
        noise_var,
        noise_sad,
        noise_sc,
        frame_sad,
        frame_sc,
        frame_Rs,
        frame_Cs;
    bool
        frame_added,
        isSceneChange,
        isIntra;
    IntMctfParams
        mfxMctfControl;
    gpuFrameData() :
        frameData(0)
        , fOut(0)
        , fIdx(0)
        , fIdxOut(0)
        , mfxFrame(0)
        , magData(0)
        , idxMag(0)
        , sc(0)
        , tc(0)
        , stc(0)
        , scene_idx(0xffffffff)
        , frame_number(0)
        , frame_relative_position(0)
        , noise_count(0)
        , filterStrength(MCTFNOFILTER)
        , noise_var(0.0)
        , noise_sad(0.0)
        , noise_sc(0.0)
        , frame_sad(0.0)
        , frame_sc(0.0)
        , frame_Rs(0.0)
        , frame_Cs(0.0)
        , frame_added(false)
        , isSceneChange(false)
        , isIntra(false)
        , mfxMctfControl(IntMctfParams{})
    {
    }
};

//////////////////////////////////////////////////////////
// Defines and Constants
//////////////////////////////////////////////////////////
#define MCTFSEARCHPATHSIZE 56

typedef struct _MulSurfIdx
{
    SurfaceIndex
        * p_ppIndex,
        * p_pIndex,
        * p_curIndex,
        * p_fIndex,
        * p_ffIndex;
} MulSurfIdx;

class CMCRuntimeError : public std::exception
{
public:
    CMCRuntimeError() : std::exception() { assert(!"CmRuntimeError"); }
};

// forward declarations
using ns_asc::ASC;
//class Time;

//Cm based Motion estimation and compensation
class CMC
{

public:
    static const mfxU16 AUTO_FILTER_STRENGTH;
    static const mfxU16 DEFAULT_FILTER_STRENGTH;
    static const mfxU16 INPIPE_FILTER_STRENGTH;
    static const mfxU32 DEFAULT_BPP;
    static const mfxU16 DEFAULT_DEBLOCKING;
    static const mfxU16 DEFAULT_OVERLAP;
    static const mfxU16 DEFAULT_ME;
    static const mfxU16 DEFAULT_REFS;

    // this function fills IntMctfParams structure which is used
    // to store MCTF params; internal structure.
    static void QueryDefaultParams(IntMctfParams*);

    //this is for API
    static void QueryDefaultParams(mfxExtVppMctf*);

    static mfxStatus CheckAndFixParams(mfxExtVppMctf*);

    // this function fills MCTF params based on extended buffer
    static void FillParamControl(IntMctfParams*, const mfxExtVppMctf*);

private:
    typedef mfxI32(CMC::*t_MCTF_ME)();
    typedef mfxI32(CMC::*t_MCTF_NOA)(bool adaptControl);
    typedef mfxI32(CMC::*t_MCTF_MERGE)();
    typedef mfxI32(CMC::*t_RUN_MCTF)(bool notInPipeline);
    typedef mfxI32(CMC::*t_MCTF_LOAD)();
    typedef mfxI32(CMC::*t_MCTF_SPDEN)();

    t_MCTF_ME       pMCTF_ME_func;
    t_MCTF_MERGE    pMCTF_MERGE_func;
    t_MCTF_NOA      pMCTF_NOA_func;
    t_RUN_MCTF      pMCTF_func;
    t_MCTF_LOAD     pMCTF_LOAD_func;
    t_MCTF_SPDEN    pMCTF_SpDen_func;

    eMFXHWType
        mctf_HWType;
    CmDevice
        * device;
    CmQueue
        * queue;
    CmTask
        * task;
    CmEvent
        * e,
        * copyEv;
    CmThreadSpace
        * threadSpace,
        * threadSpace2,
        * threadSpaceMC,
        * threadSpaceMC2;
    size_t 
        hwSize;
    mfxU32
        hwType;
    mfxU64
        exeTime,
        exeTimeT;
    //Common elements
    mfxU32
        version,
        surfPitch,
        surfSize,
        surfNoisePitch,
        surfNoiseSize,
        sceneNum,
        countFrames;
    size_t
        bufferCount;
    mfxU16
        firstFrame,
        lastFrame,
        number_of_References,
        // an index to a "slot" within QfIn which corresponds
        // to a frame being an output at this moment; this is
        // for the normal MCTF operation (not at the begining)
        DefaultIdx2Out,

        // current index in QfIn to out; it might differ
        // from DefaultIdx2Out in the beginning and end;
        CurrentIdx2Out,
        deblocking_Control,
        gopBasedFilterStrength,
        overlap_Motion;
    bool
        bitrate_Adaptation;
    MCTF_MODE
        m_AutoMode;
    MCTF_CONFIGURATION
        ConfigMode;

    // current state of MCTF: is it ready to output data or not
    mfxU16
        MctfState;

    IntMctfParams
        m_RTParams,
        m_InitRTParams;
    mfxI8
        backward_distance,
        forward_distance;
    mfxU16
        bth;
    std::unique_ptr<MeControlSmall>
        p_ctrl;
    CmBuffer
        * ctrlBuf;
    CmSurface2D
        * qpel1,
        * qpel2;
    CmSurface2DUP
        * mv_1,
        * mv_2,
        * mv_3,
        * mv_4,
        * noiseAnalysisSurf,
        * distSurf;
    SurfaceIndex
        * idxCtrl,
        * idxSrc,
        * idxRef1,
        * idxRef2,
        * idxRef3,
        * idxRef4,
        * idxMv_1,
        * idxMv_2,
        * idxMv_3,
        * idxMv_4,
        * idxNoiseAnalysis,
        * idxDist;
    mfxI32
        argIdx,
        ov_width_bl,
        ov_height_bl;
    mfxU16
        blsize,
        tsWidthFull,
        tsWidth,
        tsHeight,
        tsWidthFullMC,
        tsWidthMC,
        tsHeightMC;
    void
        * mvSys1,
        * mvSys2,
        * mvSys3,
        * mvSys4,
        * noiseAnalysisSys,
        * distSys;
    mfxF64
        bpp;
    mfxU32
        m_FrameRateExtN,
        m_FrameRateExtD;
    mfxU32
        scene_numbers[5];
    mfxU64
        time;

    mfxI32 res;

    std::vector<mfxU32>
        distRef;
    std::vector<spatialNoiseAnalysis>
        var_sc;

    SurfaceIndex
        * genxRefs1,
        * genxRefs2,
        * genxRefs3,
        * genxRefs4;

    //MC elements
    CmProgram
        * programMc,
        * programDe;
    CmKernel
        * kernelNoise,
        * kernelMcDen,
        * kernelMc1r,
        * kernelMc2r,
        * kernelMc4r;

    CmSurface2D
        * mco,
        * mco2;
    SurfaceIndex
        * idxMco,
        * idxMco2;
    bool
        m_externalSCD,
        m_adaptControl, //Based on sequence stats indicates if denoising should be performed or not
        m_doFilterFrame;
    std::unique_ptr<ASC>
        pSCD;
    // a queue MCTF of frames MCTF operates on
    std::vector<gpuFrameData>
        QfIn;

    // ----------- MSDK binds------------------
    VideoCORE
        * m_pCore;

protected:
    //ME elements
    CmProgram
        * programMe;
    CmKernel
        * kernelMe,
        * kernelMeB2,
        * kernelMeB;

private:
    inline mfxStatus SetupMeControl(
        const mfxFrameInfo & FrameInfo,
        mfxU16               th,
        mfxU16               subPelPre
    );
    mfxStatus DIM_SET(
        mfxU16 overlap
    );
    mfxStatus MCTF_InitQueue(
        mfxU16 refNum
    );

    void   RotateBuffer();
    void   RotateBufferA();
    void   RotateBufferB();

    mfxStatus IM_SURF_SET_Int();
    mfxStatus IM_SURF_SET();
    mfxStatus IM_SURF_SET(
        CmSurface2D  ** p_surface,
        SurfaceIndex ** p_idxSurf
    );
    mfxStatus IM_SURF_SET(
        AbstractSurfaceHandle      pD3DSurf,
        CmSurface2D             ** p_surface,
        SurfaceIndex            ** p_idxSurf
    );
    mfxStatus IM_MRE_SURF_SET(
        CmSurface2D  ** p_Surface,
        SurfaceIndex ** p_idxSurf
    );
    mfxStatus GEN_NoiseSURF_SET(
        CmSurface2DUP   ** p_Surface,
        void            ** p_Sys,
        SurfaceIndex    ** p_idxSurf
    );
    mfxStatus GEN_SURF_SET(
        CmSurface2DUP    ** p_Surface,
        void             ** p_Sys,
        SurfaceIndex     ** p_idxSurf
    );
    mfxI32 MCTF_SET_KERNELMe(
        SurfaceIndex      * GenxRefs,
        SurfaceIndex      * idxMV,
        mfxU16              start_x,
        mfxU16              start_y,
        mfxU8 blSize
    );
    mfxI32 MCTF_SET_KERNELMeBi(
        SurfaceIndex * GenxRefs,
        SurfaceIndex * GenxRefs2,
        SurfaceIndex * idxMV,
        SurfaceIndex * idxMV2,
        mfxU16         start_x,
        mfxU16         start_y,
        mfxU8          blSize,
        mfxI8          forwardRefDist,
        mfxI8          backwardRefDist
    );
    mfxU32 MCTF_SET_KERNELMeBiMRE(
        SurfaceIndex* GenxRefs,
        SurfaceIndex* GenxRefs2,
        SurfaceIndex* idxMV,
        SurfaceIndex* idxMV2,
        mfxU16 start_x,
        mfxU16 start_y,
        mfxU8 blSize,
        mfxI8 forwardRefDist,
        mfxI8 backwardRefDist
    );
    mfxU32 MCTF_SET_KERNELMeBiMRE2(
        SurfaceIndex* GenxRefs,
        SurfaceIndex* GenxRefs2,
        SurfaceIndex* idxMV,
        SurfaceIndex* idxMV2,
        mfxU16 start_x,
        mfxU16 start_y,
        mfxU8 blSize,
        mfxI8 forwardRefDist,
        mfxI8 backwardRefDist
    );
    mfxI32 MCTF_SET_KERNELMc(
        mfxU16 start_x,
        mfxU16 start_y,
        mfxU8  srcNum,
        mfxU8  refNum
    );
    mfxI32 MCTF_SET_KERNELMc2r(
        mfxU16 start_x,
        mfxU16 start_y
    );
    mfxI32 MCTF_SET_KERNELMc2rDen(
        mfxU16 start_x,
        mfxU16 start_y
    );
    mfxI32 MCTF_SET_KERNELMc4r(
        mfxU16 start_x,
        mfxU16 start_y
    );
    mfxI32 MCTF_SET_KERNELMc4r(
        mfxU16          start_x,
        mfxU16          start_y,
        SurfaceIndex  * multiIndex
    );
    mfxI32 MCTF_SET_KERNELMc4r(
        mfxU16 start_x,
        mfxU16 start_y,
        mfxU8  runType
    );
    mfxI32 MCTF_SET_KERNELMcMerge(
        mfxU16 start_x,
        mfxU16 start_y
    );
    mfxU8  SetOverlapOp();
    mfxU8  SetOverlapOp_half();
    mfxI32 MCTF_Enqueue(
        CmTask* taskInt,
        CmEvent* & eInt,
        const CmThreadSpace* tSInt = 0
    );    
    mfxI32 MCTF_RUN_TASK_NA(
        CmKernel * kernel,
        bool       reset,
        mfxU16     widthTs,
        mfxU16     heightTs
    );
    mfxI32 MCTF_RUN_TASK(
        CmKernel * kernel,
        bool       reset
    );
    mfxI32 MCTF_RUN_DOUBLE_TASK(
        CmKernel * meKernel,
        CmKernel * mcKernel,
        bool       reset
    );
    mfxI32 MCTF_RUN_MCTASK(
        CmKernel * kernel,
        bool       reset
    );
    mfxI32 MCTF_RUN_TASK(
        CmKernel      * kernel,
        bool            reset,
        CmThreadSpace * tS
    );
    mfxI32 MCTF_RUN_ME(
        SurfaceIndex * GenxRefs,
        SurfaceIndex * idxMV
    );
    mfxI32 MCTF_RUN_ME(
        SurfaceIndex * GenxRefs,
        SurfaceIndex * GenxRefs2,
        SurfaceIndex * idxMV,
        SurfaceIndex * idxMV2,
        char forwardRefDist,
        char backwardRefDist
    );
    mfxI32 MCTF_RUN_ME_MC_HE(
        SurfaceIndex * GenxRefs,
        SurfaceIndex * GenxRefs2,
        SurfaceIndex * idxMV,
        SurfaceIndex * idxMV2,
        mfxI8          forwardRefDist,
        mfxI8          backwardRefDist,
        mfxU8          mcSufIndex
    );
    mfxI32 MCTF_RUN_ME_MC_H(
        SurfaceIndex * GenxRefs,
        SurfaceIndex * GenxRefs2,
        SurfaceIndex * idxMV,
        SurfaceIndex * idxMV2,
        mfxI8          forwardRefDist,
        mfxI8          backwardRefDist,
        mfxU8          mcSufIndex
    );
    mfxI32 MCTF_RUN_ME_1REF();
    mfxI32 MCTF_RUN_ME_2REF();
    mfxI32 MCTF_RUN_ME_4REF();

    mfxI32 MCTF_RUN_ME_2REF_HE();

    void   GET_DISTDATA();
    void   GET_DISTDATA_H();
    void   GET_NOISEDATA();
    mfxF64 GET_TOTAL_SAD();
    mfxI32 MCTF_RUN_Noise_Analysis(
        mfxU8 srcNum
    );
    void   GetSpatioTemporalComplexityFrame(
        mfxU8 currentFrame
    );
    mfxU32 computeQpClassFromBitRate(
        mfxU8 currentFrame
    );
    bool FilterEnable(
        gpuFrameData frame
    );
    mfxI32 noise_estimator(
        bool adaptControl
    );
    mfxI32 MCTF_RUN_BLEND();
    mfxI32 MCTF_RUN_BLEND(
        mfxU8 srcNum,
        mfxU8 refNum
    );
    mfxI32 MCTF_RUN_BLEND2R();
    mfxI32 MCTF_RUN_BLEND2R_DEN();
    mfxI32 MCTF_RUN_BLEND4R(
        DRT typeRun
    );
    mfxI32 MCTF_RUN_BLEND4R(
        SurfaceIndex * multiIndex
    );
    mfxI32 MCTF_BLEND4R();
    mfxI32 MCTF_RUN_MERGE();
    mfxI32 MCTF_LOAD_1REF();
    mfxI32 MCTF_LOAD_2REF();
    mfxI32 MCTF_LOAD_4REF();
    void   AssignSceneNumber();
    mfxI32 MCTF_RUN_MCTF_DEN_1REF(
        bool
    );
    mfxI32 MCTF_RUN_MCTF_DEN(
        bool notInPipeline
    );
    mfxI32 MCTF_RUN_MCTF_DEN_4REF(
        bool
    );
    mfxI32 MCTF_RUN_AMCTF_DEN();

    mfxStatus MCTF_SET_ENV(
        VideoCORE           * core,
        const mfxFrameInfo  & FrameInfo,
        const IntMctfParams * pMctfParam,
        bool                  isCmUsed,
        bool                  isNCActive
    );
    mfxI32 MCTF_SET_KERNEL_Noise(
        mfxU16 srcNum,
        mfxU16 start_x,
        mfxU16 start_y
    );
    mfxI32 MCTF_SET_KERNELDe(
        mfxU16 srcNum,
        mfxU16 start_x,
        mfxU16 start_y
    );
    mfxI32 MCTF_SET_KERNELDe(
        mfxU16 start_x,
        mfxU16 start_y
    );
    mfxI32 MCTF_RUN_AMCTF();
    mfxI32 MCTF_RUN_AMCTF(
        mfxU16 srcNum
    );
    mfxI32 MCTF_RUN_Denoise(
        mfxU16 srcNum
    );
    mfxI32 MCTF_RUN_Denoise();

    // it will update strength/deblock/bitrate depending on the current mode MCTF operates & parameters
    // stored in QfIn, in a position srcNum; it will also update the current parameters (if for next frame
    // no information was passed)
    mfxStatus MCTF_UpdateRTParams(
        IntMctfParams * pMctfParam
    );
    mfxStatus MCTF_CheckRTParams(
        const IntMctfParams * pMctfParam
    );
    mfxStatus MCTF_UpdateANDApplyRTParams(
        mfxU8 srcNum
    );
public:
    mfxU16  MCTF_QUERY_NUMBER_OF_REFERENCES();
    // sets filter-strength
    mfxStatus SetFilterStrenght(
        unsigned short tFs,//Temporal filter strength
        unsigned short sFs //Spatial filter strength
    );
    inline mfxStatus SetFilterStrenght(
        unsigned short fs
    );
    // returns number of referencies MCTF operates on
    mfxU16 MCTF_GetReferenceNumber() { return MCTF_QUERY_NUMBER_OF_REFERENCES(); };
    // Initialize MCTF
    mfxStatus MCTF_INIT(
        VideoCORE           * core,
        CmDevice            * pCmDevice,
        const mfxFrameInfo  & FrameInfo,
        const IntMctfParams * pMctfParam
    );
    mfxStatus MCTF_INIT(//When external App is using SCD, no need to initilize internal scd.
        VideoCORE           * core,
        CmDevice            * pCmDevice,
        const mfxFrameInfo  & FrameInfo,
        const IntMctfParams * pMctfParam,
        const bool            externalSCD,
        const bool            isNCActive
    );
    mfxStatus MCTF_INIT(
        VideoCORE           * core,
        CmDevice            * pCmDevice,
        const mfxFrameInfo  & FrameInfo,
        const IntMctfParams * pMctfParam,
        bool                  isCmUsed,
        const bool            externalSCD,
        const bool            useFilterAdaptControl,
        const bool            isNCActive
    );
    // returns how many frames are needed to work;
    mfxU32 MCTF_GetQueueDepth();
    mfxStatus MCTF_SetMemory(
        const std::vector<mfxFrameSurface1*> &);
    mfxStatus MCTF_SetMemory(
        bool isCmUsed
    );
    void MCTF_CLOSE();
    MCTF_CONFIGURATION MCTF_QueryMode() { return ConfigMode; };
    mfxStatus MCTF_UpdateBitrateInfo(
        mfxU32
    );
    //returns a pointer to internally allocated surface, and locks it using core method
    mfxStatus MCTF_GetEmptySurface(
        mfxFrameSurface1 ** ppSurface
    );
    // submits a surface to MCTF & set appropriate filter-strength;
    mfxU32 IM_SURF_PUT(
        CmSurface2D *p_surface,
        mfxU8       *p_data
    );

    void IntBufferUpdate(
        bool isSceneChange,
        bool isIntraFrame,
        bool doIntraFiltering
    );
    void BufferFilterAssignment(
        mfxU16 * filterStrength,
        bool     doIntraFiltering,
        bool     isAnchorFrame,
        bool     isSceneChange
    );
    bool MCTF_Check_Use();
    mfxStatus MCTF_PUT_FRAME(
        void         * frameInData,
        mfxHDLPair     frameOutHandle,
        CmSurface2D ** frameOutData,
        bool           isCmSurface,
        mfxU16       * filterStrength,
        bool           needsOutput,
        bool           doIntraFiltering
    );
    mfxStatus MCTF_PUT_FRAME(
        IntMctfParams   * pMctfControl,
        CmSurface2D     * InSurf,
        CmSurface2D     * OutSurf
    );
    mfxStatus MCTF_PUT_FRAME(
        IntMctfParams * pMctfControl,
        CmSurface2D   * OutSurf,
        mfxU32          schgDesicion
    );
    mfxStatus MCTF_PUT_FRAME(
        mfxU32        sceneNumber,
        CmSurface2D * OutSurf
    );
    mfxStatus MCTF_UpdateBufferCount();
    mfxStatus MCTF_DO_FILTERING_IN_AVC();
    mfxU16    MCTF_QUERY_FILTER_STRENGTH();
    mfxStatus MCTF_DO_FILTERING();
    // returns (query) result of filtering to outFrame
    mfxStatus MCTF_GET_FRAME(
        CmSurface2D* outFrame
    );
    mfxStatus MCTF_GET_FRAME(
        mfxU8 * outFrame
    );
    bool    MCTF_CHECK_FILTER_USE();
    mfxStatus MCTF_RELEASE_FRAME(
        bool isCmUsed
    );
    // after MCTF_GET_FRAME is invoked, we need to update TimeStamp & FrameOrder
    // To cal this function with a surface that needs to be updated
    mfxStatus MCTF_TrackTimeStamp(
        mfxFrameSurface1 * outFrame
    );
    bool MCTF_ReadyToOutput() { return (AMCTF_READY == MctfState); };
};
