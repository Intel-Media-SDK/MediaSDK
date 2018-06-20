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

#ifndef __UMC_VA_FEI_H__
#define __UMC_VA_FEI_H__


#include "umc_va_linux.h"

#define VA_DECODE_STREAM_OUT_ENABLE       2

#include <list>

namespace UMC
{
    class VAStreamOutBuffer
        : public VACompBuffer
    {

    public:

        VAStreamOutBuffer()
            : m_remap_refs(false)
            , m_field(0)
            , m_allowed_max_mbs_in_slice(0)
        {}

        void RemapRefs(bool remap)
        { m_remap_refs = remap; }
        bool RemapRefs() const
        { return m_remap_refs; }

        void BindToField(int32_t field)
        { m_field = field; }
        int32_t GetField() const
        { return m_field; }

        void FillPicReferences(VAPictureParameterBufferH264 const*);
        void FillSliceReferences(VASliceParameterBufferH264 const*);

        void RemapReferences(void*, int32_t);

    private:

        bool           m_remap_refs;
        int32_t         m_field;

        uint16_t         m_allowed_max_mbs_in_slice;
        VAPictureH264  m_references[16];

        //map [Slice::first_mb_in_slice] onto its Ref. lists
        typedef std::list<std::pair<uint16_t, std::vector<uint32_t> > > slice_map;
        slice_map      m_slice_map;
    };

    class FEIVideoAccelerator
        : public LinuxVideoAccelerator
    {
        DYNAMIC_CAST_DECL(FEIVideoAccelerator, LinuxVideoAccelerator);

    public:

        FEIVideoAccelerator();
        ~FEIVideoAccelerator();

        Status Init(VideoAcceleratorParams*);
        Status Close();

        Status Execute();

        using LinuxVideoAccelerator::ReleaseBuffer;
        void ReleaseBuffer(VAStreamOutBuffer*);
        void* GetCompBuffer(int32_t buffer_type, UMCVACompBuffer **buf, int32_t size, int32_t index);
        Status SyncTask(int32_t index, void * error = NULL);

        VAStreamOutBuffer* QueryStreamOutBuffer(int32_t index, int32_t field);

    private:

        Status MapStreamOutBuffer(VAStreamOutBuffer*);

        VAStreamOutBuffer*               m_streamOutBuffer;
        std::vector<VAStreamOutBuffer*>  m_streamOutCache;
    };
} // namespace UMC


#endif // #ifndef __UMC_VA_FEI_H__
