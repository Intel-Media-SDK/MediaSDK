# Media SDK Tracer

## Overview

**Media SDK Tracer** is a tool which permits to dump logging information from the calls
of the application to the Media SDK library. Trace log obtained from this tool is a
recommended information to provide to Media SDK team on submitting questions and
issues.

## Installation

Use the following sequence to generate config file for the tracer:

Generate default config file:
```
# $INSTALLDIR/bin/mfx-tracer-config --default
```
Set API level coverage:
```
# $INSTALLDIR/bin/mfx-tracer-config core.level full
```
Set trace type:
```
# $INSTALLDIR/bin/mfx-tracer-config core.type file
```
Set log file:
```
# $INSTALLDIR/bin/mfx-tracer-config core.log ~/mfxtracer.log
```

Following this procedure you will generate a configuration file called `~/.mfxtracer` and set the tracer to dump
logs to `~/mfxtracer_<PID>.log` files. You may adjust your configuration with **mfx-tracer-config** tool.
Run `mfx-tracer-config -h` to get full list of supported options.

## Running

For use the tracer, run the application with **LD_PRELOAD**:

```
LD_PRELOAD=libmfx-tracer.so.1.34 <./some_application>
```

Make sure that the tracer library is added to the search path:

```
export LD_LIBRARY_PATH=$INSTALLDIR/lib:$LD_LIBRARY_PATH
```

After each run of some Media SDK based application you should see traces in the configured log files.
Note that the tracer library reads settings from `~/.mfxtracer` located in the home directory of a current user.
If you need to run application with 'sudo', copy `~/.mfxtracer` file to a home directory of the root user.

## Known issues & limitations

- This is prototype release of the tracer - not all functionality can be available
- Syslog logger is not supported
- Some API functions may not be covered by the tracing