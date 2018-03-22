# Intel® Media SDK open source
**Whats new**
 - Add MCTF support
 - API 1.26

# Intel® Media SDK open source release v1.3!
**Whats new**
 - Add SCD support
 - API 1.25
 - 1:N performance improvements
 - Support open source [Intel® Media Driver for VAAPI](https://github.com/intel/media-driver)
 - Multi-Frame encode APIs support added helping to improve transcoding dencity in multi-stream scenario, following known limitations apply to current implementation:
   - Only H264 Encode is supported at a moment. 
   - Optimized only for Xeon E3 1500 v5 series.
   - Performance can be worse than usual single-frame in next known cases: 
     - 4K resolution encoding.
     - N:N with HEVC decode present in pipeline.
     - cases where framerates differs significantly like 30 and 60 for different streams.
   - Following functionality is not supported with multi-frame encode operation and when set, multiframe will be disabled:
     - When number of slices is controlled by parameters *NumSlice*, *NumSliceI*, *NumSliceP*, *NumSliceB*.
     - ‘intra refresh’ parameters are not supported.
   - MFX_MF_AUTO implemented at a moment.
   - FEI is not supported by open source driver for multiframe operation, proper SDK behavior is not validated.
   - Supported MaxNumFrames is 3, Only 2 will be used for next cases: *EnableMBQP*, *EnableMAD*, *EnableMBForceIntra*, *MBDisableSkipMap*.

# Intel® Media SDK open source release v1.2!
**Whats new**
 - Added User-defined Bitrate Control support
 - Added C++ 11 support
 
**Known limitations**:
 - Copy from video to system memory with `mfxInitParam::GPUCopy = MFX_GPUCOPY_OFF` leads to performance degradation up to x10 comparing to Intel® Media Server Studio release
 - Intel® Media Server Studio [limitations](https://software.intel.com/en-us/articles/intel-media-server-studio-release-notes)
 - 

# Intel® Media SDK open source release v1.1!
**Whats new**
 - Aligned with Intel® Media Server Studio 2017 R3, featuring CentOS 7.3 support
 - Intel® Media Server Studio [release notes](https://software.intel.com/en-us/articles/intel-media-server-studio-release-notes)
 
# Intel® Media SDK first open source release!
