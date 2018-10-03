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
 
 
CONST_3:
.quad     0x3000300030003,    0x3000300030003  
 
CONST_21:
.quad     0x2000100020001,    0x2000100020001  
 
CONST_78:
.quad     0x7000800070008,    0x7000800070008  
 
CONST_3_1:
.quad   0x103010301030103,  0x103010301030103  
 
MASK_V1_0:
.byte  2, 3, 2, 3, 4, 5, 4, 5, 10, 11, 10, 11, 12, 13, 12, 13  
 
MASK_V1_1:
.byte  0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15  
 
MASK_V2_0:
.byte  1, 0, 1, 2, 2, 1, 2, 3, 3, 2, 3, 4, 4, 3, 4, 5  
 
MASK_V2_1:
.byte  5, 4, 5, 6, 6, 5, 6, 7, 7, 6, 7, 8, 8, 7, 8, 9  
 
 
.text
 
 
.p2align 4, 0x90
 
.globl mfxownpj_SampleUpRowH2V1_Triangle_JPEG_8u_C1

 
mfxownpj_SampleUpRowH2V1_Triangle_JPEG_8u_C1:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    mov          %esi, %esi
 
    movzbq       (%rdi), %rax
    movzbq       (1)(%rdi), %rbx
    movb         %al, (%rdx)
    lea          (2)(%rax,%rax,2), %rax
    add          %rbx, %rax
    sar          $(2), %rax
    movb         %al, (1)(%rdx)
    add          $(2), %rdx
    sub          $(2), %rsi
    jz           Lexitgas_3
    cmp          $(32), %esi
    jl           LL1gas_3
 
    mov          %rdx, %rbp
    and          $(15), %rbp
    jz           LRungas_3
    sub          $(16), %rbp
    neg          %rbp
    test         $(1), %rbp
    jne          LNAgas_3
    sar          $(1), %rbp
    cmp          %rbp, %rsi
    jle          LRungas_3
    sub          %rbp, %rsi
LL1ngas_3:  
    movzbq       (%rdi), %rax
    movzbq       (1)(%rdi), %rbx
    movzbq       (2)(%rdi), %r8
    lea          (%rbx,%rbx,2), %rbx
    lea          (1)(%rbx,%rax), %rax
    lea          (2)(%r8,%rbx), %r8
    sar          $(2), %rax
    sar          $(2), %r8
    movb         %al, (%rdx)
    movb         %r8b, (1)(%rdx)
    add          $(1), %rdi
    add          $(2), %rdx
    sub          $(1), %rbp
    jnz          LL1ngas_3
LRungas_3:  
    sub          $(16), %rsi
    jl           LL8gas_3
LL16gas_3:  
    movq         (%rdi), %xmm0
    movq         (8)(%rdi), %xmm2
    punpcklqdq   %xmm2, %xmm0
    pinsrw       $(4), (16)(%rdi), %xmm2
    movdqa       %xmm0, %xmm1
    movdqa       %xmm2, %xmm3
    pshufb       MASK_V2_0(%rip), %xmm0
    pshufb       MASK_V2_1(%rip), %xmm1
    pshufb       MASK_V2_0(%rip), %xmm2
    pshufb       MASK_V2_1(%rip), %xmm3
    pmaddubsw    CONST_3_1(%rip), %xmm0
    pmaddubsw    CONST_3_1(%rip), %xmm1
    pmaddubsw    CONST_3_1(%rip), %xmm2
    pmaddubsw    CONST_3_1(%rip), %xmm3
    paddw        CONST_21(%rip), %xmm0
    paddw        CONST_21(%rip), %xmm1
    paddw        CONST_21(%rip), %xmm2
    paddw        CONST_21(%rip), %xmm3
    psrlw        $(2), %xmm0
    psrlw        $(2), %xmm1
    psrlw        $(2), %xmm2
    psrlw        $(2), %xmm3
    packuswb     %xmm1, %xmm0
    packuswb     %xmm3, %xmm2
    movdqa       %xmm0, (%rdx)
    movdqa       %xmm2, (16)(%rdx)
    add          $(16), %rdi
    add          $(32), %rdx
    sub          $(16), %rsi
    jge          LL16gas_3
