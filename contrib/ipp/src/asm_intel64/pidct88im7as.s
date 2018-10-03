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
 
 
.globl mfxdct_8x8_fwd_16s

 
mfxdct_8x8_fwd_16s:
 
 
    mov          %rdi, %r10
    mov          %rsi, %r11
 
    test         $(15), %r11
    jz           LTEST_ALIGN_SRCgas_1
 
    mov          %rsp, %rax
    sub          $(128), %rsp
    and          $(-16), %rsp
    mov          %rsp, %r11
 
LTEST_ALIGN_SRCgas_1:  
    test         $(15), %r10
    jnz          LMISALIGNED_SRCgas_1
 
LALIGNED_SRCgas_1:  
 
 
    movdqa       (16)(%r10), %xmm0
 
    movdqa       (96)(%r10), %xmm1
    movdqa       %xmm0, %xmm2
 
    movdqa       (32)(%r10), %xmm3
    paddsw       %xmm1, %xmm0
 
    movdqa       (80)(%r10), %xmm4
    psllw        $(3), %xmm0
 
    movdqa       (%r10), %xmm5
    paddsw       %xmm3, %xmm4
 
    paddsw       (112)(%r10), %xmm5
    psubsw       %xmm1, %xmm2
 
    movdqa       tg_2_16(%rip), %xmm1
    psllw        $(3), %xmm4
 
    movdqa       %xmm0, %xmm6
    psubsw       %xmm4, %xmm0
 
    movdqa       (48)(%r10), %xmm7
    paddsw       %xmm4, %xmm6
 
    paddsw       (64)(%r10), %xmm7
    pmulhw       %xmm0, %xmm1
 
    movdqa       %xmm5, %xmm4
    psubsw       %xmm7, %xmm5
 
    paddsw       %xmm7, %xmm4
    psllw        $(3), %xmm5
 
    paddsw       %xmm5, %xmm1
    psllw        $(3), %xmm4
 
    por          one_corr(%rip), %xmm1
    psllw        $(4), %xmm2
 
    pmulhw       tg_2_16(%rip), %xmm5
    movdqa       %xmm4, %xmm7
 
    psubsw       (80)(%r10), %xmm3
    psubsw       %xmm6, %xmm4
 
    pshufhw      $(27), %xmm1, %xmm1
    movdqa       %xmm1, (32)(%r11)
    paddsw       %xmm6, %xmm7
 
    movdqa       (48)(%r10), %xmm1
    psllw        $(4), %xmm3
 
    psubsw       (64)(%r10), %xmm1
    movdqa       %xmm2, %xmm6
 
    pshufhw      $(27), %xmm4, %xmm4
    movdqa       %xmm4, (64)(%r11)
    paddsw       %xmm3, %xmm2
 
    pmulhw       ocos_4_16(%rip), %xmm2
    psubsw       %xmm3, %xmm6
 
    pmulhw       ocos_4_16(%rip), %xmm6
    psubsw       %xmm0, %xmm5
 
    por          one_corr(%rip), %xmm5
    psllw        $(3), %xmm1
 
    por          one_corr(%rip), %xmm2
    movdqa       %xmm1, %xmm4
 
    movdqa       (%r10), %xmm3
    paddsw       %xmm6, %xmm1
 
    psubsw       (112)(%r10), %xmm3
    psubsw       %xmm6, %xmm4
 
    movdqa       tg_1_16(%rip), %xmm0
    psllw        $(3), %xmm3
 
    movdqa       tg_3_16(%rip), %xmm6
    pmulhw       %xmm1, %xmm0
 
    pshufhw      $(27), %xmm7, %xmm7
    movdqa       %xmm7, (%r11)
    pmulhw       %xmm4, %xmm6
 
    pshufhw      $(27), %xmm5, %xmm5
    movdqa       %xmm5, (96)(%r11)
    movdqa       %xmm3, %xmm7
 
    movdqa       tg_3_16(%rip), %xmm5
    psubsw       %xmm2, %xmm7
 
    paddsw       %xmm2, %xmm3
    pmulhw       %xmm7, %xmm5
 
    paddsw       %xmm3, %xmm0
    paddsw       %xmm4, %xmm6
 
    pmulhw       tg_1_16(%rip), %xmm3
 
    por          one_corr(%rip), %xmm0
    paddsw       %xmm7, %xmm5
    pshufhw      $(27), %xmm0, %xmm0
 
    psubsw       %xmm6, %xmm7
    pshufhw      $(27), %xmm7, %xmm7
 
    movdqa       %xmm0, (16)(%r11)
    paddsw       %xmm4, %xmm5
 
    movdqa       %xmm7, (48)(%r11)
    psubsw       %xmm1, %xmm3
 
    pshufhw      $(27), %xmm5, %xmm5
    movdqa       %xmm5, (80)(%r11)
 
    pshufhw      $(27), %xmm3, %xmm3
    movdqa       %xmm3, (112)(%r11)
 
 
    cmp          %rsp, %r11
    je           LMISALIGNED_DSTgas_1
 
