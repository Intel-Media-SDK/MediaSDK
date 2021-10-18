# Encoding Sample

## Overview

**Encoding Sample** works with **Intel® Media SDK** \(hereinafter referred to as "**SDK**"\).

It demonstrates how to use the **SDK** API to create a simple console application that performs preprocessing and encoding of an uncompressed video stream according to a specific video compression standard. Also the sample shows how to integrate user-defined functions for video processing \(on example of picture rotation plug-in\) into **SDK** encoding pipeline.

The sample is able to work with**HEVC Decoder & Encoder** \(hereinafter referred to as "**HEVC**"\).

**Note:** To run HEVC, please read the instructions in the “HEVC Plugin” section carefully.

## Features

**Encoding Sample** supports the following video formats:

|Format type| |
|---|---|
| input \(uncompressed\) | YUV420, NV12 |
| output \(compressed\) | H.264 \(AVC, MVC – Multi-View Coding\), H.265 \(with **HEVC**\), MPEG-2 video, JPEG\*/Motion JPEG, HEVC \(High Efficiency Video Coding\) |

**Note**: For format YUV420, **Encoding Sample** assumes the order Y, U, V in the input file.

## Hardware Requirements

See [`<install-folder>/Media_Samples_Guide_Linux.md`](./Media_Samples_Guide_Linux.md).

## Software Requirements

See [`<install-folder>/Media_Samples_Guide_Linux.md`](./Media_Samples_Guide_Linux.md).

## How to Build the Application

See [`<install-folder>/Media_Samples_Guide_Linux.md`](./Media_Samples_Guide_Linux.md).

To enable V4L2 option during compilation, set --enable-v4l2=yes option while running `build.pl`

## Running the Software

See [`<install-folder>/Media_Samples_Guide_Linux.md`](./Media_Samples_Guide_Linux.md).



The executable file requires the following command-line switches to function properly:

|Option|Description|
|---|---|
| h264\|h265\|mpeg2\|mvc\|jpeg |Output video type. The use of option h265 is possible only if **HEVC** installed. The option –q is mandatory in case of JPEG encoding.|
| -i <InputFile\> | Input \(uncompressed\) video file, name and path. In case of MVC -i option must be specified for each input YUV file. Only 2 views are supported |
| -o <Output\>| Output \(compressed\) video file, name and path|
| -w <width\> | Width of input video frame |
| -h <height\> | Height of input video frame |

The following command-line switches are optional:

|Option|Description|
|---|---|
|[-nv12\|yuy2\|ayuv\|rgb4\|p010\|y210\|y410\|a2rgb10\|p016\|y216]| input color format (by default YUV420 is expected).|
|[-msb10]| 10-bit color format is expected to have data in Most Significant Bits of words. (LSB data placement is expected by default). This option also disables data shifting during file reading.|
| [-ec::p010\|yuy2\|nv12\|rgb4\|ayuv\|uyvy\|y210\|y410\|p016\|y216] | force output color format for encoder (conversion will be made if necessary). Default value: input color format|
|[-tff\|bff]| input stream is interlaced, top\|bottom field first, if not specified progressive is expected|
  | [-bref] | arrange B frames in B pyramid reference structure|
 |  [-nobref] |  do not use B-pyramid (by default the decision is made by library)|
 |  [-idr_interval size]| idr interval, default 0 means every I is an IDR, 1 means every other I frame is an IDR etc|
|   [-f frameRate] | video frame rate (frames per second)|
 |  [-n number] | number of frames to process|
 |[-device /path/to/device] | set graphics device for processing. For example: `-device /dev/dri/renderD128`. If not specified, defaults to the first Intel device found on the system. |
  | [-b bitRate] | encoded bit rate (Kbits per second), valid for H.264, H.265, MPEG2 and MVC encoders|
  | [-u usage] | target usage, valid for H.265, H.264, H.265, MPEG2 and MVC encoders. <br>Expected values: <br>veryslow (quality), slower, slow, medium (balanced), fast, faster, veryfast (speed)|
  | [-q quality] | mandatory quality parameter for JPEG encoder. In range [1,100]. 100 is the best quality.|
  | [-r distance] | Distance between I- or P- key frames (1 means no B-frames)|
  | [-g size] | GOP size (default 256)|
