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
 
 
sMask16:
.quad       0xFFFFFFFFFFFF,      0xFFFFFFFFFFFF  
 
dMask16:
.quad   0xFFFF000000000000,  0xFFFF000000000000  
 
 
.text
 
 
.p2align 4, 0x90
 
.globl mfxownpi_AndC_16u_C1R

 
mfxownpi_AndC_16u_C1R:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    push         %r12
 
 
    movslq       %edx, %rdx
    movslq       %r8d, %r8
    mov          %rdi, %rax
    shl          $(16), %rdi
    and          $(65535), %rax
    or           %rdi, %rax
    mov          %eax, %eax
    movd         %eax, %xmm7
    mov          %rax, %rdi
    shl          $(32), %rdi
    or           %rax, %rdi
    pshufd       $(0), %xmm7, %xmm7
    movdqa       %xmm7, %xmm6
    mov          %r9d, %r9d
    movl         (32)(%rsp), %ebp
LL01gas_1:  
    mov          %rsi, %r10
    mov          %rcx, %r11
    mov          %r9, %r12
    test         $(1), %r11
    jz           LRunNgas_1
    sub          $(4), %r12
    jl           LL2gas_1
LL4ngas_1:  
    mov          (%r10), %rax
    and          %rdi, %rax
    mov          %rax, (%r11)
    add          $(8), %r10
    add          $(8), %r11
    sub          $(4), %r12
    jge          LL4ngas_1
    jmp          LL2gas_1
LRunNgas_1:  
    mov          %r11, %rbx
    and          $(15), %rbx
    jz           LRungas_1
    sub          $(16), %rbx
    neg          %rbx
    shr          $(1), %rbx
    cmp          %rbx, %r12
    jl           LRungas_1
    sub          %rbx, %r12
LLN1gas_1:  
    movw         (%r10), %ax
    and          %rdi, %rax
    movw         %ax, (%r11)
    add          $(2), %r10
    add          $(2), %r11
    sub          $(1), %rbx
    jnz          LLN1gas_1
LRungas_1:  
    test         $(15), %r10
    jnz          LNNAgas_1
    sub          $(32), %r12
    jl           LL16gas_1
LL32gas_1:  
    movdqa       (%r10), %xmm0
    movdqa       (16)(%r10), %xmm1
    movdqa       (32)(%r10), %xmm2
    movdqa       (48)(%r10), %xmm3
    pand         %xmm6, %xmm0
    pand         %xmm7, %xmm1
    pand         %xmm6, %xmm2
    pand         %xmm7, %xmm3
    movdqa       %xmm0, (%r11)
    movdqa       %xmm1, (16)(%r11)
    movdqa       %xmm2, (32)(%r11)
    movdqa       %xmm3, (48)(%r11)
    add          $(64), %r10
    add          $(64), %r11
    sub          $(32), %r12
    jge          LL32gas_1
LL16gas_1:  
    add          $(16), %r12
    jl           LL8gas_1
    movdqa       (%r10), %xmm4
    movdqa       (16)(%r10), %xmm5
    pand         %xmm6, %xmm4
    pand         %xmm7, %xmm5
    movdqa       %xmm4, (%r11)
    movdqa       %xmm5, (16)(%r11)
    add          $(32), %r10
    add          $(32), %r11
    sub          $(16), %r12
LL8gas_1:  
    add          $(8), %r12
    jl           LL4gas_1
    movdqa       (%r10), %xmm0
    pand         %xmm6, %xmm0
    movdqa       %xmm0, (%r11)
    add          $(16), %r10
    add          $(16), %r11
    sub          $(8), %r12
    jmp          LL4gas_1
LNNAgas_1:  
    sub          $(32), %r12
    jl           LL16ngas_1
LL32ngas_1:  
    lddqu        (%r10), %xmm0
    lddqu        (16)(%r10), %xmm1
    lddqu        (32)(%r10), %xmm2
    lddqu        (48)(%r10), %xmm3
    pand         %xmm6, %xmm0
    pand         %xmm7, %xmm1
    pand         %xmm6, %xmm2
    pand         %xmm7, %xmm3
    movdqa       %xmm0, (%r11)
    movdqa       %xmm1, (16)(%r11)
    movdqa       %xmm2, (32)(%r11)
    movdqa       %xmm3, (48)(%r11)
    add          $(64), %r10
    add          $(64), %r11
    sub          $(32), %r12
    jge          LL32ngas_1
LL16ngas_1:  
    add          $(16), %r12
    jl           LL8ngas_1
    lddqu        (%r10), %xmm4
    lddqu        (16)(%r10), %xmm5
    pand         %xmm6, %xmm4
    pand         %xmm7, %xmm5
    movdqa       %xmm4, (%r11)
    movdqa       %xmm5, (16)(%r11)
    add          $(32), %r10
    add          $(32), %r11
    sub          $(16), %r12
LL8ngas_1:  
    add          $(8), %r12
    jl           LL4gas_1
    lddqu        (%r10), %xmm0
    pand         %xmm6, %xmm0
    movdqa       %xmm0, (%r11)
    add          $(16), %r10
    add          $(16), %r11
    sub          $(8), %r12
LL4gas_1:  
    add          $(4), %r12
    jl           LL2gas_1
    mov          (%r10), %rax
    and          %rdi, %rax
    mov          %rax, (%r11)
    add          $(8), %r10
    add          $(8), %r11
    sub          $(4), %r12
LL2gas_1:  
    add          $(2), %r12
    jl           LL1gas_1
    movl         (%r10), %eax
    and          %rdi, %rax
    movl         %eax, (%r11)
    add          $(4), %r10
    add          $(4), %r11
    sub          $(2), %r12
LL1gas_1:  
    add          $(1), %r12
    jl           LL02gas_1
    movw         (%r10), %ax
    and          %rdi, %rax
    movw         %ax, (%r11)
LL02gas_1:  
    add          %rdx, %rsi
    add          %r8, %rcx
    sub          $(1), %ebp
    jnz          LL01gas_1
 
 
    pop          %r12
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownpi_AndC_16u_C3R

 
mfxownpi_AndC_16u_C3R:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    push         %r12
 
 
    movslq       %edx, %rdx
    movslq       %r8d, %r8
    movd         (%rdi), %xmm7
    pinsrw       $(2), (4)(%rdi), %xmm7
    movdqa       %xmm7, %xmm2
    movdqa       %xmm7, %xmm3
    movdqa       %xmm7, %xmm4
    movdqa       %xmm7, %xmm5
    movdqa       %xmm7, %xmm6
    psllq        $(16), %xmm2
    psllq        $(32), %xmm3
    psllq        $(48), %xmm4
    psrlq        $(16), %xmm5
    psrlq        $(32), %xmm6
    por          %xmm6, %xmm2
    por          %xmm7, %xmm4
    por          %xmm5, %xmm3
    unpcklpd     %xmm3, %xmm4
    unpcklpd     %xmm4, %xmm2
    unpcklpd     %xmm2, %xmm3
    mov          %r9d, %r9d
    movl         (32)(%rsp), %ebp
LL01gas_2:  
    mov          %rsi, %r10
    mov          %rcx, %r11
    mov          %r9, %r12
    cmp          $(32), %r12
    jl           LNSgas_2
    test         $(1), %r11
    jz           LRunNgas_2
LNSgas_2:  
    sub          $(4), %r12
    jl           LL3gas_2
LNAgas_2:  
    movq         (%r10), %xmm5
    movq         (8)(%r10), %xmm6
    movq         (16)(%r10), %xmm7
    pand         %xmm4, %xmm5
    pand         %xmm3, %xmm6
    pand         %xmm2, %xmm7
    movq         %xmm5, (%r11)
    movq         %xmm6, (8)(%r11)
    movq         %xmm7, (16)(%r11)
    add          $(24), %r10
    add          $(24), %r11
    sub          $(4), %r12
    jge          LNAgas_2
    jmp          LL3gas_2
LRunNgas_2:  
    mov          %r11, %rbx
    and          $(15), %rbx
    jz           LRungas_2
    cmp          $(14), %rbx
    jl           Lt12gas_2
    mov          $(3), %rbx
    jmp          Lt0gas_2
Lt12gas_2:  
    cmp          $(12), %rbx
    jl           Lt10gas_2
    mov          $(6), %rbx
    jmp          Lt0gas_2
Lt10gas_2:  
    cmp          $(10), %rbx
    jl           Lt8gas_2
    mov          $(1), %rbx
    jmp          Lt0gas_2
Lt8gas_2:  
    cmp          $(8), %rbx
    jl           Lt6gas_2
    mov          $(4), %rbx
    jmp          Lt0gas_2
Lt6gas_2:  
    cmp          $(6), %rbx
    jl           Lt4gas_2
    mov          $(7), %rbx
    jmp          Lt0gas_2
Lt4gas_2:  
    cmp          $(4), %rbx
    jl           Lt2gas_2
    mov          $(2), %rbx
    jmp          Lt0gas_2
Lt2gas_2:  
    mov          $(5), %rbx
Lt0gas_2:  
    sub          %rbx, %r12
LL1ngas_2:  
    movd         (%r10), %xmm0
    pinsrw       $(2), (4)(%r10), %xmm0
    pand         %xmm4, %xmm0
    movd         %xmm0, (%r11)
    psrlq        $(32), %xmm0
    movd         %xmm0, %eax
    movw         %ax, (4)(%r11)
    add          $(6), %r10
    add          $(6), %r11
    sub          $(1), %rbx
    jnz          LL1ngas_2
LRungas_2:  
    test         $(15), %r10
    jnz          LNNAgas_2
    sub          $(8), %r12
    jl           LL4gas_2
LL8gas_2:  
    movdqa       (%r10), %xmm5
    movdqa       (16)(%r10), %xmm6
    movdqa       (32)(%r10), %xmm7
    pand         %xmm4, %xmm5
    pand         %xmm2, %xmm6
    pand         %xmm3, %xmm7
    movdqa       %xmm5, (%r11)
    movdqa       %xmm6, (16)(%r11)
    movdqa       %xmm7, (32)(%r11)
    add          $(48), %r10
    add          $(48), %r11
    sub          $(8), %r12
    jge          LL8gas_2
LL4gas_2:  
    add          $(4), %r12
    jl           LL3gas_2
    movdqa       (%r10), %xmm0
    movq         (16)(%r10), %xmm1
    pand         %xmm4, %xmm0
    pand         %xmm2, %xmm1
    movdqa       %xmm0, (%r11)
    movq         %xmm1, (16)(%r11)
    add          $(24), %r10
    add          $(24), %r11
    sub          $(4), %r12
    jmp          LL3gas_2
LNNAgas_2:  
    sub          $(8), %r12
    jl           LL4ngas_2
LL8ngas_2:  
    lddqu        (%r10), %xmm5
    lddqu        (16)(%r10), %xmm6
    lddqu        (32)(%r10), %xmm7
    pand         %xmm4, %xmm5
    pand         %xmm2, %xmm6
    pand         %xmm3, %xmm7
    movdqa       %xmm5, (%r11)
    movdqa       %xmm6, (16)(%r11)
    movdqa       %xmm7, (32)(%r11)
    add          $(48), %r10
    add          $(48), %r11
    sub          $(8), %r12
    jge          LL8ngas_2
LL4ngas_2:  
    add          $(4), %r12
    jl           LL3gas_2
    lddqu        (%r10), %xmm0
    movq         (16)(%r10), %xmm1
    pand         %xmm4, %xmm0
    pand         %xmm2, %xmm1
    movdqa       %xmm0, (%r11)
    movq         %xmm1, (16)(%r11)
    add          $(24), %r10
    add          $(24), %r11
    sub          $(4), %r12
LL3gas_2:  
    add          $(4), %r12
    jz           LL02gas_2
LL1gas_2:  
    movd         (%r10), %xmm0
    pinsrw       $(2), (4)(%r10), %xmm0
    pand         %xmm4, %xmm0
    movd         %xmm0, (%r11)
    psrlq        $(32), %xmm0
    movd         %xmm0, %eax
    movw         %ax, (4)(%r11)
    add          $(6), %r10
    add          $(6), %r11
    sub          $(1), %r12
    jnz          LL1gas_2
LL02gas_2:  
    add          %rdx, %rsi
    add          %r8, %rcx
    sub          $(1), %ebp
    jnz          LL01gas_2
 
 
    pop          %r12
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownpi_AndC_16u_C4R

 
mfxownpi_AndC_16u_C4R:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    push         %r12
 
 
    movslq       %edx, %rdx
    movslq       %r8d, %r8
    movq         (%rdi), %xmm7
    pshufd       $(68), %xmm7, %xmm7
    mov          %r9d, %r9d
    movl         (32)(%rsp), %ebp
LL01gas_3:  
    mov          %rsi, %r10
    mov          %rcx, %r11
    mov          %r9, %r12
    test         $(7), %r11
    jz           LRunNgas_3
LNAgas_3:  
    movq         (%r10), %xmm0
    pand         %xmm7, %xmm0
    movq         %xmm0, (%r11)
    add          $(8), %r10
    add          $(8), %r11
    sub          $(1), %r12
    jnz          LNAgas_3
    jmp          LL02gas_3
LRunNgas_3:  
    mov          %r11, %rbx
    and          $(15), %rbx
    jz           LRungas_3
    movq         (%r10), %xmm0
    pand         %xmm7, %xmm0
    movq         %xmm0, (%r11)
    add          $(8), %r10
    add          $(8), %r11
    sub          $(1), %r12
LRungas_3:  
    test         $(15), %r10
    jnz          LNNAgas_3
    sub          $(8), %r12
    jl           LL4gas_3
LL8gas_3:  
    movdqa       (%r10), %xmm0
    movdqa       (16)(%r10), %xmm1
    movdqa       (32)(%r10), %xmm2
    movdqa       (48)(%r10), %xmm3
    pand         %xmm7, %xmm0
    pand         %xmm7, %xmm1
    pand         %xmm7, %xmm2
    pand         %xmm7, %xmm3
    movdqa       %xmm0, (%r11)
    movdqa       %xmm1, (16)(%r11)
    movdqa       %xmm2, (32)(%r11)
    movdqa       %xmm3, (48)(%r11)
    add          $(64), %r10
    add          $(64), %r11
    sub          $(8), %r12
    jge          LL8gas_3
LL4gas_3:  
    add          $(4), %r12
    jl           LL2gas_3
    movdqa       (%r10), %xmm4
    movdqa       (16)(%r10), %xmm5
    pand         %xmm7, %xmm4
    pand         %xmm7, %xmm5
    movdqa       %xmm4, (%r11)
    movdqa       %xmm5, (16)(%r11)
    add          $(32), %r10
    add          $(32), %r11
    sub          $(4), %r12
LL2gas_3:  
    add          $(2), %r12
    jl           LL1gas_3
    movdqa       (%r10), %xmm6
    pand         %xmm7, %xmm6
    movdqa       %xmm6, (%r11)
    add          $(16), %r10
    add          $(16), %r11
    sub          $(2), %r12
    jmp          LL1gas_3
LNNAgas_3:  
    sub          $(8), %r12
    jl           LL4ngas_3
LL8ngas_3:  
    lddqu        (%r10), %xmm0
    lddqu        (16)(%r10), %xmm1
    lddqu        (32)(%r10), %xmm2
    lddqu        (48)(%r10), %xmm3
    pand         %xmm7, %xmm0
    pand         %xmm7, %xmm1
    pand         %xmm7, %xmm2
    pand         %xmm7, %xmm3
    movdqa       %xmm0, (%r11)
    movdqa       %xmm1, (16)(%r11)
    movdqa       %xmm2, (32)(%r11)
    movdqa       %xmm3, (48)(%r11)
    add          $(64), %r10
    add          $(64), %r11
    sub          $(8), %r12
    jge          LL8ngas_3
LL4ngas_3:  
    add          $(4), %r12
    jl           LL2ngas_3
    lddqu        (%r10), %xmm4
    lddqu        (16)(%r10), %xmm5
    pand         %xmm7, %xmm4
    pand         %xmm7, %xmm5
    movdqa       %xmm4, (%r11)
    movdqa       %xmm5, (16)(%r11)
    add          $(32), %r10
    add          $(32), %r11
    sub          $(4), %r12
LL2ngas_3:  
    add          $(2), %r12
    jl           LL1gas_3
    lddqu        (%r10), %xmm6
    pand         %xmm7, %xmm6
    movdqa       %xmm6, (%r11)
    add          $(16), %r10
    add          $(16), %r11
    sub          $(2), %r12
LL1gas_3:  
    add          $(1), %r12
    jl           LL02gas_3
    movq         (%r10), %xmm0
    pand         %xmm7, %xmm0
    movq         %xmm0, (%r11)
LL02gas_3:  
    add          %rdx, %rsi
    add          %r8, %rcx
    sub          $(1), %ebp
    jnz          LL01gas_3
 
 
    pop          %r12
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownpi_AndC_16u_AC4R

 
mfxownpi_AndC_16u_AC4R:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    push         %r12
 
 
    movslq       %edx, %rdx
    movslq       %r8d, %r8
    movd         (%rdi), %xmm7
    pinsrw       $(2), (4)(%rdi), %xmm7
    pshufd       $(68), %xmm7, %xmm7
    mov          %r9d, %r9d
    movl         (32)(%rsp), %ebp
