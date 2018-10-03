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
 
 
CONST_0F:
.quad     0xFF00FF00FF00FF,    0xFF00FF00FF00FF  
 
CONST_1:
.quad      0x1000100010001,     0x1000100010001  
 
CONST_3:
.quad      0x3000300030003,     0x3000300030003  
 
CONST_x1:
.quad      0x1000000010000,     0x1000000010000  
 
CONST_x3:
.quad      0x2000100020001,     0x2000100020001  
 
CONST_1x:
.quad          0x100000001,         0x100000001  
 
CONST_3x:
.quad      0x1000200010002,     0x1000200010002  
 
 
.text
 
 
.p2align 4, 0x90
 
.globl mfxownpj_SampleDownH2V1_JPEG_8u_C1

 
mfxownpj_SampleDownH2V1_JPEG_8u_C1:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    mov          %edx, %edx
    test         $(1), %rdi
    jne          LL1gas_1
    mov          %rsi, %rbp
    and          $(15), %rbp
    jz           LRungas_1
    sub          $(16), %rbp
    neg          %rbp
    cmp          %rbp, %rdx
    jle          LRungas_1
    sub          %rbp, %rdx
LLNgas_1:  
    movzbq       (%rdi), %rax
    movzbq       (1)(%rdi), %rbx
    lea          (1)(%rbx,%rax), %rax
    shr          $(1), %rax
    movb         %al, (%rsi)
    add          $(2), %rdi
    add          $(1), %rsi
    sub          $(1), %rbp
    jnz          LLNgas_1
LRungas_1:  
    test         $(15), %rdi
    jnz          LNAgas_1
    sub          $(32), %rdx
    jl           LL16gas_1
LL32gas_1:  
    movdqa       (%rdi), %xmm0
    movdqa       (16)(%rdi), %xmm2
    movdqa       (32)(%rdi), %xmm4
    movdqa       (48)(%rdi), %xmm6
    movdqa       %xmm0, %xmm1
    movdqa       %xmm2, %xmm3
    movdqa       %xmm4, %xmm5
    movdqa       %xmm6, %xmm7
    psrlw        $(8), %xmm0
    psrlw        $(8), %xmm2
    psrlw        $(8), %xmm4
    psrlw        $(8), %xmm6
    pand         CONST_0F(%rip), %xmm1
    pand         CONST_0F(%rip), %xmm3
    pand         CONST_0F(%rip), %xmm5
    pand         CONST_0F(%rip), %xmm7
    paddw        %xmm1, %xmm0
    paddw        %xmm3, %xmm2
    paddw        %xmm5, %xmm4
    paddw        %xmm7, %xmm6
    paddw        CONST_1(%rip), %xmm0
    paddw        CONST_1(%rip), %xmm2
    paddw        CONST_1(%rip), %xmm4
    paddw        CONST_1(%rip), %xmm6
    psrlw        $(1), %xmm0
    psrlw        $(1), %xmm2
    psrlw        $(1), %xmm4
    psrlw        $(1), %xmm6
    packuswb     %xmm2, %xmm0
    packuswb     %xmm6, %xmm4
    movdqa       %xmm0, (%rsi)
    movdqa       %xmm4, (16)(%rsi)
    add          $(64), %rdi
    add          $(32), %rsi
    sub          $(32), %rdx
    jge          LL32gas_1
LL16gas_1:  
    add          $(16), %rdx
    jl           LL8gas_1
    movdqa       (%rdi), %xmm0
    movdqa       (16)(%rdi), %xmm2
    movdqa       %xmm0, %xmm1
    movdqa       %xmm2, %xmm3
    psrlw        $(8), %xmm0
    psrlw        $(8), %xmm2
    pand         CONST_0F(%rip), %xmm1
    pand         CONST_0F(%rip), %xmm3
    paddw        %xmm1, %xmm0
    paddw        %xmm3, %xmm2
    paddw        CONST_1(%rip), %xmm0
    paddw        CONST_1(%rip), %xmm2
    psrlw        $(1), %xmm0
    psrlw        $(1), %xmm2
    packuswb     %xmm2, %xmm0
    movdqa       %xmm0, (%rsi)
    add          $(32), %rdi
    add          $(16), %rsi
    sub          $(16), %rdx
LL8gas_1:  
    add          $(8), %rdx
    jl           LL4gas_1
    movdqa       (%rdi), %xmm4
    movdqa       %xmm4, %xmm5
    pand         CONST_0F(%rip), %xmm4
    psrlw        $(8), %xmm5
    paddw        %xmm4, %xmm5
    paddw        CONST_1(%rip), %xmm5
    psrlw        $(1), %xmm5
    packuswb     %xmm5, %xmm5
    movq         %xmm5, (%rsi)
    add          $(16), %rdi
    add          $(8), %rsi
    sub          $(8), %rdx
    jmp          LL4gas_1
LNAgas_1:  
    sub          $(32), %rdx
    jl           LL16ngas_1
LL32ngas_1:  
    lddqu        (%rdi), %xmm0
    lddqu        (16)(%rdi), %xmm2
    lddqu        (32)(%rdi), %xmm4
    lddqu        (48)(%rdi), %xmm6
    movdqa       %xmm0, %xmm1
    movdqa       %xmm2, %xmm3
    movdqa       %xmm4, %xmm5
    movdqa       %xmm6, %xmm7
    psrlw        $(8), %xmm0
    psrlw        $(8), %xmm2
    psrlw        $(8), %xmm4
    psrlw        $(8), %xmm6
    pand         CONST_0F(%rip), %xmm1
    pand         CONST_0F(%rip), %xmm3
    pand         CONST_0F(%rip), %xmm5
    pand         CONST_0F(%rip), %xmm7
    paddw        %xmm1, %xmm0
    paddw        %xmm3, %xmm2
    paddw        %xmm5, %xmm4
    paddw        %xmm7, %xmm6
    paddw        CONST_1(%rip), %xmm0
    paddw        CONST_1(%rip), %xmm2
    paddw        CONST_1(%rip), %xmm4
    paddw        CONST_1(%rip), %xmm6
    psrlw        $(1), %xmm0
    psrlw        $(1), %xmm2
    psrlw        $(1), %xmm4
    psrlw        $(1), %xmm6
    packuswb     %xmm2, %xmm0
    packuswb     %xmm6, %xmm4
    movdqa       %xmm0, (%rsi)
    movdqa       %xmm4, (16)(%rsi)
    add          $(64), %rdi
    add          $(32), %rsi
    sub          $(32), %rdx
    jge          LL32ngas_1
