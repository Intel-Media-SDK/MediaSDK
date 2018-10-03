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
 
 
.globl mfxowniConvert_8u16u_M7

 
mfxowniConvert_8u16u_M7:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    push         %r12
 
 
    push         %r13
 
 
    push         %r14
 
 
    push         %r15
 
 
    mov          %edx, %edx
    mov          %ecx, %ecx
 
 
LAlign8u16u00gas_1:  
    test         $(15), %rsi
    jz           LCvrt_8u16u00gas_1
 
    movzbl       (%rdi), %eax
    add          $(1), %rdi
    mov          %ax, (%rsi)
    add          $(2), %rsi
    sub          $(1), %rdx
    jz           LCvrtExit_8u16ugas_1
 
    jmp          LAlign8u16u00gas_1
 
 
LCvrt_8u16u00gas_1:  
    pxor         %xmm0, %xmm0
    sub          $(32), %rdx
    jl           LCvrt16_8u16u00gas_1
 
    test         %rcx, %rcx
    jnz          LCvrt32WNt8u16u01gas_1
 
 
LCvrt32_8u16u01gas_1:  
    movq         (%rdi), %xmm1
    movq         (8)(%rdi), %xmm2
    movq         (16)(%rdi), %xmm3
    movq         (24)(%rdi), %xmm4
    add          $(32), %rdi
 
    punpcklbw    %xmm0, %xmm1
    punpcklbw    %xmm0, %xmm2
    punpcklbw    %xmm0, %xmm3
    punpcklbw    %xmm0, %xmm4
 
    movdqa       %xmm1, (%rsi)
    movdqa       %xmm2, (16)(%rsi)
    movdqa       %xmm3, (32)(%rsi)
    movdqa       %xmm4, (48)(%rsi)
 
    add          $(64), %rsi
    sub          $(32), %rdx
    jge          LCvrt32_8u16u01gas_1
 
    jmp          LCvrt16_8u16u00gas_1
 
 
LCvrt32WNt8u16u01gas_1:  
    movq         (%rdi), %xmm1
    movq         (8)(%rdi), %xmm2
    movq         (16)(%rdi), %xmm3
    movq         (24)(%rdi), %xmm4
    add          $(32), %rdi
 
    punpcklbw    %xmm0, %xmm1
    punpcklbw    %xmm0, %xmm2
    punpcklbw    %xmm0, %xmm3
    punpcklbw    %xmm0, %xmm4
 
    movntdq      %xmm1, (%rsi)
    movntdq      %xmm2, (16)(%rsi)
    movntdq      %xmm3, (32)(%rsi)
    movntdq      %xmm4, (48)(%rsi)
 
    add          $(64), %rsi
    sub          $(32), %rdx
    jge          LCvrt32WNt8u16u01gas_1
 
    sfence
 
 
LCvrt16_8u16u00gas_1:  
    add          $(16), %rdx
    jl           LTail8u16u00gas_1
 
LCvrt16_8u16u01gas_1:  
    movq         (%rdi), %xmm1
    movq         (8)(%rdi), %xmm2
    add          $(16), %rdi
 
    punpcklbw    %xmm0, %xmm1
    punpcklbw    %xmm0, %xmm2
 
    movdqa       %xmm1, (%rsi)
    movdqa       %xmm2, (16)(%rsi)
 
    add          $(32), %rsi
    sub          $(16), %rdx
 
 
LTail8u16u00gas_1:  
    add          $(16), %rdx
    jz           LCvrtExit_8u16ugas_1
 
LTail8u16u01gas_1:  
    movzbl       (%rdi), %eax
    add          $(1), %rdi
    mov          %ax, (%rsi)
    add          $(2), %rsi
    sub          $(1), %rdx
    jnz          LTail8u16u01gas_1
 
 
LCvrtExit_8u16ugas_1:  
 
 
    pop          %r15
 
 
    pop          %r14
 
 
    pop          %r13
 
 
    pop          %r12
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
.data
 
.p2align 4, 0x90
 
 
