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
 
 
.globl mfxownpj_DecodeHuffman8x8_ACFirst_JPEG_1u16s_C1

 
mfxownpj_DecodeHuffman8x8_ACFirst_JPEG_1u16s_C1:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    push         %r12
 
 
    push         %r13
 
 
    push         %r14
 
 
    push         %r15
 
 
    sub          $(72), %rsp
 
 
    mov          %rdi, (%rsp)
    mov          %esi, %esi
    mov          %rsi, (8)(%rsp)
    mov          %rdx, (16)(%rsp)
    mov          %rcx, (24)(%rsp)
    mov          %r8, (32)(%rsp)
    mov          %r9d, %r9d
    mov          %r9, (40)(%rsp)
    mov          (128)(%rsp), %eax
    mov          %rax, (128)(%rsp)
    mov          (136)(%rsp), %eax
    mov          %rax, (136)(%rsp)
 
 
    mov          $(0), %rax
    mov          %rax, (56)(%rsp)
    movq         (152)(%rsp), %rcx
    lea          (12)(%rcx), %rbx
    mov          (%rbx), %eax
    cmp          $(0), %eax
    jg           LDecHuffExitEndOfBlockRun00gas_1
 
    lea          (8)(%rcx), %rax
    mov          (%rax), %ebp
    lea          (%rcx), %rax
    movq         (%rax), %mm0
 
    mov          $(64), %eax
    sub          %ebp, %eax
    movd         %eax, %mm1
    psllq        %mm1, %mm0
 
    mov          (24)(%rsp), %rdi
 
    mov          (16)(%rsp), %rax
    mov          (%rax), %eax
    movd         %eax, %mm7
 
 
    mov          (128)(%rsp), %rax
    sub          $(63), %rax
    lea          ownTables(%rip), %rsi
    sub          %rax, %rsi
    lea          ownTables+64(%rip), %rax
    mov          %rax, %r8
    movq         (144)(%rsp), %rax
    lea          (512)(%rax), %rax
    mov          %rax, %r9
    mov          (128)(%rsp), %edi
    sub          (40)(%rsp), %edi
 
    mov          (32)(%rsp), %rax
    mov          (%rax), %eax
    test         %eax, %eax
    jnz          LDecHuffAcT00gas_1
 
LDecHuffAc00gas_1:  
    cmp          $(8), %ebp
    jl           LDecHuffAcLoad00gas_1
LDecHuffAc01gas_1:  
    movq         %mm0, %mm1
    psrlq        $(56), %mm0
    movd         %mm0, %ecx
    mov          %r9, %rax
    movq         %mm1, %mm0
    mov          (%rax,%rcx,4), %eax
    test         $(4294901760), %eax
    jz           LDecHuffAcLong00gas_1
    mov          %eax, %ebx
    shr          $(16), %eax
    movd         %eax, %mm1
    sub          %eax, %ebp
    psllq        %mm1, %mm0
LDecHuffAcRetLong00gas_1:  
    mov          %ebx, %eax
    shr          $(4), %eax
    and          $(15), %eax
    and          $(15), %ebx
    jz           LDecHuffEndOfBlockRun00gas_1
    sub          %rax, %rdi
    cmp          %ebx, %ebp
    jl           LDecHuffAcLoad01gas_1
LDecHuffAc02gas_1:  
    pextrw       $(3), %mm0, %edx
    mov          %r8, %rax
    mov          $(16), %ecx
    movd         %ebx, %mm1
    sub          %ebx, %ecx
    sub          %ebx, %ebp
    lea          (%rax,%rbx,4), %rbx
    psllq        %mm1, %mm0
    xor          %eax, %eax
    test         $(32768), %edx
    cmove        (%rbx), %eax
    movzbl       (%rdi,%rsi), %ebx
    shr          %cl, %edx
    mov          (136)(%rsp), %ecx
    add          %eax, %edx
    mov          (24)(%rsp), %rax
    shl          %cl, %edx
    mov          %dx, (%rax,%rbx,2)
    sub          $(1), %edi
    jge          LDecHuffAc00gas_1
    jmp          LDecHuffExit00gas_1
 
 
LDecHuffEndOfBlockRun00gas_1:  
    cmp          $(15), %eax
    jne          LDecHuffEndOfBlockRun01gas_1
 
    sub          $(16), %edi
    jge          LDecHuffAc00gas_1
    jmp          LDecHuffExit00gas_1
 
LDecHuffEndOfBlockRun01gas_1:  
    mov          %eax, %ecx
    mov          $(1), %eax
    shl          %cl, %eax
    test         %ecx, %ecx
    jz           LDecHuffEndOfBlockRun03gas_1
 
    movd         %eax, %mm4
    mov          %ecx, %ebx
    cmp          %ecx, %ebp
    jl           LDecHuffEndOfBlockRunLoadT00gas_1
LDecHuffEndOfBlockRun02gas_1:  
    pextrw       $(3), %mm0, %edx
    sub          %ebx, %ebp
    mov          $(16), %ecx
    movd         %ebx, %mm1
    sub          %ebx, %ecx
    psllq        %mm1, %mm0
    shr          %cl, %edx
    movd         %mm4, %eax
    add          %edx, %eax
 
