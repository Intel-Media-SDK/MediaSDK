// Copyright (c) 2017-2018 Intel Corporation
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

//  MPEG-2 is a international standard promoted by ISO/IEC and
//  other organizations. Implementations of this standard, or the standard
//  enabled platforms may require licenses from various entities, including
//  Intel Corporation.

#pragma once

#include "umc_defs.h"
#if defined (UMC_ENABLE_MPEG2_VIDEO_DECODER)


//VM headers
#include "vm_debug.h"
//UMC headers
#include "umc_structures.h"
#include "umc_media_data_ex.h"
//MPEG-2
#include "umc_mpeg2_dec_bstream.h"
#include "umc_mpeg2_dec_defs.h"
#include "umc_mpeg2_dec.h"

#include "mfxstructures.h"

namespace UMC
{
    typedef struct _SeqHeaderMask
    {
        uint8_t *memMask;
        uint16_t memSize;

    } SeqHeaderMask;

    typedef struct _SignalInfo
    {
        mfxU16          VideoFormat;
        mfxU16          VideoFullRange;
        mfxU16          ColourDescriptionPresent;
        mfxU16          ColourPrimaries;
        mfxU16          TransferCharacteristics;
        mfxU16          MatrixCoefficients;

    } SignalInfo;


    class MPEG2VideoDecoderBase
    {
    public:
        ///////////////////////////////////////////////////////
        /////////////High Level public interface///////////////
        ///////////////////////////////////////////////////////
        // Default constructor
        MPEG2VideoDecoderBase(void);

        // Default destructor
        virtual ~MPEG2VideoDecoderBase(void);

        // Initialize for subsequent frame decoding.
        virtual Status Init(BaseCodecParams *init);

        // Get next frame
        virtual Status GetFrame(MediaData* in, MediaData* out);

        // Close  decoding & free all allocated resources
        virtual Status Close(void);

        // Reset decoder to initial state
        virtual Status Reset(void);

        // Get video stream information, valid after initialization
        virtual Status GetInfo(BaseCodecParams* info);

        //additional functions for UMC-MFX interaction
        Status GetCCData(uint8_t* ptr, uint32_t *size, unsigned long long *time, uint32_t bufsize);
        Status GetPictureHeader(MediaData* input, int task_num, int prev_task_num);
        int32_t GetCurrDecodingIndex(int task_num);
        int32_t GetNextDecodingIndex(int index);
        int32_t GetPrevDecodingIndex(int index);

        void SetCorruptionFlag(int task_num);
        bool GetCorruptionFlag(int index);

        int32_t GetFrameType(int index);
        int32_t GetDisplayIndex();
        int32_t GetRetBufferLen();
        Status PostProcessUserData(int display_index);
        virtual Status ProcessRestFrame(int task_num);
        bool IsFramePictureStructure(int task_num);
        bool IsFrameSkipped();
        void SetSkipMode(int32_t SkipMode);
        double GetCurrDecodedTime(int index);
        bool isOriginalTimeStamp(int index);
        void LockTask(int index);
        void UnLockTask(int index);
        int FindFreeTask();
        int32_t GetCurrThreadsNum(int task_num);

        Status GetSequenceHeaderMemoryMask(uint8_t *buffer, uint16_t &size);
        Status GetSignalInfoInformation(mfxExtVideoSignalInfo *buffer);

    // for BSD and DEC
    public:
        virtual bool InitTables();

        virtual void DeleteHuffmanTables();

        const sSequenceHeader& GetSequenceHeader() const { return sequenceHeader; }
        const sPictureHeader& GetPictureHeader(int task_num) const { return PictureHeader[task_num]; }

        virtual Status DecodeSlices(int32_t threadID, int task_num);

    protected:

        Status          PrepareBuffer(MediaData* data, int task_num);

        typedef struct {
            double time;
            bool   isOriginal;
        } TimeStampInfo;

        VideoStreamInfo         m_ClipInfo;                         // (VideoStreamInfo) clip info

        TimeStampInfo   m_dTime[2 * DPB_SIZE];
        double          m_dTimeCalc;
        MPEG2FrameType  m_picture_coding_type_save;
        bool   bNeedNewFrame;


        enum SkipLevel
        {
            SKIP_NONE = 0,
            SKIP_B,
            SKIP_PB,
            SKIP_ALL
        };
        int32_t m_SkipLevel;

