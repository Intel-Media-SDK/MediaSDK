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
 
maskFF:
.quad   0xffffffffffffffff,  0xffffffffffffffff  
 
 
ownTables:
.byte  63, 62, 55, 47, 54, 61, 60, 53  
 

.byte  46, 39, 31, 38, 45, 52, 59, 58  
 

.byte  51, 44, 37, 30, 23, 15, 22, 29  
 

.byte  36, 43, 50, 57, 56, 49, 42, 35  
 

.byte  28, 21, 14,  7,  6, 13, 20, 27  
 

.byte  34, 41, 48, 40, 33, 26, 19, 12  
 

.byte   5,  4, 11, 18, 25, 32, 24, 17  
 

.byte  10,  3,  2,  9, 16,  8,  1,  0  
 
 

.long  0, -1, -3, -7  
 

.long  -15, -31, -63, -127  
 

.long  -255, -511, -1023, -2047  
 

.long  -4095, -8191, -16383, -32767  
 
 
.text
 
 
.p2align 4, 0x90
 
 
.globl mfxownpj_DecodeHuffman8x8_JPEG_1u16s_C1

 
mfxownpj_DecodeHuffman8x8_JPEG_1u16s_C1:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    push         %r12
 
 
    push         %r13
 
 
    push         %r14
 
 
    push         %r15
 
 
    sub          $(56), %rsp
 
 
    mov          %rdi, (%rsp)
    mov          %esi, %esi
    mov          %rsi, (8)(%rsp)
    mov          %rdx, (16)(%rsp)
    mov          %rcx, %r13
    mov          %r8, (24)(%rsp)
 
    mov          (%r9), %eax
    test         %eax, %eax
    jnz          LDecHuffExitEndOfWork01gas_1
 
 
    pxor         %xmm0, %xmm0
    mov          %r13, %rax
    mov          %r13, %rdi
    and          $(15), %rax
    jnz          LDecHuffInit01gas_1
    movq         (128)(%rsp), %rax
    lea          (8)(%rax), %rcx
    movdqa       %xmm0, (%rdi)
    mov          (%rcx), %ebp
    movd         %xmm0, (8)(%rcx)
    lea          (%rax), %rax
    movdqa       %xmm0, (16)(%rdi)
    mov          (%rax), %r11
    mov          $(64), %ecx
    movdqa       %xmm0, (32)(%rdi)
    sub          %ebp, %ecx
    movdqa       %xmm0, (48)(%rdi)
    shl          %cl, %r11
    movdqa       %xmm0, (64)(%rdi)
    mov          (%rdx), %edx
    movdqa       %xmm0, (80)(%rdi)
    mov          %rdx, %r12
    sub          (8)(%rsp), %rdx
    movdqa       %xmm0, (96)(%rdi)
    cmp          $(-24), %rdx
    jg           LDecHuffExitEndOfWork01gas_1
    movdqa       %xmm0, (112)(%rdi)
    movswl       (%r8), %eax
    mov          %eax, (32)(%rsp)
    lea          ownTables(%rip), %rsi
 
LDecHuffInit00gas_1:  
    cmp          $(8), %ebp
    jl           LDecHuffDcCall00gas_1
 
 
LDecHuffDc00gas_1:  
    movq         (112)(%rsp), %rax
    lea          (512)(%rax), %rax
    mov          %r11, %rcx
    shr          $(56), %rcx
    mov          (%rax,%rcx,4), %ecx
    test         $(4294901760), %ecx
    jz           LDecHuffDcLong00gas_1
    mov          %ecx, %ebx
    shr          $(16), %ecx
    sub          %ecx, %ebp
    shl          %cl, %r11
LDecHuffDcRetLonggas_1:  
    and          $(15), %ebx
    cmove        %ebx, %edx
    jz           LDecHuffDc02gas_1
    cmp          %ebx, %ebp
    jl           LDecHuffDcCall01gas_1
LDecHuffDc01gas_1:  
    mov          %ebx, %ecx
    sub          %ebx, %ebp
    mov          %r11, %rdx
    shl          %cl, %r11
    mov          $(16), %ecx
    shr          $(48), %rdx
    sub          %ebx, %ecx
    xor          %eax, %eax
    test         $(32768), %edx
    cmove        (64)(%rsi,%rbx,4), %eax
    shr          %cl, %edx
    add          %eax, %edx
LDecHuffDc02gas_1:  
    mov          (24)(%rsp), %rax
    addw         (%rax), %dx
    mov          %dx, (%rax)
    mov          %dx, (%r13)
 
 
    movq         (120)(%rsp), %r14
    lea          (512)(%r14), %r14
    mov          $(62), %edi
    mov          %r11, %rcx
    shr          $(56), %rcx
 
LDecHuffAc00gas_1:  
    cmp          $(8), %ebp
    jl           LDecHuffAcCall00gas_1
