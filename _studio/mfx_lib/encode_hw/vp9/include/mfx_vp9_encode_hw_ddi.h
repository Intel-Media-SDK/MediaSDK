// Copyright (c) 2018-2019 Intel Corporation
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

#include "mfx_common.h"
#if defined MFX_VA_LINUX
#include "mfx_h264_encode_struct_vaapi.h"
#endif
#include "mfx_vp9_encode_hw_utils.h"
#include "mfx_platform_headers.h"

namespace MfxHwVP9Encode
{

static const GUID DXVA2_Intel_Encode_VP9_Profile0 =
{ 0x464bdb3c, 0x91c4, 0x4e9b, { 0x89, 0x6f, 0x22, 0x54, 0x96, 0xac, 0x4e, 0xd6 } };
static const GUID DXVA2_Intel_Encode_VP9_Profile1 =
{ 0xafe4285c, 0xab63, 0x4b2d, { 0x82, 0x78, 0xe6, 0xba, 0xac, 0xea, 0x2c, 0xe9 } };
static const GUID DXVA2_Intel_Encode_VP9_10bit_Profile2 =
{ 0x4c5ba10, 0x4e9a, 0x4b8e, { 0x8d, 0xbf, 0x4f, 0x4b, 0x48, 0xaf, 0xa2, 0x7c } };
static const GUID DXVA2_Intel_Encode_VP9_10bit_Profile3 =
{ 0x24d19fca, 0xc5a2, 0x4b8e, { 0x9f, 0x93, 0xf8, 0xf6, 0xef, 0x15, 0xc8, 0x90 } };

static const GUID DXVA2_Intel_LowpowerEncode_VP9_Profile0 =
{ 0x9b31316b, 0xf204, 0x455d, { 0x8a, 0x8c, 0x93, 0x45, 0xdc, 0xa7, 0x7c, 0x1 } };
static const GUID DXVA2_Intel_LowpowerEncode_VP9_Profile1 =
{ 0x277de9c5, 0xed83, 0x48dd, {0xab, 0x8f, 0xac, 0x2d, 0x24, 0xb2, 0x29, 0x43 } };
static const GUID DXVA2_Intel_LowpowerEncode_VP9_10bit_Profile2 =
{ 0xacef8bc, 0x285f, 0x415d, {0xab, 0x22, 0x7b, 0xf2, 0x52, 0x7a, 0x3d, 0x2e } };
static const GUID DXVA2_Intel_LowpowerEncode_VP9_10bit_Profile3 =
{ 0x353aca91, 0xd945, 0x4c13, {0xae, 0x7e, 0x46, 0x90, 0x60, 0xfa, 0xc8, 0xd8 } };

GUID GetGuid(VP9MfxVideoParam const & par);

#define DDI_FROM_MAINLINE_DRIVER

typedef struct tagENCODE_CAPS_VP9
{
    union
    {
        struct
        {
            UINT    CodingLimitSet            : 1;
            UINT    Color420Only              : 1;
            UINT    ForcedSegmentationSupport : 1;
            UINT    FrameLevelRateCtrl        : 1;
            UINT    BRCReset                  : 1;
            UINT    AutoSegmentationSupport   : 1;
            UINT    TemporalLayerRateCtrl     : 3;
            UINT    DynamicScaling            : 1;
            UINT    TileSupport               : 1;
            UINT    NumScalablePipesMinus1    : 3;
            UINT    YUV422ReconSupport        : 1;
            UINT    YUV444ReconSupport        : 1;
            UINT    MaxEncodedBitDepth        : 2;
            UINT    UserMaxFrameSizeSupport   : 1;
            UINT    SegmentFeatureSupport     : 4;
            UINT    DirtyRectSupport          : 1;
            UINT    MoveRectSupport           : 1;
            UINT                              : 7;

        };

        UINT        CodingLimits;
    };

    union
    {
        struct
        {
            UCHAR   EncodeFunc            : 1; // CHV+
            UCHAR   HybridPakFunc         : 1; // HSW/BDW hybrid ENC & PAK
            UCHAR   EncFunc : 1; // BYT enc only mode
        UCHAR: 5;
        };

        UCHAR       CodingFunction;
    };

    UINT            MaxPicWidth;
    UINT            MaxPicHeight;
    USHORT          MaxNumOfDirtyRect;
    USHORT          MaxNumOfMoveRect;


} ENCODE_CAPS_VP9;

    class DriverEncoder;

