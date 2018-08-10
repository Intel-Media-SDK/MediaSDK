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

#ifndef __UMC_H264_VA_SUPPLIER_H
#define __UMC_H264_VA_SUPPLIER_H

#include "umc_h264_dec_mfx.h"
#include "umc_h264_mfx_supplier.h"
#include "umc_h264_segment_decoder_dxva.h"

namespace UMC
{

class MFXVideoDECODEH264;

class LazyCopier
{
public:
    void Reset();

    void Add(H264Slice * slice);
    void Remove(H264DecoderFrameInfo * info);
    void Remove(H264Slice * slice);
    void CopyAll();

private:
    typedef std::list<H264Slice *> SlicesList;
    SlicesList m_slices;
};

/****************************************************************************************************/
// TaskSupplier
/****************************************************************************************************/
class VATaskSupplier :
          public MFXTaskSupplier
        , public DXVASupport<VATaskSupplier>
{
    friend class TaskBroker;
    friend class DXVASupport<VATaskSupplier>;
    friend class VideoDECODEH264;

public:

    VATaskSupplier();

    virtual Status Init(VideoDecoderParams *pInit);

    virtual void Close();
    virtual void Reset();

    void SetBufferedFramesNumber(uint32_t buffered);

    virtual Status DecodeHeaders(NalUnit *nalUnit);

    virtual Status AddSource(MediaData * pSource);

protected:

    virtual void CreateTaskBroker();

    virtual Status AllocateFrameData(H264DecoderFrame * pFrame);

    virtual void InitFrameCounter(H264DecoderFrame * pFrame, const H264Slice *pSlice);

    virtual Status CompleteFrame(H264DecoderFrame * pFrame, int32_t field);

    virtual void AfterErrorRestore();

    virtual H264DecoderFrame * GetFreeFrame(const H264Slice *pSlice = NULL);

    virtual H264Slice * DecodeSliceHeader(NalUnit *nalUnit);

    virtual H264DecoderFrame *GetFrameToDisplayInternal(bool force);

protected:

    virtual int32_t GetFreeFrameIndex();

    uint32_t m_bufferedFrameNumber;
    LazyCopier m_lazyCopier;

private:
    VATaskSupplier & operator = (VATaskSupplier &)
    {
        return *this;
    }
};

// this template class added to apply big surface pool workaround depends on platform
// because platform check can't be added inside VATaskSupplier
template <class BaseClass>
class VATaskSupplierBigSurfacePool :
    public BaseClass
{
public:
    VATaskSupplierBigSurfacePool()
    {};
    virtual ~VATaskSupplierBigSurfacePool()
    {};

protected:

    virtual Status AllocateFrameData(H264DecoderFrame * pFrame)
    {
        Status ret = BaseClass::AllocateFrameData(pFrame);
        if (ret == UMC_OK)
        {
            pFrame->m_index = BaseClass::GetFreeFrameIndex();
        }

        return ret;
    }
};
} // namespace UMC

#endif // __UMC_H264_VA_SUPPLIER_H
#endif // MFX_ENABLE_H264_VIDEO_DECODE
