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
 
 
TableCacheSize:  
 
 

.byte   0xec,  0xc3  
 

.byte   0xeb,  0x93  
 

.byte   0x4d,  0x16  
 

.byte   0xea,  0x34  
 

.byte   0x4c,  0x34  
 

.byte   0xe4,  0x15  
 

.byte   0xde,  0x15  
 

.byte   0x4b,  0x15  
 

.byte   0x47,  0x15  
 

.byte   0x4e,  0x33  
 

.byte   0x4a,  0x33  
 

.byte   0xe3,  0x14  
 

.byte   0xdd,  0x14  
 

.byte   0xd8,  0x14  
 

.byte   0x49,  0x14  
 

.byte   0x29,  0x14  
 

.byte   0x46,  0x14  
 

.byte   0x48,  0x32  
 

.byte   0xe2,  0x13  
 

.byte   0xdc,  0x13  
 

.byte   0xd7,  0x13  
 

.byte   0xd2,  0x13  
 

.byte   0x25,  0x13  
 

.byte   0x7d,  0x13  
 

.byte   0x85,  0x13  
 

.byte   0x45,  0x13  
 

.byte   0xd6,  0x12  
 

.byte   0xd1,  0x12  
 

.byte   0x23,  0x12  
 

.byte   0x87,  0x12  
 

.byte   0x7c,  0x12  
 

.byte   0x78,  0x12  
 

.byte   0x84,  0x12  
 

.byte   0x44,  0x12  
 

.byte   0xd0,  0x11  
 

.byte   0x22,  0x11  
 

.byte   0x7b,  0x11  
 

.byte   0x80,  0x11  
 

.byte   0x86,  0x11  
 

.byte   0x3e,  0x11  
 

.byte   0x7f,  0x11  
 

.byte   0x83,  0x11  
 

.byte   0x43,  0x11  
 

.byte  0  
 
 
.text
 
 
.p2align 4, 0x90
 
 
.globl mfxowniCopy8uas

 
mfxowniCopy8uas:
 
 
    push         %rbx
 
 
    push         %rbp
 
 
    movslq       %esi, %rsi
    movslq       %ecx, %rcx
    movslq       %r8d, %r8
    movslq       %r9d, %r9
 
    mov          %r9, %rax
    mov          %r8, %rbp
    imul         %rbp, %rax
 
    cmp          %rsi, %rbp
    jne          LCheckSizeCopy8u_00gas_1
    cmp          %rcx, %rbp
    jne          LCheckSizeCopy8u_00gas_1
 
    mov          $(1), %r9
    mov          %rax, %r8
    mov          %rax, %rbp
 
LCheckSizeCopy8u_00gas_1:  
    cmp          $(64), %rbp
    jl           LShortCopy8u_32gas_1
 
    cmp          $(1048576), %rax
    jge          LGetCacheSize00gas_1
 
    jmp          LAverCopy8u_00gas_1
 
 
LShortCopy8u_32gas_1:  
    mov          %rdi, %r10
    mov          %rdx, %r11
    mov          %r8, %rax
 
    test         $(32), %rax
    jz           LShortCopy8u_16gas_1
 
    movq         (%r10), %xmm0
    movq         (8)(%r10), %xmm1
    movq         (16)(%r10), %xmm2
    movq         (24)(%r10), %xmm3
    add          $(32), %r10
    movq         %xmm0, (%r11)
    movq         %xmm1, (8)(%r11)
    movq         %xmm2, (16)(%r11)
    movq         %xmm3, (24)(%r11)
    add          $(32), %r11
    sub          $(32), %rax
    jz           LEndShortCopy8u_00gas_1
 
LShortCopy8u_16gas_1:  
    test         $(16), %rax
    jz           LShortCopy8u_08gas_1
 
    movq         (%r10), %xmm0
    movq         (8)(%r10), %xmm1
    add          $(16), %r10
    movq         %xmm0, (%r11)
    movq         %xmm1, (8)(%r11)
    add          $(16), %r11
    sub          $(16), %rax
    jz           LEndShortCopy8u_00gas_1
 