LDecHuffAc01gas_1:  
    mov          (%r14,%rcx,4), %ecx
    test         $(4294901760), %ecx
    jz           LDecHuffAcLong00gas_1
    mov          %ecx, %ebx
    shr          $(16), %ecx
    sub          %ecx, %ebp
    shl          %cl, %r11
LDecHuffAcRetLonggas_1:  
    mov          %ebx, %eax
    shr          $(4), %eax
    and          $(15), %eax
    and          $(15), %ebx
    jz           LDecHuffExitNorm00gas_1
    sub          %eax, %edi
    jl           LDecHuffExitEndOfWork00gas_1
    cmp          %ebx, %ebp
    jl           LDecHuffAcCall01gas_1
LDecHuffAc02gas_1:  
    mov          %ebx, %ecx
    mov          %r11, %rdx
    shl          %cl, %r11
    mov          $(16), %ecx
    shr          $(48), %rdx
    sub          %rbx, %rbp
    sub          %rbx, %rcx
    xor          %rax, %rax
    test         $(32768), %edx
    cmove        (64)(%rsi,%rbx,4), %eax
    movzbq       (%rdi,%rsi), %rbx
    shr          %cl, %edx
    mov          %r11, %rcx
    add          %eax, %edx
    shr          $(56), %rcx
    mov          %dx, (%r13,%rbx,2)
    sub          $(1), %rdi
    jge          LDecHuffAc00gas_1
    jmp          LDecHuffExitNorm01gas_1
 
 
LDecHuffExitNorm00gas_1:  
    cmp          $(15), %eax
    je           LDecHuffAc04gas_1
LDecHuffExitNorm01gas_1:  
    movq         (128)(%rsp), %rcx
    mov          $(63), %edx
    sub          %edi, %edx
    mov          $(0), %eax
    mov          %edx, (16)(%rcx)
 
LDecHuffExit00gas_1:  
    mov          $(64), %ecx
    sub          %ebp, %ecx
    shr          %cl, %r11
    cmp          $(32), %ebp
    jl           LDecHuffExit01gas_1
 
    lea          (7)(%rbp), %rcx
    and          $(4294967288), %ecx
    sub          $(32), %ecx
    shr          %cl, %r11
    sub          %ecx, %ebp
    shr          $(3), %ecx
    sub          %rcx, %r12
LDecHuffExit01gas_1:  
    mov          (16)(%rsp), %rdx
    mov          %r12d, (%rdx)
    movq         (128)(%rsp), %rdx
    lea          (8)(%rdx), %rcx
    mov          %ebp, (%rcx)
    lea          (%rdx), %rcx
    mov          %r11, (%rcx)
LDecHuffExit02gas_1:  
    add          $(56), %rsp
 
 
    pop          %r15
 
 
    pop          %r14
 
 
    pop          %r13
 
 
    pop          %r12
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
LDecHuffAc04gas_1:  
    mov          %r11, %rcx
    shr          $(56), %rcx
    sub          $(16), %edi
    jge          LDecHuffAc00gas_1
    jmp          LDecHuffExitEndOfWork00gas_1
 
 
LDecHuffExitEndOfWork00gas_1:  
    mov          (24)(%rsp), %rax
    mov          (32)(%rsp), %ecx
    mov          %cx, (%rax)
LDecHuffExitEndOfWork01gas_1:  
    mov          $(1), %eax
    jmp          LDecHuffExit02gas_1
 
 
LDecHuffAcCall00gas_1:  
    lea          LDecHuffAc01gas_1(%rip), %r8
 
LDecHuffCheckLen00gas_1:  
    mov          (8)(%rsp), %rcx
    mov          %r12, %rax
    sub          %r12, %rcx
    add          (%rsp), %rax
    cmp          $(8), %ecx
    jl           LDecHuffExitEndOfWork00gas_1
 
    mov          (%rax), %r10
    bswap        %r10
    movd         %r10, %xmm1
    pcmpeqb      maskFF(%rip), %xmm1
    mov          $(64), %ecx
    sub          %ebp, %ecx
    shr          %cl, %r11
    movd         %xmm1, %rcx
    test         %rcx, %rcx
    jnz          LDecHuffInfill10gas_1
    lea          (7)(%rbp), %rcx
    and          $(4294967288), %ecx
    mov          $(64), %edx
    sub          %ecx, %edx
    shr          %cl, %r10
    mov          %edx, %ecx
    add          %edx, %ebp
    shr          $(3), %edx
    shl          %cl, %r11
    mov          $(64), %ecx
    or           %r10, %r11
    sub          %ebp, %ecx
    add          %rdx, %r12
    shl          %cl, %r11
    mov          %r11, %rcx
    shr          $(56), %rcx
    jmp          *%r8
 
 
LDecHuffDcCall00gas_1:  
    lea          LDecHuffDc00gas_1(%rip), %r8
    jmp          LDecHuffCheckLen00gas_1
 