LL01gas_4:  
    mov          %rsi, %r10
    mov          %rcx, %r11
    mov          %r9, %r12
    test         $(7), %r11
    jz           LRunNgas_4
LNAgas_4:  
    movq         (%r10), %xmm0
    movq         (%r11), %xmm1
    pand         %xmm7, %xmm0
    pand         dMask16(%rip), %xmm1
    pand         sMask16(%rip), %xmm0
    por          %xmm1, %xmm0
    movq         %xmm0, (%r11)
    add          $(8), %r10
    add          $(8), %r11
    sub          $(1), %r12
    jnz          LNAgas_4
    jmp          LL02gas_4
LRunNgas_4:  
    mov          %r11, %rbx
    and          $(15), %rbx
    jz           LRungas_4
    movq         (%r10), %xmm0
    movq         (%r11), %xmm1
    pand         %xmm7, %xmm0
    pand         dMask16(%rip), %xmm1
    pand         sMask16(%rip), %xmm0
    por          %xmm1, %xmm0
    movq         %xmm0, (%r11)
    add          $(8), %r10
    add          $(8), %r11
    sub          $(1), %r12
LRungas_4:  
    test         $(15), %r10
    jnz          LNNAgas_4
    sub          $(4), %r12
    jl           LL2gas_4
LL4gas_4:  
    movdqa       (%r10), %xmm0
    movdqa       (16)(%r10), %xmm1
    movdqa       (%r11), %xmm2
    movdqa       (16)(%r11), %xmm3
    pand         %xmm7, %xmm0
    pand         %xmm7, %xmm1
    pand         dMask16(%rip), %xmm2
    pand         dMask16(%rip), %xmm3
    pand         sMask16(%rip), %xmm0
    pand         sMask16(%rip), %xmm1
    por          %xmm2, %xmm0
    por          %xmm3, %xmm1
    movdqa       %xmm0, (%r11)
    movdqa       %xmm1, (16)(%r11)
    add          $(32), %r10
    add          $(32), %r11
    sub          $(4), %r12
    jge          LL4gas_4
LL2gas_4:  
    add          $(2), %r12
    jl           LL1gas_4
    movdqa       (%r10), %xmm4
    movdqa       (%r11), %xmm5
    pand         %xmm7, %xmm4
    pand         dMask16(%rip), %xmm5
    pand         sMask16(%rip), %xmm4
    por          %xmm5, %xmm4
    movdqa       %xmm4, (%r11)
    add          $(16), %r10
    add          $(16), %r11
    sub          $(2), %r12
    jmp          LL1gas_4
LNNAgas_4:  
    sub          $(4), %r12
    jl           LL2ngas_4
LL4ngas_4:  
    lddqu        (%r10), %xmm0
    lddqu        (16)(%r10), %xmm1
    movdqa       (%r11), %xmm2
    movdqa       (16)(%r11), %xmm3
    pand         %xmm7, %xmm0
    pand         %xmm7, %xmm1
    pand         dMask16(%rip), %xmm2
    pand         dMask16(%rip), %xmm3
    pand         sMask16(%rip), %xmm0
    pand         sMask16(%rip), %xmm1
    por          %xmm2, %xmm0
    por          %xmm3, %xmm1
    movdqa       %xmm0, (%r11)
    movdqa       %xmm1, (16)(%r11)
    add          $(32), %r10
    add          $(32), %r11
    sub          $(4), %r12
    jge          LL4ngas_4
LL2ngas_4:  
    add          $(2), %r12
    jl           LL1gas_4
    lddqu        (%r10), %xmm4
    movdqa       (%r11), %xmm5
    pand         %xmm7, %xmm4
    pand         dMask16(%rip), %xmm5
    pand         sMask16(%rip), %xmm4
    por          %xmm5, %xmm4
    movdqa       %xmm4, (%r11)
    add          $(16), %r10
    add          $(16), %r11
    sub          $(2), %r12
LL1gas_4:  
    add          $(1), %r12
    jl           LL02gas_4
    movq         (%r10), %xmm6
    movq         (%r11), %xmm1
    pand         %xmm7, %xmm6
    pand         dMask16(%rip), %xmm1
    pand         sMask16(%rip), %xmm6
    por          %xmm1, %xmm6
    movq         %xmm6, (%r11)
LL02gas_4:  
    add          %rdx, %rsi
    add          %r8, %rcx
    sub          $(1), %ebp
    jnz          LL01gas_4
 
 
    pop          %r12
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownpi_And_16u_C1R

 
mfxownpi_And_16u_C1R:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    push         %r12
 
 
    push         %r13
 
 
    movslq       %esi, %rsi
    movslq       %ecx, %rcx
    movslq       %r9d, %r9
    movl         (48)(%rsp), %ebp
LL01gas_5:  
    mov          %rdi, %r10
    mov          %rdx, %r11
    mov          %r8, %r13
    movl         (40)(%rsp), %r12d
    test         $(1), %r13
    jz           LRunNgas_5
    sub          $(4), %r12
    jl           LL2gas_5
LL4ngas_5:  
    mov          (%r11), %rax
    and          (%r10), %rax
    mov          %rax, (%r13)
    add          $(8), %r11
    add          $(8), %r10
    add          $(8), %r13
    sub          $(4), %r12
    jge          LL4ngas_5
    jmp          LL2gas_5
LRunNgas_5:  
    mov          %r13, %rbx
    and          $(15), %rbx
    jz           LRungas_5
    sub          $(16), %rbx
    neg          %rbx
    shr          $(1), %rbx
    cmp          %rbx, %r12
    jl           LRungas_5
    sub          %rbx, %r12
LLN1gas_5:  
    movw         (%r11), %ax
    andw         (%r10), %ax
    movw         %ax, (%r13)
    add          $(2), %r11
    add          $(2), %r10
    add          $(2), %r13
    sub          $(1), %rbx
    jnz          LLN1gas_5
LRungas_5:  
    test         $(15), %r11
    jnz          LNA1gas_5
    test         $(15), %r10
    jnz          LNA2gas_5
    sub          $(64), %r12
    jl           LL32gas_5
LL64gas_5:  
    movdqa       (%r11), %xmm0
    movdqa       (16)(%r11), %xmm1
    movdqa       (32)(%r11), %xmm2
    movdqa       (48)(%r11), %xmm3
    movdqa       (64)(%r11), %xmm4
    movdqa       (80)(%r11), %xmm5
    movdqa       (96)(%r11), %xmm6
    movdqa       (112)(%r11), %xmm7
    pand         (%r10), %xmm0
    pand         (16)(%r10), %xmm1
    pand         (32)(%r10), %xmm2
    pand         (48)(%r10), %xmm3
    pand         (64)(%r10), %xmm4
    pand         (80)(%r10), %xmm5
    pand         (96)(%r10), %xmm6
    pand         (112)(%r10), %xmm7
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    movdqa       %xmm2, (32)(%r13)
    movdqa       %xmm3, (48)(%r13)
    movdqa       %xmm4, (64)(%r13)
    movdqa       %xmm5, (80)(%r13)
    movdqa       %xmm6, (96)(%r13)
    movdqa       %xmm7, (112)(%r13)
    add          $(128), %r11
    add          $(128), %r10
    add          $(128), %r13
    sub          $(64), %r12
    jge          LL64gas_5
LL32gas_5:  
    add          $(32), %r12
    jl           LL16gas_5
    movdqa       (%r11), %xmm0
    movdqa       (16)(%r11), %xmm1
    movdqa       (32)(%r11), %xmm2
    movdqa       (48)(%r11), %xmm3
    pand         (%r10), %xmm0
    pand         (16)(%r10), %xmm1
    pand         (32)(%r10), %xmm2
    pand         (48)(%r10), %xmm3
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    movdqa       %xmm2, (32)(%r13)
    movdqa       %xmm3, (48)(%r13)
    add          $(64), %r11
    add          $(64), %r10
    add          $(64), %r13
    sub          $(32), %r12
LL16gas_5:  
    add          $(16), %r12
    jl           LL8gas_5
    movdqa       (%r11), %xmm4
    movdqa       (16)(%r11), %xmm5
    pand         (%r10), %xmm4
    pand         (16)(%r10), %xmm5
    movdqa       %xmm4, (%r13)
    movdqa       %xmm5, (16)(%r13)
    add          $(32), %r11
    add          $(32), %r10
    add          $(32), %r13
    sub          $(16), %r12
LL8gas_5:  
    add          $(8), %r12
    jl           LL4gas_5
    movdqa       (%r11), %xmm6
    pand         (%r10), %xmm6
    movdqa       %xmm6, (%r13)
    add          $(16), %r11
    add          $(16), %r10
    add          $(16), %r13
    sub          $(8), %r12
    jmp          LL4gas_5
LNA1gas_5:  
    test         $(15), %r10
    jnz          LNA3gas_5
    sub          $(64), %r12
    jl           LL32n1gas_5
LL64n1gas_5:  
    lddqu        (%r11), %xmm0
    lddqu        (16)(%r11), %xmm1
    lddqu        (32)(%r11), %xmm2
    lddqu        (48)(%r11), %xmm3
    lddqu        (64)(%r11), %xmm4
    lddqu        (80)(%r11), %xmm5
    lddqu        (96)(%r11), %xmm6
    lddqu        (112)(%r11), %xmm7
    pand         (%r10), %xmm0
    pand         (16)(%r10), %xmm1
    pand         (32)(%r10), %xmm2
    pand         (48)(%r10), %xmm3
    pand         (64)(%r10), %xmm4
    pand         (80)(%r10), %xmm5
    pand         (96)(%r10), %xmm6
    pand         (112)(%r10), %xmm7
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    movdqa       %xmm2, (32)(%r13)
    movdqa       %xmm3, (48)(%r13)
    movdqa       %xmm4, (64)(%r13)
    movdqa       %xmm5, (80)(%r13)
    movdqa       %xmm6, (96)(%r13)
    movdqa       %xmm7, (112)(%r13)
    add          $(128), %r11
    add          $(128), %r10
    add          $(128), %r13
    sub          $(64), %r12
    jge          LL64n1gas_5
LL32n1gas_5:  
    add          $(32), %r12
    jl           LL16n1gas_5
    lddqu        (%r11), %xmm0
    lddqu        (16)(%r11), %xmm1
    lddqu        (32)(%r11), %xmm2
    lddqu        (48)(%r11), %xmm3
    pand         (%r10), %xmm0
    pand         (16)(%r10), %xmm1
    pand         (32)(%r10), %xmm2
    pand         (48)(%r10), %xmm3
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    movdqa       %xmm2, (32)(%r13)
    movdqa       %xmm3, (48)(%r13)
    add          $(64), %r11
    add          $(64), %r10
    add          $(64), %r13
    sub          $(32), %r12
LL16n1gas_5:  
    add          $(16), %r12
    jl           LL8n1gas_5
    lddqu        (%r11), %xmm4
    lddqu        (16)(%r11), %xmm5
    pand         (%r10), %xmm4
    pand         (16)(%r10), %xmm5
    movdqa       %xmm4, (%r13)
    movdqa       %xmm5, (16)(%r13)
    add          $(32), %r11
    add          $(32), %r10
    add          $(32), %r13
    sub          $(16), %r12
LL8n1gas_5:  
    add          $(8), %r12
    jl           LL4gas_5
    lddqu        (%r11), %xmm6
    pand         (%r10), %xmm6
    movdqa       %xmm6, (%r13)
    add          $(16), %r11
    add          $(16), %r10
    add          $(16), %r13
    sub          $(8), %r12
    jmp          LL4gas_5
LNA2gas_5:  
    sub          $(64), %r12
    jl           LL32n2gas_5
LL64n2gas_5:  
    lddqu        (%r10), %xmm0
    lddqu        (16)(%r10), %xmm1
    lddqu        (32)(%r10), %xmm2
    lddqu        (48)(%r10), %xmm3
    lddqu        (64)(%r10), %xmm4
    lddqu        (80)(%r10), %xmm5
    lddqu        (96)(%r10), %xmm6
    lddqu        (112)(%r10), %xmm7
    pand         (%r11), %xmm0
    pand         (16)(%r11), %xmm1
    pand         (32)(%r11), %xmm2
    pand         (48)(%r11), %xmm3
    pand         (64)(%r11), %xmm4
    pand         (80)(%r11), %xmm5
    pand         (96)(%r11), %xmm6
    pand         (112)(%r11), %xmm7
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    movdqa       %xmm2, (32)(%r13)
    movdqa       %xmm3, (48)(%r13)
    movdqa       %xmm4, (64)(%r13)
    movdqa       %xmm5, (80)(%r13)
    movdqa       %xmm6, (96)(%r13)
    movdqa       %xmm7, (112)(%r13)
    add          $(128), %r11
    add          $(128), %r10
    add          $(128), %r13
    sub          $(64), %r12
    jge          LL64n2gas_5
LL32n2gas_5:  
    add          $(32), %r12
    jl           LL16n2gas_5
    lddqu        (%r10), %xmm0
    lddqu        (16)(%r10), %xmm1
    lddqu        (32)(%r10), %xmm2
    lddqu        (48)(%r10), %xmm3
    pand         (%r11), %xmm0
    pand         (16)(%r11), %xmm1
    pand         (32)(%r11), %xmm2
    pand         (48)(%r11), %xmm3
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    movdqa       %xmm2, (32)(%r13)
    movdqa       %xmm3, (48)(%r13)
    add          $(64), %r11
    add          $(64), %r10
    add          $(64), %r13
    sub          $(32), %r12
LL16n2gas_5:  
    add          $(16), %r12
    jl           LL8n2gas_5
    lddqu        (%r10), %xmm4
    lddqu        (16)(%r10), %xmm5
    pand         (%r11), %xmm4
    pand         (16)(%r11), %xmm5
    movdqa       %xmm4, (%r13)
    movdqa       %xmm5, (16)(%r13)
    add          $(32), %r11
    add          $(32), %r10
    add          $(32), %r13
    sub          $(16), %r12
LL8n2gas_5:  
    add          $(8), %r12
    jl           LL4gas_5
    lddqu        (%r10), %xmm6
    pand         (%r11), %xmm6
    movdqa       %xmm6, (%r13)
    add          $(16), %r11
    add          $(16), %r10
    add          $(16), %r13
    sub          $(8), %r12
    jmp          LL4gas_5
LNA3gas_5:  
    sub          $(32), %r12
    jl           LL16n3gas_5
LL32n3gas_5:  
    lddqu        (%r11), %xmm0
    lddqu        (16)(%r11), %xmm1
    lddqu        (32)(%r11), %xmm2
    lddqu        (48)(%r11), %xmm3
    lddqu        (%r10), %xmm4
    lddqu        (16)(%r10), %xmm5
    lddqu        (32)(%r10), %xmm6
    lddqu        (48)(%r10), %xmm7
    pand         %xmm4, %xmm0
    pand         %xmm5, %xmm1
    pand         %xmm6, %xmm2
    pand         %xmm7, %xmm3
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    movdqa       %xmm2, (32)(%r13)
    movdqa       %xmm3, (48)(%r13)
    add          $(64), %r11
    add          $(64), %r10
    add          $(64), %r13
    sub          $(32), %r12
    jge          LL32n3gas_5
LL16n3gas_5:  
    add          $(16), %r12
    jl           LL8n3gas_5
    lddqu        (%r11), %xmm0
    lddqu        (16)(%r11), %xmm1
    lddqu        (%r10), %xmm2
    lddqu        (16)(%r10), %xmm3
    pand         %xmm2, %xmm0
    pand         %xmm3, %xmm1
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    add          $(32), %r11
    add          $(32), %r10
    add          $(32), %r13
    sub          $(16), %r12
LL8n3gas_5:  
    add          $(8), %r12
    jl           LL4gas_5
    lddqu        (%r11), %xmm4
    lddqu        (%r10), %xmm5
    pand         %xmm5, %xmm4
    movdqa       %xmm4, (%r13)
    add          $(16), %r11
    add          $(16), %r10
    add          $(16), %r13
    sub          $(8), %r12
LL4gas_5:  
    add          $(4), %r12
    jl           LL2gas_5
    mov          (%r11), %rax
    and          (%r10), %rax
    mov          %rax, (%r13)
    add          $(8), %r11
    add          $(8), %r10
    add          $(8), %r13
    sub          $(4), %r12
LL2gas_5:  
    add          $(2), %r12
    jl           LL1gas_5
    movl         (%r11), %eax
    andl         (%r10), %eax
    movl         %eax, (%r13)
    add          $(4), %r11
    add          $(4), %r10
    add          $(4), %r13
    sub          $(2), %r12
LL1gas_5:  
    add          $(1), %r12
    jl           LL02gas_5
    movw         (%r11), %ax
    andw         (%r10), %ax
    movw         %ax, (%r13)
LL02gas_5:  
    add          %rsi, %rdi
    add          %rcx, %rdx
    add          %r9, %r8
    sub          $(1), %ebp
    jnz          LL01gas_5
 
 
    pop          %r13
 
 
    pop          %r12
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownpi_And_16u_AC4R

 
mfxownpi_And_16u_AC4R:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    push         %r12
 
 
    push         %r13
 
 
    movslq       %esi, %rsi
    movslq       %ecx, %rcx
    movslq       %r9d, %r9
    movl         (48)(%rsp), %ebp
LL01gas_6:  
    mov          %rdi, %r10
    mov          %rdx, %r11
    mov          %r8, %r13
    movl         (40)(%rsp), %r12d
    test         $(7), %r13
    jz           LRunNgas_6
