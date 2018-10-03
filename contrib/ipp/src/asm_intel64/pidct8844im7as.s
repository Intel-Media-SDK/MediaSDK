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
 
CONST_TABLE:  
 
 
round_i4_0:
.long  66560, 66560, 0, 0  
 
round_i2_0:
.long  67072, 67072, 0, 0  
 
round_i_1:
.long  5802, 5802, 0, 0  
 
round_i_2:
.long  4520, 4520, 0, 0  
 
round_i_3:
.long  3408, 3408, 0, 0  
 
round_i_5:
.long  910, 910, 0, 0  
 
round_i_6:
.long  1024, 1024, 0, 0  
 
round_i_7:
.long  746, 746, 0, 0  
 
 
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
 
 
tab_i4_04:
.word  16384, 15137, 16384, -15137  
 

.word  20995, 7373, 8697, -17799  
 

.word  0, -6270, 0, 6270  
 

.word  -4926, -4176, 11893, -1730  
 
 
tab_i4_17:
.word  22725, 20995, 22725, -20995  
 

.word  29121, 10226, 12063, -24688  
 

.word  0, -8697, 0, 8697  
 

.word  -6833, -5793, 16496, -2399  
 
 
tab_i4_26:
.word  21407, 19777, 21407, -19777  
 

.word  27432, 9633, 11363, -23256  
 

.word  0, -8192, 0, 8192  
 

.word  -6436, -5457, 15539, -2260  
 
 
tab_i4_35:
.word  19266, 17799, 19266, -17799  
 

.word  24688, 8669, 10226, -20929  
 

.word  0, -7373, 0, 7373  
 

.word  -5793, -4911, 13985, -2034  
 
 
tab_i2_04:
.word  16384, 0, 16384, 0  
 

.word  0, 0, 0, 0  
 

.word  14846, -5213, -14846, 5213  
 

.word  3483, -2953, -3483, 2953  
 
 
tab_i2_17:
.word  22725, 0, 22725, 0  
 

.word  0, 0, 0, 0  
 

.word  20592, -7231, -20592, 7231  
 

.word  4832, -4096, -4832, 4096  
 
 
tab_i2_35:
.word  19266, 0, 19266, 0  
 

.word  0, 0, 0, 0  
 

.word  17457, -6130, -17457, 6130  
 

.word  4096, -3472, -4096, 3472  
 
 
.text
 
 
.p2align 4, 0x90
 
 
.globl mfxdct_8x8To4x4_inv_16s

 
mfxdct_8x8To4x4_inv_16s:
 
 
    sub          $(136), %rsp
 
 
    mov          %rdi, %r10
    mov          %rsi, %r11
 
    test         $(15), %r10
    jnz          LMISALIGNED_SRCgas_1
 
