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
 
 
.text
 
 
.p2align 4, 0x90
 
SINTZ4:  
    xorps        %xmm1, %xmm1
    cmpeqps      %xmm0, %xmm1
    movmskps     %xmm1, %eax
    and          %eax, %eax
    jnz          SINTZ4_N
 
    rcpps        %xmm0, %xmm1
    mulps        %xmm1, %xmm0
    movaps       %xmm7, %xmm2
    subps        %xmm0, %xmm2
    mulps        %xmm1, %xmm4
    movaps       %xmm2, %xmm0
    mulps        %xmm4, %xmm2
    addps        %xmm4, %xmm2
    mulps        %xmm0, %xmm2
    addps        %xmm2, %xmm4
    ret
 
SINTZ4_N:  
    xorps        %xmm5, %xmm5
    movaps       %xmm7, %xmm3
    addps        %xmm7, %xmm3
    movaps       %xmm7, %xmm2
    movaps       %xmm5, %xmm1
    subps        %xmm3, %xmm5
    addps        %xmm3, %xmm2
    orps         %xmm7, %xmm3
    orps         %xmm7, %xmm5
    orps         %xmm3, %xmm2
    mov          $(4), %eax
 
SINTZ4_N_LP:  
    comiss       %xmm0, %xmm1
    jz           SINTZ4_N_01
    divss        %xmm0, %xmm4
    jmp          SINTZ4_N_1
SINTZ4_N_01:  
    mov          $(1), %ebp
    jp           SINTZ4_N_02
    comiss       %xmm4, %xmm1
    jz           SINTZ4_N_02
    movss        %xmm3, %xmm4
    jc           SINTZ4_N_1
    movss        %xmm5, %xmm4
    jmp          SINTZ4_N_1
SINTZ4_N_02:  
    movss        %xmm2, %xmm4
SINTZ4_N_1:  
    shufps       $(57), %xmm0, %xmm0
    shufps       $(57), %xmm4, %xmm4
    dec          %eax
    jg           SINTZ4_N_LP
    ret
 
 
.p2align 4, 0x90
 
 
.globl mfxownippsDivCRev_16u

 
mfxownippsDivCRev_16u:
 
 
    push         %rbx
 
 
    sub          $(48), %rsp
 
    mov          %rdi, (%rsp)
 
    mov          %rsi, (8)(%rsp)
 
    mov          %rdx, (16)(%rsp)
 
    mov          %rcx, (24)(%rsp)
 
 
    movq         (%rsp), %rsi
    movzwl       (8)(%rsp), %edx
    movq         (16)(%rsp), %rdi
    movslq       (24)(%rsp), %rcx
    mov          $(1065353216), %eax
    movd         %eax, %xmm6
    cvtsi2ss     %edx, %xmm7
    shufps       $(0), %xmm6, %xmm6
    xor          %rax, %rax
    shufps       $(0), %xmm7, %xmm7
    movaps       %xmm6, %xmm8
    pxor         %xmm6, %xmm6
    cmp          $(32767), %edx
    jg           LSTART_BVgas_13
 
 
LBGN_ZR_LPgas_13:  
    test         $(14), %rdi
    jz           LBGN_ZRgas_13
    call         LSBRT_ZRgas_13
    add          $(2), %rsi
    add          $(2), %rdi
    dec          %rcx
    jg           LBGN_ZR_LPgas_13
    jmp          LFINALgas_13
 
 
LSBRT_ZRgas_13:  
    movzwl       (%rsi), %ebx
    and          %ebx, %ebx
    jz           LBGN_ZR_LP_1gas_13
    cvtsi2ss     %ebx, %xmm0
    movss        %xmm7, %xmm1
    divss        %xmm0, %xmm1
    cvtss2si     %xmm1, %ebx
LBGN_ZR_LP_RT1gas_13:  
    movw         %bx, (%rdi)
    ret
 
