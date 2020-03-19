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

#include "libmfx_allocator.h"

#include "mfxvideo++int.h"

#include "mfx_utils.h"
#include "mfx_common.h"

#define ALIGN32(X) (((mfxU32)((X)+31)) & (~ (mfxU32)31))
#define ID_BUFFER MFX_MAKEFOURCC('B','U','F','F')
#define ID_FRAME  MFX_MAKEFOURCC('F','R','M','E')

#define ERROR_STATUS(sts) ((sts)<MFX_ERR_NONE)

#define DEFAULT_ALIGNMENT_SIZE 64

// Implementation of Internal allocators
mfxStatus mfxDefaultAllocator::AllocBuffer(mfxHDL pthis, mfxU32 nbytes, mfxU16 type, mfxHDL *mid)
{
    if (!pthis)
        return MFX_ERR_INVALID_HANDLE;
    if(!mid)
        return MFX_ERR_NULL_PTR;
    mfxU32 header_size = ALIGN32(sizeof(BufferStruct));
    mfxU8 *buffer_ptr=(mfxU8 *)malloc(header_size + nbytes + DEFAULT_ALIGNMENT_SIZE);

    if (!buffer_ptr)
        return MFX_ERR_MEMORY_ALLOC;

    memset(buffer_ptr, 0, header_size + nbytes);

    BufferStruct *bs=(BufferStruct *)buffer_ptr;
    bs->allocator = pthis;
    bs->id = ID_BUFFER;
    bs->type = type;
    bs->nbytes = nbytes;

    // save index
    {
        mfxWideBufferAllocator* pBA = (mfxWideBufferAllocator*)pthis;
        pBA->m_bufHdl.push_back(bs);
        *mid = (mfxHDL) pBA->m_bufHdl.size();
    }

    return MFX_ERR_NONE;

}

inline
size_t midToSizeT(mfxHDL mid)
{
    return ((uint8_t *) mid - (uint8_t *) 0);

} // size_t midToSizeT(mfxHDL mid)

mfxStatus mfxDefaultAllocator::LockBuffer(mfxHDL pthis, mfxHDL mid, mfxU8 **ptr)
{
    //BufferStruct *bs=(BufferStruct *)mid;
    BufferStruct *bs;
    try
    {
        size_t index = midToSizeT(mid);
        if (!pthis)
            return MFX_ERR_INVALID_HANDLE;

        mfxWideBufferAllocator* pBA = (mfxWideBufferAllocator*)pthis;
        if ((index > pBA->m_bufHdl.size())||
            (index == 0))
            return MFX_ERR_INVALID_HANDLE;
        bs = pBA->m_bufHdl[index - 1];
    }
    catch (...)
    {
        return MFX_ERR_INVALID_HANDLE;
    }

    if (ptr) *ptr = UMC::align_pointer<mfxU8*>((mfxU8 *)bs + ALIGN32(sizeof(BufferStruct)), DEFAULT_ALIGNMENT_SIZE);

    return MFX_ERR_NONE;
}
mfxStatus mfxDefaultAllocator::UnlockBuffer(mfxHDL pthis, mfxHDL mid)
{
    try
    {
        if (!pthis)
            return MFX_ERR_INVALID_HANDLE;

        BufferStruct *bs;
        size_t index = midToSizeT(mid);
        mfxWideBufferAllocator* pBA = (mfxWideBufferAllocator*)pthis;
        if (index > pBA->m_bufHdl.size())
            return MFX_ERR_INVALID_HANDLE;

        bs = pBA->m_bufHdl[index - 1];
        if (bs->id!=ID_BUFFER)
            return MFX_ERR_INVALID_HANDLE;
    }
    catch (...)
    {
        return MFX_ERR_INVALID_HANDLE;
    }

    return MFX_ERR_NONE;
}
mfxStatus mfxDefaultAllocator::FreeBuffer(mfxHDL pthis, mfxMemId mid)
{
    try
    {
        if (!pthis)
            return MFX_ERR_INVALID_HANDLE;
        BufferStruct *bs;
        size_t index = midToSizeT(mid);
        mfxWideBufferAllocator* pBA = (mfxWideBufferAllocator*)pthis;
        if (index > pBA->m_bufHdl.size())
            return MFX_ERR_INVALID_HANDLE;

        bs = pBA->m_bufHdl[index - 1];
        if (bs->id!=ID_BUFFER)
            return MFX_ERR_INVALID_HANDLE;
        if (bs->id!=ID_BUFFER)
            return MFX_ERR_INVALID_HANDLE;
        free(bs);
        return MFX_ERR_NONE;
    }
    catch (...)
    {
        return MFX_ERR_INVALID_HANDLE;
    }
}

