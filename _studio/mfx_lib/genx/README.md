# Intel® Media SDK Shaders

This folder contains Shaders (EU Kernels) used in Intel® Media SDK library and some helper tools
to rebuild them. Shaders are provided in 2 forms:
1. Pre-Built c-array
2. CM language sources

See also: [Media SDK Binary Kernels](https://github.com/Intel-Media-SDK/MediaSDK/wiki/Media-SDK-Binary-Kernels) wiki page.

By default Media SDK library is built against pre-built Shaders. Optionally it is
possible to rebuild them from sources configuring with:
```sh
cmake -DBUILD_KERNELS=ON ...
```

## Shaders rebuild prerequisites

In order to rebuild the Shaders it is required to have CM compiler (aka CMC) and
some tools from Intel Graphics Compiler (aga IGC). They can me downloaded and
installed from here:
* https://github.com/intel/cm-compiler
* https://github.com/intel/intel-graphics-compiler

Once you have built CMC and IGC make sure to export CMC_SEARCH_PATH environment
variable pointing to the directory with the structure similar to:
```sh
.
├── bin
│   ├── cmc
│   └── GenX_IR  # tool from IGC
└── include
    └── cm
        ├── cm_atomic.h
        ├── cm_common.h
        ├── cm_dataport.h
        ├── cm_gateway.h
        ├── cm.h
        ├── cm_internal.h
        ├── cm_linear.h
        ├── cm_printf.h
        ├── cm_sampler.h
        ├── cm_send.h
        ├── cm_svm.h
        ├── cm_target.h
        ├── cmtl.h
        ├── cm_traits.h
        ├── cm_util.h
        ├── cm_vme.h
        ├── gen10_vme.h
        ├── gen11_vme.h
        ├── gen7_5_vme.h
        ├── gen8_vme.h
        └── gen9_vme.h
```
