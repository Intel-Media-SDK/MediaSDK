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
TOP_LABEL:  
 
 

.long  0, 1, 2, 4  
 

.long  8, 16, 32, 64  
 

.long  128, 256, 512, 1024  
 

.long  2048, 4096, 8192, 16384  
 
 

.long  0, -1, -3, -7  
 

.long  -15, -31, -63, -127  
 

.long  -255, -511, -1023, -2047  
 

.long  -4095, -8191, -16383, -32767  
 
 

.quad   0xffffffffffffffff,  0xffffffffffffffff  
 
 
.text
 
 
.p2align 4, 0x90
 
 
.globl mfxownpj_DecodeHuffmanRow_JPEG_1u16s_C1P4

 
mfxownpj_DecodeHuffmanRow_JPEG_1u16s_C1P4:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    push         %r12
 
 
    push         %r13
 
 
    push         %r14
 
 
    push         %r15
 
 
    sub          $(104), %rsp
 
 
    movslq       %esi, %rsi
    movslq       %r8d, %r8
    movslq       %r9d, %r9
    mov          %rdi, (72)(%rsp)
    mov          %rsi, (80)(%rsp)
    mov          %rdx, (88)(%rsp)
 
    sub          $(1), %r9
    jl           LDecHuffExitEndOfWork00gas_1
    cmp          $(3), %r9
    jg           LDecHuffExitEndOfWork00gas_1
 
    movq         (160)(%rsp), %rax
    mov          (%rax), %eax
    test         %eax, %eax
    jnz          LDecHuffExitEndOfWork00gas_1
 
 
    shl          $(1), %r9
    movq         (168)(%rsp), %rbx
    mov          %r9, %rdi
LInitDecHuff00gas_1:  
    mov          (%rcx), %rax
    add          $(8), %rcx
    lea          (%rax,%r8,2), %rax
    mov          %rax, (%rsp,%rdi,8)
    mov          (%rbx), %rax
    add          $(8), %rbx
    mov          %rax, (8)(%rsp,%rdi,8)
    sub          $(2), %rdi
    jge          LInitDecHuff00gas_1
 
    neg          %r8
 
    movq         (176)(%rsp), %rax
    mov          (8)(%rax), %ebp
    mov          (%rax), %r11
    mov          $(64), %ecx
    sub          %ebp, %ecx
    shl          %cl, %r11
    mov          (%rdx), %edx
    mov          %rdx, %r12
    sub          %rsi, %rdx
    lea          (,%r8,2), %rbx
    cmp          %rbx, %rdx
    jg           LDecHuffExitEndOfWork00gas_1
    lea          TOP_LABEL(%rip), %rsi
 
 
LColDecHuff00gas_1:  
    mov          %r9, %rdi
 
LDecHuff__00gas_1:  
    mov          (8)(%rsp,%rdi,8), %r15
    mov          %r11, %rcx
    shr          $(56), %rcx
    cmp          $(8), %ebp
    jl           LDecHuff__Call00gas_1
LDecHuff__01gas_1:  
    mov          (512)(%r15,%rcx,4), %ecx
    test         $(4294901760), %ecx
    jz           LDecHuff__Long00gas_1
    mov          %ecx, %ebx
    shr          $(16), %ecx
    sub          %ecx, %ebp
    shl          %cl, %r11
LDecHuff__RetLonggas_1:  
    and          $(65535), %ebx
    jz           LDecHuff__04gas_1
    cmp          $(16), %ebx
    jge          LDecHuff__05gas_1
    cmp          %ebx, %ebp
    jl           LDecHuff__Call01gas_1
LDecHuff__02gas_1:  
    mov          %ebx, %ecx
    mov          %r11, %rdx
    shl          %cl, %r11
    mov          $(16), %ecx
    shr          $(48), %rdx
    sub          %rbx, %rbp
    sub          %rbx, %rcx
    shr          %cl, %edx
    xor          %ecx, %ecx
    cmpl         (%rsi,%rbx,4), %edx
    cmovll       (64)(%rsi,%rbx,4), %ecx
    add          %ecx, %edx
LDecHuff__03gas_1:  
    mov          (%rsp,%rdi,8), %rax
    mov          %r11, %rcx
    mov          %dx, (%rax,%r8,2)
    shr          $(56), %rcx
    sub          $(2), %rdi
    jge          LDecHuff__00gas_1
 
    add          $(1), %r8
    jnz          LColDecHuff00gas_1
    jmp          LDecHuffExitNorm00gas_1
 
LDecHuff__04gas_1:  
    xor          %edx, %edx
    jmp          LDecHuff__03gas_1
LDecHuff__05gas_1:  
    mov          $(32768), %edx
    jmp          LDecHuff__03gas_1
 
 
LDecHuffExitNorm00gas_1:  
    mov          $(0), %eax
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
    mov          (88)(%rsp), %rdx
    mov          %r12d, (%rdx)
    movq         (176)(%rsp), %rdx
    mov          %ebp, (8)(%rdx)
    mov          %r11, (%rdx)
LDecHuffExit02gas_1:  
    add          $(104), %rsp
 
 
    pop          %r15
 
 
    pop          %r14
 
 
    pop          %r13
 
 
    pop          %r12
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
LDecHuffExitEndOfWork00gas_1:  
    mov          $(1), %eax
    jmp          LDecHuffExit02gas_1
 
 
LDecHuff__Call00gas_1:  
    lea          LDecHuff__01gas_1(%rip), %r13
 
LDecHuffCheckLen00gas_1:  
    mov          (80)(%rsp), %rcx
    mov          %r12, %rax
    sub          %r12, %rcx
    add          (72)(%rsp), %rax
    cmp          $(8), %ecx
    jl           LDecHuffExitEndOfWork00gas_1
 
    mov          (%rax), %r10
    bswap        %r10
    movd         %r10, %xmm1
    pcmpeqb      (128)(%rsi), %xmm1
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
    jmp          *%r13
 
 
LDecHuff__Call01gas_1:  
    lea          LDecHuff__02gas_1(%rip), %r13
    jmp          LDecHuffCheckLen00gas_1
 
LDecHuffLongCall00gas_1:  
    lea          LDecHuffLong01gas_1(%rip), %r13
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
    sub          (72)(%rsp), %rax
    mov          %rax, %r12
    mov          %rcx, %r11
    mov          $(64), %ecx
    sub          %ebp, %ecx
    shl          %cl, %r11
    mov          %r11, %rcx
    shr          $(56), %rcx
    jmp          *%r13
 
 
LDecHuff__Long00gas_1:  
    lea          LDecHuff__RetLonggas_1(%rip), %r14
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
    jmp          *%r14
 
 
