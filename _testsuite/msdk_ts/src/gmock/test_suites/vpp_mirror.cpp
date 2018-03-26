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

#include "ts_vpp.h"
#include "ts_struct.h"
#include "mfxstructures.h"

namespace vpp_mirror
{
class TestSuite : tsVideoVPP
{
public:
    TestSuite()
        : tsVideoVPP()
    { }
    ~TestSuite() {}
    int RunTest(unsigned int id);
    static const unsigned int n_cases;

private:

    enum
    {
        MFX_PAR = 1,
        EXTBUFF_VPP_MIRRORING,
        ALLOC_OPAQUE
    };

    struct tc_struct
    {
        mfxStatus sts;
        mfxU32 mode;
        struct f_pair
        {
            mfxU32 ext_type;
            const  tsStruct::Field* f;
            mfxU32 v;
        } set_par[MAX_NPARS];
    };

    static const tc_struct test_case[];
};

const TestSuite::tc_struct TestSuite::test_case[] =
{
    /* normal flow test cases */
    {/*00*/ MFX_ERR_NONE, 0, {
        {MFX_PAR, &tsStruct::mfxVideoParam.IOPattern, MFX_IOPATTERN_IN_SYSTEM_MEMORY | MFX_IOPATTERN_OUT_SYSTEM_MEMORY},
        {EXTBUFF_VPP_MIRRORING, &tsStruct::mfxExtVPPMirroring.Type, MFX_MIRRORING_HORIZONTAL}}
    },
    {/*01*/ MFX_ERR_NONE, 0, {
        {MFX_PAR, &tsStruct::mfxVideoParam.IOPattern, MFX_IOPATTERN_IN_VIDEO_MEMORY | MFX_IOPATTERN_OUT_SYSTEM_MEMORY},
        {MFX_PAR, &tsStruct::mfxVideoParam.vpp.In.Width, 1280},
        {MFX_PAR, &tsStruct::mfxVideoParam.vpp.In.Height, 720},
        {MFX_PAR, &tsStruct::mfxVideoParam.vpp.Out.Width, 1280},
        {MFX_PAR, &tsStruct::mfxVideoParam.vpp.Out.Height, 720},
        {EXTBUFF_VPP_MIRRORING, &tsStruct::mfxExtVPPMirroring.Type, MFX_MIRRORING_HORIZONTAL}}
    },
    {/*02*/ MFX_ERR_NONE, 0, {
        {MFX_PAR, &tsStruct::mfxVideoParam.IOPattern, MFX_IOPATTERN_IN_SYSTEM_MEMORY | MFX_IOPATTERN_OUT_VIDEO_MEMORY},
        {MFX_PAR, &tsStruct::mfxVideoParam.vpp.In.Width, 1920},
        {MFX_PAR, &tsStruct::mfxVideoParam.vpp.In.Height, 1088},
        {MFX_PAR, &tsStruct::mfxVideoParam.vpp.Out.Width, 1920},
        {MFX_PAR, &tsStruct::mfxVideoParam.vpp.Out.Height, 1088},
        {EXTBUFF_VPP_MIRRORING, &tsStruct::mfxExtVPPMirroring.Type, MFX_MIRRORING_HORIZONTAL}}
    },
    {/*03*/ MFX_ERR_NONE, 0, {
        {MFX_PAR, &tsStruct::mfxVideoParam.IOPattern, MFX_IOPATTERN_IN_VIDEO_MEMORY | MFX_IOPATTERN_OUT_VIDEO_MEMORY},
        {EXTBUFF_VPP_MIRRORING, &tsStruct::mfxExtVPPMirroring.Type, MFX_MIRRORING_HORIZONTAL}}
    },
    {/*04*/ MFX_ERR_NONE, ALLOC_OPAQUE, {
        {MFX_PAR, &tsStruct::mfxVideoParam.IOPattern, MFX_IOPATTERN_IN_OPAQUE_MEMORY | MFX_IOPATTERN_OUT_OPAQUE_MEMORY},
        {EXTBUFF_VPP_MIRRORING, &tsStruct::mfxExtVPPMirroring.Type, MFX_MIRRORING_HORIZONTAL}}
    },

    /*  bad parameters test cases */
    {/*05*/ MFX_ERR_INVALID_VIDEO_PARAM, 0, {
        {MFX_PAR, &tsStruct::mfxVideoParam.IOPattern, MFX_IOPATTERN_IN_SYSTEM_MEMORY | MFX_IOPATTERN_OUT_SYSTEM_MEMORY},
        {EXTBUFF_VPP_MIRRORING, &tsStruct::mfxExtVPPMirroring.Type, MFX_MIRRORING_HORIZONTAL + 100}}
    },
};

const unsigned int TestSuite::n_cases = sizeof(TestSuite::test_case)/sizeof(TestSuite::tc_struct);

int TestSuite::RunTest(unsigned int id)
{
    TS_START;
    const tc_struct& tc = test_case[id];
    mfxStatus init_sts = tc.sts;
    mfxU32 mode = tc.mode;

    MFXInit();

    // set parameters for video processing
    SETPARS(m_pPar, MFX_PAR);

    CreateAllocators();
    SetFrameAllocator();
    SetHandle();

    // prepare extended buffers with parameters and set
    std::vector<mfxExtBuffer*> ext_buffers;
    mfxExtVPPMirroring ext_buffer_mirroring;
    memset(&ext_buffer_mirroring, 0, sizeof(mfxExtVPPMirroring));
    ext_buffer_mirroring.Header.BufferId = MFX_EXTBUFF_VPP_MIRRORING;
    ext_buffer_mirroring.Header.BufferSz = sizeof(mfxExtVPPMirroring);
    SETPARS(&ext_buffer_mirroring, EXTBUFF_VPP_MIRRORING);
    ext_buffers.push_back((mfxExtBuffer*)&ext_buffer_mirroring);

    if (mode == ALLOC_OPAQUE) {
        mfxExtOpaqueSurfaceAlloc osa;
        memset(&osa, 0, sizeof(mfxExtOpaqueSurfaceAlloc));
        osa.Header.BufferId = MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION;
        osa.Header.BufferSz = sizeof(mfxExtOpaqueSurfaceAlloc);
        QueryIOSurf();

        if (m_par.IOPattern & MFX_IOPATTERN_IN_OPAQUE_MEMORY)
            m_spool_in.AllocOpaque(m_request[0], osa);
        if (m_par.IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY)
            m_spool_out.AllocOpaque(m_request[1], osa);

        ext_buffers.push_back((mfxExtBuffer*)&osa);
    }

    m_par.NumExtParam = (mfxU16)ext_buffers.size();
    m_par.ExtParam = &ext_buffers[0];

    // check Query behaviour
    g_tsStatus.expect((init_sts == MFX_ERR_INVALID_VIDEO_PARAM) ? MFX_ERR_UNSUPPORTED : init_sts);
    Query();

    // check Init behaviour
    g_tsStatus.expect(init_sts);
    Init(m_session, m_pPar);

    if (MFX_ERR_NONE == init_sts)
    {
        // run VPP and check its behaviour
        g_tsStatus.expect(MFX_ERR_NONE);
        if (mode != ALLOC_OPAQUE) AllocSurfaces();
        ProcessFrames(10);
    }

    // close the session and check surfaces to be freed
    Close(true);

    TS_END;
    return 0;
}

TS_REG_TEST_SUITE_CLASS(vpp_mirror);

}
