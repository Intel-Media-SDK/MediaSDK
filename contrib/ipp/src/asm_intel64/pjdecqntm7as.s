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
 
.globl mfxownpj_QuantInv_8x8_16s

 
mfxownpj_QuantInv_8x8_16s:
 
 
    test         $(15), %rdx
    jnz          LNAgas_1
    test         $(15), %rdi
    jnz          LNA1gas_1
    test         $(15), %rsi
    jnz          LNA2gas_1
    movdqa       (%rdi), %xmm0
    movdqa       (16)(%rdi), %xmm1
    movdqa       (32)(%rdi), %xmm2
    movdqa       (48)(%rdi), %xmm3
    movdqa       (64)(%rdi), %xmm4
    movdqa       (80)(%rdi), %xmm5
    movdqa       (96)(%rdi), %xmm6
    movdqa       (112)(%rdi), %xmm7
    pmullw       (%rdx), %xmm0
    pmullw       (16)(%rdx), %xmm1
    pmullw       (32)(%rdx), %xmm2
    pmullw       (48)(%rdx), %xmm3
    pmullw       (64)(%rdx), %xmm4
    pmullw       (80)(%rdx), %xmm5
    pmullw       (96)(%rdx), %xmm6
    pmullw       (112)(%rdx), %xmm7
    movdqa       %xmm0, (%rsi)
    movdqa       %xmm1, (16)(%rsi)
    movdqa       %xmm2, (32)(%rsi)
    movdqa       %xmm3, (48)(%rsi)
    movdqa       %xmm4, (64)(%rsi)
    movdqa       %xmm5, (80)(%rsi)
    movdqa       %xmm6, (96)(%rsi)
    movdqa       %xmm7, (112)(%rsi)
    jmp          Lexitgas_1
LNA1gas_1:  
    test         $(15), %rsi
    jnz          LNA3gas_1
    movdqu       (%rdi), %xmm0
    movdqu       (16)(%rdi), %xmm1
    movdqu       (32)(%rdi), %xmm2
    movdqu       (48)(%rdi), %xmm3
    movdqu       (64)(%rdi), %xmm4
    movdqu       (80)(%rdi), %xmm5
    movdqu       (96)(%rdi), %xmm6
    movdqu       (112)(%rdi), %xmm7
    pmullw       (%rdx), %xmm0
    pmullw       (16)(%rdx), %xmm1
    pmullw       (32)(%rdx), %xmm2
    pmullw       (48)(%rdx), %xmm3
    pmullw       (64)(%rdx), %xmm4
    pmullw       (80)(%rdx), %xmm5
    pmullw       (96)(%rdx), %xmm6
    pmullw       (112)(%rdx), %xmm7
    movdqa       %xmm0, (%rsi)
    movdqa       %xmm1, (16)(%rsi)
    movdqa       %xmm2, (32)(%rsi)
    movdqa       %xmm3, (48)(%rsi)
    movdqa       %xmm4, (64)(%rsi)
    movdqa       %xmm5, (80)(%rsi)
    movdqa       %xmm6, (96)(%rsi)
    movdqa       %xmm7, (112)(%rsi)
    jmp          Lexitgas_1
LNA2gas_1:  
    movdqa       (%rdi), %xmm0
    movdqa       (16)(%rdi), %xmm1
    movdqa       (32)(%rdi), %xmm2
    movdqa       (48)(%rdi), %xmm3
    movdqa       (64)(%rdi), %xmm4
    movdqa       (80)(%rdi), %xmm5
    movdqa       (96)(%rdi), %xmm6
    movdqa       (112)(%rdi), %xmm7
    pmullw       (%rdx), %xmm0
    pmullw       (16)(%rdx), %xmm1
    pmullw       (32)(%rdx), %xmm2
    pmullw       (48)(%rdx), %xmm3
    pmullw       (64)(%rdx), %xmm4
    pmullw       (80)(%rdx), %xmm5
    pmullw       (96)(%rdx), %xmm6
    pmullw       (112)(%rdx), %xmm7
    movdqu       %xmm0, (%rsi)
    movdqu       %xmm1, (16)(%rsi)
    movdqu       %xmm2, (32)(%rsi)
    movdqu       %xmm3, (48)(%rsi)
    movdqu       %xmm4, (64)(%rsi)
    movdqu       %xmm5, (80)(%rsi)
    movdqu       %xmm6, (96)(%rsi)
    movdqu       %xmm7, (112)(%rsi)
    jmp          Lexitgas_1