LALIGNED_DSTgas_1:  
 
 
    movdqa       (%r11), %xmm0
 
    movdqa       (64)(%r11), %xmm1
 
 
    movdqa       (%r11), %xmm2
    punpcklqdq   %xmm1, %xmm0
    punpckhqdq   %xmm1, %xmm2
 
    movdqa       %xmm0, %xmm1
    paddsw       %xmm2, %xmm0
    psubsw       %xmm2, %xmm1
 
    movdqa       %xmm0, %xmm4
    punpckldq    %xmm1, %xmm0
    punpckhdq    %xmm1, %xmm4
    pshufd       $(78), %xmm0, %xmm2
    pshufd       $(78), %xmm4, %xmm6
 
    movdqa       tab_f_04(%rip), %xmm1
 
    pmaddwd      %xmm0, %xmm1
 
    pmaddwd      tab_f_04+32(%rip), %xmm0
 
    movdqa       tab_f_04+16(%rip), %xmm3
 
    pmaddwd      %xmm2, %xmm3
 
    pmaddwd      tab_f_04+48(%rip), %xmm2
 
 
    paddd        round_frw_row(%rip), %xmm1
    paddd        %xmm3, %xmm1
    psrad        $(20), %xmm1
    paddd        round_frw_row(%rip), %xmm0
    paddd        %xmm2, %xmm0
    psrad        $(20), %xmm0
    packssdw     %xmm0, %xmm1
 
    movdqa       tab_f_04(%rip), %xmm5
 
    pmaddwd      %xmm4, %xmm5
 
    pmaddwd      tab_f_04+32(%rip), %xmm4
 
    movdqa       tab_f_04+16(%rip), %xmm7
 
    pmaddwd      %xmm6, %xmm7
 
    pmaddwd      tab_f_04+48(%rip), %xmm6
 
 
    paddd        round_frw_row(%rip), %xmm5
    paddd        %xmm7, %xmm5
    psrad        $(20), %xmm5
    paddd        round_frw_row(%rip), %xmm4
    paddd        %xmm6, %xmm4
    psrad        $(20), %xmm4
    packssdw     %xmm4, %xmm5
 
 
    movdqa       %xmm1, (%r11)
    movdqa       %xmm5, (64)(%r11)
 
 
    movdqa       (16)(%r11), %xmm0
 
    movdqa       (112)(%r11), %xmm1
 
 
    movdqa       (16)(%r11), %xmm2
    punpcklqdq   %xmm1, %xmm0
    punpckhqdq   %xmm1, %xmm2
 
    movdqa       %xmm0, %xmm1
    paddsw       %xmm2, %xmm0
    psubsw       %xmm2, %xmm1
 
    movdqa       %xmm0, %xmm4
    punpckldq    %xmm1, %xmm0
    punpckhdq    %xmm1, %xmm4
    pshufd       $(78), %xmm0, %xmm2
    pshufd       $(78), %xmm4, %xmm6
 
    movdqa       tab_f_17(%rip), %xmm1
 
    pmaddwd      %xmm0, %xmm1
 
    pmaddwd      tab_f_17+32(%rip), %xmm0
 
    movdqa       tab_f_17+16(%rip), %xmm3
 
    pmaddwd      %xmm2, %xmm3
 
    pmaddwd      tab_f_17+48(%rip), %xmm2
 
 
    paddd        round_frw_row(%rip), %xmm1
    paddd        %xmm3, %xmm1
    psrad        $(20), %xmm1
    paddd        round_frw_row(%rip), %xmm0
    paddd        %xmm2, %xmm0
    psrad        $(20), %xmm0
    packssdw     %xmm0, %xmm1
 
    movdqa       tab_f_17(%rip), %xmm5
 
    pmaddwd      %xmm4, %xmm5
 
    pmaddwd      tab_f_17+32(%rip), %xmm4
 
    movdqa       tab_f_17+16(%rip), %xmm7
 
    pmaddwd      %xmm6, %xmm7
 
    pmaddwd      tab_f_17+48(%rip), %xmm6
 
 
    paddd        round_frw_row(%rip), %xmm5
    paddd        %xmm7, %xmm5
    psrad        $(20), %xmm5
    paddd        round_frw_row(%rip), %xmm4
    paddd        %xmm6, %xmm4
    psrad        $(20), %xmm4
    packssdw     %xmm4, %xmm5
 
 
    movdqa       %xmm1, (16)(%r11)
    movdqa       %xmm5, (112)(%r11)
 
 
    movdqa       (32)(%r11), %xmm0
 
    movdqa       (96)(%r11), %xmm1
 
 
    movdqa       (32)(%r11), %xmm2
    punpcklqdq   %xmm1, %xmm0
    punpckhqdq   %xmm1, %xmm2
 
    movdqa       %xmm0, %xmm1
    paddsw       %xmm2, %xmm0
    psubsw       %xmm2, %xmm1
 
    movdqa       %xmm0, %xmm4
    punpckldq    %xmm1, %xmm0
    punpckhdq    %xmm1, %xmm4
    pshufd       $(78), %xmm0, %xmm2
    pshufd       $(78), %xmm4, %xmm6
 
    movdqa       tab_f_26(%rip), %xmm1
 
    pmaddwd      %xmm0, %xmm1
 
    pmaddwd      tab_f_26+32(%rip), %xmm0
 
    movdqa       tab_f_26+16(%rip), %xmm3
 
    pmaddwd      %xmm2, %xmm3
 
    pmaddwd      tab_f_26+48(%rip), %xmm2
 
 
    paddd        round_frw_row(%rip), %xmm1
    paddd        %xmm3, %xmm1
    psrad        $(20), %xmm1
    paddd        round_frw_row(%rip), %xmm0
    paddd        %xmm2, %xmm0
    psrad        $(20), %xmm0
    packssdw     %xmm0, %xmm1
 
    movdqa       tab_f_26(%rip), %xmm5
 
    pmaddwd      %xmm4, %xmm5
 
    pmaddwd      tab_f_26+32(%rip), %xmm4
 
    movdqa       tab_f_26+16(%rip), %xmm7
 
    pmaddwd      %xmm6, %xmm7
 
    pmaddwd      tab_f_26+48(%rip), %xmm6
 
 
    paddd        round_frw_row(%rip), %xmm5
    paddd        %xmm7, %xmm5
    psrad        $(20), %xmm5
    paddd        round_frw_row(%rip), %xmm4
    paddd        %xmm6, %xmm4
    psrad        $(20), %xmm4
    packssdw     %xmm4, %xmm5
 
 
    movdqa       %xmm1, (32)(%r11)
    movdqa       %xmm5, (96)(%r11)
 
 
    movdqa       (48)(%r11), %xmm0
 
    movdqa       (80)(%r11), %xmm1
 
 
    movdqa       (48)(%r11), %xmm2
    punpcklqdq   %xmm1, %xmm0
    punpckhqdq   %xmm1, %xmm2
 
    movdqa       %xmm0, %xmm1
    paddsw       %xmm2, %xmm0
    psubsw       %xmm2, %xmm1
 
    movdqa       %xmm0, %xmm4
    punpckldq    %xmm1, %xmm0
    punpckhdq    %xmm1, %xmm4
    pshufd       $(78), %xmm0, %xmm2
    pshufd       $(78), %xmm4, %xmm6
 
    movdqa       tab_f_35(%rip), %xmm1
 
    pmaddwd      %xmm0, %xmm1
 
    pmaddwd      tab_f_35+32(%rip), %xmm0
 
    movdqa       tab_f_35+16(%rip), %xmm3
 
    pmaddwd      %xmm2, %xmm3
 
    pmaddwd      tab_f_35+48(%rip), %xmm2
 
 
    paddd        round_frw_row(%rip), %xmm1
    paddd        %xmm3, %xmm1
    psrad        $(20), %xmm1
    paddd        round_frw_row(%rip), %xmm0
    paddd        %xmm2, %xmm0
    psrad        $(20), %xmm0
    packssdw     %xmm0, %xmm1
 
    movdqa       tab_f_35(%rip), %xmm5
 
    pmaddwd      %xmm4, %xmm5
 
    pmaddwd      tab_f_35+32(%rip), %xmm4
 
    movdqa       tab_f_35+16(%rip), %xmm7
 
    pmaddwd      %xmm6, %xmm7
 
    pmaddwd      tab_f_35+48(%rip), %xmm6
 
 
    paddd        round_frw_row(%rip), %xmm5
    paddd        %xmm7, %xmm5
    psrad        $(20), %xmm5
    paddd        round_frw_row(%rip), %xmm4
    paddd        %xmm6, %xmm4
    psrad        $(20), %xmm4
    packssdw     %xmm4, %xmm5
 
 
    movdqa       %xmm1, (48)(%r11)
    movdqa       %xmm5, (80)(%r11)
 
 
    ret
 
