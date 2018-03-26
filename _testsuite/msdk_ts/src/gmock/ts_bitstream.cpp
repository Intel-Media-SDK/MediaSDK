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

#include "ts_bitstream.h"

tsReader::tsReader(mfxBitstream bs)
    : mfxBitstream(bs)
    , m_file(0)
{
}

tsReader::tsReader(const char* fname)
    : mfxBitstream()
    , m_file(0)
{
#pragma warning(disable:4996)
    m_file = fopen(fname, "rb");
#pragma warning(default:4996)
    EXPECT_NE((void*) 0, m_file) << "ERROR: cannot open file `" << fname << "'\n";
    if(0 == m_file)
        throw tsFAIL;
}

tsReader::~tsReader()
{
    if(m_file)
    {
        fclose(m_file);
    }
}

mfxU32 tsReader::Read(mfxU8* dst, mfxU32 size)
{
    if(m_file)
    {
        return (mfxU32)fread(dst, 1, size, m_file);
    }
    else 
    {
        mfxU32 sz = TS_MIN(DataLength, size);

        memcpy(dst, Data + DataOffset, sz);

        DataOffset += sz;
        DataLength -= sz;

        return sz;
    }
}

mfxStatus tsReader::SeekToStart()
{
    fseek(m_file, 0, SEEK_SET);
    return MFX_ERR_NONE;
}

mfxBitstream* tsBitstreamProcessor::ProcessBitstream(mfxBitstream& bs)
{
    TRACE_FUNC1(tsSurfaceProcessor::ProcessBitstream, bs);
    if(ProcessBitstream(bs, 1))
    {
        return 0;
    }

    return &bs;
}


tsBitstreamWriter::tsBitstreamWriter(const char* fname, bool append)
{
    
#pragma warning(disable:4996)
    m_file = fopen(fname, append ? "ab" : "wb");
#pragma warning(default:4996)
}

tsBitstreamWriter::~tsBitstreamWriter()
{
    if(m_file)
    {
        fclose(m_file);
    }
}
    
mfxStatus tsBitstreamWriter::ProcessBitstream(mfxBitstream& bs, mfxU32 nFrames)
{
    if(bs.DataLength != fwrite(bs.Data + bs.DataOffset, 1, bs.DataLength, m_file))
    {
        return MFX_ERR_UNKNOWN;
    }
    bs.DataLength = 0;
    return MFX_ERR_NONE; 
}

tsBitstreamReader::tsBitstreamReader(const char* fname, mfxU32 buf_size)
    : tsReader(fname)
    , m_eos(false)
    , m_buf(new mfxU8[buf_size])
    , m_buf_size(buf_size)
{
}

tsBitstreamReader::tsBitstreamReader(mfxBitstream bs, mfxU32 buf_size)
    : tsReader(bs)
    , m_eos(false)
    , m_buf(new mfxU8[buf_size])
    , m_buf_size(buf_size)
{
}

tsBitstreamReader::~tsBitstreamReader()
{
    if(m_buf)
    {
        delete[] m_buf;
    }
}

mfxStatus tsBitstreamReader::ProcessBitstream(mfxBitstream& bs, mfxU32 nFrames)
{
    if(m_eos && bs.DataLength < 4)
    {
        return MFX_ERR_MORE_DATA;
    }

    if(bs.DataLength + bs.DataOffset > m_buf_size)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    bs.Data      = m_buf;
    bs.MaxLength = m_buf_size;

    if(bs.DataLength && bs.DataOffset)
    {
        memmove(bs.Data, bs.Data + bs.DataOffset, bs.DataLength);
    }
    bs.DataOffset = 0;

    bs.DataLength += Read(bs.Data + bs.DataLength, bs.MaxLength - bs.DataLength);

    m_eos = bs.DataLength != bs.MaxLength;

    return MFX_ERR_NONE;
}


mfxStatus tsBitstreamReaderIVF::ProcessBitstream(mfxBitstream& bs, mfxU32 nFrames)
{
#pragma pack(push, 4)
    struct IVF_file_header
    {
        union{
            mfxU32 _signature;
            mfxU8  signature[4]; //DKIF
        };
        mfxU16 version;
        mfxU16 header_length; //bytes
        mfxU32 FourCC;
        mfxU16 witdh;
        mfxU16 height;
        mfxU32 frame_rate;
        mfxU32 time_scale;
        mfxU32 n_frames;
        mfxU32 unused;
    } ;

    struct IVF_frame_header
    {
        mfxU32 frame_size; //bytes
        mfxU64 time_stamp;
    };
#pragma pack(pop)

    mfxU32 frame_size = 0;

    if(m_eos)
    {
        return MFX_ERR_MORE_DATA;
    }

    if(bs.DataLength + bs.DataOffset > m_buf_size)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    bs.Data      = m_buf;
    bs.MaxLength = m_buf_size;

    if(bs.DataLength && bs.DataOffset)
    {
        memmove(bs.Data, bs.Data + bs.DataOffset, bs.DataLength);
    }
    bs.DataOffset = 0;

    if(m_first_call)
    {
        mfxU32 skip_size = 0;
        unsigned char dummy[sizeof(IVF_file_header)];
        bool found = false;
        do
        {
            skip_size = Read(dummy, sizeof(dummy));
            if (skip_size != sizeof(dummy))
            {
                m_eos = true;
                return MFX_ERR_MORE_DATA;
            }

            for (mfxU32 i = 0; i < skip_size; ++i)
                if(!strcmp((char const*)(dummy + i), "DKIF"))
                {
                    skip_size -= i;
                    found = true;
                    break;
                }
        }
        while (!found);

        skip_size = sizeof(dummy) - skip_size;
        Read(dummy, skip_size);

        m_first_call = false;
    }

    mfxU32 const header_size = sizeof(IVF_frame_header);
    if(header_size != Read(bs.Data + bs.DataLength, header_size))
    {
        m_eos = true;
        return MFX_ERR_MORE_DATA;
    }

    auto fh = reinterpret_cast<IVF_frame_header const*>(bs.Data + bs.DataOffset + bs.DataLength);
    frame_size = fh->frame_size;

    if(bs.MaxLength - bs.DataLength < frame_size)
    {
        return MFX_ERR_NOT_ENOUGH_BUFFER;
    }

    if(frame_size != Read(bs.Data + bs.DataOffset + bs.DataLength, frame_size))
    {
        m_eos = true;
        return MFX_ERR_MORE_DATA;
    }

    bs.DataLength += frame_size;
    bs.DataFlag = MFX_BITSTREAM_COMPLETE_FRAME;

    return MFX_ERR_NONE;
}
