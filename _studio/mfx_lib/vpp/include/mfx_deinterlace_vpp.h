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

#include "mfx_common.h"

#if defined (MFX_ENABLE_VPP)

#ifndef __MFX_DEINTERLACE_VPP_H
#define __MFX_DEINTERLACE_VPP_H
#include "mfxvideo++int.h"

#include "mfx_vpp_defs.h"
#include "mfx_vpp_base.h"

#define VPP_DI_IN_NUM_FRAMES_REQUIRED  (3)
#define VPP_DI_OUT_NUM_FRAMES_REQUIRED (1)

#define VAR_ERR_SIZE                   (7)

/* MediaSDK has frame based logic
 * So, 30i means 30 interlaced frames
 * 30p means progressive frames.
 * Constants was corrected according to this logic*/
typedef enum {
    VPP_SIMPLE_DEINTERLACE   = 0x0001, //example: 30i -> 30p, default mode
    VPP_DEINTERLACE_MODE30i60p = 0x0002, //example: 30i -> 60p
    VPP_INVERSE_TELECINE     = 0x003   //example: 30i -> 24p

} mfxDIMode;

class MFXVideoVPPDeinterlace : public FilterVPP{
public:

  static mfxU8 GetInFramesCountExt( void ) { return VPP_DI_IN_NUM_FRAMES_REQUIRED; };
  static mfxU8 GetOutFramesCountExt(void) { return VPP_DI_OUT_NUM_FRAMES_REQUIRED; };

};


#endif // __MFX_DEINTERLACE_VPP_H

#endif // MFX_ENABLE_VPP
/* EOF */
