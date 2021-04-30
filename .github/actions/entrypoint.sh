#!/bin/sh -l

echo "Hello $1"
time=$(date)

set -ex

# CFLAGS="-O2 -Wformat -Wformat-security -Wall -Werror -D_FORTIFY_SOURCE=2 -fstack-protector-strong"

# echo "Building & installing libva"
# cd libva
# ./autogen.sh --prefix=/usr --libdir=/usr/lib/x86_64-linux-gnu
# make -j$(nproc)
# sudo make install
# make clean
# cd ..

# echo "Building oneVPL"

# cd libmfxgen
# rm -rf build
# mkdir build && cd build
# cmake -DAPI=2.3 -G Ninja -DCMAKE_BUILD_TYPE=$1 -DCMAKE_C_FLAGS_RELEASE="$CFLAGS" -DCMAKE_CXX_FLAGS_RELEASE="$CFLAGS" ..
# ninja
# sudo ninja install
# cd ..
# rm -rf build

# echo "::set-output name=status::ok"



CFLAGS="-O2 -Wformat -Wformat-security -Wall -Werror -D_FORTIFY_SOURCE=2 -fstack-protector-strong"


cmake --version
$CC --version
$CXX --version

cd libva
./autogen.sh --prefix=/usr --libdir=/usr/lib/x86_64-linux-gnu
make -j$(nproc)
make install

cd ../

cd libmfxgen
mkdir build && cd build
cmake -DAPI=latest -DBUILD_ALL=ON -DENABLE_ALL=ON -DENABLE_ITT=OFF -DCMAKE_C_FLAGS_RELEASE="$CFLAGS" -DCMAKE_CXX_FLAGS_RELEASE="$CFLAGS" ..
make -j$(nproc)
make test
make install
