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
 
.globl mfxownps_LShiftC_8u_I

 
mfxownps_LShiftC_8u_I:
 
 
    push         %rbx
 
 
    mov          %edx, %edx
    mov          %edi, %ecx
    mov          %rsi, %rbx
    and          $(15), %rbx
    jz           LRungas_1
    sub          $(16), %rbx
    neg          %rbx
    cmp          %rbx, %rdx
    jl           LRungas_1
    sub          %rbx, %rdx
LNAgas_1:  
    movb         (%rsi), %al
    shl          %cl, %al
    movb         %al, (%rsi)
    add          $(1), %rsi
    sub          $(1), %rbx
    jnz          LNAgas_1
LRungas_1:  
    movd         %edi, %xmm7
    pcmpeqb      %xmm0, %xmm0
    psllw        %xmm7, %xmm0
    psllw        $(8), %xmm0
    movdqa       %xmm0, %xmm6
    psrlw        $(8), %xmm6
    por          %xmm6, %xmm0
    sub          $(64), %rdx
    jl           LL32gas_1
LL64gas_1:  
    movdqa       (%rsi), %xmm3
    psllw        %xmm7, %xmm3
    pand         %xmm0, %xmm3
    movdqa       %xmm3, (%rsi)
    movdqa       (16)(%rsi), %xmm4
    psllw        %xmm7, %xmm4
    pand         %xmm0, %xmm4
    movdqa       %xmm4, (16)(%rsi)
    movdqa       (32)(%rsi), %xmm5
    psllw        %xmm7, %xmm5
    pand         %xmm0, %xmm5
    movdqa       %xmm5, (32)(%rsi)
    movdqa       (48)(%rsi), %xmm6
    psllw        %xmm7, %xmm6
    pand         %xmm0, %xmm6
    movdqa       %xmm6, (48)(%rsi)
    add          $(64), %rsi
    sub          $(64), %rdx
    jge          LL64gas_1
LL32gas_1:  
    add          $(32), %rdx
    jl           LL16gas_1
    movdqa       (%rsi), %xmm1
    psllw        %xmm7, %xmm1
    pand         %xmm0, %xmm1
    movdqa       %xmm1, (%rsi)
    movdqa       (16)(%rsi), %xmm2
    psllw        %xmm7, %xmm2
    pand         %xmm0, %xmm2
    movdqa       %xmm2, (16)(%rsi)
    add          $(32), %rsi
    sub          $(32), %rdx
LL16gas_1:  
    add          $(16), %rdx
    jl           LL8gas_1
    movdqa       (%rsi), %xmm3
    psllw        %xmm7, %xmm3
    pand         %xmm0, %xmm3
    movdqa       %xmm3, (%rsi)
    add          $(16), %rsi
    sub          $(16), %rdx
LL8gas_1:  
    add          $(8), %rdx
    jl           LL4gas_1
    movq         (%rsi), %xmm4
    psllw        %xmm7, %xmm4
    pand         %xmm0, %xmm4
    movq         %xmm4, (%rsi)
    add          $(8), %rsi
    sub          $(8), %rdx
LL4gas_1:  
    add          $(4), %rdx
    jl           LL31gas_1
    movd         (%rsi), %xmm5
    psllw        %xmm7, %xmm5
    pand         %xmm0, %xmm5
    movd         %xmm5, (%rsi)
    add          $(4), %rsi
    sub          $(4), %rdx
LL31gas_1:  
    add          $(4), %rdx
    jle          LExitgas_1
LL3gas_1:  
    movb         (%rsi), %al
    shl          %cl, %al
    movb         %al, (%rsi)
    add          $(1), %rsi
    sub          $(1), %rdx
    jnz          LL3gas_1
LExitgas_1:  
 
 
    pop          %rbx
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownps_LShiftC_8u

 
mfxownps_LShiftC_8u:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    mov          %ecx, %ecx
    mov          %rcx, %rbp
    mov          %esi, %ecx
    mov          %rdx, %rbx
    and          $(15), %rbx
    jz           LRungas_2
    sub          $(16), %rbx
    neg          %rbx
    cmp          %rbx, %rbp
    jl           LRungas_2
    sub          %rbx, %rbp
LNAgas_2:  
    movb         (%rdi), %al
    shl          %cl, %al
    movb         %al, (%rdx)
    add          $(1), %rdi
    add          $(1), %rdx
    sub          $(1), %rbx
    jnz          LNAgas_2
LRungas_2:  
    movd         %esi, %xmm7
    pcmpeqb      %xmm0, %xmm0
    psllw        %xmm7, %xmm0
    psllw        $(8), %xmm0
    movdqa       %xmm0, %xmm6
    psrlw        $(8), %xmm6
    por          %xmm6, %xmm0
    test         $(15), %rdi
    jnz          LNNAgas_2
    sub          $(64), %rbp
    jl           LL32gas_2
LL64gas_2:  
    movdqa       (%rdi), %xmm3
    movdqa       (16)(%rdi), %xmm4
    movdqa       (32)(%rdi), %xmm5
    movdqa       (48)(%rdi), %xmm6
    psllw        %xmm7, %xmm3
    psllw        %xmm7, %xmm4
    psllw        %xmm7, %xmm5
    psllw        %xmm7, %xmm6
    pand         %xmm0, %xmm3
    pand         %xmm0, %xmm4
    pand         %xmm0, %xmm5
    pand         %xmm0, %xmm6
    movdqa       %xmm3, (%rdx)
    movdqa       %xmm4, (16)(%rdx)
    movdqa       %xmm5, (32)(%rdx)
    movdqa       %xmm6, (48)(%rdx)
    add          $(64), %rdi
    add          $(64), %rdx
    sub          $(64), %rbp
    jge          LL64gas_2
LL32gas_2:  
    add          $(32), %rbp
    jl           LL16gas_2
    movdqa       (%rdi), %xmm1
    movdqa       (16)(%rdi), %xmm2
    psllw        %xmm7, %xmm1
    psllw        %xmm7, %xmm2
    pand         %xmm0, %xmm1
    pand         %xmm0, %xmm2
    movdqa       %xmm1, (%rdx)
    movdqa       %xmm2, (16)(%rdx)
    add          $(32), %rdi
    add          $(32), %rdx
    sub          $(32), %rbp
LL16gas_2:  
    add          $(16), %rbp
    jl           LL8gas_2
    movdqa       (%rdi), %xmm3
    psllw        %xmm7, %xmm3
    pand         %xmm0, %xmm3
    movdqa       %xmm3, (%rdx)
    add          $(16), %rdi
    add          $(16), %rdx
    sub          $(16), %rbp
    jmp          LL8gas_2
LNNAgas_2:  
    sub          $(64), %rbp
    jl           LL32ngas_2
LL64ngas_2:  
    lddqu        (%rdi), %xmm3
    lddqu        (16)(%rdi), %xmm4
    lddqu        (32)(%rdi), %xmm5
    lddqu        (48)(%rdi), %xmm6
    psllw        %xmm7, %xmm3
    psllw        %xmm7, %xmm4
    psllw        %xmm7, %xmm5
    psllw        %xmm7, %xmm6
    pand         %xmm0, %xmm3
    pand         %xmm0, %xmm4
    pand         %xmm0, %xmm5
    pand         %xmm0, %xmm6
    movdqa       %xmm3, (%rdx)
    movdqa       %xmm4, (16)(%rdx)
    movdqa       %xmm5, (32)(%rdx)
    movdqa       %xmm6, (48)(%rdx)
    add          $(64), %rdi
    add          $(64), %rdx
    sub          $(64), %rbp
    jge          LL64ngas_2
