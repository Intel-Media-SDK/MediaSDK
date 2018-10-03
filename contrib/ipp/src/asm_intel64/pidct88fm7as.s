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
 
 
cos_4_16_:
.single  0.707106781187, 0.707106781187, 0.707106781187, 0.707106781187  
 
tg_2_16_:
.single  0.414213562373, 0.414213562373, 0.414213562373, 0.414213562373  
 
cos4cos4p_:
.single  1.414213562373, 1.414213562373, 1.414213562373, 1.414213562373  
 
cos6cos6p_:
.single  0.765366864730, 0.765366864730, 0.765366864730, 0.765366864730  
 
cos2cos6m_:
.single  0.541196100146, 0.541196100146, 0.541196100146, 0.541196100146  
 
cos0cos2d_:
.single  1.082392200292, 1.082392200292, 1.082392200292, 1.082392200292  
 
 
tab_norm_frw:
.single  0.125000000000, 0.083260002914, 0.095670858091, 0.098211869798  
 

.single  0.125000000000, 0.146984450302, 0.230969883128, 0.418576300766  
 

.single  0.083260002914, 0.055457824682, 0.063724447388, 0.065416964525  
 

.single  0.083260002914, 0.097903406084, 0.153844425139, 0.278805312173  
 

.single  0.095670858091, 0.063724447388, 0.073223304703, 0.075168110867  
 

.single  0.095670858091, 0.112497027892, 0.176776695297, 0.320364430968  
 

.single  0.098211869798, 0.065416964525, 0.075168110867, 0.077164570954  
 

.single  0.098211869798, 0.115484941564, 0.181471872713, 0.328873289212  
 

.single  0.125000000000, 0.083260002914, 0.095670858091, 0.098211869798  
 

.single  0.125000000000, 0.146984450302, 0.230969883128, 0.418576300766  
 

.single  0.146984450302, 0.097903406084, 0.112497027892, 0.115484941564  
 

.single  0.146984450302, 0.172835429046, 0.271591850464, 0.492193659822  
 

.single  0.230969883128, 0.153844425139, 0.176776695297, 0.181471872713  
 

.single  0.230969883128, 0.271591850464, 0.426776695297, 0.773428154144  
 

.single  0.418576300766, 0.278805312173, 0.320364430968, 0.328873289212  
 

.single  0.418576300766, 0.492193659822, 0.773428154144, 1.401648956504  
 
 
tab_norm_inv:
.single  0.125000000000, 0.320364430968, 0.163320370610, 0.271591850464  
 

.single  0.125000000000, 0.181471872713, 0.067649512518, 0.063724447388  
 

.single  0.320364430968, 0.821066949034, 0.418576300766, 0.696066949034  
 

.single  0.320364430968, 0.465097065906, 0.173379980665, 0.163320370610  
 

.single  0.163320370610, 0.418576300766, 0.213388347648, 0.354851853378  
 

.single  0.163320370610, 0.237104428053, 0.088388347648, 0.083260002914  
 

.single  0.271591850464, 0.696066949034, 0.354851853378, 0.590097065906  
 

.single  0.271591850464, 0.394290253737, 0.146984450302, 0.138456324687  
 

.single  0.125000000000, 0.320364430968, 0.163320370610, 0.271591850464  
 

.single  0.125000000000, 0.181471872713, 0.067649512518, 0.063724447388  
 

.single  0.181471872713, 0.465097065906, 0.237104428053, 0.394290253737  
 

.single  0.181471872713, 0.263456324687, 0.098211869798, 0.092513558441  
 

.single  0.067649512518, 0.173379980665, 0.088388347648, 0.146984450302  
 

.single  0.067649512518, 0.098211869798, 0.036611652352, 0.034487422410  
 

.single  0.063724447388, 0.163320370610, 0.083260002914, 0.138456324687  
 

.single  0.063724447388, 0.092513558441, 0.034487422410, 0.032486441559  
 
 
.text
 
 
.p2align 4, 0x90
 
 
.globl mfxdct_8x8_fwd_32f

 
mfxdct_8x8_fwd_32f:
 
 
    mov          %rdi, %r10
    mov          %rsi, %r11
    lea          tab_norm_frw(%rip), %rax
 
    test         $(15), %r11
    jz           LALIGNED_16gas_1
 
LMISALIGNED_16gas_1:  
    mov          %rsp, %rcx
    sub          $(256), %rsp
    and          $(-16), %rsp
    mov          %rsp, %r11
 
