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

#ifndef __BS_READER_H
#define __BS_READER_H

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <bs_def.h>
#include <new>

#define BS_BUF_SIZE 2048
#if defined(__APPLE__) || defined(LINUX32) || defined(LINUX64) || defined(ANDROID)
    #define BS_FILE_MODE 0 /* no fteelo64 on OSX */
#else
    #define BS_FILE_MODE 1
#endif

#if (BS_FILE_MODE == 1)
  #define BS_FADDR_TYPE Bs64u
  #define BS_FOFFSET_TYPE Bs64s
  #define BS_TRACE_FORMAT "0x%016I64X[%i]: %s = %i\n"
  #define BS_TRACE_OFFSET_FORMAT "0x%016I64X[%i]: "
 #ifdef __GNUC__
  #define _FILE_OFFSET_BITS 64
  #define BS_FTELL(fh)  ftello64(fh)
  #define BS_FSEEK(fh,offset,origin) fseeko64( fh, offset, origin )
 #else
  #include <share.h>
  #include <fcntl.h>
  #include <io.h>
  #define BS_FH_TYPE int
  #define BS_FOPEN(file, fh) { int tmp_fh = 0; if(!_sopen_s(&tmp_fh, file_name, _O_BINARY|_O_RDONLY, _SH_DENYNO, 0))fh = tmp_fh;}
  #define BS_FCLOSE(fh) _close(fh)
  #define BS_FTELL(fh)  _telli64(fh)
  #define BS_FEOF(fh)   _eof(fh)
  #define BS_FREAD(fh,dst,size) _read(fh, dst, size)
  #define BS_FSEEK(fh,offset,origin) _lseeki64( fh, offset, origin )
 #endif
#endif
#if (BS_FILE_MODE == 0) || defined(__GNUC__)
 #if (BS_FILE_MODE == 0)
  #define BS_FADDR_TYPE Bs32u
  #define BS_FOFFSET_TYPE Bs32s
  #define BS_TRACE_FORMAT "0x%08X[%i]: %s = %i\n"
  #define BS_TRACE_OFFSET_FORMAT "0x%08X[%i]: "
  #define BS_FTELL(fh)  ftell(fh)
  #define BS_FSEEK(fh,offset,origin) fseek( fh, offset, origin )
 #endif
  #define BS_FH_TYPE FILE*
  #define BS_FOPEN(file, fh) {fh = fopen( file_name, "rb" );}
  #define BS_FCLOSE(fh) fclose(fh)
  #define BS_FEOF(fh)   feof(fh)
  #define BS_FREAD(fh,dst,size) fread( dst, 1, size, fh )
#endif

