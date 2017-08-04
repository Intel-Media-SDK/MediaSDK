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
 *       common implementation
 */
#include "vm_file.h"
#if defined(LINUX32)
# define SLASH '/'
#else
# define SLASH '\\'
#endif
/*
 * file name manipulations */
/*
 * return only path of file name */
void vm_file_getpath(vm_char *filename, vm_char *path, int nchars) {
  /* go to end of line and then move up until first SLASH will be found */
  int32_t len;
  path[0] = '\0';
  len = (int32_t) vm_string_strlen(filename);
  while(len && (filename[len--] != SLASH));
  if (len) {
      memcpy_s((void *)path, nchars, (const void *)filename, (len <= nchars) ? len+1 : nchars);
      path[(len <= nchars) ? len+1 : nchars] = '\0';
    }
  }

/*
 * return base file name free of path and all suffixes
 */
void vm_file_getbasename(vm_char *filename, vm_char *base, int nchars) {
  int32_t chrs = 0;
  vm_char *p, *q0, *q1, *s;
  base[0] = '\0';
  q0 = q1 = NULL;
  p = vm_string_strchr(filename, '.'); /* first invocation of . */
  s = filename;
  do {
    q0 = vm_string_strchr(s, SLASH);
    if (q0 != NULL) {
      q1 = q0;
      s = q0+1;
      }
    } while( q0 != NULL );
  if (p == NULL)
    p = &filename[vm_string_strlen(filename)];
  if ( q1 == NULL )
    q1 = filename;
  chrs = (int32_t) (p - q1);
  if (chrs) {
    if (q1[0] == SLASH) {
      ++q1;
      --chrs;
      }
    if (chrs > nchars)
      chrs = nchars-1;
    memcpy_s((void *)base, nchars, (const void *)q1, chrs);
    base[chrs] = '\0';
    }
  }
/*
 * return full file suffix or nchars of suffix if nchars is to small to fit the suffix
 * !!! if more then one suffix applied then only first from the end of filename will be found
 */
void vm_file_getsuffix(vm_char *filename, vm_char *suffix, int nchars) {
  /* go to end of line and then go up until we will meet the suffix sign . or
   * to begining of line if no suffix found */
  int32_t len, i = 0;
  len = (int32_t) vm_string_strlen(filename);
  suffix[0] = '\0';
  while(len && (filename[len--] != '.'));
  if (len) {
    len += 2;
    for( ; filename[len]; ++len) {
      suffix[i] = filename[len];
      if (++i >= nchars)
        break;
      }
    suffix[i] = '\0';
    }
  }

#define ADDPARM(A)                    \
  if ((uint32_t)nchars > vm_string_strlen(A)) {   \
    vm_string_strcat_s(filename, nchars, A);              \
    offs = (uint32_t) vm_string_strlen(filename);          \
    nchars -= offs;                   \
    if (nchars)                       \
      filename[offs] = SLASH;         \
    ++offs;                           \
    --nchars;                         \
    filename[offs] = '\0';            \
    }
/*
 * prepare complex file name according with OS rules:
 *    / delimiter for unix and \ delimiter for Windows */
void vm_file_makefilename(vm_char *path, vm_char *base, vm_char *suffix, vm_char *filename, int nchars) {
  uint32_t offs = 0;
  filename[0] = '\0';
  if ((path != NULL) && (vm_string_strlen(path) < (uint32_t)nchars))
    ADDPARM(path)
  if (nchars && (base != NULL))
    ADDPARM(base)
    if (nchars && (suffix != NULL)) {
      if (offs == 0) {
        filename[offs++] = '.';
        filename[offs] = '\0';
        --nchars;
        }
      else
        if (filename[offs-1] == SLASH)
          filename[offs-1] = '.';
      ADDPARM(suffix)
      }
    /* remove SLASH if exist */
    if (filename[offs-1] == SLASH)
      filename[offs-1] = '\0';
  }



