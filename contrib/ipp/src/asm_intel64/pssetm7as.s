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
 
 
.globl mfxownsSet_16u_M7

 
mfxownsSet_16u_M7:
 
 
    movzwq       (%rdi), %rax
    movzwq       (%rdi), %rcx
    shl          $(16), %rax
    mov          %rsi, %r8
    or           %rcx, %rax
    mov          %edx, %ecx
    movd         %rax, %xmm0
    cmp          $(524288), %rcx
    jg           LAlignSet16uWNt00gas_1
    test         $(1), %r8
    jnz          LAlignSet16uWNt00gas_1
 
    shl          $(1), %rcx
    punpckldq    %xmm0, %xmm0
 
    cmp          $(16), %rcx
    jl           LShortSet16uW02gas_1
 
    punpcklqdq   %xmm0, %xmm0
 
 
    movq         %xmm0, (%r8)
    mov          %r8, %rax
    and          $(15), %rax
    sub          $(16), %rax
    movq         %xmm0, (8)(%r8)
    sub          %rax, %r8
    add          %rax, %rcx
    jz           LExitSet16uW00gas_1
 
    cmp          $(64), %rcx
    jge          LSet16uW00gas_1
 
 
LShortSet16uW00gas_1:  
    sub          $(16), %rcx
    jl           LShortSet16uW01gas_1
 
    movdqa       %xmm0, (%r8)
    add          $(16), %r8
    sub          $(16), %rcx
    jl           LShortSet16uW01gas_1
    movdqa       %xmm0, (%r8)
    add          $(16), %r8
    sub          $(16), %rcx
    jl           LShortSet16uW01gas_1
    movdqa       %xmm0, (%r8)
    add          $(16), %r8
    sub          $(16), %rcx
 
LShortSet16uW01gas_1:  
    add          $(16), %rcx
    jz           LExitSet16uW00gas_1
 
LShortSet16uW02gas_1:  
    cmp          $(8), %rcx
    jl           LShortSet16uW04gas_1
    je           LShortSet16uW03gas_1
 
    movq         %xmm0, (%r8)
LShortSet16uW03gas_1:  
    movq         %xmm0, (-8)(%r8,%rcx)
 
LExitSet16uW00gas_1:  
    mov          %rsi, %rax
 
    ret
 
LShortSet16uW04gas_1:  
    movd         %xmm0, %rax
    cmp          $(4), %rcx
    jl           LShortSet16uW06gas_1
    je           LShortSet16uW05gas_1
 
    mov          %eax, (%r8)
LShortSet16uW05gas_1:  
    mov          %eax, (-4)(%r8,%rcx)
    mov          %rsi, %rax
 
    ret
 
LShortSet16uW06gas_1:  
    mov          %ax, (%r8)
    mov          %rsi, %rax
 
    ret
 
 
LSet16uW00gas_1:  
    sub          $(64), %rcx
 
LSet16uW01gas_1:  
    movdqa       %xmm0, (%r8)
    movdqa       %xmm0, (16)(%r8)
    movdqa       %xmm0, (32)(%r8)
    movdqa       %xmm0, (48)(%r8)
    add          $(64), %r8
    sub          $(64), %rcx
    jge          LSet16uW01gas_1
 
    add          $(64), %rcx
    jnz          LShortSet16uW00gas_1
 
    mov          %rsi, %rax
 
    ret
 
 
LAlignSet16uWNt00gas_1:  
    test         $(15), %r8
    jz           LGetCacheSize00gas_1
 
    mov          %ax, (%r8)
    add          $(2), %r8
    dec          %rcx
    jz           LExitSet16uW00gas_1
    jmp          LAlignSet16uWNt00gas_1
 
 
