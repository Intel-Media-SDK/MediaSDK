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
#ifndef _ASCSTRUCTURES_H_
#define _ASCSTRUCTURES_H_

#pragma once

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

    #include <immintrin.h>

#include "mfxdefs.h"
#include "cmrt_cross_platform.h"

    #define ASC_API_FUNC //__attribute__((visibility("default")))
    #define ASC_API      //__attribute__((visibility("default")))

typedef mfxU8*             pmfxU8;
typedef mfxI8*             pmfxI8;
typedef mfxU16*            pmfxU16;
typedef mfxI16*            pmfxI16;
typedef mfxU32*            pmfxU32;
typedef mfxI32*            pmfxI32;
typedef mfxL32*            pmfxL32;
typedef mfxF32*            pmfxF32;
typedef mfxF64*            pmfxF64;
typedef mfxU64*            pmfxU64;
typedef mfxI64*            pmfxI64;

namespace ns_asc{

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
typedef struct ASCtimming_measurement_var {
    LARGE_INTEGER
        tFrequency,
        tStart,
        tPause[10],
        tStop;
    mfxF64
        timeval,
        calctime;
}ASCTime;

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
      mfxI32
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
        MVspaceSize;                //Pixels_in_Y_layer / block_width / block_height
}ASCImDetails;

typedef struct ASCVideo_characteristics {
    ASCImDetails
        *layer;
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
    ASCTime
        timer;
}ASCVidData;

typedef struct ASCstats_structure {
    mfxI32
        frameNum,
        scVal,
        tscVal,
        pdist,
        histogram[5],
        Schg,
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
        MVdiffVal,
        AbsMVSize,
        AbsMVHSize,
        AbsMVVSize;
    mfxI16
        tcor,
        mcTcor;
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
        filterIntra_flag,
        doFilter_flag,
        ltr_flag;
}ASCTSCstat;

}; //namespace ns_asc
#endif //_STRUCTURES_H_
