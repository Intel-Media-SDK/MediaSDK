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

#include "umc_defs.h"
#ifdef MFX_ENABLE_H265_VIDEO_DECODE

#ifndef __UMC_H265_MFX_SUPPLIER_H
#define __UMC_H265_MFX_SUPPLIER_H

#include "umc_h265_task_supplier.h"
#include "umc_media_data_ex.h"

#include "umc_h265_task_broker.h"
#include "mfxvideo++int.h"

class VideoDECODEH265;

namespace UMC_HEVC_DECODER
{

// Container for raw header binary data
class RawHeader_H265
{
public:

    RawHeader_H265();

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

// Container for raw SPS and PPS stream headers
class RawHeaders_H265
{
public:

    void Reset();

    RawHeader_H265 * GetVPS();

    RawHeader_H265 * GetSPS();

    RawHeader_H265 * GetPPS();

protected:

    RawHeader_H265 m_vps;
    RawHeader_H265 m_sps;
    RawHeader_H265 m_pps;
};

/****************************************************************************************************/
// Task supplier which implements MediaSDK decoding API
/****************************************************************************************************/
class MFXTaskSupplier_H265 : public TaskSupplier_H265, public RawHeaders_H265
{
    friend class ::VideoDECODEH265;

public:

    MFXTaskSupplier_H265();

    virtual ~MFXTaskSupplier_H265();

    // Initialize task supplier
    virtual UMC::Status Init(UMC::VideoDecoderParams *pInit);

    using TaskSupplier_H265::Reset;

    // Check whether all slices for the frame were found
    virtual void CompleteFrame(H265DecoderFrame * pFrame);

    // Check whether specified frame has been decoded, and if it was,
    // whether there is some decoding work left to be done
    bool CheckDecoding(bool should_additional_check, H265DecoderFrame * decoded);

    // Set initial video params from application
    void SetVideoParams(mfxVideoParam * par);

    // Initialize mfxVideoParam structure based on decoded bitstream header values
    UMC::Status FillVideoParam(mfxVideoParam *par, bool full);

protected:

    // Decode SEI nal unit
    virtual UMC::Status DecodeSEI(UMC::MediaDataEx *nalUnit);

    // Do something in case reference frame is missing
    virtual void AddFakeReferenceFrame(H265Slice * pSlice);

    // Decode headers nal unit
    virtual UMC::Status DecodeHeaders(UMC::MediaDataEx *nalUnit);

    // Perform decoding task for thread number threadNumber
    mfxStatus RunThread(mfxU32 threadNumber);

    mfxVideoParam  m_firstVideoParams;

private:
    MFXTaskSupplier_H265 & operator = (MFXTaskSupplier_H265 &)
    {
        return *this;

    } // MFXTaskSupplier_H265 & operator = (MFXTaskSupplier_H265 &)
};

} // namespace UMC_HEVC_DECODER

#endif // __UMC_H265_MFX_SUPPLIER_H
#endif // MFX_ENABLE_H265_VIDEO_DECODE
