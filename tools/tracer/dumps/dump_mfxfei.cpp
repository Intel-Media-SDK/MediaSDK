// Copyright (c) 2015-2022 Intel Corporation
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

std::string DumpContext::dump(const std::string structName, const mfxExtFeiPreEncCtrl &_struct)
{
    std::string str;

    str += dump(structName + ".Header", _struct.Header) + "\n";
    DUMP_FIELD(Qp);
    DUMP_FIELD(LenSP);
    DUMP_FIELD(SearchPath);
    DUMP_FIELD(SubMBPartMask);
    DUMP_FIELD(SubPelMode);
    DUMP_FIELD(InterSAD);
    DUMP_FIELD(IntraSAD);
    DUMP_FIELD(AdaptiveSearch);
    DUMP_FIELD(MVPredictor);
    DUMP_FIELD(MBQp);
    DUMP_FIELD(FTEnable);
    DUMP_FIELD(IntraPartMask);
    DUMP_FIELD(RefWidth);
    DUMP_FIELD(RefHeight);
    DUMP_FIELD(SearchWindow);
    DUMP_FIELD(DisableMVOutput);
    DUMP_FIELD(DisableStatisticsOutput);
    DUMP_FIELD(Enable8x8Stat);
    DUMP_FIELD(PictureType);
    DUMP_FIELD(DownsampleInput);
    DUMP_FIELD(RefPictureType[0]);
    DUMP_FIELD(RefPictureType[1]);
    DUMP_FIELD(DownsampleReference[0]);
    DUMP_FIELD(DownsampleReference[1]);
    DUMP_FIELD(RefFrame[0]);
    DUMP_FIELD(RefFrame[1]);
    DUMP_FIELD_RESERVED(reserved);

    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtFeiPreEncMVPredictors &_struct)
{
    std::string str;

    str += dump(structName + ".Header", _struct.Header) + "\n";
    DUMP_FIELD_RESERVED(reserved1);
    DUMP_FIELD(NumMBAlloc);
    DUMP_FIELD_RESERVED(reserved2);
    if (_struct.MB)
    {
        str += structName + ".MB[]={\n";
        for (unsigned int i = 0; i < _struct.NumMBAlloc; i++)
        {
            str += "{ L0: {" + ToString(_struct.MB[i].MV[0].x) + "," + ToString(_struct.MB[i].MV[0].y)
                + "}, L1: {" + ToString(_struct.MB[i].MV[1].x) + "," + ToString(_struct.MB[i].MV[1].y)
                + "}}\n";
        }
        str += "}\n";
    }

    return str;
}


std::string DumpContext::dump(const std::string structName, const mfxExtFeiPreEncMV &_struct)
{
    std::string str;

    str += dump(structName + ".Header", _struct.Header) + "\n";
    DUMP_FIELD_RESERVED(reserved1);
    DUMP_FIELD(NumMBAlloc);
    DUMP_FIELD_RESERVED(reserved2);
    if (_struct.MB)
    {
        str += structName + ".MB[]={\n";
        for (unsigned int i = 0; i < _struct.NumMBAlloc; i++)
        {
             str += "{\n";
             for (unsigned int j = 0; j < GET_ARRAY_SIZE(_struct.MB[i].MV); j++)
             {
                str += "{ L0: {" + ToString(_struct.MB[i].MV[j][0].x) + "," + ToString(_struct.MB[i].MV[j][0].y)
                    + "}, L1: {" + ToString(_struct.MB[i].MV[j][1].x) + "," + ToString(_struct.MB[i].MV[j][1].y)
                    + "}}, ";
             }
             str += "}\n";
        }
        str += "}\n";
    }
    return str;
}


