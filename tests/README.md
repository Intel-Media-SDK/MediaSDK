# Test Driver

### Overview

TEst Driver (TED) works with installed MediaSDK. Verification of built but non-installed MediaSDK is not currently supported.

### Prerequisites
Before running tests ensure that required packages are installed on your system:
```
git
git-lfs
python >= 3.6
```

### How To Run

Assuming the following build configuration of MediaSDK:
```sh
export PREFIX=/path/to/installation/directory
export BINDIR=$PREFIX/bin
export LIBDIR=$PREFIX/lib

cmake -DCMAKE_INSTALL_PREFIX=$PREFIX -DCMAKE_INSTALL_LIBDIR=$LIBDIR ..
make && make install
```

Test Driver should be run as follows:
```sh
export PATH=$BINDIR:$PATH
export LD_LIBRARY_PATH=$LIBDIR

python3 ted.py --gold  # to collect reference results
python3 ted.py
```

### How To View Results

High level results in a form PASS/FAIL will be available in a console output.

Full logs will be available at `gold` or `results` folder relative to ted.py location (it depends on `--gold` flag).
