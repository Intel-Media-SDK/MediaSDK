// Copyright (c) 2018-2019 Intel Corporation
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

#include "mfxvideo.h"

#include <cstdio>
#include <iostream>

#include "inputparameters.h"
#include "test_processor.h"
#include "generator.h"
#include "verifier.h"

using namespace std;


#if defined(_WIN32) || defined(_WIN64)
mfxI32 _tmain(mfxI32 argc, msdk_char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
#if MFX_VERSION < MFX_VERSION_NEXT
    cout << "ERROR: For correct work minimal API MFX_VERSION_NEXT version is required" << endl;
    return -1;

#else
    try {
        InputParams params;

        params.ParseInputString(argv, argc);
        if (params.m_bPrintHelp) {
            return 0;
        }

        switch (params.m_ProcMode)
        {
        case GENERATE:
        {
            Generator generator;
            generator.RunTest(params);
        }
        break;

        case VERIFY:
        {
            Verifier verifier;
            verifier.RunTest(params);

            if (params.m_TestType == GENERATE_REPACK_CTRL)
                break;

            verifier.CheckStatistics(params.m_Thresholds);
        }
        break;

        default:
            throw std::string("ERROR: Undefined processing mode");
            break;
        }
    }
    catch (std::string& e) {
        cout << e << endl;
        return -1;
    }
    catch (int sts)
    {
        return sts;
    }
    catch (...) {
        cout << "ERROR: Unexpected exception triggered" << endl;
        return -1;
    }
    return 0;

#endif // MFX_VERSION
}