LBGN_ZR_LP_1gas_13:  
    mov          $(65535), %ebx
    mov          $(1), %eax
    jmp          LBGN_ZR_LP_RT1gas_13
 
 
LBGN_ZRgas_13:  
    sub          $(8), %rcx
    jl           LGLAST_ZRgas_13
    test         $(15), %rdi
    jnz          LGLOOP2_ZRgas_13
    test         $(15), %rsi
    jnz          LGLOOP1_ZRgas_13
    jmp          LGLOOP0_ZRgas_13
 
 
.p2align 4, 0x90
 
LGLOOP0_ZRgas_13:  
    movdqa       (%rsi), %xmm0
    movdqa       %xmm6, %xmm5
    pcmpeqw      %xmm0, %xmm6
    psubw        %xmm6, %xmm0
    movdqa       %xmm0, %xmm1
    punpcklwd    %xmm5, %xmm0
    punpckhwd    %xmm5, %xmm1
    cvtdq2ps     %xmm0, %xmm0
    cvtdq2ps     %xmm1, %xmm1
    add          $(16), %rdi
    rcpps        %xmm0, %xmm2
    movaps       %xmm8, %xmm4
    rcpps        %xmm1, %xmm3
    movaps       %xmm4, %xmm5
    mulps        %xmm2, %xmm0
    mulps        %xmm3, %xmm1
    pmovmskb     %xmm6, %ebx
    subps        %xmm0, %xmm4
    movaps       %xmm7, %xmm0
    subps        %xmm1, %xmm5
    movaps       %xmm7, %xmm1
    mulps        %xmm2, %xmm0
    mulps        %xmm3, %xmm1
    add          $(16), %rsi
    movaps       %xmm4, %xmm2
    mulps        %xmm0, %xmm4
    movaps       %xmm5, %xmm3
    mulps        %xmm1, %xmm5
    addps        %xmm4, %xmm0
    mulps        %xmm2, %xmm2
    addps        %xmm5, %xmm1
    mulps        %xmm3, %xmm3
    mulps        %xmm0, %xmm2
    mulps        %xmm1, %xmm3
    or           %ebx, %eax
    addps        %xmm0, %xmm2
    addps        %xmm1, %xmm3
    cvtps2dq     %xmm2, %xmm2
    cvtps2dq     %xmm3, %xmm3
    sub          $(8), %rcx
    packssdw     %xmm3, %xmm2
    por          %xmm6, %xmm2
    pxor         %xmm6, %xmm6
    movdqa       %xmm2, (-16)(%rdi)
    jge          LGLOOP0_ZRgas_13
    jmp          LGLAST_ZRgas_13
 
 
.p2align 4, 0x90
 
LGLOOP1_ZRgas_13:  
    movdqu       (%rsi), %xmm0
    movdqa       %xmm6, %xmm5
    pcmpeqw      %xmm0, %xmm6
    psubw        %xmm6, %xmm0
    movdqa       %xmm0, %xmm1
    punpcklwd    %xmm5, %xmm0
    punpckhwd    %xmm5, %xmm1
    cvtdq2ps     %xmm0, %xmm0
    cvtdq2ps     %xmm1, %xmm1
    add          $(16), %rdi
    rcpps        %xmm0, %xmm2
    movaps       %xmm8, %xmm4
    rcpps        %xmm1, %xmm3
    movaps       %xmm4, %xmm5
    mulps        %xmm2, %xmm0
    mulps        %xmm3, %xmm1
    pmovmskb     %xmm6, %ebx
    subps        %xmm0, %xmm4
    movaps       %xmm7, %xmm0
    subps        %xmm1, %xmm5
    movaps       %xmm7, %xmm1
    mulps        %xmm2, %xmm0
    mulps        %xmm3, %xmm1
    add          $(16), %rsi
    movaps       %xmm4, %xmm2
    mulps        %xmm0, %xmm4
    movaps       %xmm5, %xmm3
    mulps        %xmm1, %xmm5
    addps        %xmm4, %xmm0
    mulps        %xmm2, %xmm2
    addps        %xmm5, %xmm1
    mulps        %xmm3, %xmm3
    mulps        %xmm0, %xmm2
    mulps        %xmm1, %xmm3
    or           %ebx, %eax
    addps        %xmm0, %xmm2
    addps        %xmm1, %xmm3
    cvtps2dq     %xmm2, %xmm2
    cvtps2dq     %xmm3, %xmm3
    sub          $(8), %rcx
    packssdw     %xmm3, %xmm2
    por          %xmm6, %xmm2
    pxor         %xmm6, %xmm6
    movdqa       %xmm2, (-16)(%rdi)
    jge          LGLOOP1_ZRgas_13
    jmp          LGLAST_ZRgas_13
 
 