LNA3gas_1:  
    movdqu       (%rdi), %xmm0
    movdqu       (16)(%rdi), %xmm1
    movdqu       (32)(%rdi), %xmm2
    movdqu       (48)(%rdi), %xmm3
    movdqu       (64)(%rdi), %xmm4
    movdqu       (80)(%rdi), %xmm5
    movdqu       (96)(%rdi), %xmm6
    movdqu       (112)(%rdi), %xmm7
    pmullw       (%rdx), %xmm0
    pmullw       (16)(%rdx), %xmm1
    pmullw       (32)(%rdx), %xmm2
    pmullw       (48)(%rdx), %xmm3
    pmullw       (64)(%rdx), %xmm4
    pmullw       (80)(%rdx), %xmm5
    pmullw       (96)(%rdx), %xmm6
    pmullw       (112)(%rdx), %xmm7
    movdqu       %xmm0, (%rsi)
    movdqu       %xmm1, (16)(%rsi)
    movdqu       %xmm2, (32)(%rsi)
    movdqu       %xmm3, (48)(%rsi)
    movdqu       %xmm4, (64)(%rsi)
    movdqu       %xmm5, (80)(%rsi)
    movdqu       %xmm6, (96)(%rsi)
    movdqu       %xmm7, (112)(%rsi)
    jmp          Lexitgas_1
LNAgas_1:  
    test         $(15), %rdi
    jnz          LNA1ngas_1
    test         $(15), %rsi
    jnz          LNA2ngas_1
    movdqu       (%rdx), %xmm0
    movdqu       (16)(%rdx), %xmm1
    movdqu       (32)(%rdx), %xmm2
    movdqu       (48)(%rdx), %xmm3
    movdqu       (64)(%rdx), %xmm4
    movdqu       (80)(%rdx), %xmm5
    movdqu       (96)(%rdx), %xmm6
    movdqu       (112)(%rdx), %xmm7
    pmullw       (%rdi), %xmm0
    pmullw       (16)(%rdi), %xmm1
    pmullw       (32)(%rdi), %xmm2
    pmullw       (48)(%rdi), %xmm3
    pmullw       (64)(%rdi), %xmm4
    pmullw       (80)(%rdi), %xmm5
    pmullw       (96)(%rdi), %xmm6
    pmullw       (112)(%rdi), %xmm7
    movdqa       %xmm0, (%rsi)
    movdqa       %xmm1, (16)(%rsi)
    movdqa       %xmm2, (32)(%rsi)
    movdqa       %xmm3, (48)(%rsi)
    movdqa       %xmm4, (64)(%rsi)
    movdqa       %xmm5, (80)(%rsi)
    movdqa       %xmm6, (96)(%rsi)
    movdqa       %xmm7, (112)(%rsi)
    jmp          Lexitgas_1
LNA1ngas_1:  
    test         $(15), %rsi
    jnz          LNA2ngas_1
    movdqu       (%rdi), %xmm0
    movdqu       (16)(%rdi), %xmm1
    movdqu       (32)(%rdi), %xmm2
    movdqu       (48)(%rdi), %xmm3
    movdqu       (64)(%rdi), %xmm4
    movdqu       (80)(%rdi), %xmm5
    movdqu       (96)(%rdi), %xmm6
    movdqu       (112)(%rdi), %xmm7
    movdqu       (%rdx), %xmm8
    movdqu       (16)(%rdx), %xmm9
    movdqu       (32)(%rdx), %xmm10
    movdqu       (48)(%rdx), %xmm11
    movdqu       (64)(%rdx), %xmm12
    movdqu       (80)(%rdx), %xmm13
    movdqu       (96)(%rdx), %xmm14
    movdqu       (112)(%rdx), %xmm15
    pmullw       %xmm8, %xmm0
    pmullw       %xmm9, %xmm1
    pmullw       %xmm10, %xmm2
    pmullw       %xmm11, %xmm3
    pmullw       %xmm12, %xmm4
    pmullw       %xmm13, %xmm5
    pmullw       %xmm14, %xmm6
    pmullw       %xmm15, %xmm7
    movdqa       %xmm0, (%rsi)
    movdqa       %xmm1, (16)(%rsi)
    movdqa       %xmm2, (32)(%rsi)
    movdqa       %xmm3, (48)(%rsi)
    movdqa       %xmm4, (64)(%rsi)
    movdqa       %xmm5, (80)(%rsi)
    movdqa       %xmm6, (96)(%rsi)
    movdqa       %xmm7, (112)(%rsi)
    jmp          Lexitgas_1
