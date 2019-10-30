![](./pic/intel_logo.png)
<br><br><br>
# **System analyzer**

<div style="page-break-before:always" />

[**LEGAL DISCLAIMER**](./header-template.md#legal-disclaimer)

[**Optimization Notice**](./header-template.md#optimization-notice)

<div style="page-break-before:always" />

### Overview
This document describes a utility to help diagnose system and installation issues for the Intel Media-driver and Intel Media SDK.  It is a simple Python script with full source code available.
It is intended as a reference for the kinds of checks to consider from the command line and possibly from within applications. However, this implementation should be considered a prototype/proof of concept -- not a productized tool.

#### Features

When executed, the tool reports back:

Platform readiness: check if processor has necessary GPU components
OS readiness: check if OS can see GPU, and reports versions of kernel, libc, gcc
Install checks for KMD and UMD
Results from runs of small smoke test programs for Media SDK


#### System Requirements
The tool is based on Python 3. Python needs a 'pkgconfig' module. If missing, install it by:

    apt install python3-pip && pip3 install pkgconfig

or

    python3 -m pip install pkgconfig

Additionally, the tools requires g++.
