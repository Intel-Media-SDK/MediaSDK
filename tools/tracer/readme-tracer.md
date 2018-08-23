# <center>Intel® Media SDK Tracer</center>

## Overview

Media SDK Tracer is a tool which permits to dump logging information from the calls
of the application to the Media SDK library. Trace log obtained from this tool is a
recommended information to provide to Media SDK team on submitting questions and
issues.

Media SDK tracer tool version is: v1.27.0. Note that 1.27 stands for the Media SDK API
version this particular version of the Tracer is intended for.


## Prerequisites

To use this version of the tracer, it is required to have the following products installed
on the system:
- Intel® Media Server Studio 2018 R2 for Linux Servers
Please, refer to the release notes of the product on how to install it


## Installation

1. Rename real Media SDK library:
```sh
INSTALLDIR=/opt/intel/mediasdk/
cd $INSTALLDIR/lib64/
rm libmfxhw64.so
mv libmfxhw64-p.so.1.27 libmfxhw64-p-REAL.so.1.27
```
2. Create symbolic link to the tracer with the name of Media SDK library:
```sh
cd $INSTALLDIR/tools/tracer
ln -s libmfx-tracer.so libmfxhw64.so
```
3. Generate config file for the tracer:
```sh
$INSTALLDIR/tools/tracer/mfx-tracer-config --default
$INSTALLDIR/tools/tracer/mfx-tracer-config core.type file
$INSTALLDIR/tools/tracer/mfx-tracer-config core.log ~/mfxtracer.log
$INSTALLDIR/tools/tracer/mfx-tracer-config core.lib /opt/intel/mediasdk/lib64/libmfxhw64-p-REAL.so.1.27
```
Following this procedure you will generate a configuration file called '~/.mfxtracer' and set the tracer to dump
logs to '~/mfxtracer_<PID>.log' files. You may adjust your configuration with 'mfx-tracer-config' tool.
Run 'mfx-tracer-config -h' to get full list of supported options.


## Running

Make sure that the tracer library is added to the search path:
export LD_LIBRARY_PATH=$INSTALLDIR/tools/tracer:$LD_LIBRARY_PATH

After each run of some Media SDK based application you should see traces in the configured log files.
Note that the tracer library reads settings from ~/.mfxtracer located in the home directory of a current user.
If you need to run application with 'sudo', copy ~/.mfxtracer file to a home directory of the root user.


## Known issues & limitations

- Not all functionality can be available
- Syslog logger is not supported
- Some API functions may not be covered by the tracing