LL8gas_3:  
    add          $(8), %rsi
    jl           LL7gas_3
    movq         (%rdi), %xmm0
    pinsrw       $(4), (8)(%rdi), %xmm0
    movdqa       %xmm0, %xmm1
    pshufb       MASK_V2_0(%rip), %xmm0
    pshufb       MASK_V2_1(%rip), %xmm1
    pmaddubsw    CONST_3_1(%rip), %xmm0
    pmaddubsw    CONST_3_1(%rip), %xmm1
    paddw        CONST_21(%rip), %xmm0
    paddw        CONST_21(%rip), %xmm1
    psrlw        $(2), %xmm0
    psrlw        $(2), %xmm1
    packuswb     %xmm1, %xmm0
    movdqa       %xmm0, (%rdx)
    add          $(8), %rdi
    add          $(16), %rdx
    sub          $(8), %rsi
    jmp          LL7gas_3
LNAgas_3:  
    sub          $(16), %rsi
    jl           LL8ngas_3
LL16ngas_3:  
    movq         (%rdi), %xmm0
    movq         (8)(%rdi), %xmm2
    punpcklqdq   %xmm2, %xmm0
    pinsrw       $(4), (16)(%rdi), %xmm2
    movdqa       %xmm0, %xmm1
    movdqa       %xmm2, %xmm3
    pshufb       MASK_V2_0(%rip), %xmm0
    pshufb       MASK_V2_1(%rip), %xmm1
    pshufb       MASK_V2_0(%rip), %xmm2
    pshufb       MASK_V2_1(%rip), %xmm3
    pmaddubsw    CONST_3_1(%rip), %xmm0
    pmaddubsw    CONST_3_1(%rip), %xmm1
    pmaddubsw    CONST_3_1(%rip), %xmm2
    pmaddubsw    CONST_3_1(%rip), %xmm3
    paddw        CONST_21(%rip), %xmm0
    paddw        CONST_21(%rip), %xmm1
    paddw        CONST_21(%rip), %xmm2
    paddw        CONST_21(%rip), %xmm3
    psrlw        $(2), %xmm0
    psrlw        $(2), %xmm1
    psrlw        $(2), %xmm2
    psrlw        $(2), %xmm3
    packuswb     %xmm1, %xmm0
    packuswb     %xmm3, %xmm2
    movdqu       %xmm0, (%rdx)
    movdqu       %xmm2, (16)(%rdx)
    add          $(16), %rdi
    add          $(32), %rdx
    sub          $(16), %rsi
    jge          LL16ngas_3
LL8ngas_3:  
    add          $(8), %rsi
    jl           LL7gas_3
    movq         (%rdi), %xmm0
    pinsrw       $(4), (8)(%rdi), %xmm0
    movdqa       %xmm0, %xmm1
    pshufb       MASK_V2_0(%rip), %xmm0
    pshufb       MASK_V2_1(%rip), %xmm1
    pmaddubsw    CONST_3_1(%rip), %xmm0
    pmaddubsw    CONST_3_1(%rip), %xmm1
    paddw        CONST_21(%rip), %xmm0
    paddw        CONST_21(%rip), %xmm1
    psrlw        $(2), %xmm0
    psrlw        $(2), %xmm1
    packuswb     %xmm1, %xmm0
    movdqu       %xmm0, (%rdx)
    add          $(8), %rdi
    add          $(16), %rdx
    sub          $(8), %rsi
LL7gas_3:  
    add          $(8), %rsi
    jle          Lexitgas_3
LL1gas_3:  
    movzbq       (%rdi), %rax
    movzbq       (1)(%rdi), %rbx
    movzbq       (2)(%rdi), %r8
    lea          (%rbx,%rbx,2), %rbx
    lea          (1)(%rbx,%rax), %rax
    lea          (2)(%r8,%rbx), %r8
    sar          $(2), %rax
    sar          $(2), %r8
    movb         %al, (%rdx)
    movb         %r8b, (1)(%rdx)
    add          $(1), %rdi
    add          $(2), %rdx
    sub          $(1), %rsi
    jnz          LL1gas_3
