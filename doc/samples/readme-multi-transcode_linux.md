

# Multi-Transcoding Sample

## Overview

**Multi-Transcoding Sample** works with **Intel® Media SDK** \(hereinafter referred to as "**SDK**"\)

It demonstrates how to use **SDK** API to create a console application that performs the transcoding \(decoding and encoding\) of a video stream from one compressed video format to another, with optional video processing \(resizing\) of uncompressed video prior to encoding. The application supports multiple input and output streams meaning it can execute multiple transcoding sessions concurrently.

The main goal of this sample is to demonstrate CPU/GPU balancing in order to get maximum throughput on Intel® hardware-accelerated platforms \(with encoding support\). This is achieved by running several transcoding pipelines in parallel and fully loading both CPU and GPU.

This sample also demonstrates integration of user-defined functions for video processing \(picture rotation plug-in\) into **SDK** transcoding pipeline.

This version of sample also demonstrates surface type neutral transcoding \(opaque memory usage\).

The sample is able to work with **HEVC Decoder & Encoder** \(hereinafter referred to as "**HEVC**"\).

## Features

**Multi-Transcoding Sample** supports the following video formats:

|Format type| |
|---|---|
| input \(compressed\) | H.264 \(AVC, MVC – Multi-View Coding\), MPEG-2 video, VC-1, JPEG\*/Motion JPEG, HEVC \(High Efficiency Video Coding\), VP8, VP9, AV1 |
| output \(compressed\) | H.264 \(AVC, MVC – Multi-View Coding\), MPEG-2 video, JPEG\*/Motion JPEG, HEVC \(High Efficiency Video Coding\), VP9 |

## Hardware Requirements

See [`<install-folder>/Media_Samples_Guide_Linux.md`](./Media_Samples_Guide_Linux.md).

## Software Requirements

See [`<install-folder>/Media_Samples_Guide_Linux.md`](./Media_Samples_Guide_Linux.md).

## How to Build the Application

See [`<install-folder>/Media_Samples_Guide_Linux.md`](./Media_Samples_Guide_Linux.md).

## Running the Software

See [`<install-folder>/Media_Samples_Guide_Linux.md`](./Media_Samples_Guide_Linux.md).



The executable file requires the following command-line switches to function properly:

Usage: `sample_multi_transcode [options] [--] pipeline-description`
or: `sample_multi_transcode [options] -par ParFile`

#### options:

|Option|Description|
|---|---|
| -? |           Print this help and exit|
|  -p <file-name>| Collect performance statistics in specified file|
 | -timeout <seconds>|  Set time to run transcoding in seconds|
  |-greedy|Use greedy formula to calculate number of surfaces|

#### ParFile format:
ParFile is extension of what can be achieved by setting pipeline in the command line. For more information on ParFile format see readme-multi-transcode.md.

|Option|Description|
|---|---|
| -par <par\_file\> | A parameter file is a configuration file of specific structure. It contains several command lines, each line corresponding to a single transcoding, decoding or encoding **SDK** session. |

#### Pipeline description (general options):

|Option|Description|
|---|---|
 |-i::h265\|h264\|mpeg2\|vc1\|mvc\|jpeg\|vp9\|av1 \<file-name\>| Set input file and decoder type|
 |-i::i420\|nv12 <file-name>| Set raw input file and color format|
  |-i::rgb4_frame | Set input rgb4 file for compositon. File should contain just one single frame (-vpp_comp_src_h and -vpp_comp_src_w should be specified as well).|
 | -o::h265\|h264\|mpeg2\|mvc\|jpeg\|vp9\|raw \<file-name\>\|null |  Set output file and encoder type. \'null\' keyword as file-name disables output file writing|
 | -sw\|-hw\|-hw_d3d11\|-hw_d3d9 SDK implementation to use:<br>-hw - platform-specific on default display adapter (default)<br>-hw_d3d11 - platform-specific via d3d11 <br>-hw_d3d9 - platform-specific via d3d9<br>-sw - software|
 | -mfe_frames| <N> maximum number of frames to be combined in multi-frame encode pipeline               0 - default for platform will be used|
  |-mfe_mode 0\|1\|2\|3| multi-frame encode operation mode - should be the same for all sessions<br>0, MFE operates as DEFAULT mode, decided by SDK if MFE enabled<br>1, MFE is disabled<br>2, MFE operates as AUTO mode<br>3, MFE operates as MANUAL mode|
  |-mfe_timeout <N\> | multi-frame encode timeout in milliseconds - set per sessions control|
 | -mctf [Strength]|Strength is an optional value;  it is in range [0...20]<br>value 0 makes MCTF operates in auto mode;<br>Strength: integer, [0...20]. Default value is 0.Might be a CSV filename (upto 15 symbols); if a string is convertable to an integer, integer has a priority over filename<br>In fixed-strength mode, MCTF strength can be adjusted at framelevel;<br>If no Strength is given, MCTF operates in auto mode.|
  |-robust| Recover from gpu hang errors as the come (by resetting components)|
 | -async| Depth of asynchronous pipeline. default value 1|
