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

#ifndef __MFX_SCD_H
#define __MFX_SCD_H

#include "mfx_common.h"

#if defined(MFX_ENABLE_SCENE_CHANGE_DETECTION_VPP)

#include "mfx_platform_headers.h"
#include <memory>
#include <map>

// Include needed for CM wrapper
#include "cmrt_cross_platform.h"

#include <limits.h>
#ifdef MFX_VA_LINUX
  #include <immintrin.h> // SSE, AVX
#else
  #include <intrin.h>
#endif

// Pre-built SCD kernels
#include "asc_genx_bdw_isa.cpp"
#include "asc_genx_hsw_isa.cpp"
#include "asc_genx_skl_isa.cpp"

//using namespace MfxHwVideoProcessing;

/* Special scene change detector classes/structures */

enum cpuOptimizations
{
    CPU_NONE = 0,
    CPU_SSE4,
    CPU_AVX2
};

enum Simil
{
    Not_same,
    Same
};

enum GoP_Sizes
{
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
};

enum Data_Flow_Direction
{
    Reference_Frame,
    Current_Frame
};

enum FrameFields
{
    TopField,
    BottomField
};

enum FrameTypeScan
{
    progressive_frame,
    topfieldfirst_frame,
    bottomfieldFirst_frame
};

enum BufferPosition
{
    previous_previous_frame_data,
    previous_frame_data,
    current_frame_data
};

enum Layers
{
    Full_Size,
    Small_Size
};

typedef struct coordinates
{
    mfxI32 x;
    mfxI32 y;
} MVector;

typedef struct SAD_range
{
    mfxU32  SAD;
    mfxU32  distance;
    mfxU32  direction;
    MVector BestMV;
    mfxI32  angle;
} Rsad;

typedef struct YUV_layer_store {
   mfxU8 *data;
   mfxU8 *Y;
   mfxU8 *U;
   mfxU8 *V;
} YUV;

typedef struct Image_details {
    mfxI32 Original_Width;             //User input
    mfxI32 Original_Height;            //User input
    mfxI32 horizontal_pad;             //User input for original resolution in multiples of FRAMEMUL; derived for the other two layers
    mfxI32 vertical_pad;               //User input for original resolution in multiples of FRAMEMUL; derived for the other two layers
    mfxI32 _cwidth;                    //corrected size if width not multiple of FRAMEMUL
    mfxI32 _cheight;                   //corrected size if height not multiple of FRAMEMUL
    mfxI32 pitch;                      //horizontal_pad + _cwidth + horizontal_pad
    mfxI32 Extended_Width;             //horizontal_pad + _cwidth + horizontal_pad
    mfxI32 Extended_Height;            //vertical_pad + _cheight + vertical_pad
    mfxI32 Total_non_corrected_pixels;
    mfxI32 Pixels_in_Y_layer;          //_cwidth * _cheight
    mfxI32 Pixels_in_U_layer;          //Pixels_in_Y_layer / 4 (assuming 4:2:0)
    mfxI32 Pixels_in_V_layer;          //Pixels_in_Y_layer / 4 (assuming 4:2:0)
    mfxI32 Pixels_in_full_frame;       //Pixels_in_Y_layer * 3 / 2 (assuming 4:2:0)
    mfxI32 block_width;                //User input
    mfxI32 block_height;               //User input
    mfxI32 Pixels_in_block;            //block_width x block_height
    mfxI32 Width_in_blocks;            //_cwidth / block_width
    mfxI32 Height_in_blocks;           //_cheight / block_height
    mfxI32 Blocks_in_Frame;            //Pixels_in_Y_layer / Pixels_in_block
    mfxI32 initial_point;              //(Extended_Width * vertical_pad) + horizontal_pad
    mfxI32 sidesize;                   //_cheight + (1 * vertical_pad)
    mfxI32 endPoint;                   //(sidesize * Extended_Width) - horizontal_pad
    mfxI32 MVspaceSize;                //Pixels_in_Y_layer / block_width / block_height
} ImDetails;

typedef struct Video_characteristics {
    ImDetails *layer;
    char   *inputFile;
    mfxI32  starting_frame;              //Frame number where the video is going to be accessed
    mfxI32  total_number_of_frames;      //Total number of frames in video file
    mfxI32  processed_frames;            //Number of frames that are going to be processed; taking into account the starting frame
    mfxI32  accuracy;
    mfxI32  key_frame_frequency;
    mfxI32  limitRange;
    mfxI32  maxXrange;
    mfxI32  maxYrange;
    mfxI32  interlaceMode;
    mfxI32  StartingField;
    mfxI32  currentField;
}VidData;

