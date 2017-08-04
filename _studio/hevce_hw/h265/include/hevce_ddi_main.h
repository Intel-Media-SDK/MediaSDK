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
#pragma once


typedef struct tagENCODE_CAPS_HEVC
{
    union{
        struct {
            UINT    CodingLimitSet          : 1;
            UINT    BitDepth8Only           : 1;
            UINT    Color420Only            : 1;
            UINT    SliceStructure          : 3;
            UINT    SliceIPOnly             : 1;
            UINT    SliceIPBOnly            : 1;
            UINT    NoWeightedPred          : 1;
            UINT    NoMinorMVs              : 1;
            UINT    RawReconRefToggle       : 1;
            UINT    NoInterlacedField       : 1;
            UINT    BRCReset                : 1;
            UINT    RollingIntraRefresh     : 1;
            UINT    UserMaxFrameSizeSupport : 1;
            UINT    FrameLevelRateCtrl      : 1;
            UINT    SliceByteSizeCtrl       : 1;
            UINT    VCMBitRateControl       : 1;
            UINT    ParallelBRC             : 1;
            UINT    TileSupport             : 1;
            UINT    SkipFrame               : 1;
            UINT    MbQpDataSupport         : 1;
            UINT    SliceLevelWeightedPred  : 1;
            UINT    LumaWeightedPred        : 1;
            UINT    ChromaWeightedPred      : 1;
            UINT    QVBRBRCSupport          : 1;
            UINT    HMEOffsetSupport        : 1;
            UINT    YUV422ReconSupport      : 1;
            UINT    YUV444ReconSupport      : 1;
            UINT    RGBReconSupport         : 1;
            UINT    MaxEncodedBitDepth      : 2;
        };
        UINT    CodingLimits;
    };

    UINT    MaxPicWidth;
    UINT    MaxPicHeight;
    UCHAR   MaxNum_Reference0;
    UCHAR   MaxNum_Reference1;
    UCHAR   MBBRCSupport;
    UCHAR   TUSupport;

    union {
        struct {
            UCHAR    MaxNumOfROI                : 5; // [0..16]    
            UCHAR    ROIBRCPriorityLevelSupport : 1;
            UCHAR    BlockSize                  : 2;
        };
        UCHAR    ROICaps;
    };

    union {
        struct {
            UINT    SliceLevelReportSupport         : 1;
            UINT    MaxNumOfTileColumnsMinus1       : 4;
            UINT    IntraRefreshBlockUnitSize       : 2;
            UINT    LCUSizeSupported                : 3;
            UINT    MaxNumDeltaQP                   : 4;
            UINT    DirtyRectSupport                : 1;
            UINT    MoveRectSupport                 : 1;
            UINT    FrameSizeToleranceSupport       : 1;
            UINT    HWCounterAutoIncrementSupport   : 2;
            UINT    ROIDeltaQPSupport               : 1;
            UINT                                    : 12; // For future expansion
        };
        UINT    CodingLimits2;
    };

    UCHAR    MaxNum_WeightedPredL0;
    UCHAR    MaxNum_WeightedPredL1;
} ENCODE_CAPS_HEVC;