|  -join|         Join session with other session(s), by default sessions are not joined|
|  -priority| Use priority for join sessions. 0 - Low, 1 - Normal, 2 - High. Normal by default|
|  -threads num|  Number of session internal threads to create|
|  -n| Number of frames to transcode<br>(session ends after this number of frames is reached).<br>In decoding sessions (-o::sink) this parameter limits number<br>of frames acquired from decoder.<br>In encoding sessions (-o::source) and transcoding sessions<br>this parameter limits number of frames sent to encoder.
| -ext_allocator |   Force usage of external allocators|
|  -sys| Force usage of external system allocator|
|  -dec::sys| Set dec output to system memory|
|  -vpp::sys| Set vpp output to system memory|
|  -vpp::vid| Set vpp output to video memory|
|  -fps <frames per second\>|  Transcoding frame rate limit|
  |-pe | Set encoding plugin for this particular session.<br>This setting overrides plugin settings defined by SET clause.|
|  -pd|  Set decoding plugin for this particular session.<br> This setting overrides plugin settings defined by SET clause.<br>Supported values: hevcd_sw, hevcd_hw, hevce_sw, hevce_gacc, hevce_hw, vp8d_hw, vp8e_hw, vp9d_hw, vp9e_hw, camera_hw, capture_hw, h264_la_hw, ptir_hw, hevce_fei_hw<br>Direct GUID number can be used as well|
 |[-device /path/to/device] | Set graphics device for processing. For example: `-device /dev/dri/renderD128`. If not specified, defaults to the first Intel device found on the system. |
#### Pipeline description (encoding options):

|Option|Description|
|---|---|
| -b <Kbits per second\>| Encoded bit rate, valid for H.264, MPEG2 and MVC encoders|
|  -bm  | Bitrate multiplier. Use it when required bitrate isn't fit into 16-bit<br>Affects following parameters: InitialDelayInKB, BufferSizeInKB, TargetKbps, MaxKbps|
| -f <frames per second\>| Video frame rate for the FRC and deinterlace options|
|-fe <frames per second\>| Video frame rate for the FRC and deinterlace options (deprecated, will be removed in next versions).|
|-override_decoder_framerate <framerate\>|Forces decoder output framerate to be set to provided value (overwriting actual framerate from decoder)|
 | -override_encoder_framerate <framerate\>| Overwrites framerate of stream going into encoder input with provided value (this option does not enable FRC, it just ovewrites framerate value)|
