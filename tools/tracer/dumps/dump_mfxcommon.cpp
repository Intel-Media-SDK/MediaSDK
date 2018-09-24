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

#include "dump.h"
#include "../loggers/log.h"

std::string DumpContext::dump(const std::string structName, const mfxBitstream &bitstream)
{
    std::string str;
    str += structName + ".EncryptedData=" + ToString(bitstream.EncryptedData) + "\n";
    str += dump_mfxExtParams(structName, bitstream) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(bitstream.reserved) + "\n";
    str += structName + ".DecodeTimeStamp=" + ToString(bitstream.DecodeTimeStamp) + "\n";
    str += structName + ".TimeStamp=" + ToString(bitstream.TimeStamp) + "\n";
    if (Log::GetLogLevel() == LOG_LEVEL_FULL) {
        str += structName + ".Data=" + ToHexFormatString(bitstream.Data) + "\n";
    }
    str += structName + ".DataOffset=" + ToString(bitstream.DataOffset) + "\n";
    str += structName + ".DataLength=" + ToString(bitstream.DataLength) + "\n";
    str += structName + ".MaxLength=" + ToString(bitstream.MaxLength) + "\n";
    str += structName + ".PicStruct=" + ToString(bitstream.PicStruct) + "\n";
    str += structName + ".FrameType=" + ToString(bitstream.FrameType) + "\n";
    str += structName + ".DataFlag=" + ToString(bitstream.DataFlag) + "\n";
    str += structName + ".reserved2=" + ToString(bitstream.reserved2);
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtBuffer &extBuffer)
{
    std::string str;
    const char* bufid_str = get_bufferid_str(extBuffer.BufferId);
    if (bufid_str)
        str += structName + ".BufferId=" + std::string(bufid_str) + "\n";
    else
        str += structName + ".BufferId=" + ToString(extBuffer.BufferId) + "\n";
    str += structName + ".BufferSz=" + ToString(extBuffer.BufferSz);
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxIMPL &impl)
{
    return std::string("mfxIMPL " + structName + "=" + GetmfxIMPL(impl));
}

std::string DumpContext::dump(const std::string structName, const mfxPriority &priority)
{
    return std::string("mfxPriority " + structName + "=" + ToString(priority));
}

std::string DumpContext::dump(const std::string structName, const mfxInitParam &_struct)
{
    std::string str;

    str += structName + ".Implementation=" + GetmfxIMPL(_struct.Implementation) + "\n";
    str += dump(structName + ".Version", _struct.Version) + "\n";
    DUMP_FIELD(ExternalThreads);
    DUMP_FIELD(NumExtParam);
    str += structName + ".ExtParam=" + ToHexFormatString(_struct.ExtParam) + "\n";
    str += structName + ".reserved2[]=" + DUMP_RESERVED_ARRAY(_struct.reserved2) + "\n";
    str += structName + ".GPUCopy=" + ToString(_struct.GPUCopy) + "\n";
    DUMP_FIELD_RESERVED(reserved);

    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxSyncPoint &syncPoint)
{
    return std::string("mfxSyncPoint* " + structName + "=" + ToString(syncPoint));
}

std::string DumpContext::dump(const std::string structName, const mfxVersion &version)
{
    std::string str;
    str += structName + ".Major=" + ToString(version.Major) + "\n";
    str += structName + ".Minor=" + ToString(version.Minor) + "\n";
    str += structName + ".Version=" + ToString(version.Version);
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxPlatform &platform)
{
    std::string str;
    str += structName + ".CodeName=" + ToString(platform.CodeName) + "\n";
    str += structName + ".DeviceId=" + ToString(platform.DeviceId) + "\n";
    return str;
}
