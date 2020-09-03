# Intel® Media SDK
Intel® Media SDK provides a plain C API to access hardware-accelerated video decode, encode and filtering on Intel® Gen graphics hardware platforms. Implementation written in C++ 11 with parts in C-for-Media (CM).

**Supported video encoders**: HEVC, AVC, MPEG-2, JPEG, VP9  
**Supported video decoders**: HEVC, AVC, VP8, VP9, MPEG-2, VC1, JPEG  
**Supported video pre-processing filters**: Color Conversion, Deinterlace, Denoise, Resize, Rotate, Composition  

Media SDK is a part of Intel software stack for graphics:
* [Linux Graphics Drivers](https://intel.com/linux-graphics-drivers) - General Purpose GPU Drivers for Linux&ast; Operating Systems
  * Visit [documentation](https://dgpu-docs.intel.com) for instructions on installing, deploying, and updating Intel software to enable general purpose GPU (GPGPU) capabilities for Linux&ast;-based operating system distributions.

# Dependencies
Intel Media SDK depends on [LibVA](https://github.com/01org/libva/).
This version of Intel Media SDK is compatible with the open source [Intel Media Driver for VAAPI](https://github.com/intel/media-driver).

# Table of contents

  * [License](#license)
  * [How to contribute](#how-to-contribute)
  * [Documentation](#documentation)
  * [Tutorials](#tutorials)
  * [Products which use Media SDK](#products-which-use-media-sdk)
  * [System requirements](#system-requirements)
  * [How to build](#how-to-build)
    * [Build steps](#build-steps)
    * [Enabling Instrumentation and Tracing Technology (ITT)](#enabling-instrumentation-and-tracing-technology-itt)
  * [Recommendations](#recommendations)
  * [See also](#see-also)

# License
Intel Media SDK is licensed under MIT license. See [LICENSE](./LICENSE) for details.

# How to contribute
See [CONTRIBUTING](./CONTRIBUTING.md) for details. Thank you!

# Documentation

To get copy of Media SDK documentation use Git* with [LFS](https://git-lfs.github.com/) support.

Please find full documentation under the [./doc](./doc) folder. Key documents:
* [Media SDK Manual](./doc/mediasdk-man.md)
* Additional Per-Codec Manuals:
  * [Media SDK JPEG Manual](./doc/mediasdkjpeg-man.md)
  * [Media SDK VP8 Manual](./doc/mediasdkvp8-man.md)
* Advanced Topics:
  * [Media SDK User Plugins Manual](./doc/mediasdkusr-man.md)
  * [Media SDK FEI Manual](./doc/mediasdkfei-man.md)
  * [Media SDK HEVC FEI Manual](./doc/mediasdkhevcfei-man.md)
  * [MFE Overview](./doc/MFE-Overview.md)
  * [HEVC FEI Overview](./doc/HEVC_FEI_overview.pdf)
  * [Interlace content support in HEVC encoder](./doc/mediasdk_hevc_interlace_whitepaper.md)

Generic samples information is available in [Media Samples Guide](./doc/samples/Media_Samples_Guide_Linux.md)

Linux Samples Readme Documents:
* [Sample Multi Transcode](./doc/samples/readme-multi-transcode_linux.md)
* [Sample Decode](./doc/samples/readme-decode_linux.md)
* [Sample Encode](./doc/samples/readme-encode_linux.md)
* [Sample VPP](./doc/samples/readme-vpp_linux.md)
* [Metrics Monitor](./doc/samples/readme-metrics_monitor_linux.md)

Visit our [Github Wiki](https://github.com/Intel-Media-SDK/MediaSDK/wiki) for the detailed setting and building instructions, runtime tips and other information.

# Tutorials

* [Tutorials Overview](./doc/tutorials/mediasdk-tutorials-readme.md)
* [Tutorials Command Line Reference](./doc/tutorials/mediasdk-tutorials-cmd-reference.md)

# Products which use Media SDK

Use Media SDK via popular frameworks:
* [FFmpeg](http://ffmpeg.org/) via [ffmpeg-qsv](https://trac.ffmpeg.org/wiki/Hardware/QuickSync) plugins
* [GStreamer](https://gstreamer.freedesktop.org/) via plugins set included
  into [gst-plugins-bad](https://gitlab.freedesktop.org/gstreamer/gst-plugins-bad)

Learn best practises and borrow fragments for final solutions:
* https://github.com/intel/media-delivery
  * This collection of samples demonstrates best practices to achieve optimal video quality and
    performance on Intel GPUs for content delivery networks. Check out the demo, recommended command
    lines and quality and performance measuring tools.

Use Media SDK via other Intel products:
* [OpenVINO Toolkit](https://github.com/openvinotoolkit/openvino)
  * This toolkit allows developers to deploy pre-trained deep learning models through a high-level C++ Inference Engine API integrated with application logic.
* [Open Visual Cloud](https://github.com/OpenVisualCloud)
  * The Open Visual Cloud is a set of open source software stacks (with full end-to-end sample pipelines) for media, analytics, graphics and immersive media, optimized for cloud native deployment on commercial-off-the-shelf x86 CPU architecture.


# System requirements

**Operating System:**
* Linux x86-64 fully supported
* Linux x86 only build
* Windows (not all features are supported in Windows build - see Known Limitations for details)

**Software:**
* [LibVA](https://github.com/intel/libva)
* VAAPI backend driver:
  * [Intel Media Driver for VAAPI](https://github.com/intel/media-driver)
* Some features require CM Runtime library (part of [Intel Media Driver for VAAPI](https://github.com/intel/media-driver) package)

**Hardware:** Intel platforms supported by the [Intel Media Driver for VAAPI](https://github.com/intel/media-driver)

Media SDK test and sample applications may require additional software packages (for example, X Server, Wayland, LibDRM, etc.) to be functional.

**Operating System:** Windows **(experimental)**

Requires Microsoft Visual Studio 2017 for building.

# How to build

## Build steps

Get sources with the following Git* command (pay attention that to get full Media SDK sources bundle it is required to have Git* with [LFS](https://git-lfs.github.com/) support):
```sh
git clone https://github.com/Intel-Media-SDK/MediaSDK msdk
cd msdk
```

To configure and build Media SDK install cmake version 3.6 or later and run the following commands:
```sh
mkdir build && cd build
cmake ..
make
make install
```
Media SDK depends on a number of packages which are identified and checked for the proper version during configuration stage. Please, make sure to install these packages to satisfy Media SDK requirements. After successful configuration 'make' will build Media SDK binaries and samples. The following cmake configuration options can be used to customize the build:

| Option | Values | Description |
| ------ | ------ | ----------- |
| API | master\|latest\|major.minor | Build mediasdk library with specified API. 'latest' will enable experimental features. 'master' will configure the most recent available published API (default: master). |
| ENABLE_OPENCL | ON\|OFF | Enable OpenCL dependent code to be built (default: ON) |
| ENABLE_X11_DRI3 | ON\|OFF | Enable X11 DRI3 dependent code to be built (default: OFF) |
| ENABLE_WAYLAND | ON\|OFF | Enable Wayland dependent code to be built (default: OFF) |
| ENABLE_ITT | ON\|OFF | Enable ITT (VTune) instrumentation support (default: OFF) |
| ENABLE_TEXTLOG | ON\|OFF | Enable textlog trace support (default: OFF) |
| ENABLE_STAT | ON\|OFF | Enable stat trace support (default: OFF) |
| BUILD_ALL | ON\|OFF | Build all the BUILD_* targets below (default: OFF) |
| BUILD_RUNTIME | ON\|OFF | Build mediasdk runtime, library and plugins (default: ON) |
| BUILD_SAMPLES | ON\|OFF | Build samples (default: ON) |
| BUILD_TESTS | ON\|OFF | Build unit tests (default: OFF) |
| USE_SYSTEM_GTEST | ON\|OFF | Use system gtest version instead of bundled (default: OFF) |
| BUILD_TOOLS | ON\|OFF | Build tools (default: OFF) |
| MFX_ENABLE_KERNELS | ON\|OFF | Build mediasdk with [media shaders](https://github.com/Intel-Media-SDK/MediaSDK/wiki/Media-SDK-Shaders-(EU-Kernels)) support (default: ON) |


The following cmake settings can be used to adjust search path locations for some components Media SDK build may depend on:

| Setting | Values | Description |
| ------- | ------ | ----------- |
| CMAKE_ITT_HOME | Valid system path | Location of ITT installation, takes precendence over CMAKE_VTUNE_HOME (by default not defined) |
| CMAKE_VTUNE_HOME | Valid system path | Location of VTune installation (default: /opt/intel/vtune_amplifier) |

Visit our [Github Wiki](https://github.com/Intel-Media-SDK/MediaSDK/wiki) for advanced topics on setting and building Media SDK.

## Enabling Instrumentation and Tracing Technology (ITT)

To enable the Instrumentation and Tracing Technology (ITT) API you need to:
* Either install [Intel® VTune™ Amplifier](https://software.intel.com/en-us/intel-vtune-amplifier-xe)
* Or manually build an open source version (see [IntelSEAPI](https://github.com/01org/IntelSEAPI/tree/master/ittnotify) for details)

and configure Media SDK with the -DENABLE_ITT=ON. In case of VTune it will be searched in the default location (/opt/intel/vtune_amplifier). You can adjust ITT search path with either CMAKE_ITT_HOME or CMAKE_VTUNE_HOME.

Once Media SDK was built with ITT support, enable it in a runtime creating per-user configuration file ($HOME/.mfx_trace) or a system wide configuration file (/etc/mfx_trace) with the following content:
```sh
Output=0x10
```
# Known limitations
Windows build contains only samples and dispatcher library. MediaSDK library DLL is provided with Windows GFX driver.

# Recommendations

* In case of GCC compiler it is strongly recommended to use GCC version 6 or later since that's the first GCC version which has non-experimental support of C++11 being used in Media SDK.

# See also

Intel Media SDK: https://software.intel.com/en-us/media-sdk
