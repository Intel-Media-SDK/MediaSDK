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

#ifndef _MFX_PAK_EXT_H_
#define _MFX_PAK_EXT_H_



#include "mfxvideo++int.h"
#include "mfxpak.h"

class VideoPAK_Ext:  public VideoPAK
{
public:
    // Destructor
    virtual
    ~VideoPAK_Ext(void){}

    virtual
    mfxTaskThreadingPolicy GetThreadingPolicy(void) {return MFX_TASK_THREADING_DEFAULT;}

    virtual
    mfxStatus GetVideoParam(mfxVideoParam *par) = 0;
    virtual
    mfxStatus GetFrameParam(mfxFrameParam * /* par */)
    {
        return MFX_ERR_UNSUPPORTED;
    }

    virtual 
    mfxStatus  RunFramePAKCheck( mfxPAKInput *           /* in */,
                                    mfxPAKOutput *          /* out */,
                                    MFX_ENTRY_POINT*        /* pEntryPoints */)
    {
        return MFX_ERR_UNSUPPORTED;
    }

    virtual
    mfxStatus RunFramePAKCheck(  mfxPAKInput *  in,
                                    mfxPAKOutput *  out,
                                    MFX_ENTRY_POINT pEntryPoints[],
                                    mfxU32 & numEntryPoints) = 0;

    virtual
    mfxStatus RunFramePAK(mfxPAKInput * /* in */, mfxPAKOutput * /* out */)
    {
        return MFX_ERR_UNSUPPORTED;
    };

    virtual 
    void* GetDstForSync(MFX_ENTRY_POINT& /* pEntryPoints */) 
    {
        return NULL;
    }

    virtual 
    void* GetSrcForSync(MFX_ENTRY_POINT& /* pEntryPoints */) 
    {
         return NULL;   
    }

};

#endif


