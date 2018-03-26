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

#pragma once

#include "ts_common.h"

class tsReader : public mfxBitstream
{
private:
    FILE* m_file;
public:
    tsReader(mfxBitstream bs);
    tsReader(const char* fname);
    virtual ~tsReader();

    mfxU32 Read(mfxU8* dst, mfxU32 size);
    mfxStatus SeekToStart();
};

class tsBitstreamProcessor
{
public:
    virtual ~tsBitstreamProcessor() { }

    mfxBitstream* ProcessBitstream(mfxBitstream& bs);
    virtual mfxStatus ProcessBitstream(mfxBitstream& bs, mfxU32 nFrames){ return MFX_ERR_NONE; };
};

class tsBitstreamErazer : public tsBitstreamProcessor
{
public:
    tsBitstreamErazer() {};
    virtual ~tsBitstreamErazer() {};
    mfxStatus ProcessBitstream(mfxBitstream& bs, mfxU32 nFrames){ bs.DataLength = 0; return MFX_ERR_NONE; }
};

class tsBitstreamWriter : public tsBitstreamProcessor
{
private:
    FILE* m_file;
public:
    tsBitstreamWriter(const char* fname, bool append = false);
    virtual ~tsBitstreamWriter();
    mfxStatus ProcessBitstream(mfxBitstream& bs, mfxU32 nFrames);
};

inline mfxStatus WriteBitstream(const char* fname, mfxBitstream& bs, bool append = false)
{
    tsBitstreamWriter wr(fname, append);
    return wr.ProcessBitstream(bs, 1);
}

class tsBitstreamReader : public tsBitstreamProcessor, public tsReader
{
public:
    bool    m_eos;
    mfxU8*  m_buf;
    mfxU32  m_buf_size;

    tsBitstreamReader(const char* fname, mfxU32 buf_size);
    tsBitstreamReader(mfxBitstream bs, mfxU32 buf_size);
    virtual ~tsBitstreamReader();
    mfxStatus ProcessBitstream(mfxBitstream& bs, mfxU32 nFrames);
};

class tsBitstreamReaderIVF : public tsBitstreamReader
{
public:
    bool m_first_call;
    tsBitstreamReaderIVF(const char* fname, mfxU32 buf_size) : tsBitstreamReader(fname, buf_size), m_first_call(true) {};
    tsBitstreamReaderIVF(mfxBitstream bs, mfxU32 buf_size) : tsBitstreamReader(bs, buf_size), m_first_call(true)  {};
    virtual ~tsBitstreamReaderIVF(){};
    mfxStatus ProcessBitstream(mfxBitstream& bs, mfxU32 nFrames);
};