    protected:
        //The purpose of protected interface to have controlled
        //code in derived UMC MPEG2 decoder classes

        ///////////////////////////////////////////////////////
        /////////////Level 1 protected interface///////////////
        ///////////////////////////////////////////////////////
        //Level 1 can call level 2 functions or re-implement it

        //Sequence Header search. Stops after header start code
        Status FindSequenceHeader(VideoContext *video);

        //Sequence Header decode
        virtual Status DecodeSequenceHeader(VideoContext *video, int task_num);

        // Is current picture to be skipped
        bool IsPictureToSkip(int task_num);


        ///////////////////////////////////////////////////////
        /////////////Level 2 protected interface///////////////
        ///////////////////////////////////////////////////////
        //Level 2 can call level 3 functions or re-implement it

        //Picture Header decode
        Status DecodePictureHeader(int task_num);


        //Slice decode, includes MB decode
        virtual  Status DecodeSlice(VideoContext *video, int task_num) = 0;

        // decode all headers but slice, starts right after startcode
        virtual Status DecodeHeader(int32_t startcode, VideoContext *video, int task_num);

        ///////////////////////////////////////////////////////
        /////////////Level 3 protected interface///////////////
        ///////////////////////////////////////////////////////
        //Level 3 can call level 4 functions or re-implement it

        //Slice Header decode
        virtual Status DecodeSliceHeader(VideoContext *video, int task_num) = 0;

        ///////////////////////////////////////////////////////
        /////////////Level 4 protected interface///////////////
        ///////////////////////////////////////////////////////
        //Level 4 is the lowest level chunk of code
        //can be used as is for basic codec to implement standard
        //or overloaded for special purposes like HWMC, smart decode

        virtual void OnDecodePicHeaderEx(int ) {}

 protected:

        sVideoFrameBuffer::UserDataVector m_user_data;
        std::vector< std::pair<double, size_t> > m_user_ts_data;
        sSequenceHeader           sequenceHeader;
        sSHSavedPars              sHSaved;

        SeqHeaderMask             shMask;
        SignalInfo                m_signalInfo;

        sPictureHeader            PictureHeader[2*DPB_SIZE];

        int                       task_locked[DPB_SIZE*2];
        sVideoFrameBuffer::UserDataVector frame_user_data_v[DPB_SIZE*2];
        sFrameBuffer              frame_buffer;
        sFrameBuffer              frameBuffer_backup_previous;
        sFrameBuffer              frameBuffer_backup;
        int32_t                    b_curr_number_backup;
        int32_t                    first_i_occure_backup;
        int32_t                    frame_count_backup;

        VideoContext**         Video[2*DPB_SIZE];
        uint8_t *                   video_ptr;

        uint32_t                    m_lFlags;

        int32_t                    m_nNumberOfThreads;
        int32_t                    m_nNumberOfThreadsTask[2*DPB_SIZE];
        int32_t                    m_nNumberOfAllocatedThreads;
        bool                      m_IsFrameSkipped;
        bool                      m_IsSHDecoded;
        bool                      m_FirstHeaderAfterSequenceHeaderParsed;
        bool                      m_IsDataFirst;
        bool                      m_IsLastFrameProcessed;
        bool                      m_isFrameRateFromInit;
        bool                      m_isAspectRatioFromInit;
        VideoStreamInfo           m_InitClipInfo;                         // (VideoStreamInfo) Init clip info

        virtual Status          ThreadingSetup(int32_t maxThreads);
        bool                    DeleteTables();
        void                    CalculateFrameTime(double in_time, double * out_time, bool * isOriginal, int task_num, bool buffered);
        bool                    PictureStructureValid(uint32_t picture_structure);

         void                   sequence_display_extension(int task_num);
         void                   sequence_scalable_extension(int task_num);
         void                   picture_temporal_scalable_extension(int task_num);
         void                   picture_spartial_scalable_extension(int task_num);
         void                   picture_display_extension(int task_num);
         void                   copyright_extension(int task_num);
         virtual void           quant_matrix_extension(int task_num) = 0;

         void                   ReadCCData(int task_num);
         virtual Status         UpdateFrameBuffer(int , uint8_t* , uint8_t*) = 0;
    };
}

#ifdef MSVC_LANG
#pragma warning(default: 4324)
#endif

#endif // UMC_ENABLE_MPEG2_VIDEO_DECODER
