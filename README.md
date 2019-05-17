# Intel® Media SDK
Intel® Media SDK provides a plain C API to access hardware-accelerated video decode, encode and filtering on Intel® Gen graphics hardware platforms. Implementation written in C++ 11 with parts in C-for-Media (CM).

**Supported video encoders**: HEVC, AVC, MPEG-2, JPEG, VP9  
**Supported video decoders**: HEVC, AVC, VP8, VP9, MPEG-2, VC1, JPEG  
**Supported video pre-processing filters**: Color Conversion, Deinterlace, Denoise, Resize, Rotate, Composition

# Dependencies
Intel Media SDK depends on [LibVA](https://github.com/01org/libva/). 
This version of Intel Media SDK is compatible with the open source [Intel Media Driver for VAAPI](https://github.com/intel/media-driver).

# FAQ
You can find answers for the most frequently asked questions [here](https://software.intel.com/sites/default/files/managed/c0/8e/intel-media-sdk-open-source-faq.pdf).

# Table of contents

  * [License](#license)
  * [How to contribute](#how-to-contribute)
  * [Documentation](#documentation)
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

Please find full documentation under the `doc/` folder. Key documents:
* [Media SDK Developer Reference](./doc/mediasdk-man.pdf)
* [Media SDK Developer Reference Extensions for User-Defined Functions](./doc/mediasdkusr-man.pdf)
* [Media Samples Guide](./doc/samples/Media_Samples_Guide_Linux.pdf)

Visit our [Github Wiki](https://github.com/Intel-Media-SDK/MediaSDK/wiki) for the detailed setting and building instructions, runtime tips and other information.

You may also wish to visit Intel Media Server Studio [support page](https://software.intel.com/en-us/intel-media-server-studio-support/documentation) for additional documentation.

# Products which use Media SDK

* [Intel Media Server Studio](https://software.intel.com/en-us/intel-media-server-studio)
* [Intel Media SDK for Embedded Linux](https://software.intel.com/en-us/media-sdk/choose-download/embedded-iot)

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

Requires Microsoft Visual Studio 2015 or newer for building.

# How to build

## Build steps

Get sources with the following Git* command (pay attention that to get full Media SDK sources bundle it is required to have Git* with [LFS](https://git-lfs.github.com/) support):
```sh
git clone https://github.com/Intel-Media-SDK/MediaSDK msdk
cd msdk
```

To configure and build Media SDK install cmake version 2.8.5 or later and run the following commands:
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
| ENABLE_X11 | ON\|OFF | Enable X11 dependent code to be built (default: OFF) |
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
