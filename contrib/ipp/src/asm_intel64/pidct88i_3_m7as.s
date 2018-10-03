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
 
 
.data
 
.p2align 4, 0x90
 
 
c_0000:
.long  0, 0, 0, 0  
 
one_corr:
.word  1, 1, 1, 1  
 

.word  1, 1, 1, 1  
 
round_frw_row:
.long  524288, 524288  
 

.long  524288, 524288  
 
 
round_i_0:
.long  65536, 65536  
 

.long  65536, 65536  
 
round_i_1:
.long  2901, 2901  
 

.long  2901, 2901  
 
round_i_2:
.long  2260, 2260  
 

.long  2260, 2260  
 
round_i_3:
.long  1704, 1704  
 

.long  1704, 1704  
 
round_i_4:
.long  1024, 1024  
 

.long  1024, 1024  
 
round_i_5:
.long  455, 455  
 

.long  455, 455  
 
round_i_6:
.long  512, 512  
 

.long  512, 512  
 
round_i_7:
.long  373, 373  
 

.long  373, 373  
 
 
tg_1_16:
.word  13036, 13036, 13036, 13036  
 

.word  13036, 13036, 13036, 13036  
 
tg_2_16:
.word  27146, 27146, 27146, 27146  
 

.word  27146, 27146, 27146, 27146  
 
tg_3_16:
.word  -21746, -21746, -21746, -21746  
 

.word  -21746, -21746, -21746, -21746  
 
cos_4_16:
.word  -19195, -19195, -19195, -19195  
 

.word  -19195, -19195, -19195, -19195  
 
ocos_4_16:
.word  23170, 23170, 23170, 23170  
 

.word  23170, 23170, 23170, 23170  
 
 
.p2align 4, 0x90
 
 
tab_f_04:
.word  16384, 16384, 22725, 19266  
 

.word  -8867, -21407, -22725, -12873  
 

.word  16384, 16384, 12873, 4520  
 

.word  21407, 8867, 19266, -4520  
 

.word  16384, -16384, 12873, -22725  
 

.word  21407, -8867, 19266, -22725  
 

.word  -16384, 16384, 4520, 19266  
 

.word  8867, -21407, 4520, -12873  
 
 
tab_f_17:
.word  22725, 22725, 31521, 26722  
 

.word  -12299, -29692, -31521, -17855  
 

.word  22725, 22725, 17855, 6270  
 

.word  29692, 12299, 26722, -6270  
 

.word  22725, -22725, 17855, -31521  
 

.word  29692, -12299, 26722, -31521  
 

.word  -22725, 22725, 6270, 26722  
 

.word  12299, -29692, 6270, -17855  
 
 
tab_f_26:
.word  21407, 21407, 29692, 25172  
 

.word  -11585, -27969, -29692, -16819  
 

.word  21407, 21407, 16819, 5906  
 

.word  27969, 11585, 25172, -5906  
 

.word  21407, -21407, 16819, -29692  
 

.word  27969, -11585, 25172, -29692  
 

.word  -21407, 21407, 5906, 25172  
 

.word  11585, -27969, 5906, -16819  
 
 
tab_f_35:
.word  19266, 19266, 26722, 22654  
 

.word  -10426, -25172, -26722, -15137  
 

.word  19266, 19266, 15137, 5315  
 

.word  25172, 10426, 22654, -5315  
 

.word  19266, -19266, 15137, -26722  
 

.word  25172, -10426, 22654, -26722  
 

.word  -19266, 19266, 5315, 22654  
 

.word  10426, -25172, 5315, -15137  
 
 
tab_i_04:
.word  16384, 21407, 16384, 8867  
 

.word  -16384, 21407, 16384, -8867  
 

.word  16384, -8867, 16384, -21407  
 

.word  16384, 8867, -16384, -21407  
 

.word  22725, 19266, 19266, -4520  
 

.word  4520, 19266, 19266, -22725  
 