LDecHuffEndOfBlockRun03gas_1:  
    movq         (152)(%rsp), %rcx
    lea          (12)(%rcx), %rbx
    sub          $(1), %eax
    mov          %eax, (%rbx)
 
 
LDecHuffExit00gas_1:  
    mov          $(64), %eax
    sub          %ebp, %eax
    movd         %eax, %mm1
    psrlq        %mm1, %mm0
    movd         %mm7, %edx
    mov          (16)(%rsp), %rax
    mov          %edx, (%rax)
    movq         (152)(%rsp), %rdx
    lea          (8)(%rdx), %rax
    mov          %ebp, (%rax)
    lea          (%rdx), %rax
    movq         %mm0, (%rax)
LDecHuffExit02gas_1:  
    mov          (56)(%rsp), %rax
    emms
    add          $(72), %rsp
 
 
    pop          %r15
 
 
    pop          %r14
 
 
    pop          %r13
 
 
    pop          %r12
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
LDecHuffExitEndOfBlockRun00gas_1:  
    sub          $(1), %eax
    mov          %eax, (%rbx)
    jmp          LDecHuffExit02gas_1
 
 
LDecHuffSwitchOverToTail00gas_1:  
    sub          (%rsp), %rax
    movd         %eax, %mm7
    mov          $(64), %eax
    movd         %ecx, %mm0
    sub          %ebp, %eax
    movd         %eax, %mm1
    psllq        %mm1, %mm0
    movd         %edx, %mm1
    mov          (32)(%rsp), %rdx
    movd         %mm1, (%rdx)
 
 
    mov          (48)(%rsp), %rcx
    lea          LDecHuffLong01gas_1(%rip), %rax
    cmp          %rcx, %rax
    je           LDecHuffLongRetLoadT00gas_1
 
    lea          LDecHuffAc01gas_1(%rip), %rax
    cmp          %rcx, %rax
    je           LDecHuffAcT00gas_1
 
    jmp          LDecHuffAcRetLoadT01gas_1
 
 
LDecHuffSwitchOverToTail01gas_1:  
    mov          (48)(%rsp), %rcx
 
 
    lea          LDecHuffLong01gas_1(%rip), %rax
    cmp          %rcx, %rax
    je           LDecHuffLongT02gas_1
 
    lea          LDecHuffAc01gas_1(%rip), %rax
    cmp          %rcx, %rax
    je           LDecHuffAcT00gas_1
 
    jmp          LDecHuffAcLoadT01gas_1
 
 
LDecHuffErrorSrcOut00gas_1:  
    movl         $(-62), (56)(%rsp)
    jmp          LDecHuffExit00gas_1
 
LDecHuffErrorRange00gas_1:  
    movl         $(-63), (56)(%rsp)
    jmp          LDecHuffExit00gas_1
 
 
LDecHuffCheckLen00gas_1:  
    mov          (8)(%rsp), %ecx
    movd         %mm7, %eax
    sub          %eax, %ecx
    add          (%rsp), %rax
    cmp          $(8), %ecx
    jl           LDecHuffSwitchOverToTail01gas_1
 
    mov          (%rax), %ecx
    mov          (4)(%rax), %edx
    bswap        %ecx
    movd         %ecx, %mm2
    mov          $(64), %ecx
    bswap        %edx
    sub          %ebp, %ecx
    movd         %edx, %mm1
    movd         %ecx, %mm3
    punpckldq    %mm2, %mm1
    movq         maskFF(%rip), %mm2
    psrlq        %mm3, %mm0
    pcmpeqb      %mm1, %mm2
    pmovmskb     %mm2, %ecx
    test         %ecx, %ecx
    jnz          LDecHuffInfill10gas_1
    lea          (7)(%rbp), %rcx
    and          $(4294967288), %ecx
    mov          $(64), %edx
    movd         %ecx, %mm2
    sub          %ecx, %edx
    psrlq        %mm2, %mm1
    movd         %edx, %mm2
    add          %edx, %ebp
    shr          $(3), %edx
    psllq        %mm2, %mm0
    mov          $(64), %ecx
    por          %mm1, %mm0
    movd         %mm7, %eax
    sub          %ebp, %ecx
    add          %edx, %eax
    movd         %ecx, %mm1
    movd         %eax, %mm7
    mov          (48)(%rsp), %rax
    psllq        %mm1, %mm0
    jmp          *%rax
 
LDecHuffLongLoad00gas_1:  
    lea          LDecHuffLong01gas_1(%rip), %rax
    mov          %rax, (48)(%rsp)
    jmp          LDecHuffCheckLen00gas_1
 
LDecHuffAcLoad00gas_1:  
    lea          LDecHuffAc01gas_1(%rip), %rax
    mov          %rax, (48)(%rsp)
    jmp          LDecHuffCheckLen00gas_1
 
LDecHuffAcLoad01gas_1:  
    lea          LDecHuffAc02gas_1(%rip), %rax
    mov          %rax, (48)(%rsp)
    jmp          LDecHuffCheckLen00gas_1
 
 
LDecHuffInfill10gas_1:  
    movd         %mm0, %ecx
 
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
    jnz          LDecHuffSwitchOverToTail00gas_1
    mov          $(255), %edx
 
LDecHuffInfill12gas_1:  
    shl          $(8), %ecx
    or           %edx, %ecx
    add          $(8), %ebp
    jmp          LDecHuffInfill11gas_1
 
