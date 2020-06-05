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

#include "umc_defs.h"
#ifdef MFX_ENABLE_H265_VIDEO_DECODE

#ifndef __UMC_H265_VA_SUPPLIER_H
#define __UMC_H265_VA_SUPPLIER_H

#include "umc_h265_mfx_supplier.h"
#include "umc_h265_segment_decoder_dxva.h"

namespace UMC_HEVC_DECODER
{

class MFXVideoDECODEH265;

/****************************************************************************************************/
// TaskSupplier_H265
/****************************************************************************************************/
class VATaskSupplier :
          public MFXTaskSupplier_H265
        , public DXVASupport<VATaskSupplier>
{
    friend class TaskBroker_H265;
    friend class DXVASupport<VATaskSupplier>;
    friend class VideoDECODEH265;

public:

    VATaskSupplier();

    virtual UMC::Status Init(UMC::VideoDecoderParams *pInit);

    virtual void Reset();

    virtual void CreateTaskBroker();

    void SetBufferedFramesNumber(uint32_t buffered);


protected:
    virtual UMC::Status AllocateFrameData(H265DecoderFrame * pFrame, mfxSize dimensions, const H265SeqParamSet* pSeqParamSet, const H265PicParamSet *pPicParamSet);

    virtual void InitFrameCounter(H265DecoderFrame * pFrame, const H265Slice *pSlice);

    virtual void CompleteFrame(H265DecoderFrame * pFrame);

    virtual H265Slice * DecodeSliceHeader(UMC::MediaDataEx *nalUnit);

    virtual H265DecoderFrame *GetFrameToDisplayInternal(bool force);

    uint32_t m_bufferedFrameNumber;

private:
    VATaskSupplier & operator = (VATaskSupplier &)
    {
        return *this;
    }

    const int SliceHeaderSize = DEFAULT_MAX_ENETRY_POINT_NUM * 4 + DEFAULT_MAX_PREVENTION_BYTES + DEAFULT_MAX_SLICE_HEADER_SIZE;
};

// this template class added to apply big surface pool workaround depends on platform
// because platform check can't be added inside VATaskSupplier
template <class BaseClass>
class VATaskSupplierBigSurfacePool:
    public BaseClass
{
public:
    VATaskSupplierBigSurfacePool()
    {};
    virtual ~VATaskSupplierBigSurfacePool()
    {};

protected:

    virtual UMC::Status AllocateFrameData(H265DecoderFrame * pFrame, mfxSize dimensions, const H265SeqParamSet* pSeqParamSet, const H265PicParamSet * pps)
    {
        UMC::Status ret = BaseClass::AllocateFrameData(pFrame, dimensions, pSeqParamSet, pps);

        if (ret == UMC::UMC_OK)
        {
            ViewItem_H265 *pView = BaseClass::GetView();
            H265DBPList *pDPB = pView->pDPB.get();

            pFrame->m_index = pDPB->GetFreeIndex();
        }

        return ret;
    }
};

}// namespace UMC_HEVC_DECODER


#endif // __UMC_H265_VA_SUPPLIER_H
#endif // MFX_ENABLE_H265_VIDEO_DECODE