LL16ngas_1:  
    add          $(16), %rdx
    jl           LL8ngas_1
    lddqu        (%rdi), %xmm0
    lddqu        (16)(%rdi), %xmm2
    movdqa       %xmm0, %xmm1
    movdqa       %xmm2, %xmm3
    psrlw        $(8), %xmm0
    psrlw        $(8), %xmm2
    pand         CONST_0F(%rip), %xmm1
    pand         CONST_0F(%rip), %xmm3
    paddw        %xmm1, %xmm0
    paddw        %xmm3, %xmm2
    paddw        CONST_1(%rip), %xmm0
    paddw        CONST_1(%rip), %xmm2
    psrlw        $(1), %xmm0
    psrlw        $(1), %xmm2
    packuswb     %xmm2, %xmm0
    movdqa       %xmm0, (%rsi)
    add          $(32), %rdi
    add          $(16), %rsi
    sub          $(16), %rdx
LL8ngas_1:  
    add          $(8), %rdx
    jl           LL4gas_1
    lddqu        (%rdi), %xmm4
    movdqa       %xmm4, %xmm5
    pand         CONST_0F(%rip), %xmm4
    psrlw        $(8), %xmm5
    paddw        %xmm4, %xmm5
    paddw        CONST_1(%rip), %xmm5
    psrlw        $(1), %xmm5
    packuswb     %xmm5, %xmm5
    movq         %xmm5, (%rsi)
    add          $(16), %rdi
    add          $(8), %rsi
    sub          $(8), %rdx
LL4gas_1:  
    add          $(4), %rdx
    jl           LL3gas_1
    movq         (%rdi), %xmm6
    movdqa       %xmm6, %xmm7
    pand         CONST_0F(%rip), %xmm6
    psrlw        $(8), %xmm7
    paddw        %xmm6, %xmm7
    paddw        CONST_1(%rip), %xmm7
    psrlw        $(1), %xmm7
    packuswb     %xmm7, %xmm7
    movd         %xmm7, (%rsi)
    add          $(8), %rdi
    add          $(4), %rsi
    sub          $(4), %rdx
LL3gas_1:  
    add          $(4), %rdx
    jle          Lexitgas_1
LL1gas_1:  
    movzbq       (%rdi), %rax
    movzbq       (1)(%rdi), %rbx
    lea          (1)(%rbx,%rax), %rax
    shr          $(1), %rax
    movb         %al, (%rsi)
    add          $(2), %rdi
    add          $(1), %rsi
    sub          $(1), %rdx
    jnz          LL1gas_1
Lexitgas_1:  
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownpj_SampleDownH2V2_JPEG_8u_C1

 
mfxownpj_SampleDownH2V2_JPEG_8u_C1:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    mov          %ecx, %ecx
    test         $(1), %rdi
    jne          LL1gas_2
    mov          %rdx, %rbp
    and          $(15), %rbp
    jz           LRungas_2
    sub          $(16), %rbp
    neg          %rbp
    cmp          %rbp, %rcx
    jle          LRungas_2
    sub          %rbp, %rcx
LLNgas_2:  
    movzbq       (%rdi), %rax
    movzbq       (1)(%rdi), %rbx
    lea          (3)(%rbx,%rax), %rax
    movzbq       (%rdi,%rsi), %rbx
    lea          (%rbx,%rax), %rax
    movzbq       (1)(%rdi,%rsi), %rbx
    lea          (%rbx,%rax), %rax
    shr          $(2), %rax
    movb         %al, (%rdx)
    add          $(2), %rdi
    add          $(1), %rdx
    sub          $(1), %rbp
    jnz          LLNgas_2
LRungas_2:  
    test         $(15), %rdi
    jnz          LNAgas_2
    sub          $(16), %rcx
    jl           LL8gas_2
LL16gas_2:  
    movdqa       (%rdi), %xmm0
    movdqa       (16)(%rdi), %xmm2
    movdqu       (%rdi,%rsi), %xmm4
    movdqu       (16)(%rdi,%rsi), %xmm6
    movdqa       %xmm0, %xmm1
    movdqa       %xmm2, %xmm3
    movdqa       %xmm4, %xmm5
    movdqa       %xmm6, %xmm7
    psrlw        $(8), %xmm0
    psrlw        $(8), %xmm2
    psrlw        $(8), %xmm4
    psrlw        $(8), %xmm6
    pand         CONST_0F(%rip), %xmm1
    pand         CONST_0F(%rip), %xmm3
    pand         CONST_0F(%rip), %xmm5
    pand         CONST_0F(%rip), %xmm7
    paddw        %xmm1, %xmm0
    paddw        %xmm3, %xmm2
    paddw        %xmm5, %xmm4
    paddw        %xmm7, %xmm6
    paddw        %xmm4, %xmm0
    paddw        %xmm6, %xmm2
    paddw        CONST_3(%rip), %xmm0
    paddw        CONST_3(%rip), %xmm2
    psrlw        $(2), %xmm0
    psrlw        $(2), %xmm2
    packuswb     %xmm2, %xmm0
    movdqa       %xmm0, (%rdx)
    add          $(32), %rdi
    add          $(16), %rdx
    sub          $(16), %rcx
    jge          LL16gas_2
LL8gas_2:  
    add          $(8), %rcx
    jl           LL4gas_2
    movdqa       (%rdi), %xmm0
    movdqu       (%rdi,%rsi), %xmm1
    movdqa       %xmm0, %xmm4
    movdqa       %xmm1, %xmm5
    pand         CONST_0F(%rip), %xmm4
    pand         CONST_0F(%rip), %xmm5
    psrlw        $(8), %xmm0
    psrlw        $(8), %xmm1
    paddw        %xmm4, %xmm0
    paddw        %xmm5, %xmm1
    paddw        %xmm1, %xmm0
    paddw        CONST_3(%rip), %xmm0
    psrlw        $(2), %xmm0
    packuswb     %xmm0, %xmm0
    movq         %xmm0, (%rdx)
    add          $(16), %rdi
    add          $(8), %rdx
    sub          $(8), %rcx
    jmp          LL4gas_2
LNAgas_2:  
    sub          $(16), %rcx
    jl           LL8ngas_2
LL16ngas_2:  
    lddqu        (%rdi), %xmm0
    lddqu        (16)(%rdi), %xmm2
    movdqu       (%rdi,%rsi), %xmm4
    movdqu       (16)(%rdi,%rsi), %xmm6
    movdqa       %xmm0, %xmm1
    movdqa       %xmm2, %xmm3
    movdqa       %xmm4, %xmm5
    movdqa       %xmm6, %xmm7
    psrlw        $(8), %xmm0
    psrlw        $(8), %xmm2
    psrlw        $(8), %xmm4
    psrlw        $(8), %xmm6
    pand         CONST_0F(%rip), %xmm1
    pand         CONST_0F(%rip), %xmm3
    pand         CONST_0F(%rip), %xmm5
    pand         CONST_0F(%rip), %xmm7
    paddw        %xmm1, %xmm0
    paddw        %xmm3, %xmm2
    paddw        %xmm5, %xmm4
    paddw        %xmm7, %xmm6
    paddw        %xmm4, %xmm0
    paddw        %xmm6, %xmm2
    paddw        CONST_3(%rip), %xmm0
    paddw        CONST_3(%rip), %xmm2
    psrlw        $(2), %xmm0
    psrlw        $(2), %xmm2
    packuswb     %xmm2, %xmm0
    movdqa       %xmm0, (%rdx)
    add          $(32), %rdi
    add          $(16), %rdx
    sub          $(16), %rcx
    jge          LL16ngas_2