LL32ngas_2:  
    add          $(32), %rbp
    jl           LL16ngas_2
    lddqu        (%rdi), %xmm1
    lddqu        (16)(%rdi), %xmm2
    psllw        %xmm7, %xmm1
    psllw        %xmm7, %xmm2
    pand         %xmm0, %xmm1
    pand         %xmm0, %xmm2
    movdqa       %xmm1, (%rdx)
    movdqa       %xmm2, (16)(%rdx)
    add          $(32), %rdi
    add          $(32), %rdx
    sub          $(32), %rbp
LL16ngas_2:  
    add          $(16), %rbp
    jl           LL8gas_2
    lddqu        (%rdi), %xmm3
    psllw        %xmm7, %xmm3
    pand         %xmm0, %xmm3
    movdqa       %xmm3, (%rdx)
    add          $(16), %rdi
    add          $(16), %rdx
    sub          $(16), %rbp
LL8gas_2:  
    add          $(8), %rbp
    jl           LL4gas_2
    movq         (%rdi), %xmm4
    psllw        %xmm7, %xmm4
    pand         %xmm0, %xmm4
    movq         %xmm4, (%rdx)
    add          $(8), %rdi
    add          $(8), %rdx
    sub          $(8), %rbp
LL4gas_2:  
    add          $(4), %rbp
    jl           LL31gas_2
    movd         (%rdi), %xmm5
    psllw        %xmm7, %xmm5
    pand         %xmm0, %xmm5
    movd         %xmm5, (%rdx)
    add          $(4), %rdi
    add          $(4), %rdx
    sub          $(4), %rbp
LL31gas_2:  
    add          $(4), %rbp
    jle          LExitgas_2
LL3gas_2:  
    movb         (%rdi), %al
    shl          %cl, %al
    movb         %al, (%rdx)
    add          $(1), %rdi
    add          $(1), %rdx
    sub          $(1), %rbp
    jnz          LL3gas_2
LExitgas_2:  
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownps_LShiftC_16u_I

 
mfxownps_LShiftC_16u_I:
 
 
    push         %rbx
 
 
    mov          %edx, %edx
    mov          %edi, %ecx
    movd         %edi, %xmm7
    test         $(1), %rsi
    jz           LRun0gas_3
    sub          $(4), %rdx
    jl           LL2gas_3
LL4ngas_3:  
    movq         (%rsi), %xmm4
    psllw        %xmm7, %xmm4
    movq         %xmm4, (%rsi)
    add          $(8), %rsi
    sub          $(4), %rdx
    jge          LL4ngas_3
    jmp          LL2gas_3
LRun0gas_3:  
    mov          %rsi, %rbx
    and          $(15), %rbx
    jz           LRungas_3
    sub          $(16), %rbx
    neg          %rbx
    shr          $(1), %rbx
    cmp          %rbx, %rdx
    jl           LRungas_3
    sub          %rbx, %rdx
LNAgas_3:  
    movw         (%rsi), %ax
    shl          %cl, %ax
    movw         %ax, (%rsi)
    add          $(2), %rsi
    sub          $(1), %rbx
    jnz          LNAgas_3
LRungas_3:  
    sub          $(32), %rdx
    jl           LL16gas_3
LL32gas_3:  
    movdqa       (%rsi), %xmm3
    psllw        %xmm7, %xmm3
    movdqa       %xmm3, (%rsi)
    movdqa       (16)(%rsi), %xmm4
    psllw        %xmm7, %xmm4
    movdqa       %xmm4, (16)(%rsi)
    movdqa       (32)(%rsi), %xmm5
    psllw        %xmm7, %xmm5
    movdqa       %xmm5, (32)(%rsi)
    movdqa       (48)(%rsi), %xmm6
    psllw        %xmm7, %xmm6
    movdqa       %xmm6, (48)(%rsi)
    add          $(64), %rsi
    sub          $(32), %rdx
    jge          LL32gas_3
LL16gas_3:  
    add          $(16), %rdx
    jl           LL8gas_3
    movdqa       (%rsi), %xmm1
    psllw        %xmm7, %xmm1
    movdqa       %xmm1, (%rsi)
    movdqa       (16)(%rsi), %xmm2
    psllw        %xmm7, %xmm2
    movdqa       %xmm2, (16)(%rsi)
    add          $(32), %rsi
    sub          $(16), %rdx
LL8gas_3:  
    add          $(8), %rdx
    jl           LL4gas_3
    movdqa       (%rsi), %xmm3
    psllw        %xmm7, %xmm3
    movdqa       %xmm3, (%rsi)
    add          $(16), %rsi
    sub          $(8), %rdx
LL4gas_3:  
    add          $(4), %rdx
    jl           LL2gas_3
    movq         (%rsi), %xmm4
    psllw        %xmm7, %xmm4
    movq         %xmm4, (%rsi)
    add          $(8), %rsi
    sub          $(4), %rdx
LL2gas_3:  
    add          $(2), %rdx
    jl           LL1gas_3
    movd         (%rsi), %xmm5
    psllw        %xmm7, %xmm5
    movd         %xmm5, (%rsi)
    add          $(4), %rsi
    sub          $(2), %rdx
LL1gas_3:  
    add          $(1), %rdx
    jl           LExitgas_3
    movw         (%rsi), %ax
    shl          %cl, %ax
    movw         %ax, (%rsi)
LExitgas_3:  
 
 
    pop          %rbx
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownps_LShiftC_16u

 
mfxownps_LShiftC_16u:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    mov          %ecx, %ecx
    mov          %rcx, %rbp
    mov          %esi, %ecx
    movd         %ecx, %xmm7
    test         $(1), %rdx
    jz           LRun0gas_4
    sub          $(4), %rbp
    jl           LL2gas_4
LL4ngas_4:  
    movq         (%rdi), %xmm4
    psllw        %xmm7, %xmm4
    movq         %xmm4, (%rdx)
    add          $(8), %rdi
    add          $(8), %rdx
    sub          $(4), %rbp
    jge          LL4ngas_4
    jmp          LL2gas_4
LRun0gas_4:  
    mov          %rdx, %rbx
    and          $(15), %rbx
    jz           LRungas_4
    sub          $(16), %rbx
    neg          %rbx
    shr          $(1), %rbx
    cmp          %rbx, %rbp
    jl           LRungas_4
    sub          %rbx, %rbp
LNAgas_4:  
    movw         (%rdi), %ax
    shl          %cl, %ax
    movw         %ax, (%rdx)
    add          $(2), %rdi
    add          $(2), %rdx
    sub          $(1), %rbx
    jnz          LNAgas_4
LRungas_4:  
    test         $(15), %rdi
    jnz          LNNAgas_4
    sub          $(32), %rbp
    jl           LL16gas_4
LL32gas_4:  
    movdqa       (%rdi), %xmm3
    movdqa       (16)(%rdi), %xmm4
    movdqa       (32)(%rdi), %xmm5
    movdqa       (48)(%rdi), %xmm6
    psllw        %xmm7, %xmm3
    psllw        %xmm7, %xmm4
    psllw        %xmm7, %xmm5
    psllw        %xmm7, %xmm6
    movdqa       %xmm3, (%rdx)
    movdqa       %xmm4, (16)(%rdx)
    movdqa       %xmm5, (32)(%rdx)
    movdqa       %xmm6, (48)(%rdx)
    add          $(64), %rdi
    add          $(64), %rdx
    sub          $(32), %rbp
    jge          LL32gas_4
LL16gas_4:  
    add          $(16), %rbp
    jl           LL8gas_4
    movdqa       (%rdi), %xmm1
    movdqa       (16)(%rdi), %xmm2
    psllw        %xmm7, %xmm1
    psllw        %xmm7, %xmm2
    movdqa       %xmm1, (%rdx)
    movdqa       %xmm2, (16)(%rdx)
    add          $(32), %rdi
    add          $(32), %rdx
    sub          $(16), %rbp
