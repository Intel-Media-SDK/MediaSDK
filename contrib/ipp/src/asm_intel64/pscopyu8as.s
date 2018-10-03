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
 
 
.globl mfxownsCopy_8u

 
mfxownsCopy_8u:
 
 
    push         %rbx
 
 
    mov          %rsi, %r9
    movslq       %edx, %r10
 
    cmp          $(64), %r10
    jge          LLongCopy8u_00gas_1
 
 
LShortCopy8u_32gas_1:  
    test         $(32), %r10
    jz           LShortCopy8u_16gas_1
 
    movq         (%rdi), %xmm0
    movq         (8)(%rdi), %xmm1
    movq         (16)(%rdi), %xmm2
    movq         (24)(%rdi), %xmm3
    add          $(32), %rdi
    movq         %xmm0, (%r9)
    movq         %xmm1, (8)(%r9)
    movq         %xmm2, (16)(%r9)
    movq         %xmm3, (24)(%r9)
    add          $(32), %r9
    sub          $(32), %r10
    jz           LExitCopy8u_00gas_1
 
LShortCopy8u_16gas_1:  
    test         $(16), %r10
    jz           LShortCopy8u_08gas_1
 
    movq         (%rdi), %xmm0
    movq         (8)(%rdi), %xmm1
    add          $(16), %rdi
    movq         %xmm0, (%r9)
    movq         %xmm1, (8)(%r9)
    add          $(16), %r9
    sub          $(16), %r10
    jz           LExitCopy8u_00gas_1
 
LShortCopy8u_08gas_1:  
    test         $(8), %r10
    jz           LShortCopy8u_04gas_1
 
    movq         (%rdi), %xmm0
    add          $(8), %rdi
    movq         %xmm0, (%r9)
    add          $(8), %r9
    sub          $(8), %r10
    jz           LExitCopy8u_00gas_1
 
LShortCopy8u_04gas_1:  
    test         $(4), %r10
    jz           LShortCopy8u_03gas_1
 
    mov          (%rdi), %ebx
    add          $(4), %rdi
    mov          %ebx, (%r9)
    add          $(4), %r9
    sub          $(4), %r10
 
LShortCopy8u_03gas_1:  
    test         %r10, %r10
    jz           LExitCopy8u_00gas_1
 
    movzbl       (-1)(%r10,%rdi), %ebx
    mov          %bl, (-1)(%r10,%r9)
    sub          $(1), %r10
    jz           LExitCopy8u_00gas_1
    movzbl       (-1)(%r10,%rdi), %ebx
    mov          %bl, (-1)(%r10,%r9)
    sub          $(1), %r10
    jz           LExitCopy8u_00gas_1
    movzbl       (-1)(%r10,%rdi), %ebx
    mov          %bl, (-1)(%r10,%r9)
 
 
LExitCopy8u_00gas_1:  
    mov          %rsi, %rax
 
 
    pop          %rbx
 
    ret
 
 
LLongCopy8u_00gas_1:  
    test         $(15), %r9
    jnz          LAlignCopy8u_00gas_1
 
LCheckCopy8u_00gas_1:  
    cmp          $(1048576), %r10
    jge          LGetCacheSize00gas_1
 
LCheckCopy8u_01gas_1:  
    test         $(15), %rdi
    jnz          LCheckCopy8u_02gas_1
 
 
LCheckAliasingCopy8u_00gas_1:  
    mov          %rdi, %rbx
    mov          %r9, %rax
    and          $(4095), %rbx
    and          $(4095), %rax
    sub          %rax, %rbx
    je           LInv00Copy8u_00gas_1
    cmp          $(160), %rbx
    jg           LInv00Copy8u_00gas_1
    cmp          $(-3936), %rbx
    jl           LFwd00Copy8u_00gas_1
    cmp          $(0), %rbx
    jg           LFwd00Copy8u_00gas_1
 
 