LL8ngas_2:  
    add          $(8), %rcx
    jl           LL4gas_2
    lddqu        (%rdi), %xmm0
    movdqu       (%rdi,%rsi), %xmm1
    movdqa       %xmm0, %xmm4
    movdqa       %xmm1, %xmm5
    pand         CONST_0F(%rip), %xmm4
    pand         CONST_0F(%rip), %xmm5
    psrlw        $(8), %xmm0
    psrlw        $(8), %xmm1
    paddw        %xmm4, %xmm0
    paddw        %xmm5, %xmm1
    paddw        %xmm1, %xmm0
    paddw        CONST_3(%rip), %xmm0
    psrlw        $(2), %xmm0
    packuswb     %xmm0, %xmm0
    movq         %xmm0, (%rdx)
    add          $(16), %rdi
    add          $(8), %rdx
    sub          $(8), %rcx
LL4gas_2:  
    add          $(4), %rcx
    jl           LL3gas_2
    movq         (%rdi), %xmm2
    movq         (%rdi,%rsi), %xmm3
    movdqa       %xmm2, %xmm6
    movdqa       %xmm3, %xmm7
    pand         CONST_0F(%rip), %xmm6
    pand         CONST_0F(%rip), %xmm7
    psrlw        $(8), %xmm2
    psrlw        $(8), %xmm3
    paddw        %xmm6, %xmm2
    paddw        %xmm7, %xmm3
    paddw        %xmm3, %xmm2
    paddw        CONST_3(%rip), %xmm2
    psrlw        $(2), %xmm2
    packuswb     %xmm2, %xmm2
    movd         %xmm2, (%rdx)
    add          $(8), %rdi
    add          $(4), %rdx
    sub          $(4), %rcx
LL3gas_2:  
    add          $(4), %rcx
    jle          Lexitgas_2
LL1gas_2:  
    movzbq       (%rdi), %rax
    movzbq       (1)(%rdi), %rbp
    lea          (3)(%rbp,%rax), %rax
    movzbq       (%rdi,%rsi), %rbx
    movzbq       (1)(%rdi,%rsi), %rbp
    add          %rbp, %rbx
    add          %rbx, %rax
    shr          $(2), %rax
    movb         %al, (%rdx)
    add          $(2), %rdi
    add          $(1), %rdx
    sub          $(1), %rcx
    jnz          LL1gas_2
Lexitgas_2:  
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownpj_SampleDownRowH2V1_Box_JPEG_8u_C1

 
mfxownpj_SampleDownRowH2V1_Box_JPEG_8u_C1:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    mov          %esi, %esi
    cmp          $(2), %rsi
    je           Lend1gas_3
    cmp          $(4), %rsi
    je           Lend2gas_3
    cmp          $(6), %rsi
    je           Lend3gas_3
    cmp          $(8), %rsi
    je           Lend4gas_3
    cmp          $(10), %rsi
    je           Lend5gas_3
    cmp          $(12), %rsi
    je           Lend6gas_3
    cmp          $(14), %rsi
    je           Lend7gas_3
    mov          %rdx, %rbp
    and          $(15), %rbp
    jz           LRungas_3
    sub          $(16), %rbp
    neg          %rbp
    sub          %rbp, %rsi
 
 
    sub          %rbp, %rsi
    test         $(1), %rbp
    je           LL2ngas_3
    movzbq       (%rdi), %rax
    movzbq       (1)(%rdi), %rbx
    lea          (%rbx,%rax), %rax
    shr          $(1), %rax
    movb         %al, (%rdx)
    add          $(2), %rdi
    add          $(1), %rdx
    sub          $(1), %rbp
    jz           LRunOgas_3
LL2nOgas_3:  
    movzbq       (%rdi), %rax
    movzbq       (1)(%rdi), %rbx
    lea          (1)(%rbx,%rax), %rax
    shr          $(1), %rax
    movb         %al, (%rdx)
    movzbq       (2)(%rdi), %rax
    movzbq       (3)(%rdi), %rbx
    add          %rbx, %rax
    shr          $(1), %rax
    movb         %al, (1)(%rdx)
    add          $(4), %rdi
    add          $(2), %rdx
    sub          $(2), %rbp
    jnz          LL2nOgas_3
    jmp          LRunOgas_3
LL2ngas_3:  
    movzbq       (%rdi), %rax
    movzbq       (1)(%rdi), %rbx
    add          %rbx, %rax
    shr          $(1), %rax
    movb         %al, (%rdx)
    movzbq       (2)(%rdi), %rax
    movzbq       (3)(%rdi), %rbx
    lea          (1)(%rbx,%rax), %rax
    shr          $(1), %rax
    movb         %al, (1)(%rdx)
    add          $(4), %rdi
    add          $(2), %rdx
    sub          $(2), %rbp
    jnz          LL2ngas_3
LRungas_3:  
    test         $(15), %rdi
    jnz          LNAgas_3
    sub          $(64), %rsi
    jl           LL32gas_3
LL64gas_3:  
    movdqa       (%rdi), %xmm0
    movdqa       (16)(%rdi), %xmm2
    movdqa       (32)(%rdi), %xmm4
    movdqa       (48)(%rdi), %xmm6
    movdqa       %xmm0, %xmm1
    movdqa       %xmm2, %xmm3
    movdqa       %xmm4, %xmm5
    movdqa       %xmm6, %xmm7
    psrlw        $(8), %xmm0
    psrlw        $(8), %xmm2
    psrlw        $(8), %xmm4
    psrlw        $(8), %xmm6
    pand         CONST_0F(%rip), %xmm1
    pand         CONST_0F(%rip), %xmm3
    pand         CONST_0F(%rip), %xmm5
    pand         CONST_0F(%rip), %xmm7
    paddw        %xmm1, %xmm0
    paddw        %xmm3, %xmm2
    paddw        %xmm5, %xmm4
    paddw        %xmm7, %xmm6
    paddw        CONST_x1(%rip), %xmm0
    paddw        CONST_x1(%rip), %xmm2
    paddw        CONST_x1(%rip), %xmm4
    paddw        CONST_x1(%rip), %xmm6
    psrlw        $(1), %xmm0
    psrlw        $(1), %xmm2
    psrlw        $(1), %xmm4
    psrlw        $(1), %xmm6
    packuswb     %xmm2, %xmm0
    packuswb     %xmm6, %xmm4
    movdqa       %xmm0, (%rdx)
    movdqa       %xmm4, (16)(%rdx)
    add          $(64), %rdi
    add          $(32), %rdx
    sub          $(64), %rsi
    jge          LL64gas_3