LDecHuffInfill13gas_1:  
    sub          (%rsp), %rax
    movd         %eax, %mm7
    mov          $(64), %eax
    movd         %ecx, %mm0
    sub          %ebp, %eax
    movd         %eax, %mm1
    mov          (48)(%rsp), %rax
    psllq        %mm1, %mm0
    jmp          *%rax
 
 
LDecHuffAcLong00gas_1:  
    movq         (144)(%rsp), %rax
    mov          $(9), %ebx
    mov          %rax, %r10
    cmp          $(16), %ebp
    jl           LDecHuffLongLoad00gas_1
LDecHuffLong01gas_1:  
    mov          %r10, %rax
    lea          (1572)(%rax), %rdx
LDecHuffLong02gas_1:  
    movq         %mm0, %mm1
    mov          $(64), %eax
    sub          %ebx, %eax
    movd         %eax, %mm2
    movswl       (%rdx,%rbx,2), %eax
    psrlq        %mm2, %mm1
    movd         %mm1, %ecx
    test         $(32768), %eax
    jz           LDecHuffLong03gas_1
    cmp          $(-1), %eax
    je           LDecHuffLong03gas_1
    movzwl       (%rdx,%rbx,2), %eax
LDecHuffLong03gas_1:  
    cmp          %eax, %ecx
    jle          LDecHuffLong04gas_1
    add          $(1), %ebx
    cmp          $(16), %ebx
    jg           LDecHuffErrorRange00gas_1
    jmp          LDecHuffLong02gas_1
 
LDecHuffLong04gas_1:  
    movd         %ebx, %mm1
    psllq        %mm1, %mm0
    sub          %ebx, %ebp
    mov          %r10, %rax
    lea          (1536)(%rax), %rdx
    movzwl       (%rdx,%rbx,2), %eax
    sub          %eax, %ecx
    mov          %r10, %rax
    lea          (1608)(%rax), %rdx
    movzwl       (%rdx,%rbx,2), %eax
    add          %eax, %ecx
    mov          %r10, %rax
    lea          (%rax), %rdx
    movzwl       (%rdx,%rcx,2), %ebx
    jmp          LDecHuffAcRetLong00gas_1
 
 
LDecHuffAcT00gas_1:  
    xor          %ebx, %ebx
    cmp          $(8), %ebp
    jl           LDecHuffAcLoadT00gas_1
LDecHuffAcT01gas_1:  
    cmp          $(8), %ebp
    jge          LDecHuffAcT02gas_1
    add          $(1), %ebx
    jmp          LDecHuffAcLongT01gas_1
LDecHuffAcT02gas_1:  
    movq         %mm0, %mm1
    psrlq        $(56), %mm0
    movd         %mm0, %ecx
    mov          %r9, %rax
    movq         %mm1, %mm0
    mov          (%rax,%rcx,4), %eax
    test         $(4294901760), %eax
    jz           LDecHuffAcLongT00gas_1
    mov          %eax, %ebx
    shr          $(16), %eax
    movd         %eax, %mm1
    sub          %eax, %ebp
    psllq        %mm1, %mm0
LDecHuffAcRetLongT00gas_1:  
    mov          %ebx, %eax
    shr          $(4), %eax
    and          $(15), %eax
    and          $(15), %ebx
    jz           LDecHuffEndOfBlockRunT00gas_1
    sub          %rax, %rdi
    cmp          %ebx, %ebp
    jl           LDecHuffAcLoadT01gas_1
LDecHuffAcT03gas_1:  
    pextrw       $(3), %mm0, %edx
    mov          %r8, %rax
    mov          $(16), %ecx
    movd         %ebx, %mm1
    sub          %ebx, %ecx
    sub          %ebx, %ebp
    lea          (%rax,%rbx,4), %rbx
    psllq        %mm1, %mm0
    xor          %eax, %eax
    test         $(32768), %edx
    cmove        (%rbx), %eax
    movzbl       (%rdi,%rsi), %ebx
    shr          %cl, %edx
    mov          (136)(%rsp), %ecx
    add          %eax, %edx
    mov          (24)(%rsp), %rax
    shl          %cl, %edx
    mov          %dx, (%rax,%rbx,2)
    sub          $(1), %edi
    jge          LDecHuffAcT00gas_1
    jmp          LDecHuffExit00gas_1
 
 
LDecHuffEndOfBlockRunT00gas_1:  
    cmp          $(15), %eax
    jne          LDecHuffEndOfBlockRun01gas_1
 
    sub          $(16), %edi
    jge          LDecHuffAcT00gas_1
    jmp          LDecHuffExit00gas_1
 
 
LDecHuffAcLongT00gas_1:  
    mov          $(9), %ebx
 
LDecHuffAcLongT01gas_1:  
    movq         (144)(%rsp), %rax
    mov          %rax, %r10
 
LDecHuffLongT02gas_1:  
    cmp          %ebx, %ebp
    jl           LDecHuffLongLoadT00gas_1
