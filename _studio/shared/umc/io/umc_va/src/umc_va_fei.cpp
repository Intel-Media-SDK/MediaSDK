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

#include <umc_va_base.h>


#include "umc_va_fei.h"
#include "umc_frame_allocator.h"

#include "mfxfei.h"
#include "mfx_trace.h"

#include <algorithm>
#include <climits>

#define UMC_VA_DECODE_STREAM_OUT_ENABLE  2

//#define UMC_VA_STREAMOUT_DEBUG

namespace UMC
{
    inline
    bool check_supported(VADisplay display, VAContextID ctx)
    {
        VABufferID id;
        VAStatus res =
            vaCreateBuffer(display, ctx, VADecodeStreamoutBufferType, sizeof(mfxFeiDecStreamOutMBCtrl) * 4096, 1, NULL, &id);
        std::ignore = MFX_STS_TRACE(res);

        if (res != VA_STATUS_SUCCESS)
            return false;

        mfxStatus sts = CheckAndDestroyVAbuffer(display, id);
        std::ignore = MFX_STS_TRACE(res);

        return sts == MFX_ERR_NONE;
    }

    inline
    bool find_slice_mb(std::pair<uint16_t, std::vector<uint32_t> > const& l, std::pair<uint16_t, std::vector<uint32_t> > const& r)
    {
        return
            l.first < r.first;
    }

    struct find_ref_frame
    {
        static const unsigned int PICTURE_REF_TERM_MASK =
            VA_PICTURE_H264_SHORT_TERM_REFERENCE | VA_PICTURE_H264_LONG_TERM_REFERENCE;

        unsigned int index;
        unsigned int flags;

        find_ref_frame(VAPictureH264 const& p)
            : index(p.frame_idx)
            , flags(p.flags & PICTURE_REF_TERM_MASK)
        {}

        bool operator()(VAPictureH264 const& t) const
        {
            //LT & ST can have the same indices, we should distinct them
            return
                index == t.frame_idx &&
                flags == (t.flags & PICTURE_REF_TERM_MASK);
        }
    };

    inline
    uint32_t map_slice_ref(VAPictureH264 const* refs, uint32_t count, VAPictureH264 const& pic)
    {
       VAPictureH264 const* r =
            std::find_if(refs, refs + count, find_ref_frame(pic));

        //use count * 2 (top + bottom) to signal 'not found'
        return
            r != refs + count ? int32_t(r - refs) : count * 2;
    }

    void VAStreamOutBuffer::FillPicReferences(VAPictureParameterBufferH264 const* pp)
    {
        VM_ASSERT(m_remap_refs);

        //we do x2 for interlaced content during SPS parsing, scale it down here
        m_allowed_max_mbs_in_slice =
            (pp->picture_width_in_mbs_minus1 + 1) * ((pp->picture_height_in_mbs_minus1 + 1) >> pp->pic_fields.bits.field_pic_flag);

        uint32_t const count = sizeof(m_references) / sizeof(m_references[0]);
        std::copy(pp->ReferenceFrames, pp->ReferenceFrames + count, m_references);

    }