LL32gas_3:  
    add          $(32), %rsi
    jl           LL16gas_3
    movdqa       (%rdi), %xmm0
    movdqa       (16)(%rdi), %xmm2
    movdqa       %xmm0, %xmm1
    movdqa       %xmm2, %xmm3
    psrlw        $(8), %xmm0
    psrlw        $(8), %xmm2
    pand         CONST_0F(%rip), %xmm1
    pand         CONST_0F(%rip), %xmm3
    paddw        %xmm1, %xmm0
    paddw        %xmm3, %xmm2
    paddw        CONST_x1(%rip), %xmm0
    paddw        CONST_x1(%rip), %xmm2
    psrlw        $(1), %xmm0
    psrlw        $(1), %xmm2
    packuswb     %xmm2, %xmm0
    movdqa       %xmm0, (%rdx)
    add          $(32), %rdi
    add          $(16), %rdx
    sub          $(32), %rsi
LL16gas_3:  
    add          $(16), %rsi
    jl           LL8gas_3
    movdqa       (%rdi), %xmm5
    movdqa       %xmm5, %xmm7
    pand         CONST_0F(%rip), %xmm5
    psrlw        $(8), %xmm7
    paddw        %xmm5, %xmm7
    paddw        CONST_x1(%rip), %xmm7
    psrlw        $(1), %xmm7
    packuswb     %xmm7, %xmm7
    movq         %xmm7, (%rdx)
    add          $(16), %rdi
    add          $(8), %rdx
    sub          $(16), %rsi
    jmp          LL8gas_3
LNAgas_3:  
    sub          $(64), %rsi
    jl           LL32ngas_3
LL64ngas_3:  
    lddqu        (%rdi), %xmm0
    lddqu        (16)(%rdi), %xmm2
    lddqu        (32)(%rdi), %xmm4
    lddqu        (48)(%rdi), %xmm6
    movdqa       %xmm0, %xmm1
    movdqa       %xmm2, %xmm3
    movdqa       %xmm4, %xmm5
    movdqa       %xmm6, %xmm7
    psrlw        $(8), %xmm0
    psrlw        $(8), %xmm2
    psrlw        $(8), %xmm4
    psrlw        $(8), %xmm6
    pand         CONST_0F(%rip), %xmm1
    pand         CONST_0F(%rip), %xmm3
    pand         CONST_0F(%rip), %xmm5
    pand         CONST_0F(%rip), %xmm7
    paddw        %xmm1, %xmm0
    paddw        %xmm3, %xmm2
    paddw        %xmm5, %xmm4
    paddw        %xmm7, %xmm6
    paddw        CONST_x1(%rip), %xmm0
    paddw        CONST_x1(%rip), %xmm2
    paddw        CONST_x1(%rip), %xmm4
    paddw        CONST_x1(%rip), %xmm6
    psrlw        $(1), %xmm0
    psrlw        $(1), %xmm2
    psrlw        $(1), %xmm4
    psrlw        $(1), %xmm6
    packuswb     %xmm2, %xmm0
    packuswb     %xmm6, %xmm4
    movdqa       %xmm0, (%rdx)
    movdqa       %xmm4, (16)(%rdx)
    add          $(64), %rdi
    add          $(32), %rdx
    sub          $(64), %rsi
    jge          LL64ngas_3
LL32ngas_3:  
    add          $(32), %rsi
    jl           LL16ngas_3
    lddqu        (%rdi), %xmm0
    lddqu        (16)(%rdi), %xmm2
    movdqa       %xmm0, %xmm1
    movdqa       %xmm2, %xmm3
    psrlw        $(8), %xmm0
    psrlw        $(8), %xmm2
    pand         CONST_0F(%rip), %xmm1
    pand         CONST_0F(%rip), %xmm3
    paddw        %xmm1, %xmm0
    paddw        %xmm3, %xmm2
    paddw        CONST_x1(%rip), %xmm0
    paddw        CONST_x1(%rip), %xmm2
    psrlw        $(1), %xmm0
    psrlw        $(1), %xmm2
    packuswb     %xmm2, %xmm0
    movdqa       %xmm0, (%rdx)
    add          $(32), %rdi
    add          $(16), %rdx
    sub          $(32), %rsi
LL16ngas_3:  
    add          $(16), %rsi
    jl           LL8gas_3
    lddqu        (%rdi), %xmm5
    movdqa       %xmm5, %xmm7
    pand         CONST_0F(%rip), %xmm5
    psrlw        $(8), %xmm7
    paddw        %xmm5, %xmm7
    paddw        CONST_x1(%rip), %xmm7
    psrlw        $(1), %xmm7
    packuswb     %xmm7, %xmm7
    movq         %xmm7, (%rdx)
    add          $(16), %rdi
    add          $(8), %rdx
    sub          $(16), %rsi
LL8gas_3:  
    add          $(8), %rsi
    jl           LL4gas_3
    movq         (%rdi), %xmm4
    movdqa       %xmm4, %xmm5
    psrlw        $(8), %xmm4
    pand         CONST_0F(%rip), %xmm5
    paddw        %xmm5, %xmm4
    paddw        CONST_x1(%rip), %xmm4
    psrlw        $(1), %xmm4
    packuswb     %xmm4, %xmm4
    movd         %xmm4, (%rdx)
    add          $(8), %rdi
    add          $(4), %rdx
    sub          $(8), %rsi
LL4gas_3:  
    add          $(4), %rsi
    jl           LL2gas_3
    movd         (%rdi), %xmm6
    movdqa       %xmm6, %xmm7
    psrlw        $(8), %xmm6
    pand         CONST_0F(%rip), %xmm7
    paddw        %xmm7, %xmm6
    paddw        CONST_x1(%rip), %xmm6
    psrlw        $(1), %xmm6
    packuswb     %xmm6, %xmm6
    movd         %xmm6, %eax
    movw         %ax, (%rdx)
    add          $(4), %rdi
    add          $(2), %rdx
    sub          $(4), %rsi
LL2gas_3:  
    add          $(2), %rsi
    jl           Lexitgas_3
    movzbq       (%rdi), %rax
    movzbq       (1)(%rdi), %rbx
    add          %rbx, %rax
    shr          $(1), %rax
    movb         %al, (%rdx)
    jmp          Lexitgas_3
LRunOgas_3:  
    test         $(15), %rdi
    jnz          LNAOgas_3
    sub          $(64), %rsi
    jl           LL32Ogas_3
