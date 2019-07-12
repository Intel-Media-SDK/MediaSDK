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

#ifndef __ASG_HEVC_UTILS_H__
#define __ASG_HEVC_UTILS_H__

#include "mfxvideo.h"

#if MFX_VERSION >= MFX_VERSION_NEXT

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <iomanip>

#include "mfxdefs.h"
#include "mfxfeihevc.h"
#include "sample_defs.h"

#define CHECK_THROW_ERR(STS, MESSAGE) { \
        if (STS < MFX_ERR_NONE)\
            throw std::string(std::string("ERROR: (") + ConvertMfxStatusToString(STS) + std::string(") ") + MESSAGE);\
                else if (STS > MFX_ERR_NONE) \
            std::cout << "WARNING: (" << ConvertMfxStatusToString(STS) << ") " << MESSAGE << std::endl; \
}

#define CHECK_THROW_ERR_BOOL(EXPR, MESSAGE) { \
    if (!EXPR) \
        throw std::string(MESSAGE); \
}

#define ASG_HEVC_ERR_LOW_PASS_RATE 100

#define LUMA_DEFAULT 0
#define CHROMA_DEFAULT 127
#define MAX_GEN_MV_ATTEMPTS 100
#define MVP_BLOCK_SIZE 16
#define MVP_PER_16x16_BLOCK 4
#define MVMVP_SIZE_LIMIT 2048       //Hardware-imposed limit for either of MV/MVP coordinates in quarter-pixel
                                    //units. To be increased in PV5

#define HW_SEARCH_ELEMENT_SIZE 16 //Hardware search element size in pixels (i.e. each 16x16 block in a CTU
                                  //will be searched for in a search window centered on this 16x16 block)

#define LOG2_MV_COMPARE_BASE_BLOCK_SIZE 2   //While comparing MVs of ASG and FEI CTUs, motion vectors will be compared
                                           //on basis of square blocks of this size

#define DELTA_PIXEL_BI_DIRECT 15 // Delta for bi-prediction test. This value will be used as difference between L0 and L1 references PUs
#define SPLIT_LOG_TABLE_SYM_WIDTH 12 //Width of a table field in split testing partition size table output

//For repack ctrl
#define HEVC_MAX_NUMPASSES 8 //Number of additional passes with delta QPs

std::string ConvertMfxStatusToString(mfxStatus sts);

enum REF_LIST_INDEX
{
    L0 = 0,
    L1 = 1
};

template<typename T>
inline T Clip3(const T valueToClip, const T low, const T high)
{
    if (valueToClip < low)
    {
        return low;
    }
    else if (valueToClip > high)
    {
        return high;
    }

    return valueToClip;
}

// Inaccurate limits. For one directional limit is width*height <= 2000; for Bi-dir 1000
const mfxU32 SKL_SW_LIM_OD[2] = { 48, 40 }; // One-directional Motion Estimation Search Window limit; 0 - width, 1 - height
const mfxU32 SKL_SW_LIM_BD[2] = { 32, 32 }; // Bi-directional Motion Estimation Search Window limit; 0 - width, 1 - height


struct PUMotionVector
{
    PUMotionVector(mfxU8 refl0, mfxU8 refl1, mfxI16 MV1_x, mfxI16 MV1_y, mfxI16 MV2_x, mfxI16 MV2_y)
    {
        RefIdx = { refl0, refl1 };

        MV[0] = { MV1_x, MV1_y };
        MV[1] = { MV2_x, MV2_y };
    }

    PUMotionVector() : PUMotionVector(0, 0, 0, 0, 0, 0) {}

    PUMotionVector(const PUMotionVector& vec)             = default;
    PUMotionVector& operator= (const PUMotionVector& vec) = default;

    bool CheckMVPExceedsSizeLimits() const
    {
        return (abs(MV[0].x) > MVMVP_SIZE_LIMIT || abs(MV[0].y) > MVMVP_SIZE_LIMIT
            || abs(MV[1].x) > MVMVP_SIZE_LIMIT || abs(MV[1].y) > MVMVP_SIZE_LIMIT);
    }

