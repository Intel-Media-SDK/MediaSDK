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
#ifndef _ASC_H_
#define _ASC_H_

#include <list>
#include <map>
#include <string>
#include "asc_structures.h"

namespace ns_asc {

typedef void(*t_GainOffset)(pmfxU8 *pSrc, pmfxU8 *pDst, mfxU16 width, mfxU16 height, mfxU16 pitch, mfxI16 gainDiff);
typedef void(*t_RsCsCalc)(pmfxU8 pSrc, int srcPitch, int wblocks, int hblocks, pmfxU16 pRs, pmfxU16 pCs);
typedef void(*t_RsCsCalc_bound)(pmfxU16 pRs, pmfxU16 pCs, pmfxU16 pRsCs, pmfxU32 pRsFrame, pmfxU32 pCsFrame, int wblocks, int hblocks);
typedef void(*t_RsCsCalc_diff)(pmfxU16 pRs0, pmfxU16 pCs0, pmfxU16 pRs1, pmfxU16 pCs1, int wblocks, int hblocks, pmfxU32 pRsDiff, pmfxU32 pCsDiff);
typedef void(*t_ImageDiffHistogram)(pmfxU8 pSrc, pmfxU8 pRef, mfxU32 pitch, mfxU32 width, mfxU32 height, mfxI32 histogram[5], mfxI64 *pSrcDC, mfxI64 *pRefDC);
typedef mfxI16(*t_AvgLumaCalc)(pmfxU32 pAvgLineVal, int len);
typedef void(*t_ME_SAD_8x8_Block_Search)(mfxU8 *pSrc, mfxU8 *pRef, int pitch, int xrange, int yrange, mfxU16 *bestSAD, int *bestX, int *bestY);
typedef void(*t_ME_SAD_8x8_Block_FSearch)(mfxU8 *pSrc, mfxU8 *pRef, int pitch, int xrange, int yrange, mfxU32 *bestSAD, int *bestX, int *bestY);
typedef mfxStatus(*t_Calc_RaCa_pic)(mfxU8 *pPicY, mfxI32 width, mfxI32 height, mfxI32 pitch, mfxF64 &RsCs);

typedef mfxU16(*t_ME_SAD_8x8_Block)(mfxU8 *pSrc, mfxU8 *pRef, mfxU32 srcPitch, mfxU32 refPitch);
typedef void  (*t_ME_VAR_8x8_Block)(mfxU8 *pSrc, mfxU8 *pRef, mfxU8 *pMCref, mfxI16 srcAvgVal, mfxI16 refAvgVal, mfxU32 srcPitch, mfxU32 refPitch, mfxI32 &var, mfxI32 &jtvar, mfxI32 &jtMCvar);


class ASCimageData {
public:
    ASC_API ASCimageData();
    ASCYUV Image;
    ASCMVector
        *pInteger;
    mfxI32
        var,
        jtvar,
        mcjtvar;
    mfxI16
        tcor,
        mcTcor;
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
    ASC_API mfxStatus InitFrame(ASCImDetails *pDetails);
    ASC_API mfxStatus InitAuxFrame(ASCImDetails *pDetails);
    ASC_API void Close();
};

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
    ASCLayers
        size;
    bool
        firstFrame;
    ASCimageData
        gainCorrection;
    mfxU8
        control;
    mfxU32
        frameOrder;
}ASCVidRead;

class ASC {
public:
    ASC_API ASC();
private:
    CmDevice
        *m_device;
    CmQueue
        *m_queue;
    CmSurface2DUP
        *m_pSurfaceCp;
    SurfaceIndex
        *m_pIdxSurfCp;
    CmProgram
        *m_program;
    CmKernel
        *m_kernel_p,
        *m_kernel_t,
        *m_kernel_b,
        *m_kernel_cp;
    CmThreadSpace
        *m_threadSpace,
        *m_threadSpaceCp;
    CmEvent
        *m_subSamplingEv,
        *m_frameCopyEv;
    mfxU32
        m_gpuImPitch,
        m_threadsWidth,
        m_threadsHeight;
    mfxU8
        *m_frameBkp;
    CmTask
        *m_task,
        *m_taskCp;
    mfxI32
        m_gpuwidth,
        m_gpuheight;

    static const int subWidth = 128;
    static const int subHeight = 64;

    ASCVidRead *m_support;
    ASCVidData *m_dataIn;
    ASCVidSample **m_videoData;
    bool
        m_dataReady,
        m_cmDeviceAssigned,
        m_is_LTR_on,
        m_ASCinitialized;
    mfxI32
        m_width,
        m_height,
        m_pitch;
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

    // these maps will be used by m_pCmCopy to track already created surfaces
    std::map<void *, CmSurface2D *> m_tableCmRelations2;
    std::map<CmSurface2D *, SurfaceIndex *> m_tableCmIndex2;