.word  12873, -22725, 4520, -12873  
 

.word  12873, 4520, -22725, -12873  
 
 
tab_i_17:
.word  22725, 29692, 22725, 12299  
 

.word  -22725, 29692, 22725, -12299  
 

.word  22725, -12299, 22725, -29692  
 

.word  22725, 12299, -22725, -29692  
 

.word  31521, 26722, 26722, -6270  
 

.word  6270, 26722, 26722, -31521  
 

.word  17855, -31521, 6270, -17855  
 

.word  17855, 6270, -31521, -17855  
 
 
tab_i_26:
.word  21407, 27969, 21407, 11585  
 

.word  -21407, 27969, 21407, -11585  
 

.word  21407, -11585, 21407, -27969  
 

.word  21407, 11585, -21407, -27969  
 

.word  29692, 25172, 25172, -5906  
 

.word  5906, 25172, 25172, -29692  
 

.word  16819, -29692, 5906, -16819  
 

.word  16819, 5906, -29692, -16819  
 
 
tab_i_35:
.word  19266, 25172, 19266, 10426  
 

.word  -19266, 25172, 19266, -10426  
 

.word  19266, -10426, 19266, -25172  
 

.word  19266, 10426, -19266, -25172  
 

.word  26722, 22654, 22654, -5315  
 

.word  5315, 22654, 22654, -26722  
 

.word  15137, -26722, 5315, -15137  
 

.word  15137, 5315, -26722, -15137  
 
 
tab_i2_0:
.word  16384, 22725, 16384, 19266  
 

.word  16384, 12873, 16384, 4520  
 

.word  16384, -4520, 16384, -12873  
 

.word  16384, -19266, 16384, -22725  
 
 
tab_i2_1:
.word  22725, 31521, 22725, 26722  
 

.word  22725, 17855, 22725, 6270  
 

.word  22725, -6270, 22725, -17855  
 

.word  22725, -26722, 22725, -31521  
 
 
tab_i4_0:
.word  16384, 21407, 16384, 8867  
 

.word  16384, -8867, 16384, -21407  
 

.word  22725, 19266, 19266, -4520  
 

.word  12873, -22725, 4520, -12873  
 
 
tab_i4_1:
.word  22725, 29692, 22725, 12299  
 

.word  22725, -12299, 22725, -29692  
 

.word  31521, 26722, 26722, -6270  
 

.word  17855, -31521, 6270, -17855  
 
 
tab_i4_2:
.word  21407, 27969, 21407, 11585  
 

.word  21407, -11585, 21407, -27969  
 

.word  29692, 25172, 25172, -5906  
 

.word  16819, -29692, 5906, -16819  
 
 
tab_i4_3:
.word  19266, 25172, 19266, 10426  
 

.word  19266, -10426, 19266, -25172  
 

.word  26722, 22654, 22654, -5315  
 

.word  15137, -26722, 5315, -15137  
 
 
.text
 
 
.p2align 4, 0x90
 
 
.globl mfxdct_8x8_inv_2x2_16s

 
mfxdct_8x8_inv_2x2_16s:
 
 
    mov          %rdi, %r10
    mov          %rsi, %r11
 
 
    movd         (%r10), %xmm0
 
    pshufd       $(0), %xmm0, %xmm0
 
    movdqa       tab_i2_0(%rip), %xmm6
 
    pmaddwd      %xmm0, %xmm6
 
    pmaddwd      tab_i2_0+16(%rip), %xmm0
 
 
    movdqa       round_i_0(%rip), %xmm2
    paddd        %xmm2, %xmm6
    paddd        %xmm2, %xmm0
 
    psrad        $(11), %xmm6
    psrad        $(11), %xmm0
    packssdw     %xmm0, %xmm6
 
    movd         (16)(%r10), %xmm0
 
    pshufd       $(0), %xmm0, %xmm0
 
    movdqa       tab_i2_1(%rip), %xmm7
 
    pmaddwd      %xmm0, %xmm7
 
    pmaddwd      tab_i2_1+16(%rip), %xmm0
 
 
    movdqa       round_i_1(%rip), %xmm2
    paddd        %xmm2, %xmm7
    paddd        %xmm2, %xmm0
 
    psrad        $(11), %xmm7
    psrad        $(11), %xmm0
    packssdw     %xmm0, %xmm7
 
 
    test         $(15), %r11
    jnz          LMISALIGNED_DSTgas_5
 
