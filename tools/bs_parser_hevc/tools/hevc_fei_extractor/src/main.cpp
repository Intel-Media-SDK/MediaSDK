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

#include <iostream>

#if MFX_VERSION >= MFX_VERSION_NEXT

#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <bs_parser++.h>
#include "mfxfeihevc.h"

#define MAX_PU_NUM 4

struct CUBlock
{
    Bs32u m_AdrX = 0;
    Bs32u m_AdrY = 0;
    Bs32u m_Log2CbSize = 0;

    CUBlock(Bs32u adrX, Bs32u adrY, Bs32u log2CbSize) :
        m_AdrX(adrX),
        m_AdrY(adrY),
        m_Log2CbSize(log2CbSize)
    {};
};

class FileHandler
{
private:
    bool m_errorSts = false;
    FILE* m_pFile = nullptr;

public:
    FileHandler(char const* fileName, char const* mode)
    {
        if (!fileName) {
            printf("\nERROR: Unable to read file name with CU/CTU structures\n");
            m_errorSts = true;// Error sts
            return;
        }

        if (!mode) {
            printf("\nERROR: Unable to read file open mode\n");
            m_errorSts = true;// Error sts
            return;
        }

        if ((m_pFile = fopen(fileName, mode)) == nullptr) {
            printf("\nERROR: Unable to open the %s file in the FileHandler::FileHandler\n", fileName);
            m_errorSts = true;// Error sts
            return;
        }
    };

    ~FileHandler()
    {
        if (m_pFile != nullptr)
            fclose(m_pFile);
    };

    bool CheckStatus()
    {
        return m_errorSts;
    };

    template< class T > void Write(T& structure)
    {
        size_t elementsWritten = fwrite(&structure, sizeof(structure), 1, m_pFile);
        if (elementsWritten != 1) {
            printf("\nERROR: File wasn't written in the FileHandler::WriteFile\n");
            m_errorSts = true;// Error sts
        }
    };
};

using namespace BS_HEVC2;

inline bool IsHEVCSlice(Bs32u nut) { return (nut <= 21) && ((nut < 10) || (nut > 15)); }