LDecHuffLongT03gas_1:  
    mov          %r10, %rax
    lea          (1572)(%rax), %rdx
    movq         %mm0, %mm1
    mov          $(64), %eax
    sub          %ebx, %eax
    movd         %eax, %mm2
    movswl       (%rdx,%rbx,2), %eax
    psrlq        %mm2, %mm1
    movd         %mm1, %ecx
    test         $(32768), %eax
    jz           LDecHuffLongT04gas_1
    cmp          $(-1), %eax
    je           LDecHuffLongT04gas_1
    movzwl       (%rdx,%rbx,2), %eax
LDecHuffLongT04gas_1:  
    cmp          %eax, %ecx
    jle          LDecHuffLongT05gas_1
    add          $(1), %ebx
    cmp          $(16), %ebx
    jg           LDecHuffErrorRange00gas_1
    jmp          LDecHuffLongT02gas_1
 
LDecHuffLongT05gas_1:  
    movd         %ebx, %mm1
    psllq        %mm1, %mm0
    sub          %ebx, %ebp
    mov          %r10, %rax
    lea          (1536)(%rax), %rdx
    movzwl       (%rdx,%rbx,2), %eax
    sub          %eax, %ecx
    mov          %r10, %rax
    lea          (1608)(%rax), %rdx
    movzwl       (%rdx,%rbx,2), %eax
    add          %eax, %ecx
    mov          %r10, %rax
    lea          (%rax), %rdx
    movzwl       (%rdx,%rcx,2), %ebx
    jmp          LDecHuffAcRetLongT00gas_1
 
 
LDecHuffInfillT00gas_1:  
    mov          (32)(%rsp), %rax
    mov          (%rax), %eax
    test         %eax, %eax
    jnz          LDecHuffInfillT08gas_1
 
LDecHuffInfillT02gas_1:  
    mov          $(64), %ecx
    sub          %ebp, %ecx
    movd         %ecx, %mm1
    psrlq        %mm1, %mm0
    mov          $(1), %eax
    movd         %eax, %mm1
    movd         %mm0, %ecx
    movd         %mm7, %eax
    add          (%rsp), %rax
 
LDecHuffInfillT03gas_1:  
    cmp          $(24), %ebp
    jg           LDecHuffInfillT06gas_1
 
    movd         %mm7, %edx
    cmp          (8)(%rsp), %edx
    je           LDecHuffInfillT06gas_1
 
    movzbl       (%rax), %edx
    add          $(1), %rax
    paddd        %mm1, %mm7
 
    cmp          $(255), %edx
    jne          LDecHuffInfillT04gas_1
 
    movd         %mm7, %edx
    cmp          (8)(%rsp), %edx
    jge          LDecHuffInfillT09gas_1
 
    movzbl       (%rax), %edx
    add          $(1), %rax
    paddd        %mm1, %mm7
 
    test         $(255), %edx
    jnz          LDecHuffInfillT05gas_1
    mov          $(255), %edx
 
LDecHuffInfillT04gas_1:  
    shl          $(8), %ecx
    or           %edx, %ecx
    add          $(8), %ebp
    jmp          LDecHuffInfillT03gas_1
 
LDecHuffInfillT05gas_1:  
    movd         %edx, %mm1
    mov          (32)(%rsp), %rdx
    movd         %mm1, (%rdx)
 
LDecHuffInfillT06gas_1:  
    mov          $(64), %eax
    movd         %ecx, %mm0
    sub          %ebp, %eax
    movd         %eax, %mm1
    mov          (48)(%rsp), %rax
    psllq        %mm1, %mm0
LDecHuffInfillT07gas_1:  
    mov          (48)(%rsp), %rax
    jmp          *%rax
 
LDecHuffInfillT08gas_1:  
    cmp          %ebx, %ebp
    jge          LDecHuffInfillT07gas_1
    movl         $(-63), (56)(%rsp)
    jmp          LDecHuffInfillT07gas_1
 
LDecHuffInfillT09gas_1:  
    movl         $(-62), (56)(%rsp)
    jmp          LDecHuffInfillT06gas_1
 
 
LDecHuffLongLoadT00gas_1:  
    lea          LDecHuffLongRetLoadT00gas_1(%rip), %rax
    mov          %rax, (48)(%rsp)
    jmp          LDecHuffInfillT00gas_1
LDecHuffLongRetLoadT00gas_1:  
    cmpl         $(0), (56)(%rsp)
    jne          LDecHuffExit00gas_1
    cmp          %ebx, %ebp
    jl           LDecHuffErrorSrcOut00gas_1
    jmp          LDecHuffLongT03gas_1
 
LDecHuffAcLoadT00gas_1:  
    lea          LDecHuffAcRetLoadT00gas_1(%rip), %rax
    mov          %rax, (48)(%rsp)
    jmp          LDecHuffInfillT00gas_1
LDecHuffAcRetLoadT00gas_1:  
    cmpl         $(0), (56)(%rsp)
    jne          LDecHuffExit00gas_1
    cmp          %ebx, %ebp
    jl           LDecHuffErrorSrcOut00gas_1
    jmp          LDecHuffAcT01gas_1
 
LDecHuffAcLoadT01gas_1:  
    lea          LDecHuffAcRetLoadT01gas_1(%rip), %rax
    mov          %rax, (48)(%rsp)
    jmp          LDecHuffInfillT00gas_1