LInv00Copy8u_00gas_1:  
    test         $(1), %r10
    jz           LInv00Copy8u_01gas_1
 
    movzbl       (-1)(%r10,%rdi), %ebx
    mov          %bl, (-1)(%r10,%r9)
    sub          $(1), %r10
 
LInv00Copy8u_01gas_1:  
    test         $(2), %r10
    jz           LInv00Copy8u_02gas_1
 
    movzwl       (-2)(%r10,%rdi), %ebx
    mov          %bx, (-2)(%r10,%r9)
    sub          $(2), %r10
 
LInv00Copy8u_02gas_1:  
    test         $(4), %r10
    jz           LInv00Copy8u_03gas_1
 
    movl         (-4)(%r10,%rdi), %ebx
    mov          %ebx, (-4)(%r10,%r9)
    sub          $(4), %r10
 
LInv00Copy8u_03gas_1:  
    test         $(8), %r10
    jz           LInv00Copy8u_04gas_1
 
    movq         (-8)(%r10,%rdi), %xmm0
    movq         %xmm0, (-8)(%r10,%r9)
    sub          $(8), %r10
 
LInv00Copy8u_04gas_1:  
    test         $(16), %r10
    jz           LInv00Copy8u_05gas_1
 
    movdqa       (-16)(%r10,%rdi), %xmm0
    movdqa       %xmm0, (-16)(%r10,%r9)
    sub          $(16), %r10
 
LInv00Copy8u_05gas_1:  
    test         $(32), %r10
    jz           LInv00Copy8u_06gas_1
 
    movdqa       (-16)(%r10,%rdi), %xmm0
    movdqa       (-32)(%r10,%rdi), %xmm1
    movdqa       %xmm0, (-16)(%r10,%r9)
    movdqa       %xmm1, (-32)(%r10,%r9)
    sub          $(32), %r10
    jz           LExitInv00Copy8u_00gas_1
    jmp          LInv00Copy8u_06gas_1
 
.p2align 4, 0x90
LInv00Copy8u_06gas_1:  
    movdqa       (-16)(%r10,%rdi), %xmm0
    movdqa       %xmm0, (-16)(%r10,%r9)
    movdqa       (-32)(%r10,%rdi), %xmm0
    movdqa       %xmm0, (-32)(%r10,%r9)
    movdqa       (-48)(%r10,%rdi), %xmm0
    movdqa       %xmm0, (-48)(%r10,%r9)
    movdqa       (-64)(%r10,%rdi), %xmm0
    movdqa       %xmm0, (-64)(%r10,%r9)
    sub          $(64), %r10
    jnz          LInv00Copy8u_06gas_1
 
LExitInv00Copy8u_00gas_1:  
    mov          %rsi, %rax
 
 
    pop          %rbx
 
    ret
 
 
LFwd00Copy8u_00gas_1:  
    sub          $(64), %r10
 
.p2align 4, 0x90
LFwd00Copy8u_01gas_1:  
    movdqa       (%rdi), %xmm0
    movdqa       %xmm0, (%r9)
    movdqa       (16)(%rdi), %xmm0
    movdqa       %xmm0, (16)(%r9)
    movdqa       (32)(%rdi), %xmm0
    movdqa       %xmm0, (32)(%r9)
    movdqa       (48)(%rdi), %xmm0
    movdqa       %xmm0, (48)(%r9)
    add          $(64), %rdi
    add          $(64), %r9
    sub          $(64), %r10
    jge          LFwd00Copy8u_01gas_1
 
    add          $(64), %r10
    jz           LExitFwd00Copy8u_00gas_1
 
    test         $(32), %r10
    jz           LFwd00Copy8u_02gas_1
 
    movdqa       (%rdi), %xmm0
    movdqa       (16)(%rdi), %xmm1
    add          $(32), %rdi
    movdqa       %xmm0, (%r9)
    movdqa       %xmm1, (16)(%r9)
    add          $(32), %r9
    sub          $(32), %r10
    jz           LExitFwd00Copy8u_00gas_1
 