typedef struct frameBuffer
{
    YUV      Image;
    MVector *pInteger;
    mfxF32   CsVal;
    mfxF32   RsVal;
    mfxF32   avgval;
    mfxU32   *SAD;
    mfxF32   *Cs;
    mfxF32   *Rs;
    mfxF32   *RsCs;
}imageData;

typedef struct stats_structure
{
    mfxI32 frameNum;
    mfxI32 SCindex;
    mfxI32 TSCindex;
    mfxI32 scVal;
    mfxI32 tscVal;
    mfxI32 pdist;
    mfxI32 histogram[5];
    mfxI32 Schg;
    mfxI32 last_shot_distance;
    mfxF32 Rs;
    mfxF32 Cs;
    mfxF32 SC;
    mfxF32 AFD;
    mfxF32 TSC;
    mfxF32 RsDiff;
    mfxF32 CsDiff;
    mfxF32 RsCsDiff;
    mfxF32 MVdiffVal;
    mfxF32 diffAFD;
    mfxF32 diffTSC;
    mfxF32 diffRsCsDiff;
    mfxF32 diffMVdiffVal;
    mfxF64 gchDC;
    mfxF64 posBalance;
    mfxF64 negBalance;
    mfxF64 ssDCval;
    mfxF64 refDCval;
    mfxF64 avgVal;
    mfxI64 ssDCint;
    mfxI64 refDCint;
    BOOL   Gchg;
    mfxU8  picType;
    mfxU8  lastFrameInShot;
    BOOL   repeatedFrame;
    BOOL   firstFrame;
} TSCstat;

typedef struct videoBuffer {
    imageData *layer;
    mfxI32     frame_number;
    mfxI32     forward_reference;
    mfxI32     backward_reference;
} VidSample;

typedef struct extended_storage {
    mfxF64      average;
    mfxF32      avgSAD;
    mfxU32      gopSize;
    mfxU32      lastSCdetectionDistance;
    mfxU32      detectedSch;
    mfxU32      pendingSch;
    TSCstat   **logic;
    // For Pdistance table selection
    mfxI32      *PDistanceTable;
    Layers      size;
    BOOL        firstFrame;
    imageData   gainCorrection;
} VidRead;

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

class SceneChangeDetector
{
public:
    SceneChangeDetector()
        : m_pCmDevice(0)
        , m_pCmProgram(0)
        , m_pCmKernelTopField(0)
        , m_pCmKernelBotField(0)
        , m_pCmKernelFrame(0)
        , m_pCmQueue(0)
        , m_pCmBufferOut(0)
        , m_pCmThreadSpace(0)
        , gpustep_w(0)
        , gpustep_h(0)
        , m_gpuwidth(0)
        , m_gpuheight(0)
        , m_bInited(false)
        , m_cpuOpt(CPU_NONE)
        , support(0)
        , m_dataIn(0)
        , videoData(0)
        , dataReady(false)
        , GPUProc(false)
        , _width(0)
        , _height(0)
        , _pitch(0)
        , m_platform(PLATFORM_INTEL_UNKNOWN)
    {};

    ~SceneChangeDetector()
    {
       if (m_bInited)
       {
          Close();
       }
    }

    mfxStatus Init(mfxI32 width, mfxI32 height, mfxI32 pitch, mfxU32 interlaceMode, CmDevice* pCmDevice);

    mfxStatus Close();

    mfxStatus SetGoPSize(mfxU32 GoPSize);

    mfxStatus MapFrame(mfxFrameSurface1 *pSurf);

    BOOL      ProcessField();
    mfxU32    Get_frame_number();
    mfxU32    Get_frame_shot_Decision();
    mfxU32    Get_frame_last_in_scene();
    BOOL      Query_is_frame_repeated();

