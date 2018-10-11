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

#if defined(LINUX32)

#include "vm_sys_info.h"
#include <time.h>
#include <sys/utsname.h>
#include <unistd.h>

#include <sys/sysinfo.h>

uint32_t vm_sys_info_get_cpu_num(void)
{
#if defined(ANDROID)
    return sysconf(_SC_NPROCESSORS_ONLN); /* on Android *_CONF will return number of _real_ processors */
#else
    return sysconf(_SC_NPROCESSORS_CONF); /* on Linux *_CONF will return number of _logical_ processors */
#endif
} /* uint32_t vm_sys_info_get_cpu_num(void) */

void vm_sys_info_get_cpu_name(vm_char *cpu_name)
{
    FILE *pFile = NULL;
    vm_char buf[_MAX_LEN];
    vm_char tmp_buf[_MAX_LEN] = { 0 };
    size_t len;

    /* check error(s) */
    if (NULL == cpu_name)
        return;

    pFile = fopen("/proc/cpuinfo", "r");
    if (!pFile)
        return;

    while (fgets(buf, _MAX_LEN, pFile))
    {
        if (!vm_string_strncmp(buf, VM_STRING("vendor_id"), 9))
        {
            vm_string_strncpy_s(tmp_buf, _MAX_LEN, (vm_char*)(buf + 12), vm_string_strnlen_s(buf, _MAX_LEN) - 13);
        }
        else if (!vm_string_strncmp(buf, VM_STRING("model name"), 10))
        {
            if ((len = vm_string_strnlen_s(buf, _MAX_LEN) - 14) > 8)
                vm_string_strncpy_s(cpu_name, _MAX_LEN, (vm_char *)(buf + 13), len);
            else
                vm_string_snprintf(cpu_name, PATH_MAX, VM_STRING("%s"), tmp_buf);
        }
    }
    fclose(pFile);
} /* void vm_sys_info_get_cpu_name(vm_char *cpu_name) */

void vm_sys_info_get_vga_card(vm_char *vga_card)
{
    /* check error(s) */
    if (NULL == vga_card)
        return;
} /* void vm_sys_info_get_vga_card(vm_char *vga_card) */

void vm_sys_info_get_os_name(vm_char *os_name)
{
    struct utsname buf;

    /* check error(s) */
    if (NULL == os_name)
        return;

    uname(&buf);
    vm_string_sprintf(os_name, VM_STRING("%s %s"), buf.sysname, buf.release);

} /* void vm_sys_info_get_os_name(vm_char *os_name) */

void vm_sys_info_get_computer_name(vm_char *computer_name)
{
    /* check error(s) */
    if (NULL == computer_name)
        return;

    gethostname(computer_name, 128);

} /* void vm_sys_info_get_computer_name(vm_char *computer_name) */

void vm_sys_info_get_program_name(vm_char *program_name)
{
    /* check error(s) */
    if (NULL == program_name)
        return;

    vm_char path[PATH_MAX] = {0,};
    size_t i = 0;

    if(readlink("/proc/self/exe", path, sizeof(path)) == -1)
    {
        // Add error handling
    }
    i = vm_string_strrchr(path, (vm_char)('/')) - path + 1;
    vm_string_strncpy_s(program_name, PATH_MAX, path + i, vm_string_strnlen_s(path, PATH_MAX) - i);

} /* void vm_sys_info_get_program_name(vm_char *program_name) */

void vm_sys_info_get_program_path(vm_char *program_path)
{
    vm_char path[ PATH_MAX ] = {0,};
    size_t i = 0;

    /* check error(s) */
    if (NULL == program_path)
        return;

    if (readlink("/proc/self/exe", path, sizeof(path)) == -1)
    {
        // Add error handling
    }
    i = vm_string_strrchr(path, (vm_char)('/')) - path + 1;
    vm_string_strncpy_s(program_path, PATH_MAX, path, i-1);
    program_path[i - 1] = 0;

} /* void vm_sys_info_get_program_path(vm_char *program_path) */

uint32_t vm_sys_info_get_cpu_speed(void)
{
    double ret = 0;
    FILE *pFile = NULL;
    vm_char buf[PATH_MAX];

    pFile = fopen("/proc/cpuinfo", "r" );
    if (!pFile)
        return 1000;

    while (fgets(buf, PATH_MAX, pFile))
    {
        if (!vm_string_strncmp(buf, VM_STRING("cpu MHz"), 7))
        {
            ret = vm_string_atol((vm_char *)(buf + 10));
            break;
        }
    }
    fclose(pFile);
    return ((uint32_t) ret);
} /* uint32_t vm_sys_info_get_cpu_speed(void) */

uint32_t vm_sys_info_get_mem_size(void)
{
    struct sysinfo info;
    sysinfo(&info);
    return (uint32_t)((double)info.totalram / (1024 * 1024) + 0.5);

} /* uint32_t vm_sys_info_get_mem_size(void) */

#else
# pragma warning( disable: 4206 )
#endif /* LINUX32 */
