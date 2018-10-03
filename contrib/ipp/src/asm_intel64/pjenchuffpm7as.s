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
 
 
.globl mfxownpj_EncodeHuffman8x8_ACFlush_JPEG_16s1u_C1

 
mfxownpj_EncodeHuffman8x8_ACFlush_JPEG_16s1u_C1:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    push         %r12
 
 
    push         %r13
 
 
    push         %r14
 
 
    push         %r15
 
 
    sub          $(104), %rsp
 
 
    mov          %rdi, (48)(%rsp)
    mov          %esi, %esi
    mov          %rsi, (56)(%rsp)
    mov          %rdx, (64)(%rsp)
    mov          %rcx, (72)(%rsp)
    mov          %r8, (80)(%rsp)
 
 
    mov          $(0), %ebx
    mov          %rbx, (8)(%rsp)
 
    mov          (64)(%rsp), %rbx
    mov          (%rbx), %ebx
    mov          %rbx, (%rsp)
 
    mov          (80)(%rsp), %rcx
    lea          (8)(%rcx), %rbx
    mov          $(64), %ebp
    sub          (%rbx), %ebp
 
    lea          (%rcx), %rbx
    movq         (%rbx), %mm0
    lea          (12)(%rcx), %rcx
    mov          (%rcx), %ecx
    mov          %ecx, (24)(%rsp)
 
    mov          (72)(%rsp), %rbx
    lea          (%rbx), %rbx
    mov          %rbx, (16)(%rsp)
 
 
    mov          (24)(%rsp), %ebx
    test         %ebx, %ebx
    jz           LAppendFlushEncAc00gas_1
 
 
    cmp          $(1), %ebx
    jne          LEobEncFlushAc03gas_1
 
    xor          %ebx, %ebx
    lea          LEobEncFlushAc12gas_1(%rip), %rsi
    jmp          LHuffEobEncFlushAc00gas_1
 
 
LEobEncFlushAc03gas_1:  
    cmp          $(32767), %ebx
    jle          LEobEncFlushAc06gas_1
 
    mov          $(224), %ebx
    lea          LEobEncFlushAc04gas_1(%rip), %rsi
    jmp          LHuffEobEncFlushAc00gas_1
 
LEobEncFlushAc04gas_1:  
    cmp          $(14), %ebp
    jge          LEobEncFlushAc05gas_1
 
    lea          LEobEncFlushAc05gas_1(%rip), %rbx
    mov          %rbx, (32)(%rsp)
    jmp          LWriteEncFlushAc00gas_1
 
LEobEncFlushAc05gas_1:  
    mov          $(32767), %esi
    mov          (24)(%rsp), %ebx
    psllq        $(14), %mm0
    and          $(16383), %ebx
    sub          %esi, (24)(%rsp)
    movd         %ebx, %mm1
    sub          $(14), %ebp
    por          %mm1, %mm0
    mov          (24)(%rsp), %ebx
 
 
LEobEncFlushAc06gas_1:  
    lea          eobSize(%rip), %rsi
 
    cmp          $(256), %ebx
    jl           LEobEncFlushAc07gas_1
 
    shr          $(8), %ebx
    movzbl       (%rsi,%rbx), %ebx
    add          $(8), %ebx
    jmp          LEobEncFlushAc08gas_1
 
LEobEncFlushAc07gas_1:  
    movzbl       (%rsi,%rbx), %ebx
 
LEobEncFlushAc08gas_1:  
    cmp          $(14), %ebx
    jg           LErrorEncFlushAc00gas_1
 
    movd         %ebx, %mm5
    shl          $(4), %ebx
    lea          LEobEncFlushAc09gas_1(%rip), %rsi
    jmp          LHuffEobEncFlushAc00gas_1
 
LEobEncFlushAc09gas_1:  
    movd         %mm5, %esi
    cmp          %esi, %ebp
    jge          LEobEncFlushAc11gas_1
 
    lea          LEobEncFlushAc11gas_1(%rip), %rbx
    mov          %rbx, (32)(%rsp)
    jmp          LWriteEncFlushAc00gas_1
 
LEobEncFlushAc11gas_1:  
    movd         %esi, %mm1
    mov          $(32), %ecx
    sub          %esi, %ecx
    mov          (24)(%rsp), %ebx
    shl          %cl, %ebx
    shr          %cl, %ebx
    psllq        %mm1, %mm0
    movd         %ebx, %mm1
    sub          %esi, %ebp
    por          %mm1, %mm0
 
 
LEobEncFlushAc12gas_1:  
    xor          %esi, %esi
    mov          %esi, (24)(%rsp)
    jmp          LAppendFlushEncAc00gas_1
 
 
LHuffEobEncFlushAc00gas_1:  
    mov          (16)(%rsp), %rax
    movd         (%rax,%rbx,4), %mm7
    movd         (%rax,%rbx,4), %mm6
    psrld        $(16), %mm7
    movd         %mm7, %eax
    pand         Mask16(%rip), %mm6
    test         %eax, %eax
    jz           LErrorEncFlushAc01gas_1
    cmp          %ebp, %eax
    jle          LHuffEobEncFlushAc01gas_1
    lea          LHuffEobEncFlushAc01gas_1(%rip), %rbx
    mov          %rbx, (32)(%rsp)
    jmp          LWriteEncFlushAc00gas_1
LHuffEobEncFlushAc01gas_1:  
    psllq        %mm7, %mm0
    por          %mm6, %mm0
    sub          %eax, %ebp
    jmp          *%rsi
 
 
LAppendFlushEncAc00gas_1:  
    mov          (80)(%rsp), %rcx
    lea          (16)(%rcx), %rax
    mov          (%rax), %eax
    lea          (20)(%rcx), %rdi
 
LAppendFlushEncAc01gas_1:  
    test         %eax, %eax
    jz           LEndWriteFlushEncAc00gas_1
 
    cmp          $(1), %ebp
    jge          LAppendFlushEncAc02gas_1
    lea          LAppendFlushEncAc02gas_1(%rip), %rbx
    mov          %rbx, (32)(%rsp)
    jmp          LWriteEncFlushAc00gas_1
 
LAppendFlushEncAc02gas_1:  
    movzbl       (%rdi), %esi
    psllq        $(1), %mm0
    and          $(1), %esi
    movd         %esi, %mm1
    add          $(1), %rdi
    sub          $(1), %eax
    sub          $(1), %ebp
    por          %mm1, %mm0
    jmp          LAppendFlushEncAc01gas_1
 
 
LEndWriteFlushEncAc00gas_1:  
    cmp          $(64), %ebp
    je           LInitFlushEncAc00gas_1
 
    mov          %ebp, %ecx
    and          $(7), %ecx
    jz           LEndWriteFlushEncAc01gas_1
 
    movd         %ecx, %mm1
    sub          %ecx, %ebp
    mov          $(1), %ebx
    shl          %cl, %ebx
    sub          $(1), %ebx
    psllq        %mm1, %mm0
    movd         %ebx, %mm1
    por          %mm1, %mm0
 
LEndWriteFlushEncAc01gas_1:  
    lea          LInitFlushEncAc00gas_1(%rip), %rbx
    mov          %rbx, (32)(%rsp)
    jmp          LWriteEncFlushAc00gas_1
 
 
LInitFlushEncAc00gas_1:  
    xor          %eax, %eax
    mov          (80)(%rsp), %rcx
    lea          (8)(%rcx), %rbx
    mov          %eax, (%rbx)
 
    pxor         %mm0, %mm0
    lea          (%rcx), %rbx
    movq         %mm0, (%rbx)
 
    lea          (16)(%rcx), %rbx
    mov          %eax, (%rbx)
 
    mov          (24)(%rsp), %eax
    lea          (12)(%rcx), %rbx
    mov          %eax, (%rbx)
 
    xorps        %xmm0, %xmm0
    lea          (20)(%rcx), %rdi
    mov          $(1008), %ebx
    movups       %xmm0, (%rdi)
    movups       %xmm0, (%rdi,%rbx)
    and          $(-16), %rdi
    add          $(16), %rdi
    mov          $(15), %ebx
 
