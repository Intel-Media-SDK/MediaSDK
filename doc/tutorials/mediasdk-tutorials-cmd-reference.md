![](./pic/intel_logo.png)

# **Tutorials Command Line Reference**

<div style="page-break-before:always" />

**LEGAL DISCLAIMER**

INFORMATION IN THIS DOCUMENT IS PROVIDED IN CONNECTION WITH INTEL PRODUCTS. NO LICENSE, EXPRESS OR IMPLIED, BY ESTOPPEL OR OTHERWISE, TO ANY INTELLECTUAL PROPERTY RIGHTS IS GRANTED BY THIS DOCUMENT.  EXCEPT AS PROVIDED IN INTEL'S TERMS AND CONDITIONS OF SALE FOR SUCH PRODUCTS, INTEL ASSUMES NO LIABILITY WHATSOEVER AND INTEL DISCLAIMS ANY EXPRESS OR IMPLIED WARRANTY, RELATING TO SALE AND/OR USE OF INTEL PRODUCTS INCLUDING LIABILITY OR WARRANTIES RELATING TO FITNESS FOR A PARTICULAR PURPOSE, MERCHANTABILITY, OR INFRINGEMENT OF ANY PATENT, COPYRIGHT OR OTHER INTELLECTUAL PROPERTY RIGHT.

UNLESS OTHERWISE AGREED IN WRITING BY INTEL, THE INTEL PRODUCTS ARE NOT DESIGNED NOR INTENDED FOR ANY APPLICATION IN WHICH THE FAILURE OF THE INTEL PRODUCT COULD CREATE A SITUATION WHERE PERSONAL INJURY OR DEATH MAY OCCUR.

Intel may make changes to specifications and product descriptions at any time, without notice. Designers must not rely on the absence or characteristics of any features or instructions marked "reserved" or "undefined." Intel reserves these for future definition and shall have no responsibility whatsoever for conflicts or incompatibilities arising from future changes to them. The information here is subject to change without notice. Do not finalize a design with this information.

The products described in this document may contain design defects or errors known as errata which may cause the product to deviate from published specifications. Current characterized errata are available on request.

Contact your local Intel sales office or your distributor to obtain the latest specifications and before placing your product order.

