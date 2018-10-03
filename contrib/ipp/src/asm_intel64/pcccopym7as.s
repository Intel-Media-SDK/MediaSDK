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
 
 
.globl mfxownccCopy_8u_C1_M7

 
mfxownccCopy_8u_C1_M7:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    push         %r12
 
 
    push         %r13
 
 
    push         %r14
 
 
    push         %r15
 
 
    mov          %edx, %edx
    mov          %ecx, %ecx
 
    cmp          $(64), %rdx
    jl           LShort8Copy8uC1_00gas_1
 
    test         $(15), %rsi
    jz           LLoopCopy8uC1_00gas_1
 
 
    lddqu        (%rdi), %xmm0
    mov          %rsi, %rbx
    and          $(15), %rbx
    movdqu       %xmm0, (%rsi)
    sub          $(16), %rbx
    sub          %rbx, %rdi
    sub          %rbx, %rsi
    add          %rbx, %rdx
 
    cmp          $(64), %rdx
    jl           LShort8Copy8uC1_00gas_1
 
 
LLoopCopy8uC1_00gas_1:  
    sub          $(64), %rdx
 
    test         %rcx, %rcx
    jnz          LLoopCopyNt8uC1_01gas_1
 
LLoopCopy8uC1_10gas_1:  
    test         $(7), %rdi
    jnz          LLoopNACopy8uC1_01gas_1
 
    test         $(8), %rdi
    jnz          LLoop80Copy8uC1_01gas_1
 
 
LLoop00Copy8uC1_01gas_1:  
    movdqa       (%rdi), %xmm0
    movdqa       %xmm0, (%rsi)
    movdqa       (16)(%rdi), %xmm0
    movdqa       %xmm0, (16)(%rsi)
    movdqa       (32)(%rdi), %xmm0
    movdqa       %xmm0, (32)(%rsi)
    movdqa       (48)(%rdi), %xmm0
    add          $(64), %rdi
    movdqa       %xmm0, (48)(%rsi)
    add          $(64), %rsi
    sub          $(64), %rdx
    jge          LLoop00Copy8uC1_01gas_1
 
    add          $(64), %rdx
    jz           LExitCopy8uC1_00gas_1
 
    jmp          LShort8Copy8uC1_00gas_1
 
LLoop80Copy8uC1_01gas_1:  
    movq         (%rdi), %xmm0
    movq         (8)(%rdi), %xmm1
    punpcklqdq   %xmm1, %xmm0
    movdqa       %xmm0, (%rsi)
    movq         (16)(%rdi), %xmm0
    movq         (24)(%rdi), %xmm1
    punpcklqdq   %xmm1, %xmm0
    movdqa       %xmm0, (16)(%rsi)
    movq         (32)(%rdi), %xmm0
    movq         (40)(%rdi), %xmm1
    punpcklqdq   %xmm1, %xmm0
    movdqa       %xmm0, (32)(%rsi)
    movq         (48)(%rdi), %xmm0
    movq         (56)(%rdi), %xmm1
    add          $(64), %rdi
    punpcklqdq   %xmm1, %xmm0
    movdqa       %xmm0, (48)(%rsi)
    add          $(64), %rsi
    sub          $(64), %rdx
    jge          LLoop80Copy8uC1_01gas_1
 
    add          $(64), %rdx
    jz           LExitCopy8uC1_00gas_1
 
    jmp          LShort8Copy8uC1_00gas_1
 
 
LLoopNACopy8uC1_01gas_1:  
    lddqu        (%rdi), %xmm0
    movdqu       %xmm0, (%rsi)
    lddqu        (16)(%rdi), %xmm0
    movdqu       %xmm0, (16)(%rsi)
    lddqu        (32)(%rdi), %xmm0
    movdqu       %xmm0, (32)(%rsi)
    lddqu        (48)(%rdi), %xmm0
    add          $(64), %rdi
    movdqa       %xmm0, (48)(%rsi)
    add          $(64), %rsi
    sub          $(64), %rdx
    jge          LLoopNACopy8uC1_01gas_1
 
    add          $(64), %rdx
    jz           LExitCopy8uC1_00gas_1
 
    jmp          LShort8Copy8uC1_00gas_1
 
 
LLoopCopyNt8uC1_01gas_1:  
    lddqu        (%rdi), %xmm0
    movntdq      %xmm0, (%rsi)
    lddqu        (16)(%rdi), %xmm0
    movntdq      %xmm0, (16)(%rsi)
    lddqu        (32)(%rdi), %xmm0
    movntdq      %xmm0, (32)(%rsi)
    lddqu        (48)(%rdi), %xmm0
    add          $(64), %rdi
    movntdq      %xmm0, (48)(%rsi)
    add          $(64), %rsi
    sub          $(64), %rdx
    jge          LLoopCopyNt8uC1_01gas_1
 
    sfence
 
    add          $(64), %rdx
    jz           LExitCopy8uC1_00gas_1
 
 
LShort8Copy8uC1_00gas_1:  
    sub          %rdi, %rsi
    sub          $(8), %rdx
    jl           LShort4Copy8uC1_00gas_1
 
LShort8Copy8uC1_02gas_1:  
    movq         (%rdi), %xmm0
    movq         %xmm0, (%rdi,%rsi)
    add          $(8), %rdi
    sub          $(8), %rdx
    jge          LShort8Copy8uC1_02gas_1
 
LShort4Copy8uC1_00gas_1:  
    add          $(8), %rdx
    test         %rdx, %rdx
    jz           LExitCopy8uC1_00gas_1
 
    test         $(4), %rdx
    jz           LShort2Copy8uC1_00gas_1
 
    mov          (%rdi), %ebx
    sub          $(4), %rdx
    mov          %ebx, (%rdi,%rsi)
    add          $(4), %rdi
 
LShort2Copy8uC1_00gas_1:  
    test         %rdx, %rdx
    jz           LExitCopy8uC1_00gas_1
 
    test         $(2), %rdx
    jz           LShort1Copy8uC1_00gas_1
 
    mov          (%rdi), %bx
    sub          $(2), %rdx
    mov          %bx, (%rdi,%rsi)
    add          $(2), %rdi
 
LShort1Copy8uC1_00gas_1:  
    test         %rdx, %rdx
    jz           LExitCopy8uC1_00gas_1
 
    mov          (%rdi), %bl
    mov          %bl, (%rdi,%rsi)
 
 
LExitCopy8uC1_00gas_1:  
 
 
    pop          %r15
 
 
    pop          %r14
 
 
    pop          %r13
 
 
    pop          %r12
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
.data
 
.p2align 4, 0x90
 
mask3A8u:
.quad    0xffffff00ffffff,  0xffffff00ffffff  
 
maskA8u:
.quad   0xff000000ff000000, 0xff000000ff000000  
 
mask3A16s:
.quad      0xffffffffffff,    0xffffffffffff  
 
maskA16s:
.quad   0xffff000000000000, 0xffff000000000000  
 
c127:
.quad  0x7f7f7f7f7f7f7f7f, 0x7f7f7f7f7f7f7f7f  
 
 