mfxStatus mfxDefaultAllocator::AllocFrames(mfxHDL pthis, mfxFrameAllocRequest *request, mfxFrameAllocResponse *response)
{
    if (!pthis)
        return MFX_ERR_INVALID_HANDLE;

    mfxU32 numAllocated, maxNumFrames;
    mfxWideSWFrameAllocator *pSelf = (mfxWideSWFrameAllocator*)pthis;

    // frames were allocated
    // return existent frames
    if (pSelf->NumFrames)
    {
        if (request->NumFrameSuggested > pSelf->NumFrames)
            return MFX_ERR_MEMORY_ALLOC;
        else
        {
            response->mids = &pSelf->m_frameHandles[0];
            return MFX_ERR_NONE;
        }
    }

    mfxU32 Pitch=ALIGN32(request->Info.Width);
    mfxU32 Height2=ALIGN32(request->Info.Height);
    mfxU32 nbytes;
    // Decoders and Encoders use YV12 and NV12 only
    switch (request->Info.FourCC) {
    case MFX_FOURCC_YV12:
        nbytes=Pitch*Height2 + (Pitch>>1)*(Height2>>1) + (Pitch>>1)*(Height2>>1);
        break;
    case MFX_FOURCC_NV12:
        nbytes=Pitch*Height2 + (Pitch>>1)*(Height2>>1) + (Pitch>>1)*(Height2>>1);
        break;
    case MFX_FOURCC_P010:
#if (MFX_VERSION >= 1031)
    case MFX_FOURCC_P016:
#endif
        Pitch=ALIGN32(request->Info.Width*2);
        nbytes=Pitch*Height2 + (Pitch>>1)*(Height2>>1) + (Pitch>>1)*(Height2>>1);
        break;
    case MFX_FOURCC_P210:
        Pitch=ALIGN32(request->Info.Width*2);
        nbytes=Pitch*Height2 + (Pitch>>1)*(Height2) + (Pitch>>1)*(Height2);
        break;
    case MFX_FOURCC_YUY2:
        nbytes=Pitch*Height2 + (Pitch>>1)*(Height2) + (Pitch>>1)*(Height2);
        break;
    case MFX_FOURCC_RGB3:
#ifdef MFX_ENABLE_RGBP
    case MFX_FOURCC_RGBP:
#endif
        if ((request->Type & MFX_MEMTYPE_FROM_VPPIN) ||
            (request->Type & MFX_MEMTYPE_FROM_VPPOUT) )
        {
            nbytes = Pitch*Height2 + Pitch*Height2 + Pitch*Height2;
            break;
        }
        else
            return MFX_ERR_UNSUPPORTED;
#if defined (MFX_ENABLE_FOURCC_RGB565)
    case MFX_FOURCC_RGB565:
        nbytes = 2*Pitch*Height2;
        break;
#endif // MFX_ENABLE_FOURCC_RGB565
    case MFX_FOURCC_BGR4:
    case MFX_FOURCC_RGB4:
    case MFX_FOURCC_AYUV:
        nbytes = Pitch*Height2 + Pitch*Height2 + Pitch*Height2 + Pitch*Height2;
        break;
    case MFX_FOURCC_A2RGB10:
        nbytes = Pitch*Height2 + Pitch*Height2 + Pitch*Height2 + Pitch*Height2;
        break;
    case MFX_FOURCC_IMC3:
        if ((request->Type & MFX_MEMTYPE_FROM_VPPIN) ||
            (request->Type & MFX_MEMTYPE_FROM_VPPOUT) ||
            (request->Type & MFX_MEMTYPE_FROM_DECODE))
        {
            nbytes = Pitch*Height2 + (Pitch)*(Height2>>1) + (Pitch)*(Height2>>1);
            break;
        }
        else
            return MFX_ERR_UNSUPPORTED;
        break;

    case MFX_FOURCC_P8:
        if ( request->Type & MFX_MEMTYPE_FROM_ENCODE )
        {
            nbytes = Pitch*Height2;
            break;
        }
        else
            return MFX_ERR_UNSUPPORTED;
#if (MFX_VERSION >= 1027)
    case MFX_FOURCC_Y210:
#if (MFX_VERSION >= 1031)
    case MFX_FOURCC_Y216:
#endif
        Pitch=ALIGN32(request->Info.Width*2);
        nbytes=Pitch*Height2 + (Pitch>>1)*(Height2) + (Pitch>>1)*(Height2);
        break;

    case MFX_FOURCC_Y410:
        Pitch=ALIGN32(request->Info.Width*4);
        nbytes=Pitch*Height2;
        break;
#endif

#if (MFX_VERSION >= 1031)
    case MFX_FOURCC_Y416:
        Pitch=ALIGN32(request->Info.Width*8);
        nbytes=Pitch*Height2;
        break;
#endif

    default:
        return MFX_ERR_UNSUPPORTED;
    }

    // allocate frames in cycle
    maxNumFrames = request->NumFrameSuggested;
    pSelf->m_frameHandles.resize(request->NumFrameSuggested);
    for (numAllocated = 0; numAllocated < maxNumFrames; numAllocated += 1)
    {
        mfxStatus sts = (pSelf->wbufferAllocator.bufferAllocator.Alloc)(pSelf->wbufferAllocator.bufferAllocator.pthis, nbytes + ALIGN32(sizeof(FrameStruct)), request->Type, &pSelf->m_frameHandles[numAllocated]);
        if (ERROR_STATUS(sts)) break;

        FrameStruct *fs;
        sts = (pSelf->wbufferAllocator.bufferAllocator.Lock)(pSelf->wbufferAllocator.bufferAllocator.pthis, pSelf->m_frameHandles[numAllocated], (mfxU8 **)&fs);
        if (ERROR_STATUS(sts)) break;
        fs->id = ID_FRAME;
        fs->info = request->Info;
        (pSelf->wbufferAllocator.bufferAllocator.Unlock)(pSelf->wbufferAllocator.bufferAllocator.pthis, pSelf->m_frameHandles[numAllocated]);
    }
    response->mids = &pSelf->m_frameHandles[0];
    response->NumFrameActual = (mfxU16) numAllocated;

    // check the number of allocated frames
    if (numAllocated < request->NumFrameMin)
    {
        FreeFrames(pSelf, response);
        return MFX_ERR_MEMORY_ALLOC;
    }
    pSelf->NumFrames = maxNumFrames;

    return MFX_ERR_NONE;

}