LInitFlushEncAc01gas_1:  
    movaps       %xmm0, (%rdi)
    movaps       %xmm0, (16)(%rdi)
    movaps       %xmm0, (32)(%rdi)
    movaps       %xmm0, (48)(%rdi)
    add          $(64), %rdi
    sub          $(1), %ebx
    jnz          LInitFlushEncAc01gas_1
 
    movaps       %xmm0, (%rdi)
    movaps       %xmm0, (16)(%rdi)
    movaps       %xmm0, (32)(%rdi)
 
    mov          (64)(%rsp), %rbx
    mov          (%rsp), %ecx
    mov          %ecx, (%rbx)
 
 
    mov          (8)(%rsp), %rax
    emms
    add          $(104), %rsp
 
 
    pop          %r15
 
 
    pop          %r14
 
 
    pop          %r13
 
 
    pop          %r12
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
LWriteEncFlushAc00gas_1:  
    movd         %ebp, %mm1
    sub          $(64), %ebp
    movq         %mm0, %mm2
    neg          %ebp
    psllq        %mm1, %mm2
    mov          (%rsp), %ecx
    mov          (48)(%rsp), %rdx
    movq         %mm2, %mm1
 
LWriteEncFlushAc11gas_1:  
    sub          $(8), %ebp
    jl           LWriteEncFlushAc12gas_1
 
    cmp          (56)(%rsp), %ecx
    jge          LErrorEncFlushAc02gas_1
    psrlq        $(56), %mm2
    psllq        $(8), %mm1
    movd         %mm2, %ebx
    movq         %mm1, %mm2
    mov          %bl, (%rdx,%rcx)
    add          $(1), %ecx
    cmp          $(255), %ebx
    jne          LWriteEncFlushAc11gas_1
    cmp          (56)(%rsp), %ecx
    je           LErrorEncFlushAc03gas_1
    xor          %ebx, %ebx
    mov          %bl, (%rdx,%rcx)
    add          $(1), %ecx
    jmp          LWriteEncFlushAc11gas_1
 
LWriteEncFlushAc12gas_1:  
    sub          $(56), %ebp
    mov          %ecx, (%rsp)
    neg          %ebp
    mov          (32)(%rsp), %rbx
    jmp          *%rbx
 
 
LErrorEncFlushAc00gas_1:  
    mov          $(-63), %rbx
    mov          %rbx, (8)(%rsp)
    jmp          LEndWriteFlushEncAc00gas_1
 
LErrorEncFlushAc01gas_1:  
    mov          $(-64), %rbx
    mov          %rbx, (8)(%rsp)
    jmp          LEndWriteFlushEncAc00gas_1
 
LErrorEncFlushAc02gas_1:  
    sub          $(56), %ebp
    mov          %ecx, (%rsp)
    neg          %ebp
    mov          $(-62), %rbx
    mov          %rbx, (8)(%rsp)
    jmp          LInitFlushEncAc00gas_1
 
LErrorEncFlushAc03gas_1:  
    sub          $(64), %ebp
    mov          %ecx, (%rsp)
    neg          %ebp
    mov          $(-62), %rbx
    mov          %rbx, (8)(%rsp)
    jmp          LInitFlushEncAc00gas_1
 
 
.p2align 4, 0x90
 
 
.globl mfxownpj_EncodeHuffman8x8_ACFirst_JPEG_16s1u_C1

 
mfxownpj_EncodeHuffman8x8_ACFirst_JPEG_16s1u_C1:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    push         %r12
 
 
    push         %r13
 
 
    push         %r14
 
 
    push         %r15
 
 
    sub          $(136), %rsp
 
 
    mov          %rdi, (80)(%rsp)
    mov          %rsi, (88)(%rsp)
    mov          %edx, %edx
    mov          %rdx, (96)(%rsp)
    mov          %rcx, (104)(%rsp)
    mov          %r8d, %r8d
    mov          %r8, (112)(%rsp)
    mov          %r9d, %r9d
    mov          %r9, (120)(%rsp)
    mov          (192)(%rsp), %eax
    mov          %rax, (192)(%rsp)
 
 
    mov          $(0), %rbx
    mov          %rbx, (16)(%rsp)
 
    mov          (104)(%rsp), %rbx
    mov          (%rbx), %ebx
    mov          %rbx, (%rsp)
 
    movq         (208)(%rsp), %rcx
    lea          (8)(%rcx), %rbx
    mov          $(64), %ebp
    sub          (%rbx), %ebp
 
    lea          (%rcx), %rbx
    movq         (%rbx), %mm0
    lea          (12)(%rcx), %rcx
    mov          (%rcx), %ecx
    mov          %ecx, (32)(%rsp)
 
    movq         (200)(%rsp), %rbx
    lea          (%rbx), %rbx
    mov          %rbx, (24)(%rsp)
 
    mov          (120)(%rsp), %rbx
    sub          $(63), %rbx
    lea          ownZigzag(%rip), %rsi
    sub          %rbx, %rsi
    mov          %rsi, (8)(%rsp)
    mov          (120)(%rsp), %edi
    sub          (112)(%rsp), %edi
    mov          (80)(%rsp), %rcx
    xor          %ebx, %ebx
    movzbl       (%rdi,%rsi), %edx
    jmp          LEncFirstAc00gas_2
 
 
LEncFirstAc01gas_2:  
    mov          (80)(%rsp), %rcx
    add          $(1), %ebx
    movzbl       (-1)(%rdi,%rsi), %edx
    sub          $(1), %edi
    jl           LLastZerosEncFirstAc00gas_2
 
LEncFirstAc00gas_2:  
    movswl       (%rdx,%rcx), %eax
    cmp          $(0), %eax
    jnz          LEncFirstAc02gas_2
    movzbl       (-1)(%rdi,%rsi), %edx
    add          $(1), %ebx
    sub          $(1), %edi
    jl           LLastZerosEncFirstAc00gas_2
    movswl       (%rdx,%rcx), %eax
    cmp          $(0), %eax
    jnz          LEncFirstAc02gas_2
    movzbl       (-1)(%rdi,%rsi), %edx
    add          $(1), %ebx
    sub          $(1), %edi
    jl           LLastZerosEncFirstAc00gas_2
    movswl       (%rdx,%rcx), %eax
    cmp          $(0), %eax
    jnz          LEncFirstAc02gas_2
    movzbl       (-1)(%rdi,%rsi), %edx
    add          $(1), %ebx
    sub          $(1), %edi
    jl           LLastZerosEncFirstAc00gas_2
    movswl       (%rdx,%rcx), %eax
    cmp          $(0), %eax
    jnz          LEncFirstAc02gas_2
    movzbl       (-1)(%rdi,%rsi), %edx
    add          $(1), %ebx
    sub          $(1), %edi
    jl           LLastZerosEncFirstAc00gas_2
    jmp          LEncFirstAc00gas_2
 
 
LEncFirstAc02gas_2:  
    mov          (192)(%rsp), %ecx
    cdq
    xor          %edx, %eax
    sub          %edx, %eax
    shr          %cl, %eax
    xor          %eax, %edx
    test         %eax, %eax
    jz           LEncFirstAc01gas_2
 
    mov          (32)(%rsp), %ecx
    test         %ecx, %ecx
    jne          LEobEncFirstAc01gas_2
 
LEncFirstAc03gas_2:  
    cmp          $(15), %ebx
    jg           LZeros16EncFirstAc00gas_2
 
LEncFirstAc04gas_2:  
    bsr          %eax, %eax
    mov          (24)(%rsp), %rcx
    shl          $(4), %ebx
    add          $(1), %eax
    add          %eax, %ebx
 
    cmp          $(11), %eax
    jg           LErrorEncFirstAc00gas_2
 
    cmp          $(256), %ebx
    jg           LErrorEncFirstAc01gas_2
 
 
    movd         (%rcx,%rbx,4), %mm7
    movd         (%rcx,%rbx,4), %mm6
    lea          TableMask12(%rip), %rbx
    psrld        $(16), %mm7
    pand         Mask16(%rip), %mm6
    movd         %eax, %mm1
    and          (%rbx,%rax,4), %edx
    movd         %mm7, %eax
    paddd        %mm1, %mm7
    psllq        %mm1, %mm6
    movd         %edx, %mm2
    test         %eax, %eax
    jz           LErrorEncFirstAc02gas_2
    movd         %mm7, %eax
    por          %mm2, %mm6
    cmp          %ebp, %eax
    jg           LWriteEncFirstAc01gas_2
LEncFirstAc05gas_2:  
    psllq        %mm7, %mm0
    sub          %eax, %ebp
    mov          (80)(%rsp), %rcx
    por          %mm6, %mm0
    xor          %ebx, %ebx
    movzbl       (-1)(%rdi,%rsi), %edx
    sub          $(1), %edi
    jge          LEncFirstAc00gas_2
    jmp          LExitEncFirstAc00gas_2
 
 
LWriteEncFirstAc01gas_2:  
    lea          LEncFirstAc05gas_2(%rip), %rbx
    mov          %rbx, (40)(%rsp)
 