LALIGNED_DSTgas_5:  
 
 
    movdqa       %xmm6, %xmm2
    paddsw       one_corr(%rip), %xmm2
    movdqa       %xmm6, %xmm3
    psubsw       one_corr(%rip), %xmm3
 
    movdqa       tg_1_16(%rip), %xmm0
    pmulhw       %xmm7, %xmm0
 
    movdqa       %xmm7, %xmm1
    paddsw       %xmm2, %xmm1
    psubsw       %xmm7, %xmm2
 
    movdqa       %xmm0, %xmm4
    paddsw       %xmm3, %xmm0
    psubsw       %xmm4, %xmm3
 
    movdqa       %xmm7, %xmm5
    psubsw       %xmm4, %xmm5
    paddsw       %xmm7, %xmm4
 
    movdqa       cos_4_16(%rip), %xmm7
    pmulhw       %xmm5, %xmm7
    paddsw       %xmm5, %xmm7
 
    movdqa       cos_4_16(%rip), %xmm5
    pmulhw       %xmm4, %xmm5
    paddsw       %xmm4, %xmm5
 
    movdqa       %xmm6, %xmm4
    psubsw       %xmm7, %xmm4
    paddsw       %xmm6, %xmm7
 
    psraw        $(6), %xmm1
    movdqa       %xmm1, (%r11)
 
    movdqa       %xmm6, %xmm1
    psubsw       %xmm5, %xmm1
    paddsw       %xmm6, %xmm5
 
    psraw        $(6), %xmm2
    movdqa       %xmm2, (112)(%r11)
 
    psraw        $(6), %xmm0
    movdqa       %xmm0, (48)(%r11)
 
    psraw        $(6), %xmm3
    movdqa       %xmm3, (64)(%r11)
 
    psraw        $(6), %xmm7
    movdqa       %xmm7, (32)(%r11)
 
    psraw        $(6), %xmm4
    movdqa       %xmm4, (80)(%r11)
 
    psraw        $(6), %xmm5
    movdqa       %xmm5, (16)(%r11)
 
    psraw        $(6), %xmm1
    movdqa       %xmm1, (96)(%r11)
 
 
    ret
 
