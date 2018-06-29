// Copyright (c) 2017-2018 Intel Corporation
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

#ifndef __MFXSTRUCTURESPRO_H__
#define __MFXSTRUCTURESPRO_H__
#include "mfxstructures.h"

#ifdef _MSVC_LANG
#pragma warning(disable: 4201)
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/* Frame Parameters for AVC */
typedef struct {
    mfxU32  CucId;
    mfxU32  CucSz;
    mfxU8   FrameType;
    mfxU8   FrameType2nd;
    mfxU8   reserved1a;
    mfxU8   RecFrameLabel;
    mfxU8   DecFrameLabel;
    mfxU8   VppFrameLabel;
    mfxU16  NumMb;
    mfxU16  FrameWinMbMinus1;
    mfxU16  FrameHinMbMinus1;
    mfxU8   CurrFrameLabel;
    mfxU8   NumRefFrame;

    union {
        mfxU16  CodecFlags;
        struct {
            mfxU16  FieldPicFlag    :1;
            union {
                mfxU16  MbaffFrameFlag      :1;
                mfxU16  SecondFieldPicFlag  :1;
            };
            mfxU16  ResColorTransFlag   :1;
            mfxU16  SPforSwitchFlag     :1;
            mfxU16  ChromaFormatIdc     :2;
            mfxU16  RefPicFlag          :1;
            mfxU16  ConstraintIntraFlag :1;
            mfxU16  WeightedPredFlag    :1;
            mfxU16  WeightedBipredIdc   :2;
            mfxU16  MbsConsecutiveFlag  :1;
            mfxU16  FrameMbsOnlyFlag    :1;
            mfxU16  Transform8x8Flag    :1;
            mfxU16  NoMinorBipredFlag   :1;
            mfxU16  IntraPicFlag        :1;
        };
    };

    union {
        mfxU16  ExtraFlags;
        struct {
            mfxU16  EntropyCodingModeFlag   :1;
            mfxU16  Direct8x8InferenceFlag  :1;
            mfxU16  TransCoeffFlag          :1;
            mfxU16  reserved2b              :1;
            mfxU16  ZeroDeltaPicOrderFlag   :1;
            mfxU16  GapsInFrameNumAllowed   :1;
            mfxU16  reserved3c              :2;
            mfxU16  PicOrderPresent         :1;
            mfxU16  RedundantPicCntPresent  :1;
            mfxU16  ScalingListPresent      :1;
            mfxU16  SliceGroupMapPresent    :1;
            mfxU16  ILDBControlPresent      :1;
            mfxU16  MbCodePresent           :1;
            mfxU16  MvDataPresent           :1;
            mfxU16  ResDataPresent          :1;
        };
    };

    mfxI8   PicInitQpMinus26;
    mfxI8   PicInitQsMinus26;

    mfxI8   ChromaQp1stOffset;
    mfxI8   ChromaQp2ndOffset;
    mfxU8   NumRefIdxL0Minus1;
    mfxU8   NumRefIdxL1Minus1;

    mfxU8   BitDepthLumaMinus8;
    mfxU8   BitDepthChromaMinus8;

    mfxU8   Log2MaxFrameCntMinus4;
    mfxU8   PicOrderCntType;

    mfxU8   Log2MaxPicOrdCntMinus4;
    mfxU8   NumSliceGroupMinus1;

    mfxU8   SliceGroupMapType;
    mfxU8   SliceGroupXRateMinus1;

    union {
        mfxU8   RefFrameListP[16];
        mfxU8   RefFrameListB[2][8];
    };

    mfxU32  MinFrameSize;
    mfxU32  MaxFrameSize;
} mfxFrameParamAVC;

/* Frame Parameters for MPEG-2 */
typedef struct {
    mfxU32  CucId;
    mfxU32  CucSz;
    mfxU8   FrameType;
    mfxU8   FrameType2nd;
    mfxU8   reserved1a;
    mfxU8   RecFrameLabel;
    mfxU8   DecFrameLabel;
    mfxU8   VppFrameLabel;
    mfxU16  NumMb;
    mfxU16  FrameWinMbMinus1;
    mfxU16  FrameHinMbMinus1;
    mfxU8   CurrFrameLabel;
    mfxU8   NumRefFrame;

    union {
        mfxU16  CodecFlags;
        struct {
            mfxU16  FieldPicFlag        :1;
            mfxU16  InterlacedFrameFlag :1;
            mfxU16  SecondFieldFlag     :1;
            mfxU16  BottomFieldFlag     :1;
            mfxU16  ChromaFormatIdc     :2;
            mfxU16  RefPicFlag          :1;
            mfxU16  BackwardPredFlag    :1;
            mfxU16  ForwardPredFlag     :1;
            mfxU16  NoResidDiffs        :1;
            mfxU16  reserved3c          :2;
            mfxU16  FrameMbsOnlyFlag    :1;
            mfxU16  BrokenLinkFlag      :1;
            mfxU16  CloseEntryFlag      :1;
            mfxU16  IntraPicFlag        :1;
        };
    };

    union {
        mfxU16  ExtraFlags;
        struct {
            mfxU16  reserved4d          :4;
            mfxU16  MvGridAndChroma     :4;
            mfxU16  reserved5e          :8;
        };
    };

    mfxU16  reserved6f;
    mfxU16  BitStreamFcodes;

    union {
        mfxU16  BitStreamPCEelement;
        struct {
            mfxU16    reserved7g            :3;
            mfxU16    ProgressiveFrame      :1;
            mfxU16    Chroma420type         :1;
            mfxU16    RepeatFirstField      :1;
            mfxU16    AlternateScan         :1;
            mfxU16    IntraVLCformat        :1;
            mfxU16    QuantScaleType        :1;
            mfxU16    ConcealmentMVs        :1;
            mfxU16    FrameDCTprediction    :1;
            mfxU16    TopFieldFirst         :1;
            mfxU16    PicStructure          :2;
            mfxU16    IntraDCprecision      :2;
        };
    };

    mfxU8   BSConcealmentNeed;
    mfxU8   BSConcealmentMethod;
    mfxU16  TemporalReference;
    mfxU32  VBVDelay;

    union{
        mfxU8   RefFrameListP[16];
        mfxU8   RefFrameListB[2][8];
    };

    mfxU32  MinFrameSize;
    mfxU32  MaxFrameSize;
} mfxFrameParamMPEG2;

typedef union {
    mfxFrameParamAVC    AVC;
    mfxFrameParamMPEG2  MPEG2;
} mfxFrameParam;


#ifdef __cplusplus
} // extern "C"
#endif

#endif

