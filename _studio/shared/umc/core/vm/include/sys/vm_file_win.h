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

/*
 * VM 64-bits buffered file operations library
 *    Windows definitions
 */
#ifndef  VM_FILE_WIN_H
#  define VM_FILE_WIN_H
#  include <windows.h>
#  include <Winbase.h>
#  include <stdarg.h>
#  include <string.h>
#  include "vm_mutex.h"

#    define VM_TEMPORARY_PREFIX 0x55

#define vm_stdin  (vm_file *)(size_t)3
#define vm_stderr (vm_file *)(size_t)1
#define vm_stdout (vm_file *)(size_t)2


typedef struct {
  HANDLE fd;
  vm_char *tbuf;    /* temporary buffer for character io operations */
  unsigned long long fsize;
  uint32_t fattributes;
  uint8_t   ftemp; /* temporary flag sign - added to support temporary files VM_TEMPORARY_PREFIX - temporary file */
  } vm_file;

typedef struct {
  HANDLE handle;
  WIN32_FIND_DATA ent;
  } vm_dir;
#endif