    bool operator==(const PUMotionVector& rhs) const
    {
        return (MV[0].x == rhs.MV[0].x &&
            MV[0].y == rhs.MV[0].y &&
            MV[1].x == rhs.MV[1].x &&
            MV[1].y == rhs.MV[1].y &&
            RefIdx.RefL0 == rhs.RefIdx.RefL0 &&
            RefIdx.RefL1 == rhs.RefIdx.RefL1);
    }

    // Important: refIdx is not affected, i.e. inherited from this
    const PUMotionVector operator+ (const PUMotionVector& rhs) const
    {
        return PUMotionVector(RefIdx.RefL0         , RefIdx.RefL1         ,
                              MV[0].x + rhs.MV[0].x, MV[0].y + rhs.MV[0].y,
                              MV[1].x + rhs.MV[1].x, MV[1].y + rhs.MV[1].y);
    }

    inline std::tuple<mfxI32, mfxI32, mfxU32> GetL0MVTuple() const
    {
        return std::make_tuple(MV[0].x, MV[0].y, (mfxU32) RefIdx.RefL0);
    }

    inline std::tuple<mfxI32, mfxI32, mfxU32> GetL1MVTuple() const
    {
        return std::make_tuple(MV[1].x, MV[1].y, (mfxU32)RefIdx.RefL1);
    }

    struct {
        mfxU8   RefL0 : 4;
        mfxU8   RefL1 : 4;
    } RefIdx;

    mfxI16Pair MV[2]; /* index is 0 for L0 and 1 for L1 */
};

struct Counters
{
    //Mapping (width;height) to (total) partitioning blocks
    struct BlockSizeMapFEI : std::map<std::pair<mfxU32, mfxU32>, mfxU32>
    {
        inline void AddBlockTotal(mfxU32 width, mfxU32 height)
        {
            this->operator[](std::make_pair(width, height))++;
        }

        inline mfxU32 GetTotalCount()
        {
            mfxU32 count = 0;
            for (auto& blockSizeRecord : *this)
            {
                count += blockSizeRecord.second;
            }
            return count;
        }

        void PrintTable()
        {
            std::stringstream tmp;
            for (auto& blockSizeRecord : *this)
            {
                tmp.str("");
                tmp << blockSizeRecord.first.first << 'x' << blockSizeRecord.first.second;
                std::cout << std::setw(SPLIT_LOG_TABLE_SYM_WIDTH) << tmp.str();
            }
            std::cout << std::endl;
            for (auto& blockSizeRecord : *this)
            {
                std::cout << std::setw(SPLIT_LOG_TABLE_SYM_WIDTH) << blockSizeRecord.second;
            }
            std::cout << std::endl;
        }
    };

    //Mapping (width;height) to (correct in FEI; total) partitioning blocks
    struct BlockSizeMapASG : std::map<std::pair<mfxU32, mfxU32>, std::pair<mfxU32, mfxU32>>
    {
        inline void AddBlockCorrect(mfxU32 width, mfxU32 height)
        {
            this->operator[](std::make_pair(width, height)).first++;
        }

        inline void AddBlockTotal(mfxU32 width, mfxU32 height)
        {
            this->operator[](std::make_pair(width, height)).second++;
        }

        inline mfxU32 GetCorrectCount()
        {
            mfxU32 count = 0;
            for (auto& blockSizeRecord : *this)
            {
                count += blockSizeRecord.second.first;
            }
            return count;
        }

        inline mfxU32 GetTotalCount()
        {
            mfxU32 count = 0;
            for (auto& blockSizeRecord : *this)
            {
                count += blockSizeRecord.second.second;
            }
            return count;
        }