LALIGNED_SRCgas_1:  
    movdqa       (48)(%r10), %xmm0
    movdqa       (80)(%r10), %xmm4
 
 
    pshuflw      $(216), %xmm0, %xmm0
    pshufhw      $(216), %xmm0, %xmm0
    pshuflw      $(216), %xmm4, %xmm4
    pshufhw      $(216), %xmm4, %xmm4
 
    pshufd       $(80), %xmm0, %xmm1
    pshufd       $(250), %xmm0, %xmm0
    pshufd       $(80), %xmm4, %xmm5
    pshufd       $(250), %xmm4, %xmm4
 
    pmaddwd      tab_i4_35(%rip), %xmm1
 
    pmaddwd      tab_i4_35+16(%rip), %xmm0
 
 
    pmaddwd      tab_i4_35(%rip), %xmm5
 
    pmaddwd      tab_i4_35+16(%rip), %xmm4
 
 
    paddd        round_i_3(%rip), %xmm1
    paddd        round_i_5(%rip), %xmm5
    paddd        %xmm0, %xmm1
    paddd        %xmm4, %xmm5
 
    movdqa       %xmm1, %xmm2
    punpcklqdq   %xmm5, %xmm1
    punpckhqdq   %xmm5, %xmm2
 
    movdqa       %xmm1, %xmm0
    psubd        %xmm2, %xmm1
    paddd        %xmm2, %xmm0
    pshufd       $(177), %xmm1, %xmm1
 
    movdqa       %xmm0, %xmm4
    punpcklqdq   %xmm1, %xmm0
    punpckhqdq   %xmm1, %xmm4
 
    psrad        $(12), %xmm0
    psrad        $(12), %xmm4
 
    packssdw     %xmm0, %xmm0
    movdqa       %xmm0, (48)(%rsp)
 
    packssdw     %xmm4, %xmm4
    movdqa       %xmm4, (80)(%rsp)
 
 
    movdqa       (32)(%r10), %xmm0
    movdqa       (96)(%r10), %xmm4
 
 
    pshuflw      $(216), %xmm0, %xmm0
    pshufhw      $(216), %xmm0, %xmm0
    pshuflw      $(216), %xmm4, %xmm4
    pshufhw      $(216), %xmm4, %xmm4
 
    pshufd       $(80), %xmm0, %xmm1
    pshufd       $(250), %xmm0, %xmm0
    pshufd       $(80), %xmm4, %xmm5
    pshufd       $(250), %xmm4, %xmm4
 
    pmaddwd      tab_i4_26(%rip), %xmm1
 
    pmaddwd      tab_i4_26+16(%rip), %xmm0
 
 
    pmaddwd      tab_i4_26(%rip), %xmm5
 
    pmaddwd      tab_i4_26+16(%rip), %xmm4
 
 
    paddd        round_i_2(%rip), %xmm1
    paddd        round_i_6(%rip), %xmm5
    paddd        %xmm0, %xmm1
    paddd        %xmm4, %xmm5
 
    movdqa       %xmm1, %xmm2
    punpcklqdq   %xmm5, %xmm1
    punpckhqdq   %xmm5, %xmm2
 
    movdqa       %xmm1, %xmm0
    psubd        %xmm2, %xmm1
    paddd        %xmm2, %xmm0
    pshufd       $(177), %xmm1, %xmm1
 
    movdqa       %xmm0, %xmm4
    punpcklqdq   %xmm1, %xmm0
    punpckhqdq   %xmm1, %xmm4
 
    psrad        $(12), %xmm0
    psrad        $(12), %xmm4
 
    packssdw     %xmm0, %xmm0
    movdqa       %xmm0, (32)(%rsp)
 
    packssdw     %xmm4, %xmm4
    movdqa       %xmm4, (96)(%rsp)
 
 
    movdqa       (16)(%r10), %xmm0
    movdqa       (112)(%r10), %xmm4
 
 
    pshuflw      $(216), %xmm0, %xmm0
    pshufhw      $(216), %xmm0, %xmm0
    pshuflw      $(216), %xmm4, %xmm4
    pshufhw      $(216), %xmm4, %xmm4
 
    pshufd       $(80), %xmm0, %xmm1
    pshufd       $(250), %xmm0, %xmm0
    pshufd       $(80), %xmm4, %xmm5
    pshufd       $(250), %xmm4, %xmm4
 
    pmaddwd      tab_i4_17(%rip), %xmm1
 
    pmaddwd      tab_i4_17+16(%rip), %xmm0
 
 
    pmaddwd      tab_i4_17(%rip), %xmm5
 
    pmaddwd      tab_i4_17+16(%rip), %xmm4
 
 
    paddd        round_i_1(%rip), %xmm1
    paddd        round_i_7(%rip), %xmm5
    paddd        %xmm0, %xmm1
    paddd        %xmm4, %xmm5
 
    movdqa       %xmm1, %xmm2
    punpcklqdq   %xmm5, %xmm1
    punpckhqdq   %xmm5, %xmm2
 
    movdqa       %xmm1, %xmm0
    psubd        %xmm2, %xmm1
    paddd        %xmm2, %xmm0
    pshufd       $(177), %xmm1, %xmm1
 
    movdqa       %xmm0, %xmm4
    punpcklqdq   %xmm1, %xmm0
    punpckhqdq   %xmm1, %xmm4
 
    psrad        $(12), %xmm0
    psrad        $(12), %xmm4
 
    packssdw     %xmm0, %xmm0
    movdqa       %xmm0, (16)(%rsp)
 
    packssdw     %xmm4, %xmm4
    movdqa       %xmm4, (112)(%rsp)
 
 
    movdqa       (%r10), %xmm0
 
 
    pshuflw      $(216), %xmm0, %xmm0
    pshufhw      $(216), %xmm0, %xmm0
    pshufd       $(80), %xmm0, %xmm1
    pshufd       $(250), %xmm0, %xmm0
 
    pmaddwd      tab_i4_04(%rip), %xmm1
 
    pmaddwd      tab_i4_04+16(%rip), %xmm0
 
 
    paddd        %xmm0, %xmm1
    pshufd       $(238), %xmm1, %xmm0
    paddd        round_i4_0(%rip), %xmm1
 
    movdqa       %xmm0, %xmm2
    paddd        %xmm1, %xmm0
    psubd        %xmm2, %xmm1
 
    shufps       $(20), %xmm1, %xmm0
    psrad        $(12), %xmm0
 
    packssdw     %xmm0, %xmm0
    movdqa       %xmm0, (%rsp)
 
 
    jmp          LDCT_COLgas_1
 