LGetCacheSize00gas_1:  
    punpckldq    %xmm0, %xmm0
    punpcklqdq   %xmm0, %xmm0
 
    lea          TableCacheSize(%rip), %rax
 
    sub          $(64), %rsp
    mov          %rax, (16)(%rsp)
    mov          %rbx, (24)(%rsp)
    mov          %rcx, (32)(%rsp)
    mov          %rdx, (40)(%rsp)
    mov          %r8, (48)(%rsp)
    mov          %rax, (56)(%rsp)
 
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
    mov          (56)(%rsp), %rax
    add          $(64), %rsp
 
 
    cmp          $(-1), %rax
    je           LGetCacheSize01gas_1
 
    shr          $(1), %rax
 
    cmp          %rax, %rcx
    jg           LSet16uWNt00gas_1
 
LGetCacheSize01gas_1:  
    add          %ecx, %ecx
    jmp          LSet16uW00gas_1
 
 
LSet16uWNt00gas_1:  
    sub          $(8), %rcx
 
LSet16uWNt01gas_1:  
    movntdq      %xmm0, (%r8)
    add          $(16), %r8
    sub          $(8), %rcx
    jge          LSet16uWNt01gas_1
 
    sfence
 
    add          $(8), %rcx
    add          %rcx, %rcx
    jnz          LShortSet16uW00gas_1
 
    mov          %rsi, %rax
 
    ret
 
 
.p2align 4, 0x90
 
 
.globl mfxownsSet_32s_M7

 
mfxownsSet_32s_M7:
 
 
    movd         (%rdi), %xmm0
    mov          %rsi, %r8
    mov          %edx, %ecx
    test         $(3), %r8
    jnz          LAlignSet32sW01gas_2
 
LAlignSet32sW00gas_2:  
    cmp          $(262144), %rcx
    jg           LAlignSet32sWNt00gas_2
 
    shl          $(2), %rcx
    pshufd       $(0), %xmm0, %xmm0
 
    cmp          $(16), %rcx
    jl           LShortSet32sW02gas_2
 
 
    movq         %xmm0, (%r8)
    mov          %r8, %rax
    and          $(15), %rax
    sub          $(16), %rax
    movq         %xmm0, (8)(%r8)
    sub          %rax, %r8
    add          %rax, %rcx
    jz           LExitSet32sW00gas_2
 
    cmp          $(64), %rcx
    jge          LSet32sW00gas_2
 
 
LShortSet32sW00gas_2:  
    sub          $(16), %rcx
    jl           LShortSet32sW01gas_2
 
    movdqa       %xmm0, (%r8)
    add          $(16), %r8
    sub          $(16), %rcx
    jl           LShortSet32sW01gas_2
    movdqa       %xmm0, (%r8)
    add          $(16), %r8
    sub          $(16), %rcx
    jl           LShortSet32sW01gas_2
    movdqa       %xmm0, (%r8)
    add          $(16), %r8
    sub          $(16), %rcx
 
LShortSet32sW01gas_2:  
    add          $(16), %rcx
    jz           LExitSet32sW00gas_2
 
LShortSet32sW02gas_2:  
    cmp          $(8), %rcx
    jl           LShortSet32sW04gas_2
    je           LShortSet32sW03gas_2
 
    movq         %xmm0, (%r8)
LShortSet32sW03gas_2:  
    movq         %xmm0, (-8)(%r8,%rcx)
 
LExitSet32sW00gas_2:  
    mov          %rsi, %rax
 
    ret
 
LShortSet32sW04gas_2:  
    movd         %xmm0, (%r8)
    mov          %rsi, %rax
 
    ret
 
 
LSet32sW00gas_2:  
    sub          $(64), %rcx
 
LSet32sW01gas_2:  
    movdqa       %xmm0, (%r8)
    movdqa       %xmm0, (16)(%r8)
    movdqa       %xmm0, (32)(%r8)
    movdqa       %xmm0, (48)(%r8)
    add          $(64), %r8
    sub          $(64), %rcx
    jge          LSet32sW01gas_2
 
    add          $(64), %rcx
    jnz          LShortSet32sW00gas_2
 
    mov          %rsi, %rax
 
    ret
 
 
