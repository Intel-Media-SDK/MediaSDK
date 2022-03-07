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

#include "mfx_common.h"

#if defined(MFX_ENABLE_VP8_VIDEO_DECODE_HW)

#include "mfx_session.h"
#include "mfx_common_decode_int.h"
#include "mfx_vp8_dec_decode_hw.h"
#include "mfx_enc_common.h"

#include "umc_va_base.h"

#include "vm_sys_info.h"


#include <va/va.h>
#include <va/va_dec_vp8.h>


#include <iostream>
#include <sstream>
#include <fstream>

#include "mfx_vp8_dec_decode_common.h"


mfxStatus VideoDECODEVP8_HW::PackHeaders(mfxBitstream *p_bistream)
{

     using namespace VP8Defs;

     mfxStatus sts = MFX_ERR_NONE;

     sFrameInfo info = m_frames.back();

   /////////////////////////////////////////////////////////////////////////////////////////
     UMC::UMCVACompBuffer* compBufPic;
     VAPictureParameterBufferVP8 *picParams
         = (VAPictureParameterBufferVP8*)m_p_video_accelerator->GetCompBuffer(VAPictureParameterBufferType, &compBufPic,
                                                                              sizeof(VAPictureParameterBufferVP8));
     //frame width in pixels
     picParams->frame_width = m_frame_info.frameSize.width;
     //frame height in pixels
     picParams->frame_height = m_frame_info.frameSize.height;

     picParams->pic_fields.value = 0;

     if (UMC::I_PICTURE == m_frame_info.frameType)
     {
         //0 means key_frame
         picParams->pic_fields.bits.key_frame = 0;

         picParams->last_ref_frame = VA_INVALID_SURFACE;
         picParams->golden_ref_frame = VA_INVALID_SURFACE;
         picParams->alt_ref_frame = VA_INVALID_SURFACE;
         picParams->out_of_loop_frame = VA_INVALID_SURFACE;

     }
     else // inter frame
     {
         picParams->pic_fields.bits.key_frame = 1;

         picParams->last_ref_frame    =  m_p_video_accelerator->GetSurfaceID(info.lastrefIndex);
         picParams->golden_ref_frame  =  m_p_video_accelerator->GetSurfaceID(info.goldIndex);
         picParams->alt_ref_frame     =  m_p_video_accelerator->GetSurfaceID(info.altrefIndex);

         picParams->out_of_loop_frame =  VA_INVALID_SURFACE;
     }

     //same as version in bitstream syntax
     picParams->pic_fields.bits.version = m_frame_info.version;

     //same as segmentation_enabled in bitstream syntax
     picParams->pic_fields.bits.segmentation_enabled = m_frame_info.segmentationEnabled;

     //same as update_mb_segmentation_map in bitstream syntax
     picParams->pic_fields.bits.update_mb_segmentation_map = m_frame_info.updateSegmentMap;

     //same as update_segment_feature_data in bitstream syntax
     picParams->pic_fields.bits.update_segment_feature_data = m_frame_info.updateSegmentData;

     //same as filter_type in bitstream syntax
     picParams->pic_fields.bits.filter_type = m_frame_info.loopFilterType;

     //same as sharpness_level in bitstream syntax
     picParams->pic_fields.bits.sharpness_level = m_frame_info.sharpnessLevel;

     //same as loop_filter_adj_enable in bitstream syntax
     picParams->pic_fields.bits.loop_filter_adj_enable = m_frame_info.mbLoopFilterAdjust;

     //same as mode_ref_lf_delta_update in bitstream syntax
     picParams->pic_fields.bits.mode_ref_lf_delta_update =  m_frame_info.modeRefLoopFilterDeltaUpdate;

     //same as sign_bias_golden in bitstream syntax
     picParams->pic_fields.bits.sign_bias_golden = 0;

     //same as sign_bias_alternate in bitstream syntax
     picParams->pic_fields.bits.sign_bias_alternate = 0;

     if (UMC::I_PICTURE != m_frame_info.frameType)
     {
         picParams->pic_fields.bits.sign_bias_golden = m_refresh_info.refFrameBiasTable[3];
         picParams->pic_fields.bits.sign_bias_alternate = m_refresh_info.refFrameBiasTable[2];
     }

     //same as mb_no_coeff_skip in bitstream syntax
     picParams->pic_fields.bits.mb_no_coeff_skip = m_frame_info.mbSkipEnabled;
     //flag to indicate that loop filter should be disabled
     picParams->pic_fields.bits.loop_filter_disable = 0;

     if (m_frame_info.loopFilterLevel == 0 || (m_frame_info.version == 2 || m_frame_info.version == 3))
     {
        picParams->pic_fields.bits.loop_filter_disable = 1;
     }

     // probabilities of the segment_id decoding tree and same as
     // mb_segment_tree_probs in the spec.
     picParams->mb_segment_tree_probs[0] = m_frame_info.segmentTreeProbabilities[0];
     picParams->mb_segment_tree_probs[1] = m_frame_info.segmentTreeProbabilities[1];
     picParams->mb_segment_tree_probs[2] = m_frame_info.segmentTreeProbabilities[2];

     if (m_frame_info.segmentationEnabled)
     {

         for (int i = 0; i < 4; i++)
         {
             if (m_frame_info.segmentAbsMode)
                 picParams->loop_filter_level[i] = m_frame_info.segmentFeatureData[VP8_ALT_LOOP_FILTER][i];
             else
             {
                 int32_t filter_level = m_frame_info.loopFilterLevel + m_frame_info.segmentFeatureData[VP8_ALT_LOOP_FILTER][i];
                 picParams->loop_filter_level[i] = mfx::clamp(filter_level, 0, 63);
             }
         }
     }
     else
     {
         picParams->loop_filter_level[0] = m_frame_info.loopFilterLevel;
         picParams->loop_filter_level[1] = m_frame_info.loopFilterLevel;
         picParams->loop_filter_level[2] = m_frame_info.loopFilterLevel;
         picParams->loop_filter_level[3] = m_frame_info.loopFilterLevel;
     }

     //loop filter deltas for reference frame based MB level adjustment
     picParams->loop_filter_deltas_ref_frame[0] = m_frame_info.refLoopFilterDeltas[0];
     picParams->loop_filter_deltas_ref_frame[1] = m_frame_info.refLoopFilterDeltas[1];
     picParams->loop_filter_deltas_ref_frame[2] = m_frame_info.refLoopFilterDeltas[2];
     picParams->loop_filter_deltas_ref_frame[3] = m_frame_info.refLoopFilterDeltas[3];

     //loop filter deltas for coding mode based MB level adjustment
     picParams->loop_filter_deltas_mode[0] = m_frame_info.modeLoopFilterDeltas[0];
     picParams->loop_filter_deltas_mode[1] = m_frame_info.modeLoopFilterDeltas[1];
     picParams->loop_filter_deltas_mode[2] = m_frame_info.modeLoopFilterDeltas[2];
     picParams->loop_filter_deltas_mode[3] = m_frame_info.modeLoopFilterDeltas[3];

     //same as prob_skip_false in bitstream syntax
     picParams->prob_skip_false = m_frame_info.skipFalseProb;

     //same as prob_intra in bitstream syntax
     picParams->prob_intra = m_frame_info.intraProb;

     //same as prob_last in bitstream syntax
     picParams->prob_last = m_frame_info.lastProb;

     //same as prob_gf in bitstream syntax
     picParams->prob_gf = m_frame_info.goldProb;

     //list of 4 probabilities of the luma intra prediction mode decoding
     //tree and same as y_mode_probs in frame header
     //list of 3 probabilities of the chroma intra prediction mode decoding
     //tree and same as uv_mode_probs in frame header

     const mfxU8 *prob_y_table;
     const mfxU8 *prob_uv_table;

     if (UMC::I_PICTURE == m_frame_info.frameType)
     {
         prob_y_table = vp8_kf_mb_mode_y_probs;
         prob_uv_table = vp8_kf_mb_mode_uv_probs;
     }
     else
     {
         prob_y_table = m_frameProbs.mbModeProbY;
         prob_uv_table = m_frameProbs.mbModeProbUV;
     }

     for (uint32_t i = 0; i < VP8_NUM_MB_MODES_Y - 1; i += 1)
     {
         picParams->y_mode_probs[i] = prob_y_table[i];
     }

     for (uint32_t i = 0; i < VP8_NUM_MB_MODES_UV - 1; i += 1)
     {
         picParams->uv_mode_probs[i] = prob_uv_table[i];
     }

     //updated mv decoding probabilities and same as mv_probs in frame header
     for (uint32_t i = 0; i < VP8_NUM_MV_PROBS; i += 1)
     {
         picParams->mv_probs[0][i] = m_frameProbs.mvContexts[0][i];
         picParams->mv_probs[1][i] = m_frameProbs.mvContexts[1][i];
     }

     // Note that va says that count is a number of remained bits in value,
     // but in fact when count is 0 it does need next byte in value. That is what we have.
     picParams->bool_coder_ctx.range = m_boolDecoder[VP8_FIRST_PARTITION].range();
     picParams->bool_coder_ctx.value = (m_boolDecoder[VP8_FIRST_PARTITION].value() >> 24) & 0xff;
     picParams->bool_coder_ctx.count = m_boolDecoder[VP8_FIRST_PARTITION].bitcount() & 0x7;

     compBufPic->SetDataSize(sizeof(VAPictureParameterBufferVP8));

     //////////////////////////////////////////////////////////////////
     UMC::UMCVACompBuffer* compBufCp;
     VAProbabilityDataBufferVP8 *coeffProbs = (VAProbabilityDataBufferVP8*)m_p_video_accelerator->
             GetCompBuffer(VAProbabilityBufferType, &compBufCp, sizeof(VAProbabilityDataBufferVP8));

     std::copy(reinterpret_cast<const char*>(m_frameProbs.coeff_probs),
               reinterpret_cast<const char*>(m_frameProbs.coeff_probs) + sizeof(m_frameProbs.coeff_probs),
               reinterpret_cast<char*>(coeffProbs));

     compBufCp->SetDataSize(sizeof(VAProbabilityDataBufferVP8));

     //////////////////////////////////////////////////////////////////
     UMC::UMCVACompBuffer* compBufQm;
     VAIQMatrixBufferVP8 *qmTable = (VAIQMatrixBufferVP8*)m_p_video_accelerator->
             GetCompBuffer(VAIQMatrixBufferType, &compBufQm, sizeof(VAIQMatrixBufferVP8));

     if (m_frame_info.segmentationEnabled == 0)
     {

         // when segmentation is disabled, use the first entry 0 for the quantization values
         qmTable->quantization_index[0][1] = (unsigned char)m_quantInfo.ydcQ[0];
         qmTable->quantization_index[0][0] = (unsigned char)m_quantInfo.yacQ[0];
         qmTable->quantization_index[0][4] = (unsigned char)m_quantInfo.uvdcQ[0];
         qmTable->quantization_index[0][5] = (unsigned char)m_quantInfo.uvacQ[0];
         qmTable->quantization_index[0][2] = (unsigned char)m_quantInfo.y2dcQ[0];
         qmTable->quantization_index[0][3] = (unsigned char)m_quantInfo.y2acQ[0];
     }
     else
     {

         for (uint32_t i = 0; i < 4; i += 1)
         {
             qmTable->quantization_index[i][1] = (unsigned char)m_quantInfo.ydcQ[i];
             qmTable->quantization_index[i][0] = (unsigned char)m_quantInfo.yacQ[i];
             qmTable->quantization_index[i][4] = (unsigned char)m_quantInfo.uvdcQ[i];
             qmTable->quantization_index[i][5] = (unsigned char)m_quantInfo.uvacQ[i];
             qmTable->quantization_index[i][2] = (unsigned char)m_quantInfo.y2dcQ[i];
             qmTable->quantization_index[i][3] = (unsigned char)m_quantInfo.y2acQ[i];
         }
     }

     compBufQm->SetDataSize(sizeof(VAIQMatrixBufferVP8));

     //////////////////////////////////////////////////////////////////

     uint32_t offset = 0;

     if (UMC::I_PICTURE == m_frame_info.frameType)
         offset = 10;
     else
         offset = 3;

     int32_t size = p_bistream->DataLength;

     UMC::UMCVACompBuffer* compBufSlice;

     VASliceParameterBufferVP8 *sliceParams
         = (VASliceParameterBufferVP8*)m_p_video_accelerator->
             GetCompBuffer(VASliceParameterBufferType, &compBufSlice, sizeof(VASliceParameterBufferVP8));

/*
                          |         first_part_size             |
                          |<----------------------------------->|
    slice_data_offset     |macroblock_offset| |partition_size[0]|    num_of_partitions=N+1
------------------------->|<--------------->| |<--------------->|    // counts both control and token partitions
                          |                 | |<--byte boudary  |
┌-------------------------┼---------------┬-┼-┬-----------------┼--------------------┬-----┬----------------------┐
| Uncompressed data chunk |  Header info  |X|X|   Pre-MB info   |  Token Partition 0 | ... |  Token Partition N-1 |
└-------------------------┴---------------┴-┴-┴-----------------┴--------------------┴-----┴----------------------┘
                                         /  |  \
                                 ┌------------------------------------┐
                                 |used_bits |remaining_bits_in_context|
                                 └<-------->┴<----------------------->┘
                                                bool_coder_ctx.count
*/
     // number of bytes in the slice data buffer for the partitions
     sliceParams->slice_data_size = (int32_t)size - offset;

     //offset to the first byte of partition data
     sliceParams->slice_data_offset = 0;

     //see VA_SLICE_DATA_FLAG_XXX definitions
     sliceParams->slice_data_flag = VA_SLICE_DATA_FLAG_ALL;

     //offset to the first bit of MB from the first byte of partition data
     sliceParams->macroblock_offset = m_frame_info.entropyDecSize;

     // Partitions
     sliceParams->num_of_partitions = m_frame_info.numPartitions + 1;

     sliceParams->partition_size[0] = m_frame_info.firstPartitionSize;

     for (int32_t i = 1; i < m_frame_info.numPartitions + 1; i += 1)
     {
         sliceParams->partition_size[i] = m_frame_info.partitionSize[i - 1];
     }

     compBufSlice->SetDataSize(sizeof(VASliceParameterBufferVP8));

     //////////////////////////////////////////////////////////////////
     UMC::UMCVACompBuffer* compBufBs;
     MFX_CHECK(p_bistream->DataLength > offset, MFX_ERR_INVALID_VIDEO_PARAM);
     uint8_t *bistreamData = (uint8_t *)m_p_video_accelerator->GetCompBuffer(VASliceDataBufferType, &compBufBs, p_bistream->DataLength - offset);

     uint8_t *pBuffer = (uint8_t*) p_bistream->Data;

     std::copy(pBuffer + offset, pBuffer + size, bistreamData);


     compBufBs->SetDataSize((int32_t)size - offset);

     return sts;

} // Status VP8VideoDecoderHardware::PackHeaders(MediaData* src)


#endif