.p2align 4, 0x90
 
LGLOOP2_ZRgas_13:  
    movdqu       (%rsi), %xmm0
    movdqa       %xmm6, %xmm5
    pcmpeqw      %xmm0, %xmm6
    psubw        %xmm6, %xmm0
    movdqa       %xmm0, %xmm1
    punpcklwd    %xmm5, %xmm0
    punpckhwd    %xmm5, %xmm1
    cvtdq2ps     %xmm0, %xmm0
    cvtdq2ps     %xmm1, %xmm1
    add          $(16), %rdi
    rcpps        %xmm0, %xmm2
    movaps       %xmm8, %xmm4
    rcpps        %xmm1, %xmm3
    movaps       %xmm4, %xmm5
    mulps        %xmm2, %xmm0
    mulps        %xmm3, %xmm1
    pmovmskb     %xmm6, %ebx
    subps        %xmm0, %xmm4
    movaps       %xmm7, %xmm0
    subps        %xmm1, %xmm5
    movaps       %xmm7, %xmm1
    mulps        %xmm2, %xmm0
    mulps        %xmm3, %xmm1
    add          $(16), %rsi
    movaps       %xmm4, %xmm2
    mulps        %xmm0, %xmm4
    movaps       %xmm5, %xmm3
    mulps        %xmm1, %xmm5
    addps        %xmm4, %xmm0
    mulps        %xmm2, %xmm2
    addps        %xmm5, %xmm1
    mulps        %xmm3, %xmm3
    mulps        %xmm0, %xmm2
    mulps        %xmm1, %xmm3
    or           %ebx, %eax
    addps        %xmm0, %xmm2
    addps        %xmm1, %xmm3
    cvtps2dq     %xmm2, %xmm2
    cvtps2dq     %xmm3, %xmm3
    sub          $(8), %rcx
    packssdw     %xmm3, %xmm2
    por          %xmm6, %xmm2
    pxor         %xmm6, %xmm6
    movdqu       %xmm2, (-16)(%rdi)
    jge          LGLOOP2_ZRgas_13
 
 
LGLAST_ZRgas_13:  
    add          $(8), %rcx
    jle          LFINALgas_13
 
LGLAST_ZR_LPgas_13:  
    call         LSBRT_ZRgas_13
    add          $(2), %rsi
    add          $(2), %rdi
    dec          %rcx
    jg           LBGN_ZR_LPgas_13
    jmp          LFINALgas_13
 
 
LSTART_BVgas_13:  
    movaps       %xmm7, %xmm9
    movd         %edx, %xmm0
    pcmpeqw      %xmm5, %xmm5
    punpcklwd    %xmm0, %xmm0
    psrlw        $(15), %xmm5
    pshufd       $(0), %xmm0, %xmm0
    movaps       %xmm0, %xmm10
    movdqa       %xmm5, %xmm11
 
 
LBGN_BV_LPgas_13:  
    test         $(14), %rdi
    jz           LBGN_BVgas_13
    call         LSBRT_ZRgas_13
    add          $(2), %rsi
    add          $(2), %rdi
    dec          %rcx
    jg           LBGN_BV_LPgas_13
    jmp          LFINALgas_13
 
 
