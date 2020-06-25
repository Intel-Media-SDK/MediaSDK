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

File Name: dump_mfxvp8.cpp

\* ****************************************************************************** */

#include "dump.h"

std::string DumpContext::dump(const std::string structName, const mfxExtVP8CodingOption &_struct)
{
    std::string str;

    str += dump(structName + ".Header", _struct.Header) + "\n";

    DUMP_FIELD(Version);
    DUMP_FIELD(EnableMultipleSegments);
    DUMP_FIELD(LoopFilterType);
    DUMP_FIELD_RESERVED(LoopFilterLevel);
    DUMP_FIELD(SharpnessLevel);
    DUMP_FIELD(NumTokenPartitions);
    DUMP_FIELD_RESERVED(LoopFilterRefTypeDelta);
    DUMP_FIELD_RESERVED(LoopFilterMbModeDelta);
    DUMP_FIELD_RESERVED(SegmentQPDelta);
    DUMP_FIELD_RESERVED(CoeffTypeQPDelta);
    DUMP_FIELD(WriteIVFHeaders);
    DUMP_FIELD(NumFramesForIVFHeader);
    DUMP_FIELD(reserved);

    return str;
}