LMISALIGNED_SRCgas_1:  
    movdqu       (48)(%r10), %xmm0
    movdqu       (80)(%r10), %xmm4
 
 
    pshuflw      $(216), %xmm0, %xmm0
    pshufhw      $(216), %xmm0, %xmm0
    pshuflw      $(216), %xmm4, %xmm4
    pshufhw      $(216), %xmm4, %xmm4
 
    pshufd       $(80), %xmm0, %xmm1
    pshufd       $(250), %xmm0, %xmm0
    pshufd       $(80), %xmm4, %xmm5
    pshufd       $(250), %xmm4, %xmm4
 
    pmaddwd      tab_i4_35(%rip), %xmm1
 
    pmaddwd      tab_i4_35+16(%rip), %xmm0
 
 
    pmaddwd      tab_i4_35(%rip), %xmm5
 
    pmaddwd      tab_i4_35+16(%rip), %xmm4
 
 
    paddd        round_i_3(%rip), %xmm1
    paddd        round_i_5(%rip), %xmm5
    paddd        %xmm0, %xmm1
    paddd        %xmm4, %xmm5
 
    movdqa       %xmm1, %xmm2
    punpcklqdq   %xmm5, %xmm1
    punpckhqdq   %xmm5, %xmm2
 
    movdqa       %xmm1, %xmm0
    psubd        %xmm2, %xmm1
    paddd        %xmm2, %xmm0
    pshufd       $(177), %xmm1, %xmm1
 
    movdqa       %xmm0, %xmm4
    punpcklqdq   %xmm1, %xmm0
    punpckhqdq   %xmm1, %xmm4
 
    psrad        $(12), %xmm0
    psrad        $(12), %xmm4
 
    packssdw     %xmm0, %xmm0
    movdqa       %xmm0, (48)(%rsp)
 
    packssdw     %xmm4, %xmm4
    movdqa       %xmm4, (80)(%rsp)
 
 
    movdqu       (32)(%r10), %xmm0
    movdqu       (96)(%r10), %xmm4
 
 
    pshuflw      $(216), %xmm0, %xmm0
    pshufhw      $(216), %xmm0, %xmm0
    pshuflw      $(216), %xmm4, %xmm4
    pshufhw      $(216), %xmm4, %xmm4
 
    pshufd       $(80), %xmm0, %xmm1
    pshufd       $(250), %xmm0, %xmm0
    pshufd       $(80), %xmm4, %xmm5
    pshufd       $(250), %xmm4, %xmm4
 
    pmaddwd      tab_i4_26(%rip), %xmm1
 
    pmaddwd      tab_i4_26+16(%rip), %xmm0
 
 
    pmaddwd      tab_i4_26(%rip), %xmm5
 
    pmaddwd      tab_i4_26+16(%rip), %xmm4
 
 
    paddd        round_i_2(%rip), %xmm1
    paddd        round_i_6(%rip), %xmm5
    paddd        %xmm0, %xmm1
    paddd        %xmm4, %xmm5
 
    movdqa       %xmm1, %xmm2
    punpcklqdq   %xmm5, %xmm1
    punpckhqdq   %xmm5, %xmm2
 
    movdqa       %xmm1, %xmm0
    psubd        %xmm2, %xmm1
    paddd        %xmm2, %xmm0
    pshufd       $(177), %xmm1, %xmm1
 
    movdqa       %xmm0, %xmm4
    punpcklqdq   %xmm1, %xmm0
    punpckhqdq   %xmm1, %xmm4
 
    psrad        $(12), %xmm0
    psrad        $(12), %xmm4
 
    packssdw     %xmm0, %xmm0
    movdqa       %xmm0, (32)(%rsp)
 
    packssdw     %xmm4, %xmm4
    movdqa       %xmm4, (96)(%rsp)
 
 
    movdqu       (16)(%r10), %xmm0
    movdqu       (112)(%r10), %xmm4
 
 
    pshuflw      $(216), %xmm0, %xmm0
    pshufhw      $(216), %xmm0, %xmm0
    pshuflw      $(216), %xmm4, %xmm4
    pshufhw      $(216), %xmm4, %xmm4
 
    pshufd       $(80), %xmm0, %xmm1
    pshufd       $(250), %xmm0, %xmm0
    pshufd       $(80), %xmm4, %xmm5
    pshufd       $(250), %xmm4, %xmm4
 
    pmaddwd      tab_i4_17(%rip), %xmm1
 
    pmaddwd      tab_i4_17+16(%rip), %xmm0
 
 
    pmaddwd      tab_i4_17(%rip), %xmm5
 
    pmaddwd      tab_i4_17+16(%rip), %xmm4
 
 
    paddd        round_i_1(%rip), %xmm1
    paddd        round_i_7(%rip), %xmm5
    paddd        %xmm0, %xmm1
    paddd        %xmm4, %xmm5
 
    movdqa       %xmm1, %xmm2
    punpcklqdq   %xmm5, %xmm1
    punpckhqdq   %xmm5, %xmm2
 
    movdqa       %xmm1, %xmm0
    psubd        %xmm2, %xmm1
    paddd        %xmm2, %xmm0
    pshufd       $(177), %xmm1, %xmm1
 
    movdqa       %xmm0, %xmm4
    punpcklqdq   %xmm1, %xmm0
    punpckhqdq   %xmm1, %xmm4
 
    psrad        $(12), %xmm0
    psrad        $(12), %xmm4
 
    packssdw     %xmm0, %xmm0
    movdqa       %xmm0, (16)(%rsp)
 
    packssdw     %xmm4, %xmm4
    movdqa       %xmm4, (112)(%rsp)
 
 
    movdqu       (%r10), %xmm0
 
 
    pshuflw      $(216), %xmm0, %xmm0
    pshufhw      $(216), %xmm0, %xmm0
    pshufd       $(80), %xmm0, %xmm1
    pshufd       $(250), %xmm0, %xmm0
 
    pmaddwd      tab_i4_04(%rip), %xmm1
 
    pmaddwd      tab_i4_04+16(%rip), %xmm0
 
 
    paddd        %xmm0, %xmm1
    pshufd       $(238), %xmm1, %xmm0
    paddd        round_i4_0(%rip), %xmm1
 
    movdqa       %xmm0, %xmm2
    paddd        %xmm1, %xmm0
    psubd        %xmm2, %xmm1
 
    shufps       $(20), %xmm1, %xmm0
    psrad        $(12), %xmm0
 
    packssdw     %xmm0, %xmm0
    movdqa       %xmm0, (%rsp)
 
 