LL8gas_4:  
    add          $(8), %rbp
    jl           LL4gas_4
    movdqa       (%rdi), %xmm3
    psllw        %xmm7, %xmm3
    movdqa       %xmm3, (%rdx)
    add          $(16), %rdi
    add          $(16), %rdx
    sub          $(8), %rbp
    jmp          LL4gas_4
LNNAgas_4:  
    sub          $(32), %rbp
    jl           LL16ngas_4
LL32ngas_4:  
    lddqu        (%rdi), %xmm3
    lddqu        (16)(%rdi), %xmm4
    lddqu        (32)(%rdi), %xmm5
    lddqu        (48)(%rdi), %xmm6
    psllw        %xmm7, %xmm3
    psllw        %xmm7, %xmm4
    psllw        %xmm7, %xmm5
    psllw        %xmm7, %xmm6
    movdqa       %xmm3, (%rdx)
    movdqa       %xmm4, (16)(%rdx)
    movdqa       %xmm5, (32)(%rdx)
    movdqa       %xmm6, (48)(%rdx)
    add          $(64), %rdi
    add          $(64), %rdx
    sub          $(32), %rbp
    jge          LL32ngas_4
LL16ngas_4:  
    add          $(16), %rbp
    jl           LL8ngas_4
    lddqu        (%rdi), %xmm1
    lddqu        (16)(%rdi), %xmm2
    psllw        %xmm7, %xmm1
    psllw        %xmm7, %xmm2
    movdqa       %xmm1, (%rdx)
    movdqa       %xmm2, (16)(%rdx)
    add          $(32), %rdi
    add          $(32), %rdx
    sub          $(16), %rbp
LL8ngas_4:  
    add          $(8), %rbp
    jl           LL4gas_4
    lddqu        (%rdi), %xmm3
    psllw        %xmm7, %xmm3
    movdqa       %xmm3, (%rdx)
    add          $(16), %rdi
    add          $(16), %rdx
    sub          $(8), %rbp
LL4gas_4:  
    add          $(4), %rbp
    jl           LL2gas_4
    movq         (%rdi), %xmm4
    psllw        %xmm7, %xmm4
    movq         %xmm4, (%rdx)
    add          $(8), %rdi
    add          $(8), %rdx
    sub          $(4), %rbp
LL2gas_4:  
    add          $(2), %rbp
    jl           LL1gas_4
    movd         (%rdi), %xmm5
    psllw        %xmm7, %xmm5
    movd         %xmm5, (%rdx)
    add          $(4), %rdi
    add          $(4), %rdx
    sub          $(2), %rbp
LL1gas_4:  
    add          $(1), %rbp
    jl           LExitgas_4
    movw         (%rdi), %ax
    shl          %cl, %ax
    movw         %ax, (%rdx)
LExitgas_4:  
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownps_LShiftC_32s_I

 
mfxownps_LShiftC_32s_I:
 
 
    push         %rbx
 
 
    mov          %edx, %edx
    movd         %edi, %xmm7
    test         $(3), %rsi
    jz           LRun0gas_5
    sub          $(4), %rdx
    jl           LL2gas_5
LL4ngas_5:  
    movq         (%rsi), %xmm0
    pslld        %xmm7, %xmm0
    movq         %xmm0, (%rsi)
    add          $(8), %rsi
    sub          $(2), %rdx
    jge          LL4ngas_5
    jmp          LL2gas_5
LRun0gas_5:  
    mov          %rsi, %rbx
    and          $(15), %rbx
    jz           LRungas_5
    sub          $(16), %rbx
    neg          %rbx
    shr          $(2), %rbx
    cmp          %rbx, %rdx
    jl           LRungas_5
    sub          %rbx, %rdx
LNAgas_5:  
    movd         (%rsi), %xmm0
    pslld        %xmm7, %xmm0
    movd         %xmm0, (%rsi)
    add          $(4), %rsi
    sub          $(1), %rbx
    jnz          LNAgas_5
LRungas_5:  
    sub          $(16), %rdx
    jl           LL8gas_5
LL16gas_5:  
    movdqa       (%rsi), %xmm3
    pslld        %xmm7, %xmm3
    movdqa       %xmm3, (%rsi)
    movdqa       (16)(%rsi), %xmm4
    pslld        %xmm7, %xmm4
    movdqa       %xmm4, (16)(%rsi)
    movdqa       (32)(%rsi), %xmm5
    pslld        %xmm7, %xmm5
    movdqa       %xmm5, (32)(%rsi)
    movdqa       (48)(%rsi), %xmm6
    pslld        %xmm7, %xmm6
    movdqa       %xmm6, (48)(%rsi)
    add          $(64), %rsi
    sub          $(16), %rdx
    jge          LL16gas_5
LL8gas_5:  
    add          $(8), %rdx
    jl           LL4gas_5
    movdqa       (%rsi), %xmm1
    pslld        %xmm7, %xmm1
    movdqa       %xmm1, (%rsi)
    movdqa       (16)(%rsi), %xmm2
    pslld        %xmm7, %xmm2
    movdqa       %xmm2, (16)(%rsi)
    add          $(32), %rsi
    sub          $(8), %rdx
LL4gas_5:  
    add          $(4), %rdx
    jl           LL2gas_5
    movdqa       (%rsi), %xmm3
    pslld        %xmm7, %xmm3
    movdqa       %xmm3, (%rsi)
    add          $(16), %rsi
    sub          $(4), %rdx
LL2gas_5:  
    add          $(2), %rdx
    jl           LL1gas_5
    movq         (%rsi), %xmm4
    pslld        %xmm7, %xmm4
    movq         %xmm4, (%rsi)
    add          $(8), %rsi
    sub          $(2), %rdx
LL1gas_5:  
    add          $(1), %rdx
    jl           LExitgas_5
    movd         (%rsi), %xmm5
    pslld        %xmm7, %xmm5
    movd         %xmm5, (%rsi)
LExitgas_5:  
 
 
    pop          %rbx
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownps_LShiftC_32s

 
mfxownps_LShiftC_32s:
 
 
    push         %rbx
 
 
    mov          %ecx, %ecx
    movd         %esi, %xmm7
    test         $(3), %rdx
    jz           LRun0gas_6
    sub          $(2), %rcx
    jl           LL1gas_6
LL2ngas_6:  
    movq         (%rdi), %xmm4
    pslld        %xmm7, %xmm4
    movq         %xmm4, (%rdx)
    add          $(8), %rdi
    add          $(8), %rdx
    sub          $(2), %rcx
    jge          LL2ngas_6
    jmp          LL1gas_6
LRun0gas_6:  
    mov          %rdx, %rbx
    and          $(15), %rbx
    jz           LRungas_6
    sub          $(16), %rbx
    neg          %rbx
    shr          $(2), %rbx
    cmp          %rbx, %rcx
    jl           LRungas_6
    sub          %rbx, %rcx
LNAgas_6:  
    movd         (%rdi), %xmm0
    pslld        %xmm7, %xmm0
    movd         %xmm0, (%rdx)
    add          $(4), %rdi
    add          $(4), %rdx
    sub          $(1), %rbx
    jnz          LNAgas_6
LRungas_6:  
    test         $(15), %rdi
    jnz          LNNAgas_6
    sub          $(16), %rcx
    jl           LL8gas_6
