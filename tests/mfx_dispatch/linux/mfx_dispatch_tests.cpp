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

#include "gtest/gtest.h"
#include <mfxvideo.h>

extern "C"
{
  void *dlopen(const char *filename, int flag)
  {
    printf("dlopen: filename=%s, flag=%d\n", filename, flag);
    return NULL;
  }

  void *dlsym(void *handle, const char *symbol)
  {
    printf("dlsym: handle=%p, symbol=%s\n", handle, symbol);
    return NULL;
  }
} // extern "C"

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  mfxIMPL impl = 0;
  mfxVersion ver{};
  mfxSession session = NULL;

  mfxStatus sts = MFXInit(impl, &ver, &session);
  
  printf(">>> sts=%d\n", sts);

  return RUN_ALL_TESTS();
}
