We welcome community contributions to Intel® Media SDK. Thank you for your time!

Please note that review and merge might take some time at this point.

Intel Media SDK is licensed under MIT license. By contributing to the project, you agree to the license and copyright terms therein and release your contribution under these terms.

Steps:
 - Please follow guidelines (see doc/Coding_guidelines.md) in your code
 - Validate that your changes don't break a build. See [build instruction](./README.md#how-to-build)
 - Pass [testing](#testing)
 - Wait while your patchset is reviewed and tested by our internal validation cycle

# Testing

## Requirements

Python* 3.5 is required to run tests. The following instruction assumes that Intel Media SDK cloned to `~/msdk` folder.

## How to test your changes

### 1. Rename system-wide Intel Media SDK libraries:

```sh
cd /opt/intel/mediasdk/lib64
# rename all libmfx*, for example:
sudo mv libmfxhw64.so libmfxhw64.so.default

cd /opt/intel/mediasdk/plugins/
# rename all plugins, for example:
sudo mv libmfx_hevcd_hw64.so libmfx_hevcd_hw64.so.default
sudo mv libmfx_h264la_hw64.so libmfx_h264la_hw64.so.default
sudo mv libmfx_hevce_hw64.so libmfx_hevce_hw64.so.default
sudo mv libmfx_vp8d_hw64.so libmfx_vp8d_hw64.so.default
```

### 2. Build original version of open-source Intel Media SDK and samples

```sh
cd ~/msdk
export MFX_HOME=`pwd`
perl tools/builder/build_mfx.pl --cmake=intel64.make.release
make -j8 -C __cmake/intel64.make.release
```

### 3. Collect gold results

```sh
cd ~/msdk/tests
python3.5 ted.py --gold
```

### 4. Apply changes to Intel Media SDK, commit them, and rebuild library and samples

### 5. Run tests once again to compare with gold results

```sh
python3.5 ted.py
```

All tests should pass. If not, you need to provide an explanation of why streams generated using the modified library are not bit-exact with the original ones.

### 6. Attach generated test_results.json to your pull request.