        void PrintTable()
        {
            std::stringstream tmp;
            for (auto& blockSizeRecord : *this)
            {
                tmp.str("");
                tmp << blockSizeRecord.first.first << 'x' << blockSizeRecord.first.second;
                std::cout << std::setw(SPLIT_LOG_TABLE_SYM_WIDTH) << tmp.str();
            }
            std::cout << std::endl;
            for (auto& blockSizeRecord : *this)
            {
                tmp.str("");
                tmp << blockSizeRecord.second.first  << '/' << blockSizeRecord.second.second;
                std::cout << std::setw(SPLIT_LOG_TABLE_SYM_WIDTH) << tmp.str();
            }
            std::cout << std::endl;
        }
    };

    BlockSizeMapASG m_testPUSizeMapASG;
    BlockSizeMapFEI m_testPUSizeMapFEI;

    BlockSizeMapASG m_testCUSizeMapASG;
    BlockSizeMapFEI m_testCUSizeMapFEI;

    mfxU32 m_testCTUs = 0;

    // MV
    mfxU32 m_correctMVCmpBlocksL0 = 0;
    mfxU32 m_correctMVCmpBlocksL1 = 0;
    mfxU32 m_correctMVCmpBlocksBi = 0;

    mfxU32 m_totalMVCmpBlocksL0 = 0;
    mfxU32 m_totalMVCmpBlocksL1 = 0;
    mfxU32 m_totalMVCmpBlocksBi = 0;

    // Shouldn't affect test result
    mfxU32 m_correctMVCmpBlocksL0PerMVPIndex[MVP_PER_16x16_BLOCK] = { 0, 0, 0, 0 };
    mfxU32 m_correctMVCmpBlocksL1PerMVPIndex[MVP_PER_16x16_BLOCK] = { 0, 0, 0, 0 };
    mfxU32 m_correctMVCmpBlocksBiPerMVPIndex[MVP_PER_16x16_BLOCK] = { 0, 0, 0, 0 };

    mfxU32 m_totalMVCmpBlocksL0PerMVPIndex[MVP_PER_16x16_BLOCK] = { 0, 0, 0 ,0 };
    mfxU32 m_totalMVCmpBlocksL1PerMVPIndex[MVP_PER_16x16_BLOCK] = { 0, 0, 0, 0 };
    mfxU32 m_totalMVCmpBlocksBiPerMVPIndex[MVP_PER_16x16_BLOCK] = { 0, 0, 0, 0 };

    //Won't affect test result
    mfxU32 m_correctMVsL0FromBiMVCmpBlocks = 0;
    mfxU32 m_correctMVsL1FromBiMVCmpBlocks = 0;

    mfxU32 m_correctRecords = 0;
    mfxU32 m_totalPics      = 0;

    //Splits
    mfxU32 m_correctCUs  = 0;
    mfxU32 m_totalCUsASG = 0;
    mfxU32 m_totalCUsFEI = 0;

    mfxU32 m_correctPUs  = 0;
    mfxU32 m_totalPUsASG = 0;
    mfxU32 m_totalPUsFEI = 0;

    //Counters for exact matches
    mfxU32 m_exactCTUs = 0;
    mfxU32 m_exactCUs  = 0;
    mfxU32 m_exactPUs  = 0;
};

// Wrapper on extension buffers
class ExtendedBuffer
{
public:

