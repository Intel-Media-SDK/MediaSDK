
# VPPPlugin Utility Class

## Overview

**VPPlugin Utility Class** demonstrates how to use **Media SDK** (hereinafter referred to as "**SDK**") API to construct a pipeline from several videoprocessing filters including standard **SDK** VPP and User Defined **SDK** Plugin module.

## Features

**VPPPlugin utility class** implements a pipeline with up to three of the following filters, first and last filter are optional:
_[MFXVideoVPP] -->  MFXPlugin -->  [MFXVideoVPP]_
The entire pipeline is exposed under interface unified as much as possible with the **SDK** interface `MFXVideoVPP`, so the application can dynamically select between standard video processing through `MFXVideoVPP` and multi-filters video processing through VPPPlugin utility class, as demonstrated in the sample application located in `<install-folder>/sample_multi_transcode`. The middle filter in **VPPPlugin utility class** pipeline is the user-defined plugin and must be compiled as a separate dynamic library. See `<installfolder>/sample_user_modules` for examples of this plugin module. 

## Software Requirements 

See [`<install-folder>/Media_Samples_Guide_Linux.md`](./Media_Samples_Guide_Linux.md).
  
## How to Use the Utility Class

**VPPPlugin utility class** is demonstrated as a part of **Transcoding Sample** package, see `<install-folder>/doc/samples/readme-multitranscode.md` for details. VPPPlugin utility class is used once the command-line option `–angle 180` or `–opencl` (they enable user plugins in the transcoding pipeline) is passed to **Transcoding Sample**. 

## Known Limitations 

 * None

## Legal Information

INFORMATION IN THIS DOCUMENT IS PROVIDED IN CONNECTION WITH INTEL PRODUCTS. NO LICENSE, EXPRESS OR IMPLIED, BY ESTOPPEL OR OTHERWISE, TO ANY INTELLECTUAL PROPERTY RIGHTS IS GRANTED BY THIS DOCUMENT.  EXCEPT AS PROVIDED IN INTEL'S TERMS AND CONDITIONS OF SALE FOR SUCH PRODUCTS, INTEL ASSUMES NO LIABILITY WHATSOEVER AND INTEL DISCLAIMS ANY EXPRESS OR IMPLIED WARRANTY, RELATING TO SALE AND/OR USE OF INTEL PRODUCTS INCLUDING LIABILITY OR WARRANTIES RELATING TO FITNESS FOR A PARTICULAR PURPOSE, MERCHANTABILITY, OR INFRINGEMENT OF ANY PATENT, COPYRIGHT OR OTHER INTELLECTUAL PROPERTY RIGHT. 
 
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