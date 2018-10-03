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
 
 
CONST_128:
.quad     0x80008000800080,    0x80008000800080  
 
CONST_0:
.quad  0, 0  
 
 
.text
 
 
.p2align 4, 0x90
 
.globl mfxownpj_Sub128_8x8_8u16s

 
mfxownpj_Sub128_8x8_8u16s:
 
 
    push         %rbx
 
 
    mov          %esi, %esi
    mov          %rsi, %rbx
    mov          %rsi, %rax
    sal          $(1), %rbx
    add          %rbx, %rax
    pxor         %xmm4, %xmm4
    mov          $(2), %rcx
    test         $(15), %rdx
    jnz          LL1ngas_2
LL1gas_2:  
    movq         (%rdi), %xmm0
    movq         (%rdi,%rsi), %xmm1
    movq         (%rdi,%rbx), %xmm2
    movq         (%rdi,%rax), %xmm3
    punpcklbw    %xmm4, %xmm0
    punpcklbw    %xmm4, %xmm1
    punpcklbw    %xmm4, %xmm2
    punpcklbw    %xmm4, %xmm3
    psubw        CONST_128(%rip), %xmm0
    psubw        CONST_128(%rip), %xmm1
    psubw        CONST_128(%rip), %xmm2
    psubw        CONST_128(%rip), %xmm3
    movdqa       %xmm0, (%rdx)
    movdqa       %xmm1, (16)(%rdx)
    movdqa       %xmm2, (32)(%rdx)
    movdqa       %xmm3, (48)(%rdx)
    add          %rbx, %rdi
    add          %rbx, %rdi
    add          $(64), %rdx
    sub          $(1), %rcx
    jnz          LL1gas_2
    jmp          Lexitgas_2
LL1ngas_2:  
    movq         (%rdi), %xmm0
    movq         (%rdi,%rsi), %xmm1
    movq         (%rdi,%rbx), %xmm2
    movq         (%rdi,%rax), %xmm3
    punpcklbw    %xmm4, %xmm0
    punpcklbw    %xmm4, %xmm1
    punpcklbw    %xmm4, %xmm2
    punpcklbw    %xmm4, %xmm3
    psubw        CONST_128(%rip), %xmm0
    psubw        CONST_128(%rip), %xmm1
    psubw        CONST_128(%rip), %xmm2
    psubw        CONST_128(%rip), %xmm3
    movdqu       %xmm0, (%rdx)
    movdqu       %xmm1, (16)(%rdx)
    movdqu       %xmm2, (32)(%rdx)
    movdqu       %xmm3, (48)(%rdx)
    add          %rbx, %rdi
    add          %rbx, %rdi
    add          $(64), %rdx
    sub          $(1), %rcx
    jnz          LL1ngas_2
Lexitgas_2:  
 
 
    pop          %rbx
 
    ret