    explicit ExtendedBuffer(mfxU32 id, mfxU32 pitch, mfxU32 height)
    {
        switch (id)
        {
            // Fill buffer. Track dynamic memory with smart pointer

        case MFX_EXTBUFF_HEVCFEI_ENC_MV_PRED:
        {
            memset(&m_mvPred, 0, sizeof(m_mvPred));

            m_mvPred.Header.BufferId = MFX_EXTBUFF_HEVCFEI_ENC_MV_PRED;
            m_mvPred.Header.BufferSz = sizeof(m_mvPred);

            mfxU64 data_size = pitch * height * sizeof(mfxFeiHevcEncMVPredictors);

            m_data.reset(new mfxU8[data_size]);

            m_mvPred.Data = reinterpret_cast<mfxFeiHevcEncMVPredictors*>(m_data.get());
            memset(m_mvPred.Data, 0, data_size);

            m_mvPred.Pitch = pitch;
            m_mvPred.Height = height;
            break;
        }
        case MFX_EXTBUFF_HEVCFEI_PAK_CTU_REC:
        {
            memset(&m_pakCtuRecord, 0, sizeof(m_pakCtuRecord));

            m_pakCtuRecord.Header.BufferId = MFX_EXTBUFF_HEVCFEI_PAK_CTU_REC;
            m_pakCtuRecord.Header.BufferSz = sizeof(m_pakCtuRecord);

            mfxU64 data_size = pitch * height * sizeof(mfxFeiHevcPakCtuRecordV0);

            m_data.reset(new mfxU8[data_size]);

            m_pakCtuRecord.Data = reinterpret_cast<mfxFeiHevcPakCtuRecordV0*>(m_data.get());
            memset(m_pakCtuRecord.Data, 0, data_size);

            m_pakCtuRecord.Pitch = pitch;
            m_pakCtuRecord.Height = height;

            break;
        }
        case MFX_EXTBUFF_HEVCFEI_PAK_CU_REC:
        {
            memset(&m_pakCuRecord, 0, sizeof(m_pakCuRecord));

            m_pakCuRecord.Header.BufferId = MFX_EXTBUFF_HEVCFEI_PAK_CU_REC;
            m_pakCuRecord.Header.BufferSz = sizeof(m_pakCuRecord);

            mfxU64 data_size = pitch * height * sizeof(mfxFeiHevcPakCuRecordV0);
            m_data.reset(new mfxU8[data_size]);

            m_pakCuRecord.Data = reinterpret_cast<mfxFeiHevcPakCuRecordV0*>(m_data.get());
            memset(m_pakCuRecord.Data, 0, data_size);

            m_pakCuRecord.Pitch = pitch;
            m_pakCuRecord.Height = height;

            break;
        }
        }
    }

    // Hold this entry first in class
    union
    {
        mfxExtFeiHevcEncMVPredictors m_mvPred;
        mfxExtFeiHevcPakCtuRecordV0  m_pakCtuRecord;
        mfxExtFeiHevcPakCuRecordV0   m_pakCuRecord;
    };

    std::unique_ptr<mfxU8[]> m_data;
};

// Wrapper on mfxFrameSurface1
class ExtendedSurface : public mfxFrameSurface1
{
public:
    //Flag to track whether the surface in the pool has been written to disk,
    //but not reused by another frame
    bool isWritten = false;
    mfxU32 encodedOrder = 0xffffffff;

    ExtendedSurface()                                       = default;
    ExtendedSurface(ExtendedSurface &&)                     = default;
    ExtendedSurface(const ExtendedSurface &)                = delete;
    ExtendedSurface& operator= (const ExtendedSurface& vec) = delete;

    // Fill info about current surface. Track dynamic memory by smart pointer
    void AllocData(mfxU32 width, mfxU32 height)
    {
        // Only I420 is supported for now

        Info.FourCC       = MFX_FOURCC_YV12;
        Info.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
        Info.PicStruct    = MFX_PICSTRUCT_PROGRESSIVE;

        Info.Width = MSDK_ALIGN16(width);
        Info.Height = (MFX_PICSTRUCT_PROGRESSIVE == Info.PicStruct) ?
            MSDK_ALIGN16(height) : MSDK_ALIGN32(height);

        Info.CropW = width;
        Info.CropH = height;

        mfxU32 size = Info.CropH * Info.CropW * 3 / 2;
        pSurfData.reset(new mfxU8[size]);
        memset(pSurfData.get(), 0, size); //Zero-init

        Data.Y = pSurfData.get();
        Data.U = Data.Y + Info.CropH * Info.CropW;
        Data.V = Data.U + Info.CropH * Info.CropW / 4;
        Data.Pitch = Info.CropW;
    }