LL64Ogas_3:  
    movdqa       (%rdi), %xmm0
    movdqa       (16)(%rdi), %xmm2
    movdqa       (32)(%rdi), %xmm4
    movdqa       (48)(%rdi), %xmm6
    movdqa       %xmm0, %xmm1
    movdqa       %xmm2, %xmm3
    movdqa       %xmm4, %xmm5
    movdqa       %xmm6, %xmm7
    psrlw        $(8), %xmm0
    psrlw        $(8), %xmm2
    psrlw        $(8), %xmm4
    psrlw        $(8), %xmm6
    pand         CONST_0F(%rip), %xmm1
    pand         CONST_0F(%rip), %xmm3
    pand         CONST_0F(%rip), %xmm5
    pand         CONST_0F(%rip), %xmm7
    paddw        %xmm1, %xmm0
    paddw        %xmm3, %xmm2
    paddw        %xmm5, %xmm4
    paddw        %xmm7, %xmm6
    paddw        CONST_1x(%rip), %xmm0
    paddw        CONST_1x(%rip), %xmm2
    paddw        CONST_1x(%rip), %xmm4
    paddw        CONST_1x(%rip), %xmm6
    psrlw        $(1), %xmm0
    psrlw        $(1), %xmm2
    psrlw        $(1), %xmm4
    psrlw        $(1), %xmm6
    packuswb     %xmm2, %xmm0
    packuswb     %xmm6, %xmm4
    movdqa       %xmm0, (%rdx)
    movdqa       %xmm4, (16)(%rdx)
    add          $(64), %rdi
    add          $(32), %rdx
    sub          $(64), %rsi
    jge          LL64Ogas_3
LL32Ogas_3:  
    add          $(32), %rsi
    jl           LL16Ogas_3
    movdqa       (%rdi), %xmm0
    movdqa       (16)(%rdi), %xmm2
    movdqa       %xmm0, %xmm1
    movdqa       %xmm2, %xmm3
    psrlw        $(8), %xmm0
    psrlw        $(8), %xmm2
    pand         CONST_0F(%rip), %xmm1
    pand         CONST_0F(%rip), %xmm3
    paddw        %xmm1, %xmm0
    paddw        %xmm3, %xmm2
    paddw        CONST_1x(%rip), %xmm0
    paddw        CONST_1x(%rip), %xmm2
    psrlw        $(1), %xmm0
    psrlw        $(1), %xmm2
    packuswb     %xmm2, %xmm0
    movdqa       %xmm0, (%rdx)
    add          $(32), %rdi
    add          $(16), %rdx
    sub          $(32), %rsi
LL16Ogas_3:  
    add          $(16), %rsi
    jl           LL8Ogas_3
    movdqa       (%rdi), %xmm5
    movdqa       %xmm5, %xmm7
    pand         CONST_0F(%rip), %xmm5
    psrlw        $(8), %xmm7
    paddw        %xmm5, %xmm7
    paddw        CONST_1x(%rip), %xmm7
    psrlw        $(1), %xmm7
    packuswb     %xmm7, %xmm7
    movq         %xmm7, (%rdx)
    add          $(16), %rdi
    add          $(8), %rdx
    sub          $(16), %rsi
    jmp          LL8Ogas_3
LNAOgas_3:  
    sub          $(64), %rsi
    jl           LL32nOgas_3
LL64nOgas_3:  
    lddqu        (%rdi), %xmm0
    lddqu        (16)(%rdi), %xmm2
    lddqu        (32)(%rdi), %xmm4
    lddqu        (48)(%rdi), %xmm6
    movdqa       %xmm0, %xmm1
    movdqa       %xmm2, %xmm3
    movdqa       %xmm4, %xmm5
    movdqa       %xmm6, %xmm7
    psrlw        $(8), %xmm0
    psrlw        $(8), %xmm2
    psrlw        $(8), %xmm4
    psrlw        $(8), %xmm6
    pand         CONST_0F(%rip), %xmm1
    pand         CONST_0F(%rip), %xmm3
    pand         CONST_0F(%rip), %xmm5
    pand         CONST_0F(%rip), %xmm7
    paddw        %xmm1, %xmm0
    paddw        %xmm3, %xmm2
    paddw        %xmm5, %xmm4
    paddw        %xmm7, %xmm6
    paddw        CONST_1x(%rip), %xmm0
    paddw        CONST_1x(%rip), %xmm2
    paddw        CONST_1x(%rip), %xmm4
    paddw        CONST_1x(%rip), %xmm6
    psrlw        $(1), %xmm0
    psrlw        $(1), %xmm2
    psrlw        $(1), %xmm4
    psrlw        $(1), %xmm6
    packuswb     %xmm2, %xmm0
    packuswb     %xmm6, %xmm4
    movdqa       %xmm0, (%rdx)
    movdqa       %xmm4, (16)(%rdx)
    add          $(64), %rdi
    add          $(32), %rdx
    sub          $(64), %rsi
    jge          LL64nOgas_3
LL32nOgas_3:  
    add          $(32), %rsi
    jl           LL16nOgas_3
    lddqu        (%rdi), %xmm0
    lddqu        (16)(%rdi), %xmm2
    movdqa       %xmm0, %xmm1
    movdqa       %xmm2, %xmm3
    psrlw        $(8), %xmm0
    psrlw        $(8), %xmm2
    pand         CONST_0F(%rip), %xmm1
    pand         CONST_0F(%rip), %xmm3
    paddw        %xmm1, %xmm0
    paddw        %xmm3, %xmm2
    paddw        CONST_1x(%rip), %xmm0
    paddw        CONST_1x(%rip), %xmm2
    psrlw        $(1), %xmm0
    psrlw        $(1), %xmm2
    packuswb     %xmm2, %xmm0
    movdqa       %xmm0, (%rdx)
    add          $(32), %rdi
    add          $(16), %rdx
    sub          $(32), %rsi
LL16nOgas_3:  
    add          $(16), %rsi
    jl           LL8Ogas_3
    lddqu        (%rdi), %xmm5
    movdqa       %xmm5, %xmm7
    pand         CONST_0F(%rip), %xmm5
    psrlw        $(8), %xmm7
    paddw        %xmm5, %xmm7
    paddw        CONST_1x(%rip), %xmm7
    psrlw        $(1), %xmm7
    packuswb     %xmm7, %xmm7
    movq         %xmm7, (%rdx)
    add          $(16), %rdi
    add          $(8), %rdx
    sub          $(16), %rsi
LL8Ogas_3:  
    add          $(8), %rsi
    jl           LL4Ogas_3
    movq         (%rdi), %xmm4
    movdqa       %xmm4, %xmm5
    psrlw        $(8), %xmm4
    pand         CONST_0F(%rip), %xmm5
    paddw        %xmm5, %xmm4
    paddw        CONST_1x(%rip), %xmm4
    psrlw        $(1), %xmm4
    packuswb     %xmm4, %xmm4
    movd         %xmm4, (%rdx)
    add          $(8), %rdi
    add          $(4), %rdx
    sub          $(8), %rsi
LL4Ogas_3:  
    add          $(4), %rsi
    jl           LL2Ogas_3
    movd         (%rdi), %xmm6
    movdqa       %xmm6, %xmm7
    psrlw        $(8), %xmm6
    pand         CONST_0F(%rip), %xmm7
    paddw        %xmm7, %xmm6
    paddw        CONST_1x(%rip), %xmm6
    psrlw        $(1), %xmm6
    packuswb     %xmm6, %xmm6
    movd         %xmm6, %eax
    movw         %ax, (%rdx)
    add          $(4), %rdi
    add          $(2), %rdx
    sub          $(4), %rsi