LAlignSet32sW01gas_2:  
    test         $(1), %r8
    jnz          LAlignSet32sWNt00gas_2
 
    movd         %xmm0, (%r8)
    movd         %xmm0, (-4)(%r8,%rcx,4)
    pshuflw      $(225), %xmm0, %xmm0
    add          $(2), %r8
    sub          $(1), %rcx
    jz           LExitSet32sW00gas_2
    jmp          LAlignSet32sW00gas_2
 
 
LAlignSet32sWNt00gas_2:  
    test         $(15), %r8
    jz           LGetCacheSize00gas_2
 
    movd         %xmm0, (%r8)
    add          $(4), %r8
    dec          %rcx
    jz           LExitSet32sW00gas_2
    jmp          LAlignSet32sWNt00gas_2
 
 
LGetCacheSize00gas_2:  
    pshufd       $(0), %xmm0, %xmm0
 
    lea          TableCacheSize(%rip), %rax
 
    sub          $(64), %rsp
    mov          %rax, (16)(%rsp)
    mov          %rbx, (24)(%rsp)
    mov          %rcx, (32)(%rsp)
    mov          %rdx, (40)(%rsp)
    mov          %r8, (48)(%rsp)
    mov          %rax, (56)(%rsp)
 
    xor          %eax, %eax
    cpuid
 
    cmp          $(1970169159), %ebx
    jne          LCacheSizeMacro11gas_2
    cmp          $(1231384169), %edx
    jne          LCacheSizeMacro11gas_2
    cmp          $(1818588270), %ecx
    jne          LCacheSizeMacro11gas_2
 
    mov          $(2), %eax
    cpuid
 
    cmp          $(1), %al
    jne          LCacheSizeMacro11gas_2
 
    test         $(2147483648), %eax
    jz           LCacheSizeMacro00gas_2
    xor          %eax, %eax
LCacheSizeMacro00gas_2:  
    test         $(2147483648), %ebx
    jz           LCacheSizeMacro01gas_2
    xor          %ebx, %ebx
LCacheSizeMacro01gas_2:  
    test         $(2147483648), %ecx
    jz           LCacheSizeMacro02gas_2
    xor          %ecx, %ecx
LCacheSizeMacro02gas_2:  
    test         $(2147483648), %edx
    jz           LCacheSizeMacro03gas_2
    xor          %edx, %edx
 
LCacheSizeMacro03gas_2:  
    mov          %rsp, %r8
    test         %eax, %eax
    jz           LCacheSizeMacro04gas_2
    mov          %eax, (%r8)
    add          $(4), %r8
    mov          $(3), %eax
LCacheSizeMacro04gas_2:  
    test         %ebx, %ebx
    jz           LCacheSizeMacro05gas_2
    mov          %ebx, (%r8)
    add          $(4), %r8
    add          $(4), %eax
LCacheSizeMacro05gas_2:  
    test         %ecx, %ecx
    jz           LCacheSizeMacro06gas_2
    mov          %ecx, (%r8)
    add          $(4), %r8
    add          $(4), %eax
LCacheSizeMacro06gas_2:  
    test         %edx, %edx
    jz           LCacheSizeMacro07gas_2
    mov          %edx, (%r8)
    add          $(4), %eax
 
LCacheSizeMacro07gas_2:  
    mov          (56)(%rsp), %rbx
 
    test         %eax, %eax
    jz           LCacheSizeMacro11gas_2
LCacheSizeMacro08gas_2:  
    movzbl       (%rbx), %edx
    test         %edx, %edx
    jz           LCacheSizeMacro11gas_2
    add          $(2), %rbx
    mov          %eax, %ecx
LCacheSizeMacro09gas_2:  
    cmpb         (%rsp,%rcx), %dl
    je           LCacheSizeMacro10gas_2
    sub          $(1), %ecx
    jnz          LCacheSizeMacro09gas_2
    jmp          LCacheSizeMacro08gas_2
 
LCacheSizeMacro10gas_2:  
    movzbl       (-1)(%rbx), %ebx
    mov          %ebx, %ecx
    shr          $(4), %ebx
    and          $(15), %ecx
    add          $(18), %ecx
    shl          %cl, %rbx
    mov          %rbx, (56)(%rsp)
    jmp          LCacheSizeMacro12gas_2
 
