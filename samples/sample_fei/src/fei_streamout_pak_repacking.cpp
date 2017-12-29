/******************************************************************************\
Copyright (c) 2005-2017, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This sample was distributed or derived from the Intel's Media Samples package.
The original version of this sample may be obtained from https://software.intel.com/en-us/intel-media-server-studio
or https://software.intel.com/en-us/media-client-solutions-support.
\**********************************************************************************/

#include "fei_encpak.h"

static mfxU32 mv_data_length_offset = 0;
// Intra types remapped to cases with set cbp. H.264 Table 7-11.
static const int mb_type_remap[26] = {0, 21, 22, 23, 24, 21, 22, 23, 24, 21, 22, 23, 24, 21, 22, 23, 24, 21, 22, 23, 24, 21, 22, 23, 24, 25};
// Luma intra pred mode for H.264 Table 7-11.
static const int intra_16x16[26]   = {2,  0,  1,  2,  3,  0,  1,  2,  3,  0,  1,  2,  3,  0,  1,  2,  3,  0,  1,  2,  3,  0,  1,  2,  3,  2};
// Inter mb split mode for H.264 Table 7-14.
static const int inter_mb_mode[1+22] = {-1, 0,  0,  0,  1,  2,  1,  2,  1,  2,  1,  2,  1,  2,  1,  2,  1,  2,  1,  2,  1,  2,  3};
// Prediction directions for 8x8 blocks for H.264 Table 7-14.
static const mfxU8 sub_mb_type[1+22][4] = {
    {0xff,},   {0,0,0,0}, {1,1,1,1}, {2,2,2,2}, // 0-3
    {0,0,0,0}, {0,0,0,0}, {1,1,1,1}, {1,1,1,1}, // 4-7
    {0,0,1,1}, {0,1,0,1}, {1,1,0,0}, {1,0,1,0}, // 8-11
    {0,0,2,2}, {0,2,0,2}, {1,1,2,2}, {1,2,1,2}, // 12-15
    {2,2,0,0}, {2,0,2,0}, {2,2,1,1}, {2,1,2,1}, // 16-19
    {2,2,2,2}, {2,2,2,2}, {0xff,}               // 20-22
};


// Repack is neccessary because
// 1. DSO has MV for 8x8 blocks, not for 4x4
// 2. To let PAK decide on cbp, all cbp and related vars are set
// 3. Because of MV are changed, directMV and skip conditions have to be recomputed
// 4. After MV elimination some splits can be enlarged