LL2Ogas_3:  
    add          $(2), %rsi
    jl           Lexitgas_3
    movzbq       (%rdi), %rax
    movzbq       (1)(%rdi), %rbx
    lea          (1)(%rbx,%rax), %rax
    shr          $(1), %rax
    movb         %al, (%rdx)
    jmp          Lexitgas_3
Lend7gas_3:  
    movzbq       (%rdi), %rax
    movzbq       (1)(%rdi), %rbx
    add          %rbx, %rax
    shr          $(1), %rax
    movb         %al, (%rdx)
    add          $(2), %rdi
    add          $(1), %rdx
Lend6gas_3:  
    movzbq       (%rdi), %rax
    movzbq       (1)(%rdi), %rbx
    lea          (1)(%rbx,%rax), %rax
    shr          $(1), %rax
    movb         %al, (%rdx)
    add          $(2), %rdi
    add          $(1), %rdx
Lend5gas_3:  
    movzbq       (%rdi), %rax
    movzbq       (1)(%rdi), %rbx
    add          %rbx, %rax
    shr          $(1), %rax
    movb         %al, (%rdx)
    add          $(2), %rdi
    add          $(1), %rdx
Lend4gas_3:  
    movzbq       (%rdi), %rax
    movzbq       (1)(%rdi), %rbx
    lea          (1)(%rbx,%rax), %rax
    shr          $(1), %rax
    movb         %al, (%rdx)
    add          $(2), %rdi
    add          $(1), %rdx
Lend3gas_3:  
    movzbq       (%rdi), %rax
    movzbq       (1)(%rdi), %rbx
    add          %rbx, %rax
    shr          $(1), %rax
    movb         %al, (%rdx)
    add          $(2), %rdi
    add          $(1), %rdx
Lend2gas_3:  
    movzbq       (%rdi), %rax
    movzbq       (1)(%rdi), %rbx
    lea          (1)(%rbx,%rax), %rax
    shr          $(1), %rax
    movb         %al, (%rdx)
    add          $(2), %rdi
    add          $(1), %rdx
Lend1gas_3:  
    movzbq       (%rdi), %rax
    movzbq       (1)(%rdi), %rbx
    add          %rbx, %rax
    shr          $(1), %rax
    movb         %al, (%rdx)
Lexitgas_3:  
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
.p2align 4, 0x90
 
.globl mfxownpj_SampleDownRowH2V2_Box_JPEG_8u_C1

 
mfxownpj_SampleDownRowH2V2_Box_JPEG_8u_C1:
 
 
    push         %rbp
 
 
    push         %rbx
 
 
    mov          %edx, %edx
 
 
    mov          %rcx, %rbp
    and          $(15), %rbp
    jz           LRungas_4
    sub          $(16), %rbp
    neg          %rbp
    sub          %rbp, %rdx
    cmp          %rbp, %rdx
    jle          LRungas_4
    sub          %rbp, %rdx
    test         $(1), %rbp
    je           LL2ngas_4
    movzbq       (%rdi), %rax
    movzbq       (1)(%rdi), %rbx
    add          %rbx, %rax
    movzbq       (%rsi), %r8
    movzbq       (1)(%rsi), %rbx
    add          %r8, %rbx
    lea          (1)(%rbx,%rax), %rax
    shr          $(2), %rax
    movb         %al, (%rcx)
    add          $(2), %rdi
    add          $(2), %rsi
    add          $(1), %rcx
    sub          $(1), %rbp
    jz           LRunOgas_4
LL2nOgas_4:  
    movzbq       (%rdi), %rax
    movzbq       (1)(%rdi), %rbx
    add          %rbx, %rax
    movzbq       (%rsi), %r8
    movzbq       (1)(%rsi), %rbx
    add          %r8, %rbx
    lea          (2)(%rbx,%rax), %rax
    shr          $(2), %rax
    movb         %al, (%rcx)
    movzbq       (2)(%rdi), %rax
    movzbq       (3)(%rdi), %rbx
    add          %rbx, %rax
    movzbq       (2)(%rsi), %r8
    movzbq       (3)(%rsi), %rbx
    add          %r8, %rbx
    lea          (1)(%rbx,%rax), %rax
    shr          $(2), %rax
    movb         %al, (1)(%rcx)
    add          $(4), %rdi
    add          $(4), %rsi
    add          $(2), %rcx
    sub          $(2), %rbp
    jnz          LL2nOgas_4
    jmp          LRunOgas_4
LL2ngas_4:  
    movzbq       (%rdi), %rax
    movzbq       (1)(%rdi), %rbx
    add          %rbx, %rax
    movzbq       (%rsi), %r8
    movzbq       (1)(%rsi), %rbx
    add          %r8, %rbx
    lea          (1)(%rbx,%rax), %rax
    shr          $(2), %rax
    movb         %al, (%rcx)
    movzbq       (2)(%rdi), %rax
    movzbq       (3)(%rdi), %rbx
    add          %rbx, %rax
    movzbq       (2)(%rsi), %r8
    movzbq       (3)(%rsi), %rbx
    add          %r8, %rbx
    lea          (2)(%rbx,%rax), %rax
    shr          $(2), %rax
    movb         %al, (1)(%rcx)
    add          $(4), %rdi
    add          $(4), %rsi
    add          $(2), %rcx
    sub          $(2), %rbp
    jnz          LL2ngas_4
LRungas_4:  
    test         $(15), %rdi
    jnz          LNAgas_4
    test         $(15), %rsi
    jnz          LNAgas_4
    sub          $(32), %rdx
    jl           LL16gas_4
LL32gas_4:  
    movdqa       (%rdi), %xmm0
    movdqa       (16)(%rdi), %xmm2
    movdqa       (%rsi), %xmm4
    movdqa       (16)(%rsi), %xmm6
    movdqa       %xmm0, %xmm1
    movdqa       %xmm2, %xmm3
    movdqa       %xmm4, %xmm5
    movdqa       %xmm6, %xmm7
    psrlw        $(8), %xmm0
    psrlw        $(8), %xmm2
    psrlw        $(8), %xmm4
    psrlw        $(8), %xmm6
    pand         CONST_0F(%rip), %xmm1
    pand         CONST_0F(%rip), %xmm3
    pand         CONST_0F(%rip), %xmm5
    pand         CONST_0F(%rip), %xmm7
    paddw        %xmm1, %xmm0
    paddw        %xmm3, %xmm2
    paddw        %xmm5, %xmm4
    paddw        %xmm7, %xmm6
    paddw        %xmm4, %xmm0
    paddw        %xmm6, %xmm2
    paddw        CONST_x3(%rip), %xmm0
    paddw        CONST_x3(%rip), %xmm2
    psrlw        $(2), %xmm0
    psrlw        $(2), %xmm2
    packuswb     %xmm2, %xmm0
    movdqa       %xmm0, (%rcx)
    add          $(32), %rdi
    add          $(32), %rsi
    add          $(16), %rcx
    sub          $(32), %rdx
    jge          LL32gas_4