LL16gas_6:  
    movdqa       (%rdi), %xmm3
    movdqa       (16)(%rdi), %xmm4
    movdqa       (32)(%rdi), %xmm5
    movdqa       (48)(%rdi), %xmm6
    pslld        %xmm7, %xmm3
    pslld        %xmm7, %xmm4
    pslld        %xmm7, %xmm5
    pslld        %xmm7, %xmm6
    movdqa       %xmm3, (%rdx)
    movdqa       %xmm4, (16)(%rdx)
    movdqa       %xmm5, (32)(%rdx)
    movdqa       %xmm6, (48)(%rdx)
    add          $(64), %rdi
    add          $(64), %rdx
    sub          $(16), %rcx
    jge          LL16gas_6
LL8gas_6:  
    add          $(8), %rcx
    jl           LL4gas_6
    movdqa       (%rdi), %xmm1
    movdqa       (16)(%rdi), %xmm2
    pslld        %xmm7, %xmm1
    pslld        %xmm7, %xmm2
    movdqa       %xmm1, (%rdx)
    movdqa       %xmm2, (16)(%rdx)
    add          $(32), %rdi
    add          $(32), %rdx
    sub          $(8), %rcx
LL4gas_6:  
    add          $(4), %rcx
    jl           LL2gas_6
    movdqa       (%rdi), %xmm3
    pslld        %xmm7, %xmm3
    movdqa       %xmm3, (%rdx)
    add          $(16), %rdi
    add          $(16), %rdx
    sub          $(4), %rcx
    jmp          LL2gas_6
LNNAgas_6:  
    sub          $(16), %rcx
    jl           LL8ngas_6
LL16ngas_6:  
    lddqu        (%rdi), %xmm3
    lddqu        (16)(%rdi), %xmm4
    lddqu        (32)(%rdi), %xmm5
    lddqu        (48)(%rdi), %xmm6
    pslld        %xmm7, %xmm3
    pslld        %xmm7, %xmm4
    pslld        %xmm7, %xmm5
    pslld        %xmm7, %xmm6
    movdqa       %xmm3, (%rdx)
    movdqa       %xmm4, (16)(%rdx)
    movdqa       %xmm5, (32)(%rdx)
    movdqa       %xmm6, (48)(%rdx)
    add          $(64), %rdi
    add          $(64), %rdx
    sub          $(16), %rcx
    jge          LL16ngas_6
LL8ngas_6:  
    add          $(8), %rcx
    jl           LL4ngas_6
    lddqu        (%rdi), %xmm1
    lddqu        (16)(%rdi), %xmm2
    pslld        %xmm7, %xmm1
    pslld        %xmm7, %xmm2
    movdqa       %xmm1, (%rdx)
    movdqa       %xmm2, (16)(%rdx)
    add          $(32), %rdi
    add          $(32), %rdx
    sub          $(8), %rcx
LL4ngas_6:  
    add          $(4), %rcx
    jl           LL2gas_6
    lddqu        (%rdi), %xmm3
    pslld        %xmm7, %xmm3
    movdqa       %xmm3, (%rdx)
    add          $(16), %rdi
    add          $(16), %rdx
    sub          $(4), %rcx
LL2gas_6:  
    add          $(2), %rcx
    jl           LL1gas_6
    movq         (%rdi), %xmm4
    pslld        %xmm7, %xmm4
    movq         %xmm4, (%rdx)
    add          $(8), %rdi
    add          $(8), %rdx
    sub          $(2), %rcx
LL1gas_6:  
    add          $(1), %rcx
    jl           LExitgas_6
    movd         (%rdi), %xmm5
    pslld        %xmm7, %xmm5
    movd         %xmm5, (%rdx)
LExitgas_6:  
 
 
    pop          %rbx
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownps_RShiftC_8u_I

 
mfxownps_RShiftC_8u_I:
 
 
    push         %rbx
 
 
    mov          %edx, %edx
    mov          %edi, %ecx
    mov          %rsi, %rbx
    and          $(15), %rbx
    jz           LRungas_7
    sub          $(16), %rbx
    neg          %rbx
    cmp          %rbx, %rdx
    jl           LRungas_7
    sub          %rbx, %rdx
LNAgas_7:  
    movb         (%rsi), %al
    shr          %cl, %al
    movb         %al, (%rsi)
    add          $(1), %rsi
    sub          $(1), %rbx
    jnz          LNAgas_7
LRungas_7:  
    movd         %edi, %xmm7
    pcmpeqb      %xmm0, %xmm0
    psrlw        %xmm7, %xmm0
    psrlw        $(8), %xmm0
    packuswb     %xmm0, %xmm0
    sub          $(64), %rdx
    jl           LL32gas_7
LL64gas_7:  
    movdqa       (%rsi), %xmm3
    psrlw        %xmm7, %xmm3
    pand         %xmm0, %xmm3
    movdqa       %xmm3, (%rsi)
    movdqa       (16)(%rsi), %xmm4
    psrlw        %xmm7, %xmm4
    pand         %xmm0, %xmm4
    movdqa       %xmm4, (16)(%rsi)
    movdqa       (32)(%rsi), %xmm5
    psrlw        %xmm7, %xmm5
    pand         %xmm0, %xmm5
    movdqa       %xmm5, (32)(%rsi)
    movdqa       (48)(%rsi), %xmm6
    psrlw        %xmm7, %xmm6
    pand         %xmm0, %xmm6
    movdqa       %xmm6, (48)(%rsi)
    add          $(64), %rsi
    sub          $(64), %rdx
    jge          LL64gas_7
LL32gas_7:  
    add          $(32), %rdx
    jl           LL16gas_7
    movdqa       (%rsi), %xmm1
    psrlw        %xmm7, %xmm1
    pand         %xmm0, %xmm1
    movdqa       %xmm1, (%rsi)
    movdqa       (16)(%rsi), %xmm2
    psrlw        %xmm7, %xmm2
    pand         %xmm0, %xmm2
    movdqa       %xmm2, (16)(%rsi)
    add          $(32), %rsi
    sub          $(32), %rdx
LL16gas_7:  
    add          $(16), %rdx
    jl           LL8gas_7
    movdqa       (%rsi), %xmm3
    psrlw        %xmm7, %xmm3
    pand         %xmm0, %xmm3
    movdqa       %xmm3, (%rsi)
    add          $(16), %rsi
    sub          $(16), %rdx
LL8gas_7:  
    add          $(8), %rdx
    jl           LL4gas_7
    movq         (%rsi), %xmm4
    psrlw        %xmm7, %xmm4
    pand         %xmm0, %xmm4
    movq         %xmm4, (%rsi)
    add          $(8), %rsi
    sub          $(8), %rdx
LL4gas_7:  
    add          $(4), %rdx
    jl           LL31gas_7
    movd         (%rsi), %xmm5
    psrlw        %xmm7, %xmm5
    pand         %xmm0, %xmm5
    movd         %xmm5, (%rsi)
    add          $(4), %rsi
    sub          $(4), %rdx
LL31gas_7:  
    add          $(4), %rdx
    jle          LExitgas_7
LL3gas_7:  
    movb         (%rsi), %al
    shr          %cl, %al
    movb         %al, (%rsi)
    add          $(1), %rsi
    sub          $(1), %rdx
    jnz          LL3gas_7
LExitgas_7:  
 
 
    pop          %rbx
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownps_RShiftC_8u

 
mfxownps_RShiftC_8u:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    mov          %ecx, %ecx
    mov          %rcx, %rbp
    mov          %esi, %ecx
    mov          %rdx, %rbx
    and          $(15), %rbx
    jz           LRungas_8
    sub          $(16), %rbx
    neg          %rbx
    cmp          %rbx, %rbp
    jl           LRungas_8
    sub          %rbx, %rbp
LNAgas_8:  
    movb         (%rdi), %al
    shr          %cl, %al
    movb         %al, (%rdx)
    add          $(1), %rdi
    add          $(1), %rdx
    sub          $(1), %rbx
    jnz          LNAgas_8