    void      SetParityTFF();    //Sets the detection to interlaced Top Field First mode, can be done on the fly
    void      SetParityBFF();    //Sets the detection to interlaced Bottom Field First mode, can be done on the fly
    void      SetProgressiveOp();//Sets the detection to progressive frame mode, can be done on the fly

private:
    void   ShotDetect(imageData Data, imageData DataRef, ImDetails imageInfo, TSCstat *current, TSCstat *reference);
    void   SetInterlaceMode(mfxU32 interlaceMode);
    void   ResetGoPSize();
    void   PutFrameProgressive();
    void   PutFrameInterlaced();
    BOOL   PutFrame();
    BOOL   Get_Last_frame_Data();
    mfxU32 Get_starting_frame_number();
    BOOL   Get_GoPcorrected_frame_shot_Decision();
    mfxI32 Get_frame_Spatial_complexity();
    mfxI32 Get_frame_Temporal_complexity();
    mfxI32 Get_stream_parity();
    BOOL   SCDetectGPU(mfxF64 diffMVdiffVal, mfxF64 RsCsDiff,   mfxF64 MVDiff,   mfxF64 Rs,       mfxF64 AFD,
                                mfxF64 CsDiff,        mfxF64 diffTSC,    mfxF64 TSC,      mfxF64 gchDC,    mfxF64 diffRsCsdiff,
                                mfxF64 posBalance,    mfxF64 SC,         mfxF64 TSCindex, mfxF64 Scindex,  mfxF64 Cs,
                                mfxF64 diffAFD,       mfxF64 negBalance, mfxF64 ssDCval,  mfxF64 refDCval, mfxF64 RsDiff);

    void   VidSample_Alloc();
    void   VidSample_dispose();
    void   ImDetails_Init(ImDetails *Rdata);
    void   nullifier(imageData *Buffer);
    void   imageInit(YUV *buffer);
    void   VidRead_dispose();
    mfxStatus SetWidth(mfxI32 Width);
    mfxStatus SetHeight(mfxI32 Height);
    mfxStatus SetPitch(mfxI32 Pitch);
    void SetNextField();
    mfxStatus SetDimensions(mfxI32 Width, mfxI32 Height, mfxI32 Pitch);
    void GpuSubSampleASC_Image(mfxI32 srcWidth, mfxI32 srcHeight, mfxI32 inputPitch, Layers dstIdx, mfxU32 parity);
    mfxStatus SubSampleImage(mfxU32 srcWidth, mfxU32 srcHeight, const unsigned char *pData);

    void ReadOutputImage();
    void GPUProcess();
    void alloc();
    void IO_Setup();
    void Setup_UFast_Environment();
    void SetUltraFastDetection();
    void Params_Init();
    void InitStruct();
    void VidRead_Init();
    void VidSample_Init();
    BOOL CompareStats(mfxU8 current, mfxU8 reference, BOOL isInterlaced);
    BOOL FrameRepeatCheck(BOOL isInterlaced);
    void processFrame();
    void GeneralBufferRotation();
    BOOL RunFrame(mfxU32 parity);

    void RsCsCalc(imageData *exBuffer, ImDetails vidCar);

    /* Motion estimation stuff */
    void MotionAnalysis(VidData *dataIn, VidSample *videoIn, VidSample *videoRef, mfxF32 *TSC, mfxF32 *AFD, mfxF32 *MVdiffVal, Layers lyrIdx);
    mfxI32 __cdecl HME_Low8x8fast(VidRead *videoIn, mfxI32 fPos, ImDetails dataIn, imageData *scale, imageData *scaleRef, BOOL first, mfxU32 accuracy, VidData limits);

    CmDevice        *m_pCmDevice;
    CmProgram       *m_pCmProgram;
    CmKernel        *m_pCmKernelTopField; //top field only, not TFF frame !!!
    CmKernel        *m_pCmKernelBotField; //bottom field only, not BFF frame !!!
    CmKernel        *m_pCmKernelFrame;
    CmQueue         *m_pCmQueue;
    CmBufferUP      *m_pCmBufferOut;
    CmThreadSpace   *m_pCmThreadSpace;

    std::map<void *, CmSurface2D *> m_tableCmRelations;

    static const int subWidth = 112; // width of SCD output image
    static const int subHeight = 64; // heigh of SCD output image

    int gpustep_w;
    int gpustep_h;
    int m_gpuwidth;
    int m_gpuheight;

    BOOL         m_bInited; // Indicates if SCD inited or not
    mfxU16       m_cpuOpt; // CPU optimizations available
    VidRead     *support;
    VidData     *m_dataIn;
    VidSample   **videoData; // contains image data and statistics. videoData[2] only has video data
    BOOL         dataReady;
    BOOL         GPUProc;
    mfxI32      _width, _height, _pitch;
    mfxU32      m_platform;
};

#endif //defined(MFX_VA) && defined(MFX_ENABLE_SCENE_CHANGE_DETECTION_VPP)
#endif //__MFX_SCD_H
