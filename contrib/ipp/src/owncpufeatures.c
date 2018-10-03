// Copyright (c) 2018 Intel Corporation
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

#include "precomp.h"

#ifndef __OWNS_H__
#include "owncore.h"
#endif

#ifndef __CPUDEF_H__
#include "cpudef.h"
#endif

#include "dispatcher.h"

static Ipp64u ownFeaturesMask = PX_FM;


/*=======================================================================*/
/*
1). The "ownFeatures" is initialized by ippCpuFeatures.
2). Features mask (MaskOfFeature) values are defined in the ippdefs.h:
    ippCPUID_MMX        0x00000001   Intel Architecture MMX technology supported
    ippCPUID_SSE        0x00000002   Streaming SIMD Extensions
    ippCPUID_SSE2       0x00000004   Streaming SIMD Extensions 2
    ippCPUID_SSE3       0x00000008   Streaming SIMD Extensions 3
    ippCPUID_SSSE3      0x00000010   Supplemental Streaming SIMD Extensions 3
    ippCPUID_MOVBE      0x00000020   The processor supports MOVBE instruction
    ippCPUID_SSE41      0x00000040   Streaming SIMD Extensions 4.1
    ippCPUID_SSE42      0x00000080   Streaming SIMD Extensions 4.2
    ippCPUID_AVX        0x00000100   Advanced Vector Extensions instruction set
    ippAVX_ENABLEDBYOS  0x00000200   The operating system supports AVX
    ippCPUID_AES        0x00000400   AES instruction
    ippCPUID_CLMUL      0x00000800   PCLMULQDQ instruction
    ippCPUID_ABR        0x00001000   Aubrey Isle
    ippCPUID_RDRAND     0x00002000   RDRRAND instruction
    ippCPUID_F16C       0x00004000   Float16 instructions
    ippCPUID_AVX2       0x00008000   AVX2
    ippCPUID_ADCOX      0x00010000   ADCX and ADOX instructions
    ippCPUID_RDSEED     0x00020000   the RDSEED instruction
    ippCPUID_PREFETCHW  0x00040000   PREFETCHW
    ippCPUID_SHA        0x00080000   Intel (R) SHA Extensions
    ippCPUID_KNC        0x80000000   Knights Corner instruction set

//#define   ippCPUID_AVX2_FMA   0x0008000    256bits fused-multiply-add instructions set
//#define   ippCPUID_AVX2_INT   0x0010000    256bits integer instructions set
//#define   ippCPUID_AVX2_GPR   0x0020000    VEX-encoded general-purpose instructions set
*/
/*=======================================================================*/
int __CDECL mfxownGetFeature( Ipp64u MaskOfFeature )
{
  if( (ownFeaturesMask & MaskOfFeature) == MaskOfFeature ) {
    return 1;
  } else {
    return 0;
  };
}