LRungas_8:  
    movd         %esi, %xmm7
    pcmpeqb      %xmm0, %xmm0
    psrlw        %xmm7, %xmm0
    psrlw        $(8), %xmm0
    packuswb     %xmm0, %xmm0
    test         $(15), %rdi
    jnz          LNNAgas_8
    sub          $(64), %rbp
    jl           LL32gas_8
LL64gas_8:  
    movdqa       (%rdi), %xmm3
    movdqa       (16)(%rdi), %xmm4
    movdqa       (32)(%rdi), %xmm5
    movdqa       (48)(%rdi), %xmm6
    psrlw        %xmm7, %xmm3
    psrlw        %xmm7, %xmm4
    psrlw        %xmm7, %xmm5
    psrlw        %xmm7, %xmm6
    pand         %xmm0, %xmm3
    pand         %xmm0, %xmm4
    pand         %xmm0, %xmm5
    pand         %xmm0, %xmm6
    movdqa       %xmm3, (%rdx)
    movdqa       %xmm4, (16)(%rdx)
    movdqa       %xmm5, (32)(%rdx)
    movdqa       %xmm6, (48)(%rdx)
    add          $(64), %rdi
    add          $(64), %rdx
    sub          $(64), %rbp
    jge          LL64gas_8
LL32gas_8:  
    add          $(32), %rbp
    jl           LL16gas_8
    movdqa       (%rdi), %xmm1
    movdqa       (16)(%rdi), %xmm2
    psrlw        %xmm7, %xmm1
    psrlw        %xmm7, %xmm2
    pand         %xmm0, %xmm1
    pand         %xmm0, %xmm2
    movdqa       %xmm1, (%rdx)
    movdqa       %xmm2, (16)(%rdx)
    add          $(32), %rdi
    add          $(32), %rdx
    sub          $(32), %rbp
LL16gas_8:  
    add          $(16), %rbp
    jl           LL8gas_8
    movdqa       (%rdi), %xmm3
    psrlw        %xmm7, %xmm3
    pand         %xmm0, %xmm3
    movdqa       %xmm3, (%rdx)
    add          $(16), %rdi
    add          $(16), %rdx
    sub          $(16), %rbp
    jmp          LL8gas_8
LNNAgas_8:  
    sub          $(64), %rbp
    jl           LL32ngas_8
LL64ngas_8:  
    lddqu        (%rdi), %xmm3
    lddqu        (16)(%rdi), %xmm4
    lddqu        (32)(%rdi), %xmm5
    lddqu        (48)(%rdi), %xmm6
    psrlw        %xmm7, %xmm3
    psrlw        %xmm7, %xmm4
    psrlw        %xmm7, %xmm5
    psrlw        %xmm7, %xmm6
    pand         %xmm0, %xmm3
    pand         %xmm0, %xmm4
    pand         %xmm0, %xmm5
    pand         %xmm0, %xmm6
    movdqa       %xmm3, (%rdx)
    movdqa       %xmm4, (16)(%rdx)
    movdqa       %xmm5, (32)(%rdx)
    movdqa       %xmm6, (48)(%rdx)
    add          $(64), %rdi
    add          $(64), %rdx
    sub          $(64), %rbp
    jge          LL64ngas_8
LL32ngas_8:  
    add          $(32), %rbp
    jl           LL16ngas_8
    lddqu        (%rdi), %xmm1
    lddqu        (16)(%rdi), %xmm2
    psrlw        %xmm7, %xmm1
    psrlw        %xmm7, %xmm2
    pand         %xmm0, %xmm1
    pand         %xmm0, %xmm2
    movdqa       %xmm1, (%rdx)
    movdqa       %xmm2, (16)(%rdx)
    add          $(32), %rdi
    add          $(32), %rdx
    sub          $(32), %rbp
LL16ngas_8:  
    add          $(16), %rbp
    jl           LL8gas_8
    lddqu        (%rdi), %xmm3
    psrlw        %xmm7, %xmm3
    pand         %xmm0, %xmm3
    movdqa       %xmm3, (%rdx)
    add          $(16), %rdi
    add          $(16), %rdx
    sub          $(16), %rbp
LL8gas_8:  
    add          $(8), %rbp
    jl           LL4gas_8
    movq         (%rdi), %xmm4
    psrlw        %xmm7, %xmm4
    pand         %xmm0, %xmm4
    movq         %xmm4, (%rdx)
    add          $(8), %rdi
    add          $(8), %rdx
    sub          $(8), %rbp
LL4gas_8:  
    add          $(4), %rbp
    jl           LL31gas_8
    movd         (%rdi), %xmm5
    psrlw        %xmm7, %xmm5
    pand         %xmm0, %xmm5
    movd         %xmm5, (%rdx)
    add          $(4), %rdi
    add          $(4), %rdx
    sub          $(4), %rbp
LL31gas_8:  
    add          $(4), %rbp
    jle          LExitgas_8
LL3gas_8:  
    movb         (%rdi), %al
    shr          %cl, %al
    movb         %al, (%rdx)
    add          $(1), %rdi
    add          $(1), %rdx
    sub          $(1), %rbp
    jnz          LL3gas_8
LExitgas_8:  
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownps_RShiftC_16u_I

 
mfxownps_RShiftC_16u_I:
 
 
    push         %rbx
 
 
    mov          %edx, %edx
    mov          %edi, %ecx
    movd         %edi, %xmm7
    test         $(1), %rsi
    jz           LRun0gas_9
    sub          $(4), %rdx
    jl           LL2gas_9
LL4ngas_9:  
    movq         (%rsi), %xmm4
    psrlw        %xmm7, %xmm4
    movq         %xmm4, (%rsi)
    add          $(8), %rsi
    sub          $(4), %rdx
    jge          LL4ngas_9
    jmp          LL2gas_9
LRun0gas_9:  
    mov          %rsi, %rbx
    and          $(15), %rbx
    jz           LRungas_9
    sub          $(16), %rbx
    neg          %rbx
    shr          $(1), %rbx
    cmp          %rbx, %rdx
    jl           LRungas_9
    sub          %rbx, %rdx
LNAgas_9:  
    movw         (%rsi), %ax
    shr          %cl, %ax
    movw         %ax, (%rsi)
    add          $(2), %rsi
    sub          $(1), %rbx
    jnz          LNAgas_9
LRungas_9:  
    sub          $(32), %rdx
    jl           LL16gas_9
LL32gas_9:  
    movdqa       (%rsi), %xmm3
    psrlw        %xmm7, %xmm3
    movdqa       %xmm3, (%rsi)
    movdqa       (16)(%rsi), %xmm4
    psrlw        %xmm7, %xmm4
    movdqa       %xmm4, (16)(%rsi)
    movdqa       (32)(%rsi), %xmm5
    psrlw        %xmm7, %xmm5
    movdqa       %xmm5, (32)(%rsi)
    movdqa       (48)(%rsi), %xmm6
    psrlw        %xmm7, %xmm6
    movdqa       %xmm6, (48)(%rsi)
    add          $(64), %rsi
    sub          $(32), %rdx
    jge          LL32gas_9
LL16gas_9:  
    add          $(16), %rdx
    jl           LL8gas_9
    movdqa       (%rsi), %xmm1
    psrlw        %xmm7, %xmm1
    movdqa       %xmm1, (%rsi)
    movdqa       (16)(%rsi), %xmm2
    psrlw        %xmm7, %xmm2
    movdqa       %xmm2, (16)(%rsi)
    add          $(32), %rsi
    sub          $(16), %rdx
LL8gas_9:  
    add          $(8), %rdx
    jl           LL4gas_9
    movdqa       (%rsi), %xmm3
    psrlw        %xmm7, %xmm3
    movdqa       %xmm3, (%rsi)
    add          $(16), %rsi
    sub          $(8), %rdx