std::string DumpContext::dump(const std::string structName, const mfxExtFeiPreEncMBStat &_struct)
{
    std::string str;

    str += dump(structName + ".Header", _struct.Header) + "\n";
    DUMP_FIELD_RESERVED(reserved1);
    DUMP_FIELD(NumMBAlloc);
    DUMP_FIELD_RESERVED(reserved2);
    if (_struct.MB)
    {
        for (unsigned int i = 0; i < _struct.NumMBAlloc; i++)
        {
           std::string prefix = structName + ".MB[" + ToString(i) + "]";
           str += prefix + ".Inter[0].BestDistortion=" + ToString(_struct.MB[i].Inter[0].BestDistortion) + "\n";
           str += prefix + ".Inter[0].Mode=" + ToString(_struct.MB[i].Inter[0].Mode) + "\n";
           str += prefix + ".Inter[1].BestDistortion=" + ToString(_struct.MB[i].Inter[1].BestDistortion) + "\n";
           str += prefix + ".Inter[1].Mode=" + ToString(_struct.MB[i].Inter[1].Mode) + "\n";
           str += prefix + ".BestIntraDistortion=" + ToString(_struct.MB[i].BestIntraDistortion) + "\n";
           str += prefix + ".IntraMode=" + ToString(_struct.MB[i].IntraMode) + "\n";
           str += prefix + ".NumOfNonZeroCoef=" + ToString(_struct.MB[i].NumOfNonZeroCoef) + "\n";
           str += prefix + ".reserved1=" + ToString(_struct.MB[i].reserved1) + "\n";
           str += prefix + ".SumOfCoef=" + ToString(_struct.MB[i].SumOfCoef) + "\n";
           str += prefix + ".reserved2=" + ToString(_struct.MB[i].reserved2) + "\n";
           str += prefix + ".Variance16x16=" + ToString(_struct.MB[i].Variance16x16) + "\n";
           str += prefix + ".Variance8x8[0]=" + ToString(_struct.MB[i].Variance8x8[0]) + "\n";
           str += prefix + ".Variance8x8[1]=" + ToString(_struct.MB[i].Variance8x8[1]) + "\n";
           str += prefix + ".Variance8x8[2]=" + ToString(_struct.MB[i].Variance8x8[2]) + "\n";
           str += prefix + ".Variance8x8[3]=" + ToString(_struct.MB[i].Variance8x8[3]) + "\n";
           str += prefix + ".PixelAverage16x16=" + ToString(_struct.MB[i].PixelAverage16x16) + "\n";
           str += prefix + ".PixelAverage8x8[0]=" + ToString(_struct.MB[i].PixelAverage8x8[0]) + "\n";
           str += prefix + ".PixelAverage8x8[1]=" + ToString(_struct.MB[i].PixelAverage8x8[1]) + "\n";
           str += prefix + ".PixelAverage8x8[2]=" + ToString(_struct.MB[i].PixelAverage8x8[2]) + "\n";
           str += prefix + ".PixelAverage8x8[3]=" + ToString(_struct.MB[i].PixelAverage8x8[3]) + "\n";
       }
   }
   return str;
}


std::string DumpContext::dump(const std::string structName, const mfxExtFeiEncFrameCtrl &_struct)
{
    std::string str;

    DUMP_FIELD(SearchPath);
    DUMP_FIELD(LenSP);
    DUMP_FIELD(SubMBPartMask);
    DUMP_FIELD(IntraPartMask);
    DUMP_FIELD(MultiPredL0);
    DUMP_FIELD(MultiPredL1);
    DUMP_FIELD(SubPelMode);
    DUMP_FIELD(InterSAD);
    DUMP_FIELD(IntraSAD);
    DUMP_FIELD(DistortionType);
    DUMP_FIELD(RepartitionCheckEnable);
    DUMP_FIELD(AdaptiveSearch);
    DUMP_FIELD(MVPredictor);
    DUMP_FIELD(NumMVPredictors[0]);
    DUMP_FIELD(NumMVPredictors[1]);
    DUMP_FIELD(PerMBQp);
    DUMP_FIELD(PerMBInput);
    DUMP_FIELD(MBSizeCtrl);
    DUMP_FIELD(RefWidth);
    DUMP_FIELD(RefHeight);
    DUMP_FIELD(SearchWindow);
    DUMP_FIELD(ColocatedMbDistortion);
    DUMP_FIELD_RESERVED(reserved);

    return str;
}


