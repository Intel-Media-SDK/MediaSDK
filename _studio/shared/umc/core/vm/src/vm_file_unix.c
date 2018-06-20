// Copyright (c) 2017-2018 Intel Corporation
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
 *       UNIX implementation
 */
/* codecws compilation fence */
#if defined(LINUX32)

#if defined(LINUX32) && !defined(__ANDROID__) && !defined(LINUX64)
/* These defines are needed to get access to 'struct stat64'. stat64 function is seen without them, but
 * causes segmentation faults working with 'struct stat'.
 */
#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#if defined(__ANDROID__)
#include <sys/statfs.h>
#else
#include <sys/statvfs.h>
#endif /* #if !defined(__ANDROID__) */
#include <dirent.h>
#include "vm_file.h"


/* obtain file info. return 0 if file is not accessible,
   file_size or file_attr can be NULL if either is not interested */
int32_t vm_file_getinfo(const vm_char *filename, unsigned long long *file_size, uint32_t *file_attr) {
#if defined(__ANDROID__) || defined(LINUX64)
   struct stat buf;
   if (stat(filename,&buf) != 0) return 0;
#else
   struct stat64 buf;
   if (stat64(filename,&buf) != 0) return 0;
#endif
   if (file_size) *file_size=buf.st_size;
   if (file_attr) {
      *file_attr=0;
      if (buf.st_mode & S_IFREG) *file_attr|=VM_FILE_ATTR_FILE;
      if (buf.st_mode & S_IFDIR) *file_attr|=VM_FILE_ATTR_DIRECTORY;
      if (buf.st_mode & S_IFLNK) *file_attr|=VM_FILE_ATTR_LINK;
   }
   return 1;
  }



unsigned long long vm_file_fseek(vm_file *fd, long long position, VM_FILE_SEEK_MODE mode) {
#if defined(__ANDROID__) || defined(LINUX64)
  return fseeko(fd, (off_t)position, mode);
#else
  return fseeko64(fd, (__off64_t)position, mode);
#endif
  }

unsigned long long vm_file_ftell(vm_file *fd) {
#if defined(__ANDROID__) || defined(LINUX64)
  return (unsigned long long) ftello(fd);
#else
  return (unsigned long long)ftello64(fd);
#endif
  }

/*
 *   Directory manipulations
 */
int32_t vm_dir_remove(vm_char *path) {
   return !remove(path);
}

int32_t vm_dir_mkdir(vm_char *path) {
   return !mkdir(path,0777);
}


static vm_char *d_name = NULL;
int32_t vm_dir_open(vm_dir **dd, vm_char *path) {
  if ((dd[0]=opendir(path)) != NULL) {
    d_name = NULL;
    if(getcwd(d_name, 0) == NULL)
    {
      return 0;
    }
    if(chdir(path))
    {
      return 0;
    }
  }
  return (dd[0] != NULL) ? 1 : 0;
}

/*
 * directory traverse */
int32_t vm_dir_read(vm_dir *dd, vm_char *filename,int nchars) {
  (void)nchars;

  int32_t rtv = 0;
  if (dd != NULL) {
   struct dirent *ent=readdir(dd);
   if (ent) {
       vm_string_strncpy_s(filename, nchars, ent->d_name, vm_string_strnlen_s(ent->d_name, NAME_MAX));
     rtv = 1;
     }
   }
   return rtv;
}

void vm_dir_close(vm_dir *dd) {
  if (dd != NULL) {
    if (d_name != NULL) {
      if(chdir(d_name))
      {
        // Add error handling
      }
      free(d_name);
      d_name = NULL;
      }
    closedir(dd);
    }
}

/*
 * findfirst, findnext, findclose direct emulation
 * for old ala Windows applications
 */
int32_t vm_string_findnext(vm_findptr handle, vm_finddata_t *fileinfo) {
  int32_t rtv = 1;
  unsigned long long sz;
  uint32_t atr;
  if (vm_dir_read(handle, fileinfo[0].name, MAX_PATH))
    if (vm_file_getinfo(fileinfo[0].name, &sz, &atr)) {
      fileinfo[0].size = sz;
      fileinfo[0].attrib = atr;
      rtv = 0;
    }
  return rtv;
  }

vm_findptr vm_string_findfirst(vm_char *filespec, vm_finddata_t *fileinfo) {
  vm_findptr dd;
  vm_dir_open(&dd, filespec);
  if (dd != NULL)
    vm_string_findnext(dd, fileinfo);
  return dd;
  }

int32_t vm_string_findclose(vm_findptr handle) {
  return closedir(handle);
  }

unsigned long long vm_dir_get_free_disk_space( void ) {
  unsigned long long rtv = 0;
#if defined(__ANDROID__)
  struct statfs fst;
  if (statfs(".", &fst) >= 0) {
    rtv = fst.f_bsize*fst.f_bavail;
    }
#else
  struct statvfs fst;
  if (statvfs(".", &fst) >= 0) {
    rtv = fst.f_bsize*fst.f_bavail;
    }
#endif
  return rtv;
  }

void vm_string_splitpath(const vm_char *path, char *drive, char *dir, char *fname, char *ext) {

    if (path && drive && dir && fname && ext) {
        drive[0] = '\0';
        dir[0] = '\0';
        strcpy(fname, path);
        ext[0] = '\0';
    }
}

int32_t vm_file_vfprintf(vm_file *fd, vm_char* format, va_list argptr)
{
    int32_t sts = 0;
    va_list copy;
    va_copy(copy, argptr);
    sts = vfprintf(fd, format,  copy);
    va_end(argptr);
    return sts;
}

#else
# pragma warning( disable: 4206 )
#endif
