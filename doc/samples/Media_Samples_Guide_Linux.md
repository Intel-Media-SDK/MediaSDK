# Media Samples Guide

These samples demonstrate how to use the Intel Media SDK API.


## What's New

  * [License](#license)
  * Due to migration of SDK to new libva infrastructure, samples are now bound to libva.so.2 library and can be used with the latest version of SDK only.
  * New options in sample_encode:
    *  Arbitrary SEI insertion
    *  Support for interlaced HEVC encoding
    *  Intra Refresh options are added
    *  Weighted Prediction options are added
  * New options in sample_multi_transcode:
    * Support for transcoding with interlaced HEVC (as source and/or destination)
    * Weighted Prediction options are added
  * Chroma siting option is added to sample_vpp.
  * New measurements using -stat option
  * Target MSDK API version is 1.28


## Package contents



**Video Decoding Sample**

Console application which performs decoding of elementary compressed video stream to raw frames. Includes the following features:

  *   decoding of HEVC \(High Efficiency Video Coding\) video via **HEVC Decoder**
  *   decoding with video post processing \(color conversion\) of raw video sequences
  *   If X11 display is available, can render output with -r 

**Video Encoding Sample**

Console application which performs encoding of raw video frames into elementary compressed stream. Includes the following features:

  * video resizing
  * video rotation via User Plug-in Sample
  * encoding HEVC video


**Video Processing Sample**

Console application which performs various video processing algorithms on raw frames.

**Video Transcoding Sample**

Console application which performs transcoding of elementary video stream from one compressed format to another. Includes the following features:

  * multiple video streams transcoding
  * video resizing, de-interlacing
  * video rotation via User Plug-in Sample
  * video rotation via User Plug-in Sample using OpenCL
  * video processing using VPP algorithms


**Metrics Monitor**

Metrics Monitor is a user space shared library and sample for Linux that provides applications access to a number of metrics from the GPU kernel mode driver to aid in understanding the state of the Intel GPU for Media workloads.

The Metrics Monitor library collects the following i915 kernel mode driver performance counters data:

  * Amount of time each GPU Engine spent executing tasks
  * Average actual GPU frequency

Metrics Monitor allows to monitor the following GPU hardware units:

  * Render engine (execution units)
  * Multi-Format CODEC (MFX)
  * Video Quality Engine
  * Blitter engine

**FEI Encoding Samples**

Console applications using SDK FEI API \(Flexible Encoder Interface\).

NOTE: Intel does not provide technical support for the FEI through forum or Intel Premier Support.  Building an application with FEI may take significantly more effort compared to the standard Media SDK API.  FEI validation is limited. Some combinations of encoding parameters may lead to unstable application behavior, crashes and hangs.  FEI API is not backward compatible

  *  **sample\_fei** showcases use of AVC FEI
  *  **sample\_hevc\_fei** showcases usage of HEVC FEI for regular encodings and transcodings. Includes basic constructions for HEVC FEI enabling.
  *  **sample\_hevc\_fei\_abr** demonstrates how to construct Look Ahead BRC for transcoding scenarios using HEVC FEI. Includes statistics stream out from encoded stream which influence low-level HEVC FEI encoder decision for quality tuning. Also supports 1:N transcoding.




## Software & Hardware Requirements


**Hardware:**

Hardware requirements are the same as for the SDK


**Software:**

Software requirements are the same as for the SDK



## Build Instructions for Linux


By default samples are built as part of building the SDK.

Samples build can be disabled by adding -DBUILD_SAMPLES=OFF to cmake command line.

 
After build binaries are in \<build folder\> \__bin/release



## License


This software is distributed under the BSD-3 clause license, full text of license is reproduced below:

```
Copyright (c) 2005-2019, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

## Legal Information


INFORMATION IN THIS DOCUMENT IS PROVIDED IN CONNECTION WITH INTEL PRODUCTS. NO LICENSE, EXPRESS OR IMPLIED, BY ESTOPPEL OR OTHERWISE, TO ANY INTELLECTUAL PROPERTY RIGHTS IS GRANTED BY THIS DOCUMENT. EXCEPT AS PROVIDED IN INTEL'S TERMS AND CONDITIONS OF SALE FOR SUCH PRODUCTS, INTEL ASSUMES NO LIABILITY WHATSOEVER AND INTEL DISCLAIMS ANY EXPRESS OR IMPLIED WARRANTY, RELATING TO SALE AND/OR USE OF INTEL PRODUCTS INCLUDING LIABILITY OR WARRANTIES RELATING TO FITNESS FOR A PARTICULAR PURPOSE, MERCHANTABILITY, OR INFRINGEMENT OF ANY PATENT, COPYRIGHT OR OTHER INTELLECTUAL PROPERTY RIGHT.

UNLESS OTHERWISE AGREED IN WRITING BY INTEL, THE INTEL PRODUCTS ARE NOT DESIGNED NOR INTENDED FORANYAPPLICATION IN WHICH THE FAILURE OF THE INTEL PRODUCT COULD CREATE A SITUATION WHERE PERSONAL INJURY OR DEATH MAY OCCUR.

Intel may make changes to specifications and product descriptions at any time, without notice. Designers must not rely on the absence or characteristics of any features or instructions marked "reserved" or "undefined." Intel reserves these for future definition and shall have no responsibility whatsoever for conflicts or incompatibilities arising from future changes to them. The information here is subject to change without notice. Do not finalize a design with this information.

The products described in this document may contain design defects or errors known as errata which may cause the product to deviate from published specifications. Current characterized errata are available on request.

Contact your local Intel sales office or your distributor to obtain the latest specifications and before placing your product order.

Copies of documents which have an order number and are referenced in this document, or other Intel literature, may be obtained by calling 1-800-548-4725, or by visiting [Intel's Web Site](http://www.intel.com/).

MPEG is an international standard for video compression/decompression promoted by ISO. Implementations of MPEG CODECs, or MPEG enabled platforms may require licenses from various entities, including Intel Corporation.

Intel, the Intel logo, Intel Core are trademarks or registered trademarks of Intel Corporation or its subsidiaries in the United States and other countries.

**Optimization Notice**

Intel's compilers may or may not optimize to the same degree for non-Intel microprocessors for optimizations that are not unique to Intel microprocessors. These optimizations include SSE2, SSE3, and SSE3 instruction sets and other optimizations. Intel does not guarantee the availability, functionality, or effectiveness of any optimization on microprocessors not manufactured by Intel.

Microprocessor-dependent optimizations in this product are intended for use with Intel microprocessors. Certain optimizations not specific to Intel microarchitecture are reserved for Intel microprocessors. Please refer to the applicable product User and Reference Guides for more information regarding the specific instruction sets covered by this notice.

Notice revision \#20110804



##  Disclaimer

Other names and brands may be claimed as the property of others.

OpenCL and the OpenCL logo are trademarks of Apple Inc. used by permission by Khronos.

Copyright © Intel Corporation
