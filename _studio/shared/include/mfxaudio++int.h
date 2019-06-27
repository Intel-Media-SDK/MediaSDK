// Copyright (c) 2017-2019 Intel Corporation
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

#ifndef __MFXAUDIOPLUSPLUS_INTERNAL_H
#define __MFXAUDIOPLUSPLUS_INTERNAL_H

#include "mfxaudio.h"
#include <mfx_task_threading_policy.h>
#include <mfx_interface.h>



#ifndef GUID_TYPE_DEFINED

#include <string.h>

typedef struct {
    unsigned long  Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[8];
} GUID;

static int operator==(const GUID & l, const GUID & r)
{
    return MFX_EQ_FIELD(Data1) && MFX_EQ_FIELD(Data2) && MFX_EQ_FIELD(Data3) && MFX_EQ_ARRAY(Data4, 8);
}

#define GUID_TYPE_DEFINED
#endif


enum eMFXAudioPlatform
{
    MFX_AUDIO_PLATFORM_SOFTWARE      = 0,
    MFX_AUDIO_PLATFORM_HARDWARE      = 1,
};

// Forward declaration of used classes
struct MFX_ENTRY_POINT;

// AudioCORE
class AudioCORE {
public:

    virtual ~AudioCORE(void) {};

    // imported to external API
    virtual mfxStatus GetHandle(mfxHandleType type, mfxHDL *handle) = 0;
    virtual mfxStatus SetHandle(mfxHandleType type, mfxHDL handle) = 0;
    virtual mfxStatus SetBufferAllocator(mfxBufferAllocator *allocator) = 0;
    virtual mfxStatus SetFrameAllocator(mfxFrameAllocator *allocator) = 0;

    // Internal interface only
    // Utility functions for memory access
    virtual mfxStatus  AllocBuffer(mfxU32 nbytes, mfxU16 type, mfxMemId *mid) = 0;
    virtual mfxStatus  LockBuffer(mfxMemId mid, mfxU8 **ptr) = 0;
    virtual mfxStatus  UnlockBuffer(mfxMemId mid) = 0;
    virtual mfxStatus  FreeBuffer(mfxMemId mid) = 0;



    virtual mfxStatus  AllocFrames(mfxFrameAllocRequest *request,
                                   mfxFrameAllocResponse *response) = 0;

    virtual mfxStatus  AllocFrames(mfxFrameAllocRequest *request,
                                   mfxFrameAllocResponse *response,
                                   mfxFrameSurface1 **pOpaqueSurface,
                                   mfxU32 NumOpaqueSurface) = 0;

    virtual mfxStatus  LockFrame(mfxMemId mid, mfxFrameData *ptr) = 0;
    virtual mfxStatus  UnlockFrame(mfxMemId mid, mfxFrameData *ptr=0) = 0;
    virtual mfxStatus  FreeFrames(mfxFrameAllocResponse *response, bool ExtendedSearch = true) = 0;

    virtual mfxStatus  LockExternalFrame(mfxMemId mid, mfxFrameData *ptr, bool ExtendedSearch = true) = 0;
    virtual mfxStatus  UnlockExternalFrame(mfxMemId mid, mfxFrameData *ptr=0, bool ExtendedSearch = true) = 0;

    virtual mfxMemId MapIdx(mfxMemId mid) = 0;


    // Increment Surface lock
    virtual mfxStatus  IncreaseReference(mfxFrameData *ptr, bool ExtendedSearch = true) = 0;
    // Decrement Surface lock
    virtual mfxStatus  DecreaseReference(mfxFrameData *ptr, bool ExtendedSearch = true) = 0;

    // no care about surface, opaq and all round. Just increasing reference
    virtual mfxStatus IncreasePureReference(mfxU16 &) = 0;
    // no care about surface, opaq and all round. Just decreasing reference
    virtual mfxStatus DecreasePureReference(mfxU16 &) = 0;
    virtual void* QueryCoreInterface(const MFX_GUID &guid) = 0;
    virtual mfxSession GetSession() = 0;
};


class AudioENCODE
{
public:
    // Destructor
    virtual
    ~AudioENCODE(void) {}

    virtual
    mfxStatus Init(mfxAudioParam *par) = 0;
    virtual
    mfxStatus Reset(mfxAudioParam *par) = 0;
    virtual
    mfxStatus Close(void) = 0;
    virtual
    mfxTaskThreadingPolicy GetThreadingPolicy(void) {return MFX_TASK_THREADING_DEFAULT;}

    virtual
    mfxStatus GetAudioParam(mfxAudioParam *par) = 0;
    virtual
    mfxStatus EncodeFrameCheck(mfxAudioFrame *aFrame, mfxBitstream *buffer_out) = 0;
    virtual
    mfxStatus EncodeFrame(mfxAudioFrame *bs, mfxBitstream *buffer_out) = 0;

    virtual mfxStatus EncodeFrameCheck(mfxAudioFrame *aFrame, mfxBitstream *buffer_out,
        MFX_ENTRY_POINT *pEntryPoint)  = 0;

};

class AudioDECODE
{
public:
    // Destructor
    virtual
    ~AudioDECODE(void) {}

    virtual
    mfxStatus Init(mfxAudioParam *par) = 0;
    virtual
    mfxStatus Reset(mfxAudioParam *par) = 0;
    virtual
    mfxStatus Close(void) = 0;
    virtual
    mfxTaskThreadingPolicy GetThreadingPolicy(void) {return MFX_TASK_THREADING_DEFAULT;}

    virtual
    mfxStatus GetAudioParam(mfxAudioParam *par) = 0;
    virtual
    mfxStatus DecodeFrameCheck(mfxBitstream *bs,  mfxAudioFrame *aFrame,
        MFX_ENTRY_POINT *pEntryPoint)
    {
        (void)pEntryPoint;

        return DecodeFrameCheck(bs, aFrame);
    }
    virtual
    mfxStatus DecodeFrameCheck(mfxBitstream *bs, mfxAudioFrame *aFrame) = 0;
    virtual
    mfxStatus DecodeFrame(mfxBitstream *bs, mfxAudioFrame *buffer_out) = 0;
 protected:



};


struct ThreadAudioDecodeTaskInfo
{
    mfxAudioFrame         *out;
    mfxU32                 taskID; // for task ordering
};

struct ThreadAudioEncodeTaskInfo
{
    mfxAudioFrame          *in;
    mfxBitstream           *out;
    mfxU32                 taskID; // for task ordering
};

#endif // __MFXAUDIOPLUSPLUS_INTERNAL_H
