# Profiles

Profiles are intended to be a collection of build options grouped together to facilitate special usages.

## Fastboot

Profile file fastboot.cmake facilitate building of stripped version of Media SDK tailored to embedded usages

   cmake -DMFX_CONFIG_FILE=../builder/profiles/fastboot.cmake

Library name changed to mfxhw64/32-fastboot. All components disabled to dial down binary size and thus facilitate load time on an ultra-lowpower devices. Olny AVC, HEVC, VP9 and MPEG-2 Decoders left enabled in the library. Unlike regular Media SDK fastboot variant not detected by dispatcher and usually directly linked by application, which should implement minimal subset of dispatcher funcionality. I.e. pass non-default values to the MFXInit() call.