inline mfxStatus RepackStremoutMB2PakMB(mfxFeiDecStreamOutMBCtrl* dsoMB, mfxFeiPakMBCtrl* pakMB, mfxU8 QP)
{
    MSDK_CHECK_POINTER(dsoMB, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(pakMB, MFX_ERR_NULL_PTR);

    /* fill header */
    pakMB->Header              = MFX_PAK_OBJECT_HEADER;
    pakMB->MVDataLength        = dsoMB->IntraMbFlag? 0 : 128;
    pakMB->MVDataOffset        = dsoMB->IntraMbFlag? 0 : mv_data_length_offset;
    mv_data_length_offset     += 128;

    pakMB->ExtendedFormat      = 1;
    pakMB->MVFormat            = dsoMB->IntraMbFlag ? 0 : 6;

    pakMB->InterMbMode         = dsoMB->InterMbMode;
    pakMB->MBSkipFlag          = 0; // skip to be decided in PAK after quantization
    pakMB->IntraMbMode         = dsoMB->IntraMbMode;
    pakMB->FieldMbPolarityFlag = dsoMB->FieldMbPolarityFlag;
    pakMB->IntraMbFlag         = dsoMB->IntraMbFlag;
    pakMB->MbType              = pakMB->IntraMbFlag? mb_type_remap[dsoMB->MbType]: dsoMB->MbType; // intra remapped to cases with set cbp
    pakMB->FieldMbFlag         = dsoMB->FieldMbFlag;
    pakMB->Transform8x8Flag    = dsoMB->Transform8x8Flag;
    pakMB->DcBlockCodedCrFlag  = 1; // all cbp decided by PAK
    pakMB->DcBlockCodedCbFlag  = 1;
    pakMB->DcBlockCodedYFlag   = 1;
    pakMB->HorzOrigin          = dsoMB->HorzOrigin;
    pakMB->VertOrigin          = dsoMB->VertOrigin;
    pakMB->CbpY                = 0xffff; // all cbp decided by PAK
    pakMB->CbpCb               = 0xf;
    pakMB->CbpCr               = 0xf;
    pakMB->QpPrimeY            = QP;//dsoMB->QpPrimeY;
    pakMB->IsLastMB            = dsoMB->IsLastMB;
    pakMB->Direct8x8Pattern    = 0; // to be recomputed in RepackStreamoutMV
    pakMB->MbSkipConvDisable   = 0;

    MSDK_MEMCPY_VAR(pakMB->InterMB, &dsoMB->InterMB, sizeof(pakMB->InterMB)); // this part is common

    if (pakMB->IntraMbFlag ){
        if (dsoMB->MbType) { // for intra 16x16 populate the type to each of 16 4-bit field
            pakMB->IntraMB.LumaIntraPredModes[0] =
            pakMB->IntraMB.LumaIntraPredModes[1] =
            pakMB->IntraMB.LumaIntraPredModes[2] =
            pakMB->IntraMB.LumaIntraPredModes[3] = intra_16x16[dsoMB->MbType] * 0x1111;
        }
        else {
            if (pakMB->Transform8x8Flag) { // for intra 8x8: in DSO 4 types are packed in single dw. Populate each type in separate dw.
                pakMB->IntraMB.LumaIntraPredModes[0] =  (dsoMB->IntraMB.LumaIntraPredModes[0]&0x000f)      * 0x1111;
                pakMB->IntraMB.LumaIntraPredModes[1] = ((dsoMB->IntraMB.LumaIntraPredModes[0]&0x00f0)>>4)  * 0x1111;
                pakMB->IntraMB.LumaIntraPredModes[2] = ((dsoMB->IntraMB.LumaIntraPredModes[0]&0x0f00)>>8)  * 0x1111;
                pakMB->IntraMB.LumaIntraPredModes[3] = ((dsoMB->IntraMB.LumaIntraPredModes[0]&0xf000)>>12) * 0x1111;
            }
        }
    }
    else {
        pakMB->InterMB.SubMbShapes = 0; // no sub shapes after dso
        pakMB->InterMbMode = inter_mb_mode[dsoMB->MbType]; // derive split mode from mbtype
    }

    pakMB->TargetSizeInWord = 0xff;
    pakMB->MaxSizeInWord    = 0xff;

    pakMB->reserved2[4]     = pakMB->IsLastMB ? 0x5000000 : 0; /* end of slice */

    return MFX_ERR_NONE;
}

inline mfxStatus RepackStreamoutMV(mfxFeiDecStreamOutMBCtrl* dsoMB, mfxExtFeiEncMV::mfxExtFeiEncMVMB* encMB)
{

    for (int i = 0; i < 16; i++){
        encMB->MV[i][0] = dsoMB->MV[i>>2][0];
        encMB->MV[i][1] = dsoMB->MV[i>>2][1];
    }

    return MFX_ERR_NONE;
}


//
// To recompute and reset direct
//

#define I_SLICE(SliceType) ((SliceType) == 2 || (SliceType) == 7)
#define P_SLICE(SliceType) ((SliceType) == 0 || (SliceType) == 5)
#define B_SLICE(SliceType) ((SliceType) == 1 || (SliceType) == 6)

const mfxU8 rasterToZ[16] = {0,1,4,5,2,3,6,7, 8,9,12,13,10,11,14,15}; // the same for reverse

const mfxU8 sbDirect[2][16] = {
    {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15},
    {0,0,3,3,0,0,3,3,12,12,15,15,12,12,15,15}
}; // raster idx [direct_inference_8x8][raster 4x4 block]

const mfxI16Pair zeroMv = {0,0};

#define MV_NEIGHBOUR(umb, neib, nbl) \
    mfxI32 pmode = (mbCode->MB[umb].InterMB.SubMbPredModes >> (nbl*2)) & 3; \
    if (pmode == 2 || pmode == list) { \
        mvn[neib] = &mvs->MB[umb].MV[nbl*5][list]; \
        isRefDif[neib] = (ref_idx != mbCode->MB[umb].InterMB.RefIdx[list][nbl]); \
    }
// pmode==2 means bidirectional; nbl*5 makes z-index for corner 4x4 subblock of 8x8 block


// MV prediction according to chapter 8.4.1 of H.264 spec
//
void MVPrediction(mfxI32 uMB, mfxI32 wmb, mfxI32 uMBx, mfxI32 uMBy, mfxU8 list, mfxU8 ref_idx,
    const mfxExtFeiEncMV* mvs, const mfxExtFeiPakMBCtrl* mbCode, mfxI16Pair *mvPred)
{
    bool isRefDif[3] = {1,1,1};
    bool hasB = 0, hasC = 0;
    const mfxI16Pair *mvn[3] = {&zeroMv, &zeroMv, &zeroMv};
    // only for 16x16
    if (uMBx > 0 && !mbCode->MB[uMB-1].IntraMbFlag) { // A
        MV_NEIGHBOUR(uMB-1, 0, 1)
    }
    if (uMBy > 0) {
        hasB = 1;
        if (!mbCode->MB[uMB-wmb].IntraMbFlag) { // B
            MV_NEIGHBOUR(uMB-wmb, 1, 2)
        }
        if (uMBx < wmb-1) {
            hasC = 1;
            if (!mbCode->MB[uMB-wmb+1].IntraMbFlag) { // C
                MV_NEIGHBOUR(uMB-wmb+1, 2, 2)
            }
        } else if (uMBx > 0) {
            hasC = 1;
            if (!mbCode->MB[uMB-wmb-1].IntraMbFlag) { // D
                MV_NEIGHBOUR(uMB-wmb-1, 2, 3)
            }
        }
    }

    if (!hasB && !hasC)
        *mvPred = *(mvn[0]);
    else if (isRefDif[0] + isRefDif[1] + isRefDif[2] == 2) {
        if (!isRefDif[0]) *mvPred = *(mvn[0]);
        else if (!isRefDif[1]) *mvPred = *(mvn[1]);
        else *mvPred = *(mvn[2]);
    } else {
        mvPred->x = (std::min)(mvn[0]->x, mvn[1]->x) ^ (std::min)(mvn[0]->x, mvn[2]->x) ^ (std::min)(mvn[2]->x, mvn[1]->x);
        mvPred->y = (std::min)(mvn[0]->y, mvn[1]->y) ^ (std::min)(mvn[0]->y, mvn[2]->y) ^ (std::min)(mvn[2]->y, mvn[1]->y);
    }
}

// Ref prediction according to chapter 8.4.1 of H.264 spec
// for B 16x16
void RefPrediction(mfxI32 uMB, mfxI32 wmb, mfxI32 uMBx, mfxI32 uMBy,
    const mfxExtFeiPakMBCtrl* mbCode, mfxU8* refs)
{
    mfxI32 pmode;
    mfxU8 ref[2][3] = {{255,255,255}, {255,255,255}};
    // only for 16x16
    if (uMBx > 0 && !mbCode->MB[uMB-1].IntraMbFlag) {
        pmode = (mbCode->MB[uMB-1].InterMB.SubMbPredModes >> 2) & 3;
        if (pmode == 2 || pmode == 0) ref[0][0] = mbCode->MB[uMB-1].InterMB.RefIdx[0][1];
        if (pmode == 2 || pmode == 1) ref[1][0] = mbCode->MB[uMB-1].InterMB.RefIdx[1][1];
    }
    if (uMBy > 0 && !mbCode->MB[uMB-wmb].IntraMbFlag) {
        pmode = (mbCode->MB[uMB-wmb].InterMB.SubMbPredModes >> 4) & 3;
        if (pmode == 2 || pmode == 0) ref[0][1] = mbCode->MB[uMB-wmb].InterMB.RefIdx[0][2];
        if (pmode == 2 || pmode == 1) ref[1][1] = mbCode->MB[uMB-wmb].InterMB.RefIdx[1][2];
    }
    if (uMBy > 0) {
        if (uMBx < wmb-1) {
            if (!mbCode->MB[uMB-wmb+1].IntraMbFlag) { // C
                pmode = (mbCode->MB[uMB-wmb+1].InterMB.SubMbPredModes >> 4) & 3;
                if (pmode == 2 || pmode == 0) ref[0][2] = mbCode->MB[uMB-wmb+1].InterMB.RefIdx[0][2];
                if (pmode == 2 || pmode == 1) ref[1][2] = mbCode->MB[uMB-wmb+1].InterMB.RefIdx[1][2];
            }
        } else if (uMBx > 0 && !mbCode->MB[uMB-wmb-1].IntraMbFlag) { // D
            pmode = (mbCode->MB[uMB-wmb-1].InterMB.SubMbPredModes >> 6) & 3;
            if (pmode == 2 || pmode == 0) ref[0][2] = mbCode->MB[uMB-wmb-1].InterMB.RefIdx[0][3];
            if (pmode == 2 || pmode == 1) ref[1][2] = mbCode->MB[uMB-wmb-1].InterMB.RefIdx[1][3];
        }
    }
    refs[0] = (std::min)((std::min)(ref[0][0], ref[0][1]), ref[0][2]);
    refs[1] = (std::min)((std::min)(ref[1][0], ref[1][1]), ref[1][2]);
}

#ifdef DUMP_MB_DSO

void StartDumpMB(const msdk_char* fname, int frameNum, int newFile)
{
    FILE *f;
    MSDK_FOPEN(f, fname, newFile ? MSDK_CHAR("wt") : MSDK_CHAR("at"));
    if (!f) return;

    msdk_fprintf(f, MSDK_CHAR("\n=== FRAME:%3d ===\n"), frameNum);
    fclose(f);
}

void DumpMB(const msdk_char* fname, struct iTask* task, int uMB)
{
    //const mfxExtFeiEncFrameCtrl* encCtrl = (mfxExtFeiEncFrameCtrl*)GetExtBuffer(task->ENC_in.ExtParam, task->ENC_in.NumExtParam, MFX_EXTBUFF_FEI_ENC_CTRL);
    //const mfxExtFeiEncMVPredictors* mvPreds = (mfxExtFeiEncMVPredictors*)GetExtBuffer(task->ENC_in.ExtParam, task->ENC_in.NumExtParam, MFX_EXTBUFF_FEI_ENC_MV_PRED);
    //const mfxExtFeiEncMBCtrl* mbCtrl = (mfxExtFeiEncMBCtrl*)GetExtBuffer(task->ENC_in.ExtParam, task->ENC_in.NumExtParam, MFX_EXTBUFF_FEI_ENC_MB);
    //const mfxExtFeiSPS* sps = (mfxExtFeiSPS*)GetExtBuffer(task->ENC_in.ExtParam, task->inENC_in.NumExtParam, MFX_EXTBUFF_FEI_SPS);
    //const mfxExtFeiPPS* pps = (mfxExtFeiPPS*)GetExtBuffer(task->ENC_in.ExtParam, task->ENC_in.NumExtParam, MFX_EXTBUFF_FEI_PPS);
    //const mfxExtFeiSliceHeader* sliceHeader = (mfxExtFeiSliceHeader*)GetExtBuffer(task->ENC_in.ExtParam, task->ENC_in.NumExtParam, MFX_EXTBUFF_FEI_SLICE);
    //const mfxExtFeiEncQP* qps = (mfxExtFeiEncQP*)GetExtBuffer(task->ENC_in.ExtParam, task->ENC_in.NumExtParam, MFX_EXTBUFF_FEI_PREENC_QP);

    //const mfxExtFeiEncMBStat* mbdata = (mfxExtFeiEncMBStat*)GetExtBuffer(task->out.ExtParam, task->out.NumExtParam, MFX_EXTBUFF_FEI_ENC_MB_STAT);
    const mfxExtFeiEncMV* mvs = ( mfxExtFeiEncMV*)GetExtBuffer(task->PAK_in.ExtParam, task->PAK_in.NumExtParam, MFX_EXTBUFF_FEI_ENC_MV);
    const mfxExtFeiPakMBCtrl* mbCode = (mfxExtFeiPakMBCtrl*)GetExtBuffer(task->PAK_in.ExtParam, task->PAK_in.NumExtParam, MFX_EXTBUFF_FEI_PAK_CTRL);

    FILE *f;
    MSDK_FOPEN(f, fname, MSDK_CHAR("at"));
    if (!f) return;

    msdk_fprintf(f, MSDK_CHAR("\nMB:%3d  intra:%d\n"), uMB, mbCode->MB[uMB].IntraMbFlag);
    msdk_fprintf(f, MSDK_CHAR("      MbType:%2d 8x8:%d\n"), mbCode->MB[uMB].MbType, mbCode->MB[uMB].Transform8x8Flag);
    msdk_fprintf(f, MSDK_CHAR(" DCCoded:%d%d%d cbp:%x %x %x\n"), mbCode->MB[uMB].DcBlockCodedYFlag, mbCode->MB[uMB].DcBlockCodedCrFlag, mbCode->MB[uMB].DcBlockCodedCbFlag,
            mbCode->MB[uMB].CbpY, mbCode->MB[uMB].CbpCr, mbCode->MB[uMB].CbpCb);
    msdk_fprintf(f, MSDK_CHAR(" QP:%2d MbSkipConvDisable:%d EnableCoefficientClamp:%x\n"), mbCode->MB[uMB].QpPrimeY, mbCode->MB[uMB].MbSkipConvDisable, mbCode->MB[uMB].EnableCoefficientClamp);

    if(mbCode->MB[uMB].IntraMbFlag) {
        msdk_fprintf(f, MSDK_CHAR(" IntraMbMode:%d skip:%d direct:%x\n"), mbCode->MB[uMB].IntraMbMode, mbCode->MB[uMB].MBSkipFlag, mbCode->MB[uMB].Direct8x8Pattern);
        msdk_fprintf(f, MSDK_CHAR(" LumaIntraPredModes: %4x %4x %4x %4x   Chroma: %x\n"), mbCode->MB[uMB].IntraMB.LumaIntraPredModes[0], mbCode->MB[uMB].IntraMB.LumaIntraPredModes[1],
                mbCode->MB[uMB].IntraMB.LumaIntraPredModes[2], mbCode->MB[uMB].IntraMB.LumaIntraPredModes[3], mbCode->MB[uMB].IntraMB.ChromaIntraPredMode);
        int flags = mbCode->MB[uMB].IntraMB.IntraPredAvailFlags;
        msdk_fprintf(f, MSDK_CHAR(" IntraPredAvailable: %c %c %c (%2x)\n"), (flags&1)?'D':'-', (flags&4)?'B':'-', (flags&2)?'C':'-', flags);
        msdk_fprintf(f, MSDK_CHAR("                   : %c\n"), (flags&16)?'A':'-');

        msdk_fprintf(f, MSDK_CHAR("    reserved:[%d%d%d %3x] %5x %6x\n"), mbCode->MB[uMB].Reserved00, mbCode->MB[uMB].Reserved01, mbCode->MB[uMB].Reserved02, mbCode->MB[uMB].Reserved03,
                mbCode->MB[uMB].Reserved30, mbCode->MB[uMB].IntraMB.Reserved60);
    } else {
        msdk_fprintf(f, MSDK_CHAR(" InterMbMode:%d skip:%d direct:%x\n"), mbCode->MB[uMB].InterMbMode, mbCode->MB[uMB].MBSkipFlag, mbCode->MB[uMB].Direct8x8Pattern);
        for (int bl=0; bl<4; bl++) {
            msdk_fprintf(f, MSDK_CHAR(" %d: shape:%d pmode:%d ref:(%d %d)\n"), bl,
                    (mbCode->MB[uMB].InterMB.SubMbShapes>>(bl*2))&3, (mbCode->MB[uMB].InterMB.SubMbPredModes>>(bl*2))&3,
                    (mfxI8)mbCode->MB[uMB].InterMB.RefIdx[0][bl], (mfxI8)mbCode->MB[uMB].InterMB.RefIdx[1][bl]);
            for(int sb=0; sb<4; sb++)
                msdk_fprintf(f, MSDK_CHAR("  (%3d %3d)  (%3d %3d)\n"), mvs->MB[uMB].MV[bl*4+sb][0].x, mvs->MB[uMB].MV[bl*4+sb][0].y,
                        mvs->MB[uMB].MV[bl*4+sb][1].x, mvs->MB[uMB].MV[bl*4+sb][1].y);

        }
        msdk_fprintf(f, MSDK_CHAR("    reserved:[%d%d%d %3x] %5x %4x\n"), mbCode->MB[uMB].Reserved00, mbCode->MB[uMB].Reserved01, mbCode->MB[uMB].Reserved02, mbCode->MB[uMB].Reserved03,
                mbCode->MB[uMB].Reserved30, mbCode->MB[uMB].InterMB.Reserved40);
    }
    int n = sizeof(mbCode->MB[uMB].reserved2)/sizeof(mbCode->MB[uMB].reserved2[0]);
    msdk_fprintf(f, MSDK_CHAR("    reserved2[%d]:"), n);
    for (int i=0; i<n; i++) msdk_fprintf(f, MSDK_CHAR(" 0x%8.8x"), mbCode->MB[uMB].reserved2[i]);
    msdk_fprintf(f, MSDK_CHAR("\n"));

    fclose(f);
}
#endif //DUMP_MB_DSO

// Unlike ENC, SubMbPredModes from StreamOut are populated for all 4 8x8 blocks - uses all 8 bits.
// It is more convenient for our computations, and we used while inside current frame.
// PAK considers it as not populated, so we pack values in the end of the loop to have 1 per prediction partition.
// SubMbPredModes from reference frames are packed (not populated), so there are additional code for directMV computation

// pTaskList for CEncodingPipeline::m_inputTasks;
mfxStatus ResetDirect(iTask * task, iTaskPool *pTaskList)
{
    MSDK_CHECK_POINTER(task,                   MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(task->ENC_in.InSurface, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(pTaskList,              MFX_ERR_NULL_PTR);

    mfxFrameInfo* fi = &task->ENC_in.InSurface->Info;

    mfxI32 wmb = (fi->Width +15)>>4;
    mfxI32 hmb = (fi->Height+15)>>4;

    const mfxExtFeiPPS         * pps         = reinterpret_cast<mfxExtFeiPPS        *>(task->bufs->PB_bufs.in.getBufById(MFX_EXTBUFF_FEI_PPS,       0));
    const mfxExtFeiSliceHeader * sliceHeader = reinterpret_cast<mfxExtFeiSliceHeader*>(task->bufs->PB_bufs.in.getBufById(MFX_EXTBUFF_FEI_SLICE,     0));

    const mfxExtFeiEncMV       * mvs         = reinterpret_cast<mfxExtFeiEncMV      *>(task->bufs->PB_bufs.out.getBufById(MFX_EXTBUFF_FEI_ENC_MV,   0));
    const mfxExtFeiPakMBCtrl   * mbCode      = reinterpret_cast<mfxExtFeiPakMBCtrl  *>(task->bufs->PB_bufs.out.getBufById(MFX_EXTBUFF_FEI_PAK_CTRL, 0));

    MSDK_CHECK_POINTER(pps,         MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(sliceHeader, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(mvs,         MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(mbCode,      MFX_ERR_NULL_PTR);

    if (sliceHeader->NumSlice == 0)
        return MFX_ERR_INVALID_VIDEO_PARAM;

    const mfxExtFeiEncMV     * refmvs    = NULL;
    const mfxExtFeiPakMBCtrl * refmbCode = NULL;

    for (mfxI32 uMB = 0, uMBy = 0, slice = -1, nextSliceMB = 0; uMBy < hmb; uMBy++) for (mfxI32 uMBx = 0; uMBx < wmb; uMBx++, uMB++) {
        while (nextSliceMB <= uMB && slice+1 < sliceHeader->NumSlice) {
            slice++; // next or first slice
            nextSliceMB = sliceHeader->Slice[slice].MBAddress + sliceHeader->Slice[slice].NumMBs;

            if (!B_SLICE(sliceHeader->Slice[slice].SliceType)) break;

            int ridx = sliceHeader->Slice[slice].RefL1[0].Index;
#if MFX_VERSION >= 1023
            int fidx = pps->DpbBefore[ridx].Index;
#else
            int fidx = pps->ReferenceFrames[ridx];
#endif // MFX_VERSION >= 1023
            mfxFrameSurface1 *refSurface = task->PAK_in.L0Surface[fidx];

            // find iTask of L1[0]
            iTask * reftask = pTaskList->GetTaskByPAKOutputSurface(refSurface);

            if (reftask) {
                refmvs    = (mfxExtFeiEncMV*)GetExtBuffer(reftask->PAK_in.ExtParam, reftask->PAK_in.NumExtParam, MFX_EXTBUFF_FEI_ENC_MV);
                MSDK_CHECK_POINTER(refmvs, MFX_ERR_NULL_PTR);

                refmbCode = (mfxExtFeiPakMBCtrl*)GetExtBuffer(reftask->PAK_in.ExtParam, reftask->PAK_in.NumExtParam, MFX_EXTBUFF_FEI_PAK_CTRL);
                MSDK_CHECK_POINTER(refmbCode, MFX_ERR_NULL_PTR);
            }
        }
        if (slice >= sliceHeader->NumSlice) return MFX_ERR_INVALID_VIDEO_PARAM;

        // Inter prediction
        if (mbCode->MB[uMB].IntraMbFlag == 0) {

            mfxI16Pair mvSkip[2];
            mfxU8 refSkip[2];
            mfxU8 canSkip = 0; // true if MV==skipMV ref==0 and 16x16 (==directMV and 8x8 for B)
            mfxU8 canSkipSb[4]; // true if MV==directMV for 8x8 block
            mfxI16Pair mvDirect[2][16]; // [list][raster sb]
            mfxU8 refDirect[2][16];
            int smodes = mbCode->MB[uMB].InterMB.SubMbPredModes;

            // predicted ref/MV to check if it is true skip
            mvSkip[0] = mvSkip[1] = zeroMv;
            refSkip[0] = 0;
            refSkip[1] = P_SLICE(sliceHeader->Slice[slice].SliceType) ? 255 :0;
            // fill unused refidx with -1
            for (int sb=0; sb<4; sb++) {
                mfxI32 modes = (smodes >> sb*2) & 3;
                if (modes == 0) mbCode->MB[uMB].InterMB.RefIdx[1][sb] = (P_SLICE(sliceHeader->Slice[slice].SliceType)) ? 0 : 255;
                else if (modes == 1) mbCode->MB[uMB].InterMB.RefIdx[0][sb] = 255;
            }

            if (P_SLICE(sliceHeader->Slice[slice].SliceType)) {
                if (uMBx > 0 && uMBy > 0 &&
                    (mbCode->MB[uMB-1].IntraMbFlag || mbCode->MB[uMB-1].InterMB.RefIdx[0][1] != 0 ||
                        (mvs->MB[uMB-1].MV[5][0].x | mvs->MB[uMB-1].MV[5][0].y) != 0) &&
                    (mbCode->MB[uMB-wmb].IntraMbFlag || mbCode->MB[uMB-wmb].InterMB.RefIdx[0][2] != 0 ||
                        (mvs->MB[uMB-wmb].MV[10][0].x | mvs->MB[uMB-wmb].MV[10][0].y) != 0) )
                { // left and top neighbour MVs are !0
                    MVPrediction(uMB, wmb, uMBx, uMBy, 0, 0, mvs, mbCode, &mvSkip[0]);
                    refSkip[1] = 255;
                    canSkip = (mbCode->MB[uMB].InterMbMode == 0 && mbCode->MB[uMB].InterMB.RefIdx[0][0] == 0 &&
                        mvs->MB[uMB].MV[0][0].x == mvSkip[0].x && mvs->MB[uMB].MV[0][0].y == mvSkip[0].y);
                }
            } else { // B-slice
                RefPrediction(uMB, wmb, uMBx, uMBy, mbCode, refSkip);
                if (refSkip[0] < 32) MVPrediction(uMB, wmb, uMBx, uMBy, 0, refSkip[0], mvs, mbCode, &mvSkip[0]);
                if (refSkip[1] < 32) MVPrediction(uMB, wmb, uMBx, uMBy, 1, refSkip[1], mvs, mbCode, &mvSkip[1]);

                // Direct predictors, frame only (mb_col == uMB etc)
                mfxI32 noRefs = refSkip[0] >= 32 && refSkip[1] >= 32;
                canSkip = 1;
                for (mfxI32 sb = 0; sb < 4; sb++) {
                    canSkipSb[sb] = 1;
                    for (mfxI32 ypos=0; ypos < 2; ypos++) for (mfxI32 xpos=0; xpos < 2; xpos++) { // 4 4x4 blocks
                        mfxI32 zeroPred[2] = {0, 0};
                        mfxI32 bl = (ypos + (sb&2))*4 + (sb&1)*2 + xpos; // raster 4x4 block
                        mfxI32 sbColz = rasterToZ[sbDirect[1/*sps->Direct8x8InferenceFlag*/][bl]];

                        if (!noRefs && refmbCode && !refmbCode->MB[uMB].IntraMbFlag && refmvs /*&& is_l1_pic_short_term*/) // no long term
                        {
                            mfxU8 pmodesCol = (refmbCode->MB[uMB].MbType == 22) ?
                                ((refmbCode->MB[uMB].InterMB.SubMbPredModes >> (sb*2)) & 3) :
                                sub_mb_type[refmbCode->MB[uMB].MbType][sb];
                            mfxU8 ref_col = 255;
                            const mfxI16Pair *mv_col;
                            if (pmodesCol != 1) { // L0 or Bi
                                ref_col = refmbCode->MB[uMB].InterMB.RefIdx[0][sb];
                                mv_col = &refmvs->MB[uMB].MV[sbColz][0];
                            } else {
                                ref_col = refmbCode->MB[uMB].InterMB.RefIdx[1][sb];
                                mv_col = &refmvs->MB[uMB].MV[sbColz][1];
                            }
                            if (ref_col == 0 &&
                                mv_col->x >= -1 && mv_col->x <= 1 &&
                                mv_col->y >= -1 && mv_col->y <= 1)
                            {
                                zeroPred[0] = (refSkip[0] == 0);
                                zeroPred[1] = (refSkip[1] == 0);
                            }
                        }
                        mvDirect[0][bl] = (refSkip[0] < 32 && !zeroPred[0]) ? mvSkip[0] : zeroMv;
                        refDirect[0][bl] = noRefs ? 0 : refSkip[0];
                        mvDirect[1][bl] = (refSkip[1] < 32 && !zeroPred[1]) ? mvSkip[1] : zeroMv;
                        refDirect[1][bl] = noRefs ? 0 : refSkip[1];
                        if (canSkipSb[sb]) {
                            mfxI32 blz = sb*4 + ypos*2 + xpos;
                            mfxI32 modes = (smodes >> sb*2) & 3;
                            for (int l=0; l<2; l++) {
                                mfxI32 sameMV = ((refDirect[l][bl]>=32 && modes == 1-l) || // both unused
                                    //((l+1) & (modes+1)) && // direction is used (unused RefIdx is 0-filled
                                    (mbCode->MB[uMB].InterMB.RefIdx[l][sb] == refDirect[l][bl] && // same ref
                                    mvs->MB[uMB].MV[blz][l].x == mvDirect[l][bl].x && mvs->MB[uMB].MV[blz][l].y == mvDirect[l][bl].y)); // same MV
                                canSkipSb[sb] &= sameMV;
                            }
                        }
                    }
                    canSkip &= canSkipSb[sb];
                }

            } // direct predictors for B

            // for 8x8 mode populate MV inside 8x8 block from 1st subblock, if there were subshapes
            mfxFeiPakMBCtrl& mb = mbCode->MB[uMB];
            mfxExtFeiEncMV::mfxExtFeiEncMVMB& mv = mvs->MB[uMB];
            if ( mb.InterMbMode == 3 && mb.InterMB.SubMbShapes != 0) {
                for (int bl = 0; bl < 4; bl++)
                    for (int list = 0; list < 2; list++)
                        for (int sb = 0; sb < 4; sb ++)
                            mv.MV[bl*4+sb][list] = mv.MV[bl*4][list];
            }

            mb.InterMB.SubMbShapes = 0;

            // mark directs
            mb.Direct8x8Pattern = 0; // used only with B_8x8
            if (B_SLICE(sliceHeader->Slice[slice].SliceType)) {
                if (canSkip) {
                    // to be 16x16 direct (all sb are direct)
                    // for PAK we set it as B_8x8 and all 4 sb marked as direct
                    // PAK will code it as direct16x16 or skip16x16
                    mb.MbType = 22;
                    mb.InterMbMode = 3;
                    mb.Direct8x8Pattern = 15;
                } else if (mb.InterMbMode == 3) for (int bl = 0; bl < 4; bl++) {
                    // for B_8x8 mark direct sb
                    if (canSkipSb[bl])
                        mb.Direct8x8Pattern |= 1 << bl;
                }
            }

            // recombination: after sub-shapes changed to 0, some splits can be eliminated
            if (mb.InterMbMode != 0 && mb.Direct8x8Pattern != 15) {
                mfxU32 newmode = mb.InterMbMode;
                bool difx0 = (mb.InterMB.RefIdx[0][0] != mb.InterMB.RefIdx[0][1]) || (mv.MV[0][0].x != mv.MV[4][0].x)  ||  (mv.MV[0][0].y != mv.MV[4][0].y);
                bool dify0 = (mb.InterMB.RefIdx[0][0] != mb.InterMB.RefIdx[0][2]) || (mv.MV[0][0].x != mv.MV[8][0].x)  ||  (mv.MV[0][0].y != mv.MV[8][0].y);
                bool difx1 = (mb.InterMB.RefIdx[0][2] != mb.InterMB.RefIdx[0][3]) || (mv.MV[8][0].x != mv.MV[12][0].x) ||  (mv.MV[8][0].y != mv.MV[12][0].y);
                bool dify1 = (mb.InterMB.RefIdx[0][1] != mb.InterMB.RefIdx[0][3]) || (mv.MV[4][0].x != mv.MV[12][0].x) ||  (mv.MV[4][0].y != mv.MV[12][0].y);
                if (B_SLICE(sliceHeader->Slice[slice].SliceType)) {
                    difx0 = difx0 || (mb.InterMB.RefIdx[1][0] != mb.InterMB.RefIdx[1][1]) || (mv.MV[0][1].x != mv.MV[4][1].x)  ||  (mv.MV[0][1].y != mv.MV[4][1].y);
                    dify0 = dify0 || (mb.InterMB.RefIdx[1][0] != mb.InterMB.RefIdx[1][2]) || (mv.MV[0][1].x != mv.MV[8][1].x)  ||  (mv.MV[0][1].y != mv.MV[8][1].y);
                    difx1 = difx1 || (mb.InterMB.RefIdx[1][2] != mb.InterMB.RefIdx[1][3]) || (mv.MV[8][1].x != mv.MV[12][1].x) ||  (mv.MV[8][1].y != mv.MV[12][1].y);
                    dify1 = dify1 || (mb.InterMB.RefIdx[1][1] != mb.InterMB.RefIdx[1][3]) || (mv.MV[4][1].x != mv.MV[12][1].x) ||  (mv.MV[4][1].y != mv.MV[12][1].y);
                }
                if (!difx0 && !difx1)
                    if (!dify0 && !dify1) newmode = 0; //16x16
                    else newmode = 1; //16x8
                else if (!dify0 && !dify1) newmode = 2; //8x16

                if (newmode != mb.InterMbMode) { // changed to bigger partitions. Have to derive MbType
                    mb.Direct8x8Pattern = 0; // anyway not 8x8
                    if (P_SLICE(sliceHeader->Slice[slice].SliceType) || (mb.InterMB.RefIdx[1][0]>=32 && mb.InterMB.RefIdx[1][3]>=32)) // L0 only
                        mb.MbType = newmode==0 ? 1 : newmode + 3; // 012 -> 145 (P types mapped to B types)
                    else { // B-modes
                        int p0 = (mb.InterMB.RefIdx[0][0]<32) + 2*(mb.InterMB.RefIdx[1][0]<32); // 1st partition type
                        int p1 = (mb.InterMB.RefIdx[0][3]<32) + 2*(mb.InterMB.RefIdx[1][3]<32); // 2nd partition type
                        if (newmode==0) // 16x16
                            mb.MbType = p0;
                        else if (p0==1) // 1st partition is fwd
                            mb.MbType = p1 * 4 + newmode-1;
                        else if (p0==3) // 1st partition is bidir
                            mb.MbType = p1 * 2 + newmode-1 + 14;
                        else // p0==2 // bwd
                            mb.MbType = ((p1==1) ? 10 : ((p1==2) ? 6 : 14)) + newmode-1;
                    }
                }
            }
        } // inter

    } // MB loop

    // restore SubMbPredModes from populated (DSO) to packed (PAK)
    if (mbCode && sliceHeader)
    for (mfxI32 uMB = 0, uMBy = 0, slice = -1, nextSliceMB = 0; uMBy < hmb; uMBy++) for (mfxI32 uMBx = 0; uMBx < wmb; uMBx++, uMB++) {
        while (nextSliceMB <= uMB && slice+1 < sliceHeader->NumSlice) {
            slice++;
            nextSliceMB = sliceHeader->Slice[slice].MBAddress + sliceHeader->Slice[slice].NumMBs;
        }
        if (B_SLICE(sliceHeader->Slice[slice].SliceType))
        if (!mbCode->MB[uMB].IntraMbFlag) {
            int smodes = mbCode->MB[uMB].InterMB.SubMbPredModes, newsmodes = smodes;
            switch (mbCode->MB[uMB].InterMbMode) {
                case 0:
                    newsmodes = smodes&3; // Aaaa
                    break;
                case 1:
                    newsmodes = (smodes>>2)&15; // aABb
                    break;
                case 2:
                    newsmodes = smodes&15; // ABab
                    break;
                default: break; // nothing to do for 8x8 // ABCD
            }
            //if (smodes != newsmodes)
                mbCode->MB[uMB].InterMB.SubMbPredModes = (mfxU8)newsmodes;
        }
    }

#ifdef DUMP_MB_DSO

#define DUMPOUT MSDK_STRING("/home/dumpfilename.txt")

    int num = task->ENC_in.InSurface->Data.FrameOrder;
    if (num == 2) {
    StartDumpMB(DUMPOUT, num, 1);
        DumpMB(DUMPOUT, task, 2365);
        DumpMB(DUMPOUT, task, 2366);
        DumpMB(DUMPOUT, task, 2367);
        DumpMB(DUMPOUT, task, 2485);
        DumpMB(DUMPOUT, task, 2486);
    }
#endif //DUMP_MB_DSO

    return MFX_ERR_NONE;
}

mfxStatus FEI_EncPakInterface::PakOneStreamoutFrame(iTask *eTask, mfxU8 QP, iTaskPool *pTaskList)
{
    MFX_ITT_TASK("PakOneStreamoutFrame");

    MSDK_CHECK_POINTER(eTask,                   MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(eTask->PAK_in.InSurface, MFX_ERR_NULL_PTR);

    mfxU32 numOfFields = eTask->m_fieldPicFlag ? 2 : 1;

    mfxExtFeiDecStreamOut* pExtBufDecodeStreamout = NULL;
    for (mfxU16 i = 0; i < eTask->PAK_in.InSurface->Data.NumExtParam; ++i)
    {
        if (eTask->PAK_in.InSurface->Data.ExtParam[i]->BufferId == MFX_EXTBUFF_FEI_DEC_STREAM_OUT)
        {
            pExtBufDecodeStreamout = reinterpret_cast<mfxExtFeiDecStreamOut*>(eTask->PAK_in.InSurface->Data.ExtParam[i]);
            break;
        }
    }
    MSDK_CHECK_POINTER(pExtBufDecodeStreamout, MFX_ERR_NULL_PTR);

    mfxStatus sts = MFX_ERR_NONE;

    mfxExtFeiPakMBCtrl* feiEncMBCode = NULL;
    mfxExtFeiEncMV*     feiEncMV     = NULL;

    for (mfxU32 fieldId = 0; fieldId < numOfFields; ++fieldId)
    {
        mv_data_length_offset = 0;

        /* get mfxExtFeiPakMBCtrl buffer */
        feiEncMBCode = reinterpret_cast<mfxExtFeiPakMBCtrl*>(eTask->bufs->PB_bufs.out.getBufById(MFX_EXTBUFF_FEI_PAK_CTRL, fieldId));
        MSDK_CHECK_POINTER(feiEncMBCode, MFX_ERR_NULL_PTR);

        /* get mfxExtFeiEncMV buffer */
        feiEncMV = reinterpret_cast<mfxExtFeiEncMV*>(eTask->bufs->PB_bufs.out.getBufById(MFX_EXTBUFF_FEI_ENC_MV, fieldId));
        MSDK_CHECK_POINTER(feiEncMV, MFX_ERR_NULL_PTR);

        /* repack streamout output to PAK input */
        for (mfxU32 i = 0; i < feiEncMBCode->NumMBAlloc; ++i)
        {
            /* temporary, this flag is not set at all by driver */
            (pExtBufDecodeStreamout->MB + fieldId*feiEncMBCode->NumMBAlloc + i)->IsLastMB = (i == (feiEncMBCode->NumMBAlloc - 1));

            /* NOTE: streamout holds data for both fields in MB array (first NumMBAlloc for first field data, second NumMBAlloc for second field) */
            sts = RepackStremoutMB2PakMB(pExtBufDecodeStreamout->MB + fieldId*feiEncMBCode->NumMBAlloc + i, feiEncMBCode->MB + i, QP);
            MSDK_CHECK_STATUS(sts, "Decode StreamOut to PAK-object repacking: RepackStremoutMB2PakMB failed");

            sts = RepackStreamoutMV(pExtBufDecodeStreamout->MB + fieldId*feiEncMBCode->NumMBAlloc + i, feiEncMV->MB + i);
            MSDK_CHECK_STATUS(sts, "Decode StreamOut to PAK-object repacking: RepackStreamoutMV failed");
        }

        sts = ResetDirect(eTask, pTaskList);
        MSDK_CHECK_STATUS(sts, "Decode StreamOut to PAK-object repacking: ResetDirect failed");
    }

    return sts;
}
