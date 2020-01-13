
# OpenCL™ Plug-in Sample

## Overview

**OpenCL™ Plug-in Sample** demonstrates how to use **Media SDK** (hereinafter referred to as "**SDK**") API to implement a video-processing filter through Intel® OpenCL™ technology and make the filter available to participate in an **SDK** pipeline.  
Features 
 * The **OpenCL™ Plug-in Sample** implements 180-degree picture rotation using Intel OpenCL technology. 
 * Sample supports both CPU and GPU OpenCL devices.

## Software Requirements

 * Intel® SDK for OpenCL™ Applications http://software.intel.com/en-us/vcsource/tools/opencl-sdk 
 * See [`<install-folder>/Media_Samples_Guide_Linux.md`](./Media_Samples_Guide_Linux.md)

## Package Contents

**OpenCL™ Plug-in Sample** package contains the following: 

### `<install-folder>/sample_user_modules/rotate_opencl/`

|  |  |
|--|--|
|CMakeLists.txt | CMake configuration file |
|include | Header files for the sample |
|src | Source files for the sample |
 
### `<install-folder>/sample_user_modules/plugin_api/`

|  |  |
|--|--|
|rotate_plugin_api.h | Header file for the rotation interface (defines rotation parameters structure) |
 
### `<install-folder>/sample_user_modules/rotate_opencl/include/`

|  |  |
|--|--|
|sample_plugin_opencl.h | Header file for the rotation plugin class |
 
### `<install-folder>/sample_user_modules/rotate_cpu/src/`

|  |  |
|--|--|
|sample_plugin_opencl.cpp | Source file for the rotation plugin class |
|ocl_rotate.cl | OpenCL kernel file |
 
## How to Build the Application

Refer to [`<install-folder>/Media_Samples_Guide_Linux.md`](./Media_Samples_Guide_Linux.md) for general build instructions. 
Make sure to install Intel® SDK for OpenCL™ Applications prior to building this sample. 

## Running the Software

**OpenCL™ Plug-in Sample** is a shared library which is invoked from the **Encoding Sample** or **Transcoding Sample** with options `–opencl -angle 180`.

While running **Encoding Sample** or **Transcoding Sample** with options `–opencl -angle 180` the file sample_`plugin_opencl.so` and `cl_rotate.cl` must be available. 
See [`<install-folder>/doc/samples/readme-encode_linux.md`](./readme-encode_linux.md) or [`<install-folder>/doc/samples/readme-multi-transcode.md`](./readme-multi-transcode_linux.md) for details.

## Known Limitations

 * This sample supports only picture width aligned at 4.
 
## Legal Information
INFORMATION IN THIS DOCUMENT IS PROVIDED IN CONNECTION WITH INTEL PRODUCTS. NO LICENSE, EXPRESS OR IMPLIED, BY ESTOPPEL OR OTHERWISE, TO ANY INTELLECTUAL PROPERTY RIGHTS IS GRANTED BY THIS DOCUMENT. EXCEPT AS PROVIDED IN INTEL'S TERMS AND CONDITIONS OF SALE FOR SUCH PRODUCTS, INTEL ASSUMES NO LIABILITY WHATSOEVER AND INTEL DISCLAIMS ANY EXPRESS OR IMPLIED WARRANTY, RELATING TO SALE AND/OR USE OF INTEL PRODUCTS INCLUDING LIABILITY OR WARRANTIES RELATING TO FITNESS FOR A PARTICULAR PURPOSE, MERCHANTABILITY, OR INFRINGEMENT OF ANY PATENT, COPYRIGHT OR OTHER INTELLECTUAL PROPERTY RIGHT. 
 
UNLESS OTHERWISE AGREED IN WRITING BY INTEL, THE INTEL PRODUCTS ARE NOT DESIGNED NOR INTENDED FOR ANY APPLICATION IN WHICH THE FAILURE OF THE INTEL PRODUCT COULD CREATE A SITUATION WHERE PERSONAL INJURY OR DEATH MAY OCCUR. 
 
Intel may make changes to specifications and product descriptions at any time, without notice. Designers must not rely on the absence or characteristics of any features or instructions marked "reserved" or "undefined." Intel reserves these for future definition and shall have no responsibility whatsoever for conflicts or incompatibilities arising from future changes to them. The information here is subject to change without notice. Do not finalize a design with this information.  
 
The products described in this document may contain design defects or errors known as errata which may cause the product to deviate from published specifications. Current characterized errata are available on request.  
 
Contact your local Intel sales office or your distributor to obtain the latest specifications and before placing your product order.  
 
Copies of documents which have an order number and are referenced in this document, or other Intel literature, may be obtained by calling 1-800-548-4725, or by visiting Intel's Web Site. 
MPEG is an international standard for video compression/decompression promoted by ISO. Implementations of MPEG CODECs, or MPEG enabled platforms may require licenses from various entities, including Intel Corporation. 
Intel and the Intel logo are trademarks or registered trademarks of Intel Corporation or its subsidiaries in the United States and other countries. 

**Optimization Notice**

Intel's compilers may or may not optimize to the same degree for non-Intel microprocessors for optimizations that are not unique to Intel microprocessors. These optimizations include SSE2, SSE3, and SSE3 instruction sets and other optimizations. Intel does not guarantee the availability, functionality, or effectiveness of any optimization on microprocessors not manufactured by Intel.  
 
Microprocessor-dependent optimizations in this product are intended for use with Intel microprocessors. Certain optimizations not specific to Intel microarchitecture are reserved for Intel microprocessors. Please refer to the applicable product User and Reference Guides for more information regarding the specific instruction sets covered by this notice. 
Notice revision #20110804 