LCacheSizeMacro11gas_2:  
    movq         $(-1), (56)(%rsp)
 
LCacheSizeMacro12gas_2:  
    mov          (16)(%rsp), %rax
    mov          (24)(%rsp), %rbx
    mov          (32)(%rsp), %rcx
    mov          (40)(%rsp), %rdx
    mov          (48)(%rsp), %r8
    mov          (56)(%rsp), %rax
    add          $(64), %rsp
 
 
    cmp          $(-1), %rax
    je           LGetCacheSize01gas_2
 
    shr          $(2), %rax
 
    cmp          %rax, %rcx
    jg           LSet32sWNt00gas_2
 
LGetCacheSize01gas_2:  
    shl          $(2), %ecx
    jmp          LSet32sW00gas_2
 
 
LSet32sWNt00gas_2:  
    sub          $(4), %rcx
 
LSet32sWNt01gas_2:  
    movntdq      %xmm0, (%r8)
    add          $(16), %r8
    sub          $(4), %rcx
    jge          LSet32sWNt01gas_2
 
    sfence
 
    add          $(4), %rcx
    lea          (,%rcx,4), %rcx
    jnz          LShortSet32sW00gas_2
 
    mov          %rsi, %rax
 
    ret
 
 
.p2align 4, 0x90
 
 
.globl mfxownsSet_64s_M7

 
mfxownsSet_64s_M7:
 
 
    movq         (%rdi), %xmm0
    mov          %rsi, %r8
    mov          %edx, %ecx
    test         $(7), %r8
    jnz          LAlignSet64sW01gas_3
 
LAlignSet64sW00gas_3:  
    cmp          $(131072), %rcx
    jg           LAlignSet64sWNt00gas_3
 
    shl          $(3), %rcx
    pshufd       $(68), %xmm0, %xmm0
 
    cmp          $(16), %rcx
    jl           LShortSet64sW02gas_3
 
 
    movdqu       %xmm0, (%r8)
    mov          %r8, %rax
    and          $(15), %rax
    sub          $(16), %rax
    sub          %rax, %r8
    add          %rax, %rcx
    jz           LExitSet64sW00gas_3
 
    cmp          $(64), %rcx
    jge          LSet64sW00gas_3
 
 
LShortSet64sW00gas_3:  
    sub          $(16), %rcx
    jl           LShortSet64sW01gas_3
 
    movdqa       %xmm0, (%r8)
    add          $(16), %r8
    sub          $(16), %rcx
    jl           LShortSet64sW01gas_3
    movdqa       %xmm0, (%r8)
    add          $(16), %r8
    sub          $(16), %rcx
    jl           LShortSet64sW01gas_3
    movdqa       %xmm0, (%r8)
    add          $(16), %r8
    sub          $(16), %rcx
 
LShortSet64sW01gas_3:  
    add          $(16), %rcx
    jz           LExitSet64sW00gas_3
 
LShortSet64sW02gas_3:  
    movq         %xmm0, (%r8)
 
LExitSet64sW00gas_3:  
    mov          %rsi, %rax
 
    ret
 
 
LSet64sW00gas_3:  
    sub          $(64), %rcx
 
LSet64sW01gas_3:  
    movdqa       %xmm0, (%r8)
    movdqa       %xmm0, (16)(%r8)
    movdqa       %xmm0, (32)(%r8)
    movdqa       %xmm0, (48)(%r8)
    add          $(64), %r8
    sub          $(64), %rcx
    jge          LSet64sW01gas_3
 
    add          $(64), %rcx
    jnz          LShortSet64sW00gas_3
 
    mov          %rsi, %rax
 
    ret
 
 
LAlignSet64sW01gas_3:  
    test         $(3), %r8
    jnz          LAlignSet64sWNt00gas_3
 
    movq         %xmm0, (%r8)
    movq         %xmm0, (-8)(%r8,%rcx,8)
    pshuflw      $(78), %xmm0, %xmm0
    add          $(4), %r8
    dec          %rcx
    jz           LExitSet64sW00gas_3
    jmp          LAlignSet64sW00gas_3
 
 