LDCT_COLgas_1:  
 
 
    movdqa       (80)(%rsp), %xmm0
 
    movdqa       tg_3_16(%rip), %xmm1
    movdqa       %xmm0, %xmm2
 
    movdqa       (48)(%rsp), %xmm3
    pmulhw       %xmm1, %xmm0
 
    movdqa       (112)(%rsp), %xmm4
    pmulhw       %xmm3, %xmm1
 
    movdqa       tg_1_16(%rip), %xmm5
    movdqa       %xmm4, %xmm6
 
    pmulhw       %xmm5, %xmm4
    paddsw       %xmm2, %xmm0
 
    pmulhw       (16)(%rsp), %xmm5
    paddsw       %xmm3, %xmm1
 
    movdqa       (96)(%rsp), %xmm7
    paddsw       %xmm3, %xmm0
 
    movdqa       tg_2_16(%rip), %xmm3
    psubsw       %xmm1, %xmm2
 
    pmulhw       %xmm3, %xmm7
    movdqa       %xmm0, %xmm1
 
    pmulhw       (32)(%rsp), %xmm3
    psubsw       %xmm6, %xmm5
 
    paddsw       (16)(%rsp), %xmm4
    paddsw       %xmm4, %xmm0
 
    paddsw       (32)(%rsp), %xmm7
    psubsw       %xmm1, %xmm4
 
    psubsw       (96)(%rsp), %xmm3
    movdqa       %xmm5, %xmm6
 
    psubsw       %xmm2, %xmm5
    paddsw       %xmm2, %xmm6
 
    paddsw       %xmm7, %xmm3
    movdqa       %xmm4, %xmm1
 
    movdqa       cos_4_16(%rip), %xmm2
    paddsw       %xmm5, %xmm4
 
    movdqa       cos_4_16(%rip), %xmm7
    pmulhw       %xmm4, %xmm2
 
    psubsw       %xmm5, %xmm1
 
    pmulhw       %xmm1, %xmm7
 
    paddsw       %xmm2, %xmm4
 
    paddsw       %xmm1, %xmm7
 
    paddsw       %xmm0, %xmm4
    movdqa       %xmm3, %xmm1
 
    paddsw       %xmm6, %xmm7
 
    movdqa       (%rsp), %xmm6
    paddsw       %xmm6, %xmm6
 
    paddsw       %xmm6, %xmm3
    psubsw       %xmm1, %xmm6
 
    movdqa       %xmm4, %xmm1
    paddsw       %xmm3, %xmm4
    psubsw       %xmm1, %xmm3
 
    movdqa       %xmm7, %xmm2
    paddsw       %xmm6, %xmm7
    psubsw       %xmm2, %xmm6
 
    psraw        $(6), %xmm4
    movq         %xmm4, (%r11)
 
    psraw        $(6), %xmm7
    movq         %xmm7, (8)(%r11)
 
    psraw        $(6), %xmm6
    movq         %xmm6, (16)(%r11)
 
    psraw        $(6), %xmm3
    movq         %xmm3, (24)(%r11)
 
 
    add          $(136), %rsp
 
    ret
 
 
