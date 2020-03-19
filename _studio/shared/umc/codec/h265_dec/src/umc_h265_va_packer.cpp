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

#include "umc_h265_va_packer.h"
#include "umc_va_base.h"

#ifdef UMC_VA
#include "umc_h265_task_supplier.h"
#endif

#include "umc_h265_tables.h"
#include "umc_va_linux_protected.h"
#include "mfx_ext_buffers.h"

using namespace UMC;

namespace UMC_HEVC_DECODER
{
    int const s_quantTSDefault4x4[16] =
    {
      16,16,16,16,
      16,16,16,16,
      16,16,16,16,
      16,16,16,16
    };

    int const s_quantIntraDefault8x8[64] =
    {
      16,16,16,16,17,18,21,24,  // 10 10 10 10 11 12 15 18
      16,16,16,16,17,19,22,25,
      16,16,17,18,20,22,25,29,
      16,16,18,21,24,27,31,36,
      17,17,20,24,30,35,41,47,
      18,19,22,27,35,44,54,65,
      21,22,25,31,41,54,70,88,
      24,25,29,36,47,65,88,115
    };

    int const s_quantInterDefault8x8[64] =
    {
      16,16,16,16,17,18,20,24,
      16,16,16,17,18,20,24,25,
      16,16,17,18,20,24,25,28,
      16,17,18,20,24,25,28,33,
      17,18,20,24,25,28,33,41,
      18,20,24,25,28,33,41,54,
      20,24,25,28,33,41,54,71,
      24,25,28,33,41,54,71,91
    };

    //the tables used to restore original scan order of scaling lists (req. by drivers since ci-main-49045)
    uint16_t const* SL_tab_up_right[] =
    {
        ScanTableDiag4x4,
        g_sigLastScanCG32x32,
        g_sigLastScanCG32x32,
        g_sigLastScanCG32x32
    };

extern Packer * CreatePackerVAAPI(VideoAccelerator*);
#if defined(MFX_ENABLE_CPLIB)
    extern Packer * CreatePackerCENC(VideoAccelerator*);
#endif

Packer * Packer::CreatePacker(VideoAccelerator * va)
{
    Packer * packer = 0;

#ifdef MFX_ENABLE_CPLIB
    if (va->GetProtectedVA() && IS_PROTECTION_CENC(va->GetProtectedVA()->GetProtected()))
        packer = CreatePackerCENC(va);
    else
#endif
    packer = CreatePackerVAAPI(va);

    return packer;
}

Packer::Packer(VideoAccelerator * va)
    : m_va(va)
{
}

Packer::~Packer()
{
}

} // namespace UMC_HEVC_DECODER

#endif // MFX_ENABLE_H265_VIDEO_DECODE