mfxStatus mfxDefaultAllocator::LockFrame(mfxHDL pthis, mfxHDL mid, mfxFrameData *ptr)
{
    if (!pthis)
        return MFX_ERR_INVALID_HANDLE;

    mfxWideSWFrameAllocator *pSelf = (mfxWideSWFrameAllocator*)pthis;

    // The default LockFrame is to simulate using LockBuffer.
    FrameStruct *fs;
    mfxStatus sts = (pSelf->wbufferAllocator.bufferAllocator.Lock)(pSelf->wbufferAllocator.bufferAllocator.pthis, mid,(mfxU8 **)&fs);
    if (ERROR_STATUS(sts)) return sts;
    if (fs->id!=ID_FRAME)
    {
        (pSelf->wbufferAllocator.bufferAllocator.Unlock)(pSelf->wbufferAllocator.bufferAllocator.pthis, mid);
        return MFX_ERR_INVALID_HANDLE;
    }

    //ptr->MemId = mid; !!!!!!!!!!!!!!!!!!!!!!!!!
    mfxU32 Height2=ALIGN32(fs->info.Height);
    mfxU8 *sptr = (mfxU8 *)fs+ALIGN32(sizeof(FrameStruct));
    switch (fs->info.FourCC) {
    case MFX_FOURCC_NV12:
        ptr->PitchHigh=0;
        ptr->PitchLow=(mfxU16)ALIGN32(fs->info.Width);
        ptr->Y = sptr;
        ptr->U = ptr->Y + ptr->Pitch*Height2;
        ptr->V = ptr->U + 1;
        break;
    case MFX_FOURCC_P010:
#if (MFX_VERSION >= 1031)
    case MFX_FOURCC_P016:
#endif
        ptr->PitchHigh=0;
        ptr->PitchLow=(mfxU16)ALIGN32(fs->info.Width*2);
        ptr->Y = sptr;
        ptr->U = ptr->Y + ptr->Pitch*Height2;
        ptr->V = ptr->U + 2;
        break;
    case MFX_FOURCC_P210:
        ptr->PitchHigh=0;
        ptr->PitchLow=(mfxU16)ALIGN32(fs->info.Width*2);
        ptr->Y = sptr;
        ptr->U = ptr->Y + ptr->Pitch*Height2;
        ptr->V = ptr->U + 2;
        break;
    case MFX_FOURCC_YV12:
        ptr->PitchHigh=0;
        ptr->PitchLow=(mfxU16)ALIGN32(fs->info.Width);
        ptr->Y = sptr;
        ptr->V = ptr->Y + ptr->Pitch*Height2;
        ptr->U = ptr->V + (ptr->Pitch>>1)*(Height2>>1);
        break;
    case MFX_FOURCC_YUY2:
        ptr->Y = sptr;
        ptr->U = ptr->Y + 1;
        ptr->V = ptr->Y + 3;
        ptr->PitchHigh = (mfxU16)((2*ALIGN32(fs->info.Width)) / (1 << 16));
        ptr->PitchLow  = (mfxU16)((2*ALIGN32(fs->info.Width)) % (1 << 16));
        break;
#if defined (MFX_ENABLE_FOURCC_RGB565)
    case MFX_FOURCC_RGB565:
        ptr->B = sptr;
        ptr->G = ptr->B;
        ptr->R = ptr->B;
        ptr->PitchHigh = (mfxU16)((2*ALIGN32(fs->info.Width)) >> 16);
        ptr->PitchLow  = (mfxU16)((2*ALIGN32(fs->info.Width)) & 0xffff);
        break;
#endif // MFX_ENABLE_FOURCC_RGB565
    case MFX_FOURCC_RGB3:
        ptr->B = sptr;
        ptr->G = ptr->B + 1;
        ptr->R = ptr->B + 2;
        ptr->PitchHigh = (mfxU16)((3*ALIGN32(fs->info.Width)) / (1 << 16));
        ptr->PitchLow  = (mfxU16)((3*ALIGN32(fs->info.Width)) % (1 << 16));
        break;
#ifdef MFX_ENABLE_RGBP
    case MFX_FOURCC_RGBP:
        ptr->B = sptr;
        ptr->G = ptr->B + ptr->Pitch*Height2;
        ptr->R = ptr->B + 2*ptr->Pitch*Height2;;
        ptr->PitchHigh = (mfxU16)((3*ALIGN32(fs->info.Width)) / (1 << 16));
        ptr->PitchLow  = (mfxU16)((3*ALIGN32(fs->info.Width)) % (1 << 16));
        break;
#endif
    case MFX_FOURCC_RGB4:
        ptr->B = sptr;
        ptr->G = ptr->B + 1;
        ptr->R = ptr->B + 2;
        ptr->A = ptr->B + 3;
        ptr->PitchHigh = (mfxU16)((4*ALIGN32(fs->info.Width)) / (1 << 16));
        ptr->PitchLow  = (mfxU16)((4*ALIGN32(fs->info.Width)) % (1 << 16));
        break;
    case MFX_FOURCC_BGR4:
        ptr->R = sptr;
        ptr->G = ptr->R + 1;
        ptr->B = ptr->R + 2;
        ptr->A = ptr->R + 3;
        ptr->PitchHigh = (mfxU16)((4 * ALIGN32(fs->info.Width)) / (1 << 16));
        ptr->PitchLow = (mfxU16)((4 * ALIGN32(fs->info.Width)) % (1 << 16));
        break;
    case MFX_FOURCC_A2RGB10:
        ptr->R = ptr->G = ptr->B = ptr->A = sptr;
        ptr->PitchHigh = (mfxU16)((4*ALIGN32(fs->info.Width)) / (1 << 16));
        ptr->PitchLow  = (mfxU16)((4*ALIGN32(fs->info.Width)) % (1 << 16));
        break;
    case MFX_FOURCC_P8:
        ptr->PitchHigh=0;
        ptr->PitchLow=(mfxU16)ALIGN32(fs->info.Width);
        ptr->Y = sptr;
        ptr->U = 0;
        ptr->V = 0;
        break;
    case MFX_FOURCC_AYUV:
        ptr->PitchHigh = (mfxU16)((4 * ALIGN32(fs->info.Width)) / (1 << 16));
        ptr->PitchLow  = (mfxU16)((4 * ALIGN32(fs->info.Width)) % (1 << 16));
        ptr->V = sptr;
        ptr->U = ptr->V + 1;
        ptr->Y = ptr->V + 2;
        ptr->A = ptr->V + 3;
        break;
#if (MFX_VERSION >= 1027)
    case MFX_FOURCC_Y210:
#if (MFX_VERSION >= 1031)
    case MFX_FOURCC_Y216:
#endif
        ptr->PitchHigh = (mfxU16)((4 * ALIGN32(fs->info.Width)) / (1 << 16));
        ptr->PitchLow  = (mfxU16)((4 * ALIGN32(fs->info.Width)) % (1 << 16));
        ptr->Y16 = (mfxU16*)sptr;
        ptr->U16 = ptr->Y16 + 1;
        ptr->V16 = ptr->Y16 + 3;
        break;

    case MFX_FOURCC_Y410:
        ptr->PitchHigh = (mfxU16)((4 * ALIGN32(fs->info.Width)) / (1 << 16));
        ptr->PitchLow  = (mfxU16)((4 * ALIGN32(fs->info.Width)) % (1 << 16));
        ptr->Y = ptr->U = ptr->V = ptr->A = 0;
        ptr->Y410 = (mfxY410*)sptr;
        break;
#endif

#if (MFX_VERSION >= 1031)
    case MFX_FOURCC_Y416:
        ptr->PitchHigh = (mfxU16)(8 * ALIGN32(fs->info.Width) / (1 << 16));
        ptr->PitchLow  = (mfxU16)(8 * ALIGN32(fs->info.Width) % (1 << 16));
        ptr->U16 = (mfxU16*)sptr;
        ptr->Y16 = ptr->U16 + 1;
        ptr->V16 = ptr->Y16 + 1;
        ptr->A   = (mfxU8 *)(ptr->V16 + 1);
        break;
#endif

    default:
        return MFX_ERR_UNSUPPORTED;
    }
    return sts;

}
mfxStatus mfxDefaultAllocator::GetHDL(mfxHDL pthis, mfxMemId mid, mfxHDL *handle)
{
    if (!pthis)
        return MFX_ERR_INVALID_HANDLE;

    *handle = mid;
    return MFX_ERR_NONE;
}

