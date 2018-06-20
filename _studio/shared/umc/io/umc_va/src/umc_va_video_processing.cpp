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

#include "umc_va_video_processing.h"

namespace UMC
{

VideoProcessingVA::VideoProcessingVA()
    : m_pipelineParams()
    , m_surf_region()
    , m_output_surf_region()
    , m_currentOutputSurface()
{
}

VideoProcessingVA::~VideoProcessingVA()
{
}

void VideoProcessingVA::SetOutputSurface(mfxHDL surfHDL)
{
    m_currentOutputSurface = surfHDL;
}

mfxHDL VideoProcessingVA::GetCurrentOutputSurface() const
{
    return m_currentOutputSurface;
}


Status VideoProcessingVA::Init(mfxVideoParam * /* vpParams */, mfxExtDecVideoProcessing * videoProcessing)
{
    VAProcPipelineParameterBuffer *pipelineBuf = &m_pipelineParams;

    pipelineBuf->surface = 0;  // should filled in packer

    m_surf_region.x = videoProcessing->In.CropX;
    m_surf_region.y = videoProcessing->In.CropY;
    m_surf_region.width = videoProcessing->In.CropW;
    m_surf_region.height = videoProcessing->In.CropH;

    pipelineBuf->surface_region = &m_surf_region;

    pipelineBuf->surface_color_standard = VAProcColorStandardBT601;

    m_output_surf_region.x = videoProcessing->Out.CropX;
    m_output_surf_region.y = videoProcessing->Out.CropY;
    m_output_surf_region.width = videoProcessing->Out.CropW;
    m_output_surf_region.height = videoProcessing->Out.CropH;

    pipelineBuf->output_region = &m_output_surf_region;

    /* SFC does not support output_background_color for a while */
    //if (videoProcessing->FillBackground)
    //    pipelineBuf->output_background_color = videoProcessing->V | (videoProcessing->U << 8) | (videoProcessing->Y << 16);
    //else
        pipelineBuf->output_background_color = 0;

    pipelineBuf->output_color_standard = VAProcColorStandardBT601;

    pipelineBuf->pipeline_flags = 0;
    pipelineBuf->filter_flags = 0;
    pipelineBuf->filters = 0;
    pipelineBuf->num_filters = 0;

    pipelineBuf->forward_references = 0;
    pipelineBuf->num_forward_references = 0;

    pipelineBuf->backward_references = 0;
    pipelineBuf->num_backward_references = 0;

    output_surface_array[0] = 0;

    pipelineBuf->rotation_state = VA_ROTATION_NONE;
    pipelineBuf->blend_state = 0;
    pipelineBuf->mirror_state = 0;

    pipelineBuf->additional_outputs = output_surface_array;
    pipelineBuf->num_additional_outputs = 1;

    pipelineBuf->input_surface_flag = 0;

    return UMC_OK;
}


} // namespace UMC