LL4gas_9:  
    add          $(4), %rdx
    jl           LL2gas_9
    movq         (%rsi), %xmm4
    psrlw        %xmm7, %xmm4
    movq         %xmm4, (%rsi)
    add          $(8), %rsi
    sub          $(4), %rdx
LL2gas_9:  
    add          $(2), %rdx
    jl           LL1gas_9
    movd         (%rsi), %xmm5
    psrlw        %xmm7, %xmm5
    movd         %xmm5, (%rsi)
    add          $(4), %rsi
    sub          $(2), %rdx
LL1gas_9:  
    add          $(1), %rdx
    jl           LExitgas_9
    movw         (%rsi), %ax
    shr          %cl, %ax
    movw         %ax, (%rsi)
LExitgas_9:  
 
 
    pop          %rbx
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownps_RShiftC_16u

 
mfxownps_RShiftC_16u:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    mov          %ecx, %ecx
    mov          %rcx, %rbp
    mov          %esi, %ecx
    movd         %ecx, %xmm7
    test         $(1), %rdx
    jz           LRun0gas_10
    sub          $(4), %rbp
    jl           LL2gas_10
LL4ngas_10:  
    movq         (%rdi), %xmm4
    psrlw        %xmm7, %xmm4
    movq         %xmm4, (%rdx)
    add          $(8), %rdi
    add          $(8), %rdx
    sub          $(4), %rbp
    jge          LL4ngas_10
    jmp          LL2gas_10
LRun0gas_10:  
    mov          %rdx, %rbx
    and          $(15), %rbx
    jz           LRungas_10
    sub          $(16), %rbx
    neg          %rbx
    shr          $(1), %rbx
    cmp          %rbx, %rbp
    jl           LRungas_10
    sub          %rbx, %rbp
LNAgas_10:  
    movw         (%rdi), %ax
    shr          %cl, %ax
    movw         %ax, (%rdx)
    add          $(2), %rdi
    add          $(2), %rdx
    sub          $(1), %rbx
    jnz          LNAgas_10
LRungas_10:  
    test         $(15), %rdi
    jnz          LNNAgas_10
    sub          $(32), %rbp
    jl           LL16gas_10
LL32gas_10:  
    movdqa       (%rdi), %xmm3
    movdqa       (16)(%rdi), %xmm4
    movdqa       (32)(%rdi), %xmm5
    movdqa       (48)(%rdi), %xmm6
    psrlw        %xmm7, %xmm3
    psrlw        %xmm7, %xmm4
    psrlw        %xmm7, %xmm5
    psrlw        %xmm7, %xmm6
    movdqa       %xmm3, (%rdx)
    movdqa       %xmm4, (16)(%rdx)
    movdqa       %xmm5, (32)(%rdx)
    movdqa       %xmm6, (48)(%rdx)
    add          $(64), %rdi
    add          $(64), %rdx
    sub          $(32), %rbp
    jge          LL32gas_10
LL16gas_10:  
    add          $(16), %rbp
    jl           LL8gas_10
    movdqa       (%rdi), %xmm1
    movdqa       (16)(%rdi), %xmm2
    psrlw        %xmm7, %xmm1
    psrlw        %xmm7, %xmm2
    movdqa       %xmm1, (%rdx)
    movdqa       %xmm2, (16)(%rdx)
    add          $(32), %rdi
    add          $(32), %rdx
    sub          $(16), %rbp
LL8gas_10:  
    add          $(8), %rbp
    jl           LL4gas_10
    movdqa       (%rdi), %xmm3
    psrlw        %xmm7, %xmm3
    movdqa       %xmm3, (%rdx)
    add          $(16), %rdi
    add          $(16), %rdx
    sub          $(8), %rbp
    jmp          LL4gas_10
LNNAgas_10:  
    sub          $(32), %rbp
    jl           LL16ngas_10
LL32ngas_10:  
    lddqu        (%rdi), %xmm3
    lddqu        (16)(%rdi), %xmm4
    lddqu        (32)(%rdi), %xmm5
    lddqu        (48)(%rdi), %xmm6
    psrlw        %xmm7, %xmm3
    psrlw        %xmm7, %xmm4
    psrlw        %xmm7, %xmm5
    psrlw        %xmm7, %xmm6
    movdqa       %xmm3, (%rdx)
    movdqa       %xmm4, (16)(%rdx)
    movdqa       %xmm5, (32)(%rdx)
    movdqa       %xmm6, (48)(%rdx)
    add          $(64), %rdi
    add          $(64), %rdx
    sub          $(32), %rbp
    jge          LL32ngas_10
LL16ngas_10:  
    add          $(16), %rbp
    jl           LL8ngas_10
    lddqu        (%rdi), %xmm1
    lddqu        (16)(%rdi), %xmm2
    psrlw        %xmm7, %xmm1
    psrlw        %xmm7, %xmm2
    movdqa       %xmm1, (%rdx)
    movdqa       %xmm2, (16)(%rdx)
    add          $(32), %rdi
    add          $(32), %rdx
    sub          $(16), %rbp
LL8ngas_10:  
    add          $(8), %rbp
    jl           LL4gas_10
    lddqu        (%rdi), %xmm3
    psrlw        %xmm7, %xmm3
    movdqa       %xmm3, (%rdx)
    add          $(16), %rdi
    add          $(16), %rdx
    sub          $(8), %rbp
LL4gas_10:  
    add          $(4), %rbp
    jl           LL2gas_10
    movq         (%rdi), %xmm4
    psrlw        %xmm7, %xmm4
    movq         %xmm4, (%rdx)
    add          $(8), %rdi
    add          $(8), %rdx
    sub          $(4), %rbp
LL2gas_10:  
    add          $(2), %rbp
    jl           LL1gas_10
    movd         (%rdi), %xmm5
    psrlw        %xmm7, %xmm5
    movd         %xmm5, (%rdx)
    add          $(4), %rdi
    add          $(4), %rdx
    sub          $(2), %rbp
LL1gas_10:  
    add          $(1), %rbp
    jl           LExitgas_10
    movw         (%rdi), %ax
    shr          %cl, %ax
    movw         %ax, (%rdx)
LExitgas_10:  
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownps_RShiftC_16s_I

 
mfxownps_RShiftC_16s_I:
 
 
    push         %rbx
 
 
    mov          %edx, %edx
    mov          %edi, %ecx
    movd         %edi, %xmm7
    test         $(1), %rsi
    jz           LRun0gas_11
    sub          $(4), %rdx
    jl           LL2gas_11
LL4ngas_11:  
    movq         (%rsi), %xmm4
    psraw        %xmm7, %xmm4
    movq         %xmm4, (%rsi)
    add          $(8), %rsi
    sub          $(4), %rdx
    jge          LL4ngas_11
    jmp          LL2gas_11
LRun0gas_11:  
    mov          %rsi, %rbx
    and          $(15), %rbx
    jz           LRungas_11
    sub          $(16), %rbx
    neg          %rbx
    shr          $(1), %rbx
    cmp          %rbx, %rdx
    jl           LRungas_11
    sub          %rbx, %rdx
LNAgas_11:  
    movw         (%rsi), %ax
    sar          %cl, %ax
    movw         %ax, (%rsi)
    add          $(2), %rsi
    sub          $(1), %rbx
    jnz          LNAgas_11
LRungas_11:  
    sub          $(32), %rdx
    jl           LL16gas_11