LALIGNED_16gas_1:  
 
 
    movsd        (%r10), %xmm0
    movsd        (32)(%r10), %xmm5
    movlhps      %xmm5, %xmm0
    movsd        (64)(%r10), %xmm7
    movsd        (96)(%r10), %xmm5
    movlhps      %xmm5, %xmm7
    movsd        (8)(%r10), %xmm2
    movsd        (40)(%r10), %xmm5
    movlhps      %xmm5, %xmm2
 
    movaps       %xmm0, %xmm1
    shufps       $(136), %xmm7, %xmm0
    shufps       $(221), %xmm7, %xmm1
 
    movsd        (72)(%r10), %xmm7
    movsd        (104)(%r10), %xmm5
    movlhps      %xmm5, %xmm7
    movaps       %xmm2, %xmm3
    shufps       $(136), %xmm7, %xmm2
 
 
    movsd        (16)(%r10), %xmm4
    movsd        (48)(%r10), %xmm5
    movlhps      %xmm5, %xmm4
    shufps       $(221), %xmm7, %xmm3
    movsd        (80)(%r10), %xmm7
    movsd        (112)(%r10), %xmm5
    movlhps      %xmm5, %xmm7
    movsd        (24)(%r10), %xmm6
    movsd        (56)(%r10), %xmm5
    movlhps      %xmm5, %xmm6
 
    movaps       %xmm4, %xmm5
    shufps       $(136), %xmm7, %xmm4
    shufps       $(221), %xmm7, %xmm5
 
    movaps       %xmm4, (16)(%r11)
 
    movsd        (88)(%r10), %xmm4
    movsd        (120)(%r10), %xmm7
    movlhps      %xmm7, %xmm4
    movaps       %xmm6, %xmm7
    shufps       $(136), %xmm4, %xmm6
    shufps       $(221), %xmm4, %xmm7
 
 
    movaps       %xmm6, %xmm4
    addps        %xmm1, %xmm6
    subps        %xmm4, %xmm1
    movaps       %xmm5, %xmm4
    addps        %xmm2, %xmm5
    subps        %xmm4, %xmm2
    movaps       %xmm7, %xmm4
    addps        %xmm0, %xmm7
    subps        %xmm4, %xmm0
    movaps       %xmm3, (96)(%r11)
    addps        (16)(%r11), %xmm3
 
 
    movaps       %xmm6, %xmm4
    subps        %xmm5, %xmm6
    addps        %xmm4, %xmm5
    movaps       %xmm7, %xmm4
    subps        %xmm3, %xmm7
    addps        %xmm7, %xmm6
 
    addps        %xmm4, %xmm3
 
 
    movaps       %xmm5, %xmm4
    mulps        cos_4_16_(%rip), %xmm6
    addps        %xmm3, %xmm4
    subps        %xmm5, %xmm3
    movaps       (96)(%r11), %xmm5
    subps        (16)(%r11), %xmm5
    movaps       %xmm4, (%r11)
    movaps       %xmm7, %xmm4
    movaps       %xmm3, (16)(%r11)
    subps        %xmm6, %xmm4
    movaps       %xmm4, (80)(%r11)
 
 
    addps        %xmm2, %xmm5
    addps        %xmm1, %xmm2
 
    addps        %xmm0, %xmm1
 
 
    movaps       tg_2_16_(%rip), %xmm3
    mulps        cos6cos6p_(%rip), %xmm2
    mulps        %xmm5, %xmm3
    mulps        cos0cos2d_(%rip), %xmm0
    addps        %xmm1, %xmm3
    mulps        tg_2_16_(%rip), %xmm1
    addps        %xmm7, %xmm6
    movaps       %xmm6, (64)(%r11)
    subps        %xmm1, %xmm5
 
 
    movaps       %xmm2, %xmm4
    addps        %xmm0, %xmm2
    subps        %xmm4, %xmm0
 
 
    movaps       %xmm3, %xmm4
    addps        %xmm2, %xmm3
    movaps       %xmm3, (32)(%r11)
    movaps       %xmm0, %xmm7
    subps        %xmm5, %xmm0
    movaps       %xmm0, (96)(%r11)
    subps        %xmm4, %xmm2
    movaps       %xmm2, (112)(%r11)
    addps        %xmm7, %xmm5
    movaps       %xmm5, (48)(%r11)
 
 
    movsd        (128)(%r10), %xmm0
    movsd        (160)(%r10), %xmm5
    movlhps      %xmm5, %xmm0
    movsd        (192)(%r10), %xmm7
    movsd        (224)(%r10), %xmm5
    movlhps      %xmm5, %xmm7
    movsd        (136)(%r10), %xmm2
    movsd        (168)(%r10), %xmm5
    movlhps      %xmm5, %xmm2
 
    movaps       %xmm0, %xmm1
    shufps       $(136), %xmm7, %xmm0
    shufps       $(221), %xmm7, %xmm1
 
    movsd        (200)(%r10), %xmm7
    movsd        (232)(%r10), %xmm5
    movlhps      %xmm5, %xmm7
    movaps       %xmm2, %xmm3
    shufps       $(136), %xmm7, %xmm2
 
 
    movsd        (144)(%r10), %xmm4
    movsd        (176)(%r10), %xmm5
    movlhps      %xmm5, %xmm4
    shufps       $(221), %xmm7, %xmm3
    movsd        (208)(%r10), %xmm7
    movsd        (240)(%r10), %xmm5
    movlhps      %xmm5, %xmm7
    movsd        (152)(%r10), %xmm6
    movsd        (184)(%r10), %xmm5
    movlhps      %xmm5, %xmm6
 
    movaps       %xmm4, %xmm5
    shufps       $(136), %xmm7, %xmm4
    shufps       $(221), %xmm7, %xmm5
 
    movaps       %xmm4, (144)(%r11)
 
    movsd        (216)(%r10), %xmm4
    movsd        (248)(%r10), %xmm7
    movlhps      %xmm7, %xmm4
    movaps       %xmm6, %xmm7
    shufps       $(136), %xmm4, %xmm6
    shufps       $(221), %xmm4, %xmm7
 
 
    movaps       %xmm6, %xmm4
    addps        %xmm1, %xmm6
    subps        %xmm4, %xmm1
    movaps       %xmm5, %xmm4
    addps        %xmm2, %xmm5
    subps        %xmm4, %xmm2
    movaps       %xmm7, %xmm4
    addps        %xmm0, %xmm7
    subps        %xmm4, %xmm0
    movaps       %xmm3, (224)(%r11)
    addps        (144)(%r11), %xmm3
 
 
    movaps       %xmm6, %xmm4
    subps        %xmm5, %xmm6
    addps        %xmm4, %xmm5
    movaps       %xmm7, %xmm4
    subps        %xmm3, %xmm7
    addps        %xmm7, %xmm6
 
    addps        %xmm4, %xmm3
 
 
    movaps       %xmm5, %xmm4
    mulps        cos_4_16_(%rip), %xmm6
    addps        %xmm3, %xmm4
    subps        %xmm5, %xmm3
    movaps       (224)(%r11), %xmm5
    subps        (144)(%r11), %xmm5
    movaps       %xmm4, (128)(%r11)
    movaps       %xmm7, %xmm4
    movaps       %xmm3, (144)(%r11)
    subps        %xmm6, %xmm4
    movaps       %xmm4, (208)(%r11)
 
 
    addps        %xmm2, %xmm5
    addps        %xmm1, %xmm2
 
    addps        %xmm0, %xmm1
 
 
    movaps       tg_2_16_(%rip), %xmm3
    mulps        cos6cos6p_(%rip), %xmm2
    mulps        %xmm5, %xmm3
    mulps        cos0cos2d_(%rip), %xmm0
    addps        %xmm1, %xmm3
    mulps        tg_2_16_(%rip), %xmm1
    addps        %xmm7, %xmm6
    movaps       %xmm6, (192)(%r11)
    subps        %xmm1, %xmm5
 
 
    movaps       %xmm2, %xmm4
    addps        %xmm0, %xmm2
    subps        %xmm4, %xmm0
 
 
    movaps       %xmm3, %xmm4
    addps        %xmm2, %xmm3
    movaps       %xmm3, (160)(%r11)
    movaps       %xmm0, %xmm7
    subps        %xmm5, %xmm0
    movaps       %xmm0, (224)(%r11)
    subps        %xmm4, %xmm2
    movaps       %xmm2, (240)(%r11)
    addps        %xmm7, %xmm5
    movaps       %xmm5, (176)(%r11)
 
 
    movsd        (%r11), %xmm0
    movsd        (32)(%r11), %xmm5
    movlhps      %xmm5, %xmm0
    movsd        (64)(%r11), %xmm7
    movsd        (96)(%r11), %xmm5
    movlhps      %xmm5, %xmm7
    movsd        (8)(%r11), %xmm2
    movsd        (40)(%r11), %xmm5
    movlhps      %xmm5, %xmm2
 
    movaps       %xmm0, %xmm1
    shufps       $(136), %xmm7, %xmm0
    shufps       $(221), %xmm7, %xmm1
 
    movsd        (72)(%r11), %xmm7
    movsd        (104)(%r11), %xmm5
    movlhps      %xmm5, %xmm7
    movaps       %xmm2, %xmm3
    shufps       $(136), %xmm7, %xmm2
 
 
    movsd        (128)(%r11), %xmm4
    movsd        (160)(%r11), %xmm5
    movlhps      %xmm5, %xmm4
    shufps       $(221), %xmm7, %xmm3
    movsd        (192)(%r11), %xmm7
    movsd        (224)(%r11), %xmm5
    movlhps      %xmm5, %xmm7
    movsd        (136)(%r11), %xmm6
    movsd        (168)(%r11), %xmm5
    movlhps      %xmm5, %xmm6
 
    movaps       %xmm4, %xmm5
    shufps       $(136), %xmm7, %xmm4
    shufps       $(221), %xmm7, %xmm5
 
    movaps       %xmm4, (128)(%r11)
 
    movsd        (200)(%r11), %xmm4
    movsd        (232)(%r11), %xmm7
    movlhps      %xmm7, %xmm4
    movaps       %xmm6, %xmm7
    shufps       $(136), %xmm4, %xmm6
    shufps       $(221), %xmm4, %xmm7
 
 
    movaps       %xmm6, %xmm4
    addps        %xmm1, %xmm6
    subps        %xmm4, %xmm1
    movaps       %xmm5, %xmm4
    addps        %xmm2, %xmm5
    subps        %xmm4, %xmm2
    movaps       %xmm7, %xmm4
    addps        %xmm0, %xmm7
    subps        %xmm4, %xmm0
    movaps       %xmm3, (96)(%r11)
    addps        (128)(%r11), %xmm3
 
 
    movaps       %xmm6, %xmm4
    subps        %xmm5, %xmm6
    addps        %xmm4, %xmm5
    movaps       %xmm7, %xmm4
    subps        %xmm3, %xmm7
    addps        %xmm7, %xmm6
 
    addps        %xmm4, %xmm3
 
 
    movaps       %xmm5, %xmm4
    mulps        cos_4_16_(%rip), %xmm6
    addps        %xmm3, %xmm4
    mulps        (%rax), %xmm4
    subps        %xmm5, %xmm3
    mulps        (128)(%rax), %xmm3
    movaps       (96)(%r11), %xmm5
    subps        (128)(%r11), %xmm5
    movaps       %xmm4, (%r11)
    movaps       %xmm7, %xmm4
    movaps       %xmm3, (128)(%r11)
    subps        %xmm6, %xmm4
    addps        %xmm7, %xmm6
    mulps        (192)(%rax), %xmm4
    mulps        (64)(%rax), %xmm6
 
 
    addps        %xmm2, %xmm5
    addps        %xmm1, %xmm2
 
    addps        %xmm0, %xmm1
 
 
    movaps       tg_2_16_(%rip), %xmm3
    mulps        cos6cos6p_(%rip), %xmm2
    mulps        cos0cos2d_(%rip), %xmm0
    mulps        %xmm5, %xmm3
    addps        %xmm1, %xmm3
    mulps        tg_2_16_(%rip), %xmm1
 
    movaps       %xmm4, (192)(%r11)
    movaps       %xmm6, (64)(%r11)
 
 
    movaps       %xmm2, %xmm4
    addps        %xmm0, %xmm2
    subps        %xmm4, %xmm0
 
 
    movaps       %xmm3, %xmm4
    addps        %xmm2, %xmm3
    mulps        (32)(%rax), %xmm3
    subps        %xmm1, %xmm5
    movaps       %xmm3, (32)(%r11)
    movaps       %xmm0, %xmm7
    subps        %xmm5, %xmm0
    mulps        (96)(%rax), %xmm0
    subps        %xmm4, %xmm2
    mulps        (224)(%rax), %xmm2
    movaps       %xmm0, (96)(%r11)
    addps        %xmm7, %xmm5
    mulps        (160)(%rax), %xmm5
    movaps       %xmm2, (224)(%r11)
    movaps       %xmm5, (160)(%r11)
 
 
    movsd        (16)(%r11), %xmm0
    movsd        (48)(%r11), %xmm5
    movlhps      %xmm5, %xmm0
    movsd        (80)(%r11), %xmm7
    movsd        (112)(%r11), %xmm5
    movlhps      %xmm5, %xmm7
    movsd        (24)(%r11), %xmm2
    movsd        (56)(%r11), %xmm5
    movlhps      %xmm5, %xmm2
 
    movaps       %xmm0, %xmm1
    shufps       $(136), %xmm7, %xmm0
    shufps       $(221), %xmm7, %xmm1
 
    movsd        (88)(%r11), %xmm7
    movsd        (120)(%r11), %xmm5
    movlhps      %xmm5, %xmm7
    movaps       %xmm2, %xmm3
    shufps       $(136), %xmm7, %xmm2
 
 
    movsd        (144)(%r11), %xmm4
    movsd        (176)(%r11), %xmm5
    movlhps      %xmm5, %xmm4
    shufps       $(221), %xmm7, %xmm3
    movsd        (208)(%r11), %xmm7
    movsd        (240)(%r11), %xmm5
    movlhps      %xmm5, %xmm7
    movsd        (152)(%r11), %xmm6
    movsd        (184)(%r11), %xmm5
    movlhps      %xmm5, %xmm6
 
    movaps       %xmm4, %xmm5
    shufps       $(136), %xmm7, %xmm4
    shufps       $(221), %xmm7, %xmm5
 
    movaps       %xmm4, (144)(%r11)
 
    movsd        (216)(%r11), %xmm4
    movsd        (248)(%r11), %xmm7
    movlhps      %xmm7, %xmm4
    movaps       %xmm6, %xmm7
    shufps       $(136), %xmm4, %xmm6
    shufps       $(221), %xmm4, %xmm7
 
 
    movaps       %xmm6, %xmm4
    addps        %xmm1, %xmm6
    subps        %xmm4, %xmm1
    movaps       %xmm5, %xmm4
    addps        %xmm2, %xmm5
    subps        %xmm4, %xmm2
    movaps       %xmm7, %xmm4
    addps        %xmm0, %xmm7
    subps        %xmm4, %xmm0
    movaps       %xmm3, (112)(%r11)
    addps        (144)(%r11), %xmm3
 
 
    movaps       %xmm6, %xmm4
    subps        %xmm5, %xmm6
    addps        %xmm4, %xmm5
    movaps       %xmm7, %xmm4
    subps        %xmm3, %xmm7
    addps        %xmm7, %xmm6
 
    addps        %xmm4, %xmm3
 
 
    movaps       %xmm5, %xmm4
    mulps        cos_4_16_(%rip), %xmm6
    addps        %xmm3, %xmm4
    mulps        (16)(%rax), %xmm4
    subps        %xmm5, %xmm3
    mulps        (144)(%rax), %xmm3
    movaps       (112)(%r11), %xmm5
    subps        (144)(%r11), %xmm5
    movaps       %xmm4, (16)(%r11)
    movaps       %xmm7, %xmm4
    movaps       %xmm3, (144)(%r11)
    subps        %xmm6, %xmm4
    addps        %xmm7, %xmm6
    mulps        (208)(%rax), %xmm4
    mulps        (80)(%rax), %xmm6
 
 
    addps        %xmm2, %xmm5
    addps        %xmm1, %xmm2
 
    addps        %xmm0, %xmm1
 
 
    movaps       tg_2_16_(%rip), %xmm3
    mulps        cos6cos6p_(%rip), %xmm2
    mulps        cos0cos2d_(%rip), %xmm0
    mulps        %xmm5, %xmm3
    addps        %xmm1, %xmm3
    mulps        tg_2_16_(%rip), %xmm1
 
    movaps       %xmm4, (208)(%r11)
    movaps       %xmm6, (80)(%r11)
 
 
    movaps       %xmm2, %xmm4
    addps        %xmm0, %xmm2
    subps        %xmm4, %xmm0
 
 
    movaps       %xmm3, %xmm4
    addps        %xmm2, %xmm3
    mulps        (48)(%rax), %xmm3
    subps        %xmm1, %xmm5
    movaps       %xmm3, (48)(%r11)
    movaps       %xmm0, %xmm7
    subps        %xmm5, %xmm0
    mulps        (112)(%rax), %xmm0
    subps        %xmm4, %xmm2
    mulps        (240)(%rax), %xmm2
    movaps       %xmm0, (112)(%r11)
    addps        %xmm7, %xmm5
    mulps        (176)(%rax), %xmm5
    movaps       %xmm2, (240)(%r11)
    movaps       %xmm5, (176)(%r11)
 
 
    cmp          %rsp, %r11
    jne          LEXITgas_1
 
    mov          %rsi, %r11
 
 
    mov          $(256), %rax
 