    int m_AVX2_available;
    int m_SSE4_available;
    t_GainOffset               GainOffset;
    t_RsCsCalc                 RsCsCalc_4x4;
    t_RsCsCalc_bound           RsCsCalc_bound;
    t_RsCsCalc_diff            RsCsCalc_diff;
    t_ImageDiffHistogram       ImageDiffHistogram;
    t_ME_SAD_8x8_Block_Search  ME_SAD_8x8_Block_Search;
    t_Calc_RaCa_pic            Calc_RaCa_pic;
    
    t_ME_SAD_8x8_Block         ME_SAD_8x8_Block;
    t_ME_VAR_8x8_Block         ME_VAR_8x8_Block;

    void SubSample_Point(
        pmfxU8 pSrc, mfxU32 srcWidth, mfxU32 srcHeight, mfxU32 srcPitch,
        pmfxU8 pDst, mfxU32 dstWidth, mfxU32 dstHeight, mfxU32 dstPitch,
        mfxI16 &avgLuma);
    mfxStatus RsCsCalc();
    mfxI32 ShotDetect(ASCimageData& Data, ASCimageData& DataRef, ASCImDetails& imageInfo, ASCTSCstat *current, ASCTSCstat *reference, mfxU8 controlLevel);
    void MotionAnalysis(ASCVidSample *videoIn, ASCVidSample *videoRef, mfxU32 *TSC, mfxU16 *AFD, mfxU32 *MVdiffVal, mfxU32 *AbsMVSize, mfxU32 *AbsMVHSize, mfxU32 *AbsMVVSize, ASCLayers lyrIdx);

    typedef void(ASC::*t_resizeImg)(mfxU8 *frame, mfxI32 srcWidth, mfxI32 srcHeight, mfxI32 inputPitch, ns_asc::ASCLayers dstIdx, mfxU32 parity);
    t_resizeImg resizeFunc;
    mfxStatus VidSample_Alloc();
    void VidSample_dispose();
    void VidRead_dispose();
    mfxStatus SetWidth(mfxI32 Width);
    mfxStatus SetHeight(mfxI32 Height);
    mfxStatus SetPitch(mfxI32 Pitch);
    void SetNextField();
    mfxStatus SetDimensions(mfxI32 Width, mfxI32 Height, mfxI32 Pitch);
    mfxStatus alloc();
    mfxStatus InitCPU();
    void Setup_Environment();
    void SetUltraFastDetection();
    mfxStatus InitGPUsurf(CmDevice* pCmDevice);
    void Params_Init();
    mfxStatus IO_Setup();
    void InitStruct();
    mfxStatus VidRead_Init();
    void VidSample_Init();
    void SubSampleASC_ImagePro(mfxU8 *frame, mfxI32 srcWidth, mfxI32 srcHeight, mfxI32 inputPitch, ASCLayers dstIdx, mfxU32 parity);
    void SubSampleASC_ImageInt(mfxU8 *frame, mfxI32 srcWidth, mfxI32 srcHeight, mfxI32 inputPitch, ASCLayers dstIdx, mfxU32 parity);
    bool CompareStats(mfxU8 current, mfxU8 reference);
    bool DenoiseIFrameRec();
    bool FrameRepeatCheck();
    bool DoMCTFFilteringCheck();
    void DetectShotChangeFrame();
    void GeneralBufferRotation();
    void Put_LTR_Hint();
    ASC_LTR_DEC Continue_LTR_Mode(mfxU16 goodLTRLimit, mfxU16 badLTRLimit);
    mfxStatus SetKernel(SurfaceIndex *idxFrom, SurfaceIndex *idxTo, CmTask **subSamplingTask, mfxU32 parity);
    mfxStatus SetKernel(SurfaceIndex *idxFrom, CmTask **subSamplingTask, mfxU32 parity);
    mfxStatus SetKernel(SurfaceIndex *idxFrom, mfxU32 parity);

    mfxStatus QueueFrame(mfxHDL frameHDL, SurfaceIndex *idxTo, CmEvent **subSamplingEv, CmTask **subSamplingTask, mfxU32 parity);
    mfxStatus QueueFrame(mfxHDL frameHDL, CmEvent **subSamplingEv, CmTask **subSamplingTask, mfxU32 parity);

    mfxStatus QueueFrame(mfxHDL frameHDL, mfxU32 parity);
    mfxStatus QueueFrame(SurfaceIndex *idxFrom, mfxU32 parity);
#ifndef CMRT_EMU
    mfxStatus QueueFrame(SurfaceIndex *idxFrom, SurfaceIndex *idxTo, CmEvent **subSamplingEv, CmTask **subSamplingTask, mfxU32 parity);
    mfxStatus QueueFrame(SurfaceIndex *idxFrom, CmEvent **subSamplingEv, CmTask **subSamplingTask, mfxU32 parity);
#else
    mfxStatus QueueFrame(SurfaceIndex *idxFrom, CmEvent **subSamplingEv, CmTask **subSamplingTask, CmThreadSpace *subThreadSpace, mfxU32 parity);
#endif
    void AscFrameAnalysis();
    mfxStatus RunFrame(SurfaceIndex *idxFrom, mfxU32 parity);
    mfxStatus RunFrame(mfxHDL frameHDL, mfxU32 parity);
    mfxStatus RunFrame(mfxU8 *frame, mfxU32 parity);
    mfxStatus CreateCmSurface2D(void *pSrcD3D, CmSurface2D* & pCmSurface2D, SurfaceIndex* &pCmSrcIndex);
    mfxStatus CreateCmKernels();
    mfxStatus CopyFrameSurface(mfxHDL frameHDL);
    void Reset_ASCCmDevice();
    void Set_ASCCmDevice();
    ASC_API mfxStatus SetInterlaceMode(ASCFTS interlaceMode);
public:
    bool Query_ASCCmDevice();
    ASC_API mfxStatus Init(mfxI32 Width, mfxI32 Height, mfxI32 Pitch, mfxU32 PicStruct, CmDevice* pCmDevice);
    ASC_API void Close();
    ASC_API bool IsASCinitialized();