mfxStatus mfxDefaultAllocator::UnlockFrame(mfxHDL pthis, mfxHDL mid, mfxFrameData *ptr)
{
    if (!pthis)
        return MFX_ERR_INVALID_HANDLE;

    mfxWideSWFrameAllocator *pSelf = (mfxWideSWFrameAllocator*)pthis;

    // The default UnlockFrame behavior is to simulate using UnlockBuffer
    mfxStatus sts = (pSelf->wbufferAllocator.bufferAllocator.Unlock)(pSelf->wbufferAllocator.bufferAllocator.pthis, mid);
    if (ERROR_STATUS(sts)) return sts;
    if (ptr) {
        ptr->PitchHigh=0;
        ptr->PitchLow=0;
        ptr->U=ptr->V=ptr->Y=0;
        ptr->A=ptr->R=ptr->G=ptr->B=0;
    }
    return sts;

} // mfxStatus SWVideoCORE::UnlockFrame(mfxHDL mid, mfxFrameData *ptr)
mfxStatus mfxDefaultAllocator::FreeFrames(mfxHDL pthis, mfxFrameAllocResponse *response)
{
    if (!pthis)
        return MFX_ERR_INVALID_HANDLE;

    mfxWideSWFrameAllocator *pSelf = (mfxWideSWFrameAllocator*)pthis;
    mfxU32 i;
    // free all allocated frames in cycle
    for (i = 0; i < response->NumFrameActual; i += 1)
    {
        if (response->mids[i])
        {
            (pSelf->wbufferAllocator.bufferAllocator.Free)(pSelf->wbufferAllocator.bufferAllocator.pthis, response->mids[i]);
        }
    }

    pSelf->m_frameHandles.clear();

    return MFX_ERR_NONE;

} // mfxStatus SWVideoCORE::FreeFrames(void)