    void VAStreamOutBuffer::FillSliceReferences(VASliceParameterBufferH264 const* sp)
    {
        VM_ASSERT(m_remap_refs);


        //NOTE: we keep [slice_map] sorted implicitly
        slice_map::iterator s =
            std::lower_bound(m_slice_map.begin(), m_slice_map.end(),
                             std::make_pair(sp->first_mb_in_slice, std::vector<uint32_t>()),
                             find_slice_mb);

        if (s != m_slice_map.end())
            //there is this slice in map, due chopping for e.g.
            return;

        m_slice_map.push_back(std::make_pair(sp->first_mb_in_slice, std::vector<uint32_t>()));

        //skip INTRASLICE & S_INTRASLICE
        if ((sp->slice_type % 5) == 2 ||
            (sp->slice_type % 5) == 4)
            return;

        std::vector<uint32_t>& slice_refs = m_slice_map.back().second;

        uint32_t const count = sizeof(m_references) / sizeof(m_references[0]);
        //we use single array for both top/bottom & L0/L1
        //use one additional slot in table to place all invalid indices
        //layout: [ |L0 top|L0 bot|I||L1 top|L1 bot|I| ]
        slice_refs.resize((count * 2 + 1) * 2, 0);

        //loop in descending order to force using low indices when we have duplicates in RefList
        //L0
        uint32_t* map = &slice_refs[0];
        for (int32_t i = sp->num_ref_idx_l0_active_minus1; i >= 0; --i)
        {
            VAPictureH264 const& pic = sp->RefPicList0[i];
            uint32_t const idx =
                map_slice_ref(m_references, count, pic);

            uint32_t const bottom = !!(pic.flags & VA_PICTURE_H264_BOTTOM_FIELD);
            map[idx + count * bottom] = i;
        }


        if ((sp->slice_type % 5) != 1)
            return;

        //L1
        map = &slice_refs[count * 2 + 1];
        for (int32_t i = sp->num_ref_idx_l1_active_minus1; i >= 0; --i)
        {
            VAPictureH264 const& pic = sp->RefPicList1[i];
            uint32_t const idx =
                map_slice_ref(m_references, count, pic);

            uint32_t const bottom = !!(pic.flags & VA_PICTURE_H264_BOTTOM_FIELD);
            map[idx + count * bottom] = i;
        }

    }

    void VAStreamOutBuffer::RemapReferences(void* data, int32_t size)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VAStreamOutBuffer::RemapReferences");
        //InterMB.RefIdx explained:
        //Bit 7: Must Be One (1 - indicate it is in Frame store ID format, 0 - indicate it is in Reference Index format)
        //Bit 6:5: reserved MBZ
        //Bit 4:0: Frame store index or Frame Store ID (Bit 4:1 is used to form the binding table index in intel implementation)

        mfxFeiDecStreamOutMBCtrl* mb_begin
            = reinterpret_cast<mfxFeiDecStreamOutMBCtrl*>(data);
        int32_t const mb_total = size / sizeof(mfxFeiDecStreamOutMBCtrl);
        uint32_t const count = sizeof(m_references) / sizeof(m_references[0]);