LNAgas_6:  
    movq         (%r11), %xmm0
    movq         (%r10), %xmm2
    movq         (%r13), %xmm1
    pand         %xmm2, %xmm0
    pand         dMask16(%rip), %xmm1
    pand         sMask16(%rip), %xmm0
    por          %xmm1, %xmm0
    movq         %xmm0, (%r13)
    add          $(8), %r10
    add          $(8), %r11
    add          $(8), %r13
    sub          $(1), %r12
    jnz          LNAgas_6
    jmp          LL02gas_6
LRunNgas_6:  
    mov          %r13, %rbx
    and          $(15), %rbx
    jz           LRungas_6
    movq         (%r11), %xmm0
    movq         (%r10), %xmm2
    movq         (%r13), %xmm1
    pand         %xmm2, %xmm0
    pand         dMask16(%rip), %xmm1
    pand         sMask16(%rip), %xmm0
    por          %xmm1, %xmm0
    movq         %xmm0, (%r13)
    add          $(8), %r10
    add          $(8), %r11
    add          $(8), %r13
    sub          $(1), %r12
LRungas_6:  
    test         $(15), %r11
    jnz          LNA1gas_6
    test         $(15), %r10
    jnz          LNA2gas_6
    sub          $(8), %r12
    jl           LL4gas_6
LL8gas_6:  
    movdqa       (%r11), %xmm0
    movdqa       (16)(%r11), %xmm1
    movdqa       (32)(%r11), %xmm4
    movdqa       (48)(%r11), %xmm5
    movdqa       (%r13), %xmm2
    movdqa       (16)(%r13), %xmm3
    movdqa       (32)(%r13), %xmm6
    movdqa       (48)(%r13), %xmm7
    pand         (%r10), %xmm0
    pand         (16)(%r10), %xmm1
    pand         (32)(%r10), %xmm4
    pand         (48)(%r10), %xmm5
    pand         sMask16(%rip), %xmm0
    pand         sMask16(%rip), %xmm1
    pand         sMask16(%rip), %xmm4
    pand         sMask16(%rip), %xmm5
    pand         dMask16(%rip), %xmm2
    pand         dMask16(%rip), %xmm3
    pand         dMask16(%rip), %xmm6
    pand         dMask16(%rip), %xmm7
    por          %xmm2, %xmm0
    por          %xmm3, %xmm1
    por          %xmm6, %xmm4
    por          %xmm7, %xmm5
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    movdqa       %xmm4, (32)(%r13)
    movdqa       %xmm5, (48)(%r13)
    add          $(64), %r10
    add          $(64), %r11
    add          $(64), %r13
    sub          $(8), %r12
    jge          LL8gas_6
LL4gas_6:  
    add          $(4), %r12
    jl           LL2gas_6
    movdqa       (%r11), %xmm0
    movdqa       (16)(%r11), %xmm1
    movdqa       (%r13), %xmm2
    movdqa       (16)(%r13), %xmm3
    pand         (%r10), %xmm0
    pand         (16)(%r10), %xmm1
    pand         sMask16(%rip), %xmm0
    pand         sMask16(%rip), %xmm1
    pand         dMask16(%rip), %xmm2
    pand         dMask16(%rip), %xmm3
    por          %xmm2, %xmm0
    por          %xmm3, %xmm1
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    add          $(32), %r10
    add          $(32), %r11
    add          $(32), %r13
    sub          $(4), %r12
LL2gas_6:  
    add          $(2), %r12
    jl           LL1gas_6
    movdqa       (%r11), %xmm4
    movdqa       (%r13), %xmm5
    pand         (%r10), %xmm4
    pand         sMask16(%rip), %xmm4
    pand         dMask16(%rip), %xmm5
    por          %xmm5, %xmm4
    movdqa       %xmm4, (%r13)
    add          $(16), %r10
    add          $(16), %r11
    add          $(16), %r13
    sub          $(2), %r12
LL1gas_6:  
    add          $(1), %r12
    jl           LL02gas_6
    movq         (%r11), %xmm6
    movq         (%r13), %xmm7
    pand         (%r10), %xmm6
    pand         dMask16(%rip), %xmm7
    pand         sMask16(%rip), %xmm6
    por          %xmm7, %xmm6
    movq         %xmm6, (%r13)
    jmp          LL02gas_6
LNA1gas_6:  
    test         $(15), %r10
    jnz          LNA3gas_6
    sub          $(8), %r12
    jl           LL4n1gas_6
LL8n1gas_6:  
    lddqu        (%r11), %xmm0
    lddqu        (16)(%r11), %xmm1
    lddqu        (32)(%r11), %xmm4
    lddqu        (48)(%r11), %xmm5
    movdqa       (%r13), %xmm2
    movdqa       (16)(%r13), %xmm3
    movdqa       (32)(%r13), %xmm6
    movdqa       (48)(%r13), %xmm7
    pand         (%r10), %xmm0
    pand         (16)(%r10), %xmm1
    pand         (32)(%r10), %xmm4
    pand         (48)(%r10), %xmm5
    pand         sMask16(%rip), %xmm0
    pand         sMask16(%rip), %xmm1
    pand         sMask16(%rip), %xmm4
    pand         sMask16(%rip), %xmm5
    pand         dMask16(%rip), %xmm2
    pand         dMask16(%rip), %xmm3
    pand         dMask16(%rip), %xmm6
    pand         dMask16(%rip), %xmm7
    por          %xmm2, %xmm0
    por          %xmm3, %xmm1
    por          %xmm6, %xmm4
    por          %xmm7, %xmm5
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    movdqa       %xmm4, (32)(%r13)
    movdqa       %xmm5, (48)(%r13)
    add          $(64), %r10
    add          $(64), %r11
    add          $(64), %r13
    sub          $(8), %r12
    jge          LL8n1gas_6
LL4n1gas_6:  
    add          $(4), %r12
    jl           LL2n1gas_6
    lddqu        (%r11), %xmm0
    lddqu        (16)(%r11), %xmm1
    movdqa       (%r13), %xmm2
    movdqa       (16)(%r13), %xmm3
    pand         (%r10), %xmm0
    pand         (16)(%r10), %xmm1
    pand         sMask16(%rip), %xmm0
    pand         sMask16(%rip), %xmm1
    pand         dMask16(%rip), %xmm2
    pand         dMask16(%rip), %xmm3
    por          %xmm2, %xmm0
    por          %xmm3, %xmm1
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    add          $(32), %r10
    add          $(32), %r11
    add          $(32), %r13
    sub          $(4), %r12
LL2n1gas_6:  
    add          $(2), %r12
    jl           LL1gas_6
    lddqu        (%r11), %xmm4
    movdqa       (%r13), %xmm5
    pand         (%r10), %xmm4
    pand         sMask16(%rip), %xmm4
    pand         dMask16(%rip), %xmm5
    por          %xmm5, %xmm4
    movdqa       %xmm4, (%r13)
    add          $(16), %r10
    add          $(16), %r11
    add          $(16), %r13
    sub          $(2), %r12
    jmp          LL1gas_6
LNA2gas_6:  
    sub          $(8), %r12
    jl           LL4n2gas_6
LL8n2gas_6:  
    lddqu        (%r10), %xmm0
    lddqu        (16)(%r10), %xmm1
    lddqu        (32)(%r10), %xmm4
    lddqu        (48)(%r10), %xmm5
    movdqa       (%r13), %xmm2
    movdqa       (16)(%r13), %xmm3
    movdqa       (32)(%r13), %xmm6
    movdqa       (48)(%r13), %xmm7
    pand         (%r11), %xmm0
    pand         (16)(%r11), %xmm1
    pand         (32)(%r11), %xmm4
    pand         (48)(%r11), %xmm5
    pand         sMask16(%rip), %xmm0
    pand         sMask16(%rip), %xmm1
    pand         sMask16(%rip), %xmm4
    pand         sMask16(%rip), %xmm5
    pand         dMask16(%rip), %xmm2
    pand         dMask16(%rip), %xmm3
    pand         dMask16(%rip), %xmm6
    pand         dMask16(%rip), %xmm7
    por          %xmm2, %xmm0
    por          %xmm3, %xmm1
    por          %xmm6, %xmm4
    por          %xmm7, %xmm5
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    movdqa       %xmm4, (32)(%r13)
    movdqa       %xmm5, (48)(%r13)
    add          $(64), %r10
    add          $(64), %r11
    add          $(64), %r13
    sub          $(8), %r12
    jge          LL8n2gas_6
LL4n2gas_6:  
    add          $(4), %r12
    jl           LL2n2gas_6
    lddqu        (%r10), %xmm0
    lddqu        (16)(%r10), %xmm1
    movdqa       (%r13), %xmm2
    movdqa       (16)(%r13), %xmm3
    pand         (%r11), %xmm0
    pand         (16)(%r11), %xmm1
    pand         sMask16(%rip), %xmm0
    pand         sMask16(%rip), %xmm1
    pand         dMask16(%rip), %xmm2
    pand         dMask16(%rip), %xmm3
    por          %xmm2, %xmm0
    por          %xmm3, %xmm1
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    add          $(32), %r10
    add          $(32), %r11
    add          $(32), %r13
    sub          $(4), %r12
LL2n2gas_6:  
    add          $(2), %r12
    jl           LL1n3gas_6
    lddqu        (%r10), %xmm4
    movdqa       (%r13), %xmm5
    pand         (%r11), %xmm4
    pand         sMask16(%rip), %xmm4
    pand         dMask16(%rip), %xmm5
    por          %xmm5, %xmm4
    movdqa       %xmm4, (%r13)
    add          $(16), %r10
    add          $(16), %r11
    add          $(16), %r13
    sub          $(2), %r12
    jmp          LL1n3gas_6
LNA3gas_6:  
    sub          $(4), %r12
    jl           LL2n3gas_6
LL4n3gas_6:  
    lddqu        (%r11), %xmm0
    lddqu        (16)(%r11), %xmm1
    lddqu        (%r10), %xmm2
    lddqu        (16)(%r10), %xmm3
    movdqa       (%r13), %xmm4
    movdqa       (16)(%r13), %xmm5
    pand         %xmm2, %xmm0
    pand         %xmm3, %xmm1
    pand         sMask16(%rip), %xmm0
    pand         sMask16(%rip), %xmm1
    pand         dMask16(%rip), %xmm4
    pand         dMask16(%rip), %xmm5
    por          %xmm4, %xmm0
    por          %xmm5, %xmm1
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    add          $(32), %r10
    add          $(32), %r11
    add          $(32), %r13
    sub          $(4), %r12
    jge          LL4n3gas_6
LL2n3gas_6:  
    add          $(2), %r12
    jl           LL1n3gas_6
    lddqu        (%r11), %xmm7
    lddqu        (%r10), %xmm6
    movdqa       (%r13), %xmm5
    pand         %xmm6, %xmm7
    pand         sMask16(%rip), %xmm7
    pand         dMask16(%rip), %xmm5
    por          %xmm5, %xmm7
    movdqa       %xmm7, (%r13)
    add          $(16), %r10
    add          $(16), %r11
    add          $(16), %r13
    sub          $(2), %r12
LL1n3gas_6:  
    add          $(1), %r12
    jl           LL02gas_6
    movq         (%r10), %xmm5
    movq         (%r11), %xmm6
    movq         (%r13), %xmm7
    pand         %xmm5, %xmm6
    pand         dMask16(%rip), %xmm7
    pand         sMask16(%rip), %xmm6
    por          %xmm7, %xmm6
    movq         %xmm6, (%r13)
LL02gas_6:  
    add          %rsi, %rdi
    add          %rcx, %rdx
    add          %r9, %r8
    sub          $(1), %ebp
    jnz          LL01gas_6
 
 
    pop          %r13
 
 
    pop          %r12
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownpi_OrC_16u_C1R

 
mfxownpi_OrC_16u_C1R:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    push         %r12
 
 
    movslq       %edx, %rdx
    movslq       %r8d, %r8
    mov          %rdi, %rax
    shl          $(16), %rdi
    and          $(65535), %rax
    or           %rdi, %rax
    mov          %eax, %eax
    movd         %eax, %xmm7
    mov          %rax, %rdi
    shl          $(32), %rdi
    or           %rax, %rdi
    pshufd       $(0), %xmm7, %xmm7
    movdqa       %xmm7, %xmm6
    mov          %r9d, %r9d
    movl         (32)(%rsp), %ebp
LL01gas_7:  
    mov          %rsi, %r10
    mov          %rcx, %r11
    mov          %r9, %r12
    test         $(1), %r11
    jz           LRunNgas_7
    sub          $(4), %r12
    jl           LL2gas_7
LL4ngas_7:  
    mov          (%r10), %rax
    or           %rdi, %rax
    mov          %rax, (%r11)
    add          $(8), %r10
    add          $(8), %r11
    sub          $(4), %r12
    jge          LL4ngas_7
    jmp          LL2gas_7
LRunNgas_7:  
    mov          %r11, %rbx
    and          $(15), %rbx
    jz           LRungas_7
    sub          $(16), %rbx
    neg          %rbx
    shr          $(1), %rbx
    cmp          %rbx, %r12
    jl           LRungas_7
    sub          %rbx, %r12
LLN1gas_7:  
    movw         (%r10), %ax
    or           %rdi, %rax
    movw         %ax, (%r11)
    add          $(2), %r10
    add          $(2), %r11
    sub          $(1), %rbx
    jnz          LLN1gas_7
LRungas_7:  
    test         $(15), %r10
    jnz          LNNAgas_7
    sub          $(32), %r12
    jl           LL16gas_7
LL32gas_7:  
    movdqa       (%r10), %xmm0
    movdqa       (16)(%r10), %xmm1
    movdqa       (32)(%r10), %xmm2
    movdqa       (48)(%r10), %xmm3
    por          %xmm6, %xmm0
    por          %xmm7, %xmm1
    por          %xmm6, %xmm2
    por          %xmm7, %xmm3
    movdqa       %xmm0, (%r11)
    movdqa       %xmm1, (16)(%r11)
    movdqa       %xmm2, (32)(%r11)
    movdqa       %xmm3, (48)(%r11)
    add          $(64), %r10
    add          $(64), %r11
    sub          $(32), %r12
    jge          LL32gas_7
LL16gas_7:  
    add          $(16), %r12
    jl           LL8gas_7
    movdqa       (%r10), %xmm4
    movdqa       (16)(%r10), %xmm5
    por          %xmm6, %xmm4
    por          %xmm7, %xmm5
    movdqa       %xmm4, (%r11)
    movdqa       %xmm5, (16)(%r11)
    add          $(32), %r10
    add          $(32), %r11
    sub          $(16), %r12
LL8gas_7:  
    add          $(8), %r12
    jl           LL4gas_7
    movdqa       (%r10), %xmm0
    por          %xmm6, %xmm0
    movdqa       %xmm0, (%r11)
    add          $(16), %r10
    add          $(16), %r11
    sub          $(8), %r12
    jmp          LL4gas_7
LNNAgas_7:  
    sub          $(32), %r12
    jl           LL16ngas_7
LL32ngas_7:  
    lddqu        (%r10), %xmm0
    lddqu        (16)(%r10), %xmm1
    lddqu        (32)(%r10), %xmm2
    lddqu        (48)(%r10), %xmm3
    por          %xmm6, %xmm0
    por          %xmm7, %xmm1
    por          %xmm6, %xmm2
    por          %xmm7, %xmm3
    movdqa       %xmm0, (%r11)
    movdqa       %xmm1, (16)(%r11)
    movdqa       %xmm2, (32)(%r11)
    movdqa       %xmm3, (48)(%r11)
    add          $(64), %r10
    add          $(64), %r11
    sub          $(32), %r12
    jge          LL32ngas_7
LL16ngas_7:  
    add          $(16), %r12
    jl           LL8ngas_7
    lddqu        (%r10), %xmm4
    lddqu        (16)(%r10), %xmm5
    por          %xmm6, %xmm4
    por          %xmm7, %xmm5
    movdqa       %xmm4, (%r11)
    movdqa       %xmm5, (16)(%r11)
    add          $(32), %r10
    add          $(32), %r11
    sub          $(16), %r12
LL8ngas_7:  
    add          $(8), %r12
    jl           LL4gas_7
    lddqu        (%r10), %xmm0
    por          %xmm6, %xmm0
    movdqa       %xmm0, (%r11)
    add          $(16), %r10
    add          $(16), %r11
    sub          $(8), %r12
LL4gas_7:  
    add          $(4), %r12
    jl           LL2gas_7
    mov          (%r10), %rax
    or           %rdi, %rax
    mov          %rax, (%r11)
    add          $(8), %r10
    add          $(8), %r11
    sub          $(4), %r12
LL2gas_7:  
    add          $(2), %r12
    jl           LL1gas_7
    movl         (%r10), %eax
    or           %rdi, %rax
    movl         %eax, (%r11)
    add          $(4), %r10
    add          $(4), %r11
    sub          $(2), %r12
LL1gas_7:  
    add          $(1), %r12
    jl           LL02gas_7
    movw         (%r10), %ax
    or           %rdi, %rax
    movw         %ax, (%r11)