LDecHuffAcCall01gas_1:  
    lea          LDecHuffAc02gas_1(%rip), %r8
    jmp          LDecHuffCheckLen00gas_1
 
LDecHuffDcCall01gas_1:  
    lea          LDecHuffDc01gas_1(%rip), %r8
    jmp          LDecHuffCheckLen00gas_1
 
LDecHuffLongCall00gas_1:  
    lea          LDecHuffLong01gas_1(%rip), %r8
    jmp          LDecHuffCheckLen00gas_1
 
 
LDecHuffInfill10gas_1:  
    mov          %r11, %rcx
 
LDecHuffInfill11gas_1:  
    cmp          $(24), %ebp
    jg           LDecHuffInfill13gas_1
 
    movzbl       (%rax), %edx
    add          $(1), %rax
    cmp          $(255), %edx
    jne          LDecHuffInfill12gas_1
    movzbl       (%rax), %edx
    add          $(1), %rax
    test         $(255), %edx
    jnz          LDecHuffExitEndOfWork00gas_1
    mov          $(255), %edx
 
LDecHuffInfill12gas_1:  
    shl          $(8), %ecx
    or           %edx, %ecx
    add          $(8), %ebp
    jmp          LDecHuffInfill11gas_1
 
LDecHuffInfill13gas_1:  
    sub          (%rsp), %rax
    mov          %rax, %r12
    mov          %rcx, %r11
    mov          $(64), %ecx
    sub          %ebp, %ecx
    shl          %cl, %r11
    mov          %r11, %rcx
    shr          $(56), %rcx
    jmp          *%r8
 
 
LDecHuffDcLong00gas_1:  
    lea          LDecHuffDcRetLonggas_1(%rip), %r9
    movq         (112)(%rsp), %r15
    jmp          LDecHuffLong00gas_1
 
LDecHuffAcLong00gas_1:  
    lea          LDecHuffAcRetLonggas_1(%rip), %r9
    movq         (120)(%rsp), %r15
 
LDecHuffLong00gas_1:  
    mov          $(9), %ebx
    cmp          $(16), %ebp
    jl           LDecHuffLongCall00gas_1
LDecHuffLong01gas_1:  
    lea          (1572)(%r15), %rdx
LDecHuffLong02gas_1:  
    mov          $(64), %ecx
    sub          %ebx, %ecx
    mov          %r11, %r10
    movswl       (%rdx,%rbx,2), %eax
    shr          %cl, %r10
    test         $(32768), %eax
    jz           LDecHuffLong03gas_1
    cmp          $(-1), %eax
    je           LDecHuffLong03gas_1
    movzwl       (%rdx,%rbx,2), %eax
LDecHuffLong03gas_1:  
    cmp          %eax, %r10d
    jle          LDecHuffLong04gas_1
    add          $(1), %ebx
    cmp          $(16), %ebx
    jg           LDecHuffExitEndOfWork00gas_1
    jmp          LDecHuffLong02gas_1
 
LDecHuffLong04gas_1:  
    mov          %ebx, %ecx
    shl          %cl, %r11
    sub          %ebx, %ebp
    lea          (1536)(%r15), %rdx
    movzwl       (%rdx,%rbx,2), %eax
    sub          %eax, %r10d
    lea          (1608)(%r15), %rdx
    movzwl       (%rdx,%rbx,2), %eax
    add          %eax, %r10d
    lea          (%r15), %rdx
    movzwl       (%rdx,%r10,2), %ebx
    jmp          *%r9
 
 
LDecHuffInit01gas_1:  
    mov          $(16), %ebx
    sub          %rax, %rbx
    add          %r13, %rbx
    movq         (128)(%rsp), %rax
    movdqu       %xmm0, (%rdi)
    lea          (8)(%rax), %rcx
    movdqa       %xmm0, (%rbx)
    mov          (%rcx), %ebp
    movd         %xmm0, (8)(%rcx)
    lea          (%rax), %rax
    movdqa       %xmm0, (16)(%rbx)
    mov          (%rax), %r11
    mov          $(64), %ecx
    movdqa       %xmm0, (32)(%rbx)
    sub          %ebp, %ecx
    movdqa       %xmm0, (48)(%rbx)
    shl          %cl, %r11
    movdqa       %xmm0, (64)(%rbx)
    mov          (%rdx), %edx
    movdqa       %xmm0, (80)(%rbx)
    mov          %rdx, %r12
    sub          (8)(%rsp), %rdx
    movdqa       %xmm0, (96)(%rbx)
    cmp          $(-24), %rdx
    jg           LDecHuffExitEndOfWork01gas_1
    movswl       (%r8), %eax
    movdqu       %xmm0, (112)(%rdi)
    mov          %eax, (32)(%rsp)
    lea          ownTables(%rip), %rsi
    jmp          LDecHuffInit00gas_1
 
 
