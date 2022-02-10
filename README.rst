Intel® Media SDK
================

.. contents::

Intel® Media SDK provides a plain C API to access hardware-accelerated video decode, encode and filtering on Intel® Gen graphics hardware platforms. Implementation written in C++ 11 with parts in C-for-Media (CM).

**Supported video encoders**: HEVC, AVC, MPEG-2, JPEG, VP9  

**Supported video decoders**: HEVC, AVC, VP8, VP9, MPEG-2, VC1, JPEG, AV1  

**Supported video pre-processing filters**: Color Conversion, Deinterlace, Denoise, Resize, Rotate, Composition  

Media SDK is a part of Intel software stack for graphics:

* `Linux Graphics Drivers <https://intel.com/linux-graphics-drivers>`_ - General Purpose GPU Drivers for Linux&ast; Operating Systems

  * Visit `documentation <https://dgpu-docs.intel.com>`_ for instructions on installing, deploying, and updating Intel software to enable general purpose GPU (GPGPU) capabilities for Linux&ast;-based operating system distributions.

Media SDK Support Matrix
------------------------

Pay attention that Intel® Media SDK lifetime comes to an end in a form it
exists right now. In particular, 

* API 1.35 is projected to be the last API of 1.x API series
* Runtime library (libmfxhw64.so.1) is not planned to get support of new Gen platforms
* Project is going to be supported in maintainence mode, critical fixes only

All future development is planned to happen within
`oneVPL <https://github.com/oneapi-src/oneVPL>`_ library and its runtime
implementations which are direct successors of Intel® Media SDK. oneVPL introduces
API 2.x series which is not backward compatible with API 1.x series (some
`features got dropped <https://spec.oneapi.com/versions/latest/elements/oneVPL/source/appendix/VPL_intel_media_sdk.html>`_).
New `VPL Runtime for Gen Graphics <https://github.com/oneapi-src/oneVPL-intel-gpu>`_
(libmfx-gen.so.1.2) comes with the support of new Gen platforms.

Pay attention that Intel® Media SDK has forward compatibility with new VPL
runtime (libmfx-gen.so.1.2) in the scope of API features supported by **both** 1.x
and 2.x API series. As such, if application is built against Intel® Media
SDK, it still can work on new platforms. For that purpose Media SDK Dispatcher
loads either Media SDK Legacy Runtime (libmfxhw64.so.1) or VPL Runtime (libmfx-gen.so.1.2)
depending on the underlying platform. See support matrix below.

+----------------------+--------------+-------------------------------------+-------------------------------------+
|                      |              | GPU supported by                    | Runtime loaded by libmfx.so.1       |
+----------------------+--------------+-----------------+-------------------+-----------------+-------------------+
| GPU                  | Type         | libmfxhw64.so.1 | libmfx-gen.so.1.2 | libmfxhw64.so.1 | libmfx-gen.so.1.2 |
+======================+==============+=================+===================+=================+===================+
| BDW (Broadwell)      | Legacy       | ✔               |                   | ✔               |                   |
+----------------------+--------------+-----------------+-------------------+-----------------+-------------------+
| SKL (Skylake)        | Legacy       | ✔               |                   | ✔               |                   |
+----------------------+--------------+-----------------+-------------------+-----------------+-------------------+
| BXT (Broxton)        | Legacy       | ✔               |                   | ✔               |                   |
+----------------------+--------------+-----------------+-------------------+-----------------+-------------------+
| APL (Apollo Lake)    | Legacy       | ✔               |                   | ✔               |                   |
+----------------------+--------------+-----------------+-------------------+-----------------+-------------------+
| KBLx [1]             | Legacy       | ✔               |                   | ✔               |                   |
+----------------------+--------------+-----------------+-------------------+-----------------+-------------------+
| ICL (Ice Lake)       | Legacy       | ✔               |                   | ✔               |                   |
+----------------------+--------------+-----------------+-------------------+-----------------+-------------------+
| JSL (Jasper Lake)    | Legacy       | ✔               |                   | ✔               |                   |
+----------------------+--------------+-----------------+-------------------+-----------------+-------------------+
| EHL (Elkhart Lake)   | Legacy       | ✔               |                   | ✔               |                   |
+----------------------+--------------+-----------------+-------------------+-----------------+-------------------+
| TGL (Tiger Lake)     | Legacy       | ✔               | ✔                 | ✔               |                   |
+----------------------+--------------+-----------------+-------------------+-----------------+-------------------+
| DG1 (Xe MAX)         | Legacy       | ✔               | ✔                 | ✔               |                   |
+----------------------+--------------+-----------------+-------------------+-----------------+-------------------+
| SG1                  | Legacy       | ✔               |                   | ✔               |                   |
+----------------------+--------------+-----------------+-------------------+-----------------+-------------------+
| RKL (Rocket Lake)    | Legacy       | ✔               | ✔                 | ✔               |                   |
+----------------------+--------------+-----------------+-------------------+-----------------+-------------------+
| ADL-S (Alder Lake S) | VPL          |                 | ✔                 |                 | ✔                 |
+----------------------+--------------+-----------------+-------------------+-----------------+-------------------+
| ADL-P (Alder Lake P) | VPL          |                 | ✔                 |                 | ✔                 |
+----------------------+--------------+-----------------+-------------------+-----------------+-------------------+
| Future platforms...  | VPL          |                 | ✔                 |                 | ✔                 |
+----------------------+--------------+-----------------+-------------------+-----------------+-------------------+
| Multi GPU            | Legacy + VPL | See above                           | ✔               | via env var [2]   |
+----------------------+--------------+-----------------+-------------------+-----------------+-------------------+