LL32gas_11:  
    movdqa       (%rsi), %xmm3
    psraw        %xmm7, %xmm3
    movdqa       %xmm3, (%rsi)
    movdqa       (16)(%rsi), %xmm4
    psraw        %xmm7, %xmm4
    movdqa       %xmm4, (16)(%rsi)
    movdqa       (32)(%rsi), %xmm5
    psraw        %xmm7, %xmm5
    movdqa       %xmm5, (32)(%rsi)
    movdqa       (48)(%rsi), %xmm6
    psraw        %xmm7, %xmm6
    movdqa       %xmm6, (48)(%rsi)
    add          $(64), %rsi
    sub          $(32), %rdx
    jge          LL32gas_11
LL16gas_11:  
    add          $(16), %rdx
    jl           LL8gas_11
    movdqa       (%rsi), %xmm1
    psraw        %xmm7, %xmm1
    movdqa       %xmm1, (%rsi)
    movdqa       (16)(%rsi), %xmm2
    psraw        %xmm7, %xmm2
    movdqa       %xmm2, (16)(%rsi)
    add          $(32), %rsi
    sub          $(16), %rdx
LL8gas_11:  
    add          $(8), %rdx
    jl           LL4gas_11
    movdqa       (%rsi), %xmm3
    psraw        %xmm7, %xmm3
    movdqa       %xmm3, (%rsi)
    add          $(16), %rsi
    sub          $(8), %rdx
LL4gas_11:  
    add          $(4), %rdx
    jl           LL2gas_11
    movq         (%rsi), %xmm4
    psraw        %xmm7, %xmm4
    movq         %xmm4, (%rsi)
    add          $(8), %rsi
    sub          $(4), %rdx
LL2gas_11:  
    add          $(2), %rdx
    jl           LL1gas_11
    movd         (%rsi), %xmm5
    psraw        %xmm7, %xmm5
    movd         %xmm5, (%rsi)
    add          $(4), %rsi
    sub          $(2), %rdx
LL1gas_11:  
    add          $(1), %rdx
    jl           LExitgas_11
    movw         (%rsi), %ax
    sar          %cl, %ax
    movw         %ax, (%rsi)
LExitgas_11:  
 
 
    pop          %rbx
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownps_RShiftC_16s

 
mfxownps_RShiftC_16s:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    mov          %ecx, %ecx
    mov          %rcx, %rbp
    mov          %esi, %ecx
    movd         %ecx, %xmm7
    test         $(1), %rdx
    jz           LRun0gas_12
    sub          $(4), %rbp
    jl           LL2gas_12
LL4ngas_12:  
    movq         (%rdi), %xmm4
    psraw        %xmm7, %xmm4
    movq         %xmm4, (%rdx)
    add          $(8), %rdi
    add          $(8), %rdx
    sub          $(4), %rbp
    jge          LL4ngas_12
    jmp          LL2gas_12
LRun0gas_12:  
    mov          %rdx, %rbx
    and          $(15), %rbx
    jz           LRungas_12
    sub          $(16), %rbx
    neg          %rbx
    shr          $(1), %rbx
    cmp          %rbx, %rbp
    jl           LRungas_12
    sub          %rbx, %rbp
LNAgas_12:  
    movw         (%rdi), %ax
    sar          %cl, %ax
    movw         %ax, (%rdx)
    add          $(2), %rdi
    add          $(2), %rdx
    sub          $(1), %rbx
    jnz          LNAgas_12
LRungas_12:  
    test         $(15), %rdi
    jnz          LNNAgas_12
    sub          $(32), %rbp
    jl           LL16gas_12
LL32gas_12:  
    movdqa       (%rdi), %xmm3
    movdqa       (16)(%rdi), %xmm4
    movdqa       (32)(%rdi), %xmm5
    movdqa       (48)(%rdi), %xmm6
    psraw        %xmm7, %xmm3
    psraw        %xmm7, %xmm4
    psraw        %xmm7, %xmm5
    psraw        %xmm7, %xmm6
    movdqa       %xmm3, (%rdx)
    movdqa       %xmm4, (16)(%rdx)
    movdqa       %xmm5, (32)(%rdx)
    movdqa       %xmm6, (48)(%rdx)
    add          $(64), %rdi
    add          $(64), %rdx
    sub          $(32), %rbp
    jge          LL32gas_12
LL16gas_12:  
    add          $(16), %rbp
    jl           LL8gas_12
    movdqa       (%rdi), %xmm1
    movdqa       (16)(%rdi), %xmm2
    psraw        %xmm7, %xmm1
    psraw        %xmm7, %xmm2
    movdqa       %xmm1, (%rdx)
    movdqa       %xmm2, (16)(%rdx)
    add          $(32), %rdi
    add          $(32), %rdx
    sub          $(16), %rbp
LL8gas_12:  
    add          $(8), %rbp
    jl           LL4gas_12
    movdqa       (%rdi), %xmm3
    psraw        %xmm7, %xmm3
    movdqa       %xmm3, (%rdx)
    add          $(16), %rdi
    add          $(16), %rdx
    sub          $(8), %rbp
    jmp          LL4gas_12
LNNAgas_12:  
    sub          $(32), %rbp
    jl           LL16ngas_12
LL32ngas_12:  
    lddqu        (%rdi), %xmm3
    lddqu        (16)(%rdi), %xmm4
    lddqu        (32)(%rdi), %xmm5
    lddqu        (48)(%rdi), %xmm6
    psraw        %xmm7, %xmm3
    psraw        %xmm7, %xmm4
    psraw        %xmm7, %xmm5
    psraw        %xmm7, %xmm6
    movdqa       %xmm3, (%rdx)
    movdqa       %xmm4, (16)(%rdx)
    movdqa       %xmm5, (32)(%rdx)
    movdqa       %xmm6, (48)(%rdx)
    add          $(64), %rdi
    add          $(64), %rdx
    sub          $(32), %rbp
    jge          LL32ngas_12
LL16ngas_12:  
    add          $(16), %rbp
    jl           LL8ngas_12
    lddqu        (%rdi), %xmm1
    lddqu        (16)(%rdi), %xmm2
    psraw        %xmm7, %xmm1
    psraw        %xmm7, %xmm2
    movdqa       %xmm1, (%rdx)
    movdqa       %xmm2, (16)(%rdx)
    add          $(32), %rdi
    add          $(32), %rdx
    sub          $(16), %rbp
LL8ngas_12:  
    add          $(8), %rbp
    jl           LL4gas_12
    lddqu        (%rdi), %xmm3
    psraw        %xmm7, %xmm3
    movdqa       %xmm3, (%rdx)
    add          $(16), %rdi
    add          $(16), %rdx
    sub          $(8), %rbp
LL4gas_12:  
    add          $(4), %rbp
    jl           LL2gas_12
    movq         (%rdi), %xmm4
    psraw        %xmm7, %xmm4
    movq         %xmm4, (%rdx)
    add          $(8), %rdi
    add          $(8), %rdx
    sub          $(4), %rbp
LL2gas_12:  
    add          $(2), %rbp
    jl           LL1gas_12
    movd         (%rdi), %xmm5
    psraw        %xmm7, %xmm5
    movd         %xmm5, (%rdx)
    add          $(4), %rdi
    add          $(4), %rdx
    sub          $(2), %rbp
LL1gas_12:  
    add          $(1), %rbp
    jl           LExitgas_12
    movw         (%rdi), %ax
    sar          %cl, %ax
    movw         %ax, (%rdx)
LExitgas_12:  
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownps_RShiftC_32s_I

 
mfxownps_RShiftC_32s_I:
 
 
    push         %rbx
 
 
    mov          %edx, %edx
    movd         %edi, %xmm7
    test         $(3), %rsi
    jz           LRun0gas_13
    sub          $(4), %rdx
    jl           LL2gas_13
LL4ngas_13:  
    movq         (%rsi), %xmm0
    psrad        %xmm7, %xmm0
    movq         %xmm0, (%rsi)
    add          $(8), %rsi
    sub          $(2), %rdx
    jge          LL4ngas_13
    jmp          LL2gas_13