std::string DumpContext::dump(const std::string structName, const mfxExtFeiEncMVPredictors &_struct)
{
    std::string str;

    str += dump(structName + ".Header", _struct.Header) + "\n";
    DUMP_FIELD_RESERVED(reserved1);
    DUMP_FIELD(NumMBAlloc);
    DUMP_FIELD_RESERVED(reserved2);
    if (_struct.MB)
    {
        str +=  structName + ".MB[]={\n";
        for (unsigned int i = 0; i < _struct.NumMBAlloc; i++)
        {
            str += dump("", _struct.MB[i]) + ",\n";
        }
        str += "}\n";
    }
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtFeiEncMVPredictors::mfxExtFeiEncMVPredictorsMB &_struct)
{
    std::string str;

    for (unsigned int i = 0; i < GET_ARRAY_SIZE(_struct.RefIdx); i++)
    {
        str += structName + ".RefIdx[" + ToString(i) + "].RefL0=" + ToString(_struct.RefIdx[i].RefL0) + "\n";
        str += structName + ".RefIdx[" + ToString(i) + "].RefL1=" + ToString(_struct.RefIdx[i].RefL1) + "\n";
    }

    DUMP_FIELD(reserved);

    // mfxI16Pair MV[4][2]; /* first index is predictor number, second is 0 for L0 and 1 for L1 */

    str += structName + ".MV[]={\n";
    for (unsigned int i = 0; i < GET_ARRAY_SIZE(_struct.MV); i++)
    {
        str += "{ L0: {" + ToString(_struct.MV[i][0].x) + "," + ToString(_struct.MV[i][0].y)
            + "}, L1: {" + ToString(_struct.MV[i][1].x) + "," + ToString(_struct.MV[i][1].y)
            + "}}, ";
    }
    str += "}\n";

    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtFeiEncQP &_struct)
{
    std::string str;

    str += dump(structName + ".Header", _struct.Header) + "\n";
    DUMP_FIELD_RESERVED(reserved1);
    DUMP_FIELD(NumMBAlloc); /* size of allocated memory in number of QPs value*/
    DUMP_FIELD_RESERVED(reserved2);

    str += structName + ".QP[]=" + dump_reserved_array(_struct.MB, _struct.NumMBAlloc) + "\n";
    return str;
}


std::string DumpContext::dump(const std::string structName, const mfxExtFeiEncMBCtrl &_struct)
{
    std::string str;

    str += dump(structName + ".Header", _struct.Header) + "\n";
    DUMP_FIELD_RESERVED(reserved1);
    DUMP_FIELD(NumMBAlloc);
    DUMP_FIELD_RESERVED(reserved2);
    if (_struct.MB)
    {
        str += structName + ".MB[]={\n";
        for (unsigned int i = 0; i < _struct.NumMBAlloc; i++)
        {
            str += dump("", _struct.MB[i]) + ",\n";
        }
        str += "}\n";
    }
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtFeiEncMBCtrl::mfxExtFeiEncMBCtrlMB &_struct)
{
    std::string str;

    DUMP_FIELD(ForceToIntra);
    DUMP_FIELD(ForceToSkip);
    DUMP_FIELD(ForceToNoneSkip);
#if (MFX_VERSION >= 1025)
    DUMP_FIELD(DirectBiasAdjustment);
    DUMP_FIELD(GlobalMotionBiasAdjustment);
    DUMP_FIELD(MVCostScalingFactor);
#endif
    DUMP_FIELD(reserved1);

    DUMP_FIELD(reserved2);
    DUMP_FIELD(reserved3);

    DUMP_FIELD(reserved4);
    DUMP_FIELD(TargetSizeInWord);
    DUMP_FIELD(MaxSizeInWord);

    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtFeiEncMV &_struct)
{
    std::string str;

    str += dump(structName + ".Header", _struct.Header) + "\n";
    DUMP_FIELD_RESERVED(reserved1);
    DUMP_FIELD(NumMBAlloc);
    DUMP_FIELD_RESERVED(reserved2);
    if (_struct.MB)
    {
        str += structName + ".MB[]={\n";
        for (unsigned int i = 0; i < _struct.NumMBAlloc; i++)
        {
            str += "{\n";
            for (unsigned int j = 0; j < GET_ARRAY_SIZE(_struct.MB[i].MV); j++)
            {
                 str += "{ L0: {" + ToString(_struct.MB[i].MV[j][0].x) + "," + ToString(_struct.MB[i].MV[j][0].y)
                     + "}, L1: {" + ToString(_struct.MB[i].MV[j][1].x) + "," + ToString(_struct.MB[i].MV[j][1].y)
                     + "}}, ";
            }
            str += "}\n";
        }
        str += "}\n";
    }
    return str;
}