LBGN_BVgas_13:  
    movdqa       %xmm5, %xmm7
    sub          $(8), %rcx
    jl           LGLAST_BVgas_13
    test         $(15), %rdi
    jnz          LGLOOP2_BVgas_13
    test         $(15), %rsi
    jnz          LGLOOP1_BVgas_13
    jmp          LGLOOP0_BVgas_13
 
 
.p2align 4, 0x90
 
LGLOOP0_BVgas_13:  
    movdqa       (%rsi), %xmm0
    movdqa       %xmm6, %xmm5
    pcmpeqw      %xmm0, %xmm6
    pcmpeqw      %xmm0, %xmm7
    psubw        %xmm6, %xmm0
    movdqa       %xmm0, %xmm1
    punpcklwd    %xmm5, %xmm0
    punpckhwd    %xmm5, %xmm1
    cvtdq2ps     %xmm0, %xmm0
    cvtdq2ps     %xmm1, %xmm1
    add          $(16), %rdi
    rcpps        %xmm0, %xmm2
    movaps       %xmm8, %xmm4
    rcpps        %xmm1, %xmm3
    movaps       %xmm4, %xmm5
    mulps        %xmm2, %xmm0
    mulps        %xmm3, %xmm1
    pmovmskb     %xmm6, %ebx
    subps        %xmm0, %xmm4
    movaps       %xmm9, %xmm0
    subps        %xmm1, %xmm5
    movaps       %xmm0, %xmm1
    mulps        %xmm2, %xmm0
    mulps        %xmm3, %xmm1
    add          $(16), %rsi
    movaps       %xmm4, %xmm2
    mulps        %xmm0, %xmm4
    movaps       %xmm5, %xmm3
    mulps        %xmm1, %xmm5
    addps        %xmm4, %xmm0
    mulps        %xmm2, %xmm2
    addps        %xmm5, %xmm1
    mulps        %xmm3, %xmm3
    mulps        %xmm0, %xmm2
    mulps        %xmm1, %xmm3
    or           %ebx, %eax
    addps        %xmm0, %xmm2
    addps        %xmm1, %xmm3
    cvtps2dq     %xmm2, %xmm2
    cvtps2dq     %xmm3, %xmm3
    sub          $(8), %rcx
    packssdw     %xmm3, %xmm2
    psubusw      %xmm7, %xmm2
    pand         %xmm10, %xmm7
    por          %xmm6, %xmm2
    pxor         %xmm6, %xmm6
    por          %xmm7, %xmm2
    movdqa       %xmm11, %xmm7
    movdqa       %xmm2, (-16)(%rdi)
    jge          LGLOOP0_BVgas_13
    jmp          LGLAST_BVgas_13
 
 
.p2align 4, 0x90
 
LGLOOP1_BVgas_13:  
    movdqu       (%rsi), %xmm0
    movdqa       %xmm6, %xmm5
    pcmpeqw      %xmm0, %xmm6
    pcmpeqw      %xmm0, %xmm7
    psubw        %xmm6, %xmm0
    movdqa       %xmm0, %xmm1
    punpcklwd    %xmm5, %xmm0
    punpckhwd    %xmm5, %xmm1
    cvtdq2ps     %xmm0, %xmm0
    cvtdq2ps     %xmm1, %xmm1
    add          $(16), %rdi
    rcpps        %xmm0, %xmm2
    movaps       %xmm8, %xmm4
    rcpps        %xmm1, %xmm3
    movaps       %xmm4, %xmm5
    mulps        %xmm2, %xmm0
    mulps        %xmm3, %xmm1
    pmovmskb     %xmm6, %ebx
    subps        %xmm0, %xmm4
    movaps       %xmm9, %xmm0
    subps        %xmm1, %xmm5
    movaps       %xmm0, %xmm1
    mulps        %xmm2, %xmm0
    mulps        %xmm3, %xmm1
    add          $(16), %rsi
    movaps       %xmm4, %xmm2
    mulps        %xmm0, %xmm4
    movaps       %xmm5, %xmm3
    mulps        %xmm1, %xmm5
    addps        %xmm4, %xmm0
    mulps        %xmm2, %xmm2
    addps        %xmm5, %xmm1
    mulps        %xmm3, %xmm3
    mulps        %xmm0, %xmm2
    mulps        %xmm1, %xmm3
    or           %ebx, %eax
    addps        %xmm0, %xmm2
    addps        %xmm1, %xmm3
    cvtps2dq     %xmm2, %xmm2
    cvtps2dq     %xmm3, %xmm3
    sub          $(8), %rcx
    packssdw     %xmm3, %xmm2
    psubusw      %xmm7, %xmm2
    pand         %xmm10, %xmm7
    por          %xmm6, %xmm2
    pxor         %xmm6, %xmm6
    por          %xmm7, %xmm2
    movdqa       %xmm11, %xmm7
    movdqa       %xmm2, (-16)(%rdi)
    jge          LGLOOP1_BVgas_13
    jmp          LGLAST_BVgas_13
 
 