LMISALIGNED_DSTgas_5:  
 
 
    movdqa       %xmm6, %xmm2
    paddsw       one_corr(%rip), %xmm2
    movdqa       %xmm6, %xmm3
    psubsw       one_corr(%rip), %xmm3
 
    movdqa       tg_1_16(%rip), %xmm0
    pmulhw       %xmm7, %xmm0
 
    movdqa       %xmm7, %xmm1
    paddsw       %xmm2, %xmm1
    psubsw       %xmm7, %xmm2
 
    movdqa       %xmm0, %xmm4
    paddsw       %xmm3, %xmm0
    psubsw       %xmm4, %xmm3
 
    movdqa       %xmm7, %xmm5
    psubsw       %xmm4, %xmm5
    paddsw       %xmm7, %xmm4
 
    movdqa       cos_4_16(%rip), %xmm7
    pmulhw       %xmm5, %xmm7
    paddsw       %xmm5, %xmm7
 
    movdqa       cos_4_16(%rip), %xmm5
    pmulhw       %xmm4, %xmm5
    paddsw       %xmm4, %xmm5
 
    movdqa       %xmm6, %xmm4
    psubsw       %xmm7, %xmm4
    paddsw       %xmm6, %xmm7
 
    psraw        $(6), %xmm1
    movlps       %xmm1, (%r11)
    movhps       %xmm1, (8)(%r11)
 
    movdqa       %xmm6, %xmm1
    psubsw       %xmm5, %xmm1
    paddsw       %xmm6, %xmm5
 
    psraw        $(6), %xmm2
    movlps       %xmm2, (112)(%r11)
    movhps       %xmm2, (120)(%r11)
 
    psraw        $(6), %xmm0
    movlps       %xmm0, (48)(%r11)
    movhps       %xmm0, (56)(%r11)
 
    psraw        $(6), %xmm3
    movlps       %xmm3, (64)(%r11)
    movhps       %xmm3, (72)(%r11)
 
    psraw        $(6), %xmm7
    movlps       %xmm7, (32)(%r11)
    movhps       %xmm7, (40)(%r11)
 
    psraw        $(6), %xmm4
    movlps       %xmm4, (80)(%r11)
    movhps       %xmm4, (88)(%r11)
 
    psraw        $(6), %xmm5
    movlps       %xmm5, (16)(%r11)
    movhps       %xmm5, (24)(%r11)
 
    psraw        $(6), %xmm1
    movlps       %xmm1, (96)(%r11)
    movhps       %xmm1, (104)(%r11)
 
 
    ret
 
 
.p2align 4, 0x90
 
 
.globl mfxdct_8x8_inv_4x4_16s

 
mfxdct_8x8_inv_4x4_16s:
 
 
    mov          %rdi, %r10
    mov          %rsi, %r11
 
    test         $(15), %r11
    jz           LIDCT_FOR_ROWSgas_6
 
    mov          %rsp, %rax
    sub          $(128), %rsp
    and          $(-16), %rsp
    mov          %rsp, %r11
 
LIDCT_FOR_ROWSgas_6:  
 
 
    movq         (16)(%r10), %xmm0
    movq         (48)(%r10), %xmm4
 
    pshuflw      $(136), %xmm0, %xmm1
    pshuflw      $(221), %xmm0, %xmm0
    punpcklqdq   %xmm1, %xmm1
    punpcklqdq   %xmm0, %xmm0
 
    pshuflw      $(136), %xmm4, %xmm5
    pshuflw      $(221), %xmm4, %xmm4
    punpcklqdq   %xmm5, %xmm5
    punpcklqdq   %xmm4, %xmm4
 
    pmaddwd      tab_i4_1(%rip), %xmm1
 
    pmaddwd      tab_i4_1+16(%rip), %xmm0
 
 
    pmaddwd      tab_i4_3(%rip), %xmm5
 
    pmaddwd      tab_i4_3+16(%rip), %xmm4
 
 
    paddd        round_i_1(%rip), %xmm1
    paddd        round_i_3(%rip), %xmm5
 
    movdqa       %xmm1, %xmm2
    psubd        %xmm0, %xmm2
    psrad        $(11), %xmm2
    paddd        %xmm0, %xmm1
    psrad        $(11), %xmm1
 
    movdqa       %xmm5, %xmm6
    psubd        %xmm4, %xmm6
    psrad        $(11), %xmm6
    paddd        %xmm4, %xmm5
    psrad        $(11), %xmm5
 
    packssdw     %xmm2, %xmm1
    pshufhw      $(27), %xmm1, %xmm1
 
    packssdw     %xmm6, %xmm5
    pshufhw      $(27), %xmm5, %xmm5
 
    movdqa       %xmm1, (16)(%r11)
    movdqa       %xmm5, (48)(%r11)
 
 
    movq         (%r10), %xmm0
    movq         (32)(%r10), %xmm4
 
    pshuflw      $(136), %xmm0, %xmm1
    pshuflw      $(221), %xmm0, %xmm0
    punpcklqdq   %xmm1, %xmm1
    punpcklqdq   %xmm0, %xmm0
 
    pshuflw      $(136), %xmm4, %xmm5
    pshuflw      $(221), %xmm4, %xmm4
    punpcklqdq   %xmm5, %xmm5
    punpcklqdq   %xmm4, %xmm4
 
    pmaddwd      tab_i4_0(%rip), %xmm1
 
    pmaddwd      tab_i4_0+16(%rip), %xmm0
 
 
    pmaddwd      tab_i4_2(%rip), %xmm5
 
    pmaddwd      tab_i4_2+16(%rip), %xmm4
 
 
    paddd        round_i_0(%rip), %xmm1
    paddd        round_i_2(%rip), %xmm5
 
    movdqa       %xmm1, %xmm2
    psubd        %xmm0, %xmm2
    psrad        $(11), %xmm2
    paddd        %xmm0, %xmm1
    psrad        $(11), %xmm1
 
    movdqa       %xmm5, %xmm6
    psubd        %xmm4, %xmm6
    psrad        $(11), %xmm6
    paddd        %xmm4, %xmm5
    psrad        $(11), %xmm5
 
    packssdw     %xmm2, %xmm1
    pshufhw      $(27), %xmm1, %xmm1
 
    packssdw     %xmm6, %xmm5
    pshufhw      $(27), %xmm5, %xmm5
 
    movdqa       %xmm1, (%r11)
    movdqa       %xmm5, (32)(%r11)
 
 
    cmp          %rsp, %r11
    je           LMISALIGNED_DSTgas_6
 