LL02gas_7:  
    add          %rdx, %rsi
    add          %r8, %rcx
    sub          $(1), %ebp
    jnz          LL01gas_7
 
 
    pop          %r12
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownpi_OrC_16u_C3R

 
mfxownpi_OrC_16u_C3R:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    push         %r12
 
 
    movslq       %edx, %rdx
    movslq       %r8d, %r8
    movd         (%rdi), %xmm7
    pinsrw       $(2), (4)(%rdi), %xmm7
    movdqa       %xmm7, %xmm2
    movdqa       %xmm7, %xmm3
    movdqa       %xmm7, %xmm4
    movdqa       %xmm7, %xmm5
    movdqa       %xmm7, %xmm6
    psllq        $(16), %xmm2
    psllq        $(32), %xmm3
    psllq        $(48), %xmm4
    psrlq        $(16), %xmm5
    psrlq        $(32), %xmm6
    por          %xmm6, %xmm2
    por          %xmm7, %xmm4
    por          %xmm5, %xmm3
    unpcklpd     %xmm3, %xmm4
    unpcklpd     %xmm4, %xmm2
    unpcklpd     %xmm2, %xmm3
    mov          %r9d, %r9d
    movl         (32)(%rsp), %ebp
LL01gas_8:  
    mov          %rsi, %r10
    mov          %rcx, %r11
    mov          %r9, %r12
    cmp          $(32), %r12
    jl           LNSgas_8
    test         $(1), %r11
    jz           LRunNgas_8
LNSgas_8:  
    sub          $(4), %r12
    jl           LL3gas_8
LNAgas_8:  
    movq         (%r10), %xmm5
    movq         (8)(%r10), %xmm6
    movq         (16)(%r10), %xmm7
    por          %xmm4, %xmm5
    por          %xmm3, %xmm6
    por          %xmm2, %xmm7
    movq         %xmm5, (%r11)
    movq         %xmm6, (8)(%r11)
    movq         %xmm7, (16)(%r11)
    add          $(24), %r10
    add          $(24), %r11
    sub          $(4), %r12
    jge          LNAgas_8
    jmp          LL3gas_8
LRunNgas_8:  
    mov          %r11, %rbx
    and          $(15), %rbx
    jz           LRungas_8
    cmp          $(14), %rbx
    jl           Lt12gas_8
    mov          $(3), %rbx
    jmp          Lt0gas_8
Lt12gas_8:  
    cmp          $(12), %rbx
    jl           Lt10gas_8
    mov          $(6), %rbx
    jmp          Lt0gas_8
Lt10gas_8:  
    cmp          $(10), %rbx
    jl           Lt8gas_8
    mov          $(1), %rbx
    jmp          Lt0gas_8
Lt8gas_8:  
    cmp          $(8), %rbx
    jl           Lt6gas_8
    mov          $(4), %rbx
    jmp          Lt0gas_8
Lt6gas_8:  
    cmp          $(6), %rbx
    jl           Lt4gas_8
    mov          $(7), %rbx
    jmp          Lt0gas_8
Lt4gas_8:  
    cmp          $(4), %rbx
    jl           Lt2gas_8
    mov          $(2), %rbx
    jmp          Lt0gas_8
Lt2gas_8:  
    mov          $(5), %rbx
Lt0gas_8:  
    sub          %rbx, %r12
LL1ngas_8:  
    movd         (%r10), %xmm0
    pinsrw       $(2), (4)(%r10), %xmm0
    por          %xmm4, %xmm0
    movd         %xmm0, (%r11)
    psrlq        $(32), %xmm0
    movd         %xmm0, %eax
    movw         %ax, (4)(%r11)
    add          $(6), %r10
    add          $(6), %r11
    sub          $(1), %rbx
    jnz          LL1ngas_8
LRungas_8:  
    test         $(15), %r10
    jnz          LNNAgas_8
    sub          $(8), %r12
    jl           LL4gas_8
LL8gas_8:  
    movdqa       (%r10), %xmm5
    movdqa       (16)(%r10), %xmm6
    movdqa       (32)(%r10), %xmm7
    por          %xmm4, %xmm5
    por          %xmm2, %xmm6
    por          %xmm3, %xmm7
    movdqa       %xmm5, (%r11)
    movdqa       %xmm6, (16)(%r11)
    movdqa       %xmm7, (32)(%r11)
    add          $(48), %r10
    add          $(48), %r11
    sub          $(8), %r12
    jge          LL8gas_8
LL4gas_8:  
    add          $(4), %r12
    jl           LL3gas_8
    movdqa       (%r10), %xmm0
    movq         (16)(%r10), %xmm1
    por          %xmm4, %xmm0
    por          %xmm2, %xmm1
    movdqa       %xmm0, (%r11)
    movq         %xmm1, (16)(%r11)
    add          $(24), %r10
    add          $(24), %r11
    sub          $(4), %r12
    jmp          LL3gas_8
LNNAgas_8:  
    sub          $(8), %r12
    jl           LL4ngas_8
LL8ngas_8:  
    lddqu        (%r10), %xmm5
    lddqu        (16)(%r10), %xmm6
    lddqu        (32)(%r10), %xmm7
    por          %xmm4, %xmm5
    por          %xmm2, %xmm6
    por          %xmm3, %xmm7
    movdqa       %xmm5, (%r11)
    movdqa       %xmm6, (16)(%r11)
    movdqa       %xmm7, (32)(%r11)
    add          $(48), %r10
    add          $(48), %r11
    sub          $(8), %r12
    jge          LL8ngas_8
LL4ngas_8:  
    add          $(4), %r12
    jl           LL3gas_8
    lddqu        (%r10), %xmm0
    movq         (16)(%r10), %xmm1
    por          %xmm4, %xmm0
    por          %xmm2, %xmm1
    movdqa       %xmm0, (%r11)
    movq         %xmm1, (16)(%r11)
    add          $(24), %r10
    add          $(24), %r11
    sub          $(4), %r12
LL3gas_8:  
    add          $(4), %r12
    jz           LL02gas_8
LL1gas_8:  
    movd         (%r10), %xmm0
    pinsrw       $(2), (4)(%r10), %xmm0
    por          %xmm4, %xmm0
    movd         %xmm0, (%r11)
    psrlq        $(32), %xmm0
    movd         %xmm0, %eax
    movw         %ax, (4)(%r11)
    add          $(6), %r10
    add          $(6), %r11
    sub          $(1), %r12
    jnz          LL1gas_8
LL02gas_8:  
    add          %rdx, %rsi
    add          %r8, %rcx
    sub          $(1), %ebp
    jnz          LL01gas_8
 
 
    pop          %r12
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownpi_OrC_16u_C4R

 
mfxownpi_OrC_16u_C4R:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    push         %r12
 
 
    movslq       %edx, %rdx
    movslq       %r8d, %r8
    movq         (%rdi), %xmm7
    pshufd       $(68), %xmm7, %xmm7
    mov          %r9d, %r9d
    movl         (32)(%rsp), %ebp
LL01gas_9:  
    mov          %rsi, %r10
    mov          %rcx, %r11
    mov          %r9, %r12
    test         $(7), %r11
    jz           LRunNgas_9
LNAgas_9:  
    movq         (%r10), %xmm0
    por          %xmm7, %xmm0
    movq         %xmm0, (%r11)
    add          $(8), %r10
    add          $(8), %r11
    sub          $(1), %r12
    jnz          LNAgas_9
    jmp          LL02gas_9
LRunNgas_9:  
    mov          %r11, %rbx
    and          $(15), %rbx
    jz           LRungas_9
    movq         (%r10), %xmm0
    por          %xmm7, %xmm0
    movq         %xmm0, (%r11)
    add          $(8), %r10
    add          $(8), %r11
    sub          $(1), %r12
LRungas_9:  
    test         $(15), %r10
    jnz          LNNAgas_9
    sub          $(8), %r12
    jl           LL4gas_9
LL8gas_9:  
    movdqa       (%r10), %xmm0
    movdqa       (16)(%r10), %xmm1
    movdqa       (32)(%r10), %xmm2
    movdqa       (48)(%r10), %xmm3
    por          %xmm7, %xmm0
    por          %xmm7, %xmm1
    por          %xmm7, %xmm2
    por          %xmm7, %xmm3
    movdqa       %xmm0, (%r11)
    movdqa       %xmm1, (16)(%r11)
    movdqa       %xmm2, (32)(%r11)
    movdqa       %xmm3, (48)(%r11)
    add          $(64), %r10
    add          $(64), %r11
    sub          $(8), %r12
    jge          LL8gas_9
LL4gas_9:  
    add          $(4), %r12
    jl           LL2gas_9
    movdqa       (%r10), %xmm4
    movdqa       (16)(%r10), %xmm5
    por          %xmm7, %xmm4
    por          %xmm7, %xmm5
    movdqa       %xmm4, (%r11)
    movdqa       %xmm5, (16)(%r11)
    add          $(32), %r10
    add          $(32), %r11
    sub          $(4), %r12
LL2gas_9:  
    add          $(2), %r12
    jl           LL1gas_9
    movdqa       (%r10), %xmm6
    por          %xmm7, %xmm6
    movdqa       %xmm6, (%r11)
    add          $(16), %r10
    add          $(16), %r11
    sub          $(2), %r12
    jmp          LL1gas_9
LNNAgas_9:  
    sub          $(8), %r12
    jl           LL4ngas_9
LL8ngas_9:  
    lddqu        (%r10), %xmm0
    lddqu        (16)(%r10), %xmm1
    lddqu        (32)(%r10), %xmm2
    lddqu        (48)(%r10), %xmm3
    por          %xmm7, %xmm0
    por          %xmm7, %xmm1
    por          %xmm7, %xmm2
    por          %xmm7, %xmm3
    movdqa       %xmm0, (%r11)
    movdqa       %xmm1, (16)(%r11)
    movdqa       %xmm2, (32)(%r11)
    movdqa       %xmm3, (48)(%r11)
    add          $(64), %r10
    add          $(64), %r11
    sub          $(8), %r12
    jge          LL8ngas_9
LL4ngas_9:  
    add          $(4), %r12
    jl           LL2ngas_9
    lddqu        (%r10), %xmm4
    lddqu        (16)(%r10), %xmm5
    por          %xmm7, %xmm4
    por          %xmm7, %xmm5
    movdqa       %xmm4, (%r11)
    movdqa       %xmm5, (16)(%r11)
    add          $(32), %r10
    add          $(32), %r11
    sub          $(4), %r12
LL2ngas_9:  
    add          $(2), %r12
    jl           LL1gas_9
    lddqu        (%r10), %xmm6
    por          %xmm7, %xmm6
    movdqa       %xmm6, (%r11)
    add          $(16), %r10
    add          $(16), %r11
    sub          $(2), %r12
LL1gas_9:  
    add          $(1), %r12
    jl           LL02gas_9
    movq         (%r10), %xmm0
    por          %xmm7, %xmm0
    movq         %xmm0, (%r11)
LL02gas_9:  
    add          %rdx, %rsi
    add          %r8, %rcx
    sub          $(1), %ebp
    jnz          LL01gas_9
 
 
    pop          %r12
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownpi_OrC_16u_AC4R

 
mfxownpi_OrC_16u_AC4R:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    push         %r12
 
 
    movslq       %edx, %rdx
    movslq       %r8d, %r8
    movd         (%rdi), %xmm7
    pinsrw       $(2), (4)(%rdi), %xmm7
    pshufd       $(68), %xmm7, %xmm7
    mov          %r9d, %r9d
    movl         (32)(%rsp), %ebp
LL01gas_10:  
    mov          %rsi, %r10
    mov          %rcx, %r11
    mov          %r9, %r12
    test         $(7), %r11
    jz           LRunNgas_10
LNAgas_10:  
    movq         (%r10), %xmm0
    movq         (%r11), %xmm1
    por          %xmm7, %xmm0
    pand         dMask16(%rip), %xmm1
    pand         sMask16(%rip), %xmm0
    por          %xmm1, %xmm0
    movq         %xmm0, (%r11)
    add          $(8), %r10
    add          $(8), %r11
    sub          $(1), %r12
    jnz          LNAgas_10
    jmp          LL02gas_10
LRunNgas_10:  
    mov          %r11, %rbx
    and          $(15), %rbx
    jz           LRungas_10
    movq         (%r10), %xmm0
    movq         (%r11), %xmm1
    por          %xmm7, %xmm0
    pand         dMask16(%rip), %xmm1
    pand         sMask16(%rip), %xmm0
    por          %xmm1, %xmm0
    movq         %xmm0, (%r11)
    add          $(8), %r10
    add          $(8), %r11
    sub          $(1), %r12
LRungas_10:  
    test         $(15), %r10
    jnz          LNNAgas_10
    sub          $(4), %r12
    jl           LL2gas_10
LL4gas_10:  
    movdqa       (%r10), %xmm0
    movdqa       (16)(%r10), %xmm1
    movdqa       (%r11), %xmm2
    movdqa       (16)(%r11), %xmm3
    por          %xmm7, %xmm0
    por          %xmm7, %xmm1
    pand         dMask16(%rip), %xmm2
    pand         dMask16(%rip), %xmm3
    pand         sMask16(%rip), %xmm0
    pand         sMask16(%rip), %xmm1
    por          %xmm2, %xmm0
    por          %xmm3, %xmm1
    movdqa       %xmm0, (%r11)
    movdqa       %xmm1, (16)(%r11)
    add          $(32), %r10
    add          $(32), %r11
    sub          $(4), %r12
    jge          LL4gas_10
LL2gas_10:  
    add          $(2), %r12
    jl           LL1gas_10
    movdqa       (%r10), %xmm4
    movdqa       (%r11), %xmm5
    por          %xmm7, %xmm4
    pand         dMask16(%rip), %xmm5
    pand         sMask16(%rip), %xmm4
    por          %xmm5, %xmm4
    movdqa       %xmm4, (%r11)
    add          $(16), %r10
    add          $(16), %r11
    sub          $(2), %r12
    jmp          LL1gas_10
LNNAgas_10:  
    sub          $(4), %r12
    jl           LL2ngas_10
LL4ngas_10:  
    lddqu        (%r10), %xmm0
    lddqu        (16)(%r10), %xmm1
    movdqa       (%r11), %xmm2
    movdqa       (16)(%r11), %xmm3
    por          %xmm7, %xmm0
    por          %xmm7, %xmm1
    pand         dMask16(%rip), %xmm2
    pand         dMask16(%rip), %xmm3
    pand         sMask16(%rip), %xmm0
    pand         sMask16(%rip), %xmm1
    por          %xmm2, %xmm0
    por          %xmm3, %xmm1
    movdqa       %xmm0, (%r11)
    movdqa       %xmm1, (16)(%r11)
    add          $(32), %r10
    add          $(32), %r11
    sub          $(4), %r12
    jge          LL4ngas_10
LL2ngas_10:  
    add          $(2), %r12
    jl           LL1gas_10
    lddqu        (%r10), %xmm4
    movdqa       (%r11), %xmm5
    por          %xmm7, %xmm4
    pand         dMask16(%rip), %xmm5
    pand         sMask16(%rip), %xmm4
    por          %xmm5, %xmm4
    movdqa       %xmm4, (%r11)
    add          $(16), %r10
    add          $(16), %r11
    sub          $(2), %r12
LL1gas_10:  
    add          $(1), %r12
    jl           LL02gas_10
    movq         (%r10), %xmm6
    movq         (%r11), %xmm1
    por          %xmm7, %xmm6
    pand         dMask16(%rip), %xmm1
    pand         sMask16(%rip), %xmm6
    por          %xmm1, %xmm6
    movq         %xmm6, (%r11)
LL02gas_10:  
    add          %rdx, %rsi
    add          %r8, %rcx
    sub          $(1), %ebp
    jnz          LL01gas_10
 
 
    pop          %r12
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownpi_Or_16u_C1R

 
mfxownpi_Or_16u_C1R:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    push         %r12
 
 
    push         %r13
 
 
    movslq       %esi, %rsi
    movslq       %ecx, %rcx
    movslq       %r9d, %r9
    movl         (48)(%rsp), %ebp
LL01gas_11:  
    mov          %rdi, %r10
    mov          %rdx, %r11
    mov          %r8, %r13
    movl         (40)(%rsp), %r12d
    test         $(1), %r13
    jz           LRunNgas_11
    sub          $(4), %r12
    jl           LL2gas_11
LL4ngas_11:  
    mov          (%r11), %rax
    or           (%r10), %rax
    mov          %rax, (%r13)
    add          $(8), %r11
    add          $(8), %r10
    add          $(8), %r13
    sub          $(4), %r12
    jge          LL4ngas_11
    jmp          LL2gas_11
LRunNgas_11:  
    mov          %r13, %rbx
    and          $(15), %rbx
    jz           LRungas_11
    sub          $(16), %rbx
    neg          %rbx
    shr          $(1), %rbx
    cmp          %rbx, %r12
    jl           LRungas_11
    sub          %rbx, %r12
LLN1gas_11:  
    movw         (%r11), %ax
    orw          (%r10), %ax
    movw         %ax, (%r13)
    add          $(2), %r11
    add          $(2), %r10
    add          $(2), %r13
    sub          $(1), %rbx
    jnz          LLN1gas_11
LRungas_11:  
    test         $(15), %r11
    jnz          LNA1gas_11
    test         $(15), %r10
    jnz          LNA2gas_11
    sub          $(64), %r12
    jl           LL32gas_11
