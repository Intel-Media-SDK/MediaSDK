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
 
 
.globl mfxownpj_EncodeHuffman8x8_JPEG_16s1u_C1

 
mfxownpj_EncodeHuffman8x8_JPEG_16s1u_C1:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    push         %r12
 
 
    push         %r13
 
 
    push         %r14
 
 
    push         %r15
 
 
    sub          $(72), %rsp
 
 
    mov          %rdi, (%rsp)
    mov          %rdx, %rax
    mov          %eax, %eax
    mov          %rax, (8)(%rsp)
    mov          %rcx, (16)(%rsp)
    mov          %r8, (24)(%rsp)
    mov          %r9, (40)(%rsp)
    mov          %rsi, %r8
 
 
    movq         (136)(%rsp), %rax
    mov          $(64), %ebp
    lea          (8)(%rax), %rdx
    mov          $(24), %ecx
    sub          (%rdx), %ebp
    sub          (%rdx), %ecx
    lea          (%rax), %rax
    mov          (%rax), %r11
    mov          (16)(%rsp), %rax
    shr          %cl, %r11
    mov          (%rax), %eax
    lea          ownTables(%rip), %rsi
    mov          %rax, %r12
    sub          (8)(%rsp), %eax
    cmp          $(-24), %eax
    jg           LEncHuffExitEndOfWork01gas_1
 
 
    mov          (24)(%rsp), %rax
    mov          (%rsp), %rdx
    movswl       (%rax), %ecx
    movswl       (%rdx), %edx
    mov          %ecx, (32)(%rsp)
    mov          %dx, (%rax)
    sub          %ecx, %edx
    xor          %ecx, %ecx
    mov          %edx, %ebx
    neg          %edx
    setg         %cl
    cmovl        %ebx, %edx
    sub          %ecx, %ebx
    mov          $(32), %ecx
    cmp          $(256), %edx
    jge          LEncHuffDcLong00gas_1
    movzbl       (64)(%rsi,%rdx), %edx