|   [-x numRefs]   |number of reference frames|
  | [-num_active_P numRefs] | number of maximum allowed references for P frames (for HEVC only)|
  | [-num_active_BL0 numRefs] | number of maximum allowed references for B frames in L0 (for HEVC only)|
 |  [-num_active_BL1 numRefs] | number of maximum allowed references for B frames in L1 (for HEVC only)|
  | [-la] | use the look ahead bitrate control algorithm (LA BRC) (by default constant bitrate control method is used) <br> for H.264, H.265 encoder. Supported only with -hw option on 4th Generation Intel Core processors <br>if [-icq] option is also enabled simultaneously, then LA_ICQ bitrate control algotithm will be used.
 |[-lad depth] | depth parameter for the LA BRC, the number of frames to be analyzed before encoding. In range [0,100].<br>If `depth` is `0` then the encoder forces the value to Max(10, 2\*`GopRefDist`) for LA_ICQ, and to Max(40, 2\*`GopRefDist`) otherwise.<br>If `depth` is in range [1,100] then the encoder forces the value to Max(2\*`GopRefDist`,2\*`NumRefFrame`,`depth`).<br>may be 1 in the case when -mss option is specified <br>if [-icq] option is also enabled simultaneously, then LA_ICQ bitrate control algorithm will be used.|
 |  [-dstw width] | destination picture width, invokes VPP resizing|
  | [-dsth height] | destination picture height, invokes VPP resizing|
  | [-hw] | use platform specific SDK implementation (default)|
   |[-sw] | use software implementation, if not specified platform specific SDK implementation is used|
 |  [-p plugin] | encoder plugin. Supported values: hevce_sw, hevce_gacc, hevce_hw, vp8e_hw, vp9e_hw, hevce_fei_hw (optional for Media SDK in-box plugins, required for user-encoder ones)|
  | [-path path] | path to plugin (valid only in pair with -p option)|
  | [-async] | depth of asynchronous pipeline. default value is 4. must be between 1 and 20.|
  | [-gpucopy::<on,off>] |Enable or disable GPU copy mode|
 |  [-vbr]     | variable bitrate control|
  | [-cbr]     | constant bitrate control|
  | [-vcm]     | Video Conferencing Mode (VCM) bitrate control method|
  | [-qvbr quality] | variable bitrate control algorithm with constant quality. Quality in range [1,51]. 1 is the best quality.|
   |[-icq quality] | Intelligent Constant Quality (ICQ) bitrate control method. In range [1,51]. 1 is the best quality.<br> If [-la] or [-lad] options are enabled simultaneously, then LA_ICQ bitrate control method will be used.|
  | [-avbr]| average variable bitrate control algorithm|
  | [-convergence]| bitrate convergence period for avbr, in the unit of frame|
  | [-accuracy] | bitrate accuracy for avbr, in the range of [1, 100]|
  | [-cqp]| constant quantization parameter (CQP BRC) bitrate control method <br>(by default constant bitrate control method is used), should be used along with -qpi, -qpp, -qpb.|
   |[-qpi] | constant quantizer for I frames (if bitrace control method is CQP). In range [1,51]. 0 by default, i.e. no limitations on QP.|
  | [-qpp]  | constant quantizer for P frames (if bitrace control method is CQP). In range [1,51]. 0 by default, i.e. no limitations on QP.|
 |  [-qpb] | constant quantizer for B frames (if bitrace control method is CQP). In range [1,51]. 0 by default, i.e. no limitations on QP.|
 |  [-round_offset_in file]  | use this file to set per frame inter/intra rounding offset(for AVC only)|
  | [-qsv-ff]  |     Enable QSV-FF mode (deprecated)|
  | [-lowpower:<on,off>]  |     Turn this optiont ON to enable QSV-FF mode|
 |  [-ir_type]   | Intra refresh type. 0 - no refresh, 1 - vertical refresh, 2 - horizontal refresh, 3 - slice refresh|
  | [-ir_cycle_size] | Number of pictures within refresh cycle starting from 2|
  | [-ir_qp_delta]  | QP difference for inserted intra MBs. This is signed value in [-51, 51] range|
   |[-ir_cycle_dist]  | Distance between the beginnings of the intra-refresh cycles in frames|
 |  [-gpb:<on,off>] | Turn this option OFF to make HEVC encoder use regular P-frames instead of GPB|
 |  [-ppyr:<on,off>] | Turn this option ON to enable P-pyramid (by default the decision is made by library)|
  | [-num_slice] | number of slices in each video frame. 0 by default. If num_slice equals zero, the encoder may choose any slice partitioning allowed by the codec standard.|
   |[-mss] |maximum slice size in bytes. Supported only with -hw and h264 codec. This option is not compatible with -num_slice option.|
  | [-mfs]|maximum frame size in bytes. Supported only with h264 and hevc codec for VBR mode.|
  | [-BitrateLimit:<on,off>] | Modifies bitrate to be in the range imposed by the SDK encoder. Setting this flag off may lead to violation of HRD conformance. The default value is OFF, i.e. bitrate is not limited. It works with AVC only.|
  | [-re]      |enable region encode mode. Works only with h265 encoder|
  | [-CodecProfile] | specifies codec profile|
 |  [-CodecLevel]   | specifies codec level|
 |  [-GopOptFlag:closed]     |closed gop|
  | [-GopOptFlag:strict] | strict gop|
  | [-InitialDelayInKB] | the decoder starts decoding after the buffer reaches the initial size InitialDelayInKB,  which is equivalent to reaching an initial delay of InitialDelayInKB*8000/TargetKbps ms|
 |  [-BufferSizeInKB ]  |represents the maximum possible size of any compressed frames|
  | [-MaxKbps ]  | for variable bitrate control, specifies the maximum bitrate at which                             the encoded data enters the Video Buffering Verifier buffer|
  | [-ws] | specifies sliding window size in frames|
  | [-wb]| specifies the maximum bitrate averaged over a sliding window in Kbps|
 |  [-amfs:<on,off>] | adaptive max frame size. If set on, P or B frame size can exceed MaxFrameSize when the scene change is detected. It can benefit the video quality|
  | [-LowDelayBRC] | strictly obey average frame size set by MaxKbps|
  | [-signal:tm ] | represents transfer matrix coefficients for mfxExtVideoSignalInfo. 0 - unknown, 1 - BT709, 2 - BT601|
 |[-WeightedPred:default\|implicit ]   | enables weighted prediction mode|
