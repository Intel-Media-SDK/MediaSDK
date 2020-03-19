# Copyright (c) 2019 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

if( PKG_LIBDRM_FOUND )

  set( DEPENDENCIES libdrm )

  add_subdirectory(common)
  add_subdirectory(simple_1_session)
  add_subdirectory(simple_2_decode)
  add_subdirectory(simple_2_decode_vmem)
  add_subdirectory(simple_2_decode_hevc10)
  add_subdirectory(simple_3_encode)
  add_subdirectory(simple_3_encode_vmem)
  add_subdirectory(simple_3_encode_hevc10)
  add_subdirectory(simple_3_encode_vmem_async)
  add_subdirectory(simple_4_vpp_resize_denoise)
  add_subdirectory(simple_4_vpp_resize_denoise_vmem)
  add_subdirectory(simple_5_transcode)
  add_subdirectory(simple_5_transcode_opaque)
  add_subdirectory(simple_5_transcode_opaque_async)
  add_subdirectory(simple_5_transcode_opaque_async_vppresize)
  add_subdirectory(simple_5_transcode_vmem)
  add_subdirectory(simple_6_decode_vpp_postproc)
  add_subdirectory(simple_6_encode_vmem_lowlatency)
  add_subdirectory(simple_6_encode_vmem_vpp_preproc)
  add_subdirectory(simple_6_transcode_opaque_lowlatency)
  add_subdirectory(simple_7_codec)

else()

  message( STATUS "libdrm was not found, the following will not be built: tutorials." )

endif()