LFwd00Copy8u_02gas_1:  
    test         $(16), %r10
    jz           LFwd00Copy8u_03gas_1
 
    movdqa       (%rdi), %xmm0
    movdqa       %xmm0, (%r9)
    add          $(16), %rdi
    add          $(16), %r9
    sub          $(16), %r10
    jz           LExitFwd00Copy8u_00gas_1
 
LFwd00Copy8u_03gas_1:  
    test         $(8), %r10
    jz           LFwd00Copy8u_04gas_1
 
    movq         (%rdi), %xmm0
    movq         %xmm0, (%r9)
    add          $(8), %rdi
    add          $(8), %r9
    sub          $(8), %r10
    jz           LExitFwd00Copy8u_00gas_1
 
LFwd00Copy8u_04gas_1:  
    test         $(4), %r10
    jz           LFwd00Copy8u_05gas_1
 
    mov          (%rdi), %ebx
    mov          %ebx, (%r9)
    add          $(4), %rdi
    add          $(4), %r9
    sub          $(4), %r10
    jz           LExitFwd00Copy8u_00gas_1
 
LFwd00Copy8u_05gas_1:  
    test         $(2), %r10
    jz           LFwd00Copy8u_06gas_1
 
    movzwl       (%rdi), %ebx
    mov          %bx, (%r9)
    add          $(2), %rdi
    add          $(2), %r9
    sub          $(2), %r10
    jz           LExitFwd00Copy8u_00gas_1
 
LFwd00Copy8u_06gas_1:  
    test         %r10, %r10
    jz           LExitFwd00Copy8u_00gas_1
 
    movzbl       (%rdi), %ebx
    mov          %bl, (%r9)
 
LExitFwd00Copy8u_00gas_1:  
    mov          %rsi, %rax
 
 
    pop          %rbx
 
    ret
 
 
LAlignCopy8u_00gas_1:  
    movdqu       (%rdi), %xmm0
    mov          %r9, %rcx
    and          $(15), %rcx
    movdqu       %xmm0, (%r9)
    sub          $(16), %rcx
    sub          %rcx, %rdi
    sub          %rcx, %r9
    add          %rcx, %r10
 
    cmp          $(64), %r10
    jl           LShortCopy8u_32gas_1
 
    test         $(15), %rdi
    jz           LCheckAliasingCopy8u_00gas_1
 
 
    cmp          $(1048576), %r10
    jge          LGetCacheSize00gas_1
 
LCheckCopy8u_02gas_1:  
    test         $(3), %rdi
    jz           LCheckCopy8u_03gas_1
 
LLoopX0Copy8u_00gas_1:  
    sub          $(64), %r10
 
LLoopX0Copy8u_01gas_1:  
    movdqu       (%rdi), %xmm0
    movdqu       (16)(%rdi), %xmm1
    movdqu       (32)(%rdi), %xmm2
    movdqu       (48)(%rdi), %xmm3
    add          $(64), %rdi
    movdqa       %xmm0, (%r9)
    movdqa       %xmm1, (16)(%r9)
    movdqa       %xmm2, (32)(%r9)
    movdqa       %xmm3, (48)(%r9)
    add          $(64), %r9
    sub          $(64), %r10
    jge          LLoopX0Copy8u_01gas_1
 
    add          $(64), %r10
    jnz          LShortCopy8u_32gas_1
 
    jmp          LExitCopy8u_00gas_1
 
LCheckCopy8u_03gas_1:  
    movdqu       (%rdi), %xmm0
    movdqa       %xmm0, (%r9)
    add          $(16), %rdi
    add          $(16), %r9
    sub          $(32), %r10
 
    mov          $(-16), %rbx
    mov          %rdi, %rax
    and          %rdi, %rbx
    and          $(15), %rax
 
    cmp          $(12), %rax
    je           LLoop12Copy8u_00gas_1
 
    cmp          $(8), %rax
    je           LLoop08Copy8u_00gas_1
 
    mov          %r9, %rax
    sub          $(64), %r10
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
    movdqa       %xmm1, (%r9)
    movdqa       %xmm2, (16)(%r9)
    movdqa       %xmm3, (32)(%r9)
    movdqa       %xmm4, (48)(%r9)
    add          $(64), %r9
    sub          $(64), %r10
    jge          LLoop04Copy8u_00gas_1
 
    jmp          LEndNotAlignCopy_00gas_1
 