std::string DumpContext::dump(const std::string structName, const mfxExtFeiEncMBStat &_struct)
{
    std::string str;

    str += dump(structName + ".Header", _struct.Header) + "\n";
    DUMP_FIELD_RESERVED(reserved1);
    DUMP_FIELD(NumMBAlloc);
    DUMP_FIELD_RESERVED(reserved2);
    if (_struct.MB)
    {
        str += structName + ".MB[]={\n";
        for (unsigned int i = 0; i < _struct.NumMBAlloc; i++)
        {
            str += dump("", _struct.MB[i]) + ",\n";
        }
        str += "}\n";
    }
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtFeiEncMBStat::mfxExtFeiEncMBStatMB &_struct)
{
    std::string str;

    DUMP_FIELD_RESERVED(InterDistortion);
    DUMP_FIELD(BestInterDistortion);
    DUMP_FIELD(BestIntraDistortion);
    DUMP_FIELD(ColocatedMbDistortion);
    DUMP_FIELD(reserved);
    DUMP_FIELD_RESERVED(reserved1);

    return str;
}



std::string DumpContext::dump(const std::string structName, const mfxFeiPakMBCtrl &_struct)
{
    std::string str;

    DUMP_FIELD(Header);
    DUMP_FIELD(MVDataLength);
    DUMP_FIELD(MVDataOffset);
    DUMP_FIELD(InterMbMode);
    DUMP_FIELD(MBSkipFlag);
    DUMP_FIELD(Reserved00);
    DUMP_FIELD(IntraMbMode);
    DUMP_FIELD(Reserved01);
    DUMP_FIELD(FieldMbPolarityFlag);
    DUMP_FIELD(MbType);
    DUMP_FIELD(IntraMbFlag);
    DUMP_FIELD(FieldMbFlag);
    DUMP_FIELD(Transform8x8Flag);
    DUMP_FIELD(Reserved02);
    DUMP_FIELD(DcBlockCodedCrFlag);
    DUMP_FIELD(DcBlockCodedCbFlag);
    DUMP_FIELD(DcBlockCodedYFlag);
    DUMP_FIELD(MVFormat);
    DUMP_FIELD(Reserved03);
    DUMP_FIELD(ExtendedFormat);
    DUMP_FIELD(HorzOrigin);
    DUMP_FIELD(VertOrigin);
    DUMP_FIELD(CbpY);
    DUMP_FIELD(CbpCb);
    DUMP_FIELD(CbpCr);
    DUMP_FIELD(QpPrimeY);
    DUMP_FIELD(Reserved30);
    DUMP_FIELD(MbSkipConvDisable);
    DUMP_FIELD(IsLastMB);
    DUMP_FIELD(EnableCoefficientClamp);
    DUMP_FIELD(Direct8x8Pattern);

    if (_struct.IntraMbMode)
    {
        //dword 7,8
        str += structName + ".IntraMB.LumaIntraPredModes[]=" + DUMP_RESERVED_ARRAY(_struct.IntraMB.LumaIntraPredModes) + "\n";
        //dword 9
        str += structName + ".IntraMB.ChromaIntraPredMode=" + ToString(_struct.IntraMB.ChromaIntraPredMode) + "\n";
        str += structName + ".IntraMB.IntraPredAvailFlags=" + ToString(_struct.IntraMB.IntraPredAvailFlags) + "\n";
        str += structName + ".IntraMB.Reserved60=" + ToString(_struct.IntraMB.Reserved60) + "\n";
    }

    if (_struct.InterMbMode)
    {
        //dword 7
        str += structName + ".InterMB.SubMbShapes=" + ToString(_struct.InterMB.SubMbShapes) + "\n";
        str += structName + ".InterMB.SubMbPredModes=" + ToString(_struct.InterMB.SubMbPredModes) + "\n";
        str += structName + ".InterMB.Reserved40=" + ToString(_struct.InterMB.Reserved40) + "\n";
        //dword 8, 9
        for (unsigned int i = 0; i < GET_ARRAY_SIZE(_struct.InterMB.RefIdx); i++)
        {
            for (unsigned int j = 0; j < GET_ARRAY_SIZE(_struct.InterMB.RefIdx[0]); j++)
            {
                str += structName + ".InterMB.RefIdx[" + ToString(i) + "][" + ToString(j) + "]=" + ToString(_struct.InterMB.RefIdx[i][j]) + "\n";
            }
        }
    }

    //dword 10
    DUMP_FIELD(Reserved70);
    DUMP_FIELD(TargetSizeInWord);
    DUMP_FIELD(MaxSizeInWord);

    DUMP_FIELD_RESERVED(reserved2);
    return str;
}