LWriteEncFirstAc00gas_2:  
    movd         %ebp, %mm1
    sub          $(64), %ebp
    movq         %mm0, %mm2
    neg          %ebp
    psllq        %mm1, %mm2
    movq         maskFF(%rip), %mm1
    mov          (%rsp), %ebx
    mov          (%rsp), %ecx
    sub          (96)(%rsp), %ebx
    mov          (88)(%rsp), %rdx
    cmp          $(-8), %ebx
    jg           LWriteEncFirstAc10gas_2
    cmp          $(32), %ebp
    jl           LWriteEncFirstAc10gas_2
 
    pcmpeqb      %mm2, %mm1
    pmovmskb     %mm1, %ebx
    movq         %mm2, %mm1
    test         %ebx, %ebx
    jnz          LWriteEncFirstAc11gas_2
    movd         %mm2, %ebx
    psrlq        $(32), %mm1
    bswap        %ebx
    mov          %ebx, (4)(%rdx,%rcx)
    movd         %mm1, %ebx
    bswap        %ebx
    mov          %ebx, (%rdx,%rcx)
    mov          %ebp, %ebx
    and          $(7), %ebp
    and          $(4294967288), %ebx
    sub          $(64), %ebp
    neg          %ebp
    shr          $(3), %ebx
    add          %ebx, (%rsp)
    mov          (40)(%rsp), %rbx
    jmp          *%rbx
 
 
LWriteEncFirstAc10gas_2:  
    movq         %mm2, %mm1
 
LWriteEncFirstAc11gas_2:  
    sub          $(8), %ebp
    jl           LWriteEncFirstAc12gas_2
 
    cmp          (96)(%rsp), %ecx
    jge          LErrorEncFirstAc03gas_2
    psrlq        $(56), %mm2
    psllq        $(8), %mm1
    movd         %mm2, %ebx
    movq         %mm1, %mm2
    mov          %bl, (%rdx,%rcx)
    add          $(1), %ecx
    cmp          $(255), %ebx
    jne          LWriteEncFirstAc11gas_2
    cmp          (96)(%rsp), %ecx
    je           LErrorEncFirstAc04gas_2
    xor          %ebx, %ebx
    mov          %bl, (%rdx,%rcx)
    add          $(1), %ecx
    jmp          LWriteEncFirstAc11gas_2
 
LWriteEncFirstAc12gas_2:  
    sub          $(56), %ebp
    mov          %ecx, (%rsp)
    neg          %ebp
    mov          (40)(%rsp), %rbx
    jmp          *%rbx
 
 
LExitEncFirstAc00gas_2:  
    mov          (104)(%rsp), %rbx
    mov          (%rsp), %ecx
    mov          %ecx, (%rbx)
 
    movq         (208)(%rsp), %rcx
    lea          (8)(%rcx), %rbx
    sub          $(64), %ebp
    neg          %ebp
    mov          %ebp, (%rbx)
 
    lea          (%rcx), %rbx
    movq         %mm0, (%rbx)
    mov          (32)(%rsp), %ebx
    lea          (12)(%rcx), %rcx
    mov          %ebx, (%rcx)
 
    mov          (16)(%rsp), %rax
    emms
    add          $(136), %rsp
 
 
    pop          %r15
 
 
    pop          %r14
 
 
    pop          %r13
 
 
    pop          %r12
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
LZeros16EncFirstAc00gas_2:  
    mov          (24)(%rsp), %rcx
    movd         (960)(%rcx), %mm7
    movd         (960)(%rcx), %mm6
    psrld        $(16), %mm7
    movd         %mm7, %ecx
    pand         Mask16(%rip), %mm6
    test         %ecx, %ecx
    jz           LErrorEncFirstAc02gas_2
LZeros16EncFirstAc01gas_2:  
    cmp          %ebp, %ecx
    jg           LZeros16EncFirstAc03gas_2
LZeros16EncFirstAc02gas_2:  
    psllq        %mm7, %mm0
    por          %mm6, %mm0
    sub          %ecx, %ebp
    sub          $(16), %ebx
    cmp          $(16), %ebx
    jl           LEncFirstAc04gas_2
    jmp          LZeros16EncFirstAc01gas_2
 
LZeros16EncFirstAc03gas_2:  
    mov          %rbx, (56)(%rsp)
    mov          %rdx, (64)(%rsp)
    mov          %rcx, (72)(%rsp)
    lea          LZeros16EncFirstAc30gas_2(%rip), %rbx
    mov          %rbx, (40)(%rsp)
    jmp          LWriteEncFirstAc00gas_2
LZeros16EncFirstAc30gas_2:  
    mov          (56)(%rsp), %rbx
    mov          (64)(%rsp), %rdx
    mov          (72)(%rsp), %rcx
    jmp          LZeros16EncFirstAc01gas_2
 
 
LLastZerosEncFirstAc00gas_2:  
    test         %ebx, %ebx
    jz           LExitEncFirstAc00gas_2
 
    mov          (32)(%rsp), %ebx
    add          $(1), %ebx
    mov          %ebx, (32)(%rsp)
    cmp          $(32767), %ebx
    jne          LExitEncFirstAc00gas_2
 
    lea          LExitEncFirstAc00gas_2(%rip), %rsi
    jmp          LEobEncFirstAc02gas_2
 
 
LEobEncFirstAc10gas_2:  
    mov          (8)(%rsp), %rsi
    jmp          LEncFirstAc03gas_2
LEobEncFirstAc01gas_2:  
    lea          LEobEncFirstAc10gas_2(%rip), %rsi
 
 
LEobEncFirstAc02gas_2:  
    mov          %rsi, (48)(%rsp)
    mov          %rbx, (56)(%rsp)
    mov          %rax, (64)(%rsp)
    mov          %rdx, (72)(%rsp)
 
    mov          (32)(%rsp), %ebx
    test         %ebx, %ebx
    jz           LEobEncFirstAc13gas_2
 
 
    cmp          $(1), %ebx
    jne          LEobEncFirstAc03gas_2
 
    xor          %ebx, %ebx
    lea          LEobEncFirstAc12gas_2(%rip), %rsi
    jmp          LHuffEobEncFirstAc00gas_2
 
 
LEobEncFirstAc03gas_2:  
    cmp          $(32767), %ebx
    jle          LEobEncFirstAc06gas_2
 
    mov          $(224), %ebx
    lea          LEobEncFirstAc04gas_2(%rip), %rsi
    jmp          LHuffEobEncFirstAc00gas_2
 
LEobEncFirstAc04gas_2:  
    cmp          $(14), %ebp
    jge          LEobEncFirstAc05gas_2
 
    lea          LEobEncFirstAc05gas_2(%rip), %rbx
    mov          %rbx, (40)(%rsp)
    jmp          LWriteEncFirstAc00gas_2
 
LEobEncFirstAc05gas_2:  
    mov          $(32767), %esi
    mov          (32)(%rsp), %ebx
    psllq        $(14), %mm0
    and          $(16383), %ebx
    sub          %esi, (32)(%rsp)
    movd         %ebx, %mm1
    sub          $(14), %ebp
    por          %mm1, %mm0
    mov          (32)(%rsp), %ebx
 
 
LEobEncFirstAc06gas_2:  
    lea          eobSize(%rip), %rsi
 
    cmp          $(256), %ebx
    jl           LEobEncFirstAc07gas_2
 
    shr          $(8), %ebx
    movzbl       (%rsi,%rbx), %ebx
    add          $(8), %ebx
    jmp          LEobEncFirstAc08gas_2
 
LEobEncFirstAc07gas_2:  
    movzbl       (%rsi,%rbx), %ebx
 
LEobEncFirstAc08gas_2:  
    cmp          $(14), %ebx
    jg           LErrorEncFirstAc00gas_2
 
    movd         %ebx, %mm5
    shl          $(4), %ebx
    lea          LEobEncFirstAc09gas_2(%rip), %rsi
    jmp          LHuffEobEncFirstAc00gas_2
 
LEobEncFirstAc09gas_2:  
    movd         %mm5, %esi
    cmp          %esi, %ebp
    jge          LEobEncFirstAc11gas_2
 
    lea          LEobEncFirstAc11gas_2(%rip), %rbx
    mov          %rbx, (40)(%rsp)
    jmp          LWriteEncFirstAc00gas_2
 
LEobEncFirstAc11gas_2:  
    movd         %esi, %mm1
    mov          $(32), %ecx
    sub          %esi, %ecx
    mov          (32)(%rsp), %ebx
    shl          %cl, %ebx
    shr          %cl, %ebx
    psllq        %mm1, %mm0
    movd         %ebx, %mm1
    sub          %esi, %ebp
    por          %mm1, %mm0
 
 
