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


GUID GetGUID(MfxVideoParam const & par);

mfxStatus HardcodeCaps(ENCODE_CAPS_HEVC& caps, MFXCoreInterface* pCore);

class DriverEncoder;

typedef enum tagENCODER_TYPE
{
    ENCODER_DEFAULT = 0
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