LShortCopy8u_08gas_1:  
    test         $(8), %rax
    jz           LShortCopy8u_04gas_1
 
    movq         (%r10), %xmm0
    add          $(8), %r10
    movq         %xmm0, (%r11)
    add          $(8), %r11
    sub          $(8), %rax
    jz           LEndShortCopy8u_00gas_1
 
LShortCopy8u_04gas_1:  
    test         $(4), %rax
    jz           LShortCopy8u_03gas_1
 
    mov          (%r10), %ebx
    add          $(4), %r10
    mov          %ebx, (%r11)
    add          $(4), %r11
    sub          $(4), %rax
 
LShortCopy8u_03gas_1:  
    test         %rax, %rax
    jz           LEndShortCopy8u_00gas_1
 
    movzbl       (-1)(%r10,%rax), %ebx
    mov          %bl, (-1)(%r11,%rax)
    sub          $(1), %rax
    jz           LEndShortCopy8u_00gas_1
    movzbl       (-1)(%r10,%rax), %ebx
    mov          %bl, (-1)(%r11,%rax)
    sub          $(1), %rax
    jz           LEndShortCopy8u_00gas_1
    movzbl       (-1)(%r10,%rax), %ebx
    mov          %bl, (-1)(%r11,%rax)
 
 
LEndShortCopy8u_00gas_1:  
    sub          $(1), %r9
    jz           LExitCopy8u_00gas_1
 
    add          %rsi, %rdi
    add          %rcx, %rdx
    jmp          LShortCopy8u_32gas_1
 
 
LExitCopy8u_00gas_1:  
 
 
    pop          %rbp
 
 
    pop          %rbx
 
    ret
 
 
LAverCopy8u_00gas_1:  
    mov          %rdi, %r10
    mov          %rdx, %r11
    mov          %r8, %rax
 
    test         $(15), %r11
    jnz          LAlignCopy8u_00gas_1
 
    test         $(15), %r10
    jnz          LCheckAlignCopy8u_00gas_1
 
 
LCheckAliasingCopy8u_00gas_1:  
    mov          %r10, %rbx
    mov          %r11, %rbp
    and          $(4095), %ebx
    and          $(4095), %ebp
    sub          %ebp, %ebx
    cmp          $(3936), %ebx
    jge          LInv00Copy8u_00gas_1
    cmp          $(0), %ebx
    jge          LFwd00Copy8u_00gas_1
    cmp          $(-160), %ebx
    jge          LInv00Copy8u_00gas_1
 
 
LFwd00Copy8u_00gas_1:  
    sub          $(64), %rax
 
.p2align 4, 0x90
LFwd00Copy8u_01gas_1:  
    movdqa       (%r10), %xmm0
    movdqa       %xmm0, (%r11)
    movdqa       (16)(%r10), %xmm0
    movdqa       %xmm0, (16)(%r11)
    movdqa       (32)(%r10), %xmm0
    movdqa       %xmm0, (32)(%r11)
    movdqa       (48)(%r10), %xmm0
    movdqa       %xmm0, (48)(%r11)
    add          $(64), %r10
    add          $(64), %r11
    sub          $(64), %rax
    jge          LFwd00Copy8u_01gas_1
 
    add          $(64), %rax
    jz           LEndAverCopy8u_00gas_1
 
LShortFwd00Copy8u_32gas_1:  
    test         $(32), %rax
    jz           LShortFwd00Copy8u_16gas_1
 
    movdqa       (%r10), %xmm0
    movdqa       (16)(%r10), %xmm1
    add          $(32), %r10
    movdqa       %xmm0, (%r11)
    movdqa       %xmm1, (16)(%r11)
    add          $(32), %r11
    sub          $(32), %rax
    jz           LEndAverCopy8u_00gas_1
 
