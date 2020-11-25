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

#include "umc_defs.h"

#if defined (MFX_ENABLE_MPEG2_VIDEO_DECODE)

#include "umc_va_base.h"

#include <va/va.h>

namespace UMC
{ class MediaData; }

namespace UMC_MPEG2_DECODER
{
    class MPEG2DecoderFrameInfo;
    class MPEG2DecoderFrame;
    class Packer
    {

    public:

        Packer(UMC::VideoAccelerator * va);
        virtual ~Packer();

        virtual UMC::Status GetStatusReport(void* pStatusReport, size_t size) = 0;
        virtual UMC::Status SyncTask(MPEG2DecoderFrame*, void * error) = 0;
        virtual UMC::Status QueryTaskStatus(uint32_t index, void * status, void * error) = 0;

        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;

        virtual void PackAU(MPEG2DecoderFrame const&, uint8_t) = 0;

        static Packer* CreatePacker(UMC::VideoAccelerator * va);

    protected:
        virtual void PackPicParams(const MPEG2DecoderFrame&, uint8_t) = 0;
        virtual void PackQmatrix(const MPEG2DecoderFrameInfo&) = 0;
        virtual void PackSliceParams(const MPEG2Slice&, uint32_t) = 0;

    protected:

        UMC::VideoAccelerator *m_va;
    };


    class PackerVA
        : public Packer
    {

    public:

        PackerVA(UMC::VideoAccelerator * va);

        UMC::Status GetStatusReport(void * /*pStatusReport*/, size_t /*size*/) override
        { return UMC::UMC_OK; }

        // Synchronize task
        UMC::Status SyncTask(MPEG2DecoderFrame* frame, void * error) override
        { return m_va->SyncTask(frame->GetMemID(), error); }

        UMC::Status QueryTaskStatus(uint32_t index, void * status, void * error) override
        { return m_va->QueryTaskStatus(index, status, error); }

        void BeginFrame() override;
        void EndFrame() override;

        // Pack the whole picture
        void PackAU(MPEG2DecoderFrame const&, uint8_t) override;

    protected:
        // Pack picture lice parameters
        void PackPicParams(const MPEG2DecoderFrame&, uint8_t) override;
        // Pack matrix parameters
        void PackQmatrix(const MPEG2DecoderFrameInfo&) override;
        // Pack slice parameters
        void PackSliceParams(const MPEG2Slice&, uint32_t) override;
        // Pack gpu priority
        void PackPriorityParams();

    private:
        void CreateSliceDataBuffer(const MPEG2DecoderFrameInfo&);
        void CreateSliceParamBuffer(const MPEG2DecoderFrameInfo&);
    };

}

#endif // MFX_ENABLE_MPEG2_VIDEO_DECODE
