# Intel® Media SDK
Intel® Media SDK provides an API to access hardware-accelerated video decode, encode and filtering on Intel® platforms with integrated graphics.

**Supported video encoders**: HEVC, AVC, MPEG-2, JPEG  
**Supported Video decoders**: HEVC, AVC, VP8, MPEG-2, VC1, JPEG  
**Supported video pre-processing filters**: Color Conversion, Deinterlace, Denoise, Resize, Rotate

# Important note
The current version of Intel Media SDK is only compatible with the Linux* Graphics Driver from [Intel® Media Server Studio for Linux*](https://software.intel.com/en-us/intel-media-server-studio). Unfortunately no publicly avaliable driver compatible with this version Intel Media SDK.
Intel Media SDK depends on a special version of [LibVA v2.0-branch-next](https://github.com/xhaihao/libva/tree/v2.0-branch-next) which comes with Intel Media Server Studio installation and is not in upstream, so this version is not compatible with the LibVA/driver available at [01org](https://01.org/linuxgraphics/downloads ).

# FAQ
You can find answers for the most frequently asked questions [here](https://software.intel.com/sites/default/files/managed/c0/8e/intel-media-sdk-open-source-faq.pdf).

# Table of contents

  * [License](#license)
  * [How to contribute](#how-to-contribute)
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

# System requirements
**Operating System:** CentOS 7.3 is the recommended and the only tested platform.  
**Hardware:**
Intel platforms with integrated graphics:
 - Intel® Xeon® E3-1200 v4 family with C226 chipset
 - Intel Xeon E3-1200 and E3-1500 v5 families with C236 chipset
 - 5th Generation Intel® Core™ processors
 - 6th Generation Intel Core processors

Please find documentation under the `doc/` folder or navigate to [Intel Media Server Studio support page](https://software.intel.com/en-us/intel-media-server-studio-support/documentation).

# How to build
## Requirements
 - Git* (with [LFS](https://git-lfs.github.com/) support)
 - Perl* v5.16+
 - Cmake* v2.8+
 - GCC* v4.8+
 - Intel Media Server Studio 2017 R3

## Build steps

Get sources
```sh
git clone https://github.com/Intel-Media-SDK/MediaSDK msdk
cd msdk
```

Set up environment variables:
```sh
export MFX_HOME=`pwd`
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
Intel Media Server Studio: https://software.intel.com/en-us/intel-media-server-studio

Intel Media SDK: https://software.intel.com/en-us/media-sdk
