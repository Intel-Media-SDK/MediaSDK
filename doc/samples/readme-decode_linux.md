
# Decoding Sample

## Overview

**Decoding Sample** works with **Intel® Media SDK** \(hereinafter referred to as "**SDK**"\).

It demonstrates how to use the **SDK** API to create a sample console application that performs decoding of various video compression formats.

The sample is able to work with **HEVC Decoder & Encoder** \(hereinafter referred to as "**HEVC**"\).

**Note:** To run HEVC, please read the instructions in the “HEVC Plugin” section carefully.

## Features

**Decoding Sample** supports the following video formats:

|Format type| |
|---|---|
| input \(compressed\) | H.264 \(AVC, MVC – Multi-View Coding\), MPEG-2 video, VC-1, JPEG\*/Motion JPEG, HEVC \(High Efficiency Video Coding\), VP8, AV1 |
| output \(uncompressed\) | YUV420 |

**Note 2: Decoding Sample** renders the decoded video stream to a file in YUV 4:2:0 sampling format, with the color planes Y, U and V in that order.

## Hardware Requirements

See [`<install-folder>/Media_Samples_Guide_Linux.md`](./Media_Samples_Guide_Linux.md).

## Software Requirements

See [`<install-folder>/Media_Samples_Guide_Linux.md`](./Media_Samples_Guide_Linux.md).

## How to Build the Application

See [`<install-folder>/Media_Samples_Guide_Linux.md`](./Media_Samples_Guide_Linux.md).

## Running the Software

See [`<install-folder>/Media_Samples_Guide_Linux.md`](./Media_Samples_Guide_Linux.md).

The executable file requires the following command-line switches to function properly:

| Option | Description |
|---|---|
| h265\|h264\|mpeg2\|vc1\|vp8\|mvc\|jpeg\|av1 |Input video type. This is an elementary video stream. The use of option h265 is possible only if corresponding plugins are installed. |
| -i <InputFile\> | Input \(compressed\) video file, name and path. |
| -o <Output\> | Specifies output \(YUV\) video file\(s\), name and path. With MVC, specify the file name without extension to use as a pattern. The sample creates several output files with names such as “filename\_N.yuv” according to the number of views in the MVC stream. |

The following command-line switches are optional:

| Option | Description|
|---|---|
 | [-?]| print help|
  | [-hw]| use platform specific SDK implementation (default)|
 | [-sw] | use software implementation, if not specified platform specific SDK implementation is used|
   |[-p plugin] | decoder plugin. Supported values: hevcd_sw, hevcd_hw, vp8d_hw, vp9d_hw, camera_hw, capture_hw|
  | [-path path]| path to plugin (valid only in pair with -p option) (optional for Media SDK in-box plugins, required for user-decoder ones)|
  | [-fps]| limits overall fps of pipeline|
  | [-w]| output width|
  | [-h]| output height|
  | [-di bob/adi] | enable deinterlacing BOB/ADI|
  | [-scaling_mode]| specify scaling mode (lowpower/quality) for VPP|
  | [-d] | enable decode error report|
   |[-jpeg_rgb] |RGB Chroma Type|
   |[-device /path/to/device]| set graphics device for processing. For example: `-device /dev/dri/renderD128`. If not specified, defaults to the first Intel device found on the system. |
  | [-ignore_level_constrain] | ignore level constrain (AVC) |
|Output format parameters:||
 |  [-i420] | pipeline output format: NV12, output file format: I420|
  | [-nv12] | pipeline output format: NV12, output file format: NV12|
 |  [-yuy2] | pipeline output format: YUV2, output file format: YUV2|
 |  [-uyvy]|pipeline output format: UYVY, output file format: UYVY|
  | [-rgb4]| pipeline output format: RGB4, output file format: RGB4|
  | [-rgb4_fcr] | pipeline output format: RGB4 in full color range, output file format: RGB4 in full color range|
  | [-p010] | pipeline output format: P010, output file format: P010|
  | [-a2rgb10] | pipeline output format: A2RGB10, output file format: A2RGB10|
  |||
|   [-vaapi]| work with vaapi surfaces|
  | [-r]| render decoded data in a separate X11 window|
 |  [-rdrm] | render decoded data in a thru DRM frame buffer|
  | [-window x y w h]| set render window position and size|
  | [-low_latency]| configures decoder for low latency mode (supported only for H.264 and JPEG codec)|
   |[-calc_latency]| calculates latency during decoding and prints log (supported only for H.264 and JPEG codec)|
 |  [-async]| depth of asynchronous pipeline. default value is 4. must be between 1 and 20|
 |  [-gpucopy::<on,off>]| Enable or disable GPU copy mode|
   |[-timeout]| timeout in seconds|
   |[-dec_postproc force/auto] | resize after decoder using direct pipe<br>force: instruct to use decoder-based post processing or fail if the decoded stream is unsupported<br>auto: instruct to use decoder-based post processing for supported streams or perform VPP operation through separate pipeline component for unsupported streams|
  | [-threads_num]| number of mediasdk task threads|