.p2align 4, 0x90
L__0037gas_1:  
    movaps       (-32)(%rsp,%rax), %xmm0
    movlps       %xmm0, (-32)(%r11,%rax)
    movhps       %xmm0, (-24)(%r11,%rax)
    movaps       (-16)(%rsp,%rax), %xmm0
    movlps       %xmm0, (-16)(%r11,%rax)
    movhps       %xmm0, (-8)(%r11,%rax)
    sub          $(32), %rax
    jg           L__0037gas_1
 
    mov          %rcx, %rsp
 
LEXITgas_1:  
 
    ret
 
 
.p2align 4, 0x90
 
 
.globl mfxdct_8x8_inv_32f

 
mfxdct_8x8_inv_32f:
 
 
    mov          %rdi, %r10
    mov          %rsi, %r11
    lea          tab_norm_inv(%rip), %rax
 
    test         $(15), %r11
    jz           LALIGNED_16gas_2
 
LMISALIGNED_16gas_2:  
    mov          %rsp, %rcx
    sub          $(256), %rsp
    and          $(-16), %rsp
    mov          %rsp, %r11
 
LALIGNED_16gas_2:  
 
 
    movsd        (%r10), %xmm0
    movsd        (32)(%r10), %xmm5
    movlhps      %xmm5, %xmm0
    movsd        (64)(%r10), %xmm7
    movsd        (96)(%r10), %xmm5
    movlhps      %xmm5, %xmm7
    movsd        (8)(%r10), %xmm2
    movsd        (40)(%r10), %xmm5
    movlhps      %xmm5, %xmm2
 
    movaps       %xmm0, %xmm1
    shufps       $(136), %xmm7, %xmm0
    shufps       $(221), %xmm7, %xmm1
 
    movsd        (72)(%r10), %xmm7
    movsd        (104)(%r10), %xmm5
    movlhps      %xmm5, %xmm7
    movaps       %xmm2, %xmm3
    shufps       $(136), %xmm7, %xmm2
 
 
    movsd        (16)(%r10), %xmm4
    movsd        (48)(%r10), %xmm5
    movlhps      %xmm5, %xmm4
    shufps       $(221), %xmm7, %xmm3
    movsd        (80)(%r10), %xmm7
    movsd        (112)(%r10), %xmm5
    movlhps      %xmm5, %xmm7
    movsd        (24)(%r10), %xmm6
    movsd        (56)(%r10), %xmm5
    movlhps      %xmm5, %xmm6
 
    movaps       %xmm4, %xmm5
    shufps       $(136), %xmm7, %xmm4
    shufps       $(221), %xmm7, %xmm5
 
    movaps       %xmm4, (16)(%r11)
 
    movsd        (88)(%r10), %xmm4
    movsd        (120)(%r10), %xmm7
    movlhps      %xmm7, %xmm4
    movaps       %xmm6, %xmm7
    shufps       $(136), %xmm4, %xmm6
    shufps       $(221), %xmm4, %xmm7
 
 
    mulps        (96)(%rax), %xmm3
    mulps        (160)(%rax), %xmm5
    mulps        (224)(%rax), %xmm7
    mulps        (32)(%rax), %xmm1
    movaps       %xmm3, %xmm4
    subps        %xmm5, %xmm3
    addps        %xmm4, %xmm5
    movaps       %xmm7, %xmm4
    addps        %xmm1, %xmm7
    subps        %xmm4, %xmm1
 
 
    movaps       %xmm5, %xmm4
    addps        %xmm7, %xmm5
    subps        %xmm4, %xmm7
    movaps       tg_2_16_(%rip), %xmm4
    mulps        %xmm3, %xmm4
    addps        %xmm1, %xmm4
    mulps        tg_2_16_(%rip), %xmm1
    mulps        cos2cos6m_(%rip), %xmm5
    mulps        cos6cos6p_(%rip), %xmm7
    subps        %xmm3, %xmm1
    subps        %xmm5, %xmm4
    subps        %xmm4, %xmm7
 
 
    mulps        (192)(%rax), %xmm6
    mulps        (64)(%rax), %xmm2
    movaps       %xmm0, (%r11)
    addps        (16)(%r11), %xmm0
    mulps        (%rax), %xmm0
    movaps       %xmm6, %xmm3
    addps        %xmm2, %xmm6
    subps        %xmm3, %xmm2
    mulps        cos4cos4p_(%rip), %xmm2
    movaps       %xmm6, %xmm3
    addps        %xmm0, %xmm6
    subps        %xmm3, %xmm0
    subps        %xmm3, %xmm2
 
 
    movaps       %xmm5, %xmm3
    addps        %xmm6, %xmm5
    subps        %xmm3, %xmm6
    movaps       (%r11), %xmm3
    movaps       %xmm5, (%r11)
    subps        (16)(%r11), %xmm3
    subps        %xmm7, %xmm1
    mulps        (%rax), %xmm3
    movaps       %xmm6, (112)(%r11)
 
 
    movaps       %xmm1, %xmm5
    addps        %xmm0, %xmm1
    subps        %xmm5, %xmm0
    movaps       %xmm2, %xmm5
    addps        %xmm3, %xmm2
    movaps       %xmm1, (96)(%r11)
    subps        %xmm5, %xmm3
    movaps       %xmm0, (16)(%r11)
 
 
    movaps       %xmm4, %xmm6
    addps        %xmm2, %xmm4
    movaps       %xmm4, (32)(%r11)
    movaps       %xmm7, %xmm5
    addps        %xmm3, %xmm7
    movaps       %xmm7, (64)(%r11)
    subps        %xmm6, %xmm2
    movaps       %xmm2, (80)(%r11)
    subps        %xmm5, %xmm3
    movaps       %xmm3, (48)(%r11)
 
 
    movsd        (128)(%r10), %xmm0
    movsd        (160)(%r10), %xmm5
    movlhps      %xmm5, %xmm0
    movsd        (192)(%r10), %xmm7
    movsd        (224)(%r10), %xmm5
    movlhps      %xmm5, %xmm7
    movsd        (136)(%r10), %xmm2
    movsd        (168)(%r10), %xmm5
    movlhps      %xmm5, %xmm2
 
    movaps       %xmm0, %xmm1
    shufps       $(136), %xmm7, %xmm0
    shufps       $(221), %xmm7, %xmm1
 
    movsd        (200)(%r10), %xmm7
    movsd        (232)(%r10), %xmm5
    movlhps      %xmm5, %xmm7
    movaps       %xmm2, %xmm3
    shufps       $(136), %xmm7, %xmm2
 
 
    movsd        (144)(%r10), %xmm4
    movsd        (176)(%r10), %xmm5
    movlhps      %xmm5, %xmm4
    shufps       $(221), %xmm7, %xmm3
    movsd        (208)(%r10), %xmm7
    movsd        (240)(%r10), %xmm5
    movlhps      %xmm5, %xmm7
    movsd        (152)(%r10), %xmm6
    movsd        (184)(%r10), %xmm5
    movlhps      %xmm5, %xmm6
 
    movaps       %xmm4, %xmm5
    shufps       $(136), %xmm7, %xmm4
    shufps       $(221), %xmm7, %xmm5
 
    movaps       %xmm4, (144)(%r11)
 
    movsd        (216)(%r10), %xmm4
    movsd        (248)(%r10), %xmm7
    movlhps      %xmm7, %xmm4
    movaps       %xmm6, %xmm7
    shufps       $(136), %xmm4, %xmm6
    shufps       $(221), %xmm4, %xmm7
 
 
    mulps        (112)(%rax), %xmm3
    mulps        (176)(%rax), %xmm5
    mulps        (240)(%rax), %xmm7
    mulps        (48)(%rax), %xmm1
    movaps       %xmm3, %xmm4
    subps        %xmm5, %xmm3
    addps        %xmm4, %xmm5
    movaps       %xmm7, %xmm4
    addps        %xmm1, %xmm7
    subps        %xmm4, %xmm1
 
 
    movaps       %xmm5, %xmm4
    addps        %xmm7, %xmm5
    subps        %xmm4, %xmm7
    movaps       tg_2_16_(%rip), %xmm4
    mulps        %xmm3, %xmm4
    addps        %xmm1, %xmm4
    mulps        tg_2_16_(%rip), %xmm1
    mulps        cos2cos6m_(%rip), %xmm5
    mulps        cos6cos6p_(%rip), %xmm7
    subps        %xmm3, %xmm1
    subps        %xmm5, %xmm4
    subps        %xmm4, %xmm7
 
 
    mulps        (208)(%rax), %xmm6
    mulps        (80)(%rax), %xmm2
    movaps       %xmm0, (128)(%r11)
    addps        (144)(%r11), %xmm0
    mulps        (16)(%rax), %xmm0
    movaps       %xmm6, %xmm3
    addps        %xmm2, %xmm6
    subps        %xmm3, %xmm2
    mulps        cos4cos4p_(%rip), %xmm2
    movaps       %xmm6, %xmm3
    addps        %xmm0, %xmm6
    subps        %xmm3, %xmm0
    subps        %xmm3, %xmm2
 
 
    movaps       %xmm5, %xmm3
    addps        %xmm6, %xmm5
    subps        %xmm3, %xmm6
    movaps       (128)(%r11), %xmm3
    movaps       %xmm5, (128)(%r11)
    subps        (144)(%r11), %xmm3
    subps        %xmm7, %xmm1
    mulps        (16)(%rax), %xmm3
    movaps       %xmm6, (240)(%r11)
 
 
    movaps       %xmm1, %xmm5
    addps        %xmm0, %xmm1
    subps        %xmm5, %xmm0
    movaps       %xmm2, %xmm5
    addps        %xmm3, %xmm2
    movaps       %xmm1, (224)(%r11)
    subps        %xmm5, %xmm3
    movaps       %xmm0, (144)(%r11)
 
 
    movaps       %xmm4, %xmm6
    addps        %xmm2, %xmm4
    movaps       %xmm4, (160)(%r11)
    movaps       %xmm7, %xmm5
    addps        %xmm3, %xmm7
    movaps       %xmm7, (192)(%r11)
    subps        %xmm6, %xmm2
    movaps       %xmm2, (208)(%r11)
    subps        %xmm5, %xmm3
    movaps       %xmm3, (176)(%r11)
 
 
    movsd        (%r11), %xmm0
    movsd        (32)(%r11), %xmm5
    movlhps      %xmm5, %xmm0
    movsd        (64)(%r11), %xmm7
    movsd        (96)(%r11), %xmm5
    movlhps      %xmm5, %xmm7
    movsd        (8)(%r11), %xmm2
    movsd        (40)(%r11), %xmm5
    movlhps      %xmm5, %xmm2
 
    movaps       %xmm0, %xmm1
    shufps       $(136), %xmm7, %xmm0
    shufps       $(221), %xmm7, %xmm1
 
    movsd        (72)(%r11), %xmm7
    movsd        (104)(%r11), %xmm5
    movlhps      %xmm5, %xmm7
    movaps       %xmm2, %xmm3
    shufps       $(136), %xmm7, %xmm2
 
 
    movsd        (128)(%r11), %xmm4
    movsd        (160)(%r11), %xmm5
    movlhps      %xmm5, %xmm4
    shufps       $(221), %xmm7, %xmm3
    movsd        (192)(%r11), %xmm7
    movsd        (224)(%r11), %xmm5
    movlhps      %xmm5, %xmm7
    movsd        (136)(%r11), %xmm6
    movsd        (168)(%r11), %xmm5
    movlhps      %xmm5, %xmm6
 
    movaps       %xmm4, %xmm5
    shufps       $(136), %xmm7, %xmm4
    shufps       $(221), %xmm7, %xmm5
 
    movaps       %xmm4, (128)(%r11)
 
    movsd        (200)(%r11), %xmm4
    movsd        (232)(%r11), %xmm7
    movlhps      %xmm7, %xmm4
    movaps       %xmm6, %xmm7
    shufps       $(136), %xmm4, %xmm6
    shufps       $(221), %xmm4, %xmm7
 
 
    movaps       %xmm3, %xmm4
    subps        %xmm5, %xmm3
    addps        %xmm4, %xmm5
    movaps       %xmm7, %xmm4
    addps        %xmm1, %xmm7
    subps        %xmm4, %xmm1
 
 
    movaps       %xmm5, %xmm4
    addps        %xmm7, %xmm5
    subps        %xmm4, %xmm7
    movaps       tg_2_16_(%rip), %xmm4
    mulps        %xmm3, %xmm4
    mulps        cos2cos6m_(%rip), %xmm5
    mulps        cos6cos6p_(%rip), %xmm7
    addps        %xmm1, %xmm4
    mulps        tg_2_16_(%rip), %xmm1
    subps        %xmm3, %xmm1
    subps        %xmm5, %xmm4
    subps        %xmm4, %xmm7
 
 
    movaps       %xmm0, (%r11)
    addps        (128)(%r11), %xmm0
    movaps       %xmm6, %xmm3
    addps        %xmm2, %xmm6
    subps        %xmm3, %xmm2
    mulps        cos4cos4p_(%rip), %xmm2
    movaps       %xmm6, %xmm3
    addps        %xmm0, %xmm6
    subps        %xmm3, %xmm0
    subps        %xmm3, %xmm2
 
 
    subps        %xmm7, %xmm1
    movaps       %xmm5, %xmm3
    addps        %xmm6, %xmm5
    subps        %xmm3, %xmm6
    movaps       (%r11), %xmm3
    movaps       %xmm5, (%r11)
    subps        (128)(%r11), %xmm3
    movaps       %xmm6, (224)(%r11)
 
 
    movaps       %xmm1, %xmm5
    addps        %xmm0, %xmm1
    subps        %xmm5, %xmm0
    movaps       %xmm1, (96)(%r11)
    movaps       %xmm0, (128)(%r11)
    movaps       %xmm2, %xmm5
    addps        %xmm3, %xmm2
    subps        %xmm5, %xmm3
 
 
    movaps       %xmm4, %xmm6
    addps        %xmm2, %xmm4
    movaps       %xmm4, (32)(%r11)
    movaps       %xmm7, %xmm5
    addps        %xmm3, %xmm7
    movaps       %xmm7, (64)(%r11)
    subps        %xmm5, %xmm3
    movaps       %xmm3, (160)(%r11)
    subps        %xmm6, %xmm2
    movaps       %xmm2, (192)(%r11)
 
 
    movsd        (16)(%r11), %xmm0
    movsd        (48)(%r11), %xmm5
    movlhps      %xmm5, %xmm0
    movsd        (80)(%r11), %xmm7
    movsd        (112)(%r11), %xmm5
    movlhps      %xmm5, %xmm7
    movsd        (24)(%r11), %xmm2
    movsd        (56)(%r11), %xmm5
    movlhps      %xmm5, %xmm2
 
    movaps       %xmm0, %xmm1
    shufps       $(136), %xmm7, %xmm0
    shufps       $(221), %xmm7, %xmm1
 
    movsd        (88)(%r11), %xmm7
    movsd        (120)(%r11), %xmm5
    movlhps      %xmm5, %xmm7
    movaps       %xmm2, %xmm3
    shufps       $(136), %xmm7, %xmm2
 
 
    movsd        (144)(%r11), %xmm4
    movsd        (176)(%r11), %xmm5
    movlhps      %xmm5, %xmm4
    shufps       $(221), %xmm7, %xmm3
    movsd        (208)(%r11), %xmm7
    movsd        (240)(%r11), %xmm5
    movlhps      %xmm5, %xmm7
    movsd        (152)(%r11), %xmm6
    movsd        (184)(%r11), %xmm5
    movlhps      %xmm5, %xmm6
 
    movaps       %xmm4, %xmm5
    shufps       $(136), %xmm7, %xmm4
    shufps       $(221), %xmm7, %xmm5
 
    movaps       %xmm4, (144)(%r11)
 
    movsd        (216)(%r11), %xmm4
    movsd        (248)(%r11), %xmm7
    movlhps      %xmm7, %xmm4
    movaps       %xmm6, %xmm7
    shufps       $(136), %xmm4, %xmm6
    shufps       $(221), %xmm4, %xmm7
 
 
    movaps       %xmm3, %xmm4
    subps        %xmm5, %xmm3
    addps        %xmm4, %xmm5
    movaps       %xmm7, %xmm4
    addps        %xmm1, %xmm7
    subps        %xmm4, %xmm1
 
 
    movaps       %xmm5, %xmm4
    addps        %xmm7, %xmm5
    subps        %xmm4, %xmm7
    movaps       tg_2_16_(%rip), %xmm4
    mulps        %xmm3, %xmm4
    mulps        cos2cos6m_(%rip), %xmm5
    mulps        cos6cos6p_(%rip), %xmm7
    addps        %xmm1, %xmm4
    mulps        tg_2_16_(%rip), %xmm1
    subps        %xmm3, %xmm1
    subps        %xmm5, %xmm4
    subps        %xmm4, %xmm7
 
 
    movaps       %xmm0, (16)(%r11)
    addps        (144)(%r11), %xmm0
    movaps       %xmm6, %xmm3
    addps        %xmm2, %xmm6
    subps        %xmm3, %xmm2
    mulps        cos4cos4p_(%rip), %xmm2
    movaps       %xmm6, %xmm3
    addps        %xmm0, %xmm6
    subps        %xmm3, %xmm0
    subps        %xmm3, %xmm2
 
 
    subps        %xmm7, %xmm1
    movaps       %xmm5, %xmm3
    addps        %xmm6, %xmm5
    subps        %xmm3, %xmm6
    movaps       (16)(%r11), %xmm3
    movaps       %xmm5, (16)(%r11)
    subps        (144)(%r11), %xmm3
    movaps       %xmm6, (240)(%r11)
 
 
    movaps       %xmm1, %xmm5
    addps        %xmm0, %xmm1
    subps        %xmm5, %xmm0
    movaps       %xmm1, (112)(%r11)
    movaps       %xmm0, (144)(%r11)
    movaps       %xmm2, %xmm5
    addps        %xmm3, %xmm2
    subps        %xmm5, %xmm3
 
 
    movaps       %xmm4, %xmm6
    addps        %xmm2, %xmm4
    movaps       %xmm4, (48)(%r11)
    movaps       %xmm7, %xmm5
    addps        %xmm3, %xmm7
    movaps       %xmm7, (80)(%r11)
    subps        %xmm5, %xmm3
    movaps       %xmm3, (176)(%r11)
    subps        %xmm6, %xmm2
    movaps       %xmm2, (208)(%r11)
 
 
    cmp          %rsp, %r11
    jne          LEXITgas_2
 
    mov          %rsi, %r11
 
 
    mov          $(256), %rax
 
.p2align 4, 0x90
L__0074gas_2:  
    movaps       (-32)(%rsp,%rax), %xmm0
    movlps       %xmm0, (-32)(%r11,%rax)
    movhps       %xmm0, (-24)(%r11,%rax)
    movaps       (-16)(%rsp,%rax), %xmm0
    movlps       %xmm0, (-16)(%r11,%rax)
    movhps       %xmm0, (-8)(%r11,%rax)
    sub          $(32), %rax
    jg           L__0074gas_2
 
    mov          %rcx, %rsp
 
LEXITgas_2:  
 
    ret
 
 