std::string DumpContext::dump(const std::string structName, const mfxExtFeiPakMBCtrl &_struct)
{
    std::string str;

    str += dump(structName + ".Header", _struct.Header) + "\n";
    DUMP_FIELD_RESERVED(reserved1);
    DUMP_FIELD(NumMBAlloc);
    DUMP_FIELD_RESERVED(reserved2);
    if (_struct.MB)
    {
        str += structName + ".MB[]={\n";
        for (unsigned int i = 0; i < _struct.NumMBAlloc; i++)
        {
            str += dump("", _struct.MB[i]) + ",\n";
        }
        str += "}\n";
    }
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtFeiRepackCtrl &_struct)
{
    std::string str;

    str += dump(structName + ".Header", _struct.Header) + "\n";
    DUMP_FIELD(MaxFrameSize);
    DUMP_FIELD(NumPasses);
    DUMP_FIELD_RESERVED(reserved);
    DUMP_FIELD_RESERVED(DeltaQP);

    return str;
}

#if (MFX_VERSION >= 1025)
std::string DumpContext::dump(const std::string structName, const mfxExtFeiRepackStat &_struct)
{
    std::string str;
    str += dump(structName + ".Header", _struct.Header) + "\n";
    DUMP_FIELD(NumPasses);
    DUMP_FIELD_RESERVED(reserved);
    return str;
}
#endif

