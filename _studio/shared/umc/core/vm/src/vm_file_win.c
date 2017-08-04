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
 *       Windows implementation
 */


/* codecws compilation fence */
#if !defined(LINUX32)
# include <stdio.h>
# include <io.h>
# include "umc_defs.h"
# include "vm_strings.h"
# include "vm_file.h"


#define DELETE_VM_FILE(A) \
  if (A != NULL) \
  { \
    if (A[0].tbuf != NULL) \
      free((void *)A[0].tbuf); \
 \
    free((void *)A); \
    A = NULL; \
  }

#ifdef THREAD_SAFETY
# define LOCK  vm_mutex_lock(fd[0].fd)
# define UNLOCK vm_mutex_unlock(fd[0].fd)
#else
# define LOCK
# define UNLOCK
#endif


#define VM_MAX_TEMP_LINE 8192

static vm_char temporary_path[MAX_PATH];

static BOOL tmpup = FALSE;
/*
 * low level temporary file support data
 * structures and functions
 */

typedef struct
{
  void*    next;
  void*    prev;
  vm_file* handle;
  vm_char* path;

} vm_temp_file_entry;

static
vm_temp_file_entry* temp_list_head;


static
void tempfile_add_entry(vm_temp_file_entry* r)
{
  if (temp_list_head != NULL)
  {
    r[0].next = temp_list_head;
    temp_list_head[0].prev = r;
    temp_list_head = r;
  }
  else
    temp_list_head = r;

  return;
} /* tempfile_add_entry() */


static
void tempfile_remove_entry(vm_temp_file_entry* r)
{
    vm_temp_file_entry* p;
    vm_temp_file_entry* q;
    BOOL sts;

    sts = TRUE;

    if ((p = temp_list_head) != NULL)
    {
        while( sts )
        {
            if(p == r)
            {
                if (p[0].prev == NULL)
                {
                    q = (vm_temp_file_entry *)p[0].next;
                    if (q != NULL)
                        q[0].prev = NULL;
                    temp_list_head = q;
                }
                else
                {
                    q = (vm_temp_file_entry *)p[0].prev;
                    q[0].next = p[0].next;
                    q = (vm_temp_file_entry *)p[0].next;
                    if (q != NULL)
                    {
                      q[0].prev = p[0].prev;
                      if (q[0].prev == NULL)
                        temp_list_head = q;
                    }
                }

                free(p[0].path);
                free(p);
                p = NULL;
            }
            else
                p = (vm_temp_file_entry *)p[0].next;

            sts = (p != NULL);
        }
    }

    return;
} /* tempfile_remove_entry() */


static
void tempfile_delete_by_handle(vm_file* fd)
{
    vm_temp_file_entry* p;

    if ((p = temp_list_head) != NULL)
    {
        while(p != NULL)
            if (p[0].handle == fd)
            {
                vm_file_remove(p[0].path);
                tempfile_remove_entry(p);
                return;
            }
            else
                p = (vm_temp_file_entry *)p[0].next;
    }

    return;
} /* tempfile_delete_by_handle() */


static
void tempfile_delete_all(void)
{
    vm_temp_file_entry* p;
    vm_temp_file_entry* q;

    if ((p = temp_list_head) != NULL)
    {
        while ( p != NULL)
        {
            q = p;
            p = (vm_temp_file_entry *)p[0].next;
            vm_file_close(q[0].handle); /* close and delete if temporary file is being closed */
        }
    }

    return;
} /* tempfile_delete_all() */


static
void vm_file_remove_all_temporary( void )
{
    tempfile_delete_all();
    return;
}


static
vm_file* tempfile_create_file(vm_char* fname)
{
    uint32_t   nlen;
    vm_file* rtv;
    vm_temp_file_entry* t;

    rtv = NULL;

    nlen = (uint32_t)vm_string_strlen(fname) + 1;

    t = (vm_temp_file_entry*)malloc(sizeof(vm_temp_file_entry));

    if (t != NULL)
    {
        t[0].handle = NULL;
        t[0].next = t[0].prev = NULL;
        t[0].path = (vm_char*)malloc(nlen*sizeof(vm_char));

        if(t[0].path != NULL)
        {
            vm_string_strcpy_s(t[0].path, nlen*sizeof(vm_char), fname);

            if((t[0].handle = vm_file_fopen(t[0].path, _T("w+"))) != NULL)
            {
                rtv = t[0].handle;
                t[0].handle[0].ftemp = VM_TEMPORARY_PREFIX;
                tempfile_add_entry(t);
            }
        } else
            free(t);
    }

    return rtv;
} /* tempfile_create_file() */


