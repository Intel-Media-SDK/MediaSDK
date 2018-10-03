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
 
const_array_of_128:
.word  128,128,128,128,128,128,128,128  
 
const_array_of_4:
.word  4,4,4,4,4,4,4,4  
 
const_array_of_9:
.word  9,9,9,9,9,9,9,9  
 
const_array_of_0:
.word  0,0,0,0,0,0,0,0  
 
 
.text
 
 
.globl mfxrangemapping_vc1_sse2

 
mfxrangemapping_vc1_sse2:
 
 
    push         %rbp
    mov          %rsp, %rbp
 
 
    sub          $(32), %rsp
 
 
    mov          %rbx, (-24)(%rbp)
 
    mov          %r12, (-16)(%rbp)
 
 
    mov          %r14, (-8)(%rbp)
 
 
    lea          const_array_of_4(%rip), %rax
    movdqa       (%rax), %xmm4
    lea          const_array_of_128(%rip), %rax
    movdqa       (%rax), %xmm6
    movslq       (16)(%rbp), %rax
    movd         %rax, %xmm5
    pshuflw      $(0), %xmm5, %xmm5
    pshufd       $(0), %xmm5, %xmm5
    lea          const_array_of_9(%rip), %rax
    paddsw       (%rax), %xmm5
    lea          const_array_of_0(%rip), %rax
    movdqa       (%rax), %xmm0
 
    mov          %rdi, %rax
    mov          %rdx, %r10
    xor          %r12, %r12
    xor          %r14, %r14
    xor          %r11, %r11
 
LNEXT_Igas_1:  
    xor          %rbx, %rbx
 
LNEXT_Jgas_1:  
    movq         (%rax), %xmm1
    punpcklbw    %xmm0, %xmm1
 
    psubsw       %xmm6, %xmm1
    pmullw       %xmm5, %xmm1
    paddsw       %xmm4, %xmm1
    psraw        $(3), %xmm1
    paddsw       %xmm6, %xmm1
 
    packuswb     %xmm1, %xmm1
 
    movq         %xmm1, (%r10)
 
    add          $(8), %rax
    add          $(8), %r10
    add          $(1), %rbx
    cmp          %r9, %rbx
    jne          LNEXT_Jgas_1
 
    add          %rsi, %r12
    add          %rcx, %r14
    mov          %rdi, %rax
    mov          %rdx, %r10
    add          %r12, %rax
    add          %r14, %r10
    add          $(1), %r11
    cmp          %r8, %r11
 
    jne          LNEXT_Igas_1
 
 
    mov          (-24)(%rbp), %rbx
 
 
    mov          (-16)(%rbp), %r12
 
 
    mov          (-8)(%rbp), %r14
 
 
    mov          %rbp, %rsp
    pop          %rbp
    ret
 
 