LDecHuffAcRetLoadT01gas_1:  
    cmpl         $(0), (56)(%rsp)
    jne          LDecHuffExit00gas_1
    cmp          %ebx, %ebp
    jl           LDecHuffErrorSrcOut00gas_1
    jmp          LDecHuffAcT03gas_1
 
LDecHuffEndOfBlockRunLoadT00gas_1:  
    lea          LDecHuffEndOfBlockRunRetLoadT00gas_1(%rip), %rax
    mov          %rax, (48)(%rsp)
    jmp          LDecHuffInfillT00gas_1
LDecHuffEndOfBlockRunRetLoadT00gas_1:  
    cmpl         $(0), (56)(%rsp)
    jne          LDecHuffEndOfBlockRunRetLoadT01gas_1
    cmp          %ebx, %ebp
    jge          LDecHuffEndOfBlockRun02gas_1
    movl         $(-62), (56)(%rsp)
LDecHuffEndOfBlockRunRetLoadT01gas_1:  
    movq         (152)(%rsp), %rax
    lea          (12)(%rax), %rax
    movd         %mm4, (%rax)
    jmp          LDecHuffExit00gas_1
 
 
.p2align 4, 0x90
 
 
.globl mfxownpj_DecodeHuffman8x8_ACRefine_JPEG_1u16s_C1

 
mfxownpj_DecodeHuffman8x8_ACRefine_JPEG_1u16s_C1:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    push         %r12
 
 
    push         %r13
 
 
    push         %r14
 
 
    push         %r15
 
 
    sub          $(120), %rsp
 
 
    mov          %rdi, (64)(%rsp)
    mov          %esi, %esi
    mov          %rsi, (72)(%rsp)
    mov          %rdx, (80)(%rsp)
    mov          %rcx, (88)(%rsp)
    mov          %r8, (96)(%rsp)
    mov          %r9d, %r9d
    mov          %r9, (104)(%rsp)
    mov          (176)(%rsp), %eax
    mov          %rax, (176)(%rsp)
    mov          (184)(%rsp), %eax
    mov          %rax, (184)(%rsp)
 
 
    mov          $(0), %rax
    mov          %rax, (16)(%rsp)
    movq         (200)(%rsp), %rcx
    lea          (8)(%rcx), %rax
    mov          (%rax), %ebp
    lea          (%rcx), %rax
    movq         (%rax), %mm0
 
    mov          $(64), %eax
    sub          %ebp, %eax
    movd         %eax, %mm1
    psllq        %mm1, %mm0
 
    mov          (80)(%rsp), %rax
    mov          (%rax), %eax
    mov          %rax, (%rsp)
 
    mov          (176)(%rsp), %rax
    sub          $(63), %rax
    lea          ownZigzag(%rip), %rsi
    sub          %rax, %rsi
    mov          %rsi, (8)(%rsp)
    mov          (176)(%rsp), %edi
    sub          (104)(%rsp), %edi
 
    mov          (184)(%rsp), %ecx
    mov          $(1), %eax
    shl          %cl, %eax
    movd         %eax, %mm7
    mov          $(-1), %eax
    shl          %cl, %eax
    movd         %eax, %mm6
    pxor         %mm5, %mm5
 
    movq         (192)(%rsp), %rax
    lea          (512)(%rax), %rax
    mov          %rax, (56)(%rsp)
 
 
    movq         (200)(%rsp), %rax
    lea          (12)(%rax), %rax
    mov          (%rax), %eax
    cmp          $(0), %eax
    jnz          LDecRefine2Coef10gas_2
 
 
LDecHuff00gas_2:  
    cmp          $(8), %ebp
    jl           LDecHuffLoad00gas_2
LDecHuffLoadRet00gas_2:  
    movq         %mm0, %mm1
    psrlq        $(56), %mm0
    movd         %mm0, %ecx
    mov          (56)(%rsp), %rax
    movq         %mm1, %mm0
    mov          (%rax,%rcx,4), %eax
    test         $(4294901760), %eax
    jz           LDecHuffLong00gas_2
    mov          %eax, %ebx
    shr          $(16), %eax
    movd         %eax, %mm1
    sub          %eax, %ebp
    psllq        %mm1, %mm0
LDecHuffLongRetgas_2:  
    mov          %ebx, %eax
    shr          $(4), %eax
    and          $(15), %eax
    mov          %eax, (40)(%rsp)
    and          $(15), %ebx
    mov          %ebx, (48)(%rsp)
    jz           LDecEndOfBlockRun00gas_2
 
    sub          $(1), %ebp
    jl           LDecHuffLoad01gas_2
LDecHuffLoadRet01gas_2:  
    pxor         %mm2, %mm2
    movq         maskFF(%rip), %mm1
    pcmpgtd      %mm0, %mm2
    psllq        $(1), %mm0
    psrlq        $(32), %mm2
    pxor         %mm2, %mm1
    pand         %mm7, %mm2
    pand         %mm6, %mm1
    por          %mm2, %mm1
    movd         %mm1, (48)(%rsp)
    jmp          LDecRefine1Coef00gas_2
 
 
LDecRefine1Coef01gas_2:  
    mov          (40)(%rsp), %eax
    mov          (8)(%rsp), %rsi
    sub          $(1), %eax
    mov          %eax, (40)(%rsp)
    jl           LDecHuff02gas_2
 
    sub          $(1), %rdi
    jl           LDecHuff02gas_2
 