LEncHuffDc00gas_1:  
    sub          %edx, %ecx
    shl          %cl, %ebx
    shr          %cl, %ebx
    mov          (40)(%rsp), %rcx
    mov          %ebx, %r14d
    lea          (%rcx), %rbx
    mov          (%rbx,%rdx,4), %ecx
    mov          %ecx, %r15d
    shr          $(16), %ecx
    jz           LEncHuffExitEndOfWork00gas_1
    and          mask0F(%rip), %r15
    sub          %ecx, %ebp
    sub          %edx, %ebp
    shl          %cl, %r11
    mov          %edx, %ecx
    or           %r15, %r11
    shl          %cl, %r11
    or           %r14, %r11
 
 
    movq         (128)(%rsp), %r9
    lea          (%r9), %r9
    xor          %edx, %edx
    mov          (%rsp), %rbx
    mov          $(63), %edi
 
 
    or           (124)(%rbx), %edx
    jnz          LEncHuffAcZeros00gas_1
    sub          $(2), %edi
    or           (108)(%rbx), %edx
    jnz          LEncHuffAcZeros00gas_1
    sub          $(1), %edi
    or           (92)(%rbx), %edx
    jnz          LEncHuffAcZeros00gas_1
    sub          $(2), %edi
    or           (120)(%rbx), %edx
    jnz          LEncHuffAcZeros00gas_1
    sub          $(2), %edi
    or           (104)(%rbx), %edx
    jnz          LEncHuffAcZeros00gas_1
    sub          $(2), %edi
    or           (76)(%rbx), %edx
    jnz          LEncHuffAcZeros00gas_1
    sub          $(1), %edi
    or           (60)(%rbx), %edx
    jnz          LEncHuffAcZeros00gas_1
    sub          $(2), %edi
    or           (88)(%rbx), %edx
    jnz          LEncHuffAcZeros00gas_1
    sub          $(2), %edi
    or           (116)(%rbx), %edx
    jnz          LEncHuffAcZeros00gas_1
    sub          $(2), %edi
    or           (100)(%rbx), %edx
    jnz          LEncHuffAcZeros00gas_1
    sub          $(2), %edi
    or           (72)(%rbx), %edx
    jnz          LEncHuffAcZeros00gas_1
    sub          $(2), %edi
    or           (44)(%rbx), %edx
    jnz          LEncHuffAcZeros00gas_1
    sub          $(1), %edi
    or           (28)(%rbx), %edx
    jnz          LEncHuffAcZeros00gas_1
    sub          $(2), %edi
    or           (56)(%rbx), %edx
    jnz          LEncHuffAcZeros00gas_1
    sub          $(2), %edi
    or           (84)(%rbx), %edx
    jnz          LEncHuffAcZeros00gas_1
    sub          $(2), %edi
    or           (112)(%rbx), %edx
    jnz          LEncHuffAcZeros00gas_1
    sub          $(2), %edi
    or           (96)(%rbx), %edx
    jnz          LEncHuffAcZeros00gas_1
    sub          $(2), %edi
    or           (68)(%rbx), %edx
    jnz          LEncHuffAcZeros00gas_1
    sub          $(2), %edi
    or           (40)(%rbx), %edx
    jnz          LEncHuffAcZeros00gas_1
    sub          $(2), %edi
    or           (12)(%rbx), %edx
    jnz          LEncHuffAcZeros00gas_1
    sub          $(2), %edi
    or           (24)(%rbx), %edx
    jnz          LEncHuffAcZeros00gas_1
    sub          $(2), %edi
    or           (52)(%rbx), %edx
    jnz          LEncHuffAcZeros00gas_1
    sub          $(2), %edi
    or           (80)(%rbx), %edx
    jnz          LEncHuffAcZeros00gas_1
    sub          $(3), %edi
    or           (64)(%rbx), %edx
    jnz          LEncHuffAcZeros00gas_1
    sub          $(2), %edi
    or           (36)(%rbx), %edx
    jnz          LEncHuffAcZeros00gas_1
    sub          $(2), %edi
    or           (8)(%rbx), %edx
    jnz          LEncHuffAcZeros00gas_1
    sub          $(2), %edi
    or           (20)(%rbx), %edx
    jnz          LEncHuffAcZeros00gas_1
    sub          $(2), %edi
    or           (48)(%rbx), %edx
    jnz          LEncHuffAcZeros00gas_1
    sub          $(3), %edi
    or           (32)(%rbx), %edx
    jnz          LEncHuffAcZeros00gas_1
    sub          $(2), %edi
    or           (4)(%rbx), %edx
    jnz          LEncHuffAcZeros00gas_1
    sub          $(2), %edi
    or           (16)(%rbx), %edx
    jnz          LEncHuffAcZeros00gas_1
    sub          $(2), %edi
 
LEncHuffAcZeros00gas_1:  
    xor          %edx, %edx
    movzbl       (%rdi,%rsi), %ecx
LEncHuffAcZeros01gas_1:  
    orw          (%rbx,%rcx,2), %dx
    jnz          LEncHuffAcZeros02gas_1
    movzbl       (-1)(%rdi,%rsi), %ecx
    sub          $(1), %edi
    jg           LEncHuffAcZeros01gas_1
LEncHuffAcZeros02gas_1:  
    add          $(1), %edi
    mov          $(63), %eax
    cmp          $(63), %edi
    cmovg        %eax, %edi
    mov          %edi, (56)(%rsp)
    mov          $(1), %edi
    xor          %r13d, %r13d
    movzbl       (%rdi,%rsi), %ecx
 
 
LEncHuffAc00gas_1:  
    movswl       (%rbx,%rcx,2), %edx
    movzbl       (1)(%rdi,%rsi), %ecx
    test         %edx, %edx
    jnz          LEncHuffAc01gas_1
    mov          (56)(%rsp), %edx
    add          $(16), %r13d
    add          $(1), %edi
    cmp          %edx, %edi
    jle          LEncHuffAc00gas_1
    jmp          LEncHuffAc10gas_1
 
 
LEncHuffAc01gas_1:  
    cmp          $(256), %r13d
    jge          LEncHuffAc20gas_1
LEncHuffAc02gas_1:  
    xor          %ecx, %ecx
    mov          %edx, %ebx
    neg          %edx
    setg         %cl
    cmovl        %ebx, %edx
    sub          %ecx, %ebx
    mov          $(32), %ecx
    cmp          $(256), %edx
    jge          LEncHuffAcLong00gas_1
    movzbl       (64)(%rsi,%rdx), %edx