|-u <usage\> |Target usage. Valid for H.265, H.264, MPEG2 and MVC encoders. Expected values:<br>veryslow (quality), slower, slow, medium (balanced), fast, faster, veryfast (speed)|
|  -q <quality\>| Quality parameter for JPEG encoder; in range [1,100], 100 is the best quality|
|-l <numSlices\>|  Number of slices for encoder; default value 0|
|-mss <maxSliceSize\>|Maximum slice size in bytes. Supported only with -hw and h264 codec. This option is not compatible with -l option.|
|-BitrateLimit:<on,off>| Modifies bitrate to be in the range imposed by the SDK encoder. Setting this flag off may lead to violation of HRD conformance. The default value is OFF, i.e. bitrate is not limited. It works with AVC only. |
|-la|Use the look ahead bitrate control algorithm (LA BRC) for H.264 encoder. Supported only with -hw option on 4th Generation Intel Core processors.|
| -lad <depth\>|Depth parameter for the LA BRC, the number of frames to be analyzed before encoding. In range [0,100].<br>If `depth` is `0` then the encoder forces the value to Max(10, 2\*`GopRefDist`) for LA_ICQ, and to Max(40, 2\*`GopRefDist`) otherwise.<br>If `depth` is in range [1,100] then the encoder forces the value to Max(2\*`GopRefDist`,2\*`NumRefFrame`,`depth`).<br>May be 1 in the case when -mss option is specified|
|  -la_ext| Use external LA plugin (compatible with h264 & hevc encoders)|
|-cbr| Constant bitrate control|
|-vbr| Variable bitrate control|
|-vcm| Video Conferencing Mode (VCM) bitrate control|
| -hrd <KBytes\> |Maximum possible size of any compressed frames|
| -wb <Kbits per second\>|Maximum bitrate for sliding window|
| -ws| Sliding window size in frames|
| -gop_size |Size of GOP structure in frames|
| -dist| Distance between I- or P- key frames|
| -num_ref| Number of reference frames|
 | -bref| Arrange B frames in B pyramid reference structure|
| -nobref|Do not use B-pyramid (by default the decision is made by library)|
|-bpyr|   Enable B pyramid|
|-gpb:<on,off> | Enable or disable Generalized P/B frames|
| -CodecProfile| Specifies codec profile|
|-CodecLevel| Specifies codec level|
|  -GopOptFlag:closed| Closed gop|
|  -GopOptFlag:strict | Strict gop|
|-InitialDelayInKB |The decoder starts decoding after the buffer reaches the initial size InitialDelayInKB, which is equivalent to reaching an initial delay of InitialDelayInKB*8000/TargetKbps ms|
| -MaxKbps| For variable bitrate control, specifies the maximum bitrate at which the encoded data enters the Video Buffering Verifier buffer|
| -gpucopy::<on,off> |Enable or disable GPU copy mode|
| -repartitioncheck::<on,off>| Enable or disable RepartitionCheckEnable mode|
 | -cqp|Constant quantization parameter (CQP BRC) bitrate control method<br>(by default constant bitrate control method is used), should be used along with -qpi, -qpp, -qpb.|
|-qpi| Constant quantizer for I frames (if bitrace control method is CQP). In range [1,51]. 0 by default, i.e. no limitations on QP.|
 | -qpp |Constant quantizer for P frames (if bitrace control method is CQP). In range [1,51]. 0 by default, i.e. no limitations on QP.|
  |-qpb| Constant quantizer for B frames (if bitrace control method is CQP). In range [1,51]. 0 by default, i.e. no limitations on QP.|
| -DisableQPOffset| Disable QP adjustment for GOP pyramid-level frames|
 |-qsv-ff| Enable QSV-FF mode (deprecated)|
 |-lowpower:<on,off>| Turn this option ON to enable QSV-FF mode|
|  -roi_file <roi-file-name>| Set Regions of Interest for each frame from <roi-file-name>|
|-roi_qpmap|Use QP map to emulate ROI for CQP mode|
|  -extmbqp| Use external MBQP map|

#### Pipeline description (vpp options):