**Notes**:

* [1] KBLx is one of: KBL (Kaby Lake), CFL (Coffe Lake), WHL (Whiskey Lake), CML (Comet Lake), AML (Amber Lake)
* [2] On the multi GPU system which has both "Legacy" and "VPL" GPUs Media SDK Dispatcher loads Media SDK Legacy
  Runtime (libmfxhw64.so.1) by default. VPL Runtime (libmfx-gen.so.1.2) or Media SDK Runtime (libmfxhw64.so.1)
  can be explicitly selected via the following environment variable::

    export INTEL_MEDIA_RUNTIME=ONEVPL  # for VPL Runtime: libmfx-gen.so.1.2
    or
    export INTEL_MEDIA_RUNTIME=MSDK    # for Media SDK Runtime: libmfxhw64.so.1


Dependencies
------------

Intel Media SDK depends on `LibVA <https://github.com/intel/libva/>`_.
This version of Intel Media SDK is compatible with the open source `Intel Media Driver for VAAPI <https://github.com/intel/media-driver>`_.

License
-------

Intel Media SDK is licensed under MIT license. See `LICENSE <./LICENSE>`_ for details.

How to contribute
-----------------

See `CONTRIBUTING <./CONTRIBUTING.md>`_ for details. Thank you!

Documentation
-------------

To get copy of Media SDK documentation use Git* with `LFS <https://git-lfs.github.com/>`_ support.

Please find full documentation under the `./doc <./doc>`_ folder. Key documents:

* `Media SDK Manual <./doc/mediasdk-man.md>`_
* Additional Per-Codec Manuals:

  * `Media SDK JPEG Manual <./doc/mediasdkjpeg-man.md>`_
  * `Media SDK VP8 Manual <./doc/mediasdkvp8-man.md>`_

* Advanced Topics:

  * `Media SDK User Plugins Manual <./doc/mediasdkusr-man.md>`_
  * `Media SDK FEI Manual <./doc/mediasdkfei-man.md>`_
  * `Media SDK HEVC FEI Manual <./doc/mediasdkhevcfei-man.md>`_
  * `MFE Overview <./doc/MFE-Overview.md>`_
  * `HEVC FEI Overview <./doc/HEVC_FEI_overview.pdf>`_
  * `Interlace content support in HEVC encoder <./doc/mediasdk_hevc_interlace_whitepaper.md>`_

Generic samples information is available in `Media Samples Guide <./doc/samples/Media_Samples_Guide_Linux.md>`_

Linux Samples Readme Documents:

* `Sample Multi Transcode <./doc/samples/readme-multi-transcode_linux.md>`_
* `Sample Decode <./doc/samples/readme-decode_linux.md>`_
* `Sample Encode <./doc/samples/readme-encode_linux.md>`_
* `Sample VPP <./doc/samples/readme-vpp_linux.md>`_
* `Metrics Monitor <./doc/samples/readme-metrics_monitor_linux.md>`_