    mfxStatus QueryCaps(VideoCORE * pCore, ENCODE_CAPS_VP9 & caps, GUID guid, VP9MfxVideoParam const & par);

    DriverEncoder* CreatePlatformVp9Encoder(VideoCORE * pCore);

    class DriverEncoder
    {
    public:

        virtual ~DriverEncoder(){}

        virtual mfxStatus CreateAuxilliaryDevice(
                        VideoCORE * pCore,
                        GUID        guid,
                        VP9MfxVideoParam const & par) = 0;

        virtual
        mfxStatus CreateAccelerationService(
            VP9MfxVideoParam const & par) = 0;

        virtual
        mfxStatus Reset(
            VP9MfxVideoParam const & par) = 0;

        virtual
        mfxStatus Register(
            mfxFrameAllocResponse & response,
            D3DDDIFORMAT            type) = 0;

        virtual
        mfxStatus Execute(
            Task const &task,
            mfxHDLPair pair) = 0;

        virtual
        mfxStatus QueryCompBufferInfo(
            D3DDDIFORMAT           type,
            mfxFrameAllocRequest & request,
            mfxU32 frameWidth,
            mfxU32 frameHeight) = 0;

        virtual
        mfxStatus QueryEncodeCaps(
            ENCODE_CAPS_VP9 & caps) = 0;

        virtual
        mfxStatus QueryStatus(
            Task & task ) = 0;

        virtual
            mfxU32 GetReconSurfFourCC() = 0;

        virtual
        mfxStatus Destroy() = 0;
    };

#define VP9_MAX_UNCOMPRESSED_HEADER_SIZE 1000
#define SWITCHABLE 4 /* should be the last one */

    enum {
        LAST_FRAME   = 0,
        GOLDEN_FRAME = 1,
        ALTREF_FRAME = 2
    };

    struct BitOffsets
    {
        mfxU16 BitOffsetUncompressedHeader;
        mfxU16 BitOffsetForLFRefDelta;
        mfxU16 BitOffsetForLFModeDelta;
        mfxU16 BitOffsetForLFLevel;
        mfxU16 BitOffsetForQIndex;
        mfxU16 BitOffsetForFirstPartitionSize;
        mfxU16 BitOffsetForSegmentation;
        mfxU16 BitSizeForSegmentation;
    };

    inline mfxStatus AddSeqHeader(mfxU32 width,
        mfxU32 height,
        mfxU32 FrameRateN,
        mfxU32 FrameRateD,
        mfxU32 numFramesInFile,
        mfxU8* pBitstream,
        mfxU32 bufferSize)
    {
        mfxU32   ivf_file_header[8] = { 0x46494B44, 0x00200000, 0x30395056, width + (height << 16), FrameRateN, FrameRateD, numFramesInFile, 0x00000000 };
        if (bufferSize < sizeof(ivf_file_header))
            return MFX_ERR_MORE_DATA;

        std::copy(std::begin(ivf_file_header),std::end(ivf_file_header), reinterpret_cast<mfxU32*>(pBitstream));

        return MFX_ERR_NONE;
    };

    inline mfxStatus AddPictureHeader(mfxU8* pBitstream, mfxU32 bufferSize)
    {
        mfxU32 number_of_zero_bytes = 3*sizeof(mfxU32);

        if (bufferSize < number_of_zero_bytes)
                return MFX_ERR_MORE_DATA;

        std::fill_n(pBitstream, number_of_zero_bytes, 0);

        return MFX_ERR_NONE;
    };

    inline ENCODE_PACKEDHEADER_DATA MakePackedByteBuffer(mfxU8 * data, mfxU32 size)
    {
        ENCODE_PACKEDHEADER_DATA desc = {};
        desc.pData = data;
        desc.BufferSize = size;
        desc.DataLength = size;
        return desc;
    }

    struct BitBuffer {
        mfxU8 *pBuffer;
        mfxU16 bitOffset;
    };

    mfxU16 WriteUncompressedHeader(BitBuffer &buffer,
                                   Task const &task,
                                   VP9SeqLevelParam const &seqPar,
                                   BitOffsets &offsets);

    mfxU16 PrepareFrameHeader(VP9MfxVideoParam const &par,
                              mfxU8 *pBuf,
                              mfxU32 bufferSizeBytes,
                              Task const& task,
                              VP9SeqLevelParam const &seqPar,
                              BitOffsets &offsets);
} // MfxHwVP9Encode
