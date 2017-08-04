// Copyright (c) 2017 Intel Corporation
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

#ifndef __VM_SYS_INFO_H__
#define __VM_SYS_INFO_H__


#include "vm_types.h"
#include "vm_strings.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/* Functions to obtain processor's specific information */
uint32_t vm_sys_info_get_cpu_num(void);
uint32_t vm_sys_info_get_numa_nodes_num(void);
unsigned long long vm_sys_info_get_cpu_mask_of_numa_node(uint32_t node);
void vm_sys_info_get_cpu_name(vm_char *cpu_name);
void vm_sys_info_get_vga_card(vm_char *vga_card);
void vm_sys_info_get_os_name(vm_char *os_name);
void vm_sys_info_get_computer_name(vm_char *computer_name);
void vm_sys_info_get_program_name(vm_char *program_name);
void vm_sys_info_get_program_path(vm_char *program_path);
uint32_t vm_sys_info_get_cpu_speed(void);
uint32_t vm_sys_info_get_mem_size(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VM_SYS_INFO_H__ */