.p2align 4, 0x90
 
 
.globl mfxdct_8x8To2x2_inv_16s

 
mfxdct_8x8To2x2_inv_16s:
 
 
    sub          $(136), %rsp
 
 
    mov          %rdi, %r10
    mov          %rsi, %r11
 
    test         $(15), %r10
    jnz          LMISALIGNED_SRCgas_2
 
LALIGNED_SRCgas_2:  
    movdqa       (16)(%r10), %xmm0
 
 
    pshuflw      $(221), %xmm0, %xmm1
    pshuflw      $(136), %xmm0, %xmm0
    pshufhw      $(221), %xmm1, %xmm1
 
    pmaddwd      tab_i2_17(%rip), %xmm0
    pmaddwd      tab_i2_17+16(%rip), %xmm1
 
 
    pshufd       $(238), %xmm1, %xmm2
    paddd        %xmm1, %xmm2
 
    paddd        round_i_1(%rip), %xmm0
    paddd        %xmm2, %xmm0
 
    psrad        $(12), %xmm0
 
    packssdw     %xmm0, %xmm0
    movdqa       %xmm0, (16)(%rsp)
 
    movdqa       (112)(%r10), %xmm0
 
 
    pshuflw      $(221), %xmm0, %xmm1
    pshuflw      $(136), %xmm0, %xmm0
    pshufhw      $(221), %xmm1, %xmm1
 
    pmaddwd      tab_i2_17(%rip), %xmm0
    pmaddwd      tab_i2_17+16(%rip), %xmm1
 
 
    pshufd       $(238), %xmm1, %xmm2
    paddd        %xmm1, %xmm2
 
    paddd        round_i_7(%rip), %xmm0
    paddd        %xmm2, %xmm0
 
    psrad        $(12), %xmm0
 
    packssdw     %xmm0, %xmm0
    movdqa       %xmm0, (112)(%rsp)
 
 
    movdqa       (48)(%r10), %xmm0
 
 
    pshuflw      $(221), %xmm0, %xmm1
    pshuflw      $(136), %xmm0, %xmm0
    pshufhw      $(221), %xmm1, %xmm1
 
    pmaddwd      tab_i2_35(%rip), %xmm0
    pmaddwd      tab_i2_35+16(%rip), %xmm1
 
 
    pshufd       $(238), %xmm1, %xmm2
    paddd        %xmm1, %xmm2
 
    paddd        round_i_3(%rip), %xmm0
    paddd        %xmm2, %xmm0
 
    psrad        $(12), %xmm0
 
    packssdw     %xmm0, %xmm0
    movdqa       %xmm0, (48)(%rsp)
 
    movdqa       (80)(%r10), %xmm0
 
 
    pshuflw      $(221), %xmm0, %xmm1
    pshuflw      $(136), %xmm0, %xmm0
    pshufhw      $(221), %xmm1, %xmm1
 
    pmaddwd      tab_i2_35(%rip), %xmm0
    pmaddwd      tab_i2_35+16(%rip), %xmm1
 
 
    pshufd       $(238), %xmm1, %xmm2
    paddd        %xmm1, %xmm2
 
    paddd        round_i_5(%rip), %xmm0
    paddd        %xmm2, %xmm0
 
    psrad        $(12), %xmm0
 
    packssdw     %xmm0, %xmm0
    movdqa       %xmm0, (80)(%rsp)
 
 
    movdqa       (%r10), %xmm0
 
 
    pshuflw      $(221), %xmm0, %xmm1
    pshuflw      $(136), %xmm0, %xmm0
    pshufhw      $(221), %xmm1, %xmm1
 
    pmaddwd      tab_i2_04(%rip), %xmm0
    pmaddwd      tab_i2_04+16(%rip), %xmm1
 
 
    pshufd       $(238), %xmm1, %xmm2
    paddd        %xmm1, %xmm2
 
    paddd        round_i2_0(%rip), %xmm0
    paddd        %xmm2, %xmm0
 
    psrad        $(12), %xmm0
 
    packssdw     %xmm0, %xmm0
    movdqa       %xmm0, (%rsp)
 
 
    jmp          LDCT_COLgas_2
 
