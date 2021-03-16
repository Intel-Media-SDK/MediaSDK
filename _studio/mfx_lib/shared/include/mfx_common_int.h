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

#ifndef __MFX_COMMON_INT_H__
#define __MFX_COMMON_INT_H__

#include <vector>
#include <memory>
#include <errno.h>
#include "mfx_common.h"
#include "mfxaudio.h"


mfxStatus CheckFrameInfoCommon(mfxFrameInfo  *info, mfxU32 codecId);
mfxStatus CheckFrameInfoEncoders(mfxFrameInfo  *info);
mfxStatus CheckFrameInfoCodecs(mfxFrameInfo  *info, mfxU32 codecId = MFX_CODEC_AVC, bool isHW = false);

mfxStatus CheckVideoParamEncoders(mfxVideoParam *in, bool IsExternalFrameAllocator, eMFXHWType type);
mfxStatus CheckVideoParamDecoders(mfxVideoParam *in, bool IsExternalFrameAllocator, eMFXHWType type);

mfxStatus UpdateCscOutputFormat(mfxVideoParam *par, mfxFrameAllocRequest *request);

mfxStatus CheckAudioParamEncoders(mfxAudioParam *in);
mfxStatus CheckAudioParamCommon(mfxAudioParam *in);
mfxStatus CheckAudioParamDecoders(mfxAudioParam *in);

mfxStatus CheckBitstream(const mfxBitstream *bs);
mfxStatus CheckAudioFrame(const mfxAudioFrame *aFrame);
mfxStatus CheckEncryptedBitstream(const mfxBitstream *bs);
mfxStatus CheckFrameData(const mfxFrameSurface1 *surface);

mfxStatus CheckDecodersExtendedBuffers(mfxVideoParam const* par);

mfxStatus PackMfxFrameRate(mfxU32 nom, mfxU32 denom, mfxU32& packed); // fit u32 args to u16 and pack: (den << 16) | nom, returns MFX_WRN_VIDEO_PARAM_CHANGED if result differs

mfxExtBuffer* GetExtendedBuffer(mfxExtBuffer** extBuf, mfxU32 numExtBuf, mfxU32 id);
mfxExtBuffer* GetExtendedBufferInternal(mfxExtBuffer** extBuf, mfxU32 numExtBuf, mfxU32 id);

class ExtendedBuffer
{
public:

    ExtendedBuffer();
    virtual ~ExtendedBuffer();

    template<typename T> void AddTypedBuffer(mfxU32 id)
    {
        if (GetBufferByIdInternal(id))
            return;

        mfxExtBuffer * buffer = (mfxExtBuffer *)(new mfxU8[sizeof(T)]);
        memset(buffer, 0, sizeof(T));
        buffer->BufferSz = sizeof(T);
        buffer->BufferId = id;
        AddBufferInternal(buffer);
    }

    void AddBuffer(mfxExtBuffer * buffer);

    size_t GetCount() const;

    template<typename T> T * GetBufferById(mfxU32 id)
    {
        return (T*)GetBufferByIdInternal(id);
    }

    template<typename T> T * GetBufferByPosition(mfxU32 pos)
    {
        return (T*)GetBufferByPositionInternal(pos);
    }

    mfxExtBuffer ** GetBuffers();

private:

    void AddBufferInternal(mfxExtBuffer * buffer);

    mfxExtBuffer * GetBufferByIdInternal(mfxU32 id);

    mfxExtBuffer * GetBufferByPositionInternal(mfxU32 pos);

    void Release();

    typedef std::vector<mfxExtBuffer *>  BuffersList;
    BuffersList m_buffers;
};

class mfxVideoParamWrapper : public mfxVideoParam
{
public:

    mfxVideoParamWrapper();

    mfxVideoParamWrapper(const mfxVideoParam & par);

    virtual ~mfxVideoParamWrapper();

    mfxVideoParamWrapper & operator = (const mfxVideoParam & par);

    mfxVideoParamWrapper & operator = (const mfxVideoParamWrapper & par);

    bool CreateExtendedBuffer(mfxU32 bufferId);

    template<typename T> T * GetExtendedBuffer(mfxU32 id)
    {
        T * extBuf = m_buffers.GetBufferById<T>(id);

        if (!extBuf)
        {
            m_buffers.AddTypedBuffer<T>(id);
            extBuf = m_buffers.GetBufferById<T>(id);
            if (!extBuf)
                throw 1;
        }

        return extBuf;
    }

private:

    ExtendedBuffer m_buffers;
    mfxU8* m_mvcSequenceBuffer;

    void CopyVideoParam(const mfxVideoParam & par);

    // Deny copy constructor
    mfxVideoParamWrapper(const mfxVideoParamWrapper &);
};

mfxU8* GetFramePointer(mfxU32 fourcc, mfxFrameData const&);
mfxStatus GetFramePointerChecked(mfxFrameInfo const& info, mfxFrameData const&, mfxU8**);

#endif