mfxWideBufferAllocator::mfxWideBufferAllocator()
{
    memset(bufferAllocator.reserved, 0, sizeof(bufferAllocator.reserved));

    bufferAllocator.Alloc = &mfxDefaultAllocator::AllocBuffer;
    bufferAllocator.Lock =  &mfxDefaultAllocator::LockBuffer;
    bufferAllocator.Unlock = &mfxDefaultAllocator::UnlockBuffer;
    bufferAllocator.Free = &mfxDefaultAllocator::FreeBuffer;

    bufferAllocator.pthis = 0;
}

mfxWideBufferAllocator::~mfxWideBufferAllocator()
{
    memset((void*)&bufferAllocator, 0, sizeof(mfxBufferAllocator));
}
mfxBaseWideFrameAllocator::mfxBaseWideFrameAllocator(mfxU16 type)
    : NumFrames(0)
    , type(type)
{
    memset((void*)&frameAllocator, 0, sizeof(frameAllocator));

}
mfxBaseWideFrameAllocator::~mfxBaseWideFrameAllocator()
{
    memset((void*)&frameAllocator, 0, sizeof(mfxFrameAllocator));
}
mfxWideSWFrameAllocator::mfxWideSWFrameAllocator(mfxU16 type):mfxBaseWideFrameAllocator(type)
{
    frameAllocator.Alloc = &mfxDefaultAllocator::AllocFrames;
    frameAllocator.Lock = &mfxDefaultAllocator::LockFrame;
    frameAllocator.GetHDL = &mfxDefaultAllocator::GetHDL;
    frameAllocator.Unlock = &mfxDefaultAllocator::UnlockFrame;
    frameAllocator.Free = &mfxDefaultAllocator::FreeFrames;
}