Lexitgas_3:  
 
    movzbq       (1)(%rdi), %rax
    movzbq       (%rdi), %rbx
    movb         %al, (1)(%rdx)
    lea          (1)(%rax,%rax,2), %rax
    add          %rbx, %rax
    sar          $(2), %rax
    movb         %al, (%rdx)
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownpj_SampleUpRowH2V2_Triangle_JPEG_8u_C1

 
mfxownpj_SampleUpRowH2V2_Triangle_JPEG_8u_C1:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    mov          %edx, %edx
 
    movzbq       (1)(%rdi), %rax
    movzbq       (1)(%rsi), %rbx
    lea          (%rax,%rax,2), %rax
    add          %rax, %rbx
    movzbq       (%rdi), %rax
    movzbq       (%rsi), %r8
    lea          (%rax,%rax,2), %rax
    add          %r8, %rax
    mov          %rax, %r8
    lea          (7)(%rax,%rax,2), %rax
    lea          (8)(,%r8,4), %r8
    add          %rbx, %rax
    sar          $(4), %r8
    sar          $(4), %rax
    movb         %r8b, (%rcx)
    movb         %al, (1)(%rcx)
    add          $(2), %rcx
    sub          $(2), %rdx
    jz           Lexitgas_4
    mov          %rcx, %rbp
    and          $(15), %rbp
    jz           LRungas_4
    sub          $(16), %rbp
    neg          %rbp
    test         $(1), %rbp
    jne          LNAgas_4
    sar          $(1), %rbp
    cmp          %rbp, %rdx
    jle          LRungas_4
    sub          %rbp, %rdx
LL1ngas_4:  
    movzbq       (2)(%rdi), %rax
    movzbq       (2)(%rsi), %rbx
    lea          (%rax,%rax,2), %rax
    add          %rbx, %rax
    movzbq       (1)(%rdi), %r8
    movzbq       (1)(%rsi), %rbx
    lea          (%r8,%r8,2), %r8
    add          %rbx, %r8
    lea          (7)(%r8,%r8,2), %r8
    add          %r8, %rax
    sar          $(4), %rax
    movb         %al, (1)(%rcx)
    movzbq       (%rdi), %rax
    movzbq       (%rsi), %rbx
    lea          (%rax,%rax,2), %rax
    add          %rbx, %rax
    lea          (1)(%r8,%rax), %rax
    sar          $(4), %rax
    movb         %al, (%rcx)
    add          $(1), %rdi
    add          $(1), %rsi
    add          $(2), %rcx
    sub          $(1), %rbp
    jnz          LL1ngas_4
 
 
LRungas_4:  
    sub          $(8), %rdx
    jl           LL7gas_4
LL8gas_4:  
    movq         (%rdi), %xmm0
    movq         (2)(%rdi), %xmm1
    movq         (%rsi), %xmm2
    movq         (2)(%rsi), %xmm3
    punpcklbw    %xmm2, %xmm0
    punpcklbw    %xmm3, %xmm1
    pmaddubsw    CONST_3_1(%rip), %xmm0
    pmaddubsw    CONST_3_1(%rip), %xmm1
    movdqa       %xmm0, %xmm2
    movdqa       %xmm1, %xmm3
    pshufb       MASK_V1_0(%rip), %xmm0
    pshufb       MASK_V1_0(%rip), %xmm1
    pshufb       MASK_V1_1(%rip), %xmm2
    pshufb       MASK_V1_1(%rip), %xmm3
    pmullw       CONST_3(%rip), %xmm0
    pmullw       CONST_3(%rip), %xmm1
    paddw        %xmm2, %xmm0
    paddw        %xmm3, %xmm1
    movdqa       %xmm0, %xmm2
    punpcklqdq   %xmm1, %xmm0
    punpckhqdq   %xmm1, %xmm2
    paddw        CONST_78(%rip), %xmm0
    paddw        CONST_78(%rip), %xmm2
    psrlw        $(4), %xmm0
    psrlw        $(4), %xmm2
    packuswb     %xmm2, %xmm0
    movdqa       %xmm0, (%rcx)
    add          $(8), %rdi
    add          $(8), %rsi
    add          $(16), %rcx
    sub          $(8), %rdx
    jge          LL8gas_4
    jmp          LL7gas_4