LShortFwd00Copy8u_16gas_1:  
    test         $(16), %rax
    jz           LShortFwd00Copy8u_08gas_1
 
    movdqa       (%r10), %xmm0
    movdqa       %xmm0, (%r11)
    add          $(16), %r10
    add          $(16), %r11
    sub          $(16), %rax
    jz           LEndAverCopy8u_00gas_1
 
LShortFwd00Copy8u_08gas_1:  
    test         $(8), %rax
    jz           LShortFwd00Copy8u_04gas_1
 
    movq         (%r10), %xmm0
    movq         %xmm0, (%r11)
    add          $(8), %r10
    add          $(8), %r11
    sub          $(8), %rax
    jz           LEndAverCopy8u_00gas_1
 
LShortFwd00Copy8u_04gas_1:  
    test         $(4), %rax
    jz           LShortFwd00Copy8u_02gas_1
 
    mov          (%r10), %ebx
    mov          %ebx, (%r11)
    add          $(4), %r10
    add          $(4), %r11
    sub          $(4), %rax
    jz           LEndAverCopy8u_00gas_1
 
LShortFwd00Copy8u_02gas_1:  
    test         $(2), %rax
    jz           LShortFwd00Copy8u_01gas_1
 
    movzwl       (%r10), %ebx
    mov          %bx, (%r11)
    add          $(2), %r10
    add          $(2), %r11
    sub          $(2), %rax
    jz           LEndAverCopy8u_00gas_1
 
LShortFwd00Copy8u_01gas_1:  
    test         %rax, %rax
    jz           LEndAverCopy8u_00gas_1
 
    movzbl       (%r10), %ebx
    mov          %bl, (%r11)
 
 
LEndAverCopy8u_00gas_1:  
    sub          $(1), %r9
    jz           LExitAverCopy8u_00gas_1
 
    add          %rsi, %rdi
    add          %rcx, %rdx
    jmp          LAverCopy8u_00gas_1
 
 
LExitAverCopy8u_00gas_1:  
 
 
    pop          %rbp
 
 
    pop          %rbx
 
    ret
 
 
LInv00Copy8u_00gas_1:  
    test         $(1), %rax
    jz           LInv00Copy8u_01gas_1
 
    movzbl       (-1)(%r10,%rax), %ebx
    mov          %bl, (-1)(%r11,%rax)
    sub          $(1), %rax
 
LInv00Copy8u_01gas_1:  
    test         $(2), %rax
    jz           LInv00Copy8u_02gas_1
 
    movzwl       (-2)(%r10,%rax), %ebx
    mov          %bx, (-2)(%r11,%rax)
    sub          $(2), %rax
 
LInv00Copy8u_02gas_1:  
    test         $(4), %rax
    jz           LInv00Copy8u_03gas_1
 
    movl         (-4)(%r10,%rax), %ebx
    mov          %ebx, (-4)(%r11,%rax)
    sub          $(4), %rax
 
LInv00Copy8u_03gas_1:  
    test         $(8), %rax
    jz           LInv00Copy8u_04gas_1
 
    movq         (-8)(%r10,%rax), %xmm0
    movq         %xmm0, (-8)(%r11,%rax)
    sub          $(8), %rax
 
LInv00Copy8u_04gas_1:  
    test         $(16), %rax
    jz           LInv00Copy8u_05gas_1
 
    movdqa       (-16)(%r10,%rax), %xmm0
    movdqa       %xmm0, (-16)(%r11,%rax)
    sub          $(16), %rax
 
LInv00Copy8u_05gas_1:  
    test         $(32), %rax
    jz           LInv00Copy8u_06gas_1
 
    movdqa       (-16)(%r10,%rax), %xmm0
    movdqa       (-32)(%r10,%rax), %xmm1
    movdqa       %xmm0, (-16)(%r11,%rax)
    movdqa       %xmm1, (-32)(%r11,%rax)
    sub          $(32), %rax
    jz           LEndAverCopy8u_00gas_1
    jmp          LInv00Copy8u_06gas_1
 
