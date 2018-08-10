// Copyright (c) 2018 Intel Corporation
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
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE)

#include "mfx_h264_encode_struct_vaapi.h"
#include "mfxplugin++.h"

#include "mfx_h265_encode_hw_utils.h"
#include "mfx_h265_encode_hw_bs.h"
#include <memory>
#include <vector>

//#define HEADER_PACKING_TEST

#define D3DDDIFMT_NV12 (D3DDDIFORMAT)(MFX_MAKEFOURCC('N', 'V', '1', '2'))
#define D3DDDIFMT_YU12 (D3DDDIFORMAT)(MFX_MAKEFOURCC('Y', 'U', '1', '2'))

namespace MfxHwH265Encode
{

// GUIDs from DDI for HEVC Encoder spec 0.956
static const GUID DXVA2_Intel_Encode_HEVC_Main =
{ 0x28566328, 0xf041, 0x4466, { 0x8b, 0x14, 0x8f, 0x58, 0x31, 0xe7, 0x8f, 0x8b } };

static const GUID DXVA2_Intel_Encode_HEVC_Main10 =
{ 0x6b4a94db, 0x54fe, 0x4ae1, { 0x9b, 0xe4, 0x7a, 0x7d, 0xad, 0x00, 0x46, 0x00 } };

static const GUID DXVA2_Intel_LowpowerEncode_HEVC_Main =
{ 0xb8b28e0c, 0xecab, 0x4217, { 0x8c, 0x82, 0xea, 0xaa, 0x97, 0x55, 0xaa, 0xf0 } };
static const GUID DXVA2_Intel_LowpowerEncode_HEVC_Main10 =
{ 0x8732ecfd, 0x9747, 0x4897, { 0xb4, 0x2a, 0xe5, 0x34, 0xf9, 0xff, 0x2b, 0x7a } };

static const GUID DXVA2_Intel_Encode_HEVC_Main422 =
{ 0x056a6e36, 0xf3a8, 0x4d00, { 0x96, 0x63, 0x7e, 0x94, 0x30, 0x35, 0x8b, 0xf9 } };
static const GUID DXVA2_Intel_Encode_HEVC_Main422_10 =
{ 0xe139b5ca, 0x47b2, 0x40e1, { 0xaf, 0x1c, 0xad, 0x71, 0xa6, 0x7a, 0x18, 0x36 } };
static const GUID DXVA2_Intel_Encode_HEVC_Main444 =
{ 0x5415a68c, 0x231e, 0x46f4, { 0x87, 0x8b, 0x5e, 0x9a, 0x22, 0xe9, 0x67, 0xe9 } };
static const GUID DXVA2_Intel_Encode_HEVC_Main444_10 =
{ 0x161be912, 0x44c2, 0x49c0, { 0xb6, 0x1e, 0xd9, 0x46, 0x85, 0x2b, 0x32, 0xa1 } };
static const GUID DXVA2_Intel_LowpowerEncode_HEVC_Main422 =
{ 0xcee393ab, 0x1030, 0x4f7b, { 0x8d, 0xbc, 0x55, 0x62, 0x9c, 0x72, 0xf1, 0x7e } };
static const GUID DXVA2_Intel_LowpowerEncode_HEVC_Main422_10 =
{ 0x580da148, 0xe4bf, 0x49b1, { 0x94, 0x3b, 0x42, 0x14, 0xab, 0x05, 0xa6, 0xff } };
static const GUID DXVA2_Intel_LowpowerEncode_HEVC_Main444 =
{ 0x87b2ae39, 0xc9a5, 0x4c53, { 0x86, 0xb8, 0xa5, 0x2d, 0x7e, 0xdb, 0xa4, 0x88 } };
static const GUID DXVA2_Intel_LowpowerEncode_HEVC_Main444_10 =
{ 0x10e19ac8, 0xbf39, 0x4443, { 0xbe, 0xc3, 0x1b, 0x0c, 0xbf, 0xe4, 0xc7, 0xaa } };

GUID GetGUID(MfxVideoParam const & par);

const GUID GuidTable[3][3][3] =
{
    // LowPower = OFF
    {
        // BitDepthLuma = 8
        {
            /*420*/ DXVA2_Intel_Encode_HEVC_Main,
            /*422*/ DXVA2_Intel_Encode_HEVC_Main422,
            /*444*/ DXVA2_Intel_Encode_HEVC_Main444
        },
        // BitDepthLuma = 10
        {
            /*420*/ DXVA2_Intel_Encode_HEVC_Main10,
            /*422*/ DXVA2_Intel_Encode_HEVC_Main422_10,
            /*444*/ DXVA2_Intel_Encode_HEVC_Main444_10
        },
        // BitDepthLuma = 12
        {
        }
    },
    // LowPower = ON
    {
        // BitDepthLuma = 8
        {
            /*420*/ DXVA2_Intel_LowpowerEncode_HEVC_Main,
            /*422*/ DXVA2_Intel_LowpowerEncode_HEVC_Main422,
            /*444*/ DXVA2_Intel_LowpowerEncode_HEVC_Main444
        },
        // BitDepthLuma = 10
        {
            /*420*/ DXVA2_Intel_LowpowerEncode_HEVC_Main10,
            /*422*/ DXVA2_Intel_LowpowerEncode_HEVC_Main422_10,
            /*444*/ DXVA2_Intel_LowpowerEncode_HEVC_Main444_10
        },
        // BitDepthLuma = 12
        {
        }
    }
};

mfxStatus HardcodeCaps(ENCODE_CAPS_HEVC& caps, MFXCoreInterface* pCore);

class DriverEncoder;

typedef enum tagENCODER_TYPE
{
    ENCODER_DEFAULT = 0
    , ENCODER_REXT
} ENCODER_TYPE;

DriverEncoder* CreatePlatformH265Encoder(MFXCoreInterface* core, ENCODER_TYPE type = ENCODER_DEFAULT);
mfxStatus QueryHwCaps(MFXCoreInterface* core, GUID guid, ENCODE_CAPS_HEVC & caps);
mfxStatus CheckHeaders(MfxVideoParam const & par, ENCODE_CAPS_HEVC const & caps);

mfxStatus FillCUQPDataDDI(Task& task, MfxVideoParam &par, MFXCoreInterface& core, mfxFrameInfo &CUQPFrameInfo);

enum
{
    MAX_DDI_BUFFERS = 4, //sps, pps, slice, bs
};

class DriverEncoder
{
public:

    virtual ~DriverEncoder(){}

    virtual
    mfxStatus CreateAuxilliaryDevice(
                    MFXCoreInterface * core,
                    GUID        guid,
                    mfxU32      width,
                    mfxU32      height) = 0;

    virtual
    mfxStatus CreateAccelerationService(
        MfxVideoParam const & par) = 0;

    virtual
    mfxStatus Reset(
        MfxVideoParam const & par, bool bResetBRC) = 0;

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
        mfxFrameAllocRequest & request) = 0;

    virtual
    mfxStatus QueryEncodeCaps(
        ENCODE_CAPS_HEVC & caps) = 0;


    virtual
    mfxStatus QueryStatus(
        Task & task ) = 0;

    virtual
    mfxStatus Destroy() = 0;

    virtual
    ENCODE_PACKEDHEADER_DATA* PackHeader(Task const & task, mfxU32 nut) = 0;

    // Functions below are used in HEVC FEI encoder

    virtual mfxStatus PreSubmitExtraStage(Task const & /*task*/)
    {
        return MFX_ERR_NONE;
    }

    virtual mfxStatus PostQueryExtraStage(Task const & /* task */, mfxU32 /* codedStatus */)
    {
        return MFX_ERR_NONE;
    }
};

class DDIHeaderPacker
{
public:
    DDIHeaderPacker();
    ~DDIHeaderPacker();

    void Reset(MfxVideoParam const & par);

    void ResetPPS(MfxVideoParam const & par);

    ENCODE_PACKEDHEADER_DATA* PackHeader(Task const & task, mfxU32 nut);
    ENCODE_PACKEDHEADER_DATA* PackSliceHeader(Task const & task,
        mfxU32 id,
        mfxU32* qpd_offset,
        bool dyn_slice_size = false,
        mfxU32* sao_offset = 0,
        mfxU16* ssh_start_len = 0,
        mfxU32* ssh_offset = 0,
        mfxU32* pwt_offset = 0,
        mfxU32* pwt_length = 0);
    ENCODE_PACKEDHEADER_DATA* PackSkippedSlice(Task const & task, mfxU32 id, mfxU32* qpd_offset);

    inline mfxU32 MaxPackedHeaders() { return (mfxU32)m_buf.size(); }

private:
    std::vector<ENCODE_PACKEDHEADER_DATA>           m_buf;
    std::vector<ENCODE_PACKEDHEADER_DATA>::iterator m_cur;
    HeaderPacker m_packer;

    void NewHeader();
};


}; // namespace MfxHwH265Encode
#endif
