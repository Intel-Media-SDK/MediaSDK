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
    virtual void PackSliceParams(H265Slice const*, size_t, bool isLastSlice) = 0;
    virtual void PackQmatrix(const H265Slice *pSlice) = 0;

    static Packer * CreatePacker(UMC::VideoAccelerator * va);

protected:

    UMC::VideoAccelerator *m_va;
};


class PackerVA
    : public Packer
{
public:

    PackerVA(UMC::VideoAccelerator* va)
        : Packer(va)
    {}

    UMC::Status GetStatusReport(void*, size_t) override
    { return UMC::UMC_OK; }

    UMC::Status SyncTask(int32_t index, void* error) override
    { return m_va->SyncTask(index, error); }

    void BeginFrame(H265DecoderFrame*) override;
    void EndFrame() override
    { /* Nothing to do */ }

    void PackAU(H265DecoderFrame const*, TaskSupplier_H265*) override;

private:

    void PackPicParams(H265DecoderFrame const*, TaskSupplier_H265*) override;
    void PackSliceParams(H265Slice const*, size_t, bool isLastSlice) override;
    void PackQmatrix(H265Slice const*) override;

private:

    void CreateSliceDataBuffer(H265DecoderFrameInfo const*);
    void CreateSliceParamBuffer(H265DecoderFrameInfo const*);
};


} // namespace UMC_HEVC_DECODER

#endif /* __UMC_H265_VA_PACKER_H */
#endif // MFX_ENABLE_H265_VIDEO_DECODE
