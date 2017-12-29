// Copyright (c) 2017 Intel Corporation
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
#include "cmrt_cross_platform.h"
#include "libmfx_core_interface.h"
#include "libmfx_core.h"
#include "mfx_h264_encode_cm.h"
#include "assert.h"
#if defined (__GNUC__)
#include <list>
#include <iterator>
#include <cmath>
#include <malloc.h>
#include <algorithm>
#include <iterator>
#endif
#ifndef _MFX_H264_SCD_H_
#define _MFX_H264_SCD_H_

#define ALIGN_DECL(X) __attribute__ ((aligned(X)))
#define __CDECL
#include <limits.h>
#include <cstring>
#include <stdio.h>

namespace MfxHwH264Encode
{
#undef  MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#undef  MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))

#define ASC_API_FUNC
#define ASC_API
#define STDCALL

#define OUT_BLOCK   16    // output pixels computed per thread
#define RSCS_BLOCK  4

#define PRINTDATA         0
#define ENABLE_RF         1

#undef  NULL
#define NULL              0
#define MVBLK_SIZE        8
#define BLOCK_SIZE        4
#define BLOCK_SIZE_SHIFT  2
#define LN2               0.6931471805599453
#define FRAMEMUL          16
#define FLOAT_MAX         2241178.0
#define MEMALLOCERROR     1000
#define MEMALLOCERRORU8   1001
#define MEMALLOCERRORMV   1002
#define MAXLTRHISTORY     120

#define SMALL_WIDTH       128//112
#define SMALL_HEIGHT      64
#define SMALL_AREA        8192//13 bits
#define S_AREA_SHIFT      13

#define NumTSC            10
#define NumSC             10

#define TSC_INT_SCALE     5
#define GAINDIFF_THR      20

/*--MACROS--*/
#define NMAX(a,b)         ((a>b)?a:b)
#define NMIN(a,b)         ((a<b)?a:b)
#define NABS(a)           (((a)<0)?(-(a)):(a))
#define NAVG(a,b)         ((a+b)/2)

#define Clamp(x)           ((x<0)?0:((x>255)?255:x))
#define RF_DECISION_LEVEL 10

#define TSCSTATBUFFER     3
#define ASCVIDEOSTATSBUF  2
#define SIMILITUDVAL      4

#define SCD_BLOCK_PIXEL_WIDTH   32
#define SCD_BLOCK_HEIGHT  8

typedef mfxU8*             pmfxU8;
typedef mfxI8*             pmfxI8;
typedef mfxI16*            pmfxI16;
typedef mfxU16*            pmfxU16;
typedef mfxU32*            pmfxU32;
typedef mfxUL32*           pmfxUL32;
typedef mfxL32*            pmfxL32;
typedef mfxF32*            pmfxF32;
typedef mfxF64*            pmfxF64;
typedef mfxU64*            pmfxU64;
typedef mfxI64*            pmfxI64;

typedef enum ASC_LTR_DESICION {
    NO_LTR = false,
    YES_LTR = true,
    FORCE_LTR
} ASC_LTR_DEC;

typedef enum ASCSimilar {
    Not_same,
    Same
} ASCSimil;
typedef enum ASCLayers {
    ASCFull_Size,
    ASCSmall_Size
} ASClayer;
typedef enum ASCResizing_Target {
    ASCSmall_Target
} ASCRT;
typedef enum ASCData_Flow_Direction {
    ASCReference_Frame,
    ASCCurrent_Frame,
    ASCScene_Diff_Frame
}ASCDFD;
typedef enum ASCGoP_Sizes {
    Forbidden_GoP,
    Immediate_GoP,
    QuarterHEVC_GoP,
    Three_GoP,
    HalfHEVC_GoP,
    Five_GoP,
    Six_GoP,
    Seven_Gop,
    HEVC_Gop,
    Nine_Gop,
    Ten_Gop,
    Eleven_Gop,
    Twelve_Gop,
    Thirteen_Gop,
    Fourteen_Gop,
    Fifteen_Gop,
    Double_HEVC_Gop
} ASCGOP;
typedef enum ASCBufferPosition {
    ASCcurrent_frame_data,
    ASCprevious_frame_data,
    ASCprevious_previous_frame_data,
    ASCSceneVariation_frame_data
} ASCBP;
typedef enum ASCGPU_USAGE {
    ASCNo_GPU_Proc,
    ASCDo_GPU_Proc
}ASCGU;
typedef enum ASCFrameTypeScan {
    ASC_UNKNOWN,
    ASCprogressive_frame,
    ASCtopfieldfirst_frame,
    ASCbotfieldFirst_frame
}ASCFTS;
typedef enum ASCFrameFields {
    ASCTopField,
    ASCBottomField
}ASCFF;

typedef struct ASCcoordinates {
    mfxI16
        x;
    mfxI16
        y;
}ASCMVector;

typedef struct ASCBaseline {
    mfxI32
        start;
    mfxI32
        end;
}ASCLine;

typedef struct ASCYUV_layer_store {
    mfxU8
        *data,
        *Y,
        *U,
        *V;
    mfxU32
        width,
        height,
        pitch,
        hBorder,
        wBorder,
        extWidth,
        extHeight;
}ASCYUV;
typedef struct ASCSAD_range {
    mfxU16
        SAD,
        distance;
    ASCMVector
        BestMV;
}ASCRsad;

typedef struct ASCImage_details {
    mfxU32
        Original_Width,             //User input
        Original_Height,            //User input
        horizontal_pad,             //User input for original resolution in multiples of FRAMEMUL, derived for the other two layers
        vertical_pad,               //User input for original resolution in multiples of FRAMEMUL, derived for the other two layers
        _cwidth,                    //corrected size if width not multiple of FRAMEMUL
        _cheight,                   //corrected size if height not multiple of FRAMEMUL
        pitch,                      //horizontal_pad + _cwidth + horizontal_pad
        Extended_Width,             //horizontal_pad + _cwidth + horizontal_pad
        Extended_Height,            //vertical_pad + _cheight + vertical_pad
        block_width,                //User input
        block_height,               //User input
        Width_in_blocks,            //_cwidth / block_width
        Height_in_blocks,           //_cheight / block_height
        initial_point,              //(Extended_Width * vertical_pad) + horizontal_pad
        sidesize,                   //_cheight + (1 * vertical_pad)
        endPoint,                   //(sidesize * Extended_Width) - horizontal_pad
        MVspaceSize;                //Pixels_in_Y_layer / block_width / block_height
    char
        *inputFilename;
}ASCImDetails;

typedef struct ASCVideo_characteristics {
    ASCImDetails
        *layer;
    char
        *inputFile,
        *PDistFile;
    mfxI32
        starting_frame,              //Frame number where the video is going to be accessed
        total_number_of_frames,      //Total number of frames in video file
        processed_frames,            //Number of frames that are going to be processed, taking into account the starting frame
        accuracy,
        key_frame_frequency,
        limitRange,
        maxXrange,
        maxYrange,
        interlaceMode,
        StartingField,
        currentField;
}ASCVidData;

class ASCimageData {
public:
    ASCYUV Image;
    ASCMVector
        *pInteger;
    CmSurface2DUP
        *gpuImage;
    SurfaceIndex
        *idxImage;
    mfxU32
        CsVal,
        RsVal;
    mfxI16
        avgval;
    mfxU16
        *Cs,
        *Rs,
        *RsCs,
        *SAD;
    ASC_API INT    InitFrame(ASCImDetails *pDetails);
    ASC_API mfxI32 Close();
};

typedef bool(*t_SCDetect)(mfxI32 diffMVdiffVal, mfxU32 RsCsDiff, mfxU32 MVDiff, mfxU32 Rs, mfxU32 AFD,
    mfxU32 CsDiff, mfxI32 diffTSC, mfxU32 TSC, mfxU32 gchDC, mfxI32 diffRsCsdiff,
    mfxU32 posBalance, mfxU32 SC, mfxU32 TSCindex, mfxU32 Scindex, mfxU32 Cs,
    mfxI32 diffAFD, mfxU32 negBalance, mfxU32 ssDCval, mfxU32 refDCval, mfxU32 RsDiff);

typedef struct ASCstats_structure {
    bool
        Schg;
    mfxI32
        frameNum,
        scVal,
        tscVal,
        pdist,
        histogram[5],
        last_shot_distance;
    mfxU32
        SCindex,
        TSCindex,
        Rs,
        Cs,
        SC,
        TSC,
        RsDiff,
        CsDiff,
        RsCsDiff,
        MVdiffVal;
    mfxU16
        AFD;
    mfxI32
        ssDCval,
        refDCval,
        diffAFD,
        diffTSC,
        diffRsCsDiff,
        diffMVdiffVal;
    mfxU32
        gchDC,
        posBalance,
        negBalance,
        avgVal;
    mfxI64
        ssDCint,
        refDCint;
    bool
        Gchg;
    mfxU8
        picType,
        lastFrameInShot;
    bool
        repeatedFrame,
        firstFrame,
        copyFrameDelay,
        fadeIn,
        ltr_flag;
}ASCTSCstat;

typedef struct ASCvideoBuffer {
    ASCimageData
        layer;
    mfxI32
        frame_number,
        forward_reference,
        backward_reference;
} ASCVidSample;

typedef struct ASCextended_storage {
    mfxI32
        average;
    mfxI32
        avgSAD;
    mfxU32
        gopSize,
        lastSCdetectionDistance,
        detectedSch,
        pendingSch;
    ASCTSCstat
        **logic;
    // For Pdistance table selection
    pmfxI8
        PDistanceTable;
    t_SCDetect
        detectFunc;
    ASCLayers
        size;
    bool
        firstFrame;
    ASCimageData
        gainCorrection;
    mfxU8
        control;
}ASCVidRead;


// APP_LT_REF////////////////////////////////////
typedef struct {
    mfxI32  frameOrder;
    mfxI32  Rs;
    mfxI32  Cs;
    mfxI32  AFD;
    mfxI32  TSC;
    mfxI32  MVdiffVal;
    mfxI32  Schg;
}AscLtrData;
/////////////////////////////////////////////////



#define SCD_CHECK_CM_ERR(STS, ERR) if ((STS) != CM_SUCCESS) { return ERR; }
#define SCDMFX_CHECK_CM_ERR(STS) if ((STS) != CM_SUCCESS) { return MFX_ERR_ABORTED; }

#define CM_CHK_RESULT(cm_call)                                   \
if (cm_call != CM_SUCCESS) {                                     \
return cm_call;                                                  \
}
#define CM_CHK_RESULT_DATA_READY(cm_call)                        \
if (cm_call != CM_SUCCESS) {                                     \
return false;                                                    \
}

class ASCRuntimeError : public std::exception
{
public:
    ASCRuntimeError() : std::exception() { assert(!"CmRuntimeError"); }
};

class ASC {
public:
    CmDevice
        *device     = NULL;
    CmQueue
        *queue      = NULL;
    CmSurface2D
        *pSurface   = NULL;
    CmSurface2DUP
        *pSurfaceCp = NULL;
    SurfaceIndex
        *pIdxSurf   = NULL,
        *pIdxSurfCp = NULL;
    CmProgram
        *program    = NULL;
    CmKernel
        *kernel_p   = NULL,
        *kernel_t   = NULL,
        *kernel_b   = NULL,
        *kernel_cp  = NULL;
    CmThreadSpace
        *threadSpace   = NULL,
        *threadSpaceCp = NULL;
    CmEvent
        *e        = NULL;
    CM_STATUS
        status;
    mfxU8
        *frameBkp = NULL;
    mfxU32
        gpuImPitch,
        threadsWidth,
        threadsHeight;
    std::vector<mfxFrameSurface1>
        m_scdMFXSurfacePool;
    std::vector<mfxFrameSurface1*>
        m_pscdMFXSurfacePool;
    mfxFrameAllocRequest
        scdRequest;
    mfxFrameAllocResponse
        m_scdmfxAlocResponse;
    mfxFrameSurface1
        *scdmfxFrame;
    VideoCORE
        *scdCore  = NULL;
private:
    int 
        gpuwidth,
        gpuheight;

