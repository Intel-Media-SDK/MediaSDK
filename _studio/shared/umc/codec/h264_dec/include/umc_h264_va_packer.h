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
#if defined (MFX_ENABLE_H264_VIDEO_DECODE)

#ifndef __UMC_H264_VA_PACKER_H
#define __UMC_H264_VA_PACKER_H

#include "umc_va_base.h"



namespace UMC_H264_DECODER
{
    struct H264PicParamSet;
    struct H264ScalingPicParams;
}

namespace UMC
{

class H264DecoderFrame;
class H264Slice;
class H264DecoderFrameInfo;
struct ReferenceFlags;
class TaskSupplier;

class Packer
{

public:

    Packer(VideoAccelerator * va, TaskSupplier * supplier);
    virtual ~Packer();

    virtual Status GetStatusReport(void * pStatusReport, size_t size) = 0;
    virtual Status SyncTask(H264DecoderFrame*, void * error);
    virtual Status QueryTaskStatus(int32_t index, void * status, void * error);
    virtual Status QueryStreamOut(H264DecoderFrame*);

    VideoAccelerator * GetVideoAccelerator() { return m_va; }

    virtual void PackAU(const H264DecoderFrame*, int32_t isTop) = 0;

    virtual void BeginFrame(H264DecoderFrame*, int32_t field) = 0;
    virtual void EndFrame() = 0;

    virtual void Reset() {}

    static Packer * CreatePacker(VideoAccelerator * va, TaskSupplier* supplier);

private:

    virtual void PackPicParams(H264DecoderFrameInfo * pSliceInfo, H264Slice * pSlice) = 0;

    virtual int32_t PackSliceParams(H264Slice *pSlice, int32_t sliceNum, int32_t chopping, int32_t numSlicesOfPrevField) = 0;

    virtual void PackQmatrix(const UMC_H264_DECODER::H264ScalingPicParams * scaling) = 0;

protected:

    VideoAccelerator *m_va;
    TaskSupplier     *m_supplier;
};




class PackerVA
    : public Packer
{
public:
    PackerVA(VideoAccelerator * va, TaskSupplier * supplier);

    Status GetStatusReport(void * pStatusReport, size_t size);
    Status QueryStreamOut(H264DecoderFrame*);

    void PackAU(const H264DecoderFrame*, int32_t isTop);

    void BeginFrame(H264DecoderFrame*, int32_t field);
    void EndFrame();

protected:

    void PackPicParams(H264DecoderFrameInfo * pSliceInfo, H264Slice * pSlice);
    void PackPriorityParams();

    void FillFrame(VAPictureH264 * pic, const H264DecoderFrame *pFrame,
        int32_t field, int32_t reference, int32_t defaultIndex);

    int32_t FillRefFrame(VAPictureH264 * pic, const H264DecoderFrame *pFrame,
        ReferenceFlags flags, bool isField, int32_t defaultIndex);

    void FillFrameAsInvalid(VAPictureH264 * pic);

    void PackProcessingInfo(H264DecoderFrameInfo * sliceInfo);

    enum
    {
        VA_FRAME_INDEX_INVALID = 0x7f
    };

protected:

    void CreateSliceParamBuffer(H264DecoderFrameInfo * pSliceInfo);
    void CreateSliceDataBuffer(H264DecoderFrameInfo * pSliceInfo);

    int32_t PackSliceParams(H264Slice *pSlice, int32_t sliceNum, int32_t chopping, int32_t numSlicesOfPrevField);

    void PackQmatrix(const UMC_H264_DECODER::H264ScalingPicParams * scaling);

private:

    bool                       m_enableStreamOut;
};

#ifdef MFX_ENABLE_CPLIB
class PackerVA_CENC
    : public PackerVA
{

public:

    PackerVA_CENC(VideoAccelerator * va, TaskSupplier * supplier);

private:

    void PackPicParams(H264DecoderFrameInfo * pSliceInfo, H264Slice * pSlice) override;
    void PackAU(const H264DecoderFrame*, int32_t isTop) override;
};
#endif

} // namespace UMC

#endif /* __UMC_H264_VA_PACKER_H */
#endif // MFX_ENABLE_H264_VIDEO_DECODE
