/* ****************************************************************************** *\

Copyright (C) 2017-2020 Intel Corporation.  All rights reserved.

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

File Name: dump_mfxbrc.cpp

\* ****************************************************************************** */

#include "dump.h"

std::string DumpContext::dump(const std::string structName, const mfxExtBRC &_struct)
{
    std::string str;

    str += dump(structName + ".Header", _struct.Header) + "\n";

    DUMP_FIELD_RESERVED(reserved);

    DUMP_FIELD(pthis);
    DUMP_FIELD(Init);
    DUMP_FIELD(Reset);
    DUMP_FIELD(Close);
    DUMP_FIELD(GetFrameCtrl);
    DUMP_FIELD(Update);

    DUMP_FIELD_RESERVED(reserved1);

    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxBRCFrameParam &_struct)
{
    std::string str;

    DUMP_FIELD_RESERVED(reserved);

    DUMP_FIELD(EncodedOrder);
    DUMP_FIELD(DisplayOrder);
    DUMP_FIELD(CodedFrameSize);
    DUMP_FIELD(FrameType);
    DUMP_FIELD(PyramidLayer);
    DUMP_FIELD(NumRecode);
    dump_mfxExtParams(structName, _struct);

    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxBRCFrameCtrl &_struct)
{
    std::string str;

    DUMP_FIELD(QpY);
#if (MFX_VERSION >= 1029)
    DUMP_FIELD(InitialCpbRemovalDelay);
    DUMP_FIELD(InitialCpbRemovalOffset);
    DUMP_FIELD_RESERVED(reserved1);
    DUMP_FIELD(MaxFrameSize);
    DUMP_FIELD(DeltaQP);
    DUMP_FIELD(MaxNumRepak);
    dump_mfxExtParams(structName, _struct);
#else
    DUMP_FIELD_RESERVED(reserved1);
    DUMP_FIELD(reserved2);
#endif
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxBRCFrameStatus &_struct)
{
    std::string str;

    DUMP_FIELD(MinFrameSize);
    DUMP_FIELD(BRCStatus);
    DUMP_FIELD_RESERVED(reserved);
    DUMP_FIELD(reserved1);

    return str;
}