LMISALIGNED_SRCgas_2:  
    movdqu       (16)(%r10), %xmm0
 
 
    pshuflw      $(221), %xmm0, %xmm1
    pshuflw      $(136), %xmm0, %xmm0
    pshufhw      $(221), %xmm1, %xmm1
 
    pmaddwd      tab_i2_17(%rip), %xmm0
    pmaddwd      tab_i2_17+16(%rip), %xmm1
 
 
    pshufd       $(238), %xmm1, %xmm2
    paddd        %xmm1, %xmm2
 
    paddd        round_i_1(%rip), %xmm0
    paddd        %xmm2, %xmm0
 
    psrad        $(12), %xmm0
 
    packssdw     %xmm0, %xmm0
    movdqa       %xmm0, (16)(%rsp)
 
    movdqu       (112)(%r10), %xmm0
 
 
    pshuflw      $(221), %xmm0, %xmm1
    pshuflw      $(136), %xmm0, %xmm0
    pshufhw      $(221), %xmm1, %xmm1
 
    pmaddwd      tab_i2_17(%rip), %xmm0
    pmaddwd      tab_i2_17+16(%rip), %xmm1
 
 
    pshufd       $(238), %xmm1, %xmm2
    paddd        %xmm1, %xmm2
 
    paddd        round_i_7(%rip), %xmm0
    paddd        %xmm2, %xmm0
 
    psrad        $(12), %xmm0
 
    packssdw     %xmm0, %xmm0
    movdqa       %xmm0, (112)(%rsp)
 
 
    movdqu       (48)(%r10), %xmm0
 
 
    pshuflw      $(221), %xmm0, %xmm1
    pshuflw      $(136), %xmm0, %xmm0
    pshufhw      $(221), %xmm1, %xmm1
 
    pmaddwd      tab_i2_35(%rip), %xmm0
    pmaddwd      tab_i2_35+16(%rip), %xmm1
 
 
    pshufd       $(238), %xmm1, %xmm2
    paddd        %xmm1, %xmm2
 
    paddd        round_i_3(%rip), %xmm0
    paddd        %xmm2, %xmm0
 
    psrad        $(12), %xmm0
 
    packssdw     %xmm0, %xmm0
    movdqa       %xmm0, (48)(%rsp)
 
    movdqu       (80)(%r10), %xmm0
 
 
    pshuflw      $(221), %xmm0, %xmm1
    pshuflw      $(136), %xmm0, %xmm0
    pshufhw      $(221), %xmm1, %xmm1
 
    pmaddwd      tab_i2_35(%rip), %xmm0
    pmaddwd      tab_i2_35+16(%rip), %xmm1
 
 
    pshufd       $(238), %xmm1, %xmm2
    paddd        %xmm1, %xmm2
 
    paddd        round_i_5(%rip), %xmm0
    paddd        %xmm2, %xmm0
 
    psrad        $(12), %xmm0
 
    packssdw     %xmm0, %xmm0
    movdqa       %xmm0, (80)(%rsp)
 
 
    movdqu       (%r10), %xmm0
 
 
    pshuflw      $(221), %xmm0, %xmm1
    pshuflw      $(136), %xmm0, %xmm0
    pshufhw      $(221), %xmm1, %xmm1
 
    pmaddwd      tab_i2_04(%rip), %xmm0
    pmaddwd      tab_i2_04+16(%rip), %xmm1
 
 
    pshufd       $(238), %xmm1, %xmm2
    paddd        %xmm1, %xmm2
 
    paddd        round_i2_0(%rip), %xmm0
    paddd        %xmm2, %xmm0
 
    psrad        $(12), %xmm0
 
    packssdw     %xmm0, %xmm0
    movdqa       %xmm0, (%rsp)
 
 
