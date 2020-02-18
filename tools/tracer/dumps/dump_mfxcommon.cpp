/* ****************************************************************************** *\

Copyright (C) 2012-2020 Intel Corporation.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
- Neither the name of Intel Corporation nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL INTEL CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

File Name: dump_mfxcommon.cpp

\* ****************************************************************************** */

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