    // Attach buffer to surface
    //
    // Important: after this operation, original buffer is no longer valid. All further ownership is taken by surface
    void AttachBuffer(ExtendedBuffer & buffer)
    {

        ExtBuffers.push_back(std::move(buffer)); // Dynamic memory is moved to new instance of ExtendedBuffer

        // Headers of all buffers should be in same place in memory, because union is used
        vExtBuffersData.push_back(&ExtBuffers.back().m_mvPred.Header);

        ++Data.NumExtParam;
        Data.ExtParam = vExtBuffersData.data();
    }

    // Get attached buffer by id
    mfxExtBuffer* GetBuffer(mfxU32 id)
    {
        auto it = find_if(vExtBuffersData.begin(), vExtBuffersData.end(),
            [&](mfxExtBuffer* buffer) { return buffer->BufferId == id; });

        if (it == vExtBuffersData.end())
            throw std::string("ERROR: ExtendedSurface::GetBuffer: no buffer with id: ") + std::to_string(id);

        return *it;
    }


    void ForceMVPBlockSizeInOutputBuffer(mfxU32 forcedBlockSize)
    {
        for (mfxU32 i = 0; i < Data.NumExtParam; ++i)
        {
            mfxExtBuffer* buffer = Data.ExtParam[i];
            if (buffer == nullptr)
            {
                throw std::string("ERROR: ForceMVPBlockSizeInOutputBuffer: null pointer reference");
            }
            if (buffer->BufferId == MFX_EXTBUFF_HEVCFEI_ENC_MV_PRED)
            {
                mfxExtFeiHevcEncMVPredictors* mvpBuff = reinterpret_cast<mfxExtFeiHevcEncMVPredictors*>(buffer);

                for (mfxU32 i = 0; i < mvpBuff->Height * mvpBuff->Pitch; i++)
                {
                    mvpBuff->Data[i].BlockSize = forcedBlockSize;
                }
            }
        }
    }

private:
    std::unique_ptr<mfxU8[]>   pSurfData; // Surface pixels

    std::list<ExtendedBuffer>  ExtBuffers; // List of ext buffers
    std::vector<mfxExtBuffer*> vExtBuffersData; // Array of pointers
};

// This class performs writing of ext buffers to appropriate file
class BufferWriter
{
public:

    // Start tracking of current buffer; fopen the file with file_name
    void AddBuffer(mfxU32 id, msdk_string fileName)
    {
        FILE* fp = NULL;

        MSDK_FOPEN(fp, fileName.c_str(), MSDK_STRING("wb"));

        if (!fp)
            throw std::string("ERROR: BufferWriter::AddBuffer : fopen_s failed");

        m_bufferFileTable[id] = fp; //Add current file pointer to the map (buf ID->file)
    }

    // Find buffer's file pointer in table by buffer id and write the content.
    void WriteBuffer(mfxExtBuffer* buffer)
    {
        FILE* fp;

        if (m_bufferFileTable.find(buffer->BufferId) != m_bufferFileTable.end())
        {
            fp = m_bufferFileTable[buffer->BufferId];
        }
        else
        {
            throw std::string("ERROR: BufferWriter::WriteBuffer : unknown buffer with id: ") + std::to_string(buffer->BufferId);
        }

        switch (buffer->BufferId)
        {
        case MFX_EXTBUFF_HEVCFEI_ENC_MV_PRED:
        {
            mfxExtFeiHevcEncMVPredictors* mvpBuff = reinterpret_cast<mfxExtFeiHevcEncMVPredictors*>(buffer);

            auto numWritten = fwrite(mvpBuff->Data, sizeof(mfxFeiHevcEncMVPredictors) * mvpBuff->Pitch * mvpBuff->Height, 1, fp);

            if (numWritten != 1)
                throw std::string("ERROR: BufferWriter::WriteBuffer : fwrite failed");
        }
        break;

        default:
            throw std::string("ERROR: BufferWriter::WriteBuffer : unknown buffer with id: ") + std::to_string(buffer->BufferId);
            break;
        }

        return;
    }