|Option|Description|
|---|---|
|-deinterlace | Forces VPP to deinterlace input stream|
|  -deinterlace::ADI | Forces VPP to deinterlace input stream using ADI algorithm|
|  -deinterlace::ADI_SCD |Forces VPP to deinterlace input stream using ADI_SCD algorithm|
|  -deinterlace::ADI_NO_REF |Forces VPP to deinterlace input stream using ADI no ref algorithm|
| -deinterlace::BOB|  Forces VPP to deinterlace input stream using BOB algorithm|
|  -detail <level\>| Enables detail (edge enhancement) filter with provided level(0..100)|
|  -denoise <level\>|  Enables denoise filter with provided level (0..100)|
|  -FRC::PT|  Enables FRC filter with Preserve Timestamp algorithm|
| -FRC::DT| Enables FRC filter with Distributed Timestamp algorithm|
| -FRC::INTERP|  Enables FRC filter with Frame Interpolation algorithm<br>NOTE: -FRC filters do not work with -i::sink pipelines !!!|
|-ec::nv12\|rgb4\|yuy2\|nv16\|p010\|p210\|y210\|y410\|p016\|y216\|y416 |Forces encoder input to use provided chroma mode|
|-dc::nv12\|rgb4\|yuy2\|p010\|y210\|y410\|p016\|y216 | Forces decoder output to use provided chroma mode<br>NOTE: chroma transform VPP may be automatically enabled if -ec/-dc parameters are provided|
|-angle 180| Enables 180 degrees picture rotation user module before encoding|
| -opencl|Uses implementation of rotation plugin (enabled with -angle option) through Intel(R) OpenCL|
|-w| Destination picture width, invokes VPP resize or decoder fixed function resize engine (if -dec_postproc specified) |
|-h| Destination picture height, invokes VPP resize or decoder fixed function resize engine (if -dec_postproc specified)|
|-field_processing t2t\|t2b\|b2t\|b2b\|fr2fr | Field Copy feature|
|-scaling_mode| Specifies scaling mode (lowpower/quality) for VPP|
  |-WeightedPred::default\|implicit|  Enables weighted prediction usage|
|  -WeightedBiPred::default\|implicit|     Enambles weighted bi-prediction usage|
|  -extbrc:<on,off,implicit>| Enables external BRC for AVC and HEVC encoders|
|-ExtBrcAdaptiveLTR:<on,off>|Set AdaptiveLTR for implicit extbrc|
|-vpp_comp <sourcesNum\>| Enables composition from several decoding sessions. Result is written to the file|
|-vpp_comp_only <sourcesNum\>| Enables composition from several decoding sessions. Result is shown on screen|
 |-vpp_comp_num_tiles <Num\>| Quantity of tiles for composition. if equal to 0 tiles processing ignored|
  |-vpp_comp_dst_x| X position of this stream in composed stream (should be used in decoder session)|
  |-vpp_comp_dst_y  |Y position of this stream in composed stream (should be used in decoder session)|
  |-vpp_comp_dst_h|  Height of this stream in composed stream (should be used in decoder session)|
| -vpp_comp_dst_w |Width of this stream in composed stream (should be used in decoder session)|
|  -vpp_comp_src_h| Width of this stream in composed stream (should be used in decoder session)|
| -vpp_comp_src_w| Width of this stream in composed stream (should be used in decoder session)|
| -vpp_comp_tile_id| Tile_id for current channel of composition (should be used in decoder session)|
  |-vpp_comp_dump <file-name>|  Dump of VPP Composition's output into file. Valid if with -vpp_comp* options|
  |-vpp_comp_dump <null_render\>| Disabling rendering after VPP Composition. This is for performance measurements|
  |-dec_postproc |Resize after decoder using direct pipe (should be used in decoder session)|
| -single_texture_d3d11|  single texture mode for d3d11 allocator|
| -TargetBitDepthLuma | Target encoding bit-depth for luma samples. May differ from source one. 0 mean default target bit-depth which is equal to source |
| -TargetBitDepthChroma | Target encoding bit-depth for chroma samples. May differ from source one. 0 mean default target bit-depth which is equal to source. |

#### stat logging:

|Option|Description|
|---|---|
 |-stat <N\>|Output statistic every N transcoding cycles|
|  -stat-log <name\>|  Output statistic to the specified file (opened in append mode)|
|  -stat-per-frame <name\>|   Output per-frame latency values to a file (opened in append mode). The file name will be for an input session: <name\>_input_ID_<N\>.log or, for output session: <name>_output_ID_<N\>.log; <N\> - a number of a session.|

The command-line interface allows 2 usage models \(which can be mixed within one parameter file\):

1.  Multiple intra-session transcoding: several transcoding sessions, any number of sessions can be joined. Each session includes decoding, preprocessing \(optional\), and encoding.
2.  Multiple inter-session transcoding: output of a single decoding session serves as input for several encoding sessions. Either all or none of the sessions are joined. Any of the encoding sessions can optionally include preprocessing \(resizing\).

Below are several examples of parameter file contents.

Single intra-session transcoding:

```
-i::vc1 input.vc1 -async 10 -o::h264 output.h264 –n 100 –w 320 –h 240 –f 30 –b 2000 –u speed
```

