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