.p2align 4, 0x90
LInv00Copy8u_06gas_1:  
    movdqa       (-16)(%r10,%rax), %xmm0
    movdqa       %xmm0, (-16)(%r11,%rax)
    movdqa       (-32)(%r10,%rax), %xmm0
    movdqa       %xmm0, (-32)(%r11,%rax)
    movdqa       (-48)(%r10,%rax), %xmm0
    movdqa       %xmm0, (-48)(%r11,%rax)
    movdqa       (-64)(%r10,%rax), %xmm0
    movdqa       %xmm0, (-64)(%r11,%rax)
    sub          $(64), %rax
    jnz          LInv00Copy8u_06gas_1
 
    jmp          LEndAverCopy8u_00gas_1
 
 
LAlignCopy8u_00gas_1:  
    movdqu       (%r10), %xmm0
    mov          %r11, %rbx
    and          $(15), %rbx
    movdqu       %xmm0, (%r11)
    sub          $(16), %rbx
    sub          %rbx, %r10
    sub          %rbx, %r11
    add          %rbx, %rax
 
    cmp          $(64), %rax
    jge          LAlignCopy8u_01gas_1
 
    test         $(15), %r10
    jnz          LShortX0Copy8u_32gas_1
    jmp          LShortFwd00Copy8u_32gas_1
 
LAlignCopy8u_01gas_1:  
    test         $(15), %r10
    jz           LCheckAliasingCopy8u_00gas_1
 
 
LCheckAlignCopy8u_00gas_1:  
    test         $(3), %r10
    jz           LCheckCopy8u_03gas_1
 
LLoopX0Copy8u_00gas_1:  
    sub          $(64), %rax
 
LLoopX0Copy8u_01gas_1:  
    movdqu       (%r10), %xmm0
    movdqu       (16)(%r10), %xmm1
    movdqu       (32)(%r10), %xmm2
    movdqu       (48)(%r10), %xmm3
    add          $(64), %r10
    movdqa       %xmm0, (%r11)
    movdqa       %xmm1, (16)(%r11)
    movdqa       %xmm2, (32)(%r11)
    movdqa       %xmm3, (48)(%r11)
    add          $(64), %r11
    sub          $(64), %rax
    jge          LLoopX0Copy8u_01gas_1
 
    add          $(64), %rax
    jnz          LShortX0Copy8u_32gas_1
 
    jmp          LEndAverCopy8u_00gas_1
 
LCheckCopy8u_03gas_1:  
    movdqu       (%r10), %xmm0
    movdqa       %xmm0, (%r11)
    add          $(16), %r10
    add          $(16), %r11
    sub          $(32), %rax
 
    mov          $(-16), %rbx
    mov          %r10, %rbp
    and          %r10, %rbx
    and          $(15), %rbp
 
    cmp          $(12), %rbp
    je           LLoop12Copy8u_00gas_1
 
    cmp          $(8), %rbp
    je           LLoop08Copy8u_00gas_1
 
    mov          %r11, %rbp
    sub          $(64), %rax
    jl           LEndNotAlignCopy_00gas_1
 
LLoop04Copy8u_00gas_1:  
    movdqa       (%rbx), %xmm0
    movdqa       (16)(%rbx), %xmm1
    movdqa       (32)(%rbx), %xmm2
    movdqa       (48)(%rbx), %xmm3
    movdqa       (64)(%rbx), %xmm4
    add          $(64), %rbx
    palignr      $(4), %xmm3, %xmm4
    palignr      $(4), %xmm2, %xmm3
    palignr      $(4), %xmm1, %xmm2
    palignr      $(4), %xmm0, %xmm1
    movdqa       %xmm1, (%r11)
    movdqa       %xmm2, (16)(%r11)
    movdqa       %xmm3, (32)(%r11)
    movdqa       %xmm4, (48)(%r11)
    add          $(64), %r11
    sub          $(64), %rax
    jge          LLoop04Copy8u_00gas_1
 
    jmp          LEndNotAlignCopy_00gas_1
 