LL64gas_11:  
    movdqa       (%r11), %xmm0
    movdqa       (16)(%r11), %xmm1
    movdqa       (32)(%r11), %xmm2
    movdqa       (48)(%r11), %xmm3
    movdqa       (64)(%r11), %xmm4
    movdqa       (80)(%r11), %xmm5
    movdqa       (96)(%r11), %xmm6
    movdqa       (112)(%r11), %xmm7
    por          (%r10), %xmm0
    por          (16)(%r10), %xmm1
    por          (32)(%r10), %xmm2
    por          (48)(%r10), %xmm3
    por          (64)(%r10), %xmm4
    por          (80)(%r10), %xmm5
    por          (96)(%r10), %xmm6
    por          (112)(%r10), %xmm7
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    movdqa       %xmm2, (32)(%r13)
    movdqa       %xmm3, (48)(%r13)
    movdqa       %xmm4, (64)(%r13)
    movdqa       %xmm5, (80)(%r13)
    movdqa       %xmm6, (96)(%r13)
    movdqa       %xmm7, (112)(%r13)
    add          $(128), %r11
    add          $(128), %r10
    add          $(128), %r13
    sub          $(64), %r12
    jge          LL64gas_11
LL32gas_11:  
    add          $(32), %r12
    jl           LL16gas_11
    movdqa       (%r11), %xmm0
    movdqa       (16)(%r11), %xmm1
    movdqa       (32)(%r11), %xmm2
    movdqa       (48)(%r11), %xmm3
    por          (%r10), %xmm0
    por          (16)(%r10), %xmm1
    por          (32)(%r10), %xmm2
    por          (48)(%r10), %xmm3
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    movdqa       %xmm2, (32)(%r13)
    movdqa       %xmm3, (48)(%r13)
    add          $(64), %r11
    add          $(64), %r10
    add          $(64), %r13
    sub          $(32), %r12
LL16gas_11:  
    add          $(16), %r12
    jl           LL8gas_11
    movdqa       (%r11), %xmm4
    movdqa       (16)(%r11), %xmm5
    por          (%r10), %xmm4
    por          (16)(%r10), %xmm5
    movdqa       %xmm4, (%r13)
    movdqa       %xmm5, (16)(%r13)
    add          $(32), %r11
    add          $(32), %r10
    add          $(32), %r13
    sub          $(16), %r12
LL8gas_11:  
    add          $(8), %r12
    jl           LL4gas_11
    movdqa       (%r11), %xmm6
    por          (%r10), %xmm6
    movdqa       %xmm6, (%r13)
    add          $(16), %r11
    add          $(16), %r10
    add          $(16), %r13
    sub          $(8), %r12
    jmp          LL4gas_11
LNA1gas_11:  
    test         $(15), %r10
    jnz          LNA3gas_11
    sub          $(64), %r12
    jl           LL32n1gas_11
LL64n1gas_11:  
    lddqu        (%r11), %xmm0
    lddqu        (16)(%r11), %xmm1
    lddqu        (32)(%r11), %xmm2
    lddqu        (48)(%r11), %xmm3
    lddqu        (64)(%r11), %xmm4
    lddqu        (80)(%r11), %xmm5
    lddqu        (96)(%r11), %xmm6
    lddqu        (112)(%r11), %xmm7
    por          (%r10), %xmm0
    por          (16)(%r10), %xmm1
    por          (32)(%r10), %xmm2
    por          (48)(%r10), %xmm3
    por          (64)(%r10), %xmm4
    por          (80)(%r10), %xmm5
    por          (96)(%r10), %xmm6
    por          (112)(%r10), %xmm7
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    movdqa       %xmm2, (32)(%r13)
    movdqa       %xmm3, (48)(%r13)
    movdqa       %xmm4, (64)(%r13)
    movdqa       %xmm5, (80)(%r13)
    movdqa       %xmm6, (96)(%r13)
    movdqa       %xmm7, (112)(%r13)
    add          $(128), %r11
    add          $(128), %r10
    add          $(128), %r13
    sub          $(64), %r12
    jge          LL64n1gas_11
LL32n1gas_11:  
    add          $(32), %r12
    jl           LL16n1gas_11
    lddqu        (%r11), %xmm0
    lddqu        (16)(%r11), %xmm1
    lddqu        (32)(%r11), %xmm2
    lddqu        (48)(%r11), %xmm3
    por          (%r10), %xmm0
    por          (16)(%r10), %xmm1
    por          (32)(%r10), %xmm2
    por          (48)(%r10), %xmm3
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    movdqa       %xmm2, (32)(%r13)
    movdqa       %xmm3, (48)(%r13)
    add          $(64), %r11
    add          $(64), %r10
    add          $(64), %r13
    sub          $(32), %r12
LL16n1gas_11:  
    add          $(16), %r12
    jl           LL8n1gas_11
    lddqu        (%r11), %xmm4
    lddqu        (16)(%r11), %xmm5
    por          (%r10), %xmm4
    por          (16)(%r10), %xmm5
    movdqa       %xmm4, (%r13)
    movdqa       %xmm5, (16)(%r13)
    add          $(32), %r11
    add          $(32), %r10
    add          $(32), %r13
    sub          $(16), %r12
LL8n1gas_11:  
    add          $(8), %r12
    jl           LL4gas_11
    lddqu        (%r11), %xmm6
    por          (%r10), %xmm6
    movdqa       %xmm6, (%r13)
    add          $(16), %r11
    add          $(16), %r10
    add          $(16), %r13
    sub          $(8), %r12
    jmp          LL4gas_11
LNA2gas_11:  
    sub          $(64), %r12
    jl           LL32n2gas_11
LL64n2gas_11:  
    lddqu        (%r10), %xmm0
    lddqu        (16)(%r10), %xmm1
    lddqu        (32)(%r10), %xmm2
    lddqu        (48)(%r10), %xmm3
    lddqu        (64)(%r10), %xmm4
    lddqu        (80)(%r10), %xmm5
    lddqu        (96)(%r10), %xmm6
    lddqu        (112)(%r10), %xmm7
    por          (%r11), %xmm0
    por          (16)(%r11), %xmm1
    por          (32)(%r11), %xmm2
    por          (48)(%r11), %xmm3
    por          (64)(%r11), %xmm4
    por          (80)(%r11), %xmm5
    por          (96)(%r11), %xmm6
    por          (112)(%r11), %xmm7
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    movdqa       %xmm2, (32)(%r13)
    movdqa       %xmm3, (48)(%r13)
    movdqa       %xmm4, (64)(%r13)
    movdqa       %xmm5, (80)(%r13)
    movdqa       %xmm6, (96)(%r13)
    movdqa       %xmm7, (112)(%r13)
    add          $(128), %r11
    add          $(128), %r10
    add          $(128), %r13
    sub          $(64), %r12
    jge          LL64n2gas_11
LL32n2gas_11:  
    add          $(32), %r12
    jl           LL16n2gas_11
    lddqu        (%r10), %xmm0
    lddqu        (16)(%r10), %xmm1
    lddqu        (32)(%r10), %xmm2
    lddqu        (48)(%r10), %xmm3
    por          (%r11), %xmm0
    por          (16)(%r11), %xmm1
    por          (32)(%r11), %xmm2
    por          (48)(%r11), %xmm3
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    movdqa       %xmm2, (32)(%r13)
    movdqa       %xmm3, (48)(%r13)
    add          $(64), %r11
    add          $(64), %r10
    add          $(64), %r13
    sub          $(32), %r12
LL16n2gas_11:  
    add          $(16), %r12
    jl           LL8n2gas_11
    lddqu        (%r10), %xmm4
    lddqu        (16)(%r10), %xmm5
    por          (%r11), %xmm4
    por          (16)(%r11), %xmm5
    movdqa       %xmm4, (%r13)
    movdqa       %xmm5, (16)(%r13)
    add          $(32), %r11
    add          $(32), %r10
    add          $(32), %r13
    sub          $(16), %r12
LL8n2gas_11:  
    add          $(8), %r12
    jl           LL4gas_11
    lddqu        (%r10), %xmm6
    por          (%r11), %xmm6
    movdqa       %xmm6, (%r13)
    add          $(16), %r11
    add          $(16), %r10
    add          $(16), %r13
    sub          $(8), %r12
    jmp          LL4gas_11
LNA3gas_11:  
    sub          $(32), %r12
    jl           LL16n3gas_11
LL32n3gas_11:  
    lddqu        (%r11), %xmm0
    lddqu        (16)(%r11), %xmm1
    lddqu        (32)(%r11), %xmm2
    lddqu        (48)(%r11), %xmm3
    lddqu        (%r10), %xmm4
    lddqu        (16)(%r10), %xmm5
    lddqu        (32)(%r10), %xmm6
    lddqu        (48)(%r10), %xmm7
    por          %xmm4, %xmm0
    por          %xmm5, %xmm1
    por          %xmm6, %xmm2
    por          %xmm7, %xmm3
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    movdqa       %xmm2, (32)(%r13)
    movdqa       %xmm3, (48)(%r13)
    add          $(64), %r11
    add          $(64), %r10
    add          $(64), %r13
    sub          $(32), %r12
    jge          LL32n3gas_11
LL16n3gas_11:  
    add          $(16), %r12
    jl           LL8n3gas_11
    lddqu        (%r11), %xmm0
    lddqu        (16)(%r11), %xmm1
    lddqu        (%r10), %xmm2
    lddqu        (16)(%r10), %xmm3
    por          %xmm2, %xmm0
    por          %xmm3, %xmm1
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    add          $(32), %r11
    add          $(32), %r10
    add          $(32), %r13
    sub          $(16), %r12
LL8n3gas_11:  
    add          $(8), %r12
    jl           LL4gas_11
    lddqu        (%r11), %xmm4
    lddqu        (%r10), %xmm5
    por          %xmm5, %xmm4
    movdqa       %xmm4, (%r13)
    add          $(16), %r11
    add          $(16), %r10
    add          $(16), %r13
    sub          $(8), %r12
LL4gas_11:  
    add          $(4), %r12
    jl           LL2gas_11
    mov          (%r11), %rax
    or           (%r10), %rax
    mov          %rax, (%r13)
    add          $(8), %r11
    add          $(8), %r10
    add          $(8), %r13
    sub          $(4), %r12
LL2gas_11:  
    add          $(2), %r12
    jl           LL1gas_11
    movl         (%r11), %eax
    orl          (%r10), %eax
    movl         %eax, (%r13)
    add          $(4), %r11
    add          $(4), %r10
    add          $(4), %r13
    sub          $(2), %r12
LL1gas_11:  
    add          $(1), %r12
    jl           LL02gas_11
    movw         (%r11), %ax
    orw          (%r10), %ax
    movw         %ax, (%r13)
LL02gas_11:  
    add          %rsi, %rdi
    add          %rcx, %rdx
    add          %r9, %r8
    sub          $(1), %ebp
    jnz          LL01gas_11
 
 
    pop          %r13
 
 
    pop          %r12
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownpi_Or_16u_AC4R

 
mfxownpi_Or_16u_AC4R:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    push         %r12
 
 
    push         %r13
 
 
    movslq       %esi, %rsi
    movslq       %ecx, %rcx
    movslq       %r9d, %r9
    movl         (48)(%rsp), %ebp
LL01gas_12:  
    mov          %rdi, %r10
    mov          %rdx, %r11
    mov          %r8, %r13
    movl         (40)(%rsp), %r12d
    test         $(7), %r13
    jz           LRunNgas_12
LNAgas_12:  
    movq         (%r11), %xmm0
    movq         (%r10), %xmm2
    movq         (%r13), %xmm1
    por          %xmm2, %xmm0
    pand         dMask16(%rip), %xmm1
    pand         sMask16(%rip), %xmm0
    por          %xmm1, %xmm0
    movq         %xmm0, (%r13)
    add          $(8), %r10
    add          $(8), %r11
    add          $(8), %r13
    sub          $(1), %r12
    jnz          LNAgas_12
    jmp          LL02gas_12
LRunNgas_12:  
    mov          %r13, %rbx
    and          $(15), %rbx
    jz           LRungas_12
    movq         (%r11), %xmm0
    movq         (%r10), %xmm2
    movq         (%r13), %xmm1
    por          %xmm2, %xmm0
    pand         dMask16(%rip), %xmm1
    pand         sMask16(%rip), %xmm0
    por          %xmm1, %xmm0
    movq         %xmm0, (%r13)
    add          $(8), %r10
    add          $(8), %r11
    add          $(8), %r13
    sub          $(1), %r12
LRungas_12:  
    test         $(15), %r11
    jnz          LNA1gas_12
    test         $(15), %r10
    jnz          LNA2gas_12
    sub          $(8), %r12
    jl           LL4gas_12
LL8gas_12:  
    movdqa       (%r11), %xmm0
    movdqa       (16)(%r11), %xmm1
    movdqa       (32)(%r11), %xmm4
    movdqa       (48)(%r11), %xmm5
    movdqa       (%r13), %xmm2
    movdqa       (16)(%r13), %xmm3
    movdqa       (32)(%r13), %xmm6
    movdqa       (48)(%r13), %xmm7
    por          (%r10), %xmm0
    por          (16)(%r10), %xmm1
    por          (32)(%r10), %xmm4
    por          (48)(%r10), %xmm5
    pand         sMask16(%rip), %xmm0
    pand         sMask16(%rip), %xmm1
    pand         sMask16(%rip), %xmm4
    pand         sMask16(%rip), %xmm5
    pand         dMask16(%rip), %xmm2
    pand         dMask16(%rip), %xmm3
    pand         dMask16(%rip), %xmm6
    pand         dMask16(%rip), %xmm7
    por          %xmm2, %xmm0
    por          %xmm3, %xmm1
    por          %xmm6, %xmm4
    por          %xmm7, %xmm5
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    movdqa       %xmm4, (32)(%r13)
    movdqa       %xmm5, (48)(%r13)
    add          $(64), %r10
    add          $(64), %r11
    add          $(64), %r13
    sub          $(8), %r12
    jge          LL8gas_12
LL4gas_12:  
    add          $(4), %r12
    jl           LL2gas_12
    movdqa       (%r11), %xmm0
    movdqa       (16)(%r11), %xmm1
    movdqa       (%r13), %xmm2
    movdqa       (16)(%r13), %xmm3
    por          (%r10), %xmm0
    por          (16)(%r10), %xmm1
    pand         sMask16(%rip), %xmm0
    pand         sMask16(%rip), %xmm1
    pand         dMask16(%rip), %xmm2
    pand         dMask16(%rip), %xmm3
    por          %xmm2, %xmm0
    por          %xmm3, %xmm1
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    add          $(32), %r10
    add          $(32), %r11
    add          $(32), %r13
    sub          $(4), %r12
LL2gas_12:  
    add          $(2), %r12
    jl           LL1gas_12
    movdqa       (%r11), %xmm4
    movdqa       (%r13), %xmm5
    por          (%r10), %xmm4
    pand         sMask16(%rip), %xmm4
    pand         dMask16(%rip), %xmm5
    por          %xmm5, %xmm4
    movdqa       %xmm4, (%r13)
    add          $(16), %r10
    add          $(16), %r11
    add          $(16), %r13
    sub          $(2), %r12
LL1gas_12:  
    add          $(1), %r12
    jl           LL02gas_12
    movq         (%r11), %xmm6
    movq         (%r13), %xmm7
    por          (%r10), %xmm6
    pand         dMask16(%rip), %xmm7
    pand         sMask16(%rip), %xmm6
    por          %xmm7, %xmm6
    movq         %xmm6, (%r13)
    jmp          LL02gas_12
LNA1gas_12:  
    test         $(15), %r10
    jnz          LNA3gas_12
    sub          $(8), %r12
    jl           LL4n1gas_12
LL8n1gas_12:  
    lddqu        (%r11), %xmm0
    lddqu        (16)(%r11), %xmm1
    lddqu        (32)(%r11), %xmm4
    lddqu        (48)(%r11), %xmm5
    movdqa       (%r13), %xmm2
    movdqa       (16)(%r13), %xmm3
    movdqa       (32)(%r13), %xmm6
    movdqa       (48)(%r13), %xmm7
    por          (%r10), %xmm0
    por          (16)(%r10), %xmm1
    por          (32)(%r10), %xmm4
    por          (48)(%r10), %xmm5
    pand         sMask16(%rip), %xmm0
    pand         sMask16(%rip), %xmm1
    pand         sMask16(%rip), %xmm4
    pand         sMask16(%rip), %xmm5
    pand         dMask16(%rip), %xmm2
    pand         dMask16(%rip), %xmm3
    pand         dMask16(%rip), %xmm6
    pand         dMask16(%rip), %xmm7
    por          %xmm2, %xmm0
    por          %xmm3, %xmm1
    por          %xmm6, %xmm4
    por          %xmm7, %xmm5
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    movdqa       %xmm4, (32)(%r13)
    movdqa       %xmm5, (48)(%r13)
    add          $(64), %r10
    add          $(64), %r11
    add          $(64), %r13
    sub          $(8), %r12
    jge          LL8n1gas_12
LL4n1gas_12:  
    add          $(4), %r12
    jl           LL2n1gas_12
    lddqu        (%r11), %xmm0
    lddqu        (16)(%r11), %xmm1
    movdqa       (%r13), %xmm2
    movdqa       (16)(%r13), %xmm3
    por          (%r10), %xmm0
    por          (16)(%r10), %xmm1
    pand         sMask16(%rip), %xmm0
    pand         sMask16(%rip), %xmm1
    pand         dMask16(%rip), %xmm2
    pand         dMask16(%rip), %xmm3
    por          %xmm2, %xmm0
    por          %xmm3, %xmm1
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    add          $(32), %r10
    add          $(32), %r11
    add          $(32), %r13
    sub          $(4), %r12