static
void tempfile_check(void)
{
  if (!tmpup)
  {
  if (GetTempPath(MAX_PATH, temporary_path) == 0)
    vm_string_strcpy_s(temporary_path, MAX_PATH, _T("C:\\"));
    atexit(tempfile_delete_all);
  }

  tmpup = TRUE;

  return;
} /* tempfile_check() */


vm_file* vm_file_tmpfile(void)
{
    vm_file* rtv;
    vm_char* nbf;

    tempfile_check();

    rtv = NULL;

    nbf = (vm_char*)malloc(MAX_PATH*sizeof(vm_char));
    if (nbf != NULL)
    {
        if(vm_file_tmpnam(nbf) != NULL)
            rtv = tempfile_create_file(nbf);

        free(nbf);
    }

    return rtv;
} /* vm_file_tmpfile() */


vm_char* vm_file_tmpnam(vm_char* RESULT)
{
    vm_char* rtv;

    tempfile_check();

    rtv = NULL;

    if (RESULT == NULL)
        RESULT = (vm_char*)malloc(MAX_PATH*sizeof(vm_char));

    if (RESULT != NULL)
    {
        if (GetTempFileName(temporary_path, _T("vmt"), 0,  RESULT) != 0)
        {
            rtv = RESULT;
            DeleteFile(rtv);  /* we need only name, not a file */
        }
    }

    return rtv;
} /* vm_file_tmpnam() */


vm_char* vm_file_tmpnam_r(vm_char* RESULT)
{
    tempfile_check();

    if (RESULT != NULL)
        vm_file_tmpnam(RESULT);

    return RESULT;
} /* vm_file_tmpnam_r() */


vm_char* vm_file_tempnam(vm_char* DIR, vm_char* PREFIX)
{
    vm_char* nbf;

    tempfile_check();

    nbf = (vm_char*)malloc(MAX_PATH*sizeof(vm_char));
    if (0 == nbf)
    {
        return NULL;
    }
    if ((DIR == NULL) || (PREFIX == NULL))
        vm_file_tmpnam(nbf);
    else
    {
        if (GetTempFileName(DIR, PREFIX, 0,  nbf) != 0)
        {
            DeleteFile(nbf);  /* we need only name, not a file */
        }
        else
        {
            free(nbf);
            nbf = NULL;
        }
    }

    return nbf;
} /* vm_file_tempnam() */


/*
 * file access functions
 */

vm_file* vm_file_fopen(const vm_char* fname, const vm_char* mode)
{
  uint32_t   i;
  DWORD    mds[4];
  vm_char  general;
  vm_char  islog;
  vm_file* rtv;

  general = islog = 0;

  rtv = (vm_file*)malloc(sizeof(vm_file));
  if (rtv != NULL)
  {
    rtv[0].fd          = NULL;
    rtv[0].fsize       = 0;
    rtv[0].fattributes = 0;
/*#ifdef _WIN32_WCE*/
    rtv[0].ftemp       = 0;
/*#endif*/

    vm_file_getinfo(fname, &rtv[0].fsize, &rtv[0].fattributes);

    rtv[0].tbuf = (vm_char*)malloc(VM_MAX_TEMP_LINE*sizeof(vm_char));
    if (rtv[0].tbuf != NULL)
    {
      /* prepare dwDesiredAccess */
      mds[0] = mds[1] = mds[2] = mds[3] = 0;

      for(i = 0; mode[i] != 0; ++i)
      {
        switch (mode[i])
        {
          case 'w':
            mds[0] |= GENERIC_WRITE;
            mds[2]  = CREATE_ALWAYS;
            mds[1]  = FILE_SHARE_READ;
            general = 'w';
            break;

          case 'r':
            mds[0] |= GENERIC_READ;
            mds[2]  = OPEN_EXISTING;
            mds[1]  = FILE_SHARE_READ;
            general = 'r';
            break;

          case 'a':
            mds[0] |= (GENERIC_WRITE | GENERIC_READ);
            mds[2]  = OPEN_ALWAYS;
            mds[1]  = 0;
            general = 'a';
            break;

          case '+':
            mds[0] |= (GENERIC_WRITE | GENERIC_READ);
            mds[1]  = FILE_SHARE_READ;
            break;

          case 'l':
            islog = 1; /* non-buffered file - for log files */
        }
      }

      mds[3] = FILE_ATTRIBUTE_NORMAL | ((islog == 0) ? 0 : FILE_FLAG_NO_BUFFERING);

      rtv[0].fd = CreateFile((LPCTSTR)fname, mds[0], mds[1], NULL, mds[2], mds[3], NULL);
      if (rtv[0].fd == INVALID_HANDLE_VALUE)
      {
        DELETE_VM_FILE(rtv);
      }
      else
      {
        /* check file open mode and move file pointer to the end of file if need it */
        if (general == 'a')
          vm_file_fseek(rtv, 0, VM_FILE_SEEK_END);
      }
    }
    else
    {
        DELETE_VM_FILE(rtv);
    }
  }

  return rtv; /* handle created - have to be destroyed by close function */
} /* vm_file_fopen() */