LAlignSet64sWNt00gas_3:  
    test         $(15), %r8
    jz           LGetCacheSize00gas_3
 
    movq         %xmm0, (%r8)
    add          $(8), %r8
    dec          %rcx
    jz           LExitSet64sW00gas_3
    jmp          LAlignSet64sWNt00gas_3
 
 
LGetCacheSize00gas_3:  
    pshufd       $(68), %xmm0, %xmm0
 
    lea          TableCacheSize(%rip), %rax
 
    sub          $(64), %rsp
    mov          %rax, (16)(%rsp)
    mov          %rbx, (24)(%rsp)
    mov          %rcx, (32)(%rsp)
    mov          %rdx, (40)(%rsp)
    mov          %r8, (48)(%rsp)
    mov          %rax, (56)(%rsp)
 
    xor          %eax, %eax
    cpuid
 
    cmp          $(1970169159), %ebx
    jne          LCacheSizeMacro11gas_3
    cmp          $(1231384169), %edx
    jne          LCacheSizeMacro11gas_3
    cmp          $(1818588270), %ecx
    jne          LCacheSizeMacro11gas_3
 
    mov          $(2), %eax
    cpuid
 
    cmp          $(1), %al
    jne          LCacheSizeMacro11gas_3
 
    test         $(2147483648), %eax
    jz           LCacheSizeMacro00gas_3
    xor          %eax, %eax
LCacheSizeMacro00gas_3:  
    test         $(2147483648), %ebx
    jz           LCacheSizeMacro01gas_3
    xor          %ebx, %ebx
LCacheSizeMacro01gas_3:  
    test         $(2147483648), %ecx
    jz           LCacheSizeMacro02gas_3
    xor          %ecx, %ecx
LCacheSizeMacro02gas_3:  
    test         $(2147483648), %edx
    jz           LCacheSizeMacro03gas_3
    xor          %edx, %edx
 
LCacheSizeMacro03gas_3:  
    mov          %rsp, %r8
    test         %eax, %eax
    jz           LCacheSizeMacro04gas_3
    mov          %eax, (%r8)
    add          $(4), %r8
    mov          $(3), %eax
LCacheSizeMacro04gas_3:  
    test         %ebx, %ebx
    jz           LCacheSizeMacro05gas_3
    mov          %ebx, (%r8)
    add          $(4), %r8
    add          $(4), %eax
LCacheSizeMacro05gas_3:  
    test         %ecx, %ecx
    jz           LCacheSizeMacro06gas_3
    mov          %ecx, (%r8)
    add          $(4), %r8
    add          $(4), %eax
LCacheSizeMacro06gas_3:  
    test         %edx, %edx
    jz           LCacheSizeMacro07gas_3
    mov          %edx, (%r8)
    add          $(4), %eax
 
LCacheSizeMacro07gas_3:  
    mov          (56)(%rsp), %rbx
 
    test         %eax, %eax
    jz           LCacheSizeMacro11gas_3
LCacheSizeMacro08gas_3:  
    movzbl       (%rbx), %edx
    test         %edx, %edx
    jz           LCacheSizeMacro11gas_3
    add          $(2), %rbx
    mov          %eax, %ecx
LCacheSizeMacro09gas_3:  
    cmpb         (%rsp,%rcx), %dl
    je           LCacheSizeMacro10gas_3
    sub          $(1), %ecx
    jnz          LCacheSizeMacro09gas_3
    jmp          LCacheSizeMacro08gas_3
 
LCacheSizeMacro10gas_3:  
    movzbl       (-1)(%rbx), %ebx
    mov          %ebx, %ecx
    shr          $(4), %ebx
    and          $(15), %ecx
    add          $(18), %ecx
    shl          %cl, %rbx
    mov          %rbx, (56)(%rsp)
    jmp          LCacheSizeMacro12gas_3
 
LCacheSizeMacro11gas_3:  
    movq         $(-1), (56)(%rsp)
 