LL2n1gas_12:  
    add          $(2), %r12
    jl           LL1gas_12
    lddqu        (%r11), %xmm4
    movdqa       (%r13), %xmm5
    por          (%r10), %xmm4
    pand         sMask16(%rip), %xmm4
    pand         dMask16(%rip), %xmm5
    por          %xmm5, %xmm4
    movdqa       %xmm4, (%r13)
    add          $(16), %r10
    add          $(16), %r11
    add          $(16), %r13
    sub          $(2), %r12
    jmp          LL1gas_12
LNA2gas_12:  
    sub          $(8), %r12
    jl           LL4n2gas_12
LL8n2gas_12:  
    lddqu        (%r10), %xmm0
    lddqu        (16)(%r10), %xmm1
    lddqu        (32)(%r10), %xmm4
    lddqu        (48)(%r10), %xmm5
    movdqa       (%r13), %xmm2
    movdqa       (16)(%r13), %xmm3
    movdqa       (32)(%r13), %xmm6
    movdqa       (48)(%r13), %xmm7
    por          (%r11), %xmm0
    por          (16)(%r11), %xmm1
    por          (32)(%r11), %xmm4
    por          (48)(%r11), %xmm5
    pand         sMask16(%rip), %xmm0
    pand         sMask16(%rip), %xmm1
    pand         sMask16(%rip), %xmm4
    pand         sMask16(%rip), %xmm5
    pand         dMask16(%rip), %xmm2
    pand         dMask16(%rip), %xmm3
    pand         dMask16(%rip), %xmm6
    pand         dMask16(%rip), %xmm7
    por          %xmm2, %xmm0
    por          %xmm3, %xmm1
    por          %xmm6, %xmm4
    por          %xmm7, %xmm5
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    movdqa       %xmm4, (32)(%r13)
    movdqa       %xmm5, (48)(%r13)
    add          $(64), %r10
    add          $(64), %r11
    add          $(64), %r13
    sub          $(8), %r12
    jge          LL8n2gas_12
LL4n2gas_12:  
    add          $(4), %r12
    jl           LL2n2gas_12
    lddqu        (%r10), %xmm0
    lddqu        (16)(%r10), %xmm1
    movdqa       (%r13), %xmm2
    movdqa       (16)(%r13), %xmm3
    por          (%r11), %xmm0
    por          (16)(%r11), %xmm1
    pand         sMask16(%rip), %xmm0
    pand         sMask16(%rip), %xmm1
    pand         dMask16(%rip), %xmm2
    pand         dMask16(%rip), %xmm3
    por          %xmm2, %xmm0
    por          %xmm3, %xmm1
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    add          $(32), %r10
    add          $(32), %r11
    add          $(32), %r13
    sub          $(4), %r12
LL2n2gas_12:  
    add          $(2), %r12
    jl           LL1n3gas_12
    lddqu        (%r10), %xmm4
    movdqa       (%r13), %xmm5
    por          (%r11), %xmm4
    pand         sMask16(%rip), %xmm4
    pand         dMask16(%rip), %xmm5
    por          %xmm5, %xmm4
    movdqa       %xmm4, (%r13)
    add          $(16), %r10
    add          $(16), %r11
    add          $(16), %r13
    sub          $(2), %r12
    jmp          LL1n3gas_12
LNA3gas_12:  
    sub          $(4), %r12
    jl           LL2n3gas_12
LL4n3gas_12:  
    lddqu        (%r11), %xmm0
    lddqu        (16)(%r11), %xmm1
    lddqu        (%r10), %xmm2
    lddqu        (16)(%r10), %xmm3
    movdqa       (%r13), %xmm4
    movdqa       (16)(%r13), %xmm5
    por          %xmm2, %xmm0
    por          %xmm3, %xmm1
    pand         sMask16(%rip), %xmm0
    pand         sMask16(%rip), %xmm1
    pand         dMask16(%rip), %xmm4
    pand         dMask16(%rip), %xmm5
    por          %xmm4, %xmm0
    por          %xmm5, %xmm1
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    add          $(32), %r10
    add          $(32), %r11
    add          $(32), %r13
    sub          $(4), %r12
    jge          LL4n3gas_12
LL2n3gas_12:  
    add          $(2), %r12
    jl           LL1n3gas_12
    lddqu        (%r11), %xmm7
    lddqu        (%r10), %xmm6
    movdqa       (%r13), %xmm5
    por          %xmm6, %xmm7
    pand         sMask16(%rip), %xmm7
    pand         dMask16(%rip), %xmm5
    por          %xmm5, %xmm7
    movdqa       %xmm7, (%r13)
    add          $(16), %r10
    add          $(16), %r11
    add          $(16), %r13
    sub          $(2), %r12
LL1n3gas_12:  
    add          $(1), %r12
    jl           LL02gas_12
    movq         (%r10), %xmm5
    movq         (%r11), %xmm6
    movq         (%r13), %xmm7
    por          %xmm5, %xmm6
    pand         dMask16(%rip), %xmm7
    pand         sMask16(%rip), %xmm6
    por          %xmm7, %xmm6
    movq         %xmm6, (%r13)
LL02gas_12:  
    add          %rsi, %rdi
    add          %rcx, %rdx
    add          %r9, %r8
    sub          $(1), %ebp
    jnz          LL01gas_12
 
 
    pop          %r13
 
 
    pop          %r12
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownpi_XorC_16u_C1R

 
mfxownpi_XorC_16u_C1R:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    push         %r12
 
 
    movslq       %edx, %rdx
    movslq       %r8d, %r8
    mov          %rdi, %rax
    shl          $(16), %rdi
    and          $(65535), %rax
    or           %rdi, %rax
    mov          %eax, %eax
    movd         %eax, %xmm7
    mov          %rax, %rdi
    shl          $(32), %rdi
    or           %rax, %rdi
    pshufd       $(0), %xmm7, %xmm7
    movdqa       %xmm7, %xmm6
    mov          %r9d, %r9d
    movl         (32)(%rsp), %ebp
LL01gas_13:  
    mov          %rsi, %r10
    mov          %rcx, %r11
    mov          %r9, %r12
    test         $(1), %r11
    jz           LRunNgas_13
    sub          $(4), %r12
    jl           LL2gas_13
LL4ngas_13:  
    mov          (%r10), %rax
    xor          %rdi, %rax
    mov          %rax, (%r11)
    add          $(8), %r10
    add          $(8), %r11
    sub          $(4), %r12
    jge          LL4ngas_13
    jmp          LL2gas_13
LRunNgas_13:  
    mov          %r11, %rbx
    and          $(15), %rbx
    jz           LRungas_13
    sub          $(16), %rbx
    neg          %rbx
    shr          $(1), %rbx
    cmp          %rbx, %r12
    jl           LRungas_13
    sub          %rbx, %r12
LLN1gas_13:  
    movw         (%r10), %ax
    xor          %rdi, %rax
    movw         %ax, (%r11)
    add          $(2), %r10
    add          $(2), %r11
    sub          $(1), %rbx
    jnz          LLN1gas_13
LRungas_13:  
    test         $(15), %r10
    jnz          LNNAgas_13
    sub          $(32), %r12
    jl           LL16gas_13
LL32gas_13:  
    movdqa       (%r10), %xmm0
    movdqa       (16)(%r10), %xmm1
    movdqa       (32)(%r10), %xmm2
    movdqa       (48)(%r10), %xmm3
    pxor         %xmm6, %xmm0
    pxor         %xmm7, %xmm1
    pxor         %xmm6, %xmm2
    pxor         %xmm7, %xmm3
    movdqa       %xmm0, (%r11)
    movdqa       %xmm1, (16)(%r11)
    movdqa       %xmm2, (32)(%r11)
    movdqa       %xmm3, (48)(%r11)
    add          $(64), %r10
    add          $(64), %r11
    sub          $(32), %r12
    jge          LL32gas_13
LL16gas_13:  
    add          $(16), %r12
    jl           LL8gas_13
    movdqa       (%r10), %xmm4
    movdqa       (16)(%r10), %xmm5
    pxor         %xmm6, %xmm4
    pxor         %xmm7, %xmm5
    movdqa       %xmm4, (%r11)
    movdqa       %xmm5, (16)(%r11)
    add          $(32), %r10
    add          $(32), %r11
    sub          $(16), %r12
LL8gas_13:  
    add          $(8), %r12
    jl           LL4gas_13
    movdqa       (%r10), %xmm0
    pxor         %xmm6, %xmm0
    movdqa       %xmm0, (%r11)
    add          $(16), %r10
    add          $(16), %r11
    sub          $(8), %r12
    jmp          LL4gas_13
LNNAgas_13:  
    sub          $(32), %r12
    jl           LL16ngas_13
LL32ngas_13:  
    lddqu        (%r10), %xmm0
    lddqu        (16)(%r10), %xmm1
    lddqu        (32)(%r10), %xmm2
    lddqu        (48)(%r10), %xmm3
    pxor         %xmm6, %xmm0
    pxor         %xmm7, %xmm1
    pxor         %xmm6, %xmm2
    pxor         %xmm7, %xmm3
    movdqa       %xmm0, (%r11)
    movdqa       %xmm1, (16)(%r11)
    movdqa       %xmm2, (32)(%r11)
    movdqa       %xmm3, (48)(%r11)
    add          $(64), %r10
    add          $(64), %r11
    sub          $(32), %r12
    jge          LL32ngas_13
LL16ngas_13:  
    add          $(16), %r12
    jl           LL8ngas_13
    lddqu        (%r10), %xmm4
    lddqu        (16)(%r10), %xmm5
    pxor         %xmm6, %xmm4
    pxor         %xmm7, %xmm5
    movdqa       %xmm4, (%r11)
    movdqa       %xmm5, (16)(%r11)
    add          $(32), %r10
    add          $(32), %r11
    sub          $(16), %r12
LL8ngas_13:  
    add          $(8), %r12
    jl           LL4gas_13
    lddqu        (%r10), %xmm0
    pxor         %xmm6, %xmm0
    movdqa       %xmm0, (%r11)
    add          $(16), %r10
    add          $(16), %r11
    sub          $(8), %r12
LL4gas_13:  
    add          $(4), %r12
    jl           LL2gas_13
    mov          (%r10), %rax
    xor          %rdi, %rax
    mov          %rax, (%r11)
    add          $(8), %r10
    add          $(8), %r11
    sub          $(4), %r12
LL2gas_13:  
    add          $(2), %r12
    jl           LL1gas_13
    movl         (%r10), %eax
    xor          %rdi, %rax
    movl         %eax, (%r11)
    add          $(4), %r10
    add          $(4), %r11
    sub          $(2), %r12
LL1gas_13:  
    add          $(1), %r12
    jl           LL02gas_13
    movw         (%r10), %ax
    xor          %rdi, %rax
    movw         %ax, (%r11)
LL02gas_13:  
    add          %rdx, %rsi
    add          %r8, %rcx
    sub          $(1), %ebp
    jnz          LL01gas_13
 
 
    pop          %r12
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownpi_XorC_16u_C3R

 
mfxownpi_XorC_16u_C3R:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    push         %r12
 
 
    movslq       %edx, %rdx
    movslq       %r8d, %r8
    movd         (%rdi), %xmm7
    pinsrw       $(2), (4)(%rdi), %xmm7
    movdqa       %xmm7, %xmm2
    movdqa       %xmm7, %xmm3
    movdqa       %xmm7, %xmm4
    movdqa       %xmm7, %xmm5
    movdqa       %xmm7, %xmm6
    psllq        $(16), %xmm2
    psllq        $(32), %xmm3
    psllq        $(48), %xmm4
    psrlq        $(16), %xmm5
    psrlq        $(32), %xmm6
    por          %xmm6, %xmm2
    por          %xmm7, %xmm4
    por          %xmm5, %xmm3
    unpcklpd     %xmm3, %xmm4
    unpcklpd     %xmm4, %xmm2
    unpcklpd     %xmm2, %xmm3
    mov          %r9d, %r9d
    movl         (32)(%rsp), %ebp
LL01gas_14:  
    mov          %rsi, %r10
    mov          %rcx, %r11
    mov          %r9, %r12
    cmp          $(32), %r12
    jl           LNSgas_14
    test         $(1), %r11
    jz           LRunNgas_14
LNSgas_14:  
    sub          $(4), %r12
    jl           LL3gas_14
LNAgas_14:  
    movq         (%r10), %xmm5
    movq         (8)(%r10), %xmm6
    movq         (16)(%r10), %xmm7
    pxor         %xmm4, %xmm5
    pxor         %xmm3, %xmm6
    pxor         %xmm2, %xmm7
    movq         %xmm5, (%r11)
    movq         %xmm6, (8)(%r11)
    movq         %xmm7, (16)(%r11)
    add          $(24), %r10
    add          $(24), %r11
    sub          $(4), %r12
    jge          LNAgas_14
    jmp          LL3gas_14
LRunNgas_14:  
    mov          %r11, %rbx
    and          $(15), %rbx
    jz           LRungas_14
    cmp          $(14), %rbx
    jl           Lt12gas_14
    mov          $(3), %rbx
    jmp          Lt0gas_14
Lt12gas_14:  
    cmp          $(12), %rbx
    jl           Lt10gas_14
    mov          $(6), %rbx
    jmp          Lt0gas_14
Lt10gas_14:  
    cmp          $(10), %rbx
    jl           Lt8gas_14
    mov          $(1), %rbx
    jmp          Lt0gas_14
Lt8gas_14:  
    cmp          $(8), %rbx
    jl           Lt6gas_14
    mov          $(4), %rbx
    jmp          Lt0gas_14
Lt6gas_14:  
    cmp          $(6), %rbx
    jl           Lt4gas_14
    mov          $(7), %rbx
    jmp          Lt0gas_14
Lt4gas_14:  
    cmp          $(4), %rbx
    jl           Lt2gas_14
    mov          $(2), %rbx
    jmp          Lt0gas_14
Lt2gas_14:  
    mov          $(5), %rbx
Lt0gas_14:  
    sub          %rbx, %r12
LL1ngas_14:  
    movd         (%r10), %xmm0
    pinsrw       $(2), (4)(%r10), %xmm0
    pxor         %xmm4, %xmm0
    movd         %xmm0, (%r11)
    psrlq        $(32), %xmm0
    movd         %xmm0, %eax
    movw         %ax, (4)(%r11)
    add          $(6), %r10
    add          $(6), %r11
    sub          $(1), %rbx
    jnz          LL1ngas_14
LRungas_14:  
    test         $(15), %r10
    jnz          LNNAgas_14
    sub          $(8), %r12
    jl           LL4gas_14
LL8gas_14:  
    movdqa       (%r10), %xmm5
    movdqa       (16)(%r10), %xmm6
    movdqa       (32)(%r10), %xmm7
    pxor         %xmm4, %xmm5
    pxor         %xmm2, %xmm6
    pxor         %xmm3, %xmm7
    movdqa       %xmm5, (%r11)
    movdqa       %xmm6, (16)(%r11)
    movdqa       %xmm7, (32)(%r11)
    add          $(48), %r10
    add          $(48), %r11
    sub          $(8), %r12
    jge          LL8gas_14
LL4gas_14:  
    add          $(4), %r12
    jl           LL3gas_14
    movdqa       (%r10), %xmm0
    movq         (16)(%r10), %xmm1
    pxor         %xmm4, %xmm0
    pxor         %xmm2, %xmm1
    movdqa       %xmm0, (%r11)
    movq         %xmm1, (16)(%r11)
    add          $(24), %r10
    add          $(24), %r11
    sub          $(4), %r12
    jmp          LL3gas_14
LNNAgas_14:  
    sub          $(8), %r12
    jl           LL4ngas_14
LL8ngas_14:  
    lddqu        (%r10), %xmm5
    lddqu        (16)(%r10), %xmm6
    lddqu        (32)(%r10), %xmm7
    pxor         %xmm4, %xmm5
    pxor         %xmm2, %xmm6
    pxor         %xmm3, %xmm7
    movdqa       %xmm5, (%r11)
    movdqa       %xmm6, (16)(%r11)
    movdqa       %xmm7, (32)(%r11)
    add          $(48), %r10
    add          $(48), %r11
    sub          $(8), %r12
    jge          LL8ngas_14
LL4ngas_14:  
    add          $(4), %r12
    jl           LL3gas_14
    lddqu        (%r10), %xmm0
    movq         (16)(%r10), %xmm1
    pxor         %xmm4, %xmm0
    pxor         %xmm2, %xmm1
    movdqa       %xmm0, (%r11)
    movq         %xmm1, (16)(%r11)
    add          $(24), %r10
    add          $(24), %r11
    sub          $(4), %r12
LL3gas_14:  
    add          $(4), %r12
    jz           LL02gas_14
LL1gas_14:  
    movd         (%r10), %xmm0
    pinsrw       $(2), (4)(%r10), %xmm0
    pxor         %xmm4, %xmm0
    movd         %xmm0, (%r11)
    psrlq        $(32), %xmm0
    movd         %xmm0, %eax
    movw         %ax, (4)(%r11)
    add          $(6), %r10
    add          $(6), %r11
    sub          $(1), %r12
    jnz          LL1gas_14
