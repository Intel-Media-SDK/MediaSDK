FROM centos:8.1.1911 as api_next_defconfig

# Add mediasdk group and user
RUN groupadd --gid 1000 mediasdk \
    && useradd --uid 1000 --gid mediasdk mediasdk

# Install needed packages
RUN yum -y install epel-release 'dnf-command(config-manager)' \
    && dnf config-manager --set-enabled PowerTools \
    # Common packages
    && dnf -y --enablerepo=PowerTools install ninja-build \
    && yum -y install nano vim mc \
    wget git pkgconfig make python3-pip gcc-c++ \
    # Packages for libva
    libdrm libdrm-devel automake autoconf libtool meson \
    # Packages for test_monitor
    libpciaccess libpciaccess-devel \
    # Packages for rotate_opencl plugin
    ocl-icd ocl-icd-devel \
    # Packages for igc
    bison flex \
    # Packages for pack stage
    ruby-devel rubygems rpm-build \
    && gem install --no-document fpm

# Install docker dependencies
# Install python requirements
RUN wget http://mediasdk.intel.com/builds/docker_dependencies/requirements.txt \
    && pip3 install \
    buildbot-worker \
    -r requirements.txt \
    # Install OpenCL
    && wget http://mediasdk.intel.com/builds/docker_dependencies/public_linux_opencl_runtime_release/neo-20.01.15264.3740-1.x86_64.rpm \
    && rpm -i neo-20.01.15264.3740-1.x86_64.rpm \
    && rm -f neo-20.01.15264.3740-1.x86_64.rpm \
    # Install vtune_amplifier
    && wget http://mediasdk.intel.com/builds/docker_dependencies/vtune_amplifier.tar.gz \
    && mkdir -p /opt/intel \
    && tar xvzf vtune_amplifier.tar.gz -C /opt/intel \
    && rm -f vtune_amplifier.tar.gz

# Preparing work directory
WORKDIR /docker

# Install cmake 3.12.2
# Create symlink for backward compatibility. All builds submitted before 08.04.20 will not correctly rebuilded without it.
RUN yum remove -y cmake \
    && wget https://cmake.org/files/v3.12/cmake-3.12.2-Linux-x86_64.sh \
    && chmod +x cmake-3.12.2-Linux-x86_64.sh \
    && ./cmake-3.12.2-Linux-x86_64.sh --prefix=/usr --skip-license \
    && ln -s /usr/bin/cmake /usr/bin/cmake3

# Install Googletest
RUN git clone https://github.com/google/googletest \
    && cd googletest \
    && mkdir build \
    && cd build \
    && cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_INSTALL_LIBDIR=/usr/lib64 -DCMAKE_CXX_FLAGS="-fPIC -pthread" .. \
    && make \
    && make install

# Target for building full image
FROM api_next_defconfig as centos_8_1

# Install packages for X11 and wayland
RUN yum -y install libX11 libX11-devel libXext libXext-devel libXfixes libXfixes-devel libGL libGL-devel \
    libffi libffi-devel expat expat-devel

# Install wayland
RUN wget https://wayland.freedesktop.org/releases/wayland-1.8.93.tar.xz \
    && tar xf wayland-1.8.93.tar.xz \
    && rm -f wayland-1.8.93.tar.xz \
    && cd wayland-1.8.93 \
    && ./configure --disable-documentation --prefix=/usr --libdir=/usr/lib64 \
    && make \
    && make install
    
ENTRYPOINT ["/entrypoint.sh"]