LRun0gas_13:  
    mov          %rsi, %rbx
    and          $(15), %rbx
    jz           LRungas_13
    sub          $(16), %rbx
    neg          %rbx
    shr          $(2), %rbx
    cmp          %rbx, %rdx
    jl           LRungas_13
    sub          %rbx, %rdx
LNAgas_13:  
    movd         (%rsi), %xmm0
    psrad        %xmm7, %xmm0
    movd         %xmm0, (%rsi)
    add          $(4), %rsi
    sub          $(1), %rbx
    jnz          LNAgas_13
LRungas_13:  
    sub          $(16), %rdx
    jl           LL8gas_13
LL16gas_13:  
    movdqa       (%rsi), %xmm3
    psrad        %xmm7, %xmm3
    movdqa       %xmm3, (%rsi)
    movdqa       (16)(%rsi), %xmm4
    psrad        %xmm7, %xmm4
    movdqa       %xmm4, (16)(%rsi)
    movdqa       (32)(%rsi), %xmm5
    psrad        %xmm7, %xmm5
    movdqa       %xmm5, (32)(%rsi)
    movdqa       (48)(%rsi), %xmm6
    psrad        %xmm7, %xmm6
    movdqa       %xmm6, (48)(%rsi)
    add          $(64), %rsi
    sub          $(16), %rdx
    jge          LL16gas_13
LL8gas_13:  
    add          $(8), %rdx
    jl           LL4gas_13
    movdqa       (%rsi), %xmm1
    psrad        %xmm7, %xmm1
    movdqa       %xmm1, (%rsi)
    movdqa       (16)(%rsi), %xmm2
    psrad        %xmm7, %xmm2
    movdqa       %xmm2, (16)(%rsi)
    add          $(32), %rsi
    sub          $(8), %rdx
LL4gas_13:  
    add          $(4), %rdx
    jl           LL2gas_13
    movdqa       (%rsi), %xmm3
    psrad        %xmm7, %xmm3
    movdqa       %xmm3, (%rsi)
    add          $(16), %rsi
    sub          $(4), %rdx
LL2gas_13:  
    add          $(2), %rdx
    jl           LL1gas_13
    movq         (%rsi), %xmm4
    psrad        %xmm7, %xmm4
    movq         %xmm4, (%rsi)
    add          $(8), %rsi
    sub          $(2), %rdx
LL1gas_13:  
    add          $(1), %rdx
    jl           LExitgas_13
    movd         (%rsi), %xmm5
    psrad        %xmm7, %xmm5
    movd         %xmm5, (%rsi)
LExitgas_13:  
 
 
    pop          %rbx
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownps_RShiftC_32s

 
mfxownps_RShiftC_32s:
 
 
    push         %rbx
 
 
    mov          %ecx, %ecx
    movd         %esi, %xmm7
    test         $(3), %rdx
    jz           LRun0gas_14
    sub          $(2), %rcx
    jl           LL1gas_14
LL2ngas_14:  
    movq         (%rdi), %xmm4
    psrad        %xmm7, %xmm4
    movq         %xmm4, (%rdx)
    add          $(8), %rdi
    add          $(8), %rdx
    sub          $(2), %rcx
    jge          LL2ngas_14
    jmp          LL1gas_14
LRun0gas_14:  
    mov          %rdx, %rbx
    and          $(15), %rbx
    jz           LRungas_14
    sub          $(16), %rbx
    neg          %rbx
    shr          $(2), %rbx
    cmp          %rbx, %rcx
    jl           LRungas_14
    sub          %rbx, %rcx
LNAgas_14:  
    movd         (%rdi), %xmm0
    psrad        %xmm7, %xmm0
    movd         %xmm0, (%rdx)
    add          $(4), %rdi
    add          $(4), %rdx
    sub          $(1), %rbx
    jnz          LNAgas_14
LRungas_14:  
    test         $(15), %rdi
    jnz          LNNAgas_14
    sub          $(16), %rcx
    jl           LL8gas_14
LL16gas_14:  
    movdqa       (%rdi), %xmm3
    movdqa       (16)(%rdi), %xmm4
    movdqa       (32)(%rdi), %xmm5
    movdqa       (48)(%rdi), %xmm6
    psrad        %xmm7, %xmm3
    psrad        %xmm7, %xmm4
    psrad        %xmm7, %xmm5
    psrad        %xmm7, %xmm6
    movdqa       %xmm3, (%rdx)
    movdqa       %xmm4, (16)(%rdx)
    movdqa       %xmm5, (32)(%rdx)
    movdqa       %xmm6, (48)(%rdx)
    add          $(64), %rdi
    add          $(64), %rdx
    sub          $(16), %rcx
    jge          LL16gas_14
LL8gas_14:  
    add          $(8), %rcx
    jl           LL4gas_14
    movdqa       (%rdi), %xmm1
    movdqa       (16)(%rdi), %xmm2
    psrad        %xmm7, %xmm1
    psrad        %xmm7, %xmm2
    movdqa       %xmm1, (%rdx)
    movdqa       %xmm2, (16)(%rdx)
    add          $(32), %rdi
    add          $(32), %rdx
    sub          $(8), %rcx
LL4gas_14:  
    add          $(4), %rcx
    jl           LL2gas_14
    movdqa       (%rdi), %xmm3
    psrad        %xmm7, %xmm3
    movdqa       %xmm3, (%rdx)
    add          $(16), %rdi
    add          $(16), %rdx
    sub          $(4), %rcx
    jmp          LL2gas_14
LNNAgas_14:  
    sub          $(16), %rcx
    jl           LL8ngas_14
LL16ngas_14:  
    lddqu        (%rdi), %xmm3
    lddqu        (16)(%rdi), %xmm4
    lddqu        (32)(%rdi), %xmm5
    lddqu        (48)(%rdi), %xmm6
    psrad        %xmm7, %xmm3
    psrad        %xmm7, %xmm4
    psrad        %xmm7, %xmm5
    psrad        %xmm7, %xmm6
    movdqa       %xmm3, (%rdx)
    movdqa       %xmm4, (16)(%rdx)
    movdqa       %xmm5, (32)(%rdx)
    movdqa       %xmm6, (48)(%rdx)
    add          $(64), %rdi
    add          $(64), %rdx
    sub          $(16), %rcx
    jge          LL16ngas_14
LL8ngas_14:  
    add          $(8), %rcx
    jl           LL4ngas_14
    lddqu        (%rdi), %xmm1
    lddqu        (16)(%rdi), %xmm2
    psrad        %xmm7, %xmm1
    psrad        %xmm7, %xmm2
    movdqa       %xmm1, (%rdx)
    movdqa       %xmm2, (16)(%rdx)
    add          $(32), %rdi
    add          $(32), %rdx
    sub          $(8), %rcx
LL4ngas_14:  
    add          $(4), %rcx
    jl           LL2gas_14
    lddqu        (%rdi), %xmm3
    psrad        %xmm7, %xmm3
    movdqa       %xmm3, (%rdx)
    add          $(16), %rdi
    add          $(16), %rdx
    sub          $(4), %rcx
LL2gas_14:  
    add          $(2), %rcx
    jl           LL1gas_14
    movq         (%rdi), %xmm4
    psrad        %xmm7, %xmm4
    movq         %xmm4, (%rdx)
    add          $(8), %rdi
    add          $(8), %rdx
    sub          $(2), %rcx
LL1gas_14:  
    add          $(1), %rcx
    jl           LExitgas_14
    movd         (%rdi), %xmm5
    psrad        %xmm7, %xmm5
    movd         %xmm5, (%rdx)
LExitgas_14:  
 
 
    pop          %rbx
 
    ret
 
 