LEobEncFirstAc12gas_2:  
    xor          %esi, %esi
    mov          %esi, (32)(%rsp)
LEobEncFirstAc13gas_2:  
    mov          (56)(%rsp), %rbx
    mov          (64)(%rsp), %rax
    mov          (72)(%rsp), %rdx
    mov          (48)(%rsp), %rsi
    jmp          *%rsi
 
 
LHuffEobEncFirstAc00gas_2:  
    mov          (24)(%rsp), %rax
    movd         (%rax,%rbx,4), %mm7
    movd         (%rax,%rbx,4), %mm6
    psrld        $(16), %mm7
    movd         %mm7, %eax
    pand         Mask16(%rip), %mm6
    test         %eax, %eax
    jz           LErrorEncFirstAc02gas_2
    cmp          %ebp, %eax
    jle          LHuffEobEncFirstAc01gas_2
    lea          LHuffEobEncFirstAc01gas_2(%rip), %rbx
    mov          %rbx, (40)(%rsp)
    jmp          LWriteEncFirstAc00gas_2
LHuffEobEncFirstAc01gas_2:  
    psllq        %mm7, %mm0
    por          %mm6, %mm0
    sub          %eax, %ebp
    jmp          *%rsi
 
 
LErrorEncFirstAc00gas_2:  
    mov          $(-63), %rbx
    mov          %rbx, (16)(%rsp)
    jmp          LExitEncFirstAc00gas_2
 
LErrorEncFirstAc01gas_2:  
    mov          $(-2), %rbx
    mov          %rbx, (16)(%rsp)
    jmp          LExitEncFirstAc00gas_2
 
LErrorEncFirstAc02gas_2:  
    mov          $(-64), %rbx
    mov          %rbx, (16)(%rsp)
    jmp          LExitEncFirstAc00gas_2
 
LErrorEncFirstAc03gas_2:  
    sub          $(56), %ebp
    mov          %ecx, (%rsp)
    neg          %ebp
    mov          $(-62), %rbx
    mov          %rbx, (16)(%rsp)
    jmp          LExitEncFirstAc00gas_2
 
LErrorEncFirstAc04gas_2:  
    sub          $(64), %ebp
    mov          %ecx, (%rsp)
    neg          %ebp
    mov          $(-62), %rbx
    mov          %rbx, (16)(%rsp)
    jmp          LExitEncFirstAc00gas_2
 
 
.p2align 4, 0x90
 
 
.globl mfxownpj_EncodeHuffman8x8_ACRefine_JPEG_16s1u_C1

 
mfxownpj_EncodeHuffman8x8_ACRefine_JPEG_16s1u_C1:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    push         %r12
 
 
    push         %r13
 
 
    push         %r14
 
 
    push         %r15
 
 
    sub          $(680), %rsp
 
 
    mov          %rdi, (112)(%rsp)
    mov          %rsi, (120)(%rsp)
    mov          %edx, %edx
    mov          %rdx, (128)(%rsp)
    mov          %rcx, (136)(%rsp)
    mov          %r8d, %r8d
    mov          %r8, (144)(%rsp)
    mov          %r9d, %r9d
    mov          %r9, (152)(%rsp)
    mov          (736)(%rsp), %eax
    mov          %rax, (736)(%rsp)
 
 
    mov          $(0), %rbx
    mov          %rbx, (%rsp)
 
    mov          (136)(%rsp), %rbx
    mov          (%rbx), %ebx
    mov          %rbx, (8)(%rsp)
 
    movq         (752)(%rsp), %rcx
    lea          (8)(%rcx), %rbx
    mov          $(64), %ebp
    sub          (%rbx), %ebp
 
    lea          (%rcx), %rbx
    movq         (%rbx), %mm0
    lea          (12)(%rcx), %rbx
    mov          (%rbx), %ebx
    mov          %ebx, (32)(%rsp)
    mov          %ebx, (104)(%rsp)
 
    lea          (16)(%rcx), %rbx
    mov          (%rbx), %ebx
    mov          %rbx, (40)(%rsp)
    or           %ebx, (104)(%rsp)
    lea          (20)(%rcx), %rcx
    mov          %rcx, (48)(%rsp)
 
    movq         (744)(%rsp), %rbx
    lea          (%rbx), %rbx
    mov          %rbx, (24)(%rsp)
 
    mov          (152)(%rsp), %rbx
    sub          $(63), %rbx
    lea          ownZigzag(%rip), %rsi
    sub          %rbx, %rsi
    mov          (152)(%rsp), %edi
    sub          (144)(%rsp), %edi
    mov          (112)(%rsp), %rcx
 
 
    movd         %edi, %mm7
    mov          $(64), %ebx
    movzbl       (%rdi,%rsi), %edx
 
    movswl       (%rdx,%rcx), %eax
    cdq
    mov          (736)(%rsp), %ecx
    xor          %edx, %eax
    sub          %edx, %eax
    add          $(1), %edx
    shr          %cl, %eax
    mov          (112)(%rsp), %rcx
    mov          %eax, (160)(%rsp,%rdi,4)
    mov          %edx, (416)(%rsp,%rdi,4)
    cmp          $(1), %eax
    cmove        %edi, %ebx
    movzbl       (-1)(%rdi,%rsi), %edx
    sub          $(1), %edi
    jl           LEncRefineAc00gas_3
LAbsEncRefineAc00gas_3:  
    movswl       (%rdx,%rcx), %eax
    cdq
    mov          (736)(%rsp), %ecx
    xor          %edx, %eax
    sub          %edx, %eax
    add          $(1), %edx
    shr          %cl, %eax
    mov          (112)(%rsp), %rcx
    mov          %eax, (160)(%rsp,%rdi,4)
    mov          %edx, (416)(%rsp,%rdi,4)
    cmp          $(1), %eax
    cmove        %edi, %ebx
    movzbl       (-1)(%rdi,%rsi), %edx
    sub          $(1), %edi
    jl           LEncRefineAc00gas_3
    movswl       (%rdx,%rcx), %eax
    cdq
    mov          (736)(%rsp), %ecx
    xor          %edx, %eax
    sub          %edx, %eax
    add          $(1), %edx
    shr          %cl, %eax
    mov          (112)(%rsp), %rcx
    mov          %eax, (160)(%rsp,%rdi,4)
    mov          %edx, (416)(%rsp,%rdi,4)
    cmp          $(1), %eax
    cmove        %edi, %ebx
    movzbl       (-1)(%rdi,%rsi), %edx
    sub          $(1), %edi
    jge          LAbsEncRefineAc00gas_3
 
 
LEncRefineAc00gas_3:  
    mov          %ebx, (16)(%rsp)
    movd         %mm7, %edi
    xor          %esi, %esi
    mov          (48)(%rsp), %rcx
    add          (40)(%rsp), %rcx
    xor          %edx, %edx
    xor          %eax, %eax
    jmp          LEncRefineAc01gas_3
 
LEncRefineAc02gas_3:  
    and          $(1), %eax
    mov          %al, (%rdx,%rcx)
    xor          %eax, %eax
    add          $(1), %edx
    sub          $(1), %edi
    jl           LLastZerosEncRefineAc00gas_3
 
LEncRefineAc01gas_3:  
    add          (160)(%rsp,%rdi,4), %eax
    jnz          LEncRefineAc03gas_3
    add          $(1), %esi
    sub          $(1), %edi
    jl           LLastZerosEncRefineAc00gas_3
    add          (160)(%rsp,%rdi,4), %eax
    jnz          LEncRefineAc03gas_3
    add          $(1), %esi
    sub          $(1), %edi
    jl           LLastZerosEncRefineAc00gas_3
    add          (160)(%rsp,%rdi,4), %eax
    jnz          LEncRefineAc03gas_3
    add          $(1), %esi
    sub          $(1), %edi
    jl           LLastZerosEncRefineAc00gas_3
    jmp          LEncRefineAc01gas_3
 
LEncRefineAc03gas_3:  
    cmp          $(15), %esi
    jg           LZeros16EncRefinetAc00gas_3
 
LEncRefineAc04gas_3:  
    cmp          $(1), %eax
    jg           LEncRefineAc02gas_3
 
    cmp          (104)(%rsp), %eax
    jle          LEncRefineAc55gas_3
 
 