|   [-WeightedBiPred:default\|implicit ] | enables weighted bi-prediction mode|
   |[-timeout] | encoding in cycle not less than specific time in seconds|
  | [-uncut]  | do not cut output file in looped mode (in case of -timeout option)|
  | [-perf_opt n] | sets number of prefetched frames. In performance mode app preallocates buffer and loads first n frames |
  | [-fps]| limits overall fps of pipeline|
 |  [-dump fileName] |dump MSDK components configuration to the file in text form|
  | [-tcbrctestfile fileName] |file with target frame sizes in bytes. It can be defined for each frame in format: one value per line|
  | [-usei]| insert user data unregistered SEI. eg: 7fc92488825d11e7bb31be2e44b06b34:0:MSDK (uuid:type<0-preifx/1-suffix>:message) <br>the suffix SEI for HEVCe can be inserted when CQP used or HRD disabled|
  | [-extbrc:<on,off,implicit>] | External BRC for AVC and HEVC encoders|
 |  [-ExtBrcAdaptiveLTR:<on,off>] | Set AdaptiveLTR for implicit extbrcExample: `./sample_encode h265 -i InputYUVFile -o OutputEncodedFile -w width -h height -hw -p 2fca99749fdb49aeb121a5b63ef568f7`|
  | [-vaapi] | work with vaapi surfaces <br>Example: `./sample_encode h264|mpeg2|mvc -i InputYUVFile -o OutputEncodedFile -w width -h height -angle 180 -g 300 -r 1`|
  | [-viewoutput] | instruct the MVC encoder to output each view in separate bitstream buffer. Depending on the number of -o options behaves as follows:<br>1: two views are encoded in single file<br>2: two views are encoded in separate files<br>3: behaves like 2 -o opitons was used and then one -o<br>Example:<br>`./sample_encode mvc -i InputYUVFile_1 -i InputYUVFile_2 -o OutputEncodedFile_1 -o OutputEncodedFile_2 -viewoutput -w width -h height` |
  | [-TargetBitDepthLuma] | Target encoding bit-depth for luma samples. May differ from source one. 0 mean default target bit-depth which is equal to source. <br>Example:<br>`./sample_encode h265 -i InputYUVFile_10_bit -o OutputEncodedFile_8_bit -w width -h height -lowpower:on -TargetBitDepthLuma 8 -TargetBitDepthChroma 8` |
  | [-TargetBitDepthChroma] | Target encoding bit-depth for chroma samples. May differ from source one. 0 mean default target bit-depth which is equal to source. <br>Example:<br>`./sample_encode h265 -i InputYUVFile_10_bit -o OutputEncodedFile_8_bit -w width -h height -lowpower:on -TargetBitDepthLuma 8 -TargetBitDepthChroma 8` |

User module options:

|Option|Description|
|---|---|
|[-angle 180] |enables 180 degrees picture rotation before encoding, CPU implementation by default. Rotation requires NV12 input. Options -tff\|bff, -dstw, -dsth, -d3d are not effective together with this one, -nv12 is required.|
 | [-opencl] | rotation implementation through OPENCL<Br>Example:<br> `./sample_encode h264|h265|mpeg2|mvc|jpeg -i InputYUVFile -o OutputEncodedFile -w width -h height -angle 180 -opencl`

If V4L2 support is enabled during compilation, additional options are available:

|Option|Description|
|---|---|
|-d|Device video node \(eg: /dev/video0\)|
|-p|Mipi Port number \(eg: Port 0\)|
|-m|Mipi Mode Configuration \[PREVIEW/CONTINUOUS/STILL/VIDEO\]|
|-uyvy|Input Raw format types V4L2 Encode|
|-YUY2|Input Raw format types V4L2 Encode|
|-i::v4l2|Enable v4l2 mode|

