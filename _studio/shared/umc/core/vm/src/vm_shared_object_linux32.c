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

#include <dlfcn.h>
#include "vm_shared_object.h"

vm_so_handle vm_so_load(const vm_char* so_file_name)
{
    void *handle;

    /* check error(s) */
    if (NULL == so_file_name)
        return NULL;

    handle = dlopen(so_file_name, RTLD_LAZY);

    return (vm_so_handle)handle;

} /* vm_so_handle vm_so_load(vm_char* so_file_name) */

vm_so_func vm_so_get_addr(vm_so_handle so_handle, const char *so_func_name)
{
    vm_so_func addr;

    /* check error(s) */
    if (NULL == so_handle)
        return NULL;

    addr = dlsym(so_handle,so_func_name);

    return addr;

} /* void *vm_so_get_addr(vm_so_handle so_handle, const vm_char *so_func_name) */

void vm_so_free(vm_so_handle so_handle)
{
    /* check error(s) */
    if (NULL == so_handle)
        return;

    dlclose(so_handle);

} /* void vm_so_free(vm_so_handle so_handle) */
#else
# pragma warning( disable: 4206 )
#endif /* LINUX32 || OSX32 */