        int32_t mb_processed = 0;
        slice_map::iterator
            f = m_slice_map.begin(),
            l = m_slice_map.end();
        for (; f != l; ++f)
        {
            std::vector<uint32_t> const& slice_refs = (*f).second;
            if (slice_refs.empty())
                continue;

            slice_map::iterator n = f; ++n;

            uint16_t const first_mb_in_slice = (*f).first;
            int32_t const mb_per_slice_count =
                ((n != l ? (*n).first : m_allowed_max_mbs_in_slice)) - first_mb_in_slice;

            mb_processed += mb_per_slice_count;
            if (mb_processed > mb_total)
            {
                VM_ASSERT(FALSE);
                break;
            }

            mfxFeiDecStreamOutMBCtrl*
                mb     = mb_begin + first_mb_in_slice;
            mfxFeiDecStreamOutMBCtrl const*
                mb_end = mb + mb_per_slice_count;

            for (; mb != mb_end; ++mb)
            {
                if (mb->IntraMbFlag)
                    continue;

                int const l_count =
                    2;
                    //(sp->slice_type % 5) == 1;
                for (int j = 0; j < l_count; ++j)
                {
                    //select L0/L1
                    uint32_t const offset =
                        (count * 2 + 1) * j;

                    uint32_t const* map = &slice_refs[offset];
                    for (int k = 0; k < 4; ++k)
                    {
                        //NOTE: we still have no info about ref. field, use hardcoded zero
                        uint32_t const field = 0;

                        //Ad Hoc: not active references is reported as zero
                        if (!(mb->InterMB.RefIdx[j][k] & 0x80))
                            mb->InterMB.RefIdx[j][k] = UCHAR_MAX;
                        else
                        {
                            uint32_t const idx   = (mb->InterMB.RefIdx[j][k] & 0x1f);

                            mb->InterMB.RefIdx[j][k] =  map[count * field + idx];
                        }
                    }
                }
            }
        }
    }

    FEIVideoAccelerator::FEIVideoAccelerator()
        : m_streamOutBuffer(0)
    {}

    FEIVideoAccelerator::~FEIVideoAccelerator()
    {
        Close();
    }

    Status FEIVideoAccelerator::Init(VideoAcceleratorParams* params)
    {
        VM_ASSERT(GetExtBuffer(param->ExtParam, param->NumExtParam, MFX_EXTBUFF_FEI_PARAM));
        VM_ASSERT(params->m_CreateFlags & VA_DECODE_STREAM_OUT_ENABLE);

        Status sts = LinuxVideoAccelerator::Init(params);
        if (sts != UMC_OK)
            return sts;

        return
            check_supported(m_dpy, *m_pContext) ? UMC_OK : UMC_ERR_UNSUPPORTED;
    }

    Status FEIVideoAccelerator::Close()
    {
        if (m_streamOutBuffer)
        {
            VABufferID id = m_streamOutBuffer->GetID();

            VAStatus vaSts = vaUnmapBuffer(m_dpy, id);
            MFX_CHECK(vaSts == VA_STATUS_SUCCESS, UMC_ERR_FAILED);

            mfxStatus sts = CheckAndDestroyVAbuffer(m_dpy, id);
            MFX_CHECK(sts == MFX_ERR_NONE, UMC_ERR_FAILED);

            UMC_DELETE(m_streamOutBuffer);
        }

        while (!m_streamOutCache.empty())
        {
            VAStreamOutBuffer* buffer = m_streamOutCache.back();
            VM_ASSERT(buffer != m_streamOutBuffer && "Executed SO buffer should not exist in cache");

            ReleaseBuffer(buffer);
        }

        return
            LinuxVideoAccelerator::Close();
    }

    Status FEIVideoAccelerator::Execute()
    {
        VM_ASSERT(m_streamOutBuffer);
        if (!m_streamOutBuffer)
            return UMC_ERR_FAILED;

        // Save some info used for StreamOut [RefIdx] remapping
        if (m_streamOutBuffer->RemapRefs())
        {
            //use [PicParams] buffer to fetch reference frames
            VACompBuffer* buffer;
            GetCompBuffer(VAPictureParameterBufferType, reinterpret_cast<UMCVACompBuffer**>(&buffer), 0, -1);
            VM_ASSERT(buffer && "PicParam buffer should exist here");
            if (!buffer || !buffer->GetPtr())
                return UMC_ERR_FAILED;

            VAPictureParameterBufferH264 const* pp
                = reinterpret_cast<VAPictureParameterBufferH264*>(buffer->GetPtr());

            m_streamOutBuffer->FillPicReferences(pp);

            GetCompBuffer(VASliceParameterBufferType, reinterpret_cast<UMCVACompBuffer**>(&buffer), 0, -1);
            VM_ASSERT(buffer && "PicParam buffer should exist here");
            if (!buffer || !buffer->GetPtr())
                return UMC_ERR_FAILED;

            int32_t const slice_count = buffer->GetNumOfItem();
            VASliceParameterBufferH264 const* sp
                = reinterpret_cast<VASliceParameterBufferH264*>(buffer->GetPtr());

            for (VASliceParameterBufferH264 const* sp_end = sp + slice_count; sp != sp_end; ++sp)
                m_streamOutBuffer->FillSliceReferences(sp);
        }

        Status sts = LinuxVideoAccelerator::Execute();
        if (sts != UMC_OK)
            return sts;

        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_SCHED, "fei: Execute");

        std::lock_guard<std::mutex> l(m_SyncMutex);

        // StreamOut buffer is not handled by the base class, we should execute it here
        {
            VM_ASSERT(m_streamOutBuffer);

            VABufferID id = m_streamOutBuffer->GetID();
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_SCHED, "vaRenderPicture");
            VAStatus va_res = vaRenderPicture(m_dpy, *m_pContext, &id, 1);
            if (va_res != VA_STATUS_SUCCESS)
                return va_to_umc_res(va_res);
        }

        m_streamOutCache.push_back(m_streamOutBuffer);
        m_streamOutBuffer = NULL;

        return UMC_OK;
    }

    void FEIVideoAccelerator::ReleaseBuffer(VAStreamOutBuffer* buffer)
    {
        std::lock_guard<std::mutex> l(m_SyncMutex);

        VABufferID id = buffer->GetID();

        VAStatus vaSts = vaUnmapBuffer(m_dpy, id);
        std::ignore = MFX_STS_TRACE(vaSts);

        mfxStatus sts = CheckAndDestroyVAbuffer(m_dpy, id);
        std::ignore = MFX_STS_TRACE(sts);

        auto i = std::find(m_streamOutCache.begin(), m_streamOutCache.end(), buffer);
        VM_ASSERT(i != m_streamOutCache.end());
        m_streamOutCache.erase(i);

        UMC_DELETE(buffer);
    }

    void* FEIVideoAccelerator::GetCompBuffer(int32_t type, UMCVACompBuffer **buf, int32_t size, int32_t index)
    {
        if (type != VADecodeStreamoutBufferType)
            return LinuxVideoAccelerator::GetCompBuffer(type, buf, size, index);
        else
        {
            VABufferType va_type         = (VABufferType)type;
            unsigned int va_size         = size;
            unsigned int va_num_elements = 1;

            std::lock_guard<std::mutex> l(m_SyncMutex);

            VABufferID id;
            VAStatus va_res =
                vaCreateBuffer(m_dpy, *m_pContext, va_type, va_size, va_num_elements, NULL, &id);

            if (va_res != VA_STATUS_SUCCESS)
                return NULL;

            VM_ASSERT(!m_streamOutBuffer);
            m_streamOutBuffer = new VAStreamOutBuffer();
            m_streamOutBuffer->SetBufferPointer(0, va_size * va_num_elements);
            m_streamOutBuffer->SetDataSize(0);
            m_streamOutBuffer->SetBufferInfo(type, id, index);

            if (buf)
                *buf = m_streamOutBuffer;
        }

        return NULL;
    }

    Status FEIVideoAccelerator::SyncTask(int32_t index, void* error)
    {
        Status umcRes = LinuxVideoAccelerator::SyncTask(index, error);
        if (umcRes != UMC_OK)
            return umcRes;

        std::lock_guard<std::mutex> l(m_SyncMutex);

        for (int i = 0; i < 2; ++i)
        {
            VAStreamOutBuffer* streamOut = QueryStreamOutBuffer(index, i);
            if (streamOut)
            {
                umcRes = MapStreamOutBuffer(streamOut);
                if (umcRes != UMC_OK)
                    break;

                void* ptr = streamOut->GetPtr();
                VM_ASSERT(ptr);

                int32_t const size = streamOut->GetDataSize();
                VM_ASSERT(size = streamOut->GetBufferSize());

                if (streamOut->RemapRefs())
                    streamOut->RemapReferences(ptr, size);

//#define  UMC_VA_STREAMOUT_DUMP
            }
        }

        return umcRes;
    }

    VAStreamOutBuffer* FEIVideoAccelerator::QueryStreamOutBuffer(int32_t index, int32_t field)
    {
        std::vector<VAStreamOutBuffer*>::const_iterator
            f = m_streamOutCache.begin(),
            l = m_streamOutCache.end();
        for (; f != l; ++f)
        {
            if ((*f)->GetIndex() == index &&
                (*f)->GetField() == field)
                break;
        }

        return
            (f != l) ? *f : NULL;
    }

    Status FEIVideoAccelerator::MapStreamOutBuffer(VAStreamOutBuffer* buffer)
    {
        uint8_t* ptr = NULL;
        VAStatus va_res = vaMapBuffer(m_dpy, buffer->GetID(), (void**)&ptr);
        Status umcRes = va_to_umc_res(va_res);
        if (umcRes == UMC_OK)
        {
            int32_t const size = buffer->GetBufferSize();
            buffer->SetBufferPointer(ptr, size);
            buffer->SetDataSize(size);
        }

        return umcRes;
    }
} // namespace UMC