LLoop08Copy8u_00gas_1:  
    mov          %r11, %rbp
    sub          $(64), %rax
    jl           LEndNotAlignCopy_00gas_1
 
LLoop08Copy8u_01gas_1:  
    movdqa       (%rbx), %xmm0
    movdqa       (16)(%rbx), %xmm1
    movdqa       (32)(%rbx), %xmm2
    movdqa       (48)(%rbx), %xmm3
    movdqa       (64)(%rbx), %xmm4
    add          $(64), %rbx
    palignr      $(8), %xmm3, %xmm4
    palignr      $(8), %xmm2, %xmm3
    palignr      $(8), %xmm1, %xmm2
    palignr      $(8), %xmm0, %xmm1
    movdqa       %xmm1, (%r11)
    movdqa       %xmm2, (16)(%r11)
    movdqa       %xmm3, (32)(%r11)
    movdqa       %xmm4, (48)(%r11)
    add          $(64), %r11
    sub          $(64), %rax
    jge          LLoop08Copy8u_01gas_1
 
    jmp          LEndNotAlignCopy_00gas_1
 
LLoop12Copy8u_00gas_1:  
    mov          %r11, %rbp
    sub          $(64), %rax
    jl           LEndNotAlignCopy_00gas_1
 
LLoop12Copy8u_01gas_1:  
    movdqa       (%rbx), %xmm0
    movdqa       (16)(%rbx), %xmm1
    movdqa       (32)(%rbx), %xmm2
    movdqa       (48)(%rbx), %xmm3
    movdqa       (64)(%rbx), %xmm4
    add          $(64), %rbx
    palignr      $(12), %xmm3, %xmm4
    palignr      $(12), %xmm2, %xmm3
    palignr      $(12), %xmm1, %xmm2
    palignr      $(12), %xmm0, %xmm1
    movdqa       %xmm1, (%r11)
    movdqa       %xmm2, (16)(%r11)
    movdqa       %xmm3, (32)(%r11)
    movdqa       %xmm4, (48)(%r11)
    add          $(64), %r11
    sub          $(64), %rax
    jge          LLoop12Copy8u_01gas_1
 
LEndNotAlignCopy_00gas_1:  
    add          $(80), %rax
    sub          %r11, %rbp
    sub          %rbp, %r10
    cmp          $(64), %rax
    jge          LLoopX0Copy8u_00gas_1
 
 
LShortX0Copy8u_32gas_1:  
    test         $(32), %rax
    jz           LShortX0Copy8u_16gas_1
 
    movdqu       (%r10), %xmm0
    movdqu       (16)(%r10), %xmm1
    add          $(32), %r10
    movdqa       %xmm0, (%r11)
    movdqa       %xmm1, (16)(%r11)
    add          $(32), %r11
    sub          $(32), %rax
    jz           LEndAverCopy8u_00gas_1
 
LShortX0Copy8u_16gas_1:  
    test         $(16), %rax
    jz           LShortX0Copy8u_08gas_1
 
    movdqu       (%r10), %xmm0
    movdqa       %xmm0, (%r11)
    add          $(16), %r10
    add          $(16), %r11
    sub          $(16), %rax
    jz           LEndAverCopy8u_00gas_1
 
LShortX0Copy8u_08gas_1:  
    test         $(8), %rax
    jz           LShortX0Copy8u_04gas_1
 
    movq         (%r10), %xmm0
    movq         %xmm0, (%r11)
    add          $(8), %r10
    add          $(8), %r11
    sub          $(8), %rax
    jz           LEndAverCopy8u_00gas_1
 
LShortX0Copy8u_04gas_1:  
    test         $(4), %rax
    jz           LShortX0Copy8u_02gas_1
 
    mov          (%r10), %ebx
    mov          %ebx, (%r11)
    add          $(4), %r10
    add          $(4), %r11
    sub          $(4), %rax
    jz           LEndAverCopy8u_00gas_1
 