std::string DumpContext::dump(const std::string structName, const mfxFeiDecStreamOutMBCtrl &_struct)
{
    std::string str;

    //dword 0
    DUMP_FIELD(InterMbMode);
    DUMP_FIELD(MBSkipFlag);
    DUMP_FIELD(Reserved00);
    DUMP_FIELD(IntraMbMode);
    DUMP_FIELD(Reserved01);
    DUMP_FIELD(FieldMbPolarityFlag);
    DUMP_FIELD(MbType);
    DUMP_FIELD(IntraMbFlag);
    DUMP_FIELD(FieldMbFlag);
    DUMP_FIELD(Transform8x8Flag);
    DUMP_FIELD(Reserved02);
    DUMP_FIELD(DcBlockCodedCrFlag);
    DUMP_FIELD(DcBlockCodedCbFlag);
    DUMP_FIELD(DcBlockCodedYFlag);
    DUMP_FIELD(Reserved03);
    //dword 1
    DUMP_FIELD(HorzOrigin);
    DUMP_FIELD(VertOrigin);
    //dword 2
    DUMP_FIELD(CbpY);
    DUMP_FIELD(CbpCb);
    DUMP_FIELD(CbpCr);
    DUMP_FIELD(Reserved20);
    DUMP_FIELD(IsLastMB);
    DUMP_FIELD(ConcealMB);
    //dword 3
    DUMP_FIELD(QpPrimeY);
    DUMP_FIELD(Reserved30);
    DUMP_FIELD(Reserved31);
    DUMP_FIELD(NzCoeffCount);
    DUMP_FIELD(Reserved32);
    DUMP_FIELD(Direct8x8Pattern);

    if (_struct.IntraMbMode)
    {
        //dword 4, 5
        str += structName + ".IntraMB.LumaIntraPredModes[]=" + DUMP_RESERVED_ARRAY(_struct.IntraMB.LumaIntraPredModes) + "\n";
        //dword 6
        str += structName + ".IntraMB.ChromaIntraPredMode=" + ToString(_struct.IntraMB.ChromaIntraPredMode) + "\n";
        str += structName + ".IntraMB.IntraPredAvailFlags=" + ToString(_struct.IntraMB.IntraPredAvailFlags) + "\n";
        str += structName + ".IntraMB.Reserved60=" + ToString(_struct.IntraMB.Reserved60) + "\n";
    }

    if (_struct.InterMbMode)
    {
        //dword 4
        str += structName + ".InterMB.SubMbShapes=" + ToString(_struct.InterMB.SubMbShapes) + "\n";
        str += structName + ".InterMB.SubMbPredModes=" + ToString(_struct.InterMB.SubMbPredModes) + "\n";
        str += structName + ".InterMB.Reserved40=" + ToString(_struct.InterMB.Reserved40) + "\n";
        //dword 5, 6
        for (unsigned int i = 0; i < GET_ARRAY_SIZE(_struct.InterMB.RefIdx); i++)
        {
            for (unsigned int j = 0; j < GET_ARRAY_SIZE(_struct.InterMB.RefIdx[0]); j++)
            {
                str += structName + ".InterMB.RefIdx[" + ToString(i) + "][" + ToString(j) + "]=" + ToString(_struct.InterMB.RefIdx[i][j]) + "\n";
            }
        }
    }

    //dword 7
    DUMP_FIELD(Reserved70);

    //dword 8-15
    str += structName + ".MV[]={\n";
    for (unsigned int i = 0; i < GET_ARRAY_SIZE(_struct.MV); i++)
    {
        str += "{ L0: {" + ToString(_struct.MV[i][0].x) + "," + ToString(_struct.MV[i][0].y)
            + "}, L1: {" + ToString(_struct.MV[i][1].x) + "," + ToString(_struct.MV[i][1].y)
            + "}}, ";
    }
    str += "}\n";

    return str;
}


std::string DumpContext::dump(const std::string structName, const mfxExtFeiDecStreamOut &_struct)
{
    std::string str;

    str += dump(structName + ".Header", _struct.Header) + "\n";
    DUMP_FIELD_RESERVED(reserved1);
    DUMP_FIELD(NumMBAlloc);
    DUMP_FIELD(RemapRefIdx);
    DUMP_FIELD(PicStruct);
    DUMP_FIELD_RESERVED(reserved2);
    if (_struct.MB)
    {
        str += structName + ".MB[]={\n";
        for (unsigned int i = 0; i < _struct.NumMBAlloc; i++)
        {
            str += dump("", _struct.MB[i]) + ",\n";
        }
        str += "}\n";
    }
    return str;
}


std::string DumpContext::dump(const std::string structName, const mfxExtFeiParam &_struct)
{
    std::string str;

    str += dump(structName + ".Header", _struct.Header) + "\n";
    DUMP_FIELD(Func);
    DUMP_FIELD(SingleFieldProcessing);
    DUMP_FIELD_RESERVED(reserved);

    return str;
}


