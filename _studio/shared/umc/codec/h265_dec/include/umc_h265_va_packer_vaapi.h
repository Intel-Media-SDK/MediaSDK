// Copyright (c) 2013-2020 Intel Corporation
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

#ifndef __UMC_H265_VA_PACKER_VAAPI_H
#define __UMC_H265_VA_PACKER_VAAPI_H

#include "umc_h265_va_packer.h"

#include "umc_h265_task_supplier.h"

namespace UMC_HEVC_DECODER
{
    inline int
    LengthInMinCb(int length, int cbsize)
    {
        return length/(1 << cbsize);
    }

    class PackerVAAPI
        : public Packer
    {
    public:

        PackerVAAPI(UMC::VideoAccelerator* va)
            : Packer(va)
        {}

        UMC::Status GetStatusReport(void*, size_t) override
        { return UMC::UMC_OK; }

        UMC::Status SyncTask(int32_t index, void* error) override
        { return m_va->SyncTask(index, error); }

        void BeginFrame(H265DecoderFrame*) override;
        void EndFrame() override
        { /* Nothing to do */}

        void PackAU(H265DecoderFrame const*, TaskSupplier_H265*) override;

        bool PackSliceParams(H265Slice const* slice, size_t, bool last_slice) override
        { return PackSliceParams(slice, last_slice) ? true : false; }

    protected:

        virtual VASliceParameterBufferBase* PackSliceParams(H265Slice const*, bool last_slice) = 0;
        virtual void CreateSliceParamBuffer(size_t count) = 0;
        virtual void PackSliceParams(VASliceParameterBufferBase* sp_base, H265Slice const* slice, bool last_slice) = 0;
        void PackProcessingInfo(H265DecoderFrameInfo * sliceInfo);
        void PackPriorityParams();
		
    private:
        void PackQmatrix(H265Slice const*) override;
    };

    template <>
    struct Type2Buffer<VAPictureParameterBufferHEVC>
        : std::integral_constant<int32_t, VAPictureParameterBufferType>
    {};

    template <>
    struct Type2Buffer<VASliceParameterBufferBase>
        : std::integral_constant<int32_t, VASliceParameterBufferType>
    {};
    template <>
    struct Type2Buffer<VASliceParameterBufferHEVC>
        : std::integral_constant<int32_t, VASliceParameterBufferType>
    {};

    template <>
    struct Type2Buffer<VAIQMatrixBufferHEVC>
        : std::integral_constant<int32_t, VAIQMatrixBufferType>
    {};

    /* Request next (see [PeekBuffer]) buffer for slice data */
    inline
    size_t PeekSliceDataBuffer(UMC::VideoAccelerator* va, uint8_t** data, size_t size)
    {
        assert(data);

        auto p = PeekBuffer(va, VASliceDataBufferType, size);
        if (!p.first)
            throw h265_exception(UMC::UMC_ERR_FAILED);

        *data = reinterpret_cast<uint8_t*>(p.first);
        return p.second;
    }

    inline
    void CreateSliceDataBuffer(UMC::VideoAccelerator* va, H265DecoderFrameInfo const* si)
    {
        assert(va);
        assert(si);

        auto const count = si->GetSliceCount();
        size_t total = 0;
        for (size_t i = 0; i < count; i++)
        {
            H265Slice const* slice = si->GetSlice(i);
            if (!slice)
                throw h265_exception(UMC::UMC_ERR_FAILED);

            auto bs = slice->GetBitStream();
            assert(bs);

            uint32_t size = 0;
            uint32_t* ptr = 0;
            bs->GetOrg(&ptr, &size);

            total += size + 3;
        }

        UMC::UMCVACompBuffer* buffer = nullptr;
        auto ptr = reinterpret_cast<char*>(va->GetCompBuffer(VASliceDataBufferType, &buffer, total));
        if (!ptr)
            throw h265_exception(UMC::UMC_ERR_FAILED);

        buffer->SetDataSize(0);
    }

    /*inline
    size_t GetSliceDataBuffer(UMC::VideoAccelerator* va, char** data, size_t size)
    {
        assert(data);

        auto p = GetSliceBuffer(va, VASliceDataBufferType, size);
        if (!p.first)
            throw h265_exception(UMC::UMC_ERR_FAILED);

        *data = reinterpret_cast<char*>(p.first);
        return p.second;
    }*/

}; // namespace UMC_HEVC_DECODER

#if (MFX_VERSION >= 1032)
    #include "platform/umc_h265_va_packer_vaapi_g12.hpp"
#else
    #include "platform/umc_h265_va_packer_vaapi_g11.hpp"
#endif

#endif // __UMC_H265_VA_PACKER_VAAPI_H
#endif // MFX_ENABLE_H265_VIDEO_DECODE