LLoop08Copy8u_00gas_1:  
    mov          %r9, %rax
    sub          $(64), %r10
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
    movdqa       %xmm1, (%r9)
    movdqa       %xmm2, (16)(%r9)
    movdqa       %xmm3, (32)(%r9)
    movdqa       %xmm4, (48)(%r9)
    add          $(64), %r9
    sub          $(64), %r10
    jge          LLoop08Copy8u_01gas_1
 
    jmp          LEndNotAlignCopy_00gas_1
 
LLoop12Copy8u_00gas_1:  
    mov          %r9, %rax
    sub          $(64), %r10
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
    movdqa       %xmm1, (%r9)
    movdqa       %xmm2, (16)(%r9)
    movdqa       %xmm3, (32)(%r9)
    movdqa       %xmm4, (48)(%r9)
    add          $(64), %r9
    sub          $(64), %r10
    jge          LLoop12Copy8u_01gas_1
 
LEndNotAlignCopy_00gas_1:  
    add          $(80), %r10
    sub          %r9, %rax
    sub          %rax, %rdi
    cmp          $(64), %r10
    jl           LShortCopy8u_32gas_1
 
    jmp          LLoopX0Copy8u_00gas_1
 
 
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
    je           LCheckCopy8u_01gas_1
 
    shr          $(1), %rbx
 
    cmp          %rbx, %r10
    jl           LCheckCopy8u_01gas_1
 
 
LPrefLoopCopy8uNt_00gas_1:  
    sub          $(262144), %r10
    jl           LLoopCopy8uNt_02gas_1
 
    mov          $(4096), %r8d
 
LPrefLoopCopy8uNt_01gas_1:  
    movzbl       (%rdi), %ebx
    add          $(64), %rdi
    sub          $(1), %r8d
    jnz          LPrefLoopCopy8uNt_01gas_1
 
    sub          $(262144), %rdi
    mov          $(262144), %r8d
 
LLoopCopy8uNt_01gas_1:  
    movdqu       (%rdi), %xmm0
    add          $(16), %rdi
    movntdq      %xmm0, (%r9)
    add          $(16), %r9
    sub          $(16), %r8d
    jnz          LLoopCopy8uNt_01gas_1
 
    jmp          LPrefLoopCopy8uNt_00gas_1
 
LLoopCopy8uNt_02gas_1:  
    add          $(262144), %r10
    jz           LExitCopy8uNt_02gas_1
 
    mov          %r10, %r8
    jmp          LLoopCopy8uNt_04gas_1
LLoopCopy8uNt_03gas_1:  
    movzbl       (%r8,%rdi), %ebx
LLoopCopy8uNt_04gas_1:  
    sub          $(64), %r8
    jge          LLoopCopy8uNt_03gas_1
 
    sub          $(16), %r10
    jl           LExitCopy8uNt_00gas_1
 
LLoopCopy8uNt_05gas_1:  
    movdqu       (%rdi), %xmm0
    add          $(16), %rdi
    movntdq      %xmm0, (%r9)
    add          $(16), %r9
    sub          $(16), %r10
    jge          LLoopCopy8uNt_05gas_1
 
LExitCopy8uNt_00gas_1:  
    sfence
 
    add          $(16), %r10
    jnz          LShortCopy8u_32gas_1
 
LExitCopy8uNt_01gas_1:  
    mov          %rsi, %rax
 
 
    pop          %rbx
 
    ret
 
LExitCopy8uNt_02gas_1:  
    sfence
    jmp          LExitCopy8uNt_01gas_1
 
 
