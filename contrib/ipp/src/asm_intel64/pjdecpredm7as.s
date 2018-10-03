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
 
 
.globl mfxownpj_ReconstructRow_PRED1_JPEG_16s_C1

 
mfxownpj_ReconstructRow_PRED1_JPEG_16s_C1:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    push         %r12
 
 
    push         %r13
 
 
    push         %r14
 
 
    push         %r15
 
 
    mov          %ecx, %ecx
 
    movzwl       (%rdi), %eax
    add          $(2), %rdi
    addw         (%rsi), %ax
    mov          %ax, (%rdx)
    add          $(2), %rdx
    dec          %ecx
    jz           LExitReconstPred1_00gas_1
 
 
LLoopReconstPred1_00gas_1:  
    sub          $(9), %ecx
    jl           LTailReconstPred1_00gas_1
 
LLoopReconstPred1_01gas_1:  
    addw         (%rdi), %ax
    add          $(18), %rdi
    mov          %ax, (%rdx)
    addw         (-16)(%rdi), %ax
    mov          %ax, (2)(%rdx)
    addw         (-14)(%rdi), %ax
    mov          %ax, (4)(%rdx)
    addw         (-12)(%rdi), %ax
    mov          %ax, (6)(%rdx)
    addw         (-10)(%rdi), %ax
    mov          %ax, (8)(%rdx)
    addw         (-8)(%rdi), %ax
    mov          %ax, (10)(%rdx)
    addw         (-6)(%rdi), %ax
    mov          %ax, (12)(%rdx)
    addw         (-4)(%rdi), %ax
    mov          %ax, (14)(%rdx)
    addw         (-2)(%rdi), %ax
    mov          %ax, (16)(%rdx)
    add          $(18), %rdx
    sub          $(9), %ecx
    jge          LLoopReconstPred1_01gas_1
 
 
LTailReconstPred1_00gas_1:  
    add          $(9), %ecx
    jz           LExitReconstPred1_00gas_1
 
LTailReconstPred1_01gas_1:  
    addw         (%rdi), %ax
    mov          %ax, (%rdx)
    dec          %ecx
    jz           LExitReconstPred1_00gas_1
    addw         (2)(%rdi), %ax
    mov          %ax, (2)(%rdx)
    dec          %ecx
    jz           LExitReconstPred1_00gas_1
    addw         (4)(%rdi), %ax
    mov          %ax, (4)(%rdx)
    dec          %ecx
    jz           LExitReconstPred1_00gas_1
    addw         (6)(%rdi), %ax
    mov          %ax, (6)(%rdx)
    add          $(8), %rdi
    add          $(8), %rdx
    dec          %ecx
    jnz          LTailReconstPred1_01gas_1
 
 
LExitReconstPred1_00gas_1:  
 
 
    pop          %r15
 
 
    pop          %r14
 
 
    pop          %r13
 
 
    pop          %r12
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
.p2align 4, 0x90
 
 
.globl mfxownpj_ReconstructPredFirstRow_JPEG_16s_C1

 
mfxownpj_ReconstructPredFirstRow_JPEG_16s_C1:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    push         %r12
 
 
    push         %r13
 
 
    push         %r14
 
 
    push         %r15
 
 
    mov          %ecx, %ecx
    mov          %edx, %edx
    mov          %r8d, %r8d
 
    sub          %r8d, %ecx
    dec          %ecx
    mov          $(1), %eax
    shl          %cl, %eax
    addw         (%rdi), %ax
    add          $(2), %rdi
    mov          %ax, (%rsi)
    add          $(2), %rsi
    dec          %edx
    jz           LExitReconstPredFirst00gas_2
 
 
LLoopReconstPredFirst00gas_2:  
    sub          $(9), %edx
    jl           LTailReconstPredFirst00gas_2
 
LLoopReconstPredFirst01gas_2:  
    addw         (%rdi), %ax
    add          $(18), %rdi
    mov          %ax, (%rsi)
    addw         (-16)(%rdi), %ax
    mov          %ax, (2)(%rsi)
    addw         (-14)(%rdi), %ax
    mov          %ax, (4)(%rsi)
    addw         (-12)(%rdi), %ax
    mov          %ax, (6)(%rsi)
    addw         (-10)(%rdi), %ax
    mov          %ax, (8)(%rsi)
    addw         (-8)(%rdi), %ax
    mov          %ax, (10)(%rsi)
    addw         (-6)(%rdi), %ax
    mov          %ax, (12)(%rsi)
    addw         (-4)(%rdi), %ax
    mov          %ax, (14)(%rsi)
    addw         (-2)(%rdi), %ax
    mov          %ax, (16)(%rsi)
    add          $(18), %rsi
    sub          $(9), %edx
    jge          LLoopReconstPredFirst01gas_2
 
 
LTailReconstPredFirst00gas_2:  
    add          $(9), %edx
    jz           LExitReconstPredFirst00gas_2
 
LTailReconstPredFirst01gas_2:  
    addw         (%rdi), %ax
    mov          %ax, (%rsi)
    dec          %edx
    jz           LExitReconstPredFirst00gas_2
    addw         (2)(%rdi), %ax
    mov          %ax, (2)(%rsi)
    dec          %edx
    jz           LExitReconstPredFirst00gas_2
    addw         (4)(%rdi), %ax
    mov          %ax, (4)(%rsi)
    dec          %edx
    jz           LExitReconstPredFirst00gas_2
    addw         (6)(%rdi), %ax
    mov          %ax, (6)(%rsi)
    add          $(8), %rdi
    add          $(8), %rsi
    dec          %edx
    jnz          LTailReconstPredFirst01gas_2
 
 
LExitReconstPredFirst00gas_2:  
 
 
    pop          %r15
 
 
    pop          %r14
 
 
    pop          %r13
 
 
    pop          %r12
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