|   [-threads_schedtype]| scheduling type of mediasdk task threads|
|   [-threads_priority] |priority of mediasdk task threads<br>Note on the scheduling types and priorities:<br><sched_type>: <priority_min> .. <priority_max><br>The following scheduling types requires root privileges:<br>fifo: 1 .. 99 (static priority: low .. high)<br>rr: 1 .. 99 (static priority: low .. high)<br>The following scheduling types can be used by non-privileged users:<br>other: 0 .. 0 (static priority always 0)<br>batch: 0 .. 0 (static priority always 0)<br>idle: n/a<br>If you want to adjust priority for the other or batch scheduling type, you can do that process-wise using dynamic priority - so called nice value. Range for the nice value is: -20 .. 20 (high .. low) Please, see 'man(1) nice' for details.<br>Example:<br>`./sample_decode h265 -i in.bit -o out.yuv -p 15dd936825ad475ea34e35f3f54217a6`

**Note 1:** You need to have **HEVC** installed to run with h265 codec. In case of HW library \(-sw key is not specified\) it will firstly try to load HW plugin, in case of failure - it will try SW one if available.

Below are examples of a command-line to execute **Decoding Sample**:

`$ sample_decode h264 -i input.h264 -o output.yuv -hw`

Please, also pay attention on “Running the Software” section of `<install-folder>/Media_Samples_Guide.md` document where you will find important notes on backend specific usage \(drm and x11\).

## HEVC Decode Plugins

HEVC codec is implemented as a plugin unlike codecs such as MPEG2 and AVC. We provide 2 implementations of the HEVC decoders: Software \(SW\) and Hardware \(HW\).

**Note 1:** The HEVC SW plugin is available in the HEVC package which is part of the Intel® Media Software Development Kit 2018. You can find the available plugins and their IDs from `$MFX_ROOT/include/mfxplugin.h` file.

**Note 2:** HW plugin for HEVC Decode is supported starting from 6th Generation of Intel CoreTM Processors, Intel® Xeon® E3-1200 and E3-1500 v5 Family with Intel® Processor Graphics 500 Series \(codename Skylake\).

**Note 3:** Decoding sample loads the HW HEVC decode plugin with HW library and SW decode with SW library by default. You can enforce a plugin to be loaded by specifying the plugin ID using "-p" parameter and hexadecimal GUID.

Examples of using HW HEVC decoder and HW library:

```
$ sample_decode h265 -i input.265 -o output.yuv
$ sample_decode h265 -i input.265 -o output.yuv -hw
```

Examples of SW HEVC decoder usage \(the first instance loads SW library, the second one HW library\):

```
$ sample_decode h265 -i input.265 -o output.yuv -sw
$ sample_decode h265 -i input.265 -o output.yuv –p 33a61cb4c27454ca8d85dde757c6f8e
```

## Known Limitations

-   **Decoding Sample** does not fully decode some video streams from a networked folder. Instead, copy the input file to local storage prior to decoding.

-   **Decoding Sample** renders output in the simplest way. The rendering window does not support time stamps, aspect ratio.

-   Decoding Sample cannot render P010 streams in case of SW library usage.
-   `-low_latency` and `–сalc_latency` options should be used with H.264 streams having exactly 1 slice per frame. Preferable streams for an adequate latency estimate are generated by **Conferencing Sample**. The options are also effective for JPEG\* input streams. For all other input formats application would return an error.

-   If overlay is not supported by your hardware or software you won’t be able to render the decoded MVC stream.


-   Application may return error for some MJPEG streams decoded with option –low\_latency.

-   In case of using HEVC plugin \(h265 video type\), only software implementation of that plugin is used by default \(even if you provide -hw option\). To force usage of HEVC HW plugin implementation, please use -p option with proper plugin GUID.

-   VP8 HW decoding is not working if system memory is used.

-   SW HEVC plugin in 10bit mode cannot be used together with HW library VPP. Although library allows that, this is bad practice because additional per-pixel data shift is required. Please use HW HEVC + HW library or SW HEVC + SW library instead.

-   Low latency mode \(-low\_latency\) is not compatible with HEVC decoder.
-   Sample may exit with wrong status code \(MFX\_MORE\_SURFACE\) and messages

    \[ERROR\], sts=MFX\_ERR\_MORE\_SURFACE\(-11\), RunDecoding, Unexpected error!!

    \[ERROR\], sts=MFX\_ERR\_MORE\_SURFACE\(-11\), main, Pipeline.RunDecoding failed

    This may occur when number of outputted frames becomes equal to number of frames specified by "-n <frames\>" command line parameter while current status is MFX\_MORE\_SURFACE. No negative side effects present, requested number of frames are properly decoded.

-   Sample may not function properly on systems that have a non-Intel VGA controller as the first \(primary\) because Intel device is not first in the list.

    To workaround this issue, swap names of DRI device files:

    `$ cd /dev && mv card0 tmp && mv card1 card0 && mv tmp card1`

    and do the same for the files `control64/65` and `renderD128/129`


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