LCacheSizeMacro12gas_3:  
    mov          (16)(%rsp), %rax
    mov          (24)(%rsp), %rbx
    mov          (32)(%rsp), %rcx
    mov          (40)(%rsp), %rdx
    mov          (48)(%rsp), %r8
    mov          (56)(%rsp), %rax
    add          $(64), %rsp
 
 
    cmp          $(-1), %rax
    je           LGetCacheSize01gas_3
 
    shr          $(3), %rax
 
    cmp          %rax, %rcx
    jg           LSet64sWNt00gas_3
 
LGetCacheSize01gas_3:  
    shl          $(3), %ecx
    jmp          LSet64sW00gas_3
 
 
LSet64sWNt00gas_3:  
    sub          $(2), %rcx
 
LSet64sWNt01gas_3:  
    movntdq      %xmm0, (%r8)
    add          $(16), %r8
    sub          $(2), %rcx
    jge          LSet64sWNt01gas_3
 
    sfence
 
    add          $(2), %rcx
    lea          (,%rcx,8), %rcx
    jnz          LShortSet64sW00gas_3
 
    mov          %rsi, %rax
 
    ret
 
 
.p2align 4, 0x90
 
 
.globl mfxownsSet_64sc_M7

 
mfxownsSet_64sc_M7:
 
 
    movq         (%rdi), %xmm0
    movq         (8)(%rdi), %xmm1
    punpcklqdq   %xmm1, %xmm0
    mov          %rsi, %r8
    mov          %edx, %ecx
    test         $(15), %r8
    jnz          LAlignSet64scW01gas_4
 
LAlignSet64scW00gas_4:  
    cmp          $(4), %rcx
    jge          LSet64scW00gas_4
 
 
LShortSet64scW00gas_4:  
    test         %rcx, %rcx
    jz           LExitSet64scW00gas_4
 
    movdqa       %xmm0, (%r8)
    add          $(16), %r8
    dec          %rcx
    jz           LExitSet64scW00gas_4
    movdqa       %xmm0, (%r8)
    add          $(16), %r8
    dec          %rcx
    jz           LExitSet64scW00gas_4
    movdqa       %xmm0, (%r8)
 
LExitSet64scW00gas_4:  
    mov          %rsi, %rax
 
    ret
 
 
LSet64scW00gas_4:  
    cmp          $(65536), %rcx
    jg           LGetCacheSize00gas_4
 
LSet64scW01gas_4:  
    sub          $(4), %rcx
 
LSet64scW02gas_4:  
    movdqa       %xmm0, (%r8)
    movdqa       %xmm0, (16)(%r8)
    movdqa       %xmm0, (32)(%r8)
    movdqa       %xmm0, (48)(%r8)
    add          $(64), %r8
    sub          $(4), %rcx
    jge          LSet64scW02gas_4
 
    add          $(4), %rcx
    jnz          LShortSet64scW00gas_4
 
    mov          %rsi, %rax
 
    ret
 
 
LGetCacheSize00gas_4:  
 
    lea          TableCacheSize(%rip), %rax
 
    sub          $(64), %rsp
    mov          %rax, (16)(%rsp)
    mov          %rbx, (24)(%rsp)
    mov          %rcx, (32)(%rsp)
    mov          %rdx, (40)(%rsp)
    mov          %r8, (48)(%rsp)
    mov          %rax, (56)(%rsp)
 
    xor          %eax, %eax
    cpuid
 
    cmp          $(1970169159), %ebx
    jne          LCacheSizeMacro11gas_4
    cmp          $(1231384169), %edx
    jne          LCacheSizeMacro11gas_4
    cmp          $(1818588270), %ecx
    jne          LCacheSizeMacro11gas_4
 
    mov          $(2), %eax
    cpuid
 
    cmp          $(1), %al
    jne          LCacheSizeMacro11gas_4
 
    test         $(2147483648), %eax
    jz           LCacheSizeMacro00gas_4
    xor          %eax, %eax