Copies of documents which have an order number and are referenced in this document, or other Intel literature, may be obtained by calling 1-800-548-4725, or by visiting [Intel's Web Site](http://www.intel.com/).

MPEG is an international standard for video compression/decompression promoted by ISO. Implementations of MPEG CODECs, or MPEG enabled platforms may require licenses from various entities, including Intel Corporation.

Intel and the Intel logo are trademarks or registered trademarks of Intel Corporation or its subsidiaries in the United States and other countries.

\*Other names and brands may be claimed as the property of others.

Copyright © 2007-2020, Intel Corporation. All Rights reserved.
<div style="page-break-before:always" />

**Optimization Notice**

Intel's compilers may or may not optimize to the same degree for non-Intel microprocessors for optimizations that are not unique to Intel microprocessors. These optimizations include SSE2, SSE3, and SSSE3 instruction sets and other optimizations. Intel does not guarantee the availability, functionality, or effectiveness of any optimization on microprocessors not manufactured by Intel.

Microprocessor-dependent optimizations in this product are intended for use with Intel microprocessors. Certain optimizations not specific to Intel microarchitecture are reserved for Intel microprocessors. Please refer to the applicable product User and Reference Guides for more information regarding the specific instruction sets covered by this notice.

Notice revision #20110804

<div style="page-break-before:always" />

# Table of contents
- [General considerations](#general-considerations)
- [Basic tutorials](#basic-tutorials)
  - [simple_1_session](#simple-1-session)
  - [simple_7_codec](#simple_7_codec)
- [Decoding tutorials](#decoding-tutorials)
  - [simple_2_decode](#simple_2_decode)
  - [simple_2_decode_hevc10](#simple_2_decode_hevc10)
  - [simple_2_decode_vmem](#simple_2_decode_vmem)
  - [simple_6_decode_vpp_postproc](#simple_6_decode_vpp_postproc)
- [Encoding tutorials](#encoding-tutorials)
  - [simple_3_encode](#simple_3_encode)
  - [simple_3_encode_hevc10](#simple_3_encode_hevc10)
  - [simple_3_encode_vmem](#simple_3_encode_vmem)
  - [simple_3_encode_vmem_async](#simple_3_encode_vmem_async)
  - [simple_6_encode_vmem_lowlatency](simple_6_encode_vmem_lowlatency)
  - [simple_6_encode_vmem_vpp_preproc](#simple_6_encode_vmem_vpp_preproc)
- [Video PreProcessing (VPP) tutorials](#video-preprocessing-vpp-tutorials)
  - [simple_4_vpp_resize_denoise](#simple_4_vpp_resize_denoise)
  - [simple_4_vpp_resize_denoise_vmem](#simple_4_vpp_resize_denoise_vmem)
- [Transcoding tutorials](#transcoding-tutorials)
  - [simple_5_transcode](#simple_5_transcode)
  - [simple_5_transcode_vmem](#simple_5_transcode_vmem)
  - [simple_5_transcode_opaque](#simple_5_transcode_opaque)
  - [simple_5_transcode_opaque_async](#simple_5_transcode_opaque_async)
  - [simple_6_transcode_opaque_lowlatency](#simple_6_transcode_opaque_lowlatency)
  - [simple_5_transcode_opaque_async_vppresize](#simple_5_transcode_opaque_async_vppresize)


# General considerations
Intel Media SDK Tutorials package was designed in the simplistic manner. Each tutorial suites the purpose to demonstrate some technique of working with the Media SDK library. For the simplicity sample code supports only minimum required number of options without which it is impossible to variate input data and produce correct result. This command line reference is divided into few parts. Each part describes group of tutorials which have the same or similar list of command line options and arguments. The parts are:

-   **Basic tutorials**.
-   **Decoding tutorials**. The only configurable component in these tutorials is decoder (H.264).
Configuration parameters are input and output streams.
-   **Encoding tutorials**. The only configurable component is encoder (H.264). Configuration
parameters (besides input and output stream) - width, height, bitrate, framerate.
-   **VPP tutorials**. The only configurable component is VPP. Configuration parameters (besides
input and output stream) - input stream width and height.
-   **Transcoding tutorials**. Configurable components are decoder (H.264) and encoder (H.264).
Configuration parameters are: input/output streams, bitrate/framerate to encode.

# Basic tutorials
## simple_1_session
**simple_1_session *[-sw | -hw | -auto]***
Tutorial initializes Intel Media SDK session of a specified type. If *-auto* is specified (the default) Media SDK dispatcher will automatically select library implementation to use. If *-hw* is specified HW implementation will be used, *-sw* - SW one. Depending on Media SDK distribution different implementations may present. Options to adjust session type are applicable for all other tutorials described in this reference.

| Option | Description |
| ------:| -----------:|
| -sw | Loads SW Media SDK Library implementation |
| -hw | Loads HW Media SDK Library implementation |
| -auto | Automatically choses Media SDK library implementation |

## simple_7_codec
**simple_7_codec *[-sw | -hw | -auto]***
Tutorial showcases MSDK features via call Query functions. The tutorial can be run with the *-hw*, *-sw*, *-auto* or no options, it will report the capability for each codec. Since this tutorial checks all supported coded, it will configure a set of the video parameters for each codec by the predefined filling function. Each filling function will try the maximum resolutions, user might change the resolution based on his platform.

| Option | Description |
| ------:| -----------:|
| -sw | Loads SW Media SDK Library implementation |
| -hw | Loads HW Media SDK Library implementation |
| -auto | Automatically choses Media SDK library implementation |


# Decoding tutorials
## simple_2_decode
**simple_decode *[-sw | -hw | -auto] input-file [ output-file ]***
Tutorial demonstrates how to decode given raw video stream (input-file) of H.264 format. If output-file was specified, decoded YUV will be written into it. If the output-file is omitted, tutorial can be used to estimate constructed pipeline’s performance. Decoder is configured to produce decoded data in the system memory.

| Option | Description |
| ------:| -----------:|
| -sw | Loads SW Media SDK Library implementation |
| -hw | Loads HW Media SDK Library implementation |
| -auto | Automatically choses Media SDK library implementation |

| Argument | Description |
| ------:| -----------:|
| input-file | Mandatory argument. Sets incoming input bitstream to process. Input data should be in raw H.264 format. |
| output-file | Optional argument. Sets uncompressed video file to write output data. If file will not be specified, tutorial will work in the performance mode: it will process data, but will not produce any output. |
## simple_2_decode_hevc10
**simple_2_decode_hevc10 *[-sw | -hw | -auto] input-file [ output-file ]***
Tutorial demonstrates how to decode given raw video stream (input-file) of H.265 format. If output-file is specified, decoded YUV will be written into it. If the output-file is omitted, tutorial can be used to estimate constructed pipeline’s performance. Decoder is configured to produce decoded data in the system memory.

| Option | Description |
| ------:| -----------:|
| -sw | Loads SW Media SDK Library implementation |
| -hw | Loads HW Media SDK Library implementation |
| -auto | Automatically choses Media SDK library implementation |

| Argument | Description |
| ------:| -----------:|
| input-file | Mandatory argument. Sets incoming input bitstream to process. Input data should be in raw H.265 format. |
| output-file | Optional argument. Sets uncompressed video file to write output data. If file will not be specified, tutorial will work in the performance mode: it will process data, but will not produce any output. |

## simple_2_decode_vmem
**simple_decode_vmem *[-sw | -hw | -auto] input-file [ output-file ]***
Tutorial demonstrates how to decode given raw video stream (input-file) of H.264 format. If output-file was  specified, decoded YUV will be written into it. If the output-file is omitted, tutorial can be used to estimate  constructed pipeline’s performance. Decoder is configured to produce decoded data in the video memory.

| Option | Description |
| ------:| -----------:|
| -sw | Loads SW Media SDK Library implementation |
| -hw | Loads HW Media SDK Library implementation |
| -auto | Automatically choses Media SDK library implementation |

| Argument | Description |
| ------:| -----------:|
| input-file | Mandatory argument. Sets incoming input bitstream to process. Input data should be in raw H.264 format. |
| output-file | Optional argument. Sets uncompressed video file to write output data. If file will not be specified, tutorial will work in the performance mode: it will process data, but will not produce any output. |

## simple_6_decode_vpp_postproc
**simple_decode_vpp_pp *[-sw | -hw | -auto] input-file [ output-file ]***
Tutorial demonstrates how to decode given raw video stream (input-file) of H.264 format and perform some operation with the decoded frames thru the Video Post-Processing (VPP) component. If output-file was  specified, decoded YUV will be written into it. If the output-file is omitted, tutorial can be used to estimate  constructed pipeline’s performance. The performed VPP operation is x2 downscaling.

| Option | Description |
| ------:| -----------:|
| -sw | Loads SW Media SDK Library implementation |
| -hw | Loads HW Media SDK Library implementation |
| -auto | Automatically choses Media SDK library implementation |

| Argument | Description |
| ------:| -----------:|
| input-file | Mandatory argument. Sets incoming input bitstream to process. Input data should be in raw H.264 format. |
| output-file | Optional argument. Sets uncompressed video file to write output data. If file will not be specified, tutorial will work in the performance mode: it will process data, but will not produce any output. |

# Encoding tutorials
## simple_3_encode
**simple_encode *[-sw | -hw | -auto] -g WxH -b bitrate -f framerate [ input-file ] [ output-file ]***
Tutorial demonstrates how to encode given YUV video stream (input-file) in H.264 format. If output-file was specified, encoded bitstream will be written into it. If the input-file and output-file are omitted, tutorial can be used to estimate constructed pipeline’s performance (to some degree). Encoder is configured to receive input  data from system memory.

| Option | Description |
| ------:| -----------:|
| -sw | Loads SW Media SDK Library implementation |
| -hw | Loads HW Media SDK Library implementation |
| -auto | Automatically choses Media SDK library implementation |
| -g WxH | Mandatory option. Sets input video geometry, i.e. width and height. Example: -g 1920x1080. |
| -b bitrate | Mandatory option. Sets bitrate with which data should be encoded, in KBits-per-second. Example: -b 5000 to encode data at 5Mbit. |
| -f framerate | Mandatory option. Sets framerate with which data should be encoded in the form -f nominator/denominator. Example: -f 30/1. |

| Argument | Description |
| ------:| -----------:|
| input-file | Optional argument. Sets incoming input file containing uncompressed video. If file will not be specified, tutorial will work in the performance mode: input will be simulated by producing empty input frames filled with some color |
| output-file | Optional argument. Sets raw H.264 video file to write output data. If file will not be specified, tutorial will work in the performance mode: it will process data, but will not produce any output. |

## simple_3_encode_hevc10
**simple_3_encode_hevc10 *[-sw | -hw | -auto] -g WxH -b bitrate -f framerate [ input-file ] [ output-file ]***
Tutorial demonstrates how to encode given p010 YUV video stream (input-file) in H.265 format. If output-file is specified, encoded bitstream will be written into it. If the input-file and output-file are omitted, tutorial can be used to estimate constructed pipeline’s performance (to some degree). Encoder is configured to receive input  data from system memory.

| Option | Description |
| ------:| -----------:|
| -sw | Loads SW Media SDK Library implementation |
| -hw | Loads HW Media SDK Library implementation |
| -auto | Automatically choses Media SDK library implementation |
| -g WxH | Mandatory option. Sets input video geometry, i.e. width and height. Example: -g 1920x1080. |
| -b bitrate | Mandatory option. Sets bitrate with which data should be encoded, in KBits-per-second. Example: -b 5000 to encode data at 5Mbit. |
| -f framerate | Mandatory option. Sets framerate with which data should be encoded in the form -f nominator/denominator. Example: -f 30/1. |

| Argument | Description |
| ------:| -----------:|
| input-file | Optional argument. Sets incoming input file containing uncompressed video. If file is not be specified, tutorial will work in the performance mode: input will be simulated by producing empty input frames filled with some color |
| output-file | Optional argument. Sets raw H.265 video file to write output data. If file is not be specified, tutorial will work in the performance mode: it will process data, but will not produce any output. |

## simple_3_encode_vmem
**simple_encode_vmem *[-sw | -hw | -auto] -g WxHx10 -b bitrate -f framerate [ input-file ] [ output-file ]***
Tutorial demonstrates how to encode given YUV video stream (input-file) in H.264 format. If output-file was specified, encoded bitstream will be written into it. If the input-file and output-file are omitted, tutorial can be used to estimate constructed pipeline’s performance (to some degree). Encoder is configured to receive input data from video memory.

| Option | Description |
| ------:| -----------:|
| -sw | Loads SW Media SDK Library implementation |
| -hw | Loads HW Media SDK Library implementation |
| -auto | Automatically choses Media SDK library implementation |
| -g WxHx10 | Mandatory option. Sets input video geometry, i.e. width, height, bitdepth. Example: -g 1920x1080x10. |
| -b bitrate | Mandatory option. Sets bitrate with which data should be encoded, in KBits-per-second. Example: -b 5000 to encode data at 5Mbit. |
| -f framerate | Mandatory option. Sets framerate with which data should be encoded in the form -f nominator/denominator. Example: -f 30/1. |

| Argument | Description |
| ------:| -----------:|
| input-file | Optional argument. Sets incoming input file containing uncompressed video. If file will not be specified, tutorial will work in the performance mode: input will be simulated by producing empty input frames filled with some color. |
| output-file | Optional argument. Sets raw H.264 video file to write output data. If file will not be specified, tutorial will work in the performance mode: it will process data, but will not produce any output. |

## simple_3_encode_vmem_async
**simple_encode_vmem_async *[-sw | -hw | -auto] -g WxH -b bitrate -f framerate [ input-file ] [ output-file ]***

Tutorial demonstrates how to encode given YUV video stream (input-file) in H.264 format. If output-file was specified, encoded bitstream will be written into it. If the input-file and output-file are omitted, tutorial can be used to estimate constructed pipeline’s performance (to some degree). Encoder is configured to receive input data from system memory. Constructed pipeline is asynchronous and should be faster than pipelines in simple 3 encode and simple 3 encode vmem samples.

| Option | Description |
| ------:| -----------:|
| -sw | Loads SW Media SDK Library implementation |
| -hw | Loads HW Media SDK Library implementation |
| -auto | Automatically choses Media SDK library implementation |
| -g WxH | Mandatory option. Sets input video geometry, i.e. width and height. Example: -g 1920x1080. |
| -b bitrate | Mandatory option. Sets bitrate with which data should be encoded, in KBits-per-second. Example: -b 5000 to encode data at 5Mbit. |
| -f framerate | Mandatory option. Sets framerate with which data should be encoded in the form -f nominator/denominator. Example: -f 30/1. |

| Argument | Description |
| ------:| -----------:|
| input-file | Optional argument. Sets incoming input file containing uncompressed video. If file will not be specified, tutorial will work in the performance mode: input will be simulated by producing empty input frames filled with some color. |
| output-file | Optional argument. Sets raw H.264 video file to write output data. If file will not be specified, tutorial will work in the performance mode: it will process data, but will not produce any output. |

## simple_6_encode_vmem_lowlatency
**simple_encode_vmem_lowlat *[-sw | -hw | -auto] -g WxH -b bitrate -f framerate [–measure-latency | –no-measure-latency] [ input-file ] [ output-file ]***

Tutorial demonstrates how to encode given YUV video stream (input-file) in H.264 format. If output-file was specified, encoded bitstream will be written into it. If the input-file and output-file are omitted, tutorial can be used to estimate constructed pipeline’s performance (to some degree). Encoder is configured to work in low latency mode, producing encoded data as fast as possible.

| Option | Description |
| ------:| -----------:|
| -sw | Loads SW Media SDK Library implementation |
| -hw | Loads HW Media SDK Library implementation |
| -auto | Automatically choses Media SDK library implementation |
| -g WxH | Mandatory option. Sets input video geometry, i.e. width and height. Example: -g 1920x1080. |
| -b bitrate | Mandatory option. Sets bitrate with which data should be encoded, in KBits-per-second. Example: -b 5000 to encode data at 5Mbit. |
| -f framerate | Mandatory option. Sets framerate with which data should be encoded in the form -f nominator/denominator. Example: -f 30/1. |
| --measure-latency | Optional argument. With the option set tutorial calculates and prints latency statistics. This is the default. |
| --no-measure-latency | Optional argument. With the option set tutorial will not calculate and print latency statistics. |

| Argument | Description |
| ------:| -----------:|
| input-file | Optional argument. Sets incoming input file containing uncompressed video. If file will not be specified, tutorial will work in the performance mode: input will be simulated by producing empty input frames filled with some color. |
| output-file | Optional argument. Sets raw H.264 video file to write output data. If file will not be specified, tutorial will work in the perfromance mode: it will process data, but will not produce any output. |

## simple_6_encode_vmem vpp preproc
**simple_encode_vmem_preproc *[-sw | -hw | -auto] -g WxH -b bitrate -f framerate [ input-file ] [ output-file ]***

Tutorial demonstrates how to encode given RGB video stream (input-file) in H.264 format after applying some Video Pre-Processing (VPP) operation. If output-file was specified, encoded bitstream will be written into it. If  the input-file and output-file are omitted, tutorial can be used to estimate constructed pipeline’s performance (to some degree). The performed VPP operation is RGB to NV12 color space conversion.

| Option | Description |
| ------:| -----------:|
| -sw | Loads SW Media SDK Library implementation |
| -hw | Loads HW Media SDK Library implementation |
| -auto | Automatically choses Media SDK library implementation |
| -g WxH | Mandatory option. Sets input video geometry, i.e. width and height. Example: -g 1920x1080. |
| -b bitrate | Mandatory option. Sets bitrate with which data should be encoded, in KBits-per-second. Example: -b 5000 to encode data at 5Mbit. |
| -f framerate | Mandatory option. Sets framerate with which data should be encoded in the form -f nominator/denominator. Example: -f 30/1. |

| Argument | Description |
| ------:| -----------:|
| input-file | Optional argument. Sets incoming input file containing uncompressed video. If file will not be specified, tutorial will work in the performance mode: input will be simulated by producing empty input frames filled with some color. |
| output-file | Optional argument. Sets raw H.264 video file to write output data. If file will not be specified, tutorial will work in the performance mode: it will process data, but will not produce any output. |

# Video PreProcessing (VPP) tutorials
## simple_4_vpp_resize_denoise
**simple_vpp *[-sw | -hw | -auto] -g WxH [ input-file ] [ output-file ]***
Tutorial demonstrates how to run some Video Pre/Post-Processing (VPP) operation in the given input YUV stream (input-file). If output-file was specified, produced stream will be written into it. If the input-file and output-file are omitted, tutorial can be used to estimate constructed pipeline’s performance. Constructed pipeline is configured to work on system memory on the input and output. The performed VPP operation is x2 downscaling and noise reduction.

| Option | Description |
| ------:| -----------:|
| -sw | Loads SW Media SDK Library implementation |
| -hw | Loads HW Media SDK Library implementation |
| -auto | Automatically chooses Media SDK library implementation |
| -g WxH | Mandatory option. Sets input video geometry, i.e. width and height. Example: -g 1920x1080. |

| Argument | Description |
| ------:| -----------:|
| input-file | Optional argument. Sets incoming input file containing uncompressed video. If file will not be specified, tutorial will work in the performance mode: input will be simulated by producing empty input frames filled with some color. |
| output-file | Optional argument. Sets uncompressed video file to write output data. If file will not be specified, tutorial will work in the performance mode: it will process data, but will not produce any output. |

## simple_4_vpp_resize_denoise_vmem
**simple_vpp_vmem *[-sw | -hw | -auto] -g WxH [ input-file ] [ output-file ]***
Tutorial demonstrates how to run some Video Pre/Post-Processing (VPP) operation in the given input YUV stream (input-file). If output-file was specified, produced stream will be written into it. If the input-file and output-file are omitted, tutorial can be used to estimate constructed pipeline’s performance. Constructed pipeline is configured to work on video memory on the input and output. The performed VPP operation is x2 downscaling and noise reduction.

| Option | Description |
| ------:| -----------:|
| -sw | Loads SW Media SDK Library implementation |
| -hw | Loads HW Media SDK Library implementation |
| -auto | Automatically chooses Media SDK library implementation |
| -g WxH | Mandatory option. Sets input video geometry, i.e. width and height. Example: -g 1920x1080. |

| Argument | Description |
| ------:| -----------:|
| input-file | Optional argument. Sets incoming input file containing uncompressed video. If file will not be specified, tutorial will work in the performance mode: input will be simulated by producing empty input frames filled with some color. |
| output-file | Optional argument. Sets uncompressed video file to write output data. If file will not be specified, tutorial will work in the performance mode: it will process data, but will not produce any output. |

# Transcoding tutorials
## simple_5_transcode
**simple_transcode *[-sw | -hw | -auto] -b bitrate -f framerate input-file [ output-file ]***
[Command]
Tutorial demonstrates how to transcode given input raw H.264 video stream (input-file) into H.264 stream with different parameters. If output-file was specified, produced stream will be written into it. If output-file is omitted, tutorial can be used to estimate constructed pipeline’s performance. Constructed pipeline is configured to share system memory between decoder and encoder.

| Option | Description |
| ------:| -----------:|
| -sw | Loads SW Media SDK Library implementation |
| -hw | Loads HW Media SDK Library implementation |
| -auto | Automatically chooses Media SDK library implementation |
| -b bitrate | Mandatory option. Sets bitrate with which data should be encoded, in KBits-per-second. Example: -b 5000 to encode data at 5Mbit. |
| -f framerate | Mandatory option. Sets framerate with which data should be encoded in the form -f nominator/denominator. Example: -f 30/1. |

| Argument | Description |
| ------:| -----------:|
| input-file | Mandatory argument. Sets incoming input bitstream to process. Input data should be in raw H.264 format. |
| output-file | Optional argument. Sets raw H.264 video file to write output data. If file will not be specified, tutorial will work in the performance mode: it will process data, but will not produce any output |

## simple_5_transcode_vmem
**simple_transcode_vmem *[-sw | -hw | -auto] -b bitrate -f framerate input-file [ output-file ]***
Tutorial demonstrates how to transcode given input raw H.264 video stream (input-file) into H.264 stream with different parameters. If output-file was specified, produced stream will be written into it. If output-file is omitted, tutorial can be used to estimate constructed pipeline’s performance. Constructed pipeline is configured to share video memory between decoder and encoder.

| Option | Description |
| ------:| -----------:|
| -sw | Loads SW Media SDK Library implementation |
| -hw | Loads HW Media SDK Library implementation |
| -auto | Automatically chooses Media SDK library implementation |
| -b bitrate | Mandatory option. Sets bitrate with which data should be encoded, in KBits-per-second. Example: -b 5000 to encode data at 5Mbit. |
| -f framerate | Mandatory option. Sets framerate with which data should be encoded in the form -f nominator/denominator. Example: -f 30/1. |

| Argument | Description |
| ------:| -----------:|
| input-file | Mandatory argument. Sets incoming input bitstream to process. Input data should be in raw H.264 format. |
| output-file | Optional argument. Sets raw H.264 video file to write output data. If file will not be specified, tutorial will work in the performance mode: it will process data, but will not produce any output |

## simple_5_transcode_opaque
**simple_transcode_opaque *[-sw | -hw | -auto] -b bitrate -f framerate input-file [ output-file ]***
Tutorial demonstrates how to transcode given input raw H.264 video stream (input-file) into H.264 stream with different parameters. If output-file was specified, produced stream will be written into it. If output-file is omitted, tutorial can be used to estimate constructed pipeline’s performance. Constructed pipeline is configured to share opaque memory between decoder and encoder.

| Option | Description |
| ------:| -----------:|
| -sw | Loads SW Media SDK Library implementation |
| -hw | Loads HW Media SDK Library implementation |
| -auto | Automatically chooses Media SDK library implementation |
| -b bitrate | Mandatory option. Sets bitrate with which data should be encoded, in KBits-per-second. Example: -b 5000 to encode data at 5Mbit. |
| -f framerate | Mandatory option. Sets framerate with which data should be encoded in the form -f nominator/denominator. Example: -f 30/1. |

| Argument | Description |
| ------:| -----------:|
| input-file | Mandatory argument. Sets incoming input bitstream to process. Input data should be in raw H.264 format. |
| output-file | Optional argument. Sets raw H.264 video file to write output data. If file will not be specified, tutorial will work in the performance mode: it will process data, but will not produce any output |

## simple_5_transcode_opaque_async
**simple_transcode_opaque_async *[-sw | -hw | -auto] -b bitrate -f framerate input-file [ output-file ]***
Tutorial demonstrates how to transcode given input raw H.264 video stream (input-file) into H.264 stream with different parameters. If output-file was specified, produced stream will be written into it. If output-file is omitted, tutorial can be used to estimate constructed pipeline’s performance. Constructed pipeline is configured to share opaque memory between decoder and encoder and work in the asynchronous mode.

| Option | Description |
| ------:| -----------:|
| -sw | Loads SW Media SDK Library implementation |
| -hw | Loads HW Media SDK Library implementation |
| -auto | Automatically chooses Media SDK library implementation |
| -b bitrate | Mandatory option. Sets bitrate with which data should be encoded, in KBits-per-second. Example: -b 5000 to encode data at 5Mbit. |
| -f framerate | Mandatory option. Sets framerate with which data should be encoded in the form -f nominator/denominator. Example: -f 30/1. |

| Argument | Description |
| ------:| -----------:|
| input-file | Mandatory argument. Sets incoming input bitstream to process. Input data should be in raw H.264 format. |
| output-file | Optional argument. Sets raw H.264 video file to write output data. If file will not be specified, tutorial will work in the performance mode: it will process data, but will not produce any output |

## simple_6_transcode_opaque_lowlatency
**simple_transcode_opaque_lowlat *[-sw | -hw | -auto] -b bitrate -f framerate input-file [ output-file ]***
Tutorial demonstrates how to transcode given input raw H.264 video stream (input-file) into H.264 stream with different parameters. If output-file was specified, produced stream will be written into it. If output-file is omitted, tutorial can be used to estimate constructed pipeline’s performance. Constructed pipeline is configured to share opaque memory between decoder and encoder and work in the low latency mode, i.e. produce output as fast as possible.

| Option | Description |
| ------:| -----------:|
| -sw | Loads SW Media SDK Library implementation |
| -hw | Loads HW Media SDK Library implementation |
| -auto | Automatically chooses Media SDK library implementation |
| -b bitrate | Mandatory option. Sets bitrate with which data should be encoded, in KBits-per-second. Example: -b 5000 to encode data at 5Mbit. |
| -f framerate | Mandatory option. Sets framerate with which data should be encoded in the form -f nominator/denominator. Example: -f 30/1. |

| Argument | Description |
| ------:| -----------:|
| input-file | Mandatory argument. Sets incoming input bitstream to process. Input data should be in raw H.264 format. |
| output-file | Optional argument. Sets raw H.264 video file to write output data. If file will not be specified, tutorial will work in the performance mode: it will process data, but will not produce any output |

## simple_5_transcode_opaque_async_vppresize
**simple_transcode_opaque_async_vppresize *[-sw | -hw | -auto] -b bitrate -f framerate input-file [ output-file ]***
Tutorial demonstrates how to transcode given input raw H.264 video stream (input-file) into H.264 stream with different parameters. This tutorial adds VPP processing (x4 down-scaling) in the pipeline. If output-file was specified, produced stream will be written into it. If output-file is omitted, tutorial can be used to estimate constructed pipeline’s performance. Constructed pipeline is configured to share opaque memory between decoder and encoder and work in the asynchronous mode.

| Option | Description |
| ------:| -----------:|
| -sw | Loads SW Media SDK Library implementation |
| -hw | Loads HW Media SDK Library implementation |
| -auto | Automatically chooses Media SDK library implementation |
| -b bitrate | Mandatory option. Sets bitrate with which data should be encoded, in KBits-per-second. Example: -b 5000 to encode data at 5Mbit. |
| -f framerate | Mandatory option. Sets framerate with which data should be encoded in the form -f nominator/denominator. Example: -f 30/1. |

| Argument | Description |
| ------:| -----------:|
| input-file | Mandatory argument. Sets incoming input bitstream to process. Input data should be in raw H.264 format. |
| output-file | Optional argument. Sets raw H.264 video file to write output data. If file will not be specified, tutorial will work in the performance mode: it will process data, but will not produce any output |