#ifdef __BS_TRACE__
  #define BS_TRACE( val, comment ) { \
  if(trace_flag) {printf( BS_TRACE_FORMAT, file_pos+offset, bit, #comment, (int)(val) ); fflush(stdout);}\
    else (val); \
    if(last_err) return last_err; \
  }
  #define BS_SET( val, var ) { \
  if(trace_flag){ printf( BS_TRACE_FORMAT, file_pos+offset, bit, #var, (int)((var) = (val)) ); fflush(stdout);} \
    else (var) = (val); \
    if(last_err) return last_err; \
  }
  #define BS_GET_BITS( n, var ) { \
    (var) = (tmp_buf)>>(32-(n)); tmp_buf <<= (n);\
    if(trace_flag) printf( BS_TRACE_FORMAT, file_pos+offset, bit, #var, (var) ); \
  }
  #define BS_TRACE_BITS( n, var ) { \
    if(trace_flag) printf( BS_TRACE_FORMAT, file_pos+offset, bit, #var, ((tmp_buf)>>(32-(n))) ); \
    tmp_buf <<= n;\
  }
  #define BS_TROX( x, b ) if(trace_flag) printf(BS_TRACE_OFFSET_FORMAT, (unsigned int)(x), (b));
  #define BS_TRO if(trace_flag) printf(BS_TRACE_OFFSET_FORMAT,file_pos+offset, bit);
  #define BS_SETM( val, var, map ) {\
    if(trace_flag) {BS_TRO; (var) = (val); printf("%s = %s (%d)\n", #var, map[var], var);}\
    else (var) = (val);\
    if(last_err) return last_err; \
  }
  #define BS_TRACE_ARR(name, el, sz) \
  if(trace_flag){\
    printf("%s = { ", #name);\
    for(Bs32u _idx = 0; _idx < (Bs32u)(sz); _idx++ ) {\
        printf("%d ", (el));\
    }\
    printf("}\n");\
  } else for(Bs32u _idx = 0; _idx < (Bs32u)(sz); _idx++ ) { (el); }
  #define BS_TRACE_NO_OFFSET( val, comment ) { \
    if(trace_flag) printf( "%s = %d\n",  #comment, (val) ); \
    else (val); \
    if(last_err) return last_err; \
  }
  #define BS_TRACE_STR( str ) { if(trace_flag) printf( "%s\n",  str ); }
#else
  #define BS_TRACE( val, comment )  { (val); if(last_err) return last_err; }
  #define BS_SET( val, var )        { (var) = (val); if(last_err) return last_err; }
  #define BS_GET_BITS( n, var )     { (var) = (tmp_buf)>>(32-(n)); tmp_buf <<= (n);}
  #define BS_TRACE_BITS( n, var )   { tmp_buf <<= (n); }
  #define BS_TROX( x, b )           { (x); (b); };
  #define BS_TRO                    {}
  #define BS_SETM( val, var, map )  BS_SET( val, var )
  #define BS_TRACE_ARR(name, el, sz)for(Bs32u _idx = 0; _idx < (Bs32u)(sz); _idx++ ) { (el); }
  #define BS_TRACE_NO_OFFSET( val, comment ) BS_TRACE( val, comment )
  #define BS_TRACE_STR( str )       {}
#endif

#define BS_STRUCT_ALLOC( type, var ){ \
    var = (type*)malloc( sizeof(type) ); \
    if(var){ \
        memset( var, 0, sizeof(type) ); \
    }else{ \
        return last_err = BS_ERR_MEM_ALLOC; \
    }; \
    /*printf("BS_STRUCT_ALLOC:\t0x%p ( %i ) \t %s : %i\n", var, sizeof(type), __FILE__, __LINE__ );*/\
}
#define BS_CALLOC( type, size, var ){ \
    var = (type*)calloc( (size), sizeof(type) ); \
    if(!var) return last_err = BS_ERR_MEM_ALLOC; \
    /*printf("BS_CALLOC:\t\t0x%p ( %i * %i ) \t %s : %i\n", var, sizeof(type), (size), __FILE__, __LINE__ );*/\
}
#define BS_FREE(p) {if(p){ free(p); p = NULL;}}
#define BSCE if(last_err) return last_err;

class BS_Reader{
public:
    BS_Reader(){
        last_err  = BS_ERR_NONE;
        bs        = NULL;
        buf_size  = 0;
        file_pos  = 0;
        offset    = 0;
        bit       = 0;
        buf       = NULL;
        ignore_bytes = (BS_FADDR_TYPE*)malloc(sizeof(BS_FADDR_TYPE));
        ignore_bytes_cnt  = 0;
        next_ignored_byte = NULL;
        tmp_buf     = 0;
        trace_flag  = true;
        trace_level = 0xFFFFFFFF;
    }

    BS_Reader(BS_Reader const&) = delete;
    BS_Reader& operator=(BS_Reader const&) = delete;

    virtual ~BS_Reader(){
        if( bs && buf ) free( buf );
        close();
        if( ignore_bytes ) free(ignore_bytes);
    }

    BSErr open( const char* file_name );
    void  set_buffer( byte* buf, Bs32u buf_size );

    inline BSErr   get_last_err(){ return last_err; };
    inline BS_FADDR_TYPE   get_cur_pos() { return file_pos+offset; };
    inline BSErr   close(){
        last_err = (bs) ? ( BS_FCLOSE(bs) ? BS_ERR_UNKNOWN : BS_ERR_NONE ) : BS_ERR_NONE;
        if(!last_err) bs = 0;
        return last_err;
    };
    inline void set_trace_level(Bs32u level) {trace_level = level;};

protected:
    Bs32u buf_size;
    Bs32u offset;
    byte  bit;
    BS_FADDR_TYPE file_pos;
    BSErr last_err;

    Bs32u  tmp_buf;
    bool   trace_flag;
    Bs32u  trace_level;

    inline void trace_on(Bs32u level) {trace_flag = !!(trace_level & level);};
    inline void trace_off() {trace_flag = (trace_level == 0xFFFFFFFF);};

    Bs32u  ue_v();
    Bs32s  se_v();
    Bs32u  read_bits( Bs32u nBits /*1..32*/ );
    Bs32u  next_bits( Bs32u nBits /*1..32*/ ); // keep current position
    Bs32u  next_bytes( byte nBytes /*1..4*/ );
    byte   read_1_bit();
    byte   read_1_byte();
    BSErr  read_arr(byte* arr, Bs32u size);

    void    skip_bytes( Bs32u nBytes );
    void    ignore_next_byte();
    BSErr   shift(long nBytes);
    BSErr   shift_forward( Bs32u nBytes );
    BSErr   shift_back( Bs32u nBytes );

    inline BSErr shift_forward64(Bs64u nBytes){
        while(nBytes){
            Bs32u n = (Bs32u)BS_MIN(nBytes,0xFFFFFFFF);
            shift_forward(n);BSCE;
            nBytes -= n;
        }
        return last_err;
    };

    inline bool byte_aligned()           { return bit ? false : true; };
    inline void skip_byte()              { shift_forward(1); };
    inline void set_first_ignored_byte() { next_ignored_byte = &ignore_bytes[0]; };
    inline void wipe_ignore_bytes()      { ignore_bytes_cnt = 0; next_ignored_byte = NULL; };
    Bs32u get_num_ignored_bytes(BS_FADDR_TYPE end){
        Bs32u n = 0;
        BS_FADDR_TYPE* b = next_ignored_byte;
        Bs32u c = 0;

        while(c < ignore_bytes_cnt && b[c++] < end++)
            n++;

        return n;
    }

private:
    BS_FH_TYPE bs; //file handle
    byte*  buf;
    BS_FADDR_TYPE* ignore_bytes;
    BS_FADDR_TYPE* next_ignored_byte;
    Bs32u  ignore_bytes_cnt;

    BSErr read_more_data();

};
#endif //__BS_READER_H