#define CHECK_STATUS(code,sts)\
  bs_sts = (code);\
  if(bs_sts != (sts)){\
    printf("FAILED in %s at %s: %i\nReturn code = %i\n", #code, __FILE__, __LINE__, bs_sts);\
    return bs_sts;\
  }

#define ALIGN(value, alignment) (alignment) * ( (value) / (alignment) + (((value) % (alignment)) ? 1 : 0))

int printUsage(char* argv[])
{
    printf("Parser for HEVC bit-streams dumps fei-specific information.\n");
    printf("Usage: %s <stream_name> <fei_hevc_pak_ctu> <fei_hevc_pak_cu>\n", argv[0]);
    printf("   or: %s <stream_name> -pic_file <pic_refinfo>\n", argv[0]);
    printf("   or: %s <stream_name> -multi_pak_str <multi_repack_output_file>\n", argv[0]);
    return 1;
}

BSErr ConvertPredModeToFeiPredMode(Bs16u const predModeParser/*in*/, mfxFeiHevcPakCuRecordV0& pakCuRecorderV0/*out*/)
{
    switch (predModeParser) {
    case MODE_INTER:
        pakCuRecorderV0.PredMode = (1 & 0x01);
        break;
    case MODE_INTRA:
        pakCuRecorderV0.PredMode = (0 & 0x01);
        break;
    case MODE_SKIP:
        pakCuRecorderV0.PredMode = (1 & 0x01);
        break;
    default:
        return BS_ERR_INVALID_PARAMS;
    }
    return BS_ERR_NONE;
}

mfxU32 GetIntraChromaModeFEI(Bs8u intraPredModeC_0_0)
{
    switch (intraPredModeC_0_0)
    {
    case 0://Planar
        return 2;
    case 1://DC
        return 5;
    case 10://Horiz
        return 4;
    case 26://Vertic
        return 3;
    default://DM
        return 0;
    }
}

void SetIntraPredModeFEI(const CU* pCU, mfxFeiHevcPakCuRecordV0& pakCuRecorderV0)
{
    if(pCU == nullptr)
        throw std::string("ERROR: SetIntraPredModeFEI: pCU is equal to nullptr");

    // Luma
    if (pCU->IntraPredModeY[0][0] > 34
        || pCU->IntraPredModeY[1][0] > 34
        || pCU->IntraPredModeY[0][1] > 34
        || pCU->IntraPredModeY[1][1] > 34)
    {
        throw std::string("ERROR: SetIntraPredModeFEI: incorrect IntraPredModeY[X][X]\n");
    }

    pakCuRecorderV0.IntraMode0 = pCU->IntraPredModeY[0][0];
    pakCuRecorderV0.IntraMode1 = pCU->IntraPredModeY[1][0];
    pakCuRecorderV0.IntraMode2 = pCU->IntraPredModeY[0][1];
    pakCuRecorderV0.IntraMode3 = pCU->IntraPredModeY[1][1];

    // Chroma
    // For ChromaArrayType != 3 (4:4:4) all elements of the array intra_chroma_pred_mode[2][2] are equal
    // Table 7.3.8.5 from ITU-T H.265 (V4)
    // HEVC FEI ENCODE on SKL supports 4:2:0 mode only
    if (pCU->IntraPredModeC[0][0] > 34)
    {
        throw std::string("ERROR: SetIntraPredModeFEI: incorrect IntraPredModeC[0][0]");
    }

    pakCuRecorderV0.IntraChromaMode = GetIntraChromaModeFEI(pCU->IntraPredModeC[0][0]);
}

void SetLevel2(mfxFeiHevcPakCtuRecordV0& pakCtuRecordV0, Bs32u& startShift, Bs32u ctbLog2SizeY, Bs32u log2CbSize, Bs32u idxQuadTreeLevel2, Bs32u idxQuadTreeLevel1)
{
    if (log2CbSize == ctbLog2SizeY - 2)
    {
        startShift++;
    }
    else if (log2CbSize == ctbLog2SizeY - 3)
    {
        switch (idxQuadTreeLevel1)
        {
        case 0:
            pakCtuRecordV0.SplitLevel2Part0 |= 1 << idxQuadTreeLevel2;
            break;
        case 1:
            pakCtuRecordV0.SplitLevel2Part1 |= 1 << idxQuadTreeLevel2;
            break;
        case 2:
            pakCtuRecordV0.SplitLevel2Part2 |= 1 << idxQuadTreeLevel2;
            break;
        case 3:
            pakCtuRecordV0.SplitLevel2Part3 |= 1 << idxQuadTreeLevel2;
            break;
        }
        startShift += 4;
    }
    return;
}

void SetLevel1(mfxFeiHevcPakCtuRecordV0& pakCtuRecordV0, Bs32u& startShift, Bs32u ctbLog2SizeY, const std::vector<CUBlock>& vecCUs, Bs32u idxQuadTreeLevel1)
{
    Bs32u log2CbSize = vecCUs.at(startShift).m_Log2CbSize;

    if (log2CbSize == ctbLog2SizeY - 1)
    {
        startShift++;
    }
    else if ((log2CbSize == ctbLog2SizeY - 2) || (log2CbSize == ctbLog2SizeY - 3))
    {
        pakCtuRecordV0.SplitLevel1 |= 1 << idxQuadTreeLevel1;

        for (Bs32u idxQuadTreeLevel2 = 0; idxQuadTreeLevel2 < 4; ++idxQuadTreeLevel2)
        {
            SetLevel2(pakCtuRecordV0, startShift, ctbLog2SizeY, vecCUs.at(startShift).m_Log2CbSize, idxQuadTreeLevel2, idxQuadTreeLevel1);
        }
    }
    else
        throw std::string("ERROR: SetLevel1: incorrect Cb size");

    return;
}

bool IsInQuarter(Bs32u idxQuadTreeLevel1, CUBlock cu, Bs32u ctbLog2SizeY)
{
    Bs32u ctuSize = 1 << ctbLog2SizeY;
    Bs32u ctuQuarterLog2SizeY = ctbLog2SizeY - 1;
    Bs32u adrXInsideCTU = cu.m_AdrX % ctuSize;
    Bs32u adrYInsideCTU = cu.m_AdrY % ctuSize;

    return (((adrYInsideCTU >> ctuQuarterLog2SizeY) << 1) + (adrXInsideCTU >> ctuQuarterLog2SizeY)) == idxQuadTreeLevel1;
}

void SetLevel0(mfxFeiHevcPakCtuRecordV0& pakCtuRecordV0, Bs32u ctbLog2SizeY, const std::vector<CUBlock>& vecCUs)
{
    Bs32u startShift = 0;
    if (vecCUs.at(startShift).m_Log2CbSize == ctbLog2SizeY)
        return;
    else
    {
        pakCtuRecordV0.SplitLevel0 = 1;
        // If we have 4 parts of the CTU, we need to check each part in the loop
        for (Bs32u idxQuadTreeLevel1 = 0; idxQuadTreeLevel1 < 4; ++idxQuadTreeLevel1)
        {
            // This check is needed for work with different resolution.
            // If last CTU in the line hasn't got right quarters
            // or CTU in the last line hasn't got bottom quarters.
            // Function will be called only for first CU in quarter of the CTU
            if (startShift >= vecCUs.size() || !IsInQuarter(idxQuadTreeLevel1, vecCUs.at(startShift), ctbLog2SizeY))
                continue;
            SetLevel1(pakCtuRecordV0, startShift, ctbLog2SizeY, vecCUs, idxQuadTreeLevel1);
        }
        return;
    }
}

inline void SetInterpredIdc(mfxFeiHevcPakCuRecordV0& pakCuRecordV0, Bs16u interpredIdc, Bs32u countPU)
{
    if (interpredIdc >= 3)
        throw std::string("ERROR: SetInterpredIdc: unsupported value for InterpredIdc");
    else
        pakCuRecordV0.InterpredIdc |= interpredIdc << (2 * countPU);
}

inline void SetPURefIdx(mfxFeiHevcPakCuRecordV0& pakCuRecordV0, PU* pPU, Bs32u countPU)
{
    switch (countPU)
    {
    case 0:
        pakCuRecordV0.RefIdx[0].Ref0 = pPU->ref_idx_l0;
        pakCuRecordV0.RefIdx[1].Ref0 = pPU->ref_idx_l1;
        break;
    case 1:
        pakCuRecordV0.RefIdx[0].Ref1 = pPU->ref_idx_l0;
        pakCuRecordV0.RefIdx[1].Ref1 = pPU->ref_idx_l1;
        break;
    case 2:
        pakCuRecordV0.RefIdx[0].Ref2 = pPU->ref_idx_l0;
        pakCuRecordV0.RefIdx[1].Ref2 = pPU->ref_idx_l1;
        break;
    case 3:
        pakCuRecordV0.RefIdx[0].Ref3 = pPU->ref_idx_l0;
        pakCuRecordV0.RefIdx[1].Ref3 = pPU->ref_idx_l1;
        break;
    }
}

int DumpPicStruct(BS_HEVC2_parser& parser, const char* name)
{
    std::ofstream ofs(name, std::ofstream::out);
    if (!ofs.is_open())
        return 1;

    //common with asg-hevc
    struct PictureInfo
    {
        Bs32s orderCount = -1; // display order
        Bs32s codingOrder = -1;
        Bs32u type = 0;        // IPB, IDR
        Bs32u picStruct = 0;   // TF/BF
    };

    struct RefState
    {
        PictureInfo picture;
        std::vector<Bs32s> DPB;              // stores FrameOrder (POC)
        std::vector<Bs32s> RefListActive[2]; // (POC), any from DPB, can be repeated
    };

    Bs32u recordsWritten = 0;
    Bs32s dispOrderIDR = 0;

    while (true)
    {
        BS_HEVC2::NALU* pNALU = nullptr;
        BSErr bs_sts = parser.parse_next_au(pNALU);

        if (bs_sts == BS_ERR_NOT_IMPLEMENTED)
            continue;
        if (bs_sts != BS_ERR_NONE)
            break;

        RefState record;
        record.DPB.reserve(16);
        record.RefListActive[0].reserve(8);
        record.RefListActive[1].reserve(8);
        record.picture.type = 0;   // updated in different nalu
        record.picture.picStruct = MFX_PICSTRUCT_PROGRESSIVE; // for if no SEI PT

        bool isFirstSlice = true;
        for (auto pNALUIdx = pNALU; pNALUIdx != nullptr; pNALUIdx = pNALUIdx->next)
        {
            if (IsHEVCSlice(pNALUIdx->nal_unit_type) && isFirstSlice) // slice
            {
                if (pNALUIdx->nal_unit_type == NALU_TYPE::IDR_W_RADL || pNALUIdx->nal_unit_type == NALU_TYPE::IDR_N_LP) // IDR, REF
                {
                    dispOrderIDR = recordsWritten; // assume IDR POC == codingOrder
                    record.picture.type |= MFX_FRAMETYPE_IDR | MFX_FRAMETYPE_REF;
                }
                if (pNALUIdx->nal_unit_type >= NALU_TYPE::BLA_W_LP && pNALUIdx->nal_unit_type <= NALU_TYPE::CRA_NUT) // other IRAP -> REF
                {
                    record.picture.type |= MFX_FRAMETYPE_REF;
                }
                else if (pNALUIdx->nal_unit_type <= NALU_TYPE::RASL_R && (pNALUIdx->nal_unit_type & 1)) // !IRAP, REF
                    record.picture.type |= MFX_FRAMETYPE_REF;

                record.picture.orderCount = pNALUIdx->slice->POC + dispOrderIDR;
                if (pNALUIdx->slice->pps->curr_pic_ref_enabled_flag) // check
                    record.picture.type |= MFX_FRAMETYPE_REF;

                record.picture.type |= (pNALUIdx->slice->type == SLICE_TYPE::I) ? MFX_FRAMETYPE_I :
                    ((pNALUIdx->slice->type == SLICE_TYPE::P) ? MFX_FRAMETYPE_P : MFX_FRAMETYPE_B);
                for (Bs32u i = 0; i < pNALUIdx->slice->strps.NumDeltaPocs; i++) // ignore LT
                    record.DPB.push_back(pNALUIdx->slice->DPB[i].POC + dispOrderIDR);
                for (Bs32u i = 0; i < pNALUIdx->slice->num_ref_idx_l0_active; i++)
                    record.RefListActive[0].push_back(pNALUIdx->slice->L0[i].POC + dispOrderIDR);
                for (Bs32u i = 0; i < pNALUIdx->slice->num_ref_idx_l1_active; i++)
                    record.RefListActive[1].push_back(pNALUIdx->slice->L1[i].POC + dispOrderIDR);
                isFirstSlice = false; // use only first slice, others must provide equal parameters
            }

            if (pNALUIdx->nal_unit_type != NALU_TYPE::PREFIX_SEI_NUT) // not SEI
                continue;

            for (auto sei = pNALUIdx->sei; sei != nullptr; sei = sei->next)
            {
                if (sei->payloadType != SEI_TYPE::SEI_PICTURE_TIMING) // not Picture Timing
                    continue;
                switch (sei->pt->pic_struct)
                {
                case PIC_STRUCT::TOP: case PIC_STRUCT::TOP_PREVBOT: case PIC_STRUCT::TOP_NEXTBOT:
                    record.picture.picStruct = MFX_PICSTRUCT_FIELD_TOP;
                    break;
                case PIC_STRUCT::BOT: case PIC_STRUCT::BOT_PREVTOP: case PIC_STRUCT::BOT_NEXTTOP:
                    record.picture.picStruct = MFX_PICSTRUCT_FIELD_BOTTOM;
                    break;
                default:
                    break; // progressive
                }
            }
        }

        if (record.picture.orderCount != -1 && record.picture.type != 0)
        {
            record.picture.codingOrder = recordsWritten;

            const char separator = '|';
            ofs << std::setw(3) << record.picture.orderCount << ' '
                << std::hex << std::showbase << std::setw(3 + 2) << record.picture.type << ' '
                << std::setw(3 + 2) << record.picture.picStruct << std::dec;
            if (ofs.fail())
                throw std::string("ERROR: PicStruct buffer writing failed");
            for (Bs32u list = 0; list < 2; list++)
            {
                ofs << ' ' << separator;
                for (Bs32u i = 0; i < 8; i++)
                    ofs << ' ' << std::setw(3) << (i < record.RefListActive[list].size() ? record.RefListActive[list][i] : -1);
            }
            std::sort(record.DPB.begin(), record.DPB.end()); // to simplify matching
            ofs << ' ' << separator;
            for (Bs32u i = 0; i < 16; i++)
                ofs << ' ' << std::setw(3) << (i < record.DPB.size() ? record.DPB[i] : -1);
            ofs << std::endl;
            recordsWritten ++;
        }
    }

    ofs.close();

    if (recordsWritten==0)
        printf("\nERROR: NO picture structure info in stream\n");

    return (recordsWritten==0); // 0 for OK
}

struct sMultiPak
{
    Bs32u NumBytesInNalUnit;
    Bs8u SliceQP;
};

int DumpMultiPassPak(BS_HEVC2_parser& parser, const char* name)
{
    FILE *fMultiPak = nullptr;
    if ((fMultiPak = fopen(name, "wb")) == nullptr)
        throw std::string("ERROR: Opening multipasspak file failed");

    sMultiPak multiPakInfo;
    BSErr bs_sts = BS_ERR_NONE;
    BS_HEVC2::NALU* pNALU = nullptr;

    while (true)
    {
        bs_sts = parser.parse_next_au(pNALU);

        if (bs_sts == BS_ERR_NOT_IMPLEMENTED)
            continue;
        if (bs_sts)
            break;

        multiPakInfo.NumBytesInNalUnit = 0;
        for (; pNALU; pNALU = pNALU->next)
        {
            if (!IsHEVCSlice(pNALU->nal_unit_type))
                continue;

            if (!pNALU->slice)
                throw std::string("ERROR: Invalid parser pointer");

            multiPakInfo.SliceQP = 0xFF; //0xFF indicates skipping
            for (auto pCTU = pNALU->slice->ctu; pCTU; pCTU = pCTU->Next)
            {
                for (auto pCU = pCTU->Cu; pCU; pCU = pCU->Next)
                {
                    if (pCU->palette_mode_flag)
                    {
                        multiPakInfo.SliceQP = (Bs8u)pCU->QpY;
                        break;
                    }

                    for (auto pTU = pCU->Tu; pTU; pTU = pTU->Next)
                    {
                        if (pTU->cbf_luma
                         || pTU->cbf_cb
                         || pTU->cbf_cb1
                         || pTU->cbf_cr
                         || pTU->cbf_cr1)
                        {
                            multiPakInfo.SliceQP = (Bs8u)pCU->QpY;
                            break;
                        }
                    }

                    if (multiPakInfo.SliceQP != 0xFF)
                        break;
                }

                if (multiPakInfo.SliceQP != 0xFF)
                    break;
            }

            multiPakInfo.NumBytesInNalUnit += pNALU->NumBytesInNalUnit;
        }

        //Write into the Multi-pass PAK file
        size_t sizeWrite = 0;
        sizeWrite = fwrite(&multiPakInfo, sizeof(sMultiPak), 1, fMultiPak);
        if (sizeWrite != 1)
        {
            fclose(fMultiPak);
            throw std::string("ERROR: Writing to multipasspak file failed");
        }
    }

    fclose(fMultiPak);

    CHECK_STATUS(bs_sts, BS_ERR_MORE_DATA);

    return 0;
}

#endif // MFX_VERSION

int main(int argc, char* argv[]) {

#if MFX_VERSION < MFX_VERSION_NEXT
    std::cout << "ERROR: For correct work minimal API MFX_VERSION_NEXT version is required" << std::endl;
    return -1;
#else
    try
    {
        if (argc < 4) {
            return printUsage(argv);
        }
        BSErr bs_sts = BS_ERR_NONE;

        BS_HEVC2_parser parser(PARSE_SSD);

        CHECK_STATUS(parser.open(argv[1]), BS_ERR_NONE);

        if (strcmp(argv[2], "-pic_file") == 0)
        {
            return DumpPicStruct(parser, argv[3]);
        }

        if (strcmp(argv[2], "-multi_pak_str") == 0)
        {
            return DumpMultiPassPak(parser, argv[3]);
        }

        // Opening of the file for CTU
        FileHandler handlerCTU(argv[2], "wb");
        if (handlerCTU.CheckStatus())
            throw std::string("ERROR: main: issue with file opening");

        // Opening of the file for CU
        FileHandler handlerCU(argv[3], "wb");
        if (handlerCU.CheckStatus())
            throw std::string("ERROR: main: issue with file opening");

        // CTU information
        mfxFeiHevcPakCtuRecordV0 pakCtuRecordV0;
        memset(&pakCtuRecordV0, 0, sizeof(pakCtuRecordV0));

        // CU information
        mfxFeiHevcPakCuRecordV0 pakCuRecordV0;
        memset(&pakCuRecordV0, 0, sizeof(pakCuRecordV0));

        BS_HEVC2::NALU* pNALU = nullptr;

        while (true)
        {
            bs_sts = parser.parse_next_au(pNALU);

            if (bs_sts == BS_ERR_NOT_IMPLEMENTED)
                continue;
            if (bs_sts)
                break;

            for (auto pNALUIdx = pNALU; pNALUIdx; pNALUIdx = pNALUIdx->next)
            {
                if (!IsHEVCSlice(pNALUIdx->nal_unit_type))
                    continue;

                auto& slice = *pNALUIdx->slice;

                // CTB_Y size calculating
                Bs32u minCbLog2SizeY = slice.sps->log2_min_luma_coding_block_size_minus3 + 3;
                Bs32u ctbLog2SizeY = minCbLog2SizeY + slice.sps->log2_diff_max_min_luma_coding_block_size;
                Bs32u ctbSizeY = 1 << ctbLog2SizeY;

                // Parameters for FEI compatibility
                Bs32u maxNumCuInCtu = (1 << (slice.sps->log2_diff_max_min_luma_coding_block_size + slice.sps->log2_diff_max_min_luma_coding_block_size));

                // Number CTB_Y in the line
                Bs32u widthInCTU = ALIGN(slice.sps->pic_width_in_luma_samples, ctbSizeY) >> ctbLog2SizeY;

                Bs32u countCTU = 0;
                for (auto pCTU = slice.ctu; pCTU; pCTU = pCTU->Next, ++countCTU)
                {
                    // pakCtuRecordV0 cleaning
                    memset(&pakCtuRecordV0, 0, sizeof(pakCtuRecordV0));

                    std::vector<CUBlock> vecCUs;
                    vecCUs.reserve(maxNumCuInCtu);

                    Bs16u countCU = 0;
                    for (auto pCU = pCTU->Cu; pCU; pCU = pCU->Next, ++countCU)
                    {
                        // pakCuRecordV0 cleaning
                        memset(&pakCuRecordV0, 0, sizeof(pakCuRecordV0));

                        vecCUs.emplace_back((Bs32u)pCU->x, (Bs32u)pCU->y, (Bs32u)pCU->log2CbSize);

                        // Set information about prediction and partition mode
                        CHECK_STATUS(ConvertPredModeToFeiPredMode(pCU->PredMode, pakCuRecordV0), BS_ERR_NONE);
                        pakCuRecordV0.PartMode = pCU->PartMode;// need to investigate

                        // Set information about Intra prediction mode
                        if (pCU->PredMode == MODE_INTRA)
                            SetIntraPredModeFEI(pCU, pakCuRecordV0);

                        // Set information about MVs
                        Bs32u countPU = 0;
                        for (auto pPU = pCU->Pu; pPU; pPU = pPU->Next, ++countPU)
                        {
                            if (countPU >= MAX_PU_NUM) // CU can't include more than 4 PUs
                                throw std::string("ERROR: main: Number of PUs more than 4");

                            SetInterpredIdc(pakCuRecordV0, pPU->inter_pred_idc, countPU);

                            for (Bs32u listIdx = 0; listIdx < 2; listIdx++)
                            {
                                pakCuRecordV0.MVs[listIdx].x[countPU] = pPU->MvLX[listIdx][0];
                                pakCuRecordV0.MVs[listIdx].y[countPU] = pPU->MvLX[listIdx][1];
                            }

                            SetPURefIdx(pakCuRecordV0, pPU, countPU);
                        }

                        // Writing CU information into file
                        handlerCU.Write(pakCuRecordV0);
                        if (handlerCU.CheckStatus())
                            throw std::string("ERROR: main: issue with file writing");
                    }// End for (auto pCU = pCTU->Cu; pCU; pCU = pCU->Next, ++countCU)

                    if (countCU < maxNumCuInCtu)
                    {
                        // Alignment with zero-padding for FEI CU buffer
                        memset(&pakCuRecordV0, 0, sizeof(pakCuRecordV0));

                        for (Bs32u idxEmptyCU = countCU; idxEmptyCU < maxNumCuInCtu; ++idxEmptyCU)
                        {
                            // Writing zero CU information into file
                            handlerCU.Write(pakCuRecordV0);
                            if (handlerCU.CheckStatus())
                                throw std::string("ERROR: main: issue with file writing");
                        }
                    }

                    SetLevel0(pakCtuRecordV0, ctbLog2SizeY, vecCUs);

                    pakCtuRecordV0.CuCountMinus1 = countCU - 1;
                    pakCtuRecordV0.CtuAddrX = countCTU % widthInCTU;
                    pakCtuRecordV0.CtuAddrY = countCTU / widthInCTU;

                    // Writing CTU information into file
                    handlerCTU.Write(pakCtuRecordV0);
                    if (handlerCTU.CheckStatus())
                        throw std::string("ERROR: main: issue with file writing");
                } // End for (auto pCTU = slice.ctu; pCTU; pCTU = pCTU->Next)
            }// End for (auto pNALUIdx = pNALU; pNALUIdx; pNALUIdx = pNALUIdx->next)
        }// End while(1)
        CHECK_STATUS(bs_sts, BS_ERR_MORE_DATA);
    }
    catch (std::string & e) {
        std::cout << e << std::endl;
        return 1;
    }
    return 0;

#endif // MFX_VERSION
}