int32_t vm_file_fflush(vm_file *fd)
{
  int32_t rtv = -1;
  if (fd == vm_stdout)
      rtv = fflush(stdout);
  else if (fd == vm_stderr)
      rtv = fflush(stderr);
  else
      rtv = (FlushFileBuffers(fd[0].fd)) ? 0 : 1;
  return rtv;
 }

unsigned long long vm_file_fseek(vm_file* fd, long long position, VM_FILE_SEEK_MODE mode)
{
  unsigned long long rtv   = 1;
  DWORD  posmd = 0;

  union
  {
    int32_t lpt[2];
    long long hpt;
  } pwt;

  pwt.hpt = position;

  switch (mode)
  {
    case VM_FILE_SEEK_END: posmd = FILE_END;     break;
    case VM_FILE_SEEK_SET: posmd = FILE_BEGIN;   break;
    case VM_FILE_SEEK_CUR:
    default:               posmd = FILE_CURRENT; break;
  }

  if ((fd != NULL) && (fd[0].fd != INVALID_HANDLE_VALUE))
  {
    LOCK;
    pwt.lpt[0] = SetFilePointer(fd[0].fd, pwt.lpt[0], (LONG *)&pwt.lpt[1], posmd);
    if ((pwt.lpt[0] != -1) || (GetLastError() == NO_ERROR))
      rtv = 0;
    UNLOCK;
  }

  return rtv;
} /* vm_file_fseek() */


unsigned long long vm_file_ftell(vm_file* fd)
{
  unsigned long long rtv = 0;

  union
  {
    int32_t lpt[2];
    long long hpt;
  } pwt;

  pwt.hpt = 0; /* query current file position */

  if ((fd != NULL) && (fd[0].fd != INVALID_HANDLE_VALUE))
  {
    LOCK;
    pwt.lpt[0] = SetFilePointer(fd[0].fd, pwt.lpt[0], (LONG *)&pwt.lpt[1], FILE_CURRENT);
    if ((pwt.lpt[0] != -1) || (GetLastError() == NO_ERROR))
      rtv = (unsigned long long)pwt.hpt;
    else
      rtv = (unsigned long long) -1;
    UNLOCK;
  }

  return rtv;
} /* vm_file_ftell() */


int32_t vm_file_remove(vm_char* path)
{
  return DeleteFile(path);
}


int32_t vm_file_fclose(vm_file* fd)
{
  int32_t rtv = 0;

  if ((fd != NULL) && (fd[0].fd != INVALID_HANDLE_VALUE))
  {
    LOCK;
    CloseHandle(fd[0].fd);
    UNLOCK;

    /* delete file if temporary file is being closed (according to IEC/ISO specification) */
    if (fd[0].ftemp == VM_TEMPORARY_PREFIX)
        tempfile_delete_by_handle(fd);
    /* return memory, allocated in CreateFile */
    DELETE_VM_FILE(fd);
    rtv = 1;
  }

  return rtv;
} /* vm_file_fclose() */