LDCT_COLgas_2:  
 
 
    movdqa       (80)(%rsp), %xmm0
 
    movdqa       tg_3_16(%rip), %xmm1
    movdqa       %xmm0, %xmm2
 
    movdqa       (48)(%rsp), %xmm3
    pmulhw       %xmm1, %xmm0
 
    movdqa       (112)(%rsp), %xmm4
    pmulhw       %xmm3, %xmm1
 
    movdqa       tg_1_16(%rip), %xmm5
    movdqa       %xmm4, %xmm6
 
    pmulhw       %xmm5, %xmm4
    paddsw       %xmm2, %xmm0
 
    pmulhw       (16)(%rsp), %xmm5
    paddsw       %xmm3, %xmm1
 
    paddsw       %xmm3, %xmm0
 
    psubsw       %xmm1, %xmm2
 
    movdqa       %xmm0, %xmm1
 
    psubsw       %xmm6, %xmm5
 
    paddsw       (16)(%rsp), %xmm4
    paddsw       %xmm4, %xmm0
 
    psubsw       %xmm1, %xmm4
 
    paddsw       %xmm2, %xmm5
 
    movdqa       cos_4_16(%rip), %xmm2
 
    pmulhw       %xmm4, %xmm2
 
    movdqa       (%rsp), %xmm6
    paddsw       %xmm6, %xmm6
    paddsw       %xmm6, %xmm6
 
    paddsw       %xmm2, %xmm4
    paddsw       %xmm4, %xmm4
 
    paddsw       %xmm5, %xmm0
    paddsw       %xmm4, %xmm0
 
    movdqa       %xmm6, %xmm1
    paddsw       %xmm0, %xmm6
    psubsw       %xmm0, %xmm1
 
    punpckldq    %xmm1, %xmm6
    psraw        $(7), %xmm6
    movq         %xmm6, (%r11)
 
 
    add          $(136), %rsp
 
    ret
 
 