LL16gas_4:  
    add          $(16), %rdx
    jl           LL8gas_4
    movdqa       (%rdi), %xmm0
    movdqa       (%rsi), %xmm4
    movdqa       %xmm0, %xmm1
    movdqa       %xmm4, %xmm5
    psrlw        $(8), %xmm0
    psrlw        $(8), %xmm4
    pand         CONST_0F(%rip), %xmm1
    pand         CONST_0F(%rip), %xmm5
    paddw        %xmm1, %xmm0
    paddw        %xmm5, %xmm4
    paddw        %xmm4, %xmm0
    paddw        CONST_x3(%rip), %xmm0
    psrlw        $(2), %xmm0
    packuswb     %xmm0, %xmm0
    movq         %xmm0, (%rcx)
    add          $(16), %rdi
    add          $(16), %rsi
    add          $(8), %rcx
    sub          $(16), %rdx
    jmp          LL8gas_4
LNAgas_4:  
    sub          $(32), %rdx
    jl           LL16ngas_4
LL32ngas_4:  
    lddqu        (%rdi), %xmm0
    lddqu        (16)(%rdi), %xmm2
    movdqu       (%rsi), %xmm4
    movdqu       (16)(%rsi), %xmm6
    movdqa       %xmm0, %xmm1
    movdqa       %xmm2, %xmm3
    movdqa       %xmm4, %xmm5
    movdqa       %xmm6, %xmm7
    psrlw        $(8), %xmm0
    psrlw        $(8), %xmm2
    psrlw        $(8), %xmm4
    psrlw        $(8), %xmm6
    pand         CONST_0F(%rip), %xmm1
    pand         CONST_0F(%rip), %xmm3
    pand         CONST_0F(%rip), %xmm5
    pand         CONST_0F(%rip), %xmm7
    paddw        %xmm1, %xmm0
    paddw        %xmm3, %xmm2
    paddw        %xmm5, %xmm4
    paddw        %xmm7, %xmm6
    paddw        %xmm4, %xmm0
    paddw        %xmm6, %xmm2
    paddw        CONST_x3(%rip), %xmm0
    paddw        CONST_x3(%rip), %xmm2
    psrlw        $(2), %xmm0
    psrlw        $(2), %xmm2
    packuswb     %xmm2, %xmm0
    movdqa       %xmm0, (%rcx)
    add          $(32), %rdi
    add          $(32), %rsi
    add          $(16), %rcx
    sub          $(32), %rdx
    jge          LL32ngas_4
LL16ngas_4:  
    add          $(16), %rdx
    jl           LL8gas_4
    lddqu        (%rdi), %xmm0
    movdqu       (%rsi), %xmm4
    movdqa       %xmm0, %xmm1
    movdqa       %xmm4, %xmm5
    psrlw        $(8), %xmm0
    psrlw        $(8), %xmm4
    pand         CONST_0F(%rip), %xmm1
    pand         CONST_0F(%rip), %xmm5
    paddw        %xmm1, %xmm0
    paddw        %xmm5, %xmm4
    paddw        %xmm4, %xmm0
    paddw        CONST_x3(%rip), %xmm0
    psrlw        $(2), %xmm0
    packuswb     %xmm0, %xmm0
    movq         %xmm0, (%rcx)
    add          $(16), %rdi
    add          $(16), %rsi
    add          $(8), %rcx
    sub          $(16), %rdx
LL8gas_4:  
    add          $(8), %rdx
    jl           LL4gas_4
    movq         (%rdi), %xmm3
    movq         (%rsi), %xmm4
    movdqa       %xmm3, %xmm1
    movdqa       %xmm4, %xmm5
    psrlw        $(8), %xmm3
    psrlw        $(8), %xmm4
    pand         CONST_0F(%rip), %xmm1
    pand         CONST_0F(%rip), %xmm5
    paddw        %xmm1, %xmm3
    paddw        %xmm5, %xmm4
    paddw        %xmm4, %xmm3
    paddw        CONST_x3(%rip), %xmm3
    psrlw        $(2), %xmm3
    packuswb     %xmm3, %xmm3
    movd         %xmm3, (%rcx)
    add          $(8), %rdi
    add          $(8), %rsi
    add          $(4), %rcx
    sub          $(8), %rdx
LL4gas_4:  
    add          $(4), %rdx
    jl           LL2gas_4
    movd         (%rdi), %xmm6
    movd         (%rsi), %xmm4
    movdqa       %xmm6, %xmm1
    movdqa       %xmm4, %xmm5
    psrlw        $(8), %xmm6
    psrlw        $(8), %xmm4
    pand         CONST_0F(%rip), %xmm1
    pand         CONST_0F(%rip), %xmm5
    paddw        %xmm1, %xmm6
    paddw        %xmm5, %xmm4
    paddw        %xmm4, %xmm6
    paddw        CONST_x3(%rip), %xmm6
    psrlw        $(2), %xmm6
    packuswb     %xmm6, %xmm6
    movd         %xmm6, %eax
    movw         %ax, (%rcx)
    add          $(4), %rdi
    add          $(4), %rsi
    add          $(2), %rcx
    sub          $(4), %rdx
LL2gas_4:  
    add          $(2), %rdx
    jl           Lexitgas_4
    movzwq       (%rdi), %rax
    movzwq       (%rsi), %rbx
    movd         %eax, %xmm7
    movd         %ebx, %xmm4
    movdqa       %xmm7, %xmm1
    movdqa       %xmm4, %xmm5
    psrlw        $(8), %xmm7
    psrlw        $(8), %xmm4
    pand         CONST_0F(%rip), %xmm1
    pand         CONST_0F(%rip), %xmm5
    paddw        %xmm1, %xmm7
    paddw        %xmm5, %xmm4
    paddw        %xmm4, %xmm7
    paddw        CONST_x3(%rip), %xmm7
    psrlw        $(2), %xmm7
    packuswb     %xmm7, %xmm7
    movd         %xmm7, %eax
    movb         %al, (%rcx)
    jmp          Lexitgas_4
LRunOgas_4:  
    test         $(15), %rdi
    jnz          LNAOgas_4
    test         $(15), %rsi
    jnz          LNAOgas_4
    sub          $(32), %rdx
    jl           LL16Ogas_4