LMISALIGNED_SRCgas_1:  
 
 
    movdqu       (16)(%r10), %xmm0
 
    movdqu       (96)(%r10), %xmm1
    movdqa       %xmm0, %xmm2
 
    movdqu       (32)(%r10), %xmm3
    paddsw       %xmm1, %xmm0
 
    movdqu       (80)(%r10), %xmm4
    psllw        $(3), %xmm0
 
    movdqu       (%r10), %xmm5
    paddsw       %xmm3, %xmm4
 
    movdqu       (112)(%r10), %xmm6
    paddsw       %xmm6, %xmm5
    psubsw       %xmm1, %xmm2
 
    movdqa       tg_2_16(%rip), %xmm1
    psllw        $(3), %xmm4
 
    movdqa       %xmm0, %xmm6
    psubsw       %xmm4, %xmm0
 
    movdqu       (48)(%r10), %xmm7
    paddsw       %xmm4, %xmm6
 
    movdqu       (64)(%r10), %xmm4
    paddsw       %xmm4, %xmm7
    pmulhw       %xmm0, %xmm1
 
    movdqa       %xmm5, %xmm4
    psubsw       %xmm7, %xmm5
 
    paddsw       %xmm7, %xmm4
    psllw        $(3), %xmm5
 
    paddsw       %xmm5, %xmm1
    psllw        $(3), %xmm4
 
    por          one_corr(%rip), %xmm1
    psllw        $(4), %xmm2
 
    pmulhw       tg_2_16(%rip), %xmm5
    movdqa       %xmm4, %xmm7
 
    pshufhw      $(27), %xmm1, %xmm1
    movdqa       %xmm1, (32)(%r11)
    psubsw       %xmm6, %xmm4
 
    movdqu       (80)(%r10), %xmm1
    psubsw       %xmm1, %xmm3
    paddsw       %xmm6, %xmm7
 
    movdqu       (48)(%r10), %xmm1
    psllw        $(4), %xmm3
 
    movdqu       (64)(%r10), %xmm6
    psubsw       %xmm6, %xmm1
    movdqa       %xmm2, %xmm6
 
    pshufhw      $(27), %xmm4, %xmm4
    movdqa       %xmm4, (64)(%r11)
    paddsw       %xmm3, %xmm2
 
    pmulhw       ocos_4_16(%rip), %xmm2
    psubsw       %xmm3, %xmm6
 
    pmulhw       ocos_4_16(%rip), %xmm6
    psubsw       %xmm0, %xmm5
 
    por          one_corr(%rip), %xmm5
    psllw        $(3), %xmm1
 
    por          one_corr(%rip), %xmm2
    movdqa       %xmm1, %xmm4
 
    movdqu       (%r10), %xmm3
    paddsw       %xmm6, %xmm1
 
    movdqu       (112)(%r10), %xmm0
    psubsw       %xmm0, %xmm3
    psubsw       %xmm6, %xmm4
 
    movdqa       tg_1_16(%rip), %xmm0
    psllw        $(3), %xmm3
 
    movdqa       tg_3_16(%rip), %xmm6
    pmulhw       %xmm1, %xmm0
 
    pshufhw      $(27), %xmm7, %xmm7
    movdqa       %xmm7, (%r11)
    pmulhw       %xmm4, %xmm6
 
    pshufhw      $(27), %xmm5, %xmm5
    movdqa       %xmm5, (96)(%r11)
    movdqa       %xmm3, %xmm7
 
    movdqa       tg_3_16(%rip), %xmm5
    psubsw       %xmm2, %xmm7
 
    paddsw       %xmm2, %xmm3
    pmulhw       %xmm7, %xmm5
 
    paddsw       %xmm3, %xmm0
    paddsw       %xmm4, %xmm6
 
    pmulhw       tg_1_16(%rip), %xmm3
 
    por          one_corr(%rip), %xmm0
    paddsw       %xmm7, %xmm5
    pshufhw      $(27), %xmm0, %xmm0
 
    psubsw       %xmm6, %xmm7
    pshufhw      $(27), %xmm7, %xmm7
 
    movdqa       %xmm0, (16)(%r11)
    paddsw       %xmm4, %xmm5
 
    movdqa       %xmm7, (48)(%r11)
    psubsw       %xmm1, %xmm3
 
    pshufhw      $(27), %xmm5, %xmm5
    movdqa       %xmm5, (80)(%r11)
 
    pshufhw      $(27), %xmm3, %xmm3
    movdqa       %xmm3, (112)(%r11)
 
 
    cmp          %rsp, %r11
    jne          LALIGNED_DSTgas_1
 
