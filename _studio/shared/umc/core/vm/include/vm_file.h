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
 *       common definitions
 */
#ifndef VM_FILE_H
#  define VM_FILE_H

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
#  include "umc_defs.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif



#  include "vm_types.h"
#  include "vm_strings.h"
# if defined(LINUX32)
#     include "sys/vm_file_unix.h"
#  else
#     include "sys/vm_file_win.h"
#  endif

/*
 * bit or-ed file attribute */
enum {
   VM_FILE_ATTR_FILE        =0x0001,
   VM_FILE_ATTR_DIRECTORY   =0x0002,
   VM_FILE_ATTR_LINK        =0x0004,
   VM_FILE_ATTR_HIDDEN      =0x0008
};

/*
 * seek whence */
typedef enum {
   VM_FILE_SEEK_SET=0,
   VM_FILE_SEEK_CUR=1,
   VM_FILE_SEEK_END=2
} VM_FILE_SEEK_MODE;


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * file access functions
 */
unsigned long long vm_file_fseek(vm_file *fd, long long position, VM_FILE_SEEK_MODE mode);
unsigned long long vm_file_ftell(vm_file *fd);
int32_t vm_file_getinfo(const vm_char *filename, unsigned long long *file_size, uint32_t *file_attr);
int32_t vm_file_vfprintf(vm_file *fd, vm_char *format,  va_list argptr);

#if defined(LINUX32)

    #define vm_file_fflush fflush

#else /* #if defined(LINUX32) || defined(OSX) */

int32_t vm_file_fflush(vm_file *fd);
vm_file *vm_file_fopen(const vm_char *fname, const vm_char *mode);
int32_t vm_file_fclose(vm_file *fd);
int32_t vm_file_feof(vm_file* fd);
int32_t vm_file_remove(vm_char *path);
/*
 * binary file IO */
int32_t vm_file_fread(void *buf, uint32_t itemsz, int32_t nitems, vm_file *fd);
int32_t vm_file_fwrite(void *buf, uint32_t itemsz, int32_t nitems, vm_file *fd);

/*
 * character (string) file IO */
vm_char *vm_file_fgets(vm_char *str, int nchar, vm_file *fd);
int32_t vm_file_fputs(vm_char *str, vm_file *fd);
int32_t vm_file_fscanf(vm_file *fd,vm_char *format, ...);
int32_t vm_file_fprintf(vm_file *fd,vm_char *format, ...);
int32_t vm_file_vfprintf(vm_file *fd, vm_char *format,  va_list argptr);

/*
 * Temporary files support
 * ISO/IEC 9899:1990 emulation
 * Without stdlib.h defined
 *             mktemp, mkstemp and mkdtemp functions emulation for the first time.
 */
vm_file *vm_file_tmpfile(void);
vm_char *vm_file_tmpnam(vm_char *RESULT);
vm_char *vm_file_tmpnam_r(vm_char *RESULT); /* the same as tmpnam_r except that returns NULL if RESULT is NULL  -- fully reentrant function */
vm_char *vm_file_tempnam(vm_char *DIR, vm_char *PREFIX);


#endif /* #if defined(LINUX32) || defined(OSX) */

/*
 * file name manipulations */
void vm_file_getpath(vm_char *filename, vm_char *path, int nchars);
void vm_file_getbasename(vm_char *filename, vm_char *base, int nchars);
void vm_file_getsuffix(vm_char *filename, vm_char *suffix, int nchars);
void vm_file_makefilename(vm_char *path, vm_char *base, vm_char *suffix, vm_char *filename, int nchars);


/*
 *   Directory manipulations
 */
int32_t vm_dir_remove(vm_char *path);
int32_t vm_dir_mkdir(vm_char *path);

/* directory traverse */
int32_t vm_dir_open(vm_dir **dd, vm_char *path);
int32_t vm_dir_read(vm_dir *dd, vm_char *filename,int nchars);
void vm_dir_close(vm_dir *dd);

unsigned long long vm_dir_get_free_disk_space( void );

/*
 * defines for non-buffered file operations
 */
#define vm_file_open  vm_file_fopen
#define vm_file_read  vm_file_fread
#define vm_file_write vm_file_fwrite
#define vm_file_close vm_file_fclose
#define vm_file_seek  vm_file_fseek
#define vm_file_tell  vm_file_ftell
#define vm_file_gets  vm_file_fgets

/*
 */
# ifdef __cplusplus
  }
# endif
#endif /* VM_FILE_H */
