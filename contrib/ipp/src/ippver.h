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

/*
//
//  Purpose: Describes the base IPP version
//
*/

#include "ippversion.h"
#ifndef BASE_VERSION
#define BASE_VERSION() IPP_VERSION_MAJOR,IPP_VERSION_MINOR,IPP_VERSION_UPDATE
#endif

#ifndef STR_VERSION
 #ifdef IPP_REVISION
  #define STR_VERSION() STR(IPP_VERSION_MAJOR) "." STR(IPP_VERSION_MINOR) "." STR(IPP_VERSION_UPDATE) " (r" STR( IPP_REVISION ) ")"
 #else
  #define STR_VERSION() STR(IPP_VERSION_MAJOR) "." STR(IPP_VERSION_MINOR) "." STR(IPP_VERSION_UPDATE) " (-)"
 #endif
#endif


/* ////////////////////////////// End of file /////////////////////////////// */