LShortX0Copy8u_02gas_1:  
    test         $(2), %rax
    jz           LShortX0Copy8u_01gas_1
 
    movzwl       (%r10), %ebx
    mov          %bx, (%r11)
    add          $(2), %r10
    add          $(2), %r11
    sub          $(2), %rax
    jz           LEndAverCopy8u_00gas_1
 
LShortX0Copy8u_01gas_1:  
    test         %rax, %rax
    jz           LEndAverCopy8u_00gas_1
 
    movzbl       (%r10), %ebx
    mov          %bl, (%r11)
 
    jmp          LEndAverCopy8u_00gas_1
 
 
LGetCacheSize00gas_1:  
 
    lea          TableCacheSize(%rip), %rbx
 
    sub          $(64), %rsp
    mov          %rax, (16)(%rsp)
    mov          %rbx, (24)(%rsp)
    mov          %rcx, (32)(%rsp)
    mov          %rdx, (40)(%rsp)
    mov          %r8, (48)(%rsp)
    mov          %rbx, (56)(%rsp)
 
    xor          %eax, %eax
    cpuid
 
    cmp          $(1970169159), %ebx
    jne          LCacheSizeMacro11gas_1
    cmp          $(1231384169), %edx
    jne          LCacheSizeMacro11gas_1
    cmp          $(1818588270), %ecx
    jne          LCacheSizeMacro11gas_1
 
    mov          $(2), %eax
    cpuid
 
    cmp          $(1), %al
    jne          LCacheSizeMacro11gas_1
 
    test         $(2147483648), %eax
    jz           LCacheSizeMacro00gas_1
    xor          %eax, %eax
LCacheSizeMacro00gas_1:  
    test         $(2147483648), %ebx
    jz           LCacheSizeMacro01gas_1
    xor          %ebx, %ebx
LCacheSizeMacro01gas_1:  
    test         $(2147483648), %ecx
    jz           LCacheSizeMacro02gas_1
    xor          %ecx, %ecx
LCacheSizeMacro02gas_1:  
    test         $(2147483648), %edx
    jz           LCacheSizeMacro03gas_1
    xor          %edx, %edx
 
LCacheSizeMacro03gas_1:  
    mov          %rsp, %r8
    test         %eax, %eax
    jz           LCacheSizeMacro04gas_1
    mov          %eax, (%r8)
    add          $(4), %r8
    mov          $(3), %eax
LCacheSizeMacro04gas_1:  
    test         %ebx, %ebx
    jz           LCacheSizeMacro05gas_1
    mov          %ebx, (%r8)
    add          $(4), %r8
    add          $(4), %eax
LCacheSizeMacro05gas_1:  
    test         %ecx, %ecx
    jz           LCacheSizeMacro06gas_1
    mov          %ecx, (%r8)
    add          $(4), %r8
    add          $(4), %eax
LCacheSizeMacro06gas_1:  
    test         %edx, %edx
    jz           LCacheSizeMacro07gas_1
    mov          %edx, (%r8)
    add          $(4), %eax
 
LCacheSizeMacro07gas_1:  
    mov          (56)(%rsp), %rbx
 
    test         %eax, %eax
    jz           LCacheSizeMacro11gas_1
LCacheSizeMacro08gas_1:  
    movzbl       (%rbx), %edx
    test         %edx, %edx
    jz           LCacheSizeMacro11gas_1
    add          $(2), %rbx
    mov          %eax, %ecx
LCacheSizeMacro09gas_1:  
    cmpb         (%rsp,%rcx), %dl
    je           LCacheSizeMacro10gas_1
    sub          $(1), %ecx
    jnz          LCacheSizeMacro09gas_1
    jmp          LCacheSizeMacro08gas_1
 
LCacheSizeMacro10gas_1:  
    movzbl       (-1)(%rbx), %ebx
    mov          %ebx, %ecx
    shr          $(4), %ebx
    and          $(15), %ecx
    add          $(18), %ecx
    shl          %cl, %rbx
    mov          %rbx, (56)(%rsp)
    jmp          LCacheSizeMacro12gas_1
 