    ASC_API mfxStatus AssignResources(mfxU8 position, mfxU8 *pixelData);
    ASC_API mfxStatus AssignResources(mfxU8 position, CmSurface2DUP *inputFrame, mfxU8 *pixelData);
    ASC_API mfxStatus SwapResources(mfxU8 position, CmSurface2DUP **inputFrame, mfxU8 **pixelData);

    ASC_API void SetControlLevel(mfxU8 level);
    ASC_API mfxStatus SetGoPSize(mfxU32 GoPSize);
    ASC_API void ResetGoPSize();

    inline void SetParityTFF() { SetInterlaceMode(ASCtopfieldfirst_frame); }
    inline void SetParityBFF() { SetInterlaceMode(ASCbotfieldFirst_frame); }
    inline void SetProgressiveOp() { SetInterlaceMode(ASCprogressive_frame); }

    ASC_API mfxStatus QueueFrameProgressive(mfxHDL surface, SurfaceIndex *idxTo, CmEvent **subSamplingEv, CmTask **subSamplingTask);
    ASC_API mfxStatus QueueFrameProgressive(mfxHDL surface, CmEvent **taskEvent, CmTask **subSamplingTask);

    ASC_API mfxStatus QueueFrameProgressive(mfxHDL surface);
    ASC_API mfxStatus QueueFrameInterlaced(mfxHDL surface);

    ASC_API mfxStatus QueueFrameProgressive(SurfaceIndex* idxSurf, CmEvent *subSamplingEv, CmTask *subSamplingTask);
    ASC_API mfxStatus QueueFrameProgressive(SurfaceIndex* idxSurf);
    ASC_API mfxStatus QueueFrameInterlaced(SurfaceIndex* idxSurf);

    ASC_API bool Query_resize_Event();
    ASC_API mfxStatus ProcessQueuedFrame(CmEvent **subSamplingEv, CmTask **subSamplingTask, CmSurface2DUP **inputFrame, mfxU8 **pixelData);
    ASC_API mfxStatus ProcessQueuedFrame();

    ASC_API mfxStatus PutFrameProgressive(mfxHDL surface);
    ASC_API mfxStatus PutFrameInterlaced(mfxHDL surface);

    ASC_API mfxStatus PutFrameProgressive(SurfaceIndex* idxSurf);
    ASC_API mfxStatus PutFrameInterlaced(SurfaceIndex* idxSurf);

    ASC_API mfxStatus PutFrameProgressive(mfxU8 *frame, mfxI32 Pitch);
    ASC_API mfxStatus PutFrameInterlaced(mfxU8 *frame, mfxI32 Pitch);

    ASC_API bool   Get_Last_frame_Data();
    ASC_API mfxU16 Get_asc_subsampling_width();
    ASC_API mfxU16 Get_asc_subsampling_height();
    ASC_API mfxU32 Get_starting_frame_number();
    ASC_API mfxU32 Get_frame_number();
    ASC_API mfxU32 Get_frame_shot_Decision();
    ASC_API mfxU32 Get_frame_last_in_scene();
    ASC_API bool   Get_GoPcorrected_frame_shot_Decision();
    ASC_API mfxI32 Get_frame_Spatial_complexity();
    ASC_API mfxI32 Get_frame_Temporal_complexity();
    ASC_API bool   Get_intra_frame_denoise_recommendation();
    ASC_API mfxU32 Get_PDist_advice();
    ASC_API bool   Get_LTR_advice();
    ASC_API bool   Get_RepeatedFrame_advice();
    ASC_API bool   Get_Filter_advice();
    ASC_API mfxStatus get_LTR_op_hint(ASC_LTR_DEC& scd_LTR_hint);

    ASC_API mfxStatus calc_RaCa_pic(mfxU8 *pSrc, mfxI32 width, mfxI32 height, mfxI32 pitch, mfxF64 &RsCs);
    ASC_API mfxStatus calc_RaCa_Surf(mfxHDL surface, mfxF64 &rscs);

    ASC_API bool Check_last_frame_processed(mfxU32 frameOrder);
    ASC_API void Reset_last_frame_processed();

    ASC_API static mfxI32 Get_CpuFeature_AVX2();
    ASC_API static mfxI32 Get_CpuFeature_SSE41();
};
};

#endif //_ASC_H_