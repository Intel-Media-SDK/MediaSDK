# Intel® Media SDK
Intel® Media SDK provides an API to access hardware-accelerated video decode, encode and filtering on Intel® platforms with integrated graphics.

**Supported video encoders**: HEVC, AVC, MPEG-2, JPEG  
**Supported Video decoders**: HEVC, AVC, VP8, MPEG-2, VC1, JPEG  
**Supported video pre-processing filters**: Color Conversion, Deinterlace, Denoise, Resize, Rotate, Composition

# Important note
The current version of Intel Media SDK is compatible with the open source [Intel Media Driver for VAAPI](https://github.com/intel/media-driver).
Intel Media SDK depends on [LibVA](https://github.com/01org/libva/). 

# FAQ
You can find answers for the most frequently asked questions [here](https://software.intel.com/sites/default/files/managed/c0/8e/intel-media-sdk-open-source-faq.pdf).

# Table of contents

  * [License](#license)
  * [How to contribute](#how-to-contribute)
  * [Documentation](#Documentation)
  * [Products which use Media SDK](#products-which-use-media-sdk)
  * [System requirements](#system-requirements)
  * [How to build](#how-to-build)
    * [Requirements](#requirements)
    * [Build steps](#build-steps)
    * [Enabling Instrumentation and Tracing Technology](#enabling-instrumentation-and-tracing-technology)
  * [See also](#see-also)

# License
Intel Media SDK is licensed under MIT license. See [LICENSE](./LICENSE) for details.

# How to contribute
See [CONTRIBUTING](./CONTRIBUTING.md) for details. Thank you!

# Documentation

Please find full documentation under the `doc/` folder. Key documents:
* [Media SDK Developer Reference](./doc/mediasdk-man.pdf)
* [Media SDK Developer Reference Extensions for User-Defined Functions](./doc/mediasdkusr-man.pdf)
* [Media Samples Guide](./doc/samples/Media_Samples_Guide_Linux.pdf)

You may also wish to visit Intel Media Server Studio [support page](https://software.intel.com/en-us/intel-media-server-studio-support/documentation) for additional documentation.

# Products which use Media SDK

* [Intel Media Server Studio](https://software.intel.com/en-us/intel-media-server-studio)
* [Intel Media SDK for Embedded Linux](https://software.intel.com/en-us/media-sdk/choose-download/embedded-iot)

# System requirements

**Operating System:** Linux

**Software:**
* [LibVA](https://github.com/intel/libva)
* VAAPI backend driver:
  * [Intel Media Driver for VAAPI](https://github.com/intel/media-driver)
* Some features require CM Runtime library (part of [Intel Media Driver for VAAPI](https://github.com/intel/media-driver) package)

**Hardware:** Intel platforms supported by the [Intel Media Driver for VAAPI](https://github.com/intel/media-driver)

Media SDK test and sample applications may require additional software packages (for example, X Server, Wayland, LibDRM, etc.) to be functional.

# How to build
## Requirements
 - Git* (with [LFS](https://git-lfs.github.com/) support)
 - Perl* v5.16+
 - Cmake* v2.8+
 - GCC* v4.8+
 - [LibVA](https://github.com/intel/libva)

## Build steps

Get sources
```sh
git clone https://github.com/Intel-Media-SDK/MediaSDK msdk
cd msdk
```

Configure build with GCC (default) compiler:
```sh
perl tools/builder/build_mfx.pl --cmake=intel64.make.release
```
This will build MSDK binaries and MSDK samples.

If you want to configure build with CLang compiler use the following command:
```sh
perl tools/builder/build_mfx.pl --cmake=intel64.make.release.clang
```

Run build:
```sh
make -j8 -C __cmake/intel64.make.release
```

## Enabling Instrumentation and Tracing Technology
To enable the Instrumentation and Tracing Technology API you need either Intel® VTune™ Amplifier installed or to manually build an open source version. You can get ITT source files from [GitHub](https://github.com/01org/IntelSEAPI/tree/master/ittnotify) and build it on your own.

**Please note** that auto detection of the Intel VTune Amplifier configuration is not supported. The next step is mandatory if you want to use this feature: set `$ITT_PATH` so `$ITT_PATH/include/ittnotify.h` and `$ITT_PATH/libittnotify64.a` will be valid paths. MSDK build system will automatically detect it.

# See also

Intel Media SDK: https://software.intel.com/en-us/media-sdk
