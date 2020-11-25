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

#include "umc_defs.h"
#ifdef MFX_ENABLE_VP9_VIDEO_DECODE

#ifndef __UMC_VP9_VA_PACKER_H
#define __UMC_VP9_VA_PACKER_H

#include "umc_va_base.h"


    #include "va/va_dec_vp9.h"

namespace UMC
{ class MediaData; }

namespace UMC_VP9_DECODER
{
    class VP9Bitstream;
    class VP9DecoderFrame;

class Packer
{

public:

    Packer(UMC::VideoAccelerator * va);
    virtual ~Packer();

    virtual UMC::Status GetStatusReport(void* pStatusReport, size_t size) = 0;

    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;

    virtual void PackAU(VP9Bitstream*, VP9DecoderFrame const*) = 0;

    static Packer* CreatePacker(UMC::VideoAccelerator * va);

protected:

    UMC::VideoAccelerator *m_va;
};




class PackerVA
    : public Packer
{

public:

    PackerVA(UMC::VideoAccelerator * va);

    UMC::Status GetStatusReport(void * pStatusReport, size_t size);

    void BeginFrame();
    void EndFrame();

    void PackAU(VP9Bitstream*, VP9DecoderFrame const*);

 private:

    void PackPicParams(VADecPictureParameterBufferVP9*, VP9DecoderFrame const*);
    void PackSliceParams(VASliceParameterBufferVP9*, VP9DecoderFrame const*);
    void PackPriorityParams();
};


} // namespace UMC_HEVC_DECODER

#endif /* __UMC_VP9_VA_PACKER_H */
#endif // MFX_ENABLE_VP9_VIDEO_DECODE