LEncRefineAc06gas_3:  
    add          %esi, %esi
    mov          (24)(%rsp), %rax
    lea          (1)(,%rsi,8), %rsi
    movzwl       (%rax,%rsi,4), %ebx
    movzwl       (2)(%rax,%rsi,4), %eax
    test         %eax, %eax
    jz           LErrorEncRefineAc02gas_3
    add          %ebx, %ebx
    add          $(1), %eax
    add          (416)(%rsp,%rdi,4), %ebx
    movd         %eax, %mm7
    movd         %ebx, %mm6
    cmp          %ebp, %eax
    jg           LCallWriteEncRefineAc00gas_3
LEncRefineAc07gas_3:  
    psllq        %mm7, %mm0
    sub          %eax, %ebp
    por          %mm6, %mm0
    test         %edx, %edx
    jz           LEncRefineAc11gas_3
 
    test         %edx, %edx
    jz           LEncRefineAc11gas_3
 
    cmp          $(32), %edx
    jg           LCallAppendEncRefineAc03gas_3
 
 
    cmp          %ebp, %edx
    jg           LCallWriteEncRefineAc01gas_3
LEncRefineAc08gas_3:  
    movd         %edx, %mm7
    psllq        %mm7, %mm0
    sub          %edx, %ebp
    movzbl       (%rcx), %ebx
    add          $(1), %rcx
    and          $(1), %ebx
    sub          $(1), %edx
    jz           LEncRefineAc10gas_3
LEncRefineAc09gas_3:  
    add          %ebx, %ebx
    movzbl       (%rcx), %eax
    add          $(1), %rcx
    and          $(1), %eax
    or           %eax, %ebx
    sub          $(1), %edx
    jz           LEncRefineAc10gas_3
    add          %ebx, %ebx
    movzbl       (%rcx), %eax
    add          $(1), %rcx
    and          $(1), %eax
    or           %eax, %ebx
    sub          $(1), %edx
    jnz          LEncRefineAc09gas_3
LEncRefineAc10gas_3:  
    movd         %ebx, %mm6
    por          %mm6, %mm0
 
 
LEncRefineAc11gas_3:  
    xor          %esi, %esi
    mov          (48)(%rsp), %rcx
    xor          %edx, %edx
    xor          %eax, %eax
    sub          $(1), %edi
    jge          LEncRefineAc01gas_3
 
 
LLastZerosEncRefineAc00gas_3:  
    or           %edx, %esi
    jz           LExitEncRefineAc00gas_3
 
    mov          $(1), %eax
    add          %eax, (32)(%rsp)
    mov          (40)(%rsp), %eax
    add          %edx, %eax
    mov          (32)(%rsp), %ebx
 
    cmp          $(937), %eax
    jg           LLastZerosEncRefineAc01gas_3
 
    cmp          $(32767), %ebx
    je           LLastZerosEncRefineAc01gas_3
 
    jmp          LLastZerosEncRefineAc03gas_3
 
LLastZerosEncRefineAc01gas_3:  
    test         %ebx, %ebx
    jz           LLastZerosEncRefineAc02gas_3
 
    lea          LLastZerosEncRefineAc02gas_3(%rip), %rcx
    mov          %rcx, (64)(%rsp)
    jmp          LEobEncRefineAc00gas_3
 
LLastZerosEncRefineAc02gas_3:  
    test         %eax, %eax
    jnz          LCallAppendEncRefineAc04gas_3
 
LLastZerosEncRefineAc03gas_3:  
    mov          %eax, (40)(%rsp)
    jmp          LExitEncRefineAc00gas_3
 
 
LEncRefineAc55gas_3:  
    mov          (32)(%rsp), %ebx
    test         %ebx, %ebx
    jnz          LCallEobEncRefineAc00gas_3
 
LEncRefineAc05gas_3:  
    mov          (40)(%rsp), %eax
    test         %eax, %eax
    jnz          LCallAppendEncRefineAc00gas_3
    jmp          LEncRefineAc06gas_3
 
 
LRetWriteEncRefineAc03gas_3:  
    mov          (72)(%rsp), %rcx
    mov          (80)(%rsp), %rdx
    mov          (88)(%rsp), %rbx
    jmp          LZeros16EncRefinetAc04gas_3
LCallWriteEncRefineAc03gas_3:  
    mov          %rcx, (72)(%rsp)
    mov          %rdx, (80)(%rsp)
    mov          %rbx, (88)(%rsp)
    lea          LRetWriteEncRefineAc03gas_3(%rip), %rbx
    mov          %rbx, (56)(%rsp)
    jmp          LWriteEncRefineAc00gas_3
 
LCallWriteEncRefineAc02gas_3:  
    lea          LAppendEncRefineAc01gas_3(%rip), %rbx
    mov          %rbx, (56)(%rsp)
    jmp          LWriteEncRefineAc00gas_3
 
LRetWriteEncRefineAc01gas_3:  
    mov          (72)(%rsp), %rcx
    mov          (80)(%rsp), %rdx
    jmp          LEncRefineAc08gas_3
LCallWriteEncRefineAc01gas_3:  
    mov          %rcx, (72)(%rsp)
    mov          %rdx, (80)(%rsp)
    lea          LRetWriteEncRefineAc01gas_3(%rip), %rbx
    mov          %rbx, (56)(%rsp)
    jmp          LWriteEncRefineAc00gas_3
 
LRetWriteEncRefineAc00gas_3:  
    mov          (72)(%rsp), %rcx
    mov          (80)(%rsp), %rdx
    jmp          LEncRefineAc07gas_3
LCallWriteEncRefineAc00gas_3:  
    mov          %rcx, (72)(%rsp)
    mov          %rdx, (80)(%rsp)
    lea          LRetWriteEncRefineAc00gas_3(%rip), %rbx
    mov          %rbx, (56)(%rsp)
 
 
LWriteEncRefineAc00gas_3:  
    movd         %ebp, %mm1
    sub          $(64), %ebp
    movq         %mm0, %mm2
    neg          %ebp
    psllq        %mm1, %mm2
    movq         maskFF(%rip), %mm1
    mov          (8)(%rsp), %ebx
    mov          (8)(%rsp), %ecx
    sub          (128)(%rsp), %ebx
    mov          (120)(%rsp), %rdx
    cmp          $(-8), %ebx
    jg           LWriteEncRefineAc10gas_3
    cmp          $(32), %ebp
    jl           LWriteEncRefineAc10gas_3
 
    pcmpeqb      %mm2, %mm1
    pmovmskb     %mm1, %ebx
    movq         %mm2, %mm1
    test         %ebx, %ebx
    jnz          LWriteEncRefineAc11gas_3
    movd         %mm2, %ebx
    psrlq        $(32), %mm1
    bswap        %ebx
    mov          %ebx, (4)(%rdx,%rcx)
    movd         %mm1, %ebx
    bswap        %ebx
    mov          %ebx, (%rdx,%rcx)
    mov          %ebp, %ebx
    and          $(7), %ebp
    and          $(4294967288), %ebx
    sub          $(64), %ebp
    neg          %ebp
    shr          $(3), %ebx
    add          %ebx, (8)(%rsp)
    mov          (56)(%rsp), %rbx
    jmp          *%rbx
 
 
LWriteEncRefineAc10gas_3:  
    movq         %mm2, %mm1
 
LWriteEncRefineAc11gas_3:  
    sub          $(8), %ebp
    jl           LWriteEncRefineAc12gas_3
 
    cmp          (128)(%rsp), %ecx
    jge          LErrorEncRefineAc03gas_3
    psrlq        $(56), %mm2
    psllq        $(8), %mm1
    movd         %mm2, %ebx
    movq         %mm1, %mm2
    mov          %bl, (%rdx,%rcx)
    add          $(1), %ecx
    cmp          $(255), %ebx
    jne          LWriteEncRefineAc11gas_3
    cmp          (128)(%rsp), %ecx
    je           LErrorEncRefineAc04gas_3
    xor          %ebx, %ebx
    mov          %bl, (%rdx,%rcx)
    add          $(1), %ecx
    jmp          LWriteEncRefineAc11gas_3
 
LWriteEncRefineAc12gas_3:  
    sub          $(56), %ebp
    mov          %ecx, (8)(%rsp)
    neg          %ebp
    mov          (56)(%rsp), %rbx
    jmp          *%rbx
 
 