Multiple intra-session transcoding, several sessions joined:

```
-i::vc1 input1.vc1 -async 10 -o::mpeg2 output1.mpeg2
-i::h264 input2.h264 -o::mpeg2 output2.mpeg2 –join
-i::h264 input3.h264 -o::mpeg2 output3.mpeg2 –join
```

Multiple inter-session transcoding, all sessions joined:

```
-i::h264 input.h264 -o::sink –join
-o::mpeg2 output1.mpeg2 -i::source –join –w 640 –h 480
-o::mpeg2 output2.mpeg2 -async 2 -u 3 -i::source -join
```

Mixed model:

```
-i::vc1 input.vc1 -async 10 -o::h264 output.h264 –n 100 –w 320 –h 240 –f 30 –b 2000 –u speed
-hw -i::h264 input.h264 -o::sink –join
-o::mpeg2 output1.mpeg2 -i::source –join –w 640 –h 480
-o::mpeg2 output2.mpeg2 -async 2 -u 3 -i::source –join
```

Single intra-session MVC transcoding:

```
-i::mvc input.mvc -async 10 -o::mvc output.mvc –n 100 –w 320 –h 240 –f 30 –b 2000 –u speed
```

Please, also pay attention on “Running the Software” section of [`<install-folder>/Media_Samples_Guide_Linux.md`](./Media_Samples_Guide_Linux.md) document where you will find important notes on backend specific usage \(drm and x11\).

## ROI file format description

ROI file has the following format:
```
roi_count_frame_1;
    roi1_left1; roi1_top1; roi1_right1; roi1_bottom1; roi1_dqp1;
    roi2_left1; roi2_top1; roi2_right1; roi2_bottom1; roi2_dqp1;
roi_count_frame_2;
    roi1_left2; roi1_top2; roi1_right2; roi1_bottom2; roi1_dqp2;
    roi2_left2; roi2_top2; roi2_right2; roi2_bottom2; roi2_dqp2;
roi_count_frame_n;
    roi1_leftn; roi1_topn; roi1_rightn; roi1_bottomn; roi1_dqpn;
    roi2_leftn; roi2_topn; roi2_rightn; roi2_bottomn; roi2_dqpn;
    ...
```

Values are separated by semicolons. Each entry starts with a count that represents the ROI information for a single frame. The count indicates the number of (5 entry) ROI descriptions provided for the frame (up to 256 ROIs).

Example: a ROI file for a two frame stream where the first frame has two ROIs with -8 and 5 delta QP values, and the second frame includes three ROIs with -8, 8, and -4 delta QP values:

```
2;
    1104;592;1216;928;-8;
    1200;544;1504;848; 5;
3;
    1088;576;1200;912;-8;
    1200;528;1488;832; 8;
    944; 400;1264;512;-4;
```

**Tips**

1.  To achieve maximum throughput use `–async` >= 5 and the –join option when running several transcoding pipelines.
2.  If you need only one transcoding session you can avoid creating a par file and pass the arguments of this session to the application using command line. E.g.:

```
sample_multi_transcode -i::mpeg2 input.mpeg2 -async 10 -o::h264 output.h264
–n 100 –w 320 –h 240 –f 30 –b 2000 –u speed -p 1.perf –hw
```

## HEVC Plugins

HEVC codec is implemented as a plugin unlike codecs such as MPEG2 and AVC. There are 3 implementations of HEVC: Hardware or GPU \(HW\), Software or CPU \(SW\) for both decode and encode and GPU-Accelerated \(GACC\) only for encoder.

**Note 1:** The HEVC SW and GACC plugins are available only in the HEVC package which is part of the Intel® Media Software Development Kit 2018. You can find the available plugins and their IDs from `$MFX_ROOT/include/mfxplugin.h` file.

**Note 2:** HW plugins for HEVC encode and decode are supported starting from 6th Generation of Intel CoreTM Processors, Intel® Xeon® E3-1200 and E3-1500 v5 Family with Intel® Processor Graphics 500 Series \(codename Skylake\).

