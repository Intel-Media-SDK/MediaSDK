// Copyright (c) 2017-2020 Intel Corporation
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

#ifndef __MFX_EXT_BUFFERS_H__
#define __MFX_EXT_BUFFERS_H__

#include "mfxcommon.h"
#include "mfx_config.h"
#include "vm_strings.h"

#if (MFX_VERSION < 1025)
enum {
    MFX_MEMTYPE_VIDEO_MEMORY_ENCODER_TARGET = MFX_MEMTYPE_RESERVED2
};
#endif


#define MFX_EXTBUFF_DEC_ADAPTIVE_PLAYBACK MFX_MAKEFOURCC('A','P','B','K')

#define MFX_EXTBUFF_DDI MFX_MAKEFOURCC('D','D','I','P')

typedef struct {
    mfxExtBuffer Header;

    // parameters below doesn't exist in MediaSDK public API
    mfxU16  IntraPredCostType;      // from DDI: 1=SAD, 2=SSD, 4=SATD_HADAMARD, 8=SATD_HARR
    mfxU16  MEInterpolationMethod;  // from DDI: 1=VME4TAP, 2=BILINEAR, 4=WMV4TAP, 8=AVC6TAP
    mfxU16  MEFractionalSearchType; // from DDI: 1=FULL, 2=HALF, 4=SQUARE, 8=HQ, 16=DIAMOND
    mfxU16  MaxMVs;
    mfxU16  SkipCheck;              // tri-state: 0, MFX_CODINGOPTION_OFF, MFX_CODINGOPTION_ON
    mfxU16  DirectCheck;            // tri-state: 0, MFX_CODINGOPTION_OFF, MFX_CODINGOPTION_ON
    mfxU16  BiDirSearch;            // tri-state: 0, MFX_CODINGOPTION_OFF, MFX_CODINGOPTION_ON
    mfxU16  MBAFF;                  // tri-state: 0, MFX_CODINGOPTION_OFF, MFX_CODINGOPTION_ON
    mfxU16  FieldPrediction;        // tri-state: 0, MFX_CODINGOPTION_OFF, MFX_CODINGOPTION_ON
    mfxU16  RefOppositeField;       // tri-state: 0, MFX_CODINGOPTION_OFF, MFX_CODINGOPTION_ON
    mfxU16  ChromaInME;             // tri-state: 0, MFX_CODINGOPTION_OFF, MFX_CODINGOPTION_ON
    mfxU16  WeightedPrediction;     // tri-state: 0, MFX_CODINGOPTION_OFF, MFX_CODINGOPTION_ON
    mfxU16  MVPrediction;           // tri-state: 0, MFX_CODINGOPTION_OFF, MFX_CODINGOPTION_ON

    struct {
        mfxU16 IntraPredBlockSize;  // mask of 1=4x4, 2=8x8, 4=16x16, 8=PCM
        mfxU16 InterPredBlockSize;  // mask of 1=16x16, 2=16x8, 4=8x16, 8=8x8, 16=8x4, 32=4x8, 64=4x4
    } DDI;

    // MediaSDK parametrization
    mfxU16  BRCPrecision;   // 0=default=normal, 1=lowest, 2=normal, 3=highest
    mfxU16  RefRaw;         // (tri-state: 0, MFX_CODINGOPTION_OFF, MFX_CODINGOPTION_ON) on=vme reference on raw(input) frames, off=reconstructed frames
    mfxU16  reserved0;
    mfxU16  ConstQP;        // disable bit-rate control and use constant QP
    mfxU16  GlobalSearch;   // 0=default, 1=long, 2=medium, 3=short
    mfxU16  LocalSearch;    // 0=default, 1=type, 2=small, 3=square, 4=diamond, 5=large diamond, 6=exhaustive, 7=heavy horizontal, 8=heavy vertical

    mfxU16 EarlySkip;       // 0=default (let driver choose), 1=enabled, 2=disabled
    mfxU16 LaScaleFactor;   // 0=default (let msdk choose), 1=1x, 2=2x, 4=4x; Deprecated for legacy H264 encoder, for legacy use mfxExtCodingOption2::LookAheadDS instead
    mfxU16 reserved1;       //
    mfxU16 reserved2;       //
    mfxU16 StrengthN;       // strength=StrengthN/100.0
    mfxU16 FractionalQP;    // 0=disabled (default), 1=enabled

    mfxU16 NumActiveRefP;   //
    mfxU16 NumActiveRefBL0; //

    mfxU16 DisablePSubMBPartition;  // tri-state, default depends on Profile and Level
    mfxU16 DisableBSubMBPartition;  // tri-state, default depends on Profile and Level
    mfxU16 WeightedBiPredIdc;       // 0 - off, 1 - explicit (unsupported), 2 - implicit
    mfxU16 DirectSpatialMvPredFlag; // (tri-state: 0, MFX_CODINGOPTION_OFF, MFX_CODINGOPTION_ON)on=spatial on, off=temporal on
    mfxU16 Transform8x8Mode;        // tri-state: 0, MFX_CODINGOPTION_OFF, MFX_CODINGOPTION_ON
    mfxU16 reserved3;       // 0..255
    mfxU16 LongStartCodes;          // tri-state, use long start-codes for all NALU
    mfxU16 CabacInitIdcPlus1;       // 0 - use default value, 1 - cabac_init_idc = 0 and so on
    mfxU16 NumActiveRefBL1;         //
    mfxU16 QpUpdateRange;           // 
    mfxU16 RegressionWindow;        //
    mfxU16 LookAheadDependency;     // LookAheadDependency < LookAhead
    mfxU16 Hme;                     // tri-state
    mfxU16 reserved4;               //
    mfxU16 WriteIVFHeaders;         // tri-state
    mfxU16 RefreshFrameContext;
    mfxU16 ChangeFrameContextIdxForTS;
    mfxU16 SuperFrameForTS;
    mfxU16 QpAdjust;                // on/off - forces sps.QpAdjustment

} mfxExtCodingOptionDDI;







#endif // __MFX_EXT_BUFFERS_H__