LCacheSizeMacro00gas_4:  
    test         $(2147483648), %ebx
    jz           LCacheSizeMacro01gas_4
    xor          %ebx, %ebx
LCacheSizeMacro01gas_4:  
    test         $(2147483648), %ecx
    jz           LCacheSizeMacro02gas_4
    xor          %ecx, %ecx
LCacheSizeMacro02gas_4:  
    test         $(2147483648), %edx
    jz           LCacheSizeMacro03gas_4
    xor          %edx, %edx
 
LCacheSizeMacro03gas_4:  
    mov          %rsp, %r8
    test         %eax, %eax
    jz           LCacheSizeMacro04gas_4
    mov          %eax, (%r8)
    add          $(4), %r8
    mov          $(3), %eax
LCacheSizeMacro04gas_4:  
    test         %ebx, %ebx
    jz           LCacheSizeMacro05gas_4
    mov          %ebx, (%r8)
    add          $(4), %r8
    add          $(4), %eax
LCacheSizeMacro05gas_4:  
    test         %ecx, %ecx
    jz           LCacheSizeMacro06gas_4
    mov          %ecx, (%r8)
    add          $(4), %r8
    add          $(4), %eax
LCacheSizeMacro06gas_4:  
    test         %edx, %edx
    jz           LCacheSizeMacro07gas_4
    mov          %edx, (%r8)
    add          $(4), %eax
 
LCacheSizeMacro07gas_4:  
    mov          (56)(%rsp), %rbx
 
    test         %eax, %eax
    jz           LCacheSizeMacro11gas_4
LCacheSizeMacro08gas_4:  
    movzbl       (%rbx), %edx
    test         %edx, %edx
    jz           LCacheSizeMacro11gas_4
    add          $(2), %rbx
    mov          %eax, %ecx
LCacheSizeMacro09gas_4:  
    cmpb         (%rsp,%rcx), %dl
    je           LCacheSizeMacro10gas_4
    sub          $(1), %ecx
    jnz          LCacheSizeMacro09gas_4
    jmp          LCacheSizeMacro08gas_4
 
LCacheSizeMacro10gas_4:  
    movzbl       (-1)(%rbx), %ebx
    mov          %ebx, %ecx
    shr          $(4), %ebx
    and          $(15), %ecx
    add          $(18), %ecx
    shl          %cl, %rbx
    mov          %rbx, (56)(%rsp)
    jmp          LCacheSizeMacro12gas_4
 
LCacheSizeMacro11gas_4:  
    movq         $(-1), (56)(%rsp)
 
LCacheSizeMacro12gas_4:  
    mov          (16)(%rsp), %rax
    mov          (24)(%rsp), %rbx
    mov          (32)(%rsp), %rcx
    mov          (40)(%rsp), %rdx
    mov          (48)(%rsp), %r8
    mov          (56)(%rsp), %rax
    add          $(64), %rsp
 
 
    cmp          $(-1), %rax
    je           LSet64scW01gas_4
 
    shr          $(4), %rax
 
    cmp          %rax, %rcx
    jle          LSet64scW01gas_4
 
 
LSet64scWNt00gas_4:  
    movntdq      %xmm0, (%r8)
    add          $(16), %r8
    dec          %rcx
    jnz          LSet64scWNt00gas_4
 
    sfence
 
    mov          %rsi, %rax
 
    ret
 
 
LAlignSet64scW01gas_4:  
    test         $(7), %r8
    jnz          LNotAlignSet64scW00gas_4
 
    lea          (,%rcx,2), %rax
    movq         %xmm0, (%r8)
    pshufd       $(78), %xmm0, %xmm0
    movq         %xmm0, (-8)(%r8,%rax,8)
    add          $(8), %r8
    dec          %rcx
    jz           LExitSet64scW00gas_4
    jmp          LAlignSet64scW00gas_4
 
 
LNotAlignSet64scW00gas_4:  
    movdqu       %xmm0, (%r8)
    add          $(16), %r8
    dec          %rcx
    jnz          LNotAlignSet64scW00gas_4
    jmp          LExitSet64scW00gas_4
 
 