LALIGNED_DSTgas_6:  
 
 
    movdqa       tg_3_16(%rip), %xmm0
    movdqa       (48)(%r11), %xmm1
    pmulhw       %xmm1, %xmm0
 
    movdqa       tg_1_16(%rip), %xmm2
    movdqa       (16)(%r11), %xmm3
    pmulhw       %xmm3, %xmm2
 
    movdqa       tg_2_16(%rip), %xmm4
    pmulhw       (32)(%r11), %xmm4
 
    paddsw       %xmm1, %xmm0
    psubsw       %xmm1, %xmm3
 
    movdqa       %xmm2, %xmm1
    paddsw       %xmm0, %xmm2
    psubsw       %xmm0, %xmm1
 
    movdqa       %xmm3, %xmm0
    paddsw       %xmm2, %xmm3
    psubsw       %xmm2, %xmm0
 
    movdqa       cos_4_16(%rip), %xmm5
    pmulhw       %xmm3, %xmm5
    movdqa       cos_4_16(%rip), %xmm6
    pmulhw       %xmm0, %xmm6
 
    movdqa       (%r11), %xmm7
    psubsw       %xmm4, %xmm7
    paddsw       (%r11), %xmm4
 
    paddsw       %xmm3, %xmm5
    paddsw       %xmm0, %xmm6
 
    movdqa       %xmm4, %xmm3
    paddsw       %xmm5, %xmm4
    psubsw       %xmm5, %xmm3
 
    movdqa       %xmm7, %xmm0
    paddsw       %xmm6, %xmm7
    psubsw       %xmm6, %xmm0
 
    movdqa       (%r11), %xmm5
    movdqa       (32)(%r11), %xmm6
    psubsw       %xmm6, %xmm5
    paddsw       (%r11), %xmm6
 
    movdqa       %xmm5, %xmm2
    paddsw       %xmm1, %xmm5
    psubsw       %xmm1, %xmm2
 
    movdqa       (16)(%r11), %xmm1
    paddsw       (48)(%r11), %xmm1
 
    psraw        $(6), %xmm4
    movdqa       %xmm4, (16)(%r11)
 
    movdqa       %xmm6, %xmm4
    paddsw       %xmm1, %xmm6
    psubsw       %xmm1, %xmm4
 
    psraw        $(6), %xmm3
    movdqa       %xmm3, (96)(%r11)
 
    psraw        $(6), %xmm7
    movdqa       %xmm7, (32)(%r11)
 
    psraw        $(6), %xmm0
    movdqa       %xmm0, (80)(%r11)
 
    psraw        $(6), %xmm5
    movdqa       %xmm5, (48)(%r11)
 
    psraw        $(6), %xmm2
    movdqa       %xmm2, (64)(%r11)
 
    psraw        $(6), %xmm6
    movdqa       %xmm6, (%r11)
 
    psraw        $(6), %xmm4
    movdqa       %xmm4, (112)(%r11)
 
 
    ret
 
