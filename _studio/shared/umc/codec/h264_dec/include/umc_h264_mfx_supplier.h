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

#include "umc_defs.h"
#if defined (MFX_ENABLE_H264_VIDEO_DECODE)

#ifndef __UMC_H264_MFX_SUPPLIER_H
#define __UMC_H264_MFX_SUPPLIER_H

#include "umc_h264_task_supplier.h"
#include "umc_media_data_ex.h"
#include "umc_h264_task_broker.h"
#include "mfxvideo++int.h"

class VideoDECODEH264;

namespace UMC
{

class RawHeader
{
public:

    RawHeader();

    void Reset();

    int32_t GetID() const;

    size_t GetSize() const;

    uint8_t * GetPointer();

    void Resize(int32_t id, size_t newSize);


protected:
    typedef std::vector<uint8_t> BufferType;
    BufferType  m_buffer;
    int32_t      m_id;
};

class RawHeaders
{
public:

    void Reset();

    RawHeader * GetSPS();

    RawHeader * GetPPS();

protected:

    RawHeader m_sps;
    RawHeader m_pps;
};

/****************************************************************************************************/
// TaskSupplier
/****************************************************************************************************/
class MFXTaskSupplier : public TaskSupplier, public RawHeaders
{
    friend class ::VideoDECODEH264;

public:

    MFXTaskSupplier();

    virtual ~MFXTaskSupplier();

    virtual Status Init(VideoDecoderParams *pInit);

    virtual void Reset();

    virtual Status CompleteFrame(H264DecoderFrame * pFrame, int32_t field);

    virtual bool ProcessNonPairedField(H264DecoderFrame * pFrame);

    bool CheckDecoding(H264DecoderFrame * decoded);

    void SetVideoParams(mfxVideoParam * par);

protected:

    virtual Status DecodeSEI(NalUnit *nalUnit);

    virtual void AddFakeReferenceFrame(H264Slice * pSlice);

    virtual Status DecodeHeaders(NalUnit *nalUnit);

    mfxStatus RunThread(mfxU32 threadNumber);

    mfxVideoParam  m_firstVideoParams;

private:
    MFXTaskSupplier & operator = (MFXTaskSupplier &)
    {
        return *this;
    } // MFXTaskSupplier & operator = (MFXTaskSupplier &)
};

inline uint32_t ExtractProfile(mfxU32 profile)
{
    return profile & 0xFF;
}

inline bool isMVCProfile(mfxU32 profile)
{
    return (profile == MFX_PROFILE_AVC_MULTIVIEW_HIGH || profile == MFX_PROFILE_AVC_STEREO_HIGH);
}


} // namespace UMC


class MFX_Utility
{
public:

    static eMFXPlatform GetPlatform(VideoCORE * core, mfxVideoParam * par);
    static UMC::Status FillVideoParam(UMC::TaskSupplier * supplier, ::mfxVideoParam *par, bool full);
    static UMC::Status FillVideoParamMVCEx(UMC::TaskSupplier * supplier, ::mfxVideoParam *par);
    static UMC::Status DecodeHeader(UMC::TaskSupplier * supplier, UMC::H264VideoDecoderParams* params, mfxBitstream *bs, mfxVideoParam *out);

    static mfxStatus Query(VideoCORE *core, mfxVideoParam *in, mfxVideoParam *out, eMFXHWType type);
    static bool CheckVideoParam(mfxVideoParam *in, eMFXHWType type);

private:

    static bool IsNeedPartialAcceleration(mfxVideoParam * par, eMFXHWType type);

};


#endif // __UMC_H264_MFX_SUPPLIER_H
#endif // MFX_ENABLE_H264_VIDEO_DECODE
