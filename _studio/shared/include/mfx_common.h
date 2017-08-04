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

#ifndef _MFX_COMMON_H_
#define _MFX_COMMON_H_

#include "mfxvideo.h"
#include "mfxvideo++int.h"
#include "umc_defs.h"
#include "mfx_utils.h"
#include <stdio.h>
#include <string.h>

#include <string>
#include <stdexcept> /* for std exceptions on Linux/Android */

#include "mfx_config.h"

    #include <stddef.h>

#define MFX_BIT_IN_KB 8*1000

#define MFX_AUTO_ASYNC_DEPTH_VALUE  5
#define MFX_MAX_ASYNC_DEPTH_VALUE   15

#endif //_MFX_COMMON_H_