std::string DumpContext::dump(const std::string structName, const mfxPAKInput &_struct)
{
    std::string str;

    DUMP_FIELD_RESERVED(reserved);

    if (_struct.InSurface)
    {
        str += dump(structName + ".InSurface", *_struct.InSurface) + "\n";
    }
    else
    {
        str += structName + ".InSurface=NULL\n";
    }
    DUMP_FIELD(NumFrameL0);
    if (_struct.L0Surface)
    {
        for (int i = 0; i < _struct.NumFrameL0; i++)
        {
            if (_struct.L0Surface[i])
            {
                str += dump(structName + ".L0Surface[" + ToString(i) + "]", *_struct.L0Surface[i]) + "\n";
            }
            else
            {
                str += structName + ".L0Surface[" + ToString(i) + "]=NULL\n";
            }
        }
    }
    DUMP_FIELD(NumFrameL1);
    if (_struct.L1Surface)
    {
        for (int i = 0; i < _struct.NumFrameL1; i++)
        {
            if (_struct.L1Surface[i])
            {
                str += dump(structName + ".L1Surface[" + ToString(i) + "]", *_struct.L1Surface[i]) + "\n";
            }
            else
            {
                str += structName + ".L1Surface[" + ToString(i) + "]=NULL\n";
            }
        }
    }
    str += dump_mfxExtParams(structName, _struct) + "\n";
    DUMP_FIELD(NumPayload);
    if (_struct.Payload)
    {
        for (int i = 0; i < _struct.NumPayload; i++)
        {
            if (_struct.Payload[i])
            {
                str += dump(structName + ".Payload[" + ToString(i) + "]", *_struct.Payload[i]) + "\n";
            }
            else
            {
                str += structName + ".Payload[" + ToString(i) +"]=NULL\n";
            }
        }
    }

    return str;
}


std::string DumpContext::dump(const std::string structName, const mfxPAKOutput &_struct)
{
    std::string str;

    DUMP_FIELD_RESERVED(reserved);

    if (_struct.Bs)
    {
        str += dump(structName + ".Bs", *_struct.Bs) + "\n";
    }
    else
    {
        str += structName + ".Bs=NULL\n";
    }

    if (_struct.OutSurface)
    {
        str += dump(structName + ".OutSurface", *_struct.OutSurface) + "\n";
    }
    else
    {
        str += structName + ".OutSurface=NULL\n";
    }

    str += dump_mfxExtParams(structName, _struct) + "\n";

    return str;
}


std::string DumpContext::dump(const std::string structName, const mfxExtFeiSPS &_struct)
{
    std::string str;

    str += dump(structName + ".Header", _struct.Header) + "\n";
    DUMP_FIELD(SPSId);
    DUMP_FIELD(PicOrderCntType);
    DUMP_FIELD(Log2MaxPicOrderCntLsb);
    DUMP_FIELD_RESERVED(reserved);

    return str;
}


std::string DumpContext::dump(const std::string structName, const mfxExtFeiPPS &_struct)
{
    std::string str;

    str += dump(structName + ".Header", _struct.Header) + "\n";
    DUMP_FIELD(SPSId);
    DUMP_FIELD(PPSId);
    DUMP_FIELD(PictureType);
    DUMP_FIELD(FrameType);
    DUMP_FIELD(PicInitQP);
    DUMP_FIELD(NumRefIdxL0Active);
    DUMP_FIELD(NumRefIdxL1Active);
    DUMP_FIELD(ChromaQPIndexOffset);
    DUMP_FIELD(SecondChromaQPIndexOffset);
    DUMP_FIELD(Transform8x8ModeFlag);
    DUMP_FIELD_RESERVED(reserved);

    str += structName + ".DpbBefore[]={\n";
    for (unsigned int i = 0; i < GET_ARRAY_SIZE(_struct.DpbBefore); i++)
    {
        str += "    [" + ToString(i) + "]\n";
        str += "    {\n";
        str += "        .Index"            + ToString(_struct.DpbBefore[i].Index)            + "\n";
        str += "        .PicType"          + ToString(_struct.DpbBefore[i].PicType)          + "\n";
        str += "        .FrameNumWrap"     + ToString(_struct.DpbBefore[i].FrameNumWrap)     + "\n";
        str += "        .LongTermFrameIdx" + ToString(_struct.DpbBefore[i].LongTermFrameIdx) + "\n";
        str += "        .reserved[0]" + ToString(_struct.DpbBefore[i].reserved[0]) + "\n";
        str += "        .reserved[1]" + ToString(_struct.DpbBefore[i].reserved[1]) + "\n";
        str += "        .reserved[2]" + ToString(_struct.DpbBefore[i].reserved[2]) + "\n";
        str += "    }\n";
    }
    str += "}\n";

    str += structName + ".DpbAfter[]={\n";
    for (unsigned int i = 0; i < GET_ARRAY_SIZE(_struct.DpbAfter); i++)
    {
        str += "    [" + ToString(i) + "]\n";
        str += "    {\n";
        str += "        .Index"            + ToString(_struct.DpbAfter[i].Index)            + "\n";
        str += "        .PicType"          + ToString(_struct.DpbAfter[i].PicType)          + "\n";
        str += "        .FrameNumWrap"     + ToString(_struct.DpbAfter[i].FrameNumWrap)     + "\n";
        str += "        .LongTermFrameIdx" + ToString(_struct.DpbAfter[i].LongTermFrameIdx) + "\n";
        str += "    }\n";
    }
    str += "}\n";

    return str;
}