int32_t vm_file_feof(vm_file* fd)
{
  int32_t rtv = 0;

  if ((fd != NULL) && (fd[0].fd != INVALID_HANDLE_VALUE))
  {
    unsigned long long pos = vm_file_ftell(fd);
    if (pos >= fd[0].fsize)
      rtv = 1;
  }

  return rtv;
} /* vm_file_feof() */


int32_t vm_file_getinfo(const vm_char* filename, unsigned long long* file_size, uint32_t* file_attr)
{
  int32_t rtv = 0;
  int32_t needsize = (file_size != NULL);
  int32_t needattr = (file_attr != NULL);
  WIN32_FILE_ATTRIBUTE_DATA ffi;

  if (filename && (needsize || needattr))
  {
    if (GetFileAttributesEx(filename, GetFileExInfoStandard, &ffi) != 0)
    {
      if (needattr)
      {
        file_attr[0] += (ffi.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) ? VM_FILE_ATTR_HIDDEN : 0;
        file_attr[0] += (ffi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? VM_FILE_ATTR_DIRECTORY : 0;
        file_attr[0] += (ffi.dwFileAttributes & FILE_ATTRIBUTE_NORMAL) ? VM_FILE_ATTR_FILE : 0;
      }

      if (needsize)
      {
        file_size[0] = ffi.nFileSizeHigh; /* two steps to avoid 32 to 32 shift problem */
        file_size[0] = ffi.nFileSizeLow + ( file_size[0] << 32);
      }

      rtv = 1;
      }
    }

  return rtv;
} /* vm_file_getinfo() */


/* binary file IO */

int32_t vm_file_fread(void* buf, uint32_t itemsz, int32_t nitems, vm_file* fd)
{
  DWORD  nmbread = 0;
  int32_t rtv     = 0;

  if ((fd != NULL) && (fd[0].fd != INVALID_HANDLE_VALUE))
  {
    LOCK;
    rtv = (ReadFile(fd[0].fd, buf, nitems*itemsz, &nmbread, NULL)) ? (nmbread/itemsz) : 0;
    UNLOCK;
  }

  return rtv;
} /* vm_file_fread() */


int32_t vm_file_fwrite(void* buf, uint32_t itemsz, int32_t nitems, vm_file* fd)
{
  DWORD  nmbread;
  int32_t rtv = 0;

  if ((fd != NULL) && (fd[0].fd != INVALID_HANDLE_VALUE))
  {
    LOCK;
    rtv = (WriteFile(fd[0].fd, buf, nitems*itemsz, &nmbread, NULL)) ? (nmbread/itemsz) : 0;
    UNLOCK;
  }

  return rtv;
} /* vm_file_fwrite() */


/*
 * character (string) file IO
 */

/*
 * read characters from input file until newline found, null character found,
 * nchars rad or EOF reached
 */

vm_char* vm_file_fgets(vm_char* str, int nchar, vm_file* fd)
{
  /* read up to VM_MAX_TEMP_LINE bytes from input file, try
   * to satisfy fgets conditions
   */
  long long fpos;
  int32_t bytesi, rdchar, rdbyte, i, j = 0; /* j - current position in the output string */
  vm_char* rtv = NULL;
#ifdef _UNICODE
      char *pstr;
      char  b;/*second byte for unicode reading*/
#endif

  if (fd == vm_stdin)
#ifdef _UNICODE
    return fgetws(str, nchar, stdin);
#else
    return fgets(str, nchar, stdin);
#endif
  else
  {
    str[0] = str[nchar-1] = 0;
    --nchar;

    if ((fd != NULL) && (fd[0].fd != INVALID_HANDLE_VALUE))
    {
      while ((rdbyte = vm_file_fread(fd[0].tbuf, 1, VM_MAX_TEMP_LINE, fd)) != 0)
      {
#ifdef UNICODE
          rdchar = (rdbyte >> 1) + (rdbyte & 1);
#else
          rdchar = rdbyte;
#endif
        bytesi = rdbyte;
        for(i = 0; i < rdchar; ++i)
        {
          str[j] = fd[0].tbuf[i];

#ifdef UNICODE
          pstr = (char*)(str+j);
          b = *(pstr+1);
          *(pstr+1) = 0;
          
          if (i + 1 != rdchar || !(rdbyte & 1)) 
          {
              //second byte available only if exactly 2 bytes at the end
              *(pstr+2) = b;
              *(pstr+3) = 0;
          }
          else
          {
              *(pstr+2) = 0;
              *(pstr+3) = 0;
          }

          if((*pstr == 0) || (*pstr == '\n') || (j >= nchar))
          {
              bytesi = i * 2;
              break;
          }

          j++;
      
          if((str[j] == 0) || (b == '\n') || (j >= nchar))
          {
              bytesi = i * 2 + 1;
              break;
          }
#else
          if((str[j]==0) || (str[j]=='\n') || (j >= nchar))
              break;
#endif
          ++j;
        }

        //i now is a byte index not a char index
        i = bytesi;

        if (i < rdbyte)
        {
          /* one of EOS conditions found */
          if ((str[j] == '\n') && (j < nchar)) /* add null character if possible */
            str[++j] = 0;

          if (str[j-2] == '\r')
          {
            /* remove CR from end of line */
            str[j-2] = '\n';
            str[j-1] = '\0';
          }

          /* return file pointer to the first non-string character */
          ++i; /* skip end of line character */

          fpos = i - rdbyte; /* - -- because file pointer will move back */
          if (fpos != 0)
            vm_file_fseek(fd, fpos, VM_FILE_SEEK_CUR);

          rtv = str;
          break; /* leave while loop */
        }
      }

      if((rtv == NULL) && (j != 0) && vm_file_feof(fd))
      {
        rtv = str; /* end of file during read - input string is valid */
        if (j < nchar)
          str[++j] = 0;
      }
    }
  }

  return rtv;
} /* vm_file_fgets() */


#ifdef _UNICODE
# define FPUTS fputws
#else
# define FPUTS fputs
#endif

int32_t vm_file_fputs(vm_char* str, vm_file* fd)
{
    if ( fd == vm_stdout )
    {
        return FPUTS(str, stdout);
    }
    else if (fd == vm_stderr)
    {
        return FPUTS(str, stderr);
    }
    else
    {
        size_t strLen = vm_string_strlen(str);
        return (vm_file_fwrite((void*)str, 1, (uint32_t)strLen, fd)==(int32_t)strLen) ? 1 : -1;
    }
}


/* parse line, return total number of format specifiers */
static
int32_t fmtline_spcnmb( vm_char* s )
{
  int32_t rtv = 0;

  while ( *s )
    if ((*s++ == '%') && (*s != '%'))
      ++rtv;

  return rtv;
}


int32_t vm_file_fscanf(vm_file *fd, vm_char *format, ...)
{
  BOOL     eol = FALSE;
  uint32_t   i = 0;
  int32_t   rtv = 0;
  int32_t   nsize = VM_MAX_TEMP_LINE;
  int32_t   lnoffset = 0;
  int32_t   items = 0;
  vm_char* s = NULL;
  vm_char* fpt = NULL;
  vm_char* bpt = NULL;
  vm_char* ws = NULL;
  va_list  args;
  vm_char  tmp = '\0';

  if ((fd != NULL) && (fd[0].fd != INVALID_HANDLE_VALUE))
  {
    /* fscanf always deals with one text line */
    /* text line length is not limited so we have to be ready
     * to receive huge line
     */
    s = (vm_char*)malloc(VM_MAX_TEMP_LINE);
    do
    {
      if (s != NULL)
      {
        if (fd == vm_stdin)
#ifdef _UNICODE
          ws = fgetws(&s[lnoffset], VM_MAX_TEMP_LINE, stdin);
#else
          ws = fgets(&s[lnoffset], VM_MAX_TEMP_LINE, stdin);
#endif
        else
          ws = vm_file_fgets(&s[lnoffset], VM_MAX_TEMP_LINE, fd);

        if ( ws != NULL)
        {
          if ((vm_string_strchr((const vm_char*)s, 0) != NULL) ||
              (vm_string_strchr((const vm_char*)s, '\n') != NULL))
          {
            /* line complete and ready to scanf */
            items = fmtline_spcnmb( format );
            ws = vm_string_strtok(s, VM_STRING(" ,"));

            /* try to convert all parameters on the step by step basis */
            bpt = fpt = (vm_char *)malloc(vm_string_strlen(format)+16);
            if (0 == fpt)
            {
                return 0;
            }
            vm_string_strcpy_s(fpt, vm_string_strlen(format)+16, format);
            va_start( args, format );

            while(items--)
            {
              while((*fpt != '%') && (*fpt))
                ++fpt;

              tmp = '\0';

              if( items > 0)
              {
                for(i = 1; (fpt[i] != '\0' && fpt[i] != '\n' &&
                            fpt[i] != ' '  && fpt[i] != '%' &&
                            fpt[i] != ','); ++i)
                  ;

                if ((fpt[i] != '\0') && (fpt[i]!='\n'))
                {
                  tmp    = fpt[i];
                  fpt[i] = '\0';
                }
              }

              if (*fpt)
                rtv += _stscanf_s(ws, fpt, va_arg(args, void*));
              else
                items = 0;

              if (items > 0)
              {
                ws = vm_string_strtok(NULL, VM_STRING(" ,"));
                fpt[i] = tmp;
                ++fpt;
              }
            }

            va_end( args );
            eol = TRUE;
            free(bpt);
          }
          else
          {
            nsize    += VM_MAX_TEMP_LINE;
            lnoffset += VM_MAX_TEMP_LINE;
            s = (vm_char *)realloc(s, nsize);
          }
        }
        else
          eol = TRUE; /* read from file problem ? */
      }
    } while (!eol);

    free(s);
  }

  return rtv;
} /* vm_file_fscanf() */


/*
 * Error may occur if output string becomes longer than VM_MAX_TEMP_LINE
 */

int32_t vm_file_fprintf(vm_file *fd, vm_char *format, ...)
{
  DWORD   nmbwrite;
  int32_t  rtv = 0;
  va_list args;
#if defined UNICODE
  int32_t  nPos = 0;
  int32_t  nStrLen = 0;
#endif
  va_start( args, format );

  if (fd == vm_stdout)
    rtv = _vftprintf(stdout, format, args);
  else
  {
    if (fd == vm_stderr)
      rtv = _vftprintf(stderr, format, args);
    else
    {
      if ((fd != NULL) && (fd[0].fd != INVALID_HANDLE_VALUE))
      {
        vm_string_vsprintf( fd[0].tbuf, format, args );

        LOCK;
        nStrLen = (int32_t)vm_string_strlen(fd[0].tbuf);
#if defined UNICODE
        for (nPos = 0; nPos < nStrLen; nPos++)
        {
            rtv = (WriteFile(fd[0].fd, fd[0].tbuf + nPos, 1, &nmbwrite, NULL)) ? rtv + 1 : 0;
            if (!rtv)
                break;
        }
#else
        rtv = (WriteFile(fd[0].fd, fd[0].tbuf, (DWORD)vm_string_strlen(fd[0].tbuf), &nmbwrite, NULL)) ? nmbwrite : 0;

#endif
        UNLOCK;

        va_end( args );
      }
    }
  }

  return rtv;
} /* vm_file_fprintf() */


int32_t vm_file_vfprintf(vm_file* fd, vm_char* format,  va_list argptr)
{
  DWORD  nmbwrite;
  int32_t rtv = 0;
#if defined UNICODE
  int32_t  nPos = 0;
  int32_t  nStrLen = 0;
#endif

  if (fd == vm_stdout)
    _vftprintf(stdout, format, argptr);
  else
  {
    if (fd == vm_stderr)
      _vftprintf(stderr, format, argptr);
    else
    {
      if ((fd != NULL) && (fd[0].fd != INVALID_HANDLE_VALUE))
      {
        vm_string_vsprintf( fd[0].tbuf, format, argptr );

        LOCK;
        nStrLen = (int32_t)vm_string_strlen(fd[0].tbuf);
#if defined UNICODE
        for (nPos = 0; nPos < nStrLen; nPos++)
        {
            rtv = (WriteFile(fd[0].fd, fd[0].tbuf + nPos, 1, &nmbwrite, NULL)) ? rtv + 1 : 0;
            if (!rtv)
                break;
        }
#else
        rtv = (WriteFile(fd[0].fd, fd[0].tbuf, (DWORD)vm_string_strlen(fd[0].tbuf), &nmbwrite, NULL)) ? nmbwrite : 0;

#endif
        UNLOCK;
      }
    }
  }

  return rtv;
} /* vm_file_vfprintf() */


/* Directory manipulations */

int32_t vm_dir_remove(vm_char* path)
{
  int32_t rtv = 1;

  if (!DeleteFile(path))
    rtv = RemoveDirectory(path);

  return rtv;
}


int32_t vm_dir_mkdir(vm_char* path)
{
   return CreateDirectory(path, NULL);
}


int32_t vm_dir_open(vm_dir** dd, vm_char* path)
{
  int32_t rtv = 0;
  WIN32_FIND_DATA fdata;

  vm_dir* td = (vm_dir*)malloc(sizeof(vm_dir));

  if (td != NULL)
  {
    td->handle = FindFirstFile(path, &fdata/*&td->ent*/);
    *dd = td;

    if (td->handle != INVALID_HANDLE_VALUE)
      rtv = 1;
  }

  return rtv;
} /* vm_dir_open() */


int32_t vm_dir_read(vm_dir* dd, vm_char* filename,int nchars)
{
  int32_t rtv = 0;
  WIN32_FIND_DATA fdata;

  if (dd->handle != INVALID_HANDLE_VALUE)
  {
    if (FindNextFile(dd->handle,&fdata/*&dd->ent*/))
    {
      rtv = 1;
      vm_string_strncpy_s(filename, nchars, fdata.cFileName/*dd->ent.cFileName*/, min(nchars,MAX_PATH));
    }
    else
      FindClose(dd->handle);

    dd->handle = INVALID_HANDLE_VALUE;
   }

  return rtv;
} /* vm_dir_read() */


void vm_dir_close(vm_dir *dd)
{
  if (dd->handle!=INVALID_HANDLE_VALUE)
  {
      FindClose(dd->handle);
      free(dd);
  }

  return;
} /* vm_dir_close() */


unsigned long long vm_dir_get_free_disk_space( void )
{
  ULARGE_INTEGER freebytes[3];

  GetDiskFreeSpaceEx(VM_STRING("."), &freebytes[0], &freebytes[1], &freebytes[2]);

  return freebytes[0].QuadPart; /* return total number of available free bytes on disk */
}

/*
 * fndfirst, findnext with correct (for VM) attributes
 */

static
unsigned int reverse_attribute(unsigned int attr)
{
   unsigned int rtv = 0;

   if ((attr & _A_SUBDIR) == _A_SUBDIR)
     rtv |= VM_FILE_ATTR_DIRECTORY;
   else
     rtv |= VM_FILE_ATTR_FILE;

   if ((attr & _A_HIDDEN) == _A_HIDDEN)
     rtv |= VM_FILE_ATTR_HIDDEN;

   return rtv;
} /* reverse_attribute() */


#define ADDNUM 16

vm_findptr vm_string_findfirst(vm_char* filespec, vm_finddata_t* fileinfo)
{
  vm_findptr pv = 0;
  size_t i;
  size_t tmppath_size;
  vm_char* tmppath;

  /* check file path and add wildcard specification if missed */
  i = vm_string_strlen(filespec);

  tmppath = (vm_char*)malloc((i+ADDNUM)*sizeof(vm_char));
  tmppath_size = (i+ADDNUM)*sizeof(vm_char);
  if (tmppath != NULL)
  {
    vm_string_strcpy_s(tmppath, tmppath_size, filespec);

    if (i > 1)
      --i;

    if (tmppath[i] != '*')
    {
      if (tmppath[i] != '\\')
        vm_string_strcat_s(tmppath, tmppath_size, VM_STRING("\\"));

      vm_string_strcat_s(tmppath, tmppath_size, VM_STRING("*"));
    }

    if ((pv = _tfindfirst(tmppath, fileinfo)) != 0)
      fileinfo[0].attrib = reverse_attribute(fileinfo[0].attrib);
    free(tmppath);
  }

  return pv;
} /* vm_string_findfirst() */


int32_t vm_string_findnext(vm_findptr handle, vm_finddata_t *fileinfo)
{
  int32_t rtv = 0;

  if ((rtv = _tfindnext(handle, fileinfo)) == 0)
    fileinfo[0].attrib = reverse_attribute(fileinfo[0].attrib);

  return rtv;
} /* vm_string_findnext() */

#endif /* Windows is not Unix */
