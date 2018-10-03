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
 
 
.globl mfxhas_cpuid

 
mfxhas_cpuid:
    mov          $(1), %rax
 
 
    ret
 
 
.globl mfxmax_cpuid_input

 
mfxmax_cpuid_input:
 
 
    push         %rbx
 
 
    xor          %rax, %rax
    cpuid
 
 
    pop          %rbx
 
    ret
 
 
.globl mfxis_GenuineIntel

 
mfxis_GenuineIntel:
 
 
    push         %rbx
 
 
    xor          %rax, %rax
    cpuid
    xor          %rax, %rax
    cmp          $(1818588270), %ecx
    jne          Lfalsegas_18
    cmp          $(1231384169), %edx
    jne          Lfalsegas_18
    cmp          $(1970169159), %ebx
    jne          Lfalsegas_18
    mov          $(1), %rax
Lfalsegas_18:  
 
 
    pop          %rbx
 
    ret
 
 
.globl mfxownGetCacheSize

 
mfxownGetCacheSize:
 
 
    push         %rbx
 
 
    push         %rbp
 
 
    sub          $(24), %rsp
 
 
    mov          %rsp, %rbp
    xor          %esi, %esi
 
    mov          $(2), %eax
    cpuid
 
    cmp          $(1), %al
    jne          LGetCacheSize_11gas_36
 
    test         $(2147483648), %eax
    jz           LGetCacheSize_00gas_36
    xor          %eax, %eax
LGetCacheSize_00gas_36:  
    test         $(2147483648), %ebx
    jz           LGetCacheSize_01gas_36
    xor          %ebx, %ebx
LGetCacheSize_01gas_36:  
    test         $(2147483648), %ecx
    jz           LGetCacheSize_02gas_36
    xor          %ecx, %ecx
LGetCacheSize_02gas_36:  
    test         $(2147483648), %edx
    jz           LGetCacheSize_03gas_36
    xor          %edx, %edx
 
LGetCacheSize_03gas_36:  
    test         %eax, %eax
    jz           LGetCacheSize_04gas_36
    mov          %eax, (%rbp)
    add          $(4), %rbp
    add          $(3), %esi
LGetCacheSize_04gas_36:  
    test         %ebx, %ebx
    jz           LGetCacheSize_05gas_36
    mov          %ebx, (%rbp)
    add          $(4), %rbp
    add          $(4), %esi
LGetCacheSize_05gas_36:  
    test         %ecx, %ecx
    jz           LGetCacheSize_06gas_36
    mov          %ecx, (%rbp)
    add          $(4), %rbp
    add          $(4), %esi
LGetCacheSize_06gas_36:  
    test         %edx, %edx
    jz           LGetCacheSize_07gas_36
    mov          %edx, (%rbp)
    add          $(4), %esi
 
LGetCacheSize_07gas_36:  
    test         %esi, %esi
    jz           LGetCacheSize_11gas_36
    mov          $(-1), %eax
LGetCacheSize_08gas_36:  
    xor          %edx, %edx
    add          (%rdi), %edx
    jz           LExitGetCacheSize00gas_36
    add          $(8), %rdi
    mov          %esi, %ecx
LGetCacheSize_09gas_36:  
    cmpb         (%rsp,%rcx), %dl
    je           LGetCacheSize_10gas_36
    dec          %ecx
    jnz          LGetCacheSize_09gas_36
    jmp          LGetCacheSize_08gas_36
 
LGetCacheSize_10gas_36:  
    mov          (-4)(%rdi), %eax
 
LExitGetCacheSize00gas_36:  
    add          $(24), %rsp
 
 
    pop          %rbp
 
 
    pop          %rbx
 
    ret
 
LGetCacheSize_11gas_36:  
    mov          $(-1), %eax
    jmp          LExitGetCacheSize00gas_36
 
 