LL32Ogas_4:  
    movdqa       (%rdi), %xmm0
    movdqa       (16)(%rdi), %xmm2
    movdqa       (%rsi), %xmm4
    movdqa       (16)(%rsi), %xmm6
    movdqa       %xmm0, %xmm1
    movdqa       %xmm2, %xmm3
    movdqa       %xmm4, %xmm5
    movdqa       %xmm6, %xmm7
    psrlw        $(8), %xmm0
    psrlw        $(8), %xmm2
    psrlw        $(8), %xmm4
    psrlw        $(8), %xmm6
    pand         CONST_0F(%rip), %xmm1
    pand         CONST_0F(%rip), %xmm3
    pand         CONST_0F(%rip), %xmm5
    pand         CONST_0F(%rip), %xmm7
    paddw        %xmm1, %xmm0
    paddw        %xmm3, %xmm2
    paddw        %xmm5, %xmm4
    paddw        %xmm7, %xmm6
    paddw        %xmm4, %xmm0
    paddw        %xmm6, %xmm2
    paddw        CONST_3x(%rip), %xmm0
    paddw        CONST_3x(%rip), %xmm2
    psrlw        $(2), %xmm0
    psrlw        $(2), %xmm2
    packuswb     %xmm2, %xmm0
    movdqa       %xmm0, (%rcx)
    add          $(32), %rdi
    add          $(32), %rsi
    add          $(16), %rcx
    sub          $(32), %rdx
    jge          LL32Ogas_4
LL16Ogas_4:  
    add          $(16), %rdx
    jl           LL8Ogas_4
    movdqa       (%rdi), %xmm0
    movdqa       (%rsi), %xmm4
    movdqa       %xmm0, %xmm1
    movdqa       %xmm4, %xmm5
    psrlw        $(8), %xmm0
    psrlw        $(8), %xmm4
    pand         CONST_0F(%rip), %xmm1
    pand         CONST_0F(%rip), %xmm5
    paddw        %xmm1, %xmm0
    paddw        %xmm5, %xmm4
    paddw        %xmm4, %xmm0
    paddw        CONST_3x(%rip), %xmm0
    psrlw        $(2), %xmm0
    packuswb     %xmm0, %xmm0
    movq         %xmm0, (%rcx)
    add          $(16), %rdi
    add          $(16), %rsi
    add          $(8), %rcx
    sub          $(16), %rdx
    jmp          LL8Ogas_4
LNAOgas_4:  
    sub          $(32), %rdx
    jl           LL16nOgas_4
LL32nOgas_4:  
    lddqu        (%rdi), %xmm0
    lddqu        (16)(%rdi), %xmm2
    movdqu       (%rsi), %xmm4
    movdqu       (16)(%rsi), %xmm6
    movdqa       %xmm0, %xmm1
    movdqa       %xmm2, %xmm3
    movdqa       %xmm4, %xmm5
    movdqa       %xmm6, %xmm7
    psrlw        $(8), %xmm0
    psrlw        $(8), %xmm2
    psrlw        $(8), %xmm4
    psrlw        $(8), %xmm6
    pand         CONST_0F(%rip), %xmm1
    pand         CONST_0F(%rip), %xmm3
    pand         CONST_0F(%rip), %xmm5
    pand         CONST_0F(%rip), %xmm7
    paddw        %xmm1, %xmm0
    paddw        %xmm3, %xmm2
    paddw        %xmm5, %xmm4
    paddw        %xmm7, %xmm6
    paddw        %xmm4, %xmm0
    paddw        %xmm6, %xmm2
    paddw        CONST_3x(%rip), %xmm0
    paddw        CONST_3x(%rip), %xmm2
    psrlw        $(2), %xmm0
    psrlw        $(2), %xmm2
    packuswb     %xmm2, %xmm0
    movdqa       %xmm0, (%rcx)
    add          $(32), %rdi
    add          $(32), %rsi
    add          $(16), %rcx
    sub          $(32), %rdx
    jge          LL32nOgas_4
LL16nOgas_4:  
    add          $(16), %rdx
    jl           LL8Ogas_4
    lddqu        (%rdi), %xmm0
    movdqu       (%rsi), %xmm4
    movdqa       %xmm0, %xmm1
    movdqa       %xmm4, %xmm5
    psrlw        $(8), %xmm0
    psrlw        $(8), %xmm4
    pand         CONST_0F(%rip), %xmm1
    pand         CONST_0F(%rip), %xmm5
    paddw        %xmm1, %xmm0
    paddw        %xmm5, %xmm4
    paddw        %xmm4, %xmm0
    paddw        CONST_3x(%rip), %xmm0
    psrlw        $(2), %xmm0
    packuswb     %xmm0, %xmm0
    movq         %xmm0, (%rcx)
    add          $(16), %rdi
    add          $(16), %rsi
    add          $(8), %rcx
    sub          $(16), %rdx
LL8Ogas_4:  
    add          $(8), %rdx
    jl           LL4Ogas_4
    movq         (%rdi), %xmm3
    movq         (%rsi), %xmm4
    movdqa       %xmm3, %xmm1
    movdqa       %xmm4, %xmm5
    psrlw        $(8), %xmm3
    psrlw        $(8), %xmm4
    pand         CONST_0F(%rip), %xmm1
    pand         CONST_0F(%rip), %xmm5
    paddw        %xmm1, %xmm3
    paddw        %xmm5, %xmm4
    paddw        %xmm4, %xmm3
    paddw        CONST_3x(%rip), %xmm3
    psrlw        $(2), %xmm3
    packuswb     %xmm3, %xmm3
    movd         %xmm3, (%rcx)
    add          $(8), %rdi
    add          $(8), %rsi
    add          $(4), %rcx
    sub          $(8), %rdx
LL4Ogas_4:  
    add          $(4), %rdx
    jl           LL2Ogas_4
    movd         (%rdi), %xmm6
    movd         (%rsi), %xmm4
    movdqa       %xmm6, %xmm1
    movdqa       %xmm4, %xmm5
    psrlw        $(8), %xmm6
    psrlw        $(8), %xmm4
    pand         CONST_0F(%rip), %xmm1
    pand         CONST_0F(%rip), %xmm5
    paddw        %xmm1, %xmm6
    paddw        %xmm5, %xmm4
    paddw        %xmm4, %xmm6
    paddw        CONST_3x(%rip), %xmm6
    psrlw        $(2), %xmm6
    packuswb     %xmm6, %xmm6
    movd         %xmm6, %eax
    movw         %ax, (%rcx)
    add          $(4), %rdi
    add          $(4), %rsi
    add          $(2), %rcx
    sub          $(4), %rdx
LL2Ogas_4:  
    add          $(2), %rdx
    jl           Lexitgas_4
    movzwq       (%rdi), %rax
    movzwq       (%rsi), %rbx
    movd         %eax, %xmm7
    movd         %ebx, %xmm4
    movdqa       %xmm7, %xmm1
    movdqa       %xmm4, %xmm5
    psrlw        $(8), %xmm7
    psrlw        $(8), %xmm4
    pand         CONST_0F(%rip), %xmm1
    pand         CONST_0F(%rip), %xmm5
    paddw        %xmm1, %xmm7
    paddw        %xmm5, %xmm4
    paddw        %xmm4, %xmm7
    paddw        CONST_3x(%rip), %xmm7
    psrlw        $(2), %xmm7
    packuswb     %xmm7, %xmm7
    movd         %xmm7, %eax
    movb         %al, (%rcx)
Lexitgas_4:  
 
 
    pop          %rbx
 
 
    pop          %rbp
 
    ret
 
 