LMISALIGNED_DSTgas_6:  
 
    mov          %rsi, %r11
 
 
    movdqa       tg_3_16(%rip), %xmm0
    movdqa       (48)(%rsp), %xmm1
    pmulhw       %xmm1, %xmm0
 
    movdqa       tg_1_16(%rip), %xmm2
    movdqa       (16)(%rsp), %xmm3
    pmulhw       %xmm3, %xmm2
 
    movdqa       tg_2_16(%rip), %xmm4
    pmulhw       (32)(%rsp), %xmm4
 
    paddsw       %xmm1, %xmm0
    psubsw       %xmm1, %xmm3
 
    movdqa       %xmm2, %xmm1
    paddsw       %xmm0, %xmm2
    psubsw       %xmm0, %xmm1
 
    movdqa       %xmm3, %xmm0
    paddsw       %xmm2, %xmm3
    psubsw       %xmm2, %xmm0
 
    movdqa       cos_4_16(%rip), %xmm5
    pmulhw       %xmm3, %xmm5
    movdqa       cos_4_16(%rip), %xmm6
    pmulhw       %xmm0, %xmm6
 
    movdqa       (%rsp), %xmm7
    psubsw       %xmm4, %xmm7
    paddsw       (%rsp), %xmm4
 
    paddsw       %xmm3, %xmm5
    paddsw       %xmm0, %xmm6
 
    movdqa       %xmm4, %xmm3
    paddsw       %xmm5, %xmm4
    psubsw       %xmm5, %xmm3
 
    movdqa       %xmm7, %xmm0
    paddsw       %xmm6, %xmm7
    psubsw       %xmm6, %xmm0
 
    movdqa       (%rsp), %xmm5
    movdqa       (32)(%rsp), %xmm6
    psubsw       %xmm6, %xmm5
    paddsw       (%rsp), %xmm6
 
    movdqa       %xmm5, %xmm2
    paddsw       %xmm1, %xmm5
    psubsw       %xmm1, %xmm2
 
    movdqa       (16)(%rsp), %xmm1
    paddsw       (48)(%rsp), %xmm1
 
    psraw        $(6), %xmm4
    movlps       %xmm4, (16)(%r11)
    movhps       %xmm4, (24)(%r11)
 
    movdqa       %xmm6, %xmm4
    paddsw       %xmm1, %xmm6
    psubsw       %xmm1, %xmm4
 
    psraw        $(6), %xmm3
    movlps       %xmm3, (96)(%r11)
    movhps       %xmm3, (104)(%r11)
 
    psraw        $(6), %xmm7
    movlps       %xmm7, (32)(%r11)
    movhps       %xmm7, (40)(%r11)
 
    psraw        $(6), %xmm0
    movlps       %xmm0, (80)(%r11)
    movhps       %xmm0, (88)(%r11)
 
    psraw        $(6), %xmm5
    movlps       %xmm5, (48)(%r11)
    movhps       %xmm5, (56)(%r11)
 
    psraw        $(6), %xmm2
    movlps       %xmm2, (64)(%r11)
    movhps       %xmm2, (72)(%r11)
 
    psraw        $(6), %xmm6
    movlps       %xmm6, (%r11)
    movhps       %xmm6, (8)(%r11)
 
    psraw        $(6), %xmm4
    movlps       %xmm4, (112)(%r11)
    movhps       %xmm4, (120)(%r11)
 
 
    mov          %rax, %rsp
 
 
    ret
 
 