std::string DumpContext::dump(const std::string structName, const mfxExtFeiSliceHeader &_struct)
{
    std::string str;

    str += dump(structName + ".Header", _struct.Header) + "\n";

    DUMP_FIELD(NumSlice);
    DUMP_FIELD_RESERVED(reserved);

    if (_struct.Slice)
    {
        str += structName + ".Slice[]={\n";
        for (int i = 0; i < _struct.NumSlice; i++)
        {
            str += dump("", _struct.Slice[i]) + ",\n";
        }
        str += "}\n";
    }

    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtFeiSliceHeader::mfxSlice &_struct)
{
    std::string str;

    DUMP_FIELD(MBAddress);
    DUMP_FIELD(NumMBs);
    DUMP_FIELD(SliceType);
    DUMP_FIELD(PPSId);
    DUMP_FIELD(IdrPicId);
    DUMP_FIELD(CabacInitIdc);
    DUMP_FIELD(NumRefIdxL0Active);
    DUMP_FIELD(NumRefIdxL1Active);
    DUMP_FIELD(SliceQPDelta);
    DUMP_FIELD(DisableDeblockingFilterIdc);
    DUMP_FIELD(SliceAlphaC0OffsetDiv2);
    DUMP_FIELD(SliceBetaOffsetDiv2);
    DUMP_FIELD_RESERVED(reserved);

    for (unsigned int i = 0; i < GET_ARRAY_SIZE(_struct.RefL0); i++)
    {
        str += structName + ".RefL0[" + ToString(i) + "].PictureType=" + ToString(_struct.RefL0[i].PictureType) + "\n";
        str += structName + ".RefL0[" + ToString(i) + "].Index=" + ToString(_struct.RefL0[i].Index) + "\n";
        str += structName + ".RefL0[" + ToString(i) + "].reserved[0]=" + ToString(_struct.RefL0[i].reserved[0]) + "\n";
        str += structName + ".RefL0[" + ToString(i) + "].reserved[1]=" + ToString(_struct.RefL0[i].reserved[1]) + "\n";
    }

    for (unsigned int i = 0; i < GET_ARRAY_SIZE(_struct.RefL1); i++)
    {
        str += structName + ".RefL1[" + ToString(i) + "].PictureType=" + ToString(_struct.RefL1[i].PictureType) + "\n";
        str += structName + ".RefL1[" + ToString(i) + "].Index=" + ToString(_struct.RefL1[i].Index) + "\n";
        str += structName + ".RefL1[" + ToString(i) + "].reserved[0]=" + ToString(_struct.RefL1[i].reserved[0]) + "\n";
        str += structName + ".RefL1[" + ToString(i) + "].reserved[1]=" + ToString(_struct.RefL1[i].reserved[1]) + "\n";
    }

    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtFeiCodingOption &_struct)
{
    std::string str;

    str += dump(structName + ".Header", _struct.Header) + "\n";

    DUMP_FIELD(DisableHME);
    DUMP_FIELD(DisableSuperHME);
    DUMP_FIELD(DisableUltraHME);
    DUMP_FIELD_RESERVED(reserved);
    return str;
}