LCacheSizeMacro11gas_1:  
    movq         $(-1), (56)(%rsp)
 
LCacheSizeMacro12gas_1:  
    mov          (16)(%rsp), %rax
    mov          (24)(%rsp), %rbx
    mov          (32)(%rsp), %rcx
    mov          (40)(%rsp), %rdx
    mov          (48)(%rsp), %r8
    mov          (56)(%rsp), %rbx
    add          $(64), %rsp
 
 
    cmp          $(-1), %rbx
    je           LAverCopy8u_00gas_1
 
    shr          $(1), %rbx
 
    cmp          %rbx, %rax
    jl           LAverCopy8u_00gas_1
 
 
LLongCopy8u_00gas_1:  
    mov          %rdi, %r10
    mov          %rdx, %r11
    mov          %r8, %rax
 
LAlignLongCopy8u_00gas_1:  
    test         $(15), %r11
    jz           LPrefLoopCopy8u_00gas_1
 
    movzbl       (%r10), %ebx
    add          $(1), %r10
    mov          %bl, (%r11)
    add          $(1), %r11
    sub          $(1), %rax
    jz           LEndLongCopy8u_00gas_1
    jmp          LAlignLongCopy8u_00gas_1
 
 
LPrefLoopCopy8u_00gas_1:  
    sub          $(262144), %rax
    jl           LPrefLoopCopy8u_02gas_1
 
    mov          $(4096), %rbp
 
LPrefLoopCopy8u_01gas_1:  
    movzbl       (%r10), %ebx
    add          $(64), %r10
    sub          $(1), %rbp
    jnz          LPrefLoopCopy8u_01gas_1
 
    sub          $(262144), %r10
    mov          $(262144), %rbp
 
 
LLoopCopy8uNt_00gas_1:  
    movdqu       (%r10), %xmm0
    add          $(16), %r10
    movntdq      %xmm0, (%r11)
    add          $(16), %r11
    sub          $(16), %rbp
    jnz          LLoopCopy8uNt_00gas_1
 
    jmp          LPrefLoopCopy8u_00gas_1
 
LPrefLoopCopy8u_02gas_1:  
    add          $(262144), %rax
    jz           LEndLongCopy8u_00gas_1
 
 
    mov          %rax, %rbp
    jmp          LPrefLoopCopy8u_04gas_1
LPrefLoopCopy8u_03gas_1:  
    movzbl       (%r10,%rbp), %ebx
LPrefLoopCopy8u_04gas_1:  
    sub          $(64), %rbp
    jge          LPrefLoopCopy8u_03gas_1
 
 
    sub          $(16), %rax
    jl           LTailLongCopy8u_00gas_1
 
LLoopCopy8uNt_01gas_1:  
    movdqu       (%r10), %xmm0
    add          $(16), %r10
    movntdq      %xmm0, (%r11)
    add          $(16), %r11
    sub          $(16), %rax
    jge          LLoopCopy8uNt_01gas_1
 
LTailLongCopy8u_00gas_1:  
    add          $(16), %rax
    jz           LEndLongCopy8u_00gas_1
 
LTailLongCopy8u_01gas_1:  
    movzbl       (%r10), %ebx
    add          $(1), %r10
    mov          %bl, (%r11)
    add          $(1), %r11
    sub          $(1), %rax
    jnz          LTailLongCopy8u_01gas_1
 
 
LEndLongCopy8u_00gas_1:  
    sub          $(1), %r9
    jz           LExitCopy8uNt_00gas_1
 
    add          %rsi, %rdi
    add          %rcx, %rdx
    jmp          LLongCopy8u_00gas_1
 
 
LExitCopy8uNt_00gas_1:  
    sfence
 
 
    pop          %rbp
 
 
    pop          %rbx
 
    ret
 
 