Below are examples of a command-line to execute **Encoding Sample**:
```
$ sample_encode h264 -i input.yuv -o output.h264 –w 720 –h 480 –b 10000 –f 30 –u quality –d3d –hw
$ sample_encode mpeg2 -i input.yuv -o output.mpeg2 –w 1920 –h 1080 –b 15000 -u speed –nv12 –tff -hw
$ sample_encode h264 -i input.yuv -o output.h264 –w 1920 –h 1080 –dstw 360 –dsth 240 –b 1000 –u balanced -hw
```

**Note 1:** You need to have **HEVC** installed to run with h265 codec. In case of HW library it will firstly try to load HW HEVC plugin in case of failure - it will try SW one if available.

**Tip:**

To achieve better performance, use input streams in NV12 color format. If the input stream is in YUV420 format, each frame is converted to NV12 which reduces overall performance.

## HEVC Encode Plugins

HEVC codec is implemented as a plugin unlike codecs such as MPEG2 and AVC. There are 3 implementations of HEVC encoder: Hardware \(HW\), Software \(SW\) and GPU-Accelerated \(GACC\) plugins.

**Note 1:** The HEVC SW and GACC plugins are available only in the HEVC package which is part of the Intel® Media Software Development Kit 2018. You can find the available plugins and their IDs from `$MFX_ROOT/include/mfxplugin.h` file.

**Note 2:** HW plugin for HEVC encode is supported starting from 6th Generation of Intel CoreTM Processors, Intel® Xeon® E3-1200 and E3-1500 v5 Family with Intel® Processor Graphics 500 Series \(codename Skylake\).

**Note 3:** Encoding sample loads the HW HEVC encode plugin with HW library and SW encode with SW library by default. You can enforce a plugin to be loaded by specifying the plugin ID using "-p" parameter and hexadecimal GUID.

Examples of running HW HEVC encoder with HW library:

```
$ sample_encode h265 -i input.yuv -o output.h265 –w 720 –h 480 –b 10000 –f 30 –u quality
```

```
$ sample_encode h265 -i input.yuv -o output.h265 –w 720 –h 480 –b 10000 –f 30 –u quality -hw
```

Different ways to run SW HEVC encoder \(the first example uses SW library, the second one uses HW library\):

```
$ sample_encode h265 -i input.yuv -o output.h265 –w 720 –h 480 –b 10000 –f 30 –u quality -sw
$ sample_encode h265 -i input.yuv -o output.h265 –w 720 –h 480 –b 10000 –f 30 –u quality -p 2fca99749fdb49aeb121a5b63ef568f7
```

To run the HEVC GACC encoder, you have to specify the “-p” parameter with GUID. Please note that GACC plugin works only with HW library.

The following command-lines will force sample to use the HEVC GACC Encode plugin:

```
$ sample_encode h265 -i input.yuv -o output.h265 –w 720 –h 480 –b 10000 –p e5400a06c74d41f5b12d430bbaa23d0b
$ sample_encode h265 -i input.yuv -o output.h265 –w 720 –h 480 –b 10000 –p e5400a06c74d41f5b12d430bbaa23d0b -hw
```

To run HEVC plugins in 10 bit mode, you have to specify the “-ec::p010” or "-p010" option:

```
$ sample_encode h265 -i input.yuv -o output.h265 -w 720 -h 480 -b 10000 -ec::p010
```

```
$ sample_encode h265 -i input.p010 -o output.h265 -w 720 -h 480 -b 10000 -p010
```

## Known Limitations

-   Not all combinations of optional switches are supported. If the option `–angle 180` or `–opencl` is specified, options `-tff|bff, -dstw, -dsth, -d3d` and MVC output are not available.

-   **Encoding Sample** if run with `–opencl` option requires input video frame width to be aligned by 4.

-   In case of using HEVC plugin \(h265 video type\), plugin type \(hardware or software\) used by default is set depending on -sw or -hw sample options. However, hardware HEVC plugins work on specific platforms only. To force usage of specific HEVC plugin implementation, please use -p option with proper plugin GUID.
-   Sample may not function properly on systems that have a non-Intel VGA controller as the first \(primary\) because Intel device is not first in the list.

    To workaround this issue, swap names of DRI device files:

    `$ cd /dev && mv card0 tmp && mv card1 card0 && mv tmp card1`

    and do the same for the files `control64/65` and `renderD128/129`

-   HECV 10Bit encoding works with HEVC SW plugin only due to library limitations

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

##

\* Other names and brands may be claimed as the property of others.

OpenCL and the OpenCL logo are trademarks of Apple Inc. used by permission by Khronos.

Copyright © Intel Corporation