    // Zero buffer's data
    void ResetBuffer(mfxExtBuffer* buffer)
    {
        switch (buffer->BufferId)
        {
        case MFX_EXTBUFF_HEVCFEI_ENC_MV_PRED:
        {
            mfxExtFeiHevcEncMVPredictors* mvpBuff = reinterpret_cast<mfxExtFeiHevcEncMVPredictors*>(buffer);
            memset(mvpBuff->Data, 0, sizeof(mfxFeiHevcEncMVPredictors) * mvpBuff->Pitch * mvpBuff->Height);
            break;
        }
        default:
            throw std::string("ERROR: BufferWriter::ResetBuffer : unknown buffer with id: ") + std::to_string(buffer->BufferId);
            break;
        }

        return;
    }

    // Close all file pointers
    ~BufferWriter()
    {
        for (auto & fp : m_bufferFileTable)
        {
            fflush(fp.second);
            fclose(fp.second);
        }
    }

private:
    std::map<mfxU32, FILE*> m_bufferFileTable; // map of id - file pointer
};

// This class performs reading of ext buffers from appropriate file
class BufferReader
{
public:

    // Start tracking of current buffer; fopen the file with file_name
    void AddBuffer(mfxU32 id, msdk_string fileName)
    {
        FILE* fp = NULL;

        MSDK_FOPEN(fp, fileName.c_str(), MSDK_STRING("rb"));

        if (!fp)
            throw std::string("ERROR: BufferReader::AddBuffer : fopen_s failed");

        m_bufferFileTable[id] = fp; //Add current file pointer to the map (buf ID->file)
    }

    // Find buffer's file pointer in table by buffer id and write the content.
    void ReadBuffer(mfxExtBuffer* buffer) //Function for reading can read 1 CU structure per call
    {
        FILE* fp;

        if (m_bufferFileTable.find(buffer->BufferId) != m_bufferFileTable.end())
        {
            fp = m_bufferFileTable[buffer->BufferId];
        }
        else
        {
            throw std::string("ERROR: BufferReader::ReadBuffer : unknown buffer with id: ") + std::to_string(buffer->BufferId);
        }

        switch (buffer->BufferId)
        {
        case MFX_EXTBUFF_HEVCFEI_PAK_CTU_REC:
        {
            mfxExtFeiHevcPakCtuRecordV0* pakCtuBuff = reinterpret_cast<mfxExtFeiHevcPakCtuRecordV0*>(buffer);

            auto numRead = fread(pakCtuBuff->Data, sizeof(mfxFeiHevcPakCtuRecordV0) * pakCtuBuff->Pitch * pakCtuBuff->Height, 1, fp);

            if (numRead != 1)
                throw std::string("ERROR: BufferReader::ReadBuffer : fread failed");
        }
        break;

        case MFX_EXTBUFF_HEVCFEI_PAK_CU_REC:
        {
            mfxExtFeiHevcPakCuRecordV0* pakCuBuff = reinterpret_cast<mfxExtFeiHevcPakCuRecordV0*>(buffer);

            auto numRead = fread(pakCuBuff->Data, sizeof(mfxFeiHevcPakCuRecordV0) * pakCuBuff->Pitch * pakCuBuff->Height, 1, fp);

            if (numRead != 1)
                throw std::string("ERROR: BufferReader::ReadBuffer : fread failed");
        }
        break;

        default:
            throw std::string("ERROR: BufferReader::ReadBuffer : unknown buffer with id: ") + std::to_string(buffer->BufferId);
            break;
        }

        return;
    }

    // Close all file pointers
    ~BufferReader()
    {
        for (auto & fp : m_bufferFileTable)
        {
            fflush(fp.second);
            fclose(fp.second);
        }
    }

private:
    std::map<mfxU32, FILE*> m_bufferFileTable; // map of id - file pointer
};

#endif // MFX_VERSION

#endif // __ASG_HEVC_UTILS_H__