LEncHuffAc03gas_1:  
    sub          %edx, %ecx
    shl          %cl, %ebx
    or           %edx, %r13d
    shr          %cl, %ebx
    mov          %ebx, %r14d
    mov          (%r9,%r13,4), %ebx
    mov          %ebx, %r15d
    shr          $(16), %ebx
    jz           LEncHuffExitEndOfWork00gas_1
    and          mask0F(%rip), %r15
    cmp          %ebx, %ebp
    jl           LEncHuffCallWrite00gas_1
LEncHuffRetWrite00gas_1:  
    mov          %ebx, %ecx
    sub          %ebx, %ebp
    mov          (%rsp), %rbx
    shl          %cl, %r11
    or           %r15, %r11
    cmp          %edx, %ebp
    jl           LEncHuffCallWrite01gas_1
LEncHuffRetWrite01gas_1:  
    mov          %edx, %ecx
    add          $(1), %edi
    shl          %cl, %r11
    or           %r14, %r11
    sub          %edx, %ebp
    mov          (56)(%rsp), %edx
    xor          %r13d, %r13d
    movzbl       (%rdi,%rsi), %ecx
    cmp          %edx, %edi
    jle          LEncHuffAc00gas_1
    jmp          LEncHuffExitNormgas_1
 
 
LEncHuffAc10gas_1:  
    mov          (%r9), %ebx
    mov          %ebx, %r15d
    shr          $(16), %ebx
    jz           LEncHuffExitEndOfWork00gas_1
    and          mask0F(%rip), %r15
    cmp          %ebx, %ebp
    jl           LEncHuffCallWrite03gas_1
LEncHuffRetWrite03gas_1:  
    mov          %ebx, %ecx
    sub          %ebx, %ebp
    shl          %cl, %r11
    or           %r15, %r11
    jmp          LEncHuffExitNormgas_1
 
 
LEncHuffAc20gas_1:  
    mov          (960)(%r9), %ebx
    mov          %ebx, %r15d
    shr          $(16), %ebx
    jz           LEncHuffExitEndOfWork00gas_1
    and          mask0F(%rip), %r15
    cmp          %ebx, %ebp
    jl           LEncHuffCallWrite02gas_1
LEncHuffRetWrite02gas_1:  
    mov          %ebx, %ecx
    sub          %ebx, %ebp
    shl          %cl, %r11
    or           %r15, %r11
    sub          $(256), %r13d
    cmp          $(256), %r13d
    jge          LEncHuffAc20gas_1
    jmp          LEncHuffAc02gas_1
 
 
LEncHuffCallWrite04gas_1:  
    lea          LEncHuffRetWrite04gas_1(%rip), %rax
    mov          %rax, (48)(%rsp)
    jmp          LEncHuffWrite00gas_1
 
LEncHuffCallWrite03gas_1:  
    lea          LEncHuffRetWrite03gas_1(%rip), %rax
    mov          %rax, (48)(%rsp)
    jmp          LEncHuffWrite00gas_1
 
LEncHuffCallWrite02gas_1:  
    lea          LEncHuffRetWrite02gas_1(%rip), %rax
    mov          %rax, (48)(%rsp)
    jmp          LEncHuffWrite00gas_1
 
LEncHuffCallWrite01gas_1:  
    lea          LEncHuffRetWrite01gas_1(%rip), %rax
    mov          %rax, (48)(%rsp)
    jmp          LEncHuffWrite00gas_1
 
LEncHuffCallWrite00gas_1:  
    lea          LEncHuffRetWrite00gas_1(%rip), %rax
    mov          %rax, (48)(%rsp)
 
