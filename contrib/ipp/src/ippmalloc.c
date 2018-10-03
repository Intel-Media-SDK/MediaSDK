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

#include "precomp.h"

#include <stdlib.h>

#include "owndefs.h"


/* ////////////////////////////////////////////////////////////////////////// */
#define ALIGNED_MASK  (~(IPP_MALLOC_ALIGNED_BYTES-1))

#define ALLOC_ALIGNED_AND_RET( retType,lengthType,length )                             \
{                                                                                      \
    void *ptr = (void*)0;                                                              \
/*    IPP_BADARG_RET( (length<=0), NULL )*/                                            \
    if( length <= 0 ) return NULL;                                                     \
    ptr = malloc(sizeof(lengthType)*length + IPP_MALLOC_ALIGNED_BYTES + sizeof(void*));\
    if (ptr) {                                                                         \
        void **ap = (void**)( (IPP_UINT_PTR(ptr) + IPP_MALLOC_ALIGNED_BYTES +          \
                                      sizeof(void*) - 1 ) & ALIGNED_MASK );            \
        ap[-1] = (void*)ptr;                                                           \
        return (retType)ap;                                                            \
    }                                                                                  \
    return (retType)0;                                                                 \
}
/* ////////////////////////////////////////////////////////////////////////// */

/* /////////////////////////////////////////////////////////////////////////////
//  Name:       mfxMalloc
//  Purpose:    allocates aligned (32 bytes) arrays
//  Returns:    actual pointer, if no errors
//              NULL, if out of memory or if incorrect length
//  Parameter:
//    length  - sizeof in bytes
//
//  Notes:      You should release memory only by function mfxFree()
*/

IPPFUN( void*, mfxMalloc, (int length) )
{
    ALLOC_ALIGNED_AND_RET( void*, Ipp8u, length )
}


/* /////////////////////////////////////////////////////////////////////////////
//  Name:       mfxFree
//  Purpose:    frees memory allocated by mfx*_Malloc
//  Parameter:
//    ptr     - the pointer on memory, which allocated by mfx*_Malloc*
//
//  Notes: function correctly free memory, which allocated mfx*_Malloc*() only
*/
IPPFUN( void, mfxFree, (void* ptr) )
{
   if( ptr )free( ((void**)ptr)[-1] );
}

/* //////////////////////// End of file "ippmalloc.c" ///////////////////////// */