LExitEncRefineAc00gas_3:  
    mov          (136)(%rsp), %rbx
    mov          (8)(%rsp), %ecx
    mov          %ecx, (%rbx)
 
    movq         (752)(%rsp), %rcx
    lea          (8)(%rcx), %rbx
    sub          $(64), %ebp
    neg          %ebp
    mov          %ebp, (%rbx)
 
    lea          (%rcx), %rbx
    movq         %mm0, (%rbx)
    mov          (32)(%rsp), %ebx
    lea          (12)(%rcx), %rax
    mov          %ebx, (%rax)
    mov          (40)(%rsp), %ebx
    lea          (16)(%rcx), %rcx
    mov          %ebx, (%rcx)
 
    mov          (%rsp), %rax
    emms
    add          $(680), %rsp
 
 
    pop          %r15
 
 
    pop          %r14
 
 
    pop          %r13
 
 
    pop          %r12
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
LZeros16EncRefinetAc00gas_3:  
    cmp          (16)(%rsp), %edi
    jl           LEncRefineAc04gas_3
 
    mov          (32)(%rsp), %ebx
    test         %ebx, %ebx
    jnz          LCallEobEncRefineAc01gas_3
 
LZeros16EncRefinetAc01gas_3:  
    mov          (40)(%rsp), %ebx
    test         %ebx, %ebx
    jnz          LCallAppendEncRefineAc01gas_3
 
LZeros16EncRefinetAc02gas_3:  
    mov          (24)(%rsp), %rbx
    movd         (960)(%rbx), %mm7
    movd         (960)(%rbx), %mm6
    psrld        $(16), %mm7
    movd         %mm7, %ebx
    pand         Mask16(%rip), %mm6
    test         %ebx, %ebx
    jz           LErrorEncRefineAc02gas_3
LZeros16EncRefinetAc03gas_3:  
    cmp          %ebp, %ebx
    jg           LCallWriteEncRefineAc03gas_3
LZeros16EncRefinetAc04gas_3:  
    psllq        %mm7, %mm0
    por          %mm6, %mm0
    sub          %ebx, %ebp
    sub          $(16), %esi
 
    test         %edx, %edx
    jnz          LCallAppendEncRefineAc02gas_3
 
LZeros16EncRefinetAc05gas_3:  
    mov          (48)(%rsp), %rcx
 
    cmp          $(15), %esi
    jle          LEncRefineAc04gas_3
    jmp          LZeros16EncRefinetAc02gas_3
 
 
LCallAppendEncRefineAc04gas_3:  
    mov          (48)(%rsp), %rsi
    lea          LLastZerosEncRefineAc03gas_3(%rip), %rbx
    mov          %rbx, (64)(%rsp)
    jmp          LAppendEncRefineAc00gas_3
 
LCallAppendEncRefineAc03gas_3:  
    mov          %edx, %eax
    mov          %rcx, %rsi
    lea          LEncRefineAc11gas_3(%rip), %rbx
    mov          %rbx, (64)(%rsp)
    jmp          LAppendEncRefineAc00gas_3
 
LRetAppendEncRefineAc02gas_3:  
    mov          %eax, (104)(%rsp)
    mov          %eax, %edx
    mov          (72)(%rsp), %rax
    mov          (80)(%rsp), %rsi
    mov          (48)(%rsp), %rcx
    jmp          LZeros16EncRefinetAc05gas_3
LCallAppendEncRefineAc02gas_3:  
    mov          %rax, (72)(%rsp)
    mov          %rsi, (80)(%rsp)
    mov          %rcx, (88)(%rsp)
    mov          %edx, %eax
    mov          %rcx, %rsi
    lea          LRetAppendEncRefineAc02gas_3(%rip), %rbx
    mov          %rbx, (64)(%rsp)
    jmp          LAppendEncRefineAc00gas_3
 
LRetAppendEncRefineAc01gas_3:  
    mov          %eax, (104)(%rsp)
    mov          %eax, (40)(%rsp)
    mov          (72)(%rsp), %rcx
    mov          (80)(%rsp), %rdx
    mov          (88)(%rsp), %rsi
    mov          (96)(%rsp), %rax
    jmp          LZeros16EncRefinetAc02gas_3
LCallAppendEncRefineAc01gas_3:  
    mov          %rcx, (72)(%rsp)
    mov          %rdx, (80)(%rsp)
    mov          %rsi, (88)(%rsp)
    mov          %rax, (96)(%rsp)
    mov          %ebx, %eax
    mov          (48)(%rsp), %rsi
    lea          LRetAppendEncRefineAc01gas_3(%rip), %rbx
    mov          %rbx, (64)(%rsp)
    jmp          LAppendEncRefineAc00gas_3
 
LRetAppendEncRefineAc00gas_3:  
    mov          %eax, (104)(%rsp)
    mov          %eax, (40)(%rsp)
    mov          (72)(%rsp), %rcx
    mov          (80)(%rsp), %rdx
    mov          (88)(%rsp), %rsi
    jmp          LEncRefineAc06gas_3
LCallAppendEncRefineAc00gas_3:  
    mov          %rcx, (72)(%rsp)
    mov          %rdx, (80)(%rsp)
    mov          %rsi, (88)(%rsp)
    mov          (48)(%rsp), %rsi
    lea          LRetAppendEncRefineAc00gas_3(%rip), %rbx
    mov          %rbx, (64)(%rsp)
 
LAppendEncRefineAc00gas_3:  
    cmp          $(1), %ebp
    jl           LCallWriteEncRefineAc02gas_3
LAppendEncRefineAc01gas_3:  
    psllq        $(1), %mm0
    movzbl       (%rsi), %ebx
    add          $(1), %rsi
    and          $(1), %ebx
    movd         %ebx, %mm1
    sub          $(1), %ebp
    por          %mm1, %mm0
    sub          $(1), %eax
    jnz          LAppendEncRefineAc00gas_3
 
    mov          (64)(%rsp), %rbx
    jmp          *%rbx
 
 
LCallEobEncRefineAc01gas_3:  
    mov          %rsi, (96)(%rsp)
    lea          LZeros16EncRefinetAc01gas_3(%rip), %rsi
    mov          %rsi, (64)(%rsp)
    jmp          LEobEncRefineAc00gas_3
 
LCallEobEncRefineAc00gas_3:  
    mov          %rsi, (96)(%rsp)
    lea          LEncRefineAc05gas_3(%rip), %rsi
    mov          %rsi, (64)(%rsp)
 
 
LEobEncRefineAc00gas_3:  
    mov          %rcx, (72)(%rsp)
    mov          %rax, (80)(%rsp)
    mov          %rdx, (88)(%rsp)
 
 
    cmp          $(1), %ebx
    jne          LEobEncRefineAc01gas_3
 
    xor          %ebx, %ebx
    lea          LEobEncRefineAc09gas_3(%rip), %rsi
    jmp          LHuffEobEncRefineAc00gas_3
 
 
LEobEncRefineAc01gas_3:  
    cmp          $(32767), %ebx
    jle          LEobEncRefineAc04gas_3
 
    mov          $(224), %ebx
    lea          LEobEncRefineAc02gas_3(%rip), %rsi
    jmp          LHuffEobEncRefineAc00gas_3
 
LEobEncRefineAc02gas_3:  
    cmp          $(14), %ebp
    jge          LEobEncRefineAc03gas_3
 
    lea          LEobEncRefineAc03gas_3(%rip), %rbx
    mov          %rbx, (56)(%rsp)
    jmp          LWriteEncRefineAc00gas_3
 
LEobEncRefineAc03gas_3:  
    mov          $(32767), %esi
    mov          (32)(%rsp), %ebx
    psllq        $(14), %mm0
    and          $(16383), %ebx
    sub          %esi, (32)(%rsp)
    movd         %ebx, %mm1
    sub          $(14), %ebp
    por          %mm1, %mm0
    mov          (32)(%rsp), %ebx
 
 
LEobEncRefineAc04gas_3:  
    lea          eobSize(%rip), %rsi
 
    cmp          $(256), %ebx
    jl           LEobEncRefineAc05gas_3
 
    shr          $(8), %ebx
    movzbl       (%rsi,%rbx), %ebx
    add          $(8), %ebx
    jmp          LEobEncRefineAc06gas_3
 
LEobEncRefineAc05gas_3:  
    movzbl       (%rsi,%rbx), %ebx
 
LEobEncRefineAc06gas_3:  
    cmp          $(14), %ebx
    jg           LErrorEncRefineAc00gas_3
 
    movd         %ebx, %mm5
    shl          $(4), %ebx
    lea          LEobEncRefineAc07gas_3(%rip), %rsi
    jmp          LHuffEobEncRefineAc00gas_3
 
LEobEncRefineAc07gas_3:  
    movd         %mm5, %esi
    cmp          %esi, %ebp
    jge          LEobEncRefineAc08gas_3
 
    lea          LEobEncRefineAc08gas_3(%rip), %rbx
    mov          %rbx, (56)(%rsp)
    jmp          LWriteEncRefineAc00gas_3
 
