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
 *    Linux and OSX definitions
 */
#ifndef VM_FILE_UNIX_H
#  define VM_FILE_UNIX_H
#  include <stdio.h>
#  include <stdarg.h>
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <dirent.h>

#if !defined MAX_PATH
#  define MAX_PATH 260
#endif

typedef FILE vm_file;
typedef DIR vm_dir;

# define vm_stderr  stderr
# define vm_stdout  stdout
# define vm_stdin    stdin
/*
 * file access functions
 */
# if defined(LINUX64)
/* native fopen is 64-bits on OSX */
#  define vm_file_fopen    fopen
# else
#  define vm_file_fopen    fopen64
# endif

# define vm_file_fclose     fclose
# define vm_file_feof         feof
# define vm_file_remove  remove

/*
 * binary file IO */
# define vm_file_fread    fread
# define vm_file_fwrite    fwrite

/*
 * character (string) file IO */
# define vm_file_fgets      fgets
# define vm_file_fputs      fputs
# define vm_file_fscanf     fscanf
# define vm_file_fprintf    fprintf

/* temporary file support */
# define vm_file_tmpfile      tmpfile
# define vm_file_tmpnam       tmpnam
# define vm_file_tmpnam_r     tmpnam_r
# define vm_file_tempnam      tempnam

#endif //VM_FILE_UNIX_H

