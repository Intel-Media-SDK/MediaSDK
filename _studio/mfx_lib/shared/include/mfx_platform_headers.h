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

#ifndef __MFX_PLATFORM_HEADERS_H__
#define __MFX_PLATFORM_HEADERS_H__

#include <mfxvideo++int.h>


#if (defined(LINUX32) || defined(LINUX64) )
        /* That's tricky: if LibVA will not be installed on the machine, you should be able
         * to build SW Media SDK and some apps in SW mode. Thus, va.h should not be visible.
         * Since we develop on machines where LibVA is installed, we forget about LibVA-free
         * build sometimes. So, that's the reminder why MFX_VA protection was added here.
         */
        #include <va/va.h>
        typedef VADisplay                       _mfxPlatformAccelerationService;
        typedef VASurfaceID                     _mfxPlatformVideoSurface;
#endif // #if (defined(LINUX32) || defined(LINUX64) )

#ifndef D3DDDIFORMAT
#define D3DDDIFORMAT        D3DFORMAT
#endif

static const GUID GUID_NULL =
{ 0x00000000, 0x0000, 0x0000,{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };

typedef int             BOOL;
typedef char            CHAR;
typedef unsigned char   BYTE;
typedef short           SHORT;
typedef int             INT;


typedef unsigned char  UCHAR;
typedef unsigned short USHORT;
typedef unsigned int   UINT;

typedef int            LONG;  // should be 32 bit to align with Windows
typedef unsigned int   ULONG;
typedef unsigned int   DWORD;

typedef unsigned long long UINT64;

#define FALSE               0
#define TRUE                1

typedef int D3DFORMAT;


#endif // __MFX_PLATFORM_HEADERS_H__