LEncHuffWrite00gas_1:  
    mov          %ebp, %ecx
    sub          $(64), %ebp
    neg          %ebp
    mov          %r11, %r10
    shl          %cl, %r10
    movd         %r10, %xmm1
    bswap        %r10
    mov          %r12d, %eax
    mov          %r12d, %ecx
    subl         (8)(%rsp), %eax
    cmp          $(-8), %eax
    jg           LEncHuffExitEndOfWork00gas_1
    cmp          $(32), %ebp
    jl           LEncHuffWrite11gas_1
 
    pcmpeqb      maskFF(%rip), %xmm1
    movd         %xmm1, %rax
    test         %rax, %rax
    jnz          LEncHuffWrite10gas_1
    mov          %r10, (%r8,%rcx)
    mov          %ebp, %eax
    and          $(7), %ebp
    and          $(4294967288), %eax
    sub          $(64), %ebp
    shr          $(3), %eax
    add          %eax, %r12d
    mov          (48)(%rsp), %rax
    neg          %ebp
    jmp          *%rax
 
 
LEncHuffWrite10gas_1:  
    sub          (8)(%rsp), %ecx
    cmp          $(-16), %ecx
    mov          %r12d, %ecx
    jg           LEncHuffExitEndOfWork00gas_1
LEncHuffWrite11gas_1:  
    sub          $(8), %ebp
    jl           LEncHuffWrite12gas_1
    mov          %r10b, %al
    shr          $(8), %r10
    mov          %al, (%r8,%rcx)
    add          $(1), %ecx
    cmp          $(255), %al
    jne          LEncHuffWrite11gas_1
    xor          %eax, %eax
    mov          %al, (%r8,%rcx)
    add          $(1), %ecx
    jmp          LEncHuffWrite11gas_1
LEncHuffWrite12gas_1:  
    sub          $(56), %ebp
    mov          %rcx, %r12
    neg          %ebp
    mov          (48)(%rsp), %rax
    jmp          *%rax
 
 
LEncHuffExitEndOfWork00gas_1:  
    mov          (24)(%rsp), %rax
    mov          (32)(%rsp), %ecx
    mov          %cx, (%rax)
LEncHuffExitEndOfWork01gas_1:  
    mov          $(1), %eax
    jmp          LEncHuffExit00gas_1
 
LEncHuffExitNormgas_1:  
    cmp          $(56), %ebp
    jle          LEncHuffCallWrite04gas_1
LEncHuffRetWrite04gas_1:  
    mov          (16)(%rsp), %rax
    mov          %r12d, (%rax)
    mov          %ebp, %ecx
    sub          $(64), %ebp
    shl          %cl, %r11
    neg          %ebp
    shr          $(40), %r11
    movq         (136)(%rsp), %rax
    lea          (8)(%rax), %rcx
    mov          %ebp, (%rcx)
    lea          (%rax), %rax
    mov          %r11d, (%rax)
    mov          $(0), %eax
LEncHuffExit00gas_1:  
    add          $(72), %rsp
 
 
    pop          %r15
 
 
    pop          %r14
 
 
    pop          %r13
 
 
    pop          %r12
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
LEncHuffDcLong00gas_1:  
    shr          $(8), %edx
    movzbl       (64)(%rsi,%rdx), %edx
    add          $(8), %edx
    jmp          LEncHuffDc00gas_1
 
LEncHuffAcLong00gas_1:  
    shr          $(8), %edx
    movzbl       (64)(%rsi,%rdx), %edx
    add          $(8), %edx
    jmp          LEncHuffAc03gas_1
 
 
.data
 
.p2align 4, 0x90
 
maskFF:
.quad   0xffffffffffffffff,  0xffffffffffffffff  
 
mask0F:
.quad               0xffff  
 
 
ownTables:
.byte   0,  1,  8, 16,  9,  2,  3, 10  
 

.byte  17, 24, 32, 25, 18, 11,  4,  5  
 

.byte  12, 19, 26, 33, 40, 48, 41, 34  
 

.byte  27, 20, 13,  6,  7, 14, 21, 28  
 

.byte  35, 42, 49, 56, 57, 50, 43, 36  
 

.byte  29, 22, 15, 23, 30, 37, 44, 51  
 

.byte  58, 59, 52, 45, 38, 31, 39, 46  
 

.byte  53, 60, 61, 54, 47, 55, 62, 63  
 
 

.byte  0, 1, 2, 2, 3, 3, 3, 3  
 

.fill 8, 1, 4
 

.fill 16, 1, 5
 

.fill 32, 1, 6
 

.fill 64, 1, 7
 

.fill 128, 1, 8
 
 