LL02gas_14:  
    add          %rdx, %rsi
    add          %r8, %rcx
    sub          $(1), %ebp
    jnz          LL01gas_14
 
 
    pop          %r12
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownpi_XorC_16u_C4R

 
mfxownpi_XorC_16u_C4R:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    push         %r12
 
 
    movslq       %edx, %rdx
    movslq       %r8d, %r8
    movq         (%rdi), %xmm7
    pshufd       $(68), %xmm7, %xmm7
    mov          %r9d, %r9d
    movl         (32)(%rsp), %ebp
LL01gas_15:  
    mov          %rsi, %r10
    mov          %rcx, %r11
    mov          %r9, %r12
    test         $(7), %r11
    jz           LRunNgas_15
LNAgas_15:  
    movq         (%r10), %xmm0
    pxor         %xmm7, %xmm0
    movq         %xmm0, (%r11)
    add          $(8), %r10
    add          $(8), %r11
    sub          $(1), %r12
    jnz          LNAgas_15
    jmp          LL02gas_15
LRunNgas_15:  
    mov          %r11, %rbx
    and          $(15), %rbx
    jz           LRungas_15
    movq         (%r10), %xmm0
    pxor         %xmm7, %xmm0
    movq         %xmm0, (%r11)
    add          $(8), %r10
    add          $(8), %r11
    sub          $(1), %r12
LRungas_15:  
    test         $(15), %r10
    jnz          LNNAgas_15
    sub          $(8), %r12
    jl           LL4gas_15
LL8gas_15:  
    movdqa       (%r10), %xmm0
    movdqa       (16)(%r10), %xmm1
    movdqa       (32)(%r10), %xmm2
    movdqa       (48)(%r10), %xmm3
    pxor         %xmm7, %xmm0
    pxor         %xmm7, %xmm1
    pxor         %xmm7, %xmm2
    pxor         %xmm7, %xmm3
    movdqa       %xmm0, (%r11)
    movdqa       %xmm1, (16)(%r11)
    movdqa       %xmm2, (32)(%r11)
    movdqa       %xmm3, (48)(%r11)
    add          $(64), %r10
    add          $(64), %r11
    sub          $(8), %r12
    jge          LL8gas_15
LL4gas_15:  
    add          $(4), %r12
    jl           LL2gas_15
    movdqa       (%r10), %xmm4
    movdqa       (16)(%r10), %xmm5
    pxor         %xmm7, %xmm4
    pxor         %xmm7, %xmm5
    movdqa       %xmm4, (%r11)
    movdqa       %xmm5, (16)(%r11)
    add          $(32), %r10
    add          $(32), %r11
    sub          $(4), %r12
LL2gas_15:  
    add          $(2), %r12
    jl           LL1gas_15
    movdqa       (%r10), %xmm6
    pxor         %xmm7, %xmm6
    movdqa       %xmm6, (%r11)
    add          $(16), %r10
    add          $(16), %r11
    sub          $(2), %r12
    jmp          LL1gas_15
LNNAgas_15:  
    sub          $(8), %r12
    jl           LL4ngas_15
LL8ngas_15:  
    lddqu        (%r10), %xmm0
    lddqu        (16)(%r10), %xmm1
    lddqu        (32)(%r10), %xmm2
    lddqu        (48)(%r10), %xmm3
    pxor         %xmm7, %xmm0
    pxor         %xmm7, %xmm1
    pxor         %xmm7, %xmm2
    pxor         %xmm7, %xmm3
    movdqa       %xmm0, (%r11)
    movdqa       %xmm1, (16)(%r11)
    movdqa       %xmm2, (32)(%r11)
    movdqa       %xmm3, (48)(%r11)
    add          $(64), %r10
    add          $(64), %r11
    sub          $(8), %r12
    jge          LL8ngas_15
LL4ngas_15:  
    add          $(4), %r12
    jl           LL2ngas_15
    lddqu        (%r10), %xmm4
    lddqu        (16)(%r10), %xmm5
    pxor         %xmm7, %xmm4
    pxor         %xmm7, %xmm5
    movdqa       %xmm4, (%r11)
    movdqa       %xmm5, (16)(%r11)
    add          $(32), %r10
    add          $(32), %r11
    sub          $(4), %r12
LL2ngas_15:  
    add          $(2), %r12
    jl           LL1gas_15
    lddqu        (%r10), %xmm6
    pxor         %xmm7, %xmm6
    movdqa       %xmm6, (%r11)
    add          $(16), %r10
    add          $(16), %r11
    sub          $(2), %r12
LL1gas_15:  
    add          $(1), %r12
    jl           LL02gas_15
    movq         (%r10), %xmm0
    pxor         %xmm7, %xmm0
    movq         %xmm0, (%r11)
LL02gas_15:  
    add          %rdx, %rsi
    add          %r8, %rcx
    sub          $(1), %ebp
    jnz          LL01gas_15
 
 
    pop          %r12
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownpi_XorC_16u_AC4R

 
mfxownpi_XorC_16u_AC4R:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    push         %r12
 
 
    movslq       %edx, %rdx
    movslq       %r8d, %r8
    movd         (%rdi), %xmm7
    pinsrw       $(2), (4)(%rdi), %xmm7
    pshufd       $(68), %xmm7, %xmm7
    mov          %r9d, %r9d
    movl         (32)(%rsp), %ebp
LL01gas_16:  
    mov          %rsi, %r10
    mov          %rcx, %r11
    mov          %r9, %r12
    test         $(7), %r11
    jz           LRunNgas_16
LNAgas_16:  
    movq         (%r10), %xmm0
    movq         (%r11), %xmm1
    pxor         %xmm7, %xmm0
    pand         dMask16(%rip), %xmm1
    pand         sMask16(%rip), %xmm0
    por          %xmm1, %xmm0
    movq         %xmm0, (%r11)
    add          $(8), %r10
    add          $(8), %r11
    sub          $(1), %r12
    jnz          LNAgas_16
    jmp          LL02gas_16
LRunNgas_16:  
    mov          %r11, %rbx
    and          $(15), %rbx
    jz           LRungas_16
    movq         (%r10), %xmm0
    movq         (%r11), %xmm1
    pxor         %xmm7, %xmm0
    pand         dMask16(%rip), %xmm1
    pand         sMask16(%rip), %xmm0
    por          %xmm1, %xmm0
    movq         %xmm0, (%r11)
    add          $(8), %r10
    add          $(8), %r11
    sub          $(1), %r12
LRungas_16:  
    test         $(15), %r10
    jnz          LNNAgas_16
    sub          $(4), %r12
    jl           LL2gas_16
LL4gas_16:  
    movdqa       (%r10), %xmm0
    movdqa       (16)(%r10), %xmm1
    movdqa       (%r11), %xmm2
    movdqa       (16)(%r11), %xmm3
    pxor         %xmm7, %xmm0
    pxor         %xmm7, %xmm1
    pand         dMask16(%rip), %xmm2
    pand         dMask16(%rip), %xmm3
    pand         sMask16(%rip), %xmm0
    pand         sMask16(%rip), %xmm1
    por          %xmm2, %xmm0
    por          %xmm3, %xmm1
    movdqa       %xmm0, (%r11)
    movdqa       %xmm1, (16)(%r11)
    add          $(32), %r10
    add          $(32), %r11
    sub          $(4), %r12
    jge          LL4gas_16
LL2gas_16:  
    add          $(2), %r12
    jl           LL1gas_16
    movdqa       (%r10), %xmm4
    movdqa       (%r11), %xmm5
    pxor         %xmm7, %xmm4
    pand         dMask16(%rip), %xmm5
    pand         sMask16(%rip), %xmm4
    por          %xmm5, %xmm4
    movdqa       %xmm4, (%r11)
    add          $(16), %r10
    add          $(16), %r11
    sub          $(2), %r12
    jmp          LL1gas_16
LNNAgas_16:  
    sub          $(4), %r12
    jl           LL2ngas_16
LL4ngas_16:  
    lddqu        (%r10), %xmm0
    lddqu        (16)(%r10), %xmm1
    movdqa       (%r11), %xmm2
    movdqa       (16)(%r11), %xmm3
    pxor         %xmm7, %xmm0
    pxor         %xmm7, %xmm1
    pand         dMask16(%rip), %xmm2
    pand         dMask16(%rip), %xmm3
    pand         sMask16(%rip), %xmm0
    pand         sMask16(%rip), %xmm1
    por          %xmm2, %xmm0
    por          %xmm3, %xmm1
    movdqa       %xmm0, (%r11)
    movdqa       %xmm1, (16)(%r11)
    add          $(32), %r10
    add          $(32), %r11
    sub          $(4), %r12
    jge          LL4ngas_16
LL2ngas_16:  
    add          $(2), %r12
    jl           LL1gas_16
    lddqu        (%r10), %xmm4
    movdqa       (%r11), %xmm5
    pxor         %xmm7, %xmm4
    pand         dMask16(%rip), %xmm5
    pand         sMask16(%rip), %xmm4
    por          %xmm5, %xmm4
    movdqa       %xmm4, (%r11)
    add          $(16), %r10
    add          $(16), %r11
    sub          $(2), %r12
LL1gas_16:  
    add          $(1), %r12
    jl           LL02gas_16
    movq         (%r10), %xmm6
    movq         (%r11), %xmm1
    pxor         %xmm7, %xmm6
    pand         dMask16(%rip), %xmm1
    pand         sMask16(%rip), %xmm6
    por          %xmm1, %xmm6
    movq         %xmm6, (%r11)
LL02gas_16:  
    add          %rdx, %rsi
    add          %r8, %rcx
    sub          $(1), %ebp
    jnz          LL01gas_16
 
 
    pop          %r12
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownpi_Xor_16u_C1R

 
mfxownpi_Xor_16u_C1R:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    push         %r12
 
 
    push         %r13
 
 
    movslq       %esi, %rsi
    movslq       %ecx, %rcx
    movslq       %r9d, %r9
    movl         (48)(%rsp), %ebp
LL01gas_17:  
    mov          %rdi, %r10
    mov          %rdx, %r11
    mov          %r8, %r13
    movl         (40)(%rsp), %r12d
    test         $(1), %r13
    jz           LRunNgas_17
    sub          $(4), %r12
    jl           LL2gas_17
LL4ngas_17:  
    mov          (%r11), %rax
    xor          (%r10), %rax
    mov          %rax, (%r13)
    add          $(8), %r11
    add          $(8), %r10
    add          $(8), %r13
    sub          $(4), %r12
    jge          LL4ngas_17
    jmp          LL2gas_17
LRunNgas_17:  
    mov          %r13, %rbx
    and          $(15), %rbx
    jz           LRungas_17
    sub          $(16), %rbx
    neg          %rbx
    shr          $(1), %rbx
    cmp          %rbx, %r12
    jl           LRungas_17
    sub          %rbx, %r12
LLN1gas_17:  
    movw         (%r11), %ax
    xorw         (%r10), %ax
    movw         %ax, (%r13)
    add          $(2), %r11
    add          $(2), %r10
    add          $(2), %r13
    sub          $(1), %rbx
    jnz          LLN1gas_17
LRungas_17:  
    test         $(15), %r11
    jnz          LNA1gas_17
    test         $(15), %r10
    jnz          LNA2gas_17
    sub          $(64), %r12
    jl           LL32gas_17
LL64gas_17:  
    movdqa       (%r11), %xmm0
    movdqa       (16)(%r11), %xmm1
    movdqa       (32)(%r11), %xmm2
    movdqa       (48)(%r11), %xmm3
    movdqa       (64)(%r11), %xmm4
    movdqa       (80)(%r11), %xmm5
    movdqa       (96)(%r11), %xmm6
    movdqa       (112)(%r11), %xmm7
    pxor         (%r10), %xmm0
    pxor         (16)(%r10), %xmm1
    pxor         (32)(%r10), %xmm2
    pxor         (48)(%r10), %xmm3
    pxor         (64)(%r10), %xmm4
    pxor         (80)(%r10), %xmm5
    pxor         (96)(%r10), %xmm6
    pxor         (112)(%r10), %xmm7
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    movdqa       %xmm2, (32)(%r13)
    movdqa       %xmm3, (48)(%r13)
    movdqa       %xmm4, (64)(%r13)
    movdqa       %xmm5, (80)(%r13)
    movdqa       %xmm6, (96)(%r13)
    movdqa       %xmm7, (112)(%r13)
    add          $(128), %r11
    add          $(128), %r10
    add          $(128), %r13
    sub          $(64), %r12
    jge          LL64gas_17
LL32gas_17:  
    add          $(32), %r12
    jl           LL16gas_17
    movdqa       (%r11), %xmm0
    movdqa       (16)(%r11), %xmm1
    movdqa       (32)(%r11), %xmm2
    movdqa       (48)(%r11), %xmm3
    pxor         (%r10), %xmm0
    pxor         (16)(%r10), %xmm1
    pxor         (32)(%r10), %xmm2
    pxor         (48)(%r10), %xmm3
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    movdqa       %xmm2, (32)(%r13)
    movdqa       %xmm3, (48)(%r13)
    add          $(64), %r11
    add          $(64), %r10
    add          $(64), %r13
    sub          $(32), %r12
LL16gas_17:  
    add          $(16), %r12
    jl           LL8gas_17
    movdqa       (%r11), %xmm4
    movdqa       (16)(%r11), %xmm5
    pxor         (%r10), %xmm4
    pxor         (16)(%r10), %xmm5
    movdqa       %xmm4, (%r13)
    movdqa       %xmm5, (16)(%r13)
    add          $(32), %r11
    add          $(32), %r10
    add          $(32), %r13
    sub          $(16), %r12
LL8gas_17:  
    add          $(8), %r12
    jl           LL4gas_17
    movdqa       (%r11), %xmm6
    pxor         (%r10), %xmm6
    movdqa       %xmm6, (%r13)
    add          $(16), %r11
    add          $(16), %r10
    add          $(16), %r13
    sub          $(8), %r12
    jmp          LL4gas_17
LNA1gas_17:  
    test         $(15), %r10
    jnz          LNA3gas_17
    sub          $(64), %r12
    jl           LL32n1gas_17
LL64n1gas_17:  
    lddqu        (%r11), %xmm0
    lddqu        (16)(%r11), %xmm1
    lddqu        (32)(%r11), %xmm2
    lddqu        (48)(%r11), %xmm3
    lddqu        (64)(%r11), %xmm4
    lddqu        (80)(%r11), %xmm5
    lddqu        (96)(%r11), %xmm6
    lddqu        (112)(%r11), %xmm7
    pxor         (%r10), %xmm0
    pxor         (16)(%r10), %xmm1
    pxor         (32)(%r10), %xmm2
    pxor         (48)(%r10), %xmm3
    pxor         (64)(%r10), %xmm4
    pxor         (80)(%r10), %xmm5
    pxor         (96)(%r10), %xmm6
    pxor         (112)(%r10), %xmm7
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    movdqa       %xmm2, (32)(%r13)
    movdqa       %xmm3, (48)(%r13)
    movdqa       %xmm4, (64)(%r13)
    movdqa       %xmm5, (80)(%r13)
    movdqa       %xmm6, (96)(%r13)
    movdqa       %xmm7, (112)(%r13)
    add          $(128), %r11
    add          $(128), %r10
    add          $(128), %r13
    sub          $(64), %r12
    jge          LL64n1gas_17
LL32n1gas_17:  
    add          $(32), %r12
    jl           LL16n1gas_17
    lddqu        (%r11), %xmm0
    lddqu        (16)(%r11), %xmm1
    lddqu        (32)(%r11), %xmm2
    lddqu        (48)(%r11), %xmm3
    pxor         (%r10), %xmm0
    pxor         (16)(%r10), %xmm1
    pxor         (32)(%r10), %xmm2
    pxor         (48)(%r10), %xmm3
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    movdqa       %xmm2, (32)(%r13)
    movdqa       %xmm3, (48)(%r13)
    add          $(64), %r11
    add          $(64), %r10
    add          $(64), %r13
    sub          $(32), %r12
LL16n1gas_17:  
    add          $(16), %r12
    jl           LL8n1gas_17
    lddqu        (%r11), %xmm4
    lddqu        (16)(%r11), %xmm5
    pxor         (%r10), %xmm4
    pxor         (16)(%r10), %xmm5
    movdqa       %xmm4, (%r13)
    movdqa       %xmm5, (16)(%r13)
    add          $(32), %r11
    add          $(32), %r10
    add          $(32), %r13
    sub          $(16), %r12
LL8n1gas_17:  
    add          $(8), %r12
    jl           LL4gas_17
    lddqu        (%r11), %xmm6
    pxor         (%r10), %xmm6
    movdqa       %xmm6, (%r13)
    add          $(16), %r11
    add          $(16), %r10
    add          $(16), %r13
    sub          $(8), %r12
    jmp          LL4gas_17
LNA2gas_17:  
    sub          $(64), %r12
    jl           LL32n2gas_17