**Note 3:** Multi-transcoding sample loads the HW HEVC plugins with HW library and SW plugins with SW library by default. You can enforce a plugin to be loaded by specifying its hexadecimal GUID or path using "-pe" parameter for encode plugins and "-pd" parameter for decode plugins. If you need to run one plugin for multiple sessions, use "set" option.

At the example below multi-transcoding sample runs HW library with HW decode and encode plugins:

```
sample_multi_transcode -i::h265 input.265 -o::h265 out.h265 -w 480 -h 320
```

The following command line loads SW lib and SW plugins:

```
sample_multi_transcode -i::h265 input.265 -o::h265 out.h265 -w 480 -h 320 -sw
```

This is an example how to use HW library with SW plugins:

```
sample_multi_transcode -i::h265 ../content/test_stream.265 -pd 15dd936825ad475ea34e35f3f54217a6 -o::h265 out.h265 -w 480 -h 320 -pe 2fca99749fdb49aeb121a5b63ef568f7
```

HW library+SW decoder+GACC encoder:

```
sample_multi_transcode -i::h265 ../content/test_stream.265 -pd 15dd936825ad475ea34e35f3f54217a6 -o::h265 out.h265 -w 480 -h 320 -pe e5400a06c74d41f5b12d430bbaa23d0b
```

Multiple intra-session transcoding with the same SW HEVC plugin is used in both cases:

```
set -i::h265 15dd936825ad475ea34e35f3f54217a6
-i::h265 input1.265 -o::h264 output1.264
-i::h265 input2.265 -o::mpeg2 output2.mpeg2
```

Multiple intra-session transcoding using set clause:

```
set -i::h265 /path/to/so/decoder_plugin.so
set -o::h265 /path/to/so/encoder_plugin.so
-i::h265 input1.265 -o::h264 output1.264
-i::mpeg2 input2.mpeg2 -o::h265 output2.265
```

## Known Limitations

-   To use lookahead for HEVC encode, we need to have h264 LA plugin and the HEVC HW encode plugin, run in separate sessions. Following par file is an example of lookahead bitrate for HEVC encode:
    ```
    -i::h265 input.h265 -o::sink  -hw -async 1 -la -la_ext -bpyr -dist 8 -join
    -i::source -o::h265 output.h265 -u 4 -hw -b 500 -async 1 -bpyr -dist 8 -join
    ```

-   Configurations <multiple joined inter-session transcoding where one of the encoders is MPEG2\> are not supported when sample application uses platform-specific **SDK** implementation on systems with Intel® HD Graphics 3000/2000 and 4000/2500. Application can exit with error or hang. An example of a corresponding par file is given below:

    ```
    -i::h264 input.h264 -o::sink –join
    -o::mpeg2 output1.mpeg2 -i::source –join
    -o::h264 output2.h264 -i::source –join
    ```

    Systems with Intel® Iris™ Pro Graphics, Intel® Iris™ Graphics and Intel® HD Graphics 4200+ Series are free from this limitation.

-   Picture rotation sample plug-ins do not swap view order in the pipeline with MVC encoder. This should be considered if viewing of the output video is involved.

-   In case of using HEVC plugin \(h265 video type\), plugin type \(hardware or software\) used by default is set depending on -sw or -hw sample options. However, hardware HEVC plugins work on specific platforms only. To force usage of specific HEVC plugin implementation, please use -pe and -pd options with proper plugin GUID.
-   SW HEVC plugin in 10bit mode cannot be used together with HW library VPP. Although library allows that, this is bad practice because additional per-pixel data shift is required. Please use HW HEVC + HW library or SW HEVC + SW library instead.
-   Sample may crash if composition filter is used with more than 10 sources \(because of limitations in MSDK library\).
-   -timeout option set in command line together with par file name may work incorrectly. Please use -timeout option set inside par file instead.
-   Sample may not function properly on systems that have a non-Intel VGA controller as the first \(primary\) because Intel device is not first in the list.

    To workaround this issue, swap names of DRI device files:

    `$ cd /dev && mv card0 tmp && mv card1 card0 && mv tmp card1`

    and do the same for the files `control64/65` and `renderD128/129`

-   In case of inter-session transcoding \(with separate decoding and encoding sessions\) -n option should be set both for sessions to get exact number of output frames.

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