LNA2ngas_1:  
    movdqu       (%rdx), %xmm0
    movdqu       (16)(%rdx), %xmm1
    movdqu       (32)(%rdx), %xmm2
    movdqu       (48)(%rdx), %xmm3
    movdqu       (64)(%rdx), %xmm4
    movdqu       (80)(%rdx), %xmm5
    movdqu       (96)(%rdx), %xmm6
    movdqu       (112)(%rdx), %xmm7
    pmullw       (%rdi), %xmm0
    pmullw       (16)(%rdi), %xmm1
    pmullw       (32)(%rdi), %xmm2
    pmullw       (48)(%rdi), %xmm3
    pmullw       (64)(%rdi), %xmm4
    pmullw       (80)(%rdi), %xmm5
    pmullw       (96)(%rdi), %xmm6
    pmullw       (112)(%rdi), %xmm7
    movdqu       %xmm0, (%rsi)
    movdqu       %xmm1, (16)(%rsi)
    movdqu       %xmm2, (32)(%rsi)
    movdqu       %xmm3, (48)(%rsi)
    movdqu       %xmm4, (64)(%rsi)
    movdqu       %xmm5, (80)(%rsi)
    movdqu       %xmm6, (96)(%rsi)
    movdqu       %xmm7, (112)(%rsi)
Lexitgas_1:  
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownpj_QuantInv_8x8_16s_I

 
mfxownpj_QuantInv_8x8_16s_I:
 
 
    push         %rbx
 
 
    test         $(15), %rdi
    jz           Lalign_pSrcDstgas_2
    mov          %rdi, %rbx
    shr          $(1), %rbx
    mov          %rbx, %rcx
    neg          %rbx
    and          $(7), %rcx
    and          $(7), %rbx
Lnext_prgas_2:  
    movzwq       (%rdi), %rax
    movzwq       (%rsi), %rdx
    add          $(2), %rsi
    imul         %rdx, %rax
    movw         %ax, (%rdi)
    add          $(2), %rdi
    sub          $(1), %rbx
    jnz          Lnext_prgas_2
 
    test         $(15), %rsi
    jz           Lunalign_pSrcDst_align_pQTblgas_2
    movdqu       (%rsi), %xmm0
    movdqu       (16)(%rsi), %xmm2
    movdqu       (32)(%rsi), %xmm3
    movdqu       (48)(%rsi), %xmm4
    movdqa       (%rdi), %xmm6
    pmullw       %xmm0, %xmm6
    movdqa       (16)(%rdi), %xmm7
    movdqa       (32)(%rdi), %xmm1
    movdqa       (48)(%rdi), %xmm5
    movdqa       %xmm6, (%rdi)
    pmullw       %xmm2, %xmm7
    movdqa       %xmm7, (16)(%rdi)
    pmullw       %xmm3, %xmm1
    movdqa       (64)(%rdi), %xmm3
    pmullw       %xmm4, %xmm5
    movdqa       (80)(%rdi), %xmm4
    movdqa       %xmm1, (32)(%rdi)
    movdqa       %xmm5, (48)(%rdi)
    movdqu       (64)(%rsi), %xmm0
    pmullw       %xmm0, %xmm3
    movdqu       (80)(%rsi), %xmm1
    pmullw       %xmm1, %xmm4
    movdqu       (96)(%rsi), %xmm2
    movdqa       (96)(%rdi), %xmm5
    pmullw       %xmm2, %xmm5
    movdqa       %xmm3, (64)(%rdi)
    movdqa       %xmm4, (80)(%rdi)
    movdqa       %xmm5, (96)(%rdi)
Lnext_ep0gas_2:  
    movzwq       (112)(%rdi), %rax
    movzwq       (112)(%rsi), %rdx
    add          $(2), %rsi
    imul         %rdx, %rax
    movw         %ax, (112)(%rdi)
    add          $(2), %rdi
    sub          $(1), %rcx
    jnz          Lnext_ep0gas_2
    jmp          Lexitgas_2
 
