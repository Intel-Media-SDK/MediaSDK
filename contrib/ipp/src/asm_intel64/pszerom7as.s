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
 
 
.globl mfxownsZero_8u

 
mfxownsZero_8u:
 
 
    mov          %rdi, %r8
    movslq       %esi, %rsi
 
    pxor         %xmm0, %xmm0
    xor          %edx, %edx
 
    cmp          $(16), %rsi
    jl           LShortZero8uW03gas_1
 
    test         $(15), %r8
    jz           LAlignZero8uW04gas_1
 
 
    mov          %r8, %rax
    and          $(15), %eax
    sub          $(16), %eax
    neg          %eax
    test         $(1), %eax
    jz           LAlignZero8uW00gas_1
    mov          %dl, (%r8)
    add          $(1), %r8
LAlignZero8uW00gas_1:  
    test         $(2), %eax
    jz           LAlignZero8uW01gas_1
    mov          %dx, (%r8)
    add          $(2), %r8
LAlignZero8uW01gas_1:  
    test         $(4), %eax
    jz           LAlignZero8uW02gas_1
    mov          %edx, (%r8)
    add          $(4), %r8
LAlignZero8uW02gas_1:  
    test         $(8), %eax
    jz           LAlignZero8uW03gas_1
    mov          %rdx, (%r8)
    add          $(8), %r8
LAlignZero8uW03gas_1:  
    sub          %rax, %rsi
    jz           LShortZero8uW02gas_1
 
LAlignZero8uW04gas_1:  
    cmp          $(64), %rsi
    jge          LZero8uW00gas_1
 
 
LShortZero8uW00gas_1:  
    sub          $(16), %rsi
    jl           LShortZero8uW01gas_1
 
    movdqa       %xmm0, (%r8)
    add          $(16), %r8
    sub          $(16), %rsi
    jl           LShortZero8uW01gas_1
    movdqa       %xmm0, (%r8)
    add          $(16), %r8
    sub          $(16), %rsi
    jl           LShortZero8uW01gas_1
    movdqa       %xmm0, (%r8)
    add          $(16), %r8
    sub          $(16), %rsi
 
LShortZero8uW01gas_1:  
    add          $(16), %rsi
    jnz          LShortZero8uW03gas_1
 
LShortZero8uW02gas_1:  
    mov          %rdi, %rax
 
    ret
 
LShortZero8uW03gas_1:  
    cmp          $(8), %rsi
    jl           LShortZero8uW05gas_1
    je           LShortZero8uW04gas_1
 
    mov          %rdx, (%r8)
LShortZero8uW04gas_1:  
    mov          %rdx, (-8)(%r8,%rsi)
 
    mov          %rdi, %rax
 
    ret
 
LShortZero8uW05gas_1:  
    cmp          $(4), %rsi
    jl           LShortZero8uW07gas_1
    je           LShortZero8uW06gas_1
 
    mov          %edx, (%r8)
LShortZero8uW06gas_1:  
    mov          %edx, (-4)(%r8,%rsi)
 
    mov          %rdi, %rax
 
    ret
 
LShortZero8uW07gas_1:  
    mov          %dl, (%r8)
    sub          $(1), %rsi
    jz           LShortZero8uW08gas_1
    mov          %dl, (1)(%r8)
    sub          $(1), %rsi
    jz           LShortZero8uW08gas_1
    mov          %dl, (2)(%r8)
 
LShortZero8uW08gas_1:  
    mov          %rdi, %rax
 
    ret
 
 
LZero8uW00gas_1:  
    cmp          $(524288), %rsi
    jg           LGetCacheSize00gas_1
 
LZero8uW01gas_1:  
    sub          $(64), %rsi
 
LZero8uW02gas_1:  
    movdqa       %xmm0, (%r8)
    movdqa       %xmm0, (16)(%r8)
    movdqa       %xmm0, (32)(%r8)
    movdqa       %xmm0, (48)(%r8)
    add          $(64), %r8
    sub          $(64), %rsi
    jge          LZero8uW02gas_1
 
    add          $(64), %rsi
    jnz          LShortZero8uW00gas_1
 
    mov          %rdi, %rax
 
    ret
 
 
LGetCacheSize00gas_1:  
 
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
    je           LZero8uW01gas_1
 
    cmp          %rax, %rsi
    jl           LZero8uW01gas_1
 
 
LZero8uWNt00gas_1:  
    sub          $(16), %rsi
 
LZero8uWNt01gas_1:  
    movntdq      %xmm0, (%r8)
    add          $(16), %r8
    sub          $(16), %rsi
    jge          LZero8uWNt01gas_1
 
    sfence
 
    add          $(16), %rsi
    jnz          LShortZero8uW00gas_1
 
    mov          %rdi, %rax
 
    ret
 
 