LMISALIGNED_DSTgas_1:  
 
    mov          %rsi, %r11
 
 
    movdqa       (%rsp), %xmm0
 
    movdqa       (64)(%rsp), %xmm1
 
 
    movdqa       (%rsp), %xmm2
    punpcklqdq   %xmm1, %xmm0
    punpckhqdq   %xmm1, %xmm2
 
    movdqa       %xmm0, %xmm1
    paddsw       %xmm2, %xmm0
    psubsw       %xmm2, %xmm1
 
    movdqa       %xmm0, %xmm4
    punpckldq    %xmm1, %xmm0
    punpckhdq    %xmm1, %xmm4
    pshufd       $(78), %xmm0, %xmm2
    pshufd       $(78), %xmm4, %xmm6
 
    movdqa       tab_f_04(%rip), %xmm1
 
    pmaddwd      %xmm0, %xmm1
 
    pmaddwd      tab_f_04+32(%rip), %xmm0
 
    movdqa       tab_f_04+16(%rip), %xmm3
 
    pmaddwd      %xmm2, %xmm3
 
    pmaddwd      tab_f_04+48(%rip), %xmm2
 
 
    paddd        round_frw_row(%rip), %xmm1
    paddd        %xmm3, %xmm1
    psrad        $(20), %xmm1
    paddd        round_frw_row(%rip), %xmm0
    paddd        %xmm2, %xmm0
    psrad        $(20), %xmm0
    packssdw     %xmm0, %xmm1
 
    movdqa       tab_f_04(%rip), %xmm5
 
    pmaddwd      %xmm4, %xmm5
 
    pmaddwd      tab_f_04+32(%rip), %xmm4
 
    movdqa       tab_f_04+16(%rip), %xmm7
 
    pmaddwd      %xmm6, %xmm7
 
    pmaddwd      tab_f_04+48(%rip), %xmm6
 
 
    paddd        round_frw_row(%rip), %xmm5
    paddd        %xmm7, %xmm5
    psrad        $(20), %xmm5
    paddd        round_frw_row(%rip), %xmm4
    paddd        %xmm6, %xmm4
    psrad        $(20), %xmm4
    packssdw     %xmm4, %xmm5
 
 
    movlps       %xmm1, (%r11)
    movhps       %xmm1, (8)(%r11)
    movlps       %xmm5, (64)(%r11)
    movhps       %xmm5, (72)(%r11)
 
 
    movdqa       (16)(%rsp), %xmm0
 
    movdqa       (112)(%rsp), %xmm1
 
 
    movdqa       (16)(%rsp), %xmm2
    punpcklqdq   %xmm1, %xmm0
    punpckhqdq   %xmm1, %xmm2
 
    movdqa       %xmm0, %xmm1
    paddsw       %xmm2, %xmm0
    psubsw       %xmm2, %xmm1
 
    movdqa       %xmm0, %xmm4
    punpckldq    %xmm1, %xmm0
    punpckhdq    %xmm1, %xmm4
    pshufd       $(78), %xmm0, %xmm2
    pshufd       $(78), %xmm4, %xmm6
 
    movdqa       tab_f_17(%rip), %xmm1
 
    pmaddwd      %xmm0, %xmm1
 
    pmaddwd      tab_f_17+32(%rip), %xmm0
 
    movdqa       tab_f_17+16(%rip), %xmm3
 
    pmaddwd      %xmm2, %xmm3
 
    pmaddwd      tab_f_17+48(%rip), %xmm2
 
 
    paddd        round_frw_row(%rip), %xmm1
    paddd        %xmm3, %xmm1
    psrad        $(20), %xmm1
    paddd        round_frw_row(%rip), %xmm0
    paddd        %xmm2, %xmm0
    psrad        $(20), %xmm0
    packssdw     %xmm0, %xmm1
 
    movdqa       tab_f_17(%rip), %xmm5
 
    pmaddwd      %xmm4, %xmm5
 
    pmaddwd      tab_f_17+32(%rip), %xmm4
 
    movdqa       tab_f_17+16(%rip), %xmm7
 
    pmaddwd      %xmm6, %xmm7
 
    pmaddwd      tab_f_17+48(%rip), %xmm6
 
 
    paddd        round_frw_row(%rip), %xmm5
    paddd        %xmm7, %xmm5
    psrad        $(20), %xmm5
    paddd        round_frw_row(%rip), %xmm4
    paddd        %xmm6, %xmm4
    psrad        $(20), %xmm4
    packssdw     %xmm4, %xmm5
 
 
    movlps       %xmm1, (16)(%r11)
    movhps       %xmm1, (24)(%r11)
    movlps       %xmm5, (112)(%r11)
    movhps       %xmm5, (120)(%r11)
 
 
    movdqa       (32)(%rsp), %xmm0
 
    movdqa       (96)(%rsp), %xmm1
 
 
    movdqa       (32)(%rsp), %xmm2
    punpcklqdq   %xmm1, %xmm0
    punpckhqdq   %xmm1, %xmm2
 
    movdqa       %xmm0, %xmm1
    paddsw       %xmm2, %xmm0
    psubsw       %xmm2, %xmm1
 
    movdqa       %xmm0, %xmm4
    punpckldq    %xmm1, %xmm0
    punpckhdq    %xmm1, %xmm4
    pshufd       $(78), %xmm0, %xmm2
    pshufd       $(78), %xmm4, %xmm6
 
    movdqa       tab_f_26(%rip), %xmm1
 
    pmaddwd      %xmm0, %xmm1
 
    pmaddwd      tab_f_26+32(%rip), %xmm0
 
    movdqa       tab_f_26+16(%rip), %xmm3
 
    pmaddwd      %xmm2, %xmm3
 
    pmaddwd      tab_f_26+48(%rip), %xmm2
 
 
    paddd        round_frw_row(%rip), %xmm1
    paddd        %xmm3, %xmm1
    psrad        $(20), %xmm1
    paddd        round_frw_row(%rip), %xmm0
    paddd        %xmm2, %xmm0
    psrad        $(20), %xmm0
    packssdw     %xmm0, %xmm1
 
    movdqa       tab_f_26(%rip), %xmm5
 
    pmaddwd      %xmm4, %xmm5
 
    pmaddwd      tab_f_26+32(%rip), %xmm4
 
    movdqa       tab_f_26+16(%rip), %xmm7
 
    pmaddwd      %xmm6, %xmm7
 
    pmaddwd      tab_f_26+48(%rip), %xmm6
 
 
    paddd        round_frw_row(%rip), %xmm5
    paddd        %xmm7, %xmm5
    psrad        $(20), %xmm5
    paddd        round_frw_row(%rip), %xmm4
    paddd        %xmm6, %xmm4
    psrad        $(20), %xmm4
    packssdw     %xmm4, %xmm5
 
 
    movlps       %xmm1, (32)(%r11)
    movhps       %xmm1, (40)(%r11)
    movlps       %xmm5, (96)(%r11)
    movhps       %xmm5, (104)(%r11)
 
 
    movdqa       (48)(%rsp), %xmm0
 
    movdqa       (80)(%rsp), %xmm1
 
 
    movdqa       (48)(%rsp), %xmm2
    punpcklqdq   %xmm1, %xmm0
    punpckhqdq   %xmm1, %xmm2
 
    movdqa       %xmm0, %xmm1
    paddsw       %xmm2, %xmm0
    psubsw       %xmm2, %xmm1
 
    movdqa       %xmm0, %xmm4
    punpckldq    %xmm1, %xmm0
    punpckhdq    %xmm1, %xmm4
    pshufd       $(78), %xmm0, %xmm2
    pshufd       $(78), %xmm4, %xmm6
 
    movdqa       tab_f_35(%rip), %xmm1
 
    pmaddwd      %xmm0, %xmm1
 
    pmaddwd      tab_f_35+32(%rip), %xmm0
 
    movdqa       tab_f_35+16(%rip), %xmm3
 
    pmaddwd      %xmm2, %xmm3
 
    pmaddwd      tab_f_35+48(%rip), %xmm2
 
 
    paddd        round_frw_row(%rip), %xmm1
    paddd        %xmm3, %xmm1
    psrad        $(20), %xmm1
    paddd        round_frw_row(%rip), %xmm0
    paddd        %xmm2, %xmm0
    psrad        $(20), %xmm0
    packssdw     %xmm0, %xmm1
 
    movdqa       tab_f_35(%rip), %xmm5
 
    pmaddwd      %xmm4, %xmm5
 
    pmaddwd      tab_f_35+32(%rip), %xmm4
 
    movdqa       tab_f_35+16(%rip), %xmm7
 
    pmaddwd      %xmm6, %xmm7
 
    pmaddwd      tab_f_35+48(%rip), %xmm6
 
 
    paddd        round_frw_row(%rip), %xmm5
    paddd        %xmm7, %xmm5
    psrad        $(20), %xmm5
    paddd        round_frw_row(%rip), %xmm4
    paddd        %xmm6, %xmm4
    psrad        $(20), %xmm4
    packssdw     %xmm4, %xmm5
 
 
    movlps       %xmm1, (48)(%r11)
    movhps       %xmm1, (56)(%r11)
    movlps       %xmm5, (80)(%r11)
    movhps       %xmm5, (88)(%r11)
 
    mov          %rax, %rsp
 
 
    ret
 
 
