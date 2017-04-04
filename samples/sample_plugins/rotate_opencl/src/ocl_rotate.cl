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

__kernel void rotate_Y(__read_only image2d_t YIn, __write_only image2d_t YOut)
{
    int2 coord_src = (int2)(get_global_id(0), get_global_id(1));
    int2 dim = (int2)(get_global_size(0), get_global_size(1));
    int2 coord_dst = dim  - (int2)(1, 1) - coord_src;
    const sampler_t smp = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;

    // rotate Y plane
    float4 pixel = read_imagef(YIn, smp, coord_src);
    write_imagef(YOut, coord_dst, pixel);
}

__kernel void rotate_UV(__read_only image2d_t UVIn, __write_only image2d_t UVOut)
{
    int2 coord_src = (int2)(get_global_id(0), get_global_id(1));
    int2 dim = (int2)(get_global_size(0), get_global_size(1));
    int2 coord_dst = dim  - (int2)(1, 1) - coord_src;
    const sampler_t smp = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;

    // rotate UV plane
    float4 pixel = read_imagef(UVIn, smp, coord_src);
    write_imagef(UVOut, coord_dst, pixel);
}

__kernel void rotate_Y_packed(__read_only image2d_t YIn, __write_only image2d_t YOut)
 {
    int2 coord_src = (int2)(get_global_id(0), get_global_id(1));
    int2 dim = (int2)(get_global_size(0), get_global_size(1));
    int2 coord_dst = dim  - (int2)(1, 1) - coord_src;
    const sampler_t smp = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;

    // rotate Y plane
    uint4 pixel = read_imageui(YIn, smp, coord_src);
    write_imageui(YOut, coord_dst, pixel.wzyx); // rotate samples within pixel
}

__kernel void rotate_UV_packed(__read_only image2d_t UVIn, __write_only image2d_t UVOut)
{
    int2 coord_src = (int2)(get_global_id(0), get_global_id(1));
    int2 dim = (int2)(get_global_size(0), get_global_size(1));
    int2 coord_dst = dim  - (int2)(1, 1) - coord_src;
    const sampler_t smp = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;

    // rotate UV plane
    uint4 pixel = read_imageui(UVIn, smp, coord_src);
    write_imageui(UVOut, coord_dst, pixel.zwxy); // rotate samples within pixel
}
