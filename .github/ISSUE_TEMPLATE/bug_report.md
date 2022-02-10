---
name: Bug report
about: Create a report to help us improve
title: ''
labels: bug
assignees: ''

---

## System information
- CPU information(`cat /proc/cpuinfo | grep "model name" | uniq`):
- GPU information(`lspci -nn | grep -E 'VGA|isplay`):
- Display server if rendering to display(X or wayland):

## Issue behavior
### Describe the current behavior

### Describe the expected behavior

## Debug information
- What's libva/libva-utils/gmmlib/media-driver/Media SDK version?
- Could you confirm whether GPU hardware exist or not by `ls /dev/dri`?
- Could you attach dmesg log if it's GPU hang by `dmesg >dmesg.log 2>&1`?
- Could you provide vainfo log if possible by `vainfo -a >vainfo.log 2>&1`?
- Could you provide strace log if possible by `strace YOUR_CMD >strace.log 2>&1`?
- Could you provide libva trace log if possible? Run cmd `export LIBVA_TRACE=/tmp/libva_trace.log` first then execute the case.
- Media SDK tracer output (https://github.com/Intel-Media-SDK/MediaSDK/blob/master/tools/tracer/README.md)?
- Do you want to contribute a PR? (yes/no):