Lunalign_pSrcDst_align_pQTblgas_2:  
    movdqa       (%rdi), %xmm0
    pmullw       (%rsi), %xmm0
    movdqa       (16)(%rdi), %xmm1
    pmullw       (16)(%rsi), %xmm1
    movdqa       (32)(%rdi), %xmm2
    pmullw       (32)(%rsi), %xmm2
    movdqa       (48)(%rdi), %xmm3
    pmullw       (48)(%rsi), %xmm3
    movdqa       (64)(%rdi), %xmm4
    pmullw       (64)(%rsi), %xmm4
    movdqa       (80)(%rdi), %xmm6
    pmullw       (80)(%rsi), %xmm6
    movdqa       (96)(%rdi), %xmm7
    pmullw       (96)(%rsi), %xmm7
    movdqa       %xmm0, (%rdi)
    movdqa       %xmm1, (16)(%rdi)
    movdqa       %xmm2, (32)(%rdi)
    movdqa       %xmm3, (48)(%rdi)
    movdqa       %xmm4, (64)(%rdi)
    movdqa       %xmm6, (80)(%rdi)
    movdqa       %xmm7, (96)(%rdi)
Lnext_ep1gas_2:  
    movzwq       (112)(%rdi), %rax
    movzwq       (112)(%rsi), %rdx
    add          $(2), %rsi
    imul         %rdx, %rax
    movw         %ax, (112)(%rdi)
    add          $(2), %rdi
    sub          $(1), %rcx
    jnz          Lnext_ep1gas_2
    jmp          Lexitgas_2
 
Lalign_pSrcDstgas_2:  
    test         $(15), %rsi
    jz           Lalign_pSrcDst_align_pQTblgas_2
    movdqu       (%rsi), %xmm6
    movdqa       (%rdi), %xmm2
    movdqu       (16)(%rsi), %xmm7
    movdqu       (32)(%rsi), %xmm0
    movdqu       (48)(%rsi), %xmm1
    pmullw       %xmm6, %xmm2
    movdqa       (16)(%rdi), %xmm3
    movdqa       (32)(%rdi), %xmm4
    movdqa       (48)(%rdi), %xmm5
    movdqa       (96)(%rdi), %xmm6
    pmullw       %xmm7, %xmm3
    movdqa       (112)(%rdi), %xmm7
    movdqa       %xmm2, (%rdi)
    movdqa       %xmm3, (16)(%rdi)
    pmullw       %xmm0, %xmm4
    pmullw       %xmm1, %xmm5
    movdqa       %xmm4, (32)(%rdi)
    movdqa       (64)(%rdi), %xmm4
    movdqa       %xmm5, (48)(%rdi)
    movdqu       (64)(%rsi), %xmm0
    pmullw       %xmm0, %xmm4
    movdqu       (80)(%rsi), %xmm1
    movdqu       (96)(%rsi), %xmm2
    pmullw       %xmm2, %xmm6
    movdqu       (112)(%rsi), %xmm3
    movdqa       (80)(%rdi), %xmm5
    pmullw       %xmm1, %xmm5
    movdqa       %xmm4, (64)(%rdi)
    movdqa       %xmm6, (96)(%rdi)
    pmullw       %xmm3, %xmm7
    movdqa       %xmm5, (80)(%rdi)
    movdqa       %xmm7, (112)(%rdi)
    jmp          Lexitgas_2
 
Lalign_pSrcDst_align_pQTblgas_2:  
    movdqa       (%rdi), %xmm0
    movdqa       (16)(%rdi), %xmm6
    pmullw       (%rsi), %xmm0
    pmullw       (16)(%rsi), %xmm6
    movdqa       (32)(%rdi), %xmm7
    movdqa       (48)(%rdi), %xmm1
    pmullw       (32)(%rsi), %xmm7
    pmullw       (48)(%rsi), %xmm1
    movdqa       (64)(%rdi), %xmm2
    movdqa       (80)(%rdi), %xmm3
    pmullw       (64)(%rsi), %xmm2
    pmullw       (80)(%rsi), %xmm3
    movdqa       (96)(%rdi), %xmm4
    movdqa       (112)(%rdi), %xmm5
    pmullw       (96)(%rsi), %xmm4
    pmullw       (112)(%rsi), %xmm5
    movdqa       %xmm0, (%rdi)
    movdqa       %xmm6, (16)(%rdi)
    movdqa       %xmm7, (32)(%rdi)
    movdqa       %xmm1, (48)(%rdi)
    movdqa       %xmm2, (64)(%rdi)
    movdqa       %xmm3, (80)(%rdi)
    movdqa       %xmm4, (96)(%rdi)
    movdqa       %xmm5, (112)(%rdi)
Lexitgas_2:  
 
 
    pop          %rbx
 
    ret
 
 
