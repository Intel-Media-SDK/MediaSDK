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
 
 
.globl mfxowniConvert_16u8u_M7

 
mfxowniConvert_16u8u_M7:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    push         %r12
 
 
    push         %r13
 
 
    push         %r14
 
 
    push         %r15
 
 
    mov          %edx, %edx
 
 
LAlign16u8u00gas_1:  
    test         $(15), %rdi
    jz           LCvrtW16u8u00gas_1
 
    movzwl       (%rdi), %eax
    add          $(2), %rdi
    cmp          $(255), %eax
    jle          LAlign16u8u01gas_1
    mov          $(255), %eax
    jmp          LAlign16u8u02gas_1
LAlign16u8u01gas_1:  
    cmp          $(0), %eax
    jge          LAlign16u8u02gas_1
    xor          %eax, %eax
LAlign16u8u02gas_1:  
    mov          %al, (%rsi)
    add          $(1), %rsi
    sub          $(1), %rdx
    jz           LCvrtExitW16u8ugas_1
 
    jmp          LAlign16u8u00gas_1
 
 
LCvrtW16u8u00gas_1:  
    movdqa       c32767(%rip), %xmm7
    sub          $(32), %rdx
    jl           LCvrt16W16u8u00gas_1
 
 
LCvrt32W16u8u01gas_1:  
    movdqa       (%rdi), %xmm0
    movdqa       (16)(%rdi), %xmm1
    movdqa       (32)(%rdi), %xmm2
    movdqa       (48)(%rdi), %xmm3
    add          $(64), %rdi
 
    pminub       %xmm7, %xmm0
    pminub       %xmm7, %xmm1
    pminub       %xmm7, %xmm2
    pminub       %xmm7, %xmm3
 
    packuswb     %xmm1, %xmm0
    packuswb     %xmm3, %xmm2
 
    movdqu       %xmm0, (%rsi)
    movdqu       %xmm2, (16)(%rsi)
 
    add          $(32), %rsi
    sub          $(32), %rdx
    jge          LCvrt32W16u8u01gas_1
 
 
LCvrt16W16u8u00gas_1:  
    add          $(16), %rdx
    jl           LTail16u8u00gas_1
 
    movdqa       (%rdi), %xmm0
    movdqa       (16)(%rdi), %xmm1
    add          $(32), %rdi
 
    pminub       %xmm7, %xmm0
    pminub       %xmm7, %xmm1
 
    packuswb     %xmm1, %xmm0
 
    movdqu       %xmm0, (%rsi)
 
    add          $(16), %rsi
    sub          $(16), %rdx
 
 
LTail16u8u00gas_1:  
    add          $(16), %rdx
    jz           LCvrtExitW16u8ugas_1
 
LTail16u8u01gas_1:  
    movzwl       (%rdi), %eax
    add          $(2), %rdi
    cmp          $(255), %eax
    jle          LTail16u8u02gas_1
    mov          $(255), %eax
    jmp          LTail16u8u03gas_1
LTail16u8u02gas_1:  
    cmp          $(0), %eax
    jge          LTail16u8u03gas_1
    xor          %eax, %eax
LTail16u8u03gas_1:  
    mov          %al, (%rsi)
    add          $(1), %rsi
    sub          $(1), %rdx
    jnz          LTail16u8u01gas_1
 
 
LCvrtExitW16u8ugas_1:  
 
 
    pop          %r15
 
 
    pop          %r14
 
 
    pop          %r13
 
 
    pop          %r12
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
.data
 
.p2align 4, 0x90
 
c32767:
.quad  0x7fff7fff7fff7fff  
 

.quad  0x7fff7fff7fff7fff  
 
 