LL64n2gas_17:  
    lddqu        (%r10), %xmm0
    lddqu        (16)(%r10), %xmm1
    lddqu        (32)(%r10), %xmm2
    lddqu        (48)(%r10), %xmm3
    lddqu        (64)(%r10), %xmm4
    lddqu        (80)(%r10), %xmm5
    lddqu        (96)(%r10), %xmm6
    lddqu        (112)(%r10), %xmm7
    pxor         (%r11), %xmm0
    pxor         (16)(%r11), %xmm1
    pxor         (32)(%r11), %xmm2
    pxor         (48)(%r11), %xmm3
    pxor         (64)(%r11), %xmm4
    pxor         (80)(%r11), %xmm5
    pxor         (96)(%r11), %xmm6
    pxor         (112)(%r11), %xmm7
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    movdqa       %xmm2, (32)(%r13)
    movdqa       %xmm3, (48)(%r13)
    movdqa       %xmm4, (64)(%r13)
    movdqa       %xmm5, (80)(%r13)
    movdqa       %xmm6, (96)(%r13)
    movdqa       %xmm7, (112)(%r13)
    add          $(128), %r11
    add          $(128), %r10
    add          $(128), %r13
    sub          $(64), %r12
    jge          LL64n2gas_17
LL32n2gas_17:  
    add          $(32), %r12
    jl           LL16n2gas_17
    lddqu        (%r10), %xmm0
    lddqu        (16)(%r10), %xmm1
    lddqu        (32)(%r10), %xmm2
    lddqu        (48)(%r10), %xmm3
    pxor         (%r11), %xmm0
    pxor         (16)(%r11), %xmm1
    pxor         (32)(%r11), %xmm2
    pxor         (48)(%r11), %xmm3
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    movdqa       %xmm2, (32)(%r13)
    movdqa       %xmm3, (48)(%r13)
    add          $(64), %r11
    add          $(64), %r10
    add          $(64), %r13
    sub          $(32), %r12
LL16n2gas_17:  
    add          $(16), %r12
    jl           LL8n2gas_17
    lddqu        (%r10), %xmm4
    lddqu        (16)(%r10), %xmm5
    pxor         (%r11), %xmm4
    pxor         (16)(%r11), %xmm5
    movdqa       %xmm4, (%r13)
    movdqa       %xmm5, (16)(%r13)
    add          $(32), %r11
    add          $(32), %r10
    add          $(32), %r13
    sub          $(16), %r12
LL8n2gas_17:  
    add          $(8), %r12
    jl           LL4gas_17
    lddqu        (%r10), %xmm6
    pxor         (%r11), %xmm6
    movdqa       %xmm6, (%r13)
    add          $(16), %r11
    add          $(16), %r10
    add          $(16), %r13
    sub          $(8), %r12
    jmp          LL4gas_17
LNA3gas_17:  
    sub          $(32), %r12
    jl           LL16n3gas_17
LL32n3gas_17:  
    lddqu        (%r11), %xmm0
    lddqu        (16)(%r11), %xmm1
    lddqu        (32)(%r11), %xmm2
    lddqu        (48)(%r11), %xmm3
    lddqu        (%r10), %xmm4
    lddqu        (16)(%r10), %xmm5
    lddqu        (32)(%r10), %xmm6
    lddqu        (48)(%r10), %xmm7
    pxor         %xmm4, %xmm0
    pxor         %xmm5, %xmm1
    pxor         %xmm6, %xmm2
    pxor         %xmm7, %xmm3
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    movdqa       %xmm2, (32)(%r13)
    movdqa       %xmm3, (48)(%r13)
    add          $(64), %r11
    add          $(64), %r10
    add          $(64), %r13
    sub          $(32), %r12
    jge          LL32n3gas_17
LL16n3gas_17:  
    add          $(16), %r12
    jl           LL8n3gas_17
    lddqu        (%r11), %xmm0
    lddqu        (16)(%r11), %xmm1
    lddqu        (%r10), %xmm2
    lddqu        (16)(%r10), %xmm3
    pxor         %xmm2, %xmm0
    pxor         %xmm3, %xmm1
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    add          $(32), %r11
    add          $(32), %r10
    add          $(32), %r13
    sub          $(16), %r12
LL8n3gas_17:  
    add          $(8), %r12
    jl           LL4gas_17
    lddqu        (%r11), %xmm4
    lddqu        (%r10), %xmm5
    pxor         %xmm5, %xmm4
    movdqa       %xmm4, (%r13)
    add          $(16), %r11
    add          $(16), %r10
    add          $(16), %r13
    sub          $(8), %r12
LL4gas_17:  
    add          $(4), %r12
    jl           LL2gas_17
    mov          (%r11), %rax
    xor          (%r10), %rax
    mov          %rax, (%r13)
    add          $(8), %r11
    add          $(8), %r10
    add          $(8), %r13
    sub          $(4), %r12
LL2gas_17:  
    add          $(2), %r12
    jl           LL1gas_17
    movl         (%r11), %eax
    xorl         (%r10), %eax
    movl         %eax, (%r13)
    add          $(4), %r11
    add          $(4), %r10
    add          $(4), %r13
    sub          $(2), %r12
LL1gas_17:  
    add          $(1), %r12
    jl           LL02gas_17
    movw         (%r11), %ax
    xorw         (%r10), %ax
    movw         %ax, (%r13)
LL02gas_17:  
    add          %rsi, %rdi
    add          %rcx, %rdx
    add          %r9, %r8
    sub          $(1), %ebp
    jnz          LL01gas_17
 
 
    pop          %r13
 
 
    pop          %r12
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownpi_Xor_16u_AC4R

 
mfxownpi_Xor_16u_AC4R:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    push         %r12
 
 
    push         %r13
 
 
    movslq       %esi, %rsi
    movslq       %ecx, %rcx
    movslq       %r9d, %r9
    movl         (48)(%rsp), %ebp
LL01gas_18:  
    mov          %rdi, %r10
    mov          %rdx, %r11
    mov          %r8, %r13
    movl         (40)(%rsp), %r12d
    test         $(7), %r13
    jz           LRunNgas_18
LNAgas_18:  
    movq         (%r11), %xmm0
    movq         (%r10), %xmm2
    movq         (%r13), %xmm1
    pxor         %xmm2, %xmm0
    pand         dMask16(%rip), %xmm1
    pand         sMask16(%rip), %xmm0
    por          %xmm1, %xmm0
    movq         %xmm0, (%r13)
    add          $(8), %r10
    add          $(8), %r11
    add          $(8), %r13
    sub          $(1), %r12
    jnz          LNAgas_18
    jmp          LL02gas_18
LRunNgas_18:  
    mov          %r13, %rbx
    and          $(15), %rbx
    jz           LRungas_18
    movq         (%r11), %xmm0
    movq         (%r10), %xmm2
    movq         (%r13), %xmm1
    pxor         %xmm2, %xmm0
    pand         dMask16(%rip), %xmm1
    pand         sMask16(%rip), %xmm0
    por          %xmm1, %xmm0
    movq         %xmm0, (%r13)
    add          $(8), %r10
    add          $(8), %r11
    add          $(8), %r13
    sub          $(1), %r12
LRungas_18:  
    test         $(15), %r11
    jnz          LNA1gas_18
    test         $(15), %r10
    jnz          LNA2gas_18
    sub          $(8), %r12
    jl           LL4gas_18
LL8gas_18:  
    movdqa       (%r11), %xmm0
    movdqa       (16)(%r11), %xmm1
    movdqa       (32)(%r11), %xmm4
    movdqa       (48)(%r11), %xmm5
    movdqa       (%r13), %xmm2
    movdqa       (16)(%r13), %xmm3
    movdqa       (32)(%r13), %xmm6
    movdqa       (48)(%r13), %xmm7
    pxor         (%r10), %xmm0
    pxor         (16)(%r10), %xmm1
    pxor         (32)(%r10), %xmm4
    pxor         (48)(%r10), %xmm5
    pand         sMask16(%rip), %xmm0
    pand         sMask16(%rip), %xmm1
    pand         sMask16(%rip), %xmm4
    pand         sMask16(%rip), %xmm5
    pand         dMask16(%rip), %xmm2
    pand         dMask16(%rip), %xmm3
    pand         dMask16(%rip), %xmm6
    pand         dMask16(%rip), %xmm7
    por          %xmm2, %xmm0
    por          %xmm3, %xmm1
    por          %xmm6, %xmm4
    por          %xmm7, %xmm5
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    movdqa       %xmm4, (32)(%r13)
    movdqa       %xmm5, (48)(%r13)
    add          $(64), %r10
    add          $(64), %r11
    add          $(64), %r13
    sub          $(8), %r12
    jge          LL8gas_18
LL4gas_18:  
    add          $(4), %r12
    jl           LL2gas_18
    movdqa       (%r11), %xmm0
    movdqa       (16)(%r11), %xmm1
    movdqa       (%r13), %xmm2
    movdqa       (16)(%r13), %xmm3
    pxor         (%r10), %xmm0
    pxor         (16)(%r10), %xmm1
    pand         sMask16(%rip), %xmm0
    pand         sMask16(%rip), %xmm1
    pand         dMask16(%rip), %xmm2
    pand         dMask16(%rip), %xmm3
    por          %xmm2, %xmm0
    por          %xmm3, %xmm1
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    add          $(32), %r10
    add          $(32), %r11
    add          $(32), %r13
    sub          $(4), %r12
LL2gas_18:  
    add          $(2), %r12
    jl           LL1gas_18
    movdqa       (%r11), %xmm4
    movdqa       (%r13), %xmm5
    pxor         (%r10), %xmm4
    pand         sMask16(%rip), %xmm4
    pand         dMask16(%rip), %xmm5
    por          %xmm5, %xmm4
    movdqa       %xmm4, (%r13)
    add          $(16), %r10
    add          $(16), %r11
    add          $(16), %r13
    sub          $(2), %r12
LL1gas_18:  
    add          $(1), %r12
    jl           LL02gas_18
    movq         (%r11), %xmm6
    movq         (%r13), %xmm7
    pxor         (%r10), %xmm6
    pand         dMask16(%rip), %xmm7
    pand         sMask16(%rip), %xmm6
    por          %xmm7, %xmm6
    movq         %xmm6, (%r13)
    jmp          LL02gas_18
LNA1gas_18:  
    test         $(15), %r10
    jnz          LNA3gas_18
    sub          $(8), %r12
    jl           LL4n1gas_18
LL8n1gas_18:  
    lddqu        (%r11), %xmm0
    lddqu        (16)(%r11), %xmm1
    lddqu        (32)(%r11), %xmm4
    lddqu        (48)(%r11), %xmm5
    movdqa       (%r13), %xmm2
    movdqa       (16)(%r13), %xmm3
    movdqa       (32)(%r13), %xmm6
    movdqa       (48)(%r13), %xmm7
    pxor         (%r10), %xmm0
    pxor         (16)(%r10), %xmm1
    pxor         (32)(%r10), %xmm4
    pxor         (48)(%r10), %xmm5
    pand         sMask16(%rip), %xmm0
    pand         sMask16(%rip), %xmm1
    pand         sMask16(%rip), %xmm4
    pand         sMask16(%rip), %xmm5
    pand         dMask16(%rip), %xmm2
    pand         dMask16(%rip), %xmm3
    pand         dMask16(%rip), %xmm6
    pand         dMask16(%rip), %xmm7
    por          %xmm2, %xmm0
    por          %xmm3, %xmm1
    por          %xmm6, %xmm4
    por          %xmm7, %xmm5
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    movdqa       %xmm4, (32)(%r13)
    movdqa       %xmm5, (48)(%r13)
    add          $(64), %r10
    add          $(64), %r11
    add          $(64), %r13
    sub          $(8), %r12
    jge          LL8n1gas_18
LL4n1gas_18:  
    add          $(4), %r12
    jl           LL2n1gas_18
    lddqu        (%r11), %xmm0
    lddqu        (16)(%r11), %xmm1
    movdqa       (%r13), %xmm2
    movdqa       (16)(%r13), %xmm3
    pxor         (%r10), %xmm0
    pxor         (16)(%r10), %xmm1
    pand         sMask16(%rip), %xmm0
    pand         sMask16(%rip), %xmm1
    pand         dMask16(%rip), %xmm2
    pand         dMask16(%rip), %xmm3
    por          %xmm2, %xmm0
    por          %xmm3, %xmm1
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    add          $(32), %r10
    add          $(32), %r11
    add          $(32), %r13
    sub          $(4), %r12
LL2n1gas_18:  
    add          $(2), %r12
    jl           LL1gas_18
    lddqu        (%r11), %xmm4
    movdqa       (%r13), %xmm5
    pxor         (%r10), %xmm4
    pand         sMask16(%rip), %xmm4
    pand         dMask16(%rip), %xmm5
    por          %xmm5, %xmm4
    movdqa       %xmm4, (%r13)
    add          $(16), %r10
    add          $(16), %r11
    add          $(16), %r13
    sub          $(2), %r12
    jmp          LL1gas_18
LNA2gas_18:  
    sub          $(8), %r12
    jl           LL4n2gas_18
LL8n2gas_18:  
    lddqu        (%r10), %xmm0
    lddqu        (16)(%r10), %xmm1
    lddqu        (32)(%r10), %xmm4
    lddqu        (48)(%r10), %xmm5
    movdqa       (%r13), %xmm2
    movdqa       (16)(%r13), %xmm3
    movdqa       (32)(%r13), %xmm6
    movdqa       (48)(%r13), %xmm7
    pxor         (%r11), %xmm0
    pxor         (16)(%r11), %xmm1
    pxor         (32)(%r11), %xmm4
    pxor         (48)(%r11), %xmm5
    pand         sMask16(%rip), %xmm0
    pand         sMask16(%rip), %xmm1
    pand         sMask16(%rip), %xmm4
    pand         sMask16(%rip), %xmm5
    pand         dMask16(%rip), %xmm2
    pand         dMask16(%rip), %xmm3
    pand         dMask16(%rip), %xmm6
    pand         dMask16(%rip), %xmm7
    por          %xmm2, %xmm0
    por          %xmm3, %xmm1
    por          %xmm6, %xmm4
    por          %xmm7, %xmm5
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    movdqa       %xmm4, (32)(%r13)
    movdqa       %xmm5, (48)(%r13)
    add          $(64), %r10
    add          $(64), %r11
    add          $(64), %r13
    sub          $(8), %r12
    jge          LL8n2gas_18
LL4n2gas_18:  
    add          $(4), %r12
    jl           LL2n2gas_18
    lddqu        (%r10), %xmm0
    lddqu        (16)(%r10), %xmm1
    movdqa       (%r13), %xmm2
    movdqa       (16)(%r13), %xmm3
    pxor         (%r11), %xmm0
    pxor         (16)(%r11), %xmm1
    pand         sMask16(%rip), %xmm0
    pand         sMask16(%rip), %xmm1
    pand         dMask16(%rip), %xmm2
    pand         dMask16(%rip), %xmm3
    por          %xmm2, %xmm0
    por          %xmm3, %xmm1
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    add          $(32), %r10
    add          $(32), %r11
    add          $(32), %r13
    sub          $(4), %r12
LL2n2gas_18:  
    add          $(2), %r12
    jl           LL1n3gas_18
    lddqu        (%r10), %xmm4
    movdqa       (%r13), %xmm5
    pxor         (%r11), %xmm4
    pand         sMask16(%rip), %xmm4
    pand         dMask16(%rip), %xmm5
    por          %xmm5, %xmm4
    movdqa       %xmm4, (%r13)
    add          $(16), %r10
    add          $(16), %r11
    add          $(16), %r13
    sub          $(2), %r12
    jmp          LL1n3gas_18
LNA3gas_18:  
    sub          $(4), %r12
    jl           LL2n3gas_18
LL4n3gas_18:  
    lddqu        (%r11), %xmm0
    lddqu        (16)(%r11), %xmm1
    lddqu        (%r10), %xmm2
    lddqu        (16)(%r10), %xmm3
    movdqa       (%r13), %xmm4
    movdqa       (16)(%r13), %xmm5
    pxor         %xmm2, %xmm0
    pxor         %xmm3, %xmm1
    pand         sMask16(%rip), %xmm0
    pand         sMask16(%rip), %xmm1
    pand         dMask16(%rip), %xmm4
    pand         dMask16(%rip), %xmm5
    por          %xmm4, %xmm0
    por          %xmm5, %xmm1
    movdqa       %xmm0, (%r13)
    movdqa       %xmm1, (16)(%r13)
    add          $(32), %r10
    add          $(32), %r11
    add          $(32), %r13
    sub          $(4), %r12
    jge          LL4n3gas_18
LL2n3gas_18:  
    add          $(2), %r12
    jl           LL1n3gas_18
    lddqu        (%r11), %xmm7
    lddqu        (%r10), %xmm6
    movdqa       (%r13), %xmm5
    pxor         %xmm6, %xmm7
    pand         sMask16(%rip), %xmm7
    pand         dMask16(%rip), %xmm5
    por          %xmm5, %xmm7
    movdqa       %xmm7, (%r13)
    add          $(16), %r10
    add          $(16), %r11
    add          $(16), %r13
    sub          $(2), %r12
LL1n3gas_18:  
    add          $(1), %r12
    jl           LL02gas_18
    movq         (%r10), %xmm5
    movq         (%r11), %xmm6
    movq         (%r13), %xmm7
    pxor         %xmm5, %xmm6
    pand         dMask16(%rip), %xmm7
    pand         sMask16(%rip), %xmm6
    por          %xmm7, %xmm6
    movq         %xmm6, (%r13)
LL02gas_18:  
    add          %rsi, %rdi
    add          %rcx, %rdx
    add          %r9, %r8
    sub          $(1), %ebp
    jnz          LL01gas_18
 
 
    pop          %r13
 
 
    pop          %r12
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