LDecRefine1Coef00gas_2:  
    movzbl       (%rdi,%rsi), %esi
    add          (88)(%rsp), %rsi
    movswl       (%rsi), %eax
    movd         %eax, %mm1
    test         %eax, %eax
    jz           LDecRefine1Coef01gas_2
 
    sub          $(1), %ebp
    jl           LDecRefine1Load00gas_2
LDecRefine1LoadRet00gas_2:  
    pxor         %mm2, %mm2
    movq         %mm1, %mm3
    pand         %mm7, %mm1
    pcmpgtd      %mm0, %mm2
    psllq        $(1), %mm0
    psrlq        $(32), %mm2
    pcmpeqd      %mm5, %mm1
    pand         %mm2, %mm1
    movq         %mm3, %mm2
    pcmpgtd      %mm5, %mm3
    movq         %mm3, %mm4
    pxor         %mm1, %mm3
    pand         %mm7, %mm4
    pand         %mm6, %mm3
    por          %mm4, %mm3
    pand         %mm1, %mm3
    paddd        %mm3, %mm2
    movd         %mm2, %eax
    mov          %ax, (%rsi)
    mov          (8)(%rsp), %rsi
    sub          $(1), %rdi
    jge          LDecRefine1Coef00gas_2
 
LDecHuff02gas_2:  
    mov          (48)(%rsp), %eax
    test         %eax, %eax
    jz           LDecHuff03gas_2
 
    movzbl       (%rdi,%rsi), %ecx
    add          (88)(%rsp), %rcx
    mov          %ax, (%rcx)
 
LDecHuff03gas_2:  
    cmp          $(0), %edi
    jl           LDecRefine2Coef00gas_2
    sub          $(1), %rdi
    jge          LDecHuff00gas_2
 
 
LDecRefine2Coef00gas_2:  
    movq         (200)(%rsp), %rax
    lea          (12)(%rax), %rax
    mov          (%rax), %eax
    cmp          $(0), %eax
LDecRefine2Coef10gas_2:  
    jle          LDecRefineExit00gas_2
    cmp          $(0), %edi
    jl           LDecRefine2Coef03gas_2
    jmp          LDecRefine2Coef01gas_2
 
LDecRefine2Coef02gas_2:  
    mov          (8)(%rsp), %rsi
    sub          $(1), %edi
    jl           LDecRefine2Coef03gas_2
 
LDecRefine2Coef01gas_2:  
    movzbl       (%rdi,%rsi), %esi
    add          (88)(%rsp), %rsi
    movswl       (%rsi), %eax
    movd         %eax, %mm1
    test         %eax, %eax
    jz           LDecRefine2Coef02gas_2
 
    sub          $(1), %ebp
    jl           LDecRefine2Load00gas_2
LDecRefine2LoadRet00gas_2:  
    pxor         %mm2, %mm2
    movq         %mm1, %mm3
    pand         %mm7, %mm1
    pcmpgtd      %mm0, %mm2
    psllq        $(1), %mm0
    psrlq        $(32), %mm2
    pcmpeqd      %mm5, %mm1
    pand         %mm2, %mm1
    movq         %mm3, %mm2
    pcmpgtd      %mm5, %mm3
    movq         %mm3, %mm4
    pxor         %mm1, %mm3
    pand         %mm7, %mm4
    pand         %mm6, %mm3
    por          %mm4, %mm3
    pand         %mm1, %mm3
    paddd        %mm3, %mm2
    movd         %mm2, %eax
    mov          %ax, (%rsi)
    mov          (8)(%rsp), %rsi
    sub          $(1), %edi
    jge          LDecRefine2Coef01gas_2
 
LDecRefine2Coef03gas_2:  
    movq         (200)(%rsp), %rax
    lea          (12)(%rax), %rbx
    mov          (%rbx), %eax
    sub          $(1), %eax
    mov          %eax, (%rbx)
 
 
LDecRefineExit00gas_2:  
    mov          $(64), %eax
    sub          %ebp, %eax
    movd         %eax, %mm1
    psrlq        %mm1, %mm0
    movq         (200)(%rsp), %rdx
    lea          (8)(%rdx), %rax
    mov          %ebp, (%rax)
    lea          (%rdx), %rax
    movq         %mm0, (%rax)
    mov          (%rsp), %edx
    mov          (80)(%rsp), %rax
    mov          %edx, (%rax)
    mov          (16)(%rsp), %rax
    emms
    add          $(120), %rsp
 
 
    pop          %r15
 
 
    pop          %r14
 
 
    pop          %r13
 
 
    pop          %r12
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
LDecEndOfBlockRun00gas_2:  
    cmp          $(15), %eax
    je           LDecRefine1Coef00gas_2
 
    mov          %eax, %ecx
    mov          $(1), %eax
    shl          %cl, %eax
    test         %ecx, %ecx
    jz           LDecEndOfBlockRun01gas_2
 
    movd         %eax, %mm4
    mov          %ecx, %ebx
    cmp          %ecx, %ebp
    jl           LDecEndOfBlockRunLoad00gas_2