LEobEncRefineAc08gas_3:  
    movd         %esi, %mm1
    mov          $(32), %ecx
    sub          %esi, %ecx
    mov          (32)(%rsp), %ebx
    shl          %cl, %ebx
    shr          %cl, %ebx
    psllq        %mm1, %mm0
    movd         %ebx, %mm1
    sub          %esi, %ebp
    por          %mm1, %mm0
 
 
LEobEncRefineAc09gas_3:  
    xor          %esi, %esi
    mov          %esi, (32)(%rsp)
    mov          %esi, (104)(%rsp)
    mov          (72)(%rsp), %rcx
    mov          (80)(%rsp), %rax
    mov          (88)(%rsp), %rdx
    mov          (96)(%rsp), %rsi
    mov          (64)(%rsp), %rbx
    jmp          *%rbx
 
 
LHuffEobEncRefineAc00gas_3:  
    mov          (24)(%rsp), %rax
    movd         (%rax,%rbx,4), %mm7
    movd         (%rax,%rbx,4), %mm6
    psrld        $(16), %mm7
    movd         %mm7, %eax
    pand         Mask16(%rip), %mm6
    test         %eax, %eax
    jz           LErrorEncRefineAc02gas_3
    cmp          %ebp, %eax
    jle          LHuffEobEncRefineAc01gas_3
    lea          LHuffEobEncRefineAc01gas_3(%rip), %rbx
    mov          %rbx, (56)(%rsp)
    jmp          LWriteEncRefineAc00gas_3
LHuffEobEncRefineAc01gas_3:  
    psllq        %mm7, %mm0
    por          %mm6, %mm0
    sub          %eax, %ebp
    jmp          *%rsi
 
 
LErrorEncRefineAc00gas_3:  
    mov          $(-63), %rbx
    mov          %rbx, (%rsp)
    jmp          LExitEncRefineAc00gas_3
 
LErrorEncRefineAc02gas_3:  
    mov          $(-64), %rbx
    mov          %rbx, (%rsp)
    jmp          LExitEncRefineAc00gas_3
 
LErrorEncRefineAc03gas_3:  
    sub          $(56), %ebp
    mov          %ecx, (8)(%rsp)
    neg          %ebp
    mov          $(-62), %rbx
    mov          %rbx, (%rsp)
    jmp          LExitEncRefineAc00gas_3
 
LErrorEncRefineAc04gas_3:  
    sub          $(64), %ebp
    mov          %ecx, (8)(%rsp)
    neg          %ebp
    mov          $(-62), %rbx
    mov          %rbx, (%rsp)
    jmp          LExitEncRefineAc00gas_3
 
 
.p2align 4, 0x90
 
 
.globl mfxownpj_GetHuffmanStatistics8x8_ACFlush_JPEG_16s_C1

 
mfxownpj_GetHuffmanStatistics8x8_ACFlush_JPEG_16s_C1:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    push         %r12
 
 
    push         %r13
 
 
    push         %r14
 
 
    push         %r15
 
 
    sub          $(40), %rsp
 
 
    mov          %rdi, (8)(%rsp)
    mov          %rsi, (16)(%rsp)
 
 
    mov          $(0), %rax
    mov          %rax, (%rsp)
 
    mov          (16)(%rsp), %rcx
    lea          (12)(%rcx), %rax
    mov          (%rax), %eax
 
    mov          (8)(%rsp), %rcx
 
 
    test         %eax, %eax
    jz           LInitStatFlushAc00gas_4
 
    cmp          $(1), %eax
    jg           LCountStatFlushAc00gas_4
 
    add          %eax, (%rcx)
    jmp          LInitStatFlushAc00gas_4
 
LCountStatFlushAc00gas_4:  
    mov          $(1), %edx
    cmp          $(32767), %eax
    jle          LCountStatFlushAc01gas_4
 
    add          %edx, (896)(%rcx)
    sub          $(32767), %eax
 
LCountStatFlushAc01gas_4:  
    lea          eobSize(%rip), %rdx
 
    cmp          $(256), %eax
    jl           LCountStatFlushAc02gas_4
 
    shr          $(8), %eax
    movzbl       (%rdx,%rax), %eax
    add          $(8), %eax
    jmp          LCountStatFlushAc03gas_4
 
LCountStatFlushAc02gas_4:  
    movzbl       (%rdx,%rax), %eax
 
LCountStatFlushAc03gas_4:  
    mov          $(1), %edx
    cmp          $(14), %eax
    jg           LErrorStatFlushAc00gas_4
 
    shl          $(4), %eax
    add          %edx, (%rcx,%rax,4)
 
 
LInitStatFlushAc00gas_4:  
    xor          %eax, %eax
    mov          (16)(%rsp), %rcx
    lea          (8)(%rcx), %rdx
    mov          %eax, (%rdx)
 
    lea          (%rcx), %rdx
    mov          %rax, (%rdx)
 
    lea          (16)(%rcx), %rdx
    mov          %eax, (%rdx)
 
    lea          (12)(%rcx), %rdx
    mov          %eax, (%rdx)
 
    xorps        %xmm0, %xmm0
    lea          (20)(%rcx), %rdx
    mov          $(1008), %eax
    movups       %xmm0, (%rdx)
    movups       %xmm0, (%rdx,%rax)
    and          $(-16), %rdx
    add          $(16), %rdx
    mov          $(15), %eax
 
LInitStatFlushAc01gas_4:  
    movaps       %xmm0, (%rdx)
    movaps       %xmm0, (16)(%rdx)
    movaps       %xmm0, (32)(%rdx)
    movaps       %xmm0, (48)(%rdx)
    add          $(64), %rdx
    sub          $(1), %eax
    jnz          LInitStatFlushAc01gas_4
 
    movaps       %xmm0, (%rdx)
    movaps       %xmm0, (16)(%rdx)
    movaps       %xmm0, (32)(%rdx)
 
 
LExitStatFlushAc00gas_4:  
    mov          (%rsp), %rax
    add          $(40), %rsp
 
 
    pop          %r15
 
 
    pop          %r14
 
 
    pop          %r13
 
 
    pop          %r12
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
LErrorStatFlushAc00gas_4:  
    mov          $(-63), %rax
    mov          %rax, (%rsp)
    jmp          LInitStatFlushAc00gas_4
 
 
.p2align 4, 0x90
 
 
.globl mfxownpj_GetHuffmanStatistics8x8_ACRefine_JPEG_16s_C1

 
mfxownpj_GetHuffmanStatistics8x8_ACRefine_JPEG_16s_C1:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    push         %r12
 
 
    push         %r13
 
 
    push         %r14
 
 
    push         %r15
 
 
    sub          $(344), %rsp
 
 
    mov          %rdi, (32)(%rsp)
    mov          %rsi, (40)(%rsp)
    mov          %edx, %edx
    mov          %rdx, (48)(%rsp)
    mov          %ecx, %ecx
    mov          %rcx, (56)(%rsp)
    mov          %r8d, %r8d
    mov          %r8, (64)(%rsp)
    mov          %r9, (72)(%rsp)
 
 
    mov          (56)(%rsp), %rbx
    sub          $(63), %rbx
    lea          ownZigzag(%rip), %rsi
    sub          %rbx, %rsi
    mov          (56)(%rsp), %edi
    sub          (48)(%rsp), %edi
    mov          (32)(%rsp), %rcx
 
 
    mov          %edi, (24)(%rsp)
    mov          $(64), %ebx
    movzbl       (%rdi,%rsi), %edx
 
    movswl       (%rdx,%rcx), %eax
    cdq
    mov          (64)(%rsp), %ecx
    xor          %edx, %eax
    sub          %edx, %eax
    shr          %cl, %eax
    mov          (32)(%rsp), %rcx
    mov          %eax, (80)(%rsp,%rdi,4)
    cmp          $(1), %eax
    cmove        %edi, %ebx
    movzbl       (-1)(%rdi,%rsi), %edx
    sub          $(1), %edi
    jl           LStatRefineAc00gas_5