    const int subWidth  = 128;
    const int subHeight = 64;

    bool
        cmDeviceAssigned = false;
    bool
        extCmDevice      = false;
    int gpustep_w;
    int gpustep_h;

    mfxFrameAllocRequest request;
    mfxFrameAllocResponse m_mctfmfxAlocResponse;
    
    
    CmTask
        *task = NULL;
    
    mfxU32
        version,
        hwType;
    size_t
        hwSize;

private:
    //INT
    //    res;
    ASCVidRead
        *support    = NULL;
    ASCVidData
        *dataIn     = NULL;
    ASCVidSample
        **videoData = NULL;
    bool
        dataReady,
        GPUProc,
        is_LTR_on,
        pendingSC,
        lastIwasSC;
    mfxI32
        _width,
        _height,
        _pitch;

    AscLtrData
        ltrData;  // APP_LT_REF
    /**
    ****************************************************************
    * \Brief List of long term reference friendly frame detection
    *
    * The list keeps the history of ltr friendly frame detections,
    * each element in the listis made of a frame number <mfxI32> and
    * its detection <bool> as a pair.
    *
    */
    std::list<std::pair<mfxI32, bool> >
        ltr_check_history;


    typedef void(ASC::*t_resizeImg)(mfxU8 *frame, mfxI32 srcWidth, mfxI32 srcHeight, mfxI32 inputPitch, ASCLayers dstIdx, mfxU32 parity);
    t_resizeImg resizeFunc;
    typedef void(ASC::*t_PutFrame)(mfxU8 *frame);
    t_PutFrame pPutFrameFunc;
    typedef void(ASC::*t_PutFrameSurf)(mfxFrameSurface1 *surface);
    t_PutFrameSurf pPutFrameFuncSurf;
    INT VidSample_Alloc();
    void VidSample_dispose();
    void VidRead_dispose();
    INT SetWidth(mfxI32 Width);
    INT SetHeight(mfxI32 Height);
    void SetNextField();
    void SetDimensions(mfxI32 Width, mfxI32 Height, mfxI32 Pitch);
    void alloc();
    INT IO_Setup();
    INT Setup_Environment(const mfxFrameInfo& FrameInfo);
    void SetUltraFastDetection();
    INT InitGPUsurf(VideoCORE *core, CmDevice *m_cmDevice, bool useGPUsurf);
    void Params_Init();
    void InitStruct();
    void VidRead_Init();
    void VidSample_Init();
    void SubSampleASC_ImagePro(mfxU8 *frame, mfxI32 srcWidth, mfxI32 srcHeight, mfxI32 inputPitch, ASCLayers dstIdx, mfxU32 parity);
    void SubSampleASC_ImageInt(mfxU8 *frame, mfxI32 srcWidth, mfxI32 srcHeight, mfxI32 inputPitch, ASCLayers dstIdx, mfxU32 parity);
    bool CompareStats(mfxU8 current, mfxU8 reference);
    bool FrameRepeatCheck();
    void DetectShotChangeFrame();
    void GeneralBufferRotation();
    void Put_LTR_Hint();
    ASC_LTR_DEC Continue_LTR_Mode(mfxU16 goodLTRLimit, mfxU16 badLTRLimit);
    INT SetKernel(SurfaceIndex *idxFrom, mfxU32 parity);
    bool RunFrame(mfxFrameSurface1 *surface, mfxU32 parity);
    bool RunFrame(mfxU8 *frame, mfxU32 parity);
    CmDevice * TryCreateCmDevicePtr(VideoCORE * core, mfxU32 * _version);
    INT CreateCmDevicePtr(VideoCORE * core, CmDevice *& _device, mfxU32 * _version);
    // these maps will be used by m_pCmCopy to track already created surfaces
    std::map<void *, CmSurface2D *> m_tableCmRelations2;
    std::map<CmSurface2D *, SurfaceIndex *> m_tableCmIndex2;
    ASC_API mfxStatus CreateCmSurface2D(void *pSrcD3D, CmSurface2D* & pCmSurface2D, SurfaceIndex* &pCmSrcIndex);
public:
    ASC_API INT Init(VideoCORE * core, CmDevice *m_cmDevice, const mfxFrameInfo& FrameInfo,
        mfxU16 width, mfxU16 height,
        mfxU16 pitch, mfxU16 interlaceMode, bool useHWsurf);
    /*ASC_API void Init(mfxI32 width, mfxI32 height, mfxI32 pitch, mfxU32 interlaceMode);
    ASC_API void Init(mfxI32 Width, mfxI32 Height, mfxI32 Pitch, mfxU32 interlaceMode, bool LTR_on);*/
    ASC_API INT SetPitch(mfxI32 Pitch);
    ASC_API void SetControlLevel(mfxU8 level);
    ASC_API void Reset_ASCCmDevice();
    ASC_API void Set_ASCCmDevice();
    ASC_API bool Query_ASCCmDevice();
    ASC_API void Set_prevIisSC_Status();
    ASC_API void Reset_prevIisSC_Status();
    ASC_API bool Query_prevIisSC_Status();
    ASC_API void Set_pendingSC_Status();
    ASC_API void Reset_pendingSC_Status();
    ASC_API bool Query_pendingSC_Status();
    ASC_API void Set_LTR_Status();
    ASC_API void Reset_LTR_Status();
    ASC_API bool Query_LTR_Status();
    ASC_API INT SetGoPSize(mfxU32 GoPSize);
    ASC_API INT SetInterlaceMode(mfxU32 interlaceMode);
    ASC_API void ResetGoPSize();
    ASC_API void Close();
    ASC_API void PutFrameProgressive(mfxFrameSurface1 *surface);
    ASC_API void PutFrameInterlaced(mfxFrameSurface1 *surface);
    ASC_API void PutFrameProgressive(mfxU8 *frame);
    ASC_API void PutFrameInterlaced(mfxU8 *frame);
    ASC_API INT PutFrame(mfxFrameSurface1 *surface);
    ASC_API INT PutFrame(mfxU8 *frame);
    ASC_API INT CopyFrameSurface(mfxFrameSurface1 *surfaceSrc);
    ASC_API bool Get_Last_frame_Data();
    ASC_API mfxU32 Get_starting_frame_number();
    ASC_API mfxU32 Get_frame_number();
    ASC_API mfxU32 Get_frame_shot_Decision();
    ASC_API mfxU32 Get_frame_last_in_scene();
    ASC_API bool Get_GoPcorrected_frame_shot_Decision();
    ASC_API mfxI32 Get_frame_Spatial_complexity();
    ASC_API mfxI32 Get_frame_Temporal_complexity();
    ASC_API bool Get_LTR_advice();
    //ASC_API void printStats(ASCTSCstat data, char *filename, FILE *SADOut);
    //ASC_API void printStats(mfxI32 index, char *filename, FILE *SADOut);
    ASC_API void setInputFileName(char *filename);
    ASC_API ASC_LTR_DEC get_LTR_op_hint();
    ASC_API mfxStatus calc_RsCs_pic(mfxU8 *pSrc, mfxI32 width, mfxI32 height, mfxI32 pitch, mfxF64 &RsCs);
    ASC_API mfxStatus calc_RsCs_Surf(mfxFrameSurface1 *surface, mfxF64 &rscs);
    ASC_API static mfxI32 Get_CpuFeature_AVX2();
    ASC_API static mfxI32 Get_CpuFeature_SSE41();
};

}
#endif //__MFX_H264_SCD_H