LNAgas_4:  
    sub          $(8), %rdx
    jl           LL7gas_4
LL8ngas_4:  
    movq         (%rdi), %xmm0
    movq         (2)(%rdi), %xmm1
    movq         (%rsi), %xmm2
    movq         (2)(%rsi), %xmm3
    punpcklbw    %xmm2, %xmm0
    punpcklbw    %xmm3, %xmm1
    pmaddubsw    CONST_3_1(%rip), %xmm0
    pmaddubsw    CONST_3_1(%rip), %xmm1
    movdqa       %xmm0, %xmm2
    movdqa       %xmm1, %xmm3
    pshufb       MASK_V1_0(%rip), %xmm0
    pshufb       MASK_V1_0(%rip), %xmm1
    pshufb       MASK_V1_1(%rip), %xmm2
    pshufb       MASK_V1_1(%rip), %xmm3
    pmullw       CONST_3(%rip), %xmm0
    pmullw       CONST_3(%rip), %xmm1
    paddw        %xmm2, %xmm0
    paddw        %xmm3, %xmm1
    movdqa       %xmm0, %xmm2
    punpcklqdq   %xmm1, %xmm0
    punpckhqdq   %xmm1, %xmm2
    paddw        CONST_78(%rip), %xmm0
    paddw        CONST_78(%rip), %xmm2
    psrlw        $(4), %xmm0
    psrlw        $(4), %xmm2
    packuswb     %xmm2, %xmm0
    movdqu       %xmm0, (%rcx)
    add          $(8), %rdi
    add          $(8), %rsi
    add          $(16), %rcx
    sub          $(8), %rdx
    jge          LL8ngas_4
LL7gas_4:  
    add          $(8), %rdx
    jle          Lexitgas_4
LL1gas_4:  
    movzbq       (2)(%rdi), %rax
    movzbq       (2)(%rsi), %rbx
    lea          (%rax,%rax,2), %rax
    add          %rbx, %rax
    movzbq       (1)(%rdi), %r8
    movzbq       (1)(%rsi), %rbx
    lea          (%r8,%r8,2), %r8
    add          %rbx, %r8
    lea          (7)(%r8,%r8,2), %r8
    add          %r8, %rax
    sar          $(4), %rax
    movb         %al, (1)(%rcx)
    movzbq       (%rdi), %rax
    movzbq       (%rsi), %rbx
    lea          (%rax,%rax,2), %rax
    add          %rbx, %rax
    lea          (1)(%r8,%rax), %rax
    sar          $(4), %rax
    movb         %al, (%rcx)
    add          $(1), %rdi
    add          $(1), %rsi
    add          $(2), %rcx
    sub          $(1), %rdx
    jnz          LL1gas_4
Lexitgas_4:  
 
    movzbq       (1)(%rdi), %rax
    movzbq       (1)(%rsi), %rbx
    lea          (%rax,%rax,2), %rax
    add          %rbx, %rax
    mov          %rax, %r8
    movzbq       (%rdi), %rbx
    movzbq       (%rsi), %rdx
    lea          (%rax,%rax,2), %rax
    lea          (%rbx,%rbx,2), %rbx
    lea          (7)(,%r8,4), %r8
    add          %rdx, %rbx
    sar          $(4), %r8
    lea          (8)(%rbx,%rax), %rax
    sar          $(4), %rax
    movb         %r8b, (1)(%rcx)
    movb         %al, (%rcx)
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