Visit our `Github Wiki <https://github.com/Intel-Media-SDK/MediaSDK/wiki>`_ for the detailed setting and building instructions, runtime tips and other information.

Tutorials
---------

* `Tutorials Overview <./doc/tutorials/mediasdk-tutorials-readme.md>`_
* `Tutorials Command Line Reference <./doc/tutorials/mediasdk-tutorials-cmd-reference.md>`_

Products which use Media SDK
----------------------------

Use Media SDK via popular frameworks:

* `FFmpeg <http://ffmpeg.org/>`_ via `ffmpeg-qsv <https://trac.ffmpeg.org/wiki/Hardware/QuickSync>`_ plugins
* `GStreamer <https://gstreamer.freedesktop.org/>`_ via plugins set included
  into `gst-plugins-bad <https://gitlab.freedesktop.org/gstreamer/gst-plugins-bad>`_

Learn best practises and borrow fragments for final solutions:

* https://github.com/intel/media-delivery

  * This collection of samples demonstrates best practices to achieve optimal video quality and
    performance on Intel GPUs for content delivery networks. Check out the demo, recommended command
    lines and quality and performance measuring tools.

Use Media SDK via other Intel products:

* `OpenVINO Toolkit <https://github.com/openvinotoolkit/openvino>`_

  * This toolkit allows developers to deploy pre-trained deep learning models through a high-level C++ Inference Engine API integrated with application logic.

* `Open Visual Cloud <https://github.com/OpenVisualCloud>`_

  * The Open Visual Cloud is a set of open source software stacks (with full end-to-end sample pipelines) for media, analytics, graphics and immersive media, optimized for cloud native deployment on commercial-off-the-shelf x86 CPU architecture.

System requirements
-------------------

**Operating System:**

* Linux x86-64 fully supported
* Linux x86 only build
* Windows (not all features are supported in Windows build - see Known Limitations for details)

**Software:**

* `LibVA https://github.com/intel/libva)
* VAAPI backend driver:

  * `Intel Media Driver for VAAPI <https://github.com/intel/media-driver>`_

* Some features require CM Runtime library (part of `Intel Media Driver for VAAPI <https://github.com/intel/media-driver>`_ package)

**Hardware:** Intel platforms supported by the `Intel Media Driver for VAAPI <https://github.com/intel/media-driver>`_

Media SDK test and sample applications may require additional software packages (for example, X Server, Wayland, LibDRM, etc.) to be functional.

**Operating System:** Windows **(experimental)**

Requires Microsoft Visual Studio 2017 for building.

How to build
------------

Build steps
~~~~~~~~~~~

Get sources with the following Git* command (pay attention that to get full Media SDK sources bundle it is required to have Git* with `LFS <https://git-lfs.github.com/>`_
support)::

  git clone https://github.com/Intel-Media-SDK/MediaSDK msdk
  cd msdk

To configure and build Media SDK install cmake version 3.6 or later and run the following commands::

  mkdir build && cd build
  cmake ..
  make
  make install

Media SDK depends on a number of packages which are identified and checked for the proper version during configuration stage. Please, make sure to install these packages to satisfy Media SDK requirements. After successful configuration 'make' will build Media SDK binaries and samples. The following cmake configuration options can be used to customize the build:

+--------------------+-----------------------------+----------------------------------------------------------------------------------------------------+
| Option             | Values                      | Description                                                                                        |
+====================+=============================+====================================================================================================+
| API                | master, latest, major.minor | Build mediasdk library with specified API. 'latest'                                                |
|                    |                             | will enable experimental features. 'master' will                                                   |
|                    |                             | configure the most recent available published API                                                  |
|                    |                             | (default: master).                                                                                 |
+--------------------+-----------------------------+----------------------------------------------------------------------------------------------------+
| ENABLE_OPENCL      | ``ON|OFF``                  | Enable OpenCL dependent code to be built (default: ON)                                             |
+--------------------+-----------------------------+----------------------------------------------------------------------------------------------------+
| ENABLE_X11_DRI3    | ``ON|OFF``                  | Enable X11 DRI3 dependent code to be built (default: OFF)                                          |
+--------------------+-----------------------------+----------------------------------------------------------------------------------------------------+
| ENABLE_WAYLAND     | ``ON|OFF``                  | Enable Wayland dependent code to be built (default: OFF)                                           |
+--------------------+-----------------------------+----------------------------------------------------------------------------------------------------+
| ENABLE_ITT         | ``ON|OFF``                  | Enable ITT (VTune) instrumentation support (default: OFF)                                          |
+--------------------+-----------------------------+----------------------------------------------------------------------------------------------------+
| ENABLE_TEXTLOG     | ``ON|OFF``                  | Enable textlog trace support (default: OFF)                                                        |
+--------------------+-----------------------------+----------------------------------------------------------------------------------------------------+
| ENABLE_STAT        | ``ON|OFF``                  | Enable stat trace support (default: OFF)                                                           |
+--------------------+-----------------------------+----------------------------------------------------------------------------------------------------+
| BUILD_ALL          | ``ON|OFF``                  | Build all the BUILD_* targets below (default: OFF)                                                 |
+--------------------+-----------------------------+----------------------------------------------------------------------------------------------------+
| BUILD_RUNTIME      | ``ON|OFF``                  | Build mediasdk runtime, library and plugins (default: ON)                                          |
+--------------------+-----------------------------+----------------------------------------------------------------------------------------------------+
| BUILD_SAMPLES      | ``ON|OFF``                  | Build samples (default: ON)                                                                        |
+--------------------+-----------------------------+----------------------------------------------------------------------------------------------------+
| BUILD_TESTS        | ``ON|OFF``                  | Build unit tests (default: OFF)                                                                    |
+--------------------+-----------------------------+----------------------------------------------------------------------------------------------------+
| USE_SYSTEM_GTEST   | ``ON|OFF``                  | Use system gtest version instead of bundled (default: OFF)                                         |
+--------------------+-----------------------------+----------------------------------------------------------------------------------------------------+
| BUILD_TOOLS        | ``ON|OFF``                  | Build tools (default: OFF)                                                                         |
+--------------------+-----------------------------+----------------------------------------------------------------------------------------------------+
| MFX_ENABLE_KERNELS | ``ON|OFF``                  | Build mediasdk with                                                                                |
|                    |                             | `media shaders <https://github.com/Intel-Media-SDK/MediaSDK/wiki/Media-SDK-Shaders-(EU-Kernels)>`_ |
|                    |                             | support (default: ON)                                                                              |
+--------------------+-----------------------------+----------------------------------------------------------------------------------------------------+

The following cmake settings can be used to adjust search path locations for some components Media SDK build may depend on:

+------------------+-------------------+---------------------------------------------+
| Setting          | Values            | Description                                 |
+==================+===================+=============================================+
| CMAKE_ITT_HOME   | Valid system path | Location of ITT installation,               |
|                  |                   | takes precendence over ``CMAKE_VTUNE_HOME`` |
|                  |                   | (by default not defined)                    |
+------------------+-------------------+---------------------------------------------+
| CMAKE_VTUNE_HOME | Valid system path | Location of VTune installation              |
|                  |                   | (default: /opt/intel/vtune_amplifier)       |
+------------------+-------------------+---------------------------------------------+

Visit our [Github Wiki](https://github.com/Intel-Media-SDK/MediaSDK/wiki) for advanced topics on setting and building Media SDK.

Enabling Instrumentation and Tracing Technology (ITT)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To enable the Instrumentation and Tracing Technology (ITT) API you need to:

* Either install `Intel® VTune™ Amplifier <https://software.intel.com/en-us/intel-vtune-amplifier-xe>`_
* Or manually build an open source version (see `IntelSEAPI <https://github.com/intel/IntelSEAPI/tree/master/ittnotify>`_ for details)

and configure Media SDK with the -DENABLE_ITT=ON. In case of VTune it will be searched in the default location (/opt/intel/vtune_amplifier). You can adjust ITT search path with either CMAKE_ITT_HOME or CMAKE_VTUNE_HOME.

Once Media SDK was built with ITT support, enable it in a runtime creating per-user configuration file ($HOME/.mfx_trace) or a system wide configuration file (/etc/mfx_trace) with the following
content::

  Output=0x10

Known limitations
-----------------

Windows build contains only samples and dispatcher library. MediaSDK library DLL is provided with Windows GFX driver.

Recommendations
---------------

* In case of GCC compiler it is strongly recommended to use GCC version 6 or later since that's the first GCC version which has non-experimental support of C++11 being used in Media SDK.

See also
--------

Intel Media SDK: https://software.intel.com/en-us/media-sdk
