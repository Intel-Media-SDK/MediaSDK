# Copyright (c) 2018 Intel Corporation
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

add_subdirectory(shared/mfx_trace)
add_subdirectory(shared/umc)
add_subdirectory(shared/asc)
add_subdirectory(mfx_lib/decode)
add_subdirectory(mfx_lib/encode)
add_subdirectory(mfx_lib/encode_hw)
add_subdirectory(mfx_lib/fei)
add_subdirectory(mfx_lib/genx)
add_subdirectory(mfx_lib/vpp)
add_subdirectory(mfx_lib/mctf_package)
add_subdirectory(mfx_lib)
add_subdirectory(enctools)

if(MFX_ENABLE_USER_ENCODE)
  add_subdirectory(hevce_hw)
  add_subdirectory(hevc_fei)
  add_subdirectory(vp9e_hw)
endif()
add_subdirectory(mfx_lib/plugin)