LAbsStatRefineAc00gas_5:  
    movswl       (%rdx,%rcx), %eax
    cdq
    mov          (64)(%rsp), %ecx
    xor          %edx, %eax
    sub          %edx, %eax
    shr          %cl, %eax
    mov          (32)(%rsp), %rcx
    mov          %eax, (80)(%rsp,%rdi,4)
    cmp          $(1), %eax
    cmove        %edi, %ebx
    movzbl       (-1)(%rdi,%rsi), %edx
    sub          $(1), %edi
    jl           LStatRefineAc00gas_5
    movswl       (%rdx,%rcx), %eax
    cdq
    mov          (64)(%rsp), %ecx
    xor          %edx, %eax
    sub          %edx, %eax
    shr          %cl, %eax
    mov          (32)(%rsp), %rcx
    mov          %eax, (80)(%rsp,%rdi,4)
    cmp          $(1), %eax
    cmove        %edi, %ebx
    movzbl       (-1)(%rdi,%rsi), %edx
    sub          $(1), %edi
    jl           LStatRefineAc00gas_5
    movswl       (%rdx,%rcx), %eax
    cdq
    mov          (64)(%rsp), %ecx
    xor          %edx, %eax
    sub          %edx, %eax
    shr          %cl, %eax
    mov          (32)(%rsp), %rcx
    mov          %eax, (80)(%rsp,%rdi,4)
    cmp          $(1), %eax
    cmove        %edi, %ebx
    movzbl       (-1)(%rdi,%rsi), %edx
    sub          $(1), %edi
    jge          LAbsStatRefineAc00gas_5
 
 
LStatRefineAc00gas_5:  
    mov          %ebx, (8)(%rsp)
 
    mov          $(0), %rax
    mov          %rax, (%rsp)
 
    mov          (72)(%rsp), %rcx
    lea          (12)(%rcx), %rax
    mov          (%rax), %ebx
 
    lea          (16)(%rcx), %rcx
    mov          (%rcx), %ecx
 
    mov          (24)(%rsp), %edi
    xor          %esi, %esi
    xor          %edx, %edx
    xor          %eax, %eax
    mov          (40)(%rsp), %rbp
    jmp          LStatRefineAc01gas_5
 
LStatRefineAc02gas_5:  
    xor          %eax, %eax
    add          $(1), %edx
    sub          $(1), %edi
    jl           LLastZerosStatRefineAc00gas_5
 
LStatRefineAc01gas_5:  
    add          (80)(%rsp,%rdi,4), %eax
    jnz          LStatRefineAc03gas_5
    add          $(16), %esi
    sub          $(1), %edi
    jl           LLastZerosStatRefineAc00gas_5
    add          (80)(%rsp,%rdi,4), %eax
    jnz          LStatRefineAc03gas_5
    add          $(16), %esi
    sub          $(1), %edi
    jl           LLastZerosStatRefineAc00gas_5
    add          (80)(%rsp,%rdi,4), %eax
    jnz          LStatRefineAc03gas_5
    add          $(16), %esi
    sub          $(1), %edi
    jl           LLastZerosStatRefineAc00gas_5
    jmp          LStatRefineAc01gas_5
 
LStatRefineAc03gas_5:  
    cmp          $(240), %esi
    jg           LZeros16StatRefineAc00gas_5
 
LStatRefineAc04gas_5:  
    cmp          $(1), %eax
    jg           LStatRefineAc02gas_5
 
    test         %ebx, %ebx
    jnz          LStatRefineAc06gas_5
 
LStatRefineAc05gas_5:  
    xor          %ecx, %ecx
    xor          %edx, %edx
    add          $(1), %esi
    mov          $(1), %eax
    add          %eax, (%rbp,%rsi,4)
    xor          %esi, %esi
    xor          %eax, %eax
    sub          $(1), %edi
    jge          LStatRefineAc01gas_5
    jmp          LExitStatRefineAc00gas_5
 
LStatRefineAc06gas_5:  
    cmp          $(1), %ebx
    jg           LCallCountStatRefineAc00gas_5
 
    add          %ebx, (%rbp)
    xor          %ebx, %ebx
    jmp          LStatRefineAc05gas_5
 
 
LLastZerosStatRefineAc00gas_5:  
    or           %edx, %esi
    jz           LExitStatRefineAc00gas_5
 
    add          $(1), %ebx
    add          %edx, %ecx
 
    cmp          $(937), %ecx
    jg           LCallCountStatRefineAc02gas_5
 
    cmp          $(32767), %ebx
    je           LCallCountStatRefineAc02gas_5
 
 
LExitStatRefineAc00gas_5:  
    mov          (72)(%rsp), %rdx
    lea          (12)(%rdx), %rax
    mov          %ebx, (%rax)
    lea          (16)(%rdx), %rdx
    mov          %ecx, (%rdx)
 
    mov          (%rsp), %rax
    add          $(344), %rsp
 
 
    pop          %r15
 
 
    pop          %r14
 
 
    pop          %r13
 
 
    pop          %r12
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
LZeros16StatRefineAc00gas_5:  
    cmp          (8)(%rsp), %edi
    jl           LStatRefineAc04gas_5
 
    test         %ebx, %ebx
    jnz          LZeros16StatRefineAc04gas_5
 
LZeros16StatRefineAc01gas_5:  
    cmp          $(240), %esi
    jl           LZeros16StatRefineAc03gas_5
 
    mov          $(1), %ecx
 
LZeros16StatRefineAc02gas_5:  
    add          %ecx, (960)(%rbp)
    sub          $(256), %esi
    cmp          $(240), %esi
    jg           LZeros16StatRefineAc02gas_5
 
LZeros16StatRefineAc03gas_5:  
    xor          %ecx, %ecx
    xor          %edx, %edx
    jmp          LStatRefineAc04gas_5
 
LZeros16StatRefineAc04gas_5:  
    cmp          $(1), %ebx
    jg           LCallCountStatRefineAc01gas_5
 
    add          %ebx, (%rbp)
    xor          %ebx, %ebx
    jmp          LZeros16StatRefineAc01gas_5
 
 
LRetCountStatRefineAc01gas_5:  
    xor          %ecx, %ecx
    jmp          LExitStatRefineAc00gas_5
LCallCountStatRefineAc02gas_5:  
    lea          LRetCountStatRefineAc01gas_5(%rip), %rcx
    jmp          LCountStatRefineAc00gas_5
 
LCallCountStatRefineAc01gas_5:  
    lea          LZeros16StatRefineAc01gas_5(%rip), %rcx
    jmp          LCountStatRefineAc00gas_5
 
LCallCountStatRefineAc00gas_5:  
    lea          LStatRefineAc05gas_5(%rip), %rcx
 
 
LCountStatRefineAc00gas_5:  
    mov          $(1), %edx
    cmp          $(32767), %ebx
    jle          LCountStatRefineAc01gas_5
 
    add          %edx, (896)(%rbp)
    sub          $(32767), %ebx
 
LCountStatRefineAc01gas_5:  
    lea          eobSize(%rip), %rdx
 
    cmp          $(256), %ebx
    jl           LCountStatRefineAc02gas_5
 
    shr          $(8), %ebx
    movzbl       (%rbx,%rdx), %ebx
    add          $(8), %ebx
    jmp          LCountStatRefineAc03gas_5
 
LCountStatRefineAc02gas_5:  
    movzbl       (%rbx,%rdx), %ebx
 
LCountStatRefineAc03gas_5:  
    mov          $(1), %edx
    cmp          $(14), %ebx
    jg           LErrorStatRefineAc00gas_5
 
    shl          $(4), %ebx
    add          %edx, (%rbp,%rbx,4)
    xor          %ebx, %ebx
    jmp          *%rcx
 
 
LErrorStatRefineAc00gas_5:  
    mov          $(-63), %rax
    mov          %rax, (%rsp)
    jmp          LExitStatRefineAc00gas_5
 
 
.data
 
.p2align 4, 0x90
 
maskFF:
.quad   0xffffffffffffffff  
 
Mask16:
.quad              0xffff  
 
 
TableMask12:
.long  0, 1, 3, 7, 15, 31, 63, 127, 255, 511, 1023, 2047  
 
 

.byte  126, 126, 126, 126, 126, 126, 126, 126  
 
ownZigzag:
.byte  126, 124, 110,  94, 108, 122, 120, 106  
 

.byte   92,  78,  62,  76,  90, 104, 118, 116  
 

.byte  102,  88,  74,  60,  46,  30,  44,  58  
 

.byte   72,  86, 100, 114, 112,  98,  84,  70  
 

.byte   56,  42,  28,  14,  12,  26,  40,  54  
 

.byte   68,  82,  96,  80,  66,  52,  38,  24  
 

.byte   10,   8,  22,  36,  50,  64,  48,  34  
 

.byte   20,   6,   4,  18,  32,  16,   2,   0  
 
 
eobSize:
.byte  0, 0, 1, 1, 2, 2, 2, 2  
 

.fill 8, 1, 3
 

.fill 16, 1, 4
 

.fill 32, 1, 5
 

.fill 64, 1, 6
 

.fill 128, 1, 7
 

.fill 256, 1, 8
 
 
