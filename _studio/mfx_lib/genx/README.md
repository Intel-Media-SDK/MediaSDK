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
GenX_IR compiler from Intel Graphics Compiler (aka IGC). They can me downloaded and
installed from here:
* https://github.com/intel/cm-compiler
* https://github.com/intel/intel-graphics-compiler

Once you have built CMC and IGC make sure that `cmc` and `GenX_IR` are in the search path:
```sh
export PATH=/path/to/cmc:/path/to/GenX_IR:$PATH
```