LDecEndOfBlockRunLoadRet00gas_2:  
    pextrw       $(3), %mm0, %edx
    sub          %ebx, %ebp
    mov          $(16), %ecx
    movd         %ebx, %mm1
    sub          %ebx, %ecx
    psllq        %mm1, %mm0
    shr          %cl, %edx
    movd         %mm4, %eax
    add          %edx, %eax
 
LDecEndOfBlockRun01gas_2:  
    movq         (200)(%rsp), %rcx
    lea          (12)(%rcx), %rbx
    mov          %eax, (%rbx)
    jmp          LDecRefine2Coef00gas_2
 
 
LDecRefineError00gas_2:  
    cmpl         $(10), (16)(%rsp)
    je           LDecHuffErrorRng00gas_2
 
LDecHuffErrorSrcOut00gas_2:  
    movl         $(-62), (16)(%rsp)
    jmp          LDecRefineExit00gas_2
 
LDecHuffErrorRng00gas_2:  
    movl         $(-63), (16)(%rsp)
    jmp          LDecRefineExit00gas_2
 
 
LDecRefineInfill00gas_2:  
    mov          (96)(%rsp), %rax
    mov          (%rax), %eax
    test         %eax, %eax
    jnz          LDecRefineInfill08gas_2
 
    mov          (72)(%rsp), %edx
    mov          (%rsp), %eax
    sub          %eax, %edx
    add          (64)(%rsp), %rax
    cmp          $(8), %edx
    jl           LDecRefineInfill01gas_2
 
    mov          (%rax), %ecx
    mov          (4)(%rax), %edx
    bswap        %ecx
    movd         %ecx, %mm2
    mov          $(64), %ecx
    bswap        %edx
    sub          %ebp, %ecx
    movd         %edx, %mm1
    movd         %ecx, %mm3
    punpckldq    %mm2, %mm1
    movq         maskFF(%rip), %mm2
    psrlq        %mm3, %mm0
    pcmpeqb      %mm1, %mm2
    pmovmskb     %mm2, %ecx
    test         %ecx, %ecx
    jnz          LDecRefineInfill02gas_2
    lea          (7)(%rbp), %rcx
    and          $(4294967288), %ecx
    mov          $(64), %edx
    movd         %ecx, %mm2
    sub          %ecx, %edx
    psrlq        %mm2, %mm1
    movd         %edx, %mm2
    add          %edx, %ebp
    shr          $(3), %edx
    psllq        %mm2, %mm0
    mov          $(64), %ecx
    por          %mm1, %mm0
    mov          (%rsp), %eax
    sub          %ebp, %ecx
    add          %edx, %eax
    movd         %ecx, %mm1
    mov          %eax, (%rsp)
    mov          (32)(%rsp), %rax
    psllq        %mm1, %mm0
    jmp          *%rax
 
LDecRefineInfill01gas_2:  
    mov          $(64), %ecx
    sub          %ebp, %ecx
    movd         %ecx, %mm1
    psrlq        %mm1, %mm0
 
LDecRefineInfill02gas_2:  
    mov          (%rsp), %ecx
    movd         %mm0, %ebx
 
LDecRefineInfill03gas_2:  
    cmp          $(24), %ebp
    jg           LDecRefineInfill06gas_2
 
    cmp          (72)(%rsp), %ecx
    je           LDecRefineInfill06gas_2
 
    movzbl       (%rax), %edx
    add          $(1), %rax
    add          $(1), %ecx
 
    cmp          $(255), %edx
    jne          LDecRefineInfill04gas_2
 
    cmp          (72)(%rsp), %ecx
    jge          LDecRefineInfill09gas_2
 
    movzbl       (%rax), %edx
    add          $(1), %rax
    add          $(1), %ecx
 
    test         $(255), %edx
    jnz          LDecRefineInfill05gas_2
    mov          $(255), %edx
 
LDecRefineInfill04gas_2:  
    shl          $(8), %ebx
    or           %edx, %ebx
    add          $(8), %ebp
    jmp          LDecRefineInfill03gas_2
 
LDecRefineInfill05gas_2:  
    mov          (96)(%rsp), %rax
    mov          %edx, (%rax)
 
LDecRefineInfill06gas_2:  
    mov          %ecx, (%rsp)
    mov          $(32), %ecx
    sub          %ebp, %ecx
    shl          %cl, %ebx
    movd         %ebx, %mm0
    psllq        $(32), %mm0
LDecRefineInfill07gas_2:  
    mov          (32)(%rsp), %rax
    jmp          *%rax
 
LDecRefineInfill08gas_2:  
    movl         $(10), (16)(%rsp)
    jmp          LDecRefineInfill07gas_2
 
LDecRefineInfill09gas_2:  
    movl         $(-62), (16)(%rsp)
    jmp          LDecRefineInfill06gas_2
 
 
LDecHuffLoad00gas_2:  
    lea          LDecHuffLoad10gas_2(%rip), %rax
    mov          %rax, (32)(%rsp)
    jmp          LDecRefineInfill00gas_2
LDecHuffLoad10gas_2:  
    cmpl         $(-62), (16)(%rsp)
    je           LDecRefineExit00gas_2
    cmp          $(8), %ebp
    jge          LDecHuffLoadRet00gas_2
    mov          $(1), %ebx
    jmp          LDecHuffLong01gas_2
 
