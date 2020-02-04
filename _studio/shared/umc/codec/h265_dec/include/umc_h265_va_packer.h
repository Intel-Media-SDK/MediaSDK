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

#ifndef __UMC_H265_VA_PACKER_H
#define __UMC_H265_VA_PACKER_H

#include "umc_va_base.h"


#include <va/va_dec_hevc.h>

#include "umc_h265_tables.h"

namespace UMC_HEVC_DECODER
{

enum
{
    NO_REFERENCE = 0,
    SHORT_TERM_REFERENCE = 1,
    LONG_TERM_REFERENCE = 2,
    INTERVIEW_TERM_REFERENCE = 3
};

extern int const s_quantTSDefault4x4[16];
extern int const s_quantIntraDefault8x8[64];
extern int const s_quantInterDefault8x8[64];
extern uint16_t const* SL_tab_up_right[];

inline
int const* getDefaultScalingList(unsigned sizeId, unsigned listId)
{
    const int *src = 0;
    switch (sizeId)
    {
    case SCALING_LIST_4x4:
        src = (int*)g_quantTSDefault4x4;
        break;
    case SCALING_LIST_8x8:
        src = (listId < 3) ? s_quantIntraDefault8x8 : s_quantInterDefault8x8;
        break;
    case SCALING_LIST_16x16:
        src = (listId < 3) ? s_quantIntraDefault8x8 : s_quantInterDefault8x8;
        break;
    case SCALING_LIST_32x32:
        src = (listId < 1) ? s_quantIntraDefault8x8 : s_quantInterDefault8x8;
        break;
    default:
        VM_ASSERT(0);
        src = NULL;
        break;
    }

    return src;
}

template <int COUNT>
inline
void initQMatrix(const H265ScalingList *scalingList, int sizeId, unsigned char qm[6][COUNT], bool force_upright_scan = false)
{
    /*        n*m    listId
        --------------------
        Intra   Y       0
        Intra   Cb      1
        Intra   Cr      2
        Inter   Y       3
        Inter   Cb      4
        Inter   Cr      5         */

    uint16_t const* scan = 0;
    if (force_upright_scan)
        scan = SL_tab_up_right[sizeId];

    for (int n = 0; n < 6; n++)
    {
        const int *src = scalingList->getScalingListAddress(sizeId, n);
        for (int i = 0; i < COUNT; i++)  // coef.
        {
            int const idx = scan ? scan[i] : i;
            qm[n][i] = (unsigned char)src[idx];
        }
    }
}

template<int COUNT>
inline
void initQMatrix(const H265ScalingList *scalingList, int sizeId, unsigned char qm[3][2][COUNT])
{
    for(int comp=0 ; comp <= 2 ; comp++)    // Y Cb Cr
    {
        for(int n=0; n <= 1;n++)
        {
            int listId = comp + 3*n;
            const int *src = scalingList->getScalingListAddress(sizeId, listId);
            for(int i=0;i < COUNT;i++)  // coef.
                qm[comp][n][i] = (unsigned char)src[i];
        }
    }
}

inline
void initQMatrix(const H265ScalingList *scalingList, int sizeId, unsigned char qm[2][64], bool force_upright_scan = false)
{
    /*      n      m     listId
        --------------------
        Intra   Y       0
        Inter   Y       1         */

    uint16_t const* scan = 0;
    if (force_upright_scan)
        scan = SL_tab_up_right[sizeId];

    for(int n=0;n < 2;n++)  // Intra, Inter
    {
        const int *src = scalingList->getScalingListAddress(sizeId, n);

        for (int i = 0; i < 64; i++)  // coef.
        {
            int const idx = scan ? scan[i] : i;
            qm[n][i] = (unsigned char)src[idx];
        }
    }
}

class H265DecoderFrame;
class H265DecoderFrameInfo;
class H265Slice;
class TaskSupplier_H265;

class Packer
{
public:
    Packer(UMC::VideoAccelerator * va);
    virtual ~Packer();

    virtual UMC::Status GetStatusReport(void * pStatusReport, size_t size) = 0;
    virtual UMC::Status SyncTask(int32_t index, void * error) = 0;

    virtual void BeginFrame(H265DecoderFrame*) = 0;
    virtual void EndFrame() = 0;

    virtual void PackAU(H265DecoderFrame const*, TaskSupplier_H265*) = 0;
    virtual void PackPicParams(H265DecoderFrame const*, TaskSupplier_H265*) = 0;
    virtual void PackQmatrix(const H265Slice *pSlice) = 0;

    static Packer * CreatePacker(UMC::VideoAccelerator * va);

    virtual bool PackSliceParams(H265Slice const* slice, size_t, bool last_slice) = 0;

protected:
    UMC::VideoAccelerator *m_va;
};

    /* Helper struct to map given type [T] to certain [UMCVACompBuffer::type] */
    template <typename T>
    struct Type2Buffer;

    template <typename T>
    inline
    UMC::UMCVACompBuffer* GetParamsBuffer(UMC::VideoAccelerator* va, T** pb)
    {
        assert(va);
        assert(pb);

        UMC::UMCVACompBuffer* buffer = nullptr;
        *pb = reinterpret_cast<T*>(va->GetCompBuffer(Type2Buffer<T>::value, &buffer, sizeof(T)));

        return buffer;
    }

    /* Creates [count] of parameter buffers for given type [T] */
    template <typename T>
    inline
    void CreateParamsBuffer(UMC::VideoAccelerator* va, size_t count)
    {
        assert(va);

        UMC::UMCVACompBuffer* buffer = nullptr;
        va->GetCompBuffer(Type2Buffer<T>::value, &buffer, sizeof(T) * count);
        if (!buffer)
            throw h265_exception(UMC::UMC_ERR_FAILED);
    }

    /* Request next (offseted by used data size) buffer for given [type],
       returns pointer to buffer data and its offset */
    inline
    std::pair<void*, size_t>
    PeekBuffer(UMC::VideoAccelerator* va, int32_t type, size_t size, uint32_t data_alignment = 0)
    {
        assert(va);

        UMC::UMCVACompBuffer* buffer = nullptr;
        void* ptr = va->GetCompBuffer(type, &buffer, (int32_t)size);
        if (!buffer)
            throw h265_exception(UMC::UMC_ERR_FAILED);

        size_t const b_size = buffer->GetBufferSize();
        size_t offset = buffer->GetDataSize();

        offset = data_alignment ? data_alignment*((offset+data_alignment-1)/data_alignment) : offset;

        if (size + offset > b_size)
            throw h265_exception(UMC::UMC_ERR_FAILED);

        buffer->SetDataSize(int32_t(size + offset));
        ptr = reinterpret_cast<char*>(ptr) + offset;

        return std::make_pair(ptr, offset);
    }

    /* Request next (see [PeekBuffer]) buffer for given type [T] */
    template <typename T>
    inline
    void PeekParamsBuffer(UMC::VideoAccelerator* va, T** pb, uint32_t data_alignment = 0)
    {
        auto p = PeekBuffer(va, Type2Buffer<T>::value, sizeof(T), data_alignment);
        if (!p.first)
            throw h265_exception(UMC::UMC_ERR_FAILED);

        *pb = reinterpret_cast<T*>(p.first);
        **pb = {};
    }

} // namespace UMC_HEVC_DECODER

#endif /* __UMC_H265_VA_PACKER_H */
#endif // MFX_ENABLE_H265_VIDEO_DECODE
