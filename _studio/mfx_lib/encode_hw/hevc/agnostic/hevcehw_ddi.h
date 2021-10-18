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

namespace HEVCEHW
{
typedef struct tagENCODE_CAPS_HEVC
{
    union{
        struct {
            uint32_t CodingLimitSet          : 1;
            uint32_t BitDepth8Only           : 1;
            uint32_t Color420Only            : 1;
            uint32_t SliceStructure          : 3;
            uint32_t SliceIPOnly             : 1;
            uint32_t SliceIPBOnly            : 1;
            uint32_t NoWeightedPred          : 1;
            uint32_t NoMinorMVs              : 1;
            uint32_t RawReconRefToggle       : 1;
            uint32_t NoInterlacedField       : 1;
            uint32_t BRCReset                : 1;
            uint32_t RollingIntraRefresh     : 1;
            uint32_t UserMaxFrameSizeSupport : 1;
            uint32_t FrameLevelRateCtrl      : 1;
            uint32_t SliceByteSizeCtrl       : 1;
            uint32_t VCMBitRateControl       : 1;
            uint32_t ParallelBRC             : 1;
            uint32_t TileSupport             : 1;
            uint32_t SkipFrame               : 1;
            uint32_t MbQpDataSupport         : 1;
            uint32_t SliceLevelWeightedPred  : 1;
            uint32_t LumaWeightedPred        : 1;
            uint32_t ChromaWeightedPred      : 1;
            uint32_t QVBRBRCSupport          : 1;
            uint32_t HMEOffsetSupport        : 1;
            uint32_t YUV422ReconSupport      : 1;
            uint32_t YUV444ReconSupport      : 1;
            uint32_t RGBReconSupport         : 1;
            uint32_t MaxEncodedBitDepth      : 2;
        };
        uint32_t CodingLimits;
    };

    uint32_t MaxPicWidth;
    uint32_t MaxPicHeight;
    uint8_t  MaxNum_Reference0;
    uint8_t  MaxNum_Reference1;
    uint8_t  MBBRCSupport;
    uint8_t  TUSupport;

    union {
        struct {
            uint8_t MaxNumOfROI                : 5; // [0..16]    
            uint8_t ROIBRCPriorityLevelSupport : 1;
            uint8_t BlockSize                  : 2;
        };
        uint8_t ROICaps;
    };

    union {
        struct {
            uint32_t SliceLevelReportSupport         : 1;
            uint32_t CTULevelReportSupport           : 1;
            uint32_t SearchWindow64Support           : 1;
            uint32_t CustomRoundingControl           : 1;
            uint32_t LowDelayBRCSupport              : 1;
            uint32_t IntraRefreshBlockUnitSize       : 2;
            uint32_t LCUSizeSupported                : 3;
            uint32_t MaxNumDeltaQPMinus1             : 4;
            uint32_t DirtyRectSupport                : 1;
            uint32_t MoveRectSupport                 : 1;
            uint32_t FrameSizeToleranceSupport       : 1;
            uint32_t HWCounterAutoIncrementSupport   : 2;
            uint32_t ROIDeltaQPSupport               : 1;
            uint32_t NumScalablePipesMinus1          : 5;
            uint32_t NegativeQPSupport               : 1;
            uint32_t HRDConformanceSupport           : 1;
            uint32_t TileBasedEncodingSupport        : 1;
            uint32_t PartialFrameUpdateSupport       : 1;
            uint32_t RGBEncodingSupport              : 1;
            uint32_t LLCStreamingBufferSupport       : 1;
            uint32_t DDRStreamingBufferSupport       : 1;
        };
        uint32_t CodingLimits2;
    };

    uint8_t  MaxNum_WeightedPredL0;
    uint8_t  MaxNum_WeightedPredL1;
    uint16_t MaxNumOfDirtyRect;
    uint16_t MaxNumOfMoveRect;
    uint16_t MaxNumOfConcurrentFramesMinus1;
    uint16_t LLCSizeInMBytes;

    union {
        struct {
            uint16_t PFrameSupport            : 1;
            uint16_t LookaheadAnalysisSupport : 1;
            uint16_t LookaheadBRCSupport      : 1;
            uint16_t TileReplaySupport        : 1;
            uint16_t TCBRCSupport             : 1;
            uint16_t reservedbits             : 11;
        };
        uint16_t CodingLimits3;
    };

    uint32_t reserved32bits1;
    uint32_t reserved32bits2;
    uint32_t reserved32bits3;

} ENCODE_CAPS_HEVC;
}; //namespace HEVCEHW