.p2align 4, 0x90
 
LGLOOP2_BVgas_13:  
    movdqu       (%rsi), %xmm0
    movdqa       %xmm6, %xmm5
    pcmpeqw      %xmm0, %xmm6
    pcmpeqw      %xmm0, %xmm7
    psubw        %xmm6, %xmm0
    movdqa       %xmm0, %xmm1
    punpcklwd    %xmm5, %xmm0
    punpckhwd    %xmm5, %xmm1
    cvtdq2ps     %xmm0, %xmm0
    cvtdq2ps     %xmm1, %xmm1
    add          $(16), %rdi
    rcpps        %xmm0, %xmm2
    movaps       %xmm8, %xmm4
    rcpps        %xmm1, %xmm3
    movaps       %xmm4, %xmm5
    mulps        %xmm2, %xmm0
    mulps        %xmm3, %xmm1
    pmovmskb     %xmm6, %ebx
    subps        %xmm0, %xmm4
    movaps       %xmm9, %xmm0
    subps        %xmm1, %xmm5
    movaps       %xmm0, %xmm1
    mulps        %xmm2, %xmm0
    mulps        %xmm3, %xmm1
    add          $(16), %rsi
    movaps       %xmm4, %xmm2
    mulps        %xmm0, %xmm4
    movaps       %xmm5, %xmm3
    mulps        %xmm1, %xmm5
    addps        %xmm4, %xmm0
    mulps        %xmm2, %xmm2
    addps        %xmm5, %xmm1
    mulps        %xmm3, %xmm3
    mulps        %xmm0, %xmm2
    mulps        %xmm1, %xmm3
    or           %ebx, %eax
    addps        %xmm0, %xmm2
    addps        %xmm1, %xmm3
    cvtps2dq     %xmm2, %xmm2
    cvtps2dq     %xmm3, %xmm3
    sub          $(8), %rcx
    packssdw     %xmm3, %xmm2
    psubusw      %xmm7, %xmm2
    pand         %xmm10, %xmm7
    por          %xmm6, %xmm2
    pxor         %xmm6, %xmm6
    por          %xmm7, %xmm2
    movdqa       %xmm11, %xmm7
    movdqu       %xmm2, (-16)(%rdi)
    jge          LGLOOP2_BVgas_13
    jmp          LGLAST_BVgas_13
 
 
LGLAST_BVgas_13:  
    add          $(8), %rcx
    jle          LFINALgas_13
 
    movaps       %xmm9, %xmm7
 
LGLAST_BV_LPgas_13:  
    call         LSBRT_ZRgas_13
    add          $(2), %rsi
    add          $(2), %rdi
    dec          %rcx
    jg           LBGN_BV_LPgas_13
 
LFINALgas_13:  
    add          $(48), %rsp
 
 
    pop          %rbx
 
    ret
 