LDecHuffLongLoad00gas_2:  
    lea          LDecHuffLongLoad10gas_2(%rip), %rax
    mov          %rax, (32)(%rsp)
    mov          %ebx, (24)(%rsp)
    jmp          LDecRefineInfill00gas_2
LDecHuffLongLoad10gas_2:  
    cmpl         $(-62), (16)(%rsp)
    je           LDecRefineExit00gas_2
    mov          (24)(%rsp), %ebx
    cmp          %ebx, %ebp
    jl           LDecRefineError00gas_2
    jmp          LDecHuffLongLoadRet00gas_2
 
LDecHuffLoad01gas_2:  
    lea          LDecHuffLoad11gas_2(%rip), %rax
    mov          %rax, (32)(%rsp)
    add          $(1), %ebp
    jmp          LDecRefineInfill00gas_2
LDecHuffLoad11gas_2:  
    cmpl         $(-62), (16)(%rsp)
    je           LDecRefineExit00gas_2
    sub          $(1), %ebp
    jle          LDecRefineError00gas_2
    jmp          LDecHuffLoadRet01gas_2
 
LDecEndOfBlockRunLoad00gas_2:  
    lea          LDecEndOfBlockRunLoad10gas_2(%rip), %rax
    mov          %rax, (32)(%rsp)
    mov          %ebx, (24)(%rsp)
    jmp          LDecRefineInfill00gas_2
LDecEndOfBlockRunLoad10gas_2:  
    cmpl         $(-62), (16)(%rsp)
    je           LDecEndOfBlockRunLoad11gas_2
    mov          (24)(%rsp), %ebx
    cmp          %ebx, %ebp
    jge          LDecEndOfBlockRunLoadRet00gas_2
LDecEndOfBlockRunLoad11gas_2:  
    movq         (200)(%rsp), %rax
    lea          (12)(%rax), %rax
    movd         %mm4, (%rax)
    cmpl         $(10), (16)(%rsp)
    je           LDecHuffErrorRng00gas_2
    jmp          LDecHuffErrorSrcOut00gas_2
 
LDecRefine1Load00gas_2:  
    lea          LDecRefine1Load10gas_2(%rip), %rax
    mov          %rax, (32)(%rsp)
    movd         %mm1, (24)(%rsp)
    add          $(1), %ebp
    jmp          LDecRefineInfill00gas_2
LDecRefine1Load10gas_2:  
    cmpl         $(-62), (16)(%rsp)
    je           LDecRefineExit00gas_2
    movd         (24)(%rsp), %mm1
    sub          $(1), %ebp
    jl           LDecRefineError00gas_2
    jmp          LDecRefine1LoadRet00gas_2
 
LDecRefine2Load00gas_2:  
    lea          LDecRefine2Load10gas_2(%rip), %rax
    mov          %rax, (32)(%rsp)
    movd         %mm1, (24)(%rsp)
    add          $(1), %ebp
    jmp          LDecRefineInfill00gas_2
LDecRefine2Load10gas_2:  
    cmpl         $(0), (16)(%rsp)
    jl           LDecRefineExit00gas_2
    movd         (24)(%rsp), %mm1
    sub          $(1), %ebp
    jl           LDecRefineError00gas_2
    jmp          LDecRefine2LoadRet00gas_2
 
 
LDecHuffLong00gas_2:  
    mov          $(9), %ebx
 
LDecHuffLong01gas_2:  
    movq         (192)(%rsp), %rax
    mov          %rax, %r10
 
LDecHuffLong02gas_2:  
    cmp          %ebx, %ebp
    jl           LDecHuffLongLoad00gas_2
LDecHuffLongLoadRet00gas_2:  
    mov          %r10, %rax
    lea          (1572)(%rax), %rdx
    movq         %mm0, %mm1
    mov          $(64), %eax
    sub          %ebx, %eax
    movd         %eax, %mm2
    movswl       (%rdx,%rbx,2), %eax
    psrlq        %mm2, %mm1
    movd         %mm1, %ecx
    test         $(32768), %eax
    jz           LDecHuffLong03gas_2
    cmp          $(-1), %eax
    je           LDecHuffLong03gas_2
    movzwl       (%rdx,%rbx,2), %eax
LDecHuffLong03gas_2:  
    cmp          %eax, %ecx
    jle          LDecHuffLong04gas_2
    add          $(1), %ebx
    cmp          $(16), %ebx
    jg           LDecHuffErrorRng00gas_2
    jmp          LDecHuffLong02gas_2
 
LDecHuffLong04gas_2:  
    movd         %ebx, %mm1
    psllq        %mm1, %mm0
    sub          %ebx, %ebp
    mov          %r10, %rax
    lea          (1536)(%rax), %rdx
    movzwl       (%rdx,%rbx,2), %eax
    sub          %eax, %ecx
    mov          %r10, %rax
    lea          (1608)(%rax), %rdx
    movzwl       (%rdx,%rbx,2), %eax
    add          %eax, %ecx
    mov          %r10, %rax
    lea          (%rax), %rdx
    movzwl       (%rdx,%rcx,2), %ebx
    jmp          LDecHuffLongRetgas_2
 
 
.data
 
.p2align 4, 0x90
 
maskFF:
.quad   0xffffffffffffffff  
 

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
 
 

.byte  63, 63, 63, 63, 63, 63, 63, 63  
 
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